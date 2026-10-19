#!/usr/bin/env python3
"""Run the production WWII hold-breath prompt: when it shows, its key and
layout against the WWII capture in the labels' real lettering, and the stock
code it relies on to name the key that holds the breath."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_breathprompt_mp.c').read_text()
draw = (root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text()
main = (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()
weapons = (root / 'src/PC/bgame/bg_weapons.c').read_text()
misc = (root / 'src/PC/bgame/bg_misc.c').read_text()
cl_input = (root / 'src/PC/client_mp/cl_input.c').read_text()
ui_shared = (root / 'src/PC/ui_mp/ui_shared_mp.c').read_text()
font = (root / 'src/headers/PC/cgame_mp/cg_namefont_data.h').read_text()


def function(text, name):
    match = re.search(r'^((?:static )?[^\n;]*\b' + name + r'\([^;]*?\))\s*\{', text, re.M)
    assert match, name
    brace = text.index('{', match.end() - 1)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


# The prompt is drawn for living players with the HUD up, after the crosshair
# and the scope's overlay it draws.
assert draw.count('#include "headers/PC/cgame_mp/cg_breathprompt.h"\n') == 1
assert draw.count('CG_DrawBreathPrompt();') == 1
draw2d = function(draw, 'CG_Draw2D')
assert re.search(r'if \(pm_type <= 1\) \{\n(?:                [^\n]*\n)*?                CG_DrawGrenadeIndicators\(\);\n'
                 r'                CG_DrawBreathPrompt\(\);\n            \}', draw2d)
assert draw2d.index('CG_DrawCrosshair();') < draw2d.index('CG_DrawBreathPrompt();')
assert draw2d.index('drawHudMenus = !CG_AreHudMenusHidden();') < draw2d.index('CG_DrawBreathPrompt();')
assert function(draw, 'CG_DrawWeapReticle').count('material = weaponInfo->hADSOverlay;') == 1
# Its switch is the stock breath hint's, on by default, which stock
# multiplayer's hud.menu never draws.
assert main.count('cg_drawBreathHint = Dvar_RegisterBool_mac("cg_drawBreathHint", 1, 0x1001);') == 1
# It shows where PM_UpdateHoldBreath would start a hold: the weapon fully
# aimed down a scope that is not class 9, the timer recovered, a hold time.
hold = function(weapons, 'PM_UpdateHoldBreath')
assert 'if (holdTime <= 0) {' in hold
assert ('if (ps->fWeaponPosFrac == 1.0f && weapDef->overlayReticle &&\n'
        '        weapDef->weapClass != 9 && (pm->cmd.buttons & PM_WEAPON_BUTTON_HOLDBREATH)) {\n'
        '        if (!ps->holdBreathTimer)\n            ps->pm_flags |= PM_WEAPON_FLAG_HOLDBREATH;') in hold
assert weapons.count('#define PM_WEAPON_FLAG_HOLDBREATH 0x00004000\n') == 1
assert 'player_breath_hold_time = Dvar_RegisterFloat("player_breath_hold_time", 4.5f' in misc
# Sprinting's key presses the breath button, and Shift is its default key.
assert function(cl_input, 'IN_Sprint_Down') == 'void IN_Sprint_Down(void)\n{\n    IN_KeyDown(&sprintButton);\n    IN_Breath_Down();\n}\n'
assert ui_shared.count('{ "+sprint", 0xa0, 0xffffffff,') == 1
assert re.search(r'int GetKeyBindings\(const char \*command, char \(\*bindings\)\[128\]\)', ui_shared)

# The labels' lettering: advances, and where each glyph's ink lies in its
# bitmap, from the committed atlas.
glyphs = {}
for m in re.finditer(r'\{ 0x([0-9a-f]+), (-?\d+), (-?\d+), (\d+), (\d+), (\d+), (\d+), (\d+) \}', font):
    glyphs[int(m[1], 16)] = [int(v) for v in m.groups()[1:]]
atlas = re.findall(r'^\s*"([0-9a-z]+)",?$', font[font.index('cg_nameFontAtlas'):], re.M)
pixel_height = int(re.search(r'#define CG_NAME_FONT_PIXEL_HEIGHT (\d+)', font)[1])
units_per_texel = int(re.search(r'#define CG_NAME_FONT_UNITS_PER_TEXEL (\d+)', font)[1])
assert pixel_height == 52 and units_per_texel == 2


def ink(letter):
    x0, y0, dx, w, h, ax, ay = glyphs[ord(letter)]
    solid = [[int(atlas[ay + y][ax + x], 36) * 17 >= 128 for x in range(w)] for y in range(h)]
    cols = [x for x in range(w) if any(row[x] for row in solid)]
    rows = [y for y in range(h) if any(solid[y])]
    return (x0 + cols[0] * units_per_texel, y0 + rows[0] * units_per_texel, y0 + (rows[-1] + 1) * units_per_texel)


def define(name):
    return float(re.search(r'#define BREATH_PROMPT_' + name + r' (-?[\d.]+)f?\b', source)[1])


unit = lambda scale: 48.0 * scale / pixel_height
scale, key_scale = define('SCALE'), define('KEY_SCALE')
h_left, h_top, h_bottom = ink('H')
assert h_bottom == 0
# On the capture, at 1.2 px a unit from its reticle: the label's capitals 5.9
# to 6.0 units tall from 260.1 right of the centre on a baseline 61.2 above
# it, and the glyph 10.8 to 11.2 units tall, centred 63.0 above the centre,
# ending at 231.6, 28.5 before the label.
label_ink = define('TEXT_X') + h_left * unit(scale)
assert abs(label_ink - 260.1) < 0.2, label_ink
assert 5.88 <= -h_top * unit(scale) <= 6.06
assert define('BASELINE') == -61.2
assert abs(define('KEY_TOP') + 0.5 * define('KEY_HEIGHT') + 63.0) < 0.05 and 10.8 <= define('KEY_HEIGHT') <= 11.2
assert define('KEY_RIGHT') == 231.6 and abs(label_ink - define('KEY_RIGHT') - 28.5) < 0.2
# The key's capitals, 5 units tall, sit centred on the keycap.
key_caps = -h_top * unit(key_scale)
assert abs(key_caps - 5.0) < 0.05
assert abs(define('KEY_BASELINE') - 0.5 * key_caps - 0.5 * define('KEY_HEIGHT')) < 0.05

advances = ', '.join(str(glyphs[c][2] if c in glyphs else 0) for c in range(128))
# "SHIFT" at the key's scale, 0.15, and 3 units each side: its keycap.
shift_keycap = sum(glyphs[ord(c)][2] for c in 'SHIFT') * 48 * 0.15 / 52 + 6
assert 24 < shift_keycap < 26, shift_keycap
prompt = source[source.index('/* Virtual units from the screen'):]

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef float vec_t;
typedef vec_t vec4_t[4];
typedef const char *MaterialHandle;
struct Font_s { int pixelHeight; };
typedef struct Font_s *FontHandle;
typedef struct { struct { int enabled; float value; } current; } dvar_t;
typedef struct { int overlayReticle, weapClass; } WeaponDef;
typedef struct { int pm_type, pm_flags, eFlags, stats[1], weapon, holdBreathTimer; float fWeaponPosFrac; } playerState_t;
static struct { int renderingThirdPerson, inKillCam; playerState_t predictedPlayerState; } cgState;
static struct { struct { MaterialHandle whiteMaterial; } media; } cgsState = { { "white" } };
#define cg (&cgState)
#define cgs (&cgsState)
static dvar_t drawBreathHint, holdTime;
static const dvar_t *cg_drawBreathHint = &drawBreathHint, *player_breath_hold_time = &holdTime;
/* The labels' Barlow Medium: its advances, 52 font units to the em. */
static const int advance[128] = { ''' + advances + r''' };
static struct Font_s nameFont = { 52 };
static int fontReady;
static struct Font_s *CG_GetPlayerNameFont(void) { return fontReady ? &nameFont : NULL; }
static int CL_TextWidth(const char *text, int maxChars, FontHandle font) {
    int width = 0;
    assert(maxChars == 0 && font == &nameFont);
    for (; *text; text++) { assert((unsigned char)*text < 128 && advance[(unsigned char)*text]); width += advance[(unsigned char)*text]; }
    return width;
}
static float CL_NormalizedTextScale(FontHandle font, float scale) {
    assert(font == &nameFont);
    return 48.0f * scale / (float)font->pixelHeight;
}
/* No weapon, a scoped rifle, an open-sighted one and a scoped class 9. */
static WeaponDef weaponDefs[4] = { { 0, 0 }, { 1, 3 }, { 0, 3 }, { 1, 9 } };
static WeaponDef *BG_GetWeaponDef(int weapon) { assert(weapon >= 0 && weapon < 4); return &weaponDefs[weapon]; }
static WeaponDef *BG_GetPlayerWeaponDef(const playerState_t *ps, int weapon) { return BG_GetWeaponDef(weapon); }
static int BG_GetViewmodelWeaponIndex(const playerState_t *ps) { return ps->weapon; }
static const char *UI_SafeTranslateString(const char *ref) { assert(!strcmp(ref, "MENU_HOLD_BREATH")); return "Hold Breath"; }
static void I_strncpyz(char *dest, const char *src, int size) { strncpy(dest, src, size - 1); dest[size - 1] = 0; }
/* The controls' table: each command's keys as Key_KeynumToString names
 * them, translated or not by the string tables. */
static struct { const char *command; const char *keys[2]; } binds[4] = {
    { "+holdbreath" }, { "+sprint" }, { "+melee_breath" }, { "+breath_binoculars" } };
static int configs, lookups;
static void Controls_GetConfig(void) { configs++; }
static int GetKeyBindings(const char *command, char (*bindings)[128]) {
    int i, n = 0;
    lookups++;
    for (i = 0; i < 4 && strcmp(binds[i].command, command); i++)
        ;
    assert(i < 4);
    while (n < 2 && binds[i].keys[n]) { snprintf(bindings[n], 128, "%s", binds[i].keys[n]); n++; }
    if (!n) strcpy(bindings[0], "KEY_UNBOUND");
    return n;
}
static const char *SEH_StringEd_GetString(const char *ref) {
    if (!strcmp(ref, "KEY_SHIFT")) return "Shift";
    if (!strcmp(ref, "KEY_MOUSE2")) return "Right Mouse";
    return NULL;
}
enum { TEXT, STRETCH };
typedef struct { int kind; char text[64]; FontHandle font; float x, y, w, h, scale; vec4_t color; int horz, vert, style; } Drawn;
static Drawn drawn[8];
static int drawCount;
static Drawn *record(int kind, float x, float y, int horzAlign, int vertAlign, const vec_t *color) {
    Drawn *d;
    assert(drawCount < 8);
    d = &drawn[drawCount++];
    memset(d, 0, sizeof(*d));
    d->kind = kind; d->x = x; d->y = y; d->horz = horzAlign; d->vert = vertAlign;
    memcpy(d->color, color, sizeof(d->color));
    return d;
}
static void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign,
                        float scale, const vec_t *color, int style) {
    Drawn *d;
    assert(maxChars == 0x7fffffff);
    d = record(TEXT, x, y, horzAlign, vertAlign, color);
    snprintf(d->text, sizeof(d->text), "%s", text);
    d->font = font; d->scale = scale; d->style = style;
}
static void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1,
                              float s2, float t2, const vec_t *color, MaterialHandle material) {
    Drawn *d;
    assert(s1 == 0 && t1 == 0 && s2 == 1 && t2 == 1 && !strcmp(material, "white"));
    d = record(STRETCH, x, y, horzAlign, vertAlign, color);
    d->w = w; d->h = h;
}
'''

checks = r'''
static int near(float a, float b) { return fabsf(a - b) < 1e-3f; }
static const float words[3] = { 0.96f, 0.95f, 0.92f }, cream[3] = { 0.96f, 0.94f, 0.86f }, black[3] = { 0, 0, 0 };
static int colored(const float *color, const float *rgb, float alpha) {
    return near(color[0], rgb[0]) && near(color[1], rgb[1]) && near(color[2], rgb[2]) && near(color[3], alpha);
}
static float advanceOf(const char *text, float scale) {
    float width = 0;
    for (; *text; text++) width += advance[(unsigned char)*text];
    return width * 48.0f * scale / 52.0f;
}
/* The keycap's shadow, one unit down and right at 60%, the cream keycap, 11
 * units tall from 68.5 above the centre and ending 231.6 right of it, as wide
 * as the name and 3 units each side but at least square; the name in black,
 * centred on it with a baseline 8 under its top at scale 0.15; then the label
 * from 259.4 on a baseline 61.2 above the centre, at scale 0.18 with the
 * one-pixel shadow. All centred on the screen, at the prompt's alpha. */
static int prompt(const char *key, float alpha) {
    float name = advanceOf(key, 0.15f), width = name + 6.0f, left;
    const Drawn *d = drawn;
    if (width < 11.0f) width = 11.0f;
    left = 231.6f - width;
    for (int i = 0; i < drawCount; i++)
        if (d[i].horz != 2 || d[i].vert != 2) return 0;
    return drawCount == 4 &&
           d[0].kind == STRETCH && near(d[0].x, left + 1) && near(d[0].y, -67.5f) && near(d[0].w, width) &&
           near(d[0].h, 11) && colored(d[0].color, black, 0.6f * alpha) &&
           d[1].kind == STRETCH && near(d[1].x, left) && near(d[1].y, -68.5f) && near(d[1].w, width) &&
           near(d[1].h, 11) && colored(d[1].color, cream, alpha) &&
           d[2].kind == TEXT && !strcmp(d[2].text, key) && d[2].font == &nameFont &&
           near(d[2].x, left + 0.5f * (width - name)) && near(d[2].y, -60.5f) && near(d[2].scale, 0.15f) &&
           d[2].style == 0 && colored(d[2].color, black, alpha) &&
           d[3].kind == TEXT && !strcmp(d[3].text, "Hold Breath") && d[3].font == &nameFont && near(d[3].x, 259.4f) &&
           near(d[3].y, -61.2f) && near(d[3].scale, 0.18f) && d[3].style == 3 && colored(d[3].color, words, alpha);
}
static void draw(void) { drawCount = 0; CG_DrawBreathPrompt(); }
/* Alive and aiming a scoped rifle all the way, with Shift for sprinting and
 * the left stick after it. */
static void aimed(void) {
    memset(&cgState, 0, sizeof(cgState));
    cgState.predictedPlayerState.stats[0] = 100;
    cgState.predictedPlayerState.weapon = 1;
    cgState.predictedPlayerState.fWeaponPosFrac = 1.0f;
    drawBreathHint.current.enabled = 1;
    holdTime.current.value = 4.5f;
    fontReady = 1;
    for (int i = 0; i < 4; i++) binds[i].keys[0] = binds[i].keys[1] = NULL;
    binds[1].keys[0] = "KEY_SHIFT";
    binds[1].keys[1] = "LS / L3";
    configs = lookups = 0;
}
int main(void) {
    int cases = 0;

    aimed();
    draw();
    assert(prompt("SHIFT", 1) && configs == 1);
    /* "SHIFT" in capitals 5 units tall needs a keycap ''' + f'{shift_keycap:.2f}' + r''' units wide. */
    assert(fabsf(drawn[1].w - ''' + f'{shift_keycap:.4f}' + r'''f) < 0.01f);
    cases++;

    /* While the breath recovers the key cannot start a hold, so the prompt
     * dims; while it is held, the prompt is gone. */
    aimed();
    cgState.predictedPlayerState.holdBreathTimer = 1;
    draw();
    assert(prompt("SHIFT", 0.3f));
    cgState.predictedPlayerState.holdBreathTimer = 5500;
    draw();
    assert(prompt("SHIFT", 0.3f));
    cgState.predictedPlayerState.pm_flags = 0x4000;
    draw();
    assert(drawCount == 0);
    cgState.predictedPlayerState.holdBreathTimer = 0;
    draw();
    assert(drawCount == 0);
    cases += 4;

    /* Where no hold can start, or over a replay of the killer's aim, nothing
     * is drawn and no key is looked up. */
    for (int gate = 0; gate < 18; gate++) {
        aimed();
        playerState_t *ps = &cgState.predictedPlayerState;
        switch (gate) {
        case 0: drawBreathHint.current.enabled = 0; break;
        case 1: cgState.renderingThirdPerson = 1; break;
        case 2: ps->pm_type = 2; break;
        case 3: ps->pm_type = 6; break;
        case 4: ps->stats[0] = 0; break;
        case 5: ps->stats[0] = -20; break;
        case 6: ps->eFlags = 0x100; break;
        case 7: ps->eFlags = 0x200; break;
        case 8: ps->fWeaponPosFrac = 0.99f; break;
        case 9: ps->fWeaponPosFrac = 0.0f; break;
        case 10: holdTime.current.value = 0.0f; break;
        case 11: holdTime.current.value = 0.0009f; break;
        case 12: ps->weapon = 0; break;
        case 13: ps->weapon = 2; break;
        case 14: ps->weapon = 3; break;
        case 15: fontReady = 0; break;
        case 16: ps->pm_flags = 0x4000 | 0x8000; break;
        case 17: cgState.inKillCam = 1; break;
        }
        draw();
        assert(drawCount == 0 && configs == 0 && lookups == 0);
        cases++;
    }
    /* Linked to an entity, with other flags set, or with a one-millisecond
     * hold, the key still holds the breath. */
    aimed();
    cgState.predictedPlayerState.pm_type = 1;
    cgState.predictedPlayerState.eFlags = 0x8 | 0x400;
    cgState.predictedPlayerState.pm_flags = 0x8000 | 0x2000;
    holdTime.current.value = 0.001f;
    draw();
    assert(prompt("SHIFT", 1));
    cases++;

    /* The dedicated command's key comes first, then sprinting's, then the
     * stock hint's commands in its order; one key is shown, in capitals; a
     * long name grows the keycap to the left. With none bound, nothing. */
    aimed();
    binds[0].keys[0] = "KEY_MOUSE2";
    draw();
    assert(prompt("RIGHT MOUSE", 1) && lookups == 1);
    assert(near(drawn[1].x + drawn[1].w, 231.6f) && drawn[1].w > 40.0f);
    aimed();
    binds[1].keys[0] = "LS / L3"; binds[1].keys[1] = NULL;
    draw();
    assert(prompt("LS / L3", 1));
    aimed();
    binds[1].keys[0] = NULL; binds[1].keys[1] = NULL;
    binds[2].keys[0] = "V";
    binds[3].keys[0] = "B";
    draw();
    assert(prompt("V", 1) && lookups == 3);
    /* A single letter leaves the keycap square, the letter centred on it. */
    assert(near(drawn[1].w, 11) && near(drawn[2].x + 0.5f * advanceOf("V", 0.15f), 231.6f - 5.5f));
    binds[2].keys[0] = NULL;
    draw();
    assert(prompt("B", 1));
    binds[3].keys[0] = "mwheelup";
    draw();
    assert(prompt("MWHEELUP", 1));
    binds[3].keys[0] = NULL;
    configs = lookups = 0;
    draw();
    assert(drawCount == 0 && configs == 1 && lookups == 4);
    cases += 7;

    printf("%d\n", cases);
    return 0;
}
'''

mutants = [
    [('#define BREATH_PROMPT_TEXT_X 259.4f', '#define BREATH_PROMPT_TEXT_X 262.0f')],
    [('#define BREATH_PROMPT_BASELINE -61.2f', '#define BREATH_PROMPT_BASELINE -58.0f')],
    [('#define BREATH_PROMPT_SCALE 0.18f', '#define BREATH_PROMPT_SCALE 0.21f')],
    [('#define BREATH_PROMPT_STYLE 3', '#define BREATH_PROMPT_STYLE 6')],
    [('#define BREATH_PROMPT_KEY_RIGHT 231.6f', '#define BREATH_PROMPT_KEY_RIGHT 224.4f')],
    [('#define BREATH_PROMPT_KEY_TOP -68.5f', '#define BREATH_PROMPT_KEY_TOP -63.0f')],
    [('#define BREATH_PROMPT_KEY_HEIGHT 11.0f', '#define BREATH_PROMPT_KEY_HEIGHT 13.0f')],
    [('#define BREATH_PROMPT_KEY_SCALE 0.15f', '#define BREATH_PROMPT_KEY_SCALE 0.18f')],
    [('#define BREATH_PROMPT_KEY_BASELINE 8.0f', '#define BREATH_PROMPT_KEY_BASELINE 5.5f')],
    [('#define BREATH_PROMPT_KEY_PAD 3.0f', '#define BREATH_PROMPT_KEY_PAD 2.0f')],
    [('#define BREATH_PROMPT_SHADOW_OFFSET 1.0f', '#define BREATH_PROMPT_SHADOW_OFFSET 0.0f')],
    [('#define BREATH_PROMPT_SHADOW_ALPHA 0.6f', '#define BREATH_PROMPT_SHADOW_ALPHA 1.0f')],
    [('#define BREATH_PROMPT_RECOVERING_ALPHA 0.3f', '#define BREATH_PROMPT_RECOVERING_ALPHA 1.0f')],
    [('#define BREATH_PROMPT_HOLDING 0x4000', '#define BREATH_PROMPT_HOLDING 0x8000')],
    [('#define BREATH_PROMPT_ALIGN 2', '#define BREATH_PROMPT_ALIGN 7')],
    [('s_breathPromptText = { 0.96f, 0.95f, 0.92f, 1.0f }', 's_breathPromptText = { 1.0f, 1.0f, 1.0f, 1.0f }')],
    [('s_breathPromptKeycap = { 0.96f, 0.94f, 0.86f, 1.0f }', 's_breathPromptKeycap = { 0.96f, 0.95f, 0.92f, 1.0f }')],
    [('s_breathPromptKeyName = { 0.0f, 0.0f, 0.0f, 1.0f }', 's_breathPromptKeyName = { 1.0f, 1.0f, 1.0f, 1.0f }')],
    [('{ "+holdbreath", "+sprint", "+melee_breath",', '{ "+sprint", "+holdbreath", "+melee_breath",')],
    [('{ "+holdbreath", "+sprint", "+melee_breath",', '{ "+holdbreath", "+melee_breath",')],
    [('"+melee_breath",\n' + ' ' * 54 + '"+breath_binoculars" }', '"+melee_breath" }')],
    [('            if (key[c] >= \'a\' && key[c] <= \'z\')\n                key[c] = (char)(key[c] - \'a\' + \'A\');\n', '')],
    [('I_strncpyz(key, name ? name : bindings[0], BREATH_PROMPT_KEY_LEN);', 'I_strncpyz(key, bindings[0], BREATH_PROMPT_KEY_LEN);')],
    [('        if (!GetKeyBindings(s_breathPromptCommands[i], bindings))\n            continue;\n',
      '        GetKeyBindings(s_breathPromptCommands[i], bindings);\n')],
    [('    Controls_GetConfig();\n', '')],
    [('    if (cg->inKillCam)\n        return 0.0f;\n', '')],
    [('    if (!cg_drawBreathHint->current.enabled || cg->renderingThirdPerson ||', '    if (cg->renderingThirdPerson ||')],
    [('cg->renderingThirdPerson || ps->pm_type > 1 ||', 'ps->pm_type > 1 ||')],
    [('ps->pm_type > 1 ||', 'ps->pm_type > 5 ||')],
    [('ps->stats[0] <= 0 ||', 'ps->stats[0] < 0 ||')],
    [('        (ps->eFlags & 0x300) || ', '        ')],
    [('(ps->pm_flags & BREATH_PROMPT_HOLDING) || ', '')],
    [('ps->fWeaponPosFrac != 1.0f ||', 'ps->fWeaponPosFrac < 0.5f ||')],
    [('(int)(player_breath_hold_time->current.value * 1000.0f) <= 0', 'player_breath_hold_time->current.value <= 0.0f')],
    [('if (!weapDef->overlayReticle || weapDef->weapClass == 9)', 'if (weapDef->weapClass == 9)')],
    [('if (!weapDef->overlayReticle || weapDef->weapClass == 9)', 'if (!weapDef->overlayReticle)')],
    [('return ps->holdBreathTimer > 0 ? BREATH_PROMPT_RECOVERING_ALPHA : 1.0f;',
      'return ps->holdBreathTimer > 5000 ? BREATH_PROMPT_RECOVERING_ALPHA : 1.0f;')],
    [('    if (alpha <= 0.0f)\n        return;\n', '')],
    [('    if (!font || !CG_BreathPromptKey(key))', '    if (!CG_BreathPromptKey(key) || !font)')],
    [('    if (width < BREATH_PROMPT_KEY_HEIGHT)\n        width = BREATH_PROMPT_KEY_HEIGHT;\n', '')],
    [('    left = BREATH_PROMPT_KEY_RIGHT - width;', '    left = BREATH_PROMPT_KEY_RIGHT - BREATH_PROMPT_KEY_HEIGHT;')],
    [('UI_DrawText(key, 0x7fffffff, font, left + 0.5f * (width - nameWidth),', 'UI_DrawText(key, 0x7fffffff, font, left + BREATH_PROMPT_KEY_PAD,')],
    [('    CL_DrawStretchPic(left + BREATH_PROMPT_SHADOW_OFFSET', '    if (0) CL_DrawStretchPic(left + BREATH_PROMPT_SHADOW_OFFSET')],
    [('CG_BreathPromptColor(color, s_breathPromptKeyName, alpha);', 'CG_BreathPromptColor(color, s_breathPromptKeyName, 1.0f);')],
    [('    out[3] = in[3] * alpha;', '    out[3] = alpha;')],
    [('BREATH_PROMPT_BASELINE, BREATH_PROMPT_ALIGN, BREATH_PROMPT_ALIGN, BREATH_PROMPT_SCALE, color,\n                BREATH_PROMPT_STYLE);',
      'BREATH_PROMPT_BASELINE, BREATH_PROMPT_ALIGN, BREATH_PROMPT_ALIGN, BREATH_PROMPT_KEY_SCALE, color,\n                BREATH_PROMPT_STYLE);')],
    [('BREATH_PROMPT_KEY_SCALE, color, 0);', 'BREATH_PROMPT_KEY_SCALE, color, BREATH_PROMPT_STYLE);')],
]

variants = [prompt]
for pairs in mutants:
    variant = prompt
    for old, new in pairs:
        assert variant.count(old) == 1, old
        variant = variant.replace(old, new)
    variants.append(variant)

with tempfile.TemporaryDirectory(prefix='cod2-breath-prompt-') as directory:
    path = Path(directory) / 'prompt.c'
    binary = Path(directory) / 'prompt'
    cases = 0
    for index, variant in enumerate(variants):
        path.write_text(support + variant + checks)
        warnings = ['-Wall', '-Werror', '-Wno-unused-function'] if index == 0 else ['-w']
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        *warnings, str(path), '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, mutants[index - 1] if index else None,
                                                         result.stderr[-2000:])
        if index == 0:
            cases = int(result.stdout)
print('PASS: fully aimed down a scope that can hold the breath, the WWII prompt shows the key of +holdbreath, else of '
      '+sprint (Shift, which holds the breath), else of the stock hint\'s commands, one key in capitals on a cream '
      'keycap that grows to the left, then "Hold Breath" where the capture puts its label, in the labels\' lettering; '
      f'it dims while the breath recovers and is gone while it is held or a killcam replays; {cases} cases; '
      f'{len(variants) - 1} failing regression mutants')
