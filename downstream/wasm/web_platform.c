#include "common_types.h"
#include <emscripten.h>

extern clientStatic_t cls;
extern clientConnection_t clientConnections[1];
extern int CL_GetKeyCatchers(void);
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
    if (clientConnections[0].state == CA_ACTIVE && CL_GetKeyCatchers() == 0) return 2;
    if (cls.rendererStarted && cls.uiStarted) return 1;
    return 0;
}

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
