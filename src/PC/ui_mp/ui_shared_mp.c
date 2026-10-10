#include "common_types.h"
#include "imports.h"
#include "bytematch.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

extern commandDef_t commandList[28];
extern bind_t g_bindings[56];

qboolean g_editingField = 0x0;

COD2_ASSERT_FIELD(struct displayContextDef_s, realTime, 0x4);
COD2_ASSERT_FIELD(struct displayContextDef_s, blurRadiusOut, 0x28);
COD2_ASSERT_FIELD(struct displayContextDef_s, Menus, 0x2c);
COD2_ASSERT_FIELD(struct displayContextDef_s, menuCount, 0x22c);
COD2_ASSERT_FIELD(struct displayContextDef_s, menuStack, 0x230);
COD2_ASSERT_FIELD(struct itemDef_s, type, 0x270);
COD2_ASSERT_FIELD(struct itemDef_s, text, 0x294);
COD2_ASSERT_FIELD(struct itemDef_s, parent, 0x29c);
COD2_ASSERT_FIELD(struct itemDef_s, dvar, 0x2c0);
COD2_ASSERT_FIELD(struct itemDef_s, window.foreColor, 0x1cc);
COD2_ASSERT_FIELD(struct itemDef_s, window.backColor, 0x1dc);
COD2_ASSERT_FIELD(struct itemDef_s, window.borderColor, 0x1ec);
COD2_ASSERT_FIELD(struct menuDef_t, items, 0x27c);
COD2_ASSERT_FIELD(struct menuDef_t, onClose, 0x248);
COD2_ASSERT_FIELD(struct menuDef_t, onKey, 0x250);
COD2_ASSERT_FIELD(struct menuDef_t, window.dynamicFlags, 0xe8);
COD2_ASSERT_FIELD(struct editFieldDef_s, maxPaintChars, 0x18);
COD2_ASSERT_FIELD(struct editFieldDef_s, paintOffset, 0x1c);
COD2_ASSERT_FIELD(struct editFieldDef_s, maxChars,         0x10);
COD2_ASSERT_FIELD(struct editFieldDef_s, maxCharsGotoNext, 0x14);
COD2_ASSERT_FIELD(struct itemDef_s, action, 0x2b0);

extern const char *va(const char *fmt, ...);
extern void Cbuf_ExecuteText(int exec_when, const char *text);
extern int I_stricmp(const char *s0, const char *s1);
extern int I_strncmp(const char *s0, const char *s1, int n);
extern void I_strncat(char *dest, int destsize, const char *src);
extern void Com_Error(int code, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern const char *Com_ParseOnLine(const char **p);
extern const char *SEH_StringEd_GetString(const char *key);
extern Bool Dvar_GetBool(const char *name);
extern int Dvar_GetInt(const char *name);
extern float Dvar_GetFloat(const char *name);
extern const char *Dvar_GetString(const char *name);
extern void Dvar_SetFromStringByName(const char *dvarName, const char *string);
extern struct dvar_s *Dvar_FindVar(const char *name);
extern int UI_PlayLocalSoundAlias(snd_alias_list_t *aliasList);
extern int UI_PlayLocalSoundAliasByName(const char *name);
extern void UI_RunMenuScript(const char **args);
extern qboolean UI_ClientIsInGame(void);
extern void UI_Pause(qboolean pause);
extern void UI_DrawHandlePic(float x, float y, float w, float h, int horzAlign, int vertAlign, const vec_t *color, MaterialHandle material);
extern void UI_DrawRect(float x, float y, float w, float h, int horzAlign, int vertAlign, float size, const vec_t *color);
extern void UI_FillRect(float x, float y, float w, float h, int horzAlign, int vertAlign, const vec_t *color);
extern void UI_DrawSides(float x, float y, float w, float h, int horzAlign, int vertAlign, float size, const vec_t *color);
extern void UI_DrawTopBottom(float x, float y, float w, float h, int horzAlign, int vertAlign, float size, const vec_t *color);
extern void UI_DrawLoadBar(float x, float y, float w, float h, int horzAlign, int vertAlign, const vec_t *color, MaterialHandle material);
extern int UI_TextWidth(const char *text, int maxChars, FontHandle font, float scale);
extern int UI_TextHeight(FontHandle font, float scale);
extern const char *UI_SafeTranslateString(const char *key);
extern int UI_OwnerDrawWidth(int ownerDraw, FontHandle font, float scale);
extern qboolean UI_OwnerDrawVisible(int flags);
extern void UI_OwnerDraw(float x, float y, float w, float h, int horzAlign, int vertAlign, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, FontHandle font, float scale, vec_t *color, MaterialHandle shader, int textStyle);
extern qboolean UI_OwnerDrawHandleKey(int ownerDraw, int flags, int *special, int key);
extern int UI_FeederCount(float feederID);
extern MaterialHandle UI_FeederItemImage(float feederID, int index);
extern const char *UI_FeederItemText(float feederID, int index, int column, MaterialHandle *handle);
extern void UI_FeederSelection(float feederID, int index);
extern void UI_OverrideCursorPos(itemDef_t *item);
extern qboolean UI_OwnerDrawVisible(int flags);
extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);
extern void CalcScreenX(float *x, int horzAlign);
extern void CalcScreenY(float *y, int vertAlign);
extern qboolean Float_Parse(const char **p, float *f);
extern qboolean Int_Parse(const char **p, int *i);
extern qboolean Rect_Parse(const char **p, rectDef_t *r);
extern MaterialHandle CL_RegisterMaterialNoMip(const char *name, int imageTrack);
extern const char *CL_GetConfigString(int index);
extern void CIN_DrawCinematic(int handle);
extern int CIN_PlayCinematic(const char *name, int x, int y, int w, int h, int flags);
extern int CIN_RunCinematic(int handle);
extern void CIN_SetExtents(int handle, int x, int y, int w, int h);
extern int CIN_StopCinematic(int handle);
extern void Key_GetBindingBuf(int keynum, char *buf, int buflen);
extern void Key_KeynumToStringBuf(int keynum, char *buf, int buflen);
extern void Key_SetBinding(int keynum, const char *binding);
extern void Key_SetOverstrikeMode(qboolean state);
extern int Key_GetCatcher(void);
extern void Key_SetCatcher(int catcher);
extern qboolean Item_EnableShowViaDvar(itemDef_t *item, int flags);
extern int Item_GetCursorPosOffset(const itemDef_t *item, const char *buff, int direction);
extern struct listBoxDef_s *Item_GetListBoxDef(itemDef_t *item);
extern struct multiDef_s *Item_GetMultiDef(itemDef_t *item);
extern void Item_SetCursorPos(itemDef_t *item, int cursorPos);
extern void Item_SetScreenCoords(itemDef_t *item, float x, float y, int horzAlign, int vertAlign);
extern void Item_SetTextRect(itemDef_t *item, const rectDef_t *textRect);
extern void Item_SetupKeywordHash(void);
extern qboolean ListBox_HasValidCursorPos(itemDef_t *item);
extern void ListBox_SetCursorPos(itemDef_t *item, int cursorPos);
extern void ListBox_SetEndPos(itemDef_t *item, int endPos);
extern void ListBox_SetStartPos(itemDef_t *item, int startPos);
extern void Menu_SetCursorItem(menuDef_t *menu, int cursorItem);
extern void Menu_SetupKeywordHash(void);
extern void Menu_UpdatePosition(menuDef_t *menu);
extern void Window_AddDynamicFlags(itemDef_t *item, int flags);
extern void Window_RemoveDynamicFlags(itemDef_t *item, int flags);
extern void Window_SetDynamicFlags(itemDef_t *item, int flags);
extern void Window_SetOffsetTime(itemDef_t *item, int time);
extern void Window_SetRect(void *item, const rectDef_t *rect);
extern void Window_SetRectClient(void *item, const rectDef_t *rect);
extern void Window_SetRectEffects0(void *item, const rectDef_t *rect);
extern void Window_SetRectEffects1(void *item, const rectDef_t *rect);
extern int I_isdigit(int c);
extern int I_isforfilename(int c);
extern int Sys_Milliseconds(void);
extern char Com_GetDecimalDelimiter(void);

void diag_item_paint_enter(void *item)
{
    (void)item;
}
void diag_item_paint_skip(void *item, int reason)
{
    (void)item;
    (void)reason;
}
void diag_item_paint_draw(void *item)
{
    (void)item;
}

extern unsigned char updateScreenCalled[];
extern qboolean g_waitingForKey;
extern qboolean g_editingField;
extern itemDef_t *g_editItem;
extern int scriptCommandCount;
static rectDef_t rect;
static qboolean inHandleKey;
static scrollInfo_t scrollInfo;
static void (*captureFunc)();
static void *captureData;
static itemDef_t *itemCapture;
static itemDef_t *g_bindItem;
static qboolean debugMode;
static int lastListBoxClickTime;
extern void *imp_sharedUiInfo;

void LerpColor(vec_t *a, vec_t *b, vec_t *c, float t);
qboolean String_Parse(const char **p, char *out, int len);
int Menu_ItemsMatchingGroup(menuDef_t *menu, const char *name);
itemDef_t *Menu_GetMatchingItemByNumber(menuDef_t *menu, int index, const char *name);
void Script_SetColor(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_SetBackground(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Menu_ShowItemByName(menuDef_t *menu, const char *p, qboolean bShow);
void Menu_FadeItemByName(menuDef_t *menu, const char *p, qboolean fadeOut);
menuDef_t *Menus_FindByName(displayContextDef_t *dc, const char *p);
void Script_Show(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_Hide(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_FadeIn(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_FadeOut(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_SetDvar(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_ExecNow(displayContextDef_t *dc, itemDef_t *item, const char **args);
static void Script_ConditionalExecHandler(int execWhen, Bool (*shouldExec)(const char *, const char *), const char **args);
static void Script_ConditionalExecHandler_impl(int execWhen, itemDef_t *item, const char **args, Bool (*shouldExec)(const char *, const char *));
static Bool Script_ExecIfStringsEqual(const char *dvarValue, const char *testValue);
static Bool Script_ExecIfIntsEqual(const char *dvarValue, const char *testValue);
static Bool Script_ExecIfFloatsEqual(const char *dvarValue, const char *testValue);
void Script_ExecOnDvarStringValue(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_ExecOnDvarIntValue(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_ExecOnDvarFloatValue(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_ExecNowOnDvarStringValue(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_ExecNowOnDvarIntValue(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_ExecNowOnDvarFloatValue(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_Play(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_ScriptMenuResponse(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Item_RunScript(displayContextDef_t *dc, itemDef_t *item, const char *s);
void Controls_GetConfig(void);
void Controls_SetConfig(qboolean restart);
void Controls_SetDefaults(void);
void BindingFromName(const char *dvar, char *nameBind);
qboolean GetCommandHasBinding(const char *command);
int GetKeyBindings(const char *command, char (*bindings)[128]);
int GetKeyBindingLocalizedString(const char *command, char *keys);
qboolean Display_KeyBindPending(void);
int Menu_Count(displayContextDef_t *dc);
void UI_AddMenuList(displayContextDef_t *dc, MenuList *menuList);
void Item_MouseLeave(displayContextDef_t *dc, itemDef_t *item);
float Item_Slider_ThumbPosition(itemDef_t *item);
qboolean Menu_CheckOnKey(displayContextDef_t *dc, menuDef_t *menu, int key);
qboolean Menus_AnyFullScreenVisible(displayContextDef_t *dc);
menuDef_t *Menu_GetFocused(displayContextDef_t *dc);
int Item_ListBox_MaxScroll(itemDef_t *item);
int Item_ListBox_ThumbPosition(itemDef_t *item);
int Item_ListBox_ThumbDrawPosition(displayContextDef_t *dc, itemDef_t *item);
static void Scroll_Slider_SetThumbPos_impl(byte *dc, byte *item);
static void Scroll_Slider_ThumbFunc(displayContextDef_t *dc, void *p);
void Menus_Close(displayContextDef_t *dc, menuDef_t *menu);
void Menus_CloseAll(displayContextDef_t *dc);
void Item_SetTextExtents(itemDef_t *item, int *width, int *height, const char *text);
void Item_Text_AutoWrapped_Paint(itemDef_t *item, const char *textPtr, vec_t *color);
void Item_Text_Wrapped_Paint(itemDef_t *item, const char *textPtr, vec_t *color);
void Item_ListBox_Paint(displayContextDef_t *dc, itemDef_t *item);
editFieldDef_t *Item_GetEditFieldDef(itemDef_t *item);
const char *Dvar_GetVariantString(const char *dvarName);
void I_strncpyz(char *dest, const char *src, int destsize);
int Key_GetOverstrikeMode(void);
FontHandle UI_GetFontHandle(int fontEnum, float scale);
void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style);
void UI_DrawTextWithCursor(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style, int cursorPos, int cursor);
void Script_SetItemColor(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Window_Paint(displayContextDef_t *dc, float (*w)[4][32], float fadeAmount, float fadeInAmount, float fadeClamp, float fadeCycle);
void Menu_Setup(displayContextDef_t *dc);
void Menus_CloseByName(displayContextDef_t *dc, const char *p);
void Menu_SetFeederSelection(displayContextDef_t *dc, menuDef_t *menu, int feeder, int index, const char *name);
void Script_Exec(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Menu_TransitionItemByName(menuDef_t *menu, const char *p, rectDef_t rectFrom, rectDef_t rectTo, int time, float amt);
void Script_Transition(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Menu_OrbitItemByName(menuDef_t *menu, const char *p, float x, float y, float cx, float cy, int time);
void Script_Orbit(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_CloseForGameType(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_Close(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_InGameClose(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Item_TextColor(displayContextDef_t *dc, itemDef_t *item, vec4_t *newColor);
void Item_Text_Paint(displayContextDef_t *dc, itemDef_t *item);
void Item_TextField_Paint(displayContextDef_t *dc, itemDef_t *item);
void Item_YesNo_Paint(displayContextDef_t *dc, itemDef_t *item);
void Item_Slider_Paint(displayContextDef_t *dc, itemDef_t *item);
void Item_Bind_Paint(displayContextDef_t *dc, itemDef_t *item);
void Item_OwnerDraw_Paint(displayContextDef_t *dc, itemDef_t *item);
static void Item_List_Paint(displayContextDef_t *dc, itemDef_t *item, const char *text);
qboolean Item_Slider_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key, qboolean down);
qboolean Item_YesNo_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key);
qboolean Item_SetFocus(displayContextDef_t *dc, itemDef_t *item, float x, float y);
void Script_SetFocusByDvar(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_SetFocus(displayContextDef_t *dc, itemDef_t *item, const char **args);
int Item_ListBox_OverLB(itemDef_t *item, float x, float y);
void Item_ListBox_MouseEnter(itemDef_t *item, float x, float y);
void Item_MouseEnter(displayContextDef_t *dc, itemDef_t *item, float x, float y);
qboolean Item_ListBox_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key, qboolean down, qboolean force);
static void Scroll_ListBox_ThumbFunc(displayContextDef_t *dc, void *p);
static void Scroll_ListBox_AutoFunc(displayContextDef_t *dc, void *p);
qboolean Item_Bind_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key, qboolean down);
qboolean Item_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key, qboolean down);
qboolean Menu_HandleMouseMove(displayContextDef_t *dc, menuDef_t *menu, float x, float y);
qboolean Display_MouseMove(displayContextDef_t *dc, void *p, int x, int y);
void Menus_Open(displayContextDef_t *dc, menuDef_t *menu);
qboolean Menus_OpenByName(displayContextDef_t *dc, const char *p);
void Script_Open(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_OpenForGameType(displayContextDef_t *dc, itemDef_t *item, const char **args);
void Script_InGameOpen(displayContextDef_t *dc, itemDef_t *item, const char **args);
itemDef_t *Menu_SetNextCursorItem(displayContextDef_t *dc, menuDef_t *menu);
itemDef_t *Menu_SetPrevCursorItem(displayContextDef_t *dc, menuDef_t *menu);
qboolean Item_TextField_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key);
void Menu_HandleKey(displayContextDef_t *dc, menuDef_t *menu, int key, qboolean down);
void Item_Paint(displayContextDef_t *dc, itemDef_t *item);
void Menu_Paint(displayContextDef_t *dc, menuDef_t *menu, qboolean forcePaint);
void Menu_PaintAll(displayContextDef_t *dc);
void Menus_HandleOOBClick(displayContextDef_t *dc, menuDef_t *menu, int key, qboolean down);

void LerpColor(vec_t *a, vec_t *b, vec_t *c, float t)
{
    int i;
    for (i = 0; i < 4; i++) {
        float v = a[i] + (b[i] - a[i]) * t;
        float *cp = &c[i];
        *cp = v;
        if (0.0f > v)
            *(int *)cp = 0;
        else if (v > 1.0f)
            *(int *)cp = 0x3f800000;
    }
}

qboolean String_Parse(const char **p, char *out, int len)
{
    const char *token = Com_ParseOnLine(p);

    if (!*p)
        return 0;

    if (token[0] == '@') {
        const char *translated = SEH_StringEd_GetString(token + 1);
        if (translated) {
            I_strncpyz(out, translated, len);
            return 1;
        }
        if (!Dvar_GetBool("loc_warnings")) {

        } else if (Dvar_GetBool("loc_warningsAsErrors")) {
            Com_Error(6, "Could not translate menu string reference %s", token);

        } else {
            Com_Printf("^3WARNING: Could not translate menu string reference %s\n", token);

        }
    }

    I_strncpyz(out, token, len);
    return 1;
}

int Menu_ItemsMatchingGroup(menuDef_t *menu, const char *name)
{
    int itemCount = menu->itemCount;
    const char *wc = strchr(name, '*');
    int wildcard = wc ? (int)(wc - name) : -1;
    int count = 0;
    int i;

    for (i = 0; i < itemCount; i++) {
        itemDef_t *item = menu->items[i];
        const char *itemName = item->window.name;
        const char *itemGroup = item->window.group;

        if (wildcard != -1) {
            if (itemName && I_strncmp(itemName, name, wildcard) == 0) {
                count++;
                continue;
            }
            if (itemGroup && I_strncmp(itemGroup, name, wildcard) == 0) {
                count++;
                continue;
            }
        } else {
            if (itemName && I_stricmp(itemName, name) == 0) {
                count++;
                continue;
            }
            if (itemGroup && I_stricmp(itemGroup, name) == 0) {
                count++;
                continue;
            }
        }
    }
    return count;
}

itemDef_t *Menu_GetMatchingItemByNumber(menuDef_t *menu, int index, const char *name)
{
    int itemCount = menu->itemCount;
    const char *wc = strchr(name, '*');
    int wildcard = wc ? (int)(wc - name) : -1;
    int count = 0;
    int i;

    for (i = 0; i < itemCount; i++) {
        itemDef_t *item = menu->items[i];
        const char *itemName = item->window.name;
        const char *itemGroup = item->window.group;
        int matched = 0;

        if (wildcard != -1) {
            if ((itemName && I_strncmp(itemName, name, wildcard) == 0) ||
                (itemGroup && I_strncmp(itemGroup, name, wildcard) == 0))
                matched = 1;
        } else {
            if ((itemName && I_stricmp(itemName, name) == 0) ||
                (itemGroup && I_stricmp(itemGroup, name) == 0))
                matched = 1;
        }

        if (matched) {
            if (count == index)
                return item;
            count++;
        }
    }
    return NULL;
}

void Script_SetColor(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char name[0x400];
    float *out = NULL;
    int i;
    (void)dc;

    if (!String_Parse(args, name, 0x400))
        return;

    if (I_stricmp(name, "backcolor") == 0) {
        out = item->window.backColor;
        Window_AddDynamicFlags((void *)item, 0x8000);
    } else if (I_stricmp(name, "forecolor") == 0) {
        out = item->window.foreColor;
        Window_AddDynamicFlags((void *)item, 0x10000);
    } else if (I_stricmp(name, "bordercolor") == 0) {
        out = item->window.borderColor;
    }

    if (!out)
        return;

    for (i = 0; i < 4; i++) {
        float f;
        if (!Float_Parse(args, &f))
            return;
        out[i] = f;
    }
}

void Script_SetBackground(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    (void)dc;
    char name[0x400];
    if (String_Parse(args, name, 0x400)) {
        byte *it = (byte *)item;
        (*(MaterialHandle *)&((itemDef_t *)it)->window.background) = CL_RegisterMaterialNoMip(name, 3);
    }
}

void Menu_ShowItemByName(menuDef_t *menu, const char *p, qboolean bShow)
{
    int count = Menu_ItemsMatchingGroup(menu, p);
    int i;
    for (i = 0; i < count; i++) {
        byte *item = (byte *)Menu_GetMatchingItemByNumber(menu, i, p);
        if (!item)
            continue;
        if (bShow) {
            Window_AddDynamicFlags((void *)item, 4);
        } else {
            Window_RemoveDynamicFlags((void *)item, 4);
            int cinHandle = ((itemDef_t *)item)->window.cinematic;
            if (cinHandle >= 0) {
                CIN_StopCinematic(cinHandle);
                ((itemDef_t *)item)->window.cinematic = -1;
            }
        }
    }
}

void Menu_FadeItemByName(menuDef_t *menu, const char *p, qboolean fadeOut)
{
    int count = Menu_ItemsMatchingGroup(menu, p);
    int i;
    for (i = 0; i < count; i++) {
        byte *item = (byte *)Menu_GetMatchingItemByNumber(menu, i, p);
        if (!item)
            continue;
        if (fadeOut) {
            Window_AddDynamicFlags((void *)item, 0x14);
            Window_RemoveDynamicFlags((void *)item, 0x20);
        } else {
            Window_AddDynamicFlags((void *)item, 0x24);
            Window_RemoveDynamicFlags((void *)item, 0x10);
        }
    }
}

static inline __attribute__((always_inline))
menuDef_t *
Menus_FindByName_core(displayContextDef_t *dc, const char *p)
{
    int i;
    for (i = 0; i < dc->menuCount; i++) {
        if (I_stricmp(dc->Menus[i]->window.name, p) == 0)
            return dc->Menus[i];
    }
    return NULL;
}

menuDef_t *Menus_FindByName(displayContextDef_t *dc, const char *p)
{
    return Menus_FindByName_core(dc, p);
}

void Script_Show(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char name[1024];
    (void)dc;
    if (String_Parse(args, name, 1024)) {
        Menu_ShowItemByName(item->parent, name, 1);
    }
}

void Script_Hide(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char name[1024];
    (void)dc;
    if (String_Parse(args, name, 1024)) {
        Menu_ShowItemByName(item->parent, name, 0);
    }
}

void Script_FadeIn(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char name[1024];
    (void)dc;
    if (String_Parse(args, name, 1024)) {
        Menu_FadeItemByName(item->parent, name, 0);
    }
}

void Script_FadeOut(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char name[1024];
    (void)dc;
    if (String_Parse(args, name, 1024)) {
        Menu_FadeItemByName(item->parent, name, 1);
    }
}

extern void Dvar_SetFromStringByName(const char *name, const char *val);
void Script_SetDvar(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    (void)dc;
    (void)item;
    char dvarName[0x400], val[0x400];
    if (!String_Parse(args, dvarName, 0x400))
        return;
    if (!String_Parse(args, val, 0x400))
        return;
    Dvar_SetFromStringByName(dvarName, val);
}

extern void Cbuf_ExecuteText(int execWhen, const char *text);
void Script_ExecNow(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    (void)dc;
    (void)item;
    char val[0x400];
    if (String_Parse(args, val, 0x400)) {
        Cbuf_ExecuteText(0, va("%s\n", val));
    }
}

static void Script_ConditionalExecHandler(int execWhen, Bool (*shouldExec)(const char *, const char *), const char **args)
{
    Script_ConditionalExecHandler_impl(execWhen, NULL, args, shouldExec);
}

static void Script_ConditionalExecHandler_impl(int execWhen, itemDef_t *item, const char **args, Bool (*shouldExec)(const char *, const char *))
{
    char dvarName[1024];
    (void)item;
    char testValue[1024];
    char command[1024];

    if (!String_Parse(args, dvarName, 1024))
        return;
    if (!String_Parse(args, testValue, 1024))
        return;
    if (!String_Parse(args, command, 1024))
        return;

    {
        const char *dvarValue = Dvar_GetVariantString(dvarName);
        if (!shouldExec(dvarValue, testValue))
            return;
    }

    Cbuf_ExecuteText(execWhen, va("%s\n", command));
}

static Bool Script_ExecIfStringsEqual(const char *dvarValue, const char *testValue)
{
    return I_stricmp(dvarValue, testValue) == 0;
}

static Bool Script_ExecIfIntsEqual(const char *dvarValue, const char *testValue)
{
    return atoi(dvarValue) == atoi(testValue);
}

static Bool Script_ExecIfFloatsEqual(const char *dvarValue, const char *testValue)
{
    float diff = (float)(atof(dvarValue) - atof(testValue));
    if (diff < 0)
        diff = -diff;
    return diff < 9.999999747378752e-06f;
}

void Script_ExecOnDvarStringValue(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    (void)dc;
    (void)item;
    Script_ConditionalExecHandler_impl(2, item, args, Script_ExecIfStringsEqual);
}

void Script_ExecOnDvarIntValue(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    (void)dc;
    (void)item;
    Script_ConditionalExecHandler_impl(2, item, args, Script_ExecIfIntsEqual);
}

void Script_ExecOnDvarFloatValue(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    (void)dc;
    (void)item;
    Script_ConditionalExecHandler_impl(2, item, args, Script_ExecIfFloatsEqual);
}

void Script_ExecNowOnDvarStringValue(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    (void)dc;
    (void)item;
    Script_ConditionalExecHandler_impl(0, item, args, Script_ExecIfStringsEqual);
}

void Script_ExecNowOnDvarIntValue(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    (void)dc;
    (void)item;
    Script_ConditionalExecHandler_impl(0, item, args, Script_ExecIfIntsEqual);
}

void Script_ExecNowOnDvarFloatValue(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    (void)dc;
    (void)item;
    Script_ConditionalExecHandler_impl(0, item, args, Script_ExecIfFloatsEqual);
}

void Script_Play(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char val[1024];
    (void)dc;
    (void)item;
    if (String_Parse(args, val, 1024)) {
        UI_PlayLocalSoundAliasByName(val);
    }
}

void Script_ScriptMenuResponse(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char val[0x400];
    byte *it = (byte *)item;
    int iIndex;
    (void)dc;

    if (!(*(LegacyHacks **)imp_legacyHacks)->ui_scriptMenuAllowResponse)
        return;

    if (!String_Parse(args, val, 0x400))
        return;

    iIndex = -1;
    for (int i = 0; i < 0x20; i++) {
        const char *cs = CL_GetConfigString(0x4de + i);
        if (cs[0] == '\0')
            continue;
        const char *menuName = item->parent->window.name;
        if (I_stricmp(menuName, cs) == 0) {
            iIndex = i;
            break;
        }
    }

    if (getenv("MTRACE"))
        Com_Printf("[menu-trace] response menu='%s' index=%d value='%s' server=%d\n",
                   item->parent->window.name, iIndex, val, Dvar_GetInt("sv_serverId"));
    Cbuf_ExecuteText(2, va("cmd mr %i %i %s\n", Dvar_GetInt("sv_serverId"), iIndex, val));
}

void Item_RunScript(displayContextDef_t *dc, itemDef_t *item, const char *s)
{
    char script[0x400];
    char command[0x400];
    const char *p;
    int i;

    memset(script, 0, 0x400);
    if (!item || !s || s[0] == '\0')
        return;

    I_strncat(script, 0x400, s);
    p = script;

    while (String_Parse(&p, command, 0x400)) {
        if (command[0] == ';' && command[1] == '\0')
            continue;

        {
            const commandDef_t *cmd = commandList;
            for (i = 0; i < scriptCommandCount; i++, cmd++) {
                if (I_stricmp(command, cmd->name) == 0) {
                    commandList[i].handler(dc, item, &p);
                    goto next_command;
                }
            }
        }
        UI_RunMenuScript(&p);
    next_command:;
    }
}

static inline __attribute__((always_inline)) void Controls_GetConfig_core(void)
{
    char b[0x100];
    const char *command;
    byte *entry = (byte *)g_bindings;

    do {
        int twokeys[2];
        int count = 0;
        int j;

        command = *(const char **)entry;
        twokeys[1] = -1;
        twokeys[0] = -1;

        for (j = 0; j < 0x100; j++) {
            Key_GetBindingBuf(j, b, 0x100);
            if (b[0] == '\0')
                continue;
            if (!command || I_stricmp(b, command) != 0)   /* command is NULL on x64 (see g_bindings) */
                continue;
            twokeys[count] = j;
            count++;
            if (count == 2)
                break;
        }

        (((bind_t*)(entry))->bind1) = twokeys[0];
        (((bind_t*)(entry))->bind2) = twokeys[1];
        entry += sizeof(bind_t);   /* 0x14 was x86 sizeof(bind_t); x64 is 24 */

    } while (entry != (byte *)g_bindings + 56 * sizeof(bind_t));
}

void Controls_GetConfig(void)
{
    Controls_GetConfig_core();
}

extern void Key_SetBinding(int key, const char *command);
extern byte szShotName[];
void Controls_SetConfig(qboolean restart)
{
    (void)restart;

    int i;   /* was byte-stride 0x14 / +12 offsets (x86 bind_t layout) */
    for (i = 0; i < 56; i++) {
        int key1 = g_bindings[i].bind1;
        if (key1 != -1 && g_bindings[i].command) {
            Key_SetBinding(key1, g_bindings[i].command);
            int key2 = g_bindings[i].bind2;
            if (key2 != -1)
                Key_SetBinding(key2, g_bindings[i].command);
        }
    }
    Cbuf_ExecuteText(2, "bindingsave\n");
}

void Controls_SetDefaults(void)
{
    int i;   /* was byte-stride 0x14 (x86 sizeof(bind_t)) */
    for (i = 0; i < 56; i++) {
        g_bindings[i].bind1 = g_bindings[i].defaultbind1;
        g_bindings[i].bind2 = g_bindings[i].defaultbind2;
    }
}

void BindingFromName(const char *dvar, char *nameBind)
{
    int i;
    for (i = 0; i < 0x38; i++) {
        if (!g_bindings[i].command || I_stricmp(dvar, g_bindings[i].command) != 0)
            continue;

        int key1 = g_bindings[i].bind1;
        if (key1 == -1)
            break;

        Key_KeynumToStringBuf(key1, nameBind, 0x20);
        I_strncpyz(nameBind, UI_SafeTranslateString(nameBind), 0x80);

        int key2 = g_bindings[i].bind2;
        if (key2 == -1)
            return;

        char nameBind2[0x80];
        Key_KeynumToStringBuf(key2, nameBind2, 0x20);
        I_strncpyz(nameBind2, UI_SafeTranslateString(nameBind2), 0x80);
        I_strncat(nameBind, 0x80, va(" %s ", UI_SafeTranslateString("KEY_OR")));
        I_strncat(nameBind, 0x80, nameBind2);
        return;
    }

    I_strncpyz(nameBind, UI_SafeTranslateString("KEY_UNBOUND"), 0x80);
}

qboolean GetCommandHasBinding(const char *command)
{

    int i;
    for (i = 0; i < 56; i++) {
        if (g_bindings[i].command && I_stricmp(command, g_bindings[i].command) == 0) {
            if (g_bindings[i].bind1 != -1)
                return 1;
            return 0;
        }
    }
    return 0;
}

int GetKeyBindings(const char *command, char (*bindings)[128])
{
    int i;
    bindings[0][0] = '\0';
    bindings[1][0] = '\0';

    for (i = 0; i < 0x38; i++) {
        if (!g_bindings[i].command || I_stricmp(command, g_bindings[i].command) != 0)
            continue;

        int key1 = g_bindings[i].bind1;
        if (key1 == -1)
            break;

        Key_KeynumToStringBuf(key1, bindings[0], 0x80);

        int key2 = g_bindings[i].bind2;
        if (key2 == -1)
            return 1;

        Key_KeynumToStringBuf(key2, bindings[1], 0x80);
        return 2;
    }

    *(int *)&bindings[0][0] = 0x5f59454b;
    *(int *)&bindings[0][4] = 0x4f424e55;
    *(int *)&bindings[0][8] = 0x00444e55;
    return 0;
}

int GetKeyBindingLocalizedString(const char *command, char *keys)
{
    char bindings[2][128];
    int bindCount = GetKeyBindings(command, bindings);

    if (bindCount == 0) {
        I_strncpyz(keys, UI_SafeTranslateString("KEY_UNBOUND"), 0x100);
        return bindCount;
    }

    const char *translated = SEH_StringEd_GetString(bindings[0]);
    if (translated)
        I_strncpyz(keys, translated, 0x100);
    else
        I_strncpyz(keys, bindings[0], 0x100);

    if (bindCount > 1) {
        I_strncat(keys, 0x100, va(" %s ", UI_SafeTranslateString("KEY_OR")));
        translated = SEH_StringEd_GetString(bindings[1]);
        if (translated)
            I_strncat(keys, 0x100, translated);
        else
            I_strncat(keys, 0x100, bindings[1]);
    }

    return bindCount;
}

qboolean Display_KeyBindPending(void)
{
    return g_waitingForKey;
}

int Menu_Count(displayContextDef_t *dc)
{
    return dc->menuCount;
}

#ifdef __EMSCRIPTEN__
/* Keep the retail menu widgets and layout, exposing only settings implemented
 * by the browser room service. Original files stay intact. */
static void UI_ConfigureBrowserMenu(menuDef_t *menu)
{
    const char *name = menu->window.name;
    int count = 0;
    if (!name) return;
    for (int i = 0; i < menu->itemCount; ++i) {
        itemDef_t *item = menu->items[i];
        const char *text = item->text ? item->text : "";
        const char *dvar = item->dvar ? item->dvar : "";
        if (!I_stricmp(name, "main_text") && !I_stricmp(text, "@MENU_SINGLE_PLAYER"))
            continue;
        if (!I_stricmp(name, "createserver_buttons") &&
            I_stricmp(text, "@MENU_BACK") && I_stricmp(text, "@MENU_START"))
            continue;
        if (!I_stricmp(name, "createserver_serversettings")) {
            if (!I_stricmp(dvar, "sv_maxclients")) {
                item->type = 0;
                item->dvar = NULL;
                item->text = "64";
                item->action = NULL;
            } else if (I_stricmp(dvar, "sv_hostname") &&
                       I_stricmp(dvar, "ui_botCount") && I_stricmp(dvar, "ui_botDifficulty") &&
                       I_stricmp(text, "Bots") && I_stricmp(text, "Bot Difficulty") &&
                       !item->window.ownerDraw &&
                       I_stricmp(text, "@MENU_SERVER_SETTINGS") &&
                       I_stricmp(text, "@MENU_GAME_TYPE") &&
                       I_stricmp(text, "@MENU_SERVER_NAME") &&
                       I_stricmp(text, "@MENU_MAXIMUM_PLAYERS") &&
                       !(item->type == 0 && !I_stricmp(text, "64"))) {
                continue;
            }
        }
        menu->items[count++] = item;
    }
    menu->itemCount = count;
}
#endif

void UI_AddMenuList(displayContextDef_t *dc, MenuList *menuList)
{
    int i;

    /* was raw x86 offsets: menus ptr @+4 read as 4-byte (truncated), Menus[] stride *4 */
    if (!menuList)
        return;
    if (menuList->menuCount <= 0)
        return;

    for (i = 0; i < menuList->menuCount; i++) {
#ifdef __EMSCRIPTEN__
        UI_ConfigureBrowserMenu(menuList->menus[i]);
#endif
        if (dc->menuCount > 0x7f)
            Com_Error(1, "UI_AddMenu: Maximum number of menus %d exceeded.", 0x80);
        dc->Menus[dc->menuCount] = menuList->menus[i];
        dc->menuCount += 1;
    }
}

void Item_MouseLeave(displayContextDef_t *dc, itemDef_t *item)
{
    byte *it = (byte *)item;
    if (!item)
        return;
    if ((*(byte *)&((itemDef_t *)it)->window.dynamicFlags[0]) & 0x40) {

        Item_RunScript(dc, item, ((itemDef_t *)it)->mouseExitText);
        Window_RemoveDynamicFlags((void *)item, 0x40);
    }

    Item_RunScript(dc, item, ((itemDef_t *)it)->mouseExit);
    Window_RemoveDynamicFlags((void *)item, 0x300);
}

float Item_Slider_ThumbPosition(itemDef_t *item)
{
    byte *it = (byte *)item;
    editFieldDef_t *editDef = Item_GetEditFieldDef(item);
    float baseX, value, minVal, maxVal;

    if (!editDef)
        return 0.0f;

    if ((*(int *)&((itemDef_t *)it)->text))
        baseX = (((itemDef_s*)(it))->textRect[0].x) + (((itemDef_s*)(it))->textRect[0].w) + 8.0f;
    else
        baseX = (((rectDef_t*)(it))->x);

    value = Dvar_GetFloat(item->dvar);
    minVal = (((editFieldDef_s*)(editDef))->minVal);
    maxVal = *((float *)editDef + 1);

    if (value < minVal)
        value = minVal;
    else if (value > maxVal)
        value = maxVal;

    return ((value - minVal) / (maxVal - minVal)) * 84.0f + 5.0f + 1.0f + baseX;
}

qboolean Menu_CheckOnKey(displayContextDef_t *dc, menuDef_t *menu, int key)
{
    byte *m = (byte *)menu;
    byte tempItem[0x2a0];
    int i;

    byte *node = (byte *)menu->onKey;
    while (node) {
        if (*(int *)node == key) {
            *(void **)&(*(unsigned char *)&((itemDef_t *)tempItem)->parent) = menu;
            Item_RunScript(dc, (itemDef_t *)tempItem, *(const char **)(node + 4));
            return 1;
        }
        node = *(byte **)(node + 8);
    }

    int itemCount = (((menuDef_t*)(m))->itemCount);
    for (i = 0; i < itemCount; i++) {
        byte *item = *(byte **)((char *)&((menuDef_t *)m)->items[i]);

        if (!((*(byte *)&((itemDef_t *)item)->window.dynamicFlags[0]) & 4))
            continue;

        if ((*(byte *)&((itemDef_t *)item)->dvarFlags) & 0xc) {
            if (!Item_EnableShowViaDvar((itemDef_t *)item, 4))
                continue;
        }

        node = (*(byte **)&((itemDef_t *)item)->onKey);
        while (node) {
            if (*(int *)node == key) {
                *(void **)&(*(unsigned char *)&((itemDef_t *)tempItem)->parent) = menu;
                Item_RunScript(dc, (itemDef_t *)tempItem, *(const char **)(node + 4));
                return 1;
            }
            node = *(byte **)(node + 8);
        }
    }

    return 0;
}

qboolean Menus_AnyFullScreenVisible(displayContextDef_t *dc)
{
    byte *d = (byte *)dc;
    int count = (((displayContextDef_s*)(d))->openMenuCount);
    int i;
    for (i = count - 1; i >= 0; i--) {
        byte *menu = *(byte **)((char *)&((displayContextDef_s *)d)->menuStack[i]);
        if (((*(byte *)&((menuDef_t *)menu)->window.dynamicFlags[0]) & 4) && (((menuDef_t*)(menu))->fullScreen))
            return 1;
    }
    return 0;
}

menuDef_t *Menu_GetFocused(displayContextDef_t *dc)
{
    int count = dc->openMenuCount;
    int i;
    for (i = count - 1; i >= 0; i--) {
        menuDef_t *menu = dc->menuStack[i];
        int flags = menu->window.dynamicFlags[0];
        if ((flags & 4) && (flags & 2))
            return menu;
    }
    return NULL;
}

int Item_ListBox_MaxScroll(itemDef_t *item)
{
    byte *it = (byte *)item;
    byte *listPtr = (byte *)Item_GetListBoxDef(item);
    int count = UI_FeederCount(((itemDef_t *)it)->special);
    if (!listPtr)
        return 0;
    int visibleRows;
    if ((((Window *)it)->staticFlags & 0x200000)) {

        visibleRows = (int)((((rectDef_t*)(it))->w) / (((listBoxDef_s*)(listPtr))->elementWidth));
    } else {

        visibleRows = (int)((((rectDef_t*)(it))->h) / (((listBoxDef_s*)(listPtr))->elementHeight));
    }
    int maxScroll = count - visibleRows + 1;
    if (maxScroll < 0)
        return 0;
    return maxScroll;
}

int Item_ListBox_ThumbPosition(itemDef_t *item)
{
    byte *it = (byte *)item;
    byte *listPtr = (byte *)Item_GetListBoxDef(item);
    int maxScroll;
    float scrollArea, ratio;

    if (!listPtr)
        return 0;

    maxScroll = Item_ListBox_MaxScroll(item);

    if ((((Window *)it)->staticFlags & 0x200000)) {

        scrollArea = (((rectDef_t*)(it))->w) - 32.0f - 2.0f;
        ratio = (maxScroll > 0) ? (scrollArea - 16.0f) / (float)maxScroll : 0.0f;
        return (int)(ratio * (float)*(int *)listPtr + (1.0f + (((rectDef_t*)(it))->x) + 16.0f));
    } else {

        scrollArea = (((rectDef_t*)(it))->h) - 32.0f - 2.0f;
        ratio = (maxScroll > 0) ? (scrollArea - 16.0f) / (float)maxScroll : 0.0f;
        return (int)(ratio * (float)*(int *)listPtr + (1.0f + (((rectDef_t*)(it))->y) + 16.0f));
    }
}

int Item_ListBox_ThumbDrawPosition(displayContextDef_t *dc, itemDef_t *item)
{
    byte *it = (byte *)item;
    byte *d = (byte *)dc;

    if ((itemDef_t *)item != itemCapture)
        return Item_ListBox_ThumbPosition(item);

    if ((((Window *)it)->staticFlags & 0x200000)) {

        int max = (int)((((rectDef_t*)(it))->x) + (((rectDef_t*)(it))->w) - 32.0f - 1.0f);
        int cursor = ((displayContextDef_t *)d)->cursorx;
        int minPos = (int)((((rectDef_t*)(it))->x) + 16.0f + 1.0f) + 8;
        if (cursor < minPos || cursor > max + 8)
            return Item_ListBox_ThumbPosition((itemDef_t *)itemCapture);
        return cursor - 8;
    } else {

        int max = (int)((((rectDef_t*)(it))->y) + (((rectDef_t*)(it))->h) - 32.0f - 1.0f);
        int cursor = (((displayContextDef_s*)(d))->cursory);
        int minPos = (int)((((rectDef_t*)(it))->y) + 16.0f + 1.0f) + 8;
        if (cursor < minPos || cursor > max + 8)
            return Item_ListBox_ThumbPosition((itemDef_t *)itemCapture);
        return cursor - 8;
    }
}

static void Scroll_Slider_SetThumbPos_impl(byte *dc, byte *item)
{
    byte *editDef;
    float rightEdge;
    float cursorx;
    float usableStart, usableWidth;
    float yIgnored, hIgnored;
    float thumbPos, thumbFrac;
    float minVal, maxVal;

    editDef = (byte *)Item_GetEditFieldDef((itemDef_t *)item);
    if (!editDef)
        return;

    if ((*(int *)&((itemDef_t *)item)->text))
        rightEdge = (((itemDef_s*)(item))->textRect[0].x) + (((itemDef_s*)(item))->textRect[0].w) + 8.0f;
    else
        rightEdge = (((rectDef_t*)(item))->x);

    cursorx = (float)*(int *)(dc + 0xc);
    CalcScreenX(&cursorx, 4);

    usableStart = rightEdge + 5.0f + 1.0f;
    usableWidth = 84.0f;
    yIgnored = 0.0f;
    hIgnored = 0.0f;

    CalcScreenPlacement(&usableStart, &yIgnored, &usableWidth, &hIgnored,
                        (((rectDef_t*)(item))->horzAlign), (((rectDef_t*)(item))->vertAlign));

    thumbPos = cursorx - usableStart;
    if (thumbPos < 0.0f)
        thumbPos = 0.0f;
    else if (thumbPos > usableWidth)
        thumbPos = usableWidth;

    thumbFrac = thumbPos / usableWidth;
    minVal = (((editFieldDef_s*)(editDef))->minVal);
    maxVal = (((editFieldDef_s*)(editDef))->maxVal);
    Dvar_SetFromStringByName(((itemDef_t *)item)->dvar,
                             va("%g", (double)(minVal + (maxVal - minVal) * thumbFrac)));
}

static void Scroll_Slider_ThumbFunc(displayContextDef_t *dc, void *p)
{
    byte *scroll = (byte *)p;
    Scroll_Slider_SetThumbPos_impl((byte *)dc, *(byte **)(scroll + 0x18));
}

void Menus_Close(displayContextDef_t *dc, menuDef_t *menu)
{
    byte *d = (byte *)dc;
    int openCount, i, menuIndex;
    qboolean wasVisible = 0;
    byte tempItem[0x2a0];

    openCount = (((displayContextDef_s*)(d))->openMenuCount);
    for (i = openCount - 1; i >= 0; i--) {
        if (*(void **)((char *)&((displayContextDef_s *)d)->menuStack[i]) == menu) {
            int flags = menu->window.dynamicFlags[0];
            if (flags & 4)
                wasVisible = (flags >> 1) & 1;
            break;
        }
    }

    int flags = menu->window.dynamicFlags[0];
    if ((flags & 4) && (*(void **)&((menuDef_t *)menu)->onClose)) {
        *(void **)&(*(unsigned char *)&((itemDef_t *)tempItem)->parent) = menu;
        Item_RunScript(dc, (itemDef_t *)tempItem, ((menuDef_t *)menu)->onClose);
        openCount = (((displayContextDef_s*)(d))->openMenuCount);
    }

    menuIndex = -1;
    for (i = openCount - 1; i >= 0; i--) {
        if (*(void **)((char *)&((displayContextDef_s *)d)->menuStack[i]) == menu) {
            menuIndex = i;
            break;
        }
    }
    if (menuIndex >= 0) {
        (((displayContextDef_s*)(d))->openMenuCount) = openCount - 1;

        for (i = menuIndex; i < (((displayContextDef_s*)(d))->openMenuCount); i++) {
            *(void **)((char *)&((displayContextDef_s *)d)->menuStack[i]) = *(void **)((char *)&((displayContextDef_s *)d)->menuStack[(i + 1)]);
        }
    }

    if (wasVisible && (((displayContextDef_s*)(d))->openMenuCount) > 0) {
        int menuNum = (((displayContextDef_s*)(d))->openMenuCount) - 1;
        for (i = menuNum; i >= 0; i--) {
            byte *m = *(byte **)((char *)&((displayContextDef_s *)d)->menuStack[i]);
            if ((*(byte *)&((menuDef_t *)m)->window.dynamicFlags[0]) & 4) {
                Window_AddDynamicFlags((void *)m, 2);
                break;
            }
        }
    }

    Window_RemoveDynamicFlags((void *)menu, 6);
}

void Menus_CloseAll(displayContextDef_t *dc)
{
    byte *d = (byte *)dc;

    while ((((displayContextDef_s*)(d))->openMenuCount) > 0) {
        int top = (((displayContextDef_s*)(d))->openMenuCount) - 1;
        Menus_Close(dc, *(menuDef_t **)((char *)&((displayContextDef_s *)d)->menuStack[top]));
    }
    /* A menu may be initially visible in its asset without having been pushed
     * onto the open stack (the retail profile selector is one example). */
    for (int i = 0; i < dc->menuCount; ++i)
        Window_RemoveDynamicFlags(dc->Menus[i], 6);
}

void Item_SetTextExtents(itemDef_t *item, int *width, int *height, const char *text)
{
    byte *it = (byte *)item;
    const char *textPtr = text;
    int originalWidth;

    if (!textPtr) {
        textPtr = ((itemDef_t *)it)->text;
        if (!textPtr)
            return;
        if (textPtr[0] == '@')
            textPtr = UI_SafeTranslateString(textPtr + 1);
    }

    *width = (int)(((itemDef_s*)(it))->textRect[0].w);
    *height = (int)((itemDef_t *)it)->textRect[0].h;

    if (*width != 0) {
        int itemType = (((itemDef_s*)(it))->type);
        if (itemType != 8 && !(*(void **)&((itemDef_t *)it)->dvar))
            goto check_alignment;
        if ((((itemDef_s*)(it))->textalignment) == 1)
            goto compute;
    check_alignment:
        if ((((itemDef_s*)(it))->textalignment) == 3)
            goto compute;
        return;
    }

compute:;
    FontHandle font = UI_GetFontHandle(((itemDef_t *)it)->fontEnum, ((itemDef_t *)it)->textscale);
    float scale = ((itemDef_t *)it)->textscale;
    originalWidth = UI_TextWidth(textPtr, 0, font, scale);

    int itemType = (((itemDef_s*)(it))->type);
    if (itemType == 8) {
        int alignment = (((itemDef_s*)(it))->textalignment);
        if (alignment == 1 || alignment == 2)
            originalWidth += UI_OwnerDrawWidth(((itemDef_t *)it)->window.ownerDraw, font, scale);
    } else if (itemType <= 0x12 && ((1 << itemType) & 0x70210)) {
        if ((((itemDef_s*)(it))->textalignment) == 1 && (*(void **)&((itemDef_t *)it)->dvar)) {
            originalWidth += UI_TextWidth(Dvar_GetVariantString(item->dvar), 0, font, scale);
        }
    }

    if ((((itemDef_s*)(it))->textalignment) == 3)
        originalWidth += UI_TextWidth(text, 0, font, scale);

    *width = UI_TextWidth(textPtr, 0, font, scale);
    *height = UI_TextHeight(font, scale);

    float textW = (float)*width;
    float textH = (float)*height;
    float textX = (((itemDef_s*)(it))->textalignx);
    float textY = (((itemDef_s*)(it))->textaligny);
    int horzAlign = (((rectDef_t*)(it))->horzAlign);
    int vertAlign = (((rectDef_t*)(it))->vertAlign);

    int alignment = (((itemDef_s*)(it))->textalignment);
    if (alignment == 2) {

        textX -= (float)originalWidth;
    } else if (alignment == 1 || alignment == 3) {

        textX -= (float)(originalWidth / 2);
    }

    if (((itemDef_t *)it)->window.border) {
        textX += ((itemDef_t *)it)->window.borderSize;
        textY += ((itemDef_t *)it)->window.borderSize;
    }

    textX += (((rectDef_t*)(it))->x);
    textY += (((rectDef_t*)(it))->y);

    float newRect[6];
    newRect[0] = textX;
    newRect[1] = textY;
    newRect[2] = textW;
    newRect[3] = textH;
    *(int *)&newRect[4] = horzAlign;
    *(int *)&newRect[5] = vertAlign;
    Item_SetTextRect(item, (const rectDef_t *)newRect);
}

void Item_Text_AutoWrapped_Paint(itemDef_t *item, const char *textPtr, vec_t *color)
{
    byte *it = (byte *)item;
    char buff[0x400];
    FontHandle font = UI_GetFontHandle(((itemDef_t *)it)->fontEnum, ((itemDef_t *)it)->textscale);
    float scale = ((itemDef_t *)it)->textscale;
    int height = UI_TextHeight(font, scale);
    int textWidth = UI_TextWidth(textPtr, 0, font, scale);
    int style = (((itemDef_s*)(it))->textStyle);
    int horzAlign = (((rectDef_t*)(it))->horzAlign);
    int vertAlign = (((rectDef_t*)(it))->vertAlign);
    byte *textRect = (byte *)((itemDef_t *)it)->textRect;
    float lineHeight;
    int iTargetLineWidth;
    float y;
    int len, newLine, newLineWidth;
    const char *newLinePtr;
    const char *p;

    float itemW = (((rectDef_t*)(it))->w);
    if (itemW != 0.0f && (float)textWidth > itemW) {

        int numLines = (int)ceilf((float)textWidth / itemW);
        iTargetLineWidth = (int)((float)textWidth / (float)numLines);
    } else {
        iTargetLineWidth = (int)itemW;
    }

    y = (((itemDef_s*)(it))->textaligny);
    buff[0] = '\0';
    lineHeight = (float)(height + 5);

    if (!textPtr)
        return;

    len = 0;
    newLine = 0;
    newLineWidth = 0;
    newLinePtr = NULL;
    p = textPtr;

    while (p) {
        unsigned char ch = *p;

        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\0') {
            newLinePtr = p + 1;
            newLine = len;
            newLineWidth = textWidth;
        }

        textWidth = UI_TextWidth(buff, 0, font, scale);

        int shouldBreak = 0;
        if (newLine) {
            if ((float)textWidth > (((rectDef_t*)(it))->w))
                shouldBreak = 1;
        }
        if (ch == '\n' || ch == '\0')
            shouldBreak = 1;

        if (ch == ' ' && textWidth >= iTargetLineWidth)
            shouldBreak = 1;

        if (shouldBreak) {
            if (len > 0) {

                float newTextRect[6];
                newTextRect[0] = (((rectDef_t*)(textRect))->x);
                newTextRect[1] = (((rectDef_t*)(textRect))->y);
                newTextRect[2] = (((rectDef_t*)(textRect))->w);
                newTextRect[3] = (((rectDef_t*)(textRect))->h);
                *(int *)&newTextRect[4] = (((rectDef_t*)(textRect))->horzAlign);
                *(int *)&newTextRect[5] = (((rectDef_t*)(textRect))->vertAlign);

                int alignment = (((itemDef_s*)(it))->textalignment);
                if (alignment == 1) {
                    newTextRect[0] = (((itemDef_s*)(it))->textalignx) - (float)(newLineWidth / 2);
                } else if (alignment == 2) {
                    newTextRect[0] = (((itemDef_s*)(it))->textalignx) + (((rectDef_t*)(textRect))->w) - (float)newLineWidth;
                } else {
                    newTextRect[0] = (((itemDef_s*)(it))->textalignx);
                }
                newTextRect[1] = y;

                if (((itemDef_t *)it)->window.border) {
                    newTextRect[0] += ((itemDef_t *)it)->window.borderSize;
                    newTextRect[1] += ((itemDef_t *)it)->window.borderSize;
                }
                newTextRect[0] += (((rectDef_t*)(it))->x);
                newTextRect[1] += (((rectDef_t*)(it))->y);

                Item_SetTextRect(item, (const rectDef_t *)newTextRect);

                buff[newLine] = '\0';
                UI_DrawText(buff, 0x7fffffff, font,
                            (((rectDef_t*)(textRect))->x), (((rectDef_t*)(textRect))->y),
                            horzAlign, vertAlign, scale, color, style);
            }

            if (*p == '\0')
                break;

            y += lineHeight;
            p = newLinePtr;
            len = 0;
            newLine = 0;
            newLineWidth = 0;
            if (!p)
                break;
            continue;
        }

        buff[len] = ch;
        len++;
        p++;

        if (len > 0 && buff[len - 1] == '\r')
            buff[len - 1] = ' ';
        buff[len] = '\0';
    }
}

void Item_Text_Wrapped_Paint(itemDef_t *item, const char *textPtr, vec_t *color)
{
    byte *it = (byte *)item;
    char buff[0x400];
    FontHandle font = UI_GetFontHandle(((itemDef_t *)it)->fontEnum, ((itemDef_t *)it)->textscale);
    int height = UI_TextHeight(font, ((itemDef_t *)it)->textscale);
    float scale = ((itemDef_t *)it)->textscale;
    int style = (((itemDef_s*)(it))->textStyle);
    int horzAlign = (((rectDef_t*)(it))->horzAlign);
    int vertAlign = (((rectDef_t*)(it))->vertAlign);
    byte *textRect = (byte *)((itemDef_t *)it)->textRect;
    float y = (((rectDef_t*)(textRect))->y);
    float lineHeight = (float)(height + 5);

    const char *p = strchr(textPtr, 0x0d);
    if (!p || p[0] == '\0') {

        float x;
        int alignment = (((itemDef_s*)(it))->textalignment);
        if (alignment == 1) {
            int tw = UI_TextWidth(textPtr, 0, font, scale);
            x = (((rectDef_t*)(textRect))->x) + ((((rectDef_t*)(textRect))->w) - (float)tw) * 0.5f;
        } else if (alignment == 2) {
            int tw = UI_TextWidth(textPtr, 0, font, scale);
            x = (((rectDef_t*)(textRect))->x) + (((rectDef_t*)(textRect))->w) - (float)tw;
        } else {
            x = (((rectDef_t*)(textRect))->x);
        }
        UI_DrawText(textPtr, 0x7fffffff, font, x, y, horzAlign, vertAlign, scale, color, style);
        return;
    }

    const char *lineStart = textPtr;
    while (p && p[0] != '\0') {
        int lineLen = (int)(p - lineStart);
        strncpy(buff, lineStart, lineLen + 1);
        buff[lineLen] = '\0';

        float x;
        int alignment = (((itemDef_s*)(it))->textalignment);
        if (alignment == 1) {
            int tw = UI_TextWidth(buff, 0, font, scale);
            x = (((rectDef_t*)(textRect))->x) + ((((rectDef_t*)(textRect))->w) - (float)tw) * 0.5f;
        } else if (alignment == 2) {
            int tw = UI_TextWidth(buff, 0, font, scale);
            x = (((rectDef_t*)(textRect))->x) + (((rectDef_t*)(textRect))->w) - (float)tw;
        } else {
            x = (((rectDef_t*)(textRect))->x);
        }

        UI_DrawText(buff, 0x7fffffff, font, x, y, horzAlign, vertAlign, scale, color, style);
        y += lineHeight;
        lineStart = p + 1 + lineLen + 1 - (lineLen + 1);
        lineStart = p + 1;

        p = strchr(lineStart, 0x0d);
        if (!p || p[0] == '\0')
            break;
    }

    float x;
    int alignment = (((itemDef_s*)(it))->textalignment);
    if (alignment == 1) {
        int tw = UI_TextWidth(lineStart, 0, font, scale);
        x = (((rectDef_t*)(textRect))->x) + ((((rectDef_t*)(textRect))->w) - (float)tw) * 0.5f;
    } else if (alignment == 2) {
        int tw = UI_TextWidth(lineStart, 0, font, scale);
        x = (((rectDef_t*)(textRect))->x) + (((rectDef_t*)(textRect))->w) - (float)tw;
    } else {
        x = (((rectDef_t*)(textRect))->x);
    }
    UI_DrawText(lineStart, 0x7fffffff, font, x, y, horzAlign, vertAlign, scale, color, style);
}

void Item_ListBox_Paint(displayContextDef_t *dc, itemDef_t *item)
{
    byte *it = (byte *)item;
    byte *listPtr;
    int count;
    float sizeH;
    int startPos;
    int i, j;
    int numColumns;
    MaterialHandle optionalImage;
    const char *text;
    float x, y;
    byte *uiInfo;
    int horzAlign, vertAlign;

    listPtr = (byte *)Item_GetListBoxDef(item);
    if (!listPtr)
        return;

    count = UI_FeederCount(((itemDef_t *)it)->special);

    horzAlign = (((rectDef_t*)(it))->horzAlign);
    vertAlign = (((rectDef_t*)(it))->vertAlign);

    if ((((Window *)it)->staticFlags & 0x200000)) {

        float hx = (((rectDef_t*)(it))->x) + 1.0f;
        float hy_bottom = (((rectDef_t*)(it))->y);
        hy_bottom = hy_bottom + (((rectDef_t*)(it))->h) - 16.0f - 1.0f;

        uiInfo = (byte *)imp_sharedUiInfo;

        UI_DrawHandlePic(
            hx, hy_bottom, 16.0f, 16.0f,
            horzAlign, vertAlign, 0, ((sharedUiInfo_t*)uiInfo)->assets.scrollBarArrowLeft);

        float trackX = hx + 15.0f;

        float trackW = (((rectDef_t*)(it))->w) - 32.0f;
        UI_DrawHandlePic(
            trackX, hy_bottom, trackW + 1.0f, 16.0f,
            horzAlign, vertAlign, 0, ((sharedUiInfo_t*)uiInfo)->assets.scrollBar);

        float rightArrowX = trackW - 1.0f + trackX;
        UI_DrawHandlePic(
            rightArrowX, hy_bottom, 16.0f, 16.0f,
            horzAlign, vertAlign, 0, ((sharedUiInfo_t *)uiInfo)->assets.scrollBarArrowRight);

        int thumbPos = Item_ListBox_ThumbDrawPosition(dc, item);
        float thumbX = (float)thumbPos;
        float thumbMax = rightArrowX - 16.0f - 1.0f;
        if (thumbMax < thumbX)
            thumbX = thumbMax;
        UI_DrawHandlePic(
            thumbX, hy_bottom, 16.0f, 16.0f,
            horzAlign, vertAlign, 0, ((sharedUiInfo_t*)uiInfo)->assets.scrollBarThumb);

        ListBox_SetEndPos((void *)listPtr, *(int *)listPtr);

        float size = (((rectDef_t*)(it))->w) - 2.0f;

        if (((listBoxDef_t *)listPtr)->elementStyle != 1)
            return;

        x = (((rectDef_t*)(it))->x) + 1.0f;
        y = (((rectDef_t*)(it))->y) + 1.0f;
        startPos = *(int *)listPtr;
        i = startPos;

        while (i < count) {

            MaterialHandle img = UI_FeederItemImage(((itemDef_t *)it)->special, i);
            if (img) {
                UI_DrawHandlePic(
                    x + 1.0f, y + 1.0f,
                    (((listBoxDef_s*)(listPtr))->elementWidth) - 2.0f,
                    (((listBoxDef_s*)(listPtr))->elementHeight) - 2.0f,
                    horzAlign, vertAlign, 0, img);
            }

            if (i == ((itemDef_t *)it)->cursorPos[0]) {
                UI_DrawRect(x, y,
                            (((listBoxDef_s*)(listPtr))->elementWidth) - 1.0f,
                            (((listBoxDef_s*)(listPtr))->elementHeight) - 1.0f,
                            horzAlign, vertAlign,
                            ((itemDef_t *)it)->window.borderSize,
                            ((itemDef_t *)it)->window.borderColor);
            }

            size -= (((listBoxDef_s*)(listPtr))->elementWidth);
            if ((((listBoxDef_s*)(listPtr))->elementWidth) > size) {
                (((listBoxDef_s*)(listPtr))->drawPadding) = (int)size;
                return;
            }

            x += (((listBoxDef_s*)(listPtr))->elementWidth);
            ListBox_SetEndPos((void *)listPtr, (((listBoxDef_s*)(listPtr))->endPos[0]) + 1);
            i++;
        }
        return;
    }

    UI_OverrideCursorPos(item);

    numColumns = ((listBoxDef_t *)listPtr)->noScrollBars;

    if (numColumns == 0) {

        float scrollX = (((rectDef_t*)(it))->x);
        scrollX = scrollX + (((rectDef_t*)(it))->w) - 16.0f - 1.0f;

        float scrollY = (((rectDef_t*)(it))->y) + 1.0f;

        uiInfo = (byte *)imp_sharedUiInfo;

        UI_DrawHandlePic(
            scrollX, scrollY, 16.0f, 16.0f,
            horzAlign, vertAlign, 0, *(MaterialHandle *)uiInfo);

        float trackY = scrollY + 15.0f;

        ListBox_SetEndPos((void *)listPtr, *(int *)listPtr);

        float trackH = (((rectDef_t*)(it))->h) - 32.0f;
        UI_DrawHandlePic(
            scrollX, trackY, 16.0f, trackH + 1.0f,
            horzAlign, vertAlign, 0, ((sharedUiInfo_t*)uiInfo)->assets.scrollBar);

        float downArrowY = trackH - 1.0f + trackY;
        UI_DrawHandlePic(
            scrollX, downArrowY, 16.0f, 16.0f,
            horzAlign, vertAlign, 0, ((sharedUiInfo_t*)uiInfo)->assets.scrollBarArrowDown);

        int thumbPos2 = Item_ListBox_ThumbDrawPosition(dc, item);
        float thumbY = (float)thumbPos2;
        float thumbMax2 = downArrowY - 16.0f - 1.0f;
        if (thumbMax2 < thumbY)
            thumbY = thumbMax2;
        UI_DrawHandlePic(
            scrollX, thumbY, 16.0f, 16.0f,
            horzAlign, vertAlign, 0, ((sharedUiInfo_t*)uiInfo)->assets.scrollBarThumb);
    }

    sizeH = (((rectDef_t*)(it))->h) - 2.0f;

    if (((listBoxDef_t *)listPtr)->elementStyle == 1) {

        float ix = (((rectDef_t*)(it))->x) + 1.0f;
        float iy = (((rectDef_t*)(it))->y) + 1.0f;
        startPos = *(int *)listPtr;

        if (count <= startPos)
            return;

        float ix2 = ix + 1.0f;
        i = startPos;

        while (i < count) {

            MaterialHandle img2 = UI_FeederItemImage(((itemDef_t *)it)->special, i);
            if (img2) {
                UI_DrawHandlePic(
                    ix2, iy + 1.0f,
                    (((listBoxDef_s*)(listPtr))->elementWidth) - 2.0f,
                    (((listBoxDef_s*)(listPtr))->elementHeight) - 2.0f,
                    horzAlign, vertAlign, 0, img2);
            }

            if (i == ((itemDef_t *)it)->cursorPos[0]) {
                UI_DrawRect(ix, iy,
                            (((listBoxDef_s*)(listPtr))->elementWidth) - 1.0f,
                            (((listBoxDef_s*)(listPtr))->elementHeight) - 1.0f,
                            horzAlign, vertAlign,
                            ((itemDef_t *)it)->window.borderSize,
                            ((itemDef_t *)it)->window.borderColor);
            }

            ListBox_SetEndPos((void *)listPtr, (((listBoxDef_s*)(listPtr))->endPos[0]) + 1);
            sizeH -= (((listBoxDef_s*)(listPtr))->elementWidth);
            if ((((listBoxDef_s*)(listPtr))->elementHeight) > sizeH) {
                (((listBoxDef_s*)(listPtr))->drawPadding) = (int)((((listBoxDef_s*)(listPtr))->elementHeight) - sizeH);
                return;
            }
            iy += (((listBoxDef_s*)(listPtr))->elementHeight);
            i++;
        }
        return;
    }

    {
        float baseX = (((rectDef_t*)(it))->x) + 1.0f;
        y = (((rectDef_t*)(it))->y) + 1.0f;
        startPos = *(int *)(listPtr);

        if (count <= startPos)
            return;

        i = startPos;
        float preX4 = baseX + 4.0f;
        float preX2a = baseX + 2.0f;
        float preX2b = baseX + 2.0f;

        while (1) {
            int colCount = (((listBoxDef_s*)(listPtr))->numColumns);

            if (colCount > 0) {

                byte *col = listPtr;
                j = 0;
                while (j < (((listBoxDef_s*)(listPtr))->numColumns)) {

                    text = UI_FeederItemText(((itemDef_t *)it)->special, i, j, &optionalImage);

                    if (optionalImage) {

                        float imgSize = (float)((((listBoxDef_s*)(col))->columnInfo[0].width));
                        UI_DrawHandlePic(
                            (float)((((listBoxDef_s*)(col))->columnInfo[0].pos)) + preX2b,
                            y + 3.0f,
                            imgSize, imgSize,
                            horzAlign, vertAlign, 0, optionalImage);
                    } else if (text) {

                        float textScale = ((itemDef_t *)it)->textscale;
                        FontHandle font = UI_GetFontHandle(((itemDef_t *)it)->fontEnum, textScale);
                        float drawX = (float)((((listBoxDef_s*)(col))->columnInfo[0].pos)) + preX4 + (((itemDef_s*)(it))->textalignx);
                        float drawY = y + (((listBoxDef_s*)(listPtr))->elementHeight) + (((itemDef_s*)(it))->textaligny);
                        UI_DrawText(text, (((listBoxDef_s*)(col))->columnInfo[0].maxChars), font,
                                    drawX, drawY,
                                    horzAlign, vertAlign,
                                    textScale,
                                    ((itemDef_t *)it)->window.foreColor,
                                    (((itemDef_s*)(it))->textStyle));
                    }
                    j++;
                    col += 0xc;
                }
            } else {

                text = UI_FeederItemText(((itemDef_t *)it)->special, i, 0, &optionalImage);

                if (optionalImage) {

                    UI_DrawHandlePic(
                        preX2a,
                        y + 2.0f,
                        (((listBoxDef_s*)(listPtr))->elementWidth) - 4.0f,
                        (((listBoxDef_s*)(listPtr))->elementHeight) - 4.0f,
                        horzAlign, vertAlign, 0, optionalImage);
                } else if (text) {

                    float textScale2 = ((itemDef_t *)it)->textscale;
                    FontHandle font2 = UI_GetFontHandle(((itemDef_t *)it)->fontEnum, textScale2);
                    float drawX2 = preX4;
                    float drawY2 = y + (((listBoxDef_s*)(listPtr))->elementHeight);
                    UI_DrawText(text, 0x7fffffff, font2,
                                drawX2, drawY2,
                                horzAlign, vertAlign,
                                textScale2,
                                ((itemDef_t *)it)->window.foreColor,
                                (((itemDef_s*)(it))->textStyle));
                }
            }

            if (i == ((itemDef_t *)it)->cursorPos[0]) {
                UI_FillRect(preX2a, y + 2.0f,
                            (((rectDef_t*)(it))->w) - 16.0f - 4.0f,
                            (((listBoxDef_s*)(listPtr))->elementHeight),
                            horzAlign, vertAlign,
                            (float *)(((char *)it + offsetof(itemDef_t, window.outlineColor[0]))));
            }

            sizeH -= (((listBoxDef_s*)(listPtr))->elementHeight);
            if ((((listBoxDef_s*)(listPtr))->elementHeight) > sizeH) {
                (((listBoxDef_s*)(listPtr))->drawPadding) = (int)((((listBoxDef_s*)(listPtr))->elementHeight) - sizeH);
                return;
            }

            ListBox_SetEndPos((void *)listPtr, (((listBoxDef_s*)(listPtr))->endPos[0]) + 1);
            y += (((listBoxDef_s*)(listPtr))->elementHeight);

            i++;
            if (i == count)
                return;
        }
    }
}

void Script_SetItemColor(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char itemname[0x400];
    char name[0x400];
    float color[4];
    float f;
    int count, i, j;
    (void)dc;

    if (!String_Parse(args, itemname, 0x400))
        return;
    if (!String_Parse(args, name, 0x400))
        return;

    count = Menu_ItemsMatchingGroup(item->parent, itemname);

    for (j = 1; j < 5; j++) {
        if (!Float_Parse(args, &f))
            return;
        color[j - 1] = f;
    }

    for (i = 0; i < count; i++) {
        byte *item2 = (byte *)Menu_GetMatchingItemByNumber(item->parent, i, itemname);
        if (!item2)
            continue;

        float *out = NULL;
        if (I_stricmp(name, "backcolor") == 0) {
            out = (float *)(item2 + 0x1dc);
        } else if (I_stricmp(name, "forecolor") == 0) {
            out = (float *)(item2 + 0x1cc);
            Window_AddDynamicFlags((void *)item2, 0x10000);
        } else if (I_stricmp(name, "bordercolor") == 0) {
            out = (float *)(item2 + 0x1ec);
        }

        if (out) {
            for (j = 1; j < 5; j++)
                out[j - 1] = color[j - 1];
        }
    }
}

void Window_Paint(displayContextDef_t *dc, float (*w_param)[4][32], float fadeAmount, float fadeInAmount, float fadeClamp, float fadeCycle)
{
    byte *w = (byte *)w_param;
    float x, y, ww, hh;
    float color[4];
    int borderStyle, borderSizeFlag;
    int handle;

    hh = (((rectDef_t*)(w))->h);
    ww = (((rectDef_t*)(w))->w);
    y = (((rectDef_t*)(w))->y);
    x = (((rectDef_t*)(w))->x);

    if (debugMode) {
        color[0] = 1.0f;
        color[1] = 1.0f;
        color[2] = 1.0f;
        color[3] = 1.0f;
        UI_DrawRect(x, y, ww, hh,
                    (((rectDef_t*)(w))->horzAlign), (((rectDef_t*)(w))->vertAlign),
                    1.0f, color);
    }

    if (!w)
        return;

    borderStyle = ((windowDef_t *)w)->style;
    borderSizeFlag = ((windowDef_t *)w)->border;

    if (borderStyle == 0 && borderSizeFlag == 0)
        return;

    if (borderSizeFlag != 0) {

        float bs = ((windowDef_t *)w)->borderSize;
        x += bs;
        y += bs;
        ww -= (bs + 1.0f);
        hh -= (bs + 1.0f);
    }

    if (borderStyle <= 7) {
        switch (borderStyle) {
        case 1: {

            MaterialHandle background = ((windowDef_t *)w)->background;
            if (background == 0) {

                UI_FillRect(x, y, ww, hh,
                            (((rectDef_t*)(w))->horzAlign), (((rectDef_t*)(w))->vertAlign),
                            (const vec_t *)(((char *)w + offsetof(windowDef_t, backColor[0]))));
                break;
            }

            {
                int dynFlags = ((windowDef_t *)w)->dynamicFlags[0];
                int offsetTime = (int)fadeCycle;

                if (dynFlags & 0x30) {
                    int realtime = dc->realTime;
                    if (realtime > ((windowDef_t *)w)->nextTime) {
                        ((windowDef_t *)w)->nextTime = realtime + offsetTime;
                        if (dynFlags & 0x10) {

                            float fadeAlpha = ((windowDef_t *)w)->backColor[3] - fadeAmount;
                            ((windowDef_t *)w)->backColor[3] = fadeAlpha;
                            if (!(fadeAlpha > 0.0f)) {
                                dynFlags &= 0xffffffeb;
                            }
                        } else {

                            float fadeAlpha = ((windowDef_t *)w)->backColor[3] + fadeInAmount;
                            ((windowDef_t *)w)->backColor[3] = fadeAlpha;
                            if (!(fadeAlpha < fadeClamp)) {
                                ((windowDef_t *)w)->backColor[3] = fadeClamp;
                                dynFlags &= ~0x20;
                            }
                        }
                    }
                }

                Window_SetDynamicFlags((itemDef_t *)w, dynFlags);
            }

            UI_DrawHandlePic(
                x, y, ww, hh,
                (((rectDef_t*)(w))->horzAlign), (((rectDef_t*)(w))->vertAlign),
                (float *)(((char *)w + offsetof(windowDef_t, backColor[0]))), ((windowDef_t *)w)->background);
            break;
        }

        case 3: {

            float *fillColor = NULL;
            if ((((Window *)w)->dynamicFlags[0] & 0x10000))
                fillColor = (float *)(((char *)w + offsetof(windowDef_t, foreColor[0])));
            UI_DrawHandlePic(
                x, y, ww, hh,
                (((rectDef_t*)(w))->horzAlign), (((rectDef_t*)(w))->vertAlign),
                fillColor, ((windowDef_t *)w)->background);
            break;
        }

        case 5: {

            handle = ((windowDef_t *)w)->cinematic;
            if (handle == -1) {

                int result = CIN_PlayCinematic(
                    ((windowDef_t *)w)->cinematicName,
                    (int)x, (int)y, (int)ww, (int)hh, 0x0a);
                if (result != -1)
                    handle = result;
                else
                    handle = -2;
                ((windowDef_t *)w)->cinematic = handle;
            }
            if (handle < 0)
                break;
            CIN_RunCinematic(handle);
            CIN_SetExtents(((windowDef_t *)w)->cinematic, (int)x, (int)y, (int)ww, (int)hh);
            CIN_DrawCinematic(((windowDef_t *)w)->cinematic);
            break;
        }

        case 6: {

            MaterialHandle background = ((windowDef_t *)w)->background;
            if (background == 0)
                break;
            {
                float *fillColor = NULL;
                if ((((Window *)w)->dynamicFlags[0] & 0x10000))
                    fillColor = (float *)(((char *)w + offsetof(windowDef_t, foreColor[0])));
                UI_DrawHandlePic(
                    x, y, ww, hh,
                    (((rectDef_t*)(w))->horzAlign), (((rectDef_t*)(w))->vertAlign),
                    fillColor, background);
            }
            break;
        }

        case 7: {

            float *fillColor = NULL;
            if ((((Window *)w)->dynamicFlags[0] & 0x10000))
                fillColor = (float *)(((char *)w + offsetof(windowDef_t, foreColor[0])));
            UI_DrawLoadBar(
                x, y, ww, hh,
                (((rectDef_t*)(w))->horzAlign), (((rectDef_t*)(w))->vertAlign),
                fillColor, ((windowDef_t *)w)->background);
            break;
        }

        case 0:
        case 2:
        case 4:
        default:
            break;
        }
    }

    borderSizeFlag = ((windowDef_t *)w)->border;
    if (borderSizeFlag == 1) {

        float *borderColor;
        if (((windowDef_t *)w)->style == 4) {

            if (((windowDef_t *)w)->borderColor[0] > 0.0f) {

                color[0] = 1.0f;
                color[1] = 0.5f;
                color[2] = 0.5f;
            } else {

                color[0] = 0.5f;
                color[1] = 0.5f;
                color[2] = 1.0f;
            }
            color[3] = 1.0f;
            borderColor = color;
        } else {
            borderColor = (float *)(((char *)w + offsetof(windowDef_t, borderColor[0])));
        }
        UI_DrawRect(
            (((rectDef_t*)(w))->x), (((rectDef_t*)(w))->y),
            (((rectDef_t*)(w))->w), (((rectDef_t*)(w))->h),
            (((rectDef_t*)(w))->horzAlign), (((rectDef_t*)(w))->vertAlign),
            ((windowDef_t *)w)->borderSize, borderColor);
    } else if (borderSizeFlag == 2) {

        UI_DrawTopBottom(
            (((rectDef_t*)(w))->x), (((rectDef_t*)(w))->y),
            (((rectDef_t*)(w))->w), (((rectDef_t*)(w))->h),
            (((rectDef_t*)(w))->horzAlign), (((rectDef_t*)(w))->vertAlign),
            ((windowDef_t *)w)->borderSize, (const vec_t *)(((char *)w + offsetof(windowDef_t, borderColor[0]))));
        return;
    } else if (borderSizeFlag == 3) {

        UI_DrawSides(
            (((rectDef_t*)(w))->x), (((rectDef_t*)(w))->y),
            (((rectDef_t*)(w))->w), (((rectDef_t*)(w))->h),
            (((rectDef_t*)(w))->horzAlign), (((rectDef_t*)(w))->vertAlign),
            ((windowDef_t *)w)->borderSize, (const vec_t *)(((char *)w + offsetof(windowDef_t, borderColor[0]))));
        return;
    }
}

void Menu_Setup(displayContextDef_t *dc)
{
    dc->menuCount = 0;
    dc->openMenuCount = 0;
    Item_SetupKeywordHash();
    Menu_SetupKeywordHash();
    Controls_GetConfig_core();
}

static inline __attribute__((always_inline)) void Menus_CloseByName_core(displayContextDef_t *dc, const char *p)
{
    menuDef_t *menu = Menus_FindByName_core(dc, p);
    if (menu)
        Menus_Close(dc, menu);
}

void Menus_CloseByName(displayContextDef_t *dc, const char *p)
{
    Menus_CloseByName_core(dc, p);
}

void Menu_SetFeederSelection(displayContextDef_t *dc, menuDef_t *menu, int feeder, int index, const char *name)
{
    byte *m;
    float feederFloat;
    int i, itemCount;

    if (!menu) {

        if (name) {
            menu = Menus_FindByName(dc, name);
        } else {
            menu = Menu_GetFocused(dc);
        }
        if (!menu)
            return;
    }

    m = (byte *)menu;
    itemCount = (((menuDef_t*)(m))->itemCount);
    if (itemCount <= 0)
        return;

    feederFloat = (float)feeder;

    for (i = 0; i < itemCount; i++) {
        byte *item = *(byte **)((char *)&((menuDef_t *)m)->items[i]);
        byte *listPtr;

        if (((itemDef_t *)item)->special != feederFloat)
            continue;

        listPtr = (byte *)Item_GetListBoxDef((itemDef_t *)item);
        Item_SetCursorPos((itemDef_t *)item, index);
        UI_FeederSelection(((itemDef_t *)item)->special, ((itemDef_t *)item)->cursorPos[0]);

        if (!listPtr)
            continue;

        if (index == 0) {
            ListBox_SetCursorPos((itemDef_t *)listPtr, 0);
            ListBox_SetStartPos((itemDef_t *)listPtr, 0);
        } else if (index < *(int *)listPtr) {
            ListBox_SetStartPos((itemDef_t *)listPtr, index);
        }
    }
}

void Script_Exec(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    (void)dc;
    (void)item;
    char val[0x400];
    if (String_Parse(args, val, 0x400)) {
        Cbuf_ExecuteText(2, va("%s\n", val));
    }
}

void Menu_TransitionItemByName(menuDef_t *menu, const char *p, rectDef_t rectFrom, rectDef_t rectTo, int time, float amt)
{
    int count = Menu_ItemsMatchingGroup(menu, p);
    int i;

    for (i = 0; i < count; i++) {
        byte *item = (byte *)Menu_GetMatchingItemByNumber(menu, i, p);
        if (!item)
            continue;

        Window_AddDynamicFlags((void *)item, 0x84);
        Window_SetOffsetTime((itemDef_t *)item, time);
        Window_SetRectClient(item, &rectFrom);
        Window_SetRectEffects0(item, &rectTo);

        rectDef_t newRect = (*(rectDef_t *)&((itemDef_t *)item)->window.rectEffects1[0].x);
        int dx = (int)(rectTo.x - rectFrom.x);
        newRect.x = (float)(dx < 0 ? -dx : dx) / amt;
        int dy = (int)(rectTo.y - rectFrom.y);
        newRect.y = (float)(dy < 0 ? -dy : dy) / amt;
        int dw = (int)(rectTo.w - rectFrom.w);
        newRect.w = (float)(dw < 0 ? -dw : dw) / amt;
        int dh = (int)(rectTo.h - rectFrom.h);
        newRect.h = (float)(dh < 0 ? -dh : dh) / amt;

        Window_SetRectEffects1(item, &newRect);

        byte *parent = (*(byte **)&((itemDef_t *)item)->parent);
        if (parent) {
            float px = (((rectDef_t*)(parent))->x);
            float py = (((rectDef_t*)(parent))->y);
            if (((menuDef_t *)parent)->window.border) {
                float borderSize = ((menuDef_t *)parent)->window.borderSize;
                px += borderSize;
                py += borderSize;
            }
            Item_SetScreenCoords((itemDef_t *)item, px, py, (((rectDef_t*)(parent))->horzAlign), (((rectDef_t*)(parent))->vertAlign));
        }
    }
}

void Script_Transition(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char name[0x400];
    rectDef_t rectFrom, rectTo;
    int time;
    float amt;
    (void)dc;

    if (!String_Parse(args, name, 0x400))
        return;
    if (!Rect_Parse(args, &rectFrom))
        return;
    if (!Rect_Parse(args, &rectTo))
        return;
    if (!Int_Parse(args, &time))
        return;
    if (!Float_Parse(args, &amt))
        return;

    Menu_TransitionItemByName(item->parent, name, rectFrom, rectTo, time, amt);
}

void Menu_OrbitItemByName(menuDef_t *menu, const char *p, float x, float y, float cx, float cy, int time)
{
    int count = Menu_ItemsMatchingGroup(menu, p);
    int i;

    for (i = 0; i < count; i++) {
        byte *item = (byte *)Menu_GetMatchingItemByNumber(menu, i, p);
        if (!item)
            continue;

        Window_AddDynamicFlags((void *)item, 0x2004);
        Window_SetOffsetTime((itemDef_t *)item, time);

        rectDef_t newRect;
        newRect = (*(rectDef_t *)&((itemDef_t *)item)->window.rectEffects0[0].x);
        newRect.x = cx;
        newRect.y = cy;

        Window_SetRectEffects0(item, &newRect);

        newRect = (*(rectDef_t *)&((itemDef_t *)item)->window.rectClient[0].x);
        newRect.x = x;
        newRect.y = y;
        Window_SetRectClient(item, &newRect);

        byte *parent = (*(byte **)&((itemDef_t *)item)->parent);
        if (parent) {
            float px = (((rectDef_t*)(parent))->x);
            float py = (((rectDef_t*)(parent))->y);
            if (((menuDef_t *)parent)->window.border) {
                float borderSize = ((menuDef_t *)parent)->window.borderSize;
                px += borderSize;
                py += borderSize;
            }
            Item_SetScreenCoords((itemDef_t *)item, px, py, (((rectDef_t*)(parent))->horzAlign), (((rectDef_t*)(parent))->vertAlign));
        }
    }
}

void Script_Orbit(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char name[0x400];
    float cx, cy, x, y;
    int time;
    (void)dc;

    if (!String_Parse(args, name, 0x400))
        return;
    if (!Float_Parse(args, &x))
        return;
    if (!Float_Parse(args, &y))
        return;
    if (!Float_Parse(args, &cx))
        return;
    if (!Float_Parse(args, &cy))
        return;
    if (!Int_Parse(args, &time))
        return;

    Menu_OrbitItemByName(item->parent, name, x, y, cx, cy, time);
}

void Script_CloseForGameType(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char name[0x400];
    (void)item;
    if (!String_Parse(args, name, 0x400))
        return;
    const char *p = va(name, Dvar_GetString(item->dvar));
    Menus_CloseByName_core(dc, p);
}

void Script_Close(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char name[0x400];
    (void)item;
    if (!String_Parse(args, name, 0x400))
        return;
    Menus_CloseByName_core(dc, name);
}

void Script_InGameClose(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char name[0x400];
    (void)item;
    if (!String_Parse(args, name, 0x400))
        return;
    if (!UI_ClientIsInGame())
        return;
    Menus_CloseByName_core(dc, name);
}

void Menus_Open(displayContextDef_t *dc, menuDef_t *menu)
{
    byte *d = (byte *)dc;
    byte *m = (byte *)menu;
    int openCount, i, j;
    byte tempItem[0x2a0];

    openCount = (((displayContextDef_s*)(d))->openMenuCount);
    for (i = openCount - 1; i >= 0; i--)
        Window_RemoveDynamicFlags(*(void **)((char *)&((displayContextDef_s *)d)->menuStack[i]), 2);
    openCount = (((displayContextDef_s*)(d))->openMenuCount);

    int removeIdx = -1;
    for (i = openCount - 1; i >= 0; i--) {
        if (*(void **)((char *)&((displayContextDef_s *)d)->menuStack[i]) == menu) {
            removeIdx = i;
            break;
        }
    }
    if (removeIdx >= 0) {
        (((displayContextDef_s*)(d))->openMenuCount) = openCount - 1;
        for (i = removeIdx; i < (((displayContextDef_s*)(d))->openMenuCount); i++)
            *(void **)((char *)&((displayContextDef_s *)d)->menuStack[i]) = *(void **)((char *)&((displayContextDef_s *)d)->menuStack[(i + 1)]);
    }
    if ((((displayContextDef_s*)(d))->openMenuCount) == 0x10)
        Com_Error(1, "\x15Too many menus opened");
    int idx = (((displayContextDef_s*)(d))->openMenuCount);
    *(void **)((char *)&((displayContextDef_s *)d)->menuStack[idx]) = menu;
    (((displayContextDef_s*)(d))->openMenuCount) = idx + 1;
    Window_AddDynamicFlags((void *)menu, 6);
    if ((*(void **)&((menuDef_t *)m)->onOpen)) {
        *(void **)&(*(unsigned char *)&((itemDef_t *)tempItem)->parent) = menu;
        Item_RunScript(dc, (itemDef_t *)tempItem, ((menuDef_t *)m)->onOpen);
    }
    if ((*(void **)&((menuDef_t *)m)->soundName))
        UI_PlayLocalSoundAliasByName(((menuDef_t *)m)->soundName);
    openCount = (((displayContextDef_s*)(d))->openMenuCount);
    for (i = openCount - 1; i >= 0; i--) {
        byte *om = *(byte **)((char *)&((displayContextDef_s *)d)->menuStack[i]);
        if (!om)
            continue;
        if (((menuDef_t *)om)->window.style == 5) {
            int ch = ((menuDef_t *)om)->window.cinematic;
            if (ch >= 0) {
                CIN_StopCinematic(ch);
                ((menuDef_t *)om)->window.cinematic = -1;
            }
        }
        for (j = 0; j < (((menuDef_t*)(om))->itemCount); j++) {
            byte *it = *(byte **)((char *)&((menuDef_t *)om)->items[j]);
            if (((itemDef_t *)it)->window.style == 5) {
                int ch = ((itemDef_t *)it)->window.cinematic;
                if (ch >= 0) {
                    CIN_StopCinematic(ch);
                    ((itemDef_t *)it)->window.cinematic = -1;
                }
                it = *(byte **)((char *)&((menuDef_t *)om)->items[j]);
            }
            if ((((itemDef_s*)(it))->type) == 8)
                CIN_StopCinematic(-((itemDef_t *)it)->window.ownerDraw);
        }
    }
    Display_MouseMove(dc, NULL, ((displayContextDef_t *)d)->cursorx, (((displayContextDef_s*)(d))->cursory));
}

qboolean Menus_OpenByName(displayContextDef_t *dc, const char *p)
{
    menuDef_t *menu = Menus_FindByName_core(dc, p);
    if (menu) {
        Menus_Open(dc, menu);
        return 1;
    }
    return 0;
}

void Script_Open(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char name[0x400];
    (void)item;
    if (!String_Parse(args, name, 0x400))
        return;
    Menus_OpenByName(dc, name);
}

void Script_OpenForGameType(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char name[0x400];
    if (!String_Parse(args, name, 0x400))
        return;
    const char *p = va(name, Dvar_GetString(item->dvar));
    Menus_OpenByName(dc, p);
}

void Script_InGameOpen(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char name[0x400];
    (void)item;
    if (!String_Parse(args, name, 0x400))
        return;
    if (!UI_ClientIsInGame())
        return;
    Menus_OpenByName(dc, name);
}

qboolean Display_MouseMove(displayContextDef_t *dc, void *p, int x, int y)
{
    byte *d = (byte *)dc;
    if (p) {
        byte *menu = (byte *)p;
        rectDef_t newRect = *(rectDef_t *)menu;
        newRect.x += (float)x;
        newRect.y += (float)y;
        Window_SetRect(p, &newRect);
        Menu_UpdatePosition((menuDef_t *)p);
        return 1;
    }
    int openCount = (((displayContextDef_s*)(d))->openMenuCount);
    int i = openCount - 1;
    if (i < 0)
        return 1;
    int startIdx = i;
    for (; i >= 0; i--) {
        byte *menu = *(byte **)((char *)&((displayContextDef_s *)d)->menuStack[i]);
        int flags = ((menuDef_t *)menu)->window.dynamicFlags[0];
        if ((flags & 4) && (flags & 2)) {
            if ((((Window *)menu)->staticFlags & 0x1000000)) {
                Menu_HandleMouseMove(dc, (menuDef_t *)menu, (float)x, (float)y);
                return 1;
            }
            startIdx = i;
            break;
        }
    }
    for (i = 0; i <= startIdx; i++) {
        byte *menu = *(byte **)((char *)&((displayContextDef_s *)d)->menuStack[(startIdx - i)]);
        if (Menu_HandleMouseMove(dc, (menuDef_t *)menu, (float)x, (float)y))
            return 1;
    }
    return 1;
}

itemDef_t *Menu_SetNextCursorItem(displayContextDef_t *dc, menuDef_t *menu)
{
    byte *m = (byte *)menu;
    byte *d = (byte *)dc;
    int oldCursor = (((menuDef_t*)(m))->cursorItem[0]);
    int wrapped = 0;
    int cursor;
    if (oldCursor == -1) {
        Menu_SetCursorItem(menu, 0);
        wrapped = 1;
    }
    cursor = (((menuDef_t*)(m))->cursorItem[0]);
    for (;;) {
        if (cursor >= (((menuDef_t*)(m))->itemCount)) {
            Menu_SetCursorItem(menu, oldCursor);
            return NULL;
        }
        Menu_SetCursorItem(menu, cursor + 1);
        cursor = (((menuDef_t*)(m))->cursorItem[0]);
        if (cursor >= (((menuDef_t*)(m))->itemCount)) {
            if (wrapped)
                return *(itemDef_t **)((char *)&((menuDef_t *)m)->items[oldCursor]);
            Menu_SetCursorItem(menu, 0);
            wrapped = 1;
            cursor = (((menuDef_t*)(m))->cursorItem[0]);
        }
        itemDef_t *item = *(itemDef_t **)((char *)&((menuDef_t *)m)->items[cursor]);
        if (Item_SetFocus(dc, item, (float)((displayContextDef_t *)d)->cursorx, (float)(((displayContextDef_s*)(d))->cursory))) {
            cursor = (((menuDef_t*)(m))->cursorItem[0]);
            item = *(itemDef_t **)((char *)&((menuDef_t *)m)->items[cursor]);
            Menu_HandleMouseMove(dc, menu, (((rectDef_t*)(item))->x) + 1.0f, *((float *)item + 1) + 1.0f);
            cursor = (((menuDef_t*)(m))->cursorItem[0]);
            return *(itemDef_t **)((char *)&((menuDef_t *)m)->items[cursor]);
        }
        cursor = (((menuDef_t*)(m))->cursorItem[0]);
    }
}

itemDef_t *Menu_SetPrevCursorItem(displayContextDef_t *dc, menuDef_t *menu)
{
    byte *m = (byte *)menu;
    byte *d = (byte *)dc;
    int oldCursor = (((menuDef_t*)(m))->cursorItem[0]);
    int wrapped = 0;
    int cursor;
    if (oldCursor < 0) {
        Menu_SetCursorItem(menu, (((menuDef_t*)(m))->itemCount) - 1);
        wrapped = 1;
    }
    for (;;) {
        cursor = (((menuDef_t*)(m))->cursorItem[0]);
        if (cursor <= -1) {
            if (wrapped)
                goto fail;
            Menu_SetCursorItem(menu, (((menuDef_t*)(m))->itemCount) - 1);
            wrapped = 1;
            cursor = (((menuDef_t*)(m))->cursorItem[0]);
            if (cursor < 0)
                goto fail;
        } else {
            Menu_SetCursorItem(menu, cursor - 1);
            cursor = (((menuDef_t*)(m))->cursorItem[0]);
            if (cursor < 0) {
                if (wrapped)
                    goto try_focus;
                Menu_SetCursorItem(menu, (((menuDef_t*)(m))->itemCount) - 1);
                wrapped = 1;
                cursor = (((menuDef_t*)(m))->cursorItem[0]);
                if (cursor < 0)
                    goto fail;
            }
        }
    try_focus:;
        itemDef_t *item = *(itemDef_t **)((char *)&((menuDef_t *)m)->items[cursor]);
        if (Item_SetFocus(dc, item, (float)((displayContextDef_t *)d)->cursorx, (float)(((displayContextDef_s*)(d))->cursory))) {
            cursor = (((menuDef_t*)(m))->cursorItem[0]);
            item = *(itemDef_t **)((char *)&((menuDef_t *)m)->items[cursor]);
            Menu_HandleMouseMove(dc, menu, (((rectDef_t*)(item))->x) + 1.0f, *((float *)item + 1) + 1.0f);
            cursor = (((menuDef_t*)(m))->cursorItem[0]);
            return *(itemDef_t **)((char *)&((menuDef_t *)m)->items[cursor]);
        }
    }
fail:
    Menu_SetCursorItem(menu, oldCursor);
    return NULL;
}

void Item_TextColor(displayContextDef_t *dc, itemDef_t *item, vec4_t *newColor)
{
    byte *it = (byte *)item;
    byte *parent = (byte *)item->parent;
    float *out = (float *)newColor;
    int flags = ((itemDef_t *)it)->window.dynamicFlags[0];
    int curTime = dc->realTime;
    int i;

    if (flags & 0x30) {
        int offsetTime = (((menuDef_t*)(parent))->fadeCycle);
        float fadeAmount = (((menuDef_t*)(parent))->fadeAmount);
        float fadeInAmount = (((menuDef_t*)(parent))->fadeInAmount);
        float clamp = (((menuDef_t*)(parent))->fadeClamp);

        if (curTime > ((itemDef_t *)it)->window.nextTime) {
            ((itemDef_t *)it)->window.nextTime = curTime + offsetTime;
            if (flags & 0x10) {

                float alpha = ((itemDef_t *)it)->window.foreColor[3];
                alpha -= fadeAmount;
                ((itemDef_t *)it)->window.foreColor[3] = alpha;
                if (alpha <= 0.0f) {
                    flags &= ~0x14;
                }
            } else {

                float alpha = ((itemDef_t *)it)->window.foreColor[3];
                alpha += fadeInAmount;
                ((itemDef_t *)it)->window.foreColor[3] = alpha;
                if (alpha >= clamp) {
                    ((itemDef_t *)it)->window.foreColor[3] = clamp;
                    flags &= ~0x20;
                }
            }
        }
    }

    Window_SetDynamicFlags(item, flags);

    int visFlags = ((itemDef_t *)it)->window.dynamicFlags[0];
    if ((visFlags & 4) && (visFlags & 2)) {

        float lowLight[4];
        for (i = 0; i < 4; i++)
            lowLight[i] = *(float *)(((char *)parent + offsetof(menuDef_t, focusColor[0])) + i * 4) * 0.8f;

        float t = sinf((float)(curTime / 22));
        t = t * 0.5f + 0.5f;

        for (i = 0; i < 4; i++) {
            float hi = *(float *)(((char *)parent + offsetof(menuDef_t, focusColor[0])) + i * 4);
            float lo = lowLight[i];
            out[i] = hi + (lo - hi) * t;
            if (out[i] < 0.0f)
                out[i] = 0.0f;
            else if (out[i] > 1.0f)
                out[i] = 1.0f;
        }
    } else if ((((itemDef_s*)(it))->textStyle) == 1) {

        int pulseTime = curTime;
        if (pulseTime < 0)
            pulseTime += 255;
        if (!(pulseTime & 0x100)) {

            for (i = 0; i < 4; i++)
                out[i] = ((itemDef_t *)it)->window.foreColor[i];
        } else {
            float lowLight[4];
            for (i = 0; i < 4; i++)
                lowLight[i] = ((itemDef_t *)it)->window.foreColor[i] * 0.8f;

            float t = sinf((float)(curTime / 22));
            t = t * 0.5f + 0.5f;

            for (i = 0; i < 4; i++) {
                float hi = ((itemDef_t *)it)->window.foreColor[i];
                float lo = lowLight[i];
                out[i] = hi + (lo - hi) * t;
                if (out[i] < 0.0f)
                    out[i] = 0.0f;
                else if (out[i] > 1.0f)
                    out[i] = 1.0f;
            }
        }
    } else {

        for (i = 0; i < 4; i++)
            out[i] = ((itemDef_t *)it)->window.foreColor[i];
    }

    const char *dvarTest = ((itemDef_t *)it)->enableDvar;
    const char *dvarName = ((itemDef_t *)it)->dvarTest;
    if (dvarTest && dvarTest[0] && dvarName && dvarName[0]) {
        if ((*(byte *)&((itemDef_t *)it)->dvarFlags) & 3) {
            if (!Item_EnableShowViaDvar(item, 1)) {

                for (i = 0; i < 4; i++)
                    out[i] = *(float *)(((char *)parent + offsetof(menuDef_t, disableColor[0])) + i * 4);
            }
        }
    }
}

static const char *UI_TraceScriptItemParentName(itemDef_t *item)
{
    byte *it;
    byte *parent;
    const char *name;

    if (!item)
        return NULL;

    it = (byte *)item;
    parent = (byte *)item->parent;
    if (!parent)
        return NULL;

    name = ((menuDef_t *)parent)->window.name;
    if (!name)
        return NULL;

    if (!I_stricmp(name, "serverinfo_dm") ||
        !strncmp(name, "team_", 5) ||
        !I_stricmp(name, "team"))
        return name;

    return NULL;
}

void Item_Text_Paint(displayContextDef_t *dc, itemDef_t *item)
{
    byte *it = (byte *)item;
    const char *textPtr = ((itemDef_t *)it)->text;
    char text[0x400];
    float color[4];
    int width, height;
    const char *traceParent = UI_TraceScriptItemParentName(item);
    int traceText = 0;

    if (traceParent) {
        static int textTraceCount;
        if (textTraceCount < 320) {
            traceText = 1;
            ++textTraceCount;
        }
    }

    if (!textPtr) {
        if (!(*(void **)&((itemDef_t *)it)->dvar)) {
            if (traceText)
                if (getenv("MTRACE"))
                    Com_Printf("[menu-trace] Item_Text_Paint no text parent=%s item=%s type=%d dvar=%s\n",
                               traceParent,
                               item->window.name ? item->window.name : "<null>",
                               item->type,
                               item->dvar ? item->dvar : "");
            return;
        }
        I_strncpyz(text, Dvar_GetVariantString(item->dvar), 0x400);
        textPtr = text;
    }

    if (textPtr[0] == '@')
        textPtr = UI_SafeTranslateString(textPtr + 1);

    if (textPtr[0] == '\0') {
        if (traceText)
            if (getenv("MTRACE"))
                Com_Printf("[menu-trace] Item_Text_Paint empty parent=%s item=%s type=%d dvar=%s\n",
                           traceParent,
                           item->window.name ? item->window.name : "<null>",
                           item->type,
                           item->dvar ? item->dvar : "");
        return;
    }

    Item_TextColor(dc, item, (vec4_t *)color);
    Item_SetTextExtents(item, &width, &height, textPtr);

    int staticFlags = ((itemDef_t *)it)->window.staticFlags;
    if (staticFlags & 0x400000) {
        if (traceText)
            if (getenv("MTRACE"))
                Com_Printf("[menu-trace] Item_Text_Paint wrapped parent=%s item=%s text='%.80s' tr=(%.1f %.1f %.1f %.1f) color=(%.2f %.2f %.2f %.2f) static=0x%x\n",
                           traceParent,
                           item->window.name ? item->window.name : "<null>",
                           textPtr,
                           (((itemDef_s*)(it))->textRect[0].x), (((itemDef_s*)(it))->textRect[0].y), (((itemDef_s*)(it))->textRect[0].w), (((itemDef_s*)(it))->textRect[0].h),
                           color[0], color[1], color[2], color[3],
                           staticFlags);
        Item_Text_Wrapped_Paint(item, textPtr, color);
        return;
    }
    if (staticFlags & 0x800000) {
        if (traceText)
            if (getenv("MTRACE"))
                Com_Printf("[menu-trace] Item_Text_Paint autowrap parent=%s item=%s text='%.80s' tr=(%.1f %.1f %.1f %.1f) color=(%.2f %.2f %.2f %.2f) static=0x%x\n",
                           traceParent,
                           item->window.name ? item->window.name : "<null>",
                           textPtr,
                           (((itemDef_s*)(it))->textRect[0].x), (((itemDef_s*)(it))->textRect[0].y), (((itemDef_s*)(it))->textRect[0].w), (((itemDef_s*)(it))->textRect[0].h),
                           color[0], color[1], color[2], color[3],
                           staticFlags);
        Item_Text_AutoWrapped_Paint(item, textPtr, color);
        return;
    }

    FontHandle font = UI_GetFontHandle(((itemDef_t *)it)->fontEnum, ((itemDef_t *)it)->textscale);
    if (traceText)
        if (getenv("MTRACE"))
            Com_Printf("[menu-trace] Item_Text_Paint draw parent=%s item=%s text='%.80s' tr=(%.1f %.1f %.1f %.1f) wh=(%d %d) rectAlign=(%d %d) scale=%.3f style=%d font=%p color=(%.2f %.2f %.2f %.2f) static=0x%x\n",
                       traceParent,
                       item->window.name ? item->window.name : "<null>",
                       textPtr,
                       (((itemDef_s*)(it))->textRect[0].x), (((itemDef_s*)(it))->textRect[0].y), (((itemDef_s*)(it))->textRect[0].w), (((itemDef_s*)(it))->textRect[0].h),
                       width, height,
                       (((rectDef_t*)(it))->horzAlign), (((rectDef_t*)(it))->vertAlign),
                       ((itemDef_t *)it)->textscale, (((itemDef_s*)(it))->textStyle), (void *)font,
                       color[0], color[1], color[2], color[3],
                       staticFlags);
    UI_DrawText(textPtr, 0x7fffffff, font,
                (((itemDef_s*)(it))->textRect[0].x), (((itemDef_s*)(it))->textRect[0].y),
                (((rectDef_t*)(it))->horzAlign), (((rectDef_t*)(it))->vertAlign),
                ((itemDef_t *)it)->textscale, color, (((itemDef_s*)(it))->textStyle));
}

static void Item_List_Paint(displayContextDef_t *dc, itemDef_t *item, const char *text)
{
    byte *it = (byte *)item;
    float color[4];
    int width;
    int height;
    int staticFlags;
    FontHandle font;

    if (!text)
        return;

    if (text[0] == '@')
        text = UI_SafeTranslateString(text + 1);

    if (!text[0])
        return;

    Item_TextColor(dc, item, (vec4_t *)color);
    Item_SetTextExtents(item, &width, &height, text);

    staticFlags = ((itemDef_t *)it)->window.staticFlags;
    if (staticFlags & 0x400000) {
        Item_Text_Wrapped_Paint(item, text, color);
        return;
    }
    if (staticFlags & 0x800000) {
        Item_Text_AutoWrapped_Paint(item, text, color);
        return;
    }

    font = UI_GetFontHandle(((itemDef_t *)it)->fontEnum, ((itemDef_t *)it)->textscale);
    UI_DrawText(text, 0x7fffffff, font,
                (((itemDef_s*)(it))->textRect[0].x), (((itemDef_s*)(it))->textRect[0].y),
                (((rectDef_t*)(it))->horzAlign), (((rectDef_t*)(it))->vertAlign),
                ((itemDef_t *)it)->textscale, color, (((itemDef_s*)(it))->textStyle));
}

void Item_TextField_Paint(displayContextDef_t *dc, itemDef_t *item)
{
    editFieldDef_t *editPtr;
    char buff[1024];
    vec4_t newColor;
    FontHandle font;
    const char *value;
    const char *text;
    int width;
    int height;
    int textLen;
    int paintOffset;
    int maxChars;
    int cursorPos;
    int cursorChar;
    qboolean editing = g_editingField && g_editItem == item;
    float x;
    const char *traceParent = UI_TraceScriptItemParentName(item);
    int traceTextField = 0;

    if (traceParent) {
        static int textFieldTraceCount;
        if (textFieldTraceCount < 160) {
            traceTextField = 1;
            ++textFieldTraceCount;
        }
    }

    editPtr = Item_GetEditFieldDef(item);
    if (!editPtr) {
        if (traceTextField)
            if (getenv("MTRACE"))
                Com_Printf("[menu-trace] Item_TextField_Paint no edit parent=%s item=%s type=%d dvar=%s\n",
                           traceParent,
                           item->window.name ? item->window.name : "<null>",
                           item->type,
                           item->dvar ? item->dvar : "");
        return;
    }

    Item_Text_Paint(dc, item);

    buff[0] = '\0';
    if (item->dvar) {
        value = Dvar_GetVariantString(item->dvar);
        if (value) {
            I_strncpyz(buff, value, sizeof(buff));
        }
    }

    Item_SetTextExtents(item, &width, &height, buff);
    Item_TextColor(dc, item, &newColor);

    textLen = 0;
    while (textLen < (int)sizeof(buff) - 1 && buff[textLen] != '\0') {
        textLen++;
    }

    /* Scroll only the field being edited. On blur show the name's beginning. */
    paintOffset = editing ? editPtr->paintOffset : 0;
    if (paintOffset < 0) {
        paintOffset = 0;
    } else if (paintOffset > textLen) {
        paintOffset = textLen;
    }

    maxChars = editPtr->maxPaintChars;
    if (maxChars <= 0) {
        maxChars = 0x7fffffff;
    } else if (maxChars > (int)sizeof(buff) - 1 - paintOffset) {
        maxChars = (int)sizeof(buff) - 1 - paintOffset;
    }

    x = item->textRect[0].x + item->textRect[0].w;
    if (item->text && item->text[0]) {
        x += 8.0f;
    }

    text = buff + paintOffset;
    font = UI_GetFontHandle(item->fontEnum, item->textscale);
    if (!font) {
        if (traceTextField)
            if (getenv("MTRACE"))
                Com_Printf("[menu-trace] Item_TextField_Paint no font parent=%s item=%s text='%.80s' dvar=%s scale=%.3f\n",
                           traceParent,
                           item->window.name ? item->window.name : "<null>",
                           text,
                           item->dvar ? item->dvar : "",
                           item->textscale);
        return;
    }

    if (traceTextField)
        if (getenv("MTRACE"))
            Com_Printf("[menu-trace] Item_TextField_Paint draw parent=%s item=%s text='%.80s' dvar=%s tr=(%.1f %.1f %.1f %.1f) x=%.1f max=%d offset=%d cursor=%d font=%p color=(%.2f %.2f %.2f %.2f)\n",
                       traceParent,
                       item->window.name ? item->window.name : "<null>",
                       text,
                       item->dvar ? item->dvar : "",
                       item->textRect[0].x, item->textRect[0].y,
                       item->textRect[0].w, item->textRect[0].h,
                       x, maxChars, paintOffset, item->cursorPos[0],
                       (void *)font,
                       newColor[0], newColor[1], newColor[2], newColor[3]);

    if (editing) {
        cursorChar = Key_GetOverstrikeMode() == 1 ? '_' : '|';
        cursorPos = item->cursorPos[0] - paintOffset;
        if (cursorPos < -1) {
            cursorPos = -1;
        } else if (cursorPos > textLen - paintOffset) {
            cursorPos = textLen - paintOffset;
        }

        UI_DrawTextWithCursor(
            text,
            maxChars,
            font,
            x,
            item->textRect[0].y,
            item->window.rect[0].horzAlign,
            item->window.rect[0].vertAlign,
            item->textscale,
            newColor,
            item->textStyle,
            cursorPos,
            cursorChar);
        return;
    }

    UI_DrawText(
        text,
        maxChars,
        font,
        x,
        item->textRect[0].y,
        item->window.rect[0].horzAlign,
        item->window.rect[0].vertAlign,
        item->textscale,
        newColor,
        item->textStyle);
}

void Item_YesNo_Paint(displayContextDef_t *dc, itemDef_t *item)
{
    byte *it = (byte *)item;
    byte *parent = (byte *)item->parent;
    float newColor[4];
    float value = 0.0f;
    int i;

    if ((*(void **)&((itemDef_t *)it)->dvar)) {
        value = (float)atof(Dvar_GetVariantString(item->dvar));
    }

    int flags = ((itemDef_t *)it)->window.dynamicFlags[0];
    if ((flags & 4) && (flags & 2)) {
        float lowLight[4];
        for (i = 0; i < 4; i++)
            lowLight[i] = *(float *)(((char *)parent + offsetof(menuDef_t, focusColor[0])) + i * 4) * 0.8f;

        int curTime = dc->realTime;
        float t = sinf((float)(curTime / 22));
        t = t * 0.5f + 0.5f;

        for (i = 0; i < 4; i++) {
            float hi = *(float *)(((char *)parent + offsetof(menuDef_t, focusColor[0])) + i * 4);
            float lo = lowLight[i];
            newColor[i] = hi + (lo - hi) * t;
            if (newColor[i] < 0.0f)
                newColor[i] = 0.0f;
            else if (newColor[i] > 1.0f)
                newColor[i] = 1.0f;
        }
    } else {
        for (i = 0; i < 4; i++)
            newColor[i] = ((itemDef_t *)it)->window.foreColor[i];
    }

    const char *yesNoStr;
    if (value != 0.0f)
        yesNoStr = UI_SafeTranslateString("EXE_YES");
    else
        yesNoStr = UI_SafeTranslateString("EXE_NO");

    byte *textRect = (byte *)((itemDef_t *)it)->textRect;
    FontHandle font = UI_GetFontHandle(((itemDef_t *)it)->fontEnum, ((itemDef_t *)it)->textscale);
    float textX, textY;

    if ((*(int *)&((itemDef_t *)it)->text)) {

        Item_Text_Paint(dc, item);
        textX = (((itemDef_s*)(it))->textRect[0].x) + (((itemDef_s*)(it))->textRect[0].w) + 8.0f;
        textY = (((rectDef_t*)(textRect))->y);
    } else {
        textX = (((itemDef_s*)(it))->textRect[0].x);
        textY = (((rectDef_t*)(textRect))->y);
    }

    UI_DrawText(yesNoStr, 0x7fffffff, font, textX, textY,
                (((rectDef_t*)(it))->horzAlign), (((rectDef_t*)(it))->vertAlign),
                ((itemDef_t *)it)->textscale, newColor, (((itemDef_s*)(it))->textStyle));
}

extern void *imp_sharedUiInfo;
void Item_Slider_Paint(displayContextDef_t *dc, itemDef_t *item)
{
    byte *it = (byte *)item;
    byte *parent = (byte *)item->parent;
    float newColor[4];
    float y;
    float x;
    int i;

    if ((*(void **)&((itemDef_t *)it)->dvar)) {
        Dvar_GetVariantString(item->dvar);
        atof(Dvar_GetVariantString(item->dvar));
    }

    int flags = ((itemDef_t *)it)->window.dynamicFlags[0];
    if ((flags & 4) && (flags & 2)) {

        float lowLight[4];
        for (i = 0; i < 4; i++)
            lowLight[i] = *(float *)(((char *)parent + offsetof(menuDef_t, focusColor[0])) + i * 4) * 0.8f;

        int curTime = dc->realTime;
        float t = sinf((float)(curTime / 22));
        t = t * 0.5f + 0.5f;

        for (i = 0; i < 4; i++) {
            float hi = *(float *)(((char *)parent + offsetof(menuDef_t, focusColor[0])) + i * 4);
            float lo = lowLight[i];
            newColor[i] = hi + (lo - hi) * t;
            if (newColor[i] < 0.0f)
                newColor[i] = 0.0f;
            else if (newColor[i] > 1.0f)
                newColor[i] = 1.0f;
        }
    } else {

        for (i = 0; i < 4; i++)
            newColor[i] = ((itemDef_t *)it)->window.foreColor[i];
    }

    y = (((rectDef_t*)(it))->y);

    if ((*(int *)&((itemDef_t *)it)->text)) {

        Item_Text_Paint(dc, item);
        x = (((itemDef_s*)(it))->textRect[0].x) + (((itemDef_s*)(it))->textRect[0].w) + 8.0f;
    } else {
        x = (((rectDef_t*)(it))->x);
    }

    byte *uiInfo = (byte *)imp_sharedUiInfo;
    UI_DrawHandlePic(
        x, y, 96.0f, 16.0f,
        (((rectDef_t*)(it))->horzAlign), (((rectDef_t*)(it))->vertAlign), newColor,
        ((sharedUiInfo_t*)uiInfo)->assets.sliderBar);

    float thumbX = Item_Slider_ThumbPosition(item);
    UI_DrawHandlePic(
        thumbX - 5.0f, y - 2.0f, 10.0f, 20.0f,
        (((rectDef_t*)(it))->horzAlign), (((rectDef_t*)(it))->vertAlign), newColor,
        ((sharedUiInfo_t *)uiInfo)->assets.sliderThumb);
}

void Item_Bind_Paint(displayContextDef_t *dc, itemDef_t *item)
{
    byte *it = (byte *)item;
    byte *parent = (byte *)item->parent;
    float newColor[4];
    char bindText[0x100];
    const char *drawText;
    const char *bindName;
    float textX;
    float textY;
    FontHandle font;
    int maxChars;
    int i;

    editFieldDef_t *editPtr = Item_GetEditFieldDef(item);
    if (!editPtr)
        return;
    /* Retail binding rows omit maxPaintChars. Zero means unlimited text,
     * rather than an invisible key label. */
    maxChars = editPtr->maxPaintChars > 0 ? editPtr->maxPaintChars : 0x100;

    bindName = item->dvar;
    if (!bindName || !bindName[0])
        return;

    if (g_waitingForKey && g_bindItem == item) {
        drawText = UI_SafeTranslateString("EXE_KEYWAIT");
        newColor[0] = 0.8f;
        newColor[1] = 0.0f;
        newColor[2] = 0.0f;
        newColor[3] = 0.8f;
        goto draw;
    }

    BindingFromName(bindName, bindText);
    drawText = bindText;

    int flags = ((itemDef_t *)it)->window.dynamicFlags[0];
    if ((flags & 4) && (flags & 2)) {
        float lowLight[4];
        for (i = 0; i < 4; i++)
            lowLight[i] = *(float *)(((char *)parent + offsetof(menuDef_t, focusColor[0])) + i * 4) * 0.8f;

        int curTime = dc->realTime;
        float t = sinf((float)(curTime / 22));
        t = t * 0.5f + 0.5f;

        for (i = 0; i < 4; i++) {
            float hi = *(float *)(((char *)parent + offsetof(menuDef_t, focusColor[0])) + i * 4);
            float lo = lowLight[i];
            newColor[i] = hi + (lo - hi) * t;
            if (newColor[i] < 0.0f)
                newColor[i] = 0.0f;
            else if (newColor[i] > 1.0f)
                newColor[i] = 1.0f;
        }
    } else {
        for (i = 0; i < 4; i++)
            newColor[i] = ((itemDef_t *)it)->window.foreColor[i];
    }

draw:;

    byte *textRect = (byte *)((itemDef_t *)it)->textRect;
    if ((*(int *)&((itemDef_t *)it)->text)) {
        Item_Text_Paint(dc, item);

        textX = (((itemDef_s*)(it))->textRect[0].x) + (((itemDef_s*)(it))->textRect[0].w) + 8.0f;
    } else {
        textX = (((itemDef_s*)(it))->textRect[0].x);
    }
    textY = (((rectDef_t*)(textRect))->y);
    font = UI_GetFontHandle(((itemDef_t *)it)->fontEnum, ((itemDef_t *)it)->textscale);
    UI_DrawText(drawText, maxChars, font, textX, textY,
                (((rectDef_t*)(it))->horzAlign), (((rectDef_t*)(it))->vertAlign),
                ((itemDef_t *)it)->textscale, newColor, (((itemDef_s*)(it))->textStyle));
}

void Item_OwnerDraw_Paint(displayContextDef_t *dc, itemDef_t *item)
{
    byte *it = (byte *)item;
    byte *parent;
    float color[4];
    int flags, i;

    if (!item)
        return;

    parent = (byte *)item->parent;
    flags = ((itemDef_t *)it)->window.dynamicFlags[0];

    int offsetTime = (((menuDef_t*)(parent))->fadeCycle);
    int curTime = dc->realTime;

    if (flags & 0x30) {
        if (curTime > ((itemDef_t *)it)->window.nextTime) {
            ((itemDef_t *)it)->window.nextTime = curTime + offsetTime;
            if (flags & 0x10) {
                float alpha = ((itemDef_t *)it)->window.foreColor[3] - (((menuDef_t*)(parent))->fadeAmount);
                ((itemDef_t *)it)->window.foreColor[3] = alpha;
                if (alpha <= 0.0f)
                    flags &= ~0x14;
            } else {
                float alpha = ((itemDef_t *)it)->window.foreColor[3] + (((menuDef_t*)(parent))->fadeInAmount);
                ((itemDef_t *)it)->window.foreColor[3] = alpha;
                if (alpha >= (((menuDef_t*)(parent))->fadeClamp)) {
                    ((itemDef_t *)it)->window.foreColor[3] = (((menuDef_t*)(parent))->fadeClamp);
                    flags &= ~0x20;
                }
            }
        }
    }

    Window_SetDynamicFlags(item, flags);

    for (i = 0; i < 4; i++)
        color[i] = ((itemDef_t *)it)->window.foreColor[i];

    int visFlags = ((itemDef_t *)it)->window.dynamicFlags[0];
    if ((visFlags & 4) && (visFlags & 2)) {

        float lowLight[4];
        for (i = 0; i < 4; i++)
            lowLight[i] = *(float *)(((char *)parent + offsetof(menuDef_t, focusColor[0])) + i * 4) * 0.8f;
        float t = sinf((float)(curTime / 22));
        t = t * 0.5f + 0.5f;
        for (i = 0; i < 4; i++) {
            float hi = *(float *)(((char *)parent + offsetof(menuDef_t, focusColor[0])) + i * 4);
            color[i] = hi + (lowLight[i] - hi) * t;
            if (color[i] < 0.0f)
                color[i] = 0.0f;
            else if (color[i] > 1.0f)
                color[i] = 1.0f;
        }
    } else if ((((itemDef_s*)(it))->textStyle) == 1) {

        int pt = curTime;
        if (pt < 0)
            pt += 255;
        if (pt & 0x100) {
            float lowLight[4];
            for (i = 0; i < 4; i++)
                lowLight[i] = ((itemDef_t *)it)->window.foreColor[i] * 0.8f;
            float t = sinf((float)(curTime / 22));
            t = t * 0.5f + 0.5f;
            for (i = 0; i < 4; i++) {
                float hi = ((itemDef_t *)it)->window.foreColor[i];
                color[i] = hi + (lowLight[i] - hi) * t;
                if (color[i] < 0.0f)
                    color[i] = 0.0f;
                else if (color[i] > 1.0f)
                    color[i] = 1.0f;
            }
        }
    }

    if ((*(byte *)&((itemDef_t *)it)->dvarFlags) & 3) {
        if (!Item_EnableShowViaDvar(item, 1)) {
            for (i = 0; i < 4; i++)
                color[i] = *(float *)(((char *)parent + offsetof(menuDef_t, disableColor[0])) + i * 4);
        }
    }

    float scale = ((itemDef_t *)it)->textscale;
    int style = (((itemDef_s*)(it))->textStyle);
    int ownerDraw = ((itemDef_t *)it)->window.ownerDraw;
    int ownerDrawFlags = ((itemDef_t *)it)->window.ownerDrawFlags;
    int align = (((itemDef_s*)(it))->textalignment);
    float special = ((itemDef_t *)it)->special;
    MaterialHandle material = ((itemDef_t *)it)->window.background;
    FontHandle font = UI_GetFontHandle(((itemDef_t *)it)->fontEnum, scale);

    if ((*(int *)&((itemDef_t *)it)->text) && ((const char *)(*(int *)&((itemDef_t *)it)->text))[0]) {

        Item_Text_Paint(dc, item);
        byte *textRect = (byte *)((itemDef_t *)it)->textRect;
        float ownerX = (((itemDef_s*)(it))->textRect[0].x) + (((rectDef_t*)(textRect))->w) + 8.0f;
        UI_OwnerDraw(ownerX, (((rectDef_t*)(it))->y), (((rectDef_t*)(it))->w), (((rectDef_t*)(it))->h),
                     (((rectDef_t*)(it))->horzAlign), (((rectDef_t*)(it))->vertAlign), 0.0f, (((itemDef_s*)(it))->textaligny),
                     ownerDraw, ownerDrawFlags, align, special, font, scale, color, (MaterialHandle)(intptr_t)material, style);
    } else if ((*(int *)&((itemDef_t *)it)->text)) {

        byte *textRect = (byte *)((itemDef_t *)it)->textRect;
        float ownerX = (((itemDef_s*)(it))->textRect[0].x) + (((rectDef_t*)(textRect))->w);
        UI_OwnerDraw(ownerX, (((rectDef_t*)(it))->y), (((rectDef_t*)(it))->w), (((rectDef_t*)(it))->h),
                     (((rectDef_t*)(it))->horzAlign), (((rectDef_t*)(it))->vertAlign), 0.0f, (((itemDef_s*)(it))->textaligny),
                     ownerDraw, ownerDrawFlags, align, special, font, scale, color, (MaterialHandle)(intptr_t)material, style);
    } else {

        UI_OwnerDraw((((rectDef_t*)(it))->x), (((rectDef_t*)(it))->y), (((rectDef_t*)(it))->w), (((rectDef_t*)(it))->h),
                     (((rectDef_t*)(it))->horzAlign), (((rectDef_t*)(it))->vertAlign), (((itemDef_s*)(it))->textalignx), (((itemDef_s*)(it))->textaligny),
                     ownerDraw, ownerDrawFlags, align, special, font, scale, color, (MaterialHandle)(intptr_t)material, style);
    }
}

extern const char *Dvar_GetVariantString(const char *dvarName);
qboolean Item_Slider_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key, qboolean down)
{
    byte *it = (byte *)item;
    byte *d = (byte *)dc;

    if (!(*(void **)&((itemDef_t *)it)->dvar))
        return 0;
    if (!((*(byte *)&((itemDef_t *)it)->window.dynamicFlags[0]) & 4) || !((*(byte *)&((itemDef_t *)it)->window.dynamicFlags[0]) & 2))
        return 0;

    if (key >= 0xc8 && key <= 0xca) {
        float cx = (float)((displayContextDef_t *)d)->cursorx;
        float cy = (float)(((displayContextDef_s*)(d))->cursory);

        float rx = (((rectDef_t*)(it))->x), ry = (((rectDef_t*)(it))->y);
        float rw = (((rectDef_t*)(it))->w), rh = (((rectDef_t*)(it))->h);
        CalcScreenX(&cx, 4);
        CalcScreenY(&cy, 4);
        CalcScreenPlacement(&rx, &ry, &rw, &rh, (((rectDef_t*)(it))->horzAlign), (((rectDef_t*)(it))->vertAlign));
        if (cx < rx || cx > rx + rw || cy < ry || cy > ry + rh)
            return 0;

    }

    if (key >= 0xc8 && key <= 0xca) {
        Scroll_Slider_SetThumbPos_impl((byte *)dc, (byte *)item);
        return 1;
    }

    editFieldDef_t *editDef = Item_GetEditFieldDef(item);
    if (!editDef)
        return 0;

    float maxVal = *((float *)editDef + 1);
    float minVal = (((editFieldDef_s*)(editDef))->minVal);
    float step = (maxVal - minVal) * 0.05f;

    const char *valStr = Dvar_GetVariantString(((itemDef_t *)it)->dvar);
    float value = (float)atof(valStr);

    if (key == 0x9c || key == 0xa4) {

        value -= step;
        if (value < minVal)
            value = minVal;
    } else if (key == 0x9d || key == 0xa3) {

        value += step;
        if (value > maxVal)
            value = maxVal;
    } else {
        return 0;
    }

    Dvar_SetFromStringByName(((itemDef_t *)it)->dvar, va("%g", (double)value));
    return 1;
}

qboolean Item_YesNo_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key)
{
    byte *it = (byte *)item;
    byte *d = (byte *)dc;
    char dvarString[0x19];

    if (!(*(void **)&((itemDef_t *)it)->dvar))
        return 0;
    if (!((*(byte *)&((itemDef_t *)it)->window.dynamicFlags[0]) & 4) || !((*(byte *)&((itemDef_t *)it)->window.dynamicFlags[0]) & 2))
        return 0;

    if (key >= 0xc8 && key <= 0xca) {
        float cx = (float)((displayContextDef_t *)d)->cursorx;
        float cy = (float)(((displayContextDef_s*)(d))->cursory);
        float rx = (((rectDef_t*)(it))->x), ry = (((rectDef_t*)(it))->y);
        float rw = (((rectDef_t*)(it))->w), rh = (((rectDef_t*)(it))->h);
        CalcScreenX(&cx, 4);
        CalcScreenY(&cy, 4);
        CalcScreenPlacement(&rx, &ry, &rw, &rh, (((rectDef_t*)(it))->horzAlign), (((rectDef_t*)(it))->vertAlign));
        if (cx < rx || cx > rx + rw || cy < ry || cy > ry + rh)
            return 0;

    } else if (key != 0xd && key != 0x9c && key != 0x9d && key != 0xa4 && key != 0xa3 &&
               key != 0xc8 && key != 0xc9 && key != 0xca) {
        return 0;
    }

    I_strncpyz(dvarString, Dvar_GetVariantString(((itemDef_t *)it)->dvar), 0x19);
    int val = atoi(dvarString);
    Dvar_SetFromStringByName(((itemDef_t *)it)->dvar, va("%i", !val));
    return 1;
}

qboolean Item_SetFocus(displayContextDef_t *dc, itemDef_t *item, float x, float y)
{
    byte *it = (byte *)item;
    byte *d = (byte *)dc;
    int i;

    snd_alias_list_t *defaultSound = *(snd_alias_list_t **)((byte *)imp_sharedUiInfo + 0x40);

    if (!item)
        return 0;
    if ((((Window *)it)->staticFlags & 0x100000))
        return 0;
    int flags = ((itemDef_t *)it)->window.dynamicFlags[0];
    if (!(flags & 4))
        return 0;
    if (flags & 2)
        return 1;

    byte *parent = (*(byte **)&((itemDef_t *)it)->parent);

    if (parent) {
        int pflags = ((menuDef_t *)parent)->window.dynamicFlags[0];
        if (!(pflags & 4) || !(pflags & 2)) {

            byte *focusedMenu = NULL;
            int openCount = (((displayContextDef_s*)(d))->openMenuCount);
            for (i = openCount - 1; i >= 0; i--) {
                byte *om = *(byte **)((char *)&((displayContextDef_s *)d)->menuStack[i]);
                int omf = ((menuDef_t *)om)->window.dynamicFlags[0];
                if ((omf & 4) && (omf & 2)) {
                    focusedMenu = om;
                    break;
                }
            }
            if (!focusedMenu)
                goto check_dvar;

            float rx = (((rectDef_t*)(focusedMenu))->x), ry = (((rectDef_t*)(focusedMenu))->y);
            float rw = (((rectDef_t*)(focusedMenu))->w), rh = *(float *)(focusedMenu + 0xc);
            float cx = x, cy = y;
            CalcScreenX(&cx, 4);
            CalcScreenY(&cy, 4);
            CalcScreenPlacement(&rx, &ry, &rw, &rh, (((rectDef_t*)(focusedMenu))->horzAlign), (((rectDef_t*)(focusedMenu))->vertAlign));
            if (cx < rx || cx > rx + rw || cy < ry || cy > ry + rh)
                goto check_dvar;

            float prx = (((rectDef_t*)(parent))->x), pry = (((rectDef_t*)(parent))->y);
            float prw = (((rectDef_t*)(parent))->w), prh = ((menuDef_t *)parent)->window.rect[0].h;
            float pcx = x, pcy = y;
            CalcScreenX(&pcy, 4);
            CalcScreenY(&pcx, 4);
            CalcScreenPlacement(&prx, &pry, &prw, &prh, (((rectDef_t*)(parent))->horzAlign), (((rectDef_t*)(parent))->vertAlign));
            if (pcx < prx || pcx > prx + prw || pcy < pry || pcy > pry + prh)
                goto check_dvar;
        }
    }

check_dvar:;

    int dvarFlags = ((itemDef_t *)it)->dvarFlags;
    if (dvarFlags & 3) {
        if (!Item_EnableShowViaDvar(item, 1))
            return 0;
        dvarFlags = ((itemDef_t *)it)->dvarFlags;
    }
    if (dvarFlags & 0xc) {
        if (!Item_EnableShowViaDvar(item, 4))
            return 0;
    }

    byte *menu = (*(byte **)&((itemDef_t *)it)->parent);

    void *oldFocus = NULL;
    if (menu && (((menuDef_t*)(menu))->itemCount) > 0) {
        int itemCount = (((menuDef_t*)(menu))->itemCount);
        for (i = 0; i < itemCount; i++) {
            byte *sibling = *(byte **)((char *)&((menuDef_t *)menu)->items[i]);
            int sf = ((Window *)sibling)->dynamicFlags[0];
            if ((sf & 4) && (sf & 2))
                oldFocus = sibling;
            Window_RemoveDynamicFlags((void *)sibling, 2);

            byte *reloaded = *(byte **)((char *)&((menuDef_t *)menu)->items[i]);
            if (((itemDef_t *)reloaded)->leaveFocus)   /* was reloaded + 0x2bc (x86) */
                Item_RunScript(dc, (itemDef_t *)reloaded, ((itemDef_t *)reloaded)->leaveFocus);
        }
    } else {
        oldFocus = NULL;
    }

    int itemType = (((itemDef_s*)(it))->type);
    if (itemType == 0) {

        byte *textRect = (byte *)((itemDef_t *)it)->textRect;
        float tw = (((rectDef_t*)(textRect))->w);
        float th = (((rectDef_t*)(textRect))->h);
        float tx = (((itemDef_s*)(it))->textRect[0].x);
        float ty = (((rectDef_t*)(textRect))->y) - th;
        float tcx = x, tcy = y;
        CalcScreenX(&tcx, 4);
        CalcScreenY(&tcy, 4);
        CalcScreenPlacement(&tx, &ty, &tw, &th, (((rectDef_t*)(textRect))->horzAlign), (((rectDef_t*)(textRect))->vertAlign));
        if (tcx < tx || tcx > tx + tw)
            goto fail;
        if (tcy < ty || tcy > ty + th)
            goto fail;

        Window_AddDynamicFlags((void *)it, 2);
        snd_alias_list_t *itemSound = ((itemDef_t *)it)->focusSound;
        if (itemSound) {
            UI_PlayLocalSoundAlias(itemSound);
        } else if (defaultSound) {
            UI_PlayLocalSoundAlias(defaultSound);
        }
        goto set_cursor;
    }

    Window_AddDynamicFlags((void *)it, 2);

    if ((*(void **)&((itemDef_t *)it)->onFocus))
        Item_RunScript(dc, item, ((itemDef_t *)it)->onFocus);

    snd_alias_list_t *itemSound = ((itemDef_t *)it)->focusSound;
    if (itemSound) {
        UI_PlayLocalSoundAlias(itemSound);
    } else if (defaultSound) {
        UI_PlayLocalSoundAlias(defaultSound);
    }

set_cursor:;

    if (menu) {
        int itemCount = (((menuDef_t*)(menu))->itemCount);
        byte *items = (*(byte **)&((menuDef_t *)menu)->items);
        for (i = 0; i < itemCount; i++) {
            if (*(void **)(items + i * 4) == item) {
                Menu_SetCursorItem((menuDef_t *)menu, i);
                return 1;
            }
        }
    }
    return 1;

fail:

    if (oldFocus) {
        Window_AddDynamicFlags(oldFocus, 2);
        if ((*(void **)&((itemDef_t *)oldFocus)->onFocus))
            Item_RunScript(dc, (itemDef_t *)oldFocus, ((itemDef_t *)oldFocus)->onFocus);
    }

    if (menu) {
        int itemCount = (((menuDef_t*)(menu))->itemCount);
        byte *items = (*(byte **)&((menuDef_t *)menu)->items);
        for (i = 0; i < itemCount; i++) {
            if (*(void **)(items + i * 4) == item) {
                Menu_SetCursorItem((menuDef_t *)menu, i);
                return 1;
            }
        }
    }
    return 1;
}

void Script_SetFocusByDvar(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char dvarName[0x400];
    byte *parent;
    int i;

    if (!String_Parse(args, dvarName, 0x400))
        return;

    parent = (byte *)item->parent;
    if (!parent)
        return;

    int itemCount = (((menuDef_t*)(parent))->itemCount);
    for (i = 0; i < itemCount; i++) {
        byte *focusItem = *(byte **)((char *)&((menuDef_t *)parent)->items[i]);

        if (!((*(byte *)&((itemDef_t *)focusItem)->dvarFlags) & 0x10))
            continue;

        const char *dvarTest = ((itemDef_t *)focusItem)->dvarTest;
        if (!dvarTest)
            Com_Error(1, "\x15Script_SetFocusByDvar: Item's dvarTest field is empty.");

        if (I_stricmp(dvarTest, dvarName) != 0)
            continue;

        if (!Item_EnableShowViaDvar((itemDef_t *)focusItem, 0x10))
            continue;

        if (Item_SetFocus(dc, (itemDef_t *)focusItem, (((rectDef_t*)(focusItem))->x), (((rectDef_t*)(focusItem))->y)))
            return;
    }
}

void Script_SetFocus(displayContextDef_t *dc, itemDef_t *item, const char **args)
{
    char name[0x400];
    byte *menu;
    byte *focusItem = NULL;
    int i;

    if (!String_Parse(args, name, 0x400))
        return;

    menu = (byte *)item->parent;

    if (menu && (((menuDef_t*)(menu))->itemCount) > 0) {
        for (i = 0; i < (((menuDef_t*)(menu))->itemCount); i++) {
            byte *it = *(byte **)((char *)&((menuDef_t *)menu)->items[i]);
            const char *itemName = ((itemDef_t *)it)->window.name;
            if (itemName && I_stricmp(name, itemName) == 0) {
                focusItem = it;
                break;
            }
        }
    }

    if (!focusItem) {
        Com_Printf("setFocus: could not find widget named '%s'\n", name);
        return;
    }

    if (!Item_SetFocus(dc, (itemDef_t *)focusItem, (((rectDef_t*)(focusItem))->x), (((rectDef_t*)(focusItem))->y))) {
        Com_Printf("setFocus: error focusing widget '%s' (widget was found but could not accept focus)\n", name);
        return;
    }

    int itemType = (((itemDef_s*)(focusItem))->type);
    if (itemType <= 0x12 && ((1 << itemType) & 0x70210)) {
        editFieldDef_t *editPtr = Item_GetEditFieldDef((itemDef_t *)focusItem);
        if (editPtr)
            editPtr->paintOffset = 0;
        Item_SetCursorPos((itemDef_t *)focusItem, 0);
        g_editingField = 1;
        g_editItem = (itemDef_t *)focusItem;
        Key_SetOverstrikeMode(1);
    }
}

int Item_ListBox_OverLB(itemDef_t *item, float x, float y)
{
    byte *it = (byte *)item;
    int horzAlign = (((rectDef_t*)(it))->horzAlign);
    int vertAlign = (((rectDef_t*)(it))->vertAlign);

    UI_FeederCount(((itemDef_t *)it)->special);
    if (!Item_GetListBoxDef(item))
        return 0;

    if ((((Window *)it)->staticFlags & 0x200000)) {

        float scrollX = (((rectDef_t*)(it))->x) + (((rectDef_t*)(it))->w) - 16.0f;
        float itemY = (((rectDef_t*)(it))->y);

        float rx = scrollX, ry = itemY, rw = 16.0f, rh = 16.0f;
        float cx = x, cy = y;
        CalcScreenX(&cx, 4);
        CalcScreenY(&cy, 4);
        CalcScreenPlacement(&rx, &ry, &rw, &rh, horzAlign, vertAlign);
        if (cx >= rx && cx <= rx + rw && cy >= ry && cy <= ry + rh)
            return 0x200;

        float endY = itemY + (((rectDef_t*)(it))->h) - 16.0f;
        rx = scrollX;
        ry = endY;
        rw = 16.0f;
        rh = 16.0f;
        cx = x;
        cy = y;
        CalcScreenX(&cx, 4);
        CalcScreenY(&cy, 4);
        CalcScreenPlacement(&rx, &ry, &rw, &rh, horzAlign, vertAlign);
        if (cx >= rx && cx <= rx + rw && cy >= ry && cy <= ry + rh)
            return 0x100;

        int thumbPos = Item_ListBox_ThumbPosition(item);

        rx = scrollX;
        ry = (float)thumbPos;
        rw = 16.0f;
        rh = 16.0f;
        cx = x;
        cy = y;
        CalcScreenX(&cx, 4);
        CalcScreenY(&cy, 4);
        CalcScreenPlacement(&rx, &ry, &rw, &rh, horzAlign, vertAlign);
        if (cx >= rx && cx <= rx + rw && cy >= ry && cy <= ry + rh)
            return 0x800;

        rx = scrollX;
        ry = itemY + 16.0f + 1.0f;
        rw = 16.0f;
        rh = (float)thumbPos - ry;
        cx = x;
        cy = y;
        CalcScreenX(&cx, 4);
        CalcScreenY(&cy, 4);
        CalcScreenPlacement(&rx, &ry, &rw, &rh, horzAlign, vertAlign);
        if (cx >= rx && cx <= rx + rw && cy >= ry && cy <= ry + rh)
            return 0x400;

        float thumbEnd = (float)thumbPos + 16.0f + 1.0f;
        rx = scrollX;
        ry = thumbEnd;
        rw = 16.0f;
        rh = endY - thumbEnd;
        cx = x;
        cy = y;
        CalcScreenX(&cx, 4);
        CalcScreenY(&cy, 4);
        CalcScreenPlacement(&rx, &ry, &rw, &rh, horzAlign, vertAlign);
        if (cx >= rx && cx <= rx + rw && cy >= ry && cy <= ry + rh)
            return 0x400;
    } else {

        float scrollX = (((rectDef_t*)(it))->x) + (((rectDef_t*)(it))->w) - 16.0f;
        float itemY = (((rectDef_t*)(it))->y);

        float rx = scrollX, ry = itemY, rw = 16.0f, rh = 16.0f;
        float cx = x, cy = y;
        CalcScreenX(&cx, 4);
        CalcScreenY(&cy, 4);
        CalcScreenPlacement(&rx, &ry, &rw, &rh, horzAlign, vertAlign);
        if (cx >= rx && cx <= rx + rw && cy >= ry && cy <= ry + rh)
            return 0x100;

        float endY = itemY + (((rectDef_t*)(it))->h) - 16.0f;
        rx = scrollX;
        ry = endY;
        rw = 16.0f;
        rh = 16.0f;
        cx = x;
        cy = y;
        CalcScreenX(&cx, 4);
        CalcScreenY(&cy, 4);
        CalcScreenPlacement(&rx, &ry, &rw, &rh, horzAlign, vertAlign);
        if (cx >= rx && cx <= rx + rw && cy >= ry && cy <= ry + rh)
            return 0x200;

        int thumbPos = Item_ListBox_ThumbPosition(item);
        rx = scrollX;
        ry = (float)thumbPos;
        rw = 16.0f;
        rh = 16.0f;
        cx = x;
        cy = y;
        CalcScreenX(&cx, 4);
        CalcScreenY(&cy, 4);
        CalcScreenPlacement(&rx, &ry, &rw, &rh, horzAlign, vertAlign);
        if (cx >= rx && cx <= rx + rw && cy >= ry && cy <= ry + rh)
            return 0x800;

        rx = scrollX;
        ry = itemY + 16.0f + 1.0f;
        rw = 16.0f;
        rh = (float)thumbPos - ry;
        cx = x;
        cy = y;
        CalcScreenX(&cx, 4);
        CalcScreenY(&cy, 4);
        CalcScreenPlacement(&rx, &ry, &rw, &rh, horzAlign, vertAlign);
        if (cx >= rx && cx <= rx + rw && cy >= ry && cy <= ry + rh)
            return 0x400;

        float thumbEnd = (float)thumbPos + 16.0f + 1.0f;
        rx = scrollX;
        ry = thumbEnd;
        rw = 16.0f;
        rh = endY - thumbEnd;
        cx = x;
        cy = y;
        CalcScreenX(&cx, 4);
        CalcScreenY(&cy, 4);
        CalcScreenPlacement(&rx, &ry, &rw, &rh, horzAlign, vertAlign);
        if (cx >= rx && cx <= rx + rw && cy >= ry && cy <= ry + rh)
            return 0x400;
    }

    return 0;
}

qboolean Item_ListBox_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key, qboolean down, qboolean force)
{
    byte *it = (byte *)item;
    byte *d = (byte *)dc;
    byte *listPtr;
    int count, max, viewmax, flags;
    int inRect;

    listPtr = (byte *)Item_GetListBoxDef(item);
    if (!listPtr)
        return 0;

    count = UI_FeederCount(((itemDef_t *)it)->special);

    {
        float compareX, compareY;
        float compareRect_x, compareRect_y, compareRect_w, compareRect_h;

        compareX = (float)((displayContextDef_t *)d)->cursorx;
        compareY = (float)(((displayContextDef_s*)(d))->cursory);

        compareRect_x = (((rectDef_t*)(it))->x);
        compareRect_y = (((rectDef_t*)(it))->y);
        compareRect_w = (((rectDef_t*)(it))->w);
        compareRect_h = (((rectDef_t*)(it))->h);

        CalcScreenX(&compareX, 4);
        CalcScreenY(&compareY, 4);
        CalcScreenPlacement(&compareRect_x, &compareRect_y, &compareRect_w, &compareRect_h,
                            (((rectDef_t*)(it))->horzAlign), (((rectDef_t*)(it))->vertAlign));

        if (compareX >= compareRect_x &&
            compareX <= compareRect_x + compareRect_w &&
            compareY >= compareRect_y &&
            compareY <= compareRect_y + compareRect_h)
            inRect = 1;
        else
            inRect = 0;
    }

    flags = ((itemDef_t *)it)->window.dynamicFlags[0];

    if (!force) {
        if (!inRect || !(flags & 2))
            return 0;
    }

    UI_OverrideCursorPos(item);
    max = Item_ListBox_MaxScroll(item);

    if ((((Window *)it)->staticFlags & 0x200000)) {

        viewmax = (int)((((rectDef_t*)(it))->w) / (((listBoxDef_s*)(listPtr))->elementWidth));

        if (key == 0x9c) {

            if ((((listBoxDef_s*)(listPtr))->notselectable)) {

                goto scroll_start_dec;
            }
            ListBox_SetCursorPos((itemDef_t *)listPtr, (((listBoxDef_s*)(listPtr))->cursorPos[0]) - 1);
            if ((((listBoxDef_s*)(listPtr))->cursorPos[0]) < 0)
                ListBox_SetCursorPos((itemDef_t *)listPtr, 0);
            if ((((listBoxDef_s*)(listPtr))->cursorPos[0]) < *(int *)listPtr)
                ListBox_SetStartPos((itemDef_t *)listPtr, (((listBoxDef_s*)(listPtr))->cursorPos[0]));
            if ((((listBoxDef_s*)(listPtr))->cursorPos[0]) >= viewmax + *(int *)listPtr)
                ListBox_SetStartPos((itemDef_t *)listPtr, (((listBoxDef_s*)(listPtr))->cursorPos[0]) - viewmax + 1);
            Item_SetCursorPos(item, (((listBoxDef_s*)(listPtr))->cursorPos[0]));
            goto do_feeder_selection;
        }

        if (key == 0x9d) {

            if ((((listBoxDef_s*)(listPtr))->notselectable)) {

                ListBox_SetStartPos((itemDef_t *)listPtr, *(int *)listPtr + 1);
                if (count > *(int *)listPtr)
                    return 1;
                ListBox_SetStartPos((itemDef_t *)listPtr, count - 1);
                return 1;
            }
            ListBox_SetCursorPos((itemDef_t *)listPtr, (((listBoxDef_s*)(listPtr))->cursorPos[0]) + 1);
            if ((((listBoxDef_s*)(listPtr))->cursorPos[0]) < *(int *)listPtr)
                ListBox_SetStartPos((itemDef_t *)listPtr, (((listBoxDef_s*)(listPtr))->cursorPos[0]));
            if (count <= (((listBoxDef_s*)(listPtr))->cursorPos[0]))
                ListBox_SetCursorPos((itemDef_t *)listPtr, count - 1);
            if ((((listBoxDef_s*)(listPtr))->cursorPos[0]) >= viewmax + *(int *)listPtr)
                ListBox_SetStartPos((itemDef_t *)listPtr, (((listBoxDef_s*)(listPtr))->cursorPos[0]) - viewmax + 1);
            Item_SetCursorPos(item, (((listBoxDef_s*)(listPtr))->cursorPos[0]));
            goto do_feeder_selection;
        }

    } else {

        viewmax = (int)((((rectDef_t*)(it))->h) / (((listBoxDef_s*)(listPtr))->elementHeight));

        if (key == 0x9a || key == 0xce) {

            if ((((listBoxDef_s*)(listPtr))->notselectable)) {

                goto scroll_start_dec;
            }
            Item_SetCursorPos(item, ((itemDef_t *)it)->cursorPos[0] - 1);
            if (((itemDef_t *)it)->cursorPos[0] < 0)
                Item_SetCursorPos(item, 0);
            if (((itemDef_t *)it)->cursorPos[0] < *(int *)listPtr)
                ListBox_SetStartPos((itemDef_t *)listPtr, ((itemDef_t *)it)->cursorPos[0]);
            if (((itemDef_t *)it)->cursorPos[0] >= viewmax + *(int *)listPtr)
                ListBox_SetStartPos((itemDef_t *)listPtr, ((itemDef_t *)it)->cursorPos[0] - viewmax + 1);
            UI_FeederSelection(((itemDef_t *)it)->special, ((itemDef_t *)it)->cursorPos[0]);
            return 1;
        }

        if (key == 0x9b || key == 0xcd) {

            if ((((listBoxDef_s*)(listPtr))->notselectable)) {

                goto scroll_start_inc;
            }
            Item_SetCursorPos(item, ((itemDef_t *)it)->cursorPos[0] + 1);
            if (((itemDef_t *)it)->cursorPos[0] < *(int *)listPtr)
                ListBox_SetStartPos((itemDef_t *)listPtr, ((itemDef_t *)it)->cursorPos[0]);
            if (count <= ((itemDef_t *)it)->cursorPos[0])
                Item_SetCursorPos(item, count - 1);
            if (((itemDef_t *)it)->cursorPos[0] < 0)
                Item_SetCursorPos(item, 0);
            if (((itemDef_t *)it)->cursorPos[0] >= viewmax + *(int *)listPtr) {
                ListBox_SetStartPos((itemDef_t *)listPtr, ((itemDef_t *)it)->cursorPos[0] - viewmax + 1);
            }
            goto do_feeder_selection;
        }

    }

    if (key == 0xc8 || key == 0xc9) {

        if (flags & 0x100) {

            goto scroll_start_dec;
        }
        if (flags & 0x200) {

            goto scroll_start_inc;
        }
        if (flags & 0x800) {

            goto scroll_start_pgup;
        }
        if (flags & 0x1000) {

            goto scroll_start_pgdn;
        }
        if (flags & 0x400) {

            return 1;
        }

        if (((itemDef_t *)it)->special == 18.0f) {
            UI_OverrideCursorPos(item);
            UI_FeederSelection(((itemDef_t *)it)->special, ((itemDef_t *)it)->cursorPos[0]);
        }

        {
            int curTime = (((displayContextDef_s*)(d))->realTime);
            if (curTime < lastListBoxClickTime &&
                ((listBoxDef_t *)listPtr)->doubleClick != NULL &&
                ((itemDef_t *)it)->cursorPos[0] == (((listBoxDef_s*)(listPtr))->cursorPos[0])) {
                if (ListBox_HasValidCursorPos((itemDef_t *)listPtr)) {
                    Item_RunScript(dc, item, ((listBoxDef_t *)listPtr)->doubleClick);
                    curTime = (((displayContextDef_s*)(d))->realTime);
                }
            }

            lastListBoxClickTime = curTime + 0x12c;

            if (((itemDef_t *)it)->cursorPos[0] == (((listBoxDef_s*)(listPtr))->cursorPos[0]))
                return 1;

            if (((itemDef_t *)it)->cursorPos[0] < 0)
                Item_SetCursorPos(item, (((listBoxDef_s*)(listPtr))->cursorPos[0]));

            {
                int oldCursorPos = (((listBoxDef_s*)(listPtr))->cursorPos[0]);
                if (oldCursorPos < UI_FeederCount(((itemDef_t *)it)->special))
                    Item_SetCursorPos(item, (((listBoxDef_s*)(listPtr))->cursorPos[0]));
            }
            goto do_feeder_selection;
        }
    }

    if (key == 0xa5) {

        if ((((listBoxDef_s*)(listPtr))->notselectable)) {

            ListBox_SetStartPos((itemDef_t *)listPtr, 0);
            return 1;
        }
        Item_SetCursorPos(item, 0);
        ListBox_SetCursorPos((itemDef_t *)listPtr, 0);
        ListBox_SetStartPos((itemDef_t *)listPtr, 0);
        goto do_feeder_selection;
    }

    if (key == 0xa6) {

        if ((((listBoxDef_s*)(listPtr))->notselectable)) {

            ListBox_SetStartPos((itemDef_t *)listPtr, max);
            return 1;
        }
        Item_SetCursorPos(item, count - 1);
        ListBox_SetCursorPos((itemDef_t *)listPtr, count - 1);
        if (((itemDef_t *)it)->cursorPos[0] >= viewmax + *(int *)listPtr)
            ListBox_SetStartPos((itemDef_t *)listPtr, ((itemDef_t *)it)->cursorPos[0] - viewmax + 1);
        if (((itemDef_t *)it)->cursorPos[0] < 0) {
            Item_SetCursorPos(item, 0);
            ListBox_SetCursorPos((itemDef_t *)listPtr, 0);
        }
        goto do_feeder_selection;
    }

    if (key == 0xa4) {

        if ((((listBoxDef_s*)(listPtr))->notselectable)) {
            goto scroll_start_pgup;
        }
        ListBox_SetCursorPos((itemDef_t *)listPtr, (((listBoxDef_s*)(listPtr))->cursorPos[0]) - viewmax);
        if ((((listBoxDef_s*)(listPtr))->cursorPos[0]) < 0)
            ListBox_SetCursorPos((itemDef_t *)listPtr, 0);
        if ((((listBoxDef_s*)(listPtr))->cursorPos[0]) < *(int *)listPtr)
            ListBox_SetStartPos((itemDef_t *)listPtr, (((listBoxDef_s*)(listPtr))->cursorPos[0]));
        if ((((listBoxDef_s*)(listPtr))->cursorPos[0]) >= viewmax + *(int *)listPtr)
            ListBox_SetStartPos((itemDef_t *)listPtr, (((listBoxDef_s*)(listPtr))->cursorPos[0]) - viewmax + 1);
        Item_SetCursorPos(item, (((listBoxDef_s*)(listPtr))->cursorPos[0]));
        goto do_feeder_selection;
    }

    if (key == 0xa3) {

        if ((((listBoxDef_s*)(listPtr))->notselectable)) {
            goto scroll_start_pgdn;
        }
        ListBox_SetCursorPos((itemDef_t *)listPtr, (((listBoxDef_s*)(listPtr))->cursorPos[0]) + viewmax);
        if ((((listBoxDef_s*)(listPtr))->cursorPos[0]) < *(int *)listPtr)
            ListBox_SetStartPos((itemDef_t *)listPtr, (((listBoxDef_s*)(listPtr))->cursorPos[0]));
        if (count <= (((listBoxDef_s*)(listPtr))->cursorPos[0]))
            ListBox_SetCursorPos((itemDef_t *)listPtr, count - 1);
        if ((((listBoxDef_s*)(listPtr))->cursorPos[0]) < 0)
            ListBox_SetCursorPos((itemDef_t *)listPtr, 0);
        if ((((listBoxDef_s*)(listPtr))->cursorPos[0]) >= viewmax + *(int *)listPtr)
            ListBox_SetStartPos((itemDef_t *)listPtr, (((listBoxDef_s*)(listPtr))->cursorPos[0]) - viewmax + 1);
        Item_SetCursorPos(item, (((listBoxDef_s*)(listPtr))->cursorPos[0]));
        goto do_feeder_selection;
    }

    return 0;

scroll_start_dec: {
    int newStart = *(int *)listPtr - 1;
    ListBox_SetStartPos((itemDef_t *)listPtr, newStart);
    if (*(int *)listPtr < 0)
        ListBox_SetStartPos((itemDef_t *)listPtr, 0);
    return 1;
}

scroll_start_inc: {
    int newStart = *(int *)listPtr + 1;
    ListBox_SetStartPos((itemDef_t *)listPtr, newStart);
    if (max < *(int *)listPtr)
        ListBox_SetStartPos((itemDef_t *)listPtr, max);
    return 1;
}

scroll_start_pgup: {
    int newStart = *(int *)listPtr - viewmax;
    ListBox_SetStartPos((itemDef_t *)listPtr, newStart);
    if (*(int *)listPtr < 0)
        ListBox_SetStartPos((itemDef_t *)listPtr, 0);
    return 1;
}

scroll_start_pgdn: {
    int newStart = *(int *)listPtr + viewmax;
    ListBox_SetStartPos((itemDef_t *)listPtr, newStart);
    if (max < *(int *)listPtr)
        ListBox_SetStartPos((itemDef_t *)listPtr, max);
    return 1;
}

do_feeder_selection:
    UI_FeederSelection(((itemDef_t *)it)->special, ((itemDef_t *)it)->cursorPos[0]);
    return 1;
}

static void Scroll_ListBox_ThumbFunc(displayContextDef_t *dc, void *p)
{
    byte *d = (byte *)dc;
    byte *scroll = (byte *)p;
    itemDef_t *scrollItem = *(itemDef_t **)(scroll + 0x18);
    byte *it = (byte *)scrollItem;
    byte *listPtr = (byte *)Item_GetListBoxDef(scrollItem);

    if (!listPtr)
        return;

    if ((((Window *)it)->staticFlags & 0x200000)) {

        float cursorX = (float)((displayContextDef_t *)d)->cursorx;
        if (cursorX == (((scrollInfo_s*)(scroll))->xStart))
            goto check_auto;

        float startX = (((rectDef_t*)(it))->x) + 16.0f + 1.0f;
        float scrollArea = (((rectDef_t*)(it))->w) - 32.0f - 2.0f;
        int maxScroll = Item_ListBox_MaxScroll(scrollItem);
        float thumbPos = (cursorX - startX - 8.0f) * (float)maxScroll / (scrollArea - 16.0f);
        int newStart = (int)thumbPos;
        if (newStart < 0)
            newStart = 0;
        else if (newStart > maxScroll)
            newStart = maxScroll;
        ListBox_SetStartPos((itemDef_t *)listPtr, newStart);
        (((scrollInfo_s*)(scroll))->xStart) = cursorX;
    } else {

        float cursorY = (float)(((displayContextDef_s*)(d))->cursory);
        if (cursorY == (((scrollInfo_s*)(scroll))->yStart))
            goto check_auto;

        float startY = (((rectDef_t*)(it))->y) + 16.0f + 1.0f;
        float scrollArea = (((rectDef_t*)(it))->h) - 32.0f - 2.0f;
        int maxScroll = Item_ListBox_MaxScroll(scrollItem);
        float thumbPos = (cursorY - startY - 8.0f) * (float)maxScroll / (scrollArea - 16.0f);
        int newStart = (int)thumbPos;
        if (newStart < 0)
            newStart = 0;
        else if (newStart > maxScroll)
            newStart = maxScroll;
        ListBox_SetStartPos((itemDef_t *)listPtr, newStart);
        (((scrollInfo_s*)(scroll))->yStart) = cursorY;
    }

check_auto:;

    int curTime = (((displayContextDef_s*)(d))->realTime);
    while (curTime > (((scrollInfo_s*)(scroll))->nextScrollTime)) {
        Item_ListBox_HandleKey(dc, (((scrollInfo_s*)(scroll))->item), (((scrollInfo_s*)(scroll))->scrollKey), 1, 0);
        (((scrollInfo_s*)(scroll))->nextScrollTime) = curTime + (((scrollInfo_s*)(scroll))->adjustValue);
    }

    if (curTime > (((scrollInfo_s*)(scroll))->nextAdjustTime)) {
        (((scrollInfo_s*)(scroll))->nextAdjustTime) = curTime + 150;
        if ((((scrollInfo_s*)(scroll))->adjustValue) > 20)
            (((scrollInfo_s*)(scroll))->adjustValue) -= 40;
    }
}

static void Scroll_ListBox_AutoFunc(displayContextDef_t *dc, void *p)
{
    byte *d = (byte *)dc;
    byte *scroll = (byte *)p;
    int curTime = (((displayContextDef_s*)(d))->realTime);

    while (curTime > (((scrollInfo_s*)(scroll))->nextScrollTime)) {
        Item_ListBox_HandleKey(dc, (((scrollInfo_s*)(scroll))->item), (((scrollInfo_s*)(scroll))->scrollKey), 1, 0);
        (((scrollInfo_s*)(scroll))->nextScrollTime) = curTime + (((scrollInfo_s*)(scroll))->adjustValue);
    }

    if (curTime > (((scrollInfo_s*)(scroll))->nextAdjustTime)) {
        (((scrollInfo_s*)(scroll))->nextAdjustTime) = curTime + 150;
        if ((((scrollInfo_s*)(scroll))->adjustValue) > 20)
            (((scrollInfo_s*)(scroll))->adjustValue) -= 40;
    }
}

extern const char str_002157b8[];
qboolean Item_Bind_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key, qboolean down)
{
    byte *it = (byte *)item;
    int i;

    if (!g_waitingForKey) {

        if (!down)
            return 0;
        if (key == 0xd)
            goto enter_bind;
        if (key == 0xc8) {

            byte *d = (byte *)dc;
            float cx = (float)((displayContextDef_t *)d)->cursorx;
            float cy = (float)(((displayContextDef_s*)(d))->cursory);
            float rx = (((rectDef_t*)(it))->x), ry = (((rectDef_t*)(it))->y);
            float rw = (((rectDef_t*)(it))->w), rh = (((rectDef_t*)(it))->h);
            CalcScreenX(&cx, 4);
            CalcScreenY(&cy, 4);
            CalcScreenPlacement(&rx, &ry, &rw, &rh, (((rectDef_t*)(it))->horzAlign), (((rectDef_t*)(it))->vertAlign));
            if (cx < rx || cx > rx + rw || cy < ry || cy > ry + rh)
                return 0;
        enter_bind:
            g_waitingForKey = 1;
            g_bindItem = item;
            return 1;
        }
        return 0;
    }

    if (!g_bindItem)
        return 0;

    if ((key & 0x400) || key == 0x60)
        return 1;

    if (key == 0x7f) {
        const char *dvarName = item->dvar;

        for (i = 0; i < 0x38; i++) {
            if (I_stricmp(dvarName, *(const char **)((byte *)g_bindings + i * 0x14)) == 0)
                break;
        }
        if (i < 0x38) {

            int idx = i * 0x14;
            int key1 = *(int *)((byte *)g_bindings + idx + 12);
            if (key1 != -1) {
                Key_SetBinding(key1, "");
                *(int *)((byte *)g_bindings + idx + 12) = -1;
            }
            int key2 = *(int *)((byte *)g_bindings + idx + 16);
            if (key2 != -1) {
                Key_SetBinding(key2, "");
                *(int *)((byte *)g_bindings + idx + 16) = -1;
            }
        }
        Controls_SetConfig(0);
        g_waitingForKey = 0;
        g_bindItem = NULL;
        return 1;
    }

    if (key == 0x1b) {
        g_waitingForKey = 0;
        g_bindItem = NULL;
        return 1;
    }

    if (key != -1) {

        byte *entry = (byte *)g_bindings;
        while (entry != (byte *)g_bindings + 56 * 0x14) {
            if ((((bind_t*)(entry))->bind2) == key) {
                (((bind_t*)(entry))->bind2) = -1;
            }
            if ((((bind_t*)(entry))->bind1) == key) {

                (((bind_t*)(entry))->bind1) = (((bind_t*)(entry))->bind2);
                (((bind_t*)(entry))->bind2) = -1;
            }
            entry += 0x14;
        }
    }

    const char *bindName = item->dvar;
    int bindIdx = -1;
    for (i = 0; i < 0x38; i++) {
        if (I_stricmp(bindName, *(const char **)((byte *)g_bindings + i * 0x14)) == 0) {
            bindIdx = i;
            break;
        }
    }

    if (bindIdx >= 0 && key != -1) {
        int offset = bindIdx * 0x14;
        if (*(int *)((byte *)g_bindings + offset + 12) == -1) {
            *(int *)((byte *)g_bindings + offset + 12) = key;
        } else {
            *(int *)((byte *)g_bindings + offset + 16) = key;
        }
    }

    Controls_SetConfig(0);
    g_waitingForKey = 0;
    g_bindItem = NULL;
    return 1;
}

static int Item_HandleKey_RectContainsPoint(byte *it, float cx, float cy)
{
    float rx = (((rectDef_t*)(it))->x), ry = (((rectDef_t*)(it))->y);
    float rw = (((rectDef_t*)(it))->w), rh = (((rectDef_t*)(it))->h);
    CalcScreenX(&cx, 4);
    CalcScreenY(&cy, 4);
    CalcScreenPlacement(&rx, &ry, &rw, &rh, (((rectDef_t*)(it))->horzAlign), (((rectDef_t*)(it))->vertAlign));
    if (cx >= rx && rx + rw >= cx && cy >= ry && ry + rh >= cy)
        return 1;
    return 0;
}

qboolean Item_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key, qboolean down)
{
    byte *it = (byte *)item;
    byte *d = (byte *)dc;

    if (itemCapture) {
        itemCapture = NULL;
        captureFunc = NULL;
        captureData = NULL;
    } else {

        if (!down)
            return 0;
        if (key >= 0xc8 && key <= 0xca) {
            int itemType = (((itemDef_s*)(it))->type);
            if (itemType == 6) {

                float lbx = (float)((displayContextDef_t *)d)->cursorx;
                float lby = (float)(((displayContextDef_s*)(d))->cursory);
                int overLB = Item_ListBox_OverLB(item, lbx, lby);
                if (overLB & 0x300) {

                    scrollInfo.nextScrollTime = (((displayContextDef_s*)(d))->realTime) + 500;
                    scrollInfo.nextAdjustTime = (((displayContextDef_s*)(d))->realTime) + 150;
                    scrollInfo.adjustValue = 500;
                    scrollInfo.scrollKey = key;
                    scrollInfo.scrollDir = (overLB >> 8) & 1;
                    scrollInfo.item = item;
                    captureData = &scrollInfo;
                    captureFunc = Scroll_ListBox_AutoFunc;
                    itemCapture = item;
                } else if (overLB & 0x400) {

                    scrollInfo.scrollKey = key;
                    scrollInfo.item = item;
                    scrollInfo.xStart = (float)((displayContextDef_t *)d)->cursorx;
                    scrollInfo.yStart = (float)(((displayContextDef_s*)(d))->cursory);
                    captureData = &scrollInfo;
                    captureFunc = Scroll_ListBox_ThumbFunc;
                    itemCapture = item;
                }

            } else if (itemType == 0xa) {

                float sy = (float)(((displayContextDef_s*)(d))->cursory);
                float sx = (float)((displayContextDef_t *)d)->cursorx;
                float thumbX = Item_Slider_ThumbPosition(item);
                float itemY = (((rectDef_t*)(it))->y) - 2.0f;
                int horzAlign = (((rectDef_t*)(it))->horzAlign);
                int vertAlign = (((rectDef_t*)(it))->vertAlign);

                float rx = thumbX - 5.0f, ry = itemY;
                float rw = 10.0f, rh = 20.0f;
                float cx = sx, cy = sy;
                CalcScreenX(&cx, 4);
                CalcScreenY(&cy, 4);
                CalcScreenPlacement(&rx, &ry, &rw, &rh, horzAlign, vertAlign);
                int hit = (cx >= rx && rx + rw >= cx && cy >= ry && ry + rh >= cy) ? 0x400 : 0;

                if (hit) {
                    scrollInfo.scrollKey = key;
                    scrollInfo.item = item;
                    scrollInfo.xStart = (float)((displayContextDef_t *)d)->cursorx;
                    scrollInfo.yStart = (float)(((displayContextDef_s*)(d))->cursory);
                    captureData = &scrollInfo;
                    captureFunc = Scroll_Slider_ThumbFunc;
                    itemCapture = item;
                }

            }
        }
    }

    if (!down)
        return 0;

    switch ((((itemDef_s*)(it))->type)) {
    case 6:
        return Item_ListBox_HandleKey(dc, item, key, down, 0);

    case 8:
        return UI_OwnerDrawHandleKey(((itemDef_t *)it)->window.ownerDraw, ((itemDef_t *)it)->window.ownerDrawFlags,
                                     (int *)(((char *)it + offsetof(itemDef_t, special))), key);

    case 0xa:
        return Item_Slider_HandleKey(dc, item, key, down);

    case 0xb:
        return Item_YesNo_HandleKey(dc, item, key);

    case 0xc: {
        byte *multiPtr = (byte *)Item_GetMultiDef(item);
        if (!multiPtr)
            return 0;
        if (!((*(byte *)&((itemDef_t *)it)->window.dynamicFlags[0]) & 4))
            return 0;
        if (!((*(byte *)&((itemDef_t *)it)->window.dynamicFlags[0]) & 2))
            return 0;

        if (key >= 0xc8 && key <= 0xca) {
            if (!Item_HandleKey_RectContainsPoint(it, (float)((displayContextDef_t *)d)->cursorx, (float)(((displayContextDef_s*)(d))->cursory)))
                return 0;
        }

        byte *mDef = (byte *)Item_GetMultiDef(item);
        int current = 0;
        if (mDef) {
            int strDef = (((multiDef_s*)(mDef))->strDef);
            if (!strDef) {

                const char *valStr = Dvar_GetVariantString(item->dvar);
                float curVal = (float)atof(valStr);
                int count = (((multiDef_s*)(mDef))->count);
                if (count > 0) {

                    if (curVal != (((multiDef_s*)(mDef))->dvarValue[0])) {
                        int i;
                        for (i = 1; i < count; i++) {
                            if (*(float *)(mDef + 0x100 + i * 4) == curVal) {
                                current = i;
                                break;
                            }
                        }

                    }
                }
            } else {

                const char *string = Dvar_GetVariantString(item->dvar);
                int count = (((multiDef_s*)(mDef))->count);
                if (count > 0) {
                    int i;
                    for (i = 0; i < count; i++) {
                        if (I_stricmp(string, *(const char **)(mDef + 0x80 + i * 4)) == 0) {
                            current = i;
                            break;
                        }
                    }
                }
            }
        }

        byte *mDef2 = (byte *)Item_GetMultiDef(item);
        int totalCount;
        if (!mDef2)
            totalCount = 0;
        else
            totalCount = (((multiDef_s*)(mDef2))->count);

        int newIndex;
        if (!totalCount) {
            newIndex = 0;
        } else if (key == 0xc8 || key == 0xd || key == 0xca || key == 0xa3 || key == 0x9d) {

            newIndex = (current + 1) % totalCount;
        } else if (key == 0xc9 || key == 0xa4 || key == 0x9c) {

            newIndex = (current - 1 + totalCount) % totalCount;
        } else {
            return 0;
        }

        if (newIndex == current)
            return 0;

        const char *newVal;
        if ((((multiDef_s*)(multiPtr))->strDef)) {

            newVal = *(const char **)(multiPtr + 0x80 + newIndex * 4);
        } else {

            newVal = va("%g", (double)(((multiDef_s*)(multiPtr))->dvarValue[newIndex]));
        }
        Dvar_SetFromStringByName(item->dvar, newVal);
        return 1;
    }

    case 0xd: {
        if (!((*(byte *)&((itemDef_t *)it)->window.dynamicFlags[0]) & 4))
            return 0;
        if (!((*(byte *)&((itemDef_t *)it)->window.dynamicFlags[0]) & 2))
            return 0;

        if (key >= 0xc8 && key <= 0xca) {
            if (!Item_HandleKey_RectContainsPoint(it, (float)((displayContextDef_t *)d)->cursorx, (float)(((displayContextDef_s*)(d))->cursory)))
                return 0;
        }

        struct dvar_s *enumDvar = Dvar_FindVar((*(const char **)&((itemDef_t *)it)->typeData.listBox));
        int current = 0;
        if (enumDvar->type == 6) {

            const char *enumString = Dvar_GetVariantString(item->dvar);
            current = atoi(enumString);
            if (current < 0 || current >= enumDvar->domain.enumeration.stringCount) {

                int numStrings = enumDvar->domain.enumeration.stringCount;
                current = 0;
                if (numStrings > 0) {
                    const char **strings = enumDvar->domain.enumeration.strings;
                    int i;
                    for (i = 0; i < numStrings; i++) {
                        if (I_stricmp(enumString, strings[i]) == 0) {
                            current = i;
                            break;
                        }
                    }
                }
            }
        }

        struct dvar_s *dv = Dvar_FindVar((*(const char **)&((itemDef_t *)it)->typeData.listBox));
        int totalCount;
        if (dv->type == 6)
            totalCount = *(int *)((byte *)dv + 0x14);
        else
            totalCount = 0;

        int newIndex;
        if (!totalCount) {
            newIndex = 0;
        } else if (key == 0xc8 || key == 0xd || key == 0xca || key == 0xa3 || key == 0x9d) {

            newIndex = (current + 1) % totalCount;
        } else if (key == 0xc9 || key == 0xa4 || key == 0x9c) {

            newIndex = (current - 1 + totalCount) % totalCount;
        } else {
            return 0;
        }

        if (newIndex == current)
            return 0;

        Dvar_SetFromStringByName(item->dvar, va("%i", newIndex));
        return 1;
    }

    case 0xe:
        return Item_Bind_HandleKey(dc, item, key, down);

    default:
        return 0;
    }
}

void Item_ListBox_MouseEnter(itemDef_t *item, float x, float y)
{
    byte *it = (byte *)item;
    byte *listPtr = (byte *)Item_GetListBoxDef(item);
    if (!listPtr)
        return;
    Window_RemoveDynamicFlags((void *)it, 0x1f00);
    int overLB = Item_ListBox_OverLB(item, x, y);
    Window_AddDynamicFlags((void *)it, overLB);
    int horzAlign = (((rectDef_t*)(it))->horzAlign);
    int vertAlign = (((rectDef_t*)(it))->vertAlign);
    if ((((Window *)it)->staticFlags & 0x200000)) {
        if (((itemDef_t *)it)->window.dynamicFlags[0] & 0x1f00)
            return;
        if (((listBoxDef_t *)listPtr)->elementStyle == 1) {
            float itemX = (((rectDef_t*)(it))->x), itemY = (((rectDef_t*)(it))->y);
            float rectW = (((rectDef_t*)(it))->h) - 16.0f;
            int headerH = (((listBoxDef_s*)(listPtr))->drawPadding);
            float rectH = (((rectDef_t*)(it))->w) - (float)headerH;
            float cx = x, cy = y;
            float rx = itemX, ry = itemY, rw = rectH, rh = rectW;
            CalcScreenX(&cx, 4);
            CalcScreenY(&cy, 4);
            CalcScreenPlacement(&rx, &ry, &rw, &rh, horzAlign, vertAlign);
            if (cx < rx || cx > rx + rw || cy < ry || cy > ry + rh)
                return;
            int cursorPos = (int)((x - itemX) / (((listBoxDef_s*)(listPtr))->elementWidth)) + *(int *)listPtr;
            ListBox_SetCursorPos((itemDef_t *)listPtr, cursorPos);
            if ((((listBoxDef_s*)(listPtr))->cursorPos[0]) >= (((listBoxDef_s*)(listPtr))->endPos[0]))
                ListBox_SetCursorPos((itemDef_t *)listPtr, (((listBoxDef_s*)(listPtr))->endPos[0]));
        }
    } else {
        if (((itemDef_t *)it)->window.dynamicFlags[0] & 0x1f00)
            return;
        float itemY = (((rectDef_t*)(it))->y);
        float rectW = (((rectDef_t*)(it))->w) - 16.0f;
        int headerH = (((listBoxDef_s*)(listPtr))->drawPadding);
        float rectH = (((rectDef_t*)(it))->h) - (float)headerH;
        float cx = x, cy = y;
        float rx = (((rectDef_t*)(it))->x), ry = itemY, rw = rectW, rh = rectH;
        CalcScreenX(&cx, 4);
        CalcScreenY(&cy, 4);
        CalcScreenPlacement(&rx, &ry, &rw, &rh, horzAlign, vertAlign);
        if (cx < rx || cx > rx + rw || cy < ry || cy > ry + rh)
            return;
        int cursorPos = (int)((y - 2.0f - itemY) / (((listBoxDef_s*)(listPtr))->elementHeight)) + *(int *)listPtr;
        ListBox_SetCursorPos((itemDef_t *)listPtr, cursorPos);
        if ((((listBoxDef_s*)(listPtr))->cursorPos[0]) > (((listBoxDef_s*)(listPtr))->endPos[0]))
            ListBox_SetCursorPos((itemDef_t *)listPtr, (((listBoxDef_s*)(listPtr))->endPos[0]));
    }
}

void Item_MouseEnter(displayContextDef_t *dc, itemDef_t *item, float x, float y)
{
    byte *it = (byte *)item;
    if (!item)
        return;
    byte *parent = (byte *)item->parent;
    if (!parent)
        return;
    int dvarFlags = ((itemDef_t *)it)->dvarFlags;
    if (dvarFlags & 3) {
        if (!Item_EnableShowViaDvar((itemDef_t *)it, 1))
            return;
        dvarFlags = ((itemDef_t *)it)->dvarFlags;
    }
    if (dvarFlags & 0xc) {
        if (!Item_EnableShowViaDvar((itemDef_t *)it, 4))
            return;
    }
    int flags = ((itemDef_t *)it)->window.dynamicFlags[0];
    float rx = (((rectDef_t*)(it))->x), ry = (((rectDef_t*)(it))->y);
    float rw = (((rectDef_t*)(it))->w), rh = (((rectDef_t*)(it))->h);
    float cx = x, cy = y;
    CalcScreenX(&cx, 4);
    CalcScreenY(&cy, 4);
    CalcScreenPlacement(&rx, &ry, &rw, &rh, (((rectDef_t*)(it))->horzAlign), (((rectDef_t*)(it))->vertAlign));
    qboolean inside = (cx >= rx && cx <= rx + rw && cy >= ry && cy <= ry + rh);
    if (inside) {
        if (!(flags & 0x40)) {
            Item_RunScript(dc, item, ((itemDef_t *)it)->mouseEnterText);
            Window_AddDynamicFlags((void *)it, 0x40);
        }
        if (!(flags & 1)) {
            Item_RunScript(dc, item, ((itemDef_t *)it)->mouseEnter);
            Window_AddDynamicFlags((void *)it, 1);
        }
    } else {
        if (flags & 0x40) {
            Item_RunScript(dc, item, ((itemDef_t *)it)->mouseExitText);
            Window_RemoveDynamicFlags((void *)it, 0x40);
        }
        if (!(flags & 1)) {
            Item_RunScript(dc, item, ((itemDef_t *)it)->mouseEnter);
            Window_AddDynamicFlags((void *)it, 1);
        }
    }
    if ((((itemDef_s*)(it))->type) == 6)
        Item_ListBox_MouseEnter(item, x, y);
}

static qboolean Rect_ContainsPoint(byte *item, float x, float y)
{
    float rx = (((rectDef_t*)(item))->x), ry = (((rectDef_t*)(item))->y);
    float rw = (((rectDef_t*)(item))->w), rh = ((itemDef_t *)item)->window.rect[0].h;
    float cx = x, cy = y;
    CalcScreenX(&cx, 4);
    CalcScreenY(&cy, 4);
    CalcScreenPlacement(&rx, &ry, &rw, &rh, (((rectDef_t*)(item))->horzAlign), (((rectDef_t*)(item))->vertAlign));
    return (cx >= rx && cx <= rx + rw && cy >= ry && cy <= ry + rh);
}

qboolean Menu_HandleMouseMove(displayContextDef_t *dc, menuDef_t *menu, float x, float y)
{
    byte *m = (byte *)menu;
    int i, pass;
    qboolean focusSet = 0;
    void *focusItem = NULL;

    if (!menu)
        return 0;
    if ((((menuDef_t *)m)->window.dynamicFlags[0] & 0x4004) == 0)
        return 0;
    if (itemCapture)
        return 0;
    if (g_waitingForKey || g_editingField)
        return 0;

    for (pass = 0; pass < 2; pass++) {
        int itemCount = (((menuDef_t*)(m))->itemCount);
        for (i = itemCount - 1; i >= 0; i--) {
            byte *it = *(byte **)((char *)&((menuDef_t *)m)->items[i]);

            if ((((itemDef_t *)it)->window.dynamicFlags[0] & 0x4004) == 0)
                goto next_item;

            if ((*(byte *)&((itemDef_t *)it)->dvarFlags) & 3) {
                if (!Item_EnableShowViaDvar((itemDef_t *)it, 1))
                    goto next_item;
                it = *(byte **)((char *)&((menuDef_t *)m)->items[i]);
            }
            if ((*(byte *)&((itemDef_t *)it)->dvarFlags) & 0xc) {
                if (!Item_EnableShowViaDvar((itemDef_t *)it, 4))
                    goto next_item;
                it = *(byte **)((char *)&((menuDef_t *)m)->items[i]);
            }

            byte *overItem = it;
            int flags = ((itemDef_t *)overItem)->window.dynamicFlags[0];
            if ((flags & 4) && (flags & 2) && !focusItem)
                focusItem = overItem;

            if (!Rect_ContainsPoint(overItem, x, y)) {
                byte *orig = *(byte **)((char *)&((menuDef_t *)m)->items[i]);
                if ((((Window *)orig)->dynamicFlags[0] & 1)) {
                    Item_MouseLeave(dc, (itemDef_t *)orig);
                    orig = *(byte **)((char *)&((menuDef_t *)m)->items[i]);
                    if (orig)
                        Window_RemoveDynamicFlags((void *)orig, 1);
                }
                goto next_item;
            }

            if (pass == 1) {
                overItem = *(byte **)((char *)&((menuDef_t *)m)->items[i]);
                int itemType = (((itemDef_s*)(overItem))->type);
                if (itemType == 0 && (*(int *)&((itemDef_t *)overItem)->text)) {
                    int textBuf[6];
                    memset(textBuf, 0, 24);
                    textBuf[0] = (*(int*)&((itemDef_s*)(overItem))->textRect[0].x);
                    textBuf[1] = (*(int*)&((itemDef_s*)(overItem))->textRect[0].y);
                    textBuf[2] = (*(int*)&((itemDef_s*)(overItem))->textRect[0].w);
                    textBuf[3] = (*(int*)&((itemDef_s*)(overItem))->textRect[0].h);
                    textBuf[4] = (((itemDef_s*)(overItem))->textRect[0].horzAlign);
                    textBuf[5] = (((itemDef_s*)(overItem))->textRect[0].vertAlign);
                    if (*(float *)&textBuf[2] != 0.0f)
                        *(float *)&textBuf[1] -= *(float *)&textBuf[3];
                    float trx = *(float *)&textBuf[0], try_ = *(float *)&textBuf[1];
                    float trw = *(float *)&textBuf[2], trh = *(float *)&textBuf[3];
                    float tcx = x, tcy = y;
                    CalcScreenX(&tcx, 4);
                    CalcScreenY(&tcy, 4);
                    CalcScreenPlacement(&trx, &try_, &trw, &trh, textBuf[4], textBuf[5]);
                    if (tcx < trx || tcx > trx + trw || tcy < try_ || tcy > try_ + trh)
                        goto next_item;
                }

                if (!((*(byte *)&((itemDef_t *)overItem)->window.dynamicFlags[0]) & 4))
                    goto next_item;
                if ((*(byte *)&((itemDef_t *)overItem)->window.dynamicFlags[0]) & 0x10)
                    goto next_item;

                Item_MouseEnter(dc, (itemDef_t *)overItem, x, y);
                if (!focusSet) {
                    if (Item_SetFocus(dc, (itemDef_t *)overItem, x, y)) {
                        focusSet = 1;
                        focusItem = overItem;
                    }
                }
            }
        next_item:;
        }
    }

    if (focusSet)
        return 1;

    if (focusItem) {
        if (!Rect_ContainsPoint((byte *)focusItem, x, y)) {
            int itemCount = (((menuDef_t*)(m))->itemCount);
            for (i = 0; i < itemCount; i++) {
                byte *it = *(byte **)((char *)&((menuDef_t *)m)->items[i]);
                Window_RemoveDynamicFlags((void *)it, 2);
                if ((*(void **)&((itemDef_t *)it)->leaveFocus))
                    Item_RunScript(dc, (itemDef_t *)it, ((itemDef_t *)it)->leaveFocus);
            }
        }
    }

    return 0;
}

extern int ___toupper(int c);
qboolean Item_TextField_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key)
{
    byte *it = (byte *)item;
    char buff[0x400];
    int len;
    int cursorPos;

    editFieldDef_t *editPtr = Item_GetEditFieldDef(item);
    if (!editPtr)
        return 0;
    if (!(*(void **)&((itemDef_t *)it)->dvar))
        return 0;

    memset(buff, 0, 0x400);
    I_strncpyz(buff, Dvar_GetVariantString(((itemDef_t *)it)->dvar), 0x400);

    len = 0;
    while (buff[len])
        len++;

    int maxChars = editPtr->maxChars;
    if (maxChars && len > maxChars) {
        buff[maxChars] = '\0';
        len = maxChars;
    }

    if (key & 0x400) {
        key &= ~0x400;

        if (key == 8) {
            cursorPos = ((itemDef_t *)it)->cursorPos[0];
            if (cursorPos > 0)
                memmove(buff + cursorPos - 1, buff + cursorPos, len - cursorPos + 1);
            Dvar_SetFromStringByName(((itemDef_t *)it)->dvar, buff);
            I_strncpyz(buff, Dvar_GetVariantString(((itemDef_t *)it)->dvar), 0x400);
            Item_SetCursorPos(item, Item_GetCursorPosOffset(item, buff, -1));
            goto scroll_update;
        }

        if ((((itemDef_s*)(it))->type) == 0x10) {
            if (!I_isforfilename(key))
                return 1;
        }

        if (key <= 0x1f)
            return 1;
        if (!(*(void **)&((itemDef_t *)it)->dvar))
            return 1;

        if ((((itemDef_s*)(it))->type) == 9) {

            if (!I_isdigit(key))
                return 0;
        } else if ((((itemDef_s*)(it))->type) == 0x11) {

            if (!I_isdigit(key)) {
                if (key != (int)(signed char)Com_GetDecimalDelimiter())
                    return 0;
            }
            if (key == (int)(signed char)Com_GetDecimalDelimiter())
                key = '.';
        }

        if ((((itemDef_s*)(it))->type) == 0x12) {

            key = ___toupper(key);
        }

        if (Key_GetOverstrikeMode()) {

            maxChars = editPtr->maxChars;
            if (maxChars && maxChars <= ((itemDef_t *)it)->cursorPos[0]) {
                if (editPtr->maxCharsGotoNext == 0)
                    return 1;
                itemDef_t *nextItem = Menu_SetNextCursorItem(dc, ((itemDef_t *)it)->parent);
                if (nextItem && (unsigned int)((itemDef_s *)nextItem)->type <= 0x12 &&
                    ((1 << ((itemDef_s *)nextItem)->type) & 0x70210))
                    g_editItem = nextItem;
                return 1;
            }
            goto write_char;
        }

        if (len == 0xff)
            return 1;
        maxChars = editPtr->maxChars;
        if (maxChars && len >= maxChars)
            return 1;
        cursorPos = ((itemDef_t *)it)->cursorPos[0];
        memmove(buff + cursorPos + 1, buff + cursorPos, len - cursorPos + 1);

    write_char:
        buff[((itemDef_t *)it)->cursorPos[0]] = (char)key;
        Dvar_SetFromStringByName(((itemDef_t *)it)->dvar, buff);
        I_strncpyz(buff, Dvar_GetVariantString(((itemDef_t *)it)->dvar), 0x400);
        Item_SetCursorPos(item, Item_GetCursorPosOffset(item, buff, 1));

        editPtr = Item_GetEditFieldDef(item);
        cursorPos = ((itemDef_t *)it)->cursorPos[0];
        if (cursorPos < editPtr->paintOffset) {
            editPtr->paintOffset = cursorPos;
        } else if (editPtr->maxPaintChars) {
            int off = Item_GetCursorPosOffset(item, buff, -editPtr->maxPaintChars);
            if (off > editPtr->paintOffset)
                editPtr->paintOffset = off;
        }

        maxChars = editPtr->maxChars;
        if (maxChars == 0)
            return 1;
        if (maxChars > ((itemDef_t *)it)->cursorPos[0])
            return 1;
        if (editPtr->maxCharsGotoNext == 0)
            return 1;
        {
            itemDef_t *nextItem = Menu_SetNextCursorItem(dc, ((itemDef_t *)it)->parent);
            Item_SetCursorPos(nextItem, 0);
            if (nextItem && (unsigned int)((itemDef_s *)nextItem)->type <= 0x12 &&
                ((1 << ((itemDef_s *)nextItem)->type) & 0x70210))
                g_editItem = nextItem;
        }
        return 1;
    }

    switch (key) {
    case 0xa2:
        cursorPos = ((itemDef_t *)it)->cursorPos[0];
        if (len > cursorPos) {
            memmove(buff + cursorPos, buff + cursorPos + 1, len - cursorPos);
            Dvar_SetFromStringByName(((itemDef_t *)it)->dvar, buff);
        }
        return 1;

    case 0x9d:
        Item_SetCursorPos(item, Item_GetCursorPosOffset(item, buff, 1));
        goto scroll_update;

    case 0x9c:
        Item_SetCursorPos(item, Item_GetCursorPosOffset(item, buff, -1));
    scroll_update:
        editPtr = Item_GetEditFieldDef(item);
        cursorPos = ((itemDef_t *)it)->cursorPos[0];
        if (cursorPos < editPtr->paintOffset) {
            editPtr->paintOffset = cursorPos;
            return 1;
        }
        if (editPtr->maxPaintChars == 0)
            return 1;
        {
            int off = Item_GetCursorPosOffset(item, buff, -editPtr->maxPaintChars);
            if (off > editPtr->paintOffset)
                editPtr->paintOffset = off;
        }
        return 1;

    case 0xa5:
        Item_SetCursorPos(item, 0);
        editPtr->paintOffset = 0;
        return 1;

    case 0xa6:
        Item_SetCursorPos(item, len);
        goto scroll_update;

    case 0xa1:
        Key_SetOverstrikeMode(Key_GetOverstrikeMode() == 0);
        return 1;

    case 9:
    case 0x9b:
    {
        itemDef_t *nextItem = Menu_SetNextCursorItem(dc, ((itemDef_t *)it)->parent);
        if (nextItem && (unsigned int)((itemDef_s *)nextItem)->type <= 0x12 &&
            ((1 << ((itemDef_s *)nextItem)->type) & 0x70210))
            g_editItem = nextItem;
    }
        return 1;

    case 0x9a:
    {
        itemDef_t *prevItem = Menu_SetPrevCursorItem(dc, ((itemDef_t *)it)->parent);
        if (prevItem && (unsigned int)((itemDef_s *)prevItem)->type <= 0x12 &&
            ((1 << ((itemDef_s *)prevItem)->type) & 0x70210))
            g_editItem = prevItem;
    }
        return 1;

    case 0xd:
    case 0xbf:
        if ((*(void **)&((itemDef_t *)it)->onAccept))
            Item_RunScript(dc, item, ((itemDef_t *)it)->onAccept);
        return 0;

    case 0x1b:
        return 0;
    }

    return 1;
}

extern int inHandleKey;
void Menu_HandleKey(displayContextDef_t *dc, menuDef_t *menu, int key, qboolean down)
{
    byte *m = (byte *)menu;
    byte *d = (byte *)dc;
    void *focusedItem = NULL;
    int i;

    if (g_waitingForKey) {
        if (down) {
            Item_Bind_HandleKey(dc, g_bindItem, key, down);
            return;
        }
    }

    if (g_editingField) {
        if (down) {
            if (key >= 0xc8 && key <= 0xca) {
                /* Commit the field, then route this same click to its target.
                   Otherwise changing settings requires a second click. */
                g_editingField = 0;
                g_editItem = NULL;
                Display_MouseMove(dc, NULL, dc->cursorx, dc->cursory);
            } else {
                if (!Item_TextField_HandleKey(dc, g_editItem, key)) {
                    g_editingField = 0;
                    g_editItem = NULL;
                }
                return;
            }
        }
    }

    if (!menu)
        return;

    if (down) {
        if (!((((Window *)m)->staticFlags & 0x1000000)) && !(((menuDef_t*)(m))->fullScreen)) {

            float cx = (float)((displayContextDef_t *)d)->cursorx, cy = (float)(((displayContextDef_s*)(d))->cursory);
            float rx = *(float *)m, ry = (((rectDef_t*)(m))->y);
            float rw = (((rectDef_t*)(m))->w), rh = ((menuDef_t *)m)->window.rect[0].h;
            CalcScreenX(&cx, 4);
            CalcScreenY(&cy, 4);
            CalcScreenPlacement(&rx, &ry, &rw, &rh, (((rectDef_t*)(m))->horzAlign), (((rectDef_t*)(m))->vertAlign));
            int inside = (cx >= rx && cx <= rx + rw && cy >= ry && cy <= ry + rh);
            if (!inside) {

                if (!inHandleKey && key >= 0xc8 && key <= 0xca) {
                    inHandleKey = 1;
                    Menus_HandleOOBClick(dc, menu, key, down);
                    inHandleKey = 0;
                    return;
                }
            }
        }
    }

    int itemCount = (((menuDef_t*)(m))->itemCount);
    focusedItem = NULL;
    for (i = 0; i < itemCount; i++) {
        byte *it = *(byte **)((char *)&((menuDef_t *)m)->items[i]);
        int flags = ((itemDef_t *)it)->window.dynamicFlags[0];
        if ((flags & 4) && (flags & 2))
            focusedItem = it;
    }

    if (key == 0xcd || key == 0xce) {
        if (focusedItem && ((itemDef_s *)focusedItem)->type == 6) {
            goto dispatch_item;
        }
        return;
    }

    if (focusedItem) {
    dispatch_item:
        if (Item_HandleKey(dc, (itemDef_t *)focusedItem, key, down)) {

            if (focusedItem) {
                byte *fi = (byte *)focusedItem;
                if ((*(void **)&((itemDef_t *)fi)->action))
                    Item_RunScript(dc, (itemDef_t *)focusedItem, ((itemDef_t *)fi)->action);
            }
            return;
        }
    }

    if (!down)
        return;

    if (key >= 1 && key <= 0xff) {
        if (Menu_CheckOnKey(dc, menu, key))
            return;
    }

    if (key == 0xb1) {
        if (Dvar_GetInt("developer"))
            debugMode ^= 1;
        return;
    }

    if (key == 0xb2) {
        if (Dvar_GetInt("developer"))
            Cbuf_ExecuteText(2, "screenshot\n");
        return;
    }

    if (key == 0x1b) {
        if (!g_waitingForKey && (*(void **)&((menuDef_t *)m)->onESC)) {
            byte tempItem[0x2a0];
            *(void **)&(*(unsigned char *)&((itemDef_t *)tempItem)->parent) = menu;
            Item_RunScript(dc, (itemDef_t *)tempItem, ((menuDef_t *)m)->onESC);
        }
        return;
    }

    if (key == 9 || key == 0x9b || key == 0x9d || key == 0x9c) {
        Menu_SetNextCursorItem(dc, menu);
        return;
    }

    if (key == 0x9a) {
        Menu_SetPrevCursorItem(dc, menu);
        return;
    }

    if (key >= 0xc8 && key <= 0xc9) {
        if (!focusedItem)
            return;

        int itemType = ((itemDef_s *)focusedItem)->type;
        if (itemType == 0) {

            byte *fi = (byte *)focusedItem;
            int textBuf[6];
            memset(textBuf, 0, 24);
            textBuf[0] = (*(int*)&((itemDef_s*)(fi))->textRect[0].x);
            textBuf[1] = (*(int*)&((itemDef_s*)(fi))->textRect[0].y);
            textBuf[2] = (*(int*)&((itemDef_s*)(fi))->textRect[0].w);
            textBuf[3] = (*(int*)&((itemDef_s*)(fi))->textRect[0].h);
            textBuf[4] = (((itemDef_s*)(fi))->textRect[0].horzAlign);
            textBuf[5] = (((itemDef_s*)(fi))->textRect[0].vertAlign);
            float tw = *(float *)&textBuf[2];
            float ty = *(float *)&textBuf[1];
            if (tw != 0.0f)
                ty -= *(float *)&textBuf[3];
            float trx = *(float *)&textBuf[0], trw = *(float *)&textBuf[2];
            float try_ = ty, trh = *(float *)&textBuf[3];
            float tcx = (float)((displayContextDef_t *)d)->cursorx, tcy = (float)(((displayContextDef_s*)(d))->cursory);
            CalcScreenX(&tcx, 4);
            CalcScreenY(&tcy, 4);
            CalcScreenPlacement(&trx, &try_, &trw, &trh, textBuf[4], textBuf[5]);
            if (tcx < trx || tcx > trx + trw || tcy < try_ || tcy > try_ + trh)
                return;
        } else {

            byte *fi = (byte *)focusedItem;
            float irx = (((rectDef_t*)(fi))->x), iry = (((rectDef_t*)(fi))->y);
            float irw = (((rectDef_t*)(fi))->w), irh = (((rectDef_t*)(fi))->h);
            float icx = (float)((displayContextDef_t *)d)->cursorx, icy = (float)(((displayContextDef_s*)(d))->cursory);
            CalcScreenX(&icx, 4);
            CalcScreenY(&icy, 4);
            CalcScreenPlacement(&irx, &iry, &irw, &irh, (((rectDef_t*)(fi))->horzAlign), (((rectDef_t*)(fi))->vertAlign));
            if (icx < irx || icx > irx + irw || icy < iry || icy > iry + irh)
                return;

            if (itemType <= 0x12 && ((1 << itemType) & 0x70210)) {
                editFieldDef_t *ep = Item_GetEditFieldDef((itemDef_t *)focusedItem);
                if (ep)
                    ep->paintOffset = 0;
                Item_SetCursorPos((itemDef_t *)focusedItem, 0);
                g_editingField = 1;
                g_editItem = (itemDef_t *)focusedItem;
                Key_SetOverstrikeMode(1);
                return;
            }
        }

        if (((itemDef_s *)focusedItem)->action)
            Item_RunScript(dc, (itemDef_t *)focusedItem, ((itemDef_s *)focusedItem)->action);
        return;
    }

    if (key == 0xd || key == 0xbf || key == 0xca) {
        if (!focusedItem)
            return;

        int ftype = ((itemDef_s *)focusedItem)->type;
        if (ftype <= 0x12 && ((1 << ftype) & 0x70210)) {
            Item_SetCursorPos((itemDef_t *)focusedItem, 0);
            g_editingField = 1;
            g_editItem = (itemDef_t *)focusedItem;
            Key_SetOverstrikeMode(1);
        } else if (focusedItem) {
            if (((itemDef_s *)focusedItem)->action)
                Item_RunScript(dc, (itemDef_t *)focusedItem, ((itemDef_s *)focusedItem)->action);
        }
        return;
    }

    return;
}

void Item_Paint(displayContextDef_t *dc, itemDef_t *item)
{
    byte *it = (byte *)item;
    byte *parent = (*(byte **)&((itemDef_t *)it)->parent);
    int flags;
    rectDef_t newRect;
    int traceScriptItem = 0;

    flags = ((itemDef_t *)it)->window.dynamicFlags[0];
    if (parent && ((menuDef_t *)parent)->window.name &&
        (!I_stricmp(((menuDef_t *)parent)->window.name, "serverinfo_dm") ||
         !strncmp(((menuDef_t *)parent)->window.name, "team_", 5) ||
         !I_stricmp(((menuDef_t *)parent)->window.name, "team"))) {
        static int itemTraceCount;
        if (itemTraceCount < 240) {
            traceScriptItem = 1;
            if (getenv("MTRACE"))
                Com_Printf("[menu-trace] Item_Paint parent=%s item=%s flags=0x%x type=%d owner=0x%x dvarFlags=0x%x dvar=%s enable=%s\n",
                           ((menuDef_t *)parent)->window.name,
                           item->window.name ? item->window.name : "<null>",
                           flags, item->type, item->window.ownerDraw,
                           item->dvarFlags,
                           item->dvar ? item->dvar : "",
                           item->enableDvar ? item->enableDvar : "");
            ++itemTraceCount;
        }
    }

    if (flags & 0x2000) {
        int dcTime = dc->realTime;
        if (dcTime > ((itemDef_t *)it)->window.nextTime) {

            ((itemDef_t *)it)->window.nextTime = dcTime + ((itemDef_t *)it)->window.offsetTime[0];

            float w = (((itemDef_s*)(it))->window.rectClient[0].w) * 0.5f;
            float h = (((itemDef_s*)(it))->window.rectClient[0].h) * 0.5f;

            float rx = (((itemDef_s*)(it))->window.rectClient[0].x) + w - ((itemDef_t *)it)->window.rectEffects0[0].x;
            float ry = (((itemDef_s*)(it))->window.rectClient[0].y) + h - ((itemDef_t *)it)->window.rectEffects0[0].y;

            float angle;
            *(int *)&angle = 0x3d567750;
            float sin_a = sinf(angle);
            float cos_a = cosf(angle);

            newRect.w = (((itemDef_s*)(it))->window.rectClient[0].w);
            newRect.h = (((itemDef_s*)(it))->window.rectClient[0].h);
            newRect.horzAlign = (((itemDef_s*)(it))->window.rectClient[0].horzAlign);
            newRect.vertAlign = (((itemDef_s*)(it))->window.rectClient[0].vertAlign);

            newRect.x = rx * cos_a - ry * sin_a + ((itemDef_t *)it)->window.rectEffects0[0].x - w;

            newRect.y = rx * sin_a + ry * cos_a + ((itemDef_t *)it)->window.rectEffects0[0].y - h;

            Window_SetRectClient(item, &newRect);

            byte *p = (*(byte **)&((itemDef_t *)it)->parent);
            if (p) {
                float px = (((rectDef_t*)(p))->x);
                float py = (((rectDef_t*)(p))->y);
                if (((menuDef_t*)(p))->window.border) {
                    float borderSize = ((menuDef_t*)(p))->window.borderSize;
                    px += borderSize;
                    py += borderSize;
                }
                Item_SetScreenCoords((itemDef_t *)item, px, py, (((rectDef_t*)(p))->horzAlign), (((rectDef_t*)(p))->vertAlign));
            }

            flags = ((itemDef_t *)it)->window.dynamicFlags[0];
        }
    }

    if (flags & 0x80) {
        int dcTime = dc->realTime;
        if (dcTime > ((itemDef_t *)it)->window.nextTime) {

            ((itemDef_t *)it)->window.nextTime = dcTime + ((itemDef_t *)it)->window.offsetTime[0];

            float *target = (float *)(((char *)it + offsetof(itemDef_t, window.rectEffects0[0].x)));
            float *step = (float *)(((char *)it + offsetof(itemDef_t, window.rectEffects1[0].x)));
            int done = 0;

            newRect = (*(rectDef_t *)&((itemDef_t *)it)->window.rectClient[0].x);

            if (newRect.x == target[0]) {
                done++;
            } else if (target[0] > newRect.x) {
                newRect.x += step[0];
                if (newRect.x > target[0]) {
                    newRect.x = target[0];
                    done++;
                }
            } else {
                newRect.x -= step[0];
                if (newRect.x < target[0]) {
                    newRect.x = target[0];
                    done++;
                }
            }

            if (newRect.y == target[1]) {
                done++;
            } else if (target[1] > newRect.y) {
                newRect.y += step[1];
                if (newRect.y > target[1]) {
                    newRect.y = target[1];
                    done++;
                }
            } else {
                newRect.y -= step[1];
                if (newRect.y < target[1]) {
                    newRect.y = target[1];
                    done++;
                }
            }

            if (newRect.w == target[2]) {
                done++;
            } else if (target[2] > newRect.w) {
                newRect.w += step[2];
                if (newRect.w > target[2]) {
                    newRect.w = target[2];
                    done++;
                }
            } else {
                newRect.w -= step[2];
                if (newRect.w < target[2]) {
                    newRect.w = target[2];
                    done++;
                }
            }

            if (newRect.h == target[3]) {
                done++;
            } else if (target[3] > newRect.h) {
                newRect.h += step[3];
                if (newRect.h > target[3]) {
                    newRect.h = target[3];
                    done++;
                }
            } else {
                newRect.h -= step[3];
                if (newRect.h < target[3]) {
                    newRect.h = target[3];
                    done++;
                }
            }

            Window_SetRectClient(item, &newRect);

            byte *p = (*(byte **)&((itemDef_t *)it)->parent);
            if (p) {
                float px = (((rectDef_t*)(p))->x);
                float py = (((rectDef_t*)(p))->y);
                if (((menuDef_t*)(p))->window.border) {
                    float borderSize = ((menuDef_t*)(p))->window.borderSize;
                    px += borderSize;
                    py += borderSize;
                }
                Item_SetScreenCoords((itemDef_t *)item, px, py, (((rectDef_t*)(p))->horzAlign), (((rectDef_t*)(p))->vertAlign));
            }

            if (done == 4) {
                Window_RemoveDynamicFlags(item, 0x80);
            }
        }
    }

    {
        int ownerDraw = ((itemDef_t *)it)->window.ownerDrawFlags;
        if (ownerDraw) {
            if (!UI_OwnerDrawVisible(ownerDraw)) {
                if (traceScriptItem)
                    if (getenv("MTRACE"))
                        Com_Printf("[menu-trace] Item_Paint skip owner parent=%s item=%s owner=0x%x\n",
                                   ((menuDef_t *)parent)->window.name,
                                   item->window.name ? item->window.name : "<null>", ownerDraw);
                Window_RemoveDynamicFlags(item, 4);
            } else {
                Window_AddDynamicFlags(item, 4);
            }
        }
    }

    if ((*(byte *)&((itemDef_t *)it)->dvarFlags) & 0xc) {
        if (!Item_EnableShowViaDvar(item, 4)) {
            if (traceScriptItem)
                if (getenv("MTRACE"))
                    Com_Printf("[menu-trace] Item_Paint skip dvar parent=%s item=%s flagsByte=0x%x enable=%s dvar=%s\n",
                               ((menuDef_t *)parent)->window.name,
                               item->window.name ? item->window.name : "<null>",
                               (*(byte *)&((itemDef_t *)it)->dvarFlags),
                               item->enableDvar ? item->enableDvar : "",
                               item->dvar ? item->dvar : "");

            flags = ((itemDef_t *)it)->window.dynamicFlags[0];
            if ((flags & 4) && (flags & 2)) {
                Menu_HandleKey(dc, (menuDef_t *)parent, 0x9b, 1);
                Menu_HandleKey(dc, (menuDef_t *)parent, 0x9b, 0);
            }
            return;
        }
    }

    if (!((*(byte *)&((itemDef_t *)it)->window.dynamicFlags[0]) & 4)) {
        if (traceScriptItem)
            if (getenv("MTRACE"))
                Com_Printf("[menu-trace] Item_Paint skip invisible parent=%s item=%s flags=0x%x\n",
                           ((menuDef_t *)parent)->window.name,
                           item->window.name ? item->window.name : "<null>",
                           ((itemDef_t *)it)->window.dynamicFlags[0]);
        return;
    }

    if (((itemDef_t *)it)->window.style == 6) {
        const char *dvarStr = Dvar_GetString(item->dvar);
        ((itemDef_t *)it)->window.background = CL_RegisterMaterialNoMip(dvarStr, 3);
    }

    Window_Paint(dc, (void *)item,
                 (((menuDef_t*)(parent))->fadeAmount), (((menuDef_t*)(parent))->fadeInAmount),
                 (((menuDef_t*)(parent))->fadeClamp), (float)(((menuDef_t*)(parent))->fadeCycle));

    if (debugMode) {
        float adjusted_y;
        memset(&rect, 0, sizeof(rect));
        rect.x = (((itemDef_s*)(it))->textRect[0].x);
        rect.y = (((itemDef_s*)(it))->textRect[0].y);
        rect.w = (((itemDef_s*)(it))->textRect[0].w);
        rect.h = ((itemDef_t *)it)->textRect[0].h;
        rect.horzAlign = (((itemDef_s*)(it))->textRect[0].horzAlign);
        rect.vertAlign = (((itemDef_s*)(it))->textRect[0].vertAlign);

        if (rect.w != 0.0f) {
            adjusted_y = rect.y - rect.h;
        } else {
            adjusted_y = rect.y;
        }

        {
            float color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
            UI_DrawRect(rect.x, adjusted_y, rect.w, rect.h, rect.horzAlign, rect.vertAlign, 1.0f, color);
        }
    }

    if (((itemDef_t *)it)->window.style == 6) {
        return;
    }

    {
        int itemType = (((itemDef_s*)(it))->type);
        switch (itemType) {
        case 0:
        case 1:
            Item_Text_Paint(dc, item);
            break;
        case 4:
        case 9:
        case 0x10:
        case 0x11:
        case 0x12:
            Item_TextField_Paint(dc, item);
            break;
        case 6:
            Item_ListBox_Paint(dc, item);
            break;
        case 8:
            Item_OwnerDraw_Paint(dc, item);
            break;
        case 0xa:
            Item_Slider_Paint(dc, item);
            break;
        case 0xb:
            Item_YesNo_Paint(dc, item);
            break;
        case 0x0c:  {
            const char *text;
            multiDef_t *multiDef = Item_GetMultiDef(item);
            if (!multiDef) {
                text = "<dvarStrList or dvarFloatList not set>";
            } else if (multiDef->strDef) {

                const char *value = Dvar_GetVariantString(item->dvar);
                int j;
                text = "";
                for (j = 0; j < multiDef->count; j++) {
                    if (I_stricmp(value, multiDef->dvarStr[j]) == 0) {
                        text = multiDef->dvarList[j];
                        break;
                    }
                }
            } else {

                const char *value = Dvar_GetVariantString(item->dvar);
                float fval = (float)atof(value);
                int j;
                text = "";
                for (j = 0; j < multiDef->count; j++) {
                    if (fval == multiDef->dvarValue[j]) {
                        text = multiDef->dvarList[j];
                        break;
                    }
                }
            }
            Item_List_Paint(dc, item, text);
            break;
        }
        case 0x0d:  {
            const char *text;
            const char *enumDvarName = (*(const char **)&((itemDef_t *)it)->typeData.listBox);
            if (!enumDvarName) {
                text = "<dvarEnumList not set>";
            } else {
                struct dvar_s *dvar = Dvar_FindVar(enumDvarName);
                if (!dvar || dvar->type != 6) {

                    text = "<not an enum dvar>";
                } else if (dvar->domain.enumeration.stringCount <= 0 || !dvar->domain.enumeration.strings) {

                    text = "";
                } else {

                    const char *enumString = Dvar_GetVariantString(item->dvar);
                    int idx = atoi(enumString);
                    int enumCount = dvar->domain.enumeration.stringCount;
                    const char **strings = (const char **)dvar->domain.enumeration.strings;
                    if (idx >= 0 && idx < enumCount) {
                        text = strings[idx];
                    } else {

                        int j;
                        text = strings[0];
                        for (j = 0; j < enumCount; j++) {
                            if (I_stricmp(enumString, strings[j]) == 0) {
                                text = strings[j];
                                break;
                            }
                        }
                    }
                }
            }
            Item_List_Paint(dc, item, text);
            break;
        }
        case 0x0e:
            Item_Bind_Paint(dc, item);
            break;
        default:
            break;
        }
    }
}

void Menu_Paint(displayContextDef_t *dc, menuDef_t *menu, qboolean forcePaint)
{
    byte *m = (byte *)menu;
    int i;
    int traceScriptMenu = 0;

    if (!menu)
        return;

    if (menu->window.name &&
        (!I_stricmp(menu->window.name, "serverinfo_dm") ||
         !strncmp(menu->window.name, "team_", 5) ||
         !I_stricmp(menu->window.name, "team"))) {
        static int paintTraceCount;
        if (paintTraceCount < 120) {
            traceScriptMenu = 1;
            if (getenv("MTRACE"))
                Com_Printf("[menu-trace] Menu_Paint name=%s force=%d flags=0x%x owner=0x%x items=%d fullscreen=%d openCount=%d\n",
                           menu->window.name, forcePaint, ((menuDef_t *)m)->window.dynamicFlags[0],
                           ((menuDef_t *)m)->window.ownerDrawFlags, (((menuDef_t*)(m))->itemCount), (((menuDef_t*)(m))->fullScreen),
                           (((displayContextDef_s*)((byte *)dc))->openMenuCount));
            ++paintTraceCount;
        }
    }

    if (!((*(byte *)&((menuDef_t *)m)->window.dynamicFlags[0]) & 4)) {
        if (traceScriptMenu)
            if (getenv("MTRACE"))
                Com_Printf("[menu-trace] Menu_Paint skip invisible name=%s flags=0x%x\n",
                           menu->window.name, ((menuDef_t *)m)->window.dynamicFlags[0]);
        if (!forcePaint)
            return;
    }

    int ownerDrawFlags = ((menuDef_t *)m)->window.ownerDrawFlags;
    if (ownerDrawFlags) {
        if (!UI_OwnerDrawVisible(ownerDrawFlags)) {
            if (traceScriptMenu)
                if (getenv("MTRACE"))
                    Com_Printf("[menu-trace] Menu_Paint skip owner name=%s owner=0x%x\n",
                               menu->window.name, ownerDrawFlags);
            return;
        }
    }

    if ((*(void **)&((menuDef_t *)m)->soundName))
        UI_PlayLocalSoundAliasByName(((menuDef_t *)m)->soundName);

    float fadeAmount = (((menuDef_t*)(m))->blurRadius);
    if (fadeAmount != 0.0f) {
        float cursorDist = dc->blurRadiusOut;
        float dist = sqrtf(fadeAmount * fadeAmount + cursorDist * cursorDist);
        dc->blurRadiusOut = dist;
        if (!forcePaint)
            goto paint_content;
    } else if (!forcePaint) {
        goto paint_content;
    }

    Window_AddDynamicFlags((void *)m, 0x4000);

    byte *d = (byte *)dc;
    int openCount = (((displayContextDef_s*)(d))->openMenuCount);
    int removeIdx = -1;
    for (i = openCount - 1; i >= 0; i--) {
        if (*(void **)((char *)&((displayContextDef_s *)d)->menuStack[i]) == menu) {
            removeIdx = i;
            break;
        }
    }
    if (removeIdx >= 0) {
        (((displayContextDef_s*)(d))->openMenuCount) = openCount - 1;
        for (i = removeIdx; i < (((displayContextDef_s*)(d))->openMenuCount); i++)
            *(void **)((char *)&((displayContextDef_s *)d)->menuStack[i]) = *(void **)((char *)&((displayContextDef_s *)d)->menuStack[(i + 1)]);
    }

    if ((((displayContextDef_s*)(d))->openMenuCount) == 0x10)
        Com_Error(1, "\x15Too many menus opened");
    int idx = (((displayContextDef_s*)(d))->openMenuCount);
    *(void **)((char *)&((displayContextDef_s *)d)->menuStack[idx]) = menu;
    (((displayContextDef_s*)(d))->openMenuCount) = idx + 1;

paint_content:

    if ((((menuDef_t*)(m))->fullScreen) && ((menuDef_t *)m)->window.background) {
        UI_DrawHandlePic(
            0.0f, 0.0f, 640.0f, 480.0f,
            (((rectDef_t*)(m))->horzAlign), (((rectDef_t*)(m))->vertAlign), NULL,
            ((menuDef_t *)m)->window.background);
    }

    Window_Paint(dc, (void *)m,
                 (((menuDef_t*)(m))->fadeAmount), (((menuDef_t*)(m))->fadeInAmount),
                 (((menuDef_t*)(m))->fadeClamp), (float)(((menuDef_t*)(m))->fadeCycle));

    int itemCount = (((menuDef_t*)(m))->itemCount);
    if (itemCount > 0) {
        for (i = 0; i < itemCount; i++) {
            itemDef_t *item = *(itemDef_t **)((char *)&((menuDef_t *)m)->items[i]);
            Item_Paint(dc, item);
        }
    }

    if (debugMode) {
        float color[4] = { 1.0f, 1.0f, 0.0f, 1.0f };
        UI_DrawRect(*(float *)m, (((rectDef_t*)(m))->y), (((rectDef_t*)(m))->w), ((menuDef_t *)m)->window.rect[0].h,
                    (((rectDef_t*)(m))->horzAlign), (((rectDef_t*)(m))->vertAlign), 1.0f, color);
    }
}

void Menu_PaintAll(displayContextDef_t *dc)
{
    byte *d = (byte *)dc;
    int i, j;

    (((displayContextDef_s*)(d))->blurRadiusOut) = 0.0f;

    if (captureFunc)
        captureFunc(dc, captureData);

    int menuCount = ((displayContextDef_t *)d)->menuCount;
    int openCount = (((displayContextDef_s*)(d))->openMenuCount);

    for (i = 0; i < menuCount; i++) {
        void *menu = *(void **)((char *)&((displayContextDef_s *)d)->Menus[i]);

        int isOpen = 0;
        for (j = openCount - 1; j >= 0; j--) {
            if (*(void **)((char *)&((displayContextDef_s *)d)->menuStack[j]) == menu) {
                isOpen = 1;
                break;
            }
        }
        if (!isOpen)
            Menu_Paint(dc, (menuDef_t *)menu, 0);
        openCount = (((displayContextDef_s*)(d))->openMenuCount);
    }

    openCount = (((displayContextDef_s*)(d))->openMenuCount);
    int fullscreenStart = 0;
    for (i = openCount - 1; i >= 0; i--) {
        byte *menu = *(byte **)((char *)&((displayContextDef_s *)d)->menuStack[i]);
        if ((((menuDef_t*)(menu))->fullScreen)) {
            fullscreenStart = i;
            break;
        }
    }

    if (fullscreenStart < openCount) {
        for (i = fullscreenStart; i < (((displayContextDef_s*)(d))->openMenuCount); i++) {
            void *menu = *(void **)((char *)&((displayContextDef_s *)d)->menuStack[i]);
            Menu_Paint(dc, (menuDef_t *)menu, 0);
        }
    }

    if (debugMode) {
        float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float scale = 0.5f;
        FontHandle font = UI_GetFontHandle(0, scale);
        const char *fpsText = va("fps: %f", (double)(((displayContextDef_s*)(d))->FPS));
        UI_DrawText(fpsText, 0x7fffffff, font, 5.0f, 25.0f, 0, 0, scale, color, 0);
    }
}

void Menus_HandleOOBClick(displayContextDef_t *dc, menuDef_t *menu, int key, qboolean down)
{
    byte *d = (byte *)dc;
    byte *m = (byte *)menu;
    int i, j;

    if (!menu)
        return;

    if (down) {
        if ((((Window *)m)->staticFlags & 0x2000000)) {

            if (((*(byte *)&((menuDef_t *)m)->window.dynamicFlags[0]) & 4) && (*(void **)&((menuDef_t *)m)->onClose)) {
                byte tempItem[0x2a0];
                *(void **)&(*(unsigned char *)&((itemDef_t *)tempItem)->parent) = menu;
                Item_RunScript(dc, (itemDef_t *)tempItem, ((menuDef_t *)m)->onClose);
            }
            Window_RemoveDynamicFlags((void *)menu, 6);
        }

    }

    int openCount = (((displayContextDef_s*)(d))->openMenuCount);
    float cx = (float)((displayContextDef_t *)d)->cursorx;
    float cy = (float)(((displayContextDef_s*)(d))->cursory);
    int targetIdx = -1;

    for (i = openCount - 1; i >= 0; i--) {
        byte *openMenu = *(byte **)((char *)&((displayContextDef_s *)d)->menuStack[i]);
        if (!openMenu)
            continue;
        if ((((menuDef_t *)openMenu)->window.dynamicFlags[0] & 0x4004) == 0)
            continue;

        float rx = *(float *)openMenu, ry = (((rectDef_t*)(openMenu))->y);
        float rw = (((rectDef_t*)(openMenu))->w), rh = ((menuDef_t *)openMenu)->window.rect[0].h;
        float testX = cx, testY = cy;
        CalcScreenX(&testX, 4);
        CalcScreenY(&testY, 4);
        CalcScreenPlacement(&rx, &ry, &rw, &rh, (((rectDef_t*)(openMenu))->horzAlign), (((rectDef_t*)(openMenu))->vertAlign));
        if (testX < rx || testX > rx + rw || testY < ry || testY > ry + rh)
            continue;

        int itemCount = (((menuDef_t*)(openMenu))->itemCount);
        if (itemCount <= 0)
            continue;

        for (j = 0; j < itemCount; j++) {
            byte *item = *(byte **)((char *)&((menuDef_t *)openMenu)->items[j]);
            if ((((itemDef_t *)item)->window.dynamicFlags[0] & 0x4004) == 0)
                continue;
            if (((itemDef_t *)item)->window.staticFlags & 0x100000)
                continue;

            float irx = (((rectDef_t*)(item))->x), iry = (((rectDef_t*)(item))->y);
            float irw = (((rectDef_t*)(item))->w), irh = ((itemDef_t *)item)->window.rect[0].h;
            float itx = cx, ity = cy;
            CalcScreenX(&itx, 4);
            CalcScreenY(&ity, 4);
            CalcScreenPlacement(&irx, &iry, &irw, &irh, (((rectDef_t*)(item))->horzAlign), (((rectDef_t*)(item))->vertAlign));
            if (itx < irx || itx > irx + irw || ity < iry || ity > iry + irh)
                continue;

            int itemType = (((itemDef_s*)(item))->type);
            /* Text buttons use their painted text; controls with labels use
               the full item rectangle, including the editable/value area. */
            if (itemType == 0 && ((itemDef_t *)item)->text) {

                int textBuf[6];
                memset(textBuf, 0, 24);
                *(float *)&textBuf[0] = (((itemDef_s*)(item))->textRect[0].x);
                *(float *)&textBuf[1] = (((itemDef_s*)(item))->textRect[0].y);
                *(float *)&textBuf[2] = (((itemDef_s*)(item))->textRect[0].w);
                *(float *)&textBuf[3] = ((itemDef_t *)item)->textRect[0].h;
                textBuf[4] = (((itemDef_s*)(item))->textRect[0].horzAlign);
                textBuf[5] = (((itemDef_s*)(item))->textRect[0].vertAlign);
                float trY = *(float *)&textBuf[1];
                if (*(float *)&textBuf[2] != 0.0f)
                    trY -= *(float *)&textBuf[3];
                float trx2 = *(float *)&textBuf[0], trw2 = *(float *)&textBuf[2];
                float try2 = trY, trh2 = *(float *)&textBuf[3];
                float ttx = cx, tty = cy;
                CalcScreenX(&ttx, 4);
                CalcScreenY(&tty, 4);
                CalcScreenPlacement(&trx2, &try2, &trw2, &trh2, textBuf[4], textBuf[5]);
                if (ttx < trx2 || ttx > trx2 + trw2 || tty < try2 || tty > try2 + trh2)
                    continue;
            }

            targetIdx = i;
            goto found;
        }
    }

    goto count_visible;

found:;

    openCount = (((displayContextDef_s*)(d))->openMenuCount);
    for (i = openCount - 1; i >= 0; i--)
        Window_RemoveDynamicFlags(*(void **)((char *)&((displayContextDef_s *)d)->menuStack[i]), 2);

    Window_AddDynamicFlags(*(void **)((char *)&((displayContextDef_s *)d)->menuStack[targetIdx]), 6);
    Display_MouseMove(dc, NULL, ((displayContextDef_t *)d)->cursorx, (((displayContextDef_s*)(d))->cursory));
    Menu_HandleMouseMove(dc, *(menuDef_t **)((char *)&((displayContextDef_s *)d)->menuStack[targetIdx]), (float)((displayContextDef_t *)d)->cursorx, (float)(((displayContextDef_s*)(d))->cursory));
    Menu_HandleKey(dc, *(menuDef_t **)((char *)&((displayContextDef_s *)d)->menuStack[targetIdx]), key, down);

count_visible:;

    int menuCount = ((displayContextDef_t *)d)->menuCount;
    int visCount = 0;
    for (i = 0; i < menuCount; i++) {
        byte *cm = *(byte **)((char *)&((displayContextDef_s *)d)->Menus[i]);
        if ((((Window *)cm)->dynamicFlags[0] & 0x4004) != 0)
            visCount++;
    }

    if (visCount == 0) {
        UI_Pause(0);

        openCount = (((displayContextDef_s*)(d))->openMenuCount);
        for (i = openCount - 1; i >= 0; i--) {
            byte *openMenu = *(byte **)((char *)&((displayContextDef_s *)d)->menuStack[i]);
            if (!openMenu)
                continue;

            if (((menuDef_t *)openMenu)->window.style == 5) {
                int cinHandle = ((menuDef_t *)openMenu)->window.cinematic;
                if (cinHandle >= 0) {
                    CIN_StopCinematic(cinHandle);
                    ((menuDef_t *)openMenu)->window.cinematic = -1;
                }
            }

            int ic = (((menuDef_t*)(openMenu))->itemCount);
            for (j = 0; j < ic; j++) {
                byte *item = *(byte **)((char *)&((menuDef_t *)openMenu)->items[j]);
                if (((itemDef_t *)item)->window.style == 5) {
                    int ch = ((itemDef_t *)item)->window.cinematic;
                    if (ch >= 0) {
                        CIN_StopCinematic(ch);
                        ((itemDef_t *)item)->window.cinematic = -1;
                    }
                    item = *(byte **)((char *)&((menuDef_t *)openMenu)->items[j]);
                }
                if ((((itemDef_s*)(item))->type) == 8)
                    CIN_StopCinematic(-((itemDef_t *)item)->window.ownerDraw);
            }
        }
    }
}

commandDef_t commandList[28] = {
    { (const char *)&str_002abdc4, &Script_FadeIn },
    { (const char *)&str_002abdcc, &Script_FadeOut },
    { (const char *)&str_002abdd4, &Script_Show },
    { (const char *)&str_002abddc, &Script_Hide },
    { (const char *)&str_002abde4, &Script_SetColor },
    { (const char *)&str_002abdf0, &Script_Open },
    { (const char *)&str_002abdf8, &Script_OpenForGameType },
    { (const char *)&str_002abe08, &Script_CloseForGameType },
    { (const char *)&str_002abe1c, &Script_Close },
    { (const char *)&str_002abe24, &Script_InGameOpen },
    { (const char *)&str_002abe30, &Script_InGameClose },
    { (const char *)&str_002abe3c, &Script_SetBackground },
    { (const char *)&str_002abe4c, &Script_SetItemColor },
    { (const char *)&str_002abe5c, &Script_SetFocus },
    { (const char *)&str_002abe68, &Script_SetFocusByDvar },
    { (const char *)&str_002abe78, &Script_Transition },
    { (const char *)&str_002abe84, &Script_SetDvar },
    { (const char *)&str_00219060, &Script_Exec },
    { (const char *)&str_002abe8c, &Script_ExecNow },
    { (const char *)&str_002abe94, &Script_ExecOnDvarStringValue },
    { (const char *)&str_002abeac, &Script_ExecOnDvarIntValue },
    { (const char *)&str_002abec0, &Script_ExecOnDvarFloatValue },
    { (const char *)&str_002abed8, &Script_ExecNowOnDvarStringValue },
    { (const char *)&str_002abef4, &Script_ExecNowOnDvarIntValue },
    { (const char *)&str_002abf0c, &Script_ExecNowOnDvarFloatValue },
    { (const char *)&str_002abf24, &Script_Play },
    { (const char *)&str_002abf2c, &Script_Orbit },
    { (const char *)&str_002abf34, &Script_ScriptMenuResponse }
};
bind_t g_bindings[56] = {
    { (char *)&str_002abf48, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abf50, 0xc9, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abf58, 0x77, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abf64, 0x73, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abf6c, 0x2c, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abf78, 0x2e, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abf84, 0x20, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abf8c, 0x63, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abf98, 0x9c, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abfa0, 0x9d, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abfa8, 0x9e, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abfb0, 0xa3, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abfb8, 0xa2, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abfc4, 0x2f, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abfcc, 0xa6, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abfd8, 0x9f, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abfe0, 0xcd, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abfec, 0xce, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002abff8, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac008, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac014, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ab890, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ab8a0, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac020, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac02c, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac034, 0xa7, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac03c, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac048, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac054, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac060, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac06c, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac074, 0x68, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac084, 0x31, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac098, 0x32, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac0ac, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac0b8, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac0cc, 0x76, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac0d4, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { "+sprint", 0xa0, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac0f0, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac0f8, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac100, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac10c, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac118, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac128, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac134, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac13c, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac148, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac154, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac160, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac16c, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac174, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_00225608, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac180, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
    { (char *)&str_002ac190, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
};

const unsigned char border[96] = {
    0x10, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x0a, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x0e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const char str_00219060[] = "exec";
const char str_00225608[] = "screenshot";
const char str_002ab890[] = "chatmodepublic";
const char str_002ab8a0[] = "chatmodeteam";
const char str_002abdc4[] = "fadein";
const char str_002abdcc[] = "fadeout";
const char str_002abdd4[] = "show";
const char str_002abddc[] = "hide";
const char str_002abde4[] = "setcolor";
const char str_002abdf0[] = "open";
const char str_002abdf8[] = "openforgametype";
const char str_002abe08[] = "closeforgametype";
const char str_002abe1c[] = "close";
const char str_002abe24[] = "ingameopen";
const char str_002abe30[] = "ingameclose";
const char str_002abe3c[] = "setbackground";
const char str_002abe4c[] = "setitemcolor";
const char str_002abe5c[] = "setfocus";
const char str_002abe68[] = "setfocusbydvar";
const char str_002abe78[] = "transition";
const char str_002abe84[] = "setdvar";
const char str_002abe8c[] = "execnow";
const char str_002abe94[] = "execOnDvarStringValue";
const char str_002abeac[] = "execOnDvarIntValue";
const char str_002abec0[] = "execOnDvarFloatValue";
const char str_002abed8[] = "execNowOnDvarStringValue";
const char str_002abef4[] = "execNowOnDvarIntValue";
const char str_002abf0c[] = "execNowOnDvarFloatValue";
const char str_002abf24[] = "play";
const char str_002abf2c[] = "orbit";
const char str_002abf34[] = "scriptmenuresponse";
const char str_002abf48[] = "+scores";
const char str_002abf50[] = "+speed";
const char str_002abf58[] = "+forward";
const char str_002abf64[] = "+back";
const char str_002abf6c[] = "+moveleft";
const char str_002abf78[] = "+moveright";
const char str_002abf84[] = "+moveup";
const char str_002abf8c[] = "+movedown";
const char str_002abf98[] = "+left";
const char str_002abfa0[] = "+right";
const char str_002abfa8[] = "+strafe";
const char str_002abfb0[] = "+lookup";
const char str_002abfb8[] = "+lookdown";
const char str_002abfc4[] = "+mlook";
const char str_002abfcc[] = "centerview";
const char str_002abfd8[] = "+attack";
const char str_002abfe0[] = "weapprev";
const char str_002abfec[] = "weapnext";
const char str_002abff8[] = "+frag";
const char str_002ac000[] = "+smoke";
const char str_002ac008[] = "scoresUp";
const char str_002ac014[] = "scoresDown";
const char str_002ac020[] = "+activate";
const char str_002ac02c[] = "+reload";
const char str_002ac034[] = "help";
const char str_002ac03c[] = "+leanleft";
const char str_002ac048[] = "+leanright";
const char str_002ac054[] = "+usereload";
const char str_002ac060[] = "vote yes";
const char str_002ac06c[] = "vote no";
const char str_002ac074[] = "mp_QuickMessage";
const char str_002ac084[] = "weaponslot primary";
const char str_002ac098[] = "weaponslot primaryb";
const char str_002ac0ac[] = "+binoculars";
const char str_002ac0b8[] = "+breath_binoculars";
const char str_002ac0cc[] = "+melee";
const char str_002ac0d4[] = "+holdbreath";
const char str_002ac0e0[] = "+melee_breath";
const char str_002ac0f0[] = "+prone";
const char str_002ac0f8[] = "+stance";
const char str_002ac100[] = "lowerstance";
const char str_002ac10c[] = "raisestance";
const char str_002ac118[] = "togglecrouch";
const char str_002ac128[] = "toggleprone";
const char str_002ac134[] = "goprone";
const char str_002ac13c[] = "gocrouch";
const char str_002ac148[] = "+gostand";
const char str_002ac154[] = "toggleads";
const char str_002ac160[] = "leaveads";
const char str_002ac16c[] = "+talk";
const char str_002ac174[] = "togglemenu";
const char str_002ac180[] = "screenshotJPEG";
const char str_002ac190[] = "openScriptMenu ingame changeweapon";
