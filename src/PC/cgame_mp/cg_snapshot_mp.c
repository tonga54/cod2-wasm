#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "bytematch.h"

extern char **cg_dvar1;
extern char **cg_dvar2;

extern void BG_EvaluateTrajectory(void *traj, int time, float *result);
extern void BG_PlayerStateToEntityState(void *ps, void *es, int extrapolate, int snap);
extern void CG_ResetPlayerEntity(centity_t *cent);
extern void XAnimCloneAnimTree(void *srcTree, void *destTree);
extern int I_stricmp(const char *s1, const char *s2);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern void CL_ResetSkeletonCache(int val);
extern void CG_SetFrameInterpolation(void);
extern void CG_ExecuteNewServerCommands(int serverCommandSequence);
extern void CG_CheckOpenWaitingScriptMenu(void);
extern void CG_Respawn(void);
extern void CG_BuildSolidList(void);
extern void CG_ClearSolidList(void);
extern void CG_PlaySmokeGrenadesAtTime(int time);
extern void CG_SetEquippedOffHand(int offHandIndex);
extern void CG_TransitionPlayerState(void *newPs, void *oldPs);
extern void CG_CheckEvents(void *cent);
extern void CG_UpdatePlayerDObj(void *cent);
extern void CG_UpdateHandViewmodels(const char *configStr);
extern void CG_AddLagometerSnapshotInfo(void *snap);
extern int CL_GetSnapshot(int snapshotNumber, void *snap);
extern void CL_GetCurrentSnapshotNumber(int *snapshotNumber, int *serverTime);
extern const char *CL_GetConfigString(int index);
extern void CG_GameMessage(const char *msg);
extern void CG_SafeDObjFree(int entNum);
extern void CG_InitView(void);
extern void SND_SetListener(int clientNum, float *origin, float *axis);
extern void SND_FadeAllSounds(float volume, int duration);
extern void AnglesToAxis(float *angles, float *axis);
extern const char *va(const char *fmt, ...);
extern const char *UI_SafeTranslateString(const char *ref);
extern void Com_Error(int level, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern int strcmp(const char *s1, const char *s2);
extern void *XAnimGetAnims(void *tree);
extern int XAnimIsLooped(void *anims, int animIndex);
extern int XAnimGetNumChildren(void *anims, int animIndex);
extern void XAnimSetTime(void *tree, int animIndex, float time);

#define CENT_STRIDE 0x224

#define CENT_CS_NUMBER 0x000
#define CENT_CS_ETYPE 0x004
#define CENT_CS_EFLAGS 0x008
#define CENT_CS_POS 0x00c
#define CENT_CS_APOS 0x030
#define CENT_CS_CLIENTNUM 0x090
#define CENT_CS_EVENTSEQUENCE 0x0a4
#define CENT_CS_LEANF 0x0d4
#define CENT_CS_DMGFLAGS 0x0d8

#define CENT_NS_NUMBER 0x0f0
#define CENT_NS_ETYPE 0x0f4
#define CENT_NS_EFLAGS 0x0f8
#define CENT_NS_POS 0x0fc
#define CENT_NS_APOS 0x120
#define CENT_NS_TIME 0x144
#define CENT_NS_TIME2 0x148
#define CENT_NS_INDEX 0x17c
#define CENT_NS_CLIENTNUM 0x180
#define CENT_NS_EVENTSEQUENCE 0x194
#define CENT_NS_LEANF 0x1c4

#define CENT_NEXTVALID 0x1e0
#define CENT_CULLIN 0x1e1
#define CENT_BMUZZLEFLASH 0x1e2
#define CENT_BTRAILMADE 0x1e3
#define CENT_PREVIOUSEVENTSEQ 0x1e4
#define CENT_MISCTIME 0x1e8
#define CENT_LERPORIGIN 0x1ec
#define CENT_LERPANGLES 0x1f8
#define CENT_LIGHTINGORIGIN 0x204
#define CENT_TREE 0x21c

#define SNAP_NUMCLIENTS 0x26b8
#define SNAP_CLIENTS 0x116b0

#define CI_STRIDE 0x4b8
#define CI_INFOVALID 0x000
#define CI_NEXTVALID 0x004
#define CI_CLIENTNUM 0x008
#define CI_NAME 0x00c
#define CI_TEAM 0x02c
#define CI_MODEL 0x040
#define CI_ATTACHMODELNAMES 0x080
#define CI_ATTACHTAGNAMES 0x200
#define CI_DOBJDIRTY 0x3f8
#define CI_PXANIMTREE 0x4a4

#define CLSTATE_STRIDE 0x5c

#define ES_BINSIZE 0x0f0

#define VectorCopy3(src, dst)                  \
    do {                                       \
        ((int *)(dst))[0] = ((int *)(src))[0]; \
        ((int *)(dst))[1] = ((int *)(src))[1]; \
        ((int *)(dst))[2] = ((int *)(src))[2]; \
    } while (0)

#define VectorClear3(v)      \
    do {                     \
        ((int *)(v))[0] = 0; \
        ((int *)(v))[1] = 0; \
        ((int *)(v))[2] = 0; \
    } while (0)

static void CG_ResetEntity(char *cent);
void CG_SetNextSnap(snapshot_t *snap);
void CG_SetInitialSnapshot(snapshot_t *snap);
void CG_ProcessSnapshots(void);

static char *CG_EntityPtr(int entNum)
{
    return (char *)&cg_entities[entNum];
}

static char *CG_ClientInfoPtr(char *cg, int clientNum)
{
    return (char *)&((cg_t *)cg)->bgs.clientinfo[clientNum];
}

static void CG_ResetCorpseEntity(centity_t *cent)
{
    int corpseIndex = cent->nextState.number - 64;
    int clientNum = cent->nextState.clientNum;
    clientInfo_t *source;
    clientInfo_t *corpse;
    void *tree;
    /* BodyEnd clears this flag once the death animation finishes. Bit 8 is
     * the player's prone stance, not a request to copy the live pose. */
    int clone = cent->nextState.eFlags & 0x80000;
    int i;

    if ((unsigned)corpseIndex >= 8 || (unsigned)clientNum >= 64)
        return;
    source = &((cg_t *)cg)->bgs.clientinfo[clientNum];
    corpse = &((cgs_t *)cgs)->corpseinfo[corpseIndex];
    tree = corpse->pXAnimTree;

    if (clone || !corpse->model[0] || corpse->clientNum != source->clientNum) {
        *corpse = *source;
        for (i = 0; i < 6; i++) {
            if (!I_stricmp(corpse->attachTagNames[i], "J_Spine4")) {
                corpse->attachModelNames[i][0] = 0;
                corpse->attachTagNames[i][0] = 0;
            }
        }
        corpse->pXAnimTree = tree;
    }
    if (clone) {
        XAnimCloneAnimTree(source->pXAnimTree, tree);
        /* The owner's tree may already have evaluated the death clip. Make
         * the first corpse frame enter the new-animation path even then. */
        if (corpse->legs.animationNumber == cent->nextState.legsAnim)
            corpse->legs.animationNumber ^= 0x200;
        cent->previousEventSequence = 0;
    } else {
        cent->previousEventSequence = cent->nextState.eventSequence;
    }
    corpse->dobjDirty = 1;
}

static void CG_ResetEntity(char *cent)
{
    int eType;

    VectorClear3((char *)((centity_t *)cent)->lightingOrigin);

    memcpy(cent, cent + ES_BINSIZE, ES_BINSIZE) ;

    ((centity_t *)cent)->bTrailMade = 0;
    ((centity_t *)cent)->cullIn = 0;

    BG_EvaluateTrajectory((char *)&((centity_t *)cent)->nextState.pos, cg->time, (float *)((char *)((centity_t *)cent)->lerpOrigin));

    BG_EvaluateTrajectory((char *)&((centity_t *)cent)->nextState.apos, cg->time, (float *)((char *)((centity_t *)cent)->lerpAngles));

    eType = ((centity_t *)cent)->nextState.eType;

    switch (eType) {
    case 0:
    case 4:

        ((centity_t *)cent)->previousEventSequence = 0;
        break;

    case 1:
    {
        char *ci;
        int clientNum;

        ((centity_t *)cent)->previousEventSequence = ((centity_t *)cent)->nextState.eventSequence;

        clientNum = ((centity_t *)cent)->nextState.clientNum;
        ci = (char *)&((cg_t *)cg)->bgs.clientinfo[clientNum] - 0x14 ;

        *(int *)(((char *)ci + offsetof(clientInfo_t, name[8])) + 0x3e0)  = *(int *)(cent + 0x15c) ;

        *(int *)(((char *)ci + offsetof(clientInfo_t, name[8])) + 0x3e4)  = ((centity_t *)cent)->nextState.leanf;

        {
            char *to = ((char *)ci + offsetof(clientInfo_t, angles[0][0]));
            VectorCopy3((char *)((centity_t *)cent)->lerpAngles, to);
        }

        *(int *)((char *)((centity_t *)cent)->lerpAngles) = 0;
        *(int *)((char *)((centity_t *)cent)->lerpAngles + 8) = 0;

        CG_ResetPlayerEntity((centity_t *)cent);
        break;
    }

    case 2:
        CG_ResetCorpseEntity((centity_t *)cent);
        break;

    default:

        ((centity_t *)cent)->previousEventSequence = ((centity_t *)cent)->nextState.eventSequence;
        break;
    }
}

static void CG_ClearClientInfos_Inline(char *dest, char *src, char *tagBase, int size)
{
    int attachIndex;
    char *tagSrc = tagBase;
    char *dst = dest;

    memcpy(dest, src, size);

    for (attachIndex = 0; attachIndex < 6; attachIndex++) {
        if (I_stricmp(tagSrc, (const char *)"J_Spine4") == 0) {
            *(char *)(dst + 0x80)  = 0;
            *(char *)(dst + 0x200)  = 0;
        }
        tagSrc += 0x40;
        dst += 0x40;
    }
}

static inline __attribute__((always_inline)) void CG_TransitionSnapshot_Inline(void)
{
    centity_t *cg_ents = cg_entities;
    char *snap;
    int numClients, i;

    snap = (char *)cg->snap;

    numClients = *(int *)((char *)snap + SNAP_NUMCLIENTS);
    for (i = 0; i < numClients; i++) {
        clientState_t *clState = &((snapshot_t *)snap)->clients[i];
        int clientNum = clState->clientIndex ;
        char *ci = (char *)&((cg_t *)cg)->bgs.clientinfo[clientNum];

        if (((clientInfo_t *)ci)->nextValid != 0) {

            ((clientInfo_t *)ci)->nextValid = 0;
            continue;
        } else {

            void *savedTree = ((clientInfo_t *)ci)->pXAnimTree;
            memset(ci, 0, CI_STRIDE);
            ((clientInfo_t *)ci)->pXAnimTree = savedTree;
            CG_SafeDObjFree(clientNum);
            continue;
        }
    }

    cg->snap = cg->nextSnap;

    snap = (char *)cg->nextSnap;
    if (((snapshot_t *)snap)->ps.pm_flags & 0xc00000)
    {

        int playerEntNum = ((snapshot_t *)snap)->ps.clientNum;
        char *playerEnt = (char *)&cg_entities[playerEntNum];
        memcpy(playerEnt, playerEnt + ES_BINSIZE, ES_BINSIZE) ;
    }

    {
        int numEnts = ((snapshot_t *)snap)->numEntities;
        for (i = 0; i < numEnts; i++) {
            char *snapEnt = (char *)&((snapshot_t *)snap)->entities[i];
            int entNum = ((entityState_t *)snapEnt)->number ;
            char *cent = (char *)&cg_entities[entNum];
            memcpy(cent, cent + ES_BINSIZE, ES_BINSIZE) ;
        }
    }
}

void CG_SetNextSnap(snapshot_t *snap_param)
{
    char centInPrevSnapshot[1024];
    char *snap = (char *)snap_param;
    char *prevSnap;
    int i;

    memset(centInPrevSnapshot, 0, 0x400);

    prevSnap = (char *)cg->nextSnap;

    if (prevSnap) {
        int numEnts;

        numEnts = ((snapshot_t *)prevSnap)->numEntities;
        if (numEnts > 0) {
            for (i = 0; i < numEnts; i++) {
                char *snapEnt = (char *)&((snapshot_t *)prevSnap)->entities[i];
                int entNum = ((entityState_t *)snapEnt)->number ;
                char *cent = (char *)&cg_entities[entNum];

                ((centity_t *)cent)->nextValid = 0;

                centInPrevSnapshot[entNum] = 1;
            }
        }

        {
            int playerNum = ((snapshot_t *)prevSnap)->ps.clientNum;
            char *playerEnt = (char *)&cg_entities[playerNum];
            if (((centity_t *)playerEnt)->nextValid) {
                ((centity_t *)playerEnt)->nextValid = 0;
                centInPrevSnapshot[playerNum] = 1;
            }
        }
    }

    cg->nextSnap = (snapshot_t *)snap;

    if (!snap) {

        CG_ClearSolidList();
        return;
    }

#ifdef __EMSCRIPTEN__
    /* Flush reliable obituaries after this snapshot updates player names. */
    extern void CG_FlushObituaries(void);
#endif
    CL_ResetSkeletonCache(0);

    CG_SetFrameInterpolation();

    CG_ExecuteNewServerCommands(((snapshot_t *)snap)->serverCommandSequence);

    CG_CheckOpenWaitingScriptMenu();

    {
        int numClients = ((snapshot_t *)snap)->numClients;
        if (numClients > 0) {
            for (i = 0; i < numClients; i++) {
                clientState_t *clState = &((snapshot_t *)snap)->clients[i];
                int clientNum;
                char *ci;
                int modelIndex;
                const char *configStr;

                clientNum = clState->clientIndex ;
                if ((unsigned)clientNum >= 64) continue;
                ci = (char *)&cg->bgs.clientinfo[clientNum];

                if (((clientInfo_t *)ci)->infoValid == 0)
                    modelIndex = clState->team ;
                else
                    modelIndex = ((clientInfo_t *)ci)->team;

                ((clientInfo_t *)ci)->oldteam = modelIndex;
                ((clientInfo_t *)ci)->infoValid = 1;
                ((clientInfo_t *)ci)->nextValid = 1;
                ((clientInfo_t *)ci)->clientNum = clState->clientIndex ;
                ((clientInfo_t *)ci)->team = clState->team ;

                {
                    char *ciName = ((clientInfo_t *)ci)->name;
                    char *clName = clState->name;

                    if (strcmp(ciName, clName) != 0) {

                        if (((clientInfo_t *)ci)->name[0] != 0) {

                            const char *translated = UI_SafeTranslateString((const char *)"CGAME_PLAYERRENAMES");
                            const char *msg = va((const char *)"%s^7 %s %s", ciName, translated, clName);
                            CG_GameMessage(msg);
                        }

                        I_strncpyz(ciName, clName, 0x20);
                    }
                }

                {
                    int configIndex = clState->modelindex ;
                    configStr = CL_GetConfigString(configIndex + 0x14e);
                }

                {
                    char *ciModel = ((clientInfo_t *)ci)->model;
                    if (strcmp(ciModel, configStr) != 0) {

                        I_strncpyz(ciModel, configStr, 0x40);
                        ((clientInfo_t *)ci)->dobjDirty = 1;
                    }
                }

                {
                    int slot;
                    char *ciAttachModel = ((clientInfo_t *)ci)->attachModelNames[0];
                    char *ciAttachTag = ((clientInfo_t *)ci)->attachTagNames[0];

                    for (slot = 0; slot < 6; slot++) {

                        int attachModelIdx = clState->attachModelIndex[slot] ;
                        configStr = CL_GetConfigString(attachModelIdx + 0x14e);

                        if (strcmp(ciAttachModel, configStr) != 0) {

                            I_strncpyz(ciAttachModel, configStr, 0x40);
                            ((clientInfo_t *)ci)->dobjDirty = 1;
                        }

                        {
                            int attachTagIdx = clState->attachTagIndex[slot] ;
                            const char *tagStr = CL_GetConfigString(attachTagIdx + 0x6e);

                            if (strcmp(ciAttachTag, tagStr) != 0) {

                                I_strncpyz(ciAttachTag, tagStr, 0x40);
                                ((clientInfo_t *)ci)->dobjDirty = 1;
                            }
                        }

                        ciAttachModel += 0x40;
                        ciAttachTag += 0x40;
                    }
                }
            }
        }
    }

#ifdef __EMSCRIPTEN__
    CG_FlushObituaries();
#endif
    cg->identifyClientNum = ((snapshot_t *)snap)->ps.stats[3];
    cg->identifyClientHealth = ((snapshot_t *)snap)->ps.stats[4];

    {
        int entnum = ((snapshot_t *)snap)->ps.clientNum;

        if (((snapshot_t *)snap)->ps.pm_flags & 0xc00000) {

            char *playerEnt = CG_EntityPtr(entnum);

            ((centity_t *)playerEnt)->nextState.number = (unsigned short)entnum;

            BG_PlayerStateToEntityState((char *)&((snapshot_t *)snap)->ps, playerEnt + ES_BINSIZE, 0, 0);

            ((centity_t *)playerEnt)->nextValid = 1;

            {
                char *oldSnap = (char *)cg->snap;

                if (cg->mapRestart ||
                    ((snapshot_t *)snap)->ps.stats[5] != ((snapshot_t *)oldSnap)->ps.stats[5] ||
                    entnum != ((snapshot_t *)oldSnap)->ps.clientNum) {

                    memcpy((char *)&((snapshot_t *)oldSnap)->ps, (char *)&((snapshot_t *)snap)->ps, sizeof(playerState_t));

                    CG_ResetEntity(playerEnt);
                    CG_Respawn();
                } else if (!centInPrevSnapshot[entnum] ||
                           (((centity_t *)playerEnt)->currentState.eFlags ^ ((centity_t *)playerEnt)->nextState.eFlags) & 2) {

                    memcpy((char *)&((snapshot_t *)oldSnap)->ps, (char *)&((snapshot_t *)snap)->ps, sizeof(playerState_t));
                    CG_ResetEntity(playerEnt);

                    VectorClear3(cg->predictedError);
                }

            }
        } else {

            char *oldSnap = (char *)cg->snap;

            if (cg->mapRestart) {

                memcpy(((char *)oldSnap + offsetof(snapshot_t, ps.commandTime)), ((char *)snap + offsetof(snapshot_t, ps.commandTime)), sizeof(playerState_t));
                CG_Respawn();
            } else {

                if (((snapshot_t *)snap)->ps.stats[5] != ((snapshot_t *)oldSnap)->ps.stats[5]) {
                    memcpy(((char *)oldSnap + offsetof(snapshot_t, ps.commandTime)), ((char *)snap + offsetof(snapshot_t, ps.commandTime)), sizeof(playerState_t));
                    CG_Respawn();
                } else if (entnum != ((snapshot_t *)oldSnap)->ps.clientNum) {
                    memcpy(((char *)oldSnap + offsetof(snapshot_t, ps.commandTime)), ((char *)snap + offsetof(snapshot_t, ps.commandTime)), sizeof(playerState_t));
                    CG_Respawn();
                }

            }
        }
    }

    {
        int numEnts = ((snapshot_t *)snap)->numEntities;
        for (i = 0; i < numEnts; i++) {
            char *snapEnt = (char *)&((snapshot_t *)snap)->entities[i];
            int entNum = ((entityState_t *)snapEnt)->number ;
            char *cent = CG_EntityPtr(entNum);

            memcpy(cent + ES_BINSIZE, snapEnt, ES_BINSIZE);

            ((centity_t *)cent)->nextValid = 1;

            if (centInPrevSnapshot[entNum]) {

                if ((((centity_t *)cent)->currentState.eFlags ^ ((entityState_t *)snapEnt)->eFlags ) & 2) {

                    CG_ResetEntity(cent);
                }

            } else {

                CG_ResetEntity(cent);
            }
        }
    }

    {
        int numClients = ((snapshot_t *)snap)->numClients;
        for (i = 0; i < numClients; i++) {
            clientState_t *clState = &((snapshot_t *)snap)->clients[i];
            int clientNum = clState->clientIndex ;
            CG_UpdatePlayerDObj(CG_EntityPtr(clientNum));
        }
    }

    {
        int viewModelIndex = ((snapshot_t *)snap)->ps.viewmodelIndex;
        if (viewModelIndex > 0) {
            const char *cfgStr = CL_GetConfigString(viewModelIndex + 0x14e);
            CG_UpdateHandViewmodels(cfgStr);
        }
    }

    CG_BuildSolidList();

    {
        int gunInit = cg->inKillCam;
        char *nextSnap;

        if (!gunInit) {
            nextSnap = (char *)cg->nextSnap;
            int hasGun = ((snapshot_t *)nextSnap)->ps.deltaTime;

            if (hasGun) {

                cg->inKillCam = 1;

                CG_SetEquippedOffHand(0);

                CG_PlaySmokeGrenadesAtTime(cg->time);
            }
        }

        if (gunInit || (((snapshot_t *)cg->nextSnap)->ps.deltaTime != 0)) {

            nextSnap = (char *)cg->nextSnap;
            if (((snapshot_t *)nextSnap)->ps.deltaTime == 0) {

                cg->inKillCam = 0;

                CG_PlaySmokeGrenadesAtTime(cg->time);

                {
                    int numEnts;
                    nextSnap = (char *)cg->nextSnap;
                    numEnts = ((snapshot_t *)nextSnap)->numEntities;

                    for (i = 0; i < numEnts; i++) {
                        char *snapEnt = (char *)&((snapshot_t *)nextSnap)->entities[i];
                        int entNum = ((entityState_t *)snapEnt)->number ;
                        char *cent = CG_EntityPtr(entNum);

                        if (((centity_t *)cent)->nextState.eType != 2)
                            continue;

                        {
                            int corpseIndex = ((centity_t *)cent)->nextState.number - 64;
                            clientInfo_t *corpseCI;
                            if ((unsigned)corpseIndex >= 8)
                                continue;
                            corpseCI = &((cgs_t *)cgs)->corpseinfo[corpseIndex];
                            void *savedTree = corpseCI->pXAnimTree;
                            int animState = corpseCI->legs.animationNumber & ~0x200;

                            void *anims = XAnimGetAnims(savedTree);

                            if (animState == 0)
                                continue;

                            if (XAnimIsLooped(anims, animState))
                                continue;

                            if (XAnimGetNumChildren(anims, animState) != 0)
                                continue;

                            XAnimSetTime(savedTree, animState, 1.0f);
                        }
                    }
                }
            }
        }
    }

    {
        int numEnts = ((snapshot_t *)snap)->numEntities;
        for (i = 0; i < numEnts; i++) {
            char *snapEnt = (char *)&((snapshot_t *)snap)->entities[i];
            int entNum = ((entityState_t *)snapEnt)->number ;
            CG_CheckEvents(CG_EntityPtr(entNum));
        }
    }

    {
        int isDemo = cg->demoType;

        if (!isDemo) {
            char *nextSnap = (char *)cg->nextSnap;
            /* Live players deliver their effects through prediction. ADS
             * (0x40) must not also replay them from the authoritative snapshot.
             * Only followed/interpolated players use snapshot transitions. */
            if (!(((snapshot_t *)nextSnap)->ps.pm_flags & 0x400000)) {

                const dvar_t *dv1 = cg_dvar1 ? (const dvar_t *)*cg_dvar1 : NULL;
                if (!dv1 || dv1->current.enabled == 0) {
                    const dvar_t *dv2 = cg_dvar2 ? (const dvar_t *)*cg_dvar2 : NULL;
                    if (!dv2 || dv2->current.enabled == 0)
                        return;
                }
            }
        }

        {
            char *oldSnap = (char *)cg->snap;
            char *newSnap = (char *)cg->nextSnap;
            CG_TransitionPlayerState((char *)&((snapshot_t *)newSnap)->ps, (char *)&((snapshot_t *)oldSnap)->ps);
        }
    }
}

void CG_SetInitialSnapshot(snapshot_t *snap_param)
{
    char *snap = (char *)snap_param;
    float clientViewOrigin[3];
    float clientViewAxis[9];

    CG_SetNextSnap(NULL);

    cg->snap = (snapshot_t *)snap;
    cg->nextSnap = (snapshot_t *)snap;

    {
        int serverTime = ((snapshot_t *)snap)->serverTime;
        cg->time = serverTime;
        cg->bgs.time = serverTime;
        cg->oldTime = serverTime;
    }

    {

        clientViewOrigin[0] = ((snapshot_t *)snap)->ps.origin[0];
        clientViewOrigin[1] = ((snapshot_t *)snap)->ps.origin[1];

        float z;

        {
            float origin_z;
            float viewHeight;

            origin_z = ((snapshot_t *)snap)->ps.origin[2];
            viewHeight = ((snapshot_t *)snap)->ps.viewHeightCurrent;
            clientViewOrigin[2] = origin_z + viewHeight;
        }
    }

    AnglesToAxis(((snapshot_t *)snap)->ps.viewangles, clientViewAxis);

    {
        int clientNum = ((snapshot_t *)snap)->ps.clientNum;
        SND_SetListener(clientNum, clientViewOrigin, clientViewAxis);
    }

    SND_FadeAllSounds(1.0f, 0);

    CG_Respawn();

    CG_PlaySmokeGrenadesAtTime(cg->time);

    CG_InitView();

    cg->nextSnap = NULL;
}

static inline __attribute__((always_inline)) char *CG_ReadNextSnapshot(void)
{
    int snapshotNum;
    int latestNum;
    char *dest;

    latestNum = cg->latestSnapshotNum;
    snapshotNum = cgs->processedSnapshotNum;

    if (latestNum > snapshotNum + 0x3e8) {
        Com_Printf("WARNING: CG_ReadNextSnapshot: way out of range, %i > %i\n",
                   latestNum, snapshotNum);
    }

    while (snapshotNum < latestNum) {

        dest = (char *)&cg->activeSnapshots[0];
        if ((char *)cg->snap == dest)
            dest = (char *)&cg->activeSnapshots[1];

        snapshotNum++;
        cgs->processedSnapshotNum = snapshotNum;

        if (CL_GetSnapshot(cgs->processedSnapshotNum, dest)) {

            CG_AddLagometerSnapshotInfo(dest);
            return dest;
        }

        CG_AddLagometerSnapshotInfo(NULL);

        snapshotNum = cgs->processedSnapshotNum;
        latestNum = cg->latestSnapshotNum;
    }

    return NULL;
}

void CG_ProcessSnapshots(void)
{
    int n;
    int cgTime;
    int snapTime;
    char *snap;
    char *curSnap;
    char *nextSnap;

    CL_GetCurrentSnapshotNumber(&n, &cg->latestSnapshotTime);

    if (n != cg->latestSnapshotNum) {
        if (n < cg->latestSnapshotNum) {

            Com_Error(1, (const char *)"\x15"
                                       "CG_ProcessSnapshots: n < cg->latestSnapshotNum");
        }
        cg->latestSnapshotNum = n;
    }

    cg->bgs.latestSnapshotTime = cg->latestSnapshotTime;

    for (;;) {
        curSnap = (char *)cg->snap;

        if (curSnap == NULL) {
            snap = CG_ReadNextSnapshot();
            if (!snap)
                return;

            if (((snapshot_t *)snap)->snapFlags & 2) {
                continue;
            }

            CG_SetInitialSnapshot((snapshot_t *)snap);
            CG_SetNextSnap((snapshot_t *)snap);
            CG_TransitionSnapshot_Inline();
            continue;
        }

        nextSnap = (char *)cg->nextSnap;

        if (nextSnap != NULL) {
            CG_SetFrameInterpolation();
        }

        if (nextSnap != NULL && nextSnap != curSnap) {
            curSnap = (char *)cg->snap;
            nextSnap = (char *)cg->nextSnap;
            cgTime = cg->time;

            if (cgTime < ((snapshot_t *)curSnap)->serverTime ||
                cgTime >= ((snapshot_t *)nextSnap)->serverTime) {
                CG_TransitionSnapshot_Inline();
                continue;
            }

            return;
        }

        snap = CG_ReadNextSnapshot();
        if (!snap) {
            snapTime = ((snapshot_t *)curSnap)->serverTime;
            if (cg->time < snapTime) {
                cg->time = snapTime;
                cg->bgs.time = snapTime;
            }
            return;
        }

        if (((snapshot_t *)snap)->snapFlags & 2) {
            continue;
        }

        if ((((snapshot_t *)snap)->snapFlags ^ ((snapshot_t *)curSnap)->snapFlags) & 4) {
            CG_SetInitialSnapshot((snapshot_t *)snap);
            CG_SetNextSnap((snapshot_t *)snap);
            CG_TransitionSnapshot_Inline();
            continue;
        }

        if (((snapshot_t *)snap)->serverTime < ((snapshot_t *)curSnap)->serverTime) {
            Com_Error(1, (const char *)"\x15"
                                       "CG_ProcessSnapshots: Server time went backwards");
        }

        CG_SetNextSnap((snapshot_t *)snap);
    }
}
