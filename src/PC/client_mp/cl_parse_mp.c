#include "common_types.h"
#include "imports.h"
#include "www_download.h"
#include "cod2_feature_config.h"
#include "PC/qcommon/net_hardening.h"
#include <stdlib.h>

#if COD2_IS_PATCH_13
extern struct clientStatic_t cls;
extern const dvar_t *com_developer;
extern char *Dvar_GetString(const char *dvarName);
extern void FS_BuildOSPath(const char *base, const char *game, const char *qpath, char *ospath);
extern void Cbuf_ExecuteText(int execWhen, const char *text);
extern void Sys_OpenURL(const char *url, int activate);
extern int FS_ReadFile(const char *qpath, void **buffer);
extern void FS_FreeFile(void *buffer);
extern void FS_CopyFile(const char *fromOSPath, const char *toOSPath);
extern void Dvar_SetStringByName(const char *dvarName, const char *value);
void CL_WWWDownload(void);
void CL_ParseWWWDownload(msg_t *msg);
extern int CL_ClearStaticDownload(void);
#endif

extern int cl_connectedToPureServer;

/* svc_strings: server-command name table. Migrated from the ILP32 data blob
 * (build/native_gen/data32.c) to a typed C definition so it re-lays-out per
 * target -- 256 char* is 1024 B on x86 (byte-identical to the old blob) and
 * 2048 B on x64. The 8 live entries point at the reconstructed string literals
 * in common.c; entries [8..255] are NULL. (x64 port Stage 2 pilot; cf. the
 * noticeErrors / holdrand typed-data migrations.) */
char *svc_strings[256] = {
    "svc_nop",
    "svc_gamestate",
    "svc_configstring",
    "svc_baseline",
    "svc_serverCommand",
    "svc_download",
    "svc_snapshot",
    "svc_EOF",
    /* [8 .. 255] implicitly NULL */
};

extern void *cl;
extern byte cls_ptr[];
extern byte **clc_ptr;
extern dvar_t *com_dedicated;
extern const dvar_t *cl_paused;
extern const dvar_t *net_lanauthorize;
extern byte **cl_packetdelay;
extern byte **download_ui_ptr;
extern const dvar_t *cl_shownet;
extern const dvar_t *cl_shownuments;

void CL_SystemInfoChanged(void);
void CL_ParseGamestate(msg_t *msg);
void CL_ParseDownload(msg_t *msg);
void CL_ParseSnapshot(msg_t *msg);
void CL_ParseServerMessage(msg_t *msg);

extern void LargeLocal_LargeLocal(const LargeLocal *_this, int size);
extern void *LargeLocal_GetBuf(const LargeLocal *_this);
extern void ZN10LargeLocalD1Ev(const LargeLocal *_this);

extern char *Info_ValueForKey(const char *s, const char *key);
extern void Info_NextPair(const char **head, char *key, char *value);
extern void FS_PureServerSetLoadedIwds(const char *iwdSums, const char *iwdNames);
extern void FS_PureServerSetReferencedIwds(const char *iwdSums, const char *iwdNames);
extern void Dvar_SetFromStringByName(const char *dvarName, const char *string);
extern Bool Dvar_GetBool(const char *dvarName);
extern void Dvar_SetCheatState(void);
extern void Dvar_SetInt(const void *dvar, int value);
extern void Con_Close(void);
extern void CL_ClearState(void);
extern void CL_SystemInfoChanged(void);
extern qboolean FS_ConditionalRestart(int checksumFeed);
extern qboolean Sys_IsLANAddress(netadr_t adr);
extern void CL_RequestAuthorization(void);
extern void CL_InitDownloads(void);
extern void CL_AddReliableCommand(const char *cmd);
extern void CL_WritePacket(void);
extern void CL_NextDownload(void);
extern void Com_Error(int level, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern void Com_DPrintf(const char *fmt, ...);
extern int MSG_ReadLong(msg_t *msg);
extern int MSG_ReadByte(msg_t *msg);
extern int MSG_ReadShort(msg_t *msg);
extern int MSG_ReadBits(msg_t *msg, int bits);
extern int MSG_ReadBit(msg_t *msg);
extern char *MSG_ReadString(msg_t *msg);
extern char *MSG_ReadBigString(msg_t *msg);
extern void MSG_ReadData(msg_t *msg, void *data, int len);
extern void MSG_ReadDeltaPlayerstate(msg_t *msg, playerState_t *from, playerState_t *to);
extern qboolean MSG_ReadDeltaEntity(msg_t *msg, entityState_t *from, entityState_t *to, int number);
extern qboolean MSG_ReadDeltaClient(msg_t *msg, clientState_t *from, clientState_t *to, int number);
extern void MSG_Init(msg_t *buf, byte *data, int length);
extern int MSG_ReadBitsCompress(byte *from, byte *to, int size);
extern char *va(const char *format, ...);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern fileHandle_t FS_SV_FOpenFileWrite(const char *filename);
extern void FS_SV_Rename(const char *from, const char *to);
extern int FS_Write(const void *buffer, int len, fileHandle_t h);
extern void FS_FCloseFile(fileHandle_t h);

static int CL_ShowNetValue(void)
{
    return cl_shownet ? cl_shownet->current.integer : 0;
}

static qboolean CL_ShowPacketsEnabled(void)
{
    return cl_shownuments && cl_shownuments->current.enabled;
}

void CL_SystemInfoChanged(void)
{
    LargeLocal key_large_local;
    LargeLocal value_large_local;
    char *key;
    char *value;
    clientActive_t *cla;
    clientConnection_t *clc;
    char *systemInfo;
    const char *s;

    LargeLocal_LargeLocal(&key_large_local, 0x2000);
    key = (char *)LargeLocal_GetBuf(&key_large_local);

    LargeLocal_LargeLocal(&value_large_local, 0x2000);
    value = (char *)LargeLocal_GetBuf(&value_large_local);

    cla = (clientActive_t *)cl;
    clc = (clientConnection_t *)*clc_ptr;

    systemInfo = cla->gameState.stringData + cla->gameState.stringOffsets[1];

    {
        const char *sid_str = Info_ValueForKey(systemInfo, "sv_serverid");
        int sid_val = atoi(sid_str);
        cla->serverId = sid_val;
    }

    if (((clientConnection_t *)*clc_ptr)->demoplaying != 0) {
        ZN10LargeLocalD1Ev(&value_large_local);
        ZN10LargeLocalD1Ev(&key_large_local);
        return;
    }

    s = Info_ValueForKey(systemInfo, "sv_cheats");
    if (atoi(s) == 0) {

        Dvar_SetCheatState();
    }

    s = Info_ValueForKey(systemInfo, "sv_iwds");

    FS_PureServerSetLoadedIwds(s, Info_ValueForKey(systemInfo, "sv_iwdNames"));

    s = Info_ValueForKey(systemInfo, "sv_referencedIwds");

    FS_PureServerSetReferencedIwds(s, Info_ValueForKey(systemInfo, "sv_referencedIwdNames"));

    if (!com_dedicated->current.integer) {

        s = systemInfo;
        while (s) {
            Info_NextPair(&s, key, value);
            if (key[0] == '\0')
                break;
#if COD2_FEATURE_NET_HARDENING

            if (!NetHardening_ServerMaySetCvar(key)) {
                Com_Printf("WARNING: server tried to set protected cvar '%s' "
                           "via systeminfo - ignored\n",
                           key);
                continue;
            }
#endif

            Dvar_SetFromStringByName(key, value);
        }
    }

    cl_connectedToPureServer = (int)Dvar_GetBool("sv_pure");

    ZN10LargeLocalD1Ev(&value_large_local);
    ZN10LargeLocalD1Ev(&key_large_local);
}

void CL_ParseGamestate(msg_t *msg)
{
    clientConnection_t *clc;
    clientActive_t *cla;
    int newnum;
    int i;
    char *s;
    int len;
    entityState_t nullstate;

    Con_Close();

    clc = (clientConnection_t *)*clc_ptr;
    clc->connectPacketCount = 0;

    CL_ClearState();

    clc->serverCommandSequence = MSG_ReadLong(msg);

    cla = (clientActive_t *)cl;
    cla->gameState.dataCount = 1;

    for (;;) {

        int cmd = MSG_ReadByte(msg);

        if (cmd == 7)
            break;

        if (cmd == 2) {

            i = MSG_ReadShort(msg);
            if (i > 0x7ff) {
                Com_Error(1, "\x15"
                             "configstring > MAX_CONFIGSTRINGS");
            }

            s = MSG_ReadBigString(msg);

            len = strlen(s);

            cla = (clientActive_t *)cl;
            if (len + 1 + cla->gameState.dataCount > MAX_GAMESTATE_CHARS) {
                Com_Error(1, "\x15MAX_GAMESTATE_CHARS exceeded");
            }

            cla->gameState.stringOffsets[i] = cla->gameState.dataCount;

            memcpy(cla->gameState.stringData + cla->gameState.dataCount, s, len + 1);

            cla->gameState.dataCount = cla->gameState.dataCount + len + 1;
        } else if (cmd == 3) {

            newnum = MSG_ReadBits(msg, 10);
            if (newnum > 0x3ff) {
                Com_Error(1, "\x15"
                             "Baseline number out of range: %i",
                          newnum);
            }

            memset(&nullstate, 0, sizeof(entityState_t));

            MSG_ReadDeltaEntity(msg, &nullstate,
                                &cla->entityBaselines[newnum],
                                newnum);
        } else {

            Com_Error(1, "\x15"
                         "CL_ParseGamestate: bad command byte");
        }
    }

    clc = (clientConnection_t *)*clc_ptr;
    clc->clientNum = MSG_ReadLong(msg);

    clc->checksumFeed = MSG_ReadLong(msg);

    CL_SystemInfoChanged();

    FS_ConditionalRestart(clc->checksumFeed);

    if (net_lanauthorize->current.enabled == 0) {
        if (Sys_IsLANAddress(clc->serverAddress)) {

            CL_InitDownloads();
            Dvar_SetInt(cl_paused, 0);
            return;
        }
    }

    CL_RequestAuthorization();

    CL_InitDownloads();

    Dvar_SetInt(cl_paused, 0);
}

void CL_ParseDownload(msg_t *msg)
{
    LargeLocal data_large_local;
    byte *data;
    int block;
    int size;
    clientConnection_t *clc;

    LargeLocal_LargeLocal(&data_large_local, MAX_MSGLEN);
    data = (byte *)LargeLocal_GetBuf(&data_large_local);

    block = MSG_ReadShort(msg);

#if COD2_IS_PATCH_13

    if (block == -1) {
        if (!cls.wwwDlInProgress) {
            CL_ParseWWWDownload(msg);
        } else {

            MSG_ReadString(msg);
            MSG_ReadLong(msg);
            MSG_ReadLong(msg);
        }
        ZN10LargeLocalD1Ev(&data_large_local);
        return;
    }
#endif

    if (block == 0) {

        clc = (clientConnection_t *)*clc_ptr;
        clc->downloadSize = MSG_ReadLong(msg);

        *(int *)((*download_ui_ptr) + 0x10) = clc->downloadSize;

        size = clc->downloadSize;
        if (size < 0) {

            Com_Error(1, "%s", va("%s", MSG_ReadString(msg)));
            ZN10LargeLocalD1Ev(&data_large_local);
            return;
        }
    }

    size = MSG_ReadShort(msg);

    if (size > 0) {

        MSG_ReadData(msg, data, size);
    }

    clc = (clientConnection_t *)*clc_ptr;
    if (clc->downloadBlock != block) {

        Com_DPrintf("CL_ParseDownload: Expected block %d, got %d\n",
                    clc->downloadBlock, block);

        if (block > clc->downloadBlock) {

            Com_DPrintf("CL_ParseDownload: Sending retransmit request to get the missed block\n");

            CL_AddReliableCommand(va("retransdl %d", clc->downloadBlock));
        }
        ZN10LargeLocalD1Ev(&data_large_local);
        return;
    }

#if COD2_FEATURE_NET_HARDENING

    if (clc->downloadTempName[0] &&
        !NetHardening_ValidDownloadName(clc->downloadTempName, 0)) {
        Com_Printf("WARNING: refusing malicious download temp path '%s'\n",
                   clc->downloadTempName);
        CL_AddReliableCommand("stopdl");
        clc->downloadName[0] = 0;
        clc->downloadTempName[0] = 0;
        ZN10LargeLocalD1Ev(&data_large_local);
        return;
    }
    if (clc->downloadName[0] &&
        !NetHardening_ValidDownloadName(clc->downloadName, 1)) {
        Com_Printf("WARNING: refusing malicious download name '%s'\n",
                   clc->downloadName);
        CL_AddReliableCommand("stopdl");
        clc->downloadName[0] = 0;
        clc->downloadTempName[0] = 0;
        ZN10LargeLocalD1Ev(&data_large_local);
        return;
    }
#endif

    if (clc->download == 0) {

        if (clc->downloadTempName[0] == '\0') {

            Com_Printf("Server sending download, but no download was requested\n");
            CL_AddReliableCommand("stopdl");
            ZN10LargeLocalD1Ev(&data_large_local);
            return;
        }

        clc->download = FS_SV_FOpenFileWrite(clc->downloadTempName);
        if (clc->download == 0) {

            Com_Printf("Could not create %s\n", clc->downloadTempName);
            CL_AddReliableCommand("stopdl");
            CL_NextDownload();
            ZN10LargeLocalD1Ev(&data_large_local);
            return;
        }
    }

    if (size != 0) {

        FS_Write(data, size, ((clientConnection_t *)*clc_ptr)->download);
    }

    clc = (clientConnection_t *)*clc_ptr;
    CL_AddReliableCommand(va("nextdl %d", clc->downloadBlock));

    clc->downloadBlock += 1;

    clc->downloadCount += size;

    *(int *)((*download_ui_ptr) + 0x14) = clc->downloadCount;

    if (size != 0) {
        ZN10LargeLocalD1Ev(&data_large_local);
        return;
    }

    if (clc->download != 0) {

        FS_FCloseFile(clc->download);

        clc->download = 0;

        FS_SV_Rename(clc->downloadTempName, clc->downloadName);
    }

    clc->downloadName[0] = 0;
    clc->downloadTempName[0] = 0;

    *(byte *)((*download_ui_ptr) + 0x1c) = 0;

    CL_WritePacket();

    CL_WritePacket();

    CL_NextDownload();

    ZN10LargeLocalD1Ev(&data_large_local);
}

#if COD2_IS_PATCH_13

void CL_ParseWWWDownload(msg_t *msg)
{
    char toOSPath[256];
    char *fs_homepath = Dvar_GetString("fs_homepath");

    I_strncpyz(cls.originalDownloadName, cls.downloadName, sizeof(cls.originalDownloadName));
    I_strncpyz(cls.downloadName, MSG_ReadString(msg), sizeof(cls.downloadName));
    cls.downloadSize = MSG_ReadLong(msg);
    cls.downloadFlags = MSG_ReadLong(msg);

#    if COD2_FEATURE_NET_HARDENING && COD2_FEATURE_WWW_DOWNLOAD

    if (!(cls.downloadFlags & 0x2) && !NetHardening_ValidUrl(cls.downloadName)) {
        Com_Printf("WARNING: refusing wwwdl redirect to non-http(s) URL '%s'\n",
                   cls.downloadName);
        CL_AddReliableCommand("wwwdl fail");
        DL_CancelDownload();
        cls.wwwDlInProgress = 0;
        return;
    }
    if (cls.originalDownloadName[0] &&
        !NetHardening_ValidDownloadName(cls.originalDownloadName, 1)) {
        Com_Printf("WARNING: refusing malicious wwwdl local name '%s'\n",
                   cls.originalDownloadName);
        CL_AddReliableCommand("wwwdl fail");
        DL_CancelDownload();
        cls.wwwDlInProgress = 0;
        return;
    }
#    endif

    *(int *)((*download_ui_ptr) + 0x10) = cls.downloadSize;

    if (cls.downloadFlags & 0x2) {
        Sys_OpenURL(cls.downloadName, 1);
        Cbuf_ExecuteText(2, "quit\n");
        CL_AddReliableCommand("wwwdl bbl8r");
        DL_CancelDownload();
        cls.wwwDlInProgress = 0;
        return;
    }

    Com_DPrintf("Server redirected download: %s\n", cls.downloadName);
    cls.wwwDlInProgress = 1;
    CL_AddReliableCommand("wwwdl ack");

    FS_BuildOSPath(fs_homepath, cls.downloadTempName, "", toOSPath);
    I_strncpyz(cls.downloadTempName, toOSPath, sizeof(cls.downloadTempName));

    if (cls.downloadTempName[0])
        cls.downloadTempName[strlen(cls.downloadTempName) - 1] = '\0';

    if (DL_BeginDownload(cls.downloadTempName, cls.downloadName, com_developer->current.integer)) {
        if (cls.downloadFlags & 0x1) {
            CL_AddReliableCommand("wwwdl bbl8r");
            cls.wwwDlDisconnected = 1;
        }
        return;
    }

    CL_AddReliableCommand("wwwdl fail");
    DL_CancelDownload();
    cls.wwwDlInProgress = 0;
    Com_Printf("Failed to initialize download for '%s'\n", cls.downloadName);
    if (cls.downloadFlags & 0x1) {
        CL_AddReliableCommand("wwwdl bbl8r");
        cls.wwwDlDisconnected = 1;
    }
}

void CL_WWWDownload(void)
{
    char *fs_homepath = Dvar_GetString("fs_homepath");
    dlStatus_t st;

    DL_DownloadLoop();
    st = DL_GetStatus();

    if (st == DL_STATUS_IN_PROGRESS || st == DL_STATUS_IDLE)
        return;

    if (DL_DLIsMotd()) {
        if (st == DL_STATUS_DONE) {
            void *buf;
            if (FS_ReadFile("motd.txt", &buf) >= 0) {
                Dvar_SetStringByName("motd", (const char *)buf);
                FS_FreeFile(buf);
            }
        }
        cls.wwwDlInProgress = 0;
        return;
    }

    if (st == DL_STATUS_DONE) {
        char toOSPath[256];

        cls.download = 0;
        FS_BuildOSPath(fs_homepath, cls.downloadName, "", toOSPath);
        if (toOSPath[0])
            toOSPath[strlen(toOSPath) - 1] = '\0';

        if (rename(cls.downloadTempName, toOSPath) != 0) {
            FS_CopyFile(cls.downloadTempName, toOSPath);
            remove(cls.downloadTempName);
        }

        cls.downloadTempName[0] = '\0';
        cls.downloadName[0] = '\0';
        *(char *)((*download_ui_ptr) + 0x1c) = 0;

        if (cls.wwwDlDisconnected)
            Cbuf_ExecuteText(2, "reconnect\n");
        else
            CL_AddReliableCommand("wwwdl done");

        cls.wwwDlInProgress = 0;
        CL_NextDownload();
    } else {
        if (cls.wwwDlDisconnected) {
            char *err = va("Download failure while getting '%s'\n", cls.downloadName);
            cls.wwwDlDisconnected = 0;
            CL_ClearStaticDownload();
            Com_Error(1, "%s", err);
            return;
        }
        Com_Printf("Download failure while getting '%s'\n", cls.downloadName);
        CL_AddReliableCommand("wwwdl fail");
        cls.wwwDlInProgress = 0;
    }
}
#endif

static byte *CL_EntitySlot(byte *base, int num)
{
    int idx = num & 0x7ff;
    return base + 0xd30e0 + idx * 240;
}

static byte *CL_ClientSlot(byte *base, int num)
{
    int idx = num & 0x7ff;
    return base + 0x14b0e0 + idx * 92;
}

static byte *CL_SnapSlot(byte *base, int messageNum)
{
    int idx = messageNum & 0x1f;
    return base + 0x495e0 + idx * 0x26d8;
}

void CL_ParseSnapshot(msg_t *msg)
{
    LargeLocal newSnap_large_local;
    clSnapshot_t *newSnap;
    clientActive_t *cla;
    clientConnection_t *clc;
    clSnapshot_t *old;
    int oldMessageNum;
    int deltaNum;
    int newnum;
    int oldindex;
    entityState_t nullstate_ent;
    clientState_t dummy;
    byte *oldEntitySlot;
    int oldEntityNum;
    byte *oldClientSlot;
    int oldClientNum;

    LargeLocal_LargeLocal(&newSnap_large_local, sizeof(clSnapshot_t));
    newSnap = (clSnapshot_t *)LargeLocal_GetBuf(&newSnap_large_local);

    memset(newSnap, 0, sizeof(clSnapshot_t));

    clc = (clientConnection_t *)*clc_ptr;
    newSnap->serverCommandNum = clc->serverCommandSequence;

    newSnap->serverTime = MSG_ReadLong(msg);

    newSnap->messageNum = clc->serverMessageSequence;

    deltaNum = MSG_ReadByte(msg);

    if (deltaNum == 0) {

        newSnap->deltaNum = -1;
    } else {

        newSnap->deltaNum = newSnap->messageNum - deltaNum;
    }

    newSnap->snapFlags = MSG_ReadByte(msg);

    oldMessageNum = newSnap->deltaNum;
    if (oldMessageNum <= 0) {

        newSnap->valid = 1;

        ((clientConnection_t *)*clc_ptr)->demowaiting = 0;
        old = NULL;
    } else {

        cla = (clientActive_t *)cl;
        old = &cla->snapshots[oldMessageNum & 0x1f];

        if (old->valid == 0) {

            Com_Printf("Delta from invalid frame (not supposed to happen!).\n");
        } else if (old->messageNum != oldMessageNum) {

            Com_DPrintf("Delta frame too old.\n");
        } else {

            if (cla->parseEntitiesNum - old->parseEntitiesNum > 0x780) {

                Com_DPrintf("Delta parseEntitiesNum too old.\n");
            } else if (cla->parseClientsNum - old->parseClientsNum > 0x780) {

                Com_DPrintf("Delta parseClientsNum too old.\n");
            } else {

                newSnap->valid = 1;
            }
        }
    }

    if (CL_ShowNetValue() > 1) {
        Com_Printf("%3i:%s\n", msg->readcount - 1, "playerstate");
    }

    if (old) {
        MSG_ReadDeltaPlayerstate(msg, &old->ps, &newSnap->ps);
    } else {

        MSG_ReadDeltaPlayerstate(msg, NULL, &newSnap->ps);
    }

    if (CL_ShowNetValue() > 1) {
        Com_Printf("%3i:%s\n", msg->readcount - 1, "packet clients");
    }

    cla = (clientActive_t *)cl;
    newSnap->parseEntitiesNum = cla->parseEntitiesNum;
    newSnap->numEntities = 0;

    if (old && old->numEntities > 0) {

        oldEntitySlot = (byte *)&cla->parseEntities[old->parseEntitiesNum & 0x7ff];
        oldEntityNum = *(int *)oldEntitySlot;
    } else {
        oldEntitySlot = NULL;
        oldEntityNum = 0x1869f;
    }

    oldindex = 0;

    for (;;) {

        if (msg->overflowed)
            break;

        newnum = MSG_ReadBits(msg, 10);

        if (newnum == 0x3ff)
            break;

        if (msg->readcount > msg->cursize) {
            Com_Error(1, "\x15"
                         "CL_ParsePacketEntities: end of message");
        }

        while (oldEntityNum < newnum) {
            if (msg->overflowed)
                break;

            if (CL_ShowNetValue() == 3) {
                Com_Printf("%3i:  unchanged: %i\n", msg->readcount, oldEntityNum);
            }

            cla = (clientActive_t *)cl;
            memcpy(&cla->parseEntities[cla->parseEntitiesNum & 0x7ff], oldEntitySlot, sizeof(entityState_t));
            cla->parseEntitiesNum += 1;
            newSnap->numEntities += 1;

            oldindex += 1;
            if (oldindex >= old->numEntities) {
                oldEntityNum = 0x1869f;
            } else {
                oldEntitySlot = (byte *)&cla->parseEntities[(oldindex + old->parseEntitiesNum) & 0x7ff];
                oldEntityNum = *(int *)oldEntitySlot;
            }
        }

        if (oldEntityNum == newnum) {

            if (CL_ShowNetValue() == 3) {
                Com_Printf("%3i:  delta: %i\n", msg->readcount, oldEntityNum);
            }

            cla = (clientActive_t *)cl;
            if (!MSG_ReadDeltaEntity(msg, (entityState_t *)oldEntitySlot,
                                     &cla->parseEntities[cla->parseEntitiesNum & 0x7ff],
                                     oldEntityNum)) {
                cla->parseEntitiesNum += 1;
                newSnap->numEntities += 1;
            }

            oldindex += 1;
            if (oldindex >= old->numEntities) {
                oldEntityNum = 0x1869f;
            } else {
                oldEntitySlot = (byte *)&cla->parseEntities[(oldindex + old->parseEntitiesNum) & 0x7ff];
                oldEntityNum = *(int *)oldEntitySlot;
            }
        } else {

            if (CL_ShowNetValue() == 3) {
                Com_Printf("%3i:  baseline: %i\n", msg->readcount, newnum);
            }

            cla = (clientActive_t *)cl;
            if (!MSG_ReadDeltaEntity(msg,
                                     &cla->entityBaselines[newnum],
                                     &cla->parseEntities[cla->parseEntitiesNum & 0x7ff],
                                     newnum)) {
                cla->parseEntitiesNum += 1;
                newSnap->numEntities += 1;
            }
        }
    }

    while (oldEntityNum != 0x1869f) {
        if (msg->overflowed)
            break;

        if (CL_ShowNetValue() == 3) {
            Com_Printf("%3i:  unchanged: %i\n", msg->readcount, oldEntityNum);
        }

        cla = (clientActive_t *)cl;
        memcpy(&cla->parseEntities[cla->parseEntitiesNum & 0x7ff], oldEntitySlot, sizeof(entityState_t));
        cla->parseEntitiesNum += 1;
        newSnap->numEntities += 1;

        oldindex += 1;
        if (oldindex >= old->numEntities)
            break;

        oldEntitySlot = (byte *)&cla->parseEntities[(oldindex + old->parseEntitiesNum) & 0x7ff];
        oldEntityNum = *(int *)oldEntitySlot;
    }

    if (CL_ShowPacketsEnabled()) {
        Com_Printf("Entities in packet: %i\n", newSnap->numEntities);
    }

    if (CL_ShowNetValue() > 1) {
        Com_Printf("%3i:%s\n", msg->readcount - 1, "packet clients");
    }

    cla = (clientActive_t *)cl;
    newSnap->parseClientsNum = cla->parseClientsNum;
    newSnap->numClients = 0;

    if (old && old->numClients > 0) {

        oldClientSlot = (byte *)&cla->parseClients[old->parseClientsNum & 0x7ff];
        oldClientNum = *(int *)oldClientSlot;
    } else {
        oldClientSlot = NULL;
        oldClientNum = 0x1869f;
    }

    oldindex = 0;

    for (;;) {

        if (msg->overflowed)
            break;

        if (!MSG_ReadBit(msg))
            break;

        newnum = MSG_ReadBits(msg, 6);

        if (msg->readcount > msg->cursize) {
            Com_Error(1, "\x15"
                         "CL_ParsePacketClients: end of message");
        }

        while (oldClientNum < newnum) {

            if (CL_ShowNetValue() == 3) {
                Com_Printf("%3i:  unchanged: %i\n", msg->readcount, oldClientNum);
            }

            cla = (clientActive_t *)cl;
            memcpy(&cla->parseClients[cla->parseClientsNum & 0x7ff], oldClientSlot, sizeof(clientState_t));
            cla->parseClientsNum += 1;
            newSnap->numClients += 1;

            oldindex += 1;
            if (oldindex >= old->numClients) {
                oldClientNum = 0x1869f;
                break;
            }
            oldClientSlot = (byte *)&cla->parseClients[(oldindex + old->parseClientsNum) & 0x7ff];
            oldClientNum = *(int *)oldClientSlot;
        }

        if (oldClientNum == newnum) {

            if (CL_ShowNetValue() == 3) {
                Com_Printf("%3i:  delta: %i\n", msg->readcount, oldClientNum);
            }

            cla = (clientActive_t *)cl;
            if (!MSG_ReadDeltaClient(msg, (clientState_t *)oldClientSlot,
                                     &cla->parseClients[cla->parseClientsNum & 0x7ff],
                                     oldClientNum)) {
                cla->parseClientsNum += 1;
                newSnap->numClients += 1;
            }

            oldindex += 1;
            if (oldindex >= old->numClients) {
                oldClientNum = 0x1869f;
            } else {
                oldClientSlot = (byte *)&cla->parseClients[(oldindex + old->parseClientsNum) & 0x7ff];
                oldClientNum = *(int *)oldClientSlot;
            }
        } else {

            if (CL_ShowNetValue() == 3) {
                Com_Printf("%3i:  baseline: %i\n", msg->readcount, newnum);
            }

            memset(&dummy, 0, sizeof(clientState_t));

            cla = (clientActive_t *)cl;
            if (!MSG_ReadDeltaClient(msg, (clientState_t *)&dummy,
                                     &cla->parseClients[cla->parseClientsNum & 0x7ff],
                                     newnum)) {
                cla->parseClientsNum += 1;
                newSnap->numClients += 1;
            }
        }
    }

    while (oldClientNum != 0x1869f) {
        if (msg->overflowed)
            break;

        if (CL_ShowNetValue() == 3) {
            Com_Printf("%3i:  unchanged: %i\n", msg->readcount, oldClientNum);
        }

        cla = (clientActive_t *)cl;
        memcpy(&cla->parseClients[cla->parseClientsNum & 0x7ff], oldClientSlot, sizeof(clientState_t));
        cla->parseClientsNum += 1;
        newSnap->numClients += 1;

        oldindex += 1;
        if (oldindex >= old->numClients)
            break;

        oldClientSlot = (byte *)&cla->parseClients[(oldindex + old->parseClientsNum) & 0x7ff];
        oldClientNum = *(int *)oldClientSlot;
    }

    if (CL_ShowPacketsEnabled()) {
        Com_Printf("Clients in packet: %i\n", newSnap->numClients);
    }

    if (msg->overflowed) {

        newSnap->valid = 0;
        ZN10LargeLocalD1Ev(&newSnap_large_local);
        return;
    }

    if (newSnap->valid == 0) {
        ZN10LargeLocalD1Ev(&newSnap_large_local);
        return;
    }

    cla = (clientActive_t *)cl;
    {
        int oldMsg = cla->snap.messageNum + 1;
        int serverMessageSequence = newSnap->messageNum;

        if (serverMessageSequence - oldMsg >= 0x20) {
            oldMsg = serverMessageSequence - 0x1f;
        }

        while (oldMsg < serverMessageSequence) {
            cla->snapshots[oldMsg & 0x1f].valid = 0;
            oldMsg++;
        }
    }

    cla = (clientActive_t *)cl;
    cla->oldSnapServerTime = cla->snap.serverTime;

    memcpy(&cla->snap, newSnap, sizeof(clSnapshot_t));

    cla->snap.ping = 999;

    {
        int parseEntNum = ((clientConnection_t *)*clc_ptr)->netchan.outgoingSequence;
        int snapTime = cla->snap.ps.commandTime;
        int i;

        for (i = 1; i < 0x21; i++) {
            int idx = (parseEntNum - i) & 0x1f;
            outPacket_t *slot = &cla->outPackets[idx];
            if (slot->p_serverTime >= snapTime) {
                cla->snap.ping = *(int *)(*(byte **)cls_ptr + 0x118) - slot->p_realtime;
                break;
            }
        }
    }

    cla = (clientActive_t *)cl;
    memcpy(&cla->snapshots[cla->snap.messageNum & 0x1f], &cla->snap, sizeof(clSnapshot_t));

    if (CL_ShowNetValue() == 3) {
        Com_Printf("   snapshot:%i  delta:%i  ping:%i\n",
                   cla->snap.messageNum, cla->snap.deltaNum, cla->snap.ping);
    }

    cla->newSnapshots = 1;

    ZN10LargeLocalD1Ev(&newSnap_large_local);
}

void CL_ParseServerMessage(msg_t *msg)
{
    LargeLocal msgCompressed_buf_large_local;
    byte *msgCompressed_buf;
    msg_t msgCompressed;
    int cmd;

    LargeLocal_LargeLocal(&msgCompressed_buf_large_local, MAX_MSG_DECOMPRESS_BYTES);
    msgCompressed_buf = (byte *)LargeLocal_GetBuf(&msgCompressed_buf_large_local);

    if (CL_ShowNetValue() == 1) {

        Com_Printf("%i ", msg->cursize);
    } else if (CL_ShowNetValue() > 1) {

        Com_Printf("------------------\n");
    }

    MSG_Init(&msgCompressed, msgCompressed_buf, MAX_MSG_DECOMPRESS_BYTES);

    msgCompressed.cursize = MSG_ReadBitsCompress(msg->data + msg->readcount,
                                                 msgCompressed_buf,
                                                 msg->cursize - msg->readcount);

    while (!msgCompressed.overflowed) {

        cmd = MSG_ReadByte(&msgCompressed);

        if (cmd == 7) {

            if (CL_ShowNetValue() > 1) {
                Com_Printf("%3i:%s\n", msgCompressed.readcount - 1, "END OF MESSAGE");
            }
            break;
        }

        if (CL_ShowNetValue() > 1) {
            if (svc_strings[cmd] == NULL) {
                Com_Printf("%3i:BAD CMD %i\n", msgCompressed.readcount - 1, cmd);
            } else {
                Com_Printf("%3i:%s\n", msgCompressed.readcount - 1, svc_strings[cmd]);
            }
        }

        switch (cmd) {
        case 0:

            break;
        case 1: {

            CL_ParseGamestate(&msgCompressed);
            break;
        }
        case 4: {

            int seq = MSG_ReadLong(&msgCompressed);
            char *str = MSG_ReadString(&msgCompressed);
            clientConnection_t *clc = (clientConnection_t *)*clc_ptr;
            if (seq <= clc->serverCommandSequence)
                break;

            clc->serverCommandSequence = seq;

            I_strncpyz(clc->serverCommands[seq & 0x7f], str, 1024);
            break;
        }
        case 5:

            CL_ParseDownload(&msgCompressed);
            break;
        case 6:

            CL_ParseSnapshot(&msgCompressed);
            break;
        default:

            Com_Error(1, "\x15"
                         "CL_ParseServerMessage: Illegible server message %d\n",
                      cmd);
            break;
        }
    }

    ZN10LargeLocalD1Ev(&msgCompressed_buf_large_local);
}
