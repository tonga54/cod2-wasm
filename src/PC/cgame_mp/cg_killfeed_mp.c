#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_headshoticon.h"
#include "headers/PC/cgame_mp/cg_killfeed.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"

#include <string.h>

extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern FontHandle UI_GetFontHandle(int fontEnum, float scale);
extern int UI_TextWidth(const char *text, int maxChars, FontHandle font, float scale);
extern void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style);
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material);
extern MaterialHandle CL_RegisterMaterial(const char *name, int imageTrack);

/* Virtual units from the bottom-left corner of the safe area: the newest line
 * sits at the bottom, above the score panel, as in the WWII capture. */
#define KILLFEED_X 12.0f
#define KILLFEED_BASELINE -87.0f
#define KILLFEED_LINE_HEIGHT 15.0f
#define KILLFEED_TEXT_SCALE 0.24f
/* The weapon is centred in a slot of fixed width between the two names. */
#define KILLFEED_SLOT_WIDTH 62.0f
/* Silhouettes keep their proportions within this box. */
#define KILLFEED_ICON_MAX_WIDTH 44.0f
#define KILLFEED_ICON_MAX_HEIGHT 13.0f
/* Stock images leave transparent rows around the weapon, so without a
 * silhouette the box is taller. */
#define KILLFEED_STOCK_ICON_HEIGHT 22.0f
/* Distance from the text baseline up to the icon's centre. */
#define KILLFEED_ICON_RISE 4.0f
/* A headshot's symbol stands in for the weapon, its ring as wide as on the
 * WWII capture, 52.9 pixels at 4.5 a unit, with a gap to each name. It is
 * centred higher than the weapons, 0.56 of the names' capitals above their
 * foot as on the capture; that foot is 1.9 units above the baseline. */
#define KILLFEED_HEADSHOT_SIZE 11.75f
#define KILLFEED_HEADSHOT_GAP 6.0f
#define KILLFEED_HEADSHOT_RISE 6.0f
/* Between a leading icon and the victim when there is no attacker. */
#define KILLFEED_GAP 8.0f
#define KILLFEED_SHADOW_OFFSET 1.0f
#define KILLFEED_SHADOW_ALPHA 0.6f
#define KILLFEED_MAX_LINES 5
#define KILLFEED_MSEC 6000
#define KILLFEED_FADE_IN_MSEC 150
#define KILLFEED_FADE_OUT_MSEC 500
#define KILLFEED_NAME_LEN 32
#define KILLFEED_ALIGN_LEFT 1
#define KILLFEED_ALIGN_BOTTOM 3
#define KILLFEED_MAX_CLIENTS 64

enum {
    KILLFEED_NONE = -1,
    KILLFEED_SELF,
    KILLFEED_FRIENDLY,
    KILLFEED_ENEMY,
    KILLFEED_RELATION_COUNT
};

typedef struct {
    int time;
    int attackerRelation;
    int victimRelation;
    char attacker[KILLFEED_NAME_LEN];
    char victim[KILLFEED_NAME_LEN];
    MaterialHandle icon;
    float iconWidth;
    float iconHeight;
    int flip;
    /* The headshot symbol stands in for the icon whenever it can be drawn. */
    int headshot;
} CG_KillFeedLine;

/* Oldest line first. */
typedef struct {
    CG_KillFeedLine lines[KILLFEED_MAX_LINES];
    int count;
} CG_KillFeedState;

static const vec4_t s_killFeedColors[KILLFEED_RELATION_COUNT] = {
    { 0.95f, 0.95f, 0.92f, 1.0f },
    { 0.33f, 0.78f, 0.80f, 1.0f },
    { 0.93f, 0.43f, 0.20f, 1.0f }
};

static CG_KillFeedState s_killFeed;
static const dvar_t *cg_hudKillFeed;

/* Without a team of their own, spectators see the Allies as friends. Free
 * players are all enemies of each other. */
static int CG_KillFeedRelation(int clientNum, int team, int localClientNum, int localTeam)
{
    if (clientNum == localClientNum)
        return KILLFEED_SELF;
    if (localTeam == TEAM_AXIS || localTeam == TEAM_ALLIES)
        return team == localTeam ? KILLFEED_FRIENDLY : KILLFEED_ENEMY;
    if (localTeam == TEAM_SPECTATOR && team == TEAM_ALLIES)
        return KILLFEED_FRIENDLY;
    return KILLFEED_ENEMY;
}

/* The relation colors the whole name, so color codes are dropped. */
static void CG_KillFeedCleanName(char *dest, const char *src)
{
    int length = 0;

    while (*src && length < KILLFEED_NAME_LEN - 1) {
        if (src[0] == '^' && src[1] && src[1] != '^') {
            src += 2;
            continue;
        }
        dest[length++] = *src++;
    }
    dest[length] = '\0';
}

/* The stock icons are shaded grey; their silhouette takes the tint flat.
 * aspect is 0 when the stock icon has to be used. */
static MaterialHandle CG_KillFeedIcon(const char *name, float *aspect)
{
    MaterialHandle icon = CL_RegisterMaterial(name, 7);
    MaterialHandle silhouette = R_GetHudSilhouetteMaterial(icon, aspect);

    return silhouette ? silhouette : icon;
}

static void CG_KillFeedFit(float aspect, float maxWidth, float maxHeight, float *width, float *height)
{
    *height = maxHeight;
    *width = maxHeight * aspect;
    if (*width > maxWidth) {
        *width = maxWidth;
        *height = maxWidth / aspect;
    }
}

static void CG_KillFeedPush(CG_KillFeedState *state, const CG_KillFeedLine *line)
{
    if (state->count == KILLFEED_MAX_LINES) {
        memmove(state->lines, state->lines + 1, (KILLFEED_MAX_LINES - 1) * sizeof(state->lines[0]));
        state->count = KILLFEED_MAX_LINES - 1;
    }
    state->lines[state->count++] = *line;
}

static void CG_KillFeedExpire(CG_KillFeedState *state, int now)
{
    int kept = 0;
    int i;

    for (i = 0; i < state->count; ++i) {
        int elapsed = now - state->lines[i].time;
        if (elapsed >= 0 && elapsed < KILLFEED_MSEC)
            state->lines[kept++] = state->lines[i];
    }
    state->count = kept;
}

static float CG_KillFeedAlpha(int elapsed)
{
    if (elapsed < KILLFEED_FADE_IN_MSEC)
        return (float)elapsed / (float)KILLFEED_FADE_IN_MSEC;
    if (elapsed > KILLFEED_MSEC - KILLFEED_FADE_OUT_MSEC)
        return (float)(KILLFEED_MSEC - elapsed) / (float)KILLFEED_FADE_OUT_MSEC;
    return 1.0f;
}

static void CG_KillFeedDrawIcon(float x, float centerY, float width, float height, int flip, const vec_t *color,
                                MaterialHandle material)
{
    const vec4_t shadow = { 0.0f, 0.0f, 0.0f, color[3] * KILLFEED_SHADOW_ALPHA };
    float y = centerY - height * 0.5f;
    float s1 = flip ? 1.0f : 0.0f;
    float s2 = flip ? 0.0f : 1.0f;

    CL_DrawStretchPic(x + KILLFEED_SHADOW_OFFSET, y + KILLFEED_SHADOW_OFFSET, width, height, KILLFEED_ALIGN_LEFT,
                      KILLFEED_ALIGN_BOTTOM, s1, 0.0f, s2, 1.0f, shadow, material);
    CL_DrawStretchPic(x, y, width, height, KILLFEED_ALIGN_LEFT, KILLFEED_ALIGN_BOTTOM, s1, 0.0f, s2, 1.0f, color,
                      material);
}

static void CG_KillFeedDrawLine(const CG_KillFeedLine *line, float y, float alpha, FontHandle font)
{
    vec4_t color;
    float x = KILLFEED_X;
    float extent = 1.0f;
    MaterialHandle symbol = line->headshot ? CG_HeadshotIconMaterial(KILLFEED_HEADSHOT_SIZE, &extent) : NULL;
    float iconWidth = symbol ? KILLFEED_HEADSHOT_SIZE : line->iconWidth;
    float slotWidth = symbol ? KILLFEED_HEADSHOT_SIZE + 2.0f * KILLFEED_HEADSHOT_GAP : KILLFEED_SLOT_WIDTH;
    float iconX;
    int iconRelation = line->attackerRelation != KILLFEED_NONE ? line->attackerRelation : line->victimRelation;

    if (line->attackerRelation != KILLFEED_NONE) {
        memcpy(color, s_killFeedColors[line->attackerRelation], sizeof(color));
        color[3] = alpha;
        UI_DrawText(line->attacker, 0x7fffffff, font, x, y, KILLFEED_ALIGN_LEFT, KILLFEED_ALIGN_BOTTOM,
                    KILLFEED_TEXT_SCALE, color, 3);
        x += (float)UI_TextWidth(line->attacker, 0, font, KILLFEED_TEXT_SCALE);
        iconX = x + (slotWidth - iconWidth) * 0.5f;
        x += slotWidth;
    } else {
        iconX = x;
        x += iconWidth + KILLFEED_GAP;
    }

    memcpy(color, s_killFeedColors[iconRelation], sizeof(color));
    color[3] = alpha;
    if (symbol) {
        float side = KILLFEED_HEADSHOT_SIZE * extent;

        CG_KillFeedDrawIcon(iconX + 0.5f * (KILLFEED_HEADSHOT_SIZE - side), y - KILLFEED_HEADSHOT_RISE, side, side,
                            0, color, symbol);
    } else {
        CG_KillFeedDrawIcon(iconX, y - KILLFEED_ICON_RISE, line->iconWidth, line->iconHeight, line->flip, color,
                            line->icon);
    }

    memcpy(color, s_killFeedColors[line->victimRelation], sizeof(color));
    color[3] = alpha;
    UI_DrawText(line->victim, 0x7fffffff, font, x, y, KILLFEED_ALIGN_LEFT, KILLFEED_ALIGN_BOTTOM,
                KILLFEED_TEXT_SCALE, color, 3);
}

static void CG_KillFeedDraw(CG_KillFeedState *state, int now)
{
    FontHandle font;
    int i;

    CG_KillFeedExpire(state, now);
    if (!state->count)
        return;
    font = UI_GetFontHandle(0, KILLFEED_TEXT_SCALE);
    for (i = 0; i < state->count; ++i) {
        const CG_KillFeedLine *line = &state->lines[i];
        float y = KILLFEED_BASELINE - (float)(state->count - 1 - i) * KILLFEED_LINE_HEIGHT;
        CG_KillFeedDrawLine(line, y, CG_KillFeedAlpha(now - line->time), font);
    }
}

void CG_KillFeedInit(void)
{
    cg_hudKillFeed = Dvar_RegisterBool_mac("cg_hudKillFeed", 1, 0x1001);
    s_killFeed.count = 0;
    CG_HeadshotIconInit();
}

int CG_KillFeedEnabled(void)
{
    return cg_hudKillFeed && cg_hudKillFeed->current.enabled;
}

/* attacker is -1 for deaths without a killer (world, suicide). */
void CG_KillFeedAdd(int attacker, int victim, const char *iconShader, int wideIcon, int flipIcon, int headshot)
{
    CG_KillFeedLine line;
    const clientInfo_t *victimInfo;
    float aspect;
    int localTeam;

    if ((unsigned)victim >= KILLFEED_MAX_CLIENTS || attacker >= KILLFEED_MAX_CLIENTS || attacker < -1)
        return;
    victimInfo = &cg->bgs.clientinfo[victim];
    localTeam = cg->bgs.clientinfo[cg->clientNum].team;
    memset(&line, 0, sizeof(line));
    line.time = cg->time;
    line.attackerRelation = KILLFEED_NONE;
    if (attacker >= 0) {
        const clientInfo_t *attackerInfo = &cg->bgs.clientinfo[attacker];
        line.attackerRelation = CG_KillFeedRelation(attacker, attackerInfo->team, cg->clientNum, localTeam);
        CG_KillFeedCleanName(line.attacker, attackerInfo->name);
    }
    line.victimRelation = CG_KillFeedRelation(victim, victimInfo->team, cg->clientNum, localTeam);
    CG_KillFeedCleanName(line.victim, victimInfo->name);
    line.icon = CG_KillFeedIcon(iconShader, &aspect);
    if (aspect > 0.0f) {
        CG_KillFeedFit(aspect, KILLFEED_ICON_MAX_WIDTH, KILLFEED_ICON_MAX_HEIGHT, &line.iconWidth, &line.iconHeight);
    } else {
        line.iconHeight = KILLFEED_STOCK_ICON_HEIGHT;
        line.iconWidth = KILLFEED_STOCK_ICON_HEIGHT * (wideIcon ? 2.0f : 1.0f);
    }
    line.flip = flipIcon;
    line.headshot = headshot;
    CG_KillFeedPush(&s_killFeed, &line);
}

void CG_DrawKillFeed(void)
{
    if (!CG_KillFeedEnabled())
        return;
    CG_KillFeedDraw(&s_killFeed, cg->time);
}
