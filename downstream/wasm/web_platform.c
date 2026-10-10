#include "common_types.h"
#include <emscripten.h>

extern clientStatic_t cls;
extern clientConnection_t clientConnections[1];
extern int CL_GetKeyCatchers(void);
extern void Com_Frame(void);
extern void Key_ClearStates(void);
extern void Sys_QueEvent(int time, sysEventType_t type, int value, int value2,
                         int ptrLength, void *ptr);

EMSCRIPTEN_KEEPALIVE void web_capture_lost(void) {
    /* The framework owns pointer-lock loss. Queue a single native Escape. */
    if (clientConnections[0].state == CA_ACTIVE && CL_GetKeyCatchers() == 0) {
        Sys_QueEvent(0, SE_KEY, 27, 1, 0, NULL);
        Sys_QueEvent(0, SE_KEY, 27, 0, 0, NULL);
    }
}

/* State comes from the real engine, so a successful download/link cannot be
 * reported to the launcher as a running match. */
EMSCRIPTEN_KEEPALIVE int web_client_state(void) {
    if (clientConnections[0].state == CA_ACTIVE) {
        int catchers = CL_GetKeyCatchers();
        if (catchers == 0) return 2;
        /* Chat edits the in-game field and never needs a desktop pointer. */
        if (catchers == 0x10) return 3;
    }
    if (cls.rendererStarted && cls.uiStarted) return 1;
    return 0;
}

EMSCRIPTEN_KEEPALIVE void web_chat_char(int character) {
    /* The shell prevents these printable keys while the mouse stays captured,
     * which also cancels SDL's browser keypress event. Forward them once. */
    if (clientConnections[0].state == CA_ACTIVE && CL_GetKeyCatchers() == 0x10 &&
        (character == ' ' || character == '/'))
        Sys_QueEvent(0, SE_CHAR, character, 0, 0, NULL);
}

/* Menus/chat can be open during a match. Update notices wait until disconnect. */
EMSCRIPTEN_KEEPALIVE int web_client_connected(void) {
    return clientConnections[0].state >= CA_CONNECTING;
}

EMSCRIPTEN_KEEPALIVE void web_background_frame(void) {
    /* Use the normal frame error guard, including disconnect cleanup. */
    Com_Frame();
}

EMSCRIPTEN_KEEPALIVE void web_input_lost(void) {
    Key_ClearStates();
}

EM_JS(void, WebBackground_Init, (void), {
    let lastFrame = -Infinity;
    Module.cod2BackgroundPump = () => {
        if (!document.hidden || !Module._web_client_connected()) return;
        const now = performance.now();
        if (now - lastFrame < 250) return;
        lastFrame = now;
        Module._web_background_frame();
    };
    document.addEventListener('visibilitychange', () => {
        if (document.hidden) {
            Module._web_input_lost();
            Module.cod2BackgroundPump();
        } else {
            lastFrame = -Infinity;
        }
    });
    window.addEventListener('blur', () => Module._web_input_lost());
    /* Incoming packets also drive frames: Chrome can throttle hidden timers
     * to once a minute. This timer covers handshakes and a silent transport. */
    setInterval(() => Module.cod2BackgroundPump(), 1000);
});

/* Desktop splash windows and OS idle timers do not exist in the browser.
 * Loading progress is reported by the web launcher instead. */
void CMacGameEngine_DrawSplashScreen(const char *fileName) { (void)fileName; }
int UpdateSystemActivity(int activity) { (void)activity; return 0; }

EM_JS(void, web_open_url, (const char *url), {
    const value = UTF8ToString(url);
    const parsed = new URL(value, location.href);
    if (parsed.protocol === 'https:' || parsed.protocol === 'http:') {
        window.open(parsed.href, '_blank', 'noopener,noreferrer');
    }
});

void Sys_OpenURL(const char *url, int activate) {
    (void)activate;
    if (url && *url) web_open_url(url);
}
