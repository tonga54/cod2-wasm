#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include "cod2_feature_config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

extern serverStatusDvar_t serverStatusDvars[23];

char etaText[18] = { 0x45, 0x58, 0x45, 0x5f, 0x45, 0x53, 0x54, 0x5f, 0x54, 0x49, 0x4d, 0x45, 0x5f, 0x4c, 0x45, 0x46, 0x54, 0x0 };
int tleEstimates[80] = { 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c };
char xferText[15] = { 0x45, 0x58, 0x45, 0x5f, 0x54, 0x52, 0x41, 0x4e, 0x53, 0x5f, 0x52, 0x41, 0x54, 0x45, 0x0 };

const char *MonthAbbrev[12] = { (const char *)&str_002a9d68, (const char *)&str_002a9d80, (const char *)&str_002a9d98, (const char *)&str_002a9dac, (const char *)&str_002a9dc0, (const char *)&str_002a9dd4, (const char *)&str_002a9de8, (const char *)&str_002a9dfc, (const char *)&str_002a9e14, (const char *)&str_002a9e2c, (const char *)&str_002a9e44, (const char *)&str_002a9e5c };
const char *netSources[3] = { (const char *)&str_002a9e74, (const char *)&str_002a9e80, (const char *)&str_002a9e90 };

int lastColumn = 0xffffffff;

#ifndef __EMSCRIPTEN__
#    define UI_REGPARM1 __attribute__((regparm(1)))
#else
#    define UI_REGPARM1
#endif

void diag_drawtext(const char *text, void *font, float x, float y, float scale)
{
    (void)text;
    (void)font;
    (void)x;
    (void)y;
    (void)scale;
}

extern const char *va(const char *fmt, ...);
extern const dvar_t *loc_warnings;
extern const dvar_t *loc_warningsAsErrors;
extern const dvar_t *ui_smallFont;
extern const dvar_t *ui_bigFont;
extern const dvar_t *ui_extraBigFont;
extern const dvar_t *ui_netSource;
extern const dvar_t *ui_currentMap;
extern const dvar_t *ui_netGameType;
extern const dvar_t *ui_joinGameType;
extern const dvar_t *ui_dedicated;
extern const dvar_t *ui_currentNetMap;
extern const dvar_t *ui_browserShowFull;
extern const dvar_t *ui_browserShowEmpty;
extern const dvar_t *ui_browserShowPassword;
extern const dvar_t *ui_browserShowNoPassword;
extern const dvar_t *ui_browserShowPure;
#if COD2_IS_PATCH_13

const dvar_t *ui_browserShowPunkBuster;
#endif
extern const dvar_t *ui_browserFriendlyfire;
extern const dvar_t *ui_browserKillcam;
extern const dvar_t *ui_serverStatusTimeOut;
extern struct sharedUiInfo_t sharedUiInfo;
extern uiInfo_t *uiInfo;
extern uiInfo_t uiInfoArray[1];

COD2_ASSERT_FIELD(struct sharedUiInfo_t, mapCount, 4944);
COD2_ASSERT_FIELD(struct sharedUiInfo_t, mapList, 4948);
COD2_ASSERT_FIELD(struct mapInfo, active, 0xa0);
COD2_ASSERT_SIZE (struct mapInfo, 0xa4);
COD2_ASSERT_FIELD(struct sharedUiInfo_t, playerClientNums, 0x1048);
COD2_ASSERT_FIELD(struct sharedUiInfo_t, pendingServerStatus, 0x1b9f0);
COD2_ASSERT_FIELD(struct pendingServerStatus_t, server, 0x4);
COD2_ASSERT_FIELD(struct pendingServer_t, valid, 0x88);
COD2_ASSERT_SIZE (struct pendingServer_t, 0x8c);
COD2_ASSERT_FIELD(mapInfo, mapName, 0);
COD2_ASSERT_FIELD(mapInfo, active, 160);
COD2_ASSERT_SIZE(mapInfo, 164);
COD2_ASSERT_FIELD(struct sharedUiInfo_t, assets, 0);
COD2_ASSERT_FIELD(CachedAssets_t, whiteMaterial, 32);
COD2_ASSERT_FIELD(struct sharedUiInfo_t, serverHardwareIconList, 25940);
COD2_ASSERT_FIELD(struct sharedUiInfo_t, joinGameTypes, 4688);
COD2_ASSERT_FIELD(gameTypeInfo, gameType, 0);
COD2_ASSERT_FIELD(struct sharedUiInfo_t, serverStatus, 27528);
COD2_ASSERT_FIELD(serverStatus_t, sortKey, 1112);
COD2_ASSERT_FIELD(serverStatus_t, sortDir, 1116);
COD2_ASSERT_FIELD(serverStatus_t, currentServer, 1128);
COD2_ASSERT_FIELD(serverStatus_t, displayServers, 1132);
COD2_ASSERT_FIELD(serverStatus_t, motdWidth, 81164);
COD2_ASSERT_FIELD(struct sharedUiInfo_t, serverStatusInfo, 109800);
COD2_ASSERT_FIELD(serverStatusInfo_t, numLines, 3328);
COD2_ASSERT_FIELD(struct sharedUiInfo_t, serverStatusAddress, 109736);
COD2_ASSERT_FIELD(uiInfo_t, uiDC, 0);
COD2_ASSERT_FIELD(displayContextDef_t, bias, 0);
COD2_ASSERT_FIELD(displayContextDef_t, realTime, 4);
COD2_ASSERT_FIELD(dvar_t, current, 8);

#if COD2_FEATURE_GAMEPAD
extern void UI_ConsoleMode_Init(void *info);
extern int UI_ConsoleMode_SuppressCursor(void);
extern void UI_ConsoleMode_DrawGlyphs(void *info);
extern void UI_Gamepad_RegisterMenus(uiInfo_t *info);
#endif
extern qboolean g_editingField;
extern const dvar_t *ui_gametype;
extern const dvar_t *ui_netGameTypeName;
extern const dvar_t *ui_browserShowDedicated;
extern const dvar_t *ui_browserMod;
extern const dvar_t *ui_playerProfileCount;
extern const dvar_t *ui_playerProfileSelected;
extern const dvar_t *ui_playerProfileNameNew;
static char errorString[1024];
static char info[1024];
static qboolean bypassKeyClear;
static int numclean;
static char info_00ecf960[1024];
static char clientBuff[32];
extern int lastColumn;
static int lastTime;
static char info_00ecf960[1024];
static char info_00ecf960[1024];
static char info_00ecf960[1024];
static int numFound;
static int numTimeOuts;
extern char dlText[16];
extern char etaText[18];
extern char xferText[15];
extern int tleEstimates[80];
static int tleIndex;
static char g_mapname[64];
static char g_gametype[64];
static Bool g_ingameMenusLoaded;
extern const char *MonthAbbrev[12];
extern const char *netSources[3];
static const serverFilter_t serverFilters[1];
static char menuBuf2[32768];
static int ui_serverFilterType;

extern void Menus_CloseAll(uiInfo_t *info);
extern qboolean Menus_AnyFullScreenVisible(uiInfo_t *info);
extern void LAN_SaveServersToCache(void);
extern qboolean Menus_OpenByName(uiInfo_t *info, const char *name);
extern void Menus_CloseByName(uiInfo_t *info, const char *name);
extern const char *Cmd_Args(int startIndex);

static void UI_DrawCenteredText(const char *text, FontHandle font, float scale, float y, const vec_t *color, int style);
extern Bool IsTalking(void);

extern MaterialHandle CL_RegisterMaterialNoMip(const char *name, int flags);   /* was int -> truncated MaterialHandle on x64 */
extern FontHandle CL_RegisterFont(const char *name, int flags);   /* was int -> truncated FontHandle on x64 */
extern int Com_FindSoundAlias(const char *name);
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s0, float t0, float s1, float t1, const vec_t *color, MaterialHandle material);
extern float CL_NormalizedTextScale(FontHandle font, float scale);
extern int CL_TextWidth(const char *text, int maxChars, FontHandle font);
extern int CL_TextHeight(FontHandle font);
extern float GetRealHeightFromVirtualHeight(float scale);
extern void CalcScreenPlacement(float *x, float *y, float *xScale, float *yScale, int horzAlign, int vertAlign);
extern void CalcSplitScreenTextOffset(FontHandle font, float *y);
extern void CL_DrawTextPhysical(const char *text, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style);
extern void CL_DrawTextPhysicalWithCursor(const char *text, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style, int cursorPos, int cursor);
extern void GetClientState(void *cstate);
extern const char *CL_GetConfigString(int index);
extern const char *Info_ValueForKey(const char *s, const char *key);
extern int GetClientname(int index, char *name, int nameSize);
extern void I_strncpyz(char *dest, const char *src, int len);
extern char *I_CleanStr(char *str);
extern int I_stricmp(const char *s1, const char *s2);
extern int I_strnicmp(const char *s1, const char *s2, size_t n);
extern void I_strncat(char *dest, int len, const char *src);
extern int FS_FOpenFileByMode(const char *name, int *f, int mode);
extern int FS_Read(void *buf, int len, int f);
extern void FS_FCloseFile(fileHandle_t f);
extern void Com_Printf(const char *fmt, ...);
extern MenuList *UI_LoadMenu(const char *name, int imageTrack);
extern void UI_AddMenuList(uiInfo_t *info, MenuList *menuList);
extern void UI_MapLoadInfo(const char *csv);
extern void UI_FillRect(float x, float y, float w, float h, int horzAlign, int vertAlign, const vec_t *color);
extern void *Menus_FindByName(uiInfo_t *info, const char *name);
extern void Menu_Paint(uiInfo_t *info, void *menu, int full);
extern void Dvar_SetInt(const void *dvar, int value);
extern void Dvar_SetString(const void *dvar, const char *value);
extern void Dvar_SetIntByName(const char *name, int value);
extern void Dvar_SetBoolByName(const char *name, int value);
extern void Dvar_SetStringByName(const char *name, const char *value);
extern int Dvar_GetInt(const char *name);
extern Bool Dvar_GetBool(const char *name);
extern float Dvar_GetFloat(const char *name);
extern const char *Dvar_GetString(const char *name);
extern const char *Dvar_GetVariantString(const char *name);
extern void Dvar_SetFloatByName(const char *name, float value);
extern const dvar_t *Dvar_SetFromStringByNameFromSource(const char *name, const char *value, int source);
extern const void *Dvar_FindVar(const char *name);
extern int SEH_VerifyLanguageSelection(int lang);
extern const char *SEH_StringEd_GetString(const char *ref);
extern void Com_Error(int level, const char *fmt, ...);
extern int String_Parse(const char **args, char *buf, int bufSize);
extern int LAN_GetServerStatus(const char *addr, char *info, int infoSize);
extern int LAN_AddServer(int source, const char *name, const char *addr);
extern void LAN_GetServerInfo(int source, int index, char *info, int infoSize);
extern int LAN_GetServerCount(int source);
extern void LAN_ResetPings(int source);
extern int LAN_WaitServerResponse(int source);
extern int LAN_CompareServers(int source, int sortKey, int sortDir, int s1, int s2);
extern int LAN_ServerIsDirty(int source, int index);
extern int LAN_GetServerPing(int source, int index);
extern void LAN_MarkServerDirty(int source, int index, int dirty);
extern int LAN_UpdateDirtyPings(int source);
extern void LAN_GetServerAddressString(int source, int index, char *addr, int addrSize);
extern void LAN_RemoveServer(int source, const char *addr);
extern void LAN_LoadCachedServers(void);
extern int CIN_StopCinematic(int handle);
extern int CIN_PlayCinematic(const char *name, int x, int y, int w, int h, int flags);
extern int CIN_RunCinematic(int handle);
extern void CIN_SetExtents(int handle, int x, int y, int w, int h);
extern void CIN_DrawCinematic(int handle);
extern void UI_DrawHandlePic(float x, float y, float w, float h, int horzAlign, int vertAlign, const vec_t *color, MaterialHandle material);
extern void Key_SetCatcher(int catcher);
extern int Key_GetCatcher(void);
extern void Key_ClearStates(void);
extern int Menu_Count(uiInfo_t *info);
extern void *Menu_GetFocused(uiInfo_t *info);
extern void Menu_HandleKey(uiInfo_t *info, void *menu, int key, int down);
extern void Menu_PaintAll(uiInfo_t *info);
extern void Menu_SetFeederSelection(uiInfo_t *info, menuDef_t *menu, int feeder, int index, const char *name);
extern void Menu_Setup(uiInfo_t *info);
extern qboolean Display_MouseMove(uiInfo_t *info, void *capture, int x, int y);
extern int Display_KeyBindPending(void);
extern void SND_FadeAllSounds(float gain, int time);
extern int Sys_Milliseconds(void);
extern float Voice_GetVoiceLevel(void);
extern int Com_RealTime(qtime_t *q);
extern int FS_GetFileList(const char *path, const char *ext, int flags, char *buf, int bufSize);
extern const char *Com_Parse(const char **buf);
extern const char *String_Alloc(const char *s);
extern void String_Init(void);
extern void CL_GetScreenDimensions(int *w, int *h, int *aspect);
extern const void *Dvar_RegisterInt(const char *name, int value, int min, int max, int flags);
extern const void *Dvar_RegisterFloat(const char *name, float value, float min, float max, int flags);
extern const void *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern const void *Dvar_RegisterString_mac(const char *name, const char *value, int flags);
extern void UI_LoadArenas(void);
extern MenuList *UI_LoadMenus(const char *name, int imageTrack);
extern void UI_LoadSoundAliases(void);
extern void Controls_SetDefaults(void);
extern void Controls_GetConfig(void);
extern void Controls_SetConfig(int apply);
extern void CG_OwnerDraw(float x, float y, float w, float h, int horzAlign, int vertAlign, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, int font, float scale, vec_t *color, int material, int textStyle);
extern const char *CG_GameTypeString(void);
extern const char *CG_GetKillerText(void);
extern const char *SEH_LocalizeTextMessage(const char *msg, const char *fmt, const char *ctx);
extern void LerpColor(const vec_t *a, const vec_t *b, vec_t *c, float t);
extern void Item_SetCursorPos(void *item, int pos);
extern int Item_ListBox_MaxScroll(void *item);
extern void ListBox_SetStartPos(void *listPtr, int pos);
extern void ListBox_SetEndPos(void *listPtr, int pos);
extern void ListBox_SetCursorPos(void *listPtr, int pos);
extern int CL_IsPlayerTalking(int index);
extern int CL_IsPlayerMuted(int index);
extern void CL_MutePlayer(int index);
extern int Int_Parse(const char **args, int *out);
extern void CLUI_GetCDKey(char *buf, int bufSize, char *checksum, int checksumSize);
extern void CLUI_SetCDKey(char *key, char *checksum);
extern int CL_CDKeyValidate(const char *key1, const char *key2);
extern void CG_DrawInformation(int loading);
extern char **FS_ListFiles(const char *path, const char *ext, int flags, int *numFiles, int allocTrackType);
extern void FS_FreeFileList(char **list, int allocTrackType);
extern char *I_strupr(char *str);
extern void Com_ChangePlayerProfile(const char *profileName);
extern int Com_DeletePlayerProfile(const char *profileName);
extern int Com_NewPlayerProfile(const char *name);

extern void Cbuf_ExecuteText(int execWhen, const char *text);
extern int Com_sprintf(char *buf, int size, const char *fmt, ...);
extern int ___toupper(int c);
extern int ___tolower(int c);

void UI_AssetCache(void);
void UI_DrawSides(float x, float y, float w, float h, int horzAlign, int vertAlign, float size, const vec_t *color);
void UI_DrawTopBottom(float x, float y, float w, float h, int horzAlign, int vertAlign, float size, const vec_t *color);
int UI_TextWidth(const char *text, int maxChars, FontHandle font, float scale);
int UI_TextHeight(FontHandle font, float scale);
FontHandle UI_GetFontHandle(int fontEnum, float scale);
void UI_UpdateTime(int realtime);
void UI_Shutdown(void);
char *GetMenuBuffer(const char *filename);
qboolean Load_ScriptMenu(const char *pszMenu, int imageTrack);
static void UI_DrawMapPreview(const rectDef_t *rect, const vec_t *color, int net);
const char *UI_GetMapDisplayName(const char *pszMap);
const char *UI_GetMapDisplayNameFromPartialLoadNameMatch(const char *pszMap, int *mapLoadNameLen);
const char *UI_GetGameTypeDisplayName(const char *pszGameType);
static void UI_BuildPlayerList(void);
void UI_DrawMapLevelshot(void);
void UI_LoadIngameMenus(void);
void UI_SetMap(const char *mapname, const char *gametype);
qboolean UI_OwnerDrawVisible(int flags);
static int UI_ServersQsortCompare(const void *arg1, const void *arg2);
static int UI_PlayerProfilesQsortCompare(const void *arg1, const void *arg2);
static void UI_SelectCurrentMap(void);
qboolean UI_CheckExecKey(int key);
static void UI_VerifyLanguage(void);
const char *UI_SafeTranslateString(const char *reference);
static void UI_AddServerToFavoritesList(const char *name, const char *address);
static Bool UI_GetOpenOrCloseMenuOnDvarArgs(const char **args, const char *cmd,
                                            char *dvarName, char *testValue, char *menuName);
static int __attribute_regparm__(2) UI_GetServerStatusInfo(const char *serverAddress, serverStatusInfo_t *info);
void UI_FeederSelection(float feederID, int index);
static void UI_GetGameTypesList(void);
void UI_Pause(qboolean b);
void UI_OpenMenu_f(void);
void UI_CloseMenu_f(void);
void UI_Init(void);
void UI_KeyEvent(int key, qboolean down);
void UI_MouseEvent(int dx, int dy);
void UI_MouseEventAbsolute(int x, int y);
uiMenuCommand_t UI_GetActiveMenu(void);
qboolean UI_SetActiveMenu(int menu);
qboolean UI_IsFullscreen(void);
static void __attribute_regparm__(3) UI_ReadableSize(char *buf, int bufsize, int value);
float UI_GetBlurRadius(void);
qboolean UI_AnyFullScreenMenuVisible(void);
const char *UI_ReplaceConversions(const char *sourceString, ConversionArguments *arguments);
const char *UI_ReplaceConversionString(const char *sourceString, const char *replaceString);
void UI_CloseAll(void);
void UI_CloseFocusedMenu(void);
void UI_OverrideCursorPos(rectDef_t (*item)[16]);
Bool UI_DrawRecordLevel(rectDef_t *rect);
MaterialHandle UI_FeederItemImage(const float feederID, int index);
void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style);
void UI_DrawTextWithCursor(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style, int cursorPos, int cursor);
static void UI_StartServerRefresh(qboolean full);
static void __attribute_regparm__(0) UI_BuildServerDisplayList(qboolean force);
static inline __attribute__((always_inline)) void UI_UpdateServerCount(void);
qboolean UI_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key);
const char *UI_FeederItemText(float feederID, int index, int column, MaterialHandle *handle);
void UI_DrawRect(float x, float y, float width, float height, int horzAlign, int vertAlign, float size, const vec_t *color);
int UI_OwnerDrawWidth(int ownerDraw, FontHandle font, float scale);
void UI_OwnerDraw(float x, float y, float w, float h, int horzAlign, int vertAlign, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, FontHandle font, float scale, vec_t *color, MaterialHandle material, int textStyle);
static void BM_NOINLINE UI_BuildFindPlayerList(void);
static void UI_REGPARM1 UI_BuildServerStatus(int force);
void UI_Refresh(void);
void UI_RunMenuScript(const char **args);
int UI_FeederCount(float feederID);
static void UI_DisplayDownloadInfo(const char *downloadName, float centerPoint, float yStart, FontHandle font, float scale);
void UI_DrawConnectScreen(void);

void UI_AssetCache(void)
{
    sharedUiInfo.assets.whiteMaterial = CL_RegisterMaterialNoMip("white", 3);
    sharedUiInfo.assets.scrollBar = CL_RegisterMaterialNoMip("ui/assets/scrollbar.tga", 3);
    sharedUiInfo.assets.scrollBarArrowDown = CL_RegisterMaterialNoMip("ui/assets/scrollbar_arrow_dwn_a.tga", 3);
    sharedUiInfo.assets.scrollBarArrowUp = CL_RegisterMaterialNoMip("ui/assets/scrollbar_arrow_up_a.tga", 3);
    sharedUiInfo.assets.scrollBarArrowLeft = CL_RegisterMaterialNoMip("ui/assets/scrollbar_arrow_left.tga", 3);
    sharedUiInfo.assets.scrollBarArrowRight = CL_RegisterMaterialNoMip("ui/assets/scrollbar_arrow_right.tga", 3);
    sharedUiInfo.assets.scrollBarThumb = CL_RegisterMaterialNoMip("ui/assets/scrollbar_thumb.tga", 3);
    sharedUiInfo.assets.sliderBar = CL_RegisterMaterialNoMip("ui/assets/slider2.tga", 3);
    sharedUiInfo.assets.sliderThumb = CL_RegisterMaterialNoMip("ui/assets/sliderbutt_1", 3);
    sharedUiInfo.assets.cursor = CL_RegisterMaterialNoMip("$cursor", 0);
    sharedUiInfo.assets.bigFont = CL_RegisterFont("$bigfont", 0);
    sharedUiInfo.assets.smallFont = CL_RegisterFont("$smallfont", 0);
    sharedUiInfo.assets.consoleFont = CL_RegisterFont("$consolefont", 0);
    sharedUiInfo.assets.boldFont = CL_RegisterFont("$boldfont", 0);
    sharedUiInfo.assets.textFont = CL_RegisterFont("$font", 0);
    sharedUiInfo.assets.extraBigFont = CL_RegisterFont("$extrabigfont", 0);
    sharedUiInfo.assets.itemFocusSound = (snd_alias_list_t *)Com_FindSoundAlias("$itemfocussound");
}

void UI_DrawSides(float x, float y, float w, float h, int horzAlign, int vertAlign, float size, const vec_t *color)
{
    CL_DrawStretchPic(x, y, size, h, horzAlign, vertAlign, 0, 0, 0, 0, color, sharedUiInfo.assets.whiteMaterial);
    CL_DrawStretchPic(x + w - size, y, size, h, horzAlign, vertAlign, 0, 0, 0, 0, color, sharedUiInfo.assets.whiteMaterial);
}

void UI_DrawTopBottom(float x, float y, float w, float h, int horzAlign, int vertAlign, float size, const vec_t *color)
{
    CL_DrawStretchPic(x, y, w, size, horzAlign, vertAlign, 0, 0, 0, 0, color, sharedUiInfo.assets.whiteMaterial);
    CL_DrawStretchPic(x, y + h - size, w, size, horzAlign, vertAlign, 0, 0, 0, 0, color, sharedUiInfo.assets.whiteMaterial);
}

int UI_TextWidth(const char *text, int maxChars, FontHandle font, float scale)
{
    float actualScale = CL_NormalizedTextScale(font, scale);
    return (int)((float)CL_TextWidth(text, maxChars, font) * actualScale);
}

int UI_TextHeight(FontHandle font, float scale)
{
    float actualScale = CL_NormalizedTextScale(font, scale);
    return (int)((float)CL_TextHeight(font) * actualScale);
}

FontHandle UI_GetFontHandle(int fontEnum, float scale)
{
    float realHeight;
    if (fontEnum == 2)
        return sharedUiInfo.assets.bigFont;
    if (fontEnum == 3)
        return sharedUiInfo.assets.smallFont;
    if (fontEnum == 5)
        return sharedUiInfo.assets.consoleFont;

    realHeight = GetRealHeightFromVirtualHeight(scale);

    if (fontEnum == 4) {
        if (ui_smallFont->current.value >= realHeight)
            return sharedUiInfo.assets.smallFont;
        if (ui_bigFont->current.value >= realHeight)
            return sharedUiInfo.assets.boldFont;
        return sharedUiInfo.assets.textFont;
    }

    if (ui_smallFont->current.value >= realHeight)
        return sharedUiInfo.assets.smallFont;
    if (realHeight >= ui_extraBigFont->current.value)
        return sharedUiInfo.assets.extraBigFont;
    if (realHeight >= ui_bigFont->current.value)
        return sharedUiInfo.assets.bigFont;
    return sharedUiInfo.assets.textFont;
}

void UI_UpdateTime(int realtime)
{
    int *ui = (int *)uiInfo;
    int slot, sum, i;

    ui[2] = realtime - ui[1];

    ui[1] = realtime;

    slot = uiInfo->timeIndex % 4;

    uiInfo->previousTimes[slot] = ui[2];

    uiInfo->timeIndex += 1;

    if (uiInfo->timeIndex <= 4)
        return;

    sum = 0;
    for (i = 0; i < 4; i++) {
        sum += uiInfo->previousTimes[i];
    }
    uiInfo->uiDC.FPS = (float)(sum == 0 ? 4000 : 4000 / sum);
}

void UI_Shutdown(void)
{
    Menus_CloseAll(uiInfo);
    sharedUiInfo.assets.whiteMaterial = 0;
    LAN_SaveServersToCache();
}

char *GetMenuBuffer(const char *filename)
{
    int f;
    int len;

    len = FS_FOpenFileByMode(filename, &f, 0);
    if (!f) {
        Com_Printf("^1menu file not found: %s, using default\n", filename);
        return 0;
    }
    if (len > 0x7fff) {
        Com_Printf("^1menu file too large: %s is %i, max allowed is %i", filename, len, 0x8000);
        FS_FCloseFile(f);
        return 0;
    }
    FS_Read(menuBuf2, len, f);
    menuBuf2[len] = 0;
    FS_FCloseFile(f);
    return menuBuf2;
}

qboolean Load_ScriptMenu(const char *pszMenu, int imageTrack)
{
    char szMenuFile[256];
    MenuList *menuList;

    strcpy(szMenuFile, "ui_mp/scriptmenus/");

    I_strncat(szMenuFile, 0x100, pszMenu);
    I_strncat(szMenuFile, 0x100, ".menu");

    menuList = UI_LoadMenu(szMenuFile, imageTrack);
    if (!menuList)
        return 0;

    UI_AddMenuList(uiInfo, menuList);
    return 1;
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
static void UI_DrawMapPreview(const rectDef_t *rect, const vec_t *color, int net)
{
    int map;
    int mapCount;
    int material;

    if (net) {
        map = (ui_currentNetMap)->current.integer;
    } else {
        map = (ui_currentMap)->current.integer;
    }

    mapCount = sharedUiInfo.mapCount;
    if (map < 0 || map >= mapCount) {

        if (net) {
            Dvar_SetInt(ui_currentNetMap, 0);
        } else {
            Dvar_SetInt(ui_currentMap, 0);
        }
        map = 0;
    }

    material = *(int *)((char *)&sharedUiInfo + 5104 + map * 164);

    if (!material) {
        material = CL_RegisterMaterialNoMip("menu/art/unknownmap", 3);
    }

    UI_DrawHandlePic(rect->x, rect->y, rect->w, rect->h,
                     rect->horzAlign, rect->vertAlign,
                     color, material);
}
#endif

const char *UI_GetMapDisplayName(const char *pszMap)
{
    int i;
    for (i = 0; i < sharedUiInfo.mapCount; i++) {
        const char *loadName = sharedUiInfo.mapList[i].mapLoadName;
        if (I_stricmp(pszMap, loadName) == 0) {
            return sharedUiInfo.mapList[i].mapName;
        }
    }
    return pszMap;
}

const char *UI_GetMapDisplayNameFromPartialLoadNameMatch(const char *pszMap, int *mapLoadNameLen)
{
    int i;
    for (i = 0; i < sharedUiInfo.mapCount; i++) {
        int len = strlen(sharedUiInfo.mapList[i].mapLoadName);
        *mapLoadNameLen = len;
        if (I_strnicmp(pszMap, sharedUiInfo.mapList[i].mapLoadName, len) == 0) {
            return sharedUiInfo.mapList[i].mapName;
        }
    }
    return 0;
}

const char *UI_GetGameTypeDisplayName(const char *pszGameType)
{
    int i;
    for (i = 0; i < sharedUiInfo.numGameTypes; i++) {
        const char *name = sharedUiInfo.gameTypes[i].gameType;
        if (I_stricmp(pszGameType, name) == 0) {
            return sharedUiInfo.gameTypes[i].gameTypeName;
        }
    }
    return pszGameType;
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
void UI_BuildPlayerList(void)
{
    char cs[0x844];
    char name[0x20];
    int count;
    int n;

    GetClientState(cs);
    count = atoi(Info_ValueForKey(CL_GetConfigString(0), "sv_maxclients"));
    memset(sharedUiInfo.playerClientNums, -1, 0x100);
    sharedUiInfo.playerCount = 0;

    for (n = 0; n < count; n++) {
        int playerCount;
        char *playerName;

        if (!GetClientname(n, name, 0x20))
            continue;

        playerCount = sharedUiInfo.playerCount;
        playerName = sharedUiInfo.playerNames[playerCount];
        sharedUiInfo.playerClientNums[playerCount] = n;
        I_strncpyz(playerName, name, 0x20);
        I_CleanStr(playerName);
        sharedUiInfo.playerCount = playerCount + 1;
    }
}
#endif

void UI_DrawMapLevelshot(void)
{
    if (g_mapname[0]) {
        void *menu = Menus_FindByName(uiInfo, "connect");
        if (menu) {
            uiInfo->uiDC.blurRadiusOut = 0;
            Menu_Paint(uiInfo, menu, 1);
            return;
        }
    }

    UI_FillRect(0, 0, 640.0f, 480.0f, 0, 0, (const vec_t *)imp_colorBlack);
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
void UI_LoadIngameMenus(void)
{
    MenuList *menuList;

    if (g_ingameMenusLoaded)
        return;

    g_ingameMenusLoaded = 1;

    menuList = UI_LoadMenus("ui_mp/ingame.txt", 3);
    UI_AddMenuList(uiInfo, menuList);
}
#endif

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
void UI_SetMap(const char *mapname, const char *gametype)
{
    I_strncpyz(g_mapname, mapname, 0x40);
    I_strncpyz(g_gametype, gametype, 0x40);
    if (g_mapname[0]) {
        UI_MapLoadInfo(va("maps/mp/%s.csv", g_mapname));
    }
}
#endif

qboolean UI_OwnerDrawVisible(int flags)
{
    qboolean visible = 1;

    if (flags & 4) {
        if (ui_netSource->current.integer != 2) {
            visible = 0;
        }
    }

    if (flags & 0x1000) {
        if (ui_netSource->current.integer == 2) {
            visible = 0;
        }
    }

    return visible;
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
static int UI_ServersQsortCompare(const void *arg1, const void *arg2)
{
    int s1 = *(const int *)arg1;
    int s2 = *(const int *)arg2;
    return LAN_CompareServers(
        ui_netSource->current.integer,
        sharedUiInfo.serverStatus.sortKey,
        sharedUiInfo.serverStatus.sortDir,
        s1, s2);
}
#endif

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
static int UI_PlayerProfilesQsortCompare(const void *arg1, const void *arg2)
{
    int idx1 = *(const int *)arg1;
    int idx2 = *(const int *)arg2;
    int result;

    if (idx1 == idx2)
        return 0;

    result = I_stricmp(
        uiInfo->playerProfileName[idx1],
        uiInfo->playerProfileName[idx2]);

    if (uiInfo->playerProfileStatus.sortDir == 0)
        result = -result;

    return result;
}
#endif

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
static void UI_SelectCurrentMap(void)
{
    byte cstate[0x844];
    char szMap[0x40];
    int i;
    int visibleIndex;

    GetClientState(cstate);
    if (*(int *)cstate != 8)
        return;

    {
        const char *serverInfo = CL_GetConfigString(0);
        if (!serverInfo[0])
            return;
        I_strncpyz(szMap, Info_ValueForKey(serverInfo, "mapname"), sizeof(szMap));
    }

    if (sharedUiInfo.mapCount <= 0)
        return;

    visibleIndex = 0;
    for (i = 0; i < sharedUiInfo.mapCount; i++) {
        byte *entry = (byte *)&sharedUiInfo + i * 0xa4;

        if (!*(int *)(entry + 0x13f4))
            continue;

        if (I_stricmp(szMap, *(const char **)(entry + 0x1354)) == 0) {
            Menu_SetFeederSelection(uiInfo, 0, 4, visibleIndex, "createserver_maps");
            return;
        }

        visibleIndex++;
    }
}
#endif

qboolean UI_CheckExecKey(int key)
{
    menuDef_t *menu;
    int *node;

    menu = (menuDef_t *)Menu_GetFocused(uiInfo);

    if (*(int *)imp_g_editingField)
        return 1;

    if (key > 0x100)
        return 0;
    if (!menu)
        return 0;

    if ((key >= 0xc8 && key <= 0xce) ||
        key == 0x0d || key == 0xbf ||
        key == 0x09 ||
        (key >= 0x9a && key <= 0x9d) ||
        key == 0xa3 || key == 0xa4) {
        return 1;
    }

    node = (int *)menu->onKey;
    while (node) {
        if (*node == key)
            return 1;
        node = *(int **)((byte *)node + 8);
    }
    return 0;
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
static void UI_VerifyLanguage(void)
{
    int oldLanguage = Dvar_GetInt("loc_language");
    int newLanguage = Dvar_GetInt("ui_language");
    int verified = SEH_VerifyLanguageSelection(newLanguage);

    if (verified != newLanguage)
        Dvar_SetIntByName("ui_language", verified);

    if (oldLanguage != newLanguage)
        Dvar_SetBoolByName("ui_languagechanged", 1);
    else
        Dvar_SetBoolByName("ui_languagechanged", 0);
}
#endif

extern void *imp_loc_warnings;
extern void *imp_loc_warningsAsErrors;
#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
const char *UI_SafeTranslateString(const char *reference)
{
    const char *ref = reference;

    if (ref[0] == 0x15)
        ref++;

    const char *translated = SEH_StringEd_GetString(ref);
    if (translated)
        return translated;

    if ((loc_warnings)->current.enabled) {
        if ((loc_warningsAsErrors)->current.enabled) {
            Com_Error(6, "Could not translate string \"%s\"", ref);
        } else {
            Com_Printf("^3WARNING: Could not translate string \"%s\"\n", ref);
        }

        strcpy(errorString, "^1UNLOCALIZED(^7");
        I_strncat(errorString, 0x400, ref);
        I_strncat(errorString, 0x400, "^1)^7");
        return errorString;
    }

    strcpy(errorString, ref);
    return errorString;
}
#endif

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
static void UI_AddServerToFavoritesList(const char *name, const char *address)
{
    const char *message;
    const char *dvarMessage;
    int addResult;

    if (!name[0]) {
        message = UI_SafeTranslateString("EXE_FAVORITENAMEEMPTY");
        Com_Printf("%s\n", message);
        Dvar_SetStringByName("ui_favorite_message", "@EXE_FAVORITENAMEEMPTY");
        return;
    }

    if (!address[0]) {
        message = UI_SafeTranslateString("EXE_FAVORITEADDRESSEMPTY");
        Com_Printf("%s\n", message);
        Dvar_SetStringByName("ui_favorite_message", "@EXE_FAVORITEADDRESSEMPTY");
        return;
    }

    addResult = LAN_AddServer(2, name, address);
    if (addResult == 0) {
        message = UI_SafeTranslateString("EXE_FAVORITEINLIST");
        dvarMessage = "@EXE_FAVORITEINLIST";
    } else if (addResult == -1) {
        message = UI_SafeTranslateString("EXE_FAVORITELISTFULL");
        dvarMessage = "@EXE_FAVORITELISTFULL";
    } else if (addResult == -2) {
        message = UI_SafeTranslateString("EXE_BADSERVERADDRESS");
        dvarMessage = "@EXE_BADSERVERADDRESS";
    } else {
        message = UI_SafeTranslateString("EXE_FAVORITEADDED");
        dvarMessage = "@EXE_FAVORITEADDED";
    }

    Com_Printf("%s\n", message);
    Dvar_SetStringByName("ui_favorite_message", dvarMessage);
}
#endif

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
Bool UI_GetOpenOrCloseMenuOnDvarArgs(const char **args, const char *cmd,
                                     char *dvarName, char *testValue, char *menuName)
{
    if (!String_Parse(args, dvarName, 0x400)) {
        Com_Printf("%s: invalid dvar name.\n", cmd);
        return 0;
    }

    if (!String_Parse(args, testValue, 0x400)) {
        Com_Printf("%s: invalid test value.\n", cmd);
        return 0;
    }

    if (!String_Parse(args, menuName, 0x400)) {
        Com_Printf("%s: invalid menu name.\n", cmd);
        return 0;
    }

    return 1;
}
#endif

static void UI_AddServerStatusDvarRows(serverStatusInfo_t *info)
{
    int index = 0;
    int dvarIndex;

    for (dvarIndex = 0; serverStatusDvars[dvarIndex].name; dvarIndex++) {
        int j;

        for (j = 0; j < info->numLines; j++) {
            const char *oldName;
            const char *oldValue;
            const serverStatusDvar_t *dvar;

            if (!info->lines[j][1] || info->lines[j][1][0] != '\0')
                continue;

            dvar = &serverStatusDvars[dvarIndex];
            if (I_stricmp(dvar->name, info->lines[j][0]) != 0)
                continue;

            oldName = info->lines[index][0];
            oldValue = info->lines[index][3];
            info->lines[index][0] = info->lines[j][0];
            info->lines[index][3] = info->lines[j][3];
            info->lines[j][0] = oldName;
            info->lines[j][3] = oldValue;

            if (dvar->altName[0])
                info->lines[index][0] = dvar->altName;

            if (dvar->type == 1)
                info->lines[index][3] = atoi(info->lines[index][3]) ? "@EXE_YES" : "@EXE_NO";

            index++;
            break;
        }
    }
}

static int __attribute_regparm__(2) UI_GetServerStatusInfo(const char *serverAddress, serverStatusInfo_t *info)
{
    char *p;
    int currentLine;

    if (!info) {
        LAN_GetServerStatus(serverAddress, 0, 0);
        return 0;
    }

    memset(info, 0, sizeof(*info));

    p = info->text;
    if (!LAN_GetServerStatus(serverAddress, p, sizeof(info->text)))
        return 0;

    I_strncpyz(info->address, serverAddress, sizeof(info->address));
    info->lines[0][0] = "address";
    info->lines[0][1] = "";
    info->lines[0][2] = "";
    info->lines[0][3] = info->address;
    info->numLines = 1;

    if (p && p[0]) {
        for (;;) {
            char *key;
            char *value;
            char *slash;
            int row;

            p = strchr(p, '\\');
            if (!p)
                break;

            *p++ = '\0';
            if (*p == '\\')
                break;

            key = p;
            slash = strchr(key, '\\');
            if (!slash)
                break;

            *slash++ = '\0';
            value = slash;

            row = info->numLines;
            info->lines[row][0] = key;
            info->lines[row][1] = "";
            info->lines[row][2] = "";
            info->lines[row][3] = value;
            info->numLines = row + 1;

            if (info->numLines > 0x7f)
                break;

            if (!value || !value[0])
                break;

            p = value;
        }
    }

    currentLine = info->numLines;
    if (currentLine <= 0x7c) {
        int headerLine = currentLine + 1;
        int playerIndex = 0;
        int pingsLen = 0;

        info->lines[currentLine][0] = "";
        info->lines[currentLine][1] = "";
        info->lines[currentLine][2] = "";
        info->lines[currentLine][3] = "";

        info->lines[headerLine][0] = "@EXE_SV_INFO_NUM";
        info->lines[headerLine][1] = "@EXE_SV_INFO_SCORE";
        info->lines[headerLine][2] = "@EXE_SV_INFO_PING";
        info->lines[headerLine][3] = "@EXE_SV_INFO_NAME";
        info->numLines = currentLine + 2;

        while (p && p[0]) {
            char *score;
            char *ping;
            char *name;
            char *space;
            char *slash;
            char *numText;
            int row;

            if (*p == '\\')
                *p++ = '\0';

            score = p;
            space = strchr(score, ' ');
            if (!space)
                break;

            *space++ = '\0';
            ping = space;

            space = strchr(ping, ' ');
            if (!space)
                break;

            *space++ = '\0';
            name = space;

            row = info->numLines;
            numText = info->pings + pingsLen;
            Com_sprintf(numText, sizeof(info->pings) - pingsLen, "%d", playerIndex);
            info->lines[row][0] = numText;
            pingsLen += strlen(numText) + 1;

            info->lines[row][1] = score;
            info->lines[row][2] = ping;
            info->lines[row][3] = name;
            info->numLines = row + 1;

            if (info->numLines > 0x7f)
                break;

            slash = strchr(name, '\\');
            if (!slash)
                break;

            *slash = '\0';
            playerIndex++;
            p = slash + 1;
        }
    }

    UI_AddServerStatusDvarRows(info);
    return 1;
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
void UI_FeederSelection(float feederID, int index)
{
    if (feederID == 4.0f) {

        int mapVal = (ui_currentNetMap)->current.integer;
        int offset = mapVal * 164;
        int cinHandle = *(int *)((byte *)&sharedUiInfo + 4972 + offset);
        int numMaps, visCount, actual, i;

        if (cinHandle >= 0) {
            CIN_StopCinematic(cinHandle);
            *(int *)((byte *)&sharedUiInfo + 4960 + 12 + offset) = -1;
        }

        numMaps = sharedUiInfo.mapCount;
        visCount = 0;
        actual = 0;
        for (i = 0; i < numMaps; i++) {
            if (sharedUiInfo.mapList[i].active != 0) {
                if (visCount == index) {
                    actual = i;
                    goto map_found;
                }
                visCount++;
            }
        }
        actual = 0;
    map_found:
        Dvar_SetInt(ui_currentMap, actual);
        Dvar_SetInt(ui_currentNetMap, actual);

    } else if (feederID == 2.0f) {

        char *name;
        char *p;

        if (sharedUiInfo.serverStatus.numDisplayServers >= 1)
            sharedUiInfo.serverStatus.currentServer = index;

        LAN_GetServerInfo((ui_netSource)->current.integer,
                          sharedUiInfo.serverStatus.displayServers[index],
                          info, 0x400);
        name = (char *)va("levelshots/%s", Info_ValueForKey(info, "mapname"));

        for (p = name; *p; p++)
            *p = ___tolower((int)(signed char)*p);

        sharedUiInfo.serverStatus.currentServerPreview = CL_RegisterMaterialNoMip(name, 3);
        if (sharedUiInfo.serverStatus.currentServerCinematic >= 0) {
            CIN_StopCinematic(sharedUiInfo.serverStatus.currentServerCinematic);
            sharedUiInfo.serverStatus.currentServerCinematic = -1;
        }

    } else if (feederID == 7.0f || feederID == 20.0f) {

        uiInfo->playerIndex = index;

    } else if (feederID == 9.0f) {
        sharedUiInfo.modIndex = index;

    } else if (feederID == 24.0f) {

        if (index >= 0 && index < uiInfo->playerProfileCount) {
            int sortedIdx = uiInfo->playerProfileStatus.displayProfile[index];
            const char *profileName = uiInfo->playerProfileName[sortedIdx];
            Dvar_SetString(ui_playerProfileSelected, profileName);
        }
    }
}
#endif

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
static void UI_GetGameTypesList(void)
{
    char szGameTypeList[0x1000];
    int iNumGameTypeScripts;
    int i;
    char *pszFileName;
    const char *pBuffParse;

    sharedUiInfo.numGameTypes = 0;
    sharedUiInfo.numJoinGameTypes = 0;

    sharedUiInfo.joinGameTypes[0].gameType = String_Alloc("All");
    {
        int idx = sharedUiInfo.numJoinGameTypes;
        sharedUiInfo.joinGameTypes[idx].gameTypeName = "";
    }
    sharedUiInfo.numJoinGameTypes += 1;

    iNumGameTypeScripts = FS_GetFileList("maps/mp/gametypes", "gsc", 0, szGameTypeList, 0x1000);

    if (iNumGameTypeScripts > 0) {
        pszFileName = szGameTypeList;

        for (i = 0; i < iNumGameTypeScripts; i++) {
            int iLen = strlen(pszFileName);
            char *pszEnd;
            int numTypes;

            if (pszFileName[0] == '_') {
                pszFileName += iLen + 1;
                continue;
            }

            pszEnd = pszFileName + iLen;
            if (I_stricmp(pszEnd - 4, ".gsc") == 0) {
                pszFileName[iLen - 4] = '\0';
            }

            numTypes = sharedUiInfo.numGameTypes;
            if (numTypes == 32 || sharedUiInfo.numJoinGameTypes == 32) {
                Com_Printf("Too many game type scripts found! Only loading the first %i\n", 31);
                break;
            }

            sharedUiInfo.gameTypes[numTypes].gameType = String_Alloc(pszFileName);

            {
                int gt = sharedUiInfo.numGameTypes;
                const char *scriptName = sharedUiInfo.gameTypes[gt].gameType;
                int nameIdx = sharedUiInfo.numJoinGameTypes;
                sharedUiInfo.joinGameTypes[nameIdx].gameType = scriptName;
            }

            pBuffParse = GetMenuBuffer(va("maps/mp/gametypes/%s.txt", pszFileName));
            if (pBuffParse) {
                const char *parsed = Com_Parse(&pBuffParse);
                int gt2 = sharedUiInfo.numGameTypes;
                sharedUiInfo.gameTypes[gt2].gameTypeName = String_Alloc(parsed);
                {
                    const char *displayName = sharedUiInfo.gameTypes[sharedUiInfo.numGameTypes].gameTypeName;
                    int nameIdx2 = sharedUiInfo.numJoinGameTypes;
                    sharedUiInfo.joinGameTypes[nameIdx2].gameTypeName = displayName;
                }
            } else {

                int gt3 = sharedUiInfo.numGameTypes;
                *(const char **)((byte *)&sharedUiInfo + 4432 + gt3 * 8) = *(const char **)((byte *)&sharedUiInfo + 4428 + gt3 * 8);
                {
                    const char *displayName2 = sharedUiInfo.gameTypes[sharedUiInfo.numGameTypes].gameTypeName;
                    int nameIdx3 = sharedUiInfo.numJoinGameTypes;
                    sharedUiInfo.joinGameTypes[nameIdx3].gameTypeName = displayName2;
                }
            }

            sharedUiInfo.numGameTypes += 1;
            sharedUiInfo.numJoinGameTypes += 1;
            pszFileName = pszEnd + 1;
        }
    }

    if (sharedUiInfo.numGameTypes == 0) {
        Com_Error(0, "\x15No game type scripts found in maps/mp/gametypes folder");
    }
}
#endif

void UI_Pause(qboolean b)
{
    if (b) {
        Dvar_SetIntByName("cl_paused", 1);
        Key_SetCatcher(8);
    } else {
        Key_SetCatcher(Key_GetCatcher() & ~8);
        Key_ClearStates();
        Dvar_SetIntByName("cl_paused", 0);
    }
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
void UI_OpenMenu_f(void)
{
    Menus_OpenByName(uiInfo, Cmd_Args(1));
}
#endif

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
void UI_CloseMenu_f(void)
{
    Menus_CloseByName(uiInfo, Cmd_Args(1));
}
#endif

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
void UI_Init(void)
{
    byte *legacyBase;
    int width, height;
    MenuList *menuList;
    int netGameTypeIdx;
    float mPitch;

    *(int *)uiInfo = 0;
    g_ingameMenusLoaded = 0;

    UI_LoadSoundAliases();

    Dvar_RegisterBool_mac("g_allowvote", 1, 0x1001);
    Dvar_RegisterBool_mac("cg_brass", 1, 0x1001);
    Dvar_RegisterBool_mac("cg_marks", 1, 0x1001);

    Dvar_RegisterString_mac("server1", "", 0x1001);
    Dvar_RegisterString_mac("server2", "", 0x1001);
    Dvar_RegisterString_mac("server3", "", 0x1001);
    Dvar_RegisterString_mac("server4", "", 0x1001);
    Dvar_RegisterString_mac("server5", "", 0x1001);
    Dvar_RegisterString_mac("server6", "", 0x1001);
    Dvar_RegisterString_mac("server7", "", 0x1001);
    Dvar_RegisterString_mac("server8", "", 0x1001);
    Dvar_RegisterString_mac("server9", "", 0x1001);
    Dvar_RegisterString_mac("server10", "", 0x1001);
    Dvar_RegisterString_mac("server11", "", 0x1001);
    Dvar_RegisterString_mac("server12", "", 0x1001);
    Dvar_RegisterString_mac("server13", "", 0x1001);
    Dvar_RegisterString_mac("server14", "", 0x1001);
    Dvar_RegisterString_mac("server15", "", 0x1001);
    Dvar_RegisterString_mac("server16", "", 0x1001);

    ui_netSource = Dvar_RegisterInt("ui_netSource", 0, 0, 2, 0x1001);
    ui_smallFont = Dvar_RegisterFloat("ui_smallFont", 0.25f, 0.0f, 1.0f, 0x1001);
    ui_bigFont = Dvar_RegisterFloat("ui_bigFont", 0.4f, 0.0f, 1.0f, 0x1001);
    ui_extraBigFont = Dvar_RegisterFloat("ui_extraBigFont", 0.55f, 0.0f, 1.0f, 0x1001);
    ui_currentMap = Dvar_RegisterInt("ui_currentMap", 0, 0, 0x7fffffff, 0x1001);
    ui_gametype = Dvar_RegisterInt("ui_gametype", 3, 0, 0x7fffffff, 0x1001);
    ui_joinGameType = Dvar_RegisterInt("ui_joinGametype", 0, 0, 0x7fffffff, 0x1001);
    ui_netGameTypeName = Dvar_RegisterString_mac("ui_netGametypeName", "", 0x1001);
    ui_dedicated = Dvar_RegisterInt("ui_dedicated", 0, 0, 2, 0x1001);
    ui_currentNetMap = Dvar_RegisterInt("ui_currentNetMap", 0, 0, 0x7fffffff, 0x1001);
    ui_browserShowFull = Dvar_RegisterBool_mac("ui_browserShowFull", 1, 0x1001);
    ui_browserShowEmpty = Dvar_RegisterBool_mac("ui_browserShowEmpty", 1, 0x1001);
    ui_browserShowPassword = Dvar_RegisterBool_mac("ui_browserShowPassword", 1, 0x1001);
    ui_browserShowNoPassword = Dvar_RegisterBool_mac("ui_browserShowNoPassword", 1, 0x1001);
    ui_browserShowPure = Dvar_RegisterBool_mac("ui_browserShowPure", 1, 0x1001);
    ui_browserShowDedicated = Dvar_RegisterBool_mac("ui_browserShowDedicated", 0, 0x1001);
#    if COD2_IS_PATCH_13
    ui_browserShowPunkBuster = Dvar_RegisterInt("ui_browserShowPunkBuster", -1, (int)0x80000000, 0x7fffffff, 0x1001);
#    endif
    ui_browserMod = Dvar_RegisterInt("ui_browserMod", -1, -1, 0x7fffffff, 0x1001);
    ui_browserFriendlyfire = Dvar_RegisterInt("ui_browserFriendlyfire", -1, (int)0x80000000, 0x7fffffff, 0x1001);
    ui_browserKillcam = Dvar_RegisterInt("ui_browserKillcam", -1, (int)0x80000000, 0x7fffffff, 0x1001);
    ui_serverStatusTimeOut = Dvar_RegisterInt("ui_serverStatusTimeOut", 7000, 0, 0x7fffffff, 0x1001);
    ui_playerProfileCount = Dvar_RegisterInt("ui_playerProfileCount", 0, (int)0x80000000, 0x7fffffff, 0x1040);
    ui_playerProfileSelected = Dvar_RegisterString_mac("ui_playerProfileSelected", "", 0x1040);
    ui_playerProfileNameNew = Dvar_RegisterString_mac("ui_playerProfileNameNew", "", 0x1000);

    legacyBase = (byte *)imp_legacyHacksArray;
    (*(byte *)&((LegacyHacks *)legacyBase)->ui_newScriptMenu[0]) = 0;
    ((LegacyHacks *)legacyBase)->ui_newScriptMenuIndex = -1;
    (*(byte *)&((LegacyHacks *)legacyBase)->ui_scriptMenu[0]) = 0;
    ((LegacyHacks *)legacyBase)->ui_scriptMenuIndex = -1;
    ((LegacyHacks *)legacyBase)->ui_scriptMenuAllowResponse = 1;
    (*(byte *)&((LegacyHacks *)legacyBase)->ui_waitingScriptMenu[0]) = 0;
    ((LegacyHacks *)legacyBase)->ui_waitingScriptMenuIndex = -1;
    ((LegacyHacks *)legacyBase)->ui_waitingScriptMenuNoMouse = 0;

    String_Init();
    Menu_Setup(uiInfo);

    CL_GetScreenDimensions(&uiInfo->uiDC.screenWidth, &uiInfo->uiDC.screenHeight, (int *)&uiInfo->uiDC.screenAspect);

    width = uiInfo->uiDC.screenWidth;
    height = uiInfo->uiDC.screenHeight;

    if (width * 480 > height * 640) {

        uiInfo->uiDC.bias = ((float)width + (float)height * -1.3333333730697632f) * 0.5f;
    } else {
        *(int *)uiInfo = 0;
    }

    Sys_Milliseconds();
    UI_GetGameTypesList();

    ui_netGameType = Dvar_RegisterInt("ui_netGametype", 0, 0, sharedUiInfo.numGameTypes - 1, 0x1001);

    UI_LoadArenas();

    menuList = UI_LoadMenus("ui_mp/menus.txt", 3);
    UI_AddMenuList(uiInfo, menuList);
    UI_LoadIngameMenus();

    if (g_mapname[0] != '\0') {
        UI_MapLoadInfo(va("maps/mp/%s.csv", g_mapname));
    }

    UI_AssetCache();
    Menus_CloseAll(uiInfo);

    sharedUiInfo.serverHardwareIconList[0] = CL_RegisterMaterialNoMip("server_hardware_unknown", 3);
    sharedUiInfo.serverHardwareIconList[1] = CL_RegisterMaterialNoMip("server_hardware_linux_dedicated", 3);
    sharedUiInfo.serverHardwareIconList[2] = CL_RegisterMaterialNoMip("server_hardware_win_dedicated", 3);
    sharedUiInfo.serverHardwareIconList[3] = CL_RegisterMaterialNoMip("server_hardware_mac_dedicated", 3);
    sharedUiInfo.serverHardwareIconList[5] = CL_RegisterMaterialNoMip("server_hardware_win_listen", 3);
    sharedUiInfo.serverHardwareIconList[6] = CL_RegisterMaterialNoMip("server_hardware_mac_listen", 3);

    LAN_LoadCachedServers();

    if (sharedUiInfo.serverStatus.sortKey != 9) {
        sharedUiInfo.serverStatus.sortKey = 9;
        qsort(sharedUiInfo.serverStatus.displayServers, sharedUiInfo.serverStatus.numDisplayServers, 4, UI_ServersQsortCompare);
    }

    mPitch = Dvar_GetFloat("m_pitch");
    Dvar_SetBoolByName("ui_mousePitch", mPitch < 0.0f ? 1 : 0);

    sharedUiInfo.serverStatus.currentServerCinematic = -1;
    sharedUiInfo.previewMovie = -1;

    netGameTypeIdx = (ui_netGameType)->current.integer;
    Dvar_SetString(ui_netGameTypeName, sharedUiInfo.gameTypes[netGameTypeIdx].gameType);

    Dvar_RegisterBool_mac("ui_multiplayer", 1, 0x1040);

#    if COD2_FEATURE_GAMEPAD

    UI_ConsoleMode_Init(uiInfo);
    UI_Gamepad_RegisterMenus(uiInfo);
#    endif
}
#endif

static inline __attribute__((always_inline)) void UI_KeyEvent_impl(int key, qboolean down)
{
    menuDef_t *menu;

    if (Menu_Count(uiInfo) <= 0)
        return;

    menu = (menuDef_t *)Menu_GetFocused(uiInfo);
    if (menu) {
        if (Dvar_GetBool("cl_bypassMouseInput"))
            bypassKeyClear = 1;

        if (key == 0x1b && down && !Menus_AnyFullScreenVisible(uiInfo) && menu->onESC == 0) {

            Menus_CloseAll(uiInfo);
        } else {
            Menu_HandleKey(uiInfo, menu, key, down);
        }

        if (Menu_GetFocused(uiInfo))
            return;
    }

    if (!(Key_GetCatcher() & 8))
        return;

    Key_SetCatcher(Key_GetCatcher() & ~8);
    if (!bypassKeyClear)
        Key_ClearStates();
    bypassKeyClear = 0;
    Dvar_SetIntByName("cl_paused", 0);
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
void UI_KeyEvent(int key, qboolean down)
{
    UI_KeyEvent_impl(key, down);
}
#endif

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
void UI_MouseEvent(int dx, int dy)
{
    int *cursorX = &uiInfo->uiDC.cursorx;
    int *cursorY = &uiInfo->uiDC.cursory;

    *cursorX += dx;
    if (*cursorX < 0)
        *cursorX = 0;
    else if (*cursorX > 640)
        *cursorX = 640;

    *cursorY += dy;
    if (*cursorY < 0)
        *cursorY = 0;
    else if (*cursorY > 480)
        *cursorY = 480;

    if (Menu_Count(uiInfo) > 0) {
        Display_MouseMove(uiInfo, 0, *cursorX, *cursorY);
    }
}
#endif

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
void UI_MouseEventAbsolute(int x, int y)
{
    int *cursorX = &uiInfo->uiDC.cursorx;
    int *cursorY = &uiInfo->uiDC.cursory;

    if (x < 0)
        x = 0;
    else if (x > 640)
        x = 640;

    if (y < 0)
        y = 0;
    else if (y > 480)
        y = 480;

    *cursorX = x;
    *cursorY = y;

    if (Menu_Count(uiInfo) > 0)
        Display_MouseMove(uiInfo, 0, *cursorX, *cursorY);
}
#endif

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
uiMenuCommand_t UI_GetActiveMenu(void)
{
    return uiInfo->currentMenuType;
}
#endif

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
qboolean UI_SetActiveMenu(int menu)
{
    byte *legacyBase;
    menuDef_t *pFocus;
    const char *errorMsg;

    if (Menu_Count(uiInfo) <= 0)
        return 0;

    if (menu != 9 && menu != 10)
        uiInfo->currentMenuType = menu;

    if ((unsigned)menu > 11)
        return 0;

    switch (menu) {
    case 0:
        Key_SetCatcher(Key_GetCatcher() & ~8);
        Dvar_SetIntByName("cl_paused", 0);
        Menus_CloseAll(uiInfo);
        return 1;

    case 1:
        Key_SetCatcher(8);
        Menus_OpenByName(uiInfo, "main");
        errorMsg = Dvar_GetString("com_errorMessage");
        if (errorMsg[0] != '\0' && I_stricmp(errorMsg, ";") != 0) {
            Menus_OpenByName(uiInfo, "error_popmenu");
        }
        SND_FadeAllSounds(1.0f, 1000);
        return 1;

    case 2:
    {
        int opened;
        const char *cgMenuName = (*(cg_t **)imp_cg)->scriptMainMenu;
        Key_SetCatcher(8);
        Menus_CloseAll(uiInfo);
        opened = Menus_OpenByName(uiInfo, cgMenuName);
        if (!opened) {
            Menus_OpenByName(uiInfo, "main");
        }
    }
        return 1;

    case 3:
        Key_SetCatcher(8);
        Menus_OpenByName(uiInfo, "needcd");
        return 1;

    case 4:
        Key_SetCatcher(8);
        Menus_OpenByName(uiInfo, "badcd");
        return 1;

    case 5:
        Key_SetCatcher(8);
        Menus_OpenByName(uiInfo, "team");
        return 1;

    case 6:
    case 7:
        return 0;

    case 8:
        uiInfo->uiDC.cursorx = 0x27f;
        uiInfo->uiDC.cursory = 0x1df;
        Key_SetCatcher(8);
        (*(clientActive_t **)imp_cl)->displayHUDWithKeycatchUI = 1;
        Menus_CloseAll(uiInfo);
        Menus_OpenByName(uiInfo, "quickmessage");
        return 1;

    case 9:
    case 10:
        pFocus = (menuDef_t *)Menu_GetFocused(uiInfo);
        if (pFocus) {
            int activeMenu = uiInfo->currentMenuType;
            static int traceCount;
            if (traceCount++ < 32) {
                if (getenv("MTRACE"))
                    Com_Printf("[menu-trace] UI popup guard focus='%s' flags=0x%x active=%d loaded=%d\n",
                               pFocus->window.name, pFocus->window.dynamicFlags[0],
                               activeMenu, Menu_Count(uiInfo));
            }
            if (activeMenu != 9 && activeMenu != 10)
                return 0;
        }

        legacyBase = *(byte **)imp_legacyHacks;

        if (pFocus) {

            if (I_stricmp(pFocus->window.name, ((LegacyHacks *)legacyBase)->ui_newScriptMenu) == 0)
                return 1;
        }

        uiInfo->currentMenuType = 9;

        if (menu == 10) {
            uiInfo->uiDC.cursorx = 0x27f;
            uiInfo->uiDC.cursory = 0x1df;
        }

        Key_SetCatcher(8);
        (*(clientActive_t **)imp_cl)->displayHUDWithKeycatchUI = 1;
        Menus_CloseAll(uiInfo);

        strcpy(((LegacyHacks *)legacyBase)->ui_scriptMenu, ((LegacyHacks *)legacyBase)->ui_newScriptMenu);
        ((LegacyHacks *)legacyBase)->ui_scriptMenuIndex = ((LegacyHacks *)legacyBase)->ui_newScriptMenuIndex;
        (*(byte *)&((LegacyHacks *)legacyBase)->ui_newScriptMenu[0]) = 0;
        ((LegacyHacks *)legacyBase)->ui_newScriptMenuIndex = -1;

        Com_Printf("script menu '%s': join dvars auto='%s' allies='%s' axis='%s'\n",
                   ((LegacyHacks *)legacyBase)->ui_scriptMenu,
                   Dvar_GetVariantString("ui_allow_joinauto"),
                   Dvar_GetVariantString("ui_allow_joinallies"),
                   Dvar_GetVariantString("ui_allow_joinaxis"));
        Menus_OpenByName(uiInfo, ((LegacyHacks *)legacyBase)->ui_scriptMenu);
        return 1;

    case 11:
        Key_SetCatcher(8);
        Menus_OpenByName(uiInfo, "player_profile");
        SND_FadeAllSounds(1.0f, 1000);
        return 1;
    }

    return 0;
}
#endif

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
qboolean UI_IsFullscreen(void)
{
    return Menus_AnyFullScreenVisible(uiInfo);
}
#endif

static int UI_ReadableSizeFraction(int value, unsigned int mask, int shift)
{
    int product = (int)((unsigned int)(value & (int)mask) * 100u);

    if (product < 0)
        product += (int)mask;

    return product >> shift;
}

static void __attribute_regparm__(3) UI_ReadableSize(char *buf, int bufsize, int value)
{
    const char *suffix;

    if (value > 0x40000000) {
        int len;

        Com_sprintf(buf, bufsize, "%d", value >> 30);
        suffix = UI_SafeTranslateString("EXE_GIGABYTE");
        len = strlen(buf);
        Com_sprintf(buf + len, bufsize - len, ".%02d %s",
                    UI_ReadableSizeFraction(value, 0x3fffffff, 30), suffix);
    } else if (value > 0x100000) {
        int len;

        Com_sprintf(buf, bufsize, "%d", value >> 20);
        suffix = UI_SafeTranslateString("EXE_MEGABYTE");
        len = strlen(buf);
        Com_sprintf(buf + len, bufsize - len, ".%02d %s",
                    UI_ReadableSizeFraction(value, 0xfffff, 20), suffix);
    } else {
        if (value > 0x400) {
            value >>= 10;
            suffix = UI_SafeTranslateString("EXE_KILOBYTE");
        } else {
            suffix = UI_SafeTranslateString("EXE_BYTES");
        }

        Com_sprintf(buf, bufsize, "%d %s", value, suffix);
    }
}

float UI_GetBlurRadius(void)
{
    return uiInfo->uiDC.blurRadiusOut;
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
qboolean UI_AnyFullScreenMenuVisible(void)
{
    return Menus_AnyFullScreenVisible(uiInfo);
}
#endif

const char *UI_ReplaceConversions(const char *sourceString, ConversionArguments *arguments)
{
    char outputString[0x400];
    int sourceStringLength;
    int index;
    int outputStringCounter;

    if (!strstr(sourceString, "&&"))
        return sourceString;

    sourceStringLength = strlen(sourceString);
    memset(outputString, 0, 0x400);

    if (sourceStringLength <= 0)
        return va(outputString);

    outputStringCounter = 0;
    index = 0;

    while (index < sourceStringLength) {

        if (memcmp(&sourceString[index], "&&", 2) == 0) {

            int c = (signed char)sourceString[index + 2];
            {

                extern void *__DefaultRuneLocale;
                unsigned int *rt = (unsigned int *)__DefaultRuneLocale;
                if ((unsigned int)c <= 0xff && (rt[13 + c] & 0x400)) {

                    int argIndex = c - '1';
                    int argLen = strlen(arguments->args[argIndex]);
                    int j;

                    for (j = 0; j < argLen; j++) {
                        outputString[outputStringCounter] = arguments->args[argIndex][j];
                        outputStringCounter++;
                    }
                    index += 3;
                    continue;
                }
            }
        }

        outputString[outputStringCounter] = sourceString[index];
        outputStringCounter++;
        index++;
    }

    return va(outputString);
}

const char *UI_ReplaceConversionString(const char *sourceString, const char *replaceString)
{
    int convArgs[10];
    memset(convArgs, 0, sizeof(convArgs));
    convArgs[0] = 1;
    convArgs[1] = (int)replaceString;
    return UI_ReplaceConversions(sourceString, (ConversionArguments *)convArgs);
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
void UI_CloseAll(void)
{
    Menus_CloseAll(uiInfo);
}
#endif

void UI_CloseFocusedMenu(void)
{
    if (Menu_Count(uiInfo) <= 0)
        return;

    if (!Menu_GetFocused(uiInfo)) {
        if (Key_GetCatcher() & 8) {
            Key_SetCatcher(Key_GetCatcher() & ~8);
        }
        return;
    }

    if (!Menus_AnyFullScreenVisible(uiInfo)) {
        Menus_CloseAll(uiInfo);
    }
}

void UI_OverrideCursorPos(rectDef_t (*item)[16])
{
    byte *itemPtr = (byte *)item;
    float feederFloat = ((itemDef_t *)itemPtr)->special;

    if (feederFloat == 4.0f) {
        int testMapIndex = (ui_currentNetMap)->current.integer;
        int numMaps = sharedUiInfo.mapCount;
        int visCount = 0;
        int i;

        for (i = 0; i < numMaps; i++) {
            if (sharedUiInfo.mapList[i].active != 0) {
                if (i == testMapIndex)
                    goto found;
                visCount++;
            }
        }
        visCount = 0;
    found:
        Item_SetCursorPos(item, visCount);
    } else if (feederFloat == 2.0f) {
        byte *listPtr = (*(byte **)&((itemDef_t *)itemPtr)->typeData.listBox);
        int endPos = *(int *)(listPtr + 0x10);

        if (endPos == 0) {
            Item_SetCursorPos(item, -1);
        } else {
            int serverIndex = sharedUiInfo.serverStatus.currentServer;
            int cursorField = ((itemDef_t *)itemPtr)->cursorPos[0];
            int startPos_val = *(int *)listPtr;
            int delta;
            int maxScroll;

            if (serverIndex < 0)
                return;
            if (cursorField < startPos_val)
                return;
            if (cursorField > endPos)
                return;

            delta = serverIndex - cursorField;
            ListBox_SetStartPos(listPtr, delta + startPos_val);
            ListBox_SetEndPos(listPtr, delta + endPos);
            ListBox_SetCursorPos(listPtr, delta + *(int *)(listPtr + 0x24));
            Item_SetCursorPos(item, sharedUiInfo.serverStatus.currentServer);

            maxScroll = Item_ListBox_MaxScroll(item);
            if (maxScroll < *(int *)listPtr) {
                ListBox_SetStartPos(listPtr, maxScroll);
            }
            if (*(int *)listPtr < 0) {
                ListBox_SetStartPos(listPtr, 0);
            }
        }
    }
}

Bool UI_DrawRecordLevel(rectDef_t *rect)
{
    float voiceLevel;
    const vec_t *white;

    voiceLevel = Voice_GetVoiceLevel();

    if (voiceLevel > 0.0f) {
        float barWidth = voiceLevel * rect->w - 4.0f;
        if (barWidth > 0.0f) {
            UI_FillRect(rect->x + 2.0f, rect->y + 2.0f, barWidth, rect->h - 4.0f, 0, 0, (const vec_t *)imp_colorWhite);
        }
    }

    white = (const vec_t *)imp_colorWhite;

    UI_FillRect(rect->x, rect->y, rect->w, 1.0f, 0, 0, white);

    UI_FillRect(rect->x, rect->y + rect->h - 1.0f, rect->w, 1.0f, 0, 0, white);

    UI_FillRect(rect->x, rect->y + 1.0f, 1.0f, rect->h - 2.0f, 0, 0, white);

    UI_FillRect(rect->x + rect->w - 1.0f, rect->y + 1.0f, 1.0f, rect->h - 2.0f, 0, 0, white);

    return 1;
}

MaterialHandle UI_FeederItemImage(const float feederID, int index)
{
    int numMaps, c, i, mapIndex;
    int byteOff;
    byte *base;

    if (feederID != 4.0f)
        return 0;

    numMaps = sharedUiInfo.mapCount;
    mapIndex = -1;

    if (numMaps > 0) {
        c = 0;
        for (i = 0; i < numMaps; i++) {
            byte *entry = (byte *)&sharedUiInfo + i * 0xa4;
            if (*(int *)(entry + 0x13f4) != 0) {
                if (c == index) {
                    mapIndex = i;
                    break;
                }
                c++;
            }
        }
    }

    if (mapIndex < 0)
        mapIndex = 0;

    if (numMaps <= mapIndex)
        return 0;

    byteOff = mapIndex * 0xa4;

    if (*(int *)((byte *)&sharedUiInfo + 5104 + byteOff) == 0) {

        const char *name = *(const char **)((byte *)&sharedUiInfo + 4956 + byteOff);
        *(int *)((byte *)&sharedUiInfo + 5104 + byteOff) = CL_RegisterMaterialNoMip(name, 3);
    }

    return *(MaterialHandle *)((byte *)&sharedUiInfo + 5104 + byteOff);
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style)
{
    float xScale;
    float yScale;

    diag_drawtext(text, (void *)font, x, y, scale);

    xScale = CL_NormalizedTextScale(font, scale);
    yScale = xScale;
    CalcSplitScreenTextOffset(font, &y);
    CalcScreenPlacement(&x, &y, &xScale, &yScale, horzAlign, vertAlign);

    x = (float)(int)floorf(x + 0.5f);
    y = (float)(int)floorf(y + 0.5f);

    CL_DrawTextPhysical(text, maxChars, font, x, y, xScale, yScale, color, style);
}
#endif

void UI_DrawTextWithCursor(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style, int cursorPos, int cursor)
{
    float xScale;
    float yScale;

    xScale = CL_NormalizedTextScale(font, scale);
    yScale = xScale;
    CalcSplitScreenTextOffset(font, &y);
    CalcScreenPlacement(&x, &y, &xScale, &yScale, horzAlign, vertAlign);

    x = (float)(int)floorf(x + 0.5f);
    y = (float)(int)floorf(y + 0.5f);

    CL_DrawTextPhysicalWithCursor(text, maxChars, font, x, y, xScale, yScale, color, style, cursorPos, (signed char)cursor);
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
static void UI_StartServerRefresh(qboolean full)
{
    qtime_t q;
    char dvarName[0x18];
    int source;
    int now;

    Com_RealTime(&q);
    source = (ui_netSource)->current.integer;
    snprintf(dvarName, sizeof(dvarName), "ui_lastServerRefresh_%i", source);
    Dvar_SetStringByName(dvarName,
                         va("%s %i, %i   %i:%02i",
                            UI_SafeTranslateString(MonthAbbrev[q.tm_mon]),
                            q.tm_mday,
                            q.tm_year + 1900,
                            q.tm_hour,
                            q.tm_min));

    now = uiInfo->uiDC.realTime;

#ifdef __EMSCRIPTEN__
    /* Both refresh buttons rediscover rooms over HTTP; UDP pings would reserve
     * one of the two gameplay slots before the player chooses Join Server. */
    full = 1;
#endif
    if (!full) {
        LAN_ResetPings(source);
        sharedUiInfo.serverStatus.refreshActive = 1;
        sharedUiInfo.serverStatus.refreshtime = now + 1000;
        return;
    }

    sharedUiInfo.serverStatus.refreshActive = 1;
    sharedUiInfo.serverStatus.nextDisplayRefresh = now + 1000;
    sharedUiInfo.serverStatus.numDisplayServers = 0;
    sharedUiInfo.serverStatus.numPlayersOnServers = 0;
    sharedUiInfo.serverStatus.serverCount = LAN_GetServerCount(source);
    LAN_MarkServerDirty(source, -1, 1);
    LAN_ResetPings(source);

    if (source == 0) {
        Cbuf_ExecuteText(0, "localservers\n");
        sharedUiInfo.serverStatus.refreshtime = now + 1000;
        return;
    }

    sharedUiInfo.serverStatus.refreshtime = now + 5000;

    if (source == 1) {
        const char *debugProtocol = Dvar_GetVariantString("debug_protocol");
        if (debugProtocol[0])
            Cbuf_ExecuteText(0, va("globalservers %d %s full empty\n", 0, debugProtocol));
        else
            Cbuf_ExecuteText(0, va("globalservers %d %d full empty\n", 0, 0x73));
    }
}
#endif

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
static void UI_InsertServerAtPosition(int serverIndex, int position)
{
    int numServers = sharedUiInfo.serverStatus.numDisplayServers;
    int *displayServers = sharedUiInfo.serverStatus.displayServers;
    int *selectedServer = &sharedUiInfo.serverStatus.currentServer;
    int j;

    if (position < 0 || numServers < position)
        return;

    if (*selectedServer >= position && numServers != 0)
        *selectedServer += 1;

    numServers++;
    sharedUiInfo.serverStatus.numDisplayServers = numServers;

    for (j = numServers - 1; j > position; j--)
        displayServers[j] = displayServers[j - 1];

    displayServers[position] = serverIndex;
}
#endif

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
static void UI_RemoveDuplicateFromFavorites(int serverIndex)
{
    int numServers = sharedUiInfo.serverStatus.numDisplayServers;
    int *displayServers = sharedUiInfo.serverStatus.displayServers;
    int i, j;

    if (numServers <= 0)
        return;

    for (i = 0; i < numServers; i++) {
        if (displayServers[i] == serverIndex)
            break;
    }
    if (i >= numServers)
        return;

    numServers--;
    sharedUiInfo.serverStatus.numDisplayServers = numServers;

    for (j = i; j < numServers; j++)
        displayServers[j] = displayServers[j + 1];
}
#endif

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
static void UI_BinaryInsertServer(int serverIndex)
{
    int numDisplay = sharedUiInfo.serverStatus.numDisplayServers;
    int *displayServers = sharedUiInfo.serverStatus.displayServers;
    int lo, hi, mid, cmp;
    int source = (ui_netSource)->current.integer;
    int sortKey = sharedUiInfo.serverStatus.sortKey;
    int sortDir = sharedUiInfo.serverStatus.sortDir;

    lo = 0;
    hi = numDisplay;
    /* Search the insertion interval [0, count], including an empty list. */
    while (lo < hi) {
        mid = lo + (hi - lo) / 2;
        cmp = LAN_CompareServers(source, sortKey, sortDir, serverIndex, displayServers[mid]);
        if (cmp > 0)
            lo = mid + 1;
        else
            hi = mid;
    }
    UI_InsertServerAtPosition(serverIndex, lo);
}
#endif

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
static void __attribute_regparm__(0) UI_BuildServerDisplayList(qboolean force)
{
    char info_buf[0x400];
    int count, i, clients, ping;
    int netSource;

    if (!force) {
        if (uiInfo->uiDC.realTime <= sharedUiInfo.serverStatus.nextDisplayRefresh)
            return;
    } else if (force == 2) {

    }

    clients = 0;

    {
        const char *motdString = Dvar_GetString("cl_motdString");
        int motdLen;
        I_strncpyz(sharedUiInfo.serverStatus.motd, motdString, 0x400);
        motdLen = strlen(sharedUiInfo.serverStatus.motd);

        if (motdLen == 0) {
            const char *translated = UI_SafeTranslateString("EXE_COD_MULTIPLAYER");
            strcpy(sharedUiInfo.serverStatus.motd, va("%s - %s", translated, "1.0"));
            motdLen = strlen(sharedUiInfo.serverStatus.motd);
        }

        if (motdLen != sharedUiInfo.serverStatus.motdLen) {
            sharedUiInfo.serverStatus.motdLen = motdLen;
            sharedUiInfo.serverStatus.motdWidth = -1;
        }
    }

    if (force) {
        numclean = 0;
        sharedUiInfo.serverStatus.numDisplayServers = 0;
        sharedUiInfo.serverStatus.numPlayersOnServers = 0;
        netSource = (ui_netSource)->current.integer;
        sharedUiInfo.serverStatus.serverCount = LAN_GetServerCount(netSource);

        if (sharedUiInfo.serverStatus.currentServer >= 0) {
            Menu_SetFeederSelection(uiInfo, 0, 2, 0, 0);
        }

        LAN_MarkServerDirty((ui_netSource)->current.integer, -1, 1);
    }

    netSource = (ui_netSource)->current.integer;
    count = LAN_GetServerCount(netSource);

    if (LAN_WaitServerResponse(netSource) || (netSource == 0 && count == 0)) {
        sharedUiInfo.serverStatus.numDisplayServers = 0;
        sharedUiInfo.serverStatus.numPlayersOnServers = 0;
        sharedUiInfo.serverStatus.serverCount = LAN_GetServerCount((ui_netSource)->current.integer);
        sharedUiInfo.serverStatus.nextDisplayRefresh = uiInfo->uiDC.realTime + 500;
        return;
    }

    qsort(sharedUiInfo.serverStatus.displayServers, sharedUiInfo.serverStatus.numDisplayServers, 4, UI_ServersQsortCompare);

    for (i = 0; i < count; i++) {
        if (!LAN_ServerIsDirty((ui_netSource)->current.integer, i))
            continue;

        ping = LAN_GetServerPing((ui_netSource)->current.integer, i);

        if (ping <= 0 && (ui_netSource)->current.integer != 2)
            continue;

        LAN_GetServerInfo((ui_netSource)->current.integer, i, info_buf, 0x400);

        clients = atoi(Info_ValueForKey(info_buf, "clients"));
        sharedUiInfo.serverStatus.numPlayersOnServers += clients;

        if (getenv("MTRACE")) {
            Com_Printf("[server-filter] info=%s empty=%d full=%d pass=%d nopass=%d pure=%d dedicated=%d mod=%d ff=%d kc=%d gametype=%d\n",
                       info_buf, ui_browserShowEmpty->current.enabled, ui_browserShowFull->current.enabled,
                       ui_browserShowPassword->current.enabled, ui_browserShowNoPassword->current.enabled,
                       ui_browserShowPure->current.enabled, ui_browserShowDedicated->current.enabled,
                       ui_browserMod->current.integer, ui_browserFriendlyfire->current.integer,
                       ui_browserKillcam->current.integer, ui_joinGameType->current.integer);
        }

        if (I_strnicmp(Info_ValueForKey(info_buf, "addr"), "000.000.000.000", 15) == 0)
            goto reject;

        if (!(ui_browserShowEmpty)->current.enabled && clients == 0)
            goto reject;

        if (!(ui_browserShowFull)->current.enabled) {
            if (clients == atoi(Info_ValueForKey(info_buf, "sv_maxclients")))
                goto reject;
        }

        if (!(ui_browserShowPassword)->current.enabled) {
            if (atoi(Info_ValueForKey(info_buf, "pswrd")) != 0)
                goto reject;
        }

        if (!(ui_browserShowNoPassword)->current.enabled) {
            if (atoi(Info_ValueForKey(info_buf, "pswrd")) == 0)
                goto reject;
        }

        if ((ui_browserShowPure)->current.enabled) {
            if (atoi(Info_ValueForKey(info_buf, "pure")) == 0)
                goto reject;
        }

        if ((ui_browserShowDedicated)->current.enabled) {
            if ((unsigned)(atoi(Info_ValueForKey(info_buf, "hw")) - 1) > 2)
                goto reject;
        }

        if ((ui_browserMod)->current.integer >= 0) {
            if (atoi(Info_ValueForKey(info_buf, "mod")) != (ui_browserMod)->current.integer)
                goto reject;
        }

        if ((ui_browserFriendlyfire)->current.integer >= 0) {
            if (atoi(Info_ValueForKey(info_buf, "ff")) != (ui_browserFriendlyfire)->current.integer)
                goto reject;
        }

        if ((ui_browserKillcam)->current.integer >= 0) {
            if (atoi(Info_ValueForKey(info_buf, "kc")) != (ui_browserKillcam)->current.integer)
                goto reject;
        }

        {
            int joinGTIdx = (ui_joinGameType)->current.integer;
            const char *joinGTName = sharedUiInfo.joinGameTypes[joinGTIdx].gameTypeName;
            if (joinGTName[0] != '\0') {
                const char *joinGTShort = sharedUiInfo.joinGameTypes[joinGTIdx].gameType;
                if (I_stricmp(Info_ValueForKey(info_buf, "gametype"), joinGTShort) != 0)
                    goto reject;
            }
        }

        if (ui_serverFilterType > 0) {
            const char *filterBaseName = *(const char **)((byte *)serverFilters + ui_serverFilterType * 8 + 4);
            if (I_stricmp(Info_ValueForKey(info_buf, "game"), filterBaseName) != 0)
                goto reject;
        }

        if ((ui_netSource)->current.integer == 2) {
            UI_RemoveDuplicateFromFavorites(i);
        }

        UI_BinaryInsertServer(i);

        if (ping > 0) {
            LAN_MarkServerDirty((ui_netSource)->current.integer, i, 0);
            numclean++;
        }
        continue;

    reject:
        LAN_MarkServerDirty((ui_netSource)->current.integer, i, 0);
    }

    sharedUiInfo.serverStatus.refreshtime = uiInfo->uiDC.realTime;
}
#endif

static int UI_IsActionKey(int key)
{
    return (key == 0xc8 || key == 0xc9 || key == 0xd || key == 0xbf);
}

static int UI_UpdateMapVisibility(int listIndex)
{
    int numMaps = sharedUiInfo.mapCount;
    int i, visCount = 0;
    byte *p = (byte *)&sharedUiInfo;

    for (i = 0; i < numMaps; i++) {
        *(int *)(p + 0x13f4 + i * 0xa4) = 0;
        if ((*(int *)(p + 0x1368 + i * 0xa4) >> listIndex) & 1) {
            visCount++;
            *(int *)(p + 0x13f4 + i * 0xa4) = 1;
        }
    }
    return visCount;
}

static void UI_SelectFirstVisibleMap(int currentMapIdx)
{
    int numMaps = sharedUiInfo.mapCount;
    int i, firstVisible = -1;
    byte *p = (byte *)&sharedUiInfo;

    if (currentMapIdx >= 0 && currentMapIdx < numMaps) {
        int off = currentMapIdx * 0xa4;
        if (*(int *)(p + 5108 + off) != 0) {

            int listIdx = 0;
            for (i = 0; i < numMaps; i++) {
                if (*(int *)(p + 0x13f4 + i * 0xa4) != 0) {
                    if (i == currentMapIdx)
                        break;
                    listIdx++;
                }
            }
            if (i >= numMaps)
                listIdx = 0;

            Menu_SetFeederSelection(uiInfo, 0, 4, listIdx, "createserver_maps");
            UI_SelectCurrentMap();
            return;
        }
    }

    if (numMaps > 0) {
        if (sharedUiInfo.mapList[0].active != 0) {
            firstVisible = 0;
        } else {
            for (i = 1; i < numMaps; i++) {
                if (*(int *)(p + 0x1498 + (i - 1) * 0xa4) != 0) {
                    firstVisible = i;
                    break;
                }
            }
        }
    }

    if (firstVisible >= 0) {
        Menu_SetFeederSelection(uiInfo, 0, 4, 0, "createserver_maps");
        Dvar_SetInt(ui_currentNetMap, firstVisible);
    }

    UI_SelectCurrentMap();
}

qboolean UI_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key)
{
    int idx;

    idx = ownerDraw - 0xcd;
    if ((unsigned)idx > 0x30)
        return 0;

    switch (ownerDraw) {
    case 0xcd:
        if (!UI_IsActionKey(key))
            return 0;
        {
            int listIndex = (ui_netGameType)->current.integer;
            int oldVisCount = UI_UpdateMapVisibility(listIndex);
            int newGT, newVisCount;

            if (key == 0xc9) {

                newGT = (ui_gametype)->current.integer - 1;
                if (newGT == 2)
                    newGT = 1;
                else if (newGT <= 1)
                    newGT = sharedUiInfo.numGameTypes - 1;
            } else {

                newGT = (ui_gametype)->current.integer + 1;
                if (newGT >= sharedUiInfo.numGameTypes)
                    newGT = 1;
                else if (newGT == 2)
                    newGT = 3;
            }

            Dvar_SetInt(ui_gametype, newGT);
            listIndex = (ui_netGameType)->current.integer;
            newVisCount = UI_UpdateMapVisibility(listIndex);

            if (newVisCount != oldVisCount) {
                Dvar_SetInt(ui_currentMap, 0);
            }
        }
        return 1;

    case 0xdc:
        if (!UI_IsActionKey(key))
            return 0;
        {
            int nextNetSource;
            if (key == 0xc9) {

                int cur = (ui_netSource)->current.integer;
                nextNetSource = (cur == 0) ? 2 : cur - 1;
            } else {

                nextNetSource = (ui_netSource)->current.integer + 1;
                if (nextNetSource == 3)
                    nextNetSource = 0;
            }

            UI_BuildServerDisplayList(1);
            Dvar_SetInt(ui_netSource, nextNetSource);
            if ((ui_netSource)->current.integer != 1) {
                UI_StartServerRefresh(1);
                UI_BuildServerDisplayList(1);
            }
        }
        return 0;

    case 0xde:
        if (!UI_IsActionKey(key))
            return 0;
        {
            int filterType;
            if (key == 0xc9)
                filterType = ui_serverFilterType - 1;
            else
                filterType = ui_serverFilterType + 1;

            ui_serverFilterType = filterType;

            if (filterType > 0 || filterType < 0)
                ui_serverFilterType = 0;

            UI_BuildServerDisplayList(1);
        }
        return 0;

    case 0xf4:
        if (!UI_IsActionKey(key))
            return 0;
        {
            int newVal;
            if (key == 0xc9) {

                int cur = (ui_netGameType)->current.integer;
                newVal = (cur == 0) ? sharedUiInfo.numGameTypes : cur;
                newVal--;
            } else {

                newVal = (ui_netGameType)->current.integer + 1;
                if (newVal == sharedUiInfo.numGameTypes)
                    newVal = 0;
            }

            Dvar_SetInt(ui_netGameType, newVal);
            Dvar_SetString(ui_netGameTypeName, sharedUiInfo.gameTypes[(ui_netGameType)->current.integer].gameType);

            UI_UpdateMapVisibility((ui_netGameType)->current.integer);
            UI_SelectFirstVisibleMap((ui_currentNetMap)->current.integer);
        }
        return 1;

    case 0xfd:
        if (!UI_IsActionKey(key))
            return 0;
        {
            int newVal;
            if (key == 0xc9) {

                int cur = (ui_joinGameType)->current.integer;
                newVal = (cur == 0) ? sharedUiInfo.numJoinGameTypes : cur;
                newVal--;
            } else {

                newVal = (ui_joinGameType)->current.integer + 1;
                if (newVal == sharedUiInfo.numJoinGameTypes)
                    newVal = 0;
            }

            Dvar_SetInt(ui_joinGameType, newVal);
            UI_BuildServerDisplayList(1);
        }
        return 1;

    default:
        return 0;
    }
}

const char *UI_FeederItemText(float feederID, int index, int column, MaterialHandle *handle)
{
    *handle = 0;

    if (feederID == 4.0f) {

        int numMaps = sharedUiInfo.mapCount;
        int count = 0;
        int i;

        if (numMaps <= 0)
            return "";

        for (i = 0; i < numMaps; i++) {
            byte *entry = (byte *)&sharedUiInfo + i * 0xa4;
            if (*(int *)(entry + 0x13f4)) {
                if (count == index) {

                    return *(const char **)(entry + 0x1354);
                }
                count++;
            }
        }
        return "";
    } else if (feederID == 2.0f) {

        int pingVal;

        {
            int serverCount = LAN_GetServerCount((ui_netSource)->current.integer);
            if (serverCount != sharedUiInfo.serverStatus.serverCount) {
                sharedUiInfo.serverStatus.serverCount = serverCount;
                if (sharedUiInfo.serverStatus.numDisplayServers != 0) {
                    sharedUiInfo.serverStatus.currentServer = -1;
                    UI_BuildServerDisplayList(1);
                }
            }
        }

        if (index < 0 || index >= sharedUiInfo.serverStatus.numDisplayServers)
            return "";

        if (column != lastColumn ||
            uiInfo->uiDC.realTime + 5000 < lastTime) {
            LAN_GetServerInfo((ui_netSource)->current.integer,
                              sharedUiInfo.serverStatus.displayServers[index],
                              info, 0x400);
            lastColumn = column;
            lastTime = uiInfo->uiDC.realTime;
        }

        pingVal = atoi(Info_ValueForKey(info, "ping"));

        if (column <= 9) {
            switch (column) {
            case 0:
                if (atoi(Info_ValueForKey(info, "pswrd")))
                    return "X";
                return "";
            case 1: {
                int hw = atoi(Info_ValueForKey(info, "hw"));
                if ((unsigned)hw > 7)
                    return "";
                *handle = *(MaterialHandle *)((byte *)&sharedUiInfo + 25940 + hw * 4);
                return "";
            }
            case 2:
                if (pingVal <= 0)
                    return Info_ValueForKey(info, "addr");
                I_strncpyz(clientBuff, Info_ValueForKey(info, "hostname"), 0x14);
                return clientBuff;
            case 3: {
                const char *mapName = Info_ValueForKey(info, "mapname");
                int numM = sharedUiInfo.mapCount;
                int mi;
                if (numM <= 0)
                    return mapName;
                for (mi = 0; mi < numM; mi++) {
                    byte *entry = (byte *)&sharedUiInfo + mi * 0xa4;
                    if (I_stricmp(mapName, *(const char **)(entry + 0x1358)) == 0) {
                        return *(const char **)(entry + 0x1354);
                    }
                }
                return mapName;
            }
            case 4: {
                const char *maxClients = Info_ValueForKey(info, "sv_maxclients");
                const char *clients = Info_ValueForKey(info, "clients");
                Com_sprintf(clientBuff, 0x20, "%s (%s)", clients, maxClients);
                return clientBuff;
            }
            case 5: {
                const char *gt = Info_ValueForKey(info, "gametype");
                if (!gt || gt[0] == '\0')
                    return "?";
                return Info_ValueForKey(info, "gametype");
            }
            case 6:
                if (atoi(Info_ValueForKey(info, "voice")))
                    return "X";
                return "";
            case 7:
                if (atoi(Info_ValueForKey(info, "pure")))
                    return "X";
                return "";
            case 8:
                if (atoi(Info_ValueForKey(info, "mod")))
                    return "X";
                return "";
            case 9:
                if (pingVal > 0)
                    return Info_ValueForKey(info, "ping");
                return "...";
            }
        }
    } else if (feederID == 13.0f) {

        if (index < 0 || index >= sharedUiInfo.serverStatusInfo.numLines)
            return "";
        if ((unsigned)column > 3)
            return "";
        {
            const char *text = *(const char **)((byte *)&sharedUiInfo + 109864 + (column + index * 4) * 4);
            if (text[0] == '@')
                return UI_SafeTranslateString(text + 1);
            return text;
        }
    } else if (feederID == 7.0f) {

        if (index < 0 || index >= sharedUiInfo.playerCount)
            return "";
        return sharedUiInfo.playerNames[index];
    } else if (feederID == 9.0f) {

        if (index < 0 || index >= sharedUiInfo.modCount)
            return "";
        {
            const char *desc = *(const char **)((byte *)&sharedUiInfo + 25976 + index * 8);
            if (desc && desc[0] != '\0')
                return desc;
            return *(const char **)((byte *)&sharedUiInfo + 25972 + index * 8);
        }
    } else if (feederID == 20.0f) {

        if (index < 0 || index >= sharedUiInfo.playerCount)
            return "";
        if (column - 1 == 0) {

            return sharedUiInfo.playerNames[index];
        }

        if (CL_IsPlayerMuted(sharedUiInfo.playerClientNums[index]))
            return UI_SafeTranslateString("MP_MUTED");
        return "";
    } else if (feederID == 24.0f) {

        if (index < 0 || index >= uiInfo->playerProfileCount)
            return "";
        {
            int sortedIdx = uiInfo->playerProfileStatus.displayProfile[index];
            return uiInfo->playerProfileName[sortedIdx];
        }
    }

    return "";
}

void UI_DrawRect(float x, float y, float width, float height, int horzAlign, int vertAlign, float size, const vec_t *color)
{
    CL_DrawStretchPic(x, y, width, size, horzAlign, vertAlign, 0, 0, 0, 0, color, sharedUiInfo.assets.whiteMaterial);
    CL_DrawStretchPic(x, y + height - size, width, size, horzAlign, vertAlign, 0, 0, 0, 0, color, sharedUiInfo.assets.whiteMaterial);
    CL_DrawStretchPic(x, y, size, height, horzAlign, vertAlign, 0, 0, 0, 0, color, sharedUiInfo.assets.whiteMaterial);
    CL_DrawStretchPic(x + width - size, y, size, height, horzAlign, vertAlign, 0, 0, 0, 0, color, sharedUiInfo.assets.whiteMaterial);
}

int UI_OwnerDrawWidth(int ownerDraw, FontHandle font, float scale)
{
    const char *s = NULL;

    switch (ownerDraw) {
    case 0x27:
        s = CG_GameTypeString();
        break;
    case 0x32:
        s = CG_GetKillerText();
        break;
    case 0xcd:
        s = sharedUiInfo.gameTypes[(ui_gametype)->current.integer].gameTypeName;
        break;
    case 0xdc: {
        int netSrcVal = (ui_netSource)->current.integer;
        if (netSrcVal > sharedUiInfo.numJoinGameTypes) {
            Dvar_SetInt(ui_netSource, 0);
            netSrcVal = (ui_netSource)->current.integer;
        }
        s = SEH_LocalizeTextMessage(va("EXE_NETSOURCE\x14%s", netSources[netSrcVal]), "net source", 0);
        break;
    }
    case 0xde: {
        int filterType = 0;
        if ((unsigned int)ui_serverFilterType < 2)
            filterType = ui_serverFilterType;
        ui_serverFilterType = filterType;
        s = SEH_LocalizeTextMessage(va("EXE_SERVERFILTER\x14%s", serverFilters[filterType].description), "server filter", 0);
        break;
    }
    case 0xf7:
        s = Dvar_GetVariantString(va("ui_lastServerRefresh_%i", (ui_netSource)->current.integer));
        break;
    case 0xfa:
        if (Display_KeyBindPending())
            s = UI_SafeTranslateString("EXE_KEYWAIT");
        else
            s = UI_SafeTranslateString("EXE_KEYCHANGE");
        break;
    default:
        return 0;
    }

    if (!s)
        return 0;

    {
        float actualScale = CL_NormalizedTextScale(font, scale);
        return (int)((float)CL_TextWidth(s, 0, font) * actualScale);
    }
}

void UI_OwnerDraw(float x, float y, float w, float h, int horzAlign, int vertAlign, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, FontHandle font, float scale, vec_t *color, MaterialHandle material, int textStyle)
{

    float rect[6];
    const char *text;
    clientActive_t *clBase;

    clBase = *(clientActive_t **)imp_cl;
    if (clBase->cgameInitialized != 0) {
        CG_OwnerDraw(x, y, w, h, horzAlign, vertAlign, text_x, text_y, ownerDraw, ownerDrawFlags, align, special, (int)font, scale, color, (int)material, textStyle);
    }

    rect[0] = x + text_x;
    rect[1] = y + text_y;
    *(int *)&rect[2] = *(int *)&w;
    rect[3] = h;
    *(int *)&rect[4] = horzAlign;
    *(int *)&rect[5] = vertAlign;

    switch (ownerDraw) {
    case 205:
    {
        const char *gtName = sharedUiInfo.gameTypes[(ui_gametype)->current.integer].gameTypeName;
        if (gtName[0] == '\0')
            gtName = "EXE_ALL";
        text = UI_SafeTranslateString(gtName);
        UI_DrawText(text, 0x7fffffff, font, rect[0], rect[1], 0, 0, scale, color, textStyle);
        return;
    }

    case 206:
    {
        UI_DrawMapPreview((const rectDef_t *)rect, color, 1);
        return;
    }

    case 220:
    {
        text = SEH_LocalizeTextMessage(
            va("EXE_NETSOURCE\x14%s", netSources[(ui_netSource)->current.integer]),
            "net source", 0);
        UI_DrawText(text, 0x7fffffff, font, rect[0], rect[1], 0, 0, scale, color, textStyle);
        return;
    }

    case 221:
    {
        MaterialHandle lsHandle = sharedUiInfo.serverStatus.currentServerPreview;
        if (!lsHandle) {
            lsHandle = CL_RegisterMaterialNoMip("menu/art/unknownmap", 3);
        }
        UI_DrawHandlePic(rect[0], rect[1], rect[2], rect[3], *(int *)&rect[4], *(int *)&rect[5], color, lsHandle);
        return;
    }

    case 222:
    {
        int filterType = 0;
        if ((unsigned int)ui_serverFilterType < 2)
            filterType = ui_serverFilterType;
        ui_serverFilterType = filterType;
        text = SEH_LocalizeTextMessage(
            va("EXE_SERVERFILTER\x14%s", serverFilters[filterType].description),
            "server filter", 0);
        UI_DrawText(text, 0x7fffffff, font, rect[0], rect[1], 0, 0, scale, color, textStyle);
        return;
    }

    case 244:
    {
        UI_DrawMapPreview((const rectDef_t *)rect, color, 0);
        return;
    }

    case 245:
    {
        int gtIdx = (ui_netGameType)->current.integer;
        if (gtIdx > sharedUiInfo.numGameTypes) {
            Dvar_SetInt(ui_netGameType, 0);
            Dvar_SetString(ui_netGameTypeName, sharedUiInfo.gameTypes[0].gameType);
            gtIdx = (ui_netGameType)->current.integer;
        }
        {
            const char *gtName = sharedUiInfo.gameTypes[gtIdx].gameTypeName;
            if (gtName[0] == '\0')
                gtName = "EXE_ALL";
            text = UI_SafeTranslateString(gtName);
            UI_DrawText(text, 0x7fffffff, font, rect[0], rect[1], 0, 0, scale, color, textStyle);
        }
        return;
    }

    case 246:
    {
        int netMap = (ui_currentNetMap)->current.integer;
        if (netMap >= sharedUiInfo.mapCount) {
            Dvar_SetInt(ui_currentNetMap, 0);
        }
        {
            int cinHandle = sharedUiInfo.serverStatus.currentServerCinematic;
            if (cinHandle < 0) {

                MaterialHandle img = sharedUiInfo.serverStatus.currentServerPreview;
                if (!img) {
                    img = CL_RegisterMaterialNoMip("menu/art/unknownmap", 3);
                }
                UI_DrawHandlePic(rect[0], rect[1], rect[2], rect[3], *(int *)&rect[4], *(int *)&rect[5], color, img);
            } else {
                CIN_RunCinematic(cinHandle);
                CIN_SetExtents(cinHandle, (int)rect[0], (int)rect[1], (int)rect[2], (int)rect[3]);
                CIN_DrawCinematic(cinHandle);
            }
        }
        return;
    }

    case 247:
    {
        int refreshing = sharedUiInfo.serverStatus.refreshActive;
        if (refreshing) {

            vec_t lowLight[4], newColor[4];
            int serverCount;
            int waitResponse;
            const char *countText;

            lowLight[0] = color[0] * 0.8f;
            lowLight[1] = color[1] * 0.8f;
            lowLight[2] = color[2] * 0.8f;
            lowLight[3] = color[3] * 0.8f;

            {
                int realTime = uiInfo->uiDC.realTime;
                float phase = (float)(realTime % 1000);
                float sinVal;
                {
                    double sv;
                    extern double sin(double);
                    sv = sin((double)phase);
                    sinVal = (float)(sv * 0.5 + 0.5);
                }
                LerpColor(color, lowLight, newColor, sinVal);
            }

            waitResponse = LAN_WaitServerResponse((ui_netSource)->current.integer);
            if (waitResponse) {
                countText = UI_SafeTranslateString("EXE_WAITINGFORMASTERSERVERRESPONSE");
            } else {
                serverCount = LAN_GetServerCount((ui_netSource)->current.integer);
                {
                    char tempString[64];
                    int convArgs[10];
                    int ci;

                    text = UI_SafeTranslateString("EXE_GETTINGINFOFORSERVERS");

                    for (ci = 0; ci < 10; ci++)
                        convArgs[ci] = 0;
                    sprintf(tempString, "%d", serverCount);
                    convArgs[0] = 1;
                    convArgs[1] = (int)tempString;
                    countText = UI_ReplaceConversions(text, (ConversionArguments *)convArgs);
                }
            }

            UI_DrawText(countText, 0x7fffffff, font, rect[0], rect[1], 0, 0, scale, newColor, textStyle);
        } else {

            char tempString[64];
            int convArgs[10];
            int ci;

            I_strncpyz(tempString, Dvar_GetVariantString(va("ui_lastServerRefresh_%i", (ui_netSource)->current.integer)), 64);
            text = UI_SafeTranslateString("EXE_REFRESHTIME");

            for (ci = 0; ci < 10; ci++)
                convArgs[ci] = 0;
            convArgs[0] = 1;
            convArgs[1] = (int)tempString;
            text = UI_ReplaceConversions(text, (ConversionArguments *)convArgs);

            UI_DrawText(text, 0x7fffffff, font, rect[0], rect[1], 0, 0, scale, color, textStyle);
        }
        return;
    }

    case 250:
    {
        const char *keyStr;
        if (Display_KeyBindPending())
            keyStr = "EXE_KEYWAIT";
        else
            keyStr = "EXE_KEYCHANGE";
        text = UI_SafeTranslateString(keyStr);
        UI_DrawText(text, 0x7fffffff, font, rect[0], rect[1], *(int *)&rect[4], *(int *)&rect[5], scale, color, textStyle);
        return;
    }

    case 252:
    {
        int jgtIdx = (ui_joinGameType)->current.integer;
        if (jgtIdx > sharedUiInfo.numJoinGameTypes) {
            Dvar_SetInt(ui_joinGameType, 0);
            jgtIdx = (ui_joinGameType)->current.integer;
        }
        {
            const char *gtName = sharedUiInfo.joinGameTypes[jgtIdx].gameTypeName;
            if (gtName[0] == '\0')
                gtName = "EXE_ALL";
            text = UI_SafeTranslateString(gtName);
            UI_DrawText(text, 0x7fffffff, font, rect[0], rect[1], 0, 0, scale, color, textStyle);
        }
        return;
    }

    case 253:
    {
        int cinHandle = sharedUiInfo.previewMovie;
        if (cinHandle <= -2)
            return;
        cinHandle = CIN_PlayCinematic(
            sharedUiInfo.movieList[sharedUiInfo.movieIndex],
            0, 0, 0, 0, 10);
        sharedUiInfo.previewMovie = cinHandle;
        if (cinHandle < 0) {
            sharedUiInfo.previewMovie = -2;
            return;
        }
        CIN_RunCinematic(cinHandle);
        CIN_SetExtents(cinHandle, (int)rect[0], (int)rect[1], (int)rect[2], (int)rect[3]);
        CIN_DrawCinematic(cinHandle);
        return;
    }

    case 254:
    {
        UI_DrawMapPreview((const rectDef_t *)rect, color, 1);
        return;
    }

    case 263:
        UI_DrawRecordLevel((rectDef_t *)rect);
        return;

    case 264:
    {
        if ((*(dvar_t **)imp_sv_voice)->current.enabled == 0 || (*(dvar_t **)imp_cl_voice)->current.enabled == 0)
            return;
        if (!IsTalking())
            return;
        {
            MaterialHandle voiceMat = CL_RegisterMaterialNoMip("voice_on", 7);
            UI_DrawHandlePic(rect[0], rect[1], rect[2], rect[3], *(int *)&rect[4], *(int *)&rect[5], color, voiceMat);
        }
        return;
    }

    case 265:
    case 266:
    case 267:
    case 268:
    case 269:
    case 270:
    {
        int targetTalker = ownerDraw - 265;
        int talkerCount = 0;
        int clientNum;
        int pi;

        if (!(*(dvar_t **)imp_sv_voice)->current.enabled ||
            !(*(dvar_t **)imp_cl_voice)->current.enabled)
            return;

        for (pi = 0; pi < 64; pi++) {
            if (!CL_IsPlayerTalking(pi))
                continue;
            if (talkerCount == targetTalker)
                break;
            talkerCount++;
        }

        clientNum = pi;
        if (clientNum < 0 || clientNum >= 64)
            return;

        {
            MaterialHandle voiceMat;
            float actualScale;
            int textHeight;
            int num;

            UI_BuildPlayerList();

            for (num = 0; num < 64; num++) {
                if (sharedUiInfo.playerClientNums[num] == clientNum)
                    break;
            }
            if (num >= 64)
                return;

            voiceMat = CL_RegisterMaterialNoMip("voice_on", 7);
            actualScale = CL_NormalizedTextScale(font, scale);
            textHeight = CL_TextHeight(font);

            UI_DrawHandlePic(rect[0], rect[1], rect[2], rect[3], *(int *)&rect[4], *(int *)&rect[5], color, voiceMat);

            {
                float scaledHeight = (float)((int)((float)textHeight * actualScale));
                float textY = rect[1] + (rect[3] - scaledHeight) * 0.5f + scaledHeight;
                float textX = rect[0] + rect[2] + 2.0f;
                UI_DrawText(sharedUiInfo.playerNames[num], 32, font, textX, textY, *(int *)&rect[4], *(int *)&rect[5], scale, color, textStyle);
            }
        }
        return;
    }

    default:
        return;
    }

}

static int UI_StrContains(const char *str, const char *charset)
{
    const char *s;
    int ci, si, k;

    if (!charset[0])
        return str ? 1 : 0;

    for (s = str; *s; s++) {
        for (k = 0; charset[k]; k++) {
            if (s[k] == '\0')
                return 0;
            if (___toupper((signed char)charset[k]) != ___toupper((signed char)s[k]))
                break;
        }
        if (!charset[k])
            return 1;
    }
    return 0;
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
static inline __attribute__((always_inline)) void UI_UpdateServerCount(void)
{
    int serverCount = LAN_GetServerCount((ui_netSource)->current.integer);
    if (serverCount != sharedUiInfo.serverStatus.serverCount) {
        sharedUiInfo.serverStatus.serverCount = serverCount;
        if (sharedUiInfo.serverStatus.numDisplayServers != 0) {
            sharedUiInfo.serverStatus.currentServer = -1;
            UI_BuildServerDisplayList(1);
        }
    }
}
#endif

static void BM_NOINLINE UI_BuildFindPlayerList(void)
{
    serverStatusInfo_t info;
    char infoString[0x400];
    char name[0x22];
    int i, j, statusResult;
    int *pendingFlag;
    byte *serverAddr;
    byte *hostName;
    byte *hostName2;
    byte *slotBase;
    int nextRefresh;
    const char *charset;
    int numResults;

    nextRefresh = uiInfo->nextFindPlayerRefresh;
    if (!nextRefresh)
        return;
    if (nextRefresh > uiInfo->uiDC.realTime)
        return;

    UI_UpdateServerCount();

    for (i = 0; i < 16; i++) {
        slotBase = (byte *)&sharedUiInfo + i * 0x8c;
        serverAddr = (byte *)&sharedUiInfo + 113140 + i * 0x8c;
        hostName = (byte *)&sharedUiInfo + 113204 + i * 0x8c;
        hostName2 = (byte *)&sharedUiInfo + 113204 + i * 0x8c;
        pendingFlag = (int *)((byte *)&sharedUiInfo + 0x1ba7c + i * 0x8c);

        if (!*pendingFlag) {
            UI_GetServerStatusInfo((const char *)serverAddr, 0);
            *pendingFlag = 0;

            UI_UpdateServerCount();

            if (sharedUiInfo.pendingServerStatus.num < sharedUiInfo.serverStatus.numDisplayServers) {

                int curServer = sharedUiInfo.pendingServerStatus.num;
                int dispServer;

                sharedUiInfo.pendingServerStatus.server[i].startTime = uiInfo->uiDC.realTime;

                dispServer = sharedUiInfo.serverStatus.displayServers[curServer];
                LAN_GetServerAddressString((ui_netSource)->current.integer, dispServer, (char *)serverAddr, 0x40);

                LAN_GetServerInfo((ui_netSource)->current.integer, dispServer, infoString, 0x400);

                I_strncpyz((char *)hostName, Info_ValueForKey(infoString, "hostname"), 0x40);

                *pendingFlag = 1;

                curServer++;
                sharedUiInfo.pendingServerStatus.num = curServer;

                numResults = uiInfo->numFoundPlayerServers;
                Com_sprintf((char *)((byte *)uiInfo + 0xc60 + numResults * 64), 0x40, "searching %d/%d...", curServer, numFound);
            }
            continue;
        }

        statusResult = UI_GetServerStatusInfo((const char *)serverAddr, &info);

        if (!statusResult) {

            if (!*pendingFlag)
                continue;

            {
                int timeoutVal = uiInfo->uiDC.realTime - (ui_serverStatusTimeOut)->current.integer;
                if (sharedUiInfo.pendingServerStatus.server[i].startTime >= timeoutVal)
                    continue;
            }
            numTimeOuts++;
            continue;
        }

        numFound++;

        if (info.numLines > 0) {
            for (j = 0; j < info.numLines; j++) {
                if (!info.lines[j][2] || info.lines[j][2][0] == '\0')
                    continue;

                I_strncpyz(name, info.lines[j][3], 0x22);
                I_CleanStr(name);

                charset = uiInfo->findPlayerName;

                if (name[0] == '\0')
                    continue;

                if (UI_StrContains(name, charset)) {

                    numResults = uiInfo->numFoundPlayerServers;
                    if (numResults > 14) {

                        sharedUiInfo.pendingServerStatus.num = sharedUiInfo.serverStatus.numDisplayServers;
                        continue;
                    }

                    I_strncpyz((char *)((byte *)uiInfo + 0x860 + numResults * 64), (const char *)serverAddr, 0x40);
                    I_strncpyz((char *)((byte *)uiInfo + 0xc60 + numResults * 64), (const char *)hostName2, 0x40);
                    uiInfo->numFoundPlayerServers += 1;
                    continue;
                }
            }
        }

        Com_sprintf((char *)((byte *)uiInfo + 0xc60 + uiInfo->numFoundPlayerServers * 64), 0x40, "searching %d/%d...", sharedUiInfo.pendingServerStatus.num, numFound);
        *pendingFlag = 0;

        if (!*pendingFlag)
            continue;

        {
            int timeoutVal = uiInfo->uiDC.realTime - (ui_serverStatusTimeOut)->current.integer;
            if (sharedUiInfo.pendingServerStatus.server[i].startTime >= timeoutVal)
                continue;
        }
        numTimeOuts++;

    }

    for (i = 0; i < 16; i++) {
        if (sharedUiInfo.pendingServerStatus.server[i].valid != 0) {

            uiInfo->nextFindPlayerRefresh = uiInfo->uiDC.realTime + 25;
            return;
        }
    }

    numResults = uiInfo->numFoundPlayerServers;
    if (numResults == 0) {
        Com_sprintf((char *)((byte *)uiInfo + 0xc60), 0x40, "no servers found");
    } else {
        const char *plural = (numResults == 2) ? "" : "s";
        Com_sprintf(uiInfo->foundPlayerServerNames[numResults - 1], 0x40, "%d server%s found with player %s", numResults - 1, plural, uiInfo->findPlayerName);
    }
    uiInfo->nextFindPlayerRefresh = 0;
}

static void UI_BuildServerStatus_impl(int force)
{
    int serverStatusVisible = uiInfo->nextFindPlayerRefresh;

    if (serverStatusVisible)
        return;

    if (force) {

        Menu_SetFeederSelection(uiInfo, 0, 0xd, 0, 0);
        sharedUiInfo.serverStatusInfo.numLines = 0;
        LAN_GetServerStatus(0, 0, 0);
    } else {
        int nextRefresh = sharedUiInfo.nextServerStatusRefresh;
        if (nextRefresh == 0)
            return;
        if (nextRefresh > uiInfo->uiDC.realTime)
            return;
    }

    {
        int serverCount = LAN_GetServerCount((ui_netSource)->current.integer);
        if (serverCount != sharedUiInfo.serverStatus.serverCount) {
            sharedUiInfo.serverStatus.serverCount = serverCount;
            if (sharedUiInfo.serverStatus.numDisplayServers != 0) {
                sharedUiInfo.serverStatus.currentServer = -1;
                UI_BuildServerDisplayList(1);
            }
        }
    }

    {
        int selectedServer = sharedUiInfo.serverStatus.currentServer;
        int numServers;

        if (selectedServer < 0)
            return;
        numServers = sharedUiInfo.serverStatus.numDisplayServers;
        if (selectedServer > numServers)
            return;
        if (numServers == 0)
            return;

        {
            int result;
            const char *serverAddress = sharedUiInfo.serverStatusAddress;
            serverStatusInfo_t *statusInfo = &sharedUiInfo.serverStatusInfo;

            result = UI_GetServerStatusInfo(serverAddress, statusInfo);
            if (result) {

                sharedUiInfo.nextServerStatusRefresh = 0;
                UI_GetServerStatusInfo(serverAddress, 0);
            } else {

                sharedUiInfo.nextServerStatusRefresh = uiInfo->uiDC.realTime + 500;
            }
        }
    }
}

static void UI_REGPARM1 UI_BuildServerStatus(int force)
{
    UI_BuildServerStatus_impl(force);
}

void UI_Refresh(void)
{
    int netSource;
    int needRebuild;
    static int refreshTraceCount;

    if (Menu_Count(uiInfo) <= 0)
        return;

    if (getenv("MTRACE") && refreshTraceCount < 80 && (uiInfo->currentMenuType || uiInfo->uiDC.openMenuCount > 0)) {
        int i;
        Com_Printf("[menu-trace] UI_Refresh active=%d menuCount=%d openCount=%d",
                   uiInfo->currentMenuType, uiInfo->uiDC.menuCount, uiInfo->uiDC.openMenuCount);
        for (i = 0; i < uiInfo->uiDC.openMenuCount && i < 16; ++i) {
            menuDef_t *menu = uiInfo->uiDC.menuStack[i];
            Com_Printf(" stack[%d]=%s", i, menu && menu->window.name ? menu->window.name : "<null>");
        }
        Com_Printf("\n");
        ++refreshTraceCount;
    }

    Menu_PaintAll(uiInfo);

    if (sharedUiInfo.serverStatus.refreshActive) {
        netSource = (ui_netSource)->current.integer;

        if (netSource == 2) {
            needRebuild = 0;
        } else if (netSource == 0) {
            needRebuild = LAN_GetServerCount(0) == 0;
        } else {
            needRebuild = LAN_WaitServerResponse(netSource) != 0;
        }

        if (uiInfo->uiDC.realTime < sharedUiInfo.serverStatus.refreshtime) {
            if (needRebuild)
                goto after_browser;
        }

        UI_UpdateServerCount();

        if (LAN_UpdateDirtyPings(netSource)) {
            sharedUiInfo.serverStatus.refreshtime = uiInfo->uiDC.realTime + 1000;
        } else {
            if (needRebuild)
                goto rebuild_browser;

            UI_BuildServerDisplayList(2);

            if (sharedUiInfo.serverStatus.refreshActive) {
                int listedServers = sharedUiInfo.serverStatus.numDisplayServers;
                int listedPlayers = sharedUiInfo.serverStatus.numPlayersOnServers;
                int filteredServers;

                sharedUiInfo.serverStatus.refreshActive = 0;
                Com_Printf("%d servers listed in browser with %d players.\n",
                           listedServers, listedPlayers);

                filteredServers = LAN_GetServerCount(netSource) - listedServers;
                if (filteredServers > 0) {
                    Com_Printf("%d servers not listed (filtered out by game browser settings)\n",
                               filteredServers);
                }
            }
        }

    rebuild_browser:
        UI_BuildServerDisplayList(0);
    }

after_browser:
    UI_BuildServerStatus(0);
    UI_BuildFindPlayerList();

#if COD2_FEATURE_GAMEPAD

    UI_ConsoleMode_DrawGlyphs(uiInfo);
    if (UI_ConsoleMode_SuppressCursor())
        return;
#endif

    UI_DrawHandlePic((float)(uiInfo->uiDC.cursorx - 16),
                     (float)(uiInfo->uiDC.cursory - 16),
                     32.0f, 32.0f, 4, 4, 0,
                     sharedUiInfo.assets.cursor);
}

void UI_RunMenuScript(const char **args)
{
    char name[1024];
    char buff[1024];
    char buff2[2048];
    char out[32];
    char addr[2048];
    char name2[1024];
    char testValue[1024];
    char dvarName[1024];
    char menuName[1024];
    int sortColumn;

    if (!String_Parse(args, name, 0x400))
        return;

    if (I_stricmp(name, "StartServer") == 0) {
#ifdef __EMSCRIPTEN__
        extern void Web_CreateServer(const char *name);
        Web_CreateServer(Dvar_GetString("sv_hostname"));
        return;
#endif
        Dvar_SetBoolByName("cg_thirdPerson", 0);
        Dvar_SetFromStringByNameFromSource("dedicated",
                                           va("%i", (ui_dedicated)->current.integer), 1);
        Dvar_SetStringByName("g_gametype",
                             sharedUiInfo.gameTypes[(ui_netGameType)->current.integer].gameType);
        {
            int mapIdx = (ui_currentNetMap)->current.integer;
            int offset = mapIdx * 41 + mapIdx;
            const char *mapName = *(const char **)((byte *)&sharedUiInfo.mapList[0].mapLoadName + offset * 4);
            Cbuf_ExecuteText(2, va("wait ; wait ; map %s\n", mapName));
        }
        return;
    }

    if (I_stricmp(name, "resetDefaults") == 0) {
        Cbuf_ExecuteText(0, "dvar_restart\n");
        Cbuf_ExecuteText(0, "exec default_mp.cfg\n");
        Cbuf_ExecuteText(0, "exec language.cfg\n");
        Cbuf_ExecuteText(0, "setRecommended\n");
        Controls_SetDefaults();
        Dvar_SetBoolByName("com_introPlayed", 1);
        Dvar_SetBoolByName("com_recommendedSet", 1);
        Cbuf_ExecuteText(2, "vid_restart\n");
        return;
    }

    if (I_stricmp(name, "getCDKey") == 0) {
        CLUI_GetCDKey(buff, 0x11, buff2, 5);
        Dvar_SetStringByName("cdkey1", "");
        Dvar_SetStringByName("cdkey2", "");
        Dvar_SetStringByName("cdkey3", "");
        Dvar_SetStringByName("cdkey4", "");
        Dvar_SetStringByName("cdkey5", "");
        {
            int len;

            for (len = 0; buff[len]; len++)
                ;
            if (len == 0x10) {

                I_strncpyz(out, buff, 5);
                Dvar_SetStringByName("cdkey1", out);
                I_strncpyz(out, &buff[4], 5);
                Dvar_SetStringByName("cdkey2", out);
                I_strncpyz(out, &buff[8], 5);
                Dvar_SetStringByName("cdkey3", out);
                I_strncpyz(out, &buff[12], 5);
                Dvar_SetStringByName("cdkey4", out);
            }
        }
        {
            int len2;
            for (len2 = 0; buff2[len2]; len2++)
                ;
            if (len2 == 4) {
                I_strncpyz(out, buff2, 5);
                Dvar_SetStringByName("cdkey5", out);
            }
        }
        return;
    }

    if (I_stricmp(name, "verifyCDKey") == 0) {
        buff[0] = '\0';
        I_strncat(buff, 0x400, Dvar_GetVariantString("cdkey1"));
        I_strncat(buff, 0x400, Dvar_GetVariantString("cdkey2"));
        I_strncat(buff, 0x400, Dvar_GetVariantString("cdkey3"));
        I_strncat(buff, 0x400, Dvar_GetVariantString("cdkey4"));
        buff2[0] = '\0';
        I_strncat(buff2, 0x400, Dvar_GetVariantString("cdkey5"));
        if (CL_CDKeyValidate(buff, buff2)) {
            Dvar_SetStringByName("ui_cdkeyvalid", UI_SafeTranslateString("EXE_CDKEYVALID"));
            CLUI_SetCDKey(buff, buff2);
        } else {
            Dvar_SetStringByName("ui_cdkeyvalid", UI_SafeTranslateString("EXE_CDKEYINVALID"));
        }
        return;
    }

    if (I_stricmp(name, "loadArenas") == 0) {
        int i;
        const char *curGameType;

        UI_LoadArenas();

        curGameType = Dvar_GetString("g_gametype");
        {
            int numGT = sharedUiInfo.numGameTypes;
            if (numGT > 0) {
                for (i = 0; i < numGT; i++) {
                    if (I_stricmp(curGameType, *(const char **)((byte *)&sharedUiInfo + 0x114c + i * 8)) == 0) {
                        Dvar_SetInt(ui_netGameType, i);
                        Dvar_SetString(ui_netGameTypeName, sharedUiInfo.gameTypes[i].gameType);
                        break;
                    }
                }
            }
        }

        {
            int gtIdx = (ui_netGameType)->current.integer;
            int numMaps = sharedUiInfo.mapCount;
            if (numMaps > 0) {
                for (i = 0; i < numMaps; i++) {
                    byte *entry = (byte *)&sharedUiInfo + i * 0xa4;
                    *(int *)(entry + 0x13f4) = 0;
                    if ((*(int *)(entry + 0x1368) >> gtIdx) & 1) {
                        *(int *)(entry + 0x13f4) = 1;
                    }
                }
            }
        }

        Menu_SetFeederSelection(uiInfo, 0, 4, 0, "createserver_maps");
        UI_SelectCurrentMap();
        return;
    }

    if (I_stricmp(name, "loadGameInfo") == 0) {
        UI_GetGameTypesList();
        return;
    }

    if (I_stricmp(name, "LoadMods") == 0) {
        int numFiles, i, modCount;
        char *namePtr;

        sharedUiInfo.modCount = 0;
        numFiles = FS_GetFileList("$modlist", "", 1, addr, 0x800);
        if (numFiles <= 0)
            return;

        namePtr = addr;
        modCount = sharedUiInfo.modCount;

        for (i = 0; i < numFiles; i++) {
            const char *modName;
            const char *modDesc;
            int nameLen, descLen;

            for (nameLen = 0; namePtr[nameLen]; nameLen++)
                ;
            nameLen++;

            modName = namePtr;
            modDesc = namePtr + nameLen;

            *(const char **)((byte *)&sharedUiInfo + 25972 + modCount * 8) = String_Alloc(modName);

            modCount = sharedUiInfo.modCount;
            *(const char **)((byte *)&sharedUiInfo + 25976 + modCount * 8) = String_Alloc(modDesc);

            for (descLen = 0; modDesc[descLen]; descLen++)
                ;
            namePtr = (char *)modDesc + descLen + 1;

            modCount = sharedUiInfo.modCount + 1;
            sharedUiInfo.modCount = modCount;
            if (modCount > 63)
                return;
        }
        return;
    }

    if (I_stricmp(name, "voteTypeMap") == 0) {
        int mapIdx = (ui_currentNetMap)->current.integer;
        int offset = mapIdx * 41 + mapIdx;
        const char *mapName = *(const char **)((byte *)&sharedUiInfo.mapList[0].mapLoadName + offset * 4);
        const char *gtName = sharedUiInfo.gameTypes[(ui_netGameType)->current.integer].gameType;
        Cbuf_ExecuteText(2, va("callvote typemap %s %s\n", gtName, mapName));
        return;
    }

    if (I_stricmp(name, "voteMap") == 0) {
        int mapIdx = (ui_currentNetMap)->current.integer;
        if (mapIdx < 0 || mapIdx >= sharedUiInfo.mapCount)
            return;
        {
            int offset = mapIdx * 41 + mapIdx;
            const char *mapName = *(const char **)((byte *)&sharedUiInfo.mapList[0].mapLoadName + offset * 4);
            Cbuf_ExecuteText(2, va("callvote map %s\n", mapName));
        }
        return;
    }

    if (I_stricmp(name, "voteGame") == 0) {
        const char *gtName = sharedUiInfo.gameTypes[(ui_netGameType)->current.integer].gameType;
        Cbuf_ExecuteText(2, va("callvote g_gametype %s\n", gtName));
        return;
    }

    if (I_stricmp(name, "saveControls") == 0) {
        Controls_SetConfig(1);
        return;
    }

    if (I_stricmp(name, "loadControls") == 0) {
        Controls_GetConfig();
        return;
    }

    if (I_stricmp(name, "clearError") == 0) {
        Dvar_SetStringByName("com_errorMessage", "");
        Dvar_SetBoolByName("com_isNotice", 0);
        return;
    }

    if (I_stricmp(name, "RefreshServers") == 0) {
        UI_StartServerRefresh(1);
        UI_BuildServerDisplayList(1);
        return;
    }

    if (I_stricmp(name, "RefreshFilter") == 0) {
        UI_StartServerRefresh(0);
        UI_BuildServerDisplayList(1);
        return;
    }

    if (I_stricmp(name, "addPlayerProfiles") == 0) {

        int numFiles, i;
        char **fileList;

        uiInfo->playerProfileCount = 0;
        uiInfo->playerProfileStatus.sortDir = 1;

        fileList = FS_ListFiles("players", "/", 1, &sortColumn, 0);

        if (sortColumn > 0) {
            for (i = 0; i < sortColumn; i++) {
                int profileIdx = uiInfo->playerProfileCount;
                uiInfo->playerProfileName[profileIdx] = String_Alloc(fileList[i]);
                uiInfo->playerProfileCount += 1;
            }
        }

        FS_FreeFileList(fileList, 0);

        {
            int numProfiles = uiInfo->playerProfileCount;
            if (numProfiles > 0) {
                for (i = 0; i < numProfiles; i++)
                    uiInfo->playerProfileStatus.displayProfile[i] = i;
                qsort(uiInfo->playerProfileStatus.displayProfile, numProfiles, 4, UI_PlayerProfilesQsortCompare);

                {
                    int numMenus = uiInfo->uiDC.openMenuCount;
                    for (i = numMenus - 1; i >= 0; i--) {
                        menuDef_t *menu = uiInfo->uiDC.menuStack[i];
                        if (*(byte *)&menu->window.dynamicFlags[0] & 4) {
                            Menu_SetFeederSelection(uiInfo, menu, 0x18, 0, 0);
                        }
                    }
                }
            }
        }

        Dvar_SetInt(ui_playerProfileCount, uiInfo->playerProfileCount);
        return;
    }

    if (I_stricmp(name, "sortPlayerProfiles") == 0) {

        int curSort = uiInfo->playerProfileStatus.sortDir;
        uiInfo->playerProfileStatus.sortDir = !curSort;

        {
            int numProfiles = uiInfo->playerProfileCount;
            if (numProfiles > 0) {
                int i;
                for (i = 0; i < numProfiles; i++)
                    uiInfo->playerProfileStatus.displayProfile[i] = i;
                qsort(uiInfo->playerProfileStatus.displayProfile, numProfiles, 4, UI_PlayerProfilesQsortCompare);

                {
                    int numMenus = uiInfo->uiDC.openMenuCount;
                    for (i = numMenus - 1; i >= 0; i--) {
                        menuDef_t *menu = uiInfo->uiDC.menuStack[i];
                        if (*(byte *)&menu->window.dynamicFlags[0] & 4) {
                            Menu_SetFeederSelection(uiInfo, menu, 0x18, 0, 0);
                        }
                    }
                }
            }
        }
        return;
    }

    if (I_stricmp(name, "selectActivePlayerProfile") == 0) {

        const char *curProfile = (*(dvar_t **)imp_com_playerProfile)->current.string;
        int numProfiles = uiInfo->playerProfileCount;
        int i, found = -1;

        for (i = 0; i < numProfiles; i++) {
            int sortedIdx = uiInfo->playerProfileStatus.displayProfile[i];
            if (I_stricmp(curProfile, uiInfo->playerProfileName[sortedIdx]) == 0) {
                found = i;
                break;
            }
        }
        if (found < 0)
            return;
        if (found >= numProfiles)
            return;

        {
            int numMenus = uiInfo->uiDC.openMenuCount;
            for (i = numMenus - 1; i >= 0; i--) {
                menuDef_t *menu = uiInfo->uiDC.menuStack[i];
                if (*(byte *)&menu->window.dynamicFlags[0] & 4) {
                    Menu_SetFeederSelection(uiInfo, menu, 0x18, found, 0);
                }
            }
        }
        return;
    }

    if (I_stricmp(name, "createPlayerProfile") == 0) {
        const char *newName = ui_playerProfileNameNew->current.string;
        int i, numProfiles;

        if (newName[0] == '\0')
            return;

        I_strncpyz(out, newName, 0x20);
        Dvar_SetString(ui_playerProfileNameNew, "");

        numProfiles = uiInfo->playerProfileCount;
        if (numProfiles >= 64) {
            Menus_OpenByName(uiInfo, "profile_create_too_many_popmenu");
            return;
        }

        for (i = 0; i < numProfiles; i++) {
            if (I_stricmp(out, uiInfo->playerProfileName[i]) == 0) {
                Menus_OpenByName(uiInfo, "profile_exists_popmenu");
                return;
            }
        }

        if (!Com_NewPlayerProfile(out)) {
            Menus_OpenByName(uiInfo, "profile_create_fail_popmenu");
            return;
        }

        {
            int idx = uiInfo->playerProfileCount;
            uiInfo->playerProfileName[idx] = String_Alloc(out);
            uiInfo->playerProfileCount += 1;
        }

        {
            int np = uiInfo->playerProfileCount;
            if (np > 0) {
                for (i = 0; i < np; i++)
                    uiInfo->playerProfileStatus.displayProfile[i] = i;
                qsort(uiInfo->playerProfileStatus.displayProfile, np, 4, UI_PlayerProfilesQsortCompare);
            }
        }

        Dvar_SetInt(ui_playerProfileCount, uiInfo->playerProfileCount);

        {
            int np = uiInfo->playerProfileCount;
            int found = -1;
            for (i = 0; i < np; i++) {
                int si = uiInfo->playerProfileStatus.displayProfile[i];
                if (I_stricmp(out, uiInfo->playerProfileName[si]) == 0) {
                    found = i;
                    break;
                }
            }

            {
                int numMenus = uiInfo->uiDC.openMenuCount;
                for (i = numMenus - 1; i >= 0; i--) {
                    menuDef_t *menu = uiInfo->uiDC.menuStack[i];
                    if (*(byte *)&menu->window.dynamicFlags[0] & 4) {
                        Menu_SetFeederSelection(uiInfo, menu, 0x18, found, 0);
                    }
                }
            }
        }
        return;
    }

    if (I_stricmp(name, "deletePlayerProfile") == 0) {
        int numProfiles = uiInfo->playerProfileCount;
        int sortedIdx, i;
        const char *selName;

        if (numProfiles == 0)
            return;

        selName = ui_playerProfileSelected->current.string;
        if (!Com_DeletePlayerProfile(selName)) {
            Menus_OpenByName(uiInfo, "profile_delete_fail_popmenu");
            return;
        }

        {
            int found = -1;
            for (i = 0; i < numProfiles; i++) {
                int si = uiInfo->playerProfileStatus.displayProfile[i];
                if (I_stricmp(selName, uiInfo->playerProfileName[si]) == 0) {
                    found = i;
                    sortedIdx = si;
                    break;
                }
            }
            if (found < 0) {
                found = -1;
                sortedIdx = *(int *)((byte *)uiInfo + 0x388 - 4);
            }

            uiInfo->playerProfileCount -= 1;
            numProfiles = uiInfo->playerProfileCount;

            if (numProfiles == 0) {
                Dvar_SetString(ui_playerProfileSelected, "");
            } else {

                uiInfo->playerProfileName[sortedIdx] =
                    uiInfo->playerProfileName[numProfiles];

                if (found >= numProfiles)
                    found = numProfiles - 1;

                if (numProfiles > 0) {
                    for (i = 0; i < numProfiles; i++)
                        uiInfo->playerProfileStatus.displayProfile[i] = i;
                    qsort(uiInfo->playerProfileStatus.displayProfile, numProfiles, 4, UI_PlayerProfilesQsortCompare);
                }

                {
                    int numMenus = uiInfo->uiDC.openMenuCount;
                    for (i = numMenus - 1; i >= 0; i--) {
                        menuDef_t *menu = uiInfo->uiDC.menuStack[i];
                        if (*(byte *)&menu->window.dynamicFlags[0] & 4) {
                            Menu_SetFeederSelection(uiInfo, menu, 0x18, found, 0);
                        }
                    }
                }
            }
        }

        Dvar_SetInt(ui_playerProfileCount, uiInfo->playerProfileCount);
        return;
    }

    if (I_stricmp(name, "loadPlayerProfile") == 0) {
        const char *selProfile = ui_playerProfileSelected->current.string;
        if (selProfile[0] == '\0')
            return;
        Com_ChangePlayerProfile(selProfile);
        return;
    }

    if (I_stricmp(name, "LoadMovies") == 0) {
        int numMovies, i;
        char *filePtr;


        numMovies = FS_GetFileList("video", "roq", 0, addr, 0x1000);
        sharedUiInfo.movieCount = numMovies;
        if (numMovies == 0)
            return;

        if (numMovies > 256)
            numMovies = 256;
        sharedUiInfo.movieCount = numMovies;

        filePtr = addr;
        for (i = 0; i < numMovies; i++) {
            int len;
            char *end;

            for (len = 0; filePtr[len]; len++)
                ;
            end = filePtr + len;

            if (I_stricmp(end - 4, ".roq") == 0)
                *(end - 4) = '\0';

            I_strupr(filePtr);
            *(const char **)((byte *)&sharedUiInfo + 0x677c + i * 4) = String_Alloc(filePtr);

            filePtr = end + 1;
        }
        return;
    }

    if (I_stricmp(name, "playMovie") == 0) {
        int cinHandle = sharedUiInfo.previewMovie;
        if (cinHandle >= 0)
            CIN_StopCinematic(cinHandle);
        Cbuf_ExecuteText(2, va("cinematic %s 2\n",
                               sharedUiInfo.movieList[sharedUiInfo.movieIndex]));
        return;
    }

    if (I_stricmp(name, "RunMod") == 0) {
        Dvar_SetStringByName("fs_game",
                             *(const char **)((byte *)&sharedUiInfo + 25972 + sharedUiInfo.modIndex * 8));
        Cbuf_ExecuteText(2, "vid_restart;");
        return;
    }

    if (I_stricmp(name, "closeJoin") == 0) {
        int refreshing = sharedUiInfo.serverStatus.refreshActive;
        if (refreshing) {

            sharedUiInfo.serverStatus.refreshActive = 0;
            Com_Printf("%d servers listed in browser with %d players.\n",
                       sharedUiInfo.serverStatus.numDisplayServers, sharedUiInfo.serverStatus.numPlayersOnServers);
            {
                int filtered = LAN_GetServerCount((ui_netSource)->current.integer) -
                               sharedUiInfo.serverStatus.numDisplayServers;
                if (filtered > 0)
                    Com_Printf("%d servers not listed (filtered out by game browser settings)\n", filtered);
            }
            sharedUiInfo.serverStatus.nextDisplayRefresh = 0;
            sharedUiInfo.nextServerStatusRefresh = 0;
            uiInfo->nextFindPlayerRefresh = 0;
            UI_BuildServerDisplayList(1);
        } else {
            Menus_CloseByName(uiInfo, "joinserver");
            Menus_OpenByName(uiInfo, "main");
        }
        return;
    }

    if (I_stricmp(name, "StopRefresh") == 0) {
        int refreshing = sharedUiInfo.serverStatus.refreshActive;
        if (refreshing) {
            sharedUiInfo.serverStatus.refreshActive = 0;
            Com_Printf("%d servers listed in browser with %d players.\n",
                       sharedUiInfo.serverStatus.numDisplayServers, sharedUiInfo.serverStatus.numPlayersOnServers);
            {
                int filtered = LAN_GetServerCount((ui_netSource)->current.integer) -
                               sharedUiInfo.serverStatus.numDisplayServers;
                if (filtered > 0)
                    Com_Printf("%d servers not listed (filtered out by game browser settings)\n", filtered);
            }
        }
        sharedUiInfo.serverStatus.nextDisplayRefresh = 0;
        sharedUiInfo.nextServerStatusRefresh = 0;
        uiInfo->nextFindPlayerRefresh = 0;
        return;
    }

    if (I_stricmp(name, "ServerStatus") == 0) {

        UI_UpdateServerCount();

        {
            int selectedServer = sharedUiInfo.serverStatus.currentServer;
            if (selectedServer < 0 || selectedServer >= sharedUiInfo.serverStatus.numDisplayServers)
                return;

            LAN_GetServerAddressString((ui_netSource)->current.integer,
                                       sharedUiInfo.serverStatus.displayServers[selectedServer],
                                       sharedUiInfo.serverStatusAddress, 0x40);
#ifdef __EMSCRIPTEN__
            Menu_SetFeederSelection(uiInfo, 0, 0xd, 0, 0);
            sharedUiInfo.serverStatusInfo.numLines = 0;
            sharedUiInfo.nextServerStatusRefresh = uiInfo->uiDC.realTime + 500;
#else
            UI_BuildServerStatus(1);
#endif
        }
        return;
    }

    if (I_stricmp(name, "UpdateFilter") == 0) {
        if ((ui_netSource)->current.integer == 0) {
            UI_StartServerRefresh(1);
        }
        UI_BuildServerDisplayList(1);
        UI_FeederSelection(2.0f, 0);
        return;
    }

    if (I_stricmp(name, "JoinServer") == 0) {
        Dvar_SetBoolByName("cg_thirdPerson", 0);

        UI_UpdateServerCount();

        {
            int selectedServer = sharedUiInfo.serverStatus.currentServer;
            if (selectedServer < 0 || selectedServer >= sharedUiInfo.serverStatus.numDisplayServers)
                return;

            LAN_GetServerAddressString((ui_netSource)->current.integer,
                                       sharedUiInfo.serverStatus.displayServers[selectedServer],
                                       buff, 0x400);
            Cbuf_ExecuteText(2, va("connect %s\n", buff));
        }
        return;
    }

    if (I_stricmp(name, "Quit") == 0) {
        Cbuf_ExecuteText(0, "quit");
        return;
    }

    if (I_stricmp(name, "Controls") == 0) {
        Dvar_SetIntByName("cl_paused", 1);
        Key_SetCatcher(8);
        Menus_CloseAll(uiInfo);
        Menus_OpenByName(uiInfo, "setup_menu2");
        return;
    }

    if (I_stricmp(name, "Leave") == 0) {
        Cbuf_ExecuteText(2, "disconnect\n");
        Key_SetCatcher(8);
        Menus_CloseAll(uiInfo);
        Menus_OpenByName(uiInfo, "main");
        return;
    }

    if (I_stricmp(name, "ServerSort") == 0) {
        if (!Int_Parse(args, &sortColumn))
            return;

        if (sharedUiInfo.serverStatus.sortKey == sortColumn) {
            int dir = sharedUiInfo.serverStatus.sortDir;
            sharedUiInfo.serverStatus.sortDir = !dir;
        }

        sharedUiInfo.serverStatus.sortKey = sortColumn;
        qsort(sharedUiInfo.serverStatus.displayServers,
              sharedUiInfo.serverStatus.numDisplayServers, 4, UI_ServersQsortCompare);
        return;
    }

    if (I_stricmp(name, "nextSkirmish") == 0)
        return;

    if (I_stricmp(name, "SkirmishStart") == 0)
        return;

    if (I_stricmp(name, "closeingame") == 0) {
        Key_SetCatcher(Key_GetCatcher() & ~8);
        Key_ClearStates();
        Dvar_SetIntByName("cl_paused", 0);
        Menus_CloseAll(uiInfo);
        return;
    }

    if (I_stricmp(name, "voteKick") == 0) {
        int sel = uiInfo->playerIndex;
        if (sel < 0 || sel >= sharedUiInfo.playerCount)
            return;
        Cbuf_ExecuteText(2, va("callvote kick \"%s\"\n",
                               sharedUiInfo.playerNames[sel]));
        return;
    }

    if (I_stricmp(name, "voteTempBan") == 0) {
        int sel = uiInfo->playerIndex;
        if (sel < 0 || sel >= sharedUiInfo.playerCount)
            return;
        Cbuf_ExecuteText(2, va("callvote tempBanUser \"%s\"\n",
                               sharedUiInfo.playerNames[sel]));
        return;
    }

    if (I_stricmp(name, "addFavorite") == 0) {
        if ((ui_netSource)->current.integer == 2)
            return;

        addr[0] = '\0';
        out[0] = '\0';

        UI_UpdateServerCount();

        {
            int selectedServer = sharedUiInfo.serverStatus.currentServer;
            if (selectedServer >= 0 && selectedServer < sharedUiInfo.serverStatus.numDisplayServers) {

                LAN_GetServerInfo((ui_netSource)->current.integer,
                                  sharedUiInfo.serverStatus.displayServers[selectedServer],
                                  buff, 0x400);
                I_strncpyz(out, Info_ValueForKey(buff, "hostname"), 0x20);
                I_strncpyz(addr, Info_ValueForKey(buff, "addr"), 0x20);
            }
            UI_AddServerToFavoritesList(out, addr);
        }
        return;
    }

    if (I_stricmp(name, "deleteFavorite") == 0) {
        if ((ui_netSource)->current.integer != 2)
            return;

        {
            int selectedServer = sharedUiInfo.serverStatus.currentServer;
            if (selectedServer < 0 || selectedServer >= sharedUiInfo.serverStatus.numDisplayServers)
                return;

            UI_UpdateServerCount();

            LAN_GetServerInfo((ui_netSource)->current.integer,
                              sharedUiInfo.serverStatus.displayServers[selectedServer],
                              buff, 0x400);
            addr[0] = '\0';
            I_strncpyz(addr, Info_ValueForKey(buff, "addr"), 0x20);
            if (addr[0] == '\0')
                return;
            LAN_RemoveServer(2, addr);
        }
        return;
    }

    if (I_stricmp(name, "createFavorite") == 0) {
        if ((ui_netSource)->current.integer != 2)
            return;

        out[0] = '\0';
        addr[0] = '\0';
        I_strncpyz(addr, Dvar_GetVariantString("ui_favoriteName"), 0x20);
        I_strncpyz(out, Dvar_GetVariantString("ui_favoriteAddress"), 0x20);
        UI_AddServerToFavoritesList(addr, out);
        return;
    }

    if (I_stricmp(name, "update") == 0) {
        if (!String_Parse(args, name2, 0x400))
            return;

        if (I_stricmp(name2, "ui_SetName") == 0) {
            Dvar_SetStringByName("name", Dvar_GetVariantString("ui_Name"));
            return;
        }
        if (I_stricmp(name2, "ui_GetName") == 0) {
            Dvar_SetStringByName("ui_Name", Dvar_GetVariantString("name"));
            return;
        }
        if (I_stricmp(name2, "ui_setRate") == 0) {
            int rate = Dvar_GetInt("rate");
            float frate = (float)rate;
            if (frate >= 5000.0f) {
                Dvar_SetIntByName("cl_maxpackets", 30);
                Dvar_SetIntByName("cl_packetdup", 1);
            } else if (frate >= 4000.0f) {
                Dvar_SetIntByName("cl_maxpackets", 15);
                Dvar_SetIntByName("cl_packetdup", 2);
            } else {
                Dvar_SetIntByName("cl_maxpackets", 15);
                Dvar_SetIntByName("cl_packetdup", 1);
            }
            return;
        }
        if (I_stricmp(name2, "ui_mousePitch") == 0) {
            if (Dvar_GetBool(name2)) {
                Dvar_SetFloatByName("m_pitch", -0.022f);
            } else {
                Dvar_SetFloatByName("m_pitch", 0.022f);
            }
            return;
        }
        return;
    }

    if (I_stricmp(name, "startSingleplayer") == 0) {
        Cbuf_ExecuteText(2, "startSingleplayer\n");
        return;
    }

    if (I_stricmp(name, "getLanguage") == 0) {
        Dvar_SetIntByName("ui_language", Dvar_GetInt("loc_language"));
        UI_VerifyLanguage();
        return;
    }

    if (I_stricmp(name, "verifyLanguage") == 0) {
        UI_VerifyLanguage();
        return;
    }

    if (I_stricmp(name, "updateLanguage") == 0) {
        Dvar_SetIntByName("loc_language", Dvar_GetInt("ui_language"));
        UI_VerifyLanguage();
        Cbuf_ExecuteText(2, "vid_restart\n");
        return;
    }

    if (I_stricmp(name, "mutePlayer") == 0) {
        int sel = uiInfo->playerIndex;
        if (sel < 0 || sel >= sharedUiInfo.playerCount)
            return;
        CL_MutePlayer(sharedUiInfo.playerClientNums[sel]);
        return;
    }

    if (I_stricmp(name, "openMenuOnDvar") == 0 || I_stricmp(name, "openMenuOnDvarNot") == 0) {
        int wantMatch = (I_stricmp(name, "openMenuOnDvar") == 0);
        if (!UI_GetOpenOrCloseMenuOnDvarArgs(args, name, dvarName, testValue, menuName))
            return;

        if (!Dvar_FindVar(dvarName)) {
            Com_Printf("%s: cannot find dvar %s\n", name, dvarName);
            return;
        }

        {
            int matches = (I_stricmp(testValue, Dvar_GetVariantString(dvarName)) == 0);
            if (matches == wantMatch) {
                Menus_OpenByName(uiInfo, menuName);
            }
        }
        return;
    }

    if (I_stricmp(name, "closeMenuOnDvar") == 0 || I_stricmp(name, "closeMenuOnDvarNot") == 0) {
        int wantMatch = (I_stricmp(name, "closeMenuOnDvar") == 0);
        if (!UI_GetOpenOrCloseMenuOnDvarArgs(args, name, dvarName, testValue, menuName))
            return;

        if (!Dvar_FindVar(dvarName)) {
            Com_Printf("%s: cannot find dvar %s\n", name, dvarName);
            return;
        }

        {
            int matches = (I_stricmp(testValue, Dvar_GetVariantString(dvarName)) == 0);
            if (matches == wantMatch) {
                Menus_CloseByName(uiInfo, menuName);
            }
        }
        return;
    }

    Com_Printf("unknown UI script %s\n", name);
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4

int UI_FeederCount(float feederID)
{
    if (feederID == 4.0f) {

        int gameType = (ui_netGameType)->current.integer;
        int numMaps = sharedUiInfo.mapCount;
        int count = 0;
        int i;

        if (numMaps <= 0)
            return 0;

        for (i = 0; i < numMaps; i++) {
            byte *entry = (byte *)&sharedUiInfo + i * 0xa4;
            *(int *)(entry + 0x13f4) = 0;
            if ((*(int *)(entry + 0x1368) >> gameType) & 1) {
                count++;
                *(int *)(entry + 0x13f4) = 1;
            }
        }
        return count;
    } else if (feederID == 9.0f) {
        return sharedUiInfo.modCount;
    } else if (feederID == 2.0f) {

        int serverCount = LAN_GetServerCount((ui_netSource)->current.integer);
        if (serverCount != sharedUiInfo.serverStatus.serverCount) {
            sharedUiInfo.serverStatus.serverCount = serverCount;
            if (sharedUiInfo.serverStatus.numDisplayServers != 0) {
                sharedUiInfo.serverStatus.currentServer = -1;
                UI_BuildServerDisplayList(1);
            }
        }
        return sharedUiInfo.serverStatus.numDisplayServers;
    } else if (feederID == 13.0f) {
        return sharedUiInfo.serverStatusInfo.numLines;
    } else if (feederID == 7.0f || feederID == 20.0f) {

        int curTime = uiInfo->uiDC.realTime;
        if (curTime > uiInfo->playerRefresh) {
            uiInfo->playerRefresh = curTime + 3000;
            UI_BuildPlayerList();
        }
        return sharedUiInfo.playerCount;
    } else if (feederID == 24.0f) {
        return uiInfo->playerProfileCount;
    }
    return 0;
}

static void UI_DrawCenteredText(const char *text, FontHandle font, float scale, float y, const vec_t *color, int style)
{
    float actualScale = CL_NormalizedTextScale(font, scale);
    int pixelWidth = CL_TextWidth(text, 0, font);
    int scaledWidth = (int)((float)pixelWidth * actualScale);
    float x = 320.0f - (float)(scaledWidth / 2);
    UI_DrawText(text, 0x7fffffff, font, x, y, 0, 0, scale, color, style);
}

static void UI_ReadableSize_wrap(char *buf, int bufsize, int value)
{
    UI_ReadableSize(buf, bufsize, value);
}

static void UI_DisplayDownloadInfo(const char *downloadName, float centerPoint, float yStart, FontHandle font, float scale)
{
    LegacyHacks *legacyBase = *(LegacyHacks **)imp_legacyHacks;
    int downloadSize = legacyBase->cl_downloadSize;
    int downloadCount = legacyBase->cl_downloadCount;
    int downloadTime = legacyBase->cl_downloadTime;
    vec_t color[4];
    float y1, y2, y3;
    int xferRate = 0;
    char dlSizeBuf[64];
    char totalSizeBuf[64];
    char xferRateBuf[64];
    char dlTimeBuf[64];
    const char *text;

    {
        const vec_t *cBlack = (const vec_t *)imp_colorBlack;
        color[0] = cBlack[0];
        color[1] = cBlack[1];
        color[2] = cBlack[2];
    }
    color[3] = 0.2f;

    y1 = yStart + 184.0f;
    UI_FillRect(0, y1, 640.0f, 85.0f, 0, 0, color);
    y2 = yStart + 185.0f;
    UI_FillRect(0, y2, 640.0f, 83.0f, 0, 0, color);
    y3 = yStart + 186.0f;
    UI_FillRect(0, y3, 640.0f, 81.0f, 0, 0, color);

    if (downloadSize > 0) {

        const vec_t *cRed = (const vec_t *)imp_colorRed;
        int width;
        color[0] = cRed[0];
        color[1] = cRed[1];
        color[2] = cRed[2];
        color[3] = 0.15f;

        width = (int)((float)downloadCount / (float)downloadSize * 640.0f);
        UI_FillRect(0, y1, (float)(width + 2), 85.0f, 0, 0, color);
        UI_FillRect(0, y2, (float)(width + 1), 83.0f, 0, 0, color);
        UI_FillRect(0, y3, (float)width, 81.0f, 0, 0, color);
    }

    {
        float yDl = yStart + 210.0f;
        float yEta = yStart + 235.0f;
        float yXfer = yStart + 260.0f;
        const vec_t *ltGrey = (const vec_t *)imp_colorLtGrey;

        UI_DrawText(UI_SafeTranslateString(dlText), 64, font, 24.0f, yDl, 0, 0, scale, ltGrey, 3);
        UI_DrawText(UI_SafeTranslateString(etaText), 64, font, 24.0f, yEta, 0, 0, scale, ltGrey, 3);
        UI_DrawText(UI_SafeTranslateString(xferText), 64, font, 24.0f, yXfer, 0, 0, scale, ltGrey, 3);

        if (downloadSize > 0)
            text = va("%s (%d%%)", downloadName, downloadCount * 100 / downloadSize);
        else
            text = downloadName;

        UI_DrawText(text, 0x7fffffff, font, 192.0f, yDl, 0, 0, scale, ltGrey, 3);

        UI_ReadableSize_wrap(dlSizeBuf, 64, downloadCount);
        UI_ReadableSize_wrap(totalSizeBuf, 64, downloadSize);

        if (downloadCount > 0xfff && downloadTime != 0) {

            int elapsedMs = uiInfo->uiDC.realTime - downloadTime;

            if ((unsigned)(elapsedMs + 999) <= 1998) {

                xferRate = 0;
                UI_ReadableSize_wrap(xferRateBuf, 64, 0);
            } else {

                int elapsedSec = elapsedMs / 1000;
                xferRate = downloadCount / elapsedSec;
                UI_ReadableSize_wrap(xferRateBuf, 64, xferRate);
            }

            if (downloadSize == 0 || xferRate == 0) {

                goto estimating;
            }

            {

                int totalSec = downloadSize / xferRate;
                int dlCountK = downloadCount >> 10;
                int dlSizeK;
                int timeleft;
                int sum;
                int ti;
                int curTle = tleIndex;

                if (downloadSize < 0)
                    dlSizeK = (downloadSize + 1023) >> 10;
                else
                    dlSizeK = downloadSize >> 10;

                tleEstimates[curTle] = totalSec - (dlCountK * totalSec / dlSizeK);

                {
                    int nextIdx = curTle + 1;
                    tleIndex = (nextIdx < 80) ? nextIdx : 0;
                }

                sum = 0;
                for (ti = 0; ti < 80; ti++)
                    sum += tleEstimates[ti];
                timeleft = sum / 80;

                if (timeleft > 3600) {

                    int hours = timeleft / 3600;
                    int remainder = timeleft - hours * 3600;
                    int minutes = remainder / 60;
                    Com_sprintf(dlTimeBuf, 64, "%d %s %d %s", hours, UI_SafeTranslateString("EXE_HOURS"),
                                minutes, UI_SafeTranslateString("EXE_MINUTES"));
                } else if (timeleft > 60) {

                    int minutes = timeleft / 60;
                    int secs = timeleft - minutes * 60;
                    Com_sprintf(dlTimeBuf, 64, "%d %s %d %s", minutes, UI_SafeTranslateString("EXE_MINUTES"),
                                secs, UI_SafeTranslateString("EXE_SECONDS"));
                } else {

                    Com_sprintf(dlTimeBuf, 64, "%d %s", timeleft, UI_SafeTranslateString("EXE_SECONDS"));
                }

                UI_DrawText(dlTimeBuf, 0x7fffffff, font, 264.0f, yEta, 0, 0, scale, ltGrey, 3);

                {
                    const char *copiedStr = UI_SafeTranslateString("EXE_COPIED");
                    const char *ofStr = UI_SafeTranslateString("EXE_OF");
                    text = va("(%s %s %s %s)", dlSizeBuf, ofStr, totalSizeBuf, copiedStr);
                    UI_DrawCenteredText(text, font, scale, yStart + 320.0f, ltGrey, 6);
                }
            }
        } else {

        estimating:
            text = UI_SafeTranslateString("EXE_ESTIMATING");
            UI_DrawCenteredText(text, font, scale, yEta, ltGrey, 6);

            if (downloadSize != 0) {

                const char *copiedStr = UI_SafeTranslateString("EXE_COPIED");
                const char *ofStr = UI_SafeTranslateString("EXE_OF");
                text = va("(%s %s %s %s)", dlSizeBuf, ofStr, totalSizeBuf, copiedStr);
            } else {

                const char *copiedStr = UI_SafeTranslateString("EXE_COPIED");
                text = va("(%s %s)", dlSizeBuf, copiedStr);
            }
            UI_DrawCenteredText(text, font, scale, yStart + 320.0f, ltGrey, 6);

            if (xferRate == 0)
                return;
        }

        {
            const char *secStr = UI_SafeTranslateString("EXE_SECONDS");
            text = va("%s/%s", xferRateBuf, secStr);
            UI_DrawText(text, 0x7fffffff, font, 200.0f, yXfer, 0, 0, scale, ltGrey, 3);
        }
    }
}

void UI_DrawConnectScreen(void)
{
    byte *legacyBase;
    FontHandle font;
    uiClientState_t cstate;
    int bConnectInfoDisplayed;
    const char *pszGameType;
    const char *mapDisplayName;
    const float connectScale = 0.5f;

    legacyBase = *(byte **)imp_legacyHacks;
    if (!legacyBase)
        return;

    {
        clientActive_t *cl = *(clientActive_t **)imp_cl;
        int loading = 0;
        if ((*(unsigned char *)&((LegacyHacks *)legacyBase)->cl_serverloadmap[0]) != 0 || (*(unsigned char *)&((LegacyHacks *)legacyBase)->cl_serverloadgametype[0]) != 0)
            loading = 1;

        (void)cl;
        CG_DrawInformation(loading);
    }

    font = UI_GetFontHandle(0, connectScale);

    GetClientState(&cstate);

    if (g_mapname[0] != '\0') {

        int numGameTypes = sharedUiInfo.numGameTypes;
        pszGameType = g_gametype;

        if (numGameTypes > 0) {
            int gi;
            byte *base = (byte *)&sharedUiInfo;
            for (gi = 0; gi < numGameTypes; gi++) {
                if (I_stricmp(g_gametype, sharedUiInfo.gameTypes[gi].gameTypeName) == 0) {   /* was base+0x114c+gi*8 (x86) */
                    pszGameType = sharedUiInfo.gameTypes[gi].gameTypeName;
                    break;
                }
            }
        }

        UI_DrawCenteredText(UI_SafeTranslateString(pszGameType), font, connectScale, 89.0f, (const vec_t *)imp_colorWhite, 6);

        {
            int numMaps = sharedUiInfo.mapCount;
            int mi;
            mapDisplayName = g_mapname;
            for (mi = 0; mi < numMaps; mi++) {
                if (I_stricmp(g_mapname, sharedUiInfo.mapList[mi].mapName) == 0)   /* was &sharedUiInfo+mi*0xa4+0x1358 (x86) */
                    break;
            }
        }

        UI_DrawCenteredText(mapDisplayName, font, connectScale, 119.0f, (const vec_t *)imp_colorWhite, 6);

        bConnectInfoDisplayed = 1;

        if (cstate.connState <= CA_CHALLENGING) {
            goto draw_tips;
        }
    } else {
        bConnectInfoDisplayed = 0;

        if (cstate.connState <= CA_CHALLENGING) {
            goto draw_tips;
        }
    }

    goto check_connection_state;

draw_tips:

check_connection_state: {
    int cs = cstate.connState;

    if (cs == CA_CHALLENGING) {

        if (bConnectInfoDisplayed)
            return;

        {

            const char *translated;
            char tempString[64];
            int convArgs[10];
            int ci;

            translated = UI_SafeTranslateString("EXE_AWAITINGCHALLENGE");

            for (ci = 0; ci < 10; ci++)
                convArgs[ci] = bConnectInfoDisplayed;

            sprintf(tempString, "%d", cstate.connectPacketCount);
            convArgs[0] = 1;
            convArgs[1] = (int)tempString;

            translated = UI_ReplaceConversions(translated, (ConversionArguments *)convArgs);
            if (!translated)
                return;

            {
                if (I_stricmp(cstate.servername, "localhost") == 0)
                    return;
            }

            UI_DrawCenteredText(translated, font, connectScale, 145.0f, (const vec_t *)imp_colorWhite, 6);
        }
    } else if (cs == CA_CONNECTED) {

        legacyBase = *(byte **)imp_legacyHacks;
        if (!legacyBase || (*(unsigned char *)&((LegacyHacks *)legacyBase)->cl_downloadName[0]) == 0)
            return;
        UI_DisplayDownloadInfo(((LegacyHacks *)legacyBase)->cl_downloadName, 320.0f, 89.0f, font, connectScale);
    } else if (cs == CA_CONNECTING) {

        if (bConnectInfoDisplayed)
            return;

        {
            const char *translated;
            char tempString[64];
            int convArgs[10];
            int ci;

            translated = UI_SafeTranslateString("EXE_AWAITINGCONNECTION");

            for (ci = 0; ci < 10; ci++)
                convArgs[ci] = bConnectInfoDisplayed;

            sprintf(tempString, "%d", cstate.connectPacketCount);
            convArgs[0] = 1;
            convArgs[1] = (int)tempString;

            translated = UI_ReplaceConversions(translated, (ConversionArguments *)convArgs);
            if (!translated)
                return;

            {
                if (I_stricmp(cstate.servername, "localhost") == 0)
                    return;
            }

            UI_DrawCenteredText(translated, font, connectScale, 145.0f, (const vec_t *)imp_colorWhite, 6);
        }
    }
}
}
#else
static int UI_PlayerProfilesQsortCompare(const void *arg1, const void *arg2)
{
    int idx1 = *(const int *)arg1;
    int idx2 = *(const int *)arg2;
    int result;

    if (idx1 == idx2)
        return 0;

    result = I_stricmp(
        uiInfo->playerProfileName[idx1],
        uiInfo->playerProfileName[idx2]);

    if (uiInfo->playerProfileStatus.sortDir == 0)
        result = -result;

    return result;
}

static void UI_SelectCurrentMap(void)
{
    byte cstate[0x844];
    char szMap[0x40];
    int numMaps;
    int i;
    int visibleIndex;

    GetClientState(cstate);
    if (*(int *)cstate != 8)
        return;

    {
        const char *serverInfo = CL_GetConfigString(0);
        if (!serverInfo[0])
            return;
        I_strncpyz(szMap, Info_ValueForKey(serverInfo, "mapname"), sizeof(szMap));
    }

    numMaps = sharedUiInfo.mapCount;
    if (numMaps <= 0)
        return;

    visibleIndex = 0;
    for (i = 0; i < numMaps; i++) {
        byte *entry = (byte *)&sharedUiInfo + i * 0xa4;

        if (!*(int *)(entry + 0x13f4))
            continue;

        if (I_stricmp(szMap, *(const char **)(entry + 0x1354)) == 0) {
            Menu_SetFeederSelection(uiInfo, 0, 4, visibleIndex, "createserver_maps");
            return;
        }

        visibleIndex++;
    }
}

static void UI_VerifyLanguage(void)
{
    int oldLanguage = Dvar_GetInt("loc_language");
    int newLanguage = Dvar_GetInt("ui_language");
    int verified = SEH_VerifyLanguageSelection(newLanguage);

    if (verified != newLanguage)
        Dvar_SetIntByName("ui_language", verified);

    Dvar_SetBoolByName("ui_languagechanged", oldLanguage != newLanguage);
}

static void UI_AddServerToFavoritesList(const char *name, const char *address)
{
    const char *message;
    const char *dvarMessage;
    int addResult;

    if (!name[0]) {
        message = UI_SafeTranslateString("EXE_FAVORITENAMEEMPTY");
        Com_Printf("%s\n", message);
        Dvar_SetStringByName("ui_favorite_message", "@EXE_FAVORITENAMEEMPTY");
        return;
    }

    if (!address[0]) {
        message = UI_SafeTranslateString("EXE_FAVORITEADDRESSEMPTY");
        Com_Printf("%s\n", message);
        Dvar_SetStringByName("ui_favorite_message", "@EXE_FAVORITEADDRESSEMPTY");
        return;
    }

    addResult = LAN_AddServer(2, name, address);
    if (addResult == 0) {
        message = UI_SafeTranslateString("EXE_FAVORITEINLIST");
        dvarMessage = "@EXE_FAVORITEINLIST";
    } else if (addResult == -1) {
        message = UI_SafeTranslateString("EXE_FAVORITELISTFULL");
        dvarMessage = "@EXE_FAVORITELISTFULL";
    } else if (addResult == -2) {
        message = UI_SafeTranslateString("EXE_BADSERVERADDRESS");
        dvarMessage = "@EXE_BADSERVERADDRESS";
    } else {
        message = UI_SafeTranslateString("EXE_FAVORITEADDED");
        dvarMessage = "@EXE_FAVORITEADDED";
    }

    Com_Printf("%s\n", message);
    Dvar_SetStringByName("ui_favorite_message", dvarMessage);
}

static Bool UI_GetOpenOrCloseMenuOnDvarArgs(const char **args, const char *cmd,
                                            char *dvarName, char *testValue, char *menuName)
{
    if (!String_Parse(args, dvarName, 0x400)) {
        Com_Printf("%s: invalid dvar name.\n", cmd);
        return 0;
    }

    if (!String_Parse(args, testValue, 0x400)) {
        Com_Printf("%s: invalid test value.\n", cmd);
        return 0;
    }

    if (!String_Parse(args, menuName, 0x400)) {
        Com_Printf("%s: invalid menu name.\n", cmd);
        return 0;
    }

    return 1;
}

static void UI_UpdateServerCount(void)
{
    int serverCount = LAN_GetServerCount((ui_netSource)->current.integer);
    if (serverCount != sharedUiInfo.serverStatus.serverCount) {
        sharedUiInfo.serverStatus.serverCount = serverCount;
        if (sharedUiInfo.serverStatus.numDisplayServers != 0) {
            sharedUiInfo.serverStatus.currentServer = -1;
            UI_BuildServerDisplayList(1);
        }
    }
}

static void UI_StartServerRefresh(qboolean full)
{
    qtime_t q;
    char dvarName[0x18];
    int source;
    int now;

    Com_RealTime(&q);
    source = (ui_netSource)->current.integer;
    snprintf(dvarName, sizeof(dvarName), "ui_lastServerRefresh_%i", source);
    Dvar_SetStringByName(dvarName,
                         va("%s %i, %i   %i:%02i",
                            UI_SafeTranslateString(MonthAbbrev[q.tm_mon]),
                            q.tm_mday,
                            q.tm_year + 1900,
                            q.tm_hour,
                            q.tm_min));

    now = uiInfo->uiDC.realTime;

    if (!full) {
        LAN_ResetPings(source);
        sharedUiInfo.serverStatus.refreshActive = 1;
        sharedUiInfo.serverStatus.refreshtime = now + 1000;
        return;
    }

    sharedUiInfo.serverStatus.refreshActive = 1;
    sharedUiInfo.serverStatus.nextDisplayRefresh = now + 1000;
    sharedUiInfo.serverStatus.numDisplayServers = 0;
    sharedUiInfo.serverStatus.numPlayersOnServers = 0;
    sharedUiInfo.serverStatus.serverCount = LAN_GetServerCount(source);
    LAN_MarkServerDirty(source, -1, 1);
    LAN_ResetPings(source);

    if (source == 0) {
        Cbuf_ExecuteText(0, "localservers\n");
        sharedUiInfo.serverStatus.refreshtime = now + 1000;
        return;
    }

    sharedUiInfo.serverStatus.refreshtime = now + 5000;

    if (source == 1) {
        const char *debugProtocol = Dvar_GetVariantString("debug_protocol");
        if (debugProtocol[0])
            Cbuf_ExecuteText(0, va("globalservers %d %s full empty\n", 0, debugProtocol));
        else
            Cbuf_ExecuteText(0, va("globalservers %d %d full empty\n", 0, 0x73));
    }
}

const char *UI_SafeTranslateString(const char *reference)
{
    const char *ref = reference;
    const char *translated;

    if (ref[0] == 0x15)
        ref++;

    translated = SEH_StringEd_GetString(ref);
    if (translated)
        return translated;

    if ((loc_warnings)->current.enabled) {
        if ((loc_warningsAsErrors)->current.enabled) {
            Com_Error(6, "Could not translate string \"%s\"", ref);
        } else {
            Com_Printf("^3WARNING: Could not translate string \"%s\"\n", ref);
        }

        strcpy(errorString, "^1UNLOCALIZED(^7");
        I_strncat(errorString, 0x400, ref);
        I_strncat(errorString, 0x400, "^1)^7");
        return errorString;
    }

    strcpy(errorString, ref);
    return errorString;
}

void UI_FeederSelection(float feederID, int index)
{
    if (feederID == 4.0f) {
        int mapVal = (ui_currentNetMap)->current.integer;
        int offset = mapVal * 164;
        int cinHandle = *(int *)((byte *)&sharedUiInfo + 4972 + offset);
        int numMaps, visCount, actual, i;

        if (cinHandle >= 0) {
            CIN_StopCinematic(cinHandle);
            *(int *)((byte *)&sharedUiInfo + 4960 + 12 + offset) = -1;
        }

        numMaps = sharedUiInfo.mapCount;
        visCount = 0;
        actual = 0;
        for (i = 0; i < numMaps; i++) {
            if (sharedUiInfo.mapList[i].active != 0) {
                if (visCount == index) {
                    actual = i;
                    goto map_found;
                }
                visCount++;
            }
        }
        actual = 0;
    map_found:
        Dvar_SetInt(ui_currentMap, actual);
        Dvar_SetInt(ui_currentNetMap, actual);

    } else if (feederID == 2.0f) {
        char *name;
        char *p;

        if (sharedUiInfo.serverStatus.numDisplayServers >= 1)
            sharedUiInfo.serverStatus.currentServer = index;

        LAN_GetServerInfo((ui_netSource)->current.integer,
                          sharedUiInfo.serverStatus.displayServers[index],
                          info, 0x400);
        name = (char *)va("levelshots/%s", Info_ValueForKey(info, "mapname"));

        for (p = name; *p; p++)
            *p = ___tolower((int)(signed char)*p);

        sharedUiInfo.serverStatus.currentServerPreview = CL_RegisterMaterialNoMip(name, 3);
        if (sharedUiInfo.serverStatus.currentServerCinematic >= 0) {
            CIN_StopCinematic(sharedUiInfo.serverStatus.currentServerCinematic);
            sharedUiInfo.serverStatus.currentServerCinematic = -1;
        }

    } else if (feederID == 7.0f || feederID == 20.0f) {
        uiInfo->playerIndex = index;

    } else if (feederID == 9.0f) {
        sharedUiInfo.modIndex = index;

    } else if (feederID == 24.0f) {
        if (index >= 0 && index < uiInfo->playerProfileCount) {
            int sortedIdx = uiInfo->playerProfileStatus.displayProfile[index];
            const char *profileName = uiInfo->playerProfileName[sortedIdx];
            Dvar_SetString(ui_playerProfileSelected, profileName);
        }
    }
}

static void UI_GetGameTypesList(void)
{
    char szGameTypeList[0x1000];
    int iNumGameTypeScripts;
    int i;
    char *pszFileName;
    const char *pBuffParse;

    sharedUiInfo.numGameTypes = 0;
    sharedUiInfo.numJoinGameTypes = 0;

    *(const char **)((byte *)&sharedUiInfo + 4688) = String_Alloc("All");
    {
        int idx = sharedUiInfo.numJoinGameTypes;
        sharedUiInfo.joinGameTypes[idx].gameTypeName = "";
    }
    sharedUiInfo.numJoinGameTypes += 1;

    iNumGameTypeScripts = FS_GetFileList("maps/mp/gametypes", "gsc", 0, szGameTypeList, 0x1000);

    if (iNumGameTypeScripts > 0) {
        pszFileName = szGameTypeList;

        for (i = 0; i < iNumGameTypeScripts; i++) {
            int iLen = strlen(pszFileName);
            char *pszEnd;
            int numTypes;

            if (pszFileName[0] == '_') {
                pszFileName += iLen + 1;
                continue;
            }

            pszEnd = pszFileName + iLen;
            if (I_stricmp(pszEnd - 4, ".gsc") == 0) {
                pszFileName[iLen - 4] = '\0';
            }

            numTypes = sharedUiInfo.numGameTypes;
            if (numTypes == 32 || sharedUiInfo.numJoinGameTypes == 32) {
                Com_Printf("Too many game type scripts found! Only loading the first %i\n", 31);
                break;
            }

            sharedUiInfo.gameTypes[numTypes].gameType = String_Alloc(pszFileName);

            {
                int gt = sharedUiInfo.numGameTypes;
                const char *scriptName = sharedUiInfo.gameTypes[gt].gameType;
                int nameIdx = sharedUiInfo.numJoinGameTypes;
                sharedUiInfo.joinGameTypes[nameIdx].gameType = scriptName;
            }

            pBuffParse = GetMenuBuffer(va("maps/mp/gametypes/%s.txt", pszFileName));
            if (pBuffParse) {
                const char *parsed = Com_Parse(&pBuffParse);
                int gt2 = sharedUiInfo.numGameTypes;
                sharedUiInfo.gameTypes[gt2].gameTypeName = String_Alloc(parsed);
                {
                    const char *displayName = sharedUiInfo.gameTypes[sharedUiInfo.numGameTypes].gameTypeName;
                    int nameIdx2 = sharedUiInfo.numJoinGameTypes;
                    sharedUiInfo.joinGameTypes[nameIdx2].gameTypeName = displayName;
                }
            } else {
                int gt3 = sharedUiInfo.numGameTypes;
                *(const char **)((byte *)&sharedUiInfo + 4432 + gt3 * 8) = *(const char **)((byte *)&sharedUiInfo + 4428 + gt3 * 8);
                {
                    const char *displayName2 = sharedUiInfo.gameTypes[sharedUiInfo.numGameTypes].gameTypeName;
                    int nameIdx3 = sharedUiInfo.numJoinGameTypes;
                    sharedUiInfo.joinGameTypes[nameIdx3].gameTypeName = displayName2;
                }
            }

            sharedUiInfo.numGameTypes += 1;
            sharedUiInfo.numJoinGameTypes += 1;
            pszFileName = pszEnd + 1;
        }
    }

    if (sharedUiInfo.numGameTypes == 0) {
        Com_Error(0, "\x15No game type scripts found in maps/mp/gametypes folder");
    }
}

void UI_Init(void)
{
    byte *legacyBase;
    int width, height;
    MenuList *menuList;
    int netGameTypeIdx;
    float mPitch;

    *(int *)uiInfo = 0;
    g_ingameMenusLoaded = 0;

    UI_LoadSoundAliases();

    Dvar_RegisterBool_mac("g_allowvote", 1, 0x1001);
    Dvar_RegisterBool_mac("cg_brass", 1, 0x1001);
    Dvar_RegisterBool_mac("cg_marks", 1, 0x1001);

    Dvar_RegisterString_mac("server1", "", 0x1001);
    Dvar_RegisterString_mac("server2", "", 0x1001);
    Dvar_RegisterString_mac("server3", "", 0x1001);
    Dvar_RegisterString_mac("server4", "", 0x1001);
    Dvar_RegisterString_mac("server5", "", 0x1001);
    Dvar_RegisterString_mac("server6", "", 0x1001);
    Dvar_RegisterString_mac("server7", "", 0x1001);
    Dvar_RegisterString_mac("server8", "", 0x1001);
    Dvar_RegisterString_mac("server9", "", 0x1001);
    Dvar_RegisterString_mac("server10", "", 0x1001);
    Dvar_RegisterString_mac("server11", "", 0x1001);
    Dvar_RegisterString_mac("server12", "", 0x1001);
    Dvar_RegisterString_mac("server13", "", 0x1001);
    Dvar_RegisterString_mac("server14", "", 0x1001);
    Dvar_RegisterString_mac("server15", "", 0x1001);
    Dvar_RegisterString_mac("server16", "", 0x1001);

    ui_netSource = Dvar_RegisterInt("ui_netSource", 0, 0, 2, 0x1001);
    ui_smallFont = Dvar_RegisterFloat("ui_smallFont", 0.25f, 0.0f, 1.0f, 0x1001);
    ui_bigFont = Dvar_RegisterFloat("ui_bigFont", 0.4f, 0.0f, 1.0f, 0x1001);
    ui_extraBigFont = Dvar_RegisterFloat("ui_extraBigFont", 0.55f, 0.0f, 1.0f, 0x1001);
    ui_currentMap = Dvar_RegisterInt("ui_currentMap", 0, 0, 0x7fffffff, 0x1001);
    ui_gametype = Dvar_RegisterInt("ui_gametype", 3, 0, 0x7fffffff, 0x1001);
    ui_joinGameType = Dvar_RegisterInt("ui_joinGametype", 0, 0, 0x7fffffff, 0x1001);
    ui_netGameTypeName = Dvar_RegisterString_mac("ui_netGametypeName", "", 0x1001);
    ui_dedicated = Dvar_RegisterInt("ui_dedicated", 0, 0, 2, 0x1001);
    ui_currentNetMap = Dvar_RegisterInt("ui_currentNetMap", 0, 0, 0x7fffffff, 0x1001);
    ui_browserShowFull = Dvar_RegisterBool_mac("ui_browserShowFull", 1, 0x1001);
    ui_browserShowEmpty = Dvar_RegisterBool_mac("ui_browserShowEmpty", 1, 0x1001);
    ui_browserShowPassword = Dvar_RegisterBool_mac("ui_browserShowPassword", 1, 0x1001);
    ui_browserShowNoPassword = Dvar_RegisterBool_mac("ui_browserShowNoPassword", 1, 0x1001);
    ui_browserShowPure = Dvar_RegisterBool_mac("ui_browserShowPure", 1, 0x1001);
    ui_browserShowDedicated = Dvar_RegisterBool_mac("ui_browserShowDedicated", 0, 0x1001);
#    if COD2_IS_PATCH_13
    ui_browserShowPunkBuster = Dvar_RegisterInt("ui_browserShowPunkBuster", -1, (int)0x80000000, 0x7fffffff, 0x1001);
#    endif
    ui_browserMod = Dvar_RegisterInt("ui_browserMod", -1, -1, 0x7fffffff, 0x1001);
    ui_browserFriendlyfire = Dvar_RegisterInt("ui_browserFriendlyfire", -1, (int)0x80000000, 0x7fffffff, 0x1001);
    ui_browserKillcam = Dvar_RegisterInt("ui_browserKillcam", -1, (int)0x80000000, 0x7fffffff, 0x1001);
    ui_serverStatusTimeOut = Dvar_RegisterInt("ui_serverStatusTimeOut", 7000, 0, 0x7fffffff, 0x1001);
    ui_playerProfileCount = Dvar_RegisterInt("ui_playerProfileCount", 0, (int)0x80000000, 0x7fffffff, 0x1040);
    ui_playerProfileSelected = Dvar_RegisterString_mac("ui_playerProfileSelected", "", 0x1040);
    ui_playerProfileNameNew = Dvar_RegisterString_mac("ui_playerProfileNameNew", "", 0x1000);

    legacyBase = (byte *)imp_legacyHacksArray;
    (*(byte *)&((LegacyHacks *)legacyBase)->ui_newScriptMenu[0]) = 0;
    ((LegacyHacks *)legacyBase)->ui_newScriptMenuIndex = -1;
    (*(byte *)&((LegacyHacks *)legacyBase)->ui_scriptMenu[0]) = 0;
    ((LegacyHacks *)legacyBase)->ui_scriptMenuIndex = -1;
    ((LegacyHacks *)legacyBase)->ui_scriptMenuAllowResponse = 1;
    (*(byte *)&((LegacyHacks *)legacyBase)->ui_waitingScriptMenu[0]) = 0;
    ((LegacyHacks *)legacyBase)->ui_waitingScriptMenuIndex = -1;
    ((LegacyHacks *)legacyBase)->ui_waitingScriptMenuNoMouse = 0;

    String_Init();
    Menu_Setup(uiInfo);

    CL_GetScreenDimensions(&uiInfo->uiDC.screenWidth, &uiInfo->uiDC.screenHeight, (int *)&uiInfo->uiDC.screenAspect);

    width = uiInfo->uiDC.screenWidth;
    height = uiInfo->uiDC.screenHeight;

    if (width * 480 > height * 640) {
        *(float *)uiInfo = ((float)width + (float)height * -1.3333333730697632f) * 0.5f;
    } else {
        *(int *)uiInfo = 0;
    }

    Sys_Milliseconds();
    UI_GetGameTypesList();

    ui_netGameType = Dvar_RegisterInt("ui_netGametype", 0, 0, sharedUiInfo.numGameTypes - 1, 0x1001);

    UI_LoadArenas();

    menuList = UI_LoadMenus("ui_mp/menus.txt", 3);
    UI_AddMenuList(uiInfo, menuList);
    UI_LoadIngameMenus();

    if (g_mapname[0] != '\0') {
        UI_MapLoadInfo(va("maps/mp/%s.csv", g_mapname));
    }

    UI_AssetCache();
    Menus_CloseAll(uiInfo);

    *(int *)((byte *)&sharedUiInfo + 25940) = CL_RegisterMaterialNoMip("server_hardware_unknown", 3);
    *(int *)((byte *)&sharedUiInfo + 25944) = CL_RegisterMaterialNoMip("server_hardware_linux_dedicated", 3);
    *(int *)((byte *)&sharedUiInfo + 25948) = CL_RegisterMaterialNoMip("server_hardware_win_dedicated", 3);
    *(int *)((byte *)&sharedUiInfo + 25952) = CL_RegisterMaterialNoMip("server_hardware_mac_dedicated", 3);
    *(int *)((byte *)&sharedUiInfo + 25960) = CL_RegisterMaterialNoMip("server_hardware_win_listen", 3);
    *(int *)((byte *)&sharedUiInfo + 25964) = CL_RegisterMaterialNoMip("server_hardware_mac_listen", 3);

    LAN_LoadCachedServers();

    if (sharedUiInfo.serverStatus.sortKey != 9) {
        sharedUiInfo.serverStatus.sortKey = 9;
        qsort(sharedUiInfo.serverStatus.displayServers, sharedUiInfo.serverStatus.numDisplayServers, 4, UI_ServersQsortCompare);
    }

    mPitch = Dvar_GetFloat("m_pitch");
    Dvar_SetBoolByName("ui_mousePitch", mPitch < 0.0f ? 1 : 0);

    sharedUiInfo.serverStatus.currentServerCinematic = -1;
    sharedUiInfo.previewMovie = -1;

    netGameTypeIdx = (ui_netGameType)->current.integer;
    Dvar_SetString(ui_netGameTypeName, sharedUiInfo.gameTypes[netGameTypeIdx].gameType);

    Dvar_RegisterBool_mac("ui_multiplayer", 1, 0x1040);
}

void UI_KeyEvent(int key, qboolean down)
{
    UI_KeyEvent_impl(key, down);
}

static void UI_BuildPlayerList(void)
{
    char cs[0x844];
    char name[0x20];
    int count;
    int n;

    GetClientState(cs);
    count = atoi(Info_ValueForKey(CL_GetConfigString(0), "sv_maxclients"));
    memset(sharedUiInfo.playerClientNums, -1, 0x100);
    sharedUiInfo.playerCount = 0;

    for (n = 0; n < count; n++) {
        int playerCount;

        if (!GetClientname(n, name, 0x20))
            continue;

        playerCount = sharedUiInfo.playerCount;
        sharedUiInfo.playerClientNums[playerCount] = n;
        I_strncpyz(sharedUiInfo.playerNames[playerCount], name, 0x20);
        I_CleanStr(sharedUiInfo.playerNames[playerCount]);
        sharedUiInfo.playerCount = playerCount + 1;
    }
}

void UI_LoadIngameMenus(void)
{
    if (g_ingameMenusLoaded)
        return;

    g_ingameMenusLoaded = 1;

    Load_ScriptMenu("ingame", 3);
    Load_ScriptMenu("callvote", 3);
    Load_ScriptMenu("muteplayer", 3);
    Load_ScriptMenu("quickcommands", 3);
    Load_ScriptMenu("quickresponses", 3);
    Load_ScriptMenu("quickstatements", 3);
    Load_ScriptMenu("serverinfo_dm", 3);
    Load_ScriptMenu("serverinfo_tdm", 3);
    Load_ScriptMenu("serverinfo_ctf", 3);
    Load_ScriptMenu("serverinfo_hq", 3);
    Load_ScriptMenu("serverinfo_sd", 3);
    Load_ScriptMenu("team_americangerman", 3);
    Load_ScriptMenu("team_britishgerman", 3);
    Load_ScriptMenu("team_russiangerman", 3);
    Load_ScriptMenu("weapon_american", 3);
    Load_ScriptMenu("weapon_british", 3);
    Load_ScriptMenu("weapon_german", 3);
    Load_ScriptMenu("weapon_russian", 3);
}

static int UI_ServersQsortCompare(const void *arg1, const void *arg2)
{
    int s1 = *(const int *)arg1;
    int s2 = *(const int *)arg2;

    return LAN_CompareServers(
        (ui_netSource)->current.integer,
        sharedUiInfo.serverStatus.sortKey,
        sharedUiInfo.serverStatus.sortDir,
        s1, s2);
}

static void UI_InsertServerAtPosition(int serverIndex, int position)
{
    int numServers = sharedUiInfo.serverStatus.numDisplayServers;
    int *displayServers = sharedUiInfo.serverStatus.displayServers;
    int *selectedServer = &sharedUiInfo.serverStatus.currentServer;
    int j;

    if (position < 0 || numServers < position)
        return;

    if (*selectedServer >= position && numServers != 0)
        *selectedServer += 1;

    numServers++;
    sharedUiInfo.serverStatus.numDisplayServers = numServers;

    for (j = numServers - 1; j > position; j--)
        displayServers[j] = displayServers[j - 1];

    displayServers[position] = serverIndex;
}

static void UI_RemoveDuplicateFromFavorites(int serverIndex)
{
    int numServers = sharedUiInfo.serverStatus.numDisplayServers;
    int *displayServers = sharedUiInfo.serverStatus.displayServers;
    int i, j;

    if (numServers <= 0)
        return;

    for (i = 0; i < numServers; i++) {
        if (displayServers[i] == serverIndex)
            break;
    }
    if (i >= numServers)
        return;

    numServers--;
    sharedUiInfo.serverStatus.numDisplayServers = numServers;

    for (j = i; j < numServers; j++)
        displayServers[j] = displayServers[j + 1];
}

static void UI_BinaryInsertServer(int serverIndex)
{
    int numDisplay = sharedUiInfo.serverStatus.numDisplayServers;
    int *displayServers = sharedUiInfo.serverStatus.displayServers;
    int lo = 0;
    int hi = numDisplay;
    int position = 0;
    int lastCmp = 0;
    int source = (ui_netSource)->current.integer;
    int sortKey = sharedUiInfo.serverStatus.sortKey;
    int sortDir = sharedUiInfo.serverStatus.sortDir;

    while (hi > 0) {
        int mid = hi / 2;
        int testIdx = lo + mid;
        int cmp = LAN_CompareServers(source, sortKey, sortDir, serverIndex, displayServers[testIdx]);

        lastCmp = cmp;
        if (cmp == 0) {
            UI_InsertServerAtPosition(serverIndex, testIdx);
            return;
        }
        if (cmp > 0) {
            hi -= mid;
            position = testIdx + 1;
            lo = position;
            hi--;
        } else {
            hi -= mid;
        }
    }

    if (lastCmp >= 0)
        position = lo + 1;
    else
        position = lo;

    UI_InsertServerAtPosition(serverIndex, position);
}

static void UI_BuildServerDisplayList(qboolean force)
{
    char info_buf[0x400];
    int count, i, clients, ping;
    int netSource;

    if (!force) {
        if (uiInfo->uiDC.realTime <= sharedUiInfo.serverStatus.nextDisplayRefresh)
            return;
    }

    clients = 0;

    {
        const char *motdString = Dvar_GetString("cl_motdString");
        int motdLen;

        I_strncpyz(sharedUiInfo.serverStatus.motd, motdString, 0x400);
        motdLen = strlen(sharedUiInfo.serverStatus.motd);
        if (motdLen == 0) {
            const char *translated = UI_SafeTranslateString("EXE_COD_MULTIPLAYER");
            strcpy(sharedUiInfo.serverStatus.motd, va("%s - %s", translated, "1.0"));
            motdLen = strlen(sharedUiInfo.serverStatus.motd);
        }
        if (motdLen != sharedUiInfo.serverStatus.motdLen) {
            sharedUiInfo.serverStatus.motdLen = motdLen;
            sharedUiInfo.serverStatus.motdWidth = -1;
        }
    }

    if (force) {
        numclean = 0;
        sharedUiInfo.serverStatus.numDisplayServers = 0;
        sharedUiInfo.serverStatus.numPlayersOnServers = 0;
        netSource = (ui_netSource)->current.integer;
        sharedUiInfo.serverStatus.serverCount = LAN_GetServerCount(netSource);

        if (sharedUiInfo.serverStatus.currentServer >= 0)
            Menu_SetFeederSelection(uiInfo, 0, 2, 0, 0);

        LAN_MarkServerDirty((ui_netSource)->current.integer, -1, 1);
    }

    netSource = (ui_netSource)->current.integer;
    count = LAN_GetServerCount(netSource);

    if (LAN_WaitServerResponse(netSource) || (netSource == 0 && count == 0)) {
        sharedUiInfo.serverStatus.numDisplayServers = 0;
        sharedUiInfo.serverStatus.numPlayersOnServers = 0;
        sharedUiInfo.serverStatus.serverCount = LAN_GetServerCount((ui_netSource)->current.integer);
        sharedUiInfo.serverStatus.nextDisplayRefresh = uiInfo->uiDC.realTime + 500;
        return;
    }

    qsort(sharedUiInfo.serverStatus.displayServers, sharedUiInfo.serverStatus.numDisplayServers, 4, UI_ServersQsortCompare);

    for (i = 0; i < count; i++) {
        if (!LAN_ServerIsDirty((ui_netSource)->current.integer, i))
            continue;

        ping = LAN_GetServerPing((ui_netSource)->current.integer, i);
        if (ping <= 0 && (ui_netSource)->current.integer != 2)
            continue;

        LAN_GetServerInfo((ui_netSource)->current.integer, i, info_buf, 0x400);
        clients = atoi(Info_ValueForKey(info_buf, "clients"));
        sharedUiInfo.serverStatus.numPlayersOnServers += clients;

        if (I_strnicmp(Info_ValueForKey(info_buf, "addr"), "000.000.000.000", 15) == 0)
            goto reject;
        if (!(ui_browserShowEmpty)->current.enabled && clients == 0)
            goto reject;
        if (!(ui_browserShowFull)->current.enabled &&
            clients == atoi(Info_ValueForKey(info_buf, "sv_maxclients")))
            goto reject;
        if (!(ui_browserShowPassword)->current.enabled &&
            atoi(Info_ValueForKey(info_buf, "pswrd")) != 0)
            goto reject;
        if (!(ui_browserShowNoPassword)->current.enabled &&
            atoi(Info_ValueForKey(info_buf, "pswrd")) == 0)
            goto reject;
        if ((ui_browserShowPure)->current.enabled &&
            atoi(Info_ValueForKey(info_buf, "pure")) == 0)
            goto reject;
        if ((ui_browserShowDedicated)->current.enabled &&
            (unsigned)(atoi(Info_ValueForKey(info_buf, "hw")) - 1) > 2)
            goto reject;
        if ((ui_browserMod)->current.integer >= 0 &&
            atoi(Info_ValueForKey(info_buf, "mod")) != (ui_browserMod)->current.integer)
            goto reject;
        if ((ui_browserFriendlyfire)->current.integer >= 0 &&
            atoi(Info_ValueForKey(info_buf, "ff")) != (ui_browserFriendlyfire)->current.integer)
            goto reject;
        if ((ui_browserKillcam)->current.integer >= 0 &&
            atoi(Info_ValueForKey(info_buf, "kc")) != (ui_browserKillcam)->current.integer)
            goto reject;

        {
            int joinGTIdx = (ui_joinGameType)->current.integer;
            const char *joinGTName = sharedUiInfo.joinGameTypes[joinGTIdx].gameTypeName;
            if (joinGTName[0] != '\0') {
                const char *joinGTShort = sharedUiInfo.joinGameTypes[joinGTIdx].gameType;
                if (I_stricmp(Info_ValueForKey(info_buf, "gametype"), joinGTShort) != 0)
                    goto reject;
            }
        }

        if (ui_serverFilterType > 0) {
            const char *filterBaseName = *(const char **)((byte *)serverFilters + ui_serverFilterType * 8 + 4);
            if (I_stricmp(Info_ValueForKey(info_buf, "game"), filterBaseName) != 0)
                goto reject;
        }

        if ((ui_netSource)->current.integer == 2)
            UI_RemoveDuplicateFromFavorites(i);

        UI_BinaryInsertServer(i);

        if (ping > 0) {
            LAN_MarkServerDirty((ui_netSource)->current.integer, i, 0);
            numclean++;
        }
        continue;

    reject:
        LAN_MarkServerDirty((ui_netSource)->current.integer, i, 0);
    }

    sharedUiInfo.serverStatus.refreshtime = uiInfo->uiDC.realTime;
}

int UI_FeederCount(float feederID)
{
    if (feederID == 4.0f) {
        int gameType = (ui_netGameType)->current.integer;
        int numMaps = sharedUiInfo.mapCount;
        int count = 0;
        int i;

        if (numMaps <= 0)
            return 0;

        for (i = 0; i < numMaps; i++) {
            byte *entry = (byte *)&sharedUiInfo + i * 0xa4;
            *(int *)(entry + 0x13f4) = 0;
            if ((*(int *)(entry + 0x1368) >> gameType) & 1) {
                count++;
                *(int *)(entry + 0x13f4) = 1;
            }
        }
        return count;
    }

    if (feederID == 9.0f)
        return sharedUiInfo.modCount;

    if (feederID == 2.0f) {
        int serverCount = LAN_GetServerCount((ui_netSource)->current.integer);
        if (serverCount != sharedUiInfo.serverStatus.serverCount) {
            sharedUiInfo.serverStatus.serverCount = serverCount;
            if (sharedUiInfo.serverStatus.numDisplayServers != 0) {
                sharedUiInfo.serverStatus.currentServer = -1;
                UI_BuildServerDisplayList(1);
            }
        }
        return sharedUiInfo.serverStatus.numDisplayServers;
    }

    if (feederID == 13.0f)
        return sharedUiInfo.serverStatusInfo.numLines;

    if (feederID == 7.0f || feederID == 20.0f) {
        int curTime = uiInfo->uiDC.realTime;
        if (curTime > uiInfo->playerRefresh) {
            uiInfo->playerRefresh = curTime + 3000;
            UI_BuildPlayerList();
        }
        return sharedUiInfo.playerCount;
    }

    if (feederID == 24.0f)
        return uiInfo->playerProfileCount;

    return 0;
}

static void UI_DrawMapPreview(const rectDef_t *rect, const vec_t *color, int net)
{
    int map;
    int mapCount;
    int material;

    if (net) {
        map = (ui_currentNetMap)->current.integer;
    } else {
        map = (ui_currentMap)->current.integer;
    }

    mapCount = *(int *)((char *)&sharedUiInfo + 4944);
    if (map < 0 || map >= mapCount) {

        if (net) {
            Dvar_SetInt(ui_currentNetMap, 0);
        } else {
            Dvar_SetInt(ui_currentMap, 0);
        }
        map = 0;
    }

    material = *(int *)((char *)&sharedUiInfo + 5104 + map * 164);

    if (!material) {
        material = CL_RegisterMaterialNoMip("menu/art/unknownmap", 3);
    }

    UI_DrawHandlePic(rect->x, rect->y, rect->w, rect->h,
                     rect->horzAlign, rect->vertAlign,
                     color, material);
}

void UI_SetMap(const char *mapname, const char *gametype)
{
    I_strncpyz(g_mapname, mapname, 0x40);
    I_strncpyz(g_gametype, gametype, 0x40);
    if (g_mapname[0]) {
        UI_MapLoadInfo(va("maps/mp/%s.csv", g_mapname));
    }
}

void UI_OpenMenu_f(void)
{
    Menus_OpenByName(uiInfo, Cmd_Args(1));
}

void UI_CloseMenu_f(void)
{
    Menus_CloseByName(uiInfo, Cmd_Args(1));
}

void UI_MouseEvent(int dx, int dy)
{
    int *cursorX = &uiInfo->uiDC.cursorx;
    int *cursorY = &uiInfo->uiDC.cursory;

    *cursorX += dx;
    if (*cursorX < 0)
        *cursorX = 0;
    else if (*cursorX > 640)
        *cursorX = 640;

    *cursorY += dy;
    if (*cursorY < 0)
        *cursorY = 0;
    else if (*cursorY > 480)
        *cursorY = 480;

    if (Menu_Count(uiInfo) > 0) {
        Display_MouseMove(uiInfo, 0, *cursorX, *cursorY);
    }
}

void UI_MouseEventAbsolute(int x, int y)
{
    int *cursorX = &uiInfo->uiDC.cursorx;
    int *cursorY = &uiInfo->uiDC.cursory;

    if (x < 0)
        x = 0;
    else if (x > 640)
        x = 640;

    if (y < 0)
        y = 0;
    else if (y > 480)
        y = 480;

    *cursorX = x;
    *cursorY = y;

    if (Menu_Count(uiInfo) > 0)
        Display_MouseMove(uiInfo, 0, *cursorX, *cursorY);
}

uiMenuCommand_t UI_GetActiveMenu(void)
{
    return uiInfo->currentMenuType;
}

qboolean UI_SetActiveMenu(int menu)
{
    byte *legacyBase;
    menuDef_t *pFocus;
    const char *errorMsg;

    if (Menu_Count(uiInfo) <= 0)
        return 0;

    if (menu != 9 && menu != 10)
        uiInfo->currentMenuType = menu;

    if ((unsigned int)menu > 11)
        return 0;

    switch (menu) {
    case 0:
        Key_SetCatcher(Key_GetCatcher() & ~8);
        Dvar_SetIntByName("cl_paused", 0);
        Menus_CloseAll(uiInfo);
        return 1;

    case 1:
        Key_SetCatcher(8);
        Menus_OpenByName(uiInfo, "main");
        errorMsg = Dvar_GetString("com_errorMessage");
        if (errorMsg[0] != '\0' && I_stricmp(errorMsg, ";") != 0) {
            Menus_OpenByName(uiInfo, "error_popmenu");
        }
        SND_FadeAllSounds(1.0f, 1000);
        return 1;

    case 2: {
        const char *cgMenuName = (*(cg_t **)imp_cg)->scriptMainMenu;
        Key_SetCatcher(8);
        Menus_CloseAll(uiInfo);
        if (!Menus_OpenByName(uiInfo, cgMenuName)) {
            Menus_OpenByName(uiInfo, "main");
        }
        return 1;
    }

    case 3:
        Key_SetCatcher(8);
        Menus_OpenByName(uiInfo, "needcd");
        return 1;

    case 4:
        Key_SetCatcher(8);
        Menus_OpenByName(uiInfo, "badcd");
        return 1;

    case 5:
        Key_SetCatcher(8);
        Menus_OpenByName(uiInfo, "team");
        return 1;

    case 6:
    case 7:
        return 0;

    case 8:
        uiInfo->uiDC.cursorx = 0x27f;
        uiInfo->uiDC.cursory = 0x1df;
        Key_SetCatcher(8);
        (*(clientActive_t **)imp_cl)->displayHUDWithKeycatchUI = 1;
        Menus_CloseAll(uiInfo);
        Menus_OpenByName(uiInfo, "quickmessage");
        return 1;

    case 9:
    case 10:
        pFocus = (menuDef_t *)Menu_GetFocused(uiInfo);
        if (pFocus) {
            int activeMenu = uiInfo->currentMenuType;
            if (activeMenu != 9 && activeMenu != 10)
                return 0;
        }

        legacyBase = *(byte **)imp_legacyHacks;
        if (pFocus) {
            if (I_stricmp(pFocus->window.name, ((LegacyHacks *)legacyBase)->ui_newScriptMenu) == 0)
                return 1;
        }

        uiInfo->currentMenuType = 9;

        if (menu == 10) {
            uiInfo->uiDC.cursorx = 0x27f;
            uiInfo->uiDC.cursory = 0x1df;
        }

        Key_SetCatcher(8);
        Menus_CloseAll(uiInfo);
        strcpy(((LegacyHacks *)legacyBase)->ui_scriptMenu, ((LegacyHacks *)legacyBase)->ui_newScriptMenu);
        ((LegacyHacks *)legacyBase)->ui_scriptMenuIndex = ((LegacyHacks *)legacyBase)->ui_newScriptMenuIndex;
        (*(byte *)&((LegacyHacks *)legacyBase)->ui_newScriptMenu[0]) = 0;
        ((LegacyHacks *)legacyBase)->ui_newScriptMenuIndex = -1;
        Menus_OpenByName(uiInfo, ((LegacyHacks *)legacyBase)->ui_scriptMenu);
        return 1;

    case 11:
        Key_SetCatcher(8);
        Menus_OpenByName(uiInfo, "player_profile");
        SND_FadeAllSounds(1.0f, 1000);
        return 1;
    }

    return 0;
}

qboolean UI_IsFullscreen(void)
{
    return Menus_AnyFullScreenVisible(uiInfo);
}

qboolean UI_AnyFullScreenMenuVisible(void)
{
    return Menus_AnyFullScreenVisible(uiInfo);
}

void UI_CloseAll(void)
{
    Menus_CloseAll(uiInfo);
}

void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style)
{
    float xScale;
    float yScale;

    diag_drawtext(text, (void *)font, x, y, scale);

    xScale = CL_NormalizedTextScale(font, scale);
    yScale = xScale;
    CalcSplitScreenTextOffset(font, &y);
    CalcScreenPlacement(&x, &y, &xScale, &yScale, horzAlign, vertAlign);

    x = (float)(int)floorf(x + 0.5f);
    y = (float)(int)floorf(y + 0.5f);

    CL_DrawTextPhysical(text, maxChars, font, x, y, xScale, yScale, color, style);
}
#endif

serverStatusDvar_t serverStatusDvars[23] = {
    { (const char *)&str_002a714c, (const char *)&str_002a9ab8, 0x0 },
    { (const char *)&str_002a9ad0, (const char *)&str_002a9ad8, 0x0 },
    { (const char *)&str_002a9af0, (const char *)&str_002a9af8, 0x1 },
    { (const char *)&str_002a9b10, (const char *)&str_002a9b1c, 0x0 },
    { (const char *)&str_002a7100, (const char *)&str_002a9b34, 0x0 },
    { (const char *)&str_002a71f0, (const char *)&str_002a9b4c, 0x1 },
    { (const char *)&str_002a7124, (const char *)&str_002a9b60, 0x0 },
    { (const char *)&str_002168fc, (const char *)&str_002a9b74, 0x0 },
    { (const char *)&str_002a7118, (const char *)&str_002a9b8c, 0x0 },
    { (const char *)&str_002a9ba4, (const char *)&str_002a9bb0, 0x0 },
    { (const char *)&str_002a9bc8, (const char *)&str_002a9bd4, 0x0 },
    { (const char *)&str_002a9bec, (const char *)&str_002a9bf8, 0x0 },
    { (const char *)&str_002a9c10, (const char *)&str_002a9c20, 0x1 },
    { (const char *)&str_002a9c3c, (const char *)&str_002a9c50, 0x0 },
    { (const char *)&str_002a70dc, (const char *)&str_002a9c68, 0x0 },
    { (const char *)&str_002a9c80, (const char *)&str_002a9c94, 0x0 },
    { (const char *)&str_002a9cb0, (const char *)&str_002a9cc4, 0x0 },
    { (const char *)&str_00216d64, (const char *)&str_002a9ce0, 0x0 },
    { (const char *)&str_002a9cf4, (const char *)&str_002a9cf8, 0x1 },
    { (const char *)&str_002a9d04, (const char *)&str_002a9d10, 0x1 },
    { (const char *)&str_002a9d28, (const char *)&str_002a9d34, 0x1 },
    { (const char *)&str_002a71c8, (const char *)&str_002a9d4c, 0x1 },
    { 0, 0, 0x0 }
};

const char str_002168fc[] = "shortversion";
const char str_00216d64[] = "fs_game";
const char str_002a70dc[] = "sv_maxclients";
const char str_002a7100[] = "g_gametype";
const char str_002a7118[] = "protocol";
const char str_002a7124[] = "mapname";
const char str_002a714c[] = "sv_hostname";
const char str_002a71c8[] = "sv_voice";
const char str_002a71f0[] = "sv_pure";
const char str_002a9ab8[] = "@EXE_SV_INFO_SERVERNAME";
const char str_002a9ad0[] = "address";
const char str_002a9ad8[] = "@EXE_SV_INFO_ADDRESS";
const char str_002a9af0[] = "pswrd";
const char str_002a9af8[] = "@EXE_SV_INFO_PASSWORD";
const char str_002a9b10[] = "gamename";
const char str_002a9b1c[] = "@EXE_SV_INFO_GAMENAME";
const char str_002a9b34[] = "@EXE_SV_INFO_GAMETYPE";
const char str_002a9b4c[] = "@EXE_SV_INFO_PURE";
const char str_002a9b60[] = "@EXE_SV_INFO_MAP";
const char str_002a9b74[] = "@EXE_SV_INFO_VERSION";
const char str_002a9b8c[] = "@EXE_SV_INFO_PROTOCOL";
const char str_002a9ba4[] = "sv_maxping";
const char str_002a9bb0[] = "@EXE_SV_INFO_MAXPING";
const char str_002a9bc8[] = "sv_minping";
const char str_002a9bd4[] = "@EXE_SV_INFO_MINPING";
const char str_002a9bec[] = "sv_maxrate";
const char str_002a9bf8[] = "@EXE_SV_INFO_MAXRATE";
const char str_002a9c10[] = "sv_floodprotect";
const char str_002a9c20[] = "@EXE_SV_INFO_FLOODPROTECT";
const char str_002a9c3c[] = "sv_allowanonymous";
const char str_002a9c50[] = "@EXE_SV_INFO_ALLOWANON";
const char str_002a9c68[] = "@EXE_SV_INFO_MAXCLIENTS";
const char str_002a9c80[] = "sv_privateclients";
const char str_002a9c94[] = "@EXE_SV_INFO_PRIVATECLIENTS";
const char str_002a9cb0[] = "scr_friendlyFire";
const char str_002a9cc4[] = "@EXE_SV_INFO_FRIENDLY_FIRE";
const char str_002a9ce0[] = "@EXE_SV_INFO_MOD";
const char str_002a9cf4[] = "mod";
const char str_002a9cf8[] = "@MENU_MODS";
const char str_002a9d04[] = "scr_killcam";
const char str_002a9d10[] = "@EXE_SV_INFO_KILLCAM";
const char str_002a9d28[] = "g_antilag";
const char str_002a9d34[] = "@EXE_SV_INFO_ANTILAG";
const char str_002a9d4c[] = "@EXE_SV_INFO_VOICE";
const char str_002a9d68[] = "EXE_MONTH_ABV_JANUARY";
const char str_002a9d80[] = "EXE_MONTH_ABV_FEBRUARY";
const char str_002a9d98[] = "EXE_MONTH_ABV_MARCH";
const char str_002a9dac[] = "EXE_MONTH_ABV_APRIL";
const char str_002a9dc0[] = "EXE_MONTH_ABV_MAY";
const char str_002a9dd4[] = "EXE_MONTH_ABV_JUN";
const char str_002a9de8[] = "EXE_MONTH_ABV_JULY";
const char str_002a9dfc[] = "EXE_MONTH_ABV_AUGUST";
const char str_002a9e14[] = "EXE_MONTH_ABV_SEPTEMBER";
const char str_002a9e2c[] = "EXE_MONTH_ABV_OCTOBER";
const char str_002a9e44[] = "EXE_MONTH_ABV_NOVEMBER";
const char str_002a9e5c[] = "EXE_MONTH_ABV_DECEMBER";
const char str_002a9e74[] = "EXE_LOCAL";
const char str_002a9e80[] = "EXE_INTERNET";
const char str_002a9e90[] = "EXE_FAVORITES";
