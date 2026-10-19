#!/usr/bin/env python3
"""Check that a hit kicks the camera as it kicks the server's aim.

The server fires along its view angles plus BG_CalculateViewAngles, whose
damage kick runs from client->damageTime, the hit backdated 20 ms. The client
draws the screen with the same function from the viewState that
CG_CalcViewValues builds. This compiles the production view angles, the
client's viewState block and the server's, and replays a hit: the camera must
follow the aim's kick, shifted by those 20 ms, and never kick past it.
"""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
weapons = (root / 'src/PC/bgame/bg_weapons.c').read_text()
shared = (root / 'src/PC/universal/q_shared.c').read_text()
view = (root / 'src/PC/cgame_mp/cg_view_mp.c').read_text()
active = (root / 'src/PC/game_mp/g_active_mp.c').read_text()
feedback = (root / 'src/PC/cgame_mp/cg_playerstate_mp.c').read_text()


def function(source, name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;{]*?\)\n\{', source)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


def block(source, first, last):
    start = source.index(first)
    return source[start:source.index(last, start) + len(last)] + '\n'


# The client's damage time is the hit's snapshot time, its blend's end 500 ms
# later (cg_t's damageTime at 0x2bed4, v_dmg_time at 0x2bf0c, time at 0x25bb0);
# the server's is the hit's frame, 20 ms back.
damage = function(feedback, 'CG_DamageFeedback')
assert '#define CG_OFF_2BED4 0x2bed4' in feedback and '#define CG_OFF_2BF0C 0x2bf0c' in feedback
assert 'CG_INT(cg, CG_OFF_2BED4) = CG_INT(snap, 8);' in damage
assert 'CG_INT(cg, CG_OFF_2BF0C) = CG_INT(cg, CG_OFF_25BB0) + 500;' in damage
assert 'client->damageTime = ((struct level_locals_t *)imp_level)->time - 20;' in function(active, 'P_DamageFeedback')
# The gun takes the same time on both sides.
assert 'ws.damageTime = cg->damageTime;' in (root / 'src/PC/cgame_mp/cg_weapons.c').read_text()
assert 'ws.damageTime = client->damageTime;' in active

client_block = block(view, '        viewState.ps = ps;', '        BG_CalculateViewAngles(&viewState, viewAnglesResult);')
server_block = block(active, '    memset(&vs, 0, sizeof(vs));', '    BG_CalculateViewAngles(&vs, angles);')
angles = ''.join(function(weapons, name) for name in (
    'BG_TrackViewIdleFactor', 'BG_AddViewDamageKick', 'BG_AddViewIdle', 'BG_AddScopedViewBob', 'BG_AddAdsViewBob',
    'BG_CalculateViewAngles'))
lean = function(shared, 'GetLeanFraction_core') + function(shared, 'GetLeanFraction')

support = r'''
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef float vec_t;
typedef vec_t vec3_t[3];
typedef struct { int eFlags; float leanf, fWeaponPosFrac, holdBreathScale; } playerState_t;
typedef struct { playerState_t *ps; int damageTime, time; float v_dmg_pitch, v_dmg_roll, xyspeed, frametime, fLastIdleFactor;
                 int *weapIdleTime; } viewState_t;
typedef struct { int overlayReticle, bADSPositionInfo; float fHipIdleAmount, fAdsIdleAmount, hipIdleSpeed, adsIdleSpeed,
                 fIdleProneFactor, fIdleCrouchFactor, fAdsBobFactor, fAdsViewBobMult; } WeaponDef;
static WeaponDef def;
static WeaponDef *BG_GetPlayerWeaponDef(const playerState_t *ps, int weapon) { (void)ps; (void)weapon; return &def; }
static int BG_GetViewmodelWeaponIndex(const playerState_t *ps) { (void)ps; return 1; }
static float BG_GetBobCycle(const playerState_t *ps) { (void)ps; return 0.0f; }
static float BG_GetSpeed(const playerState_t *ps, int time) { (void)ps; (void)time; return 0.0f; }
static float BG_GetVerticalBobFactor(const playerState_t *ps, float c, float s, float m) { (void)ps; (void)c; (void)s; (void)m; return 0.0f; }
static float BG_GetHorizontalBobFactor(const playerState_t *ps, float c, float s, float m) { (void)ps; (void)c; (void)s; (void)m; return 0.0f; }
'''

harness = r'''
static struct { int damageTime, v_dmg_time, time, frametime, weapIdleTime; float v_dmg_pitch, v_dmg_roll;
                struct { float fLastIdleFactor; } playerEntity; } cgState, *cg = &cgState;
static struct { playerState_t ps; int damageTime, weapIdleTime; float v_dmg_pitch, v_dmg_roll, fLastIdleFactor; } clientState,
    *client = &clientState;
static struct { int time; } level;

static void ClientKick(playerState_t *ps, float *out)
{
    viewState_t viewState;
    float viewAnglesResult[3], speed = 0.0f;
CLIENT_BLOCK
    out[0] = viewAnglesResult[0];
    out[1] = viewAnglesResult[2];
}

static void ServerKick(int msec, float *out)
{
    viewState_t vs;
    float angles[3];
SERVER_BLOCK
    out[0] = angles[0];
    out[1] = angles[2];
}

int main(void)
{
    /* Hip, iron sights and a scope: the factor that scales the kick. */
    static const struct { float frac; int overlay; } aims[] = { { 0.0f, 0 }, { 1.0f, 0 }, { 1.0f, 1 }, { 0.4f, 1 } };
    static const float hits[][2] = { { 9.0f, -0.17f }, { -6.0f, 2.5f }, { -90.0f, 0.0f } };
    int failures = 0;

    for (int a = 0; a < 4; a++) {
        for (int h = 0; h < 3; h++) {
            const int hit = 410424;
            playerState_t ps;
            float limit = 0.0f, peak = 0.0f;

            /* No idle sway, whose phase the two sides keep apart. */
            memset(&def, 0, sizeof(def));
            def.bADSPositionInfo = 1;
            def.overlayReticle = aims[a].overlay;
            memset(&ps, 0, sizeof(ps));
            ps.fWeaponPosFrac = aims[a].frac;
            ps.holdBreathScale = 1.0f;
            clientState.ps = ps;
            /* The hit as CG_DamageFeedback and P_DamageFeedback record it. */
            cgState.damageTime = hit;
            cgState.v_dmg_time = hit + 500;
            cgState.v_dmg_pitch = clientState.v_dmg_pitch = hits[h][0];
            cgState.v_dmg_roll = clientState.v_dmg_roll = hits[h][1];
            clientState.damageTime = hit - 20;
            cgState.frametime = 8;
            limit = fabsf(hits[h][0]) * 1.5f;
            for (int t = hit; t <= hit + 1200; t += 4) {
                float camera[2], aim[2];

                cgState.time = t;
                ClientKick(&ps, camera);
                level.time = t - 20;
                ServerKick(50, aim);
                if (fabsf(camera[0] - aim[0]) > 1e-4f || fabsf(camera[1] - aim[1]) > 1e-4f) {
                    if (failures++ < 4)
                        printf("aim %d hit %d at %d ms: camera (%.3f %.3f), server aim (%.3f %.3f)\n", a, h, t - hit,
                               camera[0], camera[1], aim[0], aim[1]);
                }
                if (fabsf(camera[0]) > peak)
                    peak = fabsf(camera[0]);
                if (t - hit >= 500 && (camera[0] != 0.0f || camera[1] != 0.0f)) {
                    if (failures++ < 4)
                        printf("aim %d hit %d: the camera still kicks %.3f at %d ms\n", a, h, camera[0], t - hit);
                }
            }
            if (peak > limit || peak < fabsf(hits[h][0]) * 0.25f) {
                if (failures++ < 4)
                    printf("aim %d hit %d: the camera's kick peaks at %.3f for a %.1f kick\n", a, h, peak, hits[h][0]);
            }
        }
    }
    return failures != 0;
}
'''


def run(client_source):
    program = support + lean + angles + harness.replace('CLIENT_BLOCK', client_source).replace('SERVER_BLOCK', server_block)
    with tempfile.TemporaryDirectory(prefix='cod2-damage-view-') as folder:
        path = Path(folder)
        (path / 'test.c').write_text(program)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wno-unused-function', '-fsanitize=address,undefined',
                        '-fno-sanitize-recover=all', str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
        return subprocess.run([str(path / 'test')], capture_output=True, text=True)


result = run(client_block)
assert result.returncode == 0, result.stdout + result.stderr

# The blend's end in place of the hit kicked the camera 15 times over at once
# and then again half a second late, while shots followed the server's kick.
mutants = [
    ('viewState.damageTime = cg->damageTime;', 'viewState.damageTime = cg->v_dmg_time;'),
    ('viewState.time = cg->time;', 'viewState.time = cg->time + 500;'),
    ('viewState.v_dmg_pitch = cg->v_dmg_pitch;', 'viewState.v_dmg_pitch = cg->v_dmg_roll;'),
    ('viewState.v_dmg_roll = cg->v_dmg_roll;', 'viewState.v_dmg_roll = 0.0f;'),
]
for before, after in mutants:
    assert client_block.count(before) == 1, before
    assert run(client_block.replace(before, after)).returncode != 0, f'mutant survived: {after}'
print(f'PASS: a hit kicks the camera as it kicks the server\'s aim, from hip to scope, 20 ms after it and no further; '
      f'{len(mutants)} mutants fail')
