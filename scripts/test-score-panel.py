#!/usr/bin/env python3
"""Run the production WWII score panel: stock HUD elements it replaces, team and
deathmatch views, the standing label, bar fills and the drawn layout."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_scorepanel_mp.c').read_text()
hud = (root / 'src/PC/cgame_mp/cg_hudelem_mp.c').read_text()
vector = (root / 'src/PC/universal/com_vector.h').read_text()
draw = (root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text()
main = (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()


def function(text, name):
    match = re.search(r'^((?:static )?[^\n;]*\b' + name + r'\([^;]*?\))\s*\{', text, re.M)
    assert match, name
    brace = text.index('{', match.end() - 1)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


def definition(text, head):
    start = text.index(head)
    return text[start:text.index('\n};\n', start) + 4]


# Players and free spectators both get the panel, before the kill feed above
# it; every level starts with a fresh panel.
assert draw.count('        if (drawHudMenus) {\n            CG_DrawScorePanel();\n            CG_DrawKillFeed();\n') == 2
assert main.count('    CG_KillFeedInit();\n    CG_ScorePanelInit();\n') == 1

panel = source[source.index('/* Virtual units from the bottom-left corner'):]
timer = ''.join(function(hud, name) for name in (
    'cg_time', 'CG_FormatTimerString', 'CG_GetHudElemColor', 'CG_GetHudElemTimer'))

support = r'''
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
typedef unsigned char byte;
typedef int qboolean;
typedef float vec_t;
typedef vec_t vec4_t[4];
typedef const float *FontHandle;
typedef const char *MaterialHandle;
''' + definition(vector, 'enum he_type_t {') + r'''
typedef enum he_type_t he_type_t;
''' + definition(vector, 'union hudelem_color_t {') + r'''
typedef union hudelem_color_t hudelem_color_t;
''' + definition(vector, 'struct hudelem_s {') + r'''
typedef struct hudelem_s hudelem_t;
typedef struct { struct { int enabled; } current; } dvar_t;
enum { TEAM_FREE, TEAM_AXIS, TEAM_ALLIES, TEAM_SPECTATOR };
typedef struct { int infoValid; int team; } clientInfo_t;
typedef struct { int clientNum; struct { hudelem_t current[31]; hudelem_t archival[31]; } hud; } playerState_t;
typedef struct { playerState_t ps; } snapshot_t;
typedef struct { float xy[2]; float st[2]; float rgba[4]; } HudVertex2D;
static snapshot_t snapState;
/* The client table comes last, so reading past it leaves the object. */
static struct { int time; snapshot_t *snap; struct { clientInfo_t clientinfo[64]; } bgs; } cgState;
static struct { char gametype[32]; int teamScores[2]; struct { MaterialHandle whiteMaterial; } media; } cgsState;
#define cg (&cgState)
#define cgs (&cgsState)

static const char *va(const char *fmt, ...) {
    static char buffers[4][64];
    static int next;
    char *buffer = buffers[next++ & 3];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffers[0]), fmt, args);
    va_end(args);
    return buffer;
}
static dvar_t panelDvar = { { 1 } };
static const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags) {
    assert(!strcmp(name, "cg_hudScorePanel") && value == 1 && flags == 0x1001);
    return &panelDvar;
}
/* One font per scale, so a width is always measured with the drawn font. */
static float fonts[16];
static int fontCount;
static FontHandle UI_GetFontHandle(int fontEnum, float scale) {
    assert(fontEnum == 0);
    for (int i = 0; i < fontCount; i++)
        if (fonts[i] == scale) return &fonts[i];
    assert(fontCount < 16);
    fonts[fontCount] = scale;
    return &fonts[fontCount++];
}
static int UI_TextWidth(const char *text, int maxChars, FontHandle font, float scale) {
    assert(maxChars == 0 && *font == scale);
    return (int)(strlen(text) * 20.0f * scale);
}
typedef struct { int pic; char text[32]; MaterialHandle material; float x, y, w, h, s0, t0, s1, t1, scale;
                 vec4_t color; int horz, vert, style; } Drawn;
static Drawn drawn[16];
static int drawCount;
static void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign,
                        int vertAlign, float scale, const vec_t *color, int style) {
    assert(drawCount < 16 && maxChars == 0x7fffffff && *font == scale);
    Drawn *d = &drawn[drawCount++];
    memset(d, 0, sizeof(*d));
    snprintf(d->text, sizeof(d->text), "%s", text);
    d->x = x; d->y = y; d->scale = scale; memcpy(d->color, color, sizeof(d->color));
    d->horz = horzAlign; d->vert = vertAlign; d->style = style;
}
static void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1,
                              float s2, float t2, const vec_t *color, MaterialHandle material) {
    assert(drawCount < 16);
    Drawn *d = &drawn[drawCount++];
    memset(d, 0, sizeof(*d));
    d->pic = 1; d->material = material; d->x = x; d->y = y; d->w = w; d->h = h;
    d->s0 = s1; d->t0 = t1; d->s1 = s2; d->t1 = t2; memcpy(d->color, color, sizeof(d->color));
    d->horz = horzAlign; d->vert = vertAlign;
}
/* Materials are interned so that they outlive the caller's name buffer. */
static char materialNames[16][48];
static int materialCount;
static MaterialHandle intern(const char *prefix, const char *name) {
    char full[48];
    snprintf(full, sizeof(full), "%s%s", prefix, name);
    for (int i = 0; i < materialCount; i++)
        if (!strcmp(materialNames[i], full)) return materialNames[i];
    assert(materialCount < 16);
    snprintf(materialNames[materialCount], sizeof(materialNames[0]), "%s", full);
    return materialNames[materialCount++];
}
static MaterialHandle CL_RegisterMaterial(const char *name, int imageTrack) {
    assert(imageTrack == 7);
    return intern("mat:", name);
}
/* The American icon cannot be turned into an emblem. */
static MaterialHandle R_GetHudEmblemMaterial(MaterialHandle icon) {
    if (!strcmp(icon, "mat:hudicon_american")) return NULL;
    return intern("emblem:", icon + 4);
}
static const char *const materials[] = { "", "hudicon_british", "hudicon_german", "hudicon_american",
                                         "objpoint_flag" };
static int CG_ServerMaterialName(int index, char *buf, int size) {
    if (index < 1 || index > 4) return 0;
    snprintf(buf, size, "%s", materials[index]);
    return 1;
}
/* Physical pixels: twice the virtual units, offset from the bottom-left. */
static void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign) {
    assert(horzAlign == 1 && vertAlign == 3);
    *x = 10.0f + 2.0f * *x; *y = 700.0f + 2.0f * *y; *w *= 2.0f; *h *= 2.0f;
}
static HudVertex2D triVerts[3];
static unsigned short triIndices[3];
static MaterialHandle triMaterial;
static int triCount;
static void R_AddCmdDrawTriangles2D(const HudVertex2D *verts, int vertexCount, const unsigned short *indices,
                                    int indexCount, MaterialHandle material) {
    assert(vertexCount == 3 && indexCount == 3);
    memcpy(triVerts, verts, sizeof(triVerts)); memcpy(triIndices, indices, sizeof(triIndices));
    triMaterial = material; triCount++;
}
static const char *serverinfo = "";
static const char *CL_GetConfigString(int index) { assert(index == 0); return serverinfo; }
static const char *Info_ValueForKey(const char *s, const char *key) {
    static char value[64];
    char k[64];
    value[0] = 0;
    while (*s == '\\') {
        int n = 0;
        for (++s; *s && *s != '\\'; ++s) k[n++] = *s;
        k[n] = 0;
        n = 0;
        if (*s) for (++s; *s && *s != '\\'; ++s) value[n++] = *s;
        value[n] = 0;
        if (!strcmp(k, key)) return value;
    }
    value[0] = 0;
    return value;
}
static const char *const modes[][3] = {
    { "dm", "MPUI_DEATHMATCH", "Deathmatch" }, { "tdm", "MPUI_TEAM_DEATHMATCH", "Team Deathmatch" },
    { "sd", "MPUI_SEARCH_AND_DESTROY", "Search and Destroy" }, { "ctf", "MPUI_CAPTURE_THE_FLAG", "Capture the Flag" },
    { "hq", "MPUI_HEADQUARTERS", "Headquarters" } };
static const char *UI_GetGameTypeDisplayName(const char *gameType) {
    for (int i = 0; i < 5; i++)
        if (!strcasecmp(gameType, modes[i][0])) return modes[i][1];
    return gameType;
}
static const char *SEH_LocalizeTextMessage(const char *msg, const char *context, int errType) {
    assert(!strcmp(context, "score panel gametype") && errType == 0);
    for (int i = 0; i < 5; i++)
        if (!strcmp(msg, modes[i][1])) return modes[i][2];
    return msg;
}
static int I_stricmp(const char *a, const char *b) { return strcasecmp(a, b); }
'''

checks = r'''
static int near(float a, float b) { return fabsf(a - b) < 1e-3f; }
static const float cream[3] = { 0.96f, 0.95f, 0.92f }, teal[3] = { 0.22f, 0.59f, 0.59f },
                   orange[3] = { 0.69f, 0.25f, 0.16f }, white[3] = { 1.0f, 1.0f, 1.0f }, black[3] = { 0, 0, 0 };
static int colored(const float *color, const float *rgb, float alpha) {
    return near(color[0], rgb[0]) && near(color[1], rgb[1]) && near(color[2], rgb[2]) && near(color[3], alpha);
}
static int pic(int i, MaterialHandle material, float x, float y, float w, float h, const float *rgb, float alpha) {
    const Drawn *d = &drawn[i];
    return i < drawCount && d->pic && !strcmp(d->material, material) && near(d->x, x) && near(d->y, y) &&
           near(d->w, w) && near(d->h, h) && d->s0 == 0 && d->t0 == 0 && d->s1 == 1 && d->t1 == 1 &&
           d->horz == 1 && d->vert == 3 && colored(d->color, rgb, alpha);
}
static int text(int i, const char *value, float x, float y, float scale, const float *rgb, float alpha) {
    const Drawn *d = &drawn[i];
    return i < drawCount && !d->pic && !strcmp(d->text, value) && near(d->x, x) && near(d->y, y) &&
           near(d->scale, scale) && d->horz == 1 && d->vert == 3 && d->style == 3 && colored(d->color, rgb, alpha);
}
static float width(const char *value, float scale) { return (float)(int)(strlen(value) * 20.0f * scale); }
static hudelem_t make(he_type_t type, float x, float y) {
    hudelem_t e;
    memset(&e, 0, sizeof(e));
    e.type = type; e.x = x; e.y = y; e.alignScreen = (1 << 3) | 1;
    memset(&e.color, 255, sizeof(e.color));
    return e;
}
static int kind(he_type_t type, float x, float y, int label) {
    hudelem_t e = make(type, x, y);
    e.label = label;
    return CG_ScorePanelRetailElem(&e);
}
static void player(int n, int team) { cg->bgs.clientinfo[n].infoValid = 1; cg->bgs.clientinfo[n].team = team; }
static void render(int now) { drawCount = triCount = 0; cg->time = now; CG_DrawScorePanel(); }
static const char *label(int now, float *alpha) { return CG_ScorePanelLabel(&s_scorePanel, now, "Mode", alpha); }
/* tdm.gsc and _hud_teamscore.gsc: the clock and both teams' icons, scores and limits. */
static void stockHud(void) {
    hudelem_t *current = snapState.ps.hud.current, *archival = snapState.ps.hud.archival;
    memset(&snapState.ps.hud, 0, sizeof(snapState.ps.hud));
    current[0] = make(HE_TYPE_VALUE, 36, 26);
    current[1] = make(HE_TYPE_MATERIAL, 6, 28); current[1].materialIndex = 1;
    current[2] = make(HE_TYPE_MATERIAL, 6, 50); current[2].materialIndex = 2;
    current[3] = make(HE_TYPE_TEXT, 320, 240);
    /* Never read: the list ends at its first free element. */
    current[5] = make(HE_TYPE_TIMER_DOWN, 8, 2);
    archival[0] = make(HE_TYPE_TIMER_DOWN, 8, 2);
    archival[0].time = 10000 + 483000;
    byte gold[4] = { 250, 211, 148, 255 };
    memcpy(&archival[0].color, gold, 4);
    archival[1] = make(HE_TYPE_MATERIAL, 6, 28); archival[1].materialIndex = 3;
}
static const float goldRgb[3] = { 250 / 255.0f, 211 / 255.0f, 148 / 255.0f };

int main(void) {
    hudelem_t e;
    vec4_t color;
    float alpha;
    const char *s;
    CG_ScorePanelView view;
    CG_ScorePanelRetail retail;

    cg->snap = &snapState;
    cgs->media.whiteMaterial = "white";
    CG_ScorePanelInit();
    assert(s_scorePanel.standing == SCORE_PANEL_UNSEEN);

    /* The stock clock, icons, scores and limits, by type and position. */
    assert(kind(HE_TYPE_TIMER_DOWN, 8, 2, 0) == SCORE_PANEL_ELEM_CLOCK);
    assert(kind(HE_TYPE_TENTHS_TIMER_UP, 8, 2, 0) == SCORE_PANEL_ELEM_CLOCK);
    assert(kind(HE_TYPE_TIMER_DOWN, 8, 30, 0) == SCORE_PANEL_ELEM_NONE);
    /* sd.gsc's bomb timer is a clock face below. */
    assert(kind(HE_TYPE_CLOCK_DOWN, 8, 2, 0) == SCORE_PANEL_ELEM_NONE);
    assert(kind(HE_TYPE_CLOCK_DOWN, 6, 76, 0) == SCORE_PANEL_ELEM_NONE);
    assert(kind(HE_TYPE_TEXT, 8, 2, 0) == SCORE_PANEL_ELEM_NONE);
    assert(kind(HE_TYPE_MATERIAL, 6, 28, 0) == SCORE_PANEL_ELEM_ICON);
    assert(kind(HE_TYPE_MATERIAL, 6, 50, 0) == SCORE_PANEL_ELEM_ICON);
    /* The CTF flag and HQ radio icons stay. */
    assert(kind(HE_TYPE_MATERIAL, 30, 95, 0) == SCORE_PANEL_ELEM_NONE);
    assert(kind(HE_TYPE_MATERIAL, 6, 76, 0) == SCORE_PANEL_ELEM_NONE);
    assert(kind(HE_TYPE_VALUE, 36, 26, 0) == SCORE_PANEL_ELEM_SCORE);
    assert(kind(HE_TYPE_VALUE, 36, 48, 0) == SCORE_PANEL_ELEM_SCORE);
    assert(kind(HE_TYPE_VALUE, 36, 30, 0) == SCORE_PANEL_ELEM_NONE);
    /* A limit is a slash-labelled value pushed right by its score's digits. */
    assert(kind(HE_TYPE_VALUE, 49, 26, 7) == SCORE_PANEL_ELEM_SCORE);
    assert(kind(HE_TYPE_VALUE, 104, 48, 7) == SCORE_PANEL_ELEM_SCORE);
    assert(kind(HE_TYPE_VALUE, 60, 26, 0) == SCORE_PANEL_ELEM_NONE);
    assert(kind(HE_TYPE_VALUE, 48, 26, 7) == SCORE_PANEL_ELEM_NONE);
    assert(kind(HE_TYPE_VALUE, 105, 26, 7) == SCORE_PANEL_ELEM_NONE);
    e = make(HE_TYPE_TIMER_DOWN, 8, 2);
    e.alignScreen = (2 << 3) | 1;
    assert(CG_ScorePanelRetailElem(&e) == SCORE_PANEL_ELEM_NONE);
    e.alignScreen = (1 << 3) | 1; e.alignOrg = 5;
    assert(CG_ScorePanelRetailElem(&e) == SCORE_PANEL_ELEM_NONE);

    /* They are hidden in the stock gametypes while the panel is on. */
    e = make(HE_TYPE_TIMER_DOWN, 8, 2);
    hudelem_t flag = make(HE_TYPE_MATERIAL, 30, 95);
    static const char *const stock[] = { "dm", "tdm", "sd", "ctf", "hq", "TDM" };
    for (int i = 0; i < 6; i++) {
        snprintf(cgs->gametype, sizeof(cgs->gametype), "%s", stock[i]);
        assert(CG_ScorePanelHidesHudElem(&e) && !CG_ScorePanelHidesHudElem(&flag));
    }
    snprintf(cgs->gametype, sizeof(cgs->gametype), "zom");
    assert(!CG_ScorePanelHidesHudElem(&e));
    snprintf(cgs->gametype, sizeof(cgs->gametype), "tdm");
    panelDvar.current.enabled = 0;
    assert(!CG_ScorePanelHidesHudElem(&e));
    panelDvar.current.enabled = 1;

    /* The stock clock and the first two icons, up to each list's first free slot. */
    stockHud();
    CG_ScorePanelFindRetail(&snapState.ps, &retail);
    assert(retail.clock == &snapState.ps.hud.archival[0] && retail.iconCount == 2);
    assert(retail.icons[0] == &snapState.ps.hud.current[1] && retail.icons[1] == &snapState.ps.hud.current[2]);

    /* The emblem is the viewer's side: German for the Axis, the other icon for
     * the Allies, a deathmatch player's own; the stock icon if no emblem. */
    assert(!strcmp(CG_ScorePanelEmblem(&retail, TEAM_ALLIES), "emblem:hudicon_british"));
    assert(!strcmp(CG_ScorePanelEmblem(&retail, TEAM_AXIS), "emblem:hudicon_german"));
    assert(!strcmp(CG_ScorePanelEmblem(&retail, TEAM_FREE), "emblem:hudicon_british"));
    assert(!CG_ScorePanelEmblem(&retail, TEAM_SPECTATOR));
    snapState.ps.hud.current[1].materialIndex = 4;
    CG_ScorePanelFindRetail(&snapState.ps, &retail);
    assert(!strcmp(CG_ScorePanelEmblem(&retail, TEAM_FREE), "emblem:hudicon_german"));
    assert(!CG_ScorePanelEmblem(&retail, TEAM_ALLIES));
    snapState.ps.hud.current[1].materialIndex = 3;
    CG_ScorePanelFindRetail(&snapState.ps, &retail);
    assert(!strcmp(CG_ScorePanelEmblem(&retail, TEAM_ALLIES), "mat:hudicon_american"));
    snapState.ps.hud.current[1].materialIndex = 0;
    CG_ScorePanelFindRetail(&snapState.ps, &retail);
    assert(!CG_ScorePanelEmblem(&retail, TEAM_ALLIES));

    /* Team modes: the viewer's team on top against the other; teamScores
     * holds the Axis first. Spectators see the Allies on top. */
    serverinfo = "\\sv_hostname\\QA\\ui_tdm_scorelimit\\75\\ui_sd_scorelimit\\10\\g_gametype\\tdm";
    cgs->teamScores[0] = 11; cgs->teamScores[1] = 21;
    player(3, TEAM_ALLIES);
    CG_ScorePanelGetView(&s_scorePanel, SCORE_PANEL_TDM, 3, &view);
    assert(view.topScore == 21 && view.bottomScore == 11 && view.limit == 75 && view.standing == SCORE_PANEL_WINNING);
    assert(view.team == TEAM_ALLIES);
    player(3, TEAM_AXIS);
    CG_ScorePanelGetView(&s_scorePanel, SCORE_PANEL_TDM, 3, &view);
    assert(view.topScore == 11 && view.bottomScore == 21 && view.standing == SCORE_PANEL_LOSING);
    cgs->teamScores[0] = 21;
    CG_ScorePanelGetView(&s_scorePanel, SCORE_PANEL_SD, 3, &view);
    assert(view.standing == SCORE_PANEL_TIED && view.limit == 10);
    cgs->teamScores[0] = 11;
    player(3, TEAM_SPECTATOR);
    CG_ScorePanelGetView(&s_scorePanel, SCORE_PANEL_TDM, 3, &view);
    assert(view.topScore == 21 && view.bottomScore == 11 && view.standing == SCORE_PANEL_NEUTRAL);
    cg->bgs.clientinfo[3].infoValid = 0; cg->bgs.clientinfo[3].team = TEAM_ALLIES;
    CG_ScorePanelGetView(&s_scorePanel, SCORE_PANEL_TDM, 3, &view);
    assert(view.team == TEAM_SPECTATOR && view.standing == SCORE_PANEL_NEUTRAL);
    serverinfo = "\\sv_hostname\\QA";
    CG_ScorePanelGetView(&s_scorePanel, SCORE_PANEL_TDM, 3, &view);
    assert(view.limit == 0);

    /* Deathmatch: a player against the best other player, a spectator sees the
     * two best; spectators' scores do not count and a slot that empties starts
     * again from zero. */
    memset(&cg->bgs, 0, sizeof(cg->bgs));
    serverinfo = "\\ui_dm_scorelimit\\30";
    player(0, TEAM_FREE); player(1, TEAM_FREE); player(2, TEAM_FREE); player(5, TEAM_SPECTATOR);
    CG_ScorePanelSetDmScore(0, 7); CG_ScorePanelSetDmScore(1, 12); CG_ScorePanelSetDmScore(2, 9);
    CG_ScorePanelSetDmScore(5, 50); CG_ScorePanelSetDmScore(6, 40);
    CG_ScorePanelGetView(&s_scorePanel, SCORE_PANEL_DM, 0, &view);
    assert(view.topScore == 7 && view.bottomScore == 12 && view.limit == 30 && view.standing == SCORE_PANEL_LOSING);
    assert(view.team == TEAM_FREE && s_scorePanel.dmScores[6] == 0);
    CG_ScorePanelGetView(&s_scorePanel, SCORE_PANEL_DM, 1, &view);
    assert(view.topScore == 12 && view.bottomScore == 9 && view.standing == SCORE_PANEL_WINNING);
    CG_ScorePanelGetView(&s_scorePanel, SCORE_PANEL_DM, 5, &view);
    assert(view.topScore == 12 && view.bottomScore == 9 && view.standing == SCORE_PANEL_NEUTRAL);
    player(6, TEAM_FREE);
    CG_ScorePanelGetView(&s_scorePanel, SCORE_PANEL_DM, 5, &view);
    assert(view.topScore == 12 && view.bottomScore == 9);
    CG_ScorePanelSetDmScore(1, 3); CG_ScorePanelSetDmScore(2, 15); CG_ScorePanelSetDmScore(6, 10);
    CG_ScorePanelGetView(&s_scorePanel, SCORE_PANEL_DM, 5, &view);
    assert(view.topScore == 15 && view.bottomScore == 10);
    /* Suicides take scores below zero. */
    memset(&cg->bgs, 0, sizeof(cg->bgs));
    player(1, TEAM_FREE); player(2, TEAM_FREE); player(5, TEAM_SPECTATOR);
    CG_ScorePanelSetDmScore(1, 5); CG_ScorePanelSetDmScore(2, -2);
    CG_ScorePanelGetView(&s_scorePanel, SCORE_PANEL_DM, 5, &view);
    assert(view.topScore == 5 && view.bottomScore == -2);
    memset(&cg->bgs, 0, sizeof(cg->bgs));
    player(0, TEAM_FREE);
    CG_ScorePanelSetDmScore(0, 7);
    CG_ScorePanelGetView(&s_scorePanel, SCORE_PANEL_DM, 0, &view);
    assert(view.topScore == 7 && view.bottomScore == 0 && view.standing == SCORE_PANEL_WINNING);
    CG_ScorePanelSetDmScore(64, 5); CG_ScorePanelSetDmScore(-1, 5);
    CG_ScorePanelClearDmScores();
    for (int i = 0; i < 64; i++) assert(s_scorePanel.dmScores[i] == 0);

    /* A new standing shows for five seconds, then fades into the mode name. */
    CG_ScorePanelInit();
    CG_ScorePanelUpdateStanding(&s_scorePanel, SCORE_PANEL_WINNING, 1000);
    assert(!strcmp(label(1000, &alpha), "Winning") && near(alpha, 1));
    assert(!strcmp(label(5750, &alpha), "Winning") && near(alpha, 1));
    assert(!strcmp(label(5875, &alpha), "Winning") && near(alpha, 0.5f));
    assert(!strcmp(label(6000, &alpha), "Mode") && near(alpha, 0));
    assert(!strcmp(label(6125, &alpha), "Mode") && near(alpha, 0.5f));
    assert(!strcmp(label(6250, &alpha), "Mode") && near(alpha, 1));
    assert(!strcmp(label(999, &alpha), "Mode") && near(alpha, 1));
    CG_ScorePanelUpdateStanding(&s_scorePanel, SCORE_PANEL_WINNING, 7000);
    assert(!strcmp(label(7000, &alpha), "Mode"));
    CG_ScorePanelUpdateStanding(&s_scorePanel, SCORE_PANEL_LOSING, 8000);
    assert(!strcmp(label(8000, &alpha), "Losing"));
    CG_ScorePanelUpdateStanding(&s_scorePanel, SCORE_PANEL_TIED, 9000);
    assert(!strcmp(label(9000, &alpha), "Tied"));
    CG_ScorePanelUpdateStanding(&s_scorePanel, SCORE_PANEL_NEUTRAL, 9500);
    assert(!strcmp(label(9500, &alpha), "Mode") && near(alpha, 1));
    CG_ScorePanelUpdateStanding(&s_scorePanel, SCORE_PANEL_WINNING, 9600);
    assert(!strcmp(label(9600, &alpha), "Winning"));

    /* Bars fill toward the limit and always fit their number. */
    assert(near(CG_ScorePanelFill(21, 75, 10), 35) && near(CG_ScorePanelFill(0, 75, 10), 10));
    assert(near(CG_ScorePanelFill(75, 75, 10), 125) && near(CG_ScorePanelFill(100, 75, 10), 125));
    assert(near(CG_ScorePanelFill(5, 0, 12), 12) && near(CG_ScorePanelFill(-3, 75, 10), 10));

    /* The stock timers' text and color as the HUD would draw them. */
    e = make(HE_TYPE_TIMER_UP, 8, 2); e.time = 2000; cg->time = 67000;
    assert(!strcmp(CG_GetHudElemTimer(&e, color), "1:05") && colored(color, white, 1));
    e.type = HE_TYPE_TENTHS_TIMER_DOWN; e.time = 67000 + 12345;
    assert(!strcmp(CG_GetHudElemTimer(&e, color), "0:13.3"));
    byte from[4] = { 0, 0, 0, 0 };
    memcpy(&e.fromColor, from, 4); e.fadeStartTime = 66000; e.fadeTime = 2000;
    e.type = HE_TYPE_TIMER_DOWN;
    assert(CG_GetHudElemTimer(&e, color) && near(color[0], 0.5f) && near(color[3], 0.5f));
    e.type = HE_TYPE_CLOCK_DOWN;
    assert(!CG_GetHudElemTimer(&e, color));
    e.type = HE_TYPE_MATERIAL;
    assert(!CG_GetHudElemTimer(&e, color));

    /* The scoreboard reads the same clock: 8:03 in the stock gold. */
    stockHud();
    cg->time = 10000;
    assert(!strcmp(CG_ScorePanelClock(color), "8:03") && colored(color, goldRgb, 1));
    memset(&snapState.ps.hud.archival[0], 0, sizeof(snapState.ps.hud.archival[0]));
    assert(!CG_ScorePanelClock(color));
    stockHud();
    cg->snap = NULL;
    assert(!CG_ScorePanelClock(color));
    cg->snap = &snapState;

    /* An Allied player winning 21 to 11 toward 75, with 8:03 left. */
    CG_ScorePanelInit();
    snprintf(cgs->gametype, sizeof(cgs->gametype), "tdm");
    serverinfo = "\\ui_tdm_scorelimit\\75";
    cgs->teamScores[0] = 11; cgs->teamScores[1] = 21;
    memset(&cg->bgs, 0, sizeof(cg->bgs));
    player(3, TEAM_ALLIES);
    stockHud();
    snapState.ps.clientNum = 3;
    render(10000);
    float clockX = 172 - width("8:03", 0.44f);
    assert(drawCount == 9);
    assert(pic(0, "emblem:hudicon_british", 8, -55, 34, 34, white, 0.35f));
    assert(text(1, "8:03", clockX, -56, 0.44f, goldRgb, 1));
    assert(text(2, "Winning", 42, -56, 0.26f, cream, 1));
    assert(pic(3, "white", 40, -52, 35, 18, teal, 0.9f) && pic(4, "white", 75, -52, 90, 18, black, 0.4f));
    assert(text(5, "21", 44, -52 + 0.5f * (18 + 26 * 0.38f), 0.38f, cream, 1));
    float fill = 125 * 11 / 75.0f;
    assert(fill > width("11", 0.24f) + 8);
    assert(pic(6, "white", 40, -33, fill, 9, orange, 0.9f) && pic(7, "white", 40 + fill, -33, 125 - fill, 9, black, 0.4f));
    assert(text(8, "11", 44, -33 + 0.5f * (9 + 26 * 0.24f), 0.24f, cream, 1));
    /* The arrow left of the viewer's bar, in physical pixels. */
    assert(triCount == 1 && !strcmp(triMaterial, "white"));
    assert(triIndices[0] == 0 && triIndices[1] == 1 && triIndices[2] == 2);
    assert(near(triVerts[0].xy[0], 79) && near(triVerts[0].xy[1], 607));
    assert(near(triVerts[1].xy[0], 79) && near(triVerts[1].xy[1], 621));
    assert(near(triVerts[2].xy[0], 88) && near(triVerts[2].xy[1], 614));
    for (int i = 0; i < 3; i++) assert(colored(triVerts[i].rgba, cream, 1));
    /* Then the mode name. */
    render(16000);
    assert(text(2, "Team Deathmatch", 42, -56, 0.26f, cream, 1) && text(1, "7:57", clockX, -56, 0.44f, goldRgb, 1));
    /* The Allies overtaken: Losing. */
    cgs->teamScores[0] = 22;
    render(17000);
    assert(text(2, "Losing", 42, -56, 0.26f, cream, 1) && text(5, "21", 44, -52 + 0.5f * (18 + 26 * 0.38f), 0.38f, cream, 1));
    assert(text(8, "22", 44, -33 + 0.5f * (9 + 26 * 0.24f), 0.24f, cream, 1));
    /* A long mode name shrinks to stay clear of the clock. */
    snprintf(cgs->gametype, sizeof(cgs->gametype), "sd");
    serverinfo = "\\ui_sd_scorelimit\\10";
    render(30000);
    float room = 172 - width("7:43", 0.44f) - 6 - 42, sdScale = 0.26f * room / width("Search and Destroy", 0.26f);
    assert(room < width("Search and Destroy", 0.26f));
    assert(text(1, "7:43", 172 - width("7:43", 0.44f), -56, 0.44f, goldRgb, 1));
    assert(text(2, "Search and Destroy", 42, -56, sdScale, cream, 1));
    /* A full bar has no track left; without a clock nothing shrinks. */
    assert(pic(3, "white", 40, -52, 125, 18, teal, 0.9f) && text(4, "21", 44, -52 + 0.5f * (18 + 26 * 0.38f), 0.38f, cream, 1));
    memset(&snapState.ps.hud.archival[0].color, 0, 4);
    render(40000);
    assert(text(1, "Search and Destroy", 42, -56, 0.26f, cream, 1) && drawCount == 6);

    /* A spectator: no emblem, no arrow, the mode name and the Allies on top. */
    stockHud();
    snprintf(cgs->gametype, sizeof(cgs->gametype), "tdm");
    serverinfo = "\\ui_tdm_scorelimit\\75";
    player(3, TEAM_SPECTATOR);
    render(41000);
    assert(drawCount == 8 && triCount == 0 && !drawn[0].pic);
    assert(text(1, "Team Deathmatch", 42, -56, 0.26f, cream, 1) && text(4, "21", 44, -52 + 0.5f * (18 + 26 * 0.38f), 0.38f, cream, 1));
    assert(text(7, "22", 44, -33 + 0.5f * (9 + 26 * 0.24f), 0.24f, cream, 1));

    /* Deathmatch, first seen: the player's own icon and Tied at zero. */
    CG_ScorePanelInit();
    snprintf(cgs->gametype, sizeof(cgs->gametype), "dm");
    serverinfo = "\\ui_dm_scorelimit\\30";
    memset(&cg->bgs, 0, sizeof(cg->bgs));
    player(3, TEAM_FREE); player(1, TEAM_FREE);
    snapState.ps.hud.current[1].materialIndex = 2;
    snapState.ps.hud.current[2].type = HE_TYPE_FREE;
    render(50000);
    assert(pic(0, "emblem:hudicon_german", 8, -55, 34, 34, white, 0.35f) && text(2, "Tied", 42, -56, 0.26f, cream, 1));
    assert(triCount == 1);
    CG_ScorePanelSetDmScore(1, 20); CG_ScorePanelSetDmScore(3, 12);
    render(51000);
    assert(text(2, "Losing", 42, -56, 0.26f, cream, 1) && text(5, "12", 44, -52 + 0.5f * (18 + 26 * 0.38f), 0.38f, cream, 1));
    assert(pic(3, "white", 40, -52, 50, 18, teal, 0.9f) && text(8, "20", 44, -33 + 0.5f * (9 + 26 * 0.24f), 0.24f, cream, 1));

    /* Nothing without the dvar, outside the stock gametypes, without a
     * snapshot or for an impossible client. */
    panelDvar.current.enabled = 0; render(52000); assert(drawCount == 0 && triCount == 0);
    panelDvar.current.enabled = 1;
    snprintf(cgs->gametype, sizeof(cgs->gametype), "zom"); render(52000); assert(drawCount == 0);
    snprintf(cgs->gametype, sizeof(cgs->gametype), "dm");
    cg->snap = NULL; render(52000); assert(drawCount == 0);
    cg->snap = &snapState; snapState.ps.clientNum = 64; render(52000); assert(drawCount == 0);
    snapState.ps.clientNum = 3; render(52000); assert(drawCount == 9);
    return 0;
}
'''

mutants = [
    ('return elem->x == 8.0f && elem->y == 2.0f ?', 'return elem->x == 8.0f ?'),
    ('(elem->y == 28.0f || elem->y == 50.0f)', '(elem->y == 28.0f)'),
    ('(elem->label && elem->x >= 49.0f', '(elem->x >= 49.0f'),
    ('elem->x <= 104.0f', 'elem->x <= 120.0f'),
    (' || elem->alignOrg != 0)', ')'),
    ('i < count && lists[list][i].type != HE_TYPE_FREE', 'i < count'),
    ('german != (team == TEAM_AXIS)', 'german == (team == TEAM_AXIS)'),
    ('!CG_ServerMaterialName(retail->icons[i]->materialIndex, name, sizeof(name)) ||\n            strncmp(name, "hudicon_", 8)',
     '!CG_ServerMaterialName(retail->icons[i]->materialIndex, name, sizeof(name))'),
    ('return emblem ? emblem : icon;', 'return emblem;'),
    ('int own = view->team == TEAM_AXIS ? 0 : 1;', 'int own = view->team == TEAM_AXIS ? 1 : 0;'),
    ('        view->topScore = cgs->teamScores[1];\n        view->bottomScore = cgs->teamScores[0];',
     '        view->topScore = cgs->teamScores[0];\n        view->bottomScore = cgs->teamScores[1];'),
    ('            state->dmScores[i] = 0;\n', ''),
    ('if (i == ownClient || info->team == TEAM_SPECTATOR)', 'if (i == ownClient)'),
    ('} else if (count == 1 || score > second) {', '} else if (score > second) {'),
    ('state->showStanding = standing != SCORE_PANEL_NEUTRAL;', 'state->showStanding = 1;'),
    ('            *alpha = (float)(SCORE_PANEL_STANDING_MSEC - elapsed) / (float)SCORE_PANEL_LABEL_FADE_MSEC;\n', ''),
    ('if (!state->showStanding || elapsed < 0)', 'if (!state->showStanding)'),
    ('    if (width > SCORE_PANEL_BAR_WIDTH)\n        width = SCORE_PANEL_BAR_WIDTH;\n', ''),
    ('if (limit > 0 && score > 0)', 'if (score > 0)'),
    ('    if (view->standing != SCORE_PANEL_NEUTRAL)\n        CG_ScorePanelDrawArrow();', '    CG_ScorePanelDrawArrow();'),
    ('        labelScale *= room / labelWidth;\n', ''),
    ('if (clock && color[3] > 0.0f) {', 'if (clock) {'),
    ('y + 0.5f * (height + SCORE_PANEL_DIGIT_HEIGHT * scale)', 'y + 0.5f * height'),
    ('return CG_ScorePanelActiveGametype() >= 0 && CG_ScorePanelRetailElem(elem)', 'return CG_ScorePanelRetailElem(elem)'),
    ('if (!cg_hudScorePanel || !cg_hudScorePanel->current.enabled)', 'if (!cg_hudScorePanel)'),
    ('    s_scorePanel.standing = SCORE_PANEL_UNSEEN;\n', ''),
    ('    if ((unsigned)viewClient >= SCORE_PANEL_MAX_CLIENTS)\n        return;\n', ''),
    ('clockLeft -= (float)UI_TextWidth(', 'clockLeft = SCORE_PANEL_CLOCK_RIGHT + 0.0f * (float)UI_TextWidth('),
    ('    if (fill < SCORE_PANEL_BAR_WIDTH)\n', ''),
]
timer_mutants = [
    ('elem->type >= HE_TYPE_TENTHS_TIMER_DOWN', '0'),
    ('elem->type > HE_TYPE_TENTHS_TIMER_UP', 'elem->type > HE_TYPE_CLOCK_UP'),
    ('    CG_GetHudElemColor(elem, color);\n    return CG_FormatTimerString', '    return CG_FormatTimerString'),
]

variants = [(panel, timer)]
for old, new in mutants:
    assert panel.count(old) == 1, old
    variants.append((panel.replace(old, new), timer))
for old, new in timer_mutants:
    assert timer.count(old) == 1, old
    variants.append((panel, timer.replace(old, new)))

with tempfile.TemporaryDirectory(prefix='cod2-score-panel-') as directory:
    path = Path(directory) / 'panel.c'
    binary = Path(directory) / 'panel'
    for index, (panel_variant, timer_variant) in enumerate(variants):
        path.write_text(support + timer_variant + panel_variant + checks)
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wno-unused-function', str(path), '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
print('PASS: the panel hides only the stock clock, icons, scores and limits, in the stock gametypes and while '
      'cg_hudScorePanel is on; it puts the viewer\'s team or a deathmatch player on top against the other team or '
      'the best other player (spectators see the Allies or the two leaders), shows a new standing for five seconds '
      'before fading into the mode name, fills bars toward the serverinfo score limit, keeps the stock clock text '
      'and color, shrinks long mode names clear of it, and draws a faint emblem and an arrow only for a side; '
      f'{len(variants) - 1} failing regression mutants')
