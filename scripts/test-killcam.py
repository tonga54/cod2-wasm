#!/usr/bin/env python3
"""Run the production WWII killcam on recorded draws: what it hides, the duel's
score, the attackers, the weapon and the way it killed, the key and the bars."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
module = (root / 'src/PC/cgame_mp/cg_killcam_mp.c').read_text()
header = (root / 'src/headers/PC/cgame_mp/cg_killcam.h').read_text()
table = (root / 'src/PC/cgame_mp/cg_scoretable_mp.c').read_text()
table_header = (root / 'src/headers/PC/cgame_mp/cg_scoretable.h').read_text()
draw = (root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text()
main = (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()
hudelem = (root / 'src/PC/cgame_mp/cg_hudelem_mp.c').read_text()
event = (root / 'src/PC/cgame_mp/cg_event_mp.c').read_text()
commands = (root / 'src/PC/cgame_mp/cg_servercmds_mp.c').read_text()
hud2d = (root / 'src/PC/gfx_d3d/r_hud2d.c').read_text()
hud2d_header = (root / 'src/headers/PC/gfx_d3d/r_hud2d.h').read_text()


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


# The cgame starts it with the HUD and feeds it every obituary and our
# server's attacker lists; it hides the stock killcam's elements.
assert main.count('#include "headers/PC/cgame_mp/cg_killcam.h"\n') == 1
assert main.count('    CG_ScoreTableInit();\n    CG_KillcamInit();\n') == 1
for source in (draw, hudelem, event, commands):
    assert source.count('#include "headers/PC/cgame_mp/cg_killcam.h"\n') == 1
assert function(hudelem, 'void CG_Draw2dHudElems(qboolean foreground)\n{').count('        if (CG_KillcamHidesHudElem(elem))\n            continue;\n') == 1
assert event.count('            CG_KillcamObituary(attackerName[0] ? attacker : -1, target, ep);\n') == 1
assert commands.count('            CG_KillcamSetAttackers(atoi(CG_Argv(1)), others, count);\n') == 1
# It is drawn over the replay last, after the killer's crosshair, names and
# script elements; the minimap, menus and the HUD widgets give way to it,
# and so do the centre prints and notify lines after the scoreboard.
draw2d = function(draw, 'unsigned int CG_Draw2D(void)\n{')
ordered(draw2d,
        '    hudCovered = CG_ScoreTableHidesHud();\n',
        '    killcam = drawHudElems && CG_KillcamActive();\n',
        '    CG_MinimapSetHudVisible(drawHudMenus && pm_type != 4 && pm_type != 5 && !killcam && !prematch);\n',
        '            CG_DrawCrosshair();\n',
        '                CG_DrawEnemyNames();\n',
        '            CG_DrawChatMessages();\n        }\n        CG_CheckTimedMenus();\n',
        '            CG_Draw2dHudElems(0);\n        if (killcam)\n            drawHudMenus = 0;\n'
        '        if (drawHudMenus && !prematch)\n            Menu_PaintAll((void *)imp_cgDC);\n',
        '            CG_DrawScorePopup();\n        }\n        if (killcam)\n            CG_DrawKillcam();\n    }\n',
        '    drewScoreboard = CG_DrawScoreboard();\n    if (!drawHudMenus)\n        return 0;\n',
        '    if (cg->centerPrintTime != 0) {')
slots = {name: int(value) for name, value in re.findall(r'^#define (R_HUD_IMAGE_\w+) (\d+)$', hud2d_header, re.M)}
names = re.findall(r'"([^"]+)"', re.search(r's_hudImageNames\[R_HUD_IMAGE_COUNT\] = \{(.*?)\};', hud2d, re.S).group(1))
assert len(names) == slots['R_HUD_IMAGE_COUNT'] and len(set(names)) == len(names)
assert names[slots['R_HUD_IMAGE_KILLCAM']] == '*hud_killcam'

body = module
for include in ('#include "common_types.h"\n', '#include "imports.h"\n', '#include "headers/PC/cgame_mp/cg_local.h"\n',
                '#include "headers/PC/cgame_mp/cg_headshoticon.h"\n', '#include "headers/PC/cgame_mp/cg_playernames.h"\n',
                '#include "headers/PC/cgame_mp/cg_scoretable.h"\n', '#include "headers/PC/gfx_d3d/r_font_wwii.h"\n',
                '#include "headers/PC/gfx_d3d/r_hud2d.h"\n'):
    assert body.count(include) == 1, include
    body = body.replace(include, '')
assert body.count('#include "headers/PC/cgame_mp/cg_killcam.h"\n') == 1
body = body.replace('#include "headers/PC/cgame_mp/cg_killcam.h"\n', header)
lettering = table_header + ''.join(function(table, head) + '\n' for head in (
    'float CG_ScoreTableCapScale(FontHandle font, float cap)\n{',
    'float CG_ScoreTableTextWidth(const char *text, int maxChars, FontHandle font, float scale)\n{',
    'int CG_ScoreTableFit(const char *text, FontHandle font, float scale, float room)\n{'))

support = r'''
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
typedef unsigned char byte;
typedef float vec_t;
typedef float vec4_t[4];
struct Font_s { const char *fontName; int pixelHeight; };
typedef struct Font_s *FontHandle;
struct Material { const char *name; };
typedef struct Material *MaterialHandle;
typedef struct { struct { int enabled; } current; } dvar_t;
typedef enum {
    HE_TYPE_FREE = 0, HE_TYPE_TEXT = 1, HE_TYPE_MATERIAL = 6, HE_TYPE_TIMER_DOWN = 7, HE_TYPE_TENTHS_TIMER_DOWN = 9
} he_type_t;
typedef struct { he_type_t type; float x, y; int alignScreen; int width, height; int text; } hudelem_t;
typedef struct { int clientNum; int weapon; struct { hudelem_t current[31]; hudelem_t archival[31]; } hud; } playerState_t;
typedef struct { playerState_t ps; } snapshot_t;
typedef struct { int infoValid; char name[32]; int team; } clientInfo_t;
typedef struct { char *szDisplayName; char *killIcon; int wideKillIcon; int flipKillIcon; } WeaponDef;
typedef struct HudVertex2D { float xy[2]; float st[2]; float rgba[4]; } HudVertex2D;
#define R_HUD_IMAGE_KILLCAM ''' + str(slots['R_HUD_IMAGE_KILLCAM']) + r'''
static snapshot_t snap;
static struct {
    int clientNum, time, inKillCam;
    snapshot_t *nextSnap;
    struct { clientInfo_t clientinfo[64]; } bgs;
} cgState;
#define cg (&cgState)
static struct Material white = { "white" }, discsMaterial = { "*hud_killcam" }, icons[8];
static int iconCount;
static struct { struct { MaterialHandle whiteMaterial; } media; } cgsState = { { &white } };
#define cgs (&cgsState)

/* A 16:9 screen whose 4:3 area is its full height. */
static float screenW = 1280.0f, screenH = 720.0f;
void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign) {
    float s = screenH / 480.0f;
    switch (horzAlign) {
    case 1: *x *= s; *w *= s; break;
    case 2: *x = 0.5f * screenW + *x * s; *w *= s; break;
    case 3: *x = screenW + *x * s; *w *= s; break;
    case 4: *x *= screenW / 640.0f; *w *= screenW / 640.0f; break;
    default: abort();
    }
    assert(vertAlign == 0 || vertAlign == 4);
    *y *= s; *h *= s;
}

/* Fonts: the bold and regular WWII fonts, a UI font and the name font, their
 * capitals and baselines in font units, every letter half an em wide. */
static struct Font_s boldFont = { "fonts/boldFont", 30 }, regularFont = { "fonts/regularFont", 32 },
                     labelFont = { "fonts/normalFont", 16 }, nameFont = { "name", 64 };
static int regularReady = 1, nameReady = 1;
static float advance = 0.5f;
static float lastLabelScale;
static void metrics(FontHandle font, float *cap, float *baseline) {
    if (font == &boldFont) { *cap = 17.1f; *baseline = 6.0f; }
    else if (font == &regularFont) { *cap = 21.0f; *baseline = 5.0f; }
    else if (font == &labelFont) { *cap = 10.5f; *baseline = 3.0f; }
    else if (font == &nameFont) { *cap = 44.8f; *baseline = 0.0f; }
    else abort();
}
void CG_ScoreTableFontMetrics(FontHandle font, float *cap, float *baseline) { metrics(font, cap, baseline); }
float CL_NormalizedTextScale(FontHandle font, float scale) { return 48.0f * scale / (float)font->pixelHeight; }
int CL_TextWidth(const char *text, int maxChars, FontHandle font) {
    int count = 0;
    for (const char *c = text; *c && (!maxChars || count < maxChars); ++c) {
        if (c[0] == '^' && c[1] && c[1] != '^') { ++c; continue; }
        ++count;
    }
    return (int)(count * advance * (float)font->pixelHeight);
}
FontHandle UI_GetFontHandle(int fontEnum, float scale) { assert(fontEnum == 0); lastLabelScale = scale; return &labelFont; }
FontHandle CL_RegisterFont(const char *fontName, int imageTrack) {
    assert(!strcmp(fontName, "$boldfont") && imageTrack == 0);
    return &boldFont;
}
FontHandle R_RegisterWwiiFont(const char *fontName) {
    assert(!strcmp(fontName, "$regularfont"));
    return regularReady ? &regularFont : NULL;
}
struct Font_s *CG_GetPlayerNameFont(void) { return nameReady ? &nameFont : NULL; }

enum { TEXT, PIC, TRIS };
typedef struct {
    int kind;
    char text[64];
    FontHandle font;
    float x, y, w, h, scale, s0, t0, s1, t1;
    int horz, vert, style;
    vec4_t color;
    MaterialHandle material;
    HudVertex2D verts[8];
    int vertexCount, indexCount;
    unsigned short indices[12];
} Drawn;
static Drawn drawn[64];
static int drawCount;
static Drawn *record(int kind) { assert(drawCount < 64); memset(&drawn[drawCount], 0, sizeof(Drawn)); drawn[drawCount].kind = kind; return &drawn[drawCount++]; }
void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign,
                 float scale, const vec_t *color, int style) {
    Drawn *d = record(TEXT);
    assert(maxChars == 0x7fffffff && strlen(text) < sizeof(d->text));
    strcpy(d->text, text); d->font = font; d->x = x; d->y = y; d->horz = horzAlign; d->vert = vertAlign;
    d->scale = scale; d->style = style; memcpy(d->color, color, sizeof(vec4_t));
}
void CL_DrawStretchPicPhysical(float x, float y, float w, float h, float s1, float t1, float s2, float t2,
                               const vec_t *color, MaterialHandle material) {
    Drawn *d = record(PIC);
    d->x = x; d->y = y; d->w = w; d->h = h; d->s0 = s1; d->t0 = t1; d->s1 = s2; d->t1 = t2;
    memcpy(d->color, color, sizeof(vec4_t)); d->material = material;
}
void R_AddCmdDrawTriangles2D(const HudVertex2D *verts, int vertexCount, const unsigned short *indices, int indexCount,
                             MaterialHandle material) {
    Drawn *d = record(TRIS);
    assert(vertexCount <= 8 && indexCount <= 12);
    memcpy(d->verts, verts, vertexCount * sizeof(HudVertex2D)); memcpy(d->indices, indices, indexCount * 2);
    d->vertexCount = vertexCount; d->indexCount = indexCount; d->material = material;
}
MaterialHandle CL_RegisterMaterial(const char *name, int imageTrack) {
    assert(imageTrack == 7);
    for (int i = 0; i < iconCount; ++i)
        if (!strcmp(icons[i].name, name)) return &icons[i];
    assert(iconCount < 8);
    icons[iconCount].name = name;
    return &icons[iconCount++];
}
static byte image[512 * 256 * 4];
static int imageWidth, imageHeight, creates, failCreate;
MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height) {
    assert(slot == R_HUD_IMAGE_KILLCAM && width <= 512 && height <= 256);
    memcpy(image, bgra, (size_t)width * height * 4); imageWidth = width; imageHeight = height; creates++;
    return failCreate ? NULL : &discsMaterial;
}
static int symbolSize, symbolRing;
void CG_HeadshotIconRaster(byte *bgra, int size, int texels) {
    symbolSize = size; symbolRing = texels;
    for (int i = 0; i < size * size; ++i) { bgra[4 * i] = bgra[4 * i + 1] = bgra[4 * i + 2] = 255; bgra[4 * i + 3] = 77; }
}
const char *CL_GetConfigString(int index) {
    switch (index - 0x51e) {
    case 1: return "MP_KILLCAM";
    case 2: return "PLATFORM_PRESS_TO_RESPAWN";
    case 3: return "platform_press_to_skip";
    case 4: return "MP_OTHER";
    default: return "";
    }
}
static const char *boundKey = "F";
static int configs;
void Controls_GetConfig(void) { configs++; }
int GetKeyBindings(const char *command, char (*bindings)[128]) {
    assert(!strcmp(command, "+activate"));
    if (!boundKey) return 0;
    snprintf(bindings[0], 128, "%s", boundKey); bindings[1][0] = 0;
    return 1;
}
const char *SEH_StringEd_GetString(const char *name) { return !strcmp(name, "KEY_MOUSE2") ? "Right Mouse" : NULL; }
const char *UI_SafeTranslateString(const char *reference) {
    if (!strcmp(reference, "MP_KILLCAM")) return "KILLCAM";
    if (!strcmp(reference, "WEAPON_KAR98K")) return "Kar98k";
    if (!strcmp(reference, "WEAPON_COLT45")) return "Colt .45";
    if (!strcmp(reference, "WEAPON_M1GARAND")) return "M1 Garand";
    return reference;
}
static WeaponDef weapons[4] = {
    { 0 },
    { "WEAPON_KAR98K", "gfx/icons/hud@kar98.tga", 1, 1 },
    { "WEAPON_COLT45", "gfx/icons/hud@colt45.tga", 0, 1 },
    { "WEAPON_M1GARAND", "", 1, 1 },
};
WeaponDef *BG_GetWeaponDef(int weaponIndex) { assert(weaponIndex > 0 && weaponIndex < 4); return &weapons[weaponIndex]; }
int BG_GetNumWeapons(void) { return 3; }
static dvar_t killcamDvar;
const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags) {
    assert(!strcmp(name, "cg_hudKillcam") && value == 1 && flags == 0x1001);
    killcamDvar.current.enabled = value;
    return &killcamDvar;
}
int I_stricmp(const char *s1, const char *s2) { return strcasecmp(s1, s2); }
void I_strncpyz(char *dest, const char *src, int destsize) { snprintf(dest, destsize, "%s", src); }
int Com_sprintf(char *dest, int size, const char *fmt, ...) {
    va_list ap; int n;
    va_start(ap, fmt); n = vsnprintf(dest, size, fmt, ap); va_end(ap);
    return n;
}
'''

checks = r'''
static void near(float a, float b) {
    if (fabsf(a - b) > 0.01f) { fprintf(stderr, "%f != %f\n", a, b); assert(0); }
}
static float ppu(void) { return screenH / 480.0f; }
static float scaleFor(FontHandle font, float cap) {
    float fontCap, rise;
    metrics(font, &fontCap, &rise);
    return cap * (float)font->pixelHeight / (48.0f * fontCap);
}
static float widthOf(const char *text, FontHandle font, float cap) {
    return (float)CL_TextWidth(text, 0, font) * 48.0f * scaleFor(font, cap) / (float)font->pixelHeight;
}
static const Drawn *text(const char *string) {
    const Drawn *found = NULL;
    for (int i = 0; i < drawCount; ++i)
        if (drawn[i].kind == TEXT && !strcmp(drawn[i].text, string)) { assert(!found); found = &drawn[i]; }
    return found;
}
static int count(int kind) {
    int n = 0;
    for (int i = 0; i < drawCount; ++i) n += drawn[i].kind == kind;
    return n;
}
static const Drawn *nth(int kind, MaterialHandle material, int index) {
    for (int i = 0; i < drawCount; ++i)
        if (drawn[i].kind == kind && drawn[i].material == material && index-- == 0) return &drawn[i];
    return NULL;
}
static int same(const vec_t *a, const vec_t *b) { return !memcmp(a, b, sizeof(vec4_t)); }
/* A text with its pen at x and capitals cap units tall on baseline: the
 * scoreboard's half-unit offset puts it where the capture has it. */
static void placed(const Drawn *d, FontHandle font, float cap, float x, float baseline, int horz, int style) {
    float fontCap, rise, scale = scaleFor(font, cap), unit = 48.0f * scale / (float)font->pixelHeight;
    metrics(font, &fontCap, &rise);
    assert(d && d->font == font && d->horz == horz && d->vert == 0 && d->style == style);
    near(d->scale, scale);
    near(d->x, x + 0.5f * unit);
    near(d->y, baseline + (rise + 0.5f) * unit);
}
static void centred(const Drawn *d, FontHandle font, float cap, float x, float baseline, int horz) {
    placed(d, font, cap, x - 0.5f * widthOf(d->text, font, cap), baseline, horz, 0);
}
static hudelem_t elem(he_type_t type, float x, float y, int alignScreen, int width, int height, int text) {
    hudelem_t e = { type, x, y, alignScreen, width, height, text };
    return e;
}
static void setup(void) {
    memset(&cgState, 0, sizeof(cgState));
    memset(&snap, 0, sizeof(snap));
    cgState.nextSnap = &snap;
    cgState.clientNum = 2;
    cgState.time = 100000;
    for (int i = 0; i < 64; ++i) {
        cgState.bgs.clientinfo[i].infoValid = 1;
        snprintf(cgState.bgs.clientinfo[i].name, 32, "Player%d", i);
    }
    screenW = 1280.0f; screenH = 720.0f; advance = 0.5f;
    failCreate = creates = configs = 0; boundKey = "F";
    CG_KillcamInit();
    cgState.inKillCam = 1;
    snap.ps.clientNum = 5;
    snap.ps.weapon = 1;
}
static void frame(void) { drawCount = 0; CG_DrawKillcam(); }
static const vec4_t cream = { 0.875f, 0.863f, 0.82f, 1.0f }, grey = { 0.54f, 0.54f, 0.52f, 1.0f },
                    red = { 0.62f, 0.0f, 0.05f, 1.0f };
/* The cells of the discs' image at 1.5 pixels a unit: the key's 24 texels,
 * the dot's 11, the headshot symbol's 38. */
enum { KEY = 24, DOT = 11, SYMBOL = 38, WIDTH = KEY + DOT + SYMBOL };

int main(void) {
    int cases = 0;

    /* Only while another player's replay plays, with the setting on. */
    setup();
    assert(CG_KillcamActive());
    killcamDvar.current.enabled = 0;
    frame();
    assert(!CG_KillcamActive() && drawCount == 0);
    killcamDvar.current.enabled = 1;
    cgState.inKillCam = 0;
    assert(!CG_KillcamActive());
    cgState.inKillCam = 1;
    snap.ps.clientNum = 2;
    assert(!CG_KillcamActive());
    snap.ps.clientNum = 64;
    assert(!CG_KillcamActive());
    snap.ps.clientNum = -1;
    assert(!CG_KillcamActive());
    cases++;

    /* It hides the stock bars, title, prompt and timer, as _killcam.gsc makes
     * them, and nothing else. */
    setup();
    hudelem_t e = elem(HE_TYPE_MATERIAL, 0, 0, 36, 640, 112, 0);
    assert(CG_KillcamHidesHudElem(&e));
    e.y = 368; assert(CG_KillcamHidesHudElem(&e));
    e.y = 367; assert(!CG_KillcamHidesHudElem(&e));
    e.y = 0; e.alignScreen = 0; assert(!CG_KillcamHidesHudElem(&e));
    e.alignScreen = 36; e.width = 320; assert(!CG_KillcamHidesHudElem(&e));
    e.width = 640; e.height = 100; assert(!CG_KillcamHidesHudElem(&e));
    e = elem(HE_TYPE_TEXT, 0, 30, 57, 0, 0, 1); assert(CG_KillcamHidesHudElem(&e));
    e.text = 2; assert(CG_KillcamHidesHudElem(&e));
    e.text = 3; assert(CG_KillcamHidesHudElem(&e));
    e.text = 4; assert(!CG_KillcamHidesHudElem(&e));
    e.text = 0; assert(!CG_KillcamHidesHudElem(&e));
    e.text = 1; e.x = 5; assert(!CG_KillcamHidesHudElem(&e));
    e.x = 0; e.alignScreen = 9; assert(!CG_KillcamHidesHudElem(&e));
    e = elem(HE_TYPE_TENTHS_TIMER_DOWN, 0, -32, 59, 0, 0, 0); assert(CG_KillcamHidesHudElem(&e));
    e.y = -31; assert(!CG_KillcamHidesHudElem(&e));
    e.y = -32; e.type = HE_TYPE_TIMER_DOWN; assert(!CG_KillcamHidesHudElem(&e));
    e = elem(HE_TYPE_MATERIAL, 0, 0, 36, 640, 112, 0);
    cgState.inKillCam = 0; assert(!CG_KillcamHidesHudElem(&e));
    cases++;

    /* Black bars across the screen down to 83.3 units and up from 394.2, a
     * little lighter where they meet the view; the title in the bold font. */
    setup();
    frame();
    const Drawn *bars = nth(TRIS, &white, 0);
    assert(bars && bars->vertexCount == 8 && bars->indexCount == 12);
    const float rows[8] = { 0, 0, 83.3f * 1.5f, 83.3f * 1.5f, 394.2f * 1.5f, 394.2f * 1.5f, 720, 720 };
    for (int i = 0; i < 8; ++i) {
        near(bars->verts[i].xy[0], (i & 3) == 1 || (i & 3) == 2 ? 1280.0f : 0.0f);
        near(bars->verts[i].xy[1], rows[i]);
        near(bars->verts[i].rgba[3], i >= 2 && i <= 5 ? 0.92f : 1.0f);
        assert(!bars->verts[i].rgba[0] && !bars->verts[i].rgba[1] && !bars->verts[i].rgba[2]);
    }
    for (int i = 0; i < 12; ++i)
        assert(bars->indices[i] / 4 == i / 6);
    placed(text("KILLCAM"), &boldFont, 14.43f, 53.65f, 42.52f, 1, 0);
    assert(same(text("KILLCAM")->color, cream));
    /* Without the stock prompt there is no key to show. */
    assert(!text("Respawn") && !text("Skip") && !text("F") && configs == 0);
    cases++;

    /* The key that respawns: a cream disc with the key in it, then the word. */
    setup();
    snap.ps.hud.current[0] = elem(HE_TYPE_TEXT, 0, 70, 57, 0, 0, 2);
    frame();
    const Drawn *key = text("F");
    placed(key, &boldFont, 6.5f, 53.5f + 0.5f * (14.0f - widthOf("F", &boldFont, 6.5f)), 57.38f + 3.25f, 1, 0);
    placed(text("Respawn"), &regularFont, 6.85f, 53.5f + 14.0f + 2.44f, 59.92f, 1, 0);
    assert(configs == 1);
    const Drawn *left = nth(PIC, &discsMaterial, 0), *right = nth(PIC, &discsMaterial, 1);
    near(left->x, 79); near(left->w, 12); near(right->x, 91); near(right->w, 12);
    near(left->y, 74); near(left->h, KEY); near(right->y, 74);
    near(left->s0, 0); near(left->s1, 12.0f / WIDTH); near(right->s0, 12.0f / WIDTH); near(right->s1, (float)KEY / WIDTH);
    near(left->t0, 0); near(left->t1, (float)KEY / SYMBOL);
    /* No middle for one letter; the attackers' single dot comes next. */
    assert(nth(PIC, &discsMaterial, 2)->w == DOT);
    snap.ps.hud.current[0].text = 3;
    frame();
    assert(text("Skip") && !text("Respawn"));
    cases += 2;

    /* A longer key name grows the disc into a pill, its middle column drawn
     * across, and pushes the word right; with nothing bound, USE. */
    boundKey = "KEY_MOUSE2";
    frame();
    float pill = widthOf("RIGHT MOUSE", &boldFont, 6.5f) + 7.0f;
    assert(text("RIGHT MOUSE") && pill > 14.0f);
    placed(text("Skip"), &regularFont, 6.85f, 53.5f + pill + 2.44f, 59.92f, 1, 0);
    const Drawn *middle = nth(PIC, &discsMaterial, 1);
    near(middle->s0, 12.0f / WIDTH); near(middle->s1, 12.0f / WIDTH);
    near(middle->x, 91); near(middle->w, floorf(1.5f * (53.5f + pill - 7.0f) + 0.5f) - 91);
    boundKey = NULL;
    frame();
    assert(text("USE"));
    cases += 2;

    /* The duel counts kills between the killer and the viewer from the
     * obituaries, nobody else's, and starts over for a new player in the slot. */
    setup();
    CG_KillcamObituary(5, 2, 1);
    CG_KillcamObituary(2, 5, 1);
    CG_KillcamObituary(5, 2, 1);
    CG_KillcamObituary(3, 4, 1);
    CG_KillcamObituary(5, 3, 1);
    CG_KillcamObituary(-1, 2, 0x80 | 11);
    CG_KillcamObituary(2, 2, 0x80 | 12);
    CG_KillcamObituary(5, 64, 1);
    CG_KillcamObituary(70, 2, 1);
    frame();
    centred(text("ENEMY"), &labelFont, 6.82f, -108.0f, 37.38f, 3);
    centred(text("YOU"), &labelFont, 6.82f, -55.4f, 37.38f, 3);
    centred(text("VS"), &labelFont, 6.74f, -81.74f, 56.53f, 3);
    centred(text("2"), &regularFont, 18.85f, -108.0f, 64.73f, 3);
    centred(text("1"), &regularFont, 18.85f, -55.4f, 64.73f, 3);
    assert(same(text("ENEMY")->color, grey) && same(text("2")->color, cream));
    strcpy(cgState.bgs.clientinfo[5].name, "Newcomer");
    frame();
    assert(!text("2") && !text("1"));
    CG_KillcamObituary(5, 2, 1);
    frame();
    centred(text("1"), &regularFont, 18.85f, -108.0f, 64.73f, 3);
    centred(text("0"), &regularFont, 18.85f, -55.4f, 64.73f, 3);
    cases += 2;

    /* ATTACKERS lists the server's others by first hit, once each, known
     * players only, then the killer, each with a red dot. */
    setup();
    cgState.bgs.clientinfo[9].infoValid = 0;
    int others[] = { 3, 7, 5, 3, 9, 8, 70, -1 };
    CG_KillcamSetAttackers(5, others, 8);
    frame();
    placed(text("ATTACKERS"), &boldFont, 7.0f, -231.57f, 104.9f, 3, 3);
    const char *expected[] = { "Player3", "Player7", "Player8", "Player5" };
    for (int i = 0; i < 4; ++i) {
        float dotY = 116.6f + 15.2f * i;
        placed(text(expected[i]), &nameFont, 6.1f, -231.57f, dotY + 1.0f, 3, 3);
        const Drawn *dot = nth(PIC, &discsMaterial, i);
        near(dot->x, floorf(1280.0f - 37.5f * 1.5f - 0.5f * DOT + 0.5f));
        near(dot->y, floorf(dotY * 1.5f - 0.5f * DOT + 0.5f));
        near(dot->w, DOT); near(dot->h, DOT);
        near(dot->s0, (float)KEY / WIDTH); near(dot->s1, (float)(KEY + DOT) / WIDTH); near(dot->t1, (float)DOT / SYMBOL);
        assert(same(dot->color, red));
    }
    assert(!nth(PIC, &discsMaterial, 4) && !text("Player9"));
    cases++;

    /* A list for another killer, or an old one, leaves the killer alone; one
     * from up to 20 s either way of the replay's clock is used. */
    CG_KillcamSetAttackers(6, others, 8);
    frame();
    assert(text("Player5") && !text("Player3"));
    CG_KillcamSetAttackers(5, others, 8);
    cgState.time += 20000;
    frame();
    assert(!text("Player3"));
    cgState.time -= 20000 + 15000;
    frame();
    assert(text("Player3"));
    cases += 2;

    /* Five others at most; names without color codes, cut to their room, in
     * the UI font when the name font cannot be made. */
    setup();
    int many[] = { 10, 11, 12, 13, 14, 15, 16 };
    CG_KillcamSetAttackers(5, many, 7);
    strcpy(cgState.bgs.clientinfo[10].name, "^1Ki^7ng");
    frame();
    assert(text("King") && text("Player14") && !text("Player15") && text("Player5"));
    placed(text("Player5"), &nameFont, 6.1f, -231.57f, 116.6f + 5 * 15.2f + 1.0f, 3, 3);
    advance = 2.0f;
    strcpy(cgState.bgs.clientinfo[11].name, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    frame();
    assert(text("ABCDEFGHIJ") && widthOf("ABCDEFGHIJ", &nameFont, 6.1f) <= 180.0f &&
           widthOf("ABCDEFGHIJK", &nameFont, 6.1f) > 180.0f);
    advance = 0.5f;
    nameReady = 0;
    frame();
    placed(text("King"), &labelFont, 6.1f, -231.57f, 117.6f, 3, 3);
    nameReady = 1;
    cases += 3;

    /* The weapon of the obituary, its name in capitals over its icon, turned
     * up to the right and mirrored like the kill feed's when it is wide. */
    setup();
    CG_KillcamObituary(5, 2, 1);
    frame();
    centred(text("KAR98K"), &labelFont, 6.7f, 0.0f, 414.9f, 2);
    assert(same(text("KAR98K")->color, grey) && !text("Headshot") && !text("Melee"));
    const Drawn *rifle = nth(TRIS, CL_RegisterMaterial("gfx/icons/hud@kar98.tga", 7), 0);
    assert(rifle && rifle->vertexCount == 4 && rifle->indexCount == 6);
    /* 60 by 30 units, turned 43.7 degrees. */
    const float corners[4][2] = { { -45, -22.5f }, { 45, -22.5f }, { 45, 22.5f }, { -45, 22.5f } };
    const float c = cosf(43.7f * 3.14159265f / 180.0f), sn = sinf(43.7f * 3.14159265f / 180.0f);
    for (int i = 0; i < 4; ++i) {
        near(rifle->verts[i].xy[0], 640.0f + corners[i][0] * c + corners[i][1] * sn);
        near(rifle->verts[i].xy[1], 667.5f - corners[i][0] * sn + corners[i][1] * c);
        near(rifle->verts[i].st[0], corners[i][0] < 0 ? 1.0f : 0.0f);
        near(rifle->verts[i].st[1], corners[i][1] < 0 ? 0.0f : 1.0f);
        near(rifle->verts[i].rgba[0], 1); near(rifle->verts[i].rgba[3], 1);
    }
    /* The right end, the muzzle once mirrored, is the top one. */
    assert(rifle->verts[1].xy[1] < rifle->verts[3].xy[1] && rifle->verts[1].xy[0] > rifle->verts[3].xy[0]);
    cases++;

    /* A square icon stands upright; a weapon without one shows its name. */
    CG_KillcamObituary(5, 2, 2);
    frame();
    centred(text("COLT .45"), &labelFont, 6.7f, 0.0f, 414.9f, 2);
    const Drawn *pistol = nth(TRIS, CL_RegisterMaterial("gfx/icons/hud@colt45.tga", 7), 0);
    for (int i = 0; i < 4; ++i) {
        near(pistol->verts[i].xy[0], 640.0f + (corners[i][0] < 0 ? -21.0f : 21.0f));
        near(pistol->verts[i].xy[1], 667.5f + (corners[i][1] < 0 ? -21.0f : 21.0f));
        near(pistol->verts[i].st[0], corners[i][0] < 0 ? 1.0f : 0.0f);
    }
    CG_KillcamObituary(5, 2, 3);
    frame();
    assert(text("M1 GARAND") && count(TRIS) == 1);
    cases += 2;

    /* A headshot, whose obituary names no weapon, shows our symbol beside the
     * weapon in the killer's hands; a melee kill the stock melee icon. */
    setup();
    CG_KillcamObituary(5, 2, 0x80 | 8);
    frame();
    centred(text("Headshot"), &labelFont, 6.7f, -42.0f, 414.9f, 2);
    centred(text("KAR98K"), &labelFont, 6.7f, 42.0f, 414.9f, 2);
    const Drawn *symbol = nth(PIC, &discsMaterial, 1);
    assert(symbolSize == SYMBOL && symbolRing == 18);
    near(symbol->x, floorf(640.0f - 63.0f - 0.5f * SYMBOL + 0.5f)); near(symbol->y, floorf(667.5f - 0.5f * SYMBOL + 0.5f));
    near(symbol->w, SYMBOL); near(symbol->s0, (float)(KEY + DOT) / WIDTH); near(symbol->s1, 1);
    assert(same(symbol->color, cream));
    near(nth(TRIS, CL_RegisterMaterial("gfx/icons/hud@kar98.tga", 7), 0)->verts[0].xy[0],
         640.0f + 63.0f - 45.0f * c - 22.5f * sn);
    CG_KillcamObituary(5, 2, 0x80 | 7);
    frame();
    centred(text("Melee"), &labelFont, 6.7f, -42.0f, 414.9f, 2);
    const Drawn *melee = nth(TRIS, CL_RegisterMaterial("killiconmelee", 7), 0);
    near(melee->verts[0].xy[0], 577.0f - 21.0f); near(melee->verts[2].xy[1], 667.5f + 21.0f);
    near(melee->verts[0].st[0], 0);
    /* Without a weapon in hand, the way it killed alone. */
    snap.ps.weapon = 0;
    frame();
    assert(text("Melee") && !text("KAR98K"));
    /* Another killer's obituary says nothing of this replay. */
    snap.ps.weapon = 1;
    CG_KillcamObituary(6, 2, 0x80 | 8);
    frame();
    assert(!text("Headshot") && !text("Melee"));
    centred(text("KAR98K"), &labelFont, 6.7f, 0.0f, 414.9f, 2);
    cases += 4;

    /* The discs' image is built once for the screen, texel for pixel, and
     * again when the screen changes; the key's cell is even, so its halves
     * meet in the middle. Without the image the text still shows. */
    setup();
    frame();
    frame();
    assert(creates == 1 && imageWidth == WIDTH && imageHeight == SYMBOL);
    assert(image[(12 * WIDTH + 12) * 4 + 3] == 255 && image[3] == 0 && image[(12 * WIDTH + 1) * 4 + 3] < 128);
    assert(image[(5 * WIDTH + KEY + 5) * 4 + 3] == 255 && image[(10 * WIDTH + KEY + 10) * 4 + 3] == 0);
    assert(image[(37 * WIDTH + KEY + DOT) * 4 + 3] == 77 && image[(37 * WIDTH + WIDTH - 1) * 4 + 3] == 77);
    assert(image[(37 * WIDTH + KEY + DOT - 1) * 4 + 3] == 0 && image[(37 * WIDTH + KEY - 1) * 4 + 3] == 0);
    screenW = 1920.0f; screenH = 1080.0f;
    frame();
    assert(creates == 2 && imageWidth == 34 + 15 + 56 && imageHeight == 56);
    failCreate = 1;
    screenW = 2560.0f; screenH = 1440.0f;
    frame();
    assert(creates == 3 && text("KILLCAM") && text("Player5") && count(PIC) == 0);
    frame();
    assert(creates == 3);
    cases += 3;

    /* Without the regular font, numbers and the prompt take the UI font. */
    setup();
    regularReady = 0;
    CG_KillcamInit();
    snap.ps.hud.current[0] = elem(HE_TYPE_TEXT, 0, 70, 57, 0, 0, 2);
    CG_KillcamObituary(5, 2, 1);
    frame();
    placed(text("Respawn"), &labelFont, 6.85f, 53.5f + 14.0f + 2.44f, 59.92f, 1, 0);
    centred(text("1"), &labelFont, 18.85f, -108.0f, 64.73f, 3);
    regularReady = 1;
    cases++;

    printf("%d\n", cases);
    return 0;
}
'''

mutants = [
    ('return killer >= 0 && killer < KILLCAM_MAX_CLIENTS && killer != cg->clientNum;',
     'return killer >= 0 && killer < KILLCAM_MAX_CLIENTS;'),
    ('|| !cg->inKillCam ||', '||'),
    ('(elem->y == 0.0f || elem->y == 368.0f)', '(elem->y == 0.0f || elem->y == 112.0f)'),
    ('if (!I_stricmp(text, "MP_KILLCAM"))', 'if (!strcmp(text, "KILLCAM"))'),
    ('if (!I_stricmp(text, "PLATFORM_PRESS_TO_SKIP"))', 'if (!strcmp(text, "PLATFORM_PRESS_TO_SKIP"))'),
    ('elem->x == 0.0f && elem->y == -32.0f', 'elem->x == 0.0f'),
    ('#define KILLCAM_TOP_BAR 83.3f', '#define KILLCAM_TOP_BAR 112.0f'),
    ('#define KILLCAM_BAR_INNER_ALPHA 0.92f', '#define KILLCAM_BAR_INNER_ALPHA 1.0f'),
    ('UI_DrawText(text, 0x7fffffff, font, x + 0.5f * unit,', 'UI_DrawText(text, 0x7fffffff, font, x,'),
    ('    keyWidth = textWidth + 2.0f * KILLCAM_KEY_PAD;\n', '    keyWidth = KILLCAM_KEY_SIZE;\n'),
    ('prompt == KILLCAM_STOCK_RESPAWN ? "Respawn" : "Skip"', 'prompt == KILLCAM_STOCK_SKIP ? "Respawn" : "Skip"'),
    ('CG_KillcamUpper(key, name ? name : bindings[0], KILLCAM_KEY_LEN);',
     'CG_KillcamUpper(key, bindings[0], KILLCAM_KEY_LEN);'),
    ('    view->rows[view->rowCount++] = killer;\n', ''),
    ('view->rowCount < KILLCAM_MAX_ROWS - 1', 'view->rowCount < KILLCAM_MAX_ROWS'),
    ('    if (state->attackersKiller == killer && CG_KillcamRecent(state->attackersTime, now)) {',
     '    if (CG_KillcamRecent(state->attackersTime, now)) {'),
    ('            if (j == view->rowCount)\n                view->rows', '            if (1)\n                view->rows'),
    ('|| other == killer ||', '||'),
    ('return now - time > -KILLCAM_MEMORY_MSEC && now - time < KILLCAM_MEMORY_MSEC;',
     'return now - time >= 0 && now - time < KILLCAM_MEMORY_MSEC;'),
    ('        view->killedViewer = state->killedViewer[killer];\n        view->killedByViewer = state->killedByViewer[killer];',
     '        view->killedViewer = state->killedByViewer[killer];\n        view->killedByViewer = state->killedViewer[killer];'),
    ('    if (!strcmp(state->names[clientNum], name))\n        return;\n', ''),
    ('        name[CG_ScoreTableFit(name, font, scale, KILLCAM_NAME_ROOM)] = \'\\0\';\n', ''),
    ("        if (src[0] == '^' && src[1] && src[1] != '^') {", "        if (0) {"),
    ('        if (!(state->deathParm & 0x80))\n            view->weapon = state->deathParm;\n        else if',
     '        if'),
    ('if (state->deathKiller == killer && CG_KillcamRecent(state->deathTime, now))',
     'if (CG_KillcamRecent(state->deathTime, now))'),
    ('#define KILLCAM_MOD_HEAD_SHOT 8', '#define KILLCAM_MOD_HEAD_SHOT 9'),
    ('    float s = tilted ? -sinf(KILLCAM_ICON_TILT) : 0.0f;', '    float s = tilted ? sinf(KILLCAM_ICON_TILT) : 0.0f;'),
    ('#define KILLCAM_ICON_TILT 0.7627f', '#define KILLCAM_ICON_TILT 0.7854f'),
    ('#define KILLCAM_WIDE_ICON_WIDTH 60.0f', '#define KILLCAM_WIDE_ICON_WIDTH 56.0f'),
    ('(corners[i][0] > 0.0f) != (flip != 0)', '(corners[i][0] > 0.0f)'),
    ('    CG_KillcamDrawMethod(state, view->method, -0.5f * KILLCAM_SLOT_PITCH);',
     '    CG_KillcamDrawMethod(state, view->method, 0.0f);'),
    ("        if (dest[i] >= 'a' && dest[i] <= 'z')\n            dest[i] = (char)(dest[i] - 'a' + 'A');\n", ''),
    ('if (fabsf(pixelsPerUnit - s_killcam.discsPixelsPerUnit) > 0.001f)', 'if (!s_killcam.discsPixelsPerUnit)'),
    ('state->keyTexels = 2 * (int)ceilf(0.5f * KILLCAM_KEY_SIZE * pixelsPerUnit) + 2;',
     'state->keyTexels = (int)ceilf(KILLCAM_KEY_SIZE * pixelsPerUnit) + 2;'),
    ('memcpy(bgra + (y * state->discsWidth + state->keyTexels + state->dotTexels) * 4,',
     'memcpy(bgra + (y * state->discsWidth + state->keyTexels) * 4,'),
    ('            CG_KillcamDrawCell(state, state->keyTexels, state->dotTexels, x, y, s_killcamDot);',
     '            CG_KillcamDrawCell(state, state->keyTexels, state->dotTexels, x, y, s_killcamCream);'),
    ('    if (target == viewer) {\n        CG_KillcamClaim', '    if (target == viewer && 0) {\n        CG_KillcamClaim'),
    ('    } else if (attacker == viewer) {', '    } else if (0) {'),
    ('#define KILLCAM_ROW_PITCH 15.2f', '#define KILLCAM_ROW_PITCH 15.4f'),
    ('#define KILLCAM_DOT_X -37.5f', '#define KILLCAM_DOT_X -38.1f'),
    ('#define KILLCAM_SLOT_ICON_Y 445.0f', '#define KILLCAM_SLOT_ICON_Y 442.5f'),
]

variants = [body]
for old, new in mutants:
    assert body.count(old) == 1, old
    variants.append(body.replace(old, new))
with tempfile.TemporaryDirectory(prefix='cod2-killcam-') as directory:
    path = Path(directory) / 'killcam.c'
    binary = Path(directory) / 'killcam'
    cases = 0
    for index, variant in enumerate(variants):
        path.write_text(support + lettering + variant + checks)
        warnings = ['-Wall', '-Werror', '-Wno-unused-function'] if index == 0 else ['-w']
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        *warnings, str(path), '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, mutants[index - 1] if index else None,
                                                         result.stderr[-2000:])
        if index == 0:
            cases = int(result.stdout)
print('PASS: over another player\'s replay the WWII killcam hides the stock bars, title, prompt and timer and draws '
      'its own: bars to the capture\'s heights, KILLCAM, the +activate key on a disc or pill with Respawn or Skip as '
      'the stock prompt says, the duel counted from obituaries, ATTACKERS from our server\'s list by first hit with '
      'the killer last, and the killing weapon in capitals over its icon, with the headshot symbol or the melee icon '
      'beside it; its discs are built at the size drawn and the HUD gives way to it; '
      f'{cases} cases; {len(mutants)} failing regression mutants')
