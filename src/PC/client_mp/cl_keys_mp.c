#include "common_types.h"
#include "imports.h"
#include <ctype.h>
#include <string.h>

extern keyname_t keynames[99];
extern keyname_t keynames_localized[100];

extern clientStatic_t cls;
extern refexport_t re;

extern void ReplaceStringInternal(char **dest, const char *src);
extern void Cmd_AddCommand(const char *name, void (*func)(void));
extern int Cmd_Argc(void);
extern char *Cmd_Argv(int arg);
extern void I_strncat(char *dest, int maxlen, const char *src);
extern int I_stricmp(const char *s0, const char *s1);
extern Bool I_isdigit(int c);
extern int I_strnicmp(const char *s0, const char *s1, size_t n);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern void Com_Printf(const char *fmt, ...);
extern void FS_Printf(fileHandle_t f, const char *fmt, ...);
extern int SEH_GetCurrentLanguage(void);
extern void CL_SwitchToLocalClient(int localClientNum);
extern void Z_FreeInternal(void *ptr);
extern void UI_KeyEvent(int key, int down);
extern void Field_Clear(field_t *edit);
extern char *Sys_GetClipboardData(void);
extern void Cbuf_AddText(const char *text);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern void CL_AddReliableCommand(const char *cmd);
extern void Con_ToggleConsole_f(void);
extern void Con_ToggleConsoleOutput(void);
extern Bool Con_IsActive(void);
extern void Con_PageUp(void);
extern void Con_PageDown(void);
extern void Con_Top(void);
extern void Con_Bottom(void);
extern const char *Con_TokenizeInput(void);
extern Bool Con_AnySpaceAfterCommand(void);
extern qboolean UI_SetActiveMenu(int menu);
extern qboolean UI_checkKeyExec(int key);
extern int CG_KeyInterceptEvent(int key, qboolean down);
extern void CL_StopLogoOrCinematic(void);
extern void CL_Disconnect(void);
extern void Dvar_SetString(const dvar_t *dvar, const char *value);
extern int I_strncmp(const char *s0, const char *s1, int n);
extern void Cmd_ForEach(void (*callback)(const char *));
extern void Dvar_ForEach(void (*callback)(const char *));
extern void SCR_UpdateScreen(void);
extern Bool CL_AllLocalClientsDisconnected(void);

extern FontHandle UI_GetFontHandle(int fontEnum, float scale);
extern void CL_DrawTextWithCursor(const char *text, int maxChars, void *font, float x, float y, int horzAlign, int vertAlign, float xScale, float yScale, const vec_t *color, int style, int cursorPos, int cursor);
extern float GetRealWidthFromVirtualWidth(float w);
extern float GetRealHeightFromVirtualHeight(float h);
extern float GetVirtualWidthFromRealWidth(float w);
extern float GetVirtualHeightFromRealHeight(float h);
extern int SEH_PrintStrlen(const char *str);

extern PlayerKeyState playerKeys[1];
extern field_t *chatField;
extern qboolean *chat_team;
extern qboolean *key_overstrikeMode;
extern qboolean *anykeydown;
extern qkey_t *keys;
extern field_t historyEditLines[32];
extern field_t g_consoleField;
extern int nextHistoryLine;
extern int historyLine;
/* migrated from the ILP32 data blob to typed C (x64 port Stage 2). */
char *frenchNumberKeysMap[10] = {
    "\xff" "fd",
    "&",
    "\xff" "fd",
    "\"",
    "'",
    "(",
    "-",
    "\xff" "fd",
    "_",
    "\xff" "fd",
};

static char tinystr[5];
static const char *completionString;
static Bool hasExactMatch;
static char shortestMatch[1024];
static int matchCount;

enum {
    K_ENTER = 0x0d,
    K_TAB = 0x09,
    K_ESCAPE = 0x1b,
    K_UPARROW = 0x9a,
    K_DOWNARROW = 0x9b,
    K_RIGHTARROW = 0x9d,
    K_LEFTARROW = 0x9c,
    K_CTRL = 0x9f,
    K_SHIFT = 0xa0,
    K_INS = 0xa1,
    K_DEL = 0xa2,
    K_PGDN = 0xa3,
    K_PGUP = 0xa4,
    K_HOME = 0xa5,
    K_END = 0xa6,
    K_KP_ENTER = 0xbf,
    K_KP_INS = 0xc0,
    K_MWHEELDOWN = 0xcd,
    K_MWHEELUP = 0xce
};

qboolean Key_GetOverstrikeMode(void);
void Field_AdjustScroll(field_t *edit);
void Key_SetOverstrikeMode(qboolean state);
static void FindMatches(const char *s);
static void PrintMatches(const char *s);
static void keyConcatArgs(void);
static __attribute_regparm__(1) int Key_StringToKeynum(const char *str);
char *Key_KeynumToString(int keynum, qboolean translate);
void Key_SetBinding(int keynum, const char *binding);
char *Key_GetBinding(int keynum);
void CL_InitKeyCommands(void);
void Key_Shutdown(void);
void Field_Draw(field_t *edit, int x, int y, int horzAlign, int vertAlign, qboolean showCursor);
void Key_Unbind_f(void);
void Key_Unbindall_f(void);
void Key_Bind_f(void);
void Field_CharEvent(field_t *edit, int ch);
void CL_CharEvent(int key);
void Field_KeyDownEvent(field_t *edit, int key);
void Message_Key(int key);
void Console_Key(int key);
void Key_WriteBindings(fileHandle_t f);
void Key_Bindlist_f(void);
void CL_KeyEvent(int key, const qboolean down, const unsigned int time);
void Key_ClearStates(void);

#if COD2_FEATURE_GAMEPAD
extern void CL_Gamepad_NoteKbmActive(void);
#endif

qboolean Key_GetOverstrikeMode(void)
{
    return *key_overstrikeMode;
}

void Field_AdjustScroll(field_t *edit)
{
    typedef float (*GetFontHeightFn)(FontHandle font, float scale);
    typedef int (*TextWidthFn)(const char *text, int limit, FontHandle font);
    float fontScale, lineWidth, actualScale;
    FontHandle font;
    const char *bufStart;
    int scroll;

    fontScale = edit->charHeight / 48.0f;
    lineWidth = (float)edit->widthInPixels;

    if (edit->fixedSize) {
        fontScale = GetVirtualWidthFromRealWidth(fontScale);
        lineWidth = GetVirtualHeightFromRealHeight(lineWidth);
        font = cls.consoleFont;
    } else {
        font = UI_GetFontHandle(0, fontScale);
    }

    actualScale = re.NormalizedTextScale(font, fontScale);
    bufStart = edit->buffer;

    {
        int totalWidth = re.TextWidth(bufStart, 0, font);
        float totalWidthScaled = (float)totalWidth * actualScale;
        if (totalWidthScaled < lineWidth) {
            edit->scroll = 0;
            edit->drawWidth = SEH_PrintStrlen(bufStart);
            return;
        }
    }

    if (lineWidth <= 0.0f) {
        scroll = edit->scroll;
        goto compute_visible;
    }

    scroll = edit->scroll;
    while (scroll > 0) {
        int endWidth = re.TextWidth(bufStart + scroll - 1, 0, font);
        float endWidthScaled = (float)endWidth * actualScale;
        if (endWidthScaled >= lineWidth)
            break;
        scroll--;
        edit->scroll = scroll;
    }

compute_visible: {
    int textLen, textLenFromScroll;
    float scrolledWidth, cursorWidth, diff;

    textLen = re.TextWidth(bufStart + scroll, 0, font);
    textLenFromScroll = re.TextWidth(bufStart + edit->cursor, 0, font);

    scrolledWidth = (float)textLen * actualScale;
    cursorWidth = (float)textLenFromScroll * actualScale;
    diff = scrolledWidth - cursorWidth;

    if (diff < 0.0f) {
        if (edit->scroll > 0) {
            edit->scroll--;
            scroll = edit->scroll;
            goto compute_visible;
        }
        if (cursorWidth < lineWidth)
            goto compute_drawWidth;
    } else if (diff >= lineWidth) {
        edit->scroll++;
        scroll = edit->scroll;
        goto compute_visible;
    }

compute_drawWidth: {
    int len = (int)strlen(bufStart + scroll);
    int visChars = edit->cursor - scroll;
    edit->drawWidth = visChars;

    if (lineWidth > 0.0f && visChars < len) {
        while (visChars < len) {
            int w = re.TextWidth(bufStart + scroll, visChars + 1, font);
            float wScaled = (float)w * actualScale;
            if (wScaled >= lineWidth)
                break;
            visChars++;
            edit->drawWidth = visChars;
        }
    }
}
}
}

void Key_SetOverstrikeMode(qboolean state)
{
    *key_overstrikeMode = state;
}

void FindMatches(const char *s)
{
    int i;

    if (I_strnicmp(s, completionString, (int)strlen(completionString)) != 0) {
        return;
    }

    if (++matchCount == 1) {
        I_strncpyz(shortestMatch, s, sizeof(shortestMatch));
        hasExactMatch = 1;
        return;
    }

    if (!*s) {
        hasExactMatch = 1;
        shortestMatch[0] = '\0';
        return;
    }

    for (i = 0;; ++i) {
        if (tolower(shortestMatch[i]) != tolower(s[i])) {
            break;
        }
        if (!s[i]) {
            hasExactMatch = 1;
            shortestMatch[i] = '\0';
            return;
        }
    }

    if (!s[i] || (hasExactMatch && !shortestMatch[i])) {
        hasExactMatch = 1;
    } else {
        hasExactMatch = 0;
    }
    shortestMatch[i] = '\0';
}

static void PrintMatches(const char *s)
{
    if (I_strnicmp(s, shortestMatch, (int)strlen(shortestMatch)) == 0) {
        Com_Printf("    %s\n", s);
    }
}

static void keyConcatArgs(void)
{
    int i;

    for (i = 1; i < Cmd_Argc(); ++i) {
        const char *arg;
        const char *scan;

        I_strncat(g_consoleField.buffer, 0x100, " ");
        arg = Cmd_Argv(i);
        scan = arg;
        while (*scan && *scan != ' ') {
            ++scan;
        }

        if (*scan == ' ') {
            I_strncat(g_consoleField.buffer, 0x100, "\"");
        }

        I_strncat(g_consoleField.buffer, 0x100, arg);

        if (*scan == ' ') {
            I_strncat(g_consoleField.buffer, 0x100, "\"");
        }
    }
}

static inline __attribute__((always_inline)) int Key_HexCharValue(char ch)
{
    if (I_isdigit(ch)) {
        return ch - '0';
    }

    if ((unsigned int)(ch - 'a') <= 5) {
        return ch - 'a' + 10;
    }

    return 0;
}

static __attribute_regparm__(1) int Key_StringToKeynum(const char *str)
{
    keyname_t *name;

    if (str == NULL || *str == '\0') {
        return -1;
    }

    if (str[1] == '\0') {
        return (signed char)str[0];
    }

    if (str[0] == '0' && str[1] == 'x' && strlen(str) == 4) {
        return (Key_HexCharValue(str[2]) << 4) + Key_HexCharValue(str[3]);
    }

    for (name = keynames; name->name; ++name) {
        if (I_stricmp(str, name->name) == 0) {
            return name->keynum;
        }
    }

    return -1;
}

static inline __attribute__((always_inline)) char *Key_KeynumToStringInternal(int keynum, qboolean translate)
{
    keyname_t *name;
    int upperNibble;
    int lowerNibble;

    if (keynum == -1) {
        return "<KEY NOT FOUND>";
    }

    if ((unsigned int)keynum > 0xff) {
        return "<OUT OF RANGE>";
    }

#if COD2_FEATURE_GAMEPAD
    if (translate && keynum >= 0xcf && keynum <= 0xde) {
        static char *labels[] = {"A / Cross", "B / Circle", "X / Square", "Y / Triangle",
            "LB / L1", "RB / R1", "Back / Share", "Start / Options", "LS / L3", "RS / R3",
            "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right", "LT / L2", "RT / R2"};
        return labels[keynum - 0xcf];
    }
#endif

    if (translate && SEH_GetCurrentLanguage() == 1 && keynum >= '0' && keynum <= '9') {
        return frenchNumberKeysMap[keynum - '0'];
    }

    if ((unsigned int)(keynum - 0x21) <= 0x5d && keynum != '"') {
        tinystr[0] = toupper(keynum);
        tinystr[1] = '\0';
        if (keynum != ';' || translate) {
            return tinystr;
        }
    }

    name = translate ? keynames_localized : keynames;
    while (name->name) {
        if (name->keynum == keynum) {
            return name->name;
        }
        ++name;
    }

    upperNibble = keynum >> 4;
    lowerNibble = keynum & 0xf;
    tinystr[0] = '0';
    tinystr[1] = 'x';
    tinystr[2] = upperNibble > 9 ? (char)(upperNibble + 'a' - 10) : (char)(upperNibble + '0');
    tinystr[3] = lowerNibble > 9 ? (char)(lowerNibble + 'a' - 10) : (char)(lowerNibble + '0');
    tinystr[4] = '\0';
    return tinystr;
}

char *Key_KeynumToString(int keynum, qboolean translate)
{
    return Key_KeynumToStringInternal(keynum, translate);
}

static inline __attribute__((always_inline)) void Key_SetBindingInternal(int keynum, const char *binding)
{
    if (keynum == -1)
        return;
    ReplaceStringInternal((char **)&keys[keynum].binding, binding);   /* was keynum*12+8 (x86 qkey_t stride) */
    *(int *)imp_dvar_modifiedFlags |= 1;
}

static inline __attribute__((always_inline)) char *Key_GetBindingInternal(int keynum)
{
    if (keynum == -1)
        return (char *)"";
    return keys[keynum].binding;
}

void Key_SetBinding(int keynum, const char *binding)
{
    Key_SetBindingInternal(keynum, binding);
}

char *Key_GetBinding(int keynum)
{
    return Key_GetBindingInternal(keynum);
}

void CL_InitKeyCommands(void)
{
    Cmd_AddCommand("bind", Key_Bind_f);
    Cmd_AddCommand("unbind", Key_Unbind_f);
    Cmd_AddCommand("unbindall", Key_Unbindall_f);
    Cmd_AddCommand("bindlist", Key_Bindlist_f);
}

void Key_Shutdown(void)
{
    int keyIndex;

    CL_SwitchToLocalClient(0);

    for (keyIndex = 0; keyIndex < 256; ++keyIndex) {
        char *binding = keys[keyIndex].binding;
        if (binding != NULL) {
            Z_FreeInternal(binding);
            keys[keyIndex].binding = NULL;
        }
    }
}

void Field_Draw(field_t *edit, int x, int y, int horzAlign, int vertAlign, qboolean showCursor)
{
    vec4_t vColor;
    char str[0x400];
    int cursorPos;
    void *font;
    int fontStyle;
    float xScale;
    float yScale;
    float xAdj;
    float yAdj;
    int cursorChar;
    int drawWidth;
    float rawScale;

    vColor[0] = 1.0f;
    vColor[1] = 1.0f;
    vColor[2] = 1.0f;
    vColor[3] = 1.0f;

    I_strncpyz(str, edit->buffer + edit->scroll, 0x100 - edit->scroll);

    cursorPos = edit->cursor - edit->scroll;

    if (edit->fixedSize != 0) {

        font = (void *)cls.consoleFont;

        if (*key_overstrikeMode) {
            fontStyle = 0;
            xScale = 1.0f;
            yScale = 1.0f;
            cursorChar = 0x5f;
        } else {
            fontStyle = 0;
            xScale = 1.0f;
            yScale = 1.0f;
            cursorChar = 0x7c;
        }
    } else {

        rawScale = edit->charHeight / 48.0f;

        font = (void *)UI_GetFontHandle(0, rawScale);

        {
            typedef float (*GetFontYSizeFn)(void *font, float scale);
            float fontYSize;
            GetFontYSizeFn fn = (GetFontYSizeFn)re.NormalizedTextScale;
            fontYSize = fn(font, rawScale);
            (void)fontYSize;

            if (vertAlign == 5) {

                typedef float (*ConvertFn)(float val);
                ConvertFn getRealW = (ConvertFn)GetRealWidthFromVirtualWidth;
                ConvertFn getRealH = (ConvertFn)GetRealHeightFromVirtualHeight;
                xScale = getRealW(fontYSize);
                yScale = getRealH(fontYSize);
            } else {
                xScale = fontYSize;
                yScale = fontYSize;
            }
        }

        if (*key_overstrikeMode) {
            fontStyle = 3;
            cursorChar = 0x5f;
        } else {
            fontStyle = 3;
            cursorChar = 0x7c;
        }
    }

    xAdj = (float)x;

    {
        typedef int (*GetFontHeightFn)(void *font);
        GetFontHeightFn fn = (GetFontHeightFn)re.TextHeight;
        int fontHeight = fn(font);
        yAdj = (float)y + (float)fontHeight * yScale;
    }

    drawWidth = edit->drawWidth;
    if (drawWidth == 0)
        edit->drawWidth = 0x100;

    CL_DrawTextWithCursor(str, edit->drawWidth, font,
                          xAdj, yAdj, horzAlign, vertAlign,
                          xScale, yScale,
                          vColor, fontStyle, cursorPos, cursorChar);
}

void Key_Unbind_f(void)
{
    int keynum;

    if (Cmd_Argc() != 2) {
        Com_Printf("unbind <key> : remove commands from a key\n");
        return;
    }

    keynum = Key_StringToKeynum(Cmd_Argv(1));
    if (keynum == -1) {
        Com_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
        return;
    }

    Key_SetBindingInternal(keynum, "");
}

void Key_Unbindall_f(void)
{
    int keynum;
    int *modifiedFlags = (int *)imp_dvar_modifiedFlags;

    for (keynum = 0; keynum < 256; ++keynum) {
        if (keys[keynum].binding != NULL && keynum != -1) {
            ReplaceStringInternal((char **)&keys[keynum].binding, "");   /* was keynum*12+8 (x86 qkey_t stride) */
            *modifiedFlags |= 1;
        }
    }
}

void Key_Bind_f(void)
{
    int argc;
    int keynum;
    int argIndex;
    char cmd[1024];
    const char *binding;

    argc = Cmd_Argc();
    if (argc <= 1) {
        Com_Printf("bind <key> [command] : attach a command to a key\n");
        return;
    }

    keynum = Key_StringToKeynum(Cmd_Argv(1));
    if (keynum == -1) {
        Com_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
        return;
    }

    keynum = tolower(keynum);
    if (argc == 2) {
        binding = Key_GetBindingInternal(keynum);
        if (binding != NULL) {
            Com_Printf("\"%s\" = \"%s\"\n", Cmd_Argv(1), binding);
        } else {
            Com_Printf("\"%s\" is not bound\n", Cmd_Argv(1));
        }
        return;
    }

    cmd[0] = '\0';
    for (argIndex = 2; argIndex < argc; ++argIndex) {
        qboolean hasSpace;

        if (argc != 3) {
            if (strchr(Cmd_Argv(argIndex), ' ') != NULL) {
                I_strncat(cmd, sizeof(cmd), "\"");
                hasSpace = 1;
            } else {
                hasSpace = 0;
            }
        } else {
            hasSpace = 0;
        }

        I_strncat(cmd, sizeof(cmd), Cmd_Argv(argIndex));

        if (hasSpace) {
            I_strncat(cmd, sizeof(cmd), "\"");
        }

        if (argIndex != argc - 1) {
            I_strncat(cmd, sizeof(cmd), " ");
        }
    }

    Key_SetBindingInternal(keynum, cmd);
}

static inline __attribute__((always_inline)) void Field_PasteClipboard(field_t *edit)
{
    char *cbd;
    int len;
    int i;

    cbd = Sys_GetClipboardData();
    if (!cbd)
        return;

    len = (int)strlen(cbd);
    for (i = 0; i < len; ++i) {
        Field_CharEvent(edit, cbd[i]);
    }

    Z_FreeInternal(cbd);
}

void Field_CharEvent(field_t *edit, int ch)
{
    int len = (int)strlen(edit->buffer);

    if (ch == 0x16) {
        Field_PasteClipboard(edit);
        Field_AdjustScroll(edit);
        return;
    }

    if (ch == 3) {
        Field_Clear(edit);
        Field_AdjustScroll(edit);
        return;
    }

    if (ch == 8) {
        if (edit->cursor > 0) {
            memmove(edit->buffer + edit->cursor - 1,
                    edit->buffer + edit->cursor,
                    (size_t)(len - edit->cursor + 1));
            edit->cursor--;
        }
        Field_AdjustScroll(edit);
        return;
    }

    if (ch == 1) {
        edit->cursor = 0;
        edit->scroll = 0;
        return;
    }

    if (ch == 5) {
        edit->cursor = len;
        Field_AdjustScroll(edit);
        return;
    }

    if (ch <= 31)
        return;

    if (*key_overstrikeMode) {
        if (edit->cursor == 255)
            return;
        edit->buffer[edit->cursor++] = (char)ch;
        if (edit->cursor == len + 1)
            edit->buffer[edit->cursor] = 0;
    } else {
        if (len == 255)
            return;
        memmove(edit->buffer + edit->cursor + 1,
                edit->buffer + edit->cursor,
                (size_t)(len - edit->cursor + 1));
        edit->buffer[edit->cursor++] = (char)ch;
        if (edit->cursor == len + 1)
            edit->buffer[edit->cursor] = 0;
    }

    Field_AdjustScroll(edit);
}

void CL_CharEvent(int key)
{
    int keyCatchers;

    if (key == '`' || key == '~')
        return;

    keyCatchers = (*(clientActive_t **)imp_cl)->keyCatchers;

    if (keyCatchers & 1) {
        Field_CharEvent(&g_consoleField, key);
        return;
    }
    if (keyCatchers & 8) {
        UI_KeyEvent(key | 0x400, 1);
        return;
    }
    if (keyCatchers & 0x10) {
        Field_CharEvent((field_t *)chatField, key);
        return;
    }

    if (!**(int **)imp_clc) {
        Field_CharEvent(&g_consoleField, key);
        return;
    }
}

static qboolean Key_IsDown(int key)
{
    return keys && key >= 0 && key < 256 && keys[key].down;
}

static inline __attribute__((always_inline))
qboolean
Field_IsWordChar(int ch)
{
    return isalnum(ch) != 0;
}

static inline __attribute__((always_inline)) void Field_AdjustScrollForKey(field_t *edit)
{
    if (cls.uiStarted)
        Field_AdjustScroll(edit);
}

void Field_KeyDownEvent(field_t *edit, int key)
{
    int len;

    len = (int)strlen(edit->buffer);

    if ((key == K_INS || key == K_KP_INS) && Key_IsDown(K_SHIFT)) {
        Field_PasteClipboard(edit);
        Field_AdjustScrollForKey(edit);
        return;
    }

    if (key == K_DEL) {
        if (edit->cursor < len) {
            memmove(edit->buffer + edit->cursor,
                    edit->buffer + edit->cursor + 1,
                    (size_t)(len - edit->cursor));
        }
        Field_AdjustScrollForKey(edit);
        return;
    }

    if (key == K_RIGHTARROW) {
        if (edit->cursor < len)
            edit->cursor++;

        if (Key_IsDown(K_CTRL)) {
            while (edit->cursor < len && Field_IsWordChar(edit->buffer[edit->cursor]))
                edit->cursor++;
            while (edit->cursor < len && !Field_IsWordChar(edit->buffer[edit->cursor]))
                edit->cursor++;
        }
        Field_AdjustScrollForKey(edit);
        return;
    }

    if (key == K_LEFTARROW) {
        if (edit->cursor > 0)
            edit->cursor--;

        if (Key_IsDown(K_CTRL)) {
            while (edit->cursor > 0 && Field_IsWordChar(edit->buffer[edit->cursor - 1]))
                edit->cursor--;
            while (edit->cursor > 0 && !Field_IsWordChar(edit->buffer[edit->cursor - 1]))
                edit->cursor--;
        }

        if (edit->cursor < edit->scroll)
            edit->scroll = edit->cursor;

        Field_AdjustScrollForKey(edit);
        return;
    }

    if (key == K_HOME || (tolower(key) == 0x61 && Key_IsDown(K_CTRL))) {
        edit->cursor = 0;
        Field_AdjustScrollForKey(edit);
        return;
    }

    if (key == K_END || (tolower(key) == 0x65 && Key_IsDown(K_CTRL))) {
        edit->cursor = len;
        Field_AdjustScrollForKey(edit);
        return;
    }

    if (key == K_INS) {
        Key_SetOverstrikeMode(!Key_GetOverstrikeMode());
        Field_AdjustScrollForKey(edit);
        return;
    }

    Field_AdjustScrollForKey(edit);
}

static inline __attribute__((always_inline)) void Message_Close(void)
{
    (*(clientActive_t **)imp_cl)->keyCatchers &= ~0x10;
    Field_Clear(chatField);
}

void Message_Key(int key)
{
    if (key == K_ESCAPE) {
        Message_Close();
        return;
    }

    if (key == K_ENTER || key == K_KP_ENTER) {
        if (chatField->buffer[0]) {
            if ((*(clientConnection_t **)imp_clc)->state == 8) {
                char buffer[1024];

                if (*chat_team) {
                    Com_sprintf(buffer, sizeof(buffer),
                                "say_team \"\x15%s\"\n", chatField->buffer);
                } else {
                    Com_sprintf(buffer, sizeof(buffer),
                                "say \"\x15%s\"\n", chatField->buffer);
                }
                CL_AddReliableCommand(buffer);
            }
        }
        Message_Close();
        return;
    }

    Field_KeyDownEvent(chatField, key);
}

static int Console_HistoryIndex(int line)
{
    int index = line % 32;
    return index < 0 ? index + 32 : index;
}

static void Console_UpdateFieldCursor(void)
{
    g_consoleField.cursor = (int)strlen(g_consoleField.buffer);
    if (g_consoleField.cursor > 255)
        g_consoleField.cursor = 255;
    Field_AdjustScroll(&g_consoleField);
}

static void Console_SaveHistory(void)
{
    if (!g_consoleField.buffer[0])
        return;

    memcpy(&historyEditLines[Console_HistoryIndex(nextHistoryLine)],
           &g_consoleField,
           sizeof(g_consoleField));
    nextHistoryLine++;
    historyLine = nextHistoryLine;
}

static void Console_ResetField(void)
{
    Field_Clear(&g_consoleField);
    g_consoleField.widthInPixels = *(int *)imp_g_console_field_width;
    g_consoleField.charHeight = *(float *)imp_g_console_char_height;
    g_consoleField.fixedSize = 1;

    if (CL_AllLocalClientsDisconnected())
        SCR_UpdateScreen();
}

static void Console_ExecuteLine(void)
{
    clientConnection_t *clc = *(clientConnection_t **)imp_clc;

    if ((!clc || clc->state != 8) &&
        g_consoleField.buffer[0] != 0x5c &&
        g_consoleField.buffer[0] != 0x2f) {
        char temp[1024];
        I_strncpyz(temp, g_consoleField.buffer, sizeof(temp));
        Com_sprintf(g_consoleField.buffer, sizeof(g_consoleField.buffer), "\\%s", temp);
        g_consoleField.cursor++;
    }

    Com_Printf("]%s\n", g_consoleField.buffer);

    if (g_consoleField.buffer[0] == 0x5c || g_consoleField.buffer[0] == 0x2f) {
        Cbuf_AddText(g_consoleField.buffer + 1);
        Cbuf_AddText("\n");
    } else if (g_consoleField.buffer[0] && I_strncmp(g_consoleField.buffer, "rcon", 4) != 0) {
        Cbuf_AddText("cmd say ");
        Cbuf_AddText(g_consoleField.buffer);
        Cbuf_AddText("\n");
    }

    Console_SaveHistory();
    Console_ResetField();
}

static void Console_HistoryUp(void)
{
    if (nextHistoryLine - historyLine > 31 || historyLine <= 0)
        return;

    historyLine--;
    memcpy(&g_consoleField,
           &historyEditLines[Console_HistoryIndex(historyLine)],
           sizeof(g_consoleField));
    Field_AdjustScroll(&g_consoleField);
}

static void Console_HistoryDown(void)
{
    if (historyLine == nextHistoryLine)
        return;

    historyLine++;
    memcpy(&g_consoleField,
           &historyEditLines[Console_HistoryIndex(historyLine)],
           sizeof(g_consoleField));
    Field_AdjustScroll(&g_consoleField);
}

static void Console_CompleteCommand(void)
{
    completionString = Con_TokenizeInput();
    matchCount = 0;
    shortestMatch[0] = 0;

    if (!completionString || !completionString[0])
        return;

    Cmd_ForEach(FindMatches);
    Dvar_ForEach(FindMatches);

    if (matchCount == 0)
        return;

    Com_sprintf(g_consoleField.buffer, sizeof(g_consoleField.buffer), "\\%s", shortestMatch);

    if (matchCount == 1 || (hasExactMatch && Con_AnySpaceAfterCommand())) {
        if (Cmd_Argc() == 1)
            I_strncat(g_consoleField.buffer, sizeof(g_consoleField.buffer), " ");
        Console_UpdateFieldCursor();
        return;
    }

    Console_UpdateFieldCursor();
    Com_Printf("]%s\n", g_consoleField.buffer);
    Cmd_ForEach(PrintMatches);
    Dvar_ForEach(PrintMatches);
}

void Console_Key(int key)
{
    if (tolower(key) == 0x6c && Key_IsDown(K_CTRL)) {
        Cbuf_AddText("clear\n");
        return;
    }

    if (key == K_ENTER || key == K_KP_ENTER) {
        Console_ExecuteLine();
        return;
    }

    if (key == K_TAB) {
        Console_CompleteCommand();
        return;
    }

    if (key == K_UPARROW || (tolower(key) == 0x70 && Key_IsDown(K_CTRL)) ||
        (key == K_MWHEELUP && Key_IsDown(K_SHIFT))) {
        Console_HistoryUp();
        return;
    }

    if (key == K_DOWNARROW || (tolower(key) == 0x6e && Key_IsDown(K_CTRL)) ||
        (key == K_MWHEELDOWN && Key_IsDown(K_SHIFT))) {
        Console_HistoryDown();
        return;
    }

    if (key == K_PGUP || key == K_MWHEELUP) {
        Con_PageUp();
        if (key == K_MWHEELUP && Key_IsDown(K_CTRL))
            Con_PageUp();
        return;
    }

    if (key == K_PGDN || key == K_MWHEELDOWN) {
        Con_PageDown();
        if (key == K_MWHEELDOWN && Key_IsDown(K_CTRL))
            Con_PageDown();
        return;
    }

    if (key == K_HOME && Key_IsDown(K_CTRL)) {
        Con_Top();
        return;
    }

    if (key == K_END && Key_IsDown(K_CTRL)) {
        Con_Bottom();
        return;
    }

    Field_KeyDownEvent(&g_consoleField, key);
}

void Key_WriteBindings(fileHandle_t f)
{
    int keynum;

    FS_Printf(f, "unbindall\n");

    for (keynum = 0; keynum < 256; ++keynum) {
        const char *binding;
        const char *scan;

        binding = keys[keynum].binding;
        if (binding == NULL || *binding == '\0') {
            continue;
        }

        FS_Printf(f, "bind %s \"", Key_KeynumToStringInternal(keynum, 0));
        for (scan = keys[keynum].binding; *scan; ++scan) {
            if (*scan == '"') {
                FS_Printf(f, "\\\"");
            } else {
                FS_Printf(f, "%c", *scan);
            }
        }
        FS_Printf(f, "\"\n");
    }
}

void Key_Bindlist_f(void)
{
    int keynum;

    for (keynum = 0; keynum < 256; ++keynum) {
        const char *binding;

        binding = keys[keynum].binding;
        if (binding == NULL || *binding == '\0') {
            continue;
        }

        Com_Printf("%s \"%s\"\n", Key_KeynumToStringInternal(keynum, 0), binding);
    }
}

static inline __attribute__((always_inline))
qboolean
CL_DvarEnabledFromImport(void *importPtr)
{
    const dvar_t *dvar;

    if (!importPtr)
        return 0;

    dvar = *(const dvar_t **)importPtr;
    return dvar && dvar->current.enabled;
}

static inline __attribute__((always_inline)) void CL_ExecuteKeyBinding(int key, qboolean down, unsigned int time)
{
    const char *binding;
    char cmd[1024];

    if (key < 0 || key >= 256)
        return;

    binding = keys[key].binding;
    if (!binding || !binding[0]) {
        if (down && key >= 0xc8) {
            Com_Printf("%s is unbound, use controls menu to set.\n", Key_KeynumToString(key, 0));
        }
        return;
    }

    if (binding[0] == '+') {
        if (down) {
            Com_sprintf(cmd, sizeof(cmd), "%s %i %i\n", binding, key, time);
        } else {
            Com_sprintf(cmd, sizeof(cmd), "-%s %i %i\n", binding + 1, key, time);
        }
        Cbuf_AddText(cmd);
        return;
    }

    if (down) {
        Cbuf_AddText(binding);
        Cbuf_AddText("\n");
    }
}

static inline __attribute__((always_inline)) void CL_HandleEscapeKey(qboolean down)
{
    clientConnection_t *clc;
    clientActive_t *cl;

    if (!down)
        return;

    cl = *(clientActive_t **)imp_cl;
    if (cl->keyCatchers & 1) {
        Con_ToggleConsole_f();
        return;
    }

    if (cl->keyCatchers & 0x10) {
        Message_Key(0x1b);
        return;
    }
    if (cl->keyCatchers & 8) {
        UI_KeyEvent(0x1b, 1);
        return;
    }

    clc = *(clientConnection_t **)imp_clc;
    switch (clc->state) {
    case CA_CINEMATIC:
    case CA_LOGO:
        CL_StopLogoOrCinematic();
        break;
    case CA_CONNECTING:
    case CA_CHALLENGING:
    case CA_CONNECTED:
        CL_Disconnect();
        break;
    case CA_ACTIVE:
        UI_SetActiveMenu(clc->demoplaying ? 1 : 2);
        break;
    default:
        if (cls.uiStarted)
            UI_SetActiveMenu(1);
        break;
    }
}

static inline __attribute__((always_inline)) void CL_HandleConsoleToggleKey(qboolean down)
{
    clientActive_t *cl;

    if (!down)
        return;

    cl = *(clientActive_t **)imp_cl;
    if (!(cl->keyCatchers & 1) &&
        !CL_DvarEnabledFromImport(imp_com_sv_running) &&
        CL_DvarEnabledFromImport(imp_sv_disableClientConsole)) {
        return;
    }

    if (keys['`'].binding) {
        if (Con_IsActive()) {
            Con_ToggleConsole_f();
        } else {
            Con_ToggleConsole_f();
            Con_ToggleConsoleOutput();
        }
    } else {
        Con_ToggleConsole_f();
    }
}

void CL_KeyEvent(int key, const qboolean down, const unsigned int time)
{
    clientActive_t *cl;
    clientConnection_t *clc;
    qkey_t *keyState;
    int keyCatchers;
    qboolean repeated;
    qboolean bypassUi;

    if (key < 0 || key >= 256)
        return;

    cl = *(clientActive_t **)imp_cl;
    clc = *(clientConnection_t **)imp_clc;
    keyState = &keys[key];

    keyState->down = down;
    repeated = 0;

    if (down) {
        keyState->repeats++;
        if (keyState->repeats == 1) {
            ++*anykeydown;
        } else {
            repeated = 1;
        }
    } else {
        keyState->repeats = 0;
        if (*anykeydown > 0)
            --*anykeydown;
    }

#if COD2_FEATURE_GAMEPAD

    if (down && !repeated && (key < 0xcf || key > 0xde))
        CL_Gamepad_NoteKbmActive();
#endif

    if (key == '`' || key == '~') {
        CL_HandleConsoleToggleKey(down);
        return;
    }

    if (key == 0x1b) {
        CL_HandleEscapeKey(down);
        return;
    }

    if (repeated && !(cl->keyCatchers & (1 | 8 | 0x10)))
        return;

    if (!down) {
        CL_ExecuteKeyBinding(key, 0, time);
        if ((cl->keyCatchers & 8) && cls.uiStarted)
            UI_KeyEvent(key, 0);
        return;
    }

    if (cl->cgameInitialized && CG_KeyInterceptEvent(key, down))
        return;

    keyCatchers = cl->keyCatchers;
    if (keyCatchers & 1) {
        Console_Key(key);
        return;
    }

    bypassUi = 0;
    if (keyCatchers & 8) {
        qboolean bypassMouseInput = CL_DvarEnabledFromImport(imp_cl_bypassMouseInput);
        qboolean uiWantsKey = UI_checkKeyExec(key);
        if (bypassMouseInput && key >= 0xc8 && key <= 0xca) {
            bypassUi = 1;
        } else if (!uiWantsKey) {
            bypassUi = 1;
        }

        if (!bypassUi) {
            UI_KeyEvent(key, down);
            return;
        }
    }

    if (keyCatchers & 0x10) {
        Message_Key(key);
        return;
    }

    if (clc->state == CA_DISCONNECTED) {
        Console_Key(key);
        return;
    }

    CL_ExecuteKeyBinding(key, 1, time);
}

void Key_ClearStates(void)
{
    int i;
    *(int *)anykeydown = 0;

    for (i = 0; i < 256; i++) {
        if (keys[i].down) {
            CL_KeyEvent(i, 0, 0);
        }
        keys[i].down = 0;
        keys[i].repeats = 0;
    }
}

keyname_t keynames[99] = {
    { (char *)&str_002a6bf0, 0x9 },
    { (char *)&str_002a6bf4, 0xd },
    { (char *)&str_002a6bfc, 0x1b },
    { (char *)&str_002a6c04, 0x20 },
    { (char *)&str_002a6c0c, 0x7f },
    { (char *)&str_002a6c18, 0x9a },
    { (char *)&str_002a6c20, 0x9b },
    { (char *)&str_002a6c2c, 0x9c },
    { (char *)&str_002a6c38, 0x9d },
    { (char *)&str_002a6c44, 0x9e },
    { (char *)&str_002a6c48, 0x9f },
    { (char *)&str_002a6c50, 0xa0 },
    { (char *)&str_002a6c58, 0x97 },
    { (char *)&str_002a6c64, 0xa7 },
    { (char *)&str_002a6c68, 0xa8 },
    { (char *)&str_002a6c6c, 0xa9 },
    { (char *)&str_002a6c70, 0xaa },
    { (char *)&str_002a6c74, 0xab },
    { (char *)&str_002a6c78, 0xac },
    { (char *)&str_002a6c7c, 0xad },
    { (char *)&str_002a6c80, 0xae },
    { (char *)&str_002a6c84, 0xaf },
    { (char *)&str_002a6c88, 0xb0 },
    { (char *)&str_002a6c8c, 0xb1 },
    { (char *)&str_002a6c90, 0xb2 },
    { (char *)&str_002a6c94, 0xb3 },
    { (char *)&str_002a6c98, 0xb4 },
    { (char *)&str_002a6c9c, 0xb5 },
    { (char *)&str_002a6ca0, 0xa1 },
    { (char *)&str_002a6ca4, 0xa2 },
    { (char *)&str_002a6ca8, 0xa3 },
    { (char *)&str_002a6cb0, 0xa4 },
    { (char *)&str_002a6cb8, 0xa5 },
    { (char *)&str_002a6cc0, 0xa6 },
    { (char *)&str_002a6cc4, 0xc8 },
    { (char *)&str_002a6ccc, 0xc9 },
    { (char *)&str_002a6cd4, 0xca },
    { (char *)&str_002a6cdc, 0xcb },
    { (char *)&str_002a6ce4, 0xcc },
    { (char *)&str_002a6cec, 0xce },
    { (char *)&str_002a6cf8, 0xcd },
    { (char *)&str_002a6d04, 0xcf },
    { (char *)&str_002a6d0c, 0xd0 },
    { (char *)&str_002a6d14, 0xd1 },
    { (char *)&str_002a6d1c, 0xd2 },
    { (char *)&str_002a6d24, 0xd3 },
    { (char *)&str_002a6d2c, 0xd4 },
    { (char *)&str_002a6d34, 0xd5 },
    { (char *)&str_002a6d3c, 0xd6 },
    { (char *)&str_002a6d44, 0xd7 },
    { (char *)&str_002a6d4c, 0xd8 },
    { (char *)&str_002a6d54, 0xd9 },
    { (char *)&str_002a6d5c, 0xda },
    { (char *)&str_002a6d64, 0xdb },
    { (char *)&str_002a6d6c, 0xdc },
    { (char *)&str_002a6d74, 0xdd },
    { (char *)&str_002a6d7c, 0xde },
    { (char *)&str_002a6d84, 0xb6 },
    { (char *)&str_002a6d8c, 0xb7 },
    { (char *)&str_002a6d98, 0xb8 },
    { (char *)&str_002a6da0, 0xb9 },
    { (char *)&str_002a6db0, 0xba },
    { (char *)&str_002a6db8, 0xbb },
    { (char *)&str_002a6dc8, 0xbc },
    { (char *)&str_002a6dd0, 0xbd },
    { (char *)&str_002a6de0, 0xbe },
    { (char *)&str_002a6de8, 0xbf },
    { (char *)&str_002a6df4, 0xc0 },
    { (char *)&str_002a6dfc, 0xc1 },
    { (char *)&str_002a6e04, 0xc2 },
    { (char *)&str_002a6e10, 0xc3 },
    { (char *)&str_002a6e1c, 0xc4 },
    { (char *)&str_002a6e24, 0xc5 },
    { (char *)&str_002a6e30, 0xc6 },
    { (char *)&str_002a6e38, 0xc7 },
    { (char *)&str_002a6e44, 0x99 },
    { (char *)&str_002a6e4c, 0x3b },
    { (char *)&str_002a6e58, 0x96 },
    { (char *)&str_002a6e60, 0x80 },
    { (char *)&str_002a6e64, 0x81 },
    { (char *)&str_002a6e68, 0x82 },
    { (char *)&str_002a6e6c, 0x83 },
    { (char *)&str_002a6e70, 0x84 },
    { (char *)&str_002a6e74, 0x85 },
    { (char *)&str_002a6e78, 0x86 },
    { (char *)&str_002a6e7c, 0x87 },
    { (char *)&str_002a6e80, 0x88 },
    { (char *)&str_002a6e84, 0x89 },
    { (char *)&str_002a6e88, 0x8a },
    { (char *)&str_002a6e8c, 0x8b },
    { (char *)&str_002a6e90, 0x8c },
    { (char *)&str_002a6e94, 0x8d },
    { (char *)&str_002a6e98, 0x8e },
    { (char *)&str_002a6e9c, 0x8f },
    { (char *)&str_002a6ea0, 0x90 },
    { (char *)&str_002a6ea4, 0x91 },
    { (char *)&str_002a6ea8, 0x92 },
    { (char *)&str_002a6eac, 0x93 },
    { 0, 0x0 }
};
keyname_t keynames_localized[100] = {
    { (char *)&str_002a6808, 0x9 },
    { (char *)&str_002a6810, 0xd },
    { (char *)&str_002a681c, 0x1b },
    { (char *)&str_002a6828, 0x20 },
    { (char *)&str_002a6834, 0x7f },
    { (char *)&str_002a6844, 0x9a },
    { (char *)&str_002a6850, 0x9b },
    { (char *)&str_002a6860, 0x9c },
    { (char *)&str_002a6870, 0x9d },
    { (char *)&str_002a6880, 0x9e },
    { (char *)&str_002a6888, 0x9f },
    { (char *)&str_002a6894, 0xa0 },
    { (char *)&str_002a68a0, 0x97 },
    { (char *)&str_002a68b0, 0xa7 },
    { (char *)&str_002a68b8, 0xa8 },
    { (char *)&str_002a68c0, 0xa9 },
    { (char *)&str_002a68c8, 0xaa },
    { (char *)&str_002a68d0, 0xab },
    { (char *)&str_002a68d8, 0xac },
    { (char *)&str_002a68e0, 0xad },
    { (char *)&str_002a68e8, 0xae },
    { (char *)&str_002a68f0, 0xaf },
    { (char *)&str_002a68f8, 0xb0 },
    { (char *)&str_002a6900, 0xb1 },
    { (char *)&str_002a6908, 0xb2 },
    { (char *)&str_002a6910, 0xb3 },
    { (char *)&str_002a6918, 0xb4 },
    { (char *)&str_002a6920, 0xb5 },
    { (char *)&str_002a6928, 0xa1 },
    { (char *)&str_002a6930, 0xa2 },
    { (char *)&str_002a6938, 0xa3 },
    { (char *)&str_002a6944, 0xa4 },
    { (char *)&str_002a6950, 0xa5 },
    { (char *)&str_002a695c, 0xa6 },
    { (char *)&str_002a6964, 0xc8 },
    { (char *)&str_002a6970, 0xc9 },
    { (char *)&str_002a697c, 0xca },
    { (char *)&str_002a6988, 0xcb },
    { (char *)&str_002a6994, 0xcc },
    { (char *)&str_002a69a0, 0xce },
    { (char *)&str_002a69b0, 0xcd },
    { (char *)&str_002a69c0, 0xcf },
    { (char *)&str_002a69cc, 0xd0 },
    { (char *)&str_002a69d8, 0xd1 },
    { (char *)&str_002a69e4, 0xd2 },
    { (char *)&str_002a69f0, 0xd3 },
    { (char *)&str_002a69fc, 0xd4 },
    { (char *)&str_002a6a08, 0xd5 },
    { (char *)&str_002a6a14, 0xd6 },
    { (char *)&str_002a6a20, 0xd7 },
    { (char *)&str_002a6a2c, 0xd8 },
    { (char *)&str_002a6a38, 0xd9 },
    { (char *)&str_002a6a44, 0xda },
    { (char *)&str_002a6a50, 0xdb },
    { (char *)&str_002a6a5c, 0xdc },
    { (char *)&str_002a6a68, 0xdd },
    { (char *)&str_002a6a74, 0xde },
    { (char *)&str_002a6a80, 0xb6 },
    { (char *)&str_002a6a8c, 0xb7 },
    { (char *)&str_002a6a9c, 0xb8 },
    { (char *)&str_002a6aa8, 0xb9 },
    { (char *)&str_002a6abc, 0xba },
    { (char *)&str_002a6ac8, 0xbb },
    { (char *)&str_002a6adc, 0xbc },
    { (char *)&str_002a6ae8, 0xbd },
    { (char *)&str_002a6afc, 0xbe },
    { (char *)&str_002a6b08, 0xbf },
    { (char *)&str_002a6b18, 0xc0 },
    { (char *)&str_002a6b24, 0xc1 },
    { (char *)&str_002a6b30, 0xc2 },
    { (char *)&str_002a6b40, 0xc3 },
    { (char *)&str_002a6b50, 0xc4 },
    { (char *)&str_002a6b5c, 0xc5 },
    { (char *)&str_002a6b6c, 0xc6 },
    { (char *)&str_002a6b78, 0xc7 },
    { (char *)&str_002a6b88, 0x99 },
    { (char *)&str_002a6b94, 0x3b },
    { (char *)&str_002a6ba4, 0x96 },
    { (char *)&str_002a6bb0, 0x80 },
    { (char *)&str_002a6bb4, 0x81 },
    { (char *)&str_002a6bb8, 0x82 },
    { (char *)&str_002a67f0, 0x83 },
    { (char *)&str_002a6bbc, 0x84 },
    { (char *)&str_002a6bc0, 0x85 },
    { (char *)&str_002a6bc4, 0x86 },
    { (char *)&str_002a6bc8, 0x87 },
    { (char *)&str_002a6804, 0x88 },
    { (char *)&str_002a67fc, 0x89 },
    { (char *)&str_002a67f4, 0x8a },
    { (char *)&str_002a6bcc, 0x8b },
    { (char *)&str_002a6bd0, 0x8c },
    { (char *)&str_002a6bd4, 0x8d },
    { (char *)&str_002a6bd8, 0x8e },
    { (char *)&str_002a6bdc, 0x8f },
    { (char *)&str_002a6be0, 0x90 },
    { (char *)&str_002a6be4, 0x91 },
    { (char *)&str_002a6be8, 0x92 },
    { (char *)&str_002a6bec, 0x93 },
    { 0, 0x0 },
    { 0, 0x0 }
};

const char str_002a67f0[] = "\xff" "fd";
const char str_002a67f4[] = "\xff" "fd";
const char str_002a67fc[] = "\xff" "fd";
const char str_002a6804[] = "\xff" "fd";
const char str_002a6808[] = "KEY_TAB";
const char str_002a6810[] = "KEY_ENTER";
const char str_002a681c[] = "KEY_ESCAPE";
const char str_002a6828[] = "KEY_SPACE";
const char str_002a6834[] = "KEY_BACKSPACE";
const char str_002a6844[] = "KEY_UPARROW";
const char str_002a6850[] = "KEY_DOWNARROW";
const char str_002a6860[] = "KEY_LEFTARROW";
const char str_002a6870[] = "KEY_RIGHTARROW";
const char str_002a6880[] = "KEY_ALT";
const char str_002a6888[] = "KEY_CTRL";
const char str_002a6894[] = "KEY_SHIFT";
const char str_002a68a0[] = "KEY_CAPSLOCK";
const char str_002a68b0[] = "KEY_F1";
const char str_002a68b8[] = "KEY_F2";
const char str_002a68c0[] = "KEY_F3";
const char str_002a68c8[] = "KEY_F4";
const char str_002a68d0[] = "KEY_F5";
const char str_002a68d8[] = "KEY_F6";
const char str_002a68e0[] = "KEY_F7";
const char str_002a68e8[] = "KEY_F8";
const char str_002a68f0[] = "KEY_F9";
const char str_002a68f8[] = "KEY_F10";
const char str_002a6900[] = "KEY_F11";
const char str_002a6908[] = "KEY_F12";
const char str_002a6910[] = "KEY_F13";
const char str_002a6918[] = "KEY_F14";
const char str_002a6920[] = "KEY_F15";
const char str_002a6928[] = "KEY_INS";
const char str_002a6930[] = "KEY_DEL";
const char str_002a6938[] = "KEY_PGDN";
const char str_002a6944[] = "KEY_PGUP";
const char str_002a6950[] = "KEY_HOME";
const char str_002a695c[] = "KEY_END";
const char str_002a6964[] = "KEY_MOUSE1";
const char str_002a6970[] = "KEY_MOUSE2";
const char str_002a697c[] = "KEY_MOUSE3";
const char str_002a6988[] = "KEY_MOUSE4";
const char str_002a6994[] = "KEY_MOUSE5";
const char str_002a69a0[] = "KEY_MWHEELUP";
const char str_002a69b0[] = "KEY_MWHEELDOWN";
const char str_002a69c0[] = "KEY_AUX1";
const char str_002a69cc[] = "KEY_AUX2";
const char str_002a69d8[] = "KEY_AUX3";
const char str_002a69e4[] = "KEY_AUX4";
const char str_002a69f0[] = "KEY_AUX5";
const char str_002a69fc[] = "KEY_AUX6";
const char str_002a6a08[] = "KEY_AUX7";
const char str_002a6a14[] = "KEY_AUX8";
const char str_002a6a20[] = "KEY_AUX9";
const char str_002a6a2c[] = "KEY_AUX10";
const char str_002a6a38[] = "KEY_AUX11";
const char str_002a6a44[] = "KEY_AUX12";
const char str_002a6a50[] = "KEY_AUX13";
const char str_002a6a5c[] = "KEY_AUX14";
const char str_002a6a68[] = "KEY_AUX15";
const char str_002a6a74[] = "KEY_AUX16";
const char str_002a6a80[] = "KEY_KP_HOME";
const char str_002a6a8c[] = "KEY_KP_UPARROW";
const char str_002a6a9c[] = "KEY_KP_PGUP";
const char str_002a6aa8[] = "KEY_KP_LEFTARROW";
const char str_002a6abc[] = "KEY_KP_5";
const char str_002a6ac8[] = "KEY_KP_RIGHTARROW";
const char str_002a6adc[] = "KEY_KP_END";
const char str_002a6ae8[] = "KEY_KP_DOWNARROW";
const char str_002a6afc[] = "KEY_KP_PGDN";
const char str_002a6b08[] = "KEY_KP_ENTER";
const char str_002a6b18[] = "KEY_KP_INS";
const char str_002a6b24[] = "KEY_KP_DEL";
const char str_002a6b30[] = "KEY_KP_SLASH";
const char str_002a6b40[] = "KEY_KP_MINUS";
const char str_002a6b50[] = "KEY_KP_PLUS";
const char str_002a6b5c[] = "KEY_KP_NUMLOCK";
const char str_002a6b6c[] = "KEY_KP_STAR";
const char str_002a6b78[] = "KEY_KP_EQUALS";
const char str_002a6b88[] = "KEY_PAUSE";
const char str_002a6b94[] = "KEY_SEMICOLON";
const char str_002a6ba4[] = "KEY_COMMAND";
const char str_002a6bb0[] = "\xff" "fd";
const char str_002a6bb4[] = "\xff" "fd";
const char str_002a6bb8[] = "\xff" "fd";
const char str_002a6bbc[] = "\xff" "fd";
const char str_002a6bc0[] = "\xff" "fd";
const char str_002a6bc4[] = "\xff" "fd";
const char str_002a6bc8[] = "\xff" "fd";
const char str_002a6bcc[] = "\xff" "fd";
const char str_002a6bd0[] = "\xff" "fd";
const char str_002a6bd4[] = "\xff" "fd";
const char str_002a6bd8[] = "\xff" "fd";
const char str_002a6bdc[] = "\xff" "fd";
const char str_002a6be0[] = "\xff" "fd";
const char str_002a6be4[] = "\xff" "fd";
const char str_002a6be8[] = "\xff" "fd";
const char str_002a6bec[] = "\xff" "fd";
const char str_002a6bf0[] = "TAB";
const char str_002a6bf4[] = "ENTER";
const char str_002a6bfc[] = "ESCAPE";
const char str_002a6c04[] = "SPACE";
const char str_002a6c0c[] = "BACKSPACE";
const char str_002a6c18[] = "UPARROW";
const char str_002a6c20[] = "DOWNARROW";
const char str_002a6c2c[] = "LEFTARROW";
const char str_002a6c38[] = "RIGHTARROW";
const char str_002a6c44[] = "ALT";
const char str_002a6c48[] = "CTRL";
const char str_002a6c50[] = "SHIFT";
const char str_002a6c58[] = "CAPSLOCK";
const char str_002a6c64[] = "F1";
const char str_002a6c68[] = "F2";
const char str_002a6c6c[] = "F3";
const char str_002a6c70[] = "F4";
const char str_002a6c74[] = "F5";
const char str_002a6c78[] = "F6";
const char str_002a6c7c[] = "F7";
const char str_002a6c80[] = "F8";
const char str_002a6c84[] = "F9";
const char str_002a6c88[] = "F10";
const char str_002a6c8c[] = "F11";
const char str_002a6c90[] = "F12";
const char str_002a6c94[] = "F13";
const char str_002a6c98[] = "F14";
const char str_002a6c9c[] = "F15";
const char str_002a6ca0[] = "INS";
const char str_002a6ca4[] = "DEL";
const char str_002a6ca8[] = "PGDN";
const char str_002a6cb0[] = "PGUP";
const char str_002a6cb8[] = "HOME";
const char str_002a6cc0[] = "END";
const char str_002a6cc4[] = "MOUSE1";
const char str_002a6ccc[] = "MOUSE2";
const char str_002a6cd4[] = "MOUSE3";
const char str_002a6cdc[] = "MOUSE4";
const char str_002a6ce4[] = "MOUSE5";
const char str_002a6cec[] = "MWHEELUP";
const char str_002a6cf8[] = "MWHEELDOWN";
const char str_002a6d04[] = "AUX1";
const char str_002a6d0c[] = "AUX2";
const char str_002a6d14[] = "AUX3";
const char str_002a6d1c[] = "AUX4";
const char str_002a6d24[] = "AUX5";
const char str_002a6d2c[] = "AUX6";
const char str_002a6d34[] = "AUX7";
const char str_002a6d3c[] = "AUX8";
const char str_002a6d44[] = "AUX9";
const char str_002a6d4c[] = "AUX10";
const char str_002a6d54[] = "AUX11";
const char str_002a6d5c[] = "AUX12";
const char str_002a6d64[] = "AUX13";
const char str_002a6d6c[] = "AUX14";
const char str_002a6d74[] = "AUX15";
const char str_002a6d7c[] = "AUX16";
const char str_002a6d84[] = "KP_HOME";
const char str_002a6d8c[] = "KP_UPARROW";
const char str_002a6d98[] = "KP_PGUP";
const char str_002a6da0[] = "KP_LEFTARROW";
const char str_002a6db0[] = "KP_5";
const char str_002a6db8[] = "KP_RIGHTARROW";
const char str_002a6dc8[] = "KP_END";
const char str_002a6dd0[] = "KP_DOWNARROW";
const char str_002a6de0[] = "KP_PGDN";
const char str_002a6de8[] = "KP_ENTER";
const char str_002a6df4[] = "KP_INS";
const char str_002a6dfc[] = "KP_DEL";
const char str_002a6e04[] = "KP_SLASH";
const char str_002a6e10[] = "KP_MINUS";
const char str_002a6e1c[] = "KP_PLUS";
const char str_002a6e24[] = "KP_NUMLOCK";
const char str_002a6e30[] = "KP_STAR";
const char str_002a6e38[] = "KP_EQUALS";
const char str_002a6e44[] = "PAUSE";
const char str_002a6e4c[] = "SEMICOLON";
const char str_002a6e58[] = "COMMAND";
const char str_002a6e60[] = "181";
const char str_002a6e64[] = "191";
const char str_002a6e68[] = "223";
const char str_002a6e6c[] = "224";
const char str_002a6e70[] = "225";
const char str_002a6e74[] = "228";
const char str_002a6e78[] = "229";
const char str_002a6e7c[] = "230";
const char str_002a6e80[] = "231";
const char str_002a6e84[] = "232";
const char str_002a6e88[] = "233";
const char str_002a6e8c[] = "236";
const char str_002a6e90[] = "241";
const char str_002a6e94[] = "242";
const char str_002a6e98[] = "243";
const char str_002a6e9c[] = "246";
const char str_002a6ea0[] = "248";
const char str_002a6ea4[] = "249";
const char str_002a6ea8[] = "250";
const char str_002a6eac[] = "252";
