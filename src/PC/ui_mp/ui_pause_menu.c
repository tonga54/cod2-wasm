#include "common_types.h"
#include "imports.h"
#include "cod2_feature_config.h"
#include "headers/PC/cgame_mp/cg_minimap.h"
#include "headers/PC/cgame_mp/cg_overview.h"
#include "headers/PC/cgame_mp/cg_scorepanel.h"
#include "headers/PC/ui_mp/ui_class_menu.h"
#include "headers/PC/ui_mp/ui_pause_menu.h"
#include "headers/PC/ui_mp/ui_wwii_menu.h"
#include <stdio.h>
#include <string.h>

extern sharedUiInfo_t sharedUiInfo;
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1,
                              float s2, float t2, const vec_t *color, MaterialHandle material);
extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);
extern void CalcScreenX(float *x, int horzAlign);
extern void CalcScreenY(float *y, int vertAlign);
extern const char *CL_GetConfigString(int index);
extern int Dvar_GetInt(const char *name);
extern Bool Dvar_GetBool(const char *name);
extern void Cbuf_ExecuteText(int when, const char *text);
extern char *va(const char *format, ...);
extern void Menus_CloseByName(displayContextDef_t *dc, const char *name);
extern qboolean Menus_OpenByName(displayContextDef_t *dc, const char *name);
extern int I_stricmp(const char *s0, const char *s1);
extern int UI_PlayLocalSoundAliasByName(const char *aliasname);
extern const char *UI_SafeTranslateString(const char *reference);
#if COD2_FEATURE_GAMEPAD
extern int CL_Gamepad_IsActiveDevice(void);
#endif

#define PAUSE_ALIGN_LEFT 1
#define PAUSE_ALIGN_RIGHT 3
#define PAUSE_ALIGN_TOP 1
#define PAUSE_ALIGN_BOTTOM 3
#define PAUSE_ALIGN_FULLSCREEN 4

#define PAUSE_KEY_TAB 0x09
#define PAUSE_KEY_ENTER 0x0d
#define PAUSE_KEY_ESCAPE 0x1b
#define PAUSE_KEY_UP 0x9a
#define PAUSE_KEY_DOWN 0x9b
#define PAUSE_KEY_LEFT 0x9c
#define PAUSE_KEY_RIGHT 0x9d
#define PAUSE_KEY_F1 0xa7
#define PAUSE_KEY_KP_ENTER 0xbf
#define PAUSE_KEY_MOUSE1 0xc8
#define PAUSE_KEY_MOUSE2 0xc9
#define PAUSE_KEY_WHEEL_DOWN 0xcd
#define PAUSE_KEY_WHEEL_UP 0xce
/* The controller's bumpers, which the menus pass on as themselves. */
#define PAUSE_KEY_LSHLDR 0xd3
#define PAUSE_KEY_RSHLDR 0xd4

/* Units from the screen's edges, measured on the user's captures of WWII's
 * pause menu at 1024 x 576, 1.2 px a unit; capitals' heights and baselines,
 * pens from where the ink starts. */
#define PAUSE_LINE 0.9f
#define PAUSE_WASH 0.42f
#define PAUSE_WASH_MS 150
/* The tab bar: keycaps 12.5 tall from 25 down, 49.2 from the left edge;
 * labels 30 apart in capitals 5.4 tall on a baseline 34.4 down, 12.5 after
 * the left keycap and 16.7 before the right one. The active label is Bold,
 * 6.25 tall, between brackets as tall as the keycaps, centered where it
 * stands idle. */
#define PAUSE_TAB_KEY_LEFT 49.2f
#define PAUSE_TAB_KEY_TOP 25.0f
#define PAUSE_TAB_KEY_HEIGHT 12.5f
#define PAUSE_TAB_FIRST_GAP 12.5f
#define PAUSE_TAB_LAST_GAP 16.7f
#define PAUSE_TAB_GAP 30.0f
#define PAUSE_TAB_CAP 5.4f
#define PAUSE_TAB_TRACKING 0.13f
#define PAUSE_TAB_BASELINE 34.4f
#define PAUSE_TAB_ACTIVE_CAP 6.25f
#define PAUSE_TAB_ACTIVE_TRACKING 0.12f
#define PAUSE_TAB_ACTIVE_BASELINE 35.0f
#define PAUSE_TAB_BRACKET_PAD 3.0f
#define PAUSE_TAB_BRACKET_TICK 2.5f
/* Rows of a list: 27.9 tall, 29.25 apart, framed from the left edge when
 * focused; names from 54.2, focused in Bold from 62.5. */
#define PAUSE_ROW_HEIGHT 27.9f
#define PAUSE_ROW_PITCH 29.25f
#define PAUSE_ROW_LEFT 54.2f
#define PAUSE_ROW_FOCUS_LEFT 62.5f
/* The Class tab: the two lists' toggles, the rows under them and the
 * loadout's tiles, each under its slot and weapon. The chosen toggle stands
 * 2.5 higher, its label in Bold capitals 6.6 tall. */
#define PAUSE_TOGGLE_LEFT 43.3f
#define PAUSE_TOGGLE_WIDTH 90.6f
#define PAUSE_TOGGLE_STEP 95.9f
#define PAUSE_TOGGLE_TOP 70.8f
#define PAUSE_TOGGLE_HEIGHT 13.4f
#define PAUSE_TOGGLE_RAISE 2.5f
#define PAUSE_TOGGLE_CAP 5.0f
#define PAUSE_TOGGLE_CHOSEN_CAP 6.6f
#define PAUSE_CLASS_TOP 98.3f
#define PAUSE_CLASS_RIGHT 229.2f
#define PAUSE_TILE_CAPTION_CAP 5.0f
#define PAUSE_TILE_CAPTION_ABOVE 15.9f
#define PAUSE_TILE_NAME_CAP 6.5f
#define PAUSE_TILE_NAME_ABOVE 5.0f
/* Leave Game: the question in Bold capitals 11.3 tall, then two rows. */
#define PAUSE_QUESTION_LEFT 44.6f
#define PAUSE_QUESTION_BASELINE 167.0f
#define PAUSE_QUESTION_CAP 11.3f
#define PAUSE_QUESTION_TRACKING 0.06f
#define PAUSE_LEAVE_TOP 207.5f
#define PAUSE_LIST_RIGHT 265.8f
/* Settings: the rows, and the focused one's name and note at the right,
 * the note's lines 1.55 capitals apart. */
#define PAUSE_SETTINGS_TOP 89.6f
#define PAUSE_SETTINGS_TEXT_LEFT 411.0f
#define PAUSE_SETTINGS_TITLE_BASELINE 106.7f
#define PAUSE_SETTINGS_TITLE_CAP 9.6f
#define PAUSE_SETTINGS_TITLE_TRACKING 0.07f
#define PAUSE_SETTINGS_NOTE_BASELINE 121.0f
#define PAUSE_SETTINGS_NOTE_CAP 6.75f
#define PAUSE_SETTINGS_NOTE_TRACKING 0.07f
#define PAUSE_SETTINGS_NOTE_LINE 1.55f
#define PAUSE_SETTINGS_NOTE_WIDTH 236.0f
/* Change Team: two square tiles 155.8 a side, 9 apart, and one half as tall
 * under the first; titles 21 under a tile's top, the count and Slots Open
 * near its bottom. */
#define PAUSE_TEAM_LEFT 44.2f
#define PAUSE_TEAM_TOP 56.0f
#define PAUSE_TEAM_SIZE 155.8f
#define PAUSE_TEAM_GAP 9.0f
#define PAUSE_TEAM_AUTO_HEIGHT 78.3f
#define PAUSE_TEAM_PAD 9.1f
#define PAUSE_TEAM_TITLE_BASELINE 21.0f
#define PAUSE_TEAM_TITLE_CAP 7.2f
#define PAUSE_TEAM_TITLE_FOCUS_CAP 10.0f
#define PAUSE_TEAM_COUNT_CAP 10.0f
#define PAUSE_TEAM_OPEN_CAP 6.2f
#define PAUSE_TEAM_OPEN_ABOVE 5.5f
#define PAUSE_TEAM_COUNT_ABOVE 14.0f
#define PAUSE_TEAM_EMBLEM 117.0f
/* The Overview: the title, the clock at the right, two headed sections and
 * the map. */
#define PAUSE_OVERVIEW_LEFT 44.6f
#define PAUSE_OVERVIEW_TITLE_BASELINE 70.0f
#define PAUSE_OVERVIEW_TITLE_CAP 10.2f
#define PAUSE_OVERVIEW_TITLE_TRACKING 0.28f
#define PAUSE_OVERVIEW_TITLE_ROOM 340.0f
#define PAUSE_CLOCK_RIGHT 45.0f
#define PAUSE_CLOCK_LABEL_BASELINE 66.7f
#define PAUSE_CLOCK_LABEL_CAP 8.0f
#define PAUSE_CLOCK_LABEL_TRACKING 0.06f
#define PAUSE_CLOCK_BASELINE 93.3f
#define PAUSE_CLOCK_CAP 17.6f
#define PAUSE_HEADER_LEFT 43.3f
#define PAUSE_HEADER_WIDTH 250.0f
#define PAUSE_HEADER_HEIGHT 18.3f
#define PAUSE_HEADER_TEXT_LEFT 5.0f
#define PAUSE_HEADER_CAP 5.9f
#define PAUSE_HEADER_TRACKING 0.22f
#define PAUSE_MATCH_TOP 87.5f
#define PAUSE_STATS_TOP 196.7f
#define PAUSE_SCORE_EMBLEM_LEFT 43.8f
#define PAUSE_SCORE_EMBLEM_TOP 120.0f
#define PAUSE_SCORE_EMBLEM_SIZE 52.0f
#define PAUSE_SCORE_EMBLEM_ALPHA 0.16f
#define PAUSE_SCORE_BAR_LEFT 87.5f
#define PAUSE_SCORE_BAR_WIDTH 209.2f
#define PAUSE_SCORE_TOP 127.5f
#define PAUSE_SCORE_TOP_HEIGHT 27.5f
#define PAUSE_SCORE_BOTTOM 158.3f
#define PAUSE_SCORE_BOTTOM_HEIGHT 12.5f
#define PAUSE_SCORE_TOP_CAP 14.0f
#define PAUSE_SCORE_BOTTOM_CAP 7.6f
#define PAUSE_SCORE_PAD 4.2f
#define PAUSE_SCORE_ARROW_LEFT 81.0f
#define PAUSE_SCORE_ARROW_WIDTH 7.5f
#define PAUSE_SCORE_ARROW_HEIGHT 9.1f
#define PAUSE_STAT_LEFT 47.0f
#define PAUSE_STAT_COLUMN 146.0f
#define PAUSE_STAT_LABEL_BASELINE 232.3f
#define PAUSE_STAT_VALUE_BASELINE 258.3f
#define PAUSE_STAT_VALUE_INSET 2.5f
#define PAUSE_STAT_PITCH 57.1f
#define PAUSE_STAT_LABEL_CAP 6.7f
#define PAUSE_STAT_VALUE_CAP 21.0f
/* The map's box, its middle where WWII's map stands, 333 tall; a square map
 * fills its height. */
#define PAUSE_MAP_LEFT 320.0f
#define PAUSE_MAP_TOP 70.0f
#define PAUSE_MAP_WIDTH 360.0f
#define PAUSE_MAP_HEIGHT 333.0f
/* The prompts: glyphs 15 across with their middles 32.5 above the bottom,
 * from 53.3 at the left and to 54.2 from the right; labels in capitals 6.6
 * tall on a baseline 29.2 above it. */
#define PAUSE_PROMPT_LEFT 53.3f
#define PAUSE_PROMPT_RIGHT 54.2f
#define PAUSE_PROMPT_MIDDLE 32.5f
#define PAUSE_PROMPT_GLYPH 15.0f
#define PAUSE_PROMPT_KEYCAP 10.2f
#define PAUSE_PROMPT_LABEL_CAP 6.6f
#define PAUSE_PROMPT_LABEL_BASELINE 29.2f
#define PAUSE_PROMPT_GAP 3.1f
#define PAUSE_PROMPT_SPACING 9.5f
/* The Vote tab: its actions as the settings list, the focused one's name and
 * note at the right over its choices, rows 18.5 apart, twelve at a time. */
#define PAUSE_VOTE_PANEL_LEFT 300.0f
#define PAUSE_VOTE_PANEL_WIDTH 270.0f
#define PAUSE_VOTE_CHOICE_TOP 150.0f
#define PAUSE_VOTE_CHOICE_HEIGHT 17.0f
#define PAUSE_VOTE_CHOICE_PITCH 18.5f
#define PAUSE_VOTE_CHOICE_ROWS 12
#define PAUSE_VOTE_CHOICE_PAD 6.0f
#define PAUSE_VOTE_CHOICE_CAP 5.6f
#define PAUSE_VOTE_CHOICE_FOCUS_CAP 6.2f
#define PAUSE_VOTE_CHOICES 64
#define PAUSE_VOTE_TAG_CAP 4.6f
#define PAUSE_VOTE_RULE_CAP 4.6f
#define PAUSE_VOTE_RULE_WIDTH 205.0f
#define PAUSE_VOTE_TEXT_CAP 8.0f
#define PAUSE_VOTE_STAT_COLUMN 80.0f
#define PAUSE_VOTE_STAT_LABEL_BASELINE 172.0f
#define PAUSE_VOTE_STAT_VALUE_BASELINE 198.0f
#define PAUSE_VOTE_RULE_BASELINE 228.0f

enum { PAUSE_TAB_CLASS, PAUSE_TAB_OVERVIEW, PAUSE_TAB_TEAM, PAUSE_TAB_VOTE, PAUSE_TAB_LEAVE, PAUSE_TAB_SETTINGS,
       PAUSE_TAB_COUNT };
enum { PAUSE_TEAM_ALLIES, PAUSE_TEAM_AXIS, PAUSE_TEAM_AUTO, PAUSE_TEAM_COUNT };
enum { PAUSE_VOTE_MAP, PAUSE_VOTE_MODE, PAUSE_VOTE_RESTART, PAUSE_VOTE_NEXT, PAUSE_VOTE_KICK, PAUSE_VOTE_COUNT };
enum { PAUSE_BALLOT_YES, PAUSE_BALLOT_NO, PAUSE_BALLOT_COUNT };
enum { PAUSE_LEAVE_RETURN, PAUSE_LEAVE_MATCH, PAUSE_LEAVE_COUNT };
enum { PAUSE_SETTINGS_CONTROLS, PAUSE_SETTINGS_GRAPHICS, PAUSE_SETTINGS_AUDIO, PAUSE_SETTINGS_GAME, PAUSE_SETTINGS_COUNT };
/* What the cursor is over: the kind in the high byte, an index in the low. */
enum { PAUSE_HOVER_NONE, PAUSE_HOVER_TAB, PAUSE_HOVER_ROW, PAUSE_HOVER_TOGGLE, PAUSE_HOVER_TILE, PAUSE_HOVER_CHOICE };
#define PAUSE_HOVER(kind, index) (((kind) << 8) | (index))

static const char *const s_pauseTabs[PAUSE_TAB_COUNT] = { "CLASS", "OVERVIEW", "CHANGE TEAM", "VOTE", "LEAVE GAME",
                                                          "SETTINGS" };
/* What a vote may do, as the server's callvote takes it. */
static const struct {
    const char *name;
    const char *note;
} s_pauseVotes[PAUSE_VOTE_COUNT] = {
    { "CHANGE MAP", "Move the match to another map, in the mode played now." },
    { "CHANGE MODE", "Play another mode on this map, from the start." },
    { "RESTART MAP", "Start this map over, with the scores and the clock." },
    { "NEXT MAP", "Move on to the next map of the rotation." },
    { "KICK PLAYER", "Remove a player from the match." },
};
static const char *const s_pauseBallots[PAUSE_BALLOT_COUNT] = { "VOTE YES", "VOTE NO" };
#define PAUSE_VOTE_RULE "A vote passes when more than half of the players vote yes. Bots do not vote."
static const char *const s_pauseSlots[CLASS_SLOT_COUNT] = { "PRIMARY", "SECONDARY", "LETHAL", "TACTICAL" };
static const char *const s_pauseLeave[PAUSE_LEAVE_COUNT] = { "RETURN TO MATCH", "LEAVE MATCH" };
static const struct {
    const char *name;
    const char *note;
    /* The retail options page it opens over the options list; Controls opens
     * the list itself, whose categories hold every control page. */
    const char *page;
} s_pauseSettings[PAUSE_SETTINGS_COUNT] = {
    { "CONTROLS", "Set up your control scheme to match your style of play", NULL },
    { "GRAPHICS", "Adjust the picture: resolution, detail and effects", "options_graphics" },
    { "AUDIO", "Set the volume of the game's sound and music", "options_sound" },
    { "GAME", "Change your gametag and how the game behaves", "options_game" },
};

/* How a list letters its rows: the names' capitals and tracking, idle and
 * focused, and how far above the row's middle they stand. The class and
 * settings lists are lettered larger than Leave Game's, the classes wider. */
typedef struct {
    float cap;
    float tracking;
    float focusCap;
    float focusTracking;
    float lift;
} PauseRowStyle;

static const PauseRowStyle s_pauseClassRows = { 6.8f, 0.30f, 7.2f, 0.33f, 1.3f };
static const PauseRowStyle s_pauseSettingsRows = { 6.8f, 0.09f, 7.2f, 0.12f, 1.3f };
static const PauseRowStyle s_pauseLeaveRows = { 6.3f, 0.075f, 7.2f, 0.12f, 0.0f };
static const char *const s_pauseStats[6] = { "K/D RATIO", "SCORE", "KILLS", "ASSISTS", "DEATHS", "HEADSHOTS" };

static const vec4_t s_pauseBlack = { 0.0f, 0.0f, 0.0f, 1.0f };
static const vec4_t s_pauseFrame = { 0.62f, 0.58f, 0.49f, 1.0f };
static const vec4_t s_pauseBracket = { 0.73f, 0.69f, 0.59f, 0.85f };
/* The captures' letters: a paler gold than the main menu's, and the thin
 * ones lit brighter, as WWII's thin strokes come out brighter than Barlow's
 * at the same size. */
static const vec4_t s_pauseGold = { 0.73f, 0.69f, 0.59f, 1.0f };
static const vec4_t s_pauseIdle = { 0.90f, 0.885f, 0.85f, 1.0f };
static const vec4_t s_pauseLight = { 0.85f, 0.84f, 0.80f, 1.0f };
static const vec4_t s_pauseBright = { 0.90f, 0.89f, 0.85f, 1.0f };
static const vec4_t s_pauseFine = { 0.96f, 0.95f, 0.92f, 1.0f };
static const vec4_t s_pauseNoteText = { 0.92f, 0.91f, 0.87f, 1.0f };
static const vec4_t s_pauseStatLabel = { 0.80f, 0.78f, 0.75f, 1.0f };
static const vec4_t s_pauseNote = { 0.72f, 0.70f, 0.64f, 1.0f };
static const vec4_t s_pauseRule = { 1.0f, 1.0f, 1.0f, 0.22f };
static const vec4_t s_pauseTitle = { 0.73f, 0.68f, 0.56f, 1.0f };
static const vec4_t s_pauseToggle = { 0.78f, 0.76f, 0.72f, 0.95f };
static const vec4_t s_pauseToggleText = { 0.17f, 0.15f, 0.12f, 1.0f };
static const vec4_t s_pauseTile = { 0.0f, 0.0f, 0.0f, 0.32f };
static const vec4_t s_pauseTileEdge = { 1.0f, 1.0f, 1.0f, 0.24f };
static const vec4_t s_pauseTeamTile = { 0.24f, 0.29f, 0.32f, 0.62f };
static const vec4_t s_pauseTeamFocus = { 0.55f, 0.50f, 0.36f, 0.82f };
static const vec4_t s_pauseTeamEdge = { 1.0f, 1.0f, 1.0f, 0.16f };
static const vec4_t s_pauseEmblem = { 0.68f, 0.70f, 0.70f, 0.62f };
static const vec4_t s_pauseEmblemFocus = { 0.92f, 0.90f, 0.82f, 0.88f };
static const vec4_t s_pauseTrack = { 0.0f, 0.0f, 0.0f, 0.42f };
static const vec4_t s_pauseTopFill = { 0.22f, 0.59f, 0.59f, 0.9f };
static const vec4_t s_pauseBottomFill = { 0.69f, 0.25f, 0.16f, 0.9f };
static const vec4_t s_pauseValue = { 0.78f, 0.76f, 0.715f, 1.0f };
static const vec4_t s_pauseGlyphA = { 0.42f, 0.74f, 0.32f, 1.0f };
static const vec4_t s_pauseGlyphB = { 0.86f, 0.30f, 0.26f, 1.0f };

static struct {
    int open;
    int tab;
    /* The next opening keeps the tab: back from the editor or a settings page. */
    int keepTab;
    int openTime;
    int standard;
    int classFocus;
    int teamFocus;
    int leaveFocus;
    int settingsFocus;
    /* The Vote tab: the action, its choice and which of the two lists keys
     * move; while a vote runs, the ballot instead, and whether the last
     * frame painted the ballot's rows. */
    int voteFocus;
    int voteChoice;
    int voteColumn;
    int ballotFocus;
    int ballotRows;
    /* A settings page opened from here is up. */
    int settingsPage;
    int hover;
    int cursorX;
    int cursorY;
    /* Tabs shown: Change Team is not, where everyone plays for himself. */
    int tabs[PAUSE_TAB_COUNT];
    int tabCount;
} s_pause;

static float Pause_Clamp01(float value)
{
    return value < 0.0f ? 0.0f : value > 1.0f ? 1.0f : value;
}

static void Pause_Fill(float x, float y, float w, float h, int horz, const vec_t *color)
{
    CL_DrawStretchPic(x, y, w, h, horz, PAUSE_ALIGN_TOP, 0.0f, 0.0f, 1.0f, 1.0f, color,
                      sharedUiInfo.assets.whiteMaterial);
}

static void Pause_Frame(float x, float y, float w, float h, int horz, const vec_t *color)
{
    Pause_Fill(x, y, w, PAUSE_LINE, horz, color);
    Pause_Fill(x, y + h - PAUSE_LINE, w, PAUSE_LINE, horz, color);
    Pause_Fill(x, y, PAUSE_LINE, h, horz, color);
    Pause_Fill(x + w - PAUSE_LINE, y, PAUSE_LINE, h, horz, color);
}

static void Pause_Text(const char *text, int bold, float x, float baseline, int horz, float cap, float tracking,
                       const vec_t *color)
{
    UI_WWIIMenu_Text(text, bold, x, baseline, horz, PAUSE_ALIGN_TOP, cap, tracking, color);
}

static float Pause_Width(const char *text, int bold, float cap, float tracking)
{
    return UI_WWIIMenu_Width(text, bold, cap, tracking);
}

static int Pause_Hover(const displayContextDef_t *dc, float x, float y, float w, float h, int horz)
{
    float cx = (float)dc->cursorx, cy = (float)dc->cursory;

    CalcScreenPlacement(&x, &y, &w, &h, horz, PAUSE_ALIGN_TOP);
    CalcScreenX(&cx, PAUSE_ALIGN_FULLSCREEN);
    CalcScreenY(&cy, PAUSE_ALIGN_FULLSCREEN);
    return cx >= x && cx < x + w && cy >= y && cy < y + h;
}

static void Pause_Sound(const char *alias)
{
    UI_PlayLocalSoundAliasByName(alias);
}

static int Pause_Gamepad(void)
{
#if COD2_FEATURE_GAMEPAD
    return CL_Gamepad_IsActiveDevice();
#else
    return 0;
#endif
}

/* A script menu's response, as its own items send it. */
static int Pause_Respond(const char *menuName, int prefix, const char *response)
{
    int i;

    for (i = 0; i < 0x20; i++) {
        const char *menu = CL_GetConfigString(0x4de + i);

        if (!menu || !menu[0])
            continue;
        if (prefix ? !strncmp(menu, menuName, strlen(menuName)) : !I_stricmp(menu, menuName)) {
            Cbuf_ExecuteText(2, va("cmd mr %i %i %s\n", Dvar_GetInt("sv_serverId"), i, response));
            return 1;
        }
    }
    return 0;
}

static void Pause_Tabs(void)
{
    CG_TeamSlots slots;
    int i;

    CG_OverviewTeamSlots(&slots);
    s_pause.tabCount = 0;
    for (i = 0; i < PAUSE_TAB_COUNT; i++) {
        if (i == PAUSE_TAB_TEAM && slots.freeForAll)
            continue;
        s_pause.tabs[s_pause.tabCount++] = i;
    }
}

static int Pause_TabIndex(int tab)
{
    int i;

    for (i = 0; i < s_pause.tabCount; i++) {
        if (s_pause.tabs[i] == tab)
            return i;
    }
    return -1;
}

static void Pause_SetTab(int tab)
{
    if (tab != s_pause.tab)
        Pause_Sound("mouse_over");
    s_pause.tab = tab;
    s_pause.hover = PAUSE_HOVER_NONE;
}

static void Pause_StepTab(int step)
{
    int index = Pause_TabIndex(s_pause.tab);

    if (s_pause.tabCount <= 0)
        return;
    index = index < 0 ? 0 : (index + step + s_pause.tabCount) % s_pause.tabCount;
    Pause_SetTab(s_pause.tabs[index]);
}

void UI_PauseMenu_Open(displayContextDef_t *dc)
{
    CG_Vote vote;
    int standard;
    int chosen;

    s_pause.open = 1;
    s_pause.openTime = dc->realTime;
    s_pause.hover = PAUSE_HOVER_NONE;
    s_pause.cursorX = dc->cursorx;
    s_pause.cursorY = dc->cursory;
    Pause_Tabs();
    /* Back from the class editor or a settings page: the same tab. A page
     * opens the main menu twice on its way out, as it closes and when closed. */
    if ((s_pause.keepTab || s_pause.settingsPage) && Pause_TabIndex(s_pause.tab) >= 0) {
        s_pause.keepTab = 0;
        return;
    }
    s_pause.keepTab = 0;
    s_pause.settingsPage = 0;
    s_pause.tab = PAUSE_TAB_CLASS;
    s_pause.teamFocus = PAUSE_TEAM_ALLIES;
    s_pause.voteFocus = PAUSE_VOTE_MAP;
    s_pause.voteColumn = 0;
    s_pause.ballotFocus = PAUSE_BALLOT_YES;
    s_pause.leaveFocus = PAUSE_LEAVE_RETURN;
    s_pause.settingsFocus = PAUSE_SETTINGS_CONTROLS;
    chosen = UI_ClassMenu_Chosen(&standard);
    if (chosen >= 0) {
        s_pause.standard = standard;
        s_pause.classFocus = chosen;
    }
    /* A vote waiting for the player's ballot comes first. */
    if (CG_OverviewVote(&vote) && vote.active && !vote.voted && !vote.spectator)
        s_pause.tab = PAUSE_TAB_VOTE;
}

void UI_PauseMenu_Return(displayContextDef_t *dc)
{
    s_pause.keepTab = 1;
    Menus_OpenByName(dc, "ingame");
}

const char *UI_PauseMenu_Redirect(const char *name)
{
    /* Out of a match the main menu is the main menu again. */
    return s_pause.settingsPage && name && !I_stricmp(name, "main_text") && Dvar_GetBool("cl_ingame") ? "ingame"
                                                                                                     : name;
}

static int Pause_IsMenu(const menuDef_t *menu)
{
    return menu && menu->window.name && !I_stricmp(menu->window.name, "ingame");
}

int UI_PauseMenu_WantsKeys(const menuDef_t *menu)
{
    return Pause_IsMenu(menu);
}

static void Pause_Close(displayContextDef_t *dc)
{
    s_pause.open = 0;
    Menus_CloseByName(dc, "ingame");
}

/* Joining a side the server would not move the player to keeps him where he
 * is: an auto-assign from a team only balances. */
static void Pause_ChooseTeam(displayContextDef_t *dc, int choice)
{
    static const char *const responses[PAUSE_TEAM_COUNT] = { "allies", "axis", "autoassign" };
    CG_TeamSlots slots;
    int team = TEAM_SPECTATOR;

    CG_OverviewTeamSlots(&slots);
    if (choice == PAUSE_TEAM_AUTO && (slots.team == TEAM_ALLIES || slots.team == TEAM_AXIS)) {
        int own = slots.team == TEAM_ALLIES ? slots.allies : slots.axis;
        int other = slots.team == TEAM_ALLIES ? slots.axis : slots.allies;

        /* The stock auto-assign reopens the team menu for a player on a team. */
        choice = other + 1 < own ? (slots.team == TEAM_ALLIES ? PAUSE_TEAM_AXIS : PAUSE_TEAM_ALLIES) : -1;
    }
    if (choice == PAUSE_TEAM_ALLIES)
        team = TEAM_ALLIES;
    else if (choice == PAUSE_TEAM_AXIS)
        team = TEAM_AXIS;
    if (choice >= 0 && (team == TEAM_SPECTATOR || team != slots.team))
        Pause_Respond("team_", 1, responses[choice]);
    Pause_Close(dc);
}

static void Pause_OpenSettings(displayContextDef_t *dc, int page)
{
    if (!Menus_OpenByName(dc, "options_menu"))
        return;
    if (s_pauseSettings[page].page)
        Menus_OpenByName(dc, s_pauseSettings[page].page);
    s_pause.keepTab = 1;
    s_pause.settingsPage = 1;
    s_pause.open = 0;
    Menus_CloseByName(dc, "ingame");
}

/* An action's choices, capitals, each with the command that calls its vote,
 * and the one in play now, -1 for none. */
static struct {
    int count;
    int current;
    char names[PAUSE_VOTE_CHOICES][64];
    char commands[PAUSE_VOTE_CHOICES][96];
} s_voteChoices;

static void Pause_AddVoteChoice(const char *name, const char *command, int current)
{
    int i = s_voteChoices.count;
    int n;

    if (i >= PAUSE_VOTE_CHOICES)
        return;
    for (n = 0; name[n] && n < (int)sizeof(s_voteChoices.names[0]) - 1; n++)
        s_voteChoices.names[i][n] = (char)(name[n] >= 'a' && name[n] <= 'z' ? name[n] - 32 : name[n]);
    s_voteChoices.names[i][n] = '\0';
    snprintf(s_voteChoices.commands[i], sizeof(s_voteChoices.commands[0]), "%s", command);
    if (current)
        s_voteChoices.current = i;
    s_voteChoices.count = i + 1;
}

/* The arenas of Start New Server, its modes, or the people in the match;
 * restarting and the next map call their vote at once and have none. */
static void Pause_BuildVoteChoices(int action, const CG_Vote *vote)
{
    CG_VotePlayer players[PAUSE_VOTE_CHOICES];
    char command[96];
    int count;
    int i;

    s_voteChoices.count = 0;
    s_voteChoices.current = -1;
    switch (action) {
    case PAUSE_VOTE_MAP:
        for (i = 0; i < sharedUiInfo.mapCount; i++) {
            snprintf(command, sizeof(command), "cmd callvote map %s\n", sharedUiInfo.mapList[i].mapLoadName);
            Pause_AddVoteChoice(sharedUiInfo.mapList[i].mapName, command,
                                !I_stricmp(sharedUiInfo.mapList[i].mapLoadName, vote->map));
        }
        break;
    case PAUSE_VOTE_MODE:
        for (i = 0; i < sharedUiInfo.numGameTypes; i++) {
            const char *type = sharedUiInfo.gameTypes[i].gameType;
            const char *name = sharedUiInfo.gameTypes[i].gameTypeName;

            snprintf(command, sizeof(command), "cmd callvote g_gametype %s\n", type);
            Pause_AddVoteChoice(UI_SafeTranslateString(name && name[0] ? name : type), command,
                                !I_stricmp(type, vote->gametype));
        }
        break;
    case PAUSE_VOTE_KICK:
        count = CG_OverviewVotePlayers(players, PAUSE_VOTE_CHOICES);
        for (i = 0; i < count; i++) {
            snprintf(command, sizeof(command), "cmd callvote clientkick %d\n", players[i].clientNum);
            Pause_AddVoteChoice(players[i].name, command, 0);
        }
        break;
    default:
        break;
    }
}

static const char *Pause_VoteCommand(int action)
{
    if (action == PAUSE_VOTE_RESTART)
        return "cmd callvote map_restart\n";
    if (action == PAUSE_VOTE_NEXT)
        return "cmd callvote map_rotate\n";
    return NULL;
}

static void Pause_OpenVoteChoices(void)
{
    s_pause.voteColumn = 1;
    s_pause.voteChoice = s_voteChoices.current >= 0 ? s_voteChoices.current : 0;
}

/* A ballot cast, an action's choices opened, or a vote called; whatever is
 * sent closes the menu, so the vote shows on the HUD. */
static void Pause_ActivateVote(displayContextDef_t *dc)
{
    CG_Vote vote;
    const char *command;

    if (!CG_OverviewVote(&vote) || vote.spectator)
        return;
    if (vote.active) {
        if (vote.voted)
            return;
        Cbuf_ExecuteText(2, s_pause.ballotFocus == PAUSE_BALLOT_YES ? "cmd vote yes\n" : "cmd vote no\n");
        Pause_Close(dc);
        return;
    }
    command = Pause_VoteCommand(s_pause.voteFocus);
    if (command) {
        Cbuf_ExecuteText(2, command);
        Pause_Close(dc);
        return;
    }
    Pause_BuildVoteChoices(s_pause.voteFocus, &vote);
    if (!s_pause.voteColumn) {
        if (s_voteChoices.count)
            Pause_OpenVoteChoices();
        return;
    }
    if (s_pause.voteChoice >= 0 && s_pause.voteChoice < s_voteChoices.count) {
        Cbuf_ExecuteText(2, s_voteChoices.commands[s_pause.voteChoice]);
        Pause_Close(dc);
    }
}

static void Pause_Activate(displayContextDef_t *dc)
{
    Pause_Sound("mouse_click");
    switch (s_pause.tab) {
    case PAUSE_TAB_CLASS:
        if (UI_ClassMenu_Choose(s_pause.standard, s_pause.classFocus))
            Pause_Close(dc);
        break;
    case PAUSE_TAB_TEAM:
        Pause_ChooseTeam(dc, s_pause.teamFocus);
        break;
    case PAUSE_TAB_VOTE:
        Pause_ActivateVote(dc);
        break;
    case PAUSE_TAB_LEAVE:
        if (s_pause.leaveFocus == PAUSE_LEAVE_MATCH) {
            Pause_Close(dc);
            Cbuf_ExecuteText(2, "disconnect\n");
        } else {
            Pause_Close(dc);
        }
        break;
    case PAUSE_TAB_SETTINGS:
        Pause_OpenSettings(dc, s_pause.settingsFocus);
        break;
    default:
        break;
    }
}

static int Pause_Wrap(int value, int count)
{
    return (value % count + count) % count;
}

/* Up and down through the ballot, the actions or the open action's choices;
 * right opens the choices, left goes back to the actions. 1 when something
 * moved. */
static int Pause_NavigateVote(int key, int step, int vertical)
{
    CG_Vote vote;

    if (!CG_OverviewVote(&vote) || vote.spectator)
        return 0;
    if (vote.active) {
        if (!vertical || vote.voted)
            return 0;
        s_pause.ballotFocus = Pause_Wrap(s_pause.ballotFocus + step, PAUSE_BALLOT_COUNT);
        return 1;
    }
    Pause_BuildVoteChoices(s_pause.voteFocus, &vote);
    if (s_pause.voteColumn) {
        if (key == PAUSE_KEY_LEFT || !s_voteChoices.count) {
            s_pause.voteColumn = 0;
            return 1;
        }
        if (!vertical)
            return 0;
        s_pause.voteChoice = Pause_Wrap(s_pause.voteChoice + step, s_voteChoices.count);
        return 1;
    }
    if (vertical) {
        s_pause.voteFocus = Pause_Wrap(s_pause.voteFocus + step, PAUSE_VOTE_COUNT);
        return 1;
    }
    if (key == PAUSE_KEY_RIGHT && s_voteChoices.count) {
        Pause_OpenVoteChoices();
        return 1;
    }
    return 0;
}

/* Allies and Axis side by side over Auto-Assign. */
static int Pause_TeamStep(int focus, int key)
{
    if (key == PAUSE_KEY_LEFT || key == PAUSE_KEY_RIGHT)
        return focus == PAUSE_TEAM_ALLIES ? PAUSE_TEAM_AXIS : PAUSE_TEAM_ALLIES;
    if (focus == PAUSE_TEAM_AUTO)
        return PAUSE_TEAM_ALLIES;
    return PAUSE_TEAM_AUTO;
}

static void Pause_Navigate(int key)
{
    int step = key == PAUSE_KEY_UP || key == PAUSE_KEY_LEFT || key == PAUSE_KEY_WHEEL_UP ? -1 : 1;
    int vertical = key == PAUSE_KEY_UP || key == PAUSE_KEY_DOWN || key == PAUSE_KEY_WHEEL_UP || key == PAUSE_KEY_WHEEL_DOWN;

    switch (s_pause.tab) {
    case PAUSE_TAB_CLASS:
        if (vertical)
            s_pause.classFocus = Pause_Wrap(s_pause.classFocus + step, CLASS_COUNT);
        else
            s_pause.standard = !s_pause.standard;
        break;
    case PAUSE_TAB_TEAM:
        s_pause.teamFocus = Pause_TeamStep(s_pause.teamFocus, key);
        break;
    case PAUSE_TAB_VOTE:
        if (!Pause_NavigateVote(key, step, vertical))
            return;
        break;
    case PAUSE_TAB_LEAVE:
        if (vertical)
            s_pause.leaveFocus = Pause_Wrap(s_pause.leaveFocus + step, PAUSE_LEAVE_COUNT);
        break;
    case PAUSE_TAB_SETTINGS:
        if (vertical)
            s_pause.settingsFocus = Pause_Wrap(s_pause.settingsFocus + step, PAUSE_SETTINGS_COUNT);
        break;
    default:
        return;
    }
    Pause_Sound("mouse_over");
}

/* The element under the cursor takes the focus it painted with; 1 when it
 * is one that acts. A tab only changes with a click. */
static int Pause_FocusHover(void)
{
    int kind = s_pause.hover >> 8, index = s_pause.hover & 0xff;

    switch (kind) {
    case PAUSE_HOVER_TOGGLE:
        s_pause.standard = index;
        return 0;
    case PAUSE_HOVER_TILE:
        s_pause.teamFocus = index;
        return 1;
    case PAUSE_HOVER_ROW:
        if (s_pause.tab == PAUSE_TAB_CLASS) {
            s_pause.classFocus = index;
        } else if (s_pause.tab == PAUSE_TAB_LEAVE) {
            s_pause.leaveFocus = index;
        } else if (s_pause.tab == PAUSE_TAB_SETTINGS) {
            s_pause.settingsFocus = index;
        } else if (s_pause.tab == PAUSE_TAB_VOTE && s_pause.ballotRows) {
            s_pause.ballotFocus = index;
        } else if (s_pause.tab == PAUSE_TAB_VOTE) {
            s_pause.voteFocus = index;
            s_pause.voteColumn = 0;
        }
        return 1;
    case PAUSE_HOVER_CHOICE:
        s_pause.voteChoice = index;
        s_pause.voteColumn = 1;
        return 1;
    default:
        return 0;
    }
}

int UI_PauseMenu_KeyEvent(displayContextDef_t *dc, menuDef_t *menu, int key, int down)
{
    int tab;

    if (!Pause_IsMenu(menu))
        return 0;
    if (!down)
        return 1;
    if (!s_pause.open)
        UI_PauseMenu_Open(dc);
    s_pause.settingsPage = 0;
    tab = s_pause.tab;
    switch (key) {
    case PAUSE_KEY_ESCAPE:
    case PAUSE_KEY_MOUSE2:
        /* Back from a vote's choices to its actions; else out of the menu. */
        if (s_pause.tab == PAUSE_TAB_VOTE && s_pause.voteColumn)
            s_pause.voteColumn = 0;
        else
            Pause_Close(dc);
        break;
    case PAUSE_KEY_ENTER:
    case PAUSE_KEY_KP_ENTER:
        Pause_Activate(dc);
        break;
    case PAUSE_KEY_MOUSE1:
        if ((s_pause.hover >> 8) == PAUSE_HOVER_TAB)
            Pause_SetTab(s_pause.hover & 0xff);
        else if (Pause_FocusHover())
            Pause_Activate(dc);
        break;
    case 'q':
    case PAUSE_KEY_LSHLDR:
        Pause_StepTab(-1);
        break;
    case 'e':
    case PAUSE_KEY_RSHLDR:
    case PAUSE_KEY_TAB:
        Pause_StepTab(1);
        break;
    case PAUSE_KEY_F1:
        if (s_pause.tab == PAUSE_TAB_CLASS && !s_pause.standard)
            UI_ClassMenu_EditFromPause(dc, s_pause.classFocus);
        break;
    case PAUSE_KEY_UP:
    case PAUSE_KEY_DOWN:
    case PAUSE_KEY_LEFT:
    case PAUSE_KEY_RIGHT:
    case PAUSE_KEY_WHEEL_UP:
    case PAUSE_KEY_WHEEL_DOWN:
        Pause_Navigate(key);
        break;
    default:
        /* As the class menu's number keys: choose a class at once. */
        if (s_pause.tab == PAUSE_TAB_CLASS && key >= '1' && key < '1' + CLASS_COUNT) {
            s_pause.classFocus = key - '1';
            Pause_Activate(dc);
        }
        break;
    }
    /* The last frame's hover belongs to the tab it painted. */
    if (s_pause.tab != tab)
        s_pause.hover = PAUSE_HOVER_NONE;
    return 1;
}

/* The tab bar: each label 30 from the last at its idle width, so the active
 * one grows about its middle between its brackets. */
static void Pause_PaintTabs(displayContextDef_t *dc, int gamepad)
{
    float x = PAUSE_TAB_KEY_LEFT;
    int i;

    x += UI_WWIIMenu_Keycap(gamepad ? "LB" : "Q", x, PAUSE_TAB_KEY_TOP, PAUSE_TAB_KEY_HEIGHT, PAUSE_ALIGN_LEFT,
                            PAUSE_ALIGN_TOP, NULL);
    x += PAUSE_TAB_FIRST_GAP;
    for (i = 0; i < s_pause.tabCount; i++) {
        int tab = s_pause.tabs[i];
        const char *label = s_pauseTabs[tab];
        float width = Pause_Width(label, 0, PAUSE_TAB_CAP, PAUSE_TAB_TRACKING);
        float middle = x + 0.5f * width;

        if (Pause_Hover(dc, x - 0.5f * PAUSE_TAB_GAP, PAUSE_TAB_KEY_TOP - 3.0f, width + PAUSE_TAB_GAP,
                        PAUSE_TAB_KEY_HEIGHT + 6.0f, PAUSE_ALIGN_LEFT))
            s_pause.hover = PAUSE_HOVER(PAUSE_HOVER_TAB, tab);
        if (tab == s_pause.tab) {
            float active = Pause_Width(label, 1, PAUSE_TAB_ACTIVE_CAP, PAUSE_TAB_ACTIVE_TRACKING);
            float left = middle - 0.5f * active - PAUSE_TAB_BRACKET_PAD;
            float right = middle + 0.5f * active + PAUSE_TAB_BRACKET_PAD - PAUSE_LINE;

            Pause_Text(label, 1, middle - 0.5f * active, PAUSE_TAB_ACTIVE_BASELINE, PAUSE_ALIGN_LEFT,
                       PAUSE_TAB_ACTIVE_CAP, PAUSE_TAB_ACTIVE_TRACKING, s_pauseGold);
            Pause_Fill(left, PAUSE_TAB_KEY_TOP, PAUSE_LINE, PAUSE_TAB_KEY_HEIGHT, PAUSE_ALIGN_LEFT, s_pauseBracket);
            Pause_Fill(right, PAUSE_TAB_KEY_TOP, PAUSE_LINE, PAUSE_TAB_KEY_HEIGHT, PAUSE_ALIGN_LEFT, s_pauseBracket);
            Pause_Fill(left, PAUSE_TAB_KEY_TOP, PAUSE_TAB_BRACKET_TICK, PAUSE_LINE, PAUSE_ALIGN_LEFT, s_pauseBracket);
            Pause_Fill(left, PAUSE_TAB_KEY_TOP + PAUSE_TAB_KEY_HEIGHT - PAUSE_LINE, PAUSE_TAB_BRACKET_TICK, PAUSE_LINE,
                       PAUSE_ALIGN_LEFT, s_pauseBracket);
            Pause_Fill(right + PAUSE_LINE - PAUSE_TAB_BRACKET_TICK, PAUSE_TAB_KEY_TOP, PAUSE_TAB_BRACKET_TICK,
                       PAUSE_LINE, PAUSE_ALIGN_LEFT, s_pauseBracket);
            Pause_Fill(right + PAUSE_LINE - PAUSE_TAB_BRACKET_TICK, PAUSE_TAB_KEY_TOP + PAUSE_TAB_KEY_HEIGHT - PAUSE_LINE,
                       PAUSE_TAB_BRACKET_TICK, PAUSE_LINE, PAUSE_ALIGN_LEFT, s_pauseBracket);
        } else {
            Pause_Text(label, 0, x, PAUSE_TAB_BASELINE, PAUSE_ALIGN_LEFT, PAUSE_TAB_CAP, PAUSE_TAB_TRACKING,
                       s_pauseIdle);
        }
        x += width + (i + 1 < s_pause.tabCount ? PAUSE_TAB_GAP : PAUSE_TAB_LAST_GAP);
    }
    UI_WWIIMenu_Keycap(gamepad ? "RB" : "E", x, PAUSE_TAB_KEY_TOP, PAUSE_TAB_KEY_HEIGHT, PAUSE_ALIGN_LEFT,
                       PAUSE_ALIGN_TOP, NULL);
}

/* A list's row: framed from the left edge when focused, its name sliding in
 * and growing into gold Bold. */
static void Pause_Row(displayContextDef_t *dc, const char *name, int index, float top, float right, int focused,
                      const PauseRowStyle *style)
{
    float middle = top + 0.5f * PAUSE_ROW_HEIGHT - style->lift;
    char upper[64];
    int i;

    for (i = 0; name[i] && i < (int)sizeof(upper) - 1; i++)
        upper[i] = (char)(name[i] >= 'a' && name[i] <= 'z' ? name[i] - 32 : name[i]);
    upper[i] = '\0';
    if (Pause_Hover(dc, 0.0f, top, right, PAUSE_ROW_HEIGHT, PAUSE_ALIGN_LEFT))
        s_pause.hover = PAUSE_HOVER(PAUSE_HOVER_ROW, index);
    if (focused) {
        /* The frame runs from the screen's edge: no line on the left. */
        Pause_Fill(0.0f, top, right, PAUSE_LINE, PAUSE_ALIGN_LEFT, s_pauseFrame);
        Pause_Fill(0.0f, top + PAUSE_ROW_HEIGHT - PAUSE_LINE, right, PAUSE_LINE, PAUSE_ALIGN_LEFT, s_pauseFrame);
        Pause_Fill(right - PAUSE_LINE, top, PAUSE_LINE, PAUSE_ROW_HEIGHT, PAUSE_ALIGN_LEFT, s_pauseFrame);
        Pause_Text(upper, 1, PAUSE_ROW_FOCUS_LEFT, middle + 0.5f * style->focusCap, PAUSE_ALIGN_LEFT, style->focusCap,
                   style->focusTracking, s_pauseGold);
    } else {
        Pause_Text(upper, 0, PAUSE_ROW_LEFT, middle + 0.5f * style->cap, PAUSE_ALIGN_LEFT, style->cap, style->tracking,
                   s_pauseIdle);
    }
}

/* A loadout tile under its slot and weapon, the weapon's picture inside. */
static void Pause_Tile(const ClassLoadout *loadout, int slot, float x, float y, float w, float h, float nameRoom)
{
    char name[96];
    float cap = PAUSE_TILE_NAME_CAP;
    float width;
    int i;

    Pause_Text(s_pauseSlots[slot], 0, x, y - PAUSE_TILE_CAPTION_ABOVE, PAUSE_ALIGN_LEFT, PAUSE_TILE_CAPTION_CAP, 0.04f,
               s_pauseFine);
    snprintf(name, sizeof(name), "%s", UI_ClassMenu_WeaponName(loadout->weapon[slot]));
    for (i = 0; name[i]; i++)
        name[i] = (char)(name[i] >= 'a' && name[i] <= 'z' ? name[i] - 32 : name[i]);
    width = Pause_Width(name, 1, cap, 0.06f);
    if (width > nameRoom)
        cap *= nameRoom / width;
    Pause_Text(name, 1, x, y - PAUSE_TILE_NAME_ABOVE, PAUSE_ALIGN_LEFT, cap, 0.06f, s_pauseBright);
    Pause_Fill(x, y, w, h, PAUSE_ALIGN_LEFT, s_pauseTile);
    Pause_Frame(x, y, w, h, PAUSE_ALIGN_LEFT, s_pauseTileEdge);
    UI_ClassMenu_DrawWeapon(loadout, slot, x + 0.04f * w, y + 0.08f * h, 0.92f * w, 0.84f * h, PAUSE_ALIGN_LEFT,
                            UI_WWIIMenu_Color(WWII_COLOR_WHITE));
}

static void Pause_PaintClass(displayContextDef_t *dc)
{
    static const char *const toggles[2] = { "CUSTOM", "STANDARD ISSUE" };
    /* PRIMARY over SECONDARY, then LETHAL and TACTICAL side by side, where
     * WWII's division and basic training are gone. */
    static const float tiles[CLASS_SLOT_COUNT][4] = {
        { 250.0f, 91.7f, 122.5f, 121.6f },
        { 250.0f, 245.0f, 82.5f, 81.7f },
        { 250.0f, 356.7f, 38.3f, 38.3f },
        { 375.0f, 356.7f, 37.5f, 38.3f },
    };
    const ClassLoadout *classes = UI_ClassMenu_Classes(s_pause.standard);
    const ClassLoadout *focused;
    const char *note;
    int i;

    for (i = 0; i < 2; i++) {
        float x = PAUSE_TOGGLE_LEFT + (float)i * PAUSE_TOGGLE_STEP;
        int selected = i == s_pause.standard;
        float top = PAUSE_TOGGLE_TOP - (selected ? PAUSE_TOGGLE_RAISE : 0.0f);
        float height = PAUSE_TOGGLE_HEIGHT + (selected ? PAUSE_TOGGLE_RAISE : 0.0f);
        float cap = selected ? PAUSE_TOGGLE_CHOSEN_CAP : PAUSE_TOGGLE_CAP;
        float width = Pause_Width(toggles[i], selected, cap, 0.06f);

        if (Pause_Hover(dc, x, top, PAUSE_TOGGLE_WIDTH, height, PAUSE_ALIGN_LEFT))
            s_pause.hover = PAUSE_HOVER(PAUSE_HOVER_TOGGLE, i);
        if (selected)
            Pause_Fill(x, top, PAUSE_TOGGLE_WIDTH, height, PAUSE_ALIGN_LEFT, s_pauseToggle);
        else
            Pause_Frame(x, top, PAUSE_TOGGLE_WIDTH, height, PAUSE_ALIGN_LEFT, s_pauseToggle);
        Pause_Text(toggles[i], selected, x + 0.5f * (PAUSE_TOGGLE_WIDTH - width), top + 0.5f * (height + cap),
                   PAUSE_ALIGN_LEFT, cap, 0.06f, selected ? s_pauseToggleText : s_pauseFine);
    }
    for (i = 0; i < CLASS_COUNT; i++)
        Pause_Row(dc, classes[i].name, i, PAUSE_CLASS_TOP + (float)i * PAUSE_ROW_PITCH, PAUSE_CLASS_RIGHT,
                  i == s_pause.classFocus, &s_pauseClassRows);
    note = s_pause.standard ? NULL : UI_ClassMenu_Note();
    if (note)
        Pause_Text(note, 0, PAUSE_ROW_LEFT, PAUSE_CLASS_TOP + (float)CLASS_COUNT * PAUSE_ROW_PITCH + 10.0f,
                   PAUSE_ALIGN_LEFT, 4.4f, 0.0f, s_pauseNote);

    focused = &classes[s_pause.classFocus];
    for (i = 0; i < CLASS_SLOT_COUNT; i++) {
        /* A name may run past a small tile, up to the next column. */
        float room = i == CLASS_SLOT_LETHAL ? tiles[CLASS_SLOT_TACTICAL][0] - tiles[i][0] - 6.0f : 160.0f;

        Pause_Tile(focused, i, tiles[i][0], tiles[i][1], tiles[i][2], tiles[i][3], room);
    }
}

static void Pause_Header(const char *label, float top)
{
    char text[64];

    Pause_Fill(PAUSE_HEADER_LEFT, top, PAUSE_HEADER_WIDTH, PAUSE_LINE * 0.8f, PAUSE_ALIGN_LEFT, s_pauseRule);
    Pause_Fill(PAUSE_HEADER_LEFT, top + PAUSE_HEADER_HEIGHT, PAUSE_HEADER_WIDTH, PAUSE_LINE * 0.8f, PAUSE_ALIGN_LEFT,
               s_pauseRule);
    snprintf(text, sizeof(text), "\xbb %s", label);
    Pause_Text(text, 0, PAUSE_HEADER_LEFT + PAUSE_HEADER_TEXT_LEFT,
               top + 0.5f * (PAUSE_HEADER_HEIGHT + PAUSE_HEADER_CAP), PAUSE_ALIGN_LEFT, PAUSE_HEADER_CAP,
               PAUSE_HEADER_TRACKING, s_pauseNote);
}

static void Pause_ScoreBar(float top, float height, float cap, int score, int limit, const vec_t *fill)
{
    char number[16];
    float width;
    float filled = 0.0f;

    snprintf(number, sizeof(number), "%d", score);
    width = Pause_Width(number, 0, cap, 0.0f) + 2.0f * PAUSE_SCORE_PAD;
    if (limit > 0 && score > 0)
        filled = PAUSE_SCORE_BAR_WIDTH * Pause_Clamp01((float)score / (float)limit);
    if (score > 0 && filled < width)
        filled = width;
    if (filled > 0.0f)
        Pause_Fill(PAUSE_SCORE_BAR_LEFT, top, filled, height, PAUSE_ALIGN_LEFT, fill);
    Pause_Fill(PAUSE_SCORE_BAR_LEFT + filled, top, PAUSE_SCORE_BAR_WIDTH - filled, height, PAUSE_ALIGN_LEFT,
               s_pauseTrack);
    Pause_Text(number, 0, PAUSE_SCORE_BAR_LEFT + PAUSE_SCORE_PAD, top + 0.5f * (height + cap), PAUSE_ALIGN_LEFT, cap,
               0.0f, UI_WWIIMenu_Color(WWII_COLOR_WHITE));
}

static void Pause_PaintOverview(void)
{
    CG_Overview overview;
    char title[160];
    char value[32];
    float cap = PAUSE_OVERVIEW_TITLE_CAP;
    float width;
    int values[6];
    int i;

    if (!CG_OverviewGet(&overview))
        return;
    snprintf(title, sizeof(title), "%s // %s", overview.mode, overview.map);
    width = Pause_Width(title, 1, cap, PAUSE_OVERVIEW_TITLE_TRACKING);
    if (width > PAUSE_OVERVIEW_TITLE_ROOM)
        cap *= PAUSE_OVERVIEW_TITLE_ROOM / width;
    Pause_Text(title, 1, PAUSE_OVERVIEW_LEFT, PAUSE_OVERVIEW_TITLE_BASELINE, PAUSE_ALIGN_LEFT, cap,
               PAUSE_OVERVIEW_TITLE_TRACKING, s_pauseTitle);

    if (overview.clock[0]) {
        vec4_t color;

        memcpy(color, overview.clockColor, sizeof(color));
        /* The stock clock's white is the menu's; its gold in the SD grace period stays. */
        if (color[0] == 1.0f && color[1] == 1.0f && color[2] == 1.0f)
            memcpy(color, s_pauseValue, sizeof(color));
        Pause_Text("TIME REMAINING", 0,
                   -PAUSE_CLOCK_RIGHT -
                       Pause_Width("TIME REMAINING", 0, PAUSE_CLOCK_LABEL_CAP, PAUSE_CLOCK_LABEL_TRACKING),
                   PAUSE_CLOCK_LABEL_BASELINE, PAUSE_ALIGN_RIGHT, PAUSE_CLOCK_LABEL_CAP, PAUSE_CLOCK_LABEL_TRACKING,
                   s_pauseBright);
        Pause_Text(overview.clock, 1, -PAUSE_CLOCK_RIGHT - Pause_Width(overview.clock, 1, PAUSE_CLOCK_CAP, 0.02f),
                   PAUSE_CLOCK_BASELINE, PAUSE_ALIGN_RIGHT, PAUSE_CLOCK_CAP, 0.02f, color);
    }

    Pause_Header("MATCH SCORE", PAUSE_MATCH_TOP);
    if (overview.emblem >= 0) {
        int allies;
        MaterialHandle emblems = CG_OverviewEmblems(PAUSE_TEAM_EMBLEM, &allies);
        float s0 = overview.emblem == allies ? 0.0f : 0.5f;
        const vec4_t faint = { 1.0f, 1.0f, 1.0f, PAUSE_SCORE_EMBLEM_ALPHA };

        if (emblems)
            CL_DrawStretchPic(PAUSE_SCORE_EMBLEM_LEFT, PAUSE_SCORE_EMBLEM_TOP, PAUSE_SCORE_EMBLEM_SIZE,
                              PAUSE_SCORE_EMBLEM_SIZE, PAUSE_ALIGN_LEFT, PAUSE_ALIGN_TOP, s0, 0.0f, s0 + 0.5f, 1.0f,
                              faint, emblems);
    }
    Pause_ScoreBar(PAUSE_SCORE_TOP, PAUSE_SCORE_TOP_HEIGHT, PAUSE_SCORE_TOP_CAP, overview.topScore, overview.limit,
                   s_pauseTopFill);
    Pause_ScoreBar(PAUSE_SCORE_BOTTOM, PAUSE_SCORE_BOTTOM_HEIGHT, PAUSE_SCORE_BOTTOM_CAP, overview.bottomScore,
                   overview.limit, s_pauseBottomFill);
    if (overview.standing != CG_SCORE_PANEL_NEUTRAL) {
        float middle = PAUSE_SCORE_TOP + 0.5f * PAUSE_SCORE_TOP_HEIGHT;
        int step;

        /* A triangle pointing at the viewer's bar, as columns a tenth of a unit wide. */
        for (step = 0; step < 10; step++) {
            float t = (float)step / 10.0f;
            float half = 0.5f * PAUSE_SCORE_ARROW_HEIGHT * (1.0f - t);

            Pause_Fill(PAUSE_SCORE_ARROW_LEFT + t * PAUSE_SCORE_ARROW_WIDTH, middle - half,
                       PAUSE_SCORE_ARROW_WIDTH / 10.0f, 2.0f * half, PAUSE_ALIGN_LEFT,
                       UI_WWIIMenu_Color(WWII_COLOR_WHITE));
        }
    }

    Pause_Header("BATTLE STATS", PAUSE_STATS_TOP);
    values[0] = 0;
    values[1] = overview.score;
    values[2] = overview.kills;
    values[3] = overview.assists;
    values[4] = overview.deaths;
    values[5] = overview.headshots;
    for (i = 0; i < 6; i++) {
        float x = PAUSE_STAT_LEFT + (float)(i & 1) * PAUSE_STAT_COLUMN;
        float row = (float)(i >> 1) * PAUSE_STAT_PITCH;

        if (i == 0)
            CG_OverviewRatio(overview.kills, overview.deaths, value, sizeof(value));
        else
            snprintf(value, sizeof(value), "%d", values[i] < 0 ? 0 : values[i]);
        Pause_Text(s_pauseStats[i], 0, x, PAUSE_STAT_LABEL_BASELINE + row, PAUSE_ALIGN_LEFT, PAUSE_STAT_LABEL_CAP,
                   0.04f, s_pauseStatLabel);
        Pause_Text(value, 1, x - PAUSE_STAT_VALUE_INSET, PAUSE_STAT_VALUE_BASELINE + row, PAUSE_ALIGN_LEFT,
                   PAUSE_STAT_VALUE_CAP, 0.02f, s_pauseValue);
    }

    CG_DrawMinimapOverview(PAUSE_MAP_LEFT, PAUSE_MAP_TOP, PAUSE_MAP_WIDTH, PAUSE_MAP_HEIGHT, PAUSE_ALIGN_LEFT,
                           PAUSE_ALIGN_TOP, 1.0f);
}

static void Pause_TeamTile(displayContextDef_t *dc, int index, const char *title, float x, float y, float height,
                           int open, MaterialHandle emblems, float s0)
{
    float titleCap = index == s_pause.teamFocus ? PAUSE_TEAM_TITLE_FOCUS_CAP : PAUSE_TEAM_TITLE_CAP;
    float bottom = y + height;
    int focused = index == s_pause.teamFocus;
    char count[16];

    if (Pause_Hover(dc, x, y, PAUSE_TEAM_SIZE, height, PAUSE_ALIGN_LEFT))
        s_pause.hover = PAUSE_HOVER(PAUSE_HOVER_TILE, index);
    Pause_Fill(x, y, PAUSE_TEAM_SIZE, height, PAUSE_ALIGN_LEFT, focused ? s_pauseTeamFocus : s_pauseTeamTile);
    Pause_Frame(x, y, PAUSE_TEAM_SIZE, height, PAUSE_ALIGN_LEFT, s_pauseTeamEdge);
    if (emblems) {
        float offset = 0.5f * (PAUSE_TEAM_SIZE - PAUSE_TEAM_EMBLEM);

        CL_DrawStretchPic(x + offset, y + offset, PAUSE_TEAM_EMBLEM, PAUSE_TEAM_EMBLEM, PAUSE_ALIGN_LEFT,
                          PAUSE_ALIGN_TOP, s0, 0.0f, s0 + 0.5f, 1.0f, focused ? s_pauseEmblemFocus : s_pauseEmblem,
                          emblems);
    }
    Pause_Text(title, 1, x + PAUSE_TEAM_PAD, y + PAUSE_TEAM_TITLE_BASELINE, PAUSE_ALIGN_LEFT, titleCap, 0.03f,
               UI_WWIIMenu_Color(WWII_COLOR_WHITE));
    snprintf(count, sizeof(count), "%d", open);
    Pause_Text(count, 1, x + PAUSE_TEAM_PAD, bottom - PAUSE_TEAM_OPEN_ABOVE - PAUSE_TEAM_COUNT_ABOVE, PAUSE_ALIGN_LEFT,
               PAUSE_TEAM_COUNT_CAP, 0.02f, UI_WWIIMenu_Color(WWII_COLOR_WHITE));
    Pause_Text("Slots Open", 1, x + PAUSE_TEAM_PAD, bottom - PAUSE_TEAM_OPEN_ABOVE, PAUSE_ALIGN_LEFT,
               PAUSE_TEAM_OPEN_CAP, 0.0f, s_pauseLight);
}

static void Pause_PaintTeam(displayContextDef_t *dc)
{
    CG_TeamSlots slots;
    MaterialHandle emblems;
    int allies;

    CG_OverviewTeamSlots(&slots);
    emblems = CG_OverviewEmblems(PAUSE_TEAM_EMBLEM, &allies);
    Pause_TeamTile(dc, PAUSE_TEAM_ALLIES, "ALLIES", PAUSE_TEAM_LEFT, PAUSE_TEAM_TOP, PAUSE_TEAM_SIZE, slots.alliesOpen,
                   allies >= 0 ? emblems : NULL, 0.0f);
    Pause_TeamTile(dc, PAUSE_TEAM_AXIS, "AXIS", PAUSE_TEAM_LEFT + PAUSE_TEAM_SIZE + PAUSE_TEAM_GAP, PAUSE_TEAM_TOP,
                   PAUSE_TEAM_SIZE, slots.axisOpen, emblems, 0.5f);
    Pause_TeamTile(dc, PAUSE_TEAM_AUTO, "AUTO-ASSIGN", PAUSE_TEAM_LEFT, PAUSE_TEAM_TOP + PAUSE_TEAM_SIZE + PAUSE_TEAM_GAP,
                   PAUSE_TEAM_AUTO_HEIGHT, slots.open, NULL, 0.0f);
}

static void Pause_Wrapped(const char *text, float x, float baseline, float width, float cap, float tracking,
                          float spacing, const vec_t *color);

/* A choice: framed with its name in gold Bold where focused, the one in
 * play now tagged at the right. */
static void Pause_Choice(displayContextDef_t *dc, const char *name, int index, float top, int focused, int current)
{
    float middle = top + 0.5f * PAUSE_VOTE_CHOICE_HEIGHT;
    float cap = focused ? PAUSE_VOTE_CHOICE_FOCUS_CAP : PAUSE_VOTE_CHOICE_CAP;

    if (Pause_Hover(dc, PAUSE_VOTE_PANEL_LEFT, top, PAUSE_VOTE_PANEL_WIDTH, PAUSE_VOTE_CHOICE_HEIGHT, PAUSE_ALIGN_LEFT))
        s_pause.hover = PAUSE_HOVER(PAUSE_HOVER_CHOICE, index);
    if (focused)
        Pause_Frame(PAUSE_VOTE_PANEL_LEFT, top, PAUSE_VOTE_PANEL_WIDTH, PAUSE_VOTE_CHOICE_HEIGHT, PAUSE_ALIGN_LEFT,
                    s_pauseFrame);
    Pause_Text(name, focused, PAUSE_VOTE_PANEL_LEFT + PAUSE_VOTE_CHOICE_PAD, middle + 0.5f * cap, PAUSE_ALIGN_LEFT, cap,
               0.09f, focused ? s_pauseGold : s_pauseIdle);
    if (current)
        Pause_Text("NOW", 0,
                   PAUSE_VOTE_PANEL_LEFT + PAUSE_VOTE_PANEL_WIDTH - PAUSE_VOTE_CHOICE_PAD -
                       Pause_Width("NOW", 0, PAUSE_VOTE_TAG_CAP, 0.1f),
                   middle + 0.5f * PAUSE_VOTE_TAG_CAP, PAUSE_ALIGN_LEFT, PAUSE_VOTE_TAG_CAP, 0.1f, s_pauseNote);
}

/* While a vote runs: Yes and No to choose, else why not; at the right what
 * is voted on, the seconds left and both counts, as the overview's stats. */
static void Pause_PaintBallot(displayContextDef_t *dc, const CG_Vote *vote)
{
    static const char *const labels[3] = { "TIME LEFT", "YES", "NO" };
    float rowBaseline = PAUSE_SETTINGS_TOP + 0.5f * PAUSE_ROW_HEIGHT + 0.5f * s_pauseSettingsRows.cap;
    char number[16];
    int values[3];
    int i;

    if (!vote->voted && !vote->spectator) {
        s_pause.ballotRows = 1;
        for (i = 0; i < PAUSE_BALLOT_COUNT; i++)
            Pause_Row(dc, s_pauseBallots[i], i, PAUSE_SETTINGS_TOP + (float)i * PAUSE_ROW_PITCH, PAUSE_LIST_RIGHT,
                      i == s_pause.ballotFocus, &s_pauseSettingsRows);
    } else {
        Pause_Text(vote->spectator ? "Spectators cannot vote." : "Your vote is in.", 0, PAUSE_ROW_LEFT, rowBaseline,
                   PAUSE_ALIGN_LEFT, s_pauseSettingsRows.cap, 0.04f, s_pauseLight);
    }
    Pause_Text("VOTE IN PROGRESS", 1, PAUSE_VOTE_PANEL_LEFT, PAUSE_SETTINGS_TITLE_BASELINE, PAUSE_ALIGN_LEFT,
               PAUSE_SETTINGS_TITLE_CAP, PAUSE_SETTINGS_TITLE_TRACKING, s_pauseGold);
    Pause_Wrapped(vote->text, PAUSE_VOTE_PANEL_LEFT, PAUSE_SETTINGS_NOTE_BASELINE + 4.0f, PAUSE_VOTE_PANEL_WIDTH,
                  PAUSE_VOTE_TEXT_CAP, 0.04f, PAUSE_SETTINGS_NOTE_LINE, s_pauseBright);
    values[0] = vote->secondsLeft;
    values[1] = vote->yes;
    values[2] = vote->no;
    for (i = 0; i < 3; i++) {
        float x = PAUSE_VOTE_PANEL_LEFT + (float)i * PAUSE_VOTE_STAT_COLUMN;

        snprintf(number, sizeof(number), "%d", values[i]);
        Pause_Text(labels[i], 0, x, PAUSE_VOTE_STAT_LABEL_BASELINE, PAUSE_ALIGN_LEFT, PAUSE_STAT_LABEL_CAP, 0.04f,
                   s_pauseStatLabel);
        Pause_Text(number, 1, x - PAUSE_STAT_VALUE_INSET, PAUSE_VOTE_STAT_VALUE_BASELINE, PAUSE_ALIGN_LEFT,
                   PAUSE_STAT_VALUE_CAP, 0.02f, s_pauseValue);
    }
    Pause_Wrapped(PAUSE_VOTE_RULE, PAUSE_VOTE_PANEL_LEFT, PAUSE_VOTE_RULE_BASELINE, PAUSE_VOTE_PANEL_WIDTH,
                  PAUSE_SETTINGS_NOTE_CAP, PAUSE_SETTINGS_NOTE_TRACKING, PAUSE_SETTINGS_NOTE_LINE, s_pauseNote);
}

/* The actions as the settings list, the rule under them; at the right the
 * focused one's name, note and choices, twelve rows about the focused one. */
static void Pause_PaintVote(displayContextDef_t *dc)
{
    CG_Vote vote;
    int first = 0;
    int i;

    s_pause.ballotRows = 0;
    if (!CG_OverviewVote(&vote))
        return;
    if (vote.active || vote.spectator)
        s_pause.voteColumn = 0;
    if (vote.active) {
        Pause_PaintBallot(dc, &vote);
        return;
    }
    if (vote.spectator) {
        Pause_Text("Join a team to call a vote.", 0, PAUSE_ROW_LEFT,
                   PAUSE_SETTINGS_TOP + 0.5f * PAUSE_ROW_HEIGHT + 0.5f * s_pauseSettingsRows.cap, PAUSE_ALIGN_LEFT,
                   s_pauseSettingsRows.cap, 0.04f, s_pauseLight);
        return;
    }
    for (i = 0; i < PAUSE_VOTE_COUNT; i++)
        Pause_Row(dc, s_pauseVotes[i].name, i, PAUSE_SETTINGS_TOP + (float)i * PAUSE_ROW_PITCH, PAUSE_LIST_RIGHT,
                  i == s_pause.voteFocus, &s_pauseSettingsRows);
    Pause_Wrapped(PAUSE_VOTE_RULE, PAUSE_ROW_LEFT, PAUSE_SETTINGS_TOP + (float)PAUSE_VOTE_COUNT * PAUSE_ROW_PITCH + 12.0f,
                  PAUSE_VOTE_RULE_WIDTH, PAUSE_VOTE_RULE_CAP, 0.0f, PAUSE_SETTINGS_NOTE_LINE, s_pauseNote);
    Pause_Text(s_pauseVotes[s_pause.voteFocus].name, 1, PAUSE_VOTE_PANEL_LEFT, PAUSE_SETTINGS_TITLE_BASELINE,
               PAUSE_ALIGN_LEFT, PAUSE_SETTINGS_TITLE_CAP, PAUSE_SETTINGS_TITLE_TRACKING, s_pauseGold);
    Pause_Wrapped(s_pauseVotes[s_pause.voteFocus].note, PAUSE_VOTE_PANEL_LEFT, PAUSE_SETTINGS_NOTE_BASELINE,
                  PAUSE_VOTE_PANEL_WIDTH, PAUSE_SETTINGS_NOTE_CAP, PAUSE_SETTINGS_NOTE_TRACKING, PAUSE_SETTINGS_NOTE_LINE,
                  s_pauseNoteText);
    Pause_BuildVoteChoices(s_pause.voteFocus, &vote);
    if (!s_voteChoices.count) {
        s_pause.voteColumn = 0;
        if (s_pause.voteFocus == PAUSE_VOTE_KICK)
            Pause_Text("No one else is in the match.", 0, PAUSE_VOTE_PANEL_LEFT,
                       PAUSE_VOTE_CHOICE_TOP + 0.5f * (PAUSE_VOTE_CHOICE_HEIGHT + PAUSE_VOTE_CHOICE_CAP), PAUSE_ALIGN_LEFT,
                       PAUSE_VOTE_CHOICE_CAP, 0.04f, s_pauseNote);
        return;
    }
    if (s_pause.voteChoice >= s_voteChoices.count)
        s_pause.voteChoice = s_voteChoices.count - 1;
    if (s_pause.voteColumn && s_voteChoices.count > PAUSE_VOTE_CHOICE_ROWS) {
        first = s_pause.voteChoice - PAUSE_VOTE_CHOICE_ROWS / 2;
        if (first > s_voteChoices.count - PAUSE_VOTE_CHOICE_ROWS)
            first = s_voteChoices.count - PAUSE_VOTE_CHOICE_ROWS;
        if (first < 0)
            first = 0;
    }
    for (i = first; i < s_voteChoices.count && i < first + PAUSE_VOTE_CHOICE_ROWS; i++)
        Pause_Choice(dc, s_voteChoices.names[i], i, PAUSE_VOTE_CHOICE_TOP + (float)(i - first) * PAUSE_VOTE_CHOICE_PITCH,
                     s_pause.voteColumn && i == s_pause.voteChoice, i == s_voteChoices.current);
}

static void Pause_PaintLeave(displayContextDef_t *dc)
{
    int i;

    Pause_Text("Are you sure you want to leave the match?", 1, PAUSE_QUESTION_LEFT, PAUSE_QUESTION_BASELINE,
               PAUSE_ALIGN_LEFT, PAUSE_QUESTION_CAP, PAUSE_QUESTION_TRACKING, s_pauseLight);
    for (i = 0; i < PAUSE_LEAVE_COUNT; i++)
        Pause_Row(dc, s_pauseLeave[i], i, PAUSE_LEAVE_TOP + (float)i * PAUSE_ROW_PITCH, PAUSE_LIST_RIGHT,
                  i == s_pause.leaveFocus, &s_pauseLeaveRows);
}

/* Words across lines no wider than width, spacing capitals apart. */
static void Pause_Wrapped(const char *text, float x, float baseline, float width, float cap, float tracking,
                          float spacing, const vec_t *color)
{
    char line[256];
    int length = 0;

    while (*text) {
        const char *word = text;
        int wordLength = 0;

        while (word[wordLength] && word[wordLength] != ' ')
            wordLength++;
        if (length + 1 + wordLength >= (int)sizeof(line))
            break;
        if (length) {
            char trial[256];

            memcpy(trial, line, (size_t)length);
            trial[length] = ' ';
            memcpy(trial + length + 1, word, (size_t)wordLength);
            trial[length + 1 + wordLength] = '\0';
            if (Pause_Width(trial, 0, cap, tracking) > width) {
                Pause_Text(line, 0, x, baseline, PAUSE_ALIGN_LEFT, cap, tracking, color);
                baseline += cap * spacing;
                length = 0;
            }
        }
        if (length)
            line[length++] = ' ';
        memcpy(line + length, word, (size_t)wordLength);
        length += wordLength;
        line[length] = '\0';
        text = word + wordLength;
        while (*text == ' ')
            text++;
    }
    if (length)
        Pause_Text(line, 0, x, baseline, PAUSE_ALIGN_LEFT, cap, tracking, color);
}

static void Pause_PaintSettings(displayContextDef_t *dc)
{
    int i;

    for (i = 0; i < PAUSE_SETTINGS_COUNT; i++)
        Pause_Row(dc, s_pauseSettings[i].name, i, PAUSE_SETTINGS_TOP + (float)i * PAUSE_ROW_PITCH, PAUSE_LIST_RIGHT,
                  i == s_pause.settingsFocus, &s_pauseSettingsRows);
    Pause_Text(s_pauseSettings[s_pause.settingsFocus].name, 1, PAUSE_SETTINGS_TEXT_LEFT, PAUSE_SETTINGS_TITLE_BASELINE,
               PAUSE_ALIGN_LEFT, PAUSE_SETTINGS_TITLE_CAP, PAUSE_SETTINGS_TITLE_TRACKING, s_pauseGold);
    Pause_Wrapped(s_pauseSettings[s_pause.settingsFocus].note, PAUSE_SETTINGS_TEXT_LEFT, PAUSE_SETTINGS_NOTE_BASELINE,
                  PAUSE_SETTINGS_NOTE_WIDTH, PAUSE_SETTINGS_NOTE_CAP, PAUSE_SETTINGS_NOTE_TRACKING,
                  PAUSE_SETTINGS_NOTE_LINE, s_pauseNoteText);
}

/* A prompt from x: the controller's colored button, else the keyboard's
 * keycap, and its label; its width. */
static float Pause_Prompt(float x, int horz, const char *key, const char *button, const vec_t *buttonColor,
                          const char *label, int gamepad)
{
    float middle = 480.0f - PAUSE_PROMPT_MIDDLE;
    float height = gamepad ? PAUSE_PROMPT_GLYPH : PAUSE_PROMPT_KEYCAP;
    float keycap = UI_WWIIMenu_Keycap(gamepad ? button : key, x, middle - 0.5f * height, height, horz,
                                      PAUSE_ALIGN_TOP, gamepad ? buttonColor : NULL);

    Pause_Text(label, 0, x + keycap + PAUSE_PROMPT_GAP, 480.0f - PAUSE_PROMPT_LABEL_BASELINE, horz,
               PAUSE_PROMPT_LABEL_CAP, 0.0f, s_pauseBright);
    return keycap + PAUSE_PROMPT_GAP + Pause_Width(label, 0, PAUSE_PROMPT_LABEL_CAP, 0.0f);
}

/* The prompt's width without drawing it, to set one against the right edge. */
static float Pause_PromptWidth(const char *key, const char *label, int gamepad)
{
    return UI_WWIIMenu_KeycapWidth(key, gamepad ? PAUSE_PROMPT_GLYPH : PAUSE_PROMPT_KEYCAP) + PAUSE_PROMPT_GAP +
           Pause_Width(label, 0, PAUSE_PROMPT_LABEL_CAP, 0.0f);
}

/* As the captures: Select on every tab with something to choose, Back on
 * the Class tab and out of a vote's choices, and the class editor for the
 * player's own classes. */
static void Pause_PaintPrompts(int gamepad)
{
    float x = PAUSE_PROMPT_LEFT;

    if (s_pause.tab != PAUSE_TAB_OVERVIEW)
        x += Pause_Prompt(x, PAUSE_ALIGN_LEFT, "ENTER", "A", s_pauseGlyphA, "Select", gamepad) + PAUSE_PROMPT_SPACING;
    if (s_pause.tab == PAUSE_TAB_CLASS && !s_pause.standard && !gamepad)
        Pause_Prompt(x, PAUSE_ALIGN_LEFT, "F1", "", NULL, "Edit", 0);
    if (s_pause.tab == PAUSE_TAB_CLASS || (s_pause.tab == PAUSE_TAB_VOTE && s_pause.voteColumn))
        Pause_Prompt(-PAUSE_PROMPT_RIGHT - Pause_PromptWidth(gamepad ? "B" : "ESC", "Back", gamepad), PAUSE_ALIGN_RIGHT,
                     "ESC", "B", s_pauseGlyphB, "Back", gamepad);
}

void UI_PauseMenu_Paint(displayContextDef_t *dc, menuDef_t *menu)
{
    float wash;
    vec4_t shade;
    int gamepad = Pause_Gamepad();
    int moved;

    if (!Pause_IsMenu(menu))
        return;
    if (!s_pause.open)
        UI_PauseMenu_Open(dc);
    Pause_Tabs();
    if (Pause_TabIndex(s_pause.tab) < 0)
        s_pause.tab = PAUSE_TAB_CLASS;
    CG_OverviewRequestScores();
    /* Over the blurred view, a wash that fades in with the blur. */
    wash = Pause_Clamp01((float)(dc->realTime - s_pause.openTime) / (float)PAUSE_WASH_MS);
    memcpy(shade, s_pauseBlack, sizeof(shade));
    shade[3] = PAUSE_WASH * wash;
    CL_DrawStretchPic(0.0f, 0.0f, 640.0f, 480.0f, PAUSE_ALIGN_FULLSCREEN, PAUSE_ALIGN_FULLSCREEN, 0.0f, 0.0f, 1.0f,
                      1.0f, shade, sharedUiInfo.assets.whiteMaterial);
    moved = dc->cursorx != s_pause.cursorX || dc->cursory != s_pause.cursorY;
    s_pause.cursorX = dc->cursorx;
    s_pause.cursorY = dc->cursory;
    s_pause.hover = PAUSE_HOVER_NONE;
    Pause_PaintTabs(dc, gamepad);
    switch (s_pause.tab) {
    case PAUSE_TAB_OVERVIEW:
        Pause_PaintOverview();
        break;
    case PAUSE_TAB_TEAM:
        Pause_PaintTeam(dc);
        break;
    case PAUSE_TAB_VOTE:
        Pause_PaintVote(dc);
        break;
    case PAUSE_TAB_LEAVE:
        Pause_PaintLeave(dc);
        break;
    case PAUSE_TAB_SETTINGS:
        Pause_PaintSettings(dc);
        break;
    default:
        Pause_PaintClass(dc);
        break;
    }
    /* Pointer motion moves the focus to what it is over now, with the retail
     * menus' mouse_over as it enters another element; keys move it otherwise. */
    if (moved && (s_pause.hover >> 8) != PAUSE_HOVER_TOGGLE) {
        int focus[7] = { s_pause.classFocus, s_pause.teamFocus, s_pause.leaveFocus, s_pause.settingsFocus,
                         s_pause.voteFocus, s_pause.voteChoice, s_pause.ballotFocus };

        Pause_FocusHover();
        if (focus[0] != s_pause.classFocus || focus[1] != s_pause.teamFocus || focus[2] != s_pause.leaveFocus ||
            focus[3] != s_pause.settingsFocus || focus[4] != s_pause.voteFocus || focus[5] != s_pause.voteChoice ||
            focus[6] != s_pause.ballotFocus)
            Pause_Sound("mouse_over");
    }
    Pause_PaintPrompts(gamepad);
}
