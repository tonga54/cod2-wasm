#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include "cod2_feature_config.h"
#include <string.h>
#include <stdlib.h>

extern int sv_serverId_value;

#if COD2_IS_PATCH_13

const dvar_t *sv_punkbuster;
const dvar_t *sv_wwwDownload;
const dvar_t *sv_wwwBaseURL;
const dvar_t *sv_wwwDlDisconnected;
#endif

extern server_t sv;
extern serverStatic_t svs;
extern byte *sv_maxclients_dvar;
extern byte *sv_gametype_dvar;
extern byte *sv_dedicated_dvar;
extern byte *sv_running_dvar;
extern byte *sv_serverid_dvar;
extern byte *sv_expectedHunkUsage_dvar;
extern byte *sv_mapname_dvar;
extern byte *sv_privateClients_dvar;
extern byte *sv_hostname_dvar;
extern byte *sv_maxRate_dvar;
extern byte *sv_minPing_dvar;
extern byte *sv_maxPing_dvar;
extern byte *sv_floodProtect_dvar;
extern byte *sv_allowAnonymous_dvar;
extern byte *sv_showCommands_dvar;
extern byte *sv_disableClientConsole_dvar;
extern byte *sv_voice_dvar;
extern byte *sv_voiceQuality_dvar;
extern byte *sv_cheats_dvar;
extern byte *sv_pure_dvar;
extern byte *sv_iwds_dvar;
extern byte *sv_iwdNames_dvar;
extern byte *sv_referencedIwds_dvar;
extern byte *sv_referencedIwdNames_dvar;
extern byte *rcon_password_dvar;
extern byte *sv_privatePassword_dvar;
extern byte *sv_fps_dvar;
extern byte *sv_timeout_dvar;
extern byte *sv_zombietime_dvar;
extern byte *sv_allowDownload_dvar;
extern byte *sv_reconnectlimit_dvar;
extern byte *sv_padPackets_dvar;
extern byte *sv_allowedClan1_dvar;
extern byte *sv_allowedClan2_dvar;
extern byte *sv_packet_info_dvar;
extern byte *sv_showAverageBPS_dvar;
extern byte *sv_kickBanTime_dvar;
extern byte *sv_mapRotation_dvar;
extern byte *sv_mapRotationCurrent_dvar;
extern byte *sv_debugRate_dvar;
extern byte *sv_debugReliableCmds_dvar;
extern int nextmap;
extern byte *com_dvarflags_ptr;
extern byte *com_checksumFeed_dvar;
extern void *imp_com_errorEntered;

extern byte *sv_com_dvarDump_ptr;

extern void Com_Error(int code, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern int I_stricmp(const char *s1, const char *s2);
extern int stricmp(const char *s1, const char *s2);
extern void *Z_MallocInternal(int size);
extern void Z_FreeInternal(void *ptr);
extern void *Z_VirtualAllocInternal(int size);
extern void Z_VirtualFreeInternal(void *ptr);
extern void *Hunk_AllocInternal(int size);
extern void *Hunk_AllocateTempMemoryInternal(int size);
extern void Hunk_FreeTempMemory(void *buf);
extern int FS_FOpenFileByMode(const char *qpath, fileHandle_t *f, int mode);
extern int FS_Read(void *buffer, int len, fileHandle_t f);
extern void FS_FCloseFile(fileHandle_t f);
extern char *Com_Parse(const char **data_p);
extern void ReplaceStringInternal(const char **dest, const char *src);
extern char *CopyStringInternal(const char *in);
extern void SV_SendServerCommand(void *client, int reliable, const char *fmt, ...);
extern dvar_t *Dvar_RegisterString(const char *dvarName, const char *defaultValue, int flags);
extern dvar_t *Dvar_RegisterInt(const char *dvarName, int defaultValue, int min, int max, int flags);
extern dvar_t *Dvar_RegisterBool(const char *dvarName, qboolean defaultValue, int flags);
extern dvar_t *Dvar_RegisterFloat(const char *dvarName, float defaultValue, float min, float max, int flags);
extern void Dvar_SetInt(dvar_t *dvar, int value);
extern void Dvar_SetBool(dvar_t *dvar, int value);
extern void Dvar_SetString(dvar_t *dvar, const char *value);
extern void Dvar_SetStringByName(const char *dvarName, const char *value);
extern void Dvar_ClearModified(dvar_t *dvar);
extern void Dvar_ResetScriptInfo(void);
extern char *Dvar_InfoString(int bit);
extern char *Dvar_InfoString_Big(int bit);
extern const char *Info_ValueForKey(const char *s, const char *key);
extern void Com_Memset(void *dest, int val, int count);
extern void SV_AddOperatorCommands(void);
extern void SV_RemoveOperatorCommands(void);
extern void SV_MasterShutdown(void);
extern void SV_ShutdownGameProgs(void);
extern void SV_FreeClients(void);
extern void SV_DropClient(void *client, const char *reason);
extern void SV_SendClientSnapshot(void *client);
extern void SV_RunFrame(void);
extern void SV_Heartbeat_f(void);
extern void SV_MasterGameCompleteStatus(void);
extern void SV_SetConfig(int start, int max, int count);
extern void SV_SetGametype(void);
extern void SV_InitGameProgs(int savepersist);
extern void *SV_GentityNum(int num);
extern void CL_InitLoad(const char *server, const char *gametype);
extern void CL_MapLoading(const char *server);
extern void CL_ShutdownAll(void);
extern void CL_StartLoading(const char *server, const char *gametype);
extern void CL_Disconnect(void);
extern int CL_GetLocalClientActive(int localClientNum);
extern void UI_LoadIngameMenus(void);
extern void Com_Restart(void);
extern void Material_PreLoadAllShaderText(void);
extern void Com_LoadBsp(const char *filename);
extern void CM_LoadMap(const char *name, int *checksum);
extern void Com_UnloadBsp(void);
extern void CM_LinkWorld(void);
extern void Com_LoadSoundAliases(const char *filename, const char *name, snd_alias_system_t type);
extern void Com_AbortDObj(void);
extern void DObjAbort(void);
extern void XAnimAbort(void);
extern void Scr_Abort(void);
extern void Scr_ParseGameTypeList(void);
extern void FX_FreeSystem(void);
extern int FX_InitSystem(int dummy);
extern void FX_CreateDefaultEffect(void);
extern void FS_Shutdown(int closemfp);
extern void FS_ClearIwdReferences(void);
extern void FS_Restart(int checksumFeed);
extern const char *FS_LoadedIwdChecksums(void);
extern const char *FS_LoadedIwdNames(void);
extern const char *FS_ReferencedIwdChecksums(void);
extern const char *FS_ReferencedIwdNames(void);
extern const char *GetBspExtension(void);
extern int Sys_Milliseconds(void);
extern int Sys_MillisecondsRaw(void);
extern void NET_Sleep(int msec);
extern Bool NET_OutOfBandPrint(int sock, netadr_t adr, const char *data);
extern int *G_GetSavePersist(void);
extern const char *ClientConnect(int clientNum, unsigned short scriptId);


#define SV_STATE_OFF 0x0
#define SV_CHECKSUM_OFF 0x8
#define SV_CONFIGSTRINGS_OFF 0x418
#define SV_NUMENTITIES_OFF 0x5F424

COD2_ASSERT_FIELD(server_t, state,         0x0);
COD2_ASSERT_FIELD(server_t, restarting,    0x4);
COD2_ASSERT_FIELD(server_t, checksumFeed,  0xc);
COD2_ASSERT_FIELD(server_t, configstrings, 0x418);
COD2_ASSERT_FIELD(server_t, num_entities,  0x5f424);
COD2_ASSERT_FIELD(server_t, gametype,      0x5f4f4);

#if defined(__x86_64__) || defined(_M_X64)
#    define SV_CONFIGSTRING_SLOT(svbase, index) (((server_t *)(svbase))->configstrings[(index)])
#else
#    define SV_CONFIGSTRING_SLOT(svbase, index) (*(const char **)((byte *)(svbase) + SV_CONFIGSTRINGS_OFF + (index) * 4))
#endif

Bool SV_Loaded(void);
void SV_GetConfigstring(int index, char *buffer, int bufferSize);
const char *SV_GetConfigstringConst(int index);
void SV_SetUserinfo(int index, const char *val);
void SV_GetUserinfo(int index, char *buffer, int bufferSize);
void SV_SetExpectedHunkUsage(char *mapname);
void SV_EnableArchivedSnapshot(qboolean bEnable);
void SV_InitArchivedSnapshot(void);
void SV_InitDvar(void);
void SV_Init(void);
void SV_SetConfigstring(const int index, const char *val);
void SV_SetConfigValueForKey(int start, int max, const char *key, const char *value);
void SV_Shutdown(char *finalmsg);
void SV_Startup(void);
void SV_ChangeMaxClients(void);
void SV_SpawnServer(const char *server);

static void SV_ClearServer(void);
static void SV_FreeArchivedSnapshotBuffers(void);
static void SV_FreeServerStructure(void);
static void SV_TouchMemory(void);
static void SV_DropAllClients(void);
static void SV_InitMaxClients(void);
static void SV_SendMapChange(void);

Bool SV_Loaded(void)
{
    server_t *sv = (server_t *)imp_sv;
    return sv->state == 2;
}

void SV_GetConfigstring(int index, char *buffer, int bufferSize)
{
    byte *sv;
    const char *s;

    if (bufferSize <= 0) {
        Com_Error(1, "\025SV_GetConfigstring: bufferSize == %i", bufferSize);
    }
    if ((unsigned int)index > 0x7ff) {
        Com_Error(1, "\025SV_GetConfigstring: bad index %i\n", index);
    }

    sv = (byte *)imp_sv;
    s = SV_CONFIGSTRING_SLOT(sv, index);
    if (!s) {
        *buffer = '\0';
        return;
    }

    I_strncpyz(buffer, s, bufferSize);
}

const char *SV_GetConfigstringConst(int index)
{
    byte *sv;
    const char *s;
    const char *result = "";

    sv = (byte *)imp_sv;
    s = SV_CONFIGSTRING_SLOT(sv, index);
    if (s)
        result = s;
    return result;
}

void SV_SetUserinfo(int index, const char *val)
{
    byte *svs;
    byte *client;
    const char *name;

    if (index < 0 || index >= (*(dvar_t **)imp_sv_maxclients)->current.integer) {
        Com_Error(1, "SV_SetUserinfo: bad index %i\n", index);
    }

    if (!val)
        val = "";

    {
        serverStatic_t *svsPtr = (serverStatic_t *)imp_svs;

        I_strncpyz(svsPtr->clients[index].userinfo, val, 0x400);
        name = (const char *)Info_ValueForKey(val, "name");
        I_strncpyz(svsPtr->clients[index].name, name, 0x20);
    }
}

void SV_GetUserinfo(int index, char *buffer, int bufferSize)
{
    byte *svs;
    byte *client;

    if (bufferSize <= 0) {
        Com_Error(1, "\025SV_GetUserinfo: bufferSize == %i", bufferSize);
    }
    if (index < 0 || index >= (*(dvar_t **)imp_sv_maxclients)->current.integer) {
        Com_Error(1, "\025SV_GetUserinfo: bad index %i\n", index);
    }

    {
        serverStatic_t *svsPtr = (serverStatic_t *)imp_svs;
        client_t *cl = &svsPtr->clients[index];

        I_strncpyz(buffer, cl->userinfo, bufferSize);
    }
}

void SV_SetExpectedHunkUsage(char *mapname)
{
    fileHandle_t handle;
    int len;
    char *buf;
    char *buftrav;
    char *token;

    len = FS_FOpenFileByMode("hunkusage.dat", &handle, 0);
    if (len < 0)
        return;

    buf = (char *)Z_MallocInternal(len + 1);
    memset(buf, 0, len + 1);
    FS_Read(buf, len, handle);
    FS_FCloseFile(handle);
    buftrav = buf;

    while (1) {
        token = Com_Parse((const char **)&buftrav);
        if (!token || *token == '\0')
            break;
        if (I_stricmp(token, mapname) == 0) {
            token = Com_Parse((const char **)&buftrav);
            if (!token || *token == '\0')
                continue;
            Dvar_SetInt(*(dvar_t **)imp_com_expectedHunkUsage, atoi(token));
            Z_FreeInternal(buf);
            return;
        }
    }

    Z_FreeInternal(buf);
}

void SV_EnableArchivedSnapshot(qboolean bEnable)
{
    serverStatic_t *svs;

    svs = (serverStatic_t *)imp_svs;
    svs->archiveEnabled = bEnable;

    if (!bEnable)
        return;
    if (svs->archivedSnapshotFrames)
        return;

    *(void **)&svs->cachedSnapshotEntities = Z_MallocInternal(0x450000);
    *(void **)&svs->cachedSnapshotClients = Z_MallocInternal(0x2708000);
    *(void **)&svs->archivedSnapshotFrames = Z_MallocInternal(0x2580);
    svs->archivedSnapshotBuffer = Z_MallocInternal(0x2000000);
    svs->cachedSnapshotFrames = Z_MallocInternal(0x3800);
}

void SV_InitArchivedSnapshot(void)
{
    serverStatic_t *svs;

    svs = (serverStatic_t *)imp_svs;
    svs->archiveEnabled = 0;
    svs->nextArchivedSnapshotFrames = 0;
    svs->nextArchivedSnapshotBuffer = 0;
    svs->nextCachedSnapshotEntities = 0;
    svs->nextCachedSnapshotClients = 0;
    svs->nextCachedSnapshotFrames = 0;
}

void SV_InitDvar(void)
{
    Dvar_ResetScriptInfo();
}

void SV_Init(void)
{
    SV_AddOperatorCommands();

    *(dvar_t **)imp_sv_gametype = Dvar_RegisterString("g_gametype", "dm", 0x1024);
    Dvar_RegisterString("sv_keywords", "", 0x1004);
    Dvar_RegisterInt("protocol", 0x76, 0x76, 0x76, 0x1044);
    *(dvar_t **)imp_sv_mapname = Dvar_RegisterString("mapname", "", 0x1044);
    *(dvar_t **)imp_sv_privateClients = Dvar_RegisterInt("sv_privateClients", 0, 0, 0x40, 0x1004);
    *(dvar_t **)imp_sv_maxclients = Dvar_RegisterInt("sv_maxclients", 20, 1, 0x40, 0x1025);
    *(dvar_t **)imp_sv_hostname = Dvar_RegisterString("sv_hostname", "CoD2Host", 0x1005);
#if COD2_IS_PATCH_13
    sv_punkbuster = Dvar_RegisterBool("sv_punkbuster", 0, 0x1015);
#endif
    *(dvar_t **)imp_sv_maxRate = Dvar_RegisterInt("sv_maxRate", 0, 0, 25000, 0x1005);
    *(dvar_t **)imp_sv_minPing = Dvar_RegisterInt("sv_minPing", 0, 0, 999, 0x1005);
    *(dvar_t **)imp_sv_maxPing = Dvar_RegisterInt("sv_maxPing", 0, 0, 999, 0x1005);
    *(dvar_t **)imp_sv_floodProtect = Dvar_RegisterBool("sv_floodProtect", 1, 0x1005);
    *(dvar_t **)imp_sv_allowAnonymous = Dvar_RegisterBool("sv_allowAnonymous", 0, 0x1004);
    *(dvar_t **)imp_sv_showCommands = Dvar_RegisterBool("sv_showCommands", 0, 0x1000);
    *(dvar_t **)imp_sv_disableClientConsole = Dvar_RegisterBool("sv_disableClientConsole", 0, 0x1008);
    *(dvar_t **)imp_sv_voice = Dvar_RegisterBool("sv_voice", 0, 0x100d);
    *(dvar_t **)imp_sv_voiceQuality = Dvar_RegisterInt("sv_voiceQuality", 1, 0, 9, 0x1008);
    *(dvar_t **)imp_sv_cheats = Dvar_RegisterBool("sv_cheats", 0, 0x1018);
    *(dvar_t **)imp_sv_serverid = Dvar_RegisterInt("sv_serverid", 0, (int)0x80000000, 0x7fffffff, 0x1048);
    *(dvar_t **)imp_sv_pure = Dvar_RegisterBool("sv_pure", 1, 0x100c);
    *(dvar_t **)imp_sv_iwds = Dvar_RegisterString("sv_iwds", "", 0x1048);
    *(dvar_t **)imp_sv_iwdNames = Dvar_RegisterString("sv_iwdNames", "", 0x1048);
    *(dvar_t **)imp_sv_referencedIwds = Dvar_RegisterString("sv_referencedIwds", "", 0x1048);
    *(dvar_t **)imp_sv_referencedIwdNames = Dvar_RegisterString("sv_referencedIwdNames", "", 0x1048);
    *(dvar_t **)imp_rcon_password = Dvar_RegisterString("rcon_password", "", 0x1000);
    *(dvar_t **)imp_sv_privatePassword = Dvar_RegisterString("sv_privatePassword", "", 0x1000);
    *(dvar_t **)imp_sv_fps = Dvar_RegisterInt("sv_fps", 20, 10, 1000, 0x1000);
    *(dvar_t **)imp_sv_timeout = Dvar_RegisterInt("sv_timeout", 240, 0, 1800, 0x1000);
    *(dvar_t **)imp_sv_zombietime = Dvar_RegisterInt("sv_zombietime", 2, 0, 1800, 0x1000);
    *(dvar_t **)imp_sv_allowDownload = Dvar_RegisterBool("sv_allowDownload", 1, 0x1001);
    *(dvar_t **)imp_sv_reconnectlimit = Dvar_RegisterInt("sv_reconnectlimit", 3, 0, 1800, 0x1001);
    *(dvar_t **)imp_sv_padPackets = Dvar_RegisterInt("sv_padPackets", 0, 0, 0x7fffffff, 0x1000);

    {

        (*(LegacyHacks **)imp_legacyHacks)->sv_killserver = 0;
    }

    *(dvar_t **)imp_sv_allowedClan1 = Dvar_RegisterString("sv_allowedClan1", "", 0x1000);
    *(dvar_t **)imp_sv_allowedClan2 = Dvar_RegisterString("sv_allowedClan2", "", 0x1000);
    *(dvar_t **)imp_sv_packet_info = Dvar_RegisterBool("sv_packet_info", 0, 0x1000);
    *(dvar_t **)imp_sv_showAverageBPS = Dvar_RegisterBool("sv_showAverageBPS", 0, 0x1000);
    *(dvar_t **)imp_sv_kickBanTime = Dvar_RegisterFloat("sv_kickBanTime", 300.0f, 0.0f, 3600.0f, 0x1000);
    *(dvar_t **)imp_sv_mapRotation = Dvar_RegisterString("sv_mapRotation", "", 0x1000);
    *(dvar_t **)imp_sv_mapRotationCurrent = Dvar_RegisterString("sv_mapRotationCurrent", "", 0x1000);
    *(dvar_t **)imp_sv_debugRate = Dvar_RegisterBool("sv_debugRate", 0, 0x1000);
    *(dvar_t **)imp_sv_debugReliableCmds = Dvar_RegisterBool("sv_debugReliableCmds", 0, 0x1000);
#if COD2_IS_PATCH_13
    sv_wwwDownload = Dvar_RegisterBool("sv_wwwDownload", 0, 0x1001);
    sv_wwwBaseURL = Dvar_RegisterString("sv_wwwBaseURL", "", 0x1001);
    sv_wwwDlDisconnected = Dvar_RegisterBool("sv_wwwDlDisconnected", 0, 0x1001);
#endif
    *(dvar_t **)&nextmap = Dvar_RegisterString("nextmap", "", 0x1000);
    *(dvar_t **)imp_com_expectedHunkUsage = Dvar_RegisterInt("com_expectedHunkUsage", 0, 0, 0x7fffffff, 0x1040);
}

void SV_SetConfigstring(const int index, const char *val)
{
    byte *sv;
    const char *old;
    int len;
    int maxChunk;
    char buf[0x400];
    int i;
    int sent;
    int remaining;
    byte *client;
    char cmd;

    if (index > 0x7ff) {
        Com_Error(1, "SV_SetConfigstring: bad index %i\n", index);
    }

    if (!val)
        val = "";

    sv = (byte *)imp_sv;
    old = SV_CONFIGSTRING_SLOT(sv, index);
    if (!old)
        return;
    if (strcmp(val, old) == 0)
        return;

    ReplaceStringInternal((const char **)&SV_CONFIGSTRING_SLOT(sv, index), val);

    if (((server_t *)sv)->state != 2) {
        if (((server_t *)sv)->restarting == 0)
            return;
    }

    len = strlen(val);
    sprintf(buf, "%i", index);
    maxChunk = 0x3fd - strlen(buf);

    {
        serverStatic_t *svsPtr = (serverStatic_t *)imp_svs;
        for (i = 0; i < (*(dvar_t **)imp_sv_maxclients)->current.integer; i++) {
            if (svsPtr->clients[i].state <= 2)
                goto sv_next;

            if (len <= maxChunk) {
                SV_SendServerCommand(&svsPtr->clients[i], 1, "%c %i %s", 'd', index, val);
            } else {
                remaining = len;
                sent = 0;
                cmd = 'x';
                while (remaining > 0) {
                    I_strncpyz(buf, val + sent, maxChunk + 1);
                    SV_SendServerCommand(&svsPtr->clients[i], 1, "%c %i %s", cmd, index, buf);
                    sent += maxChunk;
                    remaining -= maxChunk;
                    if (remaining <= 0)
                        break;
                    if (sent != 0) {
                        cmd = (maxChunk >= remaining) ? 'z' : 'y';
                    }
                }
            }
        sv_next:;
        }
    }
}

void SV_SetConfigValueForKey(int start, int max, const char *key, const char *value)
{
    byte *sv;
    int i;
    char *cs;

    sv = (byte *)imp_sv;
    for (i = 0; i < max; i++) {
        cs = (char *)SV_CONFIGSTRING_SLOT(sv, start + i);
        if (*cs == '\0') {
            SV_SetConfigstring(start + i, key);
            break;
        }
        if (stricmp(key, cs) == 0)
            break;
    }
    if (i == max) {
        Com_Error(1, "SV_SetConfigValueForKey: overflow");
    }
    SV_SetConfigstring(start + max + i, value);
}

void SV_Shutdown(char *finalmsg)
{
    int savedState;
    serverStatic_t *svs;
    int i, j;
    int maxclients;

    {
        dvar_t *dvar = *(dvar_t **)imp_com_sv_running;
        byte *dvarVal;
        if (!dvar)
            return;
        dvarVal = *(byte **)dvar;
        if (!dvarVal || *(char *)(dvarVal + 8) == 0)
            return;
    }

    Com_Printf("----- Server Shutdown -----\n");

    savedState = *(int *)(byte *)imp_sv;

    svs = (serverStatic_t *)imp_svs;
    if (svs->clients) {
        for (j = 0; j < 2; j++) {
            maxclients = (*(dvar_t **)imp_sv_maxclients)->current.integer;
            for (i = 0; i < maxclients; i++) {
                if (svs->clients[i].state <= 1)
                    goto next_client1;

                if (svs->clients[i].netchan.remoteAddress.type != 2) {
                    SV_SendServerCommand(&svs->clients[i], 0, "%c \"%s\"", 'e', finalmsg);
                    SV_SendServerCommand(&svs->clients[i], 1, "%c \"%s\"", 'w', finalmsg);
                }

                svs->clients[i].nextSnapshotTime = -1;
                SV_SendClientSnapshot(&svs->clients[i]);
            next_client1:;
            }
        }
    }

    SV_RemoveOperatorCommands();
    SV_MasterShutdown();
    SV_ShutdownGameProgs();

    svs = (serverStatic_t *)imp_svs;
    if (svs->clients != NULL && (*(dvar_t **)imp_sv_maxclients) != NULL) {
        maxclients = (*(dvar_t **)imp_sv_maxclients)->current.integer;
        for (i = 0; i < maxclients; i++) {
            if (svs->clients[i].state > 1) {
                SV_DropClient(&svs->clients[i], "EXE_DISCONNECTED");
            }
        }
    }

    {
        byte *svLocal = (byte *)imp_sv;
#if defined(__x86_64__) || defined(_M_X64)

        {
            server_t *svc = (server_t *)svLocal;
            int csi;
            for (csi = 0; csi < 2048; csi++) {
                if (svc->configstrings[csi]) {
                    Z_FreeInternal(svc->configstrings[csi]);
                }
            }
        }
        Com_Memset(svLocal, 0, (int)sizeof(server_t));
#else
        byte *p = svLocal;
        byte *end = svLocal + sizeof(((server_t *)0)->configstrings);
        while (p < end) {
            char *cs = *(char **)(p + SV_CONFIGSTRINGS_OFF);
            if (cs) {
                Z_FreeInternal(cs);
            }
            p += 4;
        }
        Com_Memset(svLocal, 0, 0x5f534);
#endif
    }

    svs = (serverStatic_t *)imp_svs;
    if (svs->clients) {
        SV_FreeClients();
    }

    if (svs->cachedSnapshotEntities) {
        Z_FreeInternal(svs->cachedSnapshotEntities);
        *(void **)&svs->cachedSnapshotEntities = 0;
    }
    if (svs->cachedSnapshotClients) {
        Z_FreeInternal(svs->cachedSnapshotClients);
        *(void **)&svs->cachedSnapshotClients = 0;
    }
    if (svs->archivedSnapshotFrames) {
        Z_FreeInternal(svs->archivedSnapshotFrames);
        *(void **)&svs->archivedSnapshotFrames = 0;
    }
    if (svs->archivedSnapshotBuffer) {
        Z_FreeInternal(svs->archivedSnapshotBuffer);
        svs->archivedSnapshotBuffer = 0;
    }
    if (svs->cachedSnapshotFrames) {
        Z_FreeInternal(svs->cachedSnapshotFrames);
        svs->cachedSnapshotFrames = 0;
    }

    memset(svs, 0, sizeof(serverStatic_t));

    {
        if ((*(dvar_t **)imp_com_dedicated)->current.integer) {
            FX_FreeSystem();
        }
    }

    Dvar_SetBool(*(dvar_t **)imp_com_sv_running, 0);
    Com_Printf("---------------------------\n");

    if (CL_GetLocalClientActive(0)) {
        CL_Disconnect();
    }

    if (savedState == 1) {
        Com_AbortDObj();
        DObjAbort();
        XAnimAbort();
        Scr_Abort();
        *(int *)imp_com_errorEntered = 0;
    }
}

void SV_Startup(void)
{
    serverStatic_t *svs;
    dvar_t *maxclients;
    int numClients;
    int isDedicated;

    svs = (serverStatic_t *)imp_svs;
    if (svs->initialized) {
        Com_Error(0, "SV_Startup: svs.initialized");
    }

    maxclients = Dvar_RegisterInt("sv_maxclients", 20, 1, 0x40, 0x1025);
    *(dvar_t **)imp_sv_maxclients = maxclients;
    Dvar_ClearModified(maxclients);

    maxclients = *(dvar_t **)imp_sv_maxclients;
    if (maxclients->current.integer <= 0) {
        Dvar_SetInt(maxclients, 1);
        maxclients = *(dvar_t **)imp_sv_maxclients;
    }

    numClients = maxclients->current.integer;
    svs->clients = Z_VirtualAllocInternal(numClients * sizeof(client_t));
    if (!svs->clients) {
        Com_Error(0, "SV_Startup: unable to allocate svs.clients");
    }

    isDedicated = (*(const dvar_t **)imp_com_dedicated)->current.integer;
    maxclients = *(dvar_t **)imp_sv_maxclients;
    numClients = maxclients->current.integer;
    if (isDedicated) {
        svs->numSnapshotEntities = numClients << 11;
        svs->numSnapshotClients = numClients * numClients * 32;
    } else {
        svs->numSnapshotEntities = numClients << 8;
        svs->numSnapshotClients = numClients * numClients * 4;
    }

    svs->initialized = 1;
    Dvar_SetBool(*(dvar_t **)imp_com_sv_running, 1);
}

void SV_ChangeMaxClients(void)
{
    serverStatic_t *svs;
    int oldMaxClients;
    int minClients;
    client_t *oldClients;
    int i;
    dvar_t *maxclients;
    int numClients;
    int isDedicated;

    oldMaxClients = (*(dvar_t **)imp_sv_maxclients)->current.integer;

    if (oldMaxClients > 0) {
        svs = (serverStatic_t *)imp_svs;
        minClients = 0;
        for (i = 0; i < oldMaxClients; i++) {
            if (svs->clients[i].state > 1) {
                if (i > minClients)
                    minClients = i;
            }
        }
        minClients++;
    } else {
        minClients = 1;
    }

    maxclients = Dvar_RegisterInt("sv_maxclients", 20, 1, 0x40, 0x1025);
    *(dvar_t **)imp_sv_maxclients = maxclients;
    Dvar_ClearModified(maxclients);
    maxclients = *(dvar_t **)imp_sv_maxclients;
    if (maxclients->current.integer < minClients) {
        Dvar_SetInt(maxclients, minClients);
        maxclients = *(dvar_t **)imp_sv_maxclients;
    }

    if (maxclients->current.integer == oldMaxClients)
        return;

    oldClients = (client_t *)Hunk_AllocateTempMemoryInternal(minClients * sizeof(client_t));
    for (i = 0; i < minClients; i++) {
        svs = (serverStatic_t *)imp_svs;
        if (svs->clients[i].state > 1) {
            memcpy(&oldClients[i], &svs->clients[i], sizeof(client_t));
        } else {
            Com_Memset(&oldClients[i], 0, sizeof(client_t));
        }
    }

    svs = (serverStatic_t *)imp_svs;
    Z_VirtualFreeInternal(svs->clients);

    numClients = (*(dvar_t **)imp_sv_maxclients)->current.integer;
    svs->clients = Z_VirtualAllocInternal(numClients * sizeof(client_t));
    if (!svs->clients) {
        Com_Error(0, "SV_Startup: unable to allocate svs.clients");
    }

    numClients = (*(dvar_t **)imp_sv_maxclients)->current.integer;
    Com_Memset(svs->clients, 0, numClients * sizeof(client_t));

    for (i = 0; i < minClients; i++) {
        if (oldClients[i].state > 1) {
            svs = (serverStatic_t *)imp_svs;
            memcpy(&svs->clients[i], &oldClients[i], sizeof(client_t));
        }
    }

    Hunk_FreeTempMemory(oldClients);

    isDedicated = (*(const dvar_t **)imp_com_dedicated)->current.integer;
    numClients = (*(dvar_t **)imp_sv_maxclients)->current.integer;
    svs = (serverStatic_t *)imp_svs;
    if (isDedicated) {
        svs->numSnapshotEntities = numClients << 11;
        svs->numSnapshotClients = numClients * numClients * 32;
    } else {
        svs->numSnapshotEntities = numClients << 8;
        svs->numSnapshotClients = numClients * numClients * 4;
    }
}

void SV_SpawnServer(const char *server)
{
    serverStatic_t *svsg = (serverStatic_t *)imp_svs;
    server_t *svg = (server_t *)imp_sv;
    byte *sv;
    byte *svs;
    byte *client;
    int i;
    int maxclients;
    int checksum;
    char filename[64];
    int savepersist;
    char systemInfo[0x2000];
    int isDedicated;
    const char *s;
    byte *ent;
    netadr_t addr;

    Scr_ParseGameTypeList();
    SV_SetGametype();

    CL_InitLoad(server, (*(dvar_t **)imp_sv_gametype)->current.string);

    if ((*(dvar_t **)imp_com_sv_running)->current.enabled) {
        savepersist = (int)G_GetSavePersist();

        {
            serverStatic_t *svsPtr = (serverStatic_t *)imp_svs;
            maxclients = (*(dvar_t **)imp_sv_maxclients)->current.integer;
            for (i = 0; i < maxclients; i++) {
                if (svsPtr->clients[i].state > 2) {
                    Com_sprintf(filename, 64, "loadingnewmap\n%s\n%s",
                                server, (*(dvar_t **)imp_sv_gametype)->current.string);
                    addr = svsPtr->clients[i].netchan.remoteAddress;
                    NET_OutOfBandPrint(1, addr, filename);
                }
            }
        }
        NET_Sleep(250);
    } else {
        savepersist = 0;
    }

    Dvar_SetStringByName("mapname", server);
    CL_MapLoading(server);
    CL_ShutdownAll();
    SV_ShutdownGameProgs();

    Com_Printf("------ Server Initialization ------\n");
    Com_Printf("Server: %s\n", server);

    sv = (byte *)imp_sv;
    {
#if defined(__x86_64__) || defined(_M_X64)

        server_t *svc = (server_t *)sv;
        int csi;
        for (csi = 0; csi < 2048; csi++) {
            if (svc->configstrings[csi]) {
                Z_FreeInternal(svc->configstrings[csi]);
            }
        }
        Com_Memset(sv, 0, (int)sizeof(server_t));
#else
        byte *p = sv;
        byte *end = sv + sizeof(((server_t *)0)->configstrings);
        while (p < end) {
            char *cs = *(char **)(p + SV_CONFIGSTRINGS_OFF);
            if (cs) {
                Z_FreeInternal(cs);
            }
            p += 4;
        }
        Com_Memset(sv, 0, 0x5f534);
#endif
    }

    isDedicated = (*(const dvar_t **)imp_com_dedicated)->current.integer;
    if (isDedicated) {
        FX_FreeSystem();
    }

    FS_Shutdown(1);
    FS_ClearIwdReferences();
    Com_Restart();

    if ((*(dvar_t **)imp_com_sv_running)->current.enabled) {
        SV_ChangeMaxClients();
    } else {
        SV_Startup();
    }

    I_strncpyz(svg->gametype,
               (*(dvar_t **)imp_sv_gametype)->current.string, 64);

    srand(Sys_MillisecondsRaw());
    {
        int r1 = rand();
        int r2 = rand();
        int ms = Sys_Milliseconds();
        int checksumFeed = (r1 << 16) ^ r2 ^ ms;
        svg->checksumFeed = checksumFeed;
        FS_Restart(checksumFeed);
    }

    {
        const char *ext = GetBspExtension();
        Com_sprintf(filename, 64, "maps/mp/%s.%s", server, ext);
    }
    SV_SetExpectedHunkUsage(filename);
    CL_StartLoading(server, svg->gametype);

    {
        extern unsigned char mtlLoadGlob[];
        if (*(int *)mtlLoadGlob == 0)
            Material_PreLoadAllShaderText();
    }

    sv = (byte *)imp_sv;
    {
#if defined(__x86_64__) || defined(_M_X64)

        server_t *svc = (server_t *)sv;
        int csi;
        for (csi = 0; csi < 2048; csi++) {
            svc->configstrings[csi] = CopyStringInternal("");
        }
#else
        byte *p = sv;
        byte *end = sv + sizeof(((server_t *)0)->configstrings);
        while (p < end) {
            *(char **)(p + SV_CONFIGSTRINGS_OFF) = CopyStringInternal("");
            p += 4;
        }
#endif
    }

    Dvar_ResetScriptInfo();

    {
        serverStatic_t *svsPtr = (serverStatic_t *)imp_svs;
        int numSnapEnts = svsPtr->numSnapshotEntities;
        svsPtr->snapshotEntities = Hunk_AllocInternal((numSnapEnts * 256) - (numSnapEnts * 16));
        svsPtr->nextSnapshotEntities = 0;
        {
            int numSnapClients = svsPtr->numSnapshotClients;
            *(void **)&svsPtr->snapshotClients = Hunk_AllocInternal(numSnapClients * 92);
        }
        svsPtr->nextSnapshotClients = 0;

        svsPtr->archiveEnabled = 0;
        svsPtr->nextArchivedSnapshotFrames = 0;
        svsPtr->nextArchivedSnapshotBuffer = 0;
        svsPtr->nextCachedSnapshotEntities = 0;
        svsPtr->nextCachedSnapshotClients = 0;
        svsPtr->nextCachedSnapshotFrames = 0;

        svsPtr->snapFlagServerBit ^= 4;
    }

    Dvar_SetString(*(dvar_t **)&nextmap, "map_restart");

    Dvar_SetInt(*(dvar_t **)imp_cl_paused, 0);

    {
        const char *ext = GetBspExtension();
        Com_sprintf(filename, 64, "maps/mp/%s.%s", server, ext);
    }
    {
        extern void DBG_Hunk_PrintUsage(const char *);
        DBG_Hunk_PrintUsage("SV: before CM_LoadMap");
    }
    Com_LoadBsp(filename);
    CM_LoadMap(filename, &checksum);
    Com_UnloadBsp();
    CM_LinkWorld();
    {
        extern void DBG_Hunk_PrintUsage(const char *);
        DBG_Hunk_PrintUsage("SV: after CM_LoadMap");
    }

    sv_serverId_value = (sv_serverId_value + 16) & 0xff;
    if (!(sv_serverId_value & 0xf0))
        sv_serverId_value += 16;
    Dvar_SetInt(*(dvar_t **)imp_sv_serverid, sv_serverId_value);

    sv = (byte *)imp_sv;
    *(int *)(sv + SV_CHECKSUM_OFF) = *(int *)imp_com_frameTime;
    *(int *)(sv + SV_STATE_OFF) = 1;

    {
        const char *ext = GetBspExtension();
        Com_sprintf(filename, 64, "maps/mp/%s.%s", server, ext);
    }
    Com_LoadSoundAliases(filename, "all_mp", 2);

    {
        extern void DBG_Hunk_PrintUsage(const char *);
        DBG_Hunk_PrintUsage("SV: before SV_InitGameProgs");
    }
    SV_InitGameProgs(savepersist);

    isDedicated = (*(const dvar_t **)imp_com_dedicated)->current.integer;
    if (isDedicated) {
        FX_InitSystem(0);
        FX_CreateDefaultEffect();
    }

    Com_Printf("[ckpt] SV_RunFrame loop start\n");
    for (i = 0; i < 3; i++) {
        svsg->time += 100;
        SV_RunFrame();
        Com_Printf("[ckpt] SV_RunFrame %d done\n", i);
    }
    Com_Printf("[ckpt] SV_RunFrame loop done\n");

    sv = (byte *)imp_sv;
    if (*(int *)(sv + SV_NUMENTITIES_OFF) > 1) {
        byte *basePtr = sv;
        for (i = 1; i < *(int *)((byte *)imp_sv + SV_NUMENTITIES_OFF); i++) {
            int entOff;
            byte *dest;

            {
                gentity_t *gent = (gentity_t *)SV_GentityNum(i);
                if (gent->r.linked == 0) {
                    sv = (byte *)imp_sv;
                    goto next_baseline;
                }

                gent->s.number = i;

                memcpy(((char *)basePtr + offsetof(server_t, svEntities[1].baseline.s.number)), &gent->s, sizeof(entityState_t));
                ((server_t *)basePtr)->svEntities[1].baseline.r.svFlags = (unsigned char)gent->r.svFlags;
                ((server_t *)basePtr)->svEntities[1].baseline.r.clientMask[0] = gent->r.clientMask[0];
                ((server_t *)basePtr)->svEntities[1].baseline.r.clientMask[1] = gent->r.clientMask[1];

                entOff = i * 0x174;
                sv = (byte *)imp_sv;
                dest = sv + entOff + 0x2500 + 0x18;
                *(float *)dest = gent->r.absmin[0];
                *(int *)(dest + 4) = *(int *)&gent->r.absmin[1];
                *(int *)(dest + 8) = *(int *)&gent->r.absmin[2];

                dest = sv + entOff + 0x2510 + 0x14;
                *(int *)dest = *(int *)&gent->r.absmax[0];
                *(int *)(dest + 4) = *(int *)&gent->r.absmax[1];
                *(int *)(dest + 8) = *(int *)&gent->r.absmax[2];
            }
        next_baseline:
            basePtr += 0x174;
        }
    }

    Com_Printf("[ckpt] baseline done\n");
    {
        serverStatic_t *svsPtr = (serverStatic_t *)imp_svs;
        maxclients = (*(dvar_t **)imp_sv_maxclients)->current.integer;
        for (i = 0; i < maxclients; i++) {
            if (svsPtr->clients[i].state <= 1)
                continue;

            s = ClientConnect(i, svsPtr->clients[i].scriptId);
            if (s) {
                SV_DropClient(&svsPtr->clients[i], s);
            } else {
                svsPtr->clients[i].state = 2;
            }
        }
    }

    if ((*(dvar_t **)imp_sv_pure)->current.enabled) {
        s = FS_LoadedIwdChecksums();
        Dvar_SetString(*(dvar_t **)imp_sv_iwds, s);
        if (*s == '\0') {
            Com_Printf("WARNING: sv_pure set but no IWD files loaded\n");
        }
        s = FS_LoadedIwdNames();
        Dvar_SetString(*(dvar_t **)imp_sv_iwdNames, s);
    } else {
        Dvar_SetString(*(dvar_t **)imp_sv_iwds, "");
        Dvar_SetString(*(dvar_t **)imp_sv_iwdNames, "");
    }

    s = FS_ReferencedIwdChecksums();
    Dvar_SetString(*(dvar_t **)imp_sv_referencedIwds, s);
    s = FS_ReferencedIwdNames();
    Dvar_SetString(*(dvar_t **)imp_sv_referencedIwdNames, s);

    {
        char *info = Dvar_InfoString_Big(8);
        I_strncpyz(systemInfo, info, 0x2000);
        *(int *)imp_dvar_modifiedFlags &= ~8;
        SV_SetConfigstring(1, systemInfo);
        info = Dvar_InfoString(0x404);
        SV_SetConfigstring(0, info);
        *(int *)imp_dvar_modifiedFlags &= ~0x404;
        SV_SetConfig(0x8e, 0x60, 0x100);
        *(int *)imp_dvar_modifiedFlags &= ~0x100;
    }

    svg->state = 2;
    SV_Heartbeat_f();

    Com_Printf("-----------------------------------\n");
}
