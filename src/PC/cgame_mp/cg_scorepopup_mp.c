#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_scorepopup.h"
#define CG_POINTS_CLIENT_ONLY
#include "headers/PC/game_mp/g_points.h"

extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern FontHandle UI_GetFontHandle(int fontEnum, float scale);
extern int UI_TextWidth(const char *text, int maxChars, FontHandle font, float scale);
extern void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style);
extern char *va(const char *fmt, ...);

/* Centre of the text, in virtual units from the screen centre: left of and a
 * little below the crosshair, where WWII prints it. */
#define SCORE_POPUP_X -128.0f
#define SCORE_POPUP_Y 70.0f
#define SCORE_POPUP_LABEL_SCALE 0.42f
#define SCORE_POPUP_POINTS_SCALE 0.34f
#define SCORE_POPUP_GAP 5.0f
#define SCORE_POPUP_MSEC 1600
#define SCORE_POPUP_FADE_IN_MSEC 60
#define SCORE_POPUP_SETTLE_MSEC 140
#define SCORE_POPUP_FADE_OUT_MSEC 350
#define SCORE_POPUP_START_SCALE 1.45f
#define SCORE_POPUP_ALIGN_CENTER 2

typedef struct {
    int active;
    int event;
    int total;
    int startTime;
} CG_ScorePopupState;

static const char *const s_scorePopupLabels[POINTS_EVENT_COUNT] = {
    "Kill",
    "Kill",
    "Headshot",
    "Assist",
    "Flag Captured",
    "Flag Returned",
    "Bomb Planted",
    "Bomb Defused"
};

static CG_ScorePopupState s_scorePopup;
static const dvar_t *cg_hudScorePopup;

/* Points that land while the popup is still up add to it and replay the pop
 * with the newest label. */
static void CG_ScorePopupPush(CG_ScorePopupState *state, int event, int points, int now)
{
    if (event < 0 || event >= POINTS_EVENT_COUNT || points <= 0)
        return;
    if (state->active && now >= state->startTime && now - state->startTime < SCORE_POPUP_MSEC)
        state->total += points;
    else
        state->total = points;
    state->active = 1;
    state->event = event;
    state->startTime = now;
}

static int CG_ScorePopupFrame(CG_ScorePopupState *state, int now, float *scale, float *alpha)
{
    int elapsed = now - state->startTime;
    float settle;

    if (!state->active || elapsed < 0 || elapsed >= SCORE_POPUP_MSEC) {
        state->active = 0;
        return 0;
    }
    if (elapsed < SCORE_POPUP_SETTLE_MSEC) {
        settle = 1.0f - (float)elapsed / (float)SCORE_POPUP_SETTLE_MSEC;
        *scale = 1.0f + (SCORE_POPUP_START_SCALE - 1.0f) * settle * settle;
    } else {
        *scale = 1.0f;
    }
    if (elapsed < SCORE_POPUP_FADE_IN_MSEC)
        *alpha = (float)elapsed / (float)SCORE_POPUP_FADE_IN_MSEC;
    else if (elapsed > SCORE_POPUP_MSEC - SCORE_POPUP_FADE_OUT_MSEC)
        *alpha = (float)(SCORE_POPUP_MSEC - elapsed) / (float)SCORE_POPUP_FADE_OUT_MSEC;
    else
        *alpha = 1.0f;
    return 1;
}

void CG_ScorePopupInit(void)
{
    cg_hudScorePopup = Dvar_RegisterBool_mac("cg_hudScorePopup", 1, 0x1001);
    s_scorePopup.active = 0;
}

void CG_ScorePopupAdd(int event, int points)
{
    CG_ScorePopupPush(&s_scorePopup, event, points, cg->time);
}

void CG_DrawScorePopup(void)
{
    vec4_t color = { 0.96f, 0.94f, 0.86f, 1.0f };
    FontHandle font;
    const char *label;
    const char *points;
    float labelScale;
    float pointsScale;
    float scale;
    float alpha;
    float labelWidth;
    float width;
    float x;

    if (!cg_hudScorePopup || !cg_hudScorePopup->current.enabled)
        return;
    if (!CG_ScorePopupFrame(&s_scorePopup, cg->time, &scale, &alpha))
        return;

    /* One face picked at the resting label size: the glyphs do not swap
     * mid-animation and the points read as a smaller cut of the label. */
    font = UI_GetFontHandle(0, SCORE_POPUP_LABEL_SCALE);
    labelScale = SCORE_POPUP_LABEL_SCALE * scale;
    pointsScale = SCORE_POPUP_POINTS_SCALE * scale;
    label = s_scorePopupLabels[s_scorePopup.event];
    points = va("+%i", s_scorePopup.total);
    labelWidth = (float)UI_TextWidth(label, 0, font, labelScale);
    width = labelWidth + SCORE_POPUP_GAP * scale + (float)UI_TextWidth(points, 0, font, pointsScale);
    x = SCORE_POPUP_X - width * 0.5f;
    color[3] = alpha;
    UI_DrawText(label, 0x7fffffff, font, x, SCORE_POPUP_Y, SCORE_POPUP_ALIGN_CENTER, SCORE_POPUP_ALIGN_CENTER,
                labelScale, color, 3);
    UI_DrawText(points, 0x7fffffff, font, x + labelWidth + SCORE_POPUP_GAP * scale, SCORE_POPUP_Y,
                SCORE_POPUP_ALIGN_CENTER, SCORE_POPUP_ALIGN_CENTER, pointsScale, color, 3);
}
