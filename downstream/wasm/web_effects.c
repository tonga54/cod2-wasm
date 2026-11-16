#include "common_types.h"
#include "web_effects.h"
#include <math.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include "web_materials.h"
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
 *  - a scope's glass held in the hands mirrors the world around it instead
 *    of the picture painted on it. */

extern const dvar_t *r_aaAlpha;
extern const dvar_t *r_zfeather;
extern const dvar_t *r_fxHeat;

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

/* Where each scope's glass lies in its color map, the disc of its painted
 * reflection: centre and radii, v from the top. The 4x Optic wears one of
 * these three; the Mosin-Nagant's glass is plain dark metal. */
static const struct {
    const char *name;
    float disc[4];
} s_webLenses[] = {{"mtl_weapon_kar98_scope", {0.1581f, 0.6844f, 0.1520f, 0.3040f}},
                   {"mtl_weapon_enfield_scope", {0.1594f, 0.6830f, 0.1568f, 0.3090f}},
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
    /* The blast atlas already stores emitted radiance with black transparent
     * edges. Pure addition of 4–6 overlapping lobes clips all three channels.
     * Give those lobes bounded extinction without changing the brief flash,
     * point light, sparks, ambient flames or smoke-screen coverage. */
    if (state->fullDepthRange && state->srcBlend == WEB_D3DBLEND_ONE &&
        state->dstBlend == WEB_D3DBLEND_ONE && material && material->info.name &&
        !strcmp(material->info.name, "browser_blast_fireball")) {
        draw->extinction = 0.9f;
        draw->heat = 0.0f;
    }
}

#ifdef __EMSCRIPTEN__
EM_JS(void, WebEffects_FireBlend, (float extinction), {
    if (!Module.cod2Material) return;
    Module.cod2Material.fireExtinction = extinction;
    // The device reapplies the material's blend before every draw. Override
    // only this draw, after the effect's depth/scene captures have restored GL.
    if (extinction > 0) GLctx.blendFunc(GLctx.ONE, GLctx.ONE_MINUS_SRC_ALPHA);
});
EM_JS(void, WebEffects_Atmosphere, (int foliage, int smoke, float depth, float wet, float normalScale, int metallic, float masonry), {
    if (Module.cod2Material) {
        Module.cod2Material.model[2] = foliage;
        Module.cod2Material.model[3] = smoke;
        Module.cod2Material.environment[1] = depth;
        Module.cod2Material.environment[2] = wet;
        Module.cod2Material.environment[3] = normalScale;
        Module.cod2Material.finish[0] = metallic;
        Module.cod2Material.masonry[0] = masonry;
    }
});
/* The draw's effect uniforms, after WebPointLights_Draw made the material's.
 * Coverage needs the composed view's multisampled target and feathering its
 * depth, copied once a view before the first feathered draw (web_scene_fx.c);
 * the depth's linear distance takes the view's near clip times the infinite
 * projection's depth scale, as the compositor's does. */
EM_JS(void, WebEffects_Draw, (float coverage, float feather, float heat, float worldNearClip, const float *lens), {
    var material = Module.cod2Material, fx = Module.cod2SceneFx;
    if (!material || !material.effect) return;
    if (material.lens) {
        // The glass mirrors the world drawn before it, copied once a view.
        var disc = lens >> 2, glass = HEAPF32[disc + 2] > 0 && !!(fx && fx.t && fx.drawing && fx.lensScene());
        for (var k = 0; k < 4; ++k) material.lens[k] = glass ? HEAPF32[disc + k] : 0;
    }
    var drawing = !!(fx && fx.t && fx.drawing && !fx.planarDrawing);
    if (material.waterProjection) {
        material.waterProjection[3] = 0;
        if (drawing && material.environment[2] !== 0 && material.skyEye[3] > 0 && fx.waterHistory)
            fx.waterHistory(material);
    }
    if (material.finish) {
        // Capture before the first arm/weapon surface, so neither the gun nor
        // the HUD reflects itself. Scopes share this same once-per-view copy.
        var camera = Module.cod2ReflectionCamera;
        var reflected = material.model[1] > 0.5 && material.skyEye[3] > 0 &&
            camera && camera[0] > 0 && camera[1] > 0 && drawing && fx.lensScene();
        material.finish[1] = reflected ? 1 : 0;
        material.finish[2] = reflected ? camera[0] : 0;
        material.finish[3] = reflected ? camera[1] : 0;
    }
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
#ifdef __EMSCRIPTEN__
    const char *name = material && !state->is2D ? material->info.name : NULL;
    int foliage = name && state->alphaTest && (strstr(name, "canopy") || strstr(name, "canopies") ||
        strstr(name, "grass") || strstr(name, "bush") || strstr(name, "hedge") || strstr(name, "frond") ||
        strstr(name, "foliage") || strstr(name, "green_single_tree"));
    if (foliage && strstr(name, "browser_remix/prop_foliage")) foliage = 2;
    int smoke = name && state->alphaBlend && !state->depthWrite &&
        (strstr(name, "smoke") || strstr(name, "smokepuff"));
    if (smoke && strstr(name, "browser_smoke_screen_")) smoke = 2;
    if (smoke == 2 && material->textures) {
        for (int i = 0; i < material->textureCount; ++i) {
            const MaterialTextureDef *map = &material->textures[i];
            if (map->name && !strcmp(map->name, "colorMap") && map->u.image && map->u.image->name &&
                !strncmp(map->u.image->name, "browser_remix/smoke_volume_", sizeof("browser_remix/smoke_volume_") - 1)) smoke = 3;
        }
    }
    /* World particle effects draw after solid surfaces. Their transparency
     * must blend over the world's AO, not receive the hidden wall's AO. */
    if (name && state->fullDepthRange && state->alphaBlend && !state->depthWrite &&
        (smoke || WebEffects_Feather(material) > 0.0f)) {
        extern void WebSceneFx_BeforeEffects(void);
        WebSceneFx_BeforeEffects();
    }
    float depth = 0.0f, wet = 0.0f, normalScale = 1.0f;
    int metallic = 0;
    /* Only the owner's tiled height maps carry displacement in alpha. Never
     * interpret an original detail map or a model atlas as a height field. */
    for (int i = 0; name && material->textures && i < material->textureCount; ++i) {
        const MaterialTextureDef *def = &material->textures[i];
        if (def->semantic != 2 || !def->name || strcmp(def->name, "detailMap") || !def->u.image) continue;
        const char *image = def->u.image->name;
        if (!image || strncmp(image, "browser_remix/", 14)) continue;
        image += 14;
        /* Explicit owner format: neutral RGB with metalness in alpha. It is
         * valid on viewmodels too, and must never enter height displacement. */
        size_t length = strlen(image);
        if (length > 6 && !strcmp(image + length - 6, "_metal")) {
            /* The donor stock's warm dielectric finish is independent of its
             * steel. Keep this calibration local to that authored atlas. */
            metallic = !strcmp(image, "enfield_1_metal") ? 2 : 1;
            break;
        }
        /* Retail hand UV islands explicitly distinguish fabric, skin, leather
         * and the watch. Negative finish selects classification, not metalness. */
        if (length > 10 && !strcmp(image + length - 10, "_character")) { metallic = -1; break; }
        if (!state->fullDepthRange) break;
        if (!strcmp(image, "cobble_d") || !strcmp(image, "pavers_d")) depth = 0.014f;
        else if (!strcmp(image, "cobble_wet_d")) { depth = 0.014f; wet = -1.0f; }
        else if (!strcmp(image, "mud_d") || !strcmp(image, "track_d")) { depth = 0.018f; wet = 0.5f; }
        else if (!strcmp(image, "rubble_d")) depth = 0.018f;
        else if (!strcmp(image, "brick_d") || !strcmp(image, "brick_old_d") || !strcmp(image, "stone_dark_d")) depth = 0.010f;
        else if (!strcmp(image, "wood_d")) depth = 0.004f;
        else if (!strcmp(image, "joinery_plank_d")) depth = 0.006f;
        else if (!strcmp(image, "joinery_panel_d")) depth = 0.010f;
        else if (!strncmp(image, "window_frame_", 13)) depth = 0.004f;
        else if (!strcmp(image, "shutter_single_d") || !strcmp(image, "shutter_single_side_d") ||
                 !strcmp(image, "shutter_pair_side_d")) depth = 0.009f;
        else if (!strcmp(image, "slate_d") || !strcmp(image, "roof_d")) depth = 0.008f;
        else if (!strncmp(image, "plaster", 7)) { depth = 0.003f; normalScale = 0.5f; }
        else if (!strcmp(image, "impact_stone_d") || !strcmp(image, "impact_plaster_d")) depth = 0.035f;
        else if (!strcmp(image, "impact_wood1_d") || !strcmp(image, "impact_wood2_d")) depth = 0.025f;
        break;
    }
    float masonry = name && state->fullDepthRange && !foliage && wet == 0.0f ?
        WebMaterials_MasonryRoughness(material) : 0.0f;
    WebEffects_Atmosphere(foliage, smoke, depth, wet, normalScale, metallic, masonry);
#endif
    WebEffects_Draw(draw.coverage, draw.feather, draw.heat, worldNearClip, draw.lens);
    WebEffects_FireBlend(draw.extinction);
}
