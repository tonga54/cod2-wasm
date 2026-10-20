#!/usr/bin/env python3
"""Run the production WWII main menu against a recording UI: main_text's
buttons moved to the capture's column and back, their lettering and focus,
the logo, subtitle, version and key prompts over the retail background in
gray, and the gold cursor; the gray copy r_hud2d builds of a DXT1, DXT3 or
DXT5 image; the hooks that run them; and the layout's numbers against the
capture's, in Barlow's metrics."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(root / 'scripts'))
from barlow_metrics import Font  # noqa: E402

source = (root / 'src/PC/ui_mp/ui_wwii_menu.c').read_text()
header = (root / 'src/headers/PC/ui_mp/ui_wwii_menu.h').read_text()
shared = (root / 'src/PC/ui_mp/ui_shared_mp.c').read_text()
main = (root / 'src/PC/ui_mp/ui_main_mp.c').read_text()
hud2d = (root / 'src/PC/gfx_d3d/r_hud2d.c').read_text()
hud2d_header = (root / 'src/headers/PC/gfx_d3d/r_hud2d.h').read_text()
init = (root / 'src/PC/gfx_d3d/r_init.c').read_text()
scoretable = (root / 'src/PC/cgame_mp/cg_scoretable_mp.c').read_text()


def function(text, name):
    match = re.search(r'^((?:static )?[^\n;]*\b' + name + r'\([^;]*?\))\s*\{', text, re.M)
    assert match, name
    brace = text.index('{', match.end() - 1)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


# The menus load, reset, paint and take the cursor through the module.
assert shared.count('#include "headers/PC/ui_mp/ui_wwii_menu.h"\n') == 1
assert main.count('#include "headers/PC/ui_mp/ui_wwii_menu.h"\n') == 1
assert re.search(r'UI_ConfigureBrowserMenu\(menuList->menus\[i\]\);\n#endif\n        UI_WWIIMenu_AddMenu\(dc, menuList->menus\[i\]\);\n'
                 r'        UI_WWIISettings_AddMenu\(dc, menuList->menus\[i\]\);\n'
                 r'        if \(dc->menuCount > 0x7f\)', function(shared, 'UI_AddMenuList'))
assert function(shared, 'Menu_Setup').startswith('void Menu_Setup(displayContextDef_t *dc)\n{\n    UI_WWIIMenu_Reset(dc);\n')
text_paint = function(shared, 'Item_Text_Paint')
assert (text_paint.index("if (textPtr[0] == '@')") < text_paint.index("if (textPtr[0] == '\\0') {") <
        text_paint.index('if (UI_WWIIMenu_PaintText(dc, item, textPtr))\n        return;') <
        text_paint.index('Item_TextColor(dc, item, (vec4_t *)color);'))
item_paint = function(shared, 'Item_Paint')
hook = item_paint.index('if (UI_WWIIMenu_PaintItem(dc, item))\n        return;')
assert (item_paint.index('if (!((*(byte *)&((itemDef_t *)it)->window.dynamicFlags[0]) & 4)) {') <
        item_paint.index('window.background = CL_RegisterMaterialNoMip(dvarStr, 3);') < hook <
        item_paint.index('Window_Paint(dc, (void *)item,'))
assert re.search(r'            Item_Paint\(dc, item\);\n        \}\n    \}\n    UI_WWIIMenu_PaintMenu\(dc, menu\);\n',
                 function(shared, 'Menu_Paint'))
paint_all = function(shared, 'Menu_PaintAll')
assert paint_all.index('UI_WWIIMenu_Update(dc);') < paint_all.index('Menu_Paint(dc, (menuDef_t *)menu, 0);')
# Laid out, main_text takes the clicks a wide screen puts outside its 4:3
# rectangle, from itself and from the menus over it.
handle_key = function(shared, 'Menu_HandleKey')
assert handle_key.count('int inside = (cx >= rx && cx <= rx + rw && cy >= ry && cy <= ry + rh) || '
                        'UI_WWIIMenu_CoversScreen(menu);\n            if (!inside) {') == 1
assert handle_key.index('UI_WWIIMenu_CoversScreen(menu)') < handle_key.index('Menus_HandleOOBClick(dc, menu, key, down);')
assert function(shared, 'Menus_HandleOOBClick').count(
    '        if ((testX < rx || testX > rx + rw || testY < ry || testY > ry + rh) &&\n'
    '            !UI_WWIIMenu_CoversScreen((menuDef_t *)openMenu))\n            continue;\n') == 1
# The arrow keys step through the module's order, from before the first item
# or after the last when none has the cursor.
move_cursor = function(shared, 'Menu_MoveCursor')
assert move_cursor.count('    if (cursor < 0 || cursor >= count) cursor = direction > 0 ? -1 : count;\n') == 1
assert move_cursor.count('        cursor = UI_WWIIMenu_NextItem(menu, cursor, direction);\n') == 1
assert '% count' not in move_cursor
# Both UI_Init builds register the switch beside the other UI dvars.
assert main.count('    Dvar_RegisterBool_mac("cg_marks", 1, 0x1001);\n    UI_WWIIMenu_RegisterDvars();\n') == 2
assert re.search(r'    if \(!UI_WWIIMenu_DrawCursor\(uiInfo->uiDC\.cursorx, uiInfo->uiDC\.cursory\)\)\n'
                 r'        UI_DrawHandlePic\(\(float\)\(uiInfo->uiDC\.cursorx - 16\),', function(main, 'UI_Refresh'))
# The images have slots of their own, released with the fonts when the
# renderer shuts down, which the module learns from the generation.
slots = {name: int(value) for name, value in re.findall(r'^#define (R_HUD_IMAGE_\w+) (\d+)$', hud2d_header, re.M)}
names = re.findall(r'"([^"]+)"', re.search(r's_hudImageNames\[R_HUD_IMAGE_COUNT\] = \{(.*?)\};', hud2d, re.S).group(1))
assert len(names) == slots['R_HUD_IMAGE_COUNT'] and len(set(names)) == len(names)
for slot, name in (('MENU_BACKDROP', '*menu_backdrop'), ('MENU_SHADE', '*menu_shade'), ('MENU_PILL', '*menu_pill'),
                   ('MENU_CURSOR', '*menu_cursor')):
    assert names[slots['R_HUD_IMAGE_' + slot]] == name
assert function(hud2d, 'R_ShutdownHudImages').endswith('    s_hudImageGeneration++;\n}\n')
assert function(hud2d, 'R_GetHudImageGeneration') == 'int R_GetHudImageGeneration(void)\n{\n    return s_hudImageGeneration;\n}\n'
assert init.index('R_ShutdownFonts();') < init.index('R_ShutdownHudImages();\n    R_ShutdownImages();')
assert 'int R_GetHudImageGeneration(void);' in hud2d_header
# The stock fonts' metrics are the scoreboard's.
def font_table(text, name):
    body = re.search(name + r'\[\] = \{\n(.*?)\n\};', text, re.S)[1]
    return re.findall(r'\{ (\d+), ([\d.]+)f, ([\d.]+)f \}', body)


stock_fonts = font_table(scoretable, 's_tableFonts')
assert len(stock_fonts) == 5 and font_table(source, 's_wwiiMenuFonts') == stock_fonts

Regular, Bold, UiFont = Font('Regular'), Font('Bold'), Font('Medium')
UNIT = 2.25  # px a unit on the 1728 x 1080 capture
SEMI = 0.95


def evidence(text):
    """The layout's numbers against the capture's, as measured on it: the
    failures."""
    failures = []

    def define(name):
        match = re.search(r'#define WWII_' + name + r' \(?(-?[\d.]+)f?', text)
        return float(match[1]) if match else float('nan')

    def check(ok, what):
        if not ok:
            failures.append(what)

    def bearing(font, letter, cap):
        return font.ink[font.cmap[ord(letter)]][0] * cap / font.cap

    def end_bearing(font, letter, cap):
        glyph = font.cmap[ord(letter)]
        return (font.advance[glyph] - font.ink[glyph][2]) * cap / font.cap

    def advances(font, word, cap, tracking=0.0, width=1.0):
        return (sum(font.advance[font.cmap[ord(c)]] for c in word) * cap * width / font.cap +
                tracking * cap * (len(word) - 1))

    cap, focus_cap = define('MENU_CAP'), define('MENU_FOCUS_CAP')
    tracking, focus_tracking = define('MENU_TRACKING'), define('MENU_FOCUS_TRACKING')
    left, top, spacing = define('MENU_LEFT'), define('MENU_TOP'), define('MENU_SPACING')
    # Idle PLAY, SETTINGS and EXIT GAME are inked 52, 104 and 113 px wide from
    # 110 px, on baselines at 418, 480 and 540 px; capitals 13 px solid, 14
    # with their edges. Focused, PLAY is inked 65 px wide from 128 px, 17 px
    # tall.
    check(abs(cap - 6.0) < 1e-6 and abs(focus_cap - 7.6) < 1e-6, 'capitals')
    for word, px in (('PLAY', 52), ('SETTINGS', 104), ('EXIT GAME', 113)):
        check(abs(Regular.ink_width(word, tracking) * cap - px / UNIT) < 0.35, word + ' width')
        check(abs(left + bearing(Regular, word[0], cap) - 110 / UNIT) < 0.35, word + ' ink left')
    check(abs(Bold.ink_width('PLAY', focus_tracking) * focus_cap - 65 / UNIT) < 0.15, 'focused width')
    check(abs(left + define('MENU_FOCUS_INDENT') + bearing(Bold, 'P', focus_cap) - 128 / UNIT) < 0.2, 'indent')
    for row, px in enumerate((418, 480, 540)):
        check(abs(top + row * spacing - px / UNIT) < 0.3, 'baseline %d' % row)
    # The focus slides over 7 frames of the 30 fps capture and grows in 3.
    check(define('MENU_SLIDE_MS') == round(7 / 30 * 1000) and define('MENU_GROW_MS') == round(3 / 30 * 1000), 'timing')
    # CALL OF DUTY is inked from 106 to 472 px; MULTIPLAYER from 150 to 429 px,
    # 17 to 19 px tall on a baseline at 338 px. The UI font, Medium, is drawn at
    # 95% of Barlow's width.
    check(abs(define('LOGO_INK_LEFT') - 106 / UNIT) < 0.1, 'logo left')
    check(abs(define('LOGO_INK_WIDTH') - 367 / UNIT) < 0.3, 'logo width')
    title_cap, title_tracking = define('TITLE_CAP'), define('TITLE_TRACKING')
    check(abs(UiFont.ink_width('MULTIPLAYER', title_tracking, SEMI) * title_cap - 280 / UNIT) < 0.2, 'title width')
    check(abs(define('TITLE_BASELINE') - 338 / UNIT) < 0.2 and abs(title_cap - 8.0) < 1e-6, 'title baseline')
    pen = define('LOGO_INK_LEFT') + 0.5 * (define('LOGO_INK_WIDTH') - advances(UiFont, 'MULTIPLAYER', title_cap,
                                                                                title_tracking, SEMI))
    ink = pen + bearing(UiFont, 'M', title_cap) * SEMI
    check(abs(ink + 0.5 * UiFont.ink_width('MULTIPLAYER', title_tracking, SEMI) * title_cap - 290 / UNIT) < 0.4,
          'centred')
    # The version, 1.0.2082721.106.1-15, is inked to 88 px left of the right
    # edge, its digits down to row 122.
    check(abs(define('VERSION_RIGHT') + end_bearing(Regular, '5', define('VERSION_CAP')) - 88 / UNIT) < 0.2,
          'version right')
    check(abs(define('VERSION_BASELINE') - 123 / UNIT) < 0.3, 'version baseline')
    # The keycaps span rows 949 to 971, ENTER's from 101 to 171 px and ESC's
    # from 1485 to 1532; their keys 12 to 13 px tall, their bottoms at 966
    # px; Select is inked from 180 px with its bottom at 967; Exit Game is
    # inked to 1617 px.
    bottom, height = define('PROMPT_BOTTOM'), define('PROMPT_HEIGHT')
    check(abs(480 - bottom - 972 / UNIT) < 0.2 and abs(480 - bottom - height - 949 / UNIT) < 0.2, 'keycap rows')
    check(abs(define('PROMPT_LEFT') - 101 / UNIT) < 0.1, 'prompt left')
    key_cap, pad = define('PROMPT_KEY_CAP'), define('PROMPT_PAD')
    for key, px in (('ENTER', 71), ('ESC', 48)):
        check(abs(advances(Regular, key, key_cap) + 2 * pad - px / UNIT) < 0.1, key + ' keycap')
    check(12 / UNIT <= key_cap <= 13 / UNIT, 'key capitals')
    check(abs(480 - define('PROMPT_KEY_BASELINE') - 967 / UNIT) < 0.2, 'key baseline')
    check(abs(480 - define('PROMPT_BASELINE') - 968 / UNIT) < 0.2 and define('PROMPT_CAP') == 6.0, 'label baseline')
    enter = define('PROMPT_LEFT') + advances(Regular, 'ENTER', key_cap) + 2 * pad
    check(abs(enter + define('PROMPT_GAP') + bearing(Regular, 'S', 6.0) - 180 / UNIT) < 0.25, 'label gap')
    check(abs(define('PROMPT_RIGHT') + end_bearing(Regular, 'e', 6.0) - (1728 - 1618) / UNIT) < 0.2, 'prompt right')
    return failures


failures = evidence(source)
assert not failures, failures

slot_defines = ''.join(f'#define {name} {value}\n' for name, value in slots.items())


def advance_table(font, units, width=1.0):
    """The WWII font's advances: Barlow's, in units of the stock font whose
    capitals are units tall, rounded as r_font_wwii rounds them."""
    values = [int(font.advance[font.cmap[c]] * units * width / font.cap + 0.5) if c in font.cmap else 0
              for c in range(256)]
    return '{ ' + ', '.join(str(v) for v in values) + ' }'


caps = {int(height): float(cap) for height, cap, _ in stock_fonts}
fonts = (f'static const short regularAdvance[256] = {advance_table(Regular, caps[32])};\n'
         f'static const short boldAdvance[256] = {advance_table(Bold, caps[30])};\n'
         f'static const short oddAdvance[256] = {advance_table(Regular, 25)};\n' +
         ''.join(f'static const short uiAdvance{h}[256] = {advance_table(UiFont, caps[h], SEMI)};\n'
                 for h in (12, 16, 24, 32)))

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
typedef unsigned char byte;
typedef float vec_t;
typedef vec_t vec4_t[4];
typedef int qboolean;
typedef int Bool;
typedef struct Material { const char *name; } Material;
typedef Material *MaterialHandle;
typedef struct Glyph { unsigned short letter; signed char x0, y0; byte dx, pixelWidth, pixelHeight; } Glyph;
typedef struct Font_s { const char *name; int pixelHeight; const short *advance; Glyph h; } Font_s;
typedef Font_s *FontHandle;
typedef struct dvar_t { struct { int enabled; } current; } dvar_t;
typedef struct rectDef_s { float x, y, w, h; int horzAlign, vertAlign; } rectDef_t;
typedef rectDef_t UiRectangle;
typedef struct menuDef_t menuDef_t;
typedef struct Window { UiRectangle rect[4]; UiRectangle rectClient[4]; const char *name; int style;
    int dynamicFlags[4]; vec4_t foreColor; MaterialHandle background; } Window;
typedef struct itemDef_s { Window window; int type; float textalignx, textaligny; const char *text; menuDef_t *parent;
    const char *dvar; int dvarFlags; int enabled; UiRectangle textRect[1]; } itemDef_t;
struct menuDef_t { Window window; int itemCount; vec4_t disableColor; itemDef_t **items; int cursorItem[1]; };
typedef struct displayContextDef_t { int realTime; menuDef_t *menuStack[16]; int openMenuCount; int cursorx, cursory; }
    displayContextDef_t;
typedef struct sharedUiInfo_t { struct { FontHandle bigFont, smallFont, consoleFont, boldFont, textFont, extraBigFont; } assets; }
    sharedUiInfo_t;
''' + slot_defines + fonts + r'''
/* The WWII fonts at the stock fonts' pixel heights, their "H" boxes holding
 * the baseline's offset and a margin as theirs do; a font of a height no
 * stock font has, whose "H" is its capitals. */
static Font_s regularFont = { "regular", 32, regularAdvance, { 'H', 0, -27, 0, 0, 0 } };
static Font_s boldFont = { "bold", 30, boldAdvance, { 'H', 0, -24, 0, 0, 0 } };
static Font_s smallFont = { "small", 12, uiAdvance12, { 'H', 0, -10, 0, 0, 0 } };
static Font_s textFont = { "text", 16, uiAdvance16, { 'H', 0, -14, 0, 0, 0 } };
static Font_s bigFont = { "big", 24, uiAdvance24, { 'H', 0, -19, 0, 0, 0 } };
static Font_s extraBigFont = { "extraBig", 32, uiAdvance32, { 'H', 0, -27, 0, 0, 0 } };
static Font_s markFont = { "mark", 12, uiAdvance12, { 'H', 0, -10, 0, 0, 0 } };
static Font_s oddFont = { "odd", 40, oddAdvance, { 'H', 0, -25, 0, 0, 0 } };
static sharedUiInfo_t sharedUiInfo = { { &bigFont, &smallFont, NULL, &boldFont, &textFont, &extraBigFont } };

static float screenW = 1280.0f, screenH = 720.0f;
static float GetRealHeightFromVirtualHeight(float height) { return height * screenH / 480.0f; }
static void CalcScreenX(float *x, int horzAlign) { assert(horzAlign == 4); *x *= screenW / 640.0f; }
static void CalcScreenY(float *y, int vertAlign) { assert(vertAlign == 4); *y *= screenH / 480.0f; }
static int CL_TextWidth(const char *text, int maxChars, FontHandle font) {
    assert(maxChars == 1 && font && font->advance && font->advance[(unsigned char)*text]);
    return font->advance[(unsigned char)*text];
}
static float CL_NormalizedTextScale(FontHandle font, float scale) { return 48.0f * scale / (float)font->pixelHeight; }
static const Glyph *R_GetCharacterGlyph(FontHandle font, unsigned int letter) { assert(letter == 'H'); return &font->h; }
static float markScale;
static FontHandle UI_GetFontHandle(int fontEnum, float scale) { assert(fontEnum == 0); markScale = scale; return &markFont; }
static const char *UI_SafeTranslateString(const char *reference) {
    if (!strcmp(reference, "MENU_R_CIRCLE")) return "\xae";
    if (!strcmp(reference, "MENU_BACKTOGAME")) return "Back to Game";
    assert(0);
    return "";
}
static int I_stricmp(const char *s0, const char *s1) { return strcasecmp(s0, s1); }
static dvar_t wwiiDvar;
static int registrations, inGame;
static const char *versionText = "1.3";
static const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags) {
    assert(!strcmp(name, "ui_wwiiMainMenu") && value == 1 && flags == 0x1001);
    registrations++;
    wwiiDvar.current.enabled = value;
    return &wwiiDvar;
}
static Bool Dvar_GetBool(const char *name) { assert(!strcmp(name, "cl_ingame")); return inGame; }
static const char *Dvar_GetVariantString(const char *name) { assert(!strcmp(name, "shortversion")); return versionText; }
static int enableLookups;
static qboolean Item_EnableShowViaDvar(const itemDef_t *item, int flag) { assert(flag == 1); enableLookups++; return item->enabled; }
static int settingsCover;
static int UI_WWIISettings_Covers(const displayContextDef_t *dc) { assert(dc); return settingsCover; }
/* Item_SetScreenCoords for each item: the menu's origin, and its alignment
 * where the item has none. */
static int positionUpdates;
static void Menu_UpdatePosition(menuDef_t *menu) {
    positionUpdates++;
    for (int i = 0; i < menu->itemCount; i++) {
        itemDef_t *item = menu->items[i];
        UiRectangle r = item->window.rectClient[0];
        r.x += menu->window.rect[0].x;
        r.y += menu->window.rect[0].y;
        if (!r.horzAlign && !r.vertAlign) {
            r.horzAlign = menu->window.rect[0].horzAlign;
            r.vertAlign = menu->window.rect[0].vertAlign;
        }
        item->window.rect[0] = r;
    }
}
static Material hudMaterials[R_HUD_IMAGE_COUNT], grayMaterial = { "gray" };
static byte slotTexels[R_HUD_IMAGE_COUNT][64 * 64 * 4];
static int slotWidth[R_HUD_IMAGE_COUNT], slotHeight[R_HUD_IMAGE_COUNT], slotBuilds[R_HUD_IMAGE_COUNT];
static MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height) {
    assert(slot >= 0 && slot < R_HUD_IMAGE_COUNT && bgra && width * height * 4 <= (int)sizeof(slotTexels[0]));
    slotBuilds[slot]++;
    slotWidth[slot] = width;
    slotHeight[slot] = height;
    memcpy(slotTexels[slot], bgra, (size_t)width * height * 4);
    return &hudMaterials[slot];
}
static int grayBuilds, grayFails;
static MaterialHandle graySource;
static byte grayTone[256];
static MaterialHandle R_CreateHudGrayMaterial(int slot, MaterialHandle source, const byte *tone, float *aspect) {
    assert(slot == R_HUD_IMAGE_MENU_BACKDROP && source && tone && aspect);
    grayBuilds++;
    graySource = source;
    memcpy(grayTone, tone, sizeof(grayTone));
    *aspect = grayFails ? 0.0f : 2.0f;
    return grayFails ? NULL : &grayMaterial;
}
static int generation;
static int R_GetHudImageGeneration(void) { return generation; }
static int regularRegistrations, wwiiFontsOff;
static FontHandle regularHandle = &regularFont;
static FontHandle R_RegisterWwiiFont(const char *fontName) {
    assert(!strcmp(fontName, "$regularfont"));
    regularRegistrations++;
    return wwiiFontsOff ? NULL : regularHandle;
}
enum { TEXT = 1, PIC };
typedef struct { int kind; char text[64]; FontHandle font; float x, y, w, h, scale, s1, t1, s2, t2; vec4_t color;
    int horz, vert, style; MaterialHandle material; } Drawn;
static Drawn drawn[96];
static int drawCount;
static Drawn *record(int kind, float x, float y, int horzAlign, int vertAlign, const vec_t *color) {
    Drawn *d;
    assert(drawCount < 96);
    d = &drawn[drawCount++];
    memset(d, 0, sizeof(*d));
    d->kind = kind; d->x = x; d->y = y; d->horz = horzAlign; d->vert = vertAlign;
    memcpy(d->color, color, sizeof(d->color));
    return d;
}
static void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign,
                        float scale, const vec_t *color, int style) {
    Drawn *d = record(TEXT, x, y, horzAlign, vertAlign, color);
    assert(maxChars == 1 || maxChars == 0x7fffffff);
    snprintf(d->text, sizeof(d->text), "%.*s", maxChars, text);
    d->font = font; d->scale = scale; d->style = style;
}
static void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1,
                              float s2, float t2, const vec_t *color, MaterialHandle material) {
    Drawn *d = record(PIC, x, y, horzAlign, vertAlign, color);
    d->w = w; d->h = h; d->s1 = s1; d->t1 = t1; d->s2 = s2; d->t2 = t2; d->material = material;
}
''' + header

checks = r'''
static int near(float a, float b) { return fabsf(a - b) < 2e-3f; }
static int nearColor(const float *a, const float *b) {
    return near(a[0], b[0]) && near(a[1], b[1]) && near(a[2], b[2]) && near(a[3], b[3]);
}
static const float idle[4] = { 0.776f, 0.776f, 0.769f, 1.0f }, gold[4] = { 0.80f, 0.72f, 0.47f, 1.0f };
static const float titleColor[4] = { 0.851f, 0.843f, 0.827f, 1.0f }, labelColor[4] = { 0.812f, 0.812f, 0.812f, 1.0f };
static const float keyColor[4] = { 0.788f, 0.788f, 0.788f, 1.0f }, versionColor[4] = { 0.78f, 0.78f, 0.78f, 0.85f };
static const float white[4] = { 1, 1, 1, 1 }, markColor[4] = { 1, 1, 1, 0.6f }, shadeColor[4] = { 1, 1, 1, 0.5f };
/* The stock fonts' capitals and baselines above the pen, in their units. */
static void metrics(FontHandle font, float *cap, float *rise) {
    static const struct { int height; float cap, rise; } stock[] = {
        { 12, 7.0f, 2 }, { 16, 11.0f, 2 }, { 24, 13.12f, 5 }, { 30, 17.1f, 6 }, { 32, 21.0f, 5 } };
    for (int i = 0; i < 5; i++) {
        if (font->pixelHeight == stock[i].height) {
            *cap = stock[i].cap;
            *rise = stock[i].rise;
            return;
        }
    }
    *cap = (float)-font->h.y0;
    *rise = 0;
}
static float capUnits(FontHandle font) { float cap, rise; metrics(font, &cap, &rise); return cap; }
static float advance(FontHandle font, const char *text, float cap, float tracking) {
    float width = 0;
    for (int i = 0; text[i]; i++)
        width += font->advance[(unsigned char)text[i]] * cap / capUnits(font) + (text[i + 1] ? tracking * cap : 0);
    return width;
}
/* From draw first on: each letter of word but spaces, one draw each, its pen
 * after the previous letter's advance and tracking capitals, all half a font
 * unit right and the baseline's rise and half a unit down, as the renderer
 * draws. The next draw. */
static int word(int first, const char *text, FontHandle font, float x, float baseline, float cap, float tracking,
                const float *color, int horz, int vert) {
    float units, rise;
    metrics(font, &units, &rise);
    float unit = cap / units, scale = cap * font->pixelHeight / (48.0f * units), y = baseline + (rise + 0.5f) * unit;
    int d = first;
    x += 0.5f * unit;
    for (const char *c = text; *c; c++) {
        if (*c != ' ') {
            const Drawn *dr = &drawn[d++];
            if (d > drawCount || dr->kind != TEXT || dr->text[0] != *c || dr->text[1] || dr->font != font ||
                !near(dr->x, x) || !near(dr->y, y) || !near(dr->scale, scale) || dr->horz != horz ||
                dr->vert != vert || !nearColor(dr->color, color) || dr->style)
                return -1;
        }
        x += font->advance[(unsigned char)*c] * unit + tracking * cap;
    }
    return d;
}
static int line(int d, const char *text, FontHandle font, float x, float baseline, float cap, const float *color, int horz,
                int vert) {
    const Drawn *dr = &drawn[d];
    float units, rise;
    metrics(font, &units, &rise);
    float unit = cap / units;
    return d < drawCount && dr->kind == TEXT && !strcmp(dr->text, text) && dr->font == font &&
           near(dr->x, x + 0.5f * unit) && near(dr->y, baseline + (rise + 0.5f) * unit) &&
           near(dr->scale, cap * font->pixelHeight / (48.0f * units)) && dr->horz == horz && dr->vert == vert &&
           nearColor(dr->color, color) && !dr->style;
}
static int pic(int d, float x, float y, float w, float h, int horz, int vert, float s1, float t1, float s2, float t2,
               const float *color, MaterialHandle material) {
    const Drawn *dr = &drawn[d];
    return d < drawCount && dr->kind == PIC && near(dr->x, x) && near(dr->y, y) && near(dr->w, w) && near(dr->h, h) &&
           dr->horz == horz && dr->vert == vert && near(dr->s1, s1) && near(dr->t1, t1) && near(dr->s2, s2) &&
           near(dr->t2, t2) && nearColor(dr->color, color) && dr->material == material;
}

static itemDef_t pool[32];
static itemDef_t *mainList[32], *bgList[8], *otherList[2];
static menuDef_t mainText, bg, otherMain;
static Material background = { "background_american_w" }, otherBackground = { "background_british_w" };
static Material logo = { "logo_cod2" };
static displayContextDef_t uiDc, cgDc;
static int poolUsed;
static itemDef_t *add(menuDef_t *menu, itemDef_t **list, const char *text, int type, float x, float y, float w, float h) {
    itemDef_t *it = &pool[poolUsed++];
    memset(it, 0, sizeof(*it));
    it->window.rectClient[0] = (UiRectangle){ x, y, w, h, 0, 0 };
    it->window.dynamicFlags[0] = 4;
    it->window.foreColor[0] = it->window.foreColor[1] = it->window.foreColor[2] = 0.9f;
    it->window.foreColor[3] = 0.9f;
    it->type = type; it->text = text; it->parent = menu; it->textaligny = 20; it->enabled = 1;
    list[menu->itemCount++] = it;
    return it;
}
/* The browser's main_text in its file's order, and bg. */
static const struct { const char *text; int type; float x, y, w, h; int row; float note; } browser[] = {
    { "Fullscreen: Off", 1, 385, 310, 210, 20, 3, 0 }, { "Fullscreen: On", 1, 385, 310, 210, 20, 3, 0 },
    { "Fullscreen unavailable. Try again.", 0, 385, 336, 230, 12, 3, 11 },
    { "New client version available", 0, 385, 350, 230, 18, 5, -13 }, { "Update game", 1, 385, 372, 210, 20, 5, 0 },
    { "@MENU_BACKTOGAME", 1, 385, 190, 210, 20, 0, 0 }, { "@MENU_JOIN_GAME", 1, 385, 190, 210, 20, 0, 0 },
    { "@MENU_DISCONNECT", 1, 385, 220, 210, 20, 1, 0 }, { "@MENU_START_NEW_SERVER", 1, 385, 220, 210, 20, 1, 0 },
    { "@MENU_OPTIONS", 1, 385, 250, 210, 20, 2, 0 }, { "@MENU_QUIT", 1, 385, 280, 210, 20, 4, 0 },
    { "@MENU_AUTO_UPDATE", 1, 385, 430, 210, 20, 6, 0 },
    { "@MENU_GAME_EXPERIENCE_MAY_CHANGE", 1, 20, 390, 210, 20, -1, 0 } };
enum { BROWSER_ITEMS = sizeof(browser) / sizeof(browser[0]) };
static itemDef_t *bgImage, *bgGradient, *bgLogo, *bgMark, *bgVersion;
static void menus(void) {
    poolUsed = 0;
    memset(&mainText, 0, sizeof(mainText));
    memset(&bg, 0, sizeof(bg));
    memset(&otherMain, 0, sizeof(otherMain));
    mainText.window.name = "main_text"; mainText.items = mainList;
    mainText.window.rect[0] = (UiRectangle){ 0, 0, 640, 480, 0, 0 };
    mainText.window.dynamicFlags[0] = 4;
    mainText.disableColor[0] = 0.3f; mainText.disableColor[3] = 0.5f;
    bg.window.name = "bg"; bg.items = bgList; bg.window.dynamicFlags[0] = 4;
    otherMain.window.name = "main_text"; otherMain.items = otherList;
    for (int i = 0; i < BROWSER_ITEMS; i++)
        add(&mainText, mainList, browser[i].text, browser[i].type, browser[i].x, browser[i].y, browser[i].w, browser[i].h);
    add(&otherMain, otherList, "@MENU_QUIT", 1, 385, 280, 210, 20);
    bgImage = add(&bg, bgList, NULL, 0, -128, 0, 896, 480);
    bgImage->window.style = 6; bgImage->dvar = "ui_background"; bgImage->window.background = &background;
    bgGradient = add(&bg, bgList, NULL, 0, 0, 65, 416, 351);
    bgGradient->window.style = 2;
    bgLogo = add(&bg, bgList, NULL, 0, 340, 60, 288, 72);
    bgLogo->window.name = "background_cod2_logo"; bgLogo->window.style = 2; bgLogo->window.background = &logo;
    bgMark = add(&bg, bgList, "@MENU_R_CIRCLE", 1, 562, 116, 10, 10);
    bgMark->window.name = "background_version_display";
    bgVersion = add(&bg, bgList, NULL, 1, -10, -10, 0, 0);
    bgVersion->window.name = "background_version_display"; bgVersion->dvar = "shortversion";
    memset(&uiDc, 0, sizeof(uiDc));
    uiDc.menuStack[0] = &bg; uiDc.menuStack[1] = &mainText; uiDc.openMenuCount = 2;
    uiDc.realTime = 1000;
}
static void start(void) {
    UI_WWIIMenu_Reset(&uiDc);
    menus();
    UI_WWIIMenu_AddMenu(&uiDc, &bg);
    UI_WWIIMenu_AddMenu(&uiDc, &mainText);
    wwiiDvar.current.enabled = 1;
    positionUpdates = 0;
    UI_WWIIMenu_Update(&uiDc);
}
static float baseline(int row) { return WWII_MENU_TOP + row * WWII_MENU_SPACING; }
static int applied(void) {
    for (int i = 0; i < BROWSER_ITEMS; i++) {
        const itemDef_t *it = mainList[i];
        UiRectangle r = it->window.rectClient[0];
        if (browser[i].row < 0) {
            if (!near(r.x, browser[i].x) || !near(r.y, browser[i].y) || r.horzAlign || it->textaligny != 20) return 0;
            continue;
        }
        float base = baseline(browser[i].row) + browser[i].note;
        float above = browser[i].type ? WWII_MENU_HIT_ABOVE : WWII_MENU_NOTE_CAP + 2.0f;
        float h = browser[i].type ? WWII_MENU_HIT_HEIGHT : WWII_MENU_NOTE_CAP + 4.0f;
        if (!near(r.x, WWII_MENU_LEFT - WWII_MENU_HIT_PAD) || !near(r.y, base - above) || !near(r.w, WWII_MENU_HIT_WIDTH) ||
            !near(r.h, h) || r.horzAlign != 1 || r.vertAlign != 1 || !near(it->textalignx, WWII_MENU_HIT_PAD) ||
            !near(it->textaligny, above))
            return 0;
        if (!near(it->window.rect[0].x, r.x) || !near(it->window.rect[0].y, r.y) || it->window.rect[0].horzAlign != 1)
            return 0;
    }
    return 1;
}
static int retail(void) {
    for (int i = 0; i < BROWSER_ITEMS; i++) {
        const itemDef_t *it = mainList[i];
        UiRectangle r = it->window.rectClient[0];
        if (!near(r.x, browser[i].x) || !near(r.y, browser[i].y) || !near(r.w, browser[i].w) || !near(r.h, browser[i].h) ||
            r.horzAlign || r.vertAlign || it->textalignx != 0 || it->textaligny != 20 || it->window.rect[0].horzAlign ||
            !near(it->window.rect[0].x, browser[i].x))
            return 0;
    }
    return 1;
}
static float easeOut(float t) { t = t < 0 ? 0 : t > 1 ? 1 : t; return 1 - (1 - t) * (1 - t); }
/* Join Game at its row with the focus slid s and grown g of the way. */
static int button(itemDef_t *it, const char *upper, float s, float g, FontHandle font, const float *color) {
    float cap = WWII_MENU_CAP + (WWII_MENU_FOCUS_CAP - WWII_MENU_CAP) * g;
    float tracking = WWII_MENU_TRACKING + (WWII_MENU_FOCUS_TRACKING - WWII_MENU_TRACKING) * g;
    float x = it->window.rect[0].x + WWII_MENU_HIT_PAD + WWII_MENU_FOCUS_INDENT * s;
    float y = it->window.rect[0].y + WWII_MENU_HIT_ABOVE + 0.5f * (cap - WWII_MENU_CAP);
    return word(0, upper, font, x, y, cap, tracking, color, 1, 1) == drawCount;
}
static void lerp(float *out, const float *a, const float *b, float t) { for (int i = 0; i < 4; i++) out[i] = a[i] + (b[i] - a[i]) * t; }

int main(void) {
    int cases = 0;
    float color[4];

    UI_WWIIMenu_RegisterDvars();
    assert(registrations == 1);

    /* The buttons take the capture's rows in the retail column's order, Quit
     * under the others as Exit Game; the notes go under Fullscreen and over
     * Update game, the buttons they sit nearest; the item off the column
     * keeps its place. Switched off, everything is back where it was. */
    start();
    assert(applied() && positionUpdates == 1 && s_wwiiMenu.applied);
    UI_WWIIMenu_Update(&uiDc);
    assert(positionUpdates == 1);
    wwiiDvar.current.enabled = 0;
    UI_WWIIMenu_Update(&uiDc);
    assert(retail() && positionUpdates == 2 && !s_wwiiMenu.applied);
    wwiiDvar.current.enabled = 1;
    UI_WWIIMenu_Update(&uiDc);
    assert(applied() && positionUpdates == 3);
    cases += 3;
    /* Native main_text: Quit already lies under the other buttons. */
    {
        static const float tops[] = { 190, 220, 250, 280, 310, 340, 370, 430 };
        static const char *texts[] = { "a", "b", "c", "d", "e", "f", "@MENU_QUIT", "g" };
        UI_WWIIMenu_Reset(&uiDc);
        menus();
        mainText.itemCount = 0;
        for (int i = 0; i < 8; i++)
            add(&mainText, mainList, texts[i], 1, 385, tops[i], 210, 20);
        UI_WWIIMenu_AddMenu(&uiDc, &mainText);
        UI_WWIIMenu_Update(&uiDc);
        for (int i = 0; i < 8; i++)
            assert(near(mainList[i]->window.rectClient[0].y, baseline(i) - WWII_MENU_HIT_ABOVE));
        cases++;
    }

    /* Idle buttons in Barlow Regular, capitals 6 tall, in the capture's light
     * gray with its tracking; spaces are not drawn. */
    start();
    itemDef_t *join = mainList[6];
    drawCount = 0;
    assert(UI_WWIIMenu_PaintText(&uiDc, join, "Join Game") == 1);
    assert(button(join, "JOIN GAME", 0, 0, &regularFont, idle) && drawCount == 8);
    assert(regularRegistrations >= 1);
    cases++;
    /* Focused, it slides 8 to the right over 233 ms, easing out, and grows to
     * capitals 7.6 tall about the same middle, turning gold and Bold, within
     * 100 ms; frames longer than 100 ms count as 100. */
    join->window.dynamicFlags[0] |= 2;
    drawCount = 0;
    UI_WWIIMenu_PaintText(&uiDc, join, "Join Game");
    assert(button(join, "JOIN GAME", 0, 0, &regularFont, idle));
    uiDc.realTime += 50;
    drawCount = 0;
    UI_WWIIMenu_PaintText(&uiDc, join, "Join Game");
    lerp(color, idle, gold, easeOut(0.5f));
    assert(button(join, "JOIN GAME", easeOut(50 / 233.0f), easeOut(0.5f), &boldFont, color));
    uiDc.realTime += 250;
    drawCount = 0;
    UI_WWIIMenu_PaintText(&uiDc, join, "Join Game");
    assert(button(join, "JOIN GAME", easeOut(150 / 233.0f), 1, &boldFont, gold));
    uiDc.realTime += 100;
    drawCount = 0;
    UI_WWIIMenu_PaintText(&uiDc, join, "Join Game");
    assert(button(join, "JOIN GAME", 1, 1, &boldFont, gold));
    /* Losing the focus runs it back. */
    join->window.dynamicFlags[0] &= ~2;
    uiDc.realTime += 30;
    drawCount = 0;
    UI_WWIIMenu_PaintText(&uiDc, join, "Join Game");
    lerp(color, idle, gold, easeOut(0.7f));
    assert(button(join, "JOIN GAME", easeOut(1 - 30 / 233.0f), easeOut(0.7f), &boldFont, color));
    uiDc.realTime += 50;
    drawCount = 0;
    UI_WWIIMenu_PaintText(&uiDc, join, "Join Game");
    lerp(color, idle, gold, easeOut(0.2f));
    assert(button(join, "JOIN GAME", easeOut(1 - 80 / 233.0f), easeOut(0.2f), &regularFont, color));
    cases += 6;
    /* A button its dvar disables takes the menu's disabled color; letters
     * of Latin-1 are drawn in capitals too. */
    itemDef_t *options = mainList[9];
    options->dvarFlags = 1;
    options->enabled = 0;
    drawCount = 0;
    UI_WWIIMenu_PaintText(&uiDc, options, "Opci\xf3n \xf7\xff");
    assert(button(options, "OPCI\xd3N \xf7\xff", 0, 0, &regularFont, mainText.disableColor));
    options->enabled = 1;
    drawCount = 0;
    UI_WWIIMenu_PaintText(&uiDc, options, "Options");
    assert(button(options, "OPTIONS", 0, 0, &regularFont, idle));
    cases += 2;
    /* A note keeps its words, case and color in small Regular capitals. */
    itemDef_t *note = mainList[2];
    drawCount = 0;
    assert(UI_WWIIMenu_PaintText(&uiDc, note, "Fullscreen unavailable. Try again.") == 1);
    assert(drawCount == 1 && line(0, "Fullscreen unavailable. Try again.", &regularFont, WWII_MENU_LEFT,
                                  baseline(3) + 11, WWII_MENU_NOTE_CAP, note->window.foreColor, 1, 1));
    cases++;
    /* Off the column, in another menu, or switched off: the retail text. */
    assert(!UI_WWIIMenu_PaintText(&uiDc, mainList[12], "Game experience"));
    assert(!UI_WWIIMenu_PaintText(&uiDc, bgMark, "\xae"));
    wwiiDvar.current.enabled = 0;
    UI_WWIIMenu_Update(&uiDc);
    assert(!UI_WWIIMenu_PaintText(&uiDc, join, "Join Game"));
    wwiiDvar.current.enabled = 1;
    UI_WWIIMenu_Update(&uiDc);
    cases++;
    /* Without the WWII fonts the UI's font stands in. */
    wwiiFontsOff = 1;
    generation++;
    UI_WWIIMenu_Update(&uiDc);
    drawCount = 0;
    UI_WWIIMenu_PaintText(&uiDc, mainList[11], "Auto");
    assert(button(mainList[11], "AUTO", 0, 0, &textFont, idle));
    wwiiFontsOff = 0;
    /* A font of a height no stock font has is measured by its "H", on the
     * pen. */
    regularHandle = &oddFont;
    generation++;
    UI_WWIIMenu_Update(&uiDc);
    drawCount = 0;
    UI_WWIIMenu_PaintText(&uiDc, mainList[11], "Auto");
    assert(button(mainList[11], "AUTO", 0, 0, &oddFont, idle));
    regularHandle = &regularFont;
    generation++;
    UI_WWIIMenu_Update(&uiDc);
    cases += 2;

    /* bg's background is drawn in gray, over a black wash fading by 55% of
     * the width, and its other items give way, while main_text is open. */
    start();
    grayBuilds = 0;
    drawCount = 0;
    assert(UI_WWIIMenu_PaintItem(&uiDc, bgImage) == 1 && grayBuilds == 1 && graySource == &background);
    {
        float st[4];
        UI_WWIIMenu_BackdropRect(uiDc.realTime, 1280.0f / 720.0f, 896.0f / 480.0f, st);
        assert(drawCount == 2 && pic(0, 0, 0, 640, 480, 4, 4, st[0], st[1], st[2], st[3], white, &grayMaterial) &&
               pic(1, 0, 0, 352, 480, 4, 4, 0, 0, 1, 1, shadeColor, &hudMaterials[R_HUD_IMAGE_MENU_SHADE]));
        /* Drawn for the item's 896 by 480, not the image's 2 to 1. */
        assert(near((st[2] - st[0]) / (st[3] - st[1]), (1280.0f / 720.0f) / (896.0f / 480.0f)));
    }
    assert(UI_WWIIMenu_PaintItem(&uiDc, bgImage) == 1 && grayBuilds == 1);
    bgImage->window.background = &otherBackground;
    assert(UI_WWIIMenu_PaintItem(&uiDc, bgImage) == 1 && grayBuilds == 2 && graySource == &otherBackground);
    for (int i = 1; i < bg.itemCount; i++)
        assert(UI_WWIIMenu_PaintItem(&uiDc, bgList[i]) == 1);
    assert(!UI_WWIIMenu_PaintItem(&uiDc, join));
    cases += 3;
    /* With main_text closed or hidden the other menus keep bg's items. */
    uiDc.openMenuCount = 1;
    assert(UI_WWIIMenu_PaintItem(&uiDc, bgImage) == 1 && !UI_WWIIMenu_PaintItem(&uiDc, bgLogo));
    uiDc.openMenuCount = 2;
    mainText.window.dynamicFlags[0] = 0;
    assert(!UI_WWIIMenu_PaintItem(&uiDc, bgLogo));
    mainText.window.dynamicFlags[0] = 4;
    cases++;
    /* Create a Class and Start New Server paint their own pages over the
     * backdrop alone; other submenus keep bg's logo, bars and version. */
    {
        menuDef_t page = { .window.name = "create_a_class" };

        uiDc.menuStack[1] = &page;
        assert(UI_WWIIMenu_PaintItem(&uiDc, bgImage) == 1);
        for (int i = 1; i < bg.itemCount; i++)
            assert(UI_WWIIMenu_PaintItem(&uiDc, bgList[i]) == 1);
        page.window.name = "createserver";
        for (int i = 1; i < bg.itemCount; i++)
            assert(UI_WWIIMenu_PaintItem(&uiDc, bgList[i]) == 1);
        page.window.name = "options_look";
        assert(!UI_WWIIMenu_PaintItem(&uiDc, bgLogo));
        /* Laid out, Options' pages show the backdrop alone too. */
        settingsCover = 1;
        assert(UI_WWIIMenu_PaintItem(&uiDc, bgImage) == 1 && UI_WWIIMenu_PaintItem(&uiDc, bgLogo) == 1);
        settingsCover = 0;
        uiDc.menuStack[1] = &mainText;
        cases++;
    }
    /* The browser's install offer takes the right key-prompt place only where
     * nothing else does: the main menu itself, outside a match. */
    {
        menuDef_t page = { .window.name = "joinserver" };

        assert(UI_WWIIMenu_RightPromptFree());
        uiDc.menuStack[2] = &page;
        uiDc.openMenuCount = 3;
        assert(!UI_WWIIMenu_RightPromptFree());
        uiDc.openMenuCount = 2;
        settingsCover = 1;
        assert(!UI_WWIIMenu_RightPromptFree());
        settingsCover = 0;
        inGame = 1;
        assert(!UI_WWIIMenu_RightPromptFree());
        inGame = 0;
        mainText.window.dynamicFlags[0] = 0;
        assert(!UI_WWIIMenu_RightPromptFree());
        mainText.window.dynamicFlags[0] = 4;
        uiDc.openMenuCount = 1;
        assert(!UI_WWIIMenu_RightPromptFree());
        uiDc.openMenuCount = 2;
        assert(UI_WWIIMenu_RightPromptFree());
        cases++;
    }
    /* An image that cannot be made gray is left to the retail code, and not
     * read again until the background changes. */
    grayFails = 1;
    bgImage->window.background = &background;
    grayBuilds = 0;
    assert(!UI_WWIIMenu_PaintItem(&uiDc, bgImage) && !UI_WWIIMenu_PaintItem(&uiDc, bgImage) && grayBuilds == 1);
    grayFails = 0;
    cases++;
    /* Switched off, every item is the retail one. */
    wwiiDvar.current.enabled = 0;
    UI_WWIIMenu_Update(&uiDc);
    for (int i = 0; i < bg.itemCount; i++)
        assert(!UI_WWIIMenu_PaintItem(&uiDc, bgList[i]));
    assert(!UI_WWIIMenu_RightPromptFree());
    wwiiDvar.current.enabled = 1;
    UI_WWIIMenu_Update(&uiDc);
    cases++;

    /* The background shows a part of the image as wide as the screen, inside
     * it whatever the time or the screen's shape, zooming from 1.06 to 1.12
     * and drifting. */
    {
        static const float aspects[] = { 4.0f / 3, 16.0f / 10, 16.0f / 9, 21.0f / 9, 32.0f / 9 };
        float zoomMin = 9, zoomMax = 0, st[4];
        for (int a = 0; a < 5; a++) {
            for (int t = 0; t < 400000; t += 370) {
                UI_WWIIMenu_BackdropRect(t, aspects[a], 896.0f / 480.0f, st);
                assert(st[0] >= -1e-5f && st[1] >= -1e-5f && st[2] <= 1 + 1e-5f && st[3] <= 1 + 1e-5f);
                assert(fabsf((st[2] - st[0]) * (896.0f / 480.0f) / (st[3] - st[1]) - aspects[a]) < 1e-3f);
                float zoom = aspects[a] < 896.0f / 480.0f ? 1 / (st[3] - st[1]) : 1 / (st[2] - st[0]);
                zoomMin = zoom < zoomMin ? zoom : zoomMin;
                zoomMax = zoom > zoomMax ? zoom : zoomMax;
            }
        }
        assert(fabsf(zoomMin - 1.06f) < 0.002f && fabsf(zoomMax - 1.12f) < 0.002f);
        UI_WWIIMenu_BackdropRect(0, 16.0f / 9, 896.0f / 480.0f, st);
        assert(near(st[0] + st[2], 1) && near(st[3] - st[1], 1 / 1.09f));
        cases += 2;
    }
    /* Its gray: the luma's gamma 0.8, then 1.15 times the contrast about the
     * middle, clamped. */
    {
        byte tone[256];
        UI_WWIIMenu_Tone(tone);
        assert(tone[0] == 0 && tone[255] == 255 && tone[64] == 78 && tone[128] == 150 && tone[192] == 215);
        for (int i = 1; i < 256; i++)
            assert(tone[i] >= tone[i - 1]);
        assert(!memcmp(tone, grayTone, sizeof(tone)));
        cases++;
    }

    /* Over bg with main_text open: logo_cod2 as wide as the capture's
     * lettering and ending where its lockup ends, the retail registered mark
     * beside it, MULTIPLAYER centred under it in the UI's UiFont, the
     * version from bg's own item, and Enter's prompt; Escape's only in a
     * match. */
    start();
    for (int ingame = 0; ingame < 2; ingame++) {
        inGame = ingame;
        drawCount = 0;
        UI_WWIIMenu_PaintMenu(&uiDc, &bg);
        float w = WWII_LOGO_INK_WIDTH / (WWII_LOGO_INK_X1 - WWII_LOGO_INK_X0), h = w / 4;
        float x = WWII_LOGO_INK_LEFT - WWII_LOGO_INK_X0 * w, y = WWII_LOGO_INK_BOTTOM - WWII_LOGO_INK_Y1 * h;
        assert(pic(0, x, y, w, h, 1, 1, 0, 0, 1, 1, white, &logo));
        assert(drawn[1].kind == TEXT && !strcmp(drawn[1].text, "\xae") && drawn[1].font == &markFont &&
               near(drawn[1].x, x + 222.0f / 288 * w) && near(drawn[1].y, y + 56.0f / 72 * h) &&
               near(drawn[1].scale, 0.17f / 72 * h) && near(markScale, 0.17f / 72 * h) &&
               nearColor(drawn[1].color, markColor) && drawn[1].style == 3 && drawn[1].horz == 1 && drawn[1].vert == 1);
        /* At 720 lines capitals 8 tall are 12 px: the 24 px font's 13.12. */
        float title = advance(&bigFont, "MULTIPLAYER", WWII_TITLE_CAP, WWII_TITLE_TRACKING);
        int d = word(2, "MULTIPLAYER", &bigFont, WWII_LOGO_INK_LEFT + 0.5f * (WWII_LOGO_INK_WIDTH - title),
                     WWII_TITLE_BASELINE, WWII_TITLE_CAP, WWII_TITLE_TRACKING, titleColor, 1, 1);
        assert(d == 13);
        assert(line(d, "1.3", &regularFont, -WWII_VERSION_RIGHT - advance(&regularFont, "1.3", WWII_VERSION_CAP, 0),
                    WWII_VERSION_BASELINE, WWII_VERSION_CAP, versionColor, 3, 1));
        d++;
        /* Keycaps in three slices of the stadium image, the key in Regular
         * centred on it, the label after it. */
        const char *keys[2] = { "ENTER", "ESC" }, *labels[2] = { "Select", "Back to Game" };
        for (int p = 0; p <= ingame; p++) {
            float keycap = advance(&regularFont, keys[p], WWII_PROMPT_KEY_CAP, 0) + 2 * WWII_PROMPT_PAD;
            float label = advance(&regularFont, labels[p], WWII_PROMPT_CAP, 0);
            float left = p ? -WWII_PROMPT_RIGHT - (keycap + WWII_PROMPT_GAP + label) : WWII_PROMPT_LEFT;
            int horz = p ? 3 : 1;
            float top = -(WWII_PROMPT_BOTTOM + WWII_PROMPT_HEIGHT), r = 0.5f * WWII_PROMPT_HEIGHT;
            MaterialHandle pill = &hudMaterials[R_HUD_IMAGE_MENU_PILL];
            assert(pic(d, left, top, r, WWII_PROMPT_HEIGHT, horz, 3, 0, 0, 0.25f, 1, white, pill));
            assert(pic(d + 1, left + r, top, keycap - 2 * r, WWII_PROMPT_HEIGHT, horz, 3, 0.25f, 0, 0.75f, 1, white, pill));
            assert(pic(d + 2, left + keycap - r, top, r, WWII_PROMPT_HEIGHT, horz, 3, 0.75f, 0, 1, 1, white, pill));
            float key = advance(&regularFont, keys[p], WWII_PROMPT_KEY_CAP, 0);
            assert(line(d + 3, keys[p], &regularFont, left + 0.5f * (keycap - key), -WWII_PROMPT_KEY_BASELINE,
                        WWII_PROMPT_KEY_CAP, keyColor, horz, 3));
            assert(line(d + 4, labels[p], &regularFont, left + keycap + WWII_PROMPT_GAP, -WWII_PROMPT_BASELINE,
                        WWII_PROMPT_CAP, labelColor, horz, 3));
            d += 5;
        }
        assert(d == drawCount);
        cases++;
    }
    inGame = 0;
    /* At 1080 lines the title takes the 32 px size. */
    screenW = 1920; screenH = 1080;
    drawCount = 0;
    UI_WWIIMenu_PaintMenu(&uiDc, &bg);
    assert(drawn[2].font == &extraBigFont && drawn[17].font == &regularFont && !strcmp(drawn[17].text, "ENTER"));
    screenW = 1280; screenH = 720;
    cases++;
    /* Not over bg, main_text closed or hidden, or switched off: nothing. */
    drawCount = 0;
    UI_WWIIMenu_PaintMenu(&uiDc, &mainText);
    uiDc.openMenuCount = 1;
    UI_WWIIMenu_PaintMenu(&uiDc, &bg);
    uiDc.openMenuCount = 2;
    mainText.window.dynamicFlags[0] = 0;
    UI_WWIIMenu_PaintMenu(&uiDc, &bg);
    mainText.window.dynamicFlags[0] = 4;
    wwiiDvar.current.enabled = 0;
    UI_WWIIMenu_Update(&uiDc);
    UI_WWIIMenu_PaintMenu(&uiDc, &bg);
    assert(drawCount == 0);
    wwiiDvar.current.enabled = 1;
    UI_WWIIMenu_Update(&uiDc);
    cases++;

    /* The gold arrow, 17.5 units square in the screen's pixels, its tip on the
     * cursor; switched off, the retail cursor. */
    drawCount = 0;
    assert(UI_WWIIMenu_DrawCursor(320, 240) == 1);
    assert(drawCount == 1 && pic(0, 640 - 0.12f * 26.25f, 360 - 0.03f * 26.25f, 26.25f, 26.25f, 5, 5, 0, 0, 1, 1, white,
                                 &hudMaterials[R_HUD_IMAGE_MENU_CURSOR]));
    wwiiDvar.current.enabled = 0;
    UI_WWIIMenu_Update(&uiDc);
    drawCount = 0;
    assert(!UI_WWIIMenu_DrawCursor(320, 240) && drawCount == 0);
    wwiiDvar.current.enabled = 1;
    UI_WWIIMenu_Update(&uiDc);
    cases++;
    /* Laid out, main_text takes clicks over the whole screen; bg, cgame's
     * main_text and the retail layout keep their rectangles. */
    assert(UI_WWIIMenu_CoversScreen(&mainText) && !UI_WWIIMenu_CoversScreen(&bg) &&
           !UI_WWIIMenu_CoversScreen(&otherMain) && !UI_WWIIMenu_CoversScreen(NULL));
    wwiiDvar.current.enabled = 0;
    UI_WWIIMenu_Update(&uiDc);
    assert(!UI_WWIIMenu_CoversScreen(&mainText));
    wwiiDvar.current.enabled = 1;
    UI_WWIIMenu_Update(&uiDc);
    cases++;
    /* The arrow keys go down main_text's rows, Quit under Fullscreen, and
     * wrap: the column by its rows, then in the menu's order, then the item
     * off it. Switched off, and in other menus, they keep the menu's order. */
    {
        static const int order[] = { 5, 6, 7, 8, 9, 0, 1, 2, 10, 3, 4, 11, 12 };
        static const int hidden[] = { 1, 4, 5, 7, 11, 12 };
        static const int down[] = { 6, 8, 9, 0, 10, 6 }, retailDown[] = { 0, 6, 8, 9, 10, 0 };
        int cursor = -1;
        assert(s_wwiiMenu.applied && BROWSER_ITEMS == 13 && mainText.itemCount == BROWSER_ITEMS);
        for (int i = 0; i < BROWSER_ITEMS; i++)
            assert((cursor = UI_WWIIMenu_NextItem(&mainText, cursor, 1)) == order[i]);
        assert(UI_WWIIMenu_NextItem(&mainText, cursor, 1) == order[0]);
        cursor = BROWSER_ITEMS;
        for (int i = BROWSER_ITEMS - 1; i >= 0; i--)
            assert((cursor = UI_WWIIMenu_NextItem(&mainText, cursor, -1)) == order[i]);
        assert(UI_WWIIMenu_NextItem(&mainText, cursor, -1) == order[BROWSER_ITEMS - 1]);
        assert(UI_WWIIMenu_NextItem(&bg, 4, 1) == 0 && UI_WWIIMenu_NextItem(&bg, 0, -1) == 4 &&
               UI_WWIIMenu_NextItem(&otherMain, 0, 1) == 0);
        /* Outside a match: Back to Game, Disconnect, Fullscreen: On and the
         * updates hidden, the notes never taking the cursor. */
        for (int i = 0; i < 6; i++)
            mainList[hidden[i]]->window.dynamicFlags[0] = 0;
        mainText.cursorItem[0] = -1;
        for (int i = 0; i < 6; i++)
            assert(Menu_MoveCursor(&uiDc, &mainText, 1) == mainList[down[i]] && mainText.cursorItem[0] == down[i]);
        assert(Menu_MoveCursor(&uiDc, &mainText, -1) == mainList[10] && Menu_MoveCursor(&uiDc, &mainText, -1) == mainList[0]);
        mainText.cursorItem[0] = -1;
        assert(Menu_MoveCursor(&uiDc, &mainText, -1) == mainList[10]);
        wwiiDvar.current.enabled = 0;
        UI_WWIIMenu_Update(&uiDc);
        assert(UI_WWIIMenu_NextItem(&mainText, -1, 1) == 0 && UI_WWIIMenu_NextItem(&mainText, BROWSER_ITEMS, -1) == 12 &&
               UI_WWIIMenu_NextItem(&mainText, 12, 1) == 0 && UI_WWIIMenu_NextItem(&mainText, 9, 1) == 10);
        mainText.cursorItem[0] = -1;
        for (int i = 0; i < 6; i++)
            assert(Menu_MoveCursor(&uiDc, &mainText, 1) == mainList[retailDown[i]]);
        mainText.cursorItem[0] = -1;
        assert(Menu_MoveCursor(&uiDc, &mainText, -1) == mainList[10]);
        for (int i = 0; i < 6; i++)
            mainList[hidden[i]]->window.dynamicFlags[0] = 4;
        wwiiDvar.current.enabled = 1;
        UI_WWIIMenu_Update(&uiDc);
        cases += 3;
    }

    /* The images: the wash's ramp; a near-black keycap inside a light edge
     * with half-circle ends; the arrow, gold and lighter at its top. */
    {
        const byte *shade = slotTexels[R_HUD_IMAGE_MENU_SHADE];
        assert(slotWidth[R_HUD_IMAGE_MENU_SHADE] == 64 && slotHeight[R_HUD_IMAGE_MENU_SHADE] == 4);
        for (int x = 0; x < 64; x++) {
            int alpha = (int)(powf(1 - (x + 0.5f) / 64, 1.6f) * 255 + 0.5f);
            for (int y = 0; y < 4; y++) {
                const byte *t = shade + (y * 64 + x) * 4;
                assert(!t[0] && !t[1] && !t[2] && t[3] == alpha);
            }
        }
        const byte *pill = slotTexels[R_HUD_IMAGE_MENU_PILL];
        assert(slotWidth[R_HUD_IMAGE_MENU_PILL] == 64 && slotHeight[R_HUD_IMAGE_MENU_PILL] == 32);
        const byte *middle = pill + (16 * 64 + 32) * 4, *edge = pill + 32 * 4, *corner = pill;
        assert(middle[0] == 13 && middle[3] == 224 && edge[0] == 201 && edge[3] == 255 && corner[3] == 0);
        for (int y = 0; y < 32; y++)
            for (int x = 0; x < 64; x++)
                assert(!memcmp(pill + (y * 64 + x) * 4, pill + (y * 64 + 63 - x) * 4, 4) &&
                       !memcmp(pill + (y * 64 + x) * 4, pill + ((31 - y) * 64 + x) * 4, 4));
        const byte *cursor = slotTexels[R_HUD_IMAGE_MENU_CURSOR];
        assert(slotWidth[R_HUD_IMAGE_MENU_CURSOR] == 64 && slotHeight[R_HUD_IMAGE_MENU_CURSOR] == 64);
        /* The arrow as the pointer capture lights it: its upper face cream,
         * reaching the band's corner, the left face gold, the wing's lower
         * face darker than the foot's, the left edge's line darker than the
         * left face and darker down, a near-black rim under the wing and a
         * soft black shadow past the foot. */
#define TEXEL(x, y) (cursor + ((y) * 64 + (x)) * 4)
        const byte *upper = TEXEL(21, 15), *left = TEXEL(14, 24), *wing = TEXEL(31, 31), *foot = TEXEL(17, 40);
        const byte *line = TEXEL(8, 30), *rim = TEXEL(37, 35), *shadow = TEXEL(9, 58);
        assert(upper[3] == 255 && left[3] == 255 && wing[3] == 255 && foot[3] == 255 && line[3] == 255 && rim[3] == 255);
        assert(upper[2] > 220 && upper[2] > upper[1] && upper[1] > upper[0] && upper[0] > 150 && TEXEL(14, 18)[2] > 210);
        assert(left[2] < upper[2] - 30 && left[2] > left[1] && left[1] > left[0]);
        assert(wing[2] < foot[2] && foot[2] < left[2] && wing[2] > wing[1] && wing[1] > wing[0]);
        assert(line[2] < left[2] - 50 && TEXEL(8, 45)[2] < line[2]);
        assert(rim[2] < 60 && rim[2] > rim[0]);
        assert(shadow[3] > 0 && shadow[3] < 100 && !shadow[0] && !shadow[1] && !shadow[2]);
        assert(!TEXEL(60, 5)[3] && !TEXEL(5, 60)[3] && !TEXEL(46, 29)[3]);
#undef TEXEL
        cases += 3;
    }

    /* cgame's HUD menus have a context of their own: setting it up or
     * loading menus named the same there leaves the UI's alone, which its own
     * setup forgets, keeping the switch. A renderer restart rebuilds the
     * images, the gray background and the font. */
    start();
    UI_WWIIMenu_Reset(&cgDc);
    UI_WWIIMenu_AddMenu(&cgDc, &otherMain);
    assert(s_wwiiMenu.mainText == &mainText && s_wwiiMenu.applied && s_wwiiMenu.itemCount == 12);
    {
        drawCount = 0;
        UI_WWIIMenu_PaintMenu(&uiDc, &bg);
        UI_WWIIMenu_PaintItem(&uiDc, bgImage);
        int shades = slotBuilds[R_HUD_IMAGE_MENU_SHADE], fonts = regularRegistrations;
        drawCount = 0;
        UI_WWIIMenu_PaintMenu(&uiDc, &bg);
        UI_WWIIMenu_PaintItem(&uiDc, bgImage);
        assert(slotBuilds[R_HUD_IMAGE_MENU_SHADE] == shades && regularRegistrations == fonts);
        grayBuilds = 0;
        generation++;
        UI_WWIIMenu_Update(&uiDc);
        UI_WWIIMenu_PaintMenu(&uiDc, &bg);
        UI_WWIIMenu_PaintItem(&uiDc, bgImage);
        assert(slotBuilds[R_HUD_IMAGE_MENU_SHADE] == shades + 1 && slotBuilds[R_HUD_IMAGE_MENU_PILL] > 1 &&
               slotBuilds[R_HUD_IMAGE_MENU_CURSOR] > 1 && grayBuilds == 1 && regularRegistrations == fonts + 1);
    }
    UI_WWIIMenu_Reset(&uiDc);
    assert(!s_wwiiMenu.mainText && !s_wwiiMenu.bg && !s_wwiiMenu.applied && s_wwiiMenu.enabled == &wwiiDvar);
    UI_WWIIMenu_Update(&uiDc);
    assert(!UI_WWIIMenu_DrawCursor(1, 1));
    cases += 3;

    printf("%d\n", cases);
    return 0;
}
'''

mutants = [
    [('    if (quitTop >= 0.0f) {', '    if (0) {')],
    [('#define WWII_MENU_RETAIL_EXTRAS 350.0f', '#define WWII_MENU_RETAIL_EXTRAS 450.0f')],
    [('entry->rect.y > tops[row] ? WWII_MENU_NOTE_BELOW', 'entry->rect.y < tops[row] ? WWII_MENU_NOTE_BELOW')],
    [('item->window.rectClient[0] = enabled ? entry->wwiiRect : entry->rect;', 'item->window.rectClient[0] = entry->wwiiRect;')],
    [('item->textaligny = enabled ? entry->wwiiAlignY : entry->textaligny;', 'item->textaligny = entry->wwiiAlignY;')],
    [('    Menu_UpdatePosition(s_wwiiMenu.mainText);\n', '')],
    [('    return 1.0f - (1.0f - t) * (1.0f - t);', '    return t;')],
    [('#define WWII_MENU_SLIDE_MS 233.0f', '#define WWII_MENU_SLIDE_MS 200.0f')],
    [('#define WWII_MENU_GROW_MS 100.0f', '#define WWII_MENU_GROW_MS 150.0f')],
    [('elapsed = elapsed < 0.0f ? 0.0f : 100.0f;', 'elapsed = elapsed < 0.0f ? 0.0f : elapsed;')],
    [('grow >= 0.5f ? UI_WWIIMenu_Bold(cap)', 'grow >= 1.0f ? UI_WWIIMenu_Bold(cap)')],
    [('WWII_MENU_FOCUS_INDENT * slide', 'WWII_MENU_FOCUS_INDENT * grow')],
    [(' + 0.5f * (cap - WWII_MENU_CAP),', ',')],
    [('UI_WWIIMenu_Lerp(WWII_MENU_TRACKING, WWII_MENU_FOCUS_TRACKING, grow)', 'WWII_MENU_TRACKING')],
    [('    UI_WWIIMenu_Upper(text, upper, sizeof(upper));', '    snprintf(upper, sizeof(upper), "%s", text);')],
    [("(c >= 0xe0 && c <= 0xfe && c != 0xf7)", "(c >= 0xe0)")],
    [("        if (text[i] != ' ')\n", "        if (1)\n")],
    [('    if ((item->dvarFlags & 3) && !Item_EnableShowViaDvar(item, 1)) {', '    if (0) {')],
    [('WWII_MENU_NOTE_CAP, 0.0f, item->window.foreColor);', 'WWII_MENU_NOTE_CAP, 0.0f, s_wwiiIdle);')],
    [('    return UI_WWIIMenu_MainOpen(dc) || UI_WWIIMenu_PageOpen(dc) || UI_WWIISettings_Covers(dc);\n}',
      '    return 1;\n}')],
    [('!I_stricmp(menu->window.name, "create_a_class")', '0')],
    [('!I_stricmp(menu->window.name, "createserver")', '0')],
    [('    return s_wwiiMenu.applied && dc && UI_WWIIMenu_MainOpen(dc)', '    return dc && UI_WWIIMenu_MainOpen(dc)')],
    [('UI_WWIIMenu_MainOpen(dc) && !UI_WWIIMenu_PageOpen(dc) &&', 'UI_WWIIMenu_MainOpen(dc) &&')],
    [('           !UI_WWIISettings_Covers(dc) && !Dvar_GetBool("cl_ingame");', '           !Dvar_GetBool("cl_ingame");')],
    [('!UI_WWIISettings_Covers(dc) && !Dvar_GetBool("cl_ingame");', '!UI_WWIISettings_Covers(dc);')],
    [('    if (!s_wwiiMenu.backdrop)\n        return 0;', '    if (!s_wwiiMenu.backdrop)\n        return 1;')],
    [('    if (source != s_wwiiMenu.backdropSource) {', '    if (1) {')],
    [('aspect = rect->w > 0.0f && rect->h > 0.0f ? rect->w / rect->h : s_wwiiMenu.backdropAspect;',
      'aspect = s_wwiiMenu.backdropAspect;')],
    [('0.5f + 0.5f * (1.0f - w) * sinf', '0.5f + 0.6f * (1.0f - w) * sinf')],
    [('#define WWII_BACKDROP_ZOOM_SWING 0.03f', '#define WWII_BACKDROP_ZOOM_SWING 0.05f')],
    [('640.0f * WWII_SHADE_WIDTH', '640.0f')],
    [('#define WWII_SHADE_ALPHA 0.5f', '#define WWII_SHADE_ALPHA 0.8f')],
    [('#define WWII_BACKDROP_GAMMA 0.8f', '#define WWII_BACKDROP_GAMMA 1.0f')],
    [('#define WWII_BACKDROP_CONTRAST 1.15f', '#define WWII_BACKDROP_CONTRAST 1.0f')],
    [('WWII_ALIGN_RIGHT, WWII_ALIGN_TOP, WWII_VERSION_CAP', 'WWII_ALIGN_LEFT, WWII_ALIGN_TOP, WWII_VERSION_CAP')],
    [('    if (!Dvar_GetBool("cl_ingame"))\n        return;\n', '')],
    [('0.0f, 0.0f, 0.25f, 1.0f,', '0.0f, 0.0f, 0.5f, 1.0f,')],
    [('px - s_wwiiCursorShape[0][0] * size', 'px - 0.5f * size')],
    [('    float px = (float)x, py = (float)y, size;\n\n    if (!s_wwiiMenu.applied)\n        return 0;\n',
      '    float px = (float)x, py = (float)y, size;\n\n')],
    [('    if (s_wwiiMenu.dc && dc != s_wwiiMenu.dc)\n        return;\n    memset', '    memset')],
    [(' || (s_wwiiMenu.dc && dc != s_wwiiMenu.dc))\n', ')\n')],
    [('    if (s_wwiiMenu.generation != R_GetHudImageGeneration()) {', '    if (0) {')],
    [('0.5f * (WWII_LOGO_INK_WIDTH - width)', '0.0f * (WWII_LOGO_INK_WIDTH - width)')],
    [('scale = WWII_LOGO_MARK_SCALE * height;', 'scale = 0.17f;')],
    [('UI_WWIIMenu_CapUnits(fonts[i]) >= pixels', 'UI_WWIIMenu_CapUnits(fonts[i]) >= 1000.0f')],
    [('            *rise = s_wwiiMenuFonts[i].baseline;', '            *rise = 0.0f;')],
    [('{ 32, 21.0f, 5.0f },', '{ 32, 27.0f, 5.0f },')],
    [('    *cap = glyph && glyph->y0 < 0 ? (float)-glyph->y0 :', '    *cap = glyph && glyph->y0 < 0 ? (float)-glyph->y0 + 2.0f :')],
    [('    x += 0.5f * norm;\n', '')],
    [('    baseline += (rise + 0.5f) * norm;', '    baseline += 0.5f * norm;')],
    [(' || !(s_wwiiMenu.mainText->window.dynamicFlags[0] & WWII_WINDOW_VISIBLE)', '')],
    [('s_wwiiIdle = { 0.776f, 0.776f, 0.769f, 1.0f }', 's_wwiiIdle = { 0.9f, 0.9f, 0.9f, 1.0f }')],
    [('s_wwiiGold = { 0.80f, 0.72f, 0.47f, 1.0f }', 's_wwiiGold = { 1.0f, 0.8f, 0.3f, 1.0f }')],
    [('s_wwiiTitle = { 0.851f, 0.843f, 0.827f, 1.0f }', 's_wwiiTitle = { 1.0f, 1.0f, 1.0f, 1.0f }')],
    [('s_wwiiVersion = { 0.78f, 0.78f, 0.78f, 0.85f }', 's_wwiiVersion = { 0.25f, 0.25f, 0.25f, 1.0f }')],
    [('#define WWII_PILL_OUTLINE 2.0f', '#define WWII_PILL_OUTLINE 0.0f')],
    [('#define WWII_PILL_FILL_ALPHA 0.88f', '#define WWII_PILL_FILL_ALPHA 1.0f')],
    [('static const float s_wwiiCursorTip[3] = { 0.94f, 0.90f, 0.74f };',
      'static const float s_wwiiCursorTip[3] = { 0.72f, 0.65f, 0.45f };')],
    [('#define WWII_CURSOR_WING_RIM 3.5f', '#define WWII_CURSOR_WING_RIM 0.0f')],
    [('#define WWII_CURSOR_SHADOW_ALPHA 0.35f', '#define WWII_CURSOR_SHADOW_ALPHA 0.0f')],
    [('        return 1;\n    if (UI_WWIIMenu_Side(notch, foot, x, y)', '        return 2;\n    if (UI_WWIIMenu_Side(notch, foot, x, y)')],
    [('UI_WWIIMenu_Side(ridge, band, x, y) >= 0.0f && UI_WWIIMenu_Side(band, tip, x, y) >= 0.0f',
      'UI_WWIIMenu_Side(ridge, tip, x, y) >= 0.0f')],
    [('    if (x - tip[0] < WWII_CURSOR_LINE / (float)WWII_CURSOR_IMAGE) {', '    if (0) {')],
    [('#define WWII_CURSOR_SIZE 17.5f', '#define WWII_CURSOR_SIZE 32.0f')],
    [('            else if (item->dvar)\n                s_wwiiMenu.bgVersion = item;\n', '')],
    [('    return s_wwiiMenu.applied && menu && menu == s_wwiiMenu.mainText;', '    return menu && menu == s_wwiiMenu.mainText;')],
    [('    return s_wwiiMenu.applied && menu && menu == s_wwiiMenu.mainText;', '    return s_wwiiMenu.applied && menu != NULL;')],
    [('        return first != NULL;', '        return second != NULL;')],
    [('        return first->wwiiRect.y < second->wwiiRect.y;', '        return first->wwiiRect.y > second->wwiiRect.y;')],
    [('    return a < b;\n}', '    return a > b;\n}')],
    [('    if (!UI_WWIIMenu_LaidOut(menu))\n        return (cursor + direction + count) % count;', '')],
    [('    return next >= 0 ? next : wrap >= 0 ? wrap : cursor;', '    return next >= 0 ? next : cursor;')],
    [('    s_wwiiMenu.enabled = enabled;\n    s_wwiiMenu.generation', '    s_wwiiMenu.generation')],
    # The layout's numbers against the capture's.
    [('#define WWII_MENU_TRACKING 0.22f', '#define WWII_MENU_TRACKING 0.20f')],
    [('#define WWII_MENU_FOCUS_TRACKING 0.135f', '#define WWII_MENU_FOCUS_TRACKING 0.10f')],
    [('#define WWII_MENU_LEFT 48.2f', '#define WWII_MENU_LEFT 48.9f')],
    [('#define WWII_MENU_TOP 185.95f', '#define WWII_MENU_TOP 187.0f')],
    [('#define WWII_MENU_SPACING 27.1f', '#define WWII_MENU_SPACING 28.0f')],
    [('#define WWII_MENU_FOCUS_INDENT 8.0f', '#define WWII_MENU_FOCUS_INDENT 6.0f')],
    [('#define WWII_TITLE_TRACKING 0.711f', '#define WWII_TITLE_TRACKING 0.66f')],
    [('#define WWII_TITLE_BASELINE 150.2f', '#define WWII_TITLE_BASELINE 152.0f')],
    [('#define WWII_VERSION_RIGHT 38.5f', '#define WWII_VERSION_RIGHT 39.1f')],
    [('#define WWII_PROMPT_PAD 3.45f', '#define WWII_PROMPT_PAD 3.3f')],
    [('#define WWII_PROMPT_KEY_CAP 5.65f', '#define WWII_PROMPT_KEY_CAP 5.5f')],
    [('    FontHandle keyFont = UI_WWIIMenu_Regular(WWII_PROMPT_KEY_CAP);', '    FontHandle keyFont = UI_WWIIMenu_UiFont(WWII_PROMPT_KEY_CAP);')],
    [('#define WWII_PROMPT_BOTTOM 48.0f', '#define WWII_PROMPT_BOTTOM 46.0f')],
    [('#define WWII_PROMPT_RIGHT 48.4f', '#define WWII_PROMPT_RIGHT 49.1f')],
    [('#define WWII_PROMPT_GAP 3.1f', '#define WWII_PROMPT_GAP 3.4f')],
]

module = re.sub(r'^#include "[^"]+"\n', '', source, flags=re.M)
assert module.count('#include') == 2


# The production Menu_MoveCursor, where a visible button takes the cursor.
keys = r'''
static void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign) {
    (void)x; (void)y; (void)w; (void)h; (void)horzAlign; (void)vertAlign;
}
static qboolean Item_SetFocus(displayContextDef_t *dc, itemDef_t *item, float x, float y) {
    (void)dc; (void)x; (void)y;
    return item->type == 1 && (item->window.dynamicFlags[0] & 4);
}
static void Menu_SetCursorItem(menuDef_t *menu, int cursorItem) { menu->cursorItem[0] = cursorItem; }
static qboolean Menu_HandleMouseMove(displayContextDef_t *dc, menuDef_t *menu, float x, float y) {
    (void)dc; (void)menu; (void)x; (void)y;
    return 1;
}
''' + move_cursor


def program(text):
    return support + re.sub(r'^#include "[^"]+"\n', '', text, flags=re.M) + keys + checks


# The gray copy of a block-compressed image, from r_hud2d.c.
gray_body = ''.join(function(hud2d, name) for name in ('R_DecodeColorLuma', 'R_GrayBlockBytes', 'R_GetGrayImageSize',
                                                       'R_DecodeGrayImage', 'R_CreateHudGrayMaterial'))
gray_defines = ''.join(re.search(r'^#define ' + name + r' .*\n', hud2d, re.M)[0] for name in (
    'IWI_VERSION', 'IWI_FORMAT_DXT1', 'IWI_FORMAT_DXT3', 'IWI_FORMAT_DXT5', 'DXT_BLOCK_BYTES', 'DXT1_BLOCK_BYTES'))
gray_defines += re.search(r'^#define R_HUD_GRAY_MAX_SIZE .*\n', hud2d_header, re.M)[0]
gray_support = r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte;
typedef struct GfxImageFileHeader { char tag[3]; byte version, format, flags; short dimensions[3]; int fileSizeForPicmip[4]; }
    GfxImageFileHeader;
typedef struct GfxImage { const char *name; } GfxImage;
typedef struct { union { GfxImage *image; } u; } MaterialTextureDef;
typedef struct Material { unsigned short textureCount; MaterialTextureDef *textures; } Material;
typedef Material *MaterialHandle;
static struct { Material *defaultMaterial; } rgp;
''' + gray_defines + r'''
static byte fileData[1 << 16];
static int fileLength = -1, reads, frees;
static char readPath[128];
static int FS_ReadFile(const char *path, void **buffer) {
    reads++;
    snprintf(readPath, sizeof(readPath), "%s", path);
    if (fileLength < 0) { *buffer = NULL; return -1; }
    *buffer = malloc((size_t)fileLength + 1);
    memcpy(*buffer, fileData, (size_t)fileLength);
    return fileLength;
}
static void FS_FreeFile(void *buffer) { assert(buffer); frees++; free(buffer); }
static int Com_sprintf(char *dest, int size, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(dest, (size_t)size, fmt, args);
    va_end(args);
    return n < size ? n : -1;
}
static Material created = { 0, NULL };
static byte createdTexels[64 * 64 * 4];
static int createdSlot = -1, createdWidth, createdHeight;
static MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height) {
    createdSlot = slot; createdWidth = width; createdHeight = height;
    memcpy(createdTexels, bgra, (size_t)width * height * 4);
    return &created;
}
'''
gray_checks = r'''
/* An IWi file: the header, then mips smallest first, the top one last. */
static int iwi(byte *file, int version, int format, int w, int h, const byte *top, int topBytes, int lowerBytes) {
    GfxImageFileHeader header = { { 'I', 'W', 'i' }, (byte)version, (byte)format, 0, { (short)w, (short)h, 1 }, { 0 } };
    memcpy(file, &header, sizeof(header));
    memset(file + sizeof(header), 0xa5, (size_t)lowerBytes);
    memcpy(file + sizeof(header) + lowerBytes, top, (size_t)topBytes);
    return (int)sizeof(header) + lowerBytes + topBytes;
}
static void block(byte *b, int c0, int c1, unsigned int bits) {
    b[0] = (byte)c0; b[1] = (byte)(c0 >> 8); b[2] = (byte)c1; b[3] = (byte)(c1 >> 8);
    b[4] = (byte)bits; b[5] = (byte)(bits >> 8); b[6] = (byte)(bits >> 16); b[7] = (byte)(bits >> 24);
}
static int gray(const byte *bgra, int w, int x, int y) {
    const byte *t = bgra + (y * w + x) * 4;
    assert(t[0] == t[1] && t[1] == t[2] && t[3] == 255);
    return t[0];
}
int main(void) {
    byte identity[256], inverse[256], top[64], out[64 * 64 * 4];
    int cases = 0, w, h;
    for (int i = 0; i < 256; i++) { identity[i] = (byte)i; inverse[i] = (byte)(255 - i); }
    /* Indices 0, 1, 2, 3 repeated: white 0xffff before black 0x0000 makes a
     * DXT1 block of four grays; black first, three and black. */
    unsigned int ramp = 0xe4e4e4e4u;
    block(top, 0xffff, 0x0000, ramp);
    block(top + 8, 0x0000, 0xffff, ramp);
    int n = iwi(fileData, 5, 0x0b, 8, 4, top, 16, 8);
    assert(R_GetGrayImageSize(fileData, n, &w, &h) && w == 8 && h == 4);
    memset(out, 0, sizeof(out));
    assert(R_DecodeGrayImage(fileData, n, out, identity));
    for (int y = 0; y < 4; y++) {
        assert(gray(out, 8, 0, y) == 255 && gray(out, 8, 1, y) == 0 && gray(out, 8, 2, y) == 170 && gray(out, 8, 3, y) == 85);
        assert(gray(out, 8, 4, y) == 0 && gray(out, 8, 5, y) == 255 && gray(out, 8, 6, y) == 127 && gray(out, 8, 7, y) == 0);
    }
    assert(R_DecodeGrayImage(fileData, n, out, inverse) && gray(out, 8, 0, 0) == 0 && gray(out, 8, 2, 1) == 85);
    cases += 2;
    /* DXT3 and DXT5 keep their colors after 8 bytes of alpha, always four
     * grays; Rec. 709 weights for pure red, green and blue. */
    memset(top, 0x77, 8);
    block(top + 8, 0x0000, 0xffff, ramp);
    for (int format = 0x0c; format <= 0x0d; format++) {
        n = iwi(fileData, 5, format, 4, 4, top, 16, 0);
        assert(R_DecodeGrayImage(fileData, n, out, identity));
        assert(gray(out, 4, 0, 0) == 0 && gray(out, 4, 1, 0) == 255 && gray(out, 4, 2, 2) == 85 && gray(out, 4, 3, 3) == 170);
    }
    block(top, 0xf800, 0x07e0, 0);
    block(top + 8, 0x001f, 0x001f, 0);
    n = iwi(fileData, 5, 0x0b, 8, 4, top, 16, 0);
    assert(R_DecodeGrayImage(fileData, n, out, identity) && gray(out, 8, 0, 0) == 53 && gray(out, 8, 4, 0) == 18);
    block(top, 0x07e0, 0xf800, 0);
    n = iwi(fileData, 5, 0x0b, 4, 4, top, 8, 0);
    assert(R_DecodeGrayImage(fileData, n, out, identity) && gray(out, 4, 0, 0) == 182);
    cases += 2;
    /* A 6 by 6 image fills 2 by 2 blocks: only its texels are written. */
    for (int b = 0; b < 4; b++)
        block(top + 8 * b, b & 1 ? 0xffff : 0, b & 1 ? 0 : 0xffff, 0);
    n = iwi(fileData, 5, 0x0b, 6, 6, top, 32, 0);
    memset(out, 0xcd, sizeof(out));
    assert(R_DecodeGrayImage(fileData, n, out, identity));
    assert(gray(out, 6, 0, 0) == 0 && gray(out, 6, 5, 0) == 255 && gray(out, 6, 5, 5) == 255 && out[6 * 6 * 4] == 0xcd);
    cases++;
    /* Not an IWi of version 5, a format with no color blocks, too big, or
     * too short for its top mip: nothing. */
    block(top, 0xffff, 0, 0);
    n = iwi(fileData, 5, 0x0b, 4, 4, top, 8, 0);
    fileData[0] = 'J';
    assert(!R_GetGrayImageSize(fileData, n, &w, &h));
    n = iwi(fileData, 6, 0x0b, 4, 4, top, 8, 0);
    assert(!R_GetGrayImageSize(fileData, n, &w, &h));
    for (int format = 1; format <= 10; format++)
        assert(!R_GetGrayImageSize(fileData, iwi(fileData, 5, format, 4, 4, top, 8, 0), &w, &h));
    static const byte tall[8 * 1025];
    assert(R_GetGrayImageSize(fileData, iwi(fileData, 5, 0x0b, 2048, 4, tall, 8 * 512, 0), &w, &h) && w == 2048);
    assert(!R_GetGrayImageSize(fileData, iwi(fileData, 5, 0x0b, 4100, 4, tall, sizeof(tall), 0), &w, &h));
    assert(!R_GetGrayImageSize(fileData, iwi(fileData, 5, 0x0b, 4, 4100, tall, sizeof(tall), 0), &w, &h));
    assert(!R_GetGrayImageSize(fileData, iwi(fileData, 5, 0x0b, 0, 4, top, 8, 0), &w, &h));
    assert(!R_GetGrayImageSize(fileData, iwi(fileData, 5, 0x0b, 8, 4, top, 8, 0), &w, &h));
    assert(!R_GetGrayImageSize(NULL, 100, &w, &h) && !R_GetGrayImageSize(fileData, 20, &w, &h));
    n = iwi(fileData, 5, 0x0b, 4, 4, top, 8, 0);
    assert(R_GetGrayImageSize(fileData, n, &w, &h) && !R_DecodeGrayImage(fileData, n, out, NULL));
    cases += 2;

    /* A material's image read from images/<name>.iwi, made gray in its slot,
     * the file freed; its aspect its width over its height. */
    GfxImage image = { "background_american_w" };
    MaterialTextureDef texture = { { &image } };
    Material material = { 1, &texture }, fallback = { 1, &texture };
    float aspect = -1;
    block(top, 0xffff, 0x0000, ramp);
    block(top + 8, 0x0000, 0xffff, ramp);
    fileLength = iwi(fileData, 5, 0x0b, 8, 4, top, 16, 8);
    assert(R_CreateHudGrayMaterial(11, &material, inverse, &aspect) == &created && aspect == 2.0f);
    assert(!strcmp(readPath, "images/background_american_w.iwi") && reads == 1 && frees == 1);
    assert(createdSlot == 11 && createdWidth == 8 && createdHeight == 4 && gray(createdTexels, 8, 0, 0) == 0);
    /* One that is not block-compressed, missing, or the default material:
     * no material, aspect 0. */
    fileLength = iwi(fileData, 5, 0x06, 8, 4, top, 16, 8);
    createdSlot = -1;
    assert(!R_CreateHudGrayMaterial(11, &material, identity, &aspect) && aspect == 0 && createdSlot == -1);
    assert(reads == 2 && frees == 2);
    fileLength = -1;
    aspect = -1;
    assert(!R_CreateHudGrayMaterial(11, &material, identity, &aspect) && aspect == 0 && reads == 3 && frees == 2);
    rgp.defaultMaterial = &fallback;
    assert(!R_CreateHudGrayMaterial(11, &fallback, identity, &aspect) && reads == 3);
    material.textureCount = 0;
    assert(!R_CreateHudGrayMaterial(11, &material, identity, &aspect) && !R_CreateHudGrayMaterial(11, NULL, identity, &aspect));
    assert(reads == 3);
    cases += 2;
    printf("%d\n", cases);
    return 0;
}
'''
gray_mutants = [
    [('    if (!dxt1 || endpoints[0] > endpoints[1]) {', '    if (1) {')],
    [('    if (!dxt1 || endpoints[0] > endpoints[1]) {', '    if (endpoints[0] > endpoints[1]) {')],
    [('        palette[3] = 0;', '        palette[3] = 255;')],
    [('        palette[2] = (palette[0] + palette[1]) / 2;', '        palette[2] = palette[0];')],
    [('blockBytes == DXT1_BLOCK_BYTES ? block : block + 8,', 'block,')],
    [('    block = file + fileLen - blocksW * blocksH * blockBytes;', '    block = file + sizeof(*header);')],
    [('    if (fileLen - (int)sizeof(*header) < ((w + 3) / 4) * ((h + 3) / 4) * blockBytes)\n        return 0;\n', '')],
    [('w > R_HUD_GRAY_MAX_SIZE || h > R_HUD_GRAY_MAX_SIZE', 'w > R_HUD_GRAY_MAX_SIZE')],
    [('    if (header->version != IWI_VERSION)\n        return 0;\n', '')],
    [('texel[0] = texel[1] = texel[2] = tone[luma[i]];', 'texel[0] = texel[1] = texel[2] = luma[i];')],
    [('                if (x < w && y < h) {', '                if (y < h) {')],
    [('    if (format == IWI_FORMAT_DXT1)\n        return DXT1_BLOCK_BYTES;', '    if (format == IWI_FORMAT_DXT1)\n        return DXT_BLOCK_BYTES;')],
    [('    FS_FreeFile(file);\n    if (gray)', '    if (gray)')],
    [('    if (!material || material == rgp.defaultMaterial ||', '    if (!material ||')],
    [('        *aspect = (float)width / (float)height;', '        *aspect = 1.0f;')],
    [('"images/%s.iwi"', '"%s.iwi"')],
]


def run(directory, name, text, warnings, expect_pass):
    path = Path(directory) / f'{name}.c'
    binary = Path(directory) / name
    path.write_text(text)
    subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    *warnings, str(path), '-lm', '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert (result.returncode == 0) == expect_pass, (name, result.stdout[-500:], result.stderr[-3000:])
    return result


strict = ['-Wall', '-Werror', '-Wno-unused-function']
with tempfile.TemporaryDirectory(prefix='cod2-wwii-menu-') as directory:
    menu_cases = int(run(directory, 'menu', program(source), strict, True).stdout)
    for index, pairs in enumerate(mutants):
        variant = source
        for old, new in pairs:
            assert variant.count(old) == 1, old
            variant = variant.replace(old, new)
        if evidence(variant):
            continue
        run(directory, f'menu{index}', program(variant), ['-w'], False)
    gray_cases = int(run(directory, 'gray', gray_support + gray_body + gray_checks, strict, True).stdout)
    for index, pairs in enumerate(gray_mutants):
        variant = gray_body
        for old, new in pairs:
            assert variant.count(old) == 1, old
            variant = variant.replace(old, new)
        run(directory, f'gray{index}', gray_support + variant + gray_checks, ['-w'], False)

print('PASS: with ui_wwiiMainMenu on, main_text\'s buttons take the WWII capture\'s column, Quit last as its Exit Game '
      'and the notes beside their buttons, taking clicks over the whole screen and the arrow keys down its rows, and '
      'go back when it is off; they are '
      'lettered in Barlow Regular capitals '
      'with the capture\'s tracking and gray, the focused one sliding 8 units right over 233 ms and growing into gold '
      'Bold within 100 ms; over bg the retail background turns gray and drifts under a black wash, its other items give '
      'way to logo_cod2 sized to the capture\'s lettering, the registered mark, MULTIPLAYER, the version and the Enter '
      'and (in a match) Escape keycaps; the pointer capture\'s faceted gold arrow, with its rims and shadow, replaces the cursor; cgame\'s menus and renderer restarts leave '
      f'it consistent; {menu_cases} cases; gray copies of DXT1, DXT3 and DXT5 images, {gray_cases} cases; the layout\'s '
      f'numbers within the capture\'s measurements; {len(mutants) + len(gray_mutants)} failing regression mutants')
