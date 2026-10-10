#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include "cod2_feature_config.h"
#include <stdarg.h>
#include <ctype.h>

extern char cl_cdkey[52];
extern char cl_cdkeychecksum[12];

extern void BG_ShutdownWeaponDefFiles(void);
extern void Com_ShutdownInternal(char *finalmsg);
extern void UI_SetMap(const char *a, const char *b);
extern void CL_StartHunkUsers(void);
extern void Com_ShutdownDObj(void);
extern void DObjShutdown(void);
extern void XAnimShutdown(void);
extern void CM_Shutdown(void);
extern void SND_ShutdownChannels(void);
extern void Hunk_Clear(void);
extern void Scr_Shutdown(void);
extern int Cmd_Argc(void);

extern const dvar_t *com_statmon;
extern const dvar_t *com_viewlog;
extern const dvar_t *com_developer;
extern const dvar_t *com_logfile;
extern const dvar_t *com_timescale;
extern float com_timescaleValue;
extern const dvar_t *com_sv_running;
extern const dvar_t *version;
extern const dvar_t *shortversion;
extern const dvar_t *nextmap;
extern const dvar_t *com_expectedHunkUsage;
extern const dvar_t *cl_paused;
extern const dvar_t *sv_paused;
extern int com_frameTime;
extern qboolean com_errorEntered;
extern qboolean com_fixedConsolePosition;
extern const dvar_t *com_developer_script;
extern const dvar_t *com_fixedtime;
extern const dvar_t *com_maxfps;
extern const dvar_t *com_introPlayed;
extern const dvar_t *com_recommendedSet;
extern const dvar_t *com_animCheck;
extern int com_frameNumber;
extern const dvar_t *ui_errorMessage;
extern const dvar_t *ui_errorTitle;
extern int com_numConsoleLines;
extern char *com_consoleLines[32];
static qboolean opening_qconsole;
static int printedWarning;
static int timeClientFrame;
static int lastErrorTime;
static int errorCount;
static errorParm_t errorcode;
static fileHandle_t logfile;
static int com_lastFrameTime;
static float com_codeTimeScale;
static qboolean com_fullyInitialized;
static char com_errorMessage[4096];
static int com_pushedEventsHead;
static int com_pushedEventsTail;
static sysEvent_t com_pushedEvents[256];
static qboolean com_safemode;
static int iWeaponInfoSource;
static char *rd_buffer;
static int rd_buffersize;
static void (*rd_flush)();

__attribute__((used, packed, aligned(4)))
const char *noticeErrors[] = {
    "EXE_SERVER_DISCONNECTED",
    "EXE_DISCONNECTED",
    "EXE_SERVERISFULL",
    "XBOXLIVE_SIGNEDOUTOFLIVE",
    "XBOXLIVE_CANTJOINSESSION",
    "XBOXLIVE_MPNOTALLOWED",
    "XBOXLIVE_MUSTLOGIN",
    NULL,
};

extern char *va(const char *format, ...);
extern void *Sys_GetValue(int valueIndex);
extern void Sys_Error(const char *error, ...);
#ifndef _WIN32
extern int setjmp(jmp_buf env);
extern void longjmp(jmp_buf env, int val);
#endif
extern dvar_t *com_dedicated;
extern int dvar_modifiedFlags;
extern int *com_fileAccessed;
extern void Dvar_ClearModified(const dvar_t *dvar);
extern void SetAnimCheck(int enabled);
extern void NET_Sleep(int msec);
extern int Com_EventLoop(void);
extern void Cbuf_Execute(void);
extern void SV_Frame(int msec);
extern int SV_FrameWaitMilliseconds(void);
extern void CL_SwitchToLocalClient(int localClientNum);
extern void CL_RunOncePerClientFrame(int msec);
extern void CL_Shutdown(void);
extern void CL_Frame(int msec);
extern void SCR_UpdateScreenInternal(void);
extern void SCR_RunCinematic(void);
extern void SND_UpdateLoopingSounds(void);
extern void SND_Update(void);
extern void StatMon_Warning(int type, int timeout, const char *msg);
extern void Sys_ShowConsole(int visLevel, qboolean quitOnClose);
extern void Sys_NormalExit(void);
extern void SV_AddDedicatedCommands(void);
extern qboolean Com_HasPlayerProfile(void);
extern int Com_BuildPlayerProfilePath(char *buf, int bufsize, const char *format, ...);

extern const dvar_t *Dvar_RegisterInt(const char *name, int value, int min, int max, int flags);
extern int Sys_Milliseconds(void);
extern float floorf(float x);
extern qboolean UI_SetActiveMenu(int menu);
extern int stricmp(const char *s1, const char *s2);
extern void Z_FreeInternal(void *ptr);
extern void FS_FCloseFile(fileHandle_t f);
extern int FS_FOpenFileWrite(const char *filename);
extern void FS_Printf(int f, const char *fmt, ...);
extern void Key_WriteBindings(int f);
extern void Dvar_WriteVariables(int f);
void Com_BeginRedirect(char *buffer, int buffersize, void (*flush)());
void Com_EndRedirect(void);
void Com_Printf(const char *fmt, ...);
void Com_PrintMessage(print_msg_type_t type, const char *msg);
void Com_DPrintf(const char *fmt, ...);
static void Com_SetErrorMessage(const char *errorMessage);
void Com_Error(errorParm_t code, const char *fmt, ...);
qboolean Com_SafeMode(void);
void Com_StartupVariable(const char *match);
void Info_Print(const char *s);
void Com_ShutdownEvents(void);
static void Com_Error_f(void);
static void Com_Crash_f(void);
void Com_WriteCDKey(void);
static int Com_GpuStringCompare(const char *wild, const char *s);
static int Com_GetConfigureDvarNames(const char **text, char *dvarNames);
static void Com_GetConfigureDvarValues(int dvarCount, const char **text, char *dvarValues);
static void Com_SetConfigureDvars(int dvarCount, const char *dvarNames, const char *dvarValues);
static void Com_WriteConfigToFile(const char *filename);
void Com_WriteConfig_f(void);
float Com_GetTimescaleForSnd(void);
void Com_Close(void);
void Field_Clear(field_t *edit);
void Com_SetWeaponInfoMemory(int iSource);
void Com_FreeWeaponInfoMemory(int iSource);
int Com_AddToString(const char *add, char *msg, int len, int maxlen, qboolean mayAddQuotes);
char Com_GetDecimalDelimiter(void);
void Com_SetRecommended(qboolean restart);
void Com_CheckSetRecommended(void);
void Com_ReadCDKey(void);
void Com_LocalizedFloatToString(float f, char *buffer, unsigned int maxlen, unsigned int numDecimalPlaces);
void Com_Quit_f(void);
void Com_Restart(void);
void Com_ShutdownInternal(char *finalmsg);
static BM_NOINLINE void Com_ErrorCleanup(void);
void Com_ExecStartupConfigs(const char *configFile);
int Com_EventLoop(void);
qboolean Debug_EventLoop(void);
BM_NOINLINE void Com_Frame_Try_Block_Function(void);
void Com_Shutdown(char *finalmsg);
static BM_NOINLINE void Com_StartHunkUsers(void);
void Com_Frame(void);
void Com_WriteDefaults_f(void);
int Com_Milliseconds(void);
void Com_PumpMessageLoop(void);
static void Com_Freeze_f(void);
void Com_Init_Try_Block_Function(char *commandLine);
void Com_Init(char *commandLine);

void Com_BeginRedirect(char *buffer, int buffersize, void (*flush)())
{
    if (!buffer || !buffersize || !flush)
        return;
    rd_buffer = buffer;
    rd_buffersize = buffersize;
    rd_flush = flush;
    *buffer = 0;
}

void Com_EndRedirect(void)
{
    if (rd_flush)
        rd_flush(rd_buffer);
    rd_buffer = 0;
    rd_buffersize = 0;
    rd_flush = 0;
}

void Com_Printf(const char *fmt, ...)
{
    char msg[4096];
    va_list argptr;

    va_start(argptr, fmt);
    vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    Com_PrintMessage(0, msg);
}

void Com_PrintMessage(print_msg_type_t type, const char *msg)
{
    extern void I_strncat(char *dest, int destsize, const char *src);
    extern void Sys_Print(const char *msg);
    extern int FS_Initialized(void);
    extern int FS_Write(const void *buffer, int len, int h);
    extern void FS_Flush(int f);
    extern int FS_FOpenTextFileWrite(const char *filename);
    extern void CL_ConsolePrint(int type, const char *msg, int unknown1, int unknown2);

    if (rd_buffer) {
        if (type == 4)
            return;
        if (strlen(msg) + strlen(rd_buffer) - 1 > (unsigned)(rd_buffersize - 1)) {
            rd_flush(rd_buffer);
            *rd_buffer = '\0';
        }
        I_strncat(rd_buffer, rd_buffersize, msg);
        return;
    }

    if (type != 4 && (!com_dedicated || !com_dedicated->current.integer)) {
        CL_ConsolePrint(type, msg, 0, 0);
    }

    if (msg[0] == '^' && msg[1] != '\0')
        msg += 2;

    if (type != 4)
        Sys_Print(msg);

    if (!com_logfile || !com_logfile->current.integer)
        return;
    if (!FS_Initialized())
        return;

    if (!logfile) {
        if (opening_qconsole)
            return;
        opening_qconsole = 1;
        {
            long aclock;
            time(&aclock);
            logfile = FS_FOpenTextFileWrite("qconsole_mp.log");
            Com_Printf("logfile opened on %s\n", asctime(localtime(&aclock)));
        }
        opening_qconsole = 0;
        if (!logfile)
            return;
    }

    FS_Write(msg, strlen(msg), logfile);
    if (com_logfile->current.integer > 1) {
        FS_Flush(logfile);
    }
}

void Com_DPrintf(const char *fmt, ...)
{
    char msg[4096];
    va_list argptr;

    if (!com_developer || !com_developer->current.integer) {
        return;
    }

    va_start(argptr, fmt);
    vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    Com_Printf("%s", msg);
}

static void Com_SetErrorMessage(const char *errMsg)
{
    extern const dvar_t *Dvar_RegisterString(const char *name, const char *value, int flags);
    extern void Dvar_SetString(const dvar_t *dvar, const char *value);
    extern const char *SEH_LocalizeTextMessage(const char *msg, const char *context, int flag);
    extern void I_strncpyz(char *dest, const char *src, int destsize);
    const char *title;
    const char *translation;
    int isNotice;
    int i;

    ui_errorMessage = Dvar_RegisterString("com_errorMessage", "", 0x1040);
    ui_errorTitle = Dvar_RegisterString("com_errorTitle", "", 0x1040);

    isNotice = (errorcode == 2);
    if (!isNotice) {
        for (i = 0; noticeErrors[i] && noticeErrors[i][0]; i++) {
            if (stricmp(noticeErrors[i], errMsg) == 0) {
                isNotice = 1;
                break;
            }
        }
    }

    if (isNotice) {
        title = SEH_LocalizeTextMessage("MENU_NOTICE", "error message", 1);
        if (!title)
            title = "MENU_NOTICE";
    } else {
        title = SEH_LocalizeTextMessage("MENU_ERROR", "error message", 1);
        if (!title)
            title = "MENU_ERROR";
    }
    Dvar_SetString(ui_errorTitle, title);

    translation = SEH_LocalizeTextMessage(errMsg, "error message", 1);
    if (translation) {
        Dvar_SetString(ui_errorMessage, translation);
        I_strncpyz(com_errorMessage, translation, sizeof(com_errorMessage));
    } else {
        Dvar_SetString(ui_errorMessage, errMsg);
    }
}

void Com_Error(errorParm_t code, const char *fmt, ...)
{
    clientStatic_t *clsg = (clientStatic_t *)imp_cls;
    extern void CL_ConsoleFixPosition(void);
    extern qboolean UI_AnyFullScreenMenuVisible(void);
    extern char *cls;
    va_list argptr;


    if (com_errorEntered) {
        Sys_Error("recursive error after: %s", com_errorMessage);
    }

    com_errorEntered = 1;
    va_start(argptr, fmt);
    vsnprintf(com_errorMessage, sizeof(com_errorMessage), fmt, argptr);
    va_end(argptr);
    com_errorMessage[4095] = '\0';

    if (code == 4 || code == 6) {

        if (!com_fixedConsolePosition) {
            com_fixedConsolePosition = 1;
            CL_ConsoleFixPosition();
        }
        {
            int uiStarted;
            uiStarted = clsg->uiStarted;
            if (uiStarted) {
                if (!UI_AnyFullScreenMenuVisible()) {
                    Com_SetErrorMessage(com_errorMessage);
                    UI_SetActiveMenu(1);
                }
                uiStarted = clsg->uiStarted;
                if (uiStarted) {
                    com_errorEntered = 0;
                    return;
                }
            }
        }
        code = 1;
    } else if (code == 5) {

        com_fixedConsolePosition = 1;
        CL_ConsoleFixPosition();
        code = 1;
    } else {
        com_fixedConsolePosition = 0;
    }

    errorcode = code;
    longjmp(*(jmp_buf *)Sys_GetValue(2), -1);
}

qboolean Com_SafeMode(void)
{
    extern void Cmd_TokenizeString(const char *text);
    extern const char *Cmd_Argv(int arg);
    extern int I_stricmp(const char *s1, const char *s2);
    int i;

    for (i = 0; i < com_numConsoleLines; i++) {
        Cmd_TokenizeString(com_consoleLines[i]);
        if (I_stricmp(Cmd_Argv(0), "safe") == 0 ||
            I_stricmp(Cmd_Argv(0), "dvar_restart") == 0) {
            com_consoleLines[i][0] = '\0';
            return 1;
        }
    }
    return com_safemode;
}

void Com_StartupVariable(const char *match)
{
    extern void Cmd_TokenizeString(const char *text);
    extern const char *Cmd_Argv(int arg);
    extern void Dvar_Set_f(void);
    extern void Dvar_SetA_f(void);
    int lineIndex;

    for (lineIndex = 0; lineIndex < com_numConsoleLines; lineIndex++) {
        Cmd_TokenizeString(com_consoleLines[lineIndex]);
        if (match) {
            if (strcmp(Cmd_Argv(1), match) != 0)
                continue;
            if (stricmp(Cmd_Argv(0), "set") == 0) {
                Dvar_Set_f();
            } else if (stricmp(Cmd_Argv(0), "seta") == 0) {
                Dvar_SetA_f();
            }
        } else {
            if (stricmp(Cmd_Argv(0), "set") == 0) {
                Dvar_Set_f();
            } else if (stricmp(Cmd_Argv(0), "seta") == 0) {
                Dvar_SetA_f();
            }
        }
    }
}

static qboolean Com_HasStartupCommandsOtherThanSet(void)
{
    extern void Cmd_TokenizeString(const char *text);
    extern const char *Cmd_Argv(int arg);
    int lineIndex;

    for (lineIndex = 0; lineIndex < com_numConsoleLines; lineIndex++) {
        const char *cmd;

        if (!com_consoleLines[lineIndex] || com_consoleLines[lineIndex][0] == '\0')
            continue;

        Cmd_TokenizeString(com_consoleLines[lineIndex]);
        cmd = Cmd_Argv(0);
        if (!cmd || cmd[0] == '\0')
            continue;
        if (stricmp(cmd, "set") == 0 || stricmp(cmd, "seta") == 0)
            continue;
        return 1;
    }

    return 0;
}

void Info_Print(const char *s)
{
    char key[0x208];
    char value[0x208];
    char *keyp;
    int keylen;

    if (*s == '\\')
        s++;

    while (*s) {

        keyp = key;
        while (*s && *s != '\\') {
            *keyp++ = *s++;
        }
        keylen = (int)(keyp - key);
        if (keylen > 0x13) {

            *keyp = '\0';
            Com_Printf("%s", key);
        } else {

            memset(keyp, ' ', 0x14 - keylen);
            key[0x14] = '\0';
            Com_Printf("%s", key);
        }
        if (*s == '\0') {
            Com_Printf("MISSING VALUE\n");
            return;
        }
        s++;

        {
            char *vp = value;
            while (*s && *s != '\\') {
                *vp++ = *s++;
            }
            *vp = '\0';
            if (*s)
                s++;
            Com_Printf("%s\n", value);
        }
    }
}

void Com_ShutdownEvents(void)
{
    while (com_pushedEventsHead > com_pushedEventsTail) {
        int idx = (unsigned char)com_pushedEventsTail;
        sysEvent_t *ev = &com_pushedEvents[idx];
        com_pushedEventsTail++;
        if (ev->evPtr) {
            Z_FreeInternal(ev->evPtr);
        }
    }
}

static void Com_Error_f(void)
{
    if (Cmd_Argc() - 1 > 0)
        Com_Error(1, "Testing drop error");
    else
        Com_Error(0, "Testing fatal error");
}

static void Com_Crash_f(void)
{
    *(int *)0 = 0x12345678;
}

void Com_WriteCDKey(void)
{
    extern qboolean CL_CDKeyValidate(const char *key, const char *checksum);
    extern void MacPreferences_PutString(const char *key, const char *value);
    char regkey[21];

    if (!CL_CDKeyValidate(cl_cdkey, cl_cdkeychecksum)) {
        memset(cl_cdkey, ' ', 16);
        cl_cdkey[16] = '\0';
        return;
    }
    memcpy(regkey, cl_cdkey, 16);
    memcpy(regkey + 16, cl_cdkeychecksum, 4);
    regkey[20] = '\0';
    MacPreferences_PutString("codkey", regkey);
}

static int Com_GpuStringCompare(const char *wild, const char *s)
{
    for (;;) {
        char charWild = *wild;

        if (charWild == '*') {
            if (wild[1] == '\0')
                return 0;
            if (*s != '\0') {
                if (Com_GpuStringCompare(wild, s + 1) == 0)
                    return 0;
            }
            wild++;
            continue;
        }

        if (charWild == ' ') {
            char charStr = *s;
            if (charStr != '\0' && !isdigit((unsigned char)charStr)) {
                if (Com_GpuStringCompare(wild, s + 1) == 0)
                    return 0;
            }
            wild++;
            continue;
        }

        {
            char charStr = *s;
            s++;
            if (charWild != charStr && charWild != '?') {
                int diff = tolower(charWild) - tolower(charStr);
                if (diff != 0)
                    return diff < 0 ? -1 : 1;
            }
        }

        if (charWild == '\0')
            return 0;

        wild++;
    }
}

static int Com_GetConfigureDvarNames(const char **text, char *dvarNames)
{
    extern const char *Com_ParseOnLine(const char **text);
    extern void I_strncpyz(char *dest, const char *src, int destsize);
    int dvarCount = 0;

    for (;;) {
        const char *token = Com_ParseOnLine(text);
        if (!*text)
            Com_Error(0, "\x15"
                         "configure_mp.csv: unexpected end-of-file");
        if (!*token)
            return dvarCount;
        {
            unsigned int len = (unsigned int)strlen(token);
            if (len > 0x1f)
                Com_Error(0, "\x15"
                             "configure_mp.csv: dvar name \"%s\" longer than %i\n",
                          token, 0x1f);
            if (dvarCount > 0x3f)
                Com_Error(0, "\x15"
                             "configure_mp.csv: more than %i dvars\n",
                          0x40);
            I_strncpyz(dvarNames + dvarCount * 0x20, token, 0x20);
            dvarCount++;
        }
    }
}

static void Com_GetConfigureDvarValues(int dvarCount, const char **text, char *dvarValues)
{
    extern const char *Com_ParseOnLine(const char **text);
    extern void I_strncpyz(char *dest, const char *src, int destsize);
    int dvarIndex;
    char *curValues;

    if (dvarCount <= 0) {

        const char *token = Com_ParseOnLine(text);
        if (*token)
            Com_Error(0, "configure_mp.csv: extra dvar value column(s): value = %s\n", token);
        return;
    }

    if (!dvarValues) {

        for (dvarIndex = 0; dvarIndex < dvarCount; dvarIndex++) {
            const char *token = Com_ParseOnLine(text);
            if (!*text)
                Com_Error(0, "configure_mp.csv: unexpected EOF");
            if (!*token)
                Com_Error(0, "configure_mp.csv: missing entry in dvar value column %i\n", dvarIndex);
            {
                int len = (int)strlen(token);
                if (len > 0x1f)
                    Com_Error(0, "configure_mp.csv: entry '%s' in dvar value column %i is longer than %i\n", token, dvarIndex, 0x1f);
            }
        }

        {
            const char *token = Com_ParseOnLine(text);
            if (*token)
                Com_Error(0, "configure_mp.csv: extra dvar value column(s): value = %s\n", token);
        }
        return;
    }

    curValues = dvarValues;
    for (dvarIndex = 0; dvarIndex < dvarCount; dvarIndex++) {
        const char *token = Com_ParseOnLine(text);
        if (!*text)
            Com_Error(0, "configure_mp.csv: unexpected EOF");
        if (!*token)
            Com_Error(0, "configure_mp.csv: missing entry in dvar value column %i\n", dvarIndex);
        {
            int len = (int)strlen(token);
            if (len > 0x1f)
                Com_Error(0, "configure_mp.csv: entry '%s' in dvar value column %i is longer than %i\n", token, dvarIndex, 0x1f);
            I_strncpyz(curValues, token, 0x20);
            curValues += 0x20;
        }
    }

    {
        const char *token = Com_ParseOnLine(text);
        if (*token)
            Com_Error(0, "configure_mp.csv: extra dvar value column(s): value = %s\n", token);
    }
}

static void Com_SetConfigureDvars(int dvarCount, const char *dvarNames, const char *dvarValues)
{
    extern const dvar_t *Dvar_SetFromStringByNameFromSource(const char *name, const char *value, int source);
    extern const dvar_t *Dvar_FindVar(const char *name);
    extern void Dvar_AddFlags(const dvar_t *dvar, int flags);
    int dvarIndex;

    if (dvarCount <= 0)
        return;

    for (dvarIndex = 0; dvarIndex < dvarCount; dvarIndex++) {
        const char *name = dvarNames + dvarIndex * 0x20;
        const char *value = dvarValues + dvarIndex * 0x20;
        if (strncmp(name, "r_aaSamples", 12) == 0)
            Dvar_SetFromStringByNameFromSource(name, "1", 1);
        else
            Dvar_SetFromStringByNameFromSource(name, value, 1);
        {
            const dvar_t *dvar = Dvar_FindVar(name);
            Dvar_AddFlags(dvar, 1);
        }
    }
}

static void Com_SkipConfigureBlankLines(const char **text)
{
    extern void Com_SkipRestOfLine(const char **text);

    while (*text && **text && (**text == '\r' || **text == '\n'))
        Com_SkipRestOfLine(text);
}

static void Com_WriteConfigToFile(const char *filename)
{
    int f;

    f = FS_FOpenFileWrite(filename);
    if (!f) {
        Com_Printf("Couldn't write %s.\n", filename);
        return;
    }
    FS_Printf(f, "// generated by Call of Duty, do not modify\n");
    FS_Printf(f, "unbindall\n");
    Key_WriteBindings(f);
    Dvar_WriteVariables(f);
    FS_FCloseFile(f);
}

void Com_WriteConfig_f(void)
{
    extern void I_strncpyz(char *dest, const char *src, int destsize);
    extern void Com_DefaultExtension(char *path, int maxSize, const char *extension);
    extern const char *Cmd_Argv(int arg);
    char filename[64];

    if (Cmd_Argc() != 2) {
        Com_Printf("Usage: writeconfig <filename>\n");
        return;
    }
    I_strncpyz(filename, Cmd_Argv(1), 0x40);
    Com_DefaultExtension(filename, 0x40, ".cfg");
    Com_Printf("Writing %s.\n", filename);
    Com_WriteConfigToFile(filename);
}

float Com_GetTimescaleForSnd(void)
{
    int fixedtime = com_fixedtime->current.integer;
    if (fixedtime)
        return (float)fixedtime;
    return com_timescale->current.value;
}

void Com_Close(void)
{
    Com_ShutdownDObj();
    DObjShutdown();
    XAnimShutdown();
    CM_Shutdown();
    SND_ShutdownChannels();
    Hunk_Clear();
    Scr_Shutdown();
}

void Field_Clear(field_t *edit)
{
    memset(edit->buffer, 0, sizeof(edit->buffer));
    edit->cursor = 0;
    edit->scroll = 0;
    edit->drawWidth = 0x100;
}

void Com_SetWeaponInfoMemory(int iSource)
{
    iWeaponInfoSource = iSource;
}

void Com_FreeWeaponInfoMemory(int iSource)
{
    if (iWeaponInfoSource != iSource)
        return;
    iWeaponInfoSource = 0;
    BG_ShutdownWeaponDefFiles();
}

int Com_AddToString(const char *add, char *msg, int len, int maxlen, qboolean mayAddQuotes)
{
    int addQuotes = 0;

    if (mayAddQuotes) {
        if (*add) {
            if ((unsigned char)*add > ' ') {

                int remaining = maxlen - len;
                if (remaining > 0) {
                    const char *p;
                    int needsQuote = 0;
                    for (p = add + 1; (int)(p - add) < remaining; p++) {
                        if (!*p) {
                            break;
                        }
                        if ((unsigned char)*p <= ' ') {
                            needsQuote = 1;
                            break;
                        }
                    }
                    if (needsQuote) {

                        if (len < maxlen) {
                            msg[len] = '"';
                            len++;
                            addQuotes = 1;
                        }
                    }
                } else {

                    addQuotes = 0;
                }
            } else {

                if (len < maxlen) {
                    msg[len] = '"';
                    len++;
                    addQuotes = 1;
                }
            }
        } else {

            if (len < maxlen) {
                msg[len] = '"';
                len++;
                addQuotes = 1;
            }
        }
    }

    if (len < maxlen) {
        int i;
        int room = maxlen - len;
        for (i = 0; add[i] && i < room; i++) {
            msg[len + i] = add[i];
        }
        len += i;
    }

    if (addQuotes) {
        if (len < maxlen) {
            msg[len] = '"';
            len++;
        }
    }

    return len;
}

char Com_GetDecimalDelimiter(void)
{
    int lang = (*(const dvar_t **)imp_loc_language)->current.integer;
    if ((unsigned int)(lang - 1) <= 3 || lang == 6 || lang == 7)
        return ',';
    return '.';
}

void Com_SetRecommended(qboolean restart)
{
    extern void Com_Printf(const char *fmt, ...);
    extern void Sys_GetInfo(void *info);
    extern int FS_ReadFile(const char *path, void **buffer);
    extern void FS_FreeFile(void *buffer);
    extern void Com_BeginParseSession(const char *name);
    extern void Com_SetCSV(int csv);
    extern const char *Com_ParseOnLine(const char **text);
    extern void Com_SkipRestOfLine(const char **text);
    extern void Com_EndParseSession(void);
    extern void Sys_ArchiveInfo(int checksum);
    extern int stricmp(const char *s1, const char *s2);
    extern double atof(const char *s);
    extern int atoi(const char *s);
    extern void Cbuf_AddText(const char *text);
    extern void Cbuf_Execute(void);
    extern qboolean Dvar_AnyLatchedValues(void);

    SysInfo info;
    void *csv = 0;
    int filesize;
    const char *text;

    char dvarNames[0x40 * 0x20];
    int dvarCount;

    struct {
        double MHz;
        int MB;
        char dvarValues[0x40 * 0x20];
    } best;
    double bestMHz;
    int bestMB;
    qboolean foundCpuMatch;

    int gpuDvarCount;
    char gpuDvarNames[0x40 * 0x20];
    qboolean foundGpuSection;
    qboolean foundGpuMatch;

    Com_Printf("========= autoconfigure\n");
    Sys_GetInfo(&info);

    info.cpuGHz *= 1.02;
    if (info.sysMB <= 0x7f)
        info.sysMB = 0x80;

    filesize = FS_ReadFile("configure_mp.csv", &csv);
    if (filesize < 0)
        Com_Error(0, "EXE_ERR_NOT_FOUND\x15"
                     "configure_mp.csv");

    text = (const char *)csv;
    Com_BeginParseSession("configure_mp.csv");
    Com_SetCSV(1);

    dvarCount = 0;
    foundCpuMatch = 0;
    bestMHz = -1.0;
    bestMB = 0;
    memset(&best, 0, sizeof(best));
    gpuDvarCount = 0;
    foundGpuSection = 0;
    foundGpuMatch = 0;

    for (;;) {
        const char *token;
        double rowGHz;
        int rowMB;

        Com_SkipConfigureBlankLines(&text);
        if (!text || !*text)
            break;

        token = Com_ParseOnLine(&text);
        if (!*token || *token == '#') {
            Com_SkipRestOfLine(&text);
            continue;
        }

        if (!dvarCount) {
            const char *col2;

            if (stricmp(token, "cpu ghz") != 0)
                Com_Error(0, "\x15"
                             "configure_mp.csv: \"cpu ghz\" should be the first column\n");

            col2 = Com_ParseOnLine(&text);
            if (stricmp(col2, "sys mb") != 0)
                Com_Error(0, "\x15"
                             "configure_mp.csv: \"sys mb\" should be the second column\n");

            dvarCount = Com_GetConfigureDvarNames(&text, dvarNames);
            Com_SkipRestOfLine(&text);
            continue;
        }

        if (stricmp(token, "gpu") == 0) {
            gpuDvarCount = Com_GetConfigureDvarNames(&text, gpuDvarNames);
            Com_SkipRestOfLine(&text);
            foundGpuSection = 1;
            break;
        }

        rowGHz = atof(token);
        if (rowGHz < 0.0)
            Com_Error(0, "configure_mp.csv: cpu ghz %g not allowed to be less than 0\n", rowGHz);

        token = Com_ParseOnLine(&text);
        rowMB = atoi(token);
        if (rowMB <= 0x7f)
            Com_Error(0, "configure_mp.csv: sys mb %i not allowed to be less than 128", rowMB);

        if (info.cpuGHz >= rowGHz && rowMB <= info.sysMB) {
            if (rowGHz > bestMHz || (rowGHz == bestMHz && rowMB > bestMB)) {
                best.MHz = rowGHz;
                best.MB = rowMB;
                bestMHz = rowGHz;
                bestMB = rowMB;
                Com_GetConfigureDvarValues(dvarCount, &text, best.dvarValues);
                foundCpuMatch = 1;
            } else {
                Com_GetConfigureDvarValues(dvarCount, &text, 0);
            }
        } else {
            Com_GetConfigureDvarValues(dvarCount, &text, 0);
        }

        Com_SkipRestOfLine(&text);
    }

    if (!foundCpuMatch) {

        if (bestMHz < 0) {
            Sys_GetInfo(&info);
            Com_Printf("configure_mp.csv: no exact match for %.0f GHz %i MB, using highest config\n", info.cpuGHz, info.sysMB);
            bestMHz = info.cpuGHz;
            bestMB = info.sysMB;
        }
        foundCpuMatch = 1;
    }

    Com_Printf("configure_mp.csv: using CPU configuration %.0f GHz %i MB\n", bestMHz, bestMB);
    Cbuf_AddText("exec configure_mp.cfg");
    Cbuf_Execute();
    Com_SetConfigureDvars(dvarCount, dvarNames, best.dvarValues);

    if (!foundGpuSection)
        Com_Error(0, "configure_mp.csv: EXE_ERR_COULDNT_CONFIGURE \"%s\"\n", info.gpuDescription);

    for (;;) {
        const char *find;

        Com_SkipConfigureBlankLines(&text);
        if (!text || !*text)
            break;

        find = Com_ParseOnLine(&text);
        if (!*find || *find == '#') {
            Com_SkipRestOfLine(&text);
            continue;
        }

        if (!foundGpuMatch) {
            char wildcardTemplate[0x400];
            int wildcardLen = 1;
            wildcardTemplate[0] = '*';
            if (*find) {
                const char *p = find;
                char c;
                while ((c = *p++) != '\0') {
                    signed char sc = (signed char)c;

                    int isUpper = (sc >= 'A' && sc <= 'Z');
                    if (!isUpper) {
                        wildcardTemplate[wildcardLen] = c;
                        wildcardLen++;
                        if (wildcardLen >= 0x3ff)
                            Com_Error(0, "configure_mp.csv: gpu template too long");
                    } else {
                        if (wildcardLen > 0 && wildcardTemplate[wildcardLen - 1] != ' ') {
                            wildcardTemplate[wildcardLen] = ' ';
                            wildcardLen++;
                            if (wildcardLen >= 0x3ff)
                                Com_Error(0, "configure_mp.csv: gpu template too long");
                        }
                    }
                }
            }
            if (wildcardTemplate[wildcardLen - 1] == '*')
                wildcardLen--;
            wildcardTemplate[wildcardLen] = '*';
            wildcardTemplate[wildcardLen + 1] = '\0';

            if (Com_GpuStringCompare(wildcardTemplate, info.gpuDescription) == 0) {
                Com_Printf("configure_mp.csv: using GPU configuration \"%s\"\n", find);
                {
                    char gpuDvarValues[0x40 * 0x20];
                    Com_GetConfigureDvarValues(gpuDvarCount, &text, gpuDvarValues);
                    Com_SetConfigureDvars(gpuDvarCount, gpuDvarNames, gpuDvarValues);
                }
                foundGpuMatch = 1;
            } else {
                Com_GetConfigureDvarValues(gpuDvarCount, &text, 0);
            }
        } else {
            Com_GetConfigureDvarValues(gpuDvarCount, &text, 0);
        }

        Com_SkipRestOfLine(&text);
    }

    if (!foundGpuMatch)
        Com_Error(0, "configure_mp.csv: EXE_ERR_COULDNT_CONFIGURE \"%s\"\n", info.gpuDescription);

    Com_EndParseSession();

    {
        int checksum = 0;
        int i;

        for (i = 0; i < filesize; i++)
            checksum = (int)((char *)csv)[i] + checksum * 1000000007;

        checksum &= 0xfffffff;
        FS_FreeFile(csv);
        Sys_ArchiveInfo(checksum + 1);
    }

    if (restart && Dvar_AnyLatchedValues())
        Cbuf_AddText("snd_restart\n");
}

void Com_CheckSetRecommended(void)
{
    extern int FS_ReadFile(const char *path, void **buffer);
    extern void FS_FreeFile(void *buffer);
    extern qboolean Sys_HasConfigureChecksumChanged(int checksum);
    extern qboolean Sys_HasInfoChanged(void);
    extern void Dvar_SetBool(const dvar_t *dvar, int value);

    int needSet;

    if (com_recommendedSet->current.enabled) {
        void *csv;
        int filesize = FS_ReadFile("configure_mp.csv", &csv);
        if (filesize < 0)
            Com_Error(0, "EXE_ERR_NOT_FOUND\x15"
                         "configure_mp.csv");
        {
            int checksum = 0;
            if (filesize > 0) {
                int i;
                for (i = 0; i < filesize; i++) {
                    checksum = (int)((char *)csv)[i] + checksum * 1000000007;
                }
                checksum &= 0xfffffff;
            }
            FS_FreeFile(csv);
            needSet = Sys_HasConfigureChecksumChanged(checksum + 1);
        }
    } else {
        needSet = 1;
    }

    if (needSet) {
        Com_SetRecommended(0);
        Dvar_SetBool(com_recommendedSet, 1);
    }

    if (Sys_HasInfoChanged()) {
        Com_SetRecommended(0);
    }
}

void Com_ReadCDKey(void)
{
    extern qboolean MacPreferences_GetString(const char *key, char *buf, int bufsize, int defaultVal);
    extern qboolean CL_CDKeyValidate(const char *key, const char *checksum);
    char regkey[21];

    if (MacPreferences_GetString("codkey", regkey, 0x15, 0)) {
        memcpy(cl_cdkey, regkey, 16);
        cl_cdkey[16] = '\0';
        memcpy(cl_cdkeychecksum, regkey + 16, 4);
        cl_cdkeychecksum[4] = '\0';
        if (CL_CDKeyValidate(cl_cdkey, cl_cdkeychecksum))
            return;
    }
    memset(cl_cdkey, ' ', 16);
    cl_cdkey[16] = '\0';
}

void Com_LocalizedFloatToString(float f, char *buffer, unsigned int maxlen, unsigned int numDecimalPlaces)
{
    unsigned int i;
    int lang;

    snprintf(buffer, maxlen - 1, "%.*f", numDecimalPlaces, (double)f);
    buffer[maxlen - 1] = '\0';

    lang = (*(const dvar_t **)imp_loc_language)->current.integer;
    if ((unsigned int)(lang - 1) > 3 && lang != 6 && lang != 7)
        return;

    for (i = 0; i < maxlen; i++) {
        if (buffer[i] == '.') {
            buffer[i] = ',';
            return;
        }
    }
}

void Com_Quit_f(void)
{
    extern void Hunk_ClearTempMemory(void);
    extern void Hunk_ClearTempMemoryHigh(void);
    extern void Sys_DestroySplashWindow(void);
    extern void SV_Shutdown(const char *finalmsg);
    extern void FS_Shutdown(int closemfp);
    extern void FS_ShutdownServerIwdNames(void);
    extern void FS_ShutdownServerReferencedIwds(void);
    extern void Sys_Quit(void);

    Com_Printf("quitting...\n");
    if (com_errorEntered) {
        Sys_Quit();
        return;
    }
    Hunk_ClearTempMemory();
    Hunk_ClearTempMemoryHigh();
    Sys_DestroySplashWindow();
    CL_SwitchToLocalClient(0);
    CL_Shutdown();
    CL_SwitchToLocalClient(0);
    SV_Shutdown("EXE_SERVERQUIT");
    Com_Close();
    if (logfile) {
        FS_FCloseFile(logfile);
        logfile = 0;
    }
    FS_Shutdown(1);
    FS_ShutdownServerIwdNames();
    FS_ShutdownServerReferencedIwds();
    Sys_Quit();
}

void Com_Restart(void)
{
    extern void CL_ShutdownHunkUsers(void);
    extern void SV_ShutdownGameProgs(void);
    extern void CIN_CloseAllVideos(void);
    extern void Hunk_Clear(void);
    extern void Scr_Init(void);
    extern void Scr_Settings(int enabled, int developer_script, int developer);
    extern void XAnimInit(void);
    extern void DObjInit(void);
    extern void Com_InitDObj(void);

    CL_ShutdownHunkUsers();
    SV_ShutdownGameProgs();
    CIN_CloseAllVideos();
    Com_ShutdownDObj();
    DObjShutdown();
    XAnimShutdown();
    CM_Shutdown();
    SND_ShutdownChannels();
    Hunk_Clear();
    Scr_Init();
    {
        int dev = com_developer->current.integer;
        int enabled = (dev || com_logfile->current.integer) ? 1 : 0;
        int dev_script = com_developer_script->current.enabled;
        Scr_Settings(enabled, dev_script, dev);
    }
    com_fixedConsolePosition = 0;
    XAnimInit();
    DObjInit();
    Com_InitDObj();
}

void Com_ShutdownInternal(char *finalmsg)
{
    extern void CL_Disconnect(void);
    extern void CL_ShutdownAll(void);
    extern void CL_ShutdownDemo(void);
    extern void SV_Shutdown(const char *msg);

    CL_SwitchToLocalClient(0);
    CL_Disconnect();
    CL_SwitchToLocalClient(0);
    CL_ShutdownAll();
    CL_ShutdownDemo();
    SV_Shutdown(finalmsg);
    Com_Restart();
}

static BM_NOINLINE void Com_ErrorCleanup(void)
{
    clientStatic_t *clsg = (clientStatic_t *)imp_cls;
    extern void LargeLocalReset(void);
    extern void Hunk_ClearTempMemory(void);
    extern void Hunk_ClearTempMemoryHigh(void);
    extern Bool Com_IsMapProfilerActive(void);
    extern void Dvar_SetIntByName(const char *name, int value);
    extern void Dvar_SetInAutoExec(int enabled);
    extern void FS_PureServerSetLoadedIwds(const char *a, const char *b);
    extern void SEH_UpdateLanguageInfo(void);
    extern void Scr_Abort(void);
    extern void SND_ErrorCleanup(void);
    extern void Com_CleanupBsp(void);
    extern void CM_Cleanup(void);
    extern void Com_ResetParseSessions(void);
    extern void CL_FlushDebugData(int flag);
    extern void FS_ResetFiles(void);
    extern void Cbuf_Init(void);
    extern qboolean UI_SetActiveMenu(int menu);
    extern void CL_ConsoleFixPosition(void);
    extern qboolean QuitOnError(void);
    extern void I_strncpyz(char *dest, const char *src, int destsize);
    extern const char *SEH_LocalizeTextMessage(const char *msg, const char *context, int flag);

    char szUnlocedMsg[0x1008];
    int now, rendererStarted;

    LargeLocalReset();

    {
        void *re;
        re = imp_re;
        {
            void (*fn)(void) = ((refexport_t *)re)->AbortRenderCommands;
            if (fn)
                fn();
        }
    }

    Dvar_SetInAutoExec(0);
    Hunk_ClearTempMemory();
    Hunk_ClearTempMemoryHigh();
    Com_IsMapProfilerActive();
    Dvar_SetIntByName("cl_paused", 0);
    FS_PureServerSetLoadedIwds("", "");
    SEH_UpdateLanguageInfo();
    strcpy(szUnlocedMsg, com_errorMessage);

    if (errorcode == 3) {
        if (com_errorMessage[0]) {
            const char *localized = SEH_LocalizeTextMessage(com_errorMessage, "error message", 1);
            if (localized)
                I_strncpyz(com_errorMessage, localized, sizeof(com_errorMessage));
        }
    } else {
        rendererStarted = clsg->uiStarted;
        if (rendererStarted)
            UI_SetActiveMenu(0);
    }

    Com_SetErrorMessage(com_errorMessage);
    if (errorcode != 3)
        Scr_Abort();
    SND_ErrorCleanup();
    Com_CleanupBsp();
    CM_Cleanup();
    Com_ResetParseSessions();
    CL_FlushDebugData(1);

    {
        void *re;
        re = imp_re;
        {
            void (*fn)(void) = ((refexport_t *)re)->ResetImageAllocations;
            if (fn)
                fn();
        }
    }

    FS_ResetFiles();
    if (errorcode == 1)
        Cbuf_Init();

    now = Sys_Milliseconds();
    if (now - lastErrorTime <= 99) {
        errorCount++;
        if (errorCount > 3)
            errorcode = 0;
    } else {
        errorCount = 0;
    }
    lastErrorTime = now;

    if ((unsigned int)(errorcode - 1) > 2) {

        Sys_Error("%s", com_errorMessage);
    }

    *(byte *)imp_updateScreenCalled = 0;

    if (errorcode == 2) {
        Com_ShutdownInternal("Server fatal crashed: %s\n");
        com_fixedConsolePosition = 0;
        com_errorEntered = 0;
        return;
    }

    Com_Printf("********************\nERROR: %s\n********************\n", com_errorMessage);
    if (errorcode == 1) {
        rendererStarted = clsg->uiStarted;
        if (rendererStarted && !com_fixedConsolePosition)
            CL_ConsoleFixPosition();
    }
    Com_ShutdownInternal(szUnlocedMsg);
    if (errorcode == 1) {
        if (QuitOnError())
            Com_Quit_f();
    }
    com_fixedConsolePosition = 0;
    com_errorEntered = 0;
}

void Com_ExecStartupConfigs(const char *configFile)
{
    extern void Cbuf_AddText(const char *text);
    extern void Dvar_SetInAutoExec(int enabled);

    Cbuf_AddText("exec default_mp.cfg\n");
    Cbuf_AddText("exec language.cfg\n");
    if (configFile)
        Cbuf_AddText(va("exec %s\n", configFile));
    Cbuf_Execute();
    Dvar_SetInAutoExec(1);
    Cbuf_Execute();
    Dvar_SetInAutoExec(0);
    if (Com_SafeMode())
        Cbuf_AddText("exec safemode_mp.cfg\n");
    Cbuf_Execute();
}

int Com_EventLoop(void)
{
    extern void CL_KeyEvent(int key, int down, int time);
    extern void CL_CharEvent(int ch);
    extern Bool CL_PacketEvent(netadr_t from, msg_t * msg, int time);
    extern void SV_PacketEvent(netadr_t from, msg_t * msg);
    extern void Cbuf_AddText(const char *text);
    extern void LargeLocal_LargeLocal(LargeLocal * ll, int size);
    extern void *LargeLocal_GetBuf(LargeLocal * ll);
    extern void ZN10LargeLocalD1Ev(LargeLocal * ll);
    extern sysEvent_t Sys_GetEvent(void);
    extern qboolean NET_GetLoopPacket(int sock, netadr_t *net_from, msg_t *msg);
    extern void MSG_Init(msg_t * buf, byte * data, int length);

    LargeLocal bufData_ll;
    byte *data;
    msg_t buf;
    netadr_t evFrom;
    int evTime = 0;

    LargeLocal_LargeLocal(&bufData_ll, MAX_MSGLEN);
    data = (byte *)LargeLocal_GetBuf(&bufData_ll);
    MSG_Init(&buf, data, MAX_MSGLEN);

    for (;;) {
        int evType, evValue, evValue2, evPtrLength;
        void *evPtr;

        if (com_pushedEventsTail < com_pushedEventsHead) {
            int tail = com_pushedEventsTail++;
            sysEvent_t *q = &com_pushedEvents[(unsigned char)tail];
            evTime = q->evTime;
            evType = q->evType;
            evValue = q->evValue;
            evValue2 = q->evValue2;
            evPtrLength = q->evPtrLength;
            evPtr = q->evPtr;
        } else {
            sysEvent_t ev;
            ev = Sys_GetEvent();
            evTime = ev.evTime;
            evType = ev.evType;
            evValue = ev.evValue;
            evValue2 = ev.evValue2;
            evPtrLength = ev.evPtrLength;
            evPtr = ev.evPtr;
        }

        if (evType > 5) {
            Com_Error(0, "Com_EventLoop: bad event type %i", evType);
            continue;
        }

        switch (evType) {
        case 0:
            { static int c0; if (c0++ < 3) Com_Printf("[cnx] evloop case0 sv_running=%d\n", com_sv_running->current.enabled); }
            while (NET_GetLoopPacket(0, &evFrom, &buf)) {
                Com_Printf("[cnx] loop0 packet (server->client)\n");
                CL_PacketEvent(evFrom, &buf, evTime);
            }
            while (NET_GetLoopPacket(1, &evFrom, &buf)) {
                Com_Printf("[cnx] loop1 packet (client->server) sv_running=%d\n", com_sv_running->current.enabled);
                CL_SwitchToLocalClient(0);
                if (com_sv_running->current.enabled) {
                    SV_PacketEvent(evFrom, &buf);
                } else {
                    CL_PacketEvent(evFrom, &buf, evTime);
                }
            }
            ZN10LargeLocalD1Ev(&bufData_ll);
            return evTime;
        case 1:
            if (evValue < 0 || evValue > 255) {
                Com_Printf("BAD KEY: evValue=%d evValue2=%d\n", evValue, evValue2);
                break;
            }
            CL_KeyEvent(evValue, evValue2, evTime);
            break;
        case 2:
            CL_CharEvent(evValue);
            break;
        case 3:
            break;
        case 4:
            CL_SwitchToLocalClient(0);
            Cbuf_AddText((char *)evPtr);
            Z_FreeInternal(evPtr);
            Cbuf_AddText("\n");
            break;
        case 5:
        {
            netadr_t *from = (netadr_t *)evPtr;
            int dataLen = evPtrLength - 12;
            if (dataLen > buf.maxsize) {
                Z_FreeInternal(evPtr);
                Com_Printf("Com_EventLoop: oversize packet\n");
                break;
            }
            memcpy(&evFrom, from, sizeof(netadr_t));
            buf.cursize = dataLen;
            memcpy(buf.data, (char *)evPtr + 12, dataLen);
            Z_FreeInternal(evPtr);
            CL_SwitchToLocalClient(0);
            if (com_sv_running->current.enabled)
                SV_PacketEvent(evFrom, &buf);
            else
                CL_PacketEvent(evFrom, &buf, evTime);
            break;
        }
        }
    }
}

qboolean Debug_EventLoop(void)
{
    extern void CL_KeyEvent(int key, int down, int time);
    extern void CL_CharEvent(int ch);
    extern void Cbuf_AddText(const char *text);
    extern sysEvent_t Sys_GetEvent(void);

    qboolean newEvent = 0;

    for (;;) {
        int evType, evValue, evValue2, evTime;
        void *evPtr;

        if (com_pushedEventsTail < com_pushedEventsHead) {
            int tail = com_pushedEventsTail++;
            unsigned int idx = (unsigned char)tail;
            evPtr = com_pushedEvents[idx].evPtr;
            evValue2 = com_pushedEvents[idx].evValue2;
            evValue = com_pushedEvents[idx].evValue;
            evType = com_pushedEvents[idx].evType;
            evTime = com_pushedEvents[idx].evTime;
        } else {
            sysEvent_t ev;
            ev = Sys_GetEvent();
            evTime = ev.evTime;
            evType = ev.evType;
            evValue = ev.evValue;
            evValue2 = ev.evValue2;
            evPtr = ev.evPtr;
        }

        switch ((unsigned int)evType) {
        case 0:
            return newEvent;
        case 1:
            CL_KeyEvent(evValue, evValue2, evTime);
            break;
        case 2:
            CL_CharEvent(evValue);
            break;
        case 3:
            break;
        case 4:
            Cbuf_AddText((char *)evPtr);
            Z_FreeInternal(evPtr);
            Cbuf_AddText("\n");
            break;
        case 5:
            Z_FreeInternal(evPtr);
            break;
        default:
            Com_Error(0, "Com_EventLoop: bad event type %i", evType);
            break;
        }
        newEvent = 1;
    }
}

BM_NOINLINE void Com_Frame_Try_Block_Function(void)
{
    int msec, rawMsec, minMsec, maxMsec;
    qboolean useTimescale;

    if (com_fullyInitialized && (dvar_modifiedFlags & 1)) {
        dvar_modifiedFlags &= ~1;
        if (Com_HasPlayerProfile()) {
            char path[64];
            Com_BuildPlayerProfilePath(path, 64, "config_mp.cfg");
            Com_WriteConfigToFile(path);
        }
    }

    if (com_viewlog->modified) {
        if (!com_dedicated->current.integer) {
            Sys_ShowConsole(com_viewlog->current.integer, 0);
        }
        Dvar_ClearModified(com_viewlog);
    }

    SetAnimCheck(com_animCheck->current.enabled);

    if (com_dedicated->current.integer && !com_fixedtime->current.integer
        && com_timescale->current.value == 1.0f && com_codeTimeScale == 1.0f) {
        /* select() wakes as soon as UDP arrives. Com_EventLoop still processes
         * each packet immediately; idle polling need only reach the next tick. */
        minMsec = SV_FrameWaitMilliseconds();
    } else if (com_maxfps->current.integer > 0 && !com_dedicated->current.integer) {
        minMsec = 1000 / com_maxfps->current.integer;
        if (minMsec == 0)
            minMsec = 1;
    } else {
        minMsec = 1;
    }

    do {
        com_frameTime = Com_EventLoop();
        if (com_frameTime < com_lastFrameTime)
            com_lastFrameTime = com_frameTime;
        rawMsec = com_frameTime - com_lastFrameTime;
#ifdef __EMSCRIPTEN__
        /* requestAnimationFrame schedules the next opportunity; never spin
         * on the browser's main thread to enforce a desktop frame cap. */
        if (rawMsec < minMsec)
            return;
#else
        if (rawMsec < minMsec)
            NET_Sleep(minMsec - rawMsec);
#endif
    } while (rawMsec < minMsec);

    Cbuf_Execute();
    com_lastFrameTime = com_frameTime;

    if (com_fixedtime->current.integer) {
        msec = com_fixedtime->current.integer;
        useTimescale = 1;
    } else {
        float ts = com_timescale->current.value;
        if (ts == 1.0f && com_codeTimeScale == 1.0f) {
            msec = rawMsec;
            useTimescale = 0;
        } else {
            msec = (int)floorf((float)rawMsec * ts * com_codeTimeScale + 0.5f);
            useTimescale = 1;
        }
    }

    if (msec <= 0)
        msec = 1;

    if (com_dedicated->current.integer) {

        if (msec >= 501 && msec <= 499999) {
            Com_Printf("Hitch warning: %i msec frame time\n", msec);
        }
        maxMsec = 5000;
    } else {
        if (com_sv_running->current.enabled)
            maxMsec = 200;
        else
            maxMsec = 5000;
    }

    if (msec < maxMsec)
        maxMsec = msec;

    if (useTimescale && rawMsec != 0)
        com_timescaleValue = (float)maxMsec / (float)rawMsec;
    else
        com_timescaleValue = 1.0f;

    CL_SwitchToLocalClient(0);
    SV_Frame(maxMsec);

    if (!(com_dedicated->flags & 0x40)) {
        if (com_dedicated->latched.integer != com_dedicated->current.integer) {
            com_dedicated = Dvar_RegisterInt("dedicated", 0, 0, 2, 0x1020);
            if (com_dedicated->current.integer) {
                com_dedicated = Dvar_RegisterInt("dedicated", 0, 0, 2, 0x1040);
            }
            Dvar_ClearModified(com_dedicated);
            CL_SwitchToLocalClient(0);
            CL_Shutdown();
            CL_SwitchToLocalClient(0);
            Sys_ShowConsole(1, 1);
            Sys_NormalExit();
            SV_AddDedicatedCommands();
        }
    }

#ifdef DEDICATED
    return;
#endif
    if (com_dedicated->current.integer)
        return;

    CL_SwitchToLocalClient(0);
    CL_RunOncePerClientFrame(maxMsec);
    CL_SwitchToLocalClient(0);
    Com_EventLoop();
    CL_SwitchToLocalClient(0);
    Cbuf_Execute();
    CL_SwitchToLocalClient(0);
    SND_UpdateLoopingSounds();
    SND_Update();
    CL_SwitchToLocalClient(0);
    CL_Frame(maxMsec);
    CL_SwitchToLocalClient(0);
    SCR_UpdateScreenInternal();
    SCR_RunCinematic();

    if (com_statmon->current.enabled) {
        if (*com_fileAccessed) {
            StatMon_Warning(1, 3000, "File Accessed");
            *com_fileAccessed = 0;
        }
        {
            int prevTime = timeClientFrame;
            int now = Sys_Milliseconds();
            timeClientFrame = now;
            if (com_statmon->current.enabled && (now - prevTime) > 33 && prevTime != 0) {
                StatMon_Warning(0, 3000, "33 msec frame hit");
            }
        }
    }
}

void Com_Shutdown(char *finalmsg)
{
    Com_ShutdownInternal(finalmsg);
    UI_SetMap("", "");
    CL_StartHunkUsers();
}

static BM_NOINLINE void Com_StartHunkUsers(void)
{
    jmp_buf *abortframe = (jmp_buf *)Sys_GetValue(2);

    if (setjmp(*abortframe)) {
        Sys_Error("Error during initialization of hunk users");
        return;
    }

    UI_SetMap("", "");
    CL_StartHunkUsers();
    Com_EventLoop();

    if (com_dedicated && !com_dedicated->current.integer) {
        UI_SetActiveMenu(1);
    }
}

void Com_Frame(void)
{
    jmp_buf *abortframe = (jmp_buf *)Sys_GetValue(2);

    if (!setjmp(*abortframe)) {
        Com_Frame_Try_Block_Function();
        com_frameNumber++;
    }

    if (com_errorEntered) {
        Com_ErrorCleanup();
        Com_StartHunkUsers();
    }
}

void Com_WriteDefaults_f(void)
{
    extern const char *Cmd_Argv(int arg);
    extern void I_strncpyz(char *dest, const char *src, int destsize);
    extern void Com_DefaultExtension(char *path, int maxSize, const char *extension);
    extern void Dvar_WriteDefaults(int f);
    char filename[64];
    int f;

    if (Cmd_Argc() != 2) {
        Com_Printf("Usage: writedefaults <filename>\n");
        return;
    }
    I_strncpyz(filename, Cmd_Argv(1), 0x40);
    Com_DefaultExtension(filename, 0x40, ".cfg");
    Com_Printf("Writing %s.\n", filename);
    f = FS_FOpenFileWrite(filename);
    if (!f) {
        Com_Printf("Couldn't write %s.\n", filename);
        return;
    }
    FS_Printf(f, "// generated by Call of Duty, do not modify\n");
    Dvar_WriteDefaults(f);
    FS_FCloseFile(f);
}

int Com_Milliseconds(void)
{
    extern sysEvent_t Sys_GetEvent(void);
    sysEvent_t ev;

    for (;;) {
        ev = Sys_GetEvent();
        if (ev.evType == 0)
            break;

        {
            int head = com_pushedEventsHead;
            int idx = (unsigned char)head;
            sysEvent_t *pushed = &com_pushedEvents[idx];

            if (head - com_pushedEventsTail > 255) {
                if (!printedWarning) {
                    printedWarning = 1;
                    Com_Printf("WARNING: Com_PushEvent overflow\n");
                }
                if (pushed->evPtr)
                    Z_FreeInternal(pushed->evPtr);
                com_pushedEventsTail++;
            } else {
                printedWarning = 0;
            }

            *pushed = ev;
            com_pushedEventsHead++;
        }
    }

    return ev.evTime;
}

void Com_PumpMessageLoop(void)
{
    extern sysEvent_t Sys_GetEvent(void);
    sysEvent_t ev;

    for (;;) {
        ev = Sys_GetEvent();
        if (!ev.evType)
            return;

        {
            int head = com_pushedEventsHead;
            sysEvent_t *slot = &com_pushedEvents[(unsigned char)head];
            if (head - com_pushedEventsTail > 0xff) {
                if (!printedWarning) {
                    printedWarning = 1;
                    Com_Printf("WARNING: Com_PushEvent overflow\n");
                }
                if (slot->evPtr)
                    Z_FreeInternal(slot->evPtr);
                com_pushedEventsTail++;
            } else {
                printedWarning = 0;
            }
            *slot = ev;
            com_pushedEventsHead++;
        }
    }
}

static void Com_Freeze_f(void)
{
    extern const char *Cmd_Argv(int arg);
    extern sysEvent_t Sys_GetEvent(void);
    extern double atof(const char *s);
    sysEvent_t ev;
    float s;
    int startTime;

    if (Cmd_Argc() != 2) {
        Com_Printf("Usage: freeze <seconds>\n");
        return;
    }
    s = (float)atof(Cmd_Argv(1));

    for (;;) {
        ev = Sys_GetEvent();
        if (!ev.evType)
            break;
        {
            int head = com_pushedEventsHead;
            sysEvent_t *slot = &com_pushedEvents[(unsigned char)head];
            if (head - com_pushedEventsTail > 0xff) {
                if (!printedWarning) {
                    printedWarning = 1;
                    Com_Printf("WARNING: Com_PushEvent overflow\n");
                }
                if (slot->evPtr)
                    Z_FreeInternal(slot->evPtr);
                com_pushedEventsTail++;
            } else {
                printedWarning = 0;
            }
            *slot = ev;
            com_pushedEventsHead++;
        }
    }
    startTime = ev.evTime;

    for (;;) {
        ev = Sys_GetEvent();
        if (!ev.evType) {
            if ((ev.evTime - startTime) * 0.001 >= (double)s)
                return;
            continue;
        }
        {
            int head = com_pushedEventsHead;
            sysEvent_t *slot = &com_pushedEvents[(unsigned char)head];
            if (head - com_pushedEventsTail > 0xff) {
                if (!printedWarning) {
                    printedWarning = 1;
                    Com_Printf("WARNING: Com_PushEvent overflow\n");
                }
                if (slot->evPtr)
                    Z_FreeInternal(slot->evPtr);
                com_pushedEventsTail++;
            } else {
                printedWarning = 0;
            }
            *slot = ev;
            com_pushedEventsHead++;
        }
    }
}

void Com_Init_Try_Block_Function(char *commandLine)
{
    clientStatic_t *clsg = (clientStatic_t *)imp_cls;
    extern void Swap_Init(void);
    extern void Cbuf_Init(void);
    extern void Cmd_Init(void);
    extern void CL_InitKeyCommands(void);
    extern void FS_InitFilesystem(void);
    extern const dvar_t *Dvar_RegisterBool(const char *name, int value, int flags);
    extern const dvar_t *Dvar_RegisterFloat(const char *name, float value, float min, float max, int flags);
    extern const dvar_t *Dvar_RegisterString(const char *name, const char *value, int flags);
    extern void Dvar_SetString(const dvar_t *dvar, const char *value);
    extern void Dvar_SetInt(const dvar_t *dvar, int value);
    extern void Dvar_SetBool(const dvar_t *dvar, int value);
    extern void Com_InitPlayerProfiles(void);
    extern void SEH_InitLanguage(void);
    extern void SEH_UpdateLanguageInfo(void);
    extern void Com_InitHunkMemory(void);
    extern void Cmd_AddCommand(const char *name, void (*function)(void));
    extern char *getBuildNumber(void);
    extern void FxMem_Init(void);
    extern void Sys_Init(void);
    extern void Netchan_Init(int port);
    extern void Scr_Init(void);
    extern void Scr_Settings(int enabled, int developer_script, int developer);
    extern void XAnimInit(void);
    extern void DObjInit(void);
    extern void SV_Init(void);
    extern void NET_Init(void);
    extern void CL_InitOnceForAllClients(void);
    extern void CL_Init(void);
    extern void CL_InitRenderer(void);
    extern void SND_Init(void);
    extern void Sys_HideSplashWindow(void);
    extern void Sys_LoadingKeepAlive(void);
    extern void Cbuf_AddText(const char *text);
    extern int I_strnicmp(const char *s1, const char *s2, size_t count);
    char *p;
    int i;

    Com_Printf("%s %s build %s %s\n", "CoD2 MP", COD2_VERSION_SHORT, "MacOSXS-i386", COD2_VERSION_DATE);

    memset(com_pushedEvents, 0, sizeof(com_pushedEvents));
    com_pushedEventsHead = 0;
    com_pushedEventsTail = 0;

    com_consoleLines[0] = commandLine;
    com_numConsoleLines = 1;
    p = commandLine;
    while (*p) {
        if (*p == '+' || *p == '\n') {
            if (com_numConsoleLines == 32)
                break;
            com_consoleLines[com_numConsoleLines] = p + 1;
            com_numConsoleLines++;
            *p = '\0';
            p++;
            continue;
        }
        p++;
    }

    Swap_Init();
    Cbuf_Init();
    Cmd_Init();
    Com_StartupVariable(NULL);
    Com_StartupVariable("developer");
    CL_InitKeyCommands();
    FS_InitFilesystem();

    {
        int dedicated_val;
        com_dedicated = Dvar_RegisterInt("dedicated", 0, 0, 2, 0x1020);
        dedicated_val = com_dedicated->current.integer;
        if (dedicated_val) {
            Dvar_RegisterInt("dedicated", 0, 0, 2, 0x1040);
        }
    }

    com_maxfps = Dvar_RegisterInt("com_maxfps", 85, 0, 1000, 0x1001);
    com_developer = Dvar_RegisterInt("developer", 0, 0, 2, 0x1000);
    com_developer_script = Dvar_RegisterBool("developer_script", 0, 0x1000);
    com_logfile = Dvar_RegisterInt("logfile", 0, 0, 2, 0x1000);
    com_statmon = Dvar_RegisterBool("com_statmon", 0, 0x1000);
    com_timescale = Dvar_RegisterFloat("timescale", 1.0f, 0.001f, 1000.0f, 0x1088);
    com_fixedtime = Dvar_RegisterInt("fixedtime", 0, 0, 1000, 0x1080);
    com_viewlog = Dvar_RegisterInt("viewlog", 0, 0, 2, 0x1080);
    sv_paused = Dvar_RegisterInt("sv_paused", 0, 0, 2, 0x1040);
    cl_paused = Dvar_RegisterInt("cl_paused", 0, 0, 2, 0x1040);
    com_sv_running = Dvar_RegisterBool("sv_running", 0, 0x1040);

    (*(LegacyHacks **)imp_legacyHacks)->cl_running = 0;

    com_introPlayed = Dvar_RegisterBool("com_introPlayed", 0, 0x1001);
    com_animCheck = Dvar_RegisterBool("com_animCheck", 0, 0x1000);

    {
        if (com_dedicated->current.integer) {
            if (com_viewlog->current.integer == 0) {
                Dvar_SetInt(com_viewlog, 1);
            }
        }
    }

    CL_SwitchToLocalClient(0);
    Com_InitPlayerProfiles();
    CL_SwitchToLocalClient(0);
    Cbuf_Execute();

    com_recommendedSet = Dvar_RegisterBool("com_recommendedSet", 0, 0x1001);
    Com_CheckSetRecommended();
    Com_StartupVariable(NULL);
    SEH_InitLanguage();
    SEH_UpdateLanguageInfo();

    {
        if (com_dedicated->current.integer) {
            Sys_HideSplashWindow();
            Sys_ShowConsole(1, 1);
            Sys_NormalExit();
        }
    }

    Com_InitHunkMemory();

    *(int *)imp_dvar_modifiedFlags &= 0xfffffffe;
    com_codeTimeScale = 1.0f;

    if (com_developer->current.integer) {
        Cmd_AddCommand("error", Com_Error_f);
        Cmd_AddCommand("crash", Com_Crash_f);
        Cmd_AddCommand("freeze", Com_Freeze_f);
    }

    Cmd_AddCommand("quit", Com_Quit_f);
    Cmd_AddCommand("writeconfig", Com_WriteConfig_f);
    Cmd_AddCommand("writedefaults", Com_WriteDefaults_f);

    {
        char *s = va("%s %s build %s %s", "CoD2 MP", COD2_VERSION_SHORT, getBuildNumber(), "MacOSXS-i386");
        version = Dvar_RegisterString("version", "", 0x1040);
        Dvar_SetString(version, s);
    }
    shortversion = Dvar_RegisterString("shortversion", COD2_VERSION_SHORT, 0x1044);

    FxMem_Init();
    Sys_Init();

    Netchan_Init(Com_Milliseconds() & 0xffff);

    Scr_Init();
    {
        int dev = com_developer->current.integer;
        int enabled = (dev || com_logfile->current.integer) ? 1 : 0;
        Scr_Settings(enabled, com_developer_script->current.enabled, dev);
    }

    XAnimInit();
    DObjInit();
    SV_Init();
    NET_Init();

    {
        const dvar_t *ded;
        int dedicated_val;
        ded = com_dedicated;
        Dvar_ClearModified(ded);
        dedicated_val = ded->current.integer;
#ifdef DEDICATED

        dedicated_val = 2;
#endif
        if (!dedicated_val) {
            CL_InitOnceForAllClients();
            CL_SwitchToLocalClient(0);
            CL_Init();
            CL_SwitchToLocalClient(0);
            Sys_ShowConsole(com_viewlog->current.integer, 0);
        }
    }

    com_frameTime = Com_Milliseconds();

    for (i = 0; i < com_numConsoleLines; i++) {
        if (!com_consoleLines[i] || com_consoleLines[i][0] == '\0')
            continue;
        I_strnicmp(com_consoleLines[i], "set", 3);
        Cbuf_AddText(com_consoleLines[i]);
        Cbuf_AddText("\n");
    }

    {
        int dedicated_val;
        dedicated_val = com_dedicated->current.integer;
#ifdef DEDICATED
        dedicated_val = 2;
#endif
        if (dedicated_val) {

            Sys_LoadingKeepAlive();
        } else {

            {
                char *cls_ptr;
                cls_ptr = (char *)imp_cls;
                clsg->rendererStarted = 1;
                CL_InitRenderer();
                clsg->soundStarted = 1;
            }
            SND_Init();
            Sys_LoadingKeepAlive();

            dedicated_val = com_dedicated->current.integer;
            if (!dedicated_val) {
                Sys_ShowConsole(com_viewlog->current.integer, 0);
            }
        }

        dedicated_val = com_dedicated->current.integer;
#ifdef DEDICATED
        dedicated_val = 2;
#endif
        if (!dedicated_val) {
            if (!com_introPlayed->current.enabled && !Com_HasStartupCommandsOtherThanSet()) {
                Cbuf_AddText("cinematic atvi\n");
                Dvar_SetString(nextmap, "cinematic IW_logo; set nextmap cinematic cod_intro");
                Dvar_SetBool(com_introPlayed, 1);
            }
        }
    }

    com_fullyInitialized = 1;
    Com_Printf("--- Common Initialization Complete ---\n");
    Cbuf_Execute();

    if (!com_sv_running->current.enabled) {
        UI_SetMap("", "");
        CL_StartHunkUsers();
    }
}

void Com_Init(char *commandLine)
{
    jmp_buf *abortframe = (jmp_buf *)Sys_GetValue(2);

    if (!setjmp(*abortframe)) {
        Com_Init_Try_Block_Function(commandLine);
        return;
    }

    Com_Printf("[REALERROR] %s\n", com_errorMessage);
    Sys_Error(va("Error during initialization:\n%s\n", com_errorMessage));
}

char cl_cdkey[52] = { 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
char cl_cdkeychecksum[12] = { 0x20, 0x20, 0x20, 0x20, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

const char str_00215810[] = "mac.display.rect";
const char str_00215824[] = "mac.display.index";
const char str_00215838[] = "mac.display.first";
const char str_00215cf8[] = "Posted-04/18/06";
const char str_00215d08[] = "2FF037E8990B11DAA29C000A95675086";
const char str_00215f8c[] = "EXE_SERVER_DISCONNECTED";
const char str_00215fa4[] = "EXE_DISCONNECTED";
const char str_00215fb8[] = "EXE_SERVERISFULL";
const char str_00215fcc[] = "XBOXLIVE_SIGNEDOUTOFLIVE";
const char str_00215fe8[] = "XBOXLIVE_CANTJOINSESSION";
const char str_00216004[] = "XBOXLIVE_MPNOTALLOWED";
const char str_0021601c[] = "XBOXLIVE_MUSTLOGIN";
const char str_0021699c[] = "english";
const char str_002169a4[] = "french";
const char str_002169ac[] = "german";
const char str_002169b4[] = "italian";
const char str_002169bc[] = "spanish";
const char str_002169c4[] = "british";
const char str_002169cc[] = "russian";
const char str_002169d4[] = "polish";
const char str_002169dc[] = "korean";
const char str_002169e4[] = "taiwanese";
const char str_002169f0[] = "japanese";
const char str_002169fc[] = "chinese";
const char str_00216a04[] = "thai";
const char str_00216a0c[] = "leet";
const char str_00216cd8[] = "menu";
const char str_002181d8[] = "item_health_small";
const char str_002181ec[] = "health_pickup_small";
const char str_00218200[] = "xmodel/health_small";
const char str_00218214[] = "icons/iconh_small";
const char str_00218228[] = "Small Health";
const char str_00218238[] = "item_health_large";
const char str_0021824c[] = "health_pickup_large";
const char str_00218260[] = "xmodel/health_large";
const char str_00218274[] = "icons/iconh_large";
const char str_00218288[] = "Large Health";
const char str_00218298[] = "none";
const char str_002182a8[] = "shellshock";
const char str_00218e0c[] = "generic";
const char str_00218e14[] = "paddedcell";
const char str_00218e20[] = "room";
const char str_00218e28[] = "bathroom";
const char str_00218e34[] = "livingroom";
const char str_00218e40[] = "stoneroom";
const char str_00218e4c[] = "auditorium";
const char str_00218e58[] = "concerthall";
const char str_00218e64[] = "cave";
const char str_00218e6c[] = "arena";
const char str_00218e74[] = "hangar";
const char str_00218e7c[] = "carpetedhallway";
const char str_00218e8c[] = "hallway";
const char str_00218e94[] = "stonecorridor";
const char str_00218ea4[] = "alley";
const char str_00218eac[] = "forest";
const char str_00218eb4[] = "city";
const char str_00218ebc[] = "mountains";
const char str_00218ec8[] = "quarry";
const char str_00218ed0[] = "plain";
const char str_00218ed8[] = "parkinglot";
const char str_00218ee4[] = "sewerpipe";
const char str_00218ef0[] = "underwater";
const char str_00218efc[] = "drugged";
const char str_00218f04[] = "dizzy";
const char str_00218f0c[] = "psychotic";
const char str_00219524[] = "off";
const char str_00219528[] = "on";
const char str_0021a1a4[] = "linear";
const char str_0021a4ec[] = "playfx";
const char str_0021a790[] = "length";
const char str_0021a974[] = "EV_NONE";
const char str_0021a97c[] = "EV_FOOTSTEP_RUN_DEFAULT";
const char str_0021a994[] = "EV_FOOTSTEP_RUN_BARK";
const char str_0021a9ac[] = "EV_FOOTSTEP_RUN_BRICK";
const char str_0021a9c4[] = "EV_FOOTSTEP_RUN_CARPET";
const char str_0021a9dc[] = "EV_FOOTSTEP_RUN_CLOTH";
const char str_0021a9f4[] = "EV_FOOTSTEP_RUN_CONCRETE";
const char str_0021aa10[] = "EV_FOOTSTEP_RUN_DIRT";
const char str_0021aa28[] = "EV_FOOTSTEP_RUN_FLESH";
const char str_0021aa40[] = "EV_FOOTSTEP_RUN_FOLIAGE";
const char str_0021aa58[] = "EV_FOOTSTEP_RUN_GLASS";
const char str_0021aa70[] = "EV_FOOTSTEP_RUN_GRASS";
const char str_0021aa88[] = "EV_FOOTSTEP_RUN_GRAVEL";
const char str_0021aaa0[] = "EV_FOOTSTEP_RUN_ICE";
const char str_0021aab4[] = "EV_FOOTSTEP_RUN_METAL";
const char str_0021aacc[] = "EV_FOOTSTEP_RUN_MUD";
const char str_0021aae0[] = "EV_FOOTSTEP_RUN_PAPER";
const char str_0021aaf8[] = "EV_FOOTSTEP_RUN_PLASTER";
const char str_0021ab10[] = "EV_FOOTSTEP_RUN_ROCK";
const char str_0021ab28[] = "EV_FOOTSTEP_RUN_SAND";
const char str_0021ab40[] = "EV_FOOTSTEP_RUN_SNOW";
const char str_0021ab58[] = "EV_FOOTSTEP_RUN_WATER";
const char str_0021ab70[] = "EV_FOOTSTEP_RUN_WOOD";
const char str_0021ab88[] = "EV_FOOTSTEP_RUN_ASPHALT";
const char str_0021aba0[] = "EV_FOOTSTEP_WALK_DEFAULT";
const char str_0021abbc[] = "EV_FOOTSTEP_WALK_BARK";
const char str_0021abd4[] = "EV_FOOTSTEP_WALK_BRICK";
const char str_0021abec[] = "EV_FOOTSTEP_WALK_CARPET";
const char str_0021ac04[] = "EV_FOOTSTEP_WALK_CLOTH";
const char str_0021ac1c[] = "EV_FOOTSTEP_WALK_CONCRETE";
const char str_0021ac38[] = "EV_FOOTSTEP_WALK_DIRT";
const char str_0021ac50[] = "EV_FOOTSTEP_WALK_FLESH";
const char str_0021ac68[] = "EV_FOOTSTEP_WALK_FOLIAGE";
const char str_0021ac84[] = "EV_FOOTSTEP_WALK_GLASS";
const char str_0021ac9c[] = "EV_FOOTSTEP_WALK_GRASS";
const char str_0021acb4[] = "EV_FOOTSTEP_WALK_GRAVEL";
const char str_0021accc[] = "EV_FOOTSTEP_WALK_ICE";
const char str_0021ace4[] = "EV_FOOTSTEP_WALK_METAL";
const char str_0021acfc[] = "EV_FOOTSTEP_WALK_MUD";
const char str_0021ad14[] = "EV_FOOTSTEP_WALK_PAPER";
const char str_0021ad2c[] = "EV_FOOTSTEP_WALK_PLASTER";
const char str_0021ad48[] = "EV_FOOTSTEP_WALK_ROCK";
const char str_0021ad60[] = "EV_FOOTSTEP_WALK_SAND";
const char str_0021ad78[] = "EV_FOOTSTEP_WALK_SNOW";
const char str_0021ad90[] = "EV_FOOTSTEP_WALK_WATER";
const char str_0021ada8[] = "EV_FOOTSTEP_WALK_WOOD";
const char str_0021adc0[] = "EV_FOOTSTEP_WALK_ASPHALT";
const char str_0021addc[] = "EV_FOOTSTEP_PRONE_DEFAULT";
const char str_0021adf8[] = "EV_FOOTSTEP_PRONE_BARK";
const char str_0021ae10[] = "EV_FOOTSTEP_PRONE_BRICK";
const char str_0021ae28[] = "EV_FOOTSTEP_PRONE_CARPET";
const char str_0021ae44[] = "EV_FOOTSTEP_PRONE_CLOTH";
const char str_0021ae5c[] = "EV_FOOTSTEP_PRONE_CONCRETE";
const char str_0021ae78[] = "EV_FOOTSTEP_PRONE_DIRT";
const char str_0021ae90[] = "EV_FOOTSTEP_PRONE_FLESH";
const char str_0021aea8[] = "EV_FOOTSTEP_PRONE_FOLIAGE";
const char str_0021aec4[] = "EV_FOOTSTEP_PRONE_GLASS";
const char str_0021aedc[] = "EV_FOOTSTEP_PRONE_GRASS";
const char str_0021aef4[] = "EV_FOOTSTEP_PRONE_GRAVEL";
const char str_0021af10[] = "EV_FOOTSTEP_PRONE_ICE";
const char str_0021af28[] = "EV_FOOTSTEP_PRONE_METAL";
const char str_0021af40[] = "EV_FOOTSTEP_PRONE_MUD";
const char str_0021af58[] = "EV_FOOTSTEP_PRONE_PAPER";
const char str_0021af70[] = "EV_FOOTSTEP_PRONE_PLASTER";
const char str_0021af8c[] = "EV_FOOTSTEP_PRONE_ROCK";
const char str_0021afa4[] = "EV_FOOTSTEP_PRONE_SAND";
const char str_0021afbc[] = "EV_FOOTSTEP_PRONE_SNOW";
const char str_0021afd4[] = "EV_FOOTSTEP_PRONE_WATER";
const char str_0021afec[] = "EV_FOOTSTEP_PRONE_WOOD";
const char str_0021b004[] = "EV_FOOTSTEP_PRONE_ASPHALT";
const char str_0021b020[] = "EV_JUMP_DEFAULT";
const char str_0021b030[] = "EV_JUMP_BARK";
const char str_0021b040[] = "EV_JUMP_BRICK";
const char str_0021b050[] = "EV_JUMP_CARPET";
const char str_0021b060[] = "EV_JUMP_CLOTH";
const char str_0021b070[] = "EV_JUMP_CONCRETE";
const char str_0021b084[] = "EV_JUMP_DIRT";
const char str_0021b094[] = "EV_JUMP_FLESH";
const char str_0021b0a4[] = "EV_JUMP_FOLIAGE";
const char str_0021b0b4[] = "EV_JUMP_GLASS";
const char str_0021b0c4[] = "EV_JUMP_GRASS";
const char str_0021b0d4[] = "EV_JUMP_GRAVEL";
const char str_0021b0e4[] = "EV_JUMP_ICE";
const char str_0021b0f0[] = "EV_JUMP_METAL";
const char str_0021b100[] = "EV_JUMP_MUD";
const char str_0021b10c[] = "EV_JUMP_PAPER";
const char str_0021b11c[] = "EV_JUMP_PLASTER";
const char str_0021b12c[] = "EV_JUMP_ROCK";
const char str_0021b13c[] = "EV_JUMP_SAND";
const char str_0021b14c[] = "EV_JUMP_SNOW";
const char str_0021b15c[] = "EV_JUMP_WATER";
const char str_0021b16c[] = "EV_JUMP_WOOD";
const char str_0021b17c[] = "EV_JUMP_ASPHALT";
const char str_0021b18c[] = "EV_LANDING_DEFAULT";
const char str_0021b1a0[] = "EV_LANDING_BARK";
const char str_0021b1b0[] = "EV_LANDING_BRICK";
const char str_0021b1c4[] = "EV_LANDING_CARPET";
const char str_0021b1d8[] = "EV_LANDING_CLOTH";
const char str_0021b1ec[] = "EV_LANDING_CONCRETE";
const char str_0021b200[] = "EV_LANDING_DIRT";
const char str_0021b210[] = "EV_LANDING_FLESH";
const char str_0021b224[] = "EV_LANDING_FOLIAGE";
const char str_0021b238[] = "EV_LANDING_GLASS";
const char str_0021b24c[] = "EV_LANDING_GRASS";
const char str_0021b260[] = "EV_LANDING_GRAVEL";
const char str_0021b274[] = "EV_LANDING_ICE";
const char str_0021b284[] = "EV_LANDING_METAL";
const char str_0021b298[] = "EV_LANDING_MUD";
const char str_0021b2a8[] = "EV_LANDING_PAPER";
const char str_0021b2bc[] = "EV_LANDING_PLASTER";
const char str_0021b2d0[] = "EV_LANDING_ROCK";
const char str_0021b2e0[] = "EV_LANDING_SAND";
const char str_0021b2f0[] = "EV_LANDING_SNOW";
const char str_0021b300[] = "EV_LANDING_WATER";
const char str_0021b314[] = "EV_LANDING_WOOD";
const char str_0021b324[] = "EV_LANDING_ASPHALT";
const char str_0021b338[] = "EV_LANDING_PAIN_DEFAULT";
const char str_0021b350[] = "EV_LANDING_PAIN_BARK";
const char str_0021b368[] = "EV_LANDING_PAIN_BRICK";
const char str_0021b380[] = "EV_LANDING_PAIN_CARPET";
const char str_0021b398[] = "EV_LANDING_PAIN_CLOTH";
const char str_0021b3b0[] = "EV_LANDING_PAIN_CONCRETE";
const char str_0021b3cc[] = "EV_LANDING_PAIN_DIRT";
const char str_0021b3e4[] = "EV_LANDING_PAIN_FLESH";
const char str_0021b3fc[] = "EV_LANDING_PAIN_FOLIAGE";
const char str_0021b414[] = "EV_LANDING_PAIN_GLASS";
const char str_0021b42c[] = "EV_LANDING_PAIN_GRASS";
const char str_0021b444[] = "EV_LANDING_PAIN_GRAVEL";
const char str_0021b45c[] = "EV_LANDING_PAIN_ICE";
const char str_0021b470[] = "EV_LANDING_PAIN_METAL";
const char str_0021b488[] = "EV_LANDING_PAIN_MUD";
const char str_0021b49c[] = "EV_LANDING_PAIN_PAPER";
const char str_0021b4b4[] = "EV_LANDING_PAIN_PLASTER";
const char str_0021b4cc[] = "EV_LANDING_PAIN_ROCK";
const char str_0021b4e4[] = "EV_LANDING_PAIN_SAND";
const char str_0021b4fc[] = "EV_LANDING_PAIN_SNOW";
const char str_0021b514[] = "EV_LANDING_PAIN_WATER";
const char str_0021b52c[] = "EV_LANDING_PAIN_WOOD";
const char str_0021b544[] = "EV_LANDING_PAIN_ASPHALT";
const char str_0021b55c[] = "EV_FOLIAGE_SOUND";
const char str_0021b570[] = "EV_STANCE_FORCE_STAND";
const char str_0021b588[] = "EV_STANCE_FORCE_CROUCH";
const char str_0021b5a0[] = "EV_STANCE_FORCE_PRONE";
const char str_0021b5b8[] = "EV_STEP_VIEW";
const char str_0021b5c8[] = "EV_ITEM_PICKUP";
const char str_0021b5d8[] = "EV_AMMO_PICKUP";
const char str_0021b5e8[] = "EV_NOAMMO";
const char str_0021b5f4[] = "EV_EMPTYCLIP";
const char str_0021b604[] = "EV_EMPTY_OFFHAND";
const char str_0021b618[] = "EV_RESET_ADS";
const char str_0021b628[] = "EV_RELOAD";
const char str_0021b634[] = "EV_RELOAD_FROM_EMPTY";
const char str_0021b64c[] = "EV_RELOAD_START";
const char str_0021b65c[] = "EV_RELOAD_END";
const char str_0021b66c[] = "EV_RAISE_WEAPON";
const char str_0021b67c[] = "EV_PUTAWAY_WEAPON";
const char str_0021b690[] = "EV_WEAPON_ALT";
const char str_0021b6a0[] = "EV_PULLBACK_WEAPON";
const char str_0021b6b4[] = "EV_FIRE_WEAPON";
const char str_0021b6c4[] = "EV_FIRE_WEAPONB";
const char str_0021b6d4[] = "EV_FIRE_WEAPON_LASTSHOT";
const char str_0021b6ec[] = "EV_RECHAMBER_WEAPON";
const char str_0021b700[] = "EV_EJECT_BRASS";
const char str_0021b710[] = "EV_MELEE_SWIPE";
const char str_0021b720[] = "EV_FIRE_MELEE";
const char str_0021b730[] = "EV_PREP_OFFHAND";
const char str_0021b740[] = "EV_USE_OFFHAND";
const char str_0021b750[] = "EV_SWITCH_OFFHAND";
const char str_0021b764[] = "EV_BINOCULAR_ENTER";
const char str_0021b778[] = "EV_BINOCULAR_EXIT";
const char str_0021b78c[] = "EV_BINOCULAR_FIRE";
const char str_0021b7a0[] = "EV_BINOCULAR_RELEASE";
const char str_0021b7b8[] = "EV_BINOCULAR_DROP";
const char str_0021b7cc[] = "EV_MELEE_HIT";
const char str_0021b7dc[] = "EV_MELEE_MISS";
const char str_0021b7ec[] = "EV_FIRE_WEAPON_MG42";
const char str_0021b800[] = "EV_FIRE_QUADBARREL_1";
const char str_0021b818[] = "EV_FIRE_QUADBARREL_2";
const char str_0021b830[] = "EV_BULLET_TRACER";
const char str_0021b844[] = "EV_SOUND_ALIAS";
const char str_0021b854[] = "EV_SOUND_ALIAS_AS_MASTER";
const char str_0021b870[] = "EV_BULLET_HIT_SMALL";
const char str_0021b884[] = "EV_BULLET_HIT_LARGE";
const char str_0021b898[] = "EV_SHOTGUN_HIT";
const char str_0021b8a8[] = "EV_BULLET_HIT_AP";
const char str_0021b8bc[] = "EV_BULLET_HIT_CLIENT_SMALL";
const char str_0021b8d8[] = "EV_BULLET_HIT_CLIENT_LARGE";
const char str_0021b8f4[] = "EV_GRENADE_BOUNCE";
const char str_0021b908[] = "EV_GRENADE_EXPLODE";
const char str_0021b91c[] = "EV_ROCKET_EXPLODE";
const char str_0021b930[] = "EV_ROCKET_EXPLODE_NOMARKS";
const char str_0021b94c[] = "EV_CUSTOM_EXPLODE";
const char str_0021b960[] = "EV_CUSTOM_EXPLODE_NOMARKS";
const char str_0021b97c[] = "EV_BULLET";
const char str_0021b988[] = "EV_PLAY_FX";
const char str_0021b994[] = "EV_PLAY_FX_ON_TAG";
const char str_0021b9a8[] = "EV_EARTHQUAKE";
const char str_0021b9b8[] = "EV_GRENADE_SUICIDE";
const char str_0021b9cc[] = "EV_OBITUARY";
const char str_0021dd64[] = "loopsync";
const char str_0021dd70[] = "nonloopsync";
const char str_0021dd7c[] = "complete";
const char str_0021dfa4[] = "mp_mantle_root";
const char str_0021dfb4[] = "mp_mantle_up_57";
const char str_0021dfc4[] = "mp_mantle_up_51";
const char str_0021dfd4[] = "mp_mantle_up_45";
const char str_0021dfe4[] = "mp_mantle_up_39";
const char str_0021dff4[] = "mp_mantle_up_33";
const char str_0021e004[] = "mp_mantle_up_27";
const char str_0021e014[] = "mp_mantle_up_21";
const char str_0021e024[] = "mp_mantle_over_high";
const char str_0021e038[] = "mp_mantle_over_mid";
const char str_0021e04c[] = "player_mantle_over_low";
const char str_0021eaec[] = "sequence";
const char str_0021eaf8[] = "file";
const char str_0021eb00[] = "subtitle";
const char str_0021eb0c[] = "vol_min";
const char str_0021eb14[] = "vol_max";
const char str_0021eb1c[] = "vol_mod";
const char str_0021eb24[] = "pitch_min";
const char str_0021eb30[] = "pitch_max";
const char str_0021eb3c[] = "dist_min";
const char str_0021eb48[] = "dist_max";
const char str_0021eb54[] = "channel";
const char str_0021eb64[] = "loop";
const char str_0021eb6c[] = "probability";
const char str_0021eb78[] = "loadspec";
const char str_0021eb84[] = "masterslave";
const char str_0021eb90[] = "secondaryaliasname";
const char str_0021eba4[] = "volumefalloffcurve";
const char str_0021ebb8[] = "startdelay";
const char str_0021ebc4[] = "speakermap";
const char str_0021ebd0[] = "reverb";
const char str_0021ebd8[] = "lfe percentage";
const char str_0021ebe8[] = "auto";
const char str_0021ebf0[] = "auto2d";
const char str_0021ebf8[] = "weapon";
const char str_0021ec00[] = "voice";
const char str_0021ec08[] = "item";
const char str_0021ec10[] = "body";
const char str_0021ec18[] = "local";
const char str_0021ec20[] = "music";
const char str_0021ec28[] = "announcer";
const char str_00220284[] = "default";
const char str_0022028c[] = "displayName";
const char str_00220298[] = "AIOverlayDescription";
const char str_002202b0[] = "modeName";
const char str_002202bc[] = "playerAnimType";
const char str_002202cc[] = "gunModel";
const char str_002202d8[] = "handModel";
const char str_002202e4[] = "idleAnim";
const char str_002202f0[] = "emptyIdleAnim";
const char str_00220300[] = "fireAnim";
const char str_0022030c[] = "holdFireAnim";
const char str_0022031c[] = "lastShotAnim";
const char str_0022032c[] = "rechamberAnim";
const char str_0022033c[] = "meleeAnim";
const char str_00220348[] = "reloadAnim";
const char str_00220354[] = "reloadEmptyAnim";
const char str_00220364[] = "reloadStartAnim";
const char str_00220374[] = "reloadEndAnim";
const char str_00220384[] = "raiseAnim";
const char str_00220390[] = "dropAnim";
const char str_0022039c[] = "altRaiseAnim";
const char str_002203ac[] = "altDropAnim";
const char str_002203b8[] = "quickRaiseAnim";
const char str_002203c8[] = "quickDropAnim";
const char str_002203d8[] = "adsFireAnim";
const char str_002203e4[] = "adsLastShotAnim";
const char str_002203f4[] = "adsRechamberAnim";
const char str_00220408[] = "adsUpAnim";
const char str_00220414[] = "adsDownAnim";
const char str_00220420[] = "script";
const char str_00220428[] = "weaponType";
const char str_00220434[] = "weaponClass";
const char str_00220440[] = "weaponSlot";
const char str_0022044c[] = "slotStackable";
const char str_0022045c[] = "offhandClass";
const char str_0022046c[] = "viewFlashEffect";
const char str_0022047c[] = "worldFlashEffect";
const char str_00220490[] = "pickupSound";
const char str_0022049c[] = "ammoPickupSound";
const char str_002204ac[] = "projectileSound";
const char str_002204bc[] = "pullbackSound";
const char str_002204cc[] = "fireSound";
const char str_002204d8[] = "fireSoundPlayer";
const char str_002204e8[] = "loopFireSound";
const char str_002204f8[] = "loopFireSoundPlayer";
const char str_0022050c[] = "stopFireSound";
const char str_0022051c[] = "stopFireSoundPlayer";
const char str_00220530[] = "lastShotSound";
const char str_00220540[] = "lastShotSoundPlayer";
const char str_00220554[] = "meleeSwipeSound";
const char str_00220564[] = "rechamberSound";
const char str_00220574[] = "rechamberSoundPlayer";
const char str_0022058c[] = "reloadSound";
const char str_00220598[] = "reloadSoundPlayer";
const char str_002205ac[] = "reloadEmptySound";
const char str_002205c0[] = "reloadEmptySoundPlayer";
const char str_002205d8[] = "reloadStartSound";
const char str_002205ec[] = "reloadStartSoundPlayer";
const char str_00220604[] = "reloadEndSound";
const char str_00220614[] = "reloadEndSoundPlayer";
const char str_0022062c[] = "raiseSound";
const char str_00220638[] = "altSwitchSound";
const char str_00220648[] = "putawaySound";
const char str_00220658[] = "noteTrackSoundA";
const char str_00220668[] = "noteTrackSoundB";
const char str_00220678[] = "noteTrackSoundC";
const char str_00220688[] = "noteTrackSoundD";
const char str_00220698[] = "shellEjectEffect";
const char str_002206ac[] = "lastShotEjectEffect";
const char str_002206c0[] = "reticleCenter";
const char str_002206d0[] = "reticleSide";
const char str_002206dc[] = "reticleCenterSize";
const char str_002206f0[] = "reticleSideSize";
const char str_00220700[] = "reticleMinOfs";
const char str_00220710[] = "standMoveF";
const char str_0022071c[] = "standMoveR";
const char str_00220728[] = "standMoveU";
const char str_00220734[] = "standRotP";
const char str_00220740[] = "standRotY";
const char str_0022074c[] = "standRotR";
const char str_00220758[] = "duckedOfsF";
const char str_00220764[] = "duckedOfsR";
const char str_00220770[] = "duckedOfsU";
const char str_0022077c[] = "duckedMoveF";
const char str_00220788[] = "duckedMoveR";
const char str_00220794[] = "duckedMoveU";
const char str_002207a0[] = "duckedRotP";
const char str_002207ac[] = "duckedRotY";
const char str_002207b8[] = "duckedRotR";
const char str_002207c4[] = "proneOfsF";
const char str_002207d0[] = "proneOfsR";
const char str_002207dc[] = "proneOfsU";
const char str_002207e8[] = "proneMoveF";
const char str_002207f4[] = "proneMoveR";
const char str_00220800[] = "proneMoveU";
const char str_0022080c[] = "proneRotP";
const char str_00220818[] = "proneRotY";
const char str_00220824[] = "proneRotR";
const char str_00220830[] = "posMoveRate";
const char str_0022083c[] = "posProneMoveRate";
const char str_00220850[] = "standMoveMinSpeed";
const char str_00220864[] = "duckedMoveMinSpeed";
const char str_00220878[] = "proneMoveMinSpeed";
const char str_0022088c[] = "posRotRate";
const char str_00220898[] = "posProneRotRate";
const char str_002208a8[] = "standRotMinSpeed";
const char str_002208bc[] = "duckedRotMinSpeed";
const char str_002208d0[] = "proneRotMinSpeed";
const char str_002208e4[] = "worldModel";
const char str_002208f0[] = "hudIcon";
const char str_002208f8[] = "modeIcon";
const char str_00220904[] = "startAmmo";
const char str_00220910[] = "ammoName";
const char str_0022091c[] = "clipName";
const char str_00220928[] = "maxAmmo";
const char str_00220930[] = "clipSize";
const char str_0022093c[] = "shotCount";
const char str_00220948[] = "sharedAmmoCapName";
const char str_0022095c[] = "sharedAmmoCap";
const char str_0022096c[] = "damage";
const char str_00220974[] = "playerDamage";
const char str_00220984[] = "meleeDamage";
const char str_00220990[] = "minDamage";
const char str_0022099c[] = "minPlayerDamage";
const char str_002209ac[] = "maxDamageRange";
const char str_002209bc[] = "minDamageRange";
const char str_002209cc[] = "destabilizationBaseTime";
const char str_002209e4[] = "destabilizationTimeReductionRatio";
const char str_00220a08[] = "destabilizationAngleMax";
const char str_00220a20[] = "destabilizeDistance";
const char str_00220a34[] = "fireDelay";
const char str_00220a40[] = "meleeDelay";
const char str_00220a4c[] = "fireTime";
const char str_00220a58[] = "rechamberTime";
const char str_00220a68[] = "rechamberBoltTime";
const char str_00220a7c[] = "holdFireTime";
const char str_00220a8c[] = "meleeTime";
const char str_00220a98[] = "reloadTime";
const char str_00220aa4[] = "reloadEmptyTime";
const char str_00220ab4[] = "reloadAddTime";
const char str_00220ac4[] = "reloadStartTime";
const char str_00220ad4[] = "reloadStartAddTime";
const char str_00220ae8[] = "reloadEndTime";
const char str_00220af8[] = "dropTime";
const char str_00220b04[] = "raiseTime";
const char str_00220b10[] = "altDropTime";
const char str_00220b1c[] = "altRaiseTime";
const char str_00220b2c[] = "quickDropTime";
const char str_00220b3c[] = "quickRaiseTime";
const char str_00220b4c[] = "fuseTime";
const char str_00220b58[] = "autoAimRange";
const char str_00220b68[] = "slowdownAimRange";
const char str_00220b7c[] = "slowdownAimRangeAds";
const char str_00220b90[] = "lockonAimRange";
const char str_00220ba0[] = "lockonAimRangeAds";
const char str_00220bb4[] = "enemyCrosshairRange";
const char str_00220bc8[] = "moveSpeedScale";
const char str_00220bd8[] = "idleCrouchFactor";
const char str_00220bec[] = "idleProneFactor";
const char str_00220bfc[] = "gunMaxPitch";
const char str_00220c08[] = "gunMaxYaw";
const char str_00220c14[] = "swayMaxAngle";
const char str_00220c24[] = "swayLerpSpeed";
const char str_00220c34[] = "swayPitchScale";
const char str_00220c44[] = "swayYawScale";
const char str_00220c54[] = "swayHorizScale";
const char str_00220c64[] = "swayVertScale";
const char str_00220c74[] = "swayShellShockScale";
const char str_00220c88[] = "adsSwayMaxAngle";
const char str_00220c98[] = "adsSwayLerpSpeed";
const char str_00220cac[] = "adsSwayPitchScale";
const char str_00220cc0[] = "adsSwayYawScale";
const char str_00220cd0[] = "adsSwayHorizScale";
const char str_00220ce4[] = "adsSwayVertScale";
const char str_00220cf8[] = "rifleBullet";
const char str_00220d04[] = "armorPiercing";
const char str_00220d14[] = "semiAuto";
const char str_00220d20[] = "boltAction";
const char str_00220d2c[] = "aimDownSight";
const char str_00220d3c[] = "rechamberWhileAds";
const char str_00220d50[] = "adsViewErrorMin";
const char str_00220d60[] = "adsViewErrorMax";
const char str_00220d70[] = "clipOnly";
const char str_00220d7c[] = "cookOffHold";
const char str_00220d88[] = "wideListIcon";
const char str_00220d98[] = "adsFire";
const char str_00220da0[] = "killIcon";
const char str_00220dac[] = "wideKillIcon";
const char str_00220dbc[] = "flipKillIcon";
const char str_00220dcc[] = "noPartialReload";
const char str_00220ddc[] = "segmentedReload";
const char str_00220dec[] = "reloadAmmoAdd";
const char str_00220dfc[] = "reloadStartAdd";
const char str_00220e0c[] = "altWeapon";
const char str_00220e18[] = "dropAmmoMin";
const char str_00220e24[] = "dropAmmoMax";
const char str_00220e30[] = "explosionRadius";
const char str_00220e40[] = "explosionInnerDamage";
const char str_00220e58[] = "explosionOuterDamage";
const char str_00220e70[] = "projectileSpeed";
const char str_00220e80[] = "projectileSpeedUp";
const char str_00220e94[] = "projectileModel";
const char str_00220ea4[] = "projExplosionType";
const char str_00220eb8[] = "projExplosionEffect";
const char str_00220ecc[] = "projExplosionSound";
const char str_00220ee0[] = "projImpactExplode";
const char str_00220ef4[] = "parallelDefaultBounce";
const char str_00220f0c[] = "parallelBarkBounce";
const char str_00220f20[] = "parallelBrickBounce";
const char str_00220f34[] = "parallelCarpetBounce";
const char str_00220f4c[] = "parallelClothBounce";
const char str_00220f60[] = "parallelConcreteBounce";
const char str_00220f78[] = "parallelDirtBounce";
const char str_00220f8c[] = "parallelFleshBounce";
const char str_00220fa0[] = "parallelFoliageBounce";
const char str_00220fb8[] = "parallelGlassBounce";
const char str_00220fcc[] = "parallelGrassBounce";
const char str_00220fe0[] = "parallelGravelBounce";
const char str_00220ff8[] = "parallelIceBounce";
const char str_0022100c[] = "parallelMetalBounce";
const char str_00221020[] = "parallelMudBounce";
const char str_00221034[] = "parallelPaperBounce";
const char str_00221048[] = "parallelPlasterBounce";
const char str_00221060[] = "parallelRockBounce";
const char str_00221074[] = "parallelSandBounce";
const char str_00221088[] = "parallelSnowBounce";
const char str_0022109c[] = "parallelWaterBounce";
const char str_002210b0[] = "parallelWoodBounce";
const char str_002210c4[] = "parallelAsphaltBounce";
const char str_002210dc[] = "perpendicularDefaultBounce";
const char str_002210f8[] = "perpendicularBarkBounce";
const char str_00221110[] = "perpendicularBrickBounce";
const char str_0022112c[] = "perpendicularCarpetBounce";
const char str_00221148[] = "perpendicularClothBounce";
const char str_00221164[] = "perpendicularConcreteBounce";
const char str_00221180[] = "perpendicularDirtBounce";
const char str_00221198[] = "perpendicularFleshBounce";
const char str_002211b4[] = "perpendicularFoliageBounce";
const char str_002211d0[] = "perpendicularGlassBounce";
const char str_002211ec[] = "perpendicularGrassBounce";
const char str_00221208[] = "perpendicularGravelBounce";
const char str_00221224[] = "perpendicularIceBounce";
const char str_0022123c[] = "perpendicularMetalBounce";
const char str_00221258[] = "perpendicularMudBounce";
const char str_00221270[] = "perpendicularPaperBounce";
const char str_0022128c[] = "perpendicularPlasterBounce";
const char str_002212a8[] = "perpendicularRockBounce";
const char str_002212c0[] = "perpendicularSandBounce";
const char str_002212d8[] = "perpendicularSnowBounce";
const char str_002212f0[] = "perpendicularWaterBounce";
const char str_0022130c[] = "perpendicularWoodBounce";
const char str_00221324[] = "perpendicularAsphaltBounce";
const char str_00221340[] = "projTrailEffect";
const char str_00221350[] = "projectileDLight";
const char str_00221364[] = "projectileRed";
const char str_00221374[] = "projectileGreen";
const char str_00221384[] = "projectileBlue";
const char str_00221394[] = "adsTransInTime";
const char str_002213a4[] = "adsTransOutTime";
const char str_002213b4[] = "adsIdleAmount";
const char str_002213c4[] = "adsIdleSpeed";
const char str_002213d4[] = "adsZoomFov";
const char str_002213e0[] = "adsZoomInFrac";
const char str_002213f0[] = "adsZoomOutFrac";
const char str_00221400[] = "adsOverlayShader";
const char str_00221414[] = "adsOverlayReticle";
const char str_00221428[] = "adsOverlayWidth";
const char str_00221438[] = "adsOverlayHeight";
const char str_0022144c[] = "adsBobFactor";
const char str_0022145c[] = "adsViewBobMult";
const char str_0022146c[] = "adsAimPitch";
const char str_00221478[] = "adsCrosshairInFrac";
const char str_0022148c[] = "adsCrosshairOutFrac";
const char str_002214a0[] = "adsReloadTransTime";
const char str_002214b4[] = "adsGunKickReducedKickBullets";
const char str_002214d4[] = "adsGunKickReducedKickPercent";
const char str_002214f4[] = "adsGunKickPitchMin";
const char str_00221508[] = "adsGunKickPitchMax";
const char str_0022151c[] = "adsGunKickYawMin";
const char str_00221530[] = "adsGunKickYawMax";
const char str_00221544[] = "adsGunKickAccel";
const char str_00221554[] = "adsGunKickSpeedMax";
const char str_00221568[] = "adsGunKickSpeedDecay";
const char str_00221580[] = "adsGunKickStaticDecay";
const char str_00221598[] = "adsViewKickPitchMin";
const char str_002215ac[] = "adsViewKickPitchMax";
const char str_002215c0[] = "adsViewKickYawMin";
const char str_002215d4[] = "adsViewKickYawMax";
const char str_002215e8[] = "adsViewKickCenterSpeed";
const char str_00221600[] = "adsSpread";
const char str_0022160c[] = "hipSpreadStandMin";
const char str_00221620[] = "hipSpreadDuckedMin";
const char str_00221634[] = "hipSpreadProneMin";
const char str_00221648[] = "hipSpreadMax";
const char str_00221658[] = "hipSpreadDuckedMax";
const char str_0022166c[] = "hipSpreadProneMax";
const char str_00221680[] = "hipSpreadDecayRate";
const char str_00221694[] = "hipSpreadFireAdd";
const char str_002216a8[] = "hipSpreadTurnAdd";
const char str_002216bc[] = "hipSpreadMoveAdd";
const char str_002216d0[] = "hipSpreadDuckedDecay";
const char str_002216e8[] = "hipSpreadProneDecay";
const char str_002216fc[] = "hipReticleSidePos";
const char str_00221710[] = "hipIdleAmount";
const char str_00221720[] = "hipIdleSpeed";
const char str_00221730[] = "hipGunKickReducedKickBullets";
const char str_00221750[] = "hipGunKickReducedKickPercent";
const char str_00221770[] = "hipGunKickPitchMin";
const char str_00221784[] = "hipGunKickPitchMax";
const char str_00221798[] = "hipGunKickYawMin";
const char str_002217ac[] = "hipGunKickYawMax";
const char str_002217c0[] = "hipGunKickAccel";
const char str_002217d0[] = "hipGunKickSpeedMax";
const char str_002217e4[] = "hipGunKickSpeedDecay";
const char str_002217fc[] = "hipGunKickStaticDecay";
const char str_00221814[] = "hipViewKickPitchMin";
const char str_00221828[] = "hipViewKickPitchMax";
const char str_0022183c[] = "hipViewKickYawMin";
const char str_00221850[] = "hipViewKickYawMax";
const char str_00221864[] = "hipViewKickCenterSpeed";
const char str_0022187c[] = "leftArc";
const char str_00221884[] = "rightArc";
const char str_00221890[] = "topArc";
const char str_00221898[] = "bottomArc";
const char str_002218a4[] = "accuracy";
const char str_002218b0[] = "aiSpread";
const char str_002218bc[] = "playerSpread";
const char str_002218cc[] = "maxVertTurnSpeed";
const char str_002218e0[] = "maxHorTurnSpeed";
const char str_002218f0[] = "minVertTurnSpeed";
const char str_00221904[] = "minHorTurnSpeed";
const char str_00221914[] = "pitchConvergenceTime";
const char str_0022192c[] = "yawConvergenceTime";
const char str_00221940[] = "suppressionTime";
const char str_00221950[] = "maxRange";
const char str_0022195c[] = "animHorRotateInc";
const char str_00221970[] = "playerPositionDist";
const char str_00221984[] = "stance";
const char str_0022198c[] = "useHintString";
const char str_0022199c[] = "dropHintString";
const char str_002219ac[] = "horizViewJitter";
const char str_002219bc[] = "vertViewJitter";
const char str_002219cc[] = "fightDist";
const char str_002219d8[] = "maxDist";
const char str_002219e0[] = "aiVsAiAccuracyGraph";
const char str_002219f4[] = "aiVsPlayerAccuracyGraph";
const char str_00221a0c[] = "locNone";
const char str_00221a14[] = "locHelmet";
const char str_00221a20[] = "locHead";
const char str_00221a28[] = "locNeck";
const char str_00221a30[] = "locTorsoUpper";
const char str_00221a40[] = "locTorsoLower";
const char str_00221a50[] = "locRightArmUpper";
const char str_00221a64[] = "locRightArmLower";
const char str_00221a78[] = "locRightHand";
const char str_00221a88[] = "locLeftArmUpper";
const char str_00221a98[] = "locLeftArmLower";
const char str_00221aa8[] = "locLeftHand";
const char str_00221ab4[] = "locRightLegUpper";
const char str_00221ac8[] = "locRightLegLower";
const char str_00221adc[] = "locRightFoot";
const char str_00221aec[] = "locLeftLegUpper";
const char str_00221afc[] = "locLeftLegLower";
const char str_00221b0c[] = "locLeftFoot";
const char str_00221b18[] = "locGun";
const char str_00221b20[] = "fireRumble";
const char str_00221b2c[] = "meleeImpactRumble";
const char str_00221b40[] = "None";
const char str_00221b48[] = "Frag Grenade";
const char str_00221b58[] = "Smoke Grenade";
const char str_00221b68[] = "grenade";
const char str_00221b70[] = "rocket";
const char str_00221b78[] = "stand";
const char str_00221b80[] = "duck";
const char str_00221b88[] = "prone";
const char str_00221b90[] = "primary";
const char str_00221b98[] = "primaryb";
const char str_00221ba4[] = "crosshair";
const char str_00221bb0[] = "FG42";
const char str_00221bb8[] = "Springfield";
const char str_00221bc4[] = "binoculars";
const char str_00221bd0[] = "rifle";
const char str_00221bd8[] = "mg";
const char str_00221bdc[] = "smg";
const char str_00221be0[] = "spread";
const char str_00221be8[] = "pistol";
const char str_00221bf0[] = "rocketlauncher";
const char str_00221c00[] = "turret";
const char str_00221c08[] = "non-player";
const char str_00221c14[] = "bullet";
const char str_00221c1c[] = "projectile";
const char str_00222120[] = "\"";
const char str_00222ecc[] = "standard";
const char str_00222ed8[] = "wide 16:10";
const char str_00222ee4[] = "wide 16:9";
const char str_00222ef0[] = "dx9";
const char str_00222ef4[] = "dx7";
const char str_00222ef8[] = "high";
const char str_00222f00[] = "medium";
const char str_00222f08[] = "low";
const char str_00222f0c[] = "lowest";
const char str_00222f14[] = "Screen";
const char str_00222f1c[] = "Feedback";
const char str_00222f28[] = "dither (fast)";
const char str_00222f38[] = "supersample (nice)";
const char str_00222f4c[] = "boxes";
const char str_00222f54[] = "axes";
const char str_00222f5c[] = "both";
const char str_00222f64[] = "Unchanged";
const char str_00222f70[] = "Flat";
const char str_00222f78[] = "White";
const char str_00222f80[] = "Black";
const char str_00222f88[] = "beginning";
const char str_00222f94[] = "beforePresent";
const char str_00222fa4[] = "adaptive";
const char str_00222fb0[] = "normal";
const char str_00222fb8[] = "basisTangent";
const char str_00222fc8[] = "basisBinormal";
const char str_00222fd8[] = "basisNormal";
const char str_00222fe4[] = "nearest";
const char str_00222fec[] = "bilinear";
const char str_00222ff8[] = "trilinear";
const char str_00223004[] = "anisotropic";
const char str_00224568[] = "$raw";
const char str_00225108[] = "current";
const char str_00225110[] = " min_pc";
const char str_00225118[] = "misc  ";
const char str_00225120[] = "debug ";
const char str_00225128[] = "$tex+?";
const char str_00225130[] = "ui    ";
const char str_00225138[] = "lmap  ";
const char str_00225140[] = "light ";
const char str_00225148[] = "f/x   ";
const char str_00225150[] = "hud   ";
const char str_00225158[] = "model ";
const char str_00225160[] = "world ";
const char str_00225168[] = "$dynamic_shadows";
const char str_0022517c[] = "$shadow_cookie";
const char str_0022518c[] = "$shadow_cookie_blur";
const char str_002251a0[] = "$blurred_screen";
const char str_002251b0[] = "$glow_0";
const char str_002251b8[] = "$glow_1";
const char str_002251c0[] = "$pingpong_0";
const char str_002251cc[] = "$pingpong_1";
const char str_002251d8[] = "$resolved_post_sun";
const char str_002251ec[] = "$resolved_scene";
const char str_002251fc[] = "$savedscreen";
const char str_00226400[] = "r_sunsprite_shader";
const char str_00226414[] = "r_sunsprite_size";
const char str_00226428[] = "r_sunflare_shader";
const char str_0022643c[] = "r_sunflare_min_size";
const char str_00226450[] = "r_sunflare_min_angle";
const char str_00226468[] = "r_sunflare_max_size";
const char str_0022647c[] = "r_sunflare_max_angle";
const char str_00226494[] = "r_sunflare_max_alpha";
const char str_002264ac[] = "r_sunflare_fadein";
const char str_002264c0[] = "r_sunflare_fadeout";
const char str_002264d4[] = "r_sunblind_min_angle";
const char str_002264ec[] = "r_sunblind_max_angle";
const char str_00226504[] = "r_sunblind_max_darken";
const char str_0022651c[] = "r_sunblind_fadein";
const char str_00226530[] = "r_sunblind_fadeout";
const char str_00226544[] = "r_sunglare_min_angle";
const char str_0022655c[] = "r_sunglare_max_angle";
const char str_00226574[] = "r_sunglare_max_lighten";
const char str_0022658c[] = "r_sunglare_fadein";
const char str_002265a0[] = "r_sunglare_fadeout";
const char str_002265b4[] = "r_sun_fx_position";
const char str_00228dbc[] = "mac.sound.high.quality";
const char str_00262598[] = "!!ARBvp1.0\nOUTPUT oPos = result.position;\nOUTPUT oD0 = result.color;\nOUTPUT oD1 = result.color.secondary;\nOUTPUT oFog = result.fogcoord;\nOUTPUT oT0 = result.texcoord[0];\nOUTPUT oT1 = result.texcoord[1];\nOUTPUT oT2 = result.texcoord[2];\nOUTPUT oT3 = result.texcoord[3];\nOUTPUT oT4 = result.texcoord[4];\nTEMP r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15;\nADDRESS a0;\nMOV oFog.x, 1;\nPARAM c11 = program.env[11];\nPARAM c23[4] =  { program.env[23..26] };\nPARAM c4[3] =  { program.env[4..6] };\nPARAM c8[3] =  { program.env[8..10] };\nPARAM c0 = { 1.44269502, 3.05175781e-005, 0, 0.5 };\nATTRIB v0 = vertex.attrib[0];\nATTRIB v1 = vertex.attrib[3];\nATTRIB v2 = vertex.attrib[5];\nATTRIB v3 = vertex.attrib[8];\nATTRIB v4 = vertex.attrib[9];\nATTRIB v5 = vertex.attrib[10];\nATTRIB v6 = vertex.attrib[11];\nDP4 oPos.x, v0, c23[0];\nDP4 oPos.y, v0, c23[1];\nDP4 oPos.z, v0, c23[2];\nDP4 oPos.w, v0, c23[3];\nDP3 r0.x, v1, c4[0];\nDP3 r0.y, v1, c4[1];\nDP3 r0.z, v1, c4[2];\nDP3 r3.x, v4, c4[0];\nDP3 r3.y, v4, c4[1];\nDP3 r3.z, v4, c4[2];\nDP3 r2.x, v5, c4[0];\nDP3 r2.y, v5, c4[1];\nDP4 r1.x, v0, c8[0];\nDP4 r1.y, v0, c8[1];\nDP4 r1.z, v0, c8[2];\nDP3 r2.z, v5, c4[2];\nDP3 r4.x, r1, r1;\nADD r1.xyz, r3, r2;\nRSQ r0.w, r4.x;\nADD oT1.xyz, r3, r3;\nMUL r0.w, r4.x, r0.w;\nADD oT2.xyz, r2, r2;\nMUL r0.w, r0.w, c11.z;\nADD oT3.xyz, r0, -r1;\nMUL r0.w, r0.w, c0.x;\nEX2 oFog, r0.w;\nMOV oD0, v2;\nMOV oT0.xy, v3;\nMAD oT4.xyz, v6, c0.y, c0.zzww;\nEND";
const char str_002a67f8[] = "'";
const char str_002a6800[] = "_";
const char str_002a74bc[] = "map_restart";
const char str_002a7520[] = "3D";
const char str_002a7524[] = "Stream";
const char str_002a752c[] = "2D";
const char str_002a7530[] = "Off";
const char str_002a7534[] = "Simple";
const char str_002a753c[] = "Verbose";
const char str_002a7544[] = "Time";
const char str_002a85bc[] = "cg_shock_screenBlendTime";
const char str_002a85d8[] = "cg_shock_screenBlendFadeTime";
const char str_002a85f8[] = "cg_shock_viewKickPeriod";
const char str_002a8610[] = "cg_shock_viewKickRadius";
const char str_cg_shock_viewKickFadeTime[] = "cg_shock_viewKickFadeTime";
const char str_002a8628[] = "cg_shock_sound";
const char str_002a8638[] = "cg_shock_soundFadeInTime";
const char str_002a8654[] = "cg_shock_soundFadeOutTime";
const char str_002a8670[] = "cg_shock_soundLoopFadeTime";
const char str_002a868c[] = "cg_shock_soundLoopEndDelay";
const char str_002a86a8[] = "cg_shock_soundRoomType";
const char str_002a86c0[] = "cg_shock_soundDryLevel";
const char str_002a86d8[] = "cg_shock_soundWetLevel";
const char str_002a86f0[] = "cg_shock_soundModEndDelay";
const char str_002a870c[] = "cg_shock_volume_auto";
const char str_002a8724[] = "cg_shock_volume_auto2d";
const char str_002a873c[] = "cg_shock_volume_menu";
const char str_002a8754[] = "cg_shock_volume_weapon";
const char str_002a876c[] = "cg_shock_volume_voice";
const char str_002a8784[] = "cg_shock_volume_item";
const char str_002a879c[] = "cg_shock_volume_body";
const char str_002a87b4[] = "cg_shock_volume_local";
const char str_002a87cc[] = "cg_shock_volume_music";
const char str_002a87e4[] = "cg_shock_volume_announcer";
const char str_002a8800[] = "cg_shock_volume_shellshock";
const char str_002a881c[] = "cg_shock_mouse";
const char str_002a882c[] = "cg_shock_mouse_maxpitchspeed";
const char str_002a884c[] = "cg_shock_mouse_maxyawspeed";
const char str_002a8868[] = "cg_shock_mouse_sensitivityscale";
const char str_002a8888[] = "cg_shock_mouse_fadeTime";
const char str_002a9590[] = "print";
const char str_002ab028[] = "client1";
const char str_002ab030[] = "server";
const char str_002ac790[] = "kick";
const char str_002ac808[] = "map";
const char str_002acd00[] = "_up";
const char str_002acd04[] = "_dn";
const char str_002acd08[] = "_lf";
const char str_002acd0c[] = "_rt";
const char str_002acd10[] = "_ft";
const char str_002acd14[] = "_bk";
const char str_002aee58[] = "mp/playeranim.script";
const char str_002b071c[] = "println";
const char str_002b0724[] = "iprintln";
const char str_002b0730[] = "iprintlnbold";
const char str_002b0740[] = "print3d";
const char str_002b0748[] = "getent";
const char str_002b0750[] = "getentarray";
const char str_002b075c[] = "spawn";
const char str_002b0764[] = "spawnturret";
const char str_002b0770[] = "precacheturret";
const char str_002b0780[] = "spawnstruct";
const char str_002b078c[] = "assert";
const char str_002b0794[] = "assertex";
const char str_002b07a0[] = "assertmsg";
const char str_002b07ac[] = "isdefined";
const char str_002b07b8[] = "isstring";
const char str_002b07c4[] = "isalive";
const char str_002b07cc[] = "getcvar";
const char str_002b07d4[] = "getcvarint";
const char str_002b07e0[] = "getcvarfloat";
const char str_002b07f0[] = "setcvar";
const char str_002b07f8[] = "gettime";
const char str_002b0800[] = "getentbynum";
const char str_002b080c[] = "getweaponmodel";
const char str_002b081c[] = "getanimlength";
const char str_002b082c[] = "animhasnotetrack";
const char str_002b0840[] = "getbrushmodelcenter";
const char str_002b0854[] = "objective_add";
const char str_002b0864[] = "objective_delete";
const char str_002b0878[] = "objective_state";
const char str_002b0888[] = "objective_icon";
const char str_002b0898[] = "objective_position";
const char str_002b08ac[] = "objective_onentity";
const char str_002b08c0[] = "objective_current";
const char str_002b08d4[] = "bullettrace";
const char str_002b08e0[] = "bullettracepassed";
const char str_002b08f4[] = "sighttracepassed";
const char str_002b0908[] = "physicstrace";
const char str_002b0918[] = "getmovedelta";
const char str_002b0928[] = "getangledelta";
const char str_002b0938[] = "getnorthyaw";
const char str_002b0944[] = "randomint";
const char str_002b0950[] = "randomfloat";
const char str_002b095c[] = "randomintrange";
const char str_002b096c[] = "randomfloatrange";
const char str_002b0980[] = "sin";
const char str_002b0984[] = "cos";
const char str_002b0988[] = "tan";
const char str_002b098c[] = "asin";
const char str_002b0994[] = "acos";
const char str_002b099c[] = "atan";
const char str_002b09a4[] = "distance";
const char str_002b09b0[] = "distancesquared";
const char str_002b09c0[] = "lengthsquared";
const char str_002b09d0[] = "closer";
const char str_002b09d8[] = "vectordot";
const char str_002b09e4[] = "vectornormalize";
const char str_002b09f4[] = "vectortoangles";
const char str_002b0a04[] = "anglestoup";
const char str_002b0a10[] = "anglestoright";
const char str_002b0a20[] = "anglestoforward";
const char str_002b0a30[] = "issubstr";
const char str_002b0a3c[] = "getsubstr";
const char str_002b0a48[] = "tolower";
const char str_002b0a50[] = "strtok";
const char str_002b0a58[] = "musicplay";
const char str_002b0a64[] = "musicstop";
const char str_002b0a70[] = "soundfade";
const char str_002b0a7c[] = "ambientplay";
const char str_002b0a88[] = "ambientstop";
const char str_002b0a94[] = "precachemodel";
const char str_002b0aa4[] = "precacheshellshock";
const char str_002b0ab8[] = "precacheitem";
const char str_002b0ac8[] = "precacheshader";
const char str_002b0ad8[] = "precachestring";
const char str_002b0ae8[] = "precacherumble";
const char str_002b0af8[] = "loadfx";
const char str_002b0b00[] = "playfxontag";
const char str_002b0b0c[] = "playloopedfx";
const char str_002b0b1c[] = "setcullfog";
const char str_002b0b28[] = "setexpfog";
const char str_002b0b34[] = "grenadeexplosioneffect";
const char str_002b0b4c[] = "radiusdamage";
const char str_002b0b5c[] = "setplayerignoreradiusdamage";
const char str_002b0b78[] = "getnumparts";
const char str_002b0b84[] = "getpartname";
const char str_002b0b90[] = "earthquake";
const char str_002b0b9c[] = "newhudelem";
const char str_002b0ba8[] = "newclienthudelem";
const char str_002b0bbc[] = "newteamhudelem";
const char str_002b0bcc[] = "resettimeout";
const char str_002b0bdc[] = "isplayer";
const char str_002b0be8[] = "isplayernumber";
const char str_002b0bf8[] = "setwinningplayer";
const char str_002b0c0c[] = "setwinningteam";
const char str_002b0c1c[] = "announcement";
const char str_002b0c2c[] = "clientannouncement";
const char str_002b0c40[] = "getteamscore";
const char str_002b0c50[] = "setteamscore";
const char str_002b0c60[] = "setclientnamemode";
const char str_002b0c74[] = "updateclientnames";
const char str_002b0c88[] = "getteamplayersalive";
const char str_002b0c9c[] = "objective_team";
const char str_002b0cac[] = "logprint";
const char str_002b0cb8[] = "worldentnumber";
const char str_002b0cc8[] = "obituary";
const char str_002b0cd4[] = "positionwouldtelefrag";
const char str_002b0cec[] = "getstarttime";
const char str_002b0cfc[] = "precachemenu";
const char str_002b0d0c[] = "precachestatusicon";
const char str_002b0d20[] = "precacheheadicon";
const char str_002b0d34[] = "exitlevel";
const char str_002b0d40[] = "addtestclient";
const char str_002b0d50[] = "makecvarserverinfo";
const char str_002b0d64[] = "setarchive";
const char str_002b0d70[] = "allclientsprint";
const char str_002b0d80[] = "clientprint";
const char str_002b0d8c[] = "mapexists";
const char str_002b0d98[] = "isvalidgametype";
const char str_002b0da8[] = "matchend";
const char str_002b0db4[] = "setplayerteamrank";
const char str_002b0dc8[] = "sendranks";
const char str_002b0dd4[] = "setvotestring";
const char str_002b0de4[] = "setvotetime";
const char str_002b0df0[] = "setvoteyescount";
const char str_002b0e00[] = "setvotenocount";
const char str_002b0e10[] = "openfile";
const char str_002b0e1c[] = "closefile";
const char str_002b0e28[] = "fprintln";
const char str_002b0e34[] = "freadln";
const char str_002b0e3c[] = "fgetarg";
const char str_002b0e44[] = "ban";
const char str_002b0e48[] = "playrumbleonpos";
const char str_002b0e58[] = "playlooprumbleonpos";
const char str_002b0e6c[] = "stopallrumbles";
const char str_002b0e7c[] = "soundexists";
const char str_002b0e88[] = "issplitscreen";
const char str_002b0e98[] = "attach";
const char str_002b0ea0[] = "detach";
const char str_002b0ea8[] = "detachall";
const char str_002b0eb4[] = "getattachsize";
const char str_002b0ec4[] = "getattachmodelname";
const char str_002b0ed8[] = "getattachtagname";
const char str_002b0eec[] = "getattachignorecollision";
const char str_002b0f08[] = "getammocount";
const char str_002b0f18[] = "getclanid";
const char str_002b0f24[] = "getclanname";
const char str_002b0f30[] = "getclandescription";
const char str_002b0f44[] = "getclanmotto";
const char str_002b0f54[] = "getclanurl";
const char str_002b0f60[] = "linkto";
const char str_002b0f68[] = "unlink";
const char str_002b0f70[] = "enablelinkto";
const char str_002b0f80[] = "getorigin";
const char str_002b0f8c[] = "geteye";
const char str_002b0f94[] = "useby";
const char str_002b0f9c[] = "setstablemissile";
const char str_002b0fb0[] = "istouching";
const char str_002b0fbc[] = "playsound";
const char str_002b0fc8[] = "playsoundasmaster";
const char str_002b0fdc[] = "playloopsound";
const char str_002b0fec[] = "stoploopsound";
const char str_002b0ffc[] = "playrumble";
const char str_002b1008[] = "playlooprumble";
const char str_002b1018[] = "stoprumble";
const char str_002b1024[] = "delete";
const char str_002b102c[] = "setmodel";
const char str_002b1038[] = "getnormalhealth";
const char str_002b1048[] = "setnormalhealth";
const char str_002b1058[] = "showtoplayer";
const char str_002b1068[] = "setcontents";
const char str_002b1074[] = "setcursorhint";
const char str_002b1084[] = "sethintstring";
const char str_002b1094[] = "stopshellshock";
const char str_002b10a4[] = "viewkick";
const char str_002b10b0[] = "localtoworldcoords";
const char str_002b10c4[] = "setrightarc";
const char str_002b10d0[] = "setleftarc";
const char str_002b10dc[] = "settoparc";
const char str_002b10e8[] = "setbottomarc";
const char str_002b10f8[] = "getentitynumber";
const char str_002b1108[] = "enablegrenadetouchdamage";
const char str_002b1124[] = "disablegrenadetouchdamage";
const char str_002b1140[] = "enablegrenadebounce";
const char str_002b1154[] = "disablegrenadebounce";
const char str_002b116c[] = "enableaimassist";
const char str_002b117c[] = "disableaimassist";
const char str_002b1190[] = "placespawnpoint";
const char str_002b11a0[] = "updatescores";
const char str_002b11b0[] = "setteamfortrigger";
const char str_002b11c4[] = "clientclaimtrigger";
const char str_002b11d8[] = "clientreleasetrigger";
const char str_002b11f0[] = "releaseclaimedtrigger";
const char str_002b3564[] = "GAME_GC_HOLDYOURPOSITION";
const char str_002b3580[] = "GAME_GC_HOLDTHISPOSITION";
const char str_002b359c[] = "GAME_GC_COMEHERE";
const char str_002b35b0[] = "GAME_GC_COVERME";
const char str_002b35c0[] = "GAME_GC_GUARDLOCATION";
const char str_002b35d8[] = "GAME_GC_SEARCHDESTROY";
const char str_002b35f0[] = "GAME_GC_REPORT";
const char str_002b4d3c[] = "bigfixed";
const char str_002b4d48[] = "smallfixed";
const char str_002b51bc[] = "WEAPON_FRAGGRENADE";
const char str_002b51d0[] = "WEAPON_SMOKEGRENADE";
const char str_002b576c[] = "HINT_NONE";
const char str_002b5778[] = "HINT_ACTIVATE";
const char str_002b5788[] = "HINT_HEALTH";
const char str_002b5794[] = "HINT_FRIENDLY";
const char str_002b65d0[] = "helmet";
const char str_002b65d8[] = "head";
const char str_002b65e0[] = "neck";
const char str_002b65e8[] = "torso_upper";
const char str_002b65f4[] = "torso_lower";
const char str_002b6600[] = "right_arm_upper";
const char str_002b6610[] = "left_arm_upper";
const char str_002b6620[] = "right_arm_lower";
const char str_002b6630[] = "left_arm_lower";
const char str_002b6640[] = "right_hand";
const char str_002b664c[] = "left_hand";
const char str_002b6658[] = "right_leg_upper";
const char str_002b6668[] = "left_leg_upper";
const char str_002b6678[] = "right_leg_lower";
const char str_002b6688[] = "left_leg_lower";
const char str_002b6698[] = "right_foot";
const char str_002b66a4[] = "left_foot";
const char str_002b66b0[] = "gun";
const char str_002b75cc[] = "svc_nop";
const char str_002b75d4[] = "svc_gamestate";
const char str_002b75e4[] = "svc_configstring";
const char str_002b75f8[] = "svc_baseline";
const char str_002b7608[] = "svc_serverCommand";
const char str_002b761c[] = "svc_download";
const char str_002b762c[] = "svc_snapshot";
const char str_002b763c[] = "svc_EOF";
const char str_002b8750[] = "aivsai";
const char str_002b8758[] = "aivsplayer";
