#!/usr/bin/env python3
"""Run the production Start New Server screen (ui_match_menu.c) against a
recording UI: the match page's Mode and Map boxes, Game Rules and Bots rows
and Start Match box with the focused item's page dimmed beside them; the
mode, map, rules and bot pages with their panels; keys, pointer and clicks;
the room StartServer creates, its rules and its name from the gametag; the
hooks that hide createserver's retail menus under it; and the layout's
numbers against the WWII captures', in Barlow's metrics."""
import importlib.util
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(root / 'scripts'))
from barlow_metrics import Font  # noqa: E402

source = (root / 'src/PC/ui_mp/ui_match_menu.c').read_text()
header = (root / 'src/headers/PC/ui_mp/ui_match_menu.h').read_text()
wwii_header = (root / 'src/headers/PC/ui_mp/ui_wwii_menu.h').read_text()
shared = (root / 'src/PC/ui_mp/ui_shared_mp.c').read_text()
main = (root / 'src/PC/ui_mp/ui_main_mp.c').read_text()
wwii = (root / 'src/PC/ui_mp/ui_wwii_menu.c').read_text()
q_shared = (root / 'src/PC/universal/q_shared.c').read_text()


def function(text, name):
    match = re.search(r'^((?:static )?(?:inline __attribute__\(\(always_inline\)\) )?[^\n;]*\b' + name +
                      r'\([^;]*?\))\s*\{', text, re.M)
    assert match, name
    brace = text.index('{', match.end() - 1)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


# The hooks. createserver's onOpen has loaded the arenas when the screen
# resets; the pointer leaves the hidden retail items alone; the covered menus
# paint nothing of their own; the screen takes the keys before the retail
# Escape and the items; StartServer names the room after the gametag.
for text, name in ((shared, 'ui_shared_mp.c'), (main, 'ui_main_mp.c')):
    assert text.count('#include "headers/PC/ui_mp/ui_match_menu.h"\n') == 1, name
menus_open = function(shared, 'Menus_Open')
assert (menus_open.index('Item_RunScript(dc, (itemDef_t *)tempItem, ((menuDef_t *)m)->onOpen);') <
        menus_open.index('    UI_MatchMenu_Opened(dc, menu);\n') < menus_open.index('Display_MouseMove(dc, NULL,'))
mouse_move = function(shared, 'Display_MouseMove')
assert (mouse_move.index('Menu_UpdatePosition((menuDef_t *)p);') <
        mouse_move.index('    if (UI_MatchMenu_Covers(dc, Menu_GetFocused(dc)))\n        return 1;\n') <
        mouse_move.index('Menu_HandleMouseMove(dc,'))
menu_paint = function(shared, 'Menu_Paint')
assert (menu_paint.index('paint_content:') <
        menu_paint.index('    if (UI_MatchMenu_Covers(dc, menu)) {\n        UI_MatchMenu_Paint(dc, menu);\n'
                         '        return;\n    }\n') <
        menu_paint.index('Window_Paint(dc, (void *)m,'))
key_event = function(main, 'UI_KeyEvent_impl')
assert (key_event.index('UI_ClassMenu_KeyEvent(') <
        key_event.index('} else if (UI_MatchMenu_KeyEvent((displayContextDef_t *)uiInfo, menu, key, down)) {') <
        key_event.index('key == 0x1b && down') < key_event.index('Menu_HandleKey(uiInfo, menu, key, down);'))
start_server = main.split('if (I_stricmp(name, "StartServer") == 0) {', 1)[1].split('#endif', 1)[0]
assert 'UI_MatchMenu_RoomName(roomName, sizeof(roomName));' in start_server
assert 'Web_CreateServer(roomName, ' in start_server and 'sv_hostname' not in start_server
assert 'UI_MatchMenu_Rules(sharedUiInfo.gameTypes[gameTypeIdx].gameType, &timeLimit, &scoreLimit);' in start_server

# The rules offered are those the gateway and rooms.py accept; each mode's
# own is among them, Team Deathmatch's server.cfg's; the dvars hold them all.
times = [int(value) for value in re.search(r's_matchTimeLimits\[\] = \{ ([\d, ]+) \};', source)[1].split(', ')]
rule_modes = {match[1]: (int(match[2]), int(match[3]), [int(value) for value in match[4].split(', ')])
              for match in re.finditer(r'\{ "(\w+)", (\d+), (\d+), \{ ([\d, ]+) \}', source)}
spec = importlib.util.spec_from_file_location('rooms', root / 'downstream/server/rooms.py')
rooms = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rooms)
gateway = (root / 'downstream/gateway/server.mjs').read_text()
gateway_scores = re.search(r'export const SCORE_LIMITS = \{(.*?)\};', gateway, re.S)[1]
assert times == list(rooms.TIME_LIMITS) == [
    int(value) for value in re.search(r'export const TIME_LIMITS = \[([\d, ]+)\];', gateway)[1].split(', ')]
assert sorted(rule_modes) == sorted(rooms.SUPPORTED_GAMETYPES)
assert ({gametype: values for gametype, (_, _, values) in rule_modes.items()} ==
        {gametype: list(values) for gametype, values in rooms.SCORE_LIMITS.items()} ==
        {match[1]: [int(value) for value in match[2].split(', ')]
         for match in re.finditer(r'(\w+): \[([\d, ]+)\]', gateway_scores)})
for gametype, (time_limit, score_limit, values) in rule_modes.items():
    assert time_limit in times and score_limit in values, gametype
server_cfg = (root / 'server.cfg').read_text()
assert rule_modes['tdm'][:2] == (int(re.search(r'set scr_tdm_timelimit "(\d+)"', server_cfg)[1]),
                                 int(re.search(r'set scr_tdm_scorelimit "(\d+)"', server_cfg)[1]))
assert main.count(f'Dvar_RegisterInt("ui_matchTimeLimit", -1, -1, {max(times)}, 0x1001);') == 2
assert main.count(
    f'Dvar_RegisterInt("ui_matchScoreLimit", -1, -1, {max(max(v) for _, _, v in rule_modes.values())}, 0x1001);') == 2
# Over createserver, bg shows only its backdrop, as under Create a Class.
assert 'UI_WWIIMenu_PageOpen(dc)' in function(wwii, 'UI_WWIIMenu_PaintItem')

Regular, Bold = Font('Regular'), Font('Bold')
UNIT = 1.2  # px a unit on the 1024 x 576 captures


def evidence(text):
    """The layout's numbers against the captures', as measured on them: the
    failures."""
    failures = []

    def define(name):
        match = re.search(r'#define MATCH_' + name + r' \(?(-?[\d.]+)f?', text)
        return float(match[1]) if match else float('nan')

    def check(ok, what):
        if not ok:
            failures.append(what)

    def bearing(font, letter, cap):
        return font.ink[font.cmap[ord(letter)]][0] * cap / font.cap

    def ink(font, word, cap, tracking=0.0):
        return font.ink_width(word, tracking) * cap

    # Titles: GAME MODE SELECT, MAP SELECT and BOT SETUP inked 185.0, 114.2
    # and 100.8 wide from 44.2 to 45.0, capitals 14 px solid on a baseline
    # at 143 px.
    cap, tracking = define('TITLE_CAP'), define('TITLE_TRACKING')
    for word, width in (('GAME MODE SELECT', 185.0), ('MAP SELECT', 114.2), ('BOT SETUP', 100.8)):
        check(abs(ink(Bold, word, cap, tracking) - width) < 1.0, word + ' width')
        check(44.0 < define('PAGE_X') + bearing(Bold, word[0], cap) < 45.4, word + ' ink left')
    check(abs(cap - 14 / UNIT) < 0.1 and abs(define('TITLE_BASELINE') - (143 - 56) / UNIT) < 0.1, 'title capitals')
    # The panels' titles: TEAM DEATHMATCH, SAINTE MARIE DU MONT and BOT COUNT
    # inked 182.5, 220.0 and 103.3 wide, from 284.2 and 390.0.
    cap, tracking = define('PANEL_TITLE_CAP'), define('PANEL_TITLE_TRACKING')
    for word, width in (('TEAM DEATHMATCH', 182.5), ('SAINTE MARIE DU MONT', 220.0), ('BOT COUNT', 103.3)):
        check(abs(ink(Bold, word, cap, tracking) - width) < 1.0, word + ' panel width')
    check(abs(define('PANEL_X') + bearing(Bold, 'T', cap) - 284.2) < 0.3, 'panel left')
    check(abs(define('BOTS_PANEL_X') + bearing(Bold, 'B', cap) - 390.0) < 0.4, 'bot panel left')
    # Lists: DOMINATION, CAPTURE THE FLAG and SEARCH AND DESTROY inked 58.3,
    # 94.2 and 107.5 wide from 53.3 to 54.2, on baselines 32 apart from 130.0
    # down to GRIDIRON's 385.8. A pixel at either end is 0.83 units.
    cap = define('LIST_CAP')
    for word, width in (('DOMINATION', 58.3), ('CAPTURE THE FLAG', 94.2), ('SEARCH AND DESTROY', 107.5)):
        check(abs(ink(Regular, word, cap) - width) < 1.5, word + ' width')
    check(53.0 < define('TEXT_X') + bearing(Regular, 'D', cap) < 54.4, 'list ink left')
    check(abs(define('LIST_TOP') - 130.0) < 0.6 and abs(define('LIST_TOP') + 8 * define('ROW_SPACING') - 385.8) < 0.6,
          'list rows')
    check(abs(define('FOCUS_INDENT') - (74 - 64) / UNIT) < 0.2, 'focus indent')
    # The match page: GAME RULES and BOTS inked 67.5 and 26.7 wide; the boxes
    # 186.6 by 48.3 from 44.2 at 111.7 and 165.0 down; MAP 25.0 wide idle;
    # values 82.5 wide idle and 98.3 focused; the first row on a baseline at
    # 236.7, under the boxes.
    cap = define('ROW_CAP')
    check(abs(ink(Regular, 'GAME RULES', cap) - 67.5) < 0.8 and abs(ink(Regular, 'BOTS', cap) - 26.7) < 0.8, 'rows')
    check(abs(ink(Bold, 'MAP', define('BOX_NAME_CAP')) - 25.0) < 0.8, 'box name')
    check(abs(ink(Bold, 'MODE', define('BOX_FOCUS_NAME_CAP')) - 35.0) < 1.0, 'focused box name')
    check(abs(ink(Regular, 'SAINTE MARIE DU MONT', define('BOX_VALUE_CAP')) - 82.5) < 1.0, 'box value')
    check(abs(ink(Regular, 'SAINTE MARIE DU MONT', define('BOX_FOCUS_VALUE_CAP')) - 98.3) < 1.5, 'focused value')
    top, height, gap = define('BOX_TOP'), define('BOX_H'), define('GAP')
    check(abs(define('BOX_X') - 53 / UNIT) < 0.3 and abs(define('BOX_W') - (277 - 53) / UNIT) < 0.3, 'box columns')
    check(abs(top - (190 - 56) / UNIT) < 0.3 and abs(top + height - (248 - 56) / UNIT) < 0.3, 'box rows')
    check(abs(top + height + gap - (254 - 56) / UNIT) < 0.3, 'box gap')
    check(abs(top + define('BOX_VALUE_BASELINE') - 150.0) < 0.4, 'value baseline')
    check(abs(top + 2 * (height + gap) + define('ROW_ABOVE') - 236.7) < 0.4, 'first row')
    check(abs(define('ROW_H') - (222 - 190) / UNIT) < 0.3, 'row frame')
    # The chevron's ink ends 51.7 from the edge; » is inked to 0.653 of a
    # Regular capital.
    check(abs(define('BOX_X') + define('CHEVRON_X') + 0.653 * define('BOX_FOCUS_NAME_CAP') - 51.7) < 0.4, 'chevron')
    # The dimmed page is inked from 270.0, 216.7 right of the column.
    check(abs(define('TEXT_X') + define('PREVIEW_SHIFT') + bearing(Regular, 'S', define('LIST_CAP')) - 270.0) < 0.6,
          'preview')
    # Bot setup: values centred on 248.0, arrows on 175.8 and 321.25, 5.8
    # wide and 9.2 tall, the focused row framed to 404 px.
    center = define('TEXT_X') + define('VALUE_OFFSET')
    check(abs(center - 248.0) < 0.4, 'value centre')
    check(abs(center - define('ARROW_OFFSET') - 175.8) < 0.7 and abs(center + define('ARROW_OFFSET') - 321.25) < 0.7,
          'arrows')
    check(abs(define('SETTING_RIGHT') - 404 / UNIT) < 0.3, 'setting frame')
    # The map's image spans 341 to 688 px and rows 178 to 376: a 4:3
    # levelshot cropped to 16:9.
    check(abs(define('SHOT_X') - 341 / UNIT) < 0.3 and abs(define('SHOT_W') - (688 - 341) / UNIT) < 0.3, 'image span')
    check(abs(define('SHOT_TOP') - (178 - 56) / UNIT) < 0.3 and abs(define('SHOT_H') - define('SHOT_W') * 9 / 16) < 0.1,
          'image rows')
    check(abs(define('SHOT_CROP') - (1 - (4 / 3) / (16 / 9)) / 2) < 1e-6, 'image crop')
    return failures


failures = evidence(source)
assert not failures, failures

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
typedef struct Material { const char *name; } Material;
typedef Material *MaterialHandle;
typedef struct itemDef_s itemDef_t;
typedef struct HudVertex2D { float xy[2]; float st[2]; float rgba[4]; } HudVertex2D;
typedef struct Window { const char *name; int dynamicFlags[4]; } Window;
typedef struct menuDef_t { Window window; } menuDef_t;
typedef struct displayContextDef_t { int realTime; int cursorx, cursory; menuDef_t *menuStack[16]; int openMenuCount; }
    displayContextDef_t;
typedef struct mapInfo { const char *mapName, *mapLoadName, *imageName, *opponentName; int teamMembers, typeBits,
    cinematic, timeToBeat[32]; MaterialHandle levelShot; qboolean active; } mapInfo;
typedef struct gameTypeInfo { const char *gameType, *gameTypeName; } gameTypeInfo;
typedef struct sharedUiInfo_t { struct { MaterialHandle whiteMaterial; } assets; int numGameTypes;
    gameTypeInfo gameTypes[32]; int mapCount; mapInfo mapList[128]; } sharedUiInfo_t;
static Material white = { "white" }, shots[16];
sharedUiInfo_t sharedUiInfo = { { &white } };

/* 1280 x 720: 1.5 px a unit from the left and top edges; the cursor's 640 by
 * 480 stretched over the screen. */
static const float S = 1.5f, FULL_X = 2.0f, FULL_Y = 1.5f;
void CalcScreenX(float *x, int horzAlign) {
    assert(horzAlign == 1 || horzAlign == 4);
    *x *= horzAlign == 4 ? FULL_X : S;
}
void CalcScreenY(float *y, int vertAlign) {
    assert(vertAlign == 1 || vertAlign == 4);
    *y *= vertAlign == 4 ? FULL_Y : S;
}
void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign) {
    CalcScreenX(x, horzAlign);
    CalcScreenY(y, vertAlign);
    *w *= horzAlign == 4 ? FULL_X : S;
    *h *= vertAlign == 4 ? FULL_Y : S;
}

static struct { const char *name; int value; char string[64]; } dvars[] = {
    { "ui_netGametype" }, { "ui_netGametypeName" }, { "ui_currentNetMap" }, { "ui_botCount" },
    { "ui_botDifficulty", 1 }, { "ui_matchTimeLimit", -1 }, { "ui_matchScoreLimit", -1 }, { "name" } };
static int dvarSets;
static int dvarIndex(const char *name) {
    for (int i = 0; i < (int)(sizeof(dvars) / sizeof(dvars[0])); i++)
        if (!strcmp(dvars[i].name, name))
            return i;
    assert(!"unknown dvar");
    return 0;
}
int Dvar_GetInt(const char *name) { return dvars[dvarIndex(name)].value; }
const char *Dvar_GetString(const char *name) { return dvars[dvarIndex(name)].string; }
void Dvar_SetIntByName(const char *name, int value) { dvars[dvarIndex(name)].value = value; dvarSets++; }
void Dvar_SetStringByName(const char *name, const char *value) {
    snprintf(dvars[dvarIndex(name)].string, sizeof(dvars[0].string), "%s", value);
    dvarSets++;
}
#define DVAR(name) (dvars[dvarIndex(name)].value)
#define DVAR_STRING(name) (dvars[dvarIndex(name)].string)

int I_stricmp(const char *s0, const char *s1) { return strcasecmp(s0, s1); }
int I_strnicmp(const char *s0, const char *s1, size_t n) { return strncasecmp(s0, s1, n); }
void I_strncpyz(char *dest, const char *src, int destsize) { snprintf(dest, (size_t)destsize, "%s", src); }
''' + function(q_shared, 'I_CleanStr') + r'''

static const char *const translations[][2] = {
    { "MPUI_CAPTURE_THE_FLAG", "Capture the Flag" }, { "MPUI_DEATHMATCH", "Deathmatch" },
    { "MPUI_HEADQUARTERS", "Headquarters" }, { "MPUI_SEARCH_AND_DESTROY", "Search and Destroy" },
    { "MPUI_TEAM_DEATHMATCH", "Team Deathmatch" }, { "EXE_ALL", "All" } };
/* As the retail one: an unknown reference comes back in a shared buffer. */
const char *UI_SafeTranslateString(const char *reference) {
    static char unknown[256];
    for (int i = 0; i < (int)(sizeof(translations) / sizeof(translations[0])); i++)
        if (!strcmp(translations[i][0], reference))
            return translations[i][1];
    snprintf(unknown, sizeof(unknown), "%s", reference);
    return unknown;
}
/* The bundle's strings, but MENU_SD_DESC missing and one blank. */
static const char *const stringed[][2] = {
    { "MENU_CTF_DESC", "Stringed CTF." },
    { "MENU_HQ_DESC", "Stringed HQ. Hold the radio. Hold the radio. Hold the radio. Hold the radio. Hold the radio. "
                      "Hold the radio. Hold the radio. Hold the radio. Hold the radio. Hold the radio. Hold the radio. "
                      "Hold the radio. Hold the radio. Hold the radio. Hold the radio. Hold the radio. Hold the radio. "
                      "Hold the radio. Hold the radio. Hold the radio. Hold the radio. Hold the radio. The end." },
    { "MP_GAIN_POINTS_BY_ELIMINATING1_NOSCORE", "Stringed TDM." }, { "MP_GAIN_POINTS_BY_ELIMINATING_NOSCORE", "" } };
const char *SEH_StringEd_GetString(const char *reference) {
    for (int i = 0; i < (int)(sizeof(stringed) / sizeof(stringed[0])); i++)
        if (!strcmp(stringed[i][0], reference))
            return stringed[i][1];
    return NULL;
}

static char closed[64], script[64];
static int closes, scripts, overs, clicks;
static displayContextDef_t dc;
static menuDef_t createserver = { { "createserver", { 4 } } }, serversettings = { { "createserver_serversettings", { 6 } } };
static menuDef_t mapsMenu = { { "createserver_maps", { 4 } } }, mainText = { { "main_text", { 6 } } };
static menuDef_t bg = { { "bg", { 4 } } };
void Menus_CloseByName(displayContextDef_t *context, const char *name) {
    assert(context == &dc);
    snprintf(closed, sizeof(closed), "%s", name);
    closes++;
}
int UI_PlayLocalSoundAliasByName(const char *name) {
    if (!strcmp(name, "mouse_over")) overs++;
    else if (!strcmp(name, "mouse_click")) clicks++;
    else assert(!"unknown sound");
    return 1;
}
void UI_RunMenuScript(const char **args) {
    snprintf(script, sizeof(script), "%s", *args);
    scripts++;
}

enum { TEXT = 1, PIC, TRI, PROMPTS };
typedef struct { int kind; char text[160]; int bold; float x, y, w, h, cap, tracking, s1, t1, s2, t2; vec4_t color;
    int horz, vert; MaterialHandle material; float xy[3][2]; char keys[96]; } Drawn;
static Drawn drawn[400];
static int drawCount;
static Drawn *record(int kind, const vec_t *color) {
    assert(drawCount < 400);
    Drawn *d = &drawn[drawCount++];
    memset(d, 0, sizeof(*d));
    d->kind = kind;
    if (color)
        memcpy(d->color, color, sizeof(d->color));
    return d;
}
void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2,
                       float t2, const vec_t *color, MaterialHandle material) {
    Drawn *d = record(PIC, color);
    d->x = x; d->y = y; d->w = w; d->h = h; d->horz = horzAlign; d->vert = vertAlign;
    d->s1 = s1; d->t1 = t1; d->s2 = s2; d->t2 = t2; d->material = material;
}
void R_AddCmdDrawTriangles2D(const HudVertex2D *verts, int vertexCount, const unsigned short *indices, int indexCount,
                             MaterialHandle material) {
    Drawn *d = record(TRI, verts[0].rgba);
    assert(vertexCount == 3 && indexCount == 3 && indices[0] == 0 && indices[1] == 1 && indices[2] == 2);
    for (int i = 0; i < 3; i++) {
        d->xy[i][0] = verts[i].xy[0];
        d->xy[i][1] = verts[i].xy[1];
        assert(!memcmp(verts[i].rgba, verts[0].rgba, sizeof(verts[i].rgba)));
    }
    d->material = material;
}
''' + wwii_header + r'''
static const vec4_t palette[] = { { 0.776f, 0.776f, 0.769f, 1 }, { 0.80f, 0.72f, 0.47f, 1 },
    { 0.851f, 0.843f, 0.827f, 1 }, { 0.812f, 0.812f, 0.812f, 1 }, { 1, 1, 1, 1 } };
const float *UI_WWIIMenu_Color(int color) { assert(color >= 0 && color < 5); return palette[color]; }
/* Each letter 0.55 capitals wide, 0.62 in Bold. */
static float width(const char *text, int bold, float cap, float tracking) {
    float n = (float)strlen(text);
    return n * cap * (bold ? 0.62f : 0.55f) + (n > 1 ? tracking * cap * (n - 1) : 0);
}
float UI_WWIIMenu_Width(const char *text, int bold, float cap, float tracking) { return width(text, bold, cap, tracking); }
void UI_WWIIMenu_Text(const char *text, int bold, float x, float baseline, int horzAlign, int vertAlign, float cap,
                      float tracking, const vec_t *color) {
    Drawn *d = record(TEXT, color);
    assert(strlen(text) < sizeof(d->text));
    snprintf(d->text, sizeof(d->text), "%s", text);
    d->bold = bold; d->x = x; d->y = baseline; d->horz = horzAlign; d->vert = vertAlign; d->cap = cap;
    d->tracking = tracking;
}
void UI_WWIIMenu_Prompts(const char *const *keys, const char *const *labels, int count, const char *rightKey,
                         const char *rightLabel) {
    Drawn *d = record(PROMPTS, NULL);
    assert(count == 1);
    snprintf(d->keys, sizeof(d->keys), "%s %s|%s %s", keys[0], labels[0], rightKey, rightLabel);
}
''' + header

checks = r'''
static int near(float a, float b) { return fabsf(a - b) < 2e-3f; }
static int sameColor(const float *a, const float *b) {
    return near(a[0], b[0]) && near(a[1], b[1]) && near(a[2], b[2]) && near(a[3], b[3]);
}
static const float idle[4] = { 0.776f, 0.776f, 0.769f, 1 }, gold[4] = { 0.80f, 0.72f, 0.47f, 1 };
static const float light[4] = { 0.851f, 0.843f, 0.827f, 1 }, whiteColor[4] = { 1, 1, 1, 1 };
static const float note[4] = { 0.70f, 0.70f, 0.68f, 1 }, dim[4] = { 0.776f, 0.776f, 0.769f, 0.32f };
static const float shade[4] = { 0, 0, 0, 0.55f }, boxFill[4] = { 0, 0, 0, 0.3f }, focusFill[4] = { 0, 0, 0, 0.85f };
static const float band[4] = { 0.13f, 0.125f, 0.10f, 0.7f }, edge[4] = { 1, 1, 1, 0.28f };
static const float focusEdge[4] = { 0.80f, 0.72f, 0.47f, 0.55f }, rowFill[4] = { 0, 0, 0, 0.35f };

static const Drawn *find(const char *text) {
    for (int i = 0; i < drawCount; i++)
        if (drawn[i].kind == TEXT && !strcmp(drawn[i].text, text))
            return &drawn[i];
    return NULL;
}
static int count(const char *text) {
    int n = 0;
    for (int i = 0; i < drawCount; i++)
        n += drawn[i].kind == TEXT && !strcmp(drawn[i].text, text);
    return n;
}
static int text(const char *s, int bold, float x, float y, float cap, float tracking, const float *color) {
    const Drawn *d = find(s);
    return d && d->bold == bold && near(d->x, x) && near(d->y, y) && near(d->cap, cap) && near(d->tracking, tracking) &&
           sameColor(d->color, color) && d->horz == 1 && d->vert == 1;
}
static int pic(float x, float y, float w, float h, const float *color, MaterialHandle material) {
    for (int i = 0; i < drawCount; i++) {
        const Drawn *d = &drawn[i];
        if (d->kind == PIC && near(d->x, x) && near(d->y, y) && near(d->w, w) && near(d->h, h) &&
            sameColor(d->color, color) && d->material == material && d->horz == 1 && d->vert == 1 && near(d->s1, 0) &&
            near(d->s2, 1) && ((material == &white && near(d->t1, 0) && near(d->t2, 1)) || material != &white))
            return 1;
    }
    return 0;
}
static int pics(void) {
    int n = 0;
    for (int i = 0; i < drawCount; i++)
        n += drawn[i].kind == PIC;
    return n;
}
static int tris(void) {
    int n = 0;
    for (int i = 0; i < drawCount; i++)
        n += drawn[i].kind == TRI;
    return n;
}
/* The box's fill, its band when focused, its frame without overlaps, and its
 * gold edge when focused. */
static int box(float top, int focused) {
    const float x = 44.2f, w = 186.6f, h = 48.3f, l = 0.6f;
    const float *frame = focused ? focusEdge : edge;
    return pic(x, top, w, h, focused ? focusFill : boxFill, &white) && pic(x, top, w, l, frame, &white) &&
           pic(x, top + h - l, w, l, frame, &white) && pic(x + w - l, top + l, l, h - 2 * l, frame, &white) &&
           pic(x, top + l, l, h - 2 * l, frame, &white) &&
           (focused ? pic(x, top, w, 0.26f * h, band, &white) && pic(x, top, 2.5f, h, gold, &white)
                    : !pic(x, top, 2.5f, h, gold, &white));
}
/* A focused row's fill and frame from the screen's edge, without a left
 * line. */
static int row(float baseline, float right) {
    const float y = baseline - 18.3f, h = 26.7f, l = 0.6f;
    return pic(0, y, right, h, rowFill, &white) && pic(0, y, right, l, focusEdge, &white) &&
           pic(0, y + h - l, right, l, focusEdge, &white) && pic(right - l, y + l, l, h - 2 * l, focusEdge, &white) &&
           !pic(0, y + l, l, h - 2 * l, focusEdge, &white);
}
static void open(void) {
    dc.menuStack[0] = &bg;
    dc.menuStack[1] = &createserver;
    dc.menuStack[2] = &mapsMenu;
    dc.menuStack[3] = &serversettings;
    dc.openMenuCount = 4;
    UI_MatchMenu_Opened(&dc, &createserver);
}
static void paint(void) {
    drawCount = 0;
    UI_MatchMenu_Paint(&dc, &createserver);
}
static int key(int k) {
    int taken = UI_MatchMenu_KeyEvent(&dc, &serversettings, k, 1);
    assert(UI_MatchMenu_KeyEvent(&dc, &serversettings, k, 0) == taken);
    return taken;
}
/* The pointer over a point units from the left and top edges. */
static void point(float x, float y) {
    dc.cursorx = (int)lroundf(x * S / FULL_X);
    dc.cursory = (int)lroundf(y * S / FULL_Y);
}
static float baseline(int row) { return 129.6f + 32.0f * (float)row; }
static float centered(const char *value, int bold, float pen) { return pen + 195.5f - 0.5f * width(value, bold, 7.5f, 0); }
static int prompts(void) {
    for (int i = 0; i < drawCount; i++)
        if (drawn[i].kind == PROMPTS)
            return !strcmp(drawn[i].keys, "ENTER Select|ESC Back") && i == drawCount - 1;
    return 0;
}
/* The note's words in Regular lines at x, leading apart from baseline, each
 * as long as fits in limit, at most four: their count, else -1. */
static int wrapped(const char *note_, float x, float y, float limit) {
    char expected[512] = "", joined[512] = "";
    const Drawn *previous = NULL;
    int lines = 0, n = 0;
    for (const char *c = note_; *c; c++)
        if (*c != ' ' || (n && expected[n - 1] != ' ' && c[1]))
            expected[n++] = *c;
    expected[n] = '\0';
    for (int i = 0; i < drawCount; i++) {
        const Drawn *d = &drawn[i];
        char trial[600];
        if (d->kind != TEXT || d->bold || !near(d->x, x) || !near(d->cap, 6.6f))
            continue;
        if (!near(d->y, y + 10.0f * (float)lines) || !sameColor(d->color, note) || width(d->text, 0, 6.6f, 0) > limit)
            return -1;
        snprintf(trial, sizeof(trial), "%s %.*s", previous ? previous->text : "", (int)strcspn(d->text, " "), d->text);
        if (previous && width(trial, 0, 6.6f, 0) <= limit)
            return -1;
        if (joined[0])
            strcat(joined, " ");
        strcat(joined, d->text);
        previous = d;
        lines++;
    }
    return !strcmp(joined, expected) && lines <= 4 ? lines : -1;
}

static const char *const modes[5][2] = { { "ctf", "MPUI_CAPTURE_THE_FLAG" }, { "dm", "MPUI_DEATHMATCH" },
    { "hq", "MPUI_HEADQUARTERS" }, { "sd", "MPUI_SEARCH_AND_DESTROY" }, { "tdm", "MPUI_TEAM_DEATHMATCH" } };
static const char *const maps[3][2] = { { "mp_carentan", "Carentan, France" }, { "mp_dawnville", "Dawnville, France" },
    { "mp_toujane", "Toujane, Tunisia" } };
static void arenas(void) {
    memset(&sharedUiInfo.gameTypes, 0, sizeof(sharedUiInfo.gameTypes));
    memset(&sharedUiInfo.mapList, 0, sizeof(sharedUiInfo.mapList));
    sharedUiInfo.numGameTypes = 5;
    for (int i = 0; i < 5; i++) {
        sharedUiInfo.gameTypes[i].gameType = modes[i][0];
        sharedUiInfo.gameTypes[i].gameTypeName = modes[i][1];
    }
    sharedUiInfo.mapCount = 3;
    for (int i = 0; i < 3; i++) {
        sharedUiInfo.mapList[i].mapLoadName = maps[i][0];
        sharedUiInfo.mapList[i].mapName = maps[i][1];
        sharedUiInfo.mapList[i].typeBits = -1;
        sharedUiInfo.mapList[i].levelShot = &shots[i];
    }
}

int main(void) {
    int cases = 0;

    arenas();
    DVAR("ui_netGametype") = 4;
    DVAR("ui_currentNetMap") = 0;
    snprintf(DVAR_STRING("name"), 64, "^1Ton^7ga");

    /* Closed, the screen covers nothing, takes no key and paints nothing. */
    dc.menuStack[0] = &bg;
    dc.menuStack[1] = &mainText;
    dc.openMenuCount = 2;
    assert(!UI_MatchMenu_Covers(&dc, &serversettings) && !UI_MatchMenu_KeyEvent(&dc, &serversettings, 0x0d, 1));
    paint();
    assert(drawCount == 0);
    UI_MatchMenu_Opened(&dc, &mainText);
    assert(dvarSets == 0);
    cases++;
    /* Open, it covers createserver and the createserver_ menus over it, not
     * bg or main_text, and only while createserver is visible. */
    open();
    assert(UI_MatchMenu_Covers(&dc, &createserver) && UI_MatchMenu_Covers(&dc, &serversettings) &&
           UI_MatchMenu_Covers(&dc, &mapsMenu));
    assert(!UI_MatchMenu_Covers(&dc, &bg) && !UI_MatchMenu_Covers(&dc, &mainText) && !UI_MatchMenu_Covers(&dc, NULL));
    assert(!UI_MatchMenu_KeyEvent(&dc, &mainText, 0x0d, 1));
    createserver.window.dynamicFlags[0] = 0;
    assert(!UI_MatchMenu_Covers(&dc, &serversettings));
    paint();
    assert(drawCount == 0);
    createserver.window.dynamicFlags[0] = 4;
    cases += 2;

    /* The match page: the dark scene, the title and its note, the focused
     * Mode box in gold with the modes dimmed beside it, the Map box, the Game
     * Rules and Bots rows a row apart and the Start Match box; Enter and
     * Escape prompts last. */
    paint();
    assert(drawn[0].kind == PIC && drawn[0].horz == 4 && drawn[0].vert == 4 && near(drawn[0].w, 640) &&
           near(drawn[0].h, 480) && sameColor(drawn[0].color, shade) && drawn[0].material == &white);
    assert(text("MATCH", 1, 44.0f, 72.5f, 11.7f, 0.2f, gold));
    assert(text("Customize the game you want to play", 0, 44.0f, 90.0f, 6.1f, 0, note));
    assert(box(111.7f, 1) && text("\xbb", 0, 45.2f, 138.7f, 10.0f, 0, gold));
    assert(text("MODE", 1, 52.5f, 138.7f, 10.0f, 0, gold) && text("TEAM DEATHMATCH", 0, 52.5f, 150.0f, 6.5f, 0, note));
    assert(box(165.0f, 0) && text("MAP", 1, 52.5f, 192.0f, 9.0f, 0, light));
    assert(text("CARENTAN, FRANCE", 0, 52.5f, 203.3f, 5.5f, 0, note));
    assert(text("GAME RULES", 0, 52.5f, 236.6f, 8.3f, 0, idle) && !row(236.6f, 230.8f));
    assert(text("BOTS", 0, 52.5f, 268.6f, 8.3f, 0, idle) && !row(268.6f, 230.8f));
    assert(box(282.0f, 0) && text("START MATCH", 1, 52.5f, 309.0f, 9.0f, 0, light));
    assert(text("CREATE YOUR SERVER AND JOIN IT", 0, 52.5f, 320.3f, 5.5f, 0, note));
    {
        static const char *const names[5] = { "CAPTURE THE FLAG", "DEATHMATCH", "HEADQUARTERS", "SEARCH AND DESTROY",
                                               "TEAM DEATHMATCH" };
        for (int i = 0; i < 5; i++)
            assert(text(names[i], 0, 269.0f, baseline(i), 7.5f, 0, dim) || (i == 4 && count(names[i]) == 2));
        for (int i = 0; i < drawCount; i++)
            if (drawn[i].kind == TEXT && !strcmp(drawn[i].text, "TEAM DEATHMATCH") && near(drawn[i].x, 269.0f))
                assert(near(drawn[i].y, baseline(4)) && sameColor(drawn[i].color, dim) && !drawn[i].bold);
    }
    assert(prompts() && tris() == 0);
    cases++;

    /* Down goes to Map, gold and larger, the maps dimmed beside it; then the
     * Game Rules row in its frame, gold and Bold further right, Team
     * Deathmatch's rules dimmed; then the Bots row, its settings dimmed;
     * then Start Match, what it creates dimmed, its name the gametag without
     * colors; then round to Mode. Tab and the wheel go too. */
    overs = 0;
    assert(key(0x9b) && s_match.focus[0] == 1 && overs == 1);
    paint();
    assert(box(111.7f, 0) && box(165.0f, 1) && text("MAP", 1, 52.5f, 192.0f, 10.0f, 0, gold));
    assert(text("CARENTAN, FRANCE", 0, 52.5f, 203.3f, 6.5f, 0, note) && text("MODE", 1, 52.5f, 138.7f, 9.0f, 0, light));
    assert(text("DAWNVILLE, FRANCE", 0, 269.0f, baseline(1), 7.5f, 0, dim) &&
           text("TOUJANE, TUNISIA", 0, 269.0f, baseline(2), 7.5f, 0, dim));
    assert(key(0x09) && s_match.focus[0] == 2);
    paint();
    assert(row(236.6f, 230.8f) && text("GAME RULES", 1, 60.8f, 236.6f, 8.3f, 0, gold) && !row(268.6f, 230.8f));
    assert(text("Time Limit", 0, 269.0f, baseline(0), 8.3f, 0, dim) &&
           text("15 Minutes", 0, centered("15 Minutes", 0, 269.0f), baseline(0), 7.5f, 0, dim));
    assert(text("Score Limit", 0, 269.0f, baseline(1), 8.3f, 0, dim) &&
           text("100 Kills", 0, centered("100 Kills", 0, 269.0f), baseline(1), 7.5f, 0, dim));
    assert(key(0xcd) && s_match.focus[0] == 3);
    paint();
    assert(row(268.6f, 230.8f) && text("BOTS", 1, 60.8f, 268.6f, 8.3f, 0, gold) && !row(236.6f, 230.8f));
    assert(text("Bot Count", 0, 269.0f, baseline(0), 8.3f, 0, dim) &&
           text("None", 0, centered("None", 0, 269.0f), baseline(0), 7.5f, 0, dim));
    assert(text("Difficulty", 0, 269.0f, baseline(1), 8.3f, 0, dim) &&
           text("Normal", 0, centered("Normal", 0, 269.0f), baseline(1), 7.5f, 0, dim));
    assert(key(0x9b) && s_match.focus[0] == 4);
    paint();
    assert(box(282.0f, 1) && text("START MATCH", 1, 52.5f, 309.0f, 10.0f, 0, gold));
    assert(text("Mode", 0, 269.0f, baseline(0), 8.3f, 0, dim) &&
           text("Team Deathmatch", 0, centered("Team Deathmatch", 0, 269.0f), baseline(0), 7.5f, 0, dim));
    assert(text("Map", 0, 269.0f, baseline(1), 8.3f, 0, dim) && find("Carentan, France"));
    assert(text("Time Limit", 0, 269.0f, baseline(2), 8.3f, 0, dim) &&
           text("15 Minutes", 0, centered("15 Minutes", 0, 269.0f), baseline(2), 7.5f, 0, dim));
    assert(text("Score Limit", 0, 269.0f, baseline(3), 8.3f, 0, dim) &&
           text("100 Kills", 0, centered("100 Kills", 0, 269.0f), baseline(3), 7.5f, 0, dim));
    assert(text("Bots", 0, 269.0f, baseline(4), 8.3f, 0, dim) && find("None"));
    assert(text("Server Name", 0, 269.0f, baseline(5), 8.3f, 0, dim) &&
           text("Tonga", 0, centered("Tonga", 0, 269.0f), baseline(5), 7.5f, 0, dim));
    assert(key(0x9b) && s_match.focus[0] == 0 && key(0xce) && s_match.focus[0] == 4 && key(0x9a) &&
           s_match.focus[0] == 3);
    /* Left, right and characters change nothing here; every key is taken. */
    assert(key(0x9c) && key(0x9d) && key(0x9d) && key(0x400 | 'a') && s_match.focus[0] == 3 && s_match.page == 0);
    assert(DVAR("ui_botCount") == 0 && DVAR("ui_botDifficulty") == 1);
    assert(DVAR("ui_matchTimeLimit") == -1 && DVAR("ui_matchScoreLimit") == -1);
    assert(UI_MatchMenu_KeyEvent(&dc, &serversettings, 'x', 1) && s_match.focus[0] == 3);
    cases += 4;

    /* Bots: the page's two settings, the focused one's value Bold and white
     * between arrows, the other's idle; the panel names and explains it. */
    clicks = 0;
    s_match.focus[0] = 3;
    assert(key(0x0d) && s_match.page == 3 && s_match.focus[3] == 0 && clicks == 1);
    paint();
    assert(text("BOT SETUP", 1, 44.0f, 72.5f, 11.7f, 0.2f, gold) &&
           find("Set the number and difficulty of bots"));
    assert(row(baseline(0), 336.7f) && text("Bot Count", 1, 60.8f, baseline(0), 8.3f, 0, gold));
    assert(text("None", 1, centered("None", 1, 52.5f), baseline(0), 7.5f, 0, whiteColor));
    assert(text("Difficulty", 0, 52.5f, baseline(1), 8.3f, 0, idle) &&
           text("Normal", 0, centered("Normal", 0, 52.5f), baseline(1), 7.5f, 0, idle));
    assert(text("BOT COUNT", 1, 389.0f, 113.3f, 12.5f, 0.13f, gold));
    assert(wrapped("Bots join the match on their own. Each one takes a player slot.", 389.0f, 127.5f, 220.0f) == 2);
    {
        /* Arrows 5.8 by 9.2 on 175.3 and 320.7, 4.0 over the baseline, the
         * left one pointing left, both wound the same way, in the light
         * title color. */
        const Drawn *left = NULL, *right = NULL;
        for (int i = 0; i < drawCount; i++)
            if (drawn[i].kind == TRI)
                *(left ? &right : &left) = &drawn[i];
        assert(tris() == 2 && left && right && left->material == &white && sameColor(left->color, light));
        float x0 = (175.3f - 2.9f) * S, x1 = (175.3f + 2.9f) * S, y0 = (125.6f - 4.6f) * S, y1 = (125.6f + 4.6f) * S;
        assert(near(left->xy[0][0], x1) && near(left->xy[0][1], y0) && near(left->xy[1][0], x0) &&
               near(left->xy[1][1], 0.5f * (y0 + y1)) && near(left->xy[2][0], x1) && near(left->xy[2][1], y1));
        x0 = (320.7f - 2.9f) * S;
        x1 = (320.7f + 2.9f) * S;
        assert(near(right->xy[0][0], x0) && near(right->xy[0][1], y0) && near(right->xy[1][0], x0) &&
               near(right->xy[1][1], y1) && near(right->xy[2][0], x1) && near(right->xy[2][1], 0.5f * (y0 + y1)));
        float a = (left->xy[1][0] - left->xy[0][0]) * (left->xy[2][1] - left->xy[0][1]) -
                  (left->xy[1][1] - left->xy[0][1]) * (left->xy[2][0] - left->xy[0][0]);
        float b = (right->xy[1][0] - right->xy[0][0]) * (right->xy[2][1] - right->xy[0][1]) -
                  (right->xy[1][1] - right->xy[0][1]) * (right->xy[2][0] - right->xy[0][0]);
        assert(a * b > 0);
    }
    assert(prompts());
    cases += 2;
    /* Right steps the count through every one to 16, then 20, 24, 28 and 32,
     * and round to none; Left goes back; Enter steps on. A count between
     * presets goes to the presets around it. */
    {
        static const int counts[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 20, 24, 28, 32, 0 };
        clicks = 0;
        for (int i = 0; i < 21; i++)
            assert(key(0x9d) && DVAR("ui_botCount") == counts[i]);
        assert(clicks == 21 && key(0x9c) && DVAR("ui_botCount") == 32 && key(0x9c) && DVAR("ui_botCount") == 28);
        assert(key(0x0d) && DVAR("ui_botCount") == 32 && clicks == 24);
        DVAR("ui_botCount") = 17;
        assert(key(0x9d) && DVAR("ui_botCount") == 20);
        DVAR("ui_botCount") = 17;
        assert(key(0x9c) && DVAR("ui_botCount") == 16);
        DVAR("ui_botCount") = 17;
        paint();
        assert(find("17"));
        DVAR("ui_botCount") = 8;
        cases += 3;
    }
    /* Difficulty: Easy, Normal and Hard round both ways; out of range is
     * Normal; the panel explains the chosen level. */
    assert(key(0x9b) && s_match.focus[3] == 1);
    paint();
    assert(row(baseline(1), 336.7f) && text("Difficulty", 1, 60.8f, baseline(1), 8.3f, 0, gold));
    assert(text("8", 0, centered("8", 0, 52.5f), baseline(0), 7.5f, 0, idle) && text("DIFFICULTY", 1, 389.0f, 113.3f,
                                                                                       12.5f, 0.13f, gold));
    assert(wrapped("Normal bots balance their reactions, turning and accuracy.", 389.0f, 127.5f, 220.0f) == 1);
    assert(key(0x9d) && DVAR("ui_botDifficulty") == 2 && key(0x9d) && DVAR("ui_botDifficulty") == 0 && key(0x9c) &&
           DVAR("ui_botDifficulty") == 2);
    paint();
    assert(text("Hard", 1, centered("Hard", 1, 52.5f), baseline(1), 7.5f, 0, whiteColor) &&
           wrapped("Hard bots react sooner, turn faster and aim more accurately.", 389.0f, 127.5f, 220.0f) == 1);
    assert(key(0x9d) && DVAR("ui_botDifficulty") == 0);
    paint();
    assert(wrapped("Easy bots react later, turn slower and aim less accurately.", 389.0f, 127.5f, 220.0f) == 1);
    assert(key(0x9c) && DVAR("ui_botDifficulty") == 2);
    DVAR("ui_botDifficulty") = 7;
    paint();
    assert(find("Normal") && key(0x9d) && DVAR("ui_botDifficulty") == 2);
    assert(key(0x9b) && s_match.focus[3] == 0 && key(0x9a) && s_match.focus[3] == 1);
    cases += 2;
    /* Escape goes back to the match page, its Bots row focused, which shows
     * the new settings; the bot count at Start Match. */
    assert(key(0x1b) && s_match.page == 0 && s_match.focus[0] == 3 && closes == 0);
    paint();
    assert(text("8", 0, centered("8", 0, 269.0f), baseline(0), 7.5f, 0, dim) && find("Hard"));
    s_match.focus[0] = 4;
    paint();
    assert(find("8, Hard"));
    cases++;

    /* Game Rules: Time Limit and Score Limit as Bot Setup's settings, from
     * Team Deathmatch's own; the panel names the focused one and explains
     * it. */
    clicks = 0;
    s_match.focus[0] = 2;
    assert(key(0x0d) && s_match.page == 4 && s_match.focus[4] == 0 && clicks == 1);
    paint();
    assert(text("GAME RULES", 1, 44.0f, 72.5f, 11.7f, 0.2f, gold) &&
           find("Set how long the match lasts and the score that wins it"));
    assert(row(baseline(0), 336.7f) && text("Time Limit", 1, 60.8f, baseline(0), 8.3f, 0, gold));
    assert(text("15 Minutes", 1, centered("15 Minutes", 1, 52.5f), baseline(0), 7.5f, 0, whiteColor));
    assert(text("Score Limit", 0, 52.5f, baseline(1), 8.3f, 0, idle) &&
           text("100 Kills", 0, centered("100 Kills", 0, 52.5f), baseline(1), 7.5f, 0, idle));
    assert(text("TIME LIMIT", 1, 389.0f, 113.3f, 12.5f, 0.13f, gold));
    assert(wrapped("The match ends when the time runs out, and whoever leads then wins.", 389.0f, 127.5f, 220.0f) == 2);
    assert(tris() == 2 && prompts());
    /* Right steps the minutes up and round to none; Left goes back; Enter
     * steps on. */
    {
        static const int minutes[] = { 20, 30, 45, 60, 0, 5, 10, 15, 20 };
        for (int i = 0; i < 9; i++)
            assert(key(0x9d) && DVAR("ui_matchTimeLimit") == minutes[i]);
        assert(key(0x9c) && DVAR("ui_matchTimeLimit") == 15 && key(0x9c) && DVAR("ui_matchTimeLimit") == 10);
        assert(key(0x0d) && DVAR("ui_matchTimeLimit") == 15 && clicks == 13);
    }
    DVAR("ui_matchTimeLimit") = 0;
    paint();
    assert(text("Unlimited", 1, centered("Unlimited", 1, 52.5f), baseline(0), 7.5f, 0, whiteColor));
    assert(key(0x9c) && DVAR("ui_matchTimeLimit") == 60);
    /* Score Limit: Team Deathmatch's kills; the panel says what wins. */
    assert(key(0x9b) && s_match.focus[4] == 1);
    paint();
    assert(row(baseline(1), 336.7f) && text("Score Limit", 1, 60.8f, baseline(1), 8.3f, 0, gold));
    assert(text("100 Kills", 1, centered("100 Kills", 1, 52.5f), baseline(1), 7.5f, 0, whiteColor));
    assert(text("60 Minutes", 0, centered("60 Minutes", 0, 52.5f), baseline(0), 7.5f, 0, idle));
    assert(text("SCORE LIMIT", 1, 389.0f, 113.3f, 12.5f, 0.13f, gold));
    assert(wrapped("The first team to make this many kills wins.", 389.0f, 127.5f, 220.0f) == 1);
    {
        static const int kills[] = { 150, 200, 300, 0, 25, 50, 75, 100 };
        for (int i = 0; i < 8; i++)
            assert(key(0x9d) && DVAR("ui_matchScoreLimit") == kills[i]);
        assert(key(0x9c) && DVAR("ui_matchScoreLimit") == 75);
    }
    /* What the mode does not offer, stale or another mode's, is its own. */
    DVAR("ui_matchScoreLimit") = 10;
    DVAR("ui_matchTimeLimit") = 7;
    paint();
    assert(find("100 Kills") && find("15 Minutes") && !find("10 Kills") && !find("7 Minutes"));
    assert(key(0x9d) && DVAR("ui_matchScoreLimit") == 150);
    DVAR("ui_matchTimeLimit") = 10;
    DVAR("ui_matchScoreLimit") = 25;
    {
        int timeLimit = 0, scoreLimit = 0;
        UI_MatchMenu_Rules("tdm", &timeLimit, &scoreLimit);
        assert(timeLimit == 10 && scoreLimit == 25);
        UI_MatchMenu_Rules("dm", &timeLimit, &scoreLimit);
        assert(timeLimit == 10 && scoreLimit == 100);
        UI_MatchMenu_Rules("sw", &timeLimit, &scoreLimit);
        assert(timeLimit == -1 && scoreLimit == -1);
        timeLimit = scoreLimit = 0;
        UI_MatchMenu_Rules(NULL, &timeLimit, &scoreLimit);
        assert(timeLimit == -1 && scoreLimit == -1);
    }
    /* Escape goes back, Game Rules focused and its preview showing the
     * rules, as does Start Match's. */
    assert(key(0x1b) && s_match.page == 0 && s_match.focus[0] == 2);
    paint();
    assert(text("10 Minutes", 0, centered("10 Minutes", 0, 269.0f), baseline(0), 7.5f, 0, dim) &&
           text("25 Kills", 0, centered("25 Kills", 0, 269.0f), baseline(1), 7.5f, 0, dim));
    s_match.focus[0] = 4;
    paint();
    assert(text("10 Minutes", 0, centered("10 Minutes", 0, 269.0f), baseline(2), 7.5f, 0, dim) &&
           text("25 Kills", 0, centered("25 Kills", 0, 269.0f), baseline(3), 7.5f, 0, dim));
    /* Opening again keeps them; the page opens on its first rule. */
    open();
    assert(DVAR("ui_matchTimeLimit") == 10 && DVAR("ui_matchScoreLimit") == 25);
    s_match.focus[0] = 2;
    assert(key(0x0d) && key(0x9b) && s_match.focus[4] == 1 && key(0x1b) && key(0x0d) && s_match.focus[4] == 0);
    assert(key(0x1b) && s_match.page == 0 && s_match.focus[0] == 2);
    cases += 5;

    /* Mode: the five modes in the arenas' order, the current one focused;
     * the panel names it and gives the retail text, its own when stringed
     * lacks it or has it blank. */
    DVAR("ui_botCount") = 0;
    s_match.focus[0] = 0;
    assert(key(0x0d) && s_match.page == 1 && s_match.focus[1] == 4);
    paint();
    assert(text("GAME MODE SELECT", 1, 44.0f, 72.5f, 11.7f, 0.2f, gold) && find("Choose the type of gameplay"));
    assert(text("CAPTURE THE FLAG", 0, 52.5f, baseline(0), 7.5f, 0, idle) && row(baseline(4), 230.8f));
    assert(text("TEAM DEATHMATCH", 1, 60.8f, baseline(4), 8.0f, 0, gold) && count("TEAM DEATHMATCH") == 2);
    assert(wrapped("Stringed TDM.", 283.8f, 135.8f, 289.2f) == 1);
    for (int i = 0; i < drawCount; i++)
        if (drawn[i].kind == TEXT && !strcmp(drawn[i].text, "TEAM DEATHMATCH") && drawn[i].x > 200)
            assert(drawn[i].bold && near(drawn[i].x, 283.8f) && near(drawn[i].y, 123.3f) && near(drawn[i].cap, 12.5f) &&
                   near(drawn[i].tracking, 0.13f) && sameColor(drawn[i].color, light));
    /* The scene, and the focused row's fill and three lines: no picture. */
    assert(pics() == 1 + 4 && tris() == 0 && prompts());
    assert(key(0x9b) && s_match.focus[1] == 0);
    paint();
    assert(wrapped("Stringed CTF.", 283.8f, 135.8f, 289.2f) == 1);
    assert(key(0x9a) && key(0x9a) && s_match.focus[1] == 3);
    paint();
    /* MENU_SD_DESC is missing: the retail English, wrapped to 289.2. */
    assert(wrapped("The Allies try to destroy Axis held objectives by planting explosives.  Axis must defuse the "
                   "explosives if they are planted.", 283.8f, 135.8f, 289.2f) == 2);
    assert(key(0x9a) && s_match.focus[1] == 2);
    paint();
    /* A longer note stops after four full lines. */
    {
        int lines = 0;
        for (int i = 0; i < drawCount; i++)
            if (drawn[i].kind == TEXT && !drawn[i].bold && near(drawn[i].x, 283.8f) && near(drawn[i].cap, 6.6f)) {
                float w = width(drawn[i].text, 0, 6.6f, 0);
                assert(near(drawn[i].y, 135.8f + 10.0f * (float)lines) && w <= 289.2f && w > 289.2f - 7 * 0.55f * 6.6f);
                assert(!strstr(drawn[i].text, "The end."));
                assert(lines || !strncmp(drawn[i].text, "Stringed HQ. Hold the radio.", 28));
                lines++;
            }
        assert(lines == 4);
    }
    assert(key(0x9a) && key(0x9a) && s_match.focus[1] == 0 && key(0x9a) && s_match.focus[1] == 4);
    assert(key(0x9b) && s_match.focus[1] == 0 && key(0x9b) && s_match.focus[1] == 1);
    paint();
    /* MP_GAIN_POINTS_BY_ELIMINATING_NOSCORE is blank. */
    assert(wrapped("Gain points by eliminating other players.", 283.8f, 135.8f, 289.2f) == 1);
    /* Escape keeps the mode and its rules; Enter chooses it as the retail
     * control does, another mode starting from its own rules, the same one
     * keeping them. */
    assert(key(0x1b) && s_match.page == 0 && s_match.focus[0] == 0 && DVAR("ui_netGametype") == 4);
    assert(DVAR("ui_matchTimeLimit") == 10 && DVAR("ui_matchScoreLimit") == 25);
    assert(key(0x0d) && key(0x9b) && key(0x0d) && s_match.page == 0 && s_match.focus[0] == 0);
    assert(DVAR("ui_netGametype") == 0 && !strcmp(DVAR_STRING("ui_netGametypeName"), "ctf"));
    assert(DVAR("ui_matchTimeLimit") == -1 && DVAR("ui_matchScoreLimit") == -1);
    DVAR("ui_matchTimeLimit") = 45;
    assert(key(0x0d) && key(0x0d) && DVAR("ui_netGametype") == 0 && DVAR("ui_matchTimeLimit") == 45);
    DVAR("ui_matchTimeLimit") = -1;
    paint();
    assert(text("CAPTURE THE FLAG", 0, 52.5f, 150.0f, 6.5f, 0, note));
    cases += 4;
    /* Each mode's own rules, its score in its units, and its note. */
    {
        static const char *const own[5][3] = {
            { "30 Minutes", "Unlimited", "The first team to capture the enemy flag this many times wins." },
            { "30 Minutes", "100 Kills", "The first player to make this many kills wins." },
            { "30 Minutes", "300 Points", "Teams score a point for every second they hold the radio and 10 for "
                                          "destroying the enemy's. The first to this many wins." },
            { "Unlimited", "10 Rounds", "The first team to win this many rounds wins the match." },
            { "15 Minutes", "100 Kills", "The first team to make this many kills wins." } };
        static const int lines[5] = { 2, 1, 2, 1, 1 };
        for (int mode = 0; mode < 5; mode++) {
            DVAR("ui_netGametype") = mode;
            s_match.page = 4;
            s_match.focus[4] = 1;
            paint();
            assert(text(own[mode][0], 0, centered(own[mode][0], 0, 52.5f), baseline(0), 7.5f, 0, idle));
            assert(text(own[mode][1], 1, centered(own[mode][1], 1, 52.5f), baseline(1), 7.5f, 0, whiteColor));
            assert(wrapped(own[mode][2], 389.0f, 127.5f, 220.0f) == lines[mode]);
        }
        /* One capture, then more. */
        DVAR("ui_netGametype") = 0;
        assert(key(0x9d) && DVAR("ui_matchScoreLimit") == 1);
        paint();
        assert(find("1 Capture") && key(0x9d) && DVAR("ui_matchScoreLimit") == 3);
        paint();
        assert(find("3 Captures"));
        DVAR("ui_matchScoreLimit") = -1;
        s_match.page = 0;
        s_match.focus[0] = 0;
    }
    cases += 2;
    /* A mode not played on the chosen map moves it to the mode's first. */
    sharedUiInfo.mapList[0].typeBits = ~(1 << 1);
    assert(key(0x0d) && key(0x9b) && key(0x0d) && DVAR("ui_netGametype") == 1 && DVAR("ui_currentNetMap") == 1);
    assert(!strcmp(DVAR_STRING("ui_netGametypeName"), "dm"));
    cases++;

    /* Map: the mode's maps, the chosen one focused; its levelshot cropped
     * to the panel, its name in gold and the map's note. */
    assert(key(0x9b) && key(0x0d) && s_match.page == 2 && s_match.focus[2] == 0);
    paint();
    assert(text("MAP SELECT", 1, 44.0f, 72.5f, 11.7f, 0.2f, gold) && find("Pick where your match will take place"));
    assert(!find("CARENTAN, FRANCE") && row(baseline(0), 230.8f));
    assert(text("DAWNVILLE, FRANCE", 1, 60.8f, baseline(0), 8.0f, 0, gold) &&
           text("TOUJANE, TUNISIA", 0, 52.5f, baseline(1), 7.5f, 0, idle));
    {
        int shown = 0;
        for (int i = 0; i < drawCount; i++) {
            const Drawn *d = &drawn[i];
            if (d->kind == PIC && d->material == &shots[1]) {
                assert(near(d->x, 284.2f) && near(d->y, 101.7f) && near(d->w, 289.2f) && near(d->h, 162.7f) &&
                       near(d->s1, 0) && near(d->t1, 0.125f) && near(d->s2, 1) && near(d->t2, 0.875f) &&
                       sameColor(d->color, whiteColor) && d->horz == 1 && d->vert == 1);
                shown++;
            }
        }
        assert(shown == 1);
    }
    assert(count("DAWNVILLE, FRANCE") == 2);
    for (int i = 0; i < drawCount; i++)
        if (drawn[i].kind == TEXT && !strcmp(drawn[i].text, "DAWNVILLE, FRANCE") && drawn[i].x > 200)
            assert(drawn[i].bold && near(drawn[i].x, 283.8f) && near(drawn[i].y, 294.2f) && near(drawn[i].cap, 12.5f) &&
                   near(drawn[i].tracking, 0.13f) && sameColor(drawn[i].color, gold));
    assert(wrapped("Normandy village around a church and its walled churchyard, fought over by American and German "
                   "troops.", 283.8f, 309.2f, 289.2f) == 2);
    /* Without a levelshot, no picture. */
    sharedUiInfo.mapList[2].levelShot = NULL;
    assert(key(0x9b) && s_match.focus[2] == 1);
    paint();
    for (int i = 0; i < drawCount; i++)
        assert(!(drawn[i].kind == PIC && near(drawn[i].x, 284.2f)));
    assert(wrapped("Hillside town in southern Tunisia, fought over by British and Afrika Korps troops across its flat "
                   "rooftops and narrow alleys.", 283.8f, 309.2f, 289.2f) == 2);
    sharedUiInfo.mapList[2].levelShot = &shots[2];
    assert(key(0x0d) && s_match.page == 0 && s_match.focus[0] == 1 && DVAR("ui_currentNetMap") == 2);
    paint();
    assert(text("TOUJANE, TUNISIA", 0, 52.5f, 203.3f, 6.5f, 0, note));
    assert(key(0x0d) && s_match.focus[2] == 1 && key(0x1b) && DVAR("ui_currentNetMap") == 2 && s_match.focus[0] == 1);
    sharedUiInfo.mapList[0].typeBits = -1;
    cases += 3;

    /* The pointer: motion focuses what it is over, a click acts on it, the
     * other button goes back. A setting's left half steps it back. */
    s_match.focus[0] = 0;
    paint();
    overs = 0;
    point(100.0f, 236.0f);
    paint();
    paint();
    assert(s_match.focus[0] == 2 && overs == 1 && row(236.6f, 230.8f));
    point(5.0f, 230.0f);
    paint();
    paint();
    assert(s_match.focus[0] == 2 && overs == 1);
    point(150.0f, 265.0f);
    paint();
    paint();
    assert(s_match.focus[0] == 3 && overs == 2 && row(268.6f, 230.8f));
    point(150.0f, 300.0f);
    paint();
    paint();
    assert(s_match.focus[0] == 4 && overs == 3 && box(282.0f, 1));
    /* Straight up, onto Map. */
    point(150.0f, 190.0f);
    paint();
    assert(s_match.focus[0] == 1 && overs == 4);
    assert(key(0xc8) && s_match.page == 2 && s_match.focus[0] == 1 && s_match.hover == -1);
    /* The page's hover is gone until it paints: a click does nothing. */
    assert(key(0xc8) && s_match.page == 2);
    assert(key(0xc9) && s_match.page == 0);
    point(100.0f, 265.0f);
    paint();
    paint();
    assert(key(0xc8) && s_match.page == 3);
    DVAR("ui_botCount") = 8;
    point(175.0f, baseline(0) - 4.0f);
    paint();
    paint();
    assert(key(0xc8) && DVAR("ui_botCount") == 7 && s_match.focus[3] == 0);
    point(321.0f, baseline(0) - 4.0f);
    paint();
    assert(key(0xc8) && DVAR("ui_botCount") == 8);
    point(70.0f, baseline(0) - 4.0f);
    paint();
    assert(key(0xc8) && DVAR("ui_botCount") == 9);
    point(250.0f, baseline(1) - 4.0f);
    paint();
    paint();
    assert(s_match.focus[3] == 1 && key(0xc8) && DVAR("ui_botDifficulty") == 0);
    /* Off the rows, nothing. */
    point(400.0f, 300.0f);
    paint();
    paint();
    assert(key(0xc8) && DVAR("ui_botDifficulty") == 0 && s_match.focus[3] == 1);
    assert(key(0xc9) && s_match.page == 0 && s_match.focus[0] == 3);
    /* Bot setup opens on its first setting again. */
    assert(key(0x0d) && s_match.page == 3 && s_match.focus[3] == 0 && key(0x1b) && s_match.page == 0);
    /* The rules take the pointer as Bot Setup does: Deathmatch's 30 minutes
     * back to 20 and on again, its 100 kills on to 150. */
    point(100.0f, 236.0f);
    paint();
    paint();
    assert(s_match.focus[0] == 2 && key(0xc8) && s_match.page == 4 && s_match.focus[4] == 0);
    point(175.0f, baseline(0) - 4.0f);
    paint();
    paint();
    assert(key(0xc8) && DVAR("ui_matchTimeLimit") == 20 && s_match.focus[4] == 0);
    point(321.0f, baseline(0) - 4.0f);
    paint();
    assert(key(0xc8) && DVAR("ui_matchTimeLimit") == 30);
    point(250.0f, baseline(1) - 4.0f);
    paint();
    paint();
    assert(s_match.focus[4] == 1 && key(0xc8) && DVAR("ui_matchScoreLimit") == 150);
    assert(key(0xc9) && s_match.page == 0 && s_match.focus[0] == 2);
    DVAR("ui_matchTimeLimit") = DVAR("ui_matchScoreLimit") = -1;
    cases += 5;

    /* Start Match runs StartServer once, on the chosen map, and says so
     * until the screen opens again. */
    s_match.focus[0] = 4;
    DVAR("ui_currentNetMap") = 9;
    clicks = 0;
    assert(key(0x0d) && scripts == 1 && !strcmp(script, "StartServer") && DVAR("ui_currentNetMap") == 0 && clicks == 1);
    paint();
    assert(text("STARTING YOUR SERVER...", 0, 52.5f, 320.3f, 6.5f, 0, note));
    assert(key(0x0d) && key(0x0d) && scripts == 1);
    open();
    assert(s_match.page == 0 && s_match.focus[0] == 0 && !s_match.starting);
    s_match.focus[0] = 4;
    assert(key(0x0d) && scripts == 2);
    /* With no map for the mode, nothing starts. */
    open();
    for (int i = 0; i < 3; i++)
        sharedUiInfo.mapList[i].typeBits = 0;
    s_match.focus[0] = 4;
    assert(key(0x0d) && scripts == 2 && !s_match.starting);
    paint();
    assert(text("MAP", 1, 52.5f, 192.0f, 9.0f, 0, light) && find("CREATE YOUR SERVER AND JOIN IT"));
    s_match.focus[0] = 1;
    assert(key(0x0d) && s_match.page == 2 && s_match.focus[2] == 0);
    paint();
    assert(!find("TOUJANE, TUNISIA") && !find("CARENTAN, FRANCE") && find("MAP SELECT"));
    assert(key(0x9b) && s_match.focus[2] == 0 && key(0x0d) && s_match.page == 0);
    arenas();
    cases += 3;

    /* Opening resets to the match page and puts a mode and a map that exist
     * in the dvars; Escape there closes createserver. */
    DVAR("ui_netGametype") = 9;
    DVAR("ui_currentNetMap") = 2;
    sharedUiInfo.mapList[2].typeBits = ~1;
    s_match.page = 2;
    open();
    assert(s_match.page == 0 && s_match.hover == -1 && DVAR("ui_netGametype") == 0 && DVAR("ui_currentNetMap") == 0);
    assert(!strcmp(DVAR_STRING("ui_netGametypeName"), "ctf"));
    arenas();
    /* A pointer already over Map neither focuses it nor takes a click
     * before the screen has painted. */
    point(150.0f, 190.0f);
    open();
    assert(key(0xc8) && s_match.page == 0);
    paint();
    assert(s_match.focus[0] == 0 && s_match.hover == 1);
    assert(key(0x1b) && closes == 1 && !strcmp(closed, "createserver"));
    cases += 3;

    /* Long lists scroll: nine rows show, following the focus. */
    sharedUiInfo.mapCount = 12;
    for (int i = 3; i < 12; i++) {
        static char names[12][16];
        snprintf(names[i], sizeof(names[i]), "Map %d", i);
        sharedUiInfo.mapList[i].mapName = names[i];
        sharedUiInfo.mapList[i].mapLoadName = names[i];
        sharedUiInfo.mapList[i].typeBits = -1;
    }
    open();
    s_match.focus[0] = 1;
    paint();
    assert(find("MAP 8") && !find("MAP 9"));
    assert(key(0x0d) && s_match.page == 2);
    for (int i = 0; i < 10; i++)
        assert(key(0x9b));
    paint();
    assert(s_match.focus[2] == 10 && text("MAP 10", 1, 60.8f, baseline(8), 8.0f, 0, gold) && !find("CARENTAN, FRANCE"));
    assert(text("TOUJANE, TUNISIA", 0, 52.5f, baseline(0), 7.5f, 0, idle) && !find("DAWNVILLE, FRANCE"));
    assert(key(0x9b) && key(0x9b) && s_match.focus[2] == 0);
    paint();
    assert(text("CARENTAN, FRANCE", 1, 60.8f, baseline(0), 8.0f, 0, gold) && find("MAP 8") && !find("MAP 9"));
    arenas();
    cases++;

    /* The room's name: the gametag without colors, in the letters rooms.py
     * keeps, from the first that is not a space, at most 32 and none
     * trailing, within the buffer. */
    {
        static const char *const tags[][2] = {
            { "^1Ton^7ga", "Tonga" }, { "  [ARG] Gast\xf3n!  ", "ARG Gastn" }, { "a_b|c.d-e f", "a_b|c.d-e f" },
            { "x;quit", "xquit" }, { "\"+connect evil", "connect evil" }, { "", "" }, { "^^^", "" },
            { "abcdefghijklmnopqrstuvwxyz0123456789", "abcdefghijklmnopqrstuvwxyz012345" },
            { "abcdefghijklmnopqrstuvwxyz01234 6", "abcdefghijklmnopqrstuvwxyz01234" } };
        char room[64];
        for (int i = 0; i < (int)(sizeof(tags) / sizeof(tags[0])); i++) {
            snprintf(DVAR_STRING("name"), 64, "%s", tags[i][0]);
            memset(room, 'z', sizeof(room));
            UI_MatchMenu_RoomName(room, sizeof(room));
            assert(!strcmp(room, tags[i][1]));
        }
        snprintf(DVAR_STRING("name"), 64, "Tonga");
        UI_MatchMenu_RoomName(room, 4);
        assert(!strcmp(room, "Ton"));
        snprintf(DVAR_STRING("name"), 64, "!!!");
        open();
        s_match.focus[0] = 4;
        paint();
        assert(!find("Server Name") && find("Bots"));
        cases += 2;
    }

    printf("%d\n", cases);
    return 0;
}
'''

mutants = [
    [('    return mode >= 0 && mode < sharedUiInfo.numGameTypes ? mode : 0;', '    return mode;')],
    [('        if (((unsigned int)sharedUiInfo.mapList[i].typeBits >> mode) & 1u)', '        if (1)')],
    [('    return count ? maps[0] : -1;', '    return count ? map : -1;')],
    [('    if (map >= 0 && map != Dvar_GetInt("ui_currentNetMap"))\n        Dvar_SetIntByName("ui_currentNetMap", map);\n',
      '')],
    [('        i += i < MATCH_BOT_COUNTS && s_matchBotCounts[i] == count;', '        i++;')],
    [('    Dvar_SetIntByName("ui_botCount", s_matchBotCounts[(i + MATCH_BOT_COUNTS) % MATCH_BOT_COUNTS]);',
      '    Dvar_SetIntByName("ui_botCount", s_matchBotCounts[i % MATCH_BOT_COUNTS]);')],
    [('(Match_Difficulty() + delta + MATCH_DIFFICULTIES) % MATCH_DIFFICULTIES', '(Match_Difficulty() + delta) % 3')],
    [('    return difficulty >= 0 && difficulty < MATCH_DIFFICULTIES ? difficulty : 1;', '    return difficulty;')],
    [('    else if (count > 0)\n', '    else if (count >= 0)\n')],
    [('    if (s_match.starting || map < 0)\n', '    if (map < 0)\n')],
    [('    if (map != Dvar_GetInt("ui_currentNetMap"))\n        Dvar_SetIntByName("ui_currentNetMap", map);\n    s_match',
      '    s_match')],
    [('    return Match_IsFamily(menu) && Match_Up(dc);', '    return Match_IsFamily(menu);')],
    [('(menu->window.dynamicFlags[0] & MATCH_WINDOW_VISIBLE)', '1')],
    [('!I_strnicmp(menu->window.name, MATCH_MENU, strlen(MATCH_MENU))', '!I_stricmp(menu->window.name, MATCH_MENU)')],
    [('    if (s_match.page != page)\n        s_match.hover = -1;\n', '')],
    [('        Menus_CloseByName(dc, MATCH_MENU);\n', '        ;\n')],
    [('    else\n        s_match.page = MATCH_PAGE_MATCH;\n}', '}')],
    [('? -1 : 1;\n}', '? 1 : 1;\n}')],
    [('!Match_PointerLeftOf(dc, center - 1.5f * MATCH_ARROW_OFFSET)', '1')],
    [('    if (moved && s_match.hover >= 0)\n        Match_Focus(s_match.hover);\n', '')],
    [('    if (moved && s_match.hover >= 0)', '    if (s_match.hover >= 0)')],
    [('    moved = dc->cursorx != s_match.cursorX || dc->cursory != s_match.cursorY;',
      '    moved = dc->cursorx != s_match.cursorX;')],
    [('    s_match.cursorX = dc->cursorx;\n    s_match.cursorY = dc->cursory;\n    Match_SelectMode', '    Match_SelectMode')],
    [('    s_match.hover = -1;\n    s_match.cursorX', '    s_match.cursorX')],
    [('    Match_SelectMode(Match_Mode());\n}', '}')],
    [('    if (index == s_match.focus[s_match.page])\n        return;\n', '')],
    [('        if (page == MATCH_PAGE_BOTS || page == MATCH_PAGE_RULES)\n            Match_Step', '        Match_Step')],
    [('        if (page == MATCH_PAGE_BOTS || page == MATCH_PAGE_RULES)\n            Match_Step',
      '        if (page == MATCH_PAGE_BOTS)\n            Match_Step')],
    [('    if (s_match.page == MATCH_PAGE_BOTS || s_match.page == MATCH_PAGE_RULES) {',
      '    if (s_match.page == MATCH_PAGE_BOTS) {')],
    [('            s_match.focus[MATCH_PAGE_RULES] = MATCH_RULE_TIME;\n', '')],
    [('    if (page == MATCH_PAGE_RULES)\n        Match_StepRule(setting, delta);\n    else if', '    if')],
    # The rules: a new mode's own, values the mode offers, wrapping, units.
    [('    if (mode != Dvar_GetInt("ui_netGametype")) {\n        Dvar_SetIntByName("ui_matchTimeLimit", -1);\n'
      '        Dvar_SetIntByName("ui_matchScoreLimit", -1);\n    }\n', '')],
    [('    if (mode != Dvar_GetInt("ui_netGametype")) {', '    if (1) {')],
    [('        if (values[i] == value)\n            return value;', '        if (1)\n            return value;')],
    [('    return rule == MATCH_RULE_TIME ? s_matchRuleModes[ruleMode].timeLimit : s_matchRuleModes[ruleMode].scoreLimit;',
      '    return rule == MATCH_RULE_TIME ? s_matchRuleModes[ruleMode].scoreLimit : s_matchRuleModes[ruleMode].timeLimit;')],
    [('values[(i + delta + count) % count]', 'values[(i + delta) % count]')],
    [('value == 1 ? s_matchRuleModes[ruleMode].unit : s_matchRuleModes[ruleMode].units',
      's_matchRuleModes[ruleMode].units')],
    [('    else if (!value)\n        snprintf(text, (size_t)size, "Unlimited");\n', '')],
    [('        if (!I_stricmp(gameType, s_matchRuleModes[i].gameType))\n            return i;',
      '        if (1)\n            return i;')],
    [('    return -1;\n}\n\nstatic int Match_ModeRules', '    return 1;\n}\n\nstatic int Match_ModeRules')],
    [('    *scoreLimit = Match_Rule(ruleMode, MATCH_RULE_SCORE);', '    *scoreLimit = Match_Rule(ruleMode, MATCH_RULE_TIME);')],
    [('s_match.focus[MATCH_PAGE_RULES] == MATCH_RULE_TIME ? MATCH_TIME_NOTE', '0 ? MATCH_TIME_NOTE')],
    [('            Match_PaintPair(2 + i, s_matchRules[i], value);', '            Match_PaintPair(i, s_matchRules[i], value);')],
    [('        if (page == MATCH_PAGE_RULES)\n            Match_RuleValue(row, value, sizeof(value));\n        else\n', '')],
    [('            s_match.focus[MATCH_PAGE_MODE] = Match_Mode();\n', '')],
    [('            s_match.focus[MATCH_PAGE_MAP] = i < count ? i : 0;', '            s_match.focus[MATCH_PAGE_MAP] = 0;')],
    [('            s_match.focus[MATCH_PAGE_BOTS] = MATCH_SETTING_BOTS;\n', '')],
    [('    case MATCH_KEY_TAB:\n', '')],
    [('    if (!down || (key & MATCH_KEY_CHAR))\n        return 1;', '    if (!down)\n        return 1;\n'
      '    if (key & MATCH_KEY_CHAR)\n        return 0;')],
    [('            (c != ' "' '" ' || length))', '            1)')],
    [('    if (length > MATCH_ROOM_NAME_MAX)\n        length = MATCH_ROOM_NAME_MAX;\n', '')],
    [('    while (length && name[length - 1] == ' "' '" ')\n        length--;\n', '')],
    [('strchr(" _|.-", c)', 'strchr(" _|.-!", c)')],
    [('    I_CleanStr(tag);\n', '')],
    [('        if (line[0] && UI_WWIIMenu_Width(trial, 0, cap, 0.0f) > width) {', '        if (0) {')],
    [('            lines--;\n', '')],
    [('    return rows + (float)(MATCH_ITEM_BOTS - MATCH_ITEM_RULES) * MATCH_ROW_SPACING + MATCH_ROW_H + MATCH_GAP;',
      '    return rows + MATCH_ROW_H + MATCH_GAP;')],
    [('        return rows + (float)(item - MATCH_ITEM_RULES) * MATCH_ROW_SPACING;',
      '        return rows + (float)(item - MATCH_ITEM_RULES) * (MATCH_ROW_H + MATCH_GAP);')],
    [('    if (focus >= *scroll + MATCH_LIST_VISIBLE)\n        *scroll = focus - MATCH_LIST_VISIBLE + 1;\n', '')],
    [('    if (focus < *scroll)\n        *scroll = focus;\n', '')],
    [('    verts[direction > 0 ? 2 : 1].xy[1] = top + 0.5f * h;', '    verts[2].xy[1] = top + 0.5f * h;')],
    [('        verts[0].xy[0] = verts[2].xy[0] = left + w;\n        verts[1].xy[0] = left;',
      '        verts[0].xy[0] = verts[2].xy[0] = left;\n        verts[1].xy[0] = left + w;')],
    [('    if (left) {\n        edge.x = rect->x;\n        Match_Fill(&edge, color);\n    }',
      '    edge.x = rect->x;\n    Match_Fill(&edge, color);')],
    [('    edge.y = rect->y + MATCH_LINE;\n    edge.h = rect->h - 2.0f * MATCH_LINE;', '    edge.y = rect->y;')],
    [('        Match_Fill(&band, s_matchBand);\n', '')],
    [('        Match_Text("\\xbb", 0, MATCH_BOX_X + MATCH_CHEVRON_X, baseline, nameCap, 0.0f, gold);\n', '')],
    [('    float x = MATCH_TEXT_X + (focused ? MATCH_FOCUS_INDENT : 0.0f);', '    float x = MATCH_TEXT_X;')],
    [('        Match_Caps(text, focused, x, baseline,', '        Match_Caps(text, 0, x, baseline,')],
    [('0.0f, MATCH_SHOT_CROP, 1.0f, 1.0f - MATCH_SHOT_CROP,', '0.0f, 0.0f, 1.0f, 1.0f,')],
    [('    if (shot)\n', '    if (1)\n')],
    [('    return text && text[0] ? text : fallback;', '    return text ? text : fallback;')],
    [('    if (name[0])\n            Match_PaintPair(5,', '    if (1)\n            Match_PaintPair(5,')],
    [('    return UI_SafeTranslateString(name && name[0] ? name : "EXE_ALL");', '    return name;')],
    [('                         UI_WWIIMenu_Color(focused ? WWII_COLOR_WHITE : WWII_COLOR_IDLE));',
      '                         UI_WWIIMenu_Color(WWII_COLOR_WHITE));')],
    [('    UI_WWIIMenu_Prompts(keys, labels, 1, "ESC", "Back");\n', '')],
    [('    CL_DrawStretchPic(0.0f, 0.0f, 640.0f, 480.0f, MATCH_ALIGN_FULLSCREEN, MATCH_ALIGN_FULLSCREEN, 0.0f, 0.0f, 1.0f, '
      '1.0f,\n                      s_matchShade, sharedUiInfo.assets.whiteMaterial);\n', '')],
    [('    if (!Match_IsMenu(menu) || !Match_Up(dc))\n        return;\n    /*', '    if (!Match_IsMenu(menu))\n        return;\n    /*')],
    # The layout's numbers against the captures'.
    [('#define MATCH_TITLE_TRACKING 0.2f', '#define MATCH_TITLE_TRACKING 0.25f')],
    [('#define MATCH_PANEL_TITLE_TRACKING 0.13f', '#define MATCH_PANEL_TITLE_TRACKING 0.0f')],
    [('#define MATCH_LIST_CAP 7.5f', '#define MATCH_LIST_CAP 8.3f')],
    [('#define MATCH_BOX_TOP 111.7f', '#define MATCH_BOX_TOP 115.0f')],
    [('#define MATCH_ROW_SPACING 32.0f', '#define MATCH_ROW_SPACING 30.0f')],
    [('#define MATCH_FOCUS_INDENT 8.3f', '#define MATCH_FOCUS_INDENT 6.0f')],
    [('#define MATCH_SHOT_CROP 0.125f', '#define MATCH_SHOT_CROP 0.1f')],
    [('#define MATCH_VALUE_OFFSET 195.5f', '#define MATCH_VALUE_OFFSET 180.0f')],
    [('#define MATCH_PREVIEW_SHIFT 216.5f', '#define MATCH_PREVIEW_SHIFT 230.0f')],
]

# The backdrop: bg's other items give way under createserver as under
# Create a Class and Join Game, and only those three.
page_open = function(wwii, 'UI_WWIIMenu_PageOpen')
backdrop = r'''
#include <assert.h>
#include <strings.h>
typedef struct { const char *name; } Window;
typedef struct menuDef_t { Window window; } menuDef_t;
typedef struct { menuDef_t *menuStack[16]; int openMenuCount; } displayContextDef_t;
static int I_stricmp(const char *s0, const char *s1) { return strcasecmp(s0, s1); }
''' + page_open + r'''
int main(void) {
    menuDef_t bg = { { "bg" } }, page = { { "createserver" } }, unnamed = { { 0 } };
    displayContextDef_t dc = { { &bg, &unnamed, &page }, 3 };
    assert(UI_WWIIMenu_PageOpen(&dc));
    page.window.name = "CreateServer";
    assert(UI_WWIIMenu_PageOpen(&dc));
    page.window.name = "create_a_class";
    assert(UI_WWIIMenu_PageOpen(&dc));
    page.window.name = "joinserver";
    assert(UI_WWIIMenu_PageOpen(&dc));
    page.window.name = "createserver_maps";
    assert(!UI_WWIIMenu_PageOpen(&dc));
    page.window.name = "joinserver_popmenu";
    assert(!UI_WWIIMenu_PageOpen(&dc));
    page.window.name = "createserver";
    dc.openMenuCount = 2;
    assert(!UI_WWIIMenu_PageOpen(&dc));
    return 0;
}
'''


def program(text):
    return support + re.sub(r'^#include "[^"]+"\n', '', text, flags=re.M) + checks


def run(directory, name, text, warnings, expect_pass):
    path = Path(directory) / f'{name}.c'
    binary = Path(directory) / name
    path.write_text(text)
    subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    *warnings, str(path), '-lm', '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert (result.returncode == 0) == expect_pass, (name, result.stdout[-500:], result.stderr[-3000:])
    return result


module = re.sub(r'^#include "[^"]+"\n', '', source, flags=re.M)
assert module.count('#include') == 2
strict = ['-Wall', '-Wextra', '-Werror', '-Wno-unused-function', '-Wno-unused-parameter',
          '-Wno-missing-field-initializers']
with tempfile.TemporaryDirectory(prefix='cod2-match-menu-') as directory:
    cases = int(run(directory, 'menu', program(source), strict, True).stdout)
    caught = 0
    for index, pairs in enumerate(mutants):
        variant = source
        for old, new in pairs:
            assert variant.count(old) == 1, old
            variant = variant.replace(old, new)
        if evidence(variant):
            caught += 1
            continue
        run(directory, f'menu{index}', program(variant), ['-w'], False)
        caught += 1
    run(directory, 'backdrop', backdrop, strict, True)
    for index, (old, new) in enumerate(((' || !I_stricmp(menu->window.name, "createserver")', ''),
                                        ('!I_stricmp(menu->window.name, "createserver")',
                                         '!I_strnicmp(menu->window.name, "createserver", 12)'))):
        assert backdrop.count(old) == 1, old
        variant = backdrop.replace(old, new).replace('static int I_stricmp',
                                                     '#include <string.h>\nstatic int I_strnicmp(const char *a, const '
                                                     'char *b, unsigned long n) { return strncasecmp(a, b, n); }\n'
                                                     'static int I_stricmp')
        run(directory, f'backdrop{index}', variant, ['-w'], False)
        caught += 1

print(f'PASS: Start New Server draws WWII\'s MATCH screen over createserver, whose retail menus stay hidden and take '
      f'neither keys nor the pointer: Mode and Map boxes, Game Rules and Bots rows and a Start Match box, the focused '
      f'item in gold with its page dimmed beside it; mode, map, rules and bot pages with their panels, each mode\'s '
      f'time and score limits in its units, levelshots cropped to 16:9 and the retail mode texts; keys, the pointer '
      f'and clicks; StartServer once, on a map the mode plays, for a room named after the gametag with the rules '
      f'the gateway and rooms.py accept; bg\'s backdrop alone behind it; {cases} cases; the layout within the '
      f'captures\' measurements; {caught} failing regression mutants')
