#include "common_types.h"
#include "imports.h"
#include <stdint.h>
extern level_locals_t level;

extern const char *SV_GetConfigstringConst(int index);
extern void SV_SetConfigstring(int index, const char *val);
extern void Scr_Error(const char *msg);
extern char *va(const char *format, ...);
extern void Com_Error(int code, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern int stricmp(const char *s1, const char *s2);
extern char *I_strlwr(char *str);
extern struct XModel *SV_XModelGet(const char *name);
extern void Com_SafeServerDObjFree(int entnum);
extern qboolean SV_DObjUpdateServerTime(gentity_t *ent, float time, qboolean bNotify);
extern qboolean SV_DObjCreateSkelForBones(gentity_t *ent, int *partBits);
extern qboolean SV_DObjCreateSkelForBone(gentity_t *ent, int boneIndex);
extern void SV_DObjGetHierarchyBits(gentity_t *ent, int boneIndex, int *partBits);
extern void SV_DObjCalcAnim(gentity_t *ent, int *partBits);
extern void SV_DObjCalcSkel(gentity_t *ent, int *partBits);
extern int SV_DObjGetBoneIndex(gentity_t *ent, unsigned int tagName);
extern void *SV_DObjGetMatrixArray(gentity_t *ent);
extern void Scr_SetString(scr_string_t *to, unsigned int value);
extern unsigned int SL_GetString(const char *str, int type);
extern const char *SL_ConvertToString(unsigned int id);
extern void AnglesToAxis(const vec_t *angles, vec_t *axis);
extern void AxisToAngles(const vec_t *axis, vec_t *angles);
extern float vectoyaw(const vec_t *vec);
extern void MatrixMultiply(const vec_t *in1, const vec_t *in2, vec_t *out);
extern void MatrixMultiply43(const vec_t *in1, const vec_t *in2, vec_t *out);
extern void MatrixTransformVector43(const vec_t *in, const vec_t *mat, vec_t *out);
extern void MatrixTranspose(const vec_t *in, vec_t *out);
extern void MatrixInverseOrthogonal43(const vec_t *in, vec_t *out);
extern void SetClientViewAngle(gentity_t *ent, const vec_t *angles);
extern void *MT_Alloc(int size, int type);
extern void MT_Free(void *ptr, int size);
extern void SV_LinkEntity(gentity_t *ent);
extern void SV_UnlinkEntity(gentity_t *ent);
extern void *SV_DObjGetTree(gentity_t *ent);
extern void XAnimClearTree(void *tree);
extern void G_FreeTurret(gentity_t *ent);
extern void Scr_FreeEntity(gentity_t *ent);
extern unsigned int Scr_ExecEntThread(gentity_t *ent, int callback, int numArgs);
extern void Scr_FreeThread(unsigned short thread);
extern void BG_AddPredictableEventToPlayerstate(int event, int eventParm, void *ps);
extern qboolean XModelBad(struct XModel *model);
extern void Hunk_OverrideDataForFile(int type, const char *name, void *data);
extern float Vec3DistanceSq(const vec_t *a, const vec_t *b);
extern void Com_ServerDObjCreate(DObjModel_s *dobjModels, int numModels, struct XAnimTree_s *tree, int handle);
extern void SV_LocateGameData(void *gEnts, int numGEntities, int sizeofGEntity, void *clients, int sizeofGameClient);
extern qboolean SV_DObjExists(gentity_t *ent);

static struct XModel *cached_models[256];

#define _ENT(e) ((gentity_t *)(e))

#define ENTITY_STRIDE sizeof(gentity_s)

extern level_locals_t level;
extern gentity_t g_entities[];
extern scr_const_t scr_const;
extern entityHandler_t entityHandlers[20];
extern struct scr_data_t g_scr_data;

#define LEVEL_PTR (&level)
#define LEVEL_GENTITIES (LEVEL_PTR->gentities)
#define G_ENTITIES_DIRECT ((gentity_t *)imp_g_entities)
#define LEVEL_CLIENTS (LEVEL_PTR->clients)
#define LEVEL_NUMENTS (LEVEL_PTR->num_entities)
#define LEVEL_FIRSTFREEENT (LEVEL_PTR->firstFreeEnt)
#define LEVEL_LASTFREEENT (LEVEL_PTR->lastFreeEnt)
#define LEVEL_INITIALIZING (LEVEL_PTR->initializing)
#define LEVEL_TIME (LEVEL_PTR->time)
#define LEVEL_SERVERTIME (LEVEL_PTR->frametime)
#define LEVEL_DROPPED_WEAPON_CUE (LEVEL_PTR->droppedWeaponCue)
#define LEVEL_CLONEIDX (LEVEL_PTR->currentPlayerClone)

int COD2_GEntityHandle(const gentity_t *ent)
{
#if defined(COD2_X64) || defined(__x86_64__) || defined(_M_X64)
    return ent ? (int)(ent - (const gentity_t *)imp_g_entities) + 1 : 0;
#else
    return (int)(uintptr_t)ent;
#endif
}

gentity_t *COD2_GEntityFromHandle(int handle)
{
#if defined(COD2_X64) || defined(__x86_64__) || defined(_M_X64)
    if (handle <= 0 || handle > 1024)
        return NULL;
    return handle ? &((gentity_t *)imp_g_entities)[handle - 1] : NULL;
#else
    return (gentity_t *)(uintptr_t)handle;
#endif
}

int COD2_TagInfoHandle(const tagInfo_t *tagInfo)
{
#if defined(COD2_X64) || defined(__x86_64__) || defined(_M_X64)
    return tagInfo ? (int)SCR_ARENA_ENC(tagInfo) + 1 : 0;
#else
    return (int)(uintptr_t)tagInfo;
#endif
}

tagInfo_t *COD2_TagInfoFromHandle(int handle)
{
#if defined(COD2_X64) || defined(__x86_64__) || defined(_M_X64)
    unsigned int off;
    if (handle <= 0)
        return NULL;
    off = (unsigned int)(handle - 1);
    if (off >= 0x80330)
        return NULL;
    return (tagInfo_t *)SCR_ARENA_PTR(off);
#else
    return (tagInfo_t *)(uintptr_t)handle;
#endif
}

#define GUTILS_PLAYER_CLONE_BASE 64
#define GUTILS_DYNAMIC_ENTITY_START 72

#define HANDLER_CALCPOSE(h) (((entityHandler_t *)imp_entityHandlers)[(h)].controller)

#define CORPSE_ENTNUM(i) ((((struct scr_data_t *)imp_g_scr_data)->playerCorpseInfo[(i)]).entnum)
#define CORPSE_CALLBACK() (g_scr_data.delete_)
#define SCR_CONST() ((const scr_const_t *)imp_scr_const)

#define VectorCopy(a, b) ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2])
#define VectorClear(v) ((v)[0] = 0, (v)[1] = 0, (v)[2] = 0)

int G_FindConfigstringIndex(const char *name, int start, int max, qboolean create, const char *errormsg);
int G_LocalizedStringIndex(const char *string);
int G_ShaderIndex(const char *name);
int G_ModelIndex(const char *name);
const char *G_ModelName(int index);
int G_TagIndex(const char *name);
int G_EffectIndex(const char *name);
int G_ShellShockIndex(const char *name);
SoundAlias G_SoundAliasIndex(const char *name);
unsigned char G_SetModel(gentity_t *ent, const char *modelName);
void G_SafeDObjFree(gentity_t *ent);
qboolean G_DObjUpdateServerTime(gentity_t *ent, qboolean bNotify);
int G_DObjCalcPose(gentity_t *ent);
int G_DObjCalcBone(gentity_t *ent, int boneIndex);
DObjAnimMat_s *G_DObjGetLocalTagMatrix(gentity_t *ent, unsigned int tagName);
unsigned char G_InitGentity(gentity_t *e);
unsigned char G_PrintEntities(void);
int G_GetPlayerCorpseIndex(gentity_t *ent);
void G_FreeEntityDelay(gentity_t *ed);
void G_AddPredictableEvent(gentity_t *ent, int event, int eventParm);
unsigned char G_AddEvent(gentity_t *ent, int event, int eventParm);
unsigned char G_SetConstString(scr_string_t *to, const char *from);
unsigned char G_SetAngle(gentity_t *ent, const vec_t *angle);
qboolean G_XModelBad(int index);
unsigned char G_SetOrigin(gentity_t *ent, const vec_t *origin);
unsigned char G_PlaySoundAlias(gentity_t *ent, int index);
unsigned char G_OverrideModel(int modelindex, const char *defaultModelName);
int G_AnimScriptSound(int client, snd_alias_list_t *aliasList);
unsigned char G_CalcTagParentAxis(gentity_t *ent, vec3_t *parentAxis);
unsigned char G_SetFixedLink(gentity_t *ent, int eAngles);
unsigned char G_CalcTagAxis(gentity_t *ent, qboolean bAnglesOnly);
unsigned char G_EntUnlink(gentity_t *ent);
static qboolean G_EntLinkToInternal(gentity_t *ent, gentity_t *parent, unsigned int tagName);
qboolean G_EntLinkToWithOffset(gentity_t *ent, gentity_t *parent, unsigned int tagName, const vec_t *originOffset, const vec_t *anglesOffset);
qboolean G_EntLinkTo(gentity_t *ent, gentity_t *parent, unsigned int tagName);
void G_GeneralLink(gentity_t *ent);
unsigned char G_FreeEntity(gentity_t *ed);
int G_GetFreePlayerCorpseIndex(void);
void G_DObjUpdate(gentity_t *ent);
unsigned char G_EntDetachAll(gentity_t *ent);
qboolean G_EntDetach(gentity_t *ent, const char *modelName, unsigned int tagName);
qboolean G_EntAttach(gentity_t *ent, const char *modelName, unsigned int tagName, qboolean ignoreCollision);
gentity_t *G_Spawn(void);
gentity_t *G_TempEntity(const vec_t *origin, int event);
gentity_t *G_SpawnPlayerClone(void);
qboolean G_DObjGetWorldTagMatrix(gentity_t *ent, unsigned int tagName, vec3_t *tagMat);
int G_DObjGetWorldTagPos(gentity_t *ent, unsigned int tagName, vec_t *pos);

int G_FindConfigstringIndex(const char *name, int start, int max, qboolean create, const char *errormsg)
{
    int i;

    if (!name || !*name) {
        return 0;
    }

    for (i = 1; i < max; i++) {
        const char *s = SV_GetConfigstringConst(start + i);
        if (!*s) {
            break;
        }
        if (!stricmp(s, name)) {
            return i;
        }
    }

    if (!create) {
        if (errormsg) {
            Scr_Error(va("%s \"%s\" not precached", errormsg, name));
        }
        return 0;
    }

    if (i == max) {
        Com_Error(1, va("\025G_FindConfigstringIndex: overflow (%d): '%s'", start, name));
    }

    SV_SetConfigstring(start + i, name);
    return i;
}

int G_LocalizedStringIndex(const char *string)
{
    if (!*string) {
        return 0;
    }
    return G_FindConfigstringIndex(string, 0x51e, 0x100, LEVEL_INITIALIZING, "localizedstring");
}

int G_ShaderIndex(const char *name)
{
    char shaderName[64];

    strcpy(shaderName, name);
    I_strlwr(shaderName);

    return G_FindConfigstringIndex(shaderName, 0x61e, 0x80, LEVEL_INITIALIZING, "shader");
}

int G_ModelIndex(const char *name)
{
    int i;

    if (!*name) {
        return 0;
    }

    for (i = 1; i < 256; i++) {
        const char *s = SV_GetConfigstringConst(0x14e + i);
        if (!*s) {
            break;
        }
        if (!stricmp(s, name)) {
            return i;
        }
    }

    if (!LEVEL_INITIALIZING) {
        Scr_Error(va("model '%s' not precached", name));
    }

    if (i == 256) {
        Com_Error(1, "\x15G_ModelIndex: overflow");
    }

    cached_models[i] = SV_XModelGet(name);
    SV_SetConfigstring(0x14e + i, name);
    return i;
}

const char *G_ModelName(int index)
{
    return SV_GetConfigstringConst(index + 0x14e);
}

int G_TagIndex(const char *name)
{
    return G_FindConfigstringIndex(name, 0x6e, 0x20, 1, 0);
}

int G_EffectIndex(const char *name)
{
    return G_FindConfigstringIndex(name, 0x34e, 0x40, LEVEL_INITIALIZING, "effect");
}

int G_ShellShockIndex(const char *name)
{
    return G_FindConfigstringIndex(name, 0x48e, 0x10, 1, 0);
}

SoundAlias G_SoundAliasIndex(const char *name)
{
    return (SoundAlias)(unsigned char)G_FindConfigstringIndex(name, 0x24e, 0x100, 1, 0);
}

unsigned char G_SetModel(gentity_t *ent, const char *modelName)
{
    if (!*modelName) {
        (_ENT(ent)->model) = 0;
    } else {
        (_ENT(ent)->model) = (byte)G_ModelIndex(modelName);
    }
}

void G_SafeDObjFree(gentity_t *ent)
{
    Com_SafeServerDObjFree((_ENT(ent)->s.number));
}

qboolean G_DObjUpdateServerTime(gentity_t *ent, qboolean bNotify)
{
    return SV_DObjUpdateServerTime(ent, (float)LEVEL_SERVERTIME * 0.001f, bNotify);
}

int G_DObjCalcPose(gentity_t *ent)
{
    int partBits[4];
    void (*calcPoseFunc)(gentity_t *, int *);
    int created;

    partBits[0] = -1;
    partBits[1] = -1;
    partBits[2] = -1;
    partBits[3] = -1;

    created = SV_DObjCreateSkelForBones(ent, partBits);
    if (created) {
        return created;
    }

    SV_DObjCalcAnim(ent, partBits);

    calcPoseFunc = HANDLER_CALCPOSE((_ENT(ent)->handler));
    if (calcPoseFunc) {
        calcPoseFunc(ent, partBits);
    }

    SV_DObjCalcSkel(ent, partBits);
}

int G_DObjCalcBone(gentity_t *ent, int boneIndex)
{
    int partBits[4];
    void (*calcPoseFunc)(gentity_t *, int *);
    int created = SV_DObjCreateSkelForBone(ent, boneIndex);

    if (created) {
        return created;
    }

    Com_Printf("[ckpt] CalcBone before HierarchyBits\n");
    SV_DObjGetHierarchyBits(ent, boneIndex, partBits);
    Com_Printf("[ckpt] CalcBone before CalcAnim\n");
    SV_DObjCalcAnim(ent, partBits);
    Com_Printf("[ckpt] CalcBone before calcPose\n");

    calcPoseFunc = HANDLER_CALCPOSE((_ENT(ent)->handler));
    if (calcPoseFunc) {
        calcPoseFunc(ent, partBits);
    }

    Com_Printf("[ckpt] CalcBone before CalcSkel\n");
    SV_DObjCalcSkel(ent, partBits);
    Com_Printf("[ckpt] CalcBone DONE\n");
}

DObjAnimMat_s *G_DObjGetLocalTagMatrix(gentity_t *ent, unsigned int tagName)
{
    int boneIndex;

    boneIndex = SV_DObjGetBoneIndex(ent, tagName);
    if (boneIndex < 0) {
        return 0;
    }

    G_DObjCalcBone(ent, boneIndex);

    return &((DObjAnimMat_s *)SV_DObjGetMatrixArray(ent))[boneIndex];
}

static inline __attribute__((always_inline)) void G_InitGentity_core(gentity_t *e)
{
    (_ENT(e)->nextFree = COD2_GEntityHandle(NULL));
    (_ENT(e)->r.inuse) = 1;
    Scr_SetString(&e->classname, SCR_CONST()->noclass);
    (_ENT(e)->s.number) = (int)(e - (gentity_t *)imp_g_entities);
    (_ENT(e)->r.ownerNum) = 0x3FF;
    (_ENT(e)->eventTime) = 0;
    (_ENT(e)->freeAfterEvent) = 0;
}

unsigned char G_InitGentity(gentity_t *e)
{
    G_InitGentity_core(e);
}

unsigned char G_PrintEntities(void)
{
    int entityIndex;
    gentity_t *ent = G_ENTITIES_DIRECT;

    for (entityIndex = 0; entityIndex < LEVEL_NUMENTS; entityIndex++, ent++) {
        const char *classStr;
        unsigned short cn = (_ENT(ent)->classname);
        if (!cn) {
            classStr = "";
        } else {
            classStr = SL_ConvertToString(cn);
        }
        Com_Printf("%4i: '%s', origin: %f %f %f\n",
                   entityIndex, classStr,
                   (double)(_ENT(ent)->r.currentOrigin)[0],
                   (double)(_ENT(ent)->r.currentOrigin)[1],
                   (double)(_ENT(ent)->r.currentOrigin)[2]);
    }
}

int G_GetPlayerCorpseIndex(gentity_t *ent)
{
    int i;
    int entnum = (_ENT(ent)->s.number);

    for (i = 0; i < 8; i++) {
        if (CORPSE_ENTNUM(i) == entnum) {
            return i;
        }
    }
    return 0;
}

void G_FreeEntityDelay(gentity_t *ed)
{
    unsigned short thread;
    thread = (unsigned short)Scr_ExecEntThread(ed, ((struct scr_data_t *)imp_g_scr_data)->delete_, 0);
    Scr_FreeThread(thread);
}

void G_AddPredictableEvent(gentity_t *ent, int event, int eventParm)
{
    gclient_t *client = (_ENT(ent)->client);
    if (client) {
        BG_AddPredictableEventToPlayerstate(event, eventParm, &client->ps);
    }
}

unsigned char G_AddEvent(gentity_t *ent, int event, int eventParm)
{
    gclient_t *client = (_ENT(ent)->client);

    if (client) {
        int seq = (((gclient_t *)(client))->ps.eventSequence);
        (((gclient_t *)(client))->ps.events[seq & 3]) = event;
        (((gclient_t *)(client))->ps.eventParms[seq & 3]) = eventParm;
        (((gclient_t *)(client))->ps.eventSequence) = seq + 1;
    } else {
        int seq = (_ENT(ent)->s.eventSequence);
        (_ENT(ent)->s.events[seq & 3]) = event;
        (_ENT(ent)->s.eventParms[seq & 3]) = eventParm;
        (_ENT(ent)->s.eventSequence) = seq + 1;
    }

    (_ENT(ent)->eventTime) = LEVEL_TIME;
    (_ENT(ent)->r.eventTime) = LEVEL_TIME;
}

unsigned char G_SetConstString(scr_string_t *to, const char *from)
{
    Scr_SetString(to, 0);
    *to = (scr_string_t)SL_GetString(from, 0);
}

unsigned char G_SetAngle(gentity_t *ent, const vec_t *angle)
{
    float *p = (_ENT(ent)->s.apos.trBase);
    p[0] = angle[0];
    p[1] = angle[1];
    p[2] = angle[2];
    (_ENT(ent)->s.apos.trType) = 0;
    (_ENT(ent)->s.apos.trTime) = 0;
    (_ENT(ent)->s.apos.trDuration) = 0;
    p = (_ENT(ent)->s.apos.trDelta);
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;
    p = (_ENT(ent)->r.currentAngles);
    p[0] = angle[0];
    p[1] = angle[1];
    p[2] = angle[2];
}

qboolean G_XModelBad(int index)
{
    return XModelBad(cached_models[index]);
}

unsigned char G_SetOrigin(gentity_t *ent, const vec_t *origin)
{
    float *p = (_ENT(ent)->s.pos.trBase);
    p[0] = origin[0];
    p[1] = origin[1];
    p[2] = origin[2];
    (_ENT(ent)->s.pos.trType) = 0;
    (_ENT(ent)->s.pos.trTime) = 0;
    (_ENT(ent)->s.pos.trDuration) = 0;
    p = (_ENT(ent)->s.pos.trDelta);
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;
    p = (_ENT(ent)->r.currentOrigin);
    p[0] = origin[0];
    p[1] = origin[1];
    p[2] = origin[2];
}

unsigned char G_PlaySoundAlias(gentity_t *ent, int index)
{
    byte soundIndex = (byte)index;
    gclient_t *client;

    if (!soundIndex) {
        return 0;
    }

    client = (_ENT(ent)->client);
    if (client) {
        int seq = (((gclient_t *)(client))->ps.eventSequence);
        (((gclient_t *)(client))->ps.events[seq & 3]) = 0xB3;
        (((gclient_t *)(client))->ps.eventParms[seq & 3]) = (int)soundIndex;
        (((gclient_t *)(client))->ps.eventSequence) = seq + 1;
    } else {
        int seq = (_ENT(ent)->s.eventSequence);
        (_ENT(ent)->s.events[seq & 3]) = 0xB3;
        (_ENT(ent)->s.eventParms[seq & 3]) = (int)soundIndex;
        (_ENT(ent)->s.eventSequence) = seq + 1;
    }

    (_ENT(ent)->eventTime) = LEVEL_TIME;
    (_ENT(ent)->r.eventTime) = LEVEL_TIME;
}

unsigned char G_OverrideModel(int modelindex, const char *defaultModelName)
{
    const char *modelName = SV_GetConfigstringConst(0x14e + modelindex);

    cached_models[modelindex] = SV_XModelGet(defaultModelName);
    Hunk_OverrideDataForFile(4, modelName + 7, cached_models[modelindex]);
}

int G_AnimScriptSound(int client, snd_alias_list_t *aliasList)
{
    byte soundIndex;
    gentity_t *ent;

    soundIndex = (byte)G_FindConfigstringIndex(*(const char **)aliasList, 0x24e, 0x100, 1, 0);

    ent = &G_ENTITIES_DIRECT[client];

    if (soundIndex) {
        gclient_t *cl = (_ENT(ent)->client);
        if (cl) {
            int seq = (((gclient_t *)(cl))->ps.eventSequence);
            (((gclient_t *)(cl))->ps.events[seq & 3]) = 0xB3;
            (((gclient_t *)(cl))->ps.eventParms[seq & 3]) = (int)soundIndex;
            (((gclient_t *)(cl))->ps.eventSequence) = seq + 1;
        } else {
            int seq = (_ENT(ent)->s.eventSequence);
            (_ENT(ent)->s.events[seq & 3]) = 0xB3;
            (_ENT(ent)->s.eventParms[seq & 3]) = (int)soundIndex;
            (_ENT(ent)->s.eventSequence) = seq + 1;
        }

        (_ENT(ent)->eventTime) = LEVEL_TIME;
        (_ENT(ent)->r.eventTime) = LEVEL_TIME;
    }

    return 0;
}

unsigned char G_CalcTagParentAxis(gentity_t *ent, vec3_t *parentAxis)
{
    tagInfo_t *tagInfo = COD2_TagInfoFromHandle(_ENT(ent)->tagInfo);
    gentity_t *parent;
    int boneIndex;

    if (!tagInfo || !tagInfo->parent)
        return 0;

    parent = ((tagInfo)->parent);
    boneIndex = ((tagInfo)->index);

    if (boneIndex < 0) {

        AnglesToAxis((_ENT(parent)->r.currentAngles), (vec_t *)parentAxis);
        VectorCopy((_ENT(parent)->r.currentOrigin), ((vec_t *)parentAxis + 9));
    } else {
        vec3_t tempAxis[3];
        vec3_t axis[3];
        vec3_t origin;
        DObjAnimMat_s *mat;

        AnglesToAxis((_ENT(parent)->r.currentAngles), (vec_t *)tempAxis);
        VectorCopy((_ENT(parent)->r.currentOrigin), origin);

        G_DObjCalcBone(parent, boneIndex);
        mat = &((DObjAnimMat_s *)SV_DObjGetMatrixArray(parent))[boneIndex];

        {
            float scale = mat->transWeight;
            float x = scale * mat->quat[0];
            float y = scale * mat->quat[1];
            float z = scale * mat->quat[2];
            float xx = x * mat->quat[0];
            float xy = x * mat->quat[1];
            float xz = x * mat->quat[2];
            float xw = x * mat->quat[3];
            float yy = y * mat->quat[1];
            float yz = y * mat->quat[2];
            float yw = y * mat->quat[3];
            float zz = z * mat->quat[2];
            float zw = z * mat->quat[3];

            axis[0][0] = 1.0f - (yy + zz);
            axis[0][1] = zw + xy;
            axis[0][2] = xz - yw;
            axis[1][0] = xy - zw;
            axis[1][1] = 1.0f - (xx + zz);
            axis[1][2] = xw + yz;
            axis[2][0] = xz + yw;
            axis[2][1] = yz - xw;
            axis[2][2] = 1.0f - (xx + yy);
        }

        MatrixMultiply((vec_t *)axis, (vec_t *)tempAxis, (vec_t *)parentAxis);
        MatrixTransformVector43((vec_t *)&mat->trans, (vec_t *)tempAxis, ((vec_t *)parentAxis + 9));
    }
}

unsigned char G_SetFixedLink(gentity_t *ent, int eAngles)
{
    vec3_t parentAxis[4];
    tagInfo_t *tagInfo;
    vec3_t axis[4];

    tagInfo = COD2_TagInfoFromHandle(_ENT(ent)->tagInfo);
    if (!tagInfo || !tagInfo->parent) {
        _ENT(ent)->tagInfo = COD2_TagInfoHandle(NULL);
        return 0;
    }

    G_CalcTagParentAxis(ent, parentAxis);

    switch (eAngles) {
    case 0:
        MatrixMultiply43(((vec_t *)(tagInfo)->axis), (vec_t *)parentAxis, (vec_t *)axis);
        VectorCopy(((vec_t *)axis + 9), (_ENT(ent)->r.currentOrigin));
        AxisToAngles((vec_t *)axis, (_ENT(ent)->r.currentAngles));
        break;
    case 1:
        MatrixMultiply43(((vec_t *)(tagInfo)->axis), (vec_t *)parentAxis, (vec_t *)axis);
        VectorCopy(((vec_t *)axis + 9), (_ENT(ent)->r.currentOrigin));
        (_ENT(ent)->r.currentAngles)
        [1] = vectoyaw((vec_t *)axis);
        break;
    case 2:
        MatrixTransformVector43(((tagInfo)->axis[3]), (vec_t *)parentAxis, (_ENT(ent)->r.currentOrigin));
        break;
    }
}

unsigned char G_CalcTagAxis(gentity_t *ent, qboolean bAnglesOnly)
{
    vec3_t parentAxis[4];
    vec3_t axis[3];
    tagInfo_t *tagInfo;
    vec3_t invParentAxis[4];

    tagInfo = COD2_TagInfoFromHandle(_ENT(ent)->tagInfo);
    if (!tagInfo || !tagInfo->parent) {
        _ENT(ent)->tagInfo = COD2_TagInfoHandle(NULL);
        return 0;
    }

    G_CalcTagParentAxis(ent, parentAxis);
    AnglesToAxis((_ENT(ent)->r.currentAngles), (vec_t *)axis);

    if (bAnglesOnly) {
        MatrixTranspose((vec_t *)parentAxis, (vec_t *)invParentAxis);
        MatrixMultiply((vec_t *)axis, (vec_t *)invParentAxis, ((vec_t *)(tagInfo)->axis));
    } else {
        vec3_t origin;
        MatrixInverseOrthogonal43((vec_t *)parentAxis, (vec_t *)invParentAxis);
        VectorCopy((_ENT(ent)->r.currentOrigin), origin);
        MatrixMultiply43((vec_t *)axis, (vec_t *)invParentAxis, ((vec_t *)(tagInfo)->axis));
    }
}

unsigned char G_EntUnlink(gentity_t *ent)
{
    tagInfo_t *tagInfo = COD2_TagInfoFromHandle(_ENT(ent)->tagInfo);
    if (!tagInfo) {
        return 0;
    }

    VectorCopy((_ENT(ent)->r.currentOrigin), (_ENT(ent)->s.pos.trBase));
    (_ENT(ent)->s.pos.trType) = 0;
    (_ENT(ent)->s.pos.trTime) = 0;
    (_ENT(ent)->s.pos.trDuration) = 0;
    VectorClear((_ENT(ent)->s.pos.trDelta));

    VectorCopy((_ENT(ent)->r.currentAngles), (_ENT(ent)->s.apos.trBase));
    (_ENT(ent)->s.apos.trType) = 0;
    (_ENT(ent)->s.apos.trTime) = 0;
    (_ENT(ent)->s.apos.trDuration) = 0;
    VectorClear((_ENT(ent)->s.apos.trDelta));

    if ((_ENT(ent)->client)) {
        gclient_t *client = (_ENT(ent)->client);
        vec3_t viewAngles;
        viewAngles[0] = (((gclient_t *)(client))->ps.viewangles)[0];
        viewAngles[1] = (((gclient_t *)(client))->ps.viewangles)[1];
        viewAngles[2] = 0;
        SetClientViewAngle(ent, viewAngles);
    }

    {
        gentity_t *parent = ((tagInfo)->parent);
        gentity_t *child = COD2_GEntityFromHandle(_ENT(parent)->tagChildren);

        if (child == ent) {

            (_ENT(parent)->tagChildren = COD2_GEntityHandle(tagInfo->next));
        } else {

            while (child) {
                tagInfo_t *childTag = COD2_TagInfoFromHandle(_ENT(child)->tagInfo);
                gentity_t *next = ((childTag)->next);
                if (next == ent) {
                    ((childTag)->next) = ((tagInfo)->next);
                    break;
                }
                child = next;
            }
        }
    }

    (_ENT(ent)->tagInfo = COD2_TagInfoHandle(NULL));
    Scr_SetString(&((tagInfo)->name), 0);
    MT_Free((unsigned int *)tagInfo, 0x70);
}

static qboolean G_EntLinkToInternal(gentity_t *ent, gentity_t *parent, unsigned int tagName)
{
    gentity_t *checkEnt;
    tagInfo_s *tagInfo;
    int index;

    G_EntUnlink(ent);

    if (tagName) {
        if (!SV_DObjExists(parent)) {
            return 0;
        }

        index = SV_DObjGetBoneIndex(parent, tagName);
        if (index < 0) {
            return 0;
        }
    } else {
        index = -1;
    }

    for (checkEnt = parent;; checkEnt = COD2_TagInfoFromHandle(_ENT(checkEnt)->tagInfo)->parent) {
        if (checkEnt == ent) {
            return 0;
        }

        if (!COD2_TagInfoFromHandle(_ENT(checkEnt)->tagInfo)) {
            break;
        }
    }

    tagInfo = (tagInfo_s *)MT_Alloc(sizeof(*tagInfo), 0x10);
    tagInfo->parent = parent;
    tagInfo->name = 0;
    Scr_SetString(&tagInfo->name, tagName);
    tagInfo->next = COD2_GEntityFromHandle(_ENT(parent)->tagChildren);
    tagInfo->index = index;
    memset(tagInfo->axis, 0, sizeof(tagInfo->axis));
    (_ENT(parent)->tagChildren = COD2_GEntityHandle(ent));
    (_ENT(ent)->tagInfo = COD2_TagInfoHandle(tagInfo));
    memset(tagInfo->parentInvAxis, 0, sizeof(tagInfo->parentInvAxis));
    return 1;
}

qboolean G_EntLinkToWithOffset(gentity_t *ent, gentity_t *parent, unsigned int tagName, const vec_t *originOffset, const vec_t *anglesOffset)
{
    tagInfo_s *tagInfo;

    if (!G_EntLinkToInternal(ent, parent, tagName)) {
        return 0;
    }

    tagInfo = COD2_TagInfoFromHandle(_ENT(ent)->tagInfo);
    AnglesToAxis(anglesOffset, (vec_t *)tagInfo->axis);
    {
        float *d = tagInfo->axis[3];
        d[0] = originOffset[0];
        d[1] = originOffset[1];
        d[2] = originOffset[2];
    }
    return 1;
}

qboolean G_EntLinkTo(gentity_t *ent, gentity_t *parent, unsigned int tagName)
{
    if (!G_EntLinkToInternal(ent, parent, tagName)) {
        return 0;
    }

    G_CalcTagAxis(ent, 0);
    return 1;
}

void G_GeneralLink(gentity_t *ent)
{
    float *p;

    {
        tagInfo_t *tagInfo = COD2_TagInfoFromHandle(_ENT(ent)->tagInfo);
        if (!tagInfo || !tagInfo->parent) {
            _ENT(ent)->tagInfo = COD2_TagInfoHandle(NULL);
            return;
        }
    }

    G_SetFixedLink(ent, 0);

    p = (_ENT(ent)->s.pos.trBase);
    p[0] = (_ENT(ent)->r.currentOrigin)[0];
    p[1] = (_ENT(ent)->r.currentOrigin)[1];
    p[2] = (_ENT(ent)->r.currentOrigin)[2];
    (_ENT(ent)->s.pos.trTime) = 0;
    (_ENT(ent)->s.pos.trDuration) = 0;
    p = (_ENT(ent)->s.pos.trDelta);
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;

    p = (_ENT(ent)->s.apos.trBase);
    p[0] = (_ENT(ent)->r.currentAngles)[0];
    p[1] = (_ENT(ent)->r.currentAngles)[1];
    p[2] = (_ENT(ent)->r.currentAngles)[2];
    (_ENT(ent)->s.apos.trTime) = 0;
    (_ENT(ent)->s.apos.trDuration) = 0;
    p = (_ENT(ent)->s.apos.trDelta);
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;

    (_ENT(ent)->s.pos.trType) = 1;
    (_ENT(ent)->s.apos.trType) = 1;

    SV_LinkEntity(ent);
}

unsigned char G_FreeEntity(gentity_t *ed)
{
    int entnum;
    int i;
    gentity_t *ent;

    G_EntUnlink(ed);

    while (COD2_GEntityFromHandle(_ENT(ed)->tagChildren)) {
        G_EntUnlink(COD2_GEntityFromHandle(_ENT(ed)->tagChildren));
    }

    SV_UnlinkEntity(ed);

    {
        void *tree = SV_DObjGetTree(ed);
        if (tree) {
            XAnimClearTree(tree);
        }
    }

    Com_SafeServerDObjFree((_ENT(ed)->s.number));

    entnum = (_ENT(ed)->s.number);

    ent = LEVEL_GENTITIES;
    for (i = 0; i < LEVEL_NUMENTS; i++, ent++) {
        if (!(_ENT(ent)->r.inuse)) {
            continue;
        }
        if (COD2_GEntityFromHandle(_ENT(ent)->parent) == ed) {
            (_ENT(ent)->parent = COD2_GEntityHandle(NULL));
        }
        if ((_ENT(ent)->r.ownerNum) == entnum) {
            (_ENT(ent)->r.ownerNum) = 0x3FF;
            if ((_ENT(ent)->s.eType) == 9) {
                _ENT(ent)->active = 0;
            }
        }
        if ((_ENT(ent)->s.groundEntityNum) == entnum) {
            (_ENT(ent)->s.groundEntityNum) = 0x3FF;
        }
    }

    ent = LEVEL_GENTITIES;
    for (i = 0; i < 64; i++, ent++) {
        if (!(_ENT(ent)->r.inuse)) {
            continue;
        }
        gclient_t *client = (_ENT(ent)->client);
        if (!client)
            continue;
        if ((((gclient_t *)(client))->pLookatEnt) == ed) {
            (((gclient_t *)(client))->pLookatEnt) = 0;
        }
        if ((((gclient_t *)(client))->useHoldEntity) == entnum) {
            (((gclient_t *)(client))->useHoldEntity) = 0x3FF;
        }
        if ((((gclient_t *)(client))->ps.cursorHintEntIndex) == entnum) {
            (((gclient_t *)(client))->ps.cursorHintEntIndex) = 0x3FF;
        }
    }

    {
        for (i = 0; i < 32; i++) {
            if (LEVEL_DROPPED_WEAPON_CUE[i] == ed) {
                LEVEL_DROPPED_WEAPON_CUE[i] = 0;
            }
        }
    }

    if ((_ENT(ed)->pTurretInfo)) {
        G_FreeTurret(ed);
    }

    if ((_ENT(ed)->s.eType) == 2) {
        int corpseIdx = G_GetPlayerCorpseIndex(ed);
        CORPSE_ENTNUM(corpseIdx) = -1;
    }

    Scr_FreeEntity(ed);

    {
        int useCount = (_ENT(ed)->useCount);
        memset(ed, 0, ENTITY_STRIDE);
        (_ENT(ed)->eventTime) = LEVEL_TIME;

        if (ed >= &LEVEL_GENTITIES[GUTILS_DYNAMIC_ENTITY_START]) {

            if (LEVEL_LASTFREEENT) {
                (_ENT(LEVEL_LASTFREEENT)->nextFree = COD2_GEntityHandle(ed));
            } else {
                LEVEL_FIRSTFREEENT = ed;
            }
            LEVEL_LASTFREEENT = ed;
            (_ENT(ed)->nextFree = COD2_GEntityHandle(NULL));
        }

        (_ENT(ed)->useCount) = useCount + 1;
    }
}

int G_GetFreePlayerCorpseIndex(void)
{
    int i;
    unsigned short match;
    vec3_t playerPos;
    float bestDistSq;
    int bestIdx;

    match = SCR_CONST()->player;

    {
        gentity_t *ent = G_ENTITIES_DIRECT;
        gentity_t *end = G_ENTITIES_DIRECT + LEVEL_NUMENTS;
        gentity_t *found = 0;

        while (ent < end) {
            if ((_ENT(ent)->r.inuse)) {
                unsigned short cn = (_ENT(ent)->classname);
                if (cn && cn == match) {
                    found = ent;
                    break;
                }
            }
            ++ent;
        }

        if (found) {
            VectorCopy((_ENT(found)->s.pos.trBase), playerPos);
        } else {
            VectorClear(playerPos);
        }
    }

    bestDistSq = -1.0f;
    bestIdx = 0;

    for (i = 0; i < 8; i++) {
        int entnum = CORPSE_ENTNUM(i);
        if (entnum == -1) {
            return i;
        }

        {

            gentity_t *corpseEnt = &LEVEL_GENTITIES[entnum];
            float distSq = Vec3DistanceSq((_ENT(corpseEnt)->r.currentOrigin), playerPos);
            if (distSq > bestDistSq) {
                bestDistSq = distSq;
                bestIdx = i;
            }
        }
    }

    {
        int entnum2 = CORPSE_ENTNUM(bestIdx);
        gentity_t *corpseEnt2 = &LEVEL_GENTITIES[entnum2];
        G_FreeEntity(corpseEnt2);
        CORPSE_ENTNUM(bestIdx) = -1;
    }

    return bestIdx;
}

void G_DObjUpdate(gentity_t *ent)
{
    int i;
    int numModels;
    DObjModel_s dobjModels[8];

    if ((_ENT(ent)->client)) {
        return;
    }

    Com_SafeServerDObjFree((_ENT(ent)->s.number));

    if (!(_ENT(ent)->model)) {

        gentity_t *child = COD2_GEntityFromHandle(_ENT(ent)->tagChildren);
        while (child) {
            tagInfo_t *childTag = COD2_TagInfoFromHandle(_ENT(child)->tagInfo);
            gentity_t *next = ((childTag)->next);
            if (!((childTag)->name)) {
                ((childTag)->index) = -1;
            } else {
                G_EntUnlink(child);
            }
            child = next;
        }
        return;
    }

    dobjModels[0].model = cached_models[(_ENT(ent)->model)];
    dobjModels[0].boneName = 0;
    dobjModels[0].ignoreCollision = 0;

    if ((_ENT(ent)->s.eType) == 0 || (_ENT(ent)->s.eType) == 6 || (_ENT(ent)->s.eType) == 9) {
        (_ENT(ent)->s.index.brushmodel) = (_ENT(ent)->model);
    }

    numModels = 1;
    for (i = 0; i < 7; i++) {
        byte modelIdx = (_ENT(ent)->attachModelNames[i]);
        if (!modelIdx) {
            continue;
        }
        dobjModels[numModels].model = cached_models[modelIdx];
        dobjModels[numModels].boneName = SL_ConvertToString((_ENT(ent)->attachTagNames[i]));
        dobjModels[numModels].ignoreCollision = ((_ENT(ent)->attachIgnoreCollision) >> i) & 1;
        numModels++;
    }

    Com_ServerDObjCreate(dobjModels, numModels, 0, (_ENT(ent)->s.number));

    {
        gentity_t *child = COD2_GEntityFromHandle(_ENT(ent)->tagChildren);
        while (child) {
            tagInfo_t *childTag = COD2_TagInfoFromHandle(_ENT(child)->tagInfo);
            gentity_t *next = ((childTag)->next);
            if (!((childTag)->name)) {
                ((childTag)->index) = -1;
            } else {
                int boneIdx = SV_DObjGetBoneIndex(((childTag)->parent), ((childTag)->name));
                ((childTag)->index) = boneIdx;
                if (boneIdx < 0) {
                    G_EntUnlink(child);
                }
            }
            child = next;
        }
    }
}

unsigned char G_EntDetachAll(gentity_t *ent)
{
    int i;

    for (i = 0; i < 7; i++) {
        (_ENT(ent)->attachModelNames[i]) = 0;
        Scr_SetString(&ent->attachTagNames[i], 0);
    }

    (_ENT(ent)->attachIgnoreCollision) = 0;
    G_DObjUpdate(ent);
}

qboolean G_EntDetach(gentity_t *ent, const char *modelName, unsigned int tagName)
{
    int i;

    for (i = 0; i < 7; i++) {
        if ((_ENT(ent)->attachTagNames[i]) != tagName) {
            continue;
        }

        {
            const char *attachedModel = SV_GetConfigstringConst(0x14e + (_ENT(ent)->attachModelNames[i]));
            if (stricmp(attachedModel, modelName) != 0) {
                continue;
            }
        }

        (_ENT(ent)->attachModelNames[i]) = 0;
        Scr_SetString(&ent->attachTagNames[i], 0);

        if (i <= 5) {
            int j;
            for (j = i; j < 6; j++) {
                (_ENT(ent)->attachModelNames[j]) = (_ENT(ent)->attachModelNames[j + 1]);
                (_ENT(ent)->attachTagNames[j]) = (_ENT(ent)->attachTagNames[j + 1]);
                if (((_ENT(ent)->attachIgnoreCollision) >> (j + 1)) & 1) {
                    (_ENT(ent)->attachIgnoreCollision) |= (1 << j);
                } else {
                    (_ENT(ent)->attachIgnoreCollision) &= ~(1 << j);
                }
            }
            i = 6;
        }

        (_ENT(ent)->attachModelNames[i]) = 0;
        (_ENT(ent)->attachTagNames[i]) = 0;
        (_ENT(ent)->attachIgnoreCollision) &= ~(unsigned char)(1 << i) * 2 >> 1;

        {
            unsigned char mask = ~(unsigned char)((0xFFFFFFFE << i) | ~(0xFFFFFFFE << i));

            int rotmask = 0xFFFFFFFE;
            if (i)
                rotmask = (int)(((unsigned int)rotmask << i) | ((unsigned int)rotmask >> (32 - i)));
            (_ENT(ent)->attachIgnoreCollision) &= (byte)rotmask;
        }

        G_DObjUpdate(ent);
        return 1;
    }

    return 0;
}

qboolean G_EntAttach(gentity_t *ent, const char *modelName, unsigned int tagName, qboolean ignoreCollision)
{
    int i;

    for (i = 0; i < 7; i++) {
        if ((_ENT(ent)->attachModelNames[i]) == 0) {
            break;
        }
    }

    if (i == 7) {
        return 0;
    }

    (_ENT(ent)->attachModelNames[i]) = (byte)G_ModelIndex(modelName);
    Scr_SetString(&ent->attachTagNames[i], tagName);

    if (ignoreCollision) {
        (_ENT(ent)->attachIgnoreCollision) |= (byte)(1 << i);
    }

    G_DObjUpdate(ent);
    return 1;
}

gentity_t *G_Spawn(void)
{
    gentity_t *e;

    e = LEVEL_FIRSTFREEENT;
    if (e) {
        if (LEVEL_TIME - (_ENT(e)->eventTime) > 499 || LEVEL_NUMENTS > 0x3FD) {
            LEVEL_FIRSTFREEENT = COD2_GEntityFromHandle(_ENT(e)->nextFree);
            if (!COD2_GEntityFromHandle(_ENT(e)->nextFree)) {
                LEVEL_LASTFREEENT = 0;
            }
            (_ENT(e)->nextFree = COD2_GEntityHandle(NULL));
            goto init;
        }
    }

    if (LEVEL_NUMENTS == 0x3FE) {

        G_PrintEntities();
        Com_Error(1, "G_Spawn: no free entities");
    }

    {
        int num = LEVEL_NUMENTS;
        e = &LEVEL_GENTITIES[num];
        LEVEL_NUMENTS = num + 1;
        SV_LocateGameData(LEVEL_GENTITIES, num + 1, ENTITY_STRIDE, LEVEL_CLIENTS, 0x28A4);
    }

init:
    G_InitGentity_core(e);
    return e;
}

gentity_t *G_TempEntity(const vec_t *origin, int event)
{
    gentity_t *e;
    vec3_t snapped;

    e = G_Spawn();
    (_ENT(e)->s.eType) = event + 10;

    Scr_SetString(&e->classname, SCR_CONST()->tempEntity);

    (_ENT(e)->eventTime) = LEVEL_TIME;
    (_ENT(e)->r.eventTime) = LEVEL_TIME;
    (_ENT(e)->freeAfterEvent) = 1;

    snapped[0] = (float)(int)origin[0];
    snapped[1] = (float)(int)origin[1];
    snapped[2] = (float)(int)origin[2];

    VectorCopy(snapped, (_ENT(e)->s.pos.trBase));
    (_ENT(e)->s.pos.trType) = 0;
    (_ENT(e)->s.pos.trTime) = 0;
    (_ENT(e)->s.pos.trDuration) = 0;
    VectorClear((_ENT(e)->s.pos.trDelta));
    VectorCopy(snapped, (_ENT(e)->r.currentOrigin));

    SV_LinkEntity(e);

    return e;
}

gentity_t *G_SpawnPlayerClone(void)
{
    int idx;
    gentity_t *e;
    int oldEFlags;

    idx = LEVEL_CLONEIDX;
    e = &LEVEL_GENTITIES[idx + GUTILS_PLAYER_CLONE_BASE];

    idx = (idx + 1) & 0x80000007;
    if (idx < 0) {
        idx = ((idx - 1) | 0xFFFFFFF8) + 1;
    }
    LEVEL_CLONEIDX = idx;

    oldEFlags = (_ENT(e)->s.eFlags) & 2;
    oldEFlags ^= 2;

    if ((_ENT(e)->r.inuse)) {
        G_FreeEntity(e);
    }

    G_InitGentity_core(e);
    (_ENT(e)->s.eFlags) = oldEFlags;

    return e;
}

qboolean G_DObjGetWorldTagMatrix(gentity_t *ent, unsigned int tagName, vec3_t *tagMat)
{
    DObjAnimMat_s *mat;
    int boneIndex;
    /* A 4x3 transform includes the entity translation in its fourth row. */
    vec3_t ent_axis[4];
    vec3_t axis[3];

    boneIndex = SV_DObjGetBoneIndex(ent, tagName);
    if (boneIndex < 0) {
        return 0;
    }
    G_DObjCalcBone(ent, boneIndex);
    mat = &((DObjAnimMat_s *)SV_DObjGetMatrixArray(ent))[boneIndex];
    if (!mat) {
        return 0;
    }

    AnglesToAxis((_ENT(ent)->r.currentAngles), (vec_t *)ent_axis);
    VectorCopy((_ENT(ent)->r.currentOrigin), ent_axis[3]);

    {
        float scale = mat->transWeight;
        float x = scale * mat->quat[0];
        float y = scale * mat->quat[1];
        float z = scale * mat->quat[2];
        float xx = x * mat->quat[0];
        float xy = x * mat->quat[1];
        float xz = x * mat->quat[2];
        float xw = x * mat->quat[3];
        float yy = y * mat->quat[1];
        float yz = y * mat->quat[2];
        float yw = y * mat->quat[3];
        float zz = z * mat->quat[2];
        float zw = z * mat->quat[3];

        axis[0][0] = 1.0f - (yy + zz);
        axis[0][1] = zw + xy;
        axis[0][2] = xz - yw;
        axis[1][0] = xy - zw;
        axis[1][1] = 1.0f - (xx + zz);
        axis[1][2] = xw + yz;
        axis[2][0] = xz + yw;
        axis[2][1] = yz - xw;
        axis[2][2] = 1.0f - (xx + yy);
    }

    MatrixMultiply((vec_t *)axis, (vec_t *)ent_axis, (vec_t *)tagMat);
    MatrixTransformVector43((vec_t *)&mat->trans, (vec_t *)ent_axis, ((vec_t *)tagMat + 9));

    return 1;
}

int G_DObjGetWorldTagPos(gentity_t *ent, unsigned int tagName, vec_t *pos)
{
    DObjAnimMat_s *mat;
    int boneIndex;
    /* A 4x3 transform includes the entity translation in its fourth row. */
    vec3_t ent_axis[4];

    boneIndex = SV_DObjGetBoneIndex(ent, tagName);
    if (boneIndex < 0) {
        return 0;
    }
    G_DObjCalcBone(ent, boneIndex);
    mat = &((DObjAnimMat_s *)SV_DObjGetMatrixArray(ent))[boneIndex];
    if (!mat) {
        return 0;
    }

    AnglesToAxis((_ENT(ent)->r.currentAngles), (vec_t *)ent_axis);
    VectorCopy((_ENT(ent)->r.currentOrigin), ent_axis[3]);

    MatrixTransformVector43((vec_t *)&mat->trans, (vec_t *)ent_axis, pos);

    return 1;
}
