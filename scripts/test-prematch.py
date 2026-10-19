#!/usr/bin/env python3
"""Run the production WWII countdown before a match on recorded draws: gray view, mode, team and seconds left."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
module = (root / 'src/PC/cgame_mp/cg_prematch_mp.c').read_text()
header = (root / 'src/headers/PC/cgame_mp/cg_prematch.h').read_text()
table_header = (root / 'src/headers/PC/cgame_mp/cg_scoretable.h').read_text()
main = (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()
draw = (root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text()
servercmds = (root / 'src/PC/cgame_mp/cg_servercmds_mp.c').read_text()
scene = (root / 'src/PC/gfx_d3d/r_scene.c').read_text()
hud2d = (root / 'src/PC/gfx_d3d/r_hud2d.c').read_text()
hud2d_header = (root / 'src/headers/PC/gfx_d3d/r_hud2d.h').read_text()
wwii_fonts = (root / 'src/PC/gfx_d3d/r_font_wwii.c').read_text()


def function(text, head):
    assert text.count(head) == 1, head
    start = text.index(head)
    depth, end = 1, text.index('{', start) + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


def ordered(text, *parts):
    at = -1
    for part in parts:
        assert text.count(part) == 1, part
        assert text.index(part) > at, part
        at = text.index(part)


# The cgame starts it with the HUD and takes the server's countdown from its
# client dvar. Every frame, before anything can return, it sets how gray the
# next scene is; for a player waiting in the countdown it stands in for the
# minimap, the menus and the HUD widgets, and the crosshair goes while a scope
# keeps its overlay. The renderer takes the gray each scene, and the team's
# emblem has an image slot of its own.
for source in (main, draw, servercmds):
    assert source.count('#include "headers/PC/cgame_mp/cg_prematch.h"\n') == 1
assert main.count('    CG_EndgameInit();\n    CG_PrematchInit();\n') == 1
assert servercmds.count('    } else if (stricmp(dvarname, "cg_prematch") == 0) {\n'
                        '        CG_PrematchSetCountdown(value);\n') == 1
ordered(function(draw, 'unsigned int CG_Draw2D(void)\n{'),
        '    CG_PrematchFrame();\n    CG_EndgameFrame(',
        '    if (cg->cubemapShot)\n        return 0;\n',
        '    if (!cg_draw2D->current.enabled)\n        return 0;\n',
        '    prematch = CG_PrematchActive();\n',
        '    CG_MinimapSetHudVisible(drawHudMenus && pm_type != 4 && pm_type != 5 && !killcam && !prematch);\n',
        '        if (drawHudMenus && !prematch)\n            Menu_PaintAll((void *)imp_cgDC);\n',
        '        if (drawHudMenus && prematch) {\n            CG_DrawPrematch();\n        } else if (drawHudMenus) {\n'
        '            CG_DrawScorePanel();\n')
ordered(function(draw, 'unsigned int CG_DrawCrosshair(void)\n{'),
        '    reticleVisibility = CG_DrawWeapReticle();\n',
        '    if (!drawHudMenus || CG_PrematchActive())\n        return 0;\n')
ordered(scene, 'extern float g_webGrayFrac;', 'WebSceneFx_SetBackdropGray(g_webBackdropGray);',
        'WebSceneFx_SetGray(g_webGrayFrac);')
assert scene.index('WebSceneFx_SetGray(g_webGrayFrac);') < scene.index('#endif', scene.index('WebSceneFx_SetGray('))
slots = {name: int(value) for name, value in re.findall(r'^#define (R_HUD_IMAGE_\w+) (\d+)$', hud2d_header, re.M)}
names = re.findall(r'"([^"]+)"', re.search(r's_hudImageNames\[R_HUD_IMAGE_COUNT\] = \{(.*?)\};', hud2d, re.S).group(1))
assert len(names) == slots['R_HUD_IMAGE_COUNT'] and len(set(names)) == len(names)
assert names[slots['R_HUD_IMAGE_PREMATCH']] == '*hud_prematch'

# The fonts' capitals and baselines, from their own profiles.
profiles = {name: (int(pixels), float(cap), int(base)) for name, pixels, cap, base in
            re.findall(r'\{ "(\w+)", R_WWII_\w+, (\d+), ([\d.]+)f, (\d+), [\d.]+f, [01] \}', wwii_fonts)}
bold, regular = profiles['boldFont'], profiles['regularFont']

body = module
for include in ('#include "common_types.h"\n', '#include "imports.h"\n', '#include "headers/PC/cgame_mp/cg_local.h"\n',
                '#include "headers/PC/gfx_d3d/r_font_wwii.h"\n', '#include "headers/PC/gfx_d3d/r_hud2d.h"\n'):
    assert body.count(include) == 1, include
    body = body.replace(include, '')
for include, text in (('#include "headers/PC/cgame_mp/cg_prematch.h"\n', header),
                      ('#include "headers/PC/cgame_mp/cg_scoretable.h"\n', table_header)):
    assert body.count(include) == 1, include
    body = body.replace(include, text)

support = r'''
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte;
typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef struct Font_s { const char *name; int pixelHeight; } Font;
typedef Font *FontHandle;
typedef struct Material { const char *name; } Material;
typedef Material *MaterialHandle;
typedef struct { const char *name; union { int enabled; int integer; float value; } current; } dvar_t;
typedef struct HudVertex2D { float xy[2]; float st[2]; float rgba[4]; } HudVertex2D;
enum { TEAM_FREE = 0, TEAM_AXIS = 1, TEAM_ALLIES = 2, TEAM_SPECTATOR = 3 };
#define R_HUD_IMAGE_PREMATCH ''' + str(slots['R_HUD_IMAGE_PREMATCH']) + r'''
#define R_HUD2D_MAX_VERTICES ''' + re.search(r'#define R_HUD2D_MAX_VERTICES (\d+)', hud2d_header).group(1) + r'''
#define R_HUD2D_MAX_INDICES ''' + re.search(r'#define R_HUD2D_MAX_INDICES (\d+)', hud2d_header).group(1) + r'''
typedef struct { int pm_type; int pm_flags; } playerState_t;
typedef struct { playerState_t ps; } snapshot_t;
typedef struct { int infoValid; int team; } clientInfo_t;
typedef struct {
    int clientNum;
    int time;
    snapshot_t *nextSnap;
    char objectiveText[1024];
    struct { clientInfo_t clientinfo[64]; } bgs;
} cg_t;
typedef struct { char gametype[32]; struct { MaterialHandle whiteMaterial; } media; } cgs_t;
static cg_t cgState;
static cgs_t cgsState;
#define cg (&cgState)
#define cgs (&cgsState)
MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height);
void R_AddCmdDrawTriangles2D(const HudVertex2D *verts, int vertexCount, const unsigned short *indices, int indexCount,
                             MaterialHandle material);
FontHandle R_RegisterWwiiFont(const char *fontName);

static Font boldFont = { "$boldfont", ''' + str(bold[0]) + r''' };
static Font regularFont = { "$regularfont", ''' + str(regular[0]) + r''' };
static int regularReady = 1;
static Material white = { "white" }, emblem = { "*hud_prematch" };
static dvar_t prematchDvar = { "cg_hudPrematch" };
static int registrations;
const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags) {
    assert(!strcmp(name, "cg_hudPrematch") && value == 1 && flags == 0x1001);
    ++registrations;
    prematchDvar.current.enabled = value;
    return &prematchDvar;
}
FontHandle CL_RegisterFont(const char *name, int imageTrack) {
    assert(!strcmp(name, "$boldfont") && imageTrack == 0);
    return &boldFont;
}
FontHandle R_RegisterWwiiFont(const char *name) {
    assert(!strcmp(name, "$regularfont"));
    return regularReady ? &regularFont : NULL;
}
float CL_NormalizedTextScale(FontHandle font, float scale) { return 48.0f * scale / (float)font->pixelHeight; }
typedef struct { float cap; float base; } Metrics;
static Metrics metricsOf(FontHandle font) {
    Metrics boldMetrics = { ''' + repr(bold[1]) + r'''f, ''' + str(bold[2]) + r'''.0f };
    Metrics regularMetrics = { ''' + repr(regular[1]) + r'''f, ''' + str(regular[2]) + r'''.0f };
    assert(font == &boldFont || font == &regularFont);
    return font == &boldFont ? boldMetrics : regularMetrics;
}
void CG_ScoreTableFontMetrics(FontHandle font, float *cap, float *baseline) {
    Metrics m = metricsOf(font);
    *cap = m.cap;
    *baseline = m.base;
}
float CG_ScoreTableCapScale(FontHandle font, float cap) {
    return cap * (float)font->pixelHeight / (48.0f * metricsOf(font).cap);
}
/* Every letter advances nine twentieths of the font's pixel height. */
static int advanceOf(FontHandle font) { return font->pixelHeight * 9 / 20; }
float CG_ScoreTableTextWidth(const char *text, int maxChars, FontHandle font, float scale) {
    assert(maxChars == 0);
    return (float)((int)strlen(text) * advanceOf(font)) * CL_NormalizedTextScale(font, scale);
}
static int freeForAll;
int CG_ScoreTableFreeForAll(void) { return freeForAll; }
static const char *mapName = "Sainte Marie du Mont";
void CG_ScoreTableMapName(char *buf, int size) { snprintf(buf, (size_t)size, "%s", mapName); }
enum { AMERICAN, BRITISH, RUSSIAN, GERMAN };
static int alliesKind = AMERICAN;
int CG_ScoreTableEmblemKind(int team) {
    assert(team == TEAM_AXIS || team == TEAM_ALLIES);
    return team == TEAM_AXIS ? GERMAN : alliesKind;
}
/* Each emblem's rows filled with its kind, so the image shows which. */
static int rasters, rasterKind, rasterStride, rasterSize;
void CG_ScoreTableRasterizeEmblem(int kind, byte *bgra, int stride, int size) {
    ++rasters;
    rasterKind = kind;
    rasterStride = stride;
    rasterSize = size;
    for (int y = 0; y < size; ++y)
        memset(bgra + y * stride, kind + 7, (size_t)size * 4);
}
static byte image[128 * 128 * 4];
static int builds, imageSize, failBuild;
MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height) {
    assert(slot == R_HUD_IMAGE_PREMATCH && width == height && width * height * 4 <= (int)sizeof(image));
    memcpy(image, bgra, (size_t)width * (size_t)height * 4);
    imageSize = width;
    ++builds;
    return failBuild ? NULL : &emblem;
}

enum { PHYS, TEXT, TRI };
typedef struct {
    int kind;
    float x, y, w, h, s0, t0, s1, t1, scale;
    float color[4];
    MaterialHandle material;
    FontHandle font;
    char text[160];
    int style, horzAlign, vertAlign, vertexCount, indexCount;
} Draw;
static Draw draws[64];
static HudVertex2D drawVerts[64][R_HUD2D_MAX_VERTICES];
static unsigned short drawIndices[64][R_HUD2D_MAX_INDICES];
static int drawCount;
static Draw *record(int kind, const float *color) {
    Draw *d;
    assert(drawCount < 64);
    d = &draws[drawCount++];
    memset(d, 0, sizeof(*d));
    d->kind = kind;
    if (color)
        memcpy(d->color, color, sizeof(d->color));
    return d;
}
void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign,
                 float scale, const vec_t *color, int style) {
    Draw *d = record(TEXT, color);
    assert(maxChars == 0x7fffffff);
    snprintf(d->text, sizeof(d->text), "%s", text);
    d->font = font;
    d->x = x;
    d->y = y;
    d->horzAlign = horzAlign;
    d->vertAlign = vertAlign;
    d->scale = scale;
    d->style = style;
}
void CL_DrawStretchPicPhysical(float x, float y, float w, float h, float s1, float t1, float s2, float t2,
                               const vec_t *color, MaterialHandle material) {
    Draw *d = record(PHYS, color);
    d->x = x; d->y = y; d->w = w; d->h = h; d->s0 = s1; d->t0 = t1; d->s1 = s2; d->t1 = t2;
    d->material = material;
}
void R_AddCmdDrawTriangles2D(const HudVertex2D *verts, int vertexCount, const unsigned short *indices, int indexCount,
                             MaterialHandle material) {
    Draw *d = record(TRI, verts[0].rgba);
    assert(vertexCount >= 3 && vertexCount <= R_HUD2D_MAX_VERTICES && indexCount >= 3 &&
           indexCount <= R_HUD2D_MAX_INDICES && indexCount % 3 == 0);
    for (int i = 0; i < indexCount; ++i)
        assert(indices[i] < vertexCount);
    memcpy(drawVerts[d - draws], verts, sizeof(HudVertex2D) * (size_t)vertexCount);
    memcpy(drawIndices[d - draws], indices, sizeof(unsigned short) * (size_t)indexCount);
    d->vertexCount = vertexCount;
    d->indexCount = indexCount;
    d->material = material;
}
/* A 1024x576 screen, 1.2 pixels a unit, its safe area the whole screen. */
static float unitScale = 1.2f, screenWidth = 1024.0f, screenHeight = 576.0f;
void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign) {
    assert((horzAlign == 1 || horzAlign == 3) && (vertAlign == 1 || vertAlign == 3));
    *x = *x * unitScale + (horzAlign == 3 ? screenWidth : 0.0f);
    *y = *y * unitScale + (vertAlign == 3 ? screenHeight : 0.0f);
    *w *= unitScale;
    *h *= unitScale;
}
const char *UI_GetGameTypeDisplayName(const char *gameType) {
    static const char *const names[][2] = {
        { "dm", "MPUI_DEATHMATCH" }, { "tdm", "MPUI_TEAM_DEATHMATCH" }, { "sd", "MPUI_SEARCH_AND_DESTROY" },
        { "ctf", "MPUI_CAPTURE_THE_FLAG" }, { "hq", "MPUI_HEADQUARTERS" } };
    for (int i = 0; i < 5; ++i)
        if (!strcmp(gameType, names[i][0]))
            return names[i][1];
    return gameType;
}
const char *SEH_LocalizeTextMessage(const char *msg, const char *context, int errType) {
    static const char *const modes[][2] = {
        { "MPUI_DEATHMATCH", "Deathmatch" }, { "MPUI_TEAM_DEATHMATCH", "Team Deathmatch" },
        { "MPUI_SEARCH_AND_DESTROY", "Search and Destroy" }, { "MPUI_CAPTURE_THE_FLAG", "Capture the Flag" },
        { "MPUI_HEADQUARTERS", "Headquarters" } };
    static const char *const objectives[][2] = {
        { "MP_ELIMINATE_ENEMIES", "Eliminate enemies." }, { "MP_ELIMINATE_THE_ENEMY", "Eliminate the enemy." },
        { "MP_CAPTURE_THE_ENEMY_FLAG", "Capture the enemy flag." },
        { "MP_ESTABLISH_AND_DEFEND", "Establish and defend your Headquarters." },
        { "MP_DESTROY_THE_OBJECTIVE", "Destroy the objective." }, { "MP_DEFEND_THE_OBJECTIVE", "Defend the objective." } };
    assert(errType == 0);
    if (!strcmp(context, "prematch gametype")) {
        for (int i = 0; i < 5; ++i)
            if (!strcmp(msg, modes[i][0]))
                return modes[i][1];
        return msg;
    }
    assert(!strcmp(context, "prematch objective"));
    for (int i = 0; i < 6; ++i)
        if (!strcmp(msg, objectives[i][0]))
            return objectives[i][1];
    assert(0);
    return msg;
}
int I_stricmp(const char *a, const char *b) {
    for (;; ++a, ++b) {
        int x = tolower((unsigned char)*a), y = tolower((unsigned char)*b);
        if (x != y)
            return x < y ? -1 : 1;
        if (!x)
            return 0;
    }
}
int Com_sprintf(char *dest, int size, const char *fmt, ...) {
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsnprintf(dest, (size_t)size, fmt, ap);
    va_end(ap);
    return n;
}
'''

checks = r'''
static int near(float a, float b) { return fabsf(a - b) < 2e-3f; }
static int nearPx(float a, float b) { return fabsf(a - b) < 1e-2f; }
static const float cream[3] = { 0.89f, 0.87f, 0.8f };
static const float light[3] = { 0.93f, 0.92f, 0.88f };
static const float red[3] = { 0.91f, 0.29f, 0.31f };
static const float teal[3] = { 0.27f, 0.6f, 0.56f };
static const float lineRgb[3] = { 0.9f, 0.9f, 0.9f };
static const float black[3] = { 0.0f, 0.0f, 0.0f };
static int colored(const float *color, const float *rgb, float alpha) {
    return near(color[0], rgb[0]) && near(color[1], rgb[1]) && near(color[2], rgb[2]) && near(color[3], alpha);
}
static snapshot_t snap;
static void match(const char *gametype, int team) {
    memset(cg, 0, sizeof(*cg));
    snprintf(cgs->gametype, sizeof(cgs->gametype), "%s", gametype);
    cgs->media.whiteMaterial = &white;
    snap.ps.pm_type = 0;
    snap.ps.pm_flags = 0;
    cg->nextSnap = &snap;
    cg->clientNum = 5;
    cg->bgs.clientinfo[5].infoValid = 1;
    cg->bgs.clientinfo[5].team = team;
    freeForAll = !strcmp(gametype, "dm");
}
static int frame(int time) {
    cg->time = time;
    CG_PrematchFrame();
    drawCount = 0;
    CG_DrawPrematch();
    return drawCount;
}
static int count(int kind) {
    int n = 0;
    for (int i = 0; i < drawCount; ++i)
        n += draws[i].kind == kind;
    return n;
}
static const Draw *text(const char *s) {
    for (int i = 0; i < drawCount; ++i)
        if (draws[i].kind == TEXT && !strcmp(draws[i].text, s))
            return &draws[i];
    return NULL;
}
static int indexOf(const Draw *d) { return d ? (int)(d - draws) : -1; }
/* Where a text's pen, capitals and baseline land by the font's metrics once
 * the renderer moves it half a unit up and to the left. */
static float unitOf(const Draw *d) { return 48.0f * d->scale / (float)d->font->pixelHeight; }
static float penOf(const Draw *d) { return d->x - 0.5f * unitOf(d); }
static float capOf(const Draw *d) { return metricsOf(d->font).cap * unitOf(d); }
static float baselineOf(const Draw *d) { return d->y - (metricsOf(d->font).base + 0.5f) * unitOf(d); }
static float widthOf(const Draw *d) { return (float)((int)strlen(d->text) * advanceOf(d->font)) * unitOf(d); }
/* Shadowed text from x, or centered on it, its capitals cap tall on the
 * baseline, placed from the safe area's corner (horz, vert). */
static int lettered(const Draw *d, float x, float baseline, float cap, FontHandle font, const float *rgb, int centered,
                    int horz, int vert) {
    return d && d->font == font && near(penOf(d) + (centered ? 0.5f * widthOf(d) : 0.0f), x) &&
           near(baselineOf(d), baseline) && near(capOf(d), cap) && d->style == 3 && d->horzAlign == horz &&
           d->vertAlign == vert && colored(d->color, rgb, 1);
}
/* The countdown's middle in pixels: 91.25 units from the right, 193.3 down. */
static float ringX(void) { return screenWidth - 91.25f * unitScale; }
static float ringY(void) { return 193.3f * unitScale; }
/* An arc whose middle is radius pixels from (cx, cy), width pixels across,
 * from twelve o'clock counterclockwise through sweep of a turn in 128ths;
 * its sides fade out over a pixel. */
static const Draw *arc(float cx, float cy, float radius, float width, float sweep, const float *rgb, float alpha) {
    float half = width > 1.0f ? 0.5f * width : 0.5f;
    float radii[4] = { radius - half - 0.5f, radius - half + 0.5f, radius + half - 0.5f, radius + half + 0.5f };
    float core = alpha * (width < 1.0f ? width : 1.0f);
    for (int i = 0; i < drawCount; ++i) {
        const Draw *d = &draws[i];
        const HudVertex2D *verts = drawVerts[i];
        const unsigned short *indices = drawIndices[i];
        int segments = d->vertexCount / 4 - 1, ok;
        if (d->kind != TRI || d->vertexCount % 4)
            continue;
        ok = segments == (int)ceilf(sweep * 128.0f) && d->indexCount == segments * 18 && d->material == &white;
        for (int s = 0; ok && s <= segments; ++s) {
            float angle = 6.28318531f * sweep * (float)s / (float)segments;
            for (int k = 0; ok && k < 4; ++k) {
                const HudVertex2D *v = &verts[s * 4 + k];
                ok = nearPx(v->xy[0], cx - sinf(angle) * radii[k]) && nearPx(v->xy[1], cy - cosf(angle) * radii[k]) &&
                     colored(v->rgba, rgb, k == 1 || k == 2 ? core : 0.0f);
            }
        }
        for (int s = 0; ok && s < segments; ++s) {
            for (int k = 0; ok && k < 3; ++k) {
                const unsigned short *q = &indices[(s * 3 + k) * 6];
                int a = s * 4 + k;
                ok = q[0] == a && q[1] == a + 1 && q[2] == a + 5 && q[3] == a && q[4] == a + 5 && q[5] == a + 4;
            }
        }
        if (ok)
            return d;
    }
    return NULL;
}
/* The same arc behind its shadow, the shadow first. */
static int shadowedArc(float cx, float cy, float radius, float width, float sweep, const float *rgb, float shadow) {
    const Draw *front = arc(cx, cy, radius, width, sweep, rgb, 1.0f);
    const Draw *back = arc(cx + shadow, cy + shadow, radius, width, sweep, black, 0.5f);
    return front && back && indexOf(back) < indexOf(front);
}
/* A disc whose edge fades out over a pixel. */
static const Draw *disc(float cx, float cy, float radius, const float *rgb, float alpha) {
    for (int i = 0; i < drawCount; ++i) {
        const Draw *d = &draws[i];
        const HudVertex2D *verts = drawVerts[i];
        int ok = d->kind == TRI && d->vertexCount == 129 && d->indexCount == 64 * 9 && d->material == &white &&
                 nearPx(verts[0].xy[0], cx) && nearPx(verts[0].xy[1], cy) && colored(verts[0].rgba, rgb, alpha);
        for (int s = 0; ok && s < 64; ++s) {
            float angle = 6.28318531f * (float)s / 64.0f;
            ok = nearPx(verts[1 + 2 * s].xy[0], cx - sinf(angle) * (radius - 0.5f)) &&
                 nearPx(verts[1 + 2 * s].xy[1], cy - cosf(angle) * (radius - 0.5f)) &&
                 nearPx(verts[2 + 2 * s].xy[0], cx - sinf(angle) * (radius + 0.5f)) &&
                 nearPx(verts[2 + 2 * s].xy[1], cy - cosf(angle) * (radius + 0.5f)) &&
                 colored(verts[1 + 2 * s].rgba, rgb, alpha) && colored(verts[2 + 2 * s].rgba, rgb, 0.0f);
        }
        if (ok)
            return d;
    }
    return NULL;
}
static const Draw *rect(float x, float y, float w, float h, const float *rgb, float alpha, MaterialHandle material) {
    for (int i = 0; i < drawCount; ++i) {
        const Draw *d = &draws[i];
        if (d->kind == PHYS && d->x == x && d->y == y && d->w == w && d->h == h && d->material == material &&
            d->s0 == 0 && d->t0 == 0 && d->s1 == 1 && d->t1 == 1 && colored(d->color, rgb, alpha))
            return d;
    }
    return NULL;
}
/* The crossing lines, reach pixels out: whole pixels across, the horizontal
 * one through the block where they cross and the vertical one in halves. */
static int lines(float reach, int *at) {
    float exact = 0.85f * unitScale, pixels = fmaxf(1.0f, floorf(exact + 0.5f));
    float x = floorf(ringX() - 0.5f * pixels + 0.5f), y = floorf(ringY() - 0.5f * pixels + 0.5f);
    float r = floorf(reach - 0.5f * pixels + 0.5f), alpha = 0.3f * (exact < pixels ? exact / pixels : 1.0f);
    const Draw *across = rect(x - r, y, 2 * r + pixels, pixels, lineRgb, alpha, &white);
    const Draw *up = rect(x, y - r, pixels, r, lineRgb, alpha, &white);
    const Draw *down = rect(x, y + pixels, pixels, r, lineRgb, alpha, &white);
    if (at)
        *at = indexOf(across);
    return across && up && down && r >= 1;
}
/* The teal band behind the team's name: solid to 104 units, clear at 140. */
static const Draw *band(void) {
    static const float columns[3] = { 73.3f, 104.0f, 140.0f };
    for (int i = 0; i < drawCount; ++i) {
        const Draw *d = &draws[i];
        const HudVertex2D *verts = drawVerts[i];
        const unsigned short *q = drawIndices[i];
        int ok = d->kind == TRI && d->vertexCount == 6 && d->indexCount == 12 && d->material == &white;
        for (int c = 0; ok && c < 3; ++c) {
            float x = columns[c] * unitScale;
            ok = nearPx(verts[2 * c].xy[0], x) && nearPx(verts[2 * c + 1].xy[0], x) &&
                 nearPx(verts[2 * c].xy[1], screenHeight - (64.6f + 13.75f) * unitScale) &&
                 nearPx(verts[2 * c + 1].xy[1], screenHeight - (64.6f - 13.75f) * unitScale) &&
                 colored(verts[2 * c].rgba, teal, c < 2 ? 0.65f : 0.0f) &&
                 colored(verts[2 * c + 1].rgba, teal, c < 2 ? 0.65f : 0.0f);
        }
        ok = ok && q[0] == 0 && q[1] == 1 && q[2] == 3 && q[3] == 0 && q[4] == 3 && q[5] == 2 && q[6] == 2 && q[7] == 3 &&
             q[8] == 5 && q[9] == 2 && q[10] == 5 && q[11] == 4;
        if (ok)
            return d;
    }
    return NULL;
}
static float shadowPx(void) { return fmaxf(1.0f, floorf(0.8f * unitScale + 0.5f)); }
/* The emblem a texel a pixel on its middle, behind its shadow. */
static int emblemDrawn(void) {
    float size = (float)imageSize;
    float x = floorf(63.3f * unitScale - 0.5f * size + 0.5f);
    float y = floorf(screenHeight - 64.6f * unitScale - 0.5f * size + 0.5f);
    const Draw *back = rect(x + shadowPx(), y + shadowPx(), size, size, black, 0.5f, &emblem);
    const Draw *front = rect(x, y, size, size, cream, 1.0f, &emblem);
    return back && front && indexOf(back) < indexOf(front);
}
static int emblems(void) {
    int n = 0;
    for (int i = 0; i < drawCount; ++i)
        n += draws[i].material == &emblem;
    return n;
}
static const char *teamName(void) { return cg->bgs.clientinfo[5].team == TEAM_AXIS ? "Axis Team" : "Allies Team"; }
/* The whole layout on a frame remaining ms before a 15 s match start. */
static void layout(int remaining, const char *objective) {
    int seconds = (remaining + 999) / 1000, red5 = remaining <= 5000, linesAt = -1;
    float ppu = unitScale, second = (float)(remaining - (seconds - 1) * 1000) / 1000.0f;
    float reach = fminf(1.0f, (float)(remaining - 2000) / 3000.0f);
    char digits[8];
    const Draw *title = text("Team Deathmatch // Sainte Marie du Mont"), *goal = text(objective), *team = text(teamName());
    snprintf(digits, sizeof(digits), "%d", seconds);
    assert(lettered(title, 46.5f, 59.0f, 14.4f, &boldFont, cream, 0, 1, 1));
    assert(lettered(goal, 46.5f, 75.9f, 7.3f, &regularFont, light, 0, 1, 1));
    assert(lettered(team, 89.6f, -62.9f, 7.3f, &boldFont, light, 0, 1, 3));
    assert(band() && emblemDrawn() && indexOf(goal) < indexOf(band()) && indexOf(band()) < indexOf(team));
    if (reach > 0.0f)
        assert(lines(66.0f * reach * ppu, &linesAt) && linesAt > indexOf(team));
    else
        assert(!lines(1.0f, NULL) && count(PHYS) == 2);
    if (red5) {
        assert(disc(ringX(), ringY(), 23.75f * ppu, red, 0.3f));
        assert(shadowedArc(ringX(), ringY(), 23.75f * ppu, 2.4f * ppu, second, red, shadowPx()));
    } else {
        assert(arc(ringX(), ringY(), 23.75f * ppu, 1.2f * ppu, second, cream, 0.45f) && count(TRI) == 4);
    }
    assert(shadowedArc(ringX(), ringY(), 29.2f * ppu, 2.4f * ppu, (float)remaining / 15000.0f, cream, shadowPx()));
    assert(lettered(text(digits), -91.25f, 193.3f + 8.75f, 17.5f, &boldFont, red5 ? red : cream, 1, 3, 1));
    assert(indexOf(text(digits)) == drawCount - 1 && count(TEXT) == 4);
}

int main(void) {
    CG_PrematchInit();
    assert(registrations == 1 && g_webGrayFrac == 0.0f);
    match("tdm", TEAM_ALLIES);

    /* Without the server's countdown the view stays in color and nothing is drawn. */
    assert(!CG_PrematchActive() && !frame(1000) && g_webGrayFrac == 0.0f && !builds);

    /* The capture's Team Deathmatch: the Allies viewer spawned into a 15 s
     * countdown that the server started at 20000. */
    CG_PrematchSetCountdown("20000 35000");
    assert(CG_PrematchActive() && frame(20000) && g_webGrayFrac == 1.0f);
    layout(15000, "Eliminate the enemy.");
    assert(builds == 1 && rasters == 1 && rasterKind == AMERICAN && imageSize == 33 && rasterSize == 33 &&
           rasterStride == 33 * 4 && image[0] == AMERICAN + 7);
    assert(drawCount == 13);
    /* The seconds count down; the outer ring runs out with the countdown, the
     * inner one with each second. */
    assert(frame(24400) && g_webGrayFrac == 1.0f);
    layout(10600, "Eliminate the enemy.");
    assert(frame(25000));
    layout(10000, "Eliminate the enemy.");
    /* The last five turn red over a red disc, and the lines draw in until two
     * seconds before. */
    assert(frame(30000));
    layout(5000, "Eliminate the enemy.");
    assert(frame(30200));
    layout(4800, "Eliminate the enemy.");
    assert(frame(32400));
    layout(2600, "Eliminate the enemy.");
    /* Color comes back over the last two seconds. */
    assert(frame(33000) && g_webGrayFrac == 1.0f);
    assert(frame(33500) && near(g_webGrayFrac, 0.75f));
    layout(1500, "Eliminate the enemy.");
    assert(frame(34000) && near(g_webGrayFrac, 0.5f));
    assert(frame(34999) && near(g_webGrayFrac, 0.0005f));
    layout(1, "Eliminate the enemy.");
    /* Then the match begins: the HUD is back and so is the color. */
    assert(!frame(35000) && !CG_PrematchActive() && g_webGrayFrac == 0.0f);
    assert(!frame(90000) && g_webGrayFrac == 0.0f);
    /* A client behind the server's start sees the whole countdown. */
    assert(frame(19000));
    layout(15000, "Eliminate the enemy.");
    assert(builds == 1 && rasters == 1);

    /* Only a player waiting in it: not a spectator, the dead, someone
     * following another player, before the first snapshot, or with
     * cg_hudPrematch off. A player linked to an entity still waits. */
    static const int types[][3] = { { 1, 0, 1 }, { 4, 0, 0 }, { 5, 0, 0 }, { 6, 0, 0 }, { 7, 0, 0 }, { 0, 0x400000, 0 } };
    for (int i = 0; i < 6; ++i) {
        snap.ps.pm_type = types[i][0];
        snap.ps.pm_flags = types[i][1];
        assert(CG_PrematchActive() == types[i][2] && (frame(26000) > 0) == types[i][2]);
        assert(g_webGrayFrac == (types[i][2] ? 1.0f : 0.0f));
    }
    snap.ps.pm_type = 0;
    snap.ps.pm_flags = 0x8000;
    assert(CG_PrematchActive() && frame(26000));
    snap.ps.pm_flags = 0;
    cg->nextSnap = NULL;
    assert(!CG_PrematchActive() && !frame(26000) && g_webGrayFrac == 0.0f);
    cg->nextSnap = &snap;
    prematchDvar.current.enabled = 0;
    assert(!CG_PrematchActive() && !frame(26000) && g_webGrayFrac == 0.0f);
    prematchDvar.current.enabled = 1;

    /* Only a countdown the server could send: two times, the end after the
     * start, at most two minutes long. */
    static const char *const bad[] = { "", "abc", "26000", "35000 20000", "20000 20000", "0 120001", "x 30000" };
    for (int i = 0; i < 7; ++i) {
        CG_PrematchSetCountdown("20000 35000");
        CG_PrematchSetCountdown(bad[i]);
        assert(!CG_PrematchActive() && !frame(26000) && g_webGrayFrac == 0.0f);
    }
    CG_PrematchSetCountdown(NULL);
    assert(!frame(26000));
    CG_PrematchSetCountdown("0 120000");
    assert(frame(0) && text("120"));
    CG_PrematchSetCountdown("20000 35000");

    /* The Axis viewer reads his own team, its cross built at the same size. */
    cg->bgs.clientinfo[5].team = TEAM_AXIS;
    assert(frame(26000) && text("Axis Team") && !text("Allies Team") && builds == 2 && rasterKind == GERMAN);
    layout(9000, "Eliminate the enemy.");
    /* Players without a side see no team: a spectator's info, a free player,
     * one whose info has not arrived and an invalid client number. */
    static const int sides[] = { TEAM_SPECTATOR, TEAM_FREE };
    for (int i = 0; i < 2; ++i) {
        cg->bgs.clientinfo[5].team = sides[i];
        assert(frame(26000) && !band() && !emblems() && !text("Allies Team") && !text("Axis Team"));
        assert(text("Team Deathmatch // Sainte Marie du Mont") && text("9") && count(TEXT) == 3);
    }
    cg->bgs.clientinfo[5].team = TEAM_ALLIES;
    cg->bgs.clientinfo[5].infoValid = 0;
    assert(frame(26000) && !band() && count(TEXT) == 3);
    cg->bgs.clientinfo[5].infoValid = 1;
    cg->clientNum = 64;
    assert(frame(26000) && !band() && count(TEXT) == 3);
    cg->clientNum = 5;

    /* Every mode reads its retail one-line objective; deathmatch shows no team. */
    match("dm", TEAM_FREE);
    assert(frame(26000) && text("Deathmatch // Sainte Marie du Mont") && text("Eliminate enemies.") && !band());
    match("dm", TEAM_ALLIES);
    assert(frame(26000) && !band() && !emblems());
    match("ctf", TEAM_ALLIES);
    assert(frame(26000) && text("Capture the Flag // Sainte Marie du Mont") && text("Capture the enemy flag.") && band());
    match("HQ", TEAM_AXIS);
    assert(frame(26000) && text("Establish and defend your Headquarters.") && text("Axis Team"));
    /* S&D's defenders read their own, as their spawn's objective says. */
    static const char *const sd[][2] = {
        { "MP_OBJ_ATTACKERS\x15" "10", "Destroy the objective." }, { "MP_OBJ_DEFENDERS\x15" "10", "Defend the objective." },
        { "MP_OBJ_ATTACKERS_NOSCORE", "Destroy the objective." }, { "MP_OBJ_DEFENDERS_NOSCORE", "Defend the objective." },
        { "MP_DESTROY_THE_OBJECTIVE", "Destroy the objective." }, { "MP_DEFEND_THE_OBJECTIVE", "Defend the objective." },
        { "", "Destroy the objective." } };
    for (int i = 0; i < 7; ++i) {
        match("sd", TEAM_ALLIES);
        snprintf(cg->objectiveText, sizeof(cg->objectiveText), "%s", sd[i][0]);
        assert(frame(26000) && text("Search and Destroy // Sainte Marie du Mont") && text(sd[i][1]) && count(TEXT) == 4);
    }
    /* A mod's mode has a title but no objective. */
    match("zom", TEAM_ALLIES);
    assert(frame(26000) && text("zom // Sainte Marie du Mont") && count(TEXT) == 3);

    /* A long title shrinks into 540 units on its baseline. */
    match("tdm", TEAM_ALLIES);
    mapName = "A Very Long Name For A Map Somewhere Far Away In The West";
    assert(frame(26000));
    const Draw *title = text("Team Deathmatch // A Very Long Name For A Map Somewhere Far Away In The West");
    assert(title && near(widthOf(title), 540.0f) && near(penOf(title), 46.5f) && near(baselineOf(title), 59.0f) &&
           capOf(title) < 14.4f);
    mapName = "Sainte Marie du Mont";
    /* Without Regular the objective takes the bold font. */
    regularReady = 0;
    CG_PrematchInit();
    regularReady = 1;
    CG_PrematchSetCountdown("20000 35000");
    assert(frame(26000) && text("Eliminate the enemy.")->font == &boldFont);

    /* The emblem is built again for a new size, within 8 to 128 texels, or a
     * new nation; an Allies nation without one has none; a failed build
     * leaves the band and the name and waits for a change. */
    CG_PrematchInit();
    CG_PrematchSetCountdown("20000 35000");
    builds = rasters = 0;
    assert(frame(26000) && builds == 1 && imageSize == 33);
    assert(frame(26050) && builds == 1);
    unitScale = 2.25f; screenWidth = 1920.0f; screenHeight = 1080.0f;
    assert(frame(26000) && builds == 2 && imageSize == 59);
    layout(9000, "Eliminate the enemy.");
    /* On a small screen the faint ring and the lines are under a pixel wide. */
    unitScale = 0.5f; screenWidth = 640.0f; screenHeight = 360.0f;
    assert(frame(26000) && builds == 3 && imageSize == 15);
    layout(9000, "Eliminate the enemy.");
    unitScale = 10.0f;
    assert(frame(26000) && builds == 4 && imageSize == 128 && emblemDrawn());
    unitScale = 0.1f;
    assert(frame(26000) && builds == 5 && imageSize == 8 && emblemDrawn());
    unitScale = 1.2f; screenWidth = 1024.0f; screenHeight = 576.0f;
    alliesKind = BRITISH;
    assert(frame(26000) && builds == 6 && rasterKind == BRITISH && emblemDrawn());
    alliesKind = -1;
    assert(frame(26000) && builds == 6 && !emblems() && band() && text("Allies Team"));
    alliesKind = AMERICAN;
    failBuild = 1;
    assert(frame(26000) && builds == 7 && !emblems() && band() && text("Allies Team"));
    assert(frame(26050) && builds == 7 && !emblems());
    failBuild = 0;
    unitScale = 1.25f;
    assert(frame(26000) && builds == 8 && emblemDrawn());
    unitScale = 1.2f;

    /* A new map forgets the countdown and leaves the view in color. */
    assert(frame(26000) && g_webGrayFrac == 1.0f);
    CG_PrematchInit();
    assert(registrations == 4 && g_webGrayFrac == 0.0f && !CG_PrematchActive() && !frame(26000));
    puts("PASS");
    return 0;
}
'''


def mutate(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


mutants = [
    ('    if (remaining >= PREMATCH_GRAY_FADE_MSEC)\n        g_webGrayFrac = 1.0f;\n    else\n', '    if (1)\n        g_webGrayFrac = remaining ? 1.0f : 0.0f;\n    else\n'),
    ('#define PREMATCH_GRAY_FADE_MSEC 2000', '#define PREMATCH_GRAY_FADE_MSEC 1000'),
    ('    g_webGrayFrac = 0.0f;\n}', '}'),
    ('#define PREMATCH_RED_MSEC 5000', '#define PREMATCH_RED_MSEC 4000'),
    ('    int red = remaining <= PREMATCH_RED_MSEC;', '    int red = remaining < PREMATCH_RED_MSEC;'),
    ('    float second = (float)(remaining - (seconds - 1) * 1000) / 1000.0f;',
     '    float second = (float)(remaining % 1000) / 1000.0f;'),
    ('                           (float)remaining / (float)length, color);', '                           1.0f, color);'),
    ('        float dx = -sinf(angle);\n        float dy = -cosf(angle);\n        for (k = 0; k < 4; ++k)',
     '        float dx = sinf(angle);\n        float dy = -cosf(angle);\n        for (k = 0; k < 4; ++k)'),
    ('    float half = width > 1.0f ? 0.5f * width : 0.5f;', '    float half = 0.5f * width;'),
    ('    CG_PrematchColor(core, color, color[3] * (width < 1.0f ? width : 1.0f));', '    CG_PrematchColor(core, color, color[3]);'),
    ('            indices[count++] = (unsigned short)(a + 5);\n            indices[count++] = (unsigned short)(a + 4);',
     '            indices[count++] = (unsigned short)(a + 4);\n            indices[count++] = (unsigned short)(a + 5);'),
    ('    segments = (int)ceilf(sweep * PREMATCH_ARC_SEGMENTS);', '    segments = PREMATCH_ARC_SEGMENTS;'),
    ('    if (snap->ps.pm_type > PREMATCH_PLAYING_LINKED || (snap->ps.pm_flags & PREMATCH_FOLLOWING))',
     '    if (snap->ps.pm_type > PREMATCH_PLAYING_LINKED)'),
    ('    if (snap->ps.pm_type > PREMATCH_PLAYING_LINKED || (snap->ps.pm_flags & PREMATCH_FOLLOWING))',
     '    if (snap->ps.pm_type > 0 || (snap->ps.pm_flags & PREMATCH_FOLLOWING))'),
    ('    if (!cg_hudPrematch || !cg_hudPrematch->current.enabled || !snap)', '    if (!snap)'),
    (' || end - start > PREMATCH_MAX_MSEC)', ')'),
    ('    return remaining < length ? remaining : length;', '    return remaining;'),
    ('"%s // %s"', '"%s | %s"'),
    ('        if (!strncmp(text, "MP_OBJ_DEFENDERS", 16) || !strcmp(text, "MP_DEFEND_THE_OBJECTIVE"))',
     '        if (!strcmp(text, "MP_DEFEND_THE_OBJECTIVE"))'),
    ('    { "tdm", "MP_ELIMINATE_THE_ENEMY" },', '    { "tdm", "MP_ELIMINATE_ENEMIES" },'),
    ('    if ((unsigned)cg->clientNum >= 64 || CG_ScoreTableFreeForAll())', '    if ((unsigned)cg->clientNum >= 64)'),
    ('    if (!info->infoValid || (info->team != TEAM_ALLIES && info->team != TEAM_AXIS))',
     '    if (!info->infoValid || info->team == TEAM_SPECTATOR)'),
    ('        CG_PrematchColor(color, s_prematchTeal, i < 2 ? PREMATCH_BAND_ALPHA : 0.0f);',
     '        CG_PrematchColor(color, s_prematchTeal, PREMATCH_BAND_ALPHA);'),
    ('    int size = (int)(PREMATCH_EMBLEM_SIZE * CG_PrematchPixelsPerUnit() + 2.5f);',
     '    int size = (int)(PREMATCH_EMBLEM_SIZE * CG_PrematchPixelsPerUnit() + 0.5f);'),
    ('    if (kind == s_prematch.emblemKind && size == s_prematch.emblemSize)\n        return;',
     '    if (kind == s_prematch.emblemKind && s_prematch.emblemSize)\n        return;'),
    ('    if (kind == s_prematch.emblemKind && size == s_prematch.emblemSize)\n        return;',
     '    if (size == s_prematch.emblemSize)\n        return;'),
    ('    else if (size > PREMATCH_EMBLEM_MAX_TEXELS)\n        size = PREMATCH_EMBLEM_MAX_TEXELS;\n', '\n'),
    ('        x = floorf(x - 0.5f * size + 0.5f);\n', '        x = floorf(x + 0.5f);\n'),
    ('        CL_DrawStretchPicPhysical(x + shadow, y + shadow, size, size, 0.0f, 0.0f, 1.0f, 1.0f, color,\n',
     '        CL_DrawStretchPicPhysical(x, y, size, size, 0.0f, 0.0f, 1.0f, 1.0f, color,\n'),
    ('    reach = floorf(reach - 0.5f * pixels + 0.5f);', '    reach = floorf(reach + 0.5f);'),
    ('    CG_PrematchColor(ink, color, color[3] * (exact < pixels ? exact / pixels : 1.0f));',
     '    CG_PrematchColor(ink, color, color[3]);'),
    ('    if (lines > 1.0f)\n        lines = 1.0f;\n', ''),
    ('    float lines = (float)(remaining - PREMATCH_LINES_GONE_MSEC) / (float)(PREMATCH_RED_MSEC - PREMATCH_LINES_GONE_MSEC);',
     '    float lines = 1.0f;'),
    ('    CG_PrematchShadowedArc(cx, cy, PREMATCH_OUTER_RADIUS * ppu, PREMATCH_RING_WIDTH * ppu,',
     '    CG_PrematchArc(cx, cy, PREMATCH_OUTER_RADIUS * ppu, PREMATCH_RING_WIDTH * ppu,'),
    ('        CG_PrematchDisc(cx, cy, PREMATCH_INNER_RADIUS * ppu, color);\n', ''),
    ('baseline + (rise + 0.5f) * unit', 'baseline + rise * unit'),
    ('    if (centered)\n        x -= 0.5f * width;\n', ''),
    ('        scale *= room / width;\n', ''),
    ('#define PREMATCH_RING_X -91.25f', '#define PREMATCH_RING_X -90.0f'),
    ('#define PREMATCH_TITLE_BASELINE 59.0f', '#define PREMATCH_TITLE_BASELINE 60.0f'),
    ('#define PREMATCH_OUTER_RADIUS 29.2f', '#define PREMATCH_OUTER_RADIUS 30.0f'),
    ('#define PREMATCH_TEAM_BASELINE -62.9f', '#define PREMATCH_TEAM_BASELINE 62.9f'),
    ('PREMATCH_RING_Y + 0.5f * PREMATCH_DIGIT_CAP', 'PREMATCH_RING_Y'),
    ('    s_prematch.start = start;\n    s_prematch.end = end;\n', '    s_prematch.end = end;\n'),
    ('    memset(&s_prematch, 0, sizeof(s_prematch));\n', ''),
    ('    return offset < 1.0f ? 1.0f : offset;', '    return offset;'),
]
variants = [body] + [mutate(body, old, new) for old, new in mutants]
with tempfile.TemporaryDirectory(prefix='cod2-prematch-') as directory:
    path = Path(directory) / 'prematch.c'
    binary = Path(directory) / 'prematch'
    for index, source in enumerate(variants):
        path.write_text(support + source + '\n' + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wall', '-Werror' if index == 0 else '-w', '-Wno-unused-function', str(path), '-o',
                        str(binary), '-lm'], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, mutants[index - 1] if index else None,
                                                          result.stderr[-2000:])
print('PASS: a player who spawns into the server\'s countdown sees the view in black and white, color returning over '
      'the last 2 s, with the WWII capture\'s layout in place of the HUD: "<mode> // <map>" over the mode\'s retail '
      'objective (S&D\'s defenders their own), the team\'s emblem on a fading teal band with its name, and at the '
      'right the seconds left inside an outer ring that runs out with the countdown and an inner one with each '
      'second, crossed by lines that draw in from 5 s to 2 s, red over a red disc for the last five; spectators, the '
      'dead, followers and deathmatch teams excluded; the emblem built at the drawn size; only valid countdowns '
      f'accepted; {len(mutants)} failing regression mutants')
