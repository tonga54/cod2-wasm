#include "common_types.h"
#include <emscripten.h>
#include <math.h>
#include <string.h>

/* The first full-screen 3D view of a frame renders into an offscreen target
 * (multisampled when r_aaSamples > 1). Before the first 2D draw it is composed
 * into the canvas: CoD2's DX9 glow (r_glow*), WWII-style camera motion blur
 * (r_motionBlur), a CoD4-style depth of field on the viewmodel and the near
 * world while aiming, contrast adaptive sharpening and a light film grade.
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
extern const dvar_t *r_motionBlur;

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
    SCENE_FX_WORLD_NEAR_CLIP,
    SCENE_FX_NEAR_END,
    SCENE_FX_WORLD_BLUR,
    /* The camera's motion over the exposure as a multiple of its motion since
     * the last frame; 0 draws no motion blur. */
    SCENE_FX_MOTION,
    /* Column-major rotation, then offset, taking a point from this view's
     * space into the last frame's. */
    SCENE_FX_MOTION_ROTATION,
    SCENE_FX_MOTION_MOVE = SCENE_FX_MOTION_ROTATION + 9,
    /* The projection's x and y scales. */
    SCENE_FX_MOTION_PROJECTION = SCENE_FX_MOTION_MOVE + 3,
    /* How much of the whole view blurs behind a screen-filling menu. */
    SCENE_FX_BACKDROP = SCENE_FX_MOTION_PROJECTION + 2,
    SCENE_FX_PARAM_COUNT
};

/* InfinitePerspectiveMatrix's depth scale times the default r_znear of 4. */
#define SCENE_FX_DEFAULT_WORLD_NEAR_CLIP (0.9995002746582031f * 4.0f)

/* Milliseconds of camera motion the blur spans, at any frame rate. */
#define SCENE_FX_MOTION_EXPOSURE 10.0
/* Two views further apart in time or space than these are a cut: a spawn,
 * a teleport or a new killcam, not motion. */
#define SCENE_FX_MOTION_MAX_GAP 100.0
#define SCENE_FX_MOTION_MAX_TURN 0.35f
#define SCENE_FX_MOTION_MAX_MOVE 48.0f
/* Below these, a turn in radians or a move in units, the camera stands still. */
#define SCENE_FX_MOTION_MIN_TURN 0.0002f
#define SCENE_FX_MOTION_MIN_MOVE 0.05f

static float s_adsFrac;
static float s_vmStart = 6.0f;
static float s_vmEnd = 15.0f;
static float s_nearEnd = 512.0f;
static float s_nearClip = 0.1f;
static float s_worldNearClip = SCENE_FX_DEFAULT_WORLD_NEAR_CLIP;
static int s_active;
static int s_frameUsed;
static float s_aimFrac;
static float s_cameraView[16];
static float s_cameraProjection[2];
static int s_cameraPending;
static float s_lastView[16];
static double s_lastViewTime;
static int s_lastViewValid;
static float s_motionScale;
static float s_motionRotation[9];
static float s_motionMove[3];
static float s_motionProjection[2];
static float s_backdropFrac;

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
            /* Blur radius as a fraction of the largest. CoD4's r_dof_bias of 0.5
             * squares the viewmodel's circle of confusion, so the hand ahead of
             * the receiver stays crisper than the receiver. The viewmodel draws
             * into depth [0, 0.2] with the depth hack near clip. */
            var coc = "float vmBlur(highp float d) {\n" +
                "  if (d >= 0.2) return 0.0;\n" +
                "  highp float z = u_dof.y / max(u_dof.x - d * 5.0, 0.00001);\n" +
                "  float c = clamp((u_dof.z - z) * u_dof.w, 0.0, 1.0);\n" +
                "  return c * c;\n}\n";
            var nearWorld = "float worldBlur(highp float d) {\n" +
                "  highp float z = u_near.x / max(u_dof.x - d, 0.000001);\n" +
                "  float c = clamp((u_near.y - z) * u_near.z, 0.0, 1.0);\n" +
                "  return c * c * u_near.w;\n}\n";
            link("dof", "uniform sampler2D u_src;\nuniform highp sampler2D u_depth;\n" +
                "uniform highp vec2 u_texel;\nuniform highp vec4 u_dof;\n" + coc +
                "float cocAt(ivec2 p, ivec2 m) { return vmBlur(texelFetch(u_depth, min(p, m), 0).r); }\n" +
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
                "uniform highp vec4 u_near;\nuniform vec2 u_tier;\n" +
                "uniform highp vec4 u_motion;\nuniform highp mat3 u_motionRotation;\nuniform highp vec3 u_motionMove;\n" +
                coc + nearWorld +
                "void main() {\n" +
                "  vec3 c = texture(u_scene, v_uv).rgb;\n" +
                "  vec3 col = c;\n" +
                "  bool smeared = false;\n" +
                /* Each world pixel goes back to where the last frame's camera saw
                 * it, and the scene is averaged along that path. The viewmodel
                 * moves with the camera: it stays sharp and stays out of the
                 * world's average. */
                "  if (u_motion.x > 0.0) {\n" +
                "    highp float depth = texelFetch(u_depth, ivec2(gl_FragCoord.xy), 0).r;\n" +
                "    highp vec2 ndc = v_uv * 2.0 - 1.0;\n" +
                "    highp float z = min(u_near.x / max(u_dof.x - depth, 0.000001), 1000000.0);\n" +
                "    highp vec3 p = u_motionRotation * vec3(ndc * z / u_motion.yz, z) + u_motionMove;\n" +
                "    highp vec2 v = (ndc - p.xy * u_motion.yz / max(p.z, 0.001)) * (0.5 * u_motion.x);\n" +
                "    float len = length(v / u_px.xy);\n" +
                "    if (depth >= 0.2 && p.z > 1.0 && len > 1.0) {\n" +
                "      float n = min(len, u_motion.w);\n" +
                "      highp vec2 path = v / u_px.xy * (n / len);\n" +
                "      int taps = int(clamp(ceil(n * 0.25), 3.0, 12.0));\n" +
                "      float jitter = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));\n" +
                "      ivec2 last = textureSize(u_scene, 0) - 1;\n" +
                "      vec3 sum = c;\n" +
                "      float weight = 1.0;\n" +
                /* Color and depth come from the same texel, so no filtered tap
                 * blends the viewmodel into the world. */
                "      for (int i = 0; i < 12; ++i) {\n" +
                "        if (i >= taps) break;\n" +
                "        ivec2 q = clamp(ivec2(gl_FragCoord.xy + path * ((float(i) + jitter) / float(taps) - 0.5)),\n" +
                "          ivec2(0), last);\n" +
                "        float w = step(0.2, texelFetch(u_depth, q, 0).r);\n" +
                "        sum += texelFetch(u_scene, q, 0).rgb * w;\n" +
                "        weight += w;\n" +
                "      }\n" +
                "      col = sum / weight;\n" +
                "      smeared = true;\n" +
                "    }\n" +
                "  }\n" +
                "  if (u_film.x > 0.0 && !smeared) {\n" +
                "    vec3 n = texture(u_scene, v_uv + vec2(0.0, u_px.y)).rgb;\n" +
                "    vec3 s = texture(u_scene, v_uv - vec2(0.0, u_px.y)).rgb;\n" +
                "    vec3 e = texture(u_scene, v_uv + vec2(u_px.x, 0.0)).rgb;\n" +
                "    vec3 w = texture(u_scene, v_uv - vec2(u_px.x, 0.0)).rgb;\n" +
                "    vec3 lo = min(c, min(min(n, s), min(e, w)));\n" +
                "    vec3 hi = max(c, max(max(n, s), max(e, w)));\n" +
                "    vec3 wt = -u_film.x * sqrt(clamp(min(lo, 1.0 - hi) / max(hi, vec3(0.0001)), 0.0, 1.0));\n" +
                "    col = clamp((c + (n + s + e + w) * wt) / (1.0 + 4.0 * wt), 0.0, 1.0);\n" +
                "  }\n" +
                "  if (u_dof2.x > 0.0 || u_dof2.w > 0.0) {\n" +
                "    highp float depth = texelFetch(u_depth, ivec2(gl_FragCoord.xy), 0).r;\n" +
                "    vec4 large = texture(u_dofLarge, v_uv);\n" +
                "    float k;\n" +
                "    if (depth < 0.2) {\n" +
                "      k = vmBlur(depth);\n" +
                "    } else {\n" +
                "      vec2 centered = (v_uv - 0.5) * vec2(u_px.z, 1.0);\n" +
                "      k = max(worldBlur(depth),\n" +
                "        clamp(2.0 * large.a, 0.0, 1.0) * smoothstep(u_dof2.y, u_dof2.z, length(centered)));\n" +
                "    }\n" +
                "    k = max(k * u_dof2.x, u_dof2.w);\n" +
                "    if (k > 0.003) {\n" +
                "      vec3 small = 0.25 * (texture(u_scene, v_uv + u_px.xy * vec2(0.5, 1.5)).rgb +\n" +
                "        texture(u_scene, v_uv + u_px.xy * vec2(-1.5, 0.5)).rgb +\n" +
                "        texture(u_scene, v_uv + u_px.xy * vec2(-0.5, -1.5)).rgb +\n" +
                "        texture(u_scene, v_uv + u_px.xy * vec2(1.5, -0.5)).rgb);\n" +
                "      vec3 med = texture(u_dofMed, v_uv).rgb;\n" +
                "      col = k < u_tier.x ? mix(col, small, k / u_tier.x) :\n" +
                "        (k < u_tier.y ? mix(small, med, (k - u_tier.x) / (u_tier.y - u_tier.x)) :\n" +
                "          mix(med, large.rgb, (k - u_tier.y) / (1.0 - u_tier.y)));\n" +
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
                ["u_px", "u_glow", "u_dof", "u_dof2", "u_film", "u_near", "u_tier",
                 "u_motion", "u_motionRotation", "u_motionMove"]);
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
            var glow0 = H[o], glow1 = H[o + 1], ads = H[o + 6], motion = H[o + 18], backdrop = H[o + 33];
            var glow = glow0 > 0 || glow1 > 0, blurBehind = backdrop > 0.004, dof = ads > 0.004 || blurBehind;
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
                    dof || motion > 0 ? (gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT) : gl.COLOR_BUFFER_BIT, gl.NEAREST);
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
            var vmStart = H[o + 7], vmEnd = H[o + 8], nearEnd = H[o + 16];
            var invRange = 1 / Math.max(vmEnd - vmStart, 0.5);
            /* The largest blur in quarter texels for a 480-line radius. Its
             * folded binomial deviates 1.414 steps, on top of the quarter image's
             * own 2 pixels; the four scene taps deviate 1.2 pixels. Each tier
             * starts where its image matches that fraction of the largest. */
            var near = Math.min(Math.max(H[o + 10] * lines * 0.6 / 4 / 1.4142, 0.25), 2.5);
            var largest = Math.sqrt(32 * near * near + 4);
            var smallTier = Math.min(1.2 / largest, 0.45);
            var medTier = Math.min(Math.max(2 / largest, smallTier + 0.05), 0.9);
            if (dof) {
                gl.useProgram(P.dof.program);
                gl.uniform2f(P.dof.u_texel, 1 / w, 1 / h);
                gl.uniform4f(P.dof.u_dof, 0.9995002746582031, H[o + 9], vmEnd, invRange);
                fx.texture(0, t.color);
                fx.texture(1, t.depth);
                fx.pass(P.dof, t.qFbo[0], qw, qh);
                /* Behind a menu, two wider passes take the quarter image back
                 * and forth to a deviation of 13 pixels at 480 lines; each
                 * pass's nearer taps stay within twice the blur already there. */
                var b = lines / 1.5;
                var steps = blurBehind ? [Math.max(near, 0.8 * b), 1.6 * b, 3.2 * b] : [near];
                gl.useProgram(P.dofBlur.program);
                for (var p = 0; p < steps.length; ++p) {
                    gl.uniform2f(P.dofBlur.u_step, steps[p] / qw, steps[p] / qh);
                    fx.texture(0, t.q[p & 1]);
                    fx.pass(P.dofBlur, t.qFbo[1 - (p & 1)], qw, qh);
                }
            }
            var C = P.compose;
            gl.useProgram(C.program);
            gl.uniform4f(C.u_px, 1 / w, 1 / h, w / h, 0);
            gl.uniform4f(C.u_glow, glow ? 1 : 0, 0, 0, 0);
            gl.uniform4f(C.u_dof, 0.9995002746582031, H[o + 9], vmEnd, invRange);
            gl.uniform4f(C.u_dof2, ads > 0.004 ? ads : 0, 0.07, 0.17, blurBehind ? backdrop : 0);
            gl.uniform4f(C.u_film, H[o + 11], H[o + 12], H[o + 13], H[o + 14]);
            /* CoD4 blurs the near world from one unit up to nearEnd. */
            gl.uniform4f(C.u_near, H[o + 15], nearEnd, 1 / Math.max(nearEnd - 1, 1), H[o + 17]);
            gl.uniform2f(C.u_tier, smallTier, medTier);
            /* The longest smear is 6% of the screen's height. */
            gl.uniform4f(C.u_motion, motion > 0 ? motion : 0, H[o + 31], H[o + 32], 0.06 * h);
            if (motion > 0) {
                gl.uniformMatrix3fv(C.u_motionRotation, false, H, o + 19, 9);
                gl.uniform3f(C.u_motionMove, H[o + 28], H[o + 29], H[o + 30]);
            }
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

void WebSceneFx_SetAds(float adsFrac, float viewmodelStart, float viewmodelEnd, float nearEnd)
{
    s_adsFrac = WebSceneFx_Clamp(adsFrac, 0.0f, 1.0f);
    s_vmEnd = WebSceneFx_Clamp(viewmodelEnd, 2.0f, 64.0f);
    s_vmStart = WebSceneFx_Clamp(viewmodelStart, 0.0f, s_vmEnd - 0.5f);
    s_nearEnd = WebSceneFx_Clamp(nearEnd, 1.0f, 1024.0f);
}

/* Aiming down the sights fades the motion blur out. */
void WebSceneFx_SetAim(float aimFrac)
{
    s_aimFrac = WebSceneFx_Clamp(aimFrac, 0.0f, 1.0f);
}

void WebSceneFx_SetBackdrop(float backdropFrac)
{
    s_backdropFrac = WebSceneFx_Clamp(backdropFrac, 0.0f, 1.0f);
}

/* The D3D view and projection matrices of the view about to begin. */
void WebSceneFx_SetCamera(const float *viewMatrix, const float *projectionMatrix)
{
    if (!viewMatrix || !projectionMatrix)
        return;
    memcpy(s_cameraView, viewMatrix, sizeof(s_cameraView));
    s_cameraProjection[0] = projectionMatrix[0];
    s_cameraProjection[1] = projectionMatrix[5];
    s_cameraPending = 1;
}

static int WebSceneFx_CameraValid(void)
{
    int i;

    for (i = 0; i < 16; ++i)
        if (!isfinite(s_cameraView[i]))
            return 0;
    return isfinite(s_cameraProjection[0]) && isfinite(s_cameraProjection[1]) &&
           s_cameraProjection[0] > 0.0f && s_cameraProjection[1] > 0.0f;
}

/* A view takes a world point p to R p + t, with R[j][i] = view[i * 4 + j] and
 * t[j] = view[12 + j]. From this view's space into the last one's, a point
 * turns by R_last R^T and then moves by t_last - R_last R^T t. */
static void WebSceneFx_TrackCamera(void)
{
    const float *view = s_cameraView, *last = s_lastView;
    float rotation[9], move[3];
    float turn, moved, trace;
    double now, gap;
    int valid, i, j, k;

    s_motionScale = 0.0f;
    if (!s_cameraPending) {
        s_lastViewValid = 0;
        return;
    }
    s_cameraPending = 0;
    valid = WebSceneFx_CameraValid();
    now = emscripten_get_now();
    gap = now - s_lastViewTime;
    if (valid && s_lastViewValid && gap >= 1.0 && gap <= SCENE_FX_MOTION_MAX_GAP &&
        WebSceneFx_Enabled(r_motionBlur) && s_aimFrac < 1.0f) {
        for (j = 0; j < 3; ++j)
            for (k = 0; k < 3; ++k) {
                float sum = 0.0f;
                for (i = 0; i < 3; ++i)
                    sum += last[i * 4 + j] * view[i * 4 + k];
                rotation[k * 3 + j] = sum;
            }
        for (j = 0; j < 3; ++j) {
            float sum = last[12 + j];
            for (k = 0; k < 3; ++k)
                sum -= rotation[k * 3 + j] * view[12 + k];
            move[j] = sum;
        }
        trace = rotation[0] + rotation[4] + rotation[8];
        turn = 0.5f * sqrtf((rotation[5] - rotation[7]) * (rotation[5] - rotation[7]) +
                            (rotation[6] - rotation[2]) * (rotation[6] - rotation[2]) +
                            (rotation[1] - rotation[3]) * (rotation[1] - rotation[3]));
        moved = sqrtf(move[0] * move[0] + move[1] * move[1] + move[2] * move[2]);
        if (trace > 1.0f + 2.0f * cosf(SCENE_FX_MOTION_MAX_TURN) && moved < SCENE_FX_MOTION_MAX_MOVE &&
            (turn > SCENE_FX_MOTION_MIN_TURN || moved > SCENE_FX_MOTION_MIN_MOVE)) {
            s_motionScale = (float)(SCENE_FX_MOTION_EXPOSURE / gap) * (1.0f - s_aimFrac);
            memcpy(s_motionRotation, rotation, sizeof(s_motionRotation));
            memcpy(s_motionMove, move, sizeof(s_motionMove));
            s_motionProjection[0] = s_cameraProjection[0];
            s_motionProjection[1] = s_cameraProjection[1];
        }
    }
    memcpy(s_lastView, view, sizeof(s_lastView));
    s_lastViewTime = now;
    s_lastViewValid = valid;
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
    params[SCENE_FX_NEAR_BLUR] = 3.0f;
    params[SCENE_FX_SHARPEN] = film ? 0.147f : 0.0f;
    params[SCENE_FX_SATURATION] = 0.96f;
    params[SCENE_FX_CONTRAST] = film ? 0.35f : 0.0f;
    params[SCENE_FX_FILM] = film ? 1.0f : 0.0f;
    params[SCENE_FX_WORLD_NEAR_CLIP] = s_worldNearClip;
    params[SCENE_FX_NEAR_END] = s_nearEnd;
    params[SCENE_FX_WORLD_BLUR] = 1.0f;
    params[SCENE_FX_MOTION] = s_motionScale;
    memcpy(params + SCENE_FX_MOTION_ROTATION, s_motionRotation, sizeof(s_motionRotation));
    memcpy(params + SCENE_FX_MOTION_MOVE, s_motionMove, sizeof(s_motionMove));
    params[SCENE_FX_MOTION_PROJECTION] = s_motionProjection[0];
    params[SCENE_FX_MOTION_PROJECTION + 1] = s_motionProjection[1];
    params[SCENE_FX_BACKDROP] = s_backdropFrac;
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
                          float depthHackNearClip, float worldNearClip)
{
    int samples;

    WebSceneFx_EndView();
    if (s_frameUsed || x || y || width != screenWidth || height != screenHeight || width <= 0 || height <= 0)
        return;
    s_frameUsed = 1;
    WebSceneFx_TrackCamera();
    /* r_fullbright clears the frame buffer before it begins the view. */
    if (WebSceneFx_Enabled(r_fullbright))
        return;
    samples = WebSceneFx_Samples();
    if (!WebSceneFx_GlowEnabled() && !WebSceneFx_Enabled(r_filmTweakEnable) && samples <= 1 &&
        !(WebSceneFx_Enabled(r_dof_enable) && s_adsFrac > 0.0f) && s_motionScale <= 0.0f && s_backdropFrac <= 0.0f)
        return;
    s_nearClip = isfinite(depthHackNearClip) && depthHackNearClip > 0.0f ? depthHackNearClip : 0.1f;
    s_worldNearClip = isfinite(worldNearClip) && worldNearClip > 0.0f ? worldNearClip : SCENE_FX_DEFAULT_WORLD_NEAR_CLIP;
    s_active = WebSceneFx_Bind(width, height, samples);
}

void WebSceneFx_EndFrame(void)
{
    WebSceneFx_EndView();
    s_frameUsed = 0;
}
