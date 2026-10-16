#include "common_types.h"
#include "imports.h"
#include "cod2_feature_config.h"
#include "ui_console_mode.h"

#if COD2_FEATURE_GAMEPAD

extern uiInfo_t *uiInfo;

extern int UI_LoadMenus(const char *name, int imageTrack);
extern void UI_AddMenuList(uiInfo_t *info, int menuList);
extern void UI_DrawHandlePic(float x, float y, float w, float h,
                             int horzAlign, int vertAlign,
                             const vec_t *color, int material);
extern void UI_DrawText(const char *text, int maxChars, FontHandle font,
                        float x, float y, int horzAlign, int vertAlign,
                        float scale, const vec_t *color, int style);
extern int CL_RegisterMaterialNoMip(const char *name, int a);

extern void Cmd_AddCommand(const char *cmd, void (*func)(void));
extern void Cbuf_AddText(const char *text);
extern void Com_Printf(const char *fmt, ...);
extern Bool Dvar_GetBool(const char *name);
extern int Dvar_GetInt(const char *name);
extern const void *Dvar_RegisterInt(const char *name, int value,
                                    int min, int max, int flags);

#    define UI_DVAR_ARCHIVE 0x1

#    ifndef UI_FONT_NORMAL
#        define UI_FONT_NORMAL 2
#    endif

static const void *s_ui_consoleMode;
static const void *s_cl_gamepadAutoDetect;
static int s_menuSetLoaded;
static int s_cmdsRegistered;

extern int CL_Gamepad_IsActiveDevice(void);

static int s_glyphA;
static int s_glyphB;
static int s_glyphStart;
static int s_glyphsRegistered;

void UI_ConsoleMode_RegisterDvar(void)
{
    if (!s_cl_gamepadAutoDetect) {

        s_cl_gamepadAutoDetect = Dvar_RegisterInt("cl_gamepadAutoDetect", 1,
                                                  0, 1, UI_DVAR_ARCHIVE);
    }
    if (!s_ui_consoleMode) {

        s_ui_consoleMode = Dvar_RegisterInt("ui_consoleMode", 0, 0, 2,
                                            UI_DVAR_ARCHIVE);
    }
}

int UI_ConsoleMode_Active(void)
{
#ifdef __EMSCRIPTEN__
    /* Browser controller input uses the existing retail menus and cursor. */
    return 0;
#endif
    int override;

    UI_ConsoleMode_RegisterDvar();

    override = Dvar_GetInt("ui_consoleMode");
    if (override == 1)
        return 1;
    if (override == 2)
        return 0;

    return (Dvar_GetBool("cl_gamepadAutoDetect") &&
            CL_Gamepad_IsActiveDevice())
               ? 1
               : 0;
}

static void UI_ConsoleMode_OpenMain_f(void)
{
    extern qboolean Menus_OpenByName(uiInfo_t * info, const char *name);
    if (uiInfo)
        Menus_OpenByName(uiInfo, UI_ConsoleMode_MainMenuName());
}

void UI_ConsoleMode_Init(void *info)
{
    (void)info;

    UI_ConsoleMode_RegisterDvar();

    if (!UI_ConsoleMode_Active())
        return;

    Com_Printf("ui_consoleMode: controller-first presentation active over the "
               "real menus (mouse off, glyph legend, safe-area).\n");
}

const char *UI_ConsoleMode_MainMenuName(void)
{

    return (const char *)0;
}

int UI_ConsoleMode_SuppressCursor(void)
{

    return UI_ConsoleMode_Active();
}

void UI_ConsoleMode_SafeAreaRect(float *x, float *y, float *w, float *h)
{
    if (UI_ConsoleMode_Active()) {
        float insetX = UI_CONSOLE_VIRTUAL_W * UI_CONSOLE_SAFEAREA_FRAC;
        float insetY = UI_CONSOLE_VIRTUAL_H * UI_CONSOLE_SAFEAREA_FRAC;
        if (x)
            *x = insetX;
        if (y)
            *y = insetY;
        if (w)
            *w = UI_CONSOLE_VIRTUAL_W - 2.0f * insetX;
        if (h)
            *h = UI_CONSOLE_VIRTUAL_H - 2.0f * insetY;
    } else {
        if (x)
            *x = 0.0f;
        if (y)
            *y = 0.0f;
        if (w)
            *w = UI_CONSOLE_VIRTUAL_W;
        if (h)
            *h = UI_CONSOLE_VIRTUAL_H;
    }
}

static void UI_ConsoleMode_RegisterGlyphs(void)
{
    if (s_glyphsRegistered)
        return;

    s_glyphA = CL_RegisterMaterialNoMip("ui_button_xenon_a_32x32", 3);
    s_glyphB = CL_RegisterMaterialNoMip("ui_button_xenon_b_32x32", 3);
    s_glyphStart = CL_RegisterMaterialNoMip("ui_button_xenon_start_32x32", 3);
    if (!s_glyphA)
        s_glyphA = CL_RegisterMaterialNoMip("ui_button_xenon_trigl_32x32", 3);
    if (!s_glyphB)
        s_glyphB = CL_RegisterMaterialNoMip("ui_button_xenon_trigr_32x32", 3);
    s_glyphsRegistered = 1;
}

static float UI_ConsoleMode_DrawGlyph(float x, float y, int glyph,
                                      const char *label, const float *white)
{
    if (glyph)
        UI_DrawHandlePic(x, y - 14.0f, 16.0f, 16.0f, 0, 0, white, glyph);
    UI_DrawText(label, 0x7fffffff, (FontHandle)UI_FONT_NORMAL,
                x + 18.0f, y, 0 , 0,
                0.22f, white, 3 );

    return 18.0f + 6.0f + 6.0f * (float)(label ? (int)__builtin_strlen(label) : 0) + 22.0f;
}

void UI_ConsoleMode_DrawGlyphs(void *info)
{
    static const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float sx, sy, sw, sh;
    float x, y;

    (void)info;
    if (!UI_ConsoleMode_Active())
        return;

    UI_ConsoleMode_RegisterGlyphs();
    UI_ConsoleMode_SafeAreaRect(&sx, &sy, &sw, &sh);

    y = sy + sh - 6.0f;
    x = sx + 8.0f;

    x += UI_ConsoleMode_DrawGlyph(x, y, s_glyphA, "SELECT", white);
    x += UI_ConsoleMode_DrawGlyph(x, y, s_glyphB, "BACK", white);
    x += UI_ConsoleMode_DrawGlyph(x, y, s_glyphStart, "MENU", white);
}

#else

typedef int ui_console_mode_translation_unit_not_empty;

#endif
