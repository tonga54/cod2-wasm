#ifndef CLEAN_PC_CGAME_MP_CG_FUNCS_H
#define CLEAN_PC_CGAME_MP_CG_FUNCS_H

#include "common_types.h"

void CGScr_LoadAnimTrees(void);
void CG_SetDObjInfo(int iEntNum, int iEntType, struct XModel *pXModel);
qboolean CG_CheckDObjInfoMatches(int iEntNum, int iEntType, struct XModel *pXModel);
void CG_SafeDObjFree(int iEntNum);
void CG_FreeClientDObjInfo(void);
void CG_FreeEntityDObjInfo(void);

void CG_ApplyCompassPointerRadiusScale(float *radiusScale);
void CG_CompassAddWeaponPingInfo(centity_t *cent, const vec_t *origin, int msec);
void CG_DrawCompassFriendlies(rectDef_t *rect, MaterialHandle material, vec_t *color);

void CG_TargetCommand_f(void);
void CG_ScoresUp_f(void);
void CG_ScoresDown_f(void);
qboolean CG_IsConsoleCommandName(const char *cmd);
qboolean CG_ConsoleCommand(void);
void CG_InitConsoleCommands(void);

void CG_StartShakeCamera(float scale, int duration, const vec_t *src, float radius);
void CG_DrawTeamBackground(float x, float y, float w, float h, float alpha, int team);
void CG_DrawScriptUsage(void);
void CG_CalculateFPS(void);
void CG_AddLagometerFrameInfo(void);
void CG_AddLagometerSnapshotInfo(snapshot_t *snap);
void CG_DrawDisconnect(void);
void CG_PriorityCenterPrint(const char *str, float charWidth, int priority);
qboolean CG_GetWeapReticleZoom(float *pfZoom);  /* qboolean(int), not Bool: callers read full eax (see cg_draw_mp.c) */
void CG_DrawFrameOverlay(float innerLeft, float innerRight, float innerTop, float innerBottom, const vec_t *color, MaterialHandle material);
unsigned int CG_DrawCrosshairNames(void);
unsigned int CG_CheckTimedMenus(void);
unsigned int CG_DrawMaterial(void);
unsigned int CG_ShakeCamera(void);
qboolean CG_DrawFollow(void);
void CG_DrawBoldGameMessages(void);
void CG_DrawTurretCrossHair(void);
void CG_DrawPlayerSprites(void);
void CG_DrawActive(void);
unsigned int CG_DrawChatMessages(void);
float CG_DrawWeapReticle(void);
unsigned int CG_DrawCrosshair(void);
unsigned int CG_Draw2D(void);

void CG_DrawStringExt(float x, float y, const char *string, const vec_t *setColor, qboolean forceColor, qboolean shadow, float charHeight, qboolean adjust);
int CG_DrawDevString(float x, float y, const char *s, const vec_t *color, int align_in, FontHandle font, int style);
int CG_DrawBigDevStringColor(float x, float y, const char *s, const vec_t *color, int align);
int CG_DrawSmallDevStringColor(float x, float y, const char *s, const vec_t *color, int align);
float *CG_FadeColor(int startMsec, int totalMsec, int fadeMsec);
void CG_NorthDirectionChanged(void);
void CG_UpdateCompassOrientation(void);
void CG_UpdateCompPointerOrientation(void);
void CG_TileClear(void);
int CG_DrawBigDevString(float x, float y, const char *s, float alpha, int align);
void CG_DrawRotatedPicPhysical(float x, float y, float width, float height, float angle, const vec_t *color, MaterialHandle material);
void CG_DrawRotatedPic(float x, float y, float width, float height, int horzAlign, int vertAlign, float angle, const vec_t *color, MaterialHandle material);
void CG_DrawRotatedQuadPic(float x, float y, vec2_t *verts, float angle, const vec_t *color, MaterialHandle material);

struct XAnim_s *CG_GetMG42Anims(centity_t *cent);
void CG_SetFrameInterpolation(void);
void CG_ProcessClientNoteTracks(int clientNum);
void CG_DObjUpdateInfo(struct DObj_s *obj);
void CG_SoundBlend(centity_t *cent);
void CG_UsedDObjCalcPose(const centity_t *cent);
void CG_CullIn(const centity_t *cent);
void CG_AddCEntityToScene(const GfxEntity *ent, const struct DObj_s *obj, const centity_t *cent);
qboolean CG_DObjGetViewModelTagPos(struct DObj_s *obj, unsigned int tagName, vec_t *pos);
void CG_DObjCalcBoneGeneric(int handle, int localClientNum, int boneIndex);
void CG_AdjustPositionForMover(const vec_t *in, int moverNum, int fromTime, int toTime, vec_t *out, vec_t *outDeltaAngles);
void CG_CalcEntityLerpPositions(centity_t *cent);
void CG_EntityEffects(centity_t *cent);
void CG_DObjCalcPose(const centity_t *cent, const struct DObj_s *obj, int *partBits);
void CG_DObjCalcBone(const centity_t *cent, struct DObj_s *obj, int boneIndex);
DObjAnimMat *CG_DObjGetLocalTagMatrix(const centity_t *cent, struct DObj_s *obj, unsigned int tagName);
qboolean CG_DObjGetWorldTagPos(const centity_t *cent, struct DObj_s *obj, unsigned int tagName, vec_t *pos);
void CG_General(centity_t *cent);
void CG_Item(centity_t *cent);
void CG_Missile(centity_t *cent);
void CG_ScriptMover(centity_t *cent);
void CG_ProcessEntity(centity_t *cent);
void CG_AddPacketEntities(void);

void CG_EntityEvent(centity_t *cent, int event);
void CG_CheckEvents(centity_t *cent);

Bool CG_AreHudElemsHidden(void);
float CG_AlignHudElemX(int alignOrg, float x, float width);
float CG_AlignHudElemY(int alignOrg, float y, float height);
int compare_hudelems(const inflate_huft *pe0, const inflate_huft *pe1);
void CG_Draw2dHudElems(qboolean foreground);
void CG_Draw3dHudElems(void);

void CG_LoadingString(const char *s);
void CG_DrawInformation(qboolean serverLoading);

void CG_InitLocalEntities(void);
void CG_AddMovingTracer(localEntity_t *le);
localEntity_t *CG_AllocLocalEntity(void);
void CG_AddLocalEntities(void);

void CG_GetEntityOrientation(int entnum, vec_t *origin_out, vec3_t *axis_out);
int CG_CrosshairPlayer(void);
void CG_GameMessage(const char *msg);
void CG_BoldGameMessage(const char *msg);
const char *CG_Argv(int arg);
void CG_StartAmbient(void);
Bool CG_PlaySoundOnFirstClient(void);
Bool CG_PlaySoundOnCurrentLocalClient(int entitynum, const vec_t *origin, const char *aliasname);
void CG_TranslateHudElemMessage(const char *message, const char *messageType, char *hudElemString);
void CG_InitVote(void);
void CG_FreeWeapons(void);
void CG_Shutdown(void);
void *Hunk_AllocXAnimPrecache(int size);
void *Hunk_AllocXAnimClient(int size);
int CG_PlaySoundAliasAsMasterByName(int entitynum, const vec_t *origin, const char *aliasname);
void CG_GetDObjOrientation(int dobjHandle, orientation_t *orient);
void CG_PlaySmokeGrenadesAtTime(int gametime);
int CG_PlaySoundAlias(int entitynum, const vec_t *origin, snd_alias_list_t *aliasList);
int CG_PlaySoundAliasByName(int entitynum, const vec_t *origin, const char *aliasname);
void CG_SafeTranslateHudElemString(int index, char *hudElemString);
int CG_PlayClientSoundAliasByName(const char *aliasname);
int CG_PlayClientSoundAlias(snd_alias_list_t *aliasList);
int CG_PlayEntitySoundAlias(int entitynum, snd_alias_list_t *aliasList);
void CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum);

void CG_InitMarkPolys(void);
void CG_AddMarks(void);
void CG_ImpactMark(MaterialHandle markMaterial, const vec_t *origin, const vec_t *dir, float orientation, const vec_t *color, float radius);

void CG_AntiBurnInHUD_RegisterDvars(void);
Bool CG_AreHudMenusHidden(void);
float CG_CalcPlayerHealth(void);
void CG_ResetLowHealthOverlay(void);
qboolean CG_ServerMaterialName(int index, char *materialName, int maxLen);
void CG_ApplySplitScreenCompassScale(float *x, float *y, float *w, float *h);
const char *CG_GetUseString(void);
void CG_DrawMantleHint(const rectDef_t *rect, struct Font_s *font, float fontscale, int textStyle);
const char *CG_GetTranslatedLocationString(int iLocation);
const char *CG_GetKillerText(void);
const char *CG_GameTypeString(void);
int CG_KeyInterceptEvent(int key, qboolean down);
void CG_PulseLowHealthOverlay(float healthRatio);
void CG_ArchiveState(MemoryFile *memFile);
void CG_DrawHoldBreathHint(const rectDef_t *rect, struct Font_s *font, float fontscale, int textStyle);
float CG_FadeHudMenu(const dvar_t *fadeDvar, int displayStartTime, int duration);
Bool CG_CheckPlayerForLowAmmo(void);
Bool CG_CheckPlayerForLowClip(void);
int CG_GetHudWeaponIndex(void);
void CG_DrawPlayerCompassBack(const rectDef_t *rect, MaterialHandle material, vec_t *color);
void CG_OwnerDraw(float x, float y, float w, float h, int horzAlign, int vertAlign, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, struct Font_s *font, float scale, vec_t *color, MaterialHandle material, int textStyle);

void CG_UpdatePlayerDObj(centity_t *cent);
void CG_ResetPlayerEntity(centity_t *cent);
void CG_PlayerSprites(centity_t *cent);
void CG_Player(centity_t *cent);
void CG_Corpse(centity_t *cent);

void CG_Respawn(void);
void CG_DamageFeedback(int yawByte, int pitchByte, int damage);
void CG_TransitionPlayerState(playerState_t *ps, playerState_t *ops);

void CG_ClearSolidList(void);
int CG_PointContents(const vec_t *point, int passEntityNum, int contentmask);
void CG_PredictPlayerState(void);
void CG_ClipMoveToEntities(const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int skipNumber, int mask, int capsule, trace_t *tr);
void CG_TraceCapsule(trace_t *result, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int skipNumber, int mask);
void CG_BuildSolidList(void);

qboolean CG_ScoreboardDisplayed(void);
void CG_ScrollScoreboardUp(void);
void CG_ScrollScoreboardDown(void);
void CG_RegisterScoreboardGraphics(void);
int CalcBorderLines(sbpicinfo_t *backdropBorderLines);
void CG_DrawScoreboard_GetTeamColor(int team, vec_t *color);
float CG_DrawScoreboard_ScoresList(float alpha);
qboolean CG_DrawScoreboard(void);

void CG_ParseServerinfo(void);
void CG_ParseCodinfo(void);
void CG_CheckOpenWaitingScriptMenu(void);
void CG_CloseScriptMenu(void);
void CG_MenuShowNotify(int menuToShow);
void CG_ParseFog(void);
void CG_SetConfigValues(void);
void CG_MapRestart(qboolean savepersist);
void CG_ServerCommand(void);
void CG_ExecuteNewServerCommands(int latestSequence);

void CG_PerturbCamera(void);
qboolean CG_SaveShellShockDvars(const char *name);
qboolean CG_LoadShellShockDvars(const char *name);
qboolean CG_DrawShellShockSavedScreenBlend(const shellshock_parms_t *parms, int start, int duration);
void CG_SetShellShockParmsFromDvars(shellshock_parms_t *parms);
void CG_UpdateShellShock(const shellshock_parms_t *parms, int start, int duration);

void CG_SetNextSnap(snapshot_t *snap_param);
void CG_SetInitialSnapshot(snapshot_t *snap_param);
void CG_ProcessSnapshots(void);

void CG_FxRestart(void);
void CG_FxTest(void);
float CG_GetViewFov(void);
void CG_FxSetTestPosition(void);
void CG_InitView(void);
qboolean CG_DrawActiveFrame(int serverTime, DemoType demoType, CubemapShot cubemapShot, int cubemapSize, qboolean renderScreen);

int CG_WeaponDObjHandle(int weaponNum);
void CG_Weapons_SetToDefault(int weaponNum, DObjModel_s *dobjModels);
void CG_HoldBreathInit(void);
void CG_SetupWeaponDef(void);
void CG_SelectWeaponIndex(int weaponIndex);
void CG_EjectWeaponBrass(entityState_t *ent, int event);
void CG_FireWeapon(centity_t *cent, int event, int barrel);
void CG_RegisterWeapon(int weaponNum);
void CG_RegisterItemVisuals(int itemNum);
void CG_RegisterItems(void);
void CG_UpdateHandViewmodels(const char *handModel);
void CG_SpawnTracer(vec_t *pstart, vec_t *pend);
void CG_WhizbySound(vec_t *vStart, vec_t *vEnd);
void CG_BulletHitClientEvent(int sourceEntityNum, vec_t *position, int surfType, int event);
void CG_AddPlayerWeapon(GfxEntity *parent, weapProjExposion_t (*ps)[8], centity_t *cent, qboolean bDrawGun);
void CG_AddViewWeapon(weapProjExposion_t (*ps)[8]);
void CG_BulletHitEvent(int sourceEntityNum, vec_t *position, vec_t *normal, vec_t *reflected, int surfType, int event);
void CG_DrawTracer(vec_t *start, vec_t *finish);
void CG_WeaponSlot_f(qboolean next, qboolean ignoreEmpty);
qboolean CG_SelectFirstWeaponNotInSlot(qboolean bNext, qboolean bIgnoreEmpty);
void CG_CycleWeap(qboolean bNext, qboolean bIgnoreEmpty);
void CG_OutOfAmmoChange(void);
void CG_PrevWeapon_f(void);
void CG_NextWeapon_f(void);
void CG_WeaponRunXModelAnims(weapProjExposion_t (*ps)[8], weaponInfo_t *weapInfo);
void CG_UpdateViewWeaponAnim(weapProjExposion_t (*ps)[8]);

void CG_OffhandRegisterDvars(void);
void CG_DrawOffHandName(rectDef_s *rect, struct Font_s *font, float scale, vec_t *color, int textStyle, int weaponType);
void CG_PrepOffHand(entityState_t *ent, int event, int eventParam);
void CG_UseOffHand(centity_t *cent, int event, int eventParam);
void CG_SetEquippedOffHand(int offHandIndex);
void CG_SwitchOffHandCmd(void);
void CG_DrawOffHandIcon(rectDef_s *rect, float scale, vec_t *color, MaterialHandle material, int weaponType);
void CG_DrawOffHandHighlight(rectDef_s *rect, float scale, vec_t *color, MaterialHandle material, int weaponType);
void CG_DrawOffHandAmmo(rectDef_s *rect, struct Font_s *font, float scale, vec_t *color, int textStyle, int weaponType);
MaterialHandle CG_GetOffHandIcon(int weaponType, int *count);

#endif
