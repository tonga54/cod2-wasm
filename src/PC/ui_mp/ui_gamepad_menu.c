#include "common_types.h"
#include "imports.h"
#include "cod2_feature_config.h"

#if COD2_FEATURE_GAMEPAD

extern uiInfo_t *uiInfo;
extern qboolean Menus_OpenByName(uiInfo_t *info, const char *name);
extern void Cmd_AddCommand(const char *cmd, void (*func)(void));
extern void Cbuf_AddText(const char *text);
extern void Com_Printf(const char *fmt, ...);
extern const dvar_t *Dvar_RegisterString(const char *name, const char *value, int flags);

static int s_gamepadMenuCmdsRegistered;
static const dvar_t *cl_gamepadButtonConfig;

static void UI_Gamepad_OpenMenu_f(void)
{
    if (uiInfo)
        Menus_OpenByName(uiInfo, "browser_gamepad");
}

static void UI_Gamepad_Defaults_f(void)
{
    Cbuf_AddText("seta cl_gamepadEnable 1\n");
    Cbuf_AddText("seta cl_gamepadLookSensitivity 8\n");
    Cbuf_AddText("seta cl_gamepadDeadzone 0.2\n");
    Cbuf_AddText("seta cl_gamepadInvertY 0\n");
    Cbuf_AddText("seta cl_gamepadButtonConfig default\n");
    {
        extern void CL_Gamepad_DefaultBindings(void);
        CL_Gamepad_DefaultBindings();
    }
    Com_Printf("Controller settings reset to defaults.\n");
}

static void UI_Gamepad_Restart_f(void)
{
    extern void CL_Gamepad_ApplyButtonConfig(const char *preset);
    const char *preset = cl_gamepadButtonConfig
                             ? cl_gamepadButtonConfig->current.string
                             : "default";
    CL_Gamepad_ApplyButtonConfig(preset);
}

void UI_Gamepad_RegisterMenus(uiInfo_t *info)
{
    (void)info;

    if (!cl_gamepadButtonConfig)
        cl_gamepadButtonConfig = Dvar_RegisterString("cl_gamepadButtonConfig",
                                                     "default", 0x1 );

    if (!s_gamepadMenuCmdsRegistered) {
        Cmd_AddCommand("controls_gamepad", UI_Gamepad_OpenMenu_f);
        Cmd_AddCommand("gamepad_defaults", UI_Gamepad_Defaults_f);
        Cmd_AddCommand("gamepad_restart", UI_Gamepad_Restart_f);
        s_gamepadMenuCmdsRegistered = 1;
    }
}

#else

typedef int ui_gamepad_menu_translation_unit_not_empty;

#endif
