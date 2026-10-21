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
 *  - r_fxHeat: fire and explosions burn hotter, their cores reaching the glow. */

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
EM_JS(void, WebEffects_Draw, (float coverage, float feather, float heat, float worldNearClip), {
    var material = Module.cod2Material, fx = Module.cod2SceneFx;
    if (!material || !material.effect) return;
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
    WebEffects_Draw(draw.coverage, draw.feather, draw.heat, worldNearClip);
}
