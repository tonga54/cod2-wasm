#include "common_types.h"
#include "imports.h"
#include "headers/PC/ui_mp/ui_join_menu.h"
#include "headers/PC/ui_mp/ui_wwii_menu.h"
#include <stdio.h>
#include <string.h>

extern sharedUiInfo_t sharedUiInfo;
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1,
                              float s2, float t2, const vec_t *color, MaterialHandle material);
extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);
extern void CalcScreenX(float *x, int horzAlign);
extern void CalcScreenY(float *y, int vertAlign);
extern void Cbuf_ExecuteText(int when, const char *text);
extern int Dvar_GetInt(const char *name);
extern void Dvar_SetBoolByName(const char *name, int value);
extern void Dvar_SetIntByName(const char *name, int value);
extern char *I_CleanStr(char *string);
extern int I_stricmp(const char *s0, const char *s1);
extern const char *Info_ValueForKey(const char *s, const char *key);
extern void LAN_GetServerAddressString(int source, int index, char *addr, int addrSize);
extern void LAN_GetServerInfo(int source, int index, char *info, int infoSize);
extern int UI_PlayLocalSoundAliasByName(const char *name);
extern void UI_RunMenuScript(const char **args);
#ifdef __EMSCRIPTEN__
extern int Web_CanDeleteServer(const char *address);
extern void Web_DeleteServer(const char *address);
#endif

#define JOIN_MENU "joinserver"
#define JOIN_ALIGN_LEFT 1
#define JOIN_ALIGN_TOP 1
#define JOIN_ALIGN_FULLSCREEN 4
#define JOIN_WINDOW_VISIBLE 4

#define JOIN_KEY_TAB 0x09
#define JOIN_KEY_ENTER 0x0d
#define JOIN_KEY_ESCAPE 0x1b
#define JOIN_KEY_BACKSPACE 0x7f
#define JOIN_KEY_UP 0x9a
#define JOIN_KEY_DOWN 0x9b
#define JOIN_KEY_DEL 0xa2
#define JOIN_KEY_KP_ENTER 0xbf
#define JOIN_KEY_MOUSE1 0xc8
#define JOIN_KEY_MOUSE2 0xc9
#define JOIN_KEY_WHEEL_DOWN 0xcd
#define JOIN_KEY_WHEEL_UP 0xce
#define JOIN_KEY_CHAR 0x400

/* Start New Server's title, rows and focus, in menu units from the left and
 * top edges. */
#define JOIN_PAGE_X 44.0f
#define JOIN_TITLE_BASELINE 72.5f
#define JOIN_TITLE_CAP 11.7f
#define JOIN_TITLE_TRACKING 0.2f
#define JOIN_NOTE_BASELINE 90.0f
#define JOIN_NOTE_CAP 6.1f
#define JOIN_TEXT_X 52.5f
#define JOIN_ROW_ABOVE 18.3f
#define JOIN_ROW_H 26.7f
#define JOIN_ROW_SPACING 32.0f
#define JOIN_FOCUS_INDENT 8.3f
#define JOIN_LINE 0.6f
#define JOIN_LIST_CAP 7.5f
#define JOIN_LIST_FOCUS_CAP 8.0f
#define JOIN_LIST_VISIBLE 9
/* The columns' headings over the first row, as the class stats' captions;
 * the map's column starts at JOIN_MAP_X, the players end at
 * JOIN_PLAYERS_RIGHT and a focused row's frame at JOIN_ROW_RIGHT. */
#define JOIN_HEADING_BASELINE 113.0f
#define JOIN_HEADING_CAP 4.6f
#define JOIN_HEADING_TRACKING 0.12f
#define JOIN_LIST_TOP 137.6f
#define JOIN_MAP_X 330.0f
#define JOIN_PLAYERS_RIGHT 560.0f
#define JOIN_ROW_RIGHT 580.0f
#define JOIN_COLUMN_GAP 12.0f
/* The retail Quick Refresh's work, every 5 s while the screen is up. */
#define JOIN_REFRESH_MSEC 5000
#define JOIN_MAX_ROWS 16

typedef struct {
    float x, y, w, h;
} JoinRect;

typedef struct {
    char address[64];
    char name[64];
    char map[64];
    char players[24];
} JoinRow;

static struct {
    /* The servers last listed, kept while a refresh empties the list. */
    JoinRow rows[JOIN_MAX_ROWS];
    int rowCount;
    int focus;
    /* The focused server, found again by its address after a refresh. */
    char focusAddress[64];
    int scroll;
    int hover;
    int cursorX, cursorY;
    int nextRefresh;
} s_join;

static const vec4_t s_joinShade = { 0.0f, 0.0f, 0.0f, 0.55f };
static const vec4_t s_joinNote = { 0.70f, 0.70f, 0.68f, 1.0f };
static const vec4_t s_joinRowFocus = { 0.0f, 0.0f, 0.0f, 0.35f };
static const vec4_t s_joinFocusEdge = { 0.80f, 0.72f, 0.47f, 0.55f };

static int Join_IsMenu(const menuDef_t *menu)
{
    return menu && menu->window.name && !I_stricmp(menu->window.name, JOIN_MENU);
}

static int Join_Up(const displayContextDef_t *dc)
{
    int i;

    for (i = 0; i < dc->openMenuCount; i++) {
        const menuDef_t *menu = dc->menuStack[i];

        if (Join_IsMenu(menu) && (menu->window.dynamicFlags[0] & JOIN_WINDOW_VISIBLE))
            return 1;
    }
    return 0;
}

int UI_JoinMenu_Covers(displayContextDef_t *dc, const menuDef_t *menu)
{
    return Join_IsMenu(menu) && Join_Up(dc);
}

int UI_JoinMenu_WantsKeys(const menuDef_t *menu)
{
    return Join_IsMenu(menu);
}

static void Join_Script(const char *name)
{
    UI_RunMenuScript(&name);
}

/* The rooms are the LAN source's: the retail Source control is gone, so a
 * player who left it on another lists the rooms again. */
void UI_JoinMenu_Opened(displayContextDef_t *dc, menuDef_t *menu)
{
    if (!Join_IsMenu(menu))
        return;
    memset(&s_join, 0, sizeof(s_join));
    s_join.hover = -1;
    s_join.cursorX = dc->cursorx;
    s_join.cursorY = dc->cursory;
    s_join.nextRefresh = dc->realTime + JOIN_REFRESH_MSEC;
    if (Dvar_GetInt("ui_netSource") != 0) {
        Dvar_SetIntByName("ui_netSource", 0);
        Join_Script("UpdateFilter");
    }
}

/* The map's name as the arenas give it, else its file's. */
static const char *Join_MapName(const char *mapName)
{
    int i;

    for (i = 0; i < sharedUiInfo.mapCount; i++) {
        if (sharedUiInfo.mapList[i].mapLoadName && !I_stricmp(mapName, sharedUiInfo.mapList[i].mapLoadName))
            return sharedUiInfo.mapList[i].mapName;
    }
    return mapName;
}

/* The listed servers in the list's order. A refresh empties the list until
 * its rooms answer, so the last ones stay until it ends. */
static void Join_Gather(void)
{
    const int source = Dvar_GetInt("ui_netSource");
    int count = sharedUiInfo.serverStatus.numDisplayServers, i;
    char info[1024], clients[16];

    if (count <= 0) {
        if (!sharedUiInfo.serverStatus.refreshActive)
            s_join.rowCount = 0;
        return;
    }
    if (count > JOIN_MAX_ROWS)
        count = JOIN_MAX_ROWS;
    for (i = 0; i < count; i++) {
        JoinRow *row = &s_join.rows[i];
        const int server = sharedUiInfo.serverStatus.displayServers[i];

        LAN_GetServerAddressString(source, server, row->address, sizeof(row->address));
        LAN_GetServerInfo(source, server, info, sizeof(info));
        snprintf(row->name, sizeof(row->name), "%s", Info_ValueForKey(info, "hostname"));
        I_CleanStr(row->name);
        if (!row->name[0])
            snprintf(row->name, sizeof(row->name), "%s", row->address);
        snprintf(row->map, sizeof(row->map), "%s", Join_MapName(Info_ValueForKey(info, "mapname")));
        /* An empty server's getinfo leaves its clients out. */
        snprintf(clients, sizeof(clients), "%s", Info_ValueForKey(info, "clients"));
        snprintf(row->players, sizeof(row->players), "%s/%s", clients[0] ? clients : "0",
                 Info_ValueForKey(info, "sv_maxclients"));
    }
    s_join.rowCount = count;
}

/* The focus stays on its server across refreshes; a server gone leaves it on
 * the row in its place. The retail list's selection follows it. */
static void Join_KeepFocus(void)
{
    int i;

    for (i = 0; i < s_join.rowCount; i++) {
        if (!strcmp(s_join.rows[i].address, s_join.focusAddress)) {
            s_join.focus = i;
            break;
        }
    }
    if (s_join.focus >= s_join.rowCount)
        s_join.focus = s_join.rowCount - 1;
    if (s_join.focus < 0)
        s_join.focus = 0;
    if (s_join.rowCount)
        snprintf(s_join.focusAddress, sizeof(s_join.focusAddress), "%s", s_join.rows[s_join.focus].address);
    if (s_join.focus < sharedUiInfo.serverStatus.numDisplayServers)
        sharedUiInfo.serverStatus.currentServer = s_join.focus;
}

static void Join_Focus(int row)
{
    if (row < 0 || row >= s_join.rowCount || row == s_join.focus)
        return;
    s_join.focus = row;
    snprintf(s_join.focusAddress, sizeof(s_join.focusAddress), "%s", s_join.rows[row].address);
    if (row < sharedUiInfo.serverStatus.numDisplayServers)
        sharedUiInfo.serverStatus.currentServer = row;
    UI_PlayLocalSoundAliasByName("mouse_over");
}

static void Join_Move(int delta)
{
    if (s_join.rowCount > 0)
        Join_Focus((s_join.focus + delta + s_join.rowCount) % s_join.rowCount);
}

/* The retail JoinServer, by the focused server's address, so a refresh
 * emptying the list does not lose it. */
static void Join_Connect(void)
{
    char command[96];

    if (s_join.focus < 0 || s_join.focus >= s_join.rowCount)
        return;
    UI_PlayLocalSoundAliasByName("mouse_click");
    Dvar_SetBoolByName("cg_thirdPerson", 0);
    snprintf(command, sizeof(command), "connect %s\n", s_join.rows[s_join.focus].address);
    Cbuf_ExecuteText(2, command);
}

/* Only the browser that created a room may delete it. */
static int Join_CanDelete(void)
{
#ifdef __EMSCRIPTEN__
    return s_join.focus >= 0 && s_join.focus < s_join.rowCount &&
           Web_CanDeleteServer(s_join.rows[s_join.focus].address);
#else
    return 0;
#endif
}

static void Join_Delete(void)
{
    if (!Join_CanDelete())
        return;
    UI_PlayLocalSoundAliasByName("mouse_click");
#ifdef __EMSCRIPTEN__
    Web_DeleteServer(s_join.rows[s_join.focus].address);
#endif
}

/* The retail Escape's closeJoin, which first only ends a refresh. */
static void Join_Back(void)
{
    UI_PlayLocalSoundAliasByName("mouse_click");
    Join_Script("StopRefresh");
    Join_Script("closeJoin");
}

int UI_JoinMenu_KeyEvent(displayContextDef_t *dc, menuDef_t *menu, int key, int down)
{
    if (!UI_JoinMenu_Covers(dc, menu))
        return 0;
    if (!down || (key & JOIN_KEY_CHAR))
        return 1;
    switch (key) {
    case JOIN_KEY_ESCAPE:
    case JOIN_KEY_MOUSE2:
        Join_Back();
        break;
    case JOIN_KEY_ENTER:
    case JOIN_KEY_KP_ENTER:
        Join_Connect();
        break;
    case JOIN_KEY_MOUSE1:
        if (s_join.hover >= 0) {
            Join_Focus(s_join.hover);
            Join_Connect();
        }
        break;
    case JOIN_KEY_UP:
    case JOIN_KEY_WHEEL_UP:
        Join_Move(-1);
        break;
    case JOIN_KEY_DOWN:
    case JOIN_KEY_WHEEL_DOWN:
    case JOIN_KEY_TAB:
        Join_Move(1);
        break;
    case JOIN_KEY_BACKSPACE:
    case JOIN_KEY_DEL:
        Join_Delete();
        break;
    default:
        break;
    }
    return 1;
}

static int Join_Hover(const displayContextDef_t *dc, const JoinRect *rect)
{
    float x = rect->x, y = rect->y, w = rect->w, h = rect->h;
    float cx = (float)dc->cursorx, cy = (float)dc->cursory;

    CalcScreenPlacement(&x, &y, &w, &h, JOIN_ALIGN_LEFT, JOIN_ALIGN_TOP);
    CalcScreenX(&cx, JOIN_ALIGN_FULLSCREEN);
    CalcScreenY(&cy, JOIN_ALIGN_FULLSCREEN);
    return cx >= x && cx < x + w && cy >= y && cy < y + h;
}

static void Join_Fill(const JoinRect *rect, const vec_t *color)
{
    CL_DrawStretchPic(rect->x, rect->y, rect->w, rect->h, JOIN_ALIGN_LEFT, JOIN_ALIGN_TOP, 0.0f, 0.0f, 1.0f, 1.0f,
                      color, sharedUiInfo.assets.whiteMaterial);
}

/* Lines along the top, the bottom and the right of a row that runs off the
 * screen's left edge; they do not overlap. */
static void Join_Frame(const JoinRect *rect, const vec_t *color)
{
    JoinRect edge = *rect;

    edge.h = JOIN_LINE;
    Join_Fill(&edge, color);
    edge.y = rect->y + rect->h - JOIN_LINE;
    Join_Fill(&edge, color);
    edge.x = rect->x + rect->w - JOIN_LINE;
    edge.y = rect->y + JOIN_LINE;
    edge.w = JOIN_LINE;
    edge.h = rect->h - 2.0f * JOIN_LINE;
    Join_Fill(&edge, color);
}

static void Join_Text(const char *text, int bold, float x, float baseline, float cap, float tracking,
                      const vec_t *color)
{
    UI_WWIIMenu_Text(text, bold, x, baseline, JOIN_ALIGN_LEFT, JOIN_ALIGN_TOP, cap, tracking, color);
}

/* text in capitals, cut with an ellipsis to fit width. */
static void Join_Fit(const char *text, int caps, int bold, float cap, float width, char *out, int size)
{
    char source[64];
    int length, i;

    for (i = 0; text[i] && i < (int)sizeof(source) - 1; i++)
        source[i] = (char)(caps && text[i] >= 'a' && text[i] <= 'z' ? text[i] - 32 : text[i]);
    source[i] = '\0';
    snprintf(out, (size_t)size, "%s", source);
    for (length = (int)strlen(source); length > 0 && UI_WWIIMenu_Width(out, bold, cap, 0.0f) > width; length--) {
        while (length > 1 && source[length - 1] == ' ')
            length--;
        snprintf(out, (size_t)size, "%.*s...", length - 1, source);
    }
}

/* The first row shown, keeping the focus among the visible ones. */
static int Join_Scroll(void)
{
    if (s_join.focus < s_join.scroll)
        s_join.scroll = s_join.focus;
    if (s_join.focus >= s_join.scroll + JOIN_LIST_VISIBLE)
        s_join.scroll = s_join.focus - JOIN_LIST_VISIBLE + 1;
    if (s_join.scroll > s_join.rowCount - JOIN_LIST_VISIBLE)
        s_join.scroll = s_join.rowCount - JOIN_LIST_VISIBLE;
    if (s_join.scroll < 0)
        s_join.scroll = 0;
    return s_join.scroll;
}

/* A server's row: its name, gold and Bold when focused, its map and its
 * players; the frame runs from the screen's left edge. */
static void Join_PaintRow(displayContextDef_t *dc, int index, float baseline, int focused)
{
    const JoinRow *row = &s_join.rows[index];
    const JoinRect rect = { 0.0f, baseline - JOIN_ROW_ABOVE, JOIN_ROW_RIGHT, JOIN_ROW_H };
    const float cap = focused ? JOIN_LIST_FOCUS_CAP : JOIN_LIST_CAP;
    const float x = JOIN_TEXT_X + (focused ? JOIN_FOCUS_INDENT : 0.0f);
    const float players = UI_WWIIMenu_Width(row->players, 0, cap, 0.0f);
    const vec_t *other = UI_WWIIMenu_Color(focused ? WWII_COLOR_WHITE : WWII_COLOR_IDLE);
    char text[72];

    if (Join_Hover(dc, &rect))
        s_join.hover = index;
    if (focused) {
        Join_Fill(&rect, s_joinRowFocus);
        Join_Frame(&rect, s_joinFocusEdge);
    }
    Join_Fit(row->name, 0, focused, cap, JOIN_MAP_X - JOIN_COLUMN_GAP - x, text, sizeof(text));
    Join_Text(text, focused, x, baseline, cap, 0.0f, UI_WWIIMenu_Color(focused ? WWII_COLOR_GOLD : WWII_COLOR_IDLE));
    Join_Fit(row->map, 1, 0, cap, JOIN_PLAYERS_RIGHT - players - JOIN_COLUMN_GAP - JOIN_MAP_X, text, sizeof(text));
    Join_Text(text, 0, JOIN_MAP_X, baseline, cap, 0.0f, other);
    Join_Text(row->players, 0, JOIN_PLAYERS_RIGHT - players, baseline, cap, 0.0f, other);
}

void UI_JoinMenu_Paint(displayContextDef_t *dc, menuDef_t *menu)
{
    static const char *const keys[] = { "ENTER", "DEL" };
    static const char *const labels[] = { "Join", "Delete my server" };
    const vec_t *gold = UI_WWIIMenu_Color(WWII_COLOR_GOLD);
    int moved, first, i;

    if (!UI_JoinMenu_Covers(dc, menu))
        return;
    /* The text stands on a dark scene, as WWII's. */
    CL_DrawStretchPic(0.0f, 0.0f, 640.0f, 480.0f, JOIN_ALIGN_FULLSCREEN, JOIN_ALIGN_FULLSCREEN, 0.0f, 0.0f, 1.0f, 1.0f,
                      s_joinShade, sharedUiInfo.assets.whiteMaterial);
    moved = dc->cursorx != s_join.cursorX || dc->cursory != s_join.cursorY;
    s_join.cursorX = dc->cursorx;
    s_join.cursorY = dc->cursory;
    s_join.hover = -1;
    if (dc->realTime - s_join.nextRefresh >= 0 && !sharedUiInfo.serverStatus.refreshActive) {
        s_join.nextRefresh = dc->realTime + JOIN_REFRESH_MSEC;
        Join_Script("UpdateFilter");
    }
    Join_Gather();
    Join_KeepFocus();

    Join_Text("JOIN GAME", 1, JOIN_PAGE_X, JOIN_TITLE_BASELINE, JOIN_TITLE_CAP, JOIN_TITLE_TRACKING, gold);
    Join_Text("Choose a game to join", 0, JOIN_PAGE_X, JOIN_NOTE_BASELINE, JOIN_NOTE_CAP, 0.0f, s_joinNote);
    Join_Text("SERVER NAME", 0, JOIN_TEXT_X, JOIN_HEADING_BASELINE, JOIN_HEADING_CAP, JOIN_HEADING_TRACKING, gold);
    Join_Text("MAP NAME", 0, JOIN_MAP_X, JOIN_HEADING_BASELINE, JOIN_HEADING_CAP, JOIN_HEADING_TRACKING, gold);
    Join_Text("PLAYERS", 0,
              JOIN_PLAYERS_RIGHT - UI_WWIIMenu_Width("PLAYERS", 0, JOIN_HEADING_CAP, JOIN_HEADING_TRACKING),
              JOIN_HEADING_BASELINE, JOIN_HEADING_CAP, JOIN_HEADING_TRACKING, gold);
    if (s_join.rowCount) {
        first = Join_Scroll();
        for (i = first; i < s_join.rowCount && i < first + JOIN_LIST_VISIBLE; i++)
            Join_PaintRow(dc, i, JOIN_LIST_TOP + (float)(i - first) * JOIN_ROW_SPACING, i == s_join.focus);
    } else {
        Join_Text(sharedUiInfo.serverStatus.refreshActive ? "Looking for servers..."
                                                          : "No servers are running. Start one from Start New Server.",
                  0, JOIN_TEXT_X, JOIN_LIST_TOP, JOIN_LIST_CAP, 0.0f, s_joinNote);
    }
    /* Pointer motion moves the focus to what it is over now; keys move it
     * otherwise. */
    if (moved && s_join.hover >= 0)
        Join_Focus(s_join.hover);
    UI_WWIIMenu_Prompts(keys, labels, s_join.rowCount ? (Join_CanDelete() ? 2 : 1) : 0, "ESC", "Back");
}
