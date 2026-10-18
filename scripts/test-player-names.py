#!/usr/bin/env python3
"""Check the WWII player names: the baked Barlow font against its source and
the WWII captures, then the production module, compiled with the engine's real
glyph lookup, text width and fade and the real friendly and enemy name loops:
the atlas it uploads, the font it builds, the cleaned names it draws in teal
above teammates and in red above the enemy the server flags under the
crosshair, the teal it lends the crosshair over a teammate, and what stays
when they are switched off or without their image."""
from pathlib import Path
import hashlib
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
module = (root / 'src/PC/cgame_mp/cg_playernames_mp.c').read_text()
data_path = root / 'src/headers/PC/cgame_mp/cg_namefont_data.h'
data = data_path.read_text()
draw = (root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text()
drawtools = (root / 'src/PC/cgame_mp/cg_drawtools_mp.c').read_text()
main = (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()
hud2d_header = (root / 'src/headers/PC/gfx_d3d/r_hud2d.h').read_text()
hud2d = (root / 'src/PC/gfx_d3d/r_hud2d.c').read_text()
fonts = (root / 'src/PC/gfx_d3d/r_font.c').read_text()
ui = (root / 'src/PC/ui_mp/ui_main_mp.c').read_text()

# Measured on the WWII captures, which stay out of the repository. The
# teammate's: capitals 16 px tall, "[FaZe] matZiin_GOD" 204 px from the first
# ink to the last, and the brightest eighth of the letters' pixels averaging
# this color. The enemy's "[FR] ML82": its brightest eighth averaging this red.
TEAMMATE_TEXT = b'[FaZe] matZiin_GOD'
TEAMMATE_WIDTH_PER_CAP = 204 / 16
TEAMMATE_COLOR = (0.386, 0.552, 0.495)
ENEMY_COLOR = (0.648, 0.322, 0.246)
TEAMMATE_BYTES = tuple(round(channel * 255) for channel in TEAMMATE_COLOR)
ENEMY_BYTES = tuple(round(channel * 255) for channel in ENEMY_COLOR)


def function(text, name):
    match = re.search(r'^((?:static )?[^\n;]*\b' + name + r'\([^;]*?\))\s*\{', text, re.M)
    assert match, name
    brace = text.index('{', match.end() - 1)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


# The friendly name loop hands every label to the module, the enemy loop
# draws after it, each level registers the module, and the renderer has a
# slot for the font's image.
assert draw.count('#include "headers/PC/cgame_mp/cg_playernames.h"\n') == 1
position = function(draw, 'CG_GetPlayerNamePosition')
friendly = function(draw, 'CG_DrawFriendlyNames')
enemy = function(draw, 'CG_DrawEnemyNames')
assert friendly.count('CG_GetPlayerNamePosition(') == 1 and enemy.count('CG_GetPlayerNamePosition(') == 1
assert friendly.count('CG_DrawTeammateName(') == 1
assert friendly.index('CG_DrawTeammateName(') < friendly.index('UI_DrawText(')
assert enemy.count('CG_DrawEnemyName(') == 1 and 'UI_DrawText(' not in enemy
assert draw.count('CG_DrawEnemyNames();') == 1
assert draw.count('            if (pm_type <= 1) {\n                CG_DrawFriendlyNames();\n'
                  '                CG_DrawEnemyNames();\n') == 1
assert main.count('#include "headers/PC/cgame_mp/cg_playernames.h"\n') == 1
assert main.count('    CG_WeaponPromptInit();\n    CG_PlayerNamesInit();\n') == 1
slot = int(re.search(r'^#define R_HUD_IMAGE_NAME_FONT (\d+)$', hud2d_header, re.M).group(1))
count = int(re.search(r'^#define R_HUD_IMAGE_COUNT (\d+)$', hud2d_header, re.M).group(1))
names = re.search(r's_hudImageNames\[R_HUD_IMAGE_COUNT\] = \{(.*?)\};', hud2d, re.S).group(1)
names = re.findall(r'"([^"]+)"', names)
assert slot < count and len(names) == count and names[slot] == '*hud_name_font'

# The data was baked from the committed font, whose license travels with it.
font_file = root / 'third_party/barlow/Barlow-Medium.ttf'
assert hashlib.sha256(font_file.read_bytes()).hexdigest() in data.split('\n', 2)[1]
license_text = (root / 'third_party/barlow/OFL.txt').read_text()
assert license_text.startswith('Copyright 2017 The Barlow Project Authors (https://github.com/jpt/barlow)\n')
assert 'SIL Open Font License, Version 1.1' in license_text

defines = {name: int(value) for name, value in re.findall(r'^#define (CG_NAME_FONT_\w+) (\d+)$', data, re.M)}
glyph_rows = [tuple(int(value, 0) for value in match) for match in re.findall(
    r'^    \{ (0x[0-9a-f]+), (-?\d+), (-?\d+), (\d+), (\d+), (\d+), (\d+), (\d+) \},$', data, re.M)]
atlas_rows = re.findall(r'^    "([0-9a-f]*)",$', data, re.M)
UNITS = defines['CG_NAME_FONT_UNITS_PER_TEXEL']
WIDTH = defines['CG_NAME_FONT_ATLAS_WIDTH']
HEIGHT = defines['CG_NAME_FONT_ATLAS_HEIGHT']
CP1252 = list(range(0x20, 0x80)) + [code for code in range(0x80, 0x100) if code not in (0x81, 0x8d, 0x8f, 0x90, 0x9d)]


def check_data(glyphs, rows):
    """Asserts what R_GetCharacterGlyph, the Glyph fields and bilinear
    filtering need from the table and the atlas."""
    assert len(glyphs) == defines['CG_NAME_FONT_GLYPH_COUNT'] and len(rows) == HEIGHT
    assert all(len(row) == WIDTH for row in rows)
    # Direct lookups for 0x20 to 0x7f, a binary search above them.
    assert [glyph[0] for glyph in glyphs] == CP1252
    covered = [[False] * WIDTH for _ in range(HEIGHT)]
    for letter, x0, y0, dx, width, height, ax, ay in glyphs:
        assert -128 <= x0 <= 127 and -128 <= y0 <= 127 and 0 < dx <= 255, hex(letter)
        assert width * UNITS <= 255 and height * UNITS <= 255
        if letter in (0x20, 0x7f, 0xa0):
            assert width == height == 0 and dx == glyphs[0][3]
            continue
        assert width > 2 and height > 2 and ax + width <= WIDTH and ay + height <= HEIGHT, hex(letter)
        inked = False
        for y in range(ay, ay + height):
            for x in range(ax, ax + width):
                assert not covered[y][x], ('overlap', hex(letter))
                covered[y][x] = True
                border = x in (ax, ax + width - 1) or y in (ay, ay + height - 1)
                assert not border or rows[y][x] == '0', ('ink on the pad', hex(letter))
                inked |= rows[y][x] != '0'
        assert inked, hex(letter)
    assert all(rows[y][x] == '0' for y in range(HEIGHT) for x in range(WIDTH) if not covered[y][x]), 'stray ink'


def with_glyph(index, glyph):
    return glyph_rows[:index] + [glyph] + glyph_rows[index + 1:]


def with_digit(x, y, value):
    return atlas_rows[:y] + [atlas_rows[y][:x] + value + atlas_rows[y][x + 1:]] + atlas_rows[y + 1:]


check_data(glyph_rows, atlas_rows)
by_letter = {glyph[0]: glyph for glyph in glyph_rows}
h, i = by_letter[0x48], by_letter[0x49]
free = next((x, y) for y in range(HEIGHT - 1, -1, -1) for x in range(WIDTH - 1, -1, -1)
            if not any(g[6] <= x < g[6] + g[4] and g[7] <= y < g[7] + g[5] for g in glyph_rows))
broken = [
    (glyph_rows[:100] + [glyph_rows[101], glyph_rows[100]] + glyph_rows[102:], atlas_rows),  # out of order
    (with_glyph(150, glyph_rows[149]), atlas_rows),  # a letter missing
    (with_glyph(0x48 - 0x20, h[:6] + i[6:]), atlas_rows),  # two glyphs in one place
    (glyph_rows, with_digit(h[6], h[7], 'f')),  # ink on a glyph's pad
    (glyph_rows, with_digit(free[0], free[1], '8')),  # ink outside every glyph
]
for bad_glyphs, bad_rows in broken:
    try:
        check_data(bad_glyphs, bad_rows)
    except AssertionError:
        continue
    raise AssertionError('check_data accepted a broken font')


def digit(letter, x, y):
    glyph = by_letter[letter]
    return int(atlas_rows[glyph[7] + y][glyph[6] + x], 16)


# A capital's height: the coverage of the H's fullest column, in texels.
h = by_letter[0x48]
cap_texels = max(sum(digit(0x48, x, y) for y in range(h[5])) for x in range(h[4])) / 15
assert abs(cap_texels / (defines['CG_NAME_FONT_PIXEL_HEIGHT'] / UNITS) - 0.7) < 0.02, cap_texels


def ink_columns(letter):
    glyph = by_letter[letter]
    columns = [x for x in range(glyph[4]) if any(digit(letter, x, y) for y in range(glyph[5]))]
    return glyph[1] / UNITS + columns[0], glyph[1] / UNITS + columns[-1] + 1


# As wide for its height as the teammate capture's name.
pen = sum(by_letter[letter][3] for letter in TEAMMATE_TEXT[:-1]) / UNITS
ink_width = pen + ink_columns(TEAMMATE_TEXT[-1])[1] - ink_columns(TEAMMATE_TEXT[0])[0]
assert abs(ink_width / cap_texels / TEAMMATE_WIDTH_PER_CAP - 1) < 0.03, ink_width / cap_texels

engine = (r'''
#define CL_TextWidth R_TextWidth
#define CL_NormalizedTextScale R_NormalizedTextScale
static const Glyph *''' + function(fonts, 'R_GetCharacterGlyph_inl') + function(fonts, 'R_GetCharacterGlyph')
          + function(fonts, 'R_TextWidth_inl') + function(fonts, 'R_TextWidth')
          + function(fonts, 'R_NormalizedTextScale') + function(ui, 'UI_TextWidth')
          + function(drawtools, 'CG_FadeColor'))
names_module = module[module.index('/* Capitals 7 units tall'):]

support = r'''
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte;
typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef const void *MaterialHandle;
typedef struct Glyph { unsigned short letter; signed char x0; signed char y0; byte dx; byte pixelWidth; byte pixelHeight;
                       float s0, t0, s1, t1; } Glyph;
typedef struct Font_s { const char *name; int pixelHeight; int glyphCount; MaterialHandle material; Glyph *glyphs; } Font;
typedef Font *FontHandle;
typedef struct { struct { int enabled; } current; } dvar_t;
#define R_HUD_IMAGE_MINIMAP 0
#define R_HUD_IMAGE_NAME_FONT ''' + str(slot) + r'''
#define CAP_TEXELS ''' + repr(cap_texels) + r'''f
#include "''' + str(data_path) + r'''"

static unsigned int ReadChar(const char **text, int flags) { (void)flags; return (unsigned char)*(*text)++; }
static struct { unsigned int (*SEH_ReadCharFromString)(const char **text, int flags); } ri = { ReadChar };

static dvar_t teamNames = { { 1 } }, enemyNames = { { 1 } };
static int registered;
static const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags) {
    assert(value == 1 && flags == 0x1001);
    registered++;
    if (!strcmp(name, "cg_hudTeammateNames"))
        return &teamNames;
    assert(!strcmp(name, "cg_hudEnemyNames"));
    return &enemyNames;
}
static const char fakeMaterial[] = "material";
static int creates, createFails, createdSlot, createdWidth, createdHeight;
static byte *createdPixels;
static MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height) {
    creates++;
    createdSlot = slot;
    createdWidth = width;
    createdHeight = height;
    free(createdPixels);
    createdPixels = (byte *)malloc((size_t)width * height * 4);
    memcpy(createdPixels, bgra, (size_t)width * height * 4);
    return createFails ? NULL : fakeMaterial;
}
/* The renderer draws the fonts it is told are Barlow like its WWII fonts. */
static FontHandle lettered;
static int letterings;
static void R_AddWwiiLettering(FontHandle font) { lettered = font; letterings++; }
typedef struct { char text[64]; int maxChars; FontHandle font; float x, y; int horzAlign, vertAlign; float scale;
                 vec4_t color; int style; } Draw;
static Draw drawn[16];
static int draws;
static void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign,
                        float scale, const vec_t *color, int style) {
    Draw *d = &drawn[draws++];
    assert(draws <= 16 && strlen(text) < sizeof(d->text));
    strcpy(d->text, text);
    d->maxChars = maxChars;
    d->font = font;
    d->x = x;
    d->y = y;
    d->horzAlign = horzAlign;
    d->vertAlign = vertAlign;
    d->scale = scale;
    memcpy(d->color, color, sizeof(vec4_t));
    d->style = style;
}

/* The stock label's font: every glyph ten units wide in a 16 unit em. */
static Glyph stockGlyphs[96];
static Font stockFont = { "normalFont", 16, 96, fakeMaterial, stockGlyphs };
static FontHandle UI_GetFontHandle(int fontEnum, float scale) { (void)fontEnum; (void)scale; return &stockFont; }
static float GetVirtualWidthFromRealWidth(float x) { return x; }
static float GetVirtualHeightFromRealHeight(float x) { return x; }
typedef struct { int infoValid, team; char name[32]; } clientInfo_t;
typedef struct { int nextValid; struct { int eType, clientNum, eFlags; } nextState; float lerpOrigin[3]; } centity_t;
static centity_t cg_entities[1024];
typedef struct { int clientNum, pm_flags; } playerState_t;
static struct { int time, renderingThirdPerson; playerState_t predictedPlayerState;
                struct { float fov_x, fov_y; int width, height; float vieworg[3], viewaxis[3][3]; } refdef;
                struct { clientInfo_t clientinfo[64]; } bgs; } cgState, *cg = &cgState;
static vec4_t color;

/* The view ray hits traceHit, a client number or the world's 1022, at
 * traceFraction of its length, through smoke that leaves smokeVisibility. */
typedef struct { float fraction; unsigned short entityNum; } trace_t;
static int traces, traceSkip, traceMask, traceHit = 1022, smokeTests;
static float traceStart[3], traceEnd[3], traceFraction = 0.25f, smokeVisibility = 1.0f, smokeStart[3], smokeEnd[3];
static float FX_GetServerVisibility(const vec_t *start, const vec_t *end) {
    memcpy(smokeStart, start, sizeof(smokeStart));
    memcpy(smokeEnd, end, sizeof(smokeEnd));
    smokeTests++;
    return smokeVisibility;
}
static void CG_TraceCapsule(trace_t *result, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end,
                            int skipNumber, int mask) {
    int axis;
    traces++;
    for (axis = 0; axis < 3; axis++) {
        assert(mins[axis] == 0.0f && maxs[axis] == 0.0f);
        traceStart[axis] = start[axis];
        traceEnd[axis] = end[axis];
    }
    traceSkip = skipNumber;
    traceMask = mask;
    result->fraction = traceFraction;
    result->entityNum = (unsigned short)traceHit;
}
'''

checks = r'''
static int Digit(char c) { return c <= '9' ? c - '0' : c - 'a' + 10; }

static int EnemyDraws(int time) {
    int before = draws;
    cg->time = time;
    CG_DrawEnemyNames();
    assert(draws - before <= 1);
    return draws - before;
}

int main(void) {
    const char *capture = "[FaZe] matZiin_GOD";
    char longName[41];
    FontHandle font;
    float width, stockWidth, scale, anchorX, anchorY, rgb[3] = { -1.0f, -1.0f, -1.0f };
    int i, x, y;

    for (i = 0; i < 96; i++)
        stockGlyphs[i] = (Glyph){ (unsigned short)(0x20 + i), 0, -11, 10, 9, 12, 0, 0, 1, 1 };

    /* Before the level registers the dvars the stock teammate label is
     * drawn, with the stock crosshair, and no enemy label. */
    assert(!CG_DrawTeammateName("Friend", 0.0f, 0.0f) && !creates && !draws);
    assert(!CG_GetTeammateNameColor(rgb) && !creates && rgb[0] == -1.0f && rgb[1] == -1.0f && rgb[2] == -1.0f);
    CG_DrawEnemyName("Enemy", 0.0f, 0.0f, 1.0f);
    assert(!creates && !draws);
    CG_PlayerNamesInit();
    assert(registered == 2);

    /* The first name builds the font: a white atlas whose alpha is the data's. */
    assert(CG_DrawTeammateName("^1[FaZe]^7 matZiin_GOD", 12.0f, -30.0f));
    assert(creates == 1 && createdSlot == R_HUD_IMAGE_NAME_FONT && createdWidth == CG_NAME_FONT_ATLAS_WIDTH &&
           createdHeight == CG_NAME_FONT_ATLAS_HEIGHT);
    for (y = 0; y < CG_NAME_FONT_ATLAS_HEIGHT; y++) {
        for (x = 0; x < CG_NAME_FONT_ATLAS_WIDTH; x++) {
            const byte *texel = createdPixels + (y * CG_NAME_FONT_ATLAS_WIDTH + x) * 4;
            assert(texel[0] == 255 && texel[1] == 255 && texel[2] == 255);
            assert(texel[3] == Digit(cg_nameFontAtlas[y][x]) * 17);
        }
    }
    assert(draws == 1);
    font = drawn[0].font;
    assert(font != &stockFont && font->material == fakeMaterial && font->name && font->name[0]);
    assert(CG_GetPlayerNameFont() == font && creates == 1 && lettered == font && letterings == 1);
    assert(font->pixelHeight == CG_NAME_FONT_PIXEL_HEIGHT && font->glyphCount == CG_NAME_FONT_GLYPH_COUNT);
    for (i = 0; i < CG_NAME_FONT_GLYPH_COUNT; i++) {
        const CG_NameFontGlyph *src = &cg_nameFontGlyphs[i];
        const Glyph *glyph = &font->glyphs[i];
        assert(glyph->letter == src->letter && glyph->x0 == src->x0 && glyph->y0 == src->y0 && glyph->dx == src->dx);
        assert(glyph->pixelWidth == src->width * CG_NAME_FONT_UNITS_PER_TEXEL &&
               glyph->pixelHeight == src->height * CG_NAME_FONT_UNITS_PER_TEXEL);
        assert(fabsf(glyph->s0 * CG_NAME_FONT_ATLAS_WIDTH - src->atlasX) < 1e-3f);
        assert(fabsf(glyph->t0 * CG_NAME_FONT_ATLAS_HEIGHT - src->atlasY) < 1e-3f);
        assert(fabsf(glyph->s1 * CG_NAME_FONT_ATLAS_WIDTH - (src->atlasX + src->width)) < 1e-3f);
        assert(fabsf(glyph->t1 * CG_NAME_FONT_ATLAS_HEIGHT - (src->atlasY + src->height)) < 1e-3f);
        /* The engine's lookup finds every letter of the table. */
        assert(R_GetCharacterGlyph(font, src->letter) == glyph);
    }
    assert(R_GetCharacterGlyph(font, 0x81) == &font->glyphs[14] && R_GetCharacterGlyph(font, 0x2603) == &font->glyphs[14]);

    /* Centred on the anchor, in the capture's teal, with capitals 7 units tall. */
    scale = drawn[0].scale;
    width = (float)UI_TextWidth(capture, 0x7fffffff, font, scale);
    assert(!strcmp(drawn[0].text, capture) && drawn[0].maxChars == 0x7fffffff && width > 40.0f);
    assert(drawn[0].x == 12.0f - 0.5f * width && drawn[0].y == -30.0f);
    assert(drawn[0].horzAlign == 7 && drawn[0].vertAlign == 7 && drawn[0].style == 3);
    /* The capture's bytes once R_ConvertColorToBytes truncates the color. */
    assert((int)(drawn[0].color[0] * 255.0f) == ''' + str(TEAMMATE_BYTES[0]) + r''' &&
           (int)(drawn[0].color[1] * 255.0f) == ''' + str(TEAMMATE_BYTES[1]) + r''' &&
           (int)(drawn[0].color[2] * 255.0f) == ''' + str(TEAMMATE_BYTES[2]) + r''' && drawn[0].color[3] == 1.0f);
    {
        float cap = CAP_TEXELS * CG_NAME_FONT_UNITS_PER_TEXEL * 48.0f * scale / font->pixelHeight;
        assert(cap > 6.8f && cap < 7.3f);
    }

    /* Only a caret and a digit are a color code; other letters, accents
     * included, are kept, and the name keeps the stock 31 letters. */
    draws = 0;
    assert(CG_DrawTeammateName("^^7x^", 0.0f, 0.0f) && !strcmp(drawn[0].text, "^x^"));
    assert(CG_DrawTeammateName("Mart\xedn", 0.0f, 0.0f) && !strcmp(drawn[1].text, "Mart\xedn"));
    assert(R_GetCharacterGlyph(font, 0xed)->letter == 0xed);
    memset(longName, 'a', 40);
    longName[40] = 0;
    assert(CG_DrawTeammateName(longName, 0.0f, 0.0f) && strlen(drawn[2].text) == 31);
    /* A name of color codes alone is drawn as nothing at all. */
    assert(CG_DrawTeammateName("^1^2", 0.0f, 0.0f) && draws == 3 && creates == 1);

    /* Switched off, the stock label is drawn; back on, the font is kept. */
    teamNames.current.enabled = 0;
    assert(!CG_DrawTeammateName("Friend", 0.0f, 0.0f) && draws == 3);
    teamNames.current.enabled = 1;
    assert(CG_DrawTeammateName("Friend", 0.0f, 0.0f) && draws == 4 && creates == 1);

    /* The crosshair over a teammate takes the label's color while the label
     * is drawn, and keeps its stock green with the stock label. */
    assert(CG_GetTeammateNameColor(rgb) && !memcmp(rgb, drawn[3].color, sizeof(rgb)));
    teamNames.current.enabled = 0;
    rgb[0] = rgb[1] = rgb[2] = -1.0f;
    assert(!CG_GetTeammateNameColor(rgb) && rgb[0] == -1.0f && rgb[1] == -1.0f && rgb[2] == -1.0f);
    teamNames.current.enabled = 1;

    /* Without its image the stock label stays, with the stock crosshair,
     * without a retry each frame, until the next level. */
    CG_PlayerNamesInit();
    createFails = 1;
    assert(!CG_DrawTeammateName("Friend", 0.0f, 0.0f) && !CG_DrawTeammateName("Friend", 0.0f, 0.0f));
    assert(!CG_GetTeammateNameColor(rgb) && rgb[0] == -1.0f && rgb[1] == -1.0f && rgb[2] == -1.0f);
    assert(!CG_GetPlayerNameFont() && creates == 2 && draws == 4 && letterings == 1);
    CG_PlayerNamesInit();
    createFails = 0;
    /* Other WWII text can build the font first, with the labels off. */
    teamNames.current.enabled = 0;
    assert(CG_GetPlayerNameFont() == font && creates == 3 && lettered == font && letterings == 2);
    teamNames.current.enabled = 1;
    assert(CG_DrawTeammateName("Friend", 0.0f, 0.0f) && creates == 3 && draws == 5);

    /* The friendly name loop draws the teal name where the stock label is. */
    cg->refdef.fov_x = cg->refdef.fov_y = 90.0f;
    cg->refdef.width = 1280;
    cg->refdef.height = 720;
    for (i = 0; i < 3; i++)
        cg->refdef.viewaxis[i][i] = 1.0f;
    cg->bgs.clientinfo[0].team = 1;
    cg->bgs.clientinfo[63] = (clientInfo_t){ 1, 1, "^2Friend" };
    cg_entities[63] = (centity_t){ 1, { 1, 63, 0 }, { 1000.0f, 200.0f, 0.0f } };
    draws = 0;
    CG_DrawFriendlyNames();
    assert(draws == 1 && drawn[0].font != &stockFont && !strcmp(drawn[0].text, "Friend"));
    width = (float)UI_TextWidth("Friend", 0x7fffffff, drawn[0].font, drawn[0].scale);
    teamNames.current.enabled = 0;
    CG_DrawFriendlyNames();
    assert(draws == 2 && drawn[1].font == &stockFont && !strcmp(drawn[1].text, "^2Friend") && drawn[1].style == 3);
    stockWidth = (float)UI_TextWidth("^2Friend", 32, &stockFont, .23f);
    assert(fabsf(drawn[0].x + 0.5f * width - (drawn[1].x + 0.5f * stockWidth)) < 1e-3f && drawn[0].y == drawn[1].y);
    assert(drawn[1].x + 0.5f * stockWidth < 0.0f && drawn[1].y < 0.0f);
    teamNames.current.enabled = 1;

    /* No enemy is named until the server flags one under the crosshair. */
    cg->refdef.vieworg[0] = 10.0f;
    cg->refdef.vieworg[1] = -20.0f;
    cg->refdef.vieworg[2] = 60.0f;
    cg->bgs.clientinfo[5] = (clientInfo_t){ 1, 2, "^1[FR]^7 ML82" };
    cg_entities[5] = (centity_t){ 1, { 1, 5, 0 }, { 1000.0f, -150.0f, 0.0f } };
    traceHit = 5;
    draws = 0;
    assert(!EnemyDraws(50000) && !traces);
    cg->predictedPlayerState.pm_flags = 0x200000 | 0x100;
    assert(EnemyDraws(50000) && traces == 1);

    /* The view ray, skipping the player, against bodies and solid world as
     * the stock crosshair names traced it. */
    assert(traceSkip == 0 && traceMask == 0x2000001);
    assert(traceStart[0] == 10.0f && traceStart[1] == -20.0f && traceStart[2] == 60.0f);
    assert(fabsf(traceEnd[0] - 8202.0f) < 1e-3f && traceEnd[1] == -20.0f && traceEnd[2] == 60.0f);

    /* Above the head like a teammate's, in the same letters, in the capture's red. */
    assert(!strcmp(drawn[0].text, "[FR] ML82") && drawn[0].font == font && creates == 3);
    assert(drawn[0].maxChars == 0x7fffffff && drawn[0].horzAlign == 7 && drawn[0].vertAlign == 7 &&
           drawn[0].scale == scale && drawn[0].style == 3);
    assert((int)(drawn[0].color[0] * 255.0f) == ''' + str(ENEMY_BYTES[0]) + r''' &&
           (int)(drawn[0].color[1] * 255.0f) == ''' + str(ENEMY_BYTES[1]) + r''' &&
           (int)(drawn[0].color[2] * 255.0f) == ''' + str(ENEMY_BYTES[2]) + r''' && drawn[0].color[3] == 1.0f);
    assert(CG_GetPlayerNamePosition(&cg_entities[5], &anchorX, &anchorY));
    assert(fabsf(anchorX - 640.0f * 130.0f / 990.0f) < 0.01f && fabsf(anchorY + 360.0f * 16.0f / 990.0f) < 0.01f);
    width = (float)UI_TextWidth("[FR] ML82", 0x7fffffff, font, scale);
    assert(fabsf(drawn[0].x + 0.5f * width - anchorX) < 1e-3f && drawn[0].y == anchorY);

    /* Once the aim leaves him the name holds 50 ms, then fades out over 100. */
    traceHit = 1022;
    assert(EnemyDraws(50049) && drawn[1].color[3] == 1.0f);
    assert(EnemyDraws(50100) && fabsf(drawn[2].color[3] - 0.5f) < 1e-6f);
    assert((int)(drawn[2].color[0] * 255.0f) == ''' + str(ENEMY_BYTES[0]) + r''');
    assert(EnemyDraws(50149) && fabsf(drawn[3].color[3] - 0.01f) < 1e-6f);
    assert(!EnemyDraws(50150));

    /* A teammate in the way is not named, nor an enemy the server no longer
     * flags, out of the weapon's range. */
    traceHit = 63;
    assert(!EnemyDraws(51000));
    cg->predictedPlayerState.pm_flags = 0x100;
    traceHit = 5;
    traces = 0;
    assert(!EnemyDraws(52000) && !traces);

    /* Nor one behind smoke, which the server's flag ignores: the smoke between
     * the eye and where the ray meets him leaves less than a fifth visible. */
    cg->predictedPlayerState.pm_flags = 0x200000;
    smokeVisibility = 0.19f;
    smokeTests = 0;
    assert(!EnemyDraws(52500) && smokeTests == 1);
    for (i = 0; i < 3; i++) {
        assert(smokeStart[i] == traceStart[i]);
        assert(fabsf(smokeEnd[i] - (traceStart[i] + (traceEnd[i] - traceStart[i]) * traceFraction)) < 1e-3f);
    }
    smokeVisibility = 0.2f;
    assert(EnemyDraws(52500));
    smokeVisibility = 0.19f;
    assert(EnemyDraws(52600) && fabsf(drawn[5].color[3] - 0.5f) < 1e-6f && !EnemyDraws(52650));
    smokeVisibility = 1.0f;

    /* In free-for-all every other player is an enemy. */
    cg->predictedPlayerState.pm_flags = 0x200000;
    cg->bgs.clientinfo[0].team = 0;
    cg->bgs.clientinfo[5].team = 0;
    assert(EnemyDraws(53000));
    cg->bgs.clientinfo[0].team = 1;
    cg->bgs.clientinfo[5].team = 2;

    /* Nor in third person. */
    cg->renderingThirdPerson = 1;
    assert(!EnemyDraws(54000));
    cg->renderingThirdPerson = 0;

    /* A lingering name goes with its player: out of the snapshot, another
     * entity in the slot, disconnected, joined the player's team, or his head
     * off the screen. */
    assert(EnemyDraws(55000));
    traceHit = 1022;
    cg_entities[5].nextValid = 0;
    assert(!EnemyDraws(55010));
    cg_entities[5].nextValid = 1;
    cg_entities[5].nextState.eType = 4;
    assert(!EnemyDraws(55020));
    cg_entities[5].nextState.eType = 1;
    cg_entities[5].nextState.clientNum = 6;
    assert(!EnemyDraws(55030));
    cg_entities[5].nextState.clientNum = 5;
    cg->bgs.clientinfo[5].infoValid = 0;
    assert(!EnemyDraws(55040));
    cg->bgs.clientinfo[5].infoValid = 1;
    cg->bgs.clientinfo[5].team = 1;
    assert(!EnemyDraws(55050));
    cg->bgs.clientinfo[5].team = 2;
    cg_entities[5].lerpOrigin[2] = 2000.0f;
    assert(!EnemyDraws(55060));
    cg_entities[5].lerpOrigin[2] = 0.0f;
    assert(EnemyDraws(55070));

    /* A clock that starts again behind the sighting, on another server,
     * does not keep the name up. */
    traceHit = 5;
    assert(EnemyDraws(60000));
    traceHit = 1022;
    assert(!EnemyDraws(1000) && !EnemyDraws(1100));

    /* Switched off, or without its image, no enemy is named: CoD2 has no
     * label to fall back to. */
    traceHit = 5;
    enemyNames.current.enabled = 0;
    assert(!EnemyDraws(2000));
    enemyNames.current.enabled = 1;
    CG_PlayerNamesInit();
    createFails = 1;
    assert(!EnemyDraws(2010) && !EnemyDraws(2020) && creates == 4);
    createFails = 0;
    free(createdPixels);
    return 0;
}
'''

mutants = [
    ("if (src[0] == '^' && src[1] >= '0' && src[1] <= '9') {", "if (src[0] == '^' && src[1]) {"),
    ("if (src[0] == '^' && src[1] >= '0' && src[1] <= '9') {", 'if (0) {'),
    ('while (*src && length < NAME_LEN - 1) {', 'while (*src && length < NAME_LEN) {'),
    ('    if (!clean[0])\n        return;\n', ''),
    ('x - 0.5f * width, y,', 'x, y,'),
    ('#define NAME_SCALE 0.21f', '#define NAME_SCALE 0.23f'),
    ('{ 98.5f / 255.0f, 141.5f / 255.0f, 126.5f / 255.0f, 1.0f }', '{ 0.55f, 1.0f, 0.55f, 1.0f }'),
    ('{ 98.5f / 255.0f, 141.5f / 255.0f, 126.5f / 255.0f, 1.0f }', '{ 0.386f, 0.552f, 0.495f, 1.0f }'),
    ('{ 165.5f / 255.0f, 82.5f / 255.0f, 63.5f / 255.0f }', '{ 1.0f, 0.25f, 0.25f }'),
    ('{ 165.5f / 255.0f, 82.5f / 255.0f, 63.5f / 255.0f }', '{ 0.648f, 0.322f, 0.246f }'),
    ('    color[3] = alpha;\n', '    color[3] = 1.0f;\n'),
    ('#define NAME_STYLE 3', '#define NAME_STYLE 0'),
    ('#define NAME_ALIGN 7', '#define NAME_ALIGN 1'),
    ('cg_hudTeammateNames && cg_hudTeammateNames->current.enabled && ', 'cg_hudTeammateNames && '),
    ('cg_hudTeammateNames->current.enabled && CG_NameFontReady()', 'cg_hudTeammateNames->current.enabled'),
    ('    if (!CG_TeammateNamesDrawn())\n        return 0;\n    color[0]', '    color[0]'),
    ('    color[2] = s_teammateColor[2];\n', '    color[2] = s_teammateColor[1];\n'),
    ('    color[2] = s_teammateColor[2];\n    return 1;', '    color[2] = s_teammateColor[2];\n    return 0;'),
    ('!cg_hudEnemyNames->current.enabled || ', ''),
    ('!cg_hudEnemyNames->current.enabled || !CG_NameFontReady())', '!cg_hudEnemyNames->current.enabled)'),
    ('Dvar_RegisterBool_mac("cg_hudEnemyNames", 1, 0x1001)', 'Dvar_RegisterBool_mac("cg_hudEnemyNames", 0, 0x1001)'),
    ('    s_nameFontState = NAME_FONT_FAILED;\n\n', '\n'),
    ('    s_nameFontState = NAME_FONT_UNTRIED;\n}', '}'),
    ('(byte)(src->width * CG_NAME_FONT_UNITS_PER_TEXEL)', '(byte)src->width'),
    ('(byte)(src->height * CG_NAME_FONT_UNITS_PER_TEXEL)', '(byte)src->height'),
    ('(float)src->atlasX / CG_NAME_FONT_ATLAS_WIDTH', '(float)src->atlasX / CG_NAME_FONT_ATLAS_HEIGHT'),
    ('(float)(src->atlasY + src->height) / CG_NAME_FONT_ATLAS_HEIGHT',
     '(float)(src->atlasY + src->height - 1) / CG_NAME_FONT_ATLAS_HEIGHT'),
    ("c - 'a' + 10", "c - 'a'"),
    ('* CG_NAME_FONT_ALPHA_STEP);', '* 16);'),
    ('texel[0] = texel[1] = texel[2] = 255;', 'texel[0] = texel[1] = texel[2] = 0;'),
    ('R_CreateHudImageMaterial(R_HUD_IMAGE_NAME_FONT,', 'R_CreateHudImageMaterial(R_HUD_IMAGE_MINIMAP,'),
    ('s_nameFont.glyphCount = CG_NAME_FONT_GLYPH_COUNT;', 's_nameFont.glyphCount = 96;'),
    ('s_nameFont.pixelHeight = CG_NAME_FONT_PIXEL_HEIGHT;', 's_nameFont.pixelHeight = 16;'),
    ('glyph->x0 = src->x0;', 'glyph->x0 = 0;'),
    ('glyph->y0 = src->y0;', 'glyph->y0 = 0;'),
    ('glyph->dx = src->dx;', 'glyph->dx = src->width;'),
    ('    if (!material)\n        return 0;\n', ''),
    ('    return CG_NameFontReady() ? &s_nameFont : NULL;\n', '    return &s_nameFont;\n'),
    ('    R_AddWwiiLettering(&s_nameFont);\n', ''),
    ('    return CG_NameFontReady() ? &s_nameFont : NULL;\n', '    return CG_TeammateNamesDrawn() ? &s_nameFont : NULL;\n'),
]
position_mutants = [
    ('    return fabsf(*x) <= halfWidth && fabsf(*y) <= halfHeight;', '    return 1;'),
    ('cent->nextState.eFlags & 4 ? 48 : 76', 'cent->nextState.eFlags & 4 ? 48 : 64'),
]
friendly_mutants = [
    ('if (CG_DrawTeammateName(ci->name, x, y)) continue;', 'CG_DrawTeammateName(ci->name, x, y);'),
    ('if (CG_DrawTeammateName(ci->name, x, y)) continue;', 'if (CG_DrawTeammateName(ci->name, x, 0)) continue;'),
    ('if (CG_DrawTeammateName(ci->name, x, y)) continue;', ''),
]
enemy_mutants = [
    ('    if (ps->pm_flags & 0x200000) {', '    if (1) {'),
    ('    if (ps->pm_flags & 0x200000) {', '    if (ps->pm_flags & 0x100000) {'),
    ('(!team || ci->team != team)', '(ci->team != team)'),
    ('(!team || ci->team != team)', '(!team || ci->team == team)'),
    ('CG_FadeColor(lastTime, 150, 100)', 'CG_FadeColor(lastTime, 1000, 100)'),
    ('CG_FadeColor(lastTime, 150, 100)', 'CG_FadeColor(lastTime, 150, 150)'),
    ('    if (lastTime > cg->time)\n        lastTime = 0;\n', ''),
    ('    if (cg->renderingThirdPerson)\n        return;\n', ''),
    ('!cent->nextValid || ', ''),
    ('cent->nextState.eType != 1 || ', ''),
    ('cent->nextState.clientNum != lastClient ||', ''),
    ('        !ci->infoValid || (team && ci->team == team)', '        (team && ci->team == team)'),
    ('(team && ci->team == team) || !ci->name[0])', '!ci->name[0])'),
    (' && FX_GetServerVisibility(start, contact) >= 0.2f) {', ') {'),
    ('FX_GetServerVisibility(start, contact) >= 0.2f', 'FX_GetServerVisibility(start, contact) > 0.2f'),
    ('FX_GetServerVisibility(start, contact)', 'FX_GetServerVisibility(start, end)'),
    ('(end[axis] - start[axis]) * trace.fraction;', '(end[axis] - start[axis]);'),
    ('ps->clientNum, 0x2000001)', 'ps->clientNum, 0x2000000)'),
    ('ps->clientNum, 0x2000001)', '1023, 0x2000001)'),
    ('* 8192.0f;', '* 1024.0f;'),
    ('start[axis] = cg->refdef.vieworg[axis];', 'start[axis] = 0.0f;'),
    ('CG_DrawEnemyName(ci->name, x, y, fade[3]);', 'CG_DrawEnemyName(ci->name, x, y, 1.0f);'),
    ('    if (CG_GetPlayerNamePosition(cent, &x, &y))\n        CG_DrawEnemyName',
     '    x = y = 0;\n    CG_GetPlayerNamePosition(cent, &x, &y);\n        CG_DrawEnemyName'),
]

variants = [(names_module, position, friendly, enemy)]
for old, new in mutants:
    assert names_module.count(old) == 1, old
    variants.append((names_module.replace(old, new), position, friendly, enemy))
for old, new in position_mutants:
    assert position.count(old) == 1, old
    variants.append((names_module, position.replace(old, new), friendly, enemy))
for old, new in friendly_mutants:
    assert friendly.count(old) == 1, old
    variants.append((names_module, position, friendly.replace(old, new), enemy))
for old, new in enemy_mutants:
    assert enemy.count(old) == 1, old
    variants.append((names_module, position, friendly, enemy.replace(old, new)))

with tempfile.TemporaryDirectory(prefix='cod2-player-names-') as directory:
    path = Path(directory) / 'names.c'
    binary = Path(directory) / 'names'
    for index, parts in enumerate(variants):
        path.write_text(support + engine + ''.join(parts) + checks)
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wno-unused-function', str(path), '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
print(f'PASS: the name font holds {len(glyph_rows)} Windows-1252 glyphs of the committed Barlow Medium in the order '
      'R_GetCharacterGlyph searches, padded and apart in its atlas, with capitals 0.7 em tall and the WWII capture\'s '
      'width for their height; while cg_hudTeammateNames is on, the friendly name loop draws each teammate\'s name '
      'without color codes, centred where the stock label was, in the capture\'s teal with capitals 7 units tall and '
      'the stock shadow, building the atlas once per level, and lends the crosshair that teal; off or without its '
      'image the stock label and crosshair stay; while '
      'cg_hudEnemyNames is on and the server flags an enemy under the crosshair, the enemy the view ray hits, unless '
      'smoke leaves less than a fifth visible, is named above his head in the same letters in the enemy capture\'s '
      'red, held 50 ms and faded over 100 once the aim '
      'leaves him, every other player in free-for-all, never a teammate, in third person, after his player leaves or '
      'across a clock reset; the font is lent to other WWII text, labels on or off, while its image exists; '
      f'{len(broken)} broken font tables rejected and {len(variants) - 1} failing regression mutants')
