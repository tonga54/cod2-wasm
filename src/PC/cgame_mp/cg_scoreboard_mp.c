#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include "headers/PC/cgame_mp/cg_local.h"

extern const dvar_t *cg_scoreboardBannerHeight;

extern const dvar_t *cg_paused;

extern struct Material *CL_RegisterMaterialNoMip(const char *name, int imageTrack);
extern const char *Dvar_GetString(const char *dvarName);
extern void Dvar_GetUnpackedColorByName(const char *dvarName, vec_t *color);
extern void UI_DrawHandlePic(float x, float y, float w, float h, int horzAlign, int vertAlign, const vec_t *color, MaterialHandle material);
extern void UI_DrawText(const char *text, int maxChars, void *font, float x, float y, int horzAlign, int vertAlign, float scale, float *color, int style);
extern int UI_TextWidth(const char *text, int maxChars, void *font, float scale);
extern int UI_TextHeight(void *font, float scale);
extern void *UI_GetFontHandle(int fontEnum, float scale);
extern const char *UI_GetGameTypeDisplayName(const char *gameType);
extern const char *UI_GetMapDisplayName(const char *mapName);
extern const char *SEH_LocalizeTextMessage(const char *msg, const char *context, int errType);
extern const char *CL_GetServerIPAddress(void);
extern Bool CL_IsPlayerMuted(int clientIndex);
extern Bool CL_IsPlayerTalking(int clientIndex);
extern void CG_TranslateHudElemMessage(const char *message, const char *messageType, char *hudElemString);
extern int I_stricmp(const char *s1, const char *s2);
extern int I_strnicmp(const char *s1, const char *s2, size_t n);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern const char *FS_GetMapBaseName(const char *mapfile);
extern const char *UI_SafeTranslateString(const char *key);
extern void CL_AddReliableCommand(const char *cmd);
extern float *CG_FadeColor(int startMsec, int totalMsec, int fadeMsec);
extern const char *va(const char *format, ...);

extern int lastLeadTeam;

static const listColumnInfo_t columnInfo[6] = {
    { LCT_NAME, 0.46f, "", 0 },
    { LCT_STATUS_ICON, 0.05f, "", 0 },
    { LCT_TALKING_ICON, 0.05f, "", 0 },
    { LCT_SCORE, 0.15f, "CGAME_SB_SCORE", 2 },
    { LCT_DEATHS, 0.15f, "CGAME_SB_DEATHS", 2 },
    { LCT_PING, 0.14f, "CGAME_SB_PING", 2 },
};

qboolean CG_ScoreboardDisplayed(void);
void CG_ScrollScoreboardUp(void);
void CG_ScrollScoreboardDown(void);
void CG_RegisterScoreboardGraphics(void);
int CalcBorderLines(sbpicinfo_t *backdropBorderLines);
void CG_DrawScoreboard_GetTeamColor(int team, vec_t *color);
static float CG_DrawClientScore(vec_t *color, float y, score_t *score, float listWidth, qboolean selection, int *piDrawLine, const int numDrawn);
static float CG_DrawScoreboard_ListBanner(vec_t *color, float y, float listWidth, float h, int team, int *drawLine);
float CG_DrawScoreboard_ScoresList(float alpha);
qboolean CG_DrawScoreboard(void);

qboolean CG_ScoreboardDisplayed(void)
{
    return cg->showScores;
}

void CG_ScrollScoreboardUp(void)
{
    if (cg->scoresTop > 0) {
        cg->scoresTop -= *(int *)(*(int *)imp_cg_scoreboardScrollStep + 8);
        if (cg->scoresTop < 0)
            cg->scoresTop = 0;
    }
}

void CG_ScrollScoreboardDown(void)
{
    if (cg->scoresOffBottom != 0) {
        cg->scoresTop += *(int *)(*(int *)imp_cg_scoreboardScrollStep + 8);
        int maxScroll = cg->numScores - 1;
        if (cg->scoresTop > maxScroll)
            cg->scoresTop = maxScroll;
    }
}

void CG_RegisterScoreboardGraphics(void)
{
    CL_RegisterMaterialNoMip((const char *)"white", 7);
    CL_RegisterMaterialNoMip((const char *)"white", 7);
    CL_RegisterMaterialNoMip((const char *)"black", 7);
    CL_RegisterMaterialNoMip((const char *)"white", 7);
    CL_RegisterMaterialNoMip((const char *)"white", 7);
    CL_RegisterMaterialNoMip((const char *)"black", 7);
    CL_RegisterMaterialNoMip((const char *)"hudscoreboardscroll_uparrow", 7);
    CL_RegisterMaterialNoMip((const char *)"hudscoreboardscroll_upkey", 7);
    CL_RegisterMaterialNoMip((const char *)"hudscoreboardscroll_downarrow", 7);
    CL_RegisterMaterialNoMip((const char *)"hudscoreboardscroll_downkey", 7);
    CL_RegisterMaterialNoMip(Dvar_GetString((const char *)"g_ScoresBanner_Spectators"), 7);
    CL_RegisterMaterialNoMip(Dvar_GetString((const char *)"g_ScoresBanner_Axis"), 7);
    CL_RegisterMaterialNoMip(Dvar_GetString((const char *)"g_ScoresBanner_Allies"), 7);
    CL_RegisterMaterialNoMip(Dvar_GetString((const char *)"g_ScoresBanner_None"), 7);
    CL_RegisterMaterialNoMip((const char *)"voice_on", 7);
    CL_RegisterMaterialNoMip((const char *)"voice_off", 7);
}

int CalcBorderLines(sbpicinfo_t *backdropBorderLines)
{

    backdropBorderLines[0].x = 123.0f;
    backdropBorderLines[0].y = 25.0f;
    backdropBorderLines[0].w = 394.0f;
    backdropBorderLines[0].h = 2.0f;

    backdropBorderLines[1].x = 123.0f;
    backdropBorderLines[1].y = 447.0f;
    backdropBorderLines[1].w = 394.0f;
    backdropBorderLines[1].h = 2.0f;

    backdropBorderLines[2].x = 123.0f;
    backdropBorderLines[2].y = 27.0f;
    backdropBorderLines[2].w = 2.0f;
    backdropBorderLines[2].h = 420.0f;

    backdropBorderLines[3].x = 515.0f;
    backdropBorderLines[3].y = 27.0f;
    backdropBorderLines[3].w = 2.0f;
    backdropBorderLines[3].h = 420.0f;

    backdropBorderLines[4].x = 125.0f;
    backdropBorderLines[4].y = 51.0f;
    backdropBorderLines[4].w = 390.0f;
    backdropBorderLines[4].h = 1.0f;

    backdropBorderLines[5].x = 125.0f;
    backdropBorderLines[5].y = 432.0f;
    backdropBorderLines[5].w = 390.0f;
    backdropBorderLines[5].h = 1.0f;

    return 6;
}

void CG_DrawScoreboard_GetTeamColor(int team, vec_t *color)
{
    float savedAlpha = color[3];

    if (team == 1) {
        Dvar_GetUnpackedColorByName((const char *)"g_TeamColor_Axis", color);
    } else if (team == 2) {
        Dvar_GetUnpackedColorByName((const char *)"g_TeamColor_Allies", color);
    } else {
        color[0] = 1.0f;
        color[1] = 1.0f;
        color[2] = 1.0f;
    }

    color[3] = savedAlpha;
}

#define CG_SCOREBOARD_CLIENTINFO_OFFSET 0xe0914

static int CG_ScoreboardBannerHeight(void)
{
    const dvar_t *dvar = cg_scoreboardBannerHeight;
    return dvar ? dvar->current.integer : 24;
}

static clientInfo_t *CG_ScoreboardClientInfo(cg_t *cg, int clientNum)
{
    if (!cg || clientNum < 0 || clientNum >= 64)
        return 0;

    return &((clientInfo_t *)((byte *)cg + CG_SCOREBOARD_CLIENTINFO_OFFSET))[clientNum];
}

static int CG_Scoreboard_BeginVisibleLine(cg_t *cg, int *drawLine, float y, float lineHeight, float *nextY)
{
    if (cg->scoresOffBottom)
        return 0;

    if (*drawLine < cg->scoresTop) {
        ++*drawLine;
        return 0;
    }

    *nextY = y + lineHeight;
    if (*nextY > 432.0f) {
        cg->scoresOffBottom = 1;
        return 0;
    }

    ++*drawLine;
    return 1;
}

static void CG_Scoreboard_SetColor(vec_t *dst, float r, float g, float b, float a)
{
    dst[0] = r;
    dst[1] = g;
    dst[2] = b;
    dst[3] = a;
}

static void CG_Scoreboard_CopyColor(vec_t *dst, const vec_t *src, float alpha)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = alpha;
}

static void CG_Scoreboard_DrawColumnString(const char *text, void *font, float x, float y, float columnWidth, const listColumnInfo_t *column, float alpha)
{
    float scale;
    float textX;
    int style;
    vec_t textColor[4];

    if (!text || !text[0])
        return;

    scale = 0.22f;
    while ((float)UI_TextWidth(text, 0x7fffffff, font, scale) > columnWidth && scale > 0.02f)
        scale -= 0.02f;

    style = (scale >= 0.16f) ? 3 : 0;
    textX = x;
    if (column->iAlignment == 2)
        textX += columnWidth - (float)UI_TextWidth(text, 0x7fffffff, font, scale);

    CG_Scoreboard_SetColor(textColor, 1.0f, 1.0f, 1.0f, alpha);
    UI_DrawText(text, 0x7fffffff, font, textX, y + 10.08f, 0, 0, scale, textColor, style);
}

static float CG_DrawClientScore(vec_t *color, float y, score_t *score, float listWidth, qboolean selection, int *piDrawLine, const int numDrawn)
{
    clientInfo_t *ci;
    float nextY;
    float x;
    void *listFont;
    int columnIndex;

    if (!CG_Scoreboard_BeginVisibleLine(cg, piDrawLine, y, 12.0f, &nextY))
        return y;

    ci = CG_ScoreboardClientInfo(cg, score->client);
    if (!ci || !ci->infoValid)
        return y;

    if (cg->firstLineVisible < 0)
        cg->firstLineVisible = numDrawn;
    cg->lastLineVisible = numDrawn;

    if (score->client == cg->clientNum || selection) {
        MaterialHandle material;
        vec_t backColor[4];
        float alphaScale;

        material = CL_RegisterMaterialNoMip(score->client == cg->clientNum ? (const char *)"white" : (const char *)"black", 7);
        alphaScale = (score->client == cg->clientNum) ? 0.2f : 0.15f;
        CG_Scoreboard_CopyColor(backColor, color, color[3] * alphaScale);
        UI_DrawHandlePic(129.0f, y, listWidth, 12.0f, 0, 0, backColor, material);
    }

    listFont = UI_GetFontHandle(0, 0.22f);
    x = 129.0f;

    for (columnIndex = 0; columnIndex < 6; ++columnIndex) {
        const listColumnInfo_t *column = &columnInfo[columnIndex];
        float columnWidth = listWidth * column->fWidth;
        const char *text = "";

        switch (column->type) {
        case LCT_NAME:
            text = ci->name;
            break;

        case LCT_STATUS_ICON:
            if (score->hStatusIcon) {
                vec_t iconColor[4];
                float iconX = x;
                if (column->iAlignment == 2)
                    iconX += columnWidth - 13.0f;
                CG_Scoreboard_SetColor(iconColor, 1.0f, 1.0f, 1.0f, color[3]);
                UI_DrawHandlePic(iconX, y, 13.0f, 13.0f, 0, 0, iconColor, score->hStatusIcon);
            }
            break;

        case LCT_TALKING_ICON: {
            MaterialHandle material = 0;
            if (CL_IsPlayerMuted(score->client))
                material = CL_RegisterMaterialNoMip((const char *)"voice_off", 7);
            else if (CL_IsPlayerTalking(score->client))
                material = CL_RegisterMaterialNoMip((const char *)"voice_on", 7);

            if (material) {
                vec_t iconColor[4];
                CG_Scoreboard_SetColor(iconColor, 1.0f, 1.0f, 1.0f, color[3]);
                UI_DrawHandlePic(x, y, 13.0f, 13.0f, 0, 0, iconColor, material);
            }
        } break;

        case LCT_SCORE:
            if (score->team != 3)
                text = va((const char *)"%i", score->score);
            break;

        case LCT_DEATHS:
            if (score->team != 3)
                text = va((const char *)"%i", score->deaths);
            break;

        case LCT_PING:
            text = va((const char *)"%i", score->ping);
            break;

        default:
            break;
        }

        CG_Scoreboard_DrawColumnString(text, listFont, x, y, columnWidth, column, color[3]);
        x += columnWidth;
    }

    return nextY;
}

static float CG_DrawScoreboard_ListBanner(vec_t *color, float y, float listWidth, float h, int team, int *drawLine)
{
    float nextY;
    float bannerLineHeight;
    int playerCount;
    char playersString[128];
    char bannerMaterialName[64];
    char displayString[256];
    vec_t bannerColor[4];
    vec_t teamColor[4];
    MaterialHandle material;
    void *bannerFont;
    float x;
    int columnIndex;

    bannerLineHeight = (float)CG_ScoreboardBannerHeight();
    if (!CG_Scoreboard_BeginVisibleLine(cg, drawLine, y, bannerLineHeight, &nextY))
        return y;

    playerCount = cg->teamPlayers[team];
    I_strncpyz(playersString, SEH_LocalizeTextMessage(
        va(playerCount == 1 ? (const char *)"CGAME_SB_PLAYER\x15%i" : (const char *)"CGAME_SB_PLAYERS\x15%i", playerCount == 1 ? 1 : playerCount),
        (const char *)"scoreboard banner text",
        0), sizeof(playersString));

    /* Dvar_GetString and va share temporary buffers. Keep the asset name and
     * labels alive across localization and material registration calls. */

    if (team == 1) {
        const char *teamName;
        I_strncpyz(bannerMaterialName, Dvar_GetString("g_ScoresBanner_Axis"), sizeof(bannerMaterialName));
        teamName = SEH_LocalizeTextMessage(Dvar_GetString((const char *)"g_TeamName_Axis"), (const char *)"scoreboard team name", 0);
        I_strncpyz(displayString, va("%s (%s)", teamName, playersString), sizeof(displayString));
    } else if (team == 2) {
        const char *teamName;
        I_strncpyz(bannerMaterialName, Dvar_GetString("g_ScoresBanner_Allies"), sizeof(bannerMaterialName));
        teamName = SEH_LocalizeTextMessage(Dvar_GetString((const char *)"g_TeamName_Allies"), (const char *)"scoreboard team name", 0);
        I_strncpyz(displayString, va("%s (%s)", teamName, playersString), sizeof(displayString));
    } else if (team == 0) {
        I_strncpyz(bannerMaterialName, Dvar_GetString("g_ScoresBanner_None"), sizeof(bannerMaterialName));
        I_strncpyz(displayString, playersString, sizeof(displayString));
    } else {
        const char *teamName;
        I_strncpyz(bannerMaterialName, Dvar_GetString("g_ScoresBanner_Spectators"), sizeof(bannerMaterialName));
        teamName = SEH_LocalizeTextMessage((const char *)"CGAME_SPECTATORS", (const char *)"scoreboard team name", 0);
        I_strncpyz(displayString, va("%s (%s)", teamName, playersString), sizeof(displayString));
    }

    CG_Scoreboard_SetColor(bannerColor, 1.0f, 1.0f, 1.0f, color[3]);
    material = CL_RegisterMaterialNoMip(bannerMaterialName, 7);
    UI_DrawHandlePic(129.0f, y, listWidth, h, 0, 0, bannerColor, material);

    CG_Scoreboard_CopyColor(teamColor, bannerColor, bannerColor[3]);
    CG_DrawScoreboard_GetTeamColor(team, teamColor);
    bannerFont = UI_GetFontHandle(0, 0.32f);
    UI_DrawText(displayString, 0x7fffffff, bannerFont, 133.0f, y + bannerLineHeight * 0.84f, 0, 0, 0.32f, teamColor, 3);

    if (team != 1 && team != 2)
        return y + h;

    x = 129.0f;
    for (columnIndex = 0; columnIndex < 6; ++columnIndex) {
        const listColumnInfo_t *column = &columnInfo[columnIndex];
        float columnWidth = listWidth * column->fWidth;
        const char *text = 0;

        if (column->type == LCT_SCORE)
            text = va((const char *)"%i", cg->teamScores[team]);
        else if (column->type == LCT_DEATHS)
            text = va((const char *)"%i", cg->teamPings[team]);

        if (text) {
            float textX = x;
            if (column->iAlignment == 2)
                textX += columnWidth - (float)UI_TextWidth(text, 0, bannerFont, 0.32f);
            UI_DrawText(text, 0x7fffffff, bannerFont, textX, y + bannerLineHeight * 0.84f, 0, 0, 0.32f, teamColor, 3);
        }

        x += columnWidth;
    }

    return y + h;
}

static float CG_Scoreboard_DrawObjective(cg_t *cg, vec_t *color)
{
    char hudElemString[256];
    const char *lineStart;
    float y = 56.0f;
    void *font;
    MaterialHandle whiteMaterial;
    vec_t borderColor[4];
    float dividerY;

    font = UI_GetFontHandle(0, 0.24f);
    CG_TranslateHudElemMessage(cg->objectiveText, (const char *)"scoreboard objective info", hudElemString);
    hudElemString[sizeof(hudElemString) - 1] = '\0';

    lineStart = hudElemString;
    while (*lineStart) {
        const char *scan;
        const char *lastSpace = 0;
        const char *lineEnd = 0;

        while (*lineStart == ' ')
            ++lineStart;
        if (!*lineStart)
            break;

        scan = lineStart;
        while (*scan) {
            const char *next;

            if (*scan == '\n') {
                lineEnd = scan;
                break;
            }
            if (*scan == '\\' && scan[1] == 'n') {
                lineEnd = scan;
                break;
            }

            next = scan + 1;
            if ((float)UI_TextWidth(lineStart, (int)(next - lineStart), font, 0.24f) > 374.0f) {
                lineEnd = lastSpace ? lastSpace : scan;
                if (lineEnd == lineStart)
                    lineEnd = next;
                break;
            }

            if (*next == ' ')
                lastSpace = next;
            scan = next;
        }

        if (!lineEnd)
            lineEnd = scan;

        if (lineEnd > lineStart)
            UI_DrawText(lineStart, (int)(lineEnd - lineStart), font, 129.0f, y + 10.08f, 0, 0, 0.24f, color, 3);

        y += 12.0f;

        if (*lineEnd == '\\' && lineEnd[1] == 'n')
            lineStart = lineEnd + 2;
        else if (*lineEnd == '\n')
            lineStart = lineEnd + 1;
        else if (lastSpace && lineEnd == lastSpace)
            lineStart = lineEnd + 1;
        else
            lineStart = lineEnd;
    }

    dividerY = y + 2.0f;
    whiteMaterial = CL_RegisterMaterialNoMip((const char *)"white", 7);
    CG_Scoreboard_CopyColor(borderColor, color, color[3] * 0.1f);
    UI_DrawHandlePic(125.0f, dividerY, 390.0f, 1.0f, 0, 0, borderColor, whiteMaterial);

    return dividerY + 1.0f;
}

static float CG_Scoreboard_DrawHeader(vec_t *color, float y, float listWidth)
{
    void *font;
    float x;
    int columnIndex;

    font = UI_GetFontHandle(0, 0.3f);
    x = 129.0f;

    for (columnIndex = 0; columnIndex < 6; ++columnIndex) {
        const listColumnInfo_t *column = &columnInfo[columnIndex];
        float columnWidth = listWidth * column->fWidth;

        if (column->pszName && column->pszName[0]) {
            const char *translation = UI_SafeTranslateString(column->pszName);
            float textX = x;
            if (column->iAlignment == 2)
                textX += columnWidth - (float)UI_TextWidth(translation, 0, font, 0.3f);
            UI_DrawText(translation, 0x7fffffff, font, textX, y + 11.76f, 0, 0, 0.3f, color, 3);
        }

        x += columnWidth;
    }

    return y + 14.0f;
}

static void CG_Scoreboard_TeamColor(int team, float alpha, vec_t *color)
{
    CG_Scoreboard_SetColor(color, 1.0f, 1.0f, 1.0f, alpha);
    CG_DrawScoreboard_GetTeamColor(team, color);
}

static float CG_Scoreboard_DrawScoresForTeam(int team, vec_t *color, float y, float listWidth, int *drawLine, int *numDrawn)
{
    qboolean highlight = 0;
    int i;

    for (i = 0; i < cg->numScores; ++i) {
        score_t *score = &cg->scores[i];
        clientInfo_t *ci = CG_ScoreboardClientInfo(cg, score->client);

        if (ci && ci->infoValid && score->team == team) {
            y = CG_DrawClientScore(color, y, score, listWidth, highlight, drawLine, *numDrawn);
            highlight ^= 1;
            ++*numDrawn;
        }
    }

    return y;
}

static void CG_Scoreboard_DrawScrollBar(float y, int drawLine, int totalLines, float alpha)
{
    vec_t color[4];
    MaterialHandle material;
    float barHeight;
    float thumbY;
    float thumbHeight;
    int visibleLines;

    barHeight = 432.0f - y - 1.0f;
    if (barHeight <= 0.0f)
        return;

    CG_Scoreboard_SetColor(color, 1.0f, 1.0f, 1.0f, alpha * 0.5f);
    material = CL_RegisterMaterialNoMip((const char *)"black", 7);
    UI_DrawHandlePic(505.0f, y, 8.0f, barHeight, 0, 0, color, material);

    thumbY = y + 1.0f;
    thumbHeight = barHeight - 2.0f;
    if (cg->scoresTop > 0 && totalLines > 0)
        thumbY += ((float)cg->scoresTop / (float)totalLines) * thumbHeight;

    visibleLines = drawLine - cg->scoresTop;
    if (visibleLines > 1 && totalLines > 1)
        thumbHeight *= (float)visibleLines / (float)totalLines;

    CG_Scoreboard_SetColor(color, 1.0f, 1.0f, 1.0f, alpha * 0.25f);
    material = CL_RegisterMaterialNoMip((const char *)"white", 7);
    UI_DrawHandlePic(506.0f, thumbY, 6.0f, thumbHeight, 0, 0, color, material);

    CG_Scoreboard_SetColor(color, 1.0f, 1.0f, 1.0f, alpha);
    if (cg->scoresTop > 0) {
        material = CL_RegisterMaterialNoMip((const char *)"hudscoreboardscroll_uparrow", 7);
        UI_DrawHandlePic(521.0f, y, 16.0f, 16.0f, 0, 0, color, material);
        material = CL_RegisterMaterialNoMip((const char *)"hudscoreboardscroll_upkey", 7);
        UI_DrawHandlePic(521.0f, y + 18.0f, 16.0f, 16.0f, 0, 0, color, material);
    }

    if (drawLine < totalLines) {
        material = CL_RegisterMaterialNoMip((const char *)"hudscoreboardscroll_downarrow", 7);
        UI_DrawHandlePic(521.0f, 415.0f, 16.0f, 16.0f, 0, 0, color, material);
        material = CL_RegisterMaterialNoMip((const char *)"hudscoreboardscroll_downkey", 7);
        UI_DrawHandlePic(521.0f, 397.0f, 16.0f, 16.0f, 0, 0, color, material);
    }
}

float CG_DrawScoreboard_ScoresList(float alpha)
{
    vec_t color[4];
    float y;
    float headerBottomY;
    float contentHeight;
    float listWidth;
    float bannerDrawHeight;
    int bannerHeight;
    int totalLines;
    int drawScrollBar;
    int drawLine;
    int numDrawn;

    cg->scoresOffBottom = 0;
    cg->firstLineVisible = -1;

    CG_Scoreboard_SetColor(color, 1.0f, 1.0f, 1.0f, alpha);

    y = 52.0f;
    if (cg->objectiveText[0])
        y = CG_Scoreboard_DrawObjective(cg, color);

    bannerHeight = CG_ScoreboardBannerHeight();
    if (cg->teamPlayers[1] || cg->teamPlayers[2]) {
        contentHeight = (float)(bannerHeight * 2 + 8) + 10.0f;
        totalLines = 2;
    } else {
        contentHeight = 10.0f;
        totalLines = 0;
    }

    if (cg->teamPlayers[0]) {
        contentHeight += (float)(bannerHeight + 4);
        ++totalLines;
    }

    if (cg->teamPlayers[3]) {
        contentHeight += (float)(bannerHeight + 4);
        ++totalLines;
    }

    if (cg->numScores > 0) {
        contentHeight += 12.0f * (float)cg->numScores;
        totalLines += cg->numScores;
    }

    drawScrollBar = contentHeight > (432.0f - y);
    listWidth = drawScrollBar ? 374.0f : 382.0f;
    bannerDrawHeight = (float)bannerHeight / 0.75f;

    y += 4.0f;
    y = CG_Scoreboard_DrawHeader(color, y, listWidth);
    headerBottomY = y;

    drawLine = 0;
    numDrawn = 0;

    if (cg->teamPlayers[1] || cg->teamPlayers[2]) {
        int team;
        int otherTeam;
        vec_t teamColor[4];
        float teamY;

        if (cg->teamScores[1] > cg->teamScores[2])
            team = 1;
        else if (cg->teamScores[1] < cg->teamScores[2])
            team = 2;
        else
            team = lastLeadTeam;

        lastLeadTeam = team;
        teamY = CG_DrawScoreboard_ListBanner(color, y, listWidth, bannerDrawHeight, team, &drawLine);
        CG_Scoreboard_TeamColor(team, alpha, teamColor);
        teamY = CG_Scoreboard_DrawScoresForTeam(team, teamColor, teamY, listWidth, &drawLine, &numDrawn);

        otherTeam = (team == 1) ? 2 : 1;
        y = CG_DrawScoreboard_ListBanner(color, teamY + 4.0f, listWidth, bannerDrawHeight, otherTeam, &drawLine);
        CG_Scoreboard_TeamColor(otherTeam, alpha, teamColor);
        y = CG_Scoreboard_DrawScoresForTeam(otherTeam, teamColor, y, listWidth, &drawLine, &numDrawn) + 4.0f;
    }

    if (cg->teamPlayers[0]) {
        vec_t teamColor[4];
        y = CG_DrawScoreboard_ListBanner(color, y, listWidth, bannerDrawHeight, 0, &drawLine);
        CG_Scoreboard_TeamColor(0, alpha, teamColor);
        y = CG_Scoreboard_DrawScoresForTeam(0, teamColor, y, listWidth, &drawLine, &numDrawn) + 4.0f;
    }

    if (cg->teamPlayers[3]) {
        vec_t teamColor[4];
        y = CG_DrawScoreboard_ListBanner(color, y, listWidth, bannerDrawHeight, 3, &drawLine);
        CG_Scoreboard_TeamColor(3, alpha, teamColor);
        y = CG_Scoreboard_DrawScoresForTeam(3, teamColor, y, listWidth, &drawLine, &numDrawn);
    }

    if (drawScrollBar)
        CG_Scoreboard_DrawScrollBar(headerBottomY, drawLine, totalLines, alpha);

    if (cg->scoresTop > totalLines - 1)
        cg->scoresTop = totalLines - 1;

    return 0.0f;
}

qboolean CG_DrawScoreboard(void)
{
    float fade;
    float bgAlpha;
    float borderAlpha;
    float color[4];
    float fontScale;
    float footerFontScale;
    struct Material *material;
    sbpicinfo_t borderLines[6];
    char mapBuffer[64];
    const char *map;
    const char *gameType;
    const char *serverName;
    const char *serverIP;
    void *headerFont;
    void *footerFont;
    int numLines;
    int mapLen;
    float y;
    float *fadePtr;

    if (cg_paused->current.integer != 0)
        return 0;

    {
        if (cg->showScores == 0) {

            fadePtr = (float *)CG_FadeColor(cg->scoreFadeTime, 100, 100);
            if (fadePtr == 0) {

                cg->killerName[0] = 0;
                goto scoreboard_done;
            }

            fade = fadePtr[0];
            bgAlpha = fade * 0.8f;
            borderAlpha = fade * 0.1f;
        } else {

            fade = 1.0f;
            bgAlpha = 0.8f;
            borderAlpha = 0.1f;
        }

        {
            int serverTime = cg->time;
            int lastScoreTime = cg->scoresRequestTime;
            if (lastScoreTime + 0x7d0 < serverTime) {
                cg->scoresRequestTime = serverTime;
                CL_AddReliableCommand((const char *)"score");
            }
        }
    }

    {
        const vec_t *cb = (const vec_t *)imp_colorBlack;
        color[0] = cb[0];
        color[1] = cb[1];
        color[2] = cb[2];
        color[3] = bgAlpha;
        {
            struct Material *whiteMat = CL_RegisterMaterialNoMip((const char *)"white", 7);
            {
                float x = 120.0f;
                float yy = 22.0f;
                float w = 400.0f;
                float h = 430.0f;
                UI_DrawHandlePic(x, yy, w, h, 0, 0, color, whiteMat);
            }
        }
    }

    {
        const vec_t *cw = (const vec_t *)imp_colorWhite;
        color[0] = cw[0];
        color[1] = cw[1];
        color[2] = cw[2];
        color[3] = borderAlpha;
        material = CL_RegisterMaterialNoMip((const char *)"white", 7);
        numLines = CalcBorderLines(borderLines);
        if (numLines > 0) {
            int lineIdx;
            for (lineIdx = 0; lineIdx < numLines; lineIdx++) {
                const sbpicinfo_t *line = &borderLines[lineIdx];
                UI_DrawHandlePic(line->x, line->y, line->w, line->h, 0, 0, color, material);
            }
        }
    }

    gameType = (const char *)SEH_LocalizeTextMessage(
        UI_GetGameTypeDisplayName(cgs->gametype),
        (const char *)"scoreboard gametype display", 0);

    {
        const vec_t *cw = (const vec_t *)imp_colorWhite;
        float savedR = cw[0];
        float savedG = cw[1];
        float savedB = cw[2];
        color[0] = savedR;
        color[1] = savedG;
        color[2] = savedB;
        color[3] = fade;
        {
            float headerScale;
            int textHeight;
            *(int *)&headerScale = 0x3ed1eb85;
            headerFont = UI_GetFontHandle(0, headerScale);
            textHeight = UI_TextHeight(headerFont, headerScale);
            {
                float yPos = 51.0f + (float)(24 - textHeight) * -0.5f;
                UI_DrawText(gameType, 0x7fffffff, headerFont, 129.0f, yPos, 0, 0, headerScale, color, 3);
            }
        }

        color[0] = savedR;
        color[1] = savedG;
        color[2] = savedB;
        color[3] = fade;

        {
            const char *baseName = FS_GetMapBaseName(cgs->mapname);
            const char *dispName = UI_GetMapDisplayName(baseName);
            I_strncpyz(mapBuffer, dispName, 0x40);
        }

        if (mapBuffer[0] == '\0') {

            strcpy(mapBuffer, cgs->mapname);

            {
                int len = 0;
                while (mapBuffer[len] != '\0')
                    len++;
                mapLen = len;
            }

            if (mapLen >= 4 && I_strnicmp(mapBuffer + mapLen - 4, (const char *)"bsp", 3) == 0) {
                mapBuffer[mapLen - 4] = '\0';
            }
        }

        map = mapBuffer;
        if (I_strnicmp(mapBuffer, (const char *)"maps", 4) == 0) {
            char c = mapBuffer[4];
            if (c == '/' || c == '\\') {
                map = mapBuffer + 5;
            }
        }

        if (I_strnicmp(map, (const char *)"mp", 2) == 0) {
            char c = map[2];
            if (c == '/' || c == '\\') {
                map = map + 3;
            }
        }

        fontScale = 0.41f;
        numLines = 14;
        {
            float curScale = fontScale;
            void *curFont;
            while (numLines > 0) {
                curFont = UI_GetFontHandle(0, curScale);
                headerFont = curFont;
                {
                    int w1 = UI_TextWidth(gameType, 0, curFont, curScale);
                    int w2 = UI_TextWidth(map, 0, curFont, curScale);
                    if ((float)(w1 + w2 + 4) <= 386.0f)
                        break;
                }
                curScale -= 0.025f;
                fontScale = curScale;
                numLines--;
            }
        }

        {
            int mapWidth = UI_TextWidth(map, 0, headerFont, fontScale);
            int textHeight2 = UI_TextHeight(headerFont, fontScale);
            float yPos2 = 51.0f + (float)(24 - textHeight2) * -0.5f;
            float xPos = 511.0f - (float)(mapWidth + 4);
            UI_DrawText(map, 0x7fffffff, headerFont, xPos, yPos2, 0, 0, fontScale, color, 3);
        }
    }

    {
        const vec_t *cw = (const vec_t *)imp_colorWhite;
        color[0] = cw[0];
        color[1] = cw[1];
        color[2] = cw[2];
        color[3] = fade;

        serverName = cgs->szHostName;
        serverIP = (const char *)CL_GetServerIPAddress();

        if (I_stricmp(serverIP, (const char *)"0.0.0.0:0") == 0) {
            serverIP = (const char *)UI_SafeTranslateString((const char *)"CGAME_LISTENSERVER");
        }

        footerFontScale = 0.2f;
        numLines = 13;
        {
            float curScale = footerFontScale;
            void *curFont;
            while (numLines > 0) {
                curFont = UI_GetFontHandle(0, curScale);
                footerFont = curFont;
                {
                    int w1 = UI_TextWidth(serverName, 0, curFont, curScale);
                    int w2 = UI_TextWidth(serverIP, 0, curFont, curScale);
                    if ((float)(w1 + w2 + 4) <= 386.0f)
                        break;
                }
                curScale -= 0.01f;
                footerFontScale = curScale;
                numLines--;
            }
        }

        {
            int textHeight3 = UI_TextHeight(footerFont, footerFontScale);
            y = 447.0f + (float)(14 - textHeight3) * -0.5f;
        }

        UI_DrawText(serverName, 0x7fffffff, footerFont, 129.0f, y, 0, 0, footerFontScale, color, 3);

        {
            int ipWidth = UI_TextWidth(serverIP, 0, footerFont, footerFontScale);
            float xPos = 511.0f - (float)(ipWidth + 4);
            UI_DrawText(serverIP, 0x7fffffff, footerFont, xPos, y, 0, 0, footerFontScale, color, 3);
        }
    }

    CG_DrawScoreboard_ScoresList(fade);

scoreboard_done:
    return 1;
}
