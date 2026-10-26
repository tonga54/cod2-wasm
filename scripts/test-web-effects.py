#!/usr/bin/env python3
"""Exercise the production per-draw effects: coverage, z-feather, heat and wind."""
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
             'effectState.fullDepthRange = dev->viewportMinZ == 0.0f && dev->viewportMaxZ == 1.0f;',
             'effectState.world = dev->worldMatrix;',
             'effectState.time = WebSceneFx_SceneTime();'):
    at = device.index(line)
    assert device.rindex('WebEffectState effectState;', 0, hook) < at < hook, line
assert 'extern float WebSceneFx_WorldNearClip(void);' in device and '#include "web_effects.h"' in device
assert 'extern int WebSceneFx_SceneTime(void);' in device
assert 'int WebSceneFx_SceneTime(void)\n{\n    return s_cameraTime;\n}' in (root / 'downstream/wasm/web_scene_fx.c').read_text()
assert '"${CMAKE_CURRENT_LIST_DIR}/web_effects.c"' in (root / 'downstream/wasm/CMakeLists.txt').read_text()
dvars = (root / 'src/PC/gfx_d3d/r_dvars.c').read_text()
for registration in ('r_zfeather = ri.Dvar_RegisterBool("r_zfeather", 1, 0x2001);',
                     'r_fxHeat = ri.Dvar_RegisterFloat("r_fxHeat", 1.0f, 0.0f, 2.0f, 0x2001);',
                     'r_wind = ri.Dvar_RegisterFloat("r_wind", 1.0f, 0.0f, 2.0f, 0x2001);'):
    at = dvars.index(registration)
    assert dvars.rindex('#ifdef __EMSCRIPTEN__', 0, at) > dvars.rindex('#endif', 0, at)
# The original graphics menu's Soften Smoke Edges sets r_zfeather.
assert '{ "ui_zfeather", "Soften the edges where smoke meets the world." },' in \
    (root / 'src/PC/ui_mp/ui_wwii_settings.c').read_text()
# WebPointLights_Draw makes the material's effect uniform before this runs.
assert 'effect: new Float32Array(4),' in (root / 'downstream/wasm/web_point_lights.c').read_text()
assert 'wind: new Float32Array(4), windAt: new Float32Array(4),' in (root / 'downstream/wasm/web_point_lights.c').read_text()
# Every material program declares the wind and sways its vertices in the
# model's space before the view; the two vectors upload with the others.
gl_patch = (root / 'scripts/patch-emscripten-gl.py').read_text()
for text in ('"uniform highp vec4 u_cod2Wind; uniform highp vec4 u_cod2WindAt;"',
             "WIND_POSITION_ORIGINAL = '\"  vec4 ecPosition = u_modelView * a_position;\"'",
             '"  vec4 ecPosition = u_modelView * cod2Position;"',
             'cod2Position.xyz += u_cod2Wind.xyz * (cod2Height / (cod2Height + 50.0) * cod2Gust);',
             '(WIND_POSITION_ORIGINAL, WIND_POSITION_REPLACEMENT),',
             'this.cod2WindLocation = GLctx.getUniformLocation(this.program, "u_cod2Wind");',
             '"effect", "relief", "wind", "windAt"];'):
    assert gl_patch.count(text) == 1, text

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
static dvar_t aaAlpha, zfeather, fxHeat, windDvar;
const dvar_t *r_aaAlpha = &aaAlpha;
const dvar_t *r_zfeather = &zfeather;
const dvar_t *r_fxHeat = &fxHeat;
const dvar_t *r_wind = &windDvar;
static float drawn[4], drawnWind[8];
static int draws;
void WebEffects_Draw(float coverage, float feather, float heat, float worldNearClip, const float *wind)
{
    drawn[0] = coverage; drawn[1] = feather; drawn[2] = heat; drawn[3] = worldNearClip;
    memcpy(drawnWind, wind, sizeof(drawnWind));
    ++draws;
}
/* A model's D3D world matrix: turned yaw radians about the vertical,
 * scaled and standing at (x, y, z). */
static void placed(float *m, float yaw, float scale, float x, float y, float z)
{
    memset(m, 0, 16 * sizeof(float));
    m[0] = cosf(yaw) * scale; m[1] = sinf(yaw) * scale;
    m[4] = -sinf(yaw) * scale; m[5] = cosf(yaw) * scale;
    m[10] = scale;
    m[12] = x; m[13] = y; m[14] = z; m[15] = 1;
}
/* The world push the model's own push makes through its axes. */
static void worldPush(const float *m, const float *wind, float *out)
{
    for (int j = 0; j < 3; ++j)
        out[j] = wind[0] * m[j] + wind[1] * m[4 + j] + wind[2] * m[8 + j];
}
static WebEffectDraw draw;
static void pick(const Material *material, const WebEffectState *state)
{
    memset(&draw, 0xa5, sizeof(draw));
    WebEffects_Select(material, state, &draw);
}
static int none(void) { return draw.coverage == 0 && draw.feather == 0 && draw.heat == 0; }
static int still(void)
{
    for (int i = 0; i < 8; ++i)
        if (draw.wind[i] != 0.0f)
            return 0;
    return 1;
}
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
    aaAlpha.current.integer = 1; zfeather.current.enabled = 1; fxHeat.current.value = 1; windDvar.current.value = 1;

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

    /* Wind: models' grass, flowers and weeds sway all the way, trees' leaves,
     * branches and trunks 0.6, bushes and palms half, hedges 0.3; the map's
     * grass and wood, steel hedgehogs and anything else not at all. */
    const struct { const char *name; float share; } foliage[] = {
        {"mtl_newgrassgreen", 1}, {"mtl_grassflowerplants", 1}, {"mtl_caen_grass_weed_all", 1},
        {"foliage_detail@grassblades#0.dds", 1}, {"mtl_grey_oak", 0.6f}, {"mtl_grey_oak_canopy", 0.6f},
        {"mtl_river_birch_canopy", 0.6f}, {"mtl_pine", 0.6f}, {"mtl_thicktrunkbark_01", 0.6f},
        {"mtl_firhighbranch_vertical", 0.6f}, {"foliage_masked@oak1cross#0.tga", 0.6f},
        {"mtl_bushes_normandy", 0.5f}, {"mtl_desertpalmfronds", 0.5f}, {"mtl_hedgewall_all", 0.3f},
        {"duhoc_foliage_green_hedge1", 0.3f}, {"duhoc_grass_green1", 0}, {"wood@pine1.tga", 0},
        {"mtl_hedgehog", 0}, {"mtl_detail_hedgehog", 0}, {"mtl_crate", 0}, {"toujane_canopy1", 0}, {"", 0}};
    for (unsigned i = 0; i < sizeof(foliage) / sizeof(foliage[0]); ++i) {
        Material m = {{foliage[i].name}, 0, NULL};
        assert(WebEffects_Foliage(&m) == foliage[i].share);
    }
    assert(!WebEffects_Foliage(&unnamed) && !WebEffects_Foliage(NULL));
    /* A grass clump turned and scaled: its own push turns back into 14 units
     * from the west-southwest in the world; it stands where its origin is,
     * with its scale, on the wind's clock in seconds. */
    {
        Material grass = {{"mtl_newgrassgreen"}, 0, NULL}, oak = {{"mtl_grey_oak_canopy"}, 0, NULL};
        Material crate = {{"mtl_crate"}, 0, NULL};
        float world[16], push[3];
        placed(world, 1.1f, 1.5f, 2800, 400, -77);
        WebEffectState model = {0, 1, GEQUAL, 0.5f, 0, 1, ONE, ZERO, 1, world, 123456};
        pick(&grass, &model);
        worldPush(world, draw.wind, push);
        assert(near(push[0] / 14.0f, 0.9403f) && near(push[1] / 14.0f, 0.3402f) && fabsf(push[2]) < 1e-5f);
        assert(draw.wind[3] == 1 && draw.wind[4] == 2800 && draw.wind[5] == 400 && near(draw.wind[6], 1.5f));
        assert(fabsf(draw.wind[7] - 123.456f) < 1e-3f && draw.coverage == 1);
        /* A tree's crown sways 0.6 of it; r_wind scales it, up to twice. */
        pick(&oak, &model);
        worldPush(world, draw.wind, push);
        assert(fabsf(push[0] - 0.6f * 14.0f * 0.9403f) < 1e-4f);
        windDvar.current.value = 2;
        pick(&oak, &model);
        worldPush(world, draw.wind, push);
        assert(fabsf(push[0] - 1.2f * 14.0f * 0.9403f) < 1e-4f);
        const float calm[] = {0, -1, NAN, INFINITY};
        for (int i = 0; i < 4; ++i) {
            windDvar.current.value = calm[i];
            pick(&grass, &model);
            assert(still());
        }
        windDvar.current.value = 1;
        /* The clock wraps every 600 s, before the game's start too. */
        model.time = 600000 + 1500;
        pick(&grass, &model);
        assert(fabsf(draw.wind[7] - 1.5f) < 1e-3f);
        model.time = -500;
        pick(&grass, &model);
        assert(fabsf(draw.wind[7] - 599.5f) < 1e-3f);
        model.time = 123456;
        /* The map itself (the identity), no matrix, a flattened one, the
         * viewmodel's depth range, a 2D draw or another material keep still. */
        WebEffectState variant = model;
        float identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}, flat[16];
        variant.world = identity; pick(&grass, &variant); assert(still());
        variant = model; variant.world = NULL; pick(&grass, &variant); assert(still());
        placed(flat, 0.3f, 1, 10, 20, 30); flat[10] = 0;
        variant = model; variant.world = flat; pick(&grass, &variant); assert(still());
        placed(flat, 0.3f, 1, 10, 20, 30); flat[0] = flat[1] = flat[2] = 0;
        variant = model; variant.world = flat; pick(&grass, &variant); assert(still());
        placed(flat, 0.3f, 1, 10, 20, 30); flat[12] = NAN;
        variant = model; variant.world = flat; pick(&grass, &variant); assert(still());
        variant = model; variant.fullDepthRange = 0; pick(&grass, &variant); assert(still());
        variant = model; variant.is2D = 1; pick(&grass, &variant); assert(still());
        pick(&crate, &model); assert(still());
        /* An opaque trunk sways with its crown, as an alpha-tested crown does. */
        variant = model; variant.alphaTest = 0;
        pick(&oak, &variant);
        assert(draw.wind[3] == 1 && draw.coverage == 0);
        /* The device's call hands the wind over. */
        draws = 0;
        WebEffects_Apply(&grass, &model, 3.998f);
        assert(draws == 1 && drawnWind[3] == 1 && drawnWind[4] == 2800 && fabsf(drawnWind[7] - 123.456f) < 1e-3f);
        WebEffects_Apply(&crate, &model, 3.998f);
        assert(draws == 2 && drawnWind[3] == 0);
    }

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
         "depth range under r_zfeather, heat for additive fire scaled by r_fxHeat up to twice, models' grass, bushes, "
         "hedges and trees swaying under r_wind by their material, their push turned into their own space, where they "
         "stand and the wind's 600 s clock, the map, the viewmodel and other materials still, nothing for 2D draws");
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
    ('    WebEffects_Draw(draw.coverage, draw.feather, draw.heat, worldNearClip, draw.wind);',
     '    WebEffects_Draw(draw.coverage, draw.feather, draw.heat, 4.0f, draw.wind);'),
    # Wind: each kind's share, the steel hedgehogs, the map's own materials,
    # the map's identity, the push's turn into the model, where it stands,
    # its scale and clock, the viewmodel and r_wind.
    ('{"hedge", 0.3f}', '{"hedge", 0.6f}'),
    ('{"grass", 1.0f}', '{"grass", 0.5f}'),
    ('{"bush", 0.5f}', '{"bush", 1.0f}'),
    ('{"oak", 0.6f}', '{"oak", 1.0f}'),
    (' || strstr(name, "hedgehog"))', ')'),
    ('(strncmp(name, "mtl_", 4) && !strstr(name, "foliage"))', '0'),
    ('    if (!(offIdentity > 1e-4f) || !isfinite(offIdentity) ||', '    if (!isfinite(offIdentity) ||'),
    ('push[i] = WEB_EFFECTS_WIND_PUSH * share * along / length2;', 'push[i] = WEB_EFFECTS_WIND_PUSH * share * along;'),
    ('    wind[4] = world[12];\n', '    wind[4] = world[13];\n'),
    ('    wind[6] = scale;\n', '    wind[6] = 1.0f;\n'),
    ('(time % WEB_EFFECTS_WIND_PERIOD_MS + WEB_EFFECTS_WIND_PERIOD_MS) % WEB_EFFECTS_WIND_PERIOD_MS', 'time % WEB_EFFECTS_WIND_PERIOD_MS'),
    ('    if (state->world && state->fullDepthRange && r_wind) {', '    if (state->world && r_wind) {'),
    ('float share = WebEffects_Foliage(material) * r_wind->current.value;', 'float share = WebEffects_Foliage(material);'),
    ('if (share > 0.0f && isfinite(share))', 'if (share > 0.0f)'),
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
    print(f'PASS: {len(mutants)} coverage, feather, heat and wind mutants fail')
