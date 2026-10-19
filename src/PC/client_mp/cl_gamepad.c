#include "cod2_feature_config.h"

#if COD2_FEATURE_GAMEPAD

#    include <SDL2/SDL.h>
#    include <math.h>
#    include "cod2_sprint.h"

#    define __m128 __m128_cod2
#    include "common_types.h"
#    undef __m128
#    ifdef __EMSCRIPTEN__
#        include <emscripten.h>
#    endif

extern void Sys_QueEvent(int time, sysEventType_t type, int value, int value2,
                         int ptrLength, void *ptr);
extern clientStatic_t cls;
extern qboolean Display_KeyBindPending(void);
extern qboolean UI_checkKeyExec(int key);
extern void Key_SetBinding(int key, const char *command);
extern const char *Key_GetBinding(int key);
extern void Dvar_SetBoolByName(const char *name, int value);
extern void Cbuf_AddText(const char *text);
extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern const dvar_t *Dvar_RegisterFloat(const char *name, float value, float min, float max, int flags);
extern void Cmd_AddCommand(const char *cmd, void (*func)(void));
extern void Com_Printf(const char *fmt, ...);

extern void *imp_cl;

static int Gamepad_KeyCatchers(void)
{
    clientActive_t *cl = imp_cl ? *(clientActive_t **)imp_cl : NULL;
    return cl ? cl->keyCatchers : 0;
}

#    define K_AUX1 0xcf
#    define K_BUTTON_A (K_AUX1 + 0)
#    define K_BUTTON_B (K_AUX1 + 1)
#    define K_BUTTON_X (K_AUX1 + 2)
#    define K_BUTTON_Y (K_AUX1 + 3)
#    define K_BUTTON_LSHLDR (K_AUX1 + 4)
#    define K_BUTTON_RSHLDR (K_AUX1 + 5)
#    define K_BUTTON_BACK (K_AUX1 + 6)
#    define K_BUTTON_START (K_AUX1 + 7)
#    define K_BUTTON_LSTICK (K_AUX1 + 8)
#    define K_BUTTON_RSTICK (K_AUX1 + 9)
#    define K_DPAD_UP (K_AUX1 + 10)
#    define K_DPAD_DOWN (K_AUX1 + 11)
#    define K_DPAD_LEFT (K_AUX1 + 12)
#    define K_DPAD_RIGHT (K_AUX1 + 13)
#    define K_BUTTON_LTRIG (K_AUX1 + 14)
#    define K_BUTTON_RTRIG (K_AUX1 + 15)

#    define K_ENTER 0x0d
#    define K_ESCAPE 0x1b
#    define K_BACKSPACE 0x7f
#    define K_UPARROW 0x9a
#    define K_DOWNARROW 0x9b
#    define K_LEFTARROW 0x9c
#    define K_RIGHTARROW 0x9d

static int Gamepad_SdlButtonToKey(int sdlButton)
{
    switch (sdlButton) {
    case SDL_CONTROLLER_BUTTON_A:
        return K_BUTTON_A;
    case SDL_CONTROLLER_BUTTON_B:
        return K_BUTTON_B;
    case SDL_CONTROLLER_BUTTON_X:
        return K_BUTTON_X;
    case SDL_CONTROLLER_BUTTON_Y:
        return K_BUTTON_Y;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
        return K_BUTTON_LSHLDR;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
        return K_BUTTON_RSHLDR;
    case SDL_CONTROLLER_BUTTON_BACK:
        return K_BUTTON_BACK;
    case SDL_CONTROLLER_BUTTON_START:
        return K_BUTTON_START;
    case SDL_CONTROLLER_BUTTON_LEFTSTICK:
        return K_BUTTON_LSTICK;
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
        return K_BUTTON_RSTICK;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
        return K_DPAD_UP;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
        return K_DPAD_DOWN;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        return K_DPAD_LEFT;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
        return K_DPAD_RIGHT;
    default:
        return 0;
    }
}

static int Gamepad_NavRemap(int key)
{
    switch (key) {
    case K_DPAD_UP:
        return K_UPARROW;
    case K_DPAD_DOWN:
        return K_DOWNARROW;
    case K_DPAD_LEFT:
        return K_LEFTARROW;
    case K_DPAD_RIGHT:
        return K_RIGHTARROW;
    case K_BUTTON_A:
        return K_ENTER;
    case K_BUTTON_B:
        return K_ESCAPE;
    case K_BUTTON_X:
        return K_BACKSPACE;
    default:
        return 0;
    }
}

enum GamepadPhysicalAxis {
    PHYS_LSTICK_X = 0,
    PHYS_LSTICK_Y,
    PHYS_RSTICK_X,
    PHYS_RSTICK_Y,
    PHYS_LTRIG,
    PHYS_RTRIG,
    PHYS_AXIS_COUNT
};

enum GamepadVirtualAxis {
    VIRT_LOOK_X = 0,
    VIRT_LOOK_Y,
    VIRT_MOVE_X,
    VIRT_MOVE_Y,
    VIRT_AXIS_COUNT
};

enum GamepadMapping {
    MAP_LINEAR = 0,
    MAP_SQUARED = 1
};

static const struct {
    int physical;
    int mapping;
} g_virtualAxisBind[VIRT_AXIS_COUNT] = {
    [VIRT_LOOK_X] = { PHYS_RSTICK_X, MAP_SQUARED },
    [VIRT_LOOK_Y] = { PHYS_RSTICK_Y, MAP_SQUARED },
    [VIRT_MOVE_X] = { PHYS_LSTICK_X, MAP_LINEAR },
    [VIRT_MOVE_Y] = { PHYS_LSTICK_Y, MAP_LINEAR },
};

static const dvar_t *cl_gamepadEnable;
static const dvar_t *cl_gamepadLookSensitivity;
static const dvar_t *cl_gamepadDeadzone;
static const dvar_t *cl_gamepadInvertY;

static int g_buttonLayoutSouthpaw;

static int stricmp_local(const char *a, const char *b)
{
    while (*a && *b) {
        int ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z')
            ca += 32;
        if (cb >= 'A' && cb <= 'Z')
            cb += 32;
        if (ca != cb)
            return ca - cb;
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

#    define GAMEPAD_MAX_PADS 4
static SDL_GameController *g_pads[GAMEPAD_MAX_PADS];
static int g_padInstance[GAMEPAD_MAX_PADS];
static int g_gamepadInit;

#    define GAMEPAD_BUTTON_STATES 256
static unsigned char g_buttonDown[GAMEPAD_BUTTON_STATES];

static int g_gamepadActiveDevice;
/* Remember the emitted key, so changing menu/capture state never changes a
 * release into a different key and leaves attack/navigation held. */
static int g_emittedKey[GAMEPAD_BUTTON_STATES];
static int g_blockedButton[GAMEPAD_BUTTON_STATES];
static int g_menuState;
static int g_navDirection;
static int g_navDeadline;
static int g_sprintRequested;
static int g_sprintRequestTime;
static int g_sprintLatched;

static void Gamepad_SendKey(int key, int down);
static void Gamepad_ReleaseAll(void)
{
    int key;
    for (key = 1; key < GAMEPAD_BUTTON_STATES; ++key) {
        if (g_buttonDown[key]) {
            const char *binding = Key_GetBinding(key);
            if (g_emittedKey[key] == key && binding && !stricmp_local(binding, "+stance")) {
                clientActive_t *cl = imp_cl ? *(clientActive_t **)imp_cl : NULL;
                if (cl) cl->stanceHeld = 0;
            }
            Gamepad_SendKey(key, 0);
            g_blockedButton[key] = 1;
        }
    }
    if (g_navDirection)
        Sys_QueEvent(0, SE_KEY, g_navDirection, 0, 0, NULL);
    g_navDirection = 0;
    g_sprintRequested = g_sprintLatched = 0;
}

static int Gamepad_HasFocus(void)
{
#    ifdef __EMSCRIPTEN__
    return EM_ASM_INT({ return !document.hidden && document.hasFocus(); });
#    else
    return 1;
#    endif
}

void CL_Gamepad_DefaultBindings(void)
{
    static const char *commands[16] = {
        "+gostand", "+stance", "+usereload", "weapnext",
        "+smoke", "+frag", "+scores", "togglemenu",
        "+sprint", "+melee", "+breath_binoculars", "goprone",
        "+leanleft", "+leanright", "+speed", "+attack"
    };
    int i;
    Gamepad_ReleaseAll();
    for (i = 0; i < 16; ++i)
        Key_SetBinding(K_AUX1 + i, commands[i]);
    Dvar_SetBoolByName("cl_gamepadBindingsInitialized", 1);
    Dvar_SetBoolByName("cl_gamepadStanceInitialized", 1);
}

int CL_Gamepad_IsConnected(void)
{
    int i;
    if (!g_gamepadInit)
        return 0;
    for (i = 0; i < GAMEPAD_MAX_PADS; ++i)
        if (g_pads[i])
            return 1;
    return 0;
}

int CL_Gamepad_IsActiveDevice(void)
{
    return (g_gamepadActiveDevice && CL_Gamepad_IsConnected()) ? 1 : 0;
}

void CL_Gamepad_NoteKbmActive(void)
{
    g_gamepadActiveDevice = 0;
}

static SDL_GameController *Gamepad_ActivePad(void)
{
    int i;
    for (i = 0; i < GAMEPAD_MAX_PADS; ++i)
        if (g_pads[i])
            return g_pads[i];
    return NULL;
}

static void Gamepad_Open(int joystickIndex)
{
    int i;
    SDL_GameController *gc;
    SDL_JoystickID id;

    if (!SDL_IsGameController(joystickIndex))
        return;

    gc = SDL_GameControllerOpen(joystickIndex);
    if (!gc)
        return;

    id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gc));
    for (i = 0; i < GAMEPAD_MAX_PADS; ++i) {
        if (g_pads[i] && g_padInstance[i] == id) {
            SDL_GameControllerClose(gc);
            return;
        }
    }
    for (i = 0; i < GAMEPAD_MAX_PADS; ++i) {
        if (!g_pads[i]) {
            g_pads[i] = gc;
            g_padInstance[i] = id;
            Com_Printf("Gamepad connected: %s\n", SDL_GameControllerName(gc));
            return;
        }
    }
    SDL_GameControllerClose(gc);
}

static void Gamepad_CloseByInstance(int instanceId)
{
    int i;
    for (i = 0; i < GAMEPAD_MAX_PADS; ++i) {
        if (g_pads[i] && g_padInstance[i] == instanceId) {
            Com_Printf("Gamepad disconnected: %s\n",
                       SDL_GameControllerName(g_pads[i]));
            Gamepad_ReleaseAll();
            SDL_GameControllerClose(g_pads[i]);
            g_pads[i] = NULL;
            g_padInstance[i] = -1;
            return;
        }
    }
}

void CL_Gamepad_HandleDeviceEvent(int sdlEventType, int which)
{
    if (!g_gamepadInit)
        return;
    if (sdlEventType == SDL_CONTROLLERDEVICEADDED)
        Gamepad_Open(which);
    else if (sdlEventType == SDL_CONTROLLERDEVICEREMOVED)
        Gamepad_CloseByInstance(which);
}

void CL_Gamepad_Init(void)
{
    int i, n;

    Dvar_RegisterBool_mac("ui_gamepadConnected", 0, 0);
    cl_gamepadEnable = Dvar_RegisterBool_mac("cl_gamepadEnable", 1, 0x1);
    cl_gamepadLookSensitivity = Dvar_RegisterFloat("cl_gamepadLookSensitivity", 8.0f, 0.1f, 50.0f, 0x1);
    cl_gamepadDeadzone = Dvar_RegisterFloat("cl_gamepadDeadzone", 0.20f, 0.0f, 0.9f, 0x1);
    cl_gamepadInvertY = Dvar_RegisterBool_mac("cl_gamepadInvertY", 0, 0x1);

    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) {
        Com_Printf("Gamepad: SDL_INIT_GAMECONTROLLER failed: %s\n", SDL_GetError());
        return;
    }
    g_gamepadInit = 1;

    for (i = 0; i < GAMEPAD_MAX_PADS; ++i)
        g_padInstance[i] = -1;

    n = SDL_NumJoysticks();
    for (i = 0; i < n; ++i)
        Gamepad_Open(i);

    /* Config is already loaded here. Install once, including on profiles
     * created before controller support, without overwriting custom/unbound keys. */
    if (!Dvar_RegisterBool_mac("cl_gamepadBindingsInitialized", 0, 0x1)->current.enabled) {
        int hasBindings = 0;
        for (i = K_AUX1; i < K_AUX1 + 16; ++i) {
            const char *binding = Key_GetBinding(i);
            if (binding && binding[0]) hasBindings = 1;
        }
        if (!hasBindings) CL_Gamepad_DefaultBindings();
        Dvar_SetBoolByName("cl_gamepadBindingsInitialized", 1);
    }
    g_menuState = Gamepad_KeyCatchers();
    /* Upgrade the old default once; preserve remapped and cleared buttons. */
    if (!Dvar_RegisterBool_mac("cl_gamepadStanceInitialized", 0, 0x1)->current.enabled) {
        const char *binding = Key_GetBinding(K_BUTTON_B);
        if (binding && !stricmp_local(binding, "togglecrouch"))
            Key_SetBinding(K_BUTTON_B, "+stance");
        Dvar_SetBoolByName("cl_gamepadStanceInitialized", 1);
    }
}

void CL_Gamepad_ApplyButtonConfig(const char *preset)
{
    if (!preset || !preset[0])
        preset = "default";

    if (!stricmp_local(preset, "southpaw")) {

        g_buttonLayoutSouthpaw = 1;
    } else {
        g_buttonLayoutSouthpaw = 0;
    }

    if (!stricmp_local(preset, "legacy")) {

        Cbuf_AddText("bind AUX1 \"+gostand\"\n");
        Cbuf_AddText("bind AUX2 \"+stance\"\n");
        Cbuf_AddText("bind AUX3 \"+melee\"\n");
        Cbuf_AddText("bind AUX4 \"+usereload\"\n");
    } else if (!stricmp_local(preset, "tactical")) {

        Cbuf_AddText("bind AUX1 \"+gostand\"\n");
        Cbuf_AddText("bind AUX2 \"+melee\"\n");
        Cbuf_AddText("bind AUX3 \"+usereload\"\n");
        Cbuf_AddText("bind AUX4 \"+stance\"\n");
    } else {

        Cbuf_AddText("bind AUX1 \"+gostand\"\n");
        Cbuf_AddText("bind AUX2 \"+stance\"\n");
        Cbuf_AddText("bind AUX3 \"+usereload\"\n");
        Cbuf_AddText("bind AUX4 \"+melee\"\n");
    }
    Com_Printf("Controller layout: %s\n", preset);
}

void CL_Gamepad_Shutdown(void)
{
    int i;
    if (!g_gamepadInit)
        return;
    Gamepad_ReleaseAll();
    for (i = 0; i < GAMEPAD_MAX_PADS; ++i) {
        if (g_pads[i]) {
            SDL_GameControllerClose(g_pads[i]);
            g_pads[i] = NULL;
            g_padInstance[i] = -1;
        }
    }
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    g_gamepadInit = 0;
}

static float Gamepad_RawAxis(SDL_GameController *gc, int physical)
{
    int raw;
    switch (physical) {
    case PHYS_LSTICK_X:
        raw = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX);
        break;
    case PHYS_LSTICK_Y:
        raw = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY);
        break;
    case PHYS_RSTICK_X:
        raw = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTX);
        break;
    case PHYS_RSTICK_Y:
        raw = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTY);
        break;
    case PHYS_LTRIG:
        raw = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        break;
    case PHYS_RTRIG:
        raw = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        break;
    default:
        return 0.0f;
    }
    return (float)raw / 32767.0f;
}

static float Gamepad_VirtualAxis(SDL_GameController *gc, int virt)
{
    int physical = g_virtualAxisBind[virt].physical;
    int mapping = g_virtualAxisBind[virt].mapping;
    float deadzone = cl_gamepadDeadzone->current.value;
    float x, mag, other;

    if (g_buttonLayoutSouthpaw) {
        switch (physical) {
        case PHYS_LSTICK_X:
            physical = PHYS_RSTICK_X;
            break;
        case PHYS_LSTICK_Y:
            physical = PHYS_RSTICK_Y;
            break;
        case PHYS_RSTICK_X:
            physical = PHYS_LSTICK_X;
            break;
        case PHYS_RSTICK_Y:
            physical = PHYS_LSTICK_Y;
            break;
        default:
            break;
        }
    }

    x = Gamepad_RawAxis(gc, physical);

    switch (physical) {
    case PHYS_LSTICK_X:
        other = Gamepad_RawAxis(gc, PHYS_LSTICK_Y);
        break;
    case PHYS_LSTICK_Y:
        other = Gamepad_RawAxis(gc, PHYS_LSTICK_X);
        break;
    case PHYS_RSTICK_X:
        other = Gamepad_RawAxis(gc, PHYS_RSTICK_Y);
        break;
    case PHYS_RSTICK_Y:
        other = Gamepad_RawAxis(gc, PHYS_RSTICK_X);
        break;
    default:
        other = 0.0f;
        break;
    }

    mag = sqrtf(x * x + other * other);
    if (mag < deadzone)
        return 0.0f;

    if (mag > 0.0f) {
        float scaled = (mag - deadzone) / (1.0f - deadzone);
        if (scaled > 1.0f)
            scaled = 1.0f;
        x = x * (scaled / mag);
    }

    if (mapping == MAP_SQUARED) {

        x = x * fabsf(x);
    }
    return x;
}

int CL_GamepadAdsActive(void)
{
    return g_buttonDown[K_BUTTON_LTRIG] ? 1 : 0;
}

static void Gamepad_SendKey(int key, int down)
{
    int emitted;
    if (key <= 0 || key >= GAMEPAD_BUTTON_STATES)
        return;
    if (!down) g_blockedButton[key] = 0;
    if (down && g_blockedButton[key]) return;
    if (g_buttonDown[key] == (unsigned char)down)
        return;
    g_buttonDown[key] = (unsigned char)down;
    if (down) {
        int catchers = Gamepad_KeyCatchers();
        emitted = key;
        if ((catchers & (1 | 8 | 0x10)) && !Display_KeyBindPending()) {
            emitted = Gamepad_NavRemap(key);
            if (key == K_BUTTON_START) emitted = K_ESCAPE;
            /* The bumpers page the pause menu's tabs. A menu that does not
             * take them would hand them to their gameplay binds instead. */
            if ((key == K_BUTTON_LSHLDR || key == K_BUTTON_RSHLDR) && (catchers & 8) &&
                !(catchers & (1 | 0x10)) && UI_checkKeyExec(key))
                emitted = key;
        }
        g_emittedKey[key] = emitted;
        if (!(Gamepad_KeyCatchers() & (1 | 8 | 0x10))) {
            const char *binding = Key_GetBinding(key);
            if (binding && !stricmp_local(binding, "+sprint")) {
                g_sprintRequested = 1;
                g_sprintRequestTime = cls.realtime;
            }
        }
    } else {
        emitted = g_emittedKey[key];
        g_emittedKey[key] = 0;
    }
    if (emitted)
        Sys_QueEvent(0, SE_KEY, emitted, down, 0, NULL);
}

static void Gamepad_MenuStick(SDL_GameController *gc)
{
    float x = Gamepad_RawAxis(gc, PHYS_LSTICK_X);
    float y = Gamepad_RawAxis(gc, PHYS_LSTICK_Y);
    int direction = 0;
    if (!Display_KeyBindPending()) {
        if (fabsf(y) > 0.6f && fabsf(y) >= fabsf(x))
            direction = y < 0 ? K_UPARROW : K_DOWNARROW;
        else if (fabsf(x) > 0.6f)
            direction = x < 0 ? K_LEFTARROW : K_RIGHTARROW;
    }
    if (direction != g_navDirection) {
        if (g_navDirection)
            Sys_QueEvent(0, SE_KEY, g_navDirection, 0, 0, NULL);
        g_navDirection = direction;
        if (direction) {
            Sys_QueEvent(0, SE_KEY, direction, 1, 0, NULL);
            g_navDeadline = cls.realtime + 350;
        }
    } else if (direction && cls.realtime >= g_navDeadline) {
        Sys_QueEvent(0, SE_KEY, direction, 1, 0, NULL);
        g_navDeadline = cls.realtime + 100;
    }
}

void CL_Gamepad_Frame(void)
{
    SDL_GameController *gc;
    int b, catchers = Gamepad_KeyCatchers();
    if (!g_gamepadInit) return;
    Dvar_SetBoolByName("ui_gamepadConnected", CL_Gamepad_IsConnected());
    if (catchers != g_menuState) {
        Gamepad_ReleaseAll();
        g_menuState = catchers;
    }
    if (!cl_gamepadEnable->current.enabled || !Gamepad_HasFocus()) {
        Gamepad_ReleaseAll();
        return;
    }
    gc = Gamepad_ActivePad();
    if (!gc) {
        Gamepad_ReleaseAll();
        return;
    }
    for (b = 0; b < SDL_CONTROLLER_BUTTON_MAX; ++b) {
        int key = Gamepad_SdlButtonToKey(b);
        if (key) {
            int down = SDL_GameControllerGetButton(gc, b) ? 1 : 0;
            if (down && !g_buttonDown[key]) g_gamepadActiveDevice = 1;
            Gamepad_SendKey(key, down);
        }
    }
    Gamepad_SendKey(K_BUTTON_LTRIG, Gamepad_RawAxis(gc, PHYS_LTRIG) > 0.5f);
    Gamepad_SendKey(K_BUTTON_RTRIG, Gamepad_RawAxis(gc, PHYS_RTRIG) > 0.5f);
    if (catchers & 8) Gamepad_MenuStick(gc);
}

static float Gamepad_LookDelta(float axis, float seconds, float sensitivity, float limit)
{
    float delta = axis * sensitivity * cl_gamepadLookSensitivity->current.value * 22.5f * seconds;
    /* Preserve the same mounted-weapon turning limits as mouse input. */
    if (isfinite(limit) && limit > 0.0f) {
        float maxDelta = limit * seconds;
        if (delta > maxDelta) delta = maxDelta;
        if (delta < -maxDelta) delta = -maxDelta;
    }
    return delta;
}

void CL_Gamepad_Move(usercmd_t *cmd)
{
    SDL_GameController *gc;
    float mx, my;
    int side, forward;
    clientActive_t *cl;
    float seconds, lookX, lookY, sensitivity;

    if (!g_gamepadInit || !cl_gamepadEnable->current.enabled)
        return;
    if (Gamepad_KeyCatchers() & (1 | 8 | 0x10) || !Gamepad_HasFocus())
        return;

    gc = Gamepad_ActivePad();
    if (!gc)
        return;

    /* Apply angular velocity during command creation, independently of mouse
     * sensitivity and display cadence. Fractional stick motion stays smooth. */
    cl = imp_cl ? *(clientActive_t **)imp_cl : NULL;
    if (!cl) return;
    seconds = (float)cls.frametime * 0.001f;
    if (seconds > 0.1f) seconds = 0.1f;
    if (seconds < 0.0f) seconds = 0.0f;
    lookX = Gamepad_VirtualAxis(gc, VIRT_LOOK_X);
    lookY = Gamepad_VirtualAxis(gc, VIRT_LOOK_Y);
    sensitivity = isfinite(cl->cgameSensitivity) ? cl->cgameSensitivity : 1.0f;
    cl->viewangles[1] -= Gamepad_LookDelta(lookX, seconds, sensitivity, cl->cgameMaxYawSpeed);
    cl->viewangles[0] += Gamepad_LookDelta(lookY, seconds, sensitivity, cl->cgameMaxPitchSpeed) *
                         (cl_gamepadInvertY->current.enabled ? -1.0f : 1.0f);
    mx = Gamepad_VirtualAxis(gc, VIRT_MOVE_X);
    my = Gamepad_VirtualAxis(gc, VIRT_MOVE_Y);

    side = (int)cmd->rightmove + (int)(mx * 127.0f);
    forward = (int)cmd->forwardmove - (int)(my * 127.0f);

    if (side > 127)
        side = 127;
    else if (side < -127)
        side = -127;
    if (forward > 127)
        forward = 127;
    else if (forward < -127)
        forward = -127;

    cmd->rightmove = (signed char)side;
    cmd->forwardmove = (signed char)forward;

    /* Clicking the stick starts a run; releasing the click must not end it.
     * Keep the normal key events for held breath while aiming. Neutral/back,
     * stance, combat actions and menus cancel the run, requiring a new click. */
    /* A click just before pushing forward (especially while getting up from
     * prone) must survive the neutral command. Expire it quickly so a much
     * later movement cannot unexpectedly start sprinting. */
    if (g_sprintRequested &&
        (unsigned int)(cls.realtime - g_sprintRequestTime) > 250u)
        g_sprintRequested = 0;
    if (cl->snap.ps.pm_type != 0 || my > 0.1f ||
        (cmd->buttons & (0x1 | 0x4 | 0x8 | 0x10 | 0x20 | 0x40 | 0x80 |
                         0x1000 | 0x4000 | 0x10000 | 0x20000))) {
        g_sprintRequested = g_sprintLatched = 0;
    } else if (my >= -0.1f) {
        g_sprintLatched = 0;
    } else if (g_sprintRequested) {
        g_sprintLatched = 1;
        g_sprintRequested = 0;
    } else if (cmd->buttons & (0x100 | 0x200)) {
        g_sprintLatched = 0;
    }
    if (g_sprintLatched) cmd->buttons |= BUTTON_SPRINT;
}

#    if COD2_FEATURE_RUMBLE
void CL_Gamepad_SetRumble(float lowFreq, float highFreq)
{
    SDL_GameController *gc;
    Uint16 lo, hi;

    if (!g_gamepadInit)
        return;
    gc = Gamepad_ActivePad();
    if (!gc)
        return;

    if (lowFreq < 0.0f)
        lowFreq = 0.0f;
    else if (lowFreq > 1.0f)
        lowFreq = 1.0f;
    if (highFreq < 0.0f)
        highFreq = 0.0f;
    else if (highFreq > 1.0f)
        highFreq = 1.0f;

    lo = (Uint16)(lowFreq * 65535.0f);
    hi = (Uint16)(highFreq * 65535.0f);

    SDL_GameControllerRumble(gc, lo, hi, 250);
}
#    endif

#else

typedef int cl_gamepad_translation_unit_not_empty;

#endif
