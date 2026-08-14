#include "common_types.h"
#include "imports.h"
#include <stdlib.h>
#include <stdarg.h>
#include <float.h>

extern void BodyEnd();
extern void Die_trigger_damage();
extern void DroppedItemClearOwner();
extern void FinishSpawningItem();
extern void G_ExplodeMissile();
extern void G_PlayerController();
extern void Pain_trigger_damage();
extern void Reached_ScriptMover();
extern void Touch_Item_Auto();
extern void Touch_Multi();
extern void Use_trigger_damage();
extern void hurt_touch();
extern void hurt_use();
extern void player_die();
extern void turret_controller();
extern void turret_think();
extern void turret_think_init();
extern void turret_use();
extern void use_trigger_use();
extern entityHandler_t entityHandlers[20];

#define SCR_CONST() ((const scr_const_t *)imp_scr_const)

COD2_ASSERT_FIELD(struct bgs_t, anim_user,    736232);
COD2_ASSERT_FIELD(struct bgs_t, GetXModel,    736236);
COD2_ASSERT_FIELD(struct bgs_t, CreateDObj,   736240);
COD2_ASSERT_FIELD(struct bgs_t, SafeDObjFree, 736244);
COD2_ASSERT_FIELD(struct bgs_t, AllocXAnim,   736248);

static const char str_dbg_spawn[] = "";
static const char str_dbg_load[] = "";
static const char str_dbg_vmtop_fmt[] = "";
static const char str_dbg_vmtop2_fmt[] = "";
static const char str_dbg_ff_before[] = "[ckpt] after Scr_LoadLevel ff=%p\n";
static const char str_dbg_ff_after_load[] = "[ckpt] after Scr_LoadGameType ff=%p\n";
static const char str_dbg_ff_after_startup[] = "[ckpt] after Scr_StartupGameType ff=%p\n";
static const char str_dbg_ff_trace[] = "";
static const char str_dbg_ff_agv[] = "";
static const char str_dbg_ff_gls[] = "";
static const char str_dbg_endload[] = "";

#if !defined(__EMSCRIPTEN__) && !defined(_MSC_VER)
__asm__(".Lginit_fmt: .asciz \"\"\n");
#endif
void G_InitDbgPrint(const char *fmt, void *ptr)
{
    (void)fmt;
    (void)ptr;
}

extern struct bgs_t level_bgs;
extern struct bgs_t *bgs;
extern struct level_locals_t level;
extern gentity_t g_entities[];

extern const dvar_t *g_gametype;
extern const dvar_t *g_dedicated;
extern const dvar_t *g_cheats;
extern const dvar_t *g_maxclients;
extern const dvar_t *g_password;
extern const dvar_t *g_gravity;
extern const dvar_t *g_speed;
extern const dvar_t *g_knockback;
extern const dvar_t *g_useholdtime;
extern const dvar_t *g_useholdspawndelay;
extern const dvar_t *g_inactivity;
extern const dvar_t *g_debugDamage;
extern const dvar_t *g_debugBullets;
extern const dvar_t *g_weaponAmmoPools;
extern const dvar_t *g_maxDroppedWeapons;
extern const dvar_t *g_synchronousClients;
extern const dvar_t *g_motd;
extern const dvar_t *g_allowVote;
extern const dvar_t *g_dropForwardSpeed;
extern const dvar_t *g_dropUpSpeedBase;
extern const dvar_t *g_dropUpSpeedRand;
extern const dvar_t *g_clonePlayerMaxVelocity;
extern const dvar_t *voice_localEcho;
extern const dvar_t *voice_global;
extern const dvar_t *voice_deadChat;
extern const dvar_t *g_voiceChatTalkingDuration;
extern const dvar_t *g_deadChat;
extern const dvar_t *g_banIPs;
extern const dvar_t *g_smoothClients;
extern const dvar_t *g_NoScriptSpam;
extern const dvar_t *g_debugLocDamage;
extern const dvar_t *g_friendlyfireDist;
extern const dvar_t *g_friendlyNameDist;
extern const dvar_t *player_meleeRange;
extern const dvar_t *player_meleeWidth;
extern const dvar_t *player_meleeHeight;
extern const dvar_t *g_antilag;
extern const dvar_t *g_oldVoting;
extern const dvar_t *g_playerCollisionEjectSpeed;
extern const dvar_t *g_mantleBlockTimeBuffer;
extern const dvar_t *g_log;
extern const dvar_t *g_logSync;
extern const dvar_t *g_listEntity;
extern const dvar_t *g_ScoresBanner_Allies;
extern const dvar_t *g_ScoresBanner_Axis;
extern const dvar_t *g_ScoresBanner_None;
extern const dvar_t *g_ScoresBanner_Spectators;
extern const dvar_t *g_TeamName_Allies;
extern const dvar_t *g_TeamName_Axis;
extern const dvar_t *g_TeamColor_Allies;
extern const dvar_t *g_TeamColor_Axis;
extern const dvar_t *g_voteAbstainWeight;
extern const dvar_t *g_dumpAnims;
extern unsigned char g_clients[];

extern float ceilf(float x);
extern int Com_ServerDObjCreate(DObjModel_s *models, int numModels, struct XAnimTree_s *tree, int handle);
extern int *Hunk_AllocLowInternal(int size);
extern int SV_Trace(trace_t *results, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentmask, qboolean locational, unsigned char *priorityMap, qboolean staticmodels);
extern qboolean SV_TracePassed(const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int passOwnerNum, int contentmask, int locational, int staticmodels);
extern int SV_SightTrace(int *hitNum, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int passOwnerNum, int contentmask);
extern void CL_AddDebugString(const vec_t *xyz, const vec_t *color, float scale, const char *pszText, int fromServer);
extern void Cbuf_ExecuteText(int exec_when, const char *text);
extern void SV_GameSendServerCommand(int clientNum, int svscmd_type, const char *text);
extern void SV_SetConfigstring(int index, const char *val);
extern char *va(const char *format, ...);
extern void Com_Error(int code, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern int FS_Write(const void *buffer, int len, fileHandle_t h);
extern void FS_FCloseFile(fileHandle_t f);
extern unsigned char G_FreeEntity(gentity_t *ent);
extern void SV_UnlinkEntity(gentity_t *ent);
extern void HudElem_DestroyAll(void);
extern qboolean Scr_IsSystemActive(int inst);
extern void Scr_ShutdownSystem(int inst, qboolean freeScripts);
extern void SV_FreeClientScriptPers(void);
extern void Z_FreeInternal(void *ptr);
extern void Mantle_ShutdownAnims(void);
extern unsigned int GScr_FreeScripts(void);
extern void Scr_FreeScripts(int inst);
extern void XAnimFreeTree(struct XAnimTree_s *tree, int inst);
extern void Hunk_ClearToMarkLow(int mark);
extern void G_RunMissile(gentity_t *ent);
extern void G_RunItem(gentity_t *ent);
extern void G_RunCorpse(gentity_t *ent);
extern void G_RunMover(gentity_t *ent);
extern void G_RunClient(gentity_t *ent);
extern unsigned char G_GeneralLink(gentity_t *ent);

static vec3_t vec3_zero = { 0.0f, 0.0f, 0.0f };

enum {
    GMAIN_CS_VOTE_TIME = 15,
    GMAIN_ET_GENERAL = 0,
    GMAIN_ET_PLAYER_CORPSE = 2,
    GMAIN_ET_ITEM = 3,
    GMAIN_ET_MISSILE = 4,
    GMAIN_ET_SCRIPTMOVER = 6,
    GMAIN_FL_NODRAW = 0x800,
    GMAIN_EF_NODRAW = 0x20,
    GMAIN_EFLAGS_UNKNOWN = 0x10000
};

int G_GetSavePersist(void);
void G_SetSavePersist(qboolean savepersist);
float G_GetFogOpaqueDistSqrd(void);
int G_GetClientScore(int clientNum);
int G_GetClientArchiveTime(int clientNum);
void G_SetClientArchiveTime(int clientNum, int time);
clientState_t *G_GetClientState(int clientNum);
static int G_CreateDObj(DObjModel_s *dobjModels, int numModels, struct XAnimTree_s *tree, int handle, clientInfo_t *ci);
int *Hunk_AllocXAnimServer(int size);
static int SortRanks(const int *a, const int *b);
void CalculateRanks(void);
void G_LogPrintf(const char *fmt, ...);
void ExitLevel(void);
void G_InitGame(int levelTime, int randomSeed, qboolean restart, qboolean savepersist);
void CheckVote(void);
int G_RunThink(gentity_t *ent);
void G_TraceCapsule(trace_t *results, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentmask);
qboolean G_TraceCapsuleComplete(const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentmask);
void G_LocationalTrace(trace_t *results, const vec_t *start, const vec_t *end, int passEntityNum, int contentmask, unsigned char *priorityMap);
qboolean G_LocationalTracePassed(const vec_t *start, const vec_t *end, int passEntityNum, int contentmask);
int G_SightTrace(int *hitNum, const vec_t *start, const vec_t *end, int passEntityNum, int contentmask);
void G_AddDebugString(const vec_t *xyz, const vec_t *color, float scale, const char *pszText);
void G_ShutdownGame(qboolean freeScripts);
static void __attribute_regparm__(1) G_RunFrameForEntity(gentity_t *ent);
int G_RunFrame(int levelTime);

extern const char *SL_ConvertToString(unsigned int index);
void dbg_scan_ents(const char *where)
{
    int i;
    for (i = 0; i < level.num_entities; i++) {
        gentity_t *e = &g_entities[i];
        if (!e->r.inuse)
            continue;
        if ((unsigned int)e->s.pos.trType > 20u) {
            Com_Printf("[scan@%s] CORRUPT ent#%d eType=%d trType=0x%x class=%s\n", where, i,
                       e->s.eType, (unsigned int)e->s.pos.trType,
                       SL_ConvertToString((unsigned short)e->classname));
            return;
        }
    }
    Com_Printf("[scan@%s] clean (num=%d)\n", where, level.num_entities);
}

unsigned int g_dbg_lastBuiltin;
void *g_dbg_lastBuiltinFn;
unsigned int g_lastop_dbg;
/* write-watchpoint: make ent#439's page read-only so the corrupting write faults with an exact rip */
void dbg_protect_439(void)
{
    static int done;
    void *__stdcall GetModuleHandleA(const char *);
    void *__stdcall GetProcAddress(void *, const char *);
    typedef int(__stdcall * VPt)(void *, unsigned long long, unsigned long, unsigned long *);
    static VPt vp;
    char *base;
    unsigned long long page;
    unsigned long oldp;
    if (done)
        return;
    done = 1;
    if (!vp) {
        void *k = GetModuleHandleA("kernel32.dll");
        vp = (VPt)GetProcAddress(k, "VirtualProtect");
    }
    base = (char *)&g_entities[439];
    page = ((unsigned long long)(uintptr_t)base) & ~0xFFFull;
    if (vp) {
        vp((void *)(uintptr_t)page, 0x1000, 2 /*PAGE_READONLY*/, &oldp);
        Com_Printf("[wp] protected ent#439 page %p (ent at %p)\n", (void *)(uintptr_t)page, (void *)base);
    }
}
void dbg_end_probe(int step)
{
    static int rep;
    if (!rep && (unsigned int)g_entities[439].s.pos.trType > 20u) {
        rep = 1;
        Com_Printf("[end-probe] ent#439 corrupt at End step=%d trType=0x%x\n",
                   step, (unsigned int)g_entities[439].s.pos.trType);
    }
}
void dbg_check439_op(unsigned int lastop)
{
    static int rep;
    if (!rep && (unsigned int)g_entities[439].s.pos.trType > 20u) {
        rep = 1;
        Com_Printf("[op-corruptor] ent#439 corrupt AFTER opcode=0x%x trType=0x%x\n",
                   lastop, (unsigned int)g_entities[439].s.pos.trType);
    }
}
void dbg_check439_pre(unsigned int idx)
{
    static int rep;
    if (!rep && (unsigned int)g_entities[439].s.pos.trType > 20u) {
        rep = 1;
        Com_Printf("[pre] ent#439 ALREADY corrupt BEFORE builtinIndex=%u (so a prior opcode wrote it)\n", idx);
    }
}
void dbg_check439(void)
{
    static int rep;
    if (!rep && (unsigned int)g_entities[439].s.pos.trType > 20u) {
        void *__stdcall GetModuleHandleA(const char *);
        rep = 1;
        Com_Printf("[corruptor] ent#439 trType=0x%x after builtinIndex=%u fn_rva=0x%llx\n",
                   (unsigned int)g_entities[439].s.pos.trType, g_dbg_lastBuiltin,
                   (unsigned long long)((char *)g_dbg_lastBuiltinFn - (char *)GetModuleHandleA(0)));
    }
}

int G_GetSavePersist(void)
{
    return level.savepersist;
}

void G_SetSavePersist(qboolean savepersist)
{
    level.savepersist = savepersist;
}

float G_GetFogOpaqueDistSqrd(void)
{
    return level.fFogOpaqueDistSqrd;
}

int G_GetClientScore(int clientNum)
{
    return level.clients[clientNum].sess.score;
}

int G_GetClientArchiveTime(int clientNum)
{
    return level.clients[clientNum].sess.archiveTime;
}

void G_SetClientArchiveTime(int clientNum, int time)
{
    level.clients[clientNum].sess.archiveTime = time;
}

clientState_t *G_GetClientState(int clientNum)
{
    return &level.clients[clientNum].sess.cs;
}

static int G_CreateDObj(DObjModel_s *dobjModels, int numModels, struct XAnimTree_s *tree, int handle, clientInfo_t *ci)
{
    numModels = (unsigned short)numModels;
    (void)ci;
    return Com_ServerDObjCreate(dobjModels, numModels, tree, handle);
}

int *Hunk_AllocXAnimServer(int size)
{
    return Hunk_AllocLowInternal(size);
}

static int SortRanks(const int *a, const int *b)
{
    gclient_t *ca = &level.clients[*a];
    gclient_t *cb = &level.clients[*b];

    if (ca->sess.connected == CON_CONNECTING) {
        return 1;
    }
    if (cb->sess.connected == CON_CONNECTING) {
        return -1;
    }

    if (ca->sess.cs.team == TEAM_SPECTATOR) {
        if (cb->sess.cs.team != TEAM_SPECTATOR) {
            return 1;
        }
        if ((byte *)ca < (byte *)cb) {
            return -1;
        }
        if ((byte *)ca == (byte *)cb) {
            return 0;
        }
        return 1;
    }

    if (cb->sess.cs.team == TEAM_SPECTATOR) {
        return -1;
    }

    if (ca->sess.score > cb->sess.score) {
        return -1;
    }
    if (ca->sess.score < cb->sess.score) {
        return 1;
    }

    if (ca->sess.deaths < cb->sess.deaths) {
        return -1;
    }
    if (ca->sess.deaths > cb->sess.deaths) {
        return 1;
    }

    return 0;
}

void CalculateRanks(void)
{
    int i;

    level.numConnectedClients = 0;
    level.numVotingClients = 0;

    for (i = 0; i < level.maxclients; i++) {
        if (level.clients[i].sess.connected != CON_DISCONNECTED) {
            level.sortedClients[level.numConnectedClients] = i;
            level.numConnectedClients++;

            if (level.clients[i].sess.cs.team != TEAM_SPECTATOR) {
                if (level.clients[i].sess.connected == CON_CONNECTED) {
                    level.numVotingClients++;
                }
            }
        }
    }

    qsort(level.sortedClients, level.numConnectedClients, sizeof(int), (int (*)(const void *, const void *))SortRanks);
    level.bUpdateScoresForIntermission = 1;
}

void G_LogPrintf(const char *fmt, ...)
{
    char string[1024];
    char string2[1024];
    va_list argptr;
    int min, sec, tens, ones;

    if (level.logFile == 0)
        return;

    va_start(argptr, fmt);
    vsnprintf(string2, 1024, fmt, argptr);
    va_end(argptr);

    sec = level.time / 1000;
    min = sec / 60;
    sec %= 60;
    tens = sec / 10;
    ones = sec % 10;

    Com_sprintf(string, 1024, "%3i:%i%i %s", min, tens, ones, string2);
    FS_Write(string, strlen(string), level.logFile);
}

void ExitLevel(void)
{
    int i;

    Cbuf_ExecuteText(2, "map_rotate\n");

    level.teamScores[TEAM_AXIS] = 0;
    level.teamScores[TEAM_ALLIES] = 0;

    for (i = 0; i < g_maxclients->current.integer; i++) {
        if (level.clients[i].sess.connected == CON_CONNECTED) {
            level.clients[i].sess.score = 0;
        }
    }

    for (i = 0; i < g_maxclients->current.integer; i++) {
        if (level.clients[i].sess.connected == CON_CONNECTED) {
            level.clients[i].sess.connected = CON_CONNECTING;
        }
    }

    G_LogPrintf("ExitLevel: executed\n");
    return;
}

void DBG_PrintFreeVars(const char *label)
{
    (void)label;
}

static void G_RegisterDvars_impl(void);
extern const dvar_t *Dvar_RegisterString_mac(const char *name, const char *def, int flags);
extern const dvar_t *Dvar_RegisterInt(const char *name, int def, int min, int max, int flags);
extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int def, int flags);
extern const dvar_t *Dvar_RegisterFloat(const char *name, float def, float min, float max, int flags);
extern void Swap_Init(void);
extern void G_ProcessIPBans(void);
extern void G_SetupWeaponDef(void);
extern void BG_RegisterDvars(void);
extern void Rand_Init(int seed);
extern int FS_FOpenFileByMode(const char *filename, int *handle, int mode);
extern void SV_GetServerinfo(char *value, int size);
extern void SV_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity, playerState_t *clients, int sizeofGameClient);
extern void G_SpawnEntitiesFromString(void);
extern void Scr_BeginLoadScripts(void);
extern void Scr_PostCompileScripts(void);
extern void Scr_EndLoadScripts(void);
extern void GScr_LoadScripts(int inst);
extern void GScr_LoadConsts(void);
extern void Scr_BeginLoadAnimScripts(void);
extern void GScr_LoadAnimScripts(void);
extern void Scr_EndLoadAnimScripts(void);
extern void Scr_FreeEntityList(void);
extern void Scr_InitSystem(int sys);
extern void Scr_SetLoading(int loading);
extern void Scr_AllocGameVariable(void);
extern void G_LoadStructs(void);
extern unsigned int Scr_LoadLevel(void);
extern unsigned int Scr_LoadGameType(void);
extern unsigned int Scr_StartupGameType(void);
extern void RestoreBody(void);
extern void ClientUserinfoChanged(int clientNum);
extern void G_InitTurrets(void);
extern int SV_GetBrushModelCount(void);
extern void G_SpawnTriggerHurt(int numBrushModels);
extern void GScr_PostResetTimeout(void);
extern short int CheckTeamStatus(void);
extern void DeathmatchScoreboardMessage(gentity_t *ent);
extern const char *SL_ConvertToString(unsigned int index);
extern void Dvar_SetBool(const dvar_t *dvar, int value);
extern void SaveRegisteredWeapons(void);
extern void SaveRegisteredItems(void);
extern void SV_DObjDisplayAnim(gentity_t *ent);
extern void SV_DObjInitServerTime(gentity_t *ent, float dt);
extern int G_DObjUpdateServerTime(gentity_t *ent, int bNotify);
extern void Scr_RunCurrentThreads(void);
extern void Scr_IncTime(void);
extern void Com_Memcpy(void *dest, const void *src, int count);
extern int stricmp(const char *s1, const char *s2);
extern void ClientEndFrame(gentity_t *ent);
extern void HudElem_UpdateClient(gclient_t *client, int clientNum, int which);
extern unsigned char scrVarPub[];
extern unsigned char scrVmPub[];
extern unsigned int Scr_AddEntity(void *ent);
extern void Scr_Notify(void *ent, int stringValue, unsigned int paramcount);
extern void BG_LoadAnim(void);
extern int Com_FindSoundAlias(const char *name);
extern int G_AnimScriptSound(int client, snd_alias_list_t *aliasList);
extern void *XAnimCreateTree(void *anims, void *Alloc);

#ifndef __EMSCRIPTEN__
void G_InitGame(int levelTime, int randomSeed, qboolean restart, qboolean savepersist)
{
    char info[0x800];
    int i;
    const char *logFile;
    gclient_t *cl;

    Com_Printf("------- Game Initialization -------\n");
    Com_Printf("gamename: %s\n", "Call of Duty 2");
    Com_Printf("gamedate: %s\n", __DATE__);

    Swap_Init();

    memset(&level, 0, sizeof(level));
    level.initializing = 1;
    level.time = levelTime;
    level.startTime = levelTime;
    level.currentEntityThink = -1;
    level.gentities = (gentity_s *)g_entities;
    level.clients = (gclient_s *)g_clients;

    G_InitDbgPrint("[G_InitGame] level.clients=%p\n", (void *)level.clients);

    srand(randomSeed);
    Rand_Init(randomSeed);
    G_SetupWeaponDef();
    G_RegisterDvars_impl();
    BG_RegisterDvars();

    level.maxclients = g_maxclients->current.integer;

    G_ProcessIPBans();

    level_bgs.GetXModel    = (struct XModel *(*)())imp_SV_XModelGet;
    level_bgs.CreateDObj   = (void (*)())G_CreateDObj;
    level_bgs.SafeDObjFree = (void (*)())imp_Com_SafeServerDObjFree;
    level_bgs.AllocXAnim   = (void *(*)())Hunk_AllocXAnimServer;
    level_bgs.anim_user    = 1;

    logFile = g_log->current.string;
    if (*logFile != '\0') {

        FS_FOpenFileByMode(logFile, &level.logFile, g_logSync->current.integer ? 3 : 2);
        if (level.logFile != 0) {
            SV_GetServerinfo(info, sizeof(info));
            G_LogPrintf("------------------------------------------------------------\n");
            G_LogPrintf("InitGame: %s\n", info);
        } else {
            Com_Printf("WARNING: Couldn't open logfile: %s\n", g_log->current.string);
        }
    } else {
        Com_Printf("Not logging to disk.\n");
    }

    {
        int *p = Hunk_AllocLowInternal(0);
        *(void **)&scrVarPub = (void *)p;
        *(byte *)p = 0;
    }

    for (i = 0; i < level.maxclients; i++) {
        g_entities[i].client =
            (gclient_s *)((byte *)level.clients + (unsigned int)i * 0x28a4);
    }
    level.num_entities = 0x48;
    *(int *)((char *)&level + 16) = 0;
    *(int *)((char *)&level + 20) = 0;

    SV_LocateGameData((gentity_t *)g_entities, 0x48, sizeof(gentity_t), (playerState_t *)g_clients, sizeof(gclient_t));
    G_SpawnEntitiesFromString();
    DBG_PrintFreeVars(str_dbg_spawn);
    level.initializing = 0;

    G_LogPrintf("gametype: %s\n", g_gametype->current.string);

    G_InitTurrets();
    G_SpawnTriggerHurt(SV_GetBrushModelCount() + 1);
    GScr_PostResetTimeout();
    G_SetupWeaponDef();
    Scr_BeginLoadScripts();
    GScr_LoadScripts(1);
    DBG_PrintFreeVars(str_dbg_load);
    Scr_PostCompileScripts();
    Scr_EndLoadScripts();
    DBG_PrintFreeVars(str_dbg_endload);

    *(void **)imp_bgs = (void *)&level_bgs;
    level_bgs.animScriptData.soundAlias = (snd_alias_list_t * (*)()) Com_FindSoundAlias;
    level_bgs.animScriptData.playSoundAlias = (int (*)())G_AnimScriptSound;

    if (!restart) {
        BG_LoadAnim();

        void *animTree = (void *)level_bgs.animScriptData.animTree.anims;
        byte *clientTree = (byte *)&level_bgs + 0xb40a0;
        int t;
        for (t = 0; t < 64; t++, clientTree += 0x4b8)
            *(void **)clientTree = XAnimCreateTree(animTree, (void *)Hunk_AllocXAnimServer);
    }

    GScr_LoadConsts();
    Scr_FreeScripts(1);
    Scr_BeginLoadAnimScripts();
    GScr_LoadAnimScripts();
    Scr_EndLoadAnimScripts();
    G_RegisterDvars_impl();

    Com_Printf(str_dbg_vmtop_fmt, *(void **)((byte *)&scrVmPub + 16));
    Scr_FreeEntityList();
    Scr_InitSystem(1);
    Com_Printf(str_dbg_ff_trace, *(void **)((byte *)&scrVmPub + 12));
    Scr_SetLoading(1);
    Scr_AllocGameVariable();
    Com_Printf(str_dbg_ff_agv, *(void **)((byte *)&scrVmPub + 12));
    G_LoadStructs();
    Com_Printf(str_dbg_ff_gls, *(void **)((byte *)&scrVmPub + 12));

    level.initializing = 1;
    Scr_LoadLevel();
    Com_Printf(str_dbg_ff_before, *(void **)((byte *)&scrVmPub + 12));
    Scr_LoadGameType();
    Com_Printf(str_dbg_ff_after_load, *(void **)((byte *)&scrVmPub + 12));
    Scr_StartupGameType();
    Com_Printf(str_dbg_ff_after_startup, *(void **)((byte *)&scrVmPub + 12));

    if (restart && !savepersist) {
        RestoreBody();
    }

    for (i = 0; i < g_maxclients->current.integer; i++) {
        cl = (gclient_t *)((byte *)level.clients + (unsigned int)i * 0x28a4);
        if (cl->sess.connected == CON_CONNECTED) {
            ClientUserinfoChanged(i);
        }
    }

    if (g_dedicated->current.integer > 0) {
        const char *pw = g_password->current.string;
        if (*pw != '\0') {
            Com_sprintf(info, sizeof(info), "password: %s\n", pw);
        }
    }

    Com_Printf("[ckpt] before CalculateRanks\n");
    CalculateRanks();
    level.initializing = 0;
    Com_Printf("[ckpt] G_InitGame END\n");
    dbg_scan_ents("G_InitGame_end");

    return;
}
#endif
extern const dvar_t *Dvar_RegisterString_mac(const char *name, const char *def, int flags);
extern const dvar_t *Dvar_RegisterInt(const char *name, int def, int min, int max, int flags);
extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int def, int flags);
extern const dvar_t *Dvar_RegisterFloat(const char *name, float def, float min, float max, int flags);
extern void Swap_Init(void);
extern void G_ProcessIPBans(void);
extern void G_SetupWeaponDef(void);
extern void BG_RegisterDvars(void);
extern void Rand_Init(int seed);
extern int FS_FOpenFileByMode(const char *filename, int *handle, int mode);
extern void SV_GetServerinfo(char *value, int size);
extern void SV_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity, playerState_t *clients, int sizeofGameClient);
extern void G_SpawnEntitiesFromString(void);
extern void Scr_BeginLoadScripts(void);
extern void Scr_PostCompileScripts(void);
extern void Scr_EndLoadScripts(void);
extern void GScr_LoadScripts(int inst);
extern void GScr_LoadConsts(void);
extern void Scr_BeginLoadAnimScripts(void);
extern void GScr_LoadAnimScripts(void);
extern void Scr_EndLoadAnimScripts(void);
extern void Scr_FreeEntityList(void);
extern void Scr_InitSystem(int sys);
extern void Scr_SetLoading(int loading);
extern void Scr_AllocGameVariable(void);
extern void G_LoadStructs(void);
extern unsigned int Scr_LoadLevel(void);
extern unsigned int Scr_LoadGameType(void);
extern unsigned int Scr_StartupGameType(void);
extern void RestoreBody(void);
extern void ClientUserinfoChanged(int clientNum);
extern void G_InitTurrets(void);
extern int SV_GetBrushModelCount(void);
extern void G_SpawnTriggerHurt(int numBrushModels);
extern void GScr_PostResetTimeout(void);
extern short int CheckTeamStatus(void);
extern void DeathmatchScoreboardMessage(gentity_t *ent);
extern const char *SL_ConvertToString(unsigned int index);
extern void Dvar_SetBool(const dvar_t *dvar, int value);
extern void SaveRegisteredWeapons(void);
extern void SaveRegisteredItems(void);
extern void SV_DObjDisplayAnim(gentity_t *ent);
extern void SV_DObjInitServerTime(gentity_t *ent, float dt);
extern int G_DObjUpdateServerTime(gentity_t *ent, int bNotify);
extern void Scr_RunCurrentThreads(void);
extern void Scr_IncTime(void);
extern void Com_Memcpy(void *dest, const void *src, int count);
extern int stricmp(const char *s1, const char *s2);
extern void ClientEndFrame(gentity_t *ent);
extern void HudElem_UpdateClient(gclient_t *client, int clientNum, int which);
extern unsigned char scrVarPub[];
extern unsigned char scrVmPub[];
extern unsigned int Scr_AddEntity(void *ent);
extern void Scr_Notify(void *ent, int stringValue, unsigned int paramcount);

static void G_RegisterDvars_impl(void)
{

    extern const dvar_t *Dvar_RegisterColor(const char *name, float r, float g, float b, float a, int flags);

    g_cheats = Dvar_RegisterBool_mac("sv_cheats", 0, 0x1000);
    Dvar_RegisterString_mac("gamename", "Call of Duty 2", 0x1044);
    Dvar_RegisterString_mac("gamedate", "Apr 13 2006", 0x1040);
    Dvar_RegisterString_mac("sv_mapname", "", 0x1044);
    g_gametype = Dvar_RegisterString_mac("g_gametype", "dm", 0x1024);
    g_maxclients = Dvar_RegisterInt("sv_maxclients", 20, 1, 0x40, 0x1025);
    g_synchronousClients = Dvar_RegisterBool_mac("g_synchronousClients", 0, 0x1008);
    g_log = Dvar_RegisterString_mac("g_log", "games_mp.log", 0x1001);
    g_logSync = Dvar_RegisterBool_mac("g_logSync", 0, 0x1001);
    g_password = Dvar_RegisterString_mac("g_password", "", 0x1000);
    g_banIPs = Dvar_RegisterString_mac("g_banIPs", "", 0x1001);
    g_dedicated = Dvar_RegisterInt("dedicated", 0, 0, 2, 0x1020);
    if (g_dedicated->current.integer != 0) {
        Dvar_RegisterInt("dedicated", 0, 0, 2, 0x1040);
    }
    g_speed = Dvar_RegisterInt("g_speed", 190, (int)0x80000000, 0x7fffffff, 0x1000);
    g_gravity = Dvar_RegisterFloat("g_gravity", 800.0f, 1.0f, FLT_MAX, 0x1000);
    g_knockback = Dvar_RegisterFloat("g_knockback", 1000.0f, -FLT_MAX, FLT_MAX, 0x1000);
    g_weaponAmmoPools = Dvar_RegisterBool_mac("g_weaponAmmoPools", 0, 0x1000);
    g_maxDroppedWeapons = Dvar_RegisterInt("g_maxDroppedWeapons", 0x10, 1, 0x20, 0x1000);
    g_inactivity = Dvar_RegisterInt("g_inactivity", 0, 0, 0x7fffffff, 0x1000);
    g_debugDamage = Dvar_RegisterBool_mac("g_debugDamage", 0, 0x1080);
    g_debugBullets = Dvar_RegisterInt("g_debugBullets", 0, -3, 6, 0x1080);
    g_motd = Dvar_RegisterString_mac("g_motd", "", 0x1000);
    g_playerCollisionEjectSpeed = Dvar_RegisterInt("g_playerCollisionEjectSpeed", 25, 0, 0x7d00, 0x1001);
    g_dropForwardSpeed = Dvar_RegisterFloat("g_dropForwardSpeed", 10.0f, 0.0f, 1000.0f, 0x1001);
    g_dropUpSpeedBase = Dvar_RegisterFloat("g_dropUpSpeedBase", 10.0f, 0.0f, 1000.0f, 0x1001);
    g_dropUpSpeedRand = Dvar_RegisterFloat("g_dropUpSpeedRand", 5.0f, 0.0f, 1000.0f, 0x1001);
    g_clonePlayerMaxVelocity = Dvar_RegisterFloat("g_clonePlayerMaxVelocity", 80.0f, 0.0f, FLT_MAX, 0x1001);
    voice_global = Dvar_RegisterBool_mac("voice_global", 0, 0x1001);
    voice_localEcho = Dvar_RegisterBool_mac("voice_localEcho", 0, 0x1001);
    voice_deadChat = Dvar_RegisterBool_mac("voice_deadChat", 0, 0x1001);
    g_allowVote = Dvar_RegisterBool_mac("g_allowVote", 1, 0x1000);
    g_listEntity = Dvar_RegisterBool_mac("g_listEntity", 0, 0x1000);
    g_deadChat = Dvar_RegisterBool_mac("g_deadChat", 0, 0x1001);
    g_voiceChatTalkingDuration = Dvar_RegisterInt("g_voiceChatTalkingDuration", 0x1f4, 0, 0x2710, 0x1001);
    g_ScoresBanner_Allies = Dvar_RegisterString_mac("g_ScoresBanner_Allies", "mpflag_american", 0x1100);
    g_ScoresBanner_Axis = Dvar_RegisterString_mac("g_ScoresBanner_Axis", "mpflag_german", 0x1100);
    g_ScoresBanner_None = Dvar_RegisterString_mac("g_ScoresBanner_None", "mpflag_none", 0x1100);
    g_ScoresBanner_Spectators = Dvar_RegisterString_mac("g_ScoresBanner_Spectators", "mpflag_spectator", 0x1100);
    g_TeamName_Allies = Dvar_RegisterString_mac("g_TeamName_Allies", "GAME_ALLIES", 0x1100);
    g_TeamName_Axis = Dvar_RegisterString_mac("g_TeamName_Axis", "GAME_AXIS", 0x1100);
    g_TeamColor_Allies = Dvar_RegisterColor("g_TeamColor_Allies", 0.5f, 0.5f, 1.0f, 1.0f, 0x1100);
    g_TeamColor_Axis = Dvar_RegisterColor("g_TeamColor_Axis", 1.0f, 0.5f, 0.5f, 1.0f, 0x1100);
    g_smoothClients = Dvar_RegisterBool_mac("g_smoothClients", 1, 0x1000);
    g_antilag = Dvar_RegisterBool_mac("g_antilag", 1, 0x1005);
    g_oldVoting = Dvar_RegisterBool_mac("g_oldVoting", 1, 0x1001);
    g_voteAbstainWeight = Dvar_RegisterFloat("g_voteAbstainWeight", 0.5f, 0.0f, 1.0f, 0x1001);
    g_NoScriptSpam = Dvar_RegisterBool_mac("g_no_script_spam", 0, 0x1000);
    g_debugLocDamage = Dvar_RegisterBool_mac("g_debugLocDamage", 0, 0x1080);
    g_friendlyfireDist = Dvar_RegisterFloat("g_friendlyfireDist", 256.0f, 0.0f, 15000.0f, 0x1080);
    g_friendlyNameDist = Dvar_RegisterFloat("g_friendlyNameDist", 15000.0f, 0.0f, 15000.0f, 0x1080);
    player_meleeRange = Dvar_RegisterFloat("player_meleeRange", 64.0f, 0.0f, 1000.0f, 0x1080);
    player_meleeWidth = Dvar_RegisterFloat("player_meleeWidth", 10.0f, 0.0f, 1000.0f, 0x1080);
    player_meleeHeight = Dvar_RegisterFloat("player_meleeHeight", 10.0f, 0.0f, 1000.0f, 0x1080);
    g_dumpAnims = Dvar_RegisterInt("g_dumpAnims", -1, -1, 0x3ff, 0x1080);
    g_useholdtime = Dvar_RegisterInt("g_useholdtime", 0, 0, 0x7fffffff, 0x1000);
    g_useholdspawndelay = Dvar_RegisterInt("g_useholdspawndelay", 1, 0, 10, 0x1081);
    g_mantleBlockTimeBuffer = Dvar_RegisterInt("g_mantleBlockTimeBuffer", 0x1f4, 0, 0xea60, 0x1080);
}

void G_RegisterDvars(void)
{
    G_RegisterDvars_impl();
}

#ifdef __EMSCRIPTEN__
void G_InitGame(int levelTime, int randomSeed, qboolean restart, qboolean savepersist)
{
    char info[0x800];
    int i;
    const char *logFile;
    gclient_t *cl;

    Com_Printf("------- Game Initialization -------\n");
    Com_Printf("gamename: %s\n", "Call of Duty 2");
    Com_Printf("gamedate: %s\n", __DATE__);

    Swap_Init();

    memset(&level, 0, sizeof(level));
    level.initializing = 1;
    level.time = levelTime;
    level.startTime = levelTime;
    level.currentEntityThink = -1;
    level.gentities = (gentity_s *)g_entities;
    level.clients = (gclient_s *)g_clients;

    G_InitDbgPrint("[G_InitGame] level.clients=%p\n", (void *)level.clients);

    srand(randomSeed);
    Rand_Init(randomSeed);
    G_SetupWeaponDef();
    G_RegisterDvars_impl();
    BG_RegisterDvars();

    level.maxclients = g_maxclients->current.integer;

    G_ProcessIPBans();

    level_bgs.GetXModel    = (struct XModel *(*)())imp_SV_XModelGet;
    level_bgs.CreateDObj   = (void (*)())G_CreateDObj;
    level_bgs.SafeDObjFree = (void (*)())imp_Com_SafeServerDObjFree;
    level_bgs.AllocXAnim   = (void *(*)())Hunk_AllocXAnimServer;
    level_bgs.anim_user    = 1;

    logFile = g_log->current.string;
    if (*logFile != '\0') {

        FS_FOpenFileByMode(logFile, &level.logFile, g_logSync->current.integer ? 3 : 2);
        if (level.logFile != 0) {
            SV_GetServerinfo(info, sizeof(info));
            G_LogPrintf("------------------------------------------------------------\n");
            G_LogPrintf("InitGame: %s\n", info);
        } else {
            Com_Printf("WARNING: Couldn't open logfile: %s\n", g_log->current.string);
        }
    } else {
        Com_Printf("Not logging to disk.\n");
    }

    {
        int *p = Hunk_AllocLowInternal(0);
        *(void **)&scrVarPub = (void *)p;
        *(byte *)p = 0;
    }

    for (i = 0; i < level.maxclients; i++) {
        g_entities[i].client =
            (gclient_s *)((byte *)level.clients + (unsigned int)i * 0x28a4);
    }
    level.num_entities = 0x48;
    *(int *)((char *)&level + 16) = 0;
    *(int *)((char *)&level + 20) = 0;

    SV_LocateGameData((gentity_t *)g_entities, 0x48, sizeof(gentity_t), (playerState_t *)g_clients, sizeof(gclient_t));
    G_SpawnEntitiesFromString();
    DBG_PrintFreeVars(str_dbg_spawn);
    level.initializing = 0;

    G_LogPrintf("gametype: %s\n", g_gametype->current.string);

    G_InitTurrets();
    G_SpawnTriggerHurt(SV_GetBrushModelCount() + 1);
    GScr_PostResetTimeout();
    G_SetupWeaponDef();
    Scr_BeginLoadScripts();
    GScr_LoadScripts(1);
    DBG_PrintFreeVars(str_dbg_load);
    Scr_PostCompileScripts();
    Scr_EndLoadScripts();
    DBG_PrintFreeVars(str_dbg_endload);

    *(void **)imp_bgs = (void *)&level_bgs;
    level_bgs.animScriptData.soundAlias = (snd_alias_list_t * (*)()) Com_FindSoundAlias;
    level_bgs.animScriptData.playSoundAlias = (int (*)())G_AnimScriptSound;

    if (!restart) {
        BG_LoadAnim();

        void *animTree = (void *)level_bgs.animScriptData.animTree.anims;
        byte *clientTree = (byte *)&level_bgs + 0xb40a0;
        int t;
        for (t = 0; t < 64; t++, clientTree += 0x4b8)
            *(void **)clientTree = XAnimCreateTree(animTree, (void *)Hunk_AllocXAnimServer);
    }

    GScr_LoadConsts();
    Scr_FreeScripts(1);
    Scr_BeginLoadAnimScripts();
    GScr_LoadAnimScripts();
    Scr_EndLoadAnimScripts();
    G_RegisterDvars_impl();

    Com_Printf(str_dbg_vmtop_fmt, *(void **)((byte *)&scrVmPub + 16));
    Scr_FreeEntityList();
    Scr_InitSystem(1);
    Com_Printf(str_dbg_ff_trace, *(void **)((byte *)&scrVmPub + 12));
    Scr_SetLoading(1);
    Scr_AllocGameVariable();
    Com_Printf(str_dbg_ff_agv, *(void **)((byte *)&scrVmPub + 12));
    G_LoadStructs();
    Com_Printf(str_dbg_ff_gls, *(void **)((byte *)&scrVmPub + 12));

    level.initializing = 1;
    Scr_LoadLevel();
    Com_Printf(str_dbg_ff_before, *(void **)((byte *)&scrVmPub + 12));
    Scr_LoadGameType();
    Com_Printf(str_dbg_ff_after_load, *(void **)((byte *)&scrVmPub + 12));
    Scr_StartupGameType();
    Com_Printf(str_dbg_ff_after_startup, *(void **)((byte *)&scrVmPub + 12));

    if (restart && !savepersist) {
        RestoreBody();
    }

    for (i = 0; i < g_maxclients->current.integer; i++) {
        cl = (gclient_t *)((byte *)level.clients + (unsigned int)i * 0x28a4);
        if (cl->sess.connected == CON_CONNECTED) {
            ClientUserinfoChanged(i);
        }
    }

    if (g_dedicated->current.integer > 0) {
        const char *pw = g_password->current.string;
        if (*pw != '\0') {
            Com_sprintf(info, sizeof(info), "password: %s\n", pw);
        }
    }

    CalculateRanks();
    level.initializing = 0;

    return 0;
}
#endif

void CheckVote(void)
{
    if (level.voteExecuteTime && level.voteExecuteTime < level.time) {
        level.voteExecuteTime = 0;
        Cbuf_ExecuteText(2, va("%s\n", level.voteString));
    }

    if (!level.voteTime) {
        return;
    }

    if (level.time < level.voteTime) {
        int passCount;

        passCount = level.numVotingClients / 2 + 1;
        if (level.voteYes >= passCount) {
            SV_GameSendServerCommand(-1, SV_CMD_CAN_IGNORE, va("%c \"GAME_VOTEPASSED\"", 101));
            level.voteExecuteTime = level.time + 3000;
        } else if (level.voteNo > level.numVotingClients - passCount) {
            SV_GameSendServerCommand(-1, SV_CMD_CAN_IGNORE, va("%c \"GAME_VOTEFAILED\"", 101));
        } else {
            return;
        }
    } else if (level.voteYes > (int)((double)level.voteNo + ceilf((float)(level.numVotingClients - (level.voteYes + level.voteNo)) * g_voteAbstainWeight->current.value))) {
        SV_GameSendServerCommand(-1, SV_CMD_CAN_IGNORE, va("%c \"GAME_VOTEPASSED\"", 101));
        level.voteExecuteTime = level.time + 3000;
    } else {
        SV_GameSendServerCommand(-1, SV_CMD_CAN_IGNORE, va("%c \"GAME_VOTEFAILED\"", 101));
    }

    level.voteTime = 0;
    SV_SetConfigstring(GMAIN_CS_VOTE_TIME, "");
}

static inline __attribute__((always_inline)) int G_RunThink_core(gentity_t *ent)
{
    int thinktime;
    fn_think think;

    thinktime = ent->nextthink;
    if (thinktime <= 0) {
        return thinktime;
    }
    if (thinktime > level.time) {
        return thinktime;
    }

    ent->nextthink = 0;
    think = entityHandlers[ent->handler].think;
    if (!think) {
        Com_Error(1, "\x15NULL ent->think");
    }
    return ((int (*)(gentity_t *))think)(ent);
}

int G_RunThink(gentity_t *ent)
{
    return G_RunThink_core(ent);
}

void G_TraceCapsule(trace_t *results, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentmask)
{
    SV_Trace(results, start, mins, maxs, end, passEntityNum, contentmask, 0, 0, 0);
}

qboolean G_TraceCapsuleComplete(const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentmask)
{
    return SV_TracePassed(start, mins, maxs, end, passEntityNum, 0x3ff, contentmask, 0, 0);
}

void G_LocationalTrace(trace_t *results, const vec_t *start, const vec_t *end, int passEntityNum, int contentmask, unsigned char *priorityMap)
{
    SV_Trace(results, start, vec3_zero, vec3_zero, end, passEntityNum, contentmask, 1, priorityMap, 1);
    return;
}

qboolean G_LocationalTracePassed(const vec_t *start, const vec_t *end, int passEntityNum, int contentmask)
{
    return SV_TracePassed(start, vec3_zero, vec3_zero, end, passEntityNum, 0x3ff, contentmask, 1, 1);
}

int G_SightTrace(int *hitNum, const vec_t *start, const vec_t *end, int passEntityNum, int contentmask)
{
    return SV_SightTrace(hitNum, start, vec3_zero, vec3_zero, end, passEntityNum, 0x3ff, contentmask);
}

void G_AddDebugString(const vec_t *xyz, const vec_t *color, float scale, const char *pszText)
{
    CL_AddDebugString(xyz, color, scale, pszText, 1);
}

void G_ShutdownGame(qboolean freeScripts)
{
    int i;
    char *ptr;

    Com_Printf("==== ShutdownGame ====\n");

    if (level.logFile) {
        G_LogPrintf("ShutdownGame:\n");
        G_LogPrintf("------------------------------------------------------------\n");
        FS_FCloseFile(level.logFile);
    }

    *(void **)imp_bgs = NULL;

    for (i = 0; i < level.num_entities; i++) {
        if (g_entities[i].r.inuse)
            G_FreeEntity(&g_entities[i]);
    }

    if (g_entities[1022].r.inuse)
        G_FreeEntity(&g_entities[1022]);

    level.num_entities = 0;
    *(int *)((char *)&level + 16) = 0;
    *(int *)((char *)&level + 20) = 0;

    HudElem_DestroyAll();

    if (Scr_IsSystemActive(1)) {
        if (!level.savepersist)
            SV_FreeClientScriptPers();
    }

    Scr_ShutdownSystem(1, level.savepersist == 0);

    if (freeScripts) {
        Mantle_ShutdownAnims();
        GScr_FreeScripts();
        Scr_FreeScripts(1);

        for (i = 0; i < 64; i++) {
            struct XAnimTree_s *tree = level_bgs.clientinfo[i].pXAnimTree;
            if (tree && (((uintptr_t)tree & (sizeof(void *) - 1)) == 0)) {
                XAnimFreeTree(tree, 0);
            }
            level_bgs.clientinfo[i].pXAnimTree = NULL;
        }

        {
            scr_data_t *scrData = (scr_data_t *)imp_g_scr_data;
            for (i = 0; i < 8; i++) {
                corpseInfo_t *corpse = &scrData->playerCorpseInfo[i];
                struct XAnimTree_s *tree = corpse->ci.pXAnimTree;
                if (tree && (((uintptr_t)tree & (sizeof(void *) - 1)) == 0)) {
                    XAnimFreeTree(tree, 0);
                }
                corpse->ci.pXAnimTree = NULL;
                corpse->tree = NULL;
            }
        }

        Hunk_ClearToMarkLow(0);
    }

    if (level.openScriptIOFileBuffers[0]) {
        Z_FreeInternal(level.openScriptIOFileBuffers[0]);
    }
    level.openScriptIOFileBuffers[0] = NULL;

    if (level.openScriptIOFileHandles[0] >= 0) {
        FS_FCloseFile(level.openScriptIOFileHandles[0]);
    }
    level.openScriptIOFileHandles[0] = -1;

    return;
}

static void __attribute_regparm__(1) G_RunFrameForEntity(gentity_t *ent)
{
    if (ent->processedFrame == level.framenum) {
        return;
    }

    ent->processedFrame = level.framenum;

    if (!ent->client) {
        if (ent->flags & GMAIN_FL_NODRAW) {
            ent->s.eFlags |= GMAIN_EF_NODRAW;
        } else {
            ent->s.eFlags &= ~GMAIN_EF_NODRAW;
        }
    }

    if (ent->s.eFlags == GMAIN_EFLAGS_UNKNOWN && level.time > ent->s.time2) {
        G_FreeEntity(ent);
        return;
    }

    if (level.time - ent->eventTime > 300) {
        if (ent->freeAfterEvent) {
            G_FreeEntity(ent);
            return;
        }

        if (ent->unlinkAfterEvent) {
            ent->unlinkAfterEvent = 0;
            SV_UnlinkEntity(ent);
        }
    }

    if (ent->freeAfterEvent) {
        return;
    }

    if (ent->s.eType == GMAIN_ET_MISSILE) {
        G_RunMissile(ent);
        return;
    }

    if (ent->s.eType == GMAIN_ET_ITEM) {
        if (ent->tagInfo) {
            G_GeneralLink(ent);
            G_RunThink_core(ent);
            return;
        }

        G_RunItem(ent);
        return;
    }

    if (ent->s.eType == GMAIN_ET_PLAYER_CORPSE) {
        G_RunCorpse(ent);
        return;
    }

    if (ent->physicsObject) {
        G_RunItem(ent);
        return;
    }

    if (ent->s.eType == GMAIN_ET_SCRIPTMOVER) {
        G_RunMover(ent);
        return;
    }

    if (ent->client) {
        G_RunClient(ent);
        return;
    }

    if (ent->s.eType == GMAIN_ET_GENERAL && ent->tagInfo) {
        G_GeneralLink(ent);
    }

    G_RunThink_core(ent);
}

int G_RunFrame(int levelTime)
{
    int i, j;
    unsigned char entIndex[0x400];
    unsigned char index;
    int bMoreTriggered;
    int triggerCount;
    gentity_t *entPtr;
    trigger_info_t *triggerInfo;
    unsigned short entNum, otherNum;
    int savedTriggerCount;

    level.framenum += 1;
    level.previousTime = level.time;
    level.time = levelTime;
    level.frametime = levelTime - level.previousTime;

    level_bgs.time = levelTime;
    level_bgs.latestSnapshotTime = levelTime;
    level_bgs.frametime = level.frametime;

    *(void **)imp_bgs = (void *)&level_bgs;

    { static int sc; if (sc++ < 1) dbg_scan_ents("RunFrame0_start"); }
    for (i = 0; i < level.num_entities; i++) {
        entPtr = &g_entities[i];
        if (entPtr->r.inuse) {
            float dt = (float)level.frametime * 0.001f;
            SV_DObjInitServerTime(entPtr, dt);
        }
    }

    memset(entIndex, 0, sizeof(entIndex));

    triggerCount = level.pendingTriggerListSize;
    Com_Memcpy(level.currentTriggerList, level.pendingTriggerList, triggerCount * sizeof(level.currentTriggerList[0]));
    level.currentTriggerListSize = triggerCount;
    level.pendingTriggerListSize = 0;

    index = 0;

    do {
        index += 1;
        savedTriggerCount = level.currentTriggerListSize;
        bMoreTriggered = 0;

        if (savedTriggerCount <= 0) {

        } else {
            j = 0;
            triggerInfo = level.currentTriggerList;
            while (j < level.currentTriggerListSize) {
                trigger_info_t *info = &triggerInfo[j];
                entNum = info->entnum;
                entPtr = &g_entities[entNum];

                if (entPtr->useCount != info->useCount) {

                    int last = level.currentTriggerListSize - 1;
                    level.currentTriggerListSize = last;
                    triggerInfo[j] = triggerInfo[last];
                    continue;
                } else {

                    gentity_t *otherEnt;

                    otherNum = info->otherEntnum;
                    otherEnt = &g_entities[otherNum];
                    if (otherEnt->useCount != info->otherUseCount) {

                        int last = level.currentTriggerListSize - 1;
                        level.currentTriggerListSize = last;
                        triggerInfo[j] = triggerInfo[last];
                        continue;
                    } else {

                        if (entIndex[entNum] != index) {

                            entIndex[entNum] = index;
                            Scr_AddEntity(otherEnt);
                            Scr_Notify(entPtr, (int)SCR_CONST()->trigger, 1);

                            {
                                int last = level.currentTriggerListSize - 1;
                                level.currentTriggerListSize = last;
                                triggerInfo[j] = triggerInfo[last];
                            }
                            continue;
                        } else {
                            bMoreTriggered = 1;
                        }
                    }
                }
                j++;
            }
        }

        Scr_RunCurrentThreads();
        Com_Printf("[ckpt] G_RunFrame trigger round index=%d size=%d more=%d\n", index, level.currentTriggerListSize, bMoreTriggered);
    } while (bMoreTriggered);
    Com_Printf("[ckpt] G_RunFrame trigger loop DONE\n");
    { static int s; if (s++ < 1) dbg_scan_ents("after_triggers"); }

    for (i = 0; i < level.num_entities; i++) {
        entPtr = &g_entities[i];
        if (!entPtr->r.inuse)
            continue;
        if (entPtr->flags & 0x2000)
            continue;
        while (1) {
            if (!G_DObjUpdateServerTime(entPtr, 1))
                break;
            Scr_RunCurrentThreads();
            if (!entPtr->r.inuse)
                break;
            if (entPtr->flags & 0x2000)
                break;
        }
    }

    Com_Printf("[ckpt] G_RunFrame DObj-update loop DONE\n");
    Scr_IncTime();
    Com_Printf("[ckpt] G_RunFrame Scr_IncTime DONE num_entities=%d\n", level.num_entities);

    level.currentEntityThink = 0;
    for (i = 0; i < level.num_entities; i++) {
        { static int rep; if (!rep && i < level.num_entities && (unsigned int)g_entities[439].s.pos.trType > 20u) {
            rep = 1; Com_Printf("[corrupt] ent#439 trType went bad BEFORE think i=%d (prev ent eType=%d class=%s)\n",
                i, g_entities[i-1].s.eType, SL_ConvertToString((unsigned short)g_entities[i-1].classname)); } }
        entPtr = &g_entities[i];
        if (entPtr->r.inuse) {
            tagInfo_t *tagInfo = COD2_TagInfoFromHandle(entPtr->tagInfo);
            if (tagInfo) {
                gentity_t *parent = tagInfo->parent;
                G_RunFrameForEntity(parent);
            }
            G_RunFrameForEntity(entPtr);
        }
        level.currentEntityThink = i + 1;
    }
    Com_Printf("[ckpt] G_RunFrame think loop DONE\n");
    level.currentEntityThink = -1;

    {
        int numClients = level.maxclients;
        for (i = 0; i < numClients; i++) {
            gentity_t *ent2 = &level.gentities[i];
            if (!ent2->r.inuse)
                continue;
            gclient_t *cli = ent2->client;
            int lastObjId = cli->sess.cs.team;

            for (j = 0; j < 16; j++) {
                objective_t *srcObj = &level.objectives[j];
                objective_t *dstObj = &cli->ps.objective[j];

                if (srcObj->state == 0 || srcObj->teamNum == 0) {
                    if (srcObj->state != 0 && srcObj->teamNum == lastObjId) {
                        *dstObj = *srcObj;
                    } else {
                        dstObj->state = 0;
                    }
                } else {
                    *dstObj = *srcObj;
                }
            }
        }
    }

    {
        int numClients = level.maxclients;
        for (i = 0; i < numClients; i++) {
            gentity_t *ent2 = &level.gentities[i];
            if (!ent2->r.inuse)
                continue;
            HudElem_UpdateClient(ent2->client,
                                 ent2->s.number, 3);
        }
    }

    {
        int numClients = level.maxclients;
        for (i = 0; i < numClients; i++) {
            entPtr = &g_entities[i];
            if (entPtr->r.inuse) {
                ClientEndFrame(entPtr);
            }
        }
    }

    CheckTeamStatus();

    if (g_oldVoting->current.integer) {
        CheckVote();
    }

    if (level.bUpdateScoresForIntermission) {
        int numClients = level.maxclients;
        int foundAny = 0;
        for (i = 0; i < numClients; i++) {
            gclient_t *cl2 = &level.clients[i];
            if (cl2->sess.connected != CON_CONNECTED)
                continue;
            if (cl2->ps.pm_type != 5)
                continue;
            DeathmatchScoreboardMessage(&g_entities[i]);
            foundAny = 1;
        }
        if (!foundAny || numClients <= 0) {
            level.bUpdateScoresForIntermission = 0;
        }
    }

    if (g_listEntity->current.integer) {
        for (i = 0; i < 0x400; i++) {
            entPtr = &g_entities[i];
            Com_Printf("%4i: %s\n", i,
                       (const char *)SL_ConvertToString(entPtr->classname));
        }
        Dvar_SetBool(g_listEntity, 0);
    }

    if (level.registerWeapons) {
        SaveRegisteredWeapons();
    }
    if (level.bRegisterItems) {
        SaveRegisteredItems();
    }

    if (g_dumpAnims->current.integer >= 0) {
        Com_Printf("---------- Animtree dump ----------\n");
        {
            int animIdx = g_dumpAnims->current.integer;
            entPtr = &level.gentities[animIdx];
            SV_DObjDisplayAnim(entPtr);
        }
    }

    *(void **)imp_bgs = NULL;
    return (int)imp_bgs;
}

entityHandler_t entityHandlers[20] = {
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, &Touch_Multi, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, &hurt_use, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, &hurt_touch, &hurt_use, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, &Use_trigger_damage, &Pain_trigger_damage, &Die_trigger_damage, 0x0, 0x0, 0x0 },
    { 0x0, &Reached_ScriptMover, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, &Reached_ScriptMover, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { &G_ExplodeMissile, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0x4 },
    { &G_ExplodeMissile, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x5, 0x6 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, &player_die, &G_PlayerController, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, &player_die, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, &G_PlayerController, 0x0, 0x0 },
    { &BodyEnd, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { &turret_think_init, 0x0, 0x0, 0x0, &turret_use, 0x0, 0x0, &turret_controller, 0x0, 0x0 },
    { &turret_think, 0x0, 0x0, 0x0, &turret_use, 0x0, 0x0, &turret_controller, 0x0, 0x0 },
    { &DroppedItemClearOwner, 0x0, 0x0, &Touch_Item_Auto, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { &FinishSpawningItem, 0x0, 0x0, &Touch_Item_Auto, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, &Touch_Item_Auto, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, &use_trigger_use, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { &G_FreeEntity, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }
};
