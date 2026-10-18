#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_headshoticon.h"
#include "headers/PC/cgame_mp/cg_killcam.h"
#include "headers/PC/cgame_mp/cg_playernames.h"
#include "headers/PC/cgame_mp/cg_scoretable.h"
#include "headers/PC/gfx_d3d/r_font_wwii.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern FontHandle UI_GetFontHandle(int fontEnum, float scale);
extern FontHandle CL_RegisterFont(const char *fontName, int imageTrack);
extern float CL_NormalizedTextScale(FontHandle font, float scale);
extern void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style);
extern void CL_DrawStretchPicPhysical(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material);
extern MaterialHandle CL_RegisterMaterial(const char *name, int imageTrack);
extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);
extern const char *CL_GetConfigString(int index);
extern int GetKeyBindings(const char *command, char (*bindings)[128]);
extern void Controls_GetConfig(void);
extern const char *SEH_StringEd_GetString(const char *);
extern const char *UI_SafeTranslateString(const char *reference);
extern WeaponDef *BG_GetWeaponDef(int weaponIndex);
extern int BG_GetNumWeapons(void);
extern int I_stricmp(const char *s1, const char *s2);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);

/* Virtual units measured on the WWII captures (1.2 pixels a unit, a 16:9
 * view 480 units tall): the left column from the screen's left edge, the
 * right one from its right edge, the weapon from its middle. Heights given
 * for text are of its capitals; x is where a text's pen starts, or the middle
 * of a centred one, its ink being Barlow's side bearing further in. */
#define KILLCAM_ALIGN_4X3 0
#define KILLCAM_ALIGN_LEFT 1
#define KILLCAM_ALIGN_CENTER 2
#define KILLCAM_ALIGN_RIGHT 3
#define KILLCAM_ALIGN_FULLSCREEN 4
#define KILLCAM_TOP_BAR 83.3f
#define KILLCAM_BOTTOM_BAR 394.2f
/* Black at the screen's edges, letting a little of the view through where
 * they meet it. */
#define KILLCAM_BAR_INNER_ALPHA 0.92f
#define KILLCAM_TITLE_X 53.65f
#define KILLCAM_TITLE_BASELINE 42.52f
#define KILLCAM_TITLE_CAP 14.43f
/* The key is a disc, or a pill as wide as a longer name needs. */
#define KILLCAM_KEY_X 53.5f
#define KILLCAM_KEY_Y 57.38f
#define KILLCAM_KEY_SIZE 14.0f
#define KILLCAM_KEY_CAP 6.5f
#define KILLCAM_KEY_PAD 3.5f
#define KILLCAM_PROMPT_GAP 2.44f
#define KILLCAM_PROMPT_BASELINE 59.92f
#define KILLCAM_PROMPT_CAP 6.85f
#define KILLCAM_ENEMY_X -108.0f
#define KILLCAM_YOU_X -55.4f
#define KILLCAM_VS_X -81.74f
#define KILLCAM_LABEL_BASELINE 37.38f
#define KILLCAM_LABEL_CAP 6.82f
#define KILLCAM_COUNT_BASELINE 64.73f
#define KILLCAM_COUNT_CAP 18.85f
#define KILLCAM_VS_BASELINE 56.53f
#define KILLCAM_VS_CAP 6.74f
#define KILLCAM_ATTACKERS_X -231.57f
#define KILLCAM_ATTACKERS_BASELINE 104.9f
#define KILLCAM_ATTACKERS_CAP 7.0f
#define KILLCAM_DOT_X -37.5f
#define KILLCAM_DOT_Y 116.6f
#define KILLCAM_DOT_SIZE 5.5f
#define KILLCAM_ROW_PITCH 15.2f
/* A name's baseline below its dot's centre. */
#define KILLCAM_NAME_DROP 1.0f
#define KILLCAM_NAME_CAP 6.1f
#define KILLCAM_NAME_ROOM 180.0f
#define KILLCAM_MAX_ROWS 6
/* The weapon's slot, beside the way the shot killed when that was a headshot
 * or a melee hit: its name above its stock icon, a long one turned up to the
 * right as on the capture. */
#define KILLCAM_SLOT_LABEL_BASELINE 414.9f
#define KILLCAM_SLOT_LABEL_CAP 6.7f
/* Low enough for the MP40's muzzle, the highest of the stock kill icons once
 * turned, to clear the labels. */
#define KILLCAM_SLOT_ICON_Y 445.0f
#define KILLCAM_SLOT_PITCH 84.0f
#define KILLCAM_WIDE_ICON_WIDTH 60.0f
#define KILLCAM_ICON_SIZE 28.0f
/* Radians; the stock Kar98k icon's ink, turned this far, lies at the
 * capture's 47.8 degrees. */
#define KILLCAM_ICON_TILT 0.7627f
#define KILLCAM_HEADSHOT_SIZE 24.0f
#define KILLCAM_SHADOW_STYLE 3
/* Deaths and attacker lists older than this are not the replay's. */
#define KILLCAM_MEMORY_MSEC 20000
#define KILLCAM_NAME_LEN 32
#define KILLCAM_KEY_LEN 32
#define KILLCAM_LABEL_LEN 64
#define KILLCAM_MOD_MELEE 7
#define KILLCAM_MOD_HEAD_SHOT 8
#define KILLCAM_LOCALIZED_STRINGS 0x51e
/* The stock script's alignments: fullscreen bars, then text centred in the
 * safe area at its top and bottom. */
#define KILLCAM_SCREEN_BARS 36
#define KILLCAM_SCREEN_TOP 57
#define KILLCAM_SCREEN_BOTTOM 59

enum {
    KILLCAM_STOCK_NONE,
    KILLCAM_STOCK_BAR,
    KILLCAM_STOCK_TITLE,
    KILLCAM_STOCK_RESPAWN,
    KILLCAM_STOCK_SKIP,
    KILLCAM_STOCK_TIMER
};

enum {
    KILLCAM_METHOD_NONE,
    KILLCAM_METHOD_HEADSHOT,
    KILLCAM_METHOD_MELEE
};

typedef struct {
    /* How often each player killed the viewer and the viewer them, kept
     * under the name the slot had. */
    char names[KILLCAM_MAX_CLIENTS][KILLCAM_NAME_LEN];
    int killedViewer[KILLCAM_MAX_CLIENTS];
    int killedByViewer[KILLCAM_MAX_CLIENTS];
    int deathKiller;
    int deathTime;
    int deathParm;
    int attackersKiller;
    int attackersTime;
    int attackers[KILLCAM_MAX_CLIENTS];
    int attackerCount;
    /* One image holds the key's disc, the attackers' dot and the headshot
     * symbol side by side, each a cell of that many texels a side, built at
     * the size it is drawn: HUD images have no mipmaps. */
    MaterialHandle discs;
    float discsPixelsPerUnit;
    int keyTexels;
    int dotTexels;
    int symbolTexels;
    int symbolRing;
    int discsWidth;
    int discsHeight;
    FontHandle boldFont;
    FontHandle regularFont;
} CG_KillcamState;

typedef struct {
    int killer;
    int killedViewer;
    int killedByViewer;
    /* The killer comes last. */
    int rows[KILLCAM_MAX_ROWS];
    int rowCount;
    int weapon;
    int method;
    int prompt;
} CG_KillcamView;

static const vec4_t s_killcamCream = { 0.875f, 0.863f, 0.82f, 1.0f };
static const vec4_t s_killcamName = { 0.84f, 0.82f, 0.78f, 1.0f };
static const vec4_t s_killcamPrompt = { 0.64f, 0.64f, 0.62f, 1.0f };
static const vec4_t s_killcamGrey = { 0.54f, 0.54f, 0.52f, 1.0f };
static const vec4_t s_killcamKey = { 0.88f, 0.88f, 0.86f, 1.0f };
static const vec4_t s_killcamKeyLetter = { 0.1f, 0.1f, 0.1f, 1.0f };
static const vec4_t s_killcamDot = { 0.62f, 0.0f, 0.05f, 1.0f };
static const vec4_t s_killcamIcon = { 1.0f, 1.0f, 1.0f, 1.0f };

static CG_KillcamState s_killcam;
static const dvar_t *cg_hudKillcam;

static float CG_KillcamPixelsPerUnit(void)
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 1.0f;
    float h = 1.0f;

    CalcScreenPlacement(&x, &y, &w, &h, KILLCAM_ALIGN_LEFT, KILLCAM_ALIGN_4X3);
    return h;
}

static void CG_KillcamRasterDisc(byte *bgra, int width, int left, int size, float diameter)
{
    float centre = 0.5f * (float)size;
    int x, y;

    for (y = 0; y < size; ++y) {
        for (x = 0; x < size; ++x) {
            byte *texel = bgra + (y * width + left + x) * 4;
            float dx = (float)x + 0.5f - centre;
            float dy = (float)y + 0.5f - centre;
            float coverage = 0.5f * diameter - sqrtf(dx * dx + dy * dy) + 0.5f;

            if (coverage < 0.0f)
                coverage = 0.0f;
            else if (coverage > 1.0f)
                coverage = 1.0f;
            texel[0] = texel[1] = texel[2] = 255;
            texel[3] = (byte)(coverage * 255.0f + 0.5f);
        }
    }
}

/* Cells have a clear texel around the shape. The key's is even, so a pill
 * splits it down the middle. */
static void CG_KillcamBuildDiscs(CG_KillcamState *state, float pixelsPerUnit)
{
    byte *bgra;
    byte *symbol;
    int y;

    state->discs = NULL;
    state->discsPixelsPerUnit = pixelsPerUnit;
    state->keyTexels = 2 * (int)ceilf(0.5f * KILLCAM_KEY_SIZE * pixelsPerUnit) + 2;
    state->dotTexels = (int)ceilf(KILLCAM_DOT_SIZE * pixelsPerUnit) + 2;
    state->symbolRing = (int)ceilf(0.5f * KILLCAM_HEADSHOT_SIZE * pixelsPerUnit);
    state->symbolTexels = 2 * (state->symbolRing + 1);
    state->discsWidth = state->keyTexels + state->dotTexels + state->symbolTexels;
    state->discsHeight = state->keyTexels > state->symbolTexels ? state->keyTexels : state->symbolTexels;
    bgra = (byte *)calloc((size_t)state->discsWidth * (size_t)state->discsHeight, 4);
    symbol = (byte *)malloc((size_t)state->symbolTexels * (size_t)state->symbolTexels * 4);
    if (bgra && symbol) {
        CG_KillcamRasterDisc(bgra, state->discsWidth, 0, state->keyTexels, KILLCAM_KEY_SIZE * pixelsPerUnit);
        CG_KillcamRasterDisc(bgra, state->discsWidth, state->keyTexels, state->dotTexels,
                             KILLCAM_DOT_SIZE * pixelsPerUnit);
        CG_HeadshotIconRaster(symbol, state->symbolTexels, state->symbolRing);
        for (y = 0; y < state->symbolTexels; ++y)
            memcpy(bgra + (y * state->discsWidth + state->keyTexels + state->dotTexels) * 4,
                   symbol + y * state->symbolTexels * 4, (size_t)state->symbolTexels * 4);
        state->discs = R_CreateHudImageMaterial(R_HUD_IMAGE_KILLCAM, bgra, state->discsWidth, state->discsHeight);
    }
    free(symbol);
    free(bgra);
}

/* A cell drawn texel for pixel, centred on x, y in pixels; s0 and s1 are the
 * part of it drawn, across, w pixels wide from x0. */
static void CG_KillcamDrawCellPart(const CG_KillcamState *state, int cell, int texels, float x0, float w, float s0,
                                   float s1, float centreY, const vec_t *color)
{
    float top = floorf(centreY - 0.5f * (float)texels + 0.5f);
    float width = (float)state->discsWidth;

    CL_DrawStretchPicPhysical(x0, top, w, (float)texels, ((float)cell + s0) / width, 0.0f,
                              ((float)cell + s1) / width, (float)texels / (float)state->discsHeight, color,
                              state->discs);
}

static void CG_KillcamDrawCell(const CG_KillcamState *state, int cell, int texels, float centreX, float centreY,
                               const vec_t *color)
{
    float left = floorf(centreX - 0.5f * (float)texels + 0.5f);

    CG_KillcamDrawCellPart(state, cell, texels, left, (float)texels, 0.0f, (float)texels, centreY, color);
}

/* The scoreboard's placement: the renderer draws text half a unit up and to
 * the left of its pen. */
static void CG_KillcamText(const char *text, FontHandle font, float cap, float x, float baseline, int horz,
                           const vec_t *color, int style)
{
    float scale = CG_ScoreTableCapScale(font, cap);
    float unit = CL_NormalizedTextScale(font, scale);
    float fontCap;
    float rise;

    CG_ScoreTableFontMetrics(font, &fontCap, &rise);
    UI_DrawText(text, 0x7fffffff, font, x + 0.5f * unit, baseline + (rise + 0.5f) * unit, horz, KILLCAM_ALIGN_4X3,
                scale, color, style);
}

static float CG_KillcamTextWidth(const char *text, FontHandle font, float cap)
{
    return CG_ScoreTableTextWidth(text, 0, font, CG_ScoreTableCapScale(font, cap));
}

static void CG_KillcamCentredText(const char *text, FontHandle font, float cap, float x, float baseline, int horz,
                                  const vec_t *color, int style)
{
    CG_KillcamText(text, font, cap, x - 0.5f * CG_KillcamTextWidth(text, font, cap), baseline, horz, color, style);
}

static FontHandle CG_KillcamLabelFont(float cap)
{
    return UI_GetFontHandle(0, cap / 30.0f);
}

static FontHandle CG_KillcamRegularFont(const CG_KillcamState *state, float cap)
{
    return state->regularFont ? state->regularFont : CG_KillcamLabelFont(cap);
}

static FontHandle CG_KillcamNameFont(float cap)
{
    FontHandle font = CG_GetPlayerNameFont();

    return font ? font : CG_KillcamLabelFont(cap);
}

static void CG_KillcamUpper(char *dest, const char *src, int size)
{
    int i;

    I_strncpyz(dest, src, size);
    for (i = 0; dest[i]; ++i) {
        if (dest[i] >= 'a' && dest[i] <= 'z')
            dest[i] = (char)(dest[i] - 'a' + 'A');
    }
}

/* Names are drawn in one color, without their color codes. */
static void CG_KillcamCleanName(char *dest, const char *src)
{
    int length = 0;

    while (*src && length < KILLCAM_NAME_LEN - 1) {
        if (src[0] == '^' && src[1] && src[1] != '^') {
            src += 2;
            continue;
        }
        dest[length++] = *src++;
    }
    dest[length] = '\0';
}

/* The key of +activate, which skips the replay or respawns, named as the
 * controls menu names it. */
static void CG_KillcamKey(char *key)
{
    char bindings[2][128];
    const char *name;

    Controls_GetConfig();
    if (!GetKeyBindings("+activate", bindings)) {
        I_strncpyz(key, "USE", KILLCAM_KEY_LEN);
        return;
    }
    name = SEH_StringEd_GetString(bindings[0]);
    CG_KillcamUpper(key, name ? name : bindings[0], KILLCAM_KEY_LEN);
}

static int CG_KillcamStockElem(const hudelem_t *elem)
{
    const char *text;

    switch (elem->type) {
    case HE_TYPE_MATERIAL:
        return elem->alignScreen == KILLCAM_SCREEN_BARS && elem->x == 0.0f &&
                       (elem->y == 0.0f || elem->y == 368.0f) && elem->width == 640 && elem->height == 112
                   ? KILLCAM_STOCK_BAR
                   : KILLCAM_STOCK_NONE;
    case HE_TYPE_TENTHS_TIMER_DOWN:
        return elem->alignScreen == KILLCAM_SCREEN_BOTTOM && elem->x == 0.0f && elem->y == -32.0f
                   ? KILLCAM_STOCK_TIMER
                   : KILLCAM_STOCK_NONE;
    case HE_TYPE_TEXT:
        if (elem->alignScreen != KILLCAM_SCREEN_TOP || elem->x != 0.0f || !elem->text)
            return KILLCAM_STOCK_NONE;
        text = CL_GetConfigString(elem->text + KILLCAM_LOCALIZED_STRINGS);
        if (!I_stricmp(text, "MP_KILLCAM"))
            return KILLCAM_STOCK_TITLE;
        if (!I_stricmp(text, "PLATFORM_PRESS_TO_RESPAWN"))
            return KILLCAM_STOCK_RESPAWN;
        if (!I_stricmp(text, "PLATFORM_PRESS_TO_SKIP"))
            return KILLCAM_STOCK_SKIP;
        return KILLCAM_STOCK_NONE;
    default:
        return KILLCAM_STOCK_NONE;
    }
}

/* The stock prompt says whether the key respawns or skips; without it there
 * is nothing to press. */
static int CG_KillcamPrompt(const playerState_t *ps)
{
    int count = (int)(sizeof(ps->hud.current) / sizeof(ps->hud.current[0]));
    int i;

    for (i = 0; i < count && ps->hud.current[i].type != HE_TYPE_FREE; ++i) {
        int kind = CG_KillcamStockElem(&ps->hud.current[i]);
        if (kind == KILLCAM_STOCK_RESPAWN || kind == KILLCAM_STOCK_SKIP)
            return kind;
    }
    return KILLCAM_STOCK_NONE;
}

static int CG_KillcamRecent(int time, int now)
{
    return now - time > -KILLCAM_MEMORY_MSEC && now - time < KILLCAM_MEMORY_MSEC;
}

static void CG_KillcamClaim(CG_KillcamState *state, int clientNum)
{
    const char *name = cg->bgs.clientinfo[clientNum].name;

    if (!strcmp(state->names[clientNum], name))
        return;
    I_strncpyz(state->names[clientNum], name, KILLCAM_NAME_LEN);
    state->killedViewer[clientNum] = 0;
    state->killedByViewer[clientNum] = 0;
}

static void CG_KillcamBuildView(const CG_KillcamState *state, int killer, const playerState_t *ps, int now,
                                CG_KillcamView *view)
{
    int i, j;

    memset(view, 0, sizeof(*view));
    view->killer = killer;
    if (!strcmp(state->names[killer], cg->bgs.clientinfo[killer].name)) {
        view->killedViewer = state->killedViewer[killer];
        view->killedByViewer = state->killedByViewer[killer];
    }
    if (state->attackersKiller == killer && CG_KillcamRecent(state->attackersTime, now)) {
        for (i = 0; i < state->attackerCount && view->rowCount < KILLCAM_MAX_ROWS - 1; ++i) {
            int other = state->attackers[i];
            if (other < 0 || other >= KILLCAM_MAX_CLIENTS || other == killer || !cg->bgs.clientinfo[other].infoValid)
                continue;
            for (j = 0; j < view->rowCount && view->rows[j] != other; ++j)
                ;
            if (j == view->rowCount)
                view->rows[view->rowCount++] = other;
        }
    }
    view->rows[view->rowCount++] = killer;
    view->weapon = ps->weapon;
    if (state->deathKiller == killer && CG_KillcamRecent(state->deathTime, now)) {
        if (!(state->deathParm & 0x80))
            view->weapon = state->deathParm;
        else if ((state->deathParm & 0x7f) == KILLCAM_MOD_HEAD_SHOT)
            view->method = KILLCAM_METHOD_HEADSHOT;
        else if ((state->deathParm & 0x7f) == KILLCAM_MOD_MELEE)
            view->method = KILLCAM_METHOD_MELEE;
    }
    view->prompt = CG_KillcamPrompt(ps);
}

static float CG_KillcamScreenY(float y)
{
    float x = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    CalcScreenPlacement(&x, &y, &w, &h, KILLCAM_ALIGN_LEFT, KILLCAM_ALIGN_4X3);
    return y;
}

/* Two quads, each corner listed clockwise from the top left. */
static void CG_KillcamDrawBars(void)
{
    static const unsigned short indices[12] = { 0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7 };
    HudVertex2D verts[8];
    float left = 0.0f;
    float top = 0.0f;
    float width = 640.0f;
    float height = 480.0f;
    float rows[8];
    float alphas[8];
    int i;

    CalcScreenPlacement(&left, &top, &width, &height, KILLCAM_ALIGN_FULLSCREEN, KILLCAM_ALIGN_FULLSCREEN);
    rows[0] = rows[1] = top;
    rows[2] = rows[3] = CG_KillcamScreenY(KILLCAM_TOP_BAR);
    rows[4] = rows[5] = CG_KillcamScreenY(KILLCAM_BOTTOM_BAR);
    rows[6] = rows[7] = top + height;
    memset(verts, 0, sizeof(verts));
    for (i = 0; i < 8; ++i) {
        alphas[i] = i >= 2 && i <= 5 ? KILLCAM_BAR_INNER_ALPHA : 1.0f;
        verts[i].xy[0] = (i & 3) == 1 || (i & 3) == 2 ? left + width : left;
        verts[i].xy[1] = rows[i];
        verts[i].rgba[3] = alphas[i];
    }
    R_AddCmdDrawTriangles2D(verts, 8, indices, 12, cgs->media.whiteMaterial);
}

/* The disc's halves at the ends of a pill width units wide, its middle
 * column stretched between them. */
static void CG_KillcamDrawKeyCap(const CG_KillcamState *state, float width)
{
    float x = KILLCAM_KEY_X;
    float y = KILLCAM_KEY_Y;
    float w = width;
    float h = KILLCAM_KEY_SIZE;
    float half = (float)(state->keyTexels / 2);
    float leftCentre, rightCentre;

    CalcScreenPlacement(&x, &y, &w, &h, KILLCAM_ALIGN_LEFT, KILLCAM_ALIGN_4X3);
    leftCentre = floorf(x + 0.5f * h + 0.5f);
    rightCentre = floorf(x + w - 0.5f * h + 0.5f);
    CG_KillcamDrawCellPart(state, 0, state->keyTexels, leftCentre - half, half, 0.0f, half, y, s_killcamKey);
    if (rightCentre > leftCentre)
        CG_KillcamDrawCellPart(state, 0, state->keyTexels, leftCentre, rightCentre - leftCentre, half, half, y,
                               s_killcamKey);
    CG_KillcamDrawCellPart(state, 0, state->keyTexels, rightCentre, half, half, 2.0f * half, y, s_killcamKey);
}

static void CG_KillcamDrawTitle(const CG_KillcamState *state, int prompt)
{
    char key[KILLCAM_KEY_LEN];
    float keyWidth;
    float textWidth;

    CG_KillcamText(UI_SafeTranslateString("MP_KILLCAM"), state->boldFont, KILLCAM_TITLE_CAP, KILLCAM_TITLE_X,
                   KILLCAM_TITLE_BASELINE, KILLCAM_ALIGN_LEFT, s_killcamCream, 0);
    if (prompt != KILLCAM_STOCK_RESPAWN && prompt != KILLCAM_STOCK_SKIP)
        return;
    CG_KillcamKey(key);
    textWidth = CG_KillcamTextWidth(key, state->boldFont, KILLCAM_KEY_CAP);
    keyWidth = textWidth + 2.0f * KILLCAM_KEY_PAD;
    if (keyWidth < KILLCAM_KEY_SIZE)
        keyWidth = KILLCAM_KEY_SIZE;
    if (state->discs)
        CG_KillcamDrawKeyCap(state, keyWidth);
    CG_KillcamText(key, state->boldFont, KILLCAM_KEY_CAP, KILLCAM_KEY_X + 0.5f * (keyWidth - textWidth),
                   KILLCAM_KEY_Y + 0.5f * KILLCAM_KEY_CAP, KILLCAM_ALIGN_LEFT, s_killcamKeyLetter, 0);
    CG_KillcamText(prompt == KILLCAM_STOCK_RESPAWN ? "Respawn" : "Skip",
                   CG_KillcamRegularFont(state, KILLCAM_PROMPT_CAP), KILLCAM_PROMPT_CAP,
                   KILLCAM_KEY_X + keyWidth + KILLCAM_PROMPT_GAP, KILLCAM_PROMPT_BASELINE, KILLCAM_ALIGN_LEFT,
                   s_killcamPrompt, 0);
}

static void CG_KillcamDrawDuel(const CG_KillcamState *state, const CG_KillcamView *view)
{
    FontHandle label = CG_KillcamLabelFont(KILLCAM_LABEL_CAP);
    FontHandle count = CG_KillcamRegularFont(state, KILLCAM_COUNT_CAP);
    char number[16];

    CG_KillcamCentredText("ENEMY", label, KILLCAM_LABEL_CAP, KILLCAM_ENEMY_X, KILLCAM_LABEL_BASELINE,
                          KILLCAM_ALIGN_RIGHT, s_killcamGrey, 0);
    CG_KillcamCentredText("YOU", label, KILLCAM_LABEL_CAP, KILLCAM_YOU_X, KILLCAM_LABEL_BASELINE, KILLCAM_ALIGN_RIGHT,
                          s_killcamGrey, 0);
    CG_KillcamCentredText("VS", CG_KillcamLabelFont(KILLCAM_VS_CAP), KILLCAM_VS_CAP, KILLCAM_VS_X,
                          KILLCAM_VS_BASELINE, KILLCAM_ALIGN_RIGHT, s_killcamGrey, 0);
    Com_sprintf(number, sizeof(number), "%i", view->killedViewer);
    CG_KillcamCentredText(number, count, KILLCAM_COUNT_CAP, KILLCAM_ENEMY_X, KILLCAM_COUNT_BASELINE,
                          KILLCAM_ALIGN_RIGHT, s_killcamCream, 0);
    Com_sprintf(number, sizeof(number), "%i", view->killedByViewer);
    CG_KillcamCentredText(number, count, KILLCAM_COUNT_CAP, KILLCAM_YOU_X, KILLCAM_COUNT_BASELINE,
                          KILLCAM_ALIGN_RIGHT, s_killcamCream, 0);
}

static void CG_KillcamDrawAttackers(const CG_KillcamState *state, const CG_KillcamView *view)
{
    FontHandle font = CG_KillcamNameFont(KILLCAM_NAME_CAP);
    float scale = CG_ScoreTableCapScale(font, KILLCAM_NAME_CAP);
    char name[KILLCAM_NAME_LEN];
    int i;

    CG_KillcamText("ATTACKERS", state->boldFont, KILLCAM_ATTACKERS_CAP, KILLCAM_ATTACKERS_X,
                   KILLCAM_ATTACKERS_BASELINE, KILLCAM_ALIGN_RIGHT, s_killcamCream, KILLCAM_SHADOW_STYLE);
    for (i = 0; i < view->rowCount; ++i) {
        float dotY = KILLCAM_DOT_Y + (float)i * KILLCAM_ROW_PITCH;

        CG_KillcamCleanName(name, cg->bgs.clientinfo[view->rows[i]].name);
        name[CG_ScoreTableFit(name, font, scale, KILLCAM_NAME_ROOM)] = '\0';
        CG_KillcamText(name, font, KILLCAM_NAME_CAP, KILLCAM_ATTACKERS_X, dotY + KILLCAM_NAME_DROP,
                       KILLCAM_ALIGN_RIGHT, s_killcamName, KILLCAM_SHADOW_STYLE);
        if (state->discs) {
            float x = KILLCAM_DOT_X;
            float y = dotY;
            float w = 0.0f;
            float h = 0.0f;

            CalcScreenPlacement(&x, &y, &w, &h, KILLCAM_ALIGN_RIGHT, KILLCAM_ALIGN_4X3);
            CG_KillcamDrawCell(state, state->keyTexels, state->dotTexels, x, y, s_killcamDot);
        }
    }
}

/* A w by h icon centred on x, y units from the middle of the screen, turned
 * up to the right when tilted, and mirrored when flipped. */
static void CG_KillcamDrawIcon(MaterialHandle icon, float x, float y, float w, float h, int tilted, int flip)
{
    static const unsigned short indices[6] = { 0, 1, 2, 0, 2, 3 };
    static const float corners[4][2] = { { -0.5f, -0.5f }, { 0.5f, -0.5f }, { 0.5f, 0.5f }, { -0.5f, 0.5f } };
    HudVertex2D verts[4];
    float c = tilted ? cosf(KILLCAM_ICON_TILT) : 1.0f;
    float s = tilted ? -sinf(KILLCAM_ICON_TILT) : 0.0f;
    int i;

    CalcScreenPlacement(&x, &y, &w, &h, KILLCAM_ALIGN_CENTER, KILLCAM_ALIGN_4X3);
    for (i = 0; i < 4; ++i) {
        float dx = corners[i][0] * w;
        float dy = corners[i][1] * h;

        verts[i].xy[0] = x + dx * c - dy * s;
        verts[i].xy[1] = y + dx * s + dy * c;
        verts[i].st[0] = (corners[i][0] > 0.0f) != (flip != 0) ? 1.0f : 0.0f;
        verts[i].st[1] = corners[i][1] > 0.0f ? 1.0f : 0.0f;
        memcpy(verts[i].rgba, s_killcamIcon, sizeof(verts[i].rgba));
    }
    R_AddCmdDrawTriangles2D(verts, 4, indices, 6, icon);
}

static void CG_KillcamDrawSlotLabel(const char *text, float x)
{
    CG_KillcamCentredText(text, CG_KillcamLabelFont(KILLCAM_SLOT_LABEL_CAP), KILLCAM_SLOT_LABEL_CAP, x,
                          KILLCAM_SLOT_LABEL_BASELINE, KILLCAM_ALIGN_CENTER, s_killcamGrey, 0);
}

static void CG_KillcamDrawWeapon(int weapon, float x)
{
    WeaponDef *weapDef;
    char label[KILLCAM_LABEL_LEN];

    if (weapon <= 0 || weapon > BG_GetNumWeapons())
        return;
    weapDef = BG_GetWeaponDef(weapon);
    if (!weapDef)
        return;
    if (weapDef->szDisplayName && weapDef->szDisplayName[0]) {
        CG_KillcamUpper(label, UI_SafeTranslateString(weapDef->szDisplayName), sizeof(label));
        CG_KillcamDrawSlotLabel(label, x);
    }
    if (!weapDef->killIcon || !weapDef->killIcon[0])
        return;
    if (weapDef->wideKillIcon)
        CG_KillcamDrawIcon(CL_RegisterMaterial(weapDef->killIcon, 7), x, KILLCAM_SLOT_ICON_Y, KILLCAM_WIDE_ICON_WIDTH,
                           0.5f * KILLCAM_WIDE_ICON_WIDTH, 1, weapDef->flipKillIcon);
    else
        CG_KillcamDrawIcon(CL_RegisterMaterial(weapDef->killIcon, 7), x, KILLCAM_SLOT_ICON_Y, KILLCAM_ICON_SIZE,
                           KILLCAM_ICON_SIZE, 0, weapDef->flipKillIcon);
}

static void CG_KillcamDrawMethod(const CG_KillcamState *state, int method, float x)
{
    if (method == KILLCAM_METHOD_HEADSHOT) {
        CG_KillcamDrawSlotLabel("Headshot", x);
        if (state->discs) {
            float y = KILLCAM_SLOT_ICON_Y;
            float w = 0.0f;
            float h = 0.0f;

            CalcScreenPlacement(&x, &y, &w, &h, KILLCAM_ALIGN_CENTER, KILLCAM_ALIGN_4X3);
            CG_KillcamDrawCell(state, state->keyTexels + state->dotTexels, state->symbolTexels, x, y,
                               s_killcamCream);
        }
    } else if (method == KILLCAM_METHOD_MELEE) {
        CG_KillcamDrawSlotLabel("Melee", x);
        CG_KillcamDrawIcon(CL_RegisterMaterial("killiconmelee", 7), x, KILLCAM_SLOT_ICON_Y, KILLCAM_ICON_SIZE,
                           KILLCAM_ICON_SIZE, 0, 0);
    }
}

static void CG_KillcamDrawSlots(const CG_KillcamState *state, const CG_KillcamView *view)
{
    if (view->method == KILLCAM_METHOD_NONE) {
        CG_KillcamDrawWeapon(view->weapon, 0.0f);
        return;
    }
    CG_KillcamDrawMethod(state, view->method, -0.5f * KILLCAM_SLOT_PITCH);
    CG_KillcamDrawWeapon(view->weapon, 0.5f * KILLCAM_SLOT_PITCH);
}

void CG_KillcamInit(void)
{
    cg_hudKillcam = Dvar_RegisterBool_mac("cg_hudKillcam", 1, 0x1001);
    memset(&s_killcam, 0, sizeof(s_killcam));
    s_killcam.deathKiller = -1;
    s_killcam.attackersKiller = -1;
    s_killcam.boldFont = CL_RegisterFont("$boldfont", 0);
    s_killcam.regularFont = R_RegisterWwiiFont("$regularfont");
}

int CG_KillcamActive(void)
{
    int killer;

    if (!cg_hudKillcam || !cg_hudKillcam->current.enabled || !cg->inKillCam || !cg->nextSnap)
        return 0;
    killer = cg->nextSnap->ps.clientNum;
    return killer >= 0 && killer < KILLCAM_MAX_CLIENTS && killer != cg->clientNum;
}

int CG_KillcamHidesHudElem(const hudelem_t *elem)
{
    return CG_KillcamActive() && CG_KillcamStockElem(elem) != KILLCAM_STOCK_NONE;
}

void CG_KillcamObituary(int attacker, int target, int eventParm)
{
    int viewer = cg->clientNum;

    if (target < 0 || target >= KILLCAM_MAX_CLIENTS || attacker < 0 || attacker >= KILLCAM_MAX_CLIENTS ||
        attacker == target)
        return;
    if (target == viewer) {
        CG_KillcamClaim(&s_killcam, attacker);
        ++s_killcam.killedViewer[attacker];
        s_killcam.deathKiller = attacker;
        s_killcam.deathTime = cg->time;
        s_killcam.deathParm = eventParm;
    } else if (attacker == viewer) {
        CG_KillcamClaim(&s_killcam, target);
        ++s_killcam.killedByViewer[target];
    }
}

void CG_KillcamSetAttackers(int killer, const int *others, int count)
{
    int i;

    s_killcam.attackersKiller = killer;
    s_killcam.attackersTime = cg->time;
    s_killcam.attackerCount = 0;
    for (i = 0; i < count && i < KILLCAM_MAX_CLIENTS; ++i)
        s_killcam.attackers[s_killcam.attackerCount++] = others[i];
}

void CG_DrawKillcam(void)
{
    CG_KillcamView view;
    float pixelsPerUnit;

    if (!CG_KillcamActive())
        return;
    pixelsPerUnit = CG_KillcamPixelsPerUnit();
    if (fabsf(pixelsPerUnit - s_killcam.discsPixelsPerUnit) > 0.001f)
        CG_KillcamBuildDiscs(&s_killcam, pixelsPerUnit);
    CG_KillcamBuildView(&s_killcam, cg->nextSnap->ps.clientNum, &cg->nextSnap->ps, cg->time, &view);
    CG_KillcamDrawBars();
    CG_KillcamDrawTitle(&s_killcam, view.prompt);
    CG_KillcamDrawDuel(&s_killcam, &view);
    CG_KillcamDrawAttackers(&s_killcam, &view);
    CG_KillcamDrawSlots(&s_killcam, &view);
}
