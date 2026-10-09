#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include "headers/PC/cgame_mp/cg_local.h"

#include <string.h>
#include <stdlib.h>

extern const dvar_t *cg_chatHeight;
extern const dvar_t *cg_chatTime;
extern const dvar_t *cg_thirdPerson;

extern float floorf(float x);

extern const char *CL_GetConfigString(int index);
extern const char *Info_ValueForKey(const char *s, const char *key);
extern void Dvar_SetStringByName(const char *dvarName, const char *value);
extern void Dvar_SetFromStringByName(const char *dvarName, const char *value);
extern const char *GetBspExtension(void);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern void Com_DPrintf(const char *fmt, ...);
extern void Com_Error(errorParm_t code, const char *fmt, ...);
extern const char *CG_Argv(int arg);
extern int Cmd_Argc(void);
extern void Cbuf_AddText(const char *text);
extern const char *va(const char *format, ...);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern int I_stricmp(const char *s1, const char *s2);
extern qboolean CL_Popup(const char *menuName);
extern void CL_ClosePopup(const char *menuName);
extern const char *Com_Parse(const char **data_p);
extern WeaponDef *BG_GetWeaponDef(int weaponIndex);
extern void CL_SwitchFog(int fogIndex, int serverTime, int transitionTime);
extern void CL_SetFog(int fogIndex, float start, float halfDist, int r, int g, int b, float density);
extern void SND_DeactivateChannelVolumes(int prio, int fadeTime);
extern void SND_SetChannelVolumes(int prio, const float *volumes, int fadeTime);
extern void SND_DeactivateEnvironmentEffects(int prio, int fadeTime);
extern void SND_SetEnvironmentEffects(int priority, const char *roomstring, float drylevel, float wetlevel, int fademsec);
extern void SND_PlayMusicAlias(const snd_alias_t *pAlias, snd_alias_system_t system);
extern void SND_StopMusic(int fadetime);
extern void SND_FadeAllSounds(float volume, int fadetime);
extern int Load_ScriptMenu(const char *name, int flags);
extern MaterialHandle CL_RegisterMaterialNoMip(const char *name, int flags);
extern MaterialHandle CL_RegisterMaterial(const char *name, int flags);
extern struct XModel *CL_RegisterModel(const char *name);
extern struct EffectTemplate *FX_RegisterEffect(const char *fileName);
extern void CG_InitLocalEntities(void);
extern void CG_InitMarkPolys(void);
extern void FX_FreeActive(void);
extern void SND_StopSounds(int mode);
extern void CG_StartAmbient(void);
extern void Dvar_SetBool(void *dvar, int value);
extern void CL_SetADS(int value);
extern void CL_CloseAllMenus(void);
extern void CG_ScoresUp_f(void);
extern void CL_SyncTimes(void);
extern int CL_GetServerCommand(int seq);
extern int stricmp(const char *s1, const char *s2);
extern const char *SEH_LocalizeTextMessage(const char *msg, const char *context, int errType);
extern void CG_GameMessage(const char *msg);
extern void CG_BoldGameMessage(const char *msg);
extern void CG_TranslateHudElemMessage(const char *message, const char *messageType, char *hudElemString);
extern Bool CG_PlaySoundOnFirstClient(void);
extern snd_alias_t *CL_PickSoundAlias(const char *aliasname);
extern int CG_PlayClientSoundAliasByName(const char *aliasname);
extern int CL_GetLocalClientActiveCount(void);
extern void *Menus_FindByName(void *dc, const char *name);
extern void Window_AddDynamicFlags(void *window, int flags);
extern void CG_SetEquippedOffHand(int offHandIndex);
extern void CG_SelectWeaponIndex(int weaponIndex);
extern void CL_ResetPlayerMuting(int clientIndex);
extern void CL_CloseInGameMenu(void);
extern void CG_SetupWeaponDef(void);
extern void CG_RegisterItems(void);
extern void CG_NorthDirectionChanged(void);
extern qboolean CG_LoadShellShockDvars(const char *name);
extern void CG_SetShellShockParmsFromDvars(shellshock_parms_t *parms);
extern const char *UI_GetMapDisplayNameFromPartialLoadNameMatch(const char *pszMap, int *mapLoadNameLen);

#define CGUI_PTR ((char *)*(void **)imp_legacyHacks)

static qboolean CG_DvarCurrentBool(void *importPtr)
{
    const dvar_t *dvar = importPtr ? *(const dvar_t **)importPtr : NULL;
    return dvar && dvar->current.enabled;
}

void CG_ParseServerinfo(void);
void CG_ParseCodinfo(void);
static void __attribute_regparm__(1) CG_AddToTeamChat(const char *str);
static void CG_OpenScriptMenu(void);
void CG_CheckOpenWaitingScriptMenu(void);
void CG_CloseScriptMenu(void);
void CG_MenuShowNotify(int menuToShow);
static void __attribute_regparm__(2) CG_SetClientDvarFromServer(const char *dvarname, const char *value);
void CG_ParseFog(void);
static void CG_DeactivateChannelVolCmd(void);
static void CG_SetChannelVolCmd(void);
static void CG_DeactivateReverbCmd(void);
void CG_SetConfigValues(void);
void CG_MapRestart(qboolean savepersist);
void CG_ServerCommand(void);
void CG_ExecuteNewServerCommands(int latestSequence);

void CG_ParseServerinfo(void)
{
    const char *info;
    const char *val;
    const char *mapname;
    const char *ext;

    info = CL_GetConfigString(0);

    val = Info_ValueForKey(info, (const char *)"sv_hostname");
    strncpy(cgs->szHostName, val, 0x100);

    val = Info_ValueForKey(info, (const char *)"g_gametype");
    strncpy(cgs->gametype, val, 0x20);

    if (cgs->localServer == 0) {
        Dvar_SetStringByName((const char *)"g_gametype", cgs->gametype);
    }

    val = Info_ValueForKey(info, (const char *)"sv_maxclients");
    cgs->maxclients = atoi(val);

    mapname = Info_ValueForKey(info, (const char *)"mapname");
    ext = GetBspExtension();
    Com_sprintf(cgs->mapname, 0x40, (const char *)"maps/mp/%s.%s", mapname, ext);
}

void CG_ParseCodinfo(void)
{
    int i;
    const char *key;

    if (cgs->localServer != 0) {
        return;
    }

    for (i = 0x8e; i < 0xee; i++) {
        key = CL_GetConfigString(i);
        if (*key == '\0') {
            break;
        }
        Dvar_SetFromStringByName(key, CL_GetConfigString(i + 0x60));
    }
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
static void __attribute_regparm__(1) CG_AddToTeamChat(const char *str)
{
    int chatHeight;
    int row;
    char *dst;
    int len;
    int lastcolor;
    char *lastSpace;
    const char *p;

    chatHeight = cg_chatHeight->current.integer;
    if (chatHeight == 0 || cg_chatTime->current.integer <= 0) {
        cgs->teamLastChatPos = 0;
        cgs->teamChatPos = 0;
        return;
    }

    row = cgs->teamChatPos % chatHeight;
    dst = cgs->teamChatMsgs[row];
    *dst = '\0';

    len = 0;
    lastcolor = '7';
    lastSpace = NULL;
    p = str;

    for (;;) {
        const char *savedp;
        char ch;

        if (*p == '\0') {
            break;
        }

        if (len > 0x59) {
            if (lastSpace != NULL) {
                int backDist = (int)(dst - lastSpace);
                p = p - backDist + 1;
                dst = lastSpace;
            }

            *dst = '\0';

            row = cgs->teamChatPos % chatHeight;
            cgs->teamChatMsgTimes[row] = cg->time;
            cgs->teamChatPos++;

            row = cgs->teamChatPos % chatHeight;
            dst = cgs->teamChatMsgs[row];
            *dst++ = '^';
            *dst++ = (char)lastcolor;

            len = 0;
            lastSpace = NULL;
        }

        savedp = p;
        if (*p == '^') {
            p++;
            ch = savedp[1];
            if (ch != '\0' && ch != '^' && ch > '/' && ch <= '9') {
                *dst++ = '^';
                lastcolor = (signed char)ch;
                *dst++ = ch;
                p = savedp + 2;
                continue;
            }

            len++;
            ch = *savedp;
        } else {
            p++;
            len++;
            ch = *savedp;
        }

        if (ch == ' ') {
            lastSpace = dst;
        }
        *dst++ = ch;
    }

    *dst = '\0';

    row = cgs->teamChatPos % chatHeight;
    cgs->teamChatMsgTimes[row] = cg->time;
    cgs->teamChatPos++;

    if (chatHeight < cgs->teamChatPos - cgs->teamLastChatPos) {
        cgs->teamLastChatPos = cgs->teamChatPos - chatHeight;
    }
}

static void CG_OpenScriptMenu(void)
{
    int menuIndex;
    const char *pszMenu;
    unsigned char noMouseControl;
    void **cguiSlot;
    char *cgui;
    int result;
    const char *arg2;
    static int traceCount;

    menuIndex = atoi(CG_Argv(1));
    if (traceCount < 16) {
        if (getenv("MTRACE"))
            Com_Printf("[menu-trace] client open command indexArg='%s' arg2='%s' argc=%d\n",
                       CG_Argv(1), Cmd_Argc() > 2 ? CG_Argv(2) : "", Cmd_Argc());
    }

    if ((unsigned int)menuIndex > 31) {
        Com_Printf((const char *)"Server tried to open a bad script menu index: %i\n", menuIndex);
        Cbuf_AddText(va((const char *)"cmd mr %i bad\n", menuIndex));
        return;
    }

    pszMenu = CL_GetConfigString(menuIndex + 0x4de);
    if (traceCount < 16) {
        if (getenv("MTRACE"))
            Com_Printf("[menu-trace] client menu index=%d configstring='%s'\n", menuIndex, pszMenu);
    }

    if (*pszMenu == '\0') {
        Com_Printf((const char *)"Server tried to open a non-loaded script menu index: %i\n", menuIndex);
        Cbuf_AddText(va((const char *)"cmd mr %i bad\n", menuIndex));
        return;
    }

    if (Cmd_Argc() > 2) {
        arg2 = CG_Argv(2);
        if (arg2 != NULL) {
            arg2 = CG_Argv(2);
            noMouseControl = (*arg2 != '\0') ? 1 : 0;
        } else {
            noMouseControl = 0;
        }
    } else {
        noMouseControl = 0;
    }

    cguiSlot = (void **)imp_legacyHacks;
    I_strncpyz((char *)*cguiSlot + 0x2e4, pszMenu, 0x100);
    *(int *)((char *)*cguiSlot + 0x3e4) = menuIndex;

    if (noMouseControl) {
        result = CL_Popup((const char *)"UIMENU_SCRIPT_POPUP_NO_MOUSE");
    } else {
        result = CL_Popup((const char *)"UIMENU_SCRIPT_POPUP");
    }
    if (traceCount++ < 16) {
        if (getenv("MTRACE"))
            Com_Printf("[menu-trace] client popup menu='%s' noMouse=%u result=%d\n",
                       pszMenu, noMouseControl, result);
    }

    if (result) {
        return;
    }

    *((char *)*cguiSlot + 0x2e4) = '\0';
    *(int *)((char *)*cguiSlot + 0x3e4) = -1;

    cgui = (char *)*cguiSlot;
    if ((*(unsigned char *)&((LegacyHacks *)cgui)->ui_waitingScriptMenu[0]) != '\0') {
        if (I_stricmp(pszMenu, ((char *)cgui + offsetof(LegacyHacks, ui_waitingScriptMenu[0]))) == 0) {
            return;
        }
        Cbuf_AddText(va((const char *)"cmd mr %i noop\n", *(int *)((char *)*cguiSlot + 0x4e8)));
    }

    I_strncpyz((char *)*cguiSlot + 0x3e8, pszMenu, 0x100);
    *(int *)((char *)*cguiSlot + 0x4e8) = menuIndex;
    *(unsigned char *)((char *)*cguiSlot + 0x4ec) = noMouseControl;
}

void CG_CheckOpenWaitingScriptMenu(void)
{
    void **cguiSlot = (void **)imp_legacyHacks;
    char *cgui;
    int result;
    static int traceCount;

    cgui = (char *)*cguiSlot;
    if ((*(unsigned char *)&((LegacyHacks *)cgui)->ui_waitingScriptMenu[0]) == '\0') {
        return;
    }

    strcpy(((char *)cgui + offsetof(LegacyHacks, ui_newScriptMenu[0])), ((char *)cgui + offsetof(LegacyHacks, ui_waitingScriptMenu[0])));
    cgui = (char *)*cguiSlot;
    ((LegacyHacks *)cgui)->ui_newScriptMenuIndex = ((LegacyHacks *)cgui)->ui_waitingScriptMenuIndex;

    cgui = (char *)*cguiSlot;
    if (((LegacyHacks *)cgui)->ui_waitingScriptMenuNoMouse) {
        result = CL_Popup((const char *)"UIMENU_SCRIPT_POPUP_NO_MOUSE");
    } else {
        result = CL_Popup((const char *)"UIMENU_SCRIPT_POPUP");
    }
    if (traceCount++ < 32) {
        if (getenv("MTRACE"))
            Com_Printf("[menu-trace] waiting popup menu='%s' index=%d noMouse=%u result=%d\n",
                       (char *)*cguiSlot + 0x3e8,
                       *(int *)((char *)*cguiSlot + 0x4e8),
                       *(unsigned char *)((char *)*cguiSlot + 0x4ec), result);
    }

    if (result) {

        *((char *)*cguiSlot + 0x3e8) = '\0';
        *(int *)((char *)*cguiSlot + 0x4e8) = -1;
        *(unsigned char *)((char *)*cguiSlot + 0x4ec) = 0;
    } else {

        *((char *)*cguiSlot + 0x2e4) = '\0';
        *(int *)((char *)*cguiSlot + 0x3e4) = -1;
    }
}

void CG_CloseScriptMenu(void)
{
    void **cguiSlot;

    CL_ClosePopup((const char *)"UIMENU_SCRIPT_POPUP");
    CL_ClosePopup((const char *)"UIMENU_SCRIPT_POPUP_NO_MOUSE");

    cguiSlot = (void **)imp_legacyHacks;
    *(unsigned char *)((char *)*cguiSlot + 0x1de) = 0;
    *(int *)((char *)*cguiSlot + 0x2e0) = -1;
    *((char *)*cguiSlot + 0x2e4) = '\0';
    *(int *)((char *)*cguiSlot + 0x3e4) = -1;
    *((char *)*cguiSlot + 0x3e8) = '\0';
    *(int *)((char *)*cguiSlot + 0x4e8) = -1;
    *(unsigned char *)((char *)*cguiSlot + 0x4ec) = 0;
}

void CG_MenuShowNotify(int menuToShow)
{
    void *menu;

    if ((unsigned int)menuToShow > 5) {
        return;
    }

    switch (menuToShow) {
    case 0:
        if (cg->healthFadeTime >= cg->time) {
            return;
        }
        cg->healthFadeTime = cg->time;
        if (CL_GetLocalClientActiveCount() == 1) {
            menu = Menus_FindByName((void *)imp_cgDC, (const char *)"Health");
        } else {
            menu = Menus_FindByName((void *)imp_cgDC, (const char *)"Health_mp");
        }
        break;
    case 1:
        CG_MenuShowNotify(4);
        if (cg->ammoFadeTime >= cg->time) {
            return;
        }
        cg->ammoFadeTime = cg->time;
        if (CL_GetLocalClientActiveCount() == 1) {
            menu = Menus_FindByName((void *)imp_cgDC, (const char *)"weaponinfo");
        } else {
            menu = Menus_FindByName((void *)imp_cgDC, (const char *)"weaponinfo_mp");
        }
        break;
    case 2:
        if (cg->compassFadeTime >= cg->time) {
            return;
        }
        cg->compassFadeTime = cg->time;
        if (CL_GetLocalClientActiveCount() == 1) {
            menu = Menus_FindByName((void *)imp_cgDC, (const char *)"Compass");
        } else {
            menu = Menus_FindByName((void *)imp_cgDC, (const char *)"Compass_mp");
        }
        break;
    case 3:
        if (cg->stanceFadeTime >= cg->time) {
            return;
        }
        cg->stanceFadeTime = cg->time;
        if (CL_GetLocalClientActiveCount() == 1) {
            menu = Menus_FindByName((void *)imp_cgDC, (const char *)"stance");
        } else {
            menu = Menus_FindByName((void *)imp_cgDC, (const char *)"stance_mp");
        }
        break;
    case 4:
        if (cg->offhandFadeTime >= cg->time) {
            return;
        }
        cg->offhandFadeTime = cg->time;
        if (CL_GetLocalClientActiveCount() == 1) {
            menu = Menus_FindByName((void *)imp_cgDC, (const char *)"offhandinfo");
        } else {
            menu = Menus_FindByName((void *)imp_cgDC, (const char *)"offhandinfo_mp");
        }
        break;
    case 5:
        if (cg->scoreFadeTime >= cg->time) {
            return;
        }
        cg->scoreFadeTime = cg->time;
        menu = Menus_FindByName((void *)imp_cgDC, (const char *)"objectiveinfo");
        break;
    default:
        return;
    }

    if (menu != NULL) {
        Window_AddDynamicFlags(menu, 4);
    }
}

static void __attribute_regparm__(2) CG_SetClientDvarFromServer(const char *dvarname, const char *value)
{
    if (stricmp(dvarname, "cg_respawnDeadline") == 0) {
        extern void CG_SetRespawnDeadline(int deadline);
        CG_SetRespawnDeadline(atoi(value));
    } else if (stricmp(dvarname, (const char *)"cg_objectiveText") == 0) {
        I_strncpyz(cg->objectiveText, value, sizeof(cg->objectiveText));
    } else if (stricmp(dvarname, (const char *)"cg_drawHud") == 0) {
        int hud = atoi(value);
        cg->drawHud = hud;
    } else if (stricmp(dvarname, (const char *)"g_scriptMainMenu") == 0) {
        I_strncpyz(cg->scriptMainMenu, value, sizeof(cg->scriptMainMenu));
    } else {
        Dvar_SetFromStringByName(dvarname, value);
    }
}

void CG_ParseFog(void)
{
    const char *info;
    const char *token;
    double halfDist_d, val3_d, val4_d, val5_d, val6_d;
    float start;
    int transitionTime;

    info = CL_GetConfigString(12);

    token = Com_Parse(&info);
    start = (float)atof(token);

    token = Com_Parse(&info);
    if (token == NULL || *token == '\0') {

        CL_SwitchFog(0, cg->time, (int)start);
        return;
    }

    halfDist_d = atof(token);

    val3_d = atof(Com_Parse(&info));
    val4_d = atof(Com_Parse(&info));
    val5_d = atof(Com_Parse(&info));
    val6_d = atof(Com_Parse(&info));

    token = Com_Parse(&info);
    transitionTime = atoi(token);

    CL_SetFog(1, start, (float)halfDist_d,
              (int)(unsigned char)(int)floorf((float)val4_d * 255.0f + 0.5f),
              (int)(unsigned char)(int)floorf((float)val5_d * 255.0f + 0.5f),
              (int)(unsigned char)(int)floorf((float)val6_d * 255.0f + 0.5f),
              (float)val3_d);

    CL_SwitchFog(1, cg->time, transitionTime);
}

static void CG_DeactivateChannelVolCmd(void)
{
    int argc;
    int prio;
    int fadeTime;
    float fval;

    argc = Cmd_Argc();
    if (argc != 3) {
        Com_Printf((const char *)"ERROR: CG_DeactivateChannelVolCmd called with %i args (should be 3)\n", argc);
        return;
    }

    prio = atoi(CG_Argv(1));
    fval = (float)(int)floorf((float)atof(CG_Argv(2)) * 1000.0f + 0.5f);
    fadeTime = (fval > 0.0f) ? (int)fval : 0;

    SND_DeactivateChannelVolumes(prio, fadeTime);
}

static void CG_SetChannelVolCmd(void)
{
    int argc;
    int prio;
    int shockIndex;
    int fadeTime;
    float fval;

    argc = Cmd_Argc();
    if (argc != 4) {
        Com_Printf((const char *)"ERROR: CG_SetChannelVolCmd called with %i args (should be 4)\n", argc);
        return;
    }

    prio = atoi(CG_Argv(1));
    shockIndex = atoi(CG_Argv(2));
    fval = (float)(int)floorf((float)atof(CG_Argv(3)) * 1000.0f + 0.5f);
    fadeTime = (fval > 0.0f) ? (int)fval : 0;

    SND_SetChannelVolumes(prio, cgs->shellshockParms[shockIndex].sound.channelvolume, fadeTime);
}

static void CG_DeactivateReverbCmd(void)
{
    int argc;
    int prio;
    int fadeTime;
    float fval;

    argc = Cmd_Argc();
    if (argc != 3) {
        Com_Printf((const char *)"ERROR: CG_DeactivateReverbCmd called with %i args (should be 3)\n", argc);
        return;
    }

    prio = atoi(CG_Argv(1));
    fval = (float)(int)floorf((float)atof(CG_Argv(2)) * 1000.0f + 0.5f);
    fadeTime = (fval > 0.0f) ? (int)fval : 0;

    SND_DeactivateEnvironmentEffects(prio, fadeTime);
}

void CG_SetConfigValues(void)
{
    int i;
    const char *str;

    cgs->teamScores[0] = atoi(CL_GetConfigString(5));
    cgs->teamScores[1] = atoi(CL_GetConfigString(6));
    cgs->levelStartTime = atoi(CL_GetConfigString(13));

    CL_SwitchFog(0, cg->time, 0);

    for (i = 0x4de; i < 0x4fe; i++) {
        str = CL_GetConfigString(i);
        if (*str == '\0') {
            continue;
        }
        if (!Load_ScriptMenu(str, 7)) {
            Com_Error(ERR_DROP, (const char *)"\x15"
                                              "Could not load script menu file '%s'\n",
                      str);
        }
    }

    for (i = 0x17; i < 0x1f; i++) {
        CL_RegisterMaterialNoMip(CL_GetConfigString(i), 7);
    }

    for (i = 0x1f; i < 0x2e; i++) {
        CL_RegisterMaterial(CL_GetConfigString(i), 7);
    }

    for (i = 0x61f; i < 0x69e; i++) {
        str = CL_GetConfigString(i);
        if (*str == '\0') {
            continue;
        }
        CL_RegisterMaterialNoMip(str, 7);
    }
}

void CG_MapRestart(qboolean savepersist)
{
    char *cgui;

    if (CG_DvarCurrentBool(imp_cg_showmiss)) {
        Com_Printf((const char *)"CG_MapRestart\n");
    }

    cg->centerPrintTime = 0;
    cg->cursorHintFade = 0;
    cg->lastHealthLerpDelay = 1;

    CG_InitLocalEntities();
    CG_InitMarkPolys();
    FX_FreeActive();

    cgs->voteTime = 0;

    cg->mapRestart = 1;

    SND_StopSounds(0);
    CG_StartAmbient();

    cg->latchVictorySound = 0;
    cg->v_dmg_time = 0;

    memset(cg->viewDamage, 0, sizeof(cg->viewDamage));

    *(int *)&cg->fWeapSelectFrac[0] = 0;
    *(int *)&cg->fWeapSelectFrac[1] = 0;
    *(int *)&cg->fWeapSelectFrac[2] = 0;
    cg->iWeapSelectLastDrawTime = 0;

    Dvar_SetBool(cg_thirdPerson, 0);

    cgui = CGUI_PTR;
    *(int *)(cgui + 8) = 0;

    CL_SetADS(0);

    if (!savepersist) {
        cgui = CGUI_PTR;
        ((LegacyHacks *)cgui)->ui_scriptMenuAllowResponse  = 0;

        CG_CloseScriptMenu();
        CG_CloseScriptMenu();

        cgui = CGUI_PTR;
        ((LegacyHacks *)cgui)->ui_scriptMenuAllowResponse  = 1;

        CL_CloseAllMenus();
    }

    CG_ScoresUp_f();
    *(unsigned char *)cg->objectiveText = 0;

    CL_SyncTimes();
}

static int CG_ParseFadeTimeArg(int argIndex)
{
    int fadeTime;

    fadeTime = (int)floorf((float)atof(CG_Argv(argIndex)) * 1000.0f + 0.5f);
    return fadeTime > 0 ? fadeTime : 0;
}

static void CG_PrintUnknownServerCommand(const char *cmd)
{
    int argc;
    int i;

    Com_Printf("Unknown client game command: %s\n", cmd ? cmd : "");
    argc = Cmd_Argc();
    if (argc <= 1) {
        return;
    }

    Com_Printf("Arguments(%i):", argc - 1);
    for (i = 1; i < argc; ++i) {
        Com_Printf(" %s", CG_Argv(i));
    }
    Com_Printf("\n");
}

static void CG_CopyCleanChatString(char *dest, int destSize, const char *src)
{
    int out;
    int in;

    if (!dest || destSize <= 0) {
        return;
    }

    out = 0;
    for (in = 0; src && src[in] && out < destSize - 1; ++in) {
        if ((unsigned char)src[in] == 0x19) {
            continue;
        }
        dest[out++] = src[in];
    }
    dest[out] = '\0';
}

static void CG_ServerCommandMessage(const char *context, qboolean bold)
{
    char message[256];

    CG_TranslateHudElemMessage(CG_Argv(1), context, message);
    if (bold) {
        CG_BoldGameMessage(message);
    } else {
        CG_GameMessage(message);
    }
}

static void CG_ServerCommandChat(const char *context, qboolean addToTeamChat)
{
    char message[150];
    const char *localized;

    localized = SEH_LocalizeTextMessage(CG_Argv(1), context, 0);
    I_strncpyz(message, localized, sizeof(message));
    CG_CopyCleanChatString(message, sizeof(message), message);

    if (addToTeamChat) {
        CG_AddToTeamChat(message);
    }
    Com_Printf("%s\n", message);
}

static void CG_ParseScores(void)
{
    int scoreCount;
    int i;

    if (!cg) {
        return;
    }

    scoreCount = atoi(CG_Argv(1));
    if (scoreCount > 64) {
        scoreCount = 64;
    }
    if (scoreCount < 0) {
        scoreCount = 0;
    }

    cg->numScores = scoreCount;
    memset(cg->teamScores, 0, sizeof(cg->teamScores));
    memset(cg->teamPings, 0, sizeof(cg->teamPings));
    memset(cg->teamPlayers, 0, sizeof(cg->teamPlayers));
    memset(cg->scores, 0, sizeof(cg->scores));

    cg->teamScores[1] = atoi(CG_Argv(2));
    cg->teamScores[2] = atoi(CG_Argv(3));

    for (i = 0; i < scoreCount; ++i) {
        int argBase;
        int client;
        int iconIndex;
        int team;
        score_t *score;

        argBase = 4 + i * 5;
        score = &cg->scores[i];
        client = atoi(CG_Argv(argBase));
        if (client < 0 || client > 63) {
            client = 0;
        }

        score->client = client;
        score->score = atoi(CG_Argv(argBase + 1));
        score->ping = atoi(CG_Argv(argBase + 2));
        score->deaths = atoi(CG_Argv(argBase + 3));
        iconIndex = atoi(CG_Argv(argBase + 4));

        if ((unsigned int)(iconIndex - 1) <= 7) {
            score->hStatusIcon = CL_RegisterMaterialNoMip(CL_GetConfigString(iconIndex + 0x16), 7);
        }

        if (cg->bgs.clientinfo[client].infoValid) {
            cg->bgs.clientinfo[client].score = score->score;
            team = cg->bgs.clientinfo[client].team;
        } else {
            team = 0;
        }

        if ((unsigned int)team < 4) {
            score->team = team;
            cg->teamPlayers[team]++;
            cg->teamPings[team] += score->ping;
        }
    }

    for (i = 0; i < 4; ++i) {
        if (cg->teamPlayers[i] > 0 && cg->teamPings[i] > 0) {
            cg->teamPings[i] /= cg->teamPlayers[i];
        } else {
            cg->teamPings[i] = 0;
        }
    }
}

static void CG_SetVoteStringFromConfigString(const char *configString)
{
    char converted[256];
    int inPos;
    int outPos;

    inPos = 0;
    outPos = 0;

    while (configString && configString[inPos] && outPos < (int)sizeof(converted) - 1) {
        int mapLoadNameLen;
        const char *displayName;

        mapLoadNameLen = 0;
        displayName = UI_GetMapDisplayNameFromPartialLoadNameMatch(configString + inPos, &mapLoadNameLen);
        if (displayName && *displayName) {
            while (*displayName && outPos < (int)sizeof(converted) - 1) {
                converted[outPos++] = *displayName++;
            }
            inPos += mapLoadNameLen;
            continue;
        }

        converted[outPos++] = configString[inPos++];
    }

    converted[outPos] = '\0';
    I_strncpyz(cgs->voteString, SEH_LocalizeTextMessage(converted, "vote string", 0), sizeof(cgs->voteString));
}

static void CG_ConfigStringModified(int index)
{
    const char *configString;
    int shellShockIndex;

    configString = CL_GetConfigString(index);
    if (index >= 15 && index <= 18)
        Com_DPrintf("[vote] config %i = %s\n", index, configString);

    switch (index) {
    case 0:
        CG_ParseServerinfo();
        return;
    case 3:
        CG_StartAmbient();
        return;
    case 5:
        cgs->teamScores[0] = atoi(configString);
        return;
    case 6:
        cgs->teamScores[1] = atoi(configString);
        return;
    case 7:
        CG_SetupWeaponDef();
        return;
    case 8:
        CG_RegisterItems();
        return;
    case 11:
        CG_NorthDirectionChanged();
        return;
    case 12:
        CG_ParseFog();
        return;
    case 13:
        cgs->levelStartTime = atoi(configString);
        return;
    case 15:
        cgs->voteTime = atoi(configString);
        return;
    case 16:
        CG_SetVoteStringFromConfigString(configString);
        return;
    case 17:
        cgs->voteYes = atoi(configString);
        return;
    case 18:
        cgs->voteNo = atoi(configString);
        return;
    default:
        break;
    }

    if ((unsigned int)(index - 0x8e) <= 0xbf) {
        CG_ParseCodinfo();
        return;
    }

    if ((unsigned int)(index - 0x14e) <= 0xff) {
        cgs->gameModels[index - 0x14e] = CL_RegisterModel(configString);
        return;
    }

    if ((unsigned int)(index - 0x34e) <= 0x3f) {
        cgs->fxs[index - 0x34e] = FX_RegisterEffect(configString);
        return;
    }

    shellShockIndex = index - 0x48e;
    if ((unsigned int)shellShockIndex <= 0xf) {
        if (*configString && CG_LoadShellShockDvars(configString)) {
            CG_SetShellShockParmsFromDvars(&cgs->shellshockParms[shellShockIndex]);
        }
        return;
    }

    if ((unsigned int)(index - 0x17) <= 7) {
        CL_RegisterMaterialNoMip(configString, 7);
        return;
    }

    if ((unsigned int)(index - 0x1f) <= 0xe) {
        CL_RegisterMaterial(configString, 7);
        return;
    }

    if ((unsigned int)(index - 0x61e) <= 0x7f && *configString) {
        CL_RegisterMaterialNoMip(configString, 7);
    }
}

static void CG_ReverbCmd(void)
{
    int argc;

    argc = Cmd_Argc();
    if (argc != 6) {
        Com_Printf("ERROR: CG_ReverbCmd called with %i args (should be 6)\n", argc);
        return;
    }

    SND_SetEnvironmentEffects(
        atoi(CG_Argv(1)),
        CG_Argv(2),
        (float)atof(CG_Argv(3)),
        (float)atof(CG_Argv(4)),
        CG_ParseFadeTimeArg(5));
}

static void CG_LocalSoundCmd(void)
{
    int argc;
    int soundIndex;

    argc = Cmd_Argc();
    if (argc != 2) {
        Com_Printf("ERROR: CG_LocalSound called with %i args (should be 2)\n", argc);
        return;
    }

    soundIndex = atoi(CG_Argv(1));
    if ((unsigned int)(soundIndex - 1) > 0xff) {
        Com_Printf("ERROR: CG_LocalSound called with index %i (should be in range[1,%i])\n", soundIndex, 0x100);
        return;
    }

    CG_PlayClientSoundAliasByName(CL_GetConfigString(soundIndex + 0x24e));
}

void CG_ServerCommand(void)
{
    const char *cmd;
    int value;

    cmd = CG_Argv(0);
    if (!cmd || !*cmd) {
        return;
    }

    switch (cmd[0]) {
    case 'B':
        CG_MapRestart(0);
        return;
    case 'C':
        value = atoi(CG_Argv(1));
        if (!value || BG_GetWeaponDef(value)) {
            CG_SetEquippedOffHand(value);
        }
        return;
    case 'D':
        CG_DeactivateReverbCmd();
        return;
    case 'E':
        CG_SetChannelVolCmd();
        return;
    case 'F':
        CG_DeactivateChannelVolCmd();
        return;
    case 'G':
        cgs->teamScores[0] = atoi(CG_Argv(1));
        return;
    case 'H':
        cgs->teamScores[1] = atoi(CG_Argv(1));
        return;
    case 'I':
        CG_MenuShowNotify(atoi(CG_Argv(1)));
        return;
    case 'J':
        CL_ResetPlayerMuting(atoi(CG_Argv(1)));
        return;
    case 'K':
        CL_CloseInGameMenu();
        return;
    case 'a':
        CG_SelectWeaponIndex(atoi(CG_Argv(1)));
        return;
    case 'b':
        CG_ParseScores();
        return;
    case 'c':
        CG_ServerCommandMessage("announcement message", 1);
        return;
    case 'd':
        CG_ConfigStringModified(atoi(CG_Argv(1)));
        return;
    case 'e':
    case 'f':
        CG_ServerCommandMessage("game message", 0);
        return;
    case 'g':
        CG_ServerCommandMessage("bold game message", 1);
        return;
    case 'h':
        if (!CG_DvarCurrentBool(imp_cg_teamChatsOnly)) {
            CG_ServerCommandChat("chat message", 1);
        }
        return;
    case 'i':
        CG_ServerCommandChat("team chat message", 1);
        return;
    case 'n':
        CG_MapRestart(1);
        return;
    case 'o':
        if (CG_PlaySoundOnFirstClient()) {
            SND_PlayMusicAlias(CL_PickSoundAlias(CG_Argv(1)), SASYS_CGAME);
        }
        return;
    case 'p':
        if (CG_PlaySoundOnFirstClient()) {
            SND_StopMusic(atoi(CG_Argv(1)));
        }
        return;
    case 'q':
        if (CG_PlaySoundOnFirstClient()) {
            SND_FadeAllSounds((float)atof(CG_Argv(1)), atoi(CG_Argv(2)));
        }
        return;
    case 'r':
        CG_ReverbCmd();
        return;
    case 's':
        CG_LocalSoundCmd();
        return;
    case 't':
        if (getenv("MTRACE"))
            Com_Printf("[menu-trace] client received server command '%s'\n", cmd);
        CG_OpenScriptMenu();
        return;
    case 'u':
        CG_CloseScriptMenu();
        return;
    case 'v': {
        char dvarName[150];
        I_strncpyz(dvarName, CG_Argv(1), sizeof(dvarName));
        if (strncmp(dvarName, "ui_allow_join", 13) == 0) {
            Com_Printf("client dvar command %s='%s'\n", dvarName, CG_Argv(2));
        }
        CG_SetClientDvarFromServer(dvarName, CG_Argv(2));
        return;
    }
    default:
        CG_PrintUnknownServerCommand(cmd);
        return;
    }
}

void CG_ExecuteNewServerCommands(int latestSequence)
{

    while (cgs->serverCommandSequence < latestSequence) {
        cgs->serverCommandSequence += 1;
        if (CL_GetServerCommand(cgs->serverCommandSequence)) {
            CG_ServerCommand();
        }
    }
}
#else
static void CG_AddToTeamChat_impl(const char *str)
{
    int chatHeight;
    int chatCount;
    int row;
    char *dst;
    int len;
    int lastcolor;
    char *ls;
    const char *p;

    chatHeight = cg_chatHeight->current.integer;
    if (chatHeight == 0)
        goto zero_out;

    if (cg_chatTime->current.integer <= 0)
        goto zero_out;

    chatCount = cgs->teamChatPos;
    row = chatCount % chatHeight;

    dst = (char *)((byte *)cgs + 0xb170 + row * 0x10f);

    dst[0xc] = '\0';

    len = 0;
    lastcolor = 0x37;
    ls = NULL;
    p = str;
    dst = (char *)((byte *)cgs + 0xb170 + row * 0x10f + 0xc);

    while (*p != '\0') {
        char ch;
        const char *savedp;

        if (len > 0x59) {

            if (ls != NULL) {

                int backDist = (int)(dst - ls);
                p = p - backDist + 1;
                dst = ls;
            }

            *dst = '\0';

            chatCount = cgs->teamChatPos;
            {
                int r = chatCount % chatHeight;
                cgs->teamChatMsgTimes[r] = cg->time;
            }

            chatCount += 1;
            cgs->teamChatPos = chatCount;

            row = chatCount % chatHeight;
            dst = (char *)((byte *)cgs + 0xb170 + row * 0x10f + 0xc);

            *dst++ = '^';
            *dst++ = (char)(lastcolor & 0xff);

            if (p == NULL) {

                ls = NULL;
                len = 1;

                ch = '\0';
                goto check_space;
            }
            len = 0;
            ls = NULL;

            if (*p == '^')
                goto handle_caret;

            goto copy_char;
        }

        savedp = p;

        if (*p == '^') {
        handle_caret:
            p++;
            ch = *(savedp + 1);
            if (ch == '\0' || ch == '^' || ch <= '/' || ch > '9') {

                len++;
                ch = *savedp;
                goto check_space;
            }

            *dst++ = '^';
            lastcolor = (signed char)ch;
            *dst++ = ch;
            p = savedp + 2;
            continue;
        }

    copy_char:

        savedp = p;
        p++;
        len++;

    check_space:

        ch = *savedp;
        if (ch == ' ')
            ls = dst;

        *dst++ = ch;
        continue;
    }

    *dst = '\0';

    chatCount = cgs->teamChatPos;
    {
        int r = chatCount % chatHeight;
        cgs->teamChatMsgTimes[r] = cg->time;
    }

    chatCount += 1;
    cgs->teamChatPos = chatCount;

    if (chatHeight < chatCount - cgs->teamLastChatPos) {

        cgs->teamLastChatPos = chatCount - chatHeight;
    }
    return;

zero_out:

    cgs->teamLastChatPos = 0;
    cgs->teamChatPos = 0;
}

#endif
