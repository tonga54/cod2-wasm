#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
extern server_t sv;
extern serverStatic_t svs;
extern const dvar_t *sv_maxclients;
extern const dvar_t *com_developer;
extern const dvar_t *com_sv_running;
extern dvar_t *com_dedicated;

extern void Com_Error(int code, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern void SV_SendServerCommand(void *client, int type, const char *fmt, ...);
extern void SV_DropClient(void *client, const char *reason);
extern const char *Dvar_InfoString(int flag);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern void *Hunk_AllocInternal(int size);
extern Bool Com_ValidXModelName(const char *name);
extern struct XModel *XModelPrecache(const char *name, void *alloc, void *allocColl);
extern void *Com_GetServerDObj(int entityNum);
extern void DObjDumpInfo(void *obj);
extern qboolean DObjUpdateServerInfo(void *obj, float dtime, qboolean bNotify);
extern void DObjInitServerTime(void *obj, float dtime);
extern void DObjGetHierarchyBits(void *obj, int boneIndex, int *partBits);
extern void DObjCalcAnim(void *obj, int *partBits);
extern void DObjCalcSkel(void *obj, int *partBits);
extern int DObjGetBoneIndex(void *obj, unsigned int boneName);
extern DObjAnimMat_s *DObjGetRotTransArray(void *obj);
extern void DObjDisplayAnim(void *obj);
extern struct XAnimTree_s *DObjGetTree(void *obj);
extern const char *SV_GetMapBaseName(const char *name);
extern const char *GetBspExtension(void);
extern const char *va(const char *fmt, ...);
extern int FS_ReadFile(const char *path, void *buffer);
extern const char *CM_EntityString(void);
extern const char *Com_Parse(char **data_p);
extern void Com_SetWeaponInfoMemory(int set);
extern void G_ShutdownGame(int restart);
extern void Com_FreeWeaponInfoMemory(int free_flag);
extern void Com_UnloadSoundAliases(int type);
extern void CM_ModelBounds(unsigned int model, vec_t *mins, vec_t *maxs);
extern void SV_LinkEntity(gentity_t *ent);
extern qboolean ConsoleCommand(void);
extern qboolean NET_IsLocalAddress(netadr_t addr);
extern dvar_t *Dvar_RegisterString(const char *dvarName, const char *defaultValue, int flags);
extern void Dvar_SetString(dvar_t *dvar, const char *value);
extern qboolean Scr_IsValidGameType(const char *gametype);
extern int ___tolower(int c);
extern void FX_InitServer(void);
extern void G_InitGame(int svTime, int msecRaw, int restart, int savepersist);
extern void Sys_LoadingKeepAlive(void);
extern int Sys_MillisecondsRaw(void);
extern void Com_DvarDump(int channel);
extern void *G_GetSavePersist(void);
extern int SV_ClipHandleForEntity(const gentity_t *gEnt);
extern int CM_TransformedBoxTraceExternal(void *trace, const void *p1, const void *p2, const vec_t *mins, const vec_t *maxs, unsigned int model, int brushmask, const vec_t *origin, const vec_t *angles);
extern float Vec2DistanceSq(const float *p1, const float *p2);
extern int CM_PointLeafnum(const vec_t *p);
extern int CM_LeafCluster(int leafnum);
extern byte *CM_ClusterPVS(int cluster);
extern float G_GetFogOpaqueDistSqrd(void);
extern qboolean BoxDistSqrdExceeds(const vec_t *absmin, const vec_t *absmax, const vec_t *origin, float distSqrd);
extern void AnglesToAxis(const vec_t *angles, float (*axis)[3]);
extern int DObjNumBones(void *obj);
extern void DObjGetBoneInfo(void *obj, void *boneInfo);
extern int DObjGetNumModels(void *obj);
extern qboolean DObjIgnoreCollision(void *obj, int modelIndex);
extern void *DObjGetModel(void *obj, int modelIndex);
extern int XModelNumBones(void *model);
extern void MatrixTransformVector43(const vec_t *in, const float *mat, vec_t *out);
extern void MatrixTransformVector(const vec_t *in, const float (*axis)[3], vec_t *out);
extern void CL_AddDebugLine(const vec_t *start, const vec_t *end, const float *color, int depthTest, int outline, int duration);
extern int DObjSkelExists(void *obj, int timeStamp);
extern qboolean DObjSkelIsBoneUpToDate(void *obj, int boneIndex);
extern qboolean DObjSkelAreBonesUpToDate(void *obj, int *partBits);
extern int DObjGetAllocSkelSize(void *obj);
extern void DObjCreateSkel(void *obj, char *buf, int timeStamp);

extern qboolean gameInitialized;
static int warnCount;
extern int boxVerts[24][3];
static char g_sv_skel_memory[262144];
static char *g_sv_skel_memory_start;

gentity_t *SV_GentityNum(int num);
playerState_t *SV_GameClientNum(int num);
byte *SV_SvEntityForGentity(const gentity_t *gEnt);
void SV_GameSendServerCommand(int clientNum, svscmd_type type, const char *text);
void SV_GameDropClient(int clientNum, const char *reason);
void SV_GetServerinfo(char *buffer, int bufferSize);
void SV_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGameClient);
void SV_GetUsercmd(int clientNum, usercmd_t *cmd);
static long unsigned int *SV_AllocXModelPrecache(int size);
static long unsigned int *SV_AllocXModelPrecacheColl(int size);
struct XModel *SV_XModelGet(const char *name);
void SV_DObjDumpInfo(gentity_t *ent);
void SV_ResetSkeletonCache(void);
qboolean SV_DObjUpdateServerTime(gentity_t *ent, float dtime, qboolean bNotify);
void SV_DObjInitServerTime(gentity_t *ent, float dtime);
void SV_DObjGetHierarchyBits(gentity_t *ent, int boneIndex, int *partBits);
void SV_DObjCalcAnim(gentity_t *ent, int *partBits);
void SV_DObjCalcSkel(gentity_t *ent, int *partBits);
int SV_DObjGetBoneIndex(gentity_t *ent, unsigned int boneName);
DObjAnimMat_s *SV_DObjGetMatrixArray(gentity_t *ent);
void SV_DObjDisplayAnim(gentity_t *ent);
struct XAnimTree_s *SV_DObjGetTree(gentity_t *ent);
qboolean SV_MapExists(const char *name);
void SV_ResetEntityParsePoint(void);
qboolean SV_DObjExists(gentity_t *ent);
void SV_SetWeaponInfoMemory(void);
qboolean SV_GetEntityToken(char *buffer, int bufferSize);
int SV_GetGuid(int clientNum);
int SV_GetClientPing(int clientNum);
qboolean SV_IsLocalClient(int clientNum);
void SV_SetGametype(void);
static void SV_InitGameVM(int restart, int savepersist);
void SV_RestartGameProgs(qboolean savepersist);
void SV_InitGameProgs(qboolean savepersist);
qboolean SV_GameCommand(void);
void SV_SetBrushModel(gentity_t *ent);
qboolean SV_EntityContact(const vec_t *mins, const vec_t *maxs, const gentity_t *gEnt);
gentity_t *SV_GEntityForSvEntity(svEntity_t *svEnt);
void SV_XModelDebugBoxes(gentity_t *ent);
void SV_ShutdownGameProgs(void);
qboolean SV_inSnapshot(const vec_t *origin, int iEntityNum);
qboolean SV_DObjCreateSkelForBone(gentity_t *ent, int boneIndex);
qboolean SV_DObjCreateSkelForBones(gentity_t *ent, int *partBits);

gentity_t *SV_GentityNum(int num)
{
    server_t *sv = (server_t *)imp_sv;
    return (gentity_t *)((char *)sv->gentities + num * sv->gentitySize);
}

playerState_t *SV_GameClientNum(int num)
{
    server_t *sv = (server_t *)imp_sv;
    return (playerState_t *)((char *)sv->gameClients + num * sv->gameClientSize);
}

byte *SV_SvEntityForGentity(const gentity_t *gEnt)
{
    int number;

    if (gEnt == NULL || gEnt->s.number < 0 || gEnt->s.number > 1023) {
        Com_Error(1, "SV_SvEntityForGentity: bad gEnt");
    }
    number = gEnt->s.number;

    /* was cast to (int) -> truncated the 8-byte svEntity pointer on x64 */
    return (byte *)((char *)((server_t *)imp_sv) + __builtin_offsetof(server_t, svEntities) + (size_t)number * sizeof(svEntity_t));
}

void SV_GameSendServerCommand(int clientNum, svscmd_type type, const char *text)
{
    int maxClients;
    char *clients_base;
    char *client;

    if (clientNum == -1) {
        SV_SendServerCommand((void *)0, type, "%s", text);
    } else if (clientNum >= 0) {
        maxClients = sv_maxclients->current.integer;
        if (clientNum < maxClients) {
            serverStatic_t *svs = (serverStatic_t *)imp_svs;
            SV_SendServerCommand(&svs->clients[clientNum], type, "%s", text);
        }
    }
}

void SV_GameDropClient(int clientNum, const char *reason)
{
    serverStatic_t *svs;

    if (clientNum < 0)
        return;
    if (clientNum >= sv_maxclients->current.integer)
        return;
    svs = (serverStatic_t *)imp_svs;
    SV_DropClient(&svs->clients[clientNum], reason);
}

void SV_GetServerinfo(char *buffer, int bufferSize)
{
    if (bufferSize <= 0) {
        Com_Error(1, "\025SV_GetServerinfo: bufferSize == %i", bufferSize);
    }
    I_strncpyz(buffer, Dvar_InfoString(0x404), bufferSize);
}

void SV_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGameClient)
{
    server_t *sv = (server_t *)imp_sv;
    sv->gentities = gEnts;
    sv->gentitySize = sizeofGEntity_t;
    sv->num_entities = numGEntities;
    sv->gameClients = clients;
    sv->gameClientSize = sizeofGameClient;
}

void SV_GetUsercmd(int clientNum, usercmd_t *cmd)
{

    *cmd = svs.clients[clientNum].lastUsercmd;
}

static long unsigned int *SV_AllocXModelPrecache(int size)
{
    return Hunk_AllocInternal(size);
}

static long unsigned int *SV_AllocXModelPrecacheColl(int size)
{
    return Hunk_AllocInternal(size);
}

struct XModel *SV_XModelGet(const char *name)
{
    if (!Com_ValidXModelName(name)) {
        Com_Error(1, "\025bad model name '%s'", name);
    }
    return XModelPrecache(name + 7, SV_AllocXModelPrecache, SV_AllocXModelPrecacheColl);
}

void SV_DObjDumpInfo(gentity_t *ent)
{
    void *obj;
    if (com_developer->current.integer != 0) {
        obj = Com_GetServerDObj(ent->s.number);
        if (obj) {
            DObjDumpInfo(obj);
        } else {
            Com_Printf("no model.\n");
        }
    }
}

void SV_ResetSkeletonCache(void)
{
    server_t *sv = (server_t *)imp_sv;
    int idx = 1;
    int incd = sv->skelTimeStamp + 1;
    if (incd != 0)
        idx = incd;
    sv->skelTimeStamp = idx;
    g_sv_skel_memory_start = (char *)((((uintptr_t)g_sv_skel_memory) + 0xf) & ~(uintptr_t)0xf);
    sv->skelMemPos = 0;
}

qboolean SV_DObjUpdateServerTime(gentity_t *ent, float dtime, qboolean bNotify)
{
    void *obj = Com_GetServerDObj(ent->s.number);
    if (obj) {
        return DObjUpdateServerInfo(obj, dtime, bNotify);
    }
    return 0;
}

void SV_DObjInitServerTime(gentity_t *ent, float dtime)
{
    void *obj = Com_GetServerDObj(ent->s.number);
    if (obj) {
        DObjInitServerTime(obj, dtime);
    }
}

void SV_DObjGetHierarchyBits(gentity_t *ent, int boneIndex, int *partBits)
{
    void *obj = Com_GetServerDObj(ent->s.number);
    DObjGetHierarchyBits(obj, boneIndex, partBits);
}

void SV_DObjCalcAnim(gentity_t *ent, int *partBits)
{
    void *obj = Com_GetServerDObj(ent->s.number);
    DObjCalcAnim(obj, partBits);
}

void SV_DObjCalcSkel(gentity_t *ent, int *partBits)
{
    void *obj = Com_GetServerDObj(ent->s.number);
    DObjCalcSkel(obj, partBits);
}

int SV_DObjGetBoneIndex(gentity_t *ent, unsigned int boneName)
{
    void *obj = Com_GetServerDObj(ent->s.number);
    if (!obj)
        return -1;
    return DObjGetBoneIndex(obj, boneName);
}

DObjAnimMat_s *SV_DObjGetMatrixArray(gentity_t *ent)
{
    void *obj = Com_GetServerDObj(ent->s.number);
    return DObjGetRotTransArray(obj);
}

void SV_DObjDisplayAnim(gentity_t *ent)
{
    void *obj = Com_GetServerDObj(ent->s.number);
    if (obj) {
        DObjDisplayAnim(obj);
    }
}

struct XAnimTree_s *SV_DObjGetTree(gentity_t *ent)
{
    void *obj = Com_GetServerDObj(ent->s.number);
    if (!obj)
        return 0;
    return DObjGetTree(obj);
}

qboolean SV_MapExists(const char *name)
{
    const char *basename = SV_GetMapBaseName(name);
    const char *ext = GetBspExtension();
    const char *path = va("maps/mp/%s.%s", basename, ext);
    return FS_ReadFile(path, (void *)0) >= 0;
}

void SV_ResetEntityParsePoint(void)
{

#if defined(__x86_64__) || defined(_M_X64)

    const char *parse_point = CM_EntityString();
#else
    int parse_point = CM_EntityString();
#endif
    sv.entityParsePoint = parse_point;
}

qboolean SV_DObjExists(gentity_t *ent)
{
    return Com_GetServerDObj(ent->s.number) != 0;
}

void SV_SetWeaponInfoMemory(void)
{
    Com_SetWeaponInfoMemory(1);
}

qboolean SV_GetEntityToken(char *buffer, int bufferSize)
{
    server_t *sv = (server_t *)imp_sv;
    const char *s = Com_Parse((char **)&sv->entityParsePoint);
    I_strncpyz(buffer, s, bufferSize);
    if (sv->entityParsePoint || *s) {
        return 1;
    }
    return 0;
}

int SV_GetGuid(int clientNum)
{
    const dvar_t *dvar;
    if (clientNum < 0) {
        return 0;
    }
    dvar = sv_maxclients;
    if (clientNum >= dvar->current.integer) {
        return 0;
    }
    {
        serverStatic_t *svs = (serverStatic_t *)imp_svs;
        return svs->clients[clientNum].guid;
    }
}

int SV_GetClientPing(int clientNum)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    return svs->clients[clientNum].ping;
}

qboolean SV_IsLocalClient(int clientNum)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    client_t *client = &svs->clients[clientNum];
    netadr_t addr = client->netchan.remoteAddress;
    return NET_IsLocalAddress(addr);
}

void SV_SetGametype(void)
{
    char gametype[64];
    char *s;

    Dvar_RegisterString("g_gametype", "dm", 0x1024);

    if (com_sv_running->current.enabled && G_GetSavePersist()) {
        server_t *sv = (server_t *)imp_sv;
        I_strncpyz(gametype, sv->gametype, 64);
    } else {
        I_strncpyz(gametype, (*(dvar_t **)imp_sv_gametype)->current.string, 64);
    }

    for (s = gametype; *s; s++) {
        *s = ___tolower(*s);
    }

    if (!Scr_IsValidGameType(gametype)) {
        Com_Printf("g_gametype %s is not a valid gametype, defaulting to dm\n", gametype);
        gametype[0] = 'd';
        gametype[1] = 'm';
        gametype[2] = '\0';
    }

    Dvar_SetString(*(dvar_t **)imp_sv_gametype, gametype);
}

static void SV_InitGameVM(int restart, int savepersist)
{
    int i;

    FX_InitServer();

    {
        const char *parse_point = CM_EntityString();
        sv.entityParsePoint = parse_point;
    }

    Sys_LoadingKeepAlive();

    G_InitGame(svs.time, Sys_MillisecondsRaw(),
               restart, savepersist);

    Sys_LoadingKeepAlive();

    for (i = 0; i < sv_maxclients->current.integer; i++) {
        *(int *)((char *)&svs.clients[i] + 0x20c44) = 0;
    }

    if (com_dedicated->current.integer) {
        Com_DvarDump(4);
    }
}

void SV_RestartGameProgs(qboolean savepersist)
{
    G_ShutdownGame(0);
    *(int *)imp_com_fixedConsolePosition = 0;
    SV_InitGameVM(1, savepersist);
}

void SV_InitGameProgs(qboolean savepersist)
{
    gameInitialized = 1;
    SV_InitGameVM(0, savepersist);
}

qboolean SV_GameCommand(void)
{
    server_t *sv = (server_t *)imp_sv;
    if (sv->state != 2)
        return 0;
    return ConsoleCommand();
}

void SV_SetBrushModel(gentity_t *ent)
{
    vec3_t mins, maxs;
    CM_ModelBounds(ent->s.index.brushmodel, mins, maxs);
    {
        float *p = &ent->r.mins[0];
        p[0] = mins[0];
        p[1] = mins[1];
        p[2] = mins[2];
        p = &ent->r.maxs[0];
        p[0] = maxs[0];
        p[1] = maxs[1];
        p[2] = maxs[2];
    }
    ent->r.bmodel = 1;
    ent->r.contents = -1;
    SV_LinkEntity(ent);
}

qboolean SV_EntityContact(const vec_t *mins, const vec_t *maxs, const gentity_t *gEnt)
{
    unsigned char svFlags = gEnt->r.svFlags;
    float center[2];
    float radius;
    float distSq;

    if (!(svFlags & 0x60)) {

        trace_t trace;
        int clipHandle = SV_ClipHandleForEntity(gEnt);

        vec_t *vec3_origin = (vec_t *)imp_vec3_origin;
        CM_TransformedBoxTraceExternal(&trace, vec3_origin, vec3_origin,
                                       mins, maxs, clipHandle, -1,
                                       gEnt->r.currentOrigin, gEnt->r.currentAngles);
        return trace.startsolid;
    }

    if (svFlags & 0x20) {

        float entZ = gEnt->r.currentOrigin[2];
        if (entZ < maxs[2]) {
            float topZ = entZ + gEnt->r.maxs[2];
            if (mins[2] >= topZ) {
                return 0;
            }

            center[0] = (mins[0] + maxs[0]) * 0.5f;
            center[1] = (mins[1] + maxs[1]) * 0.5f;
            radius = maxs[0] - center[0] + gEnt->r.maxs[0];
            distSq = Vec2DistanceSq((float *)gEnt->r.currentOrigin, center);
            return distSq > radius * radius;
        }
        return 0;
    }

    center[0] = (mins[0] + maxs[0]) * 0.5f;
    center[1] = (mins[1] + maxs[1]) * 0.5f;
    radius = maxs[0] - center[0] + gEnt->r.maxs[0] - 64.0f;
    distSq = Vec2DistanceSq((float *)gEnt->r.currentOrigin, center);
    return distSq <= radius * radius;
}

gentity_t *SV_GEntityForSvEntity(svEntity_t *svEnt)
{
    server_t *sv = (server_t *)imp_sv;

    int index = svEnt - sv->svEntities;
    return (gentity_t *)((char *)sv->gentities + index * sv->gentitySize);
}

void SV_XModelDebugBoxes(gentity_t *ent)
{
    void *obj;
    DObjAnimMat_s *boneMatrix;
    DObjAnimMat_s *mat;
    float *boneInfoArray[128];
    int numModels, modelIndex, numBones, localBoneIndex;
    float *boneInfo;
    float boneMat[4][3];
    float axis[3][3];
    float color[4];
    float start[3], end[3], vec[3], org[3];
    float sx, sy, sz;
    float xx, xy, xz, xw;
    float yy, yz, yw;
    float zz, zw;
    int edge;

    obj = Com_GetServerDObj(ent->s.number);
    DObjNumBones(obj);
    DObjGetBoneInfo(obj, boneInfoArray);
    boneMatrix = DObjGetRotTransArray(obj);

    color[0] = 1.0f;
    color[1] = 1.0f;
    color[2] = 1.0f;
    color[3] = 0.0f;

    AnglesToAxis(ent->r.currentAngles, axis);

    numModels = DObjGetNumModels(obj);

    for (modelIndex = 0; modelIndex < numModels; modelIndex++) {
        if (DObjIgnoreCollision(obj, modelIndex))
            continue;

        numBones = XModelNumBones(DObjGetModel(obj, modelIndex));
        if (numBones <= 0)
            continue;

        mat = boneMatrix;
        for (localBoneIndex = 0; localBoneIndex < numBones; localBoneIndex++, mat++) {
            boneInfo = boneInfoArray[localBoneIndex];

            sx = mat->transWeight * mat->quat[0];
            sy = mat->transWeight * mat->quat[1];
            sz = mat->transWeight * mat->quat[2];

            xx = sx * mat->quat[0];
            xy = sx * mat->quat[1];
            xz = sx * mat->quat[2];
            xw = sx * mat->quat[3];
            yy = sy * mat->quat[1];
            yz = sy * mat->quat[2];
            yw = sy * mat->quat[3];
            zz = sz * mat->quat[2];
            zw = sz * mat->quat[3];

            boneMat[0][0] = 1.0f - (yy + zz);
            boneMat[0][1] = xy + zw;
            boneMat[0][2] = xz - yw;
            boneMat[1][0] = xy - zw;
            boneMat[1][1] = 1.0f - (xx + zz);
            boneMat[1][2] = yz + xw;
            boneMat[2][0] = xz + yw;
            boneMat[2][1] = yz - xw;
            boneMat[2][2] = 1.0f - (xx + yy);
            boneMat[3][0] = mat->trans[0];
            boneMat[3][1] = mat->trans[1];
            boneMat[3][2] = mat->trans[2];

            for (edge = 0; edge < 24; edge += 2) {

                org[0] = boneInfo[boxVerts[edge][0] * 3 + 0];
                org[1] = boneInfo[boxVerts[edge][1] * 3 + 1];
                org[2] = boneInfo[boxVerts[edge][2] * 3 + 2];
                MatrixTransformVector43(org, (float *)boneMat, vec);
                MatrixTransformVector(vec, axis, start);
                start[0] += ent->r.currentOrigin[0];
                start[1] += ent->r.currentOrigin[1];
                start[2] += ent->r.currentOrigin[2];

                org[0] = boneInfo[boxVerts[edge + 1][0] * 3 + 0];
                org[1] = boneInfo[boxVerts[edge + 1][1] * 3 + 1];
                org[2] = boneInfo[boxVerts[edge + 1][2] * 3 + 2];
                MatrixTransformVector43(org, (float *)boneMat, vec);
                MatrixTransformVector(vec, axis, end);
                end[0] += ent->r.currentOrigin[0];
                end[1] += ent->r.currentOrigin[1];
                end[2] += ent->r.currentOrigin[2];

                CL_AddDebugLine(start, end, color, 0, 0, 1);
            }
        }
        boneMatrix = mat;
    }
}

void SV_ShutdownGameProgs(void)
{
    sv.state = 0;
    Com_UnloadSoundAliases(2);
    if (gameInitialized) {
        G_ShutdownGame(1);
        Com_FreeWeaponInfoMemory(1);
        gameInitialized = 0;
    }
}

qboolean SV_inSnapshot(const vec_t *origin, int iEntityNum)
{
    server_t *sv;
    gentity_t *ent;
    svEntity_t *svEnt;
    int numClusters;
    int cluster;
    byte *clientpvs;
    int broadcastTime;
    unsigned char svFlags;
    int i;
    int lastCluster;
    float fogDistSqrd;

    sv = (server_t *)imp_sv;
    ent = (gentity_t *)((char *)sv->gentities + iEntityNum * sv->gentitySize);

    if (ent->r.linked == 0)
        return 0;

    broadcastTime = ent->r.broadcastTime;
    if (broadcastTime)
        return 1;

    svFlags = ent->r.svFlags;

    if (svFlags & 1)
        return 0;

    if (svFlags & 0x18)
        return 1;

    {
        int number = ent->s.number;
        if (ent == 0 || number < 0 || number > 1023) {
            Com_Error(1, "SV_SvEntityForGentity: bad gEnt");
            sv = (server_t *)imp_sv;
            number = ent->s.number;
        }
        svEnt = &sv->svEntities[number];
    }

    cluster = CM_PointLeafnum(origin);

    if (svEnt->numClusters == 0)
        return 0;

    cluster = CM_LeafCluster(cluster);
    clientpvs = CM_ClusterPVS(cluster);

    numClusters = svEnt->numClusters;
    if (numClusters <= 0) {

        goto check_fog;
    }

    for (i = 0; i < numClusters; i++) {
        int cl = svEnt->clusternums[i];
        if ((clientpvs[cl >> 3] >> (cl & 7)) & 1)
            goto check_fog;
    }

    lastCluster = svEnt->lastCluster;
    if (lastCluster == 0)
        return 0;

    for (i = 0; i < lastCluster; i++) {
        if ((clientpvs[i >> 3] >> (i & 7)) & 1)
            break;
    }
    if (i == lastCluster)
        return 0;

check_fog:
    fogDistSqrd = G_GetFogOpaqueDistSqrd();
    if (fogDistSqrd == 3.4028234663852886e+38f)
        return 1;

    return !BoxDistSqrdExceeds(ent->r.absmin, ent->r.absmax, origin, fogDistSqrd);
}

qboolean SV_DObjCreateSkelForBone(gentity_t *ent, int boneIndex)
{
    void *obj;
    server_t *sv;
    int allocSize;
    int alignedSize;
    char *buf;
    int timestamp;

    obj = Com_GetServerDObj(ent->s.number);
    sv = (server_t *)imp_sv;

    if (DObjSkelExists(obj, sv->skelTimeStamp)) {
        return DObjSkelIsBoneUpToDate(obj, boneIndex);
    }

    allocSize = DObjGetAllocSkelSize(obj);
    alignedSize = (allocSize + 15) & ~15;
    if (getenv("DBGSPAM")) Com_Printf("[ckpt] skel obj=%p allocSize=%d aligned=%d numBones~=%d memPos=%d\n",
               obj, allocSize, alignedSize, (allocSize - 0x30) >> 5, sv->skelMemPos);
    buf = g_sv_skel_memory_start + sv->skelMemPos;
    sv->skelMemPos += alignedSize;

    if (sv->skelMemPos > 0x3fff0) {
        buf = (char *)(((uintptr_t)g_sv_skel_memory + 15) & ~(uintptr_t)15);

        do {
            timestamp = sv->skelTimeStamp;
            if (timestamp != warnCount) {
                warnCount = timestamp;
                Com_Printf("^3WARNING: SV_SKEL_MEMORY_SIZE exceeded\n");
            }
            timestamp = sv->skelTimeStamp + 1;
            if (timestamp == 0)
                timestamp = 1;
            sv->skelTimeStamp = timestamp;
            g_sv_skel_memory_start = buf;
            sv->skelMemPos = alignedSize;
        } while (alignedSize > 0x3fff0);
    }

    sv = (server_t *)imp_sv;
    DObjCreateSkel(obj, buf, sv->skelTimeStamp);
    return 0;
}

qboolean SV_DObjCreateSkelForBones(gentity_t *ent, int *partBits)
{
    void *obj;
    server_t *sv;
    int allocSize;
    int alignedSize;
    char *buf;
    int timestamp;

    obj = Com_GetServerDObj(ent->s.number);
    sv = (server_t *)imp_sv;

    if (DObjSkelExists(obj, sv->skelTimeStamp)) {
        return DObjSkelAreBonesUpToDate(obj, partBits);
    }

    allocSize = DObjGetAllocSkelSize(obj);
    alignedSize = (allocSize + 15) & ~15;
    buf = g_sv_skel_memory_start + sv->skelMemPos;
    sv->skelMemPos += alignedSize;

    if (sv->skelMemPos > 0x3fff0) {
        buf = (char *)(((uintptr_t)g_sv_skel_memory + 15) & ~(uintptr_t)15);

        do {
            timestamp = sv->skelTimeStamp;
            if (timestamp != warnCount) {
                warnCount = timestamp;
                Com_Printf("^3WARNING: SV_SKEL_MEMORY_SIZE exceeded\n");
            }
            timestamp = sv->skelTimeStamp + 1;
            if (timestamp == 0)
                timestamp = 1;
            sv->skelTimeStamp = timestamp;
            g_sv_skel_memory_start = buf;
            sv->skelMemPos = alignedSize;
        } while (alignedSize > 0x3fff0);
    }

    sv = (server_t *)imp_sv;
    DObjCreateSkel(obj, buf, sv->skelTimeStamp);
    return 0;
}
