#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_scoretable.h"
#include "headers/PC/cgame_mp/cg_scorepanel.h"
#include "headers/PC/cgame_mp/cg_playernames.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern const char *Dvar_GetString(const char *dvarName);
extern FontHandle UI_GetFontHandle(int fontEnum, float scale);
extern FontHandle CL_RegisterFont(const char *fontName, int imageTrack);
extern int CL_TextWidth(const char *text, int maxChars, FontHandle font);
extern float CL_NormalizedTextScale(FontHandle font, float scale);
extern void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style);
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material);
extern void CL_DrawStretchPicPhysical(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material);
extern struct Material *CL_RegisterMaterialNoMip(const char *name, int imageTrack);
extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);
extern const Glyph *R_GetCharacterGlyph(FontHandle font, unsigned int letter);
extern const char *UI_GetGameTypeDisplayName(const char *gameType);
extern const char *UI_GetMapDisplayName(const char *mapName);
extern const char *FS_GetMapBaseName(const char *mapfile);
extern const char *SEH_LocalizeTextMessage(const char *msg, const char *context, int errType);
extern Bool CL_IsPlayerMuted(int clientIndex);
extern Bool CL_IsPlayerTalking(int clientIndex);
extern int I_stricmp(const char *s1, const char *s2);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern const char *va(const char *format, ...);

/* Virtual units of the 4:3 area, measured on the WWII capture (1.2 pixels a
 * unit): the title and the clock, the column names, then a block of nine
 * striped rows a team with its name, score and emblem on the left. Heights
 * given for text are of its capitals, and text starts and ends where its pen
 * does. */
#define TABLE_ALIGN 0
#define TABLE_FULLSCREEN 4
#define TABLE_TITLE_X 9.5f
#define TABLE_TITLE_BASELINE 59.7f
#define TABLE_TITLE_CAP 15.05f
#define TABLE_TITLE_RIGHT 560.0f
#define TABLE_CLOCK_RIGHT 621.0f
#define TABLE_CLOCK_BASELINE 69.3f
#define TABLE_CLOCK_CAP 13.65f
#define TABLE_HEADER_BASELINE 86.7f
#define TABLE_HEADER_CAP 8.85f
#define TABLE_LEFT 108.0f
#define TABLE_RIGHT 630.0f
#define TABLE_TOP 90.8f
#define TABLE_ROW_HEIGHT 17.7f
#define TABLE_TEAM_ROWS 9
#define TABLE_FREE_ROWS 18
#define TABLE_BLOCK_GAP 14.0f
/* More players than rows squeeze the rows down to this. */
#define TABLE_MIN_ROW_HEIGHT 9.0f
#define TABLE_ROW_CAP 7.3f
/* The text sits above the middle of its row. */
#define TABLE_ROW_BASELINE 0.612f
#define TABLE_NAME_X 159.5f
#define TABLE_TAG_RIGHT 349.6f
#define TABLE_NAME_GAP 6.0f
#define TABLE_VOICE_X 124.0f
#define TABLE_STATUS_X 140.0f
#define TABLE_ICON_SIZE 12.0f
#define TABLE_LABEL_X 15.8f
#define TABLE_LABEL_BASELINE 13.1f
#define TABLE_LABEL_CAP 13.55f
#define TABLE_TEAM_SCORE_BASELINE 37.4f
#define TABLE_TEAM_SCORE_CAP 21.7f
#define TABLE_EMBLEM_X 18.0f
#define TABLE_EMBLEM_Y 39.2f
#define TABLE_EMBLEM_SIZE 80.0f
#define TABLE_EMBLEM_ALPHA 0.55f
#define TABLE_SPECTATOR_BASELINE 446.0f
#define TABLE_SPECTATOR_CAP 7.0f
#define TABLE_OUTLINE 0.9f
#define TABLE_ACCENT_WIDTH 1.7f
#define TABLE_MARKER_WIDTH 2.5f
#define TABLE_MARKER_HEIGHT 5.5f
#define TABLE_PING_BAR_WIDTH 2.1f
#define TABLE_PING_BAR_GAP 1.0f
#define TABLE_PING_BOTTOM 0.8f
#define TABLE_STRIPE_LIGHT 0.45f
#define TABLE_STRIPE_DARK 0.25f
#define TABLE_CELL_SHADE 0.1f
#define TABLE_PING_LIT 0.32f
#define TABLE_PING_UNLIT 0.12f
#define TABLE_DIM 0.5f
#define TABLE_TEXT_STYLE 0
#define TABLE_TITLE_STYLE 3
#define TABLE_MAX_CLIENTS 64
#define TABLE_COLUMN_COUNT 4
/* The emblems are built at the size they are drawn, within these bounds. */
#define TABLE_EMBLEM_MIN_TEXELS 16
#define TABLE_EMBLEM_MAX_TEXELS 384
#define TABLE_CROSS_POINTS 12
#define TABLE_STAR_POINTS 10
#define TABLE_NAME_FONT_CAP 0.7f

enum {
    TABLE_EMBLEM_NONE = -1,
    TABLE_EMBLEM_AMERICAN,
    TABLE_EMBLEM_BRITISH,
    TABLE_EMBLEM_RUSSIAN,
    TABLE_EMBLEM_GERMAN,
    TABLE_EMBLEM_COUNT
};

typedef float CG_ScoreTablePoint[2];

typedef struct {
    int pixelHeight;
    float cap;
    float baseline;
} CG_ScoreTableFont;

typedef struct {
    int client;
    int score;
    int kills;
    int deaths;
    int ping;
    MaterialHandle statusIcon;
} CG_ScoreTableRow;

typedef struct {
    int team;
    int friendly;
    int score;
    int rowCount;
    CG_ScoreTableRow rows[TABLE_MAX_CLIENTS];
} CG_ScoreTableBlock;

typedef struct {
    int freeForAll;
    int blockCount;
    CG_ScoreTableBlock blocks[2];
    int spectatorCount;
    int spectators[TABLE_MAX_CLIENTS];
} CG_ScoreTableView;

typedef struct {
    int shapesReady;
    CG_ScoreTablePoint star[TABLE_STAR_POINTS];
    CG_ScoreTablePoint redStar[3][TABLE_STAR_POINTS];
    CG_ScoreTablePoint cross[3][TABLE_CROSS_POINTS];
    int emblemSize;
    int emblemAllies;
    MaterialHandle emblems;
    FontHandle boldFont;
    int kills[TABLE_MAX_CLIENTS];
} CG_ScoreTableState;

/* The four fonts the UI picks by size, then the bold font. */
static const CG_ScoreTableFont s_tableFonts[] = {
    { 12, 7.0f, 2.0f }, { 16, 11.0f, 2.0f }, { 24, 13.12f, 5.0f }, { 30, 17.1f, 6.0f }, { 32, 21.0f, 5.0f },
};
/* Score, Kills, Deaths, then the ping bars; the first and third are shaded. */
static const float s_tableColumns[TABLE_COLUMN_COUNT + 1] = { 354.0f, 439.0f, 524.0f, 610.0f, 630.0f };
/* The capture centers the numbers and their columns' names right of the
 * columns' middles. */
static const float s_tableCenters[TABLE_COLUMN_COUNT - 1] = { 398.6f, 482.5f, 567.2f };
static const char *const s_tableHeaders[TABLE_COLUMN_COUNT - 1] = { "Score", "Kills", "Deaths" };
static const float s_tablePingBars[3] = { 2.45f, 4.5f, 6.55f };
static const char *const s_tableNations[TABLE_EMBLEM_COUNT] = { "american", "british", "russian", "german" };
static const vec4_t s_tableText = { 0.82f, 0.79f, 0.74f, 1.0f };
static const vec3_t s_tableFriendly = { 0.145f, 0.42f, 0.38f };
static const vec3_t s_tableEnemy = { 0.67f, 0.185f, 0.16f };
static const vec4_t s_tableAccent = { 0.22f, 0.48f, 0.2f, 1.0f };
static const vec3_t s_tableBlack = { 0.0f, 0.0f, 0.0f };
/* An iron cross pattee: arm ends half the width wide, arms meeting at 0.35. */
static const CG_ScoreTablePoint s_tableCross[TABLE_CROSS_POINTS] = {
    { -0.5f, -1.0f }, { 0.5f, -1.0f }, { 0.35f, -0.35f }, { 1.0f, -0.5f }, { 1.0f, 0.5f }, { 0.35f, 0.35f },
    { 0.5f, 1.0f }, { -0.5f, 1.0f }, { -0.35f, 0.35f }, { -1.0f, 0.5f }, { -1.0f, -0.5f }, { -0.35f, -0.35f },
};

static CG_ScoreTableState s_table;
static CG_ScoreTableView s_tableView;
static const dvar_t *cg_hudScoreboard;

static void CG_ScoreTableColor(vec4_t out, const vec_t *rgb, float alpha)
{
    out[0] = rgb[0];
    out[1] = rgb[1];
    out[2] = rgb[2];
    out[3] = alpha;
}

/* Signed distance to a closed polygon, negative inside. */
static float CG_ScoreTablePolygonDistance(const CG_ScoreTablePoint *v, int count, float x, float y)
{
    float best = (x - v[0][0]) * (x - v[0][0]) + (y - v[0][1]) * (y - v[0][1]);
    float sign = 1.0f;
    int i;
    int j;

    for (i = 0, j = count - 1; i < count; j = i++) {
        float ex = v[j][0] - v[i][0];
        float ey = v[j][1] - v[i][1];
        float wx = x - v[i][0];
        float wy = y - v[i][1];
        float t = (wx * ex + wy * ey) / (ex * ex + ey * ey);
        float bx;
        float by;
        int above;
        int below;
        int left;

        if (t < 0.0f)
            t = 0.0f;
        else if (t > 1.0f)
            t = 1.0f;
        bx = wx - ex * t;
        by = wy - ey * t;
        if (bx * bx + by * by < best)
            best = bx * bx + by * by;
        above = y >= v[i][1];
        below = y < v[j][1];
        left = ex * wy > ey * wx;
        if ((above && below && left) || (!above && !below && !left))
            sign = -sign;
    }
    return sign * sqrtf(best);
}

/* Moves every edge inward by depth; the corners stay sharp. */
static void CG_ScoreTableInset(const CG_ScoreTablePoint *in, int count, float depth, CG_ScoreTablePoint *out)
{
    float area = 0.0f;
    float side;
    int i;

    for (i = 0; i < count; ++i) {
        const float *a = in[i];
        const float *b = in[(i + 1) % count];
        area += a[0] * b[1] - b[0] * a[1];
    }
    side = area > 0.0f ? 1.0f : -1.0f;
    for (i = 0; i < count; ++i) {
        const float *prev = in[(i + count - 1) % count];
        const float *point = in[i];
        const float *next = in[(i + 1) % count];
        float ax = -(point[1] - prev[1]) * side;
        float ay = (point[0] - prev[0]) * side;
        float bx = -(next[1] - point[1]) * side;
        float by = (next[0] - point[0]) * side;
        float la = sqrtf(ax * ax + ay * ay);
        float lb = sqrtf(bx * bx + by * by);
        float ca;
        float cb;
        float det;

        ax /= la;
        ay /= la;
        bx /= lb;
        by /= lb;
        ca = ax * prev[0] + ay * prev[1] + depth;
        cb = bx * point[0] + by * point[1] + depth;
        det = ax * by - ay * bx;
        if (fabsf(det) < 1e-6f) {
            out[i][0] = point[0] + ax * depth;
            out[i][1] = point[1] + ay * depth;
        } else {
            out[i][0] = (ca * by - ay * cb) / det;
            out[i][1] = (ax * cb - ca * bx) / det;
        }
    }
}

/* Five points, the first straight up. */
static void CG_ScoreTableStar(float outer, float inner, CG_ScoreTablePoint *out)
{
    int i;

    for (i = 0; i < TABLE_STAR_POINTS; ++i) {
        float angle = 3.14159265f * (-0.5f + 0.2f * (float)i);
        float radius = (i & 1) ? inner : outer;
        out[i][0] = radius * cosf(angle);
        out[i][1] = radius * sinf(angle);
    }
}

static void CG_ScoreTableBuildShapes(CG_ScoreTableState *state)
{
    if (state->shapesReady)
        return;
    CG_ScoreTableStar(0.98f, 0.375f, state->star);
    CG_ScoreTableStar(1.0f, 0.4f, state->redStar[0]);
    CG_ScoreTableInset(state->redStar[0], TABLE_STAR_POINTS, 0.06f, state->redStar[1]);
    CG_ScoreTableInset(state->redStar[0], TABLE_STAR_POINTS, 0.12f, state->redStar[2]);
    memcpy(state->cross[0], s_tableCross, sizeof(s_tableCross));
    CG_ScoreTableInset(state->cross[0], TABLE_CROSS_POINTS, 0.09f, state->cross[1]);
    CG_ScoreTableInset(state->cross[0], TABLE_CROSS_POINTS, 0.18f, state->cross[2]);
    state->shapesReady = 1;
}

/* A shape with a border: the outline's band, a gap, then the inset shape. */
static float CG_ScoreTableBorder(const CG_ScoreTablePoint *outer, const CG_ScoreTablePoint *band,
                                 const CG_ScoreTablePoint *inner, int count, float x, float y)
{
    float frame = CG_ScoreTablePolygonDistance(outer, count, x, y);
    float hole = -CG_ScoreTablePolygonDistance(band, count, x, y);
    float core = CG_ScoreTablePolygonDistance(inner, count, x, y);

    if (hole > frame)
        frame = hole;
    return core < frame ? core : frame;
}

/* The emblems in units of half their size, each traced from the WWII
 * scoreboard where it shows: the American star through a ring that breaks
 * around its points, and the iron cross inside its border. */
static float CG_ScoreTableEmblemDistance(const CG_ScoreTableState *state, int kind, float x, float y)
{
    float radius = sqrtf(x * x + y * y);
    float ring;
    float star;
    float cut;

    switch (kind) {
    case TABLE_EMBLEM_AMERICAN:
        star = CG_ScoreTablePolygonDistance(state->star, TABLE_STAR_POINTS, x, y);
        ring = radius - 1.0f > 0.84f - radius ? radius - 1.0f : 0.84f - radius;
        cut = -(star - 0.08f);
        if (cut > ring)
            ring = cut;
        return star < ring ? star : ring;
    case TABLE_EMBLEM_BRITISH:
        ring = radius - 1.0f > 0.78f - radius ? radius - 1.0f : 0.78f - radius;
        return radius - 0.5f < ring ? radius - 0.5f : ring;
    case TABLE_EMBLEM_RUSSIAN:
        return CG_ScoreTableBorder(state->redStar[0], state->redStar[1], state->redStar[2], TABLE_STAR_POINTS, x, y);
    case TABLE_EMBLEM_GERMAN:
        return CG_ScoreTableBorder(state->cross[0], state->cross[1], state->cross[2], TABLE_CROSS_POINTS, x, y);
    default:
        return 1.0f;
    }
}

/* White texels whose alpha covers the emblem, a texel of margin round it. */
static void CG_ScoreTableRasterize(const CG_ScoreTableState *state, int kind, byte *bgra, int stride, int size)
{
    float half = 0.5f * (float)size;
    float units = half - 1.0f;
    int x;
    int y;

    for (y = 0; y < size; ++y) {
        byte *texel = bgra + y * stride;
        for (x = 0; x < size; ++x, texel += 4) {
            float coverage = 0.0f;
            if (kind != TABLE_EMBLEM_NONE) {
                float u = ((float)x + 0.5f - half) / units;
                float v = ((float)y + 0.5f - half) / units;
                coverage = 0.5f - CG_ScoreTableEmblemDistance(state, kind, u, v) * units;
                if (coverage < 0.0f)
                    coverage = 0.0f;
                else if (coverage > 1.0f)
                    coverage = 1.0f;
            }
            texel[0] = texel[1] = texel[2] = 255;
            texel[3] = (byte)(coverage * 255.0f + 0.5f);
        }
    }
}

/* The Allies' nation from the stock scoreboard banner, "mpflag_british". */
static int CG_ScoreTableAlliesEmblem(void)
{
    const char *banner = Dvar_GetString("g_ScoresBanner_Allies");
    int kind;

    if (!banner || strncmp(banner, "mpflag_", 7))
        return TABLE_EMBLEM_NONE;
    for (kind = 0; kind < TABLE_EMBLEM_GERMAN; ++kind) {
        if (!I_stricmp(banner + 7, s_tableNations[kind]))
            return kind;
    }
    return TABLE_EMBLEM_NONE;
}

static float CG_ScoreTablePixelsPerUnit(void)
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 1.0f;
    float h = 1.0f;

    CalcScreenPlacement(&x, &y, &w, &h, TABLE_ALIGN, TABLE_ALIGN);
    return h;
}

/* One image holds the Allies' emblem on the left and the Axis cross. */
static void CG_ScoreTableBuildEmblems(CG_ScoreTableState *state, int size, int allies)
{
    byte *bgra;

    state->emblemSize = size;
    state->emblemAllies = allies;
    state->emblems = NULL;
    CG_ScoreTableBuildShapes(state);
    bgra = (byte *)malloc((size_t)size * (size_t)size * 8);
    if (!bgra)
        return;
    CG_ScoreTableRasterize(state, allies, bgra, size * 8, size);
    CG_ScoreTableRasterize(state, TABLE_EMBLEM_GERMAN, bgra + size * 4, size * 8, size);
    state->emblems = R_CreateHudImageMaterial(R_HUD_IMAGE_TEAM_EMBLEMS, bgra, 2 * size, size);
    free(bgra);
}

void CG_ScoreTablePrepare(void)
{
    int size;
    int allies;

    if (!CG_ScoreTableEnabled())
        return;
    size = (int)(TABLE_EMBLEM_SIZE * CG_ScoreTablePixelsPerUnit() + 0.5f);
    if (size < TABLE_EMBLEM_MIN_TEXELS)
        size = TABLE_EMBLEM_MIN_TEXELS;
    else if (size > TABLE_EMBLEM_MAX_TEXELS)
        size = TABLE_EMBLEM_MAX_TEXELS;
    allies = CG_ScoreTableAlliesEmblem();
    if (size != s_table.emblemSize || allies != s_table.emblemAllies)
        CG_ScoreTableBuildEmblems(&s_table, size, allies);
}

/* The capitals' height and the baseline's height above the pen, in font
 * units. A glyph's box also holds a margin and the baseline's offset, so they
 * come from the fonts: the name font's em is its pixel height, Barlow's
 * capitals 0.7 of it on the pen, and the WWII fonts keep the stock fonts'
 * metrics, told apart by pixel height. */
static void CG_ScoreTableFontMetrics(FontHandle font, float *cap, float *baseline)
{
    const Glyph *glyph;
    int i;

    *baseline = 0.0f;
    if (font == CG_GetPlayerNameFont()) {
        *cap = TABLE_NAME_FONT_CAP * (float)font->pixelHeight;
        return;
    }
    for (i = 0; i < (int)(sizeof(s_tableFonts) / sizeof(s_tableFonts[0])); ++i) {
        if (font->pixelHeight == s_tableFonts[i].pixelHeight) {
            *cap = s_tableFonts[i].cap;
            *baseline = s_tableFonts[i].baseline;
            return;
        }
    }
    glyph = R_GetCharacterGlyph(font, 'H');
    *cap = glyph && glyph->y0 < 0 ? (float)-glyph->y0 : 0.625f * (float)font->pixelHeight;
}

/* Scale for a font whose capitals stand cap units tall. */
static float CG_ScoreTableCapScale(FontHandle font, float cap)
{
    float units;
    float baseline;

    CG_ScoreTableFontMetrics(font, &units, &baseline);
    return cap * (float)font->pixelHeight / (48.0f * units);
}

static FontHandle CG_ScoreTableRowFont(float cap, float *scale)
{
    FontHandle font = CG_GetPlayerNameFont();

    if (!font)
        font = UI_GetFontHandle(0, cap / 30.0f);
    *scale = CG_ScoreTableCapScale(font, cap);
    return font;
}

/* The advances' sum, without the UI's rounding down to a whole unit. */
static float CG_ScoreTableTextWidth(const char *text, int maxChars, FontHandle font, float scale)
{
    return (float)CL_TextWidth(text, maxChars, font) * CL_NormalizedTextScale(font, scale);
}

/* The renderer draws text half a unit up and to the left of its pen. */
static void CG_ScoreTableDrawText(const char *text, int maxChars, FontHandle font, float x, float baseline, float scale,
                                  const vec_t *color, int style)
{
    float unit = CL_NormalizedTextScale(font, scale);
    float cap;
    float rise;

    CG_ScoreTableFontMetrics(font, &cap, &rise);
    UI_DrawText(text, maxChars, font, x + 0.5f * unit, baseline + (rise + 0.5f) * unit, TABLE_ALIGN, TABLE_ALIGN,
                scale, color, style);
}

/* How many letters of text fit in room; color codes do not count. */
static int CG_ScoreTableFit(const char *text, FontHandle font, float scale, float room)
{
    int count = 0;
    const char *c;

    for (c = text; *c; ++c) {
        if (c[0] == '^' && c[1] && c[1] != '^' && c[1] > '/' && c[1] <= '9') {
            ++c;
            continue;
        }
        ++count;
    }
    while (count > 1 && CG_ScoreTableTextWidth(text, count, font, scale) > room)
        --count;
    return count;
}

/* "[UY]Chapu54" puts its clan tag in a column of its own. A color still on
 * at the end of the tag starts the name. */
static const char *CG_ScoreTableSplitName(const char *full, char *tag, int tagSize, char *name, int nameSize)
{
    const char *c = full;
    const char *close;
    char color = 0;

    tag[0] = 0;
    while (c[0] == '^' && c[1] >= '0' && c[1] <= '9') {
        color = c[1];
        c += 2;
    }
    if (*c != '[' || !(close = strchr(c, ']')) || close - c > 10 || !close[1]) {
        I_strncpyz(name, full, nameSize);
        return name;
    }
    if ((int)(close + 1 - full) < tagSize) {
        const char *s;
        memcpy(tag, full, (size_t)(close + 1 - full));
        tag[close + 1 - full] = 0;
        for (s = c; s < close; ++s) {
            if (s[0] == '^' && s[1] >= '0' && s[1] <= '9')
                color = s[1];
        }
    }
    for (c = close + 1; *c == ' '; ++c)
        ;
    if (color)
        Com_sprintf(name, nameSize, "^%c%s", color, c);
    else
        I_strncpyz(name, c, nameSize);
    return name;
}

/* Rows a block draws and their height: nine at least, squeezed for more
 * players down to the smallest height. */
static int CG_ScoreTableRowLayout(int players, int slots, float *rowHeight)
{
    float height = (float)slots * TABLE_ROW_HEIGHT;
    int rows = players > slots ? players : slots;

    if (height / (float)rows < TABLE_MIN_ROW_HEIGHT)
        rows = (int)(height / TABLE_MIN_ROW_HEIGHT);
    *rowHeight = height / (float)rows;
    return rows;
}

static int CG_ScoreTableFreeForAll(void)
{
    if (!I_stricmp(cgs->gametype, "dm"))
        return 1;
    return cg->teamPlayers[TEAM_FREE] > 0 && !cg->teamPlayers[TEAM_AXIS] && !cg->teamPlayers[TEAM_ALLIES];
}

/* The viewer's team on top in teal; a spectator sees the Allies there. */
static void CG_ScoreTableBuildView(CG_ScoreTableView *view, int localClient)
{
    const clientInfo_t *local = &cg->bgs.clientinfo[localClient];
    int i;

    view->freeForAll = CG_ScoreTableFreeForAll();
    view->spectatorCount = 0;
    if (view->freeForAll) {
        view->blockCount = 1;
        view->blocks[0].team = TEAM_FREE;
        view->blocks[0].friendly = 0;
        view->blocks[0].score = 0;
    } else {
        int top = local->infoValid && local->team == TEAM_AXIS ? TEAM_AXIS : TEAM_ALLIES;
        view->blockCount = 2;
        view->blocks[0].team = top;
        view->blocks[1].team = top == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS;
        for (i = 0; i < 2; ++i) {
            view->blocks[i].friendly = !i;
            view->blocks[i].score = cg->teamScores[view->blocks[i].team];
        }
    }
    view->blocks[0].rowCount = 0;
    view->blocks[1].rowCount = 0;

    for (i = 0; i < cg->numScores && i < TABLE_MAX_CLIENTS; ++i) {
        const score_t *score = &cg->scores[i];
        CG_ScoreTableBlock *block = NULL;
        CG_ScoreTableRow *row;
        int b;

        if ((unsigned)score->client >= TABLE_MAX_CLIENTS || !cg->bgs.clientinfo[score->client].infoValid)
            continue;
        if (score->team == TEAM_SPECTATOR) {
            view->spectators[view->spectatorCount++] = score->client;
            continue;
        }
        for (b = 0; b < view->blockCount; ++b) {
            if (view->freeForAll || score->team == view->blocks[b].team)
                block = &view->blocks[b];
        }
        if (!block)
            continue;
        row = &block->rows[block->rowCount++];
        row->client = score->client;
        row->score = score->score;
        row->kills = s_table.kills[score->client];
        row->deaths = score->deaths;
        row->ping = score->ping;
        row->statusIcon = score->hStatusIcon;
    }
}

/* Physical pixels, rounded so that neighbouring rows share their edges. */
static void CG_ScoreTablePlace(float x, float y, float *px, float *py)
{
    float w = 0.0f;
    float h = 0.0f;

    CalcScreenPlacement(&x, &y, &w, &h, TABLE_ALIGN, TABLE_ALIGN);
    *px = floorf(x + 0.5f);
    *py = floorf(y + 0.5f);
}

static void CG_ScoreTableFillPixels(float x0, float y0, float x1, float y1, const vec_t *color)
{
    if (x1 > x0 && y1 > y0 && color[3] > 0.0f)
        CL_DrawStretchPicPhysical(x0, y0, x1 - x0, y1 - y0, 0.0f, 0.0f, 1.0f, 1.0f, color, cgs->media.whiteMaterial);
}

static void CG_ScoreTableFill(float x0, float y0, float x1, float y1, const vec_t *color)
{
    float px0;
    float py0;
    float px1;
    float py1;

    CG_ScoreTablePlace(x0, y0, &px0, &py0);
    CG_ScoreTablePlace(x1, y1, &px1, &py1);
    CG_ScoreTableFillPixels(px0, py0, px1, py1, color);
}

static void CG_ScoreTableDrawTriangle(const float (*xy)[2], const vec_t *color)
{
    static const unsigned short indices[3] = { 0, 1, 2 };
    HudVertex2D verts[3];
    int i;

    memset(verts, 0, sizeof(verts));
    for (i = 0; i < 3; ++i) {
        float w = 0.0f;
        float h = 0.0f;
        verts[i].xy[0] = xy[i][0];
        verts[i].xy[1] = xy[i][1];
        CalcScreenPlacement(&verts[i].xy[0], &verts[i].xy[1], &w, &h, TABLE_ALIGN, TABLE_ALIGN);
        memcpy(verts[i].rgba, color, sizeof(verts[i].rgba));
    }
    R_AddCmdDrawTriangles2D(verts, 3, indices, 3, cgs->media.whiteMaterial);
}

/* The viewer's row: a light outline, a green edge and an arrow outside it. */
static void CG_ScoreTableDrawOwnRow(float y0, float y1, float fade)
{
    float marker[3][2];
    float x0;
    float top;
    float x1;
    float bottom;
    float line = floorf(TABLE_OUTLINE * CG_ScoreTablePixelsPerUnit() + 0.5f);
    vec4_t color;

    if (line < 1.0f)
        line = 1.0f;
    CG_ScoreTablePlace(TABLE_LEFT, y0, &x0, &top);
    CG_ScoreTablePlace(TABLE_RIGHT, y1, &x1, &bottom);
    CG_ScoreTableColor(color, s_tableText, 0.9f * fade);
    CG_ScoreTableFillPixels(x0, top, x1, top + line, color);
    CG_ScoreTableFillPixels(x0, bottom - line, x1, bottom, color);
    CG_ScoreTableFillPixels(x0, top + line, x0 + line, bottom - line, color);
    CG_ScoreTableFillPixels(x1 - line, top + line, x1, bottom - line, color);
    CG_ScoreTableColor(color, s_tableAccent, fade);
    CG_ScoreTableFill(TABLE_LEFT, y0, TABLE_LEFT + TABLE_ACCENT_WIDTH, y1, color);

    marker[0][0] = TABLE_LEFT - TABLE_MARKER_WIDTH - 0.5f;
    marker[0][1] = 0.5f * (y0 + y1 - TABLE_MARKER_HEIGHT);
    marker[1][0] = marker[0][0];
    marker[1][1] = marker[0][1] + TABLE_MARKER_HEIGHT;
    marker[2][0] = TABLE_LEFT - 0.5f;
    marker[2][1] = 0.5f * (y0 + y1);
    CG_ScoreTableColor(color, s_tableText, fade);
    CG_ScoreTableDrawTriangle((const float (*)[2])marker, color);
}

/* Three bars: all of them under 100 ms, two under 200, then one. */
static int CG_ScoreTablePingBars(int ping)
{
    if (ping < 0)
        return 0;
    if (ping < 100)
        return 3;
    return ping < 200 ? 2 : 1;
}

static void CG_ScoreTableDrawPing(int ping, float baseline, float scale, float fade)
{
    int lit = CG_ScoreTablePingBars(ping);
    float width = 3.0f * TABLE_PING_BAR_WIDTH + 2.0f * TABLE_PING_BAR_GAP;
    float x = 0.5f * (s_tableColumns[3] + s_tableColumns[4] - width * scale);
    float bottom = baseline + TABLE_PING_BOTTOM * scale;
    vec4_t color;
    int i;

    for (i = 0; i < 3; ++i) {
        CG_ScoreTableColor(color, s_tableText, (i < lit ? TABLE_PING_LIT : TABLE_PING_UNLIT) * fade);
        CG_ScoreTableFill(x, bottom - s_tablePingBars[i] * scale, x + TABLE_PING_BAR_WIDTH * scale, bottom, color);
        x += (TABLE_PING_BAR_WIDTH + TABLE_PING_BAR_GAP) * scale;
    }
}

static void CG_ScoreTableDrawCentered(const char *text, FontHandle font, float scale, int column, float baseline,
                                      const vec_t *color)
{
    float x = s_tableCenters[column] - 0.5f * CG_ScoreTableTextWidth(text, 0, font, scale);

    CG_ScoreTableDrawText(text, 0x7fffffff, font, x, baseline, scale, color, TABLE_TEXT_STYLE);
}

static void CG_ScoreTableDrawRow(const CG_ScoreTableRow *row, float y0, float rowHeight, float fade)
{
    float fit = rowHeight < TABLE_ROW_HEIGHT ? rowHeight / TABLE_ROW_HEIGHT : 1.0f;
    float baseline = y0 + TABLE_ROW_BASELINE * rowHeight;
    float scale;
    FontHandle font = CG_ScoreTableRowFont(TABLE_ROW_CAP * fit, &scale);
    char tag[16];
    char name[48];
    float nameRight = TABLE_TAG_RIGHT;
    float icon = TABLE_ICON_SIZE * fit;
    MaterialHandle voice = NULL;
    vec4_t color;

    CG_ScoreTableColor(color, s_tableText, fade);
    CG_ScoreTableSplitName(cg->bgs.clientinfo[row->client].name, tag, sizeof(tag), name, sizeof(name));
    if (tag[0]) {
        float width = CG_ScoreTableTextWidth(tag, 0, font, scale);
        CG_ScoreTableDrawText(tag, 0x7fffffff, font, TABLE_TAG_RIGHT - width, baseline, scale, color,
                              TABLE_TEXT_STYLE);
        nameRight = TABLE_TAG_RIGHT - width - TABLE_NAME_GAP;
    }
    CG_ScoreTableDrawText(name, CG_ScoreTableFit(name, font, scale, nameRight - TABLE_NAME_X), font, TABLE_NAME_X,
                          baseline, scale, color, TABLE_TEXT_STYLE);

    if (CL_IsPlayerMuted(row->client))
        voice = CL_RegisterMaterialNoMip("voice_off", 7);
    else if (CL_IsPlayerTalking(row->client))
        voice = CL_RegisterMaterialNoMip("voice_on", 7);
    if (voice)
        CL_DrawStretchPic(TABLE_VOICE_X, y0 + 0.5f * (rowHeight - icon), icon, icon, TABLE_ALIGN, TABLE_ALIGN, 0.0f,
                          0.0f, 1.0f, 1.0f, color, voice);
    if (row->statusIcon)
        CL_DrawStretchPic(TABLE_STATUS_X, y0 + 0.5f * (rowHeight - icon), icon, icon, TABLE_ALIGN, TABLE_ALIGN, 0.0f,
                          0.0f, 1.0f, 1.0f, color, row->statusIcon);

    CG_ScoreTableDrawCentered(va("%i", row->score), font, scale, 0, baseline, color);
    if (row->kills >= 0)
        CG_ScoreTableDrawCentered(va("%i", row->kills), font, scale, 1, baseline, color);
    CG_ScoreTableDrawCentered(va("%i", row->deaths), font, scale, 2, baseline, color);
    CG_ScoreTableDrawPing(row->ping, baseline, fit, fade);
}

static void CG_ScoreTableDrawTeam(const CG_ScoreTableBlock *block, float top, float fade)
{
    FontHandle font = s_table.boldFont;
    const char *label = block->team == TEAM_AXIS ? "AXIS" : "ALLIES";
    const char *score = va("%i", block->score);
    float scale;
    vec4_t color;

    if (!font)
        return;
    CG_ScoreTableColor(color, s_tableText, fade);
    scale = CG_ScoreTableCapScale(font, TABLE_LABEL_CAP);
    CG_ScoreTableDrawText(label, 0x7fffffff, font, TABLE_LABEL_X, top + TABLE_LABEL_BASELINE, scale, color,
                          TABLE_TITLE_STYLE);
    scale = CG_ScoreTableCapScale(font, TABLE_TEAM_SCORE_CAP);
    CG_ScoreTableDrawText(score, 0x7fffffff, font, TABLE_LABEL_X, top + TABLE_TEAM_SCORE_BASELINE, scale, color,
                          TABLE_TITLE_STYLE);

    if (s_table.emblems && (block->team == TEAM_AXIS || s_table.emblemAllies != TABLE_EMBLEM_NONE)) {
        float s0 = block->team == TEAM_AXIS ? 0.5f : 0.0f;
        float x;
        float y;
        CG_ScoreTablePlace(TABLE_EMBLEM_X, top + TABLE_EMBLEM_Y, &x, &y);
        CG_ScoreTableColor(color, s_tableText, TABLE_EMBLEM_ALPHA * fade);
        CL_DrawStretchPicPhysical(x, y, (float)s_table.emblemSize, (float)s_table.emblemSize, s0, 0.0f, s0 + 0.5f,
                                  1.0f, color, s_table.emblems);
    }
}

/* The viewer always gets a row, in the last one if the block is full. */
static void CG_ScoreTableDrawBlock(const CG_ScoreTableBlock *block, int freeForAll, float top, int slots,
                                   int localClient, float fade)
{
    float rowHeight;
    int rows = CG_ScoreTableRowLayout(block->rowCount, slots, &rowHeight);
    int own = -1;
    int i;

    for (i = 0; i < block->rowCount; ++i) {
        if (block->rows[i].client == localClient)
            own = i;
    }
    if (!freeForAll)
        CG_ScoreTableDrawTeam(block, top, fade);

    for (i = 0; i < rows; ++i) {
        const CG_ScoreTableRow *row = NULL;
        float y0 = top + (float)i * rowHeight;
        float y1 = top + (float)(i + 1) * rowHeight;
        int teal = block->friendly;
        vec4_t color;

        if (i < block->rowCount)
            row = &block->rows[i == rows - 1 && own > i ? own : i];
        if (freeForAll)
            teal = row && row->client == localClient;
        CG_ScoreTableColor(color, teal ? s_tableFriendly : s_tableEnemy,
                           ((i & 1) ? TABLE_STRIPE_DARK : TABLE_STRIPE_LIGHT) * fade);
        CG_ScoreTableFill(TABLE_LEFT, y0, TABLE_RIGHT, y1, color);
        CG_ScoreTableColor(color, s_tableBlack, TABLE_CELL_SHADE * fade);
        CG_ScoreTableFill(s_tableColumns[0], y0, s_tableColumns[1], y1, color);
        CG_ScoreTableFill(s_tableColumns[2], y0, s_tableColumns[3], y1, color);
        if (!row)
            continue;
        CG_ScoreTableDrawRow(row, y0, rowHeight, fade);
        if (row->client == localClient)
            CG_ScoreTableDrawOwnRow(y0, y1, fade);
    }
}

static void CG_ScoreTableMapName(char *buf, int size)
{
    const char *base = FS_GetMapBaseName(cgs->mapname);
    const char *name = UI_GetMapDisplayName(base);
    char *comma;

    I_strncpyz(buf, name && name[0] ? name : base, size);
    /* "Toujane, Tunisia" names the place alone, like WWII's maps. */
    comma = strchr(buf, ',');
    if (comma && comma != buf)
        *comma = 0;
}

static void CG_ScoreTableDrawTitle(float fade)
{
    FontHandle font = s_table.boldFont;
    char mode[64];
    char map[64];
    char title[160];
    float scale;
    float width;
    vec4_t color;

    if (!font)
        return;
    I_strncpyz(mode, SEH_LocalizeTextMessage(UI_GetGameTypeDisplayName(cgs->gametype), "scoreboard gametype display",
                                             0),
               sizeof(mode));
    CG_ScoreTableMapName(map, sizeof(map));
    Com_sprintf(title, sizeof(title), "%s | %s", mode, map);
    scale = CG_ScoreTableCapScale(font, TABLE_TITLE_CAP);
    width = CG_ScoreTableTextWidth(title, 0, font, scale);
    if (width > TABLE_TITLE_RIGHT - TABLE_TITLE_X)
        scale *= (TABLE_TITLE_RIGHT - TABLE_TITLE_X) / width;
    CG_ScoreTableColor(color, s_tableText, fade);
    CG_ScoreTableDrawText(title, 0x7fffffff, font, TABLE_TITLE_X, TABLE_TITLE_BASELINE, scale, color,
                          TABLE_TITLE_STYLE);
}

/* The stock clock keeps a color of its own, like the SD grace period's. */
static void CG_ScoreTableDrawClock(float fade)
{
    FontHandle font = s_table.boldFont;
    vec4_t color;
    const char *clock;
    float scale;

    if (!font)
        return;
    clock = CG_ScorePanelClock(color);
    if (!clock || color[3] <= 0.0f)
        return;
    if (color[0] == 1.0f && color[1] == 1.0f && color[2] == 1.0f)
        CG_ScoreTableColor(color, s_tableText, color[3]);
    color[3] *= fade;
    scale = CG_ScoreTableCapScale(font, TABLE_CLOCK_CAP);
    CG_ScoreTableDrawText(clock, 0x7fffffff, font, TABLE_CLOCK_RIGHT - CG_ScoreTableTextWidth(clock, 0, font, scale),
                          TABLE_CLOCK_BASELINE, scale, color, TABLE_TITLE_STYLE);
}

static void CG_ScoreTableDrawHeaders(float fade)
{
    float cap = TABLE_HEADER_CAP;
    FontHandle font = UI_GetFontHandle(0, cap / 30.0f);
    float scale = CG_ScoreTableCapScale(font, cap);
    vec4_t color;
    int i;

    CG_ScoreTableColor(color, s_tableText, fade);
    for (i = 0; i < TABLE_COLUMN_COUNT - 1; ++i)
        CG_ScoreTableDrawCentered(s_tableHeaders[i], font, scale, i, TABLE_HEADER_BASELINE, color);
}

static void CG_ScoreTableDrawSpectators(const CG_ScoreTableView *view, float fade)
{
    char line[512];
    int length;
    float scale;
    FontHandle font;
    vec4_t color;
    int i;

    if (!view->spectatorCount)
        return;
    length = Com_sprintf(line, sizeof(line), "Spectators:");
    for (i = 0; i < view->spectatorCount && length < (int)sizeof(line) - 1; ++i) {
        length += Com_sprintf(line + length, sizeof(line) - length, "%s %s^7", i ? "," : "",
                              cg->bgs.clientinfo[view->spectators[i]].name);
    }
    font = CG_ScoreTableRowFont(TABLE_SPECTATOR_CAP, &scale);
    CG_ScoreTableColor(color, s_tableText, 0.8f * fade);
    CG_ScoreTableDrawText(line, CG_ScoreTableFit(line, font, scale, TABLE_RIGHT - TABLE_TITLE_X), font,
                          TABLE_TITLE_X, TABLE_SPECTATOR_BASELINE, scale, color, TABLE_TEXT_STYLE);
}

void CG_ScoreTableInit(void)
{
    int i;

    cg_hudScoreboard = Dvar_RegisterBool_mac("cg_hudScoreboard", 1, 0x1001);
    s_table.emblemSize = 0;
    s_table.emblemAllies = TABLE_EMBLEM_NONE;
    s_table.emblems = NULL;
    s_table.boldFont = CL_RegisterFont("$boldfont", 0);
    for (i = 0; i < TABLE_MAX_CLIENTS; ++i)
        s_table.kills[i] = -1;
}

int CG_ScoreTableEnabled(void)
{
    return cg_hudScoreboard && cg_hudScoreboard->current.enabled;
}

int CG_ScoreTableHidesHud(void)
{
    return CG_ScoreTableEnabled() && CG_ScoreboardDisplayed();
}

void CG_ScoreTableClearKills(void)
{
    int i;

    for (i = 0; i < TABLE_MAX_CLIENTS; ++i)
        s_table.kills[i] = -1;
}

void CG_ScoreTableSetKills(int clientNum, int kills)
{
    if ((unsigned)clientNum < TABLE_MAX_CLIENTS)
        s_table.kills[clientNum] = kills < 0 ? 0 : kills;
}

void CG_DrawScoreTable(float fade)
{
    CG_ScoreTableView *view = &s_tableView;
    int localClient = cg->clientNum;
    float x = 0.0f;
    float y = 0.0f;
    float w = 640.0f;
    float h = 480.0f;
    vec4_t color;
    int b;

    if ((unsigned)localClient >= TABLE_MAX_CLIENTS)
        return;
    CalcScreenPlacement(&x, &y, &w, &h, TABLE_FULLSCREEN, TABLE_FULLSCREEN);
    CG_ScoreTableColor(color, s_tableBlack, TABLE_DIM * fade);
    CG_ScoreTableFillPixels(x, y, x + w, y + h, color);

    CG_ScoreTableBuildView(view, localClient);
    CG_ScoreTableDrawTitle(fade);
    CG_ScoreTableDrawClock(fade);
    CG_ScoreTableDrawHeaders(fade);
    for (b = 0; b < view->blockCount; ++b) {
        int slots = view->freeForAll ? TABLE_FREE_ROWS : TABLE_TEAM_ROWS;
        float top = TABLE_TOP + (float)b * ((float)TABLE_TEAM_ROWS * TABLE_ROW_HEIGHT + TABLE_BLOCK_GAP);
        CG_ScoreTableDrawBlock(&view->blocks[b], view->freeForAll, top, slots, localClient, fade);
    }
    CG_ScoreTableDrawSpectators(view, fade);
}
