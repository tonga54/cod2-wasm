#!/usr/bin/env python3
"""Run what the pause menu reads of the match: the production overview
module (the mode and map, both sides' scores, the viewer's battle stats, the
clock, the teams' free slots, the emblems it builds and the score requests),
and the whole map it draws, with the void clear but for a glow, fitted and
turned in its box with the viewer on it."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
minimap = (root / 'src/PC/cgame_mp/cg_minimap_mp.c').read_text()
overview = (root / 'src/PC/cgame_mp/cg_overview_mp.c').read_text()
header = (root / 'src/headers/PC/gfx_d3d/r_hud2d.h').read_text()
names = (root / 'src/PC/gfx_d3d/r_hud2d.c').read_text()
panel_header = (root / 'src/headers/PC/cgame_mp/cg_scorepanel.h').read_text()


def function(source, name):
    match = re.search(r'^((?:static )?[^\n;]*\b' + name + r'\([^;]*?\))\s*\{', source, re.M)
    assert match, name
    brace = source.index('{', match.end() - 1)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


# The playable area reads the walk off the queue before the specks reuse it;
# the overview image is built from it with the minimap, before the class
# grid is freed, uploaded beside it and forgotten with it; the menu draws its
# bounds fitted.
generate = function(minimap, 'CG_MinimapGenerate')
assert generate.index('reached = CG_MinimapClassify(') < \
    generate.index('CG_MinimapPlayableArea(classes, layout.width, layout.height, reached, queue, area);') < \
    generate.index('CG_MinimapRemoveSpecks(classes, layout.width') < \
    generate.index('CG_MinimapColorize(heights, flags, classes,') < \
    generate.index('CG_MinimapOpenArea(area, layout.width, layout.height, MINIMAP_OVERVIEW_OPENING / layout.texelSize, heights);') < \
    generate.index('CG_MinimapOverviewImage(pixels, area, layout.width, layout.height, glow, heights, overview);') < \
    generate.index('CG_MinimapOverviewBounds(area, &layout, glow, bounds);') < generate.index('free(classes);')
assert 's_minimap.overviewPixels = overview;' in generate and 'free(overview);' in generate and 'free(area);' in generate
assert 'memcpy(s_minimap.overviewBounds, bounds, sizeof(bounds));' in generate
assert 'R_CreateHudImageMaterial(R_HUD_IMAGE_MINIMAP_OVERVIEW, s_minimap.overviewPixels,' in function(minimap, 'CG_MinimapUpload')
assert 's_minimap.overviewMaterial = NULL;' in function(minimap, 'CG_MinimapInit')
assert 'return tail;' in function(minimap, 'CG_MinimapClassify')
draw = function(minimap, 'CG_DrawMinimapOverview')
assert draw.index('CG_MinimapOverviewFit(layout, s_minimap.overviewBounds, x, y, w, h, &overview);') < \
    draw.index('R_AddCmdDrawTriangles2D(verts, 4, quad, 6, s_minimap.overviewMaterial);')
assert 'CG_MinimapPrepare()' in draw and 'CG_MinimapIsActive' not in draw
slots = {name: int(value) for name, value in re.findall(r'^#define (R_HUD_IMAGE_\w+) (\d+)$', header, re.M)}
image_names = re.findall(r'"([^"]+)"', re.search(r's_hudImageNames\[R_HUD_IMAGE_COUNT\] = \{(.*?)\};', names, re.S)[1])
assert image_names[slots['R_HUD_IMAGE_MINIMAP_OVERVIEW']] == '*hud_minimap_overview'
assert image_names[slots['R_HUD_IMAGE_PAUSE_EMBLEMS']] == '*menu_pause_emblems'

definitions = minimap[minimap.index('#define MINIMAP_TEXTURE_MAX'):minimap.index('static const dvar_t *cg_minimap;')]
overview_type = minimap[minimap.index('typedef struct MinimapOverview {'):minimap.index('} MinimapOverview;') + 18]
map_body = definitions + overview_type + '\n' + ''.join(function(minimap, name) for name in (
    'CG_MinimapSpreadArea', 'CG_MinimapPlayableArea', 'CG_MinimapClamp', 'CG_MinimapPutTexel', 'CG_MinimapSetVertex',
    'CG_MinimapChamfer', 'CG_MinimapOpenArea', 'CG_MinimapOverviewImage', 'CG_MinimapOverviewBounds', 'CG_MinimapOverviewFit', 'CG_MinimapOverviewPoint',
    'CG_MinimapOverviewAngle', 'CG_MinimapOverviewQuad'))

module = overview
for include in ('"common_types.h"', '"imports.h"', '"headers/PC/cgame_mp/cg_local.h"',
                '"headers/PC/cgame_mp/cg_scorepanel.h"', '"headers/PC/cgame_mp/cg_scoretable.h"',
                '"headers/PC/gfx_d3d/r_hud2d.h"'):
    assert module.count('#include ' + include + '\n') == 1, include
    module = module.replace('#include ' + include + '\n', '')
neutral = re.search(r'^#define CG_SCORE_PANEL_NEUTRAL (\d+)$', panel_header, re.M).group(1)

support = r'''
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
typedef unsigned char byte;
typedef float vec_t;
typedef vec_t vec4_t[4];
typedef struct Material { const char *name; } Material;
typedef Material *MaterialHandle;
typedef struct { unsigned short id, byteCount; } GfxCmdHeader;
typedef union { unsigned int packed; byte array[4]; } GfxColor;
''' + header + r'''
enum { TEAM_FREE, TEAM_AXIS, TEAM_ALLIES, TEAM_SPECTATOR };
#define CG_SCORE_PANEL_NEUTRAL ''' + neutral + r'''
typedef struct { int infoValid; int team; } clientInfo_t;
typedef struct { int client; int score; int deaths; } score_t;
typedef struct { struct { int clientNum; } ps; } snapshot_t;
static snapshot_t snapState;
static struct { snapshot_t *snap; int time; int scoresRequestTime; int numScores; score_t scores[64];
    struct { clientInfo_t clientinfo[64]; } bgs; } cgState;
static struct { char gametype[32]; } cgsState;
#define cg (&cgState)
#define cgs (&cgsState)
#include "headers/PC/cgame_mp/cg_overview.h"

static const char *serverinfo = "\\sv_maxclients\\64";
const char *CL_GetConfigString(int index) { assert(index == 0); return serverinfo; }
const char *Info_ValueForKey(const char *s, const char *key) {
    static char value[64];
    char pattern[64];
    const char *at;
    snprintf(pattern, sizeof(pattern), "\\%s\\", key);
    at = strstr(s, pattern);
    value[0] = '\0';
    if (at) { at += strlen(pattern); size_t n = strcspn(at, "\\"); memcpy(value, at, n); value[n] = '\0'; }
    return value;
}
const char *UI_GetGameTypeDisplayName(const char *gameType) { return !strcasecmp(gameType, "tdm") ? "MPUI_TDM" : gameType; }
const char *SEH_LocalizeTextMessage(const char *msg, const char *context, int errType) {
    assert(!strcmp(context, "pause menu gametype display") && errType == 0);
    return !strcmp(msg, "MPUI_TDM") ? "Team Deathmatch" : msg;
}
static int reliable;
void CL_AddReliableCommand(const char *cmd) { assert(!strcmp(cmd, "score")); reliable++; }
/* 1.5 px a unit. */
static float pixelsPerUnit = 1.5f;
void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign) {
    assert(horzAlign == 1 && vertAlign == 1);
    *x *= pixelsPerUnit; *y *= pixelsPerUnit; *w *= pixelsPerUnit; *h *= pixelsPerUnit;
}
void I_strncpyz(char *dest, const char *src, int size) { strncpy(dest, src, size - 1); dest[size - 1] = 0; }
int Com_sprintf(char *dest, int size, const char *fmt, ...) { va_list ap; va_start(ap, fmt); int n = vsnprintf(dest, size, fmt, ap); va_end(ap); return n; }
static int generation;
int R_GetHudImageGeneration(void) { return generation; }
static Material emblemMaterial = { "emblems" };
static int builds, buildWidth, buildHeight, failBuild;
static byte built[2 * 512 * 512 * 4];
MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height) {
    assert(slot == R_HUD_IMAGE_PAUSE_EMBLEMS && width == 2 * height && width * height * 4 <= (int)sizeof(built));
    builds++; buildWidth = width; buildHeight = height; memcpy(built, bgra, (size_t)width * height * 4);
    return failBuild ? NULL : &emblemMaterial;
}
/* An emblem of kind k fills its texels with alpha k + 1, to tell them apart. */
void CG_ScoreTableRasterizeEmblem(int kind, byte *bgra, int stride, int size) {
    for (int y = 0; y < size; y++) for (int x = 0; x < size; x++) {
        byte *t = bgra + y * stride + x * 4; t[0] = t[1] = t[2] = 255; t[3] = (byte)(kind + 1);
    }
}
static int alliesKind = 0;
int CG_ScoreTableEmblemKind(int team) { return team == TEAM_AXIS ? 3 : team == TEAM_ALLIES ? alliesKind : -1; }
void CG_ScoreTableMapName(char *buf, int size) { I_strncpyz(buf, "Toujane", size); }
static int freeForAll;
int CG_ScoreTableFreeForAll(void) { return freeForAll; }
static int kills[64];
int CG_ScoreTableKills(int clientNum) { return clientNum >= 0 && clientNum < 64 ? kills[clientNum] : -1; }
static int ownAssists = -1, ownHeadshots = -1;
void CG_ScoreTableOwnStats(int *assists, int *headshots) { *assists = ownAssists; *headshots = ownHeadshots; }
static int panelKnows = 1, panelTop = 40, panelBottom = 25, panelLimit = 100, panelStanding = 1;
int CG_ScorePanelMatch(int *topScore, int *bottomScore, int *limit, int *standing) {
    if (!panelKnows) return 0;
    *topScore = panelTop; *bottomScore = panelBottom; *limit = panelLimit; *standing = panelStanding; return 1;
}
static const char *clockText = "9:42";
static float clockAlpha = 1.0f;
const char *CG_ScorePanelClock(vec4_t color) {
    color[0] = 1.0f; color[1] = 0.8f; color[2] = 0.3f; color[3] = clockAlpha; return clockText;
}
'''

map_checks = r'''
static void near(float a, float b, float eps) { if (!(fabsf(a - b) <= eps)) { fprintf(stderr, "%.5f != %.5f\n", a, b); abort(); } }
static void mapChecks(void) {
    /* The playable area: the ground walked on, the buildings joined to it
     * (through each other too), what they close in; not the land beyond,
     * nor a building standing apart. '#' void, '.' ground not walked on,
     * 'r' walked on, 'B' building, 'c' a closed courtyard. */
    static const char *const plan[10] = {
        "############",
        "#..........#",
        "#..rrrrrr.B#",
        "#..rBBBBrBB#",
        "#..rBccBr..#",
        "#B.rBccBr..#",
        "#..rBBBBr..#",
        "#..rrrrrr..#",
        "#..........#",
        "############",
    };
    enum { PW = 12, PH = 10 };
    static byte plot[PW * PH], area[PW * PH];
    static int walk[PW * PH];
    int reachedCount = 0, inside = 0;
    for (int i = 0; i < PW * PH; i++) {
        char c = plan[i / PW][i % PW];
        plot[i] = c == '#' ? MINIMAP_CLASS_VOID : c == 'B' ? MINIMAP_CLASS_BUILDING : MINIMAP_CLASS_GROUND;
        if (c == 'r') walk[reachedCount++] = i;
    }
    CG_MinimapPlayableArea(plot, PW, PH, reachedCount, walk, area);
    for (int i = 0; i < PW * PH; i++) {
        int x = i % PW, y = i / PW;
        char c = plan[y][x];
        int expected = c == 'r' || c == 'c' || (c == 'B' && !(x == 1 && y == 5));
        if (area[i] != expected) { fprintf(stderr, "area at %d,%d is %d\n", x, y, area[i]); abort(); }
        inside += area[i];
    }
    assert(inside == 39);
    /* Its texels and room for the glow, within the used ones. */
    MinimapLayout plotLayout = { 0.0f, 0.0f, 8.0f, PW, PH, PW, PH };
    int bounds[4];
    CG_MinimapOverviewBounds(area, &plotLayout, 1, bounds);
    assert(bounds[0] == 2 && bounds[1] == 1 && bounds[2] == 12 && bounds[3] == 9);
    CG_MinimapOverviewBounds(area, &plotLayout, 3, bounds);
    assert(bounds[0] == 0 && bounds[1] == 0 && bounds[2] == 12 && bounds[3] == 10);
    /* A walk that reached too little of the map shows all of it. */
    for (int i = 0; i < 7; i++) walk[i] = 8 * PW + 2 + i;
    CG_MinimapPlayableArea(plot, PW, PH, 7, walk, area);
    for (int i = 0; i < PW * PH; i++) assert(area[i] == (plot[i] != MINIMAP_CLASS_VOID));
    CG_MinimapPlayableArea(plot, PW, PH, 0, walk, area);
    assert(area[PW + 1] == 1 && area[0] == 0);
    memset(area, 0, sizeof(area));
    CG_MinimapOverviewBounds(area, &plotLayout, 2, bounds);
    assert(bounds[0] == 0 && bounds[1] == 0 && bounds[2] == 12 && bounds[3] == 10);

    /* A fence a texel thick running out of a block goes, the block stays
     * but for its corners, and nothing is added. */
    enum { OW = 24, OH = 16 };
    static byte opened[OW * OH], before[OW * OH];
    static float scratch[OW * OH];
    for (int i = 0; i < OW * OH; i++) {
        int x = i % OW, y = i / OW;
        before[i] = opened[i] = (x >= 2 && x < 12 && y >= 3 && y < 13) || (y == 7 && x >= 12 && x < 22);
    }
    CG_MinimapOpenArea(opened, OW, OH, 2.0f, scratch);
    for (int x = 13; x < 22; x++) assert(!opened[7 * OW + x]);
    for (int y = 4; y < 12; y++) for (int x = 3; x < 11; x++) assert(opened[y * OW + x]);
    assert(opened[3 * OW + 7] && opened[12 * OW + 7] && opened[8 * OW + 2] && opened[8 * OW + 11] && !opened[3 * OW + 2]);
    for (int i = 0; i < OW * OH; i++) assert(!opened[i] || before[i]);

    /* Inside, the minimap's texels; the rest clear but for a glow fading
     * over its texels, by its chamfer distance. */
    enum { W = 32, H = 24 };
    static byte classes[W * H], inArea[W * H], pixels[W * H * 4], out[W * H * 4];
    static float dist[W * H];
    for (int i = 0; i < W * H; i++) {
        int x = i % W, y = i / W;
        classes[i] = x >= 10 && x < 14 && y >= 8 && y < 12 ? MINIMAP_CLASS_GROUND : MINIMAP_CLASS_VOID;
        inArea[i] = classes[i] != MINIMAP_CLASS_VOID;
        pixels[4 * i] = (byte)x; pixels[4 * i + 1] = (byte)y; pixels[4 * i + 2] = 7; pixels[4 * i + 3] = 255;
    }
    for (int pass = 0; pass < 2; pass++) {
        float glowTexels = pass ? 4.0f : 8.0f;
        CG_MinimapOverviewImage(pixels, inArea, W, H, (int)glowTexels, dist, out);
        for (int i = 0; i < W * H; i++) {
            int x = i % W, y = i / W;
            if (inArea[i]) { assert(!memcmp(out + 4 * i, pixels + 4 * i, 4)); continue; }
            float dx = x < 10 ? 10 - x : x > 13 ? x - 13 : 0, dy = y < 8 ? 8 - y : y > 11 ? y - 11 : 0;
            float d = fminf(dx, dy) * 1.41421356f + fabsf(dx - dy);
            float glow = d < glowTexels ? (1.0f - d / glowTexels) * (1.0f - d / glowTexels) * 0.32f : 0.0f;
            near((float)out[4 * i + 3], glow * 255.0f, 0.6f);
            assert(out[4 * i + 2] == 196 && out[4 * i + 1] == 190 && out[4 * i] == 172);
        }
        assert(out[(8 * W + 9) * 4 + 3] > out[(8 * W + 8) * 4 + 3] && out[(0 * W + 0) * 4 + 3] == 0);
    }

    /* A landscape map turns a quarter into a tall box, a tall one does not;
     * either is centered at its largest. Texels 10 units, the void border included. */
    MinimapLayout layout = { -500.0f, 300.0f, 10.0f, 128, 64, 100, 60 };
    const int full[4] = { 0, 0, 100, 60 }, part[4] = { 10, 5, 60, 45 };
    MinimapOverview ov;
    HudVertex2D quad[4];
    const float rgba[4] = { 1, 1, 1, 1 };
    float x, y;
    CG_MinimapOverviewFit(&layout, full, 600.0f, 120.0f, 280.0f, 480.0f, &ov);
    assert(ov.turned);
    near(ov.scale, 0.4666667f, 1e-5f); near(ov.width, 280.0f, 1e-3f); near(ov.height, 466.6667f, 1e-2f);
    near(ov.x, 600.0f, 1e-3f); near(ov.y, 126.6667f, 1e-2f);
    /* North is to the right: the north-west corner at the top right, east down. */
    CG_MinimapOverviewPoint(&layout, &ov, -500.0f, 300.0f, &x, &y); near(x, 880.0f, 1e-2f); near(y, 126.6667f, 1e-2f);
    CG_MinimapOverviewPoint(&layout, &ov, 500.0f, 300.0f, &x, &y); near(x, 880.0f, 1e-2f); near(y, 593.3333f, 1e-2f);
    CG_MinimapOverviewPoint(&layout, &ov, -500.0f, -300.0f, &x, &y); near(x, 600.0f, 1e-2f);
    near(CG_MinimapOverviewAngle(&ov, 90.0f), 90.0f, 1e-4f);
    near(CG_MinimapOverviewAngle(&ov, 0.0f), 180.0f, 1e-4f);
    CG_MinimapOverviewQuad(&layout, &ov, rgba, quad);
    near(quad[0].st[0], 0.0f, 0); near(quad[0].st[1], 60.0f / 64.0f, 1e-6f);
    near(quad[1].st[0], 0.0f, 0); near(quad[1].st[1], 0.0f, 0);
    near(quad[2].st[0], 100.0f / 128.0f, 1e-6f); near(quad[3].xy[1], 593.3333f, 1e-2f); near(quad[2].xy[0], 880.0f, 1e-2f);
    CG_MinimapOverviewFit(&layout, full, 0.0f, 0.0f, 480.0f, 280.0f, &ov);
    assert(!ov.turned);
    near(ov.scale, 0.4666667f, 1e-5f); near(ov.x, 6.6666667f, 1e-3f); near(ov.y, 0.0f, 1e-3f);
    CG_MinimapOverviewPoint(&layout, &ov, 0.0f, 0.0f, &x, &y); near(x, 6.6666667f + 500.0f * 0.4666667f, 1e-2f);
    near(y, 300.0f * 0.4666667f, 1e-2f);
    near(CG_MinimapOverviewAngle(&ov, 90.0f), 0.0f, 1e-4f); near(CG_MinimapOverviewAngle(&ov, 0.0f), 90.0f, 1e-4f);
    CG_MinimapOverviewQuad(&layout, &ov, rgba, quad);
    near(quad[0].st[1], 0.0f, 0); near(quad[1].st[0], 100.0f / 128.0f, 1e-6f); near(quad[3].st[1], 60.0f / 64.0f, 1e-6f);
    /* Part of the map: its texels from their corner, a unit a pixel here. */
    CG_MinimapOverviewFit(&layout, part, 0.0f, 0.0f, 500.0f, 400.0f, &ov);
    assert(!ov.turned); near(ov.scale, 1.0f, 1e-6f); near(ov.width, 500.0f, 1e-3f); near(ov.height, 400.0f, 1e-3f);
    CG_MinimapOverviewPoint(&layout, &ov, -400.0f, 250.0f, &x, &y); near(x, 0.0f, 1e-3f); near(y, 0.0f, 1e-3f);
    CG_MinimapOverviewPoint(&layout, &ov, -277.0f, 173.0f, &x, &y); near(x, 123.0f, 1e-3f); near(y, 77.0f, 1e-3f);
    CG_MinimapOverviewQuad(&layout, &ov, rgba, quad);
    near(quad[0].st[0], 10.0f / 128.0f, 1e-6f); near(quad[0].st[1], 5.0f / 64.0f, 1e-6f);
    near(quad[2].st[0], 60.0f / 128.0f, 1e-6f); near(quad[2].st[1], 45.0f / 64.0f, 1e-6f);
    CG_MinimapOverviewFit(&layout, part, 0.0f, 0.0f, 400.0f, 500.0f, &ov);
    assert(ov.turned); near(ov.width, 400.0f, 1e-3f); near(ov.height, 500.0f, 1e-3f);
    CG_MinimapOverviewPoint(&layout, &ov, -400.0f, 250.0f, &x, &y); near(x, 400.0f, 1e-3f); near(y, 0.0f, 1e-3f);
    CG_MinimapOverviewPoint(&layout, &ov, -277.0f, 173.0f, &x, &y); near(x, 323.0f, 1e-3f); near(y, 123.0f, 1e-3f);
    CG_MinimapOverviewQuad(&layout, &ov, rgba, quad);
    near(quad[0].st[0], 10.0f / 128.0f, 1e-6f); near(quad[0].st[1], 45.0f / 64.0f, 1e-6f);
    near(quad[1].st[1], 5.0f / 64.0f, 1e-6f); near(quad[2].st[0], 60.0f / 128.0f, 1e-6f);
}
'''

overview_checks = r'''
static void player(int client, int team) { cg->bgs.clientinfo[client].infoValid = 1; cg->bgs.clientinfo[client].team = team; }
static void overviewChecks(void) {
    CG_Overview o;
    CG_TeamSlots s;
    char ratio[16];

    /* Outside a match there is nothing, every stat unknown. */
    assert(!CG_OverviewGet(&o) && o.score == -1 && o.kills == -1 && o.emblem == -1);
    assert(!CG_OverviewTeamSlots(&s) && s.team == TEAM_SPECTATOR);
    CG_OverviewRequestScores(); assert(reliable == 0);
    cg->snap = &snapState; snapState.ps.clientNum = 3; snprintf(cgs->gametype, sizeof(cgs->gametype), "tdm");

    /* The mode and the map, the panel's numbers, the viewer's row of the
     * scores with their kills, their own assists and headshots, the clock. */
    player(3, TEAM_AXIS);
    cg->numScores = 2; cg->scores[0].client = 1; cg->scores[1].client = 3;
    cg->scores[1].score = 450; cg->scores[1].deaths = 2; kills[3] = 5; ownAssists = 3; ownHeadshots = 1;
    assert(CG_OverviewGet(&o));
    assert(!strcmp(o.mode, "Team Deathmatch") && !strcmp(o.map, "Toujane") && !o.freeForAll);
    assert(o.topScore == 40 && o.bottomScore == 25 && o.limit == 100 && o.standing == 1 && o.emblem == 3);
    assert(o.score == 450 && o.deaths == 2 && o.kills == 5 && o.assists == 3 && o.headshots == 1);
    assert(!strcmp(o.clock, "9:42") && o.clockColor[1] == 0.8f && o.clockColor[3] == 1.0f);
    /* A spectator sees the Allies' emblem on top, as on the panel; none in deathmatch. */
    player(3, TEAM_SPECTATOR); assert(CG_OverviewGet(&o) && o.emblem == 0);
    cg->bgs.clientinfo[3].infoValid = 0; alliesKind = 1; assert(CG_OverviewGet(&o) && o.emblem == 1); alliesKind = 0;
    freeForAll = 1; assert(CG_OverviewGet(&o) && o.emblem == -1 && o.freeForAll); freeForAll = 0;
    /* Not yet listed, or from a retail server: unknown. */
    cg->scores[1].client = 9; ownAssists = ownHeadshots = -1;
    assert(CG_OverviewGet(&o) && o.score == -1 && o.kills == -1 && o.deaths == -1 && o.assists == -1);
    cg->scores[1].client = 3; cg->numScores = 1;
    assert(CG_OverviewGet(&o) && o.score == -1);
    cg->numScores = 2;
    /* A clock that is out, or a mode the panel does not know. */
    clockAlpha = 0.0f; assert(CG_OverviewGet(&o) && !o.clock[0]); clockAlpha = 1.0f;
    clockText = NULL; assert(CG_OverviewGet(&o) && !o.clock[0]); clockText = "9:42";
    panelKnows = 0; assert(CG_OverviewGet(&o) && o.standing == CG_SCORE_PANEL_NEUTRAL && o.topScore == 0); panelKnows = 1;

    /* Free slots: half the server for each side, the odd one the Allies';
     * the viewer counts on his own side. */
    memset(&cg->bgs, 0, sizeof(cg->bgs));
    player(0, TEAM_ALLIES); player(1, TEAM_ALLIES); player(2, TEAM_AXIS); player(3, TEAM_ALLIES); player(4, TEAM_SPECTATOR);
    assert(CG_OverviewTeamSlots(&s) && s.team == TEAM_ALLIES && s.allies == 3 && s.axis == 1);
    assert(s.alliesOpen == 29 && s.axisOpen == 31 && s.open == 60 && !s.freeForAll);
    serverinfo = "\\g_gametype\\tdm\\sv_maxclients\\7";
    assert(CG_OverviewTeamSlots(&s) && s.alliesOpen == 1 && s.axisOpen == 2 && s.open == 3);
    serverinfo = "\\sv_maxclients\\5";
    player(5, TEAM_AXIS); player(6, TEAM_AXIS);
    assert(CG_OverviewTeamSlots(&s) && s.alliesOpen == 0 && s.axisOpen == 0 && s.open == 0);
    cg->bgs.clientinfo[5].infoValid = cg->bgs.clientinfo[6].infoValid = 0;
    serverinfo = "\\sv_maxclients\\200";
    assert(CG_OverviewTeamSlots(&s) && s.alliesOpen == 29 && s.axisOpen == 31);
    serverinfo = "\\sv_maxclients\\64";
    player(3, TEAM_SPECTATOR); freeForAll = 1;
    assert(CG_OverviewTeamSlots(&s) && s.team == TEAM_SPECTATOR && s.freeForAll && s.allies == 2); freeForAll = 0;

    /* Scores every two seconds while the menu shows them. */
    cg->time = 10000; cg->scoresRequestTime = 0;
    CG_OverviewRequestScores(); assert(reliable == 1 && cg->scoresRequestTime == 10000);
    cg->time = 11999; CG_OverviewRequestScores(); assert(reliable == 1);
    cg->time = 12001; CG_OverviewRequestScores(); assert(reliable == 2);

    /* The emblems, built once at the size they are drawn: Allies left, Axis right. */
    int allies;
    MaterialHandle m = CG_OverviewEmblems(117.0f, &allies);
    assert(m == &emblemMaterial && allies == 0 && builds == 1 && buildHeight == 176 && buildWidth == 352);
    assert(built[3] == 1 && built[(175 * 352 + 175) * 4 + 3] == 1 && built[176 * 4 + 3] == 4);
    assert(CG_OverviewEmblems(117.0f, &allies) == m && builds == 1);
    alliesKind = 2; assert(CG_OverviewEmblems(117.0f, &allies) == m && allies == 2 && builds == 2 && built[3] == 3);
    generation++; CG_OverviewEmblems(117.0f, &allies); assert(builds == 3);
    pixelsPerUnit = 2.25f; CG_OverviewEmblems(117.0f, &allies); assert(builds == 4 && buildHeight == 263);
    pixelsPerUnit = 10.0f; CG_OverviewEmblems(117.0f, &allies); assert(buildHeight == 512);
    failBuild = 1; generation++; assert(!CG_OverviewEmblems(117.0f, &allies)); failBuild = 0;
    pixelsPerUnit = 1.5f; alliesKind = 0;

    /* K/D: kills over deaths, or the kills alone before the first death. */
    CG_OverviewRatio(7, 2, ratio, sizeof(ratio)); assert(!strcmp(ratio, "3.50"));
    CG_OverviewRatio(5, 0, ratio, sizeof(ratio)); assert(!strcmp(ratio, "5.00"));
    CG_OverviewRatio(-1, -1, ratio, sizeof(ratio)); assert(!strcmp(ratio, "0.00"));
    CG_OverviewRatio(1, 3, ratio, sizeof(ratio)); assert(!strcmp(ratio, "0.33"));
}
int main(void) { mapChecks(); overviewChecks(); puts("ok"); return 0; }
'''

minimap_mutants = [
    ('alpha = alpha > 0.0f ? alpha * alpha * MINIMAP_OVERVIEW_GLOW_ALPHA : 0.0f;',
     'alpha = alpha > 0.0f ? alpha * MINIMAP_OVERVIEW_GLOW_ALPHA : 0.0f;'),
    ('        alpha = 1.0f - dist[i] / (float)glow;', '        alpha = 1.0f - dist[i] / 8.0f;'),
    ('                    *d = fminf(*d, d[-width - 1] + diagonal);\n', ''),
    ('            memcpy(texel, bgra + 4 * i, 4);\n            continue;', '            continue;'),
    ('        area[i] = area[i] ? (dist[i] > radius ? 2 : 1) : 0;', '        area[i] = area[i] ? 2 : 0;'),
    ('        area[i] = area[i] && dist[i] <= radius + 0.5f;', '        area[i] = area[i] != 0;'),
    ('    CG_MinimapSpreadArea(classes, width, height, 1, MINIMAP_AREA_IN, queue, reachedCount, area);\n', ''),
    ('            if (area[next] != MINIMAP_AREA_UNSEEN || (onlyBuildings && classes[next] != MINIMAP_CLASS_BUILDING))',
     '            if (area[next] != MINIMAP_AREA_UNSEEN || classes[next] != MINIMAP_CLASS_BUILDING)'),
    ('            area[next] = value;\n            queue[tail++] = next;', '            area[next] = value;'),
    ('        area[i] = area[i] != MINIMAP_AREA_OUT;', '        area[i] = area[i] == MINIMAP_AREA_IN;'),
    ('    if (inside * MINIMAP_OVERVIEW_MIN_AREA < solid) {', '    if (!inside) {'),
    ('    bounds[0] = x0 - glow > 0 ? x0 - glow : 0;', '    bounds[0] = x0 > 0 ? x0 : 0;'),
    ('    bounds[2] = x1 + glow < layout->usedWidth ? x1 + glow : layout->usedWidth;', '    bounds[2] = x1 + glow;'),
    ('    overview->turned = turnedScale > northScale;', '    overview->turned = 0;'),
    ('    overview->x = x + 0.5f * (w - overview->width);', '    overview->x = x;'),
    ('((float)(overview->texels[3] - overview->texels[1]) * layout->texelSize - south) * overview->scale;',
     '(south) * overview->scale;'),
    ('    float east = worldX - (layout->minX + (float)overview->texels[0] * layout->texelSize);',
     '    float east = worldX - layout->minX;'),
    ('    return 90.0f - yaw + (overview->turned ? 90.0f : 0.0f);', '    return 90.0f - yaw;'),
    ('        CG_MinimapSetVertex(&verts[0], overview->x, overview->y, s0, t1, rgba);',
     '        CG_MinimapSetVertex(&verts[0], overview->x, overview->y, s0, t0, rgba);'),
    ('    float s0 = (float)overview->texels[0] / (float)layout->width;', '    float s0 = 0.0f;'),
]
overview_mutants = [
    ('        overview->kills = CG_ScoreTableKills(client);\n', ''),
    ('    if (!overview->freeForAll)\n        overview->emblem', '    if (1)\n        overview->emblem'),
    ('CG_ScoreTableEmblemKind(team == TEAM_AXIS ? TEAM_AXIS : TEAM_ALLIES)', 'CG_ScoreTableEmblemKind(team)'),
    ('    if (clock && color[3] > 0.0f) {', '    if (clock) {'),
    ('        overview->standing = CG_SCORE_PANEL_NEUTRAL;', '        overview->standing = 0;'),
    ('    slots->alliesOpen = (maxClients + 1) / 2 - slots->allies;', '    slots->alliesOpen = maxClients / 2 - slots->allies;'),
    ('    if (slots->axisOpen < 0)\n        slots->axisOpen = 0;\n', ''),
    ('    if (maxClients > OVERVIEW_MAX_CLIENTS)\n        maxClients = OVERVIEW_MAX_CLIENTS;\n', ''),
    ('    if (!cg->snap || cg->scoresRequestTime + OVERVIEW_SCORES_MSEC >= cg->time)',
     '    if (!cg->snap || cg->scoresRequestTime + 500 >= cg->time)'),
    ('    cg->scoresRequestTime = cg->time;\n', ''),
    ('    if (texels == s_overviewEmblems.size && kind == s_overviewEmblems.allies &&',
     '    if (texels == s_overviewEmblems.size &&'),
    ('        R_GetHudImageGeneration() == s_overviewEmblems.generation)', '        1)'),
    ('    else if (texels > OVERVIEW_EMBLEM_MAX_TEXELS)\n        texels = OVERVIEW_EMBLEM_MAX_TEXELS;\n', ''),
    ('bgra + texels * 4, texels * 8, texels);', 'bgra, texels * 8, texels);'),
    ('deaths > 0 ? (double)kills / (double)deaths : (double)kills', '(double)kills / (double)(deaths + 1)'),
]

with tempfile.TemporaryDirectory(prefix='cod2-pause-overview-') as tmp:
    path = Path(tmp)

    def run(map_source, overview_source, name):
        code = support + map_source + '\n' + overview_source + '\n' + map_checks + overview_checks
        (path / f'{name}.c').write_text(code)
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-I', str(root / 'src'), '-fsanitize=address,undefined',
                        '-fno-sanitize-recover=all', '-Wno-unused-function', str(path / f'{name}.c'), '-lm',
                        '-o', str(path / name)], check=True)
        return subprocess.run([str(path / name)], capture_output=True, text=True)

    result = run(map_body, module, 'overview')
    assert result.returncode == 0 and result.stdout.strip() == 'ok', result.stderr[-3000:]
    for index, (before, after) in enumerate(minimap_mutants):
        assert map_body.count(before) == 1, before
        assert run(map_body.replace(before, after), module, f'map{index}').returncode != 0, after
    for index, (before, after) in enumerate(overview_mutants):
        assert module.count(before) == 1, before
        assert run(map_body, module.replace(before, after), f'overview{index}').returncode != 0, after
print('PASS: the overview reads the mode, the map, the score panel\'s numbers, the viewer\'s scores, kills, '
      'deaths, assists and headshots (unknown until listed) and the clock; free slots split the server between '
      'the sides; scores are asked for every two seconds; the emblems are built once a size, kind and renderer; '
      'the map shows the ground walked on from the spawns with the buildings joined to it and what they close in, '
      'without fences running out of it, the rest clear but for a glow, and fits and turns that part in its box with north and the viewer\'s heading '
      f'turned with it; {len(minimap_mutants) + len(overview_mutants)} '
      'failing regression mutants')
