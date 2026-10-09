#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include "cod2_feature_config.h"
#include "pb_public.h"
#include "www_download.h"

extern const dvar_t *Dvar_RegisterBool(const char *dvarName, unsigned char value, unsigned short flags);
extern int I_strnicmp(const char *s0, const char *s1, size_t n);
extern char *strupr(char *s);

COD2_ASSERT_FIELD(LegacyHacks, cl_running,                 0x4);
COD2_ASSERT_FIELD(LegacyHacks, cl_stance,                  0x8);
COD2_ASSERT_FIELD(LegacyHacks, cl_downloadSize,            0x10);
COD2_ASSERT_FIELD(LegacyHacks, cl_downloadCount,           0x14);
COD2_ASSERT_FIELD(LegacyHacks, cl_downloadTime,            0x18);
COD2_ASSERT_FIELD(LegacyHacks, cl_downloadName,            0x1c);
COD2_ASSERT_FIELD(LegacyHacks, cl_serverloadmap,           0x5c);
COD2_ASSERT_FIELD(LegacyHacks, cl_serverloadgametype,      0x9c);
COD2_ASSERT_FIELD(LegacyHacks, cl_serverloadwaiting,       0xdc);
COD2_ASSERT_FIELD(LegacyHacks, ui_scriptMenuAllowResponse, 0x4ed);
COD2_ASSERT_FIELD(clientActive_t, timeoutcount, 0x14);

#if COD2_IS_PATCH_13

const dvar_t *cl_punkbuster;
const dvar_t *cl_wwwDownload;

extern void CL_WWWDownload(void);
extern struct clientStatic_t cls;

int CL_ClearStaticDownload(void)
{
    cls.downloadRestart = 0;
    cls.downloadTempName[0] = 0;
    cls.downloadName[0] = 0;
    cls.originalDownloadName[0] = 0;
    cls.wwwDlInProgress = 0;
    cls.wwwDlDisconnected = 0;
    cls.downloadFlags = 0;
    DL_CancelDownload();
    return 0;
}
#endif

extern Bool Sys_IsMainThread(void);
extern int UI_IsFullscreen(void);
extern void Com_Printf(const char *fmt, ...);
extern const char *FS_LoadedIwdNames(void);
extern const char *FS_ReferencedIwdNames(void);
extern Bool Voice_IsClientTalking(int clientIndex);
extern void Com_SetRecommended(int);
extern void Sys_StartProcess(const char *exeName, int doexit);
extern qboolean UI_SetActiveMenu(int menu);
extern void Sys_ShowIP(void);
extern void Com_Error(int code, const char *fmt, ...);
extern void FS_FCloseFile(fileHandle_t f);
extern void MSG_WriteReliableCommandToBuffer(const char *cmd, char *buf, int bufSize);
extern int FS_Write(const void *buffer, int len, int f);
extern void CL_ShutdownCGame(void);
extern qboolean CL_ShutdownUI(void);
extern void Dvar_SetInt(const dvar_t *dvar, int value);
extern void Dvar_SetBool(const dvar_t *dvar, int value);
extern void Dvar_SetString(const dvar_t *dvar, const char *value);
extern void UI_SetMap(const char *mapname, const char *gametype);
extern void SCR_UpdateScreenInternal(void);
extern void SetScreenScaling(float safeAreaH, float safeAreaV, int vpX, int vpY, int vpW, int vpH);
extern void CalcSplitScreenTextOffset(FontHandle font, float *y);
extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);
extern void SND_Init(void);
extern void SND_StopSounds(int);
extern void SND_FadeAllSounds(float, int);
extern void SND_Shutdown(void);
extern void SND_Save(void *memFile);
extern void SND_Restore(void *memFile);
extern void SND_SaveListeners(void *listeners);
extern void SND_RestoreListeners(void *listeners);
extern void SND_DisconnectListener(void);
extern void CL_InitUI(void);
extern void Sys_LoadingKeepAlive(void);
extern void SCR_Init(void);
extern void SCR_StopCinematic(void);
extern void Con_Init(void);
extern void Con_Close(void);
extern void CL_PlayCinematic_f(void);
extern void CL_Connect_f(void);
extern void CL_GlobalServers_f(void);
extern void CL_Rcon_f(void);
extern void CL_Ping_f(void);
extern void CL_ServerStatus_f(void);
extern void CL_UpdateLevelHunkUsage(void);
extern void SCR_UpdateScreen(void);
extern void CL_CubemapShot_f(void);
extern void Com_WriteLocalizedSoundAliasFiles(void);
extern void UI_OpenMenu_f(void);
extern void UI_CloseMenu_f(void);
extern void CL_InitInput(void);
extern void CL_ShutdownInput(void);
extern void CL_RconInit(void);
extern void CL_ClearMutedList(void);
extern void CL_SetADS(int);
extern void CL_SetCGameTime(void);
extern qboolean CG_IsConsoleCommandName(const char *cmd);
extern void CL_SendCmd(void);
extern void CL_WritePacket(void);
extern void CL_UpdateColor(void);
extern void CL_InitCGame(void);
extern void UI_CloseAll(void);
extern void UI_DrawConnectScreen(void);
extern void Cbuf_Execute(void);
extern void Cbuf_ExecuteText(int, const char *);
extern void Com_DPrintf(const char *fmt, ...);
extern void Com_PrintMessage(int, const char *);
extern void Com_Restart(void);
extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int defaultValue, int flags);
extern const dvar_t *Dvar_RegisterInt(const char *name, int defaultValue, int min, int max, int flags);
extern const dvar_t *Dvar_RegisterFloat(const char *name, float defaultValue, float min, float max, int flags);
extern const dvar_t *Dvar_RegisterString_mac(const char *name, const char *defaultValue, int flags);
extern Bool Dvar_GetBool(const char *name);
extern void Cmd_AddCommand(const char *name, void (*cmd)(void));
extern void Cmd_RemoveCommand(const char *name);
extern void Cmd_SetAutoComplete(const char *cmd, const char *dir, const char *ext);
extern char *Cmd_Args(int start);
extern int atoi(const char *);
extern double atof(const char *);
extern int I_stricmp(const char *a, const char *b);
extern int I_strncmp(const char *a, const char *b, int n);
extern void Info_SetValueForKey(char *s, const char *key, const char *value);
extern const char *Info_ValueForKey(const char *s, const char *key);
extern void Com_Memset(void *dest, int val, int count);
extern int NET_CompareAdr(netadr_t a, netadr_t b);
extern int NET_CompareBaseAdr(netadr_t a, netadr_t b);
extern const char *NET_AdrToString(netadr_t a);
extern int NET_StringToAdr(const char *s, void *a);
extern Bool NET_OutOfBandPrint(netsrc_t sock, netadr_t adr, const char *data);
extern Bool NET_OutOfBandData(netsrc_t sock, netadr_t adr, const void *data, int len);
extern int Sys_IsLANAddress(netadr_t adr);
extern int Sys_Milliseconds(void);
extern int Sys_MillisecondsRaw(void);
extern void srand(unsigned int seed);
extern int FS_FileExists(const char *path);
extern int FS_FOpenFileWrite(const char *path);
extern int FS_FOpenFileRead(const char *path, int *file, int uniqueFILE);
extern int FS_Read(void *buffer, int len, int f);
extern qboolean FS_ConditionalRestart(int checksumFeed);
extern void FS_Restart(int checksumFeed);
extern const char *FS_ReferencedIwdPureChecksums(void);
extern int FS_CompareIwds(char *buf, int bufLen, int flag);
extern char * FS_ShiftStr(const char *name, int shift);
extern void MSG_Init(void *msg, void *data, int length);
extern void MSG_WriteLong(void *msg, int value);
extern void MSG_WriteShort(void *msg, int value);
extern void MSG_WriteByte(void *msg, int value);
extern void MSG_WriteBigString(void *msg, const char *s);
extern void MSG_WriteDeltaEntity(void *msg, void *from, void *to, int force);
extern int MSG_ReadLong(msg_t *msg);
extern const char *MSG_ReadStringLine(msg_t *msg);
extern const char *MSG_ReadBigString(msg_t *msg);
extern void MSG_BeginReading(msg_t *msg);
extern int MSG_WriteBitsCompress(void *src, void *dst, int len);
extern void CL_SetServerInfoByAddress(netadr_t from, const char *info, int ping);
extern void Netchan_Setup(netsrc_t sock, netchan_t *chan, netadr_t adr, int qport);
extern int Netchan_Process(netchan_t *chan, msg_t *msg);
extern void CL_Netchan_AddOOBProfilePacket(int size);
extern void CL_Netchan_Decode(byte *data, int len);
extern void CL_Netchan_SendOOBPacket(int len, const void *data, netadr_t to);
extern int CL_CDKeyValidate(const char *cdkey, const char *checksum);
extern void CL_ServerInfoPacket(netadr_t from, msg_t *msg, int time);
extern void CL_ServerStatusResponse(netadr_t from, msg_t *msg);
extern void CL_ServersResponsePacket(netadr_t from, msg_t *msg);
extern const char *CL_GetConfigString(int index);
extern void CL_ParseServerMessage(msg_t *msg);
extern void CL_ArchiveClientState(void *memFile);
extern void LargeLocal_LargeLocal(void *ll, int size);
extern void *LargeLocal_GetBuf(void *ll);
extern void ZN10LargeLocalD1Ev(void *ll);
extern void Cmd_TokenizeString(const char *text);
extern refexport_t *GetRefAPI(int apiVersion, refimport_t *rimp);
extern void MemFile_InitForWriting(MemoryFile *mf, int size, void *buf, int flag);
extern void MemFile_InitForReading(MemoryFile *mf, int size, void *buf);
extern void *Z_MallocInternal(int size);
extern void *Z_VirtualAllocInternal(int size);
extern void Z_VirtualFreeInternal(void *ptr);
extern void CG_CalculateFPS(void);
extern int Voice_GetLocalVoiceData(ClientVoicePacket_t *dest);
extern void Voice_Playback(void);
extern void SEH_UpdateLanguageInfo(void);
extern const char *SEH_LocalizeTextMessage(const char *ref, const char *defaultText, int flags);
extern char *strchr(const char *, int);
extern float floorf(float);
extern int puts(const char *);

extern refexport_t re;
extern const clientActive_t *cl;
extern const clientConnection_t *clc;
extern struct clientStatic_t cls;
extern const dvar_t *cl_nodelta;
extern const dvar_t *cl_noprint;
extern const dvar_t *cl_maxpackets;
extern const dvar_t *cl_packetdup;
extern const dvar_t *cl_shownet;
extern const dvar_t *cl_shownuments;
extern const dvar_t *cl_showSend;
extern const dvar_t *cl_showServerCommands;
extern const dvar_t *cl_showTimeDelta;
extern const dvar_t *cl_freezeDemo;
extern const dvar_t *cl_sensitivity;
extern const dvar_t *cl_freelook;
extern const dvar_t *cl_mouseAccel;
extern const dvar_t *cl_showMouseRate;
extern const dvar_t *m_pitch;
extern const dvar_t *m_yaw;
extern const dvar_t *m_forward;
extern const dvar_t *m_side;
extern const dvar_t *m_filter;
extern const dvar_t *cl_activeAction;
extern const dvar_t *cl_allowDownload;
extern const dvar_t *cl_inGameVideo;
extern const dvar_t *fx_enable;
extern const dvar_t *fx_draw;
extern const dvar_t *fx_cull;
extern const dvar_t *fx_sort;
extern const dvar_t *fx_debug;
extern const dvar_t *fx_debugBolt;
extern const dvar_t *fx_freeze;
extern const dvar_t *fx_count;
extern const dvar_t *fx_visMinTraceDist;
extern const dvar_t *fx_profile;
extern const dvar_t *nextdemo;
extern const dvar_t *cl_timeout;
extern const dvar_t *cl_connectTimeout;
extern const dvar_t *cl_avidemo;
extern const dvar_t *cl_forceavidemo;
extern const dvar_t *cl_motdString;
extern const dvar_t *cl_serverStatusResendTime;
extern const dvar_t *cl_ingame;
extern const dvar_t *name;
extern const dvar_t *cl_voice;
extern clientActive_t clients[1];
extern clientConnection_t clientConnections[1];
extern void Com_ClientDObjClearAllSkel(void);
extern Bool g_waitingForServer;
extern ping_t cl_pinglist[16];
extern unsigned int frame_msec;
extern int old_com_frameTime;
static char szServerIPAddress[128];
static qboolean recursive;
static const dvar_t *input_viewSensitivity;

void CL_SwitchToLocalClient(int clientNum);
Bool CL_GetLocalClientActive(int clientNum);
int CL_GetLocalClientActiveCount(void);
Bool CL_AllLocalClientsDisconnected(void);
Bool CL_AnyLocalClientChallenging(void);
Bool CL_IsRenderingSplitScreen(void);
const char *CL_GetUsernameForLocalClient(int controllerIndex);
void CL_AddReliableCommand(const char *cmd);
void CL_StopRecord_f(void);
void CL_ShutdownDemo(void);
int CL_GetSkelTimeStamp(int localClientNum);
char *CL_AllocSkelMemory(int localClientNum, unsigned int size);
void CL_ResetSkeletonCache(int localClientNum);
void CL_ClearState(void);
void CL_Setenv_f(void);
void CL_Reconnect_f(void);
void CL_OpenedIWDList_f(void);
void CL_ReferencedIWDList_f(void);
void CL_Configstrings_f(void);
void CL_Clientinfo_f(void);
void CL_VoicePacket(msg_t *msg);
Bool CL_IsPlayerTalking(int clientIndex);
void CL_SetupForNewServerMap(const char *pszMapName, const char *pszGametype);
void CL_VoiceTransmit(void);
Bool Voice_SendVoiceData(void);
void CL_SyncGpu(void);
void CL_SetRecommended_f(void);
void CL_RefPrintf(int print_level, const char *fmt, ...);
void CL_ShutdownRef(void);
void CL_InitRenderer(void);
void CL_StartHunkUsers(void);
int CL_ScaledMilliseconds(void);
void CL_InitRef(void);
void CL_startSingleplayer_f(void);
void CL_StopLogo(void);
void CL_ToggleMenu_f(void);
void CL_OpenScriptMenu_f(void);
void CL_InitOnceForAllClients(void);
void CL_ShutdownDebugData(void);
void CL_GetPing(int n, char *buf, int buflen, int *pingtime);
void CL_ShowIP_f(void);
const char *CL_GetServerIPAddress(void);
void CL_FlushDebugData(qboolean fromServer);
void CL_UpdateDebugData(void);
int CL_TextWidth(const char *text, int maxChars, FontHandle font);
int CL_TextHeight(FontHandle font);
float CL_NormalizedTextScale(FontHandle font, float scale);
void CL_DrawTextPhysical(const char *text, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style);
void CL_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float xScale, float yScale, const vec_t *color, int style);
void CL_DrawTextPhysicalWithCursor(const char *text, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style, int cursorPos, int cursor);
void CL_DrawTextWithCursor(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float xScale, float yScale, const vec_t *color, int style, int cursorPos, int cursor);
int CL_GetKeyCatchers(void);
Bool CL_GetDisplayHUDWithKeycatchUI(void);
FontHandle CL_RegisterFont(const char *fontName, int imageTrack);
void CL_WriteDemoMessage(msg_t *msg, int headerBytes);
void CL_Record_f(void);
void CL_ShutdownHunkUsers(void);
void CL_InitLoad(const char *mapname, const char *gametype);
void CL_DrawLogo(void);
void CL_StopLogoOrCinematic(void);
void CL_AddDebugLine(const vec_t *start, const vec_t *end, const vec_t *color, qboolean depthTest, int duration, qboolean fromServer);
void CL_AddDebugString(const vec_t *xyz, const vec_t *color, float scale, const char *text, qboolean fromServer);
void CL_RequestAuthorization(void);
void CL_CheckForResend(void);
void CL_LocalServers_f(void);
void CL_PlayLogo_f(void);
qboolean CL_UpdateDirtyPings_f(int source);
void CL_RunOncePerClientFrame(int msec);
int Client_SendVoiceData(int bytes, char *enc_buffer);
Bool CL_ConnectionlessPacket(netadr_t from, msg_t *msg, int time);
Bool CL_PacketEvent(netadr_t from, msg_t *msg, int time);
void CL_Init(void);
void CL_ShutdownAll(void);
void CL_Frame(int msec);
void CL_Vid_Restart_f(void);
void CL_Snd_Restart_f(void);
void CL_Disconnect(void);
void CL_Shutdown(void);
void CL_Disconnect_f(void);
void CL_MapLoading(const char *mapname);
void CL_DemoCompleted(void);
void CL_ReadDemoMessage(void);
void CL_PlayDemo_f(void);
void CL_ForwardCommandToServer(const char *string);
void CL_ForwardToServer_f(void);
void CL_DownloadsComplete(void);
void CL_BeginDownload(const char *localName, const char *remoteName);
void CL_NextDownload(void);
void CL_InitDownloads(void);
static void CL_SendPureChecksums(void);

void CL_SwitchToLocalClient(int clientNum)
{

}

Bool CL_GetLocalClientActive(int clientNum)
{
    return 1;
}

int CL_GetLocalClientActiveCount(void)
{
    return 1;
}

Bool CL_AllLocalClientsDisconnected(void)
{
    if (!Sys_IsMainThread())
        return 1;
    if (UI_IsFullscreen())
        return 1;
    if (*(byte *)&clients[0] == 0)
        return 1;
    if (*(int *)&clientConnections[0] <= 2)
        return 1;
    return 0;
}

Bool CL_AnyLocalClientChallenging(void)
{
    if (*(byte *)&clients[0] != 0 && *(int *)&clientConnections[0] == 4)
        return 1;
    return 0;
}

Bool CL_IsRenderingSplitScreen(void)
{
    return 0;
}

const char *CL_GetUsernameForLocalClient(int controllerIndex)
{
    return name->current.string;
}

static inline __attribute__((always_inline)) void CL_AddReliableCommand_core(const char *cmd)
{
    clientConnection_t *conn;
    int index;
    if (clientConnections[0].reliableSequence - clientConnections[0].reliableAcknowledge - 128 > 0) {
        Com_Error(1, (const char *)"EXE_ERR_CLIENT_CMD_OVERFLOW");
    }
    conn = (clientConnection_t *)clc;
    conn->reliableSequence += 1;
    index = conn->reliableSequence & 0x7f;
    MSG_WriteReliableCommandToBuffer(cmd, (char *)conn->reliableCommands[index], 0x400);
}

void CL_AddReliableCommand(const char *cmd)
{
    CL_AddReliableCommand_core(cmd);
}

void CL_StopRecord_f(void)
{
    clientConnection_t *cc = &clientConnections[0];
    clientConnection_t *conn;
    int len;

    if (!cc->demorecording) {
        Com_Printf("Not recording a demo.\n");
        return;
    }

    len = -1;
    FS_Write(&len, 4, cc->demofile);
    conn = (clientConnection_t *)clc;
    FS_Write(&len, 4, conn->demofile);
    FS_FCloseFile(conn->demofile);
    conn->demofile = 0;
    conn->demorecording = 0;
    Com_Printf("Stopped demo.\n");
}

void CL_ShutdownDemo(void)
{
    clientConnection_t *conn;

    if (!clientConnections[0].demofile)
        return;
    FS_FCloseFile(clientConnections[0].demofile);
    conn = (clientConnection_t *)clc;
    conn->demofile = 0;
    conn->demoplaying = 0;
    conn->demorecording = 0;
}

int CL_GetSkelTimeStamp(int localClientNum)
{
    clientActive_t *client = &clients[localClientNum];
    return client->skelTimeStamp;
}

char *CL_AllocSkelMemory(int localClientNum, unsigned int size)
{
    clientActive_t *client = &clients[localClientNum];
    int pos = client->skelMemPos;
    char *buf = (char *)(pos + (int)client->skelMemoryStart);
    int newPos = pos + ((size + 15) & ~15);
    client->skelMemPos = newPos;
    if (newPos >= 0x3fff1)
        return 0;
    return buf;
}

void CL_ResetSkeletonCache(int localClientNum)
{
    clientActive_t *client = &clients[localClientNum];
    unsigned int count = client->skelTimeStamp + 1;
    if (!count)
        count = 1;
    client->skelTimeStamp = count;
    client->skelMemoryStart = (char *)(((int)client->skelMemory + 15) & ~15);
    client->skelMemPos = 0;
}

void CL_ClearState(void)
{
    clientActive_t *restorePtr;
    Bool cgameInitialized, cgameInitCalled, displayHUD, active;
    int keyCatchers;

    cgameInitialized = clients[0].cgameInitialized;
    cgameInitCalled = clients[0].cgameInitCalled;
    keyCatchers = clients[0].keyCatchers;
    displayHUD = clients[0].displayHUDWithKeycatchUI;
    active = clients[0].active;

    memset(&clients[0], 0, sizeof(clientActive_t));

    restorePtr = (clientActive_t *)cl;
    restorePtr->cgameInitialized = cgameInitialized;
    restorePtr->cgameInitCalled = cgameInitCalled;
    restorePtr->keyCatchers = keyCatchers;
    restorePtr->displayHUDWithKeycatchUI = displayHUD;
    restorePtr->active = active;

    Com_ClientDObjClearAllSkel();
}

extern int Cmd_Argc(void);
extern char *Cmd_Argv(int arg);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern void I_strncat(char *dest, int maxlen, const char *src);
extern int putenv(const char *string);
extern char *getenv(const char *name);
void CL_Setenv_f(void)
{
    char buffer[1024];
    int argc, i;
    const char *env;

    argc = Cmd_Argc();

    if (argc > 2) {

        I_strncpyz(buffer, Cmd_Argv(1), 0x400);
        I_strncat(buffer, 0x400, "=");
        for (i = 2; i < argc; i++) {
            I_strncat(buffer, 0x400, Cmd_Argv(i));
            I_strncat(buffer, 0x400, " ");
        }
        putenv(buffer);
    } else if (argc == 2) {

        env = getenv(Cmd_Argv(1));
        if (env)
            Com_Printf("%s=%s\n", Cmd_Argv(1), env);
        else
            Com_Printf("%s undefined\n", Cmd_Argv(1));
    }
}

extern void Cbuf_AddText(const char *text);
extern const char *va(const char *fmt, ...);
void CL_Reconnect_f(void)
{
    char *server = cls.servername;

    if (!*server || !memcmp(server, "localhost", 10)) {
        Com_Printf("Can't reconnect to localhost.\n");
        return;
    }

    Cbuf_AddText(va("connect %s\n", server));
}

void CL_OpenedIWDList_f(void)
{
    Com_Printf("Opened IWD Names: %s\n", FS_LoadedIwdNames());
}

void CL_ReferencedIWDList_f(void)
{
    Com_Printf("Referenced IWD Names: %s\n", FS_ReferencedIwdNames());
}

void CL_Configstrings_f(void)
{
    int i;
    clientActive_t *c;
    int offset;

    if (clientConnections[0].state != 8) {
        Com_Printf("Not connected to a server.\n");
        return;
    }

    c = (clientActive_t *)cl;
    for (i = 0; i < 0x800; i++) {
        offset = c->gameState.stringOffsets[i];
        if (offset)
            Com_Printf("%4i: %s\n", i, c->gameState.stringData + offset);
    }
}

extern const char *Dvar_InfoString(int bit);
extern void Info_Print(const char *s);
void CL_Clientinfo_f(void)
{

    Com_Printf("--------- Client Information ---------\n");
    Com_Printf("state: %i\n", *(int *)clc);
    Com_Printf("Server: %s\n", cls.servername);
    Com_Printf("User info settings:\n");
    Info_Print(Dvar_InfoString(2));
    Com_Printf("--------------------------------------\n");
}

extern int MSG_ReadByte(msg_t *msg);
extern void MSG_ReadData(msg_t *msg, void *data, int len);
extern Bool CL_IsPlayerMuted(int clientNum);
extern void Voice_IncomingVoiceData(int talker, const byte *data, int dataLen);
void CL_VoicePacket(msg_t *msg)
{
    byte voiceData[256];
    int numPackets, packet, talker, dataLen;

    numPackets = MSG_ReadByte(msg);
    if (numPackets > 40 || numPackets <= 0)
        return;

    for (packet = 0; packet < numPackets; packet++) {
        talker = MSG_ReadByte(msg);
        dataLen = MSG_ReadByte(msg);

        if (dataLen < 1 || dataLen > 256) {
            Com_Printf("Invalid server voice packet of %i bytes\n", dataLen);
            return;
        }

        MSG_ReadData(msg, voiceData, dataLen);

        if (talker > 63) {
            Com_Printf("Invalid voice packet - talker was %i\n", talker);
            return;
        }

        if (CL_IsPlayerMuted(talker))
            continue;
        if (!cl_voice->current.enabled)
            continue;

        Voice_IncomingVoiceData(talker, voiceData, dataLen);
    }
}

Bool CL_IsPlayerTalking(int clientIndex)
{
    return Voice_IsClientTalking(clientIndex);
}

void CL_SetupForNewServerMap(const char *pszMapName, const char *pszGametype)
{
    LegacyHacks *hacks = *(LegacyHacks **)imp_legacyHacks;

    Com_Printf("Server changing map %s, gametype %s\n", pszMapName, pszGametype);
    I_strncpyz(hacks->cl_serverloadmap, pszMapName, 0x40);
    I_strncpyz(hacks->cl_serverloadgametype, pszGametype, 0x40);
    hacks->cl_serverloadwaiting = 0;

    if (!(*(const dvar_t **)imp_com_sv_running)->current.enabled) {
        Dvar_SetInt(*(const dvar_t **)imp_com_expectedHunkUsage, 0);
        g_waitingForServer = 1;
    }

    UI_SetMap(pszMapName, pszGametype);
    SCR_UpdateScreenInternal();
}

extern void CL_WriteVoicePacket(void);
void CL_VoiceTransmit(void)
{
    int voiceLen = clients[0].voicePacketCount;
    clientActive_t *c;

    if (voiceLen <= 0)
        return;

    if (clients[0].serverTime - clients[0].voicePacketLastTransmit > 199 || voiceLen > 9) {
        CL_WriteVoicePacket();
        c = (clientActive_t *)cl;
        c->voicePacketCount = 0;
        c->voicePacketLastTransmit = c->serverTime;
    }
}

extern int Dvar_GetInt(const char *name);
extern Bool IsTalking(void);
Bool Voice_SendVoiceData(void)
{

    if (!(*(const dvar_t **)imp_sv_voice)->current.enabled)
        return 0;
    if (!cl_voice->current.enabled)
        return 0;

    if (Dvar_GetInt("rate") <= 0x1387)
        return 0;

    if (clc->state != 8)
        return 0;

    if ((*(const dvar_t **)imp_cl_talking)->current.enabled)
        return 1;
    if (IsTalking())
        return 1;

    {
        int vpc = cl->voicePacketCount;
        if (vpc)
            return 1;
    }

    return 0;
}

void CL_SyncGpu(void)
{
    re.SyncGpu();
}

void CL_SetRecommended_f(void)
{
    Com_SetRecommended(1);
}

void CL_RefPrintf(int print_level, const char *fmt, ...)
{
    char msg[4096];
    __builtin_va_list argptr;

    __builtin_va_start(argptr, fmt);
    vsnprintf(msg, sizeof(msg), fmt, argptr);
    __builtin_va_end(argptr);

    switch (print_level) {
    case 0:
        Com_Printf("%s", msg);
        break;
    case 1:
        Com_DPrintf("^1%s", msg);
        break;
    case 2:
        Com_Printf("^3%s", msg);
        break;
    case 3:
        Com_Printf("^1%s", msg);
        break;
    default:
        break;
    }
}

extern void StatMon_Reset(void);
extern refexport_t re;
void CL_ShutdownRef(void)
{
    void (*syncRenderThread)(void) = re.SyncRenderThread;

    if (syncRenderThread)
        syncRenderThread();

    if (re.Shutdown) {
        ((void (*)(int))re.Shutdown)(1);
        memset(&re, 0, sizeof(re));
    }

    StatMon_Reset();
}

void CL_InitRenderer(void)
{
    int fieldWidth;

    re.BeginRegistration(&cls.vidConfig);
    {
        int vw = cls.vidConfig.width;
        int vh = cls.vidConfig.height;
        SetScreenScaling(1.0f, 1.0f, 0, 0, vw, vh);
    }

    cls.whiteMaterial = re.RegisterMaterial("white", 3, 3);
    cls.consoleMaterial = re.RegisterMaterial("console", 3, 3);
    cls.consoleFont = re.RegisterFont("fonts/consoleFont", 3);

    fieldWidth = cls.vidConfig.width - 0x20;
    *(int *)imp_g_console_field_width = fieldWidth;

    {
        field_t *cf = (field_t *)imp_g_consoleField;
        cf->widthInPixels = fieldWidth;
        *(float *)&cf->charHeight = *(float *)imp_g_console_char_height;
        cf->fixedSize = 1;
    }

    StatMon_Reset();
}

void CL_StartHunkUsers(void)
{
    LegacyHacks *hacks = *(LegacyHacks **)imp_legacyHacks;

    if (!hacks->cl_running)
        return;

    if (!cls.soundStarted) {
        cls.soundStarted = 1;
        SND_Init();
        Sys_LoadingKeepAlive();
    }

    if (!cls.rendererStarted) {
        cls.rendererStarted = 1;
        CL_InitRenderer();
        Sys_LoadingKeepAlive();
    }

    if (!cls.uiStarted) {
        CL_InitUI();
        Sys_LoadingKeepAlive();
    }

    cls.hunkUsersStarted = 1;
}

int CL_ScaledMilliseconds(void)
{
    return cls.realtime;
}

#ifdef COD2_GFX_DLL

extern void *__stdcall LoadLibraryA(const char *name);
extern void *__stdcall GetProcAddress(void *mod, const char *proc);
extern unsigned long __stdcall GetLastError(void);
void *g_gfxRendererModule;
refexport_t *CL_LoadRendererDll(int apiVersion, refimport_t *ri_in)
{
    const char *name = "gfx_d3d_mp_x86_s.dll";
    void *h;
    refexport_t *(*pGetRefAPI)(int, refimport_t *);
    h = LoadLibraryA(name);
    if (!h) {
        Com_Error(ERR_FATAL, "CL_InitRef: could not load renderer DLL '%s' (err %lu)",
                  name, (unsigned long)GetLastError());
        return NULL;
    }
    pGetRefAPI = (refexport_t * (*)(int, refimport_t *)) GetProcAddress(h, "GetRefAPI");
    if (!pGetRefAPI) {
        Com_Error(ERR_FATAL, "CL_InitRef: '%s' has no GetRefAPI export", name);
        return NULL;
    }
    Com_Printf("Loaded renderer module: %s\n", name);
    g_gfxRendererModule = h;
    return pGetRefAPI(apiVersion, ri_in);
}
#endif

int g_gfxV60DllActive = 0;

void *g_gfxV60AddRefEntity = 0;

#if defined(COD2_GFX_DLL) && defined(COD2_GFX_DLL_RETAIL_V60)
#    include "PC/win32/gfx_dll_v60.h"
#endif

void CL_InitRef(void)
{
    refimport_t ri_local;

    Com_Printf("----- Initializing Renderer ----\n");

    ri_local.Cmd_AddCommand = (void *)imp_Cmd_AddCommand;
    ri_local.Cmd_RemoveCommand = (void *)imp_Cmd_RemoveCommand;
    ri_local.Cmd_Argc = (void *)imp_Cmd_Argc;
    ri_local.Cmd_Argv = (void *)imp_Cmd_Argv;
    ri_local.Cbuf_ExecuteText = (void *)imp_Cbuf_ExecuteText;
    ri_local.Printf = (void *)CL_RefPrintf;
    ri_local.Error = (void *)imp_Com_Error;
    ri_local.Milliseconds = (void *)CL_ScaledMilliseconds;
    ri_local.Hunk_AllocInternal = (void *)imp_Hunk_AllocInternal;
    ri_local.Hunk_AllocateTempMemoryInternal = (void *)imp_Hunk_AllocateTempMemoryInternal;
    ri_local.Z_MallocInternal = (void *)imp_Z_MallocInternal;
    ri_local.Z_FreeInternal = (void *)imp_Z_FreeInternal;
    ri_local.Hunk_AllocAlignInternal = (void *)imp_Hunk_AllocAlignInternal;
    ri_local.Z_VirtualReserveInternal = (void *)imp_Z_VirtualReserveInternal;
    ri_local.Z_VirtualCommitInternal = (void *)imp_Z_VirtualCommitInternal;
    ri_local.Z_VirtualDecommitInternal = (void *)imp_Z_VirtualDecommitInternal;
    ri_local.Z_VirtualFreeInternal = (void *)imp_Z_VirtualFreeInternal;
    ri_local.Hunk_FreeTempMemory = (void *)imp_Hunk_FreeTempMemory;
    ri_local.Hunk_AllocateTempMemoryHighInternal = (void *)imp_Hunk_AllocateTempMemoryHighInternal;
    ri_local.Hunk_ClearTempMemory = (void *)imp_Hunk_ClearTempMemory;
    ri_local.Hunk_ClearTempMemoryHigh = (void *)imp_Hunk_ClearTempMemoryHigh;
    ri_local.Sys_DirectXFatalError = (void *)imp_Sys_DirectXFatalError;
    ri_local.Sys_ShowSplashWindow = (void *)imp_Sys_ShowSplashWindow;
    ri_local.Sys_HideSplashWindow = (void *)imp_Sys_HideSplashWindow;
    ri_local.Sys_LoadingKeepAlive = (void *)imp_Sys_LoadingKeepAlive;
    ri_local.FS_ReadFile = (void *)imp_FS_ReadFile;
    ri_local.FS_FreeFile = (void *)imp_FS_FreeFile;
    ri_local.FS_FOpenFileRead = (void *)imp_FS_FOpenFileRead;
    ri_local.FS_WriteFile = (void *)imp_FS_WriteFile;
    ri_local.FS_FreeFileList = (void *)imp_FS_FreeFileList;
    ri_local.FS_ListFiles = (void *)imp_FS_ListFiles;
    ri_local.FS_FileExists = (void *)imp_FS_FileExists;
    ri_local.FS_FOpenFileByMode = (void *)imp_FS_FOpenFileByMode;
    ri_local.FS_FCloseFile = (void *)imp_FS_FCloseFile;
    ri_local.FS_Read = (void *)imp_FS_Read;
    ri_local.FS_Write = (void *)imp_FS_Write;
    ri_local.CM_SaveLump = (void *)imp_CM_SaveLump;
    ri_local.CM_BoxTrace = (void *)imp_CM_BoxTrace;
    ri_local.CM_BoxSightTrace = (void *)imp_CM_BoxSightTrace;
    ri_local.Dvar_RegisterBool = (void *)imp_Dvar_RegisterBool;
    ri_local.Dvar_RegisterInt = (void *)imp_Dvar_RegisterInt;
    ri_local.Dvar_RegisterFloat = (void *)imp_Dvar_RegisterFloat;
    ri_local.Dvar_RegisterString = (void *)imp_Dvar_RegisterString;
    ri_local.Dvar_RegisterEnum = (void *)imp_Dvar_RegisterEnum;
    ri_local.Dvar_RegisterColor = (void *)imp_Dvar_RegisterColor;
    ri_local.Dvar_RegisterVec2 = (void *)imp_Dvar_RegisterVec2;
    ri_local.Dvar_RegisterVec3 = (void *)imp_Dvar_RegisterVec3;
    ri_local.Dvar_RegisterVec4 = (void *)imp_Dvar_RegisterVec4;
    ri_local.Dvar_ClearModified = (void *)imp_Dvar_ClearModified;
    ri_local.Dvar_SetModified = (void *)imp_Dvar_SetModified;
    ri_local.Dvar_UpdateEnumDomain = (void *)imp_Dvar_UpdateEnumDomain;
    ri_local.Dvar_UnregisterSystem = (void *)imp_Dvar_UnregisterSystem;
    ri_local.Dvar_ChangeResetValue = (void *)imp_Dvar_ChangeResetValue;
    ri_local.Dvar_IsAtDefaultValue = (void *)imp_Dvar_IsAtDefaultValue;
    ri_local.Dvar_SetBool = (void *)imp_Dvar_SetBool;
    ri_local.Dvar_SetInt = (void *)imp_Dvar_SetInt;
    ri_local.Dvar_SetFloat = (void *)imp_Dvar_SetFloat;
    ri_local.Dvar_SetString = (void *)imp_Dvar_SetString;
    ri_local.Dvar_SetColor = (void *)imp_Dvar_SetColor;
    ri_local.Dvar_SetVec2 = (void *)imp_Dvar_SetVec2;
    ri_local.Dvar_SetVec3 = (void *)imp_Dvar_SetVec3;
    ri_local.Dvar_SetVec4 = (void *)imp_Dvar_SetVec4;
    ri_local.Dvar_SetFromString = (void *)imp_Dvar_SetFromString;
    ri_local.Dvar_SetBoolByName = (void *)imp_Dvar_SetBoolByName;
    ri_local.Dvar_SetIntByName = (void *)imp_Dvar_SetIntByName;
    ri_local.Dvar_SetFloatByName = (void *)imp_Dvar_SetFloatByName;
    ri_local.Dvar_SetStringByName = (void *)imp_Dvar_SetStringByName;
    ri_local.Dvar_SetColorByName = (void *)imp_Dvar_SetColorByName;
    ri_local.Dvar_SetVec2ByName = (void *)imp_Dvar_SetVec2ByName;
    ri_local.Dvar_SetVec3ByName = (void *)imp_Dvar_SetVec3ByName;
    ri_local.Dvar_SetVec4ByName = (void *)imp_Dvar_SetVec4ByName;
    ri_local.Dvar_SetFromStringByName = (void *)imp_Dvar_SetFromStringByName;
    ri_local.Dvar_GetBool = (void *)imp_Dvar_GetBool;
    ri_local.Dvar_GetInt = (void *)imp_Dvar_GetInt;
    ri_local.Dvar_GetFloat = (void *)imp_Dvar_GetFloat;
    ri_local.Dvar_GetString = (void *)imp_Dvar_GetString;
    ri_local.Dvar_GetVariantString = (void *)imp_Dvar_GetVariantString;
    ri_local.Dvar_EnumToString = (void *)imp_Dvar_EnumToString;
    ri_local.Dvar_Reset = (void *)imp_Dvar_Reset;
    ri_local.Com_SaveDvarsToBuffer = (void *)imp_Com_SaveDvarsToBuffer;
    ri_local.Com_LoadDvarsFromBuffer = (void *)imp_Com_LoadDvarsFromBuffer;
    ri_local.Com_GetBsp = (void *)imp_Com_GetBsp;
    ri_local.SEH_ReadCharFromString = (void *)imp_SEH_ReadCharFromString;
    ri_local.CL_UpdateDebugData = (void *)CL_UpdateDebugData;
    ri_local.CL_FlushDebugData = (void *)CL_FlushDebugData;
    ri_local.StatMon_Warning = (void *)imp_StatMon_Warning;
    ri_local.XModelPrecache = (void *)imp_XModelPrecache;
    ri_local.XModelGetSurfaces = (void *)imp_XModelGetSurfaces;
    ri_local.XModelBad = (void *)imp_XModelBad;
    ri_local.Hunk_OverrideDataForFile = (void *)imp_Hunk_OverrideDataForFile;
    ri_local.XModelGetNumLods = (void *)imp_XModelGetNumLods;
    ri_local.XModelSetTestLods = (void *)imp_XModelSetTestLods;
    ri_local.XModelGetLodForDist = (void *)imp_XModelGetLodForDist;
    ri_local.XModelGetLodOutDist = (void *)imp_XModelGetLodOutDist;
    ri_local.XModelGetSurfaceName = (void *)imp_XModelGetSurfaceName;
    ri_local.XModelGetName = (void *)imp_XModelGetName;
    ri_local.XModelGetFlags = (void *)imp_XModelGetFlags;
    ri_local.XModelNumBones = (void *)imp_XModelNumBones;
    ri_local.XModelGetSkins = (void *)imp_XModelGetSkins;
    ri_local.XModelGetMemUsage = (void *)imp_XModelGetMemUsage;
    ri_local.XModelGetLodName = (void *)imp_XModelGetLodName;
    ri_local.XModelGetBasePose = (void *)imp_XModelGetBasePose;
    ri_local.XModelGetBasePoseBone = (void *)imp_XModelGetBasePoseBone;
    ri_local.DObjBad = (void *)imp_DObjBad;
    ri_local.DObjGetBounds = (void *)imp_DObjGetBounds;
    ri_local.DObjGetSurface = (void *)imp_DObjGetSurface;
    ri_local.DObjGetNumModels = (void *)imp_DObjGetNumModels;
    ri_local.DObjGetNumSurfaces = (void *)imp_DObjGetNumSurfaces;
    ri_local.DObjGetSurfaces = (void *)imp_DObjGetSurfaces;
    ri_local.DObjGetModel = (void *)imp_DObjGetModel;
    ri_local.DObjGetSurfaceName = (void *)imp_DObjGetSurfaceName;
    ri_local.DObjCreate = (void *)imp_DObjCreate;
    ri_local.DObjGetAllocSkelSize = (void *)imp_DObjGetAllocSkelSize;
    ri_local.DObjCreateSkel = (void *)imp_DObjCreateSkel;
    ri_local.DObjCalcAnim = (void *)imp_DObjCalcAnim;
    ri_local.DObjCalcSkel = (void *)imp_DObjCalcSkel;
    ri_local.DObjGetRotTransArray = (void *)imp_DObjGetRotTransArray;
    ri_local.DObjSkelAreBonesUpToDate = (void *)imp_DObjSkelAreBonesUpToDate;
    ri_local.DObjGetMatOffset = (void *)imp_DObjGetMatOffset;
    ri_local.DObjNumBones = (void *)imp_DObjNumBones;
    ri_local.DObjGetBoneInfo = (void *)imp_DObjGetBoneInfo;
    ri_local.DObjGetLodForDist = (void *)imp_DObjGetLodForDist;
    ri_local.DObjGetLodOutDist = (void *)imp_DObjGetLodOutDist;
    ri_local.DObjCompleteHierarchyBits = (void *)imp_DObjCompleteHierarchyBits;
    ri_local.DObjSetModel = (void *)imp_DObjSetModel;
    ri_local.CIN_UploadCinematic = (void *)imp_CIN_UploadCinematic;
    ri_local.CIN_PlayCinematic = (void *)imp_CIN_PlayCinematic;
    ri_local.CIN_RunCinematic = (void *)imp_CIN_RunCinematic;
    ri_local.CG_DObjCalcPose = (void *)imp_CG_DObjCalcPose;
    ri_local.CL_GetHudMsgIconMaterialName = (void *)imp_CL_GetHudMsgIconMaterialName;
    {

        extern void *CM_GetPlaneNum(int planeNum);
        ri_local.CM_GetPlaneNum = (void *)CM_GetPlaneNum;
    }

#if defined(COD2_GFX_DLL) && defined(COD2_GFX_DLL_RETAIL_V60)

    gfxV60_LoadRetailRenderer(&ri_local, &re);
#elif defined(COD2_GFX_DLL)

    {
        extern refimport_t ri;
        extern int g_gfxV60DllActive;
        extern void *g_gfxV60AddRefEntity;
        memcpy(&ri, &ri_local, sizeof(ri_local));
        memcpy(&re, CL_LoadRendererDll(0x3b, &ri_local), sizeof(re));

        g_gfxV60AddRefEntity = GetProcAddress(g_gfxRendererModule, "R_AddRefEntityToScene");
        g_gfxV60DllActive = 1;
    }
#else

    memcpy(&re, GetRefAPI(0x3b, &ri_local), sizeof(re));
#endif

    Dvar_SetInt(*(const dvar_t **)imp_cl_paused, 0);
}

void CL_startSingleplayer_f(void)
{
    Sys_StartProcess("cod2sp_s.exe", 1);
}

void CL_StopLogo(void)
{
    *(int *)&clientConnections[0] = 0;
}

void CL_ToggleMenu_f(void)
{
    if (clientConnections[0].demoplaying != 0 || (*(LegacyHacks **)imp_legacyHacks)->cl_serverloadwaiting != 0) {
        UI_SetActiveMenu(1);
    } else {
        UI_SetActiveMenu(2);
    }
}

void CL_OpenScriptMenu_f(void)
{
    char *parentMenuName, *menuResponse;
    const char *configStr;
    int menuIndex;

    if (Cmd_Argc() != 3) {
        Com_Printf("USAGE: openscriptmenu <parent menu name> <script menu response>\n");
        Com_Printf("EXAMPLE: openscriptmenu ingame changeweapon\n");
        return;
    }

    if (!(*(LegacyHacks **)imp_legacyHacks)->ui_scriptMenuAllowResponse)
        return;
    if (!cls.uiStarted)
        return;

    parentMenuName = Cmd_Argv(1);
    menuResponse = Cmd_Argv(2);

    if (!parentMenuName || !menuResponse)
        return;

    menuIndex = -1;
    for (int i = 0; i < 0x20; i++) {
        configStr = CL_GetConfigString(0x4de + i);
        if (!*configStr)
            continue;
        if (I_stricmp(parentMenuName, configStr) == 0) {
            menuIndex = i;
            break;
        }
    }

    Cbuf_ExecuteText(2, va("cmd mr %i %i %s\n", Dvar_GetInt("sv_serverId"), menuIndex, menuResponse));
}

void CL_InitOnceForAllClients(void)
{
    srand(Sys_MillisecondsRaw());
    Con_Init();
    CL_InitInput();

    cl_noprint = Dvar_RegisterBool_mac("cl_noprint", 0, 0);
    cl_voice = Dvar_RegisterBool_mac("cl_voice", 1, 0x1003);
    cl_timeout = Dvar_RegisterFloat("cl_timeout", 200.0f, 0.0f, 3600.0f, 0);
    cl_connectTimeout = Dvar_RegisterFloat("cl_connectTimeout", 200.0f, 0.0f, 3600.0f, 0);
    cl_shownet = Dvar_RegisterInt("cl_shownet", 0, -2, 4, 0);
    cl_shownuments = Dvar_RegisterBool_mac("cl_shownuments", 0, 0);
    cl_showServerCommands = Dvar_RegisterBool_mac("cl_showServerCommands", 0, 0);
    cl_showSend = Dvar_RegisterBool_mac("cl_showSend", 0, 0);
    cl_showTimeDelta = Dvar_RegisterBool_mac("cl_showTimeDelta", 0, 0);
    cl_freezeDemo = Dvar_RegisterBool_mac("cl_freezeDemo", 0, 0);
    cl_activeAction = Dvar_RegisterString_mac("activeAction", "", 0);
    cl_avidemo = Dvar_RegisterInt("cl_avidemo", 0, 0, 0x7fffffff, 0);
    cl_forceavidemo = Dvar_RegisterBool_mac("cl_forceavidemo", 0, 0);

    {
        union {
            int i;
            float f;
        } flt_max = { 0x7f7fffff }, flt_nmax = { 0xff7fffff };
        *(const dvar_t **)imp_cl_yawspeed = Dvar_RegisterFloat("cl_yawspeed", 140.0f, flt_nmax.f, flt_max.f, 0x1001);
        *(const dvar_t **)imp_cl_pitchspeed = Dvar_RegisterFloat("cl_pitchspeed", 140.0f, flt_nmax.f, flt_max.f, 0x1001);
        *(const dvar_t **)imp_cl_anglespeedkey = Dvar_RegisterFloat("cl_anglespeedkey", 1.5f, 0.0f, flt_max.f, 0);
    }

    cl_maxpackets = Dvar_RegisterInt("cl_maxpackets", 30, 15, 100, 0x1001);
    cl_packetdup = Dvar_RegisterInt("cl_packetdup", 1, 0, 5, 0x1001);
    cl_sensitivity = Dvar_RegisterFloat("sensitivity", 5.0f, 0.01f, 100.0f, 0x1001);
    cl_mouseAccel = Dvar_RegisterFloat("cl_mouseAccel", 0.0f, 0.0f, 100.0f, 0x1001);
    cl_freelook = Dvar_RegisterBool_mac("cl_freelook", 1, 0x1001);
    cl_showMouseRate = Dvar_RegisterBool_mac("cl_showmouserate", 0, 0);
    cl_allowDownload = Dvar_RegisterBool_mac("cl_allowDownload", 1, 0x1001);
#if COD2_IS_PATCH_13
    cl_wwwDownload = Dvar_RegisterBool("cl_wwwDownload", 1, 0x1003);
    cl_punkbuster = Dvar_RegisterBool("cl_punkbuster", 0, 0x1013);
#endif
    *(const dvar_t **)imp_cl_talking = Dvar_RegisterBool_mac("cl_talking", 0, 0);
    cl_inGameVideo = Dvar_RegisterBool_mac("r_inGameVideo", 1, 0x1001);
    cl_serverStatusResendTime = Dvar_RegisterInt("cl_serverStatusResendTime", 750, 0, 3600, 0);
    *(const dvar_t **)imp_cl_bypassMouseInput = Dvar_RegisterBool_mac("cl_bypassMouseInput", 0, 0);

    m_pitch = Dvar_RegisterFloat("m_pitch", 0.022f, -1.0f, 1.0f, 0x1001);
    m_yaw = Dvar_RegisterFloat("m_yaw", 0.022f, -1.0f, 1.0f, 0x1001);
    m_forward = Dvar_RegisterFloat("m_forward", 0.25f, -1.0f, 1.0f, 0x1001);
    m_side = Dvar_RegisterFloat("m_side", 0.25f, -1.0f, 1.0f, 0x1001);
    m_filter = Dvar_RegisterBool_mac("m_filter", 0, 0x1001);
    input_viewSensitivity = Dvar_RegisterFloat("input_viewSensitivity", 1.0f, 0.0001f, 5.0f, 0x1001);
    *(const dvar_t **)imp_cg_drawCrosshair = Dvar_RegisterBool_mac("cg_drawCrosshair", 1, 0x1001);
    cl_motdString = Dvar_RegisterString_mac("cl_motdString", "", 0x1040);
    cl_ingame = Dvar_RegisterBool_mac("cl_ingame", 0, 0x1040);
    Dvar_RegisterInt("cl_maxPing", 800, 20, 2000, 0x1001);
    name = Dvar_RegisterString_mac("name", "Unknown Soldier", 0x1003);
    Dvar_RegisterInt("rate", 5000, 1000, 25000, 0x1003);
    Dvar_RegisterInt("snaps", 20, 1, 30, 0x1003);
    Dvar_RegisterString_mac("password", "", 0x1002);

    fx_enable = Dvar_RegisterBool_mac("fx_enable", 1, 0x1080);
    fx_draw = Dvar_RegisterBool_mac("fx_draw", 1, 0x1080);
    fx_cull = Dvar_RegisterBool_mac("fx_cull", 1, 0);
    fx_sort = Dvar_RegisterBool_mac("fx_sort", 1, 0);
    fx_freeze = Dvar_RegisterBool_mac("fx_freeze", 0, 0x1080);
    fx_debug = Dvar_RegisterBool_mac("fx_debug", 0, 0x1080);
    fx_debugBolt = Dvar_RegisterFloat("fx_debugBolt", 0.0f, 0.0f, 100.0f, 0x1080);
    fx_count = Dvar_RegisterBool_mac("fx_count", 0, 0x1080);
    fx_visMinTraceDist = Dvar_RegisterFloat("fx_visMinTraceDist", 80.0f, 0.0f, 1000.0f, 0x1080);
    fx_profile = Dvar_RegisterBool_mac("fx_profile", 0, 0x1080);
    nextdemo = Dvar_RegisterString_mac("nextdemo", "", 0);
    Dvar_RegisterBool_mac("hud_enable", 1, 0x1001);
    Dvar_RegisterBool_mac("cg_blood", 1, 0x1001);

    Cmd_AddCommand("cmd", CL_ForwardToServer_f);
    Cmd_AddCommand("configstrings", CL_Configstrings_f);
    Cmd_AddCommand("clientinfo", CL_Clientinfo_f);
    Cmd_AddCommand("vid_restart", CL_Vid_Restart_f);
    Cmd_AddCommand("snd_restart", CL_Snd_Restart_f);
    Cmd_AddCommand("disconnect", CL_Disconnect_f);
    Cmd_AddCommand("record", CL_Record_f);
    Cmd_AddCommand("stoprecord", CL_StopRecord_f);
    Cmd_AddCommand("demo", CL_PlayDemo_f);
    Cmd_AddCommand("timedemo", CL_PlayDemo_f);
    Cmd_SetAutoComplete("demo", "demos", "dm_1");
    Cmd_SetAutoComplete("timedemo", "demos", "dm_1");
    Cmd_AddCommand("cinematic", CL_PlayCinematic_f);
    Cmd_SetAutoComplete("cinematic", "video", "roq");
    Cmd_AddCommand("logo", CL_PlayLogo_f);
    Cmd_AddCommand("connect", CL_Connect_f);
    Cmd_AddCommand("reconnect", CL_Reconnect_f);
    Cmd_AddCommand("localservers", CL_LocalServers_f);
    Cmd_AddCommand("globalservers", CL_GlobalServers_f);
    CL_RconInit();
    Cmd_AddCommand("rcon", CL_Rcon_f);
    Cmd_AddCommand("ping", CL_Ping_f);
    Cmd_AddCommand("serverstatus", CL_ServerStatus_f);
    Cmd_AddCommand("toggleMenu", CL_ToggleMenu_f);
    Cmd_AddCommand("setenv", CL_Setenv_f);
    Cmd_AddCommand("showip", CL_ShowIP_f);
    Cmd_AddCommand("fs_openedList", CL_OpenedIWDList_f);
    Cmd_AddCommand("fs_referencedList", CL_ReferencedIWDList_f);
    Cmd_AddCommand("updatehunkusage", CL_UpdateLevelHunkUsage);
    Cmd_AddCommand("updatescreen", SCR_UpdateScreen);
    Cmd_AddCommand("startSingleplayer", CL_startSingleplayer_f);
    Cmd_AddCommand("setRecommended", CL_SetRecommended_f);
    Cmd_AddCommand("cubemapShot", CL_CubemapShot_f);
    Cmd_AddCommand("openScriptMenu", CL_OpenScriptMenu_f);
    Cmd_AddCommand("localizeSoundAliasFiles", Com_WriteLocalizedSoundAliasFiles);
    Cmd_AddCommand("openmenu", UI_OpenMenu_f);
    Cmd_AddCommand("closemenu", UI_CloseMenu_f);

    CL_InitRef();
    SCR_Init();
}

extern void Z_FreeInternal(void *ptr);
void CL_ShutdownDebugData(void)
{
    void (*shutdownDebug)(void);

    if (cls.debug.lines) {
        Z_FreeInternal(cls.debug.lines);
        cls.debug.lines = 0;
    }
    if (cls.debug.lineFromServer) {
        Z_FreeInternal(cls.debug.lineFromServer);
        cls.debug.lineFromServer = 0;
    }
    if (cls.debug.lineDuration) {
        Z_FreeInternal(cls.debug.lineDuration);
        cls.debug.lineDuration = 0;
    }
    if (cls.debug.strings) {
        Z_FreeInternal(cls.debug.strings);
        cls.debug.strings = 0;
    }
    if (cls.debug.stringFromServer) {
        Z_FreeInternal(cls.debug.stringFromServer);
        cls.debug.stringFromServer = 0;
    }

    memset(&cls.debug, 0, sizeof(cls.debug));

    shutdownDebug = re.ShutdownDebug;
    if (shutdownDebug)
        shutdownDebug();
}

void CL_GetPing(int n, char *buf, int buflen, int *pingtime)
{
    ping_t *ping = &cl_pinglist[n];
    int time;
    const char *adrStr;

    if (ping->adr.port == 0) {
        buf[0] = '\0';
        *pingtime = 0;
        return;
    }

    adrStr = NET_AdrToString(ping->adr);
    I_strncpyz(buf, adrStr, buflen);

    time = ping->time;
    if (time == 0) {

        int maxPing;
        time = Sys_Milliseconds() - ping->start;
        maxPing = Dvar_GetInt("cl_maxPing");
        if (maxPing <= 99)
            maxPing = 100;
        if (maxPing > time)
            time = 0;
    }

    CL_SetServerInfoByAddress(ping->adr, ping->info, ping->time);
    *pingtime = time;
}

void CL_ShowIP_f(void)
{
    Sys_ShowIP();
}

extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
const char *CL_GetServerIPAddress(void)
{
    byte *cc = (byte *)&clientConnections[0];

    if (*(int *)cc <= 4) {
        memset(szServerIPAddress, 0, 128);
    } else {

        unsigned short port = *(unsigned short *)(cc + 28);
        port = (port >> 8) | (port << 8);
        Com_sprintf(szServerIPAddress, 128, "%i.%i.%i.%i:%i",
                    cc[24], cc[25], cc[26], cc[27], (int)(short)port);
    }

    return szServerIPAddress;
}

void CL_FlushDebugData(qboolean fromServer)
{
    int i;

    if (!cls.rendererStarted)
        return;

    if (cls.debug.strings) {
        i = 0;
        while (i < cls.debug.numStrings) {
            if (cls.debug.stringFromServer[i] == (byte)fromServer) {
                --cls.debug.numStrings;
                cls.debug.stringFromServer[i] = cls.debug.stringFromServer[cls.debug.numStrings];
                memcpy(&cls.debug.strings[i], &cls.debug.strings[cls.debug.numStrings], sizeof(cls.debug.strings[i]));
            } else {
                ++i;
            }
        }

        re.LocateDebugStrings(cls.debug.strings, cls.debug.numStrings, cls.debug.maxStrings);
    }

    if (cls.debug.lines) {
        i = 0;
        while (i < cls.debug.numLines) {
            if (cls.debug.lineFromServer[i] == (byte)fromServer) {
                --cls.debug.lineDuration[i];
                if (cls.debug.lineDuration[i] > 0) {
                    ++i;
                } else {
                    --cls.debug.numLines;
                    cls.debug.lineFromServer[i] = cls.debug.lineFromServer[cls.debug.numLines];
                    cls.debug.lineDuration[i] = cls.debug.lineDuration[cls.debug.numLines];
                    memcpy(&cls.debug.lines[i], &cls.debug.lines[cls.debug.numLines], sizeof(cls.debug.lines[i]));
                }
            } else {
                ++i;
            }
        }

        re.LocateDebugLines(cls.debug.lines, cls.debug.numLines, cls.debug.maxLines);
    }
}

void CL_UpdateDebugData(void)
{
    if (!cls.rendererStarted)
        return;

    if (cls.debug.strings) {
        re.LocateDebugStrings(cls.debug.strings, cls.debug.numStrings, cls.debug.maxStrings);
    }

    if (cls.debug.lines) {
        re.LocateDebugLines(cls.debug.lines, cls.debug.numLines, cls.debug.maxLines);
    }
}

int CL_TextWidth(const char *text, int maxChars, FontHandle font)
{
    return re.TextWidth(text, maxChars, font);
}

int CL_TextHeight(FontHandle font)
{
    return re.TextHeight(font);
}

float CL_NormalizedTextScale(FontHandle font, float scale)
{
    return re.NormalizedTextScale(font, scale);
}

void CL_DrawTextPhysical(const char *text, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style)
{
    re.DrawText(text, maxChars, font, x, y, xScale, yScale, color, style);
}

extern void CalcSplitScreenTextOffset(FontHandle font, float *y);
extern void CalcScreenPlacement(float *x, float *y, float *xScale, float *yScale, int horzAlign, int vertAlign);
void CL_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float xScale, float yScale, const vec_t *color, int style)
{
    CalcSplitScreenTextOffset(font, &y);
    CalcScreenPlacement(&x, &y, &xScale, &yScale, horzAlign, vertAlign);
    re.DrawText(text, maxChars, font, x, y, xScale, yScale, color, style);
}

void CL_DrawTextPhysicalWithCursor(const char *text, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style, int cursorPos, int cursor)
{
    cursor = (signed char)cursor;
    re.DrawTextWithCursor(text, maxChars, font, x, y, xScale, yScale, color, style, cursorPos, cursor);
}

void CL_DrawTextWithCursor(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float xScale, float yScale, const vec_t *color, int style, int cursorPos, int cursor)
{
    cursor = (signed char)cursor;
    CalcSplitScreenTextOffset(font, &y);
    CalcScreenPlacement(&x, &y, &xScale, &yScale, horzAlign, vertAlign);
    re.DrawTextWithCursor(text, maxChars, font, x, y, xScale, yScale, color, style, cursorPos, cursor);
}

int CL_GetKeyCatchers(void)
{
    return clients[0].keyCatchers;
}

Bool CL_GetDisplayHUDWithKeycatchUI(void)
{
    return clients[0].displayHUDWithKeycatchUI;
}

FontHandle CL_RegisterFont(const char *fontName, int imageTrack)
{
    return re.RegisterFont(fontName, imageTrack);
}

void CL_WriteDemoMessage(msg_t *msg, int headerBytes)
{
    clientConnection_t *cc = &clientConnections[0];
    clientConnection_t *conn;
    int swlen;
    int len;

    swlen = cc->serverMessageSequence;
    FS_Write(&swlen, 4, cc->demofile);

    len = msg->cursize - headerBytes;
    swlen = len;
    conn = (clientConnection_t *)clc;
    FS_Write(&swlen, 4, conn->demofile);

    FS_Write(msg->data + headerBytes, len, conn->demofile);
}

void CL_Record_f(void)
{
    clientConnection_t *conn = (clientConnection_t *)clc;
    clientActive_t *active = (clientActive_t *)cl;
    char demoName[64];
    char name[256];
    msg_t buf;
    byte bufData[MAX_MSGLEN];
    byte compressedBuf[MAX_MSGLEN];
    entityState_t nullstate;
    int number;
    int i;
    int compressedSize;
    int len;

    if (Cmd_Argc() > 2) {
        Com_Printf((const char *)"record <demoname>\n");
        return;
    }

    if (conn->demorecording) {
        Com_Printf((const char *)"Already recording.\n");
        return;
    }

    if (conn->state != CA_ACTIVE) {
        Com_Printf((const char *)"You must be in a level to record.\n");
        return;
    }

    if (Cmd_Argc() == 2) {
        I_strncpyz(demoName, Cmd_Argv(1), sizeof(demoName));
        Com_sprintf(name, sizeof(name), (const char *)"demos/%s.dm_%d", demoName, 1);
    } else {
        number = 0;
        for (;;) {
            Com_sprintf(demoName, sizeof(demoName), (const char *)"demo%04i", number);
            Com_sprintf(name, sizeof(name), (const char *)"demos/%s.dm_%d", demoName, 1);
            if (!FS_FileExists(name))
                break;
            if (++number == 10000)
                break;
        }
    }

    Com_Printf((const char *)"recording to %s.\n", name);
    conn->demofile = FS_FOpenFileWrite(name);
    if (!conn->demofile) {
        Com_Printf((const char *)"ERROR: couldn't open.\n");
        return;
    }

    conn->demorecording = 1;
    I_strncpyz(conn->demoName, demoName, sizeof(conn->demoName));
    conn->demowaiting = 1;

    MSG_Init(&buf, bufData, sizeof(bufData));
    MSG_WriteLong(&buf, conn->reliableSequence);
    MSG_WriteByte(&buf, 1);
    MSG_WriteLong(&buf, conn->serverCommandSequence);

    for (i = 0; i < 2048; ++i) {
        int offset = active->gameState.stringOffsets[i];
        if (!offset)
            continue;

        MSG_WriteByte(&buf, 2);
        MSG_WriteShort(&buf, i);
        MSG_WriteBigString(&buf, active->gameState.stringData + offset);
    }

    memset(&nullstate, 0, sizeof(nullstate));
    for (i = 0; i < 1024; ++i) {
        if (!active->entityBaselines[i].number)
            continue;

        MSG_WriteByte(&buf, 3);
        MSG_WriteDeltaEntity(&buf, &nullstate, &active->entityBaselines[i], 1);
    }

    MSG_WriteByte(&buf, 7);
    MSG_WriteLong(&buf, conn->clientNum);
    MSG_WriteLong(&buf, conn->checksumFeed);
    MSG_WriteByte(&buf, 7);

    *(int *)compressedBuf = *(int *)buf.data;
    compressedSize = MSG_WriteBitsCompress(buf.data + 4, compressedBuf + 4, buf.cursize - 4) + 4;

    len = conn->serverMessageSequence;
    FS_Write(&len, 4, conn->demofile);
    len = compressedSize;
    FS_Write(&len, 4, conn->demofile);
    FS_Write(compressedBuf, compressedSize, conn->demofile);
}

void CL_ShutdownHunkUsers(void)
{
    if (!cls.hunkUsersStarted)
        return;
    CL_ShutdownCGame();
    CL_ShutdownUI();
    cls.whiteMaterial = 0;
    cls.consoleMaterial = 0;
    cls.consoleFont = 0;
    cls.hunkUsersStarted = 0;
}

void CL_InitLoad(const char *mapname, const char *gametype)
{
    byte *cc;

    if (!(*(LegacyHacks **)imp_legacyHacks)->cl_running)
        return;

    Dvar_SetInt(*(const dvar_t **)imp_com_expectedHunkUsage, 0);
    UI_SetMap(mapname, gametype);

    cc = (byte *)clc;
    *(int *)cc = (*(int *)cc >= 5) ? 5 : 0;

    SCR_UpdateScreenInternal();
}

void CL_DrawLogo(void)
{
    int time;
    float alpha;
    float color[4];
    float w;
    float h0;
    float h1;

    time = cls.realtime - cls.logo.startTime;

    if (time < cls.logo.fadein) {
        alpha = (float)time / (float)cls.logo.fadein;
    } else if (time > cls.logo.duration - cls.logo.fadeout) {
        alpha = (float)(cls.logo.duration - time) / (float)cls.logo.fadeout;
    } else {
        alpha = 1.0f;
    }

    if (alpha < 0.0f)
        alpha = 0.0f;
    if (alpha > 1.0f)
        alpha = 1.0f;

    color[0] = alpha;
    color[1] = alpha;
    color[2] = alpha;
    color[3] = 1.0f;

    w = (float)cls.vidConfig.width;
    h0 = (float)(cls.vidConfig.height * 2) / 3.0f;
    h1 = (float)cls.vidConfig.height - h0;

    re.DrawStretchPic(0.0f, 0.0f, w, h0, 0.0f, 0.0f, 1.0f, 1.0f, color, cls.logo.material[0]);
    re.DrawStretchPic(0.0f, h0, w, h1, 0.0f, 0.0f, 1.0f, 1.0f, color, cls.logo.material[1]);

    if (time > cls.logo.duration)
        ((clientConnection_t *)clc)->state = CA_DISCONNECTED;
}

void CL_StopLogoOrCinematic(void)
{
    if (*(int *)&clientConnections[0] == 1) {
        SCR_StopCinematic();
    } else {
        *(int *)&clientConnections[0] = 0;
    }

    SND_StopSounds(0);

    if (!*(int *)clc)
        UI_SetActiveMenu(1);
    else
        UI_SetActiveMenu(0);
}

void CL_AddDebugLine(const vec_t *start, const vec_t *end, const vec_t *color, qboolean depthTest, int duration, qboolean fromServer)
{
    int lineIndex;
    trDebugLine_t *line;

    if (!cls.rendererStarted)
        return;

    cls.debug.maxLines = 0x1000;
    lineIndex = cls.debug.numLines;

    if (lineIndex + 1 > cls.debug.maxLines)
        return;

    if (!cls.debug.lines) {
        cls.debug.lines = Z_MallocInternal(0x2c000);
        cls.debug.lineFromServer = Z_MallocInternal(cls.debug.maxLines);
        cls.debug.lineDuration = Z_MallocInternal(cls.debug.maxLines * sizeof(*cls.debug.lineDuration));
        cls.debug.numLines = 0;
        lineIndex = 0;
    }

    line = &cls.debug.lines[lineIndex];
    line->start[0] = start[0];
    line->start[1] = start[1];
    line->start[2] = start[2];
    line->end[0] = end[0];
    line->end[1] = end[1];
    line->end[2] = end[2];
    line->color[0] = color[0];
    line->color[1] = color[1];
    line->color[2] = color[2];
    line->color[3] = color[3];
    line->depthTest = depthTest;

    cls.debug.lineFromServer[lineIndex] = (byte)fromServer;
    cls.debug.lineDuration[lineIndex] = duration;
    cls.debug.numLines++;
}

void CL_AddDebugString(const vec_t *xyz, const vec_t *color, float scale, const char *text, qboolean fromServer)
{
    int stringIndex;
    trDebugString_t *string;

    if (!cls.rendererStarted)
        return;

    cls.debug.maxStrings = 0x100;
    stringIndex = cls.debug.numStrings;

    if (stringIndex + 1 > cls.debug.maxStrings)
        return;

    if (!cls.debug.strings) {
        cls.debug.strings = Z_MallocInternal(0x8000);
        cls.debug.stringFromServer = Z_MallocInternal(cls.debug.maxStrings);
        cls.debug.numStrings = 0;
        stringIndex = 0;
    }

    string = &cls.debug.strings[stringIndex];
    string->xyz[0] = xyz[0];
    string->xyz[1] = xyz[1];
    string->xyz[2] = xyz[2];
    string->color[0] = color[0];
    string->color[1] = color[1];
    string->color[2] = color[2];
    string->color[3] = color[3];
    string->scale = scale;
    strncpy(string->text, text, 0x5f);
    string->text[95] = '\0';

    cls.debug.stringFromServer[stringIndex] = (byte)fromServer;
    cls.debug.numStrings++;
}

#if COD2_IS_PATCH_13

int lastUpdateKeyAuthTime;

static const char cl_getKeyAuthorizeFmt_13[] = "getKeyAuthorize %i %s PB %s";

static void CL_BuildMd5StrFromCDKey(char *md5Str)
{
    char nums[64];
    const char *key;
    int len, i, j;
    char *p, *e;

    md5Str[0] = '\0';
    memset(nums, 0, sizeof(nums));

    key = (const char *)imp_cl_cdkey;
    len = strlen(key);
    if (len > 0) {
        j = 0;
        for (i = 0; i < 32; ++i) {
            unsigned char ch = (unsigned char)key[i];
            if ((ch >= '0' && ch <= '9') ||
                (ch >= 'a' && ch <= 'z') ||
                (ch >= 'A' && ch <= 'Z')) {
                nums[j++] = (char)ch;
            }
        }
        nums[j] = '\0';
    }

    p = nums;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    e = p + strlen(p);
    while (e > p && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\n' || e[-1] == '\r'))
        e--;
    *e = '\0';
    if (p != nums)
        memmove(nums, p, (size_t)(e - p) + 1);
    strupr(nums);

#    if COD2_FEATURE_PUNKBUSTER

    md5Str[0] = '\0';
#    endif
}
#endif

void CL_RequestAuthorization(void)
{
    char nums[64];
    const char *cdkey;
    int len;
    int i;
    int j;
    const dvar_t *anonymous;
#if COD2_IS_PATCH_13
    char md5Str[33];

    lastUpdateKeyAuthTime = cls.realtime;
#endif

    if (!CL_CDKeyValidate((const char *)imp_cl_cdkey, (const char *)imp_cl_cdkeychecksum)) {
        Com_Error(1, (const char *)"EXE_ERR_INVALID_CD_KEY");
        return;
    }

    if (!cls.authorizeServer.port) {
        Com_Printf((const char *)"Resolving %s\n", (const char *)"cod2master.activision.com");
        if (!NET_StringToAdr((const char *)"cod2master.activision.com", &cls.authorizeServer)) {
            Com_Printf((const char *)"Couldn't resolve address\n");
            return;
        }

        cls.authorizeServer.port = 0xdc50;
        Com_Printf((const char *)"%s resolved to %i.%i.%i.%i:%i\n",
                   (const char *)"cod2master.activision.com",
                   cls.authorizeServer.ip[0],
                   cls.authorizeServer.ip[1],
                   cls.authorizeServer.ip[2],
                   cls.authorizeServer.ip[3],
                   0x50dc);
    }

    if (cls.authorizeServer.type == NA_BAD)
        return;

    if (Dvar_GetBool((const char *)"fs_restrict")) {
        I_strncpyz(nums, (const char *)"demo", sizeof(nums));
    } else {
        cdkey = (const char *)imp_cl_cdkey;
        len = strlen(cdkey);
        if (len > 32)
            len = 32;

        j = 0;
        for (i = 0; i < len; ++i) {
            unsigned char ch = (unsigned char)cdkey[i];
            if ((ch >= '0' && ch <= '9') ||
                (ch >= 'a' && ch <= 'z') ||
                (ch >= 'A' && ch <= 'Z')) {
                nums[j++] = (char)ch;
            }
        }
        nums[j] = '\0';
    }

    anonymous = Dvar_RegisterBool_mac((const char *)"cl_anonymous", 0, 0x101b);
#if COD2_IS_PATCH_13
    CL_BuildMd5StrFromCDKey(md5Str);
    NET_OutOfBandPrint(NS_CLIENT1, cls.authorizeServer,
                       va(cl_getKeyAuthorizeFmt_13, anonymous->current.enabled, nums, md5Str));
#else
    NET_OutOfBandPrint(NS_CLIENT1, cls.authorizeServer,
                       va((const char *)"getKeyAuthorize %i %s", anonymous->current.enabled, nums));
#endif
}

void CL_CheckForResend(void)
{
    clientConnection_t *conn = (clientConnection_t *)clc;
    char info[1024];
    char data[2048];
    int infoLen;

    if (conn->demoplaying)
        return;

    if (conn->state != CA_CONNECTING && conn->state != CA_CHALLENGING)
        return;

    if (cls.realtime - conn->connectTime <= 2999)
        return;

    conn->connectTime = cls.realtime;
    conn->connectPacketCount++;
    Com_Printf("[cnx] CL_CheckForResend state=%d count=%d realtime=%d\n", conn->state, conn->connectPacketCount, cls.realtime);

    if (conn->state == CA_CONNECTING) {
        const dvar_t *lanAuthorize = *(const dvar_t **)imp_net_lanauthorize;

        if (lanAuthorize->current.enabled || !Sys_IsLANAddress(conn->serverAddress))
            CL_RequestAuthorization();

        NET_OutOfBandPrint(NS_CLIENT1, conn->serverAddress, (const char *)"getchallenge");
        return;
    }

    if (conn->state != CA_CHALLENGING) {
        Com_Error(0, (const char *)"\x15"
                                   "CL_CheckForResend: bad clc->state");
        return;
    }

    I_strncpyz(info, Dvar_InfoString(2), sizeof(info));
    Info_SetValueForKey(info, (const char *)"protocol", va((const char *)"%i", 0x76));
    Info_SetValueForKey(info, (const char *)"challenge", va((const char *)"%i", conn->challenge));
    Info_SetValueForKey(info, (const char *)"qport", va((const char *)"%i", conn->qport));

    infoLen = strlen(info);
    data[0] = 'c';
    data[1] = 'o';
    data[2] = 'n';
    data[3] = 'n';
    data[4] = 'e';
    data[5] = 'c';
    data[6] = 't';
    data[7] = ' ';
    data[8] = '"';
    memcpy(data + 9, info, infoLen);
    data[9 + infoLen] = '"';
    data[10 + infoLen] = '\0';

    NET_OutOfBandData(NS_CLIENT1, conn->serverAddress, data, infoLen + 10);
    *(int *)imp_dvar_modifiedFlags &= ~2;
}

void CL_LocalServers_f(void)
{
    int i;
    int pass;
    int port;
    netadr_t to;

    Com_Printf((const char *)"Scanning for servers on the local network...\n");

    cls.numlocalservers = 0;
    cls.pingUpdateSource = 0;

    for (i = 0; i < 128; ++i) {
        byte dirty = cls.localServers[i].dirty;
        Com_Memset(&cls.localServers[i], 0, sizeof(cls.localServers[i]));
        cls.localServers[i].dirty = dirty;
    }

    Com_Memset(&to, 0, sizeof(to));

#ifdef __EMSCRIPTEN__
    /* Browsers cannot send LAN broadcasts. Discover the host's live dedicated
     * servers over HTTP without reserving a gameplay WebSocket slot. */
    extern void Web_DiscoverServers(void);
    Web_DiscoverServers();
#else
    for (pass = 0; pass < 2; ++pass) {
        for (port = 0x7120; port < 0x7124; ++port) {
            to.port = (unsigned short)(((port & 0xff) << 8) | ((port >> 8) & 0xff));
            to.type = NA_BROADCAST;
            CL_Netchan_SendOOBPacket(0xf, (const void *)"\xff"
                                                        "fd\xff"
                                                        "fd\xff"
                                                        "fd\xff"
                                                        "fdgetinfo xxx",
                                     to);
        }
    }
#endif
}

void CL_PlayLogo_f(void)
{
    clientConnection_t *conn;
    const char *name;

    if (Cmd_Argc() != 5) {
        Com_Printf((const char *)"USAGE: logo <image name> <fadein seconds> <full duration seconds> <fadeout seconds>\n");
        return;
    }

    Com_DPrintf((const char *)"CL_PlayLogo_f\n");

    conn = (clientConnection_t *)clc;
    if (conn->state == CA_CINEMATIC) {
        SCR_StopCinematic();
    } else if (conn->state == CA_LOGO) {
        conn->state = CA_DISCONNECTED;
    } else if (conn->state != CA_DISCONNECTED) {
        return;
    }

    conn->state = CA_LOGO;

    if (cls.uiStarted)
        UI_SetActiveMenu(0);

    SND_StopSounds(0);
    SND_FadeAllSounds(1.0f, 0);

    name = Cmd_Argv(1);
    cls.logo.fadein = (int)floorf((float)atof(Cmd_Argv(2)) * 1000.0f + 0.5f);
    cls.logo.duration = (int)floorf((float)atof(Cmd_Argv(3)) * 1000.0f + 0.5f);
    cls.logo.fadeout = (int)floorf((float)atof(Cmd_Argv(4)) * 1000.0f + 0.5f);
    cls.logo.duration += cls.logo.fadein + cls.logo.fadeout;

    cls.logo.material[0] = re.RegisterMaterial(va((const char *)"%s1", name), 0x30, 3);
    cls.logo.material[1] = re.RegisterMaterial(va((const char *)"%s2", name), 0x30, 3);
    cls.logo.startTime = cls.realtime + 100;
}

qboolean CL_UpdateDirtyPings_f(int source)
{
    serverInfo_t *servers;
    int max;
    int slots;
    int status;
    int i;
    char buff[1024];

    if (((clientConnection_t *)clc)->state != CA_DISCONNECTED)
        return 0;

    if ((unsigned int)source > 2)
        return 0;

    cls.pingUpdateSource = source;

    slots = 0;
    for (i = 0; i < 16; ++i) {
        if (cl_pinglist[i].adr.port)
            ++slots;
    }

    status = 0;

    if (slots <= 15) {
        if (source == 1) {
            max = cls.numglobalservers;
            servers = cls.globalServers;
        } else if (source == 2) {
            max = cls.numfavoriteservers;
            servers = cls.favoriteServers;
        } else {
            max = cls.numlocalservers;
            servers = cls.localServers;
        }

        if (max > 0) {
            for (i = 0; i < max; ++i) {
                int pingSlot;
                int duplicate;
                int j;

                if (!servers[i].dirty)
                    continue;
                if (servers[i].ping != -1)
                    continue;

                if (slots > 15) {
                    status = 1;
                    break;
                }

                duplicate = 0;
                for (j = 0; j < 16; ++j) {
                    if (!cl_pinglist[j].adr.port)
                        continue;
                    if (NET_CompareAdr(cl_pinglist[j].adr, servers[i].adr)) {
                        duplicate = 1;
                        break;
                    }
                }

                if (duplicate)
                    continue;

                pingSlot = 0;
                while (pingSlot < 16 && cl_pinglist[pingSlot].adr.port)
                    ++pingSlot;

                cl_pinglist[pingSlot].adr = servers[i].adr;
                cl_pinglist[pingSlot].start = Sys_Milliseconds();
                cl_pinglist[pingSlot].time = 0;
                NET_OutOfBandPrint(NS_CLIENT1, cl_pinglist[pingSlot].adr, (const char *)"getinfo xxx");
                ++slots;
                status = 1;
            }
        }
    }

    if (slots)
        status = 1;

    for (i = 0; i < 16; ++i) {
        int pingTime;

        if (!cl_pinglist[i].adr.port)
            continue;

        CL_GetPing(i, buff, sizeof(buff), &pingTime);
        if (!pingTime)
            continue;

        cl_pinglist[i].adr.port = 0;
        status = 1;
    }

    return status;
}

void CL_RunOncePerClientFrame(int msec)
{

    if (UI_IsFullscreen()) {
        re.SyncGpu();
    }

    int avidemoVal = cl_avidemo->current.integer;
    if (avidemoVal && msec) {

        if (clc->state == 8 || cl_forceavidemo->current.enabled) {
            Cbuf_ExecuteText(0, "screenshot silent\n");
        }
        float timescale = *(float *)imp_com_timescaleValue;
        msec = (int)(1000.0f / (float)avidemoVal * timescale);
        if (msec == 0)
            msec = 1;
    }

    if (cls.uiStarted)
        CG_CalculateFPS();

    cls.realFrametime = msec;
    cls.frametime = msec;
    cls.realtime += msec;

    int curFrameTime = *(int *)imp_com_frameTime;
    int delta = curFrameTime - old_com_frameTime;
    frame_msec = (delta != 0) ? delta : 1;
    if (frame_msec > 200)
        frame_msec = 200;
    old_com_frameTime = curFrameTime;
}

int Client_SendVoiceData(int bytes, char *enc_buffer)
{
    clientActive_t *c;
    int voiceIdx, newIdx;

    if (bytes > 0) {
        c = *(clientActive_t **)imp_cl;
        voiceIdx = c->voicePacketCount;

        memcpy(c->voicePackets[voiceIdx].data, enc_buffer, bytes);
        c->voicePackets[voiceIdx].dataSize = bytes;

        newIdx = voiceIdx + 1;
        c->voicePacketCount = newIdx;

        if (newIdx > 0) {
            if (c->serverTime - c->voicePacketLastTransmit > 199 || newIdx > 9) {
                CL_WriteVoicePacket();
                c = *(clientActive_t **)imp_cl;
                c->voicePacketCount = 0;
                c->voicePacketLastTransmit = c->serverTime;
            }
        }
    }

    return bytes;
}

Bool CL_ConnectionlessPacket(netadr_t from, msg_t *msg, int time)
{
    clientConnection_t *conn = (clientConnection_t *)clc;
    LargeLocal printBuf_large_local;
    char *printBuf;
    const char *cmd;
    Bool result = 0;

    LargeLocal_LargeLocal(&printBuf_large_local, MAX_MSGLEN);
    printBuf = (char *)LargeLocal_GetBuf(&printBuf_large_local);

    MSG_BeginReading(msg);
    MSG_ReadLong(msg);
    CL_Netchan_AddOOBProfilePacket(msg->cursize);

    Cmd_TokenizeString(MSG_ReadStringLine(msg));
    cmd = Cmd_Argv(0);
    Com_Printf("[cnx] CL_ConnectionlessPacket cmd='%s' state=%d\n", cmd, conn->state);

#if COD2_IS_PATCH_13

    if (I_strnicmp(cmd, "PB_", 3) == 0) {
        PB_HandleClientOobPacket(&from, msg);
        result = 1;
        goto finish;
    }
#endif

    if (I_stricmp(cmd, (const char *)"v") == 0) {
        CL_VoicePacket(msg);
        result = 1;
        goto finish;
    }

    if (I_stricmp(cmd, (const char *)"challengeResponse") == 0) {
        if (conn->state != CA_CONNECTING) {
            Com_Printf((const char *)"Unwanted challenge response received.  Ignored.\n");
            goto finish;
        }

        conn->challenge = atoi(Cmd_Argv(1));
        conn->state = CA_CHALLENGING;
        conn->connectPacketCount = 0;
        conn->connectTime = -99999;
        conn->serverAddress = from;
        Com_DPrintf((const char *)"challenge: %d\n", conn->challenge);
        result = 1;
        goto finish;
    }

    if (I_stricmp(cmd, (const char *)"connectResponse") == 0) {
        if (conn->state > CA_CHALLENGING) {
            Com_Printf((const char *)"Dup connect received.  Ignored.\n");
            goto finish;
        }

        if (conn->state != CA_CHALLENGING) {
            Com_Printf((const char *)"connectResponse packet while not connecting.  Ignored.\n");
            goto finish;
        }

        if (!NET_CompareBaseAdr(from, conn->serverAddress)) {
            const char *expected;
            const char *actual;

            Com_Printf((const char *)"connectResponse from a different address.  Ignored.\n");
            expected = NET_AdrToString(conn->serverAddress);
            actual = NET_AdrToString(from);
            Com_Printf((const char *)"%s should have been %s\n", actual, expected);
            goto finish;
        }

        Netchan_Setup(NS_CLIENT1, &conn->netchan, from, *(int *)imp_g_qport);
        conn->state = CA_CONNECTED;
        conn->lastPacketSentTime = cls.realtime;
        conn->lastPacketTime = -9999;
        result = 1;
        goto finish;
    }

    if (I_stricmp(cmd, (const char *)"infoResponse") == 0) {
        CL_ServerInfoPacket(from, msg, time);
        result = 1;
        goto finish;
    }

    if (I_stricmp(cmd, (const char *)"statusResponse") == 0) {
        CL_ServerStatusResponse(from, msg);
        result = 1;
        goto finish;
    }

    if (I_stricmp(cmd, (const char *)"disconnect") == 0) {
        if (conn->state && NET_CompareAdr(from, conn->netchan.remoteAddress) &&
            cls.realtime - conn->lastPacketTime > 2999) {
            Com_Error(1, (const char *)"EXE_SERVER_DISCONNECTED");
        }

        result = 1;
        goto finish;
    }

    if (I_stricmp(cmd, (const char *)"echo") == 0) {
        NET_OutOfBandPrint(NS_CLIENT1, from, va((const char *)"%s", Cmd_Argv(1)));
        result = 1;
        goto finish;
    }

    if (I_stricmp(cmd, (const char *)"keyAuthorize") == 0) {
        result = 1;
        goto finish;
    }

    if (I_stricmp(cmd, (const char *)"print") == 0) {
        const char *text = MSG_ReadBigString(msg);

        I_strncpyz(conn->serverMessage, text, sizeof(conn->serverMessage));
        Com_sprintf(printBuf, 0x4000, (const char *)"%s", text);
        Com_PrintMessage(0, printBuf);
        result = 1;
        goto finish;
    }

    if (I_stricmp(cmd, (const char *)"error") == 0) {
        if (conn->state && NET_CompareBaseAdr(from, conn->serverAddress)) {
            Com_Error(1, (const char *)"%s", MSG_ReadBigString(msg));
            result = 1;
        }
        goto finish;
    }

    if (I_strncmp(cmd, (const char *)"getserversResponse", 18) == 0) {
        CL_ServersResponsePacket(from, msg);
        result = 1;
        goto finish;
    }

    if (I_strncmp(cmd, (const char *)"needcdkey", 9) == 0) {
        I_strncpyz(conn->serverMessage, (const char *)"EXE_AWAITINGCDKEYAUTH", sizeof(conn->serverMessage));
        SEH_LocalizeTextMessage((const char *)"EXE_AWAITINGCDKEYAUTH", (const char *)"need cd key message", 0);
        Com_Printf((const char *)"%s\n", conn->serverMessage);
        CL_RequestAuthorization();
        result = 1;
        goto finish;
    }

    if (I_stricmp(cmd, (const char *)"loadingnewmap") == 0) {
        if (NET_CompareBaseAdr(from, conn->serverAddress)) {
            char mapname[64];
            const char *gametype;

            I_strncpyz(mapname, MSG_ReadStringLine(msg), sizeof(mapname));
            gametype = MSG_ReadStringLine(msg);
            CL_SetupForNewServerMap(mapname, gametype);
            /* The loading screen no longer runs CG_DrawActiveFrame/CL_Input.
             * Leave ACTIVE so CL_SendCmd keeps sending acknowledgements and
             * the old serverId that requests the new gamestate. */
            if (conn->state >= CA_CONNECTED)
                conn->state = CA_CONNECTED;
            UI_DrawConnectScreen();
        }

        result = 1;
        goto finish;
    }

finish:
    ZN10LargeLocalD1Ev(&printBuf_large_local);
    return result;
}

static inline __attribute__((always_inline))
Bool
CL_PacketEvent_real(netadr_t from, msg_t *msg, int time)
{
    clientConnection_t *conn = (clientConnection_t *)clc;
    int headerBytes;
    int savedServerMessageSequence;
    int savedReliableAcknowledge;

    if (msg->cursize > 3 && *(int *)msg->data == -1)
        return CL_ConnectionlessPacket(from, msg, time);

    if (*(int *)&clientConnections[0] <= CA_CHALLENGING)
        return 0;

    if (msg->cursize <= 3) {
        Com_Printf((const char *)"%s: Runt packet\n", NET_AdrToString(from));
        return 1;
    }

    if (!NET_CompareAdr(from, conn->netchan.remoteAddress)) {
        Com_DPrintf((const char *)"%s:sequenced packet without connection\n", NET_AdrToString(from));
        return 0;
    }

    conn->lastPacketTime = cls.realtime;

    if (!Netchan_Process(&conn->netchan, msg))
        return 0;

    headerBytes = msg->readcount;
    savedServerMessageSequence = conn->serverMessageSequence;
    savedReliableAcknowledge = conn->reliableAcknowledge;

    conn->serverMessageSequence = *(int *)msg->data;
    conn->reliableAcknowledge = MSG_ReadLong(msg);
    if (conn->reliableAcknowledge < conn->reliableSequence - 128) {
        conn->reliableAcknowledge = conn->reliableSequence;
        return 0;
    }

    CL_Netchan_Decode(msg->data + msg->readcount, msg->cursize - msg->readcount);
    CL_ParseServerMessage(msg);

    if (msg->overflowed) {
        Com_DPrintf((const char *)"ignoring illegible message");
        conn->serverMessageSequence = savedServerMessageSequence;
        conn->reliableAcknowledge = savedReliableAcknowledge;
        return 0;
    }

    if (conn->demorecording && !conn->demowaiting)
        CL_WriteDemoMessage(msg, headerBytes);

    return 1;
}

Bool CL_PacketEvent(netadr_t from, msg_t *msg, int time)
{
    return CL_PacketEvent_real(from, msg, time);
}

extern void Com_ClientDObjClearAllSkel(void);
extern void CL_ClearMutedList(void);
extern void Cbuf_Execute(void);
extern void CL_SetADS(int ads);
extern void *imp_legacyHacks;
void CL_Init(void)
{
    Com_Printf("----- Client Initialization -----\n");

    clientActive_t *c = (clientActive_t *)cl;
    Bool savedCgameInitialized = c->cgameInitialized;
    Bool savedCgameInitCalled = c->cgameInitCalled;
    int savedKeyCatchers = c->keyCatchers;
    Bool savedDisplayHUDWithKeycatchUI = c->displayHUDWithKeycatchUI;
    Bool savedActive = c->active;

    memset(c, 0, sizeof(clientActive_t));

    c->cgameInitialized = savedCgameInitialized;
    c->cgameInitCalled = savedCgameInitCalled;
    c->keyCatchers = savedKeyCatchers;
    c->displayHUDWithKeycatchUI = savedDisplayHUDWithKeycatchUI;
    c->active = savedActive;

    Com_ClientDObjClearAllSkel();
    CL_ClearMutedList();
    c->voicePacketCount = 0;

    *(int *)(byte *)clc = 0;
    cls.realtime = 0;

    c->active = 1;

    byte **lh = (byte **)imp_legacyHacks;
    (*lh)[0x5c] = 0;
    (*lh)[0x9c] = 0;
    (*lh)[0xdc] = 0;

    Cbuf_Execute();

    *(int *)(*lh + 4) = 1;
    CL_SetADS(0);

    Com_Printf("----- Client Initialization Complete -----\n");
}

void CL_ShutdownAll(void)
{
    void (*syncRenderThread)(void) = re.SyncRenderThread;

    if (syncRenderThread)
        syncRenderThread();

    if (cls.hunkUsersStarted) {
        CL_ShutdownCGame();
        CL_ShutdownUI();
        cls.whiteMaterial = 0;
        cls.consoleMaterial = 0;
        cls.consoleFont = 0;
        cls.hunkUsersStarted = 0;
    }

    if (re.Shutdown)
        ((void (*)(int))re.Shutdown)(0);

    cls.rendererStarted = 0;
}

extern void *imp_sv_paused;

void CL_Frame(int msec)
{

    clientConnection_t *clc_p = &clientConnections[0];
    clientActive_t *cl_p;
    const dvar_t *clp;
    int state;
    int t;
    const char *info;

    if ((*(LegacyHacks **)imp_legacyHacks)->cl_running == 0)
        return;

    Voice_GetLocalVoiceData((void *)&clients[0]);
    Voice_Playback();
    CL_UpdateColor();

    cl_p = *(clientActive_t **)imp_cl;

    if (clc_p->state <= 3)
        goto Lc6;

    clp = *(const dvar_t **)imp_cl_paused;
    if (!clp->current.enabled)
        goto L137;
    if (!(*(const dvar_t **)imp_sv_paused)->current.enabled)
        goto Lc6;
    if ((unsigned)(clc_p->state - 3) <= 1)
        goto L183;
    goto Lmain0;

Lc6:
    state = clc_p->state;
    if (state <= 4) {
        if ((unsigned)(state - 3) <= 1)
            goto L183;
        goto Lmain0;
    }
    t = cls.realtime - clc_p->lastPacketTime;
    if ((float)t <= cl_timeout->current.value * 1000.0f)
        goto Lmain0;
    cl_p->timeoutcount += 1;
    if (cl_p->timeoutcount > 5)
        Com_Error(1, "EXE_ERR_SERVER_TIMEOUT");
    goto Lmain1;

L137:
    if (*(int *)imp_dvar_modifiedFlags & 2)
        goto Luserinfo;
    goto Lc6;

Lmain0:
    cl_p->timeoutcount = 0;
Lmain1:
#if COD2_IS_PATCH_13

    if (DL_InProgress())
        CL_WWWDownload();
#endif
    CL_CheckForResend();
    if (clc_p->state == 8)
        goto L14a;
    if (cl_ingame->current.enabled)
        goto L16e;
    goto Ltail;

L14a:
    if (!cl_ingame->current.enabled)
        Dvar_SetBool(cl_ingame, 1);
    goto Ltail;

L16e:
    Dvar_SetBool(cl_ingame, 0);
    goto Ltail;

L183:
    if (clc_p->connectTime <= 0)
        goto Lmain0;
    t = cls.realtime - clc_p->connectTime;
    if ((float)t <= cl_connectTimeout->current.value * 1000.0f)
        goto Lmain0;
    Com_Error(1, "EXE_ERR_SERVER_TIMEOUT");
    goto Lmain1;

Luserinfo:
    *(int *)imp_dvar_modifiedFlags &= ~2;
    info = Dvar_InfoString(2);
    info = va("userinfo \"%s\"", info);
    if (clc_p->reliableSequence - clc_p->reliableAcknowledge - 0x80 > 0)
        Com_Error(1, "EXE_ERR_CLIENT_CMD_OVERFLOW");
    clc_p->reliableSequence += 1;
    MSG_WriteReliableCommandToBuffer(info,
                                     (char *)clc_p->reliableCommands[clc_p->reliableSequence & 0x7f], 0x400);
    goto Lc6;

Ltail:
    CL_SetCGameTime();
    CL_SendCmd();
}

void CL_Vid_Restart_f(void)
{
    const dvar_t *svRunning = *(const dvar_t **)imp_com_sv_running;
    clientConnection_t *conn = (clientConnection_t *)clc;
    clientActive_t *active = (clientActive_t *)cl;
    MemoryFile memFile;
    void *tempBuf;
    void *clientStateBuf = NULL;
    int clientStateBytes = 0;

    if (svRunning->current.enabled) {
        Com_Printf((const char *)"Listen server cannot video restart.\n");
        return;
    }

    if (active->cgameInitialized) {
        tempBuf = Z_VirtualAllocInternal(0xa00000);
        MemFile_InitForWriting(&memFile, 0xa00000, tempBuf, 1);
        CL_ArchiveClientState(&memFile);

        clientStateBytes = memFile.bytesUsed;
        clientStateBuf = Z_MallocInternal(clientStateBytes);
        memcpy(clientStateBuf, memFile.buffer, clientStateBytes);
        Z_VirtualFreeInternal(memFile.buffer);
    }

    Dvar_SetInt(*(const dvar_t **)imp_com_expectedHunkUsage, 0);
    g_waitingForServer = 0;
    SND_StopSounds(1);

    CL_ShutdownHunkUsers();
    CL_ShutdownRef();
    cls.rendererStarted = 0;

    CL_AddReliableCommand(va((const char *)"vdr"));
    Com_Restart();

    Dvar_RegisterInt((const char *)"loc_language", 0, 0, 13, 0x1021);
    Dvar_RegisterBool_mac((const char *)"loc_translate", 1, 0x1020);
    Dvar_RegisterBool_mac((const char *)"fs_ignoreLocalized", 0, 0x10a0);

    FS_ConditionalRestart(conn->checksumFeed);
    SEH_UpdateLanguageInfo();
    Dvar_SetInt(*(const dvar_t **)imp_cl_paused, 0);
    CL_InitRef();
    CL_StartHunkUsers();

    if (conn->state >= CA_LOADING && conn->state <= CA_ACTIVE) {
        CL_InitCGame();
        CL_SendPureChecksums();
    }

    if (clientStateBuf) {
        active = (clientActive_t *)cl;
        if (active->cgameInitialized) {
            MemFile_InitForReading(&memFile, clientStateBytes, clientStateBuf);
            CL_ArchiveClientState(&memFile);
        }
        Z_FreeInternal(clientStateBuf);
    }
}

void CL_Snd_Restart_f(void)
{
    const dvar_t *svRunning = *(const dvar_t **)imp_com_sv_running;
    MemoryFile memFile;
    snd_listener listeners;
    void *tempBuf;
    void *savedBuf;
    int bytesUsed;

    if (svRunning->current.enabled) {
        Com_Printf((const char *)"Listen server cannot sound restart.\n");
        return;
    }

    tempBuf = Z_VirtualAllocInternal(0xa00000);
    MemFile_InitForWriting(&memFile, 0xa00000, tempBuf, 1);
    SND_Save(&memFile);

    bytesUsed = memFile.bytesUsed;
    savedBuf = Z_MallocInternal(bytesUsed);
    memcpy(savedBuf, tempBuf, bytesUsed);
    Z_VirtualFreeInternal(tempBuf);

    SND_SaveListeners(&listeners);
    SND_Shutdown();
    SND_Init();
    SND_RestoreListeners(&listeners);

    CL_Vid_Restart_f();

    MemFile_InitForReading(&memFile, bytesUsed, savedBuf);
    SND_Restore(&memFile);
    Z_FreeInternal(savedBuf);
}

void CL_Disconnect(void)
{
    LegacyHacks *legacy = *(LegacyHacks **)imp_legacyHacks;
    clientConnection_t *conn = (clientConnection_t *)clc;
    clientActive_t *active = (clientActive_t *)cl;
    connstate_t oldState;
    int sequence;

    if (!legacy->cl_running)
        return;

    if (conn->demorecording)
        CL_StopRecord_f();

    if (conn->download) {
        FS_FCloseFile(conn->download);
        conn->download = 0;
    }

    conn->downloadName[0] = '\0';
    conn->downloadTempName[0] = '\0';
    legacy->cl_downloadName[0] = 0;

    if (conn->demofile) {
        FS_FCloseFile(conn->demofile);
        conn->demofile = 0;
        conn->demoplaying = 0;
        conn->demorecording = 0;
    }

    SCR_StopCinematic();

    if (conn->state > CA_CHALLENGING) {
        sequence = conn->reliableSequence;
        if (sequence - conn->reliableAcknowledge - 128 <= 0) {
            conn->reliableSequence = sequence + 1;
            MSG_WriteReliableCommandToBuffer((const char *)"disconnect",
                                             conn->reliableCommands[conn->reliableSequence & 0x7f], 0x400);
            CL_WritePacket();
            CL_WritePacket();
            CL_WritePacket();
        }
    }

    oldState = conn->state;
#ifdef __EMSCRIPTEN__
    /* Returning to menus releases this room's WebSocket/UDP player slot. */
    extern void NET_Config(qboolean enabled);
    NET_Config(0);
#endif
    CL_ClearState();
    CL_ClearMutedList();

    memset(conn, 0, sizeof(*conn));
    conn->state = CA_DISCONNECTED;
    SND_DisconnectListener();

    if (oldState > CA_LOGO)
        active->keyCatchers &= 1;

    if (!CL_AllLocalClientsDisconnected())
        return;

    Dvar_SetBool(*(const dvar_t **)imp_sv_disableClientConsole, 0);
    *(int *)imp_cl_connectedToPureServer = 0;
    *(int *)imp_fs_checksumFeed = 0;
    *(void **)imp_bgs = 0;
}

void CL_Shutdown(void)
{
    static const char *const commands[] = {
        "cmd", "configstrings", "clientinfo", "vid_restart",
        "snd_restart", "disconnect", "record", "demo",
        "cinematic", "logo", "stoprecord", "connect",
        "reconnect", "localservers", "globalservers", "rcon",
        "setenv", "ping", "serverstatus", "showip",
        "fs_openedList", "fs_referencedList", "updatehunkusage", "updatescreen",
        "SaveTranslations", "SaveNewTranslations", "LoadTranslations", "startSingleplayer",
        "buyNow", "singlePlayLink", "setRecommended", "cubemapShot",
        "openScriptMenu", "openmenu", "closemenu"
    };
    unsigned int i;

    Com_Printf((const char *)"----- CL_Shutdown -----\n");

    if (recursive) {
        puts((const char *)"recursive shutdown");
        return;
    }

    recursive = 1;

    CL_Disconnect();

    if (!(Sys_IsMainThread() && !UI_IsFullscreen() && *(byte *)&clients[0] != 0 && *(int *)&clientConnections[0] > CA_LOGO)) {
        CL_ShutdownDebugData();

        if (cls.hunkUsersStarted) {
            CL_ShutdownCGame();
            CL_ShutdownUI();
            cls.whiteMaterial = 0;
            cls.consoleMaterial = 0;
            cls.consoleFont = 0;
            cls.hunkUsersStarted = 0;
        }

        SND_Shutdown();
        CL_ShutdownRef();
        CL_ShutdownInput();

        for (i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i)
            Cmd_RemoveCommand(commands[i]);

        memset(&cls, 0, sizeof(cls));
    }

    (*(LegacyHacks **)imp_legacyHacks)->cl_running = 0;
    recursive = 0;
    Com_Printf((const char *)"-----------------------\n");
}

void CL_Disconnect_f(void)
{
    connstate_t prevState;

    SCR_StopCinematic();
    prevState = ((clientConnection_t *)clc)->state;
    CL_Disconnect();

    if (prevState <= CA_LOGO)
        return;

    if (Sys_IsMainThread() && !UI_IsFullscreen() && *(byte *)&clients[0] != 0 && *(int *)&clientConnections[0] > 2)
        return;
    Com_Error(3, "EXE_DISCONNECTED_FROM_SERVER");
}

void CL_MapLoading(const char *mapname)
{
    LegacyHacks *legacy = *(LegacyHacks **)imp_legacyHacks;
    clientConnection_t *conn = (clientConnection_t *)clc;
    clientActive_t *active = (clientActive_t *)cl;

    if (!legacy->cl_running)
        return;

    g_waitingForServer = 0;
    Con_Close();
    active->keyCatchers = 0;
    active->displayHUDWithKeycatchUI = 0;

    if (*(int *)&clientConnections[0] > CA_CHALLENGING && I_stricmp(cls.servername, (const char *)"localhost") == 0) {
        conn->state = CA_CONNECTED;
        memset(conn->serverMessage, 0, sizeof(conn->serverMessage));
        memset(&active->gameState, 0, sizeof(active->gameState));
        conn->lastPacketSentTime = -9999;
    } else {
        Dvar_SetString(*(const dvar_t **)imp_nextmap, (const char *)"");
        CL_Disconnect();
        I_strncpyz(cls.servername, (const char *)"localhost", sizeof(cls.servername));
        UI_CloseAll();

        conn->state = CA_CHALLENGING;
        ((clientConnection_t *)conn)->connectTime = -3000;
        conn->qport = *(int *)imp_g_qport;
        NET_StringToAdr(cls.servername, &conn->serverAddress);
        CL_CheckForResend();
    }

    I_strncpyz(legacy->cl_serverloadmap, mapname, 0x40);
    legacy->cl_serverloadwaiting = 0;
    SND_FadeAllSounds(0.0f, 0.0f);
}

void CL_DemoCompleted(void)
{
    clientConnection_t *conn = (clientConnection_t *)clc;
    connstate_t oldState;

    if (conn->isTimeDemo) {
        int elapsed = Sys_Milliseconds() - conn->timeDemoStart;
        if (elapsed > 0) {
            double seconds = (double)elapsed / 1000.0;
            double fps = (double)conn->timeDemoFrames * 1000.0 / (double)elapsed;
            Com_Printf((const char *)"%i frames, %3.1f seconds: %3.1f fps\n", conn->timeDemoFrames, seconds, fps);
        }
    }

    if (conn->timeDemoLog) {
        FS_FCloseFile(conn->timeDemoLog);
        conn->timeDemoLog = 0;
    }

    SCR_StopCinematic();
    oldState = conn->state;
    CL_Disconnect();

    if (oldState <= CA_LOGO)
        return;

    if (Sys_IsMainThread() && !UI_IsFullscreen() && *(byte *)&clients[0] != 0 && *(int *)&clientConnections[0] > CA_LOGO)
        return;

    Com_Error(3, (const char *)"EXE_DISCONNECTED_FROM_SERVER");
}

void CL_ReadDemoMessage(void)
{
    clientConnection_t *conn;
    msg_t buf;
    byte bufData[MAX_MSGLEN];
    int serverMessageSequence;

    if (!clientConnections[0].demofile) {
        CL_DemoCompleted();
        return;
    }

    if (FS_Read(&serverMessageSequence, 4, clientConnections[0].demofile) != 4) {
        CL_DemoCompleted();
        return;
    }

    conn = (clientConnection_t *)clc;
    conn->serverMessageSequence = serverMessageSequence;

    MSG_Init(&buf, bufData, sizeof(bufData));
    if (FS_Read(&buf.cursize, 4, conn->demofile) != 4) {
        CL_DemoCompleted();
        return;
    }

    if (buf.cursize == -1) {
        CL_DemoCompleted();
        return;
    }

    if (buf.cursize > buf.maxsize)
        Com_Error(1, (const char *)"\x15"
                                   "CL_ReadDemoMessage: demoMsglen > MAX_MSGLEN");

    if (FS_Read(buf.data, buf.cursize, conn->demofile) != buf.cursize) {
        Com_Printf((const char *)"Demo file was truncated.\n");
        CL_DemoCompleted();
        return;
    }

    conn->lastPacketTime = cls.realtime;
    buf.readcount = 0;
    conn->reliableAcknowledge = MSG_ReadLong(&buf);

    if (conn->reliableAcknowledge < conn->reliableSequence - 128) {
        conn->reliableAcknowledge = conn->reliableSequence;
        return;
    }

    CL_ParseServerMessage(&buf);
}

void CL_PlayDemo_f(void)
{
    clientConnection_t *conn;
    const char *arg;
    char extension[32];
    char name[256];
    int argLen;
    int extLen;

    if (Cmd_Argc() != 2) {
        Com_Printf((const char *)"%s <demoname>\n", Cmd_Argv(0));
        return;
    }

    if ((*(const dvar_t **)imp_com_sv_running)->current.enabled) {
        Com_Printf((const char *)"listen server cannot play a demo.\n");
        return;
    }

    CL_Disconnect();

    arg = Cmd_Argv(1);
    Com_sprintf(extension, sizeof(extension), (const char *)".dm_%d", 1);
    argLen = strlen(arg);
    extLen = strlen(extension);

    if (argLen >= extLen && I_stricmp(arg + argLen - extLen, extension) == 0)
        Com_sprintf(name, sizeof(name), (const char *)"demos/%s", arg);
    else
        Com_sprintf(name, sizeof(name), (const char *)"demos/%s.dm_%d", arg, 1);

    conn = (clientConnection_t *)clc;
    FS_FOpenFileRead(name, &conn->demofile, 1);
    if (!conn->demofile)
        Com_Error(1, va((const char *)"EXE_ERR_NOT_FOUND\x15%s", name));

    I_strncpyz(conn->demoName, Cmd_Argv(1), sizeof(conn->demoName));
    Con_Close();

    conn->state = CA_CONNECTED;
    conn->demoplaying = 1;
    conn->isTimeDemo = I_stricmp(Cmd_Argv(0), (const char *)"timedemo") == 0;
    I_strncpyz(cls.servername, Cmd_Argv(1), sizeof(cls.servername));

    while (conn->state == CA_CONNECTED || conn->state == CA_LOADING)
        CL_ReadDemoMessage();

    conn->firstDemoFrameSkipped = 0;
}

void CL_ForwardCommandToServer(const char *string)
{
    const char *cmd = Cmd_Argv(0);
    clientConnection_t *conn = (clientConnection_t *)clc;
    const char *reliableCommand;
    int seq;

    if (!cmd || cmd[0] == '-')
        return;

    if (CG_IsConsoleCommandName(cmd))
        return;

    if (conn->demoplaying || conn->state <= 4 || cmd[0] == '+') {
        Com_Printf((const char *)"Unknown command \"%s\"\n", cmd);
        return;
    }

    reliableCommand = (Cmd_Argc() > 1) ? string : cmd;
    seq = conn->reliableSequence;
    if (seq - conn->reliableAcknowledge - 128 > 0) {
        Com_Error(1, (const char *)"EXE_ERR_CLIENT_CMD_OVERFLOW");
    }

    conn->reliableSequence = seq + 1;
    MSG_WriteReliableCommandToBuffer(
        reliableCommand,
        conn->reliableCommands[conn->reliableSequence & 0x7f],
        0x400);
}

void CL_ForwardToServer_f(void)
{
    clientConnection_t *conn;
    const char *cmd;
    int seq;
    int acked;
    int slot;

    if (clientConnections[0].state != CA_ACTIVE || clientConnections[0].demoplaying) {
        Com_Printf((const char *)"Not connected to a server.\n");
        return;
    }
    if (Cmd_Argc() <= 1)
        return;
    cmd = (const char *)Cmd_Args(1);

    conn = (clientConnection_t *)clc;
    seq = conn->reliableSequence;
    if (seq - conn->reliableAcknowledge - 128 > 0) {
        Com_Error(1, (const char *)"EXE_ERR_CLIENT_CMD_OVERFLOW");
    }
    conn->reliableSequence = seq + 1;
    slot = conn->reliableSequence & 0x7f;
    MSG_WriteReliableCommandToBuffer(cmd, (char *)conn->reliableCommands[slot], 0x400);
}

static void CL_SendPureChecksums(void)
{
    char cmd[1024];
    int i;

    Com_sprintf(cmd, sizeof(cmd), (const char *)"Va ");
    I_strncat(cmd, sizeof(cmd), FS_ReferencedIwdPureChecksums());

    for (i = 0; i < 2; ++i)
        cmd[i] = (char)(cmd[i] + 13 + i * 2);

    CL_AddReliableCommand(cmd);
}

void CL_DownloadsComplete(void)
{
    char cmd[1024];
    int i;

    if (clientConnections[0].downloadRestart) {
        clientConnections[0].downloadRestart = 0;
        FS_Restart(clc->checksumFeed);
        CL_AddReliableCommand_core((const char *)"donedl");
        return;
    }

    clientConnections[0].state = CA_LOADING;

    if (!(*(const dvar_t **)imp_com_sv_running)->current.enabled) {
        const char *info;
        char mapname[64];
        char gametype[64];
        void (*shutdownInput)(void);
        void (*shutdown)(int);

        info = CL_GetConfigString(0);
        I_strncpyz(mapname, Info_ValueForKey(info, (const char *)"mapname"), sizeof(mapname));
        I_strncpyz(gametype, Info_ValueForKey(info, (const char *)"g_gametype"), sizeof(gametype));
        UI_SetMap(mapname, gametype);
        SCR_UpdateScreenInternal();

        shutdownInput = re.SyncRenderThread;
        if (shutdownInput)
            shutdownInput();

        if (cls.hunkUsersStarted) {
            CL_ShutdownCGame();
            CL_ShutdownUI();
            cls.whiteMaterial = 0;
            cls.consoleMaterial = 0;
            cls.consoleFont = 0;
            cls.hunkUsersStarted = 0;
        }

        shutdown = *(void (**)(int))&re;
        if (shutdown)
            shutdown(0);

        cls.rendererStarted = 0;
        Com_Restart();
        CL_StartHunkUsers();
        SCR_UpdateScreenInternal();
    } else if (((clientActive_t *)cl)->cgameInitialized) {
        return;
    }

    Dvar_SetInt(*(const dvar_t **)imp_cl_paused, 1);
    CL_InitCGame();
    Dvar_SetInt(*(const dvar_t **)imp_cl_paused, 0);

    {
        const char *checksums = FS_ReferencedIwdPureChecksums();
        Com_sprintf(cmd, sizeof(cmd), (const char *)"Va ");
        I_strncat(cmd, sizeof(cmd), checksums);
    }
    for (i = 0; i < 2; ++i)
        cmd[i] = (char)(cmd[i] + 13 + i * 2);
    CL_AddReliableCommand_core(cmd);

    CL_WritePacket();
    CL_WritePacket();
    CL_WritePacket();
}

void CL_BeginDownload(const char *localName, const char *remoteName)
{
    clientConnection_t *conn;
    const char *cmd;
    int sequence;

    Com_DPrintf((const char *)"***** CL_BeginDownload *****\nLocalname: %s\nRemotename: %s\n****************************\n", localName, remoteName);
    conn = (clientConnection_t *)clc;
    I_strncpyz(conn->downloadName, localName, sizeof(conn->downloadName));
    Com_sprintf(conn->downloadTempName, sizeof(conn->downloadTempName), (const char *)"%s.tmp", localName);

    I_strncpyz((*(LegacyHacks **)imp_legacyHacks)->cl_downloadName, remoteName, 0x40);
    (*(LegacyHacks **)imp_legacyHacks)->cl_downloadSize = 0;
    (*(LegacyHacks **)imp_legacyHacks)->cl_downloadCount = 0;
    (*(LegacyHacks **)imp_legacyHacks)->cl_downloadTime = cls.realtime;

    ((clientConnection_t *)conn)->downloadBlock = 0;
    ((clientConnection_t *)conn)->downloadCount = 0;

    cmd = va((const char *)"download %s", remoteName);
    sequence = conn->reliableSequence;
    if (sequence - conn->reliableAcknowledge - 128 > 0)
        Com_Error(1, (const char *)"EXE_ERR_CLIENT_CMD_OVERFLOW");

    conn->reliableSequence = sequence + 1;
    MSG_WriteReliableCommandToBuffer(cmd, conn->reliableCommands[conn->reliableSequence & 0x7f], 0x400);
}

void CL_NextDownload(void)
{
    clientConnection_t *conn;
    char *remoteName;
    char *localName;
    char *next;

    if (!clientConnections[0].downloadList[0]) {
        CL_DownloadsComplete();
        return;
    }

    remoteName = clientConnections[0].downloadList;
    if (*remoteName == '@')
        ++remoteName;

    localName = strchr(remoteName, '@');
    if (!localName) {
        CL_DownloadsComplete();
        return;
    }

    *localName++ = '\0';
    next = strchr(localName, '@');
    if (next)
        *next++ = '\0';
    else
        next = localName + strlen(localName);

    CL_BeginDownload(localName, remoteName);
    conn = (clientConnection_t *)clc;
    conn->downloadRestart = 1;
    memmove(conn->downloadList, next, strlen(next) + 1);
}

void CL_InitDownloads(void)
{
    clientConnection_t *conn;
    const dvar_t *svRunning;
    char missingFiles[1024];

    FS_ShiftStr((const char *)"ni]Zm^l", 7);

    svRunning = *(const dvar_t **)imp_com_sv_running;
    if (svRunning->current.enabled || !cl_allowDownload->current.enabled) {
        if (FS_CompareIwds(missingFiles, sizeof(missingFiles), 0))
            Com_Printf((const char *)"\nWARNING: You are missing some files referenced by the server:\n%sYou might not be able to join the game\nGo to the settings menu to turn on autodownload, or get the file elsewhere\n\n", missingFiles);
        CL_DownloadsComplete();
        return;
    }

    conn = (clientConnection_t *)clc;
    if (!FS_CompareIwds(conn->downloadList, sizeof(conn->downloadList), 1)) {
        CL_DownloadsComplete();
        return;
    }

    Com_Printf((const char *)"Need iwds: %s\n", conn->downloadList);
    if (!conn->downloadList[0]) {
        CL_DownloadsComplete();
        return;
    }

    conn->state = CA_CONNECTED;
    CL_NextDownload();
}
