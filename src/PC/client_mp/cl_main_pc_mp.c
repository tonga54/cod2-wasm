#include "common_types.h"
#include "imports.h"
extern clientStatic_t cls;

typedef struct serverStatusRequest_s {
    char string[0x2000];
    netadr_t address;
    int time;
    int startTime;
    qboolean pending;
    qboolean print;
    qboolean retrieved;
} serverStatusRequest_t;

extern serverStatusRequest_t cl_serverStatusList[16];
static Bool s_playerMute[64];
extern int NET_CompareAdrSigned(const int *a, const int *b);
extern qboolean NET_CompareAdr(netadr_t a, netadr_t b);
extern void qsort(void *base, unsigned int nmemb, unsigned int size, int (*compar)(const void *, const void *));
extern int atoi(const char *nptr);
extern const char *Info_ValueForKey(const char *s, const char *key);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern const char *va(const char *format, ...);
extern void Cbuf_ExecuteText(int exec_when, const char *text);
extern void Com_PumpMessageLoop(void);
extern const char *MSG_ReadString(msg_t *msg);
extern const char *MSG_ReadStringLine(msg_t *msg);
extern void Com_DPrintf(const char *fmt, ...);
extern const char *Dvar_GetString(const char *name);
extern const char *NET_AdrToString(netadr_t adr);
extern void Info_SetValueForKey(const char *s, const char *key, const char *value);
extern int Cmd_Argc(void);
extern const char *Cmd_Argv(int arg);
extern void Com_Printf(const char *fmt, ...);
extern int I_stricmp(const char *s0, const char *s1);
extern int I_strnicmp(const char *s0, const char *s1, size_t n);
extern int Com_AddToString(const char *add, char *msg, int len, int maxlen, qboolean mayAddQuotes);
extern void CL_Netchan_SendOOBPacket(int len, const void *data, netadr_t to);
extern int NET_StringToAdr(const char *s, netadr_t *a);
extern Bool NET_OutOfBandPrint(netsrc_t sock, netadr_t adr, const char *data);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern int Sys_Milliseconds(void);
extern int sscanf(const char *str, const char *format, ...);
extern char *strchr(const char *s, int c);
extern Bool Dvar_GetBool(const char *name);
extern int sprintf(char *str, const char *format, ...);
extern void SND_StopSounds(int a);
extern void SV_Frame(int a);
extern void CL_Disconnect(void);
extern void Con_Close(void);
extern qboolean NET_IsLocalAddress(netadr_t adr);
#ifdef COD2_DOWNSTREAM_WASM
extern qboolean Sys_IsLANAddress(netadr_t adr);
extern const dvar_t *net_lanauthorize;
#endif
extern void UI_CloseAll(void);
extern void SCR_UpdateScreen(void);
extern void Com_Error(int level, const char *fmt, ...);
extern void Com_Memset(void *dest, int val, int count);
extern void Dvar_SetString(const dvar_t *dvar, const char *value);

static byte rconGlob[40];

static int CL_CompareAdrSigned(const int *a, const int *b);
void CL_SetServerInfo(serverInfo_t *server, const char *info, int ping);
void CL_SetServerInfoByAddress(netadr_t from, const char *info, int ping);
void CL_ServerInfoPacket(netadr_t from, msg_t *msg, int time);
qboolean CL_CDKeyValidate(const char *key, const char *checksum);
void CL_SortGlobalServers(void);
void CL_RconInit(void);
void CL_Rcon_f(void);
void CL_ServerStatusResponse(netadr_t from, msg_t *msg);
void CL_ResetPlayerMuting(int clientIndex);
void CL_MutePlayer(int clientIndex);
Bool CL_IsPlayerMuted(int clientIndex);
void CL_ClearMutedList(void);
void CL_GlobalServers_f(void);
void CL_ServersResponsePacket(netadr_t from, msg_t *msg);
void CL_Ping_f(void);
void CL_Connect_f(void);
int CL_ServerStatus(char *serverAddress, char *serverStatusString, int maxLen);
void CL_ServerStatus_f(void);

static int CL_CompareAdrSigned(const int *a, const int *b)
{
    return NET_CompareAdrSigned(a, b);
}

void CL_SetServerInfo(serverInfo_t *server, const char *info, int ping)
{
    if (server == NULL) {
        return;
    }

    if (info != NULL) {
        server->clients = (byte)atoi(Info_ValueForKey(info, "clients"));
        I_strncpyz(server->hostName, Info_ValueForKey(info, "hostname"), 0x20);
        I_strncpyz(server->mapName, Info_ValueForKey(info, "mapname"), 0x20);
        server->maxClients = (byte)atoi(Info_ValueForKey(info, "sv_maxclients"));
        I_strncpyz(server->game, Info_ValueForKey(info, "game"), 0x18);
        I_strncpyz(server->gameType, Info_ValueForKey(info, "gametype"), 0x10);
        server->netType = (byte)atoi(Info_ValueForKey(info, "nettype"));
        server->minPing = (short)atoi(Info_ValueForKey(info, "minping"));
        server->maxPing = (short)atoi(Info_ValueForKey(info, "maxping"));
        server->allowAnonymous = (byte)atoi(Info_ValueForKey(info, "sv_allowAnonymous"));
        server->consoleDisabled = (char)atoi(Info_ValueForKey(info, "con_disabled"));
        server->bPassword = (byte)atoi(Info_ValueForKey(info, "pswrd"));
        server->pure = (byte)atoi(Info_ValueForKey(info, "pure"));
        server->friendlyfire = (char)atoi(Info_ValueForKey(info, "ff"));
        server->killcam = (char)atoi(Info_ValueForKey(info, "kc"));
        server->hardware = (byte)atoi(Info_ValueForKey(info, "hw"));
        server->mod = (byte)atoi(Info_ValueForKey(info, "mod"));
        server->voice = (byte)atoi(Info_ValueForKey(info, "voice"));
    }

    server->ping = (short)ping;
}

void CL_SetServerInfoByAddress(netadr_t from, const char *info, int ping)
{
    clientStatic_t *cls;
    int i;
    int low;
    int high;
    int mid;
    int compare;

    Com_PumpMessageLoop();
    cls = (clientStatic_t *)imp_cls;

    for (i = 0; i < 128; ++i) {
        if (NET_CompareAdr(from, cls->localServers[i].adr)) {
            CL_SetServerInfo(&cls->localServers[i], info, ping);
        }
    }

    high = cls->numglobalservers;
    if (high > 0) {
        low = 0;
        while (low < high) {
            mid = (low + high) / 2;
            compare = NET_CompareAdrSigned((const int *)&from, (const int *)&cls->globalServers[mid].adr);
            if (compare < 0) {
                high = mid;
                continue;
            }

            if (compare > 0) {
                low = mid + 1;
                continue;
            }

            while (mid > 0 && NET_CompareAdrSigned((const int *)&from, (const int *)&cls->globalServers[mid - 1].adr) == 0) {
                --mid;
            }

            for (i = mid; i < cls->numglobalservers; ++i) {
                if (NET_CompareAdrSigned((const int *)&from, (const int *)&cls->globalServers[i].adr) != 0) {
                    break;
                }

                CL_SetServerInfo(&cls->globalServers[i], info, ping);
            }
            break;
        }
    }

    for (i = 0; i < 128; ++i) {
        if (NET_CompareAdr(from, cls->favoriteServers[i].adr)) {
            CL_SetServerInfo(&cls->favoriteServers[i], info, ping);
        }
    }
}

void CL_ServerInfoPacket(netadr_t from, msg_t *msg, int time)
{
    const char *infoString;
    int prot;
    int expectedProt;
    const char *protoStr;
    int i;
    ping_t *pinglist;
    ping_t *entry;
    char info[1024];
    int len;
    clientStatic_t *cls;

    infoString = MSG_ReadString(msg);
    prot = atoi(Info_ValueForKey(infoString, (const char *)"protocol"));

    protoStr = Dvar_GetString((const char *)"debug_protocol");
    if (*protoStr)
        expectedProt = atoi(protoStr);
    else
        expectedProt = 0x76;

    if (prot != expectedProt) {
        Com_DPrintf((const char *)"Different protocol info packet: %s\n", infoString);
        return;
    }

    pinglist = (ping_t *)imp_cl_pinglist;
    for (i = 0; i < 16; i++) {
        ping_t *ping = &pinglist[i];

        if (ping->adr.port == 0)
            continue;

        if (ping->time != 0)
            continue;

        {
            if (!NET_CompareAdr(from, ping->adr))
                continue;
        }

        {
            int pingTime = time - ping->start + 1;
            ping->time = pingTime;
        }

        Com_DPrintf((const char *)"ping time %dms from %s\n", ping->time, NET_AdrToString(from));

        I_strncpyz(ping->info, infoString, sizeof(ping->info));

        {
            int nettype = (from.type >= 3 && from.type <= 4) ? 1 : 0;
            Info_SetValueForKey(ping->info, (const char *)"nettype", va((const char *)"%d", nettype));
        }

        CL_SetServerInfoByAddress(from, infoString, ping->time);
        return;
    }

    cls = (clientStatic_t *)imp_cls;

    if (cls->waitglobalserverresponse)
        return;

    for (i = 0; i < 128; i++) {
        serverInfo_t *server = &cls->localServers[i];

        if (server->adr.port != 0) {

            if (NET_CompareAdr(from, server->adr))
                return;
            continue;
        }

        {
            cls->numlocalservers = i + 1;

            server->adr = from;
            server->dirty = 1;
            server->hostName[0] = '\0';
            server->mapName[0] = '\0';
            server->game[0] = '\0';
            server->minPing = 0;
            server->maxPing = 0;
            server->ping = -1;
            server->gameType[0] = '\0';
            server->requestCount = 0;
            server->netType = (byte)from.type;
            server->clients = 0;

            /* infoResponse has one info string, already consumed above.
             * Populate this newly discovered entry and let the ping queue
             * measure it before the native server browser displays it. */
            CL_SetServerInfo(server, infoString, -1);
            return;
        }
    }

    Com_DPrintf((const char *)"MAX_OTHER_SERVERS hit, dropping infoResponse\n");
}

qboolean CL_CDKeyValidate(const char *key, const char *checksum)
{
    unsigned int crc = 0;
    char chs[8];
    int i;

    for (i = 1; i <= 16; i++) {
        int ch = (signed char)key[i - 1];
        int j;
        crc ^= (unsigned int)ch;
        for (j = 8; j != 0; j--) {
            if (crc & 1) {
                crc >>= 1;
                crc ^= 0xa001;
            } else {
                crc >>= 1;
            }
        }
    }

    sprintf(chs, (const char *)"%04x", crc);

    if (checksum != NULL && I_strnicmp(chs, checksum, 4) != 0)
        return 0;

    return 1;
}

void CL_SortGlobalServers(void)
{
    clientStatic_t *cls = (clientStatic_t *)imp_cls;
    int count = cls->numglobalservers;
    qsort(cls->globalServers, count, sizeof(serverInfo_t), (int (*)(const void *, const void *))CL_CompareAdrSigned);
}

void CL_RconInit(void)
{
    rconGlob[0] = 0;
    *(int *)(rconGlob + 24) = 1;
}

void CL_Rcon_f(void)
{
    const char *cmd;
    char message[0x400];
    int offset;
    int i;
    netadr_t sendAdr;

    if (Cmd_Argc() <= 1) {
        Com_Printf((const char *)"USAGE: rcon <command> <options...>\n");
        return;
    }

    cmd = Cmd_Argv(1);

    if (I_stricmp(cmd, (const char *)"login") == 0) {
        if (Cmd_Argc() != 3) {
            Com_Printf((const char *)"USAGE: rcon login <password>\n");
            return;
        }
        {
            const char *pass = Cmd_Argv(2);
            int passLen = strlen(pass);
            if (passLen > 0x17) {
                Com_Printf((const char *)"rcon password must be %i characters or less\n", 0x18);
                return;
            }
            memcpy(&rconGlob, pass, passLen + 1);
        }
        return;
    }

    if (I_stricmp(cmd, (const char *)"logout") == 0) {
        if (!rconGlob[0]) {
            Com_Printf((const char *)"Not logged in\n");
            return;
        }
        rconGlob[0] = 0;
        return;
    }

    if (I_stricmp(cmd, (const char *)"host") == 0) {
        if (Cmd_Argc() != 3) {
            Com_Printf((const char *)"USAGE: rcon host <address>\n");
            return;
        }
        if (!NET_StringToAdr(Cmd_Argv(2), (netadr_t *)(rconGlob + 24))) {
            Com_Printf((const char *)"bad host address\n");
            return;
        }
        if (*(unsigned short *)(rconGlob + 32) == 0) {
            *(unsigned short *)(rconGlob + 32) = 0x2071;
        }
        return;
    }

    if (!rconGlob[0]) {
        Com_Printf((const char *)"You need to log in with 'rcon login <password>' before using rcon.\n");
        return;
    }

    message[0] = (char)0xff;
    message[1] = (char)0xff;
    message[2] = (char)0xff;
    message[3] = (char)0xff;
    message[4] = '\0';

    offset = Com_AddToString((const char *)"rcon ", message, 4, 0x400, 0);
    offset = Com_AddToString((char *)rconGlob, message, offset, 0x400, 0);

    for (i = 1; i < Cmd_Argc(); i++) {
        offset = Com_AddToString((const char *)" ", message, offset, 0x400, 0);
        offset = Com_AddToString(Cmd_Argv(i), message, offset, 0x400, 1);
    }

    if (offset == 0x400) {
        Com_Printf((const char *)"rcon commands are limited to %i characters\n", 0x3ff);
        return;
    }

    message[offset] = '\0';

    {
        clientConnection_t *clcLocal = *(clientConnection_t **)imp_clc;
        if (clcLocal->state > 4) {

            memcpy(&sendAdr, &clcLocal->netchan.remoteAddress, sizeof(netadr_t));
        } else {
            int addrType = *(int *)(rconGlob + 24);
            if (addrType == 1) {
                Com_Printf((const char *)"Can't determine rcon target.  You can fix this by either:\n");
                Com_Printf((const char *)"1) Joining the server as a player.\n");
                Com_Printf((const char *)"2) Setting the host server with 'rcon host <address>'.\n");
                return;
            }
            memcpy(&sendAdr, rconGlob + 24, sizeof(netadr_t));
        }
    }

    {
        int msgLen = strlen(message);
        CL_Netchan_SendOOBPacket(msgLen, message, sendAdr);
    }
}

void CL_ServerStatusResponse(netadr_t from, msg_t *msg);

void CL_ServerStatusResponse(netadr_t from, msg_t *msg)
{
    int i;
    serverStatusRequest_t *serverStatus = NULL;
    const char *line;
    char info[1024];
    int infoField;

    for (i = 0; i < 16; i++) {
        if (NET_CompareAdr(from, cl_serverStatusList[i].address)) {
            serverStatus = &cl_serverStatusList[i];
            goto found_status;
        }
    }
    return;

found_status:
    if (!serverStatus)
        return;

    line = MSG_ReadStringLine(msg);
    Com_sprintf(serverStatus->string, 0x2000, (const char *)"%s", line);

    if (serverStatus->print) {
        Com_Printf((const char *)"Server settings:\n");

        while (*line) {
            for (infoField = 0; infoField < 2; infoField++) {
                int j = 0;
                if (*line == '\\')
                    line++;
                while (*line && *line != '\\' && j < 0x3ff) {
                    info[j++] = *line++;
                }
                info[j] = '\0';

                if (infoField == 0)
                    Com_Printf((const char *)"%-24s", info);
                else
                    Com_Printf((const char *)"%s\n", info);
            }
            if (!*line)
                break;
        }
    }

    {
        int statusLen = strlen(serverStatus->string);
        Com_sprintf(serverStatus->string + statusLen, 0x2000 - statusLen, (const char *)"\\");
    }

    if (serverStatus->print) {
        Com_Printf((const char *)"\nPlayers:\n");
        Com_Printf((const char *)"num: score: ping: name:\n");
    }

    {
        int playerIdx = 0;
        while (1) {
            line = MSG_ReadStringLine(msg);
            if (!*line)
                break;

            {
                int statusLen = strlen(serverStatus->string);
                Com_sprintf(serverStatus->string + statusLen, 0x2000 - statusLen, (const char *)"\\%s", line);
            }

            if (serverStatus->print) {
                int ping = 0, score = 0;
                const char *name;
                sscanf(line, (const char *)"%d %d", &score, &ping);
                name = strchr(line, ' ');
                if (name) {
                    name = strchr(name + 1, ' ');
                    if (name)
                        name++;
                    else
                        name = (const char *)"unknown";
                } else {
                    name = (const char *)"unknown";
                }
                Com_Printf((const char *)"%-2d   %-3d    %-3d   %s\n", playerIdx, score, ping, name);
            }

            playerIdx++;
        }
    }

    {
        int statusLen = strlen(serverStatus->string);
        Com_sprintf(serverStatus->string + statusLen, 0x2000 - statusLen, (const char *)"\\");
    }

    serverStatus->time = Sys_Milliseconds();
    serverStatus->address = from;
    serverStatus->pending = 0;

    if (serverStatus->print) {
        serverStatus->retrieved = 1;
    }
}

void CL_ResetPlayerMuting(int clientIndex)
{
    s_playerMute[clientIndex] = 0;
}

void CL_MutePlayer(int clientIndex)
{
    s_playerMute[clientIndex] = !s_playerMute[clientIndex];
    if (s_playerMute[clientIndex]) {
        Cbuf_ExecuteText(2, va("muteplayer %i", clientIndex));
    } else {
        Cbuf_ExecuteText(2, va("unmuteplayer %i", clientIndex));
    }
}

Bool CL_IsPlayerMuted(int clientIndex)
{
    return s_playerMute[clientIndex];
}

void CL_ClearMutedList(void)
{
    memset(s_playerMute, 0, sizeof(s_playerMute));
}

void CL_GlobalServers_f(void)
{
    netadr_t to;
    char command[1024];
    char *buffptr;
    int i;
    int count;

    if (Cmd_Argc() <= 2) {
        Com_Printf((const char *)"usage: globalservers <master# 0-1> <protocol> [keywords]\n");
        return;
    }

    {
        clientStatic_t *cls = (clientStatic_t *)imp_cls;
        int numglobal = cls->numglobalservers;

        for (i = 0; i < numglobal; i++) {
            byte rc = cls->globalServers[i].requestCount;
            byte rc1 = (byte)(rc + 1);
            if (rc1 == 0)
                cls->globalServers[i].requestCount = 0xFF;
            else
                cls->globalServers[i].requestCount = rc1;
        }
    }

    Com_Printf((const char *)"Requesting servers from the master...\n");

    NET_StringToAdr((const char *)"cod2master.activision.com", &to);

    {
        clientStatic_t *cls = (clientStatic_t *)imp_cls;
        cls->waitglobalserverresponse = 1;
        cls->pingUpdateSource = 1;
    }

    to.type = 4;
    to.port = (unsigned short)0xe650;

    sprintf(command, (const char *)"getservers %s", Cmd_Argv(2));

    buffptr = command + strlen(command);

    count = Cmd_Argc();
    if (count > 3) {

        for (i = 3; i != count; i++) {
            buffptr += sprintf(buffptr, (const char *)" %s", Cmd_Argv(i));
        }
    }

    if (Dvar_GetBool((const char *)"fs_restrict"))
    {
        *(int *)buffptr = 0x6d656420;
        *(short *)(buffptr + 4) = 0x6f;
    }

    NET_OutOfBandPrint(NS_SERVER, to, command);
}

void CL_ServersResponsePacket(netadr_t from, msg_t *msg)
{
    byte *buffptr;
    byte *buffend;
    int numservers;
    byte servers[256][6];
    byte ip[4];
    unsigned short port;
    netadr_t adr;
    int i;
    int count;
    clientStatic_t *cls;
    int low, high, mid;
    int compare;

    Com_PumpMessageLoop();

    Com_Printf((const char *)"CL_ServersResponsePacket\n");

    cls = (clientStatic_t *)imp_cls;
    cls->waitglobalserverresponse = 0;

    buffptr = msg->data;
    buffend = msg->data + msg->cursize;

    numservers = 0;

    while ((unsigned int)(buffptr + 1) < (unsigned int)buffend) {
        if (*buffptr == '\\')
            break;
        buffptr++;
    }

    while ((unsigned int)(buffptr + 1) < (unsigned int)(buffend - 6)) {

        ip[0] = buffptr[1];
        ip[1] = buffptr[2];
        ip[2] = buffptr[3];
        ip[3] = buffptr[4];

        {
            unsigned short portVal;
            portVal = ((unsigned short)buffptr[5] << 8) | (unsigned short)buffptr[6];

            port = (unsigned short)((portVal >> 8) | (portVal << 8));
        }

        buffptr += 7;

        servers[numservers][0] = ip[0];
        servers[numservers][1] = ip[1];
        servers[numservers][2] = ip[2];
        servers[numservers][3] = ip[3];
        *(unsigned short *)&servers[numservers][4] = port;

        if (*buffptr != '\\')
            break;

        Com_DPrintf((const char *)"server: %d ip: %d.%d.%d.%d:%d\n", numservers, ip[0], ip[1], ip[2], ip[3], (int)port);

        numservers++;
        if (numservers == 256)
            break;

        if (buffptr[1] == 0x45 && buffptr[2] == 0x4f && buffptr[3] == 0x54)
            break;

        continue;
    }

    cls = (clientStatic_t *)imp_cls;
    count = cls->numglobalservers;

    for (i = 0; i < numservers && count <= 0x4e1f; i++) {

        ip[0] = servers[i][0];
        ip[1] = servers[i][1];
        ip[2] = servers[i][2];
        ip[3] = servers[i][3];
        port = *(unsigned short *)&servers[i][4];

        adr.type = 4;
        adr.ip[0] = ip[0];
        adr.ip[1] = ip[1];
        adr.ip[2] = ip[2];
        adr.ip[3] = ip[3];
        adr.port = port;

        cls = (clientStatic_t *)imp_cls;
        high = cls->numglobalservers;
        if (high > 0) {
            low = 0;
            while (low < high) {
                cls = (clientStatic_t *)imp_cls;
                mid = (low + high) / 2;
                compare = NET_CompareAdrSigned((const int *)&adr, (const int *)&cls->globalServers[mid]);
                if (compare < 0) {
                    high = mid;
                } else if (compare > 0) {
                    low = mid + 1;
                } else {

                    int j = mid;
                    while (j - 1 >= 0) {
                        if (NET_CompareAdrSigned((const int *)&adr, (const int *)&cls->globalServers[j - 1]) != 0)
                            break;
                        j--;
                    }

                    {
                        clientStatic_t *clsStatic;
                        cls = (clientStatic_t *)imp_cls;
                        clsStatic = (clientStatic_t *)cls;
                        while (1) {
                            serverInfo_t *srv = &clsStatic->globalServers[j];
                            srv->adr.type = adr.type;
                            srv->adr.ip[0] = adr.ip[0];
                            srv->adr.ip[1] = adr.ip[1];
                            srv->adr.ip[2] = adr.ip[2];
                            srv->adr.ip[3] = adr.ip[3];
                            srv->adr.port = adr.port;
                            srv->clients = 0;
                            srv->hostName[0] = '\0';
                            srv->mapName[0] = '\0';
                            srv->maxClients = 0;
                            srv->minPing = 0;
                            srv->maxPing = 0;
                            srv->ping = -1;
                            srv->game[0] = '\0';
                            srv->gameType[0] = '\0';
                            srv->netType = 0;
                            srv->allowAnonymous = 0;
                            srv->dirty = 1;
                            srv->requestCount = 0;

                            j++;
                            if (j >= cls->numglobalservers)
                                break;
                            if (NET_CompareAdrSigned((const int *)&adr, (const int *)&clsStatic->globalServers[j]) != 0) {
                                cls = (clientStatic_t *)imp_cls;
                                break;
                            }
                        }
                    }
                    goto next_server;
                }
            }
            cls = (clientStatic_t *)imp_cls;
        }

        {
            serverInfo_t *srv = &cls->globalServers[count];
            srv->adr.type = 4;
            srv->adr.ip[0] = ip[0];
            srv->adr.ip[1] = ip[1];
            srv->adr.ip[2] = ip[2];
            srv->adr.ip[3] = ip[3];
            srv->adr.port = port;
            srv->clients = 0;
            srv->hostName[0] = '\0';
            srv->mapName[0] = '\0';
            srv->maxClients = 0;
            srv->minPing = 0;
            srv->maxPing = 0;
            srv->ping = -1;
            srv->game[0] = '\0';
            srv->gameType[0] = '\0';
            srv->netType = 0;
            srv->allowAnonymous = 0;
            srv->dirty = 1;
            srv->requestCount = 0;
            count++;
        }

    next_server:;
    }

    cls->numglobalservers = count;
    qsort(cls->globalServers, count, sizeof(serverInfo_t), (int (*)(const void *, const void *))CL_CompareAdrSigned);

    Com_Printf((const char *)"%d servers parsed (total %d)\n", numservers, count);

}

void CL_Ping_f(void)
{
    netadr_t to;
    int now;
    ping_t *pinglist;
    ping_t *entry;
    ping_t *endEntry;
    ping_t *bestEntry;

    if (Cmd_Argc() != 2) {
        Com_Printf((const char *)"usage: ping [server]\n");
        return;
    }

    memset(&to, 0, sizeof(netadr_t));

    if (!NET_StringToAdr(Cmd_Argv(1), &to))
        return;

    now = Sys_Milliseconds();
    pinglist = (ping_t *)imp_cl_pinglist;
    entry = pinglist;
    endEntry = pinglist + 16;

    while (entry != endEntry) {
        ping_t *ping = entry;

        if (ping->adr.port == 0)
            goto found_slot;

        {
            if (ping->time == 0) {

                int elapsed = now - ping->start;
                if (elapsed > 0x1f3)
                    goto found_slot;
            } else if (ping->time > 0x1f3) {

                goto found_slot;
            }
        }

        entry++;
    }

    {
        ping_t *scan = pinglist;
        int oldest = (int)0x80000000;
        bestEntry = pinglist;

        while (scan != endEntry) {
            int age = now - scan->start;
            if (age > oldest) {
                bestEntry = scan;
                oldest = age;
            }
            scan++;
        }
        entry = bestEntry;
        goto fill_slot;
    }

found_slot:
    entry->adr.port = 0;

fill_slot:

    entry->adr = to;

    entry->start = Sys_Milliseconds();
    entry->time = 0;

    {
        CL_SetServerInfoByAddress(entry->adr, NULL, 0);
    }

    NET_OutOfBandPrint(NS_CLIENT1, to, "getinfo xxx");

}

void CL_Connect_f(void)
{
    const char *server;
    clientConnection_t *clc;
    byte *cls_servername;

    if (Cmd_Argc() != 2) {
        Com_Printf((const char *)"usage: connect [server]\n");
        return;
    }

    SND_StopSounds(0);

    clc = *(clientConnection_t **)imp_clc;
    clc->serverMessage[0] = 0;

    server = Cmd_Argv(1);

    {
        dvar_t *sv_running = *(dvar_t **)imp_com_sv_running;
        if (sv_running->current.enabled != 0) {

            if (memcmp(server, (const char *)"localhost", 10) == 0)
            {

                LegacyHacks *legacyHacks = *(LegacyHacks **)imp_legacyHacks;
                legacyHacks->sv_killserver = 1;
            }
        }
    }

    {
        LegacyHacks *legacyHacks = *(LegacyHacks **)imp_legacyHacks;
        legacyHacks->cl_serverloadmap[0] = '\0';
        legacyHacks = *(LegacyHacks **)imp_legacyHacks;
        legacyHacks->cl_serverloadgametype[0] = '\0';
    }

#ifndef __EMSCRIPTEN__
    SV_Frame(0);
#endif
    CL_Disconnect();
    Con_Close();

    cls_servername = (byte *)cls.servername;
    I_strncpyz((char *)cls_servername, server, sizeof(((clientStatic_t *)0)->servername));

    clc = *(clientConnection_t **)imp_clc;
    {
        clientConnection_t *clcConn = (clientConnection_t *)clc;
        if (!NET_StringToAdr((const char *)cls_servername, &clcConn->serverAddress)) {
            Com_Printf((const char *)"Bad server address\n");
            clcConn->state = 0;
            return;
        }

        if (clcConn->serverAddress.port == 0) {
            clcConn->serverAddress.port = 0x2071;
        }

        {
            unsigned short netPort = clcConn->serverAddress.port;

            short displayPort = (short)((netPort >> 8) | (netPort << 8));
            Com_Printf((const char *)"%s resolved to %i.%i.%i.%i:%i\n",
                       cls_servername,
                       (int)clcConn->serverAddress.ip[0],
                       (int)clcConn->serverAddress.ip[1],
                       (int)clcConn->serverAddress.ip[2],
                       (int)clcConn->serverAddress.ip[3],
                       (int)displayPort);
        }
    }

    if (!NET_IsLocalAddress(clc->serverAddress)
#ifdef COD2_DOWNSTREAM_WASM
        /* The browser port targets the fixed LAN gateway. Match the engine's
         * existing LAN authorization policy rather than requiring Mac OS
         * registration preferences. The normal server challenge still runs. */
        && (net_lanauthorize->current.enabled || !Sys_IsLANAddress(clc->serverAddress))
#endif
    ) {

        unsigned int crc = 0;
        byte *cdkey = (byte *)imp_cl_cdkey;
        byte *cdkeyEnd = cdkey + 16;
        char chs[8];

        while (cdkey != cdkeyEnd) {
            int ch = (signed char)*cdkey;
            int j;
            crc ^= (unsigned int)ch;
            for (j = 8; j != 0; j--) {
                if (crc & 1) {
                    crc >>= 1;
                    crc ^= 0xa001;
                } else {
                    crc >>= 1;
                }
            }
            cdkey++;
        }

        sprintf(chs, (const char *)"%04x", crc);

        if (I_strnicmp(chs, (const char *)imp_cl_cdkeychecksum, 4) != 0) {
            Com_Error(1, (const char *)"EXE_ERR_INVALID_CD_KEY");
            return;
        }
    }

    {
        clientConnection_t *clcConn;
        clc = *(clientConnection_t **)imp_clc;
        clcConn = (clientConnection_t *)clc;

        if (NET_IsLocalAddress(clcConn->serverAddress)) {
            clcConn->state = 4;
        } else {
            clcConn = *(clientConnection_t **)imp_clc;
            clcConn->state = 3;
            clc = clcConn;
        }
    }

    {
        clientActive_t *clActive = *(clientActive_t **)imp_cl;
        clActive->keyCatchers = 0;
        clActive->displayHUDWithKeycatchUI = 0;
    }

    {
        clientConnection_t *clcConn = (clientConnection_t *)clc;
        clcConn->connectTime = (int)0xfffe7961;
        clcConn->connectPacketCount = 0;

        clcConn->qport = *(int *)imp_g_qport;
    }

    UI_CloseAll();
    SCR_UpdateScreen();

    return;
}

int CL_ServerStatus(char *serverAddress, char *serverStatusString, int maxLen)
{
    netadr_t to;
    serverStatusRequest_t *serverStatus;
    int i;

    if (serverAddress == NULL) {
        for (i = 0; i < 16; i++) {
            cl_serverStatusList[i].address.port = 0;
            cl_serverStatusList[i].retrieved = 1;
        }
        return 0;
    }

    if (!NET_StringToAdr(serverAddress, &to))
        return 0;

    serverStatus = NULL;
    for (i = 0; i < 16; i++) {
        if (NET_CompareAdr(to, cl_serverStatusList[i].address)) {
            serverStatus = &cl_serverStatusList[i];
            goto found_entry;
        }
    }

    {
        int idx;
        for (idx = 0; idx < 16; idx++) {
            if (cl_serverStatusList[idx].retrieved != 0) {
                serverStatus = &cl_serverStatusList[idx];
                goto found_entry;
            }
        }
    }

    {
        int bestIdx = 0;
        int oldestTime = cl_serverStatusList[0].startTime;
        int j;

        for (j = 1; j <= 15; j++) {
            int entryTime = cl_serverStatusList[j].startTime;
            if (oldestTime > entryTime) {
                bestIdx = j;
                oldestTime = cl_serverStatusList[bestIdx].startTime;
            }
        }

        serverStatus = &cl_serverStatusList[bestIdx];
    }

found_entry:

    if (serverStatusString == NULL) {

        serverStatus->retrieved = 1;
        return 0;
    }

    {
        if (NET_CompareAdr(to, serverStatus->address)) {

            if (serverStatus->pending == 0) {

                I_strncpyz(serverStatusString, serverStatus->string, maxLen);
                serverStatus->retrieved = 1;
                serverStatus->startTime = 0;
                return 1;
            }

            {
                int startTime = serverStatus->startTime;
                int now = Sys_Milliseconds();
                const dvar_t *resendDvar = *(const dvar_t **)imp_cl_serverStatusResendTime;
                int resendTime = resendDvar->current.integer;
                if (startTime >= now - resendTime)
                    return 0;
            }

            serverStatus->print = 0;
            serverStatus->pending = 1;
            serverStatus->retrieved = 0;
            serverStatus->time = 0;
            serverStatus->startTime = Sys_Milliseconds();

            NET_OutOfBandPrint(NS_CLIENT1, to, "getstatus");
            return 0;
        }
    }

    if (serverStatus->retrieved == 0)
        return 0;

    serverStatus->address = to;
    serverStatus->print = 0;
    serverStatus->pending = 1;
    serverStatus->retrieved = 0;
    serverStatus->startTime = Sys_Milliseconds();
    serverStatus->time = 0;

    NET_OutOfBandPrint(NS_CLIENT1, to, "getstatus");
    return 0;
}

void CL_ServerStatus_f(void)
{
    netadr_t to;
    const char *serverAddr;
    serverStatusRequest_t *statusEntry;
    int i;

    Com_Memset(&to, 0, sizeof(netadr_t));

    if (Cmd_Argc() == 2) {

        serverAddr = Cmd_Argv(1);
    } else {

        clientConnection_t *clcConn = *(clientConnection_t **)imp_clc;
        if (clcConn->state != 8)
            goto not_connected;
        if (clcConn->demoplaying != 0)
            goto not_connected;

        serverAddr = cls.servername;
    }

    if (!NET_StringToAdr(serverAddr, &to))
        return;

    NET_OutOfBandPrint(NS_CLIENT1, to, "getstatus");

    {
        for (i = 0; i < 16; i++) {
            if (NET_CompareAdr(to, cl_serverStatusList[i].address)) {
                statusEntry = &cl_serverStatusList[i];
                goto store_entry;
            }
        }
    }

    {
        int idx;
        for (idx = 0; idx < 16; idx++) {
            if (cl_serverStatusList[idx].retrieved != 0) {
                statusEntry = &cl_serverStatusList[idx];
                goto store_entry;
            }
        }
    }

    {
        int bestIdx = 0;
        int oldestTime = cl_serverStatusList[0].startTime;
        int j;

        for (j = 1; j <= 15; j++) {
            int entryTime = cl_serverStatusList[j].startTime;
            if (oldestTime > entryTime) {
                bestIdx = j;
                oldestTime = cl_serverStatusList[bestIdx].startTime;
            }
        }

        statusEntry = &cl_serverStatusList[bestIdx];
    }

store_entry:

    statusEntry->address = to;
    statusEntry->print = 1;
    statusEntry->pending = 1;

    return;

not_connected:
    Com_Printf((const char *)"Not connected to a server.\n");
    Com_Printf((const char *)"Usage: serverstatus [server]\n");
}
