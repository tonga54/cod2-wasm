#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include "common_types.h"
#include "imports.h"

extern Boolean gConsoleRunning;
static char sConsoleText[512];
static char sReturnedText[512];

extern void Sys_Print(const char *msg);
extern int MessageBoxA(void *hWnd, const char *lpText, const char *lpCaption, unsigned int uType);

void Sys_CreateConsole(HINSTANCE hInstance)
{
    sConsoleText[0] = '\0';
    sReturnedText[0] = '\0';
}

void Sys_DestroyConsole(void)
{

}

void Sys_ShowConsole(int visLevel, qboolean quitOnClose)
{
    gConsoleRunning = 0;

    switch (visLevel) {
    case 0:

        break;
    case 1:

        gConsoleRunning = 1;
        break;
    case 2:

        break;
    }
}

char *Sys_ConsoleInput(void)
{
#ifdef __EMSCRIPTEN__
    /* No terminal is attached to a browser client. Keyboard input arrives
     * through SDL; polling stdin would open Emscripten's blocking prompt. */
    return NULL;
#else
    static int len = 0;
    fd_set fds;
    struct timeval tv;
    int ret;
    char c;

    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    while (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
        ret = read(STDIN_FILENO, &c, 1);
        if (ret <= 0)
            break;
        if (c == '\n') {
            sConsoleText[len] = '\0';
            len = 0;
            strcpy(sReturnedText, sConsoleText);
            sConsoleText[0] = '\0';
            return sReturnedText;
        }
        if (len < (int)sizeof(sConsoleText) - 1)
            sConsoleText[len++] = c;

        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 0;
    }

    return NULL;
#endif
}

void Conbuf_AppendText(const char *pMsg)
{
    if (!pMsg)
        return;

    fputs(pMsg, stdout);
    fflush(stdout);
}

void Sys_SetErrorText(const char *buf)
{
    fprintf(stderr, "ERROR: %s\n", buf);
}
