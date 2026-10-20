#!/usr/bin/env python3
"""Run the production controller aim assist on simulated enemies: a slight slowdown and a little following."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
module = (root / 'src/PC/cgame_mp/cg_aimassist_mp.c').read_text()
header = (root / 'src/headers/PC/cgame_mp/cg_aimassist.h').read_text()
view = (root / 'src/PC/cgame_mp/cg_view_mp.c').read_text()
gamepad = (root / 'src/PC/client_mp/cl_gamepad.c').read_text()
defaults = (root / 'src/PC/ui_mp/ui_gamepad_menu.c').read_text()
settings = (root / 'src/PC/ui_mp/ui_wwii_settings.c').read_text()

# Every drawn frame hands the client the assist, after the view is set; the
# client registers the archived switch, on by default, which the controller
# defaults restore and the settings explain.
assert view.count('#include "headers/PC/cgame_mp/cg_aimassist.h"\n') == 1
assert view.count('        CG_UpdateRumble();\n    }\n#endif\n#if COD2_FEATURE_GAMEPAD\n    CG_AimAssistFrame();\n'
                  '#endif\n') == 1
assert gamepad.count('cl_gamepadAimAssist = Dvar_RegisterBool_mac("cl_gamepadAimAssist", 1, 0x1);') == 1
assert gamepad.count('void CL_Gamepad_SetAimAssist(float slowdown, float yawRate, float pitchRate)\n{') == 1
assert defaults.count('Cbuf_AddText("seta cl_gamepadAimAssist 1\\n");') == 1
assert settings.count('{ "cl_gamepadAimAssist", "') == 1
print('PASS: every drawn frame hands the client the aim assist; its archived switch is on by default, restored by '
      'the controller defaults and explained in the settings')

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define COD2_FEATURE_GAMEPAD 1
typedef float vec_t;
typedef vec_t vec3_t[3];
typedef struct {
    float fraction; vec3_t normal; int surfaceFlags, contents; const char *material;
    unsigned short entityNum, partName, partGroup; unsigned char allsolid, startsolid;
} trace_t;
typedef struct { int eType, eFlags, clientNum; } entityState_t;
typedef struct { int nextValid; entityState_t nextState; vec3_t lerpOrigin; } centity_t;
typedef struct { int infoValid, team; } clientInfo_t;
typedef struct { struct { int clientNum; } ps; } snapshot_t;
typedef struct { int pm_type; } playerState_t;
typedef struct {
    int clientNum, time; snapshot_t *nextSnap; playerState_t predictedPlayerState;
    struct { vec3_t vieworg; vec3_t viewaxis[3]; } refdef;
    struct { clientInfo_t clientinfo[64]; } bgs;
} cg_t;
static snapshot_t snap;
static cg_t cgState;
cg_t *cg = &cgState;
static centity_t entities[1024];
centity_t *cg_entities = entities;

/* Per target: a wall or another player between, or the trace stopping on its
 * own body; the smoke the server would see through. */
static unsigned short blockedBy[64];
static int hitsSelf[64];
static float smoke[64];
static int target(const vec_t *end)
{
    for (int i = 0; i < 64; ++i)
        if (entities[i].nextValid && fabsf(entities[i].lerpOrigin[0] - end[0]) < .01f &&
            fabsf(entities[i].lerpOrigin[1] - end[1]) < .01f)
            return i;
    return -1;
}
void CG_TraceCapsule(trace_t *result, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end,
                     int skipNumber, int mask)
{
    int i = target(end);
    assert(skipNumber == cg->clientNum && mask == 0x2000001 && !mins[0] && !maxs[2]);
    (void)start;
    memset(result, 0, sizeof(*result));
    result->fraction = 1.0f;
    result->entityNum = 1023;
    if (i >= 0 && blockedBy[i]) {
        result->fraction = .5f;
        result->entityNum = blockedBy[i];
    } else if (i >= 0 && hitsSelf[i]) {
        result->fraction = .97f;
        result->entityNum = (unsigned short)i;
    }
}
float FX_GetServerVisibility(const vec_t *start, const vec_t *end)
{
    int i = target(end);
    (void)start;
    return i >= 0 && smoke[i] > 0.0f ? smoke[i] : 1.0f;
}
static float slowdown = -1, yawRate, pitchRate;
static int handed;
void CL_Gamepad_SetAimAssist(float s, float y, float p) { slowdown = s; yawRate = y; pitchRate = p; handed++; }
''' + header

checks = r'''
static int near(float value, float expected, float tolerance) { return fabsf(value - expected) <= tolerance; }
static void enemy(int i, float x, float y, float z, int team, int flags)
{
    memset(&entities[i], 0, sizeof(entities[i]));
    entities[i].nextValid = 1;
    entities[i].nextState.eType = 1;
    entities[i].nextState.eFlags = flags;
    entities[i].nextState.clientNum = i;
    entities[i].lerpOrigin[0] = x;
    entities[i].lerpOrigin[1] = y;
    entities[i].lerpOrigin[2] = z;
    cg->bgs.clientinfo[i].infoValid = 1;
    cg->bgs.clientinfo[i].team = team;
    blockedBy[i] = 0;
    hitsSelf[i] = 0;
    smoke[i] = 0;
}
static void clear(void)
{
    memset(entities, 0, sizeof(entities));
    memset(cg->bgs.clientinfo, 0, sizeof(cg->bgs.clientinfo));
    cg->bgs.clientinfo[0].infoValid = 1;
    cg->bgs.clientinfo[0].team = 1;
}
/* The view from (0, 0, eye) at a yaw, level. */
static void look(float eye, float yawDegrees)
{
    float yaw = yawDegrees * 3.14159265f / 180.0f;
    cg->refdef.vieworg[0] = cg->refdef.vieworg[1] = 0.0f;
    cg->refdef.vieworg[2] = eye;
    cg->refdef.viewaxis[0][0] = cosf(yaw); cg->refdef.viewaxis[0][1] = sinf(yaw); cg->refdef.viewaxis[0][2] = 0;
    cg->refdef.viewaxis[1][0] = -sinf(yaw); cg->refdef.viewaxis[1][1] = cosf(yaw); cg->refdef.viewaxis[1][2] = 0;
    cg->refdef.viewaxis[2][0] = 0; cg->refdef.viewaxis[2][1] = 0; cg->refdef.viewaxis[2][2] = 1;
}
static float frame(int time) { cg->time = time; CG_AimAssistFrame(); return slowdown; }

int main(void)
{
    cg->clientNum = 0;
    snap.ps.clientNum = 0;
    cg->nextSnap = &snap;
    clear();
    look(60.0f, 0.0f);

    /* Over an enemy the right stick keeps three quarters of its turn; over a
     * teammate, a spectator or nothing, all of it. Without teams everyone
     * else is an enemy. */
    enemy(1, 500, 0, 0, 2, 0);
    assert(frame(1000) == 0.75f && yawRate == 0.0f && pitchRate == 0.0f);
    cg->bgs.clientinfo[1].team = 1;
    assert(frame(1016) == 1.0f && yawRate == 0.0f);
    cg->bgs.clientinfo[1].team = 3;
    assert(frame(1032) == 1.0f);
    cg->bgs.clientinfo[1].team = 0;
    cg->bgs.clientinfo[0].team = 0;
    assert(frame(1048) == 0.75f);
    clear();
    assert(frame(1064) == 1.0f && handed == 5);

    /* Only for the local player playing in their own view. */
    enemy(1, 500, 0, 0, 2, 0);
    cg->bgs.clientinfo[0].team = 3;
    assert(frame(1080) == 1.0f);
    cg->bgs.clientinfo[0].team = 1;
    cg->predictedPlayerState.pm_type = 6;
    assert(frame(1096) == 1.0f);
    cg->predictedPlayerState.pm_type = 1;
    assert(frame(1112) == 0.75f);
    cg->predictedPlayerState.pm_type = 0;
    snap.ps.clientNum = 5;
    assert(frame(1128) == 1.0f);
    snap.ps.clientNum = 0;
    cg->nextSnap = NULL;
    assert(frame(1144) == 1.0f);
    cg->nextSnap = &snap;

    /* Full within 16 units of the body's centre line, fading over 1.25
     * degrees more (10.9 units at 500), nothing past it. */
    enemy(1, 500, 16, 0, 2, 0);
    assert(frame(2000) == 0.75f);
    enemy(1, 500, -21.45f, 0, 2, 0);
    assert(near(frame(2016), 0.875f, .002f));
    enemy(1, 500, 27, 0, 2, 0);
    assert(frame(2032) == 1.0f);
    enemy(1, 2000, 59, 0, 2, 0);
    assert(near(frame(2048), 1.0f - .25f * (16 + 43.6f - 59) / 43.6f, .002f));

    /* The body runs from over the feet to the head by stance: 26 units over a
     * crouching enemy's head is the zone's edge, a standing one's head is
     * under it; a prone enemy is far under eye height, on a lower crosshair. */
    look(70.0f, 0.0f);
    enemy(1, 500, 0, 0, 2, 4);
    assert(near(frame(3000), 1.0f - .25f * (26.9f - 26) / 10.9f, .002f));
    enemy(1, 500, 0, 0, 2, 0);
    assert(frame(3008) == 0.75f);
    look(60.0f, 0.0f);
    enemy(1, 500, 0, 0, 2, 8);
    assert(frame(3016) == 1.0f);
    look(30.0f, 0.0f);
    assert(frame(3032) == 0.75f);
    look(100.0f, 0.0f);
    enemy(1, 500, 0, 0, 2, 0);
    assert(frame(3048) == 1.0f);
    look(60.0f, 0.0f);

    /* Hidden, dead, absent or other entities are no targets. */
    int ignored[][3] = { { 0x20, 1, 1 }, { 0x20000, 1, 1 }, { 0, 0, 1 }, { 0, 1, 0 } };
    for (int k = 0; k < 4; ++k) {
        enemy(1, 500, 0, 0, 2, ignored[k][0]);
        entities[1].nextValid = ignored[k][1];
        cg->bgs.clientinfo[1].infoValid = ignored[k][2];
        assert(frame(4000 + k * 16) == 1.0f);
    }
    enemy(1, 500, 0, 0, 2, 0);
    entities[1].nextState.eType = 4;
    assert(frame(4100) == 1.0f);
    enemy(1, 500, 0, 0, 2, 0);
    entities[1].nextState.clientNum = 7;
    assert(frame(4116) == 1.0f);

    /* From 32 to 3000 units ahead. */
    enemy(1, 3100, 0, 0, 2, 0);
    assert(frame(5000) == 1.0f);
    enemy(1, 2900, 0, 0, 2, 0);
    assert(frame(5016) == 0.75f);
    enemy(1, 20, 0, 0, 2, 0);
    assert(frame(5032) == 1.0f);
    enemy(1, -500, 0, 0, 2, 0);
    assert(frame(5048) == 1.0f);

    /* Only an enemy in sight: a wall or another player between hides it, its
     * own body does not; neither does thin smoke, thick smoke does. */
    enemy(1, 500, 0, 0, 2, 0);
    blockedBy[1] = 1022;
    assert(frame(6000) == 1.0f);
    blockedBy[1] = 4;
    assert(frame(6016) == 1.0f);
    blockedBy[1] = 0;
    hitsSelf[1] = 1;
    assert(frame(6032) == 0.75f);
    smoke[1] = .1f;
    assert(frame(6048) == 1.0f);
    smoke[1] = .3f;
    assert(frame(6064) == 0.75f);

    /* The enemy nearest the crosshair wins, the next in sight if it is
     * hidden. */
    clear();
    enemy(2, 500, 21.45f, 0, 2, 0);
    enemy(1, 600, 2, 0, 2, 0);
    assert(frame(7000) == 0.75f);
    blockedBy[1] = 1022;
    assert(near(frame(7016), 0.875f, .002f));
    clear();

    /* Following an enemy running across the view at 190 units a second, 500
     * away, with the view kept on them: 15% of their angular speed once the
     * 0.1 s smoothing has caught up, little at first. */
    {
        int time = 10000;
        float y = 0.0f, expected = 0.0f;
        enemy(1, 500, 0, 0, 2, 0);
        look(50.0f, 0.0f);
        frame(time);
        assert(yawRate == 0.0f);
        /* A crouch now and then changes nothing followed. */
        for (int step = 1; step <= 60; ++step) {
            time += 16;
            y += 190.0f * .016f;
            enemy(1, 500, y, 0, 2, step % 20 == 10 ? 4 : 0);
            look(50.0f, atan2f(y, 500.0f) * 180.0f / 3.14159265f);
            frame(time);
            expected = 500.0f * 190.0f / (500.0f * 500.0f + y * y) * 180.0f / 3.14159265f;
            if (step == 1)
                assert(yawRate > 0.0f && yawRate < .3f * .15f * expected);
            assert(near(slowdown, 0.75f, .001f) && near(pitchRate, 0.0f, .05f));
        }
        assert(near(yawRate, .15f * expected, .05f * .15f * expected));
        /* The same time again keeps the rate; another target starts over. */
        float kept = yawRate;
        frame(time);
        assert(yawRate == kept);
        enemy(2, 500, y, 0, 2, 0);
        entities[1].nextValid = 0;
        frame(time + 16);
        assert(yawRate == 0.0f && slowdown == 0.75f);
        /* So does a gap over 0.2 s. */
        enemy(2, 500, y + 3, 0, 2, 0);
        frame(time + 32);
        assert(yawRate > 0.0f);
        enemy(2, 500, y + 6, 0, 2, 0);
        frame(time + 300);
        assert(yawRate == 0.0f);
        /* Circling the view at 200 degrees a second is followed as 90. */
        clear();
        time = 20000;
        for (int step = 0; step <= 40; ++step, time += 16) {
            float angle = step * 3.2f;
            enemy(1, 100.0f * cosf(angle * 3.14159265f / 180.0f), 100.0f * sinf(angle * 3.14159265f / 180.0f),
                  0, 2, 0);
            look(60.0f, angle);
            frame(time);
            assert(yawRate <= 90.0f * .15f + .001f);
        }
        assert(yawRate > 88.0f * .15f);
        /* At the zone's edge the following fades with the slowdown: half. */
        clear();
        enemy(1, 500, -21.45f, 0, 2, 0);
        look(60.0f, 0.0f);
        frame(30000);
        enemy(1, 500, -21.45f + 3.04f, 0, 2, 0);
        look(60.0f, atan2f(3.04f, 500.0f) * 180.0f / 3.14159265f);
        frame(30016);
        assert(near(slowdown, 0.875f, .003f) && near(yawRate, .5f * .15f * 21.74f * 16 / 116, .01f));
    }
    puts("PASS");
    return 0;
}
'''


def mutate(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


body = '\n'.join(line for line in module.splitlines() if not line.lstrip().startswith('#    include')
                 and not line.startswith('#include'))
mutants = [
    ('#    define AIM_ASSIST_SLOWDOWN 0.25f', '#    define AIM_ASSIST_SLOWDOWN 0.4f'),
    ('#    define AIM_ASSIST_TRACKING 0.15f', '#    define AIM_ASSIST_TRACKING 0.3f'),
    ('#    define AIM_ASSIST_RADIUS 16.0f', '#    define AIM_ASSIST_RADIUS 24.0f'),
    ('#    define AIM_ASSIST_MARGIN_TAN 0.0218f', '#    define AIM_ASSIST_MARGIN_TAN 0.05f'),
    ('#    define AIM_ASSIST_RANGE 3000.0f', '#    define AIM_ASSIST_RANGE 4000.0f'),
    ('return ci->infoValid && ci->team != 3 && (!team || ci->team != team);',
     'return ci->infoValid && (!team || ci->team != team);'),
    ('return ci->infoValid && ci->team != 3 && (!team || ci->team != team);',
     'return ci->infoValid && ci->team != 3 && ci->team != team;'),
    ('return ci->infoValid && ci->team != 3 && (!team || ci->team != team);',
     'return ci->team != 3 && (!team || ci->team != team);'),
    ('    if (cent->nextState.eFlags & 0x20020)', '    if (cent->nextState.eFlags & 0x20)'),
    ('    if (cent->nextState.eFlags & 0x20020)', '    if (cent->nextState.eFlags & 0x20000)'),
    ('if (!cent->nextValid || cent->nextState.eType != 1 || cent->nextState.clientNum != clientNum)',
     'if (!cent->nextValid || cent->nextState.clientNum != clientNum)'),
    ('if (!cent->nextValid || cent->nextState.eType != 1 || cent->nextState.clientNum != clientNum)',
     'if (!cent->nextValid || cent->nextState.eType != 1)'),
    ('    if (cent->nextState.eFlags & 8) {', '    if (0) {'),
    ('    } else if (cent->nextState.eFlags & 4) {', '    } else if (0) {'),
    ('        z = z < bottom ? bottom : z > top ? top : z;\n', ''),
    ('        if (depth < AIM_ASSIST_NEAR || depth > AIM_ASSIST_RANGE)', '        if (depth > AIM_ASSIST_RANGE)'),
    ('        if (depth < AIM_ASSIST_NEAR || depth > AIM_ASSIST_RANGE)', '        if (depth < AIM_ASSIST_NEAR)'),
    ('        if (score >= bestScore)\n            continue;\n', ''),
    ('        if (trace.fraction < 1.0f && trace.entityNum != i)', '        if (trace.fraction < 1.0f)'),
    ('        if (trace.fraction < 1.0f && trace.entityNum != i)', '        if (trace.entityNum < 64 && trace.entityNum != i)'),
    ('        if (FX_GetServerVisibility(eye, point) < AIM_ASSIST_MIN_VISIBILITY)\n            continue;\n', ''),
    ('        bestWeight = off <= AIM_ASSIST_RADIUS ? 1.0f : (outer - off) / (outer - AIM_ASSIST_RADIUS);',
     '        bestWeight = 1.0f;'),
    ('cg->nextSnap->ps.clientNum != cg->clientNum || ', ''),
    ('|| cg->predictedPlayerState.pm_type > 1) {', ') {'),
    ('    if (team == 3) {', '    if (team == 4) {'),
    ('if (best != s_aimAssist.target || elapsed < 0 || elapsed > AIM_ASSIST_MAX_GAP_MSEC) {',
     'if (elapsed < 0 || elapsed > AIM_ASSIST_MAX_GAP_MSEC) {'),
    ('if (best != s_aimAssist.target || elapsed < 0 || elapsed > AIM_ASSIST_MAX_GAP_MSEC) {',
     'if (best != s_aimAssist.target || elapsed < 0) {'),
    ('        s_aimAssist.yawRate += (yawRate - s_aimAssist.yawRate) * blend;',
     '        s_aimAssist.yawRate = yawRate;'),
    ('float yawRate = CG_AimAssistClampRate(CG_AimAssistAngle(yaw - s_aimAssist.yaw) * 1000.0f / (float)elapsed);',
     'float yawRate = CG_AimAssistAngle(yaw - s_aimAssist.yaw) * 1000.0f / (float)elapsed;'),
    ('    dz = cg_entities[best].lerpOrigin[2] + AIM_ASSIST_TRACK_HEIGHT - eye[2];',
     '    dz = cg_entities[best].lerpOrigin[2] + (cg_entities[best].nextState.eFlags & 4 ? 30.0f : 48.0f) - eye[2];'),
    ('    s_aimAssist.yawRate = s_aimAssist.pitchRate = 0.0f;\n    CL_Gamepad_SetAimAssist(1.0f, 0.0f, 0.0f);\n',
     '    s_aimAssist.yawRate = s_aimAssist.pitchRate = 0.0f;\n'),
    ('                            s_aimAssist.yawRate * AIM_ASSIST_TRACKING * bestWeight,',
     '                            s_aimAssist.yawRate * AIM_ASSIST_TRACKING,'),
]
variants = [(support + body + '\n' + checks, None)]
variants += [(support + mutate(body, old, new) + '\n' + checks, (old, new)) for old, new in mutants]
with tempfile.TemporaryDirectory(prefix='cod2-aim-assist-') as directory:
    path = Path(directory) / 'assist.c'
    binary = Path(directory) / 'assist'
    for index, (source, mutant) in enumerate(variants):
        path.write_text(source)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wall', '-Werror' if mutant is None else '-w', '-Wno-unused-function', str(path), '-o',
                        str(binary), '-lm'], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (mutant is None), (index, mutant, result.stdout[-500:],
                                                               result.stderr[-2000:])
print('PASS: over a visible enemy near the crosshair the right stick keeps 75% of its turn, full within 16 units '
      'of the body, fading over 1.25 degrees more, by stance; teammates, spectators, the hidden, the dead, walls, '
      'other players between and thick smoke excluded, its own body and thin smoke not; 32 to 3000 units ahead; '
      'the nearest enemy wins; the view follows 15% of their motion, smoothed over 0.1 s, held to 90 degrees a '
      'second, unaffected by stance, starting over for a new target or after a gap; only for the local player '
      f'alive in their own view; {len(mutants)} failing regression mutants')
