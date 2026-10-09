#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include <stdlib.h>
#include <string.h>
extern scr_const_t scr_const;

extern BuiltinMethod Player_GetMethod(const char **pName);
extern BuiltinMethod ScriptEnt_GetMethod(const char **pName);
extern BuiltinMethod HudElem_GetMethod(const char **pName);
extern void RegisterItem(int iItemIndex, qboolean bUpdateCS);
extern const gitem_t *G_FindItem(const char *pickupName);

extern struct scr_data_t g_scr_data;
extern unsigned int Scr_AllocString(const char *s, int user);
extern void Scr_AddString(const char *s);
extern void Scr_AddInt(int value);
extern void Scr_AddArray(void);
extern void Scr_MakeArray(void);
extern void Scr_AddArrayStringIndexed(unsigned int stringValue);
extern int Scr_GetInt(int argIndex);
extern void Scr_AddEntity(gentity_t *ent);
extern void Scr_AddUndefined(void);
extern void Scr_ObjectError(const char *error);
extern int SV_AddTestClient(void);
extern qboolean SV_MapExists(const char *name);
extern float Vec3Normalize(vec3_t v);
extern void SV_EnableArchivedSnapshot(int enable);
extern void Scr_Error(const char *msg);
extern const char *va(const char *fmt, ...);
extern const char *Scr_GetString(unsigned int index);
extern int Scr_GetType(unsigned int index);
extern int Scr_GetPointerType(unsigned int index);
extern gentity_t *Scr_GetEntity(unsigned int index);
extern const char *Dvar_GetVariantString(const char *dvarName);
extern dvar_t *Dvar_FindVar(const char *dvarName);
extern void Dvar_AddFlags(const dvar_t *dvar, int flags);
extern const dvar_t *Dvar_RegisterString_mac(const char *dvarName, const char *value, int flags);
extern int Dvar_IsValidName(const char *dvarName);
extern void Dvar_SetFromStringByName(const char *dvarName, const char *string);
extern void Scr_AddFloat(float value);
extern void Scr_AddBool(int value);
extern const char *Scr_GetDebugString(unsigned int index);
extern scr_anim_t Scr_GetAnim(unsigned int index, XAnimTree *tree);

static unsigned int GScr_GetPackedAnim(unsigned int index) {
    scr_anim_t anim = Scr_GetAnim(index, NULL);
    return ((unsigned int)anim.tree << 16) | anim.index;
}
extern void *Scr_GetAnims(unsigned int treeIndex);
extern void XAnimGetRelDelta(const void *anims, unsigned int animIndex, vec_t *rot, vec_t *trans, float time1, float time2);
extern Bool XAnimIsPrimitive(void *anims, unsigned int animIndex);
extern float XAnimGetLength(void *anims, unsigned int animIndex);
extern float RotationToYaw(const vec_t *rot);
extern void Scr_ParamError(unsigned int index, const char *msg);
extern unsigned int Scr_GetConstString(unsigned int index);
extern unsigned int Scr_GetConstLowercaseString(unsigned int index);
extern int XAnimNotetrackExists(void *anims, unsigned int animIndex, unsigned int notetrack);
extern int Com_FindSoundAlias(const char *name);
extern int G_GetWeaponIndexForName(const char *name);
extern int G_IndexForMeansOfDeath(const char *pszMOD);
extern void *BG_GetWeaponDef(int weaponIndex);
extern scr_string_t G_GetHitLocationString(hitLocation_t hitLoc);
extern int I_stricmp(const char *s1, const char *s2);
extern char *strlwr(char *s);
extern void I_strncat(char *dest, int size, const char *src);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern int ___tolower(int c);
extern int ___maskrune(int c, unsigned long flags);
extern char I_CleanChar(char character);
extern void Com_Printf(const char *fmt, ...);
extern void Com_DPrintf(const char *fmt, ...);
extern void Com_Error(int code, const char *fmt, ...);
extern const char *Com_SurfaceTypeToName(int surfaceType);
extern int stricmp(const char *s1, const char *s2);
extern int FS_FOpenFileByMode(const char *qpath, fileHandle_t *f, int mode);
extern int FS_GetFileList(const char *path, const char *extension, int behavior, char *listbuf, int bufsize);
extern void FS_FCloseFile(fileHandle_t f);
extern int FS_Read(void *buffer, int len, fileHandle_t f);
extern int FS_Write(const void *buffer, int len, fileHandle_t f);
extern void *Z_MallocInternal(int size);
extern void Z_FreeInternal(void *ptr);
extern void Com_BeginParseSession(const char *filename);
extern void Com_EndParseSession(void);
extern void Com_SetCSV(qboolean csv);
extern void Com_ParseSetMark(const char **text, com_parse_mark_t *mark);
extern void Com_ParseReturnToMark(const char **text, com_parse_mark_t *mark);
extern const char *Com_Parse(const char **data_p);
extern const char *Com_ParseOnLine(const char **data_p);
extern void Com_SkipRestOfLine(const char **data);
extern int Com_GetArgCountOnLine(const char **data_p);
extern void G_LogPrintf(const char *fmt, ...);
extern float Scr_GetFloat(unsigned int index);
extern float flrand(float min, float max);
extern int irand(int min, int max);
extern const char *Info_ValueForKey(const char *s, const char *key);
extern void Info_SetValueForKey(char *s, const char *key, const char *value);
extern double sin(double x);
extern double cos(double x);
extern float floorf(float x);
extern unsigned int Scr_ExecThread(unsigned int handle, int paramCount);
extern void Scr_FreeThread(unsigned int threadId);
extern scr_thread_t Scr_ExecEntThread(gentity_t *ent, scr_func_t handle, unsigned int paramcount);
extern void SV_GameSendServerCommand(int clientnum, int type, const char *text);
extern void Scr_Notify(gentity_t *ent, unsigned short stringValue, unsigned int paramcount);
extern void *SV_XModelGet(const char *name);
extern int XModelNumBones(void *model);
extern int XModelBoneNames(void *model);
extern void G_AddDebugString(const vec_t *xyz, const vec_t *color, float scale, const char *pszText);
extern void CL_AddDebugLine(const vec_t *start, const vec_t *end, const vec_t *color, int depthTest, int duration, int fromServer);
extern int G_ModelIndex(const char *name);
extern gentity_t *G_Spawn(void);
extern void Scr_SetString(scr_string_t *to, unsigned int from);
extern qboolean G_CallSpawnEntity(gentity_t *ent);
extern void G_SpawnTurret(gentity_t *self, const char *weaponinfoname);
extern void Scr_RemoveClassMap(int classnum);
extern float atanf(float x);
extern float acosf(float x);
extern float asinf(float x);
extern float sinf(float x);
extern float cosf(float x);
extern int Scr_GetTypeName(int index);
extern void Scr_GetVector(unsigned int index, float *out);
extern unsigned char G_SetOrigin(gentity_t *ent, const vec_t *origin);
extern unsigned char G_SetAngle(gentity_t *ent, const vec_t *angles);
extern void SV_LinkEntity(gentity_t *ent);
extern float Vec3Distance(float *a, float *b);
extern float Vec3DistanceSq(float *a, float *b);
extern void vectoangles(float *vec, float *angles);
extern void Scr_AddVector(float *vec);
extern void Scr_AddConstString(unsigned int value);
extern void AngleVectors(float *angles, float *forward, float *right, float *up);
extern void AnglesToAxis(const vec_t *angles, vec3_t *axis);
extern void MatrixTransformVector(const vec_t *in1, const vec3_t *in2, vec_t *out);
extern int G_ShellShockIndex(const char *name);
extern int G_ShaderIndex(const char *name);
extern int G_LocalizedStringIndex(const char *name);
extern const char *Scr_GetIString(unsigned int index);
extern unsigned int Scr_GetNumParam(void);
extern void SV_GetConfigstring(int index, char *buffer, int bufferSize);
extern void SV_SetConfigstring(int index, const char *val);
extern void Cbuf_ExecuteText(int exec_when, const char *text);
extern void SV_MatchEnd(void);
extern void ExitLevel(void);
extern float sqrtf(float x);
extern void G_setfog(const char *fogstring);
extern int G_EffectIndex(const char *name);
extern int G_FindConfigstringIndex(const char *name, int start, int max, qboolean create, const char *errormsg);
extern const char *SL_ConvertToString(unsigned int stringValue);
extern unsigned int G_EntDetachAll(gentity_t *ent);
extern unsigned char G_EntUnlink(gentity_t *ent);
extern qboolean G_EntDetach(gentity_t *ent, const char *modelName, unsigned int tagName);
extern qboolean G_EntAttach(gentity_t *ent, const char *modelName, unsigned int tagName, qboolean ignoreCollision);
extern qboolean G_EntLinkTo(gentity_t *ent, gentity_t *parent, unsigned int tagName);
extern qboolean G_EntLinkToWithOffset(gentity_t *ent, gentity_t *parent, unsigned int tagName, const vec_t *originOffset, const vec_t *anglesOffset);
extern const char *G_ModelName(int index);
extern SoundAlias G_SoundAliasIndex(const char *name);
extern gentity_t *G_TempEntity(const vec_t *origin, int event);
extern unsigned char DirToByte(vec_t *dir);
extern void Vec3Cross(const vec_t *v0, const vec_t *v1, vec_t *cross);
extern void AxisToAngles(const vec_t *axis, vec_t *angles);
extern int SV_DObjGetBoneIndex(gentity_t *ent, unsigned int boneName);
extern void SV_DObjDumpInfo(gentity_t *ent);
extern qboolean SV_DObjExists(gentity_t *ent);
extern qboolean SV_EntityContact(const vec_t *mins, const vec_t *maxs, const gentity_t *gEnt);
extern void ExpandBoundsToWidth(vec_t *mins, vec_t *maxs);
extern unsigned char G_AddEvent(gentity_t *ent, int event, int eventParm);
extern unsigned char G_FreeEntity(gentity_t *ent);
extern unsigned char G_SetModel(gentity_t *ent, const char *modelName);
extern void G_DObjUpdate(gentity_t *ent);
extern qboolean G_LocationalTracePassed(const vec_t *start, const vec_t *end, int passEntityNum, int contentmask);
extern void G_LocationalTrace(trace_t *results, const vec_t *start, const vec_t *end, int passEntityNum, int contentmask, unsigned char *priorityMap);
extern int G_SightTrace(int *hitNum, const vec_t *start, const vec_t *end, int passEntityNum, int contentmask);
extern float SV_FX_GetVisibility(const vec_t *start, const vec_t *end);
extern void G_TraceCapsule(trace_t *results, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentmask);
extern qboolean G_RadiusDamage(const vec_t *origin, gentity_t *inflictor, gentity_t *attacker, float fInnerDamage, float fOuterDamage, float radius, gentity_t *ignore, int mod);
extern int CM_AreaEntities(const vec_t *mins, const vec_t *maxs, int *entityList, int maxcount, int contentmask);
extern int BG_AnimScriptEvent(playerState_t *ps, int event, int isContinue, int force);
extern int BG_WeaponAmmo(const playerState_t *ps, int weapon);
extern void ClientUserinfoChanged(int clientNum);
extern vec3_t playerMins;
extern vec3_t playerMaxs;
extern BuiltinFunctionDef functions[145];

extern BuiltinMethodDef methods[];

#define g_entities ((gentity_t *)imp_g_entities)
extern level_locals_t level;
extern bgs_t level_bgs;
extern bgs_t *bgs;
extern const dvar_t *g_NoScriptSpam;
extern const dvar_t *g_maxclients;
extern gitem_t bg_itemlist[];
extern const char *modNames[15];
extern entityHandler_t entityHandlers[20];

#define FUNCTIONS_COUNT ((int)(sizeof(functions) / sizeof(functions[0])))
#define BUILTIN_METHODS_COUNT 0x3b

#define SCRIPT_IO_HANDLE_OFFSET 0x3608
#define SCRIPT_IO_BUFFER_OFFSET 0x360c
#define SCRIPT_IO_MARK_OFFSET 0x3610

static inline __attribute__((always_inline)) fileHandle_t *GScr_ScriptIOFileHandle(int filenum)
{
    return (fileHandle_t *)((byte *)imp_level + SCRIPT_IO_HANDLE_OFFSET + filenum * sizeof(fileHandle_t));
}

static inline __attribute__((always_inline)) char **GScr_ScriptIOFileBuffer(int filenum)
{
    return (char **)((byte *)imp_level + SCRIPT_IO_BUFFER_OFFSET + filenum * sizeof(char *));
}

static inline __attribute__((always_inline)) com_parse_mark_t *GScr_ScriptIOLineMark(int filenum)
{
    return (com_parse_mark_t *)((byte *)imp_level + SCRIPT_IO_MARK_OFFSET + filenum * sizeof(com_parse_mark_t));
}

static const char *GScr_EffectName(int fxId, char *buffer, int bufferSize)
{
    if (fxId)
        SV_GetConfigstring(fxId + 0x34e, buffer, bufferSize);
    else
        I_strncpyz(buffer, "not successfully loaded", bufferSize);

    return buffer;
}

static void GScr_AddOptionalVector(const vec_t *value)
{
    if (value)
        Scr_AddVector((float *)value);
    else
        Scr_AddUndefined();
}

static void GScr_AddOptionalEntity(gentity_t *ent)
{
    if (ent)
        Scr_AddEntity(ent);
    else
        Scr_AddUndefined();
}

static void GScr_AddMeansOfDeath(int meansOfDeath)
{
    if ((unsigned int)meansOfDeath <= 0xe)
        Scr_AddString(modNames[meansOfDeath]);
    else
        Scr_AddString("badMOD");
}

static void GScr_AddWeaponName(int weaponIndex)
{
    WeaponDef *weaponDef = (WeaponDef *)BG_GetWeaponDef(weaponIndex);

    Scr_AddString(weaponDef->szInternalName);
}

static inline __attribute__((always_inline)) void G_ResetObjective(objective_t *obj)
{
    obj->state = OBJST_EMPTY;
    obj->origin[0] = 0.0f;
    obj->origin[1] = 0.0f;
    obj->origin[2] = 0.0f;
    obj->entNum = 0x3ff;
    obj->teamNum = 0;
    obj->icon = 0;
}

static inline __attribute__((always_inline)) void GScr_ClearObjectiveEntity(objective_t *obj)
{
    if (obj->entNum != 0x3ff) {
        gentity_t *ent = &g_entities[obj->entNum];

        if (ent->r.inuse)
            ent->r.svFlags &= ~0x10;

        obj->entNum = 0x3ff;
    }
}

static inline __attribute__((always_inline)) objectiveState_t GScr_ObjectiveStateFromConst(unsigned int stateString)
{
    scr_const_t *scr_const = (scr_const_t *)imp_scr_const;

    if (stateString == scr_const->empty)
        return OBJST_EMPTY;
    if (stateString == scr_const->invisible)
        return OBJST_INVISIBLE;
    if (stateString == scr_const->current)
        return OBJST_CURRENT;

    return OBJST_NUMSTATES;
}

static inline __attribute__((always_inline)) gentity_t *GScr_EntityFromEntRef(scr_entref_t entref)
{
    if (entref.classnum) {
        Scr_ObjectError("not an entity");
        return NULL;
    }

    return &g_entities[entref.entnum];
}

static inline __attribute__((always_inline)) qboolean GScr_IsUseTrigger(const gentity_t *ent)
{
    scr_const_t *scr_const = (scr_const_t *)imp_scr_const;

    return ent->classname == scr_const->trigger_use ||
           ent->classname == scr_const->trigger_use_touch;
}

static inline __attribute__((always_inline)) void GScr_UseTriggerError(const char *format)
{
    scr_const_t *scr_const = (scr_const_t *)imp_scr_const;
    const char *triggerUseTouch = SL_ConvertToString(scr_const->trigger_use_touch);
    const char *triggerUse = SL_ConvertToString(scr_const->trigger_use);

    Scr_Error(va(format, triggerUse, triggerUseTouch));
}

static inline __attribute__((always_inline)) turretInfo_t *GScr_GetTurretInfo(gentity_t *ent)
{
    turretInfo_t *pTurretInfo = ent->pTurretInfo;
    if (!pTurretInfo)
        Scr_Error("entity is not a turret");

    return pTurretInfo;
}

static qboolean GScr_MessageNeedsEscape(unsigned char c)
{
    return (unsigned char)(c - 0x14) <= 2;
}

static qboolean GScr_MessageIsAlpha(int c)
{
    return ___maskrune(c, 0x100) != 0;
}

static qboolean GScr_MessageIsAlphaNumeric(int c)
{
    return ___maskrune(c, 0x500) != 0;
}

static void GScr_CheckMessageLength(int parmIndex, const char *errorContext, int stringLimit, int stringLen, int tokenLen)
{
    if (stringLen + tokenLen + 1 >= stringLimit) {
        Scr_ParamError(parmIndex, va("%s is too long. Max length is %i\n", errorContext, stringLimit));
    }
}

static void GScr_AppendMessageToken(char *string, int *stringLen, const char *token, int tokenLen)
{
    int i;

    for (i = 0; i < tokenLen; ++i) {
        unsigned char c = (unsigned char)token[i];
        string[*stringLen + i] = GScr_MessageNeedsEscape(c) ? '.' : (char)c;
    }

    *stringLen += tokenLen;
}

static inline __attribute__((always_inline)) void ScrCmd_PlaySoundEvent(gentity_t *ent, int event)
{
    SoundAlias soundIndex = G_SoundAliasIndex(Scr_GetString(0));
    gentity_t *tempEnt = G_TempEntity(ent->r.currentOrigin, event);

    tempEnt->r.svFlags |= 8;
    tempEnt->s.eventParm = soundIndex;
}

static inline __attribute__((always_inline)) void GScr_UpdateWinnerConfig(int winner)
{
    char cs[1024];
    const char *winnerString;

    SV_GetConfigstring(0x16, cs, sizeof(cs));
    winnerString = va("%i", winner);
    if (I_stricmp(Info_ValueForKey(cs, "winner"), winnerString)) {
        Info_SetValueForKey(cs, "winner", winnerString);
        SV_SetConfigstring(0x16, cs);
    }
}

unsigned int GScr_AllocString(const char *s);
void Scr_LoadLevel(void);
void GScr_FreeScripts(void);
void ScrCmd_GetClanId(struct scr_entref_t entref);
void ScrCmd_GetClanName(scr_entref_t entref);
void ScrCmd_GetClanMotto(scr_entref_t entref);
void ScrCmd_GetClanDescription(scr_entref_t entref);
void ScrCmd_GetClanURL(scr_entref_t entref);
void print(void);
void assertCmd(void);
void assertexCmd(void);
void assertmsgCmd(void);
void GScr_IsDefined(void);
void GScr_IsString(void);
void GScr_IsAlive(void);
void GScr_GetDvar(void);
void GScr_GetDvarInt(void);
void GScr_GetDvarFloat(void);
void GScr_GetTime(void);
void Scr_GetEntByNum(void);
void Scr_GetWeaponModel(void);
void GScr_GetAnimLength(void);
void GScr_AnimHasNotetrack(void);
void GScr_PrecacheTurret(void);
void ScrCmd_SoundExists(void);
void ScrCmd_PlayRumble(scr_entref_t entref);
void ScrCmd_PlayLoopRumble(scr_entref_t entref);
void ScrCmd_StopRumble(scr_entref_t entref);
qboolean G_GetHintStringIndex(int *piIndex, const char *pszString);
unsigned int __attribute_regparm__(2) SetObjectiveIcon(objective_t *obj, int paramNum);
void Scr_Objective_Icon(void);
void Scr_Objective_OnEntity(void);
void Scr_Objective_Current(void);
void GScr_Objective_Team(void);
void GScr_LogPrint(void);
void GScr_WorldEntNumber(void);
void GScr_Obituary(void);
void GScr_getStartTime(void);
void GScr_PrecacheMenu(void);
int GScr_GetScriptMenuIndex(const char *pszMenu);
void GScr_PrecacheStatusIcon(void);
int GScr_GetStatusIconIndex(const char *pszIcon);
void GScr_PrecacheHeadIcon(void);
int GScr_GetHeadIconIndex(const char *pszIcon);
void Scr_BulletTracePassed(void);
void Scr_SightTracePassed(void);
void Scr_PhysicsTrace(void);
void Scr_RandomInt(void);
void Scr_RandomFloat(void);
void Scr_RandomIntRange(void);
void Scr_RandomFloatRange(void);
void GScr_sin(void);
void GScr_cos(void);
void GScr_tan(void);
void GScr_asin(void);
void GScr_acos(void);
void GScr_atan(void);
void GScr_CastInt(void);
void Scr_Distance(void);
void Scr_DistanceSquared(void);
void Scr_LengthSquared(void);
void Scr_Closer(void);
void Scr_VectorDot(void);
void Scr_VectorToAngles(void);
void Scr_AnglesToUp(void);
void Scr_AnglesToRight(void);
void Scr_AnglesToForward(void);
void Scr_IsSubStr(void);
void Scr_GetSubStr(void);
void Scr_ToLower(void);
void Scr_StrTok(void);
void Scr_MusicPlay(void);
void Scr_SoundFade(void);
void Scr_PrecacheModel(void);
void Scr_PrecacheShellShock(void);
void Scr_PrecacheItem(void);
void Scr_PrecacheShader(void);
void Scr_PrecacheString(void);
void Scr_PrecacheRumble(void);
void GScr_RadiusDamage(void);
void GScr_SetPlayerIgnoreRadiusDamage(void);
void GScr_GetMoveDelta(void);
void GScr_GetAngleDelta(void);
void GScr_GetNorthYaw(void);
void Scr_LoadFX(void);
void Scr_PlayFXOnTag(void);
void Scr_SetFog(const char *cmd, float fNear, float fFar, float fDensity, float fRed, float fGreen, float fBlue, float fTime);
void Scr_SetLinearFog(void);
void Scr_SetExponentialFog(void);
void GScr_IsPlayer(void);
void GScr_IsPlayerNumber(void);
void GScr_SetWinningPlayer(void);
void GScr_SetWinningTeam(void);
void GScr_GetTeamScore(void);
void GScr_SetTeamScore(void);
void GScr_SetClientNameMode(void);
void GScr_UpdateClientNames(void);
void GScr_GetTeamPlayersAlive(void);
void GScr_GetNumParts(void);
void GScr_GetPartName(void);
void GScr_Earthquake(void);
void GScr_MapRestart(void);
void GScr_LoadMap(void);
void GScr_ExitLevel(void);
void GScr_AddTestClient(void);
void GScr_AllClientsPrint(void);
void GScr_MapExists(void);
void GScr_SetVoteString(void);
void GScr_SetVoteTime(void);
void GScr_SetVoteYesCount(void);
void GScr_SetVoteNoCount(void);
void GScr_KickPlayer(void);
void GScr_BanPlayer(void);
void GScr_ClientPrint(void);
void GScr_OpenFile(void);
void GScr_CloseFile(void);
void GScr_FPrintln(void);
void GScr_FReadLn(void);
void GScr_FGetArg(void);
void GScr_SetArchive(void);
void Scr_PlayRumbleOnPos(void);
void Scr_PlayLoopRumbleOnPos(void);
void Scr_StopAllRumbles(void);
void Scr_IsSplitscreen(void);
void GScr_MatchEnd(void);
void GScr_SetPlayerTeamRank(void);
void GScr_SendXboxLiveRanks(void);
void GScr_EndXboxLiveLobby(void);
void GScr_ReleaseClaimedTrigger(scr_entref_t entref);
BuiltinFunction Scr_GetFunction(const char **pName, int *type);
BuiltinMethod Scr_GetMethod(const char **pName, int *type);
void Scr_SetOrigin(gentity_t *ent, int offset);
void Scr_SetAngles(gentity_t *ent, int offset);
void Scr_SetHealth(gentity_t *ent, int offset);
void GScr_AddEntity(gentity_t *pEnt);
void Scr_ParseGameTypeList(void);
const char *Scr_GetGameTypeNameForScript(const char *pszGameTypeScript);
void Scr_LoadGameType(void);
void Scr_StartupGameType(void);
void Scr_PlayerConnect(gentity_t *self);
void Scr_PlayerDisconnect(gentity_t *self);
void Scr_VoteCalled(gentity_t *self, char *command, char *param1, char *param2);
void Scr_PlayerVote(gentity_t *self, char *option);
void GScr_print3d(void);
void GScr_line(void);
void GScr_Spawn(void);
void GScr_SpawnTurret(void);
void Scr_VectorNormalize(void);
void Scr_GrenadeExplosionEffect(void);
void Scr_Objective_Position(void);
void GScr_positionWouldTelefrag(void);
void GScr_GetBrushModelCenter(void);
void Scr_Objective_Delete(void);
void G_InitObjectives(void);
void Scr_BulletTrace(void);
void Scr_Objective_Add(void);
void Scr_Objective_State(void);
void Scr_MusicStop(void);
void Scr_AmbientPlay(void);
void Scr_AmbientStop(void);
void Scr_PlayLoopedFX(void);
unsigned int Scr_ConstructMessageString(int firstParmIndex, int lastParmIndex, const char *errorContext, char *string, int stringLimit);
void GScr_MakeDvarServerInfo(void);
void GScr_ClientAnnouncement(void);
void GScr_Announcement(void);
void GScr_SetDvar(void);
void Scr_MakeGameMessage(int iClientNum, const char *pszCmd);
void GScr_IsValidGameType(void);
qboolean Scr_IsValidGameType(const char *pszGameType);
void Scr_Length(void);
void println(void);
void Scr_PlayerDamage(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int dflags, int meansOfDeath, int iWeapon, const vec_t *vPoint, const vec_t *vDir, const hitLocation_t hitLoc, int timeOffset);
void Scr_PlayerKilled(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath, int iWeapon, const vec_t *vDir, const hitLocation_t hitLoc, int psTimeOffset, int deathAnimDuration);
void ScrCmd_Show(scr_entref_t entref);
void ScrCmd_Hide(scr_entref_t entref);
void ScrCmd_ShowToPlayer(scr_entref_t entref);
void GScr_EnableGrenadeTouchDamage(scr_entref_t entref);
void GScr_DisableGrenadeTouchDamage(scr_entref_t entref);
void GScr_EnableGrenadeBounce(scr_entref_t entref);
void GScr_DisableGrenadeBounce(scr_entref_t entref);
void GScr_EnableAimAssist(scr_entref_t entref);
void GScr_DisableAimAssist(scr_entref_t entref);
void ScrCmd_attach(scr_entref_t entref);
void ScrCmd_detach(scr_entref_t entref);
void ScrCmd_detachAll(scr_entref_t entref);
void ScrCmd_GetAttachSize(scr_entref_t entref);
void ScrCmd_GetAttachModelName(scr_entref_t entref);
void ScrCmd_GetAttachTagName(scr_entref_t entref);
void ScrCmd_GetAttachIgnoreCollision(scr_entref_t entref);
void ScrCmd_LinkTo(scr_entref_t entref);
void ScrCmd_Unlink(scr_entref_t entref);
void ScrCmd_EnableLinkTo(scr_entref_t entref);
void ScrCmd_GetOrigin(scr_entref_t entref);
void ScrCmd_GetEye(scr_entref_t entref);
void ScrCmd_UseBy(scr_entref_t entref);
void Scr_SetStableMissile(scr_entref_t entref);
void ScrCmd_IsTouching(scr_entref_t entref);
void ScrCmd_PlaySound(scr_entref_t entref);
void ScrCmd_PlaySoundAsMaster(scr_entref_t entref);
void ScrCmd_PlayLoopSound(scr_entref_t entref);
void ScrCmd_StopLoopSound(scr_entref_t entref);
void ScrCmd_Delete(scr_entref_t entref);
void ScrCmd_SetModel(scr_entref_t entref);
void ScrCmd_GetNormalHealth(scr_entref_t entref);
void ScrCmd_SetNormalHealth(scr_entref_t entref);
void ScrCmd_SetContents(scr_entref_t entref);
void GScr_SetCursorHint(scr_entref_t entref);
void GScr_SetHintString(scr_entref_t entref);
static __attribute_regparm__(2) gentity_t *GetPlayerEntity(scr_entref_t entref);
void GScr_ViewKick(scr_entref_t entref);
void GScr_StopShellShock(scr_entref_t entref);
void GScr_ShellShock(scr_entref_t entref);
void GScr_GetAmmoCount(scr_entref_t entref);
void GScr_LocalToWorldCoords(scr_entref_t entref);
void GScr_SetRightArc(scr_entref_t entref);
void GScr_SetLeftArc(scr_entref_t entref);
void GScr_SetTopArc(scr_entref_t entref);
void GScr_SetBottomArc(scr_entref_t entref);
void GScr_GetEntityNumber(scr_entref_t entref);
void GScr_PlaceSpawnPoint(scr_entref_t entref);
void GScr_UpdateScores(scr_entref_t entref);
void GScr_SetTeamForTrigger(scr_entref_t entref);
void GScr_ClientClaimTrigger(scr_entref_t entref);
void GScr_ClientReleaseTrigger(scr_entref_t entref);
void Scr_PlayFX(void);
void iprintln(void);
void iprintlnbold(void);
unsigned int GScr_LoadGameTypeScript(void);
void GScr_LoadScripts(int inst);

unsigned int GScr_AllocString(const char *s)
{
    return Scr_AllocString(s, 1);
}

void Scr_LoadLevel(void)
{
    unsigned int handle = *(unsigned int *)&g_scr_data;
    if (handle) {
        unsigned short threadId = Scr_ExecThread(handle, 0);
        Scr_FreeThread(threadId);
    }
    return;
}

void GScr_FreeScripts(void)
{
    int classnum;
    for (classnum = 0; classnum < 4; classnum++) {
        Scr_RemoveClassMap(classnum);
    }
    return;
}

void ScrCmd_GetClanId(struct scr_entref_t entref)
{
    { (void)(Scr_AddString("0")); return; }
}

void ScrCmd_GetClanName(scr_entref_t entref)
{
    { (void)(Scr_AddString("")); return; }
}

void ScrCmd_GetClanMotto(scr_entref_t entref)
{
    { (void)(Scr_AddString("")); return; }
}

void ScrCmd_GetClanDescription(scr_entref_t entref)
{
    { (void)(Scr_AddString("")); return; }
}

void ScrCmd_GetClanURL(scr_entref_t entref)
{
    { (void)(Scr_AddString("")); return; }
}

void print(void)
{
    int num;
    int i;

    if ((*(const dvar_t **)imp_g_NoScriptSpam)->current.enabled)
        return;

    num = Scr_GetNumParam();
    if (num <= 0)
        return;

    for (i = 0; i < num; i++) {
        Com_Printf("%s", Scr_GetDebugString(i));
    }
    return;
}

void assertCmd(void)
{
    int v = Scr_GetInt(0);
    if (!v)
        Scr_Error("assert fail");
}

void assertexCmd(void)
{
    if (!Scr_GetInt(0))
        Scr_Error(va("assert fail: %s", Scr_GetString(1)));
}

void assertmsgCmd(void)
{
    Scr_Error(va("assert fail: %s", Scr_GetString(0)));
}

void GScr_IsDefined(void)
{
    int type = Scr_GetType(0);
    int result;

    if (type == 1) {
        int ptype = Scr_GetPointerType(0);
        if (ptype > 0x16 || ptype == 0x14) {
            result = 0;
        } else {
            result = 1;
        }
    } else {
        result = (type != 0);
    }
    Scr_AddInt(result);
    return;
}

void GScr_IsString(void)
{
    { (void)(Scr_AddInt(Scr_GetType(0) == 2)); return; }
}

void GScr_IsAlive(void)
{
    if (Scr_GetType(0) == 1 && Scr_GetPointerType(0) == 0x15) {
        gentity_t *ent = Scr_GetEntity(0);
        int health = ent->health;
        if (health > 0) {
            Scr_AddInt(1);
            return;
        }
    }
    Scr_AddInt(0);
    return;
}

void GScr_GetDvar(void)
{
    const char *name = Scr_GetString(0);
    const char *val = Dvar_GetVariantString(name);
    Scr_AddString(val);
}

void GScr_GetDvarInt(void)
{
    { (void)(Scr_AddInt(atoi(Dvar_GetVariantString(Scr_GetString(0))))); return; }
}

void GScr_GetDvarFloat(void)
{
    { (void)(Scr_AddFloat((float)atof(Dvar_GetVariantString(Scr_GetString(0))))); return; }
}

void GScr_GetTime(void)
{
    { (void)(Scr_AddInt(level.time)); return; }
}

void Scr_GetEntByNum(void)
{
    unsigned int num = Scr_GetInt(0);
    gentity_t *ent;

    if (num > 0x3ff)
        return;
    ent = (gentity_t *)((byte *)imp_g_entities + num * sizeof(gentity_s)); /* was 560 = x86 size */
    if (ent->r.inuse)
        Scr_AddEntity(ent);
}

void Scr_GetWeaponModel(void)
{
    const char *pszWeaponName = Scr_GetString(0);
    int weaponIndex = G_GetWeaponIndexForName(pszWeaponName);

    if (weaponIndex) {
        Scr_AddString(*(const char **)((byte *)BG_GetWeaponDef(weaponIndex) + 0x1b4));
    } else {
        if (pszWeaponName[0] && I_stricmp(pszWeaponName, "none") != 0) {
            Com_Printf(va("unknown weapon '%s' in getWeaponModel\n", pszWeaponName));
        }
        Scr_AddString("");
    }
    return;
}

void GScr_GetAnimLength(void)
{
    unsigned int anim = GScr_GetPackedAnim(0);
    void *anims = Scr_GetAnims(anim >> 16);
    unsigned short animIndex = anim;

    if (!XAnimIsPrimitive(anims, animIndex))
        Scr_ParamError(0, (const char *)"non-primitive animation has no concept of length");
    Scr_AddFloat(XAnimGetLength(anims, animIndex));
}

void GScr_AnimHasNotetrack(void)
{
    unsigned int anim = GScr_GetPackedAnim(0);
    unsigned int treeIndex = anim >> 16;
    unsigned int animIndex = anim & 0xffff;
    unsigned int notetrack = Scr_GetConstString(1);
    void *anims = Scr_GetAnims(treeIndex);

    Scr_AddBool(XAnimNotetrackExists(anims, animIndex, notetrack));
}

void GScr_PrecacheTurret(void)
{
    int initializing = level.initializing;
    if (!initializing)
        Scr_Error("precacheTurret must be called before any wait statements in the level script\n");
    G_GetWeaponIndexForName(Scr_GetString(0));
}

void ScrCmd_SoundExists(void)
{
    { (void)(Scr_AddBool(Com_FindSoundAlias(Scr_GetString(0)) != 0)); return; }
}

void ScrCmd_PlayRumble(scr_entref_t entref) {}

void ScrCmd_PlayLoopRumble(scr_entref_t entref) {}

void ScrCmd_StopRumble(scr_entref_t entref) {}

qboolean G_GetHintStringIndex(int *piIndex, const char *pszString)
{
    int i;
    char szConfigString[0x400];

    for (i = 0; i < 0x20; i++) {
        int csIndex = 0x4fe + i;
        SV_GetConfigstring(csIndex, szConfigString, 0x400);
        if (szConfigString[0] == '\0') {

            SV_SetConfigstring(csIndex, pszString);
            *piIndex = i;
            return 1;
        }
        if (strcmp(pszString, szConfigString) == 0) {

            *piIndex = i;
            return 1;
        }
    }

    *piIndex = -1;
    return 0;
}

static unsigned int SetObjectiveIcon_impl(objective_t *obj, int paramNum)
{
    const char *shaderName;
    int i;
    unsigned char ch;

    shaderName = (const char *)Scr_GetString(paramNum);
    ch = (unsigned char)shaderName[0];
    if (ch != '\0') {
        for (i = 0; shaderName[i] != '\0'; i++) {
            ch = (unsigned char)shaderName[i];

            if ((unsigned char)(ch - 0x20) > 0x5e) {
                Scr_ParamError(3, va("Illegal character '%c'(ascii %i) in objective icon name: %s\n", (int)(signed char)ch, (int)(unsigned char)ch, shaderName));
            }
        }
        if (i > 0x3f) {
            Scr_ParamError(3, va("Objective icon name is too long (> %i): %s\n", 0x3f, shaderName));
        }
    }
    obj->icon = G_ShaderIndex(shaderName);
    return obj->icon;
}

unsigned int __attribute_regparm__(2) SetObjectiveIcon(objective_t *obj, int paramNum)
{
    return SetObjectiveIcon_impl(obj, paramNum);
}

void Scr_Objective_Icon(void)
{
    int objIndex;

    objIndex = Scr_GetInt(0);
    if ((unsigned int)objIndex > 0xf)
        Scr_ParamError(0, va("index %i is an illegal objective index. Valid indexes are 0 to %i\n", objIndex, 0xf));

    { (void)(SetObjectiveIcon(&level.objectives[objIndex], 1)); return; }
}

void Scr_Objective_OnEntity(void)
{
    int objIndex;
    objective_t *obj;
    int oldEntityNum;
    gentity_t *oldEnt;
    gentity_t *newEnt;

    objIndex = Scr_GetInt(0);
    if ((unsigned int)objIndex > 0xf) {
        Scr_ParamError(0, va("index %i is an illegal objective index. Valid indexes are 0 to %i\n", objIndex, 0xf));
    }

    obj = &level.objectives[objIndex];

    oldEntityNum = obj->entNum;
    if (oldEntityNum != 0x3ff) {
        oldEnt = &g_entities[oldEntityNum];
        if (oldEnt->r.inuse) {

            oldEnt->r.svFlags &= ~0x10;
        }
        obj->entNum = 0x3ff;
    }

    newEnt = Scr_GetEntity(1);

    newEnt->r.svFlags |= 0x10;

    obj->entNum = newEnt->s.number;

    return;
}

void Scr_Objective_Current(void)
{
    int makeCurrent[16] = { 0 };
    int numParam;
    int i;

    numParam = Scr_GetNumParam();
    for (i = 0; i < numParam; i++) {
        int objNum = Scr_GetInt(i);
        if ((unsigned int)objNum > 0xf)
            Scr_ParamError(i, va("index %i is an illegal objective index. Valid indexes are 0 to %i\n", objNum, 0xf));
        makeCurrent[objNum] = 1;
    }

    for (i = 0; i < 16; i++) {
        if (makeCurrent[i])
            level.objectives[i].state = OBJST_CURRENT;
        else if (level.objectives[i].state == OBJST_CURRENT)
            level.objectives[i].state = OBJST_ACTIVE;
    }

    return;
}

void GScr_Objective_Team(void)
{
    int objIndex = Scr_GetInt(0);
    objective_t *obj;
    unsigned int team;
    scr_const_t *scr_const = (scr_const_t *)imp_scr_const;

    if ((unsigned int)objIndex > 0xf)
        Scr_ParamError(0, va("index %i is an illegal objective index. Valid indexes are 0 to %i\n", objIndex, 0xf));

    obj = &level.objectives[objIndex];
    team = Scr_GetConstString(1);

    if (team == scr_const->allies)
        obj->teamNum = 2;
    else if (team == scr_const->axis)
        obj->teamNum = 1;
    else if (team == scr_const->none)
        obj->teamNum = 0;
    else
        Scr_ParamError(1, va("Illegal team string '%s'. Must be allies, axis, or none.", SL_ConvertToString(team)));

    return;
}

void GScr_LogPrint(void)
{
    char string[1024];
    int iNumParms;
    int iStringLen;
    int i;

    string[0] = '\0';
    iNumParms = Scr_GetNumParam();

    iStringLen = 0;
    for (i = 0; i < iNumParms; ++i) {
        const char *argString = Scr_GetString(i);

        iStringLen += strlen(argString);
        if (iStringLen > 0x3ff)
            break;

        I_strncat(string, sizeof(string), argString);
    }

    G_LogPrintf(string);
    return;
}

void GScr_WorldEntNumber(void)
{
    { (void)(Scr_AddInt(0x3fe)); return; }
}

void GScr_Obituary(void)
{
    int iWeaponNum;
    int iMODNum;
    gentity_t *pOtherEnt;
    gentity_t *pEnt;

    iWeaponNum = G_GetWeaponIndexForName(Scr_GetString(2));
    iMODNum = G_IndexForMeansOfDeath(Scr_GetString(3));
    pOtherEnt = Scr_GetEntity(0);

    pEnt = G_TempEntity(*(vec3_t *)imp_vec3_origin, 0xc6);
    pEnt->s.otherEntityNum = pOtherEnt->s.number;

    if (Scr_GetType(1) == 1 && Scr_GetPointerType(1) == 0x15)
        pEnt->s.attackerEntityNum = Scr_GetEntity(1)->s.number;
    else
        pEnt->s.attackerEntityNum = 0x3fe;

    pEnt->r.svFlags = 8;
    if ((unsigned int)(iMODNum - 7) <= 1 || iMODNum == 0xc || iMODNum == 0xb || iMODNum == 9)
        pEnt->s.eventParm = iMODNum | 0x80;
    else
        pEnt->s.eventParm = iWeaponNum;

    return;
}

void GScr_getStartTime(void)
{
    { (void)(Scr_AddInt(level.startTime)); return; }
}

void GScr_PrecacheMenu(void)
{
    const char *pszNewMenu;
    char szConfigString[1024];
    int iConfigNum;
    int csIndex;

    pszNewMenu = Scr_GetString(0);

    for (csIndex = 0x4de; csIndex < 0x4fe; ++csIndex) {
        SV_GetConfigstring(csIndex, szConfigString, sizeof(szConfigString));
        if (!I_stricmp(szConfigString, pszNewMenu)) {
            Com_DPrintf("Script tried to precache the menu '%s' more than once\n", pszNewMenu);
            return;
        }
    }

    for (iConfigNum = 0; iConfigNum < 0x20; ++iConfigNum) {
        csIndex = iConfigNum + 0x4de;
        SV_GetConfigstring(csIndex, szConfigString, sizeof(szConfigString));
        if (!szConfigString[0]) {
            SV_SetConfigstring(csIndex, pszNewMenu);
            return;
        }
    }

    Scr_Error(va("Too many menus precached. Max allowed menus is %i", 0x20));
    SV_SetConfigstring(0x4fe, pszNewMenu);
    return;
}

int GScr_GetScriptMenuIndex(const char *pszMenu)
{
    char szConfigString[1024];
    int iConfigNum;

    for (iConfigNum = 0; iConfigNum < 0x20; ++iConfigNum) {
        SV_GetConfigstring(iConfigNum + 0x4de, szConfigString, sizeof(szConfigString));
        if (!I_stricmp(szConfigString, pszMenu))
            return iConfigNum;
    }

    Scr_Error(va("Menu '%s' was not precached\n", pszMenu));
    return 0;
}

void GScr_PrecacheStatusIcon(void)
{
    const char *pszNewIcon;
    char szConfigString[1024];
    int iConfigNum;
    int csIndex;

    pszNewIcon = Scr_GetString(0);

    for (csIndex = 0x17; csIndex < 0x1f; ++csIndex) {
        SV_GetConfigstring(csIndex, szConfigString, sizeof(szConfigString));
        if (!I_stricmp(szConfigString, pszNewIcon)) {
            Com_DPrintf("Script tried to precache the player status icon '%s' more than once\n", pszNewIcon);
            return;
        }
    }

    for (iConfigNum = 0; iConfigNum < 8; ++iConfigNum) {
        csIndex = iConfigNum + 0x17;
        SV_GetConfigstring(csIndex, szConfigString, sizeof(szConfigString));
        if (!szConfigString[0]) {
            SV_SetConfigstring(csIndex, pszNewIcon);
            return;
        }
    }

    Scr_Error(va("Too many player status icons precached. Max allowed is %i", 8));
    SV_SetConfigstring(0x1f, pszNewIcon);
    return;
}

int GScr_GetStatusIconIndex(const char *pszIcon)
{
    char szConfigString[1024];
    int iConfigNum;

    if (!pszIcon[0])
        return 0;

    for (iConfigNum = 0; iConfigNum < 8; iConfigNum++) {
        SV_GetConfigstring(iConfigNum + 0x17, szConfigString, sizeof(szConfigString));
        if (!I_stricmp(szConfigString, pszIcon))
            return iConfigNum + 1;
    }

    Scr_Error(va("Status icon '%s' was not precached\n", pszIcon));
    return 0;
}

void GScr_PrecacheHeadIcon(void)
{
    const char *pszNewIcon;
    char szConfigString[1024];
    int iConfigNum;
    int csIndex;

    pszNewIcon = Scr_GetString(0);

    for (csIndex = 0x1f; csIndex < 0x2e; ++csIndex) {
        SV_GetConfigstring(csIndex, szConfigString, sizeof(szConfigString));
        if (!I_stricmp(szConfigString, pszNewIcon)) {
            Com_DPrintf("Script tried to precache the player head icon '%s' more than once\n", pszNewIcon);
            return;
        }
    }

    for (iConfigNum = 0; iConfigNum < 0xf; ++iConfigNum) {
        csIndex = iConfigNum + 0x1f;
        SV_GetConfigstring(csIndex, szConfigString, sizeof(szConfigString));
        if (!szConfigString[0]) {
            SV_SetConfigstring(csIndex, pszNewIcon);
            return;
        }
    }

    Scr_Error(va("Too many player head icons precached. Max allowed is %i", 0xf));
    SV_SetConfigstring(0x2e, pszNewIcon);
    return;
}

int GScr_GetHeadIconIndex(const char *pszIcon)
{
    char szConfigString[1024];
    int iConfigNum;

    if (!pszIcon[0])
        return 0;

    for (iConfigNum = 0; iConfigNum < 15; iConfigNum++) {
        SV_GetConfigstring(iConfigNum + 0x1f, szConfigString, sizeof(szConfigString));
        if (!I_stricmp(szConfigString, pszIcon))
            return iConfigNum + 1;
    }

    Scr_Error(va("Head icon '%s' was not precached\n", pszIcon));
    return 0;
}

void Scr_BulletTracePassed(void)
{
    vec3_t vStart;
    vec3_t vEnd;
    int iClipMask;
    int entityNum = 0x3ff;

    Scr_GetVector(0, vStart);
    Scr_GetVector(1, vEnd);
    iClipMask = Scr_GetInt(2) ? 0x02802831 : 0x00802831;

    if (Scr_GetType(3) == 1 && Scr_GetPointerType(3) == 0x15)
        entityNum = Scr_GetEntity(3)->s.number;

    Scr_AddBool(G_LocationalTracePassed(vStart, vEnd, entityNum, iClipMask));
    return;
}

void Scr_SightTracePassed(void)
{
    vec3_t vStart;
    vec3_t vEnd;
    int iClipMask;
    int entityNum = 0x3ff;
    int hitNum;

    Scr_GetVector(0, vStart);
    Scr_GetVector(1, vEnd);
    iClipMask = Scr_GetInt(2) ? 0x02801803 : 0x00801803;

    if (Scr_GetType(3) == 1 && Scr_GetPointerType(3) == 0x15)
        entityNum = Scr_GetEntity(3)->s.number;

    G_SightTrace(&hitNum, vStart, vEnd, entityNum, iClipMask);
    if (!hitNum && SV_FX_GetVisibility(vStart, vEnd) < 0.2f)
        hitNum = 1;

    Scr_AddBool(hitNum == 0);
    return;
}

void Scr_PhysicsTrace(void)
{
    vec3_t start;
    vec3_t end;
    vec3_t endpos;
    vec3_t zero = { 0.0f, 0.0f, 0.0f };
    trace_t trace;

    Scr_GetVector(0, start);
    Scr_GetVector(1, end);

    G_TraceCapsule(&trace, start, zero, zero, end, 0x3ff, 0x00820011);
    endpos[0] = start[0] + (end[0] - start[0]) * trace.fraction;
    endpos[1] = start[1] + (end[1] - start[1]) * trace.fraction;
    endpos[2] = start[2] + (end[2] - start[2]) * trace.fraction;
    Scr_AddVector(endpos);
    return;
}

void Scr_RandomInt(void)
{
    int max = Scr_GetInt(0);
    if (max <= 0) {
        Com_Printf("RandomInt parm: %d  ", max);
        Scr_Error("RandomInt parm must be positive integer.\n");
        return;
    }
    Scr_AddInt(irand(0, max));
    return;
}

void Scr_RandomFloat(void)
{
    float max = Scr_GetFloat(0);

    Scr_AddFloat(flrand(0, max));
}

void Scr_RandomIntRange(void)
{
    int iMin = Scr_GetInt(0);
    int iMax = Scr_GetInt(1);

    if (iMin >= iMax) {
        Com_Printf("RandomIntRange parms: %d %d ", iMin, iMax);
        Scr_Error("RandomIntRange range must be positive integer.\n");
    }
    Scr_AddInt(irand(iMin, iMax));
    return;
}

void Scr_RandomFloatRange(void)
{
    float fMin = Scr_GetFloat(0);
    float fMax = Scr_GetFloat(1);

    if (fMin >= fMax) {
        Com_Printf("Scr_RandomFloatRange parms: %d %d ", (double)fMin, (double)fMax);
        Scr_Error("Scr_RandomFloatRange range must be positive float.\n");
    }
    Scr_AddFloat(flrand(fMin, fMax));
    return;
}

void GScr_sin(void)
{
    float val = Scr_GetFloat(0);

    Scr_AddFloat((float)sin((double)val * 0.017453292519943295));
}

void GScr_cos(void)
{
    float val = Scr_GetFloat(0);

    Scr_AddFloat((float)cos((double)val * 0.017453292519943295));
}

void GScr_tan(void)
{
    float radians = (float)((double)Scr_GetFloat(0) * 0.017453292519943295);
    float sinVal = sinf(radians);
    float cosVal = cosf(radians);

    if (cosVal == 0.0f)
        Scr_Error("divide by 0");
    Scr_AddFloat(sinVal / cosVal);
}

void GScr_asin(void)
{
    float x = Scr_GetFloat(0);

    if (x < -1.0f || x > 1.0f)
        Scr_Error(va("%g out of range", (double)x));
    Scr_AddFloat((float)((double)asinf(x) * 57.29577951308232));
}

void GScr_acos(void)
{
    float x = Scr_GetFloat(0);
    if (x < -1.0f || x > 1.0f) {
        Scr_Error(va("%g out of range", (double)x));
    }
    Scr_AddFloat((float)((double)acosf(x) * 57.29577951308232));
    return;
}

void GScr_atan(void)
{
    float val = Scr_GetFloat(0);
    Scr_AddFloat((float)((double)atanf(val) * 57.29577951308232));
}

void GScr_CastInt(void)
{
    int type = Scr_GetType(0);
    switch (type) {
    case 5:
        Scr_AddInt((int)Scr_GetFloat(0));
        break;
    case 6:
        Scr_AddInt(Scr_GetInt(0));
        break;
    case 2:
        Scr_AddInt(atoi(Scr_GetString(0)));
        break;
    default:
        Scr_ParamError(0, va("cannot cast %s to int", (const char *)Scr_GetTypeName(0)));
        break;
    }
    return;
}

void Scr_Distance(void)
{
    float v0[3], v1[3];
    Scr_GetVector(0, v0);
    Scr_GetVector(1, v1);
    Scr_AddFloat(Vec3Distance(v0, v1));
}

void Scr_DistanceSquared(void)
{
    float v0[3], v1[3];
    Scr_GetVector(0, v0);
    Scr_GetVector(1, v1);
    Scr_AddFloat(Vec3DistanceSq(v0, v1));
}

void Scr_LengthSquared(void)
{
    float v[3];
    Scr_GetVector(0, v);
    Scr_AddFloat(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

void Scr_Closer(void)
{
    float vRef[3], vA[3], vB[3];
    float fDistASqrd, fDistBSqrd;
    Scr_GetVector(0, vRef);
    Scr_GetVector(1, vA);
    Scr_GetVector(2, vB);
    fDistASqrd = Vec3DistanceSq(vA, vRef);
    fDistBSqrd = Vec3DistanceSq(vB, vRef);
    Scr_AddInt(fDistBSqrd > fDistASqrd);
}

void Scr_VectorDot(void)
{
    float a[3], b[3];
    Scr_GetVector(0, a);
    Scr_GetVector(1, b);
    Scr_AddFloat(a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}

void Scr_VectorToAngles(void)
{
    float vec[3], angles[3];
    Scr_GetVector(0, vec);
    vectoangles(vec, angles);
    Scr_AddVector(angles);
}

void Scr_AnglesToUp(void)
{
    float angles[3], up[3];
    Scr_GetVector(0, angles);
    AngleVectors(angles, 0, 0, up);
    Scr_AddVector(up);
}

void Scr_AnglesToRight(void)
{
    float angles[3], right[3];
    Scr_GetVector(0, angles);
    AngleVectors(angles, 0, right, 0);
    Scr_AddVector(right);
}

void Scr_AnglesToForward(void)
{
    float angles[3], forward[3];
    Scr_GetVector(0, angles);
    AngleVectors(angles, forward, 0, 0);
    Scr_AddVector(forward);
}

void Scr_IsSubStr(void)
{
    const char *sub = Scr_GetString(1);
    const char *s = Scr_GetString(0);
    Scr_AddBool(strstr(s, sub) != 0);
}

void Scr_GetSubStr(void)
{
    const char *s = Scr_GetString(0);
    int start = Scr_GetInt(1);
    int end = (Scr_GetNumParam() > 2) ? Scr_GetInt(2) : 0x7fffffff;
    char tempString[1025];
    int dest = 0;

    if (end > start) {
        const char *src = s + start;
        int count = end - start;

        while (*src && dest != count) {
            tempString[dest++] = *src++;
            if (dest == count)
                break;
            if (dest > 1023)
                Scr_Error("string too long");
        }
    }

    tempString[dest] = '\0';
    { (void)(Scr_AddString(tempString)); return; }
}

void Scr_ToLower(void)
{
    const char *p = Scr_GetString(0);
    char tempString[1024];
    int i = 1;

    for (;;) {
        char c = (char)___tolower((signed char)*p);
        tempString[i - 1] = c;
        if (c == 0) {
            Scr_AddString(tempString);
            return;
        }
        ++p;
        ++i;
        if (i == 1025) {
            Scr_Error("string too long");
            return;
        }
    }
}

static int Scr_IsTokenDelimiter(char ch, const char *delim, int delimLen)
{
    int i;

    for (i = 0; i < delimLen; ++i) {
        if (delim[i] == ch)
            return 1;
    }

    return 0;
}

void Scr_StrTok(void)
{
    const char *s;
    const char *delim;
    int delimLen;
    char tempString[1024];
    int dest;

    s = Scr_GetString(0);
    delim = Scr_GetString(1);
    delimLen = strlen(delim);

    Scr_MakeArray();

    dest = 0;
    while (*s) {
        if (delimLen > 0 && Scr_IsTokenDelimiter(*s, delim, delimLen)) {
            if (dest) {
                tempString[dest] = '\0';
                Scr_AddString(tempString);
                Scr_AddArray();
                dest = 0;
            }
        } else {
            tempString[dest++] = *s;
            if (dest > 0x3ff)
                Scr_Error("string too long");
        }

        ++s;
    }

    if (dest) {
        tempString[dest] = '\0';
        Scr_AddString(tempString);
        Scr_AddArray();
    }

    return;
}

void Scr_MusicPlay(void)
{
    SV_GameSendServerCommand(-1, 1, va("%c %s", 'o', Scr_GetString(0)));
}

void Scr_SoundFade(void)
{
    float targetVol = Scr_GetFloat(0);
    int fadeTime = 0;

    if (Scr_GetNumParam() > 1)
        fadeTime = (int)(Scr_GetFloat(1) * 1000.0f);
    SV_GameSendServerCommand(-1, 1, va("%c %f %i\n", 'q', (double)targetVol, fadeTime));
}

void Scr_PrecacheModel(void)
{
    if (!level.initializing)
        Scr_Error("precacheModel must be called before any wait statements in the gametype or level script\n");
    G_ModelIndex(Scr_GetString(0));
}

void Scr_PrecacheShellShock(void)
{
    if (!level.initializing)
        Scr_Error("precacheShellShock must be called before any wait statements in the gametype or level script\n");
    G_ShellShockIndex(Scr_GetString(0));
}

void Scr_PrecacheItem(void)
{
    const char *name;
    const gitem_t *item;

    if (!level.initializing) {
        Scr_Error("precacheItem must be called before any wait statements in the gametype or level script\n");
        return;
    }

    name = Scr_GetString(0);
    item = G_FindItem(name);
    if (!item) {
        Scr_ParamError(0, va("unknown item '%s'", name));
        return;
    }

    RegisterItem((int)(item - bg_itemlist), 1);
    return;
}

void Scr_PrecacheShader(void)
{
    const char *shaderName;
    if (!level.initializing)
        Scr_Error("precacheShader must be called before any wait statements in the gametype or level script\n");
    shaderName = Scr_GetString(0);
    if (!shaderName[0])
        Scr_ParamError(0, "shader name can't be empty");
    G_ShaderIndex(shaderName);
}

void Scr_PrecacheString(void)
{
    const char *s;
    int initializing = level.initializing;
    if (!initializing)
        Scr_Error("precacheString must be called before any wait statements in the gametype or level script\n");
    s = Scr_GetIString(0);
    if (s[0])
        G_LocalizedStringIndex(s);
}

void Scr_PrecacheRumble(void) {}

void GScr_RadiusDamage(void)
{
    vec3_t origin;
    float range;
    float max_damage;
    float min_damage;

    Scr_GetVector(0, origin);
    range = Scr_GetFloat(1);
    max_damage = Scr_GetFloat(2);
    min_damage = Scr_GetFloat(3);

    level.bPlayerIgnoreRadiusDamage = level.bPlayerIgnoreRadiusDamageLatched;
    G_RadiusDamage(origin, &g_entities[1022], NULL, max_damage, min_damage, range, NULL, 0xe);
    level.bPlayerIgnoreRadiusDamage = 0;
    return;
}

void GScr_SetPlayerIgnoreRadiusDamage(void)
{
    level.bPlayerIgnoreRadiusDamageLatched = Scr_GetInt(0);
}

static void GScr_GetAnimDeltaTimes(float *startTime, float *endTime)
{
    int numParams = Scr_GetNumParam();

    if (numParams == 1) {
        *startTime = 0.0f;
        *endTime = 1.0f;
        return;
    }

    if (numParams == 2) {
        *endTime = 1.0f;
    } else {
        *endTime = Scr_GetFloat(2);
        if (*endTime < 0.0f || *endTime > 1.0f)
            Scr_ParamError(2, "end time must be between 0 and 1");
    }

    *startTime = Scr_GetFloat(1);
    if (*startTime < 0.0f || *startTime > 1.0f)
        Scr_ParamError(1, "start time must be between 0 and 1");
}

static inline __attribute__((always_inline)) int GScr_SecondsToMilliseconds(float seconds)
{
    return (int)floorf(seconds * 1000.0f + 0.5f);
}

static void GScr_CleanDvarValue(const char *dvarValue, char *outString, int outLimit)
{
    int i;

    for (i = 0; dvarValue[i] && i < outLimit - 1; ++i) {
        char ch = I_CleanChar((signed char)dvarValue[i]);

        outString[i] = (ch == '"') ? '\'' : ch;
    }

    outString[i] = '\0';
}

void GScr_GetMoveDelta(void)
{
    float startTime;
    float endTime;
    unsigned int anim;
    vec3_t rot;
    vec3_t trans;

    GScr_GetAnimDeltaTimes(&startTime, &endTime);

    anim = GScr_GetPackedAnim(0);
    XAnimGetRelDelta(Scr_GetAnims(anim >> 16), (unsigned short)anim, rot, trans, startTime, endTime);
    Scr_AddVector(trans);
    return;
}

void GScr_GetAngleDelta(void)
{
    float startTime;
    float endTime;
    unsigned int anim;
    vec3_t rot;
    vec3_t trans;

    GScr_GetAnimDeltaTimes(&startTime, &endTime);

    anim = GScr_GetPackedAnim(0);
    XAnimGetRelDelta(Scr_GetAnims(anim >> 16), (unsigned short)anim, rot, trans, startTime, endTime);
    Scr_AddFloat(RotationToYaw(rot));
    return;
}

void GScr_GetNorthYaw(void)
{
    char northYawString[32];
    SV_GetConfigstring(11, northYawString, 32);
    Scr_AddFloat((float)atof(northYawString));
}

void Scr_LoadFX(void)
{
    int id;
    id = G_EffectIndex(Scr_GetString(0));
    if (!id && !level.initializing)
        Scr_Error("loadFx must be called before any wait statements in the gametype or level script, or on an already loaded effect\n");
    Scr_AddInt(id);
}

void Scr_PlayFXOnTag(void)
{
    int fxId;
    gentity_t *ent;
    unsigned int tag;
    const char *tagName;
    int configIndex;

    if (Scr_GetNumParam() != 3)
        Scr_Error("USAGE: playFxOnTag <effect id from loadFx> <entity> <tag name>");

    fxId = Scr_GetInt(0);
    if ((unsigned int)(fxId - 1) > 0x3e)
        Scr_ParamError(0, va("effect id %i is invalid\n", fxId));

    ent = Scr_GetEntity(1);
    if (!ent->model)
        Scr_ParamError(1, "cannot play fx on entity with no model");

    tag = Scr_GetConstLowercaseString(2);
    tagName = SL_ConvertToString(tag);
    if (strchr(tagName, '"'))
        Scr_ParamError(2, "cannot use \" characters in tag names\n");

    if (SV_DObjGetBoneIndex(ent, tag) < 0) {
        SV_DObjDumpInfo(ent);
        Scr_ParamError(2, va("tag '%s' does not exist on entity with model '%s'",
                             tagName, G_ModelName(ent->model)));
    }

    configIndex = G_FindConfigstringIndex(va("%02d%s", fxId, tagName), 0x38e, 0x100, 1, 0);
    G_AddEvent(ent, 0xc3, configIndex);
    return;
}

void Scr_SetFog(const char *cmd, float fNear, float fFar, float fDensity, float fRed, float fGreen, float fBlue, float fTime)
{
    if (fNear < 0.0f)
        Scr_Error(va("%s: near distance must be >= 0", cmd));
    if (fNear >= fFar)
        Scr_Error(va("%s: near distance must be less than far distance", cmd));
    if (fRed < 0.0f || fRed > 1.0f || fGreen < 0.0f || fGreen > 1.0f || fBlue < 0.0f || fBlue > 1.0f)
        Scr_Error(va("%s: red/green/blue color components must be in the range [0, 1]", cmd));
    if (fTime < 0.0f)
        Scr_Error(va("%s: transition time must be >= 0 seconds", cmd));

    G_setfog(va("%g %g %g %g %g %g %.0f",
                (double)fNear,
                (double)fFar,
                (double)fDensity,
                (double)fRed,
                (double)fGreen,
                (double)fBlue,
                (double)(fTime * 1000.0f)));
    return;
}

void Scr_SetLinearFog(void)
{
    float fNear;
    float fFar;
    float fRed;
    float fGreen;
    float fBlue;
    float fTime;

    if (Scr_GetNumParam() != 6)
        Scr_Error("USAGE: setCullFog(near distance, far distance, red, green, blue, transition time);\n");

    fNear = Scr_GetFloat(0);
    fFar = Scr_GetFloat(1);
    fRed = Scr_GetFloat(2);
    fGreen = Scr_GetFloat(3);
    fBlue = Scr_GetFloat(4);
    fTime = Scr_GetFloat(5);

    Scr_SetFog("setCullFog", fNear, fFar, 1.0f, fRed, fGreen, fBlue, fTime);
}

void Scr_SetExponentialFog(void)
{
    float fDensity;
    float fRed;
    float fGreen;
    float fBlue;
    float fTime;

    if (Scr_GetNumParam() != 5)
        Scr_Error("USAGE: setExpFog(density, red, green, blue, transition time);\nDensity must be greater than 0 and less than 1, and typically less than .001.  For example, .0002 means the fog gets .02%% more dense for every 1 unit of distance (about 1%% thicker every 50 units of distance)\n");

    fDensity = Scr_GetFloat(0);
    fRed = Scr_GetFloat(1);
    fGreen = Scr_GetFloat(2);
    fBlue = Scr_GetFloat(3);
    fTime = Scr_GetFloat(4);

    if (fDensity <= 0.0f || fDensity >= 1.0f)
        Scr_Error("setExpFog: distance must be greater than 0 and less than 1");

    Scr_SetFog("setExpFog", 0.0f, 1.0f, fDensity, fRed, fGreen, fBlue, fTime);
}

void GScr_IsPlayer(void)
{
    if (Scr_GetType(0) == 1 && Scr_GetPointerType(0) == 0x15) {
        gentity_t *ent = Scr_GetEntity(0);
        struct gclient_s *client = ent->client;
        if (client) {
            Scr_AddInt(1);
            return;
        }
    }
    Scr_AddInt(0);
    return;
}

void GScr_IsPlayerNumber(void)
{
    int num = Scr_GetInt(0);
    if ((unsigned int)num > 0x3f)
        Scr_AddInt(0);
    else
        Scr_AddInt(1);
}

void GScr_SetWinningPlayer(void)
{
    gentity_t *ent = Scr_GetEntity(0);

    GScr_UpdateWinnerConfig(ent->s.number + 1);
}

void GScr_SetWinningTeam(void)
{
    scr_string_t team = Scr_GetConstString(0);
    int winner;

    if (team == scr_const.allies)
        winner = -2;
    else if (team == scr_const.axis)
        winner = -1;
    else if (team == scr_const.none)
        winner = 0;
    else
        { (void)(Scr_ParamError(0, va("Illegal team string '%s'. Must be allies, axis, or none.", SL_ConvertToString(team)))); return; }

    GScr_UpdateWinnerConfig(winner);
}

void GScr_GetTeamScore(void)
{
    scr_string_t team = Scr_GetConstString(0);
    scr_const_t *scr_const = (scr_const_t *)imp_scr_const;

    if (team != scr_const->allies && team != scr_const->axis)
        Scr_Error(va("Illegal team string '%s'. Must be allies, or axis.", SL_ConvertToString(team)));

    if (team == scr_const->allies)
        Scr_AddInt(level.teamScores[TEAM_ALLIES]);
    else
        Scr_AddInt(level.teamScores[TEAM_AXIS]);
}

void GScr_SetTeamScore(void)
{
    scr_string_t team = Scr_GetConstString(0);
    int score;

    if (team != scr_const.allies && team != scr_const.axis)
        Scr_Error(va("Illegal team string '%s'. Must be allies, or axis.", SL_ConvertToString(team)));

    score = Scr_GetInt(1);
    if (team == scr_const.allies) {
        level.teamScores[TEAM_ALLIES] = score;
        SV_GameSendServerCommand(-1, 0, va("%c %i", 'H', score));
    } else {
        level.teamScores[TEAM_AXIS] = score;
        SV_GameSendServerCommand(-1, 0, va("%c %i", 'G', score));
    }

    level.bUpdateScoresForIntermission = 1;
}

void GScr_SetClientNameMode(void)
{
    scr_string_t mode = Scr_GetConstString(0);

    if (mode == scr_const.auto_change)
        level.manualNameChange = 0;
    else if (mode == scr_const.manual_change)
        level.manualNameChange = 1;
    else
        Scr_Error("Unknown mode");
}

void GScr_UpdateClientNames(void)
{
    int i;
    level_locals_t *lvl = &level;

    if (!lvl->manualNameChange)
        Scr_Error("Only works in [manual_change] mode");

    for (i = 0; i < lvl->maxclients; ++i) {
        gclient_t *cl = &lvl->clients[i];

        if (cl->sess.connected == CON_CONNECTED && strcmp(cl->sess.cs.name, cl->sess.newnetname) != 0) {
            char oldname[32];

            I_strncpyz(oldname, cl->sess.cs.name, sizeof(oldname));
            I_strncpyz(cl->sess.cs.name, cl->sess.newnetname, sizeof(cl->sess.cs.name));
            ClientUserinfoChanged(i);
        }
    }

    return;
}

void GScr_GetTeamPlayersAlive(void)
{
    scr_const_t *scr_const = (scr_const_t *)imp_scr_const;
    unsigned int team = Scr_GetConstString(0);
    int teamNum;
    int livePlayers = 0;
    int i;

    if (team == scr_const->allies)
        teamNum = TEAM_ALLIES;
    else {
        if (team != scr_const->axis)
            Scr_Error(va("Illegal team string '%s'. Must be allies, or axis.", SL_ConvertToString(team)));
        teamNum = TEAM_AXIS;
    }

    for (i = 0; i < g_maxclients->current.integer; i++) {
        gentity_t *ent = &g_entities[i];
        if (ent->r.inuse && ent->client->sess.cs.team == teamNum && ent->health >= 1)
            livePlayers++;
    }

    Scr_AddInt(livePlayers);
    return;
}

void GScr_GetNumParts(void)
{
    { (void)(Scr_AddInt(XModelNumBones(SV_XModelGet(Scr_GetString(0))))); return; }
}

void GScr_GetPartName(void)
{
    void *model = SV_XModelGet(Scr_GetString(0));
    int index = Scr_GetInt(1);
    int numBones = XModelNumBones(model);
    unsigned short name;

    if ((unsigned int)index >= (unsigned int)numBones)
        Scr_ParamError(1, va("index out of range (0 - %d)", numBones - 1));

    name = ((unsigned short *)XModelBoneNames(model))[index];
    if (!name)
        Scr_ParamError(0, "bad model");

    Scr_AddConstString(name);
}

void GScr_Earthquake(void)
{
    float scale;
    int duration;
    vec3_t source;
    float radius;
    gentity_t *ent;

    scale = Scr_GetFloat(0);
    duration = GScr_SecondsToMilliseconds(Scr_GetFloat(1));
    Scr_GetVector(2, source);
    radius = Scr_GetFloat(3);

    if (scale <= 0.0f)
        Scr_ParamError(0, "Scale must be greater than 0");

    if (duration <= 0)
        Scr_ParamError(1, "duration must be greater than 0");

    if (radius <= 0.0f)
        Scr_ParamError(3, "Radius must be greater than 0");

    ent = G_TempEntity(source, 0xc4);
    ent->s.angles2[0] = scale;
    ent->s.time = duration;
    ent->s.angles2[1] = radius;
}

void GScr_MapRestart(void)
{
    level_locals_t *lvl = &level;

    if (lvl->finished)
        Scr_Error(lvl->finished == 1 ? "map_restart already called" : "exitlevel already called");

    lvl->finished = 1;
    lvl->savepersist = 0;
    if (Scr_GetNumParam())
        lvl->savepersist = Scr_GetInt(0);
    Cbuf_ExecuteText(2, "fast_restart\n");
}

void GScr_LoadMap(void)
{
    const char *mapname;
    level_locals_t *lvl;

    if (!Scr_GetNumParam())
        return;

    mapname = Scr_GetString(0);
    if (!SV_MapExists(mapname))
        return;

    lvl = &level;
    if (lvl->finished)
        Scr_Error(lvl->finished == 2 ? "map already called" : "exitlevel already called");

    lvl->finished = 2;
    lvl->savepersist = 0;
    if (Scr_GetNumParam() > 1)
        lvl->savepersist = Scr_GetInt(1);
    Cbuf_ExecuteText(2, va("map %s\n", mapname));
    return;
}

void GScr_ExitLevel(void)
{
    level_locals_t *lvl = &level;

    if (lvl->finished)
        Scr_Error(lvl->finished == 1 ? "map_restart already called" : "exitlevel already called");

    lvl->finished = 3;
    lvl->savepersist = 0;
    if (Scr_GetNumParam())
        lvl->savepersist = Scr_GetInt(0);

    SV_MatchEnd();
    ExitLevel();
}

void GScr_AddTestClient(void)
{
    void *ent = (void *)SV_AddTestClient();
    if (ent)
        Scr_AddEntity(ent);
}

void GScr_AllClientsPrint(void)
{
    if (!Scr_GetNumParam())
        return;
    const char *msg = Scr_GetString(0);
    SV_GameSendServerCommand(-1, 0, va("%c \"%s\"", 0x65, msg));
}

void GScr_MapExists(void)
{
    if (!Scr_GetNumParam())
        return;
    const char *name = Scr_GetString(0);
    if (SV_MapExists(name))
        Scr_AddInt(1);
    else
        Scr_AddInt(0);
}

void GScr_SetVoteString(void)
{
    if (Scr_GetNumParam()) {
        const level_locals_t *lvl;
        SV_SetConfigstring(0x10, Scr_GetString(0));
        lvl = (const level_locals_t *)imp_level;
        SV_SetConfigstring(0xf, va("%i", lvl->voteTime));
        SV_SetConfigstring(0x11, va("%i", lvl->voteYes));
        SV_SetConfigstring(0x12, va("%i", lvl->voteNo));
    }

    return;
}

void GScr_SetVoteTime(void)
{
    if (Scr_GetNumParam()) {
        const level_locals_t *lvl;
        SV_SetConfigstring(0xf, va("%i", Scr_GetInt(0)));
        lvl = (const level_locals_t *)imp_level;
        SV_SetConfigstring(0x11, va("%i", lvl->voteYes));
        SV_SetConfigstring(0x12, va("%i", lvl->voteNo));
    }

    return;
}

void GScr_SetVoteYesCount(void)
{
    if (Scr_GetNumParam()) {
        SV_SetConfigstring(0x11, va("%i", Scr_GetInt(0)));
        SV_SetConfigstring(0x12, va("%i", level.voteNo));
    }

    return;
}

void GScr_SetVoteNoCount(void)
{
    if (Scr_GetNumParam())
        SV_SetConfigstring(0x12, va("%i", Scr_GetInt(0)));
}

void GScr_KickPlayer(void)
{
    if (Scr_GetNumParam())
        Cbuf_ExecuteText(2, va("tempBanClient %i\n", Scr_GetInt(0)));
}

void GScr_BanPlayer(void)
{
    if (Scr_GetNumParam())
        Cbuf_ExecuteText(2, va("banClient %i\n", Scr_GetInt(0)));
}

void GScr_ClientPrint(void)
{
    gentity_t *ent;

    if (!Scr_GetNumParam())
        return;

    ent = Scr_GetEntity(0);
    SV_GameSendServerCommand(ent - g_entities, 0, va("%c \"%s\"", 'e', Scr_GetString(1)));
}

void GScr_OpenFile(void)
{
    const char *filename;
    const char *mode;
    fileHandle_t tempFile;
    int fileSize;
    char *buffer;

    if (Scr_GetNumParam() <= 1)
        return;

    filename = Scr_GetString(0);
    mode = Scr_GetString(1);

    if (*GScr_ScriptIOFileHandle(0) >= 0) {
        Com_Printf("OpenFile failed.  %i files already open\n", 1);
        Scr_AddInt(-1);
        return;
    }

    if (!strcmp(mode, "read")) {
        fileSize = FS_FOpenFileByMode(va("%s/%s", "scriptdata", filename), &tempFile, 0);
        if (fileSize < 0) {
            Scr_AddInt(-1);
            return;
        }

        buffer = (char *)Z_MallocInternal(fileSize + 1);
        *GScr_ScriptIOFileBuffer(0) = buffer;
        FS_Read(buffer, fileSize, tempFile);
        FS_FCloseFile(tempFile);
        buffer[fileSize] = '\0';

        Com_BeginParseSession(filename);
        Com_SetCSV(1);
        GScr_ScriptIOLineMark(0)->lines = 0;
        Scr_AddInt(0);
        return;
    }

    if (!strcmp(mode, "write")) {
        fileSize = FS_FOpenFileByMode(va("%s/%s", "scriptdata", filename), GScr_ScriptIOFileHandle(0), 1);
        Scr_AddInt(fileSize < 0 ? -1 : 0);
        return;
    }

    if (!strcmp(mode, "append")) {
        fileSize = FS_FOpenFileByMode(va("%s/%s", "scriptdata", filename), GScr_ScriptIOFileHandle(0), 2);
        Scr_AddInt(fileSize < 0 ? -1 : 0);
        return;
    }

    Com_Printf("Valid openfile modes are 'write', 'read', and 'append'\n");
    Scr_AddInt(-1);
    return;
}

void GScr_CloseFile(void)
{
    unsigned int filenum;
    fileHandle_t handle;
    char *buffer;

    if (!Scr_GetNumParam())
        return;

    filenum = Scr_GetInt(0);
    if (filenum > 1) {
        Com_Printf("CloseFile failed, invalid file number %i\n", filenum);
        Scr_AddInt(-1);
        return;
    }

    handle = *GScr_ScriptIOFileHandle(filenum);
    if (handle > 0) {
        FS_FCloseFile(handle);
        *GScr_ScriptIOFileHandle(filenum) = -1;
        Scr_AddInt(1);
        return;
    }

    buffer = *GScr_ScriptIOFileBuffer(filenum);
    if (buffer) {
        Com_EndParseSession();
        Z_FreeInternal(buffer);
        *GScr_ScriptIOFileBuffer(filenum) = NULL;
        Scr_AddInt(1);
        return;
    }

    Com_Printf("CloseFile failed, file number %i was not open\n", filenum);
    Scr_AddInt(-1);
}

void GScr_FPrintln(void)
{
    int paramCount;
    unsigned int filenum;
    int arg;
    fileHandle_t handle;
    const char *text;

    paramCount = Scr_GetNumParam();
    if (paramCount <= 1) {
        Com_Printf("fprintln requires at least 2 parameters (file, output)\n");
        Scr_AddInt(-1);
        return;
    }

    filenum = Scr_GetInt(0);
    if (filenum > 1) {
        Com_Printf("FPrintln failed, invalid file number %i\n", filenum);
        Scr_AddInt(-1);
        return;
    }

    handle = *GScr_ScriptIOFileHandle(filenum);
    if (handle < 0) {
        Com_Printf("FPrintln failed, file number %i was not open for writing\n", filenum);
        Scr_AddInt(-1);
        return;
    }

    for (arg = 1; Scr_GetNumParam() > (unsigned int)arg; arg++) {
        text = Scr_GetString(arg);
        handle = *GScr_ScriptIOFileHandle(filenum);
        FS_Write(text, strlen(text), handle);
        FS_Write(",", 1, handle);
    }

    Scr_AddInt(Scr_GetNumParam() - 1);
}

void GScr_FReadLn(void)
{
    unsigned int filenum;
    const char *buf;
    com_parse_mark_t *mark;
    const char *token;

    if (!Scr_GetNumParam()) {
        Com_Printf("freadln requires a parameter - the file to read from\n");
        Scr_AddInt(-1);
        return;
    }

    filenum = Scr_GetInt(0);
    if (filenum > 1) {
        Com_Printf("freadln failed, invalid file number %i\n", filenum);
        Scr_AddInt(-1);
        return;
    }

    buf = *GScr_ScriptIOFileBuffer(filenum);
    if (!buf) {
        Com_Printf("freadln failed, file number %i was not open for reading\n", filenum);
        Scr_AddInt(-1);
        return;
    }

    mark = GScr_ScriptIOLineMark(filenum);
    if (!mark->lines) {
        Com_ParseSetMark(&buf, mark);
        Scr_AddInt(Com_GetArgCountOnLine(&buf));
        return;
    }

    Com_ParseReturnToMark(&buf, mark);
    Com_SkipRestOfLine(&buf);
    Com_ParseSetMark(&buf, mark);
    token = Com_Parse(&buf);
    Com_ParseReturnToMark(&buf, mark);

    if (!token[0]) {
        Scr_AddInt(-1);
        return;
    }

    Scr_AddInt(Com_GetArgCountOnLine(&buf));
}

void GScr_FGetArg(void)
{
    unsigned int filenum;
    int arg;
    int parsedArg;
    const char *buf;
    const char *token;

    if (Scr_GetNumParam() <= 1) {
        Com_Printf("freadline requires at least 2 parameters (file, string)\n");
        Scr_AddString("");
        return;
    }

    filenum = Scr_GetInt(0);
    arg = Scr_GetInt(1);

    if (filenum > 1) {
        Com_Printf("freadline failed, invalid file number %i\n", filenum);
        Scr_AddString("");
        return;
    }

    if (arg < 0) {
        Com_Printf("freadline failed, invalid argument number %i\n", arg);
        Scr_AddString("");
        return;
    }

    buf = *GScr_ScriptIOFileBuffer(filenum);
    if (!buf) {
        Com_Printf("freadline failed, file number %i was not open for reading\n", filenum);
        Scr_AddString("");
        return;
    }

    Com_ParseReturnToMark(&buf, GScr_ScriptIOLineMark(filenum));
    parsedArg = 0;
    while (1) {
        token = Com_ParseOnLine(&buf);
        if (!token[0]) {
            Com_Printf("freadline failed, there aren't %i arguments on this line, there are only %i arguments\n",
                       arg + 1, parsedArg);
            Scr_AddString("");
            return;
        }

        parsedArg++;
        if (arg < parsedArg)
            break;
    }

    Scr_AddString(token);
}

void GScr_SetArchive(void)
{
    SV_EnableArchivedSnapshot(Scr_GetInt(0));
}

void Scr_PlayRumbleOnPos(void) {}

void Scr_PlayLoopRumbleOnPos(void) {}

void Scr_StopAllRumbles(void) {}

void Scr_IsSplitscreen(void)
{
    { (void)(Scr_AddInt(0)); return; }
}

void GScr_MatchEnd(void) {}

void GScr_SetPlayerTeamRank(void) {}

void GScr_SendXboxLiveRanks(void) {}

void GScr_EndXboxLiveLobby(void)
{
}

void GScr_ReleaseClaimedTrigger(scr_entref_t entref)
{
    gentity_t *triggerEnt = GScr_EntityFromEntRef(entref);

    if (!GScr_IsUseTrigger(triggerEnt))
        GScr_UseTriggerError("releaseclaimedtrigger: trigger entity must be of type %s or %s");

    triggerEnt->trigger.singleUserEntIndex = 0x3ff;
}

/* No-op for Xbox-360-only script builtins that stock PC scripts still reference inside dead
   level.xenon branches (e.g. endparty() at maps/mp/gametypes/_menus.gsc:92). The retail PC
   engine registers these as no-ops so the scripts compile; without it the GSC compiler aborts
   with "unknown function" and the map never loads. */
static void Scr_XenonStub(void)
{
    return;
}

BuiltinFunction Scr_GetFunction(const char **pName, int *type)
{
    int i;
    for (i = 0; i < FUNCTIONS_COUNT; i++) {
        if (I_stricmp(*pName, functions[i].actionString) == 0) {
            *pName = functions[i].actionString;
            *type = functions[i].type;
            return functions[i].actionFunc;
        }
    }

    if (I_stricmp(*pName, "endparty") == 0) {
        static const char endparty_name[] = "endparty";
        *pName = endparty_name;
        *type = 0;
        return (BuiltinFunction)Scr_XenonStub;
    }

    return (BuiltinFunction)0;
}

BuiltinMethod Scr_GetMethod(const char **pName, int *type)
{
    int i;
    BuiltinMethod meth;

    *type = 0;

    meth = Player_GetMethod(pName);
    if (meth)
        return meth;

    meth = ScriptEnt_GetMethod(pName);
    if (meth)
        return meth;

    meth = HudElem_GetMethod(pName);
    if (meth)
        return meth;

    for (i = 0; i < BUILTIN_METHODS_COUNT; i++) {
        if (!methods[i].actionString)
            continue;

        if (strcmp(*pName, methods[i].actionString) == 0) {
            *pName = methods[i].actionString;
            *type = methods[i].type;
            return methods[i].actionFunc;
        }
    }

    return (BuiltinMethod)0;
}

void Scr_SetOrigin(gentity_t *ent, int offset)
{
    vec3_t org;

    (void)offset;
    Scr_GetVector(0, org);
    G_SetOrigin(ent, org);
    if (ent->r.linked) {
        SV_LinkEntity(ent);
    }
    return;
}

void Scr_SetAngles(gentity_t *ent, int offset)
{
    vec3_t angles;

    (void)offset;
    Scr_GetVector(0, angles);
    G_SetAngle(ent, angles);
}

void Scr_SetHealth(gentity_t *ent, int offset)
{
    int health;

    (void)offset;
    health = Scr_GetInt(0);

    if (ent->client) {
        ent->health = health;
        ent->client->ps.stats[0] = health;
    } else {
        ent->maxHealth = health;
        ent->health = health;
    }
    return;
}

void GScr_AddEntity(gentity_t *pEnt)
{
    if (pEnt) {
        Scr_AddEntity(pEnt);
        return;
    }
    Scr_AddUndefined();
}

void Scr_ParseGameTypeList(void)
{
    char gameTypeList[0x1000];
    char buffer[0x400];
    char *fileName;
    char *fileEnd;
    const char *parse;
    const char *token;
    gameTypeScript_t *gameType;
    fileHandle_t file;
    int fileLength;
    int scriptCount;
    int gameTypeCount;
    int i;
    int len;

    memset(g_scr_data.gametype.list, 0, sizeof(g_scr_data.gametype.list));

    scriptCount = FS_GetFileList("maps/mp/gametypes", "gsc", 0, gameTypeList, sizeof(gameTypeList));
    gameTypeCount = 0;

    if (scriptCount > 0) {
        fileName = gameTypeList;
        for (i = 0; i < scriptCount; i++) {
            len = strlen(fileName);
            fileEnd = fileName + len;

            if (fileName[0] == '_') {
                fileName = fileEnd + 1;
                continue;
            }

            if (len >= 4 && !I_stricmp(fileEnd - 4, ".gsc"))
                fileName[len - 4] = '\0';

            if (gameTypeCount == 32) {
                Com_Printf("Too many game type scripts found! Only loading the first %i\n", 31);
                break;
            }

            gameType = &g_scr_data.gametype.list[gameTypeCount];
            I_strncpyz(gameType->pszScript, fileName, sizeof(gameType->pszScript));
            strlwr(gameType->pszScript);

            fileLength = FS_FOpenFileByMode(va("maps/mp/gametypes/%s.txt", fileName), &file, 0);
            if ((unsigned int)(fileLength - 1) <= 0x3fe) {
                FS_Read(buffer, fileLength, file);
                buffer[fileLength] = '\0';
                parse = buffer;

                token = Com_Parse(&parse);
                I_strncpyz(gameType->pszName, token, sizeof(gameType->pszName));

                token = Com_Parse(&parse);
                gameType->bTeamBased = token && !I_stricmp(token, "team");
            } else {
                if (fileLength > 0)
                    Com_Printf("WARNING: GameType description file %s is too big to load.\n",
                               va("maps/mp/gametypes/%s.txt", fileName));
                else
                    Com_Printf("WARNING: Could not load GameType description file %s for gametype %s\n",
                               va("maps/mp/gametypes/%s.txt", fileName), fileName);

                I_strncpyz(gameType->pszName, gameType->pszScript, sizeof(gameType->pszName));
                gameType->bTeamBased = 0;
            }

            gameTypeCount++;
            if (fileLength > 0)
                FS_FCloseFile(file);

            fileName = fileEnd + 1;
        }
    }

    g_scr_data.gametype.iNumGameTypes = gameTypeCount;
}

const char *Scr_GetGameTypeNameForScript(const char *pszGameTypeScript)
{
    int i;

    for (i = 0; i < g_scr_data.gametype.iNumGameTypes; ++i) {
        if (!I_stricmp(g_scr_data.gametype.list[i].pszScript, pszGameTypeScript))
            return g_scr_data.gametype.list[i].pszName;
    }

    return NULL;
}

void Scr_LoadGameType(void)
{
    unsigned int handle = g_scr_data.gametype.main;
    unsigned int threadId = Scr_ExecThread(handle, 0);
    Scr_FreeThread((unsigned short)threadId);
}

void Scr_StartupGameType(void)
{
    unsigned int handle = g_scr_data.gametype.startupgametype;
    unsigned int threadId = Scr_ExecThread(handle, 0);
    Scr_FreeThread((unsigned short)threadId);
}

void Scr_PlayerConnect(gentity_t *self)
{
    unsigned int threadId = Scr_ExecEntThread(self, g_scr_data.gametype.playerconnect, 0);
    if (getenv("DBGSPAM"))
        Com_Printf("[team-trace] Scr_PlayerConnect ent=%p handle=%u thread=%u\n",
                   (void *)self, g_scr_data.gametype.playerconnect, threadId);
    Scr_FreeThread((unsigned short)threadId);
}

void Scr_PlayerDisconnect(gentity_t *self)
{
    unsigned int threadId = Scr_ExecEntThread(self, g_scr_data.gametype.playerdisconnect, 0);
    Scr_FreeThread((unsigned short)threadId);
}

void Scr_VoteCalled(gentity_t *self, char *command, char *param1, char *param2)
{
    Scr_AddString(param2);
    Scr_AddString(param1);
    Scr_AddString(command);
    Scr_Notify(self, scr_const.call_vote, 3);
}

void Scr_PlayerVote(gentity_t *self, char *option)
{
    Scr_AddString(option);
    Scr_Notify(self, scr_const.vote, 1);
}

void GScr_print3d(void)
{
    vec4_t color = { 1.0f, 1.0f, 1.0f, 1.0f };
    vec3_t rgb;
    vec3_t origin;
    const char *text;
    float scale;
    int numParams;

    numParams = Scr_GetNumParam();
    if (numParams == 2) {
        scale = 1.0f;
    } else {
        if (numParams == 3) {
            scale = 1.0f;
        } else {
            if (numParams == 4)
                scale = 1.0f;
            else
                scale = Scr_GetFloat(4);

            color[3] = Scr_GetFloat(3);
        }

        Scr_GetVector(2, rgb);
        color[0] = rgb[0];
        color[1] = rgb[1];
        color[2] = rgb[2];
    }

    text = Scr_GetString(1);
    Scr_GetVector(0, origin);
    G_AddDebugString(origin, color, scale, text);
    return;
}

void GScr_line(void)
{
    vec4_t color = { 1.0f, 1.0f, 1.0f, 1.0f };
    vec3_t rgb;
    vec3_t start;
    vec3_t end;
    int depthTest;
    int numParams;

    numParams = Scr_GetNumParam();
    if (numParams == 2) {
        depthTest = 0;
    } else {
        if (numParams == 3) {
            depthTest = 0;
        } else {
            if (numParams == 4)
                depthTest = 0;
            else
                depthTest = Scr_GetInt(4);

            color[3] = Scr_GetFloat(3);
        }

        Scr_GetVector(2, rgb);
        color[0] = rgb[0];
        color[1] = rgb[1];
        color[2] = rgb[2];
    }

    Scr_GetVector(1, end);
    Scr_GetVector(0, start);
    CL_AddDebugLine(start, end, color, depthTest, 0, 1);
    return;
}

void GScr_Spawn(void)
{
    unsigned int classname;
    vec3_t origin;
    int iSpawnFlags;
    gentity_t *ent;

    classname = Scr_GetConstString(0);
    Scr_GetVector(1, origin);

    if (Scr_GetNumParam() > 2)
        iSpawnFlags = Scr_GetInt(2);
    else
        iSpawnFlags = 0;

    ent = G_Spawn();
    Scr_SetString(&ent->classname, (unsigned short)classname);
    ent->r.currentOrigin[0] = origin[0];
    ent->r.currentOrigin[1] = origin[1];
    ent->r.currentOrigin[2] = origin[2];
    ent->spawnflags = iSpawnFlags;

    if (G_CallSpawnEntity(ent))
        Scr_AddEntity(ent);
    else
        Scr_Error(va("unable to spawn \"%s\" entity", SL_ConvertToString((unsigned short)classname)));

    return;
}

void GScr_SpawnTurret(void)
{
    unsigned int classname;
    vec3_t origin;
    const char *weaponinfoname;
    gentity_t *ent;

    classname = Scr_GetConstString(0);
    Scr_GetVector(1, origin);
    weaponinfoname = Scr_GetString(2);

    ent = G_Spawn();
    Scr_SetString(&ent->classname, (unsigned short)classname);
    {
        float *co = &ent->r.currentOrigin[0];
        co[0] = origin[0];
        co[1] = origin[1];
        co[2] = origin[2];
    }
    G_SpawnTurret(ent, weaponinfoname);
    Scr_AddEntity(ent);
}

void Scr_VectorNormalize(void)
{
    vec3_t a, b;
    Scr_GetVector(0, a);
    b[0] = a[0];
    b[1] = a[1];
    b[2] = a[2];
    Vec3Normalize(b);
    Scr_AddVector(b);
}

void Scr_GrenadeExplosionEffect(void)
{
    vec3_t vOrg;
    vec3_t vPos;
    vec3_t vDir = { 0.0f, 0.0f, 1.0f };
    vec3_t vEnd;
    trace_t trace;
    gentity_t *pEnt;

    Scr_GetVector(0, vOrg);
    vPos[0] = vOrg[0];
    vPos[1] = vOrg[1];
    vPos[2] = vOrg[2] + 1.0f;

    pEnt = G_TempEntity(vPos, 0xbc);
    pEnt->s.eventParm = DirToByte(vDir);

    vEnd[0] = vPos[0];
    vEnd[1] = vPos[1];
    vEnd[2] = vPos[2] - 17.0f;

    G_TraceCapsule(&trace, vPos, (const vec_t *)imp_vec3_origin, (const vec_t *)imp_vec3_origin, vEnd, 0x3ff, 0x811);
    pEnt->s.surfType = (unsigned int)(trace.surfaceFlags & 0x1f00000) >> 20;
    return;
}

void Scr_Objective_Position(void)
{
    int objIndex = Scr_GetInt(0);
    objective_t *obj;

    if ((unsigned int)objIndex > 0xf)
        Scr_ParamError(0, va("index %i is an illegal objective index. Valid indexes are 0 to %i\n", objIndex, 0xf));

    obj = &level.objectives[objIndex];
    GScr_ClearObjectiveEntity(obj);

    Scr_GetVector(1, obj->origin);
    obj->origin[0] = (float)(int)obj->origin[0];
    obj->origin[1] = (float)(int)obj->origin[1];
    obj->origin[2] = (float)(int)obj->origin[2];
    return;
}

void GScr_positionWouldTelefrag(void)
{
    vec3_t vPos;
    vec3_t mins;
    vec3_t maxs;
    int touch[1024];
    int num;
    int i;

    Scr_GetVector(0, vPos);
    mins[0] = vPos[0] + playerMins[0];
    mins[1] = vPos[1] + playerMins[1];
    mins[2] = vPos[2] + playerMins[2];
    maxs[0] = vPos[0] + playerMaxs[0];
    maxs[1] = vPos[1] + playerMaxs[1];
    maxs[2] = vPos[2] + playerMaxs[2];

    num = CM_AreaEntities(mins, maxs, touch, 0x400, 0x2000000);
    for (i = 0; i < num; ++i) {
        gentity_t *ent = &g_entities[touch[i]];

        if (ent->client && ent->client->ps.pm_type <= 5) {
            Scr_AddInt(1);
            return;
        }
    }

    Scr_AddInt(0);
    return;
}

void GScr_GetBrushModelCenter(void)
{
    gentity_t *ent;
    vec3_t vCenter;

    ent = Scr_GetEntity(0);
    {
        const float *mn = ent->r.absmin;
        const float *mx = ent->r.absmax;
        vCenter[0] = (mn[0] + mx[0]) * 0.5f;
        vCenter[1] = (mn[1] + mx[1]) * 0.5f;
        vCenter[2] = (mn[2] + mx[2]) * 0.5f;
    }
    Scr_AddVector(vCenter);
}

void Scr_Objective_Delete(void)
{
    int objIndex = Scr_GetInt(0);
    objective_t *obj;

    if ((unsigned int)objIndex > 0xf)
        Scr_ParamError(0, va("index %i is an illegal objective index. Valid indexes are 0 to %i\n", objIndex, 0xf));

    obj = &level.objectives[objIndex];
    if (obj->entNum != 0x3ff) {
        gentity_t *ent = &g_entities[obj->entNum];
        if (ent->r.inuse)
            ent->r.svFlags &= ~0x10;
    }

    G_ResetObjective(obj);
}

void G_InitObjectives(void)
{
    int i;

    for (i = 0; i < 16; i++) {
        G_ResetObjective(&level.objectives[i]);
    }
}

void Scr_BulletTrace(void)
{
    vec3_t vStart;
    vec3_t vEnd;
    vec3_t endpos;
    vec3_t vNorm;
    trace_t trace;
    int iClipMask;
    int entityNum;
    const scr_const_t *scr_const = (const scr_const_t *)imp_scr_const;

    Scr_GetVector(0, vStart);
    Scr_GetVector(1, vEnd);
    iClipMask = Scr_GetInt(2) ? 0x02802831 : 0x00802831;

    if (Scr_GetType(3) == 1 && Scr_GetPointerType(3) == 0x15)
        entityNum = Scr_GetEntity(3)->s.number;
    else
        entityNum = 0x3ff;

    G_LocationalTrace(&trace, vStart, vEnd, entityNum, iClipMask, NULL);

    Scr_MakeArray();

    Scr_AddFloat(trace.fraction);
    Scr_AddArrayStringIndexed(scr_const->fraction);

    endpos[0] = vStart[0] + (vEnd[0] - vStart[0]) * trace.fraction;
    endpos[1] = vStart[1] + (vEnd[1] - vStart[1]) * trace.fraction;
    endpos[2] = vStart[2] + (vEnd[2] - vStart[2]) * trace.fraction;
    Scr_AddVector(endpos);
    Scr_AddArrayStringIndexed(scr_const->position);

    if (trace.entityNum == 0x3fe || trace.entityNum == 0x3ff)
        Scr_AddUndefined();
    else
        Scr_AddEntity(&g_entities[trace.entityNum]);
    Scr_AddArrayStringIndexed(scr_const->entity);

    if (trace.fraction < 1.0f) {
        Scr_AddVector(trace.normal);
        Scr_AddArrayStringIndexed(scr_const->normal);

        Scr_AddString(Com_SurfaceTypeToName((trace.surfaceFlags & 0x1f00000) >> 20));
        Scr_AddArrayStringIndexed(scr_const->surfacetype);
        return;
    }

    vNorm[0] = vEnd[0] - vStart[0];
    vNorm[1] = vEnd[1] - vStart[1];
    vNorm[2] = vEnd[2] - vStart[2];
    Vec3Normalize(vNorm);
    Scr_AddVector(vNorm);
    Scr_AddArrayStringIndexed(scr_const->normal);

    Scr_AddConstString(scr_const->none);
    Scr_AddArrayStringIndexed(scr_const->surfacetype);
    return;
}

void Scr_Objective_Add(void)
{
    int numParam = Scr_GetNumParam();
    int objIndex;
    unsigned int stateString;
    objectiveState_t state;
    objective_t *obj;

    if (numParam <= 1)
        Scr_Error("objective_add needs at least the first two parameters out of its parameter list of: index state [string] [position]\n");

    objIndex = Scr_GetInt(0);
    if ((unsigned int)objIndex > 0xf)
        Scr_ParamError(0, va("index %i is an illegal objective index. Valid indexes are 0 to %i\n", objIndex, 0xf));

    obj = &level.objectives[objIndex];
    GScr_ClearObjectiveEntity(obj);

    stateString = Scr_GetConstString(1);
    state = GScr_ObjectiveStateFromConst(stateString);
    if (state == OBJST_NUMSTATES)
        Scr_ParamError(1, va("Illegal objective state \"%s\". Valid states are \"empty\", \"invisible\", \"current\"\n", SL_ConvertToString(stateString)));

    obj->state = state;

    if (numParam > 2) {
        Scr_GetVector(2, obj->origin);
        obj->origin[0] = (float)(int)obj->origin[0];
        obj->origin[1] = (float)(int)obj->origin[1];
        obj->origin[2] = (float)(int)obj->origin[2];
        obj->entNum = 0x3ff;

        if (numParam > 3)
            SetObjectiveIcon(obj, 3);
    }

    obj->teamNum = 0;
    return;
}

void Scr_Objective_State(void)
{
    int objIndex = Scr_GetInt(0);
    unsigned int stateString;
    objectiveState_t state;
    objective_t *obj;

    if ((unsigned int)objIndex > 0xf)
        Scr_ParamError(0, va("index %i is an illegal objective index. Valid indexes are 0 to %i\n", objIndex, 0xf));

    obj = &level.objectives[objIndex];
    stateString = Scr_GetConstString(1);
    {
        scr_const_t *scr_const = (scr_const_t *)imp_scr_const;

        if (stateString == scr_const->empty) {
            state = OBJST_EMPTY;
        } else if (stateString == scr_const->invisible) {
            state = OBJST_INVISIBLE;
        } else if (stateString == scr_const->current) {
            state = OBJST_CURRENT;
        } else {
            Scr_ParamError(1, va("Illegal objective state \"%s\". Valid states are \"empty\", \"invisible\", \"current\"\n", Scr_GetString(1)));
            state = OBJST_EMPTY;
        }
    }

    obj->state = state;

    if (state == OBJST_EMPTY || state == OBJST_INVISIBLE)
        GScr_ClearObjectiveEntity(obj);

    return;
}

void Scr_MusicStop(void)
{
    int fadeTime;
    int numParams;

    numParams = Scr_GetNumParam();
    if (!numParams) {
        fadeTime = 0;
    } else if (numParams == 1) {
        fadeTime = GScr_SecondsToMilliseconds(Scr_GetFloat(0));
        if (fadeTime < 0)
            Scr_Error("musicStop: fade time must be >= 0\n");
    } else {
        Scr_Error("USAGE: musicStop([fadetime]);\n");
        return;
    }

    SV_GameSendServerCommand(-1, 1, va("%c %i", 'p', fadeTime));
    return;
}

void Scr_AmbientPlay(void)
{
    int iFadeTime;
    const char *pszAliasName;
    int numParams;

    numParams = Scr_GetNumParam();
    if (numParams == 1) {
        iFadeTime = 0;
    } else if (numParams == 2) {
        iFadeTime = GScr_SecondsToMilliseconds(Scr_GetFloat(1));
    } else {
        Scr_Error("USAGE: ambientPlay(alias_name, <fadetime>);\n");
        return;
    }

    pszAliasName = Scr_GetString(0);
    if (!pszAliasName[0])
        Scr_Error("ambientPlay: alias name cannot be the empty string... use stop or fade version\n");

    if (iFadeTime < 0)
        Scr_Error("ambientPlay: fade time must be >= 0\n");

    SV_SetConfigstring(3, va("n\\%s\\t\\%i", pszAliasName, iFadeTime + level.time));
    return;
}

void Scr_AmbientStop(void)
{
    int iFadeTime;
    int numParams;

    numParams = Scr_GetNumParam();
    if (!numParams) {
        iFadeTime = 0;
    } else if (numParams == 1) {
        iFadeTime = GScr_SecondsToMilliseconds(Scr_GetFloat(0));
        if (iFadeTime < 0)
            Scr_Error("ambientStop: fade time must be >= 0\n");
    } else {
        Scr_Error("USAGE: ambientStop(<fadetime>);\n");
        return;
    }

    SV_SetConfigstring(3, va("t\\%i", iFadeTime + level.time));
    return;
}

void Scr_PlayLoopedFX(void)
{
    int numParams;
    int fxId;
    int repeat;
    float cullDist;
    vec3_t fwd = { 0.0f, 0.0f, 0.0f };
    vec3_t up = { 0.0f, 0.0f, 0.0f };
    vec3_t pos;
    char fxName[0x400];
    gentity_t *ent;

    numParams = Scr_GetNumParam();
    if (numParams <= 2 || numParams > 6)
        Scr_Error("USAGE: playLoopedFx <effect id from loadFx> <repeat delay> <vector position of effect> <optional cull distance (0 = never cull)> <optional forward vector> <optional up vector>");

    fxId = Scr_GetInt(0);

    if (numParams == 6) {
        Scr_GetVector(5, up);
        if (Vec3Normalize(up) == 0.0f)
            Scr_Error(va("playLoopedFx called with (0 0 0) up direction (effect = %s)\n",
                         GScr_EffectName(fxId, fxName, sizeof(fxName))));
    }

    if (numParams == 5 || numParams == 6) {
        Scr_GetVector(4, fwd);
        if (Vec3Normalize(fwd) == 0.0f)
            Scr_Error(va("playLoopedFx called with (0 0 0) forward direction (effect = %s)\n",
                         GScr_EffectName(fxId, fxName, sizeof(fxName))));
    }

    if (numParams >= 4)
        cullDist = Scr_GetFloat(3);
    else
        cullDist = 0.0f;

    Scr_GetVector(2, pos);
    repeat = (int)floorf(Scr_GetFloat(1) * 1000.0f + 0.5f);
    if (repeat <= 0)
        Scr_Error(va("playLoopedFx called with %f s repeat (should be >= 0.0005s)\n", (double)repeat));

    ent = G_Spawn();
    ent->s.eType = 8;
    ent->r.svFlags |= 8;
    ent->s.scale = (byte)fxId;
    G_SetOrigin(ent, pos);

    ent->s.origin2[0] = fwd[0];
    ent->s.origin2[1] = fwd[1];
    ent->s.origin2[2] = fwd[2];
    ent->s.eventParm = DirToByte(up);
    ent->s.angles2[0] = cullDist;
    ent->s.angles2[1] = (float)repeat;

    SV_LinkEntity(ent);
    Scr_AddEntity(ent);
    return;
}

unsigned int Scr_ConstructMessageString(int firstParmIndex, int lastParmIndex, const char *errorContext, char *string, int stringLimit)
{
    int parmIndex;
    int stringLen;
    qboolean useLocalization;

    if (firstParmIndex > lastParmIndex) {
        string[0] = '\0';
        return 0;
    }

    useLocalization = 1;
    stringLen = 0;

    for (parmIndex = firstParmIndex; parmIndex <= lastParmIndex; ++parmIndex) {
        const char *token;
        int tokenLen;
        int type = Scr_GetType(parmIndex);

        if (type == 3) {
            int charIndex;

            token = Scr_GetIString(parmIndex);
            tokenLen = strlen(token);

            for (charIndex = 0; tokenLen > 1 && charIndex < tokenLen; ++charIndex) {
                int c = (signed char)token[charIndex];

                if (!GScr_MessageIsAlphaNumeric(c) && token[charIndex] != '_') {
                    Scr_ParamError(parmIndex, va("Illegal localized string reference: %s must contain only alpha-numeric characters and underscores", token));
                }
            }

            GScr_CheckMessageLength(parmIndex, errorContext, stringLimit, stringLen, tokenLen);
            if (stringLen)
                string[stringLen++] = 0x14;

            useLocalization = 1;
            GScr_AppendMessageToken(string, &stringLen, token, tokenLen);
            continue;
        }

        if (type == 1 && Scr_GetPointerType(parmIndex) == 0x15) {
            gentity_t *ent = Scr_GetEntity(parmIndex);

            if (!ent->client)
                Scr_ParamError(parmIndex, "Entity is not a player");

            token = va("%s^7", ent->client->sess.cs.name);
            tokenLen = strlen(token);
            GScr_CheckMessageLength(parmIndex, errorContext, stringLimit, stringLen, tokenLen);
        } else {
            int charIndex;

            token = Scr_GetString(parmIndex);
            tokenLen = strlen(token);

            for (charIndex = 0; charIndex < tokenLen; ++charIndex) {
                unsigned char raw = (unsigned char)token[charIndex];
                int c;

                if (GScr_MessageNeedsEscape(raw))
                    Scr_ParamError(parmIndex, va("bad escape character (%i) present in string", (signed char)raw));

                c = (signed char)token[charIndex];
                if (GScr_MessageIsAlpha(c)) {
                    const dvar_t *locWarningsAsErrors = *(const dvar_t **)imp_loc_warningsAsErrors;

                    if (locWarningsAsErrors->current.enabled) {
                        Com_Error(6, va("non-localized %s strings are not allowed to have letters in them: \"%s\"",
                                        errorContext, token));
                    } else {
                        Com_Printf("^3WARNING: Non-localized %s string is not allowed to have letters in it. Must be changed over to a localized string: \"%s\"\n",
                                   errorContext, token);
                    }

                    break;
                }
            }

            GScr_CheckMessageLength(parmIndex, errorContext, stringLimit, stringLen, tokenLen);
        }

        if (useLocalization) {
            string[stringLen++] = 0x15;
            useLocalization = 0;
        } else {
            useLocalization = 0;
        }

        GScr_AppendMessageToken(string, &stringLen, token, tokenLen);
    }

    string[stringLen] = '\0';
    return 0;
}

void GScr_MakeDvarServerInfo(void)
{
    const char *dvarName = Scr_GetString(0);
    dvar_t *dvar = Dvar_FindVar(dvarName);
    const char *dvarValue;
    char string[1024];
    char outString[1024];

    if (dvar) {
        Dvar_AddFlags(dvar, 0x100);
        return;
    }

    if (Scr_GetType(1) == 3) {
        Scr_ConstructMessageString(1, Scr_GetNumParam() - 1, "Dvar Value", string, sizeof(string));
        dvarValue = string;
    } else {
        dvarValue = Scr_GetString(1);
    }

    GScr_CleanDvarValue(dvarValue, outString, sizeof(outString));
    Dvar_RegisterString_mac(dvarName, dvarValue, 0x4100);
    return;
}

void GScr_ClientAnnouncement(void)
{
    gentity_t *ent = Scr_GetEntity(0);
    char string[0x400];
    int numParams = Scr_GetNumParam();

    Scr_ConstructMessageString(1, numParams - 1, "Announcement", string, sizeof(string));
    SV_GameSendServerCommand(ent->s.number, 0, va("%c \"%s\" 2", 'c', string));
}

void GScr_Announcement(void)
{
    char string[0x400];
    int numParams = Scr_GetNumParam();
    Scr_ConstructMessageString(0, numParams - 1, "Announcement", string, 0x400);
    SV_GameSendServerCommand(-1, 0, va("%c \"%s\" 2", 0x63, string));
}

void GScr_SetDvar(void)
{
    const char *dvarName = Scr_GetString(0);
    const char *dvarValue;
    char string[1024];
    char outString[1024];
    int addFlags;

    if (Scr_GetType(1) == 3) {
        Scr_ConstructMessageString(1, Scr_GetNumParam() - 1, "Dvar Value", string, sizeof(string));
        dvarValue = string;
    } else {
        dvarValue = Scr_GetString(1);
    }

    memset(outString, 0, sizeof(outString));
    GScr_CleanDvarValue(dvarValue, outString, sizeof(outString));

    if (!Dvar_IsValidName(dvarName)) {
        Scr_Error(va("Dvar %s has an invalid dvar name", dvarName));
        return;
    }

    addFlags = (Scr_GetNumParam() > 2 && Scr_GetInt(2)) ? 1 : 0;
    Dvar_SetFromStringByName(dvarName, dvarValue);

    if (addFlags)
        Dvar_AddFlags(Dvar_FindVar(dvarName), 0x400);

    return;
}

static inline __attribute__((always_inline)) void Scr_MakeGameMessage_core(int iClientNum, const char *pszCmd)
{
    char string[0x400];
    int numParams = Scr_GetNumParam();

    Scr_ConstructMessageString(0, numParams - 1, "Game Message", string, sizeof(string));
    SV_GameSendServerCommand(iClientNum, 0, va("%s \"%s\"", pszCmd, string));
}

void Scr_MakeGameMessage(int iClientNum, const char *pszCmd)
{
    Scr_MakeGameMessage_core(iClientNum, pszCmd);
}

static inline __attribute__((always_inline)) qboolean Scr_IsValidGameType_core(const char *pszGameType)
{
    int i;

    for (i = 0; i < g_scr_data.gametype.iNumGameTypes; i++) {
        if (!I_stricmp(g_scr_data.gametype.list[i].pszScript, pszGameType))
            return 1;
    }

    return 0;
}

void GScr_IsValidGameType(void)
{
    if (Scr_GetNumParam())
        Scr_AddInt(Scr_IsValidGameType_core(Scr_GetString(0)));
    return;
}

qboolean Scr_IsValidGameType(const char *pszGameType)
{
    return Scr_IsValidGameType_core(pszGameType);
}

void Scr_Length(void)
{
    float v[3];

    Scr_GetVector(0, v);
    Scr_AddFloat(sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]));
}

void println(void)
{
    int num;
    int i;

    if ((*(const dvar_t **)imp_g_NoScriptSpam)->current.enabled)
        return;

    num = Scr_GetNumParam();
    for (i = 0; i < num; i++)
        Com_Printf("%s", Scr_GetDebugString(i));

    Com_Printf("\n");
    return;
}

void Scr_PlayerDamage(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int dflags, int meansOfDeath, int iWeapon, const vec_t *vPoint, const vec_t *vDir, const hitLocation_t hitLoc, int timeOffset)
{
    unsigned int threadId;

    Scr_AddInt(timeOffset);
    Scr_AddConstString(G_GetHitLocationString(hitLoc));
    GScr_AddOptionalVector(vDir);
    GScr_AddOptionalVector(vPoint);
    GScr_AddWeaponName(iWeapon);
    GScr_AddMeansOfDeath(meansOfDeath);
    Scr_AddInt(dflags);
    Scr_AddInt(damage);
    GScr_AddOptionalEntity(attacker);
    GScr_AddOptionalEntity(inflictor);

    threadId = Scr_ExecEntThread(self, g_scr_data.gametype.playerdamage, 10);
    Scr_FreeThread(threadId & 0xffff);
    return;
}

void Scr_PlayerKilled(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath, int iWeapon, const vec_t *vDir, const hitLocation_t hitLoc, int psTimeOffset, int deathAnimDuration)
{
    unsigned int threadId;

    Scr_AddInt(deathAnimDuration);
    Scr_AddInt(psTimeOffset);
    Scr_AddConstString(G_GetHitLocationString(hitLoc));
    GScr_AddOptionalVector(vDir);
    GScr_AddWeaponName(iWeapon);
    GScr_AddMeansOfDeath(meansOfDeath);
    Scr_AddInt(damage);
    GScr_AddOptionalEntity(attacker);
    GScr_AddOptionalEntity(inflictor);

    threadId = Scr_ExecEntThread(self, g_scr_data.gametype.playerkilled, 9);
    Scr_FreeThread(threadId & 0xffff);
    return;
}

void ScrCmd_Show(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);

    ent->flags &= ~0x800;
    ent->r.clientMask[0] = 0;
    ent->r.clientMask[1] = 0;
}

void ScrCmd_Hide(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);

    ent->flags |= 0x800;
    ent->r.clientMask[0] = -1;
    ent->r.clientMask[1] = -1;
}

void ScrCmd_ShowToPlayer(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    gentity_t *clientEnt = Scr_GetEntity(0);

    if (clientEnt->s.number > 63)
        { (void)(Scr_Error("showToClient error: param must be a client entity\n")); return; }

    ent->flags &= ~0x800;
    ent->r.clientMask[clientEnt->s.number >> 5] &= ~(1u << (clientEnt->s.number & 31));
}

void GScr_EnableGrenadeTouchDamage(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    scr_const_t *scr_const = (scr_const_t *)imp_scr_const;

    if (ent->classname != scr_const->trigger_damage)
        Scr_Error("Currently on supported on damage triggers");
    ent->flags |= 0x4000;
}

void GScr_DisableGrenadeTouchDamage(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    scr_const_t *scr_const = (scr_const_t *)imp_scr_const;

    if (ent->classname != scr_const->trigger_damage)
        Scr_Error("Currently on supported on damage triggers");
    ent->flags &= ~0x4000;
}

void GScr_EnableGrenadeBounce(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);

    ent->flags &= ~0x8000;
}

void GScr_DisableGrenadeBounce(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);

    ent->flags |= 0x8000;
}

void GScr_EnableAimAssist(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);

    if (!ent->r.bmodel)
        Scr_Error("Currently only supported on entities with brush models");
    ent->s.eFlags |= 0x800;
}

void GScr_DisableAimAssist(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);

    if (!ent->r.bmodel)
        Scr_Error("Currently only supported on entities with brush models");
    ent->s.eFlags &= ~0x800;
}

void ScrCmd_attach(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    const scr_const_t *scr_const = (const scr_const_t *)imp_scr_const;
    const char *modelName = Scr_GetString(0);
    unsigned int tagName;
    qboolean ignoreCollision;
    int numParam = Scr_GetNumParam();

    if (numParam > 1)
        tagName = Scr_GetConstLowercaseString(1);
    else
        tagName = scr_const->_;

    if (numParam > 2)
        ignoreCollision = Scr_GetInt(2);
    else
        ignoreCollision = 0;

    if (G_EntDetach(ent, modelName, tagName))
        Scr_Error(va("model '%s' already attached to tag '%s'", modelName, SL_ConvertToString(tagName)));

    if (!G_EntAttach(ent, modelName, tagName, ignoreCollision))
        Scr_Error("maximum attached models exceeded");

    return;
}

void ScrCmd_detach(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    const scr_const_t *scr_const = (const scr_const_t *)imp_scr_const;
    const char *modelName = Scr_GetString(0);
    unsigned int tagName;
    int i;

    if (Scr_GetNumParam() > 1)
        tagName = Scr_GetConstLowercaseString(1);
    else
        tagName = scr_const->_;

    if (G_EntDetach(ent, modelName, tagName))
        return;

    Com_Printf("Current attachments:\n");
    for (i = 0; i < 7; i++) {
        if (ent->attachModelNames[i] && ent->attachTagNames[i]) {
            Com_Printf("model: '%s', tag: '%s'\n",
                       G_ModelName(ent->attachModelNames[i]),
                       SL_ConvertToString(ent->attachTagNames[i]));
        }
    }

    Scr_Error(va("failed to detach model '%s' from tag '%s'", modelName, SL_ConvertToString(tagName)));
    return;
}

void ScrCmd_detachAll(scr_entref_t entref)
{
    { (void)(G_EntDetachAll(GScr_EntityFromEntRef(entref))); return; }
}

void ScrCmd_GetAttachSize(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    int i;

    for (i = 0; i < 7; i++) {
        if (!ent->attachModelNames[i])
            break;
    }

    { (void)(Scr_AddInt(i)); return; }
}

void ScrCmd_GetAttachModelName(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    int i = Scr_GetInt(0);

    if ((unsigned int)i > 6 || !ent->attachModelNames[i])
        Scr_ParamError(0, "bad index");

    { (void)(Scr_AddString(G_ModelName(ent->attachModelNames[i]))); return; }
}

void ScrCmd_GetAttachTagName(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    int i = Scr_GetInt(0);

    if ((unsigned int)i > 6 || !ent->attachModelNames[i])
        Scr_ParamError(0, "bad index");

    { (void)(Scr_AddConstString(ent->attachTagNames[i])); return; }
}

void ScrCmd_GetAttachIgnoreCollision(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    int i = Scr_GetInt(0);

    if ((unsigned int)i > 6 || !ent->attachModelNames[i])
        Scr_ParamError(0, "bad index");

    { (void)(Scr_AddBool((ent->attachIgnoreCollision >> i) & 1)); return; }
}

void ScrCmd_LinkTo(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    gentity_t *parent;
    unsigned int tagName = 0;
    int numParam;
    qboolean linked;
    vec3_t originOffset;
    vec3_t anglesOffset;

    if (Scr_GetType(0) != 1 || Scr_GetPointerType(0) != 0x15)
        Scr_ParamError(0, "not an entity");

    if (!(ent->flags & 0x1000))
        Scr_ObjectError(va("entity (classname: '%s') does not currently support linkTo",
                           SL_ConvertToString(ent->classname)));

    parent = Scr_GetEntity(0);
    numParam = Scr_GetNumParam();

    if (numParam > 1) {
        tagName = Scr_GetConstLowercaseString(1);
        if (!SL_ConvertToString(tagName)[0])
            tagName = 0;
    }

    if (numParam > 2) {
        Scr_GetVector(2, originOffset);
        Scr_GetVector(3, anglesOffset);
        linked = G_EntLinkToWithOffset(ent, parent, tagName, originOffset, anglesOffset);
    } else {
        linked = G_EntLinkTo(ent, parent, tagName);
    }

    if (linked)
        return;

    if (!SV_DObjExists(parent)) {
        if (!parent->model)
            Scr_Error("failed to link entity since parent has no model");

        Scr_Error(va("failed to link entity since parent model '%s' is invalid",
                     G_ModelName(parent->model)));
    }

    if (tagName && SV_DObjGetBoneIndex(parent, tagName) < 0) {
        SV_DObjDumpInfo(parent);
        Scr_Error(va("failed to link entity since tag '%s' does not exist in parent model '%s'",
                     SL_ConvertToString(tagName), G_ModelName(parent->model)));
    }

    Scr_Error("failed to link entity due to link cycle");
    return;
}

void ScrCmd_Unlink(scr_entref_t entref)
{
    { (void)(G_EntUnlink(GScr_EntityFromEntRef(entref))); return; }
}

void ScrCmd_EnableLinkTo(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);

    if (ent->flags & 0x1000)
        Scr_ObjectError("entity already has linkTo enabled");

    if (ent->s.eType || ent->physicsObject)
        Scr_ObjectError(va("entity (classname: '%s') does not currently support enableLinkTo",
                           SL_ConvertToString(ent->classname)));

    ent->flags |= 0x1000;
    return;
}

void ScrCmd_GetOrigin(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    const float *cur = ent->r.currentOrigin;
    vec3_t origin;

    origin[0] = cur[0];
    origin[1] = cur[1];
    origin[2] = cur[2];
    Scr_AddVector(origin);
}

void ScrCmd_GetEye(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    const float *cur = ent->r.currentOrigin;
    vec3_t eye;

    eye[0] = cur[0];
    eye[1] = cur[1];
    eye[2] = cur[2] + 40.0f;
    Scr_AddVector(eye);
    return;
}

void ScrCmd_UseBy(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    gentity_t *other = Scr_GetEntity(0);

    Scr_AddEntity(other);
    Scr_Notify(ent, ((const scr_const_t *)imp_scr_const)->trigger, 1);

    if ((*(entityHandler_t **)imp_entityHandlers)[ent->handler].use) {
        void (*use)(gentity_t *, gentity_t *, gentity_t *) =
            (void (*)(gentity_t *, gentity_t *, gentity_t *))(*(entityHandler_t **)imp_entityHandlers)[ent->handler].use;
        use(ent, other, other);
    }

    return;
}

void Scr_SetStableMissile(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    int stableMissile = Scr_GetInt(0);

    if (ent->s.eType != 1)
        Scr_Error("Type should be a player");

    if (stableMissile)
        ent->flags |= 0x20000;
    else
        ent->flags &= ~0x20000;
    return;
}

void ScrCmd_IsTouching(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    gentity_t *other = Scr_GetEntity(0);
    gentity_t *boundsEnt;
    gentity_t *contactEnt;
    vec3_t mins;
    vec3_t maxs;
    qboolean entSpecial;
    qboolean otherSpecial;

    entSpecial = ent->r.bmodel || (ent->r.svFlags & 0x60);
    otherSpecial = other->r.bmodel || (other->r.svFlags & 0x60);

    if (entSpecial) {
        if (otherSpecial)
            Scr_Error("istouching cannot be called on 2 brush/cylinder entities");
        boundsEnt = other;
        contactEnt = ent;
    } else {
        boundsEnt = ent;
        contactEnt = other;
    }

    mins[0] = boundsEnt->r.currentOrigin[0] + boundsEnt->r.mins[0];
    mins[1] = boundsEnt->r.currentOrigin[1] + boundsEnt->r.mins[1];
    mins[2] = boundsEnt->r.currentOrigin[2] + boundsEnt->r.mins[2];

    maxs[0] = boundsEnt->r.currentOrigin[0] + boundsEnt->r.maxs[0];
    maxs[1] = boundsEnt->r.currentOrigin[1] + boundsEnt->r.maxs[1];
    maxs[2] = boundsEnt->r.currentOrigin[2] + boundsEnt->r.maxs[2];

    ExpandBoundsToWidth(mins, maxs);
    Scr_AddInt(SV_EntityContact(mins, maxs, contactEnt));
    return;
}

void ScrCmd_PlaySound(scr_entref_t entref)
{
    ScrCmd_PlaySoundEvent(GScr_EntityFromEntRef(entref), 0xb3);
}

void ScrCmd_PlaySoundAsMaster(scr_entref_t entref)
{
    ScrCmd_PlaySoundEvent(GScr_EntityFromEntRef(entref), 0xb4);
}

void ScrCmd_PlayLoopSound(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);

    ent->r.broadcastTime = -1;
    ent->s.loopSound = G_SoundAliasIndex(Scr_GetString(0));
    return;
}

void ScrCmd_StopLoopSound(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);

    ent->r.broadcastTime = level.time + 300;
    ent->s.loopSound = 0;
}

void ScrCmd_Delete(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);

    if (ent->client)
        Scr_Error("Cannot delete a client entity");

    if (level.currentEntityThink == ent->s.number)
        Scr_Error("Cannot delete entity during its think");

    Scr_Notify(ent, scr_const.death, 0);
    { (void)(G_FreeEntity(ent)); return; }
}

void ScrCmd_SetModel(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);

    G_SetModel(ent, Scr_GetString(0));
    G_DObjUpdate(ent);
    SV_LinkEntity(ent);
    return;
}

void ScrCmd_GetNormalHealth(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    float normalHealth;

    if (ent->client) {
        if (ent->health)
            normalHealth = (float)ent->health / (float)ent->client->sess.maxHealth;
        else
            normalHealth = 0.0f;
    } else {
        normalHealth = (float)ent->health;
    }

    { (void)(Scr_AddFloat(normalHealth)); return; }
}

void ScrCmd_SetNormalHealth(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    float normalHealth = Scr_GetFloat(0);
    int newHealth;

    if (normalHealth > 1.0f)
        normalHealth = 1.0f;

    if (ent->client) {
        newHealth = (int)floorf((float)ent->client->sess.maxHealth * normalHealth + 0.5f);
        SV_GameSendServerCommand(ent - g_entities, 0, va("%c \"%i\"", 0x49, 0));
    } else if (ent->maxHealth) {
        newHealth = (int)((float)ent->maxHealth * normalHealth);
    } else {
        newHealth = (int)normalHealth;
    }

    if (newHealth <= 0) {
        Com_Printf("ERROR: Cannot setnormalhealth to 0 or below.\n");
        return;
    }

    ent->health = newHealth;
    return;
}

void ScrCmd_SetContents(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    int contents = Scr_GetInt(0);

    Scr_AddInt(ent->r.contents);
    ent->r.contents = contents;
    SV_LinkEntity(ent);
    return;
}

void GScr_SetCursorHint(scr_entref_t entref)
{
    gentity_t *pEnt = GScr_EntityFromEntRef(entref);
    const char *pszHint = Scr_GetString(0);
    const char **hintStrings = (const char **)imp_hintStrings;
    int i;

    if (GScr_IsUseTrigger(pEnt) && I_stricmp(pszHint, "HINT_INHERIT") == 0) {

        ((gentity_t *)pEnt)->s.dmgFlags = -1;
        return;
    }

    for (i = 1; hintStrings[i]; ++i) {
        if (I_stricmp(pszHint, hintStrings[i]) == 0) {

            ((gentity_t *)pEnt)->s.dmgFlags = i;
            return;
        }
        if (i + 1 == 0x85)
            break;
    }

    Com_Printf("List of valid hint type strings\n");
    if (GScr_IsUseTrigger(pEnt))
        Com_Printf("HINT_INHERIT (for trigger_use or trigger_use_touch entities only)\n");

    for (i = 1; i != 0x85 && hintStrings[i]; ++i)
        Com_Printf("%s\n", hintStrings[i]);

    Scr_Error(va("%s is not a valid hint type. See above for list of valid hint types\n", pszHint));
    return;
}

void GScr_SetHintString(scr_entref_t entref)
{
    gentity_t *pEnt = GScr_EntityFromEntRef(entref);
    char szConfigString[1024];
    char szHint[1024];
    int i;

    if (!GScr_IsUseTrigger(pEnt))
        Scr_Error("The setHintString command only works on trigger_use or trigger_use_touch entities.\n");

    if (Scr_GetType(0) == 2 && I_stricmp(Scr_GetString(0), "") == 0) {

        ((gentity_t *)pEnt)->s.scale = 0xff;
        return;
    }

    Scr_ConstructMessageString(0, Scr_GetNumParam() - 1, "Hint String", szHint, sizeof(szHint));

    for (i = 0; i < 0x20; ++i) {
        SV_GetConfigstring(0x4fe + i, szConfigString, sizeof(szConfigString));
        if (szConfigString[0] == '\0') {
            SV_SetConfigstring(0x4fe + i, szHint);
            break;
        }

        if (strcmp(szHint, szConfigString) == 0)
            break;
    }

    if (i == 0x20) {
        Scr_Error(va("Too many different hintstring values. Max allowed is %i different strings", 0x20));
        i = -1;
    }

    ((gentity_t *)pEnt)->s.scale = (unsigned char)i;
    return;
}

static __attribute_regparm__(2) gentity_t *GetPlayerEntity(scr_entref_t entref)
{
    gentity_t *ent;
    const char *targetname;

    if (entref.classnum) {
        Scr_ObjectError("not an entity");
        return NULL;
    }

    ent = &g_entities[entref.entnum];
    if (ent->client)
        return ent;

    if (ent->targetname)
        targetname = SL_ConvertToString(ent->targetname);
    else
        targetname = "<undefined>";

    Scr_Error(va("only valid on players; called on entity %i at %.0f %.0f %.0f classname %s targetname %s\n",
                 entref.entnum,
                 ent->r.currentOrigin[0],
                 ent->r.currentOrigin[1],
                 ent->r.currentOrigin[2],
                 SL_ConvertToString(ent->classname),
                 targetname));
    return ent;
}

void GScr_ViewKick(scr_entref_t entref)
{
    gentity_t *ent = GetPlayerEntity(entref);
    gclient_t *client;
    vec3_t origin;

    if (Scr_GetNumParam() != 2)
        Scr_Error("USAGE: <player> viewkick <force 0-127> <source position>\n");

    client = ent->client;
    client->damage_blood = (Scr_GetInt(0) * ent->maxHealth + 50) / 100;
    if (client->damage_blood < 0)
        Scr_Error(va("viewkick: damage %g < 0\n", Scr_GetFloat(0)));

    Scr_GetVector(1, origin);
    client->damage_from[0] = client->ps.origin[0] - origin[0];
    client->damage_from[1] = client->ps.origin[1] - origin[1];
    client->damage_from[2] = client->ps.origin[2] - origin[2];
    return;
}

void GScr_StopShellShock(scr_entref_t entref)
{
    gentity_t *ent = GetPlayerEntity(entref);

    if (Scr_GetNumParam() != 0)
        Scr_Error("USAGE: <player> stopshellshock()\n");

    ent->client->ps.shellshockIndex = 0;
    ent->client->ps.shellshockTime = 0;
    ent->client->ps.shellshockDuration = 0;
    return;
}

void GScr_ShellShock(scr_entref_t entref)
{
    gentity_t *ent = GetPlayerEntity(entref);
    const char *shock;
    char configString[1024];
    int id;
    int duration;

    if (Scr_GetNumParam() != 2)
        Scr_Error("USAGE: <player> shellshock(<shellshockname>, <duration>)\n");

    shock = Scr_GetString(0);
    for (id = 1; id != 16; ++id) {
        SV_GetConfigstring(id + 0x48e, configString, sizeof(configString));
        if (!stricmp(configString, shock))
            break;
    }

    if (id == 16) {
        Scr_Error(va("shellshock '%s' was not precached\n", shock));
        return;
    }

    duration = (int)floorf(Scr_GetFloat(1) * 1000.0f + 0.5f);
    if ((unsigned int)duration > 60000u)
        Scr_ParamError(1, va("duration %g should be >= 0 and <= 60", duration * 0.001f));

    ent->client->ps.shellshockIndex = id;
    ent->client->ps.shellshockTime = level.time;
    ent->client->ps.shellshockDuration = duration;

    if (ent->health > 0) {
        *(bgs_t **)imp_bgs = (bgs_t *)imp_level_bgs;
        BG_AnimScriptEvent(&ent->client->ps, 0x12, 0, 1);
    }

    return;
}

void GScr_GetAmmoCount(scr_entref_t entref)
{
    gentity_t *ent = GetPlayerEntity(entref);
    int weaponIndex;

    weaponIndex = G_GetWeaponIndexForName(Scr_GetString(0));
    if (!weaponIndex)
        { (void)(Scr_AddInt(0)); return; }
    { (void)(Scr_AddInt(BG_WeaponAmmo(&ent->client->ps, weaponIndex))); return; }
}

void GScr_LocalToWorldCoords(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    vec3_t vLocal;
    vec3_t vWorld;
    vec3_t axis[3];

    Scr_GetVector(0, vLocal);
    AnglesToAxis(ent->r.currentAngles, axis);
    MatrixTransformVector(vLocal, axis, vWorld);

    {
        const float *cur = ent->r.currentOrigin;
        vWorld[0] += cur[0];
        vWorld[1] += cur[1];
        vWorld[2] += cur[2];
    }
    Scr_AddVector(vWorld);
    return;
}

void GScr_SetRightArc(scr_entref_t entref)
{
    turretInfo_t *pTurretInfo = GScr_GetTurretInfo(GScr_EntityFromEntRef(entref));

    pTurretInfo->arcmin[1] = -Scr_GetFloat(0);
    if (pTurretInfo->arcmin[1] > 0.0f)
        pTurretInfo->arcmin[1] = 0.0f;
}

void GScr_SetLeftArc(scr_entref_t entref)
{
    turretInfo_t *pTurretInfo = GScr_GetTurretInfo(GScr_EntityFromEntRef(entref));

    pTurretInfo->arcmax[1] = Scr_GetFloat(0);
    if (pTurretInfo->arcmax[1] < 0.0f)
        pTurretInfo->arcmax[1] = 0.0f;
}

void GScr_SetTopArc(scr_entref_t entref)
{
    turretInfo_t *pTurretInfo = GScr_GetTurretInfo(GScr_EntityFromEntRef(entref));

    pTurretInfo->arcmin[0] = -Scr_GetFloat(0);
    if (pTurretInfo->arcmin[0] > 0.0f)
        pTurretInfo->arcmin[0] = 0.0f;
}

void GScr_SetBottomArc(scr_entref_t entref)
{
    turretInfo_t *pTurretInfo = GScr_GetTurretInfo(GScr_EntityFromEntRef(entref));

    pTurretInfo->arcmax[0] = Scr_GetFloat(0);
    if (pTurretInfo->arcmax[0] < 0.0f)
        pTurretInfo->arcmax[0] = 0.0f;
}

void GScr_GetEntityNumber(scr_entref_t entref)
{
    { (void)(Scr_AddInt(GScr_EntityFromEntRef(entref)->s.number)); return; }
}

void GScr_PlaceSpawnPoint(scr_entref_t entref)
{
    gentity_t *pEnt = GScr_EntityFromEntRef(entref);
    vec3_t vStart;
    vec3_t vEnd;
    trace_t trace;

    vStart[0] = pEnt->r.currentOrigin[0];
    vStart[1] = pEnt->r.currentOrigin[1];
    vStart[2] = pEnt->r.currentOrigin[2];

    vEnd[0] = pEnt->r.currentOrigin[0];
    vEnd[1] = pEnt->r.currentOrigin[1];
    vEnd[2] = pEnt->r.currentOrigin[2] + 128.0f;

    G_TraceCapsule(&trace, vStart, playerMins, playerMaxs, vEnd, pEnt->s.number, 0x2810011);
    vStart[0] = vStart[0] + (vEnd[0] - vStart[0]) * trace.fraction;
    vStart[1] = vStart[1] + (vEnd[1] - vStart[1]) * trace.fraction;
    vStart[2] = vStart[2] + (vEnd[2] - vStart[2]) * trace.fraction;

    vEnd[0] = vStart[0];
    vEnd[1] = vStart[1];
    vEnd[2] = vStart[2] - 262144.0f;

    G_TraceCapsule(&trace, vStart, playerMins, playerMaxs, vEnd, pEnt->s.number, 0x2810011);
    pEnt->s.groundEntityNum = trace.entityNum;

    vStart[0] = vStart[0] + (vEnd[0] - vStart[0]) * trace.fraction;
    vStart[1] = vStart[1] + (vEnd[1] - vStart[1]) * trace.fraction;
    vStart[2] = vStart[2] + (vEnd[2] - vStart[2]) * trace.fraction;

    G_TraceCapsule(&trace, vStart, playerMins, playerMaxs, vStart, pEnt->s.number, 0x2810011);
    if (trace.startsolid) {
        Com_Printf("WARNING: Spawn point entity %i is in solid at (%i, %i, %i)\n",
                   pEnt->s.number,
                   (int)pEnt->r.currentOrigin[0],
                   (int)pEnt->r.currentOrigin[1],
                   (int)pEnt->r.currentOrigin[2]);
    }

    G_SetOrigin(pEnt, vStart);
    return;
}

void GScr_UpdateScores(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    int clientNum = ent - g_entities;

    SV_GameSendServerCommand(clientNum, 0, va("%c %i", 0x48, level.teamScores[TEAM_ALLIES]));
    SV_GameSendServerCommand(clientNum, 0, va("%c %i", 0x47, level.teamScores[TEAM_AXIS]));
    return;
}

void GScr_SetTeamForTrigger(scr_entref_t entref)
{
    gentity_t *ent = GScr_EntityFromEntRef(entref);
    scr_string_t team;

    if (!GScr_IsUseTrigger(ent))
        GScr_UseTriggerError("setteamfortrigger: trigger entity must be of type %s or %s");

    team = Scr_GetConstString(0);
    if (team == scr_const.allies) {
        ent->team = 2;
        return;
    }

    if (team == scr_const.axis) {
        ent->team = 1;
        return;
    }

    if (team == scr_const.none) {
        ent->team = 0;
        return;
    }

    {
        const char *noneString = SL_ConvertToString(scr_const.none);
        const char *axisString = SL_ConvertToString(scr_const.axis);
        const char *alliesString = SL_ConvertToString(scr_const.allies);

        Scr_Error(va("setteamfortrigger: invalid team used must be %s, %s or %s",
                     alliesString, axisString, noneString));
    }
}

void GScr_ClientClaimTrigger(scr_entref_t entref)
{
    gentity_t *clientEnt = GScr_EntityFromEntRef(entref);
    gentity_t *triggerEnt;
    int clientNum;

    if (!clientEnt->client)
        Scr_Error("clientclaimtrigger: claimer must be a client.");

    triggerEnt = Scr_GetEntity(0);
    if (!GScr_IsUseTrigger(triggerEnt))
        GScr_UseTriggerError("clientclaimtrigger: trigger entity must be of type %s or %s");

    clientNum = clientEnt->client->ps.clientNum;
    if (triggerEnt->trigger.singleUserEntIndex == 0x3ff ||
        triggerEnt->trigger.singleUserEntIndex == clientNum) {
        triggerEnt->trigger.singleUserEntIndex = clientNum;
    }
}

void GScr_ClientReleaseTrigger(scr_entref_t entref)
{
    gentity_t *clientEnt = GScr_EntityFromEntRef(entref);
    gentity_t *triggerEnt;

    if (!clientEnt->client)
        Scr_Error("clientreleasetrigger: releaser must be a client.");

    triggerEnt = Scr_GetEntity(0);
    if (!GScr_IsUseTrigger(triggerEnt))
        GScr_UseTriggerError("clientreleasetrigger: trigger entity must be of type %s or %s");

    if (triggerEnt->trigger.singleUserEntIndex == clientEnt->client->ps.clientNum)
        triggerEnt->trigger.singleUserEntIndex = 0x3ff;
}

void Scr_PlayFX(void)
{
    int numParams;
    int fxId;
    vec3_t pos;
    vec3_t axis[3];
    float crossLen;
    char fxName[0x400];
    gentity_t *ent;

    numParams = Scr_GetNumParam();
    if ((unsigned int)(numParams - 2) > 2)
        Scr_Error("Incorrect number of parameters");

    fxId = Scr_GetInt(0);
    Scr_GetVector(1, pos);
    ent = G_TempEntity(pos, 0xc2);
    ent->s.eventParm = (byte)fxId;

    if (numParams == 2) {
        ent->s.apos.trBase[0] = -90.0f;
        return;
    }

    Scr_GetVector(2, axis[0]);
    if (Vec3Normalize(axis[0]) == 0.0f)
        Scr_Error(va("%s (effect = %s)\n",
                     "playFx called with (0 0 0) forward direction",
                     GScr_EffectName(fxId, fxName, sizeof(fxName))));

    if (numParams == 3) {
        vectoangles(axis[0], ent->s.apos.trBase);
        return;
    }

    Scr_GetVector(3, axis[2]);
    if (Vec3Normalize(axis[2]) == 0.0f)
        Scr_Error(va("%s (effect = %s)\n",
                     "playFx called with (0 0 0) up direction",
                     GScr_EffectName(fxId, fxName, sizeof(fxName))));

    Vec3Cross(axis[2], axis[0], axis[1]);
    crossLen = Vec3Normalize(axis[1]);
    if (crossLen < 0.001f)
        Scr_Error(va("%s (effect = %s)\n",
                     "playFx called an up direction 0 or 180 degrees from forward",
                     GScr_EffectName(fxId, fxName, sizeof(fxName))));

    if (crossLen < 0.999f)
        Vec3Cross(axis[0], axis[1], axis[2]);

    AxisToAngles((const vec_t *)axis, ent->s.apos.trBase);
    return;
}

void iprintln(void)
{
    Scr_MakeGameMessage_core(-1, "f");
}

void iprintlnbold(void)
{
    Scr_MakeGameMessage_core(-1, "g");
}

static inline __attribute__((always_inline)) scr_func_t GScr_LoadScriptFunction(const char *script, const char *label)
{
    extern unsigned int Scr_LoadScript(const char *filename);
    extern scr_func_t Scr_GetFunctionHandle(const char *filename, const char *name);
    scr_func_t func;

    if (!Scr_LoadScript(script))
        Com_Error(1, "\x15"
                     "Could not find script '%s'",
                  script);

    func = Scr_GetFunctionHandle(script, label);
    if (!func)
        Com_Error(1, "\x15"
                     "Could not find label '%s' in script '%s'",
                  label, script);

    return func;
}

unsigned int GScr_LoadGameTypeScript(void)
{
    extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
    const dvar_t *g_gametype = *(const dvar_t **)imp_g_gametype;
    const char *callbackSetup = "maps/mp/gametypes/_callbacksetup";
    char filename[64];

    Com_sprintf(filename, sizeof(filename), "maps/mp/gametypes/%s", g_gametype->current.string);

    g_scr_data.gametype.main = GScr_LoadScriptFunction(filename, "main");
    g_scr_data.gametype.startupgametype = GScr_LoadScriptFunction(callbackSetup, "CodeCallback_StartGameType");
    g_scr_data.gametype.playerconnect = GScr_LoadScriptFunction(callbackSetup, "CodeCallback_PlayerConnect");
    g_scr_data.gametype.playerdisconnect = GScr_LoadScriptFunction(callbackSetup, "CodeCallback_PlayerDisconnect");
    g_scr_data.gametype.playerdamage = GScr_LoadScriptFunction(callbackSetup, "CodeCallback_PlayerDamage");
    return (g_scr_data.gametype.playerkilled = GScr_LoadScriptFunction(callbackSetup, "CodeCallback_PlayerKilled"));
}

void GScr_LoadScripts(int inst)
{
    extern scr_func_t Scr_GetFunctionHandle(const char *file, const char *func);
    extern unsigned int Scr_LoadScript(const char *filename);
    extern void Scr_SetClassMap(int classnum);
    extern void GScr_AddFieldsForEntity(void);
    extern void GScr_AddFieldsForHudElems(void);
    extern void GScr_AddFieldsForRadiant(void);
    extern const dvar_t *mapname;
    extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
    int i;
    char s[64];

    // These engine entry points are required even when no GSC imports them.
    // Executing a missing handle (zero) enters the bytecode buffer sentinel.
    g_scr_data.delete_ = GScr_LoadScriptFunction("codescripts/delete", "main");
    g_scr_data.initstructs = GScr_LoadScriptFunction("codescripts/struct", "initstructs");
    g_scr_data.createstruct = GScr_LoadScriptFunction("codescripts/struct", "createstruct");

    GScr_LoadGameTypeScript();

    {
        extern const dvar_t *sv_mapname;
        if (sv_mapname && sv_mapname->current.string && sv_mapname->current.string[0]) {
            Com_sprintf(s, sizeof(s), "maps/mp/%s", sv_mapname->current.string);
            Scr_LoadScript(s);
            g_scr_data.levelscript = Scr_GetFunctionHandle(s, "main");
        }
    }

    for (i = 0; i <= 3; i++)
        Scr_SetClassMap(i);
    GScr_AddFieldsForEntity();
    GScr_AddFieldsForHudElems();
    GScr_AddFieldsForRadiant();
    return;
}

/* functions: script builtin dispatch table (was the ILP32 data blob symbol
 * functions). Typed BuiltinFunctionDef[] so it re-lays-out per target. Placed at
 * end-of-file so the in-file builtin prototypes are in scope; only the few not
 * declared in this TU are forward-declared here. (x64 port Stage 2.) */
void GScr_NewClientHudElem();
void GScr_NewHudElem();
void GScr_NewTeamHudElem();
void Scr_AddStruct();
void Scr_GetEnt();
void Scr_GetEntArray();
void Scr_ResetTimeout();
BuiltinFunctionDef functions[145] = {
    { "print", (BuiltinFunction)print, 1 },
    { "println", (BuiltinFunction)println, 1 },
    { "iprintln", (BuiltinFunction)iprintln, 0 },
    { "iprintlnbold", (BuiltinFunction)iprintlnbold, 0 },
    { "print3d", (BuiltinFunction)GScr_print3d, 1 },
    { "line", (BuiltinFunction)GScr_line, 1 },
    { "getent", (BuiltinFunction)Scr_GetEnt, 0 },
    { "getentarray", (BuiltinFunction)Scr_GetEntArray, 0 },
    { "spawn", (BuiltinFunction)GScr_Spawn, 0 },
    { "spawnturret", (BuiltinFunction)GScr_SpawnTurret, 0 },
    { "precacheturret", (BuiltinFunction)GScr_PrecacheTurret, 0 },
    { "spawnstruct", (BuiltinFunction)Scr_AddStruct, 0 },
    { "assert", (BuiltinFunction)assertCmd, 1 },
    { "assertex", (BuiltinFunction)assertexCmd, 1 },
    { "assertmsg", (BuiltinFunction)assertmsgCmd, 1 },
    { "isdefined", (BuiltinFunction)GScr_IsDefined, 0 },
    { "isstring", (BuiltinFunction)GScr_IsString, 0 },
    { "isalive", (BuiltinFunction)GScr_IsAlive, 0 },
    { "getcvar", (BuiltinFunction)GScr_GetDvar, 0 },
    { "getcvarint", (BuiltinFunction)GScr_GetDvarInt, 0 },
    { "getcvarfloat", (BuiltinFunction)GScr_GetDvarFloat, 0 },
    { "setcvar", (BuiltinFunction)GScr_SetDvar, 0 },
    { "gettime", (BuiltinFunction)GScr_GetTime, 0 },
    { "getentbynum", (BuiltinFunction)Scr_GetEntByNum, 1 },
    { "getweaponmodel", (BuiltinFunction)Scr_GetWeaponModel, 0 },
    { "getanimlength", (BuiltinFunction)GScr_GetAnimLength, 0 },
    { "animhasnotetrack", (BuiltinFunction)GScr_AnimHasNotetrack, 0 },
    { "getbrushmodelcenter", (BuiltinFunction)GScr_GetBrushModelCenter, 0 },
    { "objective_add", (BuiltinFunction)Scr_Objective_Add, 0 },
    { "objective_delete", (BuiltinFunction)Scr_Objective_Delete, 0 },
    { "objective_state", (BuiltinFunction)Scr_Objective_State, 0 },
    { "objective_icon", (BuiltinFunction)Scr_Objective_Icon, 0 },
    { "objective_position", (BuiltinFunction)Scr_Objective_Position, 0 },
    { "objective_onentity", (BuiltinFunction)Scr_Objective_OnEntity, 0 },
    { "objective_current", (BuiltinFunction)Scr_Objective_Current, 0 },
    { "bullettrace", (BuiltinFunction)Scr_BulletTrace, 0 },
    { "bullettracepassed", (BuiltinFunction)Scr_BulletTracePassed, 0 },
    { "sighttracepassed", (BuiltinFunction)Scr_SightTracePassed, 0 },
    { "physicstrace", (BuiltinFunction)Scr_PhysicsTrace, 0 },
    { "getmovedelta", (BuiltinFunction)GScr_GetMoveDelta, 0 },
    { "getangledelta", (BuiltinFunction)GScr_GetAngleDelta, 0 },
    { "getnorthyaw", (BuiltinFunction)GScr_GetNorthYaw, 0 },
    { "randomint", (BuiltinFunction)Scr_RandomInt, 0 },
    { "randomfloat", (BuiltinFunction)Scr_RandomFloat, 0 },
    { "randomintrange", (BuiltinFunction)Scr_RandomIntRange, 0 },
    { "randomfloatrange", (BuiltinFunction)Scr_RandomFloatRange, 0 },
    { "sin", (BuiltinFunction)GScr_sin, 0 },
    { "cos", (BuiltinFunction)GScr_cos, 0 },
    { "tan", (BuiltinFunction)GScr_tan, 0 },
    { "asin", (BuiltinFunction)GScr_asin, 0 },
    { "acos", (BuiltinFunction)GScr_acos, 0 },
    { "atan", (BuiltinFunction)GScr_atan, 0 },
    { "int", (BuiltinFunction)GScr_CastInt, 0 },
    { "distance", (BuiltinFunction)Scr_Distance, 0 },
    { "distancesquared", (BuiltinFunction)Scr_DistanceSquared, 0 },
    { "length", (BuiltinFunction)Scr_Length, 0 },
    { "lengthsquared", (BuiltinFunction)Scr_LengthSquared, 0 },
    { "closer", (BuiltinFunction)Scr_Closer, 0 },
    { "vectordot", (BuiltinFunction)Scr_VectorDot, 0 },
    { "vectornormalize", (BuiltinFunction)Scr_VectorNormalize, 0 },
    { "vectortoangles", (BuiltinFunction)Scr_VectorToAngles, 0 },
    { "anglestoup", (BuiltinFunction)Scr_AnglesToUp, 0 },
    { "anglestoright", (BuiltinFunction)Scr_AnglesToRight, 0 },
    { "anglestoforward", (BuiltinFunction)Scr_AnglesToForward, 0 },
    { "issubstr", (BuiltinFunction)Scr_IsSubStr, 0 },
    { "getsubstr", (BuiltinFunction)Scr_GetSubStr, 0 },
    { "tolower", (BuiltinFunction)Scr_ToLower, 0 },
    { "strtok", (BuiltinFunction)Scr_StrTok, 0 },
    { "musicplay", (BuiltinFunction)Scr_MusicPlay, 0 },
    { "musicstop", (BuiltinFunction)Scr_MusicStop, 0 },
    { "soundfade", (BuiltinFunction)Scr_SoundFade, 0 },
    { "ambientplay", (BuiltinFunction)Scr_AmbientPlay, 0 },
    { "ambientstop", (BuiltinFunction)Scr_AmbientStop, 0 },
    { "precachemodel", (BuiltinFunction)Scr_PrecacheModel, 0 },
    { "precacheshellshock", (BuiltinFunction)Scr_PrecacheShellShock, 0 },
    { "precacheitem", (BuiltinFunction)Scr_PrecacheItem, 0 },
    { "precacheshader", (BuiltinFunction)Scr_PrecacheShader, 0 },
    { "precachestring", (BuiltinFunction)Scr_PrecacheString, 0 },
    { "precacherumble", (BuiltinFunction)Scr_PrecacheRumble, 0 },
    { "loadfx", (BuiltinFunction)Scr_LoadFX, 0 },
    { "playfx", (BuiltinFunction)Scr_PlayFX, 0 },
    { "playfxontag", (BuiltinFunction)Scr_PlayFXOnTag, 0 },
    { "playloopedfx", (BuiltinFunction)Scr_PlayLoopedFX, 0 },
    { "setcullfog", (BuiltinFunction)Scr_SetLinearFog, 0 },
    { "setexpfog", (BuiltinFunction)Scr_SetExponentialFog, 0 },
    { "grenadeexplosioneffect", (BuiltinFunction)Scr_GrenadeExplosionEffect, 0 },
    { "radiusdamage", (BuiltinFunction)GScr_RadiusDamage, 0 },
    { "setplayerignoreradiusdamage", (BuiltinFunction)GScr_SetPlayerIgnoreRadiusDamage, 0 },
    { "getnumparts", (BuiltinFunction)GScr_GetNumParts, 0 },
    { "getpartname", (BuiltinFunction)GScr_GetPartName, 0 },
    { "earthquake", (BuiltinFunction)GScr_Earthquake, 0 },
    { "newhudelem", (BuiltinFunction)GScr_NewHudElem, 0 },
    { "newclienthudelem", (BuiltinFunction)GScr_NewClientHudElem, 0 },
    { "newteamhudelem", (BuiltinFunction)GScr_NewTeamHudElem, 0 },
    { "resettimeout", (BuiltinFunction)Scr_ResetTimeout, 0 },
    { "isplayer", (BuiltinFunction)GScr_IsPlayer, 0 },
    { "isplayernumber", (BuiltinFunction)GScr_IsPlayerNumber, 0 },
    { "setwinningplayer", (BuiltinFunction)GScr_SetWinningPlayer, 0 },
    { "setwinningteam", (BuiltinFunction)GScr_SetWinningTeam, 0 },
    { "announcement", (BuiltinFunction)GScr_Announcement, 0 },
    { "clientannouncement", (BuiltinFunction)GScr_ClientAnnouncement, 0 },
    { "getteamscore", (BuiltinFunction)GScr_GetTeamScore, 0 },
    { "setteamscore", (BuiltinFunction)GScr_SetTeamScore, 0 },
    { "setclientnamemode", (BuiltinFunction)GScr_SetClientNameMode, 0 },
    { "updateclientnames", (BuiltinFunction)GScr_UpdateClientNames, 0 },
    { "getteamplayersalive", (BuiltinFunction)GScr_GetTeamPlayersAlive, 0 },
    { "objective_team", (BuiltinFunction)GScr_Objective_Team, 0 },
    { "logprint", (BuiltinFunction)GScr_LogPrint, 0 },
    { "worldentnumber", (BuiltinFunction)GScr_WorldEntNumber, 0 },
    { "obituary", (BuiltinFunction)GScr_Obituary, 0 },
    { "positionwouldtelefrag", (BuiltinFunction)GScr_positionWouldTelefrag, 0 },
    { "getstarttime", (BuiltinFunction)GScr_getStartTime, 0 },
    { "precachemenu", (BuiltinFunction)GScr_PrecacheMenu, 0 },
    { "precachestatusicon", (BuiltinFunction)GScr_PrecacheStatusIcon, 0 },
    { "precacheheadicon", (BuiltinFunction)GScr_PrecacheHeadIcon, 0 },
    { "map_restart", (BuiltinFunction)GScr_MapRestart, 0 },
    { "exitlevel", (BuiltinFunction)GScr_ExitLevel, 0 },
    { "addtestclient", (BuiltinFunction)GScr_AddTestClient, 0 },
    { "makecvarserverinfo", (BuiltinFunction)GScr_MakeDvarServerInfo, 0 },
    { "setarchive", (BuiltinFunction)GScr_SetArchive, 0 },
    { "allclientsprint", (BuiltinFunction)GScr_AllClientsPrint, 0 },
    { "clientprint", (BuiltinFunction)GScr_ClientPrint, 0 },
    { "mapexists", (BuiltinFunction)GScr_MapExists, 0 },
    { "isvalidgametype", (BuiltinFunction)GScr_IsValidGameType, 0 },
    { "matchend", (BuiltinFunction)GScr_MatchEnd, 0 },
    { "setplayerteamrank", (BuiltinFunction)GScr_SetPlayerTeamRank, 0 },
    { "sendranks", (BuiltinFunction)GScr_SendXboxLiveRanks, 0 },
    { "setvotestring", (BuiltinFunction)GScr_SetVoteString, 0 },
    { "setvotetime", (BuiltinFunction)GScr_SetVoteTime, 0 },
    { "setvoteyescount", (BuiltinFunction)GScr_SetVoteYesCount, 0 },
    { "setvotenocount", (BuiltinFunction)GScr_SetVoteNoCount, 0 },
    { "openfile", (BuiltinFunction)GScr_OpenFile, 0 },
    { "closefile", (BuiltinFunction)GScr_CloseFile, 0 },
    { "fprintln", (BuiltinFunction)GScr_FPrintln, 0 },
    { "freadln", (BuiltinFunction)GScr_FReadLn, 0 },
    { "fgetarg", (BuiltinFunction)GScr_FGetArg, 0 },
    { "kick", (BuiltinFunction)GScr_KickPlayer, 0 },
    { "ban", (BuiltinFunction)GScr_BanPlayer, 0 },
    { "map", (BuiltinFunction)GScr_LoadMap, 0 },
    { "playrumbleonpos", (BuiltinFunction)Scr_PlayRumbleOnPos, 0 },
    { "playlooprumbleonpos", (BuiltinFunction)Scr_PlayLoopRumbleOnPos, 0 },
    { "stopallrumbles", (BuiltinFunction)Scr_StopAllRumbles, 0 },
    { "soundexists", (BuiltinFunction)ScrCmd_SoundExists, 0 },
    { "issplitscreen", (BuiltinFunction)Scr_IsSplitscreen, 0 },
    { "issplitscreen", (BuiltinFunction)GScr_EndXboxLiveLobby, 0 },
};

/* methods: script entity-method dispatch table (was the ILP32 blob symbol
 * methods_003138c0, aliased to methods). Typed BuiltinMethodDef[] defined directly
 * as methods, retiring the /alternatename (MSVC) and --defsym (Linux) seams.
 * End-of-file so the in-file method prototypes are in scope. (x64 port Stage 2.) */
BuiltinMethodDef methods[59] = {
    { "attach", (BuiltinMethod)ScrCmd_attach, 0 },
    { "detach", (BuiltinMethod)ScrCmd_detach, 0 },
    { "detachall", (BuiltinMethod)ScrCmd_detachAll, 0 },
    { "getattachsize", (BuiltinMethod)ScrCmd_GetAttachSize, 0 },
    { "getattachmodelname", (BuiltinMethod)ScrCmd_GetAttachModelName, 0 },
    { "getattachtagname", (BuiltinMethod)ScrCmd_GetAttachTagName, 0 },
    { "getattachignorecollision", (BuiltinMethod)ScrCmd_GetAttachIgnoreCollision, 0 },
    { "getammocount", (BuiltinMethod)GScr_GetAmmoCount, 0 },
    { "getclanid", (BuiltinMethod)ScrCmd_GetClanId, 0 },
    { "getclanname", (BuiltinMethod)ScrCmd_GetClanName, 0 },
    { "getclandescription", (BuiltinMethod)ScrCmd_GetClanDescription, 0 },
    { "getclanmotto", (BuiltinMethod)ScrCmd_GetClanMotto, 0 },
    { "getclanurl", (BuiltinMethod)ScrCmd_GetClanURL, 0 },
    { "linkto", (BuiltinMethod)ScrCmd_LinkTo, 0 },
    { "unlink", (BuiltinMethod)ScrCmd_Unlink, 0 },
    { "enablelinkto", (BuiltinMethod)ScrCmd_EnableLinkTo, 0 },
    { "getorigin", (BuiltinMethod)ScrCmd_GetOrigin, 0 },
    { "geteye", (BuiltinMethod)ScrCmd_GetEye, 0 },
    { "useby", (BuiltinMethod)ScrCmd_UseBy, 0 },
    { "setstablemissile", (BuiltinMethod)Scr_SetStableMissile, 0 },
    { "istouching", (BuiltinMethod)ScrCmd_IsTouching, 0 },
    { "playsound", (BuiltinMethod)ScrCmd_PlaySound, 0 },
    { "playsoundasmaster", (BuiltinMethod)ScrCmd_PlaySoundAsMaster, 0 },
    { "playloopsound", (BuiltinMethod)ScrCmd_PlayLoopSound, 0 },
    { "stoploopsound", (BuiltinMethod)ScrCmd_StopLoopSound, 0 },
    { "playrumble", (BuiltinMethod)ScrCmd_PlayRumble, 0 },
    { "playlooprumble", (BuiltinMethod)ScrCmd_PlayLoopRumble, 0 },
    { "stoprumble", (BuiltinMethod)ScrCmd_StopRumble, 0 },
    { "delete", (BuiltinMethod)ScrCmd_Delete, 0 },
    { "setmodel", (BuiltinMethod)ScrCmd_SetModel, 0 },
    { "getnormalhealth", (BuiltinMethod)ScrCmd_GetNormalHealth, 0 },
    { "setnormalhealth", (BuiltinMethod)ScrCmd_SetNormalHealth, 0 },
    { "show", (BuiltinMethod)ScrCmd_Show, 0 },
    { "hide", (BuiltinMethod)ScrCmd_Hide, 0 },
    { "showtoplayer", (BuiltinMethod)ScrCmd_ShowToPlayer, 0 },
    { "setcontents", (BuiltinMethod)ScrCmd_SetContents, 0 },
    { "setcursorhint", (BuiltinMethod)GScr_SetCursorHint, 0 },
    { "sethintstring", (BuiltinMethod)GScr_SetHintString, 0 },
    { "shellshock", (BuiltinMethod)GScr_ShellShock, 0 },
    { "stopshellshock", (BuiltinMethod)GScr_StopShellShock, 0 },
    { "viewkick", (BuiltinMethod)GScr_ViewKick, 0 },
    { "localtoworldcoords", (BuiltinMethod)GScr_LocalToWorldCoords, 0 },
    { "setrightarc", (BuiltinMethod)GScr_SetRightArc, 0 },
    { "setleftarc", (BuiltinMethod)GScr_SetLeftArc, 0 },
    { "settoparc", (BuiltinMethod)GScr_SetTopArc, 0 },
    { "setbottomarc", (BuiltinMethod)GScr_SetBottomArc, 0 },
    { "getentitynumber", (BuiltinMethod)GScr_GetEntityNumber, 0 },
    { "enablegrenadetouchdamage", (BuiltinMethod)GScr_EnableGrenadeTouchDamage, 0 },
    { "disablegrenadetouchdamage", (BuiltinMethod)GScr_DisableGrenadeTouchDamage, 0 },
    { "enablegrenadebounce", (BuiltinMethod)GScr_EnableGrenadeBounce, 0 },
    { "disablegrenadebounce", (BuiltinMethod)GScr_DisableGrenadeBounce, 0 },
    { "enableaimassist", (BuiltinMethod)GScr_EnableAimAssist, 0 },
    { "disableaimassist", (BuiltinMethod)GScr_DisableAimAssist, 0 },
    { "placespawnpoint", (BuiltinMethod)GScr_PlaceSpawnPoint, 0 },
    { "updatescores", (BuiltinMethod)GScr_UpdateScores, 0 },
    { "setteamfortrigger", (BuiltinMethod)GScr_SetTeamForTrigger, 0 },
    { "clientclaimtrigger", (BuiltinMethod)GScr_ClientClaimTrigger, 0 },
    { "clientreleasetrigger", (BuiltinMethod)GScr_ClientReleaseTrigger, 0 },
    { "releaseclaimedtrigger", (BuiltinMethod)GScr_ReleaseClaimedTrigger, 0 },
};

/* bg_itemlist: BG item table. Migrated from the ILP32 data blob to a typed
 * gitem_t[132] so it re-lays-out per target (the packed blob {6 ptr,5 int}
 * diverges from the natural struct padding on x64). Mostly empty/runtime-filled;
 * only the two health pickups carry static data. (x64 port Stage 2.) */
gitem_t bg_itemlist[132] = {
    { NULL, NULL, { NULL, NULL }, NULL, NULL, 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { NULL, "", { "", "" }, "", "", 0, 0, 0, 0, 0 },
    { "item_health_small", "health_pickup_small", { "xmodel/health_small", NULL }, "icons/iconh_small", "Small Health", 10, 3, 0, 0, 0 },
    { "item_health_large", "health_pickup_large", { "xmodel/health_large", NULL }, "icons/iconh_large", "Large Health", 50, 3, 0, 0, 0 },
    { NULL, NULL, { NULL, NULL }, NULL, NULL, 0, 0, 0, 0, 0 },
};
