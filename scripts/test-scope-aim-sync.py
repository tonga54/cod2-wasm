#!/usr/bin/env python3
"""A scoped shot leaves where the scope was drawn.

The phase of a scope's idle sway builds up frame by frame on the client, which
the server cannot follow, so the command carries the sway in its angles, as it
carries the kick, and the server aims with the view of the command that fired.
This runs the production client (the command's sway and aim, the command's
angles, the camera's view offsets) and the server (ClientThink_real's aim and
spread block, Pmove applying the command) through lives whose phases differ,
mouse turns, a held breath and the gasp after it, comparing the camera with the
server's aim every frame; then a bot's own sway, a followed or dead player's
command, the spread a shot leaves with and iron sights.
"""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
weapons = (root / 'src/PC/bgame/bg_weapons.c').read_text()
shared = (root / 'src/PC/universal/q_shared.c').read_text()
math_source = (root / 'src/PC/universal/com_math.c').read_text()
view = (root / 'src/PC/cgame_mp/cg_view_mp.c').read_text()
draw = (root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text()
active = (root / 'src/PC/game_mp/g_active_mp.c').read_text()
fire = (root / 'src/PC/game_mp/g_weapon_mp.c').read_text()
cgame = (root / 'src/PC/client_mp/cl_cgame_mp.c').read_text()
user_input = (root / 'src/PC/client_mp/cl_input.c').read_text()


def function(source, name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;{]*?\)\n\{', source)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


def block(source, first, last):
    start = source.index(first)
    return source[start:source.index(last, start) + len(last)] + '\n'


def replace_once(text, before, after):
    assert text.count(before) == 1, before
    return text.replace(before, after)


# The frame: the command's aim is set, then the command made from it, then
# predicted, then the camera drawn from the predicted view angles plus the
# view's own offsets. CG_DrawActive sets the same aim again.
frame = function(view, 'CG_DrawActiveFrame')
assert frame.index('    CG_UpdateViewKick();\n    CL_Input();\n    CG_PredictPlayerState();') < \
    frame.index('    CG_CalcViewValues();') < frame.index('        CG_DrawActive();')
active_draw = function(draw, 'CG_DrawActive')
assert 'CG_SetUserCmdAim();' in active_draw and 'CL_SetUserCmdAimValues' not in active_draw
assert 'cg->refdefViewAngles[0] = ps->viewangles[0];' in view and \
    'cg->refdefViewAngles[0] += viewAnglesResult[0];' in view
# The server aims after Pmove, before the command's events fire, and nowhere
# else; shots and throws leave along that aim.
think = function(active, 'ClientThink_real')
assert think.count('G_SetGunAngles(') == 1 and 'fGunPitch' not in think and 'fGunYaw' not in think
assert think.index('    Pmove(&pm);') < think.index('    G_SetGunAngles(client, viewOffset, sights ? gunAngles : NULL);') < \
    think.index('    ClientEvents(ent, oldEventSequence);')
for name in ('FireWeaponAntiLag', 'G_UseOffHand'):
    body = function(fire, name)
    assert 'viewang[0] = ' in body and 'fGunPitch;' in body and 'fGunYaw;' in body, name

lean = function(shared, 'GetLeanFraction_core') + function(shared, 'GetLeanFraction')
angles_code = ''.join(function(weapons, name) for name in (
    'BG_TrackViewIdleFactor', 'BG_AddViewDamageKick', 'BG_AddViewIdle', 'BG_AddScopedViewBob', 'BG_AddAdsViewBob',
    'BG_CalculateViewAngles', 'BG_CalculateViewIdleAngles'))
axes = ''.join(function(math_source, name) for name in (
    'vectoangles', 'AngleVectors', 'MatrixMultiply', 'AnglesToAxis', 'AxisToAngles'))
for name in ('MatrixMultiply', 'AnglesToAxis', 'AxisToAngles'):
    axes = replace_once(axes, 'void ' + name + '(', 'static void Math_' + name + '(')
kick_defines = ''.join(line + '\n' for line in view.splitlines() if line.startswith('#define CG_KICK_'))
client_code = kick_defines + function(view, 'CG_IntegrateViewKick') + \
    re.search(r'^static vec3_t s_scopeSway;\n', view, re.M)[0] + \
    ''.join(function(view, name) for name in ('CG_UpdateScopeSway', 'CG_SetUserCmdAim', 'CG_UpdateViewKick'))
camera_block = block(view, '        viewState.ps = ps;', '        BG_CalculateViewAngles(&viewState, viewAnglesResult);')
command_angles = block(user_input, '    for (i = 0; i < 3; ++i) {\n        cmd.angles[i] =', '    }\n')
aim_values = function(cgame, 'CL_SetUserCmdAimValues')
server_block = block(active, '    memset(&vs, 0, sizeof(vs));',
                     '    G_SetGunAngles(client, viewOffset, sights ? gunAngles : NULL);')
gun_angles = function(active, 'G_SetGunAngles')

support = r'''
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef float vec_t;
typedef vec_t vec3_t[3];
typedef int qboolean;
typedef struct {
    int pm_type, pm_flags, eFlags, weapon;
    float leanf, fWeaponPosFrac, holdBreathScale, aimSpreadScale;
    vec3_t viewangles;
    int delta_angles[3];
} playerState_t;
typedef struct {
    playerState_t *ps;
    int damageTime, time;
    float v_dmg_pitch, v_dmg_roll, xyspeed, frametime, fLastIdleFactor;
    int *weapIdleTime;
} viewState_t;
typedef struct {
    playerState_t *ps;
    float xyspeed, frametime;
    vec3_t vLastMoveAng;
    float fLastIdleFactor;
    int time, damageTime;
    float v_dmg_pitch, v_dmg_roll;
    vec3_t vGunOffset, vGunSpeed, swayAngles;
    int *weapIdleTime;
} weaponState_t;
typedef struct {
    int overlayReticle, bADSPositionInfo;
    float fHipIdleAmount, fAdsIdleAmount, hipIdleSpeed, adsIdleSpeed, fIdleProneFactor, fIdleCrouchFactor;
    float fAdsBobFactor, fAdsViewBobMult, fAdsViewKickCenterSpeed, fHipViewKickCenterSpeed;
} WeaponDef;
typedef struct { int serverTime, buttons, angles[3]; } usercmd_t;
typedef struct { playerState_t *ps; usercmd_t cmd; } pmove_t;
typedef struct {
    playerState_t ps;
    int damageTime, weapIdleTime;
    float v_dmg_pitch, v_dmg_roll, fLastIdleFactor;
    vec3_t swayViewAngles, swayOffset, swayAngles, vLastMoveAng, vGunOffset, vGunSpeed;
    float fGunPitch, fGunYaw, currentAimSpreadScale;
} gclient_t;
typedef struct { struct { int number; } s; gclient_t *client; } gentity_t;

/* A sniper scope, the Springfield's, and an iron-sighted rifle. */
static WeaponDef sniper, rifle, *held = &sniper;
static WeaponDef *BG_GetPlayerWeaponDef(const playerState_t *ps, int weapon) { (void)ps; (void)weapon; return held; }
static WeaponDef *BG_GetWeaponDef(int weapon) { (void)weapon; return held; }
static int BG_GetViewmodelWeaponIndex(const void *ps) { (void)ps; return 1; }
static qboolean BG_IsAimDownSightWeapon(int weapon) { (void)weapon; return 1; }
/* Standing still: no bob. */
static float BG_GetBobCycle(const playerState_t *ps) { (void)ps; return 0.0f; }
static float BG_GetSpeed(const playerState_t *ps, int time) { (void)ps; (void)time; return 0.0f; }
static float BG_GetVerticalBobFactor(const playerState_t *ps, float c, float s, float m) { (void)ps; (void)c; (void)s; (void)m; return 0.0f; }
static float BG_GetHorizontalBobFactor(const playerState_t *ps, float c, float s, float m) { (void)ps; (void)c; (void)s; (void)m; return 0.0f; }

typedef struct {
    playerState_t predictedPlayerState;
    vec3_t kickAVel, kickAngles, offsetAngles;
    int frametime, time, damageTime, weapIdleTime;
    float v_dmg_pitch, v_dmg_roll;
    struct { float fLastIdleFactor; } playerEntity;
} cg_t;
static cg_t cgState;
static cg_t *const cg = &cgState;
typedef struct { vec3_t viewangles, cgameKickAngles; } clientActive_t;
static clientActive_t clientActive;
#define CL_LOCAL (&clientActive)

static struct { int time; } level;
static int botClient = -1;
static qboolean SV_BotIsTestClient(int clientNum) { return clientNum == botClient; }
static void BG_CalculateWeaponPosition_Sway(playerState_t *ps, vec_t *a, vec_t *b, vec_t *c, float s, int msec)
{
    (void)ps; (void)a; (void)b; (void)c; (void)s; (void)msec;
}
/* The gun's own turn this command, which iron sights add to the shot. */
static vec3_t gunTurn;
static void BG_CalculateWeaponAngles(weaponState_t *ws, vec_t *angles) { (void)ws; memcpy(angles, gunTurn, sizeof(vec3_t)); }
static void G_BeginGrenadeInput(gentity_t *ent, const usercmd_t *cmd) { (void)ent; (void)cmd; }
static void G_EndGrenadeInput(gentity_t *ent) { (void)ent; }
'''

wrappers = r'''
/* com_math.c's, under the prototypes g_active_mp.c declares. */
static void AnglesToAxis(const vec_t *angles, vec3_t *axis) { Math_AnglesToAxis(angles, axis); }
static void MatrixMultiply(const vec_t *in1, const vec_t *in2, vec_t *out)
{
    Math_MatrixMultiply((const float (*)[3])in1, (const float (*)[3])in2, (float (*)[3])out);
}
static void AxisToAngles(const vec_t *axis, vec_t *angles) { Math_AxisToAngles((vec3_t *)axis, angles); }
'''

harness = r'''
static float breathScale = 1.0f;
/* What Pmove does to what is read here, on the server and in prediction
 * alike: the view from the command, the scope at the eye, the breath, and a
 * shot's spread over its decay. */
static void ApplyCommand(playerState_t *ps, const usercmd_t *cmd)
{
    for (int i = 0; i < 3; i++)
        ps->viewangles[i] = (float)((cmd->angles[i] + ps->delta_angles[i]) & 0xffff) * (360.0f / 65536.0f);
    ps->fWeaponPosFrac = 1.0f;
    ps->holdBreathScale = breathScale;
    ps->aimSpreadScale = cmd->buttons & 1 ? 255.0f : ps->aimSpreadScale * 0.75f;
}
static void Pmove(pmove_t *pm) { ApplyCommand(pm->ps, &pm->cmd); }

static void ServerThink(gentity_t *ent, const usercmd_t *ucmd, int msec)
{
    gclient_t *client = ent->client;
    pmove_t pm;
    viewState_t vs;
    weaponState_t ws;
    vec3_t viewOffset, gunAngles;
    qboolean sights;
    WeaponDef *weapDef;

    memset(&pm, 0, sizeof(pm));
    pm.ps = &client->ps;
    pm.cmd = *ucmd;
SERVER_BLOCK
}

static usercmd_t CreateCommand(int serverTime, int buttons)
{
    clientActive_t *cl = CL_LOCAL;
    usercmd_t cmd;
    int i;

    memset(&cmd, 0, sizeof(cmd));
    cmd.serverTime = serverTime;
    cmd.buttons = buttons;
COMMAND_ANGLES
    return cmd;
}

static void Camera(vec3_t camera)
{
    playerState_t *ps = &cg->predictedPlayerState;
    viewState_t viewState;
    float viewAnglesResult[3], speed = 0.0f;

CAMERA_BLOCK
    for (int i = 0; i < 3; i++)
        camera[i] = ps->viewangles[i] + viewAnglesResult[i];
}

static float Turn(float a, float b) { return fmodf(a - b + 540.0f, 360.0f) - 180.0f; }

int main(void)
{
    int failures = 0, frames = 0;
    /* The most sway a command carried breathing, holding the breath, gasping. */
    float swayMax[3] = { 0.0f, 0.0f, 0.0f };

    sniper.overlayReticle = 1;
    sniper.bADSPositionInfo = 1;
    sniper.fHipIdleAmount = 30.0f;
    sniper.fAdsIdleAmount = 20.0f;
    sniper.hipIdleSpeed = 1.0f;
    sniper.adsIdleSpeed = 1.5f;
    sniper.fIdleProneFactor = sniper.fIdleCrouchFactor = 1.0f;
    sniper.fAdsViewBobMult = 1.0f;
    sniper.fAdsViewKickCenterSpeed = 1000.0f;
    sniper.fHipViewKickCenterSpeed = 100.0f;
    rifle = sniper;
    rifle.overlayReticle = 0;
    rifle.fAdsIdleAmount = 2.0f;

    /* Four lives scoped: the server starts each life's phase at zero, the
     * client's carries on from the lives before, as CG_Respawn leaves it. */
    for (int life = 0; life < 4; life++) {
        gclient_t server;
        gentity_t ent;
        int time = 50000 + life * 100000;

        memset(&server, 0, sizeof(server));
        memset(&ent, 0, sizeof(ent));
        memset(&cgState, 0, sizeof(cgState));
        memset(&clientActive, 0, sizeof(clientActive));
        ent.s.number = 5;
        ent.client = &server;
        held = &sniper;
        cg->weapIdleTime = 7919 + life * 104729;
        cg->playerEntity.fLastIdleFactor = server.fLastIdleFactor = 1.0f;
        cg->offsetAngles[0] = 0.125f;
        cg->offsetAngles[1] = -0.0625f;
        cg->predictedPlayerState.delta_angles[1] = 1000 * life;
        cg->predictedPlayerState.fWeaponPosFrac = cg->predictedPlayerState.holdBreathScale = 1.0f;
        server.ps = cg->predictedPlayerState;
        clientActive.viewangles[0] = 3.0f;
        clientActive.viewangles[1] = 40.0f + 30.0f * life;
        for (int f = 0; f < 1200; f++) {
            const int msec = 7 + (f * 5 + life * 3) % 11;
            /* Breathing, holding the breath, gasping, breathing. */
            const int phase = f / 300;
            float camera[3], spread;
            usercmd_t cmd;

            time += msec;
            breathScale = phase == 1 ? 0.0f : phase == 2 ? 4.5f : 1.0f;
            /* Following a runner: the yaw turns on, the pitch wanders. */
            clientActive.viewangles[0] += 0.06f * sinf((float)f * 0.05f);
            clientActive.viewangles[1] += 0.17f;
            cg->frametime = msec;
            cg->time = time;
            CG_UpdateViewKick();
            cmd = CreateCommand(time, f % 40 == 39);
            ApplyCommand(&cg->predictedPlayerState, &cmd);
            Camera(camera);
            level.time = time;
            spread = server.ps.aimSpreadScale;
            ServerThink(&ent, &cmd, msec);
            if (fabsf(Turn(camera[0], server.fGunPitch)) > 1e-4f || fabsf(Turn(camera[1], server.fGunYaw)) > 1e-4f) {
                if (failures++ < 4)
                    printf("life %d frame %d: camera (%.4f %.4f), server aim (%.4f %.4f)\n", life, f, camera[0],
                           camera[1], server.fGunPitch, server.fGunYaw);
            }
            if (server.currentAimSpreadScale != spread / 255.0f) {
                if (failures++ < 4)
                    printf("life %d frame %d: the shot spreads by %.4f, from %.4f before its command\n", life, f,
                           server.currentAimSpreadScale, spread / 255.0f);
            }
            /* The breath reaches the sway a frame late, from that frame's prediction. */
            if (f % 300 >= 2) {
                for (int i = 0; i < 2; i++) {
                    float sway = fabsf(clientActive.cgameKickAngles[i] - cg->kickAngles[i] - cg->offsetAngles[i]);
                    int slot = phase == 3 ? 0 : phase;
                    if (sway > swayMax[slot])
                        swayMax[slot] = sway;
                }
            }
            frames++;
        }
    }
    /* The command carries the sway itself: up to 0.2 degrees, none while the
     * breath is held, four and a half times as much in the gasp after. */
    if (!(swayMax[0] > 0.15f && swayMax[0] < 0.2001f) || !(swayMax[1] < 1e-6f) ||
        !(swayMax[2] > 0.5f && swayMax[2] < 0.9001f)) {
        failures++;
        printf("the command's sway reached %.4f breathing, %.6f holding, %.4f gasping\n", swayMax[0], swayMax[1],
               swayMax[2]);
    }

    /* A bot's commands carry no sway, so the server sways its aim itself. */
    {
        gclient_t server;
        gentity_t ent;
        float most = 0.0f;
        int time = 900000;

        memset(&server, 0, sizeof(server));
        memset(&ent, 0, sizeof(ent));
        ent.s.number = botClient = 7;
        ent.client = &server;
        held = &sniper;
        breathScale = 1.0f;
        server.ps.fWeaponPosFrac = server.ps.holdBreathScale = 1.0f;
        server.fLastIdleFactor = 1.0f;
        for (int f = 0; f < 400; f++) {
            usercmd_t cmd;

            memset(&cmd, 0, sizeof(cmd));
            time += 33;
            cmd.serverTime = time;
            cmd.angles[0] = 300;
            cmd.angles[1] = 12000 + f * 7;
            level.time = time;
            ServerThink(&ent, &cmd, 33);
            most = fmaxf(most, fabsf(Turn(server.fGunPitch, server.ps.viewangles[0])));
            most = fmaxf(most, fabsf(Turn(server.fGunYaw, server.ps.viewangles[1])));
        }
        if (!(most > 0.15f && most < 0.2001f)) {
            failures++;
            printf("a bot's aim swayed %.4f from its view\n", most);
        }
        botClient = -1;
    }

    /* Raised, then dead, raised again, then following another player: dead or
     * following, the command carries no sway of the player's own. */
    {
        float most = 0.0f;

        memset(&cgState, 0, sizeof(cgState));
        memset(&clientActive, 0, sizeof(clientActive));
        held = &sniper;
        cg->playerEntity.fLastIdleFactor = 1.0f;
        cg->predictedPlayerState.fWeaponPosFrac = cg->predictedPlayerState.holdBreathScale = 1.0f;
        cg->offsetAngles[1] = 0.25f;
        for (int k = 0; k < 4; k++) {
            for (int f = 0; f < 200; f++) {
                cg->frametime = 10;
                cg->time += 10;
                cg->predictedPlayerState.pm_type = k == 1 ? 6 : 0;
                cg->predictedPlayerState.pm_flags = k == 3 ? 0x400000 : 0;
                CG_UpdateViewKick();
                for (int i = 0; i < 3; i++) {
                    float sway = clientActive.cgameKickAngles[i] - cg->kickAngles[i] - cg->offsetAngles[i];
                    if (k == 0 || k == 2)
                        most = fmaxf(most, fabsf(sway));
                    else if (sway != 0.0f && failures++ < 8)
                        printf("%s: the command carries %.4f of sway\n", k == 1 ? "dead" : "following", sway);
                }
            }
        }
        if (!(most > 0.15f)) {
            failures++;
            printf("raised, the command carried %.4f of sway\n", most);
        }
    }

    /* Through iron sights the shot also turns with the gun, from the view of
     * the command that fired. */
    {
        gclient_t server;
        gentity_t ent;
        int time = 2000000;

        memset(&server, 0, sizeof(server));
        memset(&ent, 0, sizeof(ent));
        ent.s.number = 5;
        ent.client = &server;
        held = &rifle;
        breathScale = 1.0f;
        gunTurn[0] = 0.4f;
        gunTurn[1] = -0.3f;
        gunTurn[2] = 0.2f;
        server.ps.fWeaponPosFrac = server.ps.holdBreathScale = 1.0f;
        for (int f = 0; f < 50; f++) {
            usercmd_t cmd;
            vec3_t axis[3], viewAxis[3], both[3], expected;

            memset(&cmd, 0, sizeof(cmd));
            time += 16;
            cmd.serverTime = time;
            cmd.angles[0] = 65000 - f * 40;
            cmd.angles[1] = 5000 + f * 300;
            level.time = time;
            ServerThink(&ent, &cmd, 16);
            /* No hit, lean, bob or scope idle: the view is the command's. */
            AnglesToAxis(gunTurn, axis);
            AnglesToAxis(server.ps.viewangles, viewAxis);
            MatrixMultiply((const vec_t *)axis, (const vec_t *)viewAxis, (vec_t *)both);
            AxisToAngles((const vec_t *)both, expected);
            if (fabsf(Turn(server.fGunPitch, expected[0])) > 1e-4f || fabsf(Turn(server.fGunYaw, expected[1])) > 1e-4f) {
                if (failures++ < 12)
                    printf("iron sights %d: aim (%.4f %.4f), expected (%.4f %.4f)\n", f, server.fGunPitch,
                           server.fGunYaw, expected[0], expected[1]);
            }
        }
        memset(gunTurn, 0, sizeof(gunTurn));
        held = &sniper;
    }
    printf("%d\n", frames);
    return failures != 0;
}
'''


def program(parts):
    return (support + lean + parts['angles'] + axes + wrappers + aim_values + parts['client'] + parts['gun'] +
            harness.replace('SERVER_BLOCK', parts['server']).replace('COMMAND_ANGLES', command_angles)
            .replace('CAMERA_BLOCK', parts['camera']))


def run(directory, name, parts, flags):
    path = Path(directory) / f'{name}.c'
    binary = Path(directory) / name
    path.write_text(program(parts))
    subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    *flags, str(path), '-lm', '-o', str(binary)], check=True)
    return subprocess.run([str(binary)], capture_output=True, text=True)


def aim_before_pmove(text):
    late = ('    /* After Pmove: shots and throws leave along the view this command turned to. */\n'
            '    G_SetGunAngles(client, viewOffset, sights ? gunAngles : NULL);\n')
    text = replace_once(text, late, '')
    return replace_once(text, '    G_BeginGrenadeInput(ent, &pm.cmd);\n',
                        '    G_SetGunAngles(client, viewOffset, sights ? gunAngles : NULL);\n'
                        '    G_BeginGrenadeInput(ent, &pm.cmd);\n')


def spread_after_shot(text):
    line = '    client->currentAimSpreadScale = client->ps.aimSpreadScale / 255.0f;\n'
    text = replace_once(text, line, '')
    return replace_once(text, '    Pmove(&pm);\n', '    Pmove(&pm);\n' + line)


def idle_in_view(text):
    return replace_once(text, '    BG_AddViewDamageKick(vs, angles, weapDef);\n    BG_AddScopedViewBob',
                        '    BG_AddViewDamageKick(vs, angles, weapDef);\n    BG_AddViewIdle(vs, angles, weapDef);\n'
                        '    BG_AddScopedViewBob')


def idle_in_camera(text):
    frametime = '        viewState.frametime = (float)cg->frametime * 0.001f;\n'
    return replace_once(text, frametime, frametime + '        viewState.fLastIdleFactor = cg->playerEntity.fLastIdleFactor;\n'
                                                     '        viewState.weapIdleTime = &cg->weapIdleTime;\n')


def no_sway_in_command(text):
    return replace_once(text, ' + s_scopeSway[i];', ';')


production = {'angles': angles_code, 'client': client_code, 'server': server_block, 'gun': gun_angles,
              'camera': camera_block}
mutants = [
    # As it was: the camera swayed with the client's phase, the server's aim
    # with its own, which restarts each life.
    ('each side sways with its own phase', {
        'angles': idle_in_view, 'camera': idle_in_camera, 'client': no_sway_in_command,
        'server': lambda t: replace_once(t, 'if (SV_BotIsTestClient(ent->s.number)) {', 'if (0) {')}),
    ('the camera and the server add a sway to the command\'s', {'angles': idle_in_view, 'camera': idle_in_camera}),
    ('the command without the sway', {'client': no_sway_in_command}),
    ('the server sways a player\'s aim itself',
     {'server': lambda t: replace_once(t, 'if (SV_BotIsTestClient(ent->s.number)) {', 'if (1) {')}),
    ('a bot aims without sway',
     {'server': lambda t: replace_once(t, 'if (SV_BotIsTestClient(ent->s.number)) {', 'if (0) {')}),
    ('the aim from the command before', {'server': aim_before_pmove}),
    ('the spread after the shot', {'server': spread_after_shot}),
    ('a followed player\'s sway in the command',
     {'client': lambda t: replace_once(t, 'if (ps->pm_flags & 0x400000)', 'if (0)')}),
    ('the sway kept when dead',
     {'client': lambda t: replace_once(t, '        memset(cg->kickAngles, 0, sizeof(cg->kickAngles));\n'
                                          '        memset(s_scopeSway, 0, sizeof(s_scopeSway));\n',
                                       '        memset(cg->kickAngles, 0, sizeof(cg->kickAngles));\n')}),
    ('iron sights without the gun\'s turn', {'gun': lambda t: replace_once(t, '    if (sightAngles) {', '    if (0) {')}),
]

with tempfile.TemporaryDirectory(prefix='cod2-scope-aim-') as directory:
    result = run(directory, 'production', production, ['-Wall', '-Werror', '-Wno-unused-function'])
    assert result.returncode == 0, result.stdout + result.stderr
    frames = int(result.stdout.split()[-1])
    for index, (name, edits) in enumerate(mutants):
        parts = dict(production)
        for part, mutate in edits.items():
            parts[part] = mutate(parts[part])
        result = run(directory, f'mutant{index}', parts, ['-w'])
        # A wrong aim, not a crash: the harness reports and returns 1.
        assert result.returncode == 1, f'mutant survived or crashed: {name}\n{result.stdout}{result.stderr[-2000:]}'

print(f'PASS: {frames} frames of a scoped player over four lives whose sway phases differ, turning, holding the '
      f'breath and gasping, the camera on the server\'s aim every frame with the sway in the command; a bot swayed '
      f'by the server, none in a dead or following player\'s command, the spread from before the shot and iron '
      f'sights from the command\'s view; {len(mutants)} mutants fail')
