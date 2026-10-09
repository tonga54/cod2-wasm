#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include "headers/PC/cgame_mp/cg_local.h"

extern const dvar_t *cg_centerPrintY;
extern const dvar_t *cg_centertime;
extern const dvar_t *cg_chatHeight;
extern const dvar_t *cg_chatTime;
extern const dvar_t *cg_crosshairAlpha;
extern const dvar_t *cg_crosshairAlphaMin;
extern const dvar_t *cg_crosshairDynamic;
extern const dvar_t *cg_crosshairEnemyColor;
extern const dvar_t *cg_draw2D;
extern const dvar_t *cg_drawCrosshair;
extern const dvar_t *cg_drawCrosshairNames;
extern const dvar_t *cg_drawCrosshairNamesPosX;
extern const dvar_t *cg_drawCrosshairNamesPosY;
extern const dvar_t *cg_drawFPS;
extern const dvar_t *cg_drawGameMessages;
extern const dvar_t *cg_drawGun;
extern const dvar_t *cg_drawLagometer;
extern const dvar_t *cg_drawMaterial;
extern const dvar_t *cg_drawScriptUsage;
extern const dvar_t *cg_drawSnapshot;
extern const dvar_t *cg_drawSoundOverlay;
extern const dvar_t *cg_drawTurretCrosshair;
extern const dvar_t *cg_hudChatPosition;
extern const dvar_t *cg_hudCompassSize;
extern const dvar_t *cg_hudSayPosition;
extern const dvar_t *cg_minicon;
extern const dvar_t *cg_nopredict;
extern const dvar_t *cg_paused;
extern const dvar_t *cg_subtitleCharHeight;
extern const dvar_t *cg_subtitlePosX;
extern const dvar_t *cg_subtitlePosY;
extern const dvar_t *cg_subtitles;
extern const dvar_t *cg_synchronousClients;

extern const dvar_t *hud_fade_ammodisplay;
extern const dvar_t *hud_fade_compass;
extern const dvar_t *hud_fade_healthbar;
extern const dvar_t *hud_fade_offhand;
extern const dvar_t *hud_fade_stance;
extern const dvar_t *hud_health_startpulse_injured;

extern Bool CG_AreHudElemsHidden(void);
extern Bool CG_AreHudMenusHidden(void);
extern void CG_Draw2dHudElems(qboolean foreground);
extern int CG_DrawBigDevString(float x, float y, const char *s, float alpha, int align);
extern qboolean CG_DrawScoreboard(void);
extern void CL_TrackStatistics(trStatistics_t *pStats);
extern void Con_DrawMiniConsole(int xPos, int yPos, float alpha);
extern void Con_DrawNotify(int xPos, int yPos, float alpha, msgwnd_mode_t mode);
extern void Con_DrawSay(int y);
extern void Con_DrawSubtitles(int xPos, int yPos, int charHeight, float alpha, msgwnd_mode_t mode);
extern void Menu_PaintAll(displayContextDef_t *dc);
extern float floorf(float x);

extern struct lagometer_t lagometer;
static int previous;
static struct trStatistics_t rendererStats;
static int fps_previousTimes[32];
static int fps_index;
static vec4_t colorWhiteFaded;
static vec4_t colorGreenFaded;
static vec4_t colorRedFaded;

extern const vec_t colorWhite[4];
extern void UI_DrawHandlePic(float x, float y, float w, float h, int horzAlign, int vertAlign, const vec_t *color, MaterialHandle material);
extern void CG_DrawStringExt(float x, float y, const char *string, const vec_t *setColor, qboolean forceColor, qboolean shadow, float charHeight, qboolean adjust);
extern void CL_LookupColor(int c, vec_t *color);
extern const char *va(const char *fmt, ...);
extern unsigned int Scr_GetNumScriptVars(void);
extern unsigned int Scr_GetNumScriptThreads(void);
extern unsigned int Scr_GetStringUsage(void);
extern int Sys_Milliseconds(void);
extern int CL_GetCurrentCmdNumber(void);
extern int CL_GetKeyCatchers(void);
extern qboolean CL_GetUserCmd(int cmdNumber, usercmd_t *ucmd);
extern Bool CL_IsRenderingSplitScreen(void);
extern void CL_SetUserCmdAimValues(const vec_t *angles);
extern void CL_SetUserCmdValue(int weapon, int offHandIndex, float sensitivity);
extern void CL_RenderScene(const void *refdef);
extern qboolean CG_DrawShellShockSavedScreenBlend(const shellshock_parms_t *parms, int start, int duration);
extern void CG_TileClear(void);
extern const char *UI_SafeTranslateString(const char *ref);
extern const char *UI_ReplaceConversionString(const char *sourceString, const char *replaceString);
extern const char *SEH_LocalizeTextMessage(const char *msg, const char *context, int errType);
extern FontHandle UI_GetFontHandle(int fontEnum, float scale);
extern int UI_TextWidth(const char *text, int maxChars, FontHandle font, float scale);
extern int UI_TextHeight(FontHandle font, float fontScale);
extern void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style);
extern MaterialHandle CL_RegisterMaterial(const char *name, int flags);
extern void CG_TranslateHudElemMessage(const char *message, const char *messageType, char *hudElemString);
extern void CG_TraceCapsule(trace_t *result, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int skipNumber, int mask);
extern int BG_GetViewmodelWeaponIndex(void *ps);
extern void *BG_GetWeaponDef(int weapIndex);
extern int BG_GetNumWeapons(void);
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material);
extern void CL_DrawStretchPicPhysical(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material);
extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);
extern void CG_DrawRotatedPic(float x, float y, float width, float height, int horzAlign, int vertAlign, float angle, const vec_t *color, MaterialHandle material);
extern qboolean CG_ScoreboardDisplayed(void);
extern void Con_DrawBoldMessages(int xPos, int yPos, float alpha, msgwnd_mode_t mode);
extern float *CG_FadeColor(int startMsec, int totalMsec, int fadeMsec);
extern void Controls_GetConfig(void);
extern int GetKeyBindingLocalizedString(const char *command, char *keys);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern char *I_CleanStr(char *string);
extern unsigned int SEH_ReadCharFromString(const char **ppsText, qboolean *pbIsTrailingPunctuation);
extern void CG_PlayerSprites(centity_t *cent);
extern qboolean CL_PickMaterial(const vec_t *org, const vec_t *dir, char *pszName, char *pszSurfaceFlags, char *pszContents, int iMaxChars);
extern int CG_DrawSmallDevStringColor(float x, float y, const char *s, const vec_t *color, int align);
extern int CG_DrawBigDevStringColor(float x, float y, const char *s, const vec_t *color, int align);

extern int SND_GetSoundOverlay(snd_overlay_type_t type, snd_overlay_info_t *info, int maxcount, int *cpu);
extern const char *Dvar_GetString(const char *dvarName);
extern int Dvar_GetInt(const char *dvarName);
extern Bool Dvar_GetBool(const char *dvarName);
extern void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);
extern float Vec3Distance(const vec_t *v1, const vec_t *v2);
extern float crandom(void);
extern double sin(double);
extern double tan(double);
extern float fmaxf(float a, float b);
extern float GetRealWidthFromVirtualWidth(float width);
extern float GetRealHeightFromVirtualHeight(float height);
extern float GetVirtualWidthFromRealWidth(float width);
extern float GetVirtualHeightFromRealHeight(float height);
extern void BG_GetSpreadForWeapon(const playerState_t *ps, int weaponIndex, float *minSpread, float *maxSpread);
extern float CG_AlignHudElemX(int alignOrg, float x, float width);
extern float CG_AlignHudElemY(int alignOrg, float y, float height);
extern void UI_FillRectPhysical(float x, float y, float width, float height, const vec_t *color);
extern void StatMon_GetStatsArray(const void **array, int *count);
extern void CL_CloseAllMenus(void);
extern void CG_DrawRotatedQuadPic(float x, float y, vec2_t *verts, float angle, const vec_t *color, MaterialHandle material);
extern void CL_DrawQuadPic(vec2_t *verts, const vec_t *color, MaterialHandle material);
extern const float vectoyaw(const vec_t *vec);
extern float AngleNormalize360(float angle);
extern float sinf(float);
extern float cosf(float);
extern float fabsf(float);

extern const vec_t colorYellow[4];
extern const vec_t colorRed[4];
extern const vec_t colorGreen[4];
extern const vec_t colorBlue[4];

extern void Menus_CloseByName(void *dc, const char *name);
extern void *Menus_FindByName(void *dc, const char *name);
extern void Window_AddDynamicFlags(void *window, int flags);
extern void Window_RemoveDynamicFlags(void *window, int flags);
extern void CG_MenuShowNotify(int menuToShow);
extern float CG_CalcPlayerHealth(void);
extern Bool CG_CheckPlayerForLowAmmo(void);
extern Bool CG_CheckPlayerForLowClip(void);
extern int CL_GetLocalClientActiveCount(void);

void CG_DrawTeamBackground(float x, float y, float w, float h, float alpha, int team);
static void CG_DrawScriptUsage(void);
void CG_CalculateFPS(void);
void CG_AddLagometerFrameInfo(void);
void CG_AddLagometerSnapshotInfo(snapshot_t *snap);
static void CG_DrawDisconnect(void);
void CG_PriorityCenterPrint(const char *str, float charWidth, int priority);
qboolean CG_GetWeapReticleZoom(float *pfZoom);
void CG_DrawFrameOverlay(float innerLeft, float innerRight, float innerTop, float innerBottom, const vec_t *color, MaterialHandle material);
static unsigned int CG_DrawCrosshairNames(void);
unsigned int CG_CheckTimedMenus(void);
static unsigned int CG_DrawSoundOverlay(void);
static unsigned int CG_DrawMaterial(void);
unsigned int CG_ShakeCamera(void);
static qboolean CG_DrawFollow(void);
void CG_DrawPlayerSprites(void);
void CG_DrawActive(void);
static unsigned int CG_DrawChatMessages(void);
static void __attribute_regparm__(2) CG_CalcCrosshairPosition(float *x, float *y);
static float CG_DrawFPS(float y);
void CG_DrawBoldGameMessages(void);
void CG_DrawTurretCrossHair(void);
static unsigned int CG_DrawSpectatorMessage(void);
void CG_StartShakeCamera(float scale, int duration, const vec_t *src, float radius);
static float CG_DrawWeapReticle(void);
unsigned int CG_DrawCrosshair(void);
unsigned int CG_Draw2D(void);

static inline __attribute__((always_inline)) qboolean CG_UpdateCameraShakeStrength(cg_t *cg, cameraShake_t *shake)
{
    float elapsed;
    float distanceScale;
    float rumbleScale;

    elapsed = (float)(cg->time - shake->time);
    if (elapsed < 0.0f || elapsed >= shake->length) {
        return 0;
    }

    distanceScale = 1.0f - Vec3Distance(shake->src, cg->refdef.vieworg) / shake->radius;
    rumbleScale = (1.0f - elapsed / shake->length) * shake->scale;
    if (rumbleScale <= 0.0f) {
        return 0;
    }

    shake->size = distanceScale < 0.0f ? distanceScale / rumbleScale : distanceScale * rumbleScale;
    shake->rumbleScale = rumbleScale;
    return 1;
}

void CG_StartShakeCamera(float scale, int duration, const vec_t *src, float radius)
{
    cameraShake_t temp;
    cameraShake_t *shake;
    int serverTime;
    int bestIndex;
    float bestSize;
    int i;

    temp.scale = scale;
    temp.length = (float)duration;
    temp.time = cg->time;
    temp.src[0] = src[0];
    temp.src[1] = src[1];
    temp.src[2] = src[2];
    temp.radius = radius;
    CG_UpdateCameraShakeStrength(cg, &temp);

    serverTime = cg->time;
    for (i = 0; i < 4; ++i) {
        shake = &cg->cameraShake[i];
        if (shake->time > serverTime)
            break;
        if ((float)serverTime >= (float)shake->time + shake->length)
            break;
    }

    if (i >= 4) {
        bestIndex = 4;
        bestSize = temp.size;
        for (i = 0; i < 4; ++i) {
            if (bestSize > cg->cameraShake[i].size) {
                bestIndex = i;
                bestSize = cg->cameraShake[i].size;
            }
        }
        if (bestIndex == 4)
            return;
        i = bestIndex;
    }

    cg->cameraShake[i] = temp;
}

void CG_DrawTeamBackground(float x, float y, float w, float h, float alpha, int team)
{
    vec4_t color;

    color[3] = alpha;
    if (team == 1) {
        color[0] = 1.0f;
        color[1] = 0.0f;
        color[2] = 0.0f;
    } else if (team == 2) {
        color[0] = 0.0f;
        color[1] = 0.0f;
        color[2] = 1.0f;
    } else {
        return;
    }

    UI_DrawHandlePic(x, y, w, h, 0, 0, color, cgs->media.teamStatusBar);
}

void CG_DrawScriptUsage(void)
{
    CG_DrawStringExt(480.0f, 80.0f, va("num vars:    %d", Scr_GetNumScriptVars()), colorWhite, 1, 1, 16.0f, 1);
    CG_DrawStringExt(480.0f, 96.0f, va("num threads: %d", Scr_GetNumScriptThreads()), colorWhite, 1, 1, 16.0f, 1);
    CG_DrawStringExt(480.0f, 112.0f, va("string usage: %d", Scr_GetStringUsage()), colorWhite, 1, 1, 16.0f, 1);
}

void CG_CalculateFPS(void)
{
    int now;
    int index;

    int delta;

    now = Sys_Milliseconds();
    delta = now - previous;
    previous = now;
    index = fps_index;
    fps_previousTimes[index % 32] = delta;
    fps_index = index + 1;
}

void CG_AddLagometerFrameInfo(void)
{
    int index = lagometer.frameCount;
    lagometer.frameSamples[index & 0x7f] = cg->time - cg->latestSnapshotTime;
    lagometer.frameCount = index + 1;
}

void CG_AddLagometerSnapshotInfo(snapshot_t *snap)
{
    int index;
    if (!snap) {
        index = lagometer.snapshotCount;
        lagometer.snapshotSamples[index & 0x7f] = -1;
        lagometer.snapshotCount = index + 1;
        return;
    }
    index = lagometer.snapshotCount;
    lagometer.snapshotSamples[index & 0x7f] = *(int *)((byte *)snap + 4);
    lagometer.snapshotFlags[index & 0x7f] = *(int *)snap;
    lagometer.snapshotCount = index + 1;
}

void CG_DrawDisconnect(void)
{
    usercmd_t cmd;
    snapshot_t *snap;
    const char *text;
    FontHandle font;
    int textWidth;
    vec4_t color = { 1.0f, 1.0f, 1.0f, 1.0f };

    CL_GetUserCmd(CL_GetCurrentCmdNumber() - 127, &cmd);
    snap = cg->nextSnap;
    if (cmd.serverTime <= snap->ps.commandTime || cmd.serverTime > cg->time) {
        return;
    }

    text = UI_SafeTranslateString("CGAME_CONNECTIONINTERUPTED");
    font = UI_GetFontHandle(0, 0.5f);
    textWidth = UI_TextWidth(text, 0, font, 0.5f);
    UI_DrawText(text, 0x7fffffff, font, (float)((640 - textWidth) / 2), 100.0f, 0, 0, 0.5f, color, 3);
    if (!(((byte *)&cg->time)[1] & 2)) {
        UI_DrawHandlePic(296.0f, 416.0f, 48.0f, 48.0f, 0, 0, 0, CL_RegisterMaterial("net_disconnect", 7));
    }
}

void CG_PriorityCenterPrint(const char *str, float charWidth, int priority)
{
    char hudElemString[256];
    const char *s;
    qboolean needNewline;
    int count;
    unsigned int letter;

    if (cg->centerPrintTime && priority < cg->centerPrintPriority) {
        return;
    }

    CG_TranslateHudElemMessage(str, "Center Print", hudElemString);
    I_strncpyz(cg->centerPrint, hudElemString, 0x100);
    cg->centerPrintPriority = priority;

    s = cg->centerPrint;
    needNewline = 0;
    count = 0;
    while (*s) {
        letter = SEH_ReadCharFromString(&s, 0);
        if (letter == '\n') {
            needNewline = 0;
            count = 0;
            continue;
        }

        ++count;
        if (count > 0x4a) {
            needNewline = 1;
        }

        if (needNewline && letter == ' ') {
            ((char *)s)[-1] = '\n';
            needNewline = 0;
            count = 0;
        }
    }

    cg->centerPrintTime = cg->time + 0x7d0;
    cg->centerPrintCharWidth = (int)charWidth;
    cg->centerPrintLines = 1;

    s = cg->centerPrint;
    while (*s) {
        letter = SEH_ReadCharFromString(&s, 0);
        if (letter == '\n') {
            ++cg->centerPrintLines;
            continue;
        }

        if (letter == '\\' && *s == 'n') {
            ++cg->centerPrintLines;
            ++s;
        }
    }
}

/* qboolean (int), not Bool: callers (cg_weapons.c, cg_predict_mp.c) declare it
 * `extern qboolean` and read full eax; a 1-byte Bool return leaves eax's upper
 * bytes dirty under MSVC -> garbage non-zero -> !CG_GetWeapReticleZoom() == 0 ->
 * drawGun never set -> the viewmodel was hidden even when spawned. */
qboolean CG_GetWeapReticleZoom(float *pfZoom)
{
    byte *weaponDef;
    float zoom;
    float zoomFrac;

    weaponDef = (byte *)BG_GetWeaponDef(BG_GetViewmodelWeaponIndex(&cg->predictedPlayerState));
    zoom = cg->predictedPlayerState.fWeaponPosFrac;
    *pfZoom = 0.0f;

    if (!*(char *)((*(int *)&((WeaponDef *)weaponDef)->szOverlayMaterial)) && !(*(int *)&((WeaponDef *)weaponDef)->overlayReticle))
    {
        return 0;
    }

    if (zoom == 0.0f) {
        return 0;
    }

    if (*(int *)((byte *)&cg->playerEntity + 4) ) {
        zoomFrac = zoom - (1.0f - ((WeaponDef *)weaponDef)->fAdsZoomInFrac );
        *pfZoom = zoomFrac;
        if (zoomFrac > 0.0f) {
            zoomFrac /= ((WeaponDef *)weaponDef)->fAdsZoomInFrac ;
            *pfZoom = zoomFrac;
        }
    } else {
        zoomFrac = zoom - (1.0f - ((WeaponDef *)weaponDef)->fAdsZoomOutFrac );
        *pfZoom = zoomFrac;
        if (zoomFrac > 0.0f) {
            zoomFrac /= ((WeaponDef *)weaponDef)->fAdsZoomOutFrac ;
            *pfZoom = zoomFrac;
        }
    }

    if (zoomFrac <= 0.01f) {
        return 0;
    }

    if (zoomFrac > 1.0f) {
        *pfZoom = 1.0f;
    }

    return 1;
}

void CG_DrawFrameOverlay(float innerLeft, float innerRight, float innerTop, float innerBottom, const vec_t *color, MaterialHandle material)
{
    byte *cls;
    float screenWidth;
    float screenHeight;

    cls = (byte *)imp_cls;
    screenWidth = (float)((clientStatic_t *)cls)->vidConfig.width ;
    screenHeight = (float)((clientStatic_t *)cls)->vidConfig.height ;

    if (innerLeft > 0.0f) {
        CL_DrawStretchPicPhysical(0.0f, 0.0f, innerLeft, screenHeight, 0.0f, 0.0f, 0.0f, 1.0f, color, material);
    }

    if (screenWidth > innerRight) {
        CL_DrawStretchPicPhysical(innerRight, 0.0f, screenWidth - innerRight, screenHeight, 0.0f, 0.0f, 0.0f, 1.0f, color, material);
    }

    if (innerTop > 0.0f) {
        CL_DrawStretchPicPhysical(innerLeft, 0.0f, innerRight - innerLeft, innerTop, 0.0f, 0.0f, 1.0f, 0.0f, color, material);
    }

    if (screenHeight > innerBottom) {
        CL_DrawStretchPicPhysical(innerLeft, innerBottom, innerRight - innerLeft, screenHeight - innerBottom, 0.0f, 0.0f, 1.0f, 0.0f, color, material);
    }
}

unsigned int CG_DrawCrosshairNames(void)
{
    trace_t trace;
    vec3_t start;
    vec3_t end;
    vec3_t traceMins = { 0.0f, 0.0f, 0.0f };
    vec3_t traceMaxs = { 0.0f, 0.0f, 0.0f };
    float *baseColor;
    clientInfo_t *localClientInfo;
    clientInfo_t *targetClientInfo;
    char *name;
    FontHandle font;
    vec4_t actualColor;
    float healthFrac;
    int targetClientNum;
    int x;
    int y;

    if (!cg_drawCrosshairNames->current.enabled) {
        return 0;
    }

    if (cg->renderingThirdPerson) {
        return 0;
    }

    start[0] = cg->refdef.vieworg[0];
    start[1] = cg->refdef.vieworg[1];
    start[2] = cg->refdef.vieworg[2];
    end[0] = start[0] + cg->refdef.viewaxis[0][0] * 8192.0f;
    end[1] = start[1] + cg->refdef.viewaxis[0][1] * 8192.0f;
    end[2] = start[2] + cg->refdef.viewaxis[0][2] * 8192.0f;
    CG_TraceCapsule(&trace, start, traceMins, traceMaxs, end, cg->snap->ps.clientNum, 0x2000001);

    if (trace.entityNum <= 63) {
        cg->crosshairClientNum = trace.entityNum;
        cg->crosshairClientTime = cg->time;
    }

    baseColor = CG_FadeColor(cg->crosshairClientTime, 150, 100);
    if (!baseColor) {
        return 0;
    }

    targetClientNum = cg->crosshairClientNum;
    if (targetClientNum < 0 || targetClientNum > 64) {
        return 0;
    }

    localClientInfo = &cg->bgs.clientinfo[cg->snap->ps.clientNum];
    if (!localClientInfo->infoValid || !localClientInfo->team) {
        return 0;
    }

    targetClientInfo = &cg->bgs.clientinfo[targetClientNum];
    if (!targetClientInfo->infoValid) {
        return 0;
    }

    if (localClientInfo->team != 3 && localClientInfo->team != targetClientInfo->team) {
        return 0;
    }

    name = va("%s", targetClientInfo->name);
    if (!name || !name[0]) {
        return 0;
    }
    I_CleanStr(name);

    x = cg_drawCrosshairNamesPosX->current.integer;
    y = cg_drawCrosshairNamesPosY->current.integer + 16;

    if (cg->crosshairClientNum != cg->identifyClientNum) {
        actualColor[0] = 1.0f;
        actualColor[1] = 1.0f;
        actualColor[2] = 1.0f;
    } else {
        healthFrac = (float)cg->identifyClientHealth / 100.0f;
        if (healthFrac < 0.0f) {
            healthFrac = 0.0f;
        } else if (healthFrac > 1.0f) {
            healthFrac = 1.0f;
        }

        actualColor[2] = 0.0f;
        if (healthFrac > 0.5f) {
            actualColor[0] = (1.0f - healthFrac) * 2.0f;
            actualColor[1] = 1.0f;
        } else {
            actualColor[0] = 1.0f;
            actualColor[1] = healthFrac * 2.0f;
        }
    }

    actualColor[3] = baseColor[3] * 0.60000002f;
    font = UI_GetFontHandle(0, 1.0f / 3.0f);
    UI_DrawText(name, 0x7fffffff, font, (float)x, (float)y, 0, 0, 1.0f / 3.0f, actualColor, 3);
    return 0;
}

unsigned int CG_CheckTimedMenus(void)
{
    int serverTime;
    int timedMenuTime;
    usercmd_t prevCmd;
    usercmd_t curCmd;
    usercmd_t prevCopy;
    usercmd_t curCopy;
    int cmdNum;
    int buttonBits;
    int buttonChanged;
    int hasChange;
    float health;
    float fadeVal;
    int showTime;
    void *menu;

    timedMenuTime = cg->voiceTime;
    if (timedMenuTime != 0) {
        serverTime = cg->time;
        if (serverTime - timedMenuTime > 2500) {

            Menus_CloseByName((void *)imp_cgDC, (const char *)"voiceMenu");
            cg->voiceTime = 0;
        }
    }

    cmdNum = CL_GetCurrentCmdNumber();
    if (cmdNum <= 1)
        goto after_buttons;

    CL_GetUserCmd(cmdNum - 1, &prevCmd);
    CL_GetUserCmd(cmdNum, &curCmd);

    buttonBits = curCmd.buttons;
    buttonChanged = buttonBits ^ prevCmd.buttons;

    prevCopy = prevCmd;
    curCopy = curCmd;

    if (memcmp(prevCopy.angles, curCopy.angles, 12) != 0) {
        hasChange = 1;
    } else if (*(unsigned short *)&curCopy.forwardmove != 0) {
        hasChange = 1;
    } else {
        hasChange = 0;
    }

    if (buttonBits & 0x1) {
        if (!(cg->predictedPlayerState.eFlags & 0x300)) {

            CG_MenuShowNotify(1);
            hasChange = 1;
            buttonBits = curCmd.buttons;
        }
    }

    if (buttonBits & 0x30) {
        if (!((byte)(cg->predictedPlayerState.pm_flags) & 0x4)) {
            snapshot_t *snap = cg->snap;
            if (!(snap->ps.cursorHint & 0x2)) {
                if (!(cg->predictedPlayerState.eFlags & 0x300)) {

                    CG_MenuShowNotify(1);
                    hasChange = 1;
                    buttonBits = curCmd.buttons;
                }
            }
        }
    }

    if (buttonBits & 0x30000) {

        CG_MenuShowNotify(4);
        hasChange = 1;
        buttonBits = curCmd.buttons;
    }

    if (buttonChanged & 0x2300) {

        CG_MenuShowNotify(3);
        goto do_notify2;
    }

    if (buttonBits & 0x2300) {

        CG_MenuShowNotify(3);
    }

    if (!hasChange) {
        if (!(buttonChanged & 0xffffd4ff))
            goto after_buttons;
    }

do_notify2:

    CG_MenuShowNotify(2);

after_buttons:

    health = CG_CalcPlayerHealth();
    {
        float threshold = hud_health_startpulse_injured->current.value;
        if (threshold <= health) {

            CG_MenuShowNotify(0);
        }
    }

    fadeVal = hud_fade_healthbar->current.value;
    if (fadeVal != 0.0f) {
        showTime = cg->healthFadeTime;
        if (showTime != 0) {
            serverTime = cg->time;
            if ((float)(serverTime - showTime) > fadeVal * 1000.0f) {

                if (CL_GetLocalClientActiveCount() == 1) {
                    menu = Menus_FindByName((void *)imp_cgDC, (const char *)"Health");
                } else {
                    menu = Menus_FindByName((void *)imp_cgDC, (const char *)"Health_mp");
                }

                if (menu)
                    Window_RemoveDynamicFlags(menu, 4);

                cg->healthFadeTime = 0;
            }
        }
    }

    if (CG_CheckPlayerForLowAmmo() || CG_CheckPlayerForLowClip()) {

        CG_MenuShowNotify(1);
    }

    fadeVal = hud_fade_ammodisplay->current.value;
    if (fadeVal != 0.0f) {
        showTime = cg->ammoFadeTime;
        if (showTime != 0) {
            serverTime = cg->time;
            if ((float)(serverTime - showTime) > fadeVal * 1000.0f) {

                if (CL_GetLocalClientActiveCount() == 1) {
                    menu = Menus_FindByName((void *)imp_cgDC, (const char *)"weaponinfo");
                } else {
                    menu = Menus_FindByName((void *)imp_cgDC, (const char *)"weaponinfo_mp");
                }

                if (menu)
                    Window_RemoveDynamicFlags(menu, 4);

                cg->ammoFadeTime = 0;
            }
        }
    }

    fadeVal = hud_fade_compass->current.value;
    if (fadeVal != 0.0f) {
        showTime = cg->compassFadeTime;
        if (showTime != 0) {
            serverTime = cg->time;
            if ((float)(serverTime - showTime) > fadeVal * 1000.0f) {

                if (CL_GetLocalClientActiveCount() == 1) {
                    menu = Menus_FindByName((void *)imp_cgDC, (const char *)"Compass");
                } else {
                    menu = Menus_FindByName((void *)imp_cgDC, (const char *)"Compass_mp");
                }

                if (menu)
                    Window_RemoveDynamicFlags(menu, 4);

                cg->compassFadeTime = 0;
            }
        }
    }

    {
        int ps_eFlags = cg->nextSnap->ps.eFlags;
        if ((ps_eFlags & 0x8) && (ps_eFlags & 0x100)) {

            CG_MenuShowNotify(3);
        } else if ((ps_eFlags & 0x4) && (ps_eFlags & 0x200)) {

            CG_MenuShowNotify(3);
        }
    }

    fadeVal = hud_fade_stance->current.value;
    if (fadeVal != 0.0f) {
        showTime = cg->stanceFadeTime;
        if (showTime != 0) {
            serverTime = cg->time;
            if ((float)(serverTime - showTime) > fadeVal * 1000.0f) {

                if (CL_GetLocalClientActiveCount() == 1) {
                    menu = Menus_FindByName((void *)imp_cgDC, (const char *)"stance");
                } else {
                    menu = Menus_FindByName((void *)imp_cgDC, (const char *)"stance_mp");
                }

                if (menu)
                    Window_RemoveDynamicFlags(menu, 4);

                cg->stanceFadeTime = 0;
            }
        }
    }

    fadeVal = hud_fade_offhand->current.value;
    if (fadeVal != 0.0f) {
        showTime = cg->offhandFadeTime;
        if (showTime != 0) {
            serverTime = cg->time;
            if ((float)(serverTime - showTime) > fadeVal * 1000.0f) {

                if (CL_GetLocalClientActiveCount() == 1) {
                    menu = Menus_FindByName((void *)imp_cgDC, (const char *)"offhandinfo");
                } else {
                    menu = Menus_FindByName((void *)imp_cgDC, (const char *)"offhandinfo_mp");
                }

                if (menu)
                    Window_RemoveDynamicFlags(menu, 4);

                cg->offhandFadeTime = 0;
            }
        }
    }

    if (!CG_ScoreboardDisplayed()) {
        return 0;
    }

    serverTime = cg->time - cg->scoreFadeTime;
    if (serverTime <= 100) {
        return serverTime;
    }

    menu = Menus_FindByName((void *)imp_cgDC, (const char *)"objectiveinfo");
    if (!menu) {
        return 0;
    }
    Window_RemoveDynamicFlags(menu, 4);
    return 0;
}

static unsigned int CG_DrawSoundOverlay(void)
{
    snd_overlay_info_t info[64];
    const char *provider;
    const char *line;
    int cpu;
    int count;
    int bits;
    int khz;
    int channels;
    int i;
    float y;

    count = SND_GetSoundOverlay(
        (snd_overlay_type_t)cg_drawSoundOverlay->current.integer,
        info,
        (int)(sizeof(info) / sizeof(info[0])),
        &cpu);
    if (count <= 0) {
        return 0;
    }

    provider = Dvar_GetString("mss_3d_provider");
    bits = Dvar_GetInt("snd_bits");
    khz = Dvar_GetInt("snd_khz");
    channels = Dvar_GetBool("snd_stereo") ? 2 : 1;

    CG_DrawStringExt(
        2.0f,
        82.0f,
        va("CPU: ^3%%%i ^73D provider: ^3%s ^7bits: ^3%i ^7kHz: ^3%i ^7chan: ^3%i", cpu, provider, bits, khz, channels),
        colorWhite,
        0,
        1,
        10.0f,
        0);

    y = 92.0f;
    for (i = 0; i < count; ++i) {
        if (info[i].pszSampleName) {
            line = va(
                "%2i %-50s vol^3%04.2f ^7rvol^3%04.2f ^7dist^3%5i ^7pit^3%04.2f",
                i,
                info[i].pszSampleName,
                info[i].fBaseVolume,
                info[i].fCurVolume,
                info[i].dist,
                info[i].fPitch);
        } else {
            line = va("%2i", i);
        }

        CG_DrawStringExt(2.0f, y, line, colorWhite, 0, 1, 10.0f, 0);
        y += 10.0f;
    }

    return 0;
}

unsigned int CG_DrawMaterial(void)
{
    char szName[0x58];
    char szSurfaceFlags[0x1000];
    char szContents[0x1000];
    float y;

    if (!CL_PickMaterial((const vec_t *)cg->refdef.vieworg, (const vec_t *)cg->refdef.viewaxis, szName, szSurfaceFlags, szContents, 0x1000)) {
        return 0;
    }

    y = 240.0f;
    y += (float)CG_DrawSmallDevStringColor(8.0f, y, szName, colorWhite, 5);
    y += (float)CG_DrawSmallDevStringColor(8.0f, y, szSurfaceFlags, colorWhite, 5);
    CG_DrawSmallDevStringColor(8.0f, y, szContents, colorWhite, 5);
    return 0;
}

unsigned int CG_ShakeCamera(void)
{
    cameraShake_t *shake;
    float sx;
    float bestSize;
    float bestRumble;
    float phase;
    int i;

    sx = (float)cg->time / 600.0f;
    bestSize = 0.0f;
    bestRumble = 0.0f;

    for (i = 0; i < 4; ++i) {
        shake = &cg->cameraShake[i];
        if (!CG_UpdateCameraShakeStrength(cg, shake)) {
            continue;
        }

        if (shake->size > bestSize) {
            bestSize = shake->size;
            bestRumble = shake->rumbleScale;
        }
    }

    if (cg->rumbleScale > bestSize) {
        bestSize = cg->rumbleScale;
        bestRumble = cg->rumbleScale;
    }

    if (bestSize <= 0.0f) {
        cg->cameraShakePhase = crandom() * 3.14159265358979323846f;
        return 0;
    }

    if (bestSize > 1.0f) {
        bestSize = 1.0f;
    }

    phase = cg->cameraShakePhase;
    cg->refdefViewAngles[0] += (float)sin((double)(sx * 25.132741228718345f + phase)) * bestRumble * 18.0f * bestSize;
    cg->refdefViewAngles[1] += (float)sin((double)(sx * 47.12388980384689f + phase)) * bestRumble * 16.0f * bestSize;
    cg->refdefViewAngles[2] += (float)sin((double)(sx * 37.69911184307752f + phase)) * bestRumble * 10.0f * bestSize;
    return 0;
}

qboolean CG_DrawFollow(void)
{
    clientInfo_t *clientInfo;
    const char *clientName;
    const char *followingString;
    FontHandle font;
    float scale;
    int textWidth;

    if (!(cg->snap->ps.pm_flags & 0x400000)) {
        return 0;
    }

    if (cg->inKillCam) {
        return 0;
    }

    clientInfo = &((clientInfo_t *)&((cg_t *)cg)->bgs.clientinfo[0].infoValid)[cg->snap->ps.clientNum];
    if (clientInfo->infoValid) {
        clientName = clientInfo->name;
    } else {
        clientName = "?";
    }

    followingString = SEH_LocalizeTextMessage("CGAME_FOLLOWING\x15", "spectator follow string", 0);
    scale = CL_IsRenderingSplitScreen() ? 0.5f : (1.0f / 3.0f);
    font = UI_GetFontHandle(0, 1.0f / 3.0f);

    textWidth = UI_TextWidth(followingString, 0, font, scale);
    UI_DrawText(followingString, 0x7fffffff, font, (float)(-textWidth) * 0.5f, 40.0f, 7, 1, scale, colorWhite, 3);

    textWidth = UI_TextWidth(clientName, 0, font, scale);
    UI_DrawText(clientName, 0x7fffffff, font, (float)(-textWidth) * 0.5f, 65.0f, 7, 1, scale, colorWhite, 3);

    return 1;
}

void CG_DrawBoldGameMessages(void)
{
    float *fade;
    float alpha;

    if (!CG_ScoreboardDisplayed()) {
        alpha = 1.0f;
    } else {
        fade = CG_FadeColor(cg->scoreFadeTime, 100, 100);
        if (!fade)
            return;
        alpha = fade[3];
    }

    Con_DrawBoldMessages(0, -60, alpha, MWM_BOTTOMUP_CENTERED);
}

void CG_DrawTurretCrossHair(void)
{
    const dvar_t *drawTurretCrosshair;
    vec4_t color = { 1.0f, 1.0f, 1.0f, 0.75f };

    drawTurretCrosshair = cg_drawTurretCrosshair;
    if (drawTurretCrosshair && !drawTurretCrosshair->current.enabled)
        return;

    if (!cgs)
        return;

    UI_DrawHandlePic(316.0f, 236.0f, 8.0f, 8.0f, 0, 0, color, cgs->media.whiteMaterial);
}

static unsigned int CG_DrawSpectatorMessage(void)
{
    const char *text;
    FontHandle font;
    float scale;
    int textWidth;

    if (!cg || !cg->snap)
        return 0;

    if (cg->snap->ps.pm_type != 5)
        return 0;

    text = UI_SafeTranslateString("CGAME_SPECTATOR");
    scale = CL_IsRenderingSplitScreen() ? 0.5f : (1.0f / 3.0f);
    font = UI_GetFontHandle(0, scale);
    textWidth = UI_TextWidth(text, 0, font, scale);
    UI_DrawText(text, 0x7fffffff, font, (float)(-textWidth) * 0.5f, 40.0f, 7, 1, scale, colorWhite, 3);

    return 1;
}

void CG_DrawPlayerSprites(void)
{
    int entityIndex;
    centity_t *cent;

    for (entityIndex = 0; entityIndex < cg->nextSnap->numEntities; ++entityIndex) {
        cent = &cg_entities[cg->nextSnap->entities[entityIndex].number];
        if (cent->nextState.eType == 1) {
            CG_PlayerSprites(cent);
        }
    }
}

void CG_DrawActive(void)
{
    float sensitivity;
    float angles[3];
    vec_t *ka;
    vec_t *oa;

    cg->refdef.time = cg->time;
    cg->refdef.viewIndex = 0;

    sensitivity = cg->zoomSensitivity;
    {
        float shellshockSensitivity = cg->shellshock.sensitivity;
        if (shellshockSensitivity != 0.0f)
            sensitivity *= shellshockSensitivity;
    }

    ka = cg->kickAngles;
    oa = cg->offsetAngles;
    angles[0] = ka[0] + oa[0];
    angles[1] = ka[1] + oa[1];
    angles[2] = ka[2] + oa[2];

    CL_SetUserCmdAimValues(angles);

    CL_SetUserCmdValue(cg->weaponSelect, cg->equippedOffHand, sensitivity);

    CL_RenderScene(&cg->refdef);

    if (!CL_IsRenderingSplitScreen()) {
        CG_DrawShellShockSavedScreenBlend(
            cg->shellshock.parms,
            cg->shellshock.startTime,
            cg->shellshock.duration);
    }

    CG_TileClear();
}

unsigned int CG_DrawChatMessages(void)
{
    FontHandle font;
    const char *msg;
    vec4_t color;
    float alphaPercent;
    float backgroundAlpha;
    float y;
    float timeRemaining;
    int chatHeight;
    int hudChatX;
    int hudChatY;
    int chatTime;
    int index;
    int i;
    int textWidth;

    chatHeight = cg_chatHeight->current.integer;
    if (!chatHeight) {
        return 0;
    }

    hudChatX = (int)cg_hudChatPosition->current.vector[0];
    hudChatY = (int)cg_hudChatPosition->current.vector[1];
    if (cgs->teamLastChatPos == cgs->teamChatPos) {
        return 0;
    }

    index = cgs->teamLastChatPos % chatHeight;
    chatTime = cg_chatTime->current.integer;
    if (cg->time - cgs->teamChatMsgTimes[index] > chatTime) {
        ++cgs->teamLastChatPos;
    }

    font = UI_GetFontHandle(0, 1.0f / 3.0f);
    for (i = cgs->teamChatPos - 1; i >= cgs->teamLastChatPos; --i) {
        index = i % chatHeight;
        timeRemaining = (float)chatTime - (float)(cg->time - cgs->teamChatMsgTimes[index]);
        if (timeRemaining > 200.0f) {
            alphaPercent = 1.0f;
            backgroundAlpha = 0.60000002f;
        } else {
            alphaPercent = timeRemaining / 200.0f;
            if (alphaPercent <= 0.0f) {
                continue;
            }

            backgroundAlpha = alphaPercent * 0.60000002f;
        }

        msg = cgs->teamChatMsgs[index];
        if (msg[0] == '^' && msg[1] && msg[1] != '^' && msg[1] >= '0' && msg[1] <= '9') {
            CL_LookupColor((unsigned char)msg[1], color);
        } else {
            color[0] = 1.0f;
            color[1] = 1.0f;
            color[2] = 1.0f;
        }

        color[0] *= 0.25f;
        color[1] *= 0.25f;
        color[2] *= 0.25f;
        color[3] = backgroundAlpha;

        y = (float)(hudChatY - (cgs->teamChatPos - i) * 10);
        textWidth = UI_TextWidth(msg, 0, font, 1.0f / 3.0f);
        UI_DrawHandlePic(0.0f, y, (float)(textWidth + 24), 10.0f, 1, 1, color, cgs->media.whiteMaterial);

        color[0] = 1.0f;
        color[1] = 1.0f;
        color[2] = 1.0f;
        color[3] = alphaPercent;
        UI_DrawText(msg, 0x7fffffff, font, (float)hudChatX, y + 9.0f, 1, 1, 1.0f / 3.0f, color, 3);
    }

    return 0;
}

static void __attribute_regparm__(2) CG_CalcCrosshairPosition(float *x, float *y)
{
    vec3_t gunAng;
    vec3_t gunDir;
    float forward;
    float horiz;
    float vert;

    gunAng[2] = cg->refdefViewAngles[2];
    gunAng[1] = cg->gunYaw;
    gunAng[0] = cg->gunPitch;
    AngleVectors(gunAng, gunDir, NULL, NULL);

    {
        const vec_t *axis = cg->refdef.viewaxis[0];
        forward = gunDir[0] * axis[0] + gunDir[1] * axis[1] + gunDir[2] * axis[2];
    }
    if (forward <= 0.0f || cg->refdef.fov_x <= 0.0f || cg->refdef.fov_y <= 0.0f) {
        *x = 0.0f;
        *y = 0.0f;
        return;
    }

    {
        const vec_t *axis = cg->refdef.viewaxis[1];
        horiz = gunDir[0] * axis[0] + gunDir[1] * axis[1] + gunDir[2] * axis[2];
    }
    *x = horiz / (forward * (float)tan((double)cg->refdef.fov_x * 0.008726646259971648)) * -320.0f;

    {
        const vec_t *axis = cg->refdef.viewaxis[2];
        vert = gunDir[0] * axis[0] + gunDir[1] * axis[1] + gunDir[2] * axis[2];
    }
    *y = vert / (forward * (float)tan((double)cg->refdef.fov_y * 0.008726646259971648)) * -240.0f;
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
static float CG_DrawFPS(float y)
{
    int i;
    int total = 0;
    int minTime = 0x7fffffff;
    int maxTime = 0;
    int avgFps;
    int minFps;
    int maxFps;
    const char *text;

    if (fps_index <= 31)
        return y;

    for (i = 0; i < 32; ++i) {
        int frameTime = fps_previousTimes[i];
        total += frameTime;
        if (frameTime < minTime)
            minTime = frameTime;
        if (frameTime > maxTime)
            maxTime = frameTime;
    }

    if (total <= 0)
        return y;

    if (minTime <= 0)
        minTime = 1;
    if (maxTime <= 0)
        maxTime = 1;

    avgFps = (int)floorf(32000.0f / (float)total + 0.5f);
    minFps = (int)floorf(1000.0f / (float)maxTime + 0.5f);
    maxFps = (int)floorf(1000.0f / (float)minTime + 0.5f);

    if (cg_drawFPS->current.integer > 2)
        text = va("%1.2fmspf(%i-%i)", (float)total * 0.03125f, minTime, maxTime);
    else
        text = va("%ifps(%i-%i)", avgFps, minFps, maxFps);

    y += (float)CG_DrawBigDevStringColor(620.0f, y, text, colorWhiteFaded, 6);

    if (cg_drawFPS->current.integer > 1) {
        CL_TrackStatistics(&rendererStats);
        y += (float)CG_DrawSmallDevStringColor(620.0f, y, "scene", colorGreenFaded, 6);
        y += (float)CG_DrawSmallDevStringColor(620.0f, y, va("verts %i", rendererStats.c_vertexes), colorWhiteFaded, 6);
        y += (float)CG_DrawSmallDevStringColor(620.0f, y, va("tris %i", rendererStats.c_indexes / 3), colorWhiteFaded, 6);
        y += (float)CG_DrawSmallDevStringColor(620.0f, y, va("batches %i", rendererStats.c_batches), colorWhiteFaded, 6);
    }

    return y;
}

float CG_DrawWeapReticle(void)
{
    playerState_t *ps = &cg->predictedPlayerState;
    int weapIndex = BG_GetViewmodelWeaponIndex(ps);
    WeaponDef *weapDef = (WeaponDef *)BG_GetWeaponDef(weapIndex);
    float zoom = ps->fWeaponPosFrac;
    float zoomFrac;
    float adsFrac;
    vec4_t color;
    vec2_t crosshairPos;
    weaponInfo_t *weaponInfo;
    MaterialHandle material;
    vec2_t drawPos;
    vec2_t drawSize;

    if (!weapDef->szOverlayMaterial[0] && !weapDef->overlayReticle)
        return 1.0f;

    if (zoom == 0.0f)
        return 1.0f;

    adsFrac = cg->playerEntity.bPositionToADS ? weapDef->fAdsZoomInFrac : weapDef->fAdsZoomOutFrac;
    zoomFrac = zoom - (1.0f - adsFrac);
    if (zoomFrac > 0.0f)
        zoomFrac /= adsFrac;

    if (zoomFrac <= 0.01f)
        return 1.0f;

    if (zoomFrac > 1.0f)
        zoomFrac = 1.0f;

    weapIndex = BG_GetViewmodelWeaponIndex(ps);
    weapDef = (WeaponDef *)BG_GetWeaponDef(weapIndex);
    color[0] = 1.0f;
    color[1] = 1.0f;
    color[2] = 1.0f;
    color[3] = zoomFrac;

    CG_CalcCrosshairPosition(&crosshairPos[0], &crosshairPos[1]);

    weaponInfo = &(*(weaponInfo_t **)imp_cg_weapons)[weapIndex];
    material = weaponInfo->hADSOverlay;
    if (material) {
        drawSize[0] = weapDef->overlayWidth;
        drawSize[1] = weapDef->overlayHeight;

        if (drawSize[0] > 320.0f || drawSize[1] > 240.0f) {
            drawPos[0] = crosshairPos[0] - drawSize[0] * 0.5f;
            drawPos[1] = crosshairPos[1] - drawSize[1] * 0.5f;
            CL_DrawStretchPic(drawPos[0], drawPos[1], drawSize[0], drawSize[1], 2, 2,
                              0.0f, 0.0f, 1.0f, 1.0f, color, material);

            CalcScreenPlacement(&drawPos[0], &drawPos[1], &drawSize[0], &drawSize[1], 2, 2);
            CG_DrawFrameOverlay(drawPos[0], drawPos[0] + drawSize[0],
                                drawPos[1], drawPos[1] + drawSize[1], color, material);
        } else {
            drawPos[0] = crosshairPos[0] - drawSize[0];
            drawPos[1] = crosshairPos[1] - drawSize[1];
            CL_DrawStretchPic(drawPos[0], drawPos[1], drawSize[0], drawSize[1], 2, 2,
                              0.0f, 0.0f, 1.0f, 1.0f, color, material);

            drawPos[0] = crosshairPos[0];
            drawPos[1] = crosshairPos[1] - drawSize[1];
            CL_DrawStretchPic(drawPos[0], drawPos[1], drawSize[0], drawSize[1], 2, 2,
                              1.0f, 0.0f, 0.0f, 1.0f, color, material);

            drawPos[0] = crosshairPos[0] - drawSize[0];
            drawPos[1] = crosshairPos[1];
            CL_DrawStretchPic(drawPos[0], drawPos[1], drawSize[0], drawSize[1], 2, 2,
                              0.0f, 1.0f, 1.0f, 0.0f, color, material);

            drawPos[0] = crosshairPos[0];
            drawPos[1] = crosshairPos[1];
            CL_DrawStretchPic(drawPos[0], drawPos[1], drawSize[0], drawSize[1], 2, 2,
                              1.0f, 1.0f, 0.0f, 0.0f, color, material);

            drawPos[0] = crosshairPos[0] - drawSize[0];
            drawPos[1] = crosshairPos[1] - drawSize[1];
            CalcScreenPlacement(&drawPos[0], &drawPos[1], &drawSize[0], &drawSize[1], 2, 2);
            CG_DrawFrameOverlay(drawPos[0], drawPos[0] + drawSize[0] * 2.0f,
                                drawPos[1], drawPos[1] + drawSize[1] * 2.0f, color, material);
        }
    }

    return 1.0f - zoomFrac;
}

unsigned int CG_DrawCrosshair(void)
{
    playerState_t *ps = &cg->predictedPlayerState;
    qboolean drawHudMenus;
    int weapIndex;
    WeaponDef *weapDef;
    weaponInfo_t *weapons;
    MaterialHandle material;
    vec4_t color;
    vec4_t reticleColor;
    float posLerp;
    float reticleVisibility;
    float transScale;
    float centerX;
    float centerY;
    int pmFlags;
    int weaponstate;

    if (cg->renderingThirdPerson)
        return 0;

    drawHudMenus = !CG_AreHudMenusHidden();
    posLerp = ps->fWeaponPosFrac;

    if (ps->eFlags & 0x300) {
        if (drawHudMenus && ps->viewlocked_entNum != 0x3ff)
            CG_DrawTurretCrossHair();
        return 0;
    }

    weapIndex = BG_GetViewmodelWeaponIndex(ps);
    if (weapIndex == 0)
        return 0;

    weapDef = (WeaponDef *)BG_GetWeaponDef(weapIndex);
    reticleVisibility = CG_DrawWeapReticle();

    if (!drawHudMenus)
        return 0;

    pmFlags = ps->pm_flags;
    if (pmFlags & 0x100000) {
        color[0] = 0.25f;
        color[1] = 1.0f;
        color[2] = 0.25f;
    } else if (pmFlags & 0x200000) {
        if (!cg_crosshairEnemyColor->current.enabled) {
            color[0] = 1.0f;
            color[1] = 1.0f;
            color[2] = 1.0f;
        } else {
            color[0] = 1.0f;
            color[1] = 0.25f;
            color[2] = 0.25f;
        }
    } else {
        color[0] = 1.0f;
        color[1] = 1.0f;
        color[2] = 1.0f;
    }

    /* The overlay returns the remaining hip-fire reticle visibility. ADS
     * position is zero at the hip, so using it as alpha hides the crosshair. */
    color[3] = reticleVisibility * cg_crosshairAlpha->current.value;
    if (color[3] < 0.01f)
        return 0;

    if (posLerp == 1.0f && cg_drawGun->current.enabled)
        return 0;
    if (cg_paused->current.integer)
        return 0;
    if (!cg_drawCrosshair->current.enabled)
        return 0;

    weaponstate = ps->weaponstate;
    if ((unsigned int)(weaponstate - 10) <= 1 || weaponstate == 5 ||
        (unsigned int)(weaponstate - 1) <= 1)
        return 0;

    CG_CalcCrosshairPosition(&centerX, &centerY);

    if (posLerp == 0.0f) {
        transScale = 1.0f;
    } else {
        float adsFrac = cg->playerEntity.bPositionToADS ? weapDef->fAdsCrosshairInFrac : weapDef->fAdsCrosshairOutFrac;
        float zoomFrac = posLerp - (1.0f - adsFrac);
        float scaled;
        if (zoomFrac <= 0.0f) {
            transScale = 1.0f;
        } else {
            scaled = zoomFrac / adsFrac;
            if (scaled <= 0.0f)
                transScale = 1.0f;
            else
                transScale = scaled * -0.5f + 1.0f;
        }

        if (!cg_drawGun->current.enabled && transScale < 1.0f) {
            weapons = &(*(weaponInfo_t **)imp_cg_weapons)[weapIndex];
            material = weapons->hReticleCenter;
            if (material) {
                float size = (float)weapDef->iReticleCenterSize * (1.5f - transScale);
                float half = size * -0.5f;
                CL_DrawStretchPic(centerX + half, centerY + half, size, size, 2, 2,
                                  0.0f, 0.0f, 1.0f, 1.0f, color, material);
            }
        }
    }

    if (posLerp == 1.0f && !cg_drawGun->current.enabled)
        return 0;

    if (!cg_crosshairDynamic->current.enabled) {
        centerX = 0.0f;
        centerY = 0.0f;
    }

    weapons = &(*(weaponInfo_t **)imp_cg_weapons)[weapIndex];
    material = weapons->hReticleCenter;
    if (material) {
        float centerSize = (float)weapDef->iReticleCenterSize;
        float drawSize;
        float half;
        if (weapDef->weapType == WEAPTYPE_GRENADE) {
            int grenadeTime = ps->grenadeTimeLeft;
            if (grenadeTime != 0 && weapDef->bCookOffHold)
                centerSize += (float)(grenadeTime % 1000) / 100.0f;
        }
        drawSize = centerSize * transScale;
        if (CL_IsRenderingSplitScreen())
            drawSize += drawSize;
        half = drawSize * -0.5f;
        CL_DrawStretchPic(half + centerX, centerY + half, drawSize, drawSize, 2, 2,
                          0.0f, 0.0f, 1.0f, 1.0f, color, material);
    }

    weapons = &(*(weaponInfo_t **)imp_cg_weapons)[weapIndex];
    material = weapons->hReticleSide;
    if (!material)
        return 0;

    reticleColor[0] = color[0];
    reticleColor[1] = color[1];
    reticleColor[2] = color[2];
    {
        float alpha = transScale * cg_crosshairAlpha->current.value *
                      (ps->aimSpreadScale / -255.0f + 1.0f);
        float alphaMin = (cg_crosshairAlphaMin)->current.value;
        reticleColor[3] = (alpha >= alphaMin ? alpha : alphaMin) * reticleVisibility;
    }

    {
        float sideSizeF = (float)weapDef->iReticleSideSize * transScale;
        float sideSize;
        float realW, realH;
        float spreadMin, spreadMax, spread;
        double spreadTan, fovTan;
        float minOfs;
        float divW, divH;
        float vw, vh;
        float ax, ay;

        if (CL_IsRenderingSplitScreen())
            sideSize = sideSizeF + sideSizeF;
        else
            sideSize = sideSizeF;

        BG_GetSpreadForWeapon(ps, weapIndex, &spreadMin, &spreadMax);
        spread = ((spreadMax - spreadMin) * (ps->aimSpreadScale / 255.0f) + spreadMin) * transScale;
        spreadTan = tan((double)spread * 0.017453292519943295);
        fovTan = tan((double)cg->refdef.fov_y * 0.017453292519943295 * 0.5);
        minOfs = fmaxf((float)weapDef->iReticleMinOfs, (float)(spreadTan * 240.0 / fovTan));
        minOfs -= sideSizeF * weapDef->fHipReticleSidePos;

        realW = GetRealWidthFromVirtualWidth(sideSize);
        realH = GetRealHeightFromVirtualHeight(sideSize);
        divW = (float)cgs->viewWidth;
        divH = (float)cgs->viewHeight;
        vw = GetVirtualWidthFromRealWidth(divW * (realW / divW * 0.125f));
        vh = GetVirtualHeightFromRealHeight(divH * (realH / divH * 0.125f));

        ax = CG_AlignHudElemX(4, centerX, sideSize);
        ay = CG_AlignHudElemY(2, centerY, sideSize);
        CG_DrawRotatedPic(ax, ay - minOfs - vh, sideSize, sideSize, 2, 2, 0.0f, reticleColor, material);

        ax = CG_AlignHudElemX(0, centerX, sideSize);
        ay = CG_AlignHudElemY(1, centerY, sideSize);
        CG_DrawRotatedPic(minOfs + ax, ay, sideSize, sideSize, 2, 2, 90.0f, reticleColor, material);

        ax = CG_AlignHudElemX(4, centerX, sideSize);
        ay = CG_AlignHudElemY(0, centerY, sideSize);
        CG_DrawRotatedPic(ax - vw, ay + minOfs, sideSize, sideSize, 2, 2, 180.0f, reticleColor, material);

        ax = CG_AlignHudElemX(8, centerX, sideSize);
        ay = CG_AlignHudElemY(1, centerY, sideSize);
        CG_DrawRotatedPic(ax - minOfs - vw, ay - vh, sideSize, sideSize, 2, 2, 270.0f, reticleColor, material);
    }

    return 0;
}

static void CG_Draw2D_DrawBottomOverlays(cg_t *cg)
{
    CG_DrawBoldGameMessages();

    if (cg_minicon->current.enabled)
        Con_DrawMiniConsole(2, 4, 1.0f);
    if (cg_subtitles->current.enabled)
        Con_DrawSubtitles(cg_subtitlePosX->current.integer,
                          cg_subtitlePosY->current.integer,
                          cg_subtitleCharHeight->current.integer, 1.0f, 2);

    Con_DrawSay((int)cg_hudSayPosition->current.vector[1] + 24);
    (void)cg;
}

unsigned int CG_Draw2D(void)
{
    extern int CG_GetRespawnRemaining(void);
    qboolean drawHudMenus;
    qboolean drawHudElems;
    qboolean drewScoreboard;
    float debugY = 0.0f;
    int pm_type;
    byte *cls = (byte *)imp_cls;
    float screenWidth = (float)((clientStatic_t *)cls)->vidConfig.width;
    float screenHeight = (float)((clientStatic_t *)cls)->vidConfig.height;

    if (cg->cubemapShot)
        return 0;

    if (!cg_draw2D->current.enabled)
        return 0;

    {
        int remaining = CG_GetRespawnRemaining();
        if (remaining >= 0) {
            float progress = 1.0f - (float)remaining / 3000.0f;
            if (progress < 0.0f)
                progress = 0.0f;
            vec4_t shade = { 0.0f, 0.0f, 0.0f, progress };
            const char *message = remaining > 0
                ? va("Reapareces en %d...", (remaining + 999) / 1000)
                : "Reapareciendo...";
            FontHandle font = UI_GetFontHandle(0, 0.4f);
            UI_FillRectPhysical(0.0f, 0.0f, screenWidth, screenHeight, shade);
            UI_DrawText(message, 0x7fffffff, font,
                        320.0f - UI_TextWidth(message, 0, font, 0.4f) * 0.5f,
                        250.0f, 0, 0, 0.4f, colorWhite, 3);
            CG_DrawChatMessages();
            CG_DrawScoreboard();
            return 0;
        }
    }

    if (cg->fadeRate != 0.0f) {
        int remaining = cg->fadeTime - cg->time;
        if (remaining <= 0) {
            cg->fadeColor1[0] = cg->fadeColor2[0];
            cg->fadeColor1[1] = cg->fadeColor2[1];
            cg->fadeColor1[2] = cg->fadeColor2[2];
            cg->fadeColor1[3] = cg->fadeColor2[3];
            if (cg->fadeColor1[3] == 0.0f)
                cg->fadeRate = 0.0f;
            else
                UI_FillRectPhysical(0.0f, 0.0f, screenWidth, screenHeight, cg->fadeColor1);
        } else {
            float t = (float)remaining * cg->fadeRate;
            float u = 1.0f - t;
            vec4_t fadeColor;
            int i;
            for (i = 0; i < 4; ++i)
                fadeColor[i] = t * cg->fadeColor1[i] + u * cg->fadeColor2[i];
            if (fadeColor[3] != 0.0f)
                UI_FillRectPhysical(0.0f, 0.0f, screenWidth, screenHeight, fadeColor);
        }
    }

    drawHudMenus = !CG_AreHudMenusHidden();
    drawHudElems = !CG_AreHudElemsHidden();

    pm_type = cg->nextSnap->ps.pm_type;
    if (pm_type == 5) {
        CL_CloseAllMenus();
        cg->showScores = 1;
        cg->scoreFadeTime = cg->time;
        CG_DrawScoreboard();
        return 0;
    }

    if (pm_type == 4) {
        if (drawHudMenus) {
            const char *str = UI_SafeTranslateString("CGAME_SPECTATOR");
            FontHandle font = UI_GetFontHandle(4, 0.5f);
            UI_DrawText(str, 0x7fffffff, font,
                        (float)-UI_TextWidth(str, 0, font, 0.5f) * 0.5f, -27.0f, 7, 3, 0.5f, colorWhite, 3);
            CG_DrawCrosshairNames();
            CG_DrawChatMessages();
        }
        if (drawHudElems) {
            CG_Draw2dHudElems(0);
            CG_Draw2dHudElems(1);
        }
    } else {
        if (pm_type <= 5)
            CG_DrawCrosshair();

        if (drawHudMenus) {
            if (cg->nextSnap->ps.pm_type <= 5)
                CG_DrawCrosshairNames();
            CG_DrawChatMessages();
        }
        CG_CheckTimedMenus();
        if (drawHudElems)
            CG_Draw2dHudElems(0);
        if (drawHudMenus)
            Menu_PaintAll((void *)imp_cgDC);
        CG_Draw2dHudElems(1);
    }

    drewScoreboard = CG_DrawScoreboard();
    if (!drawHudMenus)
        return 0;

    if (!drewScoreboard) {
        CG_DrawSpectatorMessage();
        CG_DrawFollow();
    }

    if (cgs->voteTime != 0) {
        char yesKey[256];
        char noKey[256];
        char keys[256];
        FontHandle font = UI_GetFontHandle(0, 0.5f);
        int secs;

        if (!GetKeyBindingLocalizedString("vote yes", keys))
            I_strncpyz(yesKey, "vote yes", 0x100);
        else
            I_strncpyz(yesKey, keys, 0x100);

        if (!GetKeyBindingLocalizedString("vote no", keys))
            I_strncpyz(noKey, "vote no", 0x100);
        else
            I_strncpyz(noKey, keys, 0x100);

        secs = (cgs->voteTime - cg->time) / 1000;
        if (secs < 0)
            secs = 0;

        UI_DrawText(va("%s(%i):%s", UI_SafeTranslateString("CGAME_VOTE"), secs, cgs->voteString),
                    0x7fffffff, font, 8.0f, 200.0f, 0, 0, 0.5f, colorYellow, 3);

        if (cg->nextSnap->ps.eFlags & 0x100000) {
            UI_DrawText(va("%s:%i, %s:%i", UI_SafeTranslateString("CGAME_YES"), cgs->voteYes,
                           UI_SafeTranslateString("CGAME_NO"), cgs->voteNo),
                        0x7fffffff, font, 8.0f, 222.0f, 0, 0, 0.5f, colorYellow, 3);
        } else {
            UI_DrawText(va("%s(%s):%i, %s(%s):%i", UI_SafeTranslateString("CGAME_YES"), yesKey,
                           cgs->voteYes, UI_SafeTranslateString("CGAME_NO"), noKey, cgs->voteNo),
                        0x7fffffff, font, 8.0f, 222.0f, 0, 0, 0.5f, colorYellow, 3);
        }
    }

    if (cg_drawLagometer->current.enabled && !cgs->localServer) {
        float prev;
        int i;

        UI_DrawHandlePic(-55.0f, -140.0f, 48.0f, 48.0f, 3, 3, 0, cgs->media.lagometerMaterial);

        prev = 0.0f;
        for (i = 0; i < 48; ++i) {
            int idx = (~i + lagometer.frameCount) & 0x7f;
            float v = (float)lagometer.frameSamples[idx] * 0.0533333346f;
            float y;
            float h;
            const vec_t *col;
            if (v > prev) {
                if (v <= 16.0f) {
                    y = -124.0f - v;
                    h = v;
                } else {
                    y = -140.0f;
                    h = 16.0f;
                }
                col = colorYellow;
                CL_DrawStretchPic(-7.0f, y, 1.0f, h, 3, 3, 0, 0, 0, 0, col, cgs->media.whiteMaterial);
                prev = 0.0f;
            } else if (v < 0.0f) {
                float n = -v;
                if (n > 16.0f)
                    n = 16.0f;
                CL_DrawStretchPic(-7.0f, -124.0f, 1.0f, n, 3, 3, 0, 0, 0, 0, colorBlue,
                                  cgs->media.whiteMaterial);
                prev = 0.0f;
            } else {
                prev = 0.0f;
            }
        }

        prev = 0.0f;
        for (i = 0; i < 48; ++i) {
            int idx = (~i + lagometer.snapshotCount) & 0x7f;
            float v = (float)lagometer.snapshotSamples[idx];
            if (v > prev) {
                float h = v * 0.0266666673f;
                float y;
                if (lagometer.snapshotFlags[idx] & 0x1)
                    y = h <= 24.0f ? -92.0f - h : -116.0f;
                else
                    y = h > 24.0f ? -116.0f : -92.0f - h;
                CL_DrawStretchPic(-7.0f, y, 1.0f, h, 3, 3, 0, 0, 0, 0, colorGreen,
                                  cgs->media.whiteMaterial);
            } else if (v < 0.0f) {
                CL_DrawStretchPic(-7.0f, -116.0f, 1.0f, 24.0f, 3, 3, 0, 0, 0, 0, colorRed,
                                  cgs->media.whiteMaterial);
            }
            prev = 0.0f;
        }

        if (cg_nopredict->current.enabled ||
            cg_synchronousClients->current.enabled)
            CG_DrawBigDevString(-55.0f, -140.0f, "snc", 1.0f, 9);
    }

    CG_DrawDisconnect();

    {
        int now = Sys_Milliseconds();
        const void *statsArray;
        int statsCount;
        StatMon_GetStatsArray(&statsArray, &statsCount);
        if (statsCount > 0) {
            float y = 200.0f;
            int i;
            for (i = 0; i < statsCount; ++i) {
                const int *entry = (const int *)((const char *)statsArray + i * 8);
                if (now <= entry[0])
                    UI_DrawHandlePic(2.0f, y, 0.0f, 0.0f, 1, 0, 0, (MaterialHandle)entry[1]);
                y += 34.0f;
            }
        }
    }

    if (cg_drawSoundOverlay->current.integer)
        CG_DrawSoundOverlay();
    else if (cg_drawScriptUsage->current.enabled)
        CG_DrawScriptUsage();
    else if (cg_drawMaterial->current.enabled)
        CG_DrawMaterial();

    CL_TrackStatistics(0);

    if (cg_drawSnapshot->current.enabled)
        CG_DrawBigDevStringColor(620.0f, 0.0f,
                                 va("time:%i snap:%i cmd:%i", cg->nextSnap->serverTime, cg->latestSnapshotNum,
                                    cgs->serverCommandSequence),
                                 colorWhite, 6);

    if (cg_drawFPS->current.integer)
        debugY = CG_DrawFPS(debugY);
    (void)debugY;

    if (drewScoreboard)
        return 0;

    if (cg->centerPrintTime != 0) {
        int totalMsec = (int)(1000.0f * cg_centertime->current.value);
        CG_FadeColor(cg->centerPrintTime, totalMsec, 100);
        if (totalMsec == 0) {
            cg->centerPrintTime = 0;
            cg->centerPrintPriority = 0;
        } else {
            float scale = (float)cg->centerPrintCharWidth * 0.03125f;
            FontHandle font;
            int lineHeight;
            float y;
            float lineStep;
            const char *start;

            if (CL_IsRenderingSplitScreen())
                scale *= 1.25f;

            font = UI_GetFontHandle(0, scale);
            lineHeight = UI_TextHeight(font, scale);
            y = (float)lineHeight * -1.20000005f * (float)cg->centerPrintLines + cg_centerPrintY->current.value;
            lineStep = (float)lineHeight * 1.20000005f;

            start = cg->centerPrint;
            while (1) {
                char line[256];
                int n = 0;
                const char *p = start;
                FontHandle lineFont;
                int width;

                while (*p && *p != '\n' && n < 75)
                    line[n++] = *p++;
                line[n] = '\0';

                lineFont = UI_GetFontHandle(0, scale);
                width = UI_TextWidth(line, 0, lineFont, scale);
                UI_DrawText(line, 0x7fffffff, lineFont, (float)width * -0.5f, y, 7, 3, scale,
                            (const vec_t *)CG_FadeColor(cg->centerPrintTime, totalMsec, 100), 3);
                y += lineStep;

                while (*start && *start != '\n')
                    start++;
                if (*start == '\0')
                    break;
                start++;
            }
        }
    }

    if (!cg_drawGameMessages->current.enabled) {
        CG_Draw2D_DrawBottomOverlays(cg);
        return 0;
    } else {
        float notifyY = floorf(cgs->compassY -
                               (cg_hudCompassSize->current.value - 1.0f) * cgs->compassHeight + 12.0f + 0.5f);
        float alpha;
        if (!CG_ScoreboardDisplayed()) {
            alpha = 1.0f;
        } else {
            float *fade = CG_FadeColor(cg->scoreFadeTime, 100, 100);
            if (cg->scoreFadeTime == 0 || fade == 0) {
                CG_Draw2D_DrawBottomOverlays(cg);
                return 0;
            }
            alpha = fade[3];
        }
        Con_DrawNotify(6, (int)notifyY, alpha, 2);
        CG_Draw2D_DrawBottomOverlays(cg);
    }

    return 0;
}

#else
static float CG_DrawFPS(float y)
{
    return 0.0f;
}
#endif
