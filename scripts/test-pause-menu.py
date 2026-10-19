#!/usr/bin/env python3
"""Run the production pause menu against a recording UI: its tabs and their
keys, the class lists, the team choices and what they send, leaving, the
settings pages and the way back from them, the pointer, the prompts and the
layout; then the hooks that open, paint and feed it, and the menu file that
replaces the retail in-game menu."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(root / 'scripts'))
from browser_class_menus import class_menus  # noqa: E402

source = (root / 'src/PC/ui_mp/ui_pause_menu.c').read_text()
main = (root / 'src/PC/ui_mp/ui_main_mp.c').read_text()
shared = (root / 'src/PC/ui_mp/ui_shared_mp.c').read_text()
neutral = re.search(r'^#define CG_SCORE_PANEL_NEUTRAL (\d+)$',
                    (root / 'src/headers/PC/cgame_mp/cg_scorepanel.h').read_text(), re.M).group(1)


def function(text, name):
    match = re.search(r'^((?:static )?[^\n;]*\b' + name + r'\([^;]*?\))\s*\{', text, re.M)
    assert match, name
    brace = text.index('{', match.end() - 1)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


# The menu file names the retail in-game menu, blurs the world and only
# opens the native screen; the asset scripts put it in the retail one's place.
menu_file = class_menus()['ui_mp/scriptmenus/ingame.menu'].decode()
assert re.search(r'name\s+"ingame"', menu_file) and re.search(r'blurWorld\s+5\.0', menu_file)
assert 'uiScript pauseMenuOpen;' in menu_file and 'itemDef' not in menu_file
assert 'scriptmenus/ingame.menu' not in (root / 'scripts/browser_settings_menu.py').read_text()
# The UI opens it from the script, keeps every key from the binds while it has
# the focus, feeds it keys before the stock Escape and paints it with the menu.
assert function(main, 'UI_CheckExecKey').count(
    '    if (UI_ClassMenu_WantsKeys(menu) || UI_PauseMenu_WantsKeys(menu))\n        return 1;\n') == 1
key_event = function(main, 'UI_KeyEvent_impl')
assert key_event.index('UI_PauseMenu_KeyEvent((displayContextDef_t *)uiInfo, menu, key, down)') < \
    key_event.index('key == 0x1b && down && !Menus_AnyFullScreenVisible(uiInfo)')
script = function(main, 'UI_RunMenuScript')
assert script.count('    if (I_stricmp(name, "pauseMenuOpen") == 0) {\n'
                    '        UI_PauseMenu_Open((displayContextDef_t *)uiInfo);\n        return;\n    }\n') == 1
menu_paint = function(shared, 'Menu_Paint')
assert menu_paint.index('    UI_WWIIMenu_PaintMenu(dc, menu);\n') < menu_paint.index('    UI_PauseMenu_Paint(dc, menu);\n') < \
    menu_paint.index('    if (debugMode) {')
assert 'menuDef_t *menu = Menus_FindByName_core(dc, UI_PauseMenu_Redirect(p));' in function(shared, 'Menus_OpenByName')

body = source
for include in ('"common_types.h"', '"imports.h"', '"cod2_feature_config.h"'):
    assert body.count('#include ' + include + '\n') == 1, include
    body = body.replace('#include ' + include + '\n', '')
body = body.replace('#include "headers/PC/cgame_mp/cg_scorepanel.h"\n',
                    '#define CG_SCORE_PANEL_NEUTRAL ' + neutral + '\n')

support = r'''
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#define COD2_FEATURE_GAMEPAD 1
typedef unsigned char byte;
typedef float vec_t;
typedef vec_t vec4_t[4];
typedef int qboolean;
typedef int Bool;
typedef struct Material { const char *name; } Material;
typedef Material *MaterialHandle;
typedef struct { const char *name; } windowDef_t;
typedef struct menuDef_t { windowDef_t window; } menuDef_t;
typedef struct itemDef_s itemDef_t;
typedef struct displayContextDef_t { int realTime; int cursorx, cursory; } displayContextDef_t;
typedef struct sharedUiInfo_t { struct { MaterialHandle whiteMaterial; } assets; } sharedUiInfo_t;
enum { TEAM_FREE, TEAM_AXIS, TEAM_ALLIES, TEAM_SPECTATOR };
#include "headers/PC/ui_mp/ui_class_data.h"
#include "headers/PC/cgame_mp/cg_overview.h"

static Material white = { "white" }, emblemMaterial = { "emblems" };
sharedUiInfo_t sharedUiInfo = { { &white } };

/* Every drawing call, as it was made. */
enum { FILL = 1, TEXT, KEYCAP, WEAPON, MAP };
typedef struct { int kind; char text[96]; int bold, horz, vert; float x, y, w, h, cap, tracking, s0, s1;
    vec4_t color; MaterialHandle material; } Drawn;
static Drawn drawn[512];
static int drawCount;
static Drawn *record(int kind) { assert(drawCount < 512); memset(&drawn[drawCount], 0, sizeof(Drawn));
    drawn[drawCount].kind = kind; return &drawn[drawCount++]; }
void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2,
                       float t2, const vec_t *color, MaterialHandle material) {
    Drawn *d = record(FILL);
    d->x = x; d->y = y; d->w = w; d->h = h; d->horz = horzAlign; d->vert = vertAlign; d->s0 = s1; d->s1 = s2;
    memcpy(d->color, color, sizeof(d->color)); d->material = material; (void)t1; (void)t2;
}
/* A 1280 x 720 screen: 1.5 px a unit from the left or right edge. */
void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign) {
    assert(vertAlign == 1);
    *x = horzAlign == 3 ? 1280.0f + *x * 1.5f : *x * 1.5f;
    *y *= 1.5f; *w *= 1.5f; *h *= 1.5f;
}
void CalcScreenX(float *x, int horzAlign) { assert(horzAlign == 4); *x *= 2.0f; }
void CalcScreenY(float *y, int vertAlign) { assert(vertAlign == 4); *y *= 1.5f; }
static const char *configMenus[0x20];
const char *CL_GetConfigString(int index) { assert(index >= 0x4de && index < 0x4de + 0x20); return configMenus[index - 0x4de]; }
int Dvar_GetInt(const char *name) { assert(!strcmp(name, "sv_serverId")); return 7; }
static int inGame = 1;
int Dvar_GetBool(const char *name) { assert(!strcmp(name, "cl_ingame")); return inGame; }
static char commands[16][160];
static int commandCount;
void Cbuf_ExecuteText(int when, const char *text) { assert(when == 2 && commandCount < 16);
    snprintf(commands[commandCount++], sizeof(commands[0]), "%s", text); }
char *va(const char *format, ...) { static char buffer[256]; va_list ap; va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap); va_end(ap); return buffer; }
static char menuLog[32][32];
static int menuLogCount, refuseOpen;
void Menus_CloseByName(displayContextDef_t *dc, const char *name) { (void)dc; assert(menuLogCount < 32);
    snprintf(menuLog[menuLogCount++], sizeof(menuLog[0]), "-%s", name); }
qboolean Menus_OpenByName(displayContextDef_t *dc, const char *name) { (void)dc; assert(menuLogCount < 32);
    snprintf(menuLog[menuLogCount++], sizeof(menuLog[0]), "+%s", name); return !refuseOpen; }
int I_stricmp(const char *s0, const char *s1) { return strcasecmp(s0, s1); }
static int sounds;
int UI_PlayLocalSoundAliasByName(const char *aliasname) { assert(!strcmp(aliasname, "mouse_over") ||
    !strcmp(aliasname, "mouse_click")); return ++sounds; }
static int gamepad;
int CL_Gamepad_IsActiveDevice(void) { return gamepad; }

/* The WWII lettering: Bold letters 0.66 of their capitals wide, Regular 0.6,
 * spaces 0.25, tracking capitals apart. */
float UI_WWIIMenu_Width(const char *text, int bold, float cap, float tracking) {
    float width = 0.0f; int n = 0;
    for (const char *c = text; *c; ++c, ++n) width += (*c == ' ' ? 0.25f : bold ? 0.66f : 0.6f) * cap;
    return width + (n > 1 ? (float)(n - 1) * tracking * cap : 0.0f);
}
void UI_WWIIMenu_Text(const char *text, int bold, float x, float baseline, int horzAlign, int vertAlign, float cap,
                      float tracking, const vec_t *color) {
    Drawn *d = record(TEXT);
    snprintf(d->text, sizeof(d->text), "%s", text);
    d->bold = bold; d->x = x; d->y = baseline; d->horz = horzAlign; d->vert = vertAlign; d->cap = cap; d->tracking = tracking;
    memcpy(d->color, color, sizeof(d->color));
}
static const vec4_t gold = { 0.8f, 0.72f, 0.47f, 1.0f }, idle = { 0.776f, 0.776f, 0.769f, 1.0f },
    whiteColor = { 1.0f, 1.0f, 1.0f, 1.0f }, otherColor = { 0.5f, 0.5f, 0.5f, 1.0f };
const float *UI_WWIIMenu_Color(int color) {
    return color == 1 ? gold : color == 0 ? idle : color == 4 ? whiteColor : otherColor;
}
float UI_WWIIMenu_KeycapWidth(const char *key, float height) {
    float width = (float)strlen(key) * 0.33f * height + 2.0f * 3.45f * height / 10.2f;
    return width < height ? height : width;
}
float UI_WWIIMenu_Keycap(const char *key, float x, float top, float height, int horzAlign, int vertAlign, const vec_t *tint) {
    Drawn *d = record(KEYCAP);
    snprintf(d->text, sizeof(d->text), "%s", key);
    d->x = x; d->y = top; d->h = height; d->horz = horzAlign; d->vert = vertAlign;
    if (tint) memcpy(d->color, tint, sizeof(d->color));
    return UI_WWIIMenu_KeycapWidth(key, height);
}

/* The class menu: five classes a list, by their primaries. */
static ClassLoadout custom[CLASS_COUNT], standard[CLASS_COUNT];
static int chosenIndex = -1, chosenStandard, chooseResult = 1, chooseCalls, chooseStandard, chooseIndex, editIndex = -1;
static const char *note;
const ClassLoadout *UI_ClassMenu_Classes(int isStandard) { return isStandard ? standard : custom; }
const char *UI_ClassMenu_Note(void) { return note; }
static const char *weaponNames[64] = { "Thompson", "Grease Gun" };
const char *UI_ClassMenu_WeaponName(int weapon) { return weapon >= 0 && weaponNames[weapon] ? weaponNames[weapon] : "Weapon"; }
void UI_ClassMenu_DrawWeapon(const ClassLoadout *loadout, int slot, float x, float y, float w, float h, int horzAlign,
                             const vec_t *color) {
    Drawn *d = record(WEAPON);
    snprintf(d->text, sizeof(d->text), "%d", loadout->weapon[slot]);
    d->x = x; d->y = y; d->w = w; d->h = h; d->horz = horzAlign; memcpy(d->color, color, sizeof(d->color));
}
int UI_ClassMenu_Choose(int isStandard, int index) { chooseCalls++; chooseStandard = isStandard; chooseIndex = index;
    return chooseResult; }
int UI_ClassMenu_Chosen(int *isStandard) { *isStandard = chosenStandard; return chosenIndex; }
void UI_ClassMenu_EditFromPause(displayContextDef_t *dc, int index) { (void)dc; editIndex = index; }

/* The match. */
static CG_Overview match;
static int overviewValid = 1, scoreRequests;
static CG_TeamSlots teams;
int CG_OverviewGet(CG_Overview *overview) { *overview = match; return overviewValid; }
int CG_OverviewTeamSlots(CG_TeamSlots *slots) { *slots = teams; return 1; }
void CG_OverviewRequestScores(void) { scoreRequests++; }
static int alliesKind = 0;
MaterialHandle CG_OverviewEmblems(float size, int *allies) { assert(size == 117.0f); *allies = alliesKind;
    return &emblemMaterial; }
void CG_OverviewRatio(int kills, int deaths, char *buf, int size) { snprintf(buf, size, "r%d/%d", kills, deaths); }
void CG_DrawMinimapOverview(float x, float y, float w, float h, int horzAlign, int vertAlign, float alpha) {
    Drawn *d = record(MAP);
    d->x = x; d->y = y; d->w = w; d->h = h; d->horz = horzAlign; d->vert = vertAlign; d->cap = alpha;
}
'''

checks = r'''
static displayContextDef_t dc = { 1000, 320, 400 };
static menuDef_t ingame = { { "ingame" } }, other = { { "options_menu" } };
static int near(float a, float b) { return fabsf(a - b) < 1e-3f; }
static const Drawn *text(const char *s) {
    for (int i = 0; i < drawCount; i++) if (drawn[i].kind == TEXT && !strcmp(drawn[i].text, s)) return &drawn[i];
    return NULL;
}
static const Drawn *keycap(const char *s) {
    for (int i = 0; i < drawCount; i++) if (drawn[i].kind == KEYCAP && !strcmp(drawn[i].text, s)) return &drawn[i];
    return NULL;
}
static int count(int kind) { int n = 0; for (int i = 0; i < drawCount; i++) n += drawn[i].kind == kind; return n; }
static void paint(void) { drawCount = 0; UI_PauseMenu_Paint(&dc, &ingame); }
static void key(int k) { assert(UI_PauseMenu_KeyEvent(&dc, &ingame, k, 1)); assert(UI_PauseMenu_KeyEvent(&dc, &ingame, k, 0)); }
static int logged(const char *entry) { for (int i = 0; i < menuLogCount; i++) if (!strcmp(menuLog[i], entry)) return 1; return 0; }
static void clearLogs(void) { menuLogCount = commandCount = 0; }
/* The cursor over a point in units from the left edge. */
static void point(float x, float y) { dc.cursorx = (int)lroundf(x * 1.5f / 2.0f); dc.cursory = (int)lroundf(y); }

int main(void) {
    int i;
    for (i = 0; i < CLASS_COUNT; i++) {
        snprintf(custom[i].name, sizeof(custom[i].name), "Mine %d", i);
        snprintf(standard[i].name, sizeof(standard[i].name), "Stock %d", i);
        for (int s = 0; s < CLASS_SLOT_COUNT; s++) custom[i].weapon[s] = standard[i].weapon[s] = s;
    }
    configMenus[2] = "browser_class";
    configMenus[5] = "team_americangerman";
    teams.team = TEAM_ALLIES; teams.allies = 4; teams.axis = 3; teams.alliesOpen = 28; teams.axisOpen = 29; teams.open = 57;

    /* Only the pause menu: every key, painted only over its own menu. */
    assert(UI_PauseMenu_WantsKeys(&ingame) && !UI_PauseMenu_WantsKeys(&other) && !UI_PauseMenu_WantsKeys(NULL));
    assert(!UI_PauseMenu_KeyEvent(&dc, &other, 0x1b, 1));
    drawCount = 0; UI_PauseMenu_Paint(&dc, &other); assert(drawCount == 0);

    /* It opens on the Class tab at the class chosen last, Q and E (or the
     * bumpers, or Tab) step through the tabs and wrap. */
    chosenIndex = 3; chosenStandard = 1;
    UI_PauseMenu_Open(&dc);
    assert(s_pause.open && s_pause.tab == PAUSE_TAB_CLASS && s_pause.standard && s_pause.classFocus == 3);
    assert(s_pause.tabCount == 5);
    key('e'); assert(s_pause.tab == PAUSE_TAB_OVERVIEW);
    key(0xd4); assert(s_pause.tab == PAUSE_TAB_TEAM);
    key(0x09); assert(s_pause.tab == PAUSE_TAB_LEAVE);
    key('e'); key('e'); assert(s_pause.tab == PAUSE_TAB_CLASS);
    key('q'); assert(s_pause.tab == PAUSE_TAB_SETTINGS);
    key(0xd3); key(0xd3); assert(s_pause.tab == PAUSE_TAB_TEAM);
    /* Characters are a typed copy of the keys: ignored. */
    key(0x400 | 'e'); assert(s_pause.tab == PAUSE_TAB_TEAM);
    /* Where everyone plays for himself there are no teams to change to. */
    teams.freeForAll = 1;
    UI_PauseMenu_Open(&dc);
    assert(s_pause.tabCount == 4 && s_pause.tab == PAUSE_TAB_CLASS);
    key('e'); key('e'); assert(s_pause.tab == PAUSE_TAB_LEAVE);
    s_pause.tab = PAUSE_TAB_TEAM; paint(); assert(s_pause.tab == PAUSE_TAB_CLASS);
    teams.freeForAll = 0;
    /* Escape and the right button close it. */
    clearLogs(); key(0x1b); assert(logged("-ingame") && !s_pause.open);
    UI_PauseMenu_Open(&dc); clearLogs(); key(0xc9); assert(logged("-ingame") && !s_pause.open);

    /* The Class tab: up and down wrap through five, left and right change the
     * list, Enter or a number chooses and closes, F1 edits the player's own. */
    chosenIndex = -1; chosenStandard = 0;
    UI_PauseMenu_Open(&dc);
    s_pause.standard = 0; s_pause.classFocus = 0;
    key(0x9a); assert(s_pause.classFocus == 4);
    key(0x9b); key(0x9b); assert(s_pause.classFocus == 1);
    key(0xce); assert(s_pause.classFocus == 0);
    key(0x9d); assert(s_pause.standard == 1);
    key(0x9c); assert(s_pause.standard == 0);
    key(0xa7); assert(editIndex == 0);
    s_pause.standard = 1; editIndex = -1; key(0xa7); assert(editIndex == -1);
    chooseResult = 0; clearLogs(); key(0x0d);
    assert(chooseCalls == 1 && chooseStandard == 1 && chooseIndex == 0 && !logged("-ingame") && s_pause.open);
    chooseResult = 1; key('3');
    assert(chooseCalls == 2 && chooseIndex == 2 && logged("-ingame") && !s_pause.open);
    UI_PauseMenu_Open(&dc); s_pause.tab = PAUSE_TAB_TEAM; clearLogs(); key('2');
    assert(chooseCalls == 2 && commandCount == 0 && s_pause.open);

    /* Change Team: Allies and Axis side by side over Auto-Assign. Joining the
     * other side sends the team menu's response; one's own side only closes. */
    UI_PauseMenu_Open(&dc); s_pause.tab = PAUSE_TAB_TEAM;
    assert(s_pause.teamFocus == PAUSE_TEAM_ALLIES);
    key(0x9d); assert(s_pause.teamFocus == PAUSE_TEAM_AXIS);
    key(0x9b); assert(s_pause.teamFocus == PAUSE_TEAM_AUTO);
    key(0x9a); assert(s_pause.teamFocus == PAUSE_TEAM_ALLIES);
    key(0x9c); assert(s_pause.teamFocus == PAUSE_TEAM_AXIS);
    clearLogs(); key(0x0d);
    assert(commandCount == 1 && !strcmp(commands[0], "cmd mr 7 5 axis\n") && logged("-ingame"));
    UI_PauseMenu_Open(&dc); s_pause.tab = PAUSE_TAB_TEAM; s_pause.teamFocus = PAUSE_TEAM_ALLIES;
    clearLogs(); key(0x0d); assert(commandCount == 0 && logged("-ingame"));
    /* Auto-assign from a side only balances: the stock one reopens the team
     * menu for a player on a team. A spectator gets the stock one. */
    teams.allies = 5; teams.axis = 3;
    UI_PauseMenu_Open(&dc); s_pause.tab = PAUSE_TAB_TEAM; s_pause.teamFocus = PAUSE_TEAM_AUTO;
    clearLogs(); key(0x0d); assert(commandCount == 1 && !strcmp(commands[0], "cmd mr 7 5 axis\n"));
    teams.allies = 4;
    UI_PauseMenu_Open(&dc); s_pause.tab = PAUSE_TAB_TEAM; s_pause.teamFocus = PAUSE_TEAM_AUTO;
    clearLogs(); key(0x0d); assert(commandCount == 0 && logged("-ingame"));
    teams.team = TEAM_AXIS; teams.allies = 2; teams.axis = 4;
    UI_PauseMenu_Open(&dc); s_pause.tab = PAUSE_TAB_TEAM; s_pause.teamFocus = PAUSE_TEAM_AUTO;
    clearLogs(); key(0x0d); assert(commandCount == 1 && !strcmp(commands[0], "cmd mr 7 5 allies\n"));
    teams.team = TEAM_SPECTATOR;
    UI_PauseMenu_Open(&dc); s_pause.tab = PAUSE_TAB_TEAM; s_pause.teamFocus = PAUSE_TEAM_AUTO;
    clearLogs(); key(0x0d); assert(commandCount == 1 && !strcmp(commands[0], "cmd mr 7 5 autoassign\n"));
    UI_PauseMenu_Open(&dc); s_pause.tab = PAUSE_TAB_TEAM; s_pause.teamFocus = PAUSE_TEAM_ALLIES;
    clearLogs(); key(0x0d); assert(commandCount == 1 && !strcmp(commands[0], "cmd mr 7 5 allies\n"));
    /* Without the team menu precached nothing is sent. */
    configMenus[5] = NULL;
    UI_PauseMenu_Open(&dc); s_pause.tab = PAUSE_TAB_TEAM; s_pause.teamFocus = PAUSE_TEAM_AXIS;
    clearLogs(); key(0x0d); assert(commandCount == 0 && logged("-ingame"));
    configMenus[5] = "team_americangerman";
    teams.team = TEAM_ALLIES; teams.allies = 4; teams.axis = 3;

    /* Leave Game: Return to Match only closes; Leave Match disconnects. */
    UI_PauseMenu_Open(&dc); s_pause.tab = PAUSE_TAB_LEAVE;
    assert(s_pause.leaveFocus == PAUSE_LEAVE_RETURN);
    clearLogs(); key(0x0d); assert(commandCount == 0 && logged("-ingame") && !s_pause.open);
    UI_PauseMenu_Open(&dc); s_pause.tab = PAUSE_TAB_LEAVE;
    key(0x9b); assert(s_pause.leaveFocus == PAUSE_LEAVE_MATCH);
    key(0x9b); assert(s_pause.leaveFocus == PAUSE_LEAVE_RETURN);
    key(0x9a); clearLogs(); key(0xbf);
    assert(commandCount == 1 && !strcmp(commands[0], "disconnect\n") && logged("-ingame"));

    /* Settings: each row opens its retail page with the options menu; the page
     * goes back to the pause menu, on this tab, where it would open the main
     * menu, until the pause menu has a key again. */
    {
        static const char *const pages[] = { "+options_look", "+options_graphics", "+options_sound", "+options_game" };
        for (i = 0; i < 4; i++) {
            UI_PauseMenu_Open(&dc); s_pause.tab = PAUSE_TAB_SETTINGS; s_pause.settingsFocus = i;
            clearLogs(); key(0x0d);
            assert(logged("+options_menu") && logged(pages[i]) && logged("-ingame") && !s_pause.open);
            assert(!strcmp(UI_PauseMenu_Redirect("options_menu"), "options_menu") && !UI_PauseMenu_Redirect(NULL));
            /* The page's Escape opens the main menu, then closing it opens it again. */
            assert(!strcmp(UI_PauseMenu_Redirect("main_text"), "ingame"));
            UI_PauseMenu_Open(&dc);
            assert(!strcmp(UI_PauseMenu_Redirect("MAIN_TEXT"), "ingame"));
            UI_PauseMenu_Open(&dc);
            assert(s_pause.tab == PAUSE_TAB_SETTINGS && s_pause.settingsFocus == i);
            /* Out of a match the main menu is never taken. */
            inGame = 0; assert(!strcmp(UI_PauseMenu_Redirect("main_text"), "main_text")); inGame = 1;
            key(0x9b); key(0x9a);
            assert(!strcmp(UI_PauseMenu_Redirect("main_text"), "main_text"));
        }
        key(0x9b); assert(s_pause.settingsFocus == 0);
        /* A page that cannot open leaves the pause menu as it was. */
        refuseOpen = 1; clearLogs(); key(0x0d);
        assert(!logged("-ingame") && s_pause.open && !strcmp(UI_PauseMenu_Redirect("main_text"), "main_text"));
        refuseOpen = 0;
        /* A fresh Escape opens the Class tab; the class editor's way back keeps the tab. */
        UI_PauseMenu_Open(&dc); assert(s_pause.tab == PAUSE_TAB_CLASS);
        s_pause.tab = PAUSE_TAB_OVERVIEW; clearLogs(); UI_PauseMenu_Return(&dc);
        assert(logged("+ingame")); UI_PauseMenu_Open(&dc); assert(s_pause.tab == PAUSE_TAB_OVERVIEW);
        UI_PauseMenu_Open(&dc); assert(s_pause.tab == PAUSE_TAB_CLASS);
    }

    /* The tab bar: Q and E keycaps, or the bumpers' with a controller; idle
     * labels 30 apart from 12.5 after the left keycap, the active one in gold
     * Bold about its idle middle, between brackets, and 16.7 to the right one. */
    UI_PauseMenu_Open(&dc); s_pause.tab = PAUSE_TAB_OVERVIEW; paint();
    assert(scoreRequests > 0);
    {
        const Drawn *q = keycap("Q"), *e = keycap("E"), *cls = text("CLASS"), *team = text("CHANGE TEAM");
        const Drawn *overview = text("OVERVIEW"), *settings = text("SETTINGS");
        float keyWidth = UI_WWIIMenu_KeycapWidth("Q", 12.5f);
        float classWidth = UI_WWIIMenu_Width("CLASS", 0, 5.4f, 0.13f);
        float overviewWidth = UI_WWIIMenu_Width("OVERVIEW", 0, 5.4f, 0.13f);
        float overviewMiddle = 49.2f + keyWidth + 12.5f + classWidth + 30.0f + 0.5f * overviewWidth;
        assert(q && e && near(q->x, 49.2f) && near(q->y, 25.0f) && near(q->h, 12.5f));
        assert(cls && !cls->bold && near(cls->x, 49.2f + keyWidth + 12.5f) && near(cls->y, 34.4f) && near(cls->cap, 5.4f));
        assert(!memcmp(cls->color, s_pauseIdle, sizeof(s_pauseIdle)));
        assert(team && near(team->x, cls->x + classWidth + 30.0f + overviewWidth + 30.0f));
        assert(overview && overview->bold && !memcmp(overview->color, s_pauseGold, sizeof(s_pauseGold)) && near(overview->cap, 6.25f));
        assert(near(overview->x + 0.5f * UI_WWIIMenu_Width("OVERVIEW", 1, 6.25f, 0.12f), overviewMiddle));
        assert(settings && near(e->x, settings->x + UI_WWIIMenu_Width("SETTINGS", 0, 5.4f, 0.13f) + 16.7f));
        /* The brackets: two lines as tall as the keycaps, ticks at their ends. */
        int brackets = 0;
        for (i = 0; i < drawCount; i++)
            if (drawn[i].kind == FILL && near(drawn[i].h, 12.5f) && near(drawn[i].y, 25.0f)) brackets++;
        assert(brackets == 2);
        assert(!text("TIME REMAINING") && count(MAP) == 1 && !text("Select") && !text("Back"));
    }
    gamepad = 1; paint(); assert(keycap("LB") && keycap("RB") && !keycap("Q")); gamepad = 0;

    /* The prompts: Select where there is something to choose, Back and Edit
     * on the Class tab, Edit for the player's own classes only; a
     * controller's colored A and B glyphs, without Edit. */
    s_pause.tab = PAUSE_TAB_CLASS; s_pause.standard = 0; paint();
    assert(keycap("ENTER") && text("Select") && keycap("F1") && text("Edit") && keycap("ESC") && text("Back"));
    assert(keycap("ESC")->horz == 3 && near(keycap("ESC")->x + UI_WWIIMenu_KeycapWidth("ESC", 10.2f) + 3.1f +
                                             UI_WWIIMenu_Width("Back", 0, 6.6f, 0.0f), -54.2f));
    assert(near(keycap("ENTER")->x, 53.3f) && near(keycap("ENTER")->y + 5.1f, 447.5f));
    s_pause.standard = 1; paint(); assert(text("Select") && !text("Edit") && text("Back"));
    gamepad = 1; s_pause.standard = 0; paint();
    assert(keycap("A") && keycap("B") && !keycap("F1") && keycap("A")->color[1] > keycap("A")->color[0]);
    assert(keycap("B")->color[0] > keycap("B")->color[1] && near(keycap("A")->h, 15.0f));
    gamepad = 0;
    for (int t = PAUSE_TAB_TEAM; t <= PAUSE_TAB_SETTINGS; t++) {
        s_pause.tab = t; paint(); assert(text("Select") && !text("Back") && !text("Edit"));
    }

    /* The Class tab: the toggles, the chosen one raised with its label larger
     * in Bold, the list in its order, framed and gold where focused, standing
     * above the rows' middles, and the focused class's four tiles. */
    chosenIndex = 1; chosenStandard = 0; note = "Loading your classes...";
    s_pause.tab = PAUSE_TAB_CLASS; s_pause.standard = 0; s_pause.classFocus = 2; paint();
    assert(text("CUSTOM") && text("STANDARD ISSUE") && text("CUSTOM")->bold && !text("STANDARD ISSUE")->bold);
    assert(near(text("CUSTOM")->cap, 6.6f) && near(text("STANDARD ISSUE")->cap, 5.0f));
    assert(near(text("CUSTOM")->y, 70.8f - 2.5f + 0.5f * (13.4f + 2.5f + 6.6f)));
    assert(near(text("STANDARD ISSUE")->y, 70.8f + 0.5f * (13.4f + 5.0f)));
    assert(text("MINE 0") && near(text("MINE 0")->x, 54.2f) && !text("MINE 0")->bold && near(text("MINE 0")->cap, 6.8f));
    assert(near(text("MINE 0")->y, 98.3f + 0.5f * 27.9f - 1.3f + 0.5f * 6.8f) && near(text("MINE 0")->tracking, 0.30f));
    assert(text("MINE 2")->bold && near(text("MINE 2")->x, 62.5f) && !memcmp(text("MINE 2")->color, s_pauseGold, sizeof(s_pauseGold)));
    assert(near(text("MINE 2")->y, 98.3f + 2.0f * 29.25f + 0.5f * 27.9f - 1.3f + 0.5f * 7.2f));
    assert(near(text("MINE 3")->y - text("MINE 0")->y, 3.0f * 29.25f) && !text("EQUIPPED"));
    assert(text("Loading your classes..."));
    assert(text("PRIMARY") && text("SECONDARY") && text("LETHAL") && text("TACTICAL") && count(WEAPON) == 4);
    assert(near(text("PRIMARY")->x, 250.0f) && near(text("PRIMARY")->y, 91.7f - 15.9f));
    assert(text("THOMPSON") && near(text("THOMPSON")->y, 91.7f - 5.0f) && near(text("TACTICAL")->x, 375.0f));
    s_pause.standard = 1; paint();
    assert(text("STOCK 0") && !text("MINE 0") && !text("Loading your classes..."));
    assert(text("STANDARD ISSUE")->bold && !text("CUSTOM")->bold && near(text("STANDARD ISSUE")->cap, 6.6f));
    /* A long lethal name shrinks to its column. */
    weaponNames[2] = "Mk 1 Fragmentation Grenade Number 9";
    paint();
    {
        const Drawn *lethal = text("MK 1 FRAGMENTATION GRENADE NUMBER 9");
        assert(lethal && lethal->cap < 6.5f && UI_WWIIMenu_Width(lethal->text, 1, lethal->cap, 0.06f) <= 375.0f - 250.0f - 6.0f + 1e-3f);
    }
    note = NULL;

    /* Overview: the title, the clock at the right, the match score's bars and
     * the arrow at the viewer's, the stats with unknown ones at 0, the map. */
    snprintf(match.mode, sizeof(match.mode), "Team Deathmatch");
    snprintf(match.map, sizeof(match.map), "Toujane");
    match.topScore = 30; match.bottomScore = 4; match.limit = 100; match.standing = 1; match.emblem = 0;
    match.score = 350; match.kills = 3; match.deaths = 1; match.assists = -1; match.headshots = 2;
    snprintf(match.clock, sizeof(match.clock), "9:42");
    match.clockColor[0] = match.clockColor[1] = match.clockColor[2] = match.clockColor[3] = 1.0f;
    s_pause.tab = PAUSE_TAB_OVERVIEW; paint();
    assert(text("Team Deathmatch // Toujane") && text("Team Deathmatch // Toujane")->bold);
    assert(text("TIME REMAINING") && text("TIME REMAINING")->horz == 3 && text("9:42") && text("9:42")->horz == 3);
    assert(near(text("9:42")->x + UI_WWIIMenu_Width("9:42", 1, 17.6f, 0.02f), -45.0f) && text("9:42")->color[2] < 1.0f);
    assert(text("\xbb MATCH SCORE") && text("\xbb BATTLE STATS") && text("30") && text("4"));
    assert(text("0") && !text("-1") && near(text("0")->x, 47.0f + 146.0f - 2.5f));
    assert(near(text("TIME REMAINING")->cap, 8.0f) && near(text("K/D RATIO")->cap, 6.7f));
    assert(text("r3/1") && text("350") && text("3") && text("2") && text("K/D RATIO") && text("HEADSHOTS"));
    assert(near(text("r3/1")->y, 258.3f) && near(text("HEADSHOTS")->y, 232.3f + 2.0f * 57.1f));
    assert(near(text("HEADSHOTS")->x, 47.0f + 146.0f));
    {
        int arrow = 0, emblem = 0, filled = 0;
        for (i = 0; i < drawCount; i++) {
            if (drawn[i].kind != FILL) continue;
            arrow += near(drawn[i].w, 0.75f);
            emblem += drawn[i].material == &emblemMaterial && near(drawn[i].s0, 0.0f) && near(drawn[i].w, 52.0f);
            filled += near(drawn[i].x, 87.5f) && near(drawn[i].w, 209.2f * 0.3f) && near(drawn[i].h, 27.5f);
        }
        assert(arrow == 10 && emblem == 1 && filled == 1);
        match.standing = atoi("''' + neutral + r'''"); match.emblem = 3; paint(); arrow = emblem = 0;
        for (i = 0; i < drawCount; i++) {
            arrow += drawn[i].kind == FILL && near(drawn[i].w, 0.75f);
            emblem += drawn[i].material == &emblemMaterial && near(drawn[i].s0, 0.5f);
        }
        assert(arrow == 0 && emblem == 1);
    }
    for (i = 0; i < drawCount; i++)
        if (drawn[i].kind == MAP)
            assert(near(drawn[i].x, 320.0f) && near(drawn[i].y, 70.0f) && near(drawn[i].w, 360.0f) &&
                   near(drawn[i].h, 333.0f) && drawn[i].horz == 1 && near(drawn[i].cap, 1.0f));
    match.clock[0] = '\0'; paint(); assert(!text("TIME REMAINING"));
    overviewValid = 0; paint(); assert(!text("TIME REMAINING") && count(MAP) == 0 && keycap("Q")); overviewValid = 1;

    /* Change Team: each tile's slots, the focused one's title larger, the
     * emblems on the sides' tiles; Allies without one when their nation has none. */
    s_pause.tab = PAUSE_TAB_TEAM; s_pause.teamFocus = PAUSE_TEAM_AXIS; paint();
    assert(text("28") && text("29") && text("57") && text("ALLIES") && text("AXIS") && text("AUTO-ASSIGN"));
    assert(near(text("AXIS")->cap, 10.0f) && near(text("ALLIES")->cap, 7.2f) && near(text("ALLIES")->x, 44.2f + 9.1f));
    assert(near(text("ALLIES")->y, 56.0f + 21.0f) && near(text("57")->y, 56.0f + 155.8f + 9.0f + 78.3f - 5.5f - 14.0f));
    {
        int emblems = 0;
        for (i = 0; i < drawCount; i++) emblems += drawn[i].material == &emblemMaterial;
        assert(emblems == 2);
        alliesKind = -1; paint(); emblems = 0;
        for (i = 0; i < drawCount; i++) emblems += drawn[i].material == &emblemMaterial;
        assert(emblems == 1); alliesKind = 0;
    }

    /* Leave Game's rows smaller and centered, under the larger question;
     * Settings' rows standing above their middles, the focused one's name and
     * note at the right, the note's words wrapped to its width. */
    s_pause.tab = PAUSE_TAB_LEAVE; s_pause.leaveFocus = PAUSE_LEAVE_RETURN; paint();
    assert(text("LEAVE MATCH") && near(text("LEAVE MATCH")->cap, 6.3f) && near(text("LEAVE MATCH")->tracking, 0.075f));
    assert(near(text("LEAVE MATCH")->y, 207.5f + 29.25f + 0.5f * 27.9f + 0.5f * 6.3f));
    assert(near(text("Are you sure you want to leave the match?")->cap, 11.3f));
    s_pause.tab = PAUSE_TAB_SETTINGS; s_pause.settingsFocus = PAUSE_SETTINGS_CONTROLS; paint();
    assert(text("GRAPHICS") && near(text("GRAPHICS")->cap, 6.8f) && near(text("GRAPHICS")->tracking, 0.09f));
    assert(near(text("GRAPHICS")->y, 89.6f + 29.25f + 0.5f * 27.9f - 1.3f + 0.5f * 6.8f));
    {
        const Drawn *controls = text("Set up your control scheme to match your style of play"), *title = NULL;
        assert(controls && near(controls->x, 411.0f) && near(controls->y, 121.0f) && near(controls->cap, 6.75f));
        for (i = 0; i < drawCount; i++)
            if (drawn[i].kind == TEXT && !strcmp(drawn[i].text, "CONTROLS") && drawn[i].x > 400.0f) title = &drawn[i];
        assert(near(controls->tracking, 0.07f) && title && near(title->cap, 9.6f) && near(title->tracking, 0.07f));
    }
    drawCount = 0;
    Pause_Wrapped("aaaa bbbb cccc", 10.0f, 100.0f, 30.0f, 10.0f, 0.1f, 1.55f, gold);
    assert(drawCount == 3 && !strcmp(drawn[0].text, "aaaa") && !strcmp(drawn[1].text, "bbbb"));
    assert(near(drawn[0].y, 100.0f) && near(drawn[1].y, 115.5f) && near(drawn[2].y, 131.0f) && near(drawn[2].tracking, 0.1f));
    Pause_Wrapped("aa bb", 10.0f, 100.0f, 40.0f, 10.0f, 0.1f, 1.55f, gold);
    assert(drawCount == 4 && !strcmp(drawn[3].text, "aa bb"));
    /* 26.5 units of letters and space, 30.5 with the tracking. */
    Pause_Wrapped("aa bb", 10.0f, 100.0f, 28.0f, 10.0f, 0.1f, 1.55f, gold);
    assert(drawCount == 6 && !strcmp(drawn[4].text, "aa") && !strcmp(drawn[5].text, "bb"));

    /* The pointer: the element under it takes the focus once it moves; a
     * click acts on it, or on a tab switches to it. A list's toggle only
     * changes with a click. */
    s_pause.tab = PAUSE_TAB_TEAM; s_pause.teamFocus = PAUSE_TEAM_ALLIES;
    point(44.2f + 155.8f + 9.0f + 70.0f, 56.0f + 70.0f); paint(); dc.cursorx++; paint();
    assert(s_pause.teamFocus == PAUSE_TEAM_AXIS);
    clearLogs(); key(0xc8); assert(commandCount == 1 && !strcmp(commands[0], "cmd mr 7 5 axis\n"));
    UI_PauseMenu_Open(&dc); s_pause.tab = PAUSE_TAB_TEAM;
    {
        const Drawn *overview;
        paint(); overview = text("OVERVIEW");
        point(overview->x + 2.0f, 30.0f); paint(); dc.cursorx++; paint();
        assert(s_pause.tab == PAUSE_TAB_TEAM);
        key(0xc8); assert(s_pause.tab == PAUSE_TAB_OVERVIEW);
    }
    s_pause.tab = PAUSE_TAB_CLASS; s_pause.standard = 0;
    point(43.3f + 95.9f + 20.0f, 76.0f); paint(); dc.cursorx++; paint(); assert(s_pause.standard == 0);
    chooseCalls = 0; key(0xc8); assert(s_pause.standard == 1 && chooseCalls == 0 && s_pause.open);
    point(20.0f, 98.3f + 4.0f * 29.25f + 10.0f); paint(); dc.cursory++; paint(); assert(s_pause.classFocus == 4);
    key(0xc8); assert(chooseCalls == 1 && chooseIndex == 4 && chooseStandard == 1);
    UI_PauseMenu_Open(&dc); s_pause.tab = PAUSE_TAB_LEAVE;
    point(20.0f, 207.5f + 29.25f + 10.0f); paint(); dc.cursory++; paint(); assert(s_pause.leaveFocus == PAUSE_LEAVE_MATCH);

    /* The wash fades in over 150 ms. */
    dc.realTime = 5000; UI_PauseMenu_Open(&dc); s_pause.tab = PAUSE_TAB_LEAVE;
    paint(); assert(drawn[0].kind == FILL && drawn[0].horz == 4 && near(drawn[0].color[3], 0.0f));
    dc.realTime = 5075; paint(); assert(near(drawn[0].color[3], 0.21f));
    dc.realTime = 6000; paint(); assert(near(drawn[0].color[3], 0.42f));
    assert(sounds > 0);
    puts("ok");
    return 0;
}
'''

mutants = [
    ('index = index < 0 ? 0 : (index + step + s_pause.tabCount) % s_pause.tabCount;',
     'index = index < 0 ? 0 : (index - step + s_pause.tabCount) % s_pause.tabCount;'),
    ('        if (i == PAUSE_TAB_TEAM && slots.freeForAll)\n            continue;\n', ''),
    ('    case PAUSE_KEY_ESCAPE:\n    case PAUSE_KEY_MOUSE2:\n        Pause_Close(dc);\n',
     '    case PAUSE_KEY_ESCAPE:\n    case PAUSE_KEY_MOUSE2:\n'),
    ("    case 'q':\n", ''),
    ('    case PAUSE_KEY_RSHLDR:\n', ''),
    ('s_pause.classFocus = Pause_Wrap(s_pause.classFocus + step, CLASS_COUNT);',
     's_pause.classFocus = (s_pause.classFocus + step) % CLASS_COUNT;'),
    ('            s_pause.standard = !s_pause.standard;', '            ;'),
    ('if (s_pause.tab == PAUSE_TAB_CLASS && !s_pause.standard)\n            UI_ClassMenu_EditFromPause',
     'if (s_pause.tab == PAUSE_TAB_CLASS)\n            UI_ClassMenu_EditFromPause'),
    ('        if (UI_ClassMenu_Choose(s_pause.standard, s_pause.classFocus))\n            Pause_Close(dc);',
     '        UI_ClassMenu_Choose(s_pause.standard, s_pause.classFocus);\n            Pause_Close(dc);'),
    ("if (s_pause.tab == PAUSE_TAB_CLASS && key >= '1' && key < '1' + CLASS_COUNT)",
     "if (key >= '1' && key < '1' + CLASS_COUNT)"),
    ('choice = other + 1 < own ?', 'choice = other < own ?'),
    ('    if (choice >= 0 && (team == TEAM_SPECTATOR || team != slots.team))',
     '    if (choice >= 0)'),
    ('    if (choice == PAUSE_TEAM_AUTO && (slots.team == TEAM_ALLIES || slots.team == TEAM_AXIS)) {',
     '    if (0) {'),
    ('        return focus == PAUSE_TEAM_ALLIES ? PAUSE_TEAM_AXIS : PAUSE_TEAM_ALLIES;', '        return focus;'),
    ('            Cbuf_ExecuteText(2, "disconnect\\n");\n', ''),
    ('return s_pause.settingsPage && name && !I_stricmp(name, "main_text") && Dvar_GetBool("cl_ingame") ? "ingame"',
     'return name && !I_stricmp(name, "main_text") && Dvar_GetBool("cl_ingame") ? "ingame"'),
    ('!I_stricmp(name, "main_text") && Dvar_GetBool("cl_ingame") ? "ingame"', '!I_stricmp(name, "main_text") ? "ingame"'),
    ('    s_pause.settingsPage = 0;\n    tab = s_pause.tab;', '    tab = s_pause.tab;'),
    ('    if ((s_pause.keepTab || s_pause.settingsPage) && Pause_TabIndex(s_pause.tab) >= 0) {',
     '    if (s_pause.keepTab && Pause_TabIndex(s_pause.tab) >= 0) {'),
    ('    if (!Menus_OpenByName(dc, "options_menu"))\n        return;\n', ''),
    ('    if ((s_pause.keepTab || s_pause.settingsPage) && Pause_TabIndex(s_pause.tab) >= 0) {', '    if (0) {'),
    ('    { "AUDIO", "Set the volume of the game\'s sound and music", "options_sound" },',
     '    { "AUDIO", "Set the volume of the game\'s sound and music", "options_graphics" },'),
    ('    if (s_pause.tab != PAUSE_TAB_OVERVIEW)\n        x +=', '    if (1)\n        x +='),
    ('    if (s_pause.tab == PAUSE_TAB_CLASS && !s_pause.standard && !gamepad)', '    if (s_pause.tab == PAUSE_TAB_CLASS)'),
    ('        if ((s_pause.hover >> 8) != PAUSE_HOVER_TOGGLE)\n            Pause_FocusHover();', '        Pause_FocusHover();'),
    ('shade[3] = PAUSE_WASH * wash;', 'shade[3] = PAUSE_WASH;'),
    ('snprintf(value, sizeof(value), "%d", values[i] < 0 ? 0 : values[i]);',
     'snprintf(value, sizeof(value), "%d", values[i]);'),
    ('    if (overview.standing != CG_SCORE_PANEL_NEUTRAL) {', '    if (1) {'),
    ('        float s0 = overview.emblem == allies ? 0.0f : 0.5f;', '        float s0 = 0.0f;'),
    ('                   allies >= 0 ? emblems : NULL, 0.0f);', '                   emblems, 0.0f);'),
    ('    if (width > nameRoom)\n        cap *= nameRoom / width;\n', ''),
    ('    note = s_pause.standard ? NULL : UI_ClassMenu_Note();', '    note = UI_ClassMenu_Note();'),
    ('    CG_OverviewRequestScores();\n', ''),
    ('#define PAUSE_TAB_GAP 30.0f', '#define PAUSE_TAB_GAP 28.0f'),
    ('#define PAUSE_TAB_FIRST_GAP 12.5f', '#define PAUSE_TAB_FIRST_GAP 14.0f'),
    ('#define PAUSE_ROW_PITCH 29.25f', '#define PAUSE_ROW_PITCH 30.0f'),
    ('static const PauseRowStyle s_pauseClassRows = { 6.8f, 0.30f, 7.2f, 0.33f, 1.3f };',
     'static const PauseRowStyle s_pauseClassRows = { 6.8f, 0.09f, 7.2f, 0.12f, 1.3f };'),
    ('static const PauseRowStyle s_pauseSettingsRows = { 6.8f, 0.09f, 7.2f, 0.12f, 1.3f };',
     'static const PauseRowStyle s_pauseSettingsRows = { 6.3f, 0.09f, 7.2f, 0.12f, 0.0f };'),
    ('static const PauseRowStyle s_pauseLeaveRows = { 6.3f, 0.075f, 7.2f, 0.12f, 0.0f };',
     'static const PauseRowStyle s_pauseLeaveRows = { 6.3f, 0.075f, 7.2f, 0.12f, 1.3f };'),
    ('    float middle = top + 0.5f * PAUSE_ROW_HEIGHT - style->lift;', '    float middle = top + 0.5f * PAUSE_ROW_HEIGHT;'),
    ('        float cap = selected ? PAUSE_TOGGLE_CHOSEN_CAP : PAUSE_TOGGLE_CAP;', '        float cap = PAUSE_TOGGLE_CAP;'),
    ('                baseline += cap * spacing;', '                baseline += cap * 1.75f;'),
    ('            if (Pause_Width(trial, 0, cap, tracking) > width) {', '            if (Pause_Width(trial, 0, cap, 0.0f) > width) {'),
    ('#define PAUSE_PROMPT_MIDDLE 32.5f', '#define PAUSE_PROMPT_MIDDLE 53.1f'),
    ('#define PAUSE_STAT_PITCH 57.1f', '#define PAUSE_STAT_PITCH 50.0f'),
    ('        if (prefix ? !strncmp(menu, menuName, strlen(menuName)) : !I_stricmp(menu, menuName)) {',
     '        if (!I_stricmp(menu, menuName)) {'),
]

with tempfile.TemporaryDirectory(prefix='cod2-pause-menu-') as tmp:
    path = Path(tmp)
    flags = ['-std=gnu99', '-I', str(root / 'src'), '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
             '-Wno-unused-function']

    def build(variant, name, opt):
        (path / f'{name}.c').write_text(support + variant + checks)
        subprocess.run(['cc', *flags, opt, '-g', str(path / f'{name}.c'), '-lm', '-o', str(path / name)], check=True)
        return subprocess.run([str(path / name)], capture_output=True, text=True)

    result = build(body, 'pause', '-O1')
    assert result.returncode == 0 and result.stdout.strip() == 'ok', result.stderr[-3000:]
    for index, (before, after) in enumerate(mutants):
        assert body.count(before) == 1, before
        result = build(body.replace(before, after), f'mutant{index}', '-O0')
        assert result.returncode != 0, f'mutant survived: {after!r}'
print('PASS: Escape opens the pause menu over the blurred view on the Class tab at the class chosen last; Q/E, the '
      'bumpers or Tab page through Class, Overview, Change Team (not in deathmatch), Leave Game and Settings; '
      'classes choose from either list, F1 edits the player\'s own; teams send the team menu\'s response, an '
      'auto-assign from a side only balances; leaving disconnects; settings pages return to the tab; the tab bar, '
      'prompts, lists, tiles, overview and pointer follow the captures\' layout; the hooks and menu file are wired; '
      f'{len(mutants)} failing regression mutants')
