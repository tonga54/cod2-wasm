#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_scorepanel.h"
#include "headers/PC/cgame_mp/cg_scoretable.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"

#include <stdlib.h>
#include <string.h>

extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern FontHandle UI_GetFontHandle(int fontEnum, float scale);
extern float CL_NormalizedTextScale(FontHandle font, float scale);
extern void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style);
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material);
extern MaterialHandle CL_RegisterMaterial(const char *name, int imageTrack);
extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);
extern const char *CL_GetConfigString(int index);
extern const char *Info_ValueForKey(const char *s, const char *key);
extern const char *UI_GetGameTypeDisplayName(const char *gameType);
extern const char *SEH_LocalizeTextMessage(const char *msg, const char *context, int errType);
extern int CG_ServerMaterialName(int materialIndex, char *buf, int bufSize);
extern const char *CG_GetHudElemTimer(const hudelem_t *elem, vec4_t color);
extern int I_stricmp(const char *s1, const char *s2);
extern const char *va(const char *format, ...);

/* Virtual units from the bottom-left corner of the safe area, measured on the
 * WWII capture: the label and the clock above two bars, the viewer's side in
 * the taller one on top. */
#define SCORE_PANEL_BAR_X 40.0f
#define SCORE_PANEL_BAR_WIDTH 125.0f
#define SCORE_PANEL_TOP_Y -52.0f
#define SCORE_PANEL_TOP_HEIGHT 18.0f
#define SCORE_PANEL_BOTTOM_Y -33.0f
#define SCORE_PANEL_BOTTOM_HEIGHT 9.0f
#define SCORE_PANEL_NUMBER_PAD 4.0f
#define SCORE_PANEL_LABEL_X 42.0f
#define SCORE_PANEL_LABEL_GAP 6.0f
#define SCORE_PANEL_FONT 0
#define SCORE_PANEL_CLOCK_RIGHT 172.0f
/* The capitals' and digits' heights, and the line the label and the clock
 * stand on: on the capture "Winning" is 9 px tall, the clock's digits 12.7,
 * the scores' 12.5 and 8.6, at 1.2 px a unit, on baselines 57.9 and 58.6
 * units up. */
#define SCORE_PANEL_LABEL_CAP 7.5f
#define SCORE_PANEL_CLOCK_CAP 10.6f
#define SCORE_PANEL_TOP_CAP 10.4f
#define SCORE_PANEL_BOTTOM_CAP 7.2f
#define SCORE_PANEL_TEXT_BASELINE -58.2f
#define SCORE_PANEL_EMBLEM_X 8.0f
#define SCORE_PANEL_EMBLEM_Y -55.0f
#define SCORE_PANEL_EMBLEM_SIZE 34.0f
#define SCORE_PANEL_EMBLEM_ALPHA 0.35f
#define SCORE_PANEL_ARROW_X 34.5f
#define SCORE_PANEL_ARROW_WIDTH 4.5f
#define SCORE_PANEL_ARROW_HEIGHT 7.0f
#define SCORE_PANEL_STANDING_MSEC 5000
#define SCORE_PANEL_LABEL_FADE_MSEC 250
#define SCORE_PANEL_ALIGN_LEFT 1
#define SCORE_PANEL_ALIGN_BOTTOM 3
/* Where the stock scripts anchor their clock and scores. */
#define SCORE_PANEL_SCREEN_TOP_LEFT 9
#define SCORE_PANEL_MAX_CLIENTS 64
#define SCORE_PANEL_MAX_ICONS 2

enum {
    SCORE_PANEL_DM,
    SCORE_PANEL_TDM,
    SCORE_PANEL_SD,
    SCORE_PANEL_CTF,
    SCORE_PANEL_HQ,
    SCORE_PANEL_GAMETYPE_COUNT
};

enum {
    SCORE_PANEL_ELEM_NONE,
    SCORE_PANEL_ELEM_CLOCK,
    SCORE_PANEL_ELEM_ICON,
    SCORE_PANEL_ELEM_SCORE
};

enum {
    SCORE_PANEL_UNSEEN = -2,
    /* The view has no side: a spectator. */
    SCORE_PANEL_NEUTRAL = -1,
    SCORE_PANEL_TIED,
    SCORE_PANEL_WINNING,
    SCORE_PANEL_LOSING,
    SCORE_PANEL_STANDING_COUNT
};

typedef struct {
    int topScore;
    int bottomScore;
    int limit;
    int standing;
    int team;
} CG_ScorePanelView;

/* The stock elements the panel takes its clock and emblem from. */
typedef struct {
    const hudelem_t *clock;
    const hudelem_t *icons[SCORE_PANEL_MAX_ICONS];
    int iconCount;
} CG_ScorePanelRetail;

typedef struct {
    int standing;
    int standingTime;
    int showStanding;
    int dmScores[SCORE_PANEL_MAX_CLIENTS];
} CG_ScorePanelState;

static const char *const s_scorePanelGametypes[SCORE_PANEL_GAMETYPE_COUNT] = { "dm", "tdm", "sd", "ctf", "hq" };
static const char *const s_scorePanelStandings[SCORE_PANEL_STANDING_COUNT] = { "Tied", "Winning", "Losing" };
static const vec4_t s_scorePanelTrack = { 0.0f, 0.0f, 0.0f, 0.4f };
static const vec4_t s_scorePanelTopFill = { 0.22f, 0.59f, 0.59f, 0.9f };
static const vec4_t s_scorePanelBottomFill = { 0.69f, 0.25f, 0.16f, 0.9f };
static const vec4_t s_scorePanelText = { 0.96f, 0.95f, 0.92f, 1.0f };

static CG_ScorePanelState s_scorePanel;
static const dvar_t *cg_hudScorePanel;

static int CG_ScorePanelGametype(const char *gametype)
{
    int i;

    for (i = 0; i < SCORE_PANEL_GAMETYPE_COUNT; ++i) {
        if (!I_stricmp(gametype, s_scorePanelGametypes[i]))
            return i;
    }
    return -1;
}

static int CG_ScorePanelActiveGametype(void)
{
    if (!cg_hudScorePanel || !cg_hudScorePanel->current.enabled)
        return -1;
    return CG_ScorePanelGametype(cgs->gametype);
}

/* The stock scripts' clock, team or player icons, scores and score limits,
 * told apart by type and position. A limit moves right as its score widens. */
static int CG_ScorePanelRetailElem(const hudelem_t *elem)
{
    if (elem->alignScreen != SCORE_PANEL_SCREEN_TOP_LEFT || elem->alignOrg != 0)
        return SCORE_PANEL_ELEM_NONE;
    switch (elem->type) {
    case HE_TYPE_TIMER_DOWN:
    case HE_TYPE_TIMER_UP:
    case HE_TYPE_TENTHS_TIMER_DOWN:
    case HE_TYPE_TENTHS_TIMER_UP:
        return elem->x == 8.0f && elem->y == 2.0f ? SCORE_PANEL_ELEM_CLOCK : SCORE_PANEL_ELEM_NONE;
    case HE_TYPE_MATERIAL:
        return elem->x == 6.0f && (elem->y == 28.0f || elem->y == 50.0f) ? SCORE_PANEL_ELEM_ICON
                                                                          : SCORE_PANEL_ELEM_NONE;
    case HE_TYPE_VALUE:
        if (elem->y != 26.0f && elem->y != 48.0f)
            return SCORE_PANEL_ELEM_NONE;
        if (elem->x == 36.0f || (elem->label && elem->x >= 49.0f && elem->x <= 104.0f))
            return SCORE_PANEL_ELEM_SCORE;
        return SCORE_PANEL_ELEM_NONE;
    default:
        return SCORE_PANEL_ELEM_NONE;
    }
}

static void CG_ScorePanelFindRetail(const playerState_t *ps, CG_ScorePanelRetail *retail)
{
    const hudelem_t *lists[2] = { ps->hud.current, ps->hud.archival };
    int count = (int)(sizeof(ps->hud.current) / sizeof(ps->hud.current[0]));
    int list;
    int i;

    memset(retail, 0, sizeof(*retail));
    for (list = 0; list < 2; ++list) {
        for (i = 0; i < count && lists[list][i].type != HE_TYPE_FREE; ++i) {
            const hudelem_t *elem = &lists[list][i];
            int kind = CG_ScorePanelRetailElem(elem);
            if (kind == SCORE_PANEL_ELEM_CLOCK && !retail->clock)
                retail->clock = elem;
            else if (kind == SCORE_PANEL_ELEM_ICON && retail->iconCount < SCORE_PANEL_MAX_ICONS)
                retail->icons[retail->iconCount++] = elem;
        }
    }
}

/* The icon of the viewer's side, in one color like the WWII emblems. The Axis
 * are always German, while the Allies' nation is known only from their icon.
 * A deathmatch player has one icon. */
static MaterialHandle CG_ScorePanelEmblem(const CG_ScorePanelRetail *retail, int team)
{
    char name[64];
    int i;

    if (team != TEAM_FREE && team != TEAM_AXIS && team != TEAM_ALLIES)
        return NULL;
    for (i = 0; i < retail->iconCount; ++i) {
        MaterialHandle icon;
        MaterialHandle emblem;
        int german;
        if (!CG_ServerMaterialName(retail->icons[i]->materialIndex, name, sizeof(name)) ||
            strncmp(name, "hudicon_", 8))
            continue;
        german = !strcmp(name, "hudicon_german");
        if (team != TEAM_FREE && german != (team == TEAM_AXIS))
            continue;
        icon = CL_RegisterMaterial(name, 7);
        emblem = R_GetHudEmblemMaterial(icon);
        return emblem ? emblem : icon;
    }
    return NULL;
}

static int CG_ScorePanelStanding(int own, int other)
{
    if (own > other)
        return SCORE_PANEL_WINNING;
    if (own < other)
        return SCORE_PANEL_LOSING;
    return SCORE_PANEL_TIED;
}

/* A player is measured against the best other player; a spectator sees the
 * two best. A slot whose client left starts again from zero. */
static void CG_ScorePanelDmView(CG_ScorePanelState *state, int ownClient, CG_ScorePanelView *view)
{
    int first = 0;
    int second = 0;
    int count = 0;
    int i;

    for (i = 0; i < SCORE_PANEL_MAX_CLIENTS; ++i) {
        const clientInfo_t *info = &cg->bgs.clientinfo[i];
        int score;
        if (!info->infoValid) {
            state->dmScores[i] = 0;
            continue;
        }
        if (i == ownClient || info->team == TEAM_SPECTATOR)
            continue;
        score = state->dmScores[i];
        if (!count || score > first) {
            second = first;
            first = score;
        } else if (count == 1 || score > second) {
            second = score;
        }
        ++count;
    }
    if (ownClient >= 0) {
        view->topScore = state->dmScores[ownClient];
        view->bottomScore = first;
        view->standing = CG_ScorePanelStanding(view->topScore, first);
    } else {
        view->topScore = first;
        view->bottomScore = second;
        view->standing = SCORE_PANEL_NEUTRAL;
    }
}

/* teamScores holds the Axis first. Spectators see the Allies on top. */
static void CG_ScorePanelGetView(CG_ScorePanelState *state, int gametype, int viewClient, CG_ScorePanelView *view)
{
    const clientInfo_t *info = &cg->bgs.clientinfo[viewClient];

    view->team = info->infoValid ? info->team : TEAM_SPECTATOR;
    view->limit = atoi(Info_ValueForKey(CL_GetConfigString(0),
                                        va("ui_%s_scorelimit", s_scorePanelGametypes[gametype])));
    if (gametype == SCORE_PANEL_DM) {
        CG_ScorePanelDmView(state, view->team == TEAM_FREE ? viewClient : -1, view);
    } else if (view->team == TEAM_AXIS || view->team == TEAM_ALLIES) {
        int own = view->team == TEAM_AXIS ? 0 : 1;
        view->topScore = cgs->teamScores[own];
        view->bottomScore = cgs->teamScores[1 - own];
        view->standing = CG_ScorePanelStanding(view->topScore, view->bottomScore);
    } else {
        view->topScore = cgs->teamScores[1];
        view->bottomScore = cgs->teamScores[0];
        view->standing = SCORE_PANEL_NEUTRAL;
    }
}

static void CG_ScorePanelUpdateStanding(CG_ScorePanelState *state, int standing, int now)
{
    if (standing == state->standing)
        return;
    state->standing = standing;
    state->standingTime = now;
    state->showStanding = standing != SCORE_PANEL_NEUTRAL;
}

/* A new standing shows for a few seconds, then fades into the mode name. */
static const char *CG_ScorePanelLabel(const CG_ScorePanelState *state, int now, const char *mode, float *alpha)
{
    int elapsed = now - state->standingTime;

    *alpha = 1.0f;
    if (!state->showStanding || elapsed < 0)
        return mode;
    if (elapsed < SCORE_PANEL_STANDING_MSEC) {
        if (elapsed > SCORE_PANEL_STANDING_MSEC - SCORE_PANEL_LABEL_FADE_MSEC)
            *alpha = (float)(SCORE_PANEL_STANDING_MSEC - elapsed) / (float)SCORE_PANEL_LABEL_FADE_MSEC;
        return s_scorePanelStandings[state->standing];
    }
    if (elapsed < SCORE_PANEL_STANDING_MSEC + SCORE_PANEL_LABEL_FADE_MSEC)
        *alpha = (float)(elapsed - SCORE_PANEL_STANDING_MSEC) / (float)SCORE_PANEL_LABEL_FADE_MSEC;
    return mode;
}

/* A bar fills toward the score limit but always fits its number. */
static float CG_ScorePanelFill(int score, int limit, float minWidth)
{
    float width = 0.0f;

    if (limit > 0 && score > 0)
        width = SCORE_PANEL_BAR_WIDTH * ((float)score / (float)limit);
    if (width > SCORE_PANEL_BAR_WIDTH)
        width = SCORE_PANEL_BAR_WIDTH;
    return width < minWidth ? minWidth : width;
}

/* A font whose capitals are cap units tall at scale. */
static FontHandle CG_ScorePanelFont(float cap, float *scale)
{
    FontHandle font = UI_GetFontHandle(SCORE_PANEL_FONT, cap / 30.0f);

    *scale = CG_ScoreTableCapScale(font, cap);
    return font;
}

/* Text from x on its baseline: the renderer draws half a unit up and left of
 * its pen, and the pen stands under the baseline. */
static void CG_ScorePanelDrawText(const char *text, FontHandle font, float x, float baseline, float scale,
                                  const vec_t *color)
{
    float unit = CL_NormalizedTextScale(font, scale);
    float cap;
    float rise;

    CG_ScoreTableFontMetrics(font, &cap, &rise);
    UI_DrawText(text, 0x7fffffff, font, x + 0.5f * unit, baseline + (rise + 0.5f) * unit, SCORE_PANEL_ALIGN_LEFT,
                SCORE_PANEL_ALIGN_BOTTOM, scale, color, 3);
}

static void CG_ScorePanelDrawBar(float y, float height, float cap, int score, int limit, const vec_t *fillColor)
{
    float scale;
    FontHandle font = CG_ScorePanelFont(cap, &scale);
    const char *number = va("%i", score);
    float fill = CG_ScorePanelFill(score, limit,
                                   CG_ScoreTableTextWidth(number, 0, font, scale) + 2.0f * SCORE_PANEL_NUMBER_PAD);

    CL_DrawStretchPic(SCORE_PANEL_BAR_X, y, fill, height, SCORE_PANEL_ALIGN_LEFT, SCORE_PANEL_ALIGN_BOTTOM, 0.0f,
                      0.0f, 1.0f, 1.0f, fillColor, cgs->media.whiteMaterial);
    if (fill < SCORE_PANEL_BAR_WIDTH)
        CL_DrawStretchPic(SCORE_PANEL_BAR_X + fill, y, SCORE_PANEL_BAR_WIDTH - fill, height, SCORE_PANEL_ALIGN_LEFT,
                          SCORE_PANEL_ALIGN_BOTTOM, 0.0f, 0.0f, 1.0f, 1.0f, s_scorePanelTrack,
                          cgs->media.whiteMaterial);
    CG_ScorePanelDrawText(number, font, SCORE_PANEL_BAR_X + SCORE_PANEL_NUMBER_PAD, y + 0.5f * (height + cap), scale,
                          s_scorePanelText);
}

/* Points at the viewer's own bar. */
static void CG_ScorePanelDrawArrow(void)
{
    static const unsigned short indices[3] = { 0, 1, 2 };
    HudVertex2D verts[3];
    float x = SCORE_PANEL_ARROW_X;
    float y = SCORE_PANEL_TOP_Y + 0.5f * (SCORE_PANEL_TOP_HEIGHT - SCORE_PANEL_ARROW_HEIGHT);
    float w = SCORE_PANEL_ARROW_WIDTH;
    float h = SCORE_PANEL_ARROW_HEIGHT;
    int i;

    CalcScreenPlacement(&x, &y, &w, &h, SCORE_PANEL_ALIGN_LEFT, SCORE_PANEL_ALIGN_BOTTOM);
    memset(verts, 0, sizeof(verts));
    verts[0].xy[0] = x;
    verts[0].xy[1] = y;
    verts[1].xy[0] = x;
    verts[1].xy[1] = y + h;
    verts[2].xy[0] = x + w;
    verts[2].xy[1] = y + 0.5f * h;
    for (i = 0; i < 3; ++i)
        memcpy(verts[i].rgba, s_scorePanelText, sizeof(verts[i].rgba));
    R_AddCmdDrawTriangles2D(verts, 3, indices, 3, cgs->media.whiteMaterial);
}

static void CG_ScorePanelDraw(const CG_ScorePanelState *state, const CG_ScorePanelRetail *retail,
                              const CG_ScorePanelView *view, int now)
{
    MaterialHandle emblem = CG_ScorePanelEmblem(retail, view->team);
    float clockLeft = SCORE_PANEL_CLOCK_RIGHT;
    float labelCap = SCORE_PANEL_LABEL_CAP;
    float labelWidth;
    float room;
    float scale;
    FontHandle font;
    const char *label;
    vec4_t color;
    float alpha;

    if (emblem) {
        const vec4_t faint = { 1.0f, 1.0f, 1.0f, SCORE_PANEL_EMBLEM_ALPHA };
        CL_DrawStretchPic(SCORE_PANEL_EMBLEM_X, SCORE_PANEL_EMBLEM_Y, SCORE_PANEL_EMBLEM_SIZE,
                          SCORE_PANEL_EMBLEM_SIZE, SCORE_PANEL_ALIGN_LEFT, SCORE_PANEL_ALIGN_BOTTOM, 0.0f, 0.0f, 1.0f,
                          1.0f, faint, emblem);
    }

    /* The stock clock keeps its own color, gold during the SD grace period. */
    if (retail->clock) {
        const char *clock = CG_GetHudElemTimer(retail->clock, color);
        if (clock && color[3] > 0.0f) {
            font = CG_ScorePanelFont(SCORE_PANEL_CLOCK_CAP, &scale);
            clockLeft -= CG_ScoreTableTextWidth(clock, 0, font, scale);
            CG_ScorePanelDrawText(clock, font, clockLeft, SCORE_PANEL_TEXT_BASELINE, scale, color);
        }
    }

    /* A long mode name shrinks to stay clear of the clock. */
    label = CG_ScorePanelLabel(state, now,
                               SEH_LocalizeTextMessage(UI_GetGameTypeDisplayName(cgs->gametype),
                                                       "score panel gametype", 0),
                               &alpha);
    font = CG_ScorePanelFont(labelCap, &scale);
    labelWidth = CG_ScoreTableTextWidth(label, 0, font, scale);
    room = clockLeft - SCORE_PANEL_LABEL_GAP - SCORE_PANEL_LABEL_X;
    if (labelWidth > room && room > 0.0f) {
        labelCap *= room / labelWidth;
        font = CG_ScorePanelFont(labelCap, &scale);
    }
    memcpy(color, s_scorePanelText, sizeof(color));
    color[3] = alpha;
    CG_ScorePanelDrawText(label, font, SCORE_PANEL_LABEL_X, SCORE_PANEL_TEXT_BASELINE, scale, color);

    CG_ScorePanelDrawBar(SCORE_PANEL_TOP_Y, SCORE_PANEL_TOP_HEIGHT, SCORE_PANEL_TOP_CAP, view->topScore,
                         view->limit, s_scorePanelTopFill);
    CG_ScorePanelDrawBar(SCORE_PANEL_BOTTOM_Y, SCORE_PANEL_BOTTOM_HEIGHT, SCORE_PANEL_BOTTOM_CAP,
                         view->bottomScore, view->limit, s_scorePanelBottomFill);
    if (view->standing != SCORE_PANEL_NEUTRAL)
        CG_ScorePanelDrawArrow();
}

void CG_ScorePanelInit(void)
{
    cg_hudScorePanel = Dvar_RegisterBool_mac("cg_hudScorePanel", 1, 0x1001);
    memset(&s_scorePanel, 0, sizeof(s_scorePanel));
    s_scorePanel.standing = SCORE_PANEL_UNSEEN;
}

void CG_ScorePanelClearDmScores(void)
{
    memset(s_scorePanel.dmScores, 0, sizeof(s_scorePanel.dmScores));
}

void CG_ScorePanelSetDmScore(int clientNum, int score)
{
    if ((unsigned)clientNum < SCORE_PANEL_MAX_CLIENTS)
        s_scorePanel.dmScores[clientNum] = score;
}

int CG_ScorePanelHidesHudElem(const hudelem_t *elem)
{
    return CG_ScorePanelActiveGametype() >= 0 && CG_ScorePanelRetailElem(elem) != SCORE_PANEL_ELEM_NONE;
}

int CG_ScorePanelMatch(int *topScore, int *bottomScore, int *limit, int *standing)
{
    CG_ScorePanelView view;
    int gametype = CG_ScorePanelGametype(cgs->gametype);
    int viewClient;

    if (gametype < 0 || !cg->snap)
        return 0;
    viewClient = cg->snap->ps.clientNum;
    if ((unsigned)viewClient >= SCORE_PANEL_MAX_CLIENTS)
        return 0;
    CG_ScorePanelGetView(&s_scorePanel, gametype, viewClient, &view);
    *topScore = view.topScore;
    *bottomScore = view.bottomScore;
    *limit = view.limit;
    *standing = view.standing == SCORE_PANEL_NEUTRAL ? CG_SCORE_PANEL_NEUTRAL
                : view.standing == SCORE_PANEL_WINNING ? 1
                : view.standing == SCORE_PANEL_LOSING ? -1
                                                       : 0;
    return 1;
}

const char *CG_ScorePanelClock(vec4_t color)
{
    CG_ScorePanelRetail retail;

    if (!cg->snap)
        return NULL;
    CG_ScorePanelFindRetail(&cg->snap->ps, &retail);
    return retail.clock ? CG_GetHudElemTimer(retail.clock, color) : NULL;
}

int CG_ScorePanelClockEnd(void)
{
    CG_ScorePanelRetail retail;

    if (!cg->snap)
        return 0;
    CG_ScorePanelFindRetail(&cg->snap->ps, &retail);
    if (!retail.clock || (retail.clock->type != HE_TYPE_TIMER_DOWN && retail.clock->type != HE_TYPE_TENTHS_TIMER_DOWN))
        return 0;
    return retail.clock->time;
}

void CG_DrawScorePanel(void)
{
    CG_ScorePanelRetail retail;
    CG_ScorePanelView view;
    int gametype = CG_ScorePanelActiveGametype();
    int viewClient;

    if (gametype < 0 || !cg->snap)
        return;
    viewClient = cg->snap->ps.clientNum;
    if ((unsigned)viewClient >= SCORE_PANEL_MAX_CLIENTS)
        return;
    CG_ScorePanelFindRetail(&cg->snap->ps, &retail);
    CG_ScorePanelGetView(&s_scorePanel, gametype, viewClient, &view);
    CG_ScorePanelUpdateStanding(&s_scorePanel, view.standing, cg->time);
    CG_ScorePanelDraw(&s_scorePanel, &retail, &view, cg->time);
}
