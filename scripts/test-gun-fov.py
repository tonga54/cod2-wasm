#!/usr/bin/env python3
"""The arms and weapon's own field of view and hip pose (cg_gunfov_mp.c).

Checks the hooks that wire it into the view weapon, its effects and tracer,
the WWII scope, the cgame's start-up and the scene compositor; runs its
projection, hip offsets and their easing, the remap of points on the weapon
and the conditions it draws under, under ASan and UBSan, with mutants that
must fail; and runs the native build's stand-ins.
"""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = root / 'src/PC/cgame_mp/cg_gunfov_mp.c'
gunfov = source.read_text()
header = (root / 'src/headers/PC/cgame_mp/cg_gunfov.h').read_text()
weapons = (root / 'src/PC/cgame_mp/cg_weapons.c').read_text()
types = (root / 'src/headers/common_types.h').read_text()


def function(text, name):
    match = re.search(r'^[^\n]*\b' + re.escape(name) + r'\([^;{]*?\)\s*\{', text, re.M)
    assert match, name
    end = text.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


# The hooks. A frame without a view weapon draws none with its own projection.
add_view = function(weapons, 'CG_AddViewWeapon')
assert add_view.index('    CG_WwiiScopeClear();\n    CG_GunFovClear();\n') < add_view.index('return;')
# The hip pose is read from the weapon with the player's attachments. Its turn
# joins the weapon's angles after the sprint's carry pose, which takes its
# place, and before a raised WWII scope gives up the rifle's turns and the
# angles become the weapon's axes.
assert ('    CG_GunFovHipPose(playerState, BG_GetPlayerWeaponDef(playerState, weaponIndex), hip);\n'
        '    for (int i = 0; i < 3; ++i)\n'
        '        weaponAngles[i] += hip[3 + i] * (1.0f - sprintBlend);\n'
        '    CG_WwiiScopeAdjustViewWeapon(playerState, weaponIndex, weaponAngles);\n') in add_view
assert add_view.index('sprintBlend = CG_SprintCarryBlend(playerState);') < add_view.index('CG_GunFovHipPose(') < \
    add_view.index('AnglesToAxis(weaponAngles, weaponAxis);')
assert add_view.count('CG_GunFovHipPose(') == 1 and 'float hip[CG_GUNFOV_HIP_POSE];' in add_view
# Its offset joins the weapon's own after the stock offsets and before the
# WWII scope centres a raised rifle.
assert ('        /* The weapon\'s right is the axis\'s left negated. */\n'
        '        CG_Madd(hand.origin, finalAxis[0], viewDef->vGunOfs[0] + hip[0]);\n'
        '        CG_Madd(hand.origin, finalAxis[1], -viewDef->vGunOfs[1] + hip[1]);\n'
        '        CG_Madd(hand.origin, finalAxis[2], viewDef->vGunOfs[2] + hip[2]);\n') in add_view
assert add_view.index('CG_Madd(hand.origin, finalAxis[2], cg->swayOffset[2] + CG_DvarValue(imp_cg_gun_z));') < \
    add_view.index('viewDef->vGunOfs[0] + hip[0]') < \
    add_view.index('CG_WwiiScopePlaceViewWeapon(playerState, weaponIndex, hand.origin);')
# The projection is taken before the weapon and its effects are added, and a
# raised scope's takes its place once the scope has placed the rifle, before
# the shooter's tracer leaves.
assert add_view.index('CG_GunFovUpdate(playerState, drawGun);') < \
    add_view.index('CG_AddPlayerWeapon(&hand, playerState, &cg->predictedPlayerEntity, drawGun);') < \
    add_view.index('CG_WwiiScopeUpdate(playerState, weaponIndex, drawGun);') < \
    add_view.index('CG_GunFovFollowScope(CG_WwiiScopeLens()->projection);') < \
    add_view.index('CG_UpdateWebWeaponPresentation(playerState, weaponIndex, drawGun);')
# The tracer leaves from the muzzle where the view draws it, and effects
# bolted to the view weapon (entities from 1024 on) from their tag where the
# view draws it, whether or not they name a bone.
presentation = function(weapons, 'CG_UpdateWebWeaponPresentation')
assert '                    muzzle[i] += 6.0f * cg->viewModelAxis[0][i];\n                CG_GunFovPoint(muzzle);\n' in presentation
bolt = function((root / 'src/PC/EffectsCore/FxUtil.c').read_text(), 'FX_GetBoneOrientation')
assert bolt.count('if (entityNum >= 0x400)\n') == 2 and bolt.count('CG_GunFovPoint(orient->origin);') == 2
assert bolt.index('        *orient = parent;\n        if (entityNum >= 0x400)\n            CG_GunFovPoint(orient->origin);\n'
                  '        return 1;\n') < bolt.index('MatrixTransformVector43(mtx->trans, parentMatrix, orient->origin);') < \
    bolt.rindex('CG_GunFovPoint(orient->origin);')
# A WWII scope's rifle narrows from the weapon's angle as it is raised.
scope = (root / 'src/PC/cgame_mp/cg_wwiiscope_mp.c').read_text()
assert '    hipFov = CG_GunFovHipAngle(hipFov);\n    CG_WwiiScopeProjectionScales(hipFov,' in scope
# The cgame registers the dvars at start-up and the renderer hands the
# projection to the compositor with the scope's, each scene.
assert 'CG_ScopeGlintInit();\n    CG_GunFovInit();\n' in (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()
scene = (root / 'src/PC/gfx_d3d/r_scene.c').read_text()
assert scene.index('WebSceneFx_SetScope(g_webScopeLens);') < scene.index('WebSceneFx_SetGunProjection(g_webGunProjection);')

stub_types = r'''
#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H
typedef float vec_t;
typedef vec_t vec3_t[3];
typedef int qboolean;
typedef struct { struct { float value; } current; } dvar_t;
typedef struct { float fWeaponPosFrac; } playerState_t;
''' + re.search(r'enum weapClass_t \{.*?\};\n', types, re.S)[0] + r'''
typedef struct { int weapClass; char *szOverlayMaterial; int overlayReticle; } WeaponDef;
typedef struct { unsigned int width, height; } vidConfig_t;
typedef struct { vidConfig_t vidConfig; } clientStatic_t;
typedef struct { int x, y, width, height; float fov_x, fov_y; vec3_t vieworg; vec3_t viewaxis[3]; } refdef_t;
typedef struct { int frametime; qboolean renderingThirdPerson; refdef_t refdef; } cg_t;
#endif
'''

checks = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "SOURCE"
static dvar_t dvars[2];
static const char *dvarNames[2];
static int registrations;
const dvar_t *Dvar_RegisterFloat(const char *name, float value, float min, float max, int flags)
{
    assert(registrations < 2 && min == 0.0f && flags == 0x1001);
    dvarNames[registrations] = name;
    dvars[registrations].current.value = value;
    (void)max;
    return &dvars[registrations++];
}
static int scopeAvailable = 1;
int WebSceneFx_ScopeAvailable(void)
{
    return scopeAvailable;
}
static cg_t cgState;
cg_t *cg = &cgState;
static clientStatic_t clsState;
void *imp_cls = &clsState;

static int near(float a, float b, float tolerance)
{
    return fabsf(a - b) <= tolerance;
}
/* The tables' poses: forward, left, up, then pitch, yaw, roll. */
static const float LONG_GUN[6] = {-3.0f, -1.5f, 0.75f, 2.0f, -2.0f, -4.0f};
static const float SCOPED[6] = {3.0f, -0.75f, 1.5f, 8.0f, -6.0f, -12.0f};
static const float PISTOL[6] = {-3.5f, 3.25f, 1.0f, 0.0f, 3.0f, 0.0f};
static const float NONE[6];
/* a is want scaled by scale, with a NaN or infinity anywhere failing. */
static int pose(const float *a, const float *want, float scale, float tolerance)
{
    for (int i = 0; i < 6; ++i)
        if (!isfinite(a[i]) || !near(a[i], want[i] * scale, tolerance))
            return 0;
    return 1;
}
/* a is from + (to - from) * t, scaled by scale. */
static int between(const float *a, const float *from, const float *to, float t, float scale, float tolerance)
{
    for (int i = 0; i < 6; ++i)
        if (!near(a[i], (from[i] + (to[i] - from[i]) * t) * scale, tolerance))
            return 0;
    return 1;
}
#define HALF_DEGREE 0.00872664626f
/* CG_CalcFov's fields of view for a cg_fov on a screen of this aspect. */
static void viewFov(float cgFov, float aspect, float *x, float *y)
{
    float tanY = tanf(cgFov * HALF_DEGREE) * 0.75f;
    *y = atanf(tanY) / HALF_DEGREE;
    *x = atanf(tanY * aspect) / HALF_DEGREE;
}
/* A view turned by yaw and pitch at origin, as the refdef holds it. */
static void view(float yaw, float pitch, float x, float y, float z)
{
    const float f[3] = {cosf(pitch) * cosf(yaw), cosf(pitch) * sinf(yaw), -sinf(pitch)};
    const float l[3] = {-sinf(yaw), cosf(yaw), 0};
    const float u[3] = {sinf(pitch) * cosf(yaw), sinf(pitch) * sinf(yaw), cosf(pitch)};
    for (int i = 0; i < 3; ++i) {
        cgState.refdef.viewaxis[0][i] = f[i];
        cgState.refdef.viewaxis[1][i] = l[i];
        cgState.refdef.viewaxis[2][i] = u[i];
    }
    cgState.refdef.vieworg[0] = x;
    cgState.refdef.vieworg[1] = y;
    cgState.refdef.vieworg[2] = z;
}
/* A point's normalized device coordinates through projection scales sx and
 * sy, from the refdef's eye and axes. */
static void ndc(const float *point, float sx, float sy, float *out)
{
    float d[3], forward = 0, left = 0, up = 0;
    for (int i = 0; i < 3; ++i) {
        d[i] = point[i] - cgState.refdef.vieworg[i];
        forward += d[i] * cgState.refdef.viewaxis[0][i];
        left += d[i] * cgState.refdef.viewaxis[1][i];
        up += d[i] * cgState.refdef.viewaxis[2][i];
    }
    out[0] = -sx * left / forward;
    out[1] = sy * up / forward;
}
/* A frame in which everything lets the weapon take its own projection. */
static void composable(void)
{
    cgState.refdef.x = cgState.refdef.y = 0;
    cgState.refdef.width = 1280;
    cgState.refdef.height = 720;
    clsState.vidConfig.width = 1280;
    clsState.vidConfig.height = 720;
    cgState.renderingThirdPerson = 0;
    scopeAvailable = 1;
}

int main(void)
{
    float fx, fy, p[2], q[2], spread, viewX, viewY, gunY;
    playerState_t ps = {0};

    /* Registered once at start-up, archived: WWII's 65 and the full hip pose. */
    CG_GunFovInit();
    assert(registrations == 2 && !strcmp(dvarNames[0], "cg_gunFov") && !strcmp(dvarNames[1], "cg_gunHip"));
    assert(cg_gunFov == &dvars[0] && cg_gunFov->current.value == 65.0f && cg_gunHip->current.value == 1.0f);

    /* At the hip, cg_fov 80 on a 16:9 screen: the weapon's vertical half
     * angle is a cg_fov-style 65's, its aspect the view's, and it spreads a
     * point a third further from the view axis than the view does. */
    viewFov(80.0f, 16.0f / 9.0f, &fx, &fy);
    viewX = tanf(fx * HALF_DEGREE);
    viewY = tanf(fy * HALF_DEGREE);
    gunY = tanf(65.0f * HALF_DEGREE) * 0.75f;
    spread = CG_GunFovProjection(65.0f, fx, fy, 0.0f, p);
    assert(near(p[1], 1.0f / gunY, 1e-5f) && near(p[1] / p[0], viewX / viewY, 1e-5f));
    assert(near(spread, viewY / gunY, 1e-5f) && spread > 1.31f && spread < 1.33f);
    /* A 4:3 screen keeps the same vertical spread. */
    {
        float x43, y43;
        viewFov(80.0f, 4.0f / 3.0f, &x43, &y43);
        assert(near(CG_GunFovProjection(65.0f, x43, y43, 0.0f, q), spread, 1e-5f) && near(q[1], p[1], 1e-5f));
    }
    /* Raising the sights narrows the spread without a step to the view's own
     * by the time a sight's window opens, 0.6 of the raise, along a
     * smoothstep of the tangent. */
    {
        float last = spread;
        for (int i = 1; i <= 70; ++i) {
            float s = CG_GunFovProjection(65.0f, fx, fy, (float)i * 0.01f, q);
            assert(s >= 1.0f && s <= last + 1e-6f && last - s < 0.02f);
            last = s;
        }
        assert(CG_GunFovProjection(65.0f, fx, fy, 0.6f, q) == 1.0f && q[0] == 0.0f && q[1] == 0.0f);
        assert(CG_GunFovProjection(65.0f, fx, fy, 1.0f, q) == 1.0f && q[0] == 0.0f && q[1] == 0.0f);
        CG_GunFovProjection(65.0f, fx, fy, 0.3f, q);
        assert(near(1.0f / q[1], 0.5f * (gunY + viewY), 1e-5f));
        CG_GunFovProjection(65.0f, fx, fy, 0.15f, q);
        assert(near(1.0f / q[1], gunY + (viewY - gunY) * 0.15625f, 1e-5f));
        assert(CG_GunFovProjection(65.0f, fx, fy, -1.0f, q) == spread && q[1] == p[1]);
    }
    /* cg_gunFov 0, or an angle or view that is no field of view, draws with
     * the view's own; a weapon angle wider than the view's draws it smaller. */
    {
        const float badGun[] = {0.0f, 0.5f, -10.0f, NAN, INFINITY, 179.0f, 200.0f};
        const float badView[] = {0.0f, -5.0f, NAN, INFINITY, 179.0f};
        for (unsigned i = 0; i < sizeof(badGun) / sizeof(badGun[0]); ++i) {
            q[0] = q[1] = 7.0f;
            assert(CG_GunFovProjection(badGun[i], fx, fy, 0.0f, q) == 1.0f && q[0] == 0.0f && q[1] == 0.0f);
        }
        for (unsigned i = 0; i < sizeof(badView) / sizeof(badView[0]); ++i) {
            assert(CG_GunFovProjection(65.0f, badView[i], fy, 0.0f, q) == 1.0f && q[0] == 0.0f && q[1] == 0.0f);
            assert(CG_GunFovProjection(65.0f, fx, badView[i], 0.0f, q) == 1.0f && q[0] == 0.0f && q[1] == 0.0f);
        }
        assert(CG_GunFovProjection(1.0f, fx, fy, 0.0f, q) > 50.0f);
        assert(CG_GunFovProjection(100.0f, fx, fy, 0.0f, q) < 1.0f && q[1] > 0.0f);
    }

    /* Hip targets, offsets in the weapon's forward, left and up then turns in
     * degrees: a long gun nearer the eye and to the right, turned a little in,
     * nose down and canted left; a scoped one further out, turned in, down and
     * canted left further; a pistol in the middle, pointed ahead; each a
     * little up; nothing for the rest. */
    {
        const int longGuns[] = {WEAPCLASS_RIFLE, WEAPCLASS_MG, WEAPCLASS_SMG, WEAPCLASS_SPREAD, WEAPCLASS_ROCKETLAUNCHER};
        const int others[] = {WEAPCLASS_GRENADE, WEAPCLASS_TURRET, WEAPCLASS_NON_PLAYER, WEAPCLASS_ITEM, WEAPCLASS_NUM, -1, 1000};
        const float badScale[] = {0.0f, -1.0f, NAN, INFINITY, -INFINITY};
        float h[CG_GUNFOV_HIP_POSE + 1];
        assert(CG_GUNFOV_HIP_POSE == 6);
        for (unsigned i = 0; i < sizeof(longGuns) / sizeof(longGuns[0]); ++i) {
            CG_GunFovHipTarget(longGuns[i], 0, 1.0f, h);
            assert(pose(h, LONG_GUN, 1.0f, 0.0f));
            CG_GunFovHipTarget(longGuns[i], 1, 1.0f, h);
            assert(pose(h, SCOPED, 1.0f, 0.0f));
        }
        for (int scoped = 0; scoped < 2; ++scoped) {
            CG_GunFovHipTarget(WEAPCLASS_PISTOL, scoped, 1.0f, h);
            assert(pose(h, PISTOL, 1.0f, 0.0f));
            for (unsigned i = 0; i < sizeof(others) / sizeof(others[0]); ++i) {
                for (int k = 0; k < 6; ++k)
                    h[k] = 9.0f;
                CG_GunFovHipTarget(others[i], scoped, 1.0f, h);
                assert(pose(h, NONE, 1.0f, 0.0f));
            }
        }
        /* Only the pose is written. */
        h[6] = 7.0f;
        CG_GunFovHipTarget(WEAPCLASS_RIFLE, 1, 1.0f, h);
        assert(h[6] == 7.0f);
        /* cg_gunHip scales the whole pose, to the full pose at most. */
        CG_GunFovHipTarget(WEAPCLASS_SMG, 0, 0.5f, h);
        assert(pose(h, LONG_GUN, 0.5f, 0.0f));
        CG_GunFovHipTarget(WEAPCLASS_SMG, 0, 2.0f, h);
        assert(pose(h, LONG_GUN, 1.0f, 0.0f));
        for (unsigned i = 0; i < sizeof(badScale) / sizeof(badScale[0]); ++i) {
            for (int k = 0; k < 6; ++k)
                h[k] = 9.0f;
            CG_GunFovHipTarget(WEAPCLASS_RIFLE, 1, badScale[i], h);
            assert(pose(h, NONE, 1.0f, 0.0f));
        }
    }
    /* The share of the hip pose the raise leaves: all at the hip, half
     * halfway to the window, none once it opens. */
    assert(CG_GunFovHipShare(0.0f) == 1.0f && CG_GunFovHipShare(-1.0f) == 1.0f && CG_GunFovHipShare(NAN) == 1.0f);
    assert(near(CG_GunFovHipShare(0.3f), 0.5f, 1e-6f) && CG_GunFovHipShare(0.6f) == 0.0f && CG_GunFovHipShare(1.0f) == 0.0f);

    /* The hip pose the view weapon takes each frame. */
    {
        WeaponDef rifle = {WEAPCLASS_RIFLE, "", 0}, sniper = {WEAPCLASS_RIFLE, "scope_overlay_kar98k", 1};
        WeaponDef pistol = {WEAPCLASS_PISTOL, "", 0};
        WeaponDef notScoped[] = {{WEAPCLASS_RIFLE, "", 1}, {WEAPCLASS_RIFLE, "scope_overlay_kar98k", 0},
                                 {WEAPCLASS_RIFLE, NULL, 1}};
        const float ease = 1.0f - expf(-1.0f);
        float h[CG_GUNFOV_HIP_POSE];

        /* The first weapon is held where it belongs at once. */
        cgState.frametime = 16;
        CG_GunFovHipPose(&ps, &sniper, h);
        assert(pose(h, SCOPED, 1.0f, 0.0f));
        /* Another eases there over about a tenth of a second, turns with the
         * offsets: a 100 ms frame covers 1 - 1/e of the way; a frame without
         * time, or a step back in time, moves nothing. */
        cgState.frametime = 100;
        CG_GunFovHipPose(&ps, &rifle, h);
        assert(between(h, SCOPED, LONG_GUN, ease, 1.0f, 1e-5f));
        cgState.frametime = 0;
        CG_GunFovHipPose(&ps, &rifle, h);
        CG_GunFovHipPose(&ps, &rifle, h);
        assert(between(h, SCOPED, LONG_GUN, ease, 1.0f, 1e-5f));
        cgState.frametime = -16;
        CG_GunFovHipPose(&ps, &rifle, h);
        assert(between(h, SCOPED, LONG_GUN, ease, 1.0f, 1e-5f));
        cgState.frametime = 16;
        for (int i = 0; i < 100; ++i)
            CG_GunFovHipPose(&ps, &rifle, h);
        assert(pose(h, LONG_GUN, 1.0f, 1e-4f));
        /* Raising the sights takes the pose away by the time a sight's window
         * opens and lowering them brings it back, still eased. */
        ps.fWeaponPosFrac = 0.3f;
        CG_GunFovHipPose(&ps, &rifle, h);
        assert(pose(h, LONG_GUN, 0.5f, 1e-4f));
        ps.fWeaponPosFrac = 0.6f;
        CG_GunFovHipPose(&ps, &rifle, h);
        assert(pose(h, NONE, 1.0f, 0.0f));
        ps.fWeaponPosFrac = 0.0f;
        CG_GunFovHipPose(&ps, &rifle, h);
        assert(pose(h, LONG_GUN, 1.0f, 1e-4f));
        /* A pistol, and a scope only with both its overlay and material. */
        for (int i = 0; i < 100; ++i)
            CG_GunFovHipPose(&ps, &pistol, h);
        assert(pose(h, PISTOL, 1.0f, 1e-4f));
        for (int k = 0; k < 3; ++k) {
            for (int i = 0; i < 100; ++i)
                CG_GunFovHipPose(&ps, &sniper, h);
            assert(pose(h, SCOPED, 1.0f, 1e-4f));
            for (int i = 0; i < 100; ++i)
                CG_GunFovHipPose(&ps, &notScoped[k], h);
            assert(pose(h, LONG_GUN, 1.0f, 1e-4f));
        }
        /* No weapon, or cg_gunHip 0, leaves the viewmodel as it stands. */
        for (int i = 0; i < 100; ++i)
            CG_GunFovHipPose(&ps, NULL, h);
        assert(pose(h, NONE, 1.0f, 1e-4f));
        dvars[1].current.value = 0.0f;
        for (int i = 0; i < 100; ++i)
            CG_GunFovHipPose(&ps, &rifle, h);
        assert(pose(h, NONE, 1.0f, 1e-4f));
        dvars[1].current.value = 1.0f;
        /* A new map's first weapon is held where it belongs at once again. */
        registrations = 0;
        CG_GunFovInit();
        CG_GunFovHipPose(&ps, &pistol, h);
        assert(pose(h, PISTOL, 1.0f, 1e-6f));
    }

    /* Each frame: its own projection only with a view weapon, in first
     * person, in a composed view that fills the screen. */
    view(0.7f, -0.2f, 100.0f, -50.0f, 60.0f);
    cgState.refdef.fov_x = fx;
    cgState.refdef.fov_y = fy;
    composable();
    ps.fWeaponPosFrac = 0.0f;
    CG_GunFovUpdate(&ps, 1);
    assert(g_webGunProjection[0] == p[0] && g_webGunProjection[1] == p[1] && s_spread == spread);
    /* A point on the weapon drawn with the weapon's projection lands where
     * the view's draws its remap, and its depth along the view stays. */
    {
        float point[3], moved[3], a[2], b[2], depth = 0.0f, movedDepth = 0.0f;
        for (int i = 0; i < 3; ++i) {
            point[i] = cgState.refdef.vieworg[i] + 20.0f * cgState.refdef.viewaxis[0][i] -
                       4.0f * cgState.refdef.viewaxis[1][i] - 6.0f * cgState.refdef.viewaxis[2][i];
            moved[i] = point[i];
        }
        CG_GunFovPoint(moved);
        ndc(point, p[0], p[1], a);
        ndc(moved, 1.0f / viewX, 1.0f / viewY, b);
        assert(near(a[0], b[0], 1e-4f) && near(a[1], b[1], 1e-4f));
        for (int i = 0; i < 3; ++i) {
            depth += (point[i] - cgState.refdef.vieworg[i]) * cgState.refdef.viewaxis[0][i];
            movedDepth += (moved[i] - cgState.refdef.vieworg[i]) * cgState.refdef.viewaxis[0][i];
        }
        assert(near(depth, movedDepth, 1e-4f));
        /* A raised scope's own projection takes the weapon's place. */
        q[0] = 2.4f;
        q[1] = 4.2f;
        CG_GunFovFollowScope(q);
        assert(near(s_spread, 4.2f * viewY, 1e-5f));
        for (int i = 0; i < 3; ++i)
            moved[i] = point[i];
        CG_GunFovPoint(moved);
        ndc(point, q[0], q[1], a);
        ndc(moved, 1.0f / viewX, 1.0f / viewY, b);
        assert(near(a[1], b[1], 1e-4f));
        /* A scope without a projection of its own leaves it. */
        {
            const float none[3][2] = {{0.0f, 4.2f}, {2.4f, -1.0f}, {NAN, NAN}};
            for (int i = 0; i < 3; ++i) {
                CG_GunFovFollowScope(none[i]);
                assert(near(s_spread, 4.2f * viewY, 1e-5f));
            }
            CG_GunFovFollowScope(NULL);
            assert(near(s_spread, 4.2f * viewY, 1e-5f));
        }
        /* Without its own projection a point stays where it is. */
        CG_GunFovClear();
        assert(s_spread == 1.0f && g_webGunProjection[0] == 0.0f && g_webGunProjection[1] == 0.0f);
        for (int i = 0; i < 3; ++i)
            moved[i] = point[i];
        CG_GunFovPoint(moved);
        assert(!memcmp(moved, point, sizeof(moved)));
    }
    /* Any condition missing draws it with the view's projection. */
    for (int condition = 0; condition < 8; ++condition) {
        int drawGun = 1;
        composable();
        CG_GunFovUpdate(&ps, 1);
        assert(g_webGunProjection[1] > 0.0f && s_spread > 1.0f);
        switch (condition) {
        case 0: drawGun = 0; break;
        case 1: cgState.renderingThirdPerson = 1; break;
        case 2: cgState.refdef.x = 8; break;
        case 3: cgState.refdef.y = 8; break;
        case 4: cgState.refdef.width = 1272; break;
        case 5: cgState.refdef.height = 700; break;
        case 6: scopeAvailable = 0; break;
        case 7: dvars[0].current.value = 0.0f; break;
        }
        CG_GunFovUpdate(&ps, drawGun);
        assert(g_webGunProjection[0] == 0.0f && g_webGunProjection[1] == 0.0f && s_spread == 1.0f);
        dvars[0].current.value = 65.0f;
    }
    {
        const dvar_t *registered = cg_gunFov;
        composable();
        cg_gunFov = NULL;
        CG_GunFovUpdate(&ps, 1);
        assert(g_webGunProjection[1] == 0.0f && s_spread == 1.0f);
        cg_gunFov = registered;
    }
    /* Raised to a sight's window, the view's own. */
    composable();
    ps.fWeaponPosFrac = 0.6f;
    CG_GunFovUpdate(&ps, 1);
    assert(g_webGunProjection[1] == 0.0f && s_spread == 1.0f);

    /* A WWII scope's rifle narrows from the weapon's angle, else the view's. */
    assert(CG_GunFovHipAngle(80.0f) == 65.0f);
    dvars[0].current.value = 0.5f;
    assert(CG_GunFovHipAngle(80.0f) == 80.0f);
    dvars[0].current.value = 0.0f;
    assert(CG_GunFovHipAngle(80.0f) == 80.0f);
    dvars[0].current.value = 65.0f;
    {
        const dvar_t *registered = cg_gunFov;
        cg_gunFov = NULL;
        assert(CG_GunFovHipAngle(80.0f) == 80.0f);
        cg_gunFov = registered;
    }
    puts("PASS: the weapon's projection at a cg_fov-style 65 with the view's aspect, a third further spread than cg_fov 80's on 16:9 and 4:3, narrowing without a step to the view's own by 0.6 of the raise; cg_gunFov 0 and angles or views that are no field of view drawn with the view's; hip poses, offsets and turns, for long guns, scoped guns and pistols, scaled by cg_gunHip up to 1, leaving over the raise and eased over a tenth of a second between weapons; a scope only with both its overlay and material; points on the weapon remapped to where the view draws them, a raised scope's projection taking the weapon's place; its own projection only with a view weapon, in first person, in a composed full-screen view");
    return 0;
}
'''

native = r'''
#include <assert.h>
#include <stdio.h>
#include "SOURCE"
int main(void)
{
    float o[CG_GUNFOV_HIP_POSE] = {9, 9, 9, 9, 9, 9}, point[3] = {1, 2, 3}, p[2] = {2, 3};
    playerState_t ps = {0};
    WeaponDef rifle = {WEAPCLASS_RIFLE, "", 0};
    CG_GunFovInit();
    CG_GunFovUpdate(&ps, 1);
    CG_GunFovFollowScope(p);
    CG_GunFovClear();
    CG_GunFovHipPose(&ps, &rifle, o);
    for (int i = 0; i < CG_GUNFOV_HIP_POSE; ++i)
        assert(o[i] == 0.0f);
    CG_GunFovPoint(point);
    assert(point[0] == 1.0f && point[1] == 2.0f && point[2] == 3.0f);
    assert(CG_GunFovHipAngle(80.0f) == 80.0f);
    assert(CG_GunFovProjection(65.0f, 96.0f, 64.0f, 0.0f, p) > 1.0f);
    CG_GunFovHipTarget(WEAPCLASS_RIFLE, 0, 1.0f, o);
    assert(o[0] == -3.0f && o[5] == -4.0f);
    puts("PASS: the native build's stand-ins leave the viewmodel, its points and the scope's angle as they are");
    return 0;
}
'''

# Each mutant must fail the checks above.
mutants = [
    ('{-3.0f, -1.5f, 0.75f, 2.0f, -2.0f, -4.0f}', '{-3.0f, -2.0f, 0.75f, 2.0f, -2.0f, -4.0f}'),
    ('{-3.0f, -1.5f, 0.75f, 2.0f, -2.0f, -4.0f}', '{-3.0f, -1.5f, 0.75f, 2.0f, -2.0f, 0.0f}'),
    ('{3.0f, -0.75f, 1.5f, 8.0f, -6.0f, -12.0f}', '{-3.0f, -1.5f, 0.75f, 2.0f, -2.0f, -4.0f}'),
    ('{3.0f, -0.75f, 1.5f, 8.0f, -6.0f, -12.0f}', '{3.0f, -0.75f, 1.5f, 0.0f, -6.0f, -12.0f}'),
    ('{3.0f, -0.75f, 1.5f, 8.0f, -6.0f, -12.0f}', '{3.0f, -0.75f, 1.5f, 8.0f, 6.0f, -12.0f}'),
    ('{-3.5f, 3.25f, 1.0f, 0.0f, 3.0f, 0.0f}', '{-3.5f, 3.25f, 0.0f, 0.0f, 3.0f, 0.0f}'),
    ('{-3.5f, 3.25f, 1.0f, 0.0f, 3.0f, 0.0f}', '{-3.5f, 3.25f, 1.0f, 0.0f, 0.0f, 0.0f}'),
    ('    for (i = 0; i < CG_GUNFOV_HIP_POSE; i++)\n        pose[i] = hip ?',
     '    for (i = 0; i < 3; i++)\n        pose[i] = hip ?'),
    ('    for (i = 0; i < CG_GUNFOV_HIP_POSE; i++) {\n        s_hip[i] +=', '    for (i = 0; i < 3; i++) {\n        s_hip[i] +='),
    ('        hip = scoped ? s_hipScoped : s_hipLongGun;', '        hip = s_hipLongGun;'),
    ('        hip = s_hipPistol;', '        hip = scoped ? s_hipScoped : s_hipLongGun;'),
    ('hip[i] * (scale < 1.0f ? scale : 1.0f)', 'hip[i] * scale'),
    ('    if (!(scale > 0.0f) || !isfinite(scale))', '    if (!(scale > 0.0f))'),
    ('#define CG_GUNFOV_RAISED 0.6f', '#define CG_GUNFOV_RAISED 1.0f'),
    ('    return blend * blend * (3.0f - 2.0f * blend);', '    return blend;'),
    ('    projection[0] = viewY / (viewX * tanY);', '    projection[0] = 1.0f / tanY;'),
    ('    gun = tanf(gunFov * 0.00872664626f) * 0.75f;', '    gun = tanf(gunFov * 0.00872664626f);'),
    ('    return viewY / tanY;', '    return 1.0f;'),
    ('    if (!(gunFov >= 1.0f && gunFov < 179.0f) ||', '    if (!(gunFov > 0.0f) ||'),
    ('!(viewFovX > 0.0f && viewFovX < 179.0f) ||', '!(viewFovX > 0.0f) ||'),
    ('#define CG_GUNFOV_HIP_EASE 0.1f', '#define CG_GUNFOV_HIP_EASE 0.2f'),
    ('    if (s_hipHeld)\n', '    if (1)\n'),
    ('    s_hipHeld = 1;\n', ''),
    ('    s_hipHeld = 0;\n', ''),
    ('ease = cg->frametime > 0 ? 1.0f - expf(-0.001f * (float)cg->frametime / CG_GUNFOV_HIP_EASE) : 0.0f;',
     'ease = 1.0f - expf(-0.001f * (float)cg->frametime / CG_GUNFOV_HIP_EASE);'),
    ('        pose[i] = s_hip[i] * share;', '        pose[i] = s_hip[i];'),
    ('weapDef->overlayReticle && weapDef->szOverlayMaterial && weapDef->szOverlayMaterial[0]', 'weapDef->overlayReticle'),
    ('weapDef->overlayReticle && weapDef->szOverlayMaterial && weapDef->szOverlayMaterial[0]',
     'weapDef->szOverlayMaterial && weapDef->szOverlayMaterial[0]'),
    ('    if (!drawGun || cg->renderingThirdPerson ||', '    if (cg->renderingThirdPerson ||'),
    ('    if (!drawGun || cg->renderingThirdPerson ||', '    if (!drawGun ||'),
    (' || !cg_gunFov || !CG_GunFovComposed())', ' || !cg_gunFov)'),
    ('cg->refdef.height == (int)vid->height && WebSceneFx_ScopeAvailable();', 'cg->refdef.height == (int)vid->height;'),
    ('    return !cg->refdef.x && !cg->refdef.y && cg->refdef.width', '    return cg->refdef.width'),
    ('    CG_GunFovClear();\n    memcpy(s_eye,', '    memcpy(s_eye,'),
    ('    memcpy(s_eye, cg->refdef.vieworg, sizeof(s_eye));\n', ''),
    ('    memcpy(s_axis, cg->refdef.viewaxis, sizeof(s_axis));\n', ''),
    ('        s_spread = projection[1] * viewY;', '        (void)0;'),
    ('    if (projection && projection[0] > 0.0f && projection[1] > 0.0f && viewY > 0.0f && isfinite(viewY))',
     '    if (projection)'),
    ('s_spread * (left * s_axis[1][i] + up * s_axis[2][i])', '(left * s_axis[1][i] + up * s_axis[2][i])'),
    ('    return cg_gunFov && cg_gunFov->current.value >= 1.0f ? cg_gunFov->current.value : viewHipFov;',
     '    return cg_gunFov ? cg_gunFov->current.value : viewHipFov;'),
    ('    g_webGunProjection[0] = g_webGunProjection[1] = 0.0f;\n    s_spread = 1.0f;\n', '    s_spread = 1.0f;\n'),
    ('cg_gunFov = Dvar_RegisterFloat("cg_gunFov", 65.0f,', 'cg_gunFov = Dvar_RegisterFloat("cg_gunFov", 80.0f,'),
]


def build(path, name, text, sanitize):
    c = path / f'{name}.c'
    c.write_text(text)
    flags = ['-O1', '-g', '-Wall', '-Werror', '-fsanitize=address,undefined', '-fno-sanitize-recover=all'] \
        if sanitize else ['-O0', '-w']
    subprocess.run(['cc', '-std=c99', '-D__EMSCRIPTEN__', *flags, '-I', str(path), str(c), '-lm', '-o',
                    str(path / name)], check=True)
    return subprocess.run([str(path / name)], capture_output=not sanitize)


with tempfile.TemporaryDirectory(prefix='cod2-gun-fov-') as tmp:
    path = Path(tmp)
    (path / 'headers/PC/cgame_mp').mkdir(parents=True)
    (path / 'common_types.h').write_text(stub_types)
    (path / 'imports.h').write_text('extern void *imp_cls;\n')
    (path / 'headers/PC/cgame_mp/cg_local.h').write_text('#include "common_types.h"\nextern cg_t *cg;\n')
    (path / 'headers/PC/cgame_mp/cg_gunfov.h').write_text(header)
    (path / 'gunfov.c').write_text(gunfov)
    assert build(path, 'gunfov-test', checks.replace('SOURCE', 'gunfov.c'), True).returncode == 0
    for index, (before, after) in enumerate(mutants):
        assert gunfov.count(before) == 1, before
        (path / f'mutant{index}.c').write_text(gunfov.replace(before, after))
        result = build(path, f'gunfov-mutant{index}', checks.replace('SOURCE', f'mutant{index}.c'), False)
        assert result.returncode != 0, f'mutant survived: {after!r}'
    print(f'PASS: {len(mutants)} gun field of view mutants fail')
    (path / 'native.c').write_text(native.replace('SOURCE', 'gunfov.c'))
    subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Werror', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', '-I', str(path), str(path / 'native.c'), '-lm', '-o',
                    str(path / 'native')], check=True)
    subprocess.run([str(path / 'native')], check=True)
