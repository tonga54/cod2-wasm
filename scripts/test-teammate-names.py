#!/usr/bin/env python3
"""Check the WWII teammate name: the baked Barlow font against its source and
the WWII capture, then the production module, compiled with the engine's real
glyph lookup and text width and the real friendly name loop: the atlas it
uploads, the font it builds, the cleaned name it draws in teal, and the stock
label it leaves when switched off or without its image."""
from pathlib import Path
import hashlib
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
module = (root / 'src/PC/cgame_mp/cg_teamnames_mp.c').read_text()
data_path = root / 'src/headers/PC/cgame_mp/cg_namefont_data.h'
data = data_path.read_text()
draw = (root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text()
main = (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()
hud2d_header = (root / 'src/headers/PC/gfx_d3d/r_hud2d.h').read_text()
hud2d = (root / 'src/PC/gfx_d3d/r_hud2d.c').read_text()
fonts = (root / 'src/PC/gfx_d3d/r_font.c').read_text()
ui = (root / 'src/PC/ui_mp/ui_main_mp.c').read_text()

# Measured on the WWII capture, which stays out of the repository: capitals
# 16 px tall, "[FaZe] matZiin_GOD" 204 px from the first ink to the last, and
# the brightest eighth of the letters' pixels averaging this color.
CAPTURE_TEXT = b'[FaZe] matZiin_GOD'
CAPTURE_WIDTH_PER_CAP = 204 / 16
CAPTURE_COLOR = (0.386, 0.552, 0.495)
CAPTURE_BYTES = tuple(round(channel * 255) for channel in CAPTURE_COLOR)


def function(text, name):
    match = re.search(r'^((?:static )?[^\n;]*\b' + name + r'\([^;]*?\))\s*\{', text, re.M)
    assert match, name
    brace = text.index('{', match.end() - 1)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


# The friendly name loop hands every label to the module, which each level
# registers, and the renderer has a slot for the font's image.
assert draw.count('#include "headers/PC/cgame_mp/cg_teamnames.h"\n') == 1
friendly = function(draw, 'CG_DrawFriendlyNames')
assert friendly.count('CG_DrawTeammateName(') == 1
assert friendly.index('CG_DrawTeammateName(') < friendly.index('UI_DrawText(')
assert main.count('#include "headers/PC/cgame_mp/cg_teamnames.h"\n') == 1
assert main.count('    CG_WeaponPromptInit();\n    CG_TeamNamesInit();\n') == 1
slot = int(re.search(r'^#define R_HUD_IMAGE_NAME_FONT (\d+)$', hud2d_header, re.M).group(1))
count = int(re.search(r'^#define R_HUD_IMAGE_COUNT (\d+)$', hud2d_header, re.M).group(1))
names = re.search(r's_hudImageNames\[R_HUD_IMAGE_COUNT\] = \{(.*?)\};', hud2d, re.S).group(1)
names = re.findall(r'"([^"]+)"', names)
assert slot == count - 1 and len(names) == count and names[slot] == '*hud_name_font'

# The data was baked from the committed font, whose license travels with it.
font_file = root / 'third_party/barlow/Barlow-SemiBold.ttf'
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


# As wide for its height as the capture's name.
pen = sum(by_letter[letter][3] for letter in CAPTURE_TEXT[:-1]) / UNITS
ink_width = pen + ink_columns(CAPTURE_TEXT[-1])[1] - ink_columns(CAPTURE_TEXT[0])[0]
assert abs(ink_width / cap_texels / CAPTURE_WIDTH_PER_CAP - 1) < 0.03, ink_width / cap_texels

engine = (r'''
#define CL_TextWidth R_TextWidth
#define CL_NormalizedTextScale R_NormalizedTextScale
static const Glyph *''' + function(fonts, 'R_GetCharacterGlyph_inl') + function(fonts, 'R_GetCharacterGlyph')
          + function(fonts, 'R_TextWidth_inl') + function(fonts, 'R_TextWidth')
          + function(fonts, 'R_NormalizedTextScale') + function(ui, 'UI_TextWidth'))
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

static dvar_t teamNames = { { 1 } };
static int registered;
static const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags) {
    assert(!strcmp(name, "cg_hudTeammateNames") && value == 1 && flags == 0x1001);
    registered++;
    return &teamNames;
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
typedef struct { char text[64]; int maxChars; FontHandle font; float x, y; int horzAlign, vertAlign; float scale;
                 vec4_t color; int style; } Draw;
static Draw drawn[8];
static int draws;
static void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign,
                        float scale, const vec_t *color, int style) {
    Draw *d = &drawn[draws++];
    assert(draws <= 8 && strlen(text) < sizeof(d->text));
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
static struct { struct { int clientNum; } predictedPlayerState;
                struct { float fov_x, fov_y; int width, height; float vieworg[3], viewaxis[3][3]; } refdef;
                struct { clientInfo_t clientinfo[64]; } bgs; } cgState, *cg = &cgState;
'''

checks = r'''
static int Digit(char c) { return c <= '9' ? c - '0' : c - 'a' + 10; }

int main(void) {
    const char *capture = "[FaZe] matZiin_GOD";
    char longName[41];
    FontHandle font;
    float width, stockWidth;
    int i, x, y;

    for (i = 0; i < 96; i++)
        stockGlyphs[i] = (Glyph){ (unsigned short)(0x20 + i), 0, -11, 10, 9, 12, 0, 0, 1, 1 };

    /* Before the level registers the dvar the stock label is drawn. */
    assert(!CG_DrawTeammateName("Friend", 0.0f, 0.0f) && !creates && !draws);
    CG_TeamNamesInit();
    assert(registered == 1);

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
    width = (float)UI_TextWidth(capture, 0x7fffffff, font, drawn[0].scale);
    assert(!strcmp(drawn[0].text, capture) && drawn[0].maxChars == 0x7fffffff && width > 40.0f);
    assert(drawn[0].x == 12.0f - 0.5f * width && drawn[0].y == -30.0f);
    assert(drawn[0].horzAlign == 7 && drawn[0].vertAlign == 7 && drawn[0].style == 3);
    /* The capture's bytes once R_ConvertColorToBytes truncates the color. */
    assert((int)(drawn[0].color[0] * 255.0f) == ''' + str(CAPTURE_BYTES[0]) + r''' &&
           (int)(drawn[0].color[1] * 255.0f) == ''' + str(CAPTURE_BYTES[1]) + r''' &&
           (int)(drawn[0].color[2] * 255.0f) == ''' + str(CAPTURE_BYTES[2]) + r''' && drawn[0].color[3] == 1.0f);
    {
        float cap = CAP_TEXELS * CG_NAME_FONT_UNITS_PER_TEXEL * 48.0f * drawn[0].scale / font->pixelHeight;
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

    /* Without its image the stock label stays, without a retry each frame,
     * until the next level. */
    CG_TeamNamesInit();
    createFails = 1;
    assert(!CG_DrawTeammateName("Friend", 0.0f, 0.0f) && !CG_DrawTeammateName("Friend", 0.0f, 0.0f));
    assert(creates == 2 && draws == 4);
    CG_TeamNamesInit();
    createFails = 0;
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
    free(createdPixels);
    return 0;
}
'''

mutants = [
    ("if (src[0] == '^' && src[1] >= '0' && src[1] <= '9') {", "if (src[0] == '^' && src[1]) {"),
    ("if (src[0] == '^' && src[1] >= '0' && src[1] <= '9') {", 'if (0) {'),
    ('while (*src && length < TEAM_NAME_LEN - 1) {', 'while (*src && length < TEAM_NAME_LEN) {'),
    ('    if (!clean[0])\n        return 1;\n', '    if (!clean[0])\n        return 0;\n'),
    ('x - 0.5f * width, y,', 'x, y,'),
    ('#define TEAM_NAME_SCALE 0.21f', '#define TEAM_NAME_SCALE 0.23f'),
    ('{ 98.5f / 255.0f, 141.5f / 255.0f, 126.5f / 255.0f, 1.0f }', '{ 0.55f, 1.0f, 0.55f, 1.0f }'),
    ('{ 98.5f / 255.0f, 141.5f / 255.0f, 126.5f / 255.0f, 1.0f }', '{ 0.386f, 0.552f, 0.495f, 1.0f }'),
    ('#define TEAM_NAME_STYLE 3', '#define TEAM_NAME_STYLE 0'),
    ('#define TEAM_NAME_ALIGN 7', '#define TEAM_NAME_ALIGN 1'),
    ('!cg_hudTeammateNames->current.enabled || ', ''),
    ('    s_teamNameFontState = TEAM_NAME_FONT_FAILED;\n\n', '\n'),
    ('    s_teamNameFontState = TEAM_NAME_FONT_UNTRIED;\n}', '}'),
    ('(byte)(src->width * CG_NAME_FONT_UNITS_PER_TEXEL)', '(byte)src->width'),
    ('(byte)(src->height * CG_NAME_FONT_UNITS_PER_TEXEL)', '(byte)src->height'),
    ('(float)src->atlasX / CG_NAME_FONT_ATLAS_WIDTH', '(float)src->atlasX / CG_NAME_FONT_ATLAS_HEIGHT'),
    ('(float)(src->atlasY + src->height) / CG_NAME_FONT_ATLAS_HEIGHT',
     '(float)(src->atlasY + src->height - 1) / CG_NAME_FONT_ATLAS_HEIGHT'),
    ("c - 'a' + 10", "c - 'a'"),
    ('* CG_NAME_FONT_ALPHA_STEP);', '* 16);'),
    ('texel[0] = texel[1] = texel[2] = 255;', 'texel[0] = texel[1] = texel[2] = 0;'),
    ('R_CreateHudImageMaterial(R_HUD_IMAGE_NAME_FONT,', 'R_CreateHudImageMaterial(R_HUD_IMAGE_MINIMAP,'),
    ('s_teamNameFont.glyphCount = CG_NAME_FONT_GLYPH_COUNT;', 's_teamNameFont.glyphCount = 96;'),
    ('s_teamNameFont.pixelHeight = CG_NAME_FONT_PIXEL_HEIGHT;', 's_teamNameFont.pixelHeight = 16;'),
    ('glyph->x0 = src->x0;', 'glyph->x0 = 0;'),
    ('glyph->y0 = src->y0;', 'glyph->y0 = 0;'),
    ('glyph->dx = src->dx;', 'glyph->dx = src->width;'),
    ('    if (!material)\n        return 0;\n', ''),
]
hook_mutants = [
    ('if (CG_DrawTeammateName(ci->name, x, y)) continue;', 'CG_DrawTeammateName(ci->name, x, y);'),
    ('if (CG_DrawTeammateName(ci->name, x, y)) continue;', 'if (CG_DrawTeammateName(ci->name, x, 0)) continue;'),
    ('if (CG_DrawTeammateName(ci->name, x, y)) continue;', ''),
]

variants = [(names_module, friendly)]
for old, new in mutants:
    assert names_module.count(old) == 1, old
    variants.append((names_module.replace(old, new), friendly))
for old, new in hook_mutants:
    assert friendly.count(old) == 1, old
    variants.append((names_module, friendly.replace(old, new)))

with tempfile.TemporaryDirectory(prefix='cod2-teammate-names-') as directory:
    path = Path(directory) / 'names.c'
    binary = Path(directory) / 'names'
    for index, (module_variant, friendly_variant) in enumerate(variants):
        path.write_text(support + engine + module_variant + friendly_variant + checks)
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wno-unused-function', str(path), '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
print(f'PASS: the name font holds {len(glyph_rows)} Windows-1252 glyphs of the committed Barlow SemiBold in the order '
      'R_GetCharacterGlyph searches, padded and apart in its atlas, with capitals 0.7 em tall and the WWII capture\'s '
      'width for their height; while cg_hudTeammateNames is on, the friendly name loop draws each teammate\'s name '
      'without color codes, centred where the stock label was, in the capture\'s teal with capitals 7 units tall and '
      'the stock shadow, building the atlas once per level; off or without its image the stock label stays; '
      f'{len(broken)} broken font tables rejected and {len(variants) - 1} failing regression mutants')
