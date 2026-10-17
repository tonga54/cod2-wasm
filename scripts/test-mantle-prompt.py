#!/usr/bin/env python3
"""Run the production WWII mantle prompt inside the real stock mantle hint: its
layout, colours and lettering, the arrow image it builds, and when the stock
hint stays."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_mantleprompt_mp.c').read_text()
newdraw = (root / 'src/PC/cgame_mp/cg_newDraw_mp.c').read_text()
main = (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()
hud2d_header = (root / 'src/headers/PC/gfx_d3d/r_hud2d.h').read_text()
hud2d = (root / 'src/PC/gfx_d3d/r_hud2d.c').read_text()


def function(text, name):
    match = re.search(r'^((?:static )?[^\n;]*\b' + name + r'\([^;]*?\))\s*\{', text, re.M)
    assert match, name
    brace = text.index('{', match.end() - 1)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


# Every level registers the setting and builds the arrow anew; the arrow has a
# HUD image slot of its own.
assert newdraw.count('#include "headers/PC/cgame_mp/cg_mantleprompt.h"\n') == 1
assert newdraw.count('CG_DrawMantlePrompt(') == 1
assert main.count('#include "headers/PC/cgame_mp/cg_mantleprompt.h"\n') == 1
assert main.count('    CG_PlayerNamesInit();\n    CG_MantlePromptInit();\n') == 1
slot = int(re.search(r'^#define R_HUD_IMAGE_MANTLE_ARROW (\d+)$', hud2d_header, re.M).group(1))
count = int(re.search(r'^#define R_HUD_IMAGE_COUNT (\d+)$', hud2d_header, re.M).group(1))
names = re.findall(r'"([^"]+)"', re.search(r's_hudImageNames\[R_HUD_IMAGE_COUNT\] = \{(.*?)\};', hud2d, re.S).group(1))
assert slot < count and len(names) == count and names[slot] == '*hud_mantle_arrow'

prompt = source[source.index('/* Virtual units from the stock hint'):]
hint = function(newdraw, 'CG_DrawMantleHint')

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte;
typedef float vec_t;
typedef vec_t vec4_t[4];
typedef const char *MaterialHandle;
struct Font_s { int id; };
typedef struct Font_s *FontHandle;
typedef struct { struct { int enabled; } current; } dvar_t;
typedef struct rectDef_s { float x, y, w, h; int horzAlign, vertAlign; } rectDef_t;
#define R_HUD_IMAGE_MANTLE_ARROW ''' + str(slot) + r'''
static struct { int renderingThirdPerson; struct { int pm_type, pm_flags, stats[1]; struct { int flags; } mantleState; }
                predictedPlayerState; } cgState;
static struct { struct { MaterialHandle mantleHint; } media; } cgsState;
#define cg (&cgState)
#define cgs (&cgsState)
static dvar_t drawMantleHint = { { 1 } }, promptDvar = { { 1 } };
static const dvar_t *cg_drawMantleHint = &drawMantleHint;
static int registered;
static const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags) {
    assert(!strcmp(name, "cg_hudMantlePrompt") && value == 1 && flags == 0x1001);
    registered++;
    return &promptDvar;
}
/* The labels' lettering, 25 font units a letter and 52 to the em, and the
 * menu's font, whose widths the stock hint takes rounded down to a unit. */
static struct Font_s nameFont = { 1 }, menuFont = { 2 };
static int fontReady = 1, fontAsks;
static struct Font_s *CG_GetPlayerNameFont(void) { fontAsks++; return fontReady ? &nameFont : NULL; }
static int CL_TextWidth(const char *text, int maxChars, FontHandle font) {
    assert(maxChars == 0 && font == &nameFont);
    return (int)strlen(text) * 25;
}
static float CL_NormalizedTextScale(FontHandle font, float scale) {
    assert(font == &nameFont && scale == 0.21f);
    return 48.0f * scale / 52.0f;
}
static int UI_TextWidth(const char *text, int maxChars, FontHandle font, float scale) {
    assert(maxChars == 0 && font == &menuFont && scale == 0.21f);
    return (int)(strlen(text) * 7);
}
static int UI_TextHeight(FontHandle font, float scale) { assert(font == &menuFont); return 20; }
static byte image[120 * 74 * 4];
static int creates, createSlot, createWidth, createHeight, createFails;
static MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height) {
    creates++; createSlot = slot; createWidth = width; createHeight = height;
    assert(width > 0 && height > 0 && width * height * 4 <= (int)sizeof(image));
    memcpy(image, bgra, width * height * 4);
    return createFails ? NULL : "*hud_mantle_arrow";
}
enum { TEXT, STRETCH, HANDLE };
typedef struct { int kind; char text[64]; MaterialHandle material; FontHandle font; float x, y, w, h, scale;
                 vec4_t color; int horz, vert, style; } Drawn;
static Drawn drawn[8];
static int drawCount;
static Drawn *record(int kind, float x, float y, int horzAlign, int vertAlign, const vec_t *color) {
    assert(drawCount < 8);
    Drawn *d = &drawn[drawCount++];
    memset(d, 0, sizeof(*d));
    d->kind = kind; d->x = x; d->y = y; d->horz = horzAlign; d->vert = vertAlign;
    memcpy(d->color, color, sizeof(d->color));
    return d;
}
static void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign,
                        int vertAlign, float scale, const vec_t *color, int style) {
    assert(maxChars == 0x7fffffff);
    Drawn *d = record(TEXT, x, y, horzAlign, vertAlign, color);
    snprintf(d->text, sizeof(d->text), "%s", text);
    d->font = font; d->scale = scale; d->style = style;
}
static void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1,
                              float s2, float t2, const vec_t *color, MaterialHandle material) {
    assert(s1 == 0 && t1 == 0 && s2 == 1 && t2 == 1 && material);
    Drawn *d = record(STRETCH, x, y, horzAlign, vertAlign, color);
    d->w = w; d->h = h; d->material = material;
}
static void UI_DrawHandlePic(float x, float y, float w, float h, int horzAlign, int vertAlign, const vec_t *color,
                             MaterialHandle material) {
    Drawn *d = record(HANDLE, x, y, horzAlign, vertAlign, color);
    d->w = w; d->h = h; d->material = material;
}
static void Controls_GetConfig(void) {}
static char jumpKey[32] = "Space", moveUpKey[32];
static int GetKeyBindingLocalizedString(const char *command, char *keys) {
    const char *key = !strcmp(command, "+gostand") ? jumpKey : moveUpKey;
    assert(!strcmp(command, "+gostand") || !strcmp(command, "+moveup"));
    strcpy(keys, *key ? key : "UNBOUND");
    return *key != 0;
}
static const char *UI_SafeTranslateString(const char *ref) {
    assert(!strcmp(ref, "PLATFORM_MANTLE"));
    return "Press [&&1] to  ";
}
static const char *UI_ReplaceConversionString(const char *text, const char *binding) {
    static char result[128];
    snprintf(result, sizeof(result), "Press [%s] to  ", binding);
    return result;
}
'''

checks = r'''
static int near(float a, float b) { return fabsf(a - b) < 1e-3f; }
static const float gold[3] = { 0.95f, 0.84f, 0.34f }, words[3] = { 0.96f, 0.95f, 0.92f }, white[3] = { 1, 1, 1 },
                   black[3] = { 0, 0, 0 };
static int horz = 2, vert = 2;
static int colored(const float *color, const float *rgb, float alpha) {
    return near(color[0], rgb[0]) && near(color[1], rgb[1]) && near(color[2], rgb[2]) && near(color[3], alpha);
}
/* Always the one-pixel shadow, whatever the menu's text style. */
static int text(int i, const char *value, float x, float y, const float *rgb) {
    const Drawn *d = &drawn[i];
    return d->kind == TEXT && !strcmp(d->text, value) && d->font == &nameFont && near(d->x, x) && near(d->y, y) &&
           near(d->scale, 0.21f) && d->horz == horz && d->vert == vert && d->style == 3 && colored(d->color, rgb, 1);
}
static int arrow(int i, float x, float y, const float *rgb, float alpha) {
    const Drawn *d = &drawn[i];
    return d->kind == STRETCH && !strcmp(d->material, "*hud_mantle_arrow") && near(d->x, x) && near(d->y, y) &&
           near(d->w, 31.2f) && near(d->h, 19.3f) && d->horz == horz && d->vert == vert && colored(d->color, rgb, alpha);
}
/* The advance as drawn, not rounded down to a unit. */
static float width(const char *value) { return (float)(strlen(value) * 25) * 48.0f * 0.21f / 52.0f; }
/* The key in gold, then "Press to" in near white and the arrow over its
 * shadow, one unit down and right: the capitals' baseline 3.5 under y, gaps of
 * 3 and 2, the arrow's 31.2 x 19.3 box from 12.1 over the baseline, and the
 * line centred on x. */
static int prompt(const char *key, float x, float y) {
    float keyWidth = width(key), left = x - 0.5f * (keyWidth + 3.0f + width("Press to") + 2.0f + 31.2f);
    float arrowX = left + keyWidth + 3.0f + width("Press to") + 2.0f;
    return drawCount == 4 && text(0, key, left, y + 3.5f, gold) &&
           text(1, "Press to", left + keyWidth + 3.0f, y + 3.5f, words) &&
           arrow(2, arrowX + 1, y + 3.5f - 12.1f + 1, black, 0.6f) && arrow(3, arrowX, y + 3.5f - 12.1f, white, 1) &&
           near(arrowX + 31.2f - x, x - left);
}
static int stock(const char *key) {
    char line[64];
    snprintf(line, sizeof(line), "Press [%s] to  ", key);
    return drawCount == 2 && drawn[0].kind == TEXT && !strcmp(drawn[0].text, line) && drawn[0].font == &menuFont &&
           drawn[1].kind == HANDLE && !strcmp(drawn[1].material, "hint_mantle");
}
static int alpha(int x, int y) { return image[(y * 120 + x) * 4 + 3]; }
static rectDef_t rect = { 0, 130, 40, 40, 2, 2 };
static void draw(void) { drawCount = 0; CG_DrawMantleHint(&rect, &menuFont, 0.21f, 6); }
/* Alive, in first person, with a ledge in reach. */
static void ledge(void) {
    memset(&cgState, 0, sizeof(cgState));
    cgState.predictedPlayerState.stats[0] = 100;
    cgState.predictedPlayerState.mantleState.flags = 8;
}
int main(void) {
    cgsState.media.mantleHint = "hint_mantle";
    ledge();
    /* Before the setting exists, the stock hint. */
    assert(!CG_MantlePromptActive());
    draw();
    assert(stock("Space") && !creates && !fontAsks);
    CG_MantlePromptInit();
    assert(registered == 1 && CG_MantlePromptActive());

    /* The arrow waits for the first ledge, then is built once. */
    cgState.predictedPlayerState.mantleState.flags = 0;
    draw();
    assert(drawCount == 0 && !creates);
    ledge();
    draw();
    assert(prompt("Space", 0, 130));
    assert(creates == 1 && createSlot == R_HUD_IMAGE_MANTLE_ARROW && createWidth == 120 && createHeight == 74);
    draw();
    assert(prompt("Space", 0, 130) && creates == 1);

    /* The arrow is white, two texels to a pixel of the capture's box: a band
     * over a hollow, thin at its left tip and thick on the right, a head
     * below its right end, cut along the head's base, smooth edges and a
     * clear border, and the area of the fitted shapes (541.6 pixels, from a
     * grid of 0.05 pixels) within 1.5%. */
    {
        int partial = 0, left = 0, right = 0;
        long sum = 0;
        for (int y = 0; y < 74; y++)
            for (int x = 0; x < 120; x++) {
                const byte *texel = &image[(y * 120 + x) * 4];
                int a = texel[3];
                assert(texel[0] == 255 && texel[1] == 255 && texel[2] == 255);
                if (x < 3 || x > 117 || y < 3 || y > 70) assert(!a);
                partial += a > 0 && a < 255;
                sum += a;
            }
        for (int y = 0; y < 74; y++) { left += alpha(20, y); right += alpha(86, y); }
        assert(alpha(53, 9) == 255 && !alpha(39, 40) && alpha(100, 51) == 255 && alpha(96, 28) == 255);
        assert(!alpha(108, 73) && partial > 200 && right > 4 * left && left > 0);
        assert(fabs(sum / 255.0 / 4.0 - 541.6) < 541.6 * 0.015);
    }

    /* The key reads as the stock hint names it, also two keys or the move-up
     * key alone; with neither bound nothing is drawn. */
    strcpy(jumpKey, "Space or Mouse 4");
    draw();
    assert(prompt("Space or Mouse 4", 0, 130));
    strcpy(jumpKey, ""); strcpy(moveUpKey, "Ctrl");
    draw();
    assert(prompt("Ctrl", 0, 130));
    strcpy(moveUpKey, "");
    draw();
    assert(drawCount == 0);
    strcpy(jumpKey, "Space");

    /* It centres on the item's rect, with its alignment. */
    horz = 7; vert = 1;
    {
        rectDef_t moved = { 10, 30, 40, 40, 7, 1 };
        drawCount = 0;
        assert(CG_DrawMantlePrompt(&moved, "Space") && prompt("Space", 10, 30));
    }
    horz = 2; vert = 2;

    /* The stock hint's guards hold for the prompt. */
    for (int gate = 0; gate < 6; gate++) {
        ledge();
        if (gate == 0) drawMantleHint.current.enabled = 0;
        if (gate == 1) cgState.renderingThirdPerson = 1;
        if (gate == 2) cgState.predictedPlayerState.pm_type = 6;
        if (gate == 3) cgState.predictedPlayerState.stats[0] = 0;
        if (gate == 4) cgState.predictedPlayerState.pm_flags = 4;
        if (gate == 5) cgsState.media.mantleHint = NULL;
        draw();
        assert(drawCount == 0);
        drawMantleHint.current.enabled = 1; cgsState.media.mantleHint = "hint_mantle";
    }
    ledge();

    /* Without the labels' lettering the stock hint stays and no arrow is
     * built; an arrow that cannot be made leaves the stock hint without a
     * try every frame, and the next level tries again. */
    CG_MantlePromptInit();
    creates = 0; fontReady = 0;
    draw();
    assert(stock("Space") && !creates);
    fontReady = 1; createFails = 1;
    draw();
    assert(stock("Space") && creates == 1);
    draw();
    assert(stock("Space") && creates == 1);
    CG_MantlePromptInit();
    createFails = 0;
    draw();
    assert(prompt("Space", 0, 130) && creates == 2);

    /* Switched off, the stock hint. */
    promptDvar.current.enabled = 0;
    assert(!CG_MantlePromptActive());
    draw();
    assert(stock("Space"));
    return 0;
}
'''

mutants = [
    [('#define MANTLE_PROMPT_SCALE 0.21f', '#define MANTLE_PROMPT_SCALE 0.24f')],
    [('#define MANTLE_PROMPT_STYLE 3', '#define MANTLE_PROMPT_STYLE 6')],
    [('#define MANTLE_PROMPT_BASELINE 3.5f', '#define MANTLE_PROMPT_BASELINE 0.0f')],
    [('#define MANTLE_PROMPT_KEY_GAP 3.0f', '#define MANTLE_PROMPT_KEY_GAP 4.0f')],
    [('#define MANTLE_PROMPT_ARROW_GAP 2.0f', '#define MANTLE_PROMPT_ARROW_GAP 3.2f')],
    [('#define MANTLE_PROMPT_ARROW_TOP -12.1f', '#define MANTLE_PROMPT_ARROW_TOP -7.1f')],
    [('#define MANTLE_PROMPT_ARROW_WIDTH 31.2f', '#define MANTLE_PROMPT_ARROW_WIDTH 40.0f')],
    [('#define MANTLE_PROMPT_ARROW_HEIGHT 19.3f', '#define MANTLE_PROMPT_ARROW_HEIGHT 31.2f')],
    [('#define MANTLE_PROMPT_SHADOW_OFFSET 1.0f', '#define MANTLE_PROMPT_SHADOW_OFFSET 0.0f')],
    [('#define MANTLE_PROMPT_SHADOW_ALPHA 0.6f', '#define MANTLE_PROMPT_SHADOW_ALPHA 1.0f')],
    [('#define MANTLE_ARROW_TEXELS 2', '#define MANTLE_ARROW_TEXELS 1')],
    [('#define MANTLE_ARROW_SAMPLES 4', '#define MANTLE_ARROW_SAMPLES 1')],
    [('s_mantlePromptKey = { 0.95f, 0.84f, 0.34f, 1.0f }', 's_mantlePromptKey = { 0.96f, 0.95f, 0.92f, 1.0f }')],
    [('s_mantlePromptArrow = { 1.0f, 1.0f, 1.0f, 1.0f }', 's_mantlePromptArrow = { 0.96f, 0.95f, 0.92f, 1.0f }')],
    [('s_mantleArrowOuter[3] = { 26.56f, 31.23f, 29.66f }', 's_mantleArrowOuter[3] = { 26.56f, 31.23f, 31.0f }')],
    [('s_mantleArrowInner[3] = { 19.62f, 35.85f, 27.5f }', 's_mantleArrowInner[3] = { 19.62f, 35.85f, 26.0f }')],
    [('{ 54.55f, 35.48f } }', '{ 50.0f, 31.0f } }')],
    [('    if (CG_MantleArrowSide(head[0], head[1], x, y) >= 0.0f)\n        return CG_MantleArrowSide',
      '    if (0)\n        return CG_MantleArrowSide')],
    [('    if (CG_MantleArrowSide(head[0], head[1], x, y) >= 0.0f)\n'
      '        return CG_MantleArrowSide(head[1], head[2], x, y) >= 0.0f && '
      'CG_MantleArrowSide(head[2], head[0], x, y) >= 0.0f;\n',
      '    if (CG_MantleArrowSide(head[0], head[1], x, y) >= 0.0f && CG_MantleArrowSide(head[1], head[2], x, y) >= 0.0f &&\n'
      '        CG_MantleArrowSide(head[2], head[0], x, y) >= 0.0f)\n        return 1;\n')],
    [('texel[0] = texel[1] = texel[2] = 255;', 'texel[0] = texel[1] = texel[2] = 0;')],
    [('texel[3] = (byte)(covered * 255 / (MANTLE_ARROW_SAMPLES * MANTLE_ARROW_SAMPLES));',
      'texel[3] = (byte)(covered * 255);')],
    [('R_CreateHudImageMaterial(R_HUD_IMAGE_MANTLE_ARROW, bgra', 'R_CreateHudImageMaterial(R_HUD_IMAGE_MANTLE_ARROW + 1, bgra')],
    [('    if (s_mantleArrowTried)\n        return s_mantleArrow;\n', '')],
    [('    s_mantleArrowTried = 1;\n    bgra = ', '    bgra = '),
     ('    free(bgra);\n    return s_mantleArrow;', '    free(bgra);\n    s_mantleArrowTried = s_mantleArrow != NULL;\n    return s_mantleArrow;')],
    [('    s_mantleArrowTried = 0;\n', '')],
    [('Dvar_RegisterBool_mac("cg_hudMantlePrompt", 1, 0x1001)', 'Dvar_RegisterBool_mac("cg_hudMantlePrompt", 0, 0x1001)')],
    [('return cg_hudMantlePrompt && cg_hudMantlePrompt->current.enabled;', 'return cg_hudMantlePrompt != 0;')],
    [('MaterialHandle arrow = font ? CG_MantleArrowMaterial() : NULL;', 'MaterialHandle arrow = CG_MantleArrowMaterial();')],
    [('return (float)CL_TextWidth(text, 0, font) * CL_NormalizedTextScale(font, MANTLE_PROMPT_SCALE);',
      'return (float)(int)((float)CL_TextWidth(text, 0, font) * CL_NormalizedTextScale(font, MANTLE_PROMPT_SCALE));')],
    [('static const char words[] = "Press to";', 'static const char words[] = "Press to ";')],
    [('x = rect->x - 0.5f * (keyWidth', 'x = -0.5f * (keyWidth')],
    [('wordsWidth + MANTLE_PROMPT_ARROW_GAP +\n                          MANTLE_PROMPT_ARROW_WIDTH);', 'wordsWidth);')],
    [('y = rect->y + MANTLE_PROMPT_BASELINE;', 'y = MANTLE_PROMPT_BASELINE;')],
    [('MANTLE_PROMPT_SCALE,\n                s_mantlePromptKey, MANTLE_PROMPT_STYLE);',
      'MANTLE_PROMPT_SCALE,\n                s_mantlePromptKey, 6);')],
    [('MANTLE_PROMPT_SCALE,\n                s_mantlePromptKey,', 'MANTLE_PROMPT_SCALE,\n                s_mantlePromptText,')],
    [('    x += keyWidth + MANTLE_PROMPT_KEY_GAP;\n', '    x += keyWidth;\n')],
    [('MANTLE_PROMPT_ARROW_HEIGHT, rect->horzAlign,\n                      rect->vertAlign, 0.0f',
      'MANTLE_PROMPT_ARROW_HEIGHT, 2,\n                      2, 0.0f')],
    [('    CL_DrawStretchPic(x + MANTLE_PROMPT_SHADOW_OFFSET', '    if (0) CL_DrawStretchPic(x + MANTLE_PROMPT_SHADOW_OFFSET')],
]
hint_mutants = [
    ('    if (CG_MantlePromptActive() && CG_DrawMantlePrompt(rect, binding))\n        return;\n', ''),
    ('CG_DrawMantlePrompt(rect, binding)', 'CG_DrawMantlePrompt(rect, "Space")'),
]

variants = [(prompt, hint)]
for pairs in mutants:
    variant = prompt
    for old, new in pairs:
        assert variant.count(old) == 1, old
        variant = variant.replace(old, new)
    variants.append((variant, hint))
for old, new in hint_mutants:
    assert hint.count(old) == 1, old
    variants.append((prompt, hint.replace(old, new)))

with tempfile.TemporaryDirectory(prefix='cod2-mantle-prompt-') as directory:
    path = Path(directory) / 'prompt.c'
    binary = Path(directory) / 'prompt'
    for index, (prompt_variant, hint_variant) in enumerate(variants):
        path.write_text(support + prompt_variant + hint_variant + checks)
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wno-unused-function', str(path), '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
print('PASS: while cg_hudMantlePrompt is on, the stock mantle hint hands its key to the WWII prompt behind the same '
      'guards: the key in gold, "Press to" in near white and a white curved arrow over a shadow, in the player labels\' '
      'lettering with capitals 7 units tall and their one-pixel shadow, spaced by the drawn advances rather than '
      'widths rounded to a unit, on one line centred on the hint\'s rect with its alignment; the arrow is built once '
      'per level as a smooth 120 x 74 image of the capture\'s shapes; without the lettering, without the arrow '
      f'image or switched off, the stock hint stays; {len(variants) - 1} failing regression mutants')
