#include "common_types.h"
#include "web_effects.h"
#include <math.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/* What the browser's DX7 techniques leave out of a draw, for the generated
 * GL shaders (scripts/patch-emscripten-gl.py):
 *  - r_aaAlpha: an alpha-tested surface (grass, leaves, hedges, wire) resolves
 *    its edge into the scene's multisample coverage, as the DX9 renderer's
 *    alpha antialiasing did;
 *  - r_zfeather: an effect whose material carries featherParms (fire, smoke,
 *    dust) fades into the surfaces it meets and before the eye, as the DX9
 *    zfeather techniques did from the float Z buffer;
 *  - r_fxHeat: fire and explosions burn hotter, their cores reaching the glow;
 *  - r_wind: grass, bushes, hedges and trees sway, their tops further than
 *    their feet, in gusts that cross the map;
 *  - a scope's glass held in the hands mirrors the world around it instead
 *    of the picture painted on it. */

extern const dvar_t *r_aaAlpha;
extern const dvar_t *r_zfeather;
extern const dvar_t *r_fxHeat;
extern const dvar_t *r_wind;

#define WEB_D3DCMP_GREATER 5
#define WEB_D3DCMP_GREATEREQUAL 7
#define WEB_D3DBLEND_ONE 2
#define WEB_D3DBLEND_SRCALPHA 5
#define WEB_D3DBLEND_INVSRCALPHA 6

/* The emissive gain of fire at r_fxHeat 1: a core at 0.8 of full brightness
 * gains half again, a dim edge at 0.2 an eighth. */
#define WEB_EFFECTS_HEAT 0.65f
#define WEB_EFFECTS_COVERAGE_MIN_REF 0.25f
#define WEB_EFFECTS_COVERAGE_MAX_REF 0.75f
/* How far a full gust pushes a model's top at r_wind 1, in units, before its
 * material's share; the generated shader eases it in with height, half of it
 * 50 units up. */
#define WEB_EFFECTS_WIND_PUSH 14.0f
/* The wind's clock wraps every 600 s, a whole number of each sway's periods. */
#define WEB_EFFECTS_WIND_PERIOD_MS 600000

float WebEffects_Feather(const Material *material)
{
    if (!material || !material->constants)
        return 0.0f;
    for (int i = 0; i < material->constantCount; ++i) {
        const MaterialConstantDef *constant = &material->constants[i];
        float scale;
        if (!constant->name || strcmp(constant->name, "featherParms"))
            continue;
        scale = constant->literal[0];
        return isfinite(scale) && scale > 0.0f ? scale : 0.0f;
    }
    return 0.0f;
}

/* gfx_fire_atlas, gfx_exp_fireball_atlas, gfx_explosion_flash and the like;
 * muzzle flashes, sparks, tracers and glints keep their light. */
int WebEffects_IsFire(const Material *material)
{
    const char *name = material ? material->info.name : NULL;

    return name && (strstr(name, "fire") || strstr(name, "flame") || strstr(name, "explosion"));
}

/* How far the wind sways a model drawn with the material: grass, flowers and
 * weeds all the way, trees' leaves, branches and trunks less, bushes half and
 * hedges least; nothing else. Models' materials are named mtl_; the map's own
 * grass and wood (duhoc_grass_green1, wood@pine1) and steel hedgehogs are not
 * foliage. */
float WebEffects_Foliage(const Material *material)
{
    static const struct {
        const char *word;
        float share;
    } kinds[] = {{"hedge", 0.3f}, {"grass", 1.0f}, {"flower", 1.0f}, {"weed", 1.0f}, {"bush", 0.5f},
                 {"shrub", 0.5f}, {"brush", 0.5f}, {"ivy", 0.5f}, {"palm", 0.5f}, {"frond", 0.5f},
                 {"oak", 0.6f}, {"birch", 0.6f}, {"pine", 0.6f}, {"tree", 0.6f}, {"branch", 0.6f},
                 {"trunk", 0.6f}, {"bark", 0.6f}, {"leaves", 0.6f}, {"leafy", 0.6f}, {"foliage", 0.6f}};
    const char *name = material ? material->info.name : NULL;
    size_t i;

    if (!name || (strncmp(name, "mtl_", 4) && !strstr(name, "foliage")) || strstr(name, "hedgehog"))
        return 0.0f;
    for (i = 0; i < sizeof(kinds) / sizeof(kinds[0]); ++i)
        if (strstr(name, kinds[i].word))
            return kinds[i].share;
    return 0.0f;
}

/* A model's sway: the wind's push from the west-southwest turned into the
 * model's own space, so its top moves as far in the world whatever the
 * model's turn and scale. The identity draws the map, which keeps still. */
static void WebEffects_Wind(const float *world, int time, float share, float *wind)
{
    static const float direction[3] = {0.9403f, 0.3402f, 0.0f};
    float push[3], offIdentity = 0.0f, scale;
    int i, j;

    for (i = 0; i < 16; ++i)
        offIdentity += fabsf(world[i] - (i % 5 == 0 ? 1.0f : 0.0f));
    scale = sqrtf(world[8] * world[8] + world[9] * world[9] + world[10] * world[10]);
    if (!(offIdentity > 1e-4f) || !isfinite(offIdentity) || !(scale > 1e-4f) || !isfinite(scale))
        return;
    for (i = 0; i < 3; ++i) {
        const float *axis = world + i * 4;
        float length2 = 0.0f, along = 0.0f;
        for (j = 0; j < 3; ++j) {
            length2 += axis[j] * axis[j];
            along += axis[j] * direction[j];
        }
        if (!(length2 > 1e-8f))
            return;
        push[i] = WEB_EFFECTS_WIND_PUSH * share * along / length2;
    }
    for (i = 0; i < 3; ++i)
        wind[i] = push[i];
    wind[3] = 1.0f;
    wind[4] = world[12];
    wind[5] = world[13];
    wind[6] = scale;
    wind[7] = (float)((time % WEB_EFFECTS_WIND_PERIOD_MS + WEB_EFFECTS_WIND_PERIOD_MS) % WEB_EFFECTS_WIND_PERIOD_MS) * 0.001f;
}

/* Where each scope's glass lies in its color map, the disc of its painted
 * reflection: centre and radii, v from the top. The 4x Optic wears one of
 * these three; the Mosin-Nagant's glass is plain dark metal. */
static const struct {
    const char *name;
    float disc[4];
} s_webLenses[] = {{"mtl_weapon_kar98_scope", {0.1719f, 0.7070f, 0.1426f, 0.2852f}},
                   {"mtl_weapon_enfield_scope", {0.1602f, 0.6875f, 0.1504f, 0.3008f}},
                   {"mtl_weapon_springfield", {0.6289f, 0.2051f, 0.1289f, 0.1289f}}};

const float *WebEffects_Lens(const Material *material)
{
    const char *name = material ? material->info.name : NULL;
    size_t i;

    for (i = 0; name && i < sizeof(s_webLenses) / sizeof(s_webLenses[0]); ++i)
        if (!strcmp(name, s_webLenses[i].name))
            return s_webLenses[i].disc;
    return NULL;
}

void WebEffects_Select(const Material *material, const WebEffectState *state, WebEffectDraw *draw)
{
    memset(draw, 0, sizeof(*draw));
    if (!state || state->is2D)
        return;
    /* The coverage ramps about the reference, so it takes a test that
     * keeps the upper part of the alpha's range (GE128) and not GT0, whose
     * clear texels would turn half covered. */
    if (state->alphaTest && !state->alphaBlend && state->depthWrite &&
        (state->alphaFunc == WEB_D3DCMP_GREATER || state->alphaFunc == WEB_D3DCMP_GREATEREQUAL) &&
        state->alphaRef >= WEB_EFFECTS_COVERAGE_MIN_REF && state->alphaRef <= WEB_EFFECTS_COVERAGE_MAX_REF &&
        r_aaAlpha && r_aaAlpha->current.integer != 0)
        draw->coverage = 1.0f;
    /* Only the viewmodel's, in the depth hack's range: a scope in the hands. */
    if (!state->fullDepthRange) {
        const float *disc = WebEffects_Lens(material);
        if (disc)
            memcpy(draw->lens, disc, sizeof(draw->lens));
    }
    if (state->world && state->fullDepthRange && r_wind) {
        float share = WebEffects_Foliage(material) * r_wind->current.value;
        if (share > 0.0f && isfinite(share))
            WebEffects_Wind(state->world, state->time, fminf(share, 2.0f), draw->wind);
    }
    if (!state->alphaBlend || state->depthWrite)
        return;
    /* The fade scales what the blend adds: the alpha where the source is
     * weighted by it, otherwise the color itself. */
    if (state->fullDepthRange && r_zfeather && r_zfeather->current.enabled) {
        float feather = WebEffects_Feather(material);
        if (feather > 0.0f) {
            if (state->srcBlend == WEB_D3DBLEND_SRCALPHA)
                draw->feather = feather;
            else if (state->srcBlend == WEB_D3DBLEND_ONE &&
                     (state->dstBlend == WEB_D3DBLEND_ONE || state->dstBlend == WEB_D3DBLEND_INVSRCALPHA))
                draw->feather = -feather;
        }
    }
    if (state->dstBlend == WEB_D3DBLEND_ONE && r_fxHeat && WebEffects_IsFire(material)) {
        float heat = r_fxHeat->current.value;
        draw->heat = isfinite(heat) && heat > 0.0f ? fminf(heat, 2.0f) * WEB_EFFECTS_HEAT : 0.0f;
    }
}

#ifdef __EMSCRIPTEN__
/* The draw's effect uniforms, after WebPointLights_Draw made the material's.
 * Coverage needs the composed view's multisampled target and feathering its
 * depth, copied once a view before the first feathered draw (web_scene_fx.c);
 * the depth's linear distance takes the view's near clip times the infinite
 * projection's depth scale, as the compositor's does. */
EM_JS(void, WebEffects_Draw, (float coverage, float feather, float heat, float worldNearClip, const float *wind,
                              const float *lens), {
    var material = Module.cod2Material, fx = Module.cod2SceneFx;
    if (!material || !material.effect) return;
    if (material.lens) {
        // The glass mirrors the world drawn before it, copied once a view.
        var disc = lens >> 2, glass = HEAPF32[disc + 2] > 0 && !!(fx && fx.t && fx.drawing && fx.lensScene());
        for (var k = 0; k < 4; ++k) material.lens[k] = glass ? HEAPF32[disc + k] : 0;
    }
    if (material.wind) {
        var at = wind >> 2;
        for (var i = 0; i < 4; ++i) {
            material.wind[i] = HEAPF32[at + i];
            material.windAt[i] = HEAPF32[at + 4 + i];
        }
    }
    var drawing = !!(fx && fx.t && fx.drawing);
    var cover = coverage > 0 && drawing && fx.samples > 1 ? 1 : 0;
    if (feather !== 0 && !(worldNearClip > 0 && drawing && fx.featherDepth())) feather = 0;
    var effect = material.effect;
    effect[0] = cover;
    effect[1] = feather;
    effect[2] = heat;
    effect[3] = feather !== 0 ? worldNearClip : 0;
    if (material.coverage !== cover) {
        if (cover) GLctx.enable(GLctx.SAMPLE_ALPHA_TO_COVERAGE);
        else GLctx.disable(GLctx.SAMPLE_ALPHA_TO_COVERAGE);
        material.coverage = cover;
    }
});
#endif

void WebEffects_Apply(const Material *material, const WebEffectState *state, float worldNearClip)
{
    WebEffectDraw draw;

    WebEffects_Select(material, state, &draw);
    WebEffects_Draw(draw.coverage, draw.feather, draw.heat, worldNearClip, draw.wind, draw.lens);
}
