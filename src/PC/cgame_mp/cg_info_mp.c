#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"

#include <ctype.h>

extern void I_strncpyz(char *dest, const char *src, int destsize);
extern const char *va(const char *format, ...);
extern void Com_Printf(const char *msg, ...);
extern void SCR_UpdateScreen(void);
extern void CG_CloseScriptMenu(void);
extern void CL_CloseAllMenus(void);
extern void SND_StopSounds(int flags);
extern const char *CL_GetConfigString(int index);
extern const char *Info_ValueForKey(const char *s, const char *key);
extern const char *Dvar_GetString(const char *dvarName);
extern int Dvar_GetInt(const char *dvarName);
extern void UI_DrawMapLevelshot(void);
extern int Sys_Milliseconds(void);
extern void *UI_GetFontHandle(int fontEnum, float scale);
extern int UI_TextWidth(const char *text, int maxChars, void *font, float scale);
extern const char *UI_SafeTranslateString(const char *ref);
extern void UI_DrawText(const char *text, int maxChars, void *font, float x, float y, int horzAlign, int vertAlign, float scale, const float *color, int style);
extern void UI_DrawLoadBar(float x, float y, float w, float h, int horzAlign, int vertAlign, const vec_t *color, MaterialHandle material);
extern void UI_FillRect(float x, float y, float width, float height, int horzAlign, int vertAlign, const vec_t *color);

extern void *imp_legacyHacks;
extern void *imp_colorWhite;
extern void *imp_g_waitingForServer;

#define CG_FIELD_5C 0x5c
#define CG_FIELD_9C 0x9c
#define CG_FIELD_DC 0xdc
#define CG_FIELD_4ED 0x4ed

extern int ___tolower(int c);

void CG_LoadingString(const char *s);
void CG_DrawInformation(qboolean serverLoading);

void CG_LoadingString(const char *s)
{

    I_strncpyz(cg->infoScreenText, s, sizeof(cg->infoScreenText));

    if (s == NULL || *s == '\0') {

        SCR_UpdateScreen();
        return;
    }

    Com_Printf(va("LOADING... %s\n", s));

    SCR_UpdateScreen();
}

void CG_DrawInformation(qboolean serverLoading)
{
    char string[0x400];
    void *font;
    char **lh = (char **)imp_legacyHacks;
    char *base;
    char *pos;
    const char *configStr;
    const char *mapname;
    const char *dots;
    const char *translated;
    int textWidth;
    int ms;
    int phase;
    float x;

    if (serverLoading) {

        base = *lh;
        if (*(char *)(base + CG_FIELD_DC) == 0) {

            *(char *)(base + CG_FIELD_4ED) = 0;

            CG_CloseScriptMenu();
            CG_CloseScriptMenu();

            base = *lh;
            *(char *)(base + CG_FIELD_4ED) = 1;

            CL_CloseAllMenus();

            SND_StopSounds(0);

            base = *lh;
            *(char *)(base + CG_FIELD_DC) = 1;
        }
    } else {

        base = *lh;
        *(char *)(base + CG_FIELD_5C) = 0;
        *(char *)(base + CG_FIELD_9C) = 0;
        *(char *)(base + CG_FIELD_DC) = 0;
    }

    configStr = CL_GetConfigString(0);

    if (serverLoading) {

        base = *lh;
        I_strncpyz(string, base + 0x5c, 0x400);
    } else {

        mapname = Info_ValueForKey(configStr, "mapname");

        if (*mapname == '\0') {

            mapname = Dvar_GetString("mapname");
        }

        I_strncpyz(string, mapname, 0x400);
    }

    pos = string;
    while (*pos) {
        *pos = (char)___tolower((int)*pos);
        pos++;
    }

    UI_DrawMapLevelshot();

    if (Dvar_GetInt("com_expectedhunkusage") > 0)
        return;

    if (!serverLoading)
        return;

    if (!*(Bool *)imp_g_waitingForServer)
        return;

    font = UI_GetFontHandle(0, 0.5f);

    ms = Sys_Milliseconds();

    phase = (ms / 750) & 3;

    switch (phase) {
    case 0:
        dots = "";
        break;
    case 1:
        dots = ".";
        break;
    case 2:
        dots = "..";
        break;
    default:
        dots = "...";
        break;
    }

    translated = UI_SafeTranslateString("CGAME_WAITINGFORSERVERLOAD");
    textWidth = UI_TextWidth(translated, 0x7fffffff, font, 0.5f);
    x = (640.0f - (float)textWidth) * 0.5f;
    UI_DrawText(va("%s%s", translated, dots), 0x7fffffff, font, x, 452.0f, 0, 0, 0.5f, (const vec_t *)imp_colorWhite, 3);
}
