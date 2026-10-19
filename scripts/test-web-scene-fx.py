#!/usr/bin/env python3
"""Exercise the production scene post-processing view/frame logic."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = root / 'downstream/wasm/web_scene_fx.c'
backend = (root / 'src/PC/gfx_d3d/rb_backend.c').read_text()
# The view begins before RB_Set3D, composes before the first 2D draw and
# always finishes before Present.
begin = backend.index('WebSceneFx_BeginView(vp->viewport.X')
assert begin < backend.index('RB_Set3D();', begin)
# The world depth is linearized with the view's own infinite projection, and
# the aim focus comes from the cgame each scene.
assert backend.index('vp->depthHackNearClip,', begin) < \
    backend.index('-((const float *)&vp->projectionMatrix)[14]);', begin) < backend.index('RB_Set3D();', begin)
# Motion blur compares each view's camera with the last frame's.
camera = backend.index('WebSceneFx_SetCamera((const float *)&vp->viewMatrix, (const float *)&vp->projectionMatrix);')
assert backend.rindex('#ifdef __EMSCRIPTEN__', 0, begin) < camera < begin
scene = (root / 'src/PC/gfx_d3d/r_scene.c').read_text()
assert 'WebSceneFx_SetAds(g_webAdsDofFrac, g_webAdsViewmodelStart, g_webAdsViewmodelEnd, g_webAdsNearEnd);' in scene
assert scene.index('WebSceneFx_SetAds(g_webAdsDofFrac') < scene.index('WebSceneFx_SetAim(g_webAimFrac);') < \
    scene.index('WebSceneFx_SetBackdrop(g_webBackdropFrac);') < scene.index('#endif', scene.index('WebSceneFx_SetAim('))
# The end of a match blurs the view behind its scoreboard.
assert 'float g_webBackdropFrac = 0.0f;' in (root / 'src/PC/cgame_mp/cg_endgame_mp.c').read_text()
# The aim fraction is read before any early return, so scopes, which hide the
# gun, still fade the blur out.
weapons = (root / 'src/PC/cgame_mp/cg_weapons.c').read_text()
view_weapon = weapons.index('void CG_AddViewWeapon(playerState_t *ps)\n{')
assert view_weapon < weapons.index('g_webAimFrac = playerState->fWeaponPosFrac;', view_weapon) < \
    weapons.index('return;', view_weapon)
dvars = (root / 'src/PC/gfx_d3d/r_dvars.c').read_text()
assert 'r_motionBlur = ri.Dvar_RegisterBool("r_motionBlur", 1, 0x2001);' in dvars
assert 'r_sceneHeight = ri.Dvar_RegisterInt("r_sceneHeight", 720, 0, 2160, 0x2001);' in dvars
assert 'mtx[2][2] = c;' in (root / 'src/PC/universal/com_math.c').read_text()
set2d = backend.index('static void RB_Set2D(void)')
assert set2d < backend.index('WebSceneFx_EndView();', set2d) < backend.index('backEnd.projection2D = 1;', set2d)
end_frame = backend.index('static void RB_EndFrame_real(void)')
assert end_frame < backend.index('WebSceneFx_EndFrame();', end_frame) < backend.index('vtable[0x44 / 4]', end_frame)

# The compositor does not read back or restore GL write masks because the D3D9
# device sets both from its own state for every clear and draw.
device = (root / 'src/Mac/DirectX_9/CDirect3DDevice.c').read_text()
def device_function(name):
    start = device.index('HRESULT ' + name + '(')
    return device[start:device.index('\nHRESULT ', start + 1)]
clear = device_function('CDirect3DDevice_Clear')
assert 'glColorMask((dev->colorWriteEnable & 1) != 0' in clear and 'glDepthMask(dev->zWriteEnable ? 1 : 0);' in clear
draw = device_function('CDirect3DDevice_DrawIndexedPrimitive')
assert 'glDepthMask(dev->zWriteEnable ? 1 : 0);' in draw and 'glDepthMask(0);' in draw
assert draw.index('glColorMask(\n            (dev->colorWriteEnable & 1) != 0') < draw.index('glColorMask(1, 1, 1, 1);')
compositor = source.read_text()
assert 'COLOR_WRITEMASK' not in compositor and 'DEPTH_WRITEMASK' not in compositor
# The compositor reads the backdrop where the C side writes it.
assert 'backdrop = H[o + 33];' in compositor
# And the scope's window: centre 34-35, half-axes 36-39, presence 40, open 41,
# blur 42, darkness 43.
assert 'var lensPresence = H[o + 40], lensBlur = H[o + 42];' in compositor
assert 'gl.uniform4f(C.u_lens, (H[o + 34] + 1) * 0.5 * w, (H[o + 35] + 1) * 0.5 * h, lensPresence, H[o + 41]);' in compositor
assert 'var ax = H[o + 36], ay = H[o + 37], bx = H[o + 38], by = H[o + 39];' in compositor
assert 'gl.uniform4f(C.u_lensFx, lensBlur, H[o + 43],' in compositor
# The view weapon's draws, and only those, get its projection and the
# stencil that keeps it off the glass; every way out of the depth hack
# lifts the stencil, and only leaving for another entity restores the
# projection by hand (popping the matrix stack restores it otherwise).
rb_state = (root / 'src/PC/gfx_d3d/rb_state.c').read_text()
assert ('    projection->_11 = scale ? scale[0] : view[0];\n    projection->_22 = scale ? scale[1] : view[5];\n'
        '    RB_SetDepthHackNearClip(nearClip);\n') in rb_state
enter = backend.index('            if (depthRange == 8) {\n')
assert backend.index('RB_SetViewmodelProjection(WebSceneFx_ViewmodelNearClip(backEnd.viewParms->depthHackNearClip),\n'
                     '                                          WebSceneFx_ViewmodelProjection());', enter) < \
    backend.index('WebSceneFx_BeginViewmodel();', enter) < backend.index('RB_SetDepthRange(0.0f, 0.2f);', enter)
assert backend.count('WebSceneFx_BeginViewmodel();') == 1
assert backend.count('                WebSceneFx_EndViewmodel();\n                RB_SetViewmodelProjection(nc, NULL);\n') == 1
assert backend.count('            WebSceneFx_EndViewmodel();\n            RB_SetViewmodelProjection(nc, NULL);\n') == 1
world = backend.index('        if (!isWorldSpaceEntPrev)\n            RB_PopMatrixStack();\n        if (depthRangePrev) {\n')
assert backend.index('            if (depthRangePrev == 8)\n                WebSceneFx_EndViewmodel();\n', world) < \
    backend.index('            RB_SetDepthRange(0.0f, 1.0f);\n            depthRangePrev = 0;', world)
assert 'WebSceneFx_SetScope(g_webScopeLens);' in scene and \
    scene.index('WebSceneFx_SetBackdrop(g_webBackdropFrac);') < scene.index('WebSceneFx_SetScope(g_webScopeLens);')
# The cgame's lens, the floats R_RenderScene hands over and the compositor's
# scope floats are the same thirteen.
lens_struct = (root / 'src/headers/PC/cgame_mp/cg_wwiiscope.h').read_text()
lens_struct = lens_struct[lens_struct.index('typedef struct CgWwiiScopeLens {'):lens_struct.index('} CgWwiiScopeLens;')]
lens_floats = 0
for dims in re.findall(r'^\s+float \w+((?:\[\d+\])*);$', lens_struct, re.M):
    count = 1
    for dim in re.findall(r'\[(\d+)\]', dims):
        count *= int(dim)
    lens_floats += count
assert lens_floats == 13, lens_floats
assert 'extern float g_webScopeLens[13];' in scene
assert 'float g_webScopeLens[13];' in (root / 'src/PC/cgame_mp/cg_wwiiscope_mp.c').read_text()

emscripten = '#define EM_JS(ret, name, params, ...) ret name params;\ndouble emscripten_get_now(void);\n'
common_types = r'''
typedef union {int enabled; int integer; float value;} DvarValue;
typedef struct {DvarValue current;} dvar_t;
'''
test = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "SOURCE"
static dvar_t glow, radius0, radius1, intensity0, intensity1, cutoff, desaturation, fullbright, samples, dof, film,
    motion, scene;
const dvar_t *r_glow = &glow;
const dvar_t *r_glowRadius[2] = {&radius0, &radius1};
const dvar_t *r_glowBloomIntensity[2] = {&intensity0, &intensity1};
const dvar_t *r_glowBloomCutoff = &cutoff;
const dvar_t *r_glowBloomDesaturation = &desaturation;
const dvar_t *r_fullbright = &fullbright;
const dvar_t *r_aaSamples = &samples;
const dvar_t *r_dof_enable = &dof;
const dvar_t *r_filmTweakEnable = &film;
const dvar_t *r_motionBlur = &motion;
const dvar_t *r_sceneHeight = &scene;
static double clockMs = 1000.0;
double emscripten_get_now(void)
{
    return clockMs;
}
static int binds, composes, bindResult = 1, boundWidth, boundHeight, boundSamples, boundScreenWidth, boundScreenHeight;
static float composed[SCENE_FX_PARAM_COUNT];
int WebSceneFx_Bind(int width, int height, int sampleCount, int screenWidth, int screenHeight)
{
    ++binds; boundWidth = width; boundHeight = height; boundSamples = sampleCount;
    boundScreenWidth = screenWidth; boundScreenHeight = screenHeight;
    return bindResult;
}
void WebSceneFx_Compose(const float *params)
{
    ++composes;
    memcpy(composed, params, sizeof(composed));
}
static int stencils, masks, maskOn;
static float stenciled[SCENE_FX_SCOPE_FLOATS];
void WebSceneFx_ScopeStencil(const float *scope)
{
    ++stencils;
    memcpy(stenciled, scope, sizeof(stenciled));
}
void WebSceneFx_ScopeMask(int on)
{
    ++masks;
    maskOn = on;
}
static void defaults(void)
{
    glow.current.enabled = 1; radius0.current.value = 5; radius1.current.value = 12;
    intensity0.current.value = 1; intensity1.current.value = 1; cutoff.current.value = 0.9f;
    desaturation.current.value = 0.75f; fullbright.current.enabled = 0; samples.current.integer = 4;
    dof.current.enabled = 1; film.current.enabled = 1; motion.current.enabled = 1; scene.current.integer = 720;
}
static void frameNear(int x, int width, float nearClip, float worldNearClip)
{
    WebSceneFx_BeginView(x, 0, width, 720, 1280, 720, nearClip, worldNearClip);
    WebSceneFx_EndView();
    WebSceneFx_EndFrame();
}
static void frame(int x, int width, float nearClip)
{
    frameNear(x, width, nearClip, 4.0f);
}
/* A D3D view (row vectors) of CoD's x-forward, y-left, z-up world, with
 * pitch positive down like the game's angles. */
static float lens[16] = {1.19f, 0, 0, 0, 0, 2.11f, 0, 0, 0, 0, 0.9995f, 1, 0, 0, -3.998f, 0};
static float lastView[16], view[16];
static void camera(float yaw, float pitch, float x, float y, float z)
{
    const float f[3] = {cosf(pitch) * cosf(yaw), cosf(pitch) * sinf(yaw), -sinf(pitch)};
    const float l[3] = {-sinf(yaw), cosf(yaw), 0};
    const float u[3] = {sinf(pitch) * cosf(yaw), sinf(pitch) * sinf(yaw), cosf(pitch)};
    const float o[3] = {x, y, z};
    memcpy(lastView, view, sizeof(view));
    memset(view, 0, sizeof(view));
    for (int i = 0; i < 3; ++i) {
        view[i * 4] = -l[i]; view[i * 4 + 1] = u[i]; view[i * 4 + 2] = f[i];
        view[12] += l[i] * o[i]; view[13] -= u[i] * o[i]; view[14] -= f[i] * o[i];
    }
    view[15] = 1;
    WebSceneFx_SetCamera(view, lens);
}
static float moved(float ms, float yaw, float pitch, float x, float y, float z)
{
    clockMs += ms;
    camera(yaw, pitch, x, y, z);
    frame(0, 1280, 0.1f);
    return composed[SCENE_FX_MOTION];
}
static void viewPoint(const float *matrix, const float *p, float *out)
{
    for (int j = 0; j < 3; ++j)
        out[j] = matrix[12 + j] + p[0] * matrix[j] + p[1] * matrix[4 + j] + p[2] * matrix[8 + j];
}
/* The composed rotation and offset take what this camera sees of a world
 * point to what the last camera saw of it. */
static void assertReprojects(const float *p)
{
    float now[3], before[3];
    viewPoint(view, p, now);
    viewPoint(lastView, p, before);
    for (int j = 0; j < 3; ++j) {
        float back = composed[SCENE_FX_MOTION_MOVE + j];
        for (int k = 0; k < 3; ++k)
            back += composed[SCENE_FX_MOTION_ROTATION + k * 3 + j] * now[k];
        assert(fabsf(back - before[j]) < 2e-3f * (1.0f + fabsf(before[j])));
    }
}
int main(void)
{
    const float defaultWorldNearClip = 0.9995002746582031f * 4.0f;
    defaults();
    for (int i = 0; i < 600; ++i) {
        binds = composes = 0;
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 7.996f);
        assert(binds == 1 && boundWidth == 1280 && boundHeight == 720 && boundSamples == 4 && !composes);
        assert(boundScreenWidth == 1280 && boundScreenHeight == 720);
        /* RB_Set2D composes once; later 2D batches and views draw directly. */
        WebSceneFx_EndView(); WebSceneFx_EndView();
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 2.0f);
        WebSceneFx_EndView();
        WebSceneFx_EndFrame();
        assert(binds == 1 && composes == 1);
    }
    assert(fabsf(composed[SCENE_FX_GLOW0] - 0.42f) < 1e-6f && fabsf(composed[SCENE_FX_GLOW1] - 0.30f) < 1e-6f);
    assert(composed[SCENE_FX_CUTOFF] == 0.9f && composed[SCENE_FX_DESATURATION] == 0.75f);
    assert(composed[SCENE_FX_RADIUS0] == 5 && composed[SCENE_FX_RADIUS1] == 12);
    assert(composed[SCENE_FX_ADS] == 0 && composed[SCENE_FX_NEAR_CLIP] == 0.1f && composed[SCENE_FX_NEAR_BLUR] == 3);
    assert(composed[SCENE_FX_SHARPEN] > 0 && composed[SCENE_FX_FILM] == 1);
    /* The composed view's own projection linearizes the world; until the
     * cgame aims, the near world blur reaches 512 units at full strength. */
    assert(composed[SCENE_FX_WORLD_NEAR_CLIP] == 7.996f);
    assert(composed[SCENE_FX_NEAR_END] == 512 && composed[SCENE_FX_WORLD_BLUR] == 1);

    /* A view left open without a 2D pass still composes before Present. */
    binds = composes = 0;
    WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
    WebSceneFx_EndFrame();
    assert(binds == 1 && composes == 1);
    /* A second full-screen view composes the first before drawing directly. */
    binds = composes = 0;
    WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
    WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
    assert(binds == 1 && composes == 1);
    WebSceneFx_EndFrame();
    assert(composes == 1);

    /* Partial views (cg_viewsize, split views) keep the original direct path. */
    binds = composes = 0;
    frame(8, 1264, 0.1f);
    frame(0, 1024, 0.1f);
    assert(!binds && !composes);

    /* r_fullbright clears before its view begins; it renders directly. */
    fullbright.current.enabled = 1;
    frame(0, 1280, 0.1f);
    assert(!binds && !composes);
    fullbright.current.enabled = 0;

    /* With every effect and MSAA off nothing changes, until the player aims. */
    glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1;
    WebSceneFx_SetAds(0, 6, 15, 512);
    frame(0, 1280, 0.1f);
    assert(!binds && !composes);
    WebSceneFx_SetAds(0.5f, 6, 15, 170);
    frame(0, 1280, 0.1f);
    assert(binds == 1 && composes == 1 && boundSamples == 1);
    assert(composed[SCENE_FX_GLOW0] == 0 && composed[SCENE_FX_GLOW1] == 0 && composed[SCENE_FX_SHARPEN] == 0);
    assert(composed[SCENE_FX_FILM] == 0 && composed[SCENE_FX_CONTRAST] == 0 && composed[SCENE_FX_ADS] == 0.5f);
    assert(composed[SCENE_FX_NEAR_END] == 170);
    dof.current.enabled = 0;
    frame(0, 1280, 0.1f);
    assert(binds == 1 && composes == 1);
    defaults();
    frame(0, 1280, 0.1f);
    assert(binds == 2 && composes == 2 && composed[SCENE_FX_ADS] == 0.5f);
    /* r_dof_enable 0 keeps aiming sharp while the other effects remain. */
    dof.current.enabled = 0;
    frame(0, 1280, 0.1f);
    assert(composes == 3 && composed[SCENE_FX_ADS] == 0);
    defaults();

    /* Glow follows the original r_glow* dvars and stays bounded. */
    intensity0.current.value = 2; intensity1.current.value = NAN; cutoff.current.value = 1.5f;
    desaturation.current.value = -1; radius0.current.value = INFINITY; radius1.current.value = 0;
    frame(0, 1280, NAN);
    assert(fabsf(composed[SCENE_FX_GLOW0] - 0.84f) < 1e-6f && composed[SCENE_FX_GLOW1] == 0);
    assert(composed[SCENE_FX_CUTOFF] == 0.99f && composed[SCENE_FX_DESATURATION] == 0);
    assert(composed[SCENE_FX_RADIUS0] == 0 && composed[SCENE_FX_RADIUS1] == 0);
    assert(composed[SCENE_FX_NEAR_CLIP] == 0.1f);
    defaults();
    frame(0, 1280, 0.25f);
    assert(composed[SCENE_FX_NEAR_CLIP] == 0.25f);
    /* A projection without a usable near plane falls back to r_znear 4. */
    frameNear(0, 1280, 0.1f, NAN);
    assert(composed[SCENE_FX_WORLD_NEAR_CLIP] == defaultWorldNearClip);
    frameNear(0, 1280, 0.1f, -3.998f);
    assert(composed[SCENE_FX_WORLD_NEAR_CLIP] == defaultWorldNearClip);
    frameNear(0, 1280, 0.1f, 0.4f);
    assert(composed[SCENE_FX_WORLD_NEAR_CLIP] == 0.4f);

    /* Focus ranges are clamped to valid, non-empty intervals; without a
     * finite near world focus nothing past the eye blurs. */
    WebSceneFx_SetAds(NAN, NAN, NAN, NAN);
    frame(0, 1280, 0.1f);
    assert(composed[SCENE_FX_ADS] == 0 && composed[SCENE_FX_VM_END] == 2 && composed[SCENE_FX_VM_START] == 0);
    assert(composed[SCENE_FX_NEAR_END] == 1);
    WebSceneFx_SetAds(4, 30, 12, 5000);
    frame(0, 1280, 0.1f);
    assert(composed[SCENE_FX_ADS] == 1 && composed[SCENE_FX_VM_END] == 12 && composed[SCENE_FX_VM_START] == 11.5f);
    assert(composed[SCENE_FX_NEAR_END] == 1024);
    WebSceneFx_SetAds(0.25f, 5, 500, 0.5f);
    frame(0, 1280, 0.1f);
    assert(composed[SCENE_FX_ADS] == 0.25f && composed[SCENE_FX_VM_END] == 64 && composed[SCENE_FX_VM_START] == 5);
    assert(composed[SCENE_FX_NEAR_END] == 1);

    /* A failed offscreen target never composes stale state. */
    bindResult = 0; composes = 0;
    for (int i = 0; i < 10; ++i)
        frame(0, 1280, 0.1f);
    assert(!composes);
    bindResult = 1;
    frame(0, 1280, 0.1f);
    assert(composes == 1);

    /* Motion blur spans 10 ms of the camera's turn and move since the last
     * frame. */
    defaults();
    WebSceneFx_SetAds(0, 6, 15, 512);
    clockMs = 5000;
    camera(0.3f, 0.1f, 100, 200, 60);
    frame(0, 1280, 0.1f);
    assert(composed[SCENE_FX_MOTION] == 0);
    assert(moved(10, 0.36f, 0.07f, 101.5f, 198.8f, 60.5f) == 1);
    const float points[][3] = {{400, 260, 80}, {130, 180, 40}, {-500, 900, 300}, {100000, 30000, 2000}};
    for (int i = 0; i < 4; ++i)
        assertReprojects(points[i]);
    assert(composed[SCENE_FX_MOTION_PROJECTION] == 1.19f && composed[SCENE_FX_MOTION_PROJECTION + 1] == 2.11f);
    assert(moved(10, 0.36f, 0.07f, 103.4f, 198.8f, 60.5f) == 1);
    assertReprojects(points[1]);
    assertReprojects(points[3]);
    /* Any frame rate draws the same exposure. */
    assert(moved(5, 0.38f, 0.07f, 103.4f, 198.8f, 60.5f) == 2);
    assert(moved(40, 0.40f, 0.07f, 103.4f, 198.8f, 60.5f) == 0.25f);
    /* Raising the sights fades it out. */
    WebSceneFx_SetAim(0.25f);
    assert(moved(10, 0.42f, 0.07f, 103.4f, 198.8f, 60.5f) == 0.75f);
    WebSceneFx_SetAim(1);
    assert(moved(10, 0.44f, 0.07f, 103.4f, 198.8f, 60.5f) == 0);
    WebSceneFx_SetAim(NAN);
    assert(moved(10, 0.46f, 0.07f, 103.4f, 198.8f, 60.5f) == 1);
    WebSceneFx_SetAim(0);
    /* Cuts start over: a turn past 0.35 rad, a move of 48 units, a gap past
     * 100 ms or under 1 ms. */
    assert(moved(10, 0.86f, 0.07f, 103.4f, 198.8f, 60.5f) == 0);
    assert(moved(10, 1.16f, 0.07f, 103.4f, 198.8f, 60.5f) == 1);
    assert(moved(10, 1.16f, 0.07f, 153.4f, 198.8f, 60.5f) == 0);
    assert(moved(10, 1.16f, 0.07f, 193.4f, 198.8f, 60.5f) == 1);
    assert(moved(150, 1.18f, 0.07f, 193.4f, 198.8f, 60.5f) == 0);
    assert(moved(0.5f, 1.20f, 0.07f, 193.4f, 198.8f, 60.5f) == 0);
    assert(moved(10, 1.22f, 0.07f, 193.4f, 198.8f, 60.5f) == 1);
    /* A camera standing still or drifting adds nothing. */
    assert(moved(10, 1.22f, 0.07f, 193.4f, 198.8f, 60.5f) == 0);
    assert(moved(10, 1.2201f, 0.07f, 193.4f, 198.8f, 60.5f) == 0);
    assert(moved(10, 1.2206f, 0.07f, 193.4f, 198.8f, 60.5f) == 1);
    assert(moved(10, 1.2206f, 0.07f, 193.42f, 198.8f, 60.5f) == 0);
    assert(moved(10, 1.2206f, 0.07f, 193.52f, 198.8f, 60.5f) == 1);
    /* r_motionBlur 0 draws none and keeps tracking. */
    motion.current.enabled = 0;
    assert(moved(10, 1.30f, 0.07f, 193.52f, 198.8f, 60.5f) == 0);
    motion.current.enabled = 1;
    assert(moved(10, 1.32f, 0.07f, 193.52f, 198.8f, 60.5f) == 1);
    /* A broken camera is no reference for the next one. */
    assert(moved(10, NAN, 0.07f, 193.52f, 198.8f, 60.5f) == 0);
    assert(moved(10, 1.34f, 0.07f, 193.52f, 198.8f, 60.5f) == 0);
    assert(moved(10, 1.36f, 0.07f, 193.52f, 198.8f, 60.5f) == 1);
    lens[0] = 0;
    assert(moved(10, 1.38f, 0.07f, 193.52f, 198.8f, 60.5f) == 0);
    lens[0] = 1.19f;
    assert(moved(10, 1.40f, 0.07f, 193.52f, 198.8f, 60.5f) == 0);
    assert(moved(10, 1.42f, 0.07f, 193.52f, 198.8f, 60.5f) == 1);
    /* With every other effect off, only a moving camera composes. */
    glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1;
    binds = composes = 0;
    moved(10, 1.42f, 0.07f, 193.52f, 198.8f, 60.5f);
    assert(!binds && !composes);
    moved(10, 1.44f, 0.07f, 193.52f, 198.8f, 60.5f);
    assert(binds == 1 && composes == 1 && boundSamples == 1 && composed[SCENE_FX_MOTION] == 1);
    assert(composed[SCENE_FX_GLOW0] == 0 && composed[SCENE_FX_SHARPEN] == 0 && composed[SCENE_FX_ADS] == 0);
    WebSceneFx_SetAim(1);
    moved(10, 1.46f, 0.07f, 193.52f, 198.8f, 60.5f);
    WebSceneFx_SetAim(0);
    motion.current.enabled = 0;
    moved(10, 1.48f, 0.07f, 193.52f, 198.8f, 60.5f);
    motion.current.enabled = 1;
    fullbright.current.enabled = 1;
    moved(10, 1.50f, 0.07f, 193.52f, 198.8f, 60.5f);
    fullbright.current.enabled = 0;
    assert(binds == 1 && composes == 1);
    /* Fullbright frames keep tracking; a partial view does not, and the next
     * full view measures from the last full one. */
    assert(moved(10, 1.52f, 0.07f, 193.52f, 198.8f, 60.5f) == 1 && binds == 2);
    clockMs += 10;
    camera(1.54f, 0.07f, 193.52f, 198.8f, 60.5f);
    frame(8, 1264, 0.1f);
    assert(binds == 2);
    assert(moved(10, 1.56f, 0.07f, 193.52f, 198.8f, 60.5f) == 0.5f && binds == 3);

    /* A menu's blurred backdrop composes on its own, whatever the other
     * effects, and stays between none and all of it. */
    assert(SCENE_FX_BACKDROP == 33 && SCENE_FX_PARAM_COUNT == 47);
    defaults();
    glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1; dof.current.enabled = 0;
    motion.current.enabled = 0;
    binds = composes = 0;
    frame(0, 1280, 0.1f);
    assert(!binds && !composes);
    WebSceneFx_SetBackdrop(0.6f);
    frame(0, 1280, 0.1f);
    assert(binds == 1 && composes == 1 && composed[SCENE_FX_BACKDROP] == 0.6f && composed[SCENE_FX_ADS] == 0);
    WebSceneFx_SetBackdrop(4);
    frame(0, 1280, 0.1f);
    assert(composes == 2 && composed[SCENE_FX_BACKDROP] == 1);
    WebSceneFx_SetBackdrop(NAN);
    frame(0, 1280, 0.1f);
    WebSceneFx_SetBackdrop(-1);
    frame(0, 1280, 0.1f);
    assert(composes == 2);
    defaults();
    frame(0, 1280, 0.1f);
    assert(composes == 3 && composed[SCENE_FX_BACKDROP] == 0);

    /* A screen taller than r_sceneHeight renders its scene at that many lines
     * and the screen's aspect, even with every effect off; a shorter one, or
     * r_sceneHeight 0, keeps the screen's size. */
    static const int sizes[][5] = {
        {1920, 1080, 720, 1280, 720}, {1600, 900, 720, 1280, 720}, {1366, 768, 720, 1281, 720},
        {2560, 1600, 720, 1152, 720}, {3840, 2160, 1080, 1920, 1080}, {100, 2000, 1, 1, 1},
        {1920, 1080, 0, 1920, 1080}, {1280, 720, 720, 1280, 720}, {1024, 768, 900, 1024, 768}};
    for (int effects = 0; effects < 2; ++effects) {
        for (unsigned k = 0; k < sizeof(sizes) / sizeof(sizes[0]); ++k) {
            const int *z = sizes[k];
            int scaled = z[3] != z[0] || z[4] != z[1];
            defaults();
            if (!effects) {
                glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1; dof.current.enabled = 0;
                motion.current.enabled = 0;
            }
            scene.current.integer = z[2];
            binds = composes = 0;
            boundWidth = boundHeight = boundScreenWidth = boundScreenHeight = 0;
            WebSceneFx_BeginView(0, 0, z[0], z[1], z[0], z[1], 0.1f, 4.0f);
            WebSceneFx_EndView();
            WebSceneFx_EndFrame();
            if (!effects && !scaled) {
                assert(!binds && !composes);
                continue;
            }
            assert(binds == 1 && composes == 1 && boundWidth == z[3] && boundHeight == z[4]);
            assert(boundScreenWidth == z[0] && boundScreenHeight == z[1] && boundSamples == (effects ? 4 : 1));
        }
    }
    /* Partial views on a taller screen still draw directly. */
    defaults();
    binds = composes = 0;
    WebSceneFx_BeginView(8, 0, 1904, 1080, 1920, 1080, 0.1f, 4.0f);
    WebSceneFx_EndView();
    WebSceneFx_EndFrame();
    assert(!binds && !composes);

    /* A scope's window: CgWwiiScopeLens's thirteen floats after the backdrop. */
    assert(SCENE_FX_SCOPE == 34 && SCENE_FX_SCOPE_AXES == 36 && SCENE_FX_SCOPE_PRESENCE == 40 &&
           SCENE_FX_SCOPE_OPEN == 41 && SCENE_FX_SCOPE_BLUR == 42 && SCENE_FX_SCOPE_DARK == 43 &&
           SCENE_FX_SCOPE_PROJECTION == 44 && SCENE_FX_SCOPE_NEAR_CLIP == 46 && SCENE_FX_SCOPE_FLOATS == 13);
    {
        const float lens[13] = {0.05f, -0.02f, 0.585f, -0.01f, 0.01f, 1.04f, 1, 0.5f, 0.5f, 0.25f, 1.7f, 3.0f, 0.8f};
        float bad[13];
        const float *projection;

        defaults();
        glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1; dof.current.enabled = 0;
        motion.current.enabled = 0;
        binds = composes = stencils = masks = 0;
        frame(0, 1280, 0.1f);
        assert(!binds && !composes);
        /* With every other effect off a window composes, and its floats reach
         * the compose pass. */
        WebSceneFx_SetScope(lens);
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
        assert(binds == 1 && WebSceneFx_ScopeAvailable());
        /* The rifle's projection while the composed view draws it, and its
         * near clip: the window's, unless the depth hack's is further. */
        projection = WebSceneFx_ViewmodelProjection();
        assert(projection && projection[0] == 1.7f && projection[1] == 3.0f);
        assert(WebSceneFx_ViewmodelNearClip(0.1f) == 0.8f && WebSceneFx_ViewmodelNearClip(2.0f) == 2.0f);
        /* The stencil is drawn once per view, before the rifle's first draw;
         * each of its draws is masked and unmasked. */
        WebSceneFx_BeginViewmodel();
        assert(stencils == 1 && masks == 1 && maskOn == 1 && !memcmp(stenciled, lens, sizeof(lens)));
        WebSceneFx_EndViewmodel();
        assert(masks == 2 && maskOn == 0);
        WebSceneFx_EndViewmodel();
        assert(masks == 2);
        WebSceneFx_BeginViewmodel();
        assert(stencils == 1 && masks == 3 && maskOn == 1);
        /* A view that ends while the rifle draws still lifts the mask. */
        WebSceneFx_EndView();
        assert(masks == 4 && maskOn == 0 && composes == 1);
        for (int i = 0; i < 13; ++i)
            assert(composed[SCENE_FX_SCOPE + i] == lens[i]);
        /* The rifle's depth is linearized with the near clip it was drawn with. */
        assert(composed[SCENE_FX_NEAR_CLIP] == 0.8f);
        WebSceneFx_EndFrame();
        /* Outside a composed view the rifle keeps the view's projection and
         * near clip and draws without the stencil. */
        assert(!WebSceneFx_ViewmodelProjection());
        assert(WebSceneFx_ViewmodelNearClip(0.1f) == 0.1f);
        WebSceneFx_BeginViewmodel();
        WebSceneFx_EndViewmodel();
        assert(stencils == 1 && masks == 4);
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
        WebSceneFx_BeginViewmodel();
        assert(stencils == 2);
        WebSceneFx_EndFrame();
        assert(maskOn == 0);
        /* The rifle's projection alone composes, before its glass opens. */
        memcpy(bad, lens, sizeof(bad));
        bad[6] = 0;
        WebSceneFx_SetScope(bad);
        binds = composes = stencils = 0;
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
        WebSceneFx_BeginViewmodel();
        assert(binds == 1 && WebSceneFx_ViewmodelProjection() && !stencils);
        WebSceneFx_EndFrame();
        /* Finite and bounded: the fractions clamp, an outline without area
         * closes the window, a non-positive projection is dropped and the
         * near clip stays within the depth hack's range. */
        for (int i = 0; i < 13; ++i)
            bad[i] = NAN;
        WebSceneFx_SetScope(bad);
        binds = 0;
        frame(0, 1280, 0.1f);
        assert(!binds);
        memcpy(bad, lens, sizeof(bad));
        bad[12] = 1e9f;
        WebSceneFx_SetScope(bad);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_SCOPE_NEAR_CLIP] == 16.0f && composed[SCENE_FX_NEAR_CLIP] == 16.0f);
        bad[12] = -1;
        WebSceneFx_SetScope(bad);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_SCOPE_NEAR_CLIP] == 0 && composed[SCENE_FX_NEAR_CLIP] == 0.1f);
        bad[12] = NAN;
        WebSceneFx_SetScope(bad);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_SCOPE_NEAR_CLIP] == 0 && composed[SCENE_FX_NEAR_CLIP] == 0.1f);
        memcpy(bad, lens, sizeof(bad));
        bad[6] = 3; bad[7] = -1; bad[8] = 2; bad[9] = INFINITY;
        WebSceneFx_SetScope(bad);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_SCOPE_PRESENCE] == 1 && composed[SCENE_FX_SCOPE_OPEN] == 0 &&
               composed[SCENE_FX_SCOPE_BLUR] == 1 && composed[SCENE_FX_SCOPE_DARK] == 0);
        memcpy(bad, lens, sizeof(bad));
        bad[2] = bad[3] = 0;
        WebSceneFx_SetScope(bad);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_SCOPE_PRESENCE] == 0 && composed[SCENE_FX_SCOPE_PROJECTION] == 1.7f);
        memcpy(bad, lens, sizeof(bad));
        bad[11] = -3;
        WebSceneFx_SetScope(bad);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_SCOPE_PROJECTION] == 0 && composed[SCENE_FX_SCOPE_PROJECTION + 1] == 0);
        /* Without a composed target, after a failed bind or under
         * r_fullbright, the cgame keeps the stock overlay. */
        WebSceneFx_SetScope(lens);
        bindResult = 0;
        frame(0, 1280, 0.1f);
        assert(!WebSceneFx_ScopeAvailable());
        bindResult = 1;
        frame(0, 1280, 0.1f);
        assert(WebSceneFx_ScopeAvailable());
        fullbright.current.enabled = 1;
        frame(0, 1280, 0.1f);
        assert(!WebSceneFx_ScopeAvailable());
        fullbright.current.enabled = 0;
        frame(0, 1280, 0.1f);
        assert(WebSceneFx_ScopeAvailable());
        WebSceneFx_SetScope(NULL);
        binds = 0;
        frame(0, 1280, 0.1f);
        assert(!binds);
    }
    puts("PASS: 600 composed frames, single compose per frame before Present, partial/fullbright/disabled views drawn directly, bounded glow, viewmodel and near world focus parameters, camera motion reprojected over a 10 ms exposure with aim fade, cuts and still cameras, a bounded menu backdrop, scenes rendered at r_sceneHeight lines and the screen's aspect on taller screens, a scope window composed with its rifle projection and near clip and a stencil drawn once per view and lifted on every exit, bounded window floats and the stock overlay without a composed target");
    return 0;
}
'''

# Each mutant must fail the motion checks above.
mutants = [
    ('sum += last[i * 4 + j] * view[i * 4 + k];', 'sum += last[j * 4 + i] * view[k * 4 + i];'),
    ('rotation[k * 3 + j] = sum;', 'rotation[j * 3 + k] = sum;'),
    ('sum -= rotation[k * 3 + j] * view[12 + k];', 'sum += rotation[k * 3 + j] * view[12 + k];'),
    (' * (1.0f - s_aimFrac);', ';'),
    ('gap >= 1.0 && gap <= SCENE_FX_MOTION_MAX_GAP', 'gap >= 1.0 && gap <= 1000.0'),
    ('gap >= 1.0 && gap', 'gap'),
    ('moved < SCENE_FX_MOTION_MAX_MOVE', 'moved < 1e9f'),
    ('trace > 1.0f + 2.0f * cosf(SCENE_FX_MOTION_MAX_TURN)', 'trace > -1.0f'),
    ('(turn > SCENE_FX_MOTION_MIN_TURN || moved > SCENE_FX_MOTION_MIN_MOVE)', '1'),
    ('(float)(SCENE_FX_MOTION_EXPOSURE / gap)', '(float)(SCENE_FX_MOTION_EXPOSURE / 10.0)'),
    ('s_lastViewValid = valid;', 's_lastViewValid = 1;'),
    ('WebSceneFx_Enabled(r_motionBlur) && ', ''),
    (' && s_motionScale <= 0.0f && ', ' && '),
    (' && s_backdropFrac <= 0.0f &&', ' &&'),
    ('s_backdropFrac <= 0.0f &&\n        !WebSceneFx_ScopeRequested())', 's_backdropFrac <= 0.0f)'),
    ('    s_scopeUnavailable = !s_active;\n', '    s_scopeUnavailable = 0;\n'),
    ('        s_scopeUnavailable = 1;\n        return;', '        return;'),
    ('    if (!s_scopeStencil) {', '    if (1) {'),
    ('    s_scopeStencil = 0;\n    WebSceneFx_TrackCamera();', '    WebSceneFx_TrackCamera();'),
    ('    WebSceneFx_ScopeMask(1);\n    s_scopeMasking = 1;', '    WebSceneFx_ScopeMask(1);'),
    ('    WebSceneFx_EndViewmodel();\n    if (!s_active)\n        return;\n    s_active = 0;',
     '    if (!s_active)\n        return;\n    s_active = 0;'),
    ('    return s_active && projection[0] > 0.0f ? projection : NULL;',
     '    return projection[0] > 0.0f ? projection : NULL;'),
    ('    memcpy(params + SCENE_FX_SCOPE, s_scope, sizeof(s_scope));\n', ''),
    ('        s_scope[i - SCENE_FX_SCOPE] = WebSceneFx_Clamp(s_scope[i - SCENE_FX_SCOPE], 0.0f, 1.0f);', '        (void)0;'),
    ('    if (!(fabsf(axes[0] * axes[3] - axes[2] * axes[1]) > 1e-6f))\n'
     '        s_scope[SCENE_FX_SCOPE_PRESENCE - SCENE_FX_SCOPE] = 0.0f;\n', ''),
    ('    if (!(projection[0] > 0.0f) || !(projection[1] > 0.0f))\n        projection[0] = projection[1] = 0.0f;\n', ''),
    ('    *nearClip = WebSceneFx_Clamp(*nearClip, 0.0f, 16.0f);\n', ''),
    ('    return scope > nearClip ? scope : nearClip;', '    return scope;'),
    ('    return s_active ? WebSceneFx_ScopeNearClip(nearClip) : nearClip;',
     '    return WebSceneFx_ScopeNearClip(nearClip);'),
    ('params[SCENE_FX_NEAR_CLIP] = WebSceneFx_ScopeNearClip(s_nearClip);', 'params[SCENE_FX_NEAR_CLIP] = s_nearClip;'),
    ('params[SCENE_FX_BACKDROP] = s_backdropFrac;', 'params[SCENE_FX_BACKDROP] = 0.0f;'),
    ('s_backdropFrac = WebSceneFx_Clamp(backdropFrac, 0.0f, 1.0f);', 's_backdropFrac = backdropFrac;'),
    ('params[SCENE_FX_MOTION_PROJECTION + 1] = s_motionProjection[1];',
     'params[SCENE_FX_MOTION_PROJECTION + 1] = s_motionProjection[0];'),
    ('    WebSceneFx_TrackCamera();\n    /* r_fullbright clears the frame buffer before it begins the view. */\n'
     '    if (WebSceneFx_Enabled(r_fullbright)) {\n        s_scopeUnavailable = 1;\n        return;\n    }\n',
     '    /* r_fullbright clears the frame buffer before it begins the view. */\n'
     '    if (WebSceneFx_Enabled(r_fullbright)) {\n        s_scopeUnavailable = 1;\n        return;\n    }\n'
     '    WebSceneFx_TrackCamera();\n'),
    ('    if (lines > 0 && lines < height) {', '    if (lines > 0) {'),
    ('(float)lines / (float)height + 0.5f);', '(float)lines / (float)height);'),
    ('        if (*sceneWidth < 1)\n            *sceneWidth = 1;\n', ''),
    ('    scaled = sceneWidth != width || sceneHeight != height;', '    scaled = 0;'),
    ('WebSceneFx_Bind(sceneWidth, sceneHeight, samples, width, height);',
     'WebSceneFx_Bind(width, height, samples, width, height);'),
]

with tempfile.TemporaryDirectory(prefix='cod2-scene-fx-') as tmp:
    path = Path(tmp)
    (path / 'emscripten.h').write_text(emscripten)
    (path / 'common_types.h').write_text(common_types)
    (path / 'test.c').write_text(test.replace('SOURCE', str(source)))
    subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Werror', '-I', str(path),
                    '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
    original = source.read_text()
    for index, (before, after) in enumerate(mutants):
        assert original.count(before) == 1, before
        mutant = path / f'mutant{index}.c'
        mutant.write_text(original.replace(before, after))
        (path / f'mutant{index}-test.c').write_text(test.replace('SOURCE', str(mutant)))
        subprocess.run(['cc', '-std=c99', '-O0', '-w', '-I', str(path), str(path / f'mutant{index}-test.c'),
                        '-lm', '-o', str(path / f'mutant{index}')], check=True)
        result = subprocess.run([str(path / f'mutant{index}')], capture_output=True)
        assert result.returncode != 0, f'mutant survived: {after!r}'
    print(f'PASS: {len(mutants)} motion blur, backdrop, scene size and scope window mutants fail')
