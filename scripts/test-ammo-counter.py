#!/usr/bin/env python3
"""Run the production WWII ammo counter with the real offhand icon lookup: the
stock items it replaces or moves, the clip and reserve layout, low colors, the
RELOAD / LOW AMMO / NO AMMO hint in the WWII lettering and its blink, grenade
icons, fades, and the stock offhand draws' alpha."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_ammocounter_mp.c').read_text()
offhand = (root / 'src/PC/cgame_mp/offhandweapons_mp.c').read_text()
newdraw = (root / 'src/PC/cgame_mp/cg_newDraw_mp.c').read_text()
draw = (root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text()
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


def definition(text, head):
    start = text.index(head)
    return text[start:text.index('\n};\n', start) + 4]


# The stock ammo and offhand items give way while the counter is on; the
# weapon name and fire-mode icon are moved, and the unused offhand highlight
# is left alone.
for case, call in (('CG_PLAYER_AMMO_VALUE', 'CG_DrawPlayerAmmoValue(&rect, font, scale, color, material, textStyle, 0);'),
                   ('CG_PLAYER_AMMOCLIP_VALUE', 'CG_DrawPlayerAmmoValue(&rect, font, scale, color, material, textStyle, 1);'),
                   ('CG_PLAYER_AMMO_BACKDROP', 'CG_DrawPlayerAmmoBackdrop(&rect, material, color);'),
                   ('CG_OFFHAND_WEAPON_ICON_FRAG', 'CG_DrawOffHandIcon(&rect, scale, color, material, 1);'),
                   ('CG_OFFHAND_WEAPON_ICON_SMOKE', 'CG_DrawOffHandIcon(&rect, scale, color, material, 2);'),
                   ('CG_OFFHAND_WEAPON_AMMO_FRAG', 'CG_DrawOffHandAmmo(&rect, font, scale, color, textStyle, 1);'),
                   ('CG_OFFHAND_WEAPON_AMMO_SMOKE', 'CG_DrawOffHandAmmo(&rect, font, scale, color, textStyle, 2);'),
                   ('CG_OFFHAND_WEAPON_NAME_FRAG', 'CG_DrawOffHandName(&rect, font, scale, color, textStyle, 1);'),
                   ('CG_OFFHAND_WEAPON_NAME_SMOKE', 'CG_DrawOffHandName(&rect, font, scale, color, textStyle, 2);')):
    assert newdraw.count(f'    case {case}:\n        if (!CG_AmmoCounterActive())\n            {call}\n        break;\n') == 1, case
assert newdraw.count('    case CG_PLAYER_WEAPON_NAME:\n        if (CG_AmmoCounterActive())\n'
                     '            CG_AmmoCounterPlaceWeaponName(&rect, &scale);\n'
                     '        CG_DrawPlayerWeaponName(&rect, font, scale, color, textStyle);\n') == 1
assert newdraw.count('    case CG_PLAYER_WEAPON_MODE_ICON:\n        if (CG_AmmoCounterActive())\n'
                     '            CG_AmmoCounterPlaceModeIcon(&rect);\n'
                     '        CG_DrawPlayerWeaponModeIcon(&rect, material, color);\n') == 1
assert newdraw.count('    case CG_OFFHAND_WEAPON_SELECT_FRAG:\n        CG_DrawOffHandHighlight(') == 1
assert newdraw.count('int CG_GetHudWeaponIndex(void)\n{\n    return CG_GetDrawWeaponIndex();\n}\n') == 1
# Only a player's view draws it, under the score popup; every level starts with
# a fresh counter.
assert draw.count('            CG_DrawKillFeed();\n            CG_DrawAmmoCounter();\n            CG_DrawScorePopup();\n') == 1
assert draw.count('CG_DrawAmmoCounter();') == 1
assert main.count('    CG_ScorePanelInit();\n    CG_AmmoCounterInit();\n') == 1

counter = source[source.index('/* Virtual units from the bottom-right corner'):]
offhand_code = definition(offhand, 'const char *offhandStrings[3] = {') + '\n' + ''.join(
    function(offhand, name) for name in ('CG_DrawOffHandName', 'CG_DrawOffHandIcon', 'CG_DrawOffHandAmmo',
                                         'CG_GetOffHandIcon'))

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
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef const char *MaterialHandle;
struct Font_s { int bold; float scale; };
typedef struct Font_s *FontHandle;
typedef struct { struct { int enabled; float value; } current; } dvar_t;
typedef struct rectDef_s { float x, y, w, h; int horzAlign, vertAlign; } rectDef_t;
typedef struct rectDef_s rectDef_s;
typedef enum { WEAPTYPE_BULLET, WEAPTYPE_GRENADE, WEAPTYPE_PROJECTILE, WEAPTYPE_BINOCULARS } weapType_t;
typedef struct { weapType_t weapType; int offhandClass; } WeaponDef;
typedef struct { MaterialHandle hHudIcon; } weaponInfo_s;
struct MantleState { float yaw; int timer; int transIndex; int flags; };
typedef struct {
    int pm_type; int eFlags; int weapon; int weaponstate; int stats[6]; int ammoclip[128]; int weapons[4];
    struct MantleState mantleState;
} playerState_t;
static struct { int time; playerState_t predictedPlayerState; int equippedOffHand; int ammoFadeTime;
                int offhandFadeTime; } cgState;
static struct { struct { MaterialHandle whiteMaterial; } media; } cgsState;
#define cg (&cgState)
#define cgs (&cgsState)

static char *va(const char *fmt, ...) {
    static char buffers[4][64];
    static int next;
    char *buffer = buffers[next++ & 3];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffers[0]), fmt, args);
    va_end(args);
    return buffer;
}
static dvar_t counterDvar = { { 1, 0 } }, ammoFade = { { 0, 2.9996f } }, offhandFade = { { 0, 0 } };
static const dvar_t *hud_fade_ammodisplay = &ammoFade, *hud_fade_offhand = &offhandFade;
static const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags) {
    assert(!strcmp(name, "cg_hudAmmoCounter") && value == 1 && flags == 0x1001);
    return &counterDvar;
}
/* One font per enum and scale, so a width is always measured with the drawn
 * font; the bold font is registered by name, and the labels lend theirs while
 * its image exists. */
static struct Font_s fonts[16], boldFont = { 1, 0 }, nameFont = { 2, 0 };
static int fontEnums[16], fontCount, boldRegistered, nameFontReady = 1;
static struct Font_s *CG_GetPlayerNameFont(void) { return nameFontReady ? &nameFont : NULL; }
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
    return (int)(strlen(text) * (font->bold == 2 ? 25.0f : font->bold ? 30.0f : 20.0f) * scale);
}
typedef struct { int pic; char text[32]; MaterialHandle material; FontHandle font; float x, y, w, h, s0, t0, s1, t1, scale;
                 vec4_t color; int horz, vert, style; } Drawn;
static Drawn drawn[16];
static int drawCount;
static void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign,
                        int vertAlign, float scale, const vec_t *color, int style) {
    assert(drawCount < 16 && maxChars == 0x7fffffff && (font->bold || font->scale == scale));
    Drawn *d = &drawn[drawCount++];
    memset(d, 0, sizeof(*d));
    snprintf(d->text, sizeof(d->text), "%s", text);
    d->font = font; d->x = x; d->y = y; d->scale = scale; memcpy(d->color, color, sizeof(d->color));
    d->horz = horzAlign; d->vert = vertAlign; d->style = style;
}
static void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1,
                              float s2, float t2, const vec_t *color, MaterialHandle material) {
    assert(drawCount < 16);
    Drawn *d = &drawn[drawCount++];
    memset(d, 0, sizeof(*d));
    d->pic = 1; d->material = material; d->x = x; d->y = y; d->w = w; d->h = h;
    d->s0 = s1; d->t0 = t1; d->s1 = s2; d->t1 = t2; memcpy(d->color, color, sizeof(d->color));
    d->horz = horzAlign; d->vert = vertAlign;
}
static void UI_DrawHandlePic(float x, float y, float w, float h, int horzAlign, int vertAlign, const vec_t *color,
                             MaterialHandle material) {
    CL_DrawStretchPic(x, y, w, h, horzAlign, vertAlign, 0, 0, 1, 1, color, material);
    drawn[drawCount - 1].style = -1;
}
static float ammoAlpha = 1, offhandAlpha = 1;
static int ammoStart, ammoDuration, offhandStart, offhandDuration;
static float CG_FadeHudMenu(const dvar_t *fadeDvar, int displayStartTime, int duration) {
    if (fadeDvar == &ammoFade) { ammoStart = displayStartTime; ammoDuration = duration; return ammoAlpha; }
    assert(fadeDvar == &offhandFade);
    offhandStart = displayStartTime; offhandDuration = duration;
    return offhandAlpha;
}
static const char *UI_SafeTranslateString(const char *s) { return s; }
static int lowClip, lowAmmo, hudWeapon, reserve, clipOnly[9];
static Bool CG_CheckPlayerForLowClip(void) { return lowClip; }
static Bool CG_CheckPlayerForLowAmmo(void) { return lowAmmo; }
static int CG_GetHudWeaponIndex(void) { return hudWeapon; }
/* Weapon 3 is the gun, 4 a second gun; 5 and 7 frag grenades, 6 a smoke. */
static WeaponDef defs[9];
static int BG_GetNumWeapons(void) { return 8; }
static WeaponDef *BG_GetWeaponDef(int weapon) { assert(weapon >= 0 && weapon < 9); return &defs[weapon]; }
static int BG_ClipForWeapon(int weapon) { assert(weapon > 0 && weapon < 9); return weapon + 20; }
static int BG_GetTotalAmmoReserve(const playerState_t *ps, int weapon) {
    assert(ps == &cg->predictedPlayerState && weapon == hudWeapon);
    return reserve;
}
static qboolean BG_WeaponIsClipOnly(int weapon) { return clipOnly[weapon]; }
static int owns(const playerState_t *ps, int weapon) { return (ps->weapons[weapon >> 5] >> (weapon & 31)) & 1; }
static int BG_GetFirstAvailableOffhand(const playerState_t *ps, int offhandClass) {
    for (int i = 1; i <= 8; i++)
        if (owns(ps, i) && defs[i].offhandClass == offhandClass && ps->ammoclip[i + 20] > 0) return i;
    return 0;
}
static int BG_GetFirstEquippedOffhand(const playerState_t *ps, int offhandClass) {
    for (int i = 1; i <= 8; i++)
        if (owns(ps, i) && defs[i].offhandClass == offhandClass) return i;
    return 0;
}
static weaponInfo_s weaponInfos[9], *weaponInfoTable = weaponInfos;
static void *imp_cg_weapons = &weaponInfoTable;
/* The German frag grenade cannot be made a silhouette. */
static MaterialHandle R_GetHudSilhouetteMaterial(MaterialHandle icon, float *aspect) {
    if (!strcmp(icon, "hud_us_grenade")) { *aspect = 0.8f; return "sil:hud_us_grenade"; }
    if (!strcmp(icon, "hud_us_smokegrenade")) { *aspect = 1.25f; return "sil:hud_us_smokegrenade"; }
    *aspect = 0.0f;
    return NULL;
}
'''

checks = r'''
static int near(float a, float b) { return fabsf(a - b) < 1e-3f; }
static const float cream[3] = { 0.96f, 0.95f, 0.92f }, grey[3] = { 0.75f, 0.75f, 0.73f },
                   pink[3] = { 0.95f, 0.6f, 0.6f }, white[3] = { 1, 1, 1 };
static int colored(const float *color, const float *rgb, float alpha) {
    return near(color[0], rgb[0]) && near(color[1], rgb[1]) && near(color[2], rgb[2]) && near(color[3], alpha);
}
static int pic(int i, MaterialHandle material, float x, float y, float w, float h, const float *rgb, float alpha) {
    const Drawn *d = &drawn[i];
    return i < drawCount && d->pic && !strcmp(d->material, material) && near(d->x, x) && near(d->y, y) &&
           near(d->w, w) && near(d->h, h) && d->s0 == 0 && d->t0 == 0 && d->s1 == 1 && d->t1 == 1 &&
           d->horz == 3 && d->vert == 3 && colored(d->color, rgb, alpha);
}
static int text(int i, const char *value, float x, float y, float scale, const float *rgb, float alpha) {
    const Drawn *d = &drawn[i];
    return i < drawCount && !d->pic && !strcmp(d->text, value) && near(d->x, x) && near(d->y, y) &&
           near(d->scale, scale) && !d->font->bold && fontEnums[d->font - fonts] == 0 && d->horz == 3 &&
           d->vert == 3 && d->style == 3 && colored(d->color, rgb, alpha);
}
/* The WWII captures' red and halfway to white, half a level up. */
static const float hintRed[3] = { 218.5f / 255, 148.5f / 255, 150.5f / 255 },
                   hintLight[3] = { 236.5f / 255, 201.5f / 255, 202.5f / 255 };
/* The hint centred under the crosshair in the given lettering, blend of the
 * way from the red to the light pink. */
static int hintIn(int i, const char *value, FontHandle font, float blend, float alpha) {
    const Drawn *d = &drawn[i];
    float rgb[3];
    for (int k = 0; k < 3; k++) rgb[k] = hintRed[k] + (hintLight[k] - hintRed[k]) * blend;
    return i < drawCount && !d->pic && !strcmp(d->text, value) && d->font == font &&
           near(d->x, -0.5f * UI_TextWidth(value, 0, font, 0.25f)) && near(d->y, 126) && near(d->scale, 0.25f) &&
           d->horz == 2 && d->vert == 2 && d->style == 3 && colored(d->color, rgb, alpha);
}
static int blinked(int i, const char *value, float blend) { return hintIn(i, value, &nameFont, blend, 1); }
static int hint(int i, const char *value, float alpha) { return hintIn(i, value, &nameFont, 0, alpha); }
static int hints(void) {
    int n = 0;
    for (int i = 0; i < drawCount; i++) n += !drawn[i].pic && (drawn[i].font == &nameFont || drawn[i].font == &boldFont);
    return n;
}
/* The bytes R_ConvertColorToBytes makes, truncating. */
static int bytes(int i, int r, int g, int b) {
    return (int)(drawn[i].color[0] * 255) == r && (int)(drawn[i].color[1] * 255) == g &&
           (int)(drawn[i].color[2] * 255) == b;
}
static float width(const char *value, float scale) { return (float)(int)(strlen(value) * 20.0f * scale); }
static void render(void) { drawCount = 0; CG_DrawAmmoCounter(); }
static int at(int time) { cg->time = time; render(); return drawCount; }
static void give(int weapon, int ammo) {
    playerState_t *ps = &cg->predictedPlayerState;
    ps->weapons[weapon >> 5] |= 1 << (weapon & 31);
    ps->ammoclip[weapon + 20] = ammo;
}
/* A living player with weapon 3 out: 20 in the clip, 192 in reserve, a smoke
 * grenade and three frags. The unowned frag type keeps stale ammo. */
static void reset(void) {
    playerState_t *ps = &cg->predictedPlayerState;
    memset(ps, 0, sizeof(*ps));
    memset(clipOnly, 0, sizeof(clipOnly));
    ps->stats[0] = 100; ps->weapon = 3; hudWeapon = 3; reserve = 192; lowClip = lowAmmo = 0;
    ammoAlpha = offhandAlpha = 1; cg->equippedOffHand = 0;
    give(3, 20); give(5, 3); give(6, 1);
    ps->ammoclip[27] = 4;
}
static int view(CG_AmmoCounterView *v) { return CG_AmmoCounterGetView(&cg->predictedPlayerState, v); }
static int currentHint(void) {
    CG_AmmoCounterView v;
    assert(view(&v));
    return CG_AmmoCounterHint(&cg->predictedPlayerState, &v);
}
int main(void) {
    CG_AmmoCounterView v;
    rectDef_t rect;
    float scale;
    int count;

    cgs->media.whiteMaterial = "white";
    defs[3].weapType = WEAPTYPE_BULLET; defs[4].weapType = WEAPTYPE_PROJECTILE;
    defs[5].weapType = defs[6].weapType = defs[7].weapType = WEAPTYPE_GRENADE;
    defs[5].offhandClass = defs[7].offhandClass = 1; defs[6].offhandClass = 2;
    weaponInfos[5].hHudIcon = "hud_us_grenade"; weaponInfos[6].hHudIcon = "hud_us_smokegrenade";
    weaponInfos[7].hHudIcon = "hud_german_grenade";
    assert(!CG_AmmoCounterActive());
    CG_AmmoCounterInit();
    assert(CG_AmmoCounterActive() && boldRegistered == 1 && s_ammoCounterHintFont == &boldFont);

    /* The clip right-aligned against a thin rule, the reserve right of it,
     * then the smoke and the frags: silhouettes in proportion, a count past one. */
    reset();
    cg->ammoFadeTime = 1234; cg->offhandFadeTime = 5678;
    render();
    assert(drawCount == 6);
    assert(text(0, "20", -91.5f - 3 - width("20", 0.62f), -21, 0.62f, cream, 1));
    assert(pic(1, "white", -91.5f, -44, 0.75f, 20, grey, 0.8f));
    assert(text(2, "192", -91.5f + 0.75f + 2.5f, -34, 0.29f, grey, 1));
    assert(pic(3, "sil:hud_us_smokegrenade", -63.5f, -35, 10, 8, white, 1));
    assert(pic(4, "sil:hud_us_grenade", -51.5f, -36, 8, 10, white, 1));
    assert(text(5, "3", -43.5f, -25, 0.2f, cream, 1));
    /* Each part fades as its stock menu does. */
    assert(ammoStart == 1234 && ammoDuration == 3000 && offhandStart == 5678 && offhandDuration == 0);

    /* A low clip turns pink and asks for a reload. */
    lowClip = 1; cg->predictedPlayerState.ammoclip[23] = 8;
    render();
    assert(drawCount == 7 && text(0, "8", -91.5f - 3 - width("8", 0.62f), -21, 0.62f, pink, 1));
    assert(text(2, "192", -88.25f, -34, 0.29f, grey, 1) && hint(3, "RELOAD", 1));
    /* Not while any reload state is under way. */
    for (int state = 0; state <= 20; state++) {
        cg->predictedPlayerState.weaponstate = state;
        assert(currentHint() == (state >= 5 && state <= 9 ? AMMO_COUNTER_HINT_NONE : AMMO_COUNTER_HINT_RELOAD));
    }
    cg->predictedPlayerState.weaponstate = 0;
    /* Only a living player, and not over the stock mantle hint. */
    cg->predictedPlayerState.pm_type = 1; assert(currentHint() == AMMO_COUNTER_HINT_RELOAD);
    cg->predictedPlayerState.pm_type = 2; assert(currentHint() == AMMO_COUNTER_HINT_NONE);
    cg->predictedPlayerState.pm_type = 0; cg->predictedPlayerState.stats[0] = 0;
    assert(currentHint() == AMMO_COUNTER_HINT_NONE);
    cg->predictedPlayerState.stats[0] = 100; cg->predictedPlayerState.mantleState.flags = 8;
    assert(currentHint() == AMMO_COUNTER_HINT_NONE);
    cg->predictedPlayerState.mantleState.flags = 7; assert(currentHint() == AMMO_COUNTER_HINT_RELOAD);
    cg->predictedPlayerState.mantleState.flags = 0;
    /* Not on a mounted weapon, standing, crouched or prone. */
    cg->predictedPlayerState.eFlags = 0x100; assert(currentHint() == AMMO_COUNTER_HINT_NONE);
    cg->predictedPlayerState.eFlags = 0x200; assert(currentHint() == AMMO_COUNTER_HINT_NONE);
    cg->predictedPlayerState.eFlags = 0x300; assert(currentHint() == AMMO_COUNTER_HINT_NONE);
    cg->predictedPlayerState.eFlags = 0xcff; assert(currentHint() == AMMO_COUNTER_HINT_RELOAD);
    cg->predictedPlayerState.eFlags = 0;
    /* Guns and launchers only. */
    defs[3].weapType = WEAPTYPE_GRENADE; assert(currentHint() == AMMO_COUNTER_HINT_NONE);
    defs[3].weapType = WEAPTYPE_BINOCULARS; assert(currentHint() == AMMO_COUNTER_HINT_NONE);
    defs[3].weapType = WEAPTYPE_BULLET;
    give(4, 0); cg->predictedPlayerState.weapon = hudWeapon = 4; reserve = 2;
    assert(currentHint() == AMMO_COUNTER_HINT_RELOAD);
    cg->predictedPlayerState.weapon = hudWeapon = 3; reserve = 192;

    /* Nothing left to reload from: low ammo, then none at all. */
    reserve = 0; lowAmmo = 1;
    render();
    assert(text(0, "8", -91.5f - 3 - width("8", 0.62f), -21, 0.62f, pink, 1) && text(2, "0", -88.25f, -34, 0.29f, pink, 1));
    assert(hint(3, "LOW AMMO", 1));
    cg->predictedPlayerState.ammoclip[23] = 0;
    render();
    assert(hint(3, "NO AMMO", 1));
    /* A low reserve behind a full clip. */
    cg->predictedPlayerState.ammoclip[23] = 30; lowClip = 0; reserve = 20;
    render();
    assert(text(0, "30", -91.5f - 3 - width("30", 0.62f), -21, 0.62f, cream, 1) && text(2, "20", -88.25f, -34, 0.29f, pink, 1));
    assert(hint(3, "LOW AMMO", 1));
    lowAmmo = 0;
    render();
    assert(drawCount == 6 && !hints());

    /* A weapon without a clip shows its reserve as the big number, alone. */
    reset();
    clipOnly[3] = 1; reserve = 5;
    render();
    assert(drawCount == 4 && text(0, "5", -91.5f - 3 - width("5", 0.62f), -21, 0.62f, cream, 1) && drawn[1].pic);
    assert(currentHint() == AMMO_COUNTER_HINT_NONE);
    lowAmmo = 1;
    render();
    assert(text(0, "5", -94.5f - width("5", 0.62f), -21, 0.62f, pink, 1) && hint(1, "LOW AMMO", 1));
    /* There is no clip to reload. */
    lowClip = 1;
    assert(currentHint() == AMMO_COUNTER_HINT_LOW);
    reserve = 0;
    assert(currentHint() == AMMO_COUNTER_HINT_EMPTY);
    clipOnly[3] = 0; cg->predictedPlayerState.ammoclip[23] = -1; reserve = 7;
    assert(view(&v) && !v.hasClip && v.reserve == 7 && currentHint() == AMMO_COUNTER_HINT_LOW);

    /* The stock counter's limits. */
    reset();
    reserve = 1500; cg->predictedPlayerState.ammoclip[23] = 1200;
    assert(view(&v) && v.hasClip && v.clip == 999 && v.reserve == 999 && v.weapon == 3);
    reserve = -5; cg->predictedPlayerState.ammoclip[23] = 0;
    assert(view(&v) && v.reserve == 0 && v.clip == 0 && v.hasClip);
    lowClip = 1; lowAmmo = 1;
    assert(view(&v) && v.lowClip && v.lowAmmo);

    /* No weapon: only the grenades. */
    reset();
    cg->predictedPlayerState.weapon = 0;
    render();
    assert(drawCount == 3 && pic(0, "sil:hud_us_smokegrenade", -63.5f, -35, 10, 8, white, 1));
    assert(!view(&v));
    cg->predictedPlayerState.weapon = 3; hudWeapon = 0;
    render();
    assert(drawCount == 3 && !view(&v));

    /* Fades: the ammo and the hint with the ammo display, the grenades with the
     * offhand display; a faded part is not drawn. */
    reset();
    lowClip = 1; ammoAlpha = 0.5f; offhandAlpha = 0.25f;
    cg->predictedPlayerState.ammoclip[25] = 0;
    give(7, 0);
    render();
    assert(drawCount == 6 && text(0, "20", -91.5f - 3 - width("20", 0.62f), -21, 0.62f, pink, 0.5f));
    assert(pic(1, "white", -91.5f, -44, 0.75f, 20, grey, 0.4f) && text(2, "192", -88.25f, -34, 0.29f, grey, 0.5f));
    assert(hint(3, "RELOAD", 0.5f) && pic(4, "sil:hud_us_smokegrenade", -63.5f, -35, 10, 8, white, 0.25f));
    /* An empty slot stays faint, without a count. */
    assert(pic(5, "sil:hud_us_grenade", -51.5f, -36, 8, 10, white, 0.25f * 0.3f));
    ammoAlpha = 0;
    render();
    assert(drawCount == 2 && drawn[0].pic && drawn[1].pic);
    ammoAlpha = 1; offhandAlpha = 0;
    render();
    assert(drawCount == 4 && hint(3, "RELOAD", 1));

    /* The grenade the slot would throw: an equipped one of the class first;
     * the count adds up every grenade of the class. A stock icon stands in
     * for a missing silhouette. */
    reset();
    give(7, 2);
    cg->equippedOffHand = 7;
    render();
    assert(drawCount == 6 && pic(4, "hud_german_grenade", -52.5f, -36, 10, 10, white, 1) && text(5, "5", -43.5f, -25, 0.2f, cream, 1));
    assert(CG_GetOffHandIcon(1, &count) && count == 5);
    cg->equippedOffHand = 6;
    assert(!strcmp(CG_GetOffHandIcon(1, &count), "hud_us_grenade") && count == 5);
    assert(!strcmp(CG_GetOffHandIcon(2, &count), "hud_us_smokegrenade") && count == 1);
    /* An unowned class has no icon; neither has a dead player. */
    cg->predictedPlayerState.weapons[0] &= ~(1 << 6);
    count = 9;
    assert(!CG_GetOffHandIcon(2, &count) && count == 0);
    render();
    assert(drawCount == 5 && pic(3, "sil:hud_us_grenade", -51.5f, -36, 8, 10, white, 1));
    cg->predictedPlayerState.pm_type = 6;
    count = 9;
    assert(!CG_GetOffHandIcon(1, &count) && count == 0);
    render();
    assert(drawCount == 3);

    /* The weapon name ends above the frag grenade, smaller; the fire-mode icon
     * sits left of the clip, or nowhere without a counter. */
    reset();
    rect.x = -385; rect.y = -15; rect.w = 290; rect.h = 40; rect.horzAlign = 1; rect.vertAlign = 1; scale = 0.4f;
    CG_AmmoCounterPlaceWeaponName(&rect, &scale);
    assert(near(rect.x, -332) && near(rect.y, -52) && near(rect.w, 290) && near(rect.h, 40) && near(scale, 0.3f));
    assert(rect.horzAlign == 3 && rect.vertAlign == 3);
    rect.x = -115; rect.y = -35; rect.w = 20; rect.h = 20; rect.horzAlign = 1; rect.vertAlign = 1;
    CG_AmmoCounterPlaceModeIcon(&rect);
    assert(near(rect.x, -91.5f - 3 - width("20", 0.62f) - 3 - 12) && near(rect.y, -41) && near(rect.w, 12) && near(rect.h, 12));
    assert(rect.horzAlign == 3 && rect.vertAlign == 3);
    cg->predictedPlayerState.weapon = 0;
    CG_AmmoCounterPlaceModeIcon(&rect);
    assert(rect.w == 0 && rect.h == 0);

    /* Nothing while cg_hudAmmoCounter is off. */
    reset();
    counterDvar.current.enabled = 0;
    render();
    assert(drawCount == 0 && !CG_AmmoCounterActive());
    counterDvar.current.enabled = 1;

    /* Without the labels' lettering, the hint takes CoD2's bold font, and
     * failing that the UI's font 4. */
    nameFontReady = 0; lowClip = 1;
    render();
    assert(hintIn(3, "RELOAD", &boldFont, 0, 1));
    s_ammoCounterHintFont = NULL;
    render();
    assert(drawn[3].font != &boldFont && drawn[3].font != &nameFont && fontEnums[drawn[3].font - fonts] == 4);
    assert(hintIn(3, "RELOAD", drawn[3].font, 0, 1));
    nameFontReady = 1; s_ammoCounterHintFont = &boldFont;

    /* The hint blinks from its first frame: 300 ms red, 50 ms up to the light
     * pink, 100 ms there and 50 ms back, every 500 ms. */
    reset();
    assert(at(10000) == 6 && !hints());
    lowClip = 1; cg->predictedPlayerState.ammoclip[23] = 8;
    {
        static const int times[] = { 0, 299, 300, 325, 349, 350, 449, 450, 475, 499, 500, 799, 825, 850, 2350 };
        static const float blends[] = { 0, 0, 0, 0.5f, 0.98f, 1, 1, 1, 0.5f, 0.02f, 0, 0, 0.5f, 1, 1 };
        for (int k = 0; k < 15; k++)
            assert(at(10040 + times[k]) == 7 && blinked(3, "RELOAD", blends[k]));
    }
    assert(at(10040) == 7 && blinked(3, "RELOAD", 0) && bytes(3, 218, 148, 150));
    assert(at(10390) == 7 && blinked(3, "RELOAD", 1) && bytes(3, 236, 201, 202));
    /* A new hint starts red, as does one that comes back, also behind a
     * faded counter. */
    lowClip = 0; lowAmmo = 1; reserve = 20;
    assert(at(12400) == 7 && blinked(3, "LOW AMMO", 0));
    assert(at(12750) == 7 && blinked(3, "LOW AMMO", 1));
    lowAmmo = 0;
    assert(at(12760) == 6 && !hints());
    lowAmmo = 1;
    assert(at(12770) == 7 && blinked(3, "LOW AMMO", 0));
    ammoAlpha = 0;
    assert(at(12780) == 3 && !hints());
    ammoAlpha = 1;
    assert(at(13120) == 7 && blinked(3, "LOW AMMO", 0));
    assert(at(13470) == 7 && blinked(3, "LOW AMMO", 1));
    /* So does a server whose clock starts over, and a new level. */
    assert(at(1000) == 7 && blinked(3, "LOW AMMO", 0));
    assert(at(1325) == 7 && blinked(3, "LOW AMMO", 0.5f));
    CG_AmmoCounterInit();
    assert(at(1350) == 7 && blinked(3, "LOW AMMO", 0));
    assert(at(1700) == 7 && blinked(3, "LOW AMMO", 1));
    cg->time = 0;

    /* The stock offhand items take the offhand fade as their alpha. Each call
     * has a fade no earlier draw used, so an alpha left unset cannot pass with
     * a value an earlier call left on the stack. */
    reset();
    struct Font_s *font = UI_GetFontHandle(0, 0.3f);
    float color[4] = { 0.2f, 0.4f, 0.6f, 0.9f };
    const float itemRgb[3] = { 0.2f, 0.4f, 0.6f }, red[3] = { 0.89f, 0.18f, 0.01f };
    rect.x = 10; rect.y = 20; rect.w = 30; rect.h = 40; rect.horzAlign = 3; rect.vertAlign = 3;
    offhandAlpha = 0.3125f;
    drawCount = 0; CG_DrawOffHandName(&rect, font, 0.3f, color, 3, 1);
    assert(drawCount == 1 && !strcmp(drawn[0].text, "WEAPON_FRAGGRENADE") && colored(drawn[0].color, itemRgb, 0.3125f));
    offhandAlpha = 0.4375f;
    drawCount = 0; CG_DrawOffHandIcon(&rect, 1, color, "fallback", 1);
    assert(drawCount == 1 && !strcmp(drawn[0].material, "hud_us_grenade") && colored(drawn[0].color, itemRgb, 0.4375f));
    offhandAlpha = 0.5625f;
    drawCount = 0; CG_DrawOffHandAmmo(&rect, font, 0.3f, color, 3, 1);
    assert(drawCount == 1 && !strcmp(drawn[0].text, "3") && colored(drawn[0].color, itemRgb, 0.5625f));
    cg->predictedPlayerState.ammoclip[25] = 0;
    offhandAlpha = 0.6875f;
    drawCount = 0; CG_DrawOffHandAmmo(&rect, font, 0.3f, color, 3, 1);
    assert(drawCount == 1 && !strcmp(drawn[0].text, "0") && colored(drawn[0].color, red, 0.6875f));
    offhandAlpha = 0;
    drawCount = 0; CG_DrawOffHandIcon(&rect, 1, color, "fallback", 1); CG_DrawOffHandAmmo(&rect, font, 0.3f, color, 3, 1);
    CG_DrawOffHandName(&rect, font, 0.3f, color, 3, 1);
    assert(drawCount == 0);
    return 0;
}
'''

mutants = [
    ('    if (!ps->weapon)\n        return 0;\n', ''),
    ('    if (view->reserve < 0)\n        view->reserve = 0;\n', ''),
    ('    if (view->reserve > 999)\n        view->reserve = 999;\n', ''),
    ('if (!BG_WeaponIsClipOnly(view->weapon)) {', 'if (1) {'),
    ('if (clip >= 0) {', 'if (1) {'),
    ('view->clip = clip > 999 ? 999 : clip;', 'view->clip = clip;'),
    (' || ps->stats[0] <= 0', ''),
    ('(ps->mantleState.flags & AMMO_COUNTER_MANTLE_HINT)', '0'),
    (' || (ps->eFlags & AMMO_COUNTER_TURRET)', ''),
    ('#define AMMO_COUNTER_TURRET 0x300', '#define AMMO_COUNTER_TURRET 0x100'),
    ('ps->pm_type > AMMO_COUNTER_PM_LIVING', 'ps->pm_type > 5'),
    ('type != WEAPTYPE_BULLET && type != WEAPTYPE_PROJECTILE', 'type != WEAPTYPE_BULLET'),
    ('ps->weaponstate >= AMMO_COUNTER_RELOAD_FIRST', 'ps->weaponstate > AMMO_COUNTER_RELOAD_FIRST'),
    ('ps->weaponstate <= AMMO_COUNTER_RELOAD_LAST', 'ps->weaponstate < AMMO_COUNTER_RELOAD_LAST'),
    ('if (view->clip + view->reserve <= 0)', 'if (view->clip <= 0)'),
    ('if (view->hasClip && view->lowClip && view->reserve > 0)', 'if (view->hasClip && view->lowClip)'),
    ('if (view->hasClip && view->lowClip && view->reserve > 0)', 'if (view->lowClip && view->reserve > 0)'),
    ('(view->hasClip ? view->lowClip : view->lowAmmo)', 'view->lowClip'),
    ('    if (!view->hasClip)\n        return;\n', ''),
    ('view->lowAmmo ? s_ammoCounterLow : s_ammoCounterReserve', 's_ammoCounterReserve'),
    ('AMMO_COUNTER_RULE_X + AMMO_COUNTER_RULE_WIDTH + AMMO_COUNTER_RESERVE_GAP', 'AMMO_COUNTER_RULE_X + AMMO_COUNTER_RESERVE_GAP'),
    ('        material = icon;\n        aspect = 1.0f;\n', '        material = icon;\n'),
    ('    if (aspect > 1.0f)\n        h /= aspect;\n    else\n        w *= aspect;\n', '    w *= aspect;\n'),
    ('count > 0 ? alpha : alpha * AMMO_COUNTER_EMPTY_ALPHA', 'alpha'),
    ('if (count > 1) {', 'if (count > 0) {'),
    ('    color[3] = alpha;\n', '    color[3] = 1.0f;\n'),
    ('s_ammoCounterHintFont ? s_ammoCounterHintFont : UI_GetFontHandle(4, AMMO_COUNTER_HINT_SCALE)',
     'UI_GetFontHandle(4, AMMO_COUNTER_HINT_SCALE)'),
    ('    s_ammoCounterHintFont = CL_RegisterFont("$boldfont", 0);\n', ''),
    ('    FontHandle font = CG_GetPlayerNameFont();\n', '    FontHandle font = NULL;\n'),
    ('    if (!font)\n        font = s_ammoCounterHintFont', '    font = s_ammoCounterHintFont'),
    ('#define AMMO_COUNTER_HINT_Y 126.0f', '#define AMMO_COUNTER_HINT_Y 128.0f'),
    ('#define AMMO_COUNTER_BLINK_RED 300', '#define AMMO_COUNTER_BLINK_RED 250'),
    ('#define AMMO_COUNTER_BLINK_FADE 50', '#define AMMO_COUNTER_BLINK_FADE 100'),
    ('#define AMMO_COUNTER_BLINK_LIGHT 100', '#define AMMO_COUNTER_BLINK_LIGHT 150'),
    ('        return (float)t / AMMO_COUNTER_BLINK_FADE;\n', '        return 1.0f;\n'),
    ('    return 1.0f - (float)t / AMMO_COUNTER_BLINK_FADE;\n', '    return 0.0f;\n'),
    ('{ 218.5f / 255.0f, 148.5f / 255.0f, 150.5f / 255.0f }', '{ 0.95f, 0.6f, 0.6f }'),
    ('{ 218.5f / 255.0f, 148.5f / 255.0f, 150.5f / 255.0f }', '{ 218.0f / 255.0f, 148.0f / 255.0f, 150.0f / 255.0f }'),
    ('{ 236.5f / 255.0f, 201.5f / 255.0f, 202.5f / 255.0f }', '{ 1.0f, 1.0f, 1.0f }'),
    ('s_ammoCounterHintRed[i] + (s_ammoCounterHintLight[i] - s_ammoCounterHintRed[i]) * blink', 's_ammoCounterHintRed[i]'),
    ('hint != s_ammoCounterHintShown || ', ''),
    (' || s_ammoCounterHintStart > cg->time', ''),
    ('        s_ammoCounterHintStart = cg->time;\n', ''),
    ('    s_ammoCounterHintShown = AMMO_COUNTER_HINT_NONE;\n', ''),
    ('    int hint = AMMO_COUNTER_HINT_NONE;\n', '    int hint = AMMO_COUNTER_HINT_RELOAD;\n'),
    ('rect->x = AMMO_COUNTER_NAME_RIGHT - rect->w;', 'rect->x = AMMO_COUNTER_NAME_RIGHT;'),
    ('    rect->horzAlign = AMMO_COUNTER_ALIGN_RIGHT;\n    rect->vertAlign = AMMO_COUNTER_ALIGN_BOTTOM;\n    *scale', '    *scale'),
    ('        rect->w = 0.0f;\n        rect->h = 0.0f;\n', ''),
    ('- AMMO_COUNTER_MODE_GAP - AMMO_COUNTER_MODE_SIZE;', '- AMMO_COUNTER_MODE_GAP;'),
    ('return cg_hudAmmoCounter && cg_hudAmmoCounter->current.enabled;', 'return cg_hudAmmoCounter != 0;'),
    ('    if (!CG_AmmoCounterActive())\n        return;\n    alpha', '    alpha'),
    ('alpha = CG_AmmoCounterFade(hud_fade_offhand, cg->offhandFadeTime);',
     'alpha = CG_AmmoCounterFade(hud_fade_ammodisplay, cg->ammoFadeTime);'),
    ('if (alpha > 0.0f && CG_AmmoCounterGetView(ps, &view)) {', 'if (CG_AmmoCounterGetView(ps, &view)) {'),
    ('(int)floorf(fadeDvar->current.value * 1000.0f + 0.5f)', '(int)(fadeDvar->current.value * 1000.0f)'),
    ('    if (hint != AMMO_COUNTER_HINT_NONE)\n', ''),
]
offhand_mutants = [
    ('    if (ps->pm_type > 5)\n        return NULL;\n', ''),
    ('== weaponType)\n        weapon = cg->equippedOffHand;\n', '== weaponType)\n        ;\n'),
    ('        ((WeaponDef *)BG_GetWeaponDef(cg->equippedOffHand))->offhandClass == weaponType)',
     '        1)'),
    ('        if (!((ps->weapons[i >> 5] >> (i & 0x1f)) & 1))\n            continue;\n', ''),
    ('        if (((WeaponDef *)BG_GetWeaponDef(i))->offhandClass != weaponType)\n            continue;\n', ''),
    ('*count += ps->ammoclip[BG_ClipForWeapon(i)];', '*count = ps->ammoclip[BG_ClipForWeapon(i)];'),
    ('    drawColor[3] = fade;\n\n    text = UI_SafeTranslateString', '\n    text = UI_SafeTranslateString'),
    ('    drawColor[3] = fade;\n\n    if (cg->equippedOffHand != 0) {', '\n    if (cg->equippedOffHand != 0) {'),
    ('    }\n    drawColor[3] = fade;\n\n    UI_DrawText', '    }\n\n    UI_DrawText'),
]

variants = [(counter, offhand_code)]
for old, new in mutants:
    assert counter.count(old) == 1, old
    variants.append((counter.replace(old, new), offhand_code))
for old, new in offhand_mutants:
    assert offhand_code.count(old) == 1, old
    variants.append((counter, offhand_code.replace(old, new)))

with tempfile.TemporaryDirectory(prefix='cod2-ammo-counter-') as directory:
    path = Path(directory) / 'counter.c'
    binary = Path(directory) / 'counter'
    for index, (counter_variant, offhand_variant) in enumerate(variants):
        path.write_text(support + offhand_variant + counter_variant + checks)
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wno-unused-function', str(path), '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
print('PASS: while cg_hudAmmoCounter is on, the stock ammo and offhand items give way and the weapon name and '
      'fire-mode icon move clear; the clip sits right-aligned against a rule with the reserve beside it (a clipless '
      'weapon shows its reserve alone), pink when low; RELOAD, LOW AMMO and NO AMMO show under the crosshair in the '
      'labels\' WWII lettering (else the bold font, else font 4) for a living player\'s guns and launchers outside '
      'reloads, mounted weapons and the mantle hint, blinking from the WWII red to halfway to white and back every '
      '500 ms from each new hint, a hint that comes back, a clock reset or a new level; the frag and smoke '
      'grenades the slots would throw show as proportioned silhouettes (or stock icons) with their counts, faint when '
      'empty; each part keeps its stock fade, and the stock offhand items take the offhand fade as their alpha; '
      f'{len(variants) - 1} failing regression mutants')
