#!/usr/bin/env python3
"""Run Create a Class's Equip Weapon against a recording UI: the slot's
weapons one kind at a time under a tab for each of its kinds (the primary's
submachine guns, rifles, automatic rifles, sniper rifles and shotguns, the
secondary's pistols); left, right, Tab and a click on a tab showing another
kind, on the class's gun when it is one of them; up, down and the wheel going
around the kind's guns with its page counter; the pointer focusing a row and
leaving the focus over a tab; a gun of another kind equipped and saved;
Modify Equipment's rows as before; and each kind's label within its tab in
Barlow's metrics."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(root / 'scripts'))
from barlow_metrics import Font  # noqa: E402

menu = (root / 'src/PC/ui_mp/ui_class_menu.c').read_text()
data = (root / 'src/PC/ui_mp/ui_class_data.c').read_text()
# The menu's first-party headers, in its order, but the renderer's: their
# few functions are stubbed below.
headers = ''.join((root / 'src/headers' / header).read_text()
                  for header in re.findall(r'^#include "headers/(PC/[^"]+\.h)"', menu, re.M)
                  if not header.startswith('PC/gfx_d3d/'))

Regular, Bold = Font('Regular'), Font('Bold')


def evidence(text):
    """Each kind's label inside its tab with 4 to spare at either side,
    chosen in Bold or idle in Regular: the failures."""
    failures = []

    def define(name):
        match = re.search(r'#define ' + name + r' (-?[\d.]+)f', text)
        return float(match[1]) if match else float('nan')

    table = re.search(r's_cacGroupTabs\[\] = \{([^}]*)\}', text)
    labels = re.findall(r'"([^"]+)"', table[1]) if table else []
    if not labels:
        failures.append('no labels')
    room = define('CAC_SELECT_TAB_WIDTH') - 8.0
    for label in labels:
        for font, cap, tracking in ((Bold, define('CAC_SELECT_TAB_CAP'), define('CAC_SELECT_TAB_TRACKING')),
                                    (Regular, define('CAC_SELECT_TAB_IDLE_CAP'),
                                     define('CAC_SELECT_TAB_IDLE_TRACKING'))):
            if not font.ink_width(label, tracking) * cap <= room:
                failures.append(label)
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
typedef struct { int kind; char text[96]; int bold; float x, y, w, h, cap; vec4_t color; MaterialHandle material; } Drawn;
static Drawn drawn[512];
static int drawCount;
static Drawn *record(int kind) {
    Drawn *d;
    assert(drawCount < 512);
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
    (void)horzAlign; (void)vertAlign; (void)tracking;
    snprintf(d->text, sizeof(d->text), "%s", text);
    d->bold = bold; d->x = x; d->y = baseline; d->cap = cap;
    memcpy(d->color, color, sizeof(d->color));
}
float UI_WWIIMenu_Width(const char *text, int bold, float cap, float tracking) {
    return (float)strlen(text) * cap * (bold ? 0.7f : 0.6f) + tracking;
}
static int promptCount;
static const char *promptKeys[4], *promptLabels[4];
void UI_WWIIMenu_PromptsAt(const char *const *keys, const char *const *labels, int count, const char *rightKey,
                           const char *rightLabel, float left, float right, float lower) {
    assert(count >= 0 && count <= 4 && !strcmp(rightKey, "ESC"));
    (void)rightLabel; (void)left; (void)right; (void)lower;
    promptCount = count;
    for (int i = 0; i < count; i++) { promptKeys[i] = keys[i]; promptLabels[i] = labels[i]; }
}
void UI_WWIIMenu_Prompts(const char *const *keys, const char *const *labels, int count, const char *rightKey,
                         const char *rightLabel) {
    UI_WWIIMenu_PromptsAt(keys, labels, count, rightKey, rightLabel, -1.0f, -1.0f, 0.0f);
}
void UI_WWIIMenu_PaintWash(void) {}
static Material white = { "white" };
sharedUiInfo_t sharedUiInfo = { { &white } };
int R_GetHudImageGeneration(void) { return 0; }
MaterialHandle R_GetHudSilhouetteMaterial(MaterialHandle icon, float *aspect) { (void)icon; *aspect = 0.0f; return NULL; }
/* No owner pictures and no camouflage: the retail images and icons. */
MaterialHandle R_GetHudPictureMaterial(const char *name, float *st, float *aspect) {
    (void)name; (void)st;
    *aspect = 0.0f;
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
    (void)horzAlign; (void)vertAlign; (void)s1; (void)t1; (void)s2; (void)t2;
    d->x = x; d->y = y; d->w = w; d->h = h; d->material = material;
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
int Dvar_GetInt(const char *name) { (void)name; return 0; }
/* Create a Class sends no class to a server. */
void Cbuf_ExecuteText(int when, const char *text) { (void)when; (void)text; assert(0); }
const char *CL_GetConfigString(int index) { (void)index; return ""; }
char *va(const char *format, ...) {
    static char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return buffer;
}
void Menus_CloseByName(displayContextDef_t *dc, const char *name) { (void)dc; (void)name; }
qboolean Menus_OpenByName(displayContextDef_t *dc, const char *name) { (void)dc; (void)name; return 1; }
int I_stricmp(const char *s0, const char *s1) { return strcasecmp(s0, s1); }
/* WEAPON_MP44 reads MP44; other references read as they are. */
const char *UI_SafeTranslateString(const char *reference) {
    return !strncmp(reference, "WEAPON_", 7) ? reference + 7 : reference;
}
float g_webMenuBackdrop;
void UI_PauseMenu_Return(displayContextDef_t *dc) { (void)dc; }
static int sounds;
static const char *lastSound = "";
int UI_PlayLocalSoundAliasByName(const char *name) {
    assert(!strcmp(name, "mouse_over") || !strcmp(name, "mouse_click"));
    sounds++;
    lastSound = name;
    return 1;
}
/* The browser's host: the lines saved. */
static int saves;
static char saved[1024];
void Web_ClassesLoad(const char *player, int generation) { (void)player; (void)generation; }
void Web_ClassesSave(const char *player, const char *lines, int generation) {
    (void)generation;
    assert(!strcmp(player, "Tester"));
    snprintf(saved, sizeof(saved), "%s", lines);
    saves++;
}
/* The menu as the browser builds it, saving through the host. */
#define __EMSCRIPTEN__ 1
'''

checks = r'''
static menuDef_t createMenu = { { "create_a_class" } };
static displayContextDef_t dc = { 1000, 0, 0 };
static const float gold[4] = { 0.80f, 0.72f, 0.47f, 1 }, idle[4] = { 0.776f, 0.776f, 0.769f, 1 };
static const float tabFill[4] = { 0.831f, 0.812f, 0.769f, 1 }, tabText[4] = { 0.09f, 0.09f, 0.09f, 1 };
static const float tabEdge[4] = { 1, 1, 1, 0.55f };
static const float rowFill[4] = { 0, 0, 0, 0.45f }, rowFocus[4] = { 0.32f, 0.29f, 0.22f, 0.55f };
static const char *const kinds[] = { "SUBMACHINE GUNS", "RIFLES", "AUTOMATIC RIFLES", "SNIPER RIFLES", "SHOTGUNS" };
static const char *const pistolKind[] = { "PISTOLS" };
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
static int texts(const char *s) {
    int n = 0;
    for (int i = 0; i < drawCount; i++)
        n += drawn[i].kind == TEXT && !strcmp(drawn[i].text, s);
    return n;
}
static const Drawn *textAt(float x, float y) {
    for (int i = 0; i < drawCount; i++)
        if (drawn[i].kind == TEXT && near(drawn[i].x, x) && near(drawn[i].y, y))
            return &drawn[i];
    return NULL;
}
static int fill(float x, float y, float w, float h, const float *color) {
    for (int i = 0; i < drawCount; i++)
        if (drawn[i].kind == PIC && drawn[i].material == &white && near(drawn[i].x, x) && near(drawn[i].y, y) &&
            near(drawn[i].w, w) && near(drawn[i].h, h) && same(drawn[i].color, color))
            return 1;
    return 0;
}
static int framed(float x, float y, float w, float h, float line, const float *color) {
    return fill(x, y, w, line, color) && fill(x, y + h - line, w, line, color) && fill(x, y, line, h, color) &&
           fill(x + w - line, y, line, h, color);
}
static void paint(void) {
    drawCount = 0;
    promptCount = -1;
    UI_ClassMenu_Paint(&dc, &createMenu);
}
static int key(int k) { return UI_ClassMenu_KeyEvent(&dc, &createMenu, k, 1); }
/* The cursor over a point of a left-aligned element: 1.5 px a unit there,
 * 2 px a cursor unit across and 1.5 down. */
static void point(float x, float y) {
    dc.cursorx = (int)lroundf(x * 0.75f);
    dc.cursory = (int)lroundf(y);
}
/* Whether the list's focus is on the weapon, among its slot's candidates. */
static int focused(const char *weapon) {
    int index = ClassData_Find(weapon), indexes[64];
    int count = ClassData_SlotWeapons(g_classWeapons[index].slot, indexes, 64);
    return s_cac.pickFocus >= 0 && s_cac.pickFocus < count && indexes[s_cac.pickFocus] == index;
}
static float tabX(int tab) { return CAC_LEFT + (float)tab * (CAC_SELECT_TAB_WIDTH + CAC_SELECT_TAB_GAP); }
static float rowTop(int row) { return CAC_PICK_TOP + (float)row * CAC_PICK_ROW; }
/* The kinds' tabs from the title's left in order: the one at chosen filled
 * from higher up with its label in dark Bold, the others framed with theirs
 * in the idle Regular, each label centred. */
static int tabs(const char *const *labels, int count, int chosen) {
    for (int i = 0; i < count; i++) {
        int on = i == chosen;
        float top = on ? CAC_PICK_TAB_TOP : CAC_PICK_TAB_IDLE_TOP, h = CAC_PICK_TAB_BOTTOM - top;
        float cap = on ? CAC_SELECT_TAB_CAP : CAC_SELECT_TAB_IDLE_CAP;
        float tracking = on ? CAC_SELECT_TAB_TRACKING : CAC_SELECT_TAB_IDLE_TRACKING;
        const Drawn *label = text(labels[i]);

        if (on ? !fill(tabX(i), top, CAC_SELECT_TAB_WIDTH, h, tabFill)
               : !framed(tabX(i), top, CAC_SELECT_TAB_WIDTH, h, CAC_SELECT_LINE, tabEdge))
            return 0;
        if (on && fill(tabX(i), CAC_PICK_TAB_IDLE_TOP, CAC_SELECT_TAB_WIDTH, CAC_PICK_TAB_BOTTOM - CAC_PICK_TAB_IDLE_TOP,
                       tabFill))
            return 0;
        if (!label || label->bold != on || !same(label->color, on ? tabText : idle) || !near(label->cap, cap) ||
            !near(label->x, tabX(i) + 0.5f * (CAC_SELECT_TAB_WIDTH - UI_WWIIMenu_Width(labels[i], on, cap, tracking))) ||
            !near(label->y, top + 0.5f * (h + cap)))
            return 0;
    }
    return 1;
}
static int equippedAt(float y) {
    for (int i = 0; i < drawCount; i++)
        if (drawn[i].kind == TEXT && !strcmp(drawn[i].text, "EQUIPPED") && near(drawn[i].y, y))
            return 1;
    return 0;
}
/* The list's rows from its top, and no others: the one at focus filled
 * warmer, framed in gold and in Bold; EQUIPPED on the one at equipped, or on
 * none for -1. */
static int listed(const char *const *names, int count, int focus, int equipped) {
    int shown = 0;

    for (int i = 0; i < drawCount; i++)
        shown += drawn[i].kind == TEXT && near(drawn[i].x, CAC_LEFT + 16.0f) && drawn[i].y > CAC_PICK_TOP;
    if (shown != count || texts("EQUIPPED") != (equipped >= 0))
        return 0;
    for (int row = 0; row < count; row++) {
        const Drawn *name = textAt(CAC_LEFT + 16.0f, rowTop(row) + 15.5f);

        if (!name || strcmp(name->text, names[row]) || name->bold != (row == focus))
            return 0;
        if (!fill(0.0f, rowTop(row), CAC_PICK_WIDTH, CAC_PICK_HEIGHT, row == focus ? rowFocus : rowFill))
            return 0;
        if ((row == focus) != framed(0.0f, rowTop(row), CAC_PICK_WIDTH, CAC_PICK_HEIGHT, CAC_LINE, gold))
            return 0;
        if ((row == equipped) != equippedAt(rowTop(row) + 15.0f))
            return 0;
    }
    return 1;
}
/* The page counter under the list's rows. */
static const char *page(void) {
    const Drawn *d = textAt(CAC_LEFT, CAC_PICK_TOP + (float)CAC_PICK_VISIBLE * CAC_PICK_ROW + 10.0f);
    return d ? d->text : "";
}

int main(void) {
    static const char *const smgs[] = { "THOMPSON", "GREASEGUN", "STEN", "PPS42", "PPSH", "MP40" };
    static const char *const rifles[] = { "M1GARAND", "M1A1CARBINE", "LEEENFIELD", "MOSINNAGANT", "SVT40", "KAR98K",
                                          "G43" };
    static const char *const automatics[] = { "BAR", "BREN", "MP44" };
    static const char *const snipers[] = { "SPRINGFIELD", "SCOPEDLEEENFIELD", "SCOPEDMOSINNAGANT", "SCOPEDKAR98K" };
    static const char *const shotguns[] = { "SHOTGUN" };
    static const char *const pistols[] = { "COLT45", "WEBLEY", "TT30", "LUGER" };
    static const char line[] = "Assault\tkar98k_mp\twebley_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\tnone\tnone"
                               "\t\t\n";
    int cases = 0;

    /* A label for each of the guns' kinds; the tabs between the title and the
     * list, clear of both, and within a 4:3 screen. */
    assert(sizeof(s_cacGroupTabs) / sizeof(s_cacGroupTabs[0]) == CLASS_GROUP_PISTOL + 1);
    assert(CAC_PICK_TAB_TOP >= CAC_TITLE_BASELINE + 8.0f && CAC_PICK_TAB_BOTTOM + 10.0f <= CAC_PICK_TOP);
    assert(tabX(4) + CAC_SELECT_TAB_WIDTH <= 600.0f);
    cases++;

    UI_ClassMenu_Open(&dc, 0);
    UI_ClassMenu_Loaded(s_cac.generation, 200,
                        "Assault\tmp44_mp\twebley_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\tnone\tnone"
                        "\tgrip+extmag\t\n");
    assert(s_cac.store == CAC_STORE_READY && ClassData_AttachmentCount(&s_cac.classes[0], CLASS_SLOT_PRIMARY) == 2);
    assert(key(0x0d) && s_cac.screen == CAC_SCREEN_EDIT && s_cac.slotFocus == CLASS_SLOT_PRIMARY);

    /* The primary's list opens on the MP44's kind: a tab for each of the five
     * kinds in the table's order, Automatic Rifles chosen, and its three guns
     * only, the MP44 focused and equipped, 3 of 3. */
    assert(key(0x0d) && s_cac.screen == CAC_SCREEN_WEAPONS && focused("mp44_mp"));
    paint();
    assert(text("EQUIP WEAPON | PRIMARY") && tabs(kinds, 5, 2) && !text("PISTOLS"));
    assert(listed(automatics, 3, 2, 2) && !strcmp(page(), "3/3"));
    assert(!text("THOMPSON") && !text("M1GARAND") && !text("SPRINGFIELD") && !text("SHOTGUN"));
    assert(promptCount == 1 && !strcmp(promptKeys[0], "ENTER") && !strcmp(promptLabels[0], "Equip"));
    cases += 2;

    /* Up, down and the wheel go around the kind's guns and no further, each
     * move with mouse_over. */
    sounds = 0;
    assert(key(0x9b) && focused("bar_mp") && sounds == 1 && !strcmp(lastSound, "mouse_over"));
    paint();
    assert(listed(automatics, 3, 0, 2) && !strcmp(page(), "1/3"));
    assert(key(0x9a) && focused("mp44_mp") && key(0x9a) && focused("bren_mp"));
    assert(key(0xcd) && focused("mp44_mp") && key(0xce) && focused("bren_mp") && sounds == 5);
    cases++;

    /* Right shows the next kind on its first gun, the MP44 being none of
     * them, and the details follow; on to the last kind and around to the
     * first; left goes back around, Tab to the next. */
    assert(key(0x9d) && focused("springfield_mp") && sounds == 6 && !strcmp(lastSound, "mouse_over"));
    paint();
    assert(tabs(kinds, 5, 3) && listed(snipers, 4, 0, -1) && !strcmp(page(), "1/4"));
    assert(textAt(CAC_DETAIL_LEFT, 135.0f) && !strcmp(textAt(CAC_DETAIL_LEFT, 135.0f)->text, "SPRINGFIELD"));
    assert(key(0x9d) && focused("shotgun_mp"));
    paint();
    assert(tabs(kinds, 5, 4) && listed(shotguns, 1, 0, -1) && !strcmp(page(), "1/1"));
    /* In a kind of one gun, up and down change nothing, silently. */
    sounds = 0;
    assert(key(0x9b) && key(0x9a) && focused("shotgun_mp") && !sounds);
    assert(key(0x9d) && focused("thompson_mp"));
    paint();
    assert(tabs(kinds, 5, 0) && listed(smgs, 6, 0, -1) && !strcmp(page(), "1/6"));
    assert(key(0x9c) && focused("shotgun_mp") && key(0x09) && focused("thompson_mp"));
    assert(key(0x09) && focused("m1garand_mp"));
    paint();
    assert(tabs(kinds, 5, 1) && listed(rifles, 7, 0, -1) && !strcmp(page(), "1/7"));
    cases += 3;

    /* Back on the automatic rifles, the list is on the class's MP44 again. */
    for (int i = 0; i < 5; i++)
        key(0x9b);
    assert(focused("kar98k_mp"));
    paint();
    assert(listed(rifles, 7, 5, -1) && !strcmp(page(), "6/7"));
    assert(key(0x9d) && focused("mp44_mp"));
    paint();
    assert(tabs(kinds, 5, 2) && listed(automatics, 3, 2, 2) && !strcmp(page(), "3/3"));
    cases++;

    /* Enter on a gun of another kind equips it without the MP44's
     * attachments and saves the class; the list reopens on its kind. */
    saves = 0;
    assert(key(0x9c) && focused("m1garand_mp"));
    for (int i = 0; i < 5; i++)
        key(0x9b);
    assert(focused("kar98k_mp") && key(0x0d) && s_cac.screen == CAC_SCREEN_EDIT && saves == 1);
    assert(s_cac.classes[0].weapon[CLASS_SLOT_PRIMARY] == ClassData_Find("kar98k_mp") &&
           !ClassData_AttachmentCount(&s_cac.classes[0], CLASS_SLOT_PRIMARY));
    assert(!strncmp(saved, line, strlen(line)));
    assert(key(0x0d) && s_cac.screen == CAC_SCREEN_WEAPONS && focused("kar98k_mp"));
    paint();
    assert(tabs(kinds, 5, 1) && listed(rifles, 7, 5, 5) && !strcmp(page(), "6/7"));
    cases += 2;

    /* The pointer over a tab leaves the focus where it is, silently; a click
     * shows that kind with mouse_click, on the class's Kar98k among the
     * rifles; a click on the kind shown changes nothing. */
    assert(key(0x9b) && focused("g43_mp"));
    sounds = 0;
    point(tabX(4) + 45.0f, CAC_PICK_TAB_BOTTOM - 5.0f);
    paint();
    assert(s_cac.hover == CAC_HOVER_TAB + CLASS_GROUP_SHOTGUN && focused("g43_mp") && !sounds);
    point(tabX(4) + 49.0f, CAC_PICK_TAB_BOTTOM - 4.0f);
    paint();
    assert(focused("g43_mp") && !sounds);
    assert(key(0xc8) && focused("shotgun_mp") && s_cac.tab == CAC_TAB_CUSTOM && sounds == 1 &&
           !strcmp(lastSound, "mouse_click"));
    paint();
    assert(tabs(kinds, 5, 4) && listed(shotguns, 1, 0, -1));
    point(tabX(1) + 45.0f, CAC_PICK_TAB_BOTTOM - 5.0f);
    paint();
    assert(key(0xc8) && focused("kar98k_mp") && sounds == 2);
    assert(key(0x9b) && focused("g43_mp") && sounds == 3);
    paint();
    assert(key(0xc8) && focused("g43_mp") && sounds == 3 && s_cac.screen == CAC_SCREEN_WEAPONS);
    cases += 2;

    /* Over a row the pointer focuses its gun with mouse_over; a click equips
     * it. */
    point(100.0f, rowTop(2) + 12.0f);
    paint();
    assert(focused("enfield_mp") && sounds == 4 && !strcmp(lastSound, "mouse_over"));
    saves = 0;
    assert(key(0xc8) && s_cac.screen == CAC_SCREEN_EDIT && saves == 1 &&
           s_cac.classes[0].weapon[CLASS_SLOT_PRIMARY] == ClassData_Find("enfield_mp"));
    cases++;

    /* The secondary's list: the pistols' one tab, chosen, over its four;
     * left, right, Tab and a click on the tab change nothing, silently. */
    s_cac.slotFocus = CLASS_SLOT_SECONDARY;
    assert(key(0x0d) && s_cac.screen == CAC_SCREEN_WEAPONS && s_cac.pickSlot == CLASS_SLOT_SECONDARY &&
           focused("webley_mp"));
    paint();
    assert(text("EQUIP WEAPON | SECONDARY") && tabs(pistolKind, 1, 0) && !text("SUBMACHINE GUNS") && !text("RIFLES"));
    assert(listed(pistols, 4, 1, 1) && !strcmp(page(), "2/4"));
    assert(key(0x9b) && focused("TT30_mp"));
    sounds = 0;
    assert(key(0x9c) && key(0x9d) && key(0x09) && focused("TT30_mp") && !sounds);
    point(tabX(0) + 45.0f, CAC_PICK_TAB_BOTTOM - 5.0f);
    paint();
    assert(s_cac.hover == CAC_HOVER_TAB + CLASS_GROUP_PISTOL && key(0xc8) && focused("TT30_mp") && !sounds);
    cases += 2;

    /* Modify Equipment still goes along a slot's row: right from the German
     * frag to the S-Mine, C4 and around. */
    assert(key(0x1b) && s_cac.screen == CAC_SCREEN_EDIT);
    s_cac.slotFocus = CLASS_SLOT_LETHAL;
    assert(key(0x0d) && s_cac.screen == CAC_SCREEN_EQUIPMENT && focused("frag_grenade_german_mp"));
    assert(key(0x9d) && focused("mine_mp") && key(0x9d) && focused("c4_mp") && key(0x9d) &&
           focused("frag_grenade_american_mp"));
    cases++;

    printf("%d\n", cases);
    return 0;
}
'''

mutants = [
    # Every primary in one list, or the list's kind fixed.
    ('        if (g_classWeapons[indexes[i]].group != group)\n            continue;\n', ''),
    ('    int group = Cac_CandidateGroup(s_cac.pickSlot, s_cac.pickFocus);\n    int groupCount',
     '    int group = CLASS_GROUP_SMG;\n    int groupCount'),
    # Up and down through all the slot's guns, or the place in the kind lost.
    ("    case CAC_SCREEN_WEAPONS:\n        /* Through the shown kind's weapons, around. */\n", '    case -1:\n'),
    ('            *row = found;\n', ''),
    # Left and right as up and down, or both forward, or past the first kind.
    ('    } else if (!vertical && s_cac.screen == CAC_SCREEN_WEAPONS) {', '    } else if (0) {'),
    ('        Cac_CycleGroup(backward ? -1 : 1);', '        Cac_CycleGroup(1);'),
    ('        Cac_ShowGroup(groups[(i + delta + count) % count]);', '        Cac_ShowGroup(groups[(i + delta) % count]);'),
    # Another kind always on its first gun; the kind shown starting over.
    ('    s_cac.pickFocus = candidates[row];', '    s_cac.pickFocus = candidates[0];'),
    ('    if (!count || group == Cac_CandidateGroup(s_cac.pickSlot, s_cac.pickFocus))', '    if (!count)'),
    # The pointer over a tab taking the focus; a click on a tab as Select
    # Class's; a tab's hover by place, not kind; a row's by row.
    ('        if (s_cac.hover < CAC_HOVER_TAB)\n            s_cac.pickFocus = s_cac.hover;',
     '        s_cac.pickFocus = s_cac.hover;'),
    ('        if (s_cac.screen == CAC_SCREEN_WEAPONS && s_cac.hover >= CAC_HOVER_TAB) {', '        if (0) {'),
    ('            s_cac.hover = CAC_HOVER_TAB + groups[i];', '            s_cac.hover = CAC_HOVER_TAB + i;'),
    ('            s_cac.hover = rows[i];', '            s_cac.hover = i;'),
    # The rows' guns, the chosen tab, the tabs' places, the page counter.
    ('        int weapon = indexes[rows[i]];', '        int weapon = indexes[i];'),
    ('        int chosen = groups[i] == group;', '        int chosen = !i;'),
    ('CacRect tab = Cac_Rect(CAC_LEFT + (float)i * (CAC_SELECT_TAB_WIDTH + CAC_SELECT_TAB_GAP), top,',
     'CacRect tab = Cac_Rect(CAC_LEFT, top,'),
    ('"%d/%d", focusRow + 1, rowCount);', '"%d/%d", s_cac.pickFocus + 1, rowCount);'),
    ('#define CAC_PICK_TAB_BOTTOM 110.0f', '#define CAC_PICK_TAB_BOTTOM 118.0f'),
    # Modify Equipment's row cut at the kinds.
    ("    case CAC_SCREEN_WEAPONS:\n        /* Through the shown kind's weapons, around. */\n",
     "    case CAC_SCREEN_EQUIPMENT:\n    case CAC_SCREEN_WEAPONS:\n        /* Through the shown kind's weapons, around. */\n"),
    # A label wider than its tab.
    ('"SUBMACHINE GUNS", "RIFLES",', '"SUBMACHINE GUNS AND CARBINES", "RIFLES",'),
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


def run(directory, name, text, warnings, objects):
    path = Path(directory) / f'{name}.c'
    binary = Path(directory) / name
    path.write_text(text)
    subprocess.run(['cc', *flags, *warnings, str(path), *objects, '-lm', '-o', str(binary)], check=True)
    return subprocess.run([str(binary)], capture_output=True, text=True)


with tempfile.TemporaryDirectory(prefix='cod2-class-weapon-groups-') as directory:
    objects = []
    for source in data_sources:
        objects.append(str(Path(directory) / (source.stem + '.o')))
        subprocess.run(['cc', *flags, '-w', '-c', str(source), '-o', objects[-1]], check=True)
    result = run(directory, 'groups', program(menu), ['-Wall', '-Werror', '-Wno-unused-function'], objects)
    assert result.returncode == 0, (result.stdout[-500:], result.stderr[-3000:])
    cases = int(result.stdout)
    for index, (old, new) in enumerate(mutants):
        assert menu.count(old) == 1, old
        variant = menu.replace(old, new)
        if evidence(variant):
            continue
        result = run(directory, f'groups{index}', program(variant), ['-w'], objects)
        assert result.returncode != 0, f'mutant survived: {new!r}'

print('PASS: Equip Weapon shows one kind of the slot\'s weapons at a time under a tab for each of its kinds, the '
      'primary\'s submachine guns, rifles, automatic rifles, sniper rifles and shotguns and the secondary\'s pistols, '
      'each label within its tab in Barlow\'s metrics; left, right, Tab and a click on a tab show another kind on the '
      'class\'s gun or the kind\'s first, up, down and the wheel go around the kind\'s guns with its page counter, the '
      'pointer focuses a row and not a tab, a gun of another kind is equipped and saved, and Modify Equipment\'s rows '
      f'are as before; {cases} cases, {len(mutants)} failing mutants')
