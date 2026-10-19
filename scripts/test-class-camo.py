#!/usr/bin/env python3
"""Run Create a Class's camouflage against a recording UI: the line under each
gun's tile, F2, a controller's Y or a click opening WWII's Primary or Secondary
Camouflage, the swatches' keys and clicks, equipping a pattern (saved only when
it changes), the guns painted in the tiles and the picker's preview (the
owner's picture of a gun when there is one), the list's panel with each slot's
caption clear of the tile above, the editor's camouflage lines and attachment
boxes together, and the class a match receives."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
menu = (root / 'src/PC/ui_mp/ui_class_menu.c').read_text()
data = (root / 'src/PC/ui_mp/ui_class_data.c').read_text()
# The menu's first-party headers, in its order, but the renderer's: their
# few functions are stubbed below.
headers = ''.join((root / 'src/headers' / header).read_text()
                  for header in re.findall(r'^#include "headers/(PC/[^"]+\.h)"', menu, re.M)
                  if not header.startswith('PC/gfx_d3d/'))

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
typedef struct { int kind; char text[96]; int bold; float x, y, w, h, s2, t2; vec4_t color; MaterialHandle material; } Drawn;
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
    (void)horzAlign; (void)vertAlign; (void)cap; (void)tracking;
    snprintf(d->text, sizeof(d->text), "%s", text);
    d->bold = bold; d->x = x; d->y = baseline;
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
static int imageGeneration;
int R_GetHudImageGeneration(void) { return imageGeneration; }
MaterialHandle R_GetHudSilhouetteMaterial(MaterialHandle icon, float *aspect) { (void)icon; *aspect = 0.0f; return NULL; }
/* The owner's picture of the Kar98k, once there is one: its visible texels
 * three quarters across and half down, three times as wide as tall. */
static Material ownerKar = { "browser_class/kar98k_mp" };
static int ownerPictures;
MaterialHandle R_GetHudPictureMaterial(const char *name, float *st, float *aspect) {
    *aspect = 0.0f;
    if (!ownerPictures || strcmp(name, ownerKar.name))
        return NULL;
    st[0] = 0.0f; st[1] = 0.0f; st[2] = 0.75f; st[3] = 0.5f;
    *aspect = 3.0f;
    return &ownerKar;
}
/* Each camouflage's swatch; the gun's own finish has none. */
static Material swatches[WEAPON_CAMO_COUNT];
MaterialHandle R_GetWeaponCamoMaterial(int camo) {
    assert(camo >= 0 && camo < WEAPON_CAMO_COUNT);
    return camo ? &swatches[camo] : NULL;
}
/* Pictures painted by kind and camouflage, a silhouette cropped 2.5 wide a
 * unit tall; none until the renderer has painted them. */
static Material painted[2][WEAPON_CAMO_COUNT];
static int paintedReady = 1, pictureCalls;
static struct { MaterialHandle source; int silhouette, camo, style; } lastPicture;
MaterialHandle R_GetWeaponCamoPicture(MaterialHandle picture, int silhouette, int camo, int style, float *aspect) {
    assert(picture && camo > WEAPON_CAMO_NONE && camo < WEAPON_CAMO_COUNT && (silhouette == 0 || silhouette == 1));
    pictureCalls++;
    lastPicture.source = picture; lastPicture.silhouette = silhouette; lastPicture.camo = camo; lastPicture.style = style;
    *aspect = 0.0f;
    if (!paintedReady)
        return NULL;
    *aspect = silhouette ? 2.5f : 2.0f;
    return &painted[silhouette][camo];
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
    (void)horzAlign; (void)vertAlign;
    assert(s1 == 0.0f && t1 == 0.0f);
    d->x = x; d->y = y; d->w = w; d->h = h; d->s2 = s2; d->t2 = t2; d->material = material;
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
int Dvar_GetInt(const char *name) { assert(!strcmp(name, "sv_serverId")); return 77; }
static char command[256];
void Cbuf_ExecuteText(int when, const char *text) { assert(when == 2); snprintf(command, sizeof(command), "%s", text); }
/* The class menu is the match's script menu 3. */
const char *CL_GetConfigString(int index) { return index == 0x4de + 3 ? "browser_class" : ""; }
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
const char *UI_SafeTranslateString(const char *reference) {
    return !strncmp(reference, "WEAPON_", 7) ? reference + 7 : reference;
}
float g_webMenuBackdrop;
void UI_PauseMenu_Return(displayContextDef_t *dc) { (void)dc; }
/* The browser's host: the loads asked for and the lines saved. */
static int loads, saves;
static char saved[1024];
void Web_ClassesLoad(const char *player, int generation) { (void)player; (void)generation; loads++; }
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
static menuDef_t createMenu = { { "create_a_class" } }, classMenu = { { "browser_class" } };
static displayContextDef_t dc = { 1000, 0, 0 };
static const float gold[4] = { 0.80f, 0.72f, 0.47f, 1 }, note[4] = { 0.70f, 0.70f, 0.68f, 1 };
static const float edge[4] = { 1, 1, 1, 0.22f }, whole[4] = { 1, 1, 1, 1 };
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
static const Drawn *picOf(MaterialHandle m) {
    for (int i = 0; i < drawCount; i++)
        if (drawn[i].kind == PIC && drawn[i].material == m)
            return &drawn[i];
    return NULL;
}
static int pics(MaterialHandle m) {
    int n = 0;
    for (int i = 0; i < drawCount; i++)
        n += drawn[i].kind == PIC && drawn[i].material == m;
    return n;
}
static int fill(float x, float y, float w, float h, const float *color) {
    for (int i = 0; i < drawCount; i++)
        if (drawn[i].kind == PIC && drawn[i].material == &white && near(drawn[i].x, x) && near(drawn[i].y, y) &&
            near(drawn[i].w, w) && near(drawn[i].h, h) && same(drawn[i].color, color))
            return 1;
    return 0;
}
static int framed(float x, float y, float w, float h, const float *color) {
    const float line = 0.5f;
    return fill(x, y, w, line, color) && fill(x, y + h - line, w, line, color) && fill(x, y, line, h, color) &&
           fill(x + w - line, y, line, h, color);
}
static int prompted(int count, const char *k0, const char *l0, const char *k1, const char *l1, const char *k2,
                    const char *l2) {
    const char *keys[3] = { k0, k1, k2 }, *labels[3] = { l0, l1, l2 };
    if (promptCount != count)
        return 0;
    for (int i = 0; i < count; i++)
        if (strcmp(promptKeys[i], keys[i]) || strcmp(promptLabels[i], labels[i]))
            return 0;
    return 1;
}
static void paint(menuDef_t *m) {
    drawCount = 0;
    promptCount = -1;
    UI_ClassMenu_Paint(&dc, m);
}
static int key(menuDef_t *m, int k) { return UI_ClassMenu_KeyEvent(&dc, m, k, 1); }
/* The cursor over a point of a left-aligned element: 1.5 px a unit there,
 * 2 px a cursor unit across and 1.5 down. */
static void point(float x, float y) {
    dc.cursorx = (int)lroundf(x * 0.75f);
    dc.cursory = (int)lroundf(y);
}
/* A swatch's place: rows of four from the list's top left. */
static float swatchX(int camo) { return CAC_LEFT + (float)(camo % 4) * 33.0f; }
static float swatchY(int camo) { return CAC_ROW_TOP + (float)(camo / 4) * 33.0f; }

int main(void) {
    const int frogskin = BG_WeaponCamo_Find("frogskin"), oakleaf = BG_WeaponCamo_Find("oakleaf");
    const int ember = BG_WeaponCamo_Find("ember");
    MaterialHandle karImage = CL_RegisterMaterialNoMip(g_classWeapons[ClassData_Find("kar98k_mp")].image, 3);
    MaterialHandle lugerIcon = CL_RegisterMaterialNoMip(g_classWeapons[ClassData_Find("luger_mp")].image, 3);
    int cases = 0;

    assert(frogskin == 1 && oakleaf == 4 && ember == WEAPON_CAMO_COUNT - 1);
    for (int i = 0; i < WEAPON_CAMO_COUNT; i++) {
        swatches[i].name = g_weaponCamos[i].id;
        painted[0][i].name = "painted";
        painted[1][i].name = "painted silhouette";
    }
    UI_ClassMenu_Open(&dc, 0);
    assert(loads == 1 && s_cac.store == CAC_STORE_LOADING);
    UI_ClassMenu_Loaded(s_cac.generation, 200,
                        "Kar Rusher\tkar98k_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\tnone\tember\n"
                        "Night Owl\tspringfield_mp\twebley_mp\tfrag_grenade_american_mp\tsmoke_grenade_german_mp\n"
                        "Trench\tshotgun_mp\tcolt_mp\tfrag_grenade_british_mp\tsmoke_grenade_russian_mp\tdesert\tnone\n"
                        "Rat\tppsh_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_american_mp\n"
                        "Bolt\tenfield_mp\tTT30_mp\tfrag_grenade_russian_mp\tsmoke_grenade_russian_mp\n");
    assert(s_cac.store == CAC_STORE_READY && s_cac.classes[0].camo[1] == ember && !s_cac.classes[0].camo[0]);

    /* The list's panel shows the focused class's guns as they are painted:
     * the Luger's silhouette in Ember, the Kar98k in its own finish. */
    paint(&createMenu);
    assert(picOf(&painted[1][ember]) && pictureCalls == 1 && lastPicture.source == lugerIcon &&
           lastPicture.style == WEAPON_CAMO_BODY && pics(karImage) == 1);
    cases++;

    /* The panel's tiles in their order, each slot's caption clear of the
     * tile above it, the last ending above the prompts' keycaps, 58.2 from
     * the bottom. */
    {
        static const char *const captions[CLASS_SLOT_COUNT] = { "PRIMARY", "SECONDARY", "LETHAL", "TACTICAL" };
        const Drawn *tiles[CLASS_SLOT_COUNT];
        int found = 0;

        for (int i = 0; i < drawCount && found < CLASS_SLOT_COUNT; i++)
            if (drawn[i].kind == PIC && drawn[i].material == &white && same(drawn[i].color, s_cacTile) &&
                near(drawn[i].x, -(CAC_SIDE_RIGHT + CAC_SIDE_WIDTH)))
                tiles[found++] = &drawn[i];
        assert(found == CLASS_SLOT_COUNT);
        for (int slot = 1; slot < CLASS_SLOT_COUNT; slot++) {
            const Drawn *caption = text(captions[slot]);
            assert(caption && caption->y - CAC_TILE_CAPTION_CAP >= tiles[slot - 1]->y + tiles[slot - 1]->h + 4.0f);
        }
        assert(tiles[CLASS_SLOT_TACTICAL]->y + tiles[CLASS_SLOT_TACTICAL]->h < 480.0f - 58.2f);
    }
    cases++;

    /* Modify: under each gun, its camouflage with a swatch of the pattern;
     * F2 customizes a gun, as WWII's Y. */
    assert(key(&createMenu, 0x0d) && s_cac.screen == CAC_SCREEN_EDIT && s_cac.slotFocus == CLASS_SLOT_PRIMARY);
    paint(&createMenu);
    {
        const Drawn *none = text("CAMOUFLAGE: NONE"), *redLine = text("CAMOUFLAGE: EMBER"), *swatch = picOf(&swatches[ember]);
        assert(none && redLine && same(none->color, note) && same(redLine->color, note));
        assert(none->y > 125.0f + 96.0f && none->y < 253.0f && redLine->y > 253.0f + 66.0f);
        assert(swatch && near(swatch->s2, 0.55f) && near(swatch->t2, 0.55f) && swatch->x < redLine->x &&
               swatch->y < redLine->y && swatch->y + swatch->h > redLine->y - 5.0f);
        assert(picOf(&painted[1][ember]) && pics(karImage) == 1);
    }
    assert(prompted(3, "ENTER", "Select", "F1", "Rename", "F2", "Customize"));
    cases += 2;

    /* Grenades take no camouflage: no prompt, and F2 or Y do nothing. */
    assert(key(&createMenu, 0x9d) && s_cac.slotFocus == CLASS_SLOT_LETHAL);
    paint(&createMenu);
    assert(prompted(2, "ENTER", "Select", "F1", "Rename", NULL, NULL));
    assert(key(&createMenu, 0xa8) && key(&createMenu, 0xd2) && s_cac.screen == CAC_SCREEN_EDIT);
    cases++;

    /* Primary Camouflage opens on the gun's own: eleven swatches in rows of
     * four, the gun's own finish lettered, the focused one in a double gold
     * frame; its place, name, note and Equipped; the gun's picture. */
    assert(key(&createMenu, 0x9c) && s_cac.slotFocus == CLASS_SLOT_PRIMARY);
    assert(key(&createMenu, 0xa8) && s_cac.screen == CAC_SCREEN_CAMO && s_cac.camoSlot == CLASS_SLOT_PRIMARY &&
           s_cac.camoFocus == WEAPON_CAMO_NONE);
    pictureCalls = 0;
    paint(&createMenu);
    assert(text("PRIMARY CAMOUFLAGE") && text("1/11") && texts("NONE") == 2 && text("The gun's own finish") &&
           text("EQUIPPED") && text("KAR98K"));
    for (int camo = 1; camo < WEAPON_CAMO_COUNT; camo++) {
        const Drawn *swatch = picOf(&swatches[camo]);
        assert(swatch && near(swatch->x, swatchX(camo)) && near(swatch->y, swatchY(camo)) && near(swatch->w, 30.0f) &&
               near(swatch->h, 30.0f) && near(swatch->s2, 0.55f) && framed(swatchX(camo), swatchY(camo), 30, 30, edge));
    }
    assert(framed(swatchX(0), swatchY(0), 30, 30, gold) && framed(swatchX(0) + 0.5f, swatchY(0) + 0.5f, 29, 29, gold));
    assert(!pictureCalls && pics(karImage) == 1 && prompted(1, "ENTER", "Equip", NULL, NULL, NULL, NULL));
    cases += 2;

    /* Frogskin: its note, not equipped, the equipped swatch framed apart,
     * and the Kar98k's picture painted on its wood. */
    assert(key(&createMenu, 0x9d) && s_cac.camoFocus == frogskin);
    paint(&createMenu);
    assert(text("2/11") && text("FROGSKIN") && text("Olive and khaki spots") && !text("EQUIPPED"));
    assert(framed(swatchX(0), swatchY(0), 30, 30, note) && framed(swatchX(1), swatchY(1), 30, 30, gold));
    {
        /* The painted menu image keeps the image's shape and place. */
        const Drawn *preview = picOf(&painted[0][frogskin]);
        assert(preview && near(preview->w / preview->h, CAC_RETAIL_IMAGE_ASPECT) && same(preview->color, whole));
        assert(!pics(karImage));
        assert(lastPicture.source == karImage && !lastPicture.silhouette && lastPicture.style == WEAPON_CAMO_WOOD);
    }
    cases++;

    /* With the owner's picture of the Kar98k, the camouflage paints that
     * picture instead, drawn through its visible texels at its shape. */
    ownerPictures = 1;
    imageGeneration++;
    paint(&createMenu);
    {
        const Drawn *preview = picOf(&painted[0][frogskin]);
        assert(preview && lastPicture.source == &ownerKar && !lastPicture.silhouette &&
               lastPicture.style == WEAPON_CAMO_WOOD);
        assert(near(preview->s2, 0.75f) && near(preview->t2, 0.5f) && near(preview->w / preview->h, 3.0f));
        assert(!pics(karImage) && !pics(&ownerKar));
    }
    ownerPictures = 0;
    imageGeneration++;
    cases++;

    /* Left and right go through the eleven and around; up and down by
     * rows, around, past the places the last row lacks. */
    assert(key(&createMenu, 0x9a) && s_cac.camoFocus == 9 && key(&createMenu, 0x9b) && s_cac.camoFocus == 1);
    for (int i = 0; i < 6; i++)
        key(&createMenu, 0x9d);
    assert(s_cac.camoFocus == 7 && key(&createMenu, 0x9b) && s_cac.camoFocus == 3);
    assert(key(&createMenu, 0x9a) && s_cac.camoFocus == 7);
    for (int i = 0; i < 7; i++)
        key(&createMenu, 0x9c);
    assert(s_cac.camoFocus == 0 && key(&createMenu, 0x9c) && s_cac.camoFocus == WEAPON_CAMO_COUNT - 1);
    assert(key(&createMenu, 0x9d) && s_cac.camoFocus == 0);
    cases++;

    /* Escape goes back unchanged; Enter equips, saves at once and returns
     * to the gun, which wears it. */
    saves = 0;
    assert(key(&createMenu, 0x9d) && key(&createMenu, 0x1b) && s_cac.screen == CAC_SCREEN_EDIT &&
           s_cac.classes[0].camo[0] == WEAPON_CAMO_NONE && !saves);
    assert(key(&createMenu, 0xa8) && s_cac.camoFocus == WEAPON_CAMO_NONE && key(&createMenu, 0x9d) &&
           key(&createMenu, 0x0d));
    assert(s_cac.screen == CAC_SCREEN_EDIT && s_cac.slotFocus == CLASS_SLOT_PRIMARY &&
           s_cac.classes[0].camo[0] == frogskin && saves == 1);
    assert(!strncmp(saved, "Kar Rusher\tkar98k_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\tfrogskin\tember\t\t\n"
                           "Night Owl\tspringfield_mp\twebley_mp\tfrag_grenade_american_mp\tsmoke_grenade_german_mp\tnone\tnone\t\t\n",
                    strlen("Kar Rusher\tkar98k_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\tfrogskin\tember\t\t\n"
                           "Night Owl\tspringfield_mp\twebley_mp\tfrag_grenade_american_mp\tsmoke_grenade_german_mp\tnone\tnone\t\t\n")));
    paint(&createMenu);
    assert(text("CAMOUFLAGE: FROGSKIN") && picOf(&painted[0][frogskin]) && picOf(&painted[1][ember]) && !pics(karImage));
    /* The one it wears again saves nothing. */
    assert(key(&createMenu, 0xa8) && s_cac.camoFocus == frogskin && key(&createMenu, 0x0d) && saves == 1 &&
           s_cac.screen == CAC_SCREEN_EDIT);
    cases += 2;

    /* Down passes the primary's attachment boxes, where Y opens nothing, on
     * the way to the pistol. A controller's Y opens Secondary Camouflage on
     * the pistol's Ember: the silhouette in it, or the icon until the
     * renderer has painted it. */
    assert(key(&createMenu, 0x9b) && s_cac.slotFocus == CAC_FOCUS_PRIMARY_ATTACHMENT);
    assert(key(&createMenu, 0xd2) && s_cac.screen == CAC_SCREEN_EDIT);
    assert(key(&createMenu, 0x9b) && s_cac.slotFocus == CLASS_SLOT_SECONDARY);
    assert(key(&createMenu, 0xd2) && s_cac.screen == CAC_SCREEN_CAMO && s_cac.camoSlot == CLASS_SLOT_SECONDARY &&
           s_cac.camoFocus == ember);
    paint(&createMenu);
    assert(text("SECONDARY CAMOUFLAGE") && text("11/11") && text("EMBER") && text("EQUIPPED") && text("LUGER"));
    assert(lastPicture.source == lugerIcon && lastPicture.silhouette && lastPicture.camo == ember);
    assert(picOf(&painted[1][ember]) && near(picOf(&painted[1][ember])->w / picOf(&painted[1][ember])->h, 2.5f));
    paintedReady = 0;
    paint(&createMenu);
    assert(!picOf(&painted[1][ember]) && pics(lugerIcon) == 1);
    paintedReady = 1;
    cases++;

    /* The pointer: over a gun's camouflage it focuses the gun; with the
     * focus moved off by a key, a click there still opens that gun's picker,
     * a click on a swatch equips it, and the gun's tile takes the focus. */
    assert(key(&createMenu, 0x1b) && s_cac.screen == CAC_SCREEN_EDIT && s_cac.slotFocus == CLASS_SLOT_SECONDARY);
    point(100.0f, 226.0f);
    paint(&createMenu);
    assert(s_cac.hover == CAC_HOVER_CAMO + CLASS_SLOT_PRIMARY && s_cac.slotFocus == CLASS_SLOT_PRIMARY);
    assert(key(&createMenu, 0x9d) && s_cac.slotFocus == CLASS_SLOT_LETHAL);
    paint(&createMenu);
    assert(s_cac.slotFocus == CLASS_SLOT_LETHAL);
    assert(key(&createMenu, 0xc8) && s_cac.screen == CAC_SCREEN_CAMO && s_cac.camoSlot == CLASS_SLOT_PRIMARY);
    point(swatchX(oakleaf) + 15.0f, swatchY(oakleaf) + 15.0f);
    paint(&createMenu);
    assert(key(&createMenu, 0xc8) && s_cac.screen == CAC_SCREEN_EDIT && s_cac.classes[0].camo[0] == oakleaf &&
           s_cac.slotFocus == CLASS_SLOT_PRIMARY && saves == 2);
    cases++;

    /* In a match, Select Class shows the player's guns painted, the game's
     * Standard Issue in their own finish; it sends a class with its
     * camouflage, and one without any as the five fields a server from
     * before it takes. */
    UI_ClassMenu_Open(&dc, 1);
    pictureCalls = 0;
    paint(&classMenu);
    assert(text("SELECT CLASS") && picOf(&painted[0][oakleaf]) && picOf(&painted[1][ember]) && pictureCalls == 2);
    assert(key(&classMenu, 0x9d) && s_cac.tab == CAC_TAB_STANDARD);
    pictureCalls = 0;
    paint(&classMenu);
    assert(text("SELECT CLASS") && !pictureCalls);
    assert(key(&classMenu, 0x9c) && s_cac.tab == CAC_TAB_CUSTOM);
    assert(key(&classMenu, 0x0d) &&
           !strcmp(command, "cmd mr 77 3 class,kar98k_mp,luger_mp,frag_grenade_german_mp,smoke_grenade_german_mp,"
                            "oakleaf,ember,none,none\n"));
    UI_ClassMenu_Open(&dc, 1);
    assert(key(&classMenu, 0x9b) && s_cac.classFocus == 1 && key(&classMenu, 0x0d) &&
           !strcmp(command, "cmd mr 77 3 class,springfield_mp,webley_mp,frag_grenade_american_mp,"
                            "smoke_grenade_german_mp\n"));
    cases++;

    /* With attachments: under each gun its camouflage line, then its
     * attachment boxes, none overlapping. Down reaches the primary's first
     * box and Enter opens Modify Attachment, the gun painted as it wears its
     * camouflage; equipping saves the line with both, Escape returns to the
     * box, and a match receives the class with both. */
    UI_ClassMenu_Open(&dc, 0);
    s_cac.classFocus = 0;
    assert(s_cac.store == CAC_STORE_READY && key(&createMenu, 0x0d) && s_cac.screen == CAC_SCREEN_EDIT);
    s_cac.slotFocus = CLASS_SLOT_PRIMARY;
    {
        const CacRect *tiles = Cac_EditTiles();

        for (int slot = 0; slot < CLASS_CAMO_SLOTS; slot++) {
            const CacRect *box = &tiles[slot == CLASS_SLOT_PRIMARY ? CAC_FOCUS_PRIMARY_ATTACHMENT
                                                                    : CAC_FOCUS_SECONDARY_ATTACHMENT];

            assert(box->y >= tiles[slot].y + tiles[slot].h + 1.5f + CAC_CAMO_LINE_HEIGHT + 1.0f);
        }
        /* Room for the secondary's two lines above its tile. */
        assert(tiles[CAC_FOCUS_PRIMARY_ATTACHMENT].y + CAC_ATTACHMENT_BOX_HEIGHT + 20.0f <= tiles[CLASS_SLOT_SECONDARY].y);
    }
    {
        const int camo = s_cac.classes[0].camo[CLASS_SLOT_PRIMARY];
        char line[160];

        assert(camo > WEAPON_CAMO_NONE && !ClassData_AttachmentCount(&s_cac.classes[0], CLASS_SLOT_PRIMARY));
        assert(key(&createMenu, 0x9b) && s_cac.slotFocus == CAC_FOCUS_PRIMARY_ATTACHMENT);
        paint(&createMenu);
        assert(text("+") && prompted(2, "ENTER", "Select", "F1", "Rename", NULL, NULL));
        assert(key(&createMenu, 0x0d) && s_cac.screen == CAC_SCREEN_ATTACHMENTS && s_cac.attachSlot == CLASS_SLOT_PRIMARY &&
               s_cac.attachPosition == 0);
        pictureCalls = 0;
        paint(&createMenu);
        assert(text("MODIFY ATTACHMENT") && text("4X OPTIC") && lastPicture.camo == camo &&
               picOf(&painted[0][camo]) && prompted(1, "ENTER", "Equip", NULL, NULL, NULL, NULL));
        saves = 0;
        assert(key(&createMenu, 0x0d) && s_cac.screen == CAC_SCREEN_EDIT && saves == 1 &&
               s_cac.slotFocus == CAC_FOCUS_PRIMARY_ATTACHMENT &&
               s_cac.classes[0].attachment[CLASS_SLOT_PRIMARY][0] == ATT_OPTIC4X);
        snprintf(line, sizeof(line), "Kar Rusher\tkar98k_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\t%s\t"
                 "ember\toptic4x\t\n", g_weaponCamos[camo].id);
        assert(!strncmp(saved, line, strlen(line)));
        assert(key(&createMenu, 0x0d) && s_cac.screen == CAC_SCREEN_ATTACHMENTS);
        assert(key(&createMenu, 0x1b) && s_cac.screen == CAC_SCREEN_EDIT && s_cac.slotFocus == CAC_FOCUS_PRIMARY_ATTACHMENT);
        assert(key(&createMenu, 0x1b) && s_cac.screen == CAC_SCREEN_LIST);
        UI_ClassMenu_Open(&dc, 1);
        s_cac.tab = CAC_TAB_CUSTOM;
        s_cac.classFocus = 0;
        snprintf(line, sizeof(line), "cmd mr 77 3 class,kar98k_mp,luger_mp,frag_grenade_german_mp,smoke_grenade_german_mp,"
                 "%s,ember,optic4x,none\n", g_weaponCamos[camo].id);
        assert(key(&classMenu, 0x0d) && !strcmp(command, line));
    }
    cases++;

    printf("%d\n", cases);
    return 0;
}
'''

mutants = [
    ('    case CAC_KEY_BUTTON_Y:\n', ''),
    ('        if (s_cac.screen == CAC_SCREEN_EDIT && s_cac.slotFocus <= CLASS_SLOT_SECONDARY)\n'
     '            Cac_OpenCamo(s_cac.slotFocus);',
     '        if (s_cac.screen == CAC_SCREEN_EDIT)\n            Cac_OpenCamo(s_cac.slotFocus);'),
    ('    if (loadout->camo[s_cac.camoSlot] != camo) {', '    if (1) {'),
    ('    while (row * CAC_CAMO_COLUMNS + column >= WEAPON_CAMO_COUNT);', '    while (0);'),
    ('        if (s_cac.slotFocus <= CLASS_SLOT_SECONDARY)\n            labels[2] = "Customize";',
     '        labels[2] = "Customize";'),
    ('    return slot < CLASS_CAMO_SLOTS ? loadout->camo[slot] : WEAPON_CAMO_NONE;', '    return WEAPON_CAMO_NONE;'),
    ('    s_cac.camoFocus = s_cac.classes[s_cac.classFocus].camo[slot];', '    s_cac.camoFocus = 0;'),
    ('        silhouette = g_classWeapons[index].slot != CLASS_SLOT_PRIMARY;', '        silhouette = 0;'),
    ('    if (gun && s_cac.pictures[index]) {', '    if (0) {'),
    ('st[0], st[1], st[2], st[3], UI_WWIIMenu_Color(WWII_COLOR_WHITE), picture);',
     '0.0f, 0.0f, 1.0f, 1.0f, UI_WWIIMenu_Color(WWII_COLOR_WHITE), picture);'),
    ('s_cac.screen == CAC_SCREEN_EQUIPMENT ||\n               s_cac.screen == CAC_SCREEN_CAMO) {',
     's_cac.screen == CAC_SCREEN_EQUIPMENT) {'),
    ('        } else if (s_cac.screen == CAC_SCREEN_EDIT && s_cac.hover >= CAC_HOVER_CAMO) {', '        } else if (0) {'),
    ('            s_cac.hover = CAC_HOVER_CAMO + slot;', '            s_cac.hover = slot;'),
    ('        Cac_CamoRows(key == CAC_KEY_UP ? -1 : 1);',
     '        Cac_Move(key == CAC_KEY_UP ? -CAC_CAMO_COLUMNS : CAC_CAMO_COLUMNS);'),
    ('0.0f, 0.0f, 0.55f, 0.55f,', '0.0f, 0.0f, 1.0f, 1.0f,'),
    ('    if (s_cac.camoFocus == loadout->camo[s_cac.camoSlot])\n        Cac_Caps("Equipped"',
     '    if (1)\n        Cac_Caps("Equipped"'),
    ('    s_cac.slotFocus = s_cac.camoSlot;\n', ''),
    ('"Camouflage: %s", g_weaponCamos[camo].name);', '"Camouflage: %s", g_weaponCamos[0].name);'),
    ('    Cac_GunPicture(weapon, s_cac.camoFocus, Cac_SlotAttachments(loadout, s_cac.camoSlot), &box,',
     '    Cac_GunPicture(weapon, loadout->camo[s_cac.camoSlot], Cac_SlotAttachments(loadout, s_cac.camoSlot), &box,'),
    ('Cac_GunPicture(weapon, Cac_SlotCamo(&Cac_Listed()[s_cac.classFocus], slot),',
     'Cac_GunPicture(weapon, WEAPON_CAMO_NONE,'),
    ('Cac_GunPicture(weapon, Cac_SlotCamo(&Cac_Listed()[s_cac.classFocus], slot),',
     'Cac_GunPicture(weapon, Cac_SlotCamo(&s_cac.classes[s_cac.classFocus], slot),'),
    ('#define CAC_ATTACHMENT_BOX_TOP (1.5f + CAC_CAMO_LINE_HEIGHT + 2.5f)', '#define CAC_ATTACHMENT_BOX_TOP 4.0f'),
    ('    Cac_GunPicture(weapon, Cac_SlotCamo(loadout, slot), preview, &box, UI_WWIIMenu_Color(WWII_COLOR_WHITE));\n'
     '    Cac_Stats(weapon, mask,',
     '    Cac_Picture(weapon, &box, UI_WWIIMenu_Color(WWII_COLOR_WHITE));\n    Cac_Stats(weapon, mask,'),
    ('        Cac_ReturnToAttachment(s_cac.attachSlot, s_cac.attachPosition);\n    } else if', '    } else if'),
    ('(slot ? 90.0f + (float)(slot - 1) * 50.0f : 0.0f)', '(slot ? 84.0f + (float)(slot - 1) * 42.0f : 0.0f)'),
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


with tempfile.TemporaryDirectory(prefix='cod2-class-camo-') as directory:
    objects = []
    for source in data_sources:
        objects.append(str(Path(directory) / (source.stem + '.o')))
        subprocess.run(['cc', *flags, '-w', '-c', str(source), '-o', objects[-1]], check=True)
    result = run(directory, 'camo', program(menu), ['-Wall', '-Werror', '-Wno-unused-function'], objects)
    assert result.returncode == 0, (result.stdout[-500:], result.stderr[-3000:])
    cases = int(result.stdout)
    for index, (old, new) in enumerate(mutants):
        assert menu.count(old) == 1, old
        result = run(directory, f'camo{index}', program(menu.replace(old, new)), ['-w'], objects)
        assert result.returncode != 0, f'mutant survived: {new!r}'

print(f'PASS: Create a Class paints each gun\'s camouflage under its tile and in the list\'s panel, whose slot captions '
      f'clear the tiles above them; F2, a controller\'s Y '
      f'or a click opens Primary or Secondary Camouflage on the gun\'s own, a grid of the eleven swatches with the '
      f'focused one\'s note and the gun, or the owner\'s picture of it, painted in it; keys and clicks move and equip, '
      f'saving only a change; grenades '
      f'take none; Select Class shows the player\'s guns painted and Standard Issue as issued, and a match receives '
      f'the class with its camouflage, an unpainted one as five fields; {cases} cases, '
      f'{len(mutants)} failing mutants')
