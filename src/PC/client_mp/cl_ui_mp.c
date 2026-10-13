#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
extern clientStatic_t cls;

extern int I_stricmp(const char *s1, const char *s2);

extern qboolean CL_UpdateDirtyPings_f(int source);
extern int CL_ServerStatus(char *serverAddress, char *serverStatus, int maxLen);
extern int SND_PlayLocalSoundAlias(snd_alias_list_t *aliasList, int channel);
extern int SND_PlayLocalSoundAliasByName(const char *aliasname, int channel);
extern void Com_LoadSoundAliases(const char *zone, const char *spec, snd_alias_system_t flags);
extern qboolean UI_CheckExecKey(int key);
extern void CL_SwitchToLocalClient(int localClientNum);
extern void UI_Init(void);
extern void UI_Component_Init(void);
extern qboolean UI_IsFullscreen(void);
extern qboolean UI_SetActiveMenu(int menu);
extern const char *Key_KeynumToString(int keynum, int translate);
extern const char *Key_GetBinding(int keynum);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern const dvar_t *Dvar_RegisterString(const char *dvarName, const char *value, unsigned int flags);
extern void Com_WriteCDKey(void);
extern void Com_PumpMessageLoop(void);
extern void Info_SetValueForKey(char *s, const char *key, const char *value);
extern char *va(const char *format, ...);
extern const char *NET_AdrToString(netadr_t adr);
extern void Com_UnloadSoundAliases(int zone);
extern void UI_Shutdown(void);

void GetClientState(uiClientState_t *state);
void LAN_ResetPings(int source);
int LAN_GetServerCount(int source);
qboolean LAN_WaitServerResponse(int source);
void LAN_GetServerInfo(int source, int n, char *buf, int buflen);
int LAN_GetServerPing(int source, int n);
void LAN_MarkServerDirty(int source, int n, qboolean dirty);
int LAN_ServerIsDirty(int source, int n);
qboolean LAN_UpdateDirtyPings(int source);
int LAN_GetServerStatus(char *serverAddress, char *serverStatus, int maxLen);
void Key_KeynumToStringBuf(int keynum, char *buf, int buflen);
void Key_GetBindingBuf(int keynum, char *buf, int buflen);
int Key_GetCatcher(void);
void Key_SetCatcher(int catcher);
void CLUI_GetCDKey(char *buf, int buflen, char *buf2, int buf2len);
void CLUI_SetCDKey(char *buf, char *buf2);
int GetClientname(int index, char *buf, int size);
int UI_PlayLocalSoundAlias(snd_alias_list_t *aliasList);
int UI_PlayLocalSoundAliasByName(const char *aliasname);
qboolean UI_ClientIsInGame(void);
qboolean CL_ShutdownUI(void);
void CL_InitUI(void);
qboolean UI_checkKeyExec(int key);
void UI_LoadSoundAliases(void);
int LAN_CompareHostname(const char *hostName1, const char *hostName2);
int LAN_CompareServers(int source, int sortKey, int sortDir, int s1, int s2);

void GetClientState(uiClientState_t *state)
{
    clientConnection_t *clc_local = *(clientConnection_t **)imp_clc;

    state->connectPacketCount = clc_local->connectPacketCount;
    state->connState = clc_local->state;
    I_strncpyz(state->servername, cls.servername, 0x400);
    I_strncpyz(state->messageString, clc_local->serverMessage, 0x400);
    state->clientNum = (*(clientActive_t **)imp_cl)->snap.ps.clientNum;
}

void LAN_ResetPings(int source)
{
    serverInfo_t *servers;
    int count;

    switch (source) {
    case 0:
        count = 128;
        servers = cls.localServers;
        break;
    case 1:
        count = cls.numglobalservers;
        servers = cls.globalServers;
        if (count <= 0)
            return;
        break;
    case 2:
        count = 128;
        servers = cls.favoriteServers;
        break;
    default:
        return;
    }

    for (int i = 0; i < count; i++) {
        servers[i].ping = -1;
    }
}

int LAN_GetServerCount(int source)
{
    switch (source) {
    case 0:
        return cls.numlocalservers;
    case 1:
        return cls.numglobalservers;
    case 2:
        return cls.numfavoriteservers;
    }
    return 0;
}

qboolean LAN_WaitServerResponse(int source)
{
    if (source != 1)
        return 0;
    return cls.waitglobalserverresponse;
}

void LAN_GetServerInfo(int source, int n, char *buf, int buflen)
{
    char info[1024];
    serverInfo_t *server = NULL;

    info[0] = '\0';

    switch (source) {
    case 0:
        if ((unsigned)n > 0x7f)
            goto fail;
        server = &cls.localServers[n];
        break;
    case 1:
        if (n < 0 || n >= cls.numglobalservers)
            goto fail;
        server = &cls.globalServers[n];
        break;
    case 2:
        if ((unsigned)n > 0x7f)
            goto fail;
        server = &cls.favoriteServers[n];
        break;
    default:
        goto fail;
    }

    if (!server)
        goto fail;
    if (!buf)
        return;

    *buf = '\0';
    Info_SetValueForKey(info, "hostname", server->hostName);
    Info_SetValueForKey(info, "mapname", server->mapName);
    Info_SetValueForKey(info, "clients", va("%i", (int)server->clients));
    Info_SetValueForKey(info, "sv_maxclients", va("%i", (int)server->maxClients));
    Info_SetValueForKey(info, "ping", va("%i", (int)server->ping));
    Info_SetValueForKey(info, "minping", va("%i", (int)server->minPing));
    Info_SetValueForKey(info, "maxping", va("%i", (int)server->maxPing));
    Info_SetValueForKey(info, "game", server->game);
    Info_SetValueForKey(info, "gametype", server->gameType);
    Info_SetValueForKey(info, "nettype", va("%i", (int)server->netType));
    {
        netadr_t adr;
        memcpy(&adr, &server->adr, sizeof(netadr_t));
        Info_SetValueForKey(info, "addr", NET_AdrToString(adr));
    }
    Info_SetValueForKey(info, "sv_allowAnonymous", va("%i", (int)server->allowAnonymous));
    Info_SetValueForKey(info, "con_disabled", va("%i", (int)server->consoleDisabled));
    Info_SetValueForKey(info, "pswrd", va("%i", (int)server->bPassword));
    Info_SetValueForKey(info, "pure", va("%i", (int)server->pure));
    Info_SetValueForKey(info, "ff", va("%i", (int)server->friendlyfire));
    Info_SetValueForKey(info, "kc", va("%i", (int)server->killcam));
    Info_SetValueForKey(info, "hw", va("%i", (int)server->hardware));
    Info_SetValueForKey(info, "mod", va("%i", (int)server->mod));
    Info_SetValueForKey(info, "voice", va("%i", (int)server->voice));
    I_strncpyz(buf, info, buflen);
    return;

fail:
    if (buf)
        *buf = '\0';
}

int LAN_GetServerPing(int source, int n)
{
    serverInfo_t *server;

    switch (source) {
    case 0:
        if ((unsigned)n > 0x7f)
            return -1;
        server = &cls.localServers[n];
        break;
    case 1:
        if (n < 0 || n >= cls.numglobalservers)
            return -1;
        server = &cls.globalServers[n];
        break;
    case 2:
        if ((unsigned)n > 0x7f)
            return -1;
        server = &cls.favoriteServers[n];
        break;
    default:
        return -1;
    }

    if (!server)
        return -1;
    return server->ping;
}

void LAN_MarkServerDirty(int source, int n, qboolean dirty)
{
    serverInfo_t *servers;
    int count;
    int i;

    Com_PumpMessageLoop();

    if (n == -1) {

        switch (source) {
        case 0:
            count = 0x80;
            servers = cls.localServers;
            break;
        case 1:
            count = cls.numglobalservers;
            servers = cls.globalServers;
            if (count <= 0)
                return;
            break;
        case 2:
            count = 0x80;
            servers = cls.favoriteServers;
            break;
        default:
            return;
        }
        for (i = 0; i < count; i++) {
            servers[i].dirty = (byte)dirty;
        }
    } else {
        switch (source) {
        case 0:
            if ((unsigned)n > 0x7f)
                return;
            cls.localServers[n].dirty = (byte)dirty;
            break;
        case 1:
            if (n < 0 || n >= cls.numglobalservers)
                return;
            cls.globalServers[n].dirty = (byte)dirty;
            break;
        case 2:
            if ((unsigned)n > 0x7f)
                return;
            cls.favoriteServers[n].dirty = (byte)dirty;
            break;
        }
    }
}

int LAN_ServerIsDirty(int source, int n)
{
    switch (source) {
    case 0:
        if ((unsigned)n > 0x7f)
            return 0;
        return cls.localServers[n].dirty;
    case 1:
        if (n < 0 || n >= cls.numglobalservers)
            return 0;
        return cls.globalServers[n].dirty;
    case 2:
        if ((unsigned)n > 0x7f)
            return 0;
        return cls.favoriteServers[n].dirty;
    }
    return 0;
}

qboolean LAN_UpdateDirtyPings(int source)
{
    return CL_UpdateDirtyPings_f(source);
}

int LAN_GetServerStatus(char *serverAddress, char *serverStatus, int maxLen)
{
    return CL_ServerStatus(serverAddress, serverStatus, maxLen);
}

void Key_KeynumToStringBuf(int keynum, char *buf, int buflen)
{
    I_strncpyz(buf, Key_KeynumToString(keynum, 1), buflen);
}

void Key_GetBindingBuf(int keynum, char *buf, int buflen)
{
    const char *binding = Key_GetBinding(keynum);
    if (binding) {
        I_strncpyz(buf, binding, buflen);
        return;
    }
    *buf = 0;
}

int Key_GetCatcher(void)
{
    return (*(clientActive_t **)imp_cl)->keyCatchers;
}

void Key_SetCatcher(int catcher)
{
    clientActive_t **pp = (clientActive_t **)imp_cl;
    clientActive_t *ptr = *pp;
    ptr->keyCatchers = catcher & ~1;
    ptr = *pp;
    if (!(ptr->keyCatchers & 8))
        ptr->displayHUDWithKeycatchUI = 0;
}

void CLUI_GetCDKey(char *buf, int buflen, char *buf2, int buf2len)
{
    char *cdkey;
    char *cdkey2;

    Dvar_RegisterString((const char *)"fs_game", (const char *)"", 0x101c);

    cdkey = (char *)imp_cl_cdkey;
    memcpy(buf, cdkey, 16);
    buf[16] = '\0';

    cdkey2 = (char *)imp_cl_cdkeychecksum;
    memcpy(buf2, cdkey2, 4);
    buf2[4] = '\0';
}

void CLUI_SetCDKey(char *buf, char *buf2)
{
    char *cdkey = (char *)imp_cl_cdkey;
    char *cdkey2;

    memcpy(cdkey, buf, 16);
    cdkey[16] = '\0';

    cdkey2 = (char *)imp_cl_cdkeychecksum;
    memcpy(cdkey2, buf2, 4);
    cdkey2[4] = '\0';

    Com_WriteCDKey();
}

int GetClientname(int index, char *buf, int size)
{
    clientActive_t *cl;
    int count;
    int start;
    int i;

    *buf = '\0';

    cl = *(clientActive_t **)imp_cl;
    if (!cl->snap.valid)
        return 0;

    count = *(int *)((char *)cl + 0x26e0);
    if (count <= 0)
        return 0;

    start = *(int *)((char *)cl + 0x26e8);

    for (i = 0; i < count; i++) {
        int slot = (start + i) & 0x7ff;
        clientState_t *entry = &cl->parseClients[slot];

        if (entry->clientIndex == index) {
            strncpy(buf, entry->name, size);
            return 1;
        }
    }

    return 0;
}

int UI_PlayLocalSoundAlias(snd_alias_list_t *aliasList)
{
    return SND_PlayLocalSoundAlias(aliasList, 0);
}

int UI_PlayLocalSoundAliasByName(const char *aliasname)
{
    return SND_PlayLocalSoundAliasByName(aliasname, 0);
}

qboolean UI_ClientIsInGame(void)
{
    clientConnection_t *clc_local = *(clientConnection_t **)imp_clc;
    return clc_local->state == 8;
}

qboolean CL_ShutdownUI(void)
{
    clientStatic_t *cls_local = &cls;
    clientActive_t *cl;
    int started = cls_local->uiStarted;

    if (!started)
        return 0;

    Com_UnloadSoundAliases(0);

    cl = *(clientActive_t **)imp_cl;
    cl->keyCatchers &= ~8;
    cl->displayHUDWithKeycatchUI = 0;

    CL_SwitchToLocalClient(0);
    UI_Shutdown();
    CL_SwitchToLocalClient(0);

    cls_local->uiStarted = 0;
    return 1;
}

void CL_InitUI(void)
{
    extern void Com_Printf(const char *, ...);
    extern int g_disableRendering;
    cls.uiStarted = 1;
    CL_SwitchToLocalClient(0);
    Com_Printf("[disrend] CL_InitUI before UI_Init = %d\n", g_disableRendering);
    UI_Init();
    Com_Printf("[disrend] CL_InitUI after UI_Init = %d\n", g_disableRendering);
    CL_SwitchToLocalClient(0);
    UI_Component_Init();
}

qboolean UI_checkKeyExec(int key)
{
    int started = cls.uiStarted;
    if (!started)
        return 0;
    return UI_CheckExecKey(key);
}

void UI_LoadSoundAliases(void)
{
    Com_LoadSoundAliases("menu", "all_mp", 0);
}

int LAN_CompareHostname(const char *hostName1, const char *hostName2)
{

    char cleanHostName1[40];
    char cleanHostName2[40];
    char *dst;
    const char *src;
    int c, result;

    dst = cleanHostName1;
    src = hostName1;
    while ((c = (unsigned char)*src) != 0) {
        if (isalpha(c))
            *dst++ = (char)c;
        src++;
    }
    *dst = '\0';

    dst = cleanHostName2;
    src = hostName2;
    while ((c = (unsigned char)*src) != 0) {
        if (isalpha(c))
            *dst++ = (char)c;
        src++;
    }
    *dst = '\0';

    result = I_stricmp(cleanHostName1, cleanHostName2);
    if (result != 0)
        return result;
    return I_stricmp(hostName1, hostName2);
}

int LAN_CompareServers(int source, int sortKey, int sortDir, int s1, int s2)
{

    clientStatic_t *cls_ptr = &cls;
    serverInfo_t *server1 = (serverInfo_t *)0;
    serverInfo_t *server2 = (serverInfo_t *)0;
    int diff;

    if (source == 1) {

        if (s1 >= 0 && s1 < cls_ptr->numglobalservers)
            server1 = &cls_ptr->globalServers[s1];
    } else if (source == 2) {

        if ((unsigned int)s1 <= 0x7f)
            server1 = &cls_ptr->favoriteServers[s1];
    } else {

        if ((unsigned int)s1 <= 0x7f)
            server1 = &cls_ptr->localServers[s1];
    }

    if (source == 1) {
        if (s2 >= 0 && s2 < cls_ptr->numglobalservers)
            server2 = &cls_ptr->globalServers[s2];
    } else if (source == 2) {
        if ((unsigned int)s2 <= 0x7f)
            server2 = &cls_ptr->favoriteServers[s2];
    } else {
        if ((unsigned int)s2 <= 0x7f)
            server2 = &cls_ptr->localServers[s2];
    }

    if (!server1 || !server2)
        return 0;

    diff = 0;
    switch (sortKey) {
    case 0:
        diff = (int)(unsigned char)server1->bPassword - (int)(unsigned char)server2->bPassword;
        if (diff == 0)
            goto compare_ping_nosortdir;
        break;
    case 1:
    {
        unsigned char hw1 = server1->hardware;
        unsigned char hw2 = server2->hardware;
        diff = (int)hw1 - (int)hw2;
        if (diff == 0)
            goto compare_ping_nosortdir;
        if (diff < 0) {

            if (!hw1)
                diff = 1;
        } else {

            if (!hw2)
                diff = -1;
        }
    } break;
    case 2:
        diff = LAN_CompareHostname(server1->hostName, server2->hostName);
        if (diff == 0)
            goto compare_ping_nosortdir;
        break;
    case 3:
        diff = I_stricmp(server1->mapName, server2->mapName);
        if (diff == 0)
            goto compare_ping_nosortdir;
        break;
    case 4:
        diff = (int)(unsigned char)server1->clients - (int)(unsigned char)server2->clients;
        if (diff == 0)
            goto compare_ping_nosortdir;
        break;
    case 5:
        diff = I_stricmp(server1->gameType, server2->gameType);
        if (diff != 0)
            break;
        goto compare_ping_nosortdir;
    case 6:
        diff = (int)(unsigned char)server1->voice - (int)(unsigned char)server2->voice;
        if (diff == 0)
            goto compare_ping_nosortdir;
        break;
    case 7:
        diff = (int)(unsigned char)server1->pure - (int)(unsigned char)server2->pure;
        if (diff == 0)
            goto compare_ping_nosortdir;
        break;
    case 8:
        diff = (int)(unsigned char)server1->mod - (int)(unsigned char)server2->mod;
        if (diff == 0) {

            goto compare_ping_nosortdir;
        }
        break;
    case 9:
        goto compare_ping;
    default:
        diff = 0;
        break;
    }
    goto apply_sortdir;

compare_ping_nosortdir:
    sortDir = 0;
compare_ping:
    diff = (int)(short)server1->ping - (int)(short)server2->ping;
    if (diff == 0) {

        diff = I_stricmp(server1->gameType, server2->gameType);
        if (diff == 0)
            diff = LAN_CompareHostname(server1->hostName, server2->hostName);
    }

apply_sortdir:
    if (sortDir)
        diff = -diff;
    return diff;
}
