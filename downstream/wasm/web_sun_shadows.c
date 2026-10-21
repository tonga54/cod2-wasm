#include "common_types.h"
#include <emscripten.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* The sun's shadow of characters, bodies and every other moving model on the
 * world. The lightmap bakes the sun for the world and its static models only.
 * Each model surface of the frame's main view is taken in world space as the
 * renderer skins it (r_model.c); the frame's first full-screen view draws them
 * once from the sun into a depth map, and lightmapped world surfaces lose the
 * sun's share of their lightmap where that map hides them
 * (patch-emscripten-gl.py, COD2_SUN_SHADOW). Only surfaces the sun reaches in
 * the bake can darken, so a shadow never falls indoors or into baked shade. */

extern const dvar_t *r_sunShadows;

#define SUN_SHADOW_MAX_VERTICES 65536
#define SUN_SHADOW_MAX_TRIANGLES 98304
/* Texels across the depth map. */
#define SUN_SHADOW_SIZE 2048
/* The map spans this many units either side of its centre, across the sun's
 * rays: one unit per texel. The centre sits this far ahead of the eye. */
#define SUN_SHADOW_RADIUS 1024.0f
#define SUN_SHADOW_AHEAD 640.0f
/* Its depth range along the rays, either side of the centre. */
#define SUN_SHADOW_DEPTH 4096.0f
/* Models further than this from the last frame's eye cast nothing. */
#define SUN_SHADOW_CASTER_RANGE 2600.0f
/* The outer part of the map fades its shadow out: 1 / 8 of its width. */
#define SUN_SHADOW_FADE 8.0f
/* A receiver this many units behind a caster's depth is in its shadow. */
#define SUN_SHADOW_BIAS 1.5f
/* A sun lower than this sine of its elevation, or dimmer than this share of
 * the lightmap's half scale, casts no shadow worth drawing. */
#define SUN_SHADOW_MIN_ELEVATION 0.05f
#define SUN_SHADOW_MIN_COLOR 0.04f

enum {
    SUN_SHADOW_STRENGTH,
    SUN_SHADOW_TEXEL,
    SUN_SHADOW_EDGE,
    SUN_SHADOW_DEPTH_BIAS,
    /* The map rectangle that holds every caster, padded for the filter:
     * receivers outside it skip the lookups. */
    SUN_SHADOW_RECT,
    SUN_SHADOW_PARAM_COUNT = SUN_SHADOW_RECT + 4
};

static float s_positions[SUN_SHADOW_MAX_VERTICES * 3];
static unsigned int s_indices[SUN_SHADOW_MAX_TRIANGLES * 3];
static int s_vertexCount;
static int s_indexCount;
/* The captured vertices' bounds in the world. */
static float s_mins[3];
static float s_maxs[3];
static int s_capturing;
static int s_overflowWarned;
static int s_viewReady;
static float s_eye[3];
static float s_forward[3];
static float s_sun[3];
static float s_sunColor[3];
static float s_lastEye[3];
static int s_lastEyeValid;

/* Draws the casters, positions three floats apart and 32-bit indices, into
 * the size x size depth map with the clip matrix (column-major, world to GL
 * clip space), and keeps the map bound on texture unit 18 for the world's
 * shaders. Returns whether the map holds this frame's casters. */
EM_JS(int, WebSunShadows_Draw, (const float *positions, int vertexCount, const unsigned int *indices,
                                int indexCount, const float *clip, int size), {
    if (typeof GLctx === "undefined" || !GLctx || !GLctx.createVertexArray) return 0;
    var gl = GLctx;
    var s = Module.cod2SunShadowPass;
    if (!s || s.gl !== gl) {
        s = Module.cod2SunShadowPass = { gl: gl, failed: false, size: 0, texture: null, fbo: null, far: new Float32Array([1]) };
        var vs = gl.createShader(gl.VERTEX_SHADER);
        gl.shaderSource(vs, "#version 300 es\nlayout(location = 0) in vec3 a_position;\nuniform highp mat4 u_clip;\n" +
            "void main() { gl_Position = u_clip * vec4(a_position, 1.0); }\n");
        gl.compileShader(vs);
        var fs = gl.createShader(gl.FRAGMENT_SHADER);
        gl.shaderSource(fs, "#version 300 es\nprecision mediump float;\nvoid main() {}\n");
        gl.compileShader(fs);
        var program = gl.createProgram();
        gl.attachShader(program, vs);
        gl.attachShader(program, fs);
        gl.linkProgram(program);
        if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
            console.warn("[sun-shadows] caster program failed: " + gl.getProgramInfoLog(program) + " " +
                gl.getShaderInfoLog(vs) + " " + gl.getShaderInfoLog(fs));
            s.failed = true;
            return 0;
        }
        s.program = program;
        s.clip = gl.getUniformLocation(program, "u_clip");
        /* The pass owns its vertex array and both buffers: the GL emulation's
         * own array buffer binding is put back after every draw. */
        s.vao = gl.createVertexArray();
        s.positions = gl.createBuffer();
        s.indices = gl.createBuffer();
        var vao = gl.getParameter(gl.VERTEX_ARRAY_BINDING), arrayBuffer = gl.getParameter(gl.ARRAY_BUFFER_BINDING);
        gl.bindVertexArray(s.vao);
        gl.bindBuffer(gl.ARRAY_BUFFER, s.positions);
        gl.enableVertexAttribArray(0);
        gl.vertexAttribPointer(0, 3, gl.FLOAT, false, 12, 0);
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, s.indices);
        gl.bindVertexArray(vao);
        gl.bindBuffer(gl.ARRAY_BUFFER, arrayBuffer);
    }
    if (s.failed || size <= 0) return 0;
    var unit = gl.getParameter(gl.ACTIVE_TEXTURE);
    var drawFramebuffer = gl.getParameter(gl.DRAW_FRAMEBUFFER_BINDING);
    var readFramebuffer = gl.getParameter(gl.READ_FRAMEBUFFER_BINDING);
    if (s.size !== size) {
        if (s.texture) gl.deleteTexture(s.texture);
        if (s.fbo) gl.deleteFramebuffer(s.fbo);
        s.size = 0;
        gl.activeTexture(gl.TEXTURE18);
        s.texture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, s.texture);
        gl.texStorage2D(gl.TEXTURE_2D, 1, gl.DEPTH_COMPONENT16, size, size);
        /* Linear filtering with a comparison: four depth tests per lookup. */
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_COMPARE_MODE, gl.COMPARE_REF_TO_TEXTURE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_COMPARE_FUNC, gl.LEQUAL);
        s.fbo = gl.createFramebuffer();
        gl.bindFramebuffer(gl.FRAMEBUFFER, s.fbo);
        gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.DEPTH_ATTACHMENT, gl.TEXTURE_2D, s.texture, 0);
        gl.drawBuffers([gl.NONE]);
        gl.readBuffer(gl.NONE);
        var complete = gl.checkFramebufferStatus(gl.FRAMEBUFFER) === gl.FRAMEBUFFER_COMPLETE;
        gl.bindFramebuffer(gl.DRAW_FRAMEBUFFER, drawFramebuffer);
        gl.bindFramebuffer(gl.READ_FRAMEBUFFER, readFramebuffer);
        gl.activeTexture(unit);
        if (!complete) {
            console.warn("[sun-shadows] depth map unavailable; models cast no sun shadow");
            s.failed = true;
            return 0;
        }
        s.size = size;
    }
    var program = gl.getParameter(gl.CURRENT_PROGRAM);
    var vao = gl.getParameter(gl.VERTEX_ARRAY_BINDING);
    var arrayBuffer = gl.getParameter(gl.ARRAY_BUFFER_BINDING);
    var viewport = gl.getParameter(gl.VIEWPORT);
    var caps = [gl.BLEND, gl.CULL_FACE, gl.SCISSOR_TEST, gl.STENCIL_TEST, gl.POLYGON_OFFSET_FILL, gl.DEPTH_TEST];
    var enabled = [];
    for (var c = 0; c < caps.length; ++c) {
        enabled.push(gl.isEnabled(caps[c]));
        gl.disable(caps[c]);
    }
    gl.bindFramebuffer(gl.FRAMEBUFFER, s.fbo);
    WebGL2RenderingContext.prototype.viewport.call(gl, 0, 0, size, size);
    /* Every D3D9 clear and draw sets the write masks and depth function again. */
    gl.depthMask(true);
    gl.clearBufferfv(gl.DEPTH, 0, s.far);
    var drawn = 0;
    if (indexCount > 0 && vertexCount > 0) {
        gl.enable(gl.DEPTH_TEST);
        gl.depthFunc(gl.LESS);
        gl.colorMask(false, false, false, false);
        gl.useProgram(s.program);
        gl.uniformMatrix4fv(s.clip, false, HEAPF32.subarray(clip >>> 2, (clip >>> 2) + 16));
        gl.bindVertexArray(s.vao);
        gl.bindBuffer(gl.ARRAY_BUFFER, s.positions);
        gl.bufferData(gl.ARRAY_BUFFER, HEAPF32.subarray(positions >>> 2, (positions >>> 2) + vertexCount * 3), gl.STREAM_DRAW);
        gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, HEAPU32.subarray(indices >>> 2, (indices >>> 2) + indexCount), gl.STREAM_DRAW);
        gl.drawElements(gl.TRIANGLES, indexCount, gl.UNSIGNED_INT, 0);
        gl.colorMask(true, true, true, true);
        drawn = 1;
    }
    gl.bindVertexArray(vao);
    gl.bindBuffer(gl.ARRAY_BUFFER, arrayBuffer);
    gl.useProgram(program);
    gl.bindFramebuffer(gl.DRAW_FRAMEBUFFER, drawFramebuffer);
    gl.bindFramebuffer(gl.READ_FRAMEBUFFER, readFramebuffer);
    WebGL2RenderingContext.prototype.viewport.call(gl, viewport[0], viewport[1], viewport[2], viewport[3]);
    for (var r = 0; r < caps.length; ++r)
        if (enabled[r]) gl.enable(caps[r]);
        else gl.disable(caps[r]);
    return drawn;
});

/* What the world's shaders take: the world-to-map matrix (column-major, map
 * texture coordinates and depth in [0, 1]), the parameters and the casters'
 * rectangle; a strength of zero turns the shadow off. web_point_lights.c
 * moves the matrix into each draw's eye space. */
EM_JS(void, WebSunShadows_SetReceivers, (const float *texture, const float *params), {
    var s = Module.cod2SunShadow;
    if (!s) s = Module.cod2SunShadow = { world: new Float32Array(16), params: new Float32Array(4),
        rect: new Float32Array(4), revision: 0 };
    s.world.set(HEAPF32.subarray(texture >>> 2, (texture >>> 2) + 16));
    s.params.set(HEAPF32.subarray(params >>> 2, (params >>> 2) + 4));
    s.rect.set(HEAPF32.subarray((params >>> 2) + 4, (params >>> 2) + 8));
    ++s.revision;
});

static int WebSunShadows_Enabled(void)
{
    return r_sunShadows && r_sunShadows->current.enabled;
}

static int WebSunShadows_Finite3(const float *v)
{
    return v && isfinite(v[0]) && isfinite(v[1]) && isfinite(v[2]);
}

/* R_BeginFrame: the front end skins this frame's models from here on. */
void WebSunShadows_BeginFrame(void)
{
    s_vertexCount = 0;
    s_indexCount = 0;
    s_capturing = WebSunShadows_Enabled();
    s_viewReady = 0;
}

/* A model surface the renderer has just skinned: vertexCount positions stride
 * bytes apart, taken into the world by transform (three axis rows, then the
 * origin), drawn as triangleCount triangles of indices. Only the cgame's
 * models cast: players, bodies, script models, grenades and dropped weapons,
 * never the viewmodel, effects or the static models the lightmap holds. */
void WebSunShadows_AddSurface(const GfxEntity *ent, const float *positions, int stride, int vertexCount,
                              const unsigned short *indices, int triangleCount, const float *transform)
{
    float *world;
    unsigned int base;
    int i, j;

    if (!s_capturing || !ent || !positions || !indices || !transform || vertexCount <= 0 || triangleCount <= 0 ||
        stride < 3 * (int)sizeof(float) || stride % (int)sizeof(float))
        return;
    if (ent->reType != 0 || (ent->renderFxFlags & 8) || !WebSunShadows_Finite3(ent->origin))
        return;
    if (s_lastEyeValid) {
        float dx = ent->origin[0] - s_lastEye[0], dy = ent->origin[1] - s_lastEye[1], dz = ent->origin[2] - s_lastEye[2];
        if (dx * dx + dy * dy + dz * dz > SUN_SHADOW_CASTER_RANGE * SUN_SHADOW_CASTER_RANGE)
            return;
    }
    if (vertexCount > SUN_SHADOW_MAX_VERTICES - s_vertexCount ||
        triangleCount > (SUN_SHADOW_MAX_TRIANGLES * 3 - s_indexCount) / 3) {
        if (!s_overflowWarned)
            printf("[sun-shadows] over %d vertices or %d triangles of models in view; the rest cast no shadow\n",
                   SUN_SHADOW_MAX_VERTICES, SUN_SHADOW_MAX_TRIANGLES);
        s_overflowWarned = 1;
        return;
    }
    base = (unsigned int)s_vertexCount;
    world = s_positions + s_vertexCount * 3;
    if (!s_vertexCount) {
        s_mins[0] = s_mins[1] = s_mins[2] = INFINITY;
        s_maxs[0] = s_maxs[1] = s_maxs[2] = -INFINITY;
    }
    for (i = 0; i < vertexCount; ++i) {
        const float *p = (const float *)((const char *)positions + (size_t)i * (size_t)stride);
        for (j = 0; j < 3; ++j) {
            float v = p[0] * transform[j] + p[1] * transform[3 + j] + p[2] * transform[6 + j] + transform[9 + j];
            world[i * 3 + j] = v;
            s_mins[j] = fminf(s_mins[j], v);
            s_maxs[j] = fmaxf(s_maxs[j], v);
        }
    }
    for (i = 0; i < triangleCount; ++i) {
        const unsigned short *corner = indices + i * 3;
        if (corner[0] >= vertexCount || corner[1] >= vertexCount || corner[2] >= vertexCount)
            continue;
        for (j = 0; j < 3; ++j)
            s_indices[s_indexCount++] = base + corner[j];
    }
    s_vertexCount += vertexCount;
}

/* After the main scene's models are skinned (R_RenderScene): its eye and
 * view direction, and the world's sun direction (towards the sun) and its
 * color at the lightmap's half scale. Later scenes this frame cast nothing. */
void WebSunShadows_EndCapture(const float *eye, const float *forward, const float *sun, const float *sunColor)
{
    if (!s_capturing)
        return;
    s_capturing = 0;
    if (!WebSunShadows_Finite3(eye) || !WebSunShadows_Finite3(forward) || !WebSunShadows_Finite3(sun) ||
        !WebSunShadows_Finite3(sunColor))
        return;
    memcpy(s_eye, eye, sizeof(s_eye));
    memcpy(s_forward, forward, sizeof(s_forward));
    memcpy(s_sun, sun, sizeof(s_sun));
    memcpy(s_sunColor, sunColor, sizeof(s_sunColor));
    memcpy(s_lastEye, eye, sizeof(s_lastEye));
    s_lastEyeValid = 1;
    s_viewReady = 1;
}

/* The map's matrices: texture takes a world point to its map coordinates and
 * depth in [0, 1], clip to GL clip space for drawing the casters. Its square
 * is centred ahead of the eye and snapped to whole texels across the rays,
 * so shadows hold still as the eye moves. Returns 0 without a usable sun. */
int WebSunShadows_Matrices(const float *eye, const float *forward, const float *sun, const float *sunColor,
                           float *texture, float *clip)
{
    float f[3], r[3], u[3], ref[3] = {0.0f, 0.0f, 1.0f}, ahead[3] = {0.0f, 0.0f, 0.0f}, centre[3];
    float length, texel, a, b, depth, inverseExtent, inverseDepth;
    int i;

    length = sqrtf(sun[0] * sun[0] + sun[1] * sun[1] + sun[2] * sun[2]);
    if (!(length > 0.0f) || !isfinite(length))
        return 0;
    if (sun[2] / length < SUN_SHADOW_MIN_ELEVATION)
        return 0;
    if (!(fmaxf(sunColor[0], fmaxf(sunColor[1], sunColor[2])) > SUN_SHADOW_MIN_COLOR))
        return 0;
    for (i = 0; i < 3; ++i)
        f[i] = -sun[i] / length;
    if (fabsf(f[2]) > 0.99f) {
        ref[0] = 1.0f;
        ref[2] = 0.0f;
    }
    r[0] = f[1] * ref[2] - f[2] * ref[1];
    r[1] = f[2] * ref[0] - f[0] * ref[2];
    r[2] = f[0] * ref[1] - f[1] * ref[0];
    length = sqrtf(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]);
    for (i = 0; i < 3; ++i)
        r[i] /= length;
    u[0] = r[1] * f[2] - r[2] * f[1];
    u[1] = r[2] * f[0] - r[0] * f[2];
    u[2] = r[0] * f[1] - r[1] * f[0];
    length = sqrtf(forward[0] * forward[0] + forward[1] * forward[1]);
    if (length > 0.001f) {
        ahead[0] = forward[0] / length;
        ahead[1] = forward[1] / length;
    }
    for (i = 0; i < 3; ++i)
        centre[i] = eye[i] + ahead[i] * SUN_SHADOW_AHEAD;
    texel = 2.0f * SUN_SHADOW_RADIUS / (float)SUN_SHADOW_SIZE;
    a = floorf((centre[0] * r[0] + centre[1] * r[1] + centre[2] * r[2]) / texel + 0.5f) * texel;
    b = floorf((centre[0] * u[0] + centre[1] * u[1] + centre[2] * u[2]) / texel + 0.5f) * texel;
    depth = centre[0] * f[0] + centre[1] * f[1] + centre[2] * f[2];
    inverseExtent = 0.5f / SUN_SHADOW_RADIUS;
    inverseDepth = 0.5f / SUN_SHADOW_DEPTH;
    for (i = 0; i < 3; ++i) {
        texture[i * 4] = r[i] * inverseExtent;
        texture[i * 4 + 1] = u[i] * inverseExtent;
        texture[i * 4 + 2] = f[i] * inverseDepth;
        texture[i * 4 + 3] = 0.0f;
    }
    texture[12] = 0.5f - a * inverseExtent;
    texture[13] = 0.5f - b * inverseExtent;
    texture[14] = 0.5f - depth * inverseDepth;
    texture[15] = 1.0f;
    for (i = 0; i < 16; ++i)
        clip[i] = i % 4 == 3 ? texture[i] : 2.0f * texture[i] - (i >= 12 ? 1.0f : 0.0f);
    return 1;
}

/* The map rectangle holding a world box, from the texture matrix's first
 * two rows, padded by pad: [u0, v0, u1, v1] within [0, 1]. */
void WebSunShadows_Rect(const float *texture, const float *mins, const float *maxs, float pad, float *rect)
{
    int axis, corner;

    rect[0] = rect[1] = INFINITY;
    rect[2] = rect[3] = -INFINITY;
    for (corner = 0; corner < 8; ++corner) {
        float p[3] = {corner & 1 ? maxs[0] : mins[0], corner & 2 ? maxs[1] : mins[1], corner & 4 ? maxs[2] : mins[2]};
        for (axis = 0; axis < 2; ++axis) {
            float t = p[0] * texture[axis] + p[1] * texture[4 + axis] + p[2] * texture[8 + axis] + texture[12 + axis];
            rect[axis] = fminf(rect[axis], t);
            rect[2 + axis] = fmaxf(rect[2 + axis], t);
        }
    }
    for (axis = 0; axis < 2; ++axis) {
        rect[axis] = fmaxf(rect[axis] - pad, 0.0f);
        rect[2 + axis] = fminf(rect[2 + axis] + pad, 1.0f);
    }
}

/* The frame's first full-screen view, before it draws the world. */
void WebSunShadows_Render(void)
{
    float texture[16], clip[16], params[SUN_SHADOW_PARAM_COUNT];
    int ready = s_viewReady;

    s_viewReady = 0;
    memset(texture, 0, sizeof(texture));
    memset(params, 0, sizeof(params));
    if (ready && WebSunShadows_Enabled() && s_indexCount > 0 &&
        WebSunShadows_Matrices(s_eye, s_forward, s_sun, s_sunColor, texture, clip) &&
        WebSunShadows_Draw(s_positions, s_vertexCount, s_indices, s_indexCount, clip, SUN_SHADOW_SIZE)) {
        params[SUN_SHADOW_STRENGTH] = 1.0f;
        params[SUN_SHADOW_TEXEL] = 1.0f / (float)SUN_SHADOW_SIZE;
        params[SUN_SHADOW_EDGE] = SUN_SHADOW_FADE;
        params[SUN_SHADOW_DEPTH_BIAS] = SUN_SHADOW_BIAS * 0.5f / SUN_SHADOW_DEPTH;
        WebSunShadows_Rect(texture, s_mins, s_maxs, 4.0f / (float)SUN_SHADOW_SIZE, params + SUN_SHADOW_RECT);
    }
    WebSunShadows_SetReceivers(texture, params);
}
