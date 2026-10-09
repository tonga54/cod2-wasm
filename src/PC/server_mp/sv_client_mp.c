#include "common_types.h"
#include "imports.h"
#include "cod2_feature_config.h"
#include "pb_public.h"
#include "PC/qcommon/net_hardening.h"
#include <stddef.h>
#include <string.h>

extern ucmd_t ucmds[12];

extern char *ClientConnect(int clientNum, int scriptPersId);
extern Bool Dvar_GetBool(const char *dvarName);
extern int I_strncmp(const char *s0, const char *s1, int n);
extern const char *NET_AdrToString(netadr_t a);
extern qboolean NET_IsLocalAddress(netadr_t adr);
extern void Netchan_Setup(netsrc_t sock, netchan_t *chan, struct netadr_t adr, int qport);
extern void SV_SendClientSnapshot(client_t *client);

extern server_t sv;
extern serverStatic_t svs;

COD2_ASSERT_FIELD(client_t, downloadName, 0x20c68);

static int botport;
extern void *imp_svs;
extern void SV_Cmd_TokenizeString(const char *text_in);
extern Bool SV_Loaded(void);
extern void ClientCommand(int clientNum);
extern void ClientThink(int clientNum);
extern void G_SetLastServerTime(int clientNum, int serverTime);
extern void FS_FCloseFile(fileHandle_t f);
extern void Z_FreeInternal(void *ptr);
extern char *Info_ValueForKey(const char *s, const char *key);
extern qboolean Sys_IsLANAddress(netadr_t adr);
extern void *imp_com_dedicated;
extern int strnicmp(const char *s1, const char *s2, size_t n);
extern int FS_WriteFile(const char *qpath, const void *buffer, int size);
extern const char *FS_LoadedIwdPureChecksums(void);

extern float FX_GetServerVisibility(const vec_t *start, const vec_t *end);
extern void Com_DPrintf(const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern const char *SV_Cmd_Argv(int arg);
extern int SV_Cmd_Argc(void);
extern int atoi(const char *s);
extern const dvar_t *Dvar_RegisterString(const char *dvarName, const char *defaultValue, int flags);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern void ClientUserinfoChanged(int clientNum);
extern int FS_ReadFile(const char *qpath, void **buffer);
extern void FS_FreeFile(void *buffer);
extern const char *Com_Parse(const char **data_p);
extern void Com_SkipRestOfLine(const char **data_p);
extern void SV_FreeClientScriptId(client_t *cl);
extern unsigned int Scr_AllocArray(void);
extern void Z_VirtualFreeInternal(void *ptr);

extern void ClientDisconnect(int clientNum);
extern void SV_SetUserinfo(int index, const char *val);
extern void SV_BotResetClient(int clientNum);
extern int FS_FOpenFileByMode(const char *qpath, fileHandle_t *f, fsMode_t mode);
extern void FS_Printf(fileHandle_t h, const char *fmt, ...);
extern char *I_CleanStr(char *string);
extern void SV_SendServerCommand(client_t *cl, svscmd_type type, const char *fmt, ...);
extern qboolean NET_CompareAdr(netadr_t a, netadr_t b);
extern qboolean NET_StringToAdr(const char *s, netadr_t *a);
extern Bool NET_OutOfBandPrint(netsrc_t sock, netadr_t adr, const char *data);
extern const netadr_t *SV_MasterAddress(void);
extern int rand(void);
extern const char *va(const char *fmt, ...);
extern void SV_Heartbeat_f(void);
extern int I_stricmp(const char *s1, const char *s2);
extern qboolean NET_CompareBaseAdr(netadr_t a, netadr_t b);
extern gentity_t *SV_GentityNum(int num);
extern playerState_t *SV_GameClientNum(int num);
extern void ClientBegin(int clientNum);
extern int sprintf(char *str, const char *format, ...);
extern void LargeLocal_LargeLocal(LargeLocal *ll, int size);
extern void *LargeLocal_GetBuf(LargeLocal *ll);
extern void ZN10LargeLocalD1Ev(LargeLocal *ll);
extern void MSG_Init(msg_t *buf, byte *data, int length);
extern void MSG_WriteByte(msg_t *msg, int c);
extern void MSG_WriteShort(msg_t *msg, int c);
extern void MSG_WriteLong(msg_t *msg, int c);
extern void MSG_WriteString(msg_t *msg, const char *s);
extern void MSG_WriteData(msg_t *buf, const void *data, int length);
extern int MSG_ReadBitsCompress(byte *from, byte *to, int size);
extern int MSG_ReadBits(msg_t *msg, int bits);
extern int MSG_ReadLong(msg_t *msg);
extern char *MSG_ReadString(msg_t *msg);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern qboolean FS_iwIwd(char *iwd, char *base);
extern int FS_SV_FOpenFileRead(const char *filename, fileHandle_t *fp);
extern int FS_Read(void *buffer, int len, fileHandle_t f);
extern void *Z_MallocInternal(int size);
extern void Dvar_SetInt(const dvar_t *dvar, int value);
extern void MSG_WriteBigString(msg_t *sb, const char *s);
extern void MSG_WriteDeltaEntity(msg_t *msg, entityState_t *from, entityState_t *to, qboolean force);
extern void SV_UpdateServerCommandsToClient(client_t *client, msg_t *msg);
extern void SV_SendMessageToClient(msg_t *msg, client_t *client);
extern Bool SV_Netchan_TransmitNextFragment(netchan_t *chan);
extern int MSG_ReadByte(msg_t *msg);
extern int Com_HashKey(char *string, int maxlen);
extern void MSG_SetDefaultUserCmd(playerState_t *ps, usercmd_t *cmd);
extern void MSG_ReadDeltaUsercmdKey(msg_t *msg, int key, usercmd_t *from, usercmd_t *to);
extern Bool BG_IsWeaponValid(const playerState_t *ps, int weaponIndex);

static void SV_SetMuteListByte(client_t *cl, unsigned int clientNum, Bool muted)
{
    *((Bool *)((char *)cl + offsetof(client_t, muteList) + clientNum)) = muted;
}

static inline __attribute__((always_inline)) int SV_ClientNumForClient(client_t *cl)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    return (int)(cl - svs->clients);
}

static inline __attribute__((always_inline)) void SV_CloseDownload(client_t *cl)
{
    int i;

    if (cl->download)
        FS_FCloseFile(cl->download);

    cl->download = 0;
    cl->downloadName[0] = '\0';

    for (i = 0; i < 8; ++i) {
        if (cl->downloadBlocks[i]) {
            Z_FreeInternal(cl->downloadBlocks[i]);
            cl->downloadBlocks[i] = NULL;
        }
    }
}

#if !defined(__EMSCRIPTEN__) && !defined(_MSC_VER)
__asm__(".Lsvexec_fmt: .asciz \"\"\n");
#endif
#if !defined(__EMSCRIPTEN__) && !defined(_MSC_VER)
__asm__(".Lsvexec_msgtype_fmt: .asciz \"\"\n");
#endif
#if !defined(__EMSCRIPTEN__) && !defined(_MSC_VER)
__asm__(".Lsvexec_clicmd_fmt: .asciz \"\"\n");
#endif
#if !defined(__EMSCRIPTEN__) && !defined(_MSC_VER)
__asm__(".Lsvexec_drop_fmt: .asciz \"\"\n");
#endif
void SV_ExecDbg(const char *fmt, int clSid, int svSid, int clState)
{
    (void)fmt;
    (void)clSid;
    (void)svSid;
    (void)clState;
}

#if COD2_IS_PATCH_13
void SV_AuthorizeRequest(struct netadr_t from, int challenge, const char *clientPBGuid);
#else
void SV_AuthorizeRequest(struct netadr_t from, int challenge);
#endif
static qboolean __attribute_regparm__(1) SV_IsBannedGuid(int guid);
void SV_BanGuidBriefly(int guid);
void SV_UnbanClient(const char *name);
void SV_AuthorizeIpPacket(netadr_t from);
void SV_UserinfoChanged(client_t *cl);
void SV_FreeClientScriptPers(void);
void SV_DelayDropClient(client_t *drop, const char *reason);
void SV_SendClientGameState(client_t *client);
void SV_ClientEnterWorld(client_t *client, const dvar_t *(*cmd)[4]);
void SV_DoneDownload_f(client_t *cl);
void SV_RetransmitDownload_f(client_t *cl);
void SV_WriteDownloadToClient(client_t *cl, msg_t *msg);
#if COD2_IS_PATCH_13

extern const dvar_t *sv_wwwDownload;
extern const dvar_t *sv_wwwBaseURL;
extern const dvar_t *sv_wwwDlDisconnected;
qboolean SV_WWWRedirectClient(client_t *cl, msg_t *msg);
void SV_WWWDownload_f(client_t *cl);
#endif
void SV_VerifyIwds_f(client_t *cl);
void SV_ResetPureClient_f(client_t *cl);
void SV_UpdateUserinfo_f(client_t *cl);
void SV_MutePlayer_f(client_t *cl);
void SV_UnmutePlayer_f(client_t *cl);
float SV_FX_GetVisibility(const vec_t *start, const vec_t *end);
void SV_ExecuteClientCommand(client_t *cl, const char *s, qboolean clientOK);
void SV_ClientThink(client_t *cl, usercmd_t *cmd);
void SV_GetChallenge(netadr_t from);
void SV_FreeClient(client_t *cl);
void SV_DropClient(client_t *drop, const char *reason);
void SV_Disconnect_f(client_t *cl);
void SV_BanClient(client_t *cl);
void SV_DirectConnect(netadr_t from);
void SV_FreeClients(void);
void SV_StopDownload_f(client_t *cl);
void SV_NextDownload_f(client_t *cl);
void SV_BeginDownload_f(client_t *cl);
void SV_UserMove(client_t *cl, msg_t *msg, qboolean delta);
void SV_ExecuteClientMessage(client_t *cl, msg_t *msg);
gentity_t *SV_AddTestClient(void);

#if COD2_IS_PATCH_13
void SV_AuthorizeRequest(struct netadr_t from, int challenge, const char *clientPBGuid)
#else
void SV_AuthorizeRequest(struct netadr_t from, int challenge)
#endif
{
    char game[0x400];
    extern void *imp_svs;
    serverStatic_t *svsPtr = (serverStatic_t *)imp_svs;

    if (svsPtr->authorizeAddress.type == 1) {
        return;
    }

    game[0] = '\0';

    const dvar_t *fs_game = Dvar_RegisterString("fs_game", "", 0x101c);

    if (fs_game) {
        const char *val = fs_game->current.string;
        if (val[0] != '\0') {

            strcpy(game, val);
        }
    }

    Com_DPrintf((const char *)"sending getIpAuthorize for %s\n", NET_AdrToString(from));

    Bool allowAnon = Dvar_GetBool((const char *)"sv_allowAnonymous");

#if COD2_IS_PATCH_13

    const char *s;
    if (clientPBGuid == NULL)
        s = va((const char *)"getIpAuthorize %i %i.%i.%i.%i \"%s\" %i", challenge,
               (unsigned int)from.ip[0], (unsigned int)from.ip[1],
               (unsigned int)from.ip[2], (unsigned int)from.ip[3],
               game, (int)(unsigned char)allowAnon);
    else
        s = va((const char *)"getIpAuthorize %i %i.%i.%i.%i \"%s\" %i PB \"%s\"", challenge,
               (unsigned int)from.ip[0], (unsigned int)from.ip[1],
               (unsigned int)from.ip[2], (unsigned int)from.ip[3],
               game, (int)(unsigned char)allowAnon, clientPBGuid);
#else
    const char *s = va((const char *)"getIpAuthorize %i %i.%i.%i.%i %s %i", challenge,
                       (unsigned int)from.ip[0], (unsigned int)from.ip[1],
                       (unsigned int)from.ip[2], (unsigned int)from.ip[3],
                       game, (int)(unsigned char)allowAnon);
#endif

    NET_OutOfBandPrint(1, svsPtr->authorizeAddress, s);
}

static qboolean __attribute_regparm__(1) SV_IsBannedGuid(int guid)
{
    void *file;
    const char *text;
    qboolean banned;

    if (!guid) {
        return 0;
    }

    if (FS_ReadFile("ban.txt", &file) < 0) {
        return 0;
    }

    text = (const char *)file;
    banned = 0;
    while (1) {
        const char *token = Com_Parse(&text);
        if (!token[0]) {
            break;
        }

        if (atoi(token) == guid) {
            banned = 1;
            break;
        }

        Com_SkipRestOfLine(&text);
    }

    FS_FreeFile(file);
    return banned;
}

void SV_BanGuidBriefly(int guid)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    int slot = 0;

    if (svs->tempBans[0].guid) {
        int i;
        for (i = 0; i < 16; i++) {
            if (svs->tempBans[i].banTime < svs->tempBans[slot].banTime) {
                slot = i;
            }

            if (i + 1 == 16) {
                break;
            }

            if (!svs->tempBans[i + 1].guid) {
                slot = i + 1;
                break;
            }
        }
    }

    svs->tempBans[slot].guid = guid;
    svs->tempBans[slot].banTime = svs->time;
}

void SV_UnbanClient(const char *name)
{
    void *file;
    char *text;
    int fileSize;
    char cleanName[64];
    int nameLen;
    int found;

    fileSize = FS_ReadFile("ban.txt", &file);
    if (fileSize < 0) {
        return;
    }

    I_strncpyz(cleanName, name, sizeof(cleanName));
    I_CleanStr(cleanName);
    nameLen = strlen(cleanName);

    text = (char *)file;
    found = 0;
    while (1) {
        char *line = text;
        const char *token = Com_Parse((const char **)&text);
        qboolean removeLine;

        if (!token[0]) {
            break;
        }

        while (*text && *text <= ' ') {
            ++text;
        }

        removeLine = (strnicmp(text, cleanName, nameLen) == 0 &&
                      (text[nameLen] == '\r' || text[nameLen] == '\n'));

        Com_SkipRestOfLine((const char **)&text);
        if (removeLine) {
            int removed = text - line;
            int tail = fileSize - (text - (char *)file) + 1;

            ++found;
            memmove(line, text, tail);
            fileSize -= removed;
            text = line;
        }
    }

    FS_WriteFile("ban.txt", file, fileSize);
    FS_FreeFile(file);

    if (found) {
        Com_Printf("unbanned %i user(s) named %s\n", found, cleanName);
    } else {
        Com_Printf("no banned user has name %s\n", cleanName);
    }
}

void SV_AuthorizeIpPacket(netadr_t from)
{
    serverStatic_t *svsPtr = (serverStatic_t *)imp_svs;
    int challenge;
    int i;
    const char *status;
    const char *reason;
    challenge_t *challengeInfo;

    if (!NET_CompareBaseAdr(from, svsPtr->authorizeAddress)) {
        Com_Printf("SV_AuthorizeIpPacket: not from authorize server\n");
        return;
    }

    challenge = atoi(SV_Cmd_Argv(1));

    for (i = 0; i < 1024; ++i) {
        if (svsPtr->challenges[i].challenge == challenge) {
            break;
        }
    }

    if (i == 1024) {
        Com_Printf("SV_AuthorizeIpPacket: challenge not found\n");
        return;
    }

    challengeInfo = &svsPtr->challenges[i];
    challengeInfo->pingTime = svsPtr->time;

    status = SV_Cmd_Argv(2);
    reason = SV_Cmd_Argv(3);

    if (!I_stricmp(status, "demo")) {
        if (Dvar_GetBool("fs_restrict")) {
            NET_OutOfBandPrint(NS_SERVER, challengeInfo->adr,
                               va("challengeResponse %i", challengeInfo->challenge));
            return;
        }

        NET_OutOfBandPrint(NS_SERVER, challengeInfo->adr, "error\nEXE_ERR_NOT_A_DEMO_SERVER");
        memset(challengeInfo, 0, sizeof(*challengeInfo));
        return;
    }

    if (!I_stricmp(status, "accept")) {
        int guid = atoi(SV_Cmd_Argv(4));
        challengeInfo->guid = guid;

        if (SV_IsBannedGuid(guid)) {
            Com_Printf("rejected connection from permanently banned GUID %i\n", challengeInfo->guid);
            NET_OutOfBandPrint(NS_SERVER, challengeInfo->adr, "error\n\x15You are permanently banned from this server");
            memset(challengeInfo, 0, sizeof(*challengeInfo));
            return;
        }

        if (challengeInfo->guid) {
            tempBanSlot_t *ban;
            tempBanSlot_t *banEnd;
            int now = svsPtr->time;
            float kickBanTime = (*(const dvar_t **)imp_sv_kickBanTime)->current.value * 1000.0f;

            for (ban = svsPtr->tempBans, banEnd = svsPtr->tempBans + 16; ban != banEnd; ++ban) {
                if (ban->guid == challengeInfo->guid && (float)(now - ban->banTime) <= kickBanTime) {
                    Com_Printf("rejected connection from temporarily banned GUID %i\n", challengeInfo->guid);
                    NET_OutOfBandPrint(NS_SERVER, challengeInfo->adr, "error\n\x15You are temporarily banned from this server");
                    memset(challengeInfo, 0, sizeof(*challengeInfo));
                    return;
                }
            }
        }

        if (challengeInfo->connected) {
            return;
        }

        NET_OutOfBandPrint(NS_SERVER, challengeInfo->adr,
                           va("challengeResponse %i", challengeInfo->challenge));
        return;
    }

    if (!I_stricmp(status, "deny")) {
        const char *message;

        if (!reason || !*reason) {
            message = "error\nEXE_ERR_CDKEY_IN_USE";
        } else if (!I_stricmp(reason, "CLIENT_UNKNOWN_TO_AUTH")) {
            message = "needcdkey";
        } else if (!I_stricmp(reason, "BAD_CDKEY")) {
            message = "needcdkey";
        } else if (!I_stricmp(reason, "INVALID_CDKEY")) {
            message = "error\nEXE_ERR_CDKEY_IN_USE";
        } else if (!I_stricmp(reason, "BANNED_CDKEY")) {
            message = "error\nEXE_ERR_BAD_CDKEY";
        } else {
            message = "error\nEXE_ERR_BAD_CDKEY";
        }

        NET_OutOfBandPrint(NS_SERVER, challengeInfo->adr, message);
        memset(challengeInfo, 0, sizeof(*challengeInfo));
        return;
    }

    if (reason && *reason) {
        char ret[1024];

        sprintf(ret, "error\n%s", reason);
        NET_OutOfBandPrint(NS_SERVER, challengeInfo->adr, ret);
    } else {
        NET_OutOfBandPrint(NS_SERVER, challengeInfo->adr, "error\nEXE_ERR_BAD_CDKEY");
    }

    memset(challengeInfo, 0, sizeof(*challengeInfo));
}

void SV_UserinfoChanged(client_t *cl)
{
    const char *value;
    int snaps;

    I_strncpyz(cl->name, Info_ValueForKey(cl->userinfo, "name"), sizeof(cl->name));

    if (Sys_IsLANAddress(cl->netchan.remoteAddress) &&
        (*(const dvar_t **)imp_com_dedicated)->current.integer != 2) {
        cl->rate = 99999;
    } else {
        value = Info_ValueForKey(cl->userinfo, "rate");
        if (*value) {
            cl->rate = atoi(value);
            if (cl->rate < 1000) {
                cl->rate = 1000;
            } else if (cl->rate > 90000) {
                cl->rate = 90000;
            }
        } else {
            cl->rate = 5000;
        }
    }

    value = Info_ValueForKey(cl->userinfo, "snaps");
    if (*value) {
        snaps = atoi(value);
        if (snaps <= 0) {
            cl->snapshotMsec = 1000;
        } else if (snaps > 30) {
            cl->snapshotMsec = 33;
        } else {
            cl->snapshotMsec = 1000 / snaps;
        }
    } else {
        cl->snapshotMsec = 50;
    }

    cl->sendVoice = atoi(Info_ValueForKey(cl->userinfo, "cl_voice")) > 0;
    if (cl->rate <= 4999) {
        cl->sendVoice = 0;
    }

#if COD2_IS_PATCH_13

    value = Info_ValueForKey(cl->userinfo, "cl_wwwDownload");
    cl->wwwOk = (*value && atoi(value) != 0);
#endif
}

void SV_FreeClientScriptPers(void)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    client_t *cl = svs->clients;
    int maxClients = (*(const dvar_t **)imp_sv_maxclients)->current.integer;
    int i;

    for (i = 0; i < maxClients; i++, cl++) {
        if (cl->state > 1) {
            SV_FreeClientScriptId(cl);
            cl->scriptId = (unsigned short)Scr_AllocArray();
        }
    }
}

void SV_DelayDropClient(client_t *drop, const char *reason)
{
    if (drop->state == 1)
        return;
    if (drop->dropReason)
        return;
    drop->dropReason = reason;
}

void SV_SendClientGameState(client_t *client)
{
    LargeLocal msgBuffer_large_local;
    byte *msgBuffer;
    msg_t msg;
    entityState_t nullstate;
    server_t *sv;
    serverStatic_t *svs;
    int i;
    int clientNum;

    LargeLocal_LargeLocal(&msgBuffer_large_local, MAX_MSG_DECOMPRESS_BYTES);
    msgBuffer = (byte *)LargeLocal_GetBuf(&msgBuffer_large_local);

    {
        netchan_t *nc = &client->netchan;
        while (client->state && client->netchan.unsentFragments)
            SV_Netchan_TransmitNextFragment(nc);
    }

    Com_DPrintf("SV_SendClientGameState() for %s\n", client->name);
    Com_DPrintf("Going from CS_CONNECTED to CS_PRIMED for %s\n", client->name);

    client->state = 3;
    client->pureAuthentic = 0;
    client->gamestateMessageNum = client->netchan.outgoingSequence;

    MSG_Init(&msg, msgBuffer, MAX_MSG_DECOMPRESS_BYTES);
    MSG_WriteLong(&msg, client->lastClientCommand);
    SV_UpdateServerCommandsToClient(client, &msg);
    MSG_WriteByte(&msg, 1);
    MSG_WriteLong(&msg, client->reliableSequence);

    {
        server_t *svcfg = (server_t *)imp_sv;
        for (i = 0; i < 2048; ++i) {
            if (svcfg->configstrings[i][0]) {
                MSG_WriteByte(&msg, 2);
                MSG_WriteShort(&msg, i);
                MSG_WriteBigString(&msg, svcfg->configstrings[i]);
            }
        }
    }

    memset(&nullstate, 0, sizeof(nullstate));

    sv = (server_t *)imp_sv;
    for (i = 0; i < 1024; ++i) {
        entityState_t *baseline = (entityState_t *)&sv->svEntities[i].baseline;

        if (baseline->number) {
            MSG_WriteByte(&msg, 3);
            MSG_WriteDeltaEntity(&msg, &nullstate, baseline, 1);
        }
    }

    MSG_WriteByte(&msg, 7);

    svs = (serverStatic_t *)imp_svs;
    clientNum = client - svs->clients;
    MSG_WriteLong(&msg, clientNum);
    MSG_WriteLong(&msg, sv->checksumFeed);
    MSG_WriteByte(&msg, 7);

    Com_DPrintf("Sending %i bytes in gamestate to client: %i\n", msg.cursize, clientNum);
    SV_SendMessageToClient(&msg, client);

    ZN10LargeLocalD1Ev(&msgBuffer_large_local);
}

void SV_ClientEnterWorld(client_t *client, const dvar_t *(*cmd)[4])
{
    int clientNum;
    gentity_t *ent;

    Com_DPrintf("Going from CS_PRIMED to CS_ACTIVE for %s\n", client->name);
    client->state = 4;

    clientNum = client - svs.clients;
    ent = SV_GentityNum(clientNum);
    ent->s.number = clientNum;
    client->gentity = (unsigned char *(*)[16])ent;
    client->deltaMessage = -1;
    {
        serverStatic_t *svs = (serverStatic_t *)imp_svs;
        client->nextSnapshotTime = svs->time;
        client->lastUsercmd = *(usercmd_t *)cmd;
        ClientBegin(client - svs->clients);
    }
}

void SV_DoneDownload_f(client_t *cl)
{
    Com_DPrintf("clientDownload: %s Done\n", cl->name);
    SV_SendClientGameState(cl);
}

void SV_RetransmitDownload_f(client_t *cl)
{
    int block = atoi(SV_Cmd_Argv(1));
    if (block == cl->downloadClientBlock)
        cl->downloadXmitBlock = block;
}

void SV_WriteDownloadToClient(client_t *cl, msg_t *msg)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    char errorMessage[1024];
    int rate;
    int maxRate;
    int blocksThisFrame;
    int sent;
    int block;
    int clientNum;

#if COD2_IS_PATCH_13
    if (!cl->downloadName[0] || cl->clientDownloadingWWW)
        return;
#else
    if (!cl->downloadName[0])
        return;
#endif

    if (!cl->download) {
#if COD2_IS_PATCH_13

        if (sv_wwwDownload->current.enabled && cl->wwwOk) {
            if (!cl->wwwFallback) {
                if (SV_WWWRedirectClient(cl, msg))
                    return;
            } else {
                cl->wwwFallback = 0;
            }
        }
        cl->downloadingWWW = 0;
#endif
        clientNum = SV_ClientNumForClient(cl);
        Com_Printf("clientDownload: %d : begining \"%s\"\n", clientNum, cl->downloadName);

        if (FS_iwIwd(cl->downloadName, (char *)"main")) {
            Com_Printf("clientDownload: %d : \"%s\" cannot download iwd files\n", clientNum, cl->downloadName);
            Com_sprintf(errorMessage, sizeof(errorMessage), "EXE_CANTAUTODLGAMEIWD\x15%s", cl->downloadName);
            goto send_error;
        }

        if (!(*(const dvar_t **)imp_sv_allowDownload)->current.enabled) {
            Com_Printf("clientDownload: %d : \"%s\" download disabled", clientNum, cl->downloadName);
            if ((*(const dvar_t **)imp_sv_pure)->current.enabled)
                Com_sprintf(errorMessage, sizeof(errorMessage), "EXE_AUTODL_SERVERDISABLED_PURE\x15%s", cl->downloadName);
            else
                Com_sprintf(errorMessage, sizeof(errorMessage), "EXE_AUTODL_SERVERDISABLED\x15%s", cl->downloadName);
            goto send_error;
        }

        cl->downloadSize = FS_SV_FOpenFileRead(cl->downloadName, &cl->download);
        if (cl->downloadSize <= 0) {
            Com_Printf("clientDownload: %d : \"%s\" file not found on server\n", clientNum, cl->downloadName);
            Com_sprintf(errorMessage, sizeof(errorMessage), "EXE_AUTODL_FILENOTONSERVER\x15%s", cl->downloadName);
            goto send_error;
        }

        cl->downloadXmitBlock = 0;
        cl->downloadClientBlock = 0;
        cl->downloadCurrentBlock = 0;
        cl->downloadCount = 0;
        cl->downloadEOF = 0;
    }

    while (cl->downloadCurrentBlock - cl->downloadClientBlock <= 7) {
        if (cl->downloadCount == cl->downloadSize) {
            if (cl->downloadEOF)
                break;

            if (cl->downloadCurrentBlock - cl->downloadClientBlock > 7)
                break;

            block = cl->downloadCurrentBlock % 8;
            cl->downloadBlockSize[block] = 0;
            cl->downloadCurrentBlock++;
            cl->downloadEOF = 1;
            break;
        }

        block = cl->downloadCurrentBlock % 8;
        if (!cl->downloadBlocks[block])
            cl->downloadBlocks[block] = (unsigned char *)Z_MallocInternal(2048);

        cl->downloadBlockSize[block] = FS_Read(cl->downloadBlocks[block], 2048, cl->download);
        if (cl->downloadBlockSize[block] < 0) {
            cl->downloadCount = cl->downloadSize;
            break;
        }

        cl->downloadCount += cl->downloadBlockSize[block];
        cl->downloadCurrentBlock++;
    }

    if (cl->downloadClientBlock == cl->downloadCurrentBlock)
        return;

    rate = cl->rate;
    maxRate = (*(const dvar_t **)imp_sv_maxRate)->current.integer;
    if (maxRate) {
        if (maxRate <= 999) {
            Dvar_SetInt(*(const dvar_t **)imp_sv_maxRate, 1000);
            maxRate = (*(const dvar_t **)imp_sv_maxRate)->current.integer;
        }
        if (rate > maxRate)
            rate = maxRate;
    }

    if (rate) {
        blocksThisFrame = (((rate * cl->snapshotMsec) / 1000) + 2048) / 2048;
        if (blocksThisFrame <= 0)
            return;
    } else {
        blocksThisFrame = 1;
    }

    sent = 0;
    while (cl->downloadClientBlock != cl->downloadCurrentBlock) {
        if (cl->downloadCurrentBlock == cl->downloadXmitBlock) {
            if (svs->time - cl->downloadSendTime <= 1000)
                return;
            cl->downloadXmitBlock = cl->downloadClientBlock;
        }

        block = cl->downloadXmitBlock % 8;

        MSG_WriteByte(msg, 5);
        MSG_WriteShort(msg, cl->downloadXmitBlock);
        if (!cl->downloadXmitBlock)
            MSG_WriteLong(msg, cl->downloadSize);
        MSG_WriteShort(msg, cl->downloadBlockSize[block]);
        if (cl->downloadBlockSize[block])
            MSG_WriteData(msg, cl->downloadBlocks[block], cl->downloadBlockSize[block]);

        Com_DPrintf("clientDownload: %d : writing block %d\n", SV_ClientNumForClient(cl), cl->downloadXmitBlock);
        cl->downloadXmitBlock++;
        cl->downloadSendTime = svs->time;

        ++sent;
        if (sent >= blocksThisFrame)
            return;
    }

    return;

send_error:
    MSG_WriteByte(msg, 5);
    MSG_WriteShort(msg, 0);
    MSG_WriteLong(msg, -1);
    MSG_WriteString(msg, errorMessage);
    cl->downloadName[0] = '\0';
}

#if COD2_IS_PATCH_13

qboolean SV_WWWRedirectClient(client_t *cl, msg_t *msg)
{
    fileHandle_t handle;
    int size;

    size = FS_SV_FOpenFileRead(cl->downloadName, &handle);
    if (size != 0) {
        FS_FCloseFile(handle);
        I_strncpyz(cl->downloadURL,
                   va("%s/%s", sv_wwwBaseURL->current.string, cl->downloadName),
                   sizeof(cl->downloadURL));
        Com_Printf("Redirecting client '%s' to %s\n", cl->name, cl->downloadURL);
        cl->downloadingWWW = 1;
        MSG_WriteByte(msg, 5);
        MSG_WriteShort(msg, -1);
        MSG_WriteString(msg, cl->downloadURL);
        MSG_WriteLong(msg, size);
        MSG_WriteLong(msg, sv_wwwDlDisconnected->current.enabled != 0);
        cl->downloadName[0] = '\0';
        return 1;
    }

    Com_Printf("ERROR: Client '%s': couldn't extract file size for %s\n",
               cl->name, cl->downloadName);
    return 0;
}

void SV_WWWDownload_f(client_t *cl)
{
    const char *subcmd = SV_Cmd_Argv(1);

    if (!cl->downloadingWWW) {
        Com_Printf("SV_WWWDownload: unexpected wwwdl '%s' for client '%s'\n", subcmd, cl->name);
        SV_DropClient(cl, "PC_PATCH_1_1_UNEXPECTEDDOWLOADMESSAGE");
        return;
    }

    if (!I_stricmp(subcmd, "ack")) {
        if (cl->clientDownloadingWWW)
            Com_Printf("WARNING: dupe wwwdl ack from client '%s'\n", cl->name);
        cl->clientDownloadingWWW = 1;
        return;
    }

    if (!I_stricmp(subcmd, "bbl8r")) {
        SV_DropClient(cl, "PC_PATCH_1_1_DOWNLOADDISCONNECTED");
        return;
    }

    if (!cl->clientDownloadingWWW) {
        Com_Printf("SV_WWWDownload: unexpected wwwdl '%s' for client '%s'\n", subcmd, cl->name);
        SV_DropClient(cl, "PC_PATCH_1_1_UNEXPECTEDDOWLOADMESSAGE");
        return;
    }

    if (!I_stricmp(subcmd, "done")) {
        cl->download = 0;
        cl->downloadName[0] = '\0';
        cl->clientDownloadingWWW = 0;
        return;
    }

    if (I_stricmp(subcmd, "fail")) {
        if (I_stricmp(subcmd, "chkfail")) {
            Com_Printf("SV_WWWDownload: unknown wwwdl subcommand '%s' for client '%s'\n", subcmd, cl->name);
            SV_DropClient(cl, "PC_PATCH_1_1_UNEXPECTEDDOWLOADMESSAGE");
            return;
        }
        Com_Printf("WARNING: client '%s' reports that the redirect download for '%s' had wrong checksum.\n",
                   cl->name, cl->downloadName);
        Com_Printf("         you should check your download redirect configuration.\n");
        cl->download = 0;
        cl->downloadName[0] = '\0';
        cl->clientDownloadingWWW = 0;
        cl->wwwFallback = 1;
    } else {
        cl->download = 0;
        cl->downloadName[0] = '\0';
        cl->clientDownloadingWWW = 0;
        cl->wwwFallback = 1;
        Com_Printf("Client '%s' reported that the http download of '%s' failed, falling back to a server download\n",
                   cl->name, cl->downloadName);
    }

    SV_SendClientGameState(cl);
}
#endif

void SV_VerifyIwds_f(client_t *cl)
{
    int clientChkSum[1024];
    int serverChkSum[1024];
    int nClientIwds = -1;
    int nServerIwds;
    int i;
    int j;
    int checksum;

    nServerIwds = SV_Cmd_Argc();
    if (nServerIwds <= 1 || SV_Cmd_Argv(1)[0] != '@') {
        cl->pureAuthentic = 2;
        return;
    }

    if (nServerIwds > 2) {
        for (i = 2; i < nServerIwds; ++i)
            clientChkSum[i - 2] = atoi(SV_Cmd_Argv(i));

        nClientIwds = nServerIwds - 3;

        for (i = 0; i < nClientIwds; ++i) {
            for (j = 0; j < nClientIwds; ++j) {
                if (i != j && clientChkSum[i] == clientChkSum[j]) {
                    cl->pureAuthentic = 2;
                    return;
                }
            }
        }
    }

    SV_Cmd_TokenizeString(FS_LoadedIwdPureChecksums());
    nServerIwds = SV_Cmd_Argc();
    if (nServerIwds > 1024)
        nServerIwds = 1024;

    for (i = 0; i < nServerIwds; ++i)
        serverChkSum[i] = atoi(SV_Cmd_Argv(i));

    if (nClientIwds > 0) {
        for (i = 0; i < nClientIwds; ++i) {
            int found = 0;
            for (j = 0; j < nServerIwds; ++j) {
                if (clientChkSum[i] == serverChkSum[j]) {
                    found = 1;
                    break;
                }
            }

            if (!found) {
                cl->pureAuthentic = 2;
                return;
            }
        }
    }

    if (nClientIwds < 0) {
        cl->pureAuthentic = 2;
        return;
    }

    checksum = sv.checksumFeed;
    for (i = 0; i < nClientIwds; ++i)
        checksum ^= clientChkSum[i];

    checksum ^= nClientIwds;
    if (checksum != clientChkSum[nClientIwds]) {
        cl->pureAuthentic = 2;
        return;
    }

    cl->pureAuthentic = 1;
}

void SV_ResetPureClient_f(client_t *cl)
{
    cl->pureAuthentic = 0;
}

void SV_UpdateUserinfo_f(client_t *cl)
{
    I_strncpyz(cl->userinfo, SV_Cmd_Argv(1), sizeof(cl->userinfo));
    SV_UserinfoChanged(cl);
    ClientUserinfoChanged((int)(cl - svs.clients));
}

void SV_MutePlayer_f(client_t *cl)
{
    int clientNum = atoi(SV_Cmd_Argv(1));

    if ((unsigned int)clientNum > 0x40) {
        Com_Printf("Invalid mute client %i\n", clientNum);
        return;
    }

    cl->muteList[clientNum] = 1;
}

void SV_UnmutePlayer_f(client_t *cl)
{
    int clientNum = atoi(SV_Cmd_Argv(1));

    if ((unsigned int)clientNum > 0x40) {
        Com_Printf("Invalid unmute client %i\n", clientNum);
        return;
    }

    cl->muteList[clientNum] = 0;
}

float SV_FX_GetVisibility(const vec_t *start, const vec_t *end)
{
    return FX_GetServerVisibility(start, end);
}

void SV_ExecuteClientCommand(client_t *cl, const char *s, qboolean clientOK)
{
    ucmd_t *cmd;

    SV_Cmd_TokenizeString(s);

#if COD2_IS_PATCH_13

    if (!strcmp(SV_Cmd_Argv(0), "wwwdl")) {
        SV_WWWDownload_f(cl);
        return;
    }
#endif

    for (cmd = ucmds; cmd->name; ++cmd) {
        if (!strcmp(SV_Cmd_Argv(0), cmd->name)) {
            ((void (*)(client_t *))cmd->func)(cl);
            break;
        }
    }

    if (clientOK && !cmd->name && SV_Loaded())
        ClientCommand(SV_ClientNumForClient(cl));
}

void SV_ClientThink(client_t *cl, usercmd_t *cmd)
{
    cl->lastUsercmd = *cmd;

    if (cl->state == 4) {
        int clientNum = SV_ClientNumForClient(cl);
        G_SetLastServerTime(clientNum, cmd->serverTime);
        ClientThink(clientNum);
    }
}

void SV_GetChallenge(netadr_t from)
{
    serverStatic_t *svsLocal = (serverStatic_t *)imp_svs;
    challenge_t *challenge;
    int oldest;
    int oldestTime;
    int i;

    oldest = 0;
    oldestTime = 0x7fffffff;

    for (i = 0; i < 1024; ++i) {
        challenge = &svsLocal->challenges[i];

        if (!challenge->connected && NET_CompareAdr(from, challenge->adr))
            goto challenge_found;

        if (challenge->time < oldestTime) {
            oldest = i;
            oldestTime = challenge->time;
        }
    }

    challenge = &svsLocal->challenges[oldest];
    challenge->challenge = ((rand() << 16) ^ rand()) ^ svsLocal->time;
    challenge->adr = from;
    challenge->firstTime = svsLocal->time;
    challenge->firstPing = 0;
    challenge->time = svsLocal->time;
    challenge->connected = 0;
    i = oldest;

challenge_found:
    if (!(*(const dvar_t **)imp_net_lanauthorize)->current.enabled && Sys_IsLANAddress(from)) {
        challenge->pingTime = svsLocal->time;
        NET_OutOfBandPrint(NS_SERVER, from, va("challengeResponse %i", challenge->challenge));
        return;
    }

    if (svsLocal->authorizeAddress.ip[0] == 0 && svsLocal->authorizeAddress.type != NA_BAD) {
        Com_Printf("Resolving %s\n", "cod2master.activision.com");
        if (!NET_StringToAdr("cod2master.activision.com", &svsLocal->authorizeAddress)) {
            Com_Printf("Couldn't resolve address\n");
            return;
        }

        svsLocal->authorizeAddress.port = (unsigned short)((20966 >> 8) | (20966 << 8));
        Com_Printf("%s resolved to %i.%i.%i.%i:%i\n", "cod2master.activision.com",
                   svsLocal->authorizeAddress.ip[0], svsLocal->authorizeAddress.ip[1],
                   svsLocal->authorizeAddress.ip[2], svsLocal->authorizeAddress.ip[3], 20966);
    }

    if (svsLocal->time - challenge->firstTime <= 5000 || NET_CompareAdr(from, *SV_MasterAddress())) {
#if COD2_IS_PATCH_13

        SV_AuthorizeRequest(from, challenge->challenge, NULL);
#else
        SV_AuthorizeRequest(from, challenge->challenge);
#endif
        return;
    }

    Com_DPrintf("authorize server timed out\n");
    challenge->pingTime = svsLocal->time;
    NET_OutOfBandPrint(NS_SERVER, challenge->adr, va("challengeResponse %i", challenge->challenge));
}

void SV_FreeClient(client_t *cl)
{
    int clientNum = SV_ClientNumForClient(cl);

    SV_CloseDownload(cl);

    if (SV_Loaded()) {
        ClientDisconnect(clientNum);
    }

    SV_BotResetClient(clientNum);
    SV_SetUserinfo(clientNum, "");
    SV_FreeClientScriptId(cl);
}

void SV_DropClient(client_t *drop, const char *reason)
{
    serverStatic_t *svsPtr = (serverStatic_t *)imp_svs;
    int clientNum;
    int i;
    int maxClients;

    if (drop->state == 1)
        return;

    drop->dropReason = NULL;
    Com_Printf("[cnx] dropping client %d: %s (serverTime=%d lastPacket=%d)\n",
               SV_ClientNumForClient(drop), reason, svsPtr->time, drop->lastPacketTime);
    SV_FreeClient(drop);

    Com_DPrintf("Going to CS_ZOMBIE for %s\n", drop->name);
    drop->state = 1;

    if (!drop->gentity) {
        for (i = 0; i < 1024; ++i) {
            if (NET_CompareAdr(svsPtr->challenges[i].adr, drop->netchan.remoteAddress)) {
                svsPtr->challenges[i].connected = 0;
                break;
            }
        }
    }

    clientNum = SV_ClientNumForClient(drop);

    if (I_stricmp(reason, "EXE_DISCONNECTED") != 0) {
        SV_SendServerCommand(NULL, SV_CMD_CAN_IGNORE, "%c \"\x15%s^7 \x14%s\"", 0x65, drop->name, reason);
    }

    Com_Printf("%i:%s %s\n", clientNum, drop->name, reason);
    SV_SendServerCommand(NULL, SV_CMD_RELIABLE, "%c %d", 0x4a, clientNum);
    SV_SendServerCommand(drop, SV_CMD_RELIABLE, "%c \"%s\"", 0x77, reason);

    maxClients = (*(const dvar_t **)imp_sv_maxclients)->current.integer;
    if (maxClients > 0) {
        for (i = 0; i < maxClients; ++i) {
            if (svsPtr->clients[i].state > 1)
                return;
        }
        SV_Heartbeat_f();
    } else if (maxClients == 0) {
        SV_Heartbeat_f();
    }
}

void SV_Disconnect_f(client_t *cl)
{
    SV_DropClient(cl, "EXE_DISCONNECTED");
}

void SV_BanClient(client_t *cl)
{
    fileHandle_t file;
    char cleanName[64];

    if (cl->netchan.remoteAddress.type == NA_LOOPBACK) {
        SV_SendServerCommand(NULL, 0, "%c \"EXE_CANNOTKICKHOSTPLAYER\"", 0x65);
        return;
    }

    if (!cl->guid) {
        Com_Printf("Can't ban user, GUID is 0\n");
        return;
    }

    if (SV_IsBannedGuid(cl->guid)) {
        Com_Printf("This GUID (%i) is already banned\n", cl->guid);
        return;
    }

    if (FS_FOpenFileByMode("ban.txt", &file, 2) < 0) {
        return;
    }

    I_strncpyz(cleanName, cl->name, sizeof(cleanName));
    I_CleanStr(cleanName);
    FS_Printf(file, "%i %s\r\n", cl->guid, cleanName);
    FS_FCloseFile(file);

    SV_DropClient(cl, "EXE_PLAYERKICKED");
    cl->lastPacketTime = svs.time;
}

void SV_DirectConnect(netadr_t from)
{
    char userinfo[1024];
    int version;
    int challenge;
    int qport;
    int guid;
    int ping;
    int challengeIndex;
    int clientNum;
    int maxClients;
    int startIndex;
    int connectedClients;
    const char *denied;
    client_t *cl;
    serverStatic_t *svs = (serverStatic_t *)imp_svs;

    Com_DPrintf("SV_DirectConnect()\n");

    I_strncpyz(userinfo, SV_Cmd_Argv(1), sizeof(userinfo));

    version = atoi(Info_ValueForKey(userinfo, "protocol"));
    if (version != 0x76) {
        NET_OutOfBandPrint((netsrc_t)1, from, va("error\nEXE_SERVER_IS_DIFFERENT_VER\x15%s\n", "1.3"));
        Com_DPrintf("    rejected connect from protocol version %i (should be %i)\n", version, 0x76);
        return;
    }

    challenge = atoi(Info_ValueForKey(userinfo, "challenge"));
    qport = atoi(Info_ValueForKey(userinfo, "qport"));

    maxClients = (*(const dvar_t **)imp_sv_maxclients)->current.integer;
    for (clientNum = 0, cl = svs->clients; clientNum < maxClients; ++clientNum, ++cl) {
        if (NET_CompareBaseAdr(from, cl->netchan.remoteAddress) &&
            (qport == cl->netchan.qport || from.port == cl->netchan.remoteAddress.port)) {
            int reconnectMsec;

            reconnectMsec = (*(const dvar_t **)imp_sv_reconnectlimit)->current.integer;
            reconnectMsec = reconnectMsec * 1000;
            if (svs->time - cl->lastConnectTime < reconnectMsec) {
                Com_DPrintf("%s:reconnect rejected : too soon\n", NET_AdrToString(from));
                return;
            }
            break;
        }
    }

    if (NET_IsLocalAddress(from)) {
        guid = 0;
        ping = 0;
        challengeIndex = -1;
    } else {
        challenge_t *challengeEntry;

        challengeEntry = NULL;
        for (challengeIndex = 0; challengeIndex < 1024; ++challengeIndex) {
            if (NET_CompareAdr(from, svs->challenges[challengeIndex].adr) &&
                challenge == svs->challenges[challengeIndex].challenge) {
                challengeEntry = &svs->challenges[challengeIndex];
                break;
            }
        }

        if (!challengeEntry) {
            NET_OutOfBandPrint((netsrc_t)1, from, "error\nEXE_BAD_CHALLENGE");
            return;
        }

        guid = challengeEntry->guid;
        ping = challengeEntry->pingTime;
        if (!ping) {
            ping = svs->time - challengeEntry->time;
            challengeEntry->pingTime = ping;
        }

        Com_Printf("Client %i connecting with %i challenge ping from %s\n", challengeIndex, ping, NET_AdrToString(from));
        challengeEntry->connected = 1;

        if (!Sys_IsLANAddress(from)) {
            int minPing;
            int maxPing;

            minPing = (*(const dvar_t **)imp_sv_minPing)->current.integer;
            if (minPing && ping < minPing) {
                NET_OutOfBandPrint((netsrc_t)1, from, "error\nEXE_ERR_HIGH_PING_ONLY");
                Com_DPrintf("Client %i rejected on a too low ping\n", challengeIndex);
                return;
            }

            maxPing = (*(const dvar_t **)imp_sv_maxPing)->current.integer;
            if (maxPing && ping > maxPing) {
                NET_OutOfBandPrint((netsrc_t)1, from, "error\nEXE_ERR_LOW_PING_ONLY");
                Com_DPrintf("Client %i rejected on a too high ping: %i\n", challengeIndex, ping);
                return;
            }
        }
    }

    for (clientNum = 0, cl = svs->clients; clientNum < maxClients; ++clientNum, ++cl) {
        if (!cl->state) {
            continue;
        }
        if (NET_CompareBaseAdr(from, cl->netchan.remoteAddress) &&
            (qport == cl->netchan.qport || from.port == cl->netchan.remoteAddress.port)) {
            Com_Printf("%s:reconnect\n", NET_AdrToString(from));
            if (cl->state > 1) {
                SV_FreeClient(cl);
            }
            goto setup_client;
        }
    }

    if (!strcmp(Info_ValueForKey(userinfo, "password"), (*(const dvar_t **)imp_sv_privatePassword)->current.string)) {
        startIndex = 0;
    } else {
        startIndex = (*(const dvar_t **)imp_sv_privateClients)->current.integer;
    }

    for (clientNum = startIndex; clientNum < maxClients; ++clientNum) {
        cl = &svs->clients[clientNum];
        if (!cl->state) {
            goto setup_client;
        }
    }

    NET_OutOfBandPrint((netsrc_t)1, from, "error\nEXE_SERVERISFULL");
    Com_DPrintf("Rejected a connection.\n");
    return;

setup_client:
    memset(cl, 0, sizeof(*cl));
    clientNum = (int)(cl - svs->clients);
    cl->gentity = (unsigned char *(*)[16])SV_GentityNum(clientNum);
    cl->scriptId = (unsigned short)Scr_AllocArray();
    cl->challenge = challenge;
    cl->guid = guid;

    Netchan_Setup((netsrc_t)1, &cl->netchan, from, qport);
    cl->voicePacketCount = 0;
    cl->sendVoice = 1;

    I_strncpyz(cl->userinfo, userinfo, sizeof(cl->userinfo));

    denied = ClientConnect(clientNum, cl->scriptId);
    if (denied) {
        NET_OutOfBandPrint((netsrc_t)1, from, va("error\n%s", denied));
        Com_DPrintf("Game rejected a connection: %s.\n", denied);
        SV_FreeClientScriptId(cl);
        return;
    }

    Com_Printf("Going from CS_FREE to CS_CONNECTED for %s (num %i guid %i)\n", cl->name, clientNum, cl->guid);

    cl->state = 2;
    cl->lastPacketTime = svs->time;
    cl->lastConnectTime = svs->time;
    cl->nextSnapshotTime = svs->time;

    SV_UserinfoChanged(cl);

    if (challengeIndex >= 0) {
        svs->challenges[challengeIndex].connected = 0;
    }

    NET_OutOfBandPrint((netsrc_t)1, from, "connectResponse");
    cl->deltaMessage = -1;

    connectedClients = 0;
    for (clientNum = 0, cl = svs->clients; clientNum < maxClients; ++clientNum, ++cl) {
        if (cl->state >= 2) {
            ++connectedClients;
        }
    }

    if (connectedClients == 1 || connectedClients == maxClients) {
        SV_Heartbeat_f();
    }
}

void SV_FreeClients(void)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    client_t *cl = svs->clients;
    const dvar_t *sv_maxclients = *(const dvar_t **)imp_sv_maxclients;
    int i;

    for (i = 0; i < sv_maxclients->current.integer; i++, cl++) {
        if (cl->state > 1) {
            SV_FreeClient(cl);
        }
    }

    Z_VirtualFreeInternal(svs->clients);
}

void SV_StopDownload_f(client_t *cl)
{
    if (cl->downloadName[0]) {
        Com_DPrintf("clientDownload: %d : file \"%s\" aborted\n",
                    SV_ClientNumForClient(cl), cl->downloadName);
    }

    SV_CloseDownload(cl);
}

void SV_NextDownload_f(client_t *cl)
{
    int block = atoi(SV_Cmd_Argv(1));

    if (block != cl->downloadClientBlock) {
        SV_DropClient(cl, "broken download");
        return;
    }

    Com_DPrintf("clientDownload: %d : client acknowledge of block %d\n",
                SV_ClientNumForClient(cl), block);

    if (!cl->downloadBlockSize[cl->downloadClientBlock % 8]) {
        Com_Printf("clientDownload: %d : file \"%s\" completed\n",
                   SV_ClientNumForClient(cl), cl->downloadName);
        SV_CloseDownload(cl);
        return;
    }

    cl->downloadSendTime = svs.time;
    cl->downloadClientBlock++;
}

void SV_BeginDownload_f(client_t *cl)
{
    SV_CloseDownload(cl);
#if COD2_FEATURE_NET_HARDENING

    {
        const char *requested = SV_Cmd_Argv(1);
        if (requested[0] && !NetHardening_ValidDownloadName(requested, 0)) {
            Com_Printf("WARNING: client requested malicious download path "
                       "'%s' - refused\n",
                       requested);
            cl->downloadName[0] = '\0';
            return;
        }
    }
#endif
    I_strncpyz(cl->downloadName, SV_Cmd_Argv(1), sizeof(cl->downloadName));
}

void SV_UserMove(client_t *cl, msg_t *msg, qboolean delta)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    server_t *sv = (server_t *)imp_sv;
    int clientNum;
    int cmdCount;
    int key;
    int i;
    playerState_t *ps;
    usercmd_t nullcmd;
    usercmd_t cmds[32];
    usercmd_t *oldcmd;
    usercmd_t *cmd;
    int newestServerTime;

    if (delta && cl->messageAcknowledge > cl->gamestateMessageNum) {
        cl->deltaMessage = cl->messageAcknowledge;
    } else {
        cl->deltaMessage = -1;
    }

    if (cl->reliableSequence - cl->reliableAcknowledge > 127)
        return;

    cmdCount = MSG_ReadByte(msg);
    if (cmdCount < 1) {
        Com_Printf("cmdCount < 1\n");
        return;
    }

    if (cmdCount > 32) {
        Com_Printf("cmdCount > MAX_PACKET_USERCMDS\n");
        return;
    }

    key = cl->messageAcknowledge ^ sv->checksumFeed;
    key ^= Com_HashKey(cl->reliableCommandInfo[cl->reliableAcknowledge & 127].cmd, 32);

    clientNum = SV_ClientNumForClient(cl);
    ps = SV_GameClientNum(clientNum);
    MSG_SetDefaultUserCmd(ps, &nullcmd);

    oldcmd = &nullcmd;
    for (i = 0; i < cmdCount; ++i) {
        cmd = &cmds[i];
        MSG_ReadDeltaUsercmdKey(msg, key, oldcmd, cmd);

        if (!BG_IsWeaponValid(ps, cmd->weapon))
            cmd->weapon = (byte)ps->weapon;

        if (!BG_IsWeaponValid(ps, cmd->offHandIndex))
            cmd->offHandIndex = (byte)ps->offHandIndex;

        oldcmd = cmd;
    }

    cl->frames[cl->messageAcknowledge & 31].messageAcked = svs->time;

    if (cl->state == 3) {
        Com_DPrintf("Going from CS_PRIMED to CS_ACTIVE for %s\n", cl->name);
        cl->state = 4;
        cl->gentity = (unsigned char *(*)[16])SV_GentityNum(clientNum);
        *(int *)cl->gentity = clientNum;
        cl->deltaMessage = -1;
        cl->nextSnapshotTime = svs->time;
        cl->lastUsercmd = cmds[0];
        ClientBegin(clientNum);
    }

    if ((*(const dvar_t **)imp_sv_pure)->current.enabled && !cl->pureAuthentic) {
        SV_DropClient(cl, "EXE_CANNOTVALIDATEPURECLIENT");
        return;
    }

    if (cl->state != 4) {
        cl->deltaMessage = -1;
        return;
    }

    newestServerTime = cmds[cmdCount - 1].serverTime;
    for (i = 0; i < cmdCount; ++i) {
        cmd = &cmds[i];

        if (cmd->serverTime > newestServerTime)
            continue;

        if (cmd->serverTime <= cl->lastUsercmd.serverTime)
            continue;

        cl->lastUsercmd = *cmd;

        if (cl->state == 4) {
            G_SetLastServerTime(clientNum, cmd->serverTime);
            ClientThink(clientNum);
        }
    }
}

void SV_ExecuteClientMessage(client_t *cl, msg_t *msg)
{
    msg_t msgCompressed;
    byte msgCompressed_buf[MAX_MSG_DECOMPRESS_BYTES];
    int serverId;

    MSG_Init(&msgCompressed, msgCompressed_buf, sizeof(msgCompressed_buf));
    msgCompressed.cursize = MSG_ReadBitsCompress(msg->data + msg->readcount,
                                                 msgCompressed_buf,
                                                 msg->cursize - msg->readcount);

    serverId = *(int *)imp_sv_serverId_value;
    SV_ExecDbg("", cl->serverId, serverId, cl->state);

    if (cl->serverId != serverId && !cl->downloadName[0]) {
        if ((cl->serverId & 0xf0) == (serverId & 0xf0)) {
            if (cl->state != 3) {
                return;
            }

            Com_DPrintf("Going from CS_PRIMED to CS_ACTIVE for %s\n", cl->name);
            cl->state = 4;
            {
                int clientNum = SV_ClientNumForClient(cl);
                gentity_t *ent = SV_GentityNum(clientNum);
                ent->s.number = clientNum;
                cl->gentity = (unsigned char *(*)[16])ent;
                cl->deltaMessage = -1;
                cl->nextSnapshotTime = svs.time;
                ClientBegin(clientNum);
            }
            return;
        }

        if (cl->messageAcknowledge <= cl->gamestateMessageNum) {
            return;
        }
        if (cl->netchan.unsentFragments || cl->state == 3) {
            return;
        }

        Com_DPrintf("%s : dropped gamestate, resending\n", cl->name);
        SV_SendClientGameState(cl);

        if (!(*(const dvar_t **)imp_net_lanauthorize)->current.enabled &&
            Sys_IsLANAddress(cl->netchan.remoteAddress)) {
            return;
        }

#if COD2_IS_PATCH_13
        SV_AuthorizeRequest(cl->netchan.remoteAddress, cl->challenge, NULL);
#else
        SV_AuthorizeRequest(cl->netchan.remoteAddress, cl->challenge);
#endif
        return;
    }

    for (;;) {
        int c = MSG_ReadBits(&msgCompressed, 3);
        SV_ExecDbg("", c, cl->state, 0);

        if (c == 3) {
            return;
        }

        if (c == 2) {
            int seq = MSG_ReadLong(&msgCompressed);
            char *s = MSG_ReadString(&msgCompressed);

            if (seq > cl->lastClientCommand) {
                int clientOK;

                if ((*(const dvar_t **)imp_sv_showCommands)->current.enabled) {
                    Com_Printf("clientCommand: %i : %s\n", seq, s);
                }

                if (seq > cl->lastClientCommand + 1) {
                    Com_Printf("Client %s lost %i clientCommands\n", cl->name, seq - cl->lastClientCommand + 1);
                    SV_DropClient(cl, "EXE_LOSTRELIABLECOMMANDS");
                    return;
                }

                clientOK = I_strncmp(s, "team ", 5) &&
                           I_strncmp(s, "score ", 6) &&
                           I_strncmp(s, "mr ", 3);

                if (!(*(const dvar_t **)imp_legacyHacks)->current.integer &&
                    cl->state > 3 &&
                    (*(const dvar_t **)imp_sv_floodProtect)->current.enabled &&
                    svs.time < cl->nextReliableTime) {
                    if (clientOK) {
                        Com_DPrintf("client text ignored for %s: %s\n", cl->name, SV_Cmd_Argv(0));
                        clientOK = 0;
                    } else {
                        clientOK = 1;
                    }
                    cl->nextReliableTime = svs.time + 800;
                } else if (!clientOK) {
                    clientOK = 1;
                }

                SV_ExecuteClientCommand(cl, s, clientOK);
                cl->lastClientCommand = seq;
                Com_sprintf(cl->lastClientCommandString, 0x400, "%s", s);
            }

            if (cl->state == 1) {
                return;
            }
            continue;
        }

        if ((*(const dvar_t **)imp_sv_pure)->current.enabled && cl->pureAuthentic == 2) {
            cl->nextSnapshotTime = -1;
            SV_DropClient(cl, "EXE_UNPURECLIENTDETECTED");
            cl->state = 4;
            SV_SendClientSnapshot(cl);
            cl->state = 1;
        }

        if (c == 0) {
            SV_UserMove(cl, &msgCompressed, 1);
            return;
        }

        if (c == 1) {
            SV_UserMove(cl, &msgCompressed, 0);
            return;
        }

        Com_Printf("WARNING: bad command byte %i for client %i\n", c, SV_ClientNumForClient(cl));
        return;
    }
}

gentity_t *SV_AddTestClient(void)
{
    serverStatic_t *svsPtr = (serverStatic_t *)imp_svs;
    int maxClients = (*(const dvar_t **)imp_sv_maxclients)->current.integer;
    char userinfo[1024];
    usercmd_t nullcmd;
    netadr_t adr;
    client_t *cl;
    gentity_t *ent;
    int clientNum;

    if (maxClients <= 0) {
        if (maxClients == 0)
            return NULL;
    } else {
        for (clientNum = 0, cl = svsPtr->clients; clientNum < maxClients; ++clientNum, ++cl) {
            if (cl->state == 0)
                break;
        }
        if (clientNum == maxClients)
            return NULL;
    }

    sprintf(userinfo, "connect \"\\cg_predictItems\\1\\cl_anonymous\\0\\color\\4\\head\\default\\model\\multi\\snaps\\20\\rate\\5000\\name\\bot%d\\protocol\\%d\"", botport, 0x76);
    SV_Cmd_TokenizeString(userinfo);

    memset(&adr, 0, sizeof(adr));
    adr.port = (unsigned short)botport;
    ++botport;

    SV_DirectConnect(adr);

    maxClients = (*(const dvar_t **)imp_sv_maxclients)->current.integer;
    for (clientNum = 0, cl = svsPtr->clients; clientNum < maxClients; ++clientNum, ++cl) {
        if (cl->state && NET_CompareBaseAdr(adr, cl->netchan.remoteAddress))
            break;
    }

    if (clientNum == maxClients)
        return NULL;

    cl->bIsTestClient = 1;
    SV_BotResetClient(clientNum);
    SV_SendClientGameState(cl);

    memset(&nullcmd, 0, sizeof(nullcmd));

    Com_DPrintf("Going from CS_PRIMED to CS_ACTIVE for %s\n", cl->name);
    cl->state = 4;

    ent = SV_GentityNum(clientNum);
    ent->s.number = clientNum;
    cl->gentity = (unsigned char *(*)[16])ent;
    cl->deltaMessage = -1;
    cl->nextSnapshotTime = svsPtr->time;
    cl->lastUsercmd = nullcmd;

    ClientBegin(clientNum);

    return SV_GentityNum(clientNum);
}

ucmd_t ucmds[12] = {
    { (char *)&str_002adea0, &SV_UpdateUserinfo_f },
    { (char *)&str_00228e90, &SV_Disconnect_f },
    { (char *)&str_002adeac, &SV_VerifyIwds_f },
    { (char *)&str_002a96d4, &SV_ResetPureClient_f },
    { (char *)&str_002adeb0, &SV_BeginDownload_f },
    { (char *)&str_002adebc, &SV_NextDownload_f },
    { (char *)&str_002adec4, &SV_StopDownload_f },
    { (char *)&str_002a98b0, &SV_DoneDownload_f },
    { (char *)&str_002adecc, &SV_RetransmitDownload_f },
    { (char *)&str_002aded8, &SV_MutePlayer_f },
    { (char *)&str_002adee4, &SV_UnmutePlayer_f },
    { 0, 0 }
};

const char str_00228e90[] = "disconnect";
const char str_002a96d4[] = "vdr";
const char str_002a98b0[] = "donedl";
const char str_002adea0[] = "userinfo";
const char str_002adeac[] = "cp";
const char str_002adeb0[] = "download";
const char str_002adebc[] = "nextdl";
const char str_002adec4[] = "stopdl";
const char str_002adecc[] = "retransdl";
const char str_002aded8[] = "muteplayer";
const char str_002adee4[] = "unmuteplayer";
