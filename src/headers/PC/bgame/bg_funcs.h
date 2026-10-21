#ifndef CLEAN_PC_BGAME_BG_FUNCS_H
#define CLEAN_PC_BGAME_BG_FUNCS_H

#include "common_types.h"
#include "bytematch.h"

#ifndef BG_PRONE_TURNED_ABI
#  define BG_PRONE_TURNED_ABI COD2_REGPARM(2) COD2_SSEREGPARM
#endif
#ifndef PM_ACCELERATE_ABI
#  define PM_ACCELERATE_ABI COD2_REGPARM(3) COD2_SSEREGPARM
#endif
#ifndef PM_REGPARM2_ABI
#  define PM_REGPARM2_ABI COD2_REGPARM(2) BM_NOINLINE
#endif
#ifndef PM_SLIDEMOVE_ABI
#  define PM_SLIDEMOVE_ABI COD2_REGPARM(3) BM_NOINLINE
#endif

void BG_InitWeaponStrings(void);
int BG_PlayAnim(playerState_t *ps, int animNum, animBodyPart_t bodyPart, int forceDuration, qboolean setTimer, qboolean isContinue, qboolean force);
void BG_UpdatePlayerDObj(struct DObj_s *pDObj, entityState_t *es, clientInfo_t *ci, int attachIgnoreCollision);
void BG_LerpOffset(vec_t *offset_goal, float maxOffsetChange, vec_t *offset);
void BG_UpdateConditionValue(int client, int condition, int value, qboolean checkConversion);
void BG_AnimUpdatePlayerStateConditions(pmove_t *pmove);
void BG_Player_DoControllers(const struct DObj_s *pDObj, const entityState_t *es, int *partBits, clientInfo_t *ci, int frametime);
void BG_PlayerAnimation(const struct DObj_s *pDObj, entityState_t *es, clientInfo_t *ci);
int BG_AnimationIndexForString(const char *string, const char *string_1);
void BG_AnimParseError(const char *msg, ...);
int BG_IndexForString(const char *token, animStringItem_t *strings, qboolean allowFail);
void BG_ParseCommands(const char **input, animScriptItem_t *scriptItem, animScriptData_t *scriptData);

void Jump_RegisterDvars(void);
void Jump_ClearState(playerState_t *ps);
Bool Jump_GetStepHeight(playerState_t *ps, const vec_t *origin, float *stepSize);
Bool Jump_IsPlayerAboveMax(playerState_t *ps);
void Jump_ActivateSlowdown(playerState_t *ps);
void Jump_ApplySlowdown(playerState_t *ps);
float Jump_ReduceFriction(playerState_t *ps);
Bool Jump_Check(pmove_t *pm, pml_t *pml);
void Jump_ClampVelocity(playerState_t *ps, const vec_t *origin);

void Mantle_RegisterDvars(void);
void Mantle_ShutdownAnims(void);
void Mantle_ClearHint(playerState_t *ps);
void Mantle_CapView(playerState_t *ps);
Bool Mantle_IsWeaponInactive(playerState_t *ps);
void Mantle_CreateAnims(MantleAnimAlloc xanimAlloc);
void Mantle_Move(pmove_t *pm, playerState_t *ps, pml_t *pml);
void Mantle_Check(pmove_t *pm, pml_t *pml);

void BG_RegisterDvars(void);
const gitem_t *BG_FindItemForWeapon(int weapon);
const gitem_t *G_FindItem(const char *pickupName);
void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t *ps);
void BG_PlayerStateToEntityState(playerState_t *ps, entityState_t *s, qboolean snap, int handler);
void BG_EvaluateTrajectoryDelta(const trajectory_t *tr, int atTime, vec_t *result);
qboolean BG_CanItemBeGrabbed(const entityState_t *ent, const playerState_t *ps, qboolean bTouched);
qboolean BG_CheckProneValid(int passEntityNum, const vec_t *vPos, const float fSize, const float fHeight, const float fYaw, float *pfTorsoHeight, float *pfTorsoPitch, float *pfWaistPitch, const qboolean bAlreadyProne, const qboolean bOnGround, vec_t *vGroundNormal, int handler, proneCheckType_t proneCheckType, float prone_feet_dist);
qboolean BG_CheckProne(int passEntityNum, const vec_t *vPos, const float fSize, const float fHeight, const float fYaw, float *pfTorsoHeight, float *pfTorsoPitch, float *pfWaistPitch, const qboolean bAlreadyProne, const qboolean bOnGround, vec_t *vGroundNormal, int handler, proneCheckType_t proneCheckType, float prone_feet_dist);
void BG_EvaluateTrajectory(const trajectory_t *tr, int atTime, vec_t *result);
qboolean BG_PlayerTouchesItem(playerState_t *ps, entityState_t *item, int atTime);

void PM_trace(pmove_t *pm, trace_t *results, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentMask);
void PM_AddTouchEnt(pmove_t *pm, int entityNum);
void PM_AddEvent(playerState_t *ps, int newEvent);
int PM_GetEffectiveStance(playerState_t *ps);
int PM_GroundSurfaceType(pml_t *pml);
int PM_GetViewHeightLerpTime(const playerState_t *ps, int iTarget, qboolean bDown);
void PM_SetProneMovementOverride(playerState_t *ps);
float BG_GetSpeed(const playerState_t *ps, int time);
qboolean PM_ShouldMakeFootsteps(pmove_t *pm);
void PM_ClipVelocity(const vec_t *in, const vec_t *normal, vec_t *out);
void PM_UpdateLean(playerState_t *ps, float msec, usercmd_t *cmd, void (*capsuleTrace)());
void PM_UpdateViewAngles(playerState_t *ps, float msec, usercmd_t *cmd, int handler);
void PM_playerTrace(pmove_t *pm, trace_t *results, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentMask);
void PM_UpdatePronePitch(pmove_t *pm, pml_t *pml);
void PM_FootstepEvent(pmove_t *pm, pml_t *pml, int iOldBobCycle, int iNewBobCycle, qboolean bFootStep);
/* Strips movement and actions from a frozen player's command. */
void PM_FilterFrozenCommand(const playerState_t *ps, usercmd_t *cmd);
void Pmove(pmove_t *pm);

void PM_StepSlideMove(pmove_t *pm, pml_t *pml, qboolean gravity);

void BG_ShutdownWeaponDefFiles(void);
WeaponDef *BG_GetWeaponDef(int iWeapon);
void BG_ClearWeaponDef(void);
int BG_GetNumWeapons(void);
int BG_GetAmmoTypeMax(int iAmmoIndex);
int BG_GetAmmoClipSize(int iClipIndex);
int BG_FindWeaponIndexForName(const char *name);
qboolean BG_IsAnyEmptyPrimaryWeaponSlot(const playerState_t *ps);
Bool PM_IsBinocularsADS(const playerState_t *ps);
void PM_ExitAimDownSight(playerState_t *ps);
float BG_GetBobCycle(const playerState_t *ps);
float BG_GetVerticalBobFactor(const playerState_t *ps, float cycle, float speed, float maxAmp);
float BG_GetHorizontalBobFactor(const playerState_t *ps, float cycle, float speed, float maxAmp);
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
int BG_GetFirstEquippedOffhand(const playerState_t *ps, int offhandClass);
void PM_AdjustAimSpreadScale(pmove_t *pm, pml_t *pml);
int BG_GetViewmodelWeaponIndex(const playerState_t *ps);
void BG_WeaponFireRecoil(const playerState_t *ps, vec_t *vGunSpeed, vec_t *kickAVel);
void BG_CalculateWeaponPosition_Sway(playerState_t *ps, vec_t *swayViewAngles, vec_t *swayOffset, vec_t *swayAngles, float ssSwayScale, int frametime);
int BG_GetFirstAvailableOffhand(const playerState_t *ps, int offhandClass);
int BG_GetMaxPickupableAmmo(const playerState_t *ps, int weaponIndex);
void PM_UpdateAimDownSightFlag(pmove_t *pm, pml_t *pml);
Bool BG_IsWeaponValid(const playerState_t *ps, int weaponIndex);
qboolean BG_TakePlayerWeapon(playerState_t *pPS, int iWeaponIndex);
void PM_UpdateAimDownSightLerp(pmove_t *pm, pml_t *pml);
void PM_ResetWeaponState(playerState_t *ps);
qboolean PM_InteruptWeaponWithProneMove(playerState_t *ps);
void PM_Weapon(pmove_t *pm, pml_t *pml);
WeaponDef *BG_LoadWeaponDef(const char *folder, const char *name);
int BG_SetupWeaponDef(WeaponDef *weapDef, void (*regWeap)(int));
int BG_GetWeaponIndexForName(const char *name, BG_RegisterWeapon regWeap);
void BG_FillInAmmoItems(BG_RegisterWeapon regWeap);
void CG_SetWeaponDefToDefaultWeapon(int iWeapon);
void BG_CalculateWeaponAngles(weaponState_t *ws, vec_t *angles);
void BG_CalculateViewAngles(viewState_t *vs, vec_t *angles);
void BG_CalculateViewIdleAngles(viewState_t *vs, vec_t *angles);

int BG_GetWeaponSlotForName(const char *pszSlotName);
const char *BG_GetWeaponSlotNameForIndex(int iSlot);
void BG_LoadWeaponStrings(void);
void BG_LoadPlayerAnimTypes(void);
void SetConfigString(char **ppszConfigString, const char *pszKeyValue);
WeaponDef *BG_LoadDefaultWeaponDef(void);
WeaponDef *BG_LoadWeaponDefInternal(const char *folder, const char *name);

#endif
