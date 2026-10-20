#include "common_types.h"
#include "imports.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include "headers/PC/ui_mp/ui_wwii_menu.h"
#include "headers/PC/ui_mp/ui_wwii_settings.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern sharedUiInfo_t sharedUiInfo;
extern const char *UI_SafeTranslateString(const char *reference);
extern const char *Dvar_GetVariantString(const char *dvarName);
extern void Dvar_SetFromStringByName(const char *dvarName, const char *string);
extern char *va(const char *format, ...);
extern int I_stricmp(const char *s0, const char *s1);
extern qboolean Item_EnableShowViaDvar(const itemDef_t *item, int flag);
extern editFieldDef_t *Item_GetEditFieldDef(itemDef_t *item);
extern qboolean Item_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key, qboolean down);
extern qboolean Item_Bind_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key, qboolean down);
extern qboolean Item_TextField_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key);
extern void Item_RunScript(displayContextDef_t *dc, itemDef_t *item, const char *s);
extern qboolean Display_KeyBindPending(void);
extern void Menus_CloseByName(displayContextDef_t *dc, const char *p);
extern const char *Item_ValueText(itemDef_t *item);
extern void Item_BindText(itemDef_t *item, char *text, int size);
extern void Item_EditField_Begin(itemDef_t *item);
extern itemDef_t *Item_EditField_Current(void);
extern void Item_EditField_End(void);
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1,
                              float s2, float t2, const vec_t *color, MaterialHandle material);
extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);
extern void CalcScreenX(float *x, int horzAlign);
extern void CalcScreenY(float *y, int vertAlign);
extern float GetRealWidthFromVirtualWidth(float width);

/* ui/menudefinition.h's alignments, item types, window flags and the keys. */
#define SET_ALIGN_LEFT 1
#define SET_ALIGN_TOP 1
#define SET_ALIGN_RIGHT 3
#define SET_ALIGN_FULLSCREEN 4
#define SET_ITEM_TEXT 0
#define SET_ITEM_BUTTON 1
#define SET_ITEM_EDITFIELD 4
#define SET_ITEM_NUMERICFIELD 9
#define SET_ITEM_SLIDER 10
#define SET_ITEM_YESNO 11
#define SET_ITEM_MULTI 12
#define SET_ITEM_DVARENUM 13
#define SET_ITEM_BIND 14
#define SET_ITEM_VALIDFILEFIELD 16
#define SET_ITEM_DECIMALFIELD 17
#define SET_ITEM_UPREDITFIELD 18
#define SET_WINDOW_HASFOCUS 2
#define SET_WINDOW_VISIBLE 4
#define SET_WINDOW_DECORATION 0x100000
/* dvarFlags: enableDvar and disableDvar, then showDvar and hideDvar. */
#define SET_DVAR_ENABLE 1
#define SET_DVAR_ENABLE_MASK 3
#define SET_DVAR_SHOW 4
#define SET_DVAR_SHOW_MASK 0xc
#define SET_KEY_TAB 0x09
#define SET_KEY_ENTER 0x0d
#define SET_KEY_ESCAPE 0x1b
#define SET_KEY_UP 0x9a
#define SET_KEY_DOWN 0x9b
#define SET_KEY_LEFT 0x9c
#define SET_KEY_RIGHT 0x9d
#define SET_KEY_PGDN 0xa3
#define SET_KEY_PGUP 0xa4
#define SET_KEY_KP_ENTER 0xbf
#define SET_KEY_MOUSE1 0xc8
#define SET_KEY_MOUSE2 0xc9
#define SET_KEY_WHEEL_DOWN 0xcd
#define SET_KEY_WHEEL_UP 0xce
#define SET_KEY_CHAR 0x400

/* Units from the screen's edges, measured on the four settings captures at
 * 1024 x 576, 1.2 px a unit. The title's capitals are 11.2 tall on a
 * baseline 69.2 down, its S inked from 45.0. Names are capitals 7.5 tall,
 * inked from 53.6, on baselines 28.9 apart from 106.5 down; eleven show at
 * once. The focused one is Bold and 8.8 further right. */
#define SET_TITLE_X 44.5f
#define SET_TITLE_BASELINE 69.2f
#define SET_TITLE_CAP 11.2f
#define SET_TITLE_TRACKING 0.235f
#define SET_TITLE "SETTINGS"
#define SET_TITLE_SEPARATOR " // "
#define SET_ROW_X 53.4f
#define SET_ROW_TOP 106.5f
#define SET_ROW_SPACING 28.9f
#define SET_ROWS_SHOWN 11
#define SET_LABEL_CAP 7.5f
#define SET_LABEL_TRACKING 0.05f
#define SET_FOCUS_TRACKING 0.085f
#define SET_FOCUS_INDENT 8.8f
/* The main menu's focus: a slide over 233 ms and the gold within 100. */
#define SET_SLIDE_MS 233.0f
#define SET_GROW_MS 100.0f
/* The focus is a thin gold frame from the left edge, its lines' middles 17.8
 * above the baseline and 8.9 below it, to 399.6 across on a page and 266.1
 * on the list. */
#define SET_BOX_ABOVE 18.2f
#define SET_BOX_HEIGHT 27.5f
#define SET_BOX_LINE 0.8f
#define SET_BOX_RIGHT 400.0f
#define SET_LIST_BOX_RIGHT 266.5f
/* Values are centred on 310.8, in Regular capitals 7.2 tall, no wider than
 * the gap between the arrows. A slider's bar spans 243.8 to 377.8, from 9.2
 * above the baseline to 1.15 below it; the focused row's arrows are 5.9
 * wide, 3.2 outside the bar, and take clicks 3 around them. */
#define SET_VALUE_CENTER 310.8f
#define SET_VALUE_CAP 7.2f
#define SET_VALUE_ROOM 136.0f
#define SET_SLIDER_LEFT 243.8f
#define SET_SLIDER_WIDTH 134.0f
#define SET_SLIDER_ABOVE 9.2f
#define SET_SLIDER_HEIGHT 10.35f
#define SET_ARROW_WIDTH 5.9f
#define SET_ARROW_GAP 3.2f
#define SET_ARROW_HIT 3.0f
#define SET_ARROW_LEFT (SET_SLIDER_LEFT - SET_ARROW_GAP - SET_ARROW_WIDTH)
#define SET_ARROW_RIGHT (SET_SLIDER_LEFT + SET_SLIDER_WIDTH + SET_ARROW_GAP)
#define SET_DISPLAY_X (SET_ARROW_RIGHT + SET_ARROW_WIDTH + 3.0f)
#define SET_LABEL_GAP 2.0f
#define SET_CARET_WIDTH 0.6f
/* The explanation: its title in Bold capitals 10.25 tall on the first row's
 * baseline, then lines 10.5 apart from 121.2 down, from 410.5 to 40 short of
 * the right edge; notes smaller under it. */
#define SET_PANEL_X 410.5f
#define SET_PANEL_RIGHT 40.0f
#define SET_PANEL_TITLE_CAP 10.25f
#define SET_PANEL_TITLE_TRACKING 0.035f
#define SET_PANEL_TEXT_CAP 7.2f
#define SET_PANEL_TEXT_BASELINE 121.2f
#define SET_PANEL_LINE 10.5f
#define SET_PANEL_NOTE_CAP 6.0f
#define SET_PANEL_GAP 5.0f
/* Brightness's five roundels: 55.8 across, 74.15 apart from 461.7, their
 * middles 239.2 down. */
#define SET_ROUNDEL_FIRST 461.7f
#define SET_ROUNDEL_STEP 74.15f
#define SET_ROUNDEL_SIZE 55.8f
#define SET_ROUNDEL_Y 239.2f
#define SET_ROUNDEL_COUNT 5
/* A thin scrollbar right of the frame when there are more rows than show. */
#define SET_SCROLL_GAP 4.0f
#define SET_SCROLL_WIDTH 0.8f
/* What is behind the pages is all but black in the captures. */
#define SET_PAGE_SHADE 0.88f
#define SET_LIST_SHADE 0.75f

/* Runtime images in texels: the arrow, and a roundel's disk, inner ring, rim
 * and star as fractions of its radius. */
#define SET_ARROW_IMAGE 32
#define SET_ROUNDEL_IMAGE 64
#define SET_IMAGE_SAMPLES 4
#define SET_ROUNDEL_DISK 0.93f
#define SET_ROUNDEL_RIM 0.035f
#define SET_ROUNDEL_RING 0.80f
#define SET_ROUNDEL_RING_HALF 0.025f
#define SET_ROUNDEL_STAR_OUTER 0.72f
#define SET_ROUNDEL_STAR_INNER 0.275f

#define SET_MAX_PAGES 16
#define SET_MAX_ROWS 48
#define SET_MAX_NOTES 8
#define SET_ROW_TOLERANCE 4.0f
#define SET_UNTABBED 1000
#define SET_TEXT_MAX 256
#define SET_NOTES_MAX 640
/* The browser's controller page shows one tab at a time by this dvar; here
 * its tabs make one list. */
#define SET_TAB_DVAR "ui_gamepadPage"
#define SET_LIST_MENU "options_menu"

enum { SET_KIND_BUTTON, SET_KIND_VALUE, SET_KIND_SLIDER, SET_KIND_BIND, SET_KIND_FIELD };
enum { SET_PART_ROW, SET_PART_LEFT, SET_PART_RIGHT, SET_PART_BAR };

/* Measured inside the title's strokes: the gold of the title, the focused
 * name and the explanation's title, #b6a789; its frame is that gold at 55%.
 * Values are a darker gray than names, the explanation #ccc7c3 and the
 * slider's bar #d5cfbe over a 45% black groove. */
static const vec4_t s_setGold = { 0.714f, 0.655f, 0.537f, 1.0f };
static const vec4_t s_setFrame = { 0.714f, 0.655f, 0.537f, 0.55f };
static const vec4_t s_setValue = { 0.47f, 0.47f, 0.47f, 1.0f };
static const vec4_t s_setText = { 0.80f, 0.78f, 0.765f, 1.0f };
static const vec4_t s_setNote = { 0.62f, 0.61f, 0.59f, 1.0f };
static const vec4_t s_setDisabled = { 0.36f, 0.36f, 0.36f, 1.0f };
static const vec4_t s_setBar = { 0.835f, 0.81f, 0.745f, 1.0f };
static const vec4_t s_setGroove = { 0.0f, 0.0f, 0.0f, 0.45f };
static const vec4_t s_setArrow = { 0.835f, 0.82f, 0.78f, 1.0f };
static const vec4_t s_setScroll = { 1.0f, 1.0f, 1.0f, 0.12f };
static const vec4_t s_setWhite = { 1.0f, 1.0f, 1.0f, 1.0f };
/* The roundels' grays, darkest first, over the disk's 0.93: the capture's 12
 * to 75 of 255. */
static const float s_setRoundels[SET_ROUNDEL_COUNT] = { 0.051f, 0.097f, 0.164f, 0.232f, 0.316f };

/* The settings pages Options opens. */
static const char *const s_setPageNames[] = {
    "options_look", "options_move", "options_shoot", "options_misc", "options_multi", "options_graphics",
    "options_graphics_texture", "options_sound", "options_voice", "options_game", "options_control_defaults",
    "options_graphics_defaults", "browser_gamepad",
};

/* What each setting does, under its name as WWII's settings explain theirs:
 * by the command or dvar a row sets, else by its menu and text. */
static const struct {
    const char *key;
    const char *text;
} s_setDescriptions[] = {
    { "options_menu/@MENU_LOOK", "Set how the mouse aims and turns your view." },
    { "options_menu/@MENU_MOVE", "Choose the keys you move, lean and change stance with." },
    { "options_menu/@MENU_SHOOT", "Choose the keys you fire, aim, reload and throw grenades with." },
    { "options_menu/@MENU_INTERACT", "Choose the keys for the scoreboard, using objects and screenshots." },
    { "options_menu/@MENU_MULTIPLAYER", "Set up voting, chat, your connection and the HUD." },
    { "options_menu/@MENU_SET_DEFAULT_CONTROLS", "Restore every control to its default key." },
    { "options_menu/@MENU_GRAPHICS", "Adjust the resolution, image quality and brightness." },
    { "options_menu/@MENU_SOUND", "Adjust the volume and quality of the game's sound." },
    { "options_menu/@MENU_VOICECHAT", "Set up voice chat." },
    { "options_menu/Controller", "Set up a gamepad: its sensitivity, dead zone, vibration, aim assist and buttons." },
    { "options_menu/@MENU_GAME_OPTIONS", "Change your gametag and set the crosshair, blood and console." },
    { "options_menu/@MENU_RESET_SYSTEM_DEFAULTS",
      "Restore the graphics and sound settings best suited to your system." },
    { "ui_mousePitch", "Reverse the mouse's vertical axis: pushing it forward looks down." },
    { "+lookup", "Tilt the view up while the key is held." },
    { "+lookdown", "Tilt the view down while the key is held." },
    { "+mlook", "Look around with the mouse while the key is held, when Free Look is off." },
    { "centerview", "Level your view." },
    { "cl_freelook", "Look around with the mouse at all times." },
    { "m_filter", "Average the mouse's movement over two frames for smoother aiming." },
    { "sensitivity", "Adjust how far the view turns when the mouse moves." },
    { "+forward", "Move forward." },
    { "+back", "Move backward." },
    { "+moveleft", "Step to the left." },
    { "+moveright", "Step to the right." },
    { "+leanleft", "Lean left around cover while the key is held." },
    { "+leanright", "Lean right around cover while the key is held." },
    { "+gostand", "Stand up, or jump when already standing." },
    { "gocrouch", "Drop to a crouch, or rise to one from prone." },
    { "goprone", "Drop to the ground." },
    { "+moveup", "Jump when standing; otherwise rise one stance." },
    { "lowerstance", "Lower your stance one step: stand, crouch, prone." },
    { "raisestance", "Raise your stance one step: prone, crouch, stand." },
    { "togglecrouch", "Crouch, or stand up again." },
    { "toggleprone", "Go prone, or get up again." },
    { "+movedown", "Crouch while the key is held." },
    { "+prone", "Stay prone while the key is held." },
    { "+left", "Turn left while the key is held." },
    { "+right", "Turn right while the key is held." },
    { "+strafe", "Turn the left and right keys into side steps while held." },
    { "+attack", "Fire your weapon." },
    { "toggleads", "Raise or lower your sights." },
    { "+speed", "Aim down the sights while the key is held." },
    { "weapnext", "Switch to your other weapon." },
    { "+reload", "Reload your weapon." },
    { "+frag", "Hold to cook a frag grenade, then release to throw it." },
    { "+smoke", "Throw a smoke grenade." },
    { "+binoculars", "Raise your binoculars." },
    { "+sprint", "Sprint while moving forward, or hold your breath to steady a scoped rifle." },
    { "+melee", "Strike with your weapon." },
    { "+melee_breath", "Strike with your weapon, or hold your breath while scoped." },
    { "+holdbreath", "Hold your breath to steady a scoped rifle." },
    { "+scores", "Show the objectives and the scoreboard while the key is held." },
    { "+activate", "Use or pick up what is in front of you." },
    { "screenshotjpeg", "Save a screenshot." },
    { "vote yes", "Vote yes on the vote being called." },
    { "vote no", "Vote no on the vote being called." },
    { "chatmodepublic", "Type a message to every player." },
    { "chatmodeteam", "Type a message to your team." },
    { "mp_QuickMessage", "Open the quick messages." },
    { "openscriptmenu ingame changeweapon", "Open the class menu." },
    { "cl_allowdownload", "Download a server's missing files when joining it." },
    { "hud_enable", "Show the HUD." },
    { "rate", "Match the data the server sends to your connection." },
    { "ui_name", "The name other players see." },
    { "cl_punkbuster", "Play on servers that run PunkBuster." },
    { "ui_r_mode", "Choose the screen resolution. Apply to restart the renderer with it." },
    { "ui_r_displayrefresh", "Choose the screen's refresh rate. Apply to restart the renderer with it." },
    { "ui_r_aspectratio", "Match the picture to the shape of your screen. Apply to restart the renderer with it." },
    { "ui_r_texturemode", "Choose how textures are filtered: anisotropic stays sharpest at an angle." },
    { "ui_r_aasamples", "Smooth jagged edges. Higher settings cost more performance." },
    { "ui_r_rendererpreference", "Choose the DirectX version to render with." },
    { "r_gamma",
      "Adjust the image brightness to give a good range of contrast. The darkest star should remain visible." },
    { "ui_r_swapinterval", "Wait for the display before drawing each frame, which avoids tearing." },
    { "r_multiGpu", "Optimize rendering for two linked video cards." },
    { "ui_sc_enable", "Cast shadows from characters and objects." },
    { "r_dlightlimit", "Limit how many dynamic lights, such as muzzle flashes, light the scene." },
    { "ui_zfeather", "Soften the edges where smoke meets the world." },
    { "ai_corpsecount", "Choose how many bodies stay in the world." },
    { "options_graphics/@MENU_TEXTURE_SETTINGS", "Set the quality and resolution of textures." },
    { "options_graphics/@MENU_APPLY", "Restart the renderer with your changes." },
    { "ui_r_picmip_manual", "Let the game choose texture resolution, or set it yourself." },
    { "ui_r_picmip", "Choose the resolution of color textures." },
    { "ui_r_picmip_bump", "Choose the resolution of normal maps, which give surfaces their relief." },
    { "ui_r_picmip_spec", "Choose the resolution of specular maps, which give surfaces their shine." },
    { "options_graphics_texture/@MENU_APPLY", "Load textures at the new settings." },
    { "snd_volume", "Adjust the volume of every sound in the game." },
    { "ui_snd_khz", "Choose the sound quality. Apply to restart the sound with it." },
    { "ui_mss_3d_provider", "Choose how positional sound is mixed. Apply to restart the sound with it." },
    { "options_sound/@MENU_APPLY", "Restart the sound with your changes." },
    { "name", "The name other players see. Press ENTER to type a new one." },
    { "cg_subtitles", "Show subtitles for dialogue." },
    { "cg_drawcrosshair", "Show the crosshair while you are not aiming down the sights." },
    { "cg_blood", "Show blood when players are hit." },
    { "con_restricted", "Allow the console to be opened." },
    { "monkeytoy", "Allow the console to be opened." },
    { "ui_language", "Choose the game's language." },
    { "options_game/@MENU_APPLY_LANGUAGE_CHANGE", "Restart the game in the new language." },
    { "cl_gamepadEnable", "Play and move through the menus with a connected gamepad." },
    { "cl_gamepadLookSensitivity", "Adjust how fast the right stick turns the view." },
    { "cl_gamepadDeadzone", "Ignore small stick movements near the center." },
    { "cl_gamepadInvertY", "Reverse the right stick's vertical axis." },
    { "cl_gamepadRumble", "Feel every shot you fire, the hits you take and explosions nearby through the controller." },
    { "cl_gamepadAimAssist", "Slow the right stick slightly over an enemy and follow a little of their movement." },
    { "+usereload", "Use what is in front of you, or reload." },
    { "+stance", "Crouch; hold to go prone." },
    { "+breath_binoculars", "Raise your binoculars, or hold your breath while scoped." },
    { "togglemenu", "Open the pause menu." },
    { "browser_gamepad/Restore controller defaults", "Restore every controller button to its default." },
    { "options_control_defaults/@MENU_YES", "Restore every control to its default key." },
    { "options_control_defaults/@MENU_NO", "Keep your controls as they are." },
    { "options_graphics_defaults/@MENU_YES", "Restore the graphics and sound settings best suited to your system." },
    { "options_graphics_defaults/@MENU_NO", "Keep your settings as they are." },
};

typedef struct {
    /* A decoration naming the row, or NULL when its control names itself. */
    itemDef_t *label;
    /* A control or a button; a second one shares the row when a dvar shows
     * one of the two. */
    itemDef_t *controls[2];
    /* A decimal field showing a slider's value. */
    itemDef_t *display;
    int tab;
    float y;
    float x;
    /* Focus from 0 to 1, slid and grown at their own pace. */
    float slide;
    float grow;
    int time;
} SettingsRow;

typedef struct {
    menuDef_t *menu;
    int list;
    itemDef_t *header;
    /* The page's Back button, which Escape runs. */
    itemDef_t *back;
    SettingsRow rows[SET_MAX_ROWS];
    int rowCount;
    itemDef_t *notes[SET_MAX_NOTES];
    int noteCount;
    int focus;
    /* The first of the visible rows shown. */
    int scroll;
} SettingsPage;

static struct {
    const displayContextDef_t *dc;
    SettingsPage pages[SET_MAX_PAGES];
    int pageCount;
    SettingsPage *shown;
    /* The list has been on screen since Options opened: pages go back to it. */
    int listShown;
    int cursorX;
    int cursorY;
    /* The row under the pointer and the part of it, as last painted. */
    SettingsPage *hoverPage;
    int hoverRow;
    int hoverPart;
    int dragRow;
    itemDef_t *bindItem;
    int generation;
    int imagesBuilt;
    MaterialHandle arrow;
    MaterialHandle roundel;
} s_settings;

static float Settings_Clamp01(float value)
{
    return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
}

static float Settings_Lerp(float from, float to, float t)
{
    return from + (to - from) * t;
}

static float Settings_EaseOut(float t)
{
    t = Settings_Clamp01(t);
    return 1.0f - (1.0f - t) * (1.0f - t);
}

static byte Settings_Byte(float value)
{
    return (byte)(Settings_Clamp01(value) * 255.0f + 0.5f);
}

static int Settings_On(void)
{
    return UI_WWIIMenu_LayoutOn();
}

static const char *Settings_Translate(const char *text)
{
    if (!text)
        return "";
    return text[0] == '@' ? UI_SafeTranslateString(text + 1) : text;
}

/* Capitals of ASCII and Latin-1 letters. */
static void Settings_Upper(const char *text, char *out, int size)
{
    int i;

    for (i = 0; text[i] && i < size - 1; i++) {
        unsigned char c = (unsigned char)text[i];

        if ((c >= 'a' && c <= 'z') || (c >= 0xe0 && c <= 0xfe && c != 0xf7))
            c = (unsigned char)(c - 0x20);
        out[i] = (char)c;
    }
    out[i] = '\0';
}

static int Settings_Blank(const char *text)
{
    return !text || !text[strspn(text, " ")];
}

static float Settings_Number(const char *dvar)
{
    const char *value = dvar ? Dvar_GetVariantString(dvar) : NULL;

    return value ? (float)atof(value) : 0.0f;
}

/* The screen's width in units, edge to edge. */
static float Settings_ScreenWidth(void)
{
    float left = 0.0f, right = 0.0f, unit = GetRealWidthFromVirtualWidth(1.0f);

    CalcScreenX(&left, SET_ALIGN_LEFT);
    CalcScreenX(&right, SET_ALIGN_RIGHT);
    return unit > 0.0f ? (right - left) / unit : 640.0f;
}

static float Settings_Baseline(int shown)
{
    return SET_ROW_TOP + (float)shown * SET_ROW_SPACING;
}

static void Settings_Fill(float x, float y, float w, float h, const vec_t *color)
{
    CL_DrawStretchPic(x, y, w, h, SET_ALIGN_LEFT, SET_ALIGN_TOP, 0.0f, 0.0f, 1.0f, 1.0f, color,
                      sharedUiInfo.assets.whiteMaterial);
}

static void Settings_Text(const char *text, int bold, float x, float baseline, float cap, float tracking,
                          const vec_t *color)
{
    UI_WWIIMenu_Text(text, bold, x, baseline, SET_ALIGN_LEFT, SET_ALIGN_TOP, cap, tracking, color);
}

/* Whether the pointer is over the rectangle. */
static int Settings_Inside(const displayContextDef_t *dc, float x, float y, float w, float h)
{
    float cx = (float)dc->cursorx, cy = (float)dc->cursory;

    CalcScreenPlacement(&x, &y, &w, &h, SET_ALIGN_LEFT, SET_ALIGN_TOP);
    CalcScreenX(&cx, SET_ALIGN_FULLSCREEN);
    CalcScreenY(&cy, SET_ALIGN_FULLSCREEN);
    return cx >= x && cx < x + w && cy >= y && cy < y + h;
}

/* A right-pointing triangle filling the image but for a texel round it, in
 * white with its coverage as alpha. */
static void Settings_ArrowImage(byte *bgra)
{
    const float margin = 1.0f / (float)SET_ARROW_IMAGE;
    const int samples = SET_IMAGE_SAMPLES * SET_IMAGE_SAMPLES;
    int x, y, sx, sy;

    for (y = 0; y < SET_ARROW_IMAGE; y++) {
        for (x = 0; x < SET_ARROW_IMAGE; x++) {
            byte *texel = bgra + (y * SET_ARROW_IMAGE + x) * 4;
            int covered = 0;

            for (sy = 0; sy < SET_IMAGE_SAMPLES; sy++) {
                for (sx = 0; sx < SET_IMAGE_SAMPLES; sx++) {
                    float u = ((float)x + ((float)sx + 0.5f) / SET_IMAGE_SAMPLES) / SET_ARROW_IMAGE;
                    float v = ((float)y + ((float)sy + 0.5f) / SET_IMAGE_SAMPLES) / SET_ARROW_IMAGE;
                    float along = (u - margin) / (1.0f - 2.0f * margin);

                    if (along >= 0.0f && along <= 1.0f && fabsf(v - 0.5f) <= (0.5f - margin) * (1.0f - along))
                        covered++;
                }
            }
            texel[0] = texel[1] = texel[2] = 255;
            texel[3] = Settings_Byte((float)covered / samples);
        }
    }
}

/* Whether a point, in radii from the middle, is inside the five-pointed star
 * standing on two points. */
static int Settings_InStar(float x, float y)
{
    float points[10][2];
    int i, j, inside = 0;

    for (i = 0; i < 10; i++) {
        float angle = -1.57079633f + (float)i * 0.62831853f;
        float radius = (i & 1) ? SET_ROUNDEL_STAR_INNER : SET_ROUNDEL_STAR_OUTER;

        points[i][0] = radius * cosf(angle);
        points[i][1] = radius * sinf(angle);
    }
    for (i = 0, j = 9; i < 10; j = i++) {
        if ((points[i][1] > y) != (points[j][1] > y) &&
            x < (points[j][0] - points[i][0]) * (y - points[i][1]) / (points[j][1] - points[i][1]) + points[i][0])
            inside = !inside;
    }
    return inside;
}

/* A roundel as the brightness captures show them: a disk in light gray with
 * a white rim, inner ring and star, its coverage as alpha in straight alpha,
 * so a draw's color sets the disk's gray and the star 7% above it. */
static void Settings_RoundelImage(byte *bgra)
{
    const float half = 0.5f * (float)SET_ROUNDEL_IMAGE;
    const float radius = half - 1.0f;
    const int samples = SET_IMAGE_SAMPLES * SET_IMAGE_SAMPLES;
    int x, y, sx, sy;

    for (y = 0; y < SET_ROUNDEL_IMAGE; y++) {
        for (x = 0; x < SET_ROUNDEL_IMAGE; x++) {
            byte *texel = bgra + (y * SET_ROUNDEL_IMAGE + x) * 4;
            float sum = 0.0f;
            int covered = 0;

            for (sy = 0; sy < SET_IMAGE_SAMPLES; sy++) {
                for (sx = 0; sx < SET_IMAGE_SAMPLES; sx++) {
                    float px = ((float)x + ((float)sx + 0.5f) / SET_IMAGE_SAMPLES - half) / radius;
                    float py = ((float)y + ((float)sy + 0.5f) / SET_IMAGE_SAMPLES - half) / radius;
                    float distance = sqrtf(px * px + py * py);

                    if (distance > 1.0f)
                        continue;
                    covered++;
                    if (distance > 1.0f - SET_ROUNDEL_RIM || fabsf(distance - SET_ROUNDEL_RING) < SET_ROUNDEL_RING_HALF ||
                        Settings_InStar(px, py))
                        sum += 1.0f;
                    else
                        sum += SET_ROUNDEL_DISK;
                }
            }
            texel[0] = texel[1] = texel[2] = Settings_Byte(covered ? sum / (float)covered : 0.0f);
            texel[3] = Settings_Byte((float)covered / samples);
        }
    }
}

static void Settings_BuildImages(void)
{
    static byte texels[SET_ROUNDEL_IMAGE * SET_ROUNDEL_IMAGE * 4];

    if (s_settings.imagesBuilt)
        return;
    s_settings.imagesBuilt = 1;
    Settings_ArrowImage(texels);
    s_settings.arrow = R_CreateHudImageMaterial(R_HUD_IMAGE_MENU_ARROW, texels, SET_ARROW_IMAGE, SET_ARROW_IMAGE);
    Settings_RoundelImage(texels);
    s_settings.roundel =
        R_CreateHudImageMaterial(R_HUD_IMAGE_MENU_ROUNDEL, texels, SET_ROUNDEL_IMAGE, SET_ROUNDEL_IMAGE);
}

static SettingsPage *Settings_Page(const menuDef_t *menu)
{
    int i;

    if (!menu)
        return NULL;
    for (i = 0; i < s_settings.pageCount; i++) {
        if (s_settings.pages[i].menu == menu)
            return &s_settings.pages[i];
    }
    return NULL;
}

static int Settings_PageName(const char *name)
{
    int i;

    for (i = 0; i < (int)(sizeof(s_setPageNames) / sizeof(s_setPageNames[0])); i++) {
        if (!I_stricmp(name, s_setPageNames[i]))
            return 1;
    }
    return 0;
}

static int Settings_Decoration(const itemDef_t *item)
{
    return (item->window.staticFlags & SET_WINDOW_DECORATION) != 0;
}

static int Settings_Tabbed(const itemDef_t *item)
{
    return item->dvarTest && !I_stricmp(item->dvarTest, SET_TAB_DVAR);
}

/* The tab an item of the controller page belongs to, by the first value its
 * showDvar lists. */
static int Settings_Tab(const itemDef_t *item)
{
    const char *p = item->enableDvar;

    if (!Settings_Tabbed(item) || !p)
        return SET_UNTABBED;
    while (*p && (*p < '0' || *p > '9'))
        p++;
    return *p ? atoi(p) : SET_UNTABBED;
}

static int Settings_IsControl(const itemDef_t *item)
{
    if (Settings_Decoration(item) || !item->dvar)
        return 0;
    switch (item->type) {
    case SET_ITEM_EDITFIELD:
    case SET_ITEM_NUMERICFIELD:
    case SET_ITEM_SLIDER:
    case SET_ITEM_YESNO:
    case SET_ITEM_MULTI:
    case SET_ITEM_DVARENUM:
    case SET_ITEM_BIND:
    case SET_ITEM_VALIDFILEFIELD:
    case SET_ITEM_DECIMALFIELD:
    case SET_ITEM_UPREDITFIELD:
        return 1;
    default:
        return 0;
    }
}

static int Settings_Kind(const itemDef_t *item)
{
    switch (item->type) {
    case SET_ITEM_SLIDER:
        return SET_KIND_SLIDER;
    case SET_ITEM_YESNO:
    case SET_ITEM_MULTI:
    case SET_ITEM_DVARENUM:
        return SET_KIND_VALUE;
    case SET_ITEM_BIND:
        return SET_KIND_BIND;
    case SET_ITEM_EDITFIELD:
    case SET_ITEM_NUMERICFIELD:
    case SET_ITEM_VALIDFILEFIELD:
    case SET_ITEM_DECIMALFIELD:
    case SET_ITEM_UPREDITFIELD:
        return SET_KIND_FIELD;
    default:
        return SET_KIND_BUTTON;
    }
}

static int Settings_IsBack(const itemDef_t *item)
{
    return item->text && (!I_stricmp(item->text, "@MENU_BACK") || !I_stricmp(item->text, "Back"));
}

/* Whether the menus would draw the item: shown, and by its showDvar, which
 * the controller page's tabs do not hide here. */
static int Settings_Shown(const itemDef_t *item)
{
    if (!item || !(item->window.dynamicFlags[0] & SET_WINDOW_VISIBLE))
        return 0;
    if (Settings_Tabbed(item) || !(item->dvarFlags & SET_DVAR_SHOW_MASK))
        return 1;
    return Item_EnableShowViaDvar(item, SET_DVAR_SHOW);
}

static int Settings_Enabled(const itemDef_t *item)
{
    return !(item->dvarFlags & SET_DVAR_ENABLE_MASK) || Item_EnableShowViaDvar(item, SET_DVAR_ENABLE);
}

/* The row's control the menus show, or NULL when they show none. */
static itemDef_t *Settings_Control(const SettingsRow *row)
{
    int i;

    for (i = 0; i < 2; i++) {
        if (Settings_Shown(row->controls[i]))
            return row->controls[i];
    }
    return NULL;
}

static const char *Settings_LabelText(const SettingsRow *row, const itemDef_t *control)
{
    return Settings_Translate(row->label ? row->label->text : control->text);
}

static void Settings_AddRow(SettingsPage *page, itemDef_t *control)
{
    const UiRectangle *rect = &control->window.rectClient[0];
    int tab = Settings_Tab(control), i;
    SettingsRow *row;

    for (i = 0; i < page->rowCount; i++) {
        row = &page->rows[i];
        if (row->tab == tab && !row->controls[1] && fabsf(row->y - rect->y) <= SET_ROW_TOLERANCE &&
            fabsf(row->x - rect->x) <= SET_ROW_TOLERANCE && Settings_IsControl(row->controls[0]) == Settings_IsControl(control)) {
            row->controls[1] = control;
            return;
        }
    }
    if (page->rowCount >= SET_MAX_ROWS)
        return;
    row = &page->rows[page->rowCount++];
    memset(row, 0, sizeof(*row));
    row->controls[0] = control;
    row->tab = tab;
    row->y = rect->y;
    row->x = rect->x;
}

static int Settings_RowBefore(const SettingsRow *a, const SettingsRow *b)
{
    if (a->tab != b->tab)
        return a->tab < b->tab;
    if (a->y != b->y)
        return a->y < b->y;
    return a->x < b->x;
}

/* The decoration on a control's row that names it. */
static int Settings_AddLabel(SettingsPage *page, itemDef_t *item)
{
    int tab = Settings_Tab(item), i;

    for (i = 0; i < page->rowCount; i++) {
        SettingsRow *row = &page->rows[i];

        if (!row->label && row->tab == tab && Settings_IsControl(row->controls[0]) &&
            Settings_Blank(row->controls[0]->text) && fabsf(row->y - item->window.rectClient[0].y) <= SET_ROW_TOLERANCE) {
            row->label = item;
            return 1;
        }
    }
    return 0;
}

static void Settings_AddDisplay(SettingsPage *page, itemDef_t *item)
{
    int i;

    for (i = 0; i < page->rowCount; i++) {
        SettingsRow *row = &page->rows[i];

        if (!row->display && fabsf(row->y - item->window.rectClient[0].y) <= SET_ROW_TOLERANCE) {
            row->display = item;
            return;
        }
    }
}

/* Rows from the menu's controls and buttons in the retail order, tab by tab;
 * their names from the decorations beside them; the heading; the rest of the
 * decorations' text as notes. */
static void Settings_Build(SettingsPage *page, menuDef_t *menu, int list)
{
    int i, j;

    memset(page, 0, sizeof(*page));
    page->menu = menu;
    page->list = list;
    for (i = 0; i < menu->itemCount; i++) {
        itemDef_t *item = menu->items[i];

        if (Settings_IsControl(item) && !list) {
            Settings_AddRow(page, item);
        } else if (item->type == SET_ITEM_BUTTON && !Settings_Decoration(item) && !Settings_Blank(item->text) &&
                   item->action && !strstr(item->action, SET_TAB_DVAR)) {
            if (Settings_IsBack(item)) {
                if (!page->back)
                    page->back = item;
            } else {
                Settings_AddRow(page, item);
            }
        }
    }
    for (i = 1; i < page->rowCount; i++) {
        SettingsRow row = page->rows[i];

        for (j = i; j > 0 && Settings_RowBefore(&row, &page->rows[j - 1]); j--)
            page->rows[j] = page->rows[j - 1];
        page->rows[j] = row;
    }
    if (list)
        return;
    for (i = 0; i < menu->itemCount; i++) {
        itemDef_t *item = menu->items[i];

        if (!Settings_Decoration(item) || Settings_Blank(item->text) ||
            (item->type != SET_ITEM_TEXT && item->type != SET_ITEM_BUTTON))
            continue;
        if (item->type == SET_ITEM_TEXT && item->window.rectClient[0].y < SET_ROW_TOP &&
            (!page->header || item->window.rectClient[0].y < page->header->window.rectClient[0].y))
            page->header = item;
    }
    /* Some names are buttons with nothing to do rather than decorations. */
    for (i = 0; i < menu->itemCount; i++) {
        itemDef_t *item = menu->items[i];

        if (item == page->header || Settings_IsControl(item) || (!Settings_Decoration(item) && item->action))
            continue;
        if (item->type == SET_ITEM_DECIMALFIELD) {
            Settings_AddDisplay(page, item);
            continue;
        }
        if (Settings_Blank(item->text) || (item->type != SET_ITEM_TEXT && item->type != SET_ITEM_BUTTON))
            continue;
        if (!Settings_AddLabel(page, item) && page->noteCount < SET_MAX_NOTES)
            page->notes[page->noteCount++] = item;
    }
}

/* The rows the menus show, in order. */
static int Settings_Visible(const SettingsPage *page, int *rows)
{
    int count = 0, i;

    for (i = 0; i < page->rowCount; i++) {
        if (Settings_Control(&page->rows[i]))
            rows[count++] = i;
    }
    return count;
}

/* The focus's place among the visible rows, moving it to the nearest shown
 * row when its own is hidden; -1 with none shown. */
static int Settings_FocusAt(SettingsPage *page, const int *rows, int count)
{
    int i;

    if (count <= 0)
        return -1;
    for (i = 0; i < count; i++) {
        if (rows[i] >= page->focus) {
            page->focus = rows[i];
            return i;
        }
    }
    page->focus = rows[count - 1];
    return count - 1;
}

static void Settings_Scroll(SettingsPage *page, int at, int count)
{
    if (at >= 0 && at < page->scroll)
        page->scroll = at;
    if (at >= page->scroll + SET_ROWS_SHOWN)
        page->scroll = at - SET_ROWS_SHOWN + 1;
    if (page->scroll > count - SET_ROWS_SHOWN)
        page->scroll = count - SET_ROWS_SHOWN;
    if (page->scroll < 0)
        page->scroll = 0;
}

/* The topmost open page; the list when no page is open over it. */
static SettingsPage *Settings_Top(const displayContextDef_t *dc)
{
    int i;

    if (dc != s_settings.dc)
        return NULL;
    for (i = dc->openMenuCount - 1; i >= 0; i--) {
        SettingsPage *page = Settings_Page(dc->menuStack[i]);

        if (page && (page->menu->window.dynamicFlags[0] & SET_WINDOW_VISIBLE))
            return page;
    }
    return NULL;
}

/* A page shown anew starts at its first row; the pointer's place does not
 * move the focus until it moves. */
static void Settings_Show(const displayContextDef_t *dc, SettingsPage *page)
{
    int i;

    if (s_settings.shown == page)
        return;
    if (!page->list) {
        page->focus = 0;
        page->scroll = 0;
    }
    for (i = 0; i < page->rowCount; i++) {
        page->rows[i].slide = page->rows[i].grow = 0.0f;
        page->rows[i].time = 0;
    }
    s_settings.shown = page;
    if (page->list)
        s_settings.listShown = 1;
    s_settings.hoverPage = NULL;
    s_settings.hoverRow = -1;
    s_settings.dragRow = -1;
    s_settings.cursorX = dc->cursorx;
    s_settings.cursorY = dc->cursory;
}

/* Focus on a row, playing its hover script as the pointer would. */
static void Settings_Focus(displayContextDef_t *dc, SettingsPage *page, int row)
{
    itemDef_t *control;

    if (row < 0 || row >= page->rowCount || row == page->focus)
        return;
    page->focus = row;
    control = Settings_Control(&page->rows[row]);
    if (control && control->mouseEnter)
        Item_RunScript(dc, control, control->mouseEnter);
}

static void Settings_Step(displayContextDef_t *dc, SettingsPage *page, int direction)
{
    int rows[SET_MAX_ROWS], count = Settings_Visible(page, rows), at = Settings_FocusAt(page, rows, count);

    if (count <= 0)
        return;
    Settings_Focus(dc, page, rows[(at + direction + count) % count]);
}

/* The key on the control as the menus give it to the focused item, and its
 * action when it took the key. */
static int Settings_Retail(displayContextDef_t *dc, menuDef_t *menu, itemDef_t *item, int key)
{
    int handled, i;

    for (i = 0; i < menu->itemCount; i++)
        menu->items[i]->window.dynamicFlags[0] &= ~SET_WINDOW_HASFOCUS;
    item->window.dynamicFlags[0] |= SET_WINDOW_HASFOCUS;
    handled = Item_HandleKey(dc, item, key, 1);
    item->window.dynamicFlags[0] &= ~SET_WINDOW_HASFOCUS;
    if (handled && item->action)
        Item_RunScript(dc, item, item->action);
    return handled;
}

static itemDef_t *Settings_Focused(const SettingsPage *page)
{
    return page->focus >= 0 && page->focus < page->rowCount ? Settings_Control(&page->rows[page->focus]) : NULL;
}

/* Left, right, Page Up or Page Down on a value or a slider. */
static void Settings_Change(displayContextDef_t *dc, SettingsPage *page, int key)
{
    itemDef_t *control = Settings_Focused(page);
    int kind;

    if (page->list || !control || !Settings_Enabled(control))
        return;
    kind = Settings_Kind(control);
    if (kind == SET_KIND_VALUE || kind == SET_KIND_SLIDER)
        Settings_Retail(dc, page->menu, control, key);
}

static void Settings_Activate(displayContextDef_t *dc, SettingsPage *page)
{
    itemDef_t *control = Settings_Focused(page);

    if (!control || !Settings_Enabled(control))
        return;
    switch (page->list ? SET_KIND_BUTTON : Settings_Kind(control)) {
    case SET_KIND_BUTTON:
        if (control->action)
            Item_RunScript(dc, control, control->action);
        break;
    case SET_KIND_VALUE:
        Settings_Retail(dc, page->menu, control, SET_KEY_ENTER);
        break;
    case SET_KIND_BIND:
        if (Settings_Retail(dc, page->menu, control, SET_KEY_ENTER))
            s_settings.bindItem = control;
        break;
    case SET_KIND_FIELD:
        Item_EditField_Begin(control);
        break;
    default:
        break;
    }
}

/* Escape: the list closes as the menu's own Escape does; a page runs its Back
 * button, else closes, back to the list. A page opened while the list was
 * never shown, as the pause menu opens one, runs its own Escape, which closes
 * Options back to where they were opened. */
static void Settings_Back(displayContextDef_t *dc, SettingsPage *page)
{
    menuDef_t *menu = page->menu;

    if (page->list) {
        if (menu->onESC && menu->itemCount > 0)
            Item_RunScript(dc, menu->items[0], menu->onESC);
    } else if (page->back && Settings_Shown(page->back) && page->back->action) {
        Item_RunScript(dc, page->back, page->back->action);
    } else if (!s_settings.listShown && menu->onESC && menu->itemCount > 0) {
        Item_RunScript(dc, menu->items[0], menu->onESC);
    } else {
        Menus_CloseByName(dc, menu->window.name);
    }
}

/* The slider's dvar at the pointer's place along the bar. */
static void Settings_SlideTo(const displayContextDef_t *dc, itemDef_t *item)
{
    editFieldDef_t *edit = Item_GetEditFieldDef(item);
    float x = SET_SLIDER_LEFT, y = 0.0f, w = SET_SLIDER_WIDTH, h = 0.0f, cx = (float)dc->cursorx, t;

    if (!edit || !item->dvar || edit->maxVal <= edit->minVal)
        return;
    CalcScreenPlacement(&x, &y, &w, &h, SET_ALIGN_LEFT, SET_ALIGN_TOP);
    CalcScreenX(&cx, SET_ALIGN_FULLSCREEN);
    t = w > 0.0f ? Settings_Clamp01((cx - x) / w) : 0.0f;
    Dvar_SetFromStringByName(item->dvar, va("%g", (double)(edit->minVal + (edit->maxVal - edit->minVal) * t)));
}

static float Settings_SliderFraction(itemDef_t *item)
{
    editFieldDef_t *edit = Item_GetEditFieldDef(item);

    if (!edit || !item->dvar || edit->maxVal <= edit->minVal)
        return 0.0f;
    return Settings_Clamp01((Settings_Number(item->dvar) - edit->minVal) / (edit->maxVal - edit->minVal));
}

static int Settings_HoverValid(const SettingsPage *page)
{
    return s_settings.hoverPage == page && s_settings.hoverRow >= 0 && s_settings.hoverRow < page->rowCount;
}

static void Settings_Hover(const displayContextDef_t *dc, SettingsPage *page, const int *rows, int count);

/* A click can come before a frame paints the pointer's new place. */
static void Settings_HoverNow(const displayContextDef_t *dc, SettingsPage *page)
{
    int rows[SET_MAX_ROWS], count = Settings_Visible(page, rows);

    Settings_Hover(dc, page, rows, count);
}

static void Settings_Click(displayContextDef_t *dc, SettingsPage *page)
{
    itemDef_t *control;

    Settings_HoverNow(dc, page);
    if (!Settings_HoverValid(page))
        return;
    Settings_Focus(dc, page, s_settings.hoverRow);
    control = Settings_Focused(page);
    if (!control || !Settings_Enabled(control))
        return;
    switch (page->list ? SET_PART_ROW : s_settings.hoverPart) {
    case SET_PART_LEFT:
        Settings_Change(dc, page, SET_KEY_LEFT);
        break;
    case SET_PART_RIGHT:
        Settings_Change(dc, page, SET_KEY_RIGHT);
        break;
    case SET_PART_BAR:
        Settings_SlideTo(dc, control);
        if (control->action)
            Item_RunScript(dc, control, control->action);
        s_settings.dragRow = page->focus;
        break;
    default:
        Settings_Activate(dc, page);
        break;
    }
}

/* The right button steps a value back, as the menus' does; elsewhere it goes
 * back. */
static void Settings_RightClick(displayContextDef_t *dc, SettingsPage *page)
{
    itemDef_t *control;

    Settings_HoverNow(dc, page);
    control = Settings_HoverValid(page) ? Settings_Control(&page->rows[s_settings.hoverRow]) : NULL;

    if (!page->list && control && Settings_Kind(control) == SET_KIND_VALUE) {
        Settings_Focus(dc, page, s_settings.hoverRow);
        Settings_Change(dc, page, SET_KEY_LEFT);
        return;
    }
    Settings_Back(dc, page);
}

int UI_WWIISettings_KeyEvent(displayContextDef_t *dc, menuDef_t *menu, int key, int down)
{
    SettingsPage *page = Settings_Page(menu);
    itemDef_t *editing;

    if (!page || !Settings_On() || dc != s_settings.dc)
        return 0;
    Settings_Show(dc, page);
    if (Display_KeyBindPending()) {
        /* A wait the menus began is theirs to finish. */
        if (!s_settings.bindItem)
            return 0;
        Item_Bind_HandleKey(dc, s_settings.bindItem, key, down);
        if (!Display_KeyBindPending())
            s_settings.bindItem = NULL;
        return 1;
    }
    editing = Item_EditField_Current();
    if (editing && down) {
        if (key == SET_KEY_MOUSE1 || key == SET_KEY_MOUSE2 || key == SET_KEY_UP || key == SET_KEY_DOWN ||
            key == SET_KEY_TAB || key == SET_KEY_WHEEL_UP || key == SET_KEY_WHEEL_DOWN) {
            Item_EditField_End();
        } else {
            if (!Item_TextField_HandleKey(dc, editing, key))
                Item_EditField_End();
            return 1;
        }
    }
    if (!down) {
        if (key == SET_KEY_MOUSE1)
            s_settings.dragRow = -1;
        return 1;
    }
    if (key & SET_KEY_CHAR)
        return 1;
    switch (key) {
    case SET_KEY_UP:
    case SET_KEY_WHEEL_UP:
        Settings_Step(dc, page, -1);
        break;
    case SET_KEY_DOWN:
    case SET_KEY_TAB:
    case SET_KEY_WHEEL_DOWN:
        Settings_Step(dc, page, 1);
        break;
    case SET_KEY_LEFT:
    case SET_KEY_RIGHT:
    case SET_KEY_PGUP:
    case SET_KEY_PGDN:
        Settings_Change(dc, page, key);
        break;
    case SET_KEY_MOUSE1:
        Settings_Click(dc, page);
        break;
    case SET_KEY_MOUSE2:
        Settings_RightClick(dc, page);
        break;
    case SET_KEY_ESCAPE:
        Settings_Back(dc, page);
        break;
    case SET_KEY_ENTER:
    case SET_KEY_KP_ENTER:
        Settings_Activate(dc, page);
        break;
    default:
        break;
    }
    return 1;
}

static void Settings_Animate(SettingsRow *row, int time, int focused)
{
    float elapsed = row->time ? (float)(time - row->time) : 0.0f;
    float step = focused ? 1.0f : -1.0f;

    if (elapsed < 0.0f || elapsed > 100.0f)
        elapsed = elapsed < 0.0f ? 0.0f : 100.0f;
    row->time = time ? time : 1;
    row->slide = Settings_Clamp01(row->slide + step * elapsed / SET_SLIDE_MS);
    row->grow = Settings_Clamp01(row->grow + step * elapsed / SET_GROW_MS);
}

/* Capitals no wider than room at cap: the cap that fits. */
static float Settings_Fit(const char *text, int bold, float cap, float tracking, float room)
{
    float width = UI_WWIIMenu_Width(text, bold, cap, tracking);

    return width > room && width > 0.0f ? cap * room / width : cap;
}

static void Settings_PaintLabel(SettingsRow *row, const char *text, float baseline, int focused, int enabled,
                                float right, int time)
{
    char upper[SET_TEXT_MAX];
    float slide, grow, tracking, x, cap;
    vec4_t color;
    int bold, i;

    Settings_Animate(row, time, focused);
    slide = Settings_EaseOut(row->slide);
    grow = Settings_EaseOut(row->grow);
    tracking = Settings_Lerp(SET_LABEL_TRACKING, SET_FOCUS_TRACKING, grow);
    for (i = 0; i < 4; i++)
        color[i] = enabled ? Settings_Lerp(UI_WWIIMenu_Color(WWII_COLOR_IDLE)[i], s_setGold[i], grow) : s_setDisabled[i];
    bold = grow >= 0.5f;
    x = SET_ROW_X + SET_FOCUS_INDENT * slide;
    Settings_Upper(text, upper, sizeof(upper));
    cap = Settings_Fit(upper, bold, SET_LABEL_CAP, tracking, right - SET_LABEL_GAP - x);
    Settings_Text(upper, bold, x, baseline, cap, tracking, color);
}

/* A Yes/No, or a list of Yes and No, as WWII words its toggles. */
static const char *Settings_ValueText(itemDef_t *item)
{
    const char *text;

    if (item->type == SET_ITEM_YESNO)
        return Settings_Number(item->dvar) != 0.0f ? "Enabled" : "Disabled";
    text = Item_ValueText(item);
    if (text && !I_stricmp(text, "@MENU_YES"))
        return "Enabled";
    if (text && !I_stricmp(text, "@MENU_NO"))
        return "Disabled";
    return Settings_Translate(text);
}

static void Settings_PaintValue(const char *text, float baseline, const vec_t *color)
{
    float cap = Settings_Fit(text, 0, SET_VALUE_CAP, 0.0f, SET_VALUE_ROOM);

    Settings_Text(text, 0, SET_VALUE_CENTER - 0.5f * UI_WWIIMenu_Width(text, 0, cap, 0.0f), baseline, cap, 0.0f,
                  color);
}

static void Settings_Arrow(float x, float baseline, int left)
{
    if (s_settings.arrow)
        CL_DrawStretchPic(x, baseline - SET_SLIDER_ABOVE, SET_ARROW_WIDTH, SET_SLIDER_HEIGHT, SET_ALIGN_LEFT,
                          SET_ALIGN_TOP, left ? 1.0f : 0.0f, 0.0f, left ? 0.0f : 1.0f, 1.0f, s_setArrow,
                          s_settings.arrow);
}

/* The field's text from its scroll, with a blinking caret while edited. */
static void Settings_PaintField(const displayContextDef_t *dc, itemDef_t *item, float baseline, const vec_t *color)
{
    editFieldDef_t *edit = Item_GetEditFieldDef(item);
    const char *value = item->dvar ? Dvar_GetVariantString(item->dvar) : "";
    int editing = Item_EditField_Current() == item, length, offset = 0, count, caret;
    char shown[SET_TEXT_MAX], before[SET_TEXT_MAX];
    float cap, x;

    if (!value)
        value = "";
    length = (int)strlen(value);
    if (editing && edit)
        offset = edit->paintOffset < 0 ? 0 : (edit->paintOffset > length ? length : edit->paintOffset);
    count = edit && edit->maxPaintChars > 0 ? edit->maxPaintChars : SET_TEXT_MAX - 1;
    if (count > SET_TEXT_MAX - 1)
        count = SET_TEXT_MAX - 1;
    snprintf(shown, sizeof(shown), "%.*s", count, value + offset);
    cap = Settings_Fit(shown, 0, SET_VALUE_CAP, 0.0f, SET_VALUE_ROOM);
    x = SET_VALUE_CENTER - 0.5f * UI_WWIIMenu_Width(shown, 0, cap, 0.0f);
    Settings_Text(shown, 0, x, baseline, cap, 0.0f, color);
    if (!editing || ((dc->realTime / 500) & 1))
        return;
    caret = item->cursorPos[0] - offset;
    if (caret < 0)
        caret = 0;
    snprintf(before, sizeof(before), "%.*s", caret, shown);
    Settings_Fill(x + UI_WWIIMenu_Width(before, 0, cap, 0.0f), baseline - cap - 1.0f, SET_CARET_WIDTH, cap + 2.0f,
                  s_setGold);
}

static void Settings_PaintRow(const displayContextDef_t *dc, SettingsPage *page, int index, float baseline)
{
    SettingsRow *row = &page->rows[index];
    itemDef_t *control = Settings_Control(row);
    int focused = index == page->focus, enabled, kind;
    const vec_t *valueColor;
    char text[SET_TEXT_MAX];

    if (!control)
        return;
    enabled = Settings_Enabled(control);
    kind = page->list ? SET_KIND_BUTTON : Settings_Kind(control);
    Settings_PaintLabel(row, Settings_LabelText(row, control), baseline, focused, enabled,
                        page->list ? SET_LIST_BOX_RIGHT
                                   : (kind == SET_KIND_BUTTON ? SET_BOX_RIGHT : SET_ARROW_LEFT),
                        dc->realTime);
    valueColor = !enabled ? s_setDisabled : (focused ? UI_WWIIMenu_Color(WWII_COLOR_IDLE) : s_setValue);
    switch (kind) {
    case SET_KIND_VALUE:
        snprintf(text, sizeof(text), "%s", Settings_ValueText(control));
        Settings_PaintValue(text, baseline, valueColor);
        break;
    case SET_KIND_SLIDER:
        Settings_Fill(SET_SLIDER_LEFT, baseline - SET_SLIDER_ABOVE, SET_SLIDER_WIDTH, SET_SLIDER_HEIGHT, s_setGroove);
        Settings_Fill(SET_SLIDER_LEFT, baseline - SET_SLIDER_ABOVE, SET_SLIDER_WIDTH * Settings_SliderFraction(control),
                      SET_SLIDER_HEIGHT, enabled ? s_setBar : s_setDisabled);
        if (row->display && row->display->dvar)
            Settings_Text(va("%g", (double)Settings_Number(row->display->dvar)), 0, SET_DISPLAY_X, baseline,
                          SET_VALUE_CAP, 0.0f, valueColor);
        break;
    case SET_KIND_BIND:
        if (Display_KeyBindPending() && s_settings.bindItem == control) {
            Settings_PaintValue(Settings_Tabbed(control) || !I_stricmp(page->menu->window.name, "browser_gamepad")
                                    ? "Press a button"
                                    : "Press a key",
                                baseline, s_setGold);
        } else {
            Item_BindText(control, text, sizeof(text));
            Settings_PaintValue(text, baseline, valueColor);
        }
        break;
    case SET_KIND_FIELD:
        Settings_PaintField(dc, control, baseline, Item_EditField_Current() == control ? s_setWhite : valueColor);
        break;
    default:
        break;
    }
    if (focused && enabled && (kind == SET_KIND_VALUE || kind == SET_KIND_SLIDER)) {
        Settings_Arrow(SET_ARROW_LEFT, baseline, 1);
        Settings_Arrow(SET_ARROW_RIGHT, baseline, 0);
    }
}

/* The frame's top, bottom and right lines; its left lies off the screen. */
static void Settings_PaintFrame(float baseline, float right)
{
    float top = baseline - SET_BOX_ABOVE;

    Settings_Fill(0.0f, top, right, SET_BOX_LINE, s_setFrame);
    Settings_Fill(0.0f, top + SET_BOX_HEIGHT - SET_BOX_LINE, right, SET_BOX_LINE, s_setFrame);
    Settings_Fill(right - SET_BOX_LINE, top + SET_BOX_LINE, SET_BOX_LINE, SET_BOX_HEIGHT - 2.0f * SET_BOX_LINE,
                  s_setFrame);
}

static void Settings_PaintScrollbar(const SettingsPage *page, int count)
{
    float top = SET_ROW_TOP - SET_BOX_ABOVE;
    float height = (float)(SET_ROWS_SHOWN - 1) * SET_ROW_SPACING + SET_BOX_HEIGHT;
    float thumb = height * (float)SET_ROWS_SHOWN / (float)count;

    Settings_Fill(SET_BOX_RIGHT + SET_SCROLL_GAP, top, SET_SCROLL_WIDTH, height, s_setScroll);
    Settings_Fill(SET_BOX_RIGHT + SET_SCROLL_GAP,
                  top + (height - thumb) * (float)page->scroll / (float)(count - SET_ROWS_SHOWN), SET_SCROLL_WIDTH,
                  thumb, s_setFrame);
}

static const char *Settings_Description(const SettingsPage *page, const itemDef_t *item)
{
    char key[SET_TEXT_MAX];
    int control = Settings_IsControl(item) && !page->list, i;

    snprintf(key, sizeof(key), "%s/%s", page->menu->window.name ? page->menu->window.name : "",
             item->text ? item->text : "");
    for (i = 0; i < (int)(sizeof(s_setDescriptions) / sizeof(s_setDescriptions[0])); i++) {
        if (control ? !I_stricmp(s_setDescriptions[i].key, item->dvar) : !I_stricmp(s_setDescriptions[i].key, key))
            return s_setDescriptions[i].text;
    }
    return NULL;
}

/* Words across lines no wider than width from baseline; the next baseline.
 * A word longer than a line is cut. */
static float Settings_Wrap(const char *text, float baseline, float width, float cap, const vec_t *color)
{
    float spacing = SET_PANEL_LINE * cap / SET_PANEL_TEXT_CAP;
    char line[SET_TEXT_MAX], trial[SET_TEXT_MAX];
    int length = 0;

    line[0] = '\0';
    while (*text == ' ')
        text++;
    while (*text) {
        int wordLength = (int)strcspn(text, " ");
        int keep = wordLength < (int)sizeof(line) - 1 ? wordLength : (int)sizeof(line) - 1;

        if (length) {
            snprintf(trial, sizeof(trial), "%s %.*s", line, keep, text);
            if ((int)strlen(trial) >= (int)sizeof(trial) - 1 || UI_WWIIMenu_Width(trial, 0, cap, 0.0f) > width) {
                Settings_Text(line, 0, SET_PANEL_X, baseline, cap, 0.0f, color);
                baseline += spacing;
                length = 0;
            }
        }
        length += snprintf(line + length, sizeof(line) - (size_t)length, "%s%.*s", length ? " " : "", keep, text);
        text += wordLength;
        while (*text == ' ')
            text++;
    }
    if (length) {
        Settings_Text(line, 0, SET_PANEL_X, baseline, cap, 0.0f, color);
        baseline += spacing;
    }
    return baseline;
}

/* The shown notes as one paragraph: a fragment going on in lower case after
 * the last joins it, as does one after a full stop; others start a sentence. */
static void Settings_Notes(const SettingsPage *page, char *out, int size)
{
    int i, length = 0;

    out[0] = '\0';
    for (i = 0; i < page->noteCount; i++) {
        const char *text;
        char last;

        if (!Settings_Shown(page->notes[i]))
            continue;
        text = Settings_Translate(page->notes[i]->text);
        while (*text == ' ')
            text++;
        if (!*text)
            continue;
        last = length ? out[length - 1] : '\0';
        length += snprintf(out + length, (size_t)(size - length), "%s%s",
                           !length ? "" : (strchr(".?!:;", last) || (*text >= 'a' && *text <= 'z') ? " " : ". "),
                           text);
        if (length >= size - 1)
            return;
        while (length && out[length - 1] == ' ')
            out[--length] = '\0';
    }
}

static void Settings_PaintRoundels(float width)
{
    float size = SET_ROUNDEL_SIZE, step = SET_ROUNDEL_STEP, first = SET_ROUNDEL_FIRST;
    float span = SET_ROUNDEL_FIRST - SET_PANEL_X + (SET_ROUNDEL_COUNT - 1) * SET_ROUNDEL_STEP + 0.5f * SET_ROUNDEL_SIZE;
    int i;

    if (!s_settings.roundel)
        return;
    if (span > width && span > 0.0f) {
        size *= width / span;
        step *= width / span;
        first = SET_PANEL_X + (SET_ROUNDEL_FIRST - SET_PANEL_X) * width / span;
    }
    for (i = 0; i < SET_ROUNDEL_COUNT; i++) {
        vec4_t color = { s_setRoundels[i], s_setRoundels[i], s_setRoundels[i], 1.0f };

        CL_DrawStretchPic(first + (float)i * step - 0.5f * size, SET_ROUNDEL_Y - 0.5f * size, size, size,
                          SET_ALIGN_LEFT, SET_ALIGN_TOP, 0.0f, 0.0f, 1.0f, 1.0f, color, s_settings.roundel);
    }
}

/* The page's heading in capitals; SETTINGS for the list. */
static void Settings_Heading(const SettingsPage *page, char *out, int size)
{
    Settings_Upper(page->list || !page->header ? SET_TITLE : Settings_Translate(page->header->text), out, size);
}

static void Settings_PageTitle(const SettingsPage *page, char *out, int size)
{
    char heading[SET_TEXT_MAX];

    Settings_Heading(page, heading, sizeof(heading));
    if (page->list || !page->header)
        snprintf(out, (size_t)size, "%s", heading);
    else
        snprintf(out, (size_t)size, "%s%s%s", SET_TITLE, SET_TITLE_SEPARATOR, heading);
}

/* The focused row's name, what it does, a key's status and the page's notes;
 * the page's heading over the notes when no row shows. */
static void Settings_PaintPanel(const SettingsPage *page, const SettingsRow *row, itemDef_t *control)
{
    float width = Settings_ScreenWidth() - SET_PANEL_X - SET_PANEL_RIGHT, baseline = SET_PANEL_TEXT_BASELINE, cap;
    char upper[SET_TEXT_MAX], notes[SET_NOTES_MAX];
    const char *text;
    int gap = 0;

    if (width <= 0.0f)
        return;
    if (row && control)
        Settings_Upper(Settings_LabelText(row, control), upper, sizeof(upper));
    else
        Settings_Heading(page, upper, sizeof(upper));
    cap = Settings_Fit(upper, 1, SET_PANEL_TITLE_CAP, SET_PANEL_TITLE_TRACKING, width);
    Settings_Text(upper, 1, SET_PANEL_X, SET_ROW_TOP, cap, SET_PANEL_TITLE_TRACKING, s_setGold);
    if (control && (text = Settings_Description(page, control)) != NULL) {
        baseline = Settings_Wrap(text, baseline, width, SET_PANEL_TEXT_CAP, s_setText);
        gap = 1;
    }
    if (control && !page->list && Settings_Kind(control) == SET_KIND_BIND) {
        int waiting = Display_KeyBindPending() && s_settings.bindItem == control;

        baseline = Settings_Wrap(UI_SafeTranslateString(waiting ? "EXE_KEYWAIT" : "EXE_KEYCHANGE"),
                                 baseline + (gap ? SET_PANEL_GAP : 0.0f), width, SET_PANEL_TEXT_CAP,
                                 waiting ? s_setGold : s_setText);
        gap = 1;
    }
    Settings_Notes(page, notes, sizeof(notes));
    if (notes[0])
        baseline = Settings_Wrap(notes, baseline + (gap ? SET_PANEL_GAP : 0.0f), width, SET_PANEL_NOTE_CAP, s_setNote);
    if (control && control->dvar && !I_stricmp(control->dvar, "r_gamma"))
        Settings_PaintRoundels(width);
}

static void Settings_PaintPrompts(int rows)
{
    static const char *const selectKeys[] = { "ENTER" }, *const selectLabels[] = { "Select" };
    static const char *const clearKeys[] = { "BACKSPACE" }, *const clearLabels[] = { "Clear" };
    static const char *const doneKeys[] = { "ENTER" }, *const doneLabels[] = { "Done" };

    if (Display_KeyBindPending())
        UI_WWIIMenu_Prompts(clearKeys, clearLabels, 1, "ESC", "Cancel");
    else if (Item_EditField_Current())
        UI_WWIIMenu_Prompts(doneKeys, doneLabels, 1, NULL, NULL);
    else
        UI_WWIIMenu_Prompts(selectKeys, selectLabels, rows > 0 ? 1 : 0, "ESC", "Back");
}

/* The row under the pointer among those shown, and the part of it. */
static void Settings_Hover(const displayContextDef_t *dc, SettingsPage *page, const int *rows, int count)
{
    float right = page->list ? SET_LIST_BOX_RIGHT : SET_BOX_RIGHT;
    int i;

    s_settings.hoverPage = page;
    s_settings.hoverRow = -1;
    s_settings.hoverPart = SET_PART_ROW;
    for (i = page->scroll; i < count && i < page->scroll + SET_ROWS_SHOWN; i++) {
        float top = Settings_Baseline(i - page->scroll) - SET_BOX_ABOVE;
        itemDef_t *control;
        int kind;

        if (!Settings_Inside(dc, 0.0f, top, right, SET_ROW_SPACING))
            continue;
        s_settings.hoverRow = rows[i];
        control = Settings_Control(&page->rows[rows[i]]);
        if (page->list || !control)
            return;
        kind = Settings_Kind(control);
        if (kind != SET_KIND_VALUE && kind != SET_KIND_SLIDER)
            return;
        if (Settings_Inside(dc, SET_ARROW_LEFT - SET_ARROW_HIT, top, SET_ARROW_WIDTH + 2.0f * SET_ARROW_HIT,
                            SET_ROW_SPACING))
            s_settings.hoverPart = SET_PART_LEFT;
        else if (Settings_Inside(dc, SET_ARROW_RIGHT - SET_ARROW_HIT, top, SET_ARROW_WIDTH + 2.0f * SET_ARROW_HIT,
                                 SET_ROW_SPACING))
            s_settings.hoverPart = SET_PART_RIGHT;
        else if (kind == SET_KIND_SLIDER && Settings_Inside(dc, SET_SLIDER_LEFT, top, SET_SLIDER_WIDTH, SET_ROW_SPACING))
            s_settings.hoverPart = SET_PART_BAR;
        return;
    }
}

void UI_WWIISettings_Paint(displayContextDef_t *dc, menuDef_t *menu)
{
    SettingsPage *page = Settings_Page(menu);
    int rows[SET_MAX_ROWS], count, at, i;
    char title[SET_TEXT_MAX];

    if (!page || !Settings_On() || Settings_Top(dc) != page)
        return;
    Settings_Show(dc, page);
    Settings_BuildImages();
    {
        vec4_t shade = { 0.0f, 0.0f, 0.0f, page->list ? SET_LIST_SHADE : SET_PAGE_SHADE };

        CL_DrawStretchPic(0.0f, 0.0f, 640.0f, 480.0f, SET_ALIGN_FULLSCREEN, SET_ALIGN_FULLSCREEN, 0.0f, 0.0f, 1.0f,
                          1.0f, shade, sharedUiInfo.assets.whiteMaterial);
    }

    count = Settings_Visible(page, rows);
    at = Settings_FocusAt(page, rows, count);
    Settings_Scroll(page, at, count);
    Settings_Hover(dc, page, rows, count);
    /* Pointer motion moves the focus, but not while it drags a slider or a
     * key or a name is being typed. */
    if (dc->cursorx != s_settings.cursorX || dc->cursory != s_settings.cursorY) {
        s_settings.cursorX = dc->cursorx;
        s_settings.cursorY = dc->cursory;
        if (s_settings.hoverRow >= 0 && s_settings.dragRow < 0 && !Display_KeyBindPending() &&
            !Item_EditField_Current()) {
            Settings_Focus(dc, page, s_settings.hoverRow);
            at = Settings_FocusAt(page, rows, count);
        }
    }
    if (s_settings.dragRow >= 0 && s_settings.dragRow == page->focus && Settings_Focused(page))
        Settings_SlideTo(dc, Settings_Focused(page));

    Settings_PageTitle(page, title, sizeof(title));
    Settings_Text(title, 1, SET_TITLE_X, SET_TITLE_BASELINE, SET_TITLE_CAP, SET_TITLE_TRACKING, s_setGold);
    for (i = page->scroll; i < count && i < page->scroll + SET_ROWS_SHOWN; i++)
        Settings_PaintRow(dc, page, rows[i], Settings_Baseline(i - page->scroll));
    if (at >= 0)
        Settings_PaintFrame(Settings_Baseline(at - page->scroll), page->list ? SET_LIST_BOX_RIGHT : SET_BOX_RIGHT);
    if (count > SET_ROWS_SHOWN)
        Settings_PaintScrollbar(page, count);
    if (at >= 0)
        Settings_PaintPanel(page, &page->rows[page->focus], Settings_Focused(page));
    else
        Settings_PaintPanel(page, NULL, NULL);
    Settings_PaintPrompts(count);
}

void UI_WWIISettings_Reset(const displayContextDef_t *dc)
{
    if (s_settings.dc && dc != s_settings.dc)
        return;
    memset(&s_settings, 0, sizeof(s_settings));
    s_settings.generation = R_GetHudImageGeneration();
    s_settings.hoverRow = -1;
    s_settings.dragRow = -1;
}

void UI_WWIISettings_AddMenu(const displayContextDef_t *dc, menuDef_t *menu)
{
    const char *name = menu ? menu->window.name : NULL;
    int list;

    if (!name || (s_settings.dc && dc != s_settings.dc) || s_settings.pageCount >= SET_MAX_PAGES)
        return;
    list = !I_stricmp(name, SET_LIST_MENU);
    if (!list && !Settings_PageName(name))
        return;
    s_settings.dc = dc;
    Settings_Build(&s_settings.pages[s_settings.pageCount++], menu, list);
}

void UI_WWIISettings_Update(const displayContextDef_t *dc)
{
    int i;

    if (dc != s_settings.dc)
        return;
    if (s_settings.generation != R_GetHudImageGeneration()) {
        s_settings.generation = R_GetHudImageGeneration();
        s_settings.imagesBuilt = 0;
        s_settings.arrow = s_settings.roundel = NULL;
    }
    if (!s_settings.shown || Settings_Top(dc))
        return;
    /* Options closed: it opens again at its first category. */
    s_settings.shown = NULL;
    s_settings.listShown = 0;
    s_settings.dragRow = -1;
    s_settings.bindItem = NULL;
    for (i = 0; i < s_settings.pageCount; i++) {
        if (s_settings.pages[i].list)
            s_settings.pages[i].focus = 0;
    }
}

int UI_WWIISettings_Owns(const menuDef_t *menu)
{
    return Settings_On() && Settings_Page(menu) != NULL;
}

int UI_WWIISettings_Covers(const displayContextDef_t *dc)
{
    return Settings_On() && Settings_Top(dc) != NULL;
}
