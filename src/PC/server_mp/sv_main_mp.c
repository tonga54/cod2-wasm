#include "common_types.h"
#include "imports.h"
#include "cod2_feature_config.h"
#include "pb_public.h"
#include "PC/qcommon/net_hardening.h"
#include <stdarg.h>
extern void SV_DelayDropClient(client_t *drop, const char *reason);
extern void MSG_WriteReliableCommandToBuffer(const char *pszCommand, char *pszBuffer, int iBufferSize);
extern int I_strnicmp(const char *s0, const char *s1, size_t n);
extern Bool NET_OutOfBandPrint(netsrc_t sock, netadr_t adr, const char *data);

extern void Scr_FreeValue(int value);
extern void SV_ResetSkeletonCache(void);
extern int G_RunFrame(int levelTime);
extern void LargeLocal_LargeLocal(LargeLocal *ll, int size);
extern void *LargeLocal_GetBuf(LargeLocal *ll);
extern void ZN10LargeLocalD1Ev(LargeLocal *ll);
extern playerState_t *SV_GameClientNum(int num);
extern void SV_ClientThink(client_t *cl, usercmd_t *cmd);
extern const char *Dvar_InfoString(int bit);
extern Bool Dvar_GetBool(const char *dvarName);
extern int Dvar_GetInt(const char *dvarName);
extern const char *Dvar_GetString(const char *dvarName);
extern const char *Info_ValueForKey(const char *s, const char *key);
extern void Info_SetValueForKey(char *s, const char *key, const char *value);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern void I_strncat(char *dest, int size, const char *src);
extern qboolean FS_iwIwd(char *iwd, char *base);
extern int G_GetClientScore(int clientNum);
extern void SV_Cmd_TokenizeString(const char *text_in);
extern int SV_Cmd_Argc(void);
extern char *SV_Cmd_Argv(int arg);
extern const char *va(const char *fmt, ...);
extern const char *MSG_ReadStringLine(msg_t *msg);
extern void SV_Netchan_AddOOBProfilePacket(int iLength);
extern const char *NET_AdrToString(netadr_t a);
extern int I_stricmp(const char *s1, const char *s2);
extern void Com_Printf(const char *fmt, ...);
extern void SV_GetChallenge(netadr_t from);
extern void SV_DirectConnect(netadr_t from);
extern void SV_AuthorizeIpPacket(netadr_t from);
extern void SVC_RemoteCommand(netadr_t from, msg_t *msg);
extern void Dvar_SetInt(const dvar_t *dvar, int value);
extern void Com_Shutdown(const char *finalmsg);
extern void Cbuf_AddText(const char *text);
extern void CL_FlushDebugData(int fromServer);
extern void Scr_SetLoading(int loading);
extern void SV_ArchiveSnapshot(void);
extern void SV_SendClientMessages(void);
extern void SV_MasterHeartbeat(const char *heartbeat);
extern void SV_SetConfig(int start, int max, int bit);
extern const char *Dvar_InfoString_Big(int bit);
extern void SV_SetConfigstring(int index, const char *val);
extern void SV_DropClient(client_t *drop, const char *reason);
extern void Com_DPrintf(const char *fmt, ...);

#if !defined(__EMSCRIPTEN__) && !defined(_MSC_VER)
__asm__(".Lsvpkt_fmt: .asciz \"\"\n");
#endif
void SV_PktEvtDbg(const char *fmt, int netchanResult, int clState, int serverId, int relAck)
{
    (void)fmt;
    (void)netchanResult;
    (void)clState;
    (void)serverId;
    (void)relAck;
}

extern struct serverStatic_t svs;
extern struct server_t sv;
extern const dvar_t *sv_fps;
extern const dvar_t *sv_timeout;
extern const dvar_t *sv_zombietime;
extern const dvar_t *rcon_password;
extern const dvar_t *sv_privatePassword;
extern const dvar_t *sv_allowDownload;
extern const dvar_t *sv_maxclients;
extern const dvar_t *sv_privateClients;
extern const dvar_t *sv_hostname;
extern const dvar_t *sv_reconnectlimit;
extern const dvar_t *sv_padPackets;
extern const dvar_t *sv_mapname;
extern const dvar_t *sv_cheats;
extern const dvar_t *sv_serverid;
extern const dvar_t *sv_maxRate;
extern const dvar_t *sv_minPing;
extern const dvar_t *sv_maxPing;
extern const dvar_t *sv_gametype;
extern const dvar_t *sv_debugRate;
extern const dvar_t *sv_debugReliableCmds;
extern const dvar_t *sv_pure;
extern const dvar_t *sv_floodProtect;
extern const dvar_t *sv_allowAnonymous;
extern const dvar_t *sv_showCommands;
extern const dvar_t *sv_packet_info;
extern const dvar_t *sv_voice;
extern const dvar_t *sv_voiceQuality;
extern const dvar_t *sv_showAverageBPS;
extern const dvar_t *sv_kickBanTime;
extern const dvar_t *sv_iwds;
extern const dvar_t *sv_iwdNames;
extern const dvar_t *sv_referencedIwds;
extern const dvar_t *sv_referencedIwdNames;
extern const dvar_t *sv_allowedClan1;
extern const dvar_t *sv_allowedClan2;
extern const dvar_t *sv_mapRotation;
extern const dvar_t *sv_mapRotationCurrent;
extern const dvar_t *sv_disableClientConsole;
static char string_00edae00[1024];

void SV_AddServerCommand(client_t *client, svscmd_type type, const char *cmd);
extern int MSG_ReadShort(msg_t *msg);
extern void MSG_BeginReading(msg_t *msg);
extern int MSG_ReadLong(msg_t *msg);
extern int MSG_ReadByte(msg_t *msg);
extern qboolean Netchan_Process(netchan_t *chan, msg_t *msg);
extern void SV_Netchan_Decode(client_t *cl, byte *data, int remaining);
extern void SV_ExecuteClientMessage(client_t *cl, msg_t *msg);
extern qboolean NET_CompareBaseAdr(netadr_t a, netadr_t b);
extern void SV_PreGameUserVoice(client_t *cl, msg_t *msg);
extern void SV_UserVoice(client_t *cl, msg_t *msg);
void SV_VoicePacket(netadr_t from, msg_t *msg);
void SVC_Status(netadr_t from);
void SVC_GameCompleteStatus(netadr_t from);
void SVC_Info(netadr_t from);
void SV_ConnectionlessPacket(netadr_t from, msg_t *msg);
void SV_PacketEvent(netadr_t from, msg_t *msg);
void SV_FreeClientScriptId(client_t *cl);
void SV_RunFrame(void);
void SV_BotUserMove(client_t *cl);
qboolean SV_BotIsTestClient(int clientNum);
void SV_BotResetClient(int clientNum);
qboolean SV_BotStop(int clientNum);
qboolean SV_BotSetMovement(int clientNum, int forward, int right);
qboolean SV_BotSetAngles(int clientNum, const vec_t *angles);
qboolean SV_BotSetWeapon(int clientNum, int weapon);
qboolean SV_BotSetAction(int clientNum, const char *action);
void SV_SendServerCommand(client_t *cl, svscmd_type type, const char *fmt, ...);
void SV_Frame(int msec);

COD2_ASSERT_FIELD(struct LegacyHacks, sv_killserver, 0xdd);
COD2_ASSERT_FIELD(struct LegacyHacks, sv_killreason, 0xde);

void SV_AddServerCommand(client_t *client, svscmd_type type, const char *cmd)
{
    int i;
    int reliableSequence;

    if (client->bIsTestClient != 0)
        return;

    reliableSequence = client->reliableSequence;
    i = reliableSequence;

    if (i - client->reliableAcknowledge > 0x3f) {

        int toIndex;
        int from = client->reliableSent + 1;

        toIndex = from;
        while (from <= reliableSequence) {
            int fromSlot = from & 0x7f;
            int fromOff = fromSlot * 0x408;
            int entryType = client->reliableCommandInfo[fromSlot].type;

            if (entryType == 0) {

                from++;
                continue;
            }

            {
                int toSlot = toIndex & 0x7f;

                if (toSlot != fromSlot) {

                    memcpy(&client->reliableCommandInfo[toSlot], &client->reliableCommandInfo[fromSlot], sizeof(svscmd_info_t));
                }
            }
            toIndex++;
            from++;
        }
        reliableSequence = toIndex - 1;
        client->reliableSequence = reliableSequence;
    }

    if (type != 0) {
        int from;
        int foundIdx = -1;

        from = client->reliableSent + 1;
        while (from <= i) {
            int slot = from & 0x7f;
            int entryType = client->reliableCommandInfo[slot].type;

            if (entryType == 0) {
                from++;
                continue;
            }

            {
                char firstChar = cmd[0];
                if (client->reliableCommandInfo[slot].cmd[0] != firstChar) {
                    from++;
                    continue;
                }

                if (firstChar == 'x' || firstChar == 'y' || firstChar == 'z') {
                    from++;
                    continue;
                }

                {
                    int eligible = 0;
                    int partialMatch = 0;

                    switch (firstChar) {
                    case 'C':
                    case 'D':
                    case 'q':
                    case 'r':
                    case 'v':
                    case 'w':
                        eligible = 1;
                        break;
                    default:
                        break;
                    }

                    if (!eligible) {
                        from++;
                        continue;
                    }

                    if (firstChar == 'v' || firstChar == 'w') {
                        partialMatch = 1;
                    }

                    if (partialMatch) {

                        const char *str1 = cmd + 2;
                        const char *str2 = client->reliableCommandInfo[slot].cmd + 2;
                        int matched = 1;

                        while (1) {
                            char c1 = *str1;
                            char c2 = *str2;

                            if (c1 == '\0') {

                                if (c2 == '\0' || c2 == ' ')
                                    break;
                                matched = 0;
                                break;
                            }
                            if (c2 == '\0') {
                                if (c1 == ' ')
                                    break;
                                matched = 0;
                                break;
                            }
                            if (c1 == ' ') {
                                if (c2 == '\0' || c2 == ' ')
                                    break;
                                matched = 0;
                                break;
                            }
                            if (c2 == ' ') {
                                if (c1 == '\0' || c1 == ' ')
                                    break;
                                matched = 0;
                                break;
                            }
                            if (c1 != c2) {
                                matched = 0;
                                break;
                            }
                            str1++;
                            str2++;
                        }

                        if (!matched) {
                            from++;
                            continue;
                        }
                    } else {

                        char *entryStr = client->reliableCommandInfo[slot].cmd + 1;
                        if (strcmp(cmd + 1, entryStr) != 0) {
                            from++;
                            continue;
                        }
                    }
                }
            }

            foundIdx = from;

            if (foundIdx >= 0) {
                int fromCmd = foundIdx + 1;
                int toCmd = foundIdx;

                if (fromCmd <= reliableSequence) {
                    while (fromCmd <= reliableSequence) {
                        int fromSlot2 = fromCmd & 0x7f;
                        int toSlot2 = toCmd & 0x7f;

                        memcpy(&client->reliableCommandInfo[toSlot2], &client->reliableCommandInfo[fromSlot2], sizeof(svscmd_info_t));
                        fromCmd++;
                        toCmd++;
                    }
                }

                reliableSequence--;
                client->reliableSequence = reliableSequence;
                i = reliableSequence;
            }
            break;
        }
    }

    if (i - client->reliableAcknowledge == 0x81) {

        int dumpFrom;

        Com_Printf("===== pending server commands =====\n");

        dumpFrom = client->reliableAcknowledge + 1;
        while (dumpFrom <= client->reliableSequence) {
            int dumpSlot = dumpFrom & 0x7f;
            Com_Printf("cmd %5d: %8d: %s\n", dumpFrom, client->reliableCommandInfo[dumpSlot].time,
                       client->reliableCommandInfo[dumpSlot].cmd);
            dumpFrom++;
        }

        Com_Printf("cmd %5d: %8d: %s\n", i, svs.time, cmd);

        {
            netadr_t addr = client->netchan.remoteAddress;
            NET_OutOfBandPrint(1, addr, "disconnect");
        }

        SV_DelayDropClient(client, "EXE_SERVERCOMMANDOVERFLOW");

        cmd = va("%c \"EXE_SERVERCOMMANDOVERFLOW\"", 0x77);
        type = 1;
        i = client->reliableSequence;
    }

    {
        int newSeq = i + 1;
        client->reliableSequence = newSeq;
        i = newSeq;
    }

    {
        int slot = i & 0x7f;

        MSG_WriteReliableCommandToBuffer(cmd, client->reliableCommandInfo[slot].cmd, 0x400);

        client->reliableCommandInfo[slot].time = svs.time;
        client->reliableCommandInfo[slot].type = type;
    }
}

void SV_VoicePacket(netadr_t from, msg_t *msg)
{
    int qport;
    int i;
    client_t *cl;

    qport = MSG_ReadShort(msg);
    cl = svs.clients;

    for (i = 0; i < sv_maxclients->current.integer; i++, cl++) {
        if (!cl->state) {
            continue;
        }
        if (!NET_CompareBaseAdr(from, cl->netchan.remoteAddress)) {
            continue;
        }
        if (qport != cl->netchan.qport) {
            continue;
        }

        if (from.port != cl->netchan.remoteAddress.port) {
            Com_Printf("SV_ReadPackets: fixing up a translated port\n");
            cl->netchan.remoteAddress.port = from.port;
        }

        if (cl->state == 1) {
            continue;
        }

        cl->lastPacketTime = svs.time;
        if (cl->state <= 3) {
            SV_PreGameUserVoice(cl, msg);
        } else {
            SV_UserVoice(cl, msg);
        }
    }
}

void SVC_Status(netadr_t from)
{
    LargeLocal status_large_local;
    char *status;
    char infostring[0x2000];
    char keywords[0x400];
    char player[0x400];
    char finalString[0x2000];
    int statusLength;
    int i;
    int mod;

#if COD2_FEATURE_NET_HARDENING

    if (NetHardening_SVC_RateLimitAddress(from, svs.time) ||
        NetHardening_SVC_RateLimit(svs.time))
        return;
#endif

    LargeLocal_LargeLocal(&status_large_local, MAX_MSGLEN);
    status = (char *)LargeLocal_GetBuf(&status_large_local);

    strcpy(infostring, Dvar_InfoString(0x404));
    Info_SetValueForKey(infostring, "challenge", SV_Cmd_Argv(1));

    if (Dvar_GetBool("fs_restrict")) {
        Com_sprintf(keywords, sizeof(keywords), "demo %s", Info_ValueForKey(infostring, "sv_keywords"));
        Info_SetValueForKey(infostring, "sv_keywords", keywords);
    }

    status[0] = '\0';
    statusLength = 0;

    for (i = 0; i < sv_maxclients->current.integer; ++i) {
        client_t *cl = &svs.clients[i];
        int newStatusLength;

        if (cl->state <= 1)
            continue;

        SV_GameClientNum(i);

        if (*(int *)imp_gameInitialized)
            G_GetClientScore((int)(cl - svs.clients));

        Com_sprintf(player, sizeof(player), "%i %i \"%s\"\n", 0, cl->ping, cl->name);
        newStatusLength = statusLength + (int)strlen(player);
        if ((unsigned int)newStatusLength > 0x3fff)
            break;

        strcpy(status + statusLength, player);
        statusLength = newStatusLength;
    }

    {
        const char *g_password = Dvar_GetString("g_password");

        if (g_password && g_password[0]) {
            Info_SetValueForKey(infostring, "pswrd", "1");
        } else {
            Info_SetValueForKey(infostring, "pswrd", "0");
        }
    }

    mod = 1;
    {
        const char *fs_game = Dvar_GetString("fs_game");

        if (sv_pure->current.enabled && fs_game && fs_game[0]) {
            const char *referencedIwds = Dvar_GetString("sv_referencedIwdNames");

            mod = 0;
            if (referencedIwds && referencedIwds[0]) {
                int count;

                SV_Cmd_TokenizeString(referencedIwds);
                count = SV_Cmd_Argc();

                for (i = 0; i < count; ++i) {
                    if (!FS_iwIwd(SV_Cmd_Argv(i), "main")) {
                        mod = 1;
                        break;
                    }
                }
            }
        }
    }

    Info_SetValueForKey(infostring, "mod", va("%i", mod));
    Com_sprintf(finalString, sizeof(finalString), "statusResponse\n%s\n%s", infostring, status);
    NET_OutOfBandPrint(1, from, finalString);

    ZN10LargeLocalD1Ev(&status_large_local);
}

void SVC_GameCompleteStatus(netadr_t from)
{
    LargeLocal status_large_local;
    char *status;
    char infostring[0x400];
    char keywords[0x400];
    char player[0x400];
    int statusLength;
    int i;

    LargeLocal_LargeLocal(&status_large_local, MAX_MSGLEN);
    status = (char *)LargeLocal_GetBuf(&status_large_local);

    strcpy(infostring, Dvar_InfoString(0x404));
    Info_SetValueForKey(infostring, "challenge", SV_Cmd_Argv(1));

    if (Dvar_GetBool("fs_restrict")) {
        Com_sprintf(keywords, sizeof(keywords), "demo %s", Info_ValueForKey(infostring, "sv_keywords"));
        Info_SetValueForKey(infostring, "sv_keywords", keywords);
    }

    status[0] = '\0';
    statusLength = 0;

    for (i = 0; i < sv_maxclients->current.integer; ++i) {
        client_t *cl = &svs.clients[i];
        int score;
        int newStatusLength;

        if (cl->state <= 1)
            continue;

        SV_GameClientNum(i);
        score = G_GetClientScore(i);
        Com_sprintf(player, sizeof(player), "%i %i \"%s\"\n", score, cl->ping, cl->name);
        newStatusLength = statusLength + (int)strlen(player);
        if ((unsigned int)newStatusLength > 0x3fff)
            break;

        strcpy(status + statusLength, player);
        statusLength = newStatusLength;
    }

    NET_OutOfBandPrint(1, from,
                       va("gameCompleteStatus\n%s\n%s", infostring, status));

    ZN10LargeLocalD1Ev(&status_large_local);
}

void SVC_Info(netadr_t from)
{
    char response[1024];
    char infostring[1024];
    const char *gamedir;
    const char *password;
    const char *iwdList;
    int privateClients;
    int maxclients;
    int privateCount;
    int clientCount;
    int publicMaxClients;
    int i;
    int value;
    int hw;
    int mod;
    int argc;

#if COD2_FEATURE_NET_HARDENING

    if (NetHardening_SVC_RateLimitAddress(from, svs.time) ||
        NetHardening_SVC_RateLimit(svs.time))
        return;
#endif

    privateClients = sv_privateClients->current.integer;
    privateCount = 0;

    for (i = 0; i < privateClients; ++i) {
        if (svs.clients[i].state >= 2)
            ++privateCount;
    }

    maxclients = sv_maxclients->current.integer;
    clientCount = privateCount;

    for (i = privateClients; i < maxclients; ++i) {
        if (svs.clients[i].state >= 2)
            ++clientCount;
    }

    infostring[0] = '\0';
    Info_SetValueForKey(infostring, "challenge", SV_Cmd_Argv(1));
    Info_SetValueForKey(infostring, "protocol", va("%i", 0x76));
    Info_SetValueForKey(infostring, "hostname", sv_hostname->current.string);
    Info_SetValueForKey(infostring, "mapname", sv_mapname->current.string);

    if (clientCount)
        Info_SetValueForKey(infostring, "clients", va("%i", clientCount));

    publicMaxClients = sv_maxclients->current.integer - (sv_privateClients->current.integer - privateCount);
    if (publicMaxClients > 0)
        Info_SetValueForKey(infostring, "sv_maxclients", va("%i", publicMaxClients));

    Info_SetValueForKey(infostring, "gametype", sv_gametype->current.string);

    if (sv_pure->current.enabled || *(int *)imp_fs_numServerIwds)
        Info_SetValueForKey(infostring, "pure", "1");

    value = sv_minPing->current.integer;
    if (value)
        Info_SetValueForKey(infostring, "minPing", va("%i", value));

    value = sv_maxPing->current.integer;
    if (value)
        Info_SetValueForKey(infostring, "maxPing", va("%i", value));

    gamedir = Dvar_GetString("fs_game");
    if (*gamedir)
        Info_SetValueForKey(infostring, "game", gamedir);

    if (sv_allowAnonymous->current.enabled)
        Info_SetValueForKey(infostring, "sv_allowAnonymous", va("%i", sv_allowAnonymous->current.enabled));

    if (sv_disableClientConsole->current.enabled)
        Info_SetValueForKey(infostring, "con_disabled", va("%i", sv_disableClientConsole->current.enabled));

    password = Dvar_GetString("g_password");
    if (password && *password)
        Info_SetValueForKey(infostring, "pswrd", "1");

    value = Dvar_GetInt("scr_friendlyfire");
    if (value)
        Info_SetValueForKey(infostring, "ff", va("%i", value));

    value = Dvar_GetInt("scr_killcam");
    if (value)
        Info_SetValueForKey(infostring, "kc", va("%i", value));

    if (*(dvar_t **)imp_com_dedicated && (*(dvar_t **)imp_com_dedicated)->current.integer)
        hw = 2;
    else
        hw = 5;

    Info_SetValueForKey(infostring, "hw", va("%i", hw));

    if (sv_pure->current.enabled && !*gamedir) {
        mod = 0;
        iwdList = Dvar_GetString("sv_referencedIwdNames");
        if (*iwdList) {
            SV_Cmd_TokenizeString(iwdList);
            argc = SV_Cmd_Argc();
            for (i = 0; i < argc; ++i) {
                if (!FS_iwIwd(SV_Cmd_Argv(i), (char *)"main")) {
                    mod = 1;
                    break;
                }
            }
        }
    } else {
        mod = 1;
    }

    Info_SetValueForKey(infostring, "mod", va("%i", mod));
    Info_SetValueForKey(infostring, "voice", va("%i", sv_voice->current.enabled));

    I_strncpyz(response, "infoResponse\n", sizeof(response));
    I_strncat(response, sizeof(response), infostring);
    NET_OutOfBandPrint(1, from, response);
}

void SV_ConnectionlessPacket(netadr_t from, msg_t *msg)
{
    const char *c;

    MSG_BeginReading(msg);
    MSG_ReadLong(msg);
    SV_Netchan_AddOOBProfilePacket(msg->cursize);

    c = MSG_ReadStringLine(msg);
    SV_Cmd_TokenizeString(c);
    c = SV_Cmd_Argv(0);
    Com_Printf("[cnx] SV_ConnectionlessPacket cmd='%s'\n", c);

#if COD2_IS_PATCH_13

    if (I_strnicmp(c, "pb_", 3) == 0) {
        PB_HandleServerOobPacket(&from, msg);
        return;
    }
#endif

    if (sv_packet_info->current.enabled)
        Com_Printf("SV packet %s : %s\n", NET_AdrToString(from), c);

    if (I_stricmp(c, "v") == 0) {
        SV_VoicePacket(from, msg);
        return;
    }

    if (I_stricmp(c, "getstatus") == 0) {
        SVC_Status(from);
        return;
    }

    if (I_stricmp(c, "getinfo") == 0) {
        SVC_Info(from);
        return;
    }

    if (I_stricmp(c, "getchallenge") == 0) {
        SV_GetChallenge(from);
        return;
    }

    if (I_stricmp(c, "connect") == 0) {
        SV_DirectConnect(from);
        return;
    }

    if (I_stricmp(c, "ipAuthorize") == 0) {
        SV_AuthorizeIpPacket(from);
        return;
    }

    if (I_stricmp(c, "rcon") == 0) {
        SVC_RemoteCommand(from, msg);
        return;
    }

    I_stricmp(c, "disconnect");
}

void SV_PacketEvent(netadr_t from, msg_t *msg)
{
    int qport;
    int i;
    int netchanResult;
    client_t *cl;

    if (msg->cursize > 3 && *(int *)msg->data == -1) {
        SV_ConnectionlessPacket(from, msg);
        return;
    }

    SV_ResetSkeletonCache();
    MSG_BeginReading(msg);
    MSG_ReadLong(msg);
    qport = (unsigned short)MSG_ReadShort(msg);

    cl = svs.clients;
    for (i = 0; i < sv_maxclients->current.integer; ++i, ++cl) {
        if (!cl->state)
            continue;

        if (!NET_CompareBaseAdr(from, cl->netchan.remoteAddress))
            continue;

        if (qport == cl->netchan.qport)
            break;
    }

    if (i >= sv_maxclients->current.integer) {
        NET_OutOfBandPrint(1, from, "disconnect");
        return;
    }

    if (from.port != cl->netchan.remoteAddress.port) {
        Com_Printf("SV_ReadPackets: fixing up a translated port\n");
        cl->netchan.remoteAddress.port = from.port;
    }

    netchanResult = Netchan_Process(&cl->netchan, msg);
    if (!netchanResult)
        return;

    cl->serverId = MSG_ReadByte(msg);
    cl->messageAcknowledge = MSG_ReadLong(msg);
    SV_PktEvtDbg("", netchanResult, cl->state, cl->serverId, cl->messageAcknowledge);

    if (cl->messageAcknowledge < 0)
        return;

    cl->reliableAcknowledge = MSG_ReadLong(msg);
    if (cl->reliableSequence - cl->reliableAcknowledge > 127) {
        cl->reliableAcknowledge = cl->reliableSequence;
        return;
    }

    SV_Netchan_Decode(cl, msg->data + msg->readcount, msg->cursize - msg->readcount);

    if (cl->state == 1)
        return;

    cl->lastPacketTime = svs.time;
    SV_ExecuteClientMessage(cl, msg);
    *(void **)imp_bgs = 0;
}

void SV_FreeClientScriptId(client_t *cl)
{
    Scr_FreeValue(cl->scriptId);
    cl->scriptId = 0;
}

void SV_RunFrame(void)
{
    SV_ResetSkeletonCache();
    G_RunFrame(*(int *)((char *)&svs + 4));
}

#define COD2_BOT_MAX_CLIENTS 64

typedef struct cod2BotCommandState_s {
    int buttons;
    signed char forwardmove;
    signed char rightmove;
    byte weapon;
    qboolean hasWeapon;
    vec3_t angles;
    qboolean hasAngles;
} cod2BotCommandState_t;

typedef struct cod2BotActionDef_s {
    const char *name;
    int buttonMask;
} cod2BotActionDef_t;

/* These masks are the native CL_CmdButtons/CL_KeyMove usercmd contract. */
static const cod2BotActionDef_t s_botActions[] = {
    { "fire",        0x00001 },
    { "melee",       0x00004 },
    { "activate",    0x00008 },
    { "reload",      0x00010 },
    { "usereload",   0x00020 },
    { "leanleft",    0x00040 },
    { "leanright",   0x00080 },
    { "goprone",     0x00100 },
    { "gocrouch",    0x00200 },
    { "gostand",     0x00400 },
    { "jump",        0x00400 },
    { "ads",         0x01000 },
    { "binoculars",  0x04000 },
    { "holdbreath",  0x08000 },
    { "frag",        0x10000 },
    { "smoke",       0x20000 },
    { NULL,            0x00000 },
};

/*
 * Test clients share client_t with network clients, and that reconstructed
 * layout is pinned to the original 32-bit ABI. Keep bot intent out-of-line so
 * adding browser/server bot support cannot move any reconstructed field.
 */
static cod2BotCommandState_t s_botCmdState[COD2_BOT_MAX_CLIENTS];

static qboolean SV_BotClientNumInRange(int clientNum)
{
    return clientNum >= 0 && clientNum < COD2_BOT_MAX_CLIENTS &&
           svs.clients && sv_maxclients &&
           clientNum < sv_maxclients->current.integer;
}

qboolean SV_BotIsTestClient(int clientNum)
{
    client_t *cl;

    if (!SV_BotClientNumInRange(clientNum))
        return 0;

    cl = &svs.clients[clientNum];
    return cl->state != 0 && cl->bIsTestClient &&
           cl->netchan.remoteAddress.type == NA_BOT;
}

void SV_BotResetClient(int clientNum)
{
    if (clientNum < 0 || clientNum >= COD2_BOT_MAX_CLIENTS)
        return;

    memset(&s_botCmdState[clientNum], 0, sizeof(s_botCmdState[clientNum]));
}

qboolean SV_BotStop(int clientNum)
{
    cod2BotCommandState_t *state;

    if (!SV_BotIsTestClient(clientNum))
        return 0;

    state = &s_botCmdState[clientNum];
    state->buttons = 0;
    state->forwardmove = 0;
    state->rightmove = 0;
    return 1;
}

static signed char SV_BotClampMove(int value)
{
    if (value > 127)
        return 127;
    if (value < -127)
        return -127;
    return (signed char)value;
}

qboolean SV_BotSetMovement(int clientNum, int forward, int right)
{
    cod2BotCommandState_t *state;

    if (!SV_BotIsTestClient(clientNum))
        return 0;

    state = &s_botCmdState[clientNum];
    state->forwardmove = SV_BotClampMove(forward);
    state->rightmove = SV_BotClampMove(right);
    return 1;
}

qboolean SV_BotSetAngles(int clientNum, const vec_t *angles)
{
    cod2BotCommandState_t *state;

    if (!angles || !SV_BotIsTestClient(clientNum))
        return 0;

    state = &s_botCmdState[clientNum];
    state->angles[0] = angles[0];
    state->angles[1] = angles[1];
    state->angles[2] = angles[2];
    state->hasAngles = 1;
    return 1;
}

qboolean SV_BotSetWeapon(int clientNum, int weapon)
{
    cod2BotCommandState_t *state;

    if (weapon < 0 || weapon > 255 || !SV_BotIsTestClient(clientNum))
        return 0;

    state = &s_botCmdState[clientNum];
    state->weapon = (byte)weapon;
    state->hasWeapon = 1;
    return 1;
}

qboolean SV_BotSetAction(int clientNum, const char *action)
{
    const cod2BotActionDef_t *def;
    cod2BotCommandState_t *state;
    qboolean pressed;

    if (!SV_BotIsTestClient(clientNum) || !action ||
        (action[0] != '+' && action[0] != '-') || !action[1]) {
        return 0;
    }

    pressed = action[0] == '+';
    state = &s_botCmdState[clientNum];

    for (def = s_botActions; def->name; ++def) {
        if (!strcmp(action + 1, def->name)) {
            if (pressed)
                state->buttons |= def->buttonMask;
            else
                state->buttons &= ~def->buttonMask;
            return 1;
        }
    }

    return 0;
}

void SV_BotUserMove(client_t *cl)
{
    cod2BotCommandState_t *state;
    playerState_t *ps;
    usercmd_t cmd;
    int clientNum;
    int i;
    const float angleToShort = 65536.0f / 360.0f;

    if (!cl->gentity)
        return;

    clientNum = (int)(cl - svs.clients);
    if (!SV_BotIsTestClient(clientNum))
        return;

    state = &s_botCmdState[clientNum];
    ps = SV_GameClientNum(clientNum);
    if (!ps)
        return;

    memset(&cmd, 0, sizeof(cmd));
    cmd.serverTime = svs.time;
    cmd.buttons = state->buttons;
    cmd.forwardmove = state->forwardmove;
    cmd.rightmove = state->rightmove;
    cmd.weapon = state->hasWeapon ? state->weapon : (byte)ps->weapon;
    cmd.offHandIndex = (byte)ps->offHandIndex;

    for (i = 0; i < 3; ++i) {
        float angle = state->hasAngles ? state->angles[i] : ps->viewangles[i];
        cmd.angles[i] = ((int)(angle * angleToShort) & 0xffff) - ps->delta_angles[i];
    }

    cl->deltaMessage = cl->netchan.outgoingSequence - 1;
    SV_ClientThink(cl, &cmd);
}

void SV_SendServerCommand(client_t *cl, svscmd_type type, const char *fmt, ...)
{
    LargeLocal message_large_local;
    char *message;
    va_list argptr;

    LargeLocal_LargeLocal(&message_large_local, MAX_MSGLEN);
    message = (char *)LargeLocal_GetBuf(&message_large_local);

    va_start(argptr, fmt);
    vsnprintf(message, 0x4000, fmt, argptr);
    va_end(argptr);

    if (cl) {
        SV_AddServerCommand(cl, type, message);
        ZN10LargeLocalD1Ev(&message_large_local);
        return;
    }

    if ((*(dvar_t **)imp_com_dedicated)->current.integer && strncmp(message, "print", 5) == 0) {
        const char *src = message;
        int l = 0;

        while (*src && l <= 0x3fc) {
            unsigned char ch = (unsigned char)*src++;

            if (ch == '\n') {
                string_00edae00[l++] = '\\';
                string_00edae00[l++] = 'n';
            } else if (ch != 0x14 && ch != 0x15) {
                string_00edae00[l++] = (char)ch;
            }
        }
        string_00edae00[l] = '\0';
        Com_Printf("broadcast: %s\n", string_00edae00);
    }

    for (cl = svs.clients; cl < svs.clients + sv_maxclients->current.integer; ++cl) {
        if (cl->state > 2)
            SV_AddServerCommand(cl, type, message);
    }

    ZN10LargeLocalD1Ev(&message_large_local);
}

int SV_FrameWaitMilliseconds(void)
{
    int remaining;
    if (!sv_fps || !(*(const dvar_t **)imp_com_sv_running)->current.enabled)
        return 50;
    remaining = 1000 / sv_fps->current.integer - sv.timeResidual;
    return remaining > 0 ? remaining : 1;
}

void SV_Frame(int msec)
{
    int frameMsec;
    int i;
    int connectedCount;
    int dvarModifiedFlags;
    int timeout;
    int zombieTimeout;
    client_t *cl;
    char mapname[64];
    struct LegacyHacks *legacyHacks;

    legacyHacks = *(struct LegacyHacks **)imp_legacyHacks;
    if (legacyHacks->sv_killserver) {
        Com_Shutdown(legacyHacks->sv_killreason);
        legacyHacks->sv_killserver = 0;
        return;
    }

    if (!(*(const dvar_t **)imp_com_sv_running)->current.enabled) {
        return;
    }

    if ((*(const dvar_t **)imp_cl_paused)->current.integer) {
        connectedCount = 0;
        cl = svs.clients;

        for (i = 0; i < sv_maxclients->current.integer; ++i, ++cl) {
            if (cl->state >= 2) {
                ++connectedCount;
            }
        }

        if (connectedCount - 1 <= 0) {
            Dvar_SetInt(*(const dvar_t **)imp_sv_paused, 1);
            return;
        }

        Dvar_SetInt(*(const dvar_t **)imp_sv_paused, 0);
    }

    frameMsec = 1000 / sv_fps->current.integer;
    sv.timeResidual += msec;

    if (frameMsec > sv.timeResidual) {
        return;
    }

    if (svs.time > 0x70000000) {
        I_strncpyz(mapname, sv_mapname->current.string, sizeof(mapname));
        Com_Shutdown("EXE_SERVERRESTARTTIMEWRAP");
        Cbuf_AddText(va("map %s\n", mapname));
        return;
    }

    if (svs.nextSnapshotEntities >= 0x7ffffffe - svs.numSnapshotEntities) {
        I_strncpyz(mapname, sv_mapname->current.string, sizeof(mapname));
        Com_Shutdown("EXE_SERVERRESTARTMISC\x15numSnapshotEntities");
        Cbuf_AddText(va("map %s\n", mapname));
        return;
    }

    if (svs.nextCachedSnapshotEntities > 0x7fffbffd) {
        I_strncpyz(mapname, sv_mapname->current.string, sizeof(mapname));
        Com_Shutdown("EXE_SERVERRESTARTMISC\x15nextCachedSnapshotEntities");
        Cbuf_AddText(va("map %s\n", mapname));
        return;
    }

    if (svs.nextCachedSnapshotClients > 0x7fffeffd) {
        I_strncpyz(mapname, sv_mapname->current.string, sizeof(mapname));
        Com_Shutdown("EXE_SERVERRESTARTMISC\x15nextCachedSnapshotClients");
        Cbuf_AddText(va("map %s\n", mapname));
        return;
    }

    if (svs.nextArchivedSnapshotFrames > 0x7ffffb4d) {
        I_strncpyz(mapname, sv_mapname->current.string, sizeof(mapname));
        Com_Shutdown("EXE_SERVERRESTARTMISC\x15nextArchivedSnapshotFrames");
        Cbuf_AddText(va("map %s\n", mapname));
        return;
    }

    if (svs.nextArchivedSnapshotBuffer > 0x7dfffffd) {
        I_strncpyz(mapname, sv_mapname->current.string, sizeof(mapname));
        Com_Shutdown("EXE_SERVERRESTARTMISC\x15nextArchivedSnapshotBuffer");
        Cbuf_AddText(va("map %s\n", mapname));
        return;
    }

    if (svs.nextCachedSnapshotFrames > 0x7ffffdfd) {
        I_strncpyz(mapname, sv_mapname->current.string, sizeof(mapname));
        Com_Shutdown("EXE_SERVERRESTARTMISC\x15nextCachedSnapshotFrames");
        Cbuf_AddText(va("map %s\n", mapname));
        return;
    }

    if (svs.nextSnapshotClients >= 0x7ffffffe - svs.numSnapshotClients) {
        I_strncpyz(mapname, sv_mapname->current.string, sizeof(mapname));
        Com_Shutdown("EXE_SERVERRESTARTMISC\x15numSnapshotClients");
        Cbuf_AddText(va("map %s\n", mapname));
        return;
    }

    dvarModifiedFlags = *(int *)imp_dvar_modifiedFlags;
    if (dvarModifiedFlags & 0x404) {
        SV_SetConfigstring(0, Dvar_InfoString(0x404));
        *(int *)imp_dvar_modifiedFlags &= ~0x404;
        dvarModifiedFlags = *(int *)imp_dvar_modifiedFlags;
    }

    if (dvarModifiedFlags & 8) {
        SV_SetConfigstring(1, Dvar_InfoString_Big(8));
        *(int *)imp_dvar_modifiedFlags &= ~8;
        dvarModifiedFlags = *(int *)imp_dvar_modifiedFlags;
    }

    if (dvarModifiedFlags & 0x100) {
        SV_SetConfig(0x8e, 0x60, 0x100);
        *(int *)imp_dvar_modifiedFlags &= ~0x100;
    }

    SV_ResetSkeletonCache();

    cl = svs.clients;
    for (i = 0; i < sv_maxclients->current.integer; ++i, ++cl) {
        if (cl->state && cl->netchan.remoteAddress.type == NA_BOT) {
            SV_BotUserMove(cl);
        }
    }

    cl = svs.clients;
    for (i = 0; i < sv_maxclients->current.integer; ++i, ++cl) {
        int pingCount;
        int pingTotal;
        int frameIndex;
        byte *frame;

        if (cl->state != 4 || !cl->gentity) {
            cl->ping = 999;
            continue;
        }

        pingCount = 0;
        pingTotal = 0;
        frame = (byte *)cl;

        for (frameIndex = 0; frameIndex < 32; ++frameIndex, frame += 0x26c4) {
            int messageAck = ((client_t *)frame)->frames[0].messageAcked;

            if (messageAck > 0) {
                ++pingCount;
                pingTotal += messageAck - ((client_t *)frame)->frames[0].messageSent;
            }
        }

        if (pingCount) {
            int ping = pingTotal / pingCount;
            cl->ping = ping < 1000 ? ping : 999;
        } else {
            cl->ping = 999;
        }
    }

    do {
        sv.timeResidual -= frameMsec;
        svs.time += frameMsec;

        CL_FlushDebugData(1);
        SV_ResetSkeletonCache();
        G_RunFrame(svs.time);
        Scr_SetLoading(0);

        if (frameMsec <= sv.timeResidual) {
            SV_ArchiveSnapshot();
        }
    } while (frameMsec <= sv.timeResidual);

    timeout = svs.time - sv_timeout->current.integer * 1000;
    zombieTimeout = svs.time - sv_zombietime->current.integer * 1000;

    cl = svs.clients;
    for (i = 0; i < sv_maxclients->current.integer; ++i, ++cl) {
        if (cl->lastPacketTime > svs.time) {
            cl->lastPacketTime = svs.time;
        }

        if (cl->bIsTestClient) {
            continue;
        }

        if (cl->state == 1) {
            if (zombieTimeout > cl->lastPacketTime) {
                Com_DPrintf("Going from CS_ZOMBIE to CS_FREE for %s\n", cl->name);
                cl->state = 0;
                continue;
            }
        } else if (cl->state > 1 && timeout > cl->lastPacketTime) {
            ++cl->timeoutCount;
            if (cl->timeoutCount > 5) {
                SV_DropClient(cl, "EXE_TIMEDOUT");
                cl->state = 0;
            }
            continue;
        }

        cl->timeoutCount = 0;
    }

    SV_SendClientMessages();
    SV_ArchiveSnapshot();
    SV_MasterHeartbeat("COD-2");
    return;
}
