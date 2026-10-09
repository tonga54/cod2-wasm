#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"

extern const dvar_t *cg_weaponCycleDelay;
extern const dvar_t *loc_warnings;
extern const dvar_t *loc_warningsAsErrors;

extern const dvar_t *cg_blood;
extern const dvar_t *cg_brass;
extern const dvar_t *cg_drawGun;
extern const dvar_t *cg_tracerChance;
extern const dvar_t *cg_tracerScale;
extern const dvar_t *cg_tracerScaleDistRange;
extern const dvar_t *cg_tracerScaleMinDist;
extern const dvar_t *cg_tracerSpeed;
extern const dvar_t *cg_tracerWidth;
extern const char *CL_GetConfigString(int index);
extern char *getenv(const char *name);
extern scr_const_t scr_const;

extern vec3_t ejectBrassCasingOrigin;
extern int removeMeWhenMPStopsCrashingInHere;
extern void *BG_GetWeaponDef(int weapIndex);
extern void CG_SetWeaponDefToDefaultWeapon(int weaponNum);
extern void SCR_UpdateScreen(void);
extern struct XAnim_s *XAnimCreateAnims(const char *debugName, int size, void *Alloc);
extern void XAnimBlend(struct XAnim_s *anims, unsigned int animIndex, const char *name, unsigned int children, unsigned int num, unsigned int flags);
extern XAnimParts *XAnimPrecache(const char *name, Alloc_t Alloc);
extern void XAnimCreate(struct XAnim_s *anims, unsigned int animIndex, const char *name);
extern void *XAnimCreateTree(void *anims, void *Alloc);
extern void XAnimClearTreeGoalWeights(XAnimTree *tree, unsigned int animIndex, float blendTime);
extern int XAnimSetGoalWeight(void *tree, unsigned int animIndex, float goalWeight, float goalTime, float rate, unsigned int notifyName, unsigned int notifyType, int bRestart);
extern void XAnimSetTime(void *tree, int animIndex, float time);
extern int XAnimIsLooped(struct XAnim_s *anims, int animIndex);
extern int XAnimGetLengthMsec(struct XAnim_s *anims, int animIndex);
extern int XModelBad(void *model);
extern void *Com_GetClientDObj(int entityNum, int localClientNum);
extern void DObjUpdateClientInfo(struct DObj_s *obj, float timescale);
extern void Com_Error(int code, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern int CL_RegisterMaterial(const char *name, int flags);
extern int CL_RegisterMaterialNoMip(const char *name, int flags);
extern struct XModel *CL_RegisterModel(const char *name);
extern int FX_RegisterEffect(const char *name);
extern void *Com_FindSoundAlias(const char *name);
extern const char *SEH_StringEd_GetString(const char *str);
extern void Com_ClientDObjCreate(DObjModel_s *dobjModels, int numModels, struct XAnimTree_s *tree, int handle);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern void CL_RegisterHudMsgIconMaterial(const char *name);
extern qboolean CG_ScoreboardDisplayed(void);
extern void CG_ScrollScoreboardUp(void);
extern void CG_ScrollScoreboardDown(void);
extern const char *CG_Argv(int arg);
extern int BG_GetWeaponSlotForName(const char *pszSlotName);
extern void CG_MenuShowNotify(int menuToShow);
extern void CL_SetADS(int ads);
extern int atoi(const char *nptr);
extern int BG_GetWeaponIndexForName(const char *name, BG_RegisterWeapon regWeap);
extern qboolean CG_DObjGetWorldTagPos(const centity_t *cent, struct DObj_s *obj, unsigned int tagName, vec_t *pos);
extern const char *SL_ConvertToString(unsigned int stringValue);
extern void Com_DPrintf(const char *fmt, ...);
extern int BG_GetNumWeapons(void);
extern int BG_IsPlayerWeaponInSlot(const playerState_t *pPS, int iWeaponIndex, qboolean bAnyMode);
extern int BG_GetStackSlotForWeapon(const playerState_t *pPS, int iWeaponIndex, weapSlot_t preferedSlot);
extern Bool BG_DoesWeaponNeedSlot(int weapIndex);
extern int BG_WeaponAmmo(const playerState_t *ps, int weapon);
extern qboolean BG_IsPlayerWeaponAnAlt(int iWeaponIndex, int iAltIndex);
extern int FX_GetBoneIndex(int entNum, unsigned int bone);
extern void FX_PlayEntityEffect(void *fx, const vec_t *org, vec3_t *axis, const int *bolt);
extern void FX_PlayEffect(void *fx, const vec_t *org, const vec_t *fwd);
extern int strcmp(const char *s1, const char *s2);
extern struct XAnimTree_s *DObjGetTree(void *obj);
extern void Com_SafeClientDObjFree(int handle);
extern int BG_GetViewmodelWeaponIndex(const playerState_t *ps);
extern Bool XAnimHasFinished(const struct XAnimTree_s *tree, unsigned int animIndex);
extern int BG_ClipForWeapon(int weapon);
extern const vec_t Vec3Normalize(vec_t *v);
extern const vec_t Vec3NormalizeTo(const vec_t *v, vec_t *out);
extern const vec_t Vec3Distance(const vec_t *v1, const vec_t *v2);
extern localEntity_t *CG_AllocLocalEntity(void);
extern int rand(void);
extern float sqrtf(float x);
extern int CG_PlaySoundAlias(int entitynum, const vec_t *origin, snd_alias_list_t *aliasList);
extern void CL_DefaultVertexFrames(int vertCount, GfxWorldVertex *verts);
extern void CL_AddPolyToScene(MaterialHandle mtlHandle, int lmapIndex, int vertCount, const GfxWorldVertex *verts);
extern void BG_WeaponFireRecoil(const playerState_t *ps, vec_t *vGunSpeed, vec_t *kickAVel);
extern qboolean CG_DObjGetViewModelTagPos(struct DObj_s *obj, unsigned int tagName, vec_t *pos);
extern void BG_EvaluateTrajectory(const trajectory_t *tr, int atTime, vec_t *result);
extern void CG_CompassAddWeaponPingInfo(centity_t *cent, const vec_t *origin, int msec);
extern qboolean CL_DObjCreateSkelForBones(const struct DObj_s *obj, int *partBits, int localClientNum);
extern void DObjCalcAnim(struct DObj_s *obj, int *partBits);
extern void DObjCalcSkel(struct DObj_s *obj, int *partBits);
extern int DObjGetClientNotifyList(XAnimNotify **notifyList);
extern int CG_PlayClientSoundAlias(snd_alias_list_t *aliasList);
extern int stricmp(const char *s1, const char *s2);
extern void AxisCopy(vec3_t *in, vec3_t *out);
extern void AxisToAngles(vec3_t *axis, vec_t *angles);
extern void AnglesToAxis(const vec_t *angles, vec3_t *axis);
extern void MatrixMultiply(const float (*in1)[3], const float (*in2)[3], float (*out)[3]);
extern void AddLeanToPosition(vec_t *position, float viewAngle, float leanFrac, float maxStand, float maxCrouch);
extern float DiffTrack(float tgt, float cur, float rate, float deltaTime);
extern int DObjGetBoneIndex(void *obj, unsigned int boneName);
extern DObjAnimMat *DObjGetRotTransArray(const struct DObj_s *obj);
extern void MatrixTransformVector43(const float *in1, const float (*in2)[3], float *out);
extern GfxEntity *R_AddRefEntityToScene(const GfxEntity *refEnt, GfxModel sceneModel, const centity_t *cent);
extern void R_SkinGfxEntityDelayed(GfxEntity *ent);
extern void SND_SetChannelVolumes(int priority, const float *channelvolume, int fademsec);
extern void SND_DeactivateChannelVolumes(int priority, int fademsec);
extern qboolean CG_GetWeapReticleZoom(float *fZoom);
extern qboolean BG_IsAimDownSightWeapon(int iWeapon);
extern void BG_CalculateWeaponPosition_Sway(playerState_t *ps, vec_t *swayViewAngles, vec_t *swayOffset, vec_t *swayAngles, float ssSwayScale, int frametime);
extern void BG_CalculateWeaponAngles(weaponState_t *ws, vec_t *angles);
static const int iSlotPreferenceOrder[2];
extern weapSlot_t (*s_barrelTags[4])[64];

COD2_ASSERT_FIELD(WeaponDef, szGunXModel,       0x0c);
COD2_ASSERT_FIELD(WeaponDef, weapSlot,          0x80);
COD2_ASSERT_FIELD(WeaponDef, bSlotStackable,    0x88);
COD2_ASSERT_FIELD(WeaponDef, iHoldFireTime,     0x210);
COD2_ASSERT_FIELD(WeaponDef, iMeleeTime,        0x214);
COD2_ASSERT_FIELD(WeaponDef, iReloadEmptyTime,  0x21c);
COD2_ASSERT_FIELD(WeaponDef, iReloadStartTime,  0x224);
COD2_ASSERT_FIELD(WeaponDef, iDropTime,         0x230);
COD2_ASSERT_FIELD(WeaponDef, iRaiseTime,        0x234);
COD2_ASSERT_FIELD(WeaponDef, iAltDropTime,      0x238);
COD2_ASSERT_FIELD(WeaponDef, iAltRaiseTime,     0x23c);
COD2_ASSERT_FIELD(WeaponDef, quickDropTime,     0x240);
COD2_ASSERT_FIELD(WeaponDef, quickRaiseTime,    0x244);
COD2_ASSERT_FIELD(WeaponDef, bBoltAction,       0x328);
COD2_ASSERT_FIELD(WeaponDef, bADSPositionInfo,  0x32c);
COD2_ASSERT_FIELD(WeaponDef, iAltWeaponIndex,   0x36c);
COD2_ASSERT_FIELD(gitem_t,   pickup_sound,      0x04);
COD2_ASSERT_FIELD(gitem_t,   icon,              0x10);
COD2_ASSERT_FIELD(gitem_t,   giType,            0x1c);
COD2_ASSERT_FIELD(gitem_t,   giTag,             0x20);
COD2_ASSERT_FIELD(itemInfo_t, registered,       0x00);
COD2_ASSERT_FIELD(itemInfo_t, icons,            0x0c);
COD2_ASSERT_FIELD(itemInfo_t, pickupSound,      0x1c);
COD2_ASSERT_FIELD(itemInfo_t, ammoPickupSound,  0x20);
COD2_ASSERT_FIELD(centity_t, localClientNum, 0x220);
COD2_ASSERT_FIELD(centity_t, bMuzzleFlash,   0x1e2);

static void __attribute_regparm__(3) CG_PlayADSAnim(void *unused, void *pAnimTree, int animIndex);
static inline __attribute__((always_inline)) qboolean CG_CanCycleWeapon(cg_t **cgOut, int *serverTimeOut);
int CG_WeaponDObjHandle(int weaponNum);
#ifndef __EMSCRIPTEN__
void CG_Weapons_SetToDefault(int weaponNum, DObjModel_s *dobjModels);
#else
void CG_Weapons_SetToDefault(int weaponNum, weaponInfo_s (*dobjModels)[4]);
#endif
void CG_HoldBreathInit(void);
void CG_SetupWeaponDef(void);
void CG_SelectWeaponIndex(int weaponIndex);
static qboolean __attribute_regparm__(3) CG_CalcMuzzlePoint(int entityNum, vec_t *muzzle, unsigned int flashTag);
void CG_EjectWeaponBrass(entityState_t *ent, int event);
void CG_FireWeapon(centity_t *cent, int event, int barrel);
void CG_RegisterWeapon(int weaponNum);
void CG_RegisterItemVisuals(int itemNum);
void CG_RegisterItems(void);
void CG_UpdateHandViewmodels(const char *handModel);
void CG_SpawnTracer(vec_t *pstart, vec_t *pend);
void CG_WhizbySound(vec_t *vStart, vec_t *vEnd);
void CG_BulletHitClientEvent(int sourceEntityNum, vec_t *position, int surfType, int event);
void CG_AddPlayerWeapon(GfxEntity *parent, playerState_t *ps, centity_t *cent, qboolean bDrawGun);
void CG_AddViewWeapon(playerState_t *ps);
void CG_BulletHitEvent(int sourceEntityNum, vec_t *position, vec_t *normal, vec_t *reflected, int surfType, int event);
void CG_DrawTracer(vec_t *start, vec_t *finish);
void CG_WeaponSlot_f(qboolean next, qboolean ignoreEmpty);
qboolean CG_SelectFirstWeaponNotInSlot(qboolean bNext, qboolean bIgnoreEmpty);
void CG_CycleWeap(qboolean bNext, qboolean bIgnoreEmpty);
void CG_OutOfAmmoChange(void);
void CG_PrevWeapon_f(void);
void CG_NextWeapon_f(void);
void CG_WeaponRunXModelAnims(playerState_t *ps, weaponInfo_t *weapInfo);
void CG_UpdateViewWeaponAnim(playerState_t *ps);

static void CG_PlayADSAnim_impl(void *pAnimTree, int animIndex)
{
    float adsProgress;

    if (animIndex == 0x15) {

        XAnimSetGoalWeight(pAnimTree, 0x15, 1.0f, 0.5f, 0.0f, 0, 1, 0);
        XAnimSetGoalWeight(pAnimTree, 0x16, 0.0f, 0.5f, 0.0f, 0, 0, 0);
    } else {

        XAnimSetGoalWeight(pAnimTree, 0x15, 0.0f, 0.5f, 0.0f, 0, 0, 0);
        XAnimSetGoalWeight(pAnimTree, 0x16, 1.0f, 0.5f, 0.0f, 0, 1, 0);
    }

    adsProgress = cg->predictedPlayerState.fWeaponPosFrac;
    XAnimSetTime(pAnimTree, 0x15, adsProgress);
    XAnimSetTime(pAnimTree, 0x16, 1.0f - adsProgress);
}

static void __attribute_regparm__(3) CG_PlayADSAnim(void *unused, void *pAnimTree, int animIndex)
{
    (void)unused;
    CG_PlayADSAnim_impl(pAnimTree, animIndex);
}

int CG_WeaponDObjHandle(int weaponNum)
{
    return weaponNum + 0x400;
}

static inline __attribute__((always_inline)) qboolean CG_HasAssetName(const char *name)
{
    return name && name[0];
}

static inline __attribute__((always_inline)) int CG_WeaponDefIntAt(const WeaponDef *weapDef, int offset)
{
    return *(const int *)((const byte *)weapDef + offset);
}

static inline __attribute__((always_inline)) float CG_WeaponAnimRate(struct XAnim_s *anims, int animIndex, int durationMsec)
{
    if (durationMsec <= 0)
        return 0.0f;

    return (float)XAnimGetLengthMsec(anims, animIndex) / (float)durationMsec;
}

static const char *CG_TranslateWeaponString(const WeaponDef *weapDef, const char *text,
                                            const char *errorFmt, const char *warningFmt)
{
    const char *translated;
    const dvar_t *locWarnings;
    const dvar_t *locWarningsAsErrors;

    translated = SEH_StringEd_GetString(text);
    if (translated)
        return translated;

    locWarnings = loc_warnings;
    if (locWarnings && locWarnings->current.enabled) {
        locWarningsAsErrors = loc_warningsAsErrors;
        if (locWarningsAsErrors && locWarningsAsErrors->current.enabled)
            Com_Error(6, errorFmt, weapDef->szInternalName, text);
        else
            Com_Printf(warningFmt, weapDef->szInternalName, text);
    }

    return text;
}

static snd_alias_list_t *CG_FindSoundAliasWithFallback(const char *name, const char *fallback)
{
    snd_alias_list_t *alias;

    alias = (snd_alias_list_t *)Com_FindSoundAlias(name);
    return alias ? alias : (snd_alias_list_t *)Com_FindSoundAlias(fallback);
}

static float CG_DvarValue(void *dvarImport)
{
    return (*(dvar_t **)dvarImport)->current.value;
}

static void CG_SetHoldBreathChannelVolumes(float frac)
{
    float volumes[11];
    int i;

    if (frac == 0.0f) {
        SND_DeactivateChannelVolumes(1, 0);
        return;
    }

    for (i = 0; i < 11; i++)
        volumes[i] = 1.0f + (cgs->holdBreathParams.sound.channelvolume[i] - 1.0f) * frac;

    SND_SetChannelVolumes(1, volumes, 0);
}

static void CG_UpdateHoldBreathSound(void)
{
    int delayMsec;

    if (cg->holdBreathDelay > 0)
        cg->holdBreathDelay -= cg->frametime;

    if (cg->predictedPlayerState.pm_flags & 0x4000) {
        cg->holdBreathFrac = DiffTrack(1.0f, cg->holdBreathFrac,
                                       CG_DvarValue(imp_player_breath_snd_lerp),
                                       (float)cg->frametime * 0.001f);

        if (cg->holdBreathTime < 0) {
            cg->holdBreathTime = 0;
            if (cg->holdBreathDelay > 0) {
                cg->holdBreathInTime = 0;
            } else {
                cg->holdBreathInTime = CG_PlayClientSoundAlias(cgs->media.playerBreathInSound);
                cg->holdBreathDelay = (int)(CG_DvarValue(imp_player_breath_snd_delay) * 1000.0f);
            }
        } else if (cg->holdBreathTime > cg->holdBreathInTime) {
            CG_PlayClientSoundAlias(cgs->media.playerHeartBeatSound);
        }

        cg->holdBreathTime += cg->frametime;
    } else {
        if (cg->holdBreathTime >= 0) {
            cg->holdBreathTime += cg->frametime;
            if ((int)(CG_DvarValue(imp_player_breath_hold_time) * 1000.0f) < cg->holdBreathTime) {
                CG_PlayClientSoundAlias(cgs->media.playerBreathGaspSound);
            } else if (cg->holdBreathDelay <= 0) {
                CG_PlayClientSoundAlias(cgs->media.playerBreathOutSound);
                cg->holdBreathDelay = (int)(CG_DvarValue(imp_player_breath_snd_delay) * 1000.0f);
            }
        }

        cg->holdBreathTime = -1;
        cg->holdBreathInTime = 0;
        cg->holdBreathFrac = 0.0f;
    }

    CG_SetHoldBreathChannelVolumes(cg->holdBreathFrac);
}

static void CG_Madd(vec3_t out, const vec3_t dir, float scale)
{
    out[0] += dir[0] * scale;
    out[1] += dir[1] * scale;
    out[2] += dir[2] * scale;
}

static void CG_ResetViewWeaponOffsets(cg_t *cg)
{
    cg->gunPitch = cg->refdefViewAngles[0];
    cg->gunYaw = cg->refdefViewAngles[1];
    cg->gunXOfs = 0.0f;
    cg->gunYOfs = 0.0f;
    cg->gunZOfs = 0.0f;
}

#ifdef __EMSCRIPTEN__
void CG_Weapons_SetToDefault(int weaponNum, weaponInfo_s (*dobjModels)[4])
{
    byte *weapDef;
    char modelFile[80];
    const char *handModel;
    const char *viewModel;

    CG_SetWeaponDefToDefaultWeapon(weaponNum);
    weapDef = (byte *)BG_GetWeaponDef(weaponNum);
    Com_Printf("WARNING: gun and/or hand model file for weapon [%s] could not be found\n",
               *(const char **)(weapDef + 4));

    handModel = ((WeaponDef *)weapDef)->szGunXModel;
    if (!handModel || handModel[0] == '\0') {
        Com_Error(1, "could not find default weapon model");
    } else {
        viewModel = ((WeaponDef *)weapDef)->szHandXModel;
        if (!viewModel || viewModel[0] == '\0') {
            Com_Error(1, "could not find default weapon model");
        }
    }

    viewModel = ((WeaponDef *)weapDef)->szHandXModel;
    sprintf(modelFile, "%s%s", "xmodel/", viewModel);
    *(void **)dobjModels = CL_RegisterModel(modelFile);

    handModel = ((WeaponDef *)weapDef)->szGunXModel;
    sprintf(modelFile, "%s%s", "xmodel/", handModel);
    *(void **)((byte *)dobjModels + 0xc) = CL_RegisterModel(modelFile);
}
#else
void CG_Weapons_SetToDefault(int weaponNum, DObjModel_s *dobjModels)
{
    WeaponDef *weapDef;
    char modelFile[64];

    CG_SetWeaponDefToDefaultWeapon(weaponNum);
    weapDef = (WeaponDef *)BG_GetWeaponDef(weaponNum);
    Com_Printf("WARNING: gun and/or hand model file for weapon [%s] could not be found\n",
               weapDef->szDisplayName);

    if (!weapDef->szGunXModel || !weapDef->szGunXModel[0] ||
        !weapDef->szHandXModel || !weapDef->szHandXModel[0])
        Com_Error(1, "\x15"
                     "CG_RegisterWeapon: No gun and/or hand model specified for default weapon!");

    sprintf(modelFile, "%s%s", "xmodel/", weapDef->szHandXModel);
    dobjModels[0].model = CL_RegisterModel(modelFile);

    sprintf(modelFile, "%s%s", "xmodel/", weapDef->szGunXModel);
    dobjModels[1].model = CL_RegisterModel(modelFile);
}
#endif

void CG_HoldBreathInit(void)
{
    cg->holdBreathTime = -1;
    cg->holdBreathInTime = 0;
    cg->holdBreathDelay = 0;
    cg->holdBreathFrac = 0.0f;
}

void CG_SetupWeaponDef(void)
{
    char szString[0x2000];
    char *pszFiles[0x7f];
    char *scan;
    int iNumFiles;
    int i;

    memset(pszFiles, 0, sizeof(pszFiles));
    strcpy(szString, CL_GetConfigString(7));

    pszFiles[0] = szString;
    iNumFiles = 1;
    scan = szString;

    while (*scan) {
        if (*scan != ' ') {
            scan++;
            continue;
        }

        *scan++ = '\0';
        if (!*scan)
            break;

        if (*scan != ' ')
            pszFiles[iNumFiles++] = scan;
    }

    for (i = 0; i < iNumFiles; i++) {
        const char *name = pszFiles[i];

        if (BG_GetWeaponIndexForName(name, 0) != i + 1)
            Com_Error(1, "Weapon index mismatch for '%s'", name);
    }
}

static inline __attribute__((always_inline)) void CG_SelectWeaponIndex_core(int weaponIndex)
{
    int oldWeaponIndex;
    qboolean keepAds;

    cg->weaponSelectTime = cg->time;

    oldWeaponIndex = cg->weaponSelect;
    if (oldWeaponIndex == weaponIndex)
        return;

    keepAds = weaponIndex && ((WeaponDef *)BG_GetWeaponDef(oldWeaponIndex))->iAltWeaponIndex == weaponIndex;
    cg->weaponSelect = weaponIndex;
    CG_MenuShowNotify(1);

    if (!keepAds)
        CL_SetADS(0);
}

void CG_SelectWeaponIndex(int weaponIndex)
{
    CG_SelectWeaponIndex_core(weaponIndex);
}

static qboolean __attribute_regparm__(3) CG_CalcMuzzlePoint(int entityNum, vec_t *muzzle, unsigned int flashTag)
{
    snapshot_t *snap;
    centity_t *cent;
    void *dobj;
    int entFlags;

    snap = cg->nextSnap;
    if (!snap || (unsigned int)entityNum >= 1024)
        return 0;

    if (!cg->renderingThirdPerson && (snap->ps.pm_flags & 0xc00000) && snap->ps.clientNum == entityNum) {
        int weapon = BG_GetViewmodelWeaponIndex(&cg->predictedPlayerState);
        struct DObj_s *viewModel = (*(weaponInfo_t **)imp_cg_weapons)[weapon].viewModelDObj;
        if (viewModel && CG_DObjGetViewModelTagPos(viewModel, flashTag, muzzle))
            return 1;
        // Keep the tracer in front of the camera if the tag is not ready yet.
        for (int i = 0; i < 3; ++i)
            muzzle[i] = cg->refdef.vieworg[i] + 16.0f * cg->refdef.viewaxis[0][i]
                       - 2.0f * cg->refdef.viewaxis[1][i] - 2.0f * cg->refdef.viewaxis[2][i];
        return 1;
    }

    cent = &cg_entities[entityNum];
    dobj = Com_GetClientDObj(cent->nextState.number, cent->localClientNum);
    if (!dobj) {
        return 0;
    }

    if (CG_DObjGetWorldTagPos(cent, (struct DObj_s *)dobj, flashTag, muzzle))
        return 1;

    muzzle[0] = cent->nextState.pos.trBase[0];
    muzzle[1] = cent->nextState.pos.trBase[1];
    muzzle[2] = cent->nextState.pos.trBase[2];

    if (entityNum > 63)
        return 1;

    Com_DPrintf("No %s in CG_CalcMuzzlePoint on entity %d.\n", SL_ConvertToString(flashTag), entityNum);

    entFlags = cent->nextState.eFlags;
    if (entFlags & 8)
        muzzle[2] += 11.0f;
    else if (entFlags & 4)
        muzzle[2] += 40.0f;
    else
        muzzle[2] += 60.0f;

    return 1;
}

void CG_EjectWeaponBrass(entityState_t *ent, int event)
{
    byte *entBytes;
    weaponInfo_t *weapInfo;
    snapshot_t *snap;
    int weaponNum;
    int bolt[2];
    void *effect;
    unsigned int tagBrass;

    if (!cg_brass->current.enabled)
        return;

    entBytes = (byte *)ent;
    if (*(int *)(entBytes + 4) > 9)
        return;

    weaponNum = *(int *)(entBytes + 0xc8);
    if (!weaponNum)
        return;

    if (weaponNum > BG_GetNumWeapons())
        Com_Error(1, "\x15"
                     "CG_EjectWeaponBrass: ent->weapon > BG_GetNumWeapons()");

    weapInfo = &(*(weaponInfo_t **)imp_cg_weapons)[weaponNum];
    effect = weapInfo->lastShotEjectEffect;
    if (event != 0xa0 || !effect) {
        effect = weapInfo->shellEjectEffect;
        if (!effect)
            return;
    }

    snap = cg->nextSnap;
    if ((snap->ps.pm_flags & 0xc00000) && ent->number == snap->ps.clientNum)
        bolt[0] = weaponNum + 0x400;
    else
        bolt[0] = ent->number;

    tagBrass = scr_const.tag_brass;
    bolt[1] = FX_GetBoneIndex(bolt[0], tagBrass);
    if (bolt[1] < 0)
        return;

    FX_PlayEntityEffect(effect, ejectBrassCasingOrigin, 0, bolt);
}

static qboolean CG_IsLocalClientEntity(cg_t *cg, entityState_t *ent)
{
    snapshot_t *snap = cg->nextSnap;

    return (snap->ps.pm_flags & 0xc00000) && ent->number == snap->ps.clientNum;
}

static inline __attribute__((always_inline)) qboolean CG_IsLocalClientEntity_inl(cg_t *cg, entityState_t *ent)
{
    snapshot_t *snap = cg->nextSnap;

    return (snap->ps.pm_flags & 0xc00000) && ent->number == snap->ps.clientNum;
}

static inline __attribute__((always_inline)) void CG_GetFireWeaponSoundOrigin(centity_t *cent, entityState_t *ent, weaponInfo_t *weapInfo, vec_t *origin)
{
    unsigned int tagFlash;

    tagFlash = scr_const.tag_flash;

    if (CG_IsLocalClientEntity_inl(cg, ent)) {
        struct DObj_s *viewModel = weapInfo->viewModelDObj;

        if (viewModel && CG_DObjGetViewModelTagPos(viewModel, tagFlash, origin))
            return;
    } else {
        struct DObj_s *obj = Com_GetClientDObj(ent->number, cent->localClientNum);

        if (obj && CG_DObjGetWorldTagPos(cent, obj, tagFlash, origin))
            return;
    }

    BG_EvaluateTrajectory(&ent->pos, cg->time, origin);
}

static inline __attribute__((always_inline)) void CG_PlayFireWeaponEffect(centity_t *cent, entityState_t *ent, int barrel)
{
    snapshot_t *snap;
    weaponInfo_t *weapInfo;
    void *fx;
    unsigned int flashTag;
    int bolt[2];
    qboolean localTurret;

    if (ent->eType != 9)
        return;

    snap = cg->nextSnap;
    localTurret = 0;
    if (!cg->renderingThirdPerson && (snap->ps.eFlags & 0x300) && snap->ps.viewlocked_entNum == ent->number)
        localTurret = 1;

    weapInfo = &(*(weaponInfo_t **)imp_cg_weapons)[ent->weapon];
    fx = (void *)(localTurret ? weapInfo->viewFlashEffect : weapInfo->worldFlashEffect);
    if (!fx)
        return;

    flashTag = *(unsigned short *)s_barrelTags[barrel];
    bolt[0] = ent->number;
    bolt[1] = FX_GetBoneIndex(ent->number, flashTag);
    if (bolt[1] < 0)
        return;

    FX_PlayEntityEffect(fx, (vec_t *)&((centity_t *)cent)->lerpOrigin[0], 0, bolt);
}

void CG_FireWeapon(centity_t *cent, int event, int barrel)
{
    entityState_t *ent;
    weaponInfo_t *weapInfo;
    WeaponDef *weapDef;
    snd_alias_list_t *fireSound;
    vec3_t origin;
    int duration;

    ent = &cent->nextState;
    if (!ent->weapon)
        return;

    if (ent->weapon > BG_GetNumWeapons())
        Com_Error(1, "\x15"
                     "CG_FireWeapon: ent->weapon > BG_GetNumWeapons()");

    weapInfo = &(*(weaponInfo_t **)imp_cg_weapons)[ent->weapon];
    BG_GetWeaponDef(ent->weapon);
    cent->bMuzzleFlash = 1;

    if (CG_IsLocalClientEntity_inl(cg, ent))
        BG_WeaponFireRecoil(&cg->predictedPlayerState, cg->vGunSpeed, cg->kickAVel);

    CG_PlayFireWeaponEffect(cent, ent, barrel);

    fireSound = weapInfo->flashSound;
    if (weapInfo->flashSoundPlayer && CG_IsLocalClientEntity_inl(cg, ent))
        fireSound = weapInfo->flashSoundPlayer;

    if (event == 0xa0) {
        if (weapInfo->lastShotSoundPlayer && CG_IsLocalClientEntity_inl(cg, ent))
            fireSound = weapInfo->lastShotSoundPlayer;
        else if (weapInfo->lastShotSound)
            fireSound = weapInfo->lastShotSound;
    }

    if (fireSound) {
        CG_GetFireWeaponSoundOrigin(cent, ent, weapInfo, origin);
        duration = CG_PlaySoundAlias(ent->number, origin, fireSound);
        if (ent->eType == 1)
            CG_CompassAddWeaponPingInfo(cent, origin, duration);
    }

    weapDef = (WeaponDef *)BG_GetWeaponDef(ent->weapon);
    if (!weapDef->bBoltAction)
        CG_EjectWeaponBrass(ent, event);
}

void CG_RegisterWeapon(int weaponNum)
{
    WeaponDef *weapDef;
    weaponInfo_t *weapInfo;
    itemInfo_t *itemInfo;
    struct XAnim_s *pAnims;
    struct XAnimTree_s *pAnimTree;
    DObjModel_s dobjModels[2];
    char modelFile[64];
    int i;

    removeMeWhenMPStopsCrashingInHere = weaponNum;

    if (!weaponNum)
        return;

    weapInfo = &(*(weaponInfo_t **)imp_cg_weapons)[weaponNum];
    weapDef = (WeaponDef *)BG_GetWeaponDef(weaponNum);
    if (weapInfo->registered)
        return;

    SCR_UpdateScreen();
    memset(weapInfo, 0, sizeof(*weapInfo));
    weapInfo->registered = 1;
    weapInfo->item = &((gitem_t *)imp_bg_itemlist)[weaponNum];
    itemInfo = &(*(itemInfo_t **)imp_cg_items)[weaponNum];

    CG_RegisterItemVisuals(weaponNum);
    weapInfo->iPrevAnim = -1;

    if (weapDef->szGunXModel[0]) {
        if (!CG_HasAssetName(weapDef->szHandXModel))
            Com_Error(1, "\x15"
                         "CG_RegisterWeapon: No hand model specified for [%s]",
                      weapDef->szDisplayName);

        dobjModels[0].boneName = 0;
        dobjModels[0].ignoreCollision = 0;
        dobjModels[1].boneName = "tag_weapon";
        dobjModels[1].ignoreCollision = 0;

        sprintf(modelFile, "%s%s", "xmodel/", weapDef->szHandXModel);
        dobjModels[0].model = CL_RegisterModel(modelFile);

        sprintf(modelFile, "%s%s", "xmodel/", weapDef->szGunXModel);
        dobjModels[1].model = CL_RegisterModel(modelFile);

        if (XModelBad(dobjModels[0].model) || XModelBad(dobjModels[1].model))
            CG_Weapons_SetToDefault(weaponNum, dobjModels);

        if (!CG_HasAssetName(weapDef->szXAnims[1]))
            Com_Error(1, "\x15"
                         "CG_RegisterWeapon: No idle anim specified for [%s]",
                      weapDef->szDisplayName);

        pAnims = XAnimCreateAnims("VIEWMODEL", 0x17, imp_Hunk_AllocXAnimClient);
        XAnimBlend(pAnims, 0, "root", 1, 0x16, 0);

        for (i = 1; i < 0x17; i++) {
            const char *animName = weapDef->szXAnims[i][0] ? weapDef->szXAnims[i] : weapDef->szXAnims[1];
            XAnimPrecache(animName, imp_Hunk_AllocXAnimPrecache);
            XAnimCreate(pAnims, i, animName);
        }

        pAnimTree = (struct XAnimTree_s *)XAnimCreateTree(pAnims, imp_Hunk_AllocXAnimClient);
        weapInfo->tree = pAnimTree;

        for (i = 0; i < 0x17; i++)
            weapInfo->viewModelAnimRates[i] = 1.0f;

        weapInfo->viewModelAnimRates[4] = CG_WeaponAnimRate(pAnims, 4, weapDef->iHoldFireTime);
        weapInfo->viewModelAnimRates[7] = CG_WeaponAnimRate(pAnims, 7, weapDef->iMeleeTime);
        weapInfo->viewModelAnimRates[8] = CG_WeaponAnimRate(pAnims, 8, CG_WeaponDefIntAt(weapDef, 0x218));
        weapInfo->viewModelAnimRates[9] = CG_WeaponAnimRate(pAnims, 9, weapDef->iReloadEmptyTime);
        weapInfo->viewModelAnimRates[10] = CG_WeaponAnimRate(pAnims, 10, weapDef->iReloadStartTime);
        weapInfo->viewModelAnimRates[11] = CG_WeaponAnimRate(pAnims, 11, CG_WeaponDefIntAt(weapDef, 0x218));
        weapInfo->viewModelAnimRates[12] = CG_WeaponAnimRate(pAnims, 12, weapDef->iRaiseTime);
        weapInfo->viewModelAnimRates[13] = CG_WeaponAnimRate(pAnims, 13, weapDef->iDropTime);
        weapInfo->viewModelAnimRates[14] = CG_WeaponAnimRate(pAnims, 14, weapDef->iAltRaiseTime);
        weapInfo->viewModelAnimRates[15] = CG_WeaponAnimRate(pAnims, 15, weapDef->iAltDropTime);
        weapInfo->viewModelAnimRates[16] = CG_WeaponAnimRate(pAnims, 16, weapDef->quickRaiseTime);
        weapInfo->viewModelAnimRates[17] = CG_WeaponAnimRate(pAnims, 17, weapDef->quickDropTime);

        if (weapDef->szXAnims[21][0] && XAnimIsLooped(pAnims, 0x15))
            Com_Error(1, "\x15"
                         "CG_RegisterWeapon: ADS anim [%s] cannot be looping",
                      weapDef->szXAnims[21]);

        if (weapDef->szXAnims[22][0] && XAnimIsLooped(pAnims, 0x16))
            Com_Error(1, "\x15"
                         "CG_RegisterWeapon: ADS anim [%s] cannot be looping",
                      weapDef->szXAnims[22]);

        Com_ClientDObjCreate(dobjModels, 2, pAnimTree, CG_WeaponDObjHandle(weaponNum));
        weapInfo->viewModelDObj = Com_GetClientDObj(CG_WeaponDObjHandle(weaponNum), 0);
        I_strncpyz(weapInfo->handModel, weapDef->szHandXModel, sizeof(weapInfo->handModel));

        XAnimClearTreeGoalWeights(pAnimTree, 0, 0);
        XAnimSetGoalWeight(pAnimTree, 0, 1.0f, 0.0f, weapInfo->viewModelAnimRates[0], 0, 0, 1);
        XAnimSetGoalWeight(pAnimTree, 1, 1.0f, 0.0f, weapInfo->viewModelAnimRates[1], 0, 0, 1);

        if (weapDef->szXAnims[22][0]) {
            XAnimSetGoalWeight(pAnimTree, 0x16, 1.0f, 0.0f, 0.0f, 0, 0, 1);
            XAnimSetTime(pAnimTree, 0x16, 1.0f);
        }

        DObjUpdateClientInfo(weapInfo->viewModelDObj, 0.05f);
    }

    if (weapDef->szWorldModel[0]) {
        weapInfo->worldSurfModel = CL_RegisterModel(weapDef->szWorldModel);
        if (!weapInfo->worldSurfModel)
            Com_Printf("WARNING: Weapon %s could not load world model\n", weapDef->szWorldModel);
    }

    if (weapDef->szReticleCenter[0])
        weapInfo->hReticleCenter = (MaterialHandle)CL_RegisterMaterialNoMip(weapDef->szReticleCenter, 7);

    if (weapDef->szReticleSide[0])
        weapInfo->hReticleSide = (MaterialHandle)CL_RegisterMaterialNoMip(weapDef->szReticleSide, 7);

    if (weapDef->szOverlayMaterial[0])
        weapInfo->hADSOverlay = (MaterialHandle)CL_RegisterMaterialNoMip(weapDef->szOverlayMaterial, 7);

    if (weapDef->szViewFlashEffect[0])
        weapInfo->viewFlashEffect = (struct EffectTemplate *)FX_RegisterEffect(weapDef->szViewFlashEffect);

    if (weapDef->szWorldFlashEffect[0])
        weapInfo->worldFlashEffect = (struct EffectTemplate *)FX_RegisterEffect(weapDef->szWorldFlashEffect);

    weapInfo->missileSound = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szProjectileSound);
    weapInfo->pullbackSound = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szPullbackSound);
    weapInfo->flashSound = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szFireSound);
    weapInfo->flashSoundPlayer = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szFireSoundPlayer);
    weapInfo->lastShotSound = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szFireLastSound);
    weapInfo->lastShotSoundPlayer = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szFireLastSoundPlayer);
    weapInfo->meleeSwipeSound = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->meleeSwipeSound);
    weapInfo->rechamberSound = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szRechamberSound);
    weapInfo->rechamberSoundPlayer = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szRechamberSoundPlayer);
    weapInfo->reloadSound = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szReloadSound);
    weapInfo->reloadSoundPlayer = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szReloadSoundPlayer);
    weapInfo->reloadEmptySound = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szReloadEmptySound);
    weapInfo->reloadEmptySoundPlayer = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szReloadEmptySoundPlayer);
    weapInfo->reloadStartSound = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szReloadStartSound);
    weapInfo->reloadStartSoundPlayer = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szReloadStartSoundPlayer);
    weapInfo->reloadEndSound = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szReloadEndSound);
    weapInfo->reloadEndSoundPlayer = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szReloadEndSoundPlayer);
    weapInfo->raiseSound = CG_FindSoundAliasWithFallback(weapDef->szRaiseSound, "weap_raise");
    weapInfo->altSwitchSound = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szAltSwitchSound);
    weapInfo->putawaySound = CG_FindSoundAliasWithFallback(weapDef->szPutawaySound, "weap_putaway");
    weapInfo->noteTrackSound[0] = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szNoteTrackSoundA);
    weapInfo->noteTrackSound[1] = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szNoteTrackSoundB);
    weapInfo->noteTrackSound[2] = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szNoteTrackSoundC);
    weapInfo->noteTrackSound[3] = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szNoteTrackSoundD);

    if (!itemInfo->pickupSound)
        itemInfo->pickupSound = (snd_alias_list_t *)Com_FindSoundAlias("weap_pickup");

    itemInfo->ammoPickupSound = CG_FindSoundAliasWithFallback(weapDef->szAmmoPickupSound, "weap_ammo_pickup");

    if (weapDef->szShellEjectEffect[0])
        weapInfo->shellEjectEffect = (struct EffectTemplate *)FX_RegisterEffect(weapDef->szShellEjectEffect);

    if (weapDef->szLastShotEjectEffect[0])
        weapInfo->lastShotEjectEffect = (struct EffectTemplate *)FX_RegisterEffect(weapDef->szLastShotEjectEffect);
    else
        weapInfo->lastShotEjectEffect = weapInfo->shellEjectEffect;

    if (weapDef->szProjectileModel[0]) {
        weapInfo->missileSurfModel = CL_RegisterModel(weapDef->szProjectileModel);
        if (!weapInfo->missileSurfModel)
            Com_Error(1, "\x15Weapon %s does not specify a valid projectile model (%s)\n", weapDef->szInternalName, weapDef->szWorldModel);
    }

    if (weapDef->szProjExplosionEffect[0])
        weapInfo->projExplosionEffect = (struct EffectTemplate *)FX_RegisterEffect(weapDef->szProjExplosionEffect);

    weapInfo->projExplosionSound = (snd_alias_list_t *)Com_FindSoundAlias(weapDef->szProjExplosionSound);

    if (weapDef->szProjTrailEffect[0])
        weapInfo->projTrailEffect = (struct EffectTemplate *)FX_RegisterEffect(weapDef->szProjTrailEffect);

    weapInfo->missileDlight = (float)weapDef->iProjectileDLight;

    if (weapDef->szHudIcon[0]) {
        weapInfo->hHudIcon = (MaterialHandle)CL_RegisterMaterial(weapDef->szHudIcon, 7);
        cgs->media.hintMaterials[weaponNum + 4] = weapInfo->hHudIcon;
    } else {
        cgs->media.hintMaterials[weaponNum + 4] = cgs->media.hintMaterials[0];
    }

    if (weapDef->killIcon[0]) {
        CL_RegisterMaterial(weapDef->killIcon, 7);
        CL_RegisterHudMsgIconMaterial(weapDef->killIcon);
    }

    if (weapDef->szModeIcon[0])
        weapInfo->hModeIcon = (MaterialHandle)CL_RegisterMaterial(weapDef->szModeIcon, 7);

    weapInfo->pszTranslatedDisplayName = CG_TranslateWeaponString(weapDef, weapDef->szDisplayName,
                                                                  "Weapon %s: Could not translate display name \"%s\"", "^3WARNING: Weapon %s: Could not translate display name \"%s\"\n");
    weapInfo->pszTranslatedModename = CG_TranslateWeaponString(weapDef, weapDef->szModeName,
                                                               "Weapon %s: Could not translate mode name \"%s\"", "^3WARNING: Weapon %s: Could not translate mode name \"%s\"\n");
    weapInfo->pszTranslatedAIOverlayDescription = CG_TranslateWeaponString(weapDef, weapDef->szOverlayName,
                                                                           "Weapon %s: Could not translate AI overlay description \"%s\"", "^3WARNING: Weapon %s: Could not translate AI overlay description \"%s\"\n");
}

void CG_RegisterItemVisuals(int itemNum)
{
    itemInfo_t *itemInfo;
    gitem_t *item;
    int i;
    void *pickupSound;

    itemInfo = &(*(itemInfo_t **)imp_cg_items)[itemNum];
    if (itemInfo->registered)
        return;

    item = &((gitem_t *)imp_bg_itemlist)[itemNum];
    itemInfo->registered = 0;

    for (i = 0; i < 2; i++) {
        const char *modelName = *(const char **)((byte *)item + 8 + i * 4);
        if (modelName && modelName[0]) {
            SCR_UpdateScreen();
            *(struct XModel **)((byte *)itemInfo + 4 + i * 4) = CL_RegisterModel(modelName);
        }
    }

    itemInfo->icons[0] = CL_RegisterMaterial(item->icon, 7);

    if (item->pickup_sound) {
        pickupSound = Com_FindSoundAlias(item->pickup_sound);
        itemInfo->pickupSound = pickupSound;
    } else {
        pickupSound = itemInfo->pickupSound;
    }
    itemInfo->ammoPickupSound = pickupSound;

    if (item->giType == 1)
        CG_RegisterWeapon(item->giTag);

    itemInfo->registered = 1;
}

void CG_RegisterItems(void)
{
    char items[0x109];
    int itemCount;
    int i;

    strcpy(items, CL_GetConfigString(8));
    itemCount = *(int *)imp_bg_numItems;

    for (i = 1; i < itemCount; i++) {
        int c = (signed char)items[i >> 2];

        if (c <= '9') {
            if (((c - '0') >> (i & 3)) & 1)
                CG_RegisterItemVisuals(i);
        } else {
            if (((c - 'W') >> (i & 3)) & 1)
                CG_RegisterItemVisuals(i);
        }
    }
}

void CG_UpdateHandViewmodels(const char *handModel)
{
    int weaponIdx;

    for (weaponIdx = 1; weaponIdx <= BG_GetNumWeapons(); weaponIdx++) {
        weaponInfo_t *weapInfo;
        WeaponDef *weapDef;
        struct XAnimTree_s *pAnimTree;
        DObjModel_s dobjModels[2];
        int dobjHandle;
        char modelFile[64];

        weapInfo = &(*(weaponInfo_t **)imp_cg_weapons)[weaponIdx];
        if (!strcmp(weapInfo->handModel, handModel))
            continue;

        if (!weaponIdx)
            continue;

        weapDef = (WeaponDef *)BG_GetWeaponDef(weaponIdx);
        if (!weapInfo->viewModelDObj)
            continue;

        if (!weapDef->szGunXModel[0])
            continue;

        pAnimTree = DObjGetTree(weapInfo->viewModelDObj);
        dobjHandle = weaponIdx + 0x400;
        Com_SafeClientDObjFree(dobjHandle);

        dobjModels[0].boneName = 0;
        dobjModels[0].ignoreCollision = 0;
        dobjModels[1].boneName = "tag_weapon";
        dobjModels[1].ignoreCollision = 0;
        dobjModels[0].model = CL_RegisterModel(handModel);

        sprintf(modelFile, "%s%s", "xmodel/", weapDef->szGunXModel);
        dobjModels[1].model = CL_RegisterModel(modelFile);

        if (XModelBad(dobjModels[0].model) || XModelBad(dobjModels[1].model))
            CG_Weapons_SetToDefault(weaponIdx, dobjModels);

        Com_ClientDObjCreate(dobjModels, 2, pAnimTree, dobjHandle);
        weapInfo->viewModelDObj = Com_GetClientDObj(dobjHandle, 0);
        I_strncpyz(weapInfo->handModel, handModel, 0x40);
        DObjUpdateClientInfo(weapInfo->viewModelDObj, 0.05f);
    }
}

void CG_SpawnTracer(vec_t *pstart, vec_t *pend)
{
    vec3_t start;
    vec3_t dir;
    float dist;
    float speed;
    int time;
    int tracerDelay;
    localEntity_t *le;

    start[0] = pstart[0];
    start[1] = pstart[1];
    start[2] = pstart[2];

    dir[0] = pend[0] - start[0];
    dir[1] = pend[1] - start[1];
    dir[2] = pend[2] - start[2];

    dist = Vec3Normalize(dir);
    speed = cg_tracerSpeed->current.value;
    if (dist <= 0.0f || speed <= 0.0f)
        return;
    le = CG_AllocLocalEntity();
    le->leType = LE_MOVING_TRACER;
    le->tracerClipDist = dist;

    time = cg->time;
    tracerDelay = cg->frametime;
    if (tracerDelay)
        time -= (rand() % tracerDelay) / 2;

    speed = cg_tracerSpeed->current.value;
    le->endTime = time + (int)((dist * 1000.0f) / speed);
    le->pos.trType = TR_LINEAR;
    le->pos.trTime = time;

    le->pos.trBase[0] = start[0];
    le->pos.trBase[1] = start[1];
    le->pos.trBase[2] = start[2];

    le->pos.trDelta[0] = speed * dir[0];
    le->pos.trDelta[1] = speed * dir[1];
    le->pos.trDelta[2] = speed * dir[2];
}

void CG_WhizbySound(vec_t *vStart, vec_t *vEnd)
{
    vec3_t vDelta;
    vec3_t vDir;
    vec3_t vProjPos;
    vec3_t vieworg;
    float segmentLen;
    float projLen;
    float distanceToView;

    vDelta[0] = vEnd[0] - vStart[0];
    vDelta[1] = vEnd[1] - vStart[1];
    vDelta[2] = vEnd[2] - vStart[2];
    Vec3NormalizeTo(vDelta, vDir);

    segmentLen = vDir[0] * vDelta[0] + vDir[1] * vDelta[1] + vDir[2] * vDelta[2];

    vieworg[0] = cg->refdef.vieworg[0];
    vieworg[1] = cg->refdef.vieworg[1];
    vieworg[2] = cg->refdef.vieworg[2];

    projLen = (vieworg[0] - vStart[0]) * vDir[0] + (vieworg[1] - vStart[1]) * vDir[1] + (vieworg[2] - vStart[2]) * vDir[2];

    if (projLen < 64.0f || projLen + 64.0f > segmentLen)
        return;

    vProjPos[0] = vStart[0] + projLen * vDir[0];
    vProjPos[1] = vStart[1] + projLen * vDir[1];
    vProjPos[2] = vStart[2] + projLen * vDir[2];

    vDelta[0] = vProjPos[0] - vieworg[0];
    vDelta[1] = vProjPos[1] - vieworg[1];
    vDelta[2] = vProjPos[2] - vieworg[2];
    distanceToView = sqrtf(vDelta[0] * vDelta[0] + vDelta[1] * vDelta[1] + vDelta[2] * vDelta[2]);
    if (distanceToView > 140.0f)
        return;

    vProjPos[0] += -16.0f * vDir[0];
    vProjPos[1] += -16.0f * vDir[1];
    vProjPos[2] += -16.0f * vDir[2];

    CG_PlaySoundAlias(0x3fe, vProjPos, cgs->media.bulletWhizby);
}

void CG_BulletHitClientEvent(int sourceEntityNum, vec_t *position, int surfType, int event)
{
    snapshot_t *snap;
    snd_alias_list_t *alias;
    unsigned int flashTag;
    vec3_t muzzle;
    float tracerChance;

    if (event == 0xb9)
        alias = cgs->media.bulletHitSmallSound[surfType];
    else if (event == 0xba)
        alias = cgs->media.bulletHitLargeSound[surfType];
    else
        alias = cgs->media.shotgunHitSound[surfType];

    CG_PlaySoundAlias(0x3fe, position, alias);

    flashTag = *(unsigned short *)s_barrelTags[0];
    tracerChance = cg_tracerChance->current.value;
    if (tracerChance <= 0.0f)
        return;

    if (!CG_CalcMuzzlePoint(sourceEntityNum, muzzle, flashTag))
        return;

    {
        int randInt = rand();
        float randValue;

        if (randInt < 0) {
            unsigned int adjusted = ((unsigned int)randInt >> 1) | (randInt & 1);
            randValue = (float)adjusted * 2.0f;
        } else {
            randValue = (float)randInt;
        }

        if (2147483648.0f * tracerChance > randValue)
            CG_SpawnTracer(muzzle, position);
    }

    CG_WhizbySound(muzzle, position);
}

void CG_AddPlayerWeapon(GfxEntity *parent, playerState_t *ps, centity_t *cent, qboolean bDrawGun)
{
    entityState_t *ent;
    playerState_t *playerState;
    weaponInfo_t *weapInfo;
    int weaponNum;
    qboolean bViewModel;
    qboolean isLocalClientEntity;

    ent = &cent->nextState;
    isLocalClientEntity = CG_IsLocalClientEntity(cg, ent);

    if (ps) {
        playerState = ps;
        weaponNum = BG_GetViewmodelWeaponIndex(playerState);
        bViewModel = 1;
    } else {
        playerState = 0;
        weaponNum = ent->weapon;
        bViewModel = 0;
    }

    if (weaponNum <= 0)
        return;

    if (ent->eFlags & 0x300)
        return;

    weapInfo = &(*(weaponInfo_t **)imp_cg_weapons)[weaponNum];

    if (bViewModel) {
        if (bDrawGun) {
            GfxEntity gun;
            GfxModel model;
            GfxEntity *skinEnt;

            memset(&gun, 0, sizeof(gun));
            gun.renderFxFlags = 0x8c;
            gun.materialRGBA[0] = 0xff;
            gun.materialRGBA[1] = 0xff;
            gun.materialRGBA[2] = 0xff;
            gun.materialRGBA[3] = 0xff;

            gun.lighting.origin[0] = playerState->origin[0];
            gun.lighting.origin[1] = playerState->origin[1];
            gun.lighting.origin[2] = playerState->origin[2] + playerState->viewHeightCurrent;
            AddLeanToPosition(gun.lighting.origin, playerState->viewangles[1],
                              playerState->leanf, 16.0f, 20.0f);

            gun.origin[0] = parent->origin[0];
            gun.origin[1] = parent->origin[1];
            gun.origin[2] = parent->origin[2];
            AxisCopy(parent->axis, gun.axis);

            model.obj = weapInfo->viewModelDObj;
            skinEnt = R_AddRefEntityToScene(&gun, model, 0);
            {
                static int apwTrace;
                if (apwTrace < 8) {
                    if (getenv("DBGSPAM"))
                        fprintf(stderr, "[viewmodel] AddPlayerWeapon: dObj=%p skinEnt=%p gunOrg=(%.0f,%.0f,%.0f) lightOrg=(%.0f,%.0f,%.0f) registered=%d\n",
                                (void *)weapInfo->viewModelDObj, (void *)skinEnt,
                                gun.origin[0], gun.origin[1], gun.origin[2],
                                gun.lighting.origin[0], gun.lighting.origin[1], gun.lighting.origin[2],
                                weapInfo->registered);
                    ++apwTrace;
                }
            }
            if (skinEnt)
                R_SkinGfxEntityDelayed(skinEnt);
        }

        cg->viewModelOrigin[0] = parent->origin[0];
        cg->viewModelOrigin[1] = parent->origin[1];
        cg->viewModelOrigin[2] = parent->origin[2];
        AxisCopy(parent->axis, cg->viewModelAxis);
        AxisToAngles(cg->viewModelAxis, cg->viewModelAngles);

        if (weapInfo->viewModelDObj) {
            int boneIndex;

            boneIndex = DObjGetBoneIndex(weapInfo->viewModelDObj, scr_const.tag_brass);
            if (boneIndex > -1) {
                DObjAnimMat *rotTransArray = DObjGetRotTransArray(weapInfo->viewModelDObj);
                if (rotTransArray) {
                    vec3_t entAxis[4];

                    AxisCopy(parent->axis, entAxis);
                    entAxis[3][0] = parent->origin[0];
                    entAxis[3][1] = parent->origin[1];
                    entAxis[3][2] = parent->origin[2];
                    MatrixTransformVector43(rotTransArray[boneIndex].trans, entAxis, ejectBrassCasingOrigin);
                }
            }
        }

        CG_UpdateHoldBreathSound();
    }

    if (!cent->bMuzzleFlash)
        return;

    if (isLocalClientEntity && !bViewModel)
        return;

    cent->bMuzzleFlash = 0;

    if (!bDrawGun)
        return;

    {
        int bolt[2];
        void *fx;
        const vec_t *origin;

        if (bViewModel) {
            fx = weapInfo->viewFlashEffect;
            origin = cg->viewModelOrigin;
            bolt[0] = CG_WeaponDObjHandle(weaponNum);
        } else {
            fx = weapInfo->worldFlashEffect;
            origin = cent->lerpOrigin;
            bolt[0] = ent->number;
        }

        if (!fx)
            return;

        bolt[1] = FX_GetBoneIndex(bolt[0], *(unsigned short *)s_barrelTags[0]);
        if (bolt[1] < 0)
            return;

        FX_PlayEntityEffect(fx, origin, 0, bolt);
    }
}

void CG_AddViewWeapon(playerState_t *ps)
{
    playerState_t *playerState;
    GfxEntity hand;
    vec3_t weaponAngles;
    vec3_t weaponAxis[3];
    vec3_t finalAxis[3];
    weaponState_t ws;
    int weaponIndex;
    qboolean drawGun;
    float zoom;

    playerState = ps;

    if (playerState->pm_type == 4 || playerState->pm_type == 5)
        return;

    if (cg->renderingThirdPerson)
        return;

    drawGun = 0;
    if (!cg->cubemapShot && cg_drawGun->current.enabled && !CG_GetWeapReticleZoom(&zoom))
        drawGun = 1;

    if (cg->shellshock.hasSavedScreen)
        return;

    if (playerState->eFlags & 0x300)
        return;

    weaponIndex = BG_GetViewmodelWeaponIndex(playerState);
#    ifdef VM_TRACE
    {
        static int n;
        if (n++ < 40 || (n % 120) == 0)
            fprintf(stderr, "[vm] AddViewWeapon: pm_type=%d 3rd=%d eFlags=0x%x drawGun=%d weaponIdx=%d cubemap=%d\n",
                    playerState->pm_type, cg->renderingThirdPerson, playerState->eFlags, drawGun, weaponIndex, cg->cubemapShot);
        fflush(stderr);
    }
#    endif
    if (weaponIndex <= 0) {
        CG_ResetViewWeaponOffsets(cg);
        return;
    }

    BG_CalculateWeaponPosition_Sway(playerState, cg->swayViewAngles, cg->swayOffset, cg->swayAngles,
                                    1.0f, cg->frametime);

    memset(&ws, 0, sizeof(ws));
    ws.ps = playerState;
    ws.xyspeed = cg->xyspeed;
    ws.frametime = (float)cg->frametime * 0.001f;
    ws.vLastMoveAng[0] = cg->playerEntity.vLastMoveAng[0];
    ws.vLastMoveAng[1] = cg->playerEntity.vLastMoveAng[1];
    ws.vLastMoveAng[2] = cg->playerEntity.vLastMoveAng[2];
    ws.fLastIdleFactor = cg->playerEntity.fLastIdleFactor;
    ws.time = cg->time;
    ws.damageTime = cg->damageTime;
    ws.v_dmg_pitch = cg->v_dmg_pitch;
    ws.v_dmg_roll = cg->v_dmg_roll;
    ws.vGunOffset[0] = cg->vGunOffset[0];
    ws.vGunOffset[1] = cg->vGunOffset[1];
    ws.vGunOffset[2] = cg->vGunOffset[2];
    ws.vGunSpeed[0] = cg->vGunSpeed[0];
    ws.vGunSpeed[1] = cg->vGunSpeed[1];
    ws.vGunSpeed[2] = cg->vGunSpeed[2];
    ws.swayAngles[0] = cg->swayAngles[0];
    ws.swayAngles[1] = cg->swayAngles[1];
    ws.swayAngles[2] = cg->swayAngles[2];
    ws.weapIdleTime = &cg->weapIdleTime;
    BG_CalculateWeaponAngles(&ws, weaponAngles);

    /* The spring and movement filters advance in ws. Keep their result for
     * the next frame instead of accumulating undamped shot impulses in cg. */
    for (int i = 0; i < 3; ++i) {
        cg->vGunOffset[i] = ws.vGunOffset[i];
        cg->vGunSpeed[i] = ws.vGunSpeed[i];
        cg->playerEntity.vLastMoveAng[i] = ws.vLastMoveAng[i];
    }
    cg->playerEntity.fLastIdleFactor = ws.fLastIdleFactor;

    AnglesToAxis(weaponAngles, weaponAxis);
    MatrixMultiply(weaponAxis, cg->refdef.viewaxis, finalAxis);

    memset(&hand, 0, sizeof(hand));
    hand.renderFxFlags = 0xc;
    AxisCopy(finalAxis, hand.axis);

    hand.origin[0] = cg->refdef.vieworg[0];
    hand.origin[1] = cg->refdef.vieworg[1];
    hand.origin[2] = cg->refdef.vieworg[2];

    /* vGunOffset is angular recoil, already applied to weaponAngles. */
    CG_Madd(hand.origin, finalAxis[0], cg->swayOffset[0] + CG_DvarValue(imp_cg_gun_x));
    CG_Madd(hand.origin, finalAxis[1], cg->swayOffset[1] + CG_DvarValue(imp_cg_gun_y));
    CG_Madd(hand.origin, finalAxis[2], cg->swayOffset[2] + CG_DvarValue(imp_cg_gun_z));

    if (BG_IsAimDownSightWeapon(weaponIndex) && playerState->fWeaponPosFrac > 0.0f) {
        vec3_t worldAngles;
        /* Crosshair projection compares against the world-space camera axis. */
        AxisToAngles(finalAxis, worldAngles);
        cg->gunPitch = worldAngles[0];
        cg->gunYaw = worldAngles[1];
        cg->gunXOfs = (hand.origin[0] - cg->refdef.vieworg[0]) * playerState->fWeaponPosFrac;
        cg->gunYOfs = (hand.origin[1] - cg->refdef.vieworg[1]) * playerState->fWeaponPosFrac;
        cg->gunZOfs = (hand.origin[2] - cg->refdef.vieworg[2]) * playerState->fWeaponPosFrac;
    } else {
        CG_ResetViewWeaponOffsets(cg);
    }

    {
        static int vwTrace;
        if (vwTrace < 8) {
            if (getenv("DBGSPAM"))
                fprintf(stderr, "[viewmodel] AddViewWeapon: drawGun=%d weaponIdx=%d pm_type=%d eFlags=0x%x thirdPerson=%d org=(%.0f,%.0f,%.0f)\n",
                        drawGun, weaponIndex, playerState->pm_type, playerState->eFlags,
                        cg->renderingThirdPerson, cg->refdef.vieworg[0], cg->refdef.vieworg[1], cg->refdef.vieworg[2]);
            ++vwTrace;
        }
    }
    CG_AddPlayerWeapon(&hand, playerState, &cg->predictedPlayerEntity, drawGun);
}

void CG_BulletHitEvent(int sourceEntityNum, vec_t *position, vec_t *normal, vec_t *reflected, int surfType, int event)
{
    byte *fxTable;
    snd_alias_list_t *alias;
    void *fxNormal;
    void *fxReflect;
    unsigned int flashTag;
    vec3_t muzzle;
    float tracerChance;

    if (event == 0xb5) {
        alias = cgs->media.bulletHitSmallSound[surfType];
        fxTable = cgs->media.fx ? (byte *)cgs->media.fx->table : NULL;
        fxNormal = fxTable ? *(void **)(fxTable + surfType * 4) : NULL;
        fxReflect = fxTable ? *(void **)(fxTable + 0x5c + surfType * 4) : NULL;
    } else if (event == 0xb6) {
        alias = cgs->media.bulletHitLargeSound[surfType];
        fxTable = cgs->media.fx ? (byte *)cgs->media.fx->table : NULL;
        fxNormal = fxTable ? *(void **)(fxTable + 0xb8 + surfType * 4) : NULL;
        fxReflect = fxTable ? *(void **)(fxTable + 0x114 + surfType * 4) : NULL;
    } else {
        alias = cgs->media.shotgunHitSound[surfType];
        fxTable = cgs->media.fx ? (byte *)cgs->media.fx->table : NULL;
        fxNormal = fxTable ? *(void **)(fxTable + 0x170 + surfType * 4) : NULL;
        fxReflect = fxTable ? *(void **)(fxTable + 0x1cc + surfType * 4) : NULL;
    }

    if (!cg_blood->current.enabled && surfType == 7) {
        fxNormal = cgs->media.fxNoBloodFleshHit;
        fxReflect = 0;
    }

    if (getenv("PTRACE"))
        Com_Printf("[impact] source=%d event=%x surface=%d pos=(%.1f %.1f %.1f) effect=%s primitives=%d\n",
                   sourceEntityNum, event, surfType, position[0], position[1], position[2],
                   fxNormal ? ((EffectTemplate *)fxNormal)->mEffectName : "none",
                   fxNormal ? ((EffectTemplate *)fxNormal)->mPrimitiveCount : 0);
    CG_PlaySoundAlias(0x3fe, position, alias);

    if (fxNormal)
        FX_PlayEffect(fxNormal, position, normal);

    if (fxReflect)
        FX_PlayEffect(fxReflect, position, reflected);

    flashTag = *(unsigned short *)s_barrelTags[0];
    tracerChance = cg_tracerChance->current.value;
    if (tracerChance <= 0.0f)
        return;

    if (!CG_CalcMuzzlePoint(sourceEntityNum, muzzle, flashTag))
        return;

    if (getenv("PTRACE"))
        Com_Printf("[tracer] source=%d muzzle=(%.1f %.1f %.1f) hit=(%.1f %.1f %.1f) chance=%.1f\n",
            sourceEntityNum, muzzle[0], muzzle[1], muzzle[2], position[0], position[1], position[2], tracerChance);

    {
        int randInt = rand();
        float randValue;

        if (randInt < 0) {
            unsigned int adjusted = ((unsigned int)randInt >> 1) | (randInt & 1);
            randValue = (float)adjusted * 2.0f;
        } else {
            randValue = (float)randInt;
        }

        if (2147483648.0f * tracerChance > randValue)
            CG_SpawnTracer(muzzle, position);
    }

    CG_WhizbySound(muzzle, position);
}

static float CG_TracerScaleForDistance(float dist, float width, float tracerScale, float minDist, float distRange)
{
    if (minDist != 0.0f)
        dist -= minDist;

    if (dist <= 0.0f)
        return width;

    if (distRange > 0.0f) {
        float scale = (dist / distRange) * tracerScale;

        if (scale < 1.0f)
            scale = 1.0f;
        else if (scale > tracerScale)
            scale = tracerScale;

        return width * scale;
    }

    return width * tracerScale;
}

void CG_DrawTracer(vec_t *start, vec_t *finish)
{
    vec3_t dir;
    vec3_t right;
    vec_t *viewOrg;
    vec_t *viewAxis0;
    vec_t *viewAxis1;
    float forwardDot;
    float rightDot;
    float width;
    float startWidth;
    float finishWidth;
    float tracerScale;
    float tracerScaleMinDist;
    float tracerScaleDistRange;
    GfxWorldVertex verts[4];

    dir[0] = finish[0] - start[0];
    dir[1] = finish[1] - start[1];
    dir[2] = finish[2] - start[2];

    viewOrg = cg->refdef.vieworg;
    viewAxis0 = cg->refdef.viewaxis[1];
    viewAxis1 = cg->refdef.viewaxis[2];

    forwardDot = dir[0] * viewAxis0[0] + dir[1] * viewAxis0[1] + dir[2] * viewAxis0[2];
    rightDot = dir[0] * viewAxis1[0] + dir[1] * viewAxis1[1] + dir[2] * viewAxis1[2];

    right[0] = rightDot * viewAxis0[0] - forwardDot * viewAxis1[0];
    right[1] = rightDot * viewAxis0[1] - forwardDot * viewAxis1[1];
    right[2] = rightDot * viewAxis0[2] - forwardDot * viewAxis1[2];
    Vec3Normalize(right);

    width = cg_tracerWidth->current.value;
    tracerScale = cg_tracerScale->current.value;
    tracerScaleMinDist = cg_tracerScaleMinDist->current.value;
    tracerScaleDistRange = cg_tracerScaleDistRange->current.value;

    if (tracerScale == 1.0f) {
        startWidth = width;
        finishWidth = width;
    } else {
        startWidth = CG_TracerScaleForDistance(Vec3Distance(start, viewOrg), width, tracerScale,
                                               tracerScaleMinDist, tracerScaleDistRange);
        finishWidth = CG_TracerScaleForDistance(Vec3Distance(finish, viewOrg), width, tracerScale,
                                                tracerScaleMinDist, tracerScaleDistRange);
    }

    verts[0].xyz[0] = finish[0] + right[0] * finishWidth;
    verts[0].xyz[1] = finish[1] + right[1] * finishWidth;
    verts[0].xyz[2] = finish[2] + right[2] * finishWidth;
    verts[0].color.packed = 0xffffffff;
    verts[0].texCoord[0] = 1.0f;
    verts[0].texCoord[1] = 1.0f;
    verts[0].lmapCoord[0] = 1.0f;
    verts[0].lmapCoord[1] = 1.0f;

    verts[1].xyz[0] = finish[0] - right[0] * finishWidth;
    verts[1].xyz[1] = finish[1] - right[1] * finishWidth;
    verts[1].xyz[2] = finish[2] - right[2] * finishWidth;
    verts[1].color.packed = 0xffffffff;
    verts[1].texCoord[0] = 1.0f;
    verts[1].texCoord[1] = 0.0f;
    verts[1].lmapCoord[0] = 1.0f;
    verts[1].lmapCoord[1] = 0.0f;

    verts[2].xyz[0] = start[0] - right[0] * startWidth;
    verts[2].xyz[1] = start[1] - right[1] * startWidth;
    verts[2].xyz[2] = start[2] - right[2] * startWidth;
    verts[2].color.packed = 0xffffffff;
    verts[2].texCoord[0] = 0.0f;
    verts[2].texCoord[1] = 0.0f;
    verts[2].lmapCoord[0] = 0.0f;
    verts[2].lmapCoord[1] = 0.0f;

    verts[3].xyz[0] = start[0] + right[0] * startWidth;
    verts[3].xyz[1] = start[1] + right[1] * startWidth;
    verts[3].xyz[2] = start[2] + right[2] * startWidth;
    verts[3].color.packed = 0xffffffff;
    verts[3].texCoord[0] = 0.0f;
    verts[3].texCoord[1] = 1.0f;
    verts[3].lmapCoord[0] = 0.0f;
    verts[3].lmapCoord[1] = 1.0f;

    CL_DefaultVertexFrames(4, verts);

    CL_AddPolyToScene(cgs->media.tracerMaterial, 0x1f, 4, verts);
}

void CG_WeaponSlot_f(qboolean next, qboolean ignoreEmpty)
{
    int serverTime;
    int slot;
    int weaponIndex;
    int oldWeaponIndex;
    int oldAltWeaponIndex;

    (void)next;
    (void)ignoreEmpty;

    if (!CG_CanCycleWeapon(&cg, &serverTime))
        return;

    cg->weaponSelectTime = serverTime;

    slot = BG_GetWeaponSlotForName(CG_Argv(1));
    if (!slot)
        slot = atoi(CG_Argv(1));

    if ((unsigned int)(slot - 1) > 1)
        return;

    weaponIndex = (signed char)cg->predictedPlayerState.weaponslots[slot];
    if (!weaponIndex)
        return;

    if (((cg->predictedPlayerState.weapons[weaponIndex >> 5] >> (weaponIndex & 0x1f)) & 1) == 0)
        return;

    oldWeaponIndex = cg->weaponSelect;
    if (oldWeaponIndex == weaponIndex)
        return;

    oldAltWeaponIndex = ((WeaponDef *)BG_GetWeaponDef(oldWeaponIndex))->iAltWeaponIndex;
    cg->weaponSelect = weaponIndex;
    CG_MenuShowNotify(1);

    if (weaponIndex != oldAltWeaponIndex)
        CL_SetADS(0);
}

qboolean CG_SelectFirstWeaponNotInSlot(qboolean bNext, qboolean bIgnoreEmpty)
{
    int weaponIndex;
    int step;

    if (bNext) {
        step = 1;
        weaponIndex = 1;
    } else {
        int n = BG_GetNumWeapons();
        if (n <= 0)
            return 0;
        step = -1;
        weaponIndex = n;
    }

    for (; weaponIndex > 0 && weaponIndex <= BG_GetNumWeapons(); weaponIndex += step) {
        if (!((cg->predictedPlayerState.weapons[weaponIndex >> 5] >> (weaponIndex & 0x1f)) & 1))
            continue;

        if (BG_IsPlayerWeaponInSlot(&cg->predictedPlayerState, weaponIndex, 1))
            continue;

        if (BG_GetStackSlotForWeapon(&cg->predictedPlayerState, weaponIndex, 0))
            continue;

        if (!BG_DoesWeaponNeedSlot(weaponIndex))
            continue;

        if (bIgnoreEmpty && !BG_WeaponAmmo(&cg->predictedPlayerState, weaponIndex))
            continue;

        cg->weaponSelectTime = cg->time;
        if (cg->weaponSelect != weaponIndex) {
            int oldAlt = ((WeaponDef *)BG_GetWeaponDef(cg->weaponSelect))->iAltWeaponIndex;
            cg->weaponSelect = weaponIndex;
            CG_MenuShowNotify(1);
            if (oldAlt != weaponIndex)
                CL_SetADS(0);
        }
        return 1;
    }

    return 0;
}

static qboolean CG_HasWeapon(cg_t *cg, int weaponIndex)
{
    return ((cg->predictedPlayerState.weapons[weaponIndex >> 5] >> (weaponIndex & 0x1f)) & 1) != 0;
}

static int CG_WeaponInSlot(cg_t *cg, int slot)
{
    return (signed char)cg->predictedPlayerState.weaponslots[slot];
}

static int CG_StepWeaponSlot(int slot, int step)
{
    slot += step;
    if (slot < 1)
        return 2;
    if (slot > 2)
        return 1;
    return slot;
}

static inline __attribute__((always_inline)) qboolean CG_SelectWeaponFromSlots(qboolean bNext, qboolean bIgnoreEmpty)
{
    playerState_t *ps;
    int step;
    int slot;

    step = bNext ? 1 : -1;
    slot = bNext ? 1 : 2;

    for (; slot >= 1 && slot <= 2; slot += step) {
        int weaponIndex;

        ps = &cg->predictedPlayerState;
        weaponIndex = CG_WeaponInSlot(cg, slot);
        if (!weaponIndex)
            continue;

        if (bIgnoreEmpty && !BG_WeaponAmmo(ps, weaponIndex))
            continue;

        CG_SelectWeaponIndex_core(weaponIndex);
        return 1;
    }

    return 0;
}

static inline __attribute__((always_inline)) qboolean CG_SelectOtherWeaponSlot(cg_t *cg, playerState_t *ps, int currentSlot, int step, int loopedSlot, qboolean bIgnoreEmpty)
{
    int slot;

    slot = currentSlot;
    for (;;) {
        int weaponIndex;

        slot = CG_StepWeaponSlot(slot, step);
        if (slot == loopedSlot)
            return 0;

        weaponIndex = CG_WeaponInSlot(cg, slot);
        if (!weaponIndex)
            continue;

        if (bIgnoreEmpty && !BG_WeaponAmmo(ps, weaponIndex))
            continue;

        CG_SelectWeaponIndex_core(weaponIndex);
        return 1;
    }
}

static inline __attribute__((always_inline)) qboolean CG_SelectNextNonSlotWeapon(qboolean bNext, qboolean bIgnoreEmpty)
{
    playerState_t *ps;
    int numWeapons;
    int step;
    int loopedWeapon;
    int currentWeapon;
    int weaponIndex;

    ps = &cg->predictedPlayerState;
    numWeapons = BG_GetNumWeapons();
    if (numWeapons <= 0)
        return 0;

    step = bNext ? 1 : -1;
    loopedWeapon = bNext ? 1 : numWeapons;
    currentWeapon = cg->weaponSelect;
    weaponIndex = currentWeapon;

    for (;;) {
        weaponIndex = ((weaponIndex + step + numWeapons - 1) % numWeapons) + 1;
        if (weaponIndex == loopedWeapon)
            return 0;

        ps = &cg->predictedPlayerState;

        if (!CG_HasWeapon(cg, weaponIndex))
            continue;

        if (BG_IsPlayerWeaponAnAlt(weaponIndex, currentWeapon))
            continue;

        if (BG_IsPlayerWeaponInSlot(ps, weaponIndex, 1))
            continue;

        if (BG_GetStackSlotForWeapon(ps, weaponIndex, 0))
            continue;

        if (bIgnoreEmpty && !BG_WeaponAmmo(ps, weaponIndex))
            continue;

        if (!BG_DoesWeaponNeedSlot(weaponIndex))
            continue;

        CG_SelectWeaponIndex_core(weaponIndex);
        return 1;
    }
}

void CG_CycleWeap(qboolean bNext, qboolean bIgnoreEmpty)
{
    snapshot_t *snap;
    playerState_t *ps;
    int currentWeapon;
    int currentSlot;
    int step;

    snap = cg->nextSnap;
    if (!snap)
        return;

    if ((((byte *)&snap->ps.pm_flags)[2] & 0x80) == 0)
        return;

    ps = &cg->predictedPlayerState;
    currentWeapon = cg->weaponSelect;
    step = bNext ? 1 : -1;

    currentSlot = BG_IsPlayerWeaponInSlot(ps, currentWeapon, 1);
    if (!currentSlot)
        currentSlot = BG_GetStackSlotForWeapon(ps, currentWeapon, 0);

    if (currentSlot) {
        if (CG_SelectOtherWeaponSlot(cg, ps, currentSlot, step, bNext ? 1 : 2, bIgnoreEmpty))
            return;

        if (CG_SelectFirstWeaponNotInSlot(bNext, bIgnoreEmpty))
            return;

        if (CG_SelectWeaponFromSlots(bNext, bIgnoreEmpty))
            return;
    } else {
        if (CG_SelectNextNonSlotWeapon(bNext, bIgnoreEmpty))
            return;

        if (CG_SelectWeaponFromSlots(bNext, bIgnoreEmpty))
            return;

        if (CG_SelectFirstWeaponNotInSlot(bNext, bIgnoreEmpty))
            return;
    }

    currentWeapon = cg->weaponSelect;
    if (currentWeapon && !CG_HasWeapon(cg, currentWeapon))
        CG_SelectWeaponIndex_core(0);
}

void CG_OutOfAmmoChange(void)
{
    playerState_t *ps;
    int currentWeapon;
    WeaponDef *currentDef;
    int i;

    if (!cg->nextSnap)
        return;

    ps = &cg->predictedPlayerState;
    currentWeapon = cg->predictedPlayerState.weapon;
    currentDef = (WeaponDef *)BG_GetWeaponDef(currentWeapon);

    if (currentDef->bSlotStackable) {
        for (i = 1; i <= BG_GetNumWeapons(); i++) {
            WeaponDef *newDef;

            if (!((cg->predictedPlayerState.weapons[i >> 5] >> (i & 0x1f)) & 1))
                continue;

            newDef = (WeaponDef *)BG_GetWeaponDef(i);
            if (!newDef->bSlotStackable)
                continue;

            if (newDef->weapSlot != currentDef->weapSlot)
                continue;

            if (!BG_WeaponAmmo(ps, i))
                continue;

            CG_SelectWeaponIndex_core(i);
            return;
        }
    }

    if (BG_IsPlayerWeaponInSlot(ps, currentWeapon, 1)) {
        for (i = 0; i < 2; i++) {
            int slot = iSlotPreferenceOrder[i];
            int newWeapon = (signed char)cg->predictedPlayerState.weaponslots[slot];

            if (!newWeapon)
                continue;

            if (!BG_WeaponAmmo(ps, newWeapon))
                continue;

            CG_SelectWeaponIndex_core(newWeapon);
            return;
        }
    }

    CG_CycleWeap(1, 1);
}

static inline __attribute__((always_inline)) qboolean CG_CanCycleWeapon(cg_t **cgOut, int *serverTimeOut)
{
    snapshot_t *ps;
    int serverTime;
    const dvar_t *cycleDelay;

    ps = cg->nextSnap;
    if (!ps)
        return 0;

    if (*(short *)&cg->predictedPlayerState.pm_flags < 0)
        return 0;

    if ((((byte *)&ps->ps.pm_flags)[2] & 0x80) == 0)
        return 0;

    serverTime = cg->time;
    cycleDelay = cg_weaponCycleDelay;
    if (serverTime - cg->weaponSelectTime < cycleDelay->current.integer)
        return 0;

    *cgOut = cg;
    *serverTimeOut = serverTime;
    return 1;
}

void CG_PrevWeapon_f(void)
{
    int serverTime;

    if (CG_ScoreboardDisplayed()) {
        CG_ScrollScoreboardDown();
        return;
    }

    if (!CG_CanCycleWeapon(&cg, &serverTime))
        return;

    cg->weaponSelectTime = serverTime;
    CG_CycleWeap(0, 0);
}

void CG_NextWeapon_f(void)
{
    int serverTime;

    if (CG_ScoreboardDisplayed()) {
        CG_ScrollScoreboardUp();
        return;
    }

    if (!CG_CanCycleWeapon(&cg, &serverTime))
        return;

    cg->weaponSelectTime = serverTime;
    CG_CycleWeap(1, 0);
}

static inline __attribute__((always_inline)) void CG_SetWeaponXAnimWeights(struct XAnimTree_s *tree, weaponInfo_t *weapInfo, int activeAnim)
{
    int i;

    for (i = 1; i < 0x15; i++) {
        float rate = weapInfo->viewModelAnimRates[i];

        if (i == activeAnim)
            XAnimSetGoalWeight(tree, i, 1.0f, 0.0f, rate, 0, 1, 1);
        else
            XAnimSetGoalWeight(tree, i, 0.0f, 0.0f, rate, 0, 0, 0);
    }
}

void CG_WeaponRunXModelAnims(playerState_t *ps, weaponInfo_t *weapInfo)
{
    static const int weapAnimToXAnim[20] = {
        0, 1, 3, 5, 6, 18, 19, 20, 7, 13,
        12, 8, 9, 10, 11, 15, 14, 17, 16, 4
    };
    byte *psBytes;
    WeaponDef *weapDef;
    struct XAnimTree_s *pAnimTree;
    int weaponIndex;
    int weapAnim;
    int maskedAnim;
    int activeAnim;
    int flags;
    qboolean adsIn;

    psBytes = (byte *)ps;
    pAnimTree = DObjGetTree(weapInfo->viewModelDObj);
    weaponIndex = BG_GetViewmodelWeaponIndex(ps);
    weapDef = (WeaponDef *)BG_GetWeaponDef(weaponIndex);

    flags = ((playerState_t *)psBytes)->pm_flags;
    adsIn = (flags & 0x40) && !(flags & 0x10);
    if (weapDef->bADSPositionInfo)
        CG_PlayADSAnim(weaponIndex, pAnimTree, adsIn ? 0x15 : 0x16);

    weapAnim = ((playerState_t *)psBytes)->weapAnim;
    if (weapAnim == weapInfo->iPrevAnim)
        return;

    maskedAnim = weapAnim & ~0x200;
    if (maskedAnim == 0) {
        int i;
        int clip;

        for (i = 1; i < 0x15; i++) {
            if (!XAnimHasFinished(pAnimTree, i)) {
                weapInfo->iPrevAnim = -1;
                return;
            }
        }

        clip = BG_ClipForWeapon(weaponIndex);
        activeAnim = *(int *)(((char *)psBytes + offsetof(playerState_t, ammoclip[0])) + clip * 4) ? 1 : 2;
    } else if (maskedAnim <= 0x13) {
        activeAnim = weapAnimToXAnim[maskedAnim];
    } else {
        activeAnim = 1;
    }

    CG_SetWeaponXAnimWeights(pAnimTree, &(*(weaponInfo_t **)imp_cg_weapons)[weaponIndex], activeAnim);

    if (maskedAnim > 0x13)
        Com_Printf("CG_WeaponRunXModelAnims: Unknown weapon animation %i\n", maskedAnim);

    weapInfo->iPrevAnim = weapAnim;
}

static inline __attribute__((always_inline)) void CG_ResetViewWeaponAnimTree(byte *psBytes, int weaponNum, weaponInfo_t *weapInfo)
{
    struct XAnimTree_s *tree;
    int clip;
    int activeAnim;

    tree = DObjGetTree(weapInfo->viewModelDObj);
    XAnimClearTreeGoalWeights(tree, 0, 0);
    XAnimSetGoalWeight(tree, 0, 1.0f, 0.0f, weapInfo->viewModelAnimRates[0], 0, 1, 0);

    clip = BG_ClipForWeapon(weaponNum);
    activeAnim = *(int *)(((char *)psBytes + offsetof(playerState_t, ammoclip[0])) + clip * 4) ? 1 : 2;
    CG_SetWeaponXAnimWeights(tree, weapInfo, activeAnim);
}

static inline __attribute__((always_inline)) void CG_PlayViewWeaponNotetrackSounds(weaponInfo_t *weapInfo)
{
    XAnimNotify *notifyList;
    int notifyCount;
    int i;

    notifyCount = DObjGetClientNotifyList(&notifyList);
    if (notifyCount <= 0)
        return;

    for (i = 0; i < notifyCount; i++) {
        snd_alias_list_t *aliasList = 0;
        const char *name = notifyList[i].name;

        if (!stricmp(name, "noteTrackSoundA"))
            aliasList = weapInfo->noteTrackSound[0];
        else if (!stricmp(name, "noteTrackSoundB"))
            aliasList = weapInfo->noteTrackSound[1];
        else if (!stricmp(name, "noteTrackSoundC"))
            aliasList = weapInfo->noteTrackSound[2];
        else if (!stricmp(name, "noteTrackSoundD"))
            aliasList = weapInfo->noteTrackSound[3];

        if (aliasList)
            CG_PlayClientSoundAlias(aliasList);
    }
}

void CG_UpdateViewWeaponAnim(playerState_t *ps)
{
    byte *psBytes;
    weaponInfo_t *weapInfoBase;
    weaponInfo_t *weapInfo;
    struct DObj_s *obj;
    int weaponIndex;
    int weaponNum;
    int partBits[4];

    psBytes = (byte *)ps;

    if (*(int *)(psBytes + 4) > 5) {
        for (weaponNum = 1; weaponNum <= BG_GetNumWeapons(); weaponNum++) {
            weapInfo = &(*(weaponInfo_t **)imp_cg_weapons)[weaponNum];
            if (weapInfo->viewModelDObj)
                CG_ResetViewWeaponAnimTree(psBytes, weaponNum, weapInfo);
        }
        return;
    }

    weaponIndex = BG_GetViewmodelWeaponIndex(ps);
    if (weaponIndex <= 0)
        return;

    CG_RegisterWeapon(weaponIndex);
    weapInfoBase = *(weaponInfo_t **)imp_cg_weapons;
    weapInfo = &weapInfoBase[weaponIndex];
    CG_WeaponRunXModelAnims(ps, weapInfo);

    obj = weapInfo->viewModelDObj;
    DObjUpdateClientInfo(obj, (float)cg->frametime * 0.0010000000474974513f);

    partBits[0] = -1;
    partBits[1] = -1;
    partBits[2] = -1;
    partBits[3] = -1;

    if (!CL_DObjCreateSkelForBones(obj, partBits, 0)) {
        DObjCalcAnim(obj, partBits);
        DObjCalcSkel(obj, partBits);
    }

    weaponIndex = BG_GetViewmodelWeaponIndex(&cg->predictedPlayerState);
    if (!weaponIndex)
        return;

    weapInfo = &weapInfoBase[weaponIndex];
    CG_PlayViewWeaponNotetrackSounds(weapInfo);
}
