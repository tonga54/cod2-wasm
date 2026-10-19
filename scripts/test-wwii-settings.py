#!/usr/bin/env python3
"""Run the production WWII settings layout against a recording UI: Options'
categories as a list and each page as rows of a name and its value, slider,
keys or field, with the focused one framed and explained at the right; the
keys, pointer and wheel driving the retail items through the menus' own key
handling; the hooks that run it; and the layout's numbers against the four
settings captures, in Barlow's metrics."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(root / 'scripts'))
from barlow_metrics import Font  # noqa: E402

source = (root / 'src/PC/ui_mp/ui_wwii_settings.c').read_text()
header = (root / 'src/headers/PC/ui_mp/ui_wwii_settings.h').read_text()
menu_header = (root / 'src/headers/PC/ui_mp/ui_wwii_menu.h').read_text()
menu_source = (root / 'src/PC/ui_mp/ui_wwii_menu.c').read_text()
shared = (root / 'src/PC/ui_mp/ui_shared_mp.c').read_text()
main = (root / 'src/PC/ui_mp/ui_main_mp.c').read_text()
hud2d = (root / 'src/PC/gfx_d3d/r_hud2d.c').read_text()
hud2d_header = (root / 'src/headers/PC/gfx_d3d/r_hud2d.h').read_text()


def function(text, name):
    match = re.search(r'^((?:static )?[^\n;]*\b' + name + r'\([^;]*?\))\s*\{', text, re.M)
    assert match, name
    brace = text.index('{', match.end() - 1)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


# The menus register, reset, paint and route keys and the pointer through the
# module; its items are neither painted nor hovered by the menus.
assert shared.count('#include "headers/PC/ui_mp/ui_wwii_settings.h"\n') == 1
assert main.count('#include "headers/PC/ui_mp/ui_wwii_settings.h"\n') == 1
assert menu_source.count('#include "headers/PC/ui_mp/ui_wwii_settings.h"\n') == 1
assert re.search(r'        UI_WWIIMenu_AddMenu\(dc, menuList->menus\[i\]\);\n        UI_WWIISettings_AddMenu\(dc, menuList->menus\[i\]\);\n',
                 function(shared, 'UI_AddMenuList'))
assert 'UI_WWIIMenu_Reset(dc);\n    UI_WWIISettings_Reset(dc);\n' in function(shared, 'Menu_Setup')
paint_all = function(shared, 'Menu_PaintAll')
assert paint_all.index('UI_WWIIMenu_Update(dc);\n    UI_WWIISettings_Update(dc);') < paint_all.index('Menu_Paint(dc, (menuDef_t *)menu, 0);')
item_paint = function(shared, 'Item_Paint')
assert item_paint.index('    if (UI_WWIISettings_Owns((menuDef_t *)parent))\n        return;\n') < item_paint.index('flags = ')
assert re.search(r'    UI_WWIISettings_Paint\(dc, menu\);\n', function(shared, 'Menu_Paint'))
mouse_move = function(shared, 'Menu_HandleMouseMove')
assert mouse_move.index('if (UI_WWIISettings_Owns(menu))\n        return 0;') < mouse_move.index('itemCapture')
assert function(shared, 'Menus_HandleOOBClick').count('        if (UI_WWIISettings_Owns((menuDef_t *)openMenu))\n            continue;\n') == 1
# The bind rows' keys and the lists' values come from one place for both
# layouts, and the fields edit as Enter on them does.
assert 'Item_BindText(item, bindText, sizeof(bindText));' in function(shared, 'Item_Bind_Paint')
assert re.search(r'        case 0x0c:\n        case 0x0d:\n            Item_List_Paint\(dc, item, Item_ValueText\(item\)\);\n',
                 item_paint)
edit_begin = function(shared, 'Item_EditField_Begin')
assert all(line in edit_begin for line in ('Item_SetCursorPos(item, 0);', 'g_editingField = 1;', 'g_editItem = item;',
                                           'Key_SetOverstrikeMode(1);'))
key_event = function(main, 'UI_KeyEvent_impl')
assert key_event.index('UI_WWIISettings_KeyEvent((displayContextDef_t *)uiInfo, menu, key, down)') < \
    key_event.index('Menu_HandleKey(uiInfo, menu, key, down);')
assert 'UI_WWIISettings_Covers(dc)' in function(menu_source, 'UI_WWIIMenu_PaintItem')
assert function(menu_source, 'UI_WWIIMenu_LayoutOn') == (
    'int UI_WWIIMenu_LayoutOn(void)\n{\n    return s_wwiiMenu.enabled && s_wwiiMenu.enabled->current.enabled;\n}\n')
# The arrow and the roundel have image slots of their own.
slots = {name: int(value) for name, value in re.findall(r'^#define (R_HUD_IMAGE_\w+) (\d+)$', hud2d_header, re.M)}
names = re.findall(r'"([^"]+)"', re.search(r's_hudImageNames\[R_HUD_IMAGE_COUNT\] = \{(.*?)\};', hud2d, re.S).group(1))
assert len(names) == slots['R_HUD_IMAGE_COUNT'] and len(set(names)) == len(names)
assert names[slots['R_HUD_IMAGE_MENU_ARROW']] == '*menu_arrow'
assert names[slots['R_HUD_IMAGE_MENU_ROUNDEL']] == '*menu_roundel'

Regular, Bold = Font('Regular'), Font('Bold')
UNIT = 1.2  # px a unit on the 1024 x 576 frames of the captures
TOP = 56    # the frames' first row in the 1024 x 665 captures


def evidence(text):
    """The layout's numbers against the captures', as measured on them: the
    failures."""
    failures = []

    def define(name):
        match = re.search(r'#define SET_' + name + r' \(?(-?[\d.]+)f?', text)
        return float(match[1]) if match else float('nan')

    def check(ok, what):
        if not ok:
            failures.append(what)

    def bearing(font, letter, cap):
        return font.ink[font.cmap[ord(letter)]][0] * cap / font.cap

    def y(px):
        return (px - TOP) / UNIT

    cap, tracking = define('LABEL_CAP'), define('LABEL_TRACKING')
    # Idle names inked 80 to 155 px wide; the condensed WWII lettering makes
    # STICK LAYOUT narrower than Barlow's.
    labels = (('STICK LAYOUT', 80), ('HORIZONTAL SENSITIVITY', 155), ('LOOK INVERSION', 100),
              ('VERTICAL SENSITIVITY', 138), ('CONTROLLER VIBRATION:', 150), ('AIM ASSIST ROTATION', 134),
              ('AIM ASSIST SLOWDOWN', 143), ('AUTO WEAPON SWITCH', 140), ('AUTO MANTLE', 87))
    errors = [Regular.ink_width(word, tracking) * cap - px / UNIT for word, px in labels]
    check((sum(e * e for e in errors) / len(errors)) ** 0.5 < 2.3 and max(abs(e) for e in errors) < 5.5, 'name widths')
    check(abs(Bold.ink_width('BUTTON LAYOUT', define('FOCUS_TRACKING')) * cap - 105 / UNIT) < 1.0, 'focused width')
    # Names inked from 64 to 65 px, the focused B from 75 px.
    for letter, px in (('S', 65.3), ('L', 65.2), ('H', 65.2), ('V', 64.3), ('A', 64.3)):
        check(abs(define('ROW_X') + bearing(Regular, letter, cap) - px / UNIT) < 0.6, 'name ink ' + letter)
    check(abs(define('ROW_X') + define('FOCUS_INDENT') + bearing(Bold, 'B', cap) - 75.2 / UNIT) < 0.4, 'indent')
    # Baselines at the rows' last inked pixels and the coverage of the next.
    for row, px in enumerate((184.25, 218.31, 253.03, 287.43, 323.0, 357.24, 392.02, 426.32, 461.05, 495.45, 531.02)):
        check(abs(define('ROW_TOP') + row * define('ROW_SPACING') - y(px)) < 0.45, 'baseline %d' % row)
    check(define('ROWS_SHOWN') == 11, 'rows shown')
    # The titles' ink: S from 54 px; SETTINGS // CONTROLS, GRAPHICS and AUDIO
    # 264, 259 and 215 px wide; capitals 13.4 px tall down to row 139.
    title_cap, title_tracking = define('TITLE_CAP'), define('TITLE_TRACKING')
    for word, px in (('SETTINGS // CONTROLS', 264), ('SETTINGS // GRAPHICS', 259), ('SETTINGS // AUDIO', 215)):
        check(abs(Bold.ink_width(word, title_tracking) * title_cap - px / UNIT) < 0.5, word)
    check(abs(define('TITLE_X') + bearing(Bold, 'S', title_cap) - 54.1 / UNIT) < 0.3, 'title ink')
    check(abs(define('TITLE_BASELINE') - y(139)) < 0.3 and abs(title_cap - 13.4 / UNIT) < 0.1, 'title baseline')
    # The frame's lines centred on rows 162.5 and 194.5 and columns 479.5 and
    # 319.3, about a pixel thick.
    line = define('BOX_LINE')
    top = define('ROW_TOP') - define('BOX_ABOVE')
    check(abs(top + 0.5 * line - y(162.5)) < 0.3, 'frame top')
    check(abs(top + define('BOX_HEIGHT') - 0.5 * line - y(194.5)) < 0.3, 'frame bottom')
    check(abs(define('BOX_RIGHT') - 0.5 * line - 479.5 / UNIT) < 0.3, 'frame right')
    check(abs(define('LIST_BOX_RIGHT') - 0.5 * line - 319.3 / UNIT) < 0.3, 'list frame right')
    check(0.7 / UNIT <= line <= 1.1 / UNIT, 'frame line')
    # The bar from 292.6 to 453.4 px, rows 172.74 to 185.17; the arrows from
    # 282.0 to 289.5 and 458.0 to 464.6 px; values centred on 372.6 px.
    check(abs(define('SLIDER_LEFT') - 292.6 / UNIT) < 0.3, 'bar left')
    check(abs(define('SLIDER_LEFT') + define('SLIDER_WIDTH') - 453.4 / UNIT) < 0.3, 'bar right')
    check(abs(define('ROW_TOP') - define('SLIDER_ABOVE') - y(172.74)) < 0.3, 'bar top')
    check(abs(define('ROW_TOP') - define('SLIDER_ABOVE') + define('SLIDER_HEIGHT') - y(185.17)) < 0.3, 'bar bottom')
    left_arrow = define('SLIDER_LEFT') - define('ARROW_GAP') - define('ARROW_WIDTH')
    right_arrow = define('SLIDER_LEFT') + define('SLIDER_WIDTH') + define('ARROW_GAP')
    check(abs(left_arrow - 282.0 / UNIT) < 0.8 and abs(left_arrow + define('ARROW_WIDTH') - 289.5 / UNIT) < 0.8,
          'left arrow')
    check(abs(right_arrow - 458.0 / UNIT) < 0.8 and abs(right_arrow + define('ARROW_WIDTH') - 464.6 / UNIT) < 0.8,
          'right arrow')
    check(abs(define('VALUE_CENTER') - 372.6 / UNIT) < 0.5, 'value centre')
    # Disabled and Enabled inked 47 and 43 px wide.
    for word, px in (('Disabled', 47), ('Enabled', 43)):
        check(abs(Regular.ink_width(word) * define('VALUE_CAP') - px / UNIT) < 3.0, word)
    # The explanation: BUTTON LAYOUT, BRIGHTNESS, GAME VOLUME and CONTROLS
    # inked 137, 103, 119 and 86 px wide from 494, 494, 493 and 493 px on row
    # 184's baseline, capitals 12.3 px tall; its text from 493 px, its lines
    # on rows 201.4 and 214.
    panel_cap, panel_tracking, panel_x = define('PANEL_TITLE_CAP'), define('PANEL_TITLE_TRACKING'), define('PANEL_X')
    for word, px, ink in (('BUTTON LAYOUT', 137, 494.2), ('BRIGHTNESS', 103, 494.2), ('GAME VOLUME', 119, 493.3),
                          ('CONTROLS', 86, 493.3)):
        check(abs(Bold.ink_width(word, panel_tracking) * panel_cap - px / UNIT) < 1.5, word + ' width')
        check(abs(panel_x + bearing(Bold, word[0], panel_cap) - ink / UNIT) < 0.6, word + ' ink')
    check(abs(panel_cap - 12.3 / UNIT) < 0.1, 'explanation title capitals')
    check(abs(panel_x + bearing(Regular, 'A', define('PANEL_TEXT_CAP')) - 493.3 / UNIT) < 0.6, 'explanation ink')
    check(abs(define('PANEL_TEXT_BASELINE') - y(201.4)) < 0.3, 'explanation baseline')
    check(abs(define('PANEL_LINE') - (214.0 - 201.4) / UNIT) < 0.3, 'explanation lines')
    # Brightness's roundels 67 px across, centred on 554.5 to 910 px, row 343.
    check(abs(define('ROUNDEL_SIZE') - 67 / UNIT) < 0.3 and abs(define('ROUNDEL_FIRST') - 554.0 / UNIT) < 0.6, 'roundels')
    check(abs(define('ROUNDEL_FIRST') + 4 * define('ROUNDEL_STEP') - 910.0 / UNIT) < 0.6, 'roundel spacing')
    check(abs(define('ROUNDEL_Y') - y(343)) < 0.3, 'roundel row')
    return failures


failures = evidence(source)
assert not failures, failures

slot_defines = ''.join(f'#define {name} {value}\n' for name, value in slots.items())

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
typedef vec_t vec4_t[4];
typedef int qboolean;
typedef struct Material { const char *name; } Material;
typedef Material *MaterialHandle;
typedef struct rectDef_s { float x, y, w, h; int horzAlign, vertAlign; } rectDef_t;
typedef rectDef_t UiRectangle;
typedef struct menuDef_t menuDef_t;
typedef struct editFieldDef_s { float minVal, maxVal, defVal, range; int maxChars, maxCharsGotoNext, maxPaintChars, paintOffset; }
    editFieldDef_t;
typedef struct multiDef_s { const char *dvarList[32]; const char *dvarStr[32]; float dvarValue[32]; int count; qboolean strDef; }
    multiDef_t;
typedef struct Window { UiRectangle rect[4]; UiRectangle rectClient[4]; const char *name; int staticFlags; int dynamicFlags[4]; }
    Window;
typedef struct itemDef_s { Window window; int type; const char *text; menuDef_t *parent; const char *mouseEnter;
    const char *action; const char *dvar; const char *dvarTest; const char *enableDvar; int dvarFlags; int cursorPos[4];
    union { editFieldDef_t *editField; multiDef_t *multi; const char *enumDvarName; } typeData; } itemDef_t;
struct menuDef_t { Window window; int itemCount; itemDef_t **items; const char *onESC; };
typedef struct displayContextDef_t { int realTime; int cursorx, cursory; menuDef_t *menuStack[16]; int openMenuCount; }
    displayContextDef_t;
typedef struct sharedUiInfo_t { struct { MaterialHandle whiteMaterial; } assets; } sharedUiInfo_t;
''' + slot_defines + menu_header + header + r'''
static Material whiteMaterial = { "white" }, hudMaterials[R_HUD_IMAGE_COUNT];
sharedUiInfo_t sharedUiInfo = { { &whiteMaterial } };

/* A 1280 x 720 screen: square units of 1.5 px from the edges, and 2 x 1.5 px
 * stretched over the whole of it. */
static float screenW = 1280.0f, screenH = 720.0f;
static float unitPx(void) { return screenH / 480.0f; }
static void CalcScreenX(float *x, int horzAlign) {
    if (horzAlign == 4) *x *= screenW / 640.0f;
    else if (horzAlign == 3) *x = *x * unitPx() + screenW;
    else { assert(horzAlign == 1); *x *= unitPx(); }
}
static void CalcScreenY(float *y, int vertAlign) {
    if (vertAlign == 4) *y *= screenH / 480.0f;
    else { assert(vertAlign == 1); *y *= unitPx(); }
}
static void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign) {
    assert(horzAlign == 1 && vertAlign == 1);
    *x *= unitPx(); *y *= unitPx(); *w *= unitPx(); *h *= unitPx();
}
static float GetRealWidthFromVirtualWidth(float width) { return width * unitPx(); }

/* Dvars by name. */
static struct { char name[48]; char value[64]; } dvars[64];
static int dvarCount, dvarSets;
static char lastSet[48], lastValue[64];
static const char *dvarGet(const char *name) {
    for (int i = 0; i < dvarCount; i++)
        if (!strcasecmp(dvars[i].name, name)) return dvars[i].value;
    return "";
}
static void dvarPut(const char *name, const char *value) {
    for (int i = 0; i < dvarCount; i++)
        if (!strcasecmp(dvars[i].name, name)) { snprintf(dvars[i].value, sizeof(dvars[i].value), "%s", value); return; }
    assert(dvarCount < 64);
    snprintf(dvars[dvarCount].name, sizeof(dvars[0].name), "%s", name);
    snprintf(dvars[dvarCount++].value, sizeof(dvars[0].value), "%s", value);
}
static const char *Dvar_GetVariantString(const char *name) { return dvarGet(name); }
static void Dvar_SetFromStringByName(const char *name, const char *value) {
    dvarSets++;
    snprintf(lastSet, sizeof(lastSet), "%s", name);
    snprintf(lastValue, sizeof(lastValue), "%s", value);
    dvarPut(name, value);
}
static char *va(const char *format, ...) {
    static char buffers[4][256];
    static int at;
    char *out = buffers[at++ & 3];
    va_list args;
    va_start(args, format);
    vsnprintf(out, 256, format, args);
    va_end(args);
    return out;
}
static int I_stricmp(const char *s0, const char *s1) { return strcasecmp(s0, s1); }
static const char *UI_SafeTranslateString(const char *reference) {
    static const char *const table[][2] = {
        { "MENU_LOOK", "Look" }, { "MENU_MOVE", "Move" }, { "MENU_GRAPHICS", "Graphics" },
        { "MENU_SET_DEFAULT_CONTROLS", "Set Default Controls" }, { "MENU_RESET_SYSTEM_DEFAULTS", "Optimal System Settings" },
        { "MENU_MULTIPLAYER", "Multiplayer" }, { "MENU_CONTROLS", "Controls" }, { "MENU_BACK", "Back" },
        { "MENU_INVERT_MOUSE", "Invert Mouse" }, { "MENU_LOOK_UP", "Look Up" }, { "MENU_MOUSE_SENSITIVITY", "Mouse Sensitivity" },
        { "MENU_VIDEO_MODE", "Video Mode" }, { "MENU_TEXTURE_FILTER", "Texture Filter" }, { "MENU_BRIGHTNESS", "Brightness" },
        { "MENU_TEXTURE_SETTINGS", "Texture Settings" }, { "MENU_APPLY", "Apply" },
        { "MENU_GRAPHICS_WARNING", "We recommend using \"Optimal System Settings.\"  Manually changing settings could result in poor performance." },
        { "MENU_BILINEAR", "Bilinear" }, { "MENU_TRILINEAR", "Trilinear" }, { "MENU_ANISOTROPIC", "Anisotropic" },
        { "MENU_GAME_OPTIONS", "Game Options" }, { "MENU_ENABLE_CROSSHAIR", "Enable Crosshair" }, { "MENU_THIS_WILL_RESTORE_ALL_SETTINGS", "Each setting will be restored" },
        { "MENU_TO_THEIR_DEFAULT_VALUE", "to its default value." }, { "MENU_ARE_YOU_SURE", "Are you sure?" },
        { "MENU_YES", "Yes" }, { "MENU_NO", "No" }, { "MENU_YOUR_CONTROLS_ARE_NOW_SET", "Your controls are now set" },
        { "MENU_TO_THEIR_ORIGINAL_DEFAULT_VALUES", "to their original default values." },
        { "EXE_KEYCHANGE", "Press ENTER or CLICK to change" },
        { "EXE_KEYWAIT", "Waiting for new key. Press ESCAPE to cancel or BACKSPACE to clear" },
    };
    for (int i = 0; i < (int)(sizeof(table) / sizeof(table[0])); i++)
        if (!strcmp(table[i][0], reference)) return table[i][1];
    fprintf(stderr, "untranslated %s\n", reference);
    assert(0);
    return "";
}
/* The menus' test of an item's showDvar or hideDvar, enableDvar or
 * disableDvar against the quoted values it lists. */
static int listed(const itemDef_t *item) {
    char value[80];
    snprintf(value, sizeof(value), "\"%s\"", dvarGet(item->dvarTest));
    return strstr(item->enableDvar, value) != NULL;
}
static int showTests;
static qboolean Item_EnableShowViaDvar(const itemDef_t *item, int flag) {
    if (!item->enableDvar || !item->dvarTest) return 1;
    showTests++;
    return (item->dvarFlags & flag) ? listed(item) : !listed(item);
}
static editFieldDef_t *Item_GetEditFieldDef(itemDef_t *item) { return item->typeData.editField; }
static const char *Item_ValueText(itemDef_t *item) {
    multiDef_t *multi = item->typeData.multi;
    if (item->type == 13) return dvarGet(item->dvar);
    for (int i = 0; multi && i < multi->count; i++)
        if (multi->strDef ? !strcasecmp(dvarGet(item->dvar), multi->dvarStr[i]) : atof(dvarGet(item->dvar)) == multi->dvarValue[i])
            return multi->dvarList[i];
    return "";
}
static void Item_BindText(itemDef_t *item, char *text, int size) { snprintf(text, (size_t)size, "KEY %s", item->dvar); }

/* The menus' key handling of the focused item: Yes/No toggles, a slider steps
 * 5% of its range, a list steps, Enter on a bind waits for a key. */
static int waiting, handleCalls, handleKey, handleFocused;
static itemDef_t *handleItem;
static qboolean Item_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key, qboolean down) {
    int right = key == 0x9d || key == 0xa3 || key == 0x0d, left = key == 0x9c || key == 0xa4;
    (void)dc;
    assert(down == 1);
    handleCalls++; handleItem = item; handleKey = key;
    handleFocused = (item->window.dynamicFlags[0] & 6) == 6;
    if (item->type == 11 && (right || left)) { dvarPut(item->dvar, atoi(dvarGet(item->dvar)) ? "0" : "1"); return 1; }
    if (item->type == 10 && (key == 0x9d || key == 0xa3 || left)) {
        editFieldDef_t *e = item->typeData.editField;
        float v = (float)atof(dvarGet(item->dvar)) + (left ? -1.0f : 1.0f) * (e->maxVal - e->minVal) * 0.05f;
        dvarPut(item->dvar, va("%g", (double)(v < e->minVal ? e->minVal : v > e->maxVal ? e->maxVal : v)));
        return 1;
    }
    /* A list steps; an enum of one mode stays. */
    if (item->type == 12 && (right || left)) return 1;
    if (item->type == 14 && key == 0x0d) { waiting = 1; return 1; }
    return 0;
}
static int bindCalls, bindKey, bindDown;
static itemDef_t *bindItem;
static qboolean Item_Bind_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key, qboolean down) {
    (void)dc;
    bindCalls++; bindItem = item; bindKey = key; bindDown = down;
    if (down) waiting = 0;
    return 1;
}
static qboolean Display_KeyBindPending(void) { return waiting; }
static itemDef_t *editItem;
static int fieldCalls, fieldKey, editBegins;
static void Item_EditField_Begin(itemDef_t *item) { editBegins++; editItem = item; item->cursorPos[0] = 0; }
static itemDef_t *Item_EditField_Current(void) { return editItem; }
static void Item_EditField_End(void) { editItem = NULL; }
static qboolean Item_TextField_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key) {
    (void)dc;
    fieldCalls++; fieldKey = key;
    assert(item == editItem);
    return key != 0x0d && key != 0x1b;
}
static struct { itemDef_t *item; char script[128]; } scripts[64];
static int scriptCount;
static void Item_RunScript(displayContextDef_t *dc, itemDef_t *item, const char *s) {
    (void)dc;
    assert(scriptCount < 64 && item && s);
    scripts[scriptCount].item = item;
    snprintf(scripts[scriptCount++].script, sizeof(scripts[0].script), "%s", s);
}
static char closed[64];
static int closes;
static void Menus_CloseByName(displayContextDef_t *dc, const char *p) {
    closes++;
    snprintf(closed, sizeof(closed), "%s", p);
    for (int i = 0; i < dc->openMenuCount; i++) {
        if (!strcasecmp(dc->menuStack[i]->window.name, p)) {
            memmove(&dc->menuStack[i], &dc->menuStack[i + 1], (size_t)(dc->openMenuCount - i - 1) * sizeof(dc->menuStack[0]));
            dc->openMenuCount--;
            break;
        }
    }
}
static int layoutOn = 1, generation;
int UI_WWIIMenu_LayoutOn(void) { return layoutOn; }
static int R_GetHudImageGeneration(void) { return generation; }
static byte slotTexels[R_HUD_IMAGE_COUNT][64 * 64 * 4];
static int slotWidth[R_HUD_IMAGE_COUNT], slotBuilds[R_HUD_IMAGE_COUNT];
static MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height) {
    assert(slot >= 0 && slot < R_HUD_IMAGE_COUNT && width * height * 4 <= (int)sizeof(slotTexels[0]));
    slotBuilds[slot]++;
    slotWidth[slot] = width;
    memcpy(slotTexels[slot], bgra, (size_t)width * height * 4);
    return &hudMaterials[slot];
}

/* The WWII lettering, measured as letters 0.6 capitals wide, Bold 0.65, a
 * space 0.25, tracking capitals between. */
enum { TEXT = 1, PIC };
typedef struct { int kind; char text[256]; int bold; float x, y, w, h, cap, tracking, s1, t1, s2, t2; vec4_t color; int horz, vert;
    MaterialHandle material; } Drawn;
static Drawn drawn[400];
static int drawCount;
float UI_WWIIMenu_Width(const char *text, int bold, float cap, float tracking) {
    float width = 0.0f;
    for (int i = 0; text[i]; i++)
        width += (text[i] == ' ' ? 0.25f : bold ? 0.65f : 0.6f) * cap + (text[i + 1] ? tracking * cap : 0.0f);
    return width;
}
void UI_WWIIMenu_Text(const char *text, int bold, float x, float baseline, int horzAlign, int vertAlign, float cap,
                      float tracking, const vec_t *color) {
    Drawn *d;
    assert(drawCount < 400);
    d = &drawn[drawCount++];
    memset(d, 0, sizeof(*d));
    d->kind = TEXT; snprintf(d->text, sizeof(d->text), "%s", text); d->bold = bold; d->x = x; d->y = baseline;
    d->horz = horzAlign; d->vert = vertAlign; d->cap = cap; d->tracking = tracking; memcpy(d->color, color, sizeof(d->color));
}
static void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2,
                              float t2, const vec_t *color, MaterialHandle material) {
    Drawn *d;
    assert(drawCount < 400);
    d = &drawn[drawCount++];
    memset(d, 0, sizeof(*d));
    d->kind = PIC; d->x = x; d->y = y; d->w = w; d->h = h; d->horz = horzAlign; d->vert = vertAlign;
    d->s1 = s1; d->t1 = t1; d->s2 = s2; d->t2 = t2; memcpy(d->color, color, sizeof(d->color)); d->material = material;
}
static const float idleColor[4] = { 0.776f, 0.776f, 0.769f, 1.0f };
const float *UI_WWIIMenu_Color(int color) { assert(color == WWII_COLOR_IDLE); return idleColor; }
static char promptText[160];
static int prompts;
void UI_WWIIMenu_Prompts(const char *const *keys, const char *const *labels, int count, const char *rightKey,
                         const char *rightLabel) {
    int at = 0;
    prompts++;
    promptText[0] = '\0';
    for (int i = 0; i < count; i++) at += snprintf(promptText + at, sizeof(promptText) - (size_t)at, "%s %s|", keys[i], labels[i]);
    if (rightKey) snprintf(promptText + at, sizeof(promptText) - (size_t)at, ">%s %s", rightKey, rightLabel);
}
'''

checks = r'''
static int near(float a, float b) { return fabsf(a - b) < 2e-3f; }
static int nearColor(const float *a, const float *b) {
    return near(a[0], b[0]) && near(a[1], b[1]) && near(a[2], b[2]) && near(a[3], b[3]);
}
static const float gold[4] = { 0.714f, 0.655f, 0.537f, 1.0f }, frame[4] = { 0.714f, 0.655f, 0.537f, 0.55f };
static const float valueGray[4] = { 0.47f, 0.47f, 0.47f, 1.0f }, textColor[4] = { 0.80f, 0.78f, 0.765f, 1.0f };
static const float noteColor[4] = { 0.62f, 0.61f, 0.59f, 1.0f }, bar[4] = { 0.835f, 0.81f, 0.745f, 1.0f };
static const float groove[4] = { 0, 0, 0, 0.45f }, arrowColor[4] = { 0.835f, 0.82f, 0.78f, 1.0f };
static const Drawn *findText(const char *text) {
    for (int i = 0; i < drawCount; i++) if (drawn[i].kind == TEXT && !strcmp(drawn[i].text, text)) return &drawn[i];
    return NULL;
}
static int countText(const char *text) {
    int n = 0;
    for (int i = 0; i < drawCount; i++) n += drawn[i].kind == TEXT && !strcmp(drawn[i].text, text);
    return n;
}
static int countPics(MaterialHandle material) {
    int n = 0;
    for (int i = 0; i < drawCount; i++) n += drawn[i].kind == PIC && drawn[i].material == material;
    return n;
}
static const Drawn *pic(MaterialHandle material, int nth) {
    for (int i = 0; i < drawCount; i++) if (drawn[i].kind == PIC && drawn[i].material == material && nth-- == 0) return &drawn[i];
    return NULL;
}
static int isPic(const Drawn *d, float x, float y, float w, float h, const float *color) {
    return d && d->kind == PIC && near(d->x, x) && near(d->y, y) && near(d->w, w) && near(d->h, h) && d->horz == 1 && d->vert == 1 &&
           nearColor(d->color, color);
}
/* Text drawn as s, in that font, place, size, tracking and color. */
static int text(const char *s, int bold, float x, float baseline, float cap, float tracking, const float *color) {
    for (int i = 0; i < drawCount; i++) {
        const Drawn *d = &drawn[i];
        if (d->kind == TEXT && !strcmp(d->text, s) && d->bold == bold && near(d->x, x) && near(d->y, baseline) &&
            d->horz == 1 && d->vert == 1 && near(d->cap, cap) && near(d->tracking, tracking) && nearColor(d->color, color))
            return 1;
    }
    return 0;
}
/* The explanation's lines in a size and color, joined by spaces; their
 * baselines from first, spacing apart. */
static const char *paragraph(float cap, const float *color, float *first, float spacing) {
    static char joined[640];
    float at = 0.0f;
    int lines = 0;
    joined[0] = '\0';
    for (int i = 0; i < drawCount; i++) {
        const Drawn *d = &drawn[i];
        if (d->kind != TEXT || !near(d->x, 410.5f) || !near(d->cap, cap) || !nearColor(d->color, color) || d->bold)
            continue;
        if (!lines) at = d->y;
        assert(near(d->y, at + spacing * (float)lines));
        if (lines++) strcat(joined, " ");
        strcat(joined, d->text);
    }
    if (first) *first = lines ? at : -1.0f;
    return joined;
}

static itemDef_t pool[160];
static itemDef_t *lists[9][40];
static menuDef_t menus[9];
static int poolUsed;
static editFieldDef_t sensitivityField = { 1, 30, 5, 0, 0, 0, 0, 0 }, gammaField = { 0.5f, 3, 1, 0, 0, 0, 0, 0 };
static editFieldDef_t nameField = { 0, 0, 0, 0, 31, 0, 20, 0 };
static multiDef_t yesNoList = { { "@MENU_YES", "@MENU_NO" }, { 0 }, { 1, 0 }, 2, 0 };
static multiDef_t filterList = { { "@MENU_BILINEAR", "@MENU_TRILINEAR", "@MENU_ANISOTROPIC" },
                                 { "bilinear", "trilinear", "anisotropic" }, { 0 }, 3, 1 };
enum { LIST, LOOK, GRAPHICS, MOVE, GAME, PAD, DEFAULTS, OTHER, VOICE };
static displayContextDef_t uiDc, cgDc;
static itemDef_t *add(int m, int type, const char *text, float x, float y, int decoration) {
    menuDef_t *menu = &menus[m];
    itemDef_t *it = &pool[poolUsed++];
    assert(poolUsed <= 160 && menu->itemCount < 40);
    memset(it, 0, sizeof(*it));
    it->window.rectClient[0] = (UiRectangle){ x, y, 1, 13, 0, 0 };
    it->window.dynamicFlags[0] = 4;
    it->window.staticFlags = decoration ? 0x100000 : 0;
    it->type = type; it->text = text; it->parent = menu;
    lists[m][menu->itemCount++] = it;
    return it;
}
static itemDef_t *control(int m, int type, const char *dvar, float y) {
    itemDef_t *it = add(m, type, " ", 50, y, 0);
    it->dvar = dvar;
    it->mouseEnter = "play \"mouse_over\"; show keyBindStatus";
    return it;
}
static void shows(itemDef_t *it, const char *dvar, int flags, const char *values) {
    it->dvarTest = dvar; it->dvarFlags = flags; it->enableDvar = values;
}
static itemDef_t *lookInvert, *lookUp, *lookSensitivity, *videoMode, *filter, *gammaHw, *gammaSoft, *textureButton, *applyButton;
static itemDef_t *warning, *crosshair, *nameEdit, *padEnable, *padFire, *padSprint, *padDefaults, *padBack, *yesOld, *yesMp, *noButton;
static itemDef_t *finishNote, *sureNote, *moveBinds[14];
static void build(void) {
    static const char *const names[] = { "options_menu", "options_look", "options_graphics", "options_move", "options_game",
                                         "browser_gamepad", "options_control_defaults", "main_text", "options_voice" };
    static const char *moveDvars[14] = { "+forward", "+back", "+moveleft", "+moveright", "+leanleft", "+leanright", "+gostand",
                                         "gocrouch", "goprone", "+moveup", "lowerstance", "raisestance", "togglecrouch", "toggleprone" };
    itemDef_t *it;
    poolUsed = 0;
    memset(menus, 0, sizeof(menus));
    for (int m = 0; m < 9; m++) {
        menus[m].window.name = names[m];
        menus[m].window.dynamicFlags[0] = 4;
        menus[m].items = lists[m];
    }
    menus[LIST].onESC = "open main_text; close options_menu;";
    add(LIST, 1, "@MENU_CONTROLS", 385, 130, 1);
    it = add(LIST, 1, "@MENU_LOOK", 400, 145, 0); it->action = "open options_look"; it->mouseEnter = "play \"mouse_over\"";
    it = add(LIST, 1, "@MENU_MOVE", 400, 160, 0); it->action = "open options_move"; it->mouseEnter = "play \"mouse_over\"";
    it = add(LIST, 1, "@MENU_MULTIPLAYER", 400, 205, 0); it->action = "open options_multi"; it->window.dynamicFlags[0] = 0;
    it = add(LIST, 1, "@MENU_SET_DEFAULT_CONTROLS", 400, 205, 0); it->action = "open options_control_defaults";
    it = add(LIST, 1, "@MENU_GRAPHICS", 400, 255, 0); it->action = "open options_graphics";
    it = add(LIST, 1, "@MENU_RESET_SYSTEM_DEFAULTS", 400, 315, 0); it->action = "open options_graphics_defaults";
    shows(it, "sv_running", 8, "\"1\" ");
    it = add(LIST, 1, "@MENU_BACK", 385, 360, 0); it->action = "close options_menu; open main_text";

    add(LOOK, 0, "@MENU_LOOK", 48, 64, 1);
    add(LOOK, 1, "@MENU_INVERT_MOUSE", 50, 120, 1);
    add(LOOK, 1, "@MENU_LOOK_UP", 50, 135, 1);
    add(LOOK, 1, "@MENU_MOUSE_SENSITIVITY", 50, 225, 1);
    it = add(LOOK, 17, "", 310, 225, 1); it->dvar = "sensitivity";
    lookInvert = control(LOOK, 11, "ui_mousePitch", 120); lookInvert->action = "play \"mouse_click\"; uiScript update ui_mousePitch";
    lookUp = control(LOOK, 14, "+lookup", 135); lookUp->action = "play \"mouse_click\";";
    lookSensitivity = control(LOOK, 10, "sensitivity", 225); lookSensitivity->typeData.editField = &sensitivityField;
    add(LOOK, 0, " ", 45, 370, 1);

    add(GRAPHICS, 0, "@MENU_GRAPHICS", 48, 64, 1);
    add(GRAPHICS, 1, "@MENU_VIDEO_MODE", 50, 120, 1);
    add(GRAPHICS, 1, "@MENU_TEXTURE_FILTER", 50, 165, 1);
    add(GRAPHICS, 1, "@MENU_BRIGHTNESS", 50, 210, 1);
    textureButton = add(GRAPHICS, 1, "@MENU_TEXTURE_SETTINGS", 50, 315, 0);
    textureButton->action = "close options_graphics; open options_graphics_texture";
    videoMode = control(GRAPHICS, 13, "ui_r_mode", 120); videoMode->action = "show graphicsapply";
    videoMode->typeData.enumDvarName = "r_mode";
    filter = control(GRAPHICS, 12, "ui_r_texturemode", 165); filter->typeData.multi = &filterList;
    gammaHw = control(GRAPHICS, 10, "r_gamma", 208); gammaHw->typeData.editField = &gammaField;
    shows(gammaHw, "r_ignorehwgamma", 4, "\"1\" ");
    gammaSoft = control(GRAPHICS, 10, "r_gamma", 208); gammaSoft->typeData.editField = &gammaField;
    shows(gammaSoft, "r_ignorehwgamma", 4, "\"0\" ");
    applyButton = add(GRAPHICS, 1, "@MENU_APPLY", 140, 340, 0); applyButton->action = "open vid_restart_popmenu";
    applyButton->window.dynamicFlags[0] = 0;
    warning = add(GRAPHICS, 0, "@MENU_GRAPHICS_WARNING", 60, 355, 1);

    add(MOVE, 0, "@MENU_MOVE", 48, 64, 1);
    for (int i = 0; i < 14; i++) moveBinds[i] = control(MOVE, 14, moveDvars[i], 120 + 15 * i);

    add(GAME, 0, "@MENU_GAME_OPTIONS", 48, 64, 1);
    add(GAME, 1, "@MENU_ENABLE_CROSSHAIR", 50, 135, 0);
    crosshair = control(GAME, 12, "cg_drawcrosshair", 135); crosshair->typeData.multi = &yesNoList;
    add(GAME, 0, "Gametag", 50, 210, 1);
    nameEdit = control(GAME, 4, "name", 210); nameEdit->typeData.editField = &nameField;

    add(PAD, 0, "Controller", 50, 64, 1);
    it = add(PAD, 0, "Controller connected", 50, 96, 1); shows(it, "ui_gamepadConnected", 4, "\"1\" ");
    it = add(PAD, 0, "Connect a controller and press a button", 50, 96, 1); shows(it, "ui_gamepadConnected", 4, "\"0\" ");
    it = add(PAD, 1, "Settings", 50, 120, 0); it->action = "setdvar ui_gamepadPage \"0\";";
    it = add(PAD, 1, "Buttons 1", 150, 120, 0); it->action = "setdvar ui_gamepadPage \"1\";";
    padEnable = add(PAD, 11, "Enable controller", 50, 158, 0); padEnable->dvar = "cl_gamepadEnable";
    shows(padEnable, "ui_gamepadPage", 4, "\"0\" ");
    padFire = add(PAD, 14, "Fire", 50, 158, 0); padFire->dvar = "+attack"; shows(padFire, "ui_gamepadPage", 4, "\"1\" ");
    padSprint = add(PAD, 14, "Sprint / hold breath", 50, 158, 0); padSprint->dvar = "+sprint";
    shows(padSprint, "ui_gamepadPage", 4, "\"2\" ");
    it = add(PAD, 0, "Left stick: move / navigate.", 50, 288, 1); shows(it, "ui_gamepadPage", 4, "\"0\" ");
    add(PAD, 0, "Esc cancels an assignment; Backspace clears it.", 50, 362, 1);
    padDefaults = add(PAD, 1, "Restore controller defaults", 50, 390, 0); padDefaults->action = "exec \"gamepad_defaults\";";
    padBack = add(PAD, 1, "Back", 300, 390, 0); padBack->action = "close browser_gamepad;";

    add(DEFAULTS, 0, "@MENU_SET_DEFAULT_CONTROLS", 48, 64, 1);
    add(DEFAULTS, 1, "@MENU_THIS_WILL_RESTORE_ALL_SETTINGS", 185, 160, 1);
    add(DEFAULTS, 1, "@MENU_TO_THEIR_DEFAULT_VALUE", 185, 175, 1);
    sureNote = add(DEFAULTS, 1, "@MENU_ARE_YOU_SURE", 185, 200, 1);
    yesOld = add(DEFAULTS, 1, "@MENU_YES", 117.5f, 225, 0); yesOld->action = "exec \"exec default.cfg\"";
    shows(yesOld, "ui_multiplayer", 4, "\"0\" ");
    yesMp = add(DEFAULTS, 1, "@MENU_YES", 117.5f, 225, 0); yesMp->action = "hide grpdefault; show grpfinish; exec \"exec default_mp.cfg\"";
    shows(yesMp, "ui_multiplayer", 8, "\"0\" ");
    noButton = add(DEFAULTS, 1, "@MENU_NO", 217.5f, 225, 0); noButton->action = "close options_control_defaults;";
    finishNote = add(DEFAULTS, 0, "@MENU_YOUR_CONTROLS_ARE_NOW_SET", 185, 230, 1); finishNote->window.dynamicFlags[0] = 0;
    it = add(DEFAULTS, 0, "@MENU_TO_THEIR_ORIGINAL_DEFAULT_VALUES", 185, 250, 1); it->window.dynamicFlags[0] = 0;

    add(OTHER, 1, "@MENU_OPTIONS", 385, 250, 0);

    it = add(VOICE, 1, "@MENU_YES", 117.5f, 225, 0); it->action = "exec yes";
    it = add(VOICE, 1, "@MENU_NO", 217.5f, 225, 0); it->action = "exec no";
}
static void reload(void) {
    UI_WWIISettings_Reset(&uiDc);
    build();
    dvarCount = 0;
    dvarPut("ui_mousePitch", "0"); dvarPut("sensitivity", "5"); dvarPut("sv_running", "0"); dvarPut("ui_r_mode", "1280x720");
    dvarPut("ui_r_texturemode", "trilinear"); dvarPut("r_ignorehwgamma", "0"); dvarPut("r_gamma", "1.75");
    dvarPut("ui_gamepadPage", "0"); dvarPut("ui_gamepadConnected", "1"); dvarPut("ui_multiplayer", "1"); dvarPut("name", "Chapu54"); dvarPut("cg_drawcrosshair", "1");
    dvarPut("cl_gamepadEnable", "1");
    for (int m = 0; m < 9; m++) UI_WWIISettings_AddMenu(m == OTHER ? &cgDc : &uiDc, &menus[m]);
    UI_WWIISettings_AddMenu(&uiDc, &menus[OTHER]);
    waiting = 0; editItem = NULL; scriptCount = 0; closes = 0;
}
static void open(int m) {
    uiDc.menuStack[uiDc.openMenuCount++] = &menus[m];
}
static void openOnly(int m) {
    uiDc.openMenuCount = 0;
    if (m != LIST) open(LIST);
    open(m);
}
static void paint(int m) {
    drawCount = 0;
    prompts = 0;
    UI_WWIISettings_Update(&uiDc);
    UI_WWIISettings_Paint(&uiDc, &menus[LIST]);
    if (m != LIST) UI_WWIISettings_Paint(&uiDc, &menus[m]);
}
static int key(int m, int k) {
    int handled = UI_WWIISettings_KeyEvent(&uiDc, &menus[m], k, 1);
    UI_WWIISettings_KeyEvent(&uiDc, &menus[m], k, 0);
    return handled;
}
static float baseline(int shown) { return SET_ROW_TOP + SET_ROW_SPACING * (float)shown; }
/* The pointer over a unit of the left-aligned layout, in the 640 x 480
 * units the pointer moves in. */
static void point(float x, float y) { uiDc.cursorx = (int)lroundf(x * 1.5f / 2.0f); uiDc.cursory = (int)lroundf(y); }
static int lastScript(const char *script, const itemDef_t *item) {
    return scriptCount > 0 && !strcmp(scripts[scriptCount - 1].script, script) && scripts[scriptCount - 1].item == item;
}

int main(void) {
    int cases = 0;
    char expected[64];

    /* Options' buttons make the list in their file's order, the hidden and
     * the Back button out of it; each page's rows pair a control with the
     * name on its row, in row order, tab by tab on the controller page; two
     * controls one of which a dvar shows share a row; the heading, notes and
     * a slider's value field are found. Menus of another context, or not
     * Options', are not laid out. */
    reload();
    {
        SettingsPage *list = Settings_Page(&menus[LIST]), *look = Settings_Page(&menus[LOOK]);
        SettingsPage *graphics = Settings_Page(&menus[GRAPHICS]), *pad = Settings_Page(&menus[PAD]);
        SettingsPage *defaults = Settings_Page(&menus[DEFAULTS]);
        int rows[48], count;
        /* Multiplayer and Set Default Controls share a place; the one shown
         * takes the row. */
        assert(list && list->list && list->rowCount == 5 && list->back == lists[LIST][7]);
        count = Settings_Visible(list, rows);
        assert(count == 5 && list->rows[rows[0]].controls[0]->text == lists[LIST][1]->text);
        assert(Settings_Control(&list->rows[rows[2]]) == lists[LIST][4] && list->rows[rows[2]].controls[0] == lists[LIST][3]);
        dvarPut("sv_running", "1");
        assert(Settings_Visible(list, rows) == 4);
        dvarPut("sv_running", "0");
        assert(look && look->rowCount == 3 && look->header == lists[LOOK][0] && !look->back && look->noteCount == 0);
        assert(look->rows[0].controls[0] == lookInvert && look->rows[0].label == lists[LOOK][1]);
        assert(look->rows[1].controls[0] == lookUp && look->rows[1].label == lists[LOOK][2]);
        assert(look->rows[2].controls[0] == lookSensitivity && look->rows[2].display == lists[LOOK][4]);
        assert(graphics->rowCount == 5 && graphics->rows[2].controls[0] == gammaHw && graphics->rows[2].controls[1] == gammaSoft);
        assert(Settings_Control(&graphics->rows[2]) == gammaSoft && graphics->rows[3].controls[0] == textureButton);
        assert(graphics->rows[4].controls[0] == applyButton && graphics->noteCount == 1 && graphics->notes[0] == warning);
        assert(Settings_Visible(graphics, rows) == 4);
        assert(pad->rowCount == 4 && pad->rows[0].controls[0] == padEnable && pad->rows[1].controls[0] == padFire);
        assert(pad->rows[2].controls[0] == padSprint && pad->rows[3].controls[0] == padDefaults && pad->back == padBack);
        assert(Settings_Visible(pad, rows) == 4 && pad->header == lists[PAD][0] && pad->noteCount == 4);
        assert(defaults->rowCount == 2 && defaults->rows[0].controls[0] == yesOld && defaults->rows[0].controls[1] == yesMp);
        assert(Settings_Control(&defaults->rows[0]) == yesMp && defaults->rows[1].controls[0] == noButton && defaults->noteCount == 5);
        assert(!Settings_Page(&menus[OTHER]) && !UI_WWIISettings_Owns(&menus[OTHER]) && UI_WWIISettings_Owns(&menus[LOOK]));
        /* Buttons side by side on a row keep a row each. */
        assert(Settings_Page(&menus[VOICE])->rowCount == 2);
        cases += 7;
    }

    /* The list over a 75% shade: SETTINGS in the title's gold Bold, the
     * categories in idle capitals on the capture's rows, the first focused
     * and sliding 8.8 right into gold Bold within 233 ms, framed to 266.5;
     * its name and what it opens at the right; Enter Select, Escape Back. */
    openOnly(LIST);
    uiDc.realTime = 1000;
    point(600, 470);
    paint(LIST);
    assert(drawCount > 0 && drawn[0].kind == PIC && near(drawn[0].w, 640) && near(drawn[0].h, 480) && drawn[0].horz == 4 &&
           near(drawn[0].color[3], 0.75f) && drawn[0].material == &whiteMaterial);
    assert(text("SETTINGS", 1, SET_TITLE_X, SET_TITLE_BASELINE, 11.2f, 0.235f, gold));
    assert(text("LOOK", 0, SET_ROW_X, baseline(0), 7.5f, 0.05f, idleColor));
    assert(text("MOVE", 0, SET_ROW_X, baseline(1), 7.5f, 0.05f, idleColor));
    assert(text("SET DEFAULT CONTROLS", 0, SET_ROW_X, baseline(2), 7.5f, 0.05f, idleColor));
    assert(text("OPTIMAL SYSTEM SETTINGS", 0, SET_ROW_X, baseline(4), 7.5f, 0.05f, idleColor));
    assert(!findText("MULTIPLAYER") && !findText("BACK") && !findText("CONTROLS"));
    {
        float top = baseline(0) - SET_BOX_ABOVE;
        int frames = 0;
        for (int i = 0; i < drawCount; i++) {
            const Drawn *d = &drawn[i];
            frames += isPic(d, 0, top, 266.5f, 0.8f, frame) || isPic(d, 0, top + SET_BOX_HEIGHT - 0.8f, 266.5f, 0.8f, frame) ||
                      isPic(d, 266.5f - 0.8f, top + 0.8f, 0.8f, SET_BOX_HEIGHT - 1.6f, frame);
        }
        assert(frames == 3);
    }
    assert(text("LOOK", 1, 410.5f, SET_ROW_TOP, 10.25f, SET_PANEL_TITLE_TRACKING, gold));
    assert(text("Set how the mouse aims and turns your view.", 0, 410.5f, 121.2f, 7.2f, 0, textColor));
    assert(!strcmp(promptText, "ENTER Select|>ESC Back"));
    /* Frames advance the focus at most 100 ms each. */
    uiDc.realTime += 300;
    paint(LIST);
    assert(text("LOOK", 1, SET_ROW_X + SET_FOCUS_INDENT * (1.0f - (1.0f - 100.0f / 233.0f) * (1.0f - 100.0f / 233.0f)), baseline(0), 7.5f,
                0.085f, gold));
    for (int i = 0; i < 2; i++) {
        uiDc.realTime += 100;
        paint(LIST);
    }
    assert(text("LOOK", 1, SET_ROW_X + SET_FOCUS_INDENT, baseline(0), 7.5f, 0.085f, gold));
    cases += 4;

    /* Down focuses the next shown category, playing its hover script; Enter
     * runs its action; Escape runs the menu's own Escape with one of its
     * items. Characters and releases are taken too. */
    scriptCount = 0;
    assert(key(LIST, 0x9b) == 1 && lastScript("play \"mouse_over\"", lists[LIST][2]));
    assert(key(LIST, 0x0d) == 1 && lastScript("open options_move", lists[LIST][2]));
    assert(key(LIST, 0x9b) == 1 && key(LIST, 0x9b) == 1 && key(LIST, 0x0d) == 1 && lastScript("open options_graphics", lists[LIST][5]));
    assert(key(LIST, 0x9a) == 1 && key(LIST, 0x9a) == 1 && key(LIST, 0x9a) == 1 && key(LIST, 0x9a) == 1);
    assert(key(LIST, 0x0d) == 1 && lastScript("open options_graphics_defaults", lists[LIST][6]));
    assert(UI_WWIISettings_KeyEvent(&uiDc, &menus[LIST], 'x' | 0x400, 1) == 1);
    assert(key(LIST, 0x1b) == 1 && lastScript("open main_text; close options_menu;", lists[LIST][0]));
    cases += 4;

    /* A page opened over the list starts at its first row and the list is
     * not painted under it; back on the list, its category keeps the focus;
     * the page opens at its first row again. */
    reload();
    openOnly(LIST);
    paint(LIST);
    key(LIST, 0x9b);
    open(LOOK);
    paint(LOOK);
    assert(Settings_Page(&menus[LOOK])->focus == 0 && findText("SETTINGS // LOOK") && !findText("SETTINGS"));
    key(LOOK, 0x9b);
    key(LOOK, 0x9b);
    assert(Settings_Page(&menus[LOOK])->focus == 2 && key(LOOK, 0x1b) == 1 && uiDc.openMenuCount == 1);
    paint(LIST);
    assert(Settings_Page(&menus[LIST])->focus == 1 && text("MOVE", 1, 410.5f, SET_ROW_TOP, 10.25f, SET_PANEL_TITLE_TRACKING, gold));
    open(LOOK);
    paint(LOOK);
    assert(Settings_Page(&menus[LOOK])->focus == 0);
    cases += 2;

    /* A page opened over the list while it never showed, as the pause menu
     * opens one, runs its own Escape, which closes Options back to where
     * they were opened. Once the list has shown, its pages close back to it,
     * even one reached from another page; Options closed, a page opened that
     * way runs its own Escape again. */
    reload();
    menus[GRAPHICS].onESC = "close options_menu;";
    openOnly(GRAPHICS);
    paint(GRAPHICS);
    scriptCount = 0;
    closes = 0;
    assert(key(GRAPHICS, 0x1b) == 1 && lastScript("close options_menu;", lists[GRAPHICS][0]) && closes == 0);
    uiDc.openMenuCount = 0;
    UI_WWIISettings_Update(&uiDc);
    openOnly(LIST);
    paint(LIST);
    open(MOVE);
    paint(MOVE);
    open(GRAPHICS);
    paint(GRAPHICS);
    scriptCount = 0;
    closes = 0;
    assert(key(GRAPHICS, 0x1b) == 1 && closes == 1 && !strcmp(closed, "options_graphics") && scriptCount == 0);
    uiDc.openMenuCount = 0;
    UI_WWIISettings_Update(&uiDc);
    openOnly(GRAPHICS);
    paint(GRAPHICS);
    closes = 0;
    assert(key(GRAPHICS, 0x1b) == 1 && lastScript("close options_menu;", lists[GRAPHICS][0]) && closes == 0);
    cases += 3;

    /* A page over an 88% shade, titled SETTINGS // its heading: a name and
     * its value in gray centred on 310.8, a key, a slider's groove and bar
     * with its value field, the focused row's arrows 6 wide outside the bar
     * and its frame to 400; the list is not painted under it. */
    reload();
    openOnly(LOOK);
    uiDc.realTime = 5000;
    paint(LOOK);
    assert(drawn[0].kind == PIC && near(drawn[0].color[3], 0.88f) && countText("SETTINGS") == 0);
    assert(text("SETTINGS // LOOK", 1, 44.5f, 69.2f, 11.2f, 0.235f, gold));
    assert(text("INVERT MOUSE", 0, SET_ROW_X, baseline(0), 7.5f, 0.05f, idleColor));
    assert(text("Disabled", 0, 310.8f - 0.5f * UI_WWIIMenu_Width("Disabled", 0, 7.2f, 0), baseline(0), 7.2f, 0, idleColor));
    assert(text("KEY +lookup", 0, 310.8f - 0.5f * UI_WWIIMenu_Width("KEY +lookup", 0, 7.2f, 0), baseline(1), 7.2f, 0, valueGray));
    {
        float b = baseline(2), fraction = (5.0f - 1.0f) / 29.0f;
        int grooves = 0, bars = 0;
        for (int i = 0; i < drawCount; i++) {
            grooves += isPic(&drawn[i], 243.8f, b - SET_SLIDER_ABOVE, 134.0f, 10.35f, groove);
            bars += isPic(&drawn[i], 243.8f, b - SET_SLIDER_ABOVE, 134.0f * fraction, 10.35f, bar);
        }
        assert(grooves == 1 && bars == 1);
        assert(text("5", 0, SET_DISPLAY_X, b, 7.2f, 0, valueGray));
    }
    assert(countPics(&hudMaterials[R_HUD_IMAGE_MENU_ARROW]) == 2);
    {
        const Drawn *left = pic(&hudMaterials[R_HUD_IMAGE_MENU_ARROW], 0), *right = pic(&hudMaterials[R_HUD_IMAGE_MENU_ARROW], 1);
        float b = baseline(0);
        assert(isPic(left, SET_ARROW_LEFT, b - SET_SLIDER_ABOVE, SET_ARROW_WIDTH, 10.35f, arrowColor) && near(left->s1, 1) && near(left->s2, 0));
        assert(isPic(right, SET_ARROW_RIGHT, b - SET_SLIDER_ABOVE, SET_ARROW_WIDTH, 10.35f, arrowColor) && near(right->s1, 0) && near(right->s2, 1));
    }
    assert(text("INVERT MOUSE", 1, 410.5f, SET_ROW_TOP, 10.25f, SET_PANEL_TITLE_TRACKING, gold));
    assert(!strcmp(paragraph(7.2f, textColor, NULL, 10.5f), "Reverse the mouse's vertical axis: pushing it forward looks down."));
    cases += 5;

    /* Right on Yes/No gives the menus the key with the item focused, then runs
     * its action; Enabled shows. Left and right step a slider; a key row
     * waits for its key with Enter: "Press a key" in gold, the wait's text,
     * Backspace and Escape prompts; the key goes to the bind's own
     * handling. */
    scriptCount = 0;
    assert(key(LOOK, 0x9d) == 1 && handleItem == lookInvert && handleKey == 0x9d && handleFocused);
    assert(!(lookInvert->window.dynamicFlags[0] & 2) && lastScript(lookInvert->action, lookInvert));
    paint(LOOK);
    assert(findText("Enabled"));
    key(LOOK, 0x9b);
    key(LOOK, 0x9b);
    assert(key(LOOK, 0x9c) == 1 && handleItem == lookSensitivity && !strcmp(dvarGet("sensitivity"), "3.55"));
    key(LOOK, 0x9a);
    handleCalls = 0;
    assert(key(LOOK, 0x9c) == 1 && handleCalls == 0);
    assert(key(LOOK, 0x0d) == 1 && handleItem == lookUp && handleKey == 0x0d && waiting && lastScript(lookUp->action, lookUp));
    paint(LOOK);
    assert(findText("Press a key") && nearColor(findText("Press a key")->color, gold));
    assert(findText("Waiting for new key. Press ESCAPE to cancel or BACKSPACE to clear"));
    assert(!strcmp(promptText, "BACKSPACE Clear|>ESC Cancel"));
    bindCalls = 0;
    assert(key(LOOK, 'k') == 1 && bindCalls == 1 && bindItem == lookUp && bindKey == 'k' && bindDown && !waiting);
    paint(LOOK);
    assert(findText("Press ENTER or CLICK to change") && !strcmp(promptText, "ENTER Select|>ESC Back"));
    /* A wait the menus began is left to them. */
    waiting = 1;
    assert(UI_WWIISettings_KeyEvent(&uiDc, &menus[LOOK], 'k', 1) == 0);
    waiting = 0;
    cases += 5;

    /* The pointer moving over a row focuses it; resting, it does not. A click
     * on a value's right arrow steps it; on the bar it sets the slider where it
     * points and drags it until released; the right button steps a value back
     * and elsewhere goes back. The wheel moves the focus. */
    reload();
    openOnly(LOOK);
    point(100, baseline(0));
    paint(LOOK);
    point(100, baseline(2) - 5);
    paint(LOOK);
    assert(Settings_Page(&menus[LOOK])->focus == 2);
    key(LOOK, 0x9a);
    paint(LOOK);
    assert(Settings_Page(&menus[LOOK])->focus == 1);
    point(384, baseline(0) - 5);
    paint(LOOK);
    assert(key(LOOK, 0xc8) == 1 && handleItem == lookInvert && handleKey == 0x9d);
    /* A click where the pointer has just moved, before a frame paints it,
     * takes the part under it. */
    point(384, baseline(2) - 5);
    assert(key(LOOK, 0xc8) == 1 && handleItem == lookSensitivity && handleKey == 0x9d &&
           Settings_Page(&menus[LOOK])->focus == 2);
    key(LOOK, 0x9a);
    key(LOOK, 0x9a);
    /* A click where the pointer has just moved, before a frame paints it,
     * takes the part under it. */
    point(384, baseline(2) - 5);
    assert(key(LOOK, 0xc8) == 1 && handleItem == lookSensitivity && handleKey == 0x9d &&
           Settings_Page(&menus[LOOK])->focus == 2);
    key(LOOK, 0x9a);
    key(LOOK, 0x9a);
    point(300, baseline(2) - 5);
    paint(LOOK);
    dvarSets = 0;
    UI_WWIISettings_KeyEvent(&uiDc, &menus[LOOK], 0xc8, 1);
    {
        float cx = (float)uiDc.cursorx * 2.0f, x = 243.8f * 1.5f, w = 134.0f * 1.5f, t = (cx - x) / w;
        snprintf(expected, sizeof(expected), "%g", (double)(1.0f + 29.0f * t));
        assert(dvarSets == 1 && !strcmp(lastSet, "sensitivity") && !strcmp(lastValue, expected));
    }
    /* Dragged over another row, the slider keeps the focus. */
    point(330, baseline(1) - 5);
    paint(LOOK);
    assert(Settings_Page(&menus[LOOK])->focus == 2 && dvarSets == 2);
    UI_WWIISettings_KeyEvent(&uiDc, &menus[LOOK], 0xc8, 0);
    point(340, baseline(4));
    paint(LOOK);
    assert(dvarSets == 2);
    point(100, baseline(0) - 5);
    paint(LOOK);
    assert(key(LOOK, 0xc9) == 1 && handleItem == lookInvert && handleKey == 0x9c);
    point(100, baseline(1) - 5);
    paint(LOOK);
    closes = 0;
    assert(key(LOOK, 0xc9) == 1 && closes == 1 && !strcmp(closed, "options_look"));
    reload();
    openOnly(LOOK);
    paint(LOOK);
    assert(key(LOOK, 0xcd) == 1 && Settings_Page(&menus[LOOK])->focus == 1);
    assert(key(LOOK, 0xce) == 1 && key(LOOK, 0xce) == 1 && Settings_Page(&menus[LOOK])->focus == 2);
    cases += 6;

    /* Graphics: a list's and an enum's values; brightness's slider is the one
     * r_ignorehwgamma shows, explained in WWII's words over its five
     * roundels; the hidden Apply joins the rows once shown; the warning is a
     * note under the explanation; Escape closes the page, back to the list. */
    reload();
    openOnly(GRAPHICS);
    paint(GRAPHICS);
    assert(text("SETTINGS // GRAPHICS", 1, 44.5f, 69.2f, 11.2f, 0.235f, gold));
    assert(findText("1280x720") && findText("Trilinear") && !findText("APPLY"));
    /* A key the menus do not take runs no action. */
    scriptCount = 0;
    handleCalls = 0;
    assert(key(GRAPHICS, 0x9d) == 1 && handleCalls == 1 && handleItem == videoMode && scriptCount == 0);
    key(GRAPHICS, 0x9b);
    key(GRAPHICS, 0x9b);
    paint(GRAPHICS);
    {
        float first;
        assert(!strcmp(paragraph(7.2f, textColor, &first, 10.5f),
                       "Adjust the image brightness to give a good range of contrast. The darkest star should remain visible."));
        assert(near(first, 121.2f));
    }
    assert(countPics(&hudMaterials[R_HUD_IMAGE_MENU_ROUNDEL]) == 5);
    {
        static const float grays[5] = { 0.051f, 0.097f, 0.164f, 0.232f, 0.316f };
        for (int i = 0; i < 5; i++) {
            const Drawn *r = pic(&hudMaterials[R_HUD_IMAGE_MENU_ROUNDEL], i);
            float color[4] = { grays[i], grays[i], grays[i], 1 };
            assert(isPic(r, 461.7f + 74.15f * i - 27.9f, 239.2f - 27.9f, 55.8f, 55.8f, color));
        }
    }
    {
        float description, note;
        paragraph(7.2f, textColor, &description, 10.5f);
        assert(!strcmp(paragraph(6.0f, noteColor, &note, 10.5f * 6.0f / 7.2f),
                       "We recommend using \"Optimal System Settings.\" Manually changing settings could result in poor performance."));
        assert(near(note, description + 10.5f + 5.0f));
    }
    assert(key(GRAPHICS, 0x9b) == 1 && key(GRAPHICS, 0x9b) == 1 && Settings_Page(&menus[GRAPHICS])->focus == 0);
    applyButton->window.dynamicFlags[0] = 4;
    paint(GRAPHICS);
    assert(findText("APPLY"));
    key(GRAPHICS, 0x9a);
    scriptCount = 0;
    assert(key(GRAPHICS, 0x0d) == 1 && lastScript("open vid_restart_popmenu", applyButton));
    key(GRAPHICS, 0x9a);
    assert(key(GRAPHICS, 0x0d) == 1 && lastScript(textureButton->action, textureButton));
    closes = 0;
    assert(key(GRAPHICS, 0x1b) == 1 && closes == 1 && !strcmp(closed, "options_graphics"));
    cases += 5;

    /* A long page shows eleven rows and scrolls to keep the focus among
     * them, with a scrollbar right of the frame. */
    reload();
    openOnly(MOVE);
    paint(MOVE);
    assert(findText("KEY +forward") && findText("KEY lowerstance") && !findText("KEY raisestance") && !findText("KEY toggleprone"));
    for (int i = 0; i < 12; i++) key(MOVE, 0x9b);
    paint(MOVE);
    {
        const Drawn *last = findText("KEY togglecrouch");
        assert(Settings_Page(&menus[MOVE])->scroll == 2 && !findText("KEY +forward") && last && near(last->y, baseline(10)));
    }
    {
        float top = SET_ROW_TOP - SET_BOX_ABOVE, height = 10 * SET_ROW_SPACING + SET_BOX_HEIGHT, thumb = height * 11.0f / 14.0f;
        const float track[4] = { 1, 1, 1, 0.12f };
        int found = 0;
        for (int i = 0; i < drawCount; i++)
            found += isPic(&drawn[i], 404, top, 0.8f, height, track) + isPic(&drawn[i], 404, top + (height - thumb) * 2.0f / 3.0f, 0.8f, thumb, frame);
        assert(found == 2);
    }
    key(MOVE, 0x9b);
    key(MOVE, 0x9b);
    paint(MOVE);
    assert(Settings_Page(&menus[MOVE])->focus == 0 && Settings_Page(&menus[MOVE])->scroll == 0 && findText("KEY +forward"));
    cases += 3;

    /* A name that is a button with nothing to do names its row; a list of
     * Yes and No reads Enabled or Disabled. The gametag edits as the menus'
     * fields do: Enter begins, characters go to the field, Escape ends it
     * without leaving the page, and Up ends it and moves the focus. */
    reload();
    openOnly(GAME);
    paint(GAME);
    assert(Settings_Page(&menus[GAME])->rows[0].label == lists[GAME][1] && findText("ENABLE CROSSHAIR") && findText("Enabled"));
    dvarPut("cg_drawcrosshair", "0");
    paint(GAME);
    assert(findText("Disabled") && !findText("Enabled"));
    assert(findText("GAMETAG") && findText("Chapu54"));
    key(GAME, 0x9b);
    assert(key(GAME, 0x0d) == 1 && editBegins == 1 && editItem == nameEdit);
    paint(GAME);
    assert(!strcmp(promptText, "ENTER Done|"));
    assert(UI_WWIISettings_KeyEvent(&uiDc, &menus[GAME], 'q' | 0x400, 1) == 1 && fieldKey == ('q' | 0x400) && editItem);
    closes = 0;
    assert(UI_WWIISettings_KeyEvent(&uiDc, &menus[GAME], 0x1b, 1) == 1 && !editItem && closes == 0);
    key(GAME, 0x0d);
    fieldCalls = 0;
    assert(key(GAME, 0x9a) == 1 && !editItem && fieldCalls == 0);
    cases += 3;

    /* The controller page lists every tab; its Back button takes Escape, and
     * a key row waits for a button. */
    reload();
    openOnly(PAD);
    paint(PAD);
    assert(text("SETTINGS // CONTROLLER", 1, 44.5f, 69.2f, 11.2f, 0.235f, gold));
    assert(findText("ENABLE CONTROLLER") && findText("FIRE") && findText("SPRINT / HOLD BREATH") && findText("RESTORE CONTROLLER DEFAULTS"));
    assert(!findText("SETTINGS") && !findText("BUTTONS 1") && !findText("BACK"));
    assert(!strcmp(paragraph(6.0f, noteColor, NULL, 10.5f * 6.0f / 7.2f),
                   "Controller connected. Left stick: move / navigate. Esc cancels an assignment; Backspace clears it."));
    key(PAD, 0x9b);
    key(PAD, 0x0d);
    paint(PAD);
    assert(findText("Press a button"));
    key(PAD, 0x1b);
    scriptCount = 0;
    assert(key(PAD, 0x1b) == 1 && lastScript("close browser_gamepad;", padBack));
    cases += 3;

    /* The confirmation: Yes and No, their notes as one paragraph; once
     * confirmed, no rows, the page's heading over the new notes. */
    reload();
    openOnly(DEFAULTS);
    paint(DEFAULTS);
    assert(findText("YES") && findText("NO"));
    assert(!strcmp(paragraph(6.0f, noteColor, NULL, 10.5f * 6.0f / 7.2f),
                   "Each setting will be restored to its default value. Are you sure?"));
    assert(!strcmp(paragraph(7.2f, textColor, NULL, 10.5f), "Restore every control to its default key."));
    yesOld->window.dynamicFlags[0] = yesMp->window.dynamicFlags[0] = noButton->window.dynamicFlags[0] = 0;
    sureNote->window.dynamicFlags[0] = 0;
    finishNote->window.dynamicFlags[0] = lists[DEFAULTS][8]->window.dynamicFlags[0] = 4;
    paint(DEFAULTS);
    assert(text("SETTINGS // SET DEFAULT CONTROLS", 1, 44.5f, 69.2f, 11.2f, 0.235f, gold));
    assert(text("SET DEFAULT CONTROLS", 1, 410.5f, SET_ROW_TOP, 10.25f, SET_PANEL_TITLE_TRACKING, gold) && !strcmp(promptText, ">ESC Back"));
    assert(!strcmp(paragraph(6.0f, noteColor, NULL, 10.5f * 6.0f / 7.2f),
                   "Each setting will be restored to its default value. Your controls are now set to their original default values."));
    assert(key(DEFAULTS, 0x0d) == 1 && key(DEFAULTS, 0x9b) == 1);
    cases += 3;

    /* On a 4:3 screen the explanation wraps greedily within its narrower
     * width, lines 10.5 apart, and the roundels shrink into it. */
    reload();
    screenW = 960.0f;
    openOnly(GRAPHICS);
    key(GRAPHICS, 0x9b);
    key(GRAPHICS, 0x9b);
    paint(GRAPHICS);
    {
        const char *whole = "Adjust the image brightness to give a good range of contrast. The darkest star should remain visible.";
        char joined[256] = "";
        float width = 640.0f - 410.5f - 40.0f, y = 121.2f;
        int lines = 0;
        for (int i = 0; i < drawCount; i++) {
            const Drawn *d = &drawn[i];
            if (d->kind != TEXT || !near(d->cap, 7.2f) || !near(d->x, 410.5f) || d->y < 115.0f) continue;
            assert(near(d->y, y) && UI_WWIIMenu_Width(d->text, 0, 7.2f, 0) <= width);
            if (lines) {
                char trial[256];
                const char *next = strchr(d->text, ' ');
                snprintf(trial, sizeof(trial), "%s %.*s", drawn[i - 1].text, next ? (int)(next - d->text) : (int)strlen(d->text), d->text);
                assert(UI_WWIIMenu_Width(trial, 0, 7.2f, 0) > width);
                strcat(joined, " ");
            }
            strcat(joined, d->text);
            y += 10.5f;
            lines++;
        }
        assert(lines >= 3 && !strcmp(joined, whole));
        const Drawn *lastRoundel = pic(&hudMaterials[R_HUD_IMAGE_MENU_ROUNDEL], 4);
        assert(lastRoundel && lastRoundel->x + lastRoundel->w <= 410.5f + width + 0.01f);
    }
    screenW = 1280.0f;
    cases += 2;

    /* Switched off, the menus keep everything: nothing is painted or taken.
     * bg gives way while a laid-out menu is open; closing Options brings it
     * back at its first category. */
    reload();
    openOnly(LIST);
    paint(LIST);
    cgDc.menuStack[0] = &menus[LIST];
    cgDc.openMenuCount = 1;
    assert(UI_WWIISettings_Covers(&uiDc) && !UI_WWIISettings_Covers(&cgDc));
    key(LIST, 0x9b);
    layoutOn = 0;
    paint(LIST);
    assert(drawCount == 0 && !UI_WWIISettings_Owns(&menus[LIST]) && !UI_WWIISettings_Covers(&uiDc));
    assert(UI_WWIISettings_KeyEvent(&uiDc, &menus[LIST], 0x9b, 1) == 0);
    layoutOn = 1;
    uiDc.openMenuCount = 0;
    UI_WWIISettings_Update(&uiDc);
    assert(!UI_WWIISettings_Covers(&uiDc) && Settings_Page(&menus[LIST])->focus == 0);
    open(OTHER);
    assert(!UI_WWIISettings_Covers(&uiDc) && UI_WWIISettings_KeyEvent(&uiDc, &menus[OTHER], 0x9b, 1) == 0);
    cases += 3;

    /* The arrow: opaque along its base, clear beyond its tip's sides and
     * partly covered at the tip; the roundel: clear in its corners, its star
     * white and its disk 0.93 gray. Both are built once, and again after the
     * renderer restarts. */
    {
        const byte *a = slotTexels[R_HUD_IMAGE_MENU_ARROW], *r = slotTexels[R_HUD_IMAGE_MENU_ROUNDEL];
        assert(slotWidth[R_HUD_IMAGE_MENU_ARROW] == 32 && slotWidth[R_HUD_IMAGE_MENU_ROUNDEL] == 64);
        assert(a[(16 * 32 + 2) * 4 + 3] == 255 && a[(1 * 32 + 30) * 4 + 3] == 0 && a[(30 * 32 + 30) * 4 + 3] == 0);
        assert(a[(16 * 32 + 30) * 4 + 3] > 0 && a[(16 * 32 + 30) * 4 + 3] < 255 && a[(16 * 32 + 2) * 4] == 255);
        assert(r[(0 * 64 + 0) * 4 + 3] == 0 && r[(32 * 64 + 32) * 4] == 255 && r[(32 * 64 + 32) * 4 + 3] == 255);
        assert(r[(32 * 64 + 50) * 4] == 237 && r[(32 * 64 + 50) * 4 + 3] == 255);
        openOnly(LIST);
        paint(LIST);
        int built = slotBuilds[R_HUD_IMAGE_MENU_ARROW];
        paint(LIST);
        assert(slotBuilds[R_HUD_IMAGE_MENU_ARROW] == built);
        generation++;
        paint(LIST);
        assert(slotBuilds[R_HUD_IMAGE_MENU_ARROW] == built + 1 && slotBuilds[R_HUD_IMAGE_MENU_ROUNDEL] == built + 1);
        cases += 2;
    }

    printf("%d\n", cases);
    return 0;
}
'''

# Regressions the cases must reject.
mutants = [
    [('    if (s_settings.shown == page)\n        return;', '    if (s_settings.shown)\n        return;')],
    [('            fabsf(row->x - rect->x) <= SET_ROW_TOLERANCE &&', '            1 &&')],
    [('    if (Settings_Tabbed(item) || !(item->dvarFlags & SET_DVAR_SHOW_MASK))', '    if (!(item->dvarFlags & SET_DVAR_SHOW_MASK))')],
    [('    item->window.dynamicFlags[0] |= SET_WINDOW_HASFOCUS;\n', '')],
    [('    if (handled && item->action)', '    if (item->action)')],
    [('        if (!s_settings.bindItem)\n            return 0;\n', '')],
    [('            Item_EditField_End();\n        } else {', '        } else {')],
    [('    if (at >= page->scroll + SET_ROWS_SHOWN)\n        page->scroll = at - SET_ROWS_SHOWN + 1;\n', '')],
    [('        if (s_settings.hoverRow >= 0 && s_settings.dragRow < 0 &&', '        if (s_settings.hoverRow >= 0 &&')],
    [('    } else if (page->back && Settings_Shown(page->back) && page->back->action) {',
      '    } else if (0) {')],
    [('        if (Settings_IsBack(item)) {', '        if (0) {')],
    [('                           !length ? "" : (strchr(".?!:;", last) || (*text >= \'a\' && *text <= \'z\') ? " " : ". "),',
      '                           !length ? "" : " ",')],
    [('            if ((int)strlen(trial) >= (int)sizeof(trial) - 1 || UI_WWIIMenu_Width(trial, 0, cap, 0.0f) > width) {',
      '            if ((int)strlen(trial) >= (int)sizeof(trial) - 1) {')],
    [('    if (span > width && span > 0.0f) {', '    if (0) {')],
    [('    if (control && control->dvar && !I_stricmp(control->dvar, "r_gamma"))', '    if (0)')],
    [('    if (focused && enabled && (kind == SET_KIND_VALUE || kind == SET_KIND_SLIDER)) {',
      '    if (enabled && (kind == SET_KIND_VALUE || kind == SET_KIND_SLIDER)) {')],
    [('                          SET_ALIGN_TOP, left ? 1.0f : 0.0f, 0.0f, left ? 0.0f : 1.0f, 1.0f, s_setArrow,',
      '                          SET_ALIGN_TOP, 0.0f, 0.0f, 1.0f, 1.0f, s_setArrow,')],
    [('    if (s_settings.generation != R_GetHudImageGeneration()) {', '    if (0) {')],
    [('    for (i = 0; i < s_settings.pageCount; i++) {\n        if (s_settings.pages[i].list)\n            s_settings.pages[i].focus = 0;\n    }\n', '')],
    [('    if (dc != s_settings.dc)\n        return NULL;\n', '')],
    [('    return Settings_On() && Settings_Page(menu) != NULL;', '    return Settings_Page(menu) != NULL;')],
    [('        if (item == page->header || Settings_IsControl(item) || (!Settings_Decoration(item) && item->action))',
      '        if (item == page->header || Settings_IsControl(item) || !Settings_Decoration(item))')],
    [('    if (text && !I_stricmp(text, "@MENU_YES"))\n        return "Enabled";\n', '')],
    [('    Settings_HoverNow(dc, page);\n    if (!Settings_HoverValid(page))\n        return;',
      '    if (!Settings_HoverValid(page))\n        return;')],
    [('#define SET_PAGE_SHADE 0.88f', '#define SET_PAGE_SHADE 0.75f')],
    [('#define SET_ROW_SPACING 28.9f', '#define SET_ROW_SPACING 27.1f')],
    [('#define SET_TITLE_TRACKING 0.235f', '#define SET_TITLE_TRACKING 0.18f')],
    [('#define SET_LABEL_TRACKING 0.05f', '#define SET_LABEL_TRACKING 0.12f')],
    [('#define SET_BOX_RIGHT 400.0f', '#define SET_BOX_RIGHT 380.0f')],
    [('#define SET_SLIDER_LEFT 243.8f', '#define SET_SLIDER_LEFT 250.0f')],
    [('#define SET_PANEL_X 410.5f', '#define SET_PANEL_X 400.0f')],
    [('#define SET_ROUNDEL_STEP 74.15f', '#define SET_ROUNDEL_STEP 70.0f')],
    [('#define SET_ROWS_SHOWN 11', '#define SET_ROWS_SHOWN 12')],
    [('    } else if (!s_settings.listShown && menu->onESC && menu->itemCount > 0) {', '    } else if (0) {')],
    [('    if (page->list)\n        s_settings.listShown = 1;\n', '')],
    [('    s_settings.listShown = 0;\n', '')],
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


def program(text):
    module = re.sub(r'^#include "[^"]+"\n', '', text, flags=re.M)
    assert module.count('#include') == 4
    return support + module + checks


strict = ['-Wall', '-Werror', '-Wno-unused-function']
with tempfile.TemporaryDirectory(prefix='cod2-wwii-settings-') as directory:
    cases = int(run(directory, 'settings', program(source), strict, True).stdout)
    rejected = 0
    for index, pairs in enumerate(mutants):
        variant = source
        for old, new in pairs:
            assert variant.count(old) == 1, old
            variant = variant.replace(old, new)
        if evidence(variant):
            rejected += 1
            continue
        run(directory, f'settings{index}', program(variant), ['-w'], False)
        rejected += 1

print('PASS: with ui_wwiiMainMenu on, Options\' buttons make a list of categories and each page rows of a name and its '
      'value, slider, keys or field, in the settings captures\' gold, grays, rows and frame, the focused row explained '
      'at the right with its notes and brightness\'s roundels, eleven rows at a time with a scrollbar; keys, pointer, '
      'wheel and right button drive the retail items through the menus\' own handling, keys wait as the menus\' binds '
      'do and the gametag edits as their fields do; Escape goes back to the list, or from a page opened without it '
      'runs the page\'s own Escape, and switched off the menus keep it '
      f'all; {cases} cases; the layout\'s numbers within the captures\' measurements; {rejected} failing regression '
      'mutants')
