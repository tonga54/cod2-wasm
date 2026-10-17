#!/usr/bin/env python3
"""Run the production WWII weapon prompt inside the real stock cursor hint, with
the real slot rules: which hints it takes over, that it chooses swap, pick up or
icon only exactly as the stock hint does, and its layout, colours, fonts,
silhouettes and fade."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_weaponprompt_mp.c').read_text()
newdraw = (root / 'src/PC/cgame_mp/cg_newDraw_mp.c').read_text()
weapons = (root / 'src/PC/bgame/bg_weapons.c').read_text()
main = (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()


def function(text, name):
    match = re.search(r'^((?:static )?[^\n;]*\b' + name + r'\([^;]*?\))\s*\{', text, re.M)
    assert match, name
    brace = text.index('{', match.end() - 1)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


# The stock cursor hint hands its weapons to the prompt; every level starts
# with the prompt's dvar and font registered.
assert newdraw.count('#include "headers/PC/cgame_mp/cg_weaponprompt.h"\n') == 1
assert newdraw.count('CG_DrawWeaponPrompt(') == 1
assert main.count('#include "headers/PC/cgame_mp/cg_weaponprompt.h"\n') == 1
assert main.count('    CG_AmmoCounterInit();\n    CG_WeaponPromptInit();\n') == 1

prompt = source[source.index('/* Virtual units from the stock hint'):]
cursorhint = function(newdraw, 'CG_DrawCursorhint')
slots = function(weapons, 'BG_DoesWeaponNeedSlot') + function(weapons, 'BG_GetEmptySlotForWeapon')

support = r'''
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int qboolean;
typedef int Bool;
typedef float vec_t;
typedef vec_t vec4_t[4];
typedef const char *MaterialHandle;
struct Font_s { int bold; float scale; };
typedef struct Font_s *FontHandle;
typedef struct { struct { int enabled; int integer; } current; } dvar_t;
typedef struct rectDef_s { float x, y, w, h; int horzAlign, vertAlign; } rectDef_t;
typedef struct { const char *szDisplayName; int weapClass; int weapSlot; int offhandClass; qboolean bWideListIcon; } WeaponDef;
typedef struct { int pm_flags; int weapon; char weaponslots[8]; int stats[6]; int cursorHint; int cursorHintString; } playerState_t;
typedef struct { playerState_t ps; } snapshot_t;
#define PMF_GRENADE_THROWBACK 0x04000000
#define GRENADE_THROWBACK_HINT 255
static struct { int time; snapshot_t *snap, *nextSnap; int renderingThirdPerson; int cursorHintIcon, cursorHintTime,
                cursorHintFade, cursorHintString; playerState_t predictedPlayerState; } cgState;
static struct { struct { MaterialHandle hintMaterials[256]; } media; } cgsState;
#define cg (&cgState)
#define cgs (&cgsState)

static const char *va(const char *fmt, ...) {
    static char buffers[4][128];
    static int next;
    char *buffer = buffers[next++ & 3];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffers[0]), fmt, args);
    va_end(args);
    return buffer;
}
static dvar_t promptDvar = { { 1, 0 } }, cursorHints = { { 0, 4 } }, hintFadeTime = { { 0, 1000 } };
static const dvar_t *cg_cursorHints = &cursorHints, *cg_hintFadeTime = &hintFadeTime;
static const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags) {
    assert(!strcmp(name, "cg_hudWeaponPrompt") && value == 1 && flags == 0x1001);
    return &promptDvar;
}
/* One font per enum and scale, so a width is always measured with the drawn
 * font; the bold font is registered by name. */
static struct Font_s fonts[16], boldFont = { 1, 0 };
static int fontEnums[16], fontCount, boldRegistered;
static FontHandle UI_GetFontHandle(int fontEnum, float scale) {
    for (int i = 0; i < fontCount; i++)
        if (fonts[i].scale == scale && fontEnums[i] == fontEnum) return &fonts[i];
    assert(fontCount < 16);
    fonts[fontCount].scale = scale; fontEnums[fontCount] = fontEnum;
    return &fonts[fontCount++];
}
static FontHandle CL_RegisterFont(const char *name, int imageTrack) {
    assert(!strcmp(name, "$boldfont") && imageTrack == 0);
    boldRegistered++;
    return &boldFont;
}
static int UI_TextWidth(const char *text, int maxChars, FontHandle font, float scale) {
    assert(maxChars == 0 && (font->bold || font->scale == scale));
    return (int)(strlen(text) * (font->bold ? 30.0f : 20.0f) * scale);
}
static int UI_TextHeight(FontHandle font, float scale) {
    assert(font->bold || font->scale == scale);
    return (int)(48.0f * scale);
}
enum { TEXT, STRETCH, HANDLE };
typedef struct { int kind; char text[64]; MaterialHandle material; FontHandle font; float x, y, w, h, scale;
                 vec4_t color; int horz, vert, style; } Drawn;
static Drawn drawn[16];
static int drawCount;
static Drawn *record(int kind, float x, float y, int horzAlign, int vertAlign, const vec_t *color) {
    assert(drawCount < 16);
    Drawn *d = &drawn[drawCount++];
    memset(d, 0, sizeof(*d));
    d->kind = kind; d->x = x; d->y = y; d->horz = horzAlign; d->vert = vertAlign;
    memcpy(d->color, color, sizeof(d->color));
    return d;
}
static void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign,
                        int vertAlign, float scale, const vec_t *color, int style) {
    assert(maxChars == 0x7fffffff && (font->bold || font->scale == scale));
    Drawn *d = record(TEXT, x, y, horzAlign, vertAlign, color);
    snprintf(d->text, sizeof(d->text), "%s", text);
    d->font = font; d->scale = scale; d->style = style;
}
static void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1,
                              float s2, float t2, const vec_t *color, MaterialHandle material) {
    assert(s1 == 0 && t1 == 0 && s2 == 1 && t2 == 1);
    Drawn *d = record(STRETCH, x, y, horzAlign, vertAlign, color);
    d->w = w; d->h = h; d->material = material;
}
static void UI_DrawHandlePic(float x, float y, float w, float h, int horzAlign, int vertAlign, const vec_t *color,
                             MaterialHandle material) {
    Drawn *d = record(HANDLE, x, y, horzAlign, vertAlign, color);
    d->w = w; d->h = h; d->material = material;
}
static vec4_t fadeState;
static float fadeAlpha = 1;
static int fadeStart, fadeTotal, fadeMsec, configs;
static float *CG_FadeColor(int startMsec, int totalMsec, int fadeMsecArg) {
    fadeStart = startMsec; fadeTotal = totalMsec; fadeMsec = fadeMsecArg;
    if (fadeAlpha <= 0) return NULL;
    fadeState[0] = fadeState[1] = fadeState[2] = 1; fadeState[3] = fadeAlpha;
    return fadeState;
}
static void Controls_GetConfig(void) { configs++; }
static char boundKeys[64] = "F";
static int bindCalls;
static int GetKeyBindingLocalizedString(const char *command, char *keys) {
    assert(!strcmp(command, "+activate"));
    bindCalls++;
    strcpy(keys, boundKeys);
    return 1;
}
static const char *UI_SafeTranslateString(const char *ref) {
    static const char *const names[][2] = { { "WEAPON_LUGER", "Luger" }, { "WEAPON_KAR98K", "Kar98k" },
                                            { "WEAPON_STEN", "Sten" }, { "WEAPON_ITEM", "Binoculars" },
                                            { "WEAPON_WEBLEY", "Webley" } };
    for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); i++)
        if (!strcmp(ref, names[i][0])) return names[i][1];
    return ref;
}
static const char *UI_ReplaceConversionString(const char *s, const char *binding) { return va("%s[%s]", s, binding); }
static const char *CG_GetUseString(void) { return "USE STRING"; }
/* 1 Luger, 2 Kar98k, 3 Sten, 4 a frag grenade, 5 an item, 6 Webley, 7 a gun
 * that needs a slot but has none, 8 a mounted MG42. */
static WeaponDef defs[9];
static WeaponDef *bg_weaponDefs[9];
static WeaponDef *BG_GetWeaponDef(int weapon) { assert(weapon >= 0 && weapon < 9); return &defs[weapon]; }
/* The rifle, SMG and Luger can be made silhouettes; the rest keep stock icons. */
static MaterialHandle R_GetHudSilhouetteMaterial(MaterialHandle icon, float *aspect) {
    if (!strcmp(icon, "hud_sten")) { *aspect = 4.0f; return "sil:hud_sten"; }
    if (!strcmp(icon, "hud_kar98k")) { *aspect = 8.0f; return "sil:hud_kar98k"; }
    if (!strcmp(icon, "hud_luger")) { *aspect = 1.5f; return "sil:hud_luger"; }
    *aspect = 0.0f;
    return NULL;
}
'''

checks = r'''
static int near(float a, float b) { return fabsf(a - b) < 1e-3f; }
static const float gold[3] = { 0.95f, 0.84f, 0.34f }, white[3] = { 0.96f, 0.95f, 0.92f },
                   cream[3] = { 0.96f, 0.94f, 0.86f }, black[3] = { 0, 0, 0 };
static int horz = 2, vert = 2, style = 6;
static int colored(const float *color, const float *rgb, float alpha) {
    return near(color[0], rgb[0]) && near(color[1], rgb[1]) && near(color[2], rgb[2]) && near(color[3], alpha);
}
static int text(int i, const char *value, FontHandle font, float x, float y, const float *rgb, float alpha) {
    const Drawn *d = &drawn[i];
    return i < drawCount && d->kind == TEXT && !strcmp(d->text, value) && d->font == font && near(d->x, x) &&
           near(d->y, y) && near(d->scale, 0.24f) && d->horz == horz && d->vert == vert && d->style == style &&
           colored(d->color, rgb, alpha);
}
static int pic(int i, MaterialHandle material, float x, float y, float w, float h, const float *rgb, float alpha) {
    const Drawn *d = &drawn[i];
    return i < drawCount && d->kind == STRETCH && !strcmp(d->material, material) && near(d->x, x) && near(d->y, y) &&
           near(d->w, w) && near(d->h, h) && d->horz == horz && d->vert == vert && colored(d->color, rgb, alpha);
}
/* The silhouette over its shadow, one unit down and right. */
static int icon(int i, MaterialHandle material, float x, float y, float w, float h, float alpha) {
    return pic(i, material, x + 1, y + 1, w, h, black, alpha * 0.6f) && pic(i + 1, material, x, y, w, h, cream, alpha);
}
static float width(const char *value, int bold) { return (float)(int)(strlen(value) * (bold ? 30.0f : 20.0f) * 0.24f); }
/* The key in gold and the line in near white, centred together on x. */
static int line(const char *key, const char *value, FontHandle keyFont, float x, float y, float alpha) {
    float keyWidth = width(key, keyFont->bold);
    float left = x - 0.5f * (keyWidth + 4 + width(value, 0));
    return text(0, key, keyFont, left, y, gold, alpha) &&
           text(1, value, UI_GetFontHandle(0, 0.24f), left + keyWidth + 4, y, white, alpha);
}
static int count(int kind) {
    int n = 0;
    for (int i = 0; i < drawCount; i++) n += drawn[i].kind == kind;
    return n;
}
static rectDef_t rect = { 0, 70, 40, 40, 2, 2 };
static snapshot_t snapState;
static FontHandle menuFont;
static void loadout(int current, int primary, int primaryb) {
    playerState_t *ps = &cg->predictedPlayerState;
    memset(ps, 0, sizeof(*ps));
    ps->weapon = current; ps->weaponslots[1] = (char)primary; ps->weaponslots[2] = (char)primaryb;
}
static void aim(int hint, int string) { snapState.ps.cursorHint = hint; snapState.ps.cursorHintString = string; }
static void draw(void) { drawCount = 0; CG_DrawCursorhint(&rect, menuFont, 0.21f, 6); }
enum { NONE, PICK_UP, SWAP };
/* What the stock hint chose, from its text; the prompt's from its line. */
static int stockChoice(void) {
    assert(!count(STRETCH) && count(HANDLE) == 1);
    if (!count(TEXT)) return NONE;
    assert(count(TEXT) == 1);
    if (!strcmp(drawn[0].text, "PLATFORM_SWAPWEAPONS[F]")) return SWAP;
    assert(!strcmp(drawn[0].text, "PLATFORM_PICKUPNEWWEAPON[F]"));
    return PICK_UP;
}
static int promptChoice(void) {
    assert(!count(HANDLE) && count(STRETCH) == 2);
    if (!count(TEXT)) return NONE;
    assert(count(TEXT) == 2);
    if (!strncmp(drawn[1].text, "Press to swap for ", 18)) return SWAP;
    assert(!strncmp(drawn[1].text, "Press to pick up ", 17));
    return PICK_UP;
}
static void weapon(int index, const char *name, int weapClass, int weapSlot, int offhandClass, int wide,
                   MaterialHandle hudIcon) {
    defs[index].szDisplayName = name; defs[index].weapClass = weapClass; defs[index].weapSlot = weapSlot;
    defs[index].offhandClass = offhandClass; defs[index].bWideListIcon = wide;
    cgs->media.hintMaterials[index + 4] = hudIcon;
}
int main(void) {
    for (int i = 0; i < 9; i++) bg_weaponDefs[i] = &defs[i];
    defs[0].szDisplayName = "WEAPON_NONE";
    weapon(1, "WEAPON_LUGER", 4, 2, 0, 0, "hud_luger");
    weapon(2, "WEAPON_KAR98K", 0, 1, 0, 1, "hud_kar98k");
    weapon(3, "WEAPON_STEN", 2, 1, 0, 1, "hud_sten");
    weapon(4, "WEAPON_FRAG", 5, 0, 1, 0, "hud_frag");
    weapon(5, "WEAPON_ITEM", 9, 0, 0, 1, "hud_item");
    weapon(6, "WEAPON_WEBLEY", 4, 2, 0, 0, "hud_webley");
    weapon(7, "WEAPON_ODD", 0, 0, 0, 0, "hud_odd");
    weapon(8, "WEAPON_MG42", 7, 0, 0, 1, "hud_mg42");
    cgs->media.hintMaterials[0] = "hud_none"; cgs->media.hintMaterials[3] = "hud_health";
    cg->snap = cg->nextSnap = &snapState;
    cg->time = 5000;
    menuFont = UI_GetFontHandle(0, 0.21f);
    assert(!CG_WeaponPromptActive());
    CG_WeaponPromptInit();
    assert(CG_WeaponPromptActive() && boldRegistered == 1 && s_weaponPromptKeyFont == &boldFont);

    /* Holding the Kar98k with the Luger, a Sten on the ground is a swap: the
     * key and the line share a baseline 4 under the anchor, the silhouette
     * fills a 14-unit-high box from 7 under the baseline, all centred. */
    loadout(2, 2, 1);
    aim(3 + 4, -1);
    draw();
    assert(drawCount == 4 && line("F", "Press to swap for Sten", &boldFont, 0, 74, 1));
    assert(icon(2, "sil:hud_sten", -28, 81, 56, 14, 1));
    assert(bindCalls == 1 && configs == 1 && cg->cursorHintIcon == 7);
    assert(fadeStart == 5000 && fadeTotal == 1000 && fadeMsec == 100);

    /* With a free slot it is a pick-up; a long rifle is held to the box's
     * width and centred in its height. */
    loadout(3, 3, 0);
    aim(2 + 4, -1);
    draw();
    assert(drawCount == 4 && line("F", "Press to pick up Kar98k", &boldFont, 0, 74, 1));
    assert(icon(2, "sil:hud_kar98k", -42, 82.75f, 84, 10.5f, 1));
    /* Weapons that need no slot are always picked up; without a silhouette the
     * stock icon keeps its own shape, wide or square. */
    loadout(3, 3, 1);
    aim(5 + 4, -1);
    draw();
    assert(drawCount == 4 && line("F", "Press to pick up Binoculars", &boldFont, 0, 74, 1));
    assert(icon(2, "hud_item", -14, 81, 28, 14, 1));
    aim(6 + 4, -1);
    draw();
    assert(drawCount == 4 && line("F", "Press to swap for Webley", &boldFont, 0, 74, 1));
    assert(icon(2, "hud_webley", -7, 81, 14, 14, 1));
    /* A weapon already in the slot it would take shows its silhouette alone,
     * as the stock hint shows its icon alone. */
    bindCalls = 0;
    aim(1 + 4, -1);
    draw();
    assert(drawCount == 2 && icon(0, "sil:hud_luger", -10.5f, 81, 21, 14, 1) && !bindCalls);
    aim(3 + 4, -1);
    draw();
    assert(drawCount == 2 && icon(0, "sil:hud_sten", -28, 81, 56, 14, 1));

    /* The key reads as the stock hint names it; without the bold font the
     * bold UI font of the prompt's size stands in. */
    loadout(2, 2, 1);
    strcpy(boundKeys, "F or E");
    aim(3 + 4, -1);
    draw();
    assert(line("F or E", "Press to swap for Sten", &boldFont, 0, 74, 1));
    strcpy(boundKeys, "F");
    s_weaponPromptKeyFont = NULL;
    draw();
    assert(line("F", "Press to swap for Sten", UI_GetFontHandle(4, 0.24f), 0, 74, 1));
    s_weaponPromptKeyFont = &boldFont;

    /* It fades with the stock hint, pulse included, and ignores the stock
     * size pulse. */
    fadeAlpha = 0.8f;
    cursorHints.current.integer = 3; cg->time = 0;
    draw();
    assert(drawCount == 4 && line("F", "Press to swap for Sten", &boldFont, 0, 74, 0.4f));
    assert(icon(2, "sil:hud_sten", -28, 81, 56, 14, 0.4f));
    cursorHints.current.integer = 1; cg->time = 1000;
    draw();
    assert(drawCount == 4 && line("F", "Press to swap for Sten", &boldFont, 0, 74, 0.8f));
    assert(icon(2, "sil:hud_sten", -28, 81, 56, 14, 0.8f));
    fadeAlpha = 0;
    draw();
    assert(drawCount == 0 && cg->cursorHintIcon == 0);
    fadeAlpha = 1; cursorHints.current.integer = 4; cg->time = 5000;

    /* It centres on the item's rect, with its alignment and text style. */
    horz = 7; vert = 1; style = 3;
    {
        rectDef_t moved = { 10, 30, 40, 40, 7, 1 };
        const float fade[4] = { 1, 1, 1, 0.5f };
        drawCount = 0;
        CG_DrawWeaponPrompt(&moved, 3, 3, "hud_sten", fade);
        assert(drawCount == 4 && line("F", "Press to swap for Sten", &boldFont, 10, 34, 0.5f));
        assert(icon(2, "sil:hud_sten", -18, 41, 56, 14, 0.5f));
    }
    horz = 2; vert = 2; style = 6;

    /* Mounted guns, other hints and a prompt switched off keep the stock hint. */
    aim(8 + 4, 5);
    draw();
    assert(drawCount == 2 && drawn[0].kind == TEXT && !strcmp(drawn[0].text, "USE STRING") && drawn[0].font == menuFont);
    assert(drawn[1].kind == HANDLE && !strcmp(drawn[1].material, "hud_mg42"));
    aim(3, -1);
    draw();
    assert(drawCount == 2 && !strcmp(drawn[0].text, "PLATFORM_PICKUPHEALTH[F]") && !strcmp(drawn[1].material, "hud_health"));
    promptDvar.current.enabled = 0;
    assert(!CG_WeaponPromptActive());
    aim(3 + 4, -1);
    draw();
    assert(stockChoice() == SWAP && drawn[0].font == menuFont && !strcmp(drawn[1].material, "hud_sten"));

    /* Over every loadout of these weapons, the prompt chooses as the stock
     * hint does. */
    {
        static const int primaries[] = { 0, 2, 3, 7 }, secondaries[] = { 0, 1, 6 };
        int compared = 0, choices[3] = { 0, 0, 0 };
        for (int current = 0; current <= 8; current++)
            for (int p = 0; p < 4; p++)
                for (int s = 0; s < 3; s++)
                    for (int ground = 1; ground <= 7; ground++) {
                        int stock;
                        loadout(current, primaries[p], secondaries[s]);
                        aim(ground + 4, -1);
                        promptDvar.current.enabled = 0;
                        draw();
                        stock = stockChoice();
                        promptDvar.current.enabled = 1;
                        draw();
                        assert(promptChoice() == stock);
                        choices[stock]++;
                        compared++;
                    }
        assert(compared == 756 && choices[NONE] && choices[PICK_UP] && choices[SWAP]);
    }
    return 0;
}
'''

mutants = [
    ('if (!BG_DoesWeaponNeedSlot(weapon) || BG_GetEmptySlotForWeapon(ps, weapon))',
     'if (BG_GetEmptySlotForWeapon(ps, weapon))'),
    ('if (!BG_DoesWeaponNeedSlot(weapon) || BG_GetEmptySlotForWeapon(ps, weapon))', 'if (!BG_DoesWeaponNeedSlot(weapon))'),
    ('if (BG_GetWeaponDef(ps->weapon)->weapSlot == weapDef->weapSlot)', 'if (0)'),
    ('return ps->weapon == weapon ? WEAPON_PROMPT_ICON_ONLY : WEAPON_PROMPT_SWAP;', 'return WEAPON_PROMPT_SWAP;'),
    ('return (signed char)ps->weaponslots[weapDef->weapSlot] == weapon ? WEAPON_PROMPT_ICON_ONLY : WEAPON_PROMPT_SWAP;',
     'return WEAPON_PROMPT_SWAP;'),
    ('s_weaponPromptKeyFont ? s_weaponPromptKeyFont : UI_GetFontHandle(4, WEAPON_PROMPT_SCALE)',
     'UI_GetFontHandle(4, WEAPON_PROMPT_SCALE)'),
    ('float x = rect->x - 0.5f * (keyWidth', 'float x = -0.5f * (keyWidth'),
    ('(keyWidth + WEAPON_PROMPT_KEY_GAP +', '(keyWidth +'),
    ('x + keyWidth + WEAPON_PROMPT_KEY_GAP, y', 'x + keyWidth, y'),
    ('float y = rect->y + WEAPON_PROMPT_BASELINE;', 'float y = rect->y;'),
    ('CG_WeaponPromptColor(color, s_weaponPromptKey, alpha);', 'CG_WeaponPromptColor(color, s_weaponPromptText, alpha);'),
    ('WEAPON_PROMPT_SCALE, color,\n                textStyle);', 'WEAPON_PROMPT_SCALE, color,\n                3);'),
    ('action == WEAPON_PROMPT_SWAP ? "Press to swap for %s" : "Press to pick up %s"', '"Press to swap for %s"'),
    ('UI_SafeTranslateString(weapDef->szDisplayName)', 'weapDef->szDisplayName'),
    ('                                fadeColor[3]);', '                                1.0f);'),
    ('if (action != WEAPON_PROMPT_ICON_ONLY) {', 'if (1) {'),
    ('aspect = wide ? 2.0f : 1.0f;', 'aspect = 1.0f;'),
    ('    if (w > WEAPON_PROMPT_ICON_WIDTH) {\n        w = WEAPON_PROMPT_ICON_WIDTH;\n        h = w / aspect;\n    }\n', ''),
    ('x = rect->x - 0.5f * w;', 'x = -0.5f * w;'),
    (' + 0.5f * (WEAPON_PROMPT_ICON_HEIGHT - h);', ';'),
    ('color[3] = alpha * WEAPON_PROMPT_SHADOW_ALPHA;', 'color[3] = alpha;'),
    ('CL_DrawStretchPic(x + WEAPON_PROMPT_SHADOW_OFFSET, y + WEAPON_PROMPT_SHADOW_OFFSET,', 'CL_DrawStretchPic(x, y,'),
    ('CG_WeaponPromptColor(color, s_weaponPromptIcon, alpha);', 'CG_WeaponPromptColor(color, s_weaponPromptText, alpha);'),
    ('CG_WeaponPromptDrawIcon(rect, icon, weapDef->bWideListIcon, fadeColor[3]);',
     'CG_WeaponPromptDrawIcon(rect, icon, weapDef->bWideListIcon, 1.0f);'),
    ('return cg_hudWeaponPrompt && cg_hudWeaponPrompt->current.enabled;', 'return cg_hudWeaponPrompt != 0;'),
    ('    s_weaponPromptKeyFont = CL_RegisterFont("$boldfont", 0);\n', ''),
]
hook_mutants = [
    ('            if (CG_WeaponPromptActive()) {', '            if (0) {'),
    ('CG_DrawWeaponPrompt(rect, textStyle, weapIdx, hintIcon, fadeColor);\n                return;\n',
     'CG_DrawWeaponPrompt(rect, textStyle, weapIdx, hintIcon, fadeColor);\n'),
    ('CG_DrawWeaponPrompt(rect, textStyle, weapIdx, hintIcon, fadeColor);',
     'CG_DrawWeaponPrompt(rect, textStyle, weapIdx, cgs->media.hintMaterials[0], fadeColor);'),
]

variants = [(prompt, cursorhint)]
for old, new in mutants:
    assert prompt.count(old) == 1, old
    variants.append((prompt.replace(old, new), cursorhint))
for old, new in hook_mutants:
    assert cursorhint.count(old) == 1, old
    variants.append((prompt, cursorhint.replace(old, new)))

with tempfile.TemporaryDirectory(prefix='cod2-weapon-prompt-') as directory:
    path = Path(directory) / 'prompt.c'
    binary = Path(directory) / 'prompt'
    for index, (prompt_variant, cursorhint_variant) in enumerate(variants):
        path.write_text(support + slots + prompt_variant + cursorhint_variant + checks)
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wno-unused-function', str(path), '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
print('PASS: while cg_hudWeaponPrompt is on, the stock cursor hint hands weapons other than mounted guns to the WWII '
      'prompt, which chooses swap, pick up or icon only as the stock hint does over 756 loadouts; the use key in gold '
      'and "Press to swap for" / "Press to pick up" with the weapon\'s name share a baseline centred on the hint\'s '
      'rect, with the silhouette (or stock icon) centred below in its proportions over a shadow, all in the hint\'s '
      'fade, alignment and text style; mounted guns, other hints and the prompt switched off keep the stock hint; '
      f'{len(variants) - 1} failing regression mutants')
