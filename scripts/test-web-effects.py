#!/usr/bin/env python3
"""Exercise the production per-draw effects: coverage, z-feather and heat."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = root / 'downstream/wasm/web_effects.c'

# The device hands every draw's state over after the material's maps and
# lights, before the draw itself; 2D draws and cube-mapped ones take none.
device = (root / 'src/Mac/DirectX_9/CDirect3DDevice.c').read_text()
hook = device.index('WebEffects_Apply(((materialCommands_t *)imp_tess)->material, &effectState, WebSceneFx_WorldNearClip());')
assert device.rindex('WebPointLights_Draw(pointLightViewValid', 0, hook) < hook < device.index('glDrawRangeElements(0x0004', hook)
for line in ('effectState.is2D = is2D || stage0Target != 0x0DE1;',
             'effectState.alphaTest = g_alphaTestEnable != 0;',
             'effectState.alphaFunc = dev->alphaFuncVal;',
             'effectState.alphaRef = dev->alphaRef;',
             'effectState.alphaBlend = dev->alphaBlendEnable;',
             'effectState.depthWrite = dev->zWriteEnable != 0;',
             'effectState.srcBlend = dev->srcBlend;',
             'effectState.dstBlend = dev->destBlend;',
             'effectState.fullDepthRange = dev->viewportMinZ == 0.0f && dev->viewportMaxZ == 1.0f;'):
    at = device.index(line)
    assert device.rindex('WebEffectState effectState;', 0, hook) < at < hook, line
assert 'extern float WebSceneFx_WorldNearClip(void);' in device and '#include "web_effects.h"' in device
assert '"${CMAKE_CURRENT_LIST_DIR}/web_effects.c"' in (root / 'downstream/wasm/CMakeLists.txt').read_text()
dvars = (root / 'src/PC/gfx_d3d/r_dvars.c').read_text()
for registration in ('r_zfeather = ri.Dvar_RegisterBool("r_zfeather", 1, 0x2001);',
                     'r_fxHeat = ri.Dvar_RegisterFloat("r_fxHeat", 1.0f, 0.0f, 2.0f, 0x2001);'):
    at = dvars.index(registration)
    assert dvars.rindex('#ifdef __EMSCRIPTEN__', 0, at) > dvars.rindex('#endif', 0, at)
# The original graphics menu's Soften Smoke Edges sets r_zfeather.
assert '{ "ui_zfeather", "Soften the edges where smoke meets the world." },' in \
    (root / 'src/PC/ui_mp/ui_wwii_settings.c').read_text()
# WebPointLights_Draw makes the material's effect uniform before this runs.
assert 'effect: new Float32Array(4),' in (root / 'downstream/wasm/web_point_lights.c').read_text()

emscripten = '#define EM_JS(ret, name, params, ...) ret name params;\n'
common_types = r'''
typedef float vec4_t[4];
typedef union {int enabled; int integer; float value;} DvarValue;
typedef struct {DvarValue current;} dvar_t;
typedef struct {const char *name;} MaterialInfo;
typedef struct {const char *name; vec4_t literal;} MaterialConstantDef;
typedef struct {
    MaterialInfo info;
    unsigned short constantCount;
    MaterialConstantDef *constants;
} Material;
'''
test = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "SOURCE"
static dvar_t aaAlpha, zfeather, fxHeat;
const dvar_t *r_aaAlpha = &aaAlpha;
const dvar_t *r_zfeather = &zfeather;
const dvar_t *r_fxHeat = &fxHeat;
static float drawn[4];
static int draws;
void WebEffects_Draw(float coverage, float feather, float heat, float worldNearClip)
{
    drawn[0] = coverage; drawn[1] = feather; drawn[2] = heat; drawn[3] = worldNearClip;
    ++draws;
}
static WebEffectDraw draw;
static void pick(const Material *material, const WebEffectState *state)
{
    memset(&draw, 0xa5, sizeof(draw));
    WebEffects_Select(material, state, &draw);
}
static int none(void) { return draw.coverage == 0 && draw.feather == 0 && draw.heat == 0; }
static int near(float a, float b) { return fabsf(a - b) < 1e-6f; }
/* D3DCMP_LESS, GREATER and GREATEREQUAL; D3DBLEND_ZERO, ONE, SRCALPHA,
 * INVSRCALPHA and DESTCOLOR. */
enum {LESS = 2, GREATER = 5, GEQUAL = 7, ZERO = 1, ONE = 2, SRCALPHA = 5, INVSRCALPHA = 6, DESTCOLOR = 9};
int main(void)
{
    MaterialConstantDef fireConstants[] = {{"colorTint", {1, 1, 1, 1}}, {"featherParms", {0.25f, 4, 0, 0}}};
    MaterialConstantDef smokeConstants[] = {{"featherParms", {0.025f, 40, 0, 0}}};
    Material fire = {{"gfx_fire_atlas"}, 2, fireConstants};
    Material smoke = {{"gfx_smokepuff_atlas"}, 1, smokeConstants};
    Material fireball = {{"gfx_exp_fireball_atlas"}, 0, NULL};
    Material leaves = {{"foliage_masked@oak1cross#0.tga"}, 0, NULL};
    aaAlpha.current.integer = 1; zfeather.current.enabled = 1; fxHeat.current.value = 1;

    /* featherParms.x, in any order among the constants; none without a
     * positive, finite one. */
    assert(WebEffects_Feather(&fire) == 0.25f && WebEffects_Feather(&smoke) == 0.025f);
    MaterialConstantDef reversed[] = {fireConstants[1], fireConstants[0]};
    Material reorder = {{"gfx_fire_atlas"}, 2, reversed};
    assert(WebEffects_Feather(&reorder) == 0.25f);
    assert(!WebEffects_Feather(&fireball) && !WebEffects_Feather(NULL));
    Material tintOnly = {{"gfx_fire_atlas"}, 1, fireConstants};
    assert(!WebEffects_Feather(&tintOnly));
    const float invalid[] = {0, -0.25f, NAN, INFINITY, -INFINITY};
    for (int i = 0; i < 5; ++i) {
        smokeConstants[0].literal[0] = invalid[i];
        assert(!WebEffects_Feather(&smoke));
    }
    smokeConstants[0].literal[0] = 0.025f;
    fireConstants[0].name = NULL;
    assert(WebEffects_Feather(&fire) == 0.25f);
    fireConstants[0].name = "colorTint";

    /* Fire and explosions burn hotter; muzzle flashes, sparks, smoke, the
     * tracer and the glint keep their light. */
    const char *hot[] = {"gfx_fire_atlas", "gfx_fire_whisp", "gfx_exp_fireball_atlas", "gfx_exp_fire_smoke_atlas",
                         "gfx_explosion_flash", "gfx_flame_jet"};
    const char *cool[] = {"gfx_muzflash2", "gfx_spark_atlas", "gfx_smokepuff_atlas", "gfx_starburst_1p",
                          "gfx/misc/tracer", "gfx_thompson_stage1", ""};
    for (int i = 0; i < 6; ++i) { Material m = {{hot[i]}, 0, NULL}; assert(WebEffects_IsFire(&m)); }
    for (int i = 0; i < 7; ++i) { Material m = {{cool[i]}, 0, NULL}; assert(!WebEffects_IsFire(&m)); }
    Material unnamed = {{NULL}, 0, NULL};
    assert(!WebEffects_IsFire(&unnamed) && !WebEffects_IsFire(NULL));

    /* Coverage: an opaque, depth-writing alpha test that keeps the upper part
     * of the alpha (GE128), with r_aaAlpha. */
    WebEffectState test = {0, 1, GEQUAL, 0.5f, 0, 1, ONE, ZERO, 1};
    pick(&leaves, &test);
    assert(draw.coverage == 1 && draw.feather == 0 && draw.heat == 0);
    test.alphaFunc = GREATER;
    pick(&leaves, &test);
    assert(draw.coverage == 1);
    for (float ref = 0.25f; ref <= 0.75f; ref += 0.125f) {
        test.alphaRef = ref;
        pick(&leaves, &test);
        assert(draw.coverage == 1);
    }
    const float clear[] = {0, 0.2f, 0.8f, 1, NAN};
    for (int i = 0; i < 5; ++i) {
        test.alphaRef = clear[i];
        pick(&leaves, &test);
        assert(none());
    }
    test.alphaRef = 0.5f;
    WebEffectState variant;
    variant = test; variant.alphaFunc = LESS; pick(&leaves, &variant); assert(none());
    variant = test; variant.alphaTest = 0; pick(&leaves, &variant); assert(none());
    variant = test; variant.alphaBlend = 1; pick(&leaves, &variant); assert(none());
    variant = test; variant.depthWrite = 0; pick(&leaves, &variant); assert(none());
    variant = test; variant.is2D = 1; pick(&leaves, &variant); assert(none());
    /* The depth hack's viewmodel keeps it: its alpha tests are its own. */
    variant = test; variant.fullDepthRange = 0; pick(&leaves, &variant); assert(draw.coverage == 1);
    aaAlpha.current.integer = 0;
    pick(&leaves, &test);
    assert(none());
    aaAlpha.current.integer = 2;
    pick(&leaves, &test);
    assert(draw.coverage == 1);
    aaAlpha.current.integer = 1;

    /* The z-feather of an effect without depth writes in the world's depth
     * range: its alpha where the blend weighs the source by it, its color
     * where the blend adds it whole; none for a multiplying blend. */
    WebEffectState fx = {0, 0, 8, 0, 1, 0, SRCALPHA, ONE, 1};
    pick(&fire, &fx);
    assert(draw.feather == 0.25f && draw.coverage == 0 && near(draw.heat, WEB_EFFECTS_HEAT));
    fx.dstBlend = INVSRCALPHA;
    pick(&smoke, &fx);
    assert(draw.feather == 0.025f && draw.heat == 0);
    fx.srcBlend = ONE; fx.dstBlend = ONE;
    pick(&fire, &fx);
    assert(draw.feather == -0.25f && near(draw.heat, WEB_EFFECTS_HEAT));
    fx.dstBlend = INVSRCALPHA;
    pick(&smoke, &fx);
    assert(draw.feather == -0.025f);
    fx.srcBlend = DESTCOLOR; fx.dstBlend = ZERO;
    pick(&smoke, &fx);
    assert(none());
    fx.srcBlend = ZERO; fx.dstBlend = SRCALPHA;
    pick(&smoke, &fx);
    assert(none());
    fx.srcBlend = ONE; fx.dstBlend = ZERO;
    pick(&smoke, &fx);
    assert(none());
    fx.srcBlend = DESTCOLOR; fx.dstBlend = ONE;
    pick(&smoke, &fx);
    assert(none());
    fx.srcBlend = SRCALPHA; fx.dstBlend = ONE;
    variant = fx; variant.depthWrite = 1; pick(&fire, &variant); assert(none());
    variant = fx; variant.alphaBlend = 0; pick(&fire, &variant); assert(none());
    variant = fx; variant.is2D = 1; pick(&fire, &variant); assert(none());
    variant = fx; variant.fullDepthRange = 0; pick(&fire, &variant);
    assert(draw.feather == 0 && near(draw.heat, WEB_EFFECTS_HEAT));
    pick(&fireball, &fx);
    assert(draw.feather == 0 && near(draw.heat, WEB_EFFECTS_HEAT));
    zfeather.current.enabled = 0;
    pick(&fire, &fx);
    assert(draw.feather == 0 && near(draw.heat, WEB_EFFECTS_HEAT));
    zfeather.current.enabled = 1;

    /* Heat only for additive fire, scaled by r_fxHeat up to twice. */
    fx.dstBlend = INVSRCALPHA;
    pick(&fire, &fx);
    assert(draw.feather == 0.25f && draw.heat == 0);
    fx.dstBlend = ONE;
    pick(&smoke, &fx);
    assert(draw.heat == 0);
    const float heats[][2] = {{0, 0}, {0.5f, 0.5f}, {2, 2}, {3, 2}, {-1, 0}, {NAN, 0}, {INFINITY, 0}};
    for (int i = 0; i < 7; ++i) {
        fxHeat.current.value = heats[i][0];
        pick(&fire, &fx);
        assert(near(draw.heat, heats[i][1] * WEB_EFFECTS_HEAT));
    }
    fxHeat.current.value = 1;

    /* No state or no material changes nothing. */
    pick(&fire, NULL);
    assert(none());
    pick(NULL, &fx);
    assert(none());
    pick(NULL, &test);
    assert(draw.coverage == 1);

    /* The device's call hands the choice and the view's near clip over. */
    draws = 0;
    WebEffects_Apply(&fire, &fx, 3.998f);
    assert(draws == 1 && drawn[0] == 0 && drawn[1] == 0.25f && near(drawn[2], WEB_EFFECTS_HEAT) && drawn[3] == 3.998f);
    WebEffects_Apply(&leaves, &test, 3.998f);
    assert(draws == 2 && drawn[0] == 1 && drawn[1] == 0 && drawn[2] == 0);
    WebEffectState flat = {1, 1, GEQUAL, 0.5f, 1, 0, SRCALPHA, ONE, 1};
    WebEffects_Apply(&fire, &flat, 3.998f);
    assert(draws == 3 && drawn[0] == 0 && drawn[1] == 0 && drawn[2] == 0);
    puts("PASS: featherParms in any order and only when positive and finite, fire and explosions only, coverage for "
         "opaque GE128-style alpha tests under r_aaAlpha, the feather of alpha or color by the blend in the world's "
         "depth range under r_zfeather, heat for additive fire scaled by r_fxHeat up to twice, nothing for 2D draws");
    return 0;
}
'''

mutants = [
    ('if (!constant->name || strcmp(constant->name, "featherParms"))', 'if (!constant->name)'),
    ('return isfinite(scale) && scale > 0.0f ? scale : 0.0f;', 'return scale > 0.0f ? scale : 0.0f;'),
    ('return isfinite(scale) && scale > 0.0f ? scale : 0.0f;', 'return isfinite(scale) ? scale : 0.0f;'),
    ('strstr(name, "fire") || strstr(name, "flame") || strstr(name, "explosion")',
     'strstr(name, "fire") || strstr(name, "flame")'),
    ('strstr(name, "fire") || strstr(name, "flame") || strstr(name, "explosion")',
     'strstr(name, "fire") || strstr(name, "flash") || strstr(name, "explosion")'),
    ('    if (!state || state->is2D)\n        return;', '    if (!state)\n        return;'),
    ('state->alphaTest && !state->alphaBlend && state->depthWrite &&', 'state->alphaTest && state->depthWrite &&'),
    ('state->alphaTest && !state->alphaBlend && state->depthWrite &&', 'state->alphaTest && !state->alphaBlend &&'),
    ('(state->alphaFunc == WEB_D3DCMP_GREATER || state->alphaFunc == WEB_D3DCMP_GREATEREQUAL)',
     '(state->alphaFunc == WEB_D3DCMP_GREATEREQUAL)'),
    ('(state->alphaFunc == WEB_D3DCMP_GREATER || state->alphaFunc == WEB_D3DCMP_GREATEREQUAL)', '1'),
    ('state->alphaRef >= WEB_EFFECTS_COVERAGE_MIN_REF && state->alphaRef <= WEB_EFFECTS_COVERAGE_MAX_REF &&', ''),
    ('#define WEB_EFFECTS_COVERAGE_MIN_REF 0.25f', '#define WEB_EFFECTS_COVERAGE_MIN_REF 0.0f'),
    ('#define WEB_EFFECTS_COVERAGE_MAX_REF 0.75f', '#define WEB_EFFECTS_COVERAGE_MAX_REF 1.0f'),
    ('r_aaAlpha && r_aaAlpha->current.integer != 0)', 'r_aaAlpha)'),
    ('    if (!state->alphaBlend || state->depthWrite)\n        return;', '    if (!state->alphaBlend)\n        return;'),
    ('    if (!state->alphaBlend || state->depthWrite)\n        return;', '    if (state->depthWrite)\n        return;'),
    ('if (state->fullDepthRange && r_zfeather && r_zfeather->current.enabled) {', 'if (r_zfeather && r_zfeather->current.enabled) {'),
    ('if (state->fullDepthRange && r_zfeather && r_zfeather->current.enabled) {', 'if (state->fullDepthRange && r_zfeather) {'),
    ('                draw->feather = feather;', '                draw->feather = -feather;'),
    ('                draw->feather = -feather;', '                draw->feather = feather;'),
    ('(state->dstBlend == WEB_D3DBLEND_ONE || state->dstBlend == WEB_D3DBLEND_INVSRCALPHA)', '1'),
    ('            else if (state->srcBlend == WEB_D3DBLEND_ONE &&', '            else if ('),
    ('    if (state->dstBlend == WEB_D3DBLEND_ONE && r_fxHeat && WebEffects_IsFire(material)) {',
     '    if (r_fxHeat && WebEffects_IsFire(material)) {'),
    ('draw->heat = isfinite(heat) && heat > 0.0f ? fminf(heat, 2.0f) * WEB_EFFECTS_HEAT : 0.0f;',
     'draw->heat = heat > 0.0f ? fminf(heat, 2.0f) * WEB_EFFECTS_HEAT : 0.0f;'),
    ('draw->heat = isfinite(heat) && heat > 0.0f ? fminf(heat, 2.0f) * WEB_EFFECTS_HEAT : 0.0f;',
     'draw->heat = isfinite(heat) && heat > 0.0f ? heat * WEB_EFFECTS_HEAT : 0.0f;'),
    ('    memset(draw, 0, sizeof(*draw));\n', ''),
    ('    WebEffects_Draw(draw.coverage, draw.feather, draw.heat, worldNearClip);',
     '    WebEffects_Draw(draw.coverage, draw.feather, draw.heat, 4.0f);'),
]

with tempfile.TemporaryDirectory(prefix='cod2-web-effects-') as tmp:
    path = Path(tmp)
    (path / 'emscripten.h').write_text(emscripten)
    (path / 'common_types.h').write_text(common_types)
    (path / 'test.c').write_text(test.replace('SOURCE', str(source)))
    flags = ['-std=c99', '-D__EMSCRIPTEN__', '-I', str(path), '-I', str(source.parent)]
    subprocess.run(['cc', *flags, '-O1', '-g', '-Wall', '-Werror', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
    original = source.read_text()
    for index, (before, after) in enumerate(mutants):
        assert original.count(before) == 1, before
        mutant = path / f'mutant{index}.c'
        mutant.write_text(original.replace(before, after))
        (path / f'mutant{index}-test.c').write_text(test.replace('SOURCE', str(mutant)))
        subprocess.run(['cc', *flags, '-O0', '-w', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        str(path / f'mutant{index}-test.c'), '-lm', '-o', str(path / f'mutant{index}')], check=True)
        result = subprocess.run([str(path / f'mutant{index}')], capture_output=True)
        assert result.returncode != 0, f'mutant survived: {after!r}'
    print(f'PASS: {len(mutants)} coverage, feather and heat mutants fail')
