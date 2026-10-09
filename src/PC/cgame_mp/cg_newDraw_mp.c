#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include <math.h>
#include <string.h>

extern const dvar_t *cg_drawHealth;
extern const dvar_t *cg_blood;
extern const dvar_t *cg_hudCompassSize;
extern const dvar_t *cg_hudStanceHintPrints;

extern const dvar_t *cg_cursorHints;
extern const dvar_t *cg_drawBreathHint;
extern const dvar_t *cg_drawMantleHint;
extern const dvar_t *cg_hintFadeTime;
extern const dvar_t *loc_warnings;
extern const dvar_t *loc_warningsAsErrors;

extern const dvar_t *hud_fade_ammodisplay;
extern const dvar_t *hud_fade_healthbar;
extern const dvar_t *hud_fade_compass;
extern const dvar_t *hud_fade_stance;
extern const dvar_t *hud_fade_offhand;
extern const dvar_t *hud_health_startpulse_injured;
extern const dvar_t *hud_health_startpulse_critical;
extern const dvar_t *hud_health_pulserate_injured;
extern const dvar_t *hud_health_pulserate_critical;
extern const dvar_t *hud_deathQuoteFadeTime;
static vec4_t color;
static char szErrorString[1024];
/* Fade successive pulses; an uninitialized table makes every pulse invisible. */
static const float pulseMags[4] = {1.0f, 0.8f, 0.6f, 0.4f};
static vec4_t color_00302d80;
static vec4_t color_00302d80;
static const dvar_t *hud_fadeout_speed;
static const dvar_t *hud_enable;
static const dvar_t *hud_healthOverlay_regenPauseTime;
static const dvar_t *hud_healthOverlay_pulseStart;
static const dvar_t *hud_healthOverlay_phaseOne_pulseDuration;
static const dvar_t *hud_healthOverlay_phaseTwo_toAlphaMultiplier;
static const dvar_t *hud_healthOverlay_phaseTwo_pulseDuration;
static const dvar_t *hud_healthOverlay_phaseThree_toAlphaMultiplier;
static const dvar_t *hud_healthOverlay_phaseThree_pulseDuration;
static const dvar_t *hud_healthOverlay_phaseEnd_toAlpha;
static const dvar_t *hud_healthOverlay_phaseEnd_pulseDuration;

typedef void (*MemoryFileArchiveProc)(MemoryFile *memFile, int size, void *data);

extern const char *va(const char *fmt, ...);
extern int CL_GetKeyCatchers(void);
extern Bool CL_GetDisplayHUDWithKeycatchUI(void);
extern const char *CL_GetConfigString(int index);
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material);
extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern const dvar_t *Dvar_RegisterFloat(const char *name, float value, float min, float max, int flags);
extern const dvar_t *Dvar_RegisterInt(const char *name, int value, int min, int max, int flags);
extern void Controls_GetConfig(void);
extern int BG_GetNumWeapons(void);
extern int GetKeyBindingLocalizedString(const char *command, char *keys);
extern int BG_GetViewmodelWeaponIndex(void *ps);
extern void *BG_GetWeaponDef(int weapIndex);
extern int BG_AmmoForWeapon(int weapon);
extern int BG_GetTotalAmmoReserve(const playerState_t *ps, int weaponIndex);
extern int BG_GetAmmoTypeMax(int iAmmoIndex);
extern qboolean BG_WeaponIsClipOnly(int weapon);
extern int BG_ClipForWeapon(int weapon);
extern int BG_GetAmmoClipSize(int iClipIndex);
extern Bool BG_DoesWeaponNeedSlot(int weapIndex);
extern int BG_GetEmptySlotForWeapon(const playerState_t *pPS, int iWeaponIndex);
extern float sinf(float x);
extern const char *SEH_LocalizeTextMessage(const char *msg, const char *context, int errType);
extern const char *UI_ReplaceConversionString(const char *sourceString, const char *replaceString);
extern const char *UI_SafeTranslateString(const char *ref);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern int UI_TextWidth(const char *text, int maxChars, FontHandle font, float fontScale);
extern int UI_TextHeight(FontHandle font, float fontScale);
extern void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style);
extern void UI_DrawHandlePic(float x, float y, float w, float h, int horzAlign, int vertAlign, const vec_t *color, MaterialHandle material);
extern const char *SEH_StringEd_GetString(const char *pszReference);
extern void Com_Error(errorParm_t code, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern void I_strncat(char *dest, int maxlen, const char *src);
extern qboolean CG_ScoreboardDisplayed(void);
extern void CG_ScrollScoreboardUp(void);
extern void CG_ScrollScoreboardDown(void);
extern float *CG_FadeColor(int startMsec, int totalMsec, int fadeMsec);
extern qboolean GetCommandHasBinding(const char *command);
extern void Dvar_GetUnpackedColor(const dvar_t *dvar, vec_t *expandedColor);
extern void CG_UpdateCompassOrientation(void);
extern void CG_UpdateCompPointerOrientation(void);
extern void CG_DrawRotatedPic(float x, float y, float width, float height, int horzAlign, int vertAlign, float angle, const vec_t *color, MaterialHandle material);
extern void CG_DrawCompassFriendlies(rectDef_t *rect, MaterialHandle material, vec_t *color);
extern void CG_DrawOffHandName(rectDef_s *rect, struct Font_s *font, float scale, vec_t *color, int textStyle, int weaponType);
extern void CG_DrawOffHandIcon(rectDef_s *rect, float scale, vec_t *color, MaterialHandle material, int weaponType);
extern void CG_DrawOffHandHighlight(rectDef_s *rect, float scale, vec_t *color, MaterialHandle material, int weaponType);
extern void CG_DrawOffHandAmmo(rectDef_s *rect, struct Font_s *font, float scale, vec_t *color, int textStyle, int weaponType);
extern void CG_DrawTeamBackground(float x, float y, float w, float h, float alpha, int team);

enum {
    CG_PLAYER_AMMO_VALUE = 5,
    CG_PLAYER_AMMO_BACKDROP = 6,
    CG_PLAYER_STANCE = 20,
    CG_BLUE_SCORE = 27,
    CG_RED_SCORE = 28,
    CG_PLAYER_LOCATION = 33,
    CG_TEAM_COLOR = 34,
    CG_GAME_TYPE = 39,
    CG_KILLER = 50,
    CG_SPECTATORS = 60,
    CG_CAPFRAGLIMIT = 66,
    CG_1STPLACE = 67,
    CG_PLAYER_AMMOCLIP_VALUE = 70,
    CG_HOLD_BREATH_HINT = 71,
    CG_CURSORHINT = 72,
    CG_PLAYER_BAR_HEALTH = 79,
    CG_MANTLE_HINT = 80,
    CG_PLAYER_WEAPON_NAME = 81,
    CG_PLAYER_WEAPON_NAME_BACK = 82,
    CG_PLAYER_WEAPON_MODE_ICON = 83,
    CG_PLAYER_COMPASS = 84,
    CG_PLAYER_COMPASS_BACK = 85,
    CG_PLAYER_COMPASS_POINTERS = 86,
    CG_PLAYER_COMPASS_FRIENDS = 89,
    CG_DRAW_SHADER = 94,
    CG_PLAYER_BAR_HEALTH_BACK = 95,
    CG_OFFHAND_WEAPON_ICON_FRAG = 100,
    CG_OFFHAND_WEAPON_ICON_SMOKE = 101,
    CG_OFFHAND_WEAPON_AMMO_FRAG = 102,
    CG_OFFHAND_WEAPON_AMMO_SMOKE = 103,
    CG_OFFHAND_WEAPON_NAME_FRAG = 104,
    CG_OFFHAND_WEAPON_NAME_SMOKE = 105,
    CG_OFFHAND_WEAPON_SELECT_FRAG = 106,
    CG_OFFHAND_WEAPON_SELECT_SMOKE = 107,
    CG_PLAYER_LOW_HEALTH_OVERLAY = 109
};

void CG_AntiBurnInHUD_RegisterDvars(void);
Bool CG_AreHudMenusHidden(void);
float CG_CalcPlayerHealth(void);
void CG_ResetLowHealthOverlay(void);
qboolean CG_ServerMaterialName(int index, char *materialName, int maxLen);
void CG_ApplySplitScreenCompassScale(float *x, float *y, float *w, float *h);
static const char *CG_GetUseString(void);
static void CG_DrawCursorhint(const rectDef_t *rect, struct Font_s *font, float fontscale, int textStyle);
void CG_DrawMantleHint(const rectDef_t *rect, struct Font_s *font, float fontscale, int textStyle);
const char *CG_GetTranslatedLocationString(int iLocation);
static void __attribute_regparm__(3) CG_DrawScore(int team, const rectDef_t *rect, struct Font_s *font, float scale, vec_t *color, MaterialHandle material, int textStyle);
const char *CG_GetKillerText(void);
const char *CG_GameTypeString(void);
int CG_KeyInterceptEvent(int key, qboolean down);
static void CG_PulseLowHealthOverlay(float healthRatio);
void CG_ArchiveState(MemoryFile *memFile);
void CG_DrawHoldBreathHint(const rectDef_t *rect, struct Font_s *font, float fontscale, int textStyle);
float CG_FadeHudMenu(const dvar_t *fadeDvar, int displayStartTime, int duration);
Bool CG_CheckPlayerForLowAmmo(void);
Bool CG_CheckPlayerForLowClip(void);
void CG_DrawPlayerCompassBack(const rectDef_t *rect, MaterialHandle material, vec_t *color);
static void CG_DrawPlayerStance(const rectDef_t *rect, vec_t *color, struct Font_s *font, float scale, int textStyle);
static void CG_DrawPlayerAmmoValue(const rectDef_t *rect, struct Font_s *font, float scale, vec_t *color, MaterialHandle material, int textStyle, int type);
void CG_OwnerDraw(float x, float y, float w, float h, int horzAlign, int vertAlign, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, struct Font_s *font, float scale, vec_t *color, MaterialHandle material, int textStyle);

COD2_ASSERT_FIELD(struct dvar_s, current,        0x08);
COD2_ASSERT_FIELD(WeaponDef,     weapClass,      0x7c);
COD2_ASSERT_FIELD(WeaponDef,     weapSlot,       0x80);
COD2_ASSERT_FIELD(WeaponDef,     overlayReticle, 0x278);
COD2_ASSERT_FIELD(WeaponDef,     bWideListIcon,  0x344);
COD2_ASSERT_FIELD(playerState_t, fWeaponPosFrac, 0xdc);

void CG_AntiBurnInHUD_RegisterDvars(void)
{
    hud_fadeout_speed = Dvar_RegisterFloat("hud_fadeout_speed", 0.1f, 0.0f, 1.0f, 0x1001);
    hud_enable = Dvar_RegisterBool_mac("hud_enable", 1, 0x1001);

    hud_fade_ammodisplay = Dvar_RegisterFloat("hud_fade_ammodisplay", 0.0f, 0.0f, 30.0f, 0x1001);
    hud_fade_healthbar = Dvar_RegisterFloat("hud_fade_healthbar", 2.0f, 0.0f, 30.0f, 0x1001);
    hud_fade_compass = Dvar_RegisterFloat("hud_fade_compass", 0.0f, 0.0f, 30.0f, 0x1001);
    hud_fade_stance = Dvar_RegisterFloat("hud_fade_stance", 1.7f, 0.0f, 30.0f, 0x1001);
    hud_fade_offhand = Dvar_RegisterFloat("hud_fade_offhand", 0.0f, 0.0f, 30.0f, 0x1001);

    hud_health_startpulse_injured = Dvar_RegisterFloat("hud_health_startpulse_injured", 1.0f, 0.0f, 1.1f, 0x1001);
    hud_health_startpulse_critical = Dvar_RegisterFloat("hud_health_startpulse_critical", 0.33f, 0.0f, 1.1f, 0x1001);
    hud_health_pulserate_injured = Dvar_RegisterFloat("hud_health_pulserate_injured", 1.0f, 0.1f, 3.0f, 0x1001);
    hud_health_pulserate_critical = Dvar_RegisterFloat("hud_health_pulserate_critical", 0.5f, 0.1f, 3.0f, 0x1001);

    hud_deathQuoteFadeTime = Dvar_RegisterInt("hud_deathQuoteFadeTime", 1000, 0, 100000, 0x1001);

    hud_healthOverlay_regenPauseTime = Dvar_RegisterInt("hud_healthOverlay_regenPauseTime", 5000, 0, 10000, 0x1080);
    hud_healthOverlay_pulseStart = Dvar_RegisterFloat("hud_healthOverlay_pulseStart", 0.35f, 0.0f, 1.0f, 0x1080);
    hud_healthOverlay_phaseOne_pulseDuration = Dvar_RegisterInt("hud_healthOverlay_phaseOne_pulseDuration", 150, 0, 1000, 0x1080);
    hud_healthOverlay_phaseTwo_toAlphaMultiplier = Dvar_RegisterFloat("hud_healthOverlay_phaseTwo_toAlphaMultiplier", 0.7f, 0.0f, 1.0f, 0x1080);
    hud_healthOverlay_phaseTwo_pulseDuration = Dvar_RegisterInt("hud_healthOverlay_phaseTwo_pulseDuration", 320, 0, 1000, 0x1080);
    hud_healthOverlay_phaseThree_toAlphaMultiplier = Dvar_RegisterFloat("hud_healthOverlay_phaseThree_toAlphaMultiplier", 0.6f, 0.0f, 1.0f, 0x1080);
    hud_healthOverlay_phaseThree_pulseDuration = Dvar_RegisterInt("hud_healthOverlay_phaseThree_pulseDuration", 400, 0, 1000, 0x1080);
    hud_healthOverlay_phaseEnd_toAlpha = Dvar_RegisterFloat("hud_healthOverlay_phaseEnd_toAlpha", 0.0f, 0.0f, 1.0f, 0x1080);
    hud_healthOverlay_phaseEnd_pulseDuration = Dvar_RegisterInt("hud_healthOverlay_phaseEnd_pulseDuration", 700, 0, 1000, 0x1080);
}

static inline __attribute__((always_inline)) Bool CG_AreHudMenusHidden_inl(void)
{
    if ((CL_GetKeyCatchers() & 8) && !CL_GetDisplayHUDWithKeycatchUI()) {
        return 1;
    }

    if (cg->drawHud) {
        return 0;
    }
    return 1;
}

Bool CG_AreHudMenusHidden(void)
{
    return CG_AreHudMenusHidden_inl();
}

float CG_CalcPlayerHealth(void)
{
    playerState_t *ps;
    float healthRatio;

    ps = &cg->nextSnap->ps;
    if (ps->stats[0] == 0 || ps->stats[2] == 0 || ps->pm_type == 6) {
        return 0.0f;
    }

    healthRatio = (float)ps->stats[0] / (float)ps->stats[2];
    if (healthRatio < 0.0f) {
        return 0.0f;
    }
    if (healthRatio > 1.0f) {
        return 1.0f;
    }
    return healthRatio;
}

void CG_ResetLowHealthOverlay(void)
{
    cg->healthOverlayHurt = 0;
    cg->healthOverlayToAlpha = hud_healthOverlay_phaseEnd_toAlpha->current.value;
    cg->healthOverlayPulseDuration = 0;
    cg->healthOverlayPulsePhase = 0;
    cg->healthOverlayPulseIndex = 0;
    cg->healthOverlayOldHealth = 1.0f;
}

qboolean CG_ServerMaterialName(int index, char *materialName, int maxLen)
{
    const char *configString;

    if ((unsigned int)(index - 1) > 0x7e) {
        return 0;
    }

    configString = CL_GetConfigString(index + 0x61e);
    if (configString[0] == '\0') {
        return 0;
    }

    if (strlen(configString) >= (unsigned int)maxLen) {
        return 0;
    }

    strcpy(materialName, configString);
    return 1;
}

void CG_ApplySplitScreenCompassScale(float *x, float *y, float *w, float *h)
{

}

const char *CG_GetUseString(void)
{
    const char *hintString;
    char binding[0x100];

    hintString = CL_GetConfigString(cg->cursorHintString + 0x4fe);
    if (!hintString || !*hintString) {
        return 0;
    }

    if (!GetKeyBindingLocalizedString("+activate", binding)) {
        I_strncpyz(binding, UI_SafeTranslateString("KEY_USE"), sizeof(binding));
    }

    return UI_ReplaceConversionString(SEH_LocalizeTextMessage(hintString, "Hint String", 0), binding);
}

static void CG_DrawCursorhint(const rectDef_t *rect, struct Font_s *font, float fontscale, int textStyle)
{
    snapshot_t *snap;
    int cursorHintValue;
    int cursorHintString;
    MaterialHandle hintIcon;
    float *fadeColor;
    float scale;
    float halfscale;
    float widthScale;
    float widthOfs;
    const char *text;
    float length;
    int cursorHintsDvarVal;
    char binding[0x100];

    if (!cg_cursorHints->current.integer)
        return;

    if (!cg->renderingThirdPerson) {

        snap = cg->nextSnap;
        if (snap->ps.cursorHint) {

            cg->cursorHintTime = cg->time;
            cg->cursorHintFade = cg_hintFadeTime->current.integer;
            cg->cursorHintIcon = snap->ps.cursorHint;
            cg->cursorHintString = snap->ps.cursorHintString;
        }
    }

    cursorHintValue = cg->cursorHintIcon;
    hintIcon = cgs->media.hintMaterials[cursorHintValue];
    if (!hintIcon)
        return;

    fadeColor = CG_FadeColor(cg->cursorHintTime, cg->cursorHintFade, 100);
    if (!fadeColor) {

        cg->cursorHintIcon = 0;
        return;
    }

    Controls_GetConfig();

    cursorHintsDvarVal = cg_cursorHints->current.integer;

    if (cursorHintsDvarVal == 3) {

        float sinVal = sinf((float)cg->time / 150.0f);
        fadeColor[3] *= sinVal * 0.5f + 0.5f;

        cursorHintsDvarVal = cg_cursorHints->current.integer;
    }

    if (cursorHintsDvarVal <= 2) {
        if (cursorHintsDvarVal == 2) {

            int startTime = cg->cursorHintTime;
            scale = (float)(startTime % 1000) / 100.0f;
        } else {

            float sinVal = sinf((float)cg->time / 150.0f);
            scale = (sinVal * 0.5f + 0.5f) * 10.0f;
        }
        halfscale = 0.5f * scale;
    } else {

        scale = 0.0f;
        halfscale = 0.0f;
    }

    cursorHintValue = cg->cursorHintIcon;
    text = NULL;
    widthScale = 1.0f;
    widthOfs = 0.0f;

    if ((unsigned)(cursorHintValue - 5) <= 0x7f) {

        WeaponDef *weapDef;
        int weapIdx = cursorHintValue - 4;

        weapDef = (WeaponDef *)BG_GetWeaponDef(weapIdx);

        if (weapDef->bWideListIcon) {

            widthOfs = rect->w * -0.5f;
            widthScale = 2.0f;
        }

        if (weapDef->weapClass == 7) {

            cursorHintString = cg->cursorHintString;
            if (cursorHintString >= 0) {
                text = CG_GetUseString();
            }

        } else {

            WeaponDef *pickupWeapDef;

            weapIdx = cg->cursorHintIcon - 4;
            pickupWeapDef = (WeaponDef *)BG_GetWeaponDef(weapIdx);

            GetKeyBindingLocalizedString("+activate", binding);

            if (BG_DoesWeaponNeedSlot(weapIdx)) {
                int emptySlot = BG_GetEmptySlotForWeapon((const playerState_t *)&cg->predictedPlayerState, weapIdx);
                if (emptySlot == 0) {

                    WeaponDef *currentWeapDef;
                    int currentWeap = cg->predictedPlayerState.weapon;
                    currentWeapDef = (WeaponDef *)BG_GetWeaponDef(currentWeap);

                    if (currentWeapDef->weapSlot == pickupWeapDef->weapSlot) {

                        if (cg->predictedPlayerState.weapon == weapIdx) {
                            text = NULL;
                            goto draw_icon;
                        }

                        text = UI_ReplaceConversionString(UI_SafeTranslateString("PLATFORM_SWAPWEAPONS"), binding);
                        goto check_text;
                    }

                    if ((signed char)cg->predictedPlayerState.weaponslots[pickupWeapDef->weapSlot] == weapIdx) {
                        text = NULL;
                        goto draw_icon;
                    }

                    text = UI_ReplaceConversionString(UI_SafeTranslateString("PLATFORM_SWAPWEAPONS"), binding);
                    goto check_text;
                }
            }

            text = UI_ReplaceConversionString(UI_SafeTranslateString("PLATFORM_PICKUPNEWWEAPON"), binding);
            goto check_text;
        }
    } else {

        cursorHintString = cg->cursorHintString;

        if (cursorHintString < 0) {

            if (cursorHintValue == 3) {

                GetKeyBindingLocalizedString("+activate", binding);
                text = UI_ReplaceConversionString(UI_SafeTranslateString("PLATFORM_PICKUPHEALTH"), binding);
                widthScale = 1.0f;
                widthOfs = 0.0f;
            } else {

                widthScale = 1.0f;
                widthOfs = 0.0f;
                text = NULL;
                goto draw_icon;
            }
        } else {

            text = CG_GetUseString();
        }
    }

check_text:

    if (text && *text) {

        float textWidth;
        int textHeight;
        float textX, textY;
        float iconW;

        textWidth = (float)UI_TextWidth(text, 0, font, fontscale);
        textHeight = UI_TextHeight(font, fontscale);

        textX = (widthScale * rect->w + scale + textWidth) * -0.5f;
        textY = -0.5f * rect->h + rect->y;

        UI_DrawText(
            text,
            0x7fffffff,
            font,
            textX,
            rect->y + (float)textHeight * 0.5f,
            rect->horzAlign,
            rect->vertAlign,
            fontscale,
            fadeColor,
            textStyle);

        iconW = widthScale * rect->w;
        UI_DrawHandlePic(
            textX + textWidth,
            textY,
            iconW + scale,
            scale + rect->h,
            rect->horzAlign,
            rect->vertAlign,
            fadeColor,
            hintIcon);
        return;
    }

draw_icon:

    {
        float iconW = widthScale * rect->w;
        float iconX = (rect->w + halfscale + widthOfs) * -0.5f + rect->x;
        UI_DrawHandlePic(
            iconX,
            rect->y - halfscale,
            iconW + scale,
            scale + rect->h,
            rect->horzAlign,
            rect->vertAlign,
            fadeColor,
            hintIcon);
    }
}

void CG_DrawMantleHint(const rectDef_t *rect, struct Font_s *font, float fontscale, int textStyle)
{
    char binding[0x100];
    const char *text;
    float length;
    float x;

    if (!cg_drawMantleHint->current.enabled) {
        return;
    }

    if (!(*(byte *)&cg->predictedPlayerState.mantleState.flags & 8)) {
        return;
    }

    Controls_GetConfig();
    if (!GetKeyBindingLocalizedString("+gostand", binding)) {
        GetKeyBindingLocalizedString("+moveup", binding);
    }

    text = UI_ReplaceConversionString(UI_SafeTranslateString("PLATFORM_MANTLE"), binding);
    length = (float)UI_TextWidth(text, 0, font, fontscale);
    x = (length + rect->w) * -0.5f + rect->x;

    UI_DrawText(
        text,
        0x7fffffff,
        font,
        x,
        rect->y + (float)UI_TextHeight(font, fontscale) * 0.5f,
        rect->horzAlign,
        rect->vertAlign,
        fontscale,
        color,
        textStyle);

    UI_DrawHandlePic(
        x + length,
        rect->h * -0.5f + rect->y,
        rect->w,
        rect->h,
        rect->horzAlign,
        rect->vertAlign,
        color,
        cgs->media.mantleHint);
}

const char *CG_GetTranslatedLocationString(int iLocation)
{
    const char *p;
    const char *localized;

    p = CL_GetConfigString(iLocation + 0x2e);
    if (!p || !*p) {
        p = "CGAME_UNKNOWN";
    }

    localized = SEH_StringEd_GetString(p);
    if (localized) {
        return localized;
    }

    if (loc_warnings->current.enabled) {
        if (loc_warningsAsErrors->current.enabled) {
            Com_Error(ERR_LOCALIZATION, "Could not translate map location string \"%s\"", p);
        } else {
            Com_Printf("^3WARNING: Could not translate map location string \"%s\"\n", p);
        }

        strcpy(szErrorString, "^1UNLOCALIZED(^7");
        I_strncat(szErrorString, sizeof(szErrorString), p);
        I_strncat(szErrorString, sizeof(szErrorString), "^1)^7");
        return szErrorString;
    }

    I_strncpyz(szErrorString, p, sizeof(szErrorString));
    return szErrorString;
}

static void __attribute_regparm__(3)
    CG_DrawScore(int team, const rectDef_t *rect, struct Font_s *font, float scale, vec_t *color, MaterialHandle material, int textStyle)
{
    char scoreText[16];
    int textWidth;

    (void)material;

    if (cgs->teamScores[team] == -9999) {
        Com_sprintf(scoreText, sizeof(scoreText), "-");
    } else {
        Com_sprintf(scoreText, sizeof(scoreText), "%i", cgs->teamScores[team]);
    }

    textWidth = UI_TextWidth(scoreText, 0, font, scale);
    UI_DrawText(
        scoreText,
        0x7fffffff,
        font,
        rect->x + rect->w - (float)textWidth,
        rect->y + rect->h,
        rect->horzAlign,
        rect->vertAlign,
        scale,
        color,
        textStyle);
}

const char *CG_GetKillerText(void)
{
    if (!cg->killerName[0])
        return "";
    return va("Fragged by %s", (const char *)cg->killerName);
}

const char *CG_GameTypeString(void)
{
    return cgs->gametype;
}

int CG_KeyInterceptEvent(int key, qboolean down)
{
    int dv = down;
    if (!dv || !CG_ScoreboardDisplayed()) {
        return 0;
    }

    if (key == 0xce || key == 0xa4) {
        CG_ScrollScoreboardUp();
        return 1;
    }

    if (key == 0xcd || key == 0xa3) {
        CG_ScrollScoreboardDown();
        return 1;
    }

    return 0;
}

void CG_PulseLowHealthOverlay(float healthRatio)
{
    float targetAlpha;
    int pulseIndex;
    int phaseThreeDuration;
    int nextPulseThresholdTime;

    if (cg->healthOverlayOldHealth > healthRatio && hud_healthOverlay_pulseStart->current.value > healthRatio) {
        cg->healthOverlayLastHitTime = cg->time;
        cg->healthOverlayPulseIndex = 0;
    }

    cg->healthOverlayOldHealth = healthRatio;
    if (cg->healthOverlayPulseTime + cg->healthOverlayPulseDuration > cg->time) {
        return;
    }

    if (hud_healthOverlay_pulseStart->current.value > healthRatio) {
        if (!cg->healthOverlayHurt) {
            cg->healthOverlayHurt = 1;
        }
    } else if (!cg->healthOverlayHurt) {
        return;
    }

    cg->healthOverlayPulseTime = cg->time;
    cg->healthOverlayFromAlpha = cg->healthOverlayToAlpha;
    pulseIndex = cg->healthOverlayPulseIndex;
    if (pulseIndex > 3) {
        cg->healthOverlayHurt = 0;
        cg->healthOverlayToAlpha = hud_healthOverlay_phaseEnd_toAlpha->current.value;
        cg->healthOverlayPulseDuration = hud_healthOverlay_phaseEnd_pulseDuration->current.integer;
        cg->healthOverlayPulsePhase = 0;
        return;
    }

    if (cg->healthOverlayPulsePhase == 0) {
        targetAlpha = pulseMags[pulseIndex];
        if (targetAlpha < 0.0f) {
            targetAlpha = 0.0f;
        } else if (targetAlpha > 1.0f) {
            targetAlpha = 1.0f;
        }

        cg->healthOverlayToAlpha = targetAlpha;
        cg->healthOverlayPulseDuration = hud_healthOverlay_phaseOne_pulseDuration->current.integer;
        cg->healthOverlayPulsePhase = 1;
        return;
    }

    if (cg->healthOverlayPulsePhase == 1) {
        targetAlpha = pulseMags[pulseIndex] * hud_healthOverlay_phaseTwo_toAlphaMultiplier->current.value;
        if (targetAlpha < 0.0f) {
            targetAlpha = 0.0f;
        } else if (targetAlpha > 1.0f) {
            targetAlpha = 1.0f;
        }

        cg->healthOverlayToAlpha = targetAlpha;
        cg->healthOverlayPulseDuration = hud_healthOverlay_phaseTwo_pulseDuration->current.integer;
        cg->healthOverlayPulsePhase = 2;
        return;
    }

    if (cg->healthOverlayPulsePhase != 2) {
        return;
    }

    targetAlpha = pulseMags[pulseIndex] * hud_healthOverlay_phaseThree_toAlphaMultiplier->current.value;
    if (targetAlpha < 0.0f) {
        targetAlpha = 0.0f;
    } else if (targetAlpha > 1.0f) {
        targetAlpha = 1.0f;
    }

    cg->healthOverlayToAlpha = targetAlpha;
    phaseThreeDuration = hud_healthOverlay_phaseThree_pulseDuration->current.integer;
    cg->healthOverlayPulseDuration = phaseThreeDuration;
    cg->healthOverlayPulsePhase = 0;

    nextPulseThresholdTime = cg->healthOverlayLastHitTime + hud_healthOverlay_regenPauseTime->current.integer;
    nextPulseThresholdTime -= (hud_healthOverlay_phaseOne_pulseDuration->current.integer + hud_healthOverlay_phaseTwo_pulseDuration->current.integer + phaseThreeDuration) * 3;
    if (cg->time >= nextPulseThresholdTime) {
        cg->healthOverlayPulseIndex = pulseIndex + 1;
    }
}

void CG_ArchiveState(MemoryFile *memFile)
{

    ((MemoryFileArchiveProc)memFile->archiveProc)(memFile, 4, (byte *)&cg->healthFadeTime);
    ((MemoryFileArchiveProc)memFile->archiveProc)(memFile, 4, (byte *)&cg->ammoFadeTime);
    ((MemoryFileArchiveProc)memFile->archiveProc)(memFile, 4, (byte *)&cg->stanceFadeTime);
    ((MemoryFileArchiveProc)memFile->archiveProc)(memFile, 4, (byte *)&cg->compassFadeTime);
    ((MemoryFileArchiveProc)memFile->archiveProc)(memFile, 4, (byte *)&cg->offhandFadeTime);
    ((MemoryFileArchiveProc)memFile->archiveProc)(memFile, 4, (byte *)&cg->drawHud);
    ((MemoryFileArchiveProc)memFile->archiveProc)(memFile, 0x400, cg->objectiveText);
    ((MemoryFileArchiveProc)memFile->archiveProc)(memFile, 0x100, cg->scriptMainMenu);
}

void CG_DrawHoldBreathHint(const rectDef_t *rect, struct Font_s *font, float fontscale, int textStyle)
{
    playerState_t *ps;
    WeaponDef *weaponDef;
    char binding[264];
    const char *text;
    int textWidth;
    int textOffset;

    if (!cg_drawBreathHint->current.enabled) {
        return;
    }

    ps = &cg->predictedPlayerState;
    if (ps->pm_flags & 0x4000) {
        return;
    }

    weaponDef = (WeaponDef *)BG_GetWeaponDef(BG_GetViewmodelWeaponIndex(ps));
    if (!weaponDef->overlayReticle || weaponDef->weapClass == 9) {
        return;
    }

    if (ps->fWeaponPosFrac != 1.0f) {
        return;
    }

    Controls_GetConfig();
    if (!GetKeyBindingLocalizedString("+holdbreath", binding)) {
        if (!GetKeyBindingLocalizedString("+melee_breath", binding)) {
            GetKeyBindingLocalizedString("+breath_binoculars", binding);
        }
    }

    text = UI_ReplaceConversionString(UI_SafeTranslateString("PLATFORM_HOLD_BREATH"), binding);
    textWidth = UI_TextWidth(text, 0, font, fontscale);
    textOffset = (int)(textWidth * 0.5f + 0.5f);

    UI_DrawText(
        text,
        0x7fffffff,
        font,
        rect->x - (float)textOffset,
        rect->y,
        rect->horzAlign,
        rect->vertAlign,
        fontscale,
        color,
        textStyle);
}

static inline __attribute__((always_inline)) float CG_FadeHudMenu_inl(const dvar_t *fadeDvar, int displayStartTime, int duration)
{
    float *fadeColor;

    if (CG_AreHudMenusHidden_inl() || !hud_enable->current.enabled) {
        return 0.0f;
    }

    if (fadeDvar->current.value == 0.0f) {
        return 1.0f;
    }

    fadeColor = CG_FadeColor(displayStartTime, duration, 700);
    if (!fadeColor) {
        return 0.0f;
    }

    return fadeColor[3];
}

float CG_FadeHudMenu(const dvar_t *fadeDvar, int displayStartTime, int duration)
{
    return CG_FadeHudMenu_inl(fadeDvar, displayStartTime, duration);
}

Bool CG_CheckPlayerForLowAmmo(void)
{
    playerState_t *ps;
    int weaponIndex;
    int ammoIndex;
    int currentAmmo;
    int maxAmmo;

    ps = &cg->predictedPlayerState;
    weaponIndex = cg->weaponSelect;

    if (weaponIndex < 0 || weaponIndex >= BG_GetNumWeapons() || !(ps->weapons[weaponIndex >> 5] & (1 << (weaponIndex & 0x1f)))) {
        weaponIndex = ps->weapon;
    }

    if (!weaponIndex) {
        return 0;
    }

    ammoIndex = BG_AmmoForWeapon(weaponIndex);
    currentAmmo = BG_GetTotalAmmoReserve(ps, weaponIndex);
    if (currentAmmo >= 1000) {
        currentAmmo = 999;
    }

    maxAmmo = BG_GetAmmoTypeMax(ammoIndex);
    if (maxAmmo < 0) {
        return 0;
    }
    if (maxAmmo > 999) {
        maxAmmo = 999;
    }

    if (0.2f * (float)maxAmmo < (float)currentAmmo) {
        return 0;
    }
    return 1;
}

Bool CG_CheckPlayerForLowClip(void)
{
    playerState_t *ps;
    int weaponIndex;
    int clipIndex;
    int currentClip;
    int clipSize;

    ps = &cg->predictedPlayerState;
    weaponIndex = cg->weaponSelect;

    if (weaponIndex < 0 || weaponIndex >= BG_GetNumWeapons() || !(ps->weapons[weaponIndex >> 5] & (1 << (weaponIndex & 0x1f)))) {
        weaponIndex = ps->weapon;
    }

    if (!weaponIndex || BG_WeaponIsClipOnly(weaponIndex)) {
        return 0;
    }

    clipIndex = BG_ClipForWeapon(weaponIndex);
    currentClip = ps->ammoclip[clipIndex];
    if (currentClip < 0) {
        return 0;
    }
    if (currentClip >= 1000) {
        currentClip = 999;
    }

    clipSize = BG_GetAmmoClipSize(clipIndex);
    if (clipSize <= 0) {
        return 0;
    }
    if (clipSize > 999) {
        clipSize = 999;
    }

    if (0.330000013f * (float)clipSize < (float)currentClip) {
        return 0;
    }
    return 1;
}

void CG_DrawPlayerCompassBack(const rectDef_t *rect, MaterialHandle material, vec_t *color)
{
    const dvar_t *compassSizeDvar;
    int duration;
    int displayStartTime;
    float alpha;
    float sizeScale;
    float height;

    duration = (int)floorf(1000.0f * hud_fade_compass->current.value + 0.5f);
    displayStartTime = cg->compassFadeTime;
    alpha = CG_FadeHudMenu_inl(hud_fade_compass, displayStartTime, duration);
    color[3] = alpha;
    if (alpha == 0.0f) {
        return;
    }

    compassSizeDvar = cg_hudCompassSize;
    sizeScale = compassSizeDvar->current.value;
    height = rect->h;
    CL_DrawStretchPic(
        rect->x,
        rect->y - height * (sizeScale - 1.0f),
        rect->w * sizeScale,
        height * sizeScale,
        rect->horzAlign,
        rect->vertAlign,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        color,
        material);
}

static inline __attribute__((always_inline)) void CG_CopyColor(vec_t out[4], const vec_t *in, float alpha)
{
    if (in) {
        out[0] = in[0];
        out[1] = in[1];
        out[2] = in[2];
        out[3] = in[3] * alpha;
    } else {
        out[0] = 1.0f;
        out[1] = 1.0f;
        out[2] = 1.0f;
        out[3] = alpha;
    }
}

static inline __attribute__((always_inline)) int CG_FadeDurationMsec(const dvar_t *fadeDvar)
{
    if (!fadeDvar) {
        return 0;
    }
    return (int)floorf(fadeDvar->current.value * 1000.0f + 0.5f);
}

static inline __attribute__((always_inline)) float CG_HudFadeAlpha(const dvar_t *fadeDvar, int displayStartTime)
{
    return CG_FadeHudMenu_inl(fadeDvar, displayStartTime, CG_FadeDurationMsec(fadeDvar));
}

static inline __attribute__((always_inline)) qboolean CG_PlayerOwnsWeapon(const playerState_t *ps, int weaponIndex)
{
    if (!ps || weaponIndex <= 0 || weaponIndex >= BG_GetNumWeapons()) {
        return 0;
    }
    return (ps->weapons[weaponIndex >> 5] & (1 << (weaponIndex & 31))) != 0;
}

static inline __attribute__((always_inline)) int CG_GetDrawWeaponIndex(void)
{
    playerState_t *ps;
    int weaponIndex;

    if (!cg) {
        return 0;
    }

    ps = &cg->predictedPlayerState;
    weaponIndex = cg->weaponSelect;
    if (!CG_PlayerOwnsWeapon(ps, weaponIndex)) {
        weaponIndex = ps->weapon;
    }

    if (weaponIndex <= 0 || weaponIndex >= BG_GetNumWeapons()) {
        return 0;
    }
    return weaponIndex;
}

static inline __attribute__((always_inline)) weaponInfo_t *CG_GetDrawWeaponInfo(int weaponIndex)
{
    weaponInfo_t *weapons;

    if (weaponIndex <= 0 || weaponIndex >= BG_GetNumWeapons()) {
        return 0;
    }

    weapons = *(weaponInfo_t **)imp_cg_weapons;
    if (!weapons) {
        return 0;
    }
    return &weapons[weaponIndex];
}

static inline __attribute__((always_inline)) void CG_DrawTextRightAligned(const char *text, const rectDef_t *rect, struct Font_s *font, float scale, const vec_t *color, int textStyle)
{
    int width;

    if (!text || !*text || !rect || !font) {
        return;
    }

    width = UI_TextWidth(text, 0, font, scale);
    UI_DrawText(text, 0x7fffffff, font, rect->x + rect->w - (float)width, rect->y,
                rect->horzAlign, rect->vertAlign, scale, color, textStyle);
}

static inline __attribute__((always_inline)) void CG_DrawCenteredTextInRect(const char *text, const rectDef_t *rect, struct Font_s *font, float scale, const vec_t *color, int textStyle)
{
    int width;

    if (!text || !*text || !rect || !font) {
        return;
    }

    width = UI_TextWidth(text, 0, font, scale);
    UI_DrawText(text, 0x7fffffff, font, rect->x + (rect->w - (float)width) * 0.5f, rect->y + rect->h,
                rect->horzAlign, rect->vertAlign, scale, color, textStyle);
}

static inline __attribute__((always_inline)) void CG_DrawHudPic(const rectDef_t *rect, MaterialHandle material, const vec_t *color)
{
    if (!rect || !material || !color) {
        return;
    }

    UI_DrawHandlePic(rect->x, rect->y, rect->w, rect->h, rect->horzAlign, rect->vertAlign, color, material);
}

static void CG_DrawPlayerStance(const rectDef_t *rect, vec_t *color, struct Font_s *font, float scale, int textStyle)
{
    const playerState_t *ps;
    vec_t drawColor[4];
    int stance;
    float alpha;
    float compassScale;
    float x;
    const char *hintText;

    (void)font;
    (void)scale;
    (void)textStyle;

    if (!rect) {
        return;
    }

    if (!cg || !cgs) {
        return;
    }

    ps = &cg->predictedPlayerState;
    stance = 0;
    if (ps->pm_flags & 1) {
        stance = 2;
    } else if (ps->pm_flags & 2) {
        stance = 1;
    }

    if (cg->lastStance != stance) {
        cg->lastStance = stance;
        cg->lastStanceChangeTime = cg->time;
        cg->lastStanceFlashTime = cg->time + 1000;
    }

    alpha = CG_HudFadeAlpha(hud_fade_stance, cg->stanceFadeTime);
    if (alpha <= 0.0f) {
        return;
    }

    CG_CopyColor(drawColor, color, alpha);
    if ((ps->pm_flags & 1) && cg->proneBlockedEndTime > cg->time) {
        drawColor[0] = 1.0f;
        drawColor[1] = 0.18f;
        drawColor[2] = 0.01f;
    }

    compassScale = cg_hudCompassSize->current.value;
    x = rect->x + (compassScale - 1.0f) * cgs->compassWidth * 0.7f;
    UI_DrawHandlePic(x, rect->y, rect->w, rect->h, rect->horzAlign, rect->vertAlign, drawColor, cgs->media.stanceMaterials[stance]);

    if (cg_hudStanceHintPrints->current.enabled && cg->lastStanceChangeTime + 3000 > cg->time) {
        hintText = stance == 2 ? "PLATFORM_STANCEHINT_PRONE" : (stance == 1 ? "PLATFORM_STANCEHINT_CROUCH" : "PLATFORM_STANCEHINT_STAND");
        UI_DrawText(UI_SafeTranslateString(hintText), 0x7fffffff, font, x + rect->w + 4.0f, rect->y + rect->h * 0.5f,
                    rect->horzAlign, rect->vertAlign, scale, drawColor, textStyle);
    }
}

static void CG_DrawPlayerAmmoValue(const rectDef_t *rect, struct Font_s *font, float scale, vec_t *color, MaterialHandle material, int textStyle, int type)
{
    playerState_t *ps;
    vec_t drawColor[4];
    vec_t lowColor[4] = { 0.89f, 0.18f, 0.01f, 1.0f };
    char ammoString[32];
    char clipString[32];
    int weaponIndex;
    int clipIndex;
    int reserveAmmo;
    int clipAmmo;
    qboolean drawReserve;
    qboolean drawClip;
    qboolean lowAmmo;
    qboolean lowClip;
    float alpha;
    float pipeX;

    (void)material;

    if (!rect || !font) {
        return;
    }

    if (!cg || !cg->predictedPlayerState.weapon) {
        return;
    }

    ps = &cg->predictedPlayerState;
    alpha = CG_HudFadeAlpha(hud_fade_ammodisplay, cg->ammoFadeTime);
    if (alpha <= 0.0f) {
        return;
    }

    weaponIndex = CG_GetDrawWeaponIndex();
    if (!weaponIndex) {
        return;
    }

    reserveAmmo = BG_GetTotalAmmoReserve(ps, weaponIndex);
    if (reserveAmmo < 0) {
        reserveAmmo = 0;
    }
    if (reserveAmmo > 999) {
        reserveAmmo = 999;
    }

    drawClip = 0;
    clipAmmo = 0;
    if (!BG_WeaponIsClipOnly(weaponIndex)) {
        clipIndex = BG_ClipForWeapon(weaponIndex);
        clipAmmo = ps->ammoclip[clipIndex];
        if (clipAmmo >= 0) {
            if (clipAmmo > 999) {
                clipAmmo = 999;
            }
            drawClip = 1;
        }
    }

    drawReserve = type == 0 || !drawClip;
    lowAmmo = CG_CheckPlayerForLowAmmo();
    lowClip = CG_CheckPlayerForLowClip();

    CG_CopyColor(drawColor, color, alpha);
    lowColor[3] = alpha;

    if (drawClip) {
        Com_sprintf(clipString, sizeof(clipString), "%2i", clipAmmo);
        UI_DrawText(clipString, 0x7fffffff, font, rect->x, rect->y, rect->horzAlign, rect->vertAlign,
                    scale, lowClip ? lowColor : drawColor, textStyle);
    }

    if (!drawReserve) {
        return;
    }

    Com_sprintf(ammoString, sizeof(ammoString), "%3i", reserveAmmo);
    if (drawClip) {
        CG_DrawTextRightAligned(ammoString, rect, font, scale, lowAmmo ? lowColor : drawColor, textStyle);
        pipeX = rect->x + (rect->w - (float)UI_TextWidth("|", 0, font, scale)) * 0.5f - 5.0f;
        UI_DrawText("|", 0x7fffffff, font, pipeX, rect->y, rect->horzAlign, rect->vertAlign,
                    scale, lowAmmo ? lowColor : drawColor, textStyle);
    } else {
        CG_DrawCenteredTextInRect(ammoString, rect, font, scale, lowAmmo ? lowColor : drawColor, textStyle);
    }
}

static void CG_DrawPlayerAmmoBackdrop(const rectDef_t *rect, MaterialHandle material, vec_t *color)
{
    vec_t drawColor[4];
    float alpha;

    if (!cg || !cg->predictedPlayerState.weapon) {
        return;
    }

    alpha = CG_HudFadeAlpha(hud_fade_ammodisplay, cg->ammoFadeTime);
    if (alpha <= 0.0f) {
        return;
    }

    CG_CopyColor(drawColor, color, alpha);
    CG_DrawHudPic(rect, material, drawColor);
}

static inline __attribute__((always_inline)) const char *CG_GetWeaponDisplayText(int weaponIndex)
{
    weaponInfo_t *weapInfo;
    WeaponDef *weapDef;

    weapInfo = CG_GetDrawWeaponInfo(weaponIndex);
    if (!weapInfo) {
        return 0;
    }

    weapDef = (WeaponDef *)BG_GetWeaponDef(weaponIndex);
    if (weapDef && weapDef->szModeName && weapDef->szModeName[0] && weapInfo->pszTranslatedModename && weapInfo->pszTranslatedModename[0]) {
        return va("%s / %s", weapInfo->pszTranslatedDisplayName, weapInfo->pszTranslatedModename);
    }
    return weapInfo->pszTranslatedDisplayName;
}

static inline __attribute__((always_inline)) float CG_WeaponSelectFadeAlpha(const cg_t *cg)
{
    float *fadeColor;

    if (CG_AreHudMenusHidden_inl() || !hud_enable->current.enabled) {
        return 0.0f;
    }

    fadeColor = CG_FadeColor(cg->weaponSelectTime, 1800, 700);
    if (!fadeColor) {
        return 0.0f;
    }
    return fadeColor[3];
}

static void CG_DrawPlayerWeaponName(const rectDef_t *rect, struct Font_s *font, float scale, vec_t *color, int textStyle)
{
    vec_t drawColor[4];
    const char *text;
    int weaponIndex;
    float alpha;

    if (!rect || !font) {
        return;
    }

    if (!cg) {
        return;
    }

    weaponIndex = CG_GetDrawWeaponIndex();
    text = CG_GetWeaponDisplayText(weaponIndex);
    if (!text || !*text) {
        return;
    }

    alpha = CG_WeaponSelectFadeAlpha(cg);
    if (alpha <= 0.0f) {
        return;
    }

    CG_CopyColor(drawColor, color, alpha);
    CG_DrawTextRightAligned(text, rect, font, scale, drawColor, textStyle);
}

static void CG_DrawPlayerWeaponNameBack(const rectDef_t *rect, struct Font_s *font, float scale, vec_t *color, MaterialHandle material)
{
    vec_t drawColor[4];
    rectDef_t backRect;
    const char *text;
    int weaponIndex;
    int textWidth;
    float alpha;

    if (!rect || !font || !material) {
        return;
    }

    if (!cg) {
        return;
    }

    weaponIndex = CG_GetDrawWeaponIndex();
    text = CG_GetWeaponDisplayText(weaponIndex);
    if (!text || !*text) {
        return;
    }

    alpha = CG_WeaponSelectFadeAlpha(cg);
    if (alpha <= 0.0f) {
        return;
    }

    textWidth = UI_TextWidth(text, 0, font, scale);
    backRect = *rect;
    backRect.w = (float)textWidth + 36.0f;
    backRect.x = rect->x + rect->w - backRect.w;

    CG_CopyColor(drawColor, color, alpha);
    CG_DrawHudPic(&backRect, material, drawColor);
}

static void CG_DrawPlayerWeaponModeIcon(const rectDef_t *rect, MaterialHandle fallbackMaterial, vec_t *color)
{
    weaponInfo_t *weapInfo;
    vec_t drawColor[4];
    int weaponIndex;
    float alpha;
    MaterialHandle material;

    if (!cg) {
        return;
    }

    alpha = CG_HudFadeAlpha(hud_fade_ammodisplay, cg->weaponSelectTime);
    if (alpha <= 0.0f) {
        return;
    }

    weaponIndex = CG_GetDrawWeaponIndex();
    weapInfo = CG_GetDrawWeaponInfo(weaponIndex);
    material = weapInfo && weapInfo->hModeIcon ? weapInfo->hModeIcon : fallbackMaterial;
    CG_CopyColor(drawColor, color, alpha);
    CG_DrawHudPic(rect, material, drawColor);
}

static void CG_DrawPlayerHealthBar(const rectDef_t *rect, MaterialHandle material, vec_t *color)
{
    rectDef_t barRect;
    vec_t drawColor[4];
    float health;
    float alpha;

    if (!cg_drawHealth->current.enabled) {
        return;
    }

    if (!cg) {
        return;
    }

    health = CG_CalcPlayerHealth();
    if (health <= 0.0f) {
        return;
    }

    alpha = CG_HudFadeAlpha(hud_fade_healthbar, cg->healthFadeTime);
    if (alpha <= 0.0f) {
        return;
    }

    barRect = *rect;
    barRect.w *= health;
    CG_CopyColor(drawColor, color, alpha);
    CG_DrawHudPic(&barRect, material, drawColor);
}

static void CG_DrawPlayerHealthBack(const rectDef_t *rect, MaterialHandle material, vec_t *color)
{
    vec_t drawColor[4];
    float alpha;

    if (!cg_drawHealth->current.enabled) {
        return;
    }

    if (!cg) {
        return;
    }

    alpha = CG_HudFadeAlpha(hud_fade_healthbar, cg->healthFadeTime);
    if (alpha <= 0.0f) {
        return;
    }

    CG_CopyColor(drawColor, color, alpha);
    CG_DrawHudPic(rect, material, drawColor);
}

static inline __attribute__((always_inline)) void CG_DrawLowHealthOverlay(const rectDef_t *rect, MaterialHandle material, vec_t *color)
{
    vec_t drawColor[4];
    float health;
    float alpha;
    int elapsed;

    if (!cg_blood->current.enabled) {
        return;
    }

    if (!cg) {
        return;
    }

    health = CG_CalcPlayerHealth();
    CG_PulseLowHealthOverlay(health);

    alpha = cg->healthOverlayToAlpha;
    if (cg->healthOverlayPulseDuration > 0) {
        elapsed = cg->time - cg->healthOverlayPulseTime;
        if (elapsed >= 0 && elapsed < cg->healthOverlayPulseDuration) {
            float frac = (float)elapsed / (float)cg->healthOverlayPulseDuration;
            alpha = cg->healthOverlayFromAlpha + (cg->healthOverlayToAlpha - cg->healthOverlayFromAlpha) * frac;
        }
    }

    if (alpha <= 0.0f) {
        return;
    }
    if (alpha > 1.0f) {
        alpha = 1.0f;
    }

    CG_CopyColor(drawColor, color, alpha);
    CG_DrawHudPic(rect, material, drawColor);
}

static inline __attribute__((always_inline)) void CG_DrawPlayerCompass(const rectDef_t *rect, MaterialHandle material, vec_t *color)
{
    rectDef_t compassRect;
    vec_t drawColor[4];
    float scale;
    float alpha;

    if (!cg) {
        return;
    }

    alpha = CG_HudFadeAlpha(hud_fade_compass, cg->compassFadeTime);
    if (alpha <= 0.0f) {
        return;
    }

    scale = cg_hudCompassSize->current.value;
    compassRect = *rect;
    compassRect.y -= rect->h * (scale - 1.0f);
    compassRect.w *= scale;
    compassRect.h *= scale;

    CG_UpdateCompassOrientation();
    CG_CopyColor(drawColor, color, alpha);
    CG_DrawRotatedPic(compassRect.x, compassRect.y, compassRect.w, compassRect.h,
                      compassRect.horzAlign, compassRect.vertAlign, cg->compassYaw, drawColor, material);
}

static inline __attribute__((always_inline)) void CG_DrawPlayerCompassFriendliesOwnerdraw(rectDef_t *rect, MaterialHandle material, vec_t *color)
{
    vec_t drawColor[4];
    float alpha;

    if (!cg) {
        return;
    }

    alpha = CG_HudFadeAlpha(hud_fade_compass, cg->compassFadeTime);
    if (alpha <= 0.0f) {
        return;
    }

    CG_CopyColor(drawColor, color, alpha);
    CG_DrawCompassFriendlies(rect, material, drawColor);
}

void CG_OwnerDraw(float x, float y, float w, float h, int horzAlign, int vertAlign, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, struct Font_s *font, float scale, vec_t *color, MaterialHandle material, int textStyle)
{
    rectDef_t rect;
    vec_t drawColor[4];
    const char *text;

    (void)ownerDrawFlags;
    (void)align;
    (void)special;

    rect.x = x + text_x;
    rect.y = y + text_y;
    rect.w = w;
    rect.h = h;
    rect.horzAlign = horzAlign;
    rect.vertAlign = vertAlign;

    switch (ownerDraw) {
    case CG_PLAYER_AMMO_VALUE:
        CG_DrawPlayerAmmoValue(&rect, font, scale, color, material, textStyle, 0);
        break;
    case CG_PLAYER_AMMOCLIP_VALUE:
        CG_DrawPlayerAmmoValue(&rect, font, scale, color, material, textStyle, 1);
        break;
    case CG_PLAYER_AMMO_BACKDROP:
        CG_DrawPlayerAmmoBackdrop(&rect, material, color);
        break;
    case CG_PLAYER_STANCE:
        CG_DrawPlayerStance(&rect, color, font, scale, textStyle);
        break;
    case CG_BLUE_SCORE:
        CG_DrawScore(1, &rect, font, scale, color, material, textStyle);
        break;
    case CG_RED_SCORE:
        CG_DrawScore(0, &rect, font, scale, color, material, textStyle);
        break;
    case CG_GAME_TYPE:
        UI_DrawText(CG_GameTypeString(), 0x7fffffff, font, rect.x, rect.y + rect.h,
                    rect.horzAlign, rect.vertAlign, scale, color, textStyle);
        break;
    case CG_KILLER:
        CG_DrawCenteredTextInRect(CG_GetKillerText(), &rect, font, scale, color, textStyle);
        break;
    case CG_CAPFRAGLIMIT:
        if (cgs->teamScores[0] != -9999) {
            text = va("%2i", cgs->teamScores[0]);
            UI_DrawText(text, 0x7fffffff, font, rect.x, rect.y, rect.horzAlign, rect.vertAlign, scale, color, textStyle);
        }
        break;
    case CG_1STPLACE:
        if (cgs->teamScores[1] != -9999) {
            text = va("%2i", cgs->teamScores[1]);
            UI_DrawText(text, 0x7fffffff, font, rect.x, rect.y, rect.horzAlign, rect.vertAlign, scale, color, textStyle);
        }
        break;
    case CG_HOLD_BREATH_HINT:
        CG_DrawHoldBreathHint(&rect, font, scale, textStyle);
        break;
    case CG_CURSORHINT:
        CG_DrawCursorhint(&rect, font, scale, textStyle);
        break;
    case CG_PLAYER_BAR_HEALTH:
        CG_DrawPlayerHealthBar(&rect, material, color);
        break;
    case CG_MANTLE_HINT:
        CG_DrawMantleHint(&rect, font, scale, textStyle);
        break;
    case CG_PLAYER_WEAPON_NAME:
        CG_DrawPlayerWeaponName(&rect, font, scale, color, textStyle);
        break;
    case CG_PLAYER_WEAPON_NAME_BACK:
        CG_DrawPlayerWeaponNameBack(&rect, font, scale, color, material);
        break;
    case CG_PLAYER_WEAPON_MODE_ICON:
        CG_DrawPlayerWeaponModeIcon(&rect, material, color);
        break;
    case CG_PLAYER_COMPASS:
        CG_DrawPlayerCompass(&rect, material, color);
        break;
    case CG_PLAYER_COMPASS_BACK:
        CG_DrawPlayerCompassBack(&rect, material, color);
        break;
    case CG_PLAYER_COMPASS_POINTERS:
        CG_UpdateCompPointerOrientation();
        break;
    case CG_PLAYER_COMPASS_FRIENDS:
        CG_DrawPlayerCompassFriendliesOwnerdraw(&rect, material, color);
        break;
    case CG_DRAW_SHADER:
        CG_DrawHudPic(&rect, material, color);
        break;
    case CG_PLAYER_BAR_HEALTH_BACK:
        CG_DrawPlayerHealthBack(&rect, material, color);
        break;
    case CG_OFFHAND_WEAPON_ICON_FRAG:
        CG_DrawOffHandIcon(&rect, scale, color, material, 1);
        break;
    case CG_OFFHAND_WEAPON_ICON_SMOKE:
        CG_DrawOffHandIcon(&rect, scale, color, material, 2);
        break;
    case CG_OFFHAND_WEAPON_AMMO_FRAG:
        CG_DrawOffHandAmmo(&rect, font, scale, color, textStyle, 1);
        break;
    case CG_OFFHAND_WEAPON_AMMO_SMOKE:
        CG_DrawOffHandAmmo(&rect, font, scale, color, textStyle, 2);
        break;
    case CG_OFFHAND_WEAPON_NAME_FRAG:
        CG_DrawOffHandName(&rect, font, scale, color, textStyle, 1);
        break;
    case CG_OFFHAND_WEAPON_NAME_SMOKE:
        CG_DrawOffHandName(&rect, font, scale, color, textStyle, 2);
        break;
    case CG_OFFHAND_WEAPON_SELECT_FRAG:
        CG_DrawOffHandHighlight(&rect, scale, color, material, 1);
        break;
    case CG_OFFHAND_WEAPON_SELECT_SMOKE:
        CG_DrawOffHandHighlight(&rect, scale, color, material, 2);
        break;
    case CG_PLAYER_LOW_HEALTH_OVERLAY:
        CG_DrawLowHealthOverlay(&rect, material, color);
        break;
    case CG_TEAM_COLOR:
        CG_CopyColor(drawColor, color, color ? color[3] : 1.0f);
        CG_DrawTeamBackground(rect.x, rect.y, rect.w, rect.h, drawColor[3], 0);
        break;
    default:
        break;
    }
}

const unsigned char __ZZ17CG_DrawMantleHintPK9rectDef_sP6Font_sfiE5color[16] = { 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f };

const unsigned char __ZZ21CG_DrawHoldBreathHintPK9rectDef_sP6Font_sfiE5color[896] = {
    0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xb6, 0xad, 0x18, 0x00, 0xe0, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xda, 0xae, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xf8, 0xbf, 0x18, 0x00, 0x4d, 0xbf, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0x78, 0xbf, 0x18, 0x00, 0xe6, 0xbe, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0x5c, 0xc1, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0x26, 0xc0, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xe9, 0xc0, 0x18, 0x00, 0xe9, 0xba, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0x73, 0xbb, 0x18, 0x00, 0x83, 0xbb, 0x18, 0x00, 0xac, 0xbb, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xe2, 0xaa, 0x18, 0x00, 0xc4, 0xbb, 0x18, 0x00,
    0xed, 0xbb, 0x18, 0x00, 0x5f, 0xbd, 0x18, 0x00, 0x0a, 0xaf, 0x18, 0x00, 0xe8, 0xaf, 0x18, 0x00,
    0x4d, 0xb1, 0x18, 0x00, 0x6b, 0xb1, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0x8c, 0xb5, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0xaa, 0xb5, 0x18, 0x00, 0xff, 0xb5, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00, 0xce, 0xb8, 0x18, 0x00,
    0xff, 0xb8, 0x18, 0x00, 0x17, 0xb9, 0x18, 0x00, 0x4f, 0xb9, 0x18, 0x00, 0x59, 0xb9, 0x18, 0x00,
    0x91, 0xb9, 0x18, 0x00, 0x9b, 0xb9, 0x18, 0x00, 0xcc, 0xb9, 0x18, 0x00, 0xab, 0xad, 0x18, 0x00,
    0xd6, 0xb9, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0b, 0xc8, 0x18, 0x00, 0x2d, 0xc8, 0x18, 0x00, 0xa1, 0xc8, 0x18, 0x00, 0x87, 0xc8, 0x18, 0x00,
    0x75, 0xc8, 0x18, 0x00, 0x09, 0xc8, 0x18, 0x00, 0xf1, 0xc7, 0x18, 0x00, 0x47, 0xc8, 0x18, 0x00,
    0xef, 0xc8, 0x18, 0x00, 0x01, 0xc9, 0x18, 0x00, 0x75, 0xc9, 0x18, 0x00, 0x5b, 0xc9, 0x18, 0x00,
    0x49, 0xc9, 0x18, 0x00, 0xed, 0xc8, 0x18, 0x00, 0xdd, 0xc8, 0x18, 0x00, 0x1b, 0xc9, 0x18, 0x00,
    0xd8, 0xcc, 0x18, 0x00, 0xda, 0xcd, 0x18, 0x00, 0xb9, 0xcd, 0x18, 0x00, 0x8c, 0xcd, 0x18, 0x00,
    0x67, 0xcd, 0x18, 0x00, 0x66, 0xcc, 0x18, 0x00, 0x3a, 0xcd, 0x18, 0x00, 0x2a, 0xcc, 0x18, 0x00,
    0xb3, 0xcc, 0x18, 0x00, 0x0d, 0xcd, 0x18, 0x00, 0x7e, 0xce, 0x18, 0x00, 0x51, 0xce, 0x18, 0x00,
    0x2c, 0xce, 0x18, 0x00, 0xae, 0xcc, 0x18, 0x00, 0x07, 0xce, 0x18, 0x00, 0x72, 0xcc, 0x18, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xc7, 0xe3, 0x19, 0x00, 0xdc, 0xe3, 0x19, 0x00, 0xb5, 0xe3, 0x19, 0x00, 0xf1, 0xe3, 0x19, 0x00,
    0x13, 0xe4, 0x19, 0x00, 0xb5, 0xe3, 0x19, 0x00, 0xb5, 0xe3, 0x19, 0x00, 0xb5, 0xe3, 0x19, 0x00,
    0xab, 0xe3, 0x19, 0x00, 0xb7, 0xe7, 0x19, 0x00, 0xcc, 0xe7, 0x19, 0x00, 0x7e, 0xe7, 0x19, 0x00,
    0xeb, 0xe7, 0x19, 0x00, 0x1a, 0xe8, 0x19, 0x00, 0x7e, 0xe7, 0x19, 0x00, 0x7e, 0xe7, 0x19, 0x00,
    0x7e, 0xe7, 0x19, 0x00, 0x6f, 0xe8, 0x19, 0x00, 0xda, 0xe8, 0x19, 0x00, 0x94, 0xe9, 0x19, 0x00,
    0x81, 0xe9, 0x19, 0x00, 0x5e, 0xe9, 0x19, 0x00, 0x4b, 0xe9, 0x19, 0x00, 0x2a, 0xe9, 0x19, 0x00,
    0x09, 0xe9, 0x19, 0x00, 0xed, 0xe8, 0x19, 0x00, 0xba, 0xe8, 0x19, 0x00, 0x8a, 0xf0, 0x19, 0x00,
    0x11, 0xf1, 0x19, 0x00, 0x84, 0xf0, 0x19, 0x00, 0xee, 0xf0, 0x19, 0x00, 0xc2, 0xf0, 0x19, 0x00,
    0x4a, 0xf0, 0x19, 0x00, 0xa1, 0xf0, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
