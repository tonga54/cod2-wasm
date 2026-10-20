#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include <string.h>
extern serverStatic_t svs;
extern const dvar_t *com_sv_running;
extern const dvar_t *sv_maxclients;

#ifdef __EMSCRIPTEN__
#    include <stdlib.h>
#endif

extern const char *FS_GetMapBaseName(const char *mapname);
extern void Com_Printf(const char *fmt, ...);
extern char *Dvar_InfoString(int bit);
extern void Info_Print(const char *s);
extern void Com_Shutdown(const char *finalmsg);
extern void SV_MasterGameCompleteStatus(void);
extern void Scr_DumpScriptThreads(void);
extern void MT_DumpTree(void);
extern void Cmd_AddCommand(const char *cmd_name, void (*function)(void));
extern void Cmd_RemoveCommand(const char *cmd_name);
extern void Cmd_SetAutoComplete(const char *cmd_name, const char *dir, const char *ext);
extern int SV_Cmd_Argc(void);
extern const char *SV_Cmd_Argv(int arg);
extern char *Cmd_Args(int start);
extern void SV_BanClient(client_t *cl);
extern void SV_UnbanClient(const char *name);
extern int I_stricmp(const char *s0, const char *s1);
extern int stricmp(const char *s1, const char *s2);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern char *I_CleanStr(char *string);
extern char *I_strlwr(char *s);
extern void I_strncat(char *dest, int size, const char *src);
extern int I_DrawStrlen(const char *str);
extern const char *NET_AdrToString(netadr_t a);
extern const char *GetBspExtension(void);
extern int FS_ReadFile(const char *qpath, void **buffer);
extern void FS_ConvertPath(char *s);
extern const char *va(const char *fmt, ...);
extern unsigned char Com_HasPlayerProfile(void);
extern void Dvar_SetInt(const dvar_t *dvar, int value);
extern void Dvar_SetBool(const dvar_t *dvar, int value);
extern void Dvar_SetString(const dvar_t *dvar, const char *value);
extern const char *Dvar_GetString(const char *dvarName);
extern const char *Com_Parse(const char **data_p);
extern void Cbuf_ExecuteText(int exec_when, const char *text);
extern void SV_SetGametype(void);
extern void SV_InitDvar(void);
extern void SV_InitArchivedSnapshot(void);
extern void SV_SpawnServer(const char *server);
extern void SV_RestartGameProgs(qboolean savepersist);
extern void SV_RunFrame(void);
extern void SV_DropClient(client_t *drop, const char *reason);
extern void SV_AddServerCommand(client_t *client, svscmd_type type, const char *cmd);
extern void SV_SendServerCommand(client_t *cl, svscmd_type type, const char *fmt, ...);
extern playerState_t *SV_GameClientNum(int num);
extern int G_GetClientScore(int clientNum);
extern int G_GetSavePersist(void);
extern void G_SetSavePersist(qboolean savepersist);
extern const char *ClientConnect(int clientNum, int scriptPersId);
extern void SV_ClientEnterWorld(client_t *client, const dvar_t *(*cmd)[4]);
extern void SV_BanGuidBriefly(int guid);
extern int atoi(const char *s);

static dvar_t *SV_ImportedDvar(void *importedDvar)
{
    return *(dvar_t **)importedDvar;
}

static const char *SV_ConsumeMapRotationToken(void)
{
    dvar_t *rotationCurrent = SV_ImportedDvar(imp_sv_mapRotationCurrent);
    const char *parse = rotationCurrent->current.string;
    const char *token = Com_Parse(&parse);

    Dvar_SetString(rotationCurrent, parse ? parse : "");
    return token;
}

static inline __attribute__((always_inline)) qboolean SV_IsHostClient(client_t *cl)
{
    return cl->netchan.remoteAddress.type == NA_LOOPBACK;
}

static inline __attribute__((always_inline)) int SV_KickClientInternal(client_t *cl, char *playerName, int maxPlayerNameLen)
{
    int guid;

    if (SV_IsHostClient(cl)) {
        SV_SendServerCommand(NULL, 0, "%c \"EXE_CANNOTKICKHOSTPLAYER\"", 0x65);
        return 0;
    }

    if (playerName) {
        I_strncpyz(playerName, cl->name, maxPlayerNameLen);
        I_CleanStr(playerName);
    }

    guid = cl->guid;
    SV_DropClient(cl, "EXE_PLAYERKICKED");
    cl->lastPacketTime = svs.time;
    return guid;
}

static qboolean initialized;

static client_t *SV_GetPlayerByName(void);
static client_t *SV_GetPlayerByNum(void);
const char *SV_GetMapBaseName(const char *mapname);
static void __attribute_regparm__(1) SV_MapRestart(qboolean fast_restart);
static void SV_MapRestart_f(void);
static void SV_FastRestart_f(void);
static int __attribute_regparm__(2) SV_KickClient_f(char *playerName, int maxPlayerNameLen);
static void SV_Ban_f(void);
static void SV_BanNum_f(void);
static void SV_Unban_f(void);
static void SV_DropNum_f(void);
static void SV_TempBanNum_f(void);
static void SV_Status_f(void);
static void SV_ConSay_f(void);
static void SV_ConTell_f(void);
void SV_Heartbeat_f(void);
static void SV_Serverinfo_f(void);
static void SV_Systeminfo_f(void);
static void SV_DumpUser_f(void);
static void SV_KillServer_f(void);
void SV_GameCompleteStatus_f(void);
static void SV_ScriptUsage_f(void);
static void SV_StringUsage_f(void);
void SV_AddDedicatedCommands(void);
void SV_RemoveOperatorCommands(void);
static void SV_Map_f(void);
void SV_AddOperatorCommands(void);
static int __attribute_regparm__(2) SV_KickUser_f(char *playerName, int maxPlayerNameLen);
static void SV_TempBan_f(void);
static void SV_Drop_f(void);
static void SV_MapRotate_f(void);

static client_t *SV_GetPlayerByName(void)
{
    const char *s;
    client_t *cl;
    int i;
    char cleanName[64];

    if (!com_sv_running->current.enabled)
        return 0;

    if (SV_Cmd_Argc() - 1 <= 0) {
        Com_Printf("No player specified.\n");
        return 0;
    }

    s = SV_Cmd_Argv(1);
    cl = svs.clients;

    for (i = 0; i < sv_maxclients->current.integer; i++, cl++) {
        if (!cl->state)
            continue;

        if (I_stricmp(cl->name, s) == 0)
            return cl;

        I_strncpyz(cleanName, cl->name, 64);
        I_CleanStr(cleanName);
        if (I_stricmp(cleanName, s) == 0)
            return cl;
    }

    Com_Printf("Player %s is not on the server\n", s);
    return 0;
}

static client_t *SV_GetPlayerByNum(void)
{
    const char *s;
    const char *p;
    int idnum;

    if (!(*(dvar_t **)imp_com_sv_running)->current.enabled) {
        return NULL;
    }

    if (SV_Cmd_Argc() - 1 <= 0) {
        Com_Printf("No player specified.\n");
        return NULL;
    }

    s = SV_Cmd_Argv(1);
    for (p = s; *p; ++p) {
        if (*p < '0' || *p > '9') {
            Com_Printf("Bad slot number: %s\n", s);
            return NULL;
        }
    }

    idnum = atoi(s);
    if (idnum < 0 || idnum >= (*(dvar_t **)imp_sv_maxclients)->current.integer) {
        Com_Printf("Bad client slot: %i\n", idnum);
        return NULL;
    }

    if (!svs.clients[idnum].state) {
        Com_Printf("Client %i is not active\n", idnum);
        return NULL;
    }

    return &svs.clients[idnum];
}

const char *SV_GetMapBaseName(const char *mapname)
{
    return FS_GetMapBaseName(mapname);
}

static void __attribute_regparm__(1) SV_MapRestart(qboolean fast_restart)
{
    server_t *sv = (server_t *)imp_sv;
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    dvar_t *svGametype;
    dvar_t *svMaxclients;
    int savepersist;

    if (!SV_ImportedDvar(imp_com_sv_running)->current.enabled) {
        Com_Printf("Server is not running.\n");
        return;
    }

    SV_SetGametype();

    svGametype = SV_ImportedDvar(imp_sv_gametype);
    I_strncpyz(sv->gametype, svGametype->current.string, sizeof(sv->gametype));

    savepersist = G_GetSavePersist();
    svMaxclients = SV_ImportedDvar(imp_sv_maxclients);

    if (svMaxclients->modified || stricmp(svGametype->current.string, sv->gametype) || !fast_restart) {
        char mapname[64];

        G_SetSavePersist(0);
        I_strncpyz(mapname, Dvar_GetString("mapname"), sizeof(mapname));
        FS_ConvertPath(mapname);
        SV_SpawnServer(mapname);
        return;
    }

    if (sv->start_frameTime == *(int *)imp_com_frameTime)
        return;

    SV_InitDvar();
    SV_InitArchivedSnapshot();

    svs->snapFlagServerBit ^= 4;
    {
        int serverId = *(int *)imp_sv_serverId_value;
        serverId = (serverId & 0xf0) + ((serverId + 1) & 0x0f);
        *(int *)imp_sv_serverId_value = serverId;
        Dvar_SetInt(SV_ImportedDvar(imp_sv_serverid), serverId);
    }

    sv->start_frameTime = *(int *)imp_com_frameTime;
    sv->state = SS_LOADING;
    sv->restarting = 1;

    SV_RestartGameProgs(savepersist);

    for (int i = 0; i < 3; ++i) {
        svs->time += 100;
        SV_RunFrame();
    }

    for (int i = 0; i < SV_ImportedDvar(imp_sv_maxclients)->current.integer; ++i) {
        client_t *client = &svs->clients[i];
        const char *denied;

        if (client->state <= 1)
            continue;

        SV_AddServerCommand(client, 1, va("%c", savepersist < 1 ? 0x42 : 0x6e));
        denied = ClientConnect(i, client->scriptId);
        if (denied) {
            SV_DropClient(client, denied);
            Com_Printf("SV_MapRestart_f: dropped client %i - denied!\n", i);
            continue;
        }

        if (client->state == 4)
            SV_ClientEnterWorld(client, (const dvar_t *(*)[4]) & client->lastUsercmd);
    }

    sv->state = SS_GAME;
    sv->restarting = 0;
}

static void SV_MapRestart_f(void)
{
    SV_MapRestart(0);
}

static void SV_FastRestart_f(void)
{
    SV_MapRestart(1);
}

static int __attribute_regparm__(2) SV_KickClient_f(char *playerName, int maxPlayerNameLen)
{
    client_t *cl;

    if (!com_sv_running->current.enabled) {
        Com_Printf("Server is not running.\n");
        return 0;
    }

    if (SV_Cmd_Argc() != 2) {
        Com_Printf("Usage: %s <client number>\n", SV_Cmd_Argv(0));
        return 0;
    }

    cl = SV_GetPlayerByNum();
    if (!cl) {
        return 0;
    }

    return SV_KickClientInternal(cl, playerName, maxPlayerNameLen);
}

static void SV_Ban_f(void)
{
    void *cl;
    if (!com_sv_running->current.enabled) {
        Com_Printf("Server is not running.\n");
        return;
    }
    if (SV_Cmd_Argc() != 2) {
        Com_Printf("Usage: banUser <player name>\n");
        return;
    }
    cl = (void *)SV_GetPlayerByName();
    if (cl)
        SV_BanClient((client_t *)cl);
}

static void SV_BanNum_f(void)
{
    void *cl;
    if (!com_sv_running->current.enabled) {
        Com_Printf("Server is not running.\n");
        return;
    }
    if (SV_Cmd_Argc() != 2) {
        Com_Printf("Usage: banClient <client number>\n");
        return;
    }
    cl = (void *)SV_GetPlayerByNum();
    if (cl)
        SV_BanClient((client_t *)cl);
}

static void SV_Unban_f(void)
{
    if (SV_Cmd_Argc() != 2) {
        Com_Printf("Usage: unban <client name>\n");
        return;
    }
    SV_UnbanClient(SV_Cmd_Argv(1));
}

static void SV_DropNum_f(void)
{
    SV_KickClient_f(NULL, 0);
}

static void SV_TempBanNum_f(void)
{
    char playerName[64];
    int guid;

    guid = SV_KickClient_f(playerName, sizeof(playerName));
    if (guid) {
        Com_Printf("%s (guid %i) was kicked for cheating\n", playerName, guid);
        SV_BanGuidBriefly(guid);
    }
}

static void SV_Status_f(void)
{
    serverStatic_t *svsPtr;
    client_t *cl;
    const char *s;
    int i;
    int l;
    int maxClients;
    int ping;

    if (!(*(dvar_t **)imp_com_sv_running)->current.enabled) {
        Com_Printf("Server is not running.\n");
        return;
    }

    Com_Printf("map: %s\n", (*(dvar_t **)imp_sv_mapname)->current.string);
    Com_Printf("num score ping guid   name            lastmsg address               qport rate\n");
    Com_Printf("--- ----- ---- ------ --------------- ------- --------------------- ----- -----\n");

    svsPtr = (serverStatic_t *)imp_svs;
    maxClients = (*(dvar_t **)imp_sv_maxclients)->current.integer;
    cl = svsPtr->clients;

    for (i = 0; i < maxClients; ++i, ++cl) {
        if (!cl->state)
            continue;

        Com_Printf("%3i ", i);
        SV_GameClientNum(i);
        Com_Printf("%5i ", G_GetClientScore(i));

        if (cl->state == 2) {
            Com_Printf("CNCT ");
        } else if (cl->state == 1) {
            Com_Printf("ZMBI ");
        } else {
            ping = cl->ping;
            if (ping >= 10000)
                ping = 9999;
            Com_Printf("%4i ", ping);
        }

        Com_Printf("%6i ", cl->guid);
        Com_Printf("%s^7", cl->name);

        l = 16 - I_DrawStrlen(cl->name);
        while (l-- > 0)
            Com_Printf(" ");

        Com_Printf("%7i ", svsPtr->time - cl->lastPacketTime);

        s = NET_AdrToString(cl->netchan.remoteAddress);
        Com_Printf("%s", s);

        l = 22 - (int)strlen(s);
        while (l-- > 0)
            Com_Printf(" ");

        Com_Printf("%5i", cl->netchan.qport);
        Com_Printf(" %5i", cl->rate);
        Com_Printf("\n");
    }

    Com_Printf("\n");
}

static void SV_ConSay_f(void)
{
    char text[1024];
    char *args;

    if (!com_sv_running->current.enabled) {
        Com_Printf("Server is not running.\n");
        return;
    }

    if (SV_Cmd_Argc() - 1 <= 0)
        return;

    memcpy(text, "console: ", 10);
    args = Cmd_Args(1);
    if (*args == '\"') {
        ++args;
        args[strlen(args) - 1] = '\0';
    }

    I_strncat(text, sizeof(text), args);
    SV_SendServerCommand(NULL, 0, "%c \"\x15%s\"", 0x68, text);
}

static void SV_ConTell_f(void)
{
    int clientNum;
    client_t *cl;
    char text[1024];
    char *args;

    if (!com_sv_running->current.enabled) {
        Com_Printf("Server is not running.\n");
        return;
    }

    if (SV_Cmd_Argc() <= 2)
        return;

    clientNum = atoi(SV_Cmd_Argv(1));
    if (clientNum < 0 || clientNum >= sv_maxclients->current.integer)
        return;

    cl = svs.clients + clientNum;
    if (cl->state != 4)
        return;

    memcpy(text, "console: ", 10);
    args = Cmd_Args(2);
    if (*args == '\"') {
        ++args;
        args[strlen(args) - 1] = '\0';
    }

    I_strncat(text, sizeof(text), args);
    SV_SendServerCommand(cl, 0, "%c \"\x15%s\"", 0x68, text);
}

void SV_Heartbeat_f(void)
{
    svs.nextHeartbeatTime = (int)0x80000000;
}

static void SV_Serverinfo_f(void)
{
    Com_Printf("Server info settings:\n");
    Info_Print(Dvar_InfoString(0x404));
}

static void SV_Systeminfo_f(void)
{
    Com_Printf("System info settings:\n");
    Info_Print(Dvar_InfoString(8));
}

static void SV_DumpUser_f(void)
{
    byte *cl;
    if (!com_sv_running->current.enabled) {
        Com_Printf("Server is not running.\n");
        return;
    }
    if (SV_Cmd_Argc() != 2) {
        Com_Printf("Usage: dumpuser <player name>\n");
        return;
    }
    cl = (byte *)SV_GetPlayerByName();
    if (!cl)
        return;
    Com_Printf("userinfo\n");
    Com_Printf("--------\n");
    Info_Print(((client_t *)cl)->userinfo);
}

static void SV_KillServer_f(void)
{
    Com_Shutdown("EXE_SERVERKILLED");
}

void SV_GameCompleteStatus_f(void)
{
    SV_MasterGameCompleteStatus();
}

static void SV_ScriptUsage_f(void)
{
    Scr_DumpScriptThreads();
}

static void SV_StringUsage_f(void)
{
    MT_DumpTree();
}

void SV_AddDedicatedCommands(void)
{
    Cmd_AddCommand("say", (void (*)(void))SV_ConSay_f);
    Cmd_AddCommand("tell", (void (*)(void))SV_ConTell_f);
}

void SV_RemoveOperatorCommands(void)
{
    if (!initialized)
        return;

    initialized = 0;
    Cmd_RemoveCommand("heartbeat");
    Cmd_RemoveCommand("onlykick");
    Cmd_RemoveCommand("banUser");
    Cmd_RemoveCommand("banClient");
    Cmd_RemoveCommand("kick");
    Cmd_RemoveCommand("tempBanUser");
    Cmd_RemoveCommand("tempBanClient");
    Cmd_RemoveCommand("unbanUser");
    Cmd_RemoveCommand("clientkick");
    Cmd_RemoveCommand("botkick");
    Cmd_RemoveCommand("status");
    Cmd_RemoveCommand("serverinfo");
    Cmd_RemoveCommand("systeminfo");
    Cmd_RemoveCommand("dumpuser");
    Cmd_RemoveCommand("map_restart");
    Cmd_RemoveCommand("fast_restart");
    Cmd_RemoveCommand("map");
    Cmd_RemoveCommand("map_rotate");
    Cmd_RemoveCommand("gameCompleteStatus");
    Cmd_RemoveCommand("devmap");
    Cmd_RemoveCommand("killserver");
    Cmd_RemoveCommand("say");
    Cmd_RemoveCommand("tell");
    Cmd_RemoveCommand("scriptUsage");
    Cmd_RemoveCommand("stringUsage");
}

/* Drops the test client browser_bots.gsc's governor chose (self botRemove()).
 * The command runs after the script frame, so a slot a human has taken since
 * is left alone. */
static void SV_BotKick_f(void)
{
    client_t *cl = SV_GetPlayerByNum();

    if (cl && cl->state && cl->bIsTestClient && cl->netchan.remoteAddress.type == NA_BOT)
        SV_DropClient(cl, "EXE_DISCONNECTED");
}

static void SV_Map_f(void)
{
    char mapname[64];
    const char *expanded;
    const char *map;
    int isDevmap;

    map = SV_Cmd_Argv(1);
    if (!map[0])
        return;

    if (!Com_HasPlayerProfile()) {
        Com_Printf("Can't load a map without a player profile selected.\n");
        return;
    }

    I_strncpyz(mapname, FS_GetMapBaseName(map), sizeof(mapname));
    I_strlwr(mapname);

    expanded = va("maps/mp/%s.%s", mapname, GetBspExtension());
    if (FS_ReadFile(expanded, NULL) == -1) {
        Com_Printf("Can't find map %s\n", expanded);
        return;
    }

    isDevmap = I_stricmp(SV_Cmd_Argv(0), "devmap");
    FS_ConvertPath(mapname);
    SV_SpawnServer(mapname);

    Dvar_SetBool(*(const dvar_t **)imp_sv_cheats, isDevmap == 0);
}

void SV_AddOperatorCommands(void)
{
    if (initialized)
        return;
    initialized = 1;
    Cmd_AddCommand("heartbeat", (void (*)(void))SV_Heartbeat_f);
    Cmd_AddCommand("onlykick", (void (*)(void))SV_Drop_f);
    Cmd_AddCommand("banUser", (void (*)(void))SV_Ban_f);
    Cmd_AddCommand("banClient", (void (*)(void))SV_BanNum_f);
    Cmd_AddCommand("kick", (void (*)(void))SV_TempBan_f);
    Cmd_AddCommand("tempBanUser", (void (*)(void))SV_TempBan_f);
    Cmd_AddCommand("tempBanClient", (void (*)(void))SV_TempBanNum_f);
    Cmd_AddCommand("unbanUser", (void (*)(void))SV_Unban_f);
    Cmd_AddCommand("clientkick", (void (*)(void))SV_DropNum_f);
    Cmd_AddCommand("botkick", (void (*)(void))SV_BotKick_f);
    Cmd_AddCommand("status", (void (*)(void))SV_Status_f);
    Cmd_AddCommand("serverinfo", (void (*)(void))SV_Serverinfo_f);
    Cmd_AddCommand("systeminfo", (void (*)(void))SV_Systeminfo_f);
    Cmd_AddCommand("dumpuser", (void (*)(void))SV_DumpUser_f);
    Cmd_AddCommand("map_restart", (void (*)(void))SV_MapRestart_f);
    Cmd_AddCommand("fast_restart", (void (*)(void))SV_FastRestart_f);
    Cmd_AddCommand("map", (void (*)(void))SV_Map_f);
    Cmd_SetAutoComplete("map", "maps/mp", "d3dbsp");
    Cmd_AddCommand("map_rotate", (void (*)(void))SV_MapRotate_f);
    Cmd_AddCommand("gameCompleteStatus", (void (*)(void))SV_GameCompleteStatus_f);
    Cmd_AddCommand("devmap", (void (*)(void))SV_Map_f);
    Cmd_SetAutoComplete("devmap", "maps/mp", "d3dbsp");
    Cmd_AddCommand("killserver", (void (*)(void))SV_KillServer_f);
    if (*(int *)((byte *)(*(void **)imp_com_dedicated) + 8)) {
        Cmd_AddCommand("say", (void (*)(void))SV_ConSay_f);
        Cmd_AddCommand("tell", (void (*)(void))SV_ConTell_f);
    }
    Cmd_AddCommand("scriptUsage", (void (*)(void))SV_ScriptUsage_f);
    Cmd_AddCommand("stringUsage", (void (*)(void))SV_StringUsage_f);
}

static int __attribute_regparm__(2) SV_KickUser_f(char *playerName, int maxPlayerNameLen)
{
    client_t *cl;

    if (!com_sv_running->current.enabled) {
        Com_Printf("Server is not running.\n");
        return 0;
    }

    if (SV_Cmd_Argc() != 2) {
        const char *cmd = SV_Cmd_Argv(0);
        Com_Printf("Usage: %s <player name>\n%s all = kick everyone\n", cmd, cmd);
        return 0;
    }

    cl = SV_GetPlayerByName();
    if (cl) {
        return SV_KickClientInternal(cl, playerName, maxPlayerNameLen);
    }

    if (!I_stricmp(SV_Cmd_Argv(1), "all")) {
        serverStatic_t *svs = (serverStatic_t *)imp_svs;

        cl = svs->clients;
        for (int i = 0; i < (*(dvar_t **)imp_sv_maxclients)->current.integer; ++i, ++cl) {
            if (!cl->state) {
                continue;
            }
            if (SV_IsHostClient(cl)) {
                SV_SendServerCommand(NULL, 0, "%c \"EXE_CANNOTKICKHOSTPLAYER\"", 0x65);
            } else {
                SV_DropClient(cl, "EXE_PLAYERKICKED");
                cl->lastPacketTime = svs->time;
            }
        }
    }

    return 0;
}

static void SV_TempBan_f(void)
{
    char playerName[64];
    int guid;

    guid = SV_KickUser_f(playerName, sizeof(playerName));
    if (guid) {
        Com_Printf("%s (guid %i) was kicked for cheating\n", playerName, guid);
        SV_BanGuidBriefly(guid);
    }
}

static void SV_Drop_f(void)
{
    SV_KickUser_f(NULL, 0);
}

static void SV_MapRotate_f(void)
{
    const char *token;
    dvar_t *mapRotation = SV_ImportedDvar(imp_sv_mapRotation);
    dvar_t *mapRotationCurrent = SV_ImportedDvar(imp_sv_mapRotationCurrent);

    Com_Printf("map_rotate...\n\n");
    Com_Printf("\"sv_mapRotation\" is:\"%s\"\n\n", mapRotation->current.string);
    Com_Printf("\"sv_mapRotationCurrent\" is:\"%s\"\n\n", mapRotationCurrent->current.string);

    if (!mapRotationCurrent->current.string[0]) {
        Dvar_SetString(mapRotationCurrent, mapRotation->current.string);
    }

    for (;;) {
        token = SV_ConsumeMapRotationToken();
        if (!token) {
            Dvar_SetString(mapRotationCurrent, mapRotation->current.string);
            continue;
        }

        if (!I_stricmp(token, "gametype")) {
            token = SV_ConsumeMapRotationToken();
            if (!token) {
                Com_Printf("No gametype specified after 'gametype' keyword in sv_mapRotation - forcing map_restart.\n");
                SV_MapRestart(1);
                return;
            }

            Com_Printf("Setting g_gametype: %s.\n", token);
            if (SV_ImportedDvar(imp_com_sv_running)->current.enabled &&
                I_stricmp(SV_ImportedDvar(imp_sv_gametype)->current.string, token)) {
                G_SetSavePersist(0);
            }
            Dvar_SetString(SV_ImportedDvar(imp_sv_gametype), token);
            continue;
        }

        if (!I_stricmp(token, "map")) {
            token = SV_ConsumeMapRotationToken();
            if (!token) {
                Com_Printf("No map specified after 'map' keyword in sv_mapRotation - forcing map_restart.\n");
                SV_MapRestart(1);
                return;
            }

            Com_Printf("Setting map: %s.\n", token);
            Cbuf_ExecuteText(0, va("map %s\n", token));
            return;
        }

        if (!*token) {
            Com_Printf("No map specified in sv_mapRotation - forcing map_restart.\n");
            SV_MapRestart(1);
            return;
        }

        Com_Printf("Unknown keyword '%s' in sv_mapRotation.\n", token);
    }
}
