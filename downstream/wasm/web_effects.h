#ifndef WEB_EFFECTS_H
#define WEB_EFFECTS_H

/* The device state of a draw that decides its effects. */
typedef struct {
    int is2D;
    int alphaTest;
    unsigned int alphaFunc;
    float alphaRef;
    int alphaBlend;
    int depthWrite;
    unsigned int srcBlend;
    unsigned int dstBlend;
    /* The world's depth range, [0, 1]: not the viewmodel's depth hack. */
    int fullDepthRange;
    /* The draw's D3D world matrix (rows: its axes, then its origin), or NULL;
     * the identity draws the map itself. */
    const float *world;
    /* The game's time of the view, in milliseconds. */
    int time;
} WebEffectState;

typedef struct {
    /* 1 when the draw's alpha test turns into multisample coverage. */
    float coverage;
    /* featherParms.x of a z-feathered effect, 1 / the units over which it
     * fades into what it meets and before the eye; negative when the blend
     * takes no alpha, so the color fades instead. 0 for none. */
    float feather;
    /* The emissive gain of fire and explosions. */
    float heat;
    /* A swaying model's push at full gust in its own space, then 1; then
     * where it stands in the world (x, y), its scale and the wind's clock in
     * seconds. All 0 for a draw the wind leaves still. */
    float wind[8];
    /* A scope's glass in the viewmodel: its disc's centre and radii in the
     * color map's coordinates; all 0 for anything else. */
    float lens[4];
} WebEffectDraw;

float WebEffects_Feather(const Material *material);
int WebEffects_IsFire(const Material *material);
float WebEffects_Foliage(const Material *material);
const float *WebEffects_Lens(const Material *material);
void WebEffects_Select(const Material *material, const WebEffectState *state, WebEffectDraw *draw);
void WebEffects_Apply(const Material *material, const WebEffectState *state, float worldNearClip);

#endif
