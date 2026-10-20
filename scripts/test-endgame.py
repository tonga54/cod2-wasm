#!/usr/bin/env python3
"""Run the production WWII end of a match on recorded draws: who won, both sides and why, then the scoreboard."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parent.parent
module = (root / 'src/PC/cgame_mp/cg_endgame_mp.c').read_text()
header = (root / 'src/headers/PC/cgame_mp/cg_endgame.h').read_text()
table_header = (root / 'src/headers/PC/cgame_mp/cg_scoretable.h').read_text()
main = (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()
draw = (root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text()
servercmds = (root / 'src/PC/cgame_mp/cg_servercmds_mp.c').read_text()
scoreboard = (root / 'src/PC/cgame_mp/cg_scoreboard_mp.c').read_text()
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


# The cgame builds it after the killcam; every frame, before anything else
# can return, it learns whether the server has the viewer at intermission;
# the stock game messages tell it why the match ended; it draws in the
# scoreboard's place while the WWII table is on, and its medals have an image
# slot of their own.
for source in (main, draw, servercmds, scoreboard):
    assert source.count('#include "headers/PC/cgame_mp/cg_endgame.h"\n') == 1
assert main.count('    CG_KillcamInit();\n    CG_EndgameInit();\n') == 1
ordered(function(draw, 'unsigned int CG_Draw2D(void)\n{'),
        '    CG_EndgameFrame(cg->nextSnap && cg->nextSnap->ps.pm_type == 5);\n    if (cg->cubemapShot)\n        return 0;\n',
        '    if (!cg_draw2D->current.enabled)\n        return 0;\n',
        '        cg->scoreFadeTime = cg->time;\n        CG_DrawScoreboard();\n        return 0;\n')
assert servercmds.count("    case 'e':\n    case 'f':\n        CG_EndgameMessage(CG_Argv(1));\n"
                        "        CG_ServerCommandMessage(\"game message\", 0);\n        return;\n") == 1
assert scoreboard.count('    if (CG_ScoreTableEnabled()) {\n        if (!CG_DrawEndgame(fade))\n'
                        '            CG_DrawScoreTable(fade);\n        return 1;\n    }\n') == 1
slots = {name: int(value) for name, value in re.findall(r'^#define (R_HUD_IMAGE_\w+) (\d+)$', hud2d_header, re.M)}
names = re.findall(r'"([^"]+)"', re.search(r's_hudImageNames\[R_HUD_IMAGE_COUNT\] = \{(.*?)\};', hud2d, re.S).group(1))
assert len(names) == slots['R_HUD_IMAGE_COUNT'] and len(set(names)) == len(names)
assert names[slots['R_HUD_IMAGE_ENDGAME']] == '*hud_endgame'

# The five modes hold the intermission as long as the client's timeline, which
# keeps at least 10 s of scoreboard after the announcement and its handover.
sys.path.insert(0, str(root / 'scripts'))
import browser_endgame
timeline = {name: int(value) for name, value in re.findall(r'^#define (ENDGAME_\w+_MSEC) (\d+)$', module, re.M)}
assert timeline['ENDGAME_INTERMISSION_MSEC'] == browser_endgame.INTERMISSION_SECONDS * 1000
assert timeline['ENDGAME_INTERMISSION_MSEC'] - timeline['ENDGAME_ANNOUNCE_MSEC'] - timeline['ENDGAME_HANDOVER_MSEC'] \
    >= 10000
retail = b'\tsendranks();\r\n\t}\r\n\r\n\twait 10;\r\n\texitLevel(false);\r\n}\r\n'
for gametype in ('ctf', 'dm', 'hq', 'sd', 'tdm'):
    name = f'maps/mp/gametypes/{gametype}.gsc'
    assert browser_endgame.apply_endgame_timing(name, retail) == retail.replace(b'wait 10;', b'wait 16;'), name
    for unexpected in (retail.replace(b'wait 10;', b'wait 12;'), retail + retail):
        try:
            browser_endgame.apply_endgame_timing(name, unexpected)
        except ValueError:
            continue
        raise AssertionError('unexpected end of a match accepted: ' + name)
assert browser_endgame.apply_endgame_timing('maps/mp/gametypes/_killcam.gsc', retail) == retail
assert browser_endgame.apply_endgame_timing('maps/mp/mp_toujane.gsc', retail) == retail

# The fonts' capitals and baselines, from their own profiles.
profiles = {name: (int(pixels), float(cap), int(base)) for name, pixels, cap, base in
            re.findall(r'\{ "(\w+)", R_WWII_\w+, (\d+), ([\d.]+)f, (\d+), [\d.]+f, [01] \}', wwii_fonts)}
bold, regular = profiles['boldFont'], profiles['regularFont']

body = module
for include in ('#include "common_types.h"\n', '#include "imports.h"\n', '#include "headers/PC/cgame_mp/cg_local.h"\n',
                '#include "headers/PC/gfx_d3d/r_font_wwii.h"\n', '#include "headers/PC/gfx_d3d/r_hud2d.h"\n'):
    assert body.count(include) == 1, include
    body = body.replace(include, '')
for include, text in (('#include "headers/PC/cgame_mp/cg_endgame.h"\n', header),
                      ('#include "headers/PC/cgame_mp/cg_scoretable.h"\n', table_header)):
    assert body.count(include) == 1, include
    body = body.replace(include, text)

support = r'''
#include <assert.h>
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
#define R_HUD_IMAGE_ENDGAME ''' + str(slots['R_HUD_IMAGE_ENDGAME']) + r'''
typedef struct { int infoValid; char name[32]; int team; } clientInfo_t;
typedef struct { int client; int score; int ping; int deaths; int team; MaterialHandle hStatusIcon; } score_t;
typedef struct {
    int clientNum;
    int time;
    int numScores;
    score_t scores[64];
    char objectiveText[1024];
    struct { clientInfo_t clientinfo[64]; } bgs;
} cg_t;
typedef struct { char gametype[32]; int teamScores[2]; struct { MaterialHandle whiteMaterial; } media; } cgs_t;
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
static Material white = { "white" }, medals = { "*hud_endgame" };
static dvar_t endgameDvar = { "cg_hudEndgame" };
static int registrations;
const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags) {
    assert(!strcmp(name, "cg_hudEndgame") && value == 1 && flags == 0x1001);
    ++registrations;
    endgameDvar.current.enabled = value;
    return &endgameDvar;
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
/* The owner's music, when the archive has it, and the music channel's calls. */
typedef struct snd_alias_t { const char *name; } snd_alias_t;
typedef struct snd_alias_list_t { snd_alias_t *head; } snd_alias_list_t;
typedef int snd_alias_system_t;
#define SASYS_CGAME 1
static snd_alias_t endgameMusic = { "browser_endgame_music" };
static snd_alias_list_t endgameMusicList = { &endgameMusic };
static int haveMusic = 1, musicPlays, musicStops, lastFade = -1;
snd_alias_list_t *Com_FindSoundAlias(const char *name) {
    assert(!strcmp(name, "browser_endgame_music"));
    return haveMusic ? &endgameMusicList : NULL;
}
snd_alias_t *Com_PickSoundAliasFromList(snd_alias_list_t *list) { return list->head; }
void SND_PlayMusicAlias(const snd_alias_t *alias, snd_alias_system_t system) {
    assert(alias == &endgameMusic && system == SASYS_CGAME);
    ++musicPlays;
}
void SND_StopMusic(int fade) { ++musicStops; lastFade = fade; }

/* The table's lettering: the fonts' capitals and baselines in font units. */
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
/* Every letter advances nine twentieths of the font's pixel height, about
 * a bold digit's; color codes take no room. */
static int visible(const char *text) {
    int n = 0;
    for (const char *c = text; *c; ++c) {
        if (c[0] == '^' && c[1] >= '0' && c[1] <= '9') {
            ++c;
            continue;
        }
        ++n;
    }
    return n;
}
static int advanceOf(FontHandle font) { return font->pixelHeight * 9 / 20; }
float CG_ScoreTableTextWidth(const char *text, int maxChars, FontHandle font, float scale) {
    assert(maxChars == 0);
    return (float)(visible(text) * advanceOf(font)) * CL_NormalizedTextScale(font, scale);
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
/* Each band of a medal filled with its kind, so the image shows where each
 * went; every call is kept. */
static int rasters, rasterKinds[1024], rasterStrides[1024], rasterSizes[1024], rasterFirst[1024], rasterRows[1024];
void CG_ScoreTableRasterizeMedal(int kind, byte *bgra, int stride, int size, int firstRow, int rows) {
    assert(rasters < 1024 && firstRow >= 0 && rows > 0 && firstRow + rows <= size);
    rasterKinds[rasters] = kind;
    rasterStrides[rasters] = stride;
    rasterSizes[rasters] = size;
    rasterFirst[rasters] = firstRow;
    rasterRows[rasters] = rows;
    ++rasters;
    for (int y = firstRow; y < firstRow + rows; ++y)
        for (int x = 0; x < size; ++x)
            memset(bgra + y * stride + x * 4, kind + 2, 4);
}
static byte image[2 * 384 * 384 * 4];
static int builds, imageWidth, imageHeight, failBuild;
MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height) {
    assert(slot == R_HUD_IMAGE_ENDGAME && width == 2 * height && width * height * 4 <= (int)sizeof(image));
    memcpy(image, bgra, (size_t)width * (size_t)height * 4);
    imageWidth = width;
    imageHeight = height;
    ++builds;
    return failBuild ? NULL : &medals;
}

enum { PHYS, TEXT, TRI, TABLE };
typedef struct {
    int kind;
    float x, y, w, h, s0, t0, s1, t1, scale;
    float color[4];
    MaterialHandle material;
    FontHandle font;
    char text[160];
    int style, vertexCount, indexCount;
    HudVertex2D verts[16];
    unsigned short indices[54];
} Draw;
static Draw draws[512];
static int drawCount;
static Draw *record(int kind, const float *color) {
    Draw *d;
    assert(drawCount < 512);
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
    assert(horzAlign == 0 && vertAlign == 0 && maxChars == 0x7fffffff);
    snprintf(d->text, sizeof(d->text), "%s", text);
    d->font = font;
    d->x = x;
    d->y = y;
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
    assert(vertexCount <= 16 && indexCount <= 54 && indexCount % 3 == 0);
    for (int i = 0; i < indexCount; ++i)
        assert(indices[i] < vertexCount);
    memcpy(d->verts, verts, sizeof(HudVertex2D) * (size_t)vertexCount);
    memcpy(d->indices, indices, sizeof(unsigned short) * (size_t)indexCount);
    d->vertexCount = vertexCount;
    d->indexCount = indexCount;
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
    if (!strcmp(gameType, "tdm"))
        return "MPUI_TEAM_DEATHMATCH";
    if (!strcmp(gameType, "dm"))
        return "MPUI_DEATHMATCH";
    return gameType;
}
const char *SEH_LocalizeTextMessage(const char *msg, const char *context, int errType) {
    assert(errType == 0);
    if (!strcmp(context, "scoreboard gametype display")) {
        if (!strcmp(msg, "MPUI_TEAM_DEATHMATCH"))
            return "Team Deathmatch";
        if (!strcmp(msg, "MPUI_DEATHMATCH"))
            return "Deathmatch";
        return msg;
    }
    assert(!strcmp(context, "endgame reason"));
    if (!strcmp(msg, "MP_TIME_LIMIT_REACHED"))
        return "Time limit reached.";
    if (!strcmp(msg, "MP_SCORE_LIMIT_REACHED"))
        return "Score limit reached.";
    assert(!strcmp(msg, "MP_ROUND_LIMIT_REACHED"));
    return "Round limit reached";
}
void I_strncpyz(char *dest, const char *src, int size) { strncpy(dest, src, (size_t)size - 1); dest[size - 1] = 0; }
int Com_sprintf(char *dest, int size, const char *fmt, ...) {
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsnprintf(dest, (size_t)size, fmt, ap);
    va_end(ap);
    return n;
}
/* The scoreboard under the result, as one recorded draw. */
static char tableResult[96], tableReason[64];
void CG_DrawScoreTableResult(float fade, const char *result, const char *reason) {
    Draw *d = record(TABLE, NULL);
    d->color[3] = fade;
    snprintf(tableResult, sizeof(tableResult), "%s", result);
    snprintf(tableReason, sizeof(tableReason), "%s", reason);
}
'''

checks = r'''
static int near(float a, float b) { return fabsf(a - b) < 2e-3f; }
static const float textRgb[3] = { 0.82f, 0.79f, 0.74f };
static const float black[3] = { 0.0f, 0.0f, 0.0f };
static const float whiteRgb[3] = { 1.0f, 1.0f, 1.0f };
static int colored(const float *color, const float *rgb, float alpha) {
    return near(color[0], rgb[0]) && near(color[1], rgb[1]) && near(color[2], rgb[2]) && near(color[3], alpha);
}
static float px(float x) { return x * unitScale + offsetX; }
static float py(float y) { return y * unitScale; }

static int frame(int intermission, int time, float fade) {
    cg->time = time;
    CG_EndgameFrame(intermission);
    drawCount = 0;
    tableResult[0] = tableReason[0] = 0;
    return CG_DrawEndgame(fade);
}
static const Draw *text(const char *s) {
    for (int i = 0; i < drawCount; ++i)
        if (draws[i].kind == TEXT && !strcmp(draws[i].text, s))
            return &draws[i];
    return NULL;
}
static int indexOf(const Draw *d) { return d ? (int)(d - draws) : -1; }
static int count(int kind) {
    int n = 0;
    for (int i = 0; i < drawCount; ++i)
        n += draws[i].kind == kind;
    return n;
}
/* Where a text's pen, capitals and middle land by the font's metrics once
 * the renderer moves it half a unit up and to the left. */
static float unitOf(const Draw *d) { return 48.0f * d->scale / (float)d->font->pixelHeight; }
static float penOf(const Draw *d) { return d->x - 0.5f * unitOf(d); }
static float capOf(const Draw *d) { return metricsOf(d->font).cap * unitOf(d); }
static float baselineOf(const Draw *d) { return d->y - (metricsOf(d->font).base + 0.5f) * unitOf(d); }
static float widthOf(const Draw *d) { return (float)(visible(d->text) * advanceOf(d->font)) * unitOf(d); }
/* Shadowed text centered on x, its capitals cap tall on the baseline. */
static int centered(const Draw *d, float x, float baseline, float cap, FontHandle font, float alpha) {
    return d && d->font == font && near(penOf(d) + 0.5f * widthOf(d), x) && near(baselineOf(d), baseline) &&
           near(capOf(d), cap) && d->style == 3 && colored(d->color, textRgb, alpha);
}
/* A pixel-thick line in the row holding y, between whole pixels nearest x0 and x1. */
static const Draw *line(float x0, float x1, float y, float alpha) {
    float left = floorf(px(x0) + 0.5f), right = floorf(px(x1) + 0.5f);
    for (int i = 0; i < drawCount; ++i) {
        const Draw *d = &draws[i];
        if (d->kind == PHYS && d->x == left && d->w == right - left && d->y == floorf(py(y)) && d->h == 1.0f &&
            d->material == &white && colored(d->color, textRgb, alpha))
            return d;
    }
    return NULL;
}
/* A diamond's fill and its outline's middle at half either way of (x, y),
 * the outline's sides faded out a pixel across its edges. */
static const Draw *diamond(float x, float y, float half, float fill, float edge) {
    static const float corners[4][2] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    float r = half * unitScale;
    for (int i = 0; i < drawCount; ++i) {
        const Draw *d = &draws[i];
        int ok = d->kind == TRI && d->vertexCount == 16 && d->indexCount == 54 && d->material == &white;
        for (int c = 0; ok && c < 4; ++c) {
            for (int ring = -1; ring <= 2; ++ring) {
                const HudVertex2D *v = &d->verts[ring < 0 ? c : 4 + ring * 4 + c];
                float reach = ring < 0 ? r : r + (float)(ring - 1) * 1.41421356f;
                ok = ok && near(v->xy[0], px(x) + corners[c][0] * reach) && near(v->xy[1], py(y) + corners[c][1] * reach);
                ok = ok && (ring < 0 ? colored(v->rgba, black, fill) : colored(v->rgba, textRgb, ring == 1 ? edge : 0));
            }
        }
        if (ok)
            return d;
    }
    return NULL;
}
/* A soft stroke's middle from (x0, y0) to (x1, y1), its sides a pixel off. */
static const Draw *stroke(float x0, float y0, float x1, float y1, float alpha) {
    for (int i = 0; i < drawCount; ++i) {
        const Draw *d = &draws[i];
        if (d->kind == TRI && d->vertexCount == 6 && d->indexCount == 12 && near(d->verts[1].xy[0], px(x0)) &&
            near(d->verts[1].xy[1], py(y0)) && near(d->verts[4].xy[0], px(x1)) && near(d->verts[4].xy[1], py(y1)) &&
            colored(d->verts[1].rgba, textRgb, alpha) && colored(d->verts[0].rgba, textRgb, 0) &&
            colored(d->verts[5].rgba, textRgb, 0) &&
            near(hypotf(d->verts[0].xy[0] - d->verts[1].xy[0], d->verts[0].xy[1] - d->verts[1].xy[1]), 1.0f))
            return d;
    }
    return NULL;
}
/* The viewer's arrow before a label whose pen starts at x. */
static int marker(float x, float alpha) {
    float tip = x - 4.0f, middle = 138.3f - 0.5f * 13.3f;
    int n = 0;
    for (int i = 0; i < drawCount; ++i) {
        const Draw *d = &draws[i];
        if (d->kind != TRI || d->vertexCount != 3)
            continue;
        assert(near(d->verts[0].xy[0], px(tip - 4.6f)) && near(d->verts[0].xy[1], py(middle - 2.9f)));
        assert(near(d->verts[1].xy[0], px(tip - 4.6f)) && near(d->verts[1].xy[1], py(middle + 2.9f)));
        assert(near(d->verts[2].xy[0], px(tip)) && near(d->verts[2].xy[1], py(middle)));
        assert(colored(d->verts[0].rgba, textRgb, alpha) && colored(d->verts[2].rgba, textRgb, alpha));
        ++n;
    }
    return n;
}
static const Draw *medal(float x, float s0) {
    float size = (float)imageHeight;
    for (int i = 0; i < drawCount; ++i) {
        const Draw *d = &draws[i];
        if (d->kind == PHYS && d->material == &medals && d->x == floorf(px(x) - 0.5f * size + 0.5f) &&
            d->y == floorf(py(220.0f) - 0.5f * size + 0.5f) && d->w == size && d->h == size && d->s0 == s0 &&
            d->s1 == s0 + 0.5f && d->t0 == 0.0f && d->t1 == 1.0f)
            return d;
    }
    return NULL;
}
static const Draw *dim(float alpha) {
    for (int i = 0; i < drawCount; ++i) {
        const Draw *d = &draws[i];
        if (d->kind == PHYS && d->x == 0 && d->y == 0 && near(d->w, 1024) && near(d->h, 576) &&
            d->material == &white && colored(d->color, black, alpha))
            return d;
    }
    return NULL;
}
static void player(int client, const char *name, int team) {
    clientInfo_t *ci = &cg->bgs.clientinfo[client];
    ci->infoValid = 1;
    snprintf(ci->name, sizeof(ci->name), "%s", name);
    ci->team = team;
}
static void score(int client, int points, int team) {
    score_t *s = &cg->scores[cg->numScores++];
    s->client = client;
    s->score = points;
    s->team = team;
}
static void match(const char *gametype, const char *objective) {
    memset(cg, 0, sizeof(*cg));
    snprintf(cgs->gametype, sizeof(cgs->gametype), "%s", gametype);
    snprintf(cg->objectiveText, sizeof(cg->objectiveText), "%s", objective);
    cgs->media.whiteMaterial = &white;
    freeForAll = !strcmp(gametype, "dm");
}
/* The texels of each half carry the medal built there. */
static void halves(int left, int right) {
    for (int y = 0; y < imageHeight; ++y)
        for (int x = 0; x < imageWidth; ++x)
            assert(image[(y * imageWidth + x) * 4] == (x < imageHeight ? left : right) + 2);
}

/* Frames, from one that goes on building, until the medals' image is made. */
static int settle(int intermission, int time) {
    int before = builds, frames = 0;
    while (builds == before && frames < 200) {
        frame(intermission, time, 1.0f);
        ++frames;
    }
    return builds == before ? -1 : frames;
}

/* The calls since rasters was cleared: both medals band by band from the
 * top, each band 2048 texels' worth of rows but the last, into one image. */
static void bands(int left, int right, int size) {
    int share = 2048 / size, row = 0;
    assert(rasters == 2 * ((size + share - 1) / share));
    for (int i = 0; i < rasters; i += 2) {
        int rows = size - row < share ? size - row : share;
        assert(rasterKinds[i] == left && rasterKinds[i + 1] == right);
        for (int j = i; j < i + 2; ++j)
            assert(rasterSizes[j] == size && rasterStrides[j] == 2 * size * 4 && rasterFirst[j] == row &&
                   rasterRows[j] == rows);
        row += rows;
    }
    assert(row == size);
}

int main(void) {
    /* The capture's match: the viewer's Allies lose on time, 9 to 15. */
    CG_EndgameInit();
    assert(registrations == 1 && !builds && g_webBackdropFrac == 0.0f);
    match("tdm", "MP_AXIS_WIN");
    cg->clientNum = 5;
    player(5, "Chapu54", TEAM_ALLIES);
    player(7, "Zebra", TEAM_AXIS);
    cgs->teamScores[0] = 15;
    cgs->teamScores[1] = 9;

    /* While the match goes on it draws nothing and leaves the scoreboard be,
     * but its medals are built over its first frames, a band of rows a frame:
     * the Allies' star and the iron cross, at the size drawn, once. */
    assert(!frame(0, 1000, 1.0f) && !drawCount && !builds && rasters == 2);
    assert(settle(0, 1000) == 13 && builds == 1 && imageHeight == 158 && imageWidth == 316);
    bands(AMERICAN, GERMAN, 158);
    halves(AMERICAN, GERMAN);
    assert(!frame(0, 1050, 1.0f) && builds == 1 && rasters == 28);
    CG_EndgameMessage("MP_TIME_LIMIT_REACHED");
    CG_EndgameMessage("MP_KILLCAM");
    CG_EndgameMessage("");

    /* The intermission starts: the announcement fades in over 400 ms, its
     * rule growing from the middle. */
    assert(frame(1, 20000, 1.0f) && !drawCount && g_webBackdropFrac == 0.0f);
    assert(frame(1, 20200, 1.0f) && dim(0.35f) && !count(TABLE));
    assert(centered(text("ALLIES DEFEAT"), 320, 63.3f, 26.7f, &boldFont, 0.5f));
    float grow = 1.0f - (1.0f - 200.0f / 600.0f) * (1.0f - 200.0f / 600.0f) * (1.0f - 200.0f / 600.0f);
    assert(line(320 - 143.3f * grow, 320 + 143.3f * grow, 72.9f, 0.1f));
    assert(line(320 - 133.3f * grow, 320 + 133.3f * grow, 75.4f, 0.075f));

    /* Then it stands as the capture has it. */
    assert(frame(1, 21000, 1.0f) && dim(0.7f) && draws[0].kind == PHYS && dim(0.7f) == &draws[0]);
    assert(centered(text("ALLIES DEFEAT"), 320, 63.3f, 26.7f, &boldFont, 1));
    assert(line(176.7f, 463.3f, 72.9f, 0.2f) && line(186.7f, 453.3f, 75.4f, 0.15f));
    for (int c = 0; c < 2; ++c) {
        float top = 74.6f + 2.5f * (float)c;
        assert(stroke(317.5f, top, 320, top + 1.7f, 0.35f) && stroke(320, top + 1.7f, 322.5f, top, 0.35f));
    }
    assert(centered(text("Team Deathmatch at Sainte Marie du Mont"), 320, 90.8f, 6.25f, &boldFont, 1));
    const Draw *allies = text("ALLIES"), *axis = text("AXIS");
    assert(centered(allies, 160, 138.3f, 13.3f, &boldFont, 1) && centered(axis, 480, 138.3f, 13.3f, &boldFont, 1));
    assert(marker(penOf(allies), 1) == 1);
    assert(medal(160, 0.0f) && medal(480, 0.5f) && colored(medal(160, 0.0f)->color, whiteRgb, 1));
    assert(centered(text("VS"), 320, 231.7f, 25, &boldFont, 1) && diamond(320, 218.2f, 29, 0.25f, 0.06f));
    static const float rows[4][3] = { { -15.85f, 27.5f, 62.1f }, { -12.0f, 31.25f, 70.4f },
                                      { 12.0f, 31.25f, 70.4f }, { 15.85f, 27.5f, 62.1f } };
    for (int i = 0; i < 4; ++i) {
        assert(line(320 - rows[i][2], 320 - rows[i][1], 218.2f + rows[i][0], 0.3f));
        assert(line(320 + rows[i][1], 320 + rows[i][2], 218.2f + rows[i][0], 0.3f));
    }
    assert(centered(text("9"), 160, 351.7f, 40, &boldFont, 1) && centered(text("15"), 480, 351.7f, 40, &boldFont, 1));
    assert(diamond(160, 335.4f, 39, 0.35f, 0.12f) && diamond(480, 335.4f, 39, 0.35f, 0.12f));
    assert(centered(text("TIME LIMIT REACHED"), 320, 432.5f, 8.3f, &regularFont, 1));
    assert(indexOf(text("ALLIES DEFEAT")) < indexOf(text("Team Deathmatch at Sainte Marie du Mont")) &&
           indexOf(text("Team Deathmatch at Sainte Marie du Mont")) < indexOf(allies) &&
           indexOf(axis) < indexOf(text("VS")) && indexOf(text("VS")) < indexOf(text("TIME LIMIT REACHED")));
    assert(count(TEXT) == 8 && count(TRI) == 4 + 1 + 3 && !count(TABLE) && g_webBackdropFrac == 0.0f);

    /* At 5 s it hands over to the scoreboard with the result over a blurred
     * view; half a second later only the scoreboard remains. */
    assert(frame(1, 25000, 1.0f) && !count(TABLE) && text("ALLIES DEFEAT"));
    assert(frame(1, 25250, 1.0f) && draws[0].kind == TABLE && near(draws[0].color[3], 0.5f));
    assert(!strcmp(tableResult, "ALLIES DEFEAT") && !strcmp(tableReason, "Time limit reached"));
    assert(dim(0.35f) && centered(text("ALLIES DEFEAT"), 320, 63.3f, 26.7f, &boldFont, 0.5f) &&
           near(g_webBackdropFrac, 0.5f));
    assert(frame(1, 25500, 1.0f) && drawCount == 1 && draws[0].kind == TABLE && near(draws[0].color[3], 1));
    assert(near(g_webBackdropFrac, 1.0f));
    assert(frame(1, 29000, 0.5f) && drawCount == 1 && near(draws[0].color[3], 0.5f) && near(g_webBackdropFrac, 1));
    /* The scoreboard's own fade takes the announcement down too. */
    assert(frame(1, 22000, 0.5f) && dim(0.35f) && centered(text("VS"), 320, 231.7f, 25, &boldFont, 0.5f));
    /* The owner's music started with it, once. Out of the intermission it is
     * gone, the music stopped at once and the view sharp again. */
    assert(musicPlays == 1 && musicStops == 0);
    assert(frame(1, 26000, 1.0f) && near(g_webBackdropFrac, 1));
    assert(!frame(0, 26050, 1.0f) && !drawCount && g_webBackdropFrac == 0.0f);
    assert(musicPlays == 1 && musicStops == 1 && lastFade == 0);

    /* A new intermission starts its own timeline and music, which fades out
     * over the 3 s before the next map loads, 16 s in, with what is left. */
    assert(frame(1, 40000, 1.0f) && !drawCount && musicPlays == 2);
    assert(frame(1, 41000, 1.0f) && text("ALLIES DEFEAT") && !count(TABLE));
    assert(frame(1, 52999, 1.0f) && musicStops == 1);
    assert(frame(1, 54000, 1.0f) && musicStops == 2 && lastFade == 2000);
    assert(frame(1, 55000, 1.0f) && musicStops == 2 && musicPlays == 2);

    /* The Axis viewer has his side on the left and wins; the scores follow. */
    cg->clientNum = 7;
    assert(frame(1, 41000, 1.0f));
    allies = text("ALLIES");
    axis = text("AXIS");
    assert(centered(text("AXIS VICTORY"), 320, 63.3f, 26.7f, &boldFont, 1) && centered(axis, 160, 138.3f, 13.3f,
                                                                                      &boldFont, 1));
    assert(centered(allies, 480, 138.3f, 13.3f, &boldFont, 1) && marker(penOf(axis), 1) == 1);
    assert(centered(text("15"), 160, 351.7f, 40, &boldFont, 1) && centered(text("9"), 480, 351.7f, 40, &boldFont, 1));
    assert(medal(160, 0.5f) && medal(480, 0.0f));
    /* A spectator sees the winner's victory, the Allies on the left, no arrow. */
    player(2, "Watcher", TEAM_SPECTATOR);
    cg->clientNum = 2;
    assert(frame(1, 41000, 1.0f) && text("AXIS VICTORY") && marker(0, 1) == 0);
    assert(centered(text("ALLIES"), 160, 138.3f, 13.3f, &boldFont, 1) && centered(text("9"), 160, 351.7f, 40,
                                                                                  &boldFont, 1));
    /* So does a viewer whose info is not in. */
    cg->clientNum = 9;
    assert(frame(1, 41000, 1.0f) && text("AXIS VICTORY") && marker(0, 1) == 0);
    cg->clientNum = 5;

    /* The stock scripts' other verdicts, marked as unlocalized or not; with
     * none the team scores decide. */
    snprintf(cg->objectiveText, sizeof(cg->objectiveText), "\x15MP_ALLIES_WIN");
    assert(frame(1, 41000, 1.0f) && text("ALLIES VICTORY"));
    snprintf(cg->objectiveText, sizeof(cg->objectiveText), "MP_THE_GAME_IS_A_TIE");
    assert(frame(1, 41000, 1.0f) && text("DRAW") && !text("ALLIES VICTORY") && !text("ALLIES DEFEAT"));
    cg->objectiveText[0] = 0;
    assert(frame(1, 41000, 1.0f) && text("ALLIES DEFEAT"));
    cgs->teamScores[1] = 20;
    assert(frame(1, 41000, 1.0f) && text("ALLIES VICTORY") && text("20"));
    cgs->teamScores[1] = 15;
    assert(frame(1, 41000, 1.0f) && text("DRAW"));
    cgs->teamScores[1] = 9;
    snprintf(cg->objectiveText, sizeof(cg->objectiveText), "MP_AXIS_WIN");

    /* Why it ended: the stock limits, without their periods; a later limit
     * message wins; a new map forgets it. */
    CG_EndgameMessage("MP_SCORE_LIMIT_REACHED");
    assert(frame(1, 41000, 1.0f) && text("SCORE LIMIT REACHED"));
    assert(frame(1, 46000, 1.0f) && !strcmp(tableReason, "Score limit reached"));
    CG_EndgameMessage("MP_ROUND_LIMIT_REACHED");
    assert(frame(1, 41000, 1.0f) && text("ROUND LIMIT REACHED"));
    regularReady = 0;
    CG_EndgameInit();
    regularReady = 1;
    assert(registrations == 2);
    assert(settle(0, 50000) == 14 && builds == 2);
    assert(frame(1, 50000, 1.0f) && frame(1, 51000, 1.0f) && text("ALLIES DEFEAT") && count(TEXT) == 7);
    assert(frame(1, 56000, 1.0f) && !tableReason[0] && !strcmp(tableResult, "ALLIES DEFEAT"));
    /* Without Regular the reason takes the bold font. */
    CG_EndgameMessage("MP_TIME_LIMIT_REACHED");
    assert(frame(1, 51000, 1.0f) && centered(text("TIME LIMIT REACHED"), 320, 432.5f, 8.3f, &boldFont, 1));
    /* A new map in the middle of a build starts it again. */
    CG_EndgameInit();
    assert(!frame(0, 59000, 1.0f) && builds == 2);
    CG_EndgameInit();
    CG_EndgameMessage("MP_TIME_LIMIT_REACHED");
    rasters = 0;
    assert(settle(0, 60000) == 14 && builds == 3);
    bands(AMERICAN, GERMAN, 158);
    assert(frame(1, 60000, 1.0f));

    /* A mod's long mode on a long map shrinks into its room on its baseline. */
    snprintf(cgs->gametype, sizeof(cgs->gametype), "a_very_long_mod_gametype_name12");
    mapName = "A Very Long Name For A Map Somewhere Far Away In The West Again";
    assert(frame(1, 61000, 1.0f));
    const Draw *subtitle =
        text("a_very_long_mod_gametype_name12 at A Very Long Name For A Map Somewhere Far Away In The West Again");
    assert(subtitle && near(widthOf(subtitle), 420) && near(baselineOf(subtitle), 90.8f) && capOf(subtitle) < 6.25f &&
           near(penOf(subtitle) + 210, 320));
    snprintf(cgs->gametype, sizeof(cgs->gametype), "tdm");
    mapName = "Sainte Marie du Mont";

    /* Medals are built again for a new size or nation, within bounds, and the
     * old ones are not drawn meanwhile; an Allies nation without an emblem
     * leaves only the cross; a failed build leaves the names and scores and
     * is not retried every frame; a change in the middle starts over. */
    rasters = 0;
    alliesKind = BRITISH;
    assert(frame(1, 61000, 1.0f) && !medal(160, 0.0f) && !medal(480, 0.5f) && text("ALLIES") && text("15"));
    assert(settle(1, 61000) == 13 && builds == 4 && medal(160, 0.0f) && medal(480, 0.5f));
    bands(BRITISH, GERMAN, 158);
    halves(BRITISH, GERMAN);
    unitScale = 1.5f;
    rasters = 0;
    assert(settle(1, 61000) == 20 && builds == 5 && imageHeight == 198 && medal(160, 0.0f));
    bands(BRITISH, GERMAN, 198);
    unitScale = 0.1f;
    assert(settle(1, 61000) == 1 && builds == 6 && imageHeight == 16);
    unitScale = 10.0f;
    rasters = 0;
    assert(settle(1, 61000) == 77 && builds == 7 && imageHeight == 384);
    bands(BRITISH, GERMAN, 384);
    unitScale = 1.2f;
    alliesKind = -1;
    assert(settle(1, 61000) == 14 && builds == 8 && !medal(160, 0.0f) && medal(480, 0.5f));
    halves(-1, GERMAN);
    alliesKind = AMERICAN;
    failBuild = 1;
    assert(settle(1, 61000) == 14 && builds == 9 && !medal(480, 0.5f));
    rasters = 0;
    assert(frame(1, 61100, 1.0f) && builds == 9 && !rasters && !medal(480, 0.5f));
    assert(text("ALLIES") && text("15") && diamond(480, 335.4f, 39, 0.35f, 0.12f));
    failBuild = 0;
    unitScale = 1.25f;
    assert(settle(1, 61000) == 14 && builds == 10 && medal(160, 0.0f) && medal(480, 0.5f));
    unitScale = 1.2f;
    assert(settle(1, 61000) == 14 && builds == 11);
    unitScale = 1.5f;
    assert(frame(1, 61000, 1.0f) && builds == 11 && !medal(160, 0.0f));
    unitScale = 1.2f;
    rasters = 0;
    assert(settle(1, 61000) == 14 && builds == 12 && imageHeight == 158);
    bands(AMERICAN, GERMAN, 158);

    /* Deathmatch: the best three, first in the middle, each named where the
     * emblems go; the viewer's place has the arrow. Spectators and players
     * not in yet take no place. */
    match("dm", "");
    cg->clientNum = 3;
    player(3, "Me", TEAM_FREE);
    player(4, "Other", TEAM_FREE);
    player(6, "^1Third", TEAM_FREE);
    player(8, "Fourth", TEAM_FREE);
    player(2, "Spec", TEAM_SPECTATOR);
    score(4, 500, TEAM_FREE);
    score(2, 0, TEAM_SPECTATOR);
    score(11, 450, TEAM_FREE);
    score(3, 400, TEAM_FREE);
    score(6, 100, TEAM_FREE);
    score(8, 50, TEAM_FREE);
    assert(!frame(0, 69000, 1.0f) && frame(1, 70000, 1.0f) && frame(1, 71000, 1.0f));
    assert(centered(text("DEFEAT"), 320, 63.3f, 26.7f, &boldFont, 1));
    assert(centered(text("Deathmatch at Sainte Marie du Mont"), 320, 90.8f, 6.25f, &boldFont, 1));
    const Draw *first = text("1ST"), *second = text("2ND"), *third = text("3RD");
    assert(centered(first, 320, 138.3f, 13.3f, &boldFont, 1) && centered(second, 160, 138.3f, 13.3f, &boldFont, 1) &&
           centered(third, 480, 138.3f, 13.3f, &boldFont, 1));
    assert(marker(penOf(second), 1) == 1);
    assert(centered(text("Other^7"), 320, 220 + 6.65f, 13.3f, &boldFont, 1) &&
           centered(text("Me^7"), 160, 220 + 6.65f, 13.3f, &boldFont, 1) &&
           centered(text("^1Third^7"), 480, 220 + 6.65f, 13.3f, &boldFont, 1));
    /* Our points are wider than the diamonds: they shrink to them. */
    static const char *const points[3] = { "500", "400", "100" };
    static const float columns[3] = { 320, 160, 480 };
    for (int i = 0; i < 3; ++i) {
        const Draw *d = text(points[i]);
        assert(d && near(widthOf(d), 78) && near(penOf(d) + 39, columns[i]) && near(baselineOf(d), 351.7f) &&
               capOf(d) < 40 && capOf(d) > 30 && d->style == 3);
    }
    assert(diamond(320, 335.4f, 39, 0.35f, 0.12f) && !text("VS") && !text("Fourth^7") && !text("Spec^7"));
    for (int i = 0; i < drawCount; ++i)
        assert(draws[i].material != &medals);
    assert(frame(1, 76000, 1.0f) && !strcmp(tableResult, "DEFEAT"));
    /* The stock script names the winner as the server ends a player's name;
     * that is the verdict, whatever the scores, or a tie. */
    snprintf(cg->objectiveText, sizeof(cg->objectiveText), "MP_WINS\x15Me^7");
    assert(frame(1, 71000, 1.0f) && text("VICTORY"));
    snprintf(cg->objectiveText, sizeof(cg->objectiveText), "MP_WINS\x15Me");
    assert(frame(1, 71000, 1.0f) && text("VICTORY"));
    snprintf(cg->objectiveText, sizeof(cg->objectiveText), "MP_WINS\x15Mexico^7");
    assert(frame(1, 71000, 1.0f) && text("DEFEAT"));
    snprintf(cg->objectiveText, sizeof(cg->objectiveText), "MP_THE_GAME_IS_A_TIE");
    assert(frame(1, 71000, 1.0f) && text("DRAW"));
    /* Without one, the best scores decide, equal ones a draw. */
    cg->objectiveText[0] = 0;
    cg->scores[0].score = 400;
    assert(frame(1, 71000, 1.0f) && text("DRAW"));
    cg->scores[0].client = 3;
    cg->scores[3].client = 4;
    cg->scores[0].score = 500;
    assert(frame(1, 71000, 1.0f) && text("VICTORY") && marker(penOf(text("1ST")), 1) == 1);
    /* A spectator is told who won. */
    cg->clientNum = 2;
    assert(frame(1, 71000, 1.0f) && text("Me^7 WINS") && marker(0, 1) == 0);
    snprintf(cg->objectiveText, sizeof(cg->objectiveText), "MP_WINS\x15Other^7");
    assert(frame(1, 71000, 1.0f) && text("Other^7 WINS"));
    snprintf(cg->objectiveText, sizeof(cg->objectiveText), "MP_THE_GAME_IS_A_TIE");
    assert(frame(1, 71000, 1.0f) && text("DRAW"));
    /* Fewer players fill fewer places; nobody at all is a draw. */
    cg->numScores = 1;
    cg->objectiveText[0] = 0;
    cg->clientNum = 3;
    assert(frame(1, 71000, 1.0f) && text("VICTORY") && text("1ST") && !text("2ND") && !text("3RD"));
    cg->numScores = 0;
    assert(frame(1, 71000, 1.0f) && text("DRAW") && !text("1ST"));
    /* A name too long for its column shrinks. */
    player(3, "AnExtremelyLongPlayerName_31", TEAM_FREE);
    cg->numScores = 1;
    assert(frame(1, 71000, 1.0f));
    const Draw *longName = text("AnExtremelyLongPlayerName_31^7");
    assert(longName && near(widthOf(longName), 150) && capOf(longName) < 13.3f);

    /* Off, or for a viewer out of range, the stock scoreboard keeps its place
     * and the view stays sharp; the medals are not built. */
    endgameDvar.current.enabled = 0;
    unitScale = 1.3f;
    rasters = 0;
    assert(!frame(1, 76000, 1.0f) && !drawCount && g_webBackdropFrac == 0.0f && builds == 12 && !rasters);
    endgameDvar.current.enabled = 1;
    unitScale = 1.2f;
    cg->clientNum = 64;
    assert(!frame(1, 76000, 1.0f) && !drawCount && g_webBackdropFrac == 0.0f);

    /* A menu painted over the last frame, as the class screens of a match
     * are, has the next one blurred and gray behind it, and only that one. */
    g_webMenuBackdrop = 1.0f;
    assert(!frame(0, 77000, 1.0f) && near(g_webBackdropFrac, 1) && near(g_webBackdropGray, 1) && !g_webMenuBackdrop);
    assert(!frame(0, 77050, 1.0f) && g_webBackdropFrac == 0.0f && g_webBackdropGray == 0.0f);

    /* A first frame past the next map's time stops the music at once; without
     * the owner's music the end of a match is silent. */
    assert(!frame(0, 80000, 1.0f));
    int plays = musicPlays, stops = musicStops;
    frame(1, 80016, 1.0f);
    frame(1, 97016, 1.0f);
    assert(musicPlays == plays + 1 && musicStops == stops + 1 && lastFade == 0);
    haveMusic = 0;
    frame(0, 98000, 1.0f);
    plays = musicPlays;
    stops = musicStops;
    frame(1, 98016, 1.0f);
    frame(1, 112016, 1.0f);
    frame(0, 112100, 1.0f);
    assert(musicPlays == plays && musicStops == stops);
    puts("PASS");
    return 0;
}
'''


def mutate(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


mutants = [
    ('    if (intermission && !s_endgame.intermission)\n        s_endgame.start = cg->time;',
     '    if (intermission)\n        s_endgame.start = cg->time;'),
    ('    g_webBackdropFrac = g_webMenuBackdrop;\n', ''),
    ('    g_webBackdropGray = g_webMenuBackdrop;\n', ''),
    ('    g_webMenuBackdrop = 0.0f;\n    CG_EndgamePrepare();', '    CG_EndgamePrepare();'),
    ('        g_webBackdropFrac = summary;\n', ''),
    ('#define ENDGAME_ANNOUNCE_MSEC 5000', '#define ENDGAME_ANNOUNCE_MSEC 4000'),
    ('#define ENDGAME_INTERMISSION_MSEC 16000', '#define ENDGAME_INTERMISSION_MSEC 15000'),
    ('#define ENDGAME_MUSIC_FADE_MSEC 3000', '#define ENDGAME_MUSIC_FADE_MSEC 1500'),
    ('            SND_StopMusic(left > 0 ? left : 0);', '            SND_StopMusic(ENDGAME_MUSIC_FADE_MSEC);'),
    ('            SND_StopMusic(left > 0 ? left : 0);', '            SND_StopMusic(left);'),
    ('            SND_PlayMusicAlias(Com_PickSoundAliasFromList(music), SASYS_CGAME);\n', '            ;\n'),
    ('        s_endgame.musicPlaying = 0;\n        SND_StopMusic(0);\n', '        s_endgame.musicPlaying = 0;\n'),
    ('        s_endgame.musicPlaying = music != NULL;', '        s_endgame.musicPlaying = 1;'),
    ('    CG_EndgameMusic(intermission);\n', ''),
    ('#define ENDGAME_FADE_MSEC 400', '#define ENDGAME_FADE_MSEC 250'),
    ('summary = elapsed <= ENDGAME_ANNOUNCE_MSEC ? 0.0f : 1.0f - announce;', 'summary = 1.0f - announce;'),
    ('    grow = 1.0f - (1.0f - grow) * (1.0f - grow) * (1.0f - grow);\n', ''),
    ('#define ENDGAME_TITLE_BASELINE 63.3f', '#define ENDGAME_TITLE_BASELINE 64.0f'),
    ('#define ENDGAME_VS_Y 218.2f', '#define ENDGAME_VS_Y 220.0f'),
    ('#define ENDGAME_SCORE_CAP 40.0f', '#define ENDGAME_SCORE_CAP 36.0f'),
    ('ENDGAME_SCORE_BASELINE, ENDGAME_SCORE_ROOM, color);', 'ENDGAME_SCORE_BASELINE, ENDGAME_SIDE_ROOM, color);'),
    ('#define ENDGAME_DIM 0.7f', '#define ENDGAME_DIM 0.5f'),
    ('CG_EndgameColor(color, s_endgameBlack, ENDGAME_DIM * alpha);', 'CG_EndgameColor(color, s_endgameBlack, ENDGAME_DIM);'),
    ('return team == TEAM_AXIS ? "AXIS" : "ALLIES";', 'return team == TEAM_AXIS ? "ALLIES" : "AXIS";'),
    ('view->marked = own == TEAM_SPECTATOR ? -1 : 0;', 'view->marked = 0;'),
    ('view->teams[0] = own == TEAM_AXIS ? TEAM_AXIS : TEAM_ALLIES;', 'view->teams[0] = TEAM_ALLIES;'),
    ('view->scores[i] = cgs->teamScores[view->teams[i] == TEAM_AXIS ? 0 : 1];',
     'view->scores[i] = cgs->teamScores[view->teams[i] == TEAM_AXIS ? 1 : 0];'),
    ('        return cgs->teamScores[1] > cgs->teamScores[0] ? TEAM_ALLIES : TEAM_AXIS;',
     '        return cgs->teamScores[1] > cgs->teamScores[0] ? TEAM_AXIS : TEAM_ALLIES;'),
    ('return (unsigned char)text[0] == ENDGAME_LITERAL ? text + 1 : text;', 'return text;'),
    ('winner == own ? "VICTORY" : "DEFEAT"', 'winner != own ? "VICTORY" : "DEFEAT"'),
    ('Com_sprintf(view->title, sizeof(view->title), "%s VICTORY", CG_EndgameTeamName(winner));',
     'Com_sprintf(view->title, sizeof(view->title), "%s DEFEAT", CG_EndgameTeamName(winner));'),
    ("    if (length && buf[length - 1] == '.')\n        buf[length - 1] = 0;\n", ''),
    ("(char)(view->reason[i] - 'a' + 'A')", 'view->reason[i]'),
    ('            s_endgame.reason = i;', '            s_endgame.reason = ENDGAME_REASON_TIME;'),
    ('outcome = CG_EndgameIsWinner(text + 8, local->name) ? ENDGAME_VICTORY : ENDGAME_DEFEAT;',
     'outcome = ENDGAME_VICTORY;'),
    ("winner[length - 2] == '^' && winner[length - 1] == '7' &&", '0 &&'),
    ('    return !strcmp(winner, name);', '    return 0;'),
    ('static const float s_endgameColumns[ENDGAME_PODIUM] = { 0.0f, -1.0f, 1.0f };',
     'static const float s_endgameColumns[ENDGAME_PODIUM] = { -1.0f, 0.0f, 1.0f };'),
    ('            score->team == TEAM_SPECTATOR)', '            0)'),
    ('(view->podium > 1 && view->podiumScores[0] == view->podiumScores[1])', '0'),
    ('        if (score->client == localClient)\n            view->marked = view->podium;\n', ''),
    ('Com_sprintf(view->title, sizeof(view->title), "%s^7 WINS", cg->bgs.clientinfo[view->clients[0]].name);',
     'I_strncpyz(view->title, "VICTORY", sizeof(view->title));'),
    ('    x -= 0.5f * width;\n', ''),
    ('        scale *= room / width;\n', ''),
    ('baseline + (rise + 0.5f) * unit', 'baseline + rise * unit'),
    ('        float r = radius + (float)(ring - 1) * 1.41421356f;', '        float r = radius + (float)(ring - 1);'),
    ('        CG_EndgameMarker(pen - ENDGAME_MARKER_GAP,', '        CG_EndgameMarker(pen,'),
    ('axis ? 0.5f : 0.0f, 0.0f, axis ? 1.0f : 0.5f, 1.0f', 'axis ? 0.0f : 0.5f, 0.0f, axis ? 0.5f : 1.0f, 1.0f'),
    ('    if (!s_endgame.medals || s_endgame.medalKinds[axis] < 0)', '    if (!s_endgame.medals)'),
    ('    if (size != s_endgame.medalSize || kinds[0] != s_endgame.medalKinds[0] || kinds[1] != s_endgame.medalKinds[1]) {',
     '    if (size != s_endgame.medalSize) {'),
    ('CG_ScoreTableRasterizeMedal(kinds[i], s_endgame.medalImage + i * size * 4, size * 8, size,',
     'CG_ScoreTableRasterizeMedal(kinds[i], s_endgame.medalImage + i * size * 4, size * 4, size,'),
    ('    rows = ENDGAME_MEDAL_TEXELS_PER_FRAME / size;\n', '    rows = size;\n'),
    ('    if (rows > size - s_endgame.medalRows)\n        rows = size - s_endgame.medalRows;\n', ''),
    ('    s_endgame.medalRows += rows;\n', ''),
    ('        s_endgame.medals = NULL;\n        s_endgame.medalRows = 0;\n', '        s_endgame.medals = NULL;\n'),
    ('        s_endgame.medals = NULL;\n        s_endgame.medalRows = 0;\n', '        s_endgame.medalRows = 0;\n'),
    ('    if (s_endgame.medalRows < size)\n        return;\n', ''),
    ('    free(s_endgame.medalImage);\n    s_endgame.medalImage = NULL;\n}', '}'),
    ('    if (size < ENDGAME_EMBLEM_MIN_TEXELS)\n        size = ENDGAME_EMBLEM_MIN_TEXELS;\n    else ', '    '),
    ('    if (!CG_EndgameEnabled())\n        return;\n    size =', '    size ='),
    ('    if (!s_endgame.intermission || !CG_EndgameEnabled() || (unsigned)localClient >= ENDGAME_MAX_CLIENTS)',
     '    if (!s_endgame.intermission || !CG_EndgameEnabled() || localClient > ENDGAME_MAX_CLIENTS)'),
    ('    s_endgame.regularFont = R_RegisterWwiiFont("$regularfont");\n', ''),
    ('    memset(&s_endgame, 0, sizeof(s_endgame));\n', ''),
]
variants = [body] + [mutate(body, old, new) for old, new in mutants]
with tempfile.TemporaryDirectory(prefix='cod2-endgame-') as directory:
    path = Path(directory) / 'endgame.c'
    binary = Path(directory) / 'endgame'
    for index, source in enumerate(variants):
        path.write_text(support + source + '\n' + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wall', '-Werror' if index == 0 else '-w', '-Wno-unused-function', str(path), '-o',
                        str(binary), '-lm'], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
print('PASS: at intermission the WWII end of a match fades in over the darkened view and announces the result '
      'where the capture has it: the viewer\'s team and its victory or defeat (the winner\'s for spectators, a draw '
      'on a tie) over a growing rule and the mode at the map, the viewer\'s side on the left with its arrow, both '
      'emblems as medals built a band of rows a frame when the map starts, VS between its lines, the scores in diamonds and the limit that '
      'ended it; deathmatch shows the best three, first in the middle, and the server\'s named winner; after 5 s '
      'the scoreboard takes over with the result and reason in its header over a blurred view, until the five '
      'modes load the next map 16 s in; the owner\'s music plays from the start, fades out over the last 3 s and '
      f'stops with the intermission, silent without it; {len(mutants)} failing regression mutants')
