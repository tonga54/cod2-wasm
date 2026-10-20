#!/usr/bin/env python3
"""Run the production Join Game screen (ui_join_menu.c) against a recording UI:
the servers by name, map and players under the WWII title and headings, the
focus by keys and pointer, joining and deleting by the focused server's
address, the refresh every 5 s that keeps the last servers on screen while it
empties the list, and the hooks that hide joinserver's retail items under it."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/ui_mp/ui_join_menu.c').read_text()
header = (root / 'src/headers/PC/ui_mp/ui_join_menu.h').read_text()
wwii_header = (root / 'src/headers/PC/ui_mp/ui_wwii_menu.h').read_text()
shared = (root / 'src/PC/ui_mp/ui_shared_mp.c').read_text()
main = (root / 'src/PC/ui_mp/ui_main_mp.c').read_text()
q_shared = (root / 'src/PC/universal/q_shared.c').read_text()


def function(text, name):
    match = re.search(r'^((?:static )?[^\n;]*\b' + name + r'\([^;]*?\))\s*\{', text, re.M)
    assert match, name
    brace = text.index('{', match.end() - 1)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


# The hooks: the screen resets after joinserver's onOpen has started its
# refresh; the pointer leaves the hidden retail items alone; joinserver paints
# nothing of its own; the screen takes every key before the retail Escape.
for text, name in ((shared, 'ui_shared_mp.c'), (main, 'ui_main_mp.c')):
    assert text.count('#include "headers/PC/ui_mp/ui_join_menu.h"\n') == 1, name
menus_open = function(shared, 'Menus_Open')
assert (menus_open.index('Item_RunScript(dc, (itemDef_t *)tempItem, ((menuDef_t *)m)->onOpen);') <
        menus_open.index('    UI_JoinMenu_Opened(dc, menu);\n') < menus_open.index('Display_MouseMove(dc, NULL,'))
mouse_move = function(shared, 'Display_MouseMove')
assert (mouse_move.index('Menu_UpdatePosition((menuDef_t *)p);') <
        mouse_move.index('    if (UI_JoinMenu_Covers(dc, Menu_GetFocused(dc)))\n        return 1;\n') <
        mouse_move.index('Menu_HandleMouseMove(dc,'))
menu_paint = function(shared, 'Menu_Paint')
assert (menu_paint.index('paint_content:') <
        menu_paint.index('    if (UI_JoinMenu_Covers(dc, menu)) {\n        UI_JoinMenu_Paint(dc, menu);\n'
                         '        return;\n    }\n') <
        menu_paint.index('Window_Paint(dc, (void *)m,'))
key_event = function(main, 'UI_KeyEvent_impl')
assert (key_event.index('} else if (UI_MatchMenu_KeyEvent(') <
        key_event.index('} else if (UI_JoinMenu_KeyEvent((displayContextDef_t *)uiInfo, menu, key, down)) {') <
        key_event.index('key == 0x1b && down') < key_event.index('Menu_HandleKey(uiInfo, menu, key, down);'))
assert 'UI_JoinMenu_WantsKeys(menu)' in function(main, 'UI_CheckExecKey')

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#define __EMSCRIPTEN__ 1
typedef float vec_t;
typedef vec_t vec4_t[4];
typedef struct Material { const char *name; } Material;
typedef Material *MaterialHandle;
typedef struct itemDef_s itemDef_t;
typedef struct Window { const char *name; int dynamicFlags[4]; } Window;
typedef struct menuDef_t { Window window; } menuDef_t;
typedef struct displayContextDef_t { int realTime; int cursorx, cursory; menuDef_t *menuStack[16]; int openMenuCount; }
    displayContextDef_t;
typedef struct mapInfo { const char *mapName, *mapLoadName; } mapInfo;
typedef struct sharedUiInfo_t { struct { MaterialHandle whiteMaterial; } assets; int mapCount; mapInfo mapList[8];
    struct { int refreshActive, numDisplayServers, displayServers[64], currentServer; } serverStatus; } sharedUiInfo_t;
static Material white = { "white" };
sharedUiInfo_t sharedUiInfo = { { &white }, 2, { { "Carentan", "mp_carentan" }, { "Toujane", "mp_toujane" } } };

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

static int netSource, thirdPerson = 1, dvarSets;
int Dvar_GetInt(const char *name) { assert(!strcmp(name, "ui_netSource")); return netSource; }
void Dvar_SetIntByName(const char *name, int value) { assert(!strcmp(name, "ui_netSource")); netSource = value; dvarSets++; }
void Dvar_SetBoolByName(const char *name, int value) { assert(!strcmp(name, "cg_thirdPerson")); thirdPerson = value; }
int I_stricmp(const char *s0, const char *s1) { return strcasecmp(s0, s1); }
''' + function(q_shared, 'I_CleanStr') + r'''
/* "\key\value" pairs, as the engine's info strings. */
const char *Info_ValueForKey(const char *s, const char *key) {
    static char value[2][256];
    static int which;
    char *out = value[which ^= 1];
    size_t length = strlen(key);
    out[0] = '\0';
    while (*s == '\\') {
        const char *k = s + 1, *v = strchr(k, '\\'), *end;
        if (!v) break;
        end = strchr(v + 1, '\\');
        if (!end) end = v + 1 + strlen(v + 1);
        if ((size_t)(v - k) == length && !strncmp(k, key, length)) {
            snprintf(out, sizeof(value[0]), "%.*s", (int)(end - v - 1), v + 1);
            break;
        }
        s = end;
    }
    return out;
}
/* The LAN source's servers: an address and an info string each. */
static struct { char address[32]; char info[256]; } servers[8];
void LAN_GetServerAddressString(int source, int index, char *addr, int addrSize) {
    assert(source == 0 && index >= 0 && index < 8);
    snprintf(addr, (size_t)addrSize, "%s", servers[index].address);
}
void LAN_GetServerInfo(int source, int index, char *info, int infoSize) {
    assert(source == 0 && index >= 0 && index < 8 && infoSize == 1024);
    snprintf(info, (size_t)infoSize, "%s", servers[index].info);
}
/* clients under 0 leaves the key out, as an empty server's getinfo does. */
static void server(int index, const char *address, const char *name, const char *map, int clients, int max) {
    char count[32] = "";
    if (clients >= 0)
        snprintf(count, sizeof(count), "\\clients\\%d", clients);
    snprintf(servers[index].address, sizeof(servers[index].address), "%s", address);
    snprintf(servers[index].info, sizeof(servers[index].info),
             "\\hostname\\%s\\mapname\\%s%s\\sv_maxclients\\%d\\gametype\\tdm", name, map, count, max);
}
/* The display list: LAN indexes in the list's order. */
static void listed(int count, const int *order) {
    sharedUiInfo.serverStatus.numDisplayServers = count;
    for (int i = 0; i < count; i++)
        sharedUiInfo.serverStatus.displayServers[i] = order[i];
}

static char command[128], scripts[8][32], deleted[64], owned[64];
static int commands, scriptCount, overs, clicks, deletes;
void Cbuf_ExecuteText(int when, const char *text) {
    assert(when == 2);
    snprintf(command, sizeof(command), "%s", text);
    commands++;
}
void UI_RunMenuScript(const char **args) {
    assert(scriptCount < 8);
    snprintf(scripts[scriptCount++], sizeof(scripts[0]), "%s", *args);
}
int UI_PlayLocalSoundAliasByName(const char *name) {
    if (!strcmp(name, "mouse_over")) overs++;
    else if (!strcmp(name, "mouse_click")) clicks++;
    else assert(!"unknown sound");
    return 1;
}
int Web_CanDeleteServer(const char *address) { return owned[0] && !strcmp(address, owned); }
void Web_DeleteServer(const char *address) { snprintf(deleted, sizeof(deleted), "%s", address); deletes++; }

enum { TEXT = 1, PIC, PROMPTS };
typedef struct { int kind; char text[160]; int bold; float x, y, w, h, cap, tracking; vec4_t color; int horz, vert;
    MaterialHandle material; char keys[96]; } Drawn;
static Drawn drawn[200];
static int drawCount;
static Drawn *record(int kind, const vec_t *color) {
    assert(drawCount < 200);
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
    assert(s1 == 0 && t1 == 0 && s2 == 1 && t2 == 1);
    d->x = x; d->y = y; d->w = w; d->h = h; d->horz = horzAlign; d->vert = vertAlign; d->material = material;
}
''' + wwii_header + r'''
static const vec4_t palette[] = { { 0.776f, 0.776f, 0.769f, 1 }, { 0.80f, 0.72f, 0.47f, 1 },
    { 0.851f, 0.843f, 0.827f, 1 }, { 0.812f, 0.812f, 0.812f, 1 }, { 1, 1, 1, 1 } };
const float *UI_WWIIMenu_Color(int color) { assert(color >= 0 && color < 5); return palette[color]; }
/* Each letter 1.2 capitals wide, 1.3 in Bold: wide enough for a long name to
 * be cut. */
static float width(const char *text, int bold, float cap, float tracking) {
    float n = (float)strlen(text);
    return n * cap * (bold ? 1.3f : 1.2f) + (n > 1 ? tracking * cap * (n - 1) : 0);
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
    int length = 0;
    for (int i = 0; i < count; i++)
        length += snprintf(d->keys + length, sizeof(d->keys) - (size_t)length, "%s %s|", keys[i], labels[i]);
    snprintf(d->keys + length, sizeof(d->keys) - (size_t)length, "%s %s", rightKey, rightLabel);
}
''' + header

checks = r'''
static int near(float a, float b) { return fabsf(a - b) < 2e-3f; }
static int sameColor(const float *a, const float *b) {
    return near(a[0], b[0]) && near(a[1], b[1]) && near(a[2], b[2]) && near(a[3], b[3]);
}
static const float idle[4] = { 0.776f, 0.776f, 0.769f, 1 }, gold[4] = { 0.80f, 0.72f, 0.47f, 1 };
static const float whiteColor[4] = { 1, 1, 1, 1 }, note[4] = { 0.70f, 0.70f, 0.68f, 1 };
static const float shade[4] = { 0, 0, 0, 0.55f }, rowFill[4] = { 0, 0, 0, 0.35f };
static const float focusEdge[4] = { 0.80f, 0.72f, 0.47f, 0.55f };

static const Drawn *find(const char *text) {
    for (int i = 0; i < drawCount; i++)
        if (drawn[i].kind == TEXT && !strcmp(drawn[i].text, text))
            return &drawn[i];
    return NULL;
}
static int text(const char *s, int bold, float x, float y, float cap, float tracking, const float *color) {
    const Drawn *d = find(s);
    return d && d->bold == bold && near(d->x, x) && near(d->y, y) && near(d->cap, cap) && near(d->tracking, tracking) &&
           sameColor(d->color, color) && d->horz == 1 && d->vert == 1;
}
static int pic(float x, float y, float w, float h, const float *color) {
    for (int i = 0; i < drawCount; i++) {
        const Drawn *d = &drawn[i];
        if (d->kind == PIC && near(d->x, x) && near(d->y, y) && near(d->w, w) && near(d->h, h) &&
            sameColor(d->color, color) && d->material == &white)
            return 1;
    }
    return 0;
}
static const char *prompts(void) {
    for (int i = 0; i < drawCount; i++)
        if (drawn[i].kind == PROMPTS)
            return i == drawCount - 1 ? drawn[i].keys : "not last";
    return "none";
}
static float baseline(int row) { return 137.6f + 32.0f * (float)row; }
/* A focused row's fill and frame from the screen's edge to 580, without a
 * left line. */
static int framed(int row) {
    const float y = baseline(row) - 18.3f, h = 26.7f, l = 0.6f;
    return pic(0, y, 580, h, rowFill) && pic(0, y, 580, l, focusEdge) && pic(0, y + h - l, 580, l, focusEdge) &&
           pic(580 - l, y + l, l, h - 2 * l, focusEdge) && !pic(0, y + l, l, h - 2 * l, focusEdge);
}
static int rows(void) {
    int n = 0;
    for (int i = 0; i < drawCount; i++)
        n += drawn[i].kind == PIC && near(drawn[i].h, 26.7f);
    return n;
}

static displayContextDef_t dc;
static menuDef_t bg = { { "bg", { 4 } } }, joinserver = { { "joinserver", { 6 } } }, other = { { "main", { 6 } } };
static void paint(void) {
    drawCount = 0;
    UI_JoinMenu_Paint(&dc, &joinserver);
}
static int key(int k) {
    int taken = UI_JoinMenu_KeyEvent(&dc, &joinserver, k, 1);
    assert(UI_JoinMenu_KeyEvent(&dc, &joinserver, k, 0) == taken);
    return taken;
}
/* The pointer over a point units from the left and top edges. */
static void point(float x, float y) {
    dc.cursorx = (int)lroundf(x * S / FULL_X);
    dc.cursory = (int)lroundf(y * S / FULL_Y);
}
static void open(void) {
    dc.menuStack[0] = &bg;
    dc.menuStack[1] = &joinserver;
    dc.openMenuCount = 2;
    UI_JoinMenu_Opened(&dc, &joinserver);
}

int main(void) {
    static const int all[3] = { 0, 1, 2 }, reordered[3] = { 2, 0, 1 }, twoLeft[2] = { 0, 2 };
    int scriptsBefore;

    /* joinserver up: covered, every key its own; other menus not. */
    dc.realTime = 1000;
    netSource = 1;
    open();
    assert(UI_JoinMenu_Covers(&dc, &joinserver) && !UI_JoinMenu_Covers(&dc, &other));
    assert(UI_JoinMenu_WantsKeys(&joinserver) && !UI_JoinMenu_WantsKeys(&other));
    assert(!UI_JoinMenu_KeyEvent(&dc, &other, 0x0d, 1));
    /* The rooms are the LAN source's: a player left on Internet lists them again. */
    assert(netSource == 0 && scriptCount == 1 && !strcmp(scripts[0], "UpdateFilter"));
    scriptCount = 0;
    open();
    assert(!scriptCount && dvarSets == 1);

    /* While the first refresh looks for rooms. */
    sharedUiInfo.serverStatus.refreshActive = 1;
    paint();
    assert(pic(0, 0, 640, 480, shade) && drawn[0].horz == 4 && drawn[0].vert == 4);
    assert(text("JOIN GAME", 1, 44.0f, 72.5f, 11.7f, 0.2f, gold));
    assert(text("Choose a game to join", 0, 44.0f, 90.0f, 6.1f, 0, note));
    assert(text("SERVER NAME", 0, 52.5f, 113.0f, 4.6f, 0.12f, gold) && text("MAP NAME", 0, 330.0f, 113.0f, 4.6f, 0.12f, gold));
    assert(text("PLAYERS", 0, 560.0f - width("PLAYERS", 0, 4.6f, 0.12f), 113.0f, 4.6f, 0.12f, gold));
    assert(text("Looking for servers...", 0, 52.5f, 137.6f, 7.5f, 0, note) && !rows());
    assert(!strcmp(prompts(), "ESC Back"));
    /* None found. */
    sharedUiInfo.serverStatus.refreshActive = 0;
    paint();
    assert(text("No servers are running. Start one from Start New Server.", 0, 52.5f, 137.6f, 7.5f, 0, note));

    /* Three rooms: each its name, its map as the arenas name it, its players
     * at the right; the first focused, gold and Bold, framed. */
    server(0, "127.0.0.1:28960", "^1Tonga", "mp_carentan", 2, 16);
    server(1, "127.0.0.2:28960", "Night Owls", "mp_toujane", -1, 8);
    server(2, "127.0.0.3:28960", "A very long server name that runs on and on", "mp_unknown", 12, 24);
    listed(3, all);
    paint();
    assert(text("Tonga", 1, 60.8f, baseline(0), 8.0f, 0, gold) && framed(0) && rows() == 1);
    assert(text("CARENTAN", 0, 330.0f, baseline(0), 8.0f, 0, whiteColor));
    assert(text("2/16", 0, 560.0f - width("2/16", 0, 8.0f, 0), baseline(0), 8.0f, 0, whiteColor));
    assert(text("Night Owls", 0, 52.5f, baseline(1), 7.5f, 0, idle) && text("TOUJANE", 0, 330.0f, baseline(1), 7.5f, 0, idle));
    assert(text("0/8", 0, 560.0f - width("0/8", 0, 7.5f, 0), baseline(1), 7.5f, 0, idle));
    /* A long name stops short of the map's column; an unknown map keeps its file's name. */
    {
        const Drawn *longName = NULL;
        for (int i = 0; i < drawCount; i++)
            if (drawn[i].kind == TEXT && !strncmp(drawn[i].text, "A very long", 11))
                longName = &drawn[i];
        assert(longName && strstr(longName->text, "...") && longName->x + width(longName->text, 0, 7.5f, 0) <= 318.0f);
        assert(longName->x + width(longName->text, 0, 7.5f, 0) + width("n", 0, 7.5f, 0) > 300.0f);
    }
    assert(text("MP_UNKNOWN", 0, 330.0f, baseline(2), 7.5f, 0, idle));
    assert(!strcmp(prompts(), "ENTER Join|ESC Back"));
    assert(sharedUiInfo.serverStatus.currentServer == 0);

    /* Keys move the focus, wrapping, with mouse_over; Enter joins the focused one by its address. */
    assert(key(0x9b) && overs == 1);
    paint();
    assert(framed(1) && text("Night Owls", 1, 60.8f, baseline(1), 8.0f, 0, gold) && sharedUiInfo.serverStatus.currentServer == 1);
    assert(key(0x9a) && key(0x9a) && overs == 3);
    paint();
    assert(framed(2));
    assert(key(0xcd) && key(0x09) && key(0xce) && overs == 6);
    paint();
    assert(framed(0));
    assert(key(0x0d) && clicks == 1 && commands == 1 && !strcmp(command, "connect 127.0.0.1:28960\n") && !thirdPerson);
    /* Characters do nothing; the key is still the screen's. */
    assert(key(0x400 | 'r') && commands == 1 && overs == 6);

    /* A refresh empties the list until the rooms answer: the last ones stay,
     * still focused and joinable; when they come back the focus follows its
     * server wherever it is listed. */
    assert(key(0x9b) && overs == 7);
    sharedUiInfo.serverStatus.refreshActive = 1;
    listed(0, all);
    paint();
    assert(rows() == 1 && framed(1) && find("Night Owls") && find("Tonga") && !find("Looking for servers..."));
    assert(key(0x0d) && !strcmp(command, "connect 127.0.0.2:28960\n") && commands == 2);
    listed(3, reordered);
    sharedUiInfo.serverStatus.refreshActive = 0;
    paint();
    assert(framed(2) && text("Night Owls", 1, 60.8f, baseline(2), 8.0f, 0, gold) && sharedUiInfo.serverStatus.currentServer == 2);
    /* The focused server gone, the focus stays where it was, within the list:
     * on the long name, cut shorter in Bold. */
    listed(2, twoLeft);
    paint();
    assert(framed(1) && rows() == 1 && sharedUiInfo.serverStatus.currentServer == 1);
    {
        const Drawn *focused = NULL;
        for (int i = 0; i < drawCount; i++)
            if (drawn[i].kind == TEXT && !strncmp(drawn[i].text, "A very long", 11))
                focused = &drawn[i];
        assert(focused && focused->bold && sameColor(focused->color, gold) && near(focused->x, 60.8f) &&
               near(focused->y, baseline(1)) && strstr(focused->text, "...") &&
               focused->x + width(focused->text, 1, 8.0f, 0) <= 318.0f);
    }
    listed(3, all);
    paint();
    assert(framed(2));

    /* Every 5 s a refresh, the retail Quick Refresh's, but not over one. */
    scriptCount = 0;
    dc.realTime = 5999;
    paint();
    assert(!scriptCount);
    dc.realTime = 6000;
    sharedUiInfo.serverStatus.refreshActive = 1;
    paint();
    assert(!scriptCount);
    sharedUiInfo.serverStatus.refreshActive = 0;
    dc.realTime = 6500;
    paint();
    assert(scriptCount == 1 && !strcmp(scripts[0], "UpdateFilter"));
    dc.realTime = 11499;
    paint();
    assert(scriptCount == 1);
    dc.realTime = 11500;
    paint();
    assert(scriptCount == 2 && !strcmp(scripts[1], "UpdateFilter"));

    /* Delete: only a room this browser created, and only then prompted. */
    scriptCount = 0;
    assert(key(0x7f) && key(0xa2) && !deletes);
    snprintf(owned, sizeof(owned), "127.0.0.2:28960");
    paint();
    assert(!strcmp(prompts(), "ENTER Join|ESC Back"));
    assert(key(0x9a));
    paint();
    assert(framed(1) && !strcmp(prompts(), "ENTER Join|DEL Delete my server|ESC Back"));
    clicks = 0;
    assert(key(0xa2) && deletes == 1 && !strcmp(deleted, owned) && clicks == 1);
    assert(key(0x7f) && deletes == 2);
    owned[0] = '\0';

    /* The pointer focuses the row it moves onto, a click joins it; a still
     * pointer, or one off the rows, leaves the focus to the keys. */
    overs = clicks = 0;
    point(300, baseline(0) - 4);
    paint();
    assert(overs == 1 && sharedUiInfo.serverStatus.currentServer == 0);
    paint();
    assert(overs == 1);
    assert(key(0x9b) && overs == 2);
    paint();
    assert(sharedUiInfo.serverStatus.currentServer == 1 && overs == 2);
    point(300, baseline(2) - 4);
    paint();
    assert(overs == 3 && sharedUiInfo.serverStatus.currentServer == 2);
    assert(key(0xc8) && !strcmp(command, "connect 127.0.0.3:28960\n") && clicks == 1);
    point(600, 400);
    paint();
    scriptsBefore = commands;
    assert(key(0xc8) && commands == scriptsBefore);

    /* Escape and the right button go back as the retail Escape, its refresh ended first. */
    scriptCount = clicks = 0;
    assert(key(0x1b) && scriptCount == 2 && !strcmp(scripts[0], "StopRefresh") && !strcmp(scripts[1], "closeJoin"));
    assert(key(0xc9) && scriptCount == 4 && clicks == 2);

    /* Hidden or closed, it is not covered and paints nothing. */
    joinserver.window.dynamicFlags[0] = 2;
    assert(!UI_JoinMenu_Covers(&dc, &joinserver) && !UI_JoinMenu_KeyEvent(&dc, &joinserver, 0x0d, 1));
    paint();
    assert(!drawCount);
    joinserver.window.dynamicFlags[0] = 6;
    dc.openMenuCount = 1;
    assert(!UI_JoinMenu_Covers(&dc, &joinserver));
    paint();
    assert(!drawCount);
    puts("ok");
    return 0;
}
'''

mutants = [
    # The last servers dropped while a refresh empties the list.
    ('        if (!sharedUiInfo.serverStatus.refreshActive)\n            s_join.rowCount = 0;\n',
     '        s_join.rowCount = 0;\n'),
    # No refresh, one over another, or every frame.
    ('        Join_Script("UpdateFilter");\n    }\n    Join_Gather();', '    }\n    Join_Gather();'),
    ('dc->realTime - s_join.nextRefresh >= 0 && !sharedUiInfo.serverStatus.refreshActive',
     'dc->realTime - s_join.nextRefresh >= 0'),
    ('        s_join.nextRefresh = dc->realTime + JOIN_REFRESH_MSEC;\n        Join_Script',
     '        Join_Script'),
    # The focus by its row, not its server.
    ('        if (!strcmp(s_join.rows[i].address, s_join.focusAddress)) {', '        if (0) {'),
    ('    if (s_join.focus >= s_join.rowCount)\n        s_join.focus = s_join.rowCount - 1;\n', ''),
    ('    if (s_join.focus < sharedUiInfo.serverStatus.numDisplayServers)\n'
     '        sharedUiInfo.serverStatus.currentServer = s_join.focus;\n', ''),
    # Joining without the address, or keeping the third-person view.
    ('"connect %s\\n", s_join.rows[s_join.focus].address', '"connect %s\\n", s_join.rows[0].address'),
    ('    Dvar_SetBoolByName("cg_thirdPerson", 0);\n', ''),
    # Deleting what is not the player's, or never prompting it.
    ('    if (!Join_CanDelete())\n        return;\n', ''),
    ('s_join.rowCount ? (Join_CanDelete() ? 2 : 1) : 0', 's_join.rowCount ? 1 : 0'),
    # Back without ending the refresh first; no LAN source forced.
    ('    Join_Script("StopRefresh");\n', ''),
    ('    if (Dvar_GetInt("ui_netSource") != 0) {', '    if (0) {'),
    # The pointer: never following, or following when still.
    ('    if (moved && s_join.hover >= 0)\n        Join_Focus(s_join.hover);\n', ''),
    ('    if (moved && s_join.hover >= 0)', '    if (s_join.hover >= 0)'),
    # An empty server's players as "/8".
    ('clients[0] ? clients : "0"', 'clients'),
    # Long names over the map's column; the map in its file's name.
    ('JOIN_MAP_X - JOIN_COLUMN_GAP - x, text, sizeof(text)', '1000.0f, text, sizeof(text)'),
    ('            return sharedUiInfo.mapList[i].mapName;', '            return mapName;'),
    # Hidden only while up; the focus frame's left line.
    ('        if (Join_IsMenu(menu) && (menu->window.dynamicFlags[0] & JOIN_WINDOW_VISIBLE))',
     '        if (Join_IsMenu(menu))'),
    ('    edge.x = rect->x + rect->w - JOIN_LINE;\n', '    edge.x = rect->x;\n'),
]


def program(text):
    return support + re.sub(r'^#include "[^"]+"\n', '', text, flags=re.M) + checks


def run(directory, name, text, expect_pass):
    path = Path(directory) / f'{name}.c'
    binary = Path(directory) / name
    path.write_text(text)
    flags = ['-Wall', '-Werror'] if expect_pass else ['-w']
    subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    *flags, '-Wno-unused-function', str(path), '-lm', '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert (result.returncode == 0) == expect_pass, (name, result.stdout[-500:], result.stderr[-2000:])


with tempfile.TemporaryDirectory(prefix='cod2-join-menu-') as directory:
    run(directory, 'join', program(source), True)
    for index, (before, after) in enumerate(mutants):
        assert source.count(before) == 1, before
        run(directory, f'mutant{index}', program(source.replace(before, after)), False)
print(f'PASS: Join Game lists the rooms by name, map and players under the WWII title and headings, the focus framed '
      f'gold from the screen\'s edge; keys and the pointer move it with mouse_over; Enter or a click joins the focused '
      f'server by its address, DEL deletes only a room this browser made, Escape ends the refresh and closes as the '
      f'retail one; a refresh every 5 s keeps the last servers and the focus on its server; {len(mutants)} mutants fail')
