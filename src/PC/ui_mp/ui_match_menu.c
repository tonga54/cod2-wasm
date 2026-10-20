#include "common_types.h"
#include "imports.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include "headers/PC/ui_mp/ui_match_menu.h"
#include "headers/PC/ui_mp/ui_wwii_menu.h"
#include <stdio.h>
#include <string.h>

extern sharedUiInfo_t sharedUiInfo;
extern const char *UI_SafeTranslateString(const char *reference);
extern const char *SEH_StringEd_GetString(const char *reference);
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1,
                              float s2, float t2, const vec_t *color, MaterialHandle material);
extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);
extern void CalcScreenX(float *x, int horzAlign);
extern void CalcScreenY(float *y, int vertAlign);
extern int Dvar_GetInt(const char *name);
extern const char *Dvar_GetString(const char *name);
extern void Dvar_SetIntByName(const char *name, int value);
extern void Dvar_SetStringByName(const char *name, const char *value);
extern char *I_CleanStr(char *string);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern int I_stricmp(const char *s0, const char *s1);
extern int I_strnicmp(const char *s0, const char *s1, size_t n);
extern void Menus_CloseByName(displayContextDef_t *dc, const char *name);
extern int UI_PlayLocalSoundAliasByName(const char *name);
extern void UI_RunMenuScript(const char **args);

#define MATCH_ALIGN_LEFT 1
#define MATCH_ALIGN_TOP 1
#define MATCH_ALIGN_FULLSCREEN 4
#define MATCH_WINDOW_VISIBLE 4

#define MATCH_KEY_TAB 0x09
#define MATCH_KEY_ENTER 0x0d
#define MATCH_KEY_ESCAPE 0x1b
#define MATCH_KEY_UP 0x9a
#define MATCH_KEY_DOWN 0x9b
#define MATCH_KEY_LEFT 0x9c
#define MATCH_KEY_RIGHT 0x9d
#define MATCH_KEY_KP_ENTER 0xbf
#define MATCH_KEY_MOUSE1 0xc8
#define MATCH_KEY_MOUSE2 0xc9
#define MATCH_KEY_WHEEL_DOWN 0xcd
#define MATCH_KEY_WHEEL_UP 0xce
#define MATCH_KEY_CHAR 0x400

#define MATCH_MENU "createserver"

/* Units from the screen's left and top edges, measured on the WWII captures
 * at 1024 x 576, 1.2 px a unit. A page's title is Bold capitals 11.7 tall,
 * 0.2 apart, inked from 45.0 on a baseline 72.5 down; its note is Regular,
 * capitals 6.1 tall, on one 90.0 down. */
#define MATCH_PAGE_X 44.0f
#define MATCH_TITLE_BASELINE 72.5f
#define MATCH_TITLE_CAP 11.7f
#define MATCH_TITLE_TRACKING 0.2f
#define MATCH_NOTE_BASELINE 90.0f
#define MATCH_NOTE_CAP 6.1f
/* The match page's boxes: 186.6 by 48.3 from 44.2, the first 111.7 down,
 * 5.0 apart. A box's name is Bold capitals 9.0 tall, 10.0 focused, on a
 * baseline 27.0 inside it; its value Regular 5.5 tall, 6.5 focused, 38.3
 * inside; both inked from 53.3. Focused, a box is filled black under a band
 * a quarter of its height, framed in gold with an edge 2.5 wide at its
 * left, and a chevron stands before its name. */
#define MATCH_BOX_X 44.2f
#define MATCH_BOX_W 186.6f
#define MATCH_BOX_H 48.3f
#define MATCH_BOX_TOP 111.7f
#define MATCH_GAP 5.0f
#define MATCH_BOX_NAME_BASELINE 27.0f
#define MATCH_BOX_VALUE_BASELINE 38.3f
#define MATCH_BOX_NAME_CAP 9.0f
#define MATCH_BOX_FOCUS_NAME_CAP 10.0f
#define MATCH_BOX_VALUE_CAP 5.5f
#define MATCH_BOX_FOCUS_VALUE_CAP 6.5f
#define MATCH_BOX_BAND 0.26f
#define MATCH_BOX_EDGE 2.5f
#define MATCH_CHEVRON_X 1.0f
#define MATCH_LINE 0.6f
#define MATCH_TEXT_X 52.5f
/* Rows stand 32 apart. A focused row is framed from the screen's edge to its
 * column's right, 18.3 over its baseline and 8.4 under it, and its text
 * turns gold and Bold 8.3 further right. The match page's rows are Regular
 * capitals 8.3 tall; the lists', 7.5 and 8.0 focused, from 129.6 down. */
#define MATCH_ROW_ABOVE 18.3f
#define MATCH_ROW_H 26.7f
#define MATCH_ROW_SPACING 32.0f
#define MATCH_FOCUS_INDENT 8.3f
#define MATCH_ROW_CAP 8.3f
#define MATCH_LIST_TOP 129.6f
#define MATCH_LIST_CAP 7.5f
#define MATCH_LIST_FOCUS_CAP 8.0f
#define MATCH_LIST_RIGHT 230.8f
#define MATCH_LIST_VISIBLE 9
/* Bot setup's rows reach 336.7 from the edge. A setting's name is as typed,
 * capitals 8.3 tall; its value 7.5 tall, centred 195.5 right of the name's
 * pen, between arrows 5.8 by 9.2 that stand 72.7 to either side, their
 * middles 4.0 over the baseline. */
#define MATCH_SETTING_RIGHT 336.7f
#define MATCH_SETTING_CAP 8.3f
#define MATCH_VALUE_CAP 7.5f
#define MATCH_VALUE_OFFSET 195.5f
#define MATCH_ARROW_OFFSET 72.7f
#define MATCH_ARROW_W 5.8f
#define MATCH_ARROW_H 9.2f
#define MATCH_ARROW_RISE 4.0f
/* The focused item's page, dimmed, 216.5 right of the column. */
#define MATCH_PREVIEW_SHIFT 216.5f
/* The panel right of a list: a map's levelshot, 4:3, cropped to 289.2 by
 * 162.7 from 284.2, 101.7 down; titles in Bold capitals 12.5 tall, 0.13
 * apart; notes in Regular, capitals 6.6 tall, lines 10.0 apart. Bot setup's
 * panel stands right of its longer rows. */
#define MATCH_PANEL_X 283.8f
#define MATCH_SHOT_X 284.2f
#define MATCH_SHOT_TOP 101.7f
#define MATCH_SHOT_W 289.2f
#define MATCH_SHOT_H 162.7f
#define MATCH_SHOT_CROP 0.125f
#define MATCH_PANEL_TITLE_CAP 12.5f
#define MATCH_PANEL_TITLE_TRACKING 0.13f
#define MATCH_PANEL_NOTE_CAP 6.6f
#define MATCH_PANEL_LEADING 10.0f
#define MATCH_PANEL_NOTE_LINES 4
#define MATCH_MODE_TITLE_BASELINE 123.3f
#define MATCH_MODE_NOTE_BASELINE 135.8f
#define MATCH_MAP_TITLE_BASELINE 294.2f
#define MATCH_MAP_NOTE_BASELINE 309.2f
#define MATCH_BOTS_PANEL_X 389.0f
#define MATCH_BOTS_TITLE_BASELINE 113.3f
#define MATCH_BOTS_NOTE_BASELINE 127.5f
#define MATCH_BOTS_NOTE_W 220.0f

/* rooms.py keeps at most this many letters of a room's name. */
#define MATCH_ROOM_NAME_MAX 32

enum { MATCH_PAGE_MATCH, MATCH_PAGE_MODE, MATCH_PAGE_MAP, MATCH_PAGE_BOTS, MATCH_PAGE_RULES, MATCH_PAGE_COUNT };
enum { MATCH_ITEM_MODE, MATCH_ITEM_MAP, MATCH_ITEM_RULES, MATCH_ITEM_BOTS, MATCH_ITEM_START, MATCH_ITEM_COUNT };
enum { MATCH_SETTING_BOTS, MATCH_SETTING_DIFFICULTY, MATCH_SETTING_COUNT };
enum { MATCH_RULE_TIME, MATCH_RULE_SCORE, MATCH_RULE_COUNT };

typedef struct {
    float x, y, w, h;
} MatchRect;

static struct {
    int page;
    int focus[MATCH_PAGE_COUNT];
    int scroll[MATCH_PAGE_COUNT];
    int hover;
    int cursorX, cursorY;
    int starting;
} s_match;

static const char *const s_matchItems[MATCH_ITEM_COUNT] = { "Mode", "Map", "Game Rules", "Bots", "Start Match" };
static const char *const s_matchSettings[MATCH_SETTING_COUNT] = { "Bot Count", "Difficulty" };
/* Every count to 16, then every fourth to the rooms' 32. */
static const int s_matchBotCounts[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 20, 24, 28, 32 };
static const char *const s_matchDifficulties[] = { "Easy", "Normal", "Hard" };
static const char *const s_matchDifficultyNotes[] = {
    "Easy bots react later, turn slower and aim less accurately.",
    "Normal bots balance their reactions, turning and accuracy.",
    "Hard bots react sooner, turn faster and aim more accurately.",
};
#define MATCH_BOT_COUNTS (int)(sizeof(s_matchBotCounts) / sizeof(s_matchBotCounts[0]))
#define MATCH_DIFFICULTIES (int)(sizeof(s_matchDifficulties) / sizeof(s_matchDifficulties[0]))

static const char *const s_matchRules[MATCH_RULE_COUNT] = { "Time Limit", "Score Limit" };
/* -1 in either for the mode's own. */
static const char *const s_matchRuleDvars[MATCH_RULE_COUNT] = { "ui_matchTimeLimit", "ui_matchScoreLimit" };
/* Minutes, and each mode's score, 0 for none: what rooms.py and the gateway
 * accept. A mode starts from what its room plays without rules: the retail
 * limits, but server.cfg's for Team Deathmatch and none for the retail 300
 * captures. */
static const int s_matchTimeLimits[] = { 0, 5, 10, 15, 20, 30, 45, 60 };
#define MATCH_TIME_LIMITS (int)(sizeof(s_matchTimeLimits) / sizeof(s_matchTimeLimits[0]))
#define MATCH_SCORE_LIMITS 8
static const struct {
    const char *gameType;
    int timeLimit, scoreLimit;
    int scoreLimits[MATCH_SCORE_LIMITS];
    const char *unit, *units, *note;
} s_matchRuleModes[] = {
    { "dm", 30, 100, { 0, 10, 20, 30, 50, 75, 100, 150 }, "Kill", "Kills",
      "The first player to make this many kills wins." },
    { "tdm", 15, 100, { 0, 25, 50, 75, 100, 150, 200, 300 }, "Kill", "Kills",
      "The first team to make this many kills wins." },
    { "ctf", 30, 0, { 0, 1, 3, 5, 7, 10, 15, 20 }, "Capture", "Captures",
      "The first team to capture the enemy flag this many times wins." },
    { "hq", 30, 300, { 0, 60, 120, 180, 300, 450, 600, 900 }, "Point", "Points",
      "Teams score a point for every second they hold the radio and 10 for destroying the enemy's. The first to this "
      "many wins." },
    { "sd", 0, 10, { 0, 3, 4, 5, 6, 7, 10, 15 }, "Round", "Rounds",
      "The first team to win this many rounds wins the match." },
};
#define MATCH_RULE_MODES (int)(sizeof(s_matchRuleModes) / sizeof(s_matchRuleModes[0]))
#define MATCH_TIME_NOTE "The match ends when the time runs out, and whoever leads then wins."

/* What the retail strings say of each mode: the serverinfo menus' objective
 * for Deathmatch and Team Deathmatch, the menus' description for the rest. */
static const struct {
    const char *gameType;
    const char *reference;
    const char *text;
} s_matchModeNotes[] = {
    { "dm", "MP_GAIN_POINTS_BY_ELIMINATING_NOSCORE", "Gain points by eliminating other players." },
    { "tdm", "MP_GAIN_POINTS_BY_ELIMINATING1_NOSCORE", "Gain points by eliminating enemy players." },
    { "ctf", "MENU_CTF_DESC",
      "Get the enemy flag while defending your flag.  Touching your flag while holding the enemy flag is a capture." },
    { "hq", "MENU_HQ_DESC",
      "Teams race to set up a radio and defend it for points.  If the enemy team sets up a radio, you must destroy it." },
    { "sd", "MENU_SD_DESC",
      "The Allies try to destroy Axis held objectives by planting explosives.  Axis must defuse the explosives if "
      "they are planted." },
};

static const struct {
    const char *map;
    const char *text;
} s_matchMapNotes[] = {
    { "mp_carentan", "Normandy town between the Utah and Omaha beachheads, fought over by American and German troops "
                     "through its streets and ruins." },
    { "mp_dawnville", "Normandy village around a church and its walled churchyard, fought over by American and German "
                      "troops." },
    { "mp_toujane", "Hillside town in southern Tunisia, fought over by British and Afrika Korps troops across its flat "
                    "rooftops and narrow alleys." },
};

static const vec4_t s_matchShade = { 0.0f, 0.0f, 0.0f, 0.55f };
static const vec4_t s_matchNote = { 0.70f, 0.70f, 0.68f, 1.0f };
static const vec4_t s_matchPreview = { 0.776f, 0.776f, 0.769f, 0.32f };
static const vec4_t s_matchBox = { 0.0f, 0.0f, 0.0f, 0.3f };
static const vec4_t s_matchBoxFocus = { 0.0f, 0.0f, 0.0f, 0.85f };
static const vec4_t s_matchBand = { 0.13f, 0.125f, 0.10f, 0.7f };
static const vec4_t s_matchEdge = { 1.0f, 1.0f, 1.0f, 0.28f };
static const vec4_t s_matchFocusEdge = { 0.80f, 0.72f, 0.47f, 0.55f };
static const vec4_t s_matchRowFocus = { 0.0f, 0.0f, 0.0f, 0.35f };

static int Match_IsMenu(const menuDef_t *menu)
{
    return menu && menu->window.name && !I_stricmp(menu->window.name, MATCH_MENU);
}

/* createserver and the createserver_ menus its onOpen opens. */
static int Match_IsFamily(const menuDef_t *menu)
{
    return menu && menu->window.name && !I_strnicmp(menu->window.name, MATCH_MENU, strlen(MATCH_MENU));
}

static int Match_Up(const displayContextDef_t *dc)
{
    int i;

    for (i = 0; i < dc->openMenuCount; i++) {
        const menuDef_t *menu = dc->menuStack[i];

        if (Match_IsMenu(menu) && (menu->window.dynamicFlags[0] & MATCH_WINDOW_VISIBLE))
            return 1;
    }
    return 0;
}

int UI_MatchMenu_Covers(displayContextDef_t *dc, const menuDef_t *menu)
{
    return Match_IsFamily(menu) && Match_Up(dc);
}

static void Match_Upper(const char *text, char *out, int size)
{
    int i;

    for (i = 0; text[i] && i < size - 1; i++)
        out[i] = (char)(text[i] >= 'a' && text[i] <= 'z' ? text[i] - 32 : text[i]);
    out[i] = '\0';
}

static const char *Match_Translate(const char *reference, const char *fallback)
{
    const char *text = SEH_StringEd_GetString(reference);

    return text && text[0] ? text : fallback;
}

static int Match_Mode(void)
{
    int mode = Dvar_GetInt("ui_netGametype");

    return mode >= 0 && mode < sharedUiInfo.numGameTypes ? mode : 0;
}

/* The retail Game Type control's name for the mode. */
static const char *Match_ModeName(int mode)
{
    const char *name = sharedUiInfo.gameTypes[mode].gameTypeName;

    return UI_SafeTranslateString(name && name[0] ? name : "EXE_ALL");
}

static const char *Match_ModeNote(int mode)
{
    int i;

    for (i = 0; i < (int)(sizeof(s_matchModeNotes) / sizeof(s_matchModeNotes[0])); i++) {
        if (!I_stricmp(sharedUiInfo.gameTypes[mode].gameType, s_matchModeNotes[i].gameType))
            return Match_Translate(s_matchModeNotes[i].reference, s_matchModeNotes[i].text);
    }
    return "";
}

static const char *Match_MapNote(int map)
{
    int i;

    for (i = 0; i < (int)(sizeof(s_matchMapNotes) / sizeof(s_matchMapNotes[0])); i++) {
        if (!I_stricmp(sharedUiInfo.mapList[map].mapLoadName, s_matchMapNotes[i].map))
            return s_matchMapNotes[i].text;
    }
    return "";
}

/* The arenas played in the mode, in their order; how many. */
static int Match_Maps(int *maps)
{
    int mode = Match_Mode(), count = 0, i;

    for (i = 0; i < sharedUiInfo.mapCount; i++) {
        if (((unsigned int)sharedUiInfo.mapList[i].typeBits >> mode) & 1u)
            maps[count++] = i;
    }
    return count;
}

/* The chosen arena while the mode is played there, else the mode's first;
 * -1 for none. */
static int Match_Map(void)
{
    int maps[128], count = Match_Maps(maps), map = Dvar_GetInt("ui_currentNetMap"), i;

    for (i = 0; i < count; i++) {
        if (maps[i] == map)
            return map;
    }
    return count ? maps[0] : -1;
}

/* As the retail Game Type control: the mode, and a map it is played on. */
static void Match_SelectMode(int mode)
{
    int map;

    if (mode < 0 || mode >= sharedUiInfo.numGameTypes)
        return;
    /* Another mode's score counts something else: it starts from its own. */
    if (mode != Dvar_GetInt("ui_netGametype")) {
        Dvar_SetIntByName("ui_matchTimeLimit", -1);
        Dvar_SetIntByName("ui_matchScoreLimit", -1);
    }
    Dvar_SetIntByName("ui_netGametype", mode);
    Dvar_SetStringByName("ui_netGametypeName", sharedUiInfo.gameTypes[mode].gameType);
    map = Match_Map();
    if (map >= 0 && map != Dvar_GetInt("ui_currentNetMap"))
        Dvar_SetIntByName("ui_currentNetMap", map);
}

static int Match_Difficulty(void)
{
    int difficulty = Dvar_GetInt("ui_botDifficulty");

    return difficulty >= 0 && difficulty < MATCH_DIFFICULTIES ? difficulty : 1;
}

/* The next count up or down, wrapping; a count between two goes to them. */
static void Match_StepBots(int delta)
{
    int count = Dvar_GetInt("ui_botCount"), i;

    for (i = 0; i < MATCH_BOT_COUNTS && s_matchBotCounts[i] < count; i++)
        ;
    if (delta > 0)
        i += i < MATCH_BOT_COUNTS && s_matchBotCounts[i] == count;
    else
        i--;
    Dvar_SetIntByName("ui_botCount", s_matchBotCounts[(i + MATCH_BOT_COUNTS) % MATCH_BOT_COUNTS]);
}

/* The gametype's rules; -1 for one without. */
static int Match_RuleMode(const char *gameType)
{
    int i;

    for (i = 0; gameType && i < MATCH_RULE_MODES; i++) {
        if (!I_stricmp(gameType, s_matchRuleModes[i].gameType))
            return i;
    }
    return -1;
}

static int Match_ModeRules(void)
{
    return Match_RuleMode(sharedUiInfo.gameTypes[Match_Mode()].gameType);
}

static const int *Match_RuleValues(int ruleMode, int rule, int *count)
{
    *count = rule == MATCH_RULE_TIME ? MATCH_TIME_LIMITS : MATCH_SCORE_LIMITS;
    return rule == MATCH_RULE_TIME ? s_matchTimeLimits : s_matchRuleModes[ruleMode].scoreLimits;
}

/* The chosen value while the mode offers it, else the mode's own; -1 for a
 * mode without rules. */
static int Match_Rule(int ruleMode, int rule)
{
    const int value = Dvar_GetInt(s_matchRuleDvars[rule]);
    const int *values;
    int count, i;

    if (ruleMode < 0)
        return -1;
    values = Match_RuleValues(ruleMode, rule, &count);
    for (i = 0; i < count; i++) {
        if (values[i] == value)
            return value;
    }
    return rule == MATCH_RULE_TIME ? s_matchRuleModes[ruleMode].timeLimit : s_matchRuleModes[ruleMode].scoreLimit;
}

/* The next value up or down, wrapping. */
static void Match_StepRule(int rule, int delta)
{
    const int ruleMode = Match_ModeRules(), value = Match_Rule(ruleMode, rule);
    const int *values;
    int count, i;

    if (ruleMode < 0)
        return;
    values = Match_RuleValues(ruleMode, rule, &count);
    for (i = 0; i < count && values[i] != value; i++)
        ;
    Dvar_SetIntByName(s_matchRuleDvars[rule], values[(i + delta + count) % count]);
}

static void Match_Step(int page, int setting, int delta)
{
    if (page == MATCH_PAGE_RULES)
        Match_StepRule(setting, delta);
    else if (setting == MATCH_SETTING_BOTS)
        Match_StepBots(delta);
    else
        Dvar_SetIntByName("ui_botDifficulty", (Match_Difficulty() + delta + MATCH_DIFFICULTIES) % MATCH_DIFFICULTIES);
    UI_PlayLocalSoundAliasByName("mouse_click");
}

static void Match_SettingValue(int setting, char *text, int size)
{
    int count = Dvar_GetInt("ui_botCount");

    if (setting == MATCH_SETTING_DIFFICULTY)
        snprintf(text, (size_t)size, "%s", s_matchDifficulties[Match_Difficulty()]);
    else if (count > 0)
        snprintf(text, (size_t)size, "%d", count);
    else
        snprintf(text, (size_t)size, "None");
}

static void Match_RuleValue(int rule, char *text, int size)
{
    const int ruleMode = Match_ModeRules(), value = Match_Rule(ruleMode, rule);

    if (value < 0)
        snprintf(text, (size_t)size, "Default");
    else if (!value)
        snprintf(text, (size_t)size, "Unlimited");
    else if (rule == MATCH_RULE_TIME)
        snprintf(text, (size_t)size, "%d Minutes", value);
    else
        snprintf(text, (size_t)size, "%d %s", value,
                 value == 1 ? s_matchRuleModes[ruleMode].unit : s_matchRuleModes[ruleMode].units);
}

void UI_MatchMenu_Rules(const char *gameType, int *timeLimit, int *scoreLimit)
{
    const int ruleMode = Match_RuleMode(gameType);

    *timeLimit = Match_Rule(ruleMode, MATCH_RULE_TIME);
    *scoreLimit = Match_Rule(ruleMode, MATCH_RULE_SCORE);
}

void UI_MatchMenu_RoomName(char *name, int size)
{
    const char *gametag = Dvar_GetString("name");
    char tag[64];
    int length = 0, i;

    I_strncpyz(tag, gametag ? gametag : "", sizeof(tag));
    I_CleanStr(tag);
    for (i = 0; tag[i] && length < size - 1; i++) {
        char c = tag[i];

        if (((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || strchr(" _|.-", c)) &&
            (c != ' ' || length))
            name[length++] = c;
    }
    if (length > MATCH_ROOM_NAME_MAX)
        length = MATCH_ROOM_NAME_MAX;
    while (length && name[length - 1] == ' ')
        length--;
    name[length] = '\0';
}

void UI_MatchMenu_Opened(displayContextDef_t *dc, menuDef_t *menu)
{
    if (!Match_IsMenu(menu))
        return;
    memset(&s_match, 0, sizeof(s_match));
    s_match.hover = -1;
    s_match.cursorX = dc->cursorx;
    s_match.cursorY = dc->cursory;
    Match_SelectMode(Match_Mode());
}

static int Match_Count(int page)
{
    int maps[128];

    switch (page) {
    case MATCH_PAGE_MODE:
        return sharedUiInfo.numGameTypes;
    case MATCH_PAGE_MAP:
        return Match_Maps(maps);
    case MATCH_PAGE_BOTS:
        return MATCH_SETTING_COUNT;
    case MATCH_PAGE_RULES:
        return MATCH_RULE_COUNT;
    default:
        return MATCH_ITEM_COUNT;
    }
}

static void Match_Focus(int index)
{
    if (index == s_match.focus[s_match.page])
        return;
    s_match.focus[s_match.page] = index;
    UI_PlayLocalSoundAliasByName("mouse_over");
}

static void Match_Move(int delta)
{
    int count = Match_Count(s_match.page);

    if (count > 0)
        Match_Focus((s_match.focus[s_match.page] + delta + count) % count);
}

/* A page goes back to the match page, still focused on the item that opened
 * it; the match page closes createserver, whose onClose opens main_text. */
static void Match_Back(displayContextDef_t *dc)
{
    if (s_match.page == MATCH_PAGE_MATCH)
        Menus_CloseByName(dc, MATCH_MENU);
    else
        s_match.page = MATCH_PAGE_MATCH;
}

/* StartServer reads the map from ui_currentNetMap. */
static void Match_Start(void)
{
    const char *script = "StartServer";
    int map = Match_Map();

    if (s_match.starting || map < 0)
        return;
    if (map != Dvar_GetInt("ui_currentNetMap"))
        Dvar_SetIntByName("ui_currentNetMap", map);
    s_match.starting = 1;
    UI_RunMenuScript(&script);
}

/* Opens the focused item's page, chooses the focused mode or map, or steps
 * the focused setting by step. */
static void Match_Activate(int step)
{
    int focus = s_match.focus[s_match.page], maps[128], count, map, i;

    if (s_match.page == MATCH_PAGE_BOTS || s_match.page == MATCH_PAGE_RULES) {
        Match_Step(s_match.page, focus, step);
        return;
    }
    UI_PlayLocalSoundAliasByName("mouse_click");
    switch (s_match.page) {
    case MATCH_PAGE_MODE:
        Match_SelectMode(focus);
        s_match.page = MATCH_PAGE_MATCH;
        break;
    case MATCH_PAGE_MAP:
        count = Match_Maps(maps);
        if (focus >= 0 && focus < count)
            Dvar_SetIntByName("ui_currentNetMap", maps[focus]);
        s_match.page = MATCH_PAGE_MATCH;
        break;
    default:
        if (focus == MATCH_ITEM_START) {
            Match_Start();
        } else if (focus == MATCH_ITEM_MODE) {
            s_match.page = MATCH_PAGE_MODE;
            s_match.focus[MATCH_PAGE_MODE] = Match_Mode();
        } else if (focus == MATCH_ITEM_MAP) {
            count = Match_Maps(maps);
            map = Match_Map();
            for (i = 0; i < count && maps[i] != map; i++)
                ;
            s_match.page = MATCH_PAGE_MAP;
            s_match.focus[MATCH_PAGE_MAP] = i < count ? i : 0;
        } else if (focus == MATCH_ITEM_RULES) {
            s_match.page = MATCH_PAGE_RULES;
            s_match.focus[MATCH_PAGE_RULES] = MATCH_RULE_TIME;
        } else {
            s_match.page = MATCH_PAGE_BOTS;
            s_match.focus[MATCH_PAGE_BOTS] = MATCH_SETTING_BOTS;
        }
        break;
    }
}

/* Whether the pointer is left of the point x units from the left edge. */
static int Match_PointerLeftOf(const displayContextDef_t *dc, float x)
{
    float cx = (float)dc->cursorx;

    CalcScreenX(&x, MATCH_ALIGN_LEFT);
    CalcScreenX(&cx, MATCH_ALIGN_FULLSCREEN);
    return cx < x;
}

/* A click left of a setting's value, past its name, steps it back as its
 * left arrow does; anywhere else on its row steps it on. */
static int Match_ClickStep(const displayContextDef_t *dc)
{
    const float center = MATCH_TEXT_X + MATCH_VALUE_OFFSET;

    return Match_PointerLeftOf(dc, center) && !Match_PointerLeftOf(dc, center - 1.5f * MATCH_ARROW_OFFSET) ? -1 : 1;
}

int UI_MatchMenu_KeyEvent(displayContextDef_t *dc, menuDef_t *menu, int key, int down)
{
    int page;

    if (!UI_MatchMenu_Covers(dc, menu))
        return 0;
    if (!down || (key & MATCH_KEY_CHAR))
        return 1;
    page = s_match.page;
    switch (key) {
    case MATCH_KEY_ESCAPE:
    case MATCH_KEY_MOUSE2:
        Match_Back(dc);
        break;
    case MATCH_KEY_ENTER:
    case MATCH_KEY_KP_ENTER:
        Match_Activate(1);
        break;
    case MATCH_KEY_MOUSE1:
        if (s_match.hover >= 0) {
            Match_Focus(s_match.hover);
            Match_Activate(Match_ClickStep(dc));
        }
        break;
    case MATCH_KEY_UP:
    case MATCH_KEY_WHEEL_UP:
        Match_Move(-1);
        break;
    case MATCH_KEY_DOWN:
    case MATCH_KEY_WHEEL_DOWN:
    case MATCH_KEY_TAB:
        Match_Move(1);
        break;
    case MATCH_KEY_LEFT:
    case MATCH_KEY_RIGHT:
        if (page == MATCH_PAGE_BOTS || page == MATCH_PAGE_RULES)
            Match_Step(page, s_match.focus[page], key == MATCH_KEY_RIGHT ? 1 : -1);
        break;
    default:
        break;
    }
    /* The last frame's hover belongs to the page it painted. */
    if (s_match.page != page)
        s_match.hover = -1;
    return 1;
}

static MatchRect Match_Rect(float x, float y, float w, float h)
{
    MatchRect rect;

    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    return rect;
}

static int Match_Hover(const displayContextDef_t *dc, const MatchRect *rect)
{
    float x = rect->x, y = rect->y, w = rect->w, h = rect->h;
    float cx = (float)dc->cursorx, cy = (float)dc->cursory;

    CalcScreenPlacement(&x, &y, &w, &h, MATCH_ALIGN_LEFT, MATCH_ALIGN_TOP);
    CalcScreenX(&cx, MATCH_ALIGN_FULLSCREEN);
    CalcScreenY(&cy, MATCH_ALIGN_FULLSCREEN);
    return cx >= x && cx < x + w && cy >= y && cy < y + h;
}

static void Match_Fill(const MatchRect *rect, const vec_t *color)
{
    CL_DrawStretchPic(rect->x, rect->y, rect->w, rect->h, MATCH_ALIGN_LEFT, MATCH_ALIGN_TOP, 0.0f, 0.0f, 1.0f, 1.0f,
                      color, sharedUiInfo.assets.whiteMaterial);
}

/* Lines along the top, the bottom and the right, and the left unless the
 * rectangle runs off the screen's edge; they do not overlap. */
static void Match_Frame(const MatchRect *rect, const vec_t *color, int left)
{
    MatchRect edge = *rect;

    edge.h = MATCH_LINE;
    Match_Fill(&edge, color);
    edge.y = rect->y + rect->h - MATCH_LINE;
    Match_Fill(&edge, color);
    edge.y = rect->y + MATCH_LINE;
    edge.h = rect->h - 2.0f * MATCH_LINE;
    edge.w = MATCH_LINE;
    edge.x = rect->x + rect->w - MATCH_LINE;
    Match_Fill(&edge, color);
    if (left) {
        edge.x = rect->x;
        Match_Fill(&edge, color);
    }
}

static void Match_Text(const char *text, int bold, float x, float baseline, float cap, float tracking,
                       const vec_t *color)
{
    UI_WWIIMenu_Text(text, bold, x, baseline, MATCH_ALIGN_LEFT, MATCH_ALIGN_TOP, cap, tracking, color);
}

static void Match_Caps(const char *text, int bold, float x, float baseline, float cap, float tracking,
                       const vec_t *color)
{
    char upper[128];

    Match_Upper(text, upper, sizeof(upper));
    Match_Text(upper, bold, x, baseline, cap, tracking, color);
}

/* Words in lines no wider than width, leading apart, at most lines of them. */
static void Match_Wrapped(const char *text, float x, float baseline, float width, float cap, float leading, int lines,
                          const vec_t *color)
{
    char line[256], trial[256];

    line[0] = '\0';
    while (lines > 0) {
        int word = 0;

        while (*text == ' ')
            text++;
        while (text[word] && text[word] != ' ')
            word++;
        if (!word)
            break;
        snprintf(trial, sizeof(trial), "%s%s%.*s", line, line[0] ? " " : "", word, text);
        if (line[0] && UI_WWIIMenu_Width(trial, 0, cap, 0.0f) > width) {
            Match_Text(line, 0, x, baseline, cap, 0.0f, color);
            baseline += leading;
            lines--;
            line[0] = '\0';
            continue;
        }
        memcpy(line, trial, sizeof(line));
        text += word;
    }
    if (line[0] && lines > 0)
        Match_Text(line, 0, x, baseline, cap, 0.0f, color);
}

/* A solid arrow w by h around x, y, pointing right for direction 1. */
static void Match_Arrow(float x, float y, int direction, const vec_t *color)
{
    static const unsigned short indices[3] = { 0, 1, 2 };
    HudVertex2D verts[3];
    float left = x - 0.5f * MATCH_ARROW_W, top = y - 0.5f * MATCH_ARROW_H;
    float w = MATCH_ARROW_W, h = MATCH_ARROW_H;
    int i;

    CalcScreenPlacement(&left, &top, &w, &h, MATCH_ALIGN_LEFT, MATCH_ALIGN_TOP);
    memset(verts, 0, sizeof(verts));
    if (direction > 0) {
        verts[0].xy[0] = verts[1].xy[0] = left;
        verts[2].xy[0] = left + w;
        verts[0].xy[1] = top;
        verts[1].xy[1] = top + h;
    } else {
        verts[0].xy[0] = verts[2].xy[0] = left + w;
        verts[1].xy[0] = left;
        verts[0].xy[1] = top;
        verts[2].xy[1] = top + h;
    }
    verts[direction > 0 ? 2 : 1].xy[1] = top + 0.5f * h;
    for (i = 0; i < 3; i++)
        memcpy(verts[i].rgba, color, sizeof(verts[i].rgba));
    R_AddCmdDrawTriangles2D(verts, 3, indices, 3, sharedUiInfo.assets.whiteMaterial);
}

static void Match_Title(const char *title, const char *note)
{
    Match_Text(title, 1, MATCH_PAGE_X, MATCH_TITLE_BASELINE, MATCH_TITLE_CAP, MATCH_TITLE_TRACKING,
               UI_WWIIMenu_Color(WWII_COLOR_GOLD));
    Match_Text(note, 0, MATCH_PAGE_X, MATCH_NOTE_BASELINE, MATCH_NOTE_CAP, 0.0f, s_matchNote);
}

/* The match page's items down the column: its two boxes, the rows for game
 * rules and bots a row apart, and the box to start, each the gap after the
 * last. */
static float Match_ItemTop(int item)
{
    const float rows = MATCH_BOX_TOP + 2.0f * (MATCH_BOX_H + MATCH_GAP);

    if (item < MATCH_ITEM_RULES)
        return MATCH_BOX_TOP + (float)item * (MATCH_BOX_H + MATCH_GAP);
    if (item < MATCH_ITEM_START)
        return rows + (float)(item - MATCH_ITEM_RULES) * MATCH_ROW_SPACING;
    return rows + (float)(MATCH_ITEM_BOTS - MATCH_ITEM_RULES) * MATCH_ROW_SPACING + MATCH_ROW_H + MATCH_GAP;
}

static void Match_PaintBox(displayContextDef_t *dc, int item, const char *value, int focused)
{
    const float top = Match_ItemTop(item);
    const vec_t *gold = UI_WWIIMenu_Color(WWII_COLOR_GOLD);
    const float baseline = top + MATCH_BOX_NAME_BASELINE;
    MatchRect box = Match_Rect(MATCH_BOX_X, top, MATCH_BOX_W, MATCH_BOX_H);
    float nameCap = focused ? MATCH_BOX_FOCUS_NAME_CAP : MATCH_BOX_NAME_CAP;

    if (Match_Hover(dc, &box))
        s_match.hover = item;
    if (focused) {
        MatchRect band = box, edge = box;

        band.h = MATCH_BOX_BAND * box.h;
        edge.w = MATCH_BOX_EDGE;
        Match_Fill(&box, s_matchBoxFocus);
        Match_Fill(&band, s_matchBand);
        Match_Frame(&box, s_matchFocusEdge, 1);
        Match_Fill(&edge, gold);
        Match_Text("\xbb", 0, MATCH_BOX_X + MATCH_CHEVRON_X, baseline, nameCap, 0.0f, gold);
    } else {
        Match_Fill(&box, s_matchBox);
        Match_Frame(&box, s_matchEdge, 1);
    }
    Match_Caps(s_matchItems[item], 1, MATCH_TEXT_X, baseline, nameCap, 0.0f,
               focused ? gold : UI_WWIIMenu_Color(WWII_COLOR_TITLE));
    Match_Caps(value, 0, MATCH_TEXT_X, top + MATCH_BOX_VALUE_BASELINE,
               focused ? MATCH_BOX_FOCUS_VALUE_CAP : MATCH_BOX_VALUE_CAP, 0.0f, s_matchNote);
}

/* A row ending at right; index is what the pointer over it focuses. */
static void Match_PaintRow(displayContextDef_t *dc, int index, const char *text, int caps, float baseline, float cap,
                           float focusCap, float right, int focused)
{
    MatchRect row = Match_Rect(0.0f, baseline - MATCH_ROW_ABOVE, right, MATCH_ROW_H);
    float x = MATCH_TEXT_X + (focused ? MATCH_FOCUS_INDENT : 0.0f);
    const vec_t *color = UI_WWIIMenu_Color(focused ? WWII_COLOR_GOLD : WWII_COLOR_IDLE);

    if (Match_Hover(dc, &row))
        s_match.hover = index;
    if (focused) {
        Match_Fill(&row, s_matchRowFocus);
        Match_Frame(&row, s_matchFocusEdge, 0);
    }
    if (caps)
        Match_Caps(text, focused, x, baseline, focused ? focusCap : cap, 0.0f, color);
    else
        Match_Text(text, focused, x, baseline, focused ? focusCap : cap, 0.0f, color);
}

/* A setting's value, centred to the right of its name's pen at x. */
static void Match_PaintValue(const char *value, float x, float baseline, int bold, const vec_t *color)
{
    Match_Text(value, bold, x + MATCH_VALUE_OFFSET - 0.5f * UI_WWIIMenu_Width(value, bold, MATCH_VALUE_CAP, 0.0f),
               baseline, MATCH_VALUE_CAP, 0.0f, color);
}

static void Match_PaintPair(int row, const char *name, const char *value)
{
    const float x = MATCH_TEXT_X + MATCH_PREVIEW_SHIFT, baseline = MATCH_LIST_TOP + (float)row * MATCH_ROW_SPACING;

    Match_Text(name, 0, x, baseline, MATCH_SETTING_CAP, 0.0f, s_matchPreview);
    Match_PaintValue(value, x, baseline, 0, s_matchPreview);
}

/* Beside the match page's column, the focused item's page, dimmed: the
 * modes, the maps, the rules, the bots' settings, or what Start Match
 * creates. */
static void Match_PaintPreview(int item)
{
    const float x = MATCH_TEXT_X + MATCH_PREVIEW_SHIFT;
    char value[64], name[64];
    int maps[128], count, i;

    switch (item) {
    case MATCH_ITEM_MODE:
        for (i = 0; i < sharedUiInfo.numGameTypes && i < MATCH_LIST_VISIBLE; i++)
            Match_Caps(Match_ModeName(i), 0, x, MATCH_LIST_TOP + (float)i * MATCH_ROW_SPACING, MATCH_LIST_CAP, 0.0f,
                       s_matchPreview);
        break;
    case MATCH_ITEM_MAP:
        count = Match_Maps(maps);
        for (i = 0; i < count && i < MATCH_LIST_VISIBLE; i++)
            Match_Caps(sharedUiInfo.mapList[maps[i]].mapName, 0, x, MATCH_LIST_TOP + (float)i * MATCH_ROW_SPACING,
                       MATCH_LIST_CAP, 0.0f, s_matchPreview);
        break;
    case MATCH_ITEM_RULES:
        for (i = 0; i < MATCH_RULE_COUNT; i++) {
            Match_RuleValue(i, value, sizeof(value));
            Match_PaintPair(i, s_matchRules[i], value);
        }
        break;
    case MATCH_ITEM_BOTS:
        for (i = 0; i < MATCH_SETTING_COUNT; i++) {
            Match_SettingValue(i, value, sizeof(value));
            Match_PaintPair(i, s_matchSettings[i], value);
        }
        break;
    default:
        Match_PaintPair(0, "Mode", Match_ModeName(Match_Mode()));
        i = Match_Map();
        Match_PaintPair(1, "Map", i >= 0 ? sharedUiInfo.mapList[i].mapName : "");
        for (i = 0; i < MATCH_RULE_COUNT; i++) {
            Match_RuleValue(i, value, sizeof(value));
            Match_PaintPair(2 + i, s_matchRules[i], value);
        }
        if (Dvar_GetInt("ui_botCount") > 0)
            snprintf(value, sizeof(value), "%d, %s", Dvar_GetInt("ui_botCount"),
                     s_matchDifficulties[Match_Difficulty()]);
        else
            snprintf(value, sizeof(value), "None");
        Match_PaintPair(4, "Bots", value);
        UI_MatchMenu_RoomName(name, sizeof(name));
        if (name[0])
            Match_PaintPair(5, "Server Name", name);
        break;
    }
}

static void Match_PaintMatch(displayContextDef_t *dc)
{
    const int focus = s_match.focus[MATCH_PAGE_MATCH];
    const int map = Match_Map();
    int item;

    Match_Title("MATCH", "Customize the game you want to play");
    Match_PaintBox(dc, MATCH_ITEM_MODE, Match_ModeName(Match_Mode()), focus == MATCH_ITEM_MODE);
    Match_PaintBox(dc, MATCH_ITEM_MAP, map >= 0 ? sharedUiInfo.mapList[map].mapName : "", focus == MATCH_ITEM_MAP);
    for (item = MATCH_ITEM_RULES; item <= MATCH_ITEM_BOTS; item++)
        Match_PaintRow(dc, item, s_matchItems[item], 1, Match_ItemTop(item) + MATCH_ROW_ABOVE, MATCH_ROW_CAP,
                       MATCH_ROW_CAP, MATCH_LIST_RIGHT, focus == item);
    Match_PaintBox(dc, MATCH_ITEM_START,
                   s_match.starting ? "Starting your server..." : "Create your server and join it",
                   focus == MATCH_ITEM_START);
    Match_PaintPreview(focus);
}

/* The page's first row shown, keeping the focus among the visible ones. */
static int Match_Scroll(int page, int count)
{
    int *scroll = &s_match.scroll[page], focus = s_match.focus[page];

    if (focus < *scroll)
        *scroll = focus;
    if (focus >= *scroll + MATCH_LIST_VISIBLE)
        *scroll = focus - MATCH_LIST_VISIBLE + 1;
    if (*scroll > count - MATCH_LIST_VISIBLE)
        *scroll = count - MATCH_LIST_VISIBLE;
    if (*scroll < 0)
        *scroll = 0;
    return *scroll;
}

static void Match_PaintModes(displayContextDef_t *dc)
{
    const int count = sharedUiInfo.numGameTypes, focus = s_match.focus[MATCH_PAGE_MODE];
    const int first = Match_Scroll(MATCH_PAGE_MODE, count);
    int mode;

    Match_Title("GAME MODE SELECT", "Choose the type of gameplay");
    for (mode = first; mode < count && mode < first + MATCH_LIST_VISIBLE; mode++)
        Match_PaintRow(dc, mode, Match_ModeName(mode), 1, MATCH_LIST_TOP + (float)(mode - first) * MATCH_ROW_SPACING,
                       MATCH_LIST_CAP, MATCH_LIST_FOCUS_CAP, MATCH_LIST_RIGHT, mode == focus);
    if (focus < 0 || focus >= count)
        return;
    Match_Caps(Match_ModeName(focus), 1, MATCH_PANEL_X, MATCH_MODE_TITLE_BASELINE, MATCH_PANEL_TITLE_CAP,
               MATCH_PANEL_TITLE_TRACKING, UI_WWIIMenu_Color(WWII_COLOR_TITLE));
    Match_Wrapped(Match_ModeNote(focus), MATCH_PANEL_X, MATCH_MODE_NOTE_BASELINE, MATCH_SHOT_W, MATCH_PANEL_NOTE_CAP,
                  MATCH_PANEL_LEADING, MATCH_PANEL_NOTE_LINES, s_matchNote);
}

static void Match_PaintMaps(displayContextDef_t *dc)
{
    int maps[128];
    const int count = Match_Maps(maps), focus = s_match.focus[MATCH_PAGE_MAP];
    const int first = Match_Scroll(MATCH_PAGE_MAP, count);
    MaterialHandle shot;
    int i;

    Match_Title("MAP SELECT", "Pick where your match will take place");
    for (i = first; i < count && i < first + MATCH_LIST_VISIBLE; i++)
        Match_PaintRow(dc, i, sharedUiInfo.mapList[maps[i]].mapName, 1,
                       MATCH_LIST_TOP + (float)(i - first) * MATCH_ROW_SPACING, MATCH_LIST_CAP, MATCH_LIST_FOCUS_CAP,
                       MATCH_LIST_RIGHT, i == focus);
    if (focus < 0 || focus >= count)
        return;
    shot = sharedUiInfo.mapList[maps[focus]].levelShot;
    if (shot)
        CL_DrawStretchPic(MATCH_SHOT_X, MATCH_SHOT_TOP, MATCH_SHOT_W, MATCH_SHOT_H, MATCH_ALIGN_LEFT, MATCH_ALIGN_TOP,
                          0.0f, MATCH_SHOT_CROP, 1.0f, 1.0f - MATCH_SHOT_CROP, UI_WWIIMenu_Color(WWII_COLOR_WHITE),
                          shot);
    Match_Caps(sharedUiInfo.mapList[maps[focus]].mapName, 1, MATCH_PANEL_X, MATCH_MAP_TITLE_BASELINE,
               MATCH_PANEL_TITLE_CAP, MATCH_PANEL_TITLE_TRACKING, UI_WWIIMenu_Color(WWII_COLOR_GOLD));
    Match_Wrapped(Match_MapNote(maps[focus]), MATCH_PANEL_X, MATCH_MAP_NOTE_BASELINE, MATCH_SHOT_W,
                  MATCH_PANEL_NOTE_CAP, MATCH_PANEL_LEADING, MATCH_PANEL_NOTE_LINES, s_matchNote);
}

/* Bot setup's or the rules' settings, the focused one's value Bold and white
 * between arrows; the panel names the focused one, and note explains it. */
static void Match_PaintSettings(displayContextDef_t *dc, int page, const char *const *names, int count,
                                const char *note)
{
    const int focus = s_match.focus[page];
    const float center = MATCH_TEXT_X + MATCH_VALUE_OFFSET;
    char value[32];
    int row;

    for (row = 0; row < count; row++) {
        const float baseline = MATCH_LIST_TOP + (float)row * MATCH_ROW_SPACING;
        const int focused = row == focus;

        Match_PaintRow(dc, row, names[row], 0, baseline, MATCH_SETTING_CAP, MATCH_SETTING_CAP,
                       MATCH_SETTING_RIGHT, focused);
        if (page == MATCH_PAGE_RULES)
            Match_RuleValue(row, value, sizeof(value));
        else
            Match_SettingValue(row, value, sizeof(value));
        Match_PaintValue(value, MATCH_TEXT_X, baseline, focused,
                         UI_WWIIMenu_Color(focused ? WWII_COLOR_WHITE : WWII_COLOR_IDLE));
        if (focused) {
            Match_Arrow(center - MATCH_ARROW_OFFSET, baseline - MATCH_ARROW_RISE, -1,
                        UI_WWIIMenu_Color(WWII_COLOR_TITLE));
            Match_Arrow(center + MATCH_ARROW_OFFSET, baseline - MATCH_ARROW_RISE, 1,
                        UI_WWIIMenu_Color(WWII_COLOR_TITLE));
        }
    }
    if (focus < 0 || focus >= count)
        return;
    Match_Caps(names[focus], 1, MATCH_BOTS_PANEL_X, MATCH_BOTS_TITLE_BASELINE, MATCH_PANEL_TITLE_CAP,
               MATCH_PANEL_TITLE_TRACKING, UI_WWIIMenu_Color(WWII_COLOR_GOLD));
    Match_Wrapped(note, MATCH_BOTS_PANEL_X, MATCH_BOTS_NOTE_BASELINE, MATCH_BOTS_NOTE_W, MATCH_PANEL_NOTE_CAP,
                  MATCH_PANEL_LEADING, MATCH_PANEL_NOTE_LINES, s_matchNote);
}

static void Match_PaintBots(displayContextDef_t *dc)
{
    Match_Title("BOT SETUP", "Set the number and difficulty of bots");
    Match_PaintSettings(dc, MATCH_PAGE_BOTS, s_matchSettings, MATCH_SETTING_COUNT,
                        s_match.focus[MATCH_PAGE_BOTS] == MATCH_SETTING_BOTS
                            ? "Bots join the match on their own. Each one takes a player slot."
                            : s_matchDifficultyNotes[Match_Difficulty()]);
}

static void Match_PaintRules(displayContextDef_t *dc)
{
    const int ruleMode = Match_ModeRules();

    Match_Title("GAME RULES", "Set how long the match lasts and the score that wins it");
    Match_PaintSettings(dc, MATCH_PAGE_RULES, s_matchRules, MATCH_RULE_COUNT,
                        s_match.focus[MATCH_PAGE_RULES] == MATCH_RULE_TIME ? MATCH_TIME_NOTE
                        : ruleMode >= 0                                    ? s_matchRuleModes[ruleMode].note
                                                                           : "");
}

void UI_MatchMenu_Paint(displayContextDef_t *dc, menuDef_t *menu)
{
    static const char *const keys[] = { "ENTER" };
    static const char *const labels[] = { "Select" };
    int moved;

    if (!Match_IsMenu(menu) || !Match_Up(dc))
        return;
    /* The text stands on a dark scene, as WWII's. */
    CL_DrawStretchPic(0.0f, 0.0f, 640.0f, 480.0f, MATCH_ALIGN_FULLSCREEN, MATCH_ALIGN_FULLSCREEN, 0.0f, 0.0f, 1.0f, 1.0f,
                      s_matchShade, sharedUiInfo.assets.whiteMaterial);
    moved = dc->cursorx != s_match.cursorX || dc->cursory != s_match.cursorY;
    s_match.cursorX = dc->cursorx;
    s_match.cursorY = dc->cursory;
    s_match.hover = -1;
    switch (s_match.page) {
    case MATCH_PAGE_MODE:
        Match_PaintModes(dc);
        break;
    case MATCH_PAGE_MAP:
        Match_PaintMaps(dc);
        break;
    case MATCH_PAGE_BOTS:
        Match_PaintBots(dc);
        break;
    case MATCH_PAGE_RULES:
        Match_PaintRules(dc);
        break;
    default:
        Match_PaintMatch(dc);
        break;
    }
    /* Pointer motion moves the focus to what it is over now; keys move it
     * otherwise. */
    if (moved && s_match.hover >= 0)
        Match_Focus(s_match.hover);
    UI_WWIIMenu_Prompts(keys, labels, 1, "ESC", "Back");
}
