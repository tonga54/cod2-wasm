#!/usr/bin/env python3
"""Exercise the production scene post-processing view/frame logic."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = root / 'downstream/wasm/web_scene_fx.c'
backend = (root / 'src/PC/gfx_d3d/rb_backend.c').read_text()
# The view begins before RB_Set3D, composes before the first 2D draw and
# always finishes before Present.
begin = backend.index('WebSceneFx_BeginView(vp->viewport.X')
assert begin < backend.index('RB_Set3D();', begin)
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

emscripten = '#define EM_JS(ret, name, params, ...) ret name params;\n'
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
static dvar_t glow, radius0, radius1, intensity0, intensity1, cutoff, desaturation, fullbright, samples, dof, film;
const dvar_t *r_glow = &glow;
const dvar_t *r_glowRadius[2] = {&radius0, &radius1};
const dvar_t *r_glowBloomIntensity[2] = {&intensity0, &intensity1};
const dvar_t *r_glowBloomCutoff = &cutoff;
const dvar_t *r_glowBloomDesaturation = &desaturation;
const dvar_t *r_fullbright = &fullbright;
const dvar_t *r_aaSamples = &samples;
const dvar_t *r_dof_enable = &dof;
const dvar_t *r_filmTweakEnable = &film;
static int binds, composes, bindResult = 1, boundWidth, boundHeight, boundSamples;
static float composed[SCENE_FX_PARAM_COUNT];
int WebSceneFx_Bind(int width, int height, int sampleCount)
{
    ++binds; boundWidth = width; boundHeight = height; boundSamples = sampleCount;
    return bindResult;
}
void WebSceneFx_Compose(const float *params)
{
    ++composes;
    memcpy(composed, params, sizeof(composed));
}
static void defaults(void)
{
    glow.current.enabled = 1; radius0.current.value = 5; radius1.current.value = 12;
    intensity0.current.value = 1; intensity1.current.value = 1; cutoff.current.value = 0.9f;
    desaturation.current.value = 0.75f; fullbright.current.enabled = 0; samples.current.integer = 4;
    dof.current.enabled = 1; film.current.enabled = 1;
}
static void frame(int x, int width, float nearClip)
{
    WebSceneFx_BeginView(x, 0, width, 720, 1280, 720, nearClip);
    WebSceneFx_EndView();
    WebSceneFx_EndFrame();
}
int main(void)
{
    defaults();
    for (int i = 0; i < 600; ++i) {
        binds = composes = 0;
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f);
        assert(binds == 1 && boundWidth == 1280 && boundHeight == 720 && boundSamples == 4 && !composes);
        /* RB_Set2D composes once; later 2D batches and views draw directly. */
        WebSceneFx_EndView(); WebSceneFx_EndView();
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f);
        WebSceneFx_EndView();
        WebSceneFx_EndFrame();
        assert(binds == 1 && composes == 1);
    }
    assert(fabsf(composed[SCENE_FX_GLOW0] - 0.42f) < 1e-6f && fabsf(composed[SCENE_FX_GLOW1] - 0.30f) < 1e-6f);
    assert(composed[SCENE_FX_CUTOFF] == 0.9f && composed[SCENE_FX_DESATURATION] == 0.75f);
    assert(composed[SCENE_FX_RADIUS0] == 5 && composed[SCENE_FX_RADIUS1] == 12);
    assert(composed[SCENE_FX_ADS] == 0 && composed[SCENE_FX_NEAR_CLIP] == 0.1f && composed[SCENE_FX_NEAR_BLUR] == 6);
    assert(composed[SCENE_FX_SHARPEN] > 0 && composed[SCENE_FX_FILM] == 1);

    /* A view left open without a 2D pass still composes before Present. */
    binds = composes = 0;
    WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f);
    WebSceneFx_EndFrame();
    assert(binds == 1 && composes == 1);
    /* A second full-screen view composes the first before drawing directly. */
    binds = composes = 0;
    WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f);
    WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f);
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
    WebSceneFx_SetAds(0, 6, 15);
    frame(0, 1280, 0.1f);
    assert(!binds && !composes);
    WebSceneFx_SetAds(0.5f, 6, 15);
    frame(0, 1280, 0.1f);
    assert(binds == 1 && composes == 1 && boundSamples == 1);
    assert(composed[SCENE_FX_GLOW0] == 0 && composed[SCENE_FX_GLOW1] == 0 && composed[SCENE_FX_SHARPEN] == 0);
    assert(composed[SCENE_FX_FILM] == 0 && composed[SCENE_FX_CONTRAST] == 0 && composed[SCENE_FX_ADS] == 0.5f);
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

    /* Viewmodel focus ranges are clamped to a valid, non-empty interval. */
    WebSceneFx_SetAds(NAN, NAN, NAN);
    frame(0, 1280, 0.1f);
    assert(composed[SCENE_FX_ADS] == 0 && composed[SCENE_FX_VM_END] == 2 && composed[SCENE_FX_VM_START] == 0);
    WebSceneFx_SetAds(4, 30, 12);
    frame(0, 1280, 0.1f);
    assert(composed[SCENE_FX_ADS] == 1 && composed[SCENE_FX_VM_END] == 12 && composed[SCENE_FX_VM_START] == 11.5f);
    WebSceneFx_SetAds(0.25f, 5, 500);
    frame(0, 1280, 0.1f);
    assert(composed[SCENE_FX_ADS] == 0.25f && composed[SCENE_FX_VM_END] == 64 && composed[SCENE_FX_VM_START] == 5);

    /* A failed offscreen target never composes stale state. */
    bindResult = 0; composes = 0;
    for (int i = 0; i < 10; ++i)
        frame(0, 1280, 0.1f);
    assert(!composes);
    bindResult = 1;
    frame(0, 1280, 0.1f);
    assert(composes == 1);
    puts("PASS: 600 composed frames, single compose per frame before Present, partial/fullbright/disabled views drawn directly, bounded glow and focus parameters");
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix='cod2-scene-fx-') as tmp:
    path = Path(tmp)
    (path / 'emscripten.h').write_text(emscripten)
    (path / 'common_types.h').write_text(common_types)
    (path / 'test.c').write_text(test.replace('SOURCE', str(source)))
    subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Werror', '-I', str(path),
                    '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
