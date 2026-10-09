#include "common_types.h"
#include "imports.h"

#ifndef __EMSCRIPTEN__
#    include <mmintrin.h>
#endif
#include <string.h>

extern char *getenv(const char *name);

extern refexport_t re;

extern void Com_Error(errorParm_t code, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern void Com_DPrintf(const char *fmt, ...);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern const char *Com_Parse(const char **buf_p);
extern void Com_LoadBsp(const char *name);
extern void Com_UnloadBsp(void);
extern void Com_TouchMemory(void);
extern void Com_InitDObj(void);
extern void Com_SetWeaponInfoMemory(int mode);
extern void Com_FreeWeaponInfoMemory(int mode);
extern void Com_UnloadSoundAliases(int mode);
extern void Com_LoadSoundAliases(const char *loadspec, const char *zone, snd_alias_system_t flag);
extern snd_alias_t *Com_PickSoundAlias(const char *aliasname);
extern const char *Cmd_Argv(int n);
extern int Cmd_Argc(void);
extern void Cmd_TokenizeString(const char *text);
extern void Cmd_TokenizeString2(const char *text, int flags);
extern void Cmd_AddCommand(const char *cmd, void (*func)(void));
extern int I_stricmp(const char *s1, const char *s2);
extern int I_strnicmp(const char *s1, const char *s2, size_t n);
extern void I_strncat(char *dest, int size, const char *src);
extern void I_strncpyz(char *dest, const char *src, int maxLen);
extern void CM_LoadMap(const char *name, int *checksum);
extern void CM_LinkWorld(void);
extern void CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum);
extern void CG_Shutdown(void);
extern qboolean CG_ConsoleCommand(void);

extern void CG_DrawStringExt(float x, float y, const char *string, const vec_t *setColor, qboolean forceColor, qboolean shadow, float charHeight, qboolean adjust);
extern void CG_ArchiveState(MemoryFile *memFile);
extern void CL_SystemInfoChanged(void);
extern void CL_StartHunkUsers(void);
extern void CL_ReadDemoMessage(void);
extern int CL_GetSkelTimeStamp(int localClientNum);
extern void *CL_AllocSkelMemory(int localClientNum, int size);
extern void CL_ConsolePrint(int channel, const char *text, int duration, int lineWidth);
extern int DObjSkelExists(const struct DObj_s *obj, int timeStamp);
extern qboolean DObjSkelIsBoneUpToDate(const struct DObj_s *obj, int boneIndex);
extern qboolean DObjSkelAreBonesUpToDate(const struct DObj_s *obj, int *partBits);
extern int DObjGetAllocSkelSize(const struct DObj_s *obj);
extern void DObjCreateSkel(const struct DObj_s *obj, void *mem, int timeStamp);
extern void FX_AdjustCamera(refdef_t *refdef, float zfar);
extern int FS_FOpenFileByMode(const char *filename, int *handle, int mode);
extern int FS_FOpenFileWrite(const char *filename);
extern int FS_Read(void *buffer, int len, int handle);
extern int FS_Write(const void *buffer, int len, int handle);
extern void FS_FCloseFile(fileHandle_t handle);
extern void FS_Printf(int handle, const char *fmt, ...);
extern int Hunk_Used(void);
extern void *Z_MallocInternal(int size);
extern void Z_FreeInternal(void *ptr);
extern const char *Info_ValueForKey(const char *s, const char *key);
extern const char *GetBspExtension(void);
extern int Sys_Milliseconds(void);
extern void Con_Close(void);
extern void Con_ClearNotify(void);
extern void Con_ClearSubtitles(void);
extern const char *SEH_SafeTranslateString(const char *text);
extern const char *SEH_StringEd_GetString(const char *text);
extern const char *UI_ReplaceConversionString(const char *base, const char *replacement);
extern qboolean UI_SetActiveMenu(int menu);
extern int UI_GetActiveMenu(void);
extern qboolean UI_IsFullscreen(void);
extern void UI_CloseFocusedMenu(void);
extern void UI_KeyEvent(int key, int down);
extern void SCR_UpdateScreenInternal(void);
extern void Cbuf_AddText(const char *text);
extern void Dvar_SetString(void *dvar, const char *value);
extern void Dvar_SetInt(void *dvar, int value);
extern int Dvar_GetInt(const char *name);
extern void Dvar_GetUnpackedColorByName(const char *name, float *color);
extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);
extern void SetScreenScaling(float scaleX, float scaleY, int x, int y, int w, int h);
extern void StatMon_Warning(int type, int threshold, const char *msg);
extern const char *va(const char *fmt, ...);
extern unsigned char ColorIndex(unsigned char c);
extern int atoi(const char *str);
extern char *strcat(char *dest, const char *src);

static int warnCount;
static int warnCount_00f13084;
static char bigConfigString[8192];
extern const vec4_t g_color_table[];

#define RE (&re)
#define CLS ((clientStatic_t *)imp_cls)
#define CL_LOCAL ((clientActive_t *)*(void **)imp_cl)
#define CLUI_STATE ((clientConnection_t *)*(void **)imp_clc)

static const dvar_t *CL_DvarFromImport(void *importPtr)
{
    return importPtr ? *(const dvar_t **)importPtr : NULL;
}

static qboolean CL_DvarCurrentBool(void *importPtr)
{
    const dvar_t *dvar = CL_DvarFromImport(importPtr);
    return dvar && dvar->current.enabled;
}

static int CL_DvarCurrentInt(void *importPtr)
{
    const dvar_t *dvar = CL_DvarFromImport(importPtr);
    return dvar ? dvar->current.integer : 0;
}

static const char *CL_DvarCurrentString(void *importPtr)
{
    const dvar_t *dvar = CL_DvarFromImport(importPtr);
    return dvar ? dvar->current.string : NULL;
}

static float CL_ComTimescaleValue(void)
{
    return *(float *)imp_com_timescaleValue;
}

void CL_GetScreenDimensions(int *width, int *height, float *aspect);
qboolean CL_GetUserCmd(int cmdNumber, usercmd_t *ucmd);
int CL_GetCurrentCmdNumber(void);
void CL_GetCurrentSnapshotNumber(int *snapshotNumber, int *serverTime);
qboolean CL_GetSnapshot(int snapshotNumber, snapshot_t *snapshot);
void CL_SetUserCmdValue(int userCmdValue, int holdableValue, float sensitivityScale);
void CL_SetUserCmdInShellshock(qboolean shocked);
void CL_AddCgameCommand(const char *cmdName);
void CL_ConfigstringModified(void);
qboolean CL_GetServerCommand(int serverCommandNumber);
void CL_SetExpectedHunkUsage(const char *mapname);
void CL_CM_LoadMap(const char *mapname);
void CL_ShutdownCGame(void);
qboolean CL_DObjCreateSkelForBone(struct DObj_s *obj, int boneIndex, int localClientNum);
qboolean CL_DObjCreateSkelForBones(const struct DObj_s *obj, int *partBits, int localClientNum);
void CL_SubtitlePrint(const char *pszText, int iDuration, int iLineWidth);
const char *CL_GetConfigString(int index);
snd_alias_t *CL_PickSoundAlias(const char *aliasname);
struct XModel *CL_RegisterModel(const char *name);
struct GfxBrushModel *CL_RegisterInlineModel(int modelIndex);
void CL_FinishLoadingModels(void);
MaterialHandle CL_RegisterMaterial(const char *name, int imageTrack);
MaterialHandle CL_RegisterMaterialNoMip(const char *name, int imageTrack);
MaterialHandle CL_Material_Duplicate(MaterialHandle mtlCopy, const char *name);
void CL_DuplicateFont(FontHandle fontCopy, const char *name);
void CL_ClearScene(void);
void CL_DefaultVertexFrames(int vertCount, GfxWorldVertex *verts);
void CL_AddPolyToScene(MaterialHandle mtlHandle, unsigned short lmapIndex, unsigned short vertCount, const GfxWorldVertex *verts);
void CL_AddLightToScene(const vec_t *org, float radius, float r, float g, float b);
void CL_SetFog(int fogvar, float start, float end, int r, int g, int b, float density);
void CL_SwitchFog(int fogvar, int startTime, int transitionTime);
void CL_RenderScene(const refdef_t *fd);
int CL_BeginDelayedDrawing(void);
void CL_EndDelayedDrawing(int marker);
void CL_IssueDelayedDrawing(int marker);
void CL_SetViewport(int x, int y, int width, int height);
void CL_SaveScreen(void);
void CL_BlendSavedScreen(int fadeMsec);
void CL_DrawStretchPicPhysical(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material);
void CL_DrawQuadPic(int (*verts)[16][4], const vec_t *color, MaterialHandle material);
void CL_DrawSprite(MaterialHandle material, const byte *rgbaColor, const vec_t *pos, float radius, float minScreenRadius, int renderFxFlags);
void CL_ModelBounds(struct GfxBrushModel *model, vec_t *mins, vec_t *maxs);
int CL_MarkFragments(objective_t *(*points)[4], const vec_t *origin, objective_t *(*axis)[4], float radius, int maxPoints, GfxWorldVertex *verts, int maxFragments, GfxMarkFragment *fragmentBuffer, MaterialHandle markMaterial);
void CL_TrackStatistics(trStatistics_t *pStats);
qboolean CL_PickMaterial(const vec_t *org, const vec_t *dir, char *pszName, char *pszSurfaceFlags, char *pszContents, int iMaxChars);
void CL_LoadSoundAliases(const char *loadspec);
qboolean CL_Popup(const char *menu);
void CL_ClosePopup(const char *menu);
void CL_CloseInGameMenu(void);
void CL_CloseAllMenus(void);
void CL_SetWeaponInfoMemory(void);
void CL_FreeWeaponInfoMemory(void);
void CL_FX_AdjustCamera(refdef_t *refdef);
void CL_CapTurnRate(float maxPitchSpeed, float maxYawSpeed);
void LoadWorld(const char *mapname);
void CL_UpdateLevelHunkUsage(void);
void CL_StartLoading(const char *mapname, const char *gametype);
void CL_InitCGame(void);
qboolean CL_GameCommand(void);
void CL_AdjustTimeDelta(void);
void CL_SetADS(int ads);
void CL_DrawString(int x, int y, const char *pszString, qboolean bShadow, int iCharHeight);
void CL_ArchiveClientState(MemoryFile *memFile);
void CL_SetLodOrigin(const refdef_t *fd);
void CL_LookupColor(int c, vec_t *color);
void CL_SetUserCmdAimValues(vec_t *kickAngles);
void CL_UpdateColor(void);
void CL_SetFullScreenViewport(void);
void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material);
void CL_SyncTimes(void);
void CL_SetCGameTime(void);

static inline __attribute__((always_inline)) void CL_FirstSnapshot(void)
{
    clientActive_t *cl = CL_LOCAL;
    clientConnection_t *clui = CLUI_STATE;

    clui->state = 8;
    int serverTime = cl->snap.serverTime;
    clientStatic_t *cls = CLS;
    cl->serverTimeDelta = serverTime - cls->realtime;
    cl->oldServerTime = serverTime;
    clui->timeDemoBaseTime = serverTime;

    {
        const dvar_t **autorecDvarp = (const dvar_t **)imp_cl_activeAction;
        const char *autorecStr = (*autorecDvarp)->current.string;
        if (*autorecStr) {
            Cbuf_AddText(autorecStr);
            Cbuf_AddText("\n");
            Dvar_SetString((void *)*autorecDvarp, "");
        }
    }

}

void CL_GetScreenDimensions(int *width, int *height, float *aspect)
{
    clientStatic_t *cls = CLS;
    *width = cls->vidConfig.width;
    *height = cls->vidConfig.height;
    *aspect = cls->vidConfig.aspectRatioWindow;
}

qboolean CL_GetUserCmd(int cmdNumber, usercmd_t *ucmd)
{
    clientActive_t *cl = CL_LOCAL;
    int currentCmd = cl->cmdNumber;

    if (currentCmd < cmdNumber) {
        Com_Error(ERR_DROP, "CL_GetUserCmd: %i >= %i", cmdNumber, currentCmd);
        currentCmd = cl->cmdNumber;
    }

    if (cmdNumber <= currentCmd - 128) {
        return 0;
    }

    int idx = cmdNumber & 0x7f;
    *ucmd = cl->cmds[idx];
    return 1;
}

int CL_GetCurrentCmdNumber(void)
{
    clientActive_t *cl = CL_LOCAL;
    return cl->cmdNumber;
}

void CL_GetCurrentSnapshotNumber(int *snapshotNumber, int *serverTime)
{
    clientActive_t *cl = CL_LOCAL;
    *snapshotNumber = cl->snap.messageNum;
    *serverTime = cl->snap.serverTime;
}

qboolean CL_GetSnapshot(int snapshotNumber, snapshot_t *snapshot)
{
    clientActive_t *cl = CL_LOCAL;
    int count;
    int i;

    if (cl->snap.messageNum < snapshotNumber) {
        Com_Error(ERR_DROP, "CL_GetSnapshot: snapshotNumber > cl.snap.messageNum");
    }

    if (cl->snap.messageNum - snapshotNumber > 0x1f) {
        return 0;
    }

    int idx = snapshotNumber & 0x1f;
    clSnapshot_t *clSnap = &cl->snapshots[idx];

    if (clSnap->valid == 0) {
        return 0;
    }

    if (cl->parseEntitiesNum - clSnap->parseEntitiesNum > 0x7ff) {
        return 0;
    }
    if (cl->parseClientsNum - clSnap->parseClientsNum > 0x7ff) {
        return 0;
    }

    snapshot->snapFlags = clSnap->snapFlags;
    snapshot->serverCommandSequence = clSnap->serverCommandNum;
    snapshot->ping = clSnap->ping;
    snapshot->serverTime = clSnap->serverTime;

    memcpy(&snapshot->ps, &clSnap->ps, sizeof(playerState_t));

    count = clSnap->numEntities;
    if (count > 256) {
        if ((*(const dvar_t **)imp_com_statmon)->current.enabled) {
            StatMon_Warning(4, 3000, "CL_GetSnapshot: truncated entities");
        } else {
            Com_DPrintf("CL_GetSnapshot: truncated %i entities to %i\n", count, 256);
        }
        count = 256;
    }

    snapshot->numEntities = count;

    for (i = 0; i < count; i++) {
        int entIdx = (i + clSnap->parseEntitiesNum) & 0x7ff;
        memcpy(&snapshot->entities[i], &cl->parseEntities[entIdx], sizeof(entityState_t));
    }

    int clientCount = clSnap->numClients;
    if (clientCount > 64) {
        clientCount = 64;
    }
    snapshot->numClients = clientCount;

    for (i = 0; i < clientCount; i++) {
        int cliIdx = (i + clSnap->parseClientsNum) & 0x7ff;
        memcpy(&snapshot->clients[i], &cl->parseClients[cliIdx], sizeof(clientState_t));
    }

    return 1;
}

void CL_SetUserCmdValue(int userCmdValue, int holdableValue, float sensitivityScale)
{
    clientActive_t *cl = CL_LOCAL;
    cl->cgameUserCmdValue = userCmdValue;
    cl->cgameUserHoldableValue = holdableValue;
    cl->cgameSensitivity = sensitivityScale;
}

void CL_SetUserCmdInShellshock(qboolean shocked)
{
    clientActive_t *cl = CL_LOCAL;
    cl->cgameInShellshock = shocked;
}

void CL_AddCgameCommand(const char *cmdName)
{
    Cmd_AddCommand(cmdName, 0);
}

void CL_ConfigstringModified(void)
{
    gameState_t oldGs;
    int index;
    const char *s;
    int i, len;
    clientActive_t *cl;

    index = atoi(Cmd_Argv(1));
    if ((unsigned int)index > 0x7ff) {
        Com_Error(ERR_DROP, "CL_ConfigstringModified: bad index");
    }

    s = Cmd_Argv(2);

    cl = CL_LOCAL;

    {
        int offset = cl->gameState.stringOffsets[index];
        const char *old = cl->gameState.stringData + offset;
        if (strcmp(old, s) == 0)
            return;
    }

    memcpy(&oldGs, &cl->gameState, sizeof(gameState_t));

    memset(&cl->gameState, 0, sizeof(gameState_t));
    cl->gameState.dataCount = 1;

    for (i = 0; i < 0x800; i++) {
        const char *dup;

        if (i == index) {
            dup = s;
        } else {
            int oldOff = oldGs.stringOffsets[i];
            dup = oldGs.stringData + oldOff;
        }

        if (*dup == '\0')
            continue;

        len = strlen(dup) + 1;
        cl = CL_LOCAL;
        int writePos = cl->gameState.dataCount;
        if (writePos + len > 0x3e80) {
            Com_Error(ERR_DROP, "CL_ConfigstringModified: overflow");
            writePos = cl->gameState.dataCount;
        }

        cl->gameState.stringOffsets[i] = writePos;
        memcpy(cl->gameState.stringData + writePos, dup, len);
        cl->gameState.dataCount = writePos + len;
    }

    if (index == 1) {
        CL_SystemInfoChanged();
    }
}

qboolean CL_GetServerCommand(int serverCommandNumber)
{
    clientConnection_t *clui = CLUI_STATE;
    char *s;
    const char *cmd;
    int argc;

    if (serverCommandNumber <= clui->serverCommandSequence - 128) {
        if (clui->demoplaying) {
            return 0;
        }
        Com_Printf("CL_GetServerCommand: a]command was lost\n");
        Com_Printf("serverCommandNumber: %d\n", serverCommandNumber & 0x7f);
        {
            int j;
            for (j = 0; j < 128; j++) {
                clientConnection_t *clui2 = CLUI_STATE;
                Com_Printf("cmd %5d: %s\n", j, clui2->serverCommands[j & 0x7f]);
            }
        }
        Com_Error(ERR_DROP, "CL_GetServerCommand: too many missed");
    }

    clui = CLUI_STATE;
    if (serverCommandNumber > clui->serverCommandSequence) {
        Com_Error(ERR_DROP, "CL_GetServerCommand: requested beyond last");
    }

    s = clui->serverCommands[serverCommandNumber & 0x7f];
    clui->lastExecutedServerCommand = serverCommandNumber;

    if ((*(const dvar_t **)imp_cl_showServerCommands)->current.enabled) {
        Com_DPrintf("serverCommand: %i : %s\n", serverCommandNumber, s);
    }

restart:
    Cmd_TokenizeString(s);
    cmd = Cmd_Argv(0);
    argc = Cmd_Argc();

    if (cmd[0] < 'B' || cmd[0] > 'z') {
        return 1;
    }

    switch (cmd[0]) {
    case 'w':
        if (argc - 1 <= 0) {
            Com_Error(ERR_SERVERDISCONNECT, "EXE_DISCONNECTED");
        }
        {
            const char *reason = SEH_SafeTranslateString(Cmd_Argv(1));
            const char *base = SEH_SafeTranslateString("EXE_SERVERDISCONNECTREASON");
            const char *msg = UI_ReplaceConversionString(base, reason);
            Com_Error(ERR_SERVERDISCONNECT, msg);
        }

    case 'x':
        Cmd_TokenizeString2(s, 3);
        {
            const char *arg2 = Cmd_Argv(2);
            const char *arg1 = Cmd_Argv(1);
            Com_sprintf(bigConfigString, 0x2000, "%c %s %s", 'd', arg1, arg2);
        }
        return 0;
    case 'y':
        Cmd_TokenizeString2(s, 3);
        {
            const char *arg2 = Cmd_Argv(2);
            if (strlen(bigConfigString) + strlen(arg2) > 0x1fff) {
                Com_Error(ERR_DROP, "BCS overflow");
            }
            strcat(bigConfigString, arg2);
        }
        return 0;
    case 'z':
        Cmd_TokenizeString2(s, 3);
        {
            const char *arg2 = Cmd_Argv(2);
            if (strlen(bigConfigString) + strlen(arg2) + 1 > 0x1fff) {
                Com_Error(ERR_DROP, "BCS overflow");
            }
            strcat(bigConfigString, arg2);
        }
        s = bigConfigString;
        goto restart;
    case 'B':
    case 'n':
        Con_ClearNotify();
        Con_ClearSubtitles();
        memset(CL_LOCAL->cmds, 0, sizeof(CL_LOCAL->cmds));
        RE->ClearFlares();
        return 1;
    case 'd':
        Cmd_TokenizeString2(s, 3);
        CL_ConfigstringModified();
        Cmd_TokenizeString2(s, 3);
        return 1;
    default:
        return 1;
    }
}

void CL_SetExpectedHunkUsage(const char *mapname)
{
    int handle;
    int len;
    char *buf;
    const char *buftrav;
    const char *token;

    len = FS_FOpenFileByMode("hunkusage.dat", &handle, 0);
    if (len < 0) {
        goto set_default;
    }

    buf = (char *)Z_MallocInternal(len + 1);
    memset(buf, 0, len + 1);
    FS_Read(buf, len, handle);
    FS_FCloseFile(handle);

    buftrav = buf;
    while (1) {
        token = Com_Parse(&buftrav);
        if (!token || *token == '\0')
            break;

        if (I_stricmp(token, mapname) != 0)
            continue;

        token = Com_Parse(&buftrav);
        if (!token || *token == '\0')
            continue;

        Dvar_SetInt(*(void **)imp_com_expectedHunkUsage, atoi(token));
        Z_FreeInternal(buf);
        return;
    }

    Z_FreeInternal(buf);

set_default:
    Dvar_SetInt(*(void **)imp_com_expectedHunkUsage, 0);
}

void CL_CM_LoadMap(const char *mapname)
{
    int checksum;

    Com_LoadBsp(mapname);
    CM_LoadMap(mapname, &checksum);

    if (!(*(const dvar_t **)imp_com_sv_running)->current.enabled) {
        CM_LinkWorld();
    }
}

void CL_ShutdownCGame(void)
{
    clientActive_t *cl;

    Com_UnloadSoundAliases(1);
    cl = CL_LOCAL;
    if (cl->cgameInitCalled) {
        CG_Shutdown();
        cl->cgameInitCalled = 0;
        cl->cgameInitialized = 0;
    }
}

qboolean CL_DObjCreateSkelForBone(struct DObj_s *obj, int boneIndex, int localClientNum)
{
    int timeStamp;
    void *mem;

    timeStamp = CL_GetSkelTimeStamp(localClientNum);

    if (DObjSkelExists(obj, timeStamp)) {
        return DObjSkelIsBoneUpToDate(obj, boneIndex);
    }

    mem = CL_AllocSkelMemory(localClientNum, DObjGetAllocSkelSize(obj));
    if (mem) {
        DObjCreateSkel(obj, mem, timeStamp);
        return 0;
    }

    if (timeStamp != warnCount) {
        warnCount = timeStamp;
        Com_Printf("^3WARNING: CL_SKEL_MEMORY_SIZE exceeded - not calculating skeleton\n");
    }
    return 1;
}

qboolean CL_DObjCreateSkelForBones(const struct DObj_s *obj, int *partBits, int localClientNum)
{
    int timeStamp;
    void *mem;

    timeStamp = CL_GetSkelTimeStamp(localClientNum);

    if (DObjSkelExists(obj, timeStamp)) {
        return DObjSkelAreBonesUpToDate(obj, partBits);
    }

    mem = CL_AllocSkelMemory(localClientNum, DObjGetAllocSkelSize(obj));
    if (mem) {
        DObjCreateSkel(obj, mem, timeStamp);
        return 0;
    }

    if (timeStamp != warnCount_00f13084) {
        warnCount_00f13084 = timeStamp;
        Com_Printf("^3WARNING: CL_SKEL_MEMORY_SIZE exceeded - not calculating skeleton\n");
    }
    return 1;
}

void CL_SubtitlePrint(const char *pszText, int iDuration, int iLineWidth)
{
    const char *translated = SEH_StringEd_GetString(pszText);

    if (translated) {
        CL_ConsolePrint(3, translated, iDuration, iLineWidth);
        return;
    }

    if (!CL_DvarCurrentBool(imp_loc_warnings)) {
        CL_ConsolePrint(3, pszText, iDuration, iLineWidth);
        return;
    }

    if (CL_DvarCurrentBool(imp_loc_warningsAsErrors)) {
        Com_Error(ERR_LOCALIZATION, "Could not translate subtitle text: \"%s\"", pszText);
    } else {
        Com_Printf("^3WARNING: Could not translate subtitle text: \"%s\"\n", pszText);
    }

    CL_ConsolePrint(3, va("^1UNLOCALIZED(^7%s^1)^7", pszText), iDuration, iLineWidth);
}

const char *CL_GetConfigString(int index)
{
    clientActive_t *cl = CL_LOCAL;
    int offset = cl->gameState.stringOffsets[index];
    return cl->gameState.stringData + offset;
}

snd_alias_t *CL_PickSoundAlias(const char *aliasname)
{
    return Com_PickSoundAlias(aliasname);
}

struct XModel *CL_RegisterModel(const char *name)
{
    return RE->RegisterModel(name);
}

struct GfxBrushModel *CL_RegisterInlineModel(int modelIndex)
{
    return RE->RegisterInlineModel(modelIndex);
}

void CL_FinishLoadingModels(void)
{
    RE->FinishLoadingModels();
}

MaterialHandle CL_RegisterMaterial(const char *name, int imageTrack)
{
    return RE->RegisterMaterial(name, 3, imageTrack);
}

MaterialHandle CL_RegisterMaterialNoMip(const char *name, int imageTrack)
{
    return RE->RegisterMaterial(name, 0x30, imageTrack);
}

MaterialHandle CL_Material_Duplicate(MaterialHandle mtlCopy, const char *name)
{
    return RE->Material_Duplicate(mtlCopy, name);
}

void CL_DuplicateFont(FontHandle fontCopy, const char *name)
{
    RE->DuplicateFont(fontCopy, name);
}

void CL_ClearScene(void)
{
    RE->ClearScene();
}

void CL_DefaultVertexFrames(int vertCount, GfxWorldVertex *verts)
{
    RE->DefaultVertexFrames(vertCount, verts);
}

void CL_AddPolyToScene(MaterialHandle mtlHandle, unsigned short lmapIndex, unsigned short vertCount, const GfxWorldVertex *verts)
{
    RE->AddPolyToScene(mtlHandle, lmapIndex, vertCount, verts);
}

void CL_AddLightToScene(const vec_t *org, float radius, float r, float g, float b)
{
    RE->AddLightToScene(org, radius, r, g, b);
}

void CL_SetFog(int fogvar, float start, float end, int r, int g, int b, float density)
{
    RE->SetFog(fogvar, start, end, (unsigned char)r, (unsigned char)g, (unsigned char)b, density);
}

void CL_SwitchFog(int fogvar, int startTime, int transitionTime)
{
    RE->SwitchFog(fogvar, startTime, transitionTime);
}

extern void *s_cmdList;
void CL_RenderScene(const refdef_t *fd)
{
    RE->RenderScene(fd);
}

int CL_BeginDelayedDrawing(void)
{
    return RE->BeginDelayedDrawing();
}

void CL_EndDelayedDrawing(int marker)
{
    RE->EndDelayedDrawing(marker);
}

void CL_IssueDelayedDrawing(int marker)
{
    RE->IssueDelayedDrawing(marker);
}

void CL_SetViewport(int x, int y, int width, int height)
{
    RE->SetViewport(x, y, width, height);
}

void CL_SaveScreen(void)
{
    RE->SaveScreen();
}

void CL_BlendSavedScreen(int fadeMsec)
{
    RE->BlendSavedScreen(fadeMsec);
}

void CL_DrawStretchPicPhysical(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material)
{
    RE->DrawStretchPic(x, y, w, h, s1, t1, s2, t2, color, material);
}

void CL_DrawQuadPic(int (*verts)[16][4], const vec_t *color, MaterialHandle material)
{
    RE->DrawQuadPic(verts, color, material);
}

void CL_DrawSprite(MaterialHandle material, const byte *rgbaColor, const vec_t *pos, float radius, float minScreenRadius, int renderFxFlags)
{
    RE->DrawSprite(material, rgbaColor, pos, radius, minScreenRadius, renderFxFlags);
}

void CL_ModelBounds(struct GfxBrushModel *model, vec_t *mins, vec_t *maxs)
{
    RE->ModelBounds(model, mins, maxs);
}

int CL_MarkFragments(objective_t *(*points)[4], const vec_t *origin, objective_t *(*axis)[4], float radius, int maxPoints, GfxWorldVertex *verts, int maxFragments, GfxMarkFragment *fragmentBuffer, MaterialHandle markMaterial)
{
    return RE->MarkFragments(points, origin, axis, radius, maxPoints, verts, maxFragments, fragmentBuffer, markMaterial);
}

void CL_TrackStatistics(trStatistics_t *pStats)
{
    RE->TrackStatistics(pStats);
}

qboolean CL_PickMaterial(const vec_t *org, const vec_t *dir, char *pszName, char *pszSurfaceFlags, char *pszContents, int iMaxChars)
{
    return RE->PickMaterial(org, dir, pszName, pszSurfaceFlags, pszContents, iMaxChars);
}

void CL_LoadSoundAliases(const char *loadspec)
{
    Com_LoadSoundAliases(loadspec, "all_mp", 1);
}

qboolean CL_Popup(const char *menu)
{
    clientConnection_t *clui = CLUI_STATE;
    int fullscreen = UI_IsFullscreen();
    int activeMenu = UI_GetActiveMenu();
    static int traceCount;

    if (traceCount++ < 32) {
        if (getenv("MTRACE"))
            Com_Printf("[menu-trace] CL_Popup menu='%s' state=%d demo=%d fullscreen=%d active=%d\n",
                       menu, clui->state, clui->demoplaying, fullscreen, activeMenu);
    }

    if (clui->state != 8)
        return 0;

    if (clui->demoplaying)
        return 0;

    if (fullscreen)
        return 0;

    if (I_stricmp(menu, "UIMENU_INGAME") == 0) {
        UI_SetActiveMenu(8);
        return 1;
    }

    if (I_strnicmp(menu, "UIMENU_SCRIPT_POPUP", 0x13) != 0) {
        return 1;
    }

    if (I_stricmp(menu, "UIMENU_SCRIPT_POPUP_SUBMENU") == 0) {
        return UI_SetActiveMenu(10);
    }

    return UI_SetActiveMenu(9);
}

void CL_ClosePopup(const char *menu)
{
    if (UI_IsFullscreen())
        return;

    if (I_stricmp(menu, "UIMENU_SCRIPT_POPUP_SUBMENU") == 0) {
        if (UI_GetActiveMenu() == 9 || UI_GetActiveMenu() == 10)
            UI_SetActiveMenu(0);
        return;
    }

    if (I_stricmp(menu, "UIMENU_SCRIPT_POPUP") == 0) {
        if (UI_GetActiveMenu() == 9 || UI_GetActiveMenu() == 10)
            UI_SetActiveMenu(0);
    }
}

void CL_CloseInGameMenu(void)
{
    if (UI_IsFullscreen())
        return;
    if (UI_GetActiveMenu() != 2)
        return;
    UI_CloseFocusedMenu();
}

void CL_CloseAllMenus(void)
{
    if (UI_IsFullscreen())
        return;
    UI_KeyEvent(0x1b, 1);
    UI_KeyEvent(0x1b, 1);
    UI_KeyEvent(0x1b, 1);
}

void CL_SetWeaponInfoMemory(void)
{
    Com_SetWeaponInfoMemory(2);
}

void CL_FreeWeaponInfoMemory(void)
{
    Com_FreeWeaponInfoMemory(2);
}

void CL_FX_AdjustCamera(refdef_t *refdef)
{
    float zfar = RE->GetFarPlaneDist();
    FX_AdjustCamera(refdef, zfar);
}

void CL_CapTurnRate(float maxPitchSpeed, float maxYawSpeed)
{
    clientActive_t *cl = CL_LOCAL;
    cl->cgameMaxPitchSpeed = maxPitchSpeed;
    cl->cgameMaxYawSpeed = maxYawSpeed;
}

void LoadWorld(const char *mapname)
{
    RE->LoadWorld(mapname, 0);
    Com_UnloadBsp();
}

void CL_UpdateLevelHunkUsage(void)
{
    int handle;
    int len;
    int memusage;
    char *buf;
    char *outbuf;
    const char *buftrav;
    const char *token;
    char outstr[256];
    int buflen;

    memusage = Hunk_Used();

    len = FS_FOpenFileByMode("hunkusage.dat", &handle, 0);
    if (len < 0)
        goto write_new;

    buflen = len + 1;
    buf = (char *)Z_MallocInternal(buflen);
    memset(buf, 0, buflen);
    outbuf = (char *)Z_MallocInternal(buflen);
    memset(outbuf, 0, buflen);

    FS_Read(buf, len, handle);
    FS_FCloseFile(handle);

    buftrav = buf;
    outbuf[0] = '\0';

    while (1) {
        token = Com_Parse(&buftrav);
        if (!token || *token == '\0')
            break;

        {
            clientActive_t *cl = CL_LOCAL;
            if (I_stricmp(token, cl->mapname) == 0) {
                token = Com_Parse(&buftrav);
                if (!token || *token == '\0')
                    continue;
                if (memusage == atoi(token)) {
                    Z_FreeInternal(buf);
                    Z_FreeInternal(outbuf);
                    goto write_new;
                }
                continue;
            }
        }

        I_strncat(outbuf, buflen, token);
        I_strncat(outbuf, buflen, " ");

        token = Com_Parse(&buftrav);
        if (!token || *token == '\0') {
            Com_Error(ERR_DROP, "CL_UpdateLevelHunkUsage: bad hunkusage.dat");
            continue;
        }

        I_strncat(outbuf, buflen, token);
        I_strncat(outbuf, buflen, "\n");
    }

    handle = FS_FOpenFileWrite("hunkusage.dat");
    if (!handle) {
        Com_Error(ERR_DROP, va("EXE_ERR_CANT_CREATE\x15%s", "hunkusage.dat"));
    }

    {
        int outLen = strlen(outbuf);
        if (FS_Write(outbuf, outLen, handle) != outLen) {
            Com_Error(ERR_DROP, va("EXE_ERR_CANT_WRITE\x15%s", "hunkusage.dat"));
        }
    }
    FS_FCloseFile(handle);

    Z_FreeInternal(buf);
    Z_FreeInternal(outbuf);

write_new:

    len = FS_FOpenFileByMode("hunkusage.dat", &handle, 2);
    if (!handle) {
        Com_Error(ERR_DROP, "CL_UpdateLevelHunkUsage: cannot open for append");
    }

    {
        clientActive_t *cl = CL_LOCAL;
        Com_sprintf(outstr, 256, "%s %i\n", cl->mapname, memusage);
    }
    FS_Write(outstr, strlen(outstr), handle);
    FS_FCloseFile(handle);

    len = FS_FOpenFileByMode("hunkusage.dat", &handle, 0);
    if (len >= 0) {
        FS_FCloseFile(handle);
    }
}

int g_cginit_loading = 0;

void CL_StartLoading(const char *mapname, const char *gametype)
{
    char *ptr = *(char **)imp_legacyHacks;
    if (*(int *)(ptr + 4) == 0)
        return;
    CL_StartHunkUsers();
    SCR_UpdateScreenInternal();
}

void CL_InitCGame(void)
{
    int t1;
    char mapname[64];
    clientActive_t *cl;

    t1 = Sys_Milliseconds();
    Con_Close();

    cl = CL_LOCAL;
    {
        int offset = cl->gameState.stringOffsets[0];
        const char *systemInfo = cl->gameState.stringData + offset;
        I_strncpyz(mapname, Info_ValueForKey(systemInfo, "mapname"), 64);
    }

    {
        const char *ext = GetBspExtension();
        Com_sprintf(cl->mapname, 64, "maps/mp/%s.%s", mapname, ext);
    }

    if (!(*(const dvar_t **)imp_com_sv_running)->current.enabled) {
        Com_InitDObj();
        CL_SetExpectedHunkUsage(cl->mapname);
    }

    {
        clientConnection_t *clui = CLUI_STATE;
        clui->state = 6;
        cl->cgameInitCalled = 1;
        CG_Init(clui->clientNum, clui->lastExecutedServerCommand, clui->serverMessageSequence);
        cl->cgameInitialized = 1;
        clui->state = 7;
    }

    {
        int t2 = Sys_Milliseconds();
        double elapsed = (double)(t2 - t1) / 1000.0;
        Com_Printf("CL_InitCGame: %5.2f seconds\n", elapsed);
    }

    RE->EndRegistration();
    Com_TouchMemory();
    Con_ClearNotify();
    Con_ClearSubtitles();
    CL_UpdateLevelHunkUsage();
}

qboolean CL_GameCommand(void)
{
    clientActive_t *cl = CL_LOCAL;
    if (!cl->cgameInitialized)
        return 0;
    return CG_ConsoleCommand();
}

void CL_AdjustTimeDelta(void)
{
    clientActive_t *cl;
    clientConnection_t *clui;
    int serverTime, frameTime, newDelta, serverTimeDelta, deltaDiff;

    cl = CL_LOCAL;
    cl->newSnapshots = 0;

    clui = CLUI_STATE;
    if (clui->demoplaying)
        return;

    serverTime = cl->snap.serverTime;
    frameTime = serverTime - cl->oldSnapServerTime;

    {
        clientStatic_t *cls = CLS;
        newDelta = serverTime - cls->realtime - frameTime - 5;
    }
    serverTimeDelta = cl->serverTimeDelta;
    deltaDiff = newDelta - serverTimeDelta;

    if (deltaDiff > 0) {
        if ((unsigned int)frameTime <= 500) {
            deltaDiff -= frameTime;
            if (deltaDiff < 0)
                goto smooth;
        }
    } else {
        deltaDiff = -deltaDiff;
    }

    if (deltaDiff > 500) {
        cl->serverTimeDelta = newDelta;
        cl = CL_LOCAL;
        cl->oldServerTime = cl->snap.serverTime;
        cl->serverTime = cl->snap.serverTime;
        if ((*(const dvar_t **)imp_cl_showTimeDelta)->current.enabled) {
            Com_Printf("cl_showTimeDelta: reset\n");
        }
        goto debug_print;
    }

    if (deltaDiff > 100) {

        if ((*(const dvar_t **)imp_cl_showTimeDelta)->current.enabled) {
            Com_Printf("cl_showTimeDelta: average\n");
        }
        cl = CL_LOCAL;
        cl->serverTimeDelta = (newDelta + cl->serverTimeDelta) >> 1;
        goto debug_print;
    }

smooth: {
    float timescale = CL_ComTimescaleValue();
    if (timescale != 1.0f)
        goto debug_print;
}

    if (cl->extrapolatedSnapshot) {
        cl->extrapolatedSnapshot = 0;
        cl = CL_LOCAL;
        cl->serverTimeDelta -= 2;
    } else if (newDelta > serverTimeDelta) {
        cl->serverTimeDelta = serverTimeDelta + 1;
    } else if (newDelta < serverTimeDelta) {
        cl->serverTimeDelta = serverTimeDelta - 1;
    }

debug_print:
    if ((*(const dvar_t **)imp_cl_showTimeDelta)->current.enabled) {
        cl = CL_LOCAL;
        {
            clientStatic_t *cls = CLS;
            Com_Printf("client time: %i, server time: %i\n",
                       cls->realtime + cl->serverTimeDelta,
                       cl->snap.serverTime);
        }
        Com_Printf("snapshot delta: %i, time delta: %i\n",
                   newDelta, cl->serverTimeDelta);
    }
}

void CL_SetADS(int ads)
{
    clientActive_t *cl = CL_LOCAL;
    cl->usingAds = (unsigned char)ads;
}

void CL_DrawString(int x, int y, const char *pszString, qboolean bShadow, int iCharHeight)
{
    clientActive_t *cl = CL_LOCAL;
    if (!cl->cgameInitialized)
        return;
    CG_DrawStringExt((float)x, (float)y, pszString, 0, 0, bShadow, (float)iCharHeight, 1);
}

void CL_ArchiveClientState(MemoryFile *memFile)
{
    CG_ArchiveState(memFile);
    RE->ArchiveFogState(memFile);
}

void CL_SetLodOrigin(const refdef_t *fd)
{
    RE->SetLodOrigin(fd);
}

void CL_LookupColor(int c, vec_t *color)
{
    unsigned int idx = ColorIndex((unsigned char)c);
    float *src;

    if (idx <= 7) {
        src = (float *)&g_color_table[idx];
    } else if ((unsigned char)c == '8') {
        src = (float *)CL_LOCAL->color_allies;
    } else if ((unsigned char)c == '9') {
        src = (float *)CL_LOCAL->color_axis;
    } else {
        color[0] = 1.0f;
        color[1] = 1.0f;
        color[2] = 1.0f;
        color[3] = 1.0f;
        return;
    }

    color[0] = src[0];
    color[1] = src[1];
    color[2] = src[2];
    color[3] = src[3];
}

void CL_SetUserCmdAimValues(vec_t *kickAngles)
{
    clientActive_t *cl = CL_LOCAL;
    vec_t *dst = cl->cgameKickAngles;
    dst[0] = kickAngles[0];
    dst[1] = kickAngles[1];
    dst[2] = kickAngles[2];
}

void CL_UpdateColor(void)
{
    clientActive_t *cl = CL_LOCAL;
    float *alliesColor = (float *)cl->color_allies;
    float *axisColor;

    Dvar_GetUnpackedColorByName("g_TeamColor_Allies", alliesColor);
    alliesColor[3] = 1.0f;

    axisColor = (float *)cl->color_axis;
    Dvar_GetUnpackedColorByName("g_TeamColor_Axis", axisColor);
    axisColor[3] = 1.0f;

    RE->UpdateColor(alliesColor, axisColor);
}

void CL_SetFullScreenViewport(void)
{
    clientStatic_t *cls = CLS;

    RE->SetViewport(0, 0, cls->vidConfig.width, cls->vidConfig.height);
    SetScreenScaling(1.0f, 1.0f, 0, 0, cls->vidConfig.width, cls->vidConfig.height);
}

void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material)
{
    CalcScreenPlacement(&x, &y, &w, &h, horzAlign, vertAlign);
    RE->DrawStretchPic(x, y, w, h, s1, t1, s2, t2, color, material);
}

void CL_SyncTimes(void)
{
    clientConnection_t *clui = CLUI_STATE;
    if (clui->state != 8)
        return;

    {
        clientActive_t *cl = CL_LOCAL;
        if (cl->snap.snapFlags & 2)
            return;
    }

    CL_FirstSnapshot();
}

void CL_SetCGameTime(void)
{
    clientConnection_t *clui;
    clientActive_t *cl;
    int state;

    clui = CLUI_STATE;
    state = clui->state;

    if (state != 8) {
        if (state != 7)
            return;

        if (clui->demoplaying) {

            if (!clui->firstDemoFrameSkipped) {
                clui->firstDemoFrameSkipped = 1;
                return;
            }
            CL_ReadDemoMessage();
        }

        cl = CL_LOCAL;
        if (cl->newSnapshots) {
            cl->newSnapshots = 0;
            if (!(cl->snap.snapFlags & 2)) {
                CL_FirstSnapshot();
            }
        }

        clui = CLUI_STATE;
        if (clui->state != 8)
            return;
    }

    cl = CL_LOCAL;
    if (!cl->snap.valid) {
        Com_Error(ERR_DROP, "CL_SetCGameTime: no snapshot");
    }

    if (CL_DvarCurrentInt(imp_sv_paused) &&
        CL_DvarCurrentInt(imp_cl_paused) &&
        CL_DvarCurrentBool(imp_com_sv_running)) {
        return;
    }

    cl = CL_LOCAL;
    {
        int serverTime = cl->snap.serverTime;
        if (serverTime < cl->oldFrameServerTime) {
            clientStatic_t *cls = CLS;
            if (I_stricmp(cls->servername, "localhost") != 0) {
                Com_Error(ERR_DROP, "cl.snap.serverTime < cl.oldFrameServerTime");
            }

            if (!(cl->snap.snapFlags & 2)) {
                CL_FirstSnapshot();
            }
        }
    }

    cl = CL_LOCAL;
    {
        int serverTime = cl->snap.serverTime;
        cl->oldFrameServerTime = serverTime;
    }

    clui = CLUI_STATE;
    {
        int demoPlaying = clui->demoplaying;
        clientStatic_t *cls = CLS;

        if (!demoPlaying || !CL_DvarCurrentBool(imp_cl_freezeDemo)) {

            int timeDelta = cl->serverTimeDelta;
            int realtime = cls->realtime;
            cl->serverTime = realtime + timeDelta;

            int oldServerTime = cl->oldServerTime;
            if (cl->serverTime < oldServerTime) {
                cl->serverTime = oldServerTime;
            }
            cl->oldServerTime = cl->serverTime;

            int serverTime = cl->snap.serverTime;
            /* Extrapolation means running AHEAD of the newest snapshot. The
             * reversed comparison kept subtracting time while we were already
             * behind, until the 500 ms reset snapped prediction backwards. */
            if (cl->serverTime >= serverTime - 5) {
                cl->extrapolatedSnapshot = 1;
                if (CL_DvarCurrentBool(imp_cl_showTimeDelta)) {
                    Com_Printf("%i extrapolating\n", cl->serverTime);
                }
            }
        } else {

            if (!clui->timeDemoLog) {

                char *mapPath = cl->mapname;
                char *base = mapPath;
                char *dotPos = 0;
                while (*mapPath) {
                    if (*mapPath == '/' || *mapPath == '\\') {
                        base = mapPath + 1;
                        dotPos = 0;
                    } else if (*mapPath == '.') {
                        dotPos = mapPath;
                    }
                    mapPath++;
                }
                {
                    const char *name;
                    if (dotPos) {
                        name = va("%s", base);
                        ((char *)name)[dotPos - base] = '\0';
                    } else {
                        name = base;
                    }
                    int mode = Dvar_GetInt("cl_timedemoMode");
                    clui->timeDemoLog = FS_FOpenFileWrite(va("demos/timedemo_%s_mode_%i.csv", name, mode));
                }
            }

            {
                int msec = Sys_Milliseconds();
                clui = CLUI_STATE;
                if (!clui->timeDemoStart) {
                    clui->timeDemoStart = msec;
                } else if (clui->timeDemoLog) {
                    FS_Printf(clui->timeDemoLog, "%i,%i\n",
                              clui->timeDemoFrames,
                              msec - clui->timeDemoPrev);
                }
                clui->timeDemoPrev = msec;
                int frameNum = clui->timeDemoFrames + 1;
                clui->timeDemoFrames = frameNum;
                cl = CL_LOCAL;
                cl->serverTime = clui->timeDemoBaseTime + frameNum * 50;
            }
        }
    }

    cl = CL_LOCAL;

    if (cl->newSnapshots) {
        CL_AdjustTimeDelta();
    }

    clui = CLUI_STATE;
    if (!clui->demoplaying)
        return;

    if (!clui->isTimeDemo) {
        cl = CL_LOCAL;
    } else {
        cl = CL_LOCAL;
    }

    while (cl->serverTime >= cl->snap.serverTime) {
        CL_ReadDemoMessage();
        clui = CLUI_STATE;
        if (clui->state != 8)
            return;
        cl = CL_LOCAL;
    }
}

const vec4_t g_color_table[36] = { { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 2.164135E-39f, 2.163334E-39f, 2.163334E-39f, 2.163334E-39f }, { 2.163334E-39f, 2.163334E-39f, 2.163334E-39f, 2.163334E-39f }, { 2.163334E-39f, 2.163334E-39f, 2.163334E-39f, 2.163334E-39f }, { 2.163334E-39f, 2.163334E-39f, 2.163334E-39f, 2.163334E-39f }, { 2.163334E-39f, 2.163334E-39f, 2.163334E-39f, 2.163334E-39f }, { 2.163334E-39f, 2.163334E-39f, 2.163334E-39f, 2.163334E-39f }, { 2.163334E-39f, 2.163334E-39f, 2.163334E-39f, 2.163334E-39f }, { 2.163334E-39f, 2.163334E-39f, 2.163334E-39f, 2.163334E-39f }, { 2.163334E-39f, 2.163334E-39f, 2.164221E-39f, 2.163334E-39f }, { 2.163334E-39f, 2.163334E-39f, 2.163334E-39f, 2.163334E-39f }, { 2.163334E-39f, 2.163334E-39f, 2.163334E-39f, 2.163334E-39f }, { 2.164135E-39f, 2.163334E-39f, 2.163334E-39f, 2.163334E-39f }, { 2.163334E-39f, 2.163334E-39f, 2.163334E-39f, 2.163334E-39f }, { 2.163334E-39f, 2.163671E-39f, 2.163711E-39f, 2.163842E-39f }, { 2.163983E-39f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 2.20311E-39f, 2.203044E-39f, 2.202998E-39f, 2.202981E-39f }, { 2.202952E-39f, 2.202906E-39f, 2.203169E-39f, 2.20314E-39f }, { 2.202618E-39f, 2.202645E-39f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f /* raw 0x7FFFFFFF (non-finite) */, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f /* raw 0x7FFFFFFF (non-finite) */, 0.0f, 0.0f, 0.0f }, { 2.224965E-39f, 2.225545E-39f, 2.225545E-39f, 2.225642E-39f }, { 2.225642E-39f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } };
