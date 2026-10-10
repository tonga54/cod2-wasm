#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#if !defined(_WIN32) || defined(W32_CLIENT)
#    include <SDL2/SDL.h>
#    ifdef __EMSCRIPTEN__
#        include <GLES3/gl3.h>
#    elif defined(_WIN32)

#        define GL_COLOR_BUFFER_BIT 0x00004000
#        define GL_DEPTH_BUFFER_BIT 0x00000100
#        include <SDL2/SDL_syswm.h>
#        include <windows.h>
#    else
#        include <SDL2/SDL_syswm.h>
#        include <GL/gl.h>
#        include <X11/Xutil.h>
#        include <X11/keysym.h>
#    endif
#endif
#include "stubs/gcc40_compat.h"

static unsigned int bsd_rune_data[13 + 256];

int ___tolower(int c)
{
    return tolower(c);
}
int ___maskrune(int c, unsigned long f)
{

    if ((unsigned int)c > 0xFF)
        return 0;
    unsigned int *rt = &bsd_rune_data[13];
    return rt[(unsigned char)c] & f;
}

#ifndef _WIN32
int pthread_main_np(void)
{
    return 1;
}
#endif

long long UpTime(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (long long)t.tv_sec * 1000000000LL + t.tv_nsec;
}
long long AddDurationToAbsolute(int duration, long long absTime)
{
    /* Carbon Duration uses milliseconds when positive, microseconds when
     * negative. AbsoluteTime is represented as monotonic nanoseconds here. */
    return absTime + (duration >= 0 ? (long long)duration * 1000000LL
                                   : -(long long)duration * 1000LL);
}
int MPDelayUntil(const long long *expiration)
{
    if (!expiration) return -50;
    long long remaining = *expiration - UpTime();
    if (remaining <= 0) return 0;
#ifdef __EMSCRIPTEN__
    /* A synchronous OS sleep cannot yield this browser main thread. */
    return -4;
#else
    struct timespec delay = { remaining / 1000000000LL, remaining % 1000000000LL };
    while (nanosleep(&delay, &delay) != 0) {
        if (errno != EINTR) return -1;
    }
    return 0;
#endif
}
void Microseconds(long long *us)
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    *us = (long long)tv.tv_sec * 1000000LL + tv.tv_usec;
}

void *NewPtrClear(long size)
{
    return calloc(1, size);
}
void DisposePtr(void *p)
{
    free(p);
}

int OTAtomicAdd32(int val, volatile int *addr)
{
    return cod2_sync_add_and_fetch_i32(addr, val);
}
unsigned char OTCompareAndSwap32(int oldVal, int newVal, volatile int *addr)
{
    return cod2_sync_bool_compare_and_swap_i32(addr, oldVal, newVal);
}

/* InterlockedExchangeAdd/Exchange/CompareExchange have real (identical,
 * cod2_sync_*-backed) defs in Mac/Tools/MacWin32.c; defining them here too is
 * what required /FORCE:MULTIPLE, so they're dropped. */

int ___isnanf(float f)
{
    return isnan(f);
}

int ___toupper(int c)
{
    return toupper(c);
}

void *___sF[3] = { 0, 0, 0 };

void *__DefaultRuneLocale;

#ifdef _MSC_VER
COD2_CONSTRUCTOR(init_rune_locale)
#else
__attribute__((constructor)) static void init_rune_locale(void)
#endif
{
    unsigned int *rt = &bsd_rune_data[13];
    int i;

    __DefaultRuneLocale = bsd_rune_data;

    for (i = 0; i < 0x20; i++)
        rt[i] = 0x0200;
    rt[0x7f] = 0x0200;

    rt['\t'] |= 0x4000 | 0x20000 | 0x08 | 0x80;
    rt['\n'] |= 0x4000 | 0x08;
    rt['\v'] |= 0x4000 | 0x08;
    rt['\f'] |= 0x4000 | 0x08;
    rt['\r'] |= 0x4000 | 0x08;

    rt[' '] = 0x4000 | 0x20000 | 0x40000 | 0x08 | 0x80;

    for (i = '0'; i <= '9'; i++)
        rt[i] = 0x0400 | 0x0004 | 0x0800 | 0x40000 | 0x10000 | 0x40;

    for (i = 'A'; i <= 'Z'; i++) {
        rt[i] = 0x0100 | 0x8000 | 0x0800 | 0x40000 | 0x01;
        if (i <= 'F')
            rt[i] |= 0x10000 | 0x40;
    }

    for (i = 'a'; i <= 'z'; i++) {
        rt[i] = 0x0100 | 0x1000 | 0x0800 | 0x40000 | 0x02;
        if (i <= 'f')
            rt[i] |= 0x10000 | 0x40;
    }

    for (i = '!'; i <= '/'; i++)
        rt[i] = 0x2000 | 0x0800 | 0x40000 | 0x10;
    for (i = ':'; i <= '@'; i++)
        rt[i] = 0x2000 | 0x0800 | 0x40000 | 0x10;
    for (i = '['; i <= '`'; i++)
        rt[i] = 0x2000 | 0x0800 | 0x40000 | 0x10;
    for (i = '{'; i <= '~'; i++)
        rt[i] = 0x2000 | 0x0800 | 0x40000 | 0x10;
}

extern void *sDisplayList[3];   /* matches the real def (void*[3]); was [12] (x86 4-byte slots) */
extern unsigned char sInWindowMode;

struct DisplayMode {
    unsigned int width;
    unsigned int height;
    unsigned int depth;
    unsigned int refreshRate;
};

static struct DisplayMode dummy_modes[] = {
    { 640, 480, 32, 60 },
    { 800, 600, 32, 60 },
    { 1024, 768, 32, 60 },
    { 1152, 864, 32, 60 },
    { 1280, 720, 32, 60 },
    { 1280, 800, 32, 60 },
    { 1280, 1024, 32, 60 },
    { 1440, 900, 32, 60 },
    { 1600, 1200, 32, 60 },
    { 1680, 1050, 32, 60 },
    { 1920, 1080, 32, 60 },
    { 1920, 1200, 32, 60 },
};

static const char dummy_gl_vendor[] = "Linux OpenGL";
static const char dummy_gl_renderer[] = "OpenGL";
static const char dummy_gl_extensions[] = "";

static unsigned char dummy_display_entry[100];

#ifdef _MSC_VER
COD2_CONSTRUCTOR(init_display_list)
#else
__attribute__((constructor)) static void init_display_list(void)
#endif
{
    int i;
    int num_modes = sizeof(dummy_modes) / sizeof(dummy_modes[0]);

    for (i = 0; i < 100; i++)
        dummy_display_entry[i] = 0;

    *(void **)&dummy_display_entry[0x14] = &dummy_modes[0];
    *(void **)&dummy_display_entry[0x18] = &dummy_modes[num_modes];

    *(const char **)&dummy_display_entry[0x30] = dummy_gl_vendor;
    *(const char **)&dummy_display_entry[0x34] = dummy_gl_renderer;
    *(const char **)&dummy_display_entry[0x38] = dummy_gl_extensions;

    *(unsigned int *)&dummy_display_entry[0x3c] = 256 * 1024 * 1024;
    *(unsigned int *)&dummy_display_entry[0x40] = 256 * 1024 * 1024;

    *(unsigned int *)&dummy_display_entry[0x44] = 8;
    *(unsigned int *)&dummy_display_entry[0x48] = 8;

    *(unsigned int *)&dummy_display_entry[0x4c] = 0xffff0200;

    sInWindowMode = 1;

    /* array indexing -> arch-correct slot offsets (0/8/16 on x64, 0/4/8 on x86);
     * the old &sDisplayList[0/4/8] wrote 8-byte pointers at 4-byte spacing on x64
     * and corrupted each other -> sDisplayList[0] read back as garbage. */
    sDisplayList[0] = dummy_display_entry;
    sDisplayList[1] = dummy_display_entry + 100;
    sDisplayList[2] = dummy_display_entry + 100;
}

#if !defined(_WIN32) || defined(W32_CLIENT)
SDL_Window *sdl_gl_window = NULL;
int sdl_gl_width = 640;
int sdl_gl_height = 480;

static int sdl_quit_watch(void *ud, SDL_Event *e)
{
    (void)ud;
    if (e->type == SDL_QUIT)
        _exit(0);
    return 0;
}
static SDL_GLContext sdl_gl_context = NULL;

#    ifdef _WIN32

static WNDPROC g_sdlWndProc;
static WNDPROC g_engineWndProc;

static LRESULT CALLBACK CoD2_InputForward(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_CHAR:
    case WM_SYSCHAR:
    case WM_DEADCHAR:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        if (g_engineWndProc)
            CallWindowProcA(g_engineWndProc, h, msg, wp, lp);
        break;
    default:
        break;
    }
    return g_sdlWndProc ? CallWindowProcA(g_sdlWndProc, h, msg, wp, lp)
                        : DefWindowProcA(h, msg, wp, lp);
}

static void Win32_BridgeInputToEngine(SDL_Window *win)
{
    SDL_SysWMinfo wm;
    WNDCLASSEXA wc;
    HWND hwnd;

    if (g_engineWndProc)
        return;
    SDL_VERSION(&wm.version);
    if (!SDL_GetWindowWMInfo(win, &wm) || wm.subsystem != SDL_SYSWM_WINDOWS)
        return;
    hwnd = wm.info.win.window;

    memset(&wc, 0, sizeof wc);
    wc.cbSize = sizeof wc;
    if (GetClassInfoExA(GetModuleHandleA(NULL), "CoD2", &wc) && wc.lpfnWndProc) {
        g_engineWndProc = wc.lpfnWndProc;
        g_sdlWndProc = (WNDPROC)(LONG_PTR)SetWindowLongPtrA(
            hwnd, GWLP_WNDPROC, (LONG_PTR)CoD2_InputForward);
        fprintf(stderr, "input bridge: hwnd=%p -> engine 'CoD2' WndProc=%p (sdl=%p)\n",
                (void *)hwnd, (void *)g_engineWndProc, (void *)g_sdlWndProc);
    } else {
        fprintf(stderr, "input bridge: engine 'CoD2' window class not found\n");
    }
}
#    endif

#    if !defined(__EMSCRIPTEN__) && !defined(_WIN32)
typedef struct LinuxInputEvent {
    int type;
    int value;
    int value2;
    int x;
    int y;
    int dx;
    int dy;
} LinuxInputEvent;

static Display *linux_x11_display;
static Window linux_x11_window;
static GC linux_x11_gc;
static XImage *linux_x11_image;
static unsigned char *linux_gl_pixels;
static int linux_present_width;
static int linux_present_height;
static int linux_last_mouse_x;
static int linux_last_mouse_y;
static int linux_have_mouse_pos;

static int Linux_InitX11Window(void)
{
    SDL_SysWMinfo wmInfo;

    if (linux_x11_display && linux_x11_window)
        return 1;
    if (!sdl_gl_window)
        return 0;

    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(sdl_gl_window, &wmInfo) ||
        wmInfo.subsystem != SDL_SYSWM_X11) {
        return 0;
    }

    linux_x11_display = wmInfo.info.x11.display;
    linux_x11_window = wmInfo.info.x11.window;
    linux_x11_gc = XCreateGC(linux_x11_display, linux_x11_window, 0, NULL);
    XSelectInput(linux_x11_display, linux_x11_window,
                 KeyPressMask | KeyReleaseMask | ButtonPressMask |
                     ButtonReleaseMask | PointerMotionMask | FocusChangeMask |
                     StructureNotifyMask);
    return linux_x11_gc != 0;
}

static int Linux_X11KeyNum(KeySym sym)
{
    switch (sym) {
    case XK_Tab:
        return 0x09;
    case XK_Return:
        return 0x0d;
    case XK_Escape:
        return 0x1b;
    case XK_space:
        return 0x20;
    case XK_BackSpace:
        return 0x7f;
    case XK_Up:
        return 0x9a;
    case XK_Down:
        return 0x9b;
    case XK_Left:
        return 0x9c;
    case XK_Right:
        return 0x9d;
    case XK_Alt_L:
    case XK_Alt_R:
        return 0x9e;
    case XK_Control_L:
    case XK_Control_R:
        return 0x9f;
    case XK_Shift_L:
    case XK_Shift_R:
        return 0xa0;
    case XK_Insert:
        return 0xa1;
    case XK_Delete:
        return 0xa2;
    case XK_Next:
        return 0xa3;
    case XK_Prior:
        return 0xa4;
    case XK_Home:
        return 0xa5;
    case XK_End:
        return 0xa6;
    case XK_F1:
        return 0xa7;
    case XK_F2:
        return 0xa8;
    case XK_F3:
        return 0xa9;
    case XK_F4:
        return 0xaa;
    case XK_F5:
        return 0xab;
    case XK_F6:
        return 0xac;
    case XK_F7:
        return 0xad;
    case XK_F8:
        return 0xae;
    case XK_F9:
        return 0xaf;
    case XK_F10:
        return 0xb0;
    case XK_F11:
        return 0xb1;
    case XK_F12:
        return 0xb2;
    default:
        if (sym >= 0x20 && sym < 0x7f) {
            if (sym >= 'A' && sym <= 'Z')
                sym += 'a' - 'A';
            return (int)sym;
        }
        return 0;
    }
}

int Linux_PollInputEvent(LinuxInputEvent *out)
{
    XEvent event;

    if (!out || !Linux_InitX11Window() || XPending(linux_x11_display) <= 0)
        return 0;

    XNextEvent(linux_x11_display, &event);
    memset(out, 0, sizeof(*out));

    switch (event.type) {
    case MotionNotify:
        out->type = 1;
        out->x = event.xmotion.x;
        out->y = event.xmotion.y;
        if (linux_have_mouse_pos) {
            out->dx = out->x - linux_last_mouse_x;
            out->dy = out->y - linux_last_mouse_y;
        }
        linux_last_mouse_x = out->x;
        linux_last_mouse_y = out->y;
        linux_have_mouse_pos = 1;
        return 1;
    case ButtonPress:
    case ButtonRelease:
        if (event.type == ButtonPress &&
            (event.xbutton.button == 4 || event.xbutton.button == 5)) {
            out->type = 6;
            out->value = event.xbutton.button == 4 ? 1 : -1;
            return 1;
        }
        if (event.xbutton.button >= 1 && event.xbutton.button <= 3) {
            static const int buttonKeys[3] = { 0xc8, 0xca, 0xc9 };
            out->type = event.type == ButtonPress ? 2 : 3;
            out->value = buttonKeys[event.xbutton.button - 1];
            return 1;
        }
        return 0;
    case KeyPress: {
        char text[8];
        KeySym sym;
        int textLen = XLookupString(&event.xkey, text, sizeof(text), &sym, NULL);
        out->type = 4;
        out->value = Linux_X11KeyNum(sym);
        if (textLen > 0)
            out->value2 = (unsigned char)text[0];
        return out->value != 0 || out->value2 != 0;
    }
    case KeyRelease:
        out->type = 5;
        out->value = Linux_X11KeyNum(XLookupKeysym(&event.xkey, 0));
        return out->value != 0;
    default:
        return 0;
    }
}
#    endif

void SDL_GL_SwapWindowDirect(void)
{
#    if defined(__EMSCRIPTEN__) || defined(_WIN32)
    if (sdl_gl_window)
        SDL_GL_SwapWindow(sdl_gl_window);
#    else
    XWindowAttributes attrs;
    int width = sdl_gl_width;
    int height = sdl_gl_height;
    int y;

    if (!Linux_InitX11Window() ||
        !XGetWindowAttributes(linux_x11_display, linux_x11_window, &attrs)) {
        return;
    }

    width = attrs.width;
    height = attrs.height;
    if (width <= 0 || height <= 0)
        return;

    if (!linux_x11_image || width != linux_present_width ||
        height != linux_present_height) {
        if (linux_x11_image) {
            XDestroyImage(linux_x11_image);
            linux_x11_image = NULL;
        }
        free(linux_gl_pixels);
        linux_gl_pixels = NULL;

        linux_x11_image = XCreateImage(
            linux_x11_display, attrs.visual, attrs.depth, ZPixmap, 0,
            calloc((size_t)width * height, 4), width, height, 32, 0);
        linux_gl_pixels = malloc((size_t)width * height * 4);
        linux_present_width = width;
        linux_present_height = height;
    }

    if (!linux_x11_image || !linux_gl_pixels ||
        linux_x11_image->bits_per_pixel != 32) {
        return;
    }

    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE,
                 linux_gl_pixels);

    for (y = 0; y < height; ++y) {
        memcpy(linux_x11_image->data + (size_t)y * linux_x11_image->bytes_per_line,
               linux_gl_pixels + (size_t)(height - 1 - y) * width * 4,
               (size_t)width * 4);
    }

    XPutImage(linux_x11_display, linux_x11_window, linux_x11_gc,
              linux_x11_image, 0, 0, 0, 0, width, height);
    XFlush(linux_x11_display);
#    endif
}

typedef void *ContextRef;
typedef int Boolean;

ContextRef MacDisplay_CreateScreenContext(int inDepthSize, int inUseStencil,
                                          int inMultiSampleType, int inMultiSampleQuality,
                                          int inPresentationInterval, Boolean *outHasAuxBuffer)
{
    unsigned char *ctx;

    if (outHasAuxBuffer)
        *outHasAuxBuffer = 0;

#    ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    // Browser MSAA smooths silhouette/fence edges without altering textures.
    extern int Dvar_GetInt(const char *name);
    int samples = Dvar_GetInt("r_aaSamples");
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, samples > 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples > 1 ? samples : 0);
#    else

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#    endif
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, inDepthSize ? inDepthSize : 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, inUseStencil ? 8 : 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    if (!sdl_gl_window) {
#    ifdef _WIN32

        {
            WNDCLASSEXA wc;
            memset(&wc, 0, sizeof wc);
            wc.cbSize = sizeof wc;
            if (GetClassInfoExA(GetModuleHandleA(NULL), "CoD2", &wc) && wc.lpfnWndProc) {

                static SDL_Window *s_pfShare;
                RECT rc;
                HWND hwnd;
                if (!s_pfShare)
                    s_pfShare = SDL_CreateWindow("cod2_pf", 0, 0, 32, 32,
                                                 SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
                rc.left = 0;
                rc.top = 0;
                rc.right = sdl_gl_width;
                rc.bottom = sdl_gl_height;
                AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
                hwnd = CreateWindowExA(0, "CoD2", "CoD2",
                                       WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                       CW_USEDEFAULT, CW_USEDEFAULT,
                                       rc.right - rc.left, rc.bottom - rc.top,
                                       NULL, NULL, GetModuleHandleA(NULL), NULL);
                if (hwnd) {
                    if (s_pfShare) {
                        char buf[32];
                        sprintf(buf, "%p", (void *)s_pfShare);
                        SDL_SetHint(SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT, buf);
                    }
                    sdl_gl_window = SDL_CreateWindowFrom((void *)hwnd);
                    if (sdl_gl_window)
                        fprintf(stderr,
                                "Game window: engine 'CoD2' class hwnd=%p, native input WndProc=%p\n",
                                (void *)hwnd, (void *)wc.lpfnWndProc);
                    else
                        DestroyWindow(hwnd);
                }

            }
        }
#    endif
        if (!sdl_gl_window) {

            sdl_gl_window = SDL_CreateWindow("Call of Duty 2 Multiplayer",
                                             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                             sdl_gl_width, sdl_gl_height,
                                             SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
            if (!sdl_gl_window) {
                fprintf(stderr, "SDL_CreateWindow FAILED: %s\n", SDL_GetError());
                return (ContextRef)0;
            }
#    ifdef _WIN32

            Win32_BridgeInputToEngine(sdl_gl_window);
#    endif
        }
        fprintf(stderr, "Game window created: %dx%d\n", sdl_gl_width, sdl_gl_height);

        SDL_AddEventWatch(sdl_quit_watch, NULL);

        SDL_ShowWindow(sdl_gl_window);
        SDL_RaiseWindow(sdl_gl_window);
#    if SDL_VERSION_ATLEAST(2, 0, 5)
        SDL_SetWindowInputFocus(sdl_gl_window);
#    endif
        SDL_WarpMouseInWindow(sdl_gl_window, sdl_gl_width / 2, sdl_gl_height / 2);
    }

    sdl_gl_context = SDL_GL_CreateContext(sdl_gl_window);
    if (!sdl_gl_context)
        return (ContextRef)0;

    {
        int got_depth = -1, got_stencil = -1, got_db = -1;
        const char *glver, *glrend;
        SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &got_depth);
        SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &got_stencil);
        SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &got_db);
        extern void Com_Printf(const char *fmt, ...);
        /* glGetString returns a pointer; without a prototype it defaults to int
         * return -> the 8-byte pointer is truncated to 32 bits on x64. */
        extern const unsigned char *glGetString(unsigned int name);
        glver = (const char *)glGetString(0x1F02 );
        glrend = (const char *)glGetString(0x1F01 );
        (void)got_db; (void)glver; (void)glrend;
        /* These GL-info diagnostics go to stderr, which is NULL/unconnected in the
         * GUI-subsystem x64 build (no console) -> fprintf derefs a NULL FILE*. Use
         * Com_Printf (the engine console) instead, which is always valid. */
        Com_Printf("GL context: requested depth=%d stencil=%d -> got depth=%d stencil=%d doublebuf=%d\n",
                inDepthSize ? inDepthSize : 24, inUseStencil ? 8 : 0,
                got_depth, got_stencil, got_db);
        Com_Printf("GL_VERSION=%s GL_RENDERER=%s\n",
                glver ? glver : "(null)", glrend ? glrend : "(null)");
        if (got_depth > 0 && got_depth < 24)
            Com_Printf("WARNING: depth buffer is only %d bits -- expect z-fighting\n",
                    got_depth);
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SDL_GL_SwapWindow(sdl_gl_window);

#    ifndef __EMSCRIPTEN__

    {
        float fogBlack[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glFogfv(0x0B66 , fogBlack);
        glFogi(0x0B63 , 0x2601 );
        glFogf(0x0B63 , 0.0f);
        glFogf(0x0B64 , 1.0f);
    }
#    endif

    ctx = (unsigned char *)calloc(1, 16);

    *(void **)&ctx[0] = sdl_gl_context;

    return (ContextRef)ctx;
}

#    ifndef __EMSCRIPTEN__
#    endif

void MacDisplay_FadeIn(float duration)
{
}

void MacDisplay_FadeOut(float duration)
{
}

#    ifndef __EMSCRIPTEN__
#    endif

#endif
