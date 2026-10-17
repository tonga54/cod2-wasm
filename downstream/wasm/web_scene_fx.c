#include "common_types.h"
#include <emscripten.h>
#include <math.h>

/* The first full-screen 3D view of a frame renders into an offscreen target
 * (multisampled when r_aaSamples > 1). Before the first 2D draw it is composed
 * into the canvas: CoD2's DX9 glow (r_glow*), a CoD4-style viewmodel depth of
 * field while aiming, contrast adaptive sharpening and a light film grade.
 * The HUD and any later view draw straight into the canvas. */

extern const dvar_t *r_glow;
extern const dvar_t *r_glowRadius[2];
extern const dvar_t *r_glowBloomIntensity[2];
extern const dvar_t *r_glowBloomCutoff;
extern const dvar_t *r_glowBloomDesaturation;
extern const dvar_t *r_fullbright;
extern const dvar_t *r_aaSamples;
extern const dvar_t *r_dof_enable;
extern const dvar_t *r_filmTweakEnable;

enum {
    SCENE_FX_GLOW0,
    SCENE_FX_GLOW1,
    SCENE_FX_CUTOFF,
    SCENE_FX_DESATURATION,
    SCENE_FX_RADIUS0,
    SCENE_FX_RADIUS1,
    SCENE_FX_ADS,
    SCENE_FX_VM_START,
    SCENE_FX_VM_END,
    SCENE_FX_NEAR_CLIP,
    SCENE_FX_NEAR_BLUR,
    SCENE_FX_SHARPEN,
    SCENE_FX_SATURATION,
    SCENE_FX_CONTRAST,
    SCENE_FX_FILM,
    SCENE_FX_PARAM_COUNT
};

static float s_adsFrac;
static float s_vmStart = 6.0f;
static float s_vmEnd = 15.0f;
static float s_nearClip = 0.1f;
static int s_active;
static int s_frameUsed;

EM_JS(int, WebSceneFx_Bind, (int width, int height, int samples), {
    if (typeof GLctx === "undefined" || !GLctx || !GLctx.blitFramebuffer) return 0;
    var gl = GLctx;
    var fx = Module.cod2SceneFx;
    if (!fx || fx.gl !== gl) {
        fx = Module.cod2SceneFx = {
            gl: gl, failed: false, width: 0, height: 0, requested: -1, samples: 0,
            t: null, programs: null, vao: null,
            maxSamples: gl.getParameter(gl.MAX_SAMPLES) | 0,
            discard: [gl.COLOR_ATTACHMENT0, gl.DEPTH_STENCIL_ATTACHMENT]
        };
        fx.build = function() {
            var header = "#version 300 es\nprecision mediump float;\nin highp vec2 v_uv;\nout vec4 o;\n";
            var vs = gl.createShader(gl.VERTEX_SHADER);
            gl.shaderSource(vs, "#version 300 es\nout highp vec2 v_uv;\nvoid main() {\n" +
                "  vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n" +
                "  v_uv = p;\n  gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);\n}\n");
            gl.compileShader(vs);
            var previous = gl.getParameter(gl.CURRENT_PROGRAM);
            var programs = {}, ok = true;
            var link = function(name, body, samplers, uniforms) {
                var fs = gl.createShader(gl.FRAGMENT_SHADER);
                gl.shaderSource(fs, header + body);
                gl.compileShader(fs);
                var program = gl.createProgram();
                gl.attachShader(program, vs);
                gl.attachShader(program, fs);
                gl.linkProgram(program);
                if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
                    console.warn("[scene-fx] " + name + " failed: " + gl.getProgramInfoLog(program) +
                        " " + gl.getShaderInfoLog(vs) + " " + gl.getShaderInfoLog(fs));
                    ok = false;
                    return;
                }
                gl.useProgram(program);
                var entry = { program: program };
                for (var s = 0; s < samplers.length; ++s)
                    gl.uniform1i(gl.getUniformLocation(program, samplers[s]), 8 + s);
                for (var u = 0; u < uniforms.length; ++u)
                    entry[uniforms[u]] = gl.getUniformLocation(program, uniforms[u]);
                programs[name] = entry;
            };
            var quarter = "  vec3 c = texture(u_src, v_uv + u_texel * vec2(-1.0, -1.0)).rgb;\n" +
                "  vec3 d = texture(u_src, v_uv + u_texel * vec2(1.0, -1.0)).rgb;\n" +
                "  vec3 e = texture(u_src, v_uv + u_texel * vec2(-1.0, 1.0)).rgb;\n" +
                "  vec3 f = texture(u_src, v_uv + u_texel * vec2(1.0, 1.0)).rgb;\n";
            /* Every pass costs a fixed GPU setup whatever its size, so the glow
             * and the aim blur each use one 2D pass instead of separable ones. */
            link("bright", "uniform sampler2D u_src;\nuniform highp vec2 u_texel;\nuniform vec3 u_bright;\n" +
                "vec3 bright(vec2 p) {\n" +
                "  vec3 c = texture(u_src, v_uv + u_texel * p).rgb;\n" +
                "  float w = clamp((max(c.r, max(c.g, c.b)) - u_bright.x) * u_bright.y, 0.0, 1.0);\n" +
                "  return c * (w * w);\n}\n" +
                "void main() {\n" +
                "  vec3 b = vec3(0.0);\n" +
                "  for (int y = -3; y <= 3; y += 2)\n" +
                "    for (int x = -3; x <= 3; x += 2) b += bright(vec2(x, y));\n" +
                "  b *= 0.0625;\n" +
                "  o = vec4(mix(b, vec3(dot(b, vec3(0.299, 0.587, 0.114))), u_bright.z), 1.0);\n}\n",
                ["u_src"], ["u_texel", "u_bright"]);
            link("bloom", "uniform sampler2D u_src;\nuniform highp vec2 u_texel;\nuniform vec3 u_narrow;\n" +
                "uniform vec3 u_wideOffset;\nuniform vec4 u_wideWeight;\nuniform vec2 u_mix;\n" +
                "void main() {\n" +
                "  float no[3] = float[3](-u_narrow.x, 0.0, u_narrow.x);\n" +
                "  float nw[3] = float[3](u_narrow.z, u_narrow.y, u_narrow.z);\n" +
                "  float wo[7] = float[7](-u_wideOffset.z, -u_wideOffset.y, -u_wideOffset.x, 0.0,\n" +
                "    u_wideOffset.x, u_wideOffset.y, u_wideOffset.z);\n" +
                "  float ww[7] = float[7](u_wideWeight.w, u_wideWeight.z, u_wideWeight.y, u_wideWeight.x,\n" +
                "    u_wideWeight.y, u_wideWeight.z, u_wideWeight.w);\n" +
                "  vec3 narrow = vec3(0.0), wide = vec3(0.0);\n" +
                "  for (int y = 0; y < 3; ++y)\n" +
                "    for (int x = 0; x < 3; ++x)\n" +
                "      narrow += texture(u_src, v_uv + vec2(no[x], no[y]) * u_texel).rgb * (nw[x] * nw[y]);\n" +
                "  for (int y = 0; y < 7; ++y)\n" +
                "    for (int x = 0; x < 7; ++x)\n" +
                "      wide += texture(u_src, v_uv + vec2(wo[x], wo[y]) * u_texel).rgb * (ww[x] * ww[y]);\n" +
                "  o = vec4(narrow * u_mix.x + wide * u_mix.y, 1.0);\n}\n",
                ["u_src"], ["u_texel", "u_narrow", "u_wideOffset", "u_wideWeight", "u_mix"]);
            link("dofBlur", "uniform sampler2D u_src;\nuniform highp vec2 u_step;\n" +
                "void main() {\n" +
                "  float k[5] = float[5](0.03515625, 0.328125, 0.2734375, 0.328125, 0.03515625);\n" +
                "  float d[5] = float[5](-3.1111111, -1.3333333, 0.0, 1.3333333, 3.1111111);\n" +
                "  vec4 sum = vec4(0.0);\n" +
                "  for (int y = 0; y < 5; ++y)\n" +
                "    for (int x = 0; x < 5; ++x)\n" +
                "      sum += texture(u_src, v_uv + vec2(d[x], d[y]) * u_step) * (k[x] * k[y]);\n" +
                "  o = sum;\n}\n",
                ["u_src"], ["u_step"]);
            var coc = "float coc(highp float d) {\n" +
                "  if (d >= 0.2) return 0.0;\n" +
                "  highp float z = u_dof.y / max(u_dof.x - d * 5.0, 0.00001);\n" +
                "  return clamp((u_dof.z - z) * u_dof.w, 0.0, 1.0);\n}\n";
            link("dof", "uniform sampler2D u_src;\nuniform highp sampler2D u_depth;\n" +
                "uniform highp vec2 u_texel;\nuniform highp vec4 u_dof;\n" + coc +
                "float cocAt(ivec2 p, ivec2 m) { return coc(texelFetch(u_depth, min(p, m), 0).r); }\n" +
                "void main() {\n" + quarter +
                "  ivec2 b = ivec2(gl_FragCoord.xy) * 4;\n" +
                "  ivec2 m = textureSize(u_depth, 0) - 1;\n" +
                "  float k = max(max(cocAt(b, m), cocAt(b + ivec2(3, 0), m)),\n" +
                "    max(max(cocAt(b + ivec2(0, 3), m), cocAt(b + ivec2(3, 3), m)), cocAt(b + ivec2(2, 2), m)));\n" +
                "  o = vec4(0.25 * (c + d + e + f), k);\n}\n",
                ["u_src", "u_depth"], ["u_texel", "u_dof"]);
            link("compose", "uniform sampler2D u_scene;\nuniform sampler2D u_bloom;\n" +
                "uniform highp sampler2D u_depth;\nuniform sampler2D u_dofMed;\nuniform sampler2D u_dofLarge;\n" +
                "uniform highp vec4 u_px;\nuniform vec4 u_glow;\nuniform highp vec4 u_dof;\nuniform vec4 u_dof2;\nuniform vec4 u_film;\n" +
                coc +
                "void main() {\n" +
                "  vec3 c = texture(u_scene, v_uv).rgb;\n" +
                "  vec3 col = c;\n" +
                "  if (u_film.x > 0.0) {\n" +
                "    vec3 n = texture(u_scene, v_uv + vec2(0.0, u_px.y)).rgb;\n" +
                "    vec3 s = texture(u_scene, v_uv - vec2(0.0, u_px.y)).rgb;\n" +
                "    vec3 e = texture(u_scene, v_uv + vec2(u_px.x, 0.0)).rgb;\n" +
                "    vec3 w = texture(u_scene, v_uv - vec2(u_px.x, 0.0)).rgb;\n" +
                "    vec3 lo = min(c, min(min(n, s), min(e, w)));\n" +
                "    vec3 hi = max(c, max(max(n, s), max(e, w)));\n" +
                "    vec3 wt = -u_film.x * sqrt(clamp(min(lo, 1.0 - hi) / max(hi, vec3(0.0001)), 0.0, 1.0));\n" +
                "    col = clamp((c + (n + s + e + w) * wt) / (1.0 + 4.0 * wt), 0.0, 1.0);\n" +
                "  }\n" +
                "  if (u_dof2.x > 0.0) {\n" +
                "    highp float depth = texelFetch(u_depth, ivec2(gl_FragCoord.xy), 0).r;\n" +
                "    vec4 large = texture(u_dofLarge, v_uv);\n" +
                "    float k;\n" +
                "    if (depth < 0.2) {\n" +
                "      k = coc(depth);\n" +
                "    } else {\n" +
                "      vec2 centered = (v_uv - 0.5) * vec2(u_px.z, 1.0);\n" +
                "      k = clamp(2.0 * large.a, 0.0, 1.0) * smoothstep(u_dof2.y, u_dof2.z, length(centered));\n" +
                "    }\n" +
                "    k *= u_dof2.x;\n" +
                "    if (k > 0.003) {\n" +
                "      vec3 small = 0.25 * (texture(u_scene, v_uv + u_px.xy * vec2(0.5, 1.5)).rgb +\n" +
                "        texture(u_scene, v_uv + u_px.xy * vec2(-1.5, 0.5)).rgb +\n" +
                "        texture(u_scene, v_uv + u_px.xy * vec2(-0.5, -1.5)).rgb +\n" +
                "        texture(u_scene, v_uv + u_px.xy * vec2(1.5, -0.5)).rgb);\n" +
                "      vec3 med = texture(u_dofMed, v_uv).rgb;\n" +
                "      col = k < 0.25 ? mix(col, small, k * 4.0) :\n" +
                "        (k < 0.55 ? mix(small, med, (k - 0.25) * 3.3333333) : mix(med, large.rgb, (k - 0.55) * 2.2222222));\n" +
                "    }\n" +
                "  }\n" +
                "  if (u_glow.x > 0.0)\n" +
                "    col = 1.0 - (1.0 - col) * (1.0 - min(texture(u_bloom, v_uv).rgb, vec3(1.0)));\n" +
                "  if (u_film.w > 0.0) {\n" +
                "    float l = dot(col, vec3(0.2126, 0.7152, 0.0722));\n" +
                "    col = mix(vec3(l), col, u_film.y);\n" +
                "    col = clamp(col * mix(vec3(0.975, 0.99, 1.03), vec3(1.025, 1.005, 0.965), smoothstep(0.05, 0.75, l)), 0.0, 1.0);\n" +
                "    col += u_film.z * col * (1.0 - col) * (col - 0.35) * 1.5384615;\n" +
                "  }\n" +
                "  o = vec4(col, 1.0);\n}\n",
                ["u_scene", "u_bloom", "u_depth", "u_dofMed", "u_dofLarge"],
                ["u_px", "u_glow", "u_dof", "u_dof2", "u_film"]);
            gl.useProgram(previous);
            if (!ok) return false;
            fx.programs = programs;
            fx.vao = gl.createVertexArray();
            return true;
        };
        fx.release = function() {
            var t = fx.t;
            fx.t = null;
            fx.width = fx.height = fx.samples = 0;
            if (!t) return;
            for (var i = 0; i < t.framebuffers.length; ++i) gl.deleteFramebuffer(t.framebuffers[i]);
            for (var j = 0; j < t.renderbuffers.length; ++j) gl.deleteRenderbuffer(t.renderbuffers[j]);
            for (var k = 0; k < t.textures.length; ++k) gl.deleteTexture(t.textures[k]);
        };
        fx.allocate = function(w, h, samples) {
            fx.release();
            if (!fx.programs && !fx.build()) return false;
            samples = samples > 1 ? Math.min(samples, fx.maxSamples) : 0;
            var t = {
                qw: Math.max(1, Math.ceil(w / 4)), qh: Math.max(1, Math.ceil(h / 4)),
                ew: Math.max(1, Math.ceil(w / 8)), eh: Math.max(1, Math.ceil(h / 8)),
                textures: [], framebuffers: [], renderbuffers: [], ms: null, q: [], qFbo: [], e: [], eFbo: []
            };
            var ok = true;
            var previousUnit = gl.getParameter(gl.ACTIVE_TEXTURE);
            gl.activeTexture(gl.TEXTURE8);
            var texture = function(format, tw, th, filter) {
                var id = gl.createTexture();
                t.textures.push(id);
                gl.bindTexture(gl.TEXTURE_2D, id);
                gl.texStorage2D(gl.TEXTURE_2D, 1, format, tw, th);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, filter);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, filter);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
                return id;
            };
            var renderbuffer = function(format) {
                var id = gl.createRenderbuffer();
                t.renderbuffers.push(id);
                gl.bindRenderbuffer(gl.RENDERBUFFER, id);
                gl.renderbufferStorageMultisample(gl.RENDERBUFFER, samples, format, w, h);
                return id;
            };
            var framebuffer = function(color, depth, colorBuffer, depthBuffer) {
                var id = gl.createFramebuffer();
                t.framebuffers.push(id);
                gl.bindFramebuffer(gl.FRAMEBUFFER, id);
                if (color) gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, color, 0);
                if (depth) gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.DEPTH_STENCIL_ATTACHMENT, gl.TEXTURE_2D, depth, 0);
                if (colorBuffer) gl.framebufferRenderbuffer(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.RENDERBUFFER, colorBuffer);
                if (depthBuffer) gl.framebufferRenderbuffer(gl.FRAMEBUFFER, gl.DEPTH_STENCIL_ATTACHMENT, gl.RENDERBUFFER, depthBuffer);
                if (gl.checkFramebufferStatus(gl.FRAMEBUFFER) !== gl.FRAMEBUFFER_COMPLETE) ok = false;
                return id;
            };
            /* RGB8 keeps the canvas behaviour of alpha:false: destination alpha reads as one. */
            t.color = texture(gl.RGB8, w, h, gl.LINEAR);
            t.depth = texture(gl.DEPTH24_STENCIL8, w, h, gl.NEAREST);
            t.scene = framebuffer(t.color, t.depth, null, null);
            if (samples > 1) {
                t.ms = framebuffer(null, null, renderbuffer(gl.RGB8), renderbuffer(gl.DEPTH24_STENCIL8));
                gl.bindRenderbuffer(gl.RENDERBUFFER, null);
            }
            for (var i = 0; i < 2; ++i) {
                t.q.push(texture(gl.RGBA8, t.qw, t.qh, gl.LINEAR));
                t.qFbo.push(framebuffer(t.q[i], null, null, null));
                t.e.push(texture(gl.RGBA8, t.ew, t.eh, gl.LINEAR));
                t.eFbo.push(framebuffer(t.e[i], null, null, null));
            }
            gl.bindTexture(gl.TEXTURE_2D, null);
            gl.activeTexture(previousUnit);
            gl.bindFramebuffer(gl.FRAMEBUFFER, null);
            fx.t = t;
            if (!ok) {
                fx.release();
                return false;
            }
            fx.width = w;
            fx.height = h;
            fx.samples = samples;
            return true;
        };
        fx.texture = function(unit, texture) {
            gl.activeTexture(gl.TEXTURE8 + unit);
            gl.bindTexture(gl.TEXTURE_2D, texture);
        };
        fx.pass = function(program, framebuffer, w, h) {
            gl.useProgram(program.program);
            gl.bindFramebuffer(gl.FRAMEBUFFER, framebuffer);
            gl.viewport(0, 0, w, h);
            gl.drawArrays(gl.TRIANGLES, 0, 3);
        };
        /* Normalized Gaussian over texels 0..2 * pairs, folded into bilinear taps:
         * [center weight, pair weights...] and [pair offsets...]. */
        fx.kernel = function(sigma, pairs) {
            var g = [], total = 0;
            for (var i = 0; i <= pairs * 2; ++i) {
                g.push(sigma > 0.05 ? Math.exp(-i * i / (2 * sigma * sigma)) : (i ? 0 : 1));
                total += i ? 2 * g[i] : g[i];
            }
            var weights = [g[0] / total], offsets = [];
            for (var p = 0; p < pairs; ++p) {
                var a = g[2 * p + 1], b = g[2 * p + 2];
                weights.push((a + b) / total);
                offsets.push(2 * p + 1 + (a + b > 0.000001 ? b / (a + b) : 0));
            }
            return { weights: weights, offsets: offsets };
        };
        fx.compose = function(o) {
            var t = fx.t, P = fx.programs, H = HEAPF32;
            if (!t || !P) return;
            var w = fx.width, h = fx.height, qw = t.qw, qh = t.qh, ew = t.ew, eh = t.eh;
            var glow0 = H[o], glow1 = H[o + 1], ads = H[o + 6];
            var glow = glow0 > 0 || glow1 > 0, dof = ads > 0.004;
            var lines = h / 480;
            var program = gl.getParameter(gl.CURRENT_PROGRAM);
            var vao = gl.getParameter(gl.VERTEX_ARRAY_BINDING);
            var unit = gl.getParameter(gl.ACTIVE_TEXTURE);
            var viewport = gl.getParameter(gl.VIEWPORT);
            var caps = [gl.SCISSOR_TEST, gl.BLEND, gl.DEPTH_TEST, gl.CULL_FACE, gl.STENCIL_TEST, gl.POLYGON_OFFSET_FILL];
            var enabled = [];
            for (var c = 0; c < caps.length; ++c) {
                enabled.push(gl.isEnabled(caps[c]));
                gl.disable(caps[c]);
            }
            if (t.ms) {
                gl.bindFramebuffer(gl.READ_FRAMEBUFFER, t.ms);
                gl.bindFramebuffer(gl.DRAW_FRAMEBUFFER, t.scene);
                gl.blitFramebuffer(0, 0, w, h, 0, 0, w, h,
                    dof ? (gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT) : gl.COLOR_BUFFER_BIT, gl.NEAREST);
                gl.invalidateFramebuffer(gl.READ_FRAMEBUFFER, fx.discard);
            }
            /* Reading the write masks back would stall on the GPU process. Every
             * D3D9 clear and draw sets both masks again, and these passes run
             * without depth testing, so only the color mask is forced here. */
            gl.colorMask(true, true, true, true);
            gl.bindVertexArray(fx.vao);
            if (glow) {
                var cutoff = H[o + 2], knee = Math.max(cutoff - 0.3, 0);
                gl.useProgram(P.bright.program);
                gl.uniform2f(P.bright.u_texel, 1 / w, 1 / h);
                gl.uniform3f(P.bright.u_bright, knee, 1 / Math.max(cutoff - knee + 0.05, 0.05), H[o + 3]);
                fx.texture(0, t.color);
                fx.pass(P.bright, t.eFbo[0], ew, eh);
                /* r_glowRadius is in 480-line pixels; the kernels run at 1/8 scale. */
                var narrow = fx.kernel(Math.min(H[o + 4] * lines / 8, 1.2), 1);
                var wide = fx.kernel(Math.min(H[o + 5] * lines / 8, 2.75), 3);
                var B = P.bloom;
                gl.useProgram(B.program);
                gl.uniform2f(B.u_texel, 1 / ew, 1 / eh);
                gl.uniform3f(B.u_narrow, narrow.offsets[0], narrow.weights[0], narrow.weights[1]);
                gl.uniform3f(B.u_wideOffset, wide.offsets[0], wide.offsets[1], wide.offsets[2]);
                gl.uniform4f(B.u_wideWeight, wide.weights[0], wide.weights[1], wide.weights[2], wide.weights[3]);
                gl.uniform2f(B.u_mix, glow0, glow1);
                fx.texture(0, t.e[0]);
                fx.pass(B, t.eFbo[1], ew, eh);
            }
            var vmStart = H[o + 7], vmEnd = H[o + 8];
            var invRange = 1 / Math.max(vmEnd - vmStart, 0.5);
            if (dof) {
                gl.useProgram(P.dof.program);
                gl.uniform2f(P.dof.u_texel, 1 / w, 1 / h);
                gl.uniform4f(P.dof.u_dof, 0.9995002746582031, H[o + 9], vmEnd, invRange);
                fx.texture(0, t.color);
                fx.texture(1, t.depth);
                fx.pass(P.dof, t.qFbo[0], qw, qh);
                var near = Math.min(Math.max(H[o + 10] * lines * 0.6 / 4 / 1.4142, 0.25), 2.5);
                gl.useProgram(P.dofBlur.program);
                gl.uniform2f(P.dofBlur.u_step, near / qw, near / qh);
                fx.texture(0, t.q[0]);
                fx.pass(P.dofBlur, t.qFbo[1], qw, qh);
            }
            var C = P.compose;
            gl.useProgram(C.program);
            gl.uniform4f(C.u_px, 1 / w, 1 / h, w / h, 0);
            gl.uniform4f(C.u_glow, glow ? 1 : 0, 0, 0, 0);
            gl.uniform4f(C.u_dof, 0.9995002746582031, H[o + 9], vmEnd, invRange);
            gl.uniform4f(C.u_dof2, dof ? ads : 0, 0.07, 0.17, 0);
            gl.uniform4f(C.u_film, H[o + 11], H[o + 12], H[o + 13], H[o + 14]);
            var sources = [t.color, t.e[1], t.depth, t.q[0], t.q[1]];
            for (var s = 0; s < sources.length; ++s) fx.texture(s, sources[s]);
            fx.pass(C, null, w, h);
            for (var u = sources.length - 1; u >= 0; --u) fx.texture(u, null);
            gl.bindVertexArray(vao);
            gl.useProgram(program);
            gl.activeTexture(unit);
            gl.viewport(viewport[0], viewport[1], viewport[2], viewport[3]);
            for (var r = 0; r < caps.length; ++r)
                if (enabled[r]) gl.enable(caps[r]);
        };
    }
    if (fx.failed || width <= 0 || height <= 0) return 0;
    if (fx.width !== width || fx.height !== height || fx.requested !== samples) {
        var ok = fx.allocate(width, height, samples);
        if (!ok && samples > 1) ok = fx.allocate(width, height, 1);
        fx.requested = samples;
        if (!ok) {
            fx.failed = true;
            console.warn("[scene-fx] offscreen scene target unavailable; drawing directly to the canvas");
            return 0;
        }
    }
    gl.bindFramebuffer(gl.FRAMEBUFFER, fx.t.ms || fx.t.scene);
    return 1;
});

EM_JS(void, WebSceneFx_Compose, (const float *params), {
    var fx = Module.cod2SceneFx;
    if (fx && fx.t) fx.compose(params >>> 2);
});

static float WebSceneFx_Clamp(float value, float lo, float hi)
{
    if (!isfinite(value) || value < lo)
        return lo;
    return value > hi ? hi : value;
}

static int WebSceneFx_Enabled(const dvar_t *dvar)
{
    return dvar && dvar->current.enabled;
}

static int WebSceneFx_GlowEnabled(void)
{
    return WebSceneFx_Enabled(r_glow) && !WebSceneFx_Enabled(r_fullbright);
}

static int WebSceneFx_Samples(void)
{
    return r_aaSamples ? r_aaSamples->current.integer : 1;
}

void WebSceneFx_SetAds(float adsFrac, float viewmodelStart, float viewmodelEnd)
{
    s_adsFrac = WebSceneFx_Clamp(adsFrac, 0.0f, 1.0f);
    s_vmEnd = WebSceneFx_Clamp(viewmodelEnd, 2.0f, 64.0f);
    s_vmStart = WebSceneFx_Clamp(viewmodelStart, 0.0f, s_vmEnd - 0.5f);
}

static void WebSceneFx_FillParams(float *params)
{
    int glow = WebSceneFx_GlowEnabled();
    int film = WebSceneFx_Enabled(r_filmTweakEnable);
    int dof = WebSceneFx_Enabled(r_dof_enable);

    params[SCENE_FX_GLOW0] = glow && r_glowBloomIntensity[0] ?
        WebSceneFx_Clamp(r_glowBloomIntensity[0]->current.value, 0.0f, 8.0f) * 0.42f : 0.0f;
    params[SCENE_FX_GLOW1] = glow && r_glowBloomIntensity[1] ?
        WebSceneFx_Clamp(r_glowBloomIntensity[1]->current.value, 0.0f, 8.0f) * 0.30f : 0.0f;
    params[SCENE_FX_CUTOFF] = r_glowBloomCutoff ?
        WebSceneFx_Clamp(r_glowBloomCutoff->current.value, 0.0f, 0.99f) : 0.9f;
    params[SCENE_FX_DESATURATION] = r_glowBloomDesaturation ?
        WebSceneFx_Clamp(r_glowBloomDesaturation->current.value, 0.0f, 1.0f) : 0.75f;
    params[SCENE_FX_RADIUS0] = r_glowRadius[0] ?
        WebSceneFx_Clamp(r_glowRadius[0]->current.value, 0.0f, 32.0f) : 5.0f;
    params[SCENE_FX_RADIUS1] = r_glowRadius[1] ?
        WebSceneFx_Clamp(r_glowRadius[1]->current.value, 0.0f, 32.0f) : 12.0f;
    params[SCENE_FX_ADS] = dof ? s_adsFrac : 0.0f;
    params[SCENE_FX_VM_START] = s_vmStart;
    params[SCENE_FX_VM_END] = s_vmEnd;
    params[SCENE_FX_NEAR_CLIP] = s_nearClip;
    params[SCENE_FX_NEAR_BLUR] = 6.0f;
    params[SCENE_FX_SHARPEN] = film ? 0.147f : 0.0f;
    params[SCENE_FX_SATURATION] = 0.96f;
    params[SCENE_FX_CONTRAST] = film ? 0.35f : 0.0f;
    params[SCENE_FX_FILM] = film ? 1.0f : 0.0f;
}

void WebSceneFx_EndView(void)
{
    float params[SCENE_FX_PARAM_COUNT];

    if (!s_active)
        return;
    s_active = 0;
    WebSceneFx_FillParams(params);
    WebSceneFx_Compose(params);
}

void WebSceneFx_BeginView(int x, int y, int width, int height, int screenWidth, int screenHeight,
                          float depthHackNearClip)
{
    int samples;

    WebSceneFx_EndView();
    if (s_frameUsed || x || y || width != screenWidth || height != screenHeight || width <= 0 || height <= 0)
        return;
    s_frameUsed = 1;
    /* r_fullbright clears the frame buffer before it begins the view. */
    if (WebSceneFx_Enabled(r_fullbright))
        return;
    samples = WebSceneFx_Samples();
    if (!WebSceneFx_GlowEnabled() && !WebSceneFx_Enabled(r_filmTweakEnable) && samples <= 1 &&
        !(WebSceneFx_Enabled(r_dof_enable) && s_adsFrac > 0.0f))
        return;
    s_nearClip = isfinite(depthHackNearClip) && depthHackNearClip > 0.0f ? depthHackNearClip : 0.1f;
    s_active = WebSceneFx_Bind(width, height, samples);
}

void WebSceneFx_EndFrame(void)
{
    WebSceneFx_EndView();
    s_frameUsed = 0;
}
