#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define __m128 __m128_cod2
#include "common_types.h"
#undef __m128

#include "cod2_feature_config.h"

#if COD2_FEATURE_GAMEPAD

extern void CL_Gamepad_Frame(void);
extern void CL_Gamepad_HandleDeviceEvent(int sdlEventType, int which);
#endif

extern void CCallOfDutyEngine_DoMouseMoved(const CCallOfDutyEngine *_this,
                                           Point inDelta, Point inPoint,
                                           UInt32 inModifiers);
extern void CCallOfDutyEngine_DoMouseWheel(const CCallOfDutyEngine *_this,
                                           SInt32 inDelta, Point inPoint,
                                           UInt32 inModifiers);
extern void CCallOfDutyEngine_DoTextInput(const CCallOfDutyEngine *_this,
                                          UInt16 inUniChar);
extern void Sys_QueEvent(int time, sysEventType_t type, int value, int value2,
                         int ptrLength, void *ptr);
extern void CL_MouseEventAbsolute(int x, int y, int dx, int dy);

typedef struct LinuxInputEvent {
    int type;
    int value;
    int value2;
    int x;
    int y;
    int dx;
    int dy;
} LinuxInputEvent;

extern int Linux_PollInputEvent(LinuxInputEvent *event);

#define K_BACKSPACE  0x7f
#define K_COMMAND    0x96
#define K_CAPSLOCK   0x97
#define K_PAUSE      0x99
#define K_UPARROW    0x9a
#define K_DOWNARROW  0x9b
#define K_LEFTARROW  0x9c
#define K_RIGHTARROW 0x9d
#define K_ALT        0x9e
#define K_CTRL       0x9f
#define K_SHIFT      0xa0
#define K_INS        0xa1
#define K_DEL        0xa2
#define K_PGDN       0xa3
#define K_PGUP       0xa4
#define K_HOME       0xa5
#define K_END        0xa6
#define K_F1         0xa7
#define K_KP_HOME    0xb6
#define K_KP_UPARROW 0xb7
#define K_KP_PGUP    0xb8
#define K_KP_LEFTARROW  0xb9
#define K_KP_5       0xba
#define K_KP_RIGHTARROW 0xbb
#define K_KP_END     0xbc
#define K_KP_DOWNARROW  0xbd
#define K_KP_PGDN    0xbe
#define K_KP_ENTER   0xbf
#define K_KP_INS     0xc0
#define K_KP_DEL     0xc1
#define K_KP_SLASH   0xc2
#define K_KP_MINUS   0xc3
#define K_KP_PLUS    0xc4
#define K_KP_NUMLOCK 0xc5
#define K_KP_STAR    0xc6
#define K_KP_EQUALS  0xc7
#define K_MOUSE1     0xc8

static int sdl_to_keynum(SDL_Keycode sym)
{
    switch (sym) {
    case SDLK_TAB:          return 0x09;
    case SDLK_RETURN:
    case SDLK_RETURN2:      return 0x0d;
    case SDLK_ESCAPE:       return 0x1b;
    case SDLK_SPACE:        return 0x20;
    case SDLK_BACKSPACE:    return K_BACKSPACE;
    case SDLK_BACKQUOTE:    return '`';

    case SDLK_UP:           return K_UPARROW;
    case SDLK_DOWN:         return K_DOWNARROW;
    case SDLK_LEFT:         return K_LEFTARROW;
    case SDLK_RIGHT:        return K_RIGHTARROW;

    case SDLK_LALT:
    case SDLK_RALT:         return K_ALT;
    case SDLK_LCTRL:
    case SDLK_RCTRL:        return K_CTRL;
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:       return K_SHIFT;
    case SDLK_LGUI:
    case SDLK_RGUI:         return K_COMMAND;
    case SDLK_CAPSLOCK:     return K_CAPSLOCK;
    case SDLK_PAUSE:        return K_PAUSE;

    case SDLK_INSERT:       return K_INS;
    case SDLK_DELETE:       return K_DEL;
    case SDLK_PAGEDOWN:     return K_PGDN;
    case SDLK_PAGEUP:       return K_PGUP;
    case SDLK_HOME:         return K_HOME;
    case SDLK_END:          return K_END;

    case SDLK_F1:  return K_F1 + 0;   case SDLK_F2:  return K_F1 + 1;
    case SDLK_F3:  return K_F1 + 2;   case SDLK_F4:  return K_F1 + 3;
    case SDLK_F5:  return K_F1 + 4;   case SDLK_F6:  return K_F1 + 5;
    case SDLK_F7:  return K_F1 + 6;   case SDLK_F8:  return K_F1 + 7;
    case SDLK_F9:  return K_F1 + 8;   case SDLK_F10: return K_F1 + 9;
    case SDLK_F11: return K_F1 + 10;  case SDLK_F12: return K_F1 + 11;
    case SDLK_F13: return K_F1 + 12;  case SDLK_F14: return K_F1 + 13;
    case SDLK_F15: return K_F1 + 14;

    case SDLK_KP_7:         return K_KP_HOME;
    case SDLK_KP_8:         return K_KP_UPARROW;
    case SDLK_KP_9:         return K_KP_PGUP;
    case SDLK_KP_4:         return K_KP_LEFTARROW;
    case SDLK_KP_5:         return K_KP_5;
    case SDLK_KP_6:         return K_KP_RIGHTARROW;
    case SDLK_KP_1:         return K_KP_END;
    case SDLK_KP_2:         return K_KP_DOWNARROW;
    case SDLK_KP_3:         return K_KP_PGDN;
    case SDLK_KP_0:         return K_KP_INS;
    case SDLK_KP_PERIOD:    return K_KP_DEL;
    case SDLK_KP_ENTER:     return K_KP_ENTER;
    case SDLK_KP_DIVIDE:    return K_KP_SLASH;
    case SDLK_KP_MINUS:     return K_KP_MINUS;
    case SDLK_KP_PLUS:      return K_KP_PLUS;
    case SDLK_NUMLOCKCLEAR: return K_KP_NUMLOCK;
    case SDLK_KP_MULTIPLY:  return K_KP_STAR;
    case SDLK_KP_EQUALS:    return K_KP_EQUALS;

    default:

        if (sym >= 0x20 && sym < 0x7f)
            return (int)sym;
        return 0;
    }
}

static int sdl_button_to_keynum(Uint8 button)
{
    switch (button) {
    case SDL_BUTTON_LEFT:   return K_MOUSE1 + 0;
    case SDL_BUTTON_RIGHT:  return K_MOUSE1 + 1;
    case SDL_BUTTON_MIDDLE: return K_MOUSE1 + 2;
    case SDL_BUTTON_X1:     return K_MOUSE1 + 3;
    case SDL_BUTTON_X2:     return K_MOUSE1 + 4;
    default:                return 0;
    }
}

static const CCallOfDutyEngine *get_engine(void)
{

    static unsigned char linux_engine_storage[0x24];

    return (const CCallOfDutyEngine *)linux_engine_storage;
}

int SDL_PumpInputEvents(void)
{
    SDL_Event ev;
    int eventCount;
    int inputEventCount = 0;
    static int textInputStarted;

    if (!textInputStarted && !SDL_IsTextInputActive()) {
        SDL_StartTextInput();
        textInputStarted = 1;
    }

    for (eventCount = 0; eventCount < 256 && SDL_PollEvent(&ev); ++eventCount) {
        switch (ev.type) {
        case SDL_MOUSEMOTION: {
            CL_MouseEventAbsolute(ev.motion.x, ev.motion.y,
                                  ev.motion.xrel, ev.motion.yrel);
            ++inputEventCount;
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            int k = sdl_button_to_keynum(ev.button.button);
            if (k) {
                Sys_QueEvent(0, SE_KEY, k, 1, 0, 0);
                ++inputEventCount;
            }
            break;
        }
        case SDL_MOUSEBUTTONUP: {
            int k = sdl_button_to_keynum(ev.button.button);
            if (k) {
                Sys_QueEvent(0, SE_KEY, k, 0, 0, 0);
                ++inputEventCount;
            }
            break;
        }
        case SDL_MOUSEWHEEL: {
            Point point = { 0, 0 };
            CCallOfDutyEngine_DoMouseWheel(get_engine(), ev.wheel.y, point, 0);
            ++inputEventCount;
            break;
        }
        case SDL_KEYDOWN: {
            int k = sdl_to_keynum(ev.key.keysym.sym);
            if (k) {
                Sys_QueEvent(0, SE_KEY, k, 1, 0, 0);
                ++inputEventCount;
            }

            if (ev.key.keysym.sym == SDLK_BACKSPACE) {
                CCallOfDutyEngine_DoTextInput(get_engine(), 8);
                ++inputEventCount;
            }
            break;
        }
        case SDL_KEYUP: {
            int k = sdl_to_keynum(ev.key.keysym.sym);
            if (k) {
                Sys_QueEvent(0, SE_KEY, k, 0, 0, 0);
                ++inputEventCount;
            }
            break;
        }
        case SDL_TEXTINPUT:

            if (ev.text.text[0]) {
                CCallOfDutyEngine_DoTextInput(get_engine(),
                                              (unsigned char)ev.text.text[0]);
                ++inputEventCount;
            }
            break;
#if COD2_FEATURE_GAMEPAD
        case SDL_CONTROLLERDEVICEADDED:

            CL_Gamepad_HandleDeviceEvent(ev.type, ev.cdevice.which);
            break;
        case SDL_CONTROLLERDEVICEREMOVED:

            CL_Gamepad_HandleDeviceEvent(ev.type, ev.cdevice.which);
            break;
#endif
        }
    }

    return inputEventCount;
}

void IN_Frame(void)
{
    LinuxInputEvent event;
    int eventCount;
    static int suppressNativeInputFrames;

#ifndef __EMSCRIPTEN__
    /* The browser framework owns pointer lock and its trusted user gesture.
     * SDL still delivers movement/buttons, but must not request/release a
     * second lock independently when native UI state changes. */
    {
        extern void *imp_cl;
        extern SDL_Window *sdl_gl_window;
        byte *cl = imp_cl ? *(byte **)imp_cl : NULL;
        int keyCatchers = cl ? *(int *)(cl + 4) : -1;
        static int grabbed = -1;
        int wantGrab = (cl && keyCatchers == 0) ? 1 : 0;
        if (wantGrab != grabbed) {
            if (sdl_gl_window)
                SDL_SetWindowGrab(sdl_gl_window, wantGrab ? SDL_TRUE : SDL_FALSE);
            SDL_SetRelativeMouseMode(wantGrab ? SDL_TRUE : SDL_FALSE);
            grabbed = wantGrab;
        }
    }
#endif

    if (SDL_PumpInputEvents() > 0) {
        suppressNativeInputFrames = 12;
    }

#if COD2_FEATURE_GAMEPAD

    CL_Gamepad_Frame();
#endif

#ifdef __EMSCRIPTEN__
    /* Browser input arrives exclusively through SDL; there is no X11 queue. */
    return;
#else
    if (suppressNativeInputFrames > 0) {

        for (eventCount = 0;
             eventCount < 256 && Linux_PollInputEvent(&event);
             ++eventCount) {
        }
        --suppressNativeInputFrames;
        return;
    }

    for (eventCount = 0;
         eventCount < 256 && Linux_PollInputEvent(&event);
         ++eventCount) {
        switch (event.type) {
        case 1:
        {
            CL_MouseEventAbsolute(event.x, event.y, event.dx, event.dy);
            break;
        }
        case 2:
            Sys_QueEvent(0, SE_KEY, event.value, 1, 0, 0);
            break;
        case 3:
            Sys_QueEvent(0, SE_KEY, event.value, 0, 0, 0);
            break;
        case 4:
            if (event.value)
                Sys_QueEvent(0, SE_KEY, event.value, 1, 0, 0);
            if (event.value2)
                Sys_QueEvent(0, SE_CHAR, event.value2, 0, 0, 0);
            break;
        case 5:
            Sys_QueEvent(0, SE_KEY, event.value, 0, 0, 0);
            break;
        case 6:
        {
            Point point = { 0, 0 };
            CCallOfDutyEngine_DoMouseWheel(get_engine(), event.value, point, 0);
            break;
        }
        }
    }
#endif
}
