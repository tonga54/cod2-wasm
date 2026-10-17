#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_weaponprompt.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"

extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern FontHandle UI_GetFontHandle(int fontEnum, float scale);
extern FontHandle CL_RegisterFont(const char *fontName, int imageTrack);
extern int UI_TextWidth(const char *text, int maxChars, FontHandle font, float scale);
extern void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style);
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material);
extern int GetKeyBindingLocalizedString(const char *command, char *keys);
extern WeaponDef *BG_GetWeaponDef(int iWeapon);
extern Bool BG_DoesWeaponNeedSlot(int weapIndex);
extern int BG_GetEmptySlotForWeapon(const playerState_t *pPS, int iWeaponIndex);
extern const char *va(const char *format, ...);

/* Virtual units from the stock hint's anchor, 70 below the screen centre,
 * matched on screen to the WWII capture's proportions: the key and the line
 * share a baseline half a cap height apart, and the silhouette's box, 1.8 cap
 * heights tall, starts 1.4 cap heights below the letters. */
#define WEAPON_PROMPT_SCALE 0.24f
#define WEAPON_PROMPT_FONT 0
#define WEAPON_PROMPT_BASELINE 4.0f
#define WEAPON_PROMPT_KEY_GAP 4.0f
#define WEAPON_PROMPT_ICON_TOP 7.0f
#define WEAPON_PROMPT_ICON_HEIGHT 14.0f
#define WEAPON_PROMPT_ICON_WIDTH 84.0f
#define WEAPON_PROMPT_SHADOW_OFFSET 1.0f
#define WEAPON_PROMPT_SHADOW_ALPHA 0.6f

enum {
    WEAPON_PROMPT_ICON_ONLY,
    WEAPON_PROMPT_PICK_UP,
    WEAPON_PROMPT_SWAP
};

/* The capture's colours, brightened to the HUD's text: a gold key, near-white
 * words and a cream silhouette. */
static const vec4_t s_weaponPromptKey = { 0.95f, 0.84f, 0.34f, 1.0f };
static const vec4_t s_weaponPromptText = { 0.96f, 0.95f, 0.92f, 1.0f };
static const vec4_t s_weaponPromptIcon = { 0.96f, 0.94f, 0.86f, 1.0f };

static const dvar_t *cg_hudWeaponPrompt;
static FontHandle s_weaponPromptKeyFont;

static void CG_WeaponPromptColor(vec4_t out, const vec4_t in, float alpha)
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
    out[3] = in[3] * alpha;
}

/* The stock hint's choice: a weapon that needs a slot when none is free is a
 * swap, unless the slot it would take already holds it. */
static int CG_WeaponPromptAction(const playerState_t *ps, int weapon)
{
    const WeaponDef *weapDef = BG_GetWeaponDef(weapon);

    if (!BG_DoesWeaponNeedSlot(weapon) || BG_GetEmptySlotForWeapon(ps, weapon))
        return WEAPON_PROMPT_PICK_UP;
    if (BG_GetWeaponDef(ps->weapon)->weapSlot == weapDef->weapSlot)
        return ps->weapon == weapon ? WEAPON_PROMPT_ICON_ONLY : WEAPON_PROMPT_SWAP;
    return (signed char)ps->weaponslots[weapDef->weapSlot] == weapon ? WEAPON_PROMPT_ICON_ONLY : WEAPON_PROMPT_SWAP;
}

static void CG_WeaponPromptDrawLine(const rectDef_t *rect, int textStyle, const char *key, const char *text,
                                    float alpha)
{
    FontHandle keyFont = s_weaponPromptKeyFont ? s_weaponPromptKeyFont : UI_GetFontHandle(4, WEAPON_PROMPT_SCALE);
    FontHandle font = UI_GetFontHandle(WEAPON_PROMPT_FONT, WEAPON_PROMPT_SCALE);
    float keyWidth = (float)UI_TextWidth(key, 0, keyFont, WEAPON_PROMPT_SCALE);
    float x = rect->x - 0.5f * (keyWidth + WEAPON_PROMPT_KEY_GAP +
                                (float)UI_TextWidth(text, 0, font, WEAPON_PROMPT_SCALE));
    float y = rect->y + WEAPON_PROMPT_BASELINE;
    vec4_t color;

    CG_WeaponPromptColor(color, s_weaponPromptKey, alpha);
    UI_DrawText(key, 0x7fffffff, keyFont, x, y, rect->horzAlign, rect->vertAlign, WEAPON_PROMPT_SCALE, color,
                textStyle);
    CG_WeaponPromptColor(color, s_weaponPromptText, alpha);
    UI_DrawText(text, 0x7fffffff, font, x + keyWidth + WEAPON_PROMPT_KEY_GAP, y, rect->horzAlign, rect->vertAlign,
                WEAPON_PROMPT_SCALE, color, textStyle);
}

/* The silhouette kept in proportion within its box and centred in it, over a
 * shadow; the stock icon stands in when no silhouette can be built. */
static void CG_WeaponPromptDrawIcon(const rectDef_t *rect, MaterialHandle icon, int wide, float alpha)
{
    float aspect;
    MaterialHandle material = R_GetHudSilhouetteMaterial(icon, &aspect);
    float w, h, x, y;
    vec4_t color;

    if (!material) {
        material = icon;
        aspect = wide ? 2.0f : 1.0f;
    }
    h = WEAPON_PROMPT_ICON_HEIGHT;
    w = h * aspect;
    if (w > WEAPON_PROMPT_ICON_WIDTH) {
        w = WEAPON_PROMPT_ICON_WIDTH;
        h = w / aspect;
    }
    x = rect->x - 0.5f * w;
    y = rect->y + WEAPON_PROMPT_BASELINE + WEAPON_PROMPT_ICON_TOP + 0.5f * (WEAPON_PROMPT_ICON_HEIGHT - h);
    color[0] = color[1] = color[2] = 0.0f;
    color[3] = alpha * WEAPON_PROMPT_SHADOW_ALPHA;
    CL_DrawStretchPic(x + WEAPON_PROMPT_SHADOW_OFFSET, y + WEAPON_PROMPT_SHADOW_OFFSET, w, h, rect->horzAlign,
                      rect->vertAlign, 0.0f, 0.0f, 1.0f, 1.0f, color, material);
    CG_WeaponPromptColor(color, s_weaponPromptIcon, alpha);
    CL_DrawStretchPic(x, y, w, h, rect->horzAlign, rect->vertAlign, 0.0f, 0.0f, 1.0f, 1.0f, color, material);
}

void CG_WeaponPromptInit(void)
{
    cg_hudWeaponPrompt = Dvar_RegisterBool_mac("cg_hudWeaponPrompt", 1, 0x1001);
    s_weaponPromptKeyFont = CL_RegisterFont("$boldfont", 0);
}

int CG_WeaponPromptActive(void)
{
    return cg_hudWeaponPrompt && cg_hudWeaponPrompt->current.enabled;
}

/* Where the stock hint would show the icon alone, so does the prompt. */
void CG_DrawWeaponPrompt(const rectDef_t *rect, int textStyle, int weapon, MaterialHandle icon, const vec_t *fadeColor)
{
    const WeaponDef *weapDef = BG_GetWeaponDef(weapon);
    int action = CG_WeaponPromptAction(&cg->predictedPlayerState, weapon);
    char key[0x100];

    if (action != WEAPON_PROMPT_ICON_ONLY) {
        GetKeyBindingLocalizedString("+activate", key);
        CG_WeaponPromptDrawLine(rect, textStyle, key,
                                va(action == WEAPON_PROMPT_SWAP ? "Press to swap for %s" : "Press to pick up %s",
                                   UI_SafeTranslateString(weapDef->szDisplayName)),
                                fadeColor[3]);
    }
    CG_WeaponPromptDrawIcon(rect, icon, weapDef->bWideListIcon, fadeColor[3]);
}
