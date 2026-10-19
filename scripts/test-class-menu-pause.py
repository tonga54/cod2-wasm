#!/usr/bin/env python3
"""Run the production class screens' side of the pause menu: the two lists
it shows, the store's note, a class chosen from either list sent as the class
menu's response and remembered, the class screen's own choice on the same
path, and the class editor opened from the pause menu going back to it."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/ui_mp/ui_class_menu.c').read_text()
data = root / 'src/PC/ui_mp/ui_class_data.c'
hud = (root / 'src/headers/PC/gfx_d3d/r_hud2d.h').read_text()

body = source
for include in ('"common_types.h"', '"imports.h"', '"headers/PC/ui_mp/ui_pause_menu.h"'):
    assert body.count('#include ' + include + '\n') == 1, include
    body = body.replace('#include ' + include + '\n', '')
body = body.replace('#include "headers/PC/gfx_d3d/r_hud2d.h"\n', hud)

support = r'''
#include <assert.h>
#include <math.h>
#include <stdarg.h>
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
typedef struct { unsigned short id, byteCount; } GfxCmdHeader;
typedef union { unsigned int packed; byte array[4]; } GfxColor;
typedef struct { const char *name; } windowDef_t;
typedef struct menuDef_t { windowDef_t window; } menuDef_t;
typedef struct displayContextDef_t { int realTime; int cursorx, cursory; } displayContextDef_t;
typedef struct sharedUiInfo_t { struct { MaterialHandle whiteMaterial; } assets; } sharedUiInfo_t;
typedef struct itemDef_t itemDef_t;
#include "headers/PC/ui_mp/ui_class_data.h"
#include "headers/PC/ui_mp/ui_wwii_menu.h"
static Material white = { "white" }, image = { "image" };
sharedUiInfo_t sharedUiInfo = { { &white } };
const char *UI_SafeTranslateString(const char *reference) { return reference; }
typedef struct { float x, y, w, h; MaterialHandle material; } Pic;
static Pic pics[256];
static int picCount;
void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2,
                       float t2, const vec_t *color, MaterialHandle material) {
    (void)horzAlign; (void)vertAlign; (void)s1; (void)t1; (void)s2; (void)t2; (void)color;
    if (picCount < 256) { pics[picCount].x = x; pics[picCount].y = y; pics[picCount].w = w; pics[picCount].h = h;
        pics[picCount++].material = material; }
}
MaterialHandle CL_RegisterMaterialNoMip(const char *name, int imageTrack) { (void)name; assert(imageTrack == 3); return &image; }
void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign) {
    (void)x; (void)y; (void)w; (void)h; (void)horzAlign; (void)vertAlign; }
void CalcScreenX(float *x, int horzAlign) { (void)x; (void)horzAlign; }
void CalcScreenY(float *y, int vertAlign) { (void)y; (void)vertAlign; }
int FS_ReadFile(const char *qpath, void **buffer) { (void)qpath; *buffer = NULL; return -1; }
void FS_FreeFile(void *buffer) { (void)buffer; }
static int nameLookups;
const char *Dvar_GetVariantString(const char *dvarName) { assert(!strcmp(dvarName, "name")); nameLookups++; return "Tonga"; }
int Dvar_GetInt(const char *name) { assert(!strcmp(name, "sv_serverId")); return 12; }
static char commands[8][256];
static int commandCount;
void Cbuf_ExecuteText(int when, const char *text) { assert(when == 2 && commandCount < 8);
    snprintf(commands[commandCount++], sizeof(commands[0]), "%s", text); }
static const char *configMenus[0x20];
const char *CL_GetConfigString(int index) { assert(index >= 0x4de && index < 0x4de + 0x20); return configMenus[index - 0x4de]; }
char *va(const char *format, ...) { static char buffer[512]; va_list ap; va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap); va_end(ap); return buffer; }
static char menuLog[16][32];
static int menuLogCount, refuseOpen, returns;
void UI_ClassMenu_Open(displayContextDef_t *dc, int choose);
static displayContextDef_t dc = { 0, 320, 240 };
void Menus_CloseByName(displayContextDef_t *d, const char *name) { (void)d; assert(menuLogCount < 16);
    snprintf(menuLog[menuLogCount++], sizeof(menuLog[0]), "-%s", name); }
/* The browser_class menu's onOpen runs the class screens' open script. */
qboolean Menus_OpenByName(displayContextDef_t *d, const char *name) {
    assert(menuLogCount < 16);
    snprintf(menuLog[menuLogCount++], sizeof(menuLog[0]), "+%s", name);
    if (refuseOpen) return 0;
    if (!strcmp(name, "browser_class")) UI_ClassMenu_Open(d, 1);
    return 1;
}
int I_stricmp(const char *s0, const char *s1) { return strcasecmp(s0, s1); }
int R_GetHudImageGeneration(void) { return 1; }
MaterialHandle R_GetHudSilhouetteMaterial(MaterialHandle icon, float *aspect) { (void)icon; *aspect = 2.0f; return &image; }
static const vec4_t gold = { 0.8f, 0.72f, 0.47f, 1.0f };
const float *UI_WWIIMenu_Color(int color) { (void)color; return gold; }
void UI_WWIIMenu_Text(const char *text, int bold, float x, float baseline, int horzAlign, int vertAlign, float cap,
                      float tracking, const vec_t *color) {
    (void)text; (void)bold; (void)x; (void)baseline; (void)horzAlign; (void)vertAlign; (void)cap; (void)tracking; (void)color; }
float UI_WWIIMenu_Width(const char *text, int bold, float cap, float tracking) { (void)bold; (void)tracking;
    return (float)strlen(text) * cap * 0.6f; }
void UI_WWIIMenu_Prompts(const char *const *keys, const char *const *labels, int count, const char *rightKey,
                         const char *rightLabel) { (void)keys; (void)labels; (void)count; (void)rightKey; (void)rightLabel; }
void UI_PauseMenu_Return(displayContextDef_t *d) { (void)d; returns++; Menus_OpenByName(d, "ingame"); }
'''

checks = r'''
static menuDef_t classMenu = { { "browser_class" } };
static int logged(const char *entry) { for (int i = 0; i < menuLogCount; i++) if (!strcmp(menuLog[i], entry)) return 1; return 0; }
static void clearLogs(void) { menuLogCount = commandCount = 0; }
static void key(int k) { assert(UI_ClassMenu_KeyEvent(&dc, &classMenu, k, 1)); }
int main(void) {
    const ClassLoadout *mine, *stock;
    char expected[256], response[192];
    int standard;

    configMenus[4] = "browser_class";
    /* The player's classes load once by gametag; the game's are the defaults. */
    mine = UI_ClassMenu_Classes(0);
    stock = UI_ClassMenu_Classes(1);
    assert(nameLookups >= 1 && mine != stock && !strcmp(stock[0].name, "Assault") && !strcmp(stock[4].name, "Close Quarters"));
    assert(!strcmp(UI_ClassMenu_Note(), "Classes are unavailable: changes last until you leave."));
    s_cac.store = CAC_STORE_LOADING; assert(!strcmp(UI_ClassMenu_Note(), "Loading your classes..."));
    UI_ClassMenu_Loaded(s_cac.generation, 200,
                        "Rush\tmp40_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\n");
    assert(!UI_ClassMenu_Note() && !strcmp(UI_ClassMenu_Classes(0)[0].name, "Rush"));
    assert(!strcmp(UI_ClassMenu_Classes(1)[0].name, "Assault"));
    UI_ClassMenu_Saved(s_cac.generation, 503); assert(!strcmp(UI_ClassMenu_Note(), "The last change was not saved."));
    UI_ClassMenu_Saved(s_cac.generation, 200);
    UI_ClassMenu_Loaded(s_cac.generation, 400, NULL);
    assert(!strcmp(UI_ClassMenu_Note(), "Choose a gametag in Options to save your classes."));
    s_cac.store = CAC_STORE_READY;
    assert(!strcmp(UI_ClassMenu_WeaponName(ClassData_Find("mp40_mp")), "WEAPON_MP40") && !UI_ClassMenu_WeaponName(-1)[0]);

    /* Nothing chosen yet; a class from either list goes as the class menu's
     * response and is the one chosen. */
    assert(UI_ClassMenu_Chosen(&standard) == -1 && !standard);
    clearLogs();
    assert(UI_ClassMenu_Choose(1, 3));
    ClassData_Response(&UI_ClassMenu_Classes(1)[3], response, sizeof(response));
    snprintf(expected, sizeof(expected), "cmd mr 12 4 %s\n", response);
    assert(commandCount == 1 && !strcmp(commands[0], expected) && strstr(commands[0], "kar98k_sniper_mp"));
    assert(UI_ClassMenu_Chosen(&standard) == 3 && standard && s_cac.chosen == -1);
    assert(UI_ClassMenu_Choose(0, 0) && strstr(commands[1], "class,mp40_mp,luger_mp,"));
    assert(UI_ClassMenu_Chosen(&standard) == 0 && !standard);
    /* Without the class menu precached, or for no class, nothing is sent or kept. */
    configMenus[4] = NULL;
    assert(!UI_ClassMenu_Choose(1, 2) && commandCount == 2 && UI_ClassMenu_Chosen(&standard) == 0 && !standard);
    configMenus[4] = "browser_class";
    assert(!UI_ClassMenu_Choose(0, 5) && !UI_ClassMenu_Choose(1, -1) && commandCount == 2);

    /* The class screen's own choice goes the same way and closes it. */
    UI_ClassMenu_Choose(1, 1);
    UI_ClassMenu_Open(&dc, 1);
    s_cac.classFocus = 2; clearLogs(); key(0x0d);
    assert(commandCount == 1 && !strncmp(commands[0], "cmd mr 12 4 class,", 18) && logged("-browser_class"));
    assert(UI_ClassMenu_Chosen(&standard) == 2 && !standard && !s_cac.open);

    /* The editor opened from the pause menu: its Back returns there. */
    clearLogs();
    UI_ClassMenu_EditFromPause(&dc, 3);
    assert(logged("+browser_class") && logged("-ingame") && s_cac.open && s_cac.choose);
    assert(s_cac.screen == CAC_SCREEN_EDIT && s_cac.classFocus == 3 && s_cac.fromPause);
    key(0x0d); assert(s_cac.screen == CAC_SCREEN_WEAPONS);
    key(0x1b); assert(s_cac.screen == CAC_SCREEN_EDIT && returns == 0);
    clearLogs(); key(0x1b);
    assert(returns == 1 && logged("+ingame") && logged("-browser_class") && !s_cac.open && !s_cac.fromPause);
    /* Opened by the server, the same screens go back to the list as before. */
    UI_ClassMenu_EditFromPause(&dc, 1);
    UI_ClassMenu_Open(&dc, 1);
    s_cac.screen = CAC_SCREEN_EDIT; key(0x1b);
    assert(s_cac.screen == CAC_SCREEN_LIST && returns == 1 && s_cac.open);
    /* A class menu that cannot open leaves everything as it was. */
    s_cac.open = 0; refuseOpen = 1; clearLogs();
    UI_ClassMenu_EditFromPause(&dc, 4);
    assert(!logged("-ingame") && !s_cac.fromPause && s_cac.classFocus != 4);
    refuseOpen = 0;
    UI_ClassMenu_EditFromPause(&dc, 9); assert(s_cac.classFocus != 9);

    /* A weapon's picture fills the box at its own width over height. */
    picCount = 0;
    UI_ClassMenu_DrawWeapon(ClassData_Find("mp40_mp"), 10.0f, 20.0f, 199.0f, 200.0f, 1, gold);
    assert(picCount == 1 && fabsf(pics[0].w - 199.0f) < 1e-3f && fabsf(pics[0].h - 112.0f) < 1e-3f);
    assert(fabsf(pics[0].y - 64.0f) < 1e-3f && fabsf(pics[0].x - 10.0f) < 1e-3f);
    picCount = 0;
    UI_ClassMenu_DrawWeapon(ClassData_Find("luger_mp"), 0.0f, 0.0f, 100.0f, 100.0f, 1, gold);
    assert(picCount == 1 && fabsf(pics[0].h - 50.0f) < 1e-3f && fabsf(pics[0].y - 25.0f) < 1e-3f);
    puts("ok");
    return 0;
}
'''

mutants = [
    ('    return standard ? s_cac.standard : s_cac.classes;', '    return s_cac.classes;'),
    ('    s_cac.chosen = standard ? -1 : index;', '    s_cac.chosen = index;'),
    ('    s_cac.chosenStandard = standard ? index : -1;', '    s_cac.chosenStandard = -1;'),
    ('    if (index < 0 || index >= CLASS_COUNT || !Cac_Respond(&UI_ClassMenu_Classes(standard)[index]))',
     '    if (!Cac_Respond(&UI_ClassMenu_Classes(standard)[index]))'),
    ('    s_cac.chosen = s_cac.classFocus;\n    s_cac.chosenStandard = -1;', '    s_cac.chosen = s_cac.classFocus;'),
    ('    } else if (s_cac.screen == CAC_SCREEN_EDIT && s_cac.fromPause) {', '    } else if (0) {'),
    ('        UI_PauseMenu_Return(dc);\n', ''),
    ('    s_cac.fromPause = 0;\n    s_cac.renaming = 0;', '    s_cac.renaming = 0;'),
    ('    if (index < 0 || index >= CLASS_COUNT || !Menus_OpenByName(dc, "browser_class"))',
     '    if (!Menus_OpenByName(dc, "browser_class"))'),
    ('    s_cac.screen = CAC_SCREEN_EDIT;\n    s_cac.fromPause = 1;', '    s_cac.fromPause = 1;'),
    ('        return "The last change was not saved.";', '        return NULL;'),
    ('        ClassData_Defaults(s_cac.standard);\n', ''),
]

with tempfile.TemporaryDirectory(prefix='cod2-class-pause-') as tmp:
    path = Path(tmp)

    def run(variant, name):
        (path / f'{name}.c').write_text(support + '#include "' + str(data) + '"\n' + variant + checks)
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-I', str(root / 'src'), '-fsanitize=address,undefined',
                        '-fno-sanitize-recover=all', '-Wno-unused-function', str(path / f'{name}.c'), '-lm',
                        '-o', str(path / name)], check=True)
        return subprocess.run([str(path / name)], capture_output=True, text=True)

    result = run(body, 'classes')
    assert result.returncode == 0 and result.stdout.strip() == 'ok', result.stderr[-3000:]
    for index, (before, after) in enumerate(mutants):
        assert body.count(before) == 1, before
        assert run(body.replace(before, after), f'mutant{index}').returncode != 0, after
print('PASS: the pause menu lists the player\'s classes and the game\'s five, with the store\'s note; a class from '
      'either list or from the class screen goes as the class menu\'s response and is remembered with its list; the '
      'editor opened from the pause menu goes back to it, the server\'s class menu to its list; pictures keep their '
      f'shape; {len(mutants)} failing regression mutants')
