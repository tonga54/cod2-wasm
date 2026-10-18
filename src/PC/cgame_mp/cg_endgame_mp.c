#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_endgame.h"
#include "headers/PC/cgame_mp/cg_scoretable.h"
#include "headers/PC/gfx_d3d/r_font_wwii.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern FontHandle CL_RegisterFont(const char *fontName, int imageTrack);
extern float CL_NormalizedTextScale(FontHandle font, float scale);
extern void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style);
extern void CL_DrawStretchPicPhysical(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material);
extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);
extern const char *UI_GetGameTypeDisplayName(const char *gameType);
extern const char *SEH_LocalizeTextMessage(const char *msg, const char *context, int errType);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);

/* Virtual units of the 4:3 area, measured on the WWII capture (1.2 pixels a
 * unit): the result over a rule and the mode at the map, the two sides
 * either side of VS with their names, emblems and scores in diamonds, and
 * why the match ended at the bottom. Heights given for text are of its
 * capitals; a line's y is its middle, and it is a pixel thick. */
#define ENDGAME_ALIGN 0
#define ENDGAME_FULLSCREEN 4
#define ENDGAME_MIDDLE 320.0f
#define ENDGAME_SIDE 160.0f
#define ENDGAME_TITLE_BASELINE 63.3f
#define ENDGAME_TITLE_CAP 26.7f
#define ENDGAME_TITLE_ROOM 600.0f
#define ENDGAME_RULE_Y 72.9f
#define ENDGAME_RULE_HALF 143.3f
#define ENDGAME_RULE_ALPHA 0.2f
#define ENDGAME_UNDERLINE_Y 75.4f
#define ENDGAME_UNDERLINE_HALF 133.3f
#define ENDGAME_UNDERLINE_ALPHA 0.15f
/* Two chevrons under the rule's middle, a tip below each top. */
#define ENDGAME_CHEVRON_Y 74.6f
#define ENDGAME_CHEVRON_GAP 2.5f
#define ENDGAME_CHEVRON_HALF 2.5f
#define ENDGAME_CHEVRON_DEPTH 1.7f
#define ENDGAME_CHEVRON_ALPHA 0.35f
#define ENDGAME_SUBTITLE_BASELINE 90.8f
#define ENDGAME_SUBTITLE_CAP 6.25f
#define ENDGAME_SUBTITLE_ROOM 420.0f
#define ENDGAME_LABEL_BASELINE 138.3f
#define ENDGAME_LABEL_CAP 13.3f
#define ENDGAME_SIDE_ROOM 150.0f
/* The viewer's side has an arrow before its name. */
#define ENDGAME_MARKER_GAP 4.0f
#define ENDGAME_MARKER_WIDTH 4.6f
#define ENDGAME_MARKER_HEIGHT 5.8f
#define ENDGAME_EMBLEM_Y 220.0f
#define ENDGAME_EMBLEM_SIZE 132.0f
#define ENDGAME_EMBLEM_MIN_TEXELS 16
#define ENDGAME_EMBLEM_MAX_TEXELS 384
/* Texels of each medal rasterized a frame: a whole pair stalls a frame for
 * 10 to 40 ms natively, so the first frames of a map share it. */
#define ENDGAME_MEDAL_TEXELS_PER_FRAME 2048
/* A free-for-all names its best players where the emblems go. */
#define ENDGAME_NAME_CAP 13.3f
#define ENDGAME_VS_BASELINE 231.7f
#define ENDGAME_VS_CAP 25.0f
#define ENDGAME_VS_Y 218.2f
#define ENDGAME_VS_DIAMOND 29.0f
#define ENDGAME_VS_FILL 0.25f
#define ENDGAME_VS_EDGE 0.06f
/* Two lines over VS and two under it on either side, the outer pair nearer
 * to it and shorter: their distance from the middle, then from VS's. */
#define ENDGAME_VS_OUTER_DY 15.85f
#define ENDGAME_VS_OUTER_NEAR 27.5f
#define ENDGAME_VS_OUTER_FAR 62.1f
#define ENDGAME_VS_INNER_DY 12.0f
#define ENDGAME_VS_INNER_NEAR 31.25f
#define ENDGAME_VS_INNER_FAR 70.4f
#define ENDGAME_VS_LINE_ALPHA 0.3f
#define ENDGAME_SCORE_Y 335.4f
#define ENDGAME_SCORE_DIAMOND 39.0f
#define ENDGAME_SCORE_FILL 0.35f
#define ENDGAME_SCORE_EDGE 0.12f
#define ENDGAME_SCORE_BASELINE 351.7f
#define ENDGAME_SCORE_CAP 40.0f
/* A score wider than its diamond, as our points make them, shrinks to it. */
#define ENDGAME_SCORE_ROOM (2.0f * ENDGAME_SCORE_DIAMOND)
#define ENDGAME_REASON_BASELINE 432.5f
#define ENDGAME_REASON_CAP 8.3f
#define ENDGAME_DIM 0.7f
#define ENDGAME_TEXT_STYLE 3
/* Milliseconds into the intermission: the announcement fades in while its
 * rule grows from the middle, stays, then hands over to the scoreboard. The
 * stock scripts change the map after "wait 10", which our servers' sv_fps 30
 * runs in 6.7 s; the scoreboard keeps half of it. */
#define ENDGAME_FADE_MSEC 400
#define ENDGAME_RULE_MSEC 600
#define ENDGAME_ANNOUNCE_MSEC 3000
#define ENDGAME_HANDOVER_MSEC 500
#define ENDGAME_MAX_CLIENTS 64
#define ENDGAME_PODIUM 3
/* Marks a string the server sent without localizing it. */
#define ENDGAME_LITERAL 0x15

enum {
    ENDGAME_REASON_NONE,
    ENDGAME_REASON_TIME,
    ENDGAME_REASON_SCORE,
    ENDGAME_REASON_ROUND,
    ENDGAME_REASON_COUNT
};

enum {
    ENDGAME_DRAW,
    ENDGAME_VICTORY,
    ENDGAME_DEFEAT
};

typedef struct {
    char title[96];
    char reason[64];
    int freeForAll;
    /* The viewer's side on the left, the Allies for a spectator. */
    int teams[2];
    int scores[2];
    /* The side or place with the viewer's arrow, -1 for none. */
    int marked;
    /* A free-for-all's best players, the first in the middle. */
    int podium;
    int clients[ENDGAME_PODIUM];
    int podiumScores[ENDGAME_PODIUM];
} CG_EndgameView;

typedef struct {
    int intermission;
    int start;
    int reason;
    int medalSize;
    int medalKinds[2];
    /* The image while its rows are rasterized, then the material. */
    byte *medalImage;
    int medalRows;
    MaterialHandle medals;
    FontHandle boldFont;
    FontHandle regularFont;
} CG_EndgameState;

static const char *const s_endgameReasons[ENDGAME_REASON_COUNT] = {
    "", "MP_TIME_LIMIT_REACHED", "MP_SCORE_LIMIT_REACHED", "MP_ROUND_LIMIT_REACHED",
};
static const char *const s_endgamePlaces[ENDGAME_PODIUM] = { "1ST", "2ND", "3RD" };
/* Places left to right: second, first, third. */
static const float s_endgameColumns[ENDGAME_PODIUM] = { 0.0f, -1.0f, 1.0f };
static const vec4_t s_endgameText = { 0.82f, 0.79f, 0.74f, 1.0f };
static const vec3_t s_endgameBlack = { 0.0f, 0.0f, 0.0f };
static const vec3_t s_endgameWhite = { 1.0f, 1.0f, 1.0f };

static CG_EndgameState s_endgame;
static CG_EndgameView s_endgameView;
static const dvar_t *cg_hudEndgame;
float g_webBackdropFrac = 0.0f;

static int CG_EndgameEnabled(void)
{
    return cg_hudEndgame && cg_hudEndgame->current.enabled;
}

static void CG_EndgameColor(vec4_t out, const vec_t *rgb, float alpha)
{
    out[0] = rgb[0];
    out[1] = rgb[1];
    out[2] = rgb[2];
    out[3] = alpha;
}

static float CG_EndgamePixelsPerUnit(void)
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 1.0f;
    float h = 1.0f;

    CalcScreenPlacement(&x, &y, &w, &h, ENDGAME_ALIGN, ENDGAME_ALIGN);
    return h;
}

static void CG_EndgamePlace(float x, float y, float *px, float *py)
{
    float w = 0.0f;
    float h = 0.0f;

    CalcScreenPlacement(&x, &y, &w, &h, ENDGAME_ALIGN, ENDGAME_ALIGN);
    *px = x;
    *py = y;
}

/* One image holds the Allies' medal on the left and the Axis', drawn a
 * texel to a pixel; built again when the screen or a nation changes. */
static void CG_EndgamePrepare(void)
{
    int kinds[2];
    int size;
    int rows;
    int i;

    if (!CG_EndgameEnabled())
        return;
    size = (int)(ENDGAME_EMBLEM_SIZE * CG_EndgamePixelsPerUnit() + 0.5f);
    if (size < ENDGAME_EMBLEM_MIN_TEXELS)
        size = ENDGAME_EMBLEM_MIN_TEXELS;
    else if (size > ENDGAME_EMBLEM_MAX_TEXELS)
        size = ENDGAME_EMBLEM_MAX_TEXELS;
    kinds[0] = CG_ScoreTableEmblemKind(TEAM_ALLIES);
    kinds[1] = CG_ScoreTableEmblemKind(TEAM_AXIS);
    if (size != s_endgame.medalSize || kinds[0] != s_endgame.medalKinds[0] || kinds[1] != s_endgame.medalKinds[1]) {
        s_endgame.medalSize = size;
        s_endgame.medalKinds[0] = kinds[0];
        s_endgame.medalKinds[1] = kinds[1];
        s_endgame.medals = NULL;
        s_endgame.medalRows = 0;
        free(s_endgame.medalImage);
        s_endgame.medalImage = (byte *)malloc((size_t)size * (size_t)size * 8);
    }
    if (!s_endgame.medalImage)
        return;
    rows = ENDGAME_MEDAL_TEXELS_PER_FRAME / size;
    if (rows > size - s_endgame.medalRows)
        rows = size - s_endgame.medalRows;
    for (i = 0; i < 2; ++i)
        CG_ScoreTableRasterizeMedal(kinds[i], s_endgame.medalImage + i * size * 4, size * 8, size,
                                    s_endgame.medalRows, rows);
    s_endgame.medalRows += rows;
    if (s_endgame.medalRows < size)
        return;
    s_endgame.medals = R_CreateHudImageMaterial(R_HUD_IMAGE_ENDGAME, s_endgame.medalImage, 2 * size, size);
    free(s_endgame.medalImage);
    s_endgame.medalImage = NULL;
}

void CG_EndgameInit(void)
{
    cg_hudEndgame = Dvar_RegisterBool_mac("cg_hudEndgame", 1, 0x1001);
    free(s_endgame.medalImage);
    memset(&s_endgame, 0, sizeof(s_endgame));
    s_endgame.medalKinds[0] = -1;
    s_endgame.medalKinds[1] = -1;
    s_endgame.boldFont = CL_RegisterFont("$boldfont", 0);
    s_endgame.regularFont = R_RegisterWwiiFont("$regularfont");
    g_webBackdropFrac = 0.0f;
}

void CG_EndgameMessage(const char *message)
{
    int i;

    for (i = ENDGAME_REASON_TIME; message && i < ENDGAME_REASON_COUNT; ++i) {
        if (!strcmp(message, s_endgameReasons[i]))
            s_endgame.reason = i;
    }
}

/* The medals are built with the first frames of a map, not when it ends. */
void CG_EndgameFrame(int intermission)
{
    if (intermission && !s_endgame.intermission)
        s_endgame.start = cg->time;
    s_endgame.intermission = intermission;
    g_webBackdropFrac = 0.0f;
    CG_EndgamePrepare();
}

static const char *CG_EndgameObjective(void)
{
    const char *text = cg->objectiveText;

    return (unsigned char)text[0] == ENDGAME_LITERAL ? text + 1 : text;
}

/* The winner the stock scripts name, TEAM_FREE for a tie; without one, the
 * team scores the server keeps current decide. */
static int CG_EndgameTeamWinner(void)
{
    const char *text = CG_EndgameObjective();

    if (!strcmp(text, "MP_ALLIES_WIN"))
        return TEAM_ALLIES;
    if (!strcmp(text, "MP_AXIS_WIN"))
        return TEAM_AXIS;
    if (!strcmp(text, "MP_THE_GAME_IS_A_TIE"))
        return TEAM_FREE;
    if (cgs->teamScores[1] != cgs->teamScores[0])
        return cgs->teamScores[1] > cgs->teamScores[0] ? TEAM_ALLIES : TEAM_AXIS;
    return TEAM_FREE;
}

static const char *CG_EndgameTeamName(int team)
{
    return team == TEAM_AXIS ? "AXIS" : "ALLIES";
}

/* teamScores holds the Axis first. */
static void CG_EndgameBuildTeams(CG_EndgameView *view, const clientInfo_t *local)
{
    int own = local->infoValid && (local->team == TEAM_AXIS || local->team == TEAM_ALLIES) ? local->team
                                                                                           : TEAM_SPECTATOR;
    int winner = CG_EndgameTeamWinner();
    int i;

    view->teams[0] = own == TEAM_AXIS ? TEAM_AXIS : TEAM_ALLIES;
    view->teams[1] = view->teams[0] == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS;
    view->marked = own == TEAM_SPECTATOR ? -1 : 0;
    for (i = 0; i < 2; ++i)
        view->scores[i] = cgs->teamScores[view->teams[i] == TEAM_AXIS ? 0 : 1];
    if (winner == TEAM_FREE)
        I_strncpyz(view->title, "DRAW", sizeof(view->title));
    else if (own == TEAM_SPECTATOR)
        Com_sprintf(view->title, sizeof(view->title), "%s VICTORY", CG_EndgameTeamName(winner));
    else
        Com_sprintf(view->title, sizeof(view->title), "%s %s", CG_EndgameTeamName(own),
                    winner == own ? "VICTORY" : "DEFEAT");
}

/* "MP_WINS" names the winner the way the server ends a player's name. */
static int CG_EndgameIsWinner(const char *winner, const char *name)
{
    int length = (int)strlen(winner);

    if (length >= 2 && winner[length - 2] == '^' && winner[length - 1] == '7' &&
        (int)strlen(name) == length - 2 && !strncmp(winner, name, (size_t)(length - 2)))
        return 1;
    return !strcmp(winner, name);
}

/* The best three in the server's order; the stock script names the winner
 * or calls a tie, and without it equal best scores are a draw. */
static void CG_EndgameBuildPodium(CG_EndgameView *view, int localClient, const clientInfo_t *local)
{
    const char *text = CG_EndgameObjective();
    int playing = local->infoValid && local->team != TEAM_SPECTATOR;
    int outcome;
    int i;

    view->podium = 0;
    view->marked = -1;
    for (i = 0; i < cg->numScores && view->podium < ENDGAME_PODIUM; ++i) {
        const score_t *score = &cg->scores[i];
        if ((unsigned)score->client >= ENDGAME_MAX_CLIENTS || !cg->bgs.clientinfo[score->client].infoValid ||
            score->team == TEAM_SPECTATOR)
            continue;
        if (score->client == localClient)
            view->marked = view->podium;
        view->clients[view->podium] = score->client;
        view->podiumScores[view->podium++] = score->score;
    }

    if (!strcmp(text, "MP_THE_GAME_IS_A_TIE"))
        outcome = ENDGAME_DRAW;
    else if (!strncmp(text, "MP_WINS\x15", 8))
        outcome = CG_EndgameIsWinner(text + 8, local->name) ? ENDGAME_VICTORY : ENDGAME_DEFEAT;
    else if (!view->podium || (view->podium > 1 && view->podiumScores[0] == view->podiumScores[1]))
        outcome = ENDGAME_DRAW;
    else
        outcome = view->clients[0] == localClient ? ENDGAME_VICTORY : ENDGAME_DEFEAT;

    if (outcome == ENDGAME_DRAW)
        I_strncpyz(view->title, "DRAW", sizeof(view->title));
    else if (playing)
        I_strncpyz(view->title, outcome == ENDGAME_VICTORY ? "VICTORY" : "DEFEAT", sizeof(view->title));
    else if (!strncmp(text, "MP_WINS\x15", 8))
        Com_sprintf(view->title, sizeof(view->title), "%s WINS", text + 8);
    else
        Com_sprintf(view->title, sizeof(view->title), "%s^7 WINS", cg->bgs.clientinfo[view->clients[0]].name);
}

/* "Time limit reached." without its period, as WWII writes it. */
static void CG_EndgameReasonText(int reason, char *buf, int size)
{
    int length;

    buf[0] = 0;
    if (reason <= ENDGAME_REASON_NONE || reason >= ENDGAME_REASON_COUNT)
        return;
    I_strncpyz(buf, SEH_LocalizeTextMessage(s_endgameReasons[reason], "endgame reason", 0), size);
    length = (int)strlen(buf);
    if (length && buf[length - 1] == '.')
        buf[length - 1] = 0;
}

static void CG_EndgameBuildView(CG_EndgameView *view, int localClient)
{
    const clientInfo_t *local = &cg->bgs.clientinfo[localClient];

    view->freeForAll = CG_ScoreTableFreeForAll();
    if (view->freeForAll)
        CG_EndgameBuildPodium(view, localClient, local);
    else
        CG_EndgameBuildTeams(view, local);
    CG_EndgameReasonText(s_endgame.reason, view->reason, sizeof(view->reason));
}

/* Text whose capitals stand cap tall on the baseline, centered on x and
 * shrunk to fit room; returns where its pen starts. */
static float CG_EndgameText(const char *text, FontHandle font, float cap, float x, float baseline, float room,
                            const vec_t *color)
{
    float scale;
    float width;
    float unit;
    float units;
    float rise;

    if (!font || !text[0])
        return x;
    scale = CG_ScoreTableCapScale(font, cap);
    width = CG_ScoreTableTextWidth(text, 0, font, scale);
    if (room > 0.0f && width > room) {
        scale *= room / width;
        width = room;
    }
    x -= 0.5f * width;
    unit = CL_NormalizedTextScale(font, scale);
    CG_ScoreTableFontMetrics(font, &units, &rise);
    UI_DrawText(text, 0x7fffffff, font, x + 0.5f * unit, baseline + (rise + 0.5f) * unit, ENDGAME_ALIGN, ENDGAME_ALIGN,
                scale, color, ENDGAME_TEXT_STYLE);
    return x;
}

/* A pixel-thick line from x0 to x1 in the pixel row holding y. */
static void CG_EndgameLine(float x0, float x1, float y, const vec_t *color)
{
    float px0;
    float px1;
    float py;

    CG_EndgamePlace(x0, y, &px0, &py);
    CG_EndgamePlace(x1, y, &px1, &py);
    px0 = floorf(px0 + 0.5f);
    px1 = floorf(px1 + 0.5f);
    if (px1 > px0 && color[3] > 0.0f)
        CL_DrawStretchPicPhysical(px0, floorf(py), px1 - px0, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, color,
                                  cgs->media.whiteMaterial);
}

static void CG_EndgameVertex(HudVertex2D *vertex, float x, float y, const vec_t *color)
{
    memset(vertex, 0, sizeof(*vertex));
    vertex->xy[0] = x;
    vertex->xy[1] = y;
    memcpy(vertex->rgba, color, sizeof(vertex->rgba));
}

/* A pixel-wide line whose sides fade out over a pixel. */
static void CG_EndgameStroke(float x0, float y0, float x1, float y1, const vec_t *color)
{
    static const unsigned short indices[12] = { 0, 1, 4, 0, 4, 3, 1, 2, 5, 1, 5, 4 };
    HudVertex2D verts[6];
    vec4_t clear;
    float a[2];
    float b[2];
    float length;
    float nx;
    float ny;
    int i;

    CG_EndgamePlace(x0, y0, &a[0], &a[1]);
    CG_EndgamePlace(x1, y1, &b[0], &b[1]);
    length = sqrtf((b[0] - a[0]) * (b[0] - a[0]) + (b[1] - a[1]) * (b[1] - a[1]));
    if (length <= 0.0f || color[3] <= 0.0f)
        return;
    nx = -(b[1] - a[1]) / length;
    ny = (b[0] - a[0]) / length;
    CG_EndgameColor(clear, color, 0.0f);
    for (i = 0; i < 3; ++i) {
        float side = (float)(i - 1);
        CG_EndgameVertex(&verts[i], a[0] + nx * side, a[1] + ny * side, i == 1 ? color : clear);
        CG_EndgameVertex(&verts[3 + i], b[0] + nx * side, b[1] + ny * side, i == 1 ? color : clear);
    }
    R_AddCmdDrawTriangles2D(verts, 6, indices, 12, cgs->media.whiteMaterial);
}

/* A diamond reaching half either way from (x, y): its dark fill, then a
 * pixel-wide outline whose sides fade out over a pixel. */
static void CG_EndgameDiamond(float x, float y, float half, float fillAlpha, float edgeAlpha, float fade)
{
    static const float corners[4][2] = { { 0.0f, -1.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { -1.0f, 0.0f } };
    HudVertex2D verts[16];
    unsigned short indices[54];
    vec4_t fill;
    vec4_t edge;
    vec4_t clear;
    float px;
    float py;
    float radius = half * CG_EndgamePixelsPerUnit();
    int count = 0;
    int ring;
    int i;

    CG_EndgamePlace(x, y, &px, &py);
    CG_EndgameColor(fill, s_endgameBlack, fillAlpha * fade);
    CG_EndgameColor(edge, s_endgameText, edgeAlpha * fade);
    CG_EndgameColor(clear, s_endgameText, 0.0f);
    for (i = 0; i < 4; ++i)
        CG_EndgameVertex(&verts[i], px + corners[i][0] * radius, py + corners[i][1] * radius, fill);
    /* The outline's middle and both its faded sides: a pixel across an
     * edge at 45 degrees moves a corner by the square root of two. */
    for (ring = 0; ring < 3; ++ring) {
        float r = radius + (float)(ring - 1) * 1.41421356f;
        for (i = 0; i < 4; ++i)
            CG_EndgameVertex(&verts[4 + ring * 4 + i], px + corners[i][0] * r, py + corners[i][1] * r,
                             ring == 1 ? edge : clear);
    }
    indices[count++] = 0;
    indices[count++] = 1;
    indices[count++] = 2;
    indices[count++] = 0;
    indices[count++] = 2;
    indices[count++] = 3;
    for (ring = 0; ring < 2; ++ring) {
        for (i = 0; i < 4; ++i) {
            unsigned short a = (unsigned short)(4 + ring * 4 + i);
            unsigned short b = (unsigned short)(4 + ring * 4 + (i + 1) % 4);
            indices[count++] = a;
            indices[count++] = b;
            indices[count++] = (unsigned short)(b + 4);
            indices[count++] = a;
            indices[count++] = (unsigned short)(b + 4);
            indices[count++] = (unsigned short)(a + 4);
        }
    }
    R_AddCmdDrawTriangles2D(verts, 16, indices, count, cgs->media.whiteMaterial);
}

/* The viewer's arrow, its tip at x, level with a capital's middle at y. */
static void CG_EndgameMarker(float x, float y, const vec_t *color)
{
    static const unsigned short indices[3] = { 0, 1, 2 };
    HudVertex2D verts[3];
    float px[3];
    float py[3];
    int i;

    CG_EndgamePlace(x - ENDGAME_MARKER_WIDTH, y - 0.5f * ENDGAME_MARKER_HEIGHT, &px[0], &py[0]);
    CG_EndgamePlace(x - ENDGAME_MARKER_WIDTH, y + 0.5f * ENDGAME_MARKER_HEIGHT, &px[1], &py[1]);
    CG_EndgamePlace(x, y, &px[2], &py[2]);
    for (i = 0; i < 3; ++i)
        CG_EndgameVertex(&verts[i], px[i], py[i], color);
    R_AddCmdDrawTriangles2D(verts, 3, indices, 3, cgs->media.whiteMaterial);
}

/* A name or place over x, with the viewer's arrow before it. */
static void CG_EndgameDrawLabel(const char *label, float x, int marked, const vec_t *color)
{
    float pen = CG_EndgameText(label, s_endgame.boldFont, ENDGAME_LABEL_CAP, x, ENDGAME_LABEL_BASELINE,
                               ENDGAME_SIDE_ROOM, color);

    if (marked && s_endgame.boldFont)
        CG_EndgameMarker(pen - ENDGAME_MARKER_GAP, ENDGAME_LABEL_BASELINE - 0.5f * ENDGAME_LABEL_CAP, color);
}

static void CG_EndgameDrawScore(int score, float x, float alpha, const vec_t *color)
{
    char number[16];

    CG_EndgameDiamond(x, ENDGAME_SCORE_Y, ENDGAME_SCORE_DIAMOND, ENDGAME_SCORE_FILL, ENDGAME_SCORE_EDGE, alpha);
    Com_sprintf(number, sizeof(number), "%i", score);
    CG_EndgameText(number, s_endgame.boldFont, ENDGAME_SCORE_CAP, x, ENDGAME_SCORE_BASELINE, ENDGAME_SCORE_ROOM, color);
}

static void CG_EndgameDrawMedal(int team, float x, float alpha)
{
    int axis = team == TEAM_AXIS;
    float size = (float)s_endgame.medalSize;
    vec4_t color;
    float px;
    float py;

    if (!s_endgame.medals || s_endgame.medalKinds[axis] < 0)
        return;
    CG_EndgamePlace(x, ENDGAME_EMBLEM_Y, &px, &py);
    CG_EndgameColor(color, s_endgameWhite, alpha);
    CL_DrawStretchPicPhysical(floorf(px - 0.5f * size + 0.5f), floorf(py - 0.5f * size + 0.5f), size, size,
                              axis ? 0.5f : 0.0f, 0.0f, axis ? 1.0f : 0.5f, 1.0f, color, s_endgame.medals);
}

static void CG_EndgameDrawTeams(const CG_EndgameView *view, float alpha, const vec_t *color)
{
    static const float rows[4] = { -ENDGAME_VS_OUTER_DY, -ENDGAME_VS_INNER_DY, ENDGAME_VS_INNER_DY,
                                   ENDGAME_VS_OUTER_DY };
    vec4_t line;
    int side;
    int i;

    for (side = 0; side < 2; ++side) {
        float x = ENDGAME_MIDDLE + (side ? ENDGAME_SIDE : -ENDGAME_SIDE);
        CG_EndgameDrawLabel(CG_EndgameTeamName(view->teams[side]), x, view->marked == side, color);
        CG_EndgameDrawMedal(view->teams[side], x, alpha);
        CG_EndgameDrawScore(view->scores[side], x, alpha, color);
    }
    CG_EndgameDiamond(ENDGAME_MIDDLE, ENDGAME_VS_Y, ENDGAME_VS_DIAMOND, ENDGAME_VS_FILL, ENDGAME_VS_EDGE, alpha);
    CG_EndgameText("VS", s_endgame.boldFont, ENDGAME_VS_CAP, ENDGAME_MIDDLE, ENDGAME_VS_BASELINE, 0.0f, color);
    CG_EndgameColor(line, s_endgameText, ENDGAME_VS_LINE_ALPHA * alpha);
    for (i = 0; i < 4; ++i) {
        int outer = i == 0 || i == 3;
        float nearX = outer ? ENDGAME_VS_OUTER_NEAR : ENDGAME_VS_INNER_NEAR;
        float farX = outer ? ENDGAME_VS_OUTER_FAR : ENDGAME_VS_INNER_FAR;
        CG_EndgameLine(ENDGAME_MIDDLE - farX, ENDGAME_MIDDLE - nearX, ENDGAME_VS_Y + rows[i], line);
        CG_EndgameLine(ENDGAME_MIDDLE + nearX, ENDGAME_MIDDLE + farX, ENDGAME_VS_Y + rows[i], line);
    }
}

static void CG_EndgameDrawPodium(const CG_EndgameView *view, float alpha, const vec_t *color)
{
    int i;

    for (i = 0; i < view->podium; ++i) {
        float x = ENDGAME_MIDDLE + s_endgameColumns[i] * ENDGAME_SIDE;
        char name[64];
        Com_sprintf(name, sizeof(name), "%s^7", cg->bgs.clientinfo[view->clients[i]].name);
        CG_EndgameDrawLabel(s_endgamePlaces[i], x, view->marked == i, color);
        CG_EndgameText(name, s_endgame.boldFont, ENDGAME_NAME_CAP, x, ENDGAME_EMBLEM_Y + 0.5f * ENDGAME_NAME_CAP,
                       ENDGAME_SIDE_ROOM, color);
        CG_EndgameDrawScore(view->podiumScores[i], x, alpha, color);
    }
}

static void CG_EndgameDrawAnnouncement(const CG_EndgameView *view, float alpha, int elapsed)
{
    FontHandle reasonFont = s_endgame.regularFont ? s_endgame.regularFont : s_endgame.boldFont;
    float grow = elapsed >= ENDGAME_RULE_MSEC ? 1.0f : (float)elapsed / (float)ENDGAME_RULE_MSEC;
    char subtitle[160];
    char mode[64];
    char map[64];
    char reason[64];
    vec4_t color;
    float x = 0.0f;
    float y = 0.0f;
    float w = 640.0f;
    float h = 480.0f;
    int c;
    int i;

    CalcScreenPlacement(&x, &y, &w, &h, ENDGAME_FULLSCREEN, ENDGAME_FULLSCREEN);
    CG_EndgameColor(color, s_endgameBlack, ENDGAME_DIM * alpha);
    CL_DrawStretchPicPhysical(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, color, cgs->media.whiteMaterial);

    CG_EndgameColor(color, s_endgameText, alpha);
    CG_EndgameText(view->title, s_endgame.boldFont, ENDGAME_TITLE_CAP, ENDGAME_MIDDLE, ENDGAME_TITLE_BASELINE,
                   ENDGAME_TITLE_ROOM, color);

    /* The rule eases out from the middle. */
    grow = 1.0f - (1.0f - grow) * (1.0f - grow) * (1.0f - grow);
    CG_EndgameColor(color, s_endgameText, ENDGAME_RULE_ALPHA * alpha);
    CG_EndgameLine(ENDGAME_MIDDLE - ENDGAME_RULE_HALF * grow, ENDGAME_MIDDLE + ENDGAME_RULE_HALF * grow,
                   ENDGAME_RULE_Y, color);
    CG_EndgameColor(color, s_endgameText, ENDGAME_UNDERLINE_ALPHA * alpha);
    CG_EndgameLine(ENDGAME_MIDDLE - ENDGAME_UNDERLINE_HALF * grow, ENDGAME_MIDDLE + ENDGAME_UNDERLINE_HALF * grow,
                   ENDGAME_UNDERLINE_Y, color);
    CG_EndgameColor(color, s_endgameText, ENDGAME_CHEVRON_ALPHA * alpha);
    for (c = 0; c < 2; ++c) {
        float top = ENDGAME_CHEVRON_Y + (float)c * ENDGAME_CHEVRON_GAP;
        CG_EndgameStroke(ENDGAME_MIDDLE - ENDGAME_CHEVRON_HALF, top, ENDGAME_MIDDLE, top + ENDGAME_CHEVRON_DEPTH,
                         color);
        CG_EndgameStroke(ENDGAME_MIDDLE, top + ENDGAME_CHEVRON_DEPTH, ENDGAME_MIDDLE + ENDGAME_CHEVRON_HALF, top,
                         color);
    }

    CG_EndgameColor(color, s_endgameText, alpha);
    I_strncpyz(mode, SEH_LocalizeTextMessage(UI_GetGameTypeDisplayName(cgs->gametype), "scoreboard gametype display",
                                             0),
               sizeof(mode));
    CG_ScoreTableMapName(map, sizeof(map));
    Com_sprintf(subtitle, sizeof(subtitle), "%s at %s", mode, map);
    CG_EndgameText(subtitle, s_endgame.boldFont, ENDGAME_SUBTITLE_CAP, ENDGAME_MIDDLE, ENDGAME_SUBTITLE_BASELINE,
                   ENDGAME_SUBTITLE_ROOM, color);

    if (view->freeForAll)
        CG_EndgameDrawPodium(view, alpha, color);
    else
        CG_EndgameDrawTeams(view, alpha, color);

    for (i = 0; view->reason[i] && i < (int)sizeof(reason) - 1; ++i)
        reason[i] = view->reason[i] >= 'a' && view->reason[i] <= 'z' ? (char)(view->reason[i] - 'a' + 'A')
                                                                     : view->reason[i];
    reason[i] = 0;
    CG_EndgameText(reason, reasonFont, ENDGAME_REASON_CAP, ENDGAME_MIDDLE, ENDGAME_REASON_BASELINE,
                   ENDGAME_TITLE_ROOM, color);
}

int CG_DrawEndgame(float fade)
{
    CG_EndgameView *view = &s_endgameView;
    int localClient = cg->clientNum;
    int elapsed;
    float announce;
    float summary;

    if (!s_endgame.intermission || !CG_EndgameEnabled() || (unsigned)localClient >= ENDGAME_MAX_CLIENTS)
        return 0;
    elapsed = cg->time - s_endgame.start;
    if (elapsed < 0)
        elapsed = 0;
    if (elapsed < ENDGAME_FADE_MSEC)
        announce = (float)elapsed / (float)ENDGAME_FADE_MSEC;
    else if (elapsed < ENDGAME_ANNOUNCE_MSEC)
        announce = 1.0f;
    else if (elapsed < ENDGAME_ANNOUNCE_MSEC + ENDGAME_HANDOVER_MSEC)
        announce = 1.0f - (float)(elapsed - ENDGAME_ANNOUNCE_MSEC) / (float)ENDGAME_HANDOVER_MSEC;
    else
        announce = 0.0f;
    summary = elapsed <= ENDGAME_ANNOUNCE_MSEC ? 0.0f : 1.0f - announce;

    CG_EndgameBuildView(view, localClient);
    if (summary > 0.0f) {
        CG_DrawScoreTableResult(fade * summary, view->title, view->reason);
        g_webBackdropFrac = summary;
    }
    if (announce > 0.0f)
        CG_EndgameDrawAnnouncement(view, fade * announce, elapsed);
    return 1;
}
