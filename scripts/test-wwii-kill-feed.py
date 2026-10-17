#!/usr/bin/env python3
"""Run the production WWII kill feed: relations, colors, icons, stacking and fades."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_killfeed_mp.c').read_text()
event = (root / 'src/PC/cgame_mp/cg_event_mp.c').read_text()
draw = (root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text()
main = (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()


def block(text, start_text, declaration=False):
    assert text.count(start_text) == 1, start_text
    start = text.index(start_text)
    brace = text.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    if declaration:
        end = text.index(';', end) + 1
    return text[start:end]


# Obituaries reach the feed (test-kill-feed.py runs that path); the HUD draws
# it with the menus, for players and free spectators, and every level starts
# with an empty feed.
assert event.count('CG_KillFeedAdd(attackerName[0] ? attacker : -1, target, iconShader, iconWidth > 2.0f, iconHorzFlip,\n') == 1
assert draw.count('        if (drawHudMenus) {\n            CG_DrawScorePanel();\n            CG_DrawKillFeed();\n') == 2
assert main.count('    CG_ScorePopupInit();\n    CG_KillFeedInit();\n') == 1

defines = '\n'.join(re.findall(r'^#define KILLFEED_\w+ .*$', source, re.M))
pieces = [
    block(source, 'enum {\n    KILLFEED_NONE = -1,', True),
    block(source, 'typedef struct {\n    int time;', True),
    block(source, 'typedef struct {\n    CG_KillFeedLine lines', True),
    block(source, 'static const vec4_t s_killFeedColors[KILLFEED_RELATION_COUNT] = {', True),
    'static CG_KillFeedState s_killFeed;\nstatic const dvar_t *cg_hudKillFeed;',
]
body = '\n'.join(block(source, name) for name in [
    'static int CG_KillFeedRelation(',
    'static void CG_KillFeedCleanName(',
    'static MaterialHandle CG_KillFeedIcon(',
    'static void CG_KillFeedFit(',
    'static void CG_KillFeedPush(',
    'static void CG_KillFeedExpire(',
    'static float CG_KillFeedAlpha(',
    'static void CG_KillFeedDrawIcon(',
    'static void CG_KillFeedDrawLine(',
    'static void CG_KillFeedDraw(',
    'void CG_KillFeedInit(void)',
    'int CG_KillFeedEnabled(void)',
    'void CG_KillFeedAdd(',
    'void CG_DrawKillFeed(void)',
])

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef float vec_t;
typedef vec_t vec4_t[4];
typedef const float *FontHandle;
typedef const char *MaterialHandle;
typedef struct { struct { int enabled; } current; } dvar_t;
enum { TEAM_FREE, TEAM_AXIS, TEAM_ALLIES, TEAM_SPECTATOR };
typedef struct { int infoValid; char name[32]; int team; } clientInfo_t;
static struct { int time; int clientNum; struct { clientInfo_t clientinfo[64]; } bgs; } cgState;
#define cg (&cgState)
static dvar_t feedDvar = { { 1 } };
const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags) {
    assert(!strcmp(name, "cg_hudKillFeed") && value == 1 && flags == 0x1001);
    return &feedDvar;
}
static const float feedFont = 0.24f;
FontHandle UI_GetFontHandle(int fontEnum, float scale) {
    assert(fontEnum == 0 && fabsf(scale - 0.24f) < 1e-6f);
    return &feedFont;
}
int UI_TextWidth(const char *text, int maxChars, FontHandle font, float scale) {
    assert(maxChars == 0 && font == &feedFont);
    return (int)(strlen(text) * 20.0f * scale);
}
typedef struct { int icon; char text[32]; MaterialHandle material; float x, y, w, h, s0, s1, t0, t1;
                 vec4_t color; int horz, vert, style; } Drawn;
static Drawn drawn[48];
static int drawCount;
void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign,
                 float scale, const vec_t *color, int style) {
    assert(drawCount < 48 && maxChars == 0x7fffffff && font == &feedFont && fabsf(scale - 0.24f) < 1e-6f);
    Drawn *d = &drawn[drawCount++];
    memset(d, 0, sizeof(*d));
    snprintf(d->text, sizeof(d->text), "%s", text);
    d->x = x; d->y = y; memcpy(d->color, color, sizeof(d->color));
    d->horz = horzAlign; d->vert = vertAlign; d->style = style;
}
void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2,
                       float t2, const vec_t *color, MaterialHandle material) {
    assert(drawCount < 48);
    Drawn *d = &drawn[drawCount++];
    memset(d, 0, sizeof(*d));
    d->icon = 1; d->material = material;
    d->x = x; d->y = y; d->w = w; d->h = h; d->s0 = s1; d->t0 = t1; d->s1 = s2; d->t1 = t2;
    memcpy(d->color, color, sizeof(d->color)); d->horz = horzAlign; d->vert = vertAlign;
}
MaterialHandle CL_RegisterMaterial(const char *name, int imageTrack) {
    assert(imageTrack == 7);
    return name;
}
/* The Kar98 image cannot be decoded, so it keeps its own material. */
MaterialHandle R_GetHudSilhouetteMaterial(MaterialHandle icon, float *aspect) {
    static const struct { const char *icon, *flat; float aspect; } flat[] = {
        { "killiconmp40", "flat:killiconmp40", 2.0f }, { "killiconm1garand", "flat:killiconm1garand", 5.0f },
        { "killiconsuicide", "flat:killiconsuicide", 1.0f }, { "killiconheadshot", "flat:killiconheadshot", 1.0f } };
    *aspect = 0.0f;
    for (int i = 0; i < 4; i++) {
        if (!strcmp(icon, flat[i].icon)) {
            *aspect = flat[i].aspect;
            return flat[i].flat;
        }
    }
    return NULL;
}
/* The symbol's image: a quarter wider than its ring for its border. */
static int symbolInits, noSymbol;
static float symbolDiameter;
void CG_HeadshotIconInit(void) { symbolInits++; }
MaterialHandle CG_HeadshotIconMaterial(float diameter, float *extent) {
    symbolDiameter = diameter;
    *extent = 1.25f;
    return noSymbol ? NULL : "*hud_headshot";
}
''' + defines + '\n' + '\n'.join(pieces) + '\n'

checks = r'''
static int near(float a, float b) { return fabsf(a - b) < 1e-4f; }
static const float self[3] = { 0.95f, 0.95f, 0.92f }, friendly[3] = { 0.33f, 0.78f, 0.80f },
                   enemy[3] = { 0.93f, 0.43f, 0.20f }, black[3] = { 0.0f, 0.0f, 0.0f };
static int colored(const Drawn *d, const float *rgb, float alpha) {
    return near(d->color[0], rgb[0]) && near(d->color[1], rgb[1]) && near(d->color[2], rgb[2]) &&
           near(d->color[3], alpha);
}
/* Icons are a black shadow one unit down-right, then the tinted icon. */
static int icon(int i, MaterialHandle material, float x, float y, float w, float h, int flip, const float *rgb,
                float alpha) {
    const Drawn *shadow = &drawn[i], *pic = &drawn[i + 1];
    return shadow->icon && pic->icon && !strcmp(shadow->material, material) && !strcmp(pic->material, material) &&
           near(pic->x, x) && near(pic->y, y) && near(pic->w, w) && near(pic->h, h) &&
           near(shadow->x, x + 1.0f) && near(shadow->y, y + 1.0f) && near(shadow->w, w) && near(shadow->h, h) &&
           near(pic->s0, flip ? 1.0f : 0.0f) && near(pic->s1, flip ? 0.0f : 1.0f) && near(pic->t0, 0.0f) &&
           near(pic->t1, 1.0f) && near(shadow->s0, pic->s0) && near(shadow->s1, pic->s1) &&
           pic->horz == 1 && pic->vert == 3 && shadow->horz == 1 && shadow->vert == 3 &&
           colored(pic, rgb, alpha) && colored(shadow, black, alpha * 0.6f);
}
static int text(int i, const char *value, float x, float y, const float *rgb, float alpha) {
    const Drawn *d = &drawn[i];
    return !d->icon && !strcmp(d->text, value) && near(d->x, x) && near(d->y, y) && d->horz == 1 && d->vert == 3 &&
           d->style == 3 && colored(d, rgb, alpha);
}
static void player(int n, const char *name, int team) {
    cgState.bgs.clientinfo[n].infoValid = 1;
    snprintf(cgState.bgs.clientinfo[n].name, sizeof(cgState.bgs.clientinfo[n].name), "%s", name);
    cgState.bgs.clientinfo[n].team = team;
}
static void render(int now) { drawCount = 0; cgState.time = now; CG_DrawKillFeed(); }
static float width(const char *value) { return (float)(int)(strlen(value) * 20.0f * 0.24f); }

int main(void) {
    char name[KILLFEED_NAME_LEN];
    CG_KillFeedInit();
    assert(CG_KillFeedEnabled() && s_killFeed.count == 0 && symbolInits == 1);

    /* The local player is white, teammates teal and enemies orange; free
     * players are all enemies; spectators see the Allies as friends. */
    assert(CG_KillFeedRelation(4, TEAM_ALLIES, 4, TEAM_ALLIES) == KILLFEED_SELF);
    assert(CG_KillFeedRelation(5, TEAM_ALLIES, 4, TEAM_ALLIES) == KILLFEED_FRIENDLY);
    assert(CG_KillFeedRelation(6, TEAM_AXIS, 4, TEAM_ALLIES) == KILLFEED_ENEMY);
    assert(CG_KillFeedRelation(6, TEAM_AXIS, 4, TEAM_AXIS) == KILLFEED_FRIENDLY);
    assert(CG_KillFeedRelation(5, TEAM_ALLIES, 4, TEAM_AXIS) == KILLFEED_ENEMY);
    assert(CG_KillFeedRelation(5, TEAM_FREE, 4, TEAM_FREE) == KILLFEED_ENEMY);
    assert(CG_KillFeedRelation(4, TEAM_FREE, 4, TEAM_FREE) == KILLFEED_SELF);
    assert(CG_KillFeedRelation(5, TEAM_ALLIES, 4, TEAM_SPECTATOR) == KILLFEED_FRIENDLY);
    assert(CG_KillFeedRelation(6, TEAM_AXIS, 4, TEAM_SPECTATOR) == KILLFEED_ENEMY);

    /* Names lose their color codes and fit the line buffer. */
    CG_KillFeedCleanName(name, "^1Red^7Name^");
    assert(!strcmp(name, "RedName^"));
    CG_KillFeedCleanName(name, "a^^7b");
    assert(!strcmp(name, "a^b"));
    CG_KillFeedCleanName(name, "0123456789012345678901234567890123456789");
    assert(strlen(name) == KILLFEED_NAME_LEN - 1);

    /* "attacker [icon] victim" from the bottom-left corner: the flat icon in
     * the attacker's color, at most 44 x 13 in its own proportions, centred in
     * a 62-unit slot 4 units above the baseline. */
    cgState.clientNum = 0;
    player(0, "^5HUD ^7QA", TEAM_ALLIES);
    player(1, "Mate", TEAM_ALLIES);
    player(2, "Foe", TEAM_AXIS);
    player(3, "Other", TEAM_AXIS);
    cgState.time = 1000;
    CG_KillFeedAdd(0, 2, "killiconmp40", 0, 0, 0);
    render(2000);
    float slot = 12.0f + width("HUD QA");
    assert(drawCount == 4 && text(0, "HUD QA", 12.0f, -87.0f, self, 1.0f));
    assert(icon(1, "flat:killiconmp40", slot + 18.0f, -87.0f - 4.0f - 6.5f, 26.0f, 13.0f, 0, self, 1.0f));
    assert(text(3, "Foe", slot + 62.0f, -87.0f, enemy, 1.0f));

    /* A stock image fills a 22-unit box, twice as wide for wide icons, and
     * flipped icons mirror. A death without a killer starts with the icon,
     * in the victim's color. */
    cgState.time = 1100;
    CG_KillFeedAdd(2, 1, "killiconkar98", 1, 1, 0);
    CG_KillFeedAdd(-1, 3, "killiconsuicide", 0, 0, 0);
    render(2000);
    assert(drawCount == 4 + 4 + 3);
    assert(text(0, "HUD QA", 12.0f, -117.0f, self, 1.0f) && text(3, "Foe", slot + 62.0f, -117.0f, enemy, 1.0f));
    slot = 12.0f + width("Foe");
    assert(text(4, "Foe", 12.0f, -102.0f, enemy, 1.0f));
    assert(icon(5, "killiconkar98", slot + 9.0f, -102.0f - 4.0f - 11.0f, 44.0f, 22.0f, 1, enemy, 1.0f));
    assert(text(7, "Mate", slot + 62.0f, -102.0f, friendly, 1.0f));
    assert(icon(8, "flat:killiconsuicide", 12.0f, -87.0f - 4.0f - 6.5f, 13.0f, 13.0f, 0, enemy, 1.0f));
    assert(text(10, "Other", 12.0f + 13.0f + 8.0f, -87.0f, enemy, 1.0f));

    /* A long silhouette is held to the width instead. */
    s_killFeed.count = 0;
    cgState.time = 1100;
    CG_KillFeedAdd(1, 2, "killiconm1garand", 1, 0, 0);
    render(2000);
    slot = 12.0f + width("Mate");
    assert(drawCount == 4 && icon(1, "flat:killiconm1garand", slot + 9.0f, -87.0f - 4.0f - 4.4f, 44.0f, 8.8f, 0, friendly, 1.0f));

    /* Five lines at most: the oldest goes first. */
    s_killFeed.count = 0;
    cgState.time = 1000;
    CG_KillFeedAdd(0, 2, "killiconmp40", 0, 0, 0);
    cgState.time = 1100;
    CG_KillFeedAdd(2, 1, "killiconkar98", 1, 1, 0);
    CG_KillFeedAdd(-1, 3, "killiconsuicide", 0, 0, 0);
    cgState.time = 1200;
    CG_KillFeedAdd(1, 2, "killiconmp40", 0, 0, 0);
    CG_KillFeedAdd(1, 3, "killiconmp40", 0, 0, 0);
    CG_KillFeedAdd(3, 0, "killiconmp40", 0, 0, 0);
    assert(s_killFeed.count == 5 && s_killFeed.lines[0].time == 1100 && !strcmp(s_killFeed.lines[0].victim, "Mate"));
    assert(!strcmp(s_killFeed.lines[4].victim, "HUD QA") && s_killFeed.lines[4].victimRelation == KILLFEED_SELF);
    render(2000);
    assert(near(drawn[0].y, -87.0f - 60.0f));

    /* Invalid clients are ignored. */
    CG_KillFeedAdd(64, 2, "killiconmp40", 0, 0, 0);
    CG_KillFeedAdd(-2, 2, "killiconmp40", 0, 0, 0);
    CG_KillFeedAdd(1, 64, "killiconmp40", 0, 0, 0);
    CG_KillFeedAdd(1, -1, "killiconmp40", 0, 0, 0);
    assert(s_killFeed.count == 5 && s_killFeed.lines[0].time == 1100 && !strcmp(s_killFeed.lines[4].victim, "HUD QA"));

    /* A headshot shows the symbol instead of the icon it came with: its ring
     * 11.75 units wide in a slot 6 units wider each side, centred 6 units
     * above the baseline in the attacker's color, the image's border around
     * the ring. Without an attacker the symbol leads, the victim 8 units
     * after the ring. */
    s_killFeed.count = 0;
    cgState.time = 3000;
    CG_KillFeedAdd(0, 2, "killiconheadshot", 0, 0, 1);
    CG_KillFeedAdd(2, 1, "killiconheadshot", 0, 0, 1);
    CG_KillFeedAdd(-1, 3, "killiconheadshot", 0, 0, 1);
    render(3500);
    const float side = 11.75f * 1.25f, border = 0.5f * (side - 11.75f);
    slot = 12.0f + width("HUD QA");
    assert(drawCount == 4 + 4 + 3 && near(symbolDiameter, 11.75f) && text(0, "HUD QA", 12.0f, -117.0f, self, 1.0f));
    assert(icon(1, "*hud_headshot", slot + 6.0f - border, -117.0f - 6.0f - 0.5f * side, side, side, 0, self, 1.0f));
    assert(text(3, "Foe", slot + 23.75f, -117.0f, enemy, 1.0f));
    slot = 12.0f + width("Foe");
    assert(text(4, "Foe", 12.0f, -102.0f, enemy, 1.0f));
    assert(icon(5, "*hud_headshot", slot + 6.0f - border, -102.0f - 6.0f - 0.5f * side, side, side, 0, enemy, 1.0f));
    assert(text(7, "Mate", slot + 23.75f, -102.0f, friendly, 1.0f));
    assert(icon(8, "*hud_headshot", 12.0f - border, -87.0f - 6.0f - 0.5f * side, side, side, 0, enemy, 1.0f));
    assert(text(10, "Other", 12.0f + 11.75f + 8.0f, -87.0f, enemy, 1.0f));
    /* While there is no symbol to draw, the line keeps its icon and slot. */
    noSymbol = 1;
    render(3500);
    slot = 12.0f + width("HUD QA");
    assert(drawCount == 4 + 4 + 3 && icon(1, "flat:killiconheadshot", slot + 24.5f, -117.0f - 4.0f - 6.5f, 13.0f, 13.0f, 0, self, 1.0f));
    assert(text(3, "Foe", slot + 62.0f, -117.0f, enemy, 1.0f));
    assert(icon(8, "flat:killiconheadshot", 12.0f, -87.0f - 4.0f - 6.5f, 13.0f, 13.0f, 0, enemy, 1.0f));
    assert(text(10, "Other", 12.0f + 13.0f + 8.0f, -87.0f, enemy, 1.0f));
    noSymbol = 0;
    /* Only headshots show it. */
    s_killFeed.count = 0;
    cgState.time = 3000;
    CG_KillFeedAdd(0, 2, "killiconheadshot", 0, 0, 0);
    render(3500);
    slot = 12.0f + width("HUD QA");
    assert(drawCount == 4 && icon(1, "flat:killiconheadshot", slot + 24.5f, -87.0f - 4.0f - 6.5f, 13.0f, 13.0f, 0, self, 1.0f));

    /* Lines fade in over 150 ms and out over the last 500 ms of 6 s. */
    s_killFeed.count = 0;
    cgState.time = 10000;
    CG_KillFeedAdd(0, 2, "killiconmp40", 0, 0, 0);
    render(10075);
    slot = 12.0f + width("HUD QA");
    assert(drawCount == 4 && text(0, "HUD QA", 12.0f, -87.0f, self, 0.5f) && text(3, "Foe", slot + 62.0f, -87.0f, enemy, 0.5f));
    assert(icon(1, "flat:killiconmp40", slot + 18.0f, -97.5f, 26.0f, 13.0f, 0, self, 0.5f));
    render(15750);
    assert(drawCount == 4 && text(0, "HUD QA", 12.0f, -87.0f, self, 0.5f));
    assert(icon(1, "flat:killiconmp40", slot + 18.0f, -97.5f, 26.0f, 13.0f, 0, self, 0.5f));
    render(16000);
    assert(drawCount == 0 && s_killFeed.count == 0);

    /* A clock that went back (a new level) drops the old lines. */
    cgState.time = 10000;
    CG_KillFeedAdd(0, 2, "killiconmp40", 0, 0, 0);
    render(500);
    assert(drawCount == 0 && s_killFeed.count == 0);

    /* Turned off, nothing is drawn. */
    cgState.time = 20000;
    CG_KillFeedAdd(0, 2, "killiconmp40", 0, 0, 0);
    feedDvar.current.enabled = 0;
    assert(!CG_KillFeedEnabled());
    render(20500);
    assert(drawCount == 0);
    return 0;
}
'''


def mutate(old, new):
    assert body.count(old) + support.count(old) == 1, old
    return (support.replace(old, new), body) if old in support else (support, body.replace(old, new))


mutants = [
    mutate('if (clientNum == localClientNum)\n', 'if (0)\n'),
    mutate('return team == localTeam ? KILLFEED_FRIENDLY : KILLFEED_ENEMY;', 'return KILLFEED_FRIENDLY;'),
    mutate('if (localTeam == TEAM_SPECTATOR && team == TEAM_ALLIES)', 'if (localTeam == TEAM_SPECTATOR)'),
    mutate("if (src[0] == '^' && src[1] && src[1] != '^')", "if (src[0] == '^' && src[1])"),
    mutate('        memmove(state->lines, state->lines + 1, (KILLFEED_MAX_LINES - 1) * sizeof(state->lines[0]));\n',
           ''),
    mutate('if (elapsed >= 0 && elapsed < KILLFEED_MSEC)', 'if (elapsed < KILLFEED_MSEC)'),
    mutate('if (elapsed > KILLFEED_MSEC - KILLFEED_FADE_OUT_MSEC)', 'if (0)'),
    mutate('(float)(state->count - 1 - i) * KILLFEED_LINE_HEIGHT', '(float)i * KILLFEED_LINE_HEIGHT'),
    mutate('KILLFEED_STOCK_ICON_HEIGHT * (wideIcon ? 2.0f : 1.0f)', 'KILLFEED_STOCK_ICON_HEIGHT'),
    mutate('    if (aspect > 0.0f) {\n', '    if (1) {\n'),
    mutate('    if (*width > maxWidth) {', '    if (0) {'),
    mutate('*width = maxHeight * aspect;', '*width = maxHeight;'),
    mutate('float s1 = flip ? 1.0f : 0.0f;', 'float s1 = 0.0f;'),
    mutate('line->attackerRelation != KILLFEED_NONE ? line->attackerRelation : line->victimRelation;',
           'line->victimRelation;'),
    mutate('    memcpy(color, s_killFeedColors[iconRelation], sizeof(color));\n    color[3] = alpha;\n',
           '    memcpy(color, s_killFeedColors[iconRelation], sizeof(color));\n'),
    mutate('iconX = x + (slotWidth - iconWidth) * 0.5f;', 'iconX = x;'),
    mutate('        x += slotWidth;\n', '        x += iconWidth;\n'),
    mutate('x += iconWidth + KILLFEED_GAP;', 'x += iconWidth;'),
    mutate('MaterialHandle symbol = line->headshot ? CG_HeadshotIconMaterial',
           'MaterialHandle symbol = 1 ? CG_HeadshotIconMaterial'),
    mutate('float iconWidth = symbol ? KILLFEED_HEADSHOT_SIZE : line->iconWidth;',
           'float iconWidth = line->iconWidth;'),
    mutate('float slotWidth = symbol ? KILLFEED_HEADSHOT_SIZE + 2.0f * KILLFEED_HEADSHOT_GAP : KILLFEED_SLOT_WIDTH;',
           'float slotWidth = symbol ? KILLFEED_HEADSHOT_SIZE + KILLFEED_HEADSHOT_GAP : KILLFEED_SLOT_WIDTH;'),
    mutate('float side = KILLFEED_HEADSHOT_SIZE * extent;', 'float side = KILLFEED_HEADSHOT_SIZE;'),
    mutate('CG_KillFeedDrawIcon(iconX + 0.5f * (KILLFEED_HEADSHOT_SIZE - side),', 'CG_KillFeedDrawIcon(iconX,'),
    mutate('side, side,\n                            0, color, symbol);', 'side, side,\n                            0, color, line->icon);'),
    mutate('#define KILLFEED_HEADSHOT_RISE 6.0f', '#define KILLFEED_HEADSHOT_RISE 4.0f'),
    mutate('    line.headshot = headshot;\n', ''),
    mutate('    CG_HeadshotIconInit();\n', ''),
    mutate('#define KILLFEED_HEADSHOT_SIZE 11.75f', '#define KILLFEED_HEADSHOT_SIZE 11.0f'),
    mutate('#define KILLFEED_HEADSHOT_GAP 6.0f', '#define KILLFEED_HEADSHOT_GAP 2.0f'),
    mutate('return silhouette ? silhouette : icon;', 'return icon;'),
    mutate('color[3] * KILLFEED_SHADOW_ALPHA', 'KILLFEED_SHADOW_ALPHA'),
    mutate('CL_DrawStretchPic(x + KILLFEED_SHADOW_OFFSET, y + KILLFEED_SHADOW_OFFSET,', 'CL_DrawStretchPic(x, y,'),
    mutate('CG_KillFeedDrawIcon(iconX, y - KILLFEED_ICON_RISE, line->iconWidth,', 'CG_KillFeedDrawIcon(iconX, y, line->iconWidth,'),
    mutate('attacker >= KILLFEED_MAX_CLIENTS || attacker < -1', 'attacker >= KILLFEED_MAX_CLIENTS'),
    mutate('return cg_hudKillFeed && cg_hudKillFeed->current.enabled;', 'return cg_hudKillFeed != 0;'),
    mutate('#define KILLFEED_MAX_LINES 5', '#define KILLFEED_MAX_LINES 6'),
    mutate('#define KILLFEED_BASELINE -87.0f', '#define KILLFEED_BASELINE -60.0f'),
    mutate('#define KILLFEED_LINE_HEIGHT 15.0f', '#define KILLFEED_LINE_HEIGHT 14.0f'),
]
with tempfile.TemporaryDirectory(prefix='cod2-killfeed-') as directory:
    path = Path(directory) / 'killfeed.c'
    binary = Path(directory) / 'killfeed'
    for index, (support_source, feed_source) in enumerate([(support, body)] + mutants):
        path.write_text(support_source + feed_source + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        str(path), '-o', str(binary), '-lm'], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
print('PASS: the kill feed draws "attacker [weapon] victim" from the bottom-left with the newest line lowest, colors '
      'the local player white, teammates teal and enemies orange (Allies for spectators), centres the flat icon in a '
      'fixed slot with a shadow and the attacker\'s tint, fits silhouettes into 44 x 13 by their proportions, keeps '
      'stock icons in a 22-unit box widened and mirrored as the weapon asks, shows a headshot as the WWII symbol '
      '11.75 units wide in its own slot, keeping the given icon while the symbol cannot be drawn, keeps five lines, '
      'fades over 6 s and drops lines from an older clock; '
      f'{len(mutants)} failing regression mutants')
