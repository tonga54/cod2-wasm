#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_ammocounter.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"

#include <string.h>

extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern FontHandle UI_GetFontHandle(int fontEnum, float scale);
extern FontHandle CL_RegisterFont(const char *fontName, int imageTrack);
extern int UI_TextWidth(const char *text, int maxChars, FontHandle font, float scale);
extern void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style);
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material);
extern float CG_FadeHudMenu(const dvar_t *fadeDvar, int displayStartTime, int duration);
extern Bool CG_CheckPlayerForLowAmmo(void);
extern Bool CG_CheckPlayerForLowClip(void);
extern int CG_GetHudWeaponIndex(void);
extern MaterialHandle CG_GetOffHandIcon(int weaponType, int *count);
extern int BG_GetTotalAmmoReserve(const playerState_t *ps, int weaponIndex);
extern qboolean BG_WeaponIsClipOnly(int weapon);
extern int BG_ClipForWeapon(int weapon);
extern WeaponDef *BG_GetWeaponDef(int iWeapon);
extern const char *va(const char *format, ...);
extern float floorf(float x);

extern const dvar_t *hud_fade_ammodisplay;
extern const dvar_t *hud_fade_offhand;

/* Virtual units from the bottom-right corner of the safe area, measured on the
 * WWII capture: the clip left of a thin rule, the reserve right of it level
 * with the clip's top, then the smoke and frag grenades. */
#define AMMO_COUNTER_RULE_X -91.5f
#define AMMO_COUNTER_RULE_Y -44.0f
#define AMMO_COUNTER_RULE_WIDTH 0.75f
#define AMMO_COUNTER_RULE_HEIGHT 20.0f
#define AMMO_COUNTER_CLIP_GAP 3.0f
#define AMMO_COUNTER_CLIP_Y -21.0f
#define AMMO_COUNTER_CLIP_SCALE 0.62f
#define AMMO_COUNTER_RESERVE_GAP 2.5f
#define AMMO_COUNTER_RESERVE_Y -34.0f
#define AMMO_COUNTER_RESERVE_SCALE 0.29f
#define AMMO_COUNTER_FONT 0
#define AMMO_COUNTER_SMOKE_X -58.5f
#define AMMO_COUNTER_FRAG_X -47.5f
#define AMMO_COUNTER_ICON_Y -31.0f
#define AMMO_COUNTER_ICON_SIZE 10.0f
#define AMMO_COUNTER_EMPTY_ALPHA 0.3f
/* A count past one sits at the icon's lower right. */
#define AMMO_COUNTER_COUNT_X 4.0f
#define AMMO_COUNTER_COUNT_Y 6.0f
#define AMMO_COUNTER_COUNT_SCALE 0.2f
/* The weapon name ends above the frag grenade; the fire-mode icon sits left
 * of the clip. */
#define AMMO_COUNTER_NAME_RIGHT -42.0f
#define AMMO_COUNTER_NAME_Y -52.0f
#define AMMO_COUNTER_NAME_SCALE 0.3f
#define AMMO_COUNTER_MODE_GAP 3.0f
#define AMMO_COUNTER_MODE_Y -41.0f
#define AMMO_COUNTER_MODE_SIZE 12.0f
/* The hint's baseline, below the screen centre, in the bold font. */
#define AMMO_COUNTER_HINT_Y 128.0f
#define AMMO_COUNTER_HINT_SCALE 0.25f
#define AMMO_COUNTER_ALIGN_CENTER 2
#define AMMO_COUNTER_ALIGN_RIGHT 3
#define AMMO_COUNTER_ALIGN_BOTTOM 3
#define AMMO_COUNTER_FRAG 1
#define AMMO_COUNTER_SMOKE 2
/* pm_type up to PM_NORMAL_LINKED, as for the stock mantle hint. */
#define AMMO_COUNTER_PM_LIVING 1
/* The stock mantle hint is drawn just below while this flag is set. */
#define AMMO_COUNTER_MANTLE_HINT 8
/* eFlags while on a mounted weapon, when the carried gun cannot reload. */
#define AMMO_COUNTER_TURRET 0x300
/* weaponstate from WEAPON_RELOADING to WEAPON_RELOAD_END. */
#define AMMO_COUNTER_RELOAD_FIRST 5
#define AMMO_COUNTER_RELOAD_LAST 9

enum {
    AMMO_COUNTER_HINT_NONE,
    AMMO_COUNTER_HINT_RELOAD,
    AMMO_COUNTER_HINT_LOW,
    AMMO_COUNTER_HINT_EMPTY,
    AMMO_COUNTER_HINT_COUNT
};

typedef struct {
    int weapon;
    int hasClip;
    int clip;
    int reserve;
    int lowClip;
    int lowAmmo;
} CG_AmmoCounterView;

static const char *const s_ammoCounterHints[AMMO_COUNTER_HINT_COUNT] = { "", "RELOAD", "LOW AMMO", "NO AMMO" };
static const vec4_t s_ammoCounterText = { 0.96f, 0.95f, 0.92f, 1.0f };
static const vec4_t s_ammoCounterReserve = { 0.75f, 0.75f, 0.73f, 1.0f };
static const vec4_t s_ammoCounterLow = { 0.95f, 0.6f, 0.6f, 1.0f };
static const vec4_t s_ammoCounterRule = { 0.75f, 0.75f, 0.73f, 0.8f };
static const vec4_t s_ammoCounterIcon = { 1.0f, 1.0f, 1.0f, 1.0f };

static const dvar_t *cg_hudAmmoCounter;
static FontHandle s_ammoCounterHintFont;

static void CG_AmmoCounterColor(vec4_t out, const vec4_t in, float alpha)
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
    out[3] = in[3] * alpha;
}

static float CG_AmmoCounterFade(const dvar_t *fadeDvar, int displayStartTime)
{
    return CG_FadeHudMenu(fadeDvar, displayStartTime, (int)floorf(fadeDvar->current.value * 1000.0f + 0.5f));
}

/* The stock counter's numbers: the reserve, and the clip unless the weapon
 * has none. */
static int CG_AmmoCounterGetView(const playerState_t *ps, CG_AmmoCounterView *view)
{
    memset(view, 0, sizeof(*view));
    if (!ps->weapon)
        return 0;
    view->weapon = CG_GetHudWeaponIndex();
    if (!view->weapon)
        return 0;
    view->reserve = BG_GetTotalAmmoReserve(ps, view->weapon);
    if (view->reserve < 0)
        view->reserve = 0;
    if (view->reserve > 999)
        view->reserve = 999;
    if (!BG_WeaponIsClipOnly(view->weapon)) {
        int clip = ps->ammoclip[BG_ClipForWeapon(view->weapon)];
        if (clip >= 0) {
            view->hasClip = 1;
            view->clip = clip > 999 ? 999 : clip;
        }
    }
    view->lowClip = CG_CheckPlayerForLowClip();
    view->lowAmmo = CG_CheckPlayerForLowAmmo();
    return 1;
}

/* For a living player's guns and launchers, off mounted weapons and not
 * during a reload. A low clip asks for a reload only while there is a
 * reserve to reload from. */
static int CG_AmmoCounterHint(const playerState_t *ps, const CG_AmmoCounterView *view)
{
    int type;

    if (ps->pm_type > AMMO_COUNTER_PM_LIVING || ps->stats[0] <= 0 ||
        (ps->mantleState.flags & AMMO_COUNTER_MANTLE_HINT) || (ps->eFlags & AMMO_COUNTER_TURRET))
        return AMMO_COUNTER_HINT_NONE;
    type = BG_GetWeaponDef(view->weapon)->weapType;
    if (type != WEAPTYPE_BULLET && type != WEAPTYPE_PROJECTILE)
        return AMMO_COUNTER_HINT_NONE;
    if (ps->weaponstate >= AMMO_COUNTER_RELOAD_FIRST && ps->weaponstate <= AMMO_COUNTER_RELOAD_LAST)
        return AMMO_COUNTER_HINT_NONE;
    if (view->clip + view->reserve <= 0)
        return AMMO_COUNTER_HINT_EMPTY;
    if (view->hasClip && view->lowClip && view->reserve > 0)
        return AMMO_COUNTER_HINT_RELOAD;
    return view->lowAmmo ? AMMO_COUNTER_HINT_LOW : AMMO_COUNTER_HINT_NONE;
}

/* The clip, or a weapon without one its reserve, ending left of the rule.
 * Returns where it starts. */
static float CG_AmmoCounterBigNumber(const CG_AmmoCounterView *view, const char **text, FontHandle *font)
{
    *text = va("%i", view->hasClip ? view->clip : view->reserve);
    *font = UI_GetFontHandle(AMMO_COUNTER_FONT, AMMO_COUNTER_CLIP_SCALE);
    return AMMO_COUNTER_RULE_X - AMMO_COUNTER_CLIP_GAP -
           (float)UI_TextWidth(*text, 0, *font, AMMO_COUNTER_CLIP_SCALE);
}

static void CG_AmmoCounterDrawAmmo(const CG_AmmoCounterView *view, float alpha)
{
    const char *text;
    FontHandle font;
    vec4_t color;
    float x = CG_AmmoCounterBigNumber(view, &text, &font);

    CG_AmmoCounterColor(color, (view->hasClip ? view->lowClip : view->lowAmmo) ? s_ammoCounterLow : s_ammoCounterText,
                        alpha);
    UI_DrawText(text, 0x7fffffff, font, x, AMMO_COUNTER_CLIP_Y, AMMO_COUNTER_ALIGN_RIGHT, AMMO_COUNTER_ALIGN_BOTTOM,
                AMMO_COUNTER_CLIP_SCALE, color, 3);
    if (!view->hasClip)
        return;

    CG_AmmoCounterColor(color, s_ammoCounterRule, alpha);
    CL_DrawStretchPic(AMMO_COUNTER_RULE_X, AMMO_COUNTER_RULE_Y, AMMO_COUNTER_RULE_WIDTH, AMMO_COUNTER_RULE_HEIGHT,
                      AMMO_COUNTER_ALIGN_RIGHT, AMMO_COUNTER_ALIGN_BOTTOM, 0.0f, 0.0f, 1.0f, 1.0f, color,
                      cgs->media.whiteMaterial);
    text = va("%i", view->reserve);
    font = UI_GetFontHandle(AMMO_COUNTER_FONT, AMMO_COUNTER_RESERVE_SCALE);
    CG_AmmoCounterColor(color, view->lowAmmo ? s_ammoCounterLow : s_ammoCounterReserve, alpha);
    UI_DrawText(text, 0x7fffffff, font, AMMO_COUNTER_RULE_X + AMMO_COUNTER_RULE_WIDTH + AMMO_COUNTER_RESERVE_GAP,
                AMMO_COUNTER_RESERVE_Y, AMMO_COUNTER_ALIGN_RIGHT, AMMO_COUNTER_ALIGN_BOTTOM,
                AMMO_COUNTER_RESERVE_SCALE, color, 3);
}

/* A white silhouette kept in proportion, or the stock icon if none can be
 * built. An empty slot stays faint. */
static void CG_AmmoCounterDrawOffhand(int offhandClass, float centerX, float alpha)
{
    MaterialHandle icon;
    MaterialHandle material;
    float aspect;
    float w = AMMO_COUNTER_ICON_SIZE;
    float h = AMMO_COUNTER_ICON_SIZE;
    vec4_t color;
    int count;

    icon = CG_GetOffHandIcon(offhandClass, &count);
    if (!icon)
        return;
    material = R_GetHudSilhouetteMaterial(icon, &aspect);
    if (!material) {
        material = icon;
        aspect = 1.0f;
    }
    if (aspect > 1.0f)
        h /= aspect;
    else
        w *= aspect;
    CG_AmmoCounterColor(color, s_ammoCounterIcon, count > 0 ? alpha : alpha * AMMO_COUNTER_EMPTY_ALPHA);
    CL_DrawStretchPic(centerX - 0.5f * w, AMMO_COUNTER_ICON_Y - 0.5f * h, w, h, AMMO_COUNTER_ALIGN_RIGHT,
                      AMMO_COUNTER_ALIGN_BOTTOM, 0.0f, 0.0f, 1.0f, 1.0f, color, material);
    if (count > 1) {
        CG_AmmoCounterColor(color, s_ammoCounterText, alpha);
        UI_DrawText(va("%i", count), 0x7fffffff, UI_GetFontHandle(AMMO_COUNTER_FONT, AMMO_COUNTER_COUNT_SCALE),
                    centerX + AMMO_COUNTER_COUNT_X, AMMO_COUNTER_ICON_Y + AMMO_COUNTER_COUNT_Y,
                    AMMO_COUNTER_ALIGN_RIGHT, AMMO_COUNTER_ALIGN_BOTTOM, AMMO_COUNTER_COUNT_SCALE, color, 3);
    }
}

static void CG_AmmoCounterDrawHint(int hint, float alpha)
{
    const char *text = s_ammoCounterHints[hint];
    FontHandle font = s_ammoCounterHintFont ? s_ammoCounterHintFont : UI_GetFontHandle(4, AMMO_COUNTER_HINT_SCALE);
    vec4_t color;

    CG_AmmoCounterColor(color, s_ammoCounterLow, alpha);
    UI_DrawText(text, 0x7fffffff, font, -0.5f * (float)UI_TextWidth(text, 0, font, AMMO_COUNTER_HINT_SCALE),
                AMMO_COUNTER_HINT_Y, AMMO_COUNTER_ALIGN_CENTER, AMMO_COUNTER_ALIGN_CENTER, AMMO_COUNTER_HINT_SCALE,
                color, 3);
}

void CG_AmmoCounterInit(void)
{
    cg_hudAmmoCounter = Dvar_RegisterBool_mac("cg_hudAmmoCounter", 1, 0x1001);
    s_ammoCounterHintFont = CL_RegisterFont("$boldfont", 0);
}

int CG_AmmoCounterActive(void)
{
    return cg_hudAmmoCounter && cg_hudAmmoCounter->current.enabled;
}

void CG_AmmoCounterPlaceWeaponName(rectDef_t *rect, float *scale)
{
    rect->x = AMMO_COUNTER_NAME_RIGHT - rect->w;
    rect->y = AMMO_COUNTER_NAME_Y;
    rect->horzAlign = AMMO_COUNTER_ALIGN_RIGHT;
    rect->vertAlign = AMMO_COUNTER_ALIGN_BOTTOM;
    *scale = AMMO_COUNTER_NAME_SCALE;
}

/* Without a counter there is nothing to sit beside. */
void CG_AmmoCounterPlaceModeIcon(rectDef_t *rect)
{
    CG_AmmoCounterView view;
    const char *text;
    FontHandle font;

    rect->horzAlign = AMMO_COUNTER_ALIGN_RIGHT;
    rect->vertAlign = AMMO_COUNTER_ALIGN_BOTTOM;
    rect->y = AMMO_COUNTER_MODE_Y;
    if (!CG_AmmoCounterGetView(&cg->predictedPlayerState, &view)) {
        rect->w = 0.0f;
        rect->h = 0.0f;
        return;
    }
    rect->x = CG_AmmoCounterBigNumber(&view, &text, &font) - AMMO_COUNTER_MODE_GAP - AMMO_COUNTER_MODE_SIZE;
    rect->w = AMMO_COUNTER_MODE_SIZE;
    rect->h = AMMO_COUNTER_MODE_SIZE;
}

void CG_DrawAmmoCounter(void)
{
    const playerState_t *ps = &cg->predictedPlayerState;
    CG_AmmoCounterView view;
    float alpha;
    int hint;

    if (!CG_AmmoCounterActive())
        return;
    alpha = CG_AmmoCounterFade(hud_fade_ammodisplay, cg->ammoFadeTime);
    if (alpha > 0.0f && CG_AmmoCounterGetView(ps, &view)) {
        CG_AmmoCounterDrawAmmo(&view, alpha);
        hint = CG_AmmoCounterHint(ps, &view);
        if (hint != AMMO_COUNTER_HINT_NONE)
            CG_AmmoCounterDrawHint(hint, alpha);
    }
    alpha = CG_AmmoCounterFade(hud_fade_offhand, cg->offhandFadeTime);
    if (alpha > 0.0f) {
        CG_AmmoCounterDrawOffhand(AMMO_COUNTER_SMOKE, AMMO_COUNTER_SMOKE_X, alpha);
        CG_AmmoCounterDrawOffhand(AMMO_COUNTER_FRAG, AMMO_COUNTER_FRAG_X, alpha);
    }
}
