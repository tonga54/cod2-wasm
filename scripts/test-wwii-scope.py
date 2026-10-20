#!/usr/bin/env python3
"""The WWII sniper scope (cg_wwiiscope_mp.c) and its lens images (r_hud2d.c).

Checks the hooks that wire the scope into the view weapon, the reticle, the
zoom and the cgame's start-up; runs its glass projection, rifle projection,
zoom, centring, bolt, blackout and reticle math, the glass a player's weapon
is drawn with, a 4x Optic's read from its glass file, and the lens image
decoder, under ASan and UBSan, each with mutants that must fail; and, when the retail
IWDs are in data/main, checks the glass table and the rifle's near clip
against the sniper viewmodels and the lens edges against the scope overlays.
"""
from pathlib import Path
import math
import re
import struct
import subprocess
import tempfile
import zipfile

root = Path(__file__).resolve().parent.parent
scope = (root / 'src/PC/cgame_mp/cg_wwiiscope_mp.c').read_text()
header = (root / 'src/headers/PC/cgame_mp/cg_wwiiscope.h').read_text()
hud2d = (root / 'src/PC/gfx_d3d/r_hud2d.c').read_text()
hud2d_header = (root / 'src/headers/PC/gfx_d3d/r_hud2d.h').read_text()
weapons = (root / 'src/PC/cgame_mp/cg_weapons.c').read_text()


def function(text, name):
    match = re.search(r'^[^\n]*\b' + re.escape(name) + r'\([^;{]*?\)\s*\{', text, re.M)
    assert match, name
    end = text.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


# The hooks. A frame without a view weapon closes the window; a WWII scope's
# rifle stays drawn; its angles are set before they become its axes and its
# origin after every stock offset; its glass is read once it is added.
add_view = function(weapons, 'CG_AddViewWeapon')
assert add_view.index('    g_webAimFrac = playerState->fWeaponPosFrac;\n    CG_WwiiScopeClear();\n') < \
    add_view.index('return;')
assert '(!CG_GetWeapReticleZoom(&zoom) || CG_WwiiScopeGlassForWeapon(BG_GetViewmodelWeaponIndex(playerState))))\n' \
    '        drawGun = 1;' in add_view
assert add_view.index('CG_WwiiScopeAdjustViewWeapon(playerState, weaponIndex, weaponAngles);') < \
    add_view.index('AnglesToAxis(weaponAngles, weaponAxis);')
assert add_view.index('CG_Madd(hand.origin, finalAxis[2], cg->swayOffset[2] + CG_DvarValue(imp_cg_gun_z));') < \
    add_view.index('CG_WwiiScopePlaceViewWeapon(playerState, weaponIndex, hand.origin);') < \
    add_view.index('cg->gunXOfs =')
assert add_view.index('CG_AddPlayerWeapon(&hand, playerState, &cg->predictedPlayerEntity, drawGun);') < \
    add_view.index('CG_WwiiScopeUpdate(playerState, weaponIndex, drawGun);') < \
    add_view.index('CG_UpdateWebWeaponPresentation(playerState, weaponIndex, drawGun);')
# Raised, the rifle keeps only its muzzle light, and its own tracer, which
# would streak across the glass, is not drawn.
assert 'fx = bDrawGun && !CG_WwiiScopeMuzzleLightsOnly() ? weapInfo->viewFlashEffect :' in weapons
assert 'int scoped = CG_WwiiScopeLens()->presence > 0.0f;' in weapons
assert 'if (scoped || randomf() >= cg_tracerChance->current.value)' in weapons
# The world zooms over at least the scope's share of the raise, on the curve
# CG_WwiiScopeZoomFraction follows below.
view = (root / 'src/PC/cgame_mp/cg_view_mp.c').read_text()
assert 'weapDef = (char *)BG_GetPlayerWeaponDef(&cg->predictedPlayerState, weapIndex);' in view
assert 'transTime = CG_WwiiScopeZoomInFrac(weapIndex, ((WeaponDef *)weapDef)->fAdsZoomInFrac);' in view
assert 'normalizedLerp = normalizedLerp * normalizedLerp * (3.0f - 2.0f * normalizedLerp);' in view
# The reticle replaces the stock overlay while the WWII scope draws.
reticle = function((root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text(), 'CG_DrawWeapReticle')
assert reticle.index('wwiiVisibility = CG_DrawWwiiScopeReticle();\n    if (wwiiVisibility >= 0.0f)\n'
                     '        return wwiiVisibility;\n') < reticle.index('weapDef->szOverlayMaterial[0]')
main = (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()
assert main.index('CG_HitMarkerInit();') < main.index('CG_WwiiScopeInit();')
# The view weapon's anim goes through the scope once chosen; the rifle is centred with the roll it is turned
# back by, its bolt's included.
run_anims = function(weapons, 'CG_WeaponRunXModelAnims')
assert run_anims.index('    activeAnim = CG_WwiiScopeViewAnim(ps, weaponIndex, activeAnim);\n') < \
    run_anims.index('CG_SetWeaponXAnimWeights(pAnimTree')
place = function(scope, 'CG_WwiiScopePlaceViewWeapon')
assert place.index('CG_WwiiScopeBoltState(ps, weapon, &bolt);') < place.index(
    'CG_WwiiScopeCentreOffset(glass, hand, aim, (glass->roll + CG_WWIISCOPE_BOLT_ROLL * bolt.procedural) * aim, offset);')
adjust = function(scope, 'CG_WwiiScopeAdjustViewWeapon')
assert 'angles[2] -= glass->roll * aim;' in adjust and 'angles[2] -= CG_WWIISCOPE_BOLT_ROLL * bolt.procedural * aim;' in adjust
# The window's near clip comes and goes with the window.
assert 'lens->nearClip = CG_WWIISCOPE_NEAR_CLIP * lens->presence;' in scope

types = re.search(r'#define CG_WWIISCOPE_GLASS_COUNT \d+\n', header)[0] + \
    re.search(r'typedef struct CgWwiiScopeGlass \{.*?\} CgWwiiScopeLens;\n', header, re.S)[0] + \
    re.search(r'typedef struct CgWwiiScopeBolt \{.*?\} CgWwiiScopeBolt;\n', scope, re.S)[0] + \
    re.search(r'typedef struct HudVertex2D \{.*?\} HudVertex2D;\n', hud2d_header, re.S)[0]
defines = ''.join(m[0] for m in re.finditer(r'^#define CG_WWIISCOPE_\w+ [^\n]+\n', scope, re.M))
table = re.search(r'^const CgWwiiScopeGlass cg_wwiiScopeGlasses\[CG_WWIISCOPE_GLASS_COUNT\] = \{\n.*?^\};\n',
                  scope, re.M | re.S)[0]
keeps = re.search(r'^static const float s_fireKeep\[3\] = [^\n]+\n^static const float s_rechamberKeep\[3\] = [^\n]+\n',
                  scope, re.M)[0]
math_body = ''.join(function(scope, name) for name in (
    'CG_WwiiScopeFindGlass', 'CG_WwiiScopeParseGlass', 'CG_WwiiScopeClamp01', 'CG_WwiiScopeSmoothstep', 'CG_WwiiScopeZoomFraction',
    'CG_WwiiScopeCentreOffset', 'CG_WwiiScopeBoltEnvelope', 'CG_WwiiScopeBoltShift', 'CG_WwiiScopeSteady',
    'CG_WwiiScopeBoltProgress', 'CG_WwiiScopeBoltState', 'CG_WwiiScopeProjectPoint', 'CG_WwiiScopeProjectGlass',
    'CG_WwiiScopeProjectionScales', 'CG_WwiiScopeDarkStep', 'CG_WwiiScopeReticleQuad'))
math_support = r'''
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
typedef float vec_t;
typedef vec_t vec3_t[3];
typedef int qboolean;
struct XAnimTree_s { float weights[32]; };
typedef struct { int weaponstate, weaponTime; float fWeaponPosFrac; } playerState_t;
typedef struct { int iRechamberTime; } WeaponDef;
typedef struct { struct XAnimTree_s *tree; } weaponInfo_t;
''' + types + defines + r'''
static int I_stricmp(const char *a, const char *b) { return strcasecmp(a, b); }
static WeaponDef weaponDef = { 1000 };
static WeaponDef *BG_GetPlayerWeaponDef(const playerState_t *ps, int weapon) { (void)ps; (void)weapon; return &weaponDef; }
static struct XAnimTree_s tree;
static weaponInfo_t weaponInfos[4];
static weaponInfo_t *weaponInfoTable = weaponInfos;
static void *imp_cg_weapons = &weaponInfoTable;
static float XAnimGetWeight(const struct XAnimTree_s *t, unsigned int anim) { return t->weights[anim]; }
static unsigned char s_idleBolt[4], s_heldBolt[4];
''' + table + keeps
math_checks = r'''
static int near(float a, float b, float tolerance) { return fabsf(a - b) <= tolerance; }
#define EPS 1e-5f
#define GLASS(i) (&cg_wwiiScopeGlasses[i])
/* The j_gun bone origin that puts a glass's rim centre at (x, 0, 0). */
static void placeRim(const CgWwiiScopeGlass *glass, vec3_t *axis, float x, vec_t *origin) {
    for (int i = 0; i < 3; i++)
        origin[i] = (i ? 0 : x) - glass->rim[0] * axis[0][i] - glass->rim[1] * axis[1][i] - glass->rim[2] * axis[2][i];
}
int main(void) {
    int cases = 0;

    /* Each glass is found by its gun model in any case; nothing else is. */
    for (int i = 0; i < CG_WWIISCOPE_GLASS_COUNT; i++) {
        char upper[64];
        snprintf(upper, sizeof(upper), "%s", GLASS(i)->gunModel);
        for (char *c = upper; *c; c++)
            *c = (char)toupper((unsigned char)*c);
        assert(CG_WwiiScopeFindGlass(GLASS(i)->gunModel) == GLASS(i) && CG_WwiiScopeFindGlass(upper) == GLASS(i));
        assert(GLASS(i)->radius > 0 && GLASS(i)->eye[0] > 0);
    }
    assert(!CG_WwiiScopeFindGlass(NULL) && !CG_WwiiScopeFindGlass("viewmodel_kar98") &&
           !CG_WwiiScopeFindGlass("viewmodel_kar98_scope_x") && !CG_WwiiScopeFindGlass("viewmodel_binoculars"));
    cases++;

    /* A 4x Optic's glass file, the generator's line, is read back; anything
     * else leaves the glass as it was. */
    {
        static const char *const bad[] = {
            "-5.2107 0.1407 3.2472 0.5780 6.3633 0.1443 1.0267\n",
            "-5.2107 0.1407 3.2472 0.5780 6.3633 0.1443 1.0267 0.1268 9\n",
            "-5.2107 0.1407 3.2472 0.5780 6.3633 0.1443 1.0267 0.1268 x\n",
            "-5.2107 0.1407 3.2472 0.0000 6.3633 0.1443 1.0267 0.1268\n",
            "-5.2107 0.1407 3.2472 0.5780 0.0000 0.1443 1.0267 0.1268\n",
            "-5.2107 0.1407 3.2472 0.5780 -6.3633 0.1443 1.0267 0.1268\n",
            "-5.2107 0.1407 nan 0.5780 6.3633 0.1443 1.0267 0.1268\n",
            "-5.2107 0.1407 3.2472 0.5780 6.3633 inf 1.0267 0.1268\n",
            "-5.2107 0.1407 3.2472 0.5780 6.3633 0.1443 1.0267 45.0000\n",
            "-50.0000 0.1407 3.2472 0.5780 6.3633 0.1443 1.0267 0.1268\n",
            "glass\n",
            "",
        };
        CgWwiiScopeGlass glass, before;

        memset(&glass, 0, sizeof(glass));
        assert(CG_WwiiScopeParseGlass("-5.2107 0.1407 3.2472 0.5780 6.3633 0.1443 1.0267 -4.8838\n", &glass));
        assert(glass.rim[0] == -5.2107f && glass.rim[1] == 0.1407f && glass.rim[2] == 3.2472f && glass.radius == 0.578f);
        assert(glass.eye[0] == 6.3633f && glass.eye[1] == 0.1443f && glass.eye[2] == 1.0267f && glass.roll == -4.8838f);
        for (unsigned k = 0; k < sizeof(bad) / sizeof(bad[0]); k++) {
            before = glass;
            assert(!CG_WwiiScopeParseGlass(bad[k], &glass) && !memcmp(&glass, &before, sizeof(glass)));
        }
        assert(!CG_WwiiScopeParseGlass(NULL, &glass));
    }
    cases++;

    /* The window opens with the world's zoom: CG_GetViewFov's smoothed share
     * of the last frac of the raise, all of it at full aim. */
    static const float fracs[] = { 0.05f, 0.08f, 0.4f, 0.42f, 1.0f };
    for (unsigned k = 0; k < sizeof(fracs) / sizeof(fracs[0]); k++) {
        float last = 0;
        for (int step = 0; step <= 1000; step++) {
            float pos = step / 1000.0f, fov = 0, zoom = CG_WwiiScopeZoomFraction(pos, fracs[k]);
            if (pos >= 1.0f) {
                fov = 1;
            } else {
                float n = pos - (1.0f - fracs[k]);
                if (n > 0) {
                    n /= fracs[k];
                    if (n > 1)
                        n = 1;
                    fov = n * n * (3 - 2 * n);
                }
            }
            assert(near(zoom, fov, 1e-6f) && zoom >= last);
            last = zoom;
        }
    }
    assert(CG_WwiiScopeZoomFraction(0.99f, 0) == 0 && CG_WwiiScopeZoomFraction(1, 0) == 1 &&
           CG_WwiiScopeZoomFraction(0.5f, NAN) == 0);
    cases++;

    /* The rifle's projection: the hip view's as the raise starts, narrowing
     * until at full aim each glass fills LENS_RADIUS of half the screen's
     * height; x over the screen's aspect. */
    const float aspect = 16.0f / 9.0f, hipY = 1.0f / (tanf(40.0f * 3.14159265f / 180.0f) * 0.75f);
    for (int i = 0; i < CG_WWIISCOPE_GLASS_COUNT; i++) {
        float p[2], q[2], last = 0;
        CG_WwiiScopeProjectionScales(80, aspect, 0, GLASS(i)->radius, GLASS(i)->eye[0], p);
        assert(p[0] == 0 && p[1] == 0);
        CG_WwiiScopeProjectionScales(80, aspect, 1e-4f, GLASS(i)->radius, GLASS(i)->eye[0], p);
        assert(near(p[1], hipY, 1e-3f * hipY));
        for (int step = 1; step <= 100; step++) {
            CG_WwiiScopeProjectionScales(80, aspect, step / 100.0f, GLASS(i)->radius, GLASS(i)->eye[0], p);
            assert(p[1] > last && near(p[0] * aspect, p[1], 1e-5f * p[1]));
            last = p[1];
        }
        assert(near(GLASS(i)->radius / GLASS(i)->eye[0] * p[1], CG_WWIISCOPE_LENS_RADIUS, 1e-5f));
        CG_WwiiScopeProjectionScales(80, aspect, 1.5f, GLASS(i)->radius, GLASS(i)->eye[0], q);
        assert(q[0] == p[0] && q[1] == p[1]);
    }
    static const float badScales[][5] = { { 80, 0, 1, 0.6f, 2 }, { 80, 1.7f, 1, 0, 2 }, { 80, 1.7f, 1, 0.6f, 0.04f },
                                          { 0, 1.7f, 1, 0.6f, 2 }, { 180, 1.7f, 1, 0.6f, 2 }, { NAN, 1.7f, 1, 0.6f, 2 },
                                          { 80, 1.7f, NAN, 0.6f, 2 }, { 80, 1.7f, -1, 0.6f, 2 } };
    for (unsigned k = 0; k < sizeof(badScales) / sizeof(badScales[0]); k++) {
        float p[2] = { 9, 9 };
        CG_WwiiScopeProjectionScales(badScales[k][0], badScales[k][1], badScales[k][2], badScales[k][3],
                                     badScales[k][4], p);
        assert(p[0] == 0 && p[1] == 0);
    }
    cases++;

    /* The glass's outline: straight ahead, seen from off its axis, canted,
     * facing away, too near and without area. */
    {
        const CgWwiiScopeGlass *glass = GLASS(0);
        vec3_t axis[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } }, view[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
        vec3_t eye = { 0, 0, 0 }, origin;
        const float projection[2] = { 1.7f, 3.02f }, d = glass->eye[0], r = glass->radius;
        float c = cosf(0.3f), s = sinf(0.3f);
        CgWwiiScopeLens lens;

        memset(&lens, 0, sizeof(lens));
        placeRim(glass, axis, d, origin);
        assert(CG_WwiiScopeProjectGlass(glass, (const vec3_t *)axis, origin, eye, (const vec3_t *)view, projection, &lens));
        assert(near(lens.center[0], 0, EPS) && near(lens.center[1], 0, EPS));
        /* The scope's right is the screen's right and its up the screen's up. */
        assert(near(lens.axes[0][0], r / d * projection[0], EPS) && near(lens.axes[0][1], 0, EPS));
        assert(near(lens.axes[1][0], 0, EPS) && near(lens.axes[1][1], r / d * projection[1], EPS));
        /* An eye left of and above the glass sees it right of and below centre. */
        eye[1] = 0.2f;
        eye[2] = 0.1f;
        assert(CG_WwiiScopeProjectGlass(glass, (const vec3_t *)axis, origin, eye, (const vec3_t *)view, projection, &lens));
        assert(near(lens.center[0], 0.2f / d * projection[0], 1e-4f) && near(lens.center[1], -0.1f / d * projection[1], 1e-4f));
        eye[1] = eye[2] = 0;
        /* Canted clockwise, the outline's top leans right. */
        vec3_t canted[3] = { { 1, 0, 0 }, { 0, c, s }, { 0, -s, c } };
        placeRim(glass, canted, d, origin);
        assert(CG_WwiiScopeProjectGlass(glass, (const vec3_t *)canted, origin, eye, (const vec3_t *)view, projection, &lens));
        assert(near(lens.axes[0][0], r / d * c * projection[0], 1e-4f) && near(lens.axes[0][1], -r / d * s * projection[1], 1e-4f));
        assert(near(lens.axes[1][0], r / d * s * projection[0], 1e-4f) && near(lens.axes[1][1], r / d * c * projection[1], 1e-4f));
        vec3_t away[3] = { { -1, 0, 0 }, { 0, -1, 0 }, { 0, 0, 1 } };
        placeRim(glass, away, d, origin);
        assert(!CG_WwiiScopeProjectGlass(glass, (const vec3_t *)away, origin, eye, (const vec3_t *)view, projection, &lens));
        placeRim(glass, axis, 0.5f * CG_WWIISCOPE_MIN_DEPTH, origin);
        assert(!CG_WwiiScopeProjectGlass(glass, (const vec3_t *)axis, origin, eye, (const vec3_t *)view, projection, &lens));
        CgWwiiScopeGlass flat = *glass;
        flat.radius = 0;
        placeRim(glass, axis, d, origin);
        assert(!CG_WwiiScopeProjectGlass(&flat, (const vec3_t *)axis, origin, eye, (const vec3_t *)view, projection, &lens));
    }
    cases++;

    /* The blackout comes in over DARK_IN seconds and goes over DARK_OUT,
     * never past its target; no time, no change. */
    {
        float dark = 0;
        int frames = 0;
        while (dark < 1) {
            float next = CG_WwiiScopeDarkStep(dark, 1, 0.016f);
            assert(next > dark && next <= 1);
            dark = next;
            frames++;
        }
        assert(frames == (int)ceilf(CG_WWIISCOPE_DARK_IN / 0.016f));
        for (frames = 0; dark > 0; frames++) {
            float next = CG_WwiiScopeDarkStep(dark, 0, 0.016f);
            assert(next < dark && next >= 0);
            dark = next;
        }
        assert(frames == (int)ceilf(CG_WWIISCOPE_DARK_OUT / 0.016f));
        assert(CG_WwiiScopeDarkStep(0.5f, 1, 0) == 0.5f && CG_WwiiScopeDarkStep(0.5f, 1, -1) == 0.5f &&
               CG_WwiiScopeDarkStep(0.5f, 0.5f, 0.016f) == 0.5f && CG_WwiiScopeDarkStep(0.5f, 1, NAN) == 0.5f);
    }
    cases++;

    /* Centring: the resting glass, carried round the rifle's origin by the
     * roll the rifle is turned back by (angles[2] less it, AnglesToAxis
     * turning a model's left to (0, cos r, sin r) and its up to (0, -sin r,
     * cos r)), the cant's and a procedural bolt's, is on the view axis at full
     * aim wherever the hand is, half way there at half aim. A Bren's 4x Optic
     * glass sits high over the origin of a canted rifle. An unmeasured eye
     * stays put. */
    {
        CgWwiiScopeGlass glasses[CG_WWIISCOPE_GLASS_COUNT + 1];
        const CgWwiiScopeGlass bren = { "viewmodel_bren", { -6.7356f, 0.8039f, 3.7633f }, 0.635f,
                                        { 7.4642f, 0.1022f, 1.3118f }, -4.8838f };

        memcpy(glasses, cg_wwiiScopeGlasses, sizeof(cg_wwiiScopeGlasses));
        glasses[CG_WWIISCOPE_GLASS_COUNT] = bren;
        for (int i = 0; i <= CG_WWIISCOPE_GLASS_COUNT; i++) {
            const CgWwiiScopeGlass *glass = &glasses[i];
            for (int k = 0; k < 20; k++) {
                for (int a = 0; a < 4; a++) {
                    float aim = a & 1 ? 0.5f : 1.0f;
                    float roll = (glass->roll + (a & 2 ? 0.5f * CG_WWIISCOPE_BOLT_ROLL : 0.0f)) * aim;
                    float r = -roll * 3.14159265f / 180.0f;
                    float left = glass->eye[1] * cosf(r) - glass->eye[2] * sinf(r);
                    float up = glass->eye[1] * sinf(r) + glass->eye[2] * cosf(r);
                    float hand[3] = { 0.3f * k - 3, 0.07f * k - 0.7f, 0.5f - 0.05f * k }, offset[3];
                    CG_WwiiScopeCentreOffset(glass, hand, aim, roll, offset);
                    assert(offset[0] == 0 && near(hand[1] + left + offset[1], (1 - aim) * (hand[1] + left), EPS) &&
                           near(hand[2] + up + offset[2], (1 - aim) * (hand[2] + up), EPS));
                }
            }
        }
        /* Levelled, the Bren's glass comes round to just right of the origin. */
        float hand[3] = { 0, 0, 0 }, offset[3];
        CG_WwiiScopeCentreOffset(&bren, hand, 1, bren.roll, offset);
        assert(near(offset[1], 0.0098f, 2e-4f) && near(offset[2], -1.3157f, 2e-4f));
    }
    {
        CgWwiiScopeGlass unmeasured = *GLASS(0);
        float hand[3] = { 1, 2, 3 }, offset[3];
        unmeasured.eye[0] = 0;
        CG_WwiiScopeCentreOffset(&unmeasured, hand, 1, unmeasured.roll, offset);
        assert(offset[0] == 0 && offset[1] == 0 && offset[2] == 0);
    }
    cases++;

    /* A procedural bolt goes out over the rechamber's first third, holds,
     * and is back by 95 percent, smoothly; across the screen its glass moves
     * the same share of it whatever the glass's size. */
    {
        float last = 0;
        for (int step = 0; step <= 1000; step++) {
            float t = step / 1000.0f, e = CG_WwiiScopeBoltEnvelope(t);
            assert(e >= 0 && e <= 1 && fabsf(e - last) < 0.01f);
            if (t >= 0.3f && t <= 0.6f)
                assert(e == 1);
            if (t <= 0.04f || t >= 0.95f)
                assert(e == 0);
            last = e;
        }
    }
    for (int i = 0; i < CG_WWIISCOPE_GLASS_COUNT; i++) {
        float offset[3] = { 0.5f, 0.25f, -0.125f }, units = CG_WWIISCOPE_LENS_RADIUS / GLASS(i)->radius;
        CG_WwiiScopeBoltShift(GLASS(i)->radius, 0.5f, offset);
        assert(offset[0] == 0.5f && near((offset[1] - 0.25f) * units, 0.5f * CG_WWIISCOPE_BOLT_LEFT, EPS) &&
               near((offset[2] + 0.125f) * units, -0.5f * CG_WWIISCOPE_BOLT_DOWN, EPS));
    }
    cases++;

    /* The stock anims at the eye: the glass keeps s_fireKeep of a fire anim's
     * move and none of a rechamber's move toward or away from the eye; a
     * glass smaller than BOLT_GLASS keeps a smaller share across. The kick
     * still shows, but at most a quarter of it toward the eye. */
    assert(s_fireKeep[0] > 0 && s_fireKeep[0] <= 0.25f && s_rechamberKeep[0] == 0);
    for (int i = 0; i < CG_WWIISCOPE_GLASS_COUNT; i++) {
        const CgWwiiScopeGlass *glass = GLASS(i);
        float scale = glass->radius < CG_WWIISCOPE_BOLT_GLASS ? glass->radius / CG_WWIISCOPE_BOLT_GLASS : 1;
        for (int k = 0; k < 8; k++) {
            float move[3] = { -1.7f + 0.3f * k, 0.4f - 0.1f * k, 0.05f * k - 0.3f }, model[3], offset[3] = { 0, 0, 0 };
            for (int a = 0; a < 3; a++)
                model[a] = glass->eye[a] + move[a];
            CG_WwiiScopeSteady(glass, model, s_fireKeep, 1, offset);
            assert(near(move[0] + offset[0], s_fireKeep[0] * move[0], EPS));
            for (int a = 1; a < 3; a++)
                assert(near(move[a] + offset[a], s_fireKeep[a] * scale * move[a], EPS));
            memset(offset, 0, sizeof(offset));
            CG_WwiiScopeSteady(glass, model, s_rechamberKeep, 1, offset);
            assert(near(move[0] + offset[0], 0, EPS));
            for (int a = 1; a < 3; a++)
                assert(near(move[a] + offset[a], s_rechamberKeep[a] * scale * move[a], EPS));
            memset(offset, 0, sizeof(offset));
            CG_WwiiScopeSteady(glass, model, s_rechamberKeep, 0.5f, offset);
            assert(near(move[0] + offset[0], 0.5f * move[0], EPS));
            memset(offset, 0, sizeof(offset));
            CG_WwiiScopeSteady(glass, model, s_fireKeep, 0, offset);
            assert(offset[0] == 0 && offset[1] == 0 && offset[2] == 0);
        }
    }
    cases++;

    /* What holds the rifle's pose is the anims' weights, not the weapon's
     * state: the client plays a fire or rechamber anim to its end, past its
     * state's time. */
    {
        static const int fires[4] = { 3, 5, 18, 19 };
        playerState_t ps = { 0, 0, 1 };
        CgWwiiScopeBolt bolt;

        weaponInfos[1].tree = weaponInfos[2].tree = &tree;
        s_idleBolt[1] = 0;
        s_idleBolt[2] = 1;
        for (int k = 0; k < 4; k++) {
            memset(&tree, 0, sizeof(tree));
            tree.weights[fires[k]] = 0.75f;
            for (int weapon = 1; weapon <= 2; weapon++) {
                CG_WwiiScopeBoltState(&ps, weapon, &bolt);
                assert(bolt.fire == 0.75f && bolt.rechamber == 0 && bolt.procedural == 0);
            }
        }
        /* Not the idle, the raise or a reload. */
        for (int anim = 0; anim < 32; anim++) {
            if (anim == 3 || anim == 5 || anim == 6 || anim == 18 || anim == 19 || anim == 20)
                continue;
            memset(&tree, 0, sizeof(tree));
            tree.weights[anim] = 1;
            CG_WwiiScopeBoltState(&ps, 1, &bolt);
            assert(bolt.fire == 0 && bolt.rechamber == 0 && bolt.procedural == 0);
        }
        /* The hip rechamber is animated on every rifle, the scoped one on a
         * rifle that has one; the other works its bolt procedurally. */
        memset(&tree, 0, sizeof(tree));
        tree.weights[6] = 1;
        CG_WwiiScopeBoltState(&ps, 1, &bolt);
        assert(bolt.rechamber == 1);
        CG_WwiiScopeBoltState(&ps, 2, &bolt);
        assert(bolt.rechamber == 1 && bolt.procedural == 0);
        memset(&tree, 0, sizeof(tree));
        tree.weights[20] = 1;
        ps.weaponstate = CG_WWIISCOPE_RECHAMBERING;
        ps.weaponTime = 500;
        CG_WwiiScopeBoltState(&ps, 1, &bolt);
        assert(bolt.rechamber == 1 && bolt.procedural == 0);
        CG_WwiiScopeBoltState(&ps, 2, &bolt);
        assert(bolt.rechamber == 0 && bolt.procedural == 1);
        /* Its state over, the scoped rechamber anim still holds the pose. */
        ps.weaponstate = 0;
        ps.weaponTime = 0;
        CG_WwiiScopeBoltState(&ps, 1, &bolt);
        assert(bolt.rechamber == 1);
        CG_WwiiScopeBoltState(&ps, 2, &bolt);
        assert(bolt.procedural == 0);
        /* The procedural bolt follows the rechamber's progress and the scoped
         * anim's weight. */
        ps.weaponstate = CG_WWIISCOPE_RECHAMBERING;
        tree.weights[20] = 0.5f;
        for (int left = 1000; left >= 0; left -= 50) {
            ps.weaponTime = left;
            CG_WwiiScopeBoltState(&ps, 2, &bolt);
            assert(near(bolt.procedural, 0.5f * CG_WwiiScopeBoltEnvelope(1 - left / 1000.0f), EPS));
        }
        weaponDef.iRechamberTime = 0;
        CG_WwiiScopeBoltState(&ps, 2, &bolt);
        assert(bolt.procedural == 0);
        weaponDef.iRechamberTime = 1000;
        /* A rifle whose scoped rechamber anim gave way to the idle pose works
         * its bolt procedurally over the whole rechamber, no scoped anim
         * playing. */
        memset(&tree, 0, sizeof(tree));
        tree.weights[1] = 1;
        s_heldBolt[1] = 1;
        for (int left = 1000; left >= 0; left -= 50) {
            ps.weaponTime = left;
            CG_WwiiScopeBoltState(&ps, 1, &bolt);
            assert(bolt.rechamber == 0 && near(bolt.procedural, CG_WwiiScopeBoltEnvelope(1 - left / 1000.0f), EPS));
        }
        s_heldBolt[1] = 0;
        ps.weaponTime = 500;
        CG_WwiiScopeBoltState(&ps, 1, &bolt);
        assert(bolt.rechamber == 0 && bolt.procedural == 0);
        /* No anim tree, no bolt. */
        CG_WwiiScopeBoltState(&ps, 3, &bolt);
        assert(bolt.fire == 0 && bolt.rechamber == 0 && bolt.procedural == 0);
    }
    cases++;

    /* The reticle: the overlay's lens circle lands on the glass's outline,
     * the image's top along the scope's up, faded by alpha. */
    {
        const float width = 1920, height = 1080, edge = 0.8f;
        CgWwiiScopeLens lens;
        HudVertex2D v[4];
        float cx, cy;

        memset(&lens, 0, sizeof(lens));
        lens.center[0] = 0.1f;
        lens.center[1] = -0.2f;
        lens.axes[0][0] = 0.5f;
        lens.axes[0][1] = 0.05f;
        lens.axes[1][0] = -0.03f;
        lens.axes[1][1] = 0.9f;
        CG_WwiiScopeReticleQuad(&lens, edge, width, height, 0.7f, v);
        for (int i = 0; i < 4; i++)
            assert(v[i].rgba[0] == 1 && v[i].rgba[1] == 1 && v[i].rgba[2] == 1 && v[i].rgba[3] == 0.7f);
        assert(v[0].st[0] == 0 && v[0].st[1] == 0 && v[1].st[0] == 1 && v[1].st[1] == 0 && v[2].st[0] == 1 &&
               v[2].st[1] == 1 && v[3].st[0] == 0 && v[3].st[1] == 1);
#define AT(s, t, c) (v[0].xy[c] + (s) * (v[1].xy[c] - v[0].xy[c]) + (t) * (v[3].xy[c] - v[0].xy[c]))
        cx = (lens.center[0] + 1) * 0.5f * width;
        cy = (1 - lens.center[1]) * 0.5f * height;
        assert(near(AT(0.5f, 0.5f, 0), cx, 1e-2f) && near(AT(0.5f, 0.5f, 1), cy, 1e-2f));
        assert(near(AT(0.5f + 0.5f * edge, 0.5f, 0), cx + lens.axes[0][0] * 0.5f * width, 1e-2f));
        assert(near(AT(0.5f + 0.5f * edge, 0.5f, 1), cy - lens.axes[0][1] * 0.5f * height, 1e-2f));
        assert(near(AT(0.5f, 0.5f - 0.5f * edge, 0), cx + lens.axes[1][0] * 0.5f * width, 1e-2f));
        assert(near(AT(0.5f, 0.5f - 0.5f * edge, 1), cy - lens.axes[1][1] * 0.5f * height, 1e-2f));
        for (int c = 0; c < 2; c++)
            assert(near(v[3].xy[c], v[0].xy[c] + v[2].xy[c] - v[1].xy[c], 1e-2f));
    }
    cases++;
    printf("%d\n", cases);
    return 0;
}
'''
math_mutants = [
    ('if (!I_stricmp(cg_wwiiScopeGlasses[i].gunModel, gunModel))', 'if (!strcmp(cg_wwiiScopeGlasses[i].gunModel, gunModel))'),
    ('    return CG_WwiiScopeSmoothstep(0.0f, 1.0f, (posFrac - (1.0f - frac)) / frac);',
     '    return CG_WwiiScopeClamp01((posFrac - (1.0f - frac)) / frac);'),
    ('    projection[0] = projection[1] / aspect;', '    projection[0] = projection[1];'),
    ('    hipTan = tanf(hipFov * 0.00872664626f) * 0.75f;', '    hipTan = tanf(hipFov * 0.00872664626f);'),
    ('    scopeTan = radius / (CG_WWIISCOPE_LENS_RADIUS * eyeForward);', '    scopeTan = radius / eyeForward;'),
    ('    if (!(facing > 0.0f))\n        return 0;\n', ''),
    ('        lens->axes[0][i] = 0.5f * (ndc[0][i] - ndc[1][i]);', '        lens->axes[0][i] = 0.5f * (ndc[1][i] - ndc[0][i]);'),
    ('    ndc[0] = -left / forward * projection[0];', '    ndc[0] = left / forward * projection[0];'),
    ('    if (!(forward > CG_WWIISCOPE_MIN_DEPTH))\n        return 0;\n', ''),
    ('target > dark ? CG_WWIISCOPE_DARK_IN : CG_WWIISCOPE_DARK_OUT', 'target > dark ? CG_WWIISCOPE_DARK_OUT : CG_WWIISCOPE_DARK_IN'),
    ('    return target > dark ? fminf(target, dark + rate) : fmaxf(target, dark - rate);',
     '    return target > dark ? dark + rate : dark - rate;'),
    ('-(hand[1] + left) * aim', '-(hand[1] + left)'),
    ('    float turn = roll * 0.0174532925f;', '    float turn = 0.0f;'),
    ('    float turn = roll * 0.0174532925f;', '    float turn = glass->roll * aim * 0.0174532925f;'),
    ('glass->eye[1] * cosf(turn) + glass->eye[2] * sinf(turn)', 'glass->eye[1] * cosf(turn) - glass->eye[2] * sinf(turn)'),
    (' || used < 0 || text[used])', ')'),
    ('    if (!(v[3] > 0.0f) || !(v[4] > 0.0f))\n        return 0;\n', ''),
    ('        if (!(fabsf(v[i]) < CG_WWIISCOPE_GLASS_REACH))\n            return 0;\n', ''),
    ('        glass->eye[i] = v[4 + i];', '        glass->eye[i] = v[3 + i];'),
    ('    return CG_WwiiScopeSmoothstep(0.04f, 0.3f, t) * (1.0f - CG_WwiiScopeSmoothstep(0.6f, 0.95f, t));',
     '    return CG_WwiiScopeSmoothstep(0.04f, 0.3f, t);'),
    ('    float units = radius / CG_WWIISCOPE_LENS_RADIUS * envelope;', '    float units = envelope;'),
    ('(1.0f - keep[i] * (i ? scale : 1.0f))', '(1.0f - keep[i])'),
    ('static const float s_rechamberKeep[3] = { 0.0f, 0.8f, 0.3f };', 'static const float s_rechamberKeep[3] = { 0.25f, 0.8f, 0.3f };'),
    ('idle ? 0.0f : scoped', 'scoped'),
    ('    scoped = s_heldBolt[weapon] ? 1.0f : XAnimGetWeight', '    scoped = XAnimGetWeight'),
    ('    idle = s_idleBolt[weapon] || s_heldBolt[weapon];', '    idle = s_idleBolt[weapon];'),
    ('            bolt->procedural = CG_WwiiScopeBoltEnvelope(progress) * scoped;',
     '            bolt->procedural = CG_WwiiScopeBoltEnvelope(progress);'),
    ('    bolt->rechamber = fmaxf(XAnimGetWeight(tree, CG_WWIISCOPE_RECHAMBER_ANIM), idle ? 0.0f : scoped);',
     '    bolt->rechamber = ps->weaponstate == CG_WWIISCOPE_RECHAMBERING && !idle;'),
    ('    static const unsigned char fireAnims[4] = { 3, 5, 18, 19 };', '    static const unsigned char fireAnims[4] = { 3, 5, 18, 18 };'),
    ('    if (!tree)\n        return;\n', ''),
    ('        verts[i].st[1] = corners[i][1] > 0.0f ? 0.0f : 1.0f;', '        verts[i].st[1] = corners[i][1] > 0.0f ? 1.0f : 0.0f;'),
    ('    right[1] = -lens->axes[0][1] * 0.5f * height / edge;', '    right[1] = lens->axes[0][1] * 0.5f * height / edge;'),
    ('        verts[i].rgba[3] = alpha;', '        verts[i].rgba[3] = 1.0f;'),
]

# The lens image, from r_hud2d.c.
lens_body = ''.join(function(hud2d, name) for name in (
    'R_DecodeBlockAlpha', 'R_GrayBlockBytes', 'R_GetGrayImageSize', 'R_DecodeColorBlock', 'R_DecodeLensImage',
    'R_ClearOutsideLens'))
lens_defines = ''.join(re.search(r'^#define ' + name + r' .*\n', hud2d, re.M)[0] for name in (
    'IWI_VERSION', 'IWI_FORMAT_DXT1', 'IWI_FORMAT_DXT3', 'IWI_FORMAT_DXT5', 'DXT_BLOCK_BYTES', 'DXT1_BLOCK_BYTES',
    'LENS_OPAQUE_ALPHA', 'LENS_DIRECTIONS'))
lens_defines += ''.join(re.search(r'^#define ' + name + r' .*\n', hud2d_header, re.M)[0] for name in (
    'R_HUD_GRAY_MAX_SIZE', 'R_HUD_LENS_MAX_SIZE'))
lens_support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte;
typedef struct GfxImageFileHeader { char tag[3]; byte version, format, flags; short dimensions[3]; int fileSizeForPicmip[4]; }
    GfxImageFileHeader;
''' + lens_defines
lens_checks = r'''
static byte fileData[1 << 22];
static byte out[512 * 512 * 4];
/* An IWi file: the header, then mips smallest first, the top one last. */
static int iwi(byte *file, int format, int w, int h, const byte *top, int topBytes) {
    GfxImageFileHeader header = { { 'I', 'W', 'i' }, 5, (byte)format, 0, { (short)w, (short)h, 1 }, { 0 } };
    memcpy(file, &header, sizeof(header));
    memcpy(file + sizeof(header), top, (size_t)topBytes);
    return (int)sizeof(header) + topBytes;
}
static void block(byte *b, int c0, int c1, unsigned int bits) {
    b[0] = (byte)c0; b[1] = (byte)(c0 >> 8); b[2] = (byte)c1; b[3] = (byte)(c1 >> 8);
    b[4] = (byte)bits; b[5] = (byte)(bits >> 8); b[6] = (byte)(bits >> 16); b[7] = (byte)(bits >> 24);
}
static int texel(const byte *t, int b, int g, int r, int a) { return t[0] == b && t[1] == g && t[2] == r && t[3] == a; }
/* A lens overlay of size by size texels: black from radius on about its
 * centre, a fainter darkening rim three texels inside that, and a reticle's
 * posts along the axes. */
static void overlay(byte *bgra, int size, float radius) {
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            byte *t = bgra + (y * size + x) * 4;
            float fx = x + 0.5f - size / 2, fy = y + 0.5f - size / 2, r = sqrtf(fx * fx + fy * fy);
            t[0] = t[1] = t[2] = 40;
            t[3] = r >= radius || x == size / 2 || y == size / 2 ? 255 : r >= radius - 3 ? 120 : 0;
        }
    }
}
static float overlayEdge(byte *bgra, int size, float radius, float edge) {
    static byte before[64 * 64 * 4];
    memcpy(before, bgra, (size_t)size * size * 4);
    float half = size / 2.0f, found = R_ClearOutsideLens(bgra, size, size) * half;
    assert(fabsf(found - edge) <= 1.0f);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float fx = x + 0.5f - half, fy = y + 0.5f - half, r = sqrtf(fx * fx + fy * fy);
            int i = (y * size + x) * 4;
            assert(!memcmp(bgra + i, before + i, 3));
            if (r <= found - 0.5f)
                assert(bgra[i + 3] == before[i + 3]);
            if (r >= found + 0.5f)
                assert(bgra[i + 3] == 0);
        }
    }
    (void)radius;
    return found;
}
int main(int argc, char **argv) {
    int cases = 0, w, h, n;
    static byte top[4096];

    /* DXT1, red above blue: four colors, in B, G, R order, opaque. */
    block(top, 0xf800, 0x001f, 0xe4e4e4e4u);
    n = iwi(fileData, 0x0b, 4, 4, top, 8);
    assert(R_DecodeLensImage(fileData, n, out, &w, &h) && w == 4 && h == 4);
    for (int y = 0; y < 4; y++) {
        const byte *t = out + y * 16;
        assert(texel(t, 0, 0, 255, 255) && texel(t + 4, 255, 0, 0, 255) && texel(t + 8, 85, 0, 170, 255) &&
               texel(t + 12, 170, 0, 85, 255));
    }
    /* Blue first: three colors and clear black. */
    block(top, 0x001f, 0xf800, 0xe4e4e4e4u);
    n = iwi(fileData, 0x0b, 4, 4, top, 8);
    assert(R_DecodeLensImage(fileData, n, out, &w, &h));
    assert(texel(out + 8, 127, 0, 127, 255) && texel(out + 12, 0, 0, 0, 0));
    /* DXT5: alpha from its own block, four colors whatever the endpoints. */
    byte dxt5[16] = { 255, 0, 0x01, 0, 0, 0, 0, 0 };
    block(dxt5 + 8, 0x001f, 0xf800, 0xe4e4e4e4u);
    n = iwi(fileData, 0x0d, 4, 4, dxt5, 16);
    assert(R_DecodeLensImage(fileData, n, out, &w, &h));
    assert(out[3] == 0 && out[7] == 255 && texel(out + 12, 85, 0, 170, 255));
    cases += 3;

    /* Wider or taller than R_HUD_LENS_MAX_SIZE, it is halved; each 2x2
     * average weighs colors by alpha, so one opaque red texel among clear
     * green ones stays red, at a quarter of the alpha. */
    byte quad[16] = { 255, 0 };
    unsigned long long alphaBits = (1ull << 3) | (1ull << 12) | (1ull << 15);
    for (int i = 0; i < 6; i++)
        quad[2 + i] = (byte)(alphaBits >> (8 * i));
    block(quad + 8, 0xf800, 0x07e0, (1u << 2) | (1u << 8) | (1u << 10));
    for (int b = 0; b < 256; b++)
        memcpy(top + 16 * b, quad, 16);
    n = iwi(fileData, 0x0d, 1024, 4, top, 4096);
    assert(R_DecodeLensImage(fileData, n, out, &w, &h) && w == 512 && h == 2);
    assert(texel(out, 0, 0, 255, 63) && texel(out + 4, 0, 0, 255, 255));
    n = iwi(fileData, 0x0d, 4, 1024, top, 4096);
    assert(R_DecodeLensImage(fileData, n, out, &w, &h) && w == 2 && h == 512);
    assert(texel(out, 0, 0, 255, 63));
    cases += 2;

    /* The lens edge is where the image first turns opaque between the axes,
     * the median of its directions, so a reticle's posts and a speck of dirt
     * do not move it; outside it the image is clear, inside untouched. */
    static byte lens[64 * 64 * 4];
    overlay(lens, 64, 20);
    overlayEdge(lens, 64, 20, 20);
    overlay(lens, 64, 24);
    for (int y = 38; y < 46; y++)
        for (int x = 38; x < 46; x++)
            lens[(y * 64 + x) * 4 + 3] = 255;
    overlayEdge(lens, 64, 24, 24);
    /* Nothing opaque: the whole half width. */
    memset(lens, 0, sizeof(lens));
    assert(R_ClearOutsideLens(lens, 64, 64) == 1.0f);
    cases += 3;

    /* The retail overlays: each lens reaches nearly to its image's sides
     * (0.976 of the half width, the Russian 0.904) and is clear between its
     * reticle's lines. */
    for (int k = 1; k < argc; k++) {
        FILE *f = fopen(argv[k], "rb");
        assert(f);
        n = (int)fread(fileData, 1, sizeof(fileData), f);
        fclose(f);
        assert(R_DecodeLensImage(fileData, n, out, &w, &h) && w <= 512 && h <= 512 && w == h);
        float edge = R_ClearOutsideLens(out, w, h), half = w / 2.0f;
        int clear = 0, inside = 0;
        assert(edge > 0.85f && edge < 1.0f);
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                float fx = x + 0.5f - half, fy = y + 0.5f - half, r = sqrtf(fx * fx + fy * fy);
                if (r >= edge * half + 0.5f)
                    assert(out[(y * w + x) * 4 + 3] == 0);
                else if (r < 0.8f * edge * half) {
                    inside++;
                    clear += out[(y * w + x) * 4 + 3] < 128;
                }
            }
        }
        assert(clear > inside / 2);
        printf("edge %s %.3f\n", argv[k], edge);
        cases++;
    }
    printf("%d\n", cases);
    return 0;
}
'''
lens_mutants = [
    ('            alpha[i] = endpoints[0] <= endpoints[1] && index == 3 ? 0 : 255;', '            alpha[i] = 255;'),
    ('        if (!dxt1 || endpoints[0] > endpoints[1]) {', '        if (endpoints[0] > endpoints[1]) {'),
    ('                R_DecodeColorBlock(block + 8, 0, bgr, alpha);', '                R_DecodeColorBlock(block, 0, bgr, alpha);'),
    ('                out[c] = (byte)(sum[3] ? sum[c] / sum[3] : plain[c] / (unsigned int)n);',
     '                out[c] = (byte)(plain[c] / (unsigned int)n);'),
    ('(w >> shift) > R_HUD_LENS_MAX_SIZE || (h >> shift) > R_HUD_LENS_MAX_SIZE', '(w >> shift) > R_HUD_LENS_MAX_SIZE'),
    ('    edge = found ? edges[found / 2] : half;', '    edge = found ? edges[0] : half;'),
    ('            if (cover <= 0.0f)\n                *alpha = 0;\n            else if', '            if'),
    ('            if (bgra[(ty * width + tx) * 4 + 3] >= LENS_OPAQUE_ALPHA) {', '            if (bgra[(ty * width + tx) * 4 + 3]) {'),
]


def run(directory, name, text, flags, expect_pass, args=()):
    path = Path(directory) / f'{name}.c'
    binary = Path(directory) / name
    path.write_text(text)
    subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    *flags, str(path), '-lm', '-o', str(binary)], check=True)
    result = subprocess.run([str(binary), *args], capture_output=True, text=True)
    assert (result.returncode == 0) == expect_pass, (name, result.stdout[-500:], result.stderr[-3000:])
    return result


def mutate(body, before, after):
    assert body.count(before) == 1, before
    return body.replace(before, after)


# The retail assets, when present: later IWDs override earlier ones.
iwds = sorted((root / 'data/main').glob('iw_*.iwd'))
archives = [zipfile.ZipFile(path) for path in iwds]


def asset(name):
    data = None
    for archive in archives:
        try:
            data = archive.read(name)
        except KeyError:
            pass
    return data


def cstring(data, offset):
    end = data.index(b'\0', offset)
    return data[offset:end].decode('latin1'), end + 1


def model_points(model):
    """The vertices of a viewmodel's first LOD bound to j_gun, in its space."""
    data = asset('xmodel/' + model)
    offset = 27
    lods = []
    for _ in range(4):
        offset += 4
        name, offset = cstring(data, offset)
        lods.append(name)
    parts = asset('xmodelparts/' + lods[0])
    count, roots = struct.unpack_from('<hh', parts, 2)
    offset = 6 + 19 * count
    bones = []
    for _ in range(roots + count):
        name, offset = cstring(parts, offset)
        bones.append(name)
    gun = bones.index('j_gun')
    surfs = asset('xmodelsurfs/' + lods[0])
    points = []
    offset = 4
    for _ in range(struct.unpack_from('<h', surfs, 2)[0]):
        offset += 1
        vertices, triangles = struct.unpack_from('<HH', surfs, offset)
        offset += 4
        rigid = struct.unpack_from('<h', surfs, offset)[0]
        offset += 2 if rigid != -1 else 4
        for _ in range(vertices):
            offset += 12 + 4 + 8 + 24
            if rigid != -1:
                bone = rigid
            else:
                weights = surfs[offset]
                bone = struct.unpack_from('<h', surfs, offset + 1)[0]
                offset += 3
            position = struct.unpack_from('<3f', surfs, offset)
            offset += 12
            if rigid == -1 and weights:
                offset += 1 + 16 * weights
            if bone == gun:
                points.append(position)
        offset += 6 * triangles
    assert offset == len(surfs), model
    return points


glass_rows = re.findall(r'\{ "(\w+)", \{ ([-\d.]+)f, ([-\d.]+)f, ([-\d.]+)f \}, ([-\d.]+)f, '
                        r'\{ ([-\d.]+)f, ([-\d.]+)f, ([-\d.]+)f \}, ([-\d.]+)f \}', table)
assert len(glass_rows) == int(re.search(r'#define CG_WWIISCOPE_GLASS_COUNT (\d+)', header)[1])
near_clip = float(re.search(r'#define CG_WWIISCOPE_NEAR_CLIP ([\d.]+)f', scope)[1])
fire_keep = float(re.search(r's_fireKeep\[3\] = \{ ([\d.]+)f', scope)[1])
asset_note = 'no retail IWDs in data/main: the glass table, near clip and retail lens checks were skipped'
overlays = []
if archives:
    for model, *numbers in glass_rows:
        rim, radius, eye = [float(v) for v in numbers[0:3]], float(numbers[3]), float(numbers[4])
        points = model_points(model)
        # The glass lies in the plane of a whole ring of the scope's vertices,
        # at its radius (the Mosin-Nagant's painted glass sits just inside its
        # brass ring).
        angles = {round(math.degrees(math.atan2(p[2] - rim[2], p[1] - rim[1])) / 10) for p in points
                  if abs(p[0] - rim[0]) < 0.02 and radius - 0.02 < math.hypot(p[1] - rim[1], p[2] - rim[2]) < radius + 0.15}
        assert len(angles) >= 8, (model, len(angles))
        # Every eyepiece stays beyond the window's near clip from the eye at
        # rest, even after keeping its share of a fire anim's kick, which
        # pulls a glass back by up to two units.
        rear = min(p[0] for p in points if math.hypot(p[1] - rim[1], p[2] - rim[2]) < 1.6)
        clearance = rear - (rim[0] - eye)
        assert clearance - fire_keep * 2.0 > near_clip, (model, clearance)
    for country in ('american', 'britain', 'german', 'russian'):
        data = asset(f'images/scope_overlay_{country}.iwi')
        assert data, country
        overlays.append((country, data))
    asset_note = (f'the {len(glass_rows)} glasses against their viewmodels\' rings, the eyepieces beyond the '
                  f'{near_clip}-unit near clip, and {len(overlays)} retail lens edges')

# The zoom's share of the raise is the weapon's own, with its player's
# attachments, raised to the scope's for a rifle the WWII scope draws.
zoom_body = function(scope, 'CG_WwiiScopeZoomInFrac')
zoom_support = r'''
#include <assert.h>
typedef struct CgWwiiScopeGlass CgWwiiScopeGlass;
static int glassWeapon = 3;
static const CgWwiiScopeGlass *CG_WwiiScopeGlassForWeapon(int weapon) {
    return weapon == glassWeapon ? (const CgWwiiScopeGlass *)&glassWeapon : 0;
}
''' + re.search(r'^#define CG_WWIISCOPE_ZOOM_IN_FRAC [^\n]+\n', scope, re.M)[0]
zoom_checks = r'''
int main(void) {
    /* A 4x Optic's 0.42 on an iron-sighted gun stays its own. */
    assert(CG_WwiiScopeZoomInFrac(5, 0.42f) == 0.42f && CG_WwiiScopeZoomInFrac(5, 0.1f) == 0.1f);
    assert(CG_WwiiScopeZoomInFrac(3, 0.1f) == CG_WWIISCOPE_ZOOM_IN_FRAC && CG_WwiiScopeZoomInFrac(3, 0.6f) == 0.6f);
    return 0;
}
'''
zoom_mutants = [('frac < CG_WWIISCOPE_ZOOM_IN_FRAC', 'frac > CG_WWIISCOPE_ZOOM_IN_FRAC'),
                ('    return frac;\n', '    return CG_WWIISCOPE_ZOOM_IN_FRAC;\n')]

# The glass a player's weapon is drawn with: a scoped rifle's own; a 4x Optic's, read once from beside the optic's
# model, while the player has one on a gun without a scope of its own and the optic brought its overlay; the
# reticle cut from that overlay. Every match starts with the 4x Optics' glass unread.
assert 'memset(s_sightKnown, CG_WWIISCOPE_UNKNOWN, sizeof(s_sightKnown));' in function(scope, 'CG_WwiiScopeInit')
# A Reflex or Lens Sight's window, when no WWII scope draws the weapon: its ring's disc projected as the view
# draws the rifle, coming in over the last of the raise, handed to the compositor in the scope's place.
update = function(scope, 'CG_WwiiScopeUpdate')
assert update.index('    if (!s_viewWeapon)\n        CG_WwiiScopeUpdateWindow(ps, weapon, drawGun);\n') < \
    update.index('    memcpy(g_webScopeLens, s_viewWeapon ? lens : &s_window, sizeof(g_webScopeLens));')
window_update = function(scope, 'CG_WwiiScopeUpdateWindow')
for line in ('!(ps->fWeaponPosFrac > CG_WWIISCOPE_WINDOW_START) || !CG_WwiiScopeComposed()',
             '    disc.radius *= ring;',
             '    projection[0] = 1.0f / tanf(cg->refdef.fov_x * 0.00872664626f);',
             '    projection[1] = 1.0f / tanf(cg->refdef.fov_y * 0.00872664626f);',
             'CG_WwiiScopeSmoothstep(CG_WWIISCOPE_WINDOW_START, CG_WWIISCOPE_WINDOW_END, ps->fWeaponPosFrac);',
             '    s_window.open = s_window.presence;',
             '    s_windowOpening = 1.0f / ring;'):
    assert line in window_update, line
assert 'blur' not in window_update and 'dark' not in window_update and 'nearClip' not in window_update
for name in ('CG_WwiiScopeClear', 'CG_WwiiScopeInit'):
    assert 'memset(&s_window, 0, sizeof(s_window));' in function(scope, name), name
select_body = ''.join(function(scope, name) for name in (
    'CG_WwiiScopeFindGlass', 'CG_WwiiScopeParseGlass', 'CG_WwiiScopeWeaponGlass', 'CG_WwiiScopeSightGlass',
    'CG_WwiiScopeWindowGlass', 'CG_WwiiScopePlayerGlass', 'CG_WwiiScopeOverlay', 'CG_WwiiScopeViewAnim',
    'CG_WwiiScopeOpeningPixels'))
select_support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
typedef float vec_t;
typedef struct Material *MaterialHandle;
struct Material { const char *name; };
typedef struct { unsigned attachments; } playerState_t;
typedef struct {
    char *szGunXModel;
    char *szXAnims[23];
    int weapClass;
    char *szOverlayMaterial;
    int overlayReticle;
} WeaponDef;
typedef struct { MaterialHandle hADSOverlay; } weaponInfo_t;
enum { ATT_REFLEX, ATT_LENS, ATT_OPTIC4X };
#define ATT_BIT(a) (1u << (a))
static const struct { const char *name; } g_attachments[] = { { "reflex" }, { "lens" }, { "optic4x" } };
''' + types + defines + r'''
static int I_stricmp(const char *a, const char *b) { return strcasecmp(a, b); }
''' + table + re.search(r'^static const unsigned char s_sights\[.*?^static const float s_sightRings\[[^\n]*\n', scope,
                        re.M | re.S)[0] + \
    re.search(r'^static signed char s_glassIndex\[.*?^static unsigned char s_heldBolt\[[^\n]*\n', scope,
              re.M | re.S)[0] + r'''
/* The scope draws the view weapon unless cg_wwiiScope or the compositor turn it off. */
static int scopeOn = 1;
static const CgWwiiScopeGlass *CG_WwiiScopePlayerGlass(const playerState_t *ps, int weapon);
static const CgWwiiScopeGlass *CG_WwiiScopeGlassFor(const playerState_t *ps, int weapon) {
    return scopeOn ? CG_WwiiScopePlayerGlass(ps, weapon) : NULL;
}
#define WEAPONS 8
static WeaponDef defs[WEAPONS + 1], attached[WEAPONS + 1];
/* The overlay each gun's 4x Optic borrows, as BG_ApplyAttachments gives it to a gun without one. */
static char *nations[WEAPONS + 1];
static weaponInfo_t weaponInfos[WEAPONS + 1];
static weaponInfo_t *weaponInfoTable = weaponInfos;
static void *imp_cg_weapons = &weaponInfoTable;
static int BG_GetNumWeapons(void) { return WEAPONS; }
static WeaponDef *BG_GetWeaponDef(int weapon) { return &defs[weapon]; }
/* The player's attachments ride on every weapon here. */
static unsigned BG_GetWeaponAttachments(const playerState_t *ps, int weapon) { (void)weapon; return ps->attachments; }
static WeaponDef *BG_GetPlayerWeaponDef(const playerState_t *ps, int weapon) {
    if (!ps->attachments)
        return &defs[weapon];
    attached[weapon] = defs[weapon];
    if ((ps->attachments & ATT_BIT(ATT_OPTIC4X)) && !defs[weapon].szOverlayMaterial[0] && nations[weapon]) {
        attached[weapon].szOverlayMaterial = nations[weapon];
        attached[weapon].overlayReticle = 2;
    }
    return &attached[weapon];
}
static const struct { const char *name, *text; } files[] = {
    { "glass/viewmodel_m1garand__optic4x", "-5.2107 0.1407 3.2472 0.5780 6.3633 0.1443 1.0267 0.1268\n" },
    { "glass/viewmodel_kar98k__optic4x", "-6.1160 -0.0570 2.9960\n" },
    { "glass/viewmodel_bren__optic4x", "-6.7356 0.8039 3.7633 0.6350 7.4642 0.1022 1.3118 -4.8838\n" },
    { "glass/viewmodel_scoped__optic4x", "-6.1160 -0.0570 2.9960 0.6120 5.3382 0.0118 1.0586 -0.2231\n" },
    { "glass/viewmodel_mosinnagant__optic4x", "-3.8107 0.1590 2.9502 0.5780 4.8633 0.1466 1.1944 0.0000\n" },
    { "glass/viewmodel_m1garand__reflex", "-4.1300 0.0400 3.3200 0.6400 8.0000 0.0000 0.0000 0.1268\n" },
    { "glass/viewmodel_m1garand__lens", "-4.1300 0.0400 3.3200 0.5000 8.0000 0.0000 0.0000 0.1268\n" },
};
static int reads, frees;
static int FS_ReadFile(const char *qpath, void **buffer) {
    reads++;
    for (unsigned i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        if (!strcmp(files[i].name, qpath)) {
            size_t length = strlen(files[i].text);
            *buffer = memcpy(malloc(length + 1), files[i].text, length + 1);
            return (int)length;
        }
    }
    *buffer = NULL;
    return -1;
}
static void FS_FreeFile(void *buffer) { frees++; free(buffer); }
static struct Material materials[8];
static int registered;
static MaterialHandle CL_RegisterMaterialNoMip(const char *name, int imageTrack) {
    assert(imageTrack == 7);
    for (int i = 0; i < registered; i++)
        if (!strcmp(materials[i].name, name))
            return &materials[i];
    materials[registered].name = name;
    return &materials[registered++];
}
'''
select_checks = r'''
int main(void) {
    static char empty[] = "", springfield[] = "viewmodel_springfield", garand[] = "viewmodel_m1garand",
                thompson[] = "viewmodel_thompson", binoculars[] = "viewmodel_binoculars", kar98k[] = "viewmodel_kar98k",
                bren[] = "viewmodel_bren", american[] = "scope_overlay_american", german[] = "scope_overlay_german",
                idle[] = "viewmodel_m1garand_idle", kar98kIdle[] = "viewmodel_kar98_idle",
                kar98kRechamber[] = "viewmodel_kar98_ads_rechamber", scoped[] = "viewmodel_scoped",
                mosin[] = "viewmodel_mosinnagant", mosinIdle[] = "viewmodel_mosin_idle",
                mosinRechamber[] = "viewmodel_mosin_ads_rechamber", russian[] = "scope_overlay_russian";
    static struct Material own = { "own" };
    const playerState_t none = { 0 }, optic = { ATT_BIT(ATT_OPTIC4X) }, reflex = { ATT_BIT(ATT_REFLEX) }, all = { ~0u };
    const CgWwiiScopeGlass *glass;
    int cases = 0;

    memset(s_glassIndex, CG_WWIISCOPE_UNKNOWN, sizeof(s_glassIndex));
    memset(s_sightKnown, CG_WWIISCOPE_UNKNOWN, sizeof(s_sightKnown));
    for (int w = 0; w <= WEAPONS; w++)
        defs[w].szGunXModel = defs[w].szOverlayMaterial = empty;
    /* 1 the scoped Springfield; 2 the Garand, its rechamber its idle pose; 3 the Thompson, with no 4x Optic
     * model; 4 binoculars; 5 the Kar98k, its glass file broken; 6 the Bren, its optic bringing no overlay; 7 a
     * scope of its own the table lacks; 8 the Mosin-Nagant, its bolt worked in a scoped rechamber anim. */
    defs[1].szGunXModel = springfield;
    defs[1].szOverlayMaterial = american;
    defs[1].overlayReticle = 3;
    weaponInfos[1].hADSOverlay = &own;
    defs[2].szGunXModel = garand;
    defs[2].szXAnims[1] = idle;
    nations[2] = american;
    defs[3].szGunXModel = thompson;
    nations[3] = american;
    defs[4].szGunXModel = binoculars;
    defs[4].weapClass = 9;
    defs[4].szOverlayMaterial = american;
    defs[4].overlayReticle = 1;
    defs[5].szGunXModel = kar98k;
    defs[5].szXAnims[1] = kar98kIdle;
    defs[5].szXAnims[20] = kar98kRechamber;
    nations[5] = german;
    defs[6].szGunXModel = bren;
    defs[7].szGunXModel = scoped;
    defs[7].szOverlayMaterial = german;
    defs[7].overlayReticle = 2;
    defs[8].szGunXModel = mosin;
    defs[8].szXAnims[1] = mosinIdle;
    defs[8].szXAnims[20] = mosinRechamber;
    nations[8] = russian;

    /* A scoped rifle has its own glass, with or without the optic, and reads no file. */
    assert(CG_WwiiScopePlayerGlass(&none, 1) == &cg_wwiiScopeGlasses[1]);
    assert(CG_WwiiScopePlayerGlass(&optic, 1) == &cg_wwiiScopeGlasses[1] && reads == 0);
    /* Binoculars keep their own overlay. */
    assert(!CG_WwiiScopeWeaponGlass(4));
    cases++;
    /* The Garand has none without the optic or with another sight, and reads none. */
    assert(!CG_WwiiScopePlayerGlass(&none, 2) && !CG_WwiiScopePlayerGlass(&reflex, 2) && reads == 0);
    /* With it, the glass beside the optic's model, read once and freed. */
    glass = CG_WwiiScopePlayerGlass(&optic, 2);
    assert(glass && reads == 1 && frees == 1 && !strcmp(glass->gunModel, "viewmodel_m1garand"));
    assert(glass->rim[0] == -5.2107f && glass->rim[2] == 3.2472f && glass->radius == 0.578f);
    assert(glass->eye[0] == 6.3633f && glass->eye[2] == 1.0267f && glass->roll == 0.1268f);
    assert(CG_WwiiScopePlayerGlass(&optic, 2) == glass && reads == 1);
    /* The glass is the gun's, the choice each player's. */
    assert(!CG_WwiiScopePlayerGlass(&none, 2));
    /* A rechamber that is the idle pose is worked procedurally; the Kar98k's has an anim of its own. */
    assert(s_idleBolt[2] == 1);
    cases++;
    /* No 4x Optic model, no glass, looked for once; a broken one, none, freed and not read again. */
    assert(!CG_WwiiScopePlayerGlass(&optic, 3) && !CG_WwiiScopePlayerGlass(&optic, 3) && reads == 2 && frees == 1);
    assert(!CG_WwiiScopePlayerGlass(&optic, 5) && !CG_WwiiScopePlayerGlass(&optic, 5) && reads == 3 && frees == 2);
    assert(s_idleBolt[5] == 0);
    /* An optic that brought no overlay leaves the gun to the stock code, its glass unread; so does another
     * attachment on a scope the table lacks: only a 4x Optic's glass is read. */
    assert(!CG_WwiiScopePlayerGlass(&optic, 6) && !CG_WwiiScopePlayerGlass(&reflex, 7) && reads == 3);
    /* Nothing outside the weapons. */
    assert(!CG_WwiiScopePlayerGlass(&all, 0) && !CG_WwiiScopePlayerGlass(&all, -1));
    assert(!CG_WwiiScopePlayerGlass(&all, WEAPONS + 1) && !CG_WwiiScopePlayerGlass(&all, CG_WWIISCOPE_MAX_WEAPONS));
    cases++;
    /* The reticle is cut from the weapon's own overlay, or from the one its 4x Optic borrows. */
    assert(CG_WwiiScopeOverlay(&none, 1) == &own && CG_WwiiScopeOverlay(&optic, 1) == &own && registered == 0);
    MaterialHandle borrowed = CG_WwiiScopeOverlay(&optic, 2);
    assert(borrowed && !strcmp(borrowed->name, "scope_overlay_american") && registered == 1);
    cases++;
    /* Through its 4x Optic the Mosin-Nagant works its bolt in its idle pose, the scoped rechamber anim held
     * back until another anim plays; from the hip, without the optic or without the WWII scope its anims play
     * as they are. A scoped rifle's rechamber is its idle pose already. */
    assert(CG_WwiiScopeViewAnim(&optic, 8, 20) == 1 && s_heldBolt[8] == 1);
    assert(CG_WwiiScopeViewAnim(&optic, 8, 7) == 7 && s_heldBolt[8] == 0);
    assert(CG_WwiiScopeViewAnim(&optic, 8, 6) == 6 && s_heldBolt[8] == 0);
    assert(CG_WwiiScopeViewAnim(&none, 8, 20) == 20 && s_heldBolt[8] == 0);
    scopeOn = 0;
    assert(CG_WwiiScopeViewAnim(&optic, 8, 20) == 20 && s_heldBolt[8] == 0);
    scopeOn = 1;
    assert(CG_WwiiScopeViewAnim(&optic, 1, 20) == 20 && s_heldBolt[1] == 0);
    assert(CG_WwiiScopeViewAnim(&all, 0, 20) == 20 && CG_WwiiScopeViewAnim(&all, -1, 20) == 20);
    assert(CG_WwiiScopeViewAnim(&all, CG_WWIISCOPE_MAX_WEAPONS, 20) == 20);
    cases++;
    /* A Reflex or Lens Sight's window: its opening, read from beside the
     * sight's model once per gun and sight, and how many times the opening
     * the sight's ring reaches; none for any other sight or a gun without the
     * file, which is not looked for again. */
    {
        const playerState_t lensSight = { ATT_BIT(ATT_LENS) };
        float ring = 0.0f;
        int before = reads;

        glass = CG_WwiiScopeWindowGlass(&reflex, 2, &ring);
        assert(glass && ring == 1.15625f && glass->radius == 0.64f && glass->eye[0] == 8.0f && reads == before + 1);
        assert(!strcmp(glass->gunModel, "viewmodel_m1garand") && frees == reads - 1);
        assert(CG_WwiiScopeWindowGlass(&reflex, 2, &ring) == glass && reads == before + 1);
        glass = CG_WwiiScopeWindowGlass(&lensSight, 2, &ring);
        assert(glass && ring == 1.2f && glass->radius == 0.5f && reads == before + 2);
        assert(!CG_WwiiScopeWindowGlass(&none, 2, &ring) && !CG_WwiiScopeWindowGlass(&optic, 2, &ring));
        assert(!CG_WwiiScopeWindowGlass(&reflex, 3, &ring) && !CG_WwiiScopeWindowGlass(&reflex, 3, &ring));
        assert(reads == before + 3);
        assert(!CG_WwiiScopeWindowGlass(&all, 0, &ring) && !CG_WwiiScopeWindowGlass(&all, -1, &ring));
        assert(!CG_WwiiScopeWindowGlass(&all, WEAPONS + 1, &ring));
        assert(!CG_WwiiScopeWindowGlass(&all, CG_WWIISCOPE_MAX_WEAPONS, &ring));
    }
    cases++;
    /* The opening in a 1000 by 500 screen's pixels: the window's centre, and
     * a matrix taking the ends of its half-axes, scaled to the opening, to
     * the unit circle's, for a level window and a canted, stretched one. A
     * window without area has none. */
    {
#define CLOSE(a, b) (fabsf((a) - (b)) <= 1e-4f * (1.0f + fabsf(b)))
        static const float axesCases[2][4] = { { 0.2f, 0.0f, 0.0f, 0.3f }, { 0.15f, 0.05f, -0.04f, 0.25f } };
        CgWwiiScopeLens window;
        float centre[2], toUnit[4];

        for (int k = 0; k < 2; k++) {
            float a[2], b[2];

            memset(&window, 0, sizeof(window));
            window.center[0] = 0.1f;
            window.center[1] = -0.2f;
            window.axes[0][0] = axesCases[k][0];
            window.axes[0][1] = axesCases[k][1];
            window.axes[1][0] = axesCases[k][2];
            window.axes[1][1] = axesCases[k][3];
            assert(CG_WwiiScopeOpeningPixels(&window, 0.8f, 1000.0f, 500.0f, centre, toUnit));
            assert(CLOSE(centre[0], 550.0f) && CLOSE(centre[1], 300.0f));
            a[0] = axesCases[k][0] * 500.0f * 0.8f;
            a[1] = -axesCases[k][1] * 250.0f * 0.8f;
            b[0] = axesCases[k][2] * 500.0f * 0.8f;
            b[1] = -axesCases[k][3] * 250.0f * 0.8f;
            assert(CLOSE(toUnit[0] * a[0] + toUnit[1] * a[1], 1.0f) && fabsf(toUnit[2] * a[0] + toUnit[3] * a[1]) < 1e-4f);
            assert(fabsf(toUnit[0] * b[0] + toUnit[1] * b[1]) < 1e-4f && CLOSE(toUnit[2] * b[0] + toUnit[3] * b[1], 1.0f));
        }
        memset(window.axes, 0, sizeof(window.axes));
        assert(!CG_WwiiScopeOpeningPixels(&window, 0.8f, 1000.0f, 500.0f, centre, toUnit));
    }
    cases++;
    printf("%d\n", cases);
    return 0;
}
'''
select_mutants = [
    ('!(BG_GetWeaponAttachments(ps, weapon) & ATT_BIT(ATT_OPTIC4X))', '!BG_GetWeaponAttachments(ps, weapon)'),
    ('    if (!weapDef || !weapDef->overlayReticle || !weapDef->szOverlayMaterial[0])\n        return NULL;\n', ''),
    ('        s_sightKnown[weapon][sight] = CG_WWIISCOPE_NONE;\n', ''),
    ('            if (file)\n                FS_FreeFile(file);\n', ''),
    ('glass || weapon <= 0 || weapon >= CG_WWIISCOPE_MAX_WEAPONS || weapon > BG_GetNumWeapons() ||', 'glass ||'),
    ('s_idleBolt[weapon] = weapDef && (', 's_idleBolt[weapon] = glass && ('),
    ('    if (weapDef->szOverlayMaterial != BG_GetWeaponDef(weapon)->szOverlayMaterial)\n'
     '        return CL_RegisterMaterialNoMip(weapDef->szOverlayMaterial, 7);\n', ''),
    ('                glass->gunModel = weapDef->szGunXModel;\n', ''),
    ('    *ring = s_sightRings[sight];\n', '    *ring = 1.0f;\n'),
    ('mask & ATT_BIT(ATT_LENS)   ? CG_WWIISCOPE_SIGHT_LENS', 'mask & ATT_BIT(ATT_LENS)   ? CG_WWIISCOPE_SIGHT_REFLEX'),
    ('    if (weapon <= 0 || weapon >= CG_WWIISCOPE_MAX_WEAPONS || weapon > BG_GetNumWeapons())\n        return NULL;\n'
     '    mask = BG_GetWeaponAttachments(ps, weapon);\n',
     '    if (weapon < 0)\n        return NULL;\n    mask = BG_GetWeaponAttachments(ps, weapon);\n'),
    ('    a[1] = -window->axes[0][1] * 0.5f * height * opening;', '    a[1] = window->axes[0][1] * 0.5f * height * opening;'),
    ('    centre[1] = (1.0f - window->center[1]) * 0.5f * height;', '    centre[1] = (1.0f + window->center[1]) * 0.5f * height;'),
    ('    toUnit[1] = -b[0] / det;', '    toUnit[1] = b[0] / det;'),
    ('    b[0] = window->axes[1][0] * 0.5f * width * opening;', '    b[0] = window->axes[1][0] * 0.5f * width;'),
    ('    if (!(fabsf(det) > 1e-6f))\n        return 0;\n    toUnit', '    toUnit'),
    ('"glass/%s__%s"', '"glass/%s_%s"'),
    ('        anim == CG_WWIISCOPE_ADS_RECHAMBER_ANIM && CG_WwiiScopeGlassFor', '        CG_WwiiScopeGlassFor'),
    (' && !s_idleBolt[weapon];', ';'),
    ('    return s_heldBolt[weapon] ? CG_WWIISCOPE_IDLE_ANIM : anim;', '    return anim;'),
    ('    if (weapon <= 0 || weapon >= CG_WWIISCOPE_MAX_WEAPONS)\n        return anim;\n', ''),
]

strict = ['-Wall', '-Werror', '-Wno-unused-function']
with tempfile.TemporaryDirectory(prefix='cod2-wwii-scope-') as directory:
    run(directory, 'zoom', zoom_support + zoom_body + zoom_checks, strict, True)
    for index, (before, after) in enumerate(zoom_mutants):
        run(directory, f'zoom{index}', zoom_support + mutate(zoom_body, before, after) + zoom_checks, ['-w'], False)
    select_cases = int(run(directory, 'select', select_support + select_body + select_checks, strict, True).stdout)
    for index, (before, after) in enumerate(select_mutants):
        run(directory, f'select{index}', select_support + mutate(select_body, before, after) + select_checks, ['-w'],
            False)
    math_cases = int(run(directory, 'scope', math_support + math_body + math_checks, strict, True).stdout)
    for index, (before, after) in enumerate(math_mutants):
        if before.startswith('static const float s_rechamberKeep'):
            support = mutate(math_support, before, after)
            run(directory, f'scope{index}', support + math_body + math_checks, ['-w'], False)
        else:
            run(directory, f'scope{index}', math_support + mutate(math_body, before, after) + math_checks, ['-w'], False)
    paths = []
    for country, data in overlays:
        path = Path(directory) / f'scope_overlay_{country}.iwi'
        path.write_bytes(data)
        paths.append(str(path))
    lens_output = run(directory, 'lens', lens_support + lens_body + lens_checks, strict, True, paths).stdout
    lens_cases = int(lens_output.split()[-1])
    for index, (before, after) in enumerate(lens_mutants):
        run(directory, f'lens{index}', lens_support + mutate(lens_body, before, after) + lens_checks, ['-w'], False)

print(f'PASS: the WWII scope\'s hooks in the view weapon, reticle, zoom and start-up; {math_cases} cases of its glass '
      f'outline, rifle projection, zoom, blackout, centring, bolt and reticle math, the rifle\'s pose held by its '
      f'fire and rechamber anims\' weights past their states; {lens_cases} cases of lens images decoded, halved by '
      f'alpha and cleared outside their edge; the zoom\'s share of the raise as the attachments leave it; '
      f'{select_cases} cases of the glass a player\'s weapon is drawn with, a 4x Optic\'s read once beside its model, '
      f'the overlay its reticle is cut from, the bolt worked in the idle pose through it, a Reflex or Lens Sight\'s '
      f'window and its opening in pixels; '
      f'{len(math_mutants) + len(lens_mutants) + len(zoom_mutants) + len(select_mutants)} mutants fail; {asset_note}')
