#!/usr/bin/env python3
"""Run production Pmove through prone lives: a shot or a refused turn or stance
while down must not keep sprint and leaning off once the player is up."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
pmove_source = (root / 'src/PC/bgame/bg_pmove.c').read_text()


def function(name):
    match = re.search(r'^([^\n]*\b' + name + r'\([^;]*?\))\s*\{', pmove_source, re.M)
    assert match, name
    depth, end = 1, pmove_source.index('{', match.start()) + 1
    while depth:
        depth += (pmove_source[end] == '{') - (pmove_source[end] == '}')
        end += 1
    return pmove_source[match.start():end] + '\n'


def ordered(text, *parts):
    at = -1
    for part in parts:
        assert text.count(part) == 1, part
        assert text.index(part) > at, part
        at = text.index(part)


pmove = function('Pmove')
# Once per command step, before the view and stance updates set either flag
# again and before sprint reads them.
ordered(pmove, '    while (ps->commandTime != finalTime) {\n',
        '        BG_AnimUpdatePlayerStateConditions(pm);\n        PM_ClearProneFlags(ps);\n',
        '        PM_UpdateViewAngles(ps, (float)msec, &pm->cmd, pm->handler);\n',
        '        PM_CheckDuck(pm, &pml);\n',
        '                PM_UpdateSprint(pm, &pml);\n')

common = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "cod2_sprint.h"
typedef int qboolean;
typedef float vec_t, vec3_t[3];
typedef struct {
    int commandTime, pm_type, pm_flags, eFlags, weaponstate, clientNum, groundEntityNum;
    vec3_t origin, velocity, mins, maxs, viewangles;
    float leanf, fWeaponPosFrac;
} playerState_t;
typedef struct { int serverTime, buttons; signed char forwardmove, rightmove; } usercmd_t;
typedef struct {
    playerState_t *ps; usercmd_t cmd, oldcmd;
    int numtouch, mantleStarted, tracemask, handler; vec3_t mins, maxs; float xyspeed;
} pmove_t;
typedef struct {
    int msec, walking, groundPlane, almostGroundPlane, previous_waterlevel; float frametime;
    vec3_t forward, right, up, previous_origin, previous_velocity;
} pml_t;
'''
common += pmove_source[pmove_source.index('#define PMF_PRONE '):pmove_source.index('static float PM_DvarFloat')]
helpers = ''.join(function(name) for name in (
    'PM_SetProneMovementOverride', 'PM_UpdateSprint', 'PM_ClearProneFlags', 'PM_CanLean',
    'PM_FilterFrozenCommand'))
stubs = r'''
static int turnBlocked, proneBlocked, leanAllowed, fireTimeLeft;
static void PM_VectorCopy(const vec_t *a, vec_t *b) { memcpy(b, a, sizeof(vec3_t)); }
static float PM_VectorLength2D(const vec_t *v) { return hypotf(v[0], v[1]); }
static void BG_AnimUpdatePlayerStateConditions(pmove_t *pm) {}
int PM_WeaponAmmoAvailable(playerState_t *ps) { return 1; }
static void PM_ResetWeaponState(playerState_t *ps) {}
static void PM_DropTimers(playerState_t *ps, int msec) {}
static void PM_AdjustAimSpreadScale(pmove_t *pm, pml_t *pml) {}
/* The production view update gates leaning first; a prone turn into an
 * obstacle marks the command blocked. */
static void PM_UpdateViewAngles(playerState_t *ps, float msec, usercmd_t *cmd, int handler) {
    leanAllowed = PM_CanLean(ps);
    if ((ps->pm_flags & 0x1) && turnBlocked)
        ps->pm_flags |= 0x10000;
}
static void AngleVectors(const vec_t *a, vec_t *f, vec_t *r, vec_t *u) {}
/* Production stance changes: a refused prone marks the command blocked, and
 * getting up from prone passes through crouch. */
static void PM_CheckDuck(pmove_t *pm, pml_t *pml) {
    playerState_t *ps = pm->ps;
    if (pm->cmd.buttons & 0x100) {
        if ((ps->pm_flags & 0x1) || !proneBlocked)
            ps->pm_flags = (ps->pm_flags | 0x1) & ~0x2;
        else
            ps->pm_flags |= 0x10000;
    } else if (ps->pm_flags & 0x1) {
        ps->pm_flags = (ps->pm_flags | 0x2) & ~0x1;
    } else {
        ps->pm_flags &= ~0x2;
    }
}
static void PM_UpdateAimDownSightFlag(pmove_t *pm, pml_t *pml) {}
static void Mantle_Check(pmove_t *pm, pml_t *pml) {}
static void Mantle_Move(pmove_t *pm, playerState_t *ps, pml_t *pml) {}
static void PM_GroundTrace(pmove_t *pm, pml_t *pml) { pml->walking = 1; pm->ps->groundEntityNum = ENTITYNUM_WORLD; }
static void PM_CheckLadderMove(pmove_t *pm, pml_t *pml) {}
static void PM_LadderMove(pmove_t *pm, pml_t *pml) {}
static void PM_WalkMove(pmove_t *pm, pml_t *pml) {}
static void PM_AirMove(pmove_t *pm, pml_t *pml) {}
static void PM_NoclipMove(pmove_t *pm, pml_t *pml) {}
static void PM_Footsteps(pmove_t *pm, pml_t *pml) {}
static void PM_UpdateAimDownSightLerp(pmove_t *pm, pml_t *pml) {}
/* A shot starts 100 ms of firing and, while prone, holds crawling. */
static void PM_Weapon(pmove_t *pm, pml_t *pml) {
    playerState_t *ps = pm->ps;
    if (fireTimeLeft > 0 && (fireTimeLeft -= pml->msec) <= 0)
        ps->weaponstate = 0;
    if ((pm->cmd.buttons & 0x1) && !ps->weaponstate) {
        ps->weaponstate = 3;
        fireTimeLeft = 100;
        PM_SetProneMovementOverride(ps);
    }
}
static void PM_ViewHeightAdjust(pmove_t *pm, pml_t *pml) {}
static void Sys_SnapVector(vec_t *v) {}
'''
checks = r'''
static playerState_t ps;
static pmove_t pm;
static int now, step;

static void spawn(void) {
    memset(&ps, 0, sizeof(ps));
    ps.groundEntityNum = ENTITYNUM_WORLD;
    now = ps.commandTime = 1000;
    memset(&pm, 0, sizeof(pm));
    pm.ps = &ps;
    turnBlocked = proneBlocked = leanAllowed = fireTimeLeft = 0;
}

static void command(int buttons, int forward) {
    now += step;
    pm.cmd = (usercmd_t){ .serverTime = now, .buttons = buttons, .forwardmove = (signed char)forward };
    Pmove(&pm);
    assert(ps.commandTime == now);
}

/* Shift and forward stand the player up; from the first command standing,
 * the run must be on, and leaning possible. */
static void getUpAndRun(void) {
    int standing = 0;
    for (int elapsed = 0; elapsed < 600; elapsed += step) {
        command(BUTTON_SPRINT, 127);
        if (ps.pm_flags & 0x3)
            continue;
        standing = 1;
        assert(ps.pm_flags & PMF_SPRINT);
        assert(leanAllowed);
    }
    assert(standing);
}

int main(void) {
    static const int steps[] = { 1, 7, 16, 33, 50, 66, 67, 100, 133, 250, 1000 };
    for (unsigned i = 0; i < sizeof(steps) / sizeof(steps[0]); ++i) {
        step = steps[i];

        /* Shooting from prone holds crawling while the weapon is busy. */
        spawn();
        command(0x100, 0);
        assert(ps.pm_flags & 0x1);
        for (int elapsed = 0; elapsed < 300; elapsed += step) {
            command(0x100 | 0x1, 0);
            assert(ps.weaponstate == 3 && (ps.pm_flags & 0x800));
        }
        for (int elapsed = 0; elapsed < 300; elapsed += step)
            command(0x100, 0);
        assert(ps.weaponstate == 0 && (ps.pm_flags & 0x1));
        getUpAndRun();

        /* A prone turn into a wall is refused for that command only. */
        spawn();
        command(0x100, 0);
        turnBlocked = 1;
        command(0x100, 0);
        assert(ps.pm_flags & 0x10000);
        turnBlocked = 0;
        command(0x100, 0);
        assert(leanAllowed && !(ps.pm_flags & 0x10000));
        getUpAndRun();

        /* So is going prone where there is no room, as on stairs. */
        spawn();
        proneBlocked = 1;
        command(0x100, 0);
        assert(!(ps.pm_flags & 0x1) && (ps.pm_flags & 0x10000));
        command(BUTTON_SPRINT, 127);
        assert((ps.pm_flags & PMF_SPRINT) && leanAllowed);
    }
    printf("PASS: %u command lengths, sprint and lean after a prone shot, a refused prone turn and a refused prone\n",
           (unsigned)(sizeof(steps) / sizeof(steps[0])));
    return 0;
}
'''

clear_800 = '    if (!(ps->pm_flags & 0x1))\n        ps->pm_flags &= ~0x800;\n'
mutants = [
    ('no per-command clear', helpers, pmove.replace('        PM_ClearProneFlags(ps);\n', '')),
    ('blocked prone kept', helpers.replace('    ps->pm_flags &= ~0x10000;\n', ''), pmove),
    ('weapon hold kept', helpers.replace(clear_800, ''), pmove),
    ('weapon hold dropped while prone', helpers.replace(clear_800, '    ps->pm_flags &= ~0x800;\n'), pmove),
]
for name, variant_helpers, variant_pmove in mutants:
    assert (variant_helpers, variant_pmove) != (helpers, pmove), name

with tempfile.TemporaryDirectory(prefix='cod2-prone-sprint-') as directory:
    path = Path(directory)
    for index, (name, variant_helpers, variant_pmove) in enumerate([('production', helpers, pmove)] + mutants):
        source, binary = path / 'test.c', path / 'test'
        source.write_text(common + variant_helpers + stubs + variant_pmove + checks)
        subprocess.run(['cc', '-I', str(root / 'src/headers'), '-std=c99', '-O1', '-g',
                        '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wall', '-Werror' if index == 0 else '-w', str(source), '-lm', '-o', str(binary)],
                       check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (name, result.stdout, result.stderr[-1000:])
        if index == 0:
            print(result.stdout.strip())
print(f'PASS: {len(mutants)} failing regression mutants')
