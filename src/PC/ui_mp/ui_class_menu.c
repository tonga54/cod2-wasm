#include "common_types.h"
#include "imports.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include "headers/PC/ui_mp/ui_class_data.h"
#include "headers/PC/ui_mp/ui_class_menu.h"
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
#define CAC_KEY_UP 0x9a
#define CAC_KEY_DOWN 0x9b
#define CAC_KEY_LEFT 0x9c
#define CAC_KEY_RIGHT 0x9d
#define CAC_KEY_F1 0xa7
#define CAC_KEY_KP_ENTER 0xbf
#define CAC_KEY_MOUSE1 0xc8
#define CAC_KEY_MOUSE2 0xc9
#define CAC_KEY_WHEEL_DOWN 0xcd
#define CAC_KEY_WHEEL_UP 0xce
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
/* The equipment rows' icons, 34.5 square, 48 apart. */
#define CAC_ICON_SIZE 34.5f
#define CAC_ICON_STEP 48.0f
#define CAC_RETAIL_IMAGE_ASPECT (199.0f / 112.0f)

enum { CAC_SCREEN_LIST, CAC_SCREEN_EDIT, CAC_SCREEN_WEAPONS, CAC_SCREEN_EQUIPMENT };
enum { CAC_STORE_LOADING, CAC_STORE_READY, CAC_STORE_NO_NAME, CAC_STORE_OFFLINE };

typedef struct {
    float x, y, w, h;
    int horz;
} CacRect;

static struct {
    int open;
    int choose;
    int screen;
    int classFocus;
    int chosen;
    int slotFocus;
    int pickSlot;
    int pickFocus;
    int pickScroll;
    int hover;
    int cursorX, cursorY;
    int renaming;
    char rename[CLASS_NAME_MAX + 1];
    ClassLoadout classes[CLASS_COUNT];
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
} s_cac;

static const char *const s_cacGroups[] = {
    "Submachine Gun", "Rifle", "Automatic Rifle", "Sniper Rifle", "Shotgun", "Pistol",
    "Lethal Fragmentation Grenade", "Tactical Smoke Grenade",
};
static const char *const s_cacNations[] = { "United States", "United Kingdom", "Soviet Union", "Germany" };
static const char *const s_cacSlots[] = { "Primary", "Secondary", "Lethal", "Tactical" };
static const char *const s_cacStats[] = { "Damage", "Range", "Accuracy", "Fire Rate", "Mobility" };
static const vec4_t s_cacRow = { 0.0f, 0.0f, 0.0f, 0.45f };
static const vec4_t s_cacRowFocus = { 0.32f, 0.29f, 0.22f, 0.55f };
static const vec4_t s_cacTile = { 0.0f, 0.0f, 0.0f, 0.5f };
static const vec4_t s_cacEdge = { 1.0f, 1.0f, 1.0f, 0.22f };
static const vec4_t s_cacBar = { 1.0f, 1.0f, 1.0f, 0.18f };
static const vec4_t s_cacShade = { 0.0f, 0.0f, 0.0f, 0.55f };
static const vec4_t s_cacNote = { 0.70f, 0.70f, 0.68f, 1.0f };

static int Cac_Ready(void)
{
    if (!s_cac.initialized) {
        ClassData_Defaults(s_cac.classes);
        s_cac.initialized = 1;
        s_cac.chosen = -1;
    }
    return 1;
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

static void Cac_Frame(const CacRect *rect, const vec_t *color)
{
    CacRect edge = *rect;

    edge.h = CAC_LINE;
    Cac_Fill(&edge, color);
    edge.y = rect->y + rect->h - CAC_LINE;
    Cac_Fill(&edge, color);
    edge.y = rect->y;
    edge.h = rect->h;
    edge.w = CAC_LINE;
    Cac_Fill(&edge, color);
    edge.x = rect->x + rect->w - CAC_LINE;
    Cac_Fill(&edge, color);
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

/* A weapon's picture, its texture coordinates and its width over height: the
 * owner's picture of the weapon when there is one, else the retail menu image
 * for a primary, else the HUD icon's silhouette, cropped to its pixels. */
static MaterialHandle Cac_Image(int index, float *aspect, const float **st)
{
    if (s_cac.imageGeneration != R_GetHudImageGeneration()) {
        s_cac.imageGeneration = R_GetHudImageGeneration();
        memset(s_cac.images, 0, sizeof(s_cac.images));
    }
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
    (void)dc;
    Cac_Load();
    s_cac.open = 1;
    s_cac.choose = choose;
    s_cac.screen = CAC_SCREEN_LIST;
    s_cac.renaming = 0;
    s_cac.hover = -1;
    if (choose && s_cac.chosen >= 0)
        s_cac.classFocus = s_cac.chosen;
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

static void Cac_Choose(displayContextDef_t *dc)
{
    char response[192];
    int menuIndex = -1, i;

    if (ClassData_Response(&s_cac.classes[s_cac.classFocus], response, sizeof(response)) < 0)
        return;
    for (i = 0; i < 0x20; i++) {
        const char *menu = CL_GetConfigString(0x4de + i);

        if (menu && menu[0] && !I_stricmp(menu, "browser_class")) {
            menuIndex = i;
            break;
        }
    }
    if (menuIndex < 0)
        return;
    Cbuf_ExecuteText(2, va("cmd mr %i %i %s\n", Dvar_GetInt("sv_serverId"), menuIndex, response));
    s_cac.chosen = s_cac.classFocus;
    s_cac.open = 0;
    Menus_CloseByName(dc, "browser_class");
}

static void Cac_Back(displayContextDef_t *dc)
{
    if (s_cac.screen == CAC_SCREEN_WEAPONS || s_cac.screen == CAC_SCREEN_EQUIPMENT) {
        s_cac.screen = CAC_SCREEN_EDIT;
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
        loadout->weapon[s_cac.pickSlot] = indexes[candidate];
        Cac_Store();
    }
    s_cac.screen = CAC_SCREEN_EDIT;
    s_cac.slotFocus = s_cac.pickSlot;
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

static void Cac_Move(int delta)
{
    int indexes[64], count;

    switch (s_cac.screen) {
    case CAC_SCREEN_LIST:
        s_cac.classFocus = (s_cac.classFocus + delta + CLASS_COUNT) % CLASS_COUNT;
        break;
    case CAC_SCREEN_EDIT:
        s_cac.slotFocus = (s_cac.slotFocus + delta + CLASS_SLOT_COUNT) % CLASS_SLOT_COUNT;
        break;
    case CAC_SCREEN_WEAPONS:
        count = Cac_SlotCandidates(s_cac.pickSlot, indexes);
        s_cac.pickFocus = (s_cac.pickFocus + delta + count) % count;
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

    if (key != CAC_KEY_TAB && s_cac.screen == CAC_SCREEN_EDIT) {
        /* The editor's two columns of two tiles: primary over secondary, lethal over tactical. */
        int column = s_cac.slotFocus >= CLASS_SLOT_LETHAL, row = s_cac.slotFocus & 1;

        if (vertical)
            row ^= 1;
        else
            column ^= 1;
        s_cac.slotFocus = column * 2 + row;
    } else if (vertical && s_cac.screen == CAC_SCREEN_EQUIPMENT) {
        Cac_OpenPicker(s_cac.pickSlot == CLASS_SLOT_LETHAL ? CLASS_SLOT_TACTICAL : CLASS_SLOT_LETHAL);
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
    case CAC_SCREEN_EDIT:
        Cac_OpenPicker(s_cac.slotFocus);
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
        s_cac.classFocus = s_cac.hover;
        break;
    case CAC_SCREEN_EDIT:
        s_cac.slotFocus = s_cac.hover;
        break;
    case CAC_SCREEN_EQUIPMENT:
        s_cac.pickSlot = s_cac.hover / 4 ? CLASS_SLOT_LETHAL : CLASS_SLOT_TACTICAL;
        s_cac.pickFocus = s_cac.hover % 4;
        break;
    default:
        s_cac.pickFocus = s_cac.hover;
        break;
    }
}

int UI_ClassMenu_WantsKeys(const menuDef_t *menu)
{
    return Cac_MenuKind(menu) != 0;
}

int UI_ClassMenu_KeyEvent(displayContextDef_t *dc, menuDef_t *menu, int key, int down)
{
    int kind = Cac_MenuKind(menu), screen;

    if (!kind)
        return 0;
    if (!s_cac.open)
        UI_ClassMenu_Open(dc, kind == 2);
    if (!down)
        return 1;
    if (s_cac.renaming) {
        if (key == CAC_KEY_ENTER || key == CAC_KEY_KP_ENTER)
            Cac_FinishRename(1);
        else if (key == CAC_KEY_ESCAPE || key == CAC_KEY_MOUSE2)
            Cac_FinishRename(0);
        else
            ClassData_EditName(s_cac.rename, key);
        return 1;
    }
    if (key & CAC_KEY_CHAR) {
        /* As the retail weapon menus' number keys: choose a class at once. */
        int number = (key & ~CAC_KEY_CHAR) - '1';

        if (s_cac.choose && s_cac.screen == CAC_SCREEN_LIST && number >= 0 && number < CLASS_COUNT) {
            s_cac.classFocus = number;
            Cac_Choose(dc);
        }
        return 1;
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
        if (s_cac.hover >= 0) {
            Cac_FocusHover();
            Cac_Activate(dc);
        }
        break;
    case CAC_KEY_F1:
        if (s_cac.screen == CAC_SCREEN_LIST && s_cac.choose)
            s_cac.screen = CAC_SCREEN_EDIT;
        else if (s_cac.screen <= CAC_SCREEN_EDIT)
            Cac_StartRename();
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
    const char *note = NULL;

    if (s_cac.store == CAC_STORE_LOADING)
        note = "Loading your classes...";
    else if (s_cac.store == CAC_STORE_NO_NAME)
        note = "Choose a gametag in Options to save your classes.";
    else if (s_cac.store == CAC_STORE_OFFLINE)
        note = "Classes are unavailable: changes last until you leave.";
    else if (s_cac.saveFailed)
        note = "The last change was not saved.";
    if (note)
        Cac_Text(note, 0, CAC_LEFT, baseline, CAC_ALIGN_LEFT, CAC_ROW_NOTE_CAP, 0.0f, s_cacNote);
}

/* A slot's caption, the weapon's name above the tile, the tile and its
 * picture; gold edges when focused. */
static void Cac_Tile(const CacRect *tile, int slot, int weapon, int focused)
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
    Cac_Picture(weapon, &box, UI_WWIIMenu_Color(WWII_COLOR_WHITE));
}

static void Cac_Stats(int weapon)
{
    const ClassStats *stats = weapon >= 0 && weapon < (int)(sizeof(s_cac.stats) / sizeof(s_cac.stats[0]))
                                  ? &s_cac.stats[weapon]
                                  : NULL;
    const float x = -(CAC_SIDE_RIGHT + CAC_STAT_BAR);
    char text[32];
    int stat;

    if (!stats || !stats->valid)
        return;
    for (stat = 0; stat < CLASS_STAT_COUNT; stat++) {
        float top = CAC_STAT_TOP + (float)stat * CAC_STAT_ROW;
        int rating = ClassData_Rate(stats->value[stat], s_cac.lowest[stat], s_cac.highest[stat]);
        CacRect bar = Cac_Rect(x, top + 2.0f, CAC_STAT_BAR, 1.2f, CAC_ALIGN_RIGHT);

        snprintf(text, sizeof(text), "%d", rating);
        Cac_Text(text, 1, x - 3.0f - UI_WWIIMenu_Width(text, 1, 7.0f, 0.0f), top + 1.5f, CAC_ALIGN_RIGHT, 7.0f, 0.0f,
                 UI_WWIIMenu_Color(WWII_COLOR_WHITE));
        Cac_Caps(s_cacStats[stat], 0, x, top - 1.0f, CAC_ALIGN_RIGHT, 4.2f, 0.12f, UI_WWIIMenu_Color(WWII_COLOR_GOLD));
        Cac_Fill(&bar, s_cacBar);
        bar.w = CAC_STAT_BAR * (float)rating / 10.0f;
        Cac_Fill(&bar, s_cacNote);
    }
    snprintf(text, sizeof(text), "%d/%d", stats->clip, stats->ammo);
    Cac_Caps("Capacity", 0, x, CAC_STAT_TOP + 5.0f * CAC_STAT_ROW + 2.0f, CAC_ALIGN_RIGHT, 4.2f, 0.12f, s_cacNote);
    Cac_Text(text, 1, x, CAC_STAT_TOP + 5.0f * CAC_STAT_ROW + 12.0f, CAC_ALIGN_RIGHT, 7.0f, 0.0f,
             UI_WWIIMenu_Color(WWII_COLOR_WHITE));
}

/* Name, kind and origin, and the retail description. */
static void Cac_Detail(int weapon, float x, float baseline, int horz, float width)
{
    char kind[96];
    const ClassWeapon *entry;

    if (weapon < 0)
        return;
    entry = &g_classWeapons[weapon];
    Cac_Caps(Cac_WeaponName(weapon), 1, x, baseline, horz, 10.0f, 0.06f, UI_WWIIMenu_Color(WWII_COLOR_WHITE));
    snprintf(kind, sizeof(kind), "%s, %s", s_cacGroups[entry->group], s_cacNations[entry->nation]);
    Cac_Text(kind, 1, x, baseline + 11.0f, horz, 4.8f, 0.0f, UI_WWIIMenu_Color(WWII_COLOR_TITLE));
    if (entry->description)
        Cac_Wrapped(Cac_Translate(entry->description, ""), x, baseline + 24.0f, horz, width, 4.4f, 5, s_cacNote);
}

static void Cac_PaintList(displayContextDef_t *dc)
{
    const ClassLoadout *focused;
    float top = CAC_ROW_TOP;
    int i, slot;

    Cac_Title(s_cac.choose ? "Choose Class" : "Create a Class", NULL);
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
        snprintf(note, sizeof(note), "%s, %s", Cac_WeaponName(loadout->weapon[CLASS_SLOT_PRIMARY]),
                 Cac_WeaponName(loadout->weapon[CLASS_SLOT_SECONDARY]));
        Cac_Text(note, 0, CAC_LEFT + 6.0f, top + 22.5f, CAC_ALIGN_LEFT, CAC_ROW_NOTE_CAP, 0.0f, s_cacNote);
        if (s_cac.choose && i == s_cac.chosen)
            Cac_Caps("Equipped", 0, CAC_LEFT + CAC_ROW_WIDTH - 6.0f -
                     UI_WWIIMenu_Width("EQUIPPED", 0, CAC_ROW_NOTE_CAP, 0.12f), top + 12.5f, CAC_ALIGN_LEFT,
                     CAC_ROW_NOTE_CAP, 0.12f, UI_WWIIMenu_Color(WWII_COLOR_GOLD));
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
    for (slot = 0; slot < CLASS_SLOT_COUNT; slot++) {
        float rowTop = CAC_ROW_TOP + 22.0f + (slot ? 84.0f + (float)(slot - 1) * 42.0f : 0.0f);
        float height = slot ? 30.0f : 70.0f;
        CacRect tile = Cac_Rect(-(CAC_SIDE_RIGHT + CAC_SIDE_WIDTH), rowTop, slot ? 96.0f : CAC_SIDE_WIDTH, height,
                                CAC_ALIGN_RIGHT);

        Cac_Tile(&tile, slot, focused->weapon[slot], 0);
    }
}

static const CacRect *Cac_EditTiles(void)
{
    static CacRect tiles[CLASS_SLOT_COUNT];

    /* As WWII's two columns of tiles, each under its two lines: the guns,
     * then the grenades. */
    tiles[CLASS_SLOT_PRIMARY] = Cac_Rect(CAC_LEFT, 125.0f, 170.0f, 96.0f, CAC_ALIGN_LEFT);
    tiles[CLASS_SLOT_SECONDARY] = Cac_Rect(CAC_LEFT, 253.0f, 110.0f, 66.0f, CAC_ALIGN_LEFT);
    tiles[CLASS_SLOT_LETHAL] = Cac_Rect(CAC_LEFT + 188.0f, 125.0f, 60.0f, 60.0f, CAC_ALIGN_LEFT);
    tiles[CLASS_SLOT_TACTICAL] = Cac_Rect(CAC_LEFT + 188.0f, 217.0f, 60.0f, 60.0f, CAC_ALIGN_LEFT);
    return tiles;
}

static void Cac_PaintEdit(displayContextDef_t *dc)
{
    const ClassLoadout *loadout = &s_cac.classes[s_cac.classFocus];
    const CacRect *tiles = Cac_EditTiles();
    char title[64];
    int slot;

    snprintf(title, sizeof(title), "Modify %s", loadout->name);
    Cac_Title(title, NULL);
    for (slot = 0; slot < CLASS_SLOT_COUNT; slot++) {
        if (Cac_Hover(dc, &tiles[slot]))
            s_cac.hover = slot;
        Cac_Tile(&tiles[slot], slot, loadout->weapon[slot], slot == s_cac.slotFocus);
    }
    Cac_StatusNote(366.0f);
    Cac_Detail(loadout->weapon[s_cac.slotFocus], -(CAC_SIDE_RIGHT + CAC_SIDE_WIDTH), 135.0f, CAC_ALIGN_RIGHT,
               CAC_SIDE_WIDTH);
    Cac_Stats(loadout->weapon[s_cac.slotFocus]);
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
    Cac_Picture(indexes[s_cac.pickFocus], &box, UI_WWIIMenu_Color(WWII_COLOR_WHITE));
    Cac_Stats(indexes[s_cac.pickFocus]);
}

static void Cac_PaintEquipment(displayContextDef_t *dc)
{
    const ClassLoadout *loadout = &s_cac.classes[s_cac.classFocus];
    static const int rows[2] = { CLASS_SLOT_TACTICAL, CLASS_SLOT_LETHAL };
    int row, focusWeapon = -1;
    char hint[96];
    CacRect box;

    Cac_Title("Modify Equipment", NULL);
    for (row = 0; row < 2; row++) {
        int indexes[64], count = Cac_SlotCandidates(rows[row], indexes), i;
        float top = row ? 236.0f : 115.0f;
        CacRect line = Cac_Rect(CAC_LEFT, top - 6.0f, 186.0f, CAC_LINE, CAC_ALIGN_LEFT);

        Cac_Fill(&line, s_cacEdge);
        Cac_Caps(s_cacSlots[rows[row]], 0, CAC_LEFT, top + 1.5f, CAC_ALIGN_LEFT, 4.4f, 0.12f, s_cacNote);
        for (i = 0; i < count && i < 4; i++) {
            CacRect tile = Cac_Rect(CAC_LEFT + (float)i * CAC_ICON_STEP, top + 8.0f, CAC_ICON_SIZE, CAC_ICON_SIZE,
                                    CAC_ALIGN_LEFT);
            int isFocus = s_cac.pickSlot == rows[row] && i == s_cac.pickFocus;
            int equipped = indexes[i] == loadout->weapon[rows[row]];
            CacRect picture = tile;

            if (Cac_Hover(dc, &tile))
                s_cac.hover = row * 4 + i;
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
    Cac_Detail(focusWeapon, CAC_LEFT + 4.0f * CAC_ICON_STEP + 12.0f, 135.0f, CAC_ALIGN_LEFT, CAC_DETAIL_WIDTH);
    /* The default binds. Every frag cooks in the hand (BG_GrenadeCanCook); smoke does not. */
    if (s_cac.pickSlot == CLASS_SLOT_LETHAL && s_cac.stats[focusWeapon].fuse > 0.0f)
        snprintf(hint, sizeof(hint), "Hold G to cook it: the %g-second fuse starts with the pin.",
                 (double)s_cac.stats[focusWeapon].fuse);
    else
        snprintf(hint, sizeof(hint), "%s", s_cac.pickSlot == CLASS_SLOT_LETHAL ? "Hold G to cook it." : "Press 4 to throw it.");
    Cac_Text(hint, 0, CAC_LEFT + 4.0f * CAC_ICON_STEP + 12.0f, 172.0f, CAC_ALIGN_LEFT, 4.4f, 0.0f, s_cacNote);
    box = Cac_Rect(90.0f, 190.0f, 130.0f, 130.0f, CAC_ALIGN_CENTER);
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

void UI_ClassMenu_Paint(displayContextDef_t *dc, menuDef_t *menu)
{
    int kind = Cac_MenuKind(menu);
    static const char *const keys[] = { "ENTER", "F1" };
    const char *labels[2];

    if (!kind)
        return;
    if (!s_cac.open)
        UI_ClassMenu_Open(dc, kind == 2);
    Cac_ReadStats();
    /* The text stands on a dark scene, as WWII's: the backdrop or the match. */
    CL_DrawStretchPic(0.0f, 0.0f, 640.0f, 480.0f, CAC_ALIGN_FULLSCREEN, CAC_ALIGN_FULLSCREEN, 0.0f, 0.0f, 1.0f, 1.0f,
                      s_cacShade, sharedUiInfo.assets.whiteMaterial);
    /* Pointer motion moves the focus; keys move it otherwise. */
    if (dc->cursorx != s_cac.cursorX || dc->cursory != s_cac.cursorY) {
        s_cac.cursorX = dc->cursorx;
        s_cac.cursorY = dc->cursory;
        if (!s_cac.renaming)
            Cac_FocusHover();
    }
    s_cac.hover = -1;
    switch (s_cac.screen) {
    case CAC_SCREEN_EDIT:
        Cac_PaintEdit(dc);
        labels[0] = "Select";
        labels[1] = "Rename";
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
    default:
        Cac_PaintList(dc);
        labels[0] = s_cac.choose ? "Select" : "Edit";
        labels[1] = s_cac.choose ? "Edit" : "Rename";
        break;
    }
    if (s_cac.renaming) {
        static const char *const renameKeys[] = { "ENTER" };
        static const char *const renameLabels[] = { "Save" };

        Cac_PaintRename(dc);
        UI_WWIIMenu_Prompts(renameKeys, renameLabels, 1, "ESC", "Cancel");
        return;
    }
    UI_WWIIMenu_Prompts(keys, labels, labels[1] ? 2 : 1, "ESC", "Back");
}
