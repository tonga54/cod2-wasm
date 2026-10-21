#!/usr/bin/env python3
"""Run the production Select Class screen against a recording UI: the class
menu a match opens first, laid out as the WWII capture of Select Division
without its Division and Basic Training panels; its Custom and Standard Issue
tabs; the response each choice sends; editing only the player's own classes;
the blurred, gray match it asks the renderer for; and its numbers against the
capture's, in Barlow's metrics."""
from pathlib import Path
import math
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(root / 'scripts'))
from barlow_metrics import Font  # noqa: E402

menu = (root / 'src/PC/ui_mp/ui_class_menu.c').read_text()
data = (root / 'src/PC/ui_mp/ui_class_data.c').read_text()
headers = ''.join((root / 'src/headers/PC/ui_mp' / name).read_text()
                  for name in ('ui_class_data.h', 'ui_class_menu.h', 'ui_wwii_menu.h'))
# The menu's own first-party headers beyond those, as the build has them.
for header in re.findall(r'^#include "headers/(PC/(?!ui_mp/ui_(?:class_data|class_menu|wwii_menu)\.h)[^"]+\.h)"', menu, re.M):
    if not header.startswith('PC/gfx_d3d/'):
        headers = (root / 'src/headers' / header).read_text() + headers
browser_class = (root / 'downstream/ui/browser_class.menu').read_text()
ui_main = (root / 'src/PC/ui_mp/ui_main_mp.c').read_text()

# The match's class menu opens the native screen in its choosing mode.
assert 'uiScript classMenuOpen choose;' in browser_class
assert 'UI_ClassMenu_Open((displayContextDef_t *)uiInfo, !I_stricmp(mode, "choose"));' in ui_main

# Under a class screen of a match the HUD gives way, as under the scoreboard:
# the frame takes the screen's backdrop request before it draws.
draw2d = (root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text()
draw2d = draw2d[draw2d.index('unsigned int CG_Draw2D(void)\n{'):]
assert (draw2d.index('    CG_EndgameFrame(') < draw2d.index('    hudCovered = CG_ScoreTableHidesHud();\n') <
        draw2d.index('    if (g_webBackdropGray > 0.0f) {\n        hudCovered = 1;\n        drawHudMenus = 0;\n'
                     '        drawHudElems = 0;\n    }\n') <
        draw2d.index('killcam = drawHudElems && CG_KillcamActive();'))

# Without a team, the in-game menu's Choose Class opens the class menu: the
# server answers changeweapon after the gametype's handler has closed menus.
gsc = (root / 'downstream/server/browser_classes.gsc').read_text()
handler = gsc[gsc.index('onMenuResponse()\n{'):]
handler = handler[:handler.index('\n}\n')]
assert 'player thread onMenuResponse();' in gsc
assert (handler.index('self waittill("menuresponse", menu, response);') < handler.index('response != "changeweapon"') <
        handler.index('self onTeam()') < handler.index('waittillframeend;') <
        handler.index('self openMenu(level.browser_class_menu);'))

Regular, Bold = Font('Regular'), Font('Bold')
UNIT = 1.2  # px a unit on the 1024 x 576 capture of Select Division


def evidence(text):
    """The screen's numbers against the capture's, as measured on it: the
    failures."""
    failures = []

    def define(name):
        match = re.search(r'#define CAC_SELECT_' + name + r' (-?[\d.]+)f', text)
        return float(match[1]) if match else float('nan')

    def check(ok, what):
        if not ok:
            failures.append(what)

    def bearing(font, letter, cap):
        return font.ink[font.cmap[ord(letter)]][0] * cap / font.cap

    def width(font, word, cap, tracking):
        return font.ink_width(word, tracking) * cap

    def rms(font, words, cap, tracking):
        return math.sqrt(sum((width(font, w, cap, tracking) - px / UNIT) ** 2 for w, px in words) / len(words))

    # SELECT DIVISION in gold Bold is inked from 56 to 242 px, its capitals
    # from rows 104.4 to 117.8; the frame starts at row 56.
    cap, tracking = define('TITLE_CAP'), define('TITLE_TRACKING')
    check(abs(width(Bold, 'SELECT DIVISION', cap, tracking) - 186 / UNIT) < 0.8, 'title width')
    check(abs(define('TITLE_LEFT') + bearing(Bold, 'S', cap) - 56 / UNIT) < 0.3, 'title left')
    check(abs(cap - 13.4 / UNIT) < 0.3 and abs(define('TITLE_BASELINE') - (117.8 - 56) / UNIT) < 0.3, 'title rows')
    # The idle rows' names, inked from 65 px: Regular capitals 8.6 to 9 px
    # tall, baselines 35 px apart from 227.7.
    idle = (('AIRBORNE', 68), ('ARMORED', 66), ('MOUNTAIN', 72), ('EXPEDITIONARY', 112), ('RESISTANCE', 86),
            ('CAVALRY', 61), ('COMMANDO', 78), ('WANDERLUST', 95), ('SPECIALIST', 81))
    cap, tracking = define('ROW_CAP'), define('ROW_TRACKING')
    check(rms(Regular, idle, cap, tracking) < 1.1, 'row widths')
    check(abs(define('ROW_LEFT') + bearing(Regular, 'A', cap) - 65 / UNIT) < 0.3, 'row left')
    check(abs(define('ROW_PITCH') - 35 / UNIT) < 0.05, 'row pitch')
    check(abs(define('ROW_TOP') + define('ROW_PITCH') + define('ROW_BASELINE') - (227.7 - 56) / UNIT) < 0.3,
          'row baseline')
    # The focused row is framed from row 174 to 207 and to 276 px; INFANTRY
    # in gold Bold is inked from 75 to 151 px on a baseline at 193.9.
    cap, tracking = define('FOCUS_CAP'), define('FOCUS_TRACKING')
    check(abs(width(Bold, 'INFANTRY', cap, tracking) - 76 / UNIT) < 0.5, 'focused width')
    check(abs(define('ROW_LEFT') + define('FOCUS_INDENT') + bearing(Bold, 'I', cap) - 75 / UNIT) < 0.3, 'indent')
    check(abs(define('ROW_TOP') + define('FOCUS_BASELINE') - (193.9 - 56) / UNIT) < 0.3, 'focused baseline')
    check(abs(define('ROW_TOP') - (174 - 56) / UNIT) < 0.1 and abs(define('ROW_WIDTH') - 276 / UNIT) < 0.5 and
          abs(define('ROW_HEIGHT') - 33.6 / UNIT) < 0.5, 'focused frame')
    # CUSTOM, framed from 52 to 160 px and rows 141 to 157, inked 39 px wide;
    # STANDARD ISSUE, filled from 167 to 276 px and row 138.3, inked 85 px.
    check(abs(width(Regular, 'CUSTOM', define('TAB_IDLE_CAP'), define('TAB_IDLE_TRACKING')) - 39 / UNIT) < 0.6,
          'idle tab width')
    check(abs(width(Bold, 'STANDARD ISSUE', define('TAB_CAP'), define('TAB_TRACKING')) - 85 / UNIT) < 0.8, 'tab width')
    left, tab, gap = define('TAB_LEFT'), define('TAB_WIDTH'), define('TAB_GAP')
    check(abs(left - 53 / UNIT) < 0.5 and abs(left + tab - 160 / UNIT) < 1.0 and abs(left + 2 * tab + gap - 276 / UNIT) < 0.5,
          'tab columns')
    check(abs(define('TAB_TOP') - (138.3 - 56) / UNIT) < 0.3 and abs(define('TAB_IDLE_TOP') - (141 - 56) / UNIT) < 0.3 and
          abs(define('TAB_BOTTOM') - (157 - 56) / UNIT) < 0.3, 'tab rows')
    # The capture's Division column starts at 300 px and its loadout at 464:
    # tiles of 147, 99 and 45 px from rows 166, 350 and 484, the tactical one
    # at 614 px; the captions in Regular, PRIMARY inked 36 px wide; the names
    # in Bold, INFANTRY 61 px and PANZERSCHRECK 106 px.
    check(abs(define('LOADOUT_LEFT') - 300 / UNIT) < 0.1 and abs(define('LOADOUT_COLUMN') - 150 / UNIT) < 0.1,
          'loadout columns')
    for slot, top, size in (('PRIMARY', 166, 147), ('SECONDARY', 350, 99), ('EQUIPMENT', 484, 45)):
        check(abs(define(slot + '_TOP') - (top - 56) / UNIT) < 0.3 and abs(define(slot + '_SIZE') - size / UNIT) < 0.3,
              slot.lower() + ' tile')
    labels = (('DIVISION', 33), ('PRIMARY', 36), ('SECONDARY', 50), ('LETHAL', 31), ('TACTICAL', 38))
    check(rms(Regular, labels, define('CAPTION_CAP'), define('CAPTION_TRACKING')) < 0.9, 'caption widths')
    names = (('INFANTRY', 61), ('PANZERSCHRECK', 106))
    check(rms(Bold, names, define('NAME_CAP'), define('NAME_TRACKING')) < 0.5, 'name widths')
    # Select's label ends 28.9 above the bottom; the main menu's 49.8.
    check(abs(define('PROMPT_LOWER') - (49.8 - 28.9)) < 0.1, 'prompt row')
    check(abs(define('PROMPT_LEFT') - 64 / UNIT) < 0.1 and abs(define('PROMPT_RIGHT') - (1024 - 959) / UNIT) < 0.1,
          'prompt edges')
    return failures


failures = evidence(menu)
assert not failures, failures

support = r'''
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
typedef float vec_t;
typedef vec_t vec4_t[4];
typedef int qboolean;
typedef struct Material { const char *name; } Material;
typedef Material *MaterialHandle;
typedef struct itemDef_s itemDef_t;
typedef struct { const char *name; } Window;
typedef struct menuDef_t { Window window; } menuDef_t;
typedef struct displayContextDef_t { int realTime; int cursorx, cursory; } displayContextDef_t;
typedef struct sharedUiInfo_t { struct { MaterialHandle whiteMaterial; } assets; } sharedUiInfo_t;
int R_GetHudImageGeneration(void);
MaterialHandle R_GetHudSilhouetteMaterial(MaterialHandle icon, float *aspect);
MaterialHandle R_GetHudPictureMaterial(const char *name, float *st, float *aspect);
MaterialHandle R_GetWeaponCamoMaterial(int camo);
MaterialHandle R_GetWeaponCamoPicture(MaterialHandle picture, int silhouette, int camo, int style, float *aspect);
''' + headers + r'''
enum { TEXT = 1, PIC };
typedef struct { int kind; char text[96]; int bold; float x, y, w, h, cap, tracking; int horz, vert; vec4_t color;
    MaterialHandle material; } Drawn;
static Drawn drawn[256];
static int drawCount;
static Drawn *record(int kind) {
    Drawn *d;
    assert(drawCount < 256);
    d = &drawn[drawCount++];
    memset(d, 0, sizeof(*d));
    d->kind = kind;
    return d;
}
static const float wwiiColors[5][4] = { { 0.776f, 0.776f, 0.769f, 1 }, { 0.80f, 0.72f, 0.47f, 1 },
                                        { 0.851f, 0.843f, 0.827f, 1 }, { 0.812f, 0.812f, 0.812f, 1 }, { 1, 1, 1, 1 } };
const float *UI_WWIIMenu_Color(int color) { assert(color >= 0 && color < 5); return wwiiColors[color]; }
void UI_WWIIMenu_Text(const char *text, int bold, float x, float baseline, int horzAlign, int vertAlign, float cap,
                      float tracking, const vec_t *color) {
    Drawn *d = record(TEXT);
    snprintf(d->text, sizeof(d->text), "%s", text);
    d->bold = bold; d->x = x; d->y = baseline; d->horz = horzAlign; d->vert = vertAlign; d->cap = cap;
    d->tracking = tracking;
    memcpy(d->color, color, sizeof(d->color));
}
/* Capitals 0.6 of their height wide, Bold's 0.7, and W and M as wide as
 * tall, standing in for Barlow's. */
float UI_WWIIMenu_Width(const char *text, int bold, float cap, float tracking) {
    float width = 0.0f;
    for (int i = 0; text[i]; i++)
        width += cap * (text[i] == 'W' || text[i] == 'M' ? 1.0f : bold ? 0.7f : 0.6f) + (text[i + 1] ? tracking * cap : 0.0f);
    return width;
}
static int promptCalls, promptCount;
static const char *promptKeys[4], *promptLabels[4], *promptRightKey, *promptRightLabel;
static float promptLeft, promptRight, promptLower;
void UI_WWIIMenu_PromptsAt(const char *const *keys, const char *const *labels, int count, const char *rightKey,
                           const char *rightLabel, float left, float right, float lower) {
    assert(count >= 0 && count <= 4);
    promptCalls++;
    promptCount = count;
    for (int i = 0; i < count; i++) { promptKeys[i] = keys[i]; promptLabels[i] = labels[i]; }
    promptRightKey = rightKey; promptRightLabel = rightLabel;
    promptLeft = left; promptRight = right; promptLower = lower;
}
void UI_WWIIMenu_Prompts(const char *const *keys, const char *const *labels, int count, const char *rightKey,
                         const char *rightLabel) {
    UI_WWIIMenu_PromptsAt(keys, labels, count, rightKey, rightLabel, -1.0f, -1.0f, 0.0f);
}
static int washes;
void UI_WWIIMenu_PaintWash(void) { washes++; }
static Material white = { "white" };
sharedUiInfo_t sharedUiInfo = { { &white } };
int R_GetHudImageGeneration(void) { return 0; }
MaterialHandle R_GetHudSilhouetteMaterial(MaterialHandle icon, float *aspect) { (void)icon; *aspect = 0.0f; return NULL; }
/* No owner pictures and no camouflage: the retail images and icons. */
MaterialHandle R_GetHudPictureMaterial(const char *name, float *st, float *aspect) {
    (void)name; (void)st; (void)aspect;
    return NULL;
}
MaterialHandle R_GetWeaponCamoMaterial(int camo) { (void)camo; return NULL; }
MaterialHandle R_GetWeaponCamoPicture(MaterialHandle picture, int silhouette, int camo, int style, float *aspect) {
    (void)picture; (void)silhouette; (void)camo; (void)style;
    *aspect = 0.0f;
    return NULL;
}
static Material materials[64];
static int materialCount;
MaterialHandle CL_RegisterMaterialNoMip(const char *name, int imageTrack) {
    assert(imageTrack == 3);
    for (int i = 0; i < materialCount; i++)
        if (!strcmp(materials[i].name, name))
            return &materials[i];
    assert(materialCount < 64);
    materials[materialCount].name = name;
    return &materials[materialCount++];
}
void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2,
                       float t2, const vec_t *color, MaterialHandle material) {
    Drawn *d = record(PIC);
    assert(s1 == 0.0f && t1 == 0.0f && s2 == 1.0f && t2 == 1.0f);
    d->x = x; d->y = y; d->w = w; d->h = h; d->horz = horzAlign; d->vert = vertAlign; d->material = material;
    memcpy(d->color, color, sizeof(d->color));
}
/* A 1280 x 720 screen: aligned items scale by its 1.5 px a unit from their
 * edge; the full screen's 640 by 480, where the cursor moves, stretches over
 * all of it. */
void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign) {
    assert(vertAlign == 1 && (horzAlign == 1 || horzAlign == 2 || horzAlign == 3));
    *x = (horzAlign == 1 ? 0.0f : horzAlign == 2 ? 640.0f : 1280.0f) + *x * 1.5f;
    *y *= 1.5f; *w *= 1.5f; *h *= 1.5f;
}
void CalcScreenX(float *x, int horzAlign) { assert(horzAlign == 4); *x *= 2.0f; }
void CalcScreenY(float *y, int vertAlign) { assert(vertAlign == 4); *y *= 1.5f; }
int FS_ReadFile(const char *qpath, void **buffer) { (void)qpath; *buffer = NULL; return -1; }
void FS_FreeFile(void *buffer) { (void)buffer; assert(0); }
const char *Dvar_GetVariantString(const char *name) { assert(!strcmp(name, "name")); return "Tester"; }
int Dvar_GetInt(const char *name) { assert(!strcmp(name, "sv_serverId")); return 77; }
static char command[256];
static int commands;
void Cbuf_ExecuteText(int when, const char *text) {
    assert(when == 2);
    snprintf(command, sizeof(command), "%s", text);
    commands++;
}
/* The class menu is the match's script menu 3. */
const char *CL_GetConfigString(int index) { return index == 0x4de + 3 ? "browser_class" : index == 0x4de ? "ingame" : ""; }
char *va(const char *format, ...) {
    static char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return buffer;
}
static char closed[64], opened[64];
void Menus_CloseByName(displayContextDef_t *dc, const char *name) { (void)dc; snprintf(closed, sizeof(closed), "%s", name); }
qboolean Menus_OpenByName(displayContextDef_t *dc, const char *name) {
    (void)dc;
    snprintf(opened, sizeof(opened), "%s", name);
    return 1;
}
int I_stricmp(const char *s0, const char *s1) { return strcasecmp(s0, s1); }
/* WEAPON_MP44 reads MP44; other references read as they are. */
const char *UI_SafeTranslateString(const char *reference) {
    return !strncmp(reference, "WEAPON_", 7) ? reference + 7 : reference;
}
float g_webMenuBackdrop;
void UI_PauseMenu_Return(displayContextDef_t *dc) { (void)dc; }
int UI_PlayLocalSoundAliasByName(const char *name) {
    assert(!strcmp(name, "mouse_over") || !strcmp(name, "mouse_click"));
    return 1;
}
'''

checks = r'''
static menuDef_t classMenu = { { "browser_class" } }, createMenu = { { "create_a_class" } };
static displayContextDef_t dc = { 1000, 0, 0 };
static const float gold[4] = { 0.80f, 0.72f, 0.47f, 1 }, idle[4] = { 0.776f, 0.776f, 0.769f, 1 };
static const float title[4] = { 0.851f, 0.843f, 0.827f, 1 };
static const float note[4] = { 0.70f, 0.70f, 0.68f, 1 }, edge[4] = { 1, 1, 1, 0.22f };
static const float selectShade[4] = { 0, 0, 0, 0.3f }, shade[4] = { 0, 0, 0, 0.55f };
static const float tabFill[4] = { 0.831f, 0.812f, 0.769f, 1 }, tabText[4] = { 0.09f, 0.09f, 0.09f, 1 };
static const float tabEdge[4] = { 1, 1, 1, 0.55f }, rowFill[4] = { 1, 1, 1, 0.05f };
static const float rowEdge[4] = { 0.80f, 0.72f, 0.47f, 0.85f }, tileFill[4] = { 1, 1, 1, 0.07f };
static const char offline[] = "Classes are unavailable: changes last until you leave.";
static int near(float a, float b) { return fabsf(a - b) < 2e-3f; }
static int same(const float *a, const float *b) {
    return near(a[0], b[0]) && near(a[1], b[1]) && near(a[2], b[2]) && near(a[3], b[3]);
}
static const Drawn *text(const char *s) {
    for (int i = 0; i < drawCount; i++)
        if (drawn[i].kind == TEXT && !strcmp(drawn[i].text, s))
            return &drawn[i];
    return NULL;
}
static int drawnText(const char *s, int bold, float x, float y, float cap, float tracking, const float *color) {
    const Drawn *d = text(s);
    return d && d->bold == bold && near(d->x, x) && near(d->y, y) && near(d->cap, cap) && near(d->tracking, tracking) &&
           d->horz == 1 && d->vert == 1 && same(d->color, color);
}
static const Drawn *pic(float x, float y, float w, float h, const float *color) {
    for (int i = 0; i < drawCount; i++)
        if (drawn[i].kind == PIC && near(drawn[i].x, x) && near(drawn[i].y, y) && near(drawn[i].w, w) &&
            near(drawn[i].h, h) && same(drawn[i].color, color))
            return &drawn[i];
    return NULL;
}
/* A rectangle's four edges, line units wide. */
static int framed(float x, float y, float w, float h, float line, const float *color) {
    return pic(x, y, w, line, color) && pic(x, y + h - line, w, line, color) && pic(x, y, line, h, color) &&
           pic(x + w - line, y, line, h, color);
}
static int pictures(const char *name) {
    int n = 0;
    for (int i = 0; i < drawCount; i++)
        n += drawn[i].kind == PIC && drawn[i].material && !strcmp(drawn[i].material->name, name);
    return n;
}
static void paint(menuDef_t *m) {
    drawCount = 0;
    promptCalls = 0;
    washes = 0;
    g_webMenuBackdrop = 0.0f;
    UI_ClassMenu_Paint(&dc, m);
}
static int key(int k) { return UI_ClassMenu_KeyEvent(&dc, &classMenu, k, 1); }
/* The cursor over a point of a left-aligned element: 1.5 px a unit there,
 * 2 px a cursor unit across and 1.5 down. */
static void point(float x, float y) {
    dc.cursorx = (int)lroundf(x * 0.75f);
    dc.cursory = (int)lroundf(y);
}
static int chose(const char *response) {
    char expected[256];
    snprintf(expected, sizeof(expected), "cmd mr 77 3 %s\n", response);
    return !strcmp(command, expected) && !strcmp(closed, "browser_class") && !s_cac.open;
}
static float rowTop(int row) { return CAC_SELECT_ROW_TOP + (float)row * CAC_SELECT_ROW_PITCH; }
static float focusLeft(void) { return CAC_SELECT_ROW_LEFT + CAC_SELECT_FOCUS_INDENT; }

int main(void) {
    int cases = 0;

    /* The match opens the class menu: Select Class over the gray scene, on
     * the player's own classes, which start as the game's. */
    UI_ClassMenu_Open(&dc, 1);
    paint(&classMenu);
    assert(g_webMenuBackdrop == 1.0f && washes == 1);
    assert(drawn[0].kind == PIC && near(drawn[0].x, 0) && near(drawn[0].y, 0) && near(drawn[0].w, 640) &&
           near(drawn[0].h, 480) && drawn[0].horz == 4 && drawn[0].vert == 4 && same(drawn[0].color, selectShade) &&
           drawn[0].material == &white);
    assert(drawnText("SELECT CLASS", 1, CAC_SELECT_TITLE_LEFT, CAC_SELECT_TITLE_BASELINE, CAC_SELECT_TITLE_CAP,
                     CAC_SELECT_TITLE_TRACKING, gold));
    cases++;
    /* Custom chosen: filled cream from higher up, dark Bold centred; Standard
     * Issue framed, Regular in the idle gray. */
    {
        float h = CAC_SELECT_TAB_BOTTOM - CAC_SELECT_TAB_TOP, idleH = CAC_SELECT_TAB_BOTTOM - CAC_SELECT_TAB_IDLE_TOP;
        float second = CAC_SELECT_TAB_LEFT + CAC_SELECT_TAB_WIDTH + CAC_SELECT_TAB_GAP;
        float w = UI_WWIIMenu_Width("CUSTOM", 1, CAC_SELECT_TAB_CAP, CAC_SELECT_TAB_TRACKING);
        float idleW = UI_WWIIMenu_Width("STANDARD ISSUE", 0, CAC_SELECT_TAB_IDLE_CAP, CAC_SELECT_TAB_IDLE_TRACKING);
        assert(pic(CAC_SELECT_TAB_LEFT, CAC_SELECT_TAB_TOP, CAC_SELECT_TAB_WIDTH, h, tabFill));
        assert(drawnText("CUSTOM", 1, CAC_SELECT_TAB_LEFT + 0.5f * (CAC_SELECT_TAB_WIDTH - w),
                         CAC_SELECT_TAB_TOP + 0.5f * (h + CAC_SELECT_TAB_CAP), CAC_SELECT_TAB_CAP, CAC_SELECT_TAB_TRACKING,
                         tabText));
        assert(framed(second, CAC_SELECT_TAB_IDLE_TOP, CAC_SELECT_TAB_WIDTH, idleH, CAC_SELECT_LINE, tabEdge));
        assert(drawnText("STANDARD ISSUE", 0, second + 0.5f * (CAC_SELECT_TAB_WIDTH - idleW),
                         CAC_SELECT_TAB_IDLE_TOP + 0.5f * (idleH + CAC_SELECT_TAB_IDLE_CAP), CAC_SELECT_TAB_IDLE_CAP,
                         CAC_SELECT_TAB_IDLE_TRACKING, idle));
        cases++;
    }
    /* The five classes down the left in capitals: the first framed from the
     * screen's edge, in gold Bold further right, the others Regular. */
    {
        static const char *const names[] = { "ASSAULT", "SUBMACHINE GUNNER", "RIFLEMAN", "SNIPER", "CLOSE QUARTERS" };
        assert(framed(0, rowTop(0), CAC_SELECT_ROW_WIDTH, CAC_SELECT_ROW_HEIGHT, CAC_SELECT_LINE, rowEdge) &&
               pic(0, rowTop(0), CAC_SELECT_ROW_WIDTH, CAC_SELECT_ROW_HEIGHT, rowFill));
        assert(drawnText(names[0], 1, focusLeft(), rowTop(0) + CAC_SELECT_FOCUS_BASELINE, CAC_SELECT_FOCUS_CAP,
                         CAC_SELECT_FOCUS_TRACKING, gold));
        for (int i = 1; i < 5; i++)
            assert(drawnText(names[i], 0, CAC_SELECT_ROW_LEFT, rowTop(i) + CAC_SELECT_ROW_BASELINE, CAC_SELECT_ROW_CAP,
                             CAC_SELECT_ROW_TRACKING, idle));
        assert(!pic(0, rowTop(1), CAC_SELECT_ROW_WIDTH, CAC_SELECT_ROW_HEIGHT, rowFill));
        assert(text(offline) && near(text(offline)->y, rowTop(5) + 8.0f));
        cases++;
    }
    /* The focused class's loadout where the Division column was: each
     * slot's caption and weapon over its framed tile and picture; the guns'
     * tiles filled. No Division, no Basic Training. */
    {
        const float x = CAC_SELECT_LOADOUT_LEFT, x2 = x + CAC_SELECT_LOADOUT_COLUMN;
        static const struct { const char *caption, *name, *image; int column; float top, size; int filled; } slots[] = {
            { "PRIMARY", "MP44", "weapon_mp44", 0, CAC_SELECT_PRIMARY_TOP, CAC_SELECT_PRIMARY_SIZE, 1 },
            { "SECONDARY", "LUGER", "gfx/icons/hud@luger.tga", 0, CAC_SELECT_SECONDARY_TOP, CAC_SELECT_SECONDARY_SIZE, 1 },
            { "LETHAL", "GERMANGRENADE", "gfx/icons/hud@steilhandgrenate.tga", 0, CAC_SELECT_EQUIPMENT_TOP,
              CAC_SELECT_EQUIPMENT_SIZE, 0 },
            { "TACTICAL", "NEBELHANDGRANATE", "hud_us_smokegrenade", 1, CAC_SELECT_EQUIPMENT_TOP,
              CAC_SELECT_EQUIPMENT_SIZE, 0 } };
        for (int i = 0; i < 4; i++) {
            float left = slots[i].column ? x2 : x, top = slots[i].top, size = slots[i].size;
            assert(drawnText(slots[i].caption, 0, left, top - CAC_SELECT_CAPTION_ABOVE, CAC_SELECT_CAPTION_CAP,
                             CAC_SELECT_CAPTION_TRACKING, note));
            assert(drawnText(slots[i].name, 1, left, top - CAC_SELECT_NAME_ABOVE, CAC_SELECT_NAME_CAP,
                             CAC_SELECT_NAME_TRACKING, title));
            assert(framed(left, top, size, size, CAC_SELECT_LINE, edge));
            assert(!!pic(left, top, size, size, tileFill) == slots[i].filled);
            assert(pictures(slots[i].image) == 1);
        }
        assert(!text("DIVISION") && !text("BASIC TRAINING") && !text("INFANTRY") && !text("BLITZKRIEG"));
        cases += 2;
    }
    /* Enter selects, F1 edits a class of the player's own, Escape goes back,
     * in a row lower and further in than the main menu's. */
    assert(promptCalls == 1 && promptCount == 2 && !strcmp(promptKeys[0], "ENTER") && !strcmp(promptLabels[0], "Select") &&
           !strcmp(promptKeys[1], "F1") && !strcmp(promptLabels[1], "Edit") && !strcmp(promptRightKey, "ESC") &&
           !strcmp(promptRightLabel, "Back") && near(promptLeft, CAC_SELECT_PROMPT_LEFT) &&
           near(promptRight, CAC_SELECT_PROMPT_RIGHT) && near(promptLower, CAC_SELECT_PROMPT_LOWER));
    cases++;

    /* Standard Issue lists the game's classes, whatever the player made of
     * theirs; it has no edit prompt and no storage note. */
    assert(key(0x9d) == 1 && s_cac.tab == CAC_TAB_STANDARD);
    paint(&classMenu);
    assert(text("STANDARD ISSUE")->bold && same(text("STANDARD ISSUE")->color, tabText) && !text("CUSTOM")->bold);
    assert(promptCount == 1 && !text(offline));
    UI_ClassMenu_Loaded(s_cac.generation, 200,
                        "Kar Rusher\tkar98k_mp\tTT30_mp\tfrag_grenade_russian_mp\tsmoke_grenade_british_mp\n"
                        "Night Owl\tspringfield_mp\twebley_mp\tfrag_grenade_american_mp\tsmoke_grenade_german_mp\n"
                        "Trench\tshotgun_mp\tcolt_mp\tfrag_grenade_british_mp\tsmoke_grenade_russian_mp\n"
                        "Rat\tppsh_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_american_mp\n"
                        "Bolt\tenfield_mp\tTT30_mp\tfrag_grenade_russian_mp\tsmoke_grenade_russian_mp\n");
    paint(&classMenu);
    assert(text("ASSAULT") && text("CLOSE QUARTERS") && !text("KAR RUSHER") && text("MP44"));
    cases++;
    /* Left, the other way, and Tab bring the player's classes back. */
    assert(key(0x9c) == 1 && s_cac.tab == CAC_TAB_CUSTOM);
    paint(&classMenu);
    assert(text("CUSTOM")->bold && text("KAR RUSHER") && text("BOLT") && !text("ASSAULT") && text("KAR98K") &&
           text("TT30") && text("RUSSIANGRENADE") && text("NO77_WP_SMOKE_GRENADE") && !text(offline));
    assert(key(0x9c) == 1 && s_cac.tab == CAC_TAB_CUSTOM && key(0x09) == 1 && s_cac.tab == CAC_TAB_STANDARD &&
           key(0x09) == 1 && s_cac.tab == CAC_TAB_CUSTOM && key(0x9d) == 1 && key(0x9d) == 1 &&
           s_cac.tab == CAC_TAB_STANDARD);
    cases++;

    /* Enter sends the focused class of the shown tab as the class menu's
     * response and closes it; reopened, it shows that tab and class. */
    commands = 0;
    assert(key(0x0d) == 1 && commands == 1);
    assert(chose("class,mp44_mp,luger_mp,frag_grenade_german_mp,smoke_grenade_german_mp,none,none,grip+extmag,none"));
    UI_ClassMenu_Open(&dc, 1);
    assert(s_cac.tab == CAC_TAB_STANDARD && s_cac.classFocus == 0);
    /* Looking at the other tab and backing out leaves the choice as it was. */
    key(0x9c);
    key(0x9b);
    assert(key(0x1b) == 1 && !s_cac.open);
    UI_ClassMenu_Open(&dc, 1);
    assert(s_cac.tab == CAC_TAB_STANDARD && s_cac.classFocus == 0);
    key(0x9b);
    key(0x9b);
    assert(key(0xbf) == 1 && chose("class,m1garand_mp,colt_mp,frag_grenade_american_mp,smoke_grenade_american_mp,none,none,"
                                     "quickdraw+extmag,none"));
    UI_ClassMenu_Open(&dc, 1);
    assert(s_cac.tab == CAC_TAB_STANDARD && s_cac.classFocus == 2);
    /* A number chooses that class of the tab at once. */
    assert(key(0x400 | '4') == 1 && chose("class,kar98k_sniper_mp,luger_mp,frag_grenade_german_mp,smoke_grenade_german_mp,"
                                                "none,none,fmj+extmag,none"));
    UI_ClassMenu_Open(&dc, 1);
    key(0x9c);
    assert(key(0x400 | '1') == 1 && chose("class,kar98k_mp,TT30_mp,frag_grenade_russian_mp,smoke_grenade_british_mp"));
    UI_ClassMenu_Open(&dc, 1);
    assert(s_cac.tab == CAC_TAB_CUSTOM && s_cac.classFocus == 0);
    key(0x9a);
    assert(key(0x0d) == 1 && chose("class,enfield_mp,TT30_mp,frag_grenade_russian_mp,smoke_grenade_russian_mp"));
    cases += 3;

    /* F1 edits a class of the player's own and Escape returns to the list,
     * still over the gray scene; the game's classes cannot be edited. */
    UI_ClassMenu_Open(&dc, 1);
    key(0x9d);
    assert(key(0xa7) == 1 && s_cac.screen == CAC_SCREEN_LIST);
    key(0x9c);
    key(0x9b);
    assert(key(0xa7) == 1 && s_cac.screen == CAC_SCREEN_EDIT && !s_cac.renaming);
    paint(&classMenu);
    assert(text("MODIFY KAR RUSHER") && g_webMenuBackdrop == 1.0f && !washes && same(drawn[0].color, shade));
    assert(key(0x1b) == 1 && s_cac.screen == CAC_SCREEN_LIST && s_cac.open);
    paint(&classMenu);
    assert(text("SELECT CLASS") && washes == 1);
    cases += 2;

    /* The pointer focuses the row it is over now, not the one it left, and
     * clicks choose it; over a tab it leaves the focus, and a click shows
     * that tab. */
    point(100, rowTop(2) + 14);
    paint(&classMenu);
    assert(s_cac.classFocus == 2);
    point(100, rowTop(3) + 14);
    paint(&classMenu);
    assert(s_cac.classFocus == 3);
    paint(&classMenu);
    assert(s_cac.classFocus == 3 && drawnText("RAT", 1, focusLeft(), rowTop(3) + CAC_SELECT_FOCUS_BASELINE,
                                               CAC_SELECT_FOCUS_CAP, CAC_SELECT_FOCUS_TRACKING, gold));
    point(CAC_SELECT_TAB_LEFT + CAC_SELECT_TAB_WIDTH + CAC_SELECT_TAB_GAP + 40, CAC_SELECT_TAB_BOTTOM - 4);
    paint(&classMenu);
    point(CAC_SELECT_TAB_LEFT + CAC_SELECT_TAB_WIDTH + CAC_SELECT_TAB_GAP + 44, CAC_SELECT_TAB_BOTTOM - 4);
    paint(&classMenu);
    assert(s_cac.classFocus == 3 && s_cac.tab == CAC_TAB_CUSTOM);
    commands = 0;
    assert(key(0xc8) == 1 && s_cac.tab == CAC_TAB_STANDARD && !commands && s_cac.open);
    point(60, rowTop(1) + 10);
    paint(&classMenu);
    assert(key(0xc8) == 1 && chose("class,thompson_mp,colt_mp,frag_grenade_american_mp,smoke_grenade_american_mp,none,none,"
                                     "steadyaim+extmag,none"));
    /* Reopened with the pointer over another row, it keeps the class chosen
     * until the pointer moves. */
    point(100, rowTop(4) + 14);
    UI_ClassMenu_Open(&dc, 1);
    paint(&classMenu);
    assert(s_cac.classFocus == 1 && s_cac.tab == CAC_TAB_STANDARD);
    cases += 3;

    /* Escape closes it without a choice. */
    UI_ClassMenu_Open(&dc, 1);
    closed[0] = 0;
    commands = 0;
    assert(key(0x1b) == 1 && !strcmp(closed, "browser_class") && !s_cac.open && !commands);
    cases++;

    /* A name too wide for its row shrinks to fit it. */
    UI_ClassMenu_Open(&dc, 1);
    key(0x9c);
    strcpy(s_cac.classes[0].name, "WWWWWWWWWWWWWWWWWWWW");
    strcpy(s_cac.classes[1].name, "MMMMMMMMMMMMMMMMMMMM");
    while (s_cac.classFocus)
        key(0x9a);
    paint(&classMenu);
    {
        const Drawn *wide = text("WWWWWWWWWWWWWWWWWWWW"), *other = text("MMMMMMMMMMMMMMMMMMMM");
        float room = CAC_SELECT_ROW_WIDTH - 4.0f - focusLeft();
        assert(wide && wide->cap < CAC_SELECT_FOCUS_CAP &&
               UI_WWIIMenu_Width(wide->text, 1, wide->cap, wide->tracking) <= room + 1e-3f);
        room = CAC_SELECT_ROW_WIDTH - 4.0f - CAC_SELECT_ROW_LEFT;
        assert(other && other->cap < CAC_SELECT_ROW_CAP &&
               UI_WWIIMenu_Width(other->text, 0, other->cap, other->tracking) <= room + 1e-3f);
        assert(text("BOLT")->cap == CAC_SELECT_ROW_CAP);
        cases++;
    }

    /* Create a Class in the main menu keeps its own list of the player's
     * classes, over the menu's backdrop: no scene to blur. */
    s_cac.tab = CAC_TAB_STANDARD;
    UI_ClassMenu_Open(&dc, 0);
    drawCount = 0;
    g_webMenuBackdrop = 0.0f;
    washes = 0;
    UI_ClassMenu_Paint(&dc, &createMenu);
    assert(g_webMenuBackdrop == 0.0f && !washes && same(drawn[0].color, shade) && text("CREATE A CLASS") &&
           text("Trench") && !text("SELECT CLASS") && !text("Assault"));
    cases++;

    printf("%d\n", cases);
    return 0;
}
'''

mutants = [
    ('    return s_cac.choose && s_cac.tab == CAC_TAB_STANDARD ? s_cac.standard : s_cac.classes;',
     '    return s_cac.classes;'),
    ('        ClassData_Defaults(s_cac.standard);\n', ''),
    ('    if (kind == 2)\n        g_webMenuBackdrop = 1.0f;', '    if (kind)\n        g_webMenuBackdrop = 1.0f;'),
    ('    if (kind == 2)\n        g_webMenuBackdrop = 1.0f;', '    if (0)\n        g_webMenuBackdrop = 1.0f;'),
    ('s_cac.tab = key == CAC_KEY_RIGHT ? CAC_TAB_STANDARD : CAC_TAB_CUSTOM;',
     's_cac.tab = key == CAC_KEY_LEFT ? CAC_TAB_STANDARD : CAC_TAB_CUSTOM;'),
    ('            if (s_cac.tab == CAC_TAB_CUSTOM)\n                s_cac.screen = CAC_SCREEN_EDIT;',
     '            s_cac.screen = CAC_SCREEN_EDIT;'),
    ('    s_cac.chosenTab = s_cac.tab;\n', ''),
    ('        s_cac.tab = s_cac.chosenTab;\n', ''),
    ('            s_cac.tab = s_cac.hover - CAC_HOVER_TAB;', '            s_cac.tab = CAC_TAB_CUSTOM;'),
    ('        if (s_cac.hover < CLASS_COUNT)\n            s_cac.classFocus = s_cac.hover;',
     '        if (s_cac.hover >= 0)\n            s_cac.classFocus = s_cac.hover % CLASS_COUNT;'),
    # The focus from the hover the last frame painted, where the pointer was.
    ('    s_cac.hover = -1;\n    Cac_PaintScreen(dc);',
     '    if (moved && !s_cac.renaming)\n        Cac_FocusHover();\n    moved = 0;\n    s_cac.hover = -1;\n'
     '    Cac_PaintScreen(dc);'),
    ('    moved = dc->cursorx != s_cac.cursorX || dc->cursory != s_cac.cursorY;',
     '    moved = dc->cursorx != s_cac.cursorX;'),
    ('    s_cac.hover = -1;\n    s_cac.cursorX = dc->cursorx;\n    s_cac.cursorY = dc->cursory;\n    if (choose',
     '    s_cac.hover = -1;\n    if (choose'),
    ('s_cac.tab == CAC_TAB_CUSTOM ? 2 : 1, "ESC", "Back"', '2, "ESC", "Back"'),
    ('    if (s_cac.tab == CAC_TAB_CUSTOM)\n        Cac_StatusNote(', '    if (1)\n        Cac_StatusNote('),
    ('    if (!Cac_Respond(&Cac_Listed()[s_cac.classFocus]))', '    if (!Cac_Respond(&s_cac.classes[s_cac.classFocus]))'),
    ('        if (s_cac.choose) {\n            Cac_PaintSelect(dc);\n            return;\n        }\n', ''),
    ('    if (s_cac.choose && s_cac.screen == CAC_SCREEN_LIST) {\n        CL_DrawStretchPic',
     '    if (0) {\n        CL_DrawStretchPic'),
    ('        UI_WWIIMenu_PaintWash();\n', ''),
    ('    if (slot <= CLASS_SLOT_SECONDARY)\n        Cac_Fill(&tile, s_cacSelectTile);', '    Cac_Fill(&tile, s_cacSelectTile);'),
    ('    Cac_SelectTile(right, CAC_SELECT_EQUIPMENT_TOP', '    Cac_SelectTile(CAC_SELECT_LOADOUT_LEFT, CAC_SELECT_EQUIPMENT_TOP'),
    ('    return wide > width && wide > 0.0f ? cap * width / wide : cap;', '    return cap;'),
    ('            left += CAC_SELECT_FOCUS_INDENT;\n', ''),
    ('    else\n        Cac_FrameWidth(tab, s_cacSelectTabEdge, CAC_SELECT_LINE);', ''),
    ('chosen ? s_cacSelectTabText : UI_WWIIMenu_Color(WWII_COLOR_IDLE)', 'UI_WWIIMenu_Color(WWII_COLOR_IDLE)'),
    ('        float top = chosen ? CAC_SELECT_TAB_TOP : CAC_SELECT_TAB_IDLE_TOP;', '        float top = CAC_SELECT_TAB_TOP;'),
    ('             tab->y + 0.5f * (tab->h + cap), tab->horz, cap, tracking,',
     '             tab->y + tab->h, tab->horz, cap, tracking,'),
    ('static const vec4_t s_cacSelectShade = { 0.0f, 0.0f, 0.0f, 0.3f };',
     'static const vec4_t s_cacSelectShade = { 0.0f, 0.0f, 0.0f, 0.55f };'),
    # The layout's numbers against the capture's.
    ('#define CAC_SELECT_TITLE_TRACKING 0.24f', '#define CAC_SELECT_TITLE_TRACKING 0.18f'),
    ('#define CAC_SELECT_TITLE_BASELINE 51.5f', '#define CAC_SELECT_TITLE_BASELINE 84.0f'),
    ('#define CAC_SELECT_ROW_TRACKING 0.19f', '#define CAC_SELECT_ROW_TRACKING 0.25f'),
    ('#define CAC_SELECT_ROW_PITCH 29.2f', '#define CAC_SELECT_ROW_PITCH 33.3f'),
    ('#define CAC_SELECT_FOCUS_INDENT 8.0f', '#define CAC_SELECT_FOCUS_INDENT 6.0f'),
    ('#define CAC_SELECT_TAB_IDLE_TOP 70.8f', '#define CAC_SELECT_TAB_IDLE_TOP 68.3f'),
    ('#define CAC_SELECT_LOADOUT_LEFT 250.0f', '#define CAC_SELECT_LOADOUT_LEFT 386.7f'),
    ('#define CAC_SELECT_PRIMARY_SIZE 122.5f', '#define CAC_SELECT_PRIMARY_SIZE 100.0f'),
    ('#define CAC_SELECT_NAME_TRACKING 0.08f', '#define CAC_SELECT_NAME_TRACKING 0.2f'),
    ('#define CAC_SELECT_PROMPT_LOWER 20.9f', '#define CAC_SELECT_PROMPT_LOWER 0.0f'),
]


def strip(source):
    return re.sub(r'^#include "[^"]+"\n', '', source, flags=re.M)


def program(menu_source):
    return support + strip(menu_source) + checks


# The class data module as it is built, with the first-party sources its
# headers name.
data_sources = [root / 'src/PC/ui_mp/ui_class_data.c']
for header in re.findall(r'^#include "headers/(PC/[^"]+)\.h"', data, re.M):
    source = root / 'src' / (header + '.c')
    if source.exists() and source not in data_sources:
        data_sources.append(source)
flags = ['-std=gnu99', '-O1', '-g', '-I', str(root / 'src'), '-fsanitize=address,undefined', '-fno-sanitize-recover=all']


def run(directory, name, text, warnings, expect_pass, objects):
    path = Path(directory) / f'{name}.c'
    binary = Path(directory) / name
    path.write_text(text)
    subprocess.run(['cc', *flags, *warnings, str(path), *objects, '-lm', '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert (result.returncode == 0) == expect_pass, (name, result.stdout[-500:], result.stderr[-3000:])
    return result


strict = ['-Wall', '-Werror', '-Wno-unused-function']
with tempfile.TemporaryDirectory(prefix='cod2-class-select-') as directory:
    objects = []
    for source in data_sources:
        objects.append(str(Path(directory) / (source.stem + '.o')))
        subprocess.run(['cc', *flags, '-w', '-c', str(source), '-o', objects[-1]], check=True)
    cases = int(run(directory, 'select', program(menu), strict, True, objects).stdout)
    for index, (old, new) in enumerate(mutants):
        assert menu.count(old) == 1, old
        variant = menu.replace(old, new)
        if evidence(variant):
            continue
        run(directory, f'select{index}', program(variant), ['-w'], False, objects)

print('PASS: a match\'s class menu is WWII\'s Select Division without its Division and Basic Training: the gold '
      'title, Custom and Standard Issue tabs, the tab\'s classes down the left with the focused one framed in gold '
      'Bold, and that class\'s primary, secondary, lethal and tactical over their tiles; Standard Issue lists the '
      'game\'s own classes whatever the player made of theirs; Enter, a number or a click sends the shown class, '
      'reopening on it; only the player\'s classes can be edited; it asks for the match blurred and gray behind it, '
      'which Create a Class does not; '
      f'{cases} cases; the layout\'s numbers within the capture\'s measurements; {len(mutants)} failing regression '
      'mutants')
