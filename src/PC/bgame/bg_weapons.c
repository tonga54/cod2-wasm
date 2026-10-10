#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include "cod2_grenade.h"

/* Unused bit inside the existing 27-bit replicated pm_flags field. The latch
 * must be part of player state so prediction replays the same trigger history
 * as the server. Presses during recovery are consumed rather than queued. */
#define PM_WEAPON_FLAG_TRIGGER_HELD 0x00040000

extern int bg_iNumWeapons;
extern WeaponDef *bg_weaponDefs[128];
static WeaponDef *bg_weapAmmoTypes[128];
static int bg_iNumAmmoTypes;
static WeaponDef *bg_sharedAmmoCaps[128];
static int bg_iNumSharedAmmoCaps;
static WeaponDef *bg_weapClips[128];
static int bg_iNumWeapClips;
static const char bg_emptyString[] = "";

void BG_ShutdownWeaponDefFiles(void);
WeaponDef *BG_GetWeaponDef(int iWeapon);
extern void *BG_LoadDefaultWeaponDef(void);
extern WeaponDef *BG_LoadWeaponDefInternal(const char *folder, const char *name);
extern void BG_LoadPlayerAnimTypes(void);
extern void BG_InitWeaponStrings(void);
extern int I_stricmp(const char *, const char *);
extern void Com_Error(int code, const char *fmt, ...);
extern void Com_DPrintf(const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern int BG_AnimScriptEvent(playerState_t *ps, int event, int isContinue, int force);
extern void BG_UpdateConditionValue(int client, int condition, int value, qboolean checkConversion);
extern void PM_SetProneMovementOverride(playerState_t *ps);
extern qboolean Mantle_IsWeaponInactive(playerState_t *ps);
extern void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t *ps);
extern const dvar_t *bg_aimSpreadMoveSpeedThreshold;
extern const dvar_t *player_scopeExitOnDamage;
extern const dvar_t *player_adsExitDelay;
extern const dvar_t *player_breath_fire_delay;
extern const dvar_t *player_breath_gasp_lerp;
extern const dvar_t *player_breath_gasp_scale;
extern const dvar_t *player_breath_gasp_time;
extern const dvar_t *player_breath_hold_lerp;
extern const dvar_t *player_breath_hold_time;
extern const dvar_t *player_toggleBinoculars;
extern float AngleSubtract(float a1, float a2);
extern void AnglesSubtract(const vec_t *v1, const vec_t *v2, vec_t *v3);
extern float DiffTrack(float tgt, float cur, float rate, float deltaTime);
extern float DiffTrackAngle(float tgt, float cur, float rate, float deltaTime, float f, float granularity, float epsilon);
extern float GetLeanFraction(float fFrac);
extern float randomf(void);
extern float sinf(float);
extern double sin(double);
void BG_ClearWeaponDef(void);
int BG_GetNumWeapons(void);
int BG_GetAmmoTypeMax(int iAmmoIndex);
int BG_GetAmmoClipSize(int iClipIndex);
int BG_FindWeaponIndexForName(const char *name);
WeaponDef *BG_LoadWeaponDef(const char *folder, const char *name);
int BG_SetupWeaponDef(WeaponDef *weapDef, void (*regWeap)(int));
qboolean BG_IsAnyEmptyPrimaryWeaponSlot(const playerState_t *ps);
Bool PM_IsBinocularsADS(const playerState_t *ps);
void PM_ExitAimDownSight(playerState_t *ps);
float BG_GetBobCycle(const playerState_t *ps);
float BG_GetVerticalBobFactor(const playerState_t *ps, float cycle, float speed, float maxAmp);
float BG_GetHorizontalBobFactor(const playerState_t *ps, float cycle, float speed, float maxAmp);
static void BG_SetupClipIndexes(int weapIndex);
static void BG_SetupSharedAmmoIndexes(int weapIndex);
static void BG_SetupAmmoIndexes(int weapIndex);
static void BG_SetupWeaponItem(int weapIndex, WeaponDef *weapDef);
qboolean BG_IsAimDownSightWeapon(int iWeapon);
Bool BG_DoesWeaponRequireSlot(int weaponIndex);
Bool BG_DoesWeaponNeedSlot(int weapIndex);
int BG_GetEmptySlotForWeapon(const playerState_t *pPS, int iWeaponIndex);
int BG_GetStackSlotForWeapon(const playerState_t *pPS, int iWeaponIndex, weapSlot_t preferedSlot);
qboolean BG_IsPlayerWeaponAnAlt(int iWeaponIndex, int iAltIndex);
void BG_GetSpreadForWeapon(const playerState_t *ps, int weaponIndex, float *minSpread, float *maxSpread);
int BG_ClipForWeapon(int weapon);
int BG_AmmoForWeapon(int weapon);
qboolean BG_WeaponIsClipOnly(int weapon);
int PM_WeaponAmmoAvailable(playerState_t *ps);
int BG_WeaponAmmo(const playerState_t *ps, int weapon);
int BG_IsPlayerWeaponInSlot(const playerState_t *pPS, int iWeaponIndex, qboolean bAnyMode);
int BG_GetTotalAmmoReserve(const playerState_t *ps, int weaponIndex);
static void __attribute_regparm__(1) PM_ReloadClip(playerState_t *ps);
static qboolean __attribute_regparm__(1) PM_Weapon_AllowReload(playerState_t *ps);
int BG_GetFirstEquippedOffhand(const playerState_t *ps, int offhandClass);
void PM_AdjustAimSpreadScale(pmove_t *pm, pml_t *pml);
static qboolean BG_CalculateWeaponPosition_GunRecoil_SingleAngle(vec_t *offset, vec_t *speed,
                                                                 float fTimeStep, float fOfsCap, float fGunKickAccel, float fGunKickSpeedMax,
                                                                 float fGunKickSpeedDecay, float fGunKickStaticDecay);
static void __attribute_regparm__(1) PM_Weapon_ReloadDelayedAction(playerState_t *ps);
int BG_GetViewmodelWeaponIndex(const playerState_t *ps);
void BG_WeaponFireRecoil(const playerState_t *ps, vec_t *vGunSpeed, vec_t *kickAVel);
void BG_CalculateWeaponPosition_Sway(playerState_t *ps, vec_t *swayViewAngles, vec_t *swayOffset, vec_t *swayAngles, float ssSwayScale, int frametime);
int BG_GetFirstAvailableOffhand(const playerState_t *ps, int offhandClass);
int BG_GetMaxPickupableAmmo(const playerState_t *ps, int weaponIndex);
void PM_UpdateAimDownSightFlag(pmove_t *pm, pml_t *pml);
Bool BG_IsWeaponValid(const playerState_t *ps, int weaponIndex);
qboolean BG_TakePlayerWeapon(playerState_t *pPS, int iWeaponIndex);
static void __attribute_regparm__(2) PM_BeginWeaponChange(playerState_t *ps, int newweapon);
static void __attribute_regparm__(1) PM_SetReloadingState(playerState_t *ps);
static void __attribute_regparm__(1) PM_BeginWeaponReload(playerState_t *ps);
void PM_UpdateAimDownSightLerp(pmove_t *pm, pml_t *pml);
void PM_ResetWeaponState(playerState_t *ps);
qboolean PM_InteruptWeaponWithProneMove(playerState_t *ps);
void PM_Weapon(pmove_t *pm, pml_t *pml);
int BG_GetWeaponIndexForName(const char *name, BG_RegisterWeapon regWeap);
void BG_FillInAmmoItems(BG_RegisterWeapon regWeap);
void CG_SetWeaponDefToDefaultWeapon(int iWeapon);
void BG_CalculateWeaponAngles(weaponState_t *ws, vec_t *angles);
void BG_CalculateViewAngles(viewState_t *vs, vec_t *angles);

void BG_ShutdownWeaponDefFiles(void)
{
    bg_iNumWeapons = 0;
}

WeaponDef *BG_GetWeaponDef(int iWeapon)
{
    return bg_weaponDefs[iWeapon];
}

void BG_ClearWeaponDef(void)
{
    WeaponDef *defaultDef;
    gitem_t *itemList;
    int i;

    defaultDef = (WeaponDef *)BG_LoadDefaultWeaponDef();

    bg_weaponDefs[0] = defaultDef;

    bg_weapAmmoTypes[0] = defaultDef;
    bg_iNumAmmoTypes = 1;

    bg_sharedAmmoCaps[0] = defaultDef;
    bg_iNumSharedAmmoCaps = 1;

    bg_weapClips[0] = defaultDef;
    bg_iNumWeapClips = 1;

    itemList = (gitem_t *)((void *)imp_bg_itemlist);
    for (i = 1; i <= 128; i++) {
        itemList[i].giType = 0;
    }

    BG_LoadPlayerAnimTypes();
    BG_InitWeaponStrings();
}

int BG_GetNumWeapons(void)
{
    return bg_iNumWeapons;
}

int BG_GetAmmoTypeMax(int iAmmoIndex)
{
    return bg_weapAmmoTypes[iAmmoIndex]->iMaxAmmo;
}

int BG_GetAmmoClipSize(int iClipIndex)
{
    return bg_weapClips[iClipIndex]->iClipSize;
}

int BG_FindWeaponIndexForName(const char *name)
{
    extern int bg_iNumWeapons;
    extern WeaponDef *bg_weaponDefs[128];
    extern int I_stricmp(const char *, const char *);
    int weapIndex;
    for (weapIndex = 1; weapIndex <= bg_iNumWeapons; weapIndex++) {
        const char *defName = *(const char **)bg_weaponDefs[weapIndex];
        if (I_stricmp(name, defName) == 0)
            return weapIndex;
    }
    return 0;
}

qboolean BG_IsAnyEmptyPrimaryWeaponSlot(const playerState_t *ps)
{
    return ps->weaponslots[1] == 0 || ps->weaponslots[2] == 0;
}

static BM_ALWAYS_INLINE Bool PM_IsBinocularsADS_core(const playerState_t *ps)
{
    int val = ps->weaponstate - 0x13;
    return (unsigned int)val <= 1;
}

Bool PM_IsBinocularsADS(const playerState_t *ps)
{
    return PM_IsBinocularsADS_core(ps);
}

extern void PM_AddEvent(playerState_t *ps, int newEvent);

void PM_ExitAimDownSight(playerState_t *ps)
{
    PM_AddEvent(ps, 0x95);
    ps->pm_flags &= ~0x40;
}

float BG_GetBobCycle(const playerState_t *ps)
{
    float val = (float)(*(byte *)&ps->bobCycle) / 255.0f;
    return (float)((double)val * 3.141592653589793 * 2.0 + 6.283185307179586);
}

static BM_ALWAYS_INLINE float BG_GetBobAmplitude(const playerState_t *ps, float speed, float maxAmp)
{
    const dvar_t *dvar;
    float amplitude;

    if (ps->viewHeightTarget == 11)
        dvar = *(const dvar_t **)imp_bg_bobAmplitudeProne;
    else if (ps->viewHeightTarget == 40)
        dvar = *(const dvar_t **)imp_bg_bobAmplitudeDucked;
    else
        dvar = *(const dvar_t **)imp_bg_bobAmplitudeStanding;

    amplitude = speed * dvar->current.value;
    if (amplitude > maxAmp)
        amplitude = maxAmp;
    return amplitude;
}

float BG_GetVerticalBobFactor(const playerState_t *ps, float cycle, float speed, float maxAmp)
{
    float amplitude = BG_GetBobAmplitude(ps, speed, maxAmp);
    float bob = sinf(cycle + cycle);

    bob += (float)sin((double)(cycle * 4.0f) + 1.5707963267948966) * 0.2f;
    return amplitude * (bob * 0.75f);
}

float BG_GetHorizontalBobFactor(const playerState_t *ps, float cycle, float speed, float maxAmp)
{
    return BG_GetBobAmplitude(ps, speed, maxAmp) * sinf(cycle);
}

static void BG_SetupClipIndexes(int weapIndex)
{
    WeaponDef *weapDef = bg_weaponDefs[weapIndex];
    int iIndex;

    for (iIndex = 0; iIndex < bg_iNumWeapClips; iIndex++) {
        WeaponDef *clipDef = bg_weapClips[iIndex];

        if (strcmp(clipDef->szClipName, weapDef->szClipName) == 0) {
            weapDef->iClipIndex = iIndex;

            if (clipDef->iClipSize != weapDef->iClipSize && iIndex != 0 && weapIndex > 1) {
                int j;

                for (j = 1; j < weapIndex; j++) {
                    WeaponDef *otherWeapDef = bg_weaponDefs[j];

                    if (I_stricmp(clipDef->szClipName, otherWeapDef->szClipName) == 0 &&
                        clipDef->iClipSize == otherWeapDef->iClipSize) {
                        Com_Error(1, "\x15"
                                     "Clip Size mismatch for \"%s\" clip: '%s\" set it to %i, but \"%s\" already set it to %i.\n",
                                  weapDef->szAmmoName, weapDef->szInternalName,
                                  weapDef->iClipSize, otherWeapDef->szInternalName, otherWeapDef->iClipSize);
                    }
                }
            }

            return;
        }
    }

    bg_weapClips[bg_iNumWeapClips] = weapDef;
    weapDef->iClipIndex = bg_iNumWeapClips;
    bg_iNumWeapClips++;
}

static void BG_SetupSharedAmmoIndexes(int weapIndex)
{
    WeaponDef *weapDef = bg_weaponDefs[weapIndex];
    int iIndex;

    weapDef->iSharedAmmoCapIndex = -1;
    if (!weapDef->szSharedAmmoCapName[0])
        return;

    Com_DPrintf("%s: %s\n", weapDef->szInternalName, weapDef->szSharedAmmoCapName);

    for (iIndex = 0; iIndex < bg_iNumSharedAmmoCaps; iIndex++) {
        WeaponDef *sharedAmmoCap = bg_sharedAmmoCaps[iIndex];

        if (I_stricmp(sharedAmmoCap->szSharedAmmoCapName, weapDef->szSharedAmmoCapName) == 0) {
            weapDef->iSharedAmmoCapIndex = iIndex;

            if (sharedAmmoCap->iSharedAmmoCap != weapDef->iSharedAmmoCap && iIndex != 0 && weapIndex > 1) {
                int j;

                for (j = 1; j < weapIndex; j++) {
                    WeaponDef *otherWeapDef = bg_weaponDefs[j];

                    if (I_stricmp(sharedAmmoCap->szSharedAmmoCapName, otherWeapDef->szSharedAmmoCapName) == 0 &&
                        sharedAmmoCap->iSharedAmmoCap == otherWeapDef->iSharedAmmoCap) {
                        Com_Error(1, "\x15Shared ammo cap mismatch for \"%s\" shared ammo cap: '%s\" set it to %i, but \"%s\" already set it to %i.\n", weapDef->szSharedAmmoCapName, weapDef->szInternalName,
                                  weapDef->iSharedAmmoCap, otherWeapDef->szInternalName, otherWeapDef->iSharedAmmoCap);
                    }
                }
            }

            return;
        }
    }

    bg_sharedAmmoCaps[bg_iNumSharedAmmoCaps] = weapDef;
    weapDef->iSharedAmmoCapIndex = bg_iNumSharedAmmoCaps;
    bg_iNumSharedAmmoCaps++;
}

static void BG_SetupAmmoIndexes(int weapIndex)
{
    WeaponDef *weapDef = bg_weaponDefs[weapIndex];
    int iIndex;

    for (iIndex = 0; iIndex < bg_iNumAmmoTypes; iIndex++) {
        WeaponDef *ammoDef = bg_weapAmmoTypes[iIndex];

        if (strcmp(ammoDef->szAmmoName, weapDef->szAmmoName) == 0) {
            weapDef->iAmmoIndex = iIndex;

            if (ammoDef->iMaxAmmo != weapDef->iMaxAmmo && iIndex != 0 && weapIndex > 1) {
                int j;

                for (j = 1; j < weapIndex; j++) {
                    WeaponDef *otherWeapDef = bg_weaponDefs[j];

                    if (I_stricmp(ammoDef->szAmmoName, otherWeapDef->szAmmoName) == 0 &&
                        ammoDef->iMaxAmmo == otherWeapDef->iMaxAmmo) {
                        Com_Error(1, "\x15Max ammo mismatch for \"%s\" ammo: '%s\" set it to %i, but \"%s\" already set it to %i.\n", weapDef->szAmmoName, weapDef->szInternalName,
                                  weapDef->iMaxAmmo, otherWeapDef->szInternalName, otherWeapDef->iMaxAmmo);
                    }
                }
            }

            return;
        }
    }

    bg_weapAmmoTypes[bg_iNumAmmoTypes] = weapDef;
    weapDef->iAmmoIndex = bg_iNumAmmoTypes;
    bg_iNumAmmoTypes++;
}

static const char *BG_WeaponItemString(const char *string)
{
    return string ? string : bg_emptyString;
}

static void BG_SetupWeaponItem(int weapIndex, WeaponDef *weapDef)
{
    int *numItems = (int *)imp_bg_numItems;
    gitem_t *item = &((gitem_t *)imp_bg_itemlist)[weapIndex];

    item->classname = (char *)BG_WeaponItemString(weapDef->szInternalName);
    item->pickup_sound = (char *)BG_WeaponItemString(weapDef->szPickupSound);
    item->world_model[0] = (char *)BG_WeaponItemString(weapDef->szWorldModel);
    item->world_model[1] = (char *)bg_emptyString;
    item->icon = (char *)BG_WeaponItemString(weapDef->szHudIcon);
    item->pickup_name = (char *)BG_WeaponItemString(weapDef->szDisplayName);
    item->quantity = weapDef->iStartAmmo;
    item->giType = IT_WEAPON;
    item->giTag = weapIndex;
    item->giAmmoIndex = weapDef->iAmmoIndex;
    item->giClipIndex = weapDef->iClipIndex;

    if (*numItems <= weapIndex)
        *numItems = weapIndex + 1;
}

qboolean BG_IsAimDownSightWeapon(int iWeapon)
{
    return bg_weaponDefs[iWeapon]->bADSPositionInfo;
}

Bool BG_DoesWeaponRequireSlot(int weaponIndex)
{
    int val = bg_weaponDefs[weaponIndex]->weapSlot;
    return (unsigned int)(val - 1) <= 1;
}

Bool BG_DoesWeaponNeedSlot(int weapIndex)
{

    const WeaponDef *weapDef = bg_weaponDefs[weapIndex];
    if (weapDef->weapClass == 9)
        return 0;
    if (weapDef->offhandClass != 0)
        return 0;
    return 1;
}

int BG_GetEmptySlotForWeapon(const playerState_t *pPS, int iWeaponIndex)
{
    const WeaponDef *weapDef = bg_weaponDefs[iWeaponIndex];

    if ((unsigned int)(weapDef->weapSlot - 1) > 1)
        return 0;
    if (!pPS->weaponslots[1])
        return 1;
    if (!pPS->weaponslots[2])
        return 2;
    return 0;
}

int BG_GetStackSlotForWeapon(const playerState_t *pPS, int iWeaponIndex, weapSlot_t preferedSlot)
{
    const WeaponDef *weapDef = bg_weaponDefs[iWeaponIndex];

    if (!weapDef->bSlotStackable || (unsigned int)(weapDef->weapSlot - 1) > 1)
        return 0;

    if ((unsigned int)(preferedSlot - 1) <= 1) {
        int weapon = pPS->weaponslots[preferedSlot];

        if (!weapon || bg_weaponDefs[weapon]->bSlotStackable)
            return preferedSlot;
    }

    if (!pPS->weaponslots[1] || bg_weaponDefs[(int)pPS->weaponslots[1]]->bSlotStackable)
        return 1;
    if (!pPS->weaponslots[2] || bg_weaponDefs[(int)pPS->weaponslots[2]]->bSlotStackable)
        return 2;
    return 0;
}

qboolean BG_IsPlayerWeaponAnAlt(int iWeaponIndex, int iAltIndex)
{
    int altIndex = bg_weaponDefs[iWeaponIndex]->iAltWeaponIndex;

    while (altIndex) {
        if (altIndex == iAltIndex)
            return 1;
        if (altIndex == iWeaponIndex)
            break;
        altIndex = bg_weaponDefs[altIndex]->iAltWeaponIndex;
    }

    return 0;
}

void BG_GetSpreadForWeapon(const playerState_t *ps, int weaponIndex, float *minSpread, float *maxSpread)
{
    const WeaponDef *weapDef = bg_weaponDefs[weaponIndex];
    float frac;

    if (ps->viewHeightCurrent > 40.0f) {
        frac = (ps->viewHeightCurrent - 40.0f) / 20.0f;
        *minSpread = weapDef->fHipSpreadDuckedMin + (weapDef->fHipSpreadStandMin - weapDef->fHipSpreadDuckedMin) * frac;
        *maxSpread = weapDef->hipSpreadDuckedMax + (weapDef->hipSpreadStandMax - weapDef->hipSpreadDuckedMax) * frac;
    } else {
        frac = (ps->viewHeightCurrent - 11.0f) / 29.0f;
        *minSpread = weapDef->fHipSpreadProneMin + (weapDef->fHipSpreadDuckedMin - weapDef->fHipSpreadProneMin) * frac;
        *maxSpread = weapDef->hipSpreadProneMax + (weapDef->hipSpreadDuckedMax - weapDef->hipSpreadProneMax) * frac;
    }
}

int BG_ClipForWeapon(int weapon)
{
    return bg_weaponDefs[weapon]->iClipIndex;
}

int BG_AmmoForWeapon(int weapon)
{
    return bg_weaponDefs[weapon]->iAmmoIndex;
}

qboolean BG_WeaponIsClipOnly(int weapon)
{
    return bg_weaponDefs[weapon]->bClipOnly;
}

int PM_WeaponAmmoAvailable(playerState_t *ps)
{
    int weapon = ps->weapon;
    int clipIndex = bg_weaponDefs[weapon]->iClipIndex;
    return ps->ammoclip[clipIndex];
}

int BG_WeaponAmmo(const playerState_t *ps, int weapon)
{
    const WeaponDef *weapDef = bg_weaponDefs[weapon];
    int ammoIndex = weapDef->iAmmoIndex;
    int clipIndex = weapDef->iClipIndex;

    return ps->ammo[ammoIndex] + ps->ammoclip[clipIndex];
}

static BM_ALWAYS_INLINE int BG_IsPlayerWeaponInSlot_core(const playerState_t *pPS, int iWeaponIndex, qboolean bAnyMode)
{
    int curWeaponIndex;

    if (!(pPS->weapons[iWeaponIndex >> 5] & (1 << (iWeaponIndex & 0x1f))))
        return 0;

    curWeaponIndex = iWeaponIndex;
    do {
        const WeaponDef *weapDef = bg_weaponDefs[curWeaponIndex];

        if ((unsigned int)(weapDef->weapSlot - 1) > 1)
            return 0;
        if ((int)pPS->weaponslots[1] == curWeaponIndex)
            return 1;
        if ((int)pPS->weaponslots[2] == curWeaponIndex)
            return 2;

        if (!bAnyMode)
            break;
        if (weapDef->iAltWeaponIndex)
            curWeaponIndex = weapDef->iAltWeaponIndex;
    } while (curWeaponIndex != iWeaponIndex);

    return 0;
}

int BG_IsPlayerWeaponInSlot(const playerState_t *pPS, int iWeaponIndex, qboolean bAnyMode)
{
    return BG_IsPlayerWeaponInSlot_core(pPS, iWeaponIndex, bAnyMode);
}

int BG_GetTotalAmmoReserve(const playerState_t *ps, int weaponIndex)
{
    const WeaponDef *weapDef = bg_weaponDefs[weaponIndex];
    int ammoIndex = weapDef->iAmmoIndex;
    int clipIndex = weapDef->iClipIndex;
    int ammoCounted[128];
    int clipCounted[128];
    int ammo;
    int i;

    memset(ammoCounted, 0, sizeof(ammoCounted));
    memset(clipCounted, 0, sizeof(clipCounted));

    if (weapDef->iSharedAmmoCapIndex < 0) {
        if (weapDef->bClipOnly)
            return ps->ammoclip[clipIndex];
        return ps->ammo[ammoIndex];
    }

    ammo = 0;
    for (i = 1; i <= bg_iNumWeapons; i++) {
        const WeaponDef *otherWeapDef;

        if (!(ps->weapons[i >> 5] & (1 << (i & 0x1f))))
            continue;

        otherWeapDef = bg_weaponDefs[i];
        if (otherWeapDef->iSharedAmmoCapIndex != weapDef->iSharedAmmoCapIndex)
            continue;

        if (otherWeapDef->bClipOnly) {
            clipIndex = otherWeapDef->iClipIndex;
            if (clipCounted[clipIndex])
                continue;

            clipCounted[clipIndex] = 1;
            ammo += ps->ammoclip[clipIndex];
        } else {
            ammoIndex = otherWeapDef->iAmmoIndex;
            if (ammoCounted[ammoIndex])
                continue;

            ammoCounted[ammoIndex] = 1;
            ammo += ps->ammo[ammoIndex];
        }
    }

    return ammo;
}

static void __attribute_regparm__(1) PM_ReloadClip(playerState_t *ps)
{
    WeaponDef *weapDef = bg_weaponDefs[ps->weapon];
    int reloadState = ps->weaponstate - 7;
    int ammoIndex;
    int clipIndex;
    int ammoReserve;
    int clipAmmo;
    int clipSize;
    int ammoToAdd;
    int reloadAdd;

    if ((unsigned int)reloadState <= 1 && !weapDef->iReloadStartAdd)
        return;

    ammoIndex = weapDef->iAmmoIndex;
    clipIndex = weapDef->iClipIndex;
    ammoReserve = ps->ammo[ammoIndex];
    clipAmmo = ps->ammoclip[clipIndex];
    clipSize = bg_weapClips[clipIndex]->iClipSize;
    ammoToAdd = clipSize - clipAmmo;
    if (ammoReserve < ammoToAdd)
        ammoToAdd = ammoReserve;

    if ((unsigned int)reloadState <= 1)
        reloadAdd = weapDef->iReloadStartAdd;
    else
        reloadAdd = weapDef->iReloadAmmoAdd;

    if (reloadAdd && clipSize > reloadAdd && ammoToAdd > reloadAdd)
        ammoToAdd = reloadAdd;

    if (ammoToAdd) {
        ps->ammo[ammoIndex] = ammoReserve - ammoToAdd;
        ps->ammoclip[clipIndex] = clipAmmo + ammoToAdd;
    }
}

static qboolean __attribute_regparm__(1) PM_Weapon_AllowReload(playerState_t *ps)
{
    WeaponDef *weapDef = bg_weaponDefs[ps->weapon];
    int ammoReserve = ps->ammo[weapDef->iAmmoIndex];
    int clipIndex = weapDef->iClipIndex;
    int clipAmmo;
    int clipSize;
    int reloadAdd;

    if (!ammoReserve)
        return 0;

    clipAmmo = ps->ammoclip[clipIndex];
    clipSize = bg_weapClips[clipIndex]->iClipSize;
    if (clipAmmo >= clipSize)
        return 0;

    if (!weapDef->bNoPartialReload)
        return 1;

    reloadAdd = weapDef->iReloadAmmoAdd;
    if (!reloadAdd || reloadAdd >= clipSize)
        return clipAmmo == 0;

    return reloadAdd <= clipSize - clipAmmo;
}

int BG_GetFirstEquippedOffhand(const playerState_t *ps, int offhandClass)
{
    int weaponIndex;

    for (weaponIndex = 1; weaponIndex <= bg_iNumWeapons; weaponIndex++) {
        if (bg_weaponDefs[weaponIndex]->offhandClass != offhandClass)
            continue;
        if (ps->weapons[weaponIndex >> 5] & (1 << (weaponIndex & 0x1f)))
            return weaponIndex;
    }

    return 0;
}

void PM_AdjustAimSpreadScale(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    WeaponDef *weapDef = bg_weaponDefs[ps->weapon];
    float spreadChange;

    if (weapDef->fHipSpreadDecayRate == 0.0f) {
        spreadChange = -1.0f;
    } else {
        float decrease = weapDef->fHipSpreadDecayRate;
        float increase = 0.0f;

        if (ps->groundEntityNum == 0x3ff && ps->pm_type != 1) {
            decrease *= 0.5f;
        } else if (ps->eFlags & 8) {
            decrease *= weapDef->fHipSpreadProneDecay;
        } else if (ps->eFlags & 4) {
            decrease *= weapDef->fHipSpreadDuckedDecay;
        }

        decrease *= pml->frametime;

        if (ps->fWeaponPosFrac == 1.0f) {
            spreadChange = -decrease;
        } else {
            if (weapDef->fHipSpreadTurnAdd != 0.0f) {
                int i;

                for (i = 0; i < 2; i++) {
                    float oldAngle = (float)pm->oldcmd.angles[i] * 0.0054931640625f;
                    float newAngle = (float)pm->cmd.angles[i] * 0.0054931640625f;

                    increase += __builtin_fabsf(AngleSubtract(oldAngle, newAngle)) *
                                0.009999999776482582f * weapDef->fHipSpreadTurnAdd /
                                pml->frametime;
                }
            }

            if (weapDef->fHipSpreadMoveAdd != 0.0f &&
                (pm->cmd.forwardmove || pm->cmd.rightmove)) {
                float speedSq = ps->velocity[0] * ps->velocity[0] +
                                ps->velocity[1] * ps->velocity[1];
                float threshold = bg_aimSpreadMoveSpeedThreshold->current.value;

                if (speedSq > threshold * threshold)
                    increase += weapDef->fHipSpreadMoveAdd;
            }

            if (ps->groundEntityNum == 0x3ff && ps->pm_type != 1)
                increase += 1.2799999713897705f + 1.2799999713897705f;

            spreadChange = increase * pml->frametime - decrease;
        }
    }

    ps->aimSpreadScale += spreadChange * 255.0f;
    if (ps->aimSpreadScale < 0.0f)
        ps->aimSpreadScale = 0.0f;
    if (ps->aimSpreadScale > 255.0f)
        ps->aimSpreadScale = 255.0f;
}

static qboolean BG_CalculateWeaponPosition_GunRecoil_SingleAngle(vec_t *offset, vec_t *speed,
                                                                 float fTimeStep, float fOfsCap, float fGunKickAccel, float fGunKickSpeedMax,
                                                                 float fGunKickSpeedDecay, float fGunKickStaticDecay)
{
    float newSpeed;

    if (__builtin_fabsf(*offset) < 0.25f && __builtin_fabsf(*speed) < 1.0f) {
        *offset = 0.0f;
        *speed = 0.0f;
        return 1;
    }

    *offset += *speed * fTimeStep;
    if (*offset > fOfsCap) {
        *offset = fOfsCap;
        if (*speed > 0.0f)
            *speed = 0.0f;
    } else if (*offset < -fOfsCap) {
        *offset = -fOfsCap;
        if (*speed < 0.0f)
            *speed = 0.0f;
    }

    if (*offset > 0.0f)
        *speed -= fGunKickAccel * fTimeStep;
    else if (*offset < 0.0f)
        *speed += fGunKickAccel * fTimeStep;

    newSpeed = *speed - *speed * fGunKickSpeedDecay * fTimeStep;
    *speed = newSpeed;

    if (newSpeed > 0.0f) {
        newSpeed -= fGunKickStaticDecay * fTimeStep;
        if (newSpeed < 0.0f)
            newSpeed = 0.0f;
    } else if (newSpeed < 0.0f) {
        newSpeed += fGunKickStaticDecay * fTimeStep;
        if (newSpeed > 0.0f)
            newSpeed = 0.0f;
    }

    if (newSpeed > fGunKickSpeedMax)
        newSpeed = fGunKickSpeedMax;
    if (newSpeed < -fGunKickSpeedMax)
        newSpeed = -fGunKickSpeedMax;

    *speed = newSpeed;
    return 0;
}

static void __attribute_regparm__(1) PM_Weapon_ReloadDelayedAction(playerState_t *ps)
{
    int iWeapon = ps->weapon;
    WeaponDef *weapDef = bg_weaponDefs[iWeapon];
    int weaponBit = 1 << (iWeapon & 0x1f);
    int weaponWord = iWeapon >> 5;
    int reloadState;
    int reloadTime;
    int boltTime;

    if (!weapDef->bBoltAction || !(ps->weaponrechamber[weaponWord] & weaponBit)) {
        PM_ReloadClip(ps);
        return;
    }

    ps->weaponrechamber[weaponWord] &= ~weaponBit;
    PM_AddEvent(ps, 0xa2);

    reloadState = ps->weaponstate - 7;
    if ((unsigned int)reloadState <= 1 && !weapDef->iReloadStartAddTime)
        return;

    if (!ps->weaponTime) {
        PM_ReloadClip(ps);
        return;
    }

    if ((unsigned int)reloadState <= 1) {
        reloadTime = weapDef->iReloadStartAddTime;
        if (reloadTime >= weapDef->iReloadStartTime)
            reloadTime = weapDef->iReloadStartTime;
    } else {
        if (ps->ammoclip[weapDef->iClipIndex] || weapDef->weapType)
            reloadTime = weapDef->iReloadTime;
        else
            reloadTime = weapDef->iReloadEmptyTime;

        if (weapDef->iReloadAddTime && reloadTime > weapDef->iReloadAddTime)
            reloadTime = weapDef->iReloadAddTime;
    }

    boltTime = weapDef->iRechamberBoltTime;
    if (reloadTime <= boltTime)
        boltTime = 1;

    reloadTime -= boltTime;
    if (reloadTime <= 0) {
        PM_ReloadClip(ps);
        return;
    }

    ps->weaponDelay = reloadTime;
}

static BM_ALWAYS_INLINE int BG_GetViewmodelWeaponIndex_core(const playerState_t *ps)
{
    int weaponIndex;

    if (ps->pm_flags & 0x20000) {
        for (weaponIndex = 1; weaponIndex <= bg_iNumWeapons; weaponIndex++) {
            if (bg_weaponDefs[weaponIndex]->weapType == 3)
                return (unsigned char)weaponIndex;
        }

        return 0;
    }

    if (ps->pm_flags & 0x10)
        return ps->offHandIndex;

    return ps->weapon;
}

int BG_GetViewmodelWeaponIndex(const playerState_t *ps)
{
    return BG_GetViewmodelWeaponIndex_core(ps);
}

static BM_ALWAYS_INLINE float BG_RandomBetween(float min, float max)
{
    return min + (max - min) * randomf();
}

void BG_WeaponFireRecoil(const playerState_t *ps, vec_t *vGunSpeed, vec_t *kickAVel)
{
    WeaponDef *weapDef = bg_weaponDefs[BG_GetViewmodelWeaponIndex_core(ps)];
    float fPosLerp = ps->fWeaponPosFrac;
    float fReducePercent = 1.0f;
    float pitch;
    float yaw;

    if (ps->weaponRestrictKickTime > 0) {
        if (fPosLerp == 1.0f)
            fReducePercent = 0.009999999776482582f * weapDef->adsGunKickReducedKickPercent;
        else
            fReducePercent = 0.009999999776482582f * weapDef->hipGunKickReducedKickPercent;
    }

    if (fPosLerp == 1.0f) {
        pitch = BG_RandomBetween(weapDef->fAdsViewKickPitchMin, weapDef->fAdsViewKickPitchMax);
        yaw = BG_RandomBetween(weapDef->fAdsViewKickYawMin, weapDef->fAdsViewKickYawMax);
    } else {
        pitch = BG_RandomBetween(weapDef->fHipViewKickPitchMin, weapDef->fHipViewKickPitchMax);
        yaw = BG_RandomBetween(weapDef->fHipViewKickYawMin, weapDef->fHipViewKickYawMax);
    }

    yaw *= fReducePercent;
    pitch *= fReducePercent;

    kickAVel[0] = -pitch;
    kickAVel[1] = yaw;
    kickAVel[2] = yaw * -0.5f;

    if (fPosLerp > 0.0f) {
        pitch = BG_RandomBetween(weapDef->fAdsGunKickPitchMin, weapDef->fAdsGunKickPitchMax);
        yaw = BG_RandomBetween(weapDef->fAdsGunKickYawMin, weapDef->fAdsGunKickYawMax);
    } else {
        pitch = BG_RandomBetween(weapDef->fHipGunKickPitchMin, weapDef->fHipGunKickPitchMax);
        yaw = BG_RandomBetween(weapDef->fHipGunKickYawMin, weapDef->fHipGunKickYawMax);
    }

    vGunSpeed[0] += pitch * fReducePercent;
    vGunSpeed[1] += yaw * fReducePercent;
}

void BG_CalculateWeaponPosition_Sway(playerState_t *ps, vec_t *swayViewAngles, vec_t *swayOffset, vec_t *swayAngles, float ssSwayScale, int frametime)
{
    WeaponDef *weapDef;
    float f;
    float dt;
    float swayMaxAngle;
    float swayLerpSpeed;
    float swayPitchScale;
    float swayYawScale;
    float swayHorizScale;
    float swayVertScale;
    vec3_t deltaAngles;
    float scale;

    if (!frametime)
        return;

    weapDef = bg_weaponDefs[BG_GetViewmodelWeaponIndex(ps)];
    f = ps->fWeaponPosFrac;
    dt = (float)frametime * 0.0010000000474974513f;

    if (weapDef->bADSPositionInfo) {
        if (f > 0.0f && weapDef->overlayReticle)
            return;

        swayMaxAngle = weapDef->swayMaxAngle + (weapDef->adsSwayMaxAngle - weapDef->swayMaxAngle) * f;
        swayLerpSpeed = weapDef->swayLerpSpeed + (weapDef->adsSwayLerpSpeed - weapDef->swayLerpSpeed) * f;
        swayPitchScale = weapDef->swayPitchScale + (weapDef->adsSwayPitchScale - weapDef->swayPitchScale) * f;
        swayYawScale = weapDef->swayYawScale + (weapDef->adsSwayYawScale - weapDef->swayYawScale) * f;
        swayHorizScale = weapDef->swayHorizScale + (weapDef->adsSwayHorizScale - weapDef->swayHorizScale) * f;
        swayVertScale = weapDef->swayVertScale + (weapDef->adsSwayVertScale - weapDef->swayVertScale) * f;
    } else {
        swayMaxAngle = weapDef->swayMaxAngle;
        swayLerpSpeed = weapDef->swayLerpSpeed;
        swayPitchScale = weapDef->swayPitchScale;
        swayYawScale = weapDef->swayYawScale;
        swayHorizScale = weapDef->swayHorizScale;
        swayVertScale = weapDef->swayVertScale;
    }

    swayPitchScale *= ssSwayScale;
    swayYawScale *= ssSwayScale;
    swayHorizScale *= ssSwayScale;
    swayVertScale *= ssSwayScale;

    AnglesSubtract(ps->viewangles, swayViewAngles, deltaAngles);
    scale = 1.0f / (dt * 60.0f);
    deltaAngles[0] *= scale;
    deltaAngles[1] *= scale;

    if (deltaAngles[0] > swayMaxAngle)
        deltaAngles[0] = swayMaxAngle;
    if (deltaAngles[0] < -swayMaxAngle)
        deltaAngles[0] = -swayMaxAngle;
    if (deltaAngles[1] > swayMaxAngle)
        deltaAngles[1] = swayMaxAngle;
    if (deltaAngles[1] < -swayMaxAngle)
        deltaAngles[1] = -swayMaxAngle;

    swayOffset[1] = DiffTrack(deltaAngles[1] * swayVertScale, swayOffset[1], swayLerpSpeed, dt);
    swayOffset[2] = DiffTrack(deltaAngles[0] * swayHorizScale, swayOffset[2], swayLerpSpeed, dt);
    swayAngles[0] = DiffTrackAngle(deltaAngles[0] * swayPitchScale, swayAngles[0], swayLerpSpeed, dt, 0.0f, 0.0f, 0.0f);
    swayAngles[1] = DiffTrackAngle(deltaAngles[1] * swayYawScale, swayAngles[1], swayLerpSpeed, dt, 0.0f, 0.0f, 0.0f);

    swayViewAngles[0] = ps->viewangles[0];
    swayViewAngles[1] = ps->viewangles[1];
    swayViewAngles[2] = ps->viewangles[2];
}

int BG_GetFirstAvailableOffhand(const playerState_t *ps, int offhandClass)
{
    int weaponIndex;

    for (weaponIndex = 1; weaponIndex <= bg_iNumWeapons; weaponIndex++) {
        const WeaponDef *weapDef = bg_weaponDefs[weaponIndex];

        if (weapDef->offhandClass != offhandClass)
            continue;
        if (!(ps->weapons[weaponIndex >> 5] & (1 << (weaponIndex & 0x1f))))
            continue;
        if (ps->ammo[weapDef->iAmmoIndex] + ps->ammoclip[weapDef->iClipIndex])
            return weaponIndex;
    }

    return 0;
}

int BG_GetMaxPickupableAmmo(const playerState_t *ps, int weaponIndex)
{
    const WeaponDef *weapDef = bg_weaponDefs[weaponIndex];
    int ammoCounted[128];
    int clipCounted[128];
    int ammoIndex = weapDef->iAmmoIndex;
    int clipIndex = weapDef->iClipIndex;
    int ammo;
    int i;

    memset(ammoCounted, 0, sizeof(ammoCounted));
    memset(clipCounted, 0, sizeof(clipCounted));

    if (weapDef->iSharedAmmoCapIndex < 0) {
        if (weapDef->bClipOnly)
            return bg_weapClips[clipIndex]->iClipSize - ps->ammoclip[clipIndex];
        return bg_weapAmmoTypes[ammoIndex]->iMaxAmmo - ps->ammo[ammoIndex];
    }

    ammo = bg_sharedAmmoCaps[weapDef->iSharedAmmoCapIndex]->iSharedAmmoCap;
    for (i = 1; i <= bg_iNumWeapons; i++) {
        const WeaponDef *otherWeapDef;

        if (!(ps->weapons[i >> 5] & (1 << (i & 0x1f))))
            continue;

        otherWeapDef = bg_weaponDefs[i];
        if (otherWeapDef->iSharedAmmoCapIndex != weapDef->iSharedAmmoCapIndex)
            continue;

        if (otherWeapDef->bClipOnly) {
            clipIndex = otherWeapDef->iClipIndex;
            if (clipCounted[clipIndex])
                continue;

            clipCounted[clipIndex] = 1;
            ammo -= ps->ammoclip[clipIndex];
        } else {
            ammoIndex = otherWeapDef->iAmmoIndex;
            if (ammoCounted[ammoIndex])
                continue;

            ammoCounted[ammoIndex] = 1;
            ammo -= ps->ammo[ammoIndex];
        }
    }

    return ammo;
}

void PM_UpdateAimDownSightFlag(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    int oldFlags = ps->pm_flags;
    int baseFlags = oldFlags & ~0x40;
    qboolean canAds = 0;

    ps->pm_flags = baseFlags;

    if ((ps->pm_type == 1 && !pml->almostGroundPlane) || ps->pm_type < 1 || ps->pm_type > 7) {
        const WeaponDef *weapDef = bg_weaponDefs[BG_GetViewmodelWeaponIndex_core(ps)];
        int weaponstate = ps->weaponstate;

        if (weapDef->bADSPositionInfo &&
            (unsigned int)(weaponstate - 12) > 4 &&
            (unsigned int)(weaponstate - 10) > 1 &&
            (unsigned int)(weaponstate - 1) > 1) {
            if ((unsigned int)(weaponstate - 17) <= 5)
                canAds = (unsigned int)(weaponstate - 19) <= 1;
            else
                canAds = 1;
        }
    }

    if ((pm->cmd.buttons & 0x1000) || PM_IsBinocularsADS_core(ps)) {
        if (canAds) {
            if (!(oldFlags & 1)) {
                ps->pm_flags = baseFlags | 0x40;
            } else if (!(pm->oldcmd.buttons & 0x1000) || (!pm->cmd.forwardmove && !pm->cmd.rightmove)) {
                ps->pm_flags = baseFlags | 0x840;
            }
        }
    }

    BG_UpdateConditionValue(ps->clientNum, 7, (ps->pm_flags & 0x40) != 0, 1);
}

Bool BG_IsWeaponValid(const playerState_t *ps, int weaponIndex)
{
    const WeaponDef *weapDef;
    Bool owned;

    if (weaponIndex < 0 || weaponIndex > bg_iNumWeapons)
        return 0;

    owned = (ps->weapons[weaponIndex >> 5] & (1 << (weaponIndex & 0x1f))) != 0;
    if (!owned)
        return 0;

    weapDef = bg_weaponDefs[weaponIndex];
    if (weapDef->offhandClass)
        return owned;
    if ((int)ps->weaponslots[1] == weaponIndex)
        return owned;
    if ((int)ps->weaponslots[2] == weaponIndex)
        return owned;
    if (weapDef->iAltWeaponIndex == weaponIndex)
        return owned;

    return 0;
}

qboolean BG_TakePlayerWeapon(playerState_t *pPS, int iWeaponIndex)
{
    WeaponDef *weapDef;
    int slot;
    int altWeaponIndex;

    if (!(pPS->weapons[iWeaponIndex >> 5] & (1 << (iWeaponIndex & 0x1f))))
        return 0;

    weapDef = bg_weaponDefs[iWeaponIndex];
    slot = BG_IsPlayerWeaponInSlot_core(pPS, iWeaponIndex, 1);
    if (slot) {
        if (weapDef->bSlotStackable) {
            int replacement;

            pPS->weaponslots[slot] = 0;
            for (replacement = 1; replacement <= bg_iNumWeapons; replacement++) {
                if ((pPS->weapons[replacement >> 5] & (1 << (replacement & 0x1f))) &&
                    !BG_IsPlayerWeaponInSlot_core(pPS, replacement, 1)) {
                    pPS->weaponslots[slot] = (char)replacement;
                    break;
                }
            }
        } else {
            pPS->weaponslots[slot] = 0;
        }
    }

    pPS->weapons[iWeaponIndex >> 5] &= ~(1 << (iWeaponIndex & 0x1f));

    altWeaponIndex = weapDef->iAltWeaponIndex;
    while (altWeaponIndex &&
           (pPS->weapons[altWeaponIndex >> 5] & (1 << (altWeaponIndex & 0x1f)))) {
        pPS->weapons[altWeaponIndex >> 5] &= ~(1 << (altWeaponIndex & 0x1f));
        altWeaponIndex = bg_weaponDefs[altWeaponIndex]->iAltWeaponIndex;
    }

    if (pPS->weapon == iWeaponIndex)
        pPS->weapon = 0;

    return 1;
}

static BM_NOINLINE void __attribute_regparm__(2) PM_BeginWeaponChange(playerState_t *ps, int newweapon)
{
    int oldWeapon;
    WeaponDef *oldWeapDef;
    qboolean altSwitch;
    qboolean ammoAvailable;

    if (newweapon && !(ps->weapons[newweapon >> 5] & (1 << (newweapon & 0x1f))))
        return;
    if (ps->weaponstate == 2)
        return;

    ps->weaponDelay = 0;
    oldWeapon = ps->weapon;
    if (!oldWeapon || !(ps->weapons[oldWeapon >> 5] & (1 << (oldWeapon & 0x1f))) || ps->grenadeTimeLeft > 0) {
        ps->weaponTime = 0;
        ps->weaponstate = 2;
        ps->grenadeTimeLeft = 0;
        PM_SetProneMovementOverride(ps);
        return;
    }

    oldWeapDef = bg_weaponDefs[oldWeapon];
    altSwitch = newweapon && newweapon == oldWeapDef->iAltWeaponIndex;
    ammoAvailable = !oldWeapDef->bClipOnly || ps->ammoclip[oldWeapDef->iClipIndex] != 0;

    ps->grenadeTimeLeft = 0;

    if (altSwitch) {
        PM_AddEvent(ps, 0x9c);
        if (ps->pm_type <= 5)
            ps->weapAnim = ((ps->weapAnim & 0x200) ^ 0x200) | 0xf;
    } else {
        if (ammoAvailable) {
            PM_AddEvent(ps, 0x9b);
            if (ps->pm_type <= 5)
                ps->weapAnim = ((ps->weapAnim & 0x200) ^ 0x200) | 9;
        }

        if (!(ps->pm_flags & 4))
            BG_AnimScriptEvent(ps, 6, 0, 0);
    }

    ps->weaponstate = 2;
    PM_SetProneMovementOverride(ps);

    if (!altSwitch) {
        ps->weaponTime = oldWeapDef->iDropTime;
        return;
    }

    ps->weaponTime = oldWeapDef->iAltDropTime;
    if (ps->weapons[oldWeapon >> 5] & (1 << (oldWeapon & 0x1f))) {
        int slot = BG_IsPlayerWeaponInSlot_core(ps, oldWeapon, 1);

        if ((unsigned int)(slot - 1) <= 1 &&
            (ps->weapons[newweapon >> 5] & (1 << (newweapon & 0x1f))) &&
            (unsigned int)(bg_weaponDefs[newweapon]->weapSlot - 1) <= 1) {
            if ((int)ps->weaponslots[slot] != newweapon && ps->weapon != newweapon)
                ps->weapon = 0;
            ps->weaponslots[slot] = (char)newweapon;
        }
    }
}

static BM_ALWAYS_INLINE void PM_SetReloadDelay(playerState_t *ps)
{
    int iWeapon = ps->weapon;
    WeaponDef *weapDef = bg_weaponDefs[iWeapon];
    int reloadTime;

    if ((unsigned int)(ps->weaponstate - 7) <= 1) {
        reloadTime = weapDef->iReloadStartAddTime;
        if (reloadTime && reloadTime >= weapDef->iReloadStartTime)
            reloadTime = weapDef->iReloadStartTime;
    } else {
        if (ps->ammoclip[weapDef->iClipIndex] || weapDef->weapType)
            reloadTime = weapDef->iReloadTime;
        else
            reloadTime = weapDef->iReloadEmptyTime;

        if (weapDef->iReloadAddTime && reloadTime > weapDef->iReloadAddTime)
            reloadTime = weapDef->iReloadAddTime;
    }

    if (weapDef->bBoltAction &&
        (ps->weaponrechamber[iWeapon >> 5] & (1 << (iWeapon & 0x1f)))) {
        int delay;

        if (!reloadTime)
            reloadTime = ps->weaponTime;

        delay = weapDef->iRechamberBoltTime;
        if (reloadTime <= delay)
            delay = reloadTime;
        if (!delay)
            delay = 1;

        ps->weaponDelay = delay;
    } else if (reloadTime) {
        ps->weaponDelay = reloadTime;
    }
}

static void __attribute_regparm__(1) PM_SetReloadingState(playerState_t *ps)
{
    WeaponDef *weapDef = bg_weaponDefs[ps->weapon];

    if (!ps->ammoclip[weapDef->iClipIndex] && !weapDef->weapType) {
        if (ps->pm_type <= 5)
            ps->weapAnim = ((ps->weapAnim & 0x200) ^ 0x200) | 0xc;

        ps->weaponTime = weapDef->iReloadEmptyTime;
        PM_AddEvent(ps, 0x97);
    } else {
        if (ps->pm_type <= 5)
            ps->weapAnim = ((ps->weapAnim & 0x200) ^ 0x200) | 0xb;

        ps->weaponTime = weapDef->iReloadTime;
        PM_AddEvent(ps, 0x96);
    }

    ps->weaponstate = (ps->weaponstate == 8) + 5;
    PM_SetReloadDelay(ps);
}

static void __attribute_regparm__(1) PM_BeginWeaponReload(playerState_t *ps)
{
    int iWeapon = ps->weapon;
    WeaponDef *weapDef;

    if (ps->weaponstate && ps->weaponstate != 3 && ps->weaponstate != 4)
        return;
    if (!iWeapon || iWeapon > bg_iNumWeapons)
        return;

    weapDef = bg_weaponDefs[iWeapon];
    if (!weapDef->bClipOnly)
        BG_AnimScriptEvent(ps, 0xa, 0, 1);

    /* ADS interpolation lowers the weapon during reload. Preserve the input
       request so held/toggled aim resumes when the animation permits it. */
    if (weapDef->bSegmentedReload && weapDef->iReloadStartTime) {
        if (ps->pm_type <= 5)
            ps->weapAnim = ((ps->weapAnim & 0x200) ^ 0x200) | 0xd;

        ps->weaponTime = weapDef->iReloadStartTime;
        ps->weaponstate = 7;
        PM_AddEvent(ps, 0x98);
        PM_SetReloadDelay(ps);
        return;
    }

    PM_SetReloadingState(ps);
}

void PM_UpdateAimDownSightLerp(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    WeaponDef *weapDef = bg_weaponDefs[BG_GetViewmodelWeaponIndex(ps)];
    qboolean adsRequested;
    float pos;

    if (player_scopeExitOnDamage->current.enabled && ps->damageCount && weapDef->overlayReticle) {
        PM_AddEvent(ps, 0x95);
        ps->pm_flags &= ~0x40;
        ps->fWeaponPosFrac = 0.0f;
        ps->adsDelayTime = 0;
        return;
    }

    if (!weapDef->bADSPositionInfo) {
        ps->fWeaponPosFrac = 0.0f;
        ps->adsDelayTime = 0;
        return;
    }

    if ((weapDef->bSegmentedReload &&
         ((unsigned int)(ps->weaponstate - 5) <= 3 ||
          (ps->weaponstate == 9 && ps->weaponTime > weapDef->iPositionReloadTransTime))) ||
        (!weapDef->bSegmentedReload && ps->weaponstate == 5 &&
         ps->weaponTime > weapDef->iPositionReloadTransTime) ||
        (!weapDef->bRechamberWhileAds && ps->weaponstate == 4)) {
        adsRequested = 0;
    } else {
        adsRequested = (ps->pm_flags & 0x40) != 0;
    }

    if (weapDef->bADSFire && ps->weaponDelay && ps->weaponstate == 3)
        adsRequested = 1;

    if (ps->fWeaponPosFrac == 1.0f && !adsRequested &&
        player_adsExitDelay->current.integer > 0) {
        int adsDelayTime = ps->adsDelayTime;

        if (!adsDelayTime) {
            adsDelayTime = pm->cmd.serverTime + player_adsExitDelay->current.integer;
            ps->adsDelayTime = adsDelayTime;
        }

        if (adsDelayTime > pm->cmd.serverTime)
            return;

        ps->adsDelayTime = 0;
    } else {
        ps->adsDelayTime = 0;
    }

    if (adsRequested) {
        if (ps->fWeaponPosFrac == 1.0f)
            return;

        pos = ps->fWeaponPosFrac + (float)pml->msec * weapDef->fOOPosAnimLength[0];
    } else {
        if (ps->fWeaponPosFrac == 0.0f)
            return;

        pos = ps->fWeaponPosFrac - (float)pml->msec * weapDef->fOOPosAnimLength[1];
    }

    if (pos > 1.0f)
        pos = 1.0f;
    if (pos < 0.0f)
        pos = 0.0f;

    ps->fWeaponPosFrac = pos;
}

void PM_ResetWeaponState(playerState_t *ps)
{
    ps->pm_flags &= ~(0x810 | PM_WEAPON_FLAG_TRIGGER_HELD);

    if ((unsigned int)(ps->weaponstate - 0x11) <= 5) {
        ps->fWeaponPosFrac = 0.0f;
        PM_AddEvent(ps, 0x95);
        ps->pm_flags &= ~0x20040;
        PM_AddEvent(ps, 0xa9);
    }

    ps->weaponTime = 0;
    ps->weaponDelay = 0;
    ps->weaponstate = 0;

    if (ps->pm_type <= 5)
        ps->weapAnim = (ps->weapAnim & 0x200) ^ 0x200;
}

qboolean PM_InteruptWeaponWithProneMove(playerState_t *ps)
{
    int weaponstate = ps->weaponstate;

    if ((unsigned int)weaponstate <= 2 || weaponstate == 5 || weaponstate == 7 ||
        weaponstate == 9 || weaponstate == 8 || weaponstate == 6 || weaponstate == 4)
        return 1;

    if (weaponstate == 3 || weaponstate == 11 || (unsigned int)(weaponstate - 12) <= 4)
        return 0;

    ps->pm_flags &= ~0x810;

    if ((unsigned int)(weaponstate - 0x11) <= 5) {
        ps->fWeaponPosFrac = 0.0f;
        PM_AddEvent(ps, 0x95);
        ps->pm_flags &= ~0x20040;
        PM_AddEvent(ps, 0xa9);
    }

    ps->weaponTime = 0;
    ps->weaponDelay = 0;
    ps->weaponstate = 0;

    if (ps->pm_type <= 5)
        ps->weapAnim = (ps->weapAnim & 0x200) ^ 0x200;

    return 1;
}

#define WEAPON_READY 0
#define WEAPON_RAISING 1
#define WEAPON_DROPPING 2
#define WEAPON_FIRING 3
#define WEAPON_RECHAMBERING 4
#define WEAPON_RELOADING 5
#define WEAPON_RELOADING_INTERUPT 6
#define WEAPON_RELOAD_START 7
#define WEAPON_RELOAD_START_INTERUPT 8
#define WEAPON_RELOAD_END 9
#define WEAPON_MELEE_INIT 10
#define WEAPON_MELEE_FIRE 11
#define WEAPON_OFFHAND_INIT 12
#define WEAPON_OFFHAND_PREPARE 13
#define WEAPON_OFFHAND_HOLD 14
#define WEAPON_OFFHAND_FIRE 15
#define WEAPON_OFFHAND_END 16
#define WEAPON_BINOCULARS_INIT 17
#define WEAPON_BINOCULARS_RAISE 18
#define WEAPON_BINOCULARS_HOLD 19
#define WEAPON_BINOCULARS_IDLE 20
#define WEAPON_BINOCULARS_DROP 21
#define WEAPON_BINOCULARS_END 22

#define PM_WEAPON_BUTTON_ATTACK 0x00000001
#define PM_WEAPON_BUTTON_MELEE 0x00000004
#define PM_WEAPON_BUTTON_RELOAD 0x00000010
#define PM_WEAPON_BUTTON_ADS 0x00001000
#define PM_WEAPON_BUTTON_BINOCULARS 0x00004000
#define PM_WEAPON_BUTTON_HOLDBREATH 0x00008000
#define PM_WEAPON_BUTTON_FRAG 0x00010000
#define PM_WEAPON_BUTTON_SMOKE 0x00020000

#define PM_WEAPON_FLAG_OFFHAND 0x00000010
#define PM_WEAPON_FLAG_ADS 0x00000040
#define PM_WEAPON_FLAG_RELOAD 0x00000008
#define PM_WEAPON_FLAG_MELEE_HELD 0x00002000
#define PM_WEAPON_FLAG_HOLDBREATH 0x00004000
#define PM_WEAPON_FLAG_BINOCULARS 0x00020000

static qboolean PM_WeaponHasInventoryWeapon(const playerState_t *ps, int weapon)
{
    if (weapon <= 0 || weapon > bg_iNumWeapons)
        return 0;

    return (ps->weapons[weapon >> 5] & (1 << (weapon & 0x1f))) != 0;
}

static void PM_WeaponSetAnim(playerState_t *ps, int anim)
{
    if (ps->pm_type <= 5)
        ps->weapAnim = ((ps->weapAnim & 0x200) ^ 0x200) | anim;
}

static void PM_WeaponClearAnim(playerState_t *ps)
{
    if (ps->pm_type <= 5)
        ps->weapAnim = (ps->weapAnim & 0x200) ^ 0x200;
}

static void PM_WeaponSetIdle(playerState_t *ps)
{
    ps->weaponTime = 0;
    ps->weaponDelay = 0;
    ps->weaponstate = WEAPON_READY;
    PM_WeaponClearAnim(ps);
}

static qboolean PM_WeaponStateIsBinoculars(int weaponstate)
{
    return (unsigned int)(weaponstate - WEAPON_BINOCULARS_INIT) <=
           (WEAPON_BINOCULARS_END - WEAPON_BINOCULARS_INIT);
}

static qboolean PM_WeaponStateIsOffhand(int weaponstate)
{
    return (unsigned int)(weaponstate - WEAPON_OFFHAND_INIT) <=
           (WEAPON_OFFHAND_END - WEAPON_OFFHAND_INIT);
}

static qboolean PM_WeaponStateIsReloading(int weaponstate)
{
    return (weaponstate == WEAPON_RELOADING ||
            weaponstate == WEAPON_RELOADING_INTERUPT ||
            weaponstate == WEAPON_RELOAD_START ||
            weaponstate == WEAPON_RELOAD_START_INTERUPT ||
            weaponstate == WEAPON_RELOAD_END);
}

static qboolean PM_WeaponStateBlocksChange(int weaponstate)
{
    if (weaponstate == WEAPON_FIRING || weaponstate == WEAPON_RECHAMBERING)
        return 1;
    if (weaponstate == WEAPON_MELEE_INIT || weaponstate == WEAPON_MELEE_FIRE)
        return 1;
    if (PM_WeaponStateIsReloading(weaponstate) || PM_WeaponStateIsOffhand(weaponstate))
        return 1;
    return 0;
}

static int PM_FindFirstSelectableWeapon(const playerState_t *ps)
{
    int weapon;

    for (weapon = 1; weapon <= bg_iNumWeapons; weapon++) {
        if (!PM_WeaponHasInventoryWeapon(ps, weapon))
            continue;
        if (bg_weaponDefs[weapon]->offhandClass)
            continue;
        return weapon;
    }

    return 0;
}

static int PM_FindBinocularWeapon(void)
{
    int weapon;

    for (weapon = 1; weapon <= bg_iNumWeapons; weapon++) {
        if (bg_weaponDefs[weapon]->weapType == WEAPTYPE_BINOCULARS)
            return (unsigned char)weapon;
    }

    return 0;
}

static void PM_SetWeaponRechamberBit(playerState_t *ps, int weapon)
{
    ps->weaponrechamber[weapon >> 5] |= 1 << (weapon & 0x1f);
}

static void PM_ClearWeaponRechamberBit(playerState_t *ps, int weapon)
{
    ps->weaponrechamber[weapon >> 5] &= ~(1 << (weapon & 0x1f));
}

static qboolean PM_HasWeaponRechamberBit(const playerState_t *ps, int weapon)
{
    return (ps->weaponrechamber[weapon >> 5] & (1 << (weapon & 0x1f))) != 0;
}

static void PM_UpdateHoldBreath(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    WeaponDef *weapDef = bg_weaponDefs[BG_GetViewmodelWeaponIndex(ps)];
    int holdTime = (int)(player_breath_hold_time->current.value * 1000.0f);
    int gaspTime = (int)(player_breath_gasp_time->current.value * 1000.0f);
    float target;
    float rate;

    if (holdTime <= 0) {
        ps->pm_flags &= ~PM_WEAPON_FLAG_HOLDBREATH;
        ps->holdBreathScale = 1.0f;
        ps->holdBreathTimer = 0;
        return;
    }

    if (ps->fWeaponPosFrac == 1.0f && weapDef->overlayReticle &&
        weapDef->weapClass != 9 && (pm->cmd.buttons & PM_WEAPON_BUTTON_HOLDBREATH)) {
        if (!ps->holdBreathTimer)
            ps->pm_flags |= PM_WEAPON_FLAG_HOLDBREATH;
    } else {
        ps->pm_flags &= ~PM_WEAPON_FLAG_HOLDBREATH;
    }

    if (ps->pm_flags & PM_WEAPON_FLAG_HOLDBREATH)
        ps->holdBreathTimer += pml->msec;
    else
        ps->holdBreathTimer -= pml->msec;

    if (ps->holdBreathTimer < 0)
        ps->holdBreathTimer = 0;

    if (ps->pm_flags & PM_WEAPON_FLAG_HOLDBREATH) {
        if (ps->holdBreathTimer > holdTime) {
            ps->holdBreathTimer = holdTime + gaspTime;
            ps->pm_flags &= ~PM_WEAPON_FLAG_HOLDBREATH;
        }
    }

    if (ps->pm_flags & PM_WEAPON_FLAG_HOLDBREATH) {
        target = 1.0f - ps->fWeaponPosFrac;
        rate = player_breath_hold_lerp->current.value;
    } else {
        int totalTime = holdTime + gaspTime;
        float gaspFrac = 0.0f;

        if (totalTime > 0)
            gaspFrac = (float)ps->holdBreathTimer / (float)totalTime;

        target = 1.0f + ps->fWeaponPosFrac * gaspFrac *
                            (player_breath_gasp_scale->current.value - 1.0f);
        rate = player_breath_gasp_lerp->current.value;
    }

    ps->holdBreathScale = DiffTrack(target, ps->holdBreathScale, rate, pml->frametime);
}

static qboolean PM_UpdateOffhandCook(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    WeaponDef *weapDef;

    if (!(ps->pm_flags & PM_WEAPON_FLAG_OFFHAND) || !ps->offHandIndex)
        return 0;

    /* Once the release event has fired, the missile owns the remaining fuse.
     * The predicting client must not continue cooking it in the player's hand. */
    if (ps->weaponstate == WEAPON_OFFHAND_FIRE && !ps->weaponDelay)
        return 0;

    weapDef = bg_weaponDefs[ps->offHandIndex];
    if (!BG_GrenadeCanCook(weapDef) || ps->grenadeTimeLeft <= 0)
        return 0;

    ps->grenadeTimeLeft -= pml->msec;
    if (ps->grenadeTimeLeft > 0)
        return 0;

    if (!(ps->pm_flags & PMF_GRENADE_THROWBACK) && ps->ammoclip[weapDef->iClipIndex] > 0)
        ps->ammoclip[weapDef->iClipIndex]--;
    ps->grenadeTimeLeft = 0;
    ps->weaponstate = WEAPON_OFFHAND_END;
    ps->weaponTime = ps->weaponDelay = 0;
    ps->pm_flags &= ~(0x810 | PMF_GRENADE_THROWBACK);
    PM_AddEvent(ps, 0xc5);
    return 1;
}

static qboolean PM_UpdateWeaponTimers(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    qboolean delayedAction = 0;

    if (ps->weaponRestrictKickTime > 0) {
        ps->weaponRestrictKickTime -= pml->msec;
        if (ps->weaponRestrictKickTime < 0)
            ps->weaponRestrictKickTime = 0;
    }

    if (ps->weaponTime > 0) {
        ps->weaponTime -= pml->msec;
        if (ps->weaponTime <= 0)
            ps->weaponTime = 0;
    }

    if (ps->weaponDelay > 0) {
        ps->weaponDelay -= pml->msec;
        if (ps->weaponDelay <= 0) {
            ps->weaponDelay = 0;
            delayedAction = 1;
        }
    }

    return delayedAction;
}

static void PM_UpdateWeaponConditions(playerState_t *ps, WeaponDef *weapDef)
{
    BG_UpdateConditionValue(ps->clientNum, 0, weapDef->playerAnimType, 1);
    BG_UpdateConditionValue(ps->clientNum, 1, weapDef->weapClass, 1);
}

static void PM_FinishWeaponChange(pmove_t *pm)
{
    playerState_t *ps = pm->ps;
    int oldWeapon = ps->weapon;
    int newWeapon = pm->cmd.weapon;
    WeaponDef *weapDef;
    qboolean altSwitch = 0;

    if (!PM_WeaponHasInventoryWeapon(ps, newWeapon))
        newWeapon = PM_FindFirstSelectableWeapon(ps);

    ps->weapon = (unsigned char)newWeapon;

    if (oldWeapon == newWeapon) {
        PM_WeaponSetIdle(ps);
        return;
    }

    if (!newWeapon) {
        PM_WeaponSetIdle(ps);
        return;
    }

    weapDef = bg_weaponDefs[newWeapon];
    if (oldWeapon && bg_weaponDefs[oldWeapon]->iAltWeaponIndex == newWeapon)
        altSwitch = 1;

    ps->weaponstate = WEAPON_RAISING;
    ps->weaponDelay = 0;
    PM_SetProneMovementOverride(ps);
    PM_UpdateWeaponConditions(ps, weapDef);

    if (altSwitch) {
        ps->weaponTime = weapDef->iAltRaiseTime;
        if (ps->aimSpreadScale < 128.0f)
            ps->aimSpreadScale = 128.0f;
        PM_WeaponSetAnim(ps, 0x10);
        return;
    }

    if (oldWeapon)
        PM_AddEvent(ps, 0x9a);

    ps->weaponTime = weapDef->iRaiseTime;
    BG_AnimScriptEvent(ps, 7, 0, 1);
    ps->aimSpreadScale = 255.0f;
    PM_WeaponSetAnim(ps, 0x0a);
}

static qboolean PM_ShouldChangeWeapon(pmove_t *pm)
{
    playerState_t *ps = pm->ps;
    int newWeapon = pm->cmd.weapon;

    if (!newWeapon || newWeapon == ps->weapon)
        return 0;
    if (!PM_WeaponHasInventoryWeapon(ps, newWeapon))
        return 0;
    if (PM_WeaponStateBlocksChange(ps->weaponstate))
        return 0;
    if (PM_WeaponStateIsBinoculars(ps->weaponstate))
        return 1;
    if (ps->weaponstate == WEAPON_DROPPING)
        return 0;

    return 1;
}

static void PM_CheckWeaponChange(pmove_t *pm)
{
    playerState_t *ps = pm->ps;

    if (ps->weaponstate == WEAPON_DROPPING)
        return;

    if (PM_ShouldChangeWeapon(pm)) {
        if (PM_WeaponStateIsBinoculars(ps->weaponstate))
            PM_ResetWeaponState(ps);
        PM_BeginWeaponChange(ps, pm->cmd.weapon);
    }
}

static int PM_OffhandClassForButtons(int buttons)
{
    if (buttons & PM_WEAPON_BUTTON_FRAG)
        return OFFHAND_CLASS_FRAG_GRENADE;
    if (buttons & PM_WEAPON_BUTTON_SMOKE)
        return OFFHAND_CLASS_SMOKE_GRENADE;
    return OFFHAND_CLASS_NONE;
}

static void PM_StartOffhandPrepare(playerState_t *ps)
{
    WeaponDef *weapDef = bg_weaponDefs[ps->offHandIndex];

    ps->weaponstate = WEAPON_OFFHAND_PREPARE;
    ps->weaponTime = weapDef->iHoldFireTime;
    ps->weaponDelay = 0;
    ps->pm_flags |= PM_WEAPON_FLAG_OFFHAND;
    BG_AddPredictableEventToPlayerstate(0xa5, ps->offHandIndex, ps);
    PM_WeaponSetAnim(ps, 0x13);
    PM_SetProneMovementOverride(ps);
}

static qboolean PM_TryStartOffhand(pmove_t *pm)
{
    playerState_t *ps = pm->ps;
    int offhandClass;
    int offhandIndex;
    WeaponDef *offhandDef;

    if (PM_WeaponStateBlocksChange(ps->weaponstate) || PM_WeaponStateIsBinoculars(ps->weaponstate))
        return 0;
    if ((ps->eFlags & 0x300) || (ps->pm_flags & 4))
        return 0;

    offhandClass = PM_OffhandClassForButtons(pm->cmd.buttons);
    if (!offhandClass)
        return 0;

    /* The server claims the nearby live missile before running Pmove. Until
     * that snapshot arrives, don't predict pulling a new inventory grenade. */
    if (offhandClass == OFFHAND_CLASS_FRAG_GRENADE && ps->cursorHint == GRENADE_THROWBACK_HINT)
        return 1;

    offhandIndex = BG_GetFirstAvailableOffhand(ps, offhandClass);
    if (!offhandIndex) {
        PM_AddEvent(ps, 0x94);
        return 0;
    }

    BG_AddPredictableEventToPlayerstate(0xa7, offhandIndex, ps);
    ps->offHandIndex = offhandIndex;
    offhandDef = bg_weaponDefs[offhandIndex];

    if (offhandDef->weapType != WEAPTYPE_GRENADE)
        Com_Error(1, "[%s] Only grenades are currently supported for off hand use\n", offhandDef->szInternalName);
    if (!offhandDef->offhandClass)
        Com_Error(1, "[%s] No offhand class set\n", offhandDef->szInternalName);

    if (ps->weapon && ps->weaponstate != WEAPON_OFFHAND_END) {
        WeaponDef *weapDef = bg_weaponDefs[ps->weapon];

        ps->weaponstate = WEAPON_OFFHAND_INIT;
        ps->weaponTime = weapDef->quickDropTime;
        ps->weaponDelay = 0;
        ps->pm_flags &= ~PM_WEAPON_FLAG_OFFHAND;
        PM_AddEvent(ps, 0x95);
        ps->pm_flags &= ~PM_WEAPON_FLAG_ADS;
        PM_WeaponSetAnim(ps, 0x11);
        return 1;
    }

    PM_StartOffhandPrepare(ps);
    return 1;
}

static void PM_StartBinocularRaise(playerState_t *ps)
{
    int binocularWeapon = PM_FindBinocularWeapon();
    WeaponDef *weapDef = bg_weaponDefs[binocularWeapon];

    ps->weaponstate = WEAPON_BINOCULARS_HOLD;
    ps->weaponTime = weapDef->iAdsTransInTime;
    ps->weaponDelay = 0;
    ps->pm_flags |= PM_WEAPON_FLAG_BINOCULARS;
    PM_AddEvent(ps, 0xa8);
    PM_WeaponClearAnim(ps);
}

static void PM_StartBinocularDrop(playerState_t *ps)
{
    ps->weaponstate = WEAPON_BINOCULARS_END;
    ps->pm_flags &= ~PM_WEAPON_FLAG_BINOCULARS;
    PM_AddEvent(ps, 0xa9);

    if (ps->weapon) {
        WeaponDef *weapDef = bg_weaponDefs[ps->weapon];

        ps->weaponTime = weapDef->quickRaiseTime;
        ps->weaponDelay = 0;
        PM_WeaponSetAnim(ps, 0x12);
    } else {
        ps->weaponTime = 0;
        ps->weaponDelay = 1;
    }
}

static qboolean PM_TryToggleBinoculars(pmove_t *pm)
{
    playerState_t *ps = pm->ps;
    qboolean togglePressed;
    qboolean isIdle;

    if (PM_WeaponStateBlocksChange(ps->weaponstate) || PM_WeaponStateIsOffhand(ps->weaponstate))
        return 0;

    if (player_toggleBinoculars->current.enabled) {
        togglePressed = !(pm->oldcmd.buttons & PM_WEAPON_BUTTON_BINOCULARS) &&
                        (pm->cmd.buttons & PM_WEAPON_BUTTON_BINOCULARS);
    } else {
        togglePressed = (pm->cmd.buttons & PM_WEAPON_BUTTON_BINOCULARS) != 0;
    }

    isIdle = ps->weaponstate >= WEAPON_BINOCULARS_RAISE && ps->weaponstate <= WEAPON_BINOCULARS_IDLE;

    if (togglePressed && !isIdle) {
        int binocularWeapon = PM_FindBinocularWeapon();

        if (!binocularWeapon)
            return 0;

        if (ps->weapon) {
            WeaponDef *weapDef = bg_weaponDefs[ps->weapon];

            ps->weaponstate = WEAPON_BINOCULARS_INIT;
            ps->weaponTime = weapDef->quickDropTime;
            ps->weaponDelay = 0;
            PM_WeaponClearAnim(ps);
        } else {
            PM_StartBinocularRaise(ps);
        }
        return 1;
    }

    if (!togglePressed && isIdle && !player_toggleBinoculars->current.enabled) {
        PM_StartBinocularDrop(ps);
        return 1;
    }

    if (togglePressed && isIdle && player_toggleBinoculars->current.enabled) {
        PM_StartBinocularDrop(ps);
        return 1;
    }

    return 0;
}

static qboolean PM_WeaponCanReloadNow(playerState_t *ps, qboolean reloadRequested)
{
    WeaponDef *weapDef;
    int ammoIndex;
    int clipIndex;

    if (!ps->weapon)
        return 0;

    weapDef = bg_weaponDefs[ps->weapon];
    ammoIndex = weapDef->iAmmoIndex;
    clipIndex = weapDef->iClipIndex;

    if (reloadRequested && PM_Weapon_AllowReload(ps))
        return 1;

    if (!ps->ammoclip[clipIndex] && ps->ammo[ammoIndex] && ps->weaponstate != WEAPON_FIRING)
        return 1;

    return 0;
}

static void PM_CheckReloadStart(pmove_t *pm, qboolean delayedAction)
{
    playerState_t *ps = pm->ps;
    qboolean reloadRequested;

    if (delayedAction || PM_WeaponStateIsBinoculars(ps->weaponstate) ||
        PM_WeaponStateIsOffhand(ps->weaponstate) || PM_WeaponStateIsReloading(ps->weaponstate))
        return;
    if (ps->weaponstate != WEAPON_READY && ps->weaponstate != WEAPON_FIRING &&
        ps->weaponstate != WEAPON_RECHAMBERING)
        return;

    reloadRequested = (pm->cmd.buttons & PM_WEAPON_BUTTON_RELOAD) != 0;
    if (ps->pm_flags & PM_WEAPON_FLAG_RELOAD) {
        ps->pm_flags &= ~PM_WEAPON_FLAG_RELOAD;
        reloadRequested = 1;
    }

    if (PM_WeaponCanReloadNow(ps, reloadRequested))
        PM_BeginWeaponReload(ps);
}

static qboolean PM_StartMelee(playerState_t *ps, WeaponDef *weapDef)
{
    if (!weapDef->iMeleeDamage)
        return 0;

    if (ps->pm_flags & PM_WEAPON_FLAG_MELEE_HELD)
        return 0;

    ps->pm_flags |= PM_WEAPON_FLAG_MELEE_HELD;
    BG_AnimScriptEvent(ps, 0x11, 0, 1);
    PM_WeaponSetAnim(ps, 8);
    PM_AddEvent(ps, 0xa3);

    if (weapDef->iMeleeDelay) {
        ps->weaponstate = WEAPON_MELEE_INIT;
        ps->weaponTime = weapDef->iMeleeTime;
        ps->weaponDelay = weapDef->iMeleeDelay;
    } else {
        ps->weaponstate = WEAPON_MELEE_FIRE;
        ps->weaponTime = weapDef->iMeleeTime;
        ps->weaponDelay = 0;
        PM_AddEvent(ps, 0xa4);
        PM_SetProneMovementOverride(ps);
    }

    return 1;
}

static qboolean PM_CheckMeleeStart(pmove_t *pm, qboolean delayedAction)
{
    playerState_t *ps = pm->ps;
    WeaponDef *weapDef;

    if (delayedAction || !(pm->cmd.buttons & PM_WEAPON_BUTTON_MELEE)) {
        if (!(pm->cmd.buttons & PM_WEAPON_BUTTON_MELEE))
            ps->pm_flags &= ~PM_WEAPON_FLAG_MELEE_HELD;
        return 0;
    }

    if (!ps->weapon || PM_WeaponStateBlocksChange(ps->weaponstate) ||
        PM_WeaponStateIsBinoculars(ps->weaponstate))
        return 0;

    weapDef = bg_weaponDefs[ps->weapon];
    return PM_StartMelee(ps, weapDef);
}

static void PM_FinishMeleeDelay(playerState_t *ps)
{
    WeaponDef *weapDef = bg_weaponDefs[ps->weapon];
    int remaining = weapDef->iMeleeTime - weapDef->iMeleeDelay;

    if (ps->weaponTime < remaining)
        ps->weaponTime = remaining;

    PM_AddEvent(ps, 0xa4);
    ps->weaponstate = WEAPON_MELEE_FIRE;
    PM_SetProneMovementOverride(ps);
}

static void PM_StartRechamber(playerState_t *ps, WeaponDef *weapDef)
{
    ps->weaponstate = WEAPON_RECHAMBERING;
    ps->weaponTime = weapDef->iRechamberTime;
    ps->weaponDelay = weapDef->iRechamberBoltTime;
    PM_AddEvent(ps, 0xa1);

    if (ps->pm_type <= 5) {
        int anim = (ps->fWeaponPosFrac > 0.75f) ? 7 : 4;
        PM_WeaponSetAnim(ps, anim);
    }
}

static void PM_FinishRechamber(playerState_t *ps)
{
    if (ps->weapon && PM_HasWeaponRechamberBit(ps, ps->weapon)) {
        PM_ClearWeaponRechamberBit(ps, ps->weapon);
        PM_AddEvent(ps, 0xa2);
    }

    if (!ps->weaponTime && !ps->weaponDelay)
        PM_WeaponSetIdle(ps);
}

static void PM_AddFireSpread(playerState_t *ps, WeaponDef *weapDef)
{
    if (ps->fWeaponPosFrac == 1.0f)
        return;

    ps->aimSpreadScale += weapDef->fHipSpreadFireAdd * 255.0f;
    if (ps->aimSpreadScale > 255.0f)
        ps->aimSpreadScale = 255.0f;
}

static void PM_TryTakeEmptyClipOnlyWeapon(playerState_t *ps, WeaponDef *weapDef)
{
    int ammoIndex;
    int clipIndex;
    int weapon;

    if (!weapDef->bClipOnly)
        return;

    ammoIndex = weapDef->iAmmoIndex;
    clipIndex = weapDef->iClipIndex;
    if (ps->ammoclip[clipIndex] || ps->ammo[ammoIndex])
        return;

    weapon = ps->weapon;
    if (BG_TakePlayerWeapon(ps, weapon))
        PM_AddEvent(ps, 0x92);
}

static qboolean PM_ConsumeFireAmmo(playerState_t *ps, WeaponDef *weapDef)
{
    int clipIndex = weapDef->iClipIndex;
    int fireAnim;

    if (ps->ammoclip[clipIndex] <= 0)
        return 0;

    if (!(ps->eFlags & 0x300))
        ps->ammoclip[clipIndex]--;

    if (weapDef->weapType == WEAPTYPE_GRENADE && weapDef->bCookOffHold)
        ps->grenadeTimeLeft = 0;

    if (ps->fWeaponPosFrac > 0.75f)
        fireAnim = ps->ammoclip[clipIndex] ? 5 : 6;
    else
        fireAnim = ps->ammoclip[clipIndex] ? 2 : 3;

    PM_WeaponSetAnim(ps, fireAnim);
    PM_AddEvent(ps, ps->ammoclip[clipIndex] ? 0x9e : 0xa0);
    PM_AddFireSpread(ps, weapDef);
    PM_TryTakeEmptyClipOnlyWeapon(ps, weapDef);
    return 1;
}

static void PM_StartFireTimers(playerState_t *ps, WeaponDef *weapDef)
{
    ps->weaponDelay = weapDef->iFireDelay;
    ps->weaponTime = weapDef->iFireTime;

    if (weapDef->bADSFire) {
        float fracLeft = 1.0f - ps->fWeaponPosFrac;
        if (fracLeft > 0.0f && weapDef->fOOPosAnimLength[0] != 0.0f)
            ps->weaponDelay = (int)(fracLeft / weapDef->fOOPosAnimLength[0]);
    }

    BG_AnimScriptEvent(ps, 2, 0, 1);

    if (weapDef->bBoltAction)
        PM_SetWeaponRechamberBit(ps, ps->weapon);

    if (ps->fWeaponPosFrac >= 1.0f)
        ps->weaponRestrictKickTime = weapDef->iFireTime * weapDef->adsGunKickReducedKickBullets + weapDef->iFireDelay;
    else
        ps->weaponRestrictKickTime = weapDef->iFireTime * weapDef->hipGunKickReducedKickBullets + weapDef->iFireDelay;

    ps->weaponstate = WEAPON_FIRING;
    PM_SetProneMovementOverride(ps);
}

static qboolean PM_TryStartGrenadeFire(playerState_t *ps, WeaponDef *weapDef, qboolean delayedAction)
{
    int clipIndex = weapDef->iClipIndex;

    if (!ps->ammoclip[clipIndex])
        return 0;

    if (!delayedAction) {
        ps->grenadeTimeLeft = weapDef->iFuseTime;
        ps->weaponTime = weapDef->iHoldFireTime;
        ps->weaponDelay = weapDef->iFireDelay;
        ps->weaponstate = WEAPON_FIRING;
        PM_WeaponSetAnim(ps, 0x13);
        PM_AddEvent(ps, 0x9d);
        PM_SetProneMovementOverride(ps);

        if (weapDef->bCookOffHold)
            return 1;
    }

    PM_StartFireTimers(ps, weapDef);
    if (!ps->weaponDelay || delayedAction)
        PM_ConsumeFireAmmo(ps, weapDef);

    return 1;
}

static qboolean PM_TryFireWeapon(pmove_t *pm, qboolean delayedAction, qboolean triggerPressed)
{
    playerState_t *ps = pm->ps;
    WeaponDef *weapDef;
    int clipIndex;

    if (!ps->weapon)
        return 0;
    if (ps->weaponstate != WEAPON_READY && ps->weaponstate != WEAPON_FIRING &&
        ps->weaponstate != WEAPON_RECHAMBERING)
        return 0;
    if (!(pm->cmd.buttons & PM_WEAPON_BUTTON_ATTACK) && !delayedAction)
        return 0;

    weapDef = bg_weaponDefs[ps->weapon];
    clipIndex = weapDef->iClipIndex;

    /* A new shot always waits for recovery, even if another caller reaches
     * this helper before the state-machine timer gate. Delayed actions finish
     * an already started shot and must retain their original timing. */
    if (!delayedAction && (ps->weaponTime || ps->weaponDelay))
        return 0;

    if (weapDef->bSemiAuto && !delayedAction && !triggerPressed)
        return 0;

    if (ps->weaponstate == WEAPON_RECHAMBERING && ps->weaponTime)
        return 0;

    if (!ps->ammoclip[clipIndex]) {
        if (PM_WeaponCanReloadNow(ps, 1))
            PM_BeginWeaponReload(ps);
        else
            PM_AddEvent(ps, 0x92);
        return 1;
    }

    if (weapDef->weapType == WEAPTYPE_GRENADE)
        return PM_TryStartGrenadeFire(ps, weapDef, delayedAction);

    if (ps->weaponstate != WEAPON_FIRING || !delayedAction)
        PM_StartFireTimers(ps, weapDef);

    if (!ps->weaponDelay || delayedAction)
        PM_ConsumeFireAmmo(ps, weapDef);

    return 1;
}

static void PM_FinishFire(playerState_t *ps)
{
    WeaponDef *weapDef;

    if (!ps->weapon) {
        PM_WeaponSetIdle(ps);
        return;
    }

    weapDef = bg_weaponDefs[ps->weapon];
    if (weapDef->bBoltAction && PM_HasWeaponRechamberBit(ps, ps->weapon) &&
        ps->ammoclip[weapDef->iClipIndex] > 0) {
        PM_StartRechamber(ps, weapDef);
        return;
    }

    PM_WeaponSetIdle(ps);
}

static void PM_RunReloadState(playerState_t *ps, qboolean delayedAction)
{
    WeaponDef *weapDef;

    if (!ps->weapon)
        return;

    weapDef = bg_weaponDefs[ps->weapon];

    if (delayedAction)
        PM_Weapon_ReloadDelayedAction(ps);

    if (ps->weaponTime)
        return;

    if (ps->weaponstate == WEAPON_RELOAD_START ||
        ps->weaponstate == WEAPON_RELOAD_START_INTERUPT) {
        if (PM_Weapon_AllowReload(ps)) {
            PM_SetReloadingState(ps);
            return;
        }
    }

    if ((ps->weaponstate == WEAPON_RELOADING ||
         ps->weaponstate == WEAPON_RELOADING_INTERUPT) &&
        weapDef->bSegmentedReload && ps->weaponstate != WEAPON_RELOADING_INTERUPT &&
        PM_Weapon_AllowReload(ps)) {
        PM_SetReloadingState(ps);
        return;
    }

    if (weapDef->iReloadEndTime) {
        ps->weaponstate = WEAPON_RELOAD_END;
        ps->weaponTime = weapDef->iReloadEndTime;
        ps->weaponDelay = 0;
        PM_WeaponSetAnim(ps, 0x0e);
        PM_AddEvent(ps, 0x99);
        return;
    }

    PM_WeaponSetIdle(ps);
}

static void PM_ReleaseOffhand(playerState_t *ps, WeaponDef *weapDef)
{
    qboolean throwback = (ps->pm_flags & PMF_GRENADE_THROWBACK) != 0;
    if (!throwback && ps->ammoclip[weapDef->iClipIndex] <= 0)
        return;
    BG_AddPredictableEventToPlayerstate(0xa6, ps->offHandIndex, ps);
    if (!throwback && !(ps->eFlags & 0x300))
        ps->ammoclip[weapDef->iClipIndex]--;
    if (!throwback && !ps->ammoclip[weapDef->iClipIndex])
        PM_AddEvent(ps, 0x94);
    ps->pm_flags &= ~PMF_GRENADE_THROWBACK;
}

static void PM_RunOffhandState(pmove_t *pm, qboolean delayedAction)
{
    playerState_t *ps = pm->ps;
    WeaponDef *weapDef;

    if (!ps->offHandIndex)
        return;

    weapDef = bg_weaponDefs[ps->offHandIndex];

    switch (ps->weaponstate) {
    case WEAPON_OFFHAND_INIT:
        if (!ps->weaponTime)
            PM_StartOffhandPrepare(ps);
        break;
    case WEAPON_OFFHAND_PREPARE:
        if (!ps->weaponTime) {
            ps->weaponstate = WEAPON_OFFHAND_HOLD;
            ps->weaponTime = 0;
            ps->weaponDelay = 0;
            ps->pm_flags |= PM_WEAPON_FLAG_OFFHAND;
            ps->grenadeTimeLeft = weapDef->iFuseTime;
        }
        break;
    case WEAPON_OFFHAND_HOLD:
        if ((pm->cmd.buttons & ((ps->pm_flags & PMF_GRENADE_THROWBACK) ? PM_WEAPON_BUTTON_FRAG :
             (PM_WEAPON_BUTTON_ATTACK | PM_WEAPON_BUTTON_FRAG | PM_WEAPON_BUTTON_SMOKE))) &&
            ps->grenadeTimeLeft > 0)
            break;

        ps->weaponstate = WEAPON_OFFHAND_FIRE;
        ps->weaponTime = weapDef->iFireTime;
        ps->weaponDelay = weapDef->iFireDelay;
        if (ps->weaponTime < ps->weaponDelay)
            ps->weaponTime = ps->weaponDelay;
        ps->pm_flags |= PM_WEAPON_FLAG_OFFHAND;
        PM_WeaponSetAnim(ps, 2); /* Original fireAnim, started before release. */
        BG_AnimScriptEvent(ps, 2, 0, 1);
        if (!ps->weaponDelay)
            PM_ReleaseOffhand(ps, weapDef);
        break;
    case WEAPON_OFFHAND_FIRE:
        if (delayedAction)
            PM_ReleaseOffhand(ps, weapDef);
        if (!ps->weaponTime) {
            ps->weaponstate = WEAPON_OFFHAND_END;
            ps->pm_flags &= ~0x810;
            ps->weaponDelay = 0;
            if (ps->weapon) {
                ps->weaponTime = bg_weaponDefs[ps->weapon]->quickRaiseTime;
                PM_WeaponSetAnim(ps, 0x12);
            }
        }
        break;
    case WEAPON_OFFHAND_END:
        if (!ps->weaponTime && !ps->weaponDelay)
            PM_ResetWeaponState(ps);
        break;
    default:
        break;
    }
}

static void PM_RunBinocularState(playerState_t *ps)
{
    switch (ps->weaponstate) {
    case WEAPON_BINOCULARS_INIT:
        if (!ps->weaponTime) {
            ps->weaponstate = WEAPON_BINOCULARS_RAISE;
            ps->weaponTime = 0;
            ps->weaponDelay = 0;
            PM_WeaponSetAnim(ps, 0x11);
        }
        break;
    case WEAPON_BINOCULARS_RAISE:
        if (!ps->weaponTime)
            PM_StartBinocularRaise(ps);
        break;
    case WEAPON_BINOCULARS_HOLD:
        if (!ps->weaponTime) {
            ps->weaponstate = WEAPON_BINOCULARS_IDLE;
            ps->weaponTime = 0;
            ps->weaponDelay = 0;
        }
        break;
    case WEAPON_BINOCULARS_DROP:
        PM_StartBinocularDrop(ps);
        break;
    case WEAPON_BINOCULARS_END:
        if (!ps->weaponTime && !ps->weaponDelay)
            PM_ResetWeaponState(ps);
        break;
    default:
        break;
    }
}

void PM_Weapon(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    qboolean delayedAction;
    qboolean triggerPressed = (pm->cmd.buttons & PM_WEAPON_BUTTON_ATTACK) &&
                              !(ps->pm_flags & PM_WEAPON_FLAG_TRIGGER_HELD);

    /* Track every press, including blocked input during fire/reload/raise.
     * Updating only when a shot starts buffers early presses until recovery
     * ends. Replicated state also keeps command subdivision and replay stable. */
    if (pm->cmd.buttons & PM_WEAPON_BUTTON_ATTACK)
        ps->pm_flags |= PM_WEAPON_FLAG_TRIGGER_HELD;
    else
        ps->pm_flags &= ~PM_WEAPON_FLAG_TRIGGER_HELD;

    if (ps->pm_flags & 0x1000)
        return;

    if (ps->pm_type > 5) {
        ps->weapon = 0;
        return;
    }

    if (ps->eFlags & 0x300)
        return;

    PM_UpdateHoldBreath(pm, pml);

    if (PM_UpdateOffhandCook(pm, pml))
        return;

    delayedAction = PM_UpdateWeaponTimers(pm, pml);

    PM_CheckWeaponChange(pm);
    ps = pm->ps;

    if (PM_TryStartOffhand(pm))
        return;

    if (PM_TryToggleBinoculars(pm))
        return;

    if (ps->pm_flags & 1) {
        const signed char *move = &pm->cmd.forwardmove;
        if (move[0] || move[1])
            ps->aimSpreadScale = 255.0f;
    }

    if (PM_CheckMeleeStart(pm, delayedAction))
        return;

    PM_CheckReloadStart(pm, delayedAction);

    if (!delayedAction && (ps->weaponTime || ps->weaponDelay))
        return;

    switch (ps->weaponstate) {
    case WEAPON_READY:
        PM_TryFireWeapon(pm, delayedAction, triggerPressed);
        break;
    case WEAPON_RAISING:
        if (!ps->weaponTime)
            PM_WeaponSetIdle(ps);
        break;
    case WEAPON_DROPPING:
        if (!ps->weaponTime)
            PM_FinishWeaponChange(pm);
        break;
    case WEAPON_FIRING:
        if (delayedAction)
            PM_TryFireWeapon(pm, delayedAction, triggerPressed);
        else if (!ps->weaponTime) {
            PM_FinishFire(ps);
            /* An automatic gun can start its next shot on the command that
             * expires fireTime. Waiting for another READY command makes its
             * cadence depend on the player's frame/input rate. Semi-auto
             * guns additionally require a fresh, unbuffered trigger press. */
            if (ps->weaponstate == WEAPON_READY)
                PM_TryFireWeapon(pm, 0, triggerPressed);
        }
        break;
    case WEAPON_RECHAMBERING:
        if (delayedAction || (!ps->weaponTime && !ps->weaponDelay)) {
            PM_FinishRechamber(ps);
            if (ps->weaponstate == WEAPON_READY)
                PM_TryFireWeapon(pm, 0, triggerPressed);
        }
        break;
    case WEAPON_RELOADING:
    case WEAPON_RELOADING_INTERUPT:
    case WEAPON_RELOAD_START:
    case WEAPON_RELOAD_START_INTERUPT:
        PM_RunReloadState(ps, delayedAction);
        break;
    case WEAPON_RELOAD_END:
        if (!ps->weaponTime)
            PM_WeaponSetIdle(ps);
        break;
    case WEAPON_MELEE_INIT:
        if (delayedAction)
            PM_FinishMeleeDelay(ps);
        break;
    case WEAPON_MELEE_FIRE:
        if (!ps->weaponTime)
            PM_WeaponSetIdle(ps);
        break;
    case WEAPON_OFFHAND_INIT:
    case WEAPON_OFFHAND_PREPARE:
    case WEAPON_OFFHAND_HOLD:
    case WEAPON_OFFHAND_FIRE:
    case WEAPON_OFFHAND_END:
        PM_RunOffhandState(pm, delayedAction);
        break;
    case WEAPON_BINOCULARS_INIT:
    case WEAPON_BINOCULARS_RAISE:
    case WEAPON_BINOCULARS_HOLD:
    case WEAPON_BINOCULARS_IDLE:
    case WEAPON_BINOCULARS_DROP:
    case WEAPON_BINOCULARS_END:
        PM_RunBinocularState(ps);
        break;
    default:
        PM_ResetWeaponState(ps);
        break;
    }
}

WeaponDef *BG_LoadWeaponDef(const char *folder, const char *name)
{
    WeaponDef *weapDef = BG_LoadWeaponDefInternal(folder, name);

    if (weapDef)
        return weapDef;

    if (I_stricmp(name, "defaultweapon_mp") == 0)
        Com_Error(1, "\x15"
                     "BG_LoadWeaponDef: Could not find default weapon");

    weapDef = BG_LoadWeaponDefInternal(folder, "defaultweapon_mp");
    if (!weapDef)
        Com_Error(1, "\x15"
                     "BG_LoadWeaponDef: Could not find default weapon");

    return weapDef;
}

int BG_SetupWeaponDef(WeaponDef *weapDef, void (*regWeap)(int))
{
    int weapIndex;

    if (!weapDef)
        return 0;

    weapIndex = BG_FindWeaponIndexForName(weapDef->szInternalName);
    if (weapIndex)
        return weapIndex;

    if (bg_iNumWeapons >= 127)
        Com_Error(1, "\x15"
                     "Exceeded maximum weapon count");

    weapIndex = ++bg_iNumWeapons;
    bg_weaponDefs[weapIndex] = weapDef;

    BG_SetupAmmoIndexes(weapIndex);
    BG_SetupSharedAmmoIndexes(weapIndex);
    BG_SetupClipIndexes(weapIndex);
    BG_SetupWeaponItem(weapIndex, weapDef);

    if (weapDef->offhandClass != OFFHAND_CLASS_NONE && weapDef->weapType != WEAPTYPE_GRENADE) {
        Com_Printf("[%s] Only grenades are currently supported for off hand use\n", weapDef->szInternalName);
    } else if (weapDef->weapType == WEAPTYPE_GRENADE && weapDef->offhandClass == OFFHAND_CLASS_NONE) {
        Com_Printf("[%s] No offhand class set\n", weapDef->szInternalName);
    }

    if (weapDef->szAltWeaponName && weapDef->szAltWeaponName[0]) {
        int altWeaponIndex = BG_GetWeaponIndexForName(weapDef->szAltWeaponName, (BG_RegisterWeapon)regWeap);
        WeaponDef *altWeaponDef;

        if (!altWeaponIndex)
            Com_Error(1, "\x15"
                         "could not find altWeapon '%s' for weapon '%s'",
                      weapDef->szAltWeaponName, weapDef->szInternalName);

        weapDef->iAltWeaponIndex = altWeaponIndex;
        altWeaponDef = bg_weaponDefs[altWeaponIndex];

        if (weapDef->weapSlot != altWeaponDef->weapSlot) {
            Com_Error(1, "\x15"
                         "weapon '%s' does not have same weaponSlot setting as its alt weapon '%s'",
                      weapDef->szInternalName, altWeaponDef->szInternalName);
        }

        if (weapDef->bSlotStackable != altWeaponDef->bSlotStackable) {
            Com_Error(1, "\x15"
                         "weapon '%s' does not have same slotStackable setting as its alt weapon '%s'",
                      weapDef->szInternalName, altWeaponDef->szInternalName);
        }
    } else {
        weapDef->iAltWeaponIndex = 0;
    }

    if (regWeap)
        regWeap(weapIndex);

    return weapIndex;
}

int BG_GetWeaponIndexForName(const char *name, BG_RegisterWeapon regWeap)
{
    WeaponDef *weapDef;
    int weapIndex;

    if (!name[0] || I_stricmp(name, "none") == 0)
        return 0;

    weapIndex = BG_FindWeaponIndexForName(name);
    if (weapIndex)
        return weapIndex;

    weapDef = BG_LoadWeaponDef("mp", name);
    if (weapDef)
        return BG_SetupWeaponDef(weapDef, (void (*)(int))regWeap);

    Com_Printf("Couldn't find weapon \"%s\"\n", name);
    return 0;
}

void BG_FillInAmmoItems(BG_RegisterWeapon regWeap)
{
    int *numItems = (int *)imp_bg_numItems;
    gitem_t *itemList = (gitem_t *)imp_bg_itemlist;
    int i;

    for (i = 0x81; i < *numItems; i++) {
        gitem_t *item = &itemList[i];
        char name[64];
        char *space;
        int weaponIndex;
        WeaponDef *weapDef;

        if (item->giType != IT_AMMO)
            continue;

        I_strncpyz(name, item->pickup_name, sizeof(name));
        for (space = name; *space; space++) {
            if (*space == ' ') {
                *space = '\0';
                break;
            }
        }

        weaponIndex = BG_GetWeaponIndexForName(name, regWeap);
        if (weaponIndex) {
            weapDef = bg_weaponDefs[weaponIndex];
            item->giTag = weaponIndex;
            item->giAmmoIndex = weapDef->iAmmoIndex;
            item->giClipIndex = weapDef->iClipIndex;
            return;
        }

        Com_Printf("^3WARNING^7: Could not find weapon for ammo item %s\n", item->pickup_name);
        weaponIndex = BG_GetWeaponIndexForName("defaultweapon_mp", regWeap);
        if (!weaponIndex) {
            Com_Error(1, "\x15"
                         "BG_FillInAmmoItems: weapon def for default weapon could not be found!");
        }

        weapDef = bg_weaponDefs[weaponIndex];
        item->giTag = 1;
        item->giAmmoIndex = weapDef->iAmmoIndex;
        item->giClipIndex = weapDef->iClipIndex;
    }
}

void CG_SetWeaponDefToDefaultWeapon(int iWeapon)
{
    WeaponDef *weapDef = bg_weaponDefs[iWeapon];
    int defaultWeaponNum = BG_GetWeaponIndexForName("defaultweapon_mp", 0);
    WeaponDef *defaultWeaponDef;
    int i;

    if (!defaultWeaponNum) {
        Com_Error(1, "\x15"
                     "CG_SetWeaponDefToDefaultWeapon: weapon def for default weapon could not be found!");
    }

    defaultWeaponDef = bg_weaponDefs[defaultWeaponNum];
    weapDef->szGunXModel = defaultWeaponDef->szGunXModel;
    weapDef->szHandXModel = defaultWeaponDef->szHandXModel;

    for (i = 0; i < 23; i++)
        weapDef->szXAnims[i] = defaultWeaponDef->szXAnims[i];
}

static void BG_TrackWeaponAngle(vec_t *value, float target, float rate, float frametime)
{
    float cur = *value;
    float delta = (target - cur) * frametime * rate;
    float minStep = frametime * 0.10000000149011612f;

    if (target > cur) {
        if (delta < minStep)
            delta = minStep;
        cur += delta;
        if (cur > target)
            cur = target;
    } else if (target < cur) {
        if (delta > -minStep)
            delta = -minStep;
        cur += delta;
        if (cur < target)
            cur = target;
    }

    *value = cur;
}

static void BG_AddWeaponMovementAngles(weaponState_t *ws, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = ws->ps;
    const vec_t *move;
    float minSpeed;
    float target[3];
    float frac;
    float rate;
    int i;

    if (ps->eFlags & 8) {
        move = weapDef->vProneMove;
        minSpeed = weapDef->fProneRotMinSpeed;
    } else if (ps->eFlags & 4) {
        move = weapDef->vDuckedMove;
        minSpeed = weapDef->fDuckedRotMinSpeed;
    } else {
        move = weapDef->vStandMove;
        minSpeed = weapDef->fStandRotMinSpeed;
    }

    if (ws->xyspeed <= minSpeed || ps->weaponstate == 5) {
        target[0] = 0.0f;
        target[1] = 0.0f;
        target[2] = 0.0f;
    } else {
        float topSpeed = (float)ps->speed - minSpeed;

        if (topSpeed == 0.0f)
            frac = 0.0f;
        else
            frac = (ws->xyspeed - minSpeed) / topSpeed;

        if (frac < 0.0f)
            frac = 0.0f;
        if (frac > 1.0f)
            frac = 1.0f;

        target[0] = move[0] * frac;
        target[1] = move[1] * frac;
        target[2] = move[2] * frac;
    }

    rate = (ps->viewHeightCurrent == 11.0f) ? weapDef->fPosProneRotRate : weapDef->fPosRotRate;
    for (i = 0; i < 3; i++)
        BG_TrackWeaponAngle(&ws->vLastMoveAng[i], target[i], rate, ws->frametime);

    if (ps->fWeaponPosFrac < 0.5f) {
        float scale = 1.0f;

        if (ps->fWeaponPosFrac != 0.0f)
            scale = 1.0f - ps->fWeaponPosFrac * 2.0f;

        angles[0] += ws->vLastMoveAng[0] * scale;
        angles[1] += ws->vLastMoveAng[1] * scale;
        angles[2] += ws->vLastMoveAng[2] * scale;
    }
}

static void BG_TrackWeaponIdleFactor(weaponState_t *ws, float target)
{
    float step = ws->frametime * 0.5f;

    if (ws->fLastIdleFactor < target) {
        ws->fLastIdleFactor += step;
        if (ws->fLastIdleFactor > target)
            ws->fLastIdleFactor = target;
    } else if (ws->fLastIdleFactor > target) {
        ws->fLastIdleFactor -= step;
        if (ws->fLastIdleFactor < target)
            ws->fLastIdleFactor = target;
    }
}

static void BG_AddWeaponIdleAngles(weaponState_t *ws, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = ws->ps;
    float f = ps->fWeaponPosFrac;
    float idleAmount;
    float idleSpeed;
    float idleFactor;
    float amount;

    if (weapDef->bADSPositionInfo) {
        idleAmount = weapDef->fHipIdleAmount + (weapDef->fAdsIdleAmount - weapDef->fHipIdleAmount) * f;
        idleSpeed = weapDef->hipIdleSpeed + (weapDef->adsIdleSpeed - weapDef->hipIdleSpeed) * f;
    } else {
        idleAmount = weapDef->fHipIdleAmount;
        if (idleAmount == 0.0f) {
            idleAmount = 80.0f;
            idleSpeed = 1.0f;
        } else {
            idleSpeed = weapDef->hipIdleSpeed;
        }
    }

    if (ps->eFlags & 8)
        idleFactor = weapDef->fIdleProneFactor;
    else if (ps->eFlags & 4)
        idleFactor = weapDef->fIdleCrouchFactor;
    else
        idleFactor = 1.0f;

    BG_TrackWeaponIdleFactor(ws, idleFactor);

    amount = idleAmount * ws->fLastIdleFactor;
    if (weapDef->overlayReticle)
        amount *= 1.0f - f;

    *ws->weapIdleTime += (int)(idleSpeed * ws->frametime * 1000.0f);

    angles[2] += sinf((float)*ws->weapIdleTime * 0.0005000000237487257f) * amount * 0.009999999776482582f;
    angles[1] += sinf((float)*ws->weapIdleTime * 0.000699999975040555f) * amount * 0.009999999776482582f;
    angles[0] += sinf((float)*ws->weapIdleTime * 0.0010000000474974513f) * amount * 0.009999999776482582f;
}

static void BG_AddWeaponBobAngles(weaponState_t *ws, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = ws->ps;
    float f = ps->fWeaponPosFrac;
    float cycle = BG_GetBobCycle(ps) + 0.7853981633974483f;
    float speed = ws->xyspeed * 0.1599999964237213f;
    float pitch = -BG_GetVerticalBobFactor(ps, cycle, speed, 10.0f);
    float yaw = -BG_GetHorizontalBobFactor(ps, cycle, speed, 10.0f);
    float roll = BG_GetHorizontalBobFactor(ps, cycle - 0.4712389167638204f, speed * 1.5f, 10.0f);

    if (roll > 0.0f)
        roll = 0.0f;

    if (f != 0.0f) {
        float factor = 1.0f - (1.0f - weapDef->fAdsBobFactor) * f;

        pitch *= factor;
        yaw *= factor;
        roll *= factor;
    }

    if (weapDef->overlayReticle) {
        float scale = 1.0f - f;

        pitch *= scale;
        yaw *= scale;
        roll *= scale;
    }

    angles[0] += pitch;
    angles[1] += yaw;
    angles[2] += roll;
}

static void BG_AddWeaponDamageKick(weaponState_t *ws, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = ws->ps;
    float f = ps->fWeaponPosFrac;
    float factor = f * 0.5f + 0.5f;
    float attack;
    float decay;
    float frac;
    int elapsed;

    if (!ws->damageTime)
        return;

    if (f != 0.0f && weapDef->overlayReticle)
        factor *= 1.0f - f * 0.75f;

    attack = 100.0f * factor;
    decay = 400.0f * factor;
    elapsed = ws->time - ws->damageTime;

    if ((float)elapsed < attack) {
        frac = GetLeanFraction((float)elapsed / attack) * factor;
    } else {
        float fade = 1.0f - ((float)elapsed - attack) / decay;

        if (fade <= 0.0f)
            return;

        frac = (1.0f - GetLeanFraction(1.0f - fade)) * factor;
    }

    angles[0] += frac * ws->v_dmg_pitch * 0.5f;
    angles[1] -= frac * ws->v_dmg_roll;
    angles[2] += frac * ws->v_dmg_roll * 0.5f;
}

static void BG_AddWeaponGunKick(weaponState_t *ws, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = ws->ps;
    float f = ps->fWeaponPosFrac;
    float fGunKickAccel;
    float fGunKickSpeedMax;
    float fGunKickSpeedDecay;
    float fGunKickStaticDecay;
    float timeLeft;

    if (!weapDef->bADSPositionInfo)
        goto add_offset;

    fGunKickAccel = weapDef->fHipGunKickAccel + (weapDef->fAdsGunKickAccel - weapDef->fHipGunKickAccel) * f;
    fGunKickSpeedMax = weapDef->fHipGunKickSpeedMax + (weapDef->fAdsGunKickSpeedMax - weapDef->fHipGunKickSpeedMax) * f;
    fGunKickSpeedDecay = weapDef->fHipGunKickSpeedDecay + (weapDef->fAdsGunKickSpeedDecay - weapDef->fHipGunKickSpeedDecay) * f;
    fGunKickStaticDecay = weapDef->fHipGunKickStaticDecay + (weapDef->fAdsGunKickStaticDecay - weapDef->fHipGunKickStaticDecay) * f;

    timeLeft = ws->frametime;
    while (timeLeft > 0.0f) {
        float step = timeLeft;
        qboolean pitchDone;
        qboolean yawDone;

        if (step > 0.004999999888241291f)
            step = 0.004999999888241291f;

        pitchDone = BG_CalculateWeaponPosition_GunRecoil_SingleAngle(&ws->vGunOffset[0], &ws->vGunSpeed[0],
                                                                     step, weapDef->fGunMaxPitch, fGunKickAccel, fGunKickSpeedMax, fGunKickSpeedDecay, fGunKickStaticDecay);
        yawDone = BG_CalculateWeaponPosition_GunRecoil_SingleAngle(&ws->vGunOffset[1], &ws->vGunSpeed[1],
                                                                   step, weapDef->fGunMaxYaw, fGunKickAccel, fGunKickSpeedMax, fGunKickSpeedDecay, fGunKickStaticDecay);

        if (pitchDone && yawDone)
            break;

        timeLeft -= step;
    }

add_offset:
    angles[0] += ws->vGunOffset[0];
    angles[1] += ws->vGunOffset[1];
    angles[2] += ws->vGunOffset[2];
}

void BG_CalculateWeaponAngles(weaponState_t *ws, vec_t *angles)
{
    playerState_t *ps = ws->ps;
    WeaponDef *weapDef = bg_weaponDefs[BG_GetViewmodelWeaponIndex(ps)];

    angles[0] = 0.0f;
    angles[1] = 0.0f;
    angles[2] = 0.0f;

    if (ps->leanf != 0.0f)
        angles[2] += GetLeanFraction(ps->leanf) * -2.0f;

    if (weapDef->bADSPositionInfo)
        angles[0] += ps->fWeaponPosFrac * weapDef->fAdsAimPitch;

    BG_AddWeaponMovementAngles(ws, angles, weapDef);
    BG_AddWeaponIdleAngles(ws, angles, weapDef);
    BG_AddWeaponBobAngles(ws, angles, weapDef);
    BG_AddWeaponDamageKick(ws, angles, weapDef);
    BG_AddWeaponGunKick(ws, angles, weapDef);

    angles[0] = AngleSubtract(angles[0], ws->swayAngles[0]);
    angles[1] = AngleSubtract(angles[1], ws->swayAngles[1]);
}

static void BG_TrackViewIdleFactor(viewState_t *vs, float target)
{
    float step = vs->frametime * 0.5f;

    if (vs->fLastIdleFactor < target) {
        vs->fLastIdleFactor += step;
        if (vs->fLastIdleFactor > target)
            vs->fLastIdleFactor = target;
    } else if (vs->fLastIdleFactor > target) {
        vs->fLastIdleFactor -= step;
        if (vs->fLastIdleFactor < target)
            vs->fLastIdleFactor = target;
    }
}

static void BG_AddViewDamageKick(viewState_t *vs, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = vs->ps;
    float f = ps->fWeaponPosFrac;
    float factor;
    float frac;
    int elapsed;

    if (!vs->damageTime)
        return;

    factor = 1.0f - f * 0.5f;
    if (f != 0.0f && weapDef->overlayReticle)
        factor *= f * 0.5f + 1.0f;

    elapsed = vs->time - vs->damageTime;
    if ((float)elapsed < 100.0f) {
        frac = GetLeanFraction((float)elapsed / 100.0f);
    } else {
        float fade = 1.0f - ((float)elapsed - 100.0f) / 400.0f;

        if (fade <= 0.0f)
            return;

        frac = 1.0f - GetLeanFraction(1.0f - fade);
    }

    frac *= factor;
    angles[0] += frac * vs->v_dmg_pitch;
    angles[2] += frac * vs->v_dmg_roll;
}

static void BG_AddViewIdle(viewState_t *vs, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = vs->ps;
    float f = ps->fWeaponPosFrac;
    float idleAmount;
    float idleSpeed;
    float idleFactor;
    float amount;

    if (!weapDef->overlayReticle)
        return;

    if (weapDef->bADSPositionInfo) {
        idleAmount = weapDef->fHipIdleAmount +
                     (weapDef->fAdsIdleAmount - weapDef->fHipIdleAmount) * f;
        idleSpeed = weapDef->hipIdleSpeed +
                    (weapDef->adsIdleSpeed - weapDef->hipIdleSpeed) * f;
    } else {
        idleAmount = weapDef->fHipIdleAmount;
        if (idleAmount == 0.0f) {
            idleAmount = 80.0f;
            idleSpeed = 1.0f;
        } else {
            idleSpeed = weapDef->hipIdleSpeed;
        }
    }

    if (ps->eFlags & 8)
        idleFactor = weapDef->fIdleProneFactor;
    else if (ps->eFlags & 4)
        idleFactor = weapDef->fIdleCrouchFactor;
    else
        idleFactor = 1.0f;

    if (f != 0.0f)
        BG_TrackViewIdleFactor(vs, idleFactor);

    amount = idleAmount * vs->fLastIdleFactor * f * ps->holdBreathScale;
    *vs->weapIdleTime += (int)(idleSpeed * vs->frametime * 1000.0f * ps->holdBreathScale);

    angles[1] += sinf((float)*vs->weapIdleTime * 0.000699999975040555f) * amount * 0.009999999776482582f;
    angles[0] += sinf((float)*vs->weapIdleTime * 0.0010000000474974513f) * amount * 0.009999999776482582f;
}

static void BG_AddScopedViewBob(viewState_t *vs, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = vs->ps;
    float f = ps->fWeaponPosFrac;
    float cycle;
    float speed;
    float pitch;
    float yaw;
    float roll;

    if (!weapDef->overlayReticle)
        return;

    cycle = BG_GetBobCycle(ps) + 0.7853981633974483f;
    speed = vs->xyspeed * 0.1599999964237213f;

    pitch = -BG_GetVerticalBobFactor(ps, cycle, speed, 10.0f);
    yaw = -BG_GetHorizontalBobFactor(ps, cycle, speed, 10.0f);
    roll = BG_GetHorizontalBobFactor(ps, cycle - 0.4712389167638204f, speed * 1.5f, 10.0f);
    if (roll > 0.0f)
        roll = 0.0f;

    if (f != 0.0f) {
        float factor = 1.0f - (1.0f - weapDef->fAdsBobFactor) * f;

        pitch *= factor;
        yaw *= factor;
        roll *= factor;
    }

    angles[0] += pitch * f;
    angles[1] += yaw * f;
    angles[2] += roll * f;
}

static void BG_AddAdsViewBob(viewState_t *vs, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = vs->ps;
    float f = ps->fWeaponPosFrac;
    float scale;
    float cycle;

    if (ps->eFlags & 0x300)
        return;
    if (f == 0.0f || weapDef->fAdsViewBobMult == 0.0f)
        return;

    scale = f * weapDef->fAdsViewBobMult;
    cycle = BG_GetBobCycle(ps);
    angles[0] -= BG_GetVerticalBobFactor(ps, cycle, vs->xyspeed, 45.0f) * scale;
    angles[1] -= BG_GetHorizontalBobFactor(ps, cycle, vs->xyspeed, 45.0f) * scale;
}

void BG_CalculateViewAngles(viewState_t *vs, vec_t *angles)
{
    playerState_t *ps = vs->ps;
    WeaponDef *weapDef;

    angles[0] = 0.0f;
    angles[1] = 0.0f;
    angles[2] = 0.0f;

    weapDef = bg_weaponDefs[BG_GetViewmodelWeaponIndex(ps)];

    BG_AddViewDamageKick(vs, angles, weapDef);
    BG_AddViewIdle(vs, angles, weapDef);
    BG_AddScopedViewBob(vs, angles, weapDef);
    BG_AddAdsViewBob(vs, angles, weapDef);
}
