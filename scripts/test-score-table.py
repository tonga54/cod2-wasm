#!/usr/bin/env python3
"""Run the production WWII scoreboard on recorded draws: layout, teams, kills and emblems."""
from pathlib import Path
import re
import struct
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
module = (root / 'src/PC/cgame_mp/cg_scoretable_mp.c').read_text()
header = (root / 'src/headers/PC/cgame_mp/cg_scoretable.h').read_text()
servercmds = (root / 'src/PC/cgame_mp/cg_servercmds_mp.c').read_text()
scoreboard = (root / 'src/PC/cgame_mp/cg_scoreboard_mp.c').read_text()
draw = (root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text()
main = (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()
hud2d = (root / 'src/PC/gfx_d3d/r_hud2d.c').read_text()
hud2d_header = (root / 'src/headers/PC/gfx_d3d/r_hud2d.h').read_text()
wwii_fonts = (root / 'src/PC/gfx_d3d/r_font_wwii.c').read_text()
name_font = (root / 'src/headers/PC/cgame_mp/cg_namefont_data.h').read_text()


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


# The cgame builds it with the HUD, draws it in place of the stock scoreboard
# and hides the HUD under it; its emblems have an image slot of their own.
assert main.count('    CG_GrenadeWarningInit();\n    CG_ScoreTableInit();\n') == 1
assert main.count('#include "headers/PC/cgame_mp/cg_scoretable.h"\n') == 1
ordered(function(scoreboard, 'qboolean CG_DrawScoreboard(void)\n{'),
        '    CG_ScoreTablePrepare();\n    if (cg_paused->current.integer != 0 &&',
        'CL_AddReliableCommand((const char *)"score");',
        '    if (CG_ScoreTableEnabled()) {\n        if (!CG_DrawEndgame(fade))\n            CG_DrawScoreTable(fade);\n'
        '        return 1;\n    }\n',
        'imp_colorBlack')
draw2d = function(draw, 'unsigned int CG_Draw2D(void)\n{')
ordered(draw2d,
        '    drawHudElems = !CG_AreHudElemsHidden();\n',
        '    hudCovered = CG_ScoreTableHidesHud();\n    if (hudCovered) {\n        drawHudMenus = 0;\n        drawHudElems = 0;\n    }\n',
        '    CG_MinimapSetHudVisible(drawHudMenus',
        '        if (drawHudElems) {\n            CG_Draw2dHudElems(0);\n            CG_Draw2dHudElems(1);\n',
        '        if (pm_type <= 5 && !hudCovered)\n            CG_DrawCrosshair();\n',
        '        if (!hudCovered)\n            CG_Draw2dHudElems(1);\n',
        '    drewScoreboard = CG_DrawScoreboard();\n    if (!drawHudMenus)\n        return 0;\n')
assert draw2d.count('CG_DrawCrosshair();') == 1 and draw2d.count('CG_Draw2dHudElems(1);') == 2
slots = {name: int(value) for name, value in re.findall(r'^#define (R_HUD_IMAGE_\w+) (\d+)$', hud2d_header, re.M)}
names = re.findall(r'"([^"]+)"', re.search(r's_hudImageNames\[R_HUD_IMAGE_COUNT\] = \{(.*?)\};', hud2d, re.S).group(1))
assert len(names) == slots['R_HUD_IMAGE_COUNT'] and names[slots['R_HUD_IMAGE_TEAM_EMBLEMS']] == '*hud_team_emblems'

# Where the fonts put their capitals, from their own sources: the WWII fonts
# scale Barlow's H to their profile's height, on a baseline that many units
# above the pen; the name font's em is its pixel height, its pen on the
# baseline.
profiles = {name: (int(pixels), float(cap), int(base)) for name, pixels, cap, base in
            re.findall(r'\{ "(\w+)", R_WWII_\w+, (\d+), ([\d.]+)f, (\d+), [\d.]+f, [01] \}', wwii_fonts)}
ui_fonts = ('smallFont', 'normalFont', 'bigFont', 'extraBigFont')
assert set(ui_fonts + ('boldFont',)) <= set(profiles)
# Regular, under the result, keeps the extra big font's metrics, so the table
# tells it by its pixel height.
assert profiles['regularFont'] == profiles['extraBigFont']


def cap_height(path):
    data = path.read_bytes()
    tables = {}
    for i in range(struct.unpack('>H', data[4:6])[0]):
        entry = data[12 + 16 * i:28 + 16 * i]
        tables[entry[:4].decode('latin1')] = struct.unpack('>I', entry[8:12])[0]
    em = struct.unpack('>H', data[tables['head'] + 18:tables['head'] + 20])[0]
    return struct.unpack('>h', data[tables['OS/2'] + 88:tables['OS/2'] + 90])[0] / em


name_pixels = int(re.search(r'#define CG_NAME_FONT_PIXEL_HEIGHT (\d+)', name_font).group(1))
texel_units = int(re.search(r'#define CG_NAME_FONT_UNITS_PER_TEXEL (\d+)', name_font).group(1))
name_cap = cap_height(root / 'third_party/barlow/Barlow-SemiBold.ttf') * name_pixels
# The atlas's H agrees to a texel, standing on the pen.
x0, y0, dx, w, h, ax, ay = map(int, re.search(
    r'\{ 0x48, (-?\d+), (-?\d+), (\d+), (\d+), (\d+), (\d+), (\d+) \}', name_font).groups())
atlas = re.findall(r'^\s*"([0-9a-f]+)",?\s*$', name_font.split('cg_nameFontAtlas[')[1], re.M)
ink = [max(int(c, 16) for c in atlas[ay + r][ax:ax + w]) / 15 for r in range(h)]
inked = [r for r, a in enumerate(ink) if a > 0]
ink_top = y0 + texel_units * (inked[0] + 1 - ink[inked[0]])
ink_bottom = y0 + texel_units * (inked[-1] + ink[inked[-1]])
assert abs(ink_bottom - ink_top - name_cap) <= texel_units and abs(ink_bottom) <= texel_units / 2, \
    (ink_top, ink_bottom, name_cap)
font_metrics = ''.join(f'    {{ {profiles[name][0]}, {profiles[name][1]!r}f, {profiles[name][2]}.0f }},\n'
                       for name in ui_fonts + ('boldFont',))

body = module
for include in ('#include "common_types.h"\n', '#include "imports.h"\n', '#include "headers/PC/cgame_mp/cg_local.h"\n',
                '#include "headers/PC/cgame_mp/cg_scorepanel.h"\n', '#include "headers/PC/cgame_mp/cg_playernames.h"\n',
                '#include "headers/PC/gfx_d3d/r_font_wwii.h"\n', '#include "headers/PC/gfx_d3d/r_hud2d.h"\n'):
    assert body.count(include) == 1, include
    body = body.replace(include, '')
assert body.count('#include "headers/PC/cgame_mp/cg_scoretable.h"\n') == 1
body = body.replace('#include "headers/PC/cgame_mp/cg_scoretable.h"\n', header)
parse = function(servercmds, 'static void CG_ParseScores(void)\n{')

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
typedef int Bool;
typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef struct Glyph { int letter; int y0; } Glyph;
/* hTop is where its H's box starts above the pen, 0 for a font without the glyph. */
typedef struct Font_s { const char *name; int pixelHeight; int hTop; } Font;
typedef Font *FontHandle;
typedef struct Material { const char *name; } Material;
typedef Material *MaterialHandle;
typedef struct { const char *name; union { int enabled; int integer; float value; } current; } dvar_t;
typedef struct HudVertex2D { float xy[2]; float st[2]; float rgba[4]; } HudVertex2D;
enum { TEAM_FREE = 0, TEAM_AXIS = 1, TEAM_ALLIES = 2, TEAM_SPECTATOR = 3 };
#define R_HUD_IMAGE_TEAM_EMBLEMS ''' + str(slots['R_HUD_IMAGE_TEAM_EMBLEMS']) + r'''
typedef struct { int infoValid; char name[32]; int team; int score; } clientInfo_t;
typedef struct { int client; int score; int ping; int deaths; int team; MaterialHandle hStatusIcon; } score_t;
typedef struct {
    int clientNum;
    int numScores;
    score_t scores[64];
    int teamScores[4];
    int teamPings[4];
    int teamPlayers[4];
    struct { clientInfo_t clientinfo[64]; } bgs;
} cg_t;
typedef struct { char gametype[32]; char mapname[64]; struct { MaterialHandle whiteMaterial; } media; } cgs_t;
static cg_t cgState;
static cgs_t cgsState;
#define cg (&cgState)
#define cgs (&cgsState)

struct Font_s *CG_GetPlayerNameFont(void);
const char *CG_ScorePanelClock(vec4_t color);
qboolean CG_ScoreboardDisplayed(void);
MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height);
void R_AddCmdDrawTriangles2D(const HudVertex2D *verts, int vertexCount, const unsigned short *indices, int indexCount,
                             MaterialHandle material);

/* The capitals' height and the baseline's above the pen, in font units. */
typedef struct { int pixelHeight; float cap; float base; } Metrics;
static const Metrics stockFonts[] = {
''' + font_metrics + r'''};
#define NAME_CAP ''' + repr(name_cap) + r'''f
/* The fonts at their sizes, with the boxes of their H. */
static Font nameFont = { "playernames", ''' + str(name_pixels) + r''', -40 };
static Font boldFont = { "$boldfont", ''' + str(profiles['boldFont'][0]) + r''', -24 };
static Font uiFont = { "fonts/normalFont", ''' + str(profiles['normalFont'][0]) + r''', -14 };
static Font regularFont = { "$regularfont", ''' + str(profiles['regularFont'][0]) + r''', -21 };
static int nameFontReady = 1;
static Material white = { "white" }, emblemMaterial = { "*hud_team_emblems" }, voiceOn = { "voice_on" };
static Material voiceOff = { "voice_off" }, statusIcon = { "status" };
static dvar_t scoreboardDvar = { "cg_hudScoreboard" };
static int dvarRegistrations;
const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags) {
    assert(!strcmp(name, "cg_hudScoreboard") && value == 1 && flags == 0x1001);
    ++dvarRegistrations;
    scoreboardDvar.current.enabled = value;
    return &scoreboardDvar;
}
static const char *alliesBanner = "mpflag_american";
const char *Dvar_GetString(const char *name) { assert(!strcmp(name, "g_ScoresBanner_Allies")); return alliesBanner; }
static float uiFontScale;
FontHandle UI_GetFontHandle(int fontEnum, float scale) { assert(fontEnum == 0); uiFontScale = scale; return &uiFont; }
FontHandle CL_RegisterFont(const char *name, int imageTrack) {
    assert(!strcmp(name, "$boldfont") && imageTrack == 0);
    return &boldFont;
}
FontHandle R_RegisterWwiiFont(const char *name) { assert(!strcmp(name, "$regularfont")); return &regularFont; }
struct Font_s *CG_GetPlayerNameFont(void) { return nameFontReady ? &nameFont : NULL; }
const Glyph *R_GetCharacterGlyph(FontHandle font, unsigned int letter) {
    static Glyph glyph;
    assert(letter == 'H');
    if (!font->hTop)
        return NULL;
    glyph.letter = 'H';
    glyph.y0 = font->hTop;
    return &glyph;
}
static int visible(const char *text, int maxChars) {
    int n = 0;
    for (const char *c = text; *c && (maxChars <= 0 || n < maxChars); ++c) {
        if (c[0] == '^' && c[1] && c[1] != '^' && c[1] > '/' && c[1] <= '9') {
            ++c;
            continue;
        }
        ++n;
    }
    return n;
}
/* Every letter advances three quarters of the font's pixel height. */
static int advanceOf(FontHandle font) { return font->pixelHeight * 3 / 4; }
int CL_TextWidth(const char *text, int maxChars, FontHandle font) {
    assert(font == &nameFont || font == &boldFont || font == &uiFont || font == &regularFont);
    return visible(text, maxChars) * advanceOf(font);
}
float CL_NormalizedTextScale(FontHandle font, float scale) { return 48.0f * scale / (float)font->pixelHeight; }

enum { PIC, PHYS, TEXT, TRI };
typedef struct {
    int kind;
    float x, y, w, h, s0, t0, s1, t1, scale;
    float color[4];
    MaterialHandle material;
    FontHandle font;
    char text[160];
    int maxChars, style;
    HudVertex2D verts[3];
} Draw;
static Draw draws[2048];
static int drawCount;
static Draw *record(int kind, const float *color) {
    Draw *d;
    assert(drawCount < 2048);
    d = &draws[drawCount++];
    memset(d, 0, sizeof(*d));
    d->kind = kind;
    memcpy(d->color, color, sizeof(d->color));
    return d;
}
void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign,
                 float scale, const vec_t *color, int style) {
    Draw *d = record(TEXT, color);
    assert(horzAlign == 0 && vertAlign == 0 && maxChars > 0);
    snprintf(d->text, sizeof(d->text), "%s", text);
    d->maxChars = maxChars;
    d->font = font;
    d->x = x;
    d->y = y;
    d->scale = scale;
    d->style = style;
}
void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2,
                       float t2, const vec_t *color, MaterialHandle material) {
    Draw *d = record(PIC, color);
    assert(horzAlign == 0 && vertAlign == 0);
    d->x = x; d->y = y; d->w = w; d->h = h; d->s0 = s1; d->t0 = t1; d->s1 = s2; d->t1 = t2;
    d->material = material;
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
    assert(vertexCount == 3 && indexCount == 3 && indices[0] == 0 && indices[1] == 1 && indices[2] == 2);
    memcpy(d->verts, verts, sizeof(d->verts));
    d->material = material;
}
/* A 1024x576 screen: the 4:3 area 1.2 pixels a unit, 128 from the left. */
static float unitScale = 1.2f, offsetX = 128.0f;
void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign) {
    assert((horzAlign == 0 && vertAlign == 0) || (horzAlign == 4 && vertAlign == 4));
    if (horzAlign == 4) {
        *x *= 1.6f; *w *= 1.6f; *y *= 1.2f; *h *= 1.2f;
        return;
    }
    *x = *x * unitScale + offsetX;
    *w *= unitScale;
    *y *= unitScale;
    *h *= unitScale;
}
const char *UI_GetGameTypeDisplayName(const char *gameType) {
    if (!strcasecmp(gameType, "tdm"))
        return "MPUI_TEAM_DEATHMATCH";
    if (!strcasecmp(gameType, "dm"))
        return "MPUI_DEATHMATCH";
    return gameType;
}
const char *SEH_LocalizeTextMessage(const char *msg, const char *context, int errType) {
    assert(!strcmp(context, "scoreboard gametype display") && errType == 0);
    if (!strcmp(msg, "MPUI_TEAM_DEATHMATCH"))
        return "Team Deathmatch";
    if (!strcmp(msg, "MPUI_DEATHMATCH"))
        return "Deathmatch";
    return msg;
}
const char *FS_GetMapBaseName(const char *mapfile) {
    static char base[64];
    const char *slash = strrchr(mapfile, '/');
    char *dot;
    snprintf(base, sizeof(base), "%s", slash ? slash + 1 : mapfile);
    if ((dot = strrchr(base, '.')))
        *dot = 0;
    return base;
}
static const char *mapDisplayName = "Toujane, Tunisia";
const char *UI_GetMapDisplayName(const char *mapName) { assert(!strcmp(mapName, "mp_toujane")); return mapDisplayName; }
static int muted[64], talking[64];
Bool CL_IsPlayerMuted(int clientIndex) { return muted[clientIndex]; }
Bool CL_IsPlayerTalking(int clientIndex) { return talking[clientIndex]; }
struct Material *CL_RegisterMaterialNoMip(const char *name, int imageTrack) {
    assert(imageTrack == 7);
    if (!strcmp(name, "voice_on"))
        return &voiceOn;
    if (!strcmp(name, "voice_off"))
        return &voiceOff;
    assert(!strcmp(name, "status"));
    return &statusIcon;
}
int I_stricmp(const char *a, const char *b) { return strcasecmp(a, b); }
void I_strncpyz(char *dest, const char *src, int size) { strncpy(dest, src, size - 1); dest[size - 1] = 0; }
int Com_sprintf(char *dest, int size, const char *fmt, ...) {
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsnprintf(dest, size, fmt, ap);
    va_end(ap);
    return n;
}
const char *va(const char *fmt, ...) {
    static char buffers[8][256];
    static int next;
    char *out = buffers[next++ & 7];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(out, 256, fmt, ap);
    va_end(ap);
    return out;
}
static const char *clockText = "3:40";
static float clockColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
const char *CG_ScorePanelClock(vec4_t color) { memcpy(color, clockColor, sizeof(clockColor)); return clockText; }
static byte emblemTexels[2 * 384 * 384 * 4];
static int emblemWidth, emblemHeight, emblemBuilds, failBuild;
MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height) {
    assert(slot == R_HUD_IMAGE_TEAM_EMBLEMS && width == 2 * height && width * height * 4 <= (int)sizeof(emblemTexels));
    memcpy(emblemTexels, bgra, (size_t)width * (size_t)height * 4);
    emblemWidth = width;
    emblemHeight = height;
    ++emblemBuilds;
    return failBuild ? NULL : &emblemMaterial;
}
static int displayed = 1;
qboolean CG_ScoreboardDisplayed(void) { return displayed; }
static const char *argv[512];
static int argc;
const char *CG_Argv(int arg) { return arg < argc ? argv[arg] : ""; }
int Cmd_Argc(void) { return argc; }
const char *CL_GetConfigString(int index) { assert(index >= 0x17 && index <= 0x1e); return "status"; }
'''

checks = r'''
static int near(float a, float b) { return fabsf(a - b) < 2e-3f; }
static const float textRgb[3] = { 0.82f, 0.79f, 0.74f };
static const float teal[3] = { 0.145f, 0.42f, 0.38f };
static const float red[3] = { 0.67f, 0.185f, 0.16f };
static const float green[3] = { 0.22f, 0.48f, 0.2f };
static const float pingGood[3] = { 0.36f, 0.72f, 0.33f };
static const float pingFair[3] = { 0.89f, 0.73f, 0.26f };
static const float pingPoor[3] = { 0.82f, 0.26f, 0.21f };
static const float black[3] = { 0.0f, 0.0f, 0.0f };
static int colored(const float *color, const float *rgb, float alpha) {
    return near(color[0], rgb[0]) && near(color[1], rgb[1]) && near(color[2], rgb[2]) && near(color[3], alpha);
}
static int rgbIs(const float *color, const float *rgb) { return colored(color, rgb, color[3]); }

static char commandText[2048];
static void scores(const char *text) {
    snprintf(commandText, sizeof(commandText), "%s", text);
    argc = 0;
    for (char *t = strtok(commandText, " "); t; t = strtok(NULL, " "))
        argv[argc++] = t;
    CG_ParseScores();
}
static void player(int client, const char *name, int team) {
    clientInfo_t *ci = &cg->bgs.clientinfo[client];
    ci->infoValid = 1;
    snprintf(ci->name, sizeof(ci->name), "%s", name);
    ci->team = team;
}
static void render(float fade) {
    drawCount = 0;
    CG_ScoreTablePrepare();
    CG_DrawScoreTable(fade);
}
static const Draw *text(const char *s) {
    for (int i = 0; i < drawCount; ++i) {
        if (draws[i].kind == TEXT && !strcmp(draws[i].text, s))
            return &draws[i];
    }
    return NULL;
}
static int textIndex(const char *s) { const Draw *d = text(s); return d ? (int)(d - draws) : -1; }
/* A font unit in virtual units; then where the text's pen and capitals land
 * by the font's metrics once the renderer moves it half a unit up and to the
 * left. */
static float unitOf(const Draw *d) { return 48.0f * d->scale / (float)d->font->pixelHeight; }
static float penOf(const Draw *d) { return d->x - 0.5f * unitOf(d); }
static Metrics metricsOf(FontHandle font) {
    Metrics name = { font->pixelHeight, NAME_CAP, 0.0f };
    if (font == &nameFont)
        return name;
    for (int i = 0; i < (int)(sizeof(stockFonts) / sizeof(stockFonts[0])); ++i) {
        if (stockFonts[i].pixelHeight == font->pixelHeight)
            return stockFonts[i];
    }
    assert(!"a font of a stock size");
    return name;
}
static float capOf(const Draw *d) { return metricsOf(d->font).cap * unitOf(d); }
static float baselineOf(const Draw *d) { return d->y - (metricsOf(d->font).base + 0.5f) * unitOf(d); }
static float width(const Draw *d) { return (float)(visible(d->text, d->maxChars) * advanceOf(d->font)) * unitOf(d); }
/* Its letters fit the room, and one more would not. */
static int fits(const Draw *d, float room) {
    return width(d) <= room && width(d) + (float)advanceOf(d->font) * unitOf(d) > room;
}
static const float centers[3] = { 398.6f, 482.5f, 567.2f };
/* The text centered on a column of numbers on a row's baseline. */
static const Draw *cell(int column, float baseline) {
    for (int i = 0; i < drawCount; ++i) {
        const Draw *d = &draws[i];
        if (d->kind == TEXT && near(baselineOf(d), baseline) && near(penOf(d) + 0.5f * width(d), centers[column]))
            return d;
    }
    return NULL;
}
/* Text whose capitals stand cap tall on a baseline, its pen starting at x. */
static int textAt(const Draw *d, float x, float baseline, float cap, FontHandle font, int style, float alpha) {
    return d && d->font == font && near(penOf(d), x) && near(baselineOf(d), baseline) && near(capOf(d), cap) &&
           d->style == style && colored(d->color, textRgb, alpha);
}
static int textsAt(const char *s, float x, float baseline, float cap, FontHandle font, int style, float alpha) {
    int n = 0;
    for (int i = 0; i < drawCount; ++i)
        n += draws[i].kind == TEXT && !strcmp(draws[i].text, s) && textAt(&draws[i], x, baseline, cap, font, style, alpha);
    return n;
}
/* Full-width stripes of a color, top to bottom; edges gets each top, then the last bottom. */
static int stripes(const float *rgb, float *edges, float *alphas) {
    int n = 0;
    for (int i = 0; i < drawCount; ++i) {
        const Draw *d = &draws[i];
        if (d->kind != PHYS || d->x != 258.0f || d->w != 626.0f || !rgbIs(d->color, rgb) || d->h < 2.0f)
            continue;
        if (edges) {
            if (n)
                assert(edges[n] == d->y);
            edges[n] = d->y;
            edges[n + 1] = d->y + d->h;
        }
        if (alphas)
            alphas[n] = d->color[3];
        ++n;
    }
    return n;
}
static int count(int kind, MaterialHandle material) {
    int n = 0;
    for (int i = 0; i < drawCount; ++i)
        n += draws[i].kind == kind && (!material || draws[i].material == material);
    return n;
}
static const Draw *phys(float x, float y, float w, float h, const float *rgb, float alpha) {
    for (int i = 0; i < drawCount; ++i) {
        const Draw *d = &draws[i];
        if (d->kind == PHYS && d->x == x && d->y == y && d->w == w && d->h == h && colored(d->color, rgb, alpha))
            return d;
    }
    return NULL;
}
static int alphaAt(int half, int x, int y) { return emblemTexels[(y * emblemWidth + half * emblemHeight + x) * 4 + 3]; }
/* Every texel is white with the coverage of its emblem at its center. */
static void rasterOf(int half, int kind) {
    int size = emblemHeight;
    float units = 0.5f * (float)size - 1.0f;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const byte *texel = &emblemTexels[(y * emblemWidth + half * size + x) * 4];
            float coverage = 0.0f;
            if (kind >= 0) {
                float u = ((float)x + 0.5f - 0.5f * (float)size) / units;
                float v = ((float)y + 0.5f - 0.5f * (float)size) / units;
                coverage = 0.5f - CG_ScoreTableEmblemDistance(&s_table, kind, u, v) * units;
                coverage = coverage < 0.0f ? 0.0f : coverage > 1.0f ? 1.0f : coverage;
            }
            assert(texel[0] == 255 && texel[1] == 255 && texel[2] == 255);
            assert(abs((int)texel[3] - (int)(coverage * 255.0f + 0.5f)) <= 1);
        }
    }
    for (int i = 0; i < size; ++i) {
        assert(!alphaAt(half, i, 0) && !alphaAt(half, i, size - 1) && !alphaAt(half, 0, i) && !alphaAt(half, size - 1, i));
    }
}
static float at(int kind, float radius, float degrees) {
    float a = degrees * 3.14159265f / 180.0f;
    return CG_ScoreTableEmblemDistance(&s_table, kind, radius * cosf(a), radius * sinf(a));
}
static void reset(const char *gametype) {
    memset(cg, 0, sizeof(*cg));
    snprintf(cgs->gametype, sizeof(cgs->gametype), "%s", gametype);
    snprintf(cgs->mapname, sizeof(cgs->mapname), "maps/mp/mp_toujane.bsp");
    cgs->media.whiteMaterial = &white;
    memset(muted, 0, sizeof(muted));
    memset(talking, 0, sizeof(talking));
}

int main(void) {
    const float edges0[10] = { 109, 130, 151, 173, 194, 215, 236, 258, 279, 300 };
    const float edges1[10] = { 317, 338, 359, 381, 402, 423, 444, 466, 487, 508 };
    float edges[40];
    float alphas[40];

    /* The capture's match: Chapu54 of [UY] on the Allies against three
     * Germans, a spectator, and a player whose info is not in yet. */
    reset("tdm");
    CG_ScoreTableInit();
    assert(dvarRegistrations == 1 && CG_ScoreTableEnabled() && CG_ScoreTableHidesHud());
    cg->clientNum = 5;
    player(5, "[UY]Chapu54", TEAM_ALLIES);
    player(7, "Zebra", TEAM_AXIS);
    player(9, "Foxchase", TEAM_AXIS);
    player(11, "Love", TEAM_AXIS);
    player(2, "Bob", TEAM_SPECTATOR);
    talking[7] = 1;
    muted[9] = 1;
    scores("b 6 9 3 7 325 60 1 2 9 200 140 0 0 11 100 250 0 0 5 300 40 9 0 2 0 0 0 0 13 50 50 0 0 4 2 1 3 0 7");
    render(1.0f);

    /* The screen dims, then the title, the clock and the column names. */
    assert(draws[0].kind == PHYS && draws[0].x == 0 && draws[0].y == 0 && near(draws[0].w, 1024) &&
           near(draws[0].h, 576) && colored(draws[0].color, black, 0.5f) && draws[0].material == &white);
    assert(textAt(text("Team Deathmatch | Toujane"), 9.5f, 59.7f, 15.05f, &boldFont, 3, 1));
    const Draw *clock = text("3:40");
    assert(textAt(clock, 621 - width(clock), 69.3f, 13.65f, &boldFont, 3, 1));
    assert(near(uiFontScale, 8.85f / 30.0f));
    static const char *const headers[3] = { "Score", "Kills", "Deaths" };
    for (int i = 0; i < 3; ++i) {
        const Draw *d = text(headers[i]);
        assert(textAt(d, centers[i] - 0.5f * width(d), 86.7f, 8.85f, &uiFont, 0, 1));
    }
    /* In every size the UI picks for them. */
    for (int i = 0; i < 4; ++i) {
        uiFont.pixelHeight = stockFonts[i].pixelHeight;
        render(1.0f);
        const Draw *d = text("Deaths");
        assert(textAt(d, centers[2] - 0.5f * width(d), 86.7f, 8.85f, &uiFont, 0, 1));
    }
    uiFont.pixelHeight = stockFonts[1].pixelHeight;
    render(1.0f);

    /* The viewer's Allies on top in teal with their name, score and star. */
    assert(textAt(text("ALLIES"), 15.8f, 90.8f + 13.1f, 13.55f, &boldFont, 3, 1));
    assert(textAt(text("AXIS"), 15.8f, 264.1f + 13.1f, 13.55f, &boldFont, 3, 1));
    assert(textsAt("3", 15.8f, 90.8f + 37.4f, 21.7f, &boldFont, 3, 1) == 1);
    assert(textsAt("9", 15.8f, 264.1f + 37.4f, 21.7f, &boldFont, 3, 1) == 1);
    assert(emblemBuilds == 1 && emblemWidth == 192 && emblemHeight == 96 && count(PHYS, &emblemMaterial) == 2);
    const Draw *star = phys(150, 156, 96, 96, textRgb, 0.55f);
    const Draw *cross = phys(150, 364, 96, 96, textRgb, 0.55f);
    assert(star && star->material == &emblemMaterial && star->s0 == 0.0f && star->s1 == 0.5f && star->t0 == 0 &&
           star->t1 == 1);
    assert(cross && cross->material == &emblemMaterial && cross->s0 == 0.5f && cross->s1 == 1.0f);

    /* Nine stripes a team, light first, sharing their pixel edges. */
    assert(stripes(teal, edges, alphas) == 9 && stripes(red, NULL, NULL) == 9);
    for (int i = 0; i < 9; ++i)
        assert(edges[i] == edges0[i] && edges[i + 1] == edges0[i + 1] && near(alphas[i], (i & 1) ? 0.25f : 0.45f));
    stripes(red, edges, alphas);
    for (int i = 0; i < 9; ++i)
        assert(edges[i] == edges1[i] && edges[i + 1] == edges1[i + 1] && near(alphas[i], (i & 1) ? 0.25f : 0.45f));
    for (int i = 0; i < 9; ++i) {
        assert(phys(553, edges0[i], 102, edges0[i + 1] - edges0[i], black, 0.1f));
        assert(phys(757, edges1[i], 103, edges1[i + 1] - edges1[i], black, 0.1f));
    }

    /* Chapu54's row: the tag in its column, the numbers centered, all four
     * bars green for 40 ms, then the light outline, the green edge and the
     * arrow outside. */
    float base = 90.8f + 0.612f * 17.7f;
    const Draw *tag = text("[UY]");
    assert(textAt(tag, 349.6f - width(tag), base, 7.3f, &nameFont, 0, 1));
    const Draw *name = text("Chapu54");
    assert(textAt(name, 159.5f, base, 7.3f, &nameFont, 0, 1) && name->maxChars == 7);
    assert(textIndex("Chapu54") > 0 && !strcmp(cell(0, base)->text, "300") && !strcmp(cell(1, base)->text, "3") &&
           !strcmp(cell(2, base)->text, "9"));
    assert(phys(865, 121, 3, 2, pingGood, 1) && phys(869, 119, 3, 4, pingGood, 1) &&
           phys(873, 117, 3, 6, pingGood, 1) && phys(877, 115, 3, 8, pingGood, 1));
    assert(phys(258, 109, 626, 1, textRgb, 0.9f) && phys(258, 129, 626, 1, textRgb, 0.9f));
    assert(phys(258, 110, 1, 19, textRgb, 0.9f) && phys(883, 110, 1, 19, textRgb, 0.9f));
    assert(phys(258, 109, 2, 21, green, 1));
    assert(count(TRI, &white) == 1);
    for (int i = 0; i < drawCount; ++i) {
        if (draws[i].kind != TRI)
            continue;
        const HudVertex2D *v = draws[i].verts;
        assert(near(v[0].xy[0], 254) && near(v[0].xy[1], 116.28f) && near(v[1].xy[0], 254) &&
               near(v[1].xy[1], 122.88f) && near(v[2].xy[0], 257) && near(v[2].xy[1], 119.58f));
        assert(colored(v[0].rgba, textRgb, 1) && colored(v[2].rgba, textRgb, 1));
        assert(i > textIndex("Chapu54"));
    }

    /* The Germans in the server's order with their kills, talk and status. */
    float axisBase = 264.1f + 0.612f * 17.7f;
    assert(textAt(text("Zebra"), 159.5f, axisBase, 7.3f, &nameFont, 0, 1));
    assert(!strcmp(cell(0, axisBase)->text, "325") && !strcmp(cell(1, axisBase)->text, "4") &&
           !strcmp(cell(2, axisBase)->text, "1"));
    assert(textAt(text("Foxchase"), 159.5f, axisBase + 17.7f, 7.3f, &nameFont, 0, 1));
    assert(!strcmp(cell(1, axisBase + 17.7f)->text, "2"));
    assert(textAt(text("Love"), 159.5f, axisBase + 35.4f, 7.3f, &nameFont, 0, 1));
    assert(count(PIC, &voiceOn) == 1 && count(PIC, &voiceOff) == 1 && count(PIC, &statusIcon) == 1);
    for (int i = 0; i < drawCount; ++i) {
        const Draw *d = &draws[i];
        if (d->kind == PIC && d->material == &voiceOn)
            assert(near(d->x, 124) && near(d->y, 264.1f + 2.85f) && near(d->w, 12) && colored(d->color, textRgb, 1));
        if (d->kind == PIC && d->material == &statusIcon)
            assert(near(d->x, 140) && near(d->y, 264.1f + 2.85f) && near(d->w, 12));
        if (d->kind == PIC && d->material == &voiceOff)
            assert(near(d->x, 124) && near(d->y, 264.1f + 17.7f + 2.85f));
    }
    /* 40 ms lights four bars green, 60 three yellow, 140 two red and 250 one
     * red; the rest are faint. */
    int good = 0, fair = 0, poor = 0, unlit = 0;
    for (int i = 0; i < drawCount; ++i) {
        const Draw *d = &draws[i];
        if (d->kind != PHYS || d->x < 864 || d->w > 4)
            continue;
        good += colored(d->color, pingGood, 1);
        fair += colored(d->color, pingFair, 1);
        poor += colored(d->color, pingPoor, 1);
        unlit += colored(d->color, textRgb, 0.12f);
    }
    assert(good == 4 && fair == 3 && poor == 2 + 1 && unlit == 1 + 2 + 3);
    {
        const int pings[][2] = { { -1, 0 }, { 0, 4 }, { 59, 4 }, { 60, 3 }, { 119, 3 }, { 120, 2 }, { 199, 2 },
                                 { 200, 1 }, { 999, 1 } };
        for (unsigned k = 0; k < sizeof(pings) / sizeof(pings[0]); ++k)
            assert(CG_ScoreTablePingBars(pings[k][0]) == pings[k][1]);
    }
    assert(!text("Ghost") && textAt(text("Spectators: Bob^7"), 9.5f, 446, 7, &nameFont, 0, 0.8f));

    /* The stock scoreboard's message has no kills: the column stays empty. */
    scores("b 6 9 3 7 325 60 1 0 9 200 140 0 0 11 100 250 0 0 5 300 40 9 0 2 0 0 0 0 13 50 50 0 0");
    render(1.0f);
    assert(!cell(1, base) && !cell(1, axisBase) && !strcmp(cell(0, axisBase)->text, "325"));
    scores("b 1 9 3 5 300 40 9 0 -4");
    render(1.0f);
    assert(!strcmp(cell(1, base)->text, "0"));

    /* An Axis viewer has the Germans on top in teal; a spectator the Allies. */
    scores("b 6 9 3 7 325 60 1 0 9 200 140 0 0 11 100 250 0 0 5 300 40 9 0 2 0 0 0 0 13 50 50 0 0 4 2 1 3 0 7");
    cg->clientNum = 7;
    render(1.0f);
    assert(textAt(text("AXIS"), 15.8f, 90.8f + 13.1f, 13.55f, &boldFont, 3, 1));
    assert(textAt(text("ALLIES"), 15.8f, 264.1f + 13.1f, 13.55f, &boldFont, 3, 1));
    assert(textAt(text("Zebra"), 159.5f, base, 7.3f, &nameFont, 0, 1) && stripes(teal, edges, NULL) == 9 &&
           edges[0] == 109);
    cross = phys(150, 156, 96, 96, textRgb, 0.55f);
    assert(cross && cross->s0 == 0.5f && phys(150, 364, 96, 96, textRgb, 0.55f)->s0 == 0.0f);
    assert(phys(258, 109, 2, 21, green, 1) && !phys(258, 317, 2, 21, green, 1));
    cg->clientNum = 2;
    render(1.0f);
    assert(textAt(text("ALLIES"), 15.8f, 90.8f + 13.1f, 13.55f, &boldFont, 3, 1) && count(TRI, NULL) == 0);
    cg->clientNum = 5;

    /* Fading out takes everything down with it. */
    render(0.5f);
    assert(colored(draws[0].color, black, 0.25f) && phys(150, 156, 96, 96, textRgb, 0.275f));
    assert(stripes(teal, NULL, alphas) == 9 && near(alphas[0], 0.225f) && text("Chapu54")->color[3] == 0.5f);

    /* The clock keeps a color of its own and leaves when the HUD has none. */
    clockColor[1] = clockColor[2] = 0.3f;
    render(1.0f);
    assert(near(text("3:40")->color[0], 1) && near(text("3:40")->color[1], 0.3f));
    clockColor[3] = 0.0f;
    render(1.0f);
    assert(!text("3:40"));
    clockColor[1] = clockColor[2] = clockColor[3] = 1.0f;
    clockText = NULL;
    render(1.0f);
    assert(!text("3:40") && text("Team Deathmatch | Toujane"));
    clockText = "3:40";

    /* At the end of a match the result stands over the title in bold and why
     * the match ended under it in Regular, where the clock was. */
    drawCount = 0;
    CG_DrawScoreTableResult(1.0f, "ALLIES VICTORY", "Time limit reached");
    assert(textAt(text("ALLIES VICTORY"), 9.5f, 37.1f, 11.25f, &boldFont, 3, 1));
    assert(textAt(text("Time limit reached"), 9.5f, 79.7f, 8.0f, &regularFont, 0, 1));
    assert(!text("3:40") && textAt(text("Team Deathmatch | Toujane"), 9.5f, 59.7f, 15.05f, &boldFont, 3, 1));
    assert(colored(draws[0].color, black, 0.5f) && textAt(text("Chapu54"), 159.5f, base, 7.3f, &nameFont, 0, 1));
    assert(textIndex("ALLIES VICTORY") > textIndex("Team Deathmatch | Toujane") &&
           textIndex("Time limit reached") < textIndex("Score"));
    /* Without a reason the result stands alone, and it fades with the rest. */
    drawCount = 0;
    CG_DrawScoreTableResult(0.5f, "DRAW", "");
    assert(textAt(text("DRAW"), 9.5f, 37.1f, 11.25f, &boldFont, 3, 0.5f) && !text("") && !text("3:40"));
    drawCount = 0;
    CG_DrawScoreTableResult(1.0f, NULL, NULL);
    assert(!text("3:40") && text("Team Deathmatch | Toujane"));
    /* Without Regular the reason takes the UI font. */
    s_table.regularFont = NULL;
    drawCount = 0;
    CG_DrawScoreTableResult(1.0f, "AXIS DEFEAT", "Score limit reached");
    assert(textAt(text("Score limit reached"), 9.5f, 79.7f, 8.0f, &uiFont, 0, 1));
    s_table.regularFont = &regularFont;
    render(1.0f);
    assert(text("3:40") && !text("ALLIES VICTORY"));

    /* A map without a display name keeps its file name; a long title shrinks. */
    mapDisplayName = "";
    render(1.0f);
    assert(text("Team Deathmatch | mp_toujane"));
    mapDisplayName = "Sainte Marie du Mont, Normandy, France";
    render(1.0f);
    assert(text("Team Deathmatch | Sainte Marie du Mont"));
    mapDisplayName = "A Very Long Name For A Map, Far Away";
    render(1.0f);
    const Draw *title = text("Team Deathmatch | A Very Long Name For A Map");
    assert(title && capOf(title) < 15.0f && fabsf(width(title) - 550.5f) < 0.01f && near(baselineOf(title), 59.7f));
    mapDisplayName = "Toujane, Tunisia";

    /* Clan tags take a column, colors carry into the name, long names are cut. */
    player(5, "^1[UY]Chapu", TEAM_ALLIES);
    render(1.0f);
    tag = text("^1[UY]");
    assert(textAt(tag, 349.6f - width(tag), base, 7.3f, &nameFont, 0, 1));
    assert(textAt(text("^1Chapu"), 159.5f, base, 7.3f, &nameFont, 0, 1));
    player(5, "[UY]  Spaced", TEAM_ALLIES);
    render(1.0f);
    assert(text("[UY]") && text("Spaced"));
    player(5, "[TOOLONGTAG1]x", TEAM_ALLIES);
    render(1.0f);
    assert(text("[TOOLONGTAG1]x") && near(penOf(text("[TOOLONGTAG1]x")), 159.5f));
    player(5, "[UY]", TEAM_ALLIES);
    render(1.0f);
    assert(text("[UY]") && near(penOf(text("[UY]")), 159.5f));
    player(5, "Supercalifragilistic_expialidoc", TEAM_ALLIES);
    render(1.0f);
    name = text("Supercalifragilistic_expialidoc");
    assert(name->maxChars == 24 && fits(name, 349.6f - 159.5f));
    player(5, "[UY]Supercalifragilistic_expia", TEAM_ALLIES);
    render(1.0f);
    name = text("Supercalifragilistic_expia");
    tag = text("[UY]");
    assert(name->maxChars == 19 && fits(name, 349.6f - width(tag) - 6 - 159.5f));
    player(5, "[UY]Chapu54", TEAM_ALLIES);

    /* Without the name font the rows use the UI font; one of a size of its
     * own is measured by its H, or a share of its height without one. */
    nameFontReady = 0;
    render(1.0f);
    assert(textAt(text("Chapu54"), 159.5f, base, 7.3f, &uiFont, 0, 1));
    uiFont.pixelHeight = 20;
    uiFont.hTop = -15;
    render(1.0f);
    name = text("Chapu54");
    assert(near(name->scale, 7.3f * 20 / (48 * 15.0f)) && near(name->y, base + 0.5f * 7.3f / 15.0f));
    uiFont.hTop = 0;
    render(1.0f);
    name = text("Chapu54");
    assert(near(name->scale, 7.3f * 20 / (48 * 12.5f)) && near(name->y, base + 0.5f * 7.3f / 12.5f));
    uiFont.pixelHeight = stockFonts[1].pixelHeight;
    uiFont.hTop = -14;
    nameFontReady = 1;

    /* Crowded teams squeeze their rows; past the smallest height the viewer
     * still gets the last row. */
    reset("tdm");
    cg->clientNum = 40;
    char command[2048];
    int length = snprintf(command, sizeof(command), "b 21 0 0");
    for (int i = 0; i < 20; ++i) {
        char pname[16];
        snprintf(pname, sizeof(pname), "Axis%02d", i);
        player(20 + i, pname, TEAM_AXIS);
        length += snprintf(command + length, sizeof(command) - length, " %d %d 50 0 0", 20 + i, 1000 - i);
    }
    player(40, "Me", TEAM_AXIS);
    length += snprintf(command + length, sizeof(command) - length, " 40 1 50 0 0");
    scores(command);
    render(1.0f);
    float squeezed = 159.3f / 17.0f;
    assert(stripes(teal, edges, NULL) == 17 && edges[0] == 109 && edges[17] == 300);
    assert(textAt(text("Me"), 159.5f, 90.8f + 16.0f * squeezed + 0.612f * squeezed, 7.3f * squeezed / 17.7f, &nameFont,
                  0, 1));
    assert(text("Axis15") && !text("Axis16") && !text("Axis19"));
    assert(stripes(red, NULL, NULL) == 9);
    reset("tdm");
    cg->clientNum = 40;
    player(40, "Me", TEAM_AXIS);
    length = snprintf(command, sizeof(command), "b 12 0 0");
    for (int i = 0; i < 12; ++i) {
        char pname[16];
        snprintf(pname, sizeof(pname), "Axis%02d", i);
        player(28 + i, pname, TEAM_AXIS);
        length += snprintf(command + length, sizeof(command) - length, " %d 0 50 0 0", 28 + i);
    }
    scores(command);
    render(1.0f);
    assert(stripes(teal, edges, NULL) == 12 && edges[12] == 300);
    assert(textAt(text("Axis11"), 159.5f, 90.8f + 11 * 13.275f + 0.612f * 13.275f, 7.3f * 0.75f, &nameFont, 0, 1));

    /* Deathmatch: one block of eighteen rows, the viewer's own in teal, with
     * no team names or emblems, even before any score arrives. */
    reset("dm");
    cg->clientNum = 3;
    player(3, "Me", TEAM_FREE);
    player(4, "Other", TEAM_FREE);
    player(6, "Third", TEAM_FREE);
    scores("b 3 0 0 4 500 20 1 0 3 400 30 2 0 6 100 40 3 0 5 4 1");
    render(1.0f);
    assert(stripes(teal, edges, NULL) == 1 && edges[0] == 130 && stripes(red, NULL, NULL) == 17);
    assert(!text("AXIS") && !text("ALLIES") && count(PHYS, &emblemMaterial) == 0);
    assert(textAt(text("Other"), 159.5f, base, 7.3f, &nameFont, 0, 1) && !strcmp(cell(1, base)->text, "5"));
    assert(text("Deathmatch | Toujane") && count(TRI, NULL) == 1);
    reset("dm");
    scores("b 0 0 0");
    render(1.0f);
    assert(stripes(red, NULL, NULL) == 18 && !text("AXIS"));
    /* A mod's free-for-all is told by its players' teams. */
    reset("gg");
    cg->clientNum = 3;
    player(3, "Me", TEAM_FREE);
    player(4, "Other", TEAM_FREE);
    scores("b 2 0 0 4 500 20 1 0 3 400 30 2 0");
    render(1.0f);
    assert(stripes(teal, NULL, NULL) == 1 && stripes(red, NULL, NULL) == 17 && !text("AXIS"));

    /* The emblems traced from the capture, in units of half their size. */
    enum { AMERICAN = TABLE_EMBLEM_AMERICAN, BRITISH = TABLE_EMBLEM_BRITISH, RUSSIAN = TABLE_EMBLEM_RUSSIAN,
           GERMAN = TABLE_EMBLEM_GERMAN };
    assert(at(AMERICAN, 0, 0) < -0.2f && at(AMERICAN, 0.95f, -90) < 0 && at(AMERICAN, 0.92f, -54) < -0.05f);
    assert(at(AMERICAN, 0.95f, -86) > 0 && at(AMERICAN, 0.95f, -94) > 0 && at(AMERICAN, 0.7f, -54) > 0.1f);
    assert(at(AMERICAN, 1.1f, -54) > 0.05f && at(AMERICAN, 0.92f, 90) < -0.05f);
    assert(at(BRITISH, 0, 0) < -0.3f && at(BRITISH, 0.65f, 10) > 0.1f && at(BRITISH, 0.9f, 10) < -0.05f);
    assert(at(BRITISH, 1.1f, 10) > 0.05f);
    assert(at(RUSSIAN, 0, 0) < -0.15f && at(RUSSIAN, 0.92f, -90) < 0 && at(RUSSIAN, 0.72f, -90) > 0);
    assert(at(RUSSIAN, 0.5f, -90) < 0 && at(RUSSIAN, 1.1f, -90) > 0 && at(RUSSIAN, 0.6f, -54) > 0);
    assert(at(GERMAN, 0, 0) < -0.05f && at(GERMAN, 0.95f, -90) < 0 && at(GERMAN, 0.865f, -90) > 0);
    assert(at(GERMAN, 0.6f, -90) < 0 && at(GERMAN, 0.95f, 0) < 0 && at(GERMAN, 0.865f, 180) > 0);
    assert(CG_ScoreTableEmblemDistance(&s_table, GERMAN, 0.9f, -0.9f) > 0.1f);
    rasterOf(0, AMERICAN);
    rasterOf(1, GERMAN);
    assert(alphaAt(0, 47, 47) == 255 && alphaAt(1, 47, 47) == 255);

    /* Built once for a size and nation; the British and Russians get theirs,
     * an unknown banner leaves the Allies without one. */
    reset("tdm");
    cg->clientNum = 5;
    player(5, "Me", TEAM_ALLIES);
    player(7, "Them", TEAM_AXIS);
    scores("b 2 1 2 5 10 5 0 0 7 5 5 0 0");
    render(1.0f);
    render(1.0f);
    assert(emblemBuilds == 1);
    alliesBanner = "mpflag_british";
    render(1.0f);
    assert(emblemBuilds == 2);
    rasterOf(0, BRITISH);
    rasterOf(1, GERMAN);
    alliesBanner = "mpflag_Russian";
    render(1.0f);
    assert(emblemBuilds == 3);
    rasterOf(0, RUSSIAN);
    alliesBanner = "";
    render(1.0f);
    assert(emblemBuilds == 4 && count(PHYS, &emblemMaterial) == 1 && phys(150, 364, 96, 96, textRgb, 0.55f));
    rasterOf(0, -1);
    alliesBanner = "mpflag_german";
    render(1.0f);
    assert(emblemBuilds == 4 && count(PHYS, &emblemMaterial) == 1);
    alliesBanner = "mpflag_american";
    unitScale = 1.5f;
    render(1.0f);
    assert(emblemBuilds == 5 && emblemHeight == 120 && phys(155, 195, 120, 120, textRgb, 0.55f));
    {
        /* Where rounding each edge would make bars of 4 and 3 pixels, the
         * meter's bars are all as wide and as far apart. */
        const Draw *bars[4];
        int n = 0;
        for (int i = 0; i < drawCount && n < 4; ++i)
            if (draws[i].kind == PHYS && colored(draws[i].color, pingGood, 1))
                bars[n++] = &draws[i];
        assert(n == 4);
        for (int i = 1; i < 4; ++i)
            assert(bars[i]->w == bars[0]->w && bars[i]->x - bars[i - 1]->x == bars[1]->x - bars[0]->x &&
                   bars[i]->h > bars[i - 1]->h && bars[i]->y + bars[i]->h == bars[0]->y + bars[0]->h);
        assert(bars[0]->w == 4 && bars[1]->x - bars[0]->x == 5);
    }
    rasterOf(0, AMERICAN);
    unitScale = 0.1f;
    render(1.0f);
    assert(emblemBuilds == 6 && emblemHeight == 16);
    unitScale = 10.0f;
    render(1.0f);
    assert(emblemBuilds == 7 && emblemHeight == 384);
    rasterOf(1, GERMAN);
    unitScale = 1.2f;
    failBuild = 1;
    render(1.0f);
    render(1.0f);
    assert(emblemBuilds == 8 && count(PHYS, &emblemMaterial) == 0 && text("ALLIES"));
    failBuild = 0;
    unitScale = 1.25f;
    render(1.0f);
    assert(emblemBuilds == 9 && count(PHYS, &emblemMaterial) == 2);
    unitScale = 1.2f;

    /* The end of a match shows them as lit medals: clear a texel round the
     * outline and opaque inside it, each part in its enamel or metal, and each
     * face brighter where it leans toward the light at the top left than
     * where it leans away. Parts are told by their own outlines, a texel clear
     * of each. */
    static byte medal[96 * 96 * 4];
    for (int kind = TABLE_EMBLEM_NONE; kind <= GERMAN; ++kind) {
        const float units = 47.0f, edge = 1.0f / 47.0f;
        float lit = 0.0f, shaded = 0.0f;
        int litCount = 0, shadedCount = 0;
        memset(medal, 0xab, sizeof(medal));
        CG_ScoreTableRasterizeMedal(kind, medal, 96 * 4, 96, 0, 96);
        /* Built in bands, it writes only each band's rows, the same texels. */
        static byte banded[96 * 96 * 4];
        memset(banded, 0xcd, sizeof(banded));
        CG_ScoreTableRasterizeMedal(kind, banded, 96 * 4, 96, 40, 7);
        for (int i = 0; i < 96 * 96 * 4; ++i)
            assert(i / (96 * 4) >= 40 && i / (96 * 4) < 47 ? banded[i] == medal[i] : banded[i] == 0xcd);
        for (int first = -5; first < 96; first += 13)
            CG_ScoreTableRasterizeMedal(kind, banded, 96 * 4, 96, first, 13);
        CG_ScoreTableRasterizeMedal(kind, banded, 96 * 4, 96, 96, 40);
        assert(!memcmp(banded, medal, sizeof(medal)));
        for (int y = 0; y < 96; ++y) {
            for (int x = 0; x < 96; ++x) {
                const byte *t = &medal[(y * 96 + x) * 4];
                float u = ((float)x + 0.5f - 48.0f) / units, v = ((float)y + 0.5f - 48.0f) / units;
                float outline = CG_ScoreTableMedalOutline(&s_table, kind, u, v);
                float r = sqrtf(u * u + v * v);
                int points = kind == RUSSIAN ? TABLE_STAR_POINTS : TABLE_CROSS_POINTS;
                float band = CG_ScoreTablePolygonDistance(kind == RUSSIAN ? s_table.redStar[1] : s_table.cross[1],
                                                          points, u, v);
                float core = CG_ScoreTablePolygonDistance(kind == RUSSIAN ? s_table.redStar[2] : s_table.cross[2],
                                                          points, u, v);
                if (outline > edge) {
                    assert(!t[0] && !t[1] && !t[2] && !t[3]);
                    continue;
                }
                if (!x || !y || x == 95 || y == 95) {
                    assert(!t[3]);
                    continue;
                }
                if (outline >= -edge)
                    continue;
                assert(t[3] == 255);
                CG_ScoreTableField lean = { 0.0f, 0.0f, 0.0f };
                if (kind == AMERICAN)
                    CG_ScoreTablePolygonField(s_table.star, TABLE_STAR_POINTS, u, v, &lean);
                else if (kind == BRITISH && r > 0.1f && r < 0.5f - edge)
                    lean.gx = u / r, lean.gy = v / r;
                else if ((kind == RUSSIAN || kind == GERMAN) && core < -edge)
                    CG_ScoreTablePolygonField(kind == RUSSIAN ? s_table.redStar[2] : s_table.cross[2], points, u, v,
                                              &lean);
                if (-(lean.gx + lean.gy) > 0.7f) {
                    lit += t[0] + t[1] + t[2];
                    ++litCount;
                } else if (lean.gx + lean.gy > 0.7f) {
                    shaded += t[0] + t[1] + t[2];
                    ++shadedCount;
                }
                switch (kind) {
                case AMERICAN:
                    assert(t[2] > t[1] && t[1] > t[0]);
                    break;
                case BRITISH:
                    if (r < 0.5f - edge)
                        assert(t[2] > 2 * t[1] && t[2] > 2 * t[0]);
                    else if (r > 0.5f + edge && r < 0.78f - edge)
                        assert(t[0] > 150 && t[1] > 150 && t[2] > 150);
                    else if (r > 0.78f + edge)
                        assert(t[0] > 2 * t[2]);
                    break;
                case RUSSIAN:
                    if (band > edge)
                        assert(t[2] >= t[1] && t[1] > t[0] && t[2] > t[0] + 20);
                    else if (band < -edge && core > edge)
                        assert(t[2] < 90 && t[2] > 2 * t[1]);
                    else if (core < -edge)
                        assert(t[2] > 2 * t[1] && t[2] > 2 * t[0]);
                    break;
                case GERMAN:
                    if (band > edge)
                        assert(t[2] > 50 && abs(t[2] - t[1]) < 8 && abs(t[2] - t[0]) < 20);
                    else if (band < -edge && core > edge)
                        assert(t[0] < 30 && t[1] < 30 && t[2] < 30);
                    else if (core < -edge)
                        assert(t[2] > 15 && t[2] < 130 && abs(t[2] - t[0]) < 15);
                    break;
                }
            }
        }
        if (kind != TABLE_EMBLEM_NONE)
            assert(litCount > 50 && shadedCount > 50 && lit / litCount > 1.1f * shaded / shadedCount);
    }

    /* Off, it builds nothing and leaves the HUD alone; shown only while held. */
    displayed = 0;
    assert(!CG_ScoreTableHidesHud());
    displayed = 1;
    scoreboardDvar.current.enabled = 0;
    CG_ScoreTablePrepare();
    assert(emblemBuilds == 9 && !CG_ScoreTableEnabled() && !CG_ScoreTableHidesHud());
    scoreboardDvar.current.enabled = 1;
    assert(CG_ScoreTableHidesHud());
    cg->clientNum = 64;
    drawCount = 0;
    CG_DrawScoreTable(1.0f);
    assert(drawCount == 0);
    return 0;
}
'''


def mutate(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


module_mutants = [
    ('CG_ScoreTableColor(color, teal ? s_tableFriendly : s_tableEnemy,',
     'CG_ScoreTableColor(color, teal ? s_tableEnemy : s_tableFriendly,'),
    ('((i & 1) ? TABLE_STRIPE_DARK : TABLE_STRIPE_LIGHT) * fade', '((i & 1) ? TABLE_STRIPE_LIGHT : TABLE_STRIPE_DARK) * fade'),
    ('int top = local->infoValid && local->team == TEAM_AXIS ? TEAM_AXIS : TEAM_ALLIES;', 'int top = TEAM_ALLIES;'),
    ('row = &block->rows[i == rows - 1 && own > i ? own : i];', 'row = &block->rows[i];'),
    ('if (height / (float)rows < TABLE_MIN_ROW_HEIGHT)', 'if (0)'),
    ('if (row->kills >= 0)', 'if (1)'),
    ('#define TABLE_ROW_HEIGHT 17.7f', '#define TABLE_ROW_HEIGHT 18.0f'),
    ('#define TABLE_TOP 90.8f', '#define TABLE_TOP 92.0f'),
    ('#define TABLE_EMBLEM_SIZE 80.0f', '#define TABLE_EMBLEM_SIZE 64.0f'),
    ('|| close - c > 10 ||', '|| close - c > 20 ||'),
    ("    while (c[0] == '^' && c[1] >= '0' && c[1] <= '9') {\n        color = c[1];",
     "    while (c[0] == '^' && c[1] >= '0' && c[1] <= '9') {\n        ;"),
    ('while (count > 1 && CG_ScoreTableTextWidth(text, count, font, scale) > room)', 'while (0)'),
    ('CG_ScoreTableTextWidth(text, count, font, scale)', 'CG_ScoreTableTextWidth(text, 0, font, scale)'),
    ('return (float)CL_TextWidth(text, maxChars, font) * CL_NormalizedTextScale(font, scale);',
     'return (float)(int)((float)CL_TextWidth(text, maxChars, font) * CL_NormalizedTextScale(font, scale));'),
    ('baseline + (rise + 0.5f) * unit', 'baseline + 0.5f * unit'),
    ('x + 0.5f * unit, baseline + (rise + 0.5f) * unit', 'x, baseline + rise * unit'),
    ('{ 398.6f, 482.5f, 567.2f }', '{ 396.5f, 481.5f, 567.0f }'),
    ('        if (font->pixelHeight == s_tableFonts[i].pixelHeight) {', '        if (0) {'),
    ('#define TABLE_NAME_FONT_CAP 0.7f', '#define TABLE_NAME_FONT_CAP 0.75f'),
    ('    if (font == CG_GetPlayerNameFont()) {\n        *cap = TABLE_NAME_FONT_CAP', '    if (0) {\n        *cap = TABLE_NAME_FONT_CAP'),
    ('{ 12, 7.0f, 2.0f }', '{ 12, 7.5f, 2.0f }'),
    ('{ 16, 11.0f, 2.0f }', '{ 16, 11.0f, 3.0f }'),
    ('{ 24, 13.12f, 5.0f }', '{ 24, 13.12f, 3.0f }'),
    ('{ 30, 17.1f, 6.0f }', '{ 30, 17.1f, 5.0f }'),
    ('{ 32, 21.0f, 5.0f }', '{ 32, 20.0f, 5.0f }'),
    ('{ 2.0f, 3.5f, 5.05f, 6.55f }', '{ 2.5f, 4.0f, 5.5f, 7.0f }'),
    ('if (size != s_table.emblemSize || allies != s_table.emblemAllies)', 'if (1)'),
    ('float s0 = block->team == TEAM_AXIS ? 0.5f : 0.0f;', 'float s0 = block->team == TEAM_AXIS ? 0.0f : 0.5f;'),
    ('cut = -(star - 0.08f);', 'cut = -1.0f;'),
    ('CG_ScoreTableInset(state->cross[0], TABLE_CROSS_POINTS, 0.18f, state->cross[2]);',
     'CG_ScoreTableInset(state->cross[0], TABLE_CROSS_POINTS, 0.09f, state->cross[2]);'),
    ('CG_ScoreTableInset(state->redStar[0], TABLE_STAR_POINTS, 0.12f, state->redStar[2]);',
     'CG_ScoreTableInset(state->redStar[0], TABLE_STAR_POINTS, 0.06f, state->redStar[2]);'),
    ('return radius - 0.5f < ring ? radius - 0.5f : ring;', 'return ring;'),
    ('coverage = 0.5f - CG_ScoreTableEmblemDistance(state, kind, u, v) * units;',
     'coverage = CG_ScoreTableEmblemDistance(state, kind, u, v) < 0.0f ? 1.0f : 0.0f;'),
    ('CG_ScoreTableRasterize(state, TABLE_EMBLEM_GERMAN, bgra + size * 4, size * 8, size);',
     'CG_ScoreTableRasterize(state, TABLE_EMBLEM_GERMAN, bgra, size * 8, size);'),
    ('return CG_ScoreTableEnabled() && CG_ScoreboardDisplayed();', 'return CG_ScoreTableEnabled();'),
    ('s_table.kills[clientNum] = kills < 0 ? 0 : kills;', 's_table.kills[clientNum] = kills;'),
    ('    CG_ScoreTableFillPixels(x, y, x + w, y + h, color);\n', ''),
    ('    *py = floorf(y + 0.5f);', '    *py = y;'),
    ('        if (score->team == TEAM_SPECTATOR) {', '        if (0) {'),
    ('if (!I_stricmp(cgs->gametype, "dm"))\n        return 1;', 'if (0)\n        return 1;'),
    ('return cg->teamPlayers[TEAM_FREE] > 0 && !cg->teamPlayers[TEAM_AXIS] && !cg->teamPlayers[TEAM_ALLIES];',
     'return 0;'),
    ('CG_ScoreTableColor(color, s_tableBlack, TABLE_CELL_SHADE * fade);',
     'CG_ScoreTableColor(color, s_tableBlack, 0.0f);'),
    ('    if (comma && comma != buf)\n        *comma = 0;\n', ''),
    ('        scale *= (TABLE_TITLE_RIGHT - TABLE_TITLE_X) / width;\n', ''),
    ('    if (color[0] == 1.0f && color[1] == 1.0f && color[2] == 1.0f)\n', '    if (1)\n'),
    ('    if (!font)\n        font = UI_GetFontHandle(0, cap / 30.0f);', '    font = UI_GetFontHandle(0, cap / 30.0f);'),
    ('(float)-glyph->y0 : 0.625f * (float)font->pixelHeight;', '(float)-glyph->y0 : 0.5f * (float)font->pixelHeight;'),
    ('*cap = glyph && glyph->y0 < 0 ? (float)-glyph->y0 :', '*cap = glyph && glyph->y0 < 0 ? (float)-glyph->y0 + 1.0f :'),
    ('    if (ping < TABLE_PING_GOOD)\n        return 4;', '    if (ping < 30)\n        return 4;'),
    ('#define TABLE_PING_FAIR 120', '#define TABLE_PING_FAIR 121'),
    ('lit >= 4 ? s_tablePingGood : lit == 3 ? s_tablePingFair : s_tablePingPoor', 'lit >= 3 ? s_tablePingGood : s_tablePingPoor'),
    ('    bar = barRight > left ? barRight - left : 1.0f;', '    bar = 2.0f;'),
    ('    pitch = nextLeft - left;', '    pitch = nextLeft - left + 1.0f;'),
    ('        if (i < lit)\n            CG_ScoreTableColor(color, rgb, fade);', '        if (i <= lit)\n            CG_ScoreTableColor(color, rgb, fade);'),
    ('        nameRight = TABLE_TAG_RIGHT - width - TABLE_NAME_GAP;\n', ''),
    ('    CG_ScoreTableDrawTriangle((const float (*)[2])marker, color);\n', ''),
    ('    if ((unsigned)localClient >= TABLE_MAX_CLIENTS)\n        return;\n', '    if (localClient > TABLE_MAX_CLIENTS)\n        return;\n'),
    ('#define TABLE_RESULT_BASELINE 37.1f', '#define TABLE_RESULT_BASELINE 38.0f'),
    ('#define TABLE_REASON_BASELINE 79.7f', '#define TABLE_REASON_BASELINE 78.0f'),
    ('#define TABLE_REASON_CAP 8.0f', '#define TABLE_REASON_CAP 8.85f'),
    ('    if (result)\n        CG_ScoreTableDrawResult(result, reason, fade);\n    else\n        CG_ScoreTableDrawClock(fade);\n',
     '    if (result)\n        CG_ScoreTableDrawResult(result, reason, fade);\n    CG_ScoreTableDrawClock(fade);\n'),
    ('                          CG_ScoreTableCapScale(font, TABLE_REASON_CAP), color, TABLE_TEXT_STYLE);',
     '                          CG_ScoreTableCapScale(font, TABLE_REASON_CAP), color, TABLE_TITLE_STYLE);'),
    ('    s_table.regularFont = R_RegisterWwiiFont("$regularfont");\n', ''),
    ('    CG_ScoreTableDraw(fade, result ? result : "", reason);', '    CG_ScoreTableDraw(fade, result, reason);'),
    ('texel[2 - i] = ', 'texel[i] = '),
    ('static const vec3_t s_medalLight = { -0.5f, -0.6f, 0.62f };',
     'static const vec3_t s_medalLight = { 0.5f, 0.6f, 0.62f };'),
    ('CG_ScoreTableMedalFace(s_medalRed, gx, gy, TABLE_MEDAL_DOME_SLOPE * radius, coverage[2], ink);',
     'CG_ScoreTableMedalFace(s_medalBlue, gx, gy, TABLE_MEDAL_DOME_SLOPE * radius, coverage[2], ink);'),
    ('s_medalGold, s_medalDarkRed, s_medalRed,', 's_medalRed, s_medalDarkRed, s_medalGold,'),
    ('s_medalSilver, s_medalGroove, s_medalIron,', 's_medalIron, s_medalGroove, s_medalSilver,'),
    ('texel[3] = (byte)(ink[3] / (float)(TABLE_MEDAL_SAMPLES * TABLE_MEDAL_SAMPLES) * 255.0f + 0.5f);',
     'texel[3] = (byte)(ink[3] / 4.5f * 255.0f + 0.5f);'),
    ('            texel[0] = texel[1] = texel[2] = texel[3] = 0;\n            if (CG_ScoreTableMedalOutline',
     '            if (CG_ScoreTableMedalOutline'),
    ('y = firstRow < 0 ? 0 : firstRow; y < firstRow + rows && y < size;', 'y = 0; y < size;'),
    ('y < firstRow + rows && y < size;', 'y < firstRow + rows;'),
    ('y = firstRow < 0 ? 0 : firstRow;', 'y = firstRow;'),
]
parse_mutants = [
    ('if (Cmd_Argc() >= 4 + scoreCount * 6) {', 'if (1) {'),
    ('    CG_ScoreTableClearKills();\n', ''),
    ('atoi(CG_Argv(4 + scoreCount * 5 + i))', 'atoi(CG_Argv(4 + scoreCount * 5 + i + 1))'),
]
variants = [(body, parse)]
variants += [(mutate(body, old, new), parse) for old, new in module_mutants]
variants += [(body, mutate(parse, old, new)) for old, new in parse_mutants]
with tempfile.TemporaryDirectory(prefix='cod2-score-table-') as directory:
    path = Path(directory) / 'table.c'
    binary = Path(directory) / 'table'
    for index, (table_source, parse_source) in enumerate(variants):
        path.write_text(support + table_source + '\n' + parse_source + '\n' + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wno-unused-function', '-Wno-address', '-Wno-pointer-bool-conversion', str(path), '-o',
                        str(binary), '-lm'], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
print('PASS: the WWII scoreboard dims the screen and draws the title, clock and columns where the capture has them, '
      'their capitals as tall and on the baselines it measures by the fonts\' own metrics, '
      'the viewer\'s team on top in teal (Allies for spectators) with nine pixel-snapped stripes a team, its name, '
      'score and emblem, rows in server order with clan tags, cut names, voice and status icons, kills only from our '
      'server, a ping meter of four even bars (four green under 60 ms, three yellow under 120, two red under 200, one red '
      'above) and the viewer\'s outlined row kept in a crowded team; deathmatch gets one block of eighteen; '
      'the star, roundel, red star and iron cross are built once per size and nation; the HUD hides under it; '
      'at the end of a match the result and its reason take the clock\'s place, and the emblems become lit medals '
      'in their enamels and metals; '
      f'{len(variants) - 1} failing regression mutants')
