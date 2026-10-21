#!/usr/bin/env python3
"""Exercise the sun's shadow map of moving models: which surfaces cast, their
world-space capture, the map's matrices and rectangle, and what the world's
shaders receive each frame (downstream/wasm/web_sun_shadows.c)."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = root / 'downstream/wasm/web_sun_shadows.c'
module = source.read_text()

# The front end starts each frame's capture, the main view's scene ends it
# after its models are skinned, and the frame's first full-screen view draws
# the map before the world.
frames = (root / 'src/PC/gfx_d3d/r_rendercmds.c').read_text()
begin = frames[frames.index('void R_BeginFrame(void)\n{'):]
assert begin.index('WebSunShadows_BeginFrame();') < begin.index('R_BeginFrame_impl();')
scene = (root / 'src/PC/gfx_d3d/r_scene.c').read_text()
render = scene[scene.index('void R_RenderScene(const refdef_t *refdef)\n{'):]
assert render.index('R_AddWorldSurfacesDpvs(viewParmsDraw, cellIdx);') < render.index('FX_DrawScheduledEffects();') < \
    render.index('WebSunShadows_EndCapture(refdef->vieworg, refdef->viewaxis[0], s_world.sunLight.position, halfSun);') < \
    render.index('qsortDrawSurfs(')
assert ('if (!refdef->x && !refdef->y && refdef->width == ((const vidConfig_t *)imp_vidConfig)->width &&\n'
        '            refdef->height == ((const vidConfig_t *)imp_vidConfig)->height) {') in render
assert ('const float halfSun[3] = {s_world.sunLight.color[0] * 0.5f, s_world.sunLight.color[1] * 0.5f,\n'
        '                                      s_world.sunLight.color[2] * 0.5f};') in render
compositor = (root / 'downstream/wasm/web_scene_fx.c').read_text()
view = compositor[compositor.index('void WebSceneFx_BeginView('):]
assert view.index('    s_frameUsed = 1;') < view.index('WebSunShadows_Render();') < view.index('WebSceneFx_Bind(')
assert '"${CMAKE_CURRENT_LIST_DIR}/web_sun_shadows.c"' in (root / 'downstream/wasm/CMakeLists.txt').read_text()
dvars = (root / 'src/PC/gfx_d3d/r_dvars.c').read_text()
at = dvars.index('r_sunShadows = ri.Dvar_RegisterBool("r_sunShadows", 1, 0x2001);')
assert dvars.rindex('#ifdef __EMSCRIPTEN__', 0, at) > dvars.rindex('#endif', 0, at)

emscripten = '#define EM_JS(ret, name, params, ...) ret name params;\n'
common_types = r'''
typedef float vec3_t[3];
typedef union {int enabled; int integer; float value;} DvarValue;
typedef struct {DvarValue current;} dvar_t;
typedef struct {int reType; int renderFxFlags; float lighting[3]; vec3_t axis[3]; float scale; vec3_t origin;} GfxEntity;
'''
test = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "SOURCE"
static dvar_t shadows = {{1}};
const dvar_t *r_sunShadows = &shadows;
static int draws, drawResult = 1, drawVertices, drawIndices, drawSize, receivers;
static float drawClip[16], received[16], receivedParams[SUN_SHADOW_PARAM_COUNT];
static const float *drawPositions;
static const unsigned int *drawIndexArray;
int WebSunShadows_Draw(const float *positions, int vertexCount, const unsigned int *indices, int indexCount,
                       const float *clip, int size)
{
    ++draws;
    drawPositions = positions; drawVertices = vertexCount; drawIndexArray = indices; drawIndices = indexCount;
    memcpy(drawClip, clip, sizeof(drawClip));
    drawSize = size;
    return drawResult;
}
void WebSunShadows_SetReceivers(const float *texture, const float *params)
{
    ++receivers;
    memcpy(received, texture, sizeof(received));
    memcpy(receivedParams, params, sizeof(receivedParams));
}
static const float sun[3] = {-0.5f, 0.5f, 0.70710677f}, sunColor[3] = {0.38f, 0.35f, 0.29f};
static const float eye[3] = {468, 992, 138}, forward[3] = {-1, 0, 0};
static void map(const float *m, const float *p, float *out)
{
    for (int r = 0; r < 3; ++r)
        out[r] = m[r] * p[0] + m[4 + r] * p[1] + m[8 + r] * p[2] + m[12 + r];
}
/* A quad of two triangles, model x forward, placed by its entity. */
static const float quad[4][4] = {{0, -10, 0, 99}, {0, 10, 0, 99}, {0, 10, 70, 99}, {0, -10, 70, 99}};
static const unsigned short twoTriangles[6] = {0, 1, 2, 0, 2, 3};
static GfxEntity player(float x, float y, float z)
{
    GfxEntity ent;
    memset(&ent, 0, sizeof(ent));
    ent.renderFxFlags = 0x80;
    ent.scale = 1;
    for (int i = 0; i < 3; ++i)
        ent.axis[i][i] = 1;
    ent.origin[0] = x; ent.origin[1] = y; ent.origin[2] = z;
    return ent;
}
static void placement(const GfxEntity *ent, float *transform)
{
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            transform[i * 3 + j] = ent->axis[i][j] * ent->scale;
    for (int j = 0; j < 3; ++j)
        transform[9 + j] = ent->origin[j];
}
static void add(const GfxEntity *ent)
{
    float transform[12];
    placement(ent, transform);
    /* Four floats apart, like a skinned vertex's position and more. */
    WebSunShadows_AddSurface(ent, &quad[0][0], 16, 4, twoTriangles, 2, transform);
}
static void frame(void)
{
    WebSunShadows_BeginFrame();
}
int main(void)
{
    float texture[16], clip[16], p[3], q[3];

    /* The map's matrices: a world point along the sun's rays keeps its map
     * coordinates and gains depth away from the sun; the square is centred
     * 640 units ahead of the eye, within a texel, and spans 2048 units. */
    assert(WebSunShadows_Matrices(eye, forward, sun, sunColor, texture, clip));
    {
        const float centre[3] = {eye[0] - 640, eye[1], eye[2]};
        map(texture, centre, p);
        assert(fabsf(p[0] - 0.5f) <= 0.5f / 2048 + 1e-6f && fabsf(p[1] - 0.5f) <= 0.5f / 2048 + 1e-6f);
        assert(fabsf(p[2] - 0.5f) < 1e-6f);
        for (float t = -400; t <= 400; t += 100) {
            const float along[3] = {centre[0] + sun[0] * t, centre[1] + sun[1] * t, centre[2] + sun[2] * t};
            map(texture, along, q);
            assert(fabsf(q[0] - p[0]) < 1e-5f && fabsf(q[1] - p[1]) < 1e-5f);
            assert(fabsf(q[2] - (0.5f - t / 8192.0f)) < 1e-5f);
        }
        /* 1024 units across the rays reach the map's edge. */
        float across[3] = {sun[1], -sun[0], 0}, length = sqrtf(across[0] * across[0] + across[1] * across[1]);
        const float edge[3] = {centre[0] + across[0] / length * 1024, centre[1] + across[1] / length * 1024, centre[2]};
        map(texture, edge, q);
        assert(fabsf(hypotf(q[0] - p[0], q[1] - p[1]) - 0.5f) < 1e-3f);
    }
    /* Clip space is the map's coordinates and depth from -1 to 1. */
    for (int i = 0; i < 16; ++i) {
        float expected = i % 4 == 3 ? texture[i] : 2 * texture[i] - (i >= 12 ? 1 : 0);
        assert(fabsf(clip[i] - expected) < 1e-6f);
    }
    assert(texture[3] == 0 && texture[7] == 0 && texture[11] == 0 && texture[15] == 1);
    /* Its square snaps to whole texels across the rays: an eye moving a
     * fraction of a texel leaves the matrix alone. */
    {
        float moved[16], movedClip[16];
        const float nudged[3] = {eye[0] + 0.1f, eye[1] - 0.1f, eye[2]};
        assert(WebSunShadows_Matrices(nudged, forward, sun, sunColor, moved, movedClip));
        assert(moved[12] == texture[12] && moved[13] == texture[13]);
        const float far[3] = {eye[0] + 300, eye[1] - 300, eye[2]};
        assert(WebSunShadows_Matrices(far, forward, sun, sunColor, moved, movedClip));
        float texel = 1.0f / 2048;
        float du = (moved[12] - texture[12]) / texel, dv = (moved[13] - texture[13]) / texel;
        assert(fabsf(du - roundf(du)) < 1e-2f && fabsf(dv - roundf(dv)) < 1e-2f && (fabsf(du) > 1 || fabsf(dv) > 1));
    }
    /* Looking straight down, the square centres on the eye; a vertical sun
     * still makes a finite basis. */
    {
        const float down[3] = {0, 0, -1}, overhead[3] = {0, 0, 1};
        assert(WebSunShadows_Matrices(eye, down, overhead, sunColor, texture, clip));
        map(texture, eye, p);
        assert(fabsf(p[0] - 0.5f) <= 0.5f / 2048 + 1e-6f && fabsf(p[1] - 0.5f) <= 0.5f / 2048 + 1e-6f);
        for (int i = 0; i < 16; ++i)
            assert(isfinite(texture[i]) && isfinite(clip[i]));
    }
    /* No usable sun: below the horizon, dim, or without a direction. */
    {
        const float low[3] = {1, 0, 0.04f}, below[3] = {0, 0.5f, -0.8f}, none[3] = {0, 0, 0}, dim[3] = {0.03f, 0.04f, 0.02f};
        const float bad[3] = {NAN, 0, 1};
        assert(!WebSunShadows_Matrices(eye, forward, low, sunColor, texture, clip));
        assert(!WebSunShadows_Matrices(eye, forward, below, sunColor, texture, clip));
        assert(!WebSunShadows_Matrices(eye, forward, none, sunColor, texture, clip));
        assert(!WebSunShadows_Matrices(eye, forward, bad, sunColor, texture, clip));
        assert(!WebSunShadows_Matrices(eye, forward, sun, dim, texture, clip));
    }

    /* The casters' rectangle: a world box's eight corners, padded and kept
     * within the map. */
    {
        float rect[4];
        const float mins[3] = {400, 900, 80}, maxs[3] = {420, 940, 150};
        assert(WebSunShadows_Matrices(eye, forward, sun, sunColor, texture, clip));
        WebSunShadows_Rect(texture, mins, maxs, 0.01f, rect);
        for (int corner = 0; corner < 8; ++corner) {
            const float c[3] = {corner & 1 ? maxs[0] : mins[0], corner & 2 ? maxs[1] : mins[1], corner & 4 ? maxs[2] : mins[2]};
            map(texture, c, p);
            assert(p[0] >= rect[0] + 0.0099f && p[0] <= rect[2] - 0.0099f);
            assert(p[1] >= rect[1] + 0.0099f && p[1] <= rect[3] - 0.0099f);
        }
        const float hugeMins[3] = {-1e5f, -1e5f, -1e5f}, hugeMaxs[3] = {1e5f, 1e5f, 1e5f};
        WebSunShadows_Rect(texture, hugeMins, hugeMaxs, 0.01f, rect);
        assert(rect[0] == 0 && rect[1] == 0 && rect[2] == 1 && rect[3] == 1);
    }

    /* A frame's casters: the cgame's models in world space, their triangles
     * indexed after every earlier surface's vertices. */
    frame();
    GfxEntity a = player(400, 900, 80), b = player(420, 960, 80);
    a.axis[0][0] = 0; a.axis[0][1] = 1; a.axis[1][0] = -1; a.axis[1][1] = 0; a.scale = 2;
    add(&a);
    add(&b);
    WebSunShadows_EndCapture(eye, forward, sun, sunColor);
    draws = receivers = 0;
    WebSunShadows_Render();
    assert(draws == 1 && receivers == 1 && drawVertices == 8 && drawIndices == 12 && drawSize == 2048);
    for (int v = 0; v < 4; ++v) {
        const float *got = drawPositions + v * 3;
        assert(fabsf(got[0] - (400 - 2 * quad[v][1])) < 1e-4f && fabsf(got[1] - (900 + 0)) < 1e-4f);
        assert(fabsf(got[2] - (80 + 2 * quad[v][2])) < 1e-4f);
        got = drawPositions + (4 + v) * 3;
        assert(got[0] == 420 && got[1] == 960 + quad[v][1] && got[2] == 80 + quad[v][2]);
    }
    for (int i = 0; i < 6; ++i)
        assert(drawIndexArray[i] == twoTriangles[i] && drawIndexArray[6 + i] == 4u + twoTriangles[i]);
    assert(WebSunShadows_Matrices(eye, forward, sun, sunColor, texture, clip));
    assert(!memcmp(received, texture, sizeof(texture)) && !memcmp(drawClip, clip, sizeof(clip)));
    assert(receivedParams[SUN_SHADOW_STRENGTH] == 1 && receivedParams[SUN_SHADOW_TEXEL] == 1.0f / 2048);
    assert(receivedParams[SUN_SHADOW_EDGE] == 8 && fabsf(receivedParams[SUN_SHADOW_DEPTH_BIAS] - 1.5f / 8192) < 1e-9f);
    {
        /* The rectangle holds both models' every vertex. */
        const float *r = receivedParams + SUN_SHADOW_RECT;
        for (int v = 0; v < 8; ++v) {
            map(texture, drawPositions + v * 3, p);
            assert(p[0] >= r[0] && p[0] <= r[2] && p[1] >= r[1] && p[1] <= r[3]);
        }
        assert(r[2] - r[0] < 0.1f && r[3] - r[1] < 0.1f);
    }
    /* The view is spent: a second view this frame draws no map and turns the
     * world's shadow off. */
    WebSunShadows_Render();
    assert(draws == 1 && receivers == 2 && receivedParams[SUN_SHADOW_STRENGTH] == 0);

    /* Only the cgame's models cast: never the viewmodel, effects or static
     * models, nor a surface with broken arguments. */
    frame();
    GfxEntity gun = player(470, 990, 130), fx = player(470, 990, 130), smodel = player(470, 990, 130), lost = player(NAN, 0, 0);
    gun.renderFxFlags = 0x8c; fx.reType = 4; smodel.reType = 2;
    add(&gun); add(&fx); add(&smodel); add(&lost);
    {
        float transform[12];
        placement(&b, transform);
        WebSunShadows_AddSurface(&b, &quad[0][0], 8, 4, twoTriangles, 2, transform);
        WebSunShadows_AddSurface(&b, &quad[0][0], 14, 4, twoTriangles, 2, transform);
        WebSunShadows_AddSurface(&b, &quad[0][0], 16, 0, twoTriangles, 2, transform);
        WebSunShadows_AddSurface(&b, &quad[0][0], 16, 4, twoTriangles, 0, transform);
        WebSunShadows_AddSurface(&b, NULL, 16, 4, twoTriangles, 2, transform);
        WebSunShadows_AddSurface(&b, &quad[0][0], 16, 4, NULL, 2, transform);
        WebSunShadows_AddSurface(&b, &quad[0][0], 16, 4, twoTriangles, 2, NULL);
        WebSunShadows_AddSurface(NULL, &quad[0][0], 16, 4, twoTriangles, 2, transform);
        /* A triangle naming a vertex past the surface's is dropped. */
        const unsigned short broken[6] = {0, 1, 4, 3, 2, 1};
        WebSunShadows_AddSurface(&b, &quad[0][0], 16, 4, broken, 2, transform);
    }
    WebSunShadows_EndCapture(eye, forward, sun, sunColor);
    draws = 0;
    WebSunShadows_Render();
    assert(draws == 1 && drawVertices == 4 && drawIndices == 3);
    assert(drawIndexArray[0] == 3 && drawIndexArray[1] == 2 && drawIndexArray[2] == 1);

    /* Models far from the last frame's eye cast nothing. */
    frame();
    GfxEntity distant = player(eye[0] + 2700, eye[1], eye[2]), nearby = player(eye[0] + 2500, eye[1], eye[2]);
    add(&distant);
    add(&nearby);
    WebSunShadows_EndCapture(eye, forward, sun, sunColor);
    draws = 0;
    WebSunShadows_Render();
    assert(draws == 1 && drawVertices == 4);

    /* Without casters, or a sun, no map is drawn and the world's shadow is off. */
    frame();
    WebSunShadows_EndCapture(eye, forward, sun, sunColor);
    draws = receivers = 0;
    WebSunShadows_Render();
    assert(!draws && receivers == 1 && receivedParams[SUN_SHADOW_STRENGTH] == 0);
    frame();
    add(&b);
    {
        const float dark[3] = {0, 0, 0};
        WebSunShadows_EndCapture(eye, forward, sun, dark);
    }
    WebSunShadows_Render();
    assert(!draws && receivedParams[SUN_SHADOW_STRENGTH] == 0);
    /* A broken view ends the capture without drawing. */
    frame();
    add(&b);
    {
        const float broken[3] = {NAN, 0, 0};
        WebSunShadows_EndCapture(broken, forward, sun, sunColor);
    }
    add(&b);
    WebSunShadows_Render();
    assert(!draws && receivedParams[SUN_SHADOW_STRENGTH] == 0);
    /* Later scenes this frame add nothing to the map. */
    frame();
    add(&b);
    WebSunShadows_EndCapture(eye, forward, sun, sunColor);
    add(&a);
    WebSunShadows_Render();
    assert(draws == 1 && drawVertices == 4 && drawPositions[0] == 420);
    /* A map that could not be drawn gives the world no shadow. */
    frame();
    add(&b);
    WebSunShadows_EndCapture(eye, forward, sun, sunColor);
    drawResult = 0;
    WebSunShadows_Render();
    assert(draws == 2 && receivedParams[SUN_SHADOW_STRENGTH] == 0);
    drawResult = 1;
    /* r_sunShadows 0 captures and draws nothing, even if it turns back on
     * before the frame's view. */
    shadows.current.enabled = 0;
    frame();
    add(&b);
    WebSunShadows_EndCapture(eye, forward, sun, sunColor);
    WebSunShadows_Render();
    assert(draws == 2 && receivedParams[SUN_SHADOW_STRENGTH] == 0);
    frame();
    add(&b);
    WebSunShadows_EndCapture(eye, forward, sun, sunColor);
    shadows.current.enabled = 1;
    WebSunShadows_Render();
    assert(draws == 2 && receivedParams[SUN_SHADOW_STRENGTH] == 0);

    /* More models than the map holds: the ones that fit cast, once warned. */
    frame();
    {
        static float many[SUN_SHADOW_MAX_VERTICES / 2][3];
        static unsigned short triangles[SUN_SHADOW_MAX_VERTICES / 2 * 3];
        float transform[12];
        for (int i = 0; i < SUN_SHADOW_MAX_VERTICES / 2; ++i) {
            many[i][0] = (float)(i % 64); many[i][1] = (float)(i / 64 % 64); many[i][2] = 0;
            triangles[i * 3] = (unsigned short)i;
            triangles[i * 3 + 1] = (unsigned short)((i + 1) % (SUN_SHADOW_MAX_VERTICES / 2));
            triangles[i * 3 + 2] = (unsigned short)((i + 2) % (SUN_SHADOW_MAX_VERTICES / 2));
        }
        placement(&b, transform);
        for (int i = 0; i < 3; ++i)
            WebSunShadows_AddSurface(&b, &many[0][0], 12, SUN_SHADOW_MAX_VERTICES / 2, triangles,
                                     SUN_SHADOW_MAX_VERTICES / 2, transform);
    }
    WebSunShadows_EndCapture(eye, forward, sun, sunColor);
    draws = 0;
    WebSunShadows_Render();
    assert(draws == 1 && drawVertices == SUN_SHADOW_MAX_VERTICES && drawIndices == SUN_SHADOW_MAX_VERTICES / 2 * 3 * 2);
    puts("PASS: the sun's map of moving models: rays keep their map coordinates and gain depth away from the sun, a "
         "square 640 units ahead snapped to whole texels, clip space from the same matrix, no shadow from a low, dim or "
         "missing sun; the casters' padded rectangle; the cgame's models only, in world space with rebased indices, "
         "near the last eye, without broken arguments or triangles; one map per frame from the main view's capture, "
         "none without casters, a sun, a view, a drawn map or r_sunShadows; overflow keeps what fits");
    return 0;
}
'''

mutants = [
    ('    if (ent->reType != 0 || (ent->renderFxFlags & 8) || !WebSunShadows_Finite3(ent->origin))',
     '    if (ent->reType != 0 || !WebSunShadows_Finite3(ent->origin))'),
    ('    if (ent->reType != 0 || (ent->renderFxFlags & 8) || !WebSunShadows_Finite3(ent->origin))',
     '    if ((ent->renderFxFlags & 8) || !WebSunShadows_Finite3(ent->origin))'),
    ('        if (dx * dx + dy * dy + dz * dz > SUN_SHADOW_CASTER_RANGE * SUN_SHADOW_CASTER_RANGE)',
     '        if (0)'),
    ('        if (corner[0] >= vertexCount || corner[1] >= vertexCount || corner[2] >= vertexCount)\n            continue;\n',
     ''),
    ('            s_indices[s_indexCount++] = base + corner[j];', '            s_indices[s_indexCount++] = corner[j];'),
    ('p[0] * transform[j] + p[1] * transform[3 + j] + p[2] * transform[6 + j] + transform[9 + j];',
     'p[0] * transform[j * 3] + p[1] * transform[j * 3 + 1] + p[2] * transform[j * 3 + 2] + transform[9 + j];'),
    ('    s_capturing = 0;\n    if (!WebSunShadows_Finite3(eye)', '    if (!WebSunShadows_Finite3(eye)'),
    ('    s_viewReady = 0;\n    memset(texture, 0, sizeof(texture));', '    memset(texture, 0, sizeof(texture));'),
    ('    if (sun[2] / length < SUN_SHADOW_MIN_ELEVATION)\n        return 0;\n', ''),
    ('fmaxf(sunColor[0], fmaxf(sunColor[1], sunColor[2])) > SUN_SHADOW_MIN_COLOR', '1'),
    ('        f[i] = -sun[i] / length;', '        f[i] = sun[i] / length;'),
    ('        centre[i] = eye[i] + ahead[i] * SUN_SHADOW_AHEAD;', '        centre[i] = eye[i];'),
    ('a = floorf((centre[0] * r[0] + centre[1] * r[1] + centre[2] * r[2]) / texel + 0.5f) * texel;',
     'a = centre[0] * r[0] + centre[1] * r[1] + centre[2] * r[2];'),
    ('    inverseDepth = 0.5f / SUN_SHADOW_DEPTH;', '    inverseDepth = 1.0f / SUN_SHADOW_DEPTH;'),
    ('        clip[i] = i % 4 == 3 ? texture[i] : 2.0f * texture[i] - (i >= 12 ? 1.0f : 0.0f);',
     '        clip[i] = i % 4 == 3 ? texture[i] : 2.0f * texture[i];'),
    ('        rect[axis] = fmaxf(rect[axis] - pad, 0.0f);', '        rect[axis] = rect[axis] - pad;'),
    ('        float p[3] = {corner & 1 ? maxs[0] : mins[0], corner & 2 ? maxs[1] : mins[1], corner & 4 ? maxs[2] : mins[2]};',
     '        float p[3] = {corner & 1 ? maxs[0] : mins[0], corner & 2 ? maxs[1] : mins[1], mins[2]};'),
    ('        params[SUN_SHADOW_DEPTH_BIAS] = SUN_SHADOW_BIAS * 0.5f / SUN_SHADOW_DEPTH;',
     '        params[SUN_SHADOW_DEPTH_BIAS] = SUN_SHADOW_BIAS;'),
    ('    s_capturing = WebSunShadows_Enabled();', '    s_capturing = 1;'),
    ('    if (vertexCount > SUN_SHADOW_MAX_VERTICES - s_vertexCount ||', '    if (vertexCount > SUN_SHADOW_MAX_VERTICES ||'),
]

with tempfile.TemporaryDirectory(prefix='cod2-sun-shadows-') as tmp:
    path = Path(tmp)
    (path / 'emscripten.h').write_text(emscripten)
    (path / 'common_types.h').write_text(common_types)
    (path / 'test.c').write_text(test.replace('SOURCE', str(source)))
    subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Werror', '-I', str(path),
                    '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
    for index, (before, after) in enumerate(mutants):
        assert module.count(before) == 1, before
        mutant = path / f'mutant{index}.c'
        mutant.write_text(module.replace(before, after))
        (path / f'mutant{index}-test.c').write_text(test.replace('SOURCE', str(mutant)))
        subprocess.run(['cc', '-std=c99', '-O0', '-w', '-I', str(path), str(path / f'mutant{index}-test.c'),
                        '-lm', '-o', str(path / f'mutant{index}')], check=True)
        result = subprocess.run([str(path / f'mutant{index}')], capture_output=True)
        assert result.returncode != 0, f'mutant survived: {after!r}'
    print(f'PASS: {len(mutants)} sun shadow mutants fail')
