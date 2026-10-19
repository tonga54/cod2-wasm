#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_prematch.h"
#include "headers/PC/cgame_mp/cg_scoretable.h"
#include "headers/PC/gfx_d3d/r_font_wwii.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"

#include <math.h>
#include <stdio.h>
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
extern int I_stricmp(const char *s1, const char *s2);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);

/* Virtual units measured on the WWII capture (1.2 pixels a unit): the mode
 * and map over its objective in the top-left corner of the safe area, the
 * viewer's team in the bottom-left one and the countdown at the right.
 * Heights given for text are of its capitals. */
#define PREMATCH_ALIGN_LEFT 1
#define PREMATCH_ALIGN_TOP 1
#define PREMATCH_ALIGN_RIGHT 3
#define PREMATCH_ALIGN_BOTTOM 3
#define PREMATCH_TEXT_X 46.5f
#define PREMATCH_TITLE_BASELINE 59.0f
#define PREMATCH_TITLE_CAP 14.4f
#define PREMATCH_OBJECTIVE_BASELINE 75.9f
#define PREMATCH_OBJECTIVE_CAP 7.3f
#define PREMATCH_TEXT_ROOM 540.0f
/* The team, up from the bottom: its emblem, a teal band behind its name that
 * fades out to the right, and the name. */
#define PREMATCH_EMBLEM_X 63.3f
#define PREMATCH_EMBLEM_Y -64.6f
#define PREMATCH_EMBLEM_SIZE 25.5f
#define PREMATCH_BAND_LEFT 73.3f
#define PREMATCH_BAND_SOLID 104.0f
#define PREMATCH_BAND_RIGHT 140.0f
#define PREMATCH_BAND_HEIGHT 27.5f
#define PREMATCH_BAND_ALPHA 0.65f
#define PREMATCH_TEAM_X 89.6f
#define PREMATCH_TEAM_BASELINE -62.9f
#define PREMATCH_TEAM_CAP 7.3f
/* The countdown from the top-right corner: faint lines crossing at its middle,
 * an inner ring that runs out with each second and an outer one with the whole
 * countdown, both from twelve o'clock, round the seconds left. A ring's radius
 * is to its middle. */
#define PREMATCH_RING_X -91.25f
#define PREMATCH_RING_Y 193.3f
#define PREMATCH_OUTER_RADIUS 29.2f
#define PREMATCH_INNER_RADIUS 23.75f
#define PREMATCH_RING_WIDTH 2.4f
#define PREMATCH_TICK_WIDTH 1.2f
#define PREMATCH_TICK_ALPHA 0.45f
#define PREMATCH_DISC_ALPHA 0.3f
#define PREMATCH_LINE_REACH 66.0f
#define PREMATCH_LINE_WIDTH 0.85f
#define PREMATCH_LINE_ALPHA 0.3f
#define PREMATCH_DIGIT_CAP 17.5f
#define PREMATCH_SHADOW_OFFSET 0.8f
#define PREMATCH_SHADOW_ALPHA 0.5f
#define PREMATCH_TEXT_STYLE 3
/* Milliseconds before the match: the last five seconds are red over a red
 * disc, the lines draw in from then until two seconds before, and the color
 * comes back over the last two. */
#define PREMATCH_RED_MSEC 5000
#define PREMATCH_LINES_GONE_MSEC 2000
#define PREMATCH_GRAY_FADE_MSEC 2000
/* A longer countdown is not one the server sends. */
#define PREMATCH_MAX_MSEC 120000
#define PREMATCH_ARC_SEGMENTS 128
#define PREMATCH_DISC_SEGMENTS 64
#define PREMATCH_EMBLEM_MIN_TEXELS 8
#define PREMATCH_EMBLEM_MAX_TEXELS 128
#define PREMATCH_EMBLEM_UNSEEN -2
#define PREMATCH_PLAYING_LINKED 1
#define PREMATCH_FOLLOWING 0x400000
#define PREMATCH_TWO_PI 6.28318531f

typedef struct {
    const char *gametype;
    const char *objective;
} CG_PrematchMode;

typedef struct {
    int start;
    int end;
    FontHandle boldFont;
    FontHandle regularFont;
    int emblemKind;
    int emblemSize;
    MaterialHandle emblem;
} CG_PrematchState;

/* The stock modes' one-line objectives, which the retail game shows on a split
 * screen. */
static const CG_PrematchMode s_prematchModes[] = {
    { "dm", "MP_ELIMINATE_ENEMIES" },
    { "tdm", "MP_ELIMINATE_THE_ENEMY" },
    { "ctf", "MP_CAPTURE_THE_ENEMY_FLAG" },
    { "hq", "MP_ESTABLISH_AND_DEFEND" },
};
static const vec3_t s_prematchCream = { 0.89f, 0.87f, 0.8f };
static const vec3_t s_prematchLight = { 0.93f, 0.92f, 0.88f };
static const vec3_t s_prematchRed = { 0.91f, 0.29f, 0.31f };
static const vec3_t s_prematchTeal = { 0.27f, 0.6f, 0.56f };
static const vec3_t s_prematchLine = { 0.9f, 0.9f, 0.9f };
static const vec3_t s_prematchBlack = { 0.0f, 0.0f, 0.0f };

static CG_PrematchState s_prematch;
static const dvar_t *cg_hudPrematch;
static HudVertex2D s_prematchVerts[(PREMATCH_ARC_SEGMENTS + 1) * 4];
static unsigned short s_prematchIndices[PREMATCH_ARC_SEGMENTS * 18];
float g_webGrayFrac = 0.0f;

static void CG_PrematchColor(vec4_t out, const vec_t *rgb, float alpha)
{
    out[0] = rgb[0];
    out[1] = rgb[1];
    out[2] = rgb[2];
    out[3] = alpha;
}

static float CG_PrematchPixelsPerUnit(void)
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 1.0f;
    float h = 1.0f;

    CalcScreenPlacement(&x, &y, &w, &h, PREMATCH_ALIGN_RIGHT, PREMATCH_ALIGN_TOP);
    return h;
}

static void CG_PrematchPlace(float x, float y, int horzAlign, int vertAlign, float *px, float *py)
{
    float w = 0.0f;
    float h = 0.0f;

    CalcScreenPlacement(&x, &y, &w, &h, horzAlign, vertAlign);
    *px = x;
    *py = y;
}

static void CG_PrematchVertex(HudVertex2D *vertex, float x, float y, const vec_t *color)
{
    memset(vertex, 0, sizeof(*vertex));
    vertex->xy[0] = x;
    vertex->xy[1] = y;
    memcpy(vertex->rgba, color, sizeof(vertex->rgba));
}

void CG_PrematchInit(void)
{
    cg_hudPrematch = Dvar_RegisterBool_mac("cg_hudPrematch", 1, 0x1001);
    memset(&s_prematch, 0, sizeof(s_prematch));
    s_prematch.emblemKind = PREMATCH_EMBLEM_UNSEEN;
    s_prematch.boldFont = CL_RegisterFont("$boldfont", 0);
    s_prematch.regularFont = R_RegisterWwiiFont("$regularfont");
    g_webGrayFrac = 0.0f;
}

void CG_PrematchSetCountdown(const char *value)
{
    int start = 0;
    int end = 0;

    if (!value || sscanf(value, "%d %d", &start, &end) != 2 || end <= start || end - start > PREMATCH_MAX_MSEC)
        start = end = 0;
    s_prematch.start = start;
    s_prematch.end = end;
}

/* Milliseconds until the match begins, 0 without a countdown. */
static int CG_PrematchRemaining(void)
{
    int length = s_prematch.end - s_prematch.start;
    int remaining = s_prematch.end - cg->time;

    if (!s_prematch.end || remaining <= 0)
        return 0;
    return remaining < length ? remaining : length;
}

int CG_PrematchActive(void)
{
    const snapshot_t *snap = cg->nextSnap;

    if (!cg_hudPrematch || !cg_hudPrematch->current.enabled || !snap)
        return 0;
    if (snap->ps.pm_type > PREMATCH_PLAYING_LINKED || (snap->ps.pm_flags & PREMATCH_FOLLOWING))
        return 0;
    return CG_PrematchRemaining() > 0;
}

void CG_PrematchFrame(void)
{
    int remaining = CG_PrematchActive() ? CG_PrematchRemaining() : 0;

    if (remaining >= PREMATCH_GRAY_FADE_MSEC)
        g_webGrayFrac = 1.0f;
    else
        g_webGrayFrac = (float)remaining / (float)PREMATCH_GRAY_FADE_MSEC;
}

/* Text whose capitals stand cap tall on the baseline, from x or centered on
 * it and shrunk to fit room; the renderer draws half a unit up and to the left
 * of its pen. */
static void CG_PrematchText(const char *text, FontHandle font, float cap, float x, float baseline, float room,
                            int centered, int horzAlign, int vertAlign, const vec_t *color)
{
    float scale;
    float width;
    float unit;
    float units;
    float rise;

    if (!font || !text || !text[0])
        return;
    scale = CG_ScoreTableCapScale(font, cap);
    width = CG_ScoreTableTextWidth(text, 0, font, scale);
    if (room > 0.0f && width > room) {
        scale *= room / width;
        width = room;
    }
    if (centered)
        x -= 0.5f * width;
    unit = CL_NormalizedTextScale(font, scale);
    CG_ScoreTableFontMetrics(font, &units, &rise);
    UI_DrawText(text, 0x7fffffff, font, x + 0.5f * unit, baseline + (rise + 0.5f) * unit, horzAlign, vertAlign, scale,
                color, PREMATCH_TEXT_STYLE);
}

/* In S&D the objective text set at a spawn tells defenders from attackers. */
static const char *CG_PrematchObjective(void)
{
    const char *text = cg->objectiveText;
    int i;

    if (!I_stricmp(cgs->gametype, "sd")) {
        if (!strncmp(text, "MP_OBJ_DEFENDERS", 16) || !strcmp(text, "MP_DEFEND_THE_OBJECTIVE"))
            return "MP_DEFEND_THE_OBJECTIVE";
        return "MP_DESTROY_THE_OBJECTIVE";
    }
    for (i = 0; i < (int)(sizeof(s_prematchModes) / sizeof(s_prematchModes[0])); ++i) {
        if (!I_stricmp(cgs->gametype, s_prematchModes[i].gametype))
            return s_prematchModes[i].objective;
    }
    return NULL;
}

static void CG_PrematchDrawTitle(void)
{
    const char *objective = CG_PrematchObjective();
    char mode[64];
    char map[64];
    char title[160];
    vec4_t color;

    Com_sprintf(mode, sizeof(mode), "%s",
                SEH_LocalizeTextMessage(UI_GetGameTypeDisplayName(cgs->gametype), "prematch gametype", 0));
    CG_ScoreTableMapName(map, sizeof(map));
    Com_sprintf(title, sizeof(title), "%s // %s", mode, map);
    CG_PrematchColor(color, s_prematchCream, 1.0f);
    CG_PrematchText(title, s_prematch.boldFont, PREMATCH_TITLE_CAP, PREMATCH_TEXT_X, PREMATCH_TITLE_BASELINE,
                    PREMATCH_TEXT_ROOM, 0, PREMATCH_ALIGN_LEFT, PREMATCH_ALIGN_TOP, color);
    if (!objective)
        return;
    CG_PrematchColor(color, s_prematchLight, 1.0f);
    CG_PrematchText(SEH_LocalizeTextMessage(objective, "prematch objective", 0),
                    s_prematch.regularFont ? s_prematch.regularFont : s_prematch.boldFont, PREMATCH_OBJECTIVE_CAP,
                    PREMATCH_TEXT_X, PREMATCH_OBJECTIVE_BASELINE, PREMATCH_TEXT_ROOM, 0, PREMATCH_ALIGN_LEFT,
                    PREMATCH_ALIGN_TOP, color);
}

/* Built at the size it is drawn, a texel a pixel, again when the screen or
 * the nation changes; a failed build is not tried again until then. */
static void CG_PrematchPrepareEmblem(int kind)
{
    int size = (int)(PREMATCH_EMBLEM_SIZE * CG_PrematchPixelsPerUnit() + 2.5f);
    byte *bgra;

    if (size < PREMATCH_EMBLEM_MIN_TEXELS)
        size = PREMATCH_EMBLEM_MIN_TEXELS;
    else if (size > PREMATCH_EMBLEM_MAX_TEXELS)
        size = PREMATCH_EMBLEM_MAX_TEXELS;
    if (kind == s_prematch.emblemKind && size == s_prematch.emblemSize)
        return;
    s_prematch.emblemKind = kind;
    s_prematch.emblemSize = size;
    s_prematch.emblem = NULL;
    if (kind < 0)
        return;
    bgra = (byte *)malloc((size_t)size * (size_t)size * 4);
    if (!bgra)
        return;
    CG_ScoreTableRasterizeEmblem(kind, bgra, size * 4, size);
    s_prematch.emblem = R_CreateHudImageMaterial(R_HUD_IMAGE_PREMATCH, bgra, size, size);
    free(bgra);
}

static float CG_PrematchShadow(void)
{
    float offset = floorf(PREMATCH_SHADOW_OFFSET * CG_PrematchPixelsPerUnit() + 0.5f);

    return offset < 1.0f ? 1.0f : offset;
}

/* Left to right: solid, solid, clear. */
static void CG_PrematchDrawBand(void)
{
    static const unsigned short indices[12] = { 0, 1, 3, 0, 3, 2, 2, 3, 5, 2, 5, 4 };
    static const float columns[3] = { PREMATCH_BAND_LEFT, PREMATCH_BAND_SOLID, PREMATCH_BAND_RIGHT };
    HudVertex2D verts[6];
    vec4_t color;
    float x;
    float top;
    float bottom;
    int i;

    for (i = 0; i < 3; ++i) {
        CG_PrematchColor(color, s_prematchTeal, i < 2 ? PREMATCH_BAND_ALPHA : 0.0f);
        CG_PrematchPlace(columns[i], PREMATCH_EMBLEM_Y - 0.5f * PREMATCH_BAND_HEIGHT, PREMATCH_ALIGN_LEFT,
                         PREMATCH_ALIGN_BOTTOM, &x, &top);
        CG_PrematchPlace(columns[i], PREMATCH_EMBLEM_Y + 0.5f * PREMATCH_BAND_HEIGHT, PREMATCH_ALIGN_LEFT,
                         PREMATCH_ALIGN_BOTTOM, &x, &bottom);
        CG_PrematchVertex(&verts[2 * i], x, top, color);
        CG_PrematchVertex(&verts[2 * i + 1], x, bottom, color);
    }
    R_AddCmdDrawTriangles2D(verts, 6, indices, 12, cgs->media.whiteMaterial);
}

/* A deathmatch player and a spectator have no team to show. */
static void CG_PrematchDrawTeam(void)
{
    const clientInfo_t *info;
    vec4_t color;
    float x;
    float y;
    float size;
    float shadow;

    if ((unsigned)cg->clientNum >= 64 || CG_ScoreTableFreeForAll())
        return;
    info = &cg->bgs.clientinfo[cg->clientNum];
    if (!info->infoValid || (info->team != TEAM_ALLIES && info->team != TEAM_AXIS))
        return;
    CG_PrematchDrawBand();
    CG_PrematchPrepareEmblem(CG_ScoreTableEmblemKind(info->team));
    if (s_prematch.emblem) {
        size = (float)s_prematch.emblemSize;
        shadow = CG_PrematchShadow();
        CG_PrematchPlace(PREMATCH_EMBLEM_X, PREMATCH_EMBLEM_Y, PREMATCH_ALIGN_LEFT, PREMATCH_ALIGN_BOTTOM, &x, &y);
        x = floorf(x - 0.5f * size + 0.5f);
        y = floorf(y - 0.5f * size + 0.5f);
        CG_PrematchColor(color, s_prematchBlack, PREMATCH_SHADOW_ALPHA);
        CL_DrawStretchPicPhysical(x + shadow, y + shadow, size, size, 0.0f, 0.0f, 1.0f, 1.0f, color,
                                  s_prematch.emblem);
        CG_PrematchColor(color, s_prematchCream, 1.0f);
        CL_DrawStretchPicPhysical(x, y, size, size, 0.0f, 0.0f, 1.0f, 1.0f, color, s_prematch.emblem);
    }
    CG_PrematchColor(color, s_prematchLight, 1.0f);
    CG_PrematchText(info->team == TEAM_AXIS ? "Axis Team" : "Allies Team", s_prematch.boldFont, PREMATCH_TEAM_CAP,
                    PREMATCH_TEAM_X, PREMATCH_TEAM_BASELINE, 0.0f, 0, PREMATCH_ALIGN_LEFT, PREMATCH_ALIGN_BOTTOM,
                    color);
}

/* An arc width pixels across from twelve o'clock, counterclockwise through
 * sweep of a turn, its sides fading out over a pixel. */
static void CG_PrematchArc(float cx, float cy, float radius, float width, float sweep, const vec_t *color)
{
    float half = width > 1.0f ? 0.5f * width : 0.5f;
    float radii[4] = { radius - half - 0.5f, radius - half + 0.5f, radius + half - 0.5f, radius + half + 0.5f };
    HudVertex2D *verts = s_prematchVerts;
    unsigned short *indices = s_prematchIndices;
    vec4_t core;
    vec4_t clear;
    int segments;
    int count = 0;
    int i;
    int k;

    if (sweep <= 0.0f || color[3] <= 0.0f)
        return;
    if (sweep > 1.0f)
        sweep = 1.0f;
    segments = (int)ceilf(sweep * PREMATCH_ARC_SEGMENTS);
    CG_PrematchColor(core, color, color[3] * (width < 1.0f ? width : 1.0f));
    CG_PrematchColor(clear, color, 0.0f);
    for (i = 0; i <= segments; ++i) {
        float angle = PREMATCH_TWO_PI * sweep * (float)i / (float)segments;
        float dx = -sinf(angle);
        float dy = -cosf(angle);
        for (k = 0; k < 4; ++k)
            CG_PrematchVertex(&verts[i * 4 + k], cx + dx * radii[k], cy + dy * radii[k],
                              k == 1 || k == 2 ? core : clear);
    }
    for (i = 0; i < segments; ++i) {
        for (k = 0; k < 3; ++k) {
            unsigned short a = (unsigned short)(i * 4 + k);
            indices[count++] = a;
            indices[count++] = (unsigned short)(a + 1);
            indices[count++] = (unsigned short)(a + 5);
            indices[count++] = a;
            indices[count++] = (unsigned short)(a + 5);
            indices[count++] = (unsigned short)(a + 4);
        }
    }
    R_AddCmdDrawTriangles2D(verts, (segments + 1) * 4, indices, count, cgs->media.whiteMaterial);
}

static void CG_PrematchShadowedArc(float cx, float cy, float radius, float width, float sweep, const vec_t *color)
{
    float shadow = CG_PrematchShadow();
    vec4_t dark;

    CG_PrematchColor(dark, s_prematchBlack, PREMATCH_SHADOW_ALPHA * color[3]);
    CG_PrematchArc(cx + shadow, cy + shadow, radius, width, sweep, dark);
    CG_PrematchArc(cx, cy, radius, width, sweep, color);
}

/* A disc whose edge fades out over a pixel. */
static void CG_PrematchDisc(float cx, float cy, float radius, const vec_t *color)
{
    HudVertex2D *verts = s_prematchVerts;
    unsigned short *indices = s_prematchIndices;
    vec4_t clear;
    int count = 0;
    int i;

    CG_PrematchColor(clear, color, 0.0f);
    CG_PrematchVertex(&verts[0], cx, cy, color);
    for (i = 0; i < PREMATCH_DISC_SEGMENTS; ++i) {
        float angle = PREMATCH_TWO_PI * (float)i / (float)PREMATCH_DISC_SEGMENTS;
        float dx = -sinf(angle);
        float dy = -cosf(angle);
        int next = (i + 1) % PREMATCH_DISC_SEGMENTS;
        CG_PrematchVertex(&verts[1 + 2 * i], cx + dx * (radius - 0.5f), cy + dy * (radius - 0.5f), color);
        CG_PrematchVertex(&verts[2 + 2 * i], cx + dx * (radius + 0.5f), cy + dy * (radius + 0.5f), clear);
        indices[count++] = 0;
        indices[count++] = (unsigned short)(1 + 2 * i);
        indices[count++] = (unsigned short)(1 + 2 * next);
        indices[count++] = (unsigned short)(1 + 2 * i);
        indices[count++] = (unsigned short)(2 + 2 * i);
        indices[count++] = (unsigned short)(2 + 2 * next);
        indices[count++] = (unsigned short)(1 + 2 * i);
        indices[count++] = (unsigned short)(2 + 2 * next);
        indices[count++] = (unsigned short)(1 + 2 * next);
    }
    R_AddCmdDrawTriangles2D(verts, 1 + 2 * PREMATCH_DISC_SEGMENTS, indices, count, cgs->media.whiteMaterial);
}

/* Whole pixels across, reaching out either way of the block where they cross,
 * which the horizontal one fills; a line under a pixel wide is fainter. */
static void CG_PrematchLines(float cx, float cy, float reach, const vec_t *color)
{
    float exact = PREMATCH_LINE_WIDTH * CG_PrematchPixelsPerUnit();
    float pixels = floorf(exact + 0.5f);
    float x;
    float y;
    vec4_t ink;

    if (pixels < 1.0f)
        pixels = 1.0f;
    reach = floorf(reach - 0.5f * pixels + 0.5f);
    if (reach < 1.0f)
        return;
    CG_PrematchColor(ink, color, color[3] * (exact < pixels ? exact / pixels : 1.0f));
    x = floorf(cx - 0.5f * pixels + 0.5f);
    y = floorf(cy - 0.5f * pixels + 0.5f);
    CL_DrawStretchPicPhysical(x - reach, y, 2.0f * reach + pixels, pixels, 0.0f, 0.0f, 1.0f, 1.0f, ink,
                              cgs->media.whiteMaterial);
    CL_DrawStretchPicPhysical(x, y - reach, pixels, reach, 0.0f, 0.0f, 1.0f, 1.0f, ink, cgs->media.whiteMaterial);
    CL_DrawStretchPicPhysical(x, y + pixels, pixels, reach, 0.0f, 0.0f, 1.0f, 1.0f, ink, cgs->media.whiteMaterial);
}

static void CG_PrematchDrawCountdown(int remaining)
{
    float ppu = CG_PrematchPixelsPerUnit();
    int length = s_prematch.end - s_prematch.start;
    int seconds = (remaining + 999) / 1000;
    int red = remaining <= PREMATCH_RED_MSEC;
    float second = (float)(remaining - (seconds - 1) * 1000) / 1000.0f;
    float lines = (float)(remaining - PREMATCH_LINES_GONE_MSEC) / (float)(PREMATCH_RED_MSEC - PREMATCH_LINES_GONE_MSEC);
    char digits[16];
    vec4_t color;
    float cx;
    float cy;

    CG_PrematchPlace(PREMATCH_RING_X, PREMATCH_RING_Y, PREMATCH_ALIGN_RIGHT, PREMATCH_ALIGN_TOP, &cx, &cy);
    if (lines > 1.0f)
        lines = 1.0f;
    if (lines > 0.0f) {
        CG_PrematchColor(color, s_prematchLine, PREMATCH_LINE_ALPHA);
        CG_PrematchLines(cx, cy, PREMATCH_LINE_REACH * lines * ppu, color);
    }
    if (red) {
        CG_PrematchColor(color, s_prematchRed, PREMATCH_DISC_ALPHA);
        CG_PrematchDisc(cx, cy, PREMATCH_INNER_RADIUS * ppu, color);
        CG_PrematchColor(color, s_prematchRed, 1.0f);
        CG_PrematchShadowedArc(cx, cy, PREMATCH_INNER_RADIUS * ppu, PREMATCH_RING_WIDTH * ppu, second, color);
    } else {
        CG_PrematchColor(color, s_prematchCream, PREMATCH_TICK_ALPHA);
        CG_PrematchArc(cx, cy, PREMATCH_INNER_RADIUS * ppu, PREMATCH_TICK_WIDTH * ppu, second, color);
    }
    CG_PrematchColor(color, s_prematchCream, 1.0f);
    CG_PrematchShadowedArc(cx, cy, PREMATCH_OUTER_RADIUS * ppu, PREMATCH_RING_WIDTH * ppu,
                           (float)remaining / (float)length, color);
    Com_sprintf(digits, sizeof(digits), "%i", seconds);
    CG_PrematchColor(color, red ? s_prematchRed : s_prematchCream, 1.0f);
    CG_PrematchText(digits, s_prematch.boldFont, PREMATCH_DIGIT_CAP, PREMATCH_RING_X,
                    PREMATCH_RING_Y + 0.5f * PREMATCH_DIGIT_CAP, 0.0f, 1, PREMATCH_ALIGN_RIGHT, PREMATCH_ALIGN_TOP,
                    color);
}

void CG_DrawPrematch(void)
{
    int remaining;

    if (!CG_PrematchActive())
        return;
    remaining = CG_PrematchRemaining();
    CG_PrematchDrawTitle();
    CG_PrematchDrawTeam();
    CG_PrematchDrawCountdown(remaining);
}
