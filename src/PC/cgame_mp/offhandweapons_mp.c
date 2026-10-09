#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include <string.h>
extern scr_const_t scr_const;

extern const dvar_t *Dvar_RegisterFloat(const char *name, float defaultValue, float min, float max, int flags);
extern int BG_GetFirstAvailableOffhand(void *ps, int weaponType);
extern int BG_GetFirstEquippedOffhand(void *ps, int weaponType);
extern float CG_FadeHudMenu(void *hud, int val, int time);
extern void UI_DrawText(const char *text, int maxChars, void *font, float x, float y, int horzAlign, int vertAlign, float scale, const float *color, int textStyle);
extern const char *UI_SafeTranslateString(const char *str);
extern int CG_PlayEntitySoundAlias(int entNum, int soundAlias);
extern int CG_PlaySoundAlias(int entitynum, const vec_t *origin, snd_alias_list_t *aliasList);
extern qboolean CG_DObjGetWorldTagPos(void *cent, void *dobj, unsigned short tag, float *origin);
extern int CG_DObjGetViewModelTagPos(void *dobj, unsigned short tag, float *origin);
extern void BG_EvaluateTrajectory(void *trajectory, int time, float *result);
extern void *BG_GetWeaponDef(int weaponIndex);
extern int BG_GetNumWeapons(void);
extern int BG_ClipForWeapon(int weaponIndex);
extern void *Com_GetClientDObj(int clientNum, int lod);
extern void CG_MenuShowNotify(int type);
extern void UI_DrawHandlePic(float x, float y, float w, float h, int horzAlign, int vertAlign, const float *color, MaterialHandle material);
extern float floorf(float x);
extern float cosf(float x);
extern char *va(const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);

extern byte *cg_weapons;
extern void *imp_cg_weapons;
extern const dvar_t *hud_fade_offhand;

/* offhandStrings: migrated from the ILP32 data blob to typed C (re-lays-out per
 * target). Blob extent was 12 B of pointers + 20 B inter-symbol padding (next
 * symbol hintStrings distinct; indexed [weaponType] in 0..2). [0] is the empty
 * literal (literals32.c); the other two are in common.c. (x64 port Stage 2.) */
const char *offhandStrings[3] = {
    "",
    "WEAPON_FRAGGRENADE",
    "WEAPON_SMOKEGRENADE",
};
static const dvar_t *hud_flash_time_offhand;
static const dvar_t *hud_flash_period_offhand;

COD2_ASSERT_FIELD(struct dvar_s, current,      0x08);
COD2_ASSERT_FIELD(WeaponDef,     offhandClass, 0x84);

void CG_OffhandRegisterDvars(void);
void CG_DrawOffHandName(rectDef_s *rect, struct Font_s *font, float scale, vec_t *color, int textStyle, int weaponType);
void CG_PrepOffHand(entityState_t *ent, int event, int eventParam);
void CG_UseOffHand(centity_t *cent, int event, int eventParam);
void CG_SetEquippedOffHand(int offHandIndex);
void CG_SwitchOffHandCmd(void);
void CG_DrawOffHandIcon(rectDef_s *rect, float scale, vec_t *color, MaterialHandle material, int weaponType);
void CG_DrawOffHandHighlight(rectDef_s *rect, float scale, vec_t *color, MaterialHandle material, int weaponType);
void CG_DrawOffHandAmmo(rectDef_s *rect, struct Font_s *font, float scale, vec_t *color, int textStyle, int weaponType);

void CG_OffhandRegisterDvars(void)
{
    hud_flash_time_offhand = Dvar_RegisterFloat("hud_flash_time_offhand", 2.0f, 0.0f, 30.0f, 0x1001);
    hud_flash_period_offhand = Dvar_RegisterFloat("hud_flash_period_offhand", 0.5f, 0.0f, 30.0f, 0x1001);
}

void CG_DrawOffHandName(rectDef_s *rect, struct Font_s *font, float scale, vec_t *color, int textStyle, int weaponType)
{
    const dvar_t *hud;
    float fade;
    float drawColor[4];
    const char *text;
    int weapon;
    int time;

    if (cg->predictedPlayerState.pm_type > 5)
        return;

    weapon = BG_GetFirstAvailableOffhand((void *)&cg->predictedPlayerState, weaponType);
    if (weapon == 0) {
        weapon = BG_GetFirstEquippedOffhand((void *)&cg->predictedPlayerState, weaponType);
        if (weapon == 0)
            return;
    }

    hud = hud_fade_offhand;

    time = (int)floorf(hud->current.value * 1000.0f + 0.5f);
    fade = CG_FadeHudMenu((void *)hud, cg->offhandFadeTime, time);

    if (fade == 0.0f)
        return;

    *(int *)&drawColor[0] = *(int *)&color[0];
    *(int *)&drawColor[1] = *(int *)&color[1];
    *(int *)&drawColor[2] = *(int *)&color[2];

    text = UI_SafeTranslateString(offhandStrings[weaponType]);

    UI_DrawText(text, 0x7fffffff, font,
                rect->x, rect->y,
                rect->horzAlign, rect->vertAlign,
                scale, drawColor, textStyle);
}

void CG_PrepOffHand(entityState_t *ent, int event, int eventParam)
{
    weaponInfo_s *wi;
    int soundAlias;

    wi = &((*(weaponInfo_s **)imp_cg_weapons))[eventParam];
    soundAlias = (int)(intptr_t)wi->pullbackSound;

    if (soundAlias != 0) {
        CG_PlayEntitySoundAlias(ent->number, soundAlias);
    }
}

void CG_UseOffHand(centity_t *cent, int event, int eventParam)
{
    weaponInfo_s *wi;
    int soundAlias;
    int clientNum;
    float origin[3];
    void *dobj;

    wi = &((*(weaponInfo_s **)imp_cg_weapons))[eventParam];
    soundAlias = (int)(intptr_t)wi->flashSound;

    if (soundAlias == 0)
        return;

    clientNum = cent->nextState.number;

    if (clientNum == cg->snap->ps.clientNum) {

        void *viewModel = wi->viewModelDObj;
        if (viewModel != NULL) {
            if (CG_DObjGetViewModelTagPos(viewModel, scr_const.tag_flash, origin)) {
                goto play_sound;
            }
        }
    } else {

        dobj = Com_GetClientDObj(clientNum, cent->localClientNum);
        if (dobj != NULL) {
            if (CG_DObjGetWorldTagPos(cent, dobj, scr_const.tag_flash, origin)) {
                goto play_sound;
            }
        }
    }

    BG_EvaluateTrajectory((void *)&cent->nextState.pos, cg->time, origin);

play_sound:
    CG_PlaySoundAlias(cent->nextState.number, origin, (int)(intptr_t)wi->flashSound);
}

void CG_SetEquippedOffHand(int offHandIndex)
{
    cg->equippedOffHand = offHandIndex;
    CG_MenuShowNotify(4);
}

void CG_SwitchOffHandCmd(void)
{
    int currentWeapon;
    void *weapDef;
    int newWeapon;

    currentWeapon = cg->equippedOffHand;

    if (currentWeapon == 0)
        return;

    weapDef = BG_GetWeaponDef(currentWeapon);
    newWeapon = BG_GetFirstAvailableOffhand((void *)&cg->predictedPlayerState, ((WeaponDef *)weapDef)->offhandClass);

    if (newWeapon == 0)
        return;

    cg->equippedOffHand = newWeapon;
    CG_MenuShowNotify(4);
}

void CG_DrawOffHandIcon(rectDef_s *rect, float scale, vec_t *color, MaterialHandle material, int weaponType)
{
    const dvar_t *hud;
    float fade;
    float drawColor[4];
    int weapon;
    int time;
    MaterialHandle iconMaterial;
    void *weapDef;

    if (cg->predictedPlayerState.pm_type > 5)
        return;

    weapon = BG_GetFirstAvailableOffhand((void *)&cg->predictedPlayerState, weaponType);
    if (weapon == 0) {
        weapon = BG_GetFirstEquippedOffhand((void *)&cg->predictedPlayerState, weaponType);
        if (weapon == 0)
            return;
    }

    hud = hud_fade_offhand;

    time = (int)floorf(hud->current.value * 1000.0f + 0.5f);
    fade = CG_FadeHudMenu((void *)hud, cg->offhandFadeTime, time);

    if (fade == 0.0f)
        return;

    *(int *)&drawColor[0] = *(int *)&color[0];
    *(int *)&drawColor[1] = *(int *)&color[1];
    *(int *)&drawColor[2] = *(int *)&color[2];

    if (cg->equippedOffHand != 0) {
        weapDef = BG_GetWeaponDef(cg->equippedOffHand);
        if (((WeaponDef *)weapDef)->offhandClass == weaponType) {
            weapon = cg->equippedOffHand;
            if (weapon == 0)
                goto find_weapon;
            goto draw_icon;
        }
    }

find_weapon:
    weapon = BG_GetFirstAvailableOffhand((void *)&cg->predictedPlayerState, weaponType);
    if (weapon == 0) {
        weapon = BG_GetFirstEquippedOffhand((void *)&cg->predictedPlayerState, weaponType);
        if (weapon == 0)
            return;
    }

draw_icon:
    iconMaterial = ((*(weaponInfo_s **)imp_cg_weapons))[weapon].hHudIcon;

    UI_DrawHandlePic(rect->x, rect->y, rect->w,
                     rect->h, rect->horzAlign, rect->vertAlign,
                     drawColor, iconMaterial);
}

void CG_DrawOffHandHighlight(rectDef_s *rect, float scale, vec_t *color, MaterialHandle material, int weaponType)
{
    const dvar_t *hud;
    float fade;
    float flashColor[4];
    int weapon;
    int time;
    void *weapDef;
    int weapCount;
    int ammoCount;
    int i;
    int clip;
    float timeSinceF;
    float flashTime;
    float angle;
    float pulse;

    if (cg->predictedPlayerState.pm_type > 5)
        return;

    weapon = BG_GetFirstAvailableOffhand((void *)&cg->predictedPlayerState, weaponType);
    if (weapon == 0) {
        weapon = BG_GetFirstEquippedOffhand((void *)&cg->predictedPlayerState, weaponType);
        if (weapon == 0)
            return;
    }

    if (cg->equippedOffHand == 0)
        return;

    hud = hud_fade_offhand;

    time = (int)floorf(hud->current.value * 1000.0f + 0.5f);
    fade = CG_FadeHudMenu((void *)hud, cg->offhandFadeTime, time);

    if (fade == 0.0f)
        return;

    weapDef = BG_GetWeaponDef(cg->equippedOffHand);
    if (((WeaponDef *)weapDef)->offhandClass != weaponType)
        return;

    weapCount = BG_GetNumWeapons();
    ammoCount = 0;
    for (i = 1; i <= weapCount; i++) {
        if (!((cg->predictedPlayerState.weapons[i >> 5] >> (i & 0x1f)) & 1))
            continue;
        weapDef = BG_GetWeaponDef(i);
        if (((WeaponDef *)weapDef)->offhandClass != weaponType)
            continue;
        clip = BG_ClipForWeapon(i);
        ammoCount += cg->predictedPlayerState.ammoclip[clip];
    }

    if (ammoCount == 0) {
        flashColor[0] = 0.89f;
        flashColor[1] = 0.18f;
        flashColor[2] = 0.01f;
    } else {
        *(int *)&flashColor[0] = *(int *)&color[0];
        *(int *)&flashColor[1] = *(int *)&color[1];
        *(int *)&flashColor[2] = *(int *)&color[2];
    }

    flashColor[3] = fade;

    timeSinceF = (float)(cg->time - cg->offhandFlashTime) / 1000.0f;
    flashTime = hud_flash_time_offhand->current.value;

    if (flashTime > timeSinceF) {
        angle = timeSinceF * 6.2831855f / hud_flash_period_offhand->current.value;
        pulse = cosf(angle) * 0.5f + 0.5f;
        flashColor[3] = pulse * fade;
    }

    UI_DrawHandlePic(rect->x, rect->y, rect->w,
                     rect->h, rect->horzAlign, rect->vertAlign,
                     flashColor, material);
}

void CG_DrawOffHandAmmo(rectDef_s *rect, struct Font_s *font, float scale, vec_t *color, int textStyle, int weaponType)
{
    const dvar_t *hud;
    float fade;
    float drawColor[4];
    int weapon;
    int time;
    int weapCount;
    int ammoCount;
    int i;
    int clip;
    char *text;
    void *weapDef;

    if (cg->predictedPlayerState.pm_type > 5)
        return;

    weapon = BG_GetFirstAvailableOffhand((void *)&cg->predictedPlayerState, weaponType);
    if (weapon == 0) {
        weapon = BG_GetFirstEquippedOffhand((void *)&cg->predictedPlayerState, weaponType);
        if (weapon == 0)
            return;
    }

    hud = hud_fade_offhand;

    time = (int)floorf(hud->current.value * 1000.0f + 0.5f);
    fade = CG_FadeHudMenu((void *)hud, cg->offhandFadeTime, time);

    if (fade == 0.0f)
        return;

    weapCount = BG_GetNumWeapons();
    ammoCount = 0;
    for (i = 1; i <= weapCount; i++) {
        if (!((cg->predictedPlayerState.weapons[i >> 5] >> (i & 0x1f)) & 1))
            continue;
        weapDef = BG_GetWeaponDef(i);
        if (((WeaponDef *)weapDef)->offhandClass != weaponType)
            continue;
        clip = BG_ClipForWeapon(i);
        ammoCount += cg->predictedPlayerState.ammoclip[clip];
    }

    text = va("%i", ammoCount);

    if (ammoCount == 0) {
        drawColor[0] = 0.89f;
        drawColor[1] = 0.18f;
        drawColor[2] = 0.01f;
    } else {
        *(int *)&drawColor[0] = *(int *)&color[0];
        *(int *)&drawColor[1] = *(int *)&color[1];
        *(int *)&drawColor[2] = *(int *)&color[2];
    }

    UI_DrawText(text, 0x7fffffff, font,
                rect->x, rect->y,
                rect->horzAlign, rect->vertAlign,
                scale, drawColor, textStyle);
}
