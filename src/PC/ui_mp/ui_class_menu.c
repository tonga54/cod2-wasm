#include "common_types.h"
#include "imports.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include "headers/PC/gfx_d3d/r_weapon_camo.h"
#include "headers/PC/bgame/bg_weapon_camo.h"
#include "headers/PC/ui_mp/ui_class_data.h"
#include "headers/PC/ui_mp/ui_class_menu.h"
#include "headers/PC/ui_mp/ui_pause_menu.h"
#include "headers/PC/ui_mp/ui_wwii_menu.h"
#include <stdio.h>
#include <string.h>

extern sharedUiInfo_t sharedUiInfo;
extern const char *UI_SafeTranslateString(const char *reference);
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1,
                              float s2, float t2, const vec_t *color, MaterialHandle material);
extern MaterialHandle CL_RegisterMaterialNoMip(const char *name, int imageTrack);
extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);
extern void CalcScreenX(float *x, int horzAlign);
extern void CalcScreenY(float *y, int vertAlign);
extern int FS_ReadFile(const char *qpath, void **buffer);
extern void FS_FreeFile(void *buffer);
extern const char *Dvar_GetVariantString(const char *dvarName);
extern int Dvar_GetInt(const char *name);
extern void Cbuf_ExecuteText(int when, const char *text);
extern const char *CL_GetConfigString(int index);
extern char *va(const char *format, ...);
extern void Menus_CloseByName(displayContextDef_t *dc, const char *name);
extern qboolean Menus_OpenByName(displayContextDef_t *dc, const char *name);
extern int I_stricmp(const char *s0, const char *s1);
extern int UI_PlayLocalSoundAliasByName(const char *name);
extern float g_webMenuBackdrop;
#ifdef __EMSCRIPTEN__
extern void Web_ClassesLoad(const char *player, int generation);
extern void Web_ClassesSave(const char *player, const char *lines, int generation);
#endif

#define CAC_ALIGN_LEFT 1
#define CAC_ALIGN_CENTER 2
#define CAC_ALIGN_RIGHT 3
#define CAC_ALIGN_TOP 1
#define CAC_ALIGN_FULLSCREEN 4

#define CAC_KEY_TAB 0x09
#define CAC_KEY_ENTER 0x0d
#define CAC_KEY_ESCAPE 0x1b
/* A controller's X arrives as Backspace. */
#define CAC_KEY_BACKSPACE 0x7f
#define CAC_KEY_UP 0x9a
#define CAC_KEY_DOWN 0x9b
#define CAC_KEY_LEFT 0x9c
#define CAC_KEY_RIGHT 0x9d
#define CAC_KEY_DEL 0xa2
#define CAC_KEY_F1 0xa7
#define CAC_KEY_F2 0xa8
#define CAC_KEY_KP_ENTER 0xbf
#define CAC_KEY_MOUSE1 0xc8
#define CAC_KEY_MOUSE2 0xc9
#define CAC_KEY_WHEEL_DOWN 0xcd
#define CAC_KEY_WHEEL_UP 0xce
/* A controller's Y, WWII's Customize; the gamepad passes it to these screens. */
#define CAC_KEY_BUTTON_Y 0xd2
#define CAC_KEY_CHAR 0x400

/* Units from the screen's edges, measured on the WWII capture at 1728 x 1080,
 * 2.25 px a unit, like the main menu's: the title's capitals 9 tall on a
 * baseline 84 down, the column 40 from the left edge. */
#define CAC_LEFT 40.0f
#define CAC_TITLE_BASELINE 84.0f
#define CAC_TITLE_CAP 9.0f
#define CAC_TITLE_TRACKING 0.18f
#define CAC_LINE 0.5f
/* The list's rows: 33.3 apart from 117 down, 217 wide; a name's capitals 6
 * tall, the line under it 4.4. */
#define CAC_ROW_TOP 117.0f
#define CAC_ROW_HEIGHT 33.3f
#define CAC_ROW_WIDTH 217.0f
#define CAC_ROW_NAME_CAP 6.0f
#define CAC_ROW_NOTE_CAP 4.4f
/* The detail panel right of the lists, and the side panel at the right edge. */
#define CAC_DETAIL_LEFT 246.0f
#define CAC_DETAIL_WIDTH 178.0f
#define CAC_SIDE_RIGHT 40.0f
#define CAC_SIDE_WIDTH 168.0f
/* A tile's two lines above it: the slot's capitals 4.4 tall, the weapon's 6. */
#define CAC_TILE_CAPTION_CAP 4.4f
#define CAC_TILE_NAME_CAP 6.0f
/* The weapon list: rows 25 tall, 30 apart from 122 down, from the left edge. */
#define CAC_PICK_TOP 122.0f
#define CAC_PICK_ROW 30.0f
#define CAC_PICK_HEIGHT 25.0f
#define CAC_PICK_WIDTH 217.0f
#define CAC_PICK_VISIBLE 8
/* Stat rows at the right edge, 17 apart, bars 104 long. */
#define CAC_STAT_TOP 300.0f
#define CAC_STAT_ROW 17.0f
#define CAC_STAT_BAR 104.0f
/* The equipment rows' icons, 30 square, 38 apart, six to a row: four
 * grenades, the S-Mine and C4. */
#define CAC_ICON_SIZE 30.0f
#define CAC_ICON_STEP 38.0f
#define CAC_EQUIP_COLUMNS 6
#define CAC_RETAIL_IMAGE_ASPECT (199.0f / 112.0f)
/* Camouflage, as WWII's: swatches 30 square and 3 apart, four a row from the
 * list's top, the focused pattern's name beside them; under each gun's tile
 * in the editor, a line with its camouflage. */
#define CAC_CAMO_SWATCH 30.0f
#define CAC_CAMO_GAP 3.0f
#define CAC_CAMO_COLUMNS 4
#define CAC_CAMO_LINE_HEIGHT 9.0f
#define CAC_CAMO_LINE_CAP 4.0f

/* Select Class, where a match starts: units measured on the WWII capture of
 * Select Division at 1024 x 576, 1.2 px a unit. The title's capitals are 11
 * tall in Bold on a baseline 51.5 down, inked from 46.7. */
#define CAC_SELECT_TITLE_LEFT 46.1f
#define CAC_SELECT_TITLE_BASELINE 51.5f
#define CAC_SELECT_TITLE_CAP 11.0f
#define CAC_SELECT_TITLE_TRACKING 0.24f
/* The tabs, 90 wide and 5.8 apart from 44.2, end 84.2 down: the chosen one
 * filled from 68.3 with dark Bold capitals 6.3 tall, the other framed from
 * 70.8 with Regular ones 5.6 tall. */
#define CAC_SELECT_TAB_LEFT 44.2f
#define CAC_SELECT_TAB_WIDTH 90.0f
#define CAC_SELECT_TAB_GAP 5.8f
#define CAC_SELECT_TAB_TOP 68.3f
#define CAC_SELECT_TAB_IDLE_TOP 70.8f
#define CAC_SELECT_TAB_BOTTOM 84.2f
#define CAC_SELECT_TAB_CAP 6.3f
#define CAC_SELECT_TAB_TRACKING 0.0f
#define CAC_SELECT_TAB_IDLE_CAP 5.6f
#define CAC_SELECT_TAB_IDLE_TRACKING 0.14f
/* The list's rows, 29.2 apart from 98.3 down: Regular capitals 7.5 tall from
 * 53.8; the focused row framed from the screen's left edge to 230, its name
 * in gold Bold 7.8 tall, 8 further right and a unit lower. */
#define CAC_SELECT_ROW_TOP 98.3f
#define CAC_SELECT_ROW_PITCH 29.2f
#define CAC_SELECT_ROW_HEIGHT 28.0f
#define CAC_SELECT_ROW_WIDTH 230.0f
#define CAC_SELECT_ROW_LEFT 53.8f
#define CAC_SELECT_ROW_BASELINE 15.6f
#define CAC_SELECT_ROW_CAP 7.5f
#define CAC_SELECT_ROW_TRACKING 0.19f
#define CAC_SELECT_FOCUS_INDENT 8.0f
#define CAC_SELECT_FOCUS_BASELINE 16.6f
#define CAC_SELECT_FOCUS_CAP 7.8f
#define CAC_SELECT_FOCUS_TRACKING 0.23f
#define CAC_SELECT_LINE 0.8f
/* The loadout takes the place of the capture's Division column, 20 right of
 * the list, without it: tiles 122.5, 82.5 and 37.5 square, the tactical one
 * a column of 125 to the right; each slot's caption, Regular capitals 5.2
 * tall, 15.5 above its tile and its weapon's name, Bold 7.25 tall, 5 above. */
#define CAC_SELECT_LOADOUT_LEFT 250.0f
#define CAC_SELECT_LOADOUT_COLUMN 125.0f
#define CAC_SELECT_PRIMARY_TOP 91.7f
#define CAC_SELECT_PRIMARY_SIZE 122.5f
#define CAC_SELECT_SECONDARY_TOP 245.0f
#define CAC_SELECT_SECONDARY_SIZE 82.5f
#define CAC_SELECT_EQUIPMENT_TOP 356.7f
#define CAC_SELECT_EQUIPMENT_SIZE 37.5f
#define CAC_SELECT_CAPTION_ABOVE 15.5f
#define CAC_SELECT_CAPTION_CAP 5.2f
#define CAC_SELECT_CAPTION_TRACKING 0.02f
#define CAC_SELECT_NAME_ABOVE 5.0f
#define CAC_SELECT_NAME_CAP 7.25f
#define CAC_SELECT_NAME_TRACKING 0.08f
/* Its prompts 20.9 lower than the main menu's, 53.3 from the left edge and
 * 54.1 from the right. */
#define CAC_SELECT_PROMPT_LEFT 53.3f
#define CAC_SELECT_PROMPT_RIGHT 54.1f
#define CAC_SELECT_PROMPT_LOWER 20.9f

/* The attachment boxes under each gun's camouflage line, and WWII's grid of
 * three attachment tiles a row. */
#define CAC_ATTACHMENT_BOX_TOP (1.5f + CAC_CAMO_LINE_HEIGHT + 2.5f)
#define CAC_ATTACHMENT_BOX_HEIGHT 20.0f
#define CAC_ATTACHMENT_BOX_GAP 4.0f
#define CAC_ATTACHMENT_COLUMNS 3
#define CAC_ATTACHMENT_TILE_WIDTH 50.0f
#define CAC_ATTACHMENT_TILE_HEIGHT 50.0f
#define CAC_ATTACHMENT_TILE_GAP 3.0f
#define CAC_EFFECT_LINES 6

/* The pictures render-class-pictures.py lays on a gun's canvas, in drawing order: its longer
 * magazine in place of its own, then what sits on it. */
static const struct {
    int attachment;
    const char *suffix;
} s_cacLayers[] = {
    { ATT_EXTMAG, "extmag" }, { ATT_SUPPRESSOR, "suppressor" }, { ATT_REFLEX, "reflex" },
    { ATT_LENS, "lens" },     { ATT_OPTIC4X, "optic4x" },
};
#define CAC_LAYER_COUNT ((int)(sizeof(s_cacLayers) / sizeof(s_cacLayers[0])))

enum {
    CAC_SCREEN_LIST, CAC_SCREEN_EDIT, CAC_SCREEN_WEAPONS, CAC_SCREEN_EQUIPMENT, CAC_SCREEN_CAMO,
    CAC_SCREEN_ATTACHMENTS
};
/* The editor's focus: the four slots, then the primary's attachment boxes,
 * then the secondary's. */
enum {
    CAC_FOCUS_PRIMARY_ATTACHMENT = CLASS_SLOT_COUNT,
    CAC_FOCUS_SECONDARY_ATTACHMENT = CAC_FOCUS_PRIMARY_ATTACHMENT + ATT_PRIMARY_MAX,
    CAC_FOCUS_COUNT = CAC_FOCUS_SECONDARY_ATTACHMENT + ATT_SECONDARY_MAX
};
enum { CAC_STORE_LOADING, CAC_STORE_READY, CAC_STORE_NO_NAME, CAC_STORE_OFFLINE };
/* Select Class's tabs: the player's classes, then the game's own. */
enum { CAC_TAB_CUSTOM, CAC_TAB_STANDARD, CAC_TAB_COUNT };
/* Hover values from here on are the tabs, after the rows. */
#define CAC_HOVER_TAB 100
/* The editor's camouflage lines, after its tiles. */
#define CAC_HOVER_CAMO 50

typedef struct {
    float x, y, w, h;
    int horz;
} CacRect;

static struct {
    int open;
    int choose;
    int screen;
    int tab;
    int classFocus;
    /* The player's class chosen last, or the game's; -1 for none. */
    int chosen;
    int chosenTab;
    /* The editor was opened from the pause menu, and goes back to it. */
    int fromPause;
    int slotFocus;
    int pickSlot;
    int pickFocus;
    int pickScroll;
    /* The gun whose camouflage is being chosen, and the focused one. */
    int camoSlot;
    int camoFocus;
    /* The gun and attachment position being modified, and the grid's focus
     * among the gun's attachments. */
    int attachSlot;
    int attachPosition;
    int attachFocus;
    int hover;
    int cursorX, cursorY;
    int renaming;
    char rename[CLASS_NAME_MAX + 1];
    ClassLoadout classes[CLASS_COUNT];
    /* Standard Issue: every gametag's first classes, as the game gives them. */
    ClassLoadout standard[CLASS_COUNT];
    int initialized;
    int store;
    int saveFailed;
    int generation;
    char player[64];
    int statsRead;
    ClassStats stats[64];
    float lowest[CLASS_STAT_COUNT];
    float highest[CLASS_STAT_COUNT];
    int imageGeneration;
    MaterialHandle images[64];
    float aspects[64];
    float st[64][4];
    /* Drawn in their own colors. */
    int pictures[64];
    /* Each gun's attachment layers on its picture's canvas and each attachment's icon, read once
     * an image generation. */
    MaterialHandle layers[64][CAC_LAYER_COUNT];
    float layerSt[64][CAC_LAYER_COUNT][4];
    unsigned char layerRead[64][CAC_LAYER_COUNT];
    MaterialHandle icons[ATT_COUNT];
    float iconSt[ATT_COUNT][4];
    float iconAspects[ATT_COUNT];
    unsigned char iconRead[ATT_COUNT];
} s_cac;

static const char *const s_cacGroups[] = {
    "Submachine Gun", "Rifle", "Automatic Rifle", "Sniper Rifle", "Shotgun", "Pistol",
    "Lethal Fragmentation Grenade", "Tactical Smoke Grenade", "Lethal Proximity Mine", "Lethal Remote Charge",
};
static const char *const s_cacNations[] = { "United States", "United Kingdom", "Soviet Union", "Germany", NULL };
static const char *const s_cacSlots[] = { "Primary", "Secondary", "Lethal", "Tactical" };
static const char *const s_cacStats[] = { "Damage", "Range", "Accuracy", "Fire Rate", "Mobility" };
static const vec4_t s_cacRow = { 0.0f, 0.0f, 0.0f, 0.45f };
static const vec4_t s_cacRowFocus = { 0.32f, 0.29f, 0.22f, 0.55f };
static const vec4_t s_cacTile = { 0.0f, 0.0f, 0.0f, 0.5f };
static const vec4_t s_cacEdge = { 1.0f, 1.0f, 1.0f, 0.22f };
static const vec4_t s_cacBar = { 1.0f, 1.0f, 1.0f, 0.18f };
static const vec4_t s_cacShade = { 0.0f, 0.0f, 0.0f, 0.55f };
static const vec4_t s_cacNote = { 0.70f, 0.70f, 0.68f, 1.0f };
/* Select Class over the gray scene: a lighter shade, the main menu's wash
 * darkening the left; the capture's cream tab #d4cfc4 with near-black
 * lettering, the other tab's light frame, the focused row's faint fill and
 * gold frame, and the guns' tiles a little lighter than the scene. */
static const vec4_t s_cacSelectShade = { 0.0f, 0.0f, 0.0f, 0.3f };
static const vec4_t s_cacSelectTab = { 0.831f, 0.812f, 0.769f, 1.0f };
static const vec4_t s_cacSelectTabText = { 0.09f, 0.09f, 0.09f, 1.0f };
static const vec4_t s_cacSelectTabEdge = { 1.0f, 1.0f, 1.0f, 0.55f };
static const vec4_t s_cacSelectRow = { 1.0f, 1.0f, 1.0f, 0.05f };
static const vec4_t s_cacSelectRowEdge = { 0.80f, 0.72f, 0.47f, 0.85f };
static const vec4_t s_cacSelectTile = { 1.0f, 1.0f, 1.0f, 0.07f };
/* WWII's green and red for a stat an attachment raises or lowers, and its
 * yellow for what keeps an attachment off. */
static const vec4_t s_cacBetter = { 0.45f, 0.76f, 0.38f, 1.0f };
static const vec4_t s_cacWorse = { 0.82f, 0.33f, 0.27f, 1.0f };
static const vec4_t s_cacWarning = { 0.91f, 0.76f, 0.27f, 1.0f };
static const vec4_t s_cacDisabled = { 0.70f, 0.70f, 0.68f, 0.35f };
static const vec4_t s_cacIconBlocked = { 1.0f, 1.0f, 1.0f, 0.3f };
/* The attachment grid's tiles, the capture's warm dark gray behind WWII's icons. */
static const vec4_t s_cacAttachTile = { 0.15f, 0.14f, 0.125f, 0.9f };

static int Cac_Ready(void)
{
    if (!s_cac.initialized) {
        ClassData_Defaults(s_cac.classes);
        ClassData_Defaults(s_cac.standard);
        s_cac.initialized = 1;
        s_cac.chosen = -1;
    }
    return 1;
}

/* The classes the list shows: the player's, or the game's own on a match's
 * Standard Issue tab. */
static const ClassLoadout *Cac_Listed(void)
{
    return s_cac.choose && s_cac.tab == CAC_TAB_STANDARD ? s_cac.standard : s_cac.classes;
}

static const char *Cac_Translate(const char *reference, const char *fallback)
{
    const char *text = reference ? UI_SafeTranslateString(reference) : NULL;

    return text && text[0] && text[0] != '@' ? text : fallback;
}

static const char *Cac_WeaponName(int index)
{
    return index < 0 ? "" : Cac_Translate(g_classWeapons[index].displayName, g_classWeapons[index].weapon);
}

static void Cac_Upper(const char *text, char *out, int size)
{
    int i;

    for (i = 0; text[i] && i < size - 1; i++)
        out[i] = (char)(text[i] >= 'a' && text[i] <= 'z' ? text[i] - 32 : text[i]);
    out[i] = '\0';
}

static CacRect Cac_Rect(float x, float y, float w, float h, int horz)
{
    CacRect rect;

    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    rect.horz = horz;
    return rect;
}

static int Cac_Hover(const displayContextDef_t *dc, const CacRect *rect)
{
    float x = rect->x, y = rect->y, w = rect->w, h = rect->h;
    float cx = (float)dc->cursorx, cy = (float)dc->cursory;

    CalcScreenPlacement(&x, &y, &w, &h, rect->horz, CAC_ALIGN_TOP);
    CalcScreenX(&cx, CAC_ALIGN_FULLSCREEN);
    CalcScreenY(&cy, CAC_ALIGN_FULLSCREEN);
    return cx >= x && cx < x + w && cy >= y && cy < y + h;
}

static void Cac_Fill(const CacRect *rect, const vec_t *color)
{
    CL_DrawStretchPic(rect->x, rect->y, rect->w, rect->h, rect->horz, CAC_ALIGN_TOP, 0.0f, 0.0f, 1.0f, 1.0f, color,
                      sharedUiInfo.assets.whiteMaterial);
}

static void Cac_FrameWidth(const CacRect *rect, const vec_t *color, float line)
{
    CacRect edge = *rect;

    edge.h = line;
    Cac_Fill(&edge, color);
    edge.y = rect->y + rect->h - line;
    Cac_Fill(&edge, color);
    edge.y = rect->y;
    edge.h = rect->h;
    edge.w = line;
    Cac_Fill(&edge, color);
    edge.x = rect->x + rect->w - line;
    Cac_Fill(&edge, color);
}

static void Cac_Frame(const CacRect *rect, const vec_t *color)
{
    Cac_FrameWidth(rect, color, CAC_LINE);
}

static void Cac_Text(const char *text, int bold, float x, float baseline, int horz, float cap, float tracking,
                     const vec_t *color)
{
    UI_WWIIMenu_Text(text, bold, x, baseline, horz, CAC_ALIGN_TOP, cap, tracking, color);
}

static void Cac_Caps(const char *text, int bold, float x, float baseline, int horz, float cap, float tracking,
                     const vec_t *color)
{
    char upper[128];

    Cac_Upper(text, upper, sizeof(upper));
    Cac_Text(upper, bold, x, baseline, horz, cap, tracking, color);
}

/* Words across lines no wider than width; the next baseline. */
static float Cac_Wrapped(const char *text, float x, float baseline, int horz, float width, float cap, int lines,
                         const vec_t *color)
{
    char line[256];
    int length = 0;

    while (*text && lines > 0) {
        const char *word = text;
        int wordLength = 0;

        while (word[wordLength] && word[wordLength] != ' ')
            wordLength++;
        if (length && length + 1 + wordLength < (int)sizeof(line)) {
            char trial[256];

            memcpy(trial, line, (size_t)length);
            trial[length] = ' ';
            memcpy(trial + length + 1, word, (size_t)wordLength);
            trial[length + 1 + wordLength] = '\0';
            if (UI_WWIIMenu_Width(trial, 0, cap, 0.0f) > width) {
                Cac_Text(line, 0, x, baseline, horz, cap, 0.0f, color);
                baseline += cap * 2.1f;
                length = 0;
                if (--lines <= 0)
                    break;
            }
        }
        if (length)
            line[length++] = ' ';
        if (length + wordLength >= (int)sizeof(line))
            wordLength = (int)sizeof(line) - 1 - length;
        memcpy(line + length, word, (size_t)wordLength);
        length += wordLength;
        line[length] = '\0';
        text = word + wordLength;
        while (*text == ' ')
            text++;
    }
    if (length && lines > 0) {
        Cac_Text(line, 0, x, baseline, horz, cap, 0.0f, color);
        baseline += cap * 2.1f;
    }
    return baseline;
}

/* Pictures read before the renderer last restarted are gone. */
static void Cac_ImagesFresh(void)
{
    if (s_cac.imageGeneration != R_GetHudImageGeneration()) {
        s_cac.imageGeneration = R_GetHudImageGeneration();
        memset(s_cac.images, 0, sizeof(s_cac.images));
        memset(s_cac.layerRead, 0, sizeof(s_cac.layerRead));
        memset(s_cac.iconRead, 0, sizeof(s_cac.iconRead));
    }
}

/* A weapon's picture, its texture coordinates and its width over height: the
 * owner's picture of the weapon when there is one, else the retail menu image
 * for a primary, else the HUD icon's silhouette, cropped to its pixels. */
static MaterialHandle Cac_Image(int index, float *aspect, const float **st)
{
    Cac_ImagesFresh();
    if (index < 0 || index >= g_classWeaponCount || index >= (int)(sizeof(s_cac.images) / sizeof(s_cac.images[0])))
        return NULL;
    if (!s_cac.images[index]) {
        char name[64];
        MaterialHandle image;
        int i;

        snprintf(name, sizeof(name), "browser_class/%s", g_classWeapons[index].weapon);
        for (i = 0; name[i]; i++)
            name[i] = (char)(name[i] >= 'A' && name[i] <= 'Z' ? name[i] + 32 : name[i]);
        image = R_GetHudPictureMaterial(name, s_cac.st[index], &s_cac.aspects[index]);
        s_cac.pictures[index] = image != NULL;
        if (image) {
            s_cac.images[index] = image;
            *aspect = s_cac.aspects[index];
            *st = s_cac.st[index];
            return image;
        }
        s_cac.st[index][0] = s_cac.st[index][1] = 0.0f;
        s_cac.st[index][2] = s_cac.st[index][3] = 1.0f;
        image = CL_RegisterMaterialNoMip(g_classWeapons[index].image, 3);
        s_cac.aspects[index] = CAC_RETAIL_IMAGE_ASPECT;
        if (g_classWeapons[index].slot != CLASS_SLOT_PRIMARY && image) {
            float cropped = 0.0f;
            MaterialHandle silhouette = R_GetHudSilhouetteMaterial(image, &cropped);

            if (silhouette && cropped > 0.0f) {
                image = silhouette;
                s_cac.aspects[index] = cropped;
            } else {
                s_cac.aspects[index] = 1.0f;
            }
        }
        s_cac.images[index] = image;
    }
    *aspect = s_cac.aspects[index];
    *st = s_cac.st[index];
    return s_cac.images[index];
}

/* The picture as large as it fits in the box, centered; an owner's picture
 * keeps its colors, a silhouette takes color. */
static void Cac_Picture(int index, const CacRect *box, const vec_t *color)
{
    float aspect = 1.0f, w, h;
    const float *st = NULL;
    MaterialHandle image = Cac_Image(index, &aspect, &st);

    if (!image || aspect <= 0.0f)
        return;
    w = box->w;
    h = w / aspect;
    if (h > box->h) {
        h = box->h;
        w = h * aspect;
    }
    CL_DrawStretchPic(box->x + 0.5f * (box->w - w), box->y + 0.5f * (box->h - h), w, h, box->horz, CAC_ALIGN_TOP,
                      st[0], st[1], st[2], st[3], s_cac.pictures[index] ? UI_WWIIMenu_Color(WWII_COLOR_WHITE) : color,
                      image);
}

/* A picture by name under browser_class/, lowercased, its visible texels' coordinates in st. */
static MaterialHandle Cac_ClassPicture(const char *name, float *st, float *aspect)
{
    char path[96];
    int i;

    snprintf(path, sizeof(path), "browser_class/%s", name);
    for (i = 0; path[i]; i++)
        path[i] = (char)(path[i] >= 'A' && path[i] <= 'Z' ? path[i] + 32 : path[i]);
    return R_GetHudPictureMaterial(path, st, aspect);
}

/* A gun's layer on the canvas of its picture, which Cac_Image read first. */
static MaterialHandle Cac_Layer(int index, int layer, const float **st)
{
    if (!s_cac.layerRead[index][layer]) {
        char name[80];
        float aspect = 0.0f;

        s_cac.layerRead[index][layer] = 1;
        snprintf(name, sizeof(name), "%s__%s", g_classWeapons[index].weapon, s_cacLayers[layer].suffix);
        s_cac.layers[index][layer] = Cac_ClassPicture(name, s_cac.layerSt[index][layer], &aspect);
    }
    *st = s_cac.layerSt[index][layer];
    return s_cac.layers[index][layer];
}

/* An attachment's icon, WWII's cut from the owner's captures; NULL without one. */
static MaterialHandle Cac_AttachmentIcon(int attachment, const float **st, float *aspect)
{
    Cac_ImagesFresh();
    if (attachment < 0 || attachment >= ATT_COUNT)
        return NULL;
    if (!s_cac.iconRead[attachment]) {
        char name[64];

        s_cac.iconRead[attachment] = 1;
        snprintf(name, sizeof(name), "attachment__%s", g_attachments[attachment].name);
        s_cac.icons[attachment] = Cac_ClassPicture(name, s_cac.iconSt[attachment], &s_cac.iconAspects[attachment]);
    }
    *st = s_cac.iconSt[attachment];
    *aspect = s_cac.iconAspects[attachment];
    return s_cac.icons[attachment];
}

/* The icon as large as it fits in box scaled by fill, centered, in color. */
static int Cac_DrawIcon(int attachment, const CacRect *box, float fill, const vec_t *color)
{
    float aspect = 0.0f, w, h;
    const float *st = NULL;
    MaterialHandle icon = Cac_AttachmentIcon(attachment, &st, &aspect);

    if (!icon || aspect <= 0.0f)
        return 0;
    w = box->w * fill;
    h = w / aspect;
    if (h > box->h * fill) {
        h = box->h * fill;
        w = h * aspect;
    }
    CL_DrawStretchPic(box->x + 0.5f * (box->w - w), box->y + 0.5f * (box->h - h), w, h, box->horz, CAC_ALIGN_TOP,
                      st[0], st[1], st[2], st[3], color, icon);
    return 1;
}

/* A gun with its attachments, from the owner's picture and the layers on its canvas: the gun, or
 * its longer magazine's picture in its place, painted in the camouflage; then each attachment over
 * it; what is drawn framed as large as it fits in the box. 0 when the gun shows none of them. */
static int Cac_LayeredPicture(int index, int camo, unsigned attachments, MaterialHandle image, float aspect,
                              const float *st, const CacRect *box)
{
    MaterialHandle drawn[CAC_LAYER_COUNT + 1];
    const float *sts[CAC_LAYER_COUNT + 1];
    const WeaponCamoGun *gun;
    float span[4], canvas, wide, w, h, x, y;
    int count = 1, layer, i;

    drawn[0] = image;
    sts[0] = st;
    for (layer = 0; layer < CAC_LAYER_COUNT; layer++) {
        const float *at;
        MaterialHandle picture;

        if (!(attachments & ATT_BIT(s_cacLayers[layer].attachment)))
            continue;
        picture = Cac_Layer(index, layer, &at);
        if (!picture)
            continue;
        if (s_cacLayers[layer].attachment == ATT_EXTMAG) {
            drawn[0] = picture;
            sts[0] = at;
        } else {
            drawn[count] = picture;
            sts[count++] = at;
        }
    }
    if (drawn[0] == image && count == 1)
        return 0;
    gun = camo > WEAPON_CAMO_NONE ? BG_WeaponCamo_Gun(g_classWeapons[index].weapon) : NULL;
    if (gun) {
        float painted = 0.0f;
        MaterialHandle picture = R_GetWeaponCamoPicture(drawn[0], 0, camo, gun->style, &painted);

        if (picture && painted > 0.0f)
            drawn[0] = picture;
    }
    memcpy(span, sts[0], sizeof(span));
    for (i = 1; i < count; i++) {
        span[0] = sts[i][0] < span[0] ? sts[i][0] : span[0];
        span[1] = sts[i][1] < span[1] ? sts[i][1] : span[1];
        span[2] = sts[i][2] > span[2] ? sts[i][2] : span[2];
        span[3] = sts[i][3] > span[3] ? sts[i][3] : span[3];
    }
    /* The canvas's width over its height, from the gun's visible texels. */
    canvas = aspect * (st[3] - st[1]) / (st[2] - st[0]);
    wide = canvas * (span[2] - span[0]) / (span[3] - span[1]);
    w = box->w;
    h = w / wide;
    if (h > box->h) {
        h = box->h;
        w = h * wide;
    }
    x = box->x + 0.5f * (box->w - w);
    y = box->y + 0.5f * (box->h - h);
    for (i = 0; i < count; i++) {
        float fx = w / (span[2] - span[0]), fy = h / (span[3] - span[1]);

        CL_DrawStretchPic(x + (sts[i][0] - span[0]) * fx, y + (sts[i][1] - span[1]) * fy,
                          (sts[i][2] - sts[i][0]) * fx, (sts[i][3] - sts[i][1]) * fy, box->horz, CAC_ALIGN_TOP,
                          sts[i][0], sts[i][1], sts[i][2], sts[i][3], UI_WWIIMenu_Color(WWII_COLOR_WHITE), drawn[i]);
    }
    return 1;
}

/* A gun's picture wearing a camouflage and the attachments that show on it,
 * placed as Cac_Picture places it: the owner's picture or the retail image
 * painted, or the HUD icon's silhouette filled with the pattern, in their
 * colors. Without the camouflage, or until it can be painted, the plain
 * picture; attachments need the owner's picture and its layers. */
static void Cac_GunPicture(int index, int camo, unsigned attachments, const CacRect *box, const vec_t *color)
{
    float aspect = 1.0f, painted = 0.0f, w, h;
    const float *st = NULL;
    MaterialHandle image = Cac_Image(index, &aspect, &st), picture = NULL;
    const WeaponCamoGun *gun = NULL;
    int silhouette = 0;

    if (image && aspect > 0.0f && attachments && s_cac.pictures[index] &&
        g_classWeapons[index].slot <= CLASS_SLOT_SECONDARY &&
        Cac_LayeredPicture(index, camo, attachments, image, aspect, st, box))
        return;
    if (image && aspect > 0.0f && camo > WEAPON_CAMO_NONE && g_classWeapons[index].slot <= CLASS_SLOT_SECONDARY)
        gun = BG_WeaponCamo_Gun(g_classWeapons[index].weapon);
    if (gun && s_cac.pictures[index]) {
        picture = R_GetWeaponCamoPicture(image, 0, camo, gun->style, &painted);
    } else if (gun) {
        silhouette = g_classWeapons[index].slot != CLASS_SLOT_PRIMARY;
        picture = R_GetWeaponCamoPicture(CL_RegisterMaterialNoMip(g_classWeapons[index].image, 3), silhouette, camo,
                                         gun->style, &painted);
    }
    if (!picture || painted <= 0.0f) {
        Cac_Picture(index, box, color);
        return;
    }
    if (silhouette)
        aspect = painted;
    w = box->w;
    h = w / aspect;
    if (h > box->h) {
        h = box->h;
        w = h * aspect;
    }
    CL_DrawStretchPic(box->x + 0.5f * (box->w - w), box->y + 0.5f * (box->h - h), w, h, box->horz, CAC_ALIGN_TOP,
                      st[0], st[1], st[2], st[3], UI_WWIIMenu_Color(WWII_COLOR_WHITE), picture);
}

/* A camouflage's swatch: part of a tile of its pattern, so its spots read at
 * a swatch's size; nothing for the gun's own finish. */
static void Cac_CamoSwatch(int camo, const CacRect *rect)
{
    MaterialHandle pattern = R_GetWeaponCamoMaterial(camo);

    if (pattern)
        CL_DrawStretchPic(rect->x, rect->y, rect->w, rect->h, rect->horz, CAC_ALIGN_TOP, 0.0f, 0.0f, 0.55f, 0.55f,
                          UI_WWIIMenu_Color(WWII_COLOR_WHITE), pattern);
}

static void Cac_ReadStats(void)
{
    int i, stat;

    if (s_cac.statsRead)
        return;
    s_cac.statsRead = 1;
    for (stat = 0; stat < CLASS_STAT_COUNT; stat++) {
        s_cac.lowest[stat] = 0.0f;
        s_cac.highest[stat] = 0.0f;
    }
    for (i = 0; i < g_classWeaponCount && i < (int)(sizeof(s_cac.stats) / sizeof(s_cac.stats[0])); i++) {
        void *file = NULL;
        int length;

        memset(&s_cac.stats[i], 0, sizeof(s_cac.stats[i]));
        length = FS_ReadFile(va("weapons/mp/%s", g_classWeapons[i].weapon), &file);
        if (length > 0 && file)
            ClassData_ReadStats((const char *)file, &s_cac.stats[i]);
        if (file)
            FS_FreeFile(file);
        /* The bars compare the guns; a grenade only gives its fuse. */
        if (!s_cac.stats[i].valid || g_classWeapons[i].slot > CLASS_SLOT_SECONDARY)
            continue;
        for (stat = 0; stat < CLASS_STAT_COUNT; stat++) {
            float value = s_cac.stats[i].value[stat];
            int first = s_cac.lowest[stat] == 0.0f && s_cac.highest[stat] == 0.0f;

            if (first || value < s_cac.lowest[stat])
                s_cac.lowest[stat] = value;
            if (first || value > s_cac.highest[stat])
                s_cac.highest[stat] = value;
        }
    }
}

static void Cac_Store(void)
{
    char lines[1024];

    if (ClassData_Serialize(s_cac.classes, lines, sizeof(lines)) < 0)
        return;
    s_cac.saveFailed = 0;
#ifdef __EMSCRIPTEN__
    if (s_cac.store == CAC_STORE_READY)
        Web_ClassesSave(s_cac.player, lines, s_cac.generation);
#endif
}

static void Cac_Load(void)
{
    const char *player = Dvar_GetVariantString("name");

    Cac_Ready();
    if (!player)
        player = "";
    if (s_cac.store != CAC_STORE_LOADING && !strcmp(s_cac.player, player))
        return;
    snprintf(s_cac.player, sizeof(s_cac.player), "%s", player);
    s_cac.generation++;
    s_cac.store = CAC_STORE_LOADING;
#ifdef __EMSCRIPTEN__
    Web_ClassesLoad(s_cac.player, s_cac.generation);
#else
    s_cac.store = CAC_STORE_OFFLINE;
#endif
}

void UI_ClassMenu_Loaded(int generation, int status, const char *lines)
{
    if (generation != s_cac.generation)
        return;
    if (status == 200 && lines) {
        ClassData_Parse(lines, s_cac.classes);
        s_cac.store = CAC_STORE_READY;
    } else {
        s_cac.store = status == 400 ? CAC_STORE_NO_NAME : CAC_STORE_OFFLINE;
    }
}

void UI_ClassMenu_Saved(int generation, int status)
{
    if (generation == s_cac.generation)
        s_cac.saveFailed = status != 200;
}

void UI_ClassMenu_Open(displayContextDef_t *dc, int choose)
{
    Cac_Load();
    s_cac.open = 1;
    s_cac.choose = choose;
    s_cac.screen = CAC_SCREEN_LIST;
    s_cac.fromPause = 0;
    s_cac.renaming = 0;
    s_cac.hover = -1;
    s_cac.cursorX = dc->cursorx;
    s_cac.cursorY = dc->cursory;
    if (choose && s_cac.chosen >= 0) {
        s_cac.classFocus = s_cac.chosen;
        s_cac.tab = s_cac.chosenTab;
    }
    if (s_cac.classFocus < 0 || s_cac.classFocus >= CLASS_COUNT)
        s_cac.classFocus = 0;
}

static int Cac_MenuKind(const menuDef_t *menu)
{
    const char *name = menu ? menu->window.name : NULL;

    if (!name)
        return 0;
    if (!I_stricmp(name, "create_a_class"))
        return 1;
    if (!I_stricmp(name, "browser_class"))
        return 2;
    return 0;
}

static int Cac_SlotCandidates(int slot, int *indexes)
{
    return ClassData_SlotWeapons(slot, indexes, 64);
}

/* The attachments the class's gun in the slot takes, in table order. */
static int Cac_AttachmentCandidates(const ClassLoadout *loadout, int slot, int *attachments)
{
    /* WWII's grid order: its SMG's and its sniper's captures both follow it. */
    static const int order[ATT_COUNT] = {
        ATT_LENS, ATT_QUICKDRAW, ATT_REFLEX, ATT_OPTIC4X, ATT_CALIBRATION, ATT_GRIP, ATT_STEADYAIM,
        ATT_FMJ, ATT_HIGHCAL, ATT_RIFLING, ATT_EXTMAG, ATT_RAPIDFIRE, ATT_SUPPRESSOR,
    };
    unsigned allowed = loadout->weapon[slot] >= 0 ? BG_AttachmentsAllowed(g_classWeapons[loadout->weapon[slot]].weapon) : 0;
    int count = 0, i;

    for (i = 0; i < ATT_COUNT; i++) {
        if (allowed & ATT_BIT(order[i]))
            attachments[count++] = order[i];
    }
    return count;
}

/* Whether an editor focus is an attachment box, and which. */
static int Cac_FocusAttachment(int focus, int *slot, int *position)
{
    if (focus >= CAC_FOCUS_SECONDARY_ATTACHMENT && focus < CAC_FOCUS_COUNT) {
        *slot = CLASS_SLOT_SECONDARY;
        *position = focus - CAC_FOCUS_SECONDARY_ATTACHMENT;
        return 1;
    }
    if (focus >= CAC_FOCUS_PRIMARY_ATTACHMENT && focus < CAC_FOCUS_SECONDARY_ATTACHMENT) {
        *slot = CLASS_SLOT_PRIMARY;
        *position = focus - CAC_FOCUS_PRIMARY_ATTACHMENT;
        return 1;
    }
    return 0;
}

/* The class's weapon as an index among its slot's candidates. */
static int Cac_CandidateOf(int slot, int weapon)
{
    int indexes[64], count = Cac_SlotCandidates(slot, indexes), i;

    for (i = 0; i < count; i++) {
        if (indexes[i] == weapon)
            return i;
    }
    return 0;
}

static void Cac_OpenPicker(int slot)
{
    ClassLoadout *loadout = &s_cac.classes[s_cac.classFocus];

    s_cac.pickSlot = slot;
    s_cac.pickFocus = Cac_CandidateOf(slot, loadout->weapon[slot]);
    s_cac.pickScroll = s_cac.pickFocus - CAC_PICK_VISIBLE / 2;
    s_cac.screen = slot <= CLASS_SLOT_SECONDARY ? CAC_SCREEN_WEAPONS : CAC_SCREEN_EQUIPMENT;
    s_cac.hover = -1;
}

/* The grid opens on the attachment in the box, or the first. */
static void Cac_OpenAttachments(int slot, int position)
{
    const ClassLoadout *loadout = &s_cac.classes[s_cac.classFocus];
    int attachments[ATT_COUNT], count = Cac_AttachmentCandidates(loadout, slot, attachments), i;

    if (!count)
        return;
    if (position > ClassData_AttachmentCount(loadout, slot))
        position = ClassData_AttachmentCount(loadout, slot);
    s_cac.attachSlot = slot;
    s_cac.attachPosition = position;
    s_cac.attachFocus = 0;
    for (i = 0; i < count; i++) {
        if (position < ClassData_AttachmentCount(loadout, slot) && attachments[i] == loadout->attachment[slot][position])
            s_cac.attachFocus = i;
    }
    s_cac.screen = CAC_SCREEN_ATTACHMENTS;
    s_cac.hover = -1;
}

/* A gun's camouflage picker, on the one it wears. */
static void Cac_OpenCamo(int slot)
{
    s_cac.camoSlot = slot;
    s_cac.camoFocus = s_cac.classes[s_cac.classFocus].camo[slot];
    s_cac.screen = CAC_SCREEN_CAMO;
    s_cac.hover = -1;
}

static void Cac_EquipCamo(int camo)
{
    ClassLoadout *loadout = &s_cac.classes[s_cac.classFocus];

    if (camo < 0 || camo >= WEAPON_CAMO_COUNT)
        return;
    if (loadout->camo[s_cac.camoSlot] != camo) {
        loadout->camo[s_cac.camoSlot] = camo;
        Cac_Store();
    }
    s_cac.screen = CAC_SCREEN_EDIT;
    s_cac.slotFocus = s_cac.camoSlot;
    s_cac.hover = -1;
}

/* Rows down or up the swatches, in the same column; past the last row with
 * that column, around to the first. */
static void Cac_CamoRows(int rows)
{
    int column = s_cac.camoFocus % CAC_CAMO_COLUMNS, row = s_cac.camoFocus / CAC_CAMO_COLUMNS;
    const int rowCount = (WEAPON_CAMO_COUNT + CAC_CAMO_COLUMNS - 1) / CAC_CAMO_COLUMNS;

    do
        row = (row + rows + rowCount) % rowCount;
    while (row * CAC_CAMO_COLUMNS + column >= WEAPON_CAMO_COUNT);
    s_cac.camoFocus = row * CAC_CAMO_COLUMNS + column;
}

/* The class as the browser_class menu's response, wherever it was chosen. */
static int Cac_Respond(const ClassLoadout *loadout)
{
    char response[256];
    int menuIndex = -1, i;

    if (ClassData_Response(loadout, response, sizeof(response)) < 0)
        return 0;
    for (i = 0; i < 0x20; i++) {
        const char *menu = CL_GetConfigString(0x4de + i);

        if (menu && menu[0] && !I_stricmp(menu, "browser_class")) {
            menuIndex = i;
            break;
        }
    }
    if (menuIndex < 0)
        return 0;
    Cbuf_ExecuteText(2, va("cmd mr %i %i %s\n", Dvar_GetInt("sv_serverId"), menuIndex, response));
    return 1;
}

static void Cac_Choose(displayContextDef_t *dc)
{
    if (!Cac_Respond(&Cac_Listed()[s_cac.classFocus]))
        return;
    s_cac.chosen = s_cac.classFocus;
    s_cac.chosenTab = s_cac.tab;
    s_cac.open = 0;
    Menus_CloseByName(dc, "browser_class");
}

/* The editor, focused on an attachment box of the gun. */
static void Cac_ReturnToAttachment(int slot, int position)
{
    s_cac.screen = CAC_SCREEN_EDIT;
    s_cac.slotFocus =
        (slot == CLASS_SLOT_PRIMARY ? CAC_FOCUS_PRIMARY_ATTACHMENT : CAC_FOCUS_SECONDARY_ATTACHMENT) + position;
}

static void Cac_Back(displayContextDef_t *dc)
{
    if (s_cac.screen == CAC_SCREEN_ATTACHMENTS) {
        Cac_ReturnToAttachment(s_cac.attachSlot, s_cac.attachPosition);
    } else if (s_cac.screen == CAC_SCREEN_WEAPONS || s_cac.screen == CAC_SCREEN_EQUIPMENT ||
               s_cac.screen == CAC_SCREEN_CAMO) {
        s_cac.screen = CAC_SCREEN_EDIT;
    } else if (s_cac.screen == CAC_SCREEN_EDIT && s_cac.fromPause) {
        s_cac.fromPause = 0;
        s_cac.open = 0;
        UI_PauseMenu_Return(dc);
        Menus_CloseByName(dc, "browser_class");
    } else if (s_cac.screen == CAC_SCREEN_EDIT) {
        s_cac.screen = CAC_SCREEN_LIST;
    } else {
        s_cac.open = 0;
        if (s_cac.choose) {
            Menus_CloseByName(dc, "browser_class");
        } else {
            Menus_CloseByName(dc, "create_a_class");
            Menus_OpenByName(dc, "main_text");
        }
    }
    s_cac.hover = -1;
}

static void Cac_Equip(int candidate)
{
    int indexes[64], count = Cac_SlotCandidates(s_cac.pickSlot, indexes);
    ClassLoadout *loadout = &s_cac.classes[s_cac.classFocus];

    if (candidate < 0 || candidate >= count)
        return;
    if (loadout->weapon[s_cac.pickSlot] != indexes[candidate]) {
        ClassData_SetWeapon(loadout, s_cac.pickSlot, indexes[candidate]);
        Cac_Store();
    }
    s_cac.screen = CAC_SCREEN_EDIT;
    s_cac.slotFocus = s_cac.pickSlot;
    s_cac.hover = -1;
}

/* What keeps the focused attachment off the gun at the position: the
 * attachment itself in another box, another optic, or -1. */
static int Cac_AttachmentBlocker(const ClassLoadout *loadout, int attachment)
{
    unsigned others = ClassData_AttachmentMask(loadout, s_cac.attachSlot);

    if (s_cac.attachPosition < ClassData_AttachmentCount(loadout, s_cac.attachSlot))
        others &= ~ATT_BIT(loadout->attachment[s_cac.attachSlot][s_cac.attachPosition]);
    return BG_AttachmentConflict(others, attachment);
}

static void Cac_EquipAttachment(int candidate)
{
    ClassLoadout *loadout = &s_cac.classes[s_cac.classFocus];
    int attachments[ATT_COUNT], count = Cac_AttachmentCandidates(loadout, s_cac.attachSlot, attachments);
    int slot = s_cac.attachSlot, position = s_cac.attachPosition;

    if (candidate < 0 || candidate >= count || Cac_AttachmentBlocker(loadout, attachments[candidate]) >= 0)
        return;
    if (position >= ClassData_AttachmentCount(loadout, slot) || loadout->attachment[slot][position] != attachments[candidate]) {
        ClassData_EquipAttachment(loadout, slot, position, attachments[candidate]);
        Cac_Store();
    }
    Cac_ReturnToAttachment(slot, position < ClassData_AttachmentCount(loadout, slot) ? position
                                                                                     : ClassData_AttachmentCount(loadout, slot) - 1);
    s_cac.hover = -1;
}

static void Cac_RemoveAttachment(void)
{
    ClassLoadout *loadout = &s_cac.classes[s_cac.classFocus];

    if (s_cac.attachPosition >= ClassData_AttachmentCount(loadout, s_cac.attachSlot))
        return;
    ClassData_RemoveAttachment(loadout, s_cac.attachSlot, s_cac.attachPosition);
    Cac_Store();
    Cac_ReturnToAttachment(s_cac.attachSlot, s_cac.attachPosition);
    s_cac.hover = -1;
}

static void Cac_StartRename(void)
{
    snprintf(s_cac.rename, sizeof(s_cac.rename), "%s", s_cac.classes[s_cac.classFocus].name);
    s_cac.renaming = 1;
}

static void Cac_FinishRename(int save)
{
    char *name = s_cac.rename;
    size_t length = strlen(name);

    while (length && name[length - 1] == ' ')
        name[--length] = '\0';
    if (save && ClassData_ValidName(name) && strcmp(name, s_cac.classes[s_cac.classFocus].name)) {
        snprintf(s_cac.classes[s_cac.classFocus].name, sizeof(s_cac.classes[s_cac.classFocus].name), "%s", name);
        Cac_Store();
    }
    s_cac.renaming = 0;
}

/* The editor's focus in Tab order: each gun, then its attachment boxes, then
 * the grenades. */
static const int s_cacEditOrder[CAC_FOCUS_COUNT] = {
    CLASS_SLOT_PRIMARY,
    CAC_FOCUS_PRIMARY_ATTACHMENT,
    CAC_FOCUS_PRIMARY_ATTACHMENT + 1,
    CAC_FOCUS_PRIMARY_ATTACHMENT + 2,
    CLASS_SLOT_SECONDARY,
    CAC_FOCUS_SECONDARY_ATTACHMENT,
    CLASS_SLOT_LETHAL,
    CLASS_SLOT_TACTICAL,
};
/* Up, down, left and right of each editor focus, as the tiles lie: the guns'
 * column with their boxes under them, the grenades' column at its right. */
static const int s_cacEditNeighbours[CAC_FOCUS_COUNT][4] = {
    { CAC_FOCUS_SECONDARY_ATTACHMENT, CAC_FOCUS_PRIMARY_ATTACHMENT, CLASS_SLOT_LETHAL, CLASS_SLOT_LETHAL },
    { CAC_FOCUS_PRIMARY_ATTACHMENT, CAC_FOCUS_SECONDARY_ATTACHMENT, CLASS_SLOT_TACTICAL, CLASS_SLOT_TACTICAL },
    { CLASS_SLOT_TACTICAL, CLASS_SLOT_TACTICAL, CLASS_SLOT_PRIMARY, CLASS_SLOT_PRIMARY },
    { CLASS_SLOT_LETHAL, CLASS_SLOT_LETHAL, CLASS_SLOT_SECONDARY, CLASS_SLOT_SECONDARY },
    { CLASS_SLOT_PRIMARY, CLASS_SLOT_SECONDARY, CLASS_SLOT_LETHAL, CAC_FOCUS_PRIMARY_ATTACHMENT + 1 },
    { CLASS_SLOT_PRIMARY, CLASS_SLOT_SECONDARY, CAC_FOCUS_PRIMARY_ATTACHMENT, CAC_FOCUS_PRIMARY_ATTACHMENT + 2 },
    { CLASS_SLOT_PRIMARY, CLASS_SLOT_SECONDARY, CAC_FOCUS_PRIMARY_ATTACHMENT + 1, CLASS_SLOT_TACTICAL },
    { CLASS_SLOT_SECONDARY, CLASS_SLOT_PRIMARY, CLASS_SLOT_TACTICAL, CLASS_SLOT_TACTICAL },
};

static void Cac_Move(int delta)
{
    const ClassLoadout *loadout = &s_cac.classes[s_cac.classFocus];
    int indexes[64], count, i;

    switch (s_cac.screen) {
    case CAC_SCREEN_LIST:
        s_cac.classFocus = (s_cac.classFocus + delta + CLASS_COUNT) % CLASS_COUNT;
        break;
    case CAC_SCREEN_EDIT:
        for (i = 0; i < CAC_FOCUS_COUNT && s_cacEditOrder[i] != s_cac.slotFocus; i++)
            ;
        s_cac.slotFocus = s_cacEditOrder[(i + delta + CAC_FOCUS_COUNT) % CAC_FOCUS_COUNT];
        break;
    case CAC_SCREEN_ATTACHMENTS:
        count = Cac_AttachmentCandidates(loadout, s_cac.attachSlot, indexes);
        if (count)
            s_cac.attachFocus = (s_cac.attachFocus + delta + count) % count;
        break;
    case CAC_SCREEN_CAMO:
        s_cac.camoFocus = (s_cac.camoFocus + delta + WEAPON_CAMO_COUNT) % WEAPON_CAMO_COUNT;
        break;
    default:
        count = Cac_SlotCandidates(s_cac.pickSlot, indexes);
        s_cac.pickFocus = (s_cac.pickFocus + delta + count) % count;
        break;
    }
}

static void Cac_Navigate(int key)
{
    int vertical = key == CAC_KEY_UP || key == CAC_KEY_DOWN || key == CAC_KEY_WHEEL_UP || key == CAC_KEY_WHEEL_DOWN;
    int backward = key == CAC_KEY_UP || key == CAC_KEY_LEFT || key == CAC_KEY_WHEEL_UP;

    if (!vertical && s_cac.choose && s_cac.screen == CAC_SCREEN_LIST) {
        /* Select Class's tabs lie side by side; Tab goes to the other one. */
        if (key == CAC_KEY_TAB)
            s_cac.tab = s_cac.tab == CAC_TAB_CUSTOM ? CAC_TAB_STANDARD : CAC_TAB_CUSTOM;
        else
            s_cac.tab = key == CAC_KEY_RIGHT ? CAC_TAB_STANDARD : CAC_TAB_CUSTOM;
    } else if (key != CAC_KEY_TAB && s_cac.screen == CAC_SCREEN_EDIT) {
        int direction = vertical ? (backward ? 0 : 1) : (backward ? 2 : 3);

        if (s_cac.slotFocus >= 0 && s_cac.slotFocus < CAC_FOCUS_COUNT)
            s_cac.slotFocus = s_cacEditNeighbours[s_cac.slotFocus][direction];
    } else if (vertical && key != CAC_KEY_WHEEL_UP && key != CAC_KEY_WHEEL_DOWN &&
               s_cac.screen == CAC_SCREEN_ATTACHMENTS) {
        /* A row of the grid up or down, wrapping within the column. */
        const ClassLoadout *loadout = &s_cac.classes[s_cac.classFocus];
        int attachments[ATT_COUNT], count = Cac_AttachmentCandidates(loadout, s_cac.attachSlot, attachments);
        int rows = (count + CAC_ATTACHMENT_COLUMNS - 1) / CAC_ATTACHMENT_COLUMNS, step;

        for (step = 0; step < rows && count; step++) {
            s_cac.attachFocus += backward ? -CAC_ATTACHMENT_COLUMNS : CAC_ATTACHMENT_COLUMNS;
            if (s_cac.attachFocus < 0)
                s_cac.attachFocus += rows * CAC_ATTACHMENT_COLUMNS;
            else if (s_cac.attachFocus >= rows * CAC_ATTACHMENT_COLUMNS)
                s_cac.attachFocus -= rows * CAC_ATTACHMENT_COLUMNS;
            if (s_cac.attachFocus < count)
                break;
        }
    } else if (vertical && s_cac.screen == CAC_SCREEN_EQUIPMENT) {
        Cac_OpenPicker(s_cac.pickSlot == CLASS_SLOT_LETHAL ? CLASS_SLOT_TACTICAL : CLASS_SLOT_LETHAL);
    } else if (s_cac.screen == CAC_SCREEN_CAMO && (key == CAC_KEY_UP || key == CAC_KEY_DOWN)) {
        Cac_CamoRows(key == CAC_KEY_UP ? -1 : 1);
    } else {
        Cac_Move(key == CAC_KEY_UP || key == CAC_KEY_LEFT || key == CAC_KEY_WHEEL_UP ? -1 : 1);
    }
}

static void Cac_Activate(displayContextDef_t *dc)
{
    switch (s_cac.screen) {
    case CAC_SCREEN_LIST:
        if (s_cac.choose)
            Cac_Choose(dc);
        else
            s_cac.screen = CAC_SCREEN_EDIT;
        break;
    case CAC_SCREEN_EDIT: {
        int slot, position;

        if (Cac_FocusAttachment(s_cac.slotFocus, &slot, &position))
            Cac_OpenAttachments(slot, position);
        else
            Cac_OpenPicker(s_cac.slotFocus);
        break;
    }
    case CAC_SCREEN_ATTACHMENTS:
        Cac_EquipAttachment(s_cac.attachFocus);
        break;
    case CAC_SCREEN_CAMO:
        Cac_EquipCamo(s_cac.camoFocus);
        break;
    default:
        Cac_Equip(s_cac.pickFocus);
        break;
    }
}

/* The element under the cursor takes the focus it painted with. */
static void Cac_FocusHover(void)
{
    if (s_cac.hover < 0)
        return;
    switch (s_cac.screen) {
    case CAC_SCREEN_LIST:
        if (s_cac.hover < CLASS_COUNT)
            s_cac.classFocus = s_cac.hover;
        break;
    case CAC_SCREEN_EDIT:
        s_cac.slotFocus = s_cac.hover >= CAC_HOVER_CAMO ? s_cac.hover - CAC_HOVER_CAMO : s_cac.hover;
        break;
    case CAC_SCREEN_CAMO:
        s_cac.camoFocus = s_cac.hover;
        break;
    case CAC_SCREEN_EQUIPMENT:
        s_cac.pickSlot = s_cac.hover / CAC_EQUIP_COLUMNS ? CLASS_SLOT_LETHAL : CLASS_SLOT_TACTICAL;
        s_cac.pickFocus = s_cac.hover % CAC_EQUIP_COLUMNS;
        break;
    case CAC_SCREEN_ATTACHMENTS:
        s_cac.attachFocus = s_cac.hover;
        break;
    default:
        s_cac.pickFocus = s_cac.hover;
        break;
    }
}

/* What the menu has in focus: its screen and the element each screen
 * focuses. */
typedef struct {
    int screen, tab, classFocus, slotFocus, pickSlot, pickFocus, attachFocus, camoFocus;
} CacFocus;

static CacFocus Cac_Focus(void)
{
    CacFocus focus = { s_cac.screen,   s_cac.tab,       s_cac.classFocus,  s_cac.slotFocus,
                       s_cac.pickSlot, s_cac.pickFocus, s_cac.attachFocus, s_cac.camoFocus };

    return focus;
}

static int Cac_FocusMoved(const CacFocus *before)
{
    CacFocus now = Cac_Focus();

    return memcmp(before, &now, sizeof(now)) != 0;
}

static int Cac_NavigationKey(int key)
{
    return key == CAC_KEY_UP || key == CAC_KEY_DOWN || key == CAC_KEY_LEFT || key == CAC_KEY_RIGHT ||
           key == CAC_KEY_WHEEL_UP || key == CAC_KEY_WHEEL_DOWN || key == CAC_KEY_TAB;
}

int UI_ClassMenu_WantsKeys(const menuDef_t *menu)
{
    return Cac_MenuKind(menu) != 0;
}

static void Cac_Key(displayContextDef_t *dc, int key)
{
    int screen;

    if (s_cac.renaming) {
        if (key == CAC_KEY_ENTER || key == CAC_KEY_KP_ENTER)
            Cac_FinishRename(1);
        else if (key == CAC_KEY_ESCAPE || key == CAC_KEY_MOUSE2)
            Cac_FinishRename(0);
        else
            ClassData_EditName(s_cac.rename, key);
        return;
    }
    if (key & CAC_KEY_CHAR) {
        /* As the retail weapon menus' number keys: choose a class at once. */
        int number = (key & ~CAC_KEY_CHAR) - '1';

        if (s_cac.choose && s_cac.screen == CAC_SCREEN_LIST && number >= 0 && number < CLASS_COUNT) {
            s_cac.classFocus = number;
            Cac_Choose(dc);
        }
        return;
    }
    screen = s_cac.screen;
    switch (key) {
    case CAC_KEY_ESCAPE:
    case CAC_KEY_MOUSE2:
        Cac_Back(dc);
        break;
    case CAC_KEY_ENTER:
    case CAC_KEY_KP_ENTER:
        Cac_Activate(dc);
        break;
    case CAC_KEY_MOUSE1:
        if (s_cac.hover >= CAC_HOVER_TAB) {
            s_cac.tab = s_cac.hover - CAC_HOVER_TAB;
        } else if (s_cac.screen == CAC_SCREEN_EDIT && s_cac.hover >= CAC_HOVER_CAMO) {
            Cac_OpenCamo(s_cac.hover - CAC_HOVER_CAMO);
        } else if (s_cac.hover >= 0) {
            Cac_FocusHover();
            Cac_Activate(dc);
        }
        break;
    case CAC_KEY_F1:
        /* Only the player's own classes can be edited. */
        if (s_cac.screen == CAC_SCREEN_LIST && s_cac.choose) {
            if (s_cac.tab == CAC_TAB_CUSTOM)
                s_cac.screen = CAC_SCREEN_EDIT;
        } else if (s_cac.screen <= CAC_SCREEN_EDIT) {
            Cac_StartRename();
        }
        break;
    case CAC_KEY_F2:
    case CAC_KEY_BUTTON_Y:
        /* WWII's Customize: the focused gun's camouflage. */
        if (s_cac.screen == CAC_SCREEN_EDIT && s_cac.slotFocus <= CLASS_SLOT_SECONDARY)
            Cac_OpenCamo(s_cac.slotFocus);
        break;
    case CAC_KEY_BACKSPACE:
    case CAC_KEY_DEL:
        if (s_cac.screen == CAC_SCREEN_ATTACHMENTS)
            Cac_RemoveAttachment();
        break;
    case CAC_KEY_UP:
    case CAC_KEY_DOWN:
    case CAC_KEY_LEFT:
    case CAC_KEY_RIGHT:
    case CAC_KEY_WHEEL_UP:
    case CAC_KEY_WHEEL_DOWN:
    case CAC_KEY_TAB:
        Cac_Navigate(key);
        break;
    default:
        break;
    }
    /* The last frame's hover belongs to the screen it painted. */
    if (s_cac.screen != screen)
        s_cac.hover = -1;
}

int UI_ClassMenu_KeyEvent(displayContextDef_t *dc, menuDef_t *menu, int key, int down)
{
    int kind = Cac_MenuKind(menu), renaming;
    CacFocus focus;

    if (!kind)
        return 0;
    if (!s_cac.open)
        UI_ClassMenu_Open(dc, kind == 2);
    if (!down)
        return 1;
    focus = Cac_Focus();
    renaming = s_cac.renaming;
    Cac_Key(dc, key);
    /* The retail menus' two sounds: a click for what acts (choosing, opening,
     * going back, removing, renaming) and mouse_over as keys move the focus.
     * Typing a name, or a key that changes nothing, is silent. */
    if (!s_cac.open || s_cac.screen != focus.screen || s_cac.renaming != renaming)
        UI_PlayLocalSoundAliasByName("mouse_click");
    else if (Cac_FocusMoved(&focus))
        UI_PlayLocalSoundAliasByName(Cac_NavigationKey(key) ? "mouse_over" : "mouse_click");
    return 1;
}

static void Cac_Title(const char *first, const char *second)
{
    char title[96];

    if (second)
        snprintf(title, sizeof(title), "%s | %s", first, second);
    else
        snprintf(title, sizeof(title), "%s", first);
    Cac_Caps(title, 1, CAC_LEFT, CAC_TITLE_BASELINE, CAC_ALIGN_LEFT, CAC_TITLE_CAP, CAC_TITLE_TRACKING,
             UI_WWIIMenu_Color(WWII_COLOR_GOLD));
}

static void Cac_StatusNote(float baseline)
{
    const char *note = UI_ClassMenu_Note();

    if (note)
        Cac_Text(note, 0, CAC_LEFT, baseline, CAC_ALIGN_LEFT, CAC_ROW_NOTE_CAP, 0.0f, s_cacNote);
}

/* A slot's caption, the weapon's name above the tile, the tile and its
 * picture, a gun's in its camouflage; gold edges when focused. */
static void Cac_Tile(const CacRect *tile, int slot, int weapon, int camo, unsigned attachments, int focused)
{
    const vec_t *edge = focused ? UI_WWIIMenu_Color(WWII_COLOR_GOLD) : s_cacEdge;
    CacRect box = *tile;

    Cac_Caps(s_cacSlots[slot], 0, tile->x, tile->y - 11.0f, tile->horz, CAC_TILE_CAPTION_CAP, 0.12f, s_cacNote);
    Cac_Caps(Cac_WeaponName(weapon), 1, tile->x, tile->y - 3.0f, tile->horz, CAC_TILE_NAME_CAP, 0.08f,
             UI_WWIIMenu_Color(focused ? WWII_COLOR_GOLD : WWII_COLOR_WHITE));
    Cac_Fill(tile, s_cacTile);
    Cac_Frame(tile, edge);
    box.x += 0.08f * tile->w;
    box.y += 0.1f * tile->h;
    box.w *= 0.84f;
    box.h *= 0.8f;
    Cac_GunPicture(weapon, camo, attachments, &box, UI_WWIIMenu_Color(WWII_COLOR_WHITE));
}

/* The slot's camouflage, as the class keeps it. */
static int Cac_SlotCamo(const ClassLoadout *loadout, int slot)
{
    return slot < CLASS_CAMO_SLOTS ? loadout->camo[slot] : WEAPON_CAMO_NONE;
}

/* The slot's attachments, as the class keeps them; grenades take none. */
static unsigned Cac_SlotAttachments(const ClassLoadout *loadout, int slot)
{
    return slot <= CLASS_SLOT_SECONDARY ? ClassData_AttachmentMask(loadout, slot) : 0;
}

static const vec_t *Cac_ChangeColor(float before, float after)
{
    return after > before ? s_cacBetter : after < before ? s_cacWorse : UI_WWIIMenu_Color(WWII_COLOR_WHITE);
}

/* As WWII's bars: the gun with the base attachments in white, what the
 * preview set adds in green and what it takes away in red, a number in the
 * color of its change. */
static void Cac_Stats(int weapon, unsigned base, unsigned preview)
{
    const ClassStats *file = weapon >= 0 && weapon < (int)(sizeof(s_cac.stats) / sizeof(s_cac.stats[0]))
                                 ? &s_cac.stats[weapon]
                                 : NULL;
    const float x = -(CAC_SIDE_RIGHT + CAC_STAT_BAR);
    ClassStats before, after;
    char text[32];
    int stat;

    if (!file || !file->valid)
        return;
    ClassData_AttachedStats(file, g_classWeapons[weapon].weapon, base, &before);
    ClassData_AttachedStats(file, g_classWeapons[weapon].weapon, preview, &after);
    for (stat = 0; stat < CLASS_STAT_COUNT; stat++) {
        float top = CAC_STAT_TOP + (float)stat * CAC_STAT_ROW;
        float from =
            ClassData_AttachedRating(file->value[stat], before.value[stat], s_cac.lowest[stat], s_cac.highest[stat]);
        float to =
            ClassData_AttachedRating(file->value[stat], after.value[stat], s_cac.lowest[stat], s_cac.highest[stat]);
        int rating = (int)(to + 0.5f), was = (int)(from + 0.5f);
        CacRect bar = Cac_Rect(x, top + 2.0f, CAC_STAT_BAR, 1.2f, CAC_ALIGN_RIGHT);

        snprintf(text, sizeof(text), "%d", rating);
        Cac_Text(text, 1, x - 3.0f - UI_WWIIMenu_Width(text, 1, 7.0f, 0.0f), top + 1.5f, CAC_ALIGN_RIGHT, 7.0f, 0.0f,
                 Cac_ChangeColor((float)was, (float)rating));
        Cac_Caps(s_cacStats[stat], 0, x, top - 1.0f, CAC_ALIGN_RIGHT, 4.2f, 0.12f, UI_WWIIMenu_Color(WWII_COLOR_GOLD));
        Cac_Fill(&bar, s_cacBar);
        bar.w = CAC_STAT_BAR * 0.1f * (from < to ? from : to);
        Cac_Fill(&bar, s_cacNote);
        if (from != to) {
            bar.x += bar.w;
            bar.w = CAC_STAT_BAR * 0.1f * (from < to ? to - from : from - to);
            Cac_Fill(&bar, Cac_ChangeColor(from, to));
        }
    }
    Cac_Caps("Capacity", 0, x, CAC_STAT_TOP + 5.0f * CAC_STAT_ROW + 2.0f, CAC_ALIGN_RIGHT, 4.2f, 0.12f, s_cacNote);
    snprintf(text, sizeof(text), "%d/%d", after.clip, after.ammo);
    Cac_Text(text, 1, x, CAC_STAT_TOP + 5.0f * CAC_STAT_ROW + 12.0f, CAC_ALIGN_RIGHT, 7.0f, 0.0f,
             Cac_ChangeColor((float)before.clip, (float)after.clip));
}

/* An attachment's name in capitals, on one line or two, centred in a box. */
static void Cac_AttachmentName(const char *name, const CacRect *box, float cap, const vec_t *color)
{
    char upper[64], line[64];
    const char *space;
    float width = box->w - 6.0f, x = box->x + 3.0f, middle = box->y + 0.5f * box->h + 0.5f * cap;

    Cac_Upper(name, upper, sizeof(upper));
    space = strrchr(upper, ' ');
    if (UI_WWIIMenu_Width(upper, 0, cap, 0.06f) <= width || !space) {
        Cac_Text(upper, 0, x, middle, box->horz, cap, 0.06f, color);
        return;
    }
    /* The last word goes down, as "Full Metal / Jacket". */
    snprintf(line, sizeof(line), "%.*s", (int)(space - upper), upper);
    Cac_Text(line, 0, x, middle - 1.05f * cap, box->horz, cap, 0.06f, color);
    Cac_Text(space + 1, 0, x, middle + 1.05f * cap, box->horz, cap, 0.06f, color);
}

/* What the attachment does to the gun beside its others: its name, the
 * description and a line for each change in its tone. The next baseline. */
static float Cac_AttachmentDetail(int weapon, unsigned others, int attachment, float x, float baseline, int horz,
                                  float width)
{
    char lines[CAC_EFFECT_LINES][48];
    int tones[CAC_EFFECT_LINES], count = 0, i;
    ClassStats from, to;

    Cac_Caps(g_attachments[attachment].displayName, 1, x, baseline, horz, 10.0f, 0.06f,
             UI_WWIIMenu_Color(WWII_COLOR_WHITE));
    baseline = Cac_Wrapped(g_attachments[attachment].description, x, baseline + 14.0f, horz, width, 4.4f, 3,
                           s_cacNote);
    if (weapon >= 0 && weapon < (int)(sizeof(s_cac.stats) / sizeof(s_cac.stats[0])) && s_cac.stats[weapon].valid) {
        ClassData_AttachedStats(&s_cac.stats[weapon], g_classWeapons[weapon].weapon, others, &from);
        ClassData_AttachedStats(&s_cac.stats[weapon], g_classWeapons[weapon].weapon, others | ATT_BIT(attachment),
                                &to);
        count = ClassData_AttachmentEffects(&from.raw, &to.raw, lines, tones, CAC_EFFECT_LINES);
    }
    baseline += 3.0f;
    for (i = 0; i < count; i++, baseline += 4.4f * 2.1f)
        Cac_Text(lines[i], 1, x, baseline, horz, 4.4f, 0.0f,
                 tones[i] > 0 ? s_cacBetter : tones[i] < 0 ? s_cacWorse : UI_WWIIMenu_Color(WWII_COLOR_WHITE));
    return baseline;
}

/* Name, kind and origin, and the retail description; the baseline after them. */
static float Cac_Detail(int weapon, float x, float baseline, int horz, float width)
{
    char kind[96];
    const ClassWeapon *entry;

    if (weapon < 0)
        return baseline;
    entry = &g_classWeapons[weapon];
    Cac_Caps(Cac_WeaponName(weapon), 1, x, baseline, horz, 10.0f, 0.06f, UI_WWIIMenu_Color(WWII_COLOR_WHITE));
    if (s_cacNations[entry->nation])
        snprintf(kind, sizeof(kind), "%s, %s", s_cacGroups[entry->group], s_cacNations[entry->nation]);
    else
        snprintf(kind, sizeof(kind), "%s", s_cacGroups[entry->group]);
    Cac_Text(kind, 1, x, baseline + 11.0f, horz, 4.8f, 0.0f, UI_WWIIMenu_Color(WWII_COLOR_TITLE));
    if (entry->description)
        return Cac_Wrapped(Cac_Translate(entry->description, ""), x, baseline + 24.0f, horz, width, 4.4f, 5, s_cacNote);
    return baseline + 24.0f;
}

/* The gun's name and how many attachments it carries. */
static void Cac_GunNote(const ClassLoadout *loadout, int slot, char *out, int size)
{
    int count = ClassData_AttachmentCount(loadout, slot);

    if (count)
        snprintf(out, (size_t)size, "%s +%d", Cac_WeaponName(loadout->weapon[slot]), count);
    else
        snprintf(out, (size_t)size, "%s", Cac_WeaponName(loadout->weapon[slot]));
}

/* The gun's attachments by name, along the foot of its tile. */
static void Cac_AttachmentList(const ClassLoadout *loadout, int slot, const CacRect *tile)
{
    char text[128];
    int count = ClassData_AttachmentCount(loadout, slot), length = 0, i;

    text[0] = '\0';
    for (i = 0; i < count && length < (int)sizeof(text); i++)
        length += snprintf(text + length, sizeof(text) - (size_t)length, "%s%s", i ? ", " : "",
                           g_attachments[loadout->attachment[slot][i]].displayName);
    if (count)
        Cac_Caps(text, 0, tile->x + 4.0f, tile->y + tile->h - 2.5f, tile->horz, 3.2f, 0.08f, s_cacNote);
}

static void Cac_PaintList(displayContextDef_t *dc)
{
    const ClassLoadout *focused;
    float top = CAC_ROW_TOP;
    int i, slot;

    Cac_Title("Create a Class", NULL);
    for (i = 0; i < CLASS_COUNT; i++, top += CAC_ROW_HEIGHT) {
        const ClassLoadout *loadout = &s_cac.classes[i];
        CacRect row = Cac_Rect(CAC_LEFT, top, CAC_ROW_WIDTH, CAC_ROW_HEIGHT - 1.5f, CAC_ALIGN_LEFT);
        int isFocus = i == s_cac.classFocus;
        char note[96];

        if (Cac_Hover(dc, &row))
            s_cac.hover = i;
        Cac_Fill(&row, isFocus ? s_cacRowFocus : s_cacRow);
        if (isFocus)
            Cac_Frame(&row, UI_WWIIMenu_Color(WWII_COLOR_GOLD));
        Cac_Text(loadout->name, 1, CAC_LEFT + 6.0f, top + 12.5f, CAC_ALIGN_LEFT, CAC_ROW_NAME_CAP, 0.04f,
                 UI_WWIIMenu_Color(isFocus ? WWII_COLOR_GOLD : WWII_COLOR_WHITE));
        Cac_GunNote(loadout, CLASS_SLOT_PRIMARY, note, sizeof(note));
        snprintf(note + strlen(note), sizeof(note) - strlen(note), ", ");
        Cac_GunNote(loadout, CLASS_SLOT_SECONDARY, note + strlen(note), (int)(sizeof(note) - strlen(note)));
        Cac_Text(note, 0, CAC_LEFT + 6.0f, top + 22.5f, CAC_ALIGN_LEFT, CAC_ROW_NOTE_CAP, 0.0f, s_cacNote);
    }
    Cac_StatusNote(top + 10.0f);

    /* The focused class's loadout at the right edge, as WWII's side panel. */
    focused = &s_cac.classes[s_cac.classFocus];
    Cac_Text("\xbb", 1, -(CAC_SIDE_RIGHT + CAC_SIDE_WIDTH), CAC_ROW_TOP - 1.5f, CAC_ALIGN_RIGHT, 6.5f, 0.0f,
             UI_WWIIMenu_Color(WWII_COLOR_GOLD));
    Cac_Caps("Loadout", 0, -(CAC_SIDE_RIGHT + CAC_SIDE_WIDTH) + 8.0f, CAC_ROW_TOP - 2.0f, CAC_ALIGN_RIGHT, 4.6f, 0.14f,
             s_cacNote);
    {
        CacRect line = Cac_Rect(-(CAC_SIDE_RIGHT + CAC_SIDE_WIDTH), CAC_ROW_TOP + 1.5f, CAC_SIDE_WIDTH, CAC_LINE,
                                CAC_ALIGN_RIGHT);
        Cac_Fill(&line, s_cacEdge);
    }
    /* 20 between a tile and the next: the two lines over each take 15.4. */
    for (slot = 0; slot < CLASS_SLOT_COUNT; slot++) {
        float rowTop = CAC_ROW_TOP + 22.0f + (slot ? 90.0f + (float)(slot - 1) * 50.0f : 0.0f);
        float height = slot ? 30.0f : 70.0f;
        CacRect tile = Cac_Rect(-(CAC_SIDE_RIGHT + CAC_SIDE_WIDTH), rowTop, slot ? 96.0f : CAC_SIDE_WIDTH, height,
                                CAC_ALIGN_RIGHT);

        Cac_Tile(&tile, slot, focused->weapon[slot], Cac_SlotCamo(focused, slot), Cac_SlotAttachments(focused, slot),
                 0);
        if (slot == CLASS_SLOT_PRIMARY)
            Cac_AttachmentList(focused, slot, &tile);
    }
}

static const CacRect *Cac_EditTiles(void)
{
    static CacRect tiles[CAC_FOCUS_COUNT];
    const float boxWidth = (170.0f - (float)(ATT_PRIMARY_MAX - 1) * CAC_ATTACHMENT_BOX_GAP) / (float)ATT_PRIMARY_MAX;
    int i;

    /* As WWII's two columns of tiles, each under its two lines: the guns
     * over their camouflage lines and attachment boxes, then the grenades. */
    tiles[CLASS_SLOT_PRIMARY] = Cac_Rect(CAC_LEFT, 125.0f, 170.0f, 96.0f, CAC_ALIGN_LEFT);
    tiles[CLASS_SLOT_SECONDARY] = Cac_Rect(CAC_LEFT, 281.0f, 110.0f, 66.0f, CAC_ALIGN_LEFT);
    tiles[CLASS_SLOT_LETHAL] = Cac_Rect(CAC_LEFT + 188.0f, 125.0f, 60.0f, 60.0f, CAC_ALIGN_LEFT);
    tiles[CLASS_SLOT_TACTICAL] = Cac_Rect(CAC_LEFT + 188.0f, 217.0f, 60.0f, 60.0f, CAC_ALIGN_LEFT);
    for (i = 0; i < ATT_PRIMARY_MAX; i++)
        tiles[CAC_FOCUS_PRIMARY_ATTACHMENT + i] =
            Cac_Rect(CAC_LEFT + (float)i * (boxWidth + CAC_ATTACHMENT_BOX_GAP), 125.0f + 96.0f + CAC_ATTACHMENT_BOX_TOP,
                     boxWidth, CAC_ATTACHMENT_BOX_HEIGHT, CAC_ALIGN_LEFT);
    tiles[CAC_FOCUS_SECONDARY_ATTACHMENT] =
        Cac_Rect(CAC_LEFT, 281.0f + 66.0f + CAC_ATTACHMENT_BOX_TOP, 110.0f, CAC_ATTACHMENT_BOX_HEIGHT, CAC_ALIGN_LEFT);
    return tiles;
}

/* An attachment box: the attachment's name, or a plus on the next free one. */
static void Cac_AttachmentBox(const CacRect *box, const ClassLoadout *loadout, int slot, int position, int focused)
{
    int count = ClassData_AttachmentCount(loadout, slot);

    Cac_Fill(box, focused ? s_cacRowFocus : s_cacTile);
    Cac_Frame(box, focused ? UI_WWIIMenu_Color(WWII_COLOR_GOLD) : s_cacEdge);
    if (position < count) {
        int attachment = loadout->attachment[slot][position];
        CacRect icon = Cac_Rect(box->x + 1.0f, box->y + 1.0f, box->h - 2.0f, box->h - 2.0f, box->horz);
        CacRect name = *box;

        /* The icon at the left, the name beside it. */
        if (Cac_DrawIcon(attachment, &icon, 0.95f, UI_WWIIMenu_Color(WWII_COLOR_WHITE))) {
            name.x += box->h - 2.0f;
            name.w -= box->h - 2.0f;
        }
        Cac_AttachmentName(g_attachments[attachment].displayName, &name, 3.4f,
                           UI_WWIIMenu_Color(focused ? WWII_COLOR_GOLD : WWII_COLOR_WHITE));
    } else if (position == count)
        Cac_Text("+", 1, box->x + 0.5f * box->w - 2.0f, box->y + 0.5f * box->h + 3.5f, box->horz, 7.0f, 0.0f,
                 focused ? UI_WWIIMenu_Color(WWII_COLOR_GOLD) : s_cacNote);
}

static void Cac_PaintEdit(displayContextDef_t *dc)
{
    const ClassLoadout *loadout = &s_cac.classes[s_cac.classFocus];
    const CacRect *tiles = Cac_EditTiles();
    const float x = -(CAC_SIDE_RIGHT + CAC_SIDE_WIDTH);
    char title[64];
    int focus, slot, position;

    snprintf(title, sizeof(title), "Modify %s", loadout->name);
    Cac_Title(title, NULL);
    for (focus = 0; focus < CAC_FOCUS_COUNT; focus++) {
        if (Cac_Hover(dc, &tiles[focus]))
            s_cac.hover = focus;
        if (Cac_FocusAttachment(focus, &slot, &position))
            Cac_AttachmentBox(&tiles[focus], loadout, slot, position, focus == s_cac.slotFocus);
        else
            Cac_Tile(&tiles[focus], focus, loadout->weapon[focus], Cac_SlotCamo(loadout, focus),
                     Cac_SlotAttachments(loadout, focus), focus == s_cac.slotFocus);
    }
    /* Under each gun, its camouflage: a click, F2 or Y opens the picker. */
    for (slot = 0; slot < CLASS_CAMO_SLOTS; slot++) {
        const CacRect *tile = &tiles[slot];
        CacRect line = Cac_Rect(tile->x, tile->y + tile->h + 1.5f, tile->w, CAC_CAMO_LINE_HEIGHT, CAC_ALIGN_LEFT);
        CacRect swatch = Cac_Rect(line.x, line.y + 1.0f, CAC_CAMO_LINE_HEIGHT - 2.0f, CAC_CAMO_LINE_HEIGHT - 2.0f,
                                  CAC_ALIGN_LEFT);
        int camo = loadout->camo[slot], hover = Cac_Hover(dc, &line);
        char text[48];

        if (hover)
            s_cac.hover = CAC_HOVER_CAMO + slot;
        Cac_CamoSwatch(camo, &swatch);
        Cac_Frame(&swatch, s_cacEdge);
        snprintf(text, sizeof(text), "Camouflage: %s", g_weaponCamos[camo].name);
        Cac_Caps(text, hover, swatch.x + swatch.w + 3.0f, line.y + 0.5f * (line.h + CAC_CAMO_LINE_CAP), CAC_ALIGN_LEFT,
                 CAC_CAMO_LINE_CAP, 0.1f, hover ? UI_WWIIMenu_Color(WWII_COLOR_GOLD) : s_cacNote);
    }
    Cac_StatusNote(404.0f);
    if (!Cac_FocusAttachment(s_cac.slotFocus, &slot, &position)) {
        slot = s_cac.slotFocus;
        Cac_Detail(loadout->weapon[slot], x, 135.0f, CAC_ALIGN_RIGHT, CAC_SIDE_WIDTH);
        Cac_Stats(loadout->weapon[slot], 0, slot <= CLASS_SLOT_SECONDARY ? ClassData_AttachmentMask(loadout, slot) : 0);
        return;
    }
    if (position < ClassData_AttachmentCount(loadout, slot)) {
        int attachment = loadout->attachment[slot][position];

        Cac_AttachmentDetail(loadout->weapon[slot], ClassData_AttachmentMask(loadout, slot) & ~ATT_BIT(attachment),
                             attachment, x, 135.0f, CAC_ALIGN_RIGHT, CAC_SIDE_WIDTH);
    } else {
        char note[96];
        int attachments[ATT_COUNT];

        Cac_Caps("Empty", 1, x, 135.0f, CAC_ALIGN_RIGHT, 10.0f, 0.06f, UI_WWIIMenu_Color(WWII_COLOR_WHITE));
        snprintf(note, sizeof(note), "The %s takes %d of its %d attachments.", Cac_WeaponName(loadout->weapon[slot]),
                 ClassData_AttachmentMax(slot), Cac_AttachmentCandidates(loadout, slot, attachments));
        Cac_Wrapped(note, x, 149.0f, CAC_ALIGN_RIGHT, CAC_SIDE_WIDTH, 4.4f, 3, s_cacNote);
    }
    Cac_Stats(loadout->weapon[slot], 0, ClassData_AttachmentMask(loadout, slot));
}

static void Cac_PaintAttachments(displayContextDef_t *dc)
{
    const ClassLoadout *loadout = &s_cac.classes[s_cac.classFocus];
    int slot = s_cac.attachSlot, weapon = loadout->weapon[slot];
    int attachments[ATT_COUNT], count = Cac_AttachmentCandidates(loadout, slot, attachments), i;
    unsigned mask = ClassData_AttachmentMask(loadout, slot), others = mask, preview;
    int current = s_cac.attachPosition < ClassData_AttachmentCount(loadout, slot)
                      ? loadout->attachment[slot][s_cac.attachPosition]
                      : -1;
    int focused, blocker;
    float baseline;
    CacRect box;

    Cac_Title("Modify Attachment", NULL);
    if (!count)
        return;
    if (current >= 0)
        others &= ~ATT_BIT(current);
    if (s_cac.attachFocus < 0 || s_cac.attachFocus >= count)
        s_cac.attachFocus = 0;
    for (i = 0; i < count; i++) {
        CacRect tile = Cac_Rect(CAC_LEFT + (float)(i % CAC_ATTACHMENT_COLUMNS) *
                                               (CAC_ATTACHMENT_TILE_WIDTH + CAC_ATTACHMENT_TILE_GAP),
                                CAC_ROW_TOP + (float)(i / CAC_ATTACHMENT_COLUMNS) *
                                                  (CAC_ATTACHMENT_TILE_HEIGHT + CAC_ATTACHMENT_TILE_GAP),
                                CAC_ATTACHMENT_TILE_WIDTH, CAC_ATTACHMENT_TILE_HEIGHT, CAC_ALIGN_LEFT);
        int isFocus = i == s_cac.attachFocus, here = attachments[i] == current;
        int blocked = BG_AttachmentConflict(others, attachments[i]) >= 0;

        if (Cac_Hover(dc, &tile))
            s_cac.hover = i;
        Cac_Fill(&tile, isFocus ? s_cacRowFocus : s_cacAttachTile);
        Cac_Frame(&tile, isFocus ? UI_WWIIMenu_Color(WWII_COLOR_GOLD) : here ? s_cacNote : s_cacEdge);
        if (!Cac_DrawIcon(attachments[i], &tile, 0.86f, blocked ? s_cacIconBlocked : UI_WWIIMenu_Color(WWII_COLOR_WHITE)))
            Cac_AttachmentName(g_attachments[attachments[i]].displayName, &tile, 3.8f,
                               blocked ? s_cacDisabled
                               : here  ? UI_WWIIMenu_Color(WWII_COLOR_GOLD)
                                       : UI_WWIIMenu_Color(isFocus ? WWII_COLOR_WHITE : WWII_COLOR_IDLE));
        /* The others on the gun keep a mark, as WWII's tiles do. */
        if (mask & ATT_BIT(attachments[i])) {
            CacRect mark = Cac_Rect(tile.x, tile.y, 1.5f, tile.h, CAC_ALIGN_LEFT);

            Cac_Fill(&mark, UI_WWIIMenu_Color(WWII_COLOR_GOLD));
        }
    }
    focused = attachments[s_cac.attachFocus];
    blocker = BG_AttachmentConflict(others, focused);
    baseline = Cac_AttachmentDetail(weapon, others, focused, CAC_DETAIL_LEFT, 135.0f, CAC_ALIGN_LEFT,
                                    CAC_DETAIL_WIDTH);
    if (blocker >= 0) {
        char note[96];

        if (blocker == focused)
            snprintf(note, sizeof(note), "Already on the %s.", Cac_WeaponName(weapon));
        else
            snprintf(note, sizeof(note), "One optic at a time: remove the %s first.",
                     g_attachments[blocker].displayName);
        Cac_Wrapped(note, CAC_DETAIL_LEFT, baseline + 4.0f, CAC_ALIGN_LEFT, CAC_DETAIL_WIDTH, 4.4f, 2, s_cacWarning);
    }
    box = Cac_Rect(-150.0f, 200.0f, 300.0f, 160.0f, CAC_ALIGN_CENTER);
    box.x += 60.0f;
    preview = blocker >= 0 ? mask : others | ATT_BIT(focused);
    Cac_GunPicture(weapon, Cac_SlotCamo(loadout, slot), preview, &box, UI_WWIIMenu_Color(WWII_COLOR_WHITE));
    Cac_Stats(weapon, mask, preview);
}

static void Cac_PaintWeapons(displayContextDef_t *dc)
{
    const ClassLoadout *loadout = &s_cac.classes[s_cac.classFocus];
    int indexes[64], count = Cac_SlotCandidates(s_cac.pickSlot, indexes), i;
    char page[32];
    CacRect box;

    Cac_Title("Equip Weapon", s_cacSlots[s_cac.pickSlot]);
    if (s_cac.pickFocus < s_cac.pickScroll)
        s_cac.pickScroll = s_cac.pickFocus;
    if (s_cac.pickFocus >= s_cac.pickScroll + CAC_PICK_VISIBLE)
        s_cac.pickScroll = s_cac.pickFocus - CAC_PICK_VISIBLE + 1;
    if (s_cac.pickScroll > count - CAC_PICK_VISIBLE)
        s_cac.pickScroll = count - CAC_PICK_VISIBLE;
    if (s_cac.pickScroll < 0)
        s_cac.pickScroll = 0;
    for (i = s_cac.pickScroll; i < count && i < s_cac.pickScroll + CAC_PICK_VISIBLE; i++) {
        float top = CAC_PICK_TOP + (float)(i - s_cac.pickScroll) * CAC_PICK_ROW;
        CacRect row = Cac_Rect(0.0f, top, CAC_PICK_WIDTH, CAC_PICK_HEIGHT, CAC_ALIGN_LEFT);
        int isFocus = i == s_cac.pickFocus, equipped = indexes[i] == loadout->weapon[s_cac.pickSlot];

        if (Cac_Hover(dc, &row))
            s_cac.hover = i;
        Cac_Fill(&row, isFocus ? s_cacRowFocus : s_cacRow);
        if (isFocus)
            Cac_Frame(&row, UI_WWIIMenu_Color(WWII_COLOR_GOLD));
        Cac_Caps(Cac_WeaponName(indexes[i]), isFocus, CAC_LEFT + 16.0f, top + 15.5f, CAC_ALIGN_LEFT, 5.4f, 0.1f,
                 UI_WWIIMenu_Color(isFocus ? WWII_COLOR_WHITE : WWII_COLOR_IDLE));
        if (equipped)
            Cac_Caps("Equipped", 0, CAC_PICK_WIDTH - 8.0f - UI_WWIIMenu_Width("EQUIPPED", 0, 3.8f, 0.12f),
                     top + 15.0f, CAC_ALIGN_LEFT, 3.8f, 0.12f, UI_WWIIMenu_Color(WWII_COLOR_GOLD));
    }
    snprintf(page, sizeof(page), "%d/%d", s_cac.pickFocus + 1, count);
    Cac_Text(page, 0, CAC_LEFT, CAC_PICK_TOP + (float)CAC_PICK_VISIBLE * CAC_PICK_ROW + 10.0f, CAC_ALIGN_LEFT, 4.4f,
             0.0f, s_cacNote);
    if (count <= 0)
        return;
    Cac_Detail(indexes[s_cac.pickFocus], CAC_DETAIL_LEFT, 135.0f, CAC_ALIGN_LEFT, CAC_DETAIL_WIDTH);
    box = Cac_Rect(-150.0f, 200.0f, 300.0f, 160.0f, CAC_ALIGN_CENTER);
    box.x += 60.0f;
    Cac_GunPicture(indexes[s_cac.pickFocus], Cac_SlotCamo(loadout, s_cac.pickSlot), 0, &box,
                   UI_WWIIMenu_Color(WWII_COLOR_WHITE));
    /* The equipped gun keeps its attachments; another starts without. */
    {
        unsigned mask = indexes[s_cac.pickFocus] == loadout->weapon[s_cac.pickSlot]
                            ? ClassData_AttachmentMask(loadout, s_cac.pickSlot)
                            : 0;

        Cac_Stats(indexes[s_cac.pickFocus], mask, mask);
    }
}

static CacRect Cac_CamoSwatchRect(int camo)
{
    return Cac_Rect(CAC_LEFT + (float)(camo % CAC_CAMO_COLUMNS) * (CAC_CAMO_SWATCH + CAC_CAMO_GAP),
                    CAC_ROW_TOP + (float)(camo / CAC_CAMO_COLUMNS) * (CAC_CAMO_SWATCH + CAC_CAMO_GAP), CAC_CAMO_SWATCH,
                    CAC_CAMO_SWATCH, CAC_ALIGN_LEFT);
}

/* As WWII's Primary Camouflage: the patterns' swatches, the focused one's
 * name and note beside them, its place in the list under them, and the gun
 * wearing it. */
static void Cac_PaintCamo(displayContextDef_t *dc)
{
    const ClassLoadout *loadout = &s_cac.classes[s_cac.classFocus];
    const int weapon = loadout->weapon[s_cac.camoSlot];
    const float right = CAC_LEFT + (float)CAC_CAMO_COLUMNS * (CAC_CAMO_SWATCH + CAC_CAMO_GAP) + 9.0f;
    const int rows = (WEAPON_CAMO_COUNT + CAC_CAMO_COLUMNS - 1) / CAC_CAMO_COLUMNS;
    char page[32];
    CacRect box;
    int camo;

    Cac_Title(s_cac.camoSlot == CLASS_SLOT_PRIMARY ? "Primary Camouflage" : "Secondary Camouflage", NULL);
    for (camo = 0; camo < WEAPON_CAMO_COUNT; camo++) {
        CacRect swatch = Cac_CamoSwatchRect(camo);
        int isFocus = camo == s_cac.camoFocus, equipped = camo == loadout->camo[s_cac.camoSlot];

        if (Cac_Hover(dc, &swatch))
            s_cac.hover = camo;
        Cac_Fill(&swatch, s_cacTile);
        Cac_CamoSwatch(camo, &swatch);
        if (camo == WEAPON_CAMO_NONE)
            Cac_Caps(g_weaponCamos[camo].name, 0,
                     swatch.x + 0.5f * (swatch.w - UI_WWIIMenu_Width("NONE", 0, CAC_CAMO_LINE_CAP, 0.12f)),
                     swatch.y + 0.5f * (swatch.h + CAC_CAMO_LINE_CAP), CAC_ALIGN_LEFT, CAC_CAMO_LINE_CAP, 0.12f,
                     s_cacNote);
        Cac_Frame(&swatch, isFocus ? UI_WWIIMenu_Color(WWII_COLOR_GOLD) : equipped ? s_cacNote : s_cacEdge);
        if (isFocus) {
            CacRect inner = Cac_Rect(swatch.x + CAC_LINE, swatch.y + CAC_LINE, swatch.w - 2.0f * CAC_LINE,
                                     swatch.h - 2.0f * CAC_LINE, CAC_ALIGN_LEFT);

            Cac_Frame(&inner, UI_WWIIMenu_Color(WWII_COLOR_GOLD));
        }
    }
    snprintf(page, sizeof(page), "%d/%d", s_cac.camoFocus + 1, WEAPON_CAMO_COUNT);
    Cac_Text(page, 0, CAC_LEFT, CAC_ROW_TOP + (float)rows * (CAC_CAMO_SWATCH + CAC_CAMO_GAP) + 8.0f, CAC_ALIGN_LEFT,
             4.4f, 0.0f, s_cacNote);
    Cac_Caps(g_weaponCamos[s_cac.camoFocus].name, 1, right, CAC_ROW_TOP + 10.0f, CAC_ALIGN_LEFT, 10.0f, 0.06f,
             UI_WWIIMenu_Color(WWII_COLOR_WHITE));
    Cac_Text(g_weaponCamos[s_cac.camoFocus].note, 1, right, CAC_ROW_TOP + 21.0f, CAC_ALIGN_LEFT, 4.8f, 0.0f,
             UI_WWIIMenu_Color(WWII_COLOR_TITLE));
    if (s_cac.camoFocus == loadout->camo[s_cac.camoSlot])
        Cac_Caps("Equipped", 0, right, CAC_ROW_TOP + 32.0f, CAC_ALIGN_LEFT, 3.8f, 0.12f,
                 UI_WWIIMenu_Color(WWII_COLOR_GOLD));
    Cac_Caps(Cac_WeaponName(weapon), 0, -150.0f + 60.0f, 190.0f, CAC_ALIGN_CENTER, 4.6f, 0.14f, s_cacNote);
    box = Cac_Rect(-150.0f, 200.0f, 300.0f, 160.0f, CAC_ALIGN_CENTER);
    box.x += 60.0f;
    Cac_GunPicture(weapon, s_cac.camoFocus, Cac_SlotAttachments(loadout, s_cac.camoSlot), &box,
                   UI_WWIIMenu_Color(WWII_COLOR_WHITE));
}

static void Cac_PaintEquipment(displayContextDef_t *dc)
{
    const ClassLoadout *loadout = &s_cac.classes[s_cac.classFocus];
    static const int rows[2] = { CLASS_SLOT_TACTICAL, CLASS_SLOT_LETHAL };
    int row, focusWeapon = -1;
    char hint[96];
    float below;
    CacRect box;

    Cac_Title("Modify Equipment", NULL);
    for (row = 0; row < 2; row++) {
        int indexes[64], count = Cac_SlotCandidates(rows[row], indexes), i;
        float top = row ? 236.0f : 115.0f;
        CacRect line = Cac_Rect(CAC_LEFT, top - 6.0f, CAC_EQUIP_COLUMNS * CAC_ICON_STEP - (CAC_ICON_STEP - CAC_ICON_SIZE),
                                CAC_LINE, CAC_ALIGN_LEFT);

        Cac_Fill(&line, s_cacEdge);
        Cac_Caps(s_cacSlots[rows[row]], 0, CAC_LEFT, top + 1.5f, CAC_ALIGN_LEFT, 4.4f, 0.12f, s_cacNote);
        for (i = 0; i < count && i < CAC_EQUIP_COLUMNS; i++) {
            CacRect tile = Cac_Rect(CAC_LEFT + (float)i * CAC_ICON_STEP, top + 8.0f, CAC_ICON_SIZE, CAC_ICON_SIZE,
                                    CAC_ALIGN_LEFT);
            int isFocus = s_cac.pickSlot == rows[row] && i == s_cac.pickFocus;
            int equipped = indexes[i] == loadout->weapon[rows[row]];
            CacRect picture = tile;

            if (Cac_Hover(dc, &tile))
                s_cac.hover = row * CAC_EQUIP_COLUMNS + i;
            if (isFocus)
                focusWeapon = indexes[i];
            Cac_Fill(&tile, isFocus ? s_cacRowFocus : s_cacTile);
            Cac_Frame(&tile, isFocus ? UI_WWIIMenu_Color(WWII_COLOR_GOLD) : equipped ? s_cacNote : s_cacEdge);
            picture.x += 5.0f;
            picture.y += 5.0f;
            picture.w -= 10.0f;
            picture.h -= 10.0f;
            Cac_Picture(indexes[i], &picture, UI_WWIIMenu_Color(WWII_COLOR_WHITE));
        }
    }
    if (focusWeapon < 0)
        return;
    below = Cac_Detail(focusWeapon, CAC_LEFT + CAC_EQUIP_COLUMNS * CAC_ICON_STEP + 12.0f, 135.0f, CAC_ALIGN_LEFT,
                       CAC_DETAIL_WIDTH);
    /* The default binds. Every frag cooks in the hand (BG_GrenadeCanCook); smoke, the
     * mine and C4 do not. */
    if (g_classWeapons[focusWeapon].group == CLASS_GROUP_MINE)
        snprintf(hint, sizeof(hint), "%s", "Press G to set it at your feet; hold F over it to take it back.");
    else if (g_classWeapons[focusWeapon].group == CLASS_GROUP_CHARGE)
        snprintf(hint, sizeof(hint), "%s", "Press G to take it out: fire throws it, aim sets them off.");
    else if (s_cac.pickSlot == CLASS_SLOT_LETHAL && s_cac.stats[focusWeapon].fuse > 0.0f)
        snprintf(hint, sizeof(hint), "Hold G to cook it: the %g-second fuse starts with the pin.",
                 (double)s_cac.stats[focusWeapon].fuse);
    else
        snprintf(hint, sizeof(hint), "%s", s_cac.pickSlot == CLASS_SLOT_LETHAL ? "Hold G to cook it." : "Press 4 to throw it.");
    Cac_Text(hint, 0, CAC_LEFT + CAC_EQUIP_COLUMNS * CAC_ICON_STEP + 12.0f, below + 4.0f, CAC_ALIGN_LEFT, 4.4f, 0.0f,
             s_cacNote);
    box = Cac_Rect(90.0f, 235.0f, 130.0f, 130.0f, CAC_ALIGN_CENTER);
    Cac_Picture(focusWeapon, &box, s_cacNote);
}

static void Cac_PaintRename(displayContextDef_t *dc)
{
    CacRect box = Cac_Rect(-120.0f, 190.0f, 240.0f, 76.0f, CAC_ALIGN_CENTER);
    CacRect field = Cac_Rect(-108.0f, 222.0f, 216.0f, 22.0f, CAC_ALIGN_CENTER);
    float width;

    CL_DrawStretchPic(0.0f, 0.0f, 640.0f, 480.0f, CAC_ALIGN_FULLSCREEN, CAC_ALIGN_FULLSCREEN, 0.0f, 0.0f, 1.0f, 1.0f,
                      s_cacShade, sharedUiInfo.assets.whiteMaterial);
    Cac_Fill(&box, s_cacShade);
    Cac_Frame(&box, UI_WWIIMenu_Color(WWII_COLOR_GOLD));
    Cac_Caps("Rename Class", 1, box.x + 12.0f, box.y + 20.0f, CAC_ALIGN_CENTER, 6.0f, 0.14f,
             UI_WWIIMenu_Color(WWII_COLOR_GOLD));
    Cac_Fill(&field, s_cacRow);
    Cac_Frame(&field, s_cacEdge);
    Cac_Text(s_cac.rename, 1, field.x + 6.0f, field.y + 15.0f, CAC_ALIGN_CENTER, 6.0f, 0.04f,
             UI_WWIIMenu_Color(WWII_COLOR_WHITE));
    width = UI_WWIIMenu_Width(s_cac.rename, 1, 6.0f, 0.04f);
    if ((dc->realTime / 500) & 1) {
        CacRect caret = Cac_Rect(field.x + 7.0f + width, field.y + 5.0f, 0.8f, 12.0f, CAC_ALIGN_CENTER);
        Cac_Fill(&caret, UI_WWIIMenu_Color(WWII_COLOR_GOLD));
    }
}

/* Capitals as tall as cap, or smaller when they would run wider than width. */
static float Cac_FitCap(const char *text, int bold, float cap, float tracking, float width)
{
    float wide = UI_WWIIMenu_Width(text, bold, cap, tracking);

    return wide > width && wide > 0.0f ? cap * width / wide : cap;
}

/* A slot's caption and its weapon's name over the weapon's tile, framed; the
 * guns' tiles lighter. */
static void Cac_SelectTile(float x, float top, float size, int slot, int weapon)
{
    CacRect tile = Cac_Rect(x, top, size, size, CAC_ALIGN_LEFT);
    CacRect box = tile;
    const char *name = Cac_WeaponName(weapon);
    char upper[96];

    Cac_Caps(s_cacSlots[slot], 0, x, top - CAC_SELECT_CAPTION_ABOVE, CAC_ALIGN_LEFT, CAC_SELECT_CAPTION_CAP,
             CAC_SELECT_CAPTION_TRACKING, s_cacNote);
    Cac_Upper(name, upper, sizeof(upper));
    Cac_Text(upper, 1, x, top - CAC_SELECT_NAME_ABOVE, CAC_ALIGN_LEFT,
             Cac_FitCap(upper, 1, CAC_SELECT_NAME_CAP, CAC_SELECT_NAME_TRACKING,
                        slot == CLASS_SLOT_LETHAL ? CAC_SELECT_LOADOUT_COLUMN - 4.0f : CAC_SELECT_PRIMARY_SIZE),
             CAC_SELECT_NAME_TRACKING, UI_WWIIMenu_Color(WWII_COLOR_TITLE));
    if (slot <= CLASS_SLOT_SECONDARY)
        Cac_Fill(&tile, s_cacSelectTile);
    Cac_FrameWidth(&tile, s_cacEdge, CAC_SELECT_LINE);
    box.x += 0.08f * size;
    box.y += 0.08f * size;
    box.w = box.h = 0.84f * size;
    Cac_GunPicture(weapon, Cac_SlotCamo(&Cac_Listed()[s_cac.classFocus], slot),
                   Cac_SlotAttachments(&Cac_Listed()[s_cac.classFocus], slot), &box,
                   UI_WWIIMenu_Color(WWII_COLOR_WHITE));
}

/* Select Class, as Select Division: the title, Custom and Standard Issue
 * tabs, the tab's classes down the left, the focused class's loadout. */
static void Cac_PaintSelect(displayContextDef_t *dc)
{
    static const char *const tabs[CAC_TAB_COUNT] = { "CUSTOM", "STANDARD ISSUE" };
    static const char *const keys[] = { "ENTER", "F1" };
    static const char *const labels[] = { "Select", "Edit" };
    const ClassLoadout *listed = Cac_Listed(), *focused = &listed[s_cac.classFocus];
    const float right = CAC_SELECT_LOADOUT_LEFT + CAC_SELECT_LOADOUT_COLUMN;
    int i;

    Cac_Text("SELECT CLASS", 1, CAC_SELECT_TITLE_LEFT, CAC_SELECT_TITLE_BASELINE, CAC_ALIGN_LEFT, CAC_SELECT_TITLE_CAP,
             CAC_SELECT_TITLE_TRACKING, UI_WWIIMenu_Color(WWII_COLOR_GOLD));
    for (i = 0; i < CAC_TAB_COUNT; i++) {
        int chosen = i == s_cac.tab;
        float top = chosen ? CAC_SELECT_TAB_TOP : CAC_SELECT_TAB_IDLE_TOP;
        float cap = chosen ? CAC_SELECT_TAB_CAP : CAC_SELECT_TAB_IDLE_CAP;
        float tracking = chosen ? CAC_SELECT_TAB_TRACKING : CAC_SELECT_TAB_IDLE_TRACKING;
        CacRect tab = Cac_Rect(CAC_SELECT_TAB_LEFT + (float)i * (CAC_SELECT_TAB_WIDTH + CAC_SELECT_TAB_GAP), top,
                               CAC_SELECT_TAB_WIDTH, CAC_SELECT_TAB_BOTTOM - top, CAC_ALIGN_LEFT);

        if (Cac_Hover(dc, &tab))
            s_cac.hover = CAC_HOVER_TAB + i;
        if (chosen)
            Cac_Fill(&tab, s_cacSelectTab);
        else
            Cac_FrameWidth(&tab, s_cacSelectTabEdge, CAC_SELECT_LINE);
        Cac_Text(tabs[i], chosen, tab.x + 0.5f * (tab.w - UI_WWIIMenu_Width(tabs[i], chosen, cap, tracking)),
                 tab.y + 0.5f * (tab.h + cap), CAC_ALIGN_LEFT, cap, tracking,
                 chosen ? s_cacSelectTabText : UI_WWIIMenu_Color(WWII_COLOR_IDLE));
    }
    for (i = 0; i < CLASS_COUNT; i++) {
        float top = CAC_SELECT_ROW_TOP + (float)i * CAC_SELECT_ROW_PITCH;
        CacRect row = Cac_Rect(0.0f, top, CAC_SELECT_ROW_WIDTH, CAC_SELECT_ROW_HEIGHT, CAC_ALIGN_LEFT);
        float left = CAC_SELECT_ROW_LEFT, room;
        char upper[64];

        if (Cac_Hover(dc, &row))
            s_cac.hover = i;
        Cac_Upper(listed[i].name, upper, sizeof(upper));
        if (i == s_cac.classFocus) {
            left += CAC_SELECT_FOCUS_INDENT;
            room = CAC_SELECT_ROW_WIDTH - 4.0f - left;
            Cac_Fill(&row, s_cacSelectRow);
            Cac_FrameWidth(&row, s_cacSelectRowEdge, CAC_SELECT_LINE);
            Cac_Text(upper, 1, left, top + CAC_SELECT_FOCUS_BASELINE, CAC_ALIGN_LEFT,
                     Cac_FitCap(upper, 1, CAC_SELECT_FOCUS_CAP, CAC_SELECT_FOCUS_TRACKING, room),
                     CAC_SELECT_FOCUS_TRACKING, UI_WWIIMenu_Color(WWII_COLOR_GOLD));
        } else {
            room = CAC_SELECT_ROW_WIDTH - 4.0f - left;
            Cac_Text(upper, 0, left, top + CAC_SELECT_ROW_BASELINE, CAC_ALIGN_LEFT,
                     Cac_FitCap(upper, 0, CAC_SELECT_ROW_CAP, CAC_SELECT_ROW_TRACKING, room), CAC_SELECT_ROW_TRACKING,
                     UI_WWIIMenu_Color(WWII_COLOR_IDLE));
        }
    }
    if (s_cac.tab == CAC_TAB_CUSTOM)
        Cac_StatusNote(CAC_SELECT_ROW_TOP + (float)CLASS_COUNT * CAC_SELECT_ROW_PITCH + 8.0f);

    Cac_SelectTile(CAC_SELECT_LOADOUT_LEFT, CAC_SELECT_PRIMARY_TOP, CAC_SELECT_PRIMARY_SIZE, CLASS_SLOT_PRIMARY,
                   focused->weapon[CLASS_SLOT_PRIMARY]);
    Cac_SelectTile(CAC_SELECT_LOADOUT_LEFT, CAC_SELECT_SECONDARY_TOP, CAC_SELECT_SECONDARY_SIZE, CLASS_SLOT_SECONDARY,
                   focused->weapon[CLASS_SLOT_SECONDARY]);
    Cac_SelectTile(CAC_SELECT_LOADOUT_LEFT, CAC_SELECT_EQUIPMENT_TOP, CAC_SELECT_EQUIPMENT_SIZE, CLASS_SLOT_LETHAL,
                   focused->weapon[CLASS_SLOT_LETHAL]);
    Cac_SelectTile(right, CAC_SELECT_EQUIPMENT_TOP, CAC_SELECT_EQUIPMENT_SIZE, CLASS_SLOT_TACTICAL,
                   focused->weapon[CLASS_SLOT_TACTICAL]);

    UI_WWIIMenu_PromptsAt(keys, labels, s_cac.tab == CAC_TAB_CUSTOM ? 2 : 1, "ESC", "Back", CAC_SELECT_PROMPT_LEFT,
                          CAC_SELECT_PROMPT_RIGHT, CAC_SELECT_PROMPT_LOWER);
}

/* The screen up, with its prompts. */
static void Cac_PaintScreen(displayContextDef_t *dc)
{
    static const char *const keys[] = { "ENTER", "F1", "F2" };
    static const char *const attachmentKeys[] = { "ENTER", "BACKSPACE" };
    const char *labels[3] = { NULL, NULL, NULL };

    switch (s_cac.screen) {
    case CAC_SCREEN_EDIT:
        Cac_PaintEdit(dc);
        labels[0] = "Select";
        labels[1] = "Rename";
        if (s_cac.slotFocus <= CLASS_SLOT_SECONDARY)
            labels[2] = "Customize";
        break;
    case CAC_SCREEN_CAMO:
        Cac_PaintCamo(dc);
        labels[0] = "Equip";
        break;
    case CAC_SCREEN_WEAPONS:
        Cac_PaintWeapons(dc);
        labels[0] = "Equip";
        labels[1] = NULL;
        break;
    case CAC_SCREEN_EQUIPMENT:
        Cac_PaintEquipment(dc);
        labels[0] = "Equip";
        labels[1] = NULL;
        break;
    case CAC_SCREEN_ATTACHMENTS:
        Cac_PaintAttachments(dc);
        labels[0] = "Equip";
        labels[1] = s_cac.attachPosition < ClassData_AttachmentCount(&s_cac.classes[s_cac.classFocus], s_cac.attachSlot)
                        ? "Remove"
                        : NULL;
        if (!s_cac.renaming) {
            UI_WWIIMenu_Prompts(attachmentKeys, labels, labels[1] ? 2 : 1, "ESC", "Back");
            return;
        }
        break;
    default:
        if (s_cac.choose) {
            Cac_PaintSelect(dc);
            return;
        }
        Cac_PaintList(dc);
        labels[0] = "Edit";
        labels[1] = "Rename";
        break;
    }
    if (s_cac.renaming) {
        static const char *const renameKeys[] = { "ENTER" };
        static const char *const renameLabels[] = { "Save" };

        Cac_PaintRename(dc);
        UI_WWIIMenu_Prompts(renameKeys, renameLabels, 1, "ESC", "Cancel");
        return;
    }
    UI_WWIIMenu_Prompts(keys, labels, labels[2] ? 3 : labels[1] ? 2 : 1, "ESC", "Back");
}

void UI_ClassMenu_Paint(displayContextDef_t *dc, menuDef_t *menu)
{
    int kind = Cac_MenuKind(menu);
    int moved;

    if (!kind)
        return;
    if (!s_cac.open)
        UI_ClassMenu_Open(dc, kind == 2);
    Cac_ReadStats();
    /* In a match the renderer blurs the scene behind and turns it gray, as
     * WWII's; the next frame takes the request. */
    if (kind == 2)
        g_webMenuBackdrop = 1.0f;
    /* The text stands on a dark scene, as WWII's: the backdrop or the match. */
    if (s_cac.choose && s_cac.screen == CAC_SCREEN_LIST) {
        CL_DrawStretchPic(0.0f, 0.0f, 640.0f, 480.0f, CAC_ALIGN_FULLSCREEN, CAC_ALIGN_FULLSCREEN, 0.0f, 0.0f, 1.0f,
                          1.0f, s_cacSelectShade, sharedUiInfo.assets.whiteMaterial);
        UI_WWIIMenu_PaintWash();
    } else {
        CL_DrawStretchPic(0.0f, 0.0f, 640.0f, 480.0f, CAC_ALIGN_FULLSCREEN, CAC_ALIGN_FULLSCREEN, 0.0f, 0.0f, 1.0f,
                          1.0f, s_cacShade, sharedUiInfo.assets.whiteMaterial);
    }
    moved = dc->cursorx != s_cac.cursorX || dc->cursory != s_cac.cursorY;
    s_cac.cursorX = dc->cursorx;
    s_cac.cursorY = dc->cursory;
    s_cac.hover = -1;
    Cac_PaintScreen(dc);
    /* Pointer motion moves the focus to what it is over now, with the retail
     * menus' mouse_over as it enters another element; keys move it otherwise. */
    if (moved && !s_cac.renaming) {
        CacFocus focus = Cac_Focus();

        Cac_FocusHover();
        if (Cac_FocusMoved(&focus))
            UI_PlayLocalSoundAliasByName("mouse_over");
    }
}

const ClassLoadout *UI_ClassMenu_Classes(int standard)
{
    Cac_Load();
    return standard ? s_cac.standard : s_cac.classes;
}

const char *UI_ClassMenu_Note(void)
{
    if (s_cac.store == CAC_STORE_LOADING)
        return "Loading your classes...";
    if (s_cac.store == CAC_STORE_NO_NAME)
        return "Choose a gametag in Options to save your classes.";
    if (s_cac.store == CAC_STORE_OFFLINE)
        return "Classes are unavailable: changes last until you leave.";
    if (s_cac.saveFailed)
        return "The last change was not saved.";
    return NULL;
}

const char *UI_ClassMenu_WeaponName(int weapon)
{
    return Cac_WeaponName(weapon);
}

void UI_ClassMenu_DrawWeapon(const ClassLoadout *loadout, int slot, float x, float y, float w, float h, int horzAlign,
                             const vec_t *color)
{
    CacRect box = Cac_Rect(x, y, w, h, horzAlign);

    Cac_GunPicture(loadout->weapon[slot], Cac_SlotCamo(loadout, slot), Cac_SlotAttachments(loadout, slot), &box,
                   color);
}

int UI_ClassMenu_Choose(int standard, int index)
{
    if (index < 0 || index >= CLASS_COUNT || !Cac_Respond(&UI_ClassMenu_Classes(standard)[index]))
        return 0;
    s_cac.chosen = index;
    s_cac.chosenTab = standard ? CAC_TAB_STANDARD : CAC_TAB_CUSTOM;
    return 1;
}

int UI_ClassMenu_Chosen(int *standard)
{
    Cac_Ready();
    *standard = s_cac.chosenTab == CAC_TAB_STANDARD;
    return s_cac.chosen;
}

void UI_ClassMenu_EditFromPause(displayContextDef_t *dc, int index)
{
    if (index < 0 || index >= CLASS_COUNT || !Menus_OpenByName(dc, "browser_class"))
        return;
    Menus_CloseByName(dc, "ingame");
    s_cac.tab = CAC_TAB_CUSTOM;
    s_cac.classFocus = index;
    s_cac.slotFocus = CLASS_SLOT_PRIMARY;
    s_cac.screen = CAC_SCREEN_EDIT;
    s_cac.fromPause = 1;
    s_cac.hover = -1;
}
