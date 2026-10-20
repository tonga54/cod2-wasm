#!/usr/bin/env python3
"""Run the production controller vibration on simulated shots, blasts, hits and melee: one kick a round."""
from pathlib import Path
import re
import subprocess
import tempfile
import zipfile

root = Path(__file__).resolve().parent.parent
module = (root / 'src/PC/cgame_mp/cg_rumble_mp.c').read_text()
header = (root / 'src/headers/PC/cgame_mp/cg_rumble.h').read_text()
event = (root / 'src/PC/cgame_mp/cg_event_mp.c').read_text()
weapons = (root / 'src/PC/cgame_mp/cg_weapons.c').read_text()
playerstate = (root / 'src/PC/cgame_mp/cg_playerstate_mp.c').read_text()
view = (root / 'src/PC/cgame_mp/cg_view_mp.c').read_text()
consolecmds = (root / 'src/PC/cgame_mp/cg_consolecmds_mp.c').read_text()
cmake = (root / 'downstream/wasm/CMakeLists.txt').read_text()
bridge = (root / 'downstream/wasm/web_haptics.c').read_text()
script = (root / 'downstream/wasm/web_haptics.js').read_text()


def function(text, head):
    assert text.count(head) == 1, head
    start = text.index(head)
    depth, end = 1, text.index('{', start) + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


def ordered(text, *parts):
    at = -1
    for part in parts:
        assert text.count(part) == 1, part
        assert text.index(part) > at, part
        at = text.index(part)


def case(label):
    start = event.index(f'        case {label}:')
    return event[start:event.index('\n        case ', start + 1)]


# The browser build vibrates: the cgame's events feed the mixer, which runs
# every frame, and the Gamepad API bridge is linked in.
assert cmake.count('  COD2_FEATURE_RUMBLE=1\n') == 1 and 'COD2_FEATURE_RUMBLE=0' not in cmake
assert cmake.count('"${CMAKE_CURRENT_LIST_DIR}/web_haptics.c"') == 1
assert cmake.count('"SHELL:--pre-js ${CMAKE_CURRENT_LIST_DIR}/web_haptics.js"') == 1
assert cmake.count('LINK_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/web_haptics.js"') == 1
assert bridge.count('EM_JS(void, WebHaptics_Rumble, (float strong, float weak), {') == 1
assert bridge.count('Module.cod2CreateHaptics()') == 1 and script.count("Module['cod2CreateHaptics'] = ") == 1
assert view.count('        CG_UpdateRumble();\n') == 1 and consolecmds.count('        CG_InitRumble();\n') == 1
for source in (event, weapons, playerstate):
    assert source.count('#include "headers/PC/cgame_mp/cg_rumble.h"\n') == 1
# Grenades, S-Mines and C4 go off as 0xbc and rockets as 0xbd/0xbe, each with
# its weapon; smoke (0xbf), a landing (0xbb) and the held grenade's own event
# (0xc5, followed by the grenade's 0xbc) stay still.
for label in ('0xbc', '0xbd', '0xbe'):
    assert case(label).count('#if COD2_FEATURE_RUMBLE\n            CG_RumbleExplosion((const vec_t *)position, '
                             'es->weapon);\n#endif\n            return;') == 1, label
for label in ('0xbb', '0xbf', '0xc5'):
    assert 'CG_Rumble' not in case(label), label
assert case('0xa3').count('#if COD2_FEATURE_RUMBLE\n            CG_RumbleMeleeSwing(es);\n#endif\n') == 1
for label in ('0xad', '0xae'):
    assert case(label).count('#if COD2_FEATURE_RUMBLE\n            CG_RumbleMeleeImpact((const vec_t *)position);\n'
                             '#endif\n') == 1, label
assert case('0xc4').count('#if COD2_FEATURE_RUMBLE\n            CG_RumbleEarthquake(') == 1
assert event.count('CG_Rumble') == 7
# Every fire event, after the local player's recoil.
ordered(function(weapons, 'void CG_FireWeapon(centity_t *cent, int event, int barrel)\n{'),
        '        BG_WeaponFireRecoil(&cg->predictedPlayerState, cg->vGunSpeed, cg->kickAVel);\n',
        '    }\n#if COD2_FEATURE_RUMBLE\n    CG_RumbleFireEvent(ent);\n#endif\n',
        '    CG_PlayFireWeaponEffect(cent, ent, barrel);\n')
assert weapons.count('CG_Rumble') == 1
assert function(playerstate, 'void CG_DamageFeedback(int yawByte, int pitchByte, int damage)\n{').endswith(
    '    CG_MenuShowNotify(0);\n#if COD2_FEATURE_RUMBLE\n    CG_RumbleDamage(damage);\n#endif\n}')
assert function(playerstate, 'void CG_TransitionPlayerState(playerState_t *ps, playerState_t *ops)\n{').count(
    '    int event;\n\n#if COD2_FEATURE_RUMBLE\n    CG_RumbleTransition(ps, ops);\n#endif\n'
    '    if (ps->damageEvent != ops->damageEvent) {\n') == 1
assert playerstate.count('CG_Rumble') == 2
print('PASS: the browser build links the Gamepad API bridge; every fire event, the hits taken, melee swings and '
      'impacts, grenade/S-Mine/C4/rocket blasts and earthquake() feed the mixer, which runs every frame; smoke, '
      'landings and the held grenade\'s own event stay still')

# The browser's package carries every graph the cgame can play, both motors,
# well formed: the mixer's own names and each weapon file's fire and melee.
wanted = set(re.findall(r'^#    define RUMBLE_\w+_GRAPH "(\w+)"$', module, re.M))
wanted |= set(re.findall(r'^        \{ "\w+", "(\w+)" \},$', module, re.M))
wanted |= set(re.findall(r'^    return "(\w+)";$', module, re.M))
assert {'artillery_quake', 'damage_heavy', 'melee_medium', 'mg42_fire', 'defaultweapon_fire'} <= wanted, wanted
with zipfile.ZipFile(root / 'data/browser/main/cod2_browser_renderer.iwd') as archive:
    names = set(archive.namelist())
    weapon_files = [name for name in names if name.startswith('weapons/mp/')]
    for name in weapon_files:
        fields = archive.read(name).decode('latin-1').split('\\')
        values = dict(zip(fields[1::2], fields[2::2]))
        wanted |= {values[key] for key in ('fireRumble', 'meleeImpactRumble') if values.get(key)}
    for graph in sorted(wanted):
        for motor in ('_h', '_l'):
            path = f'rumble/{graph}{motor}.rmb'
            assert path in names, path
            tokens = archive.read(path).decode('ascii').split()
            assert tokens[0] == 'RUMBLEGRAPHFILE' and len(tokens) == 2 + 2 * int(tokens[1]), path
print(f'PASS: the browser package carries all {len(wanted)} graphs the cgame plays, from the mixer and '
      f'{len(weapon_files)} weapon files, for both motors')

# earthquake() sends its scale and radius as floats and its duration in time:
# the camera shake and the vibration both take the radius as it is.
quake = r'''
#include <assert.h>
#define COD2_FEATURE_RUMBLE 1
typedef float vec_t;
typedef struct { int time; vec_t angles2[3]; } entityState_t;
static float shakeScale, shakeRadius, quakeIntensity, quakeRadius;
static int shakeDuration, quakeDuration;
static const vec_t *shakeOrigin, *quakeOrigin;
void CG_StartShakeCamera(float scale, int duration, const vec_t *src, float radius)
{
    shakeScale = scale; shakeDuration = duration; shakeOrigin = src; shakeRadius = radius;
}
void CG_RumbleEarthquake(const vec_t *origin, float intensity, int duration, float radius)
{
    quakeOrigin = origin; quakeIntensity = intensity; quakeDuration = duration; quakeRadius = radius;
}
static void event(entityState_t *es, char *position)
{
    switch (0xc4) {
''' + case('0xc4') + r'''
    }
}
int main(void)
{
    entityState_t es = { 500, { 0.15f, 250.0f, 0.0f } };
    vec_t position[3] = { 1, 2, 3 };
    event(&es, (char *)position);
    assert(shakeScale == 0.15f && shakeDuration == 500 && shakeRadius == 250.0f && shakeOrigin == position);
    assert(quakeIntensity == 0.15f && quakeDuration == 500 && quakeRadius == 250.0f && quakeOrigin == position);
    return 0;
}
'''

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define COD2_FEATURE_RUMBLE 1
typedef int qboolean;
typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef struct { union { int enabled; int integer; float value; } current; } dvar_t;
enum { WEAPCLASS_RIFLE, WEAPCLASS_MG, WEAPCLASS_SMG, WEAPCLASS_SPREAD, WEAPCLASS_PISTOL, WEAPCLASS_GRENADE,
       WEAPCLASS_ROCKETLAUNCHER, WEAPCLASS_TURRET, WEAPCLASS_NON_PLAYER, WEAPCLASS_ITEM };
enum { WEAPTYPE_BULLET, WEAPTYPE_GRENADE, WEAPTYPE_PROJECTILE, WEAPTYPE_BINOCULARS };
typedef struct {
    const char *szInternalName;
    int weapType, weapClass, iFireTime, bSemiAuto, bBoltAction, iExplosionRadius;
    char *fireRumble, *meleeImpactRumble;
} WeaponDef;
typedef struct { int number, eType, weapon; } entityState_t;
typedef struct { int clientNum, eFlags, viewlocked_entNum, pm_type; } playerState_t;
typedef struct { playerState_t ps; } snapshot_t;
typedef struct { int clientNum; snapshot_t *nextSnap; struct { vec3_t vieworg; } refdef; } cg_t;
static snapshot_t snap;
static cg_t cgState;
cg_t *cg = &cgState;

static int nowMs;
int Sys_Milliseconds(void) { return nowMs; }
static float outLow, outHigh;
void CL_Gamepad_SetRumble(float low, float high) { outLow = low; outHigh = high; }

/* The graph files by path, missing ones included, and how often each opened. */
static struct { char path[64]; char text[160]; int missing, opens; } files[160];
static int fileCount;
int FS_FOpenFileByMode(const char *name, int *fh, int mode)
{
    int i;
    (void)mode;
    for (i = 0; i < fileCount && strcmp(files[i].path, name); ++i) {}
    if (i == fileCount) {
        snprintf(files[fileCount].path, sizeof(files[0].path), "%s", name);
        files[fileCount++].missing = 1;
    }
    files[i].opens++;
    *fh = files[i].missing ? 0 : i + 1;
    return files[i].missing ? -1 : (int)strlen(files[i].text);
}
int FS_Read(void *buffer, int len, int fh) { memcpy(buffer, files[fh - 1].text, len); return len; }
void FS_FCloseFile(int fh) { (void)fh; }
static int opens(const char *path)
{
    for (int i = 0; i < fileCount; ++i)
        if (!strcmp(files[i].path, path))
            return files[i].opens;
    return 0;
}
static void graph(const char *name, const char *high, const char *low)
{
    const char *knots[2] = { high, low }, *suffix[2] = { "_h", "_l" };
    for (int i = 0; i < 2; ++i) {
        int spaces = 0;
        for (const char *p = knots[i]; *p; ++p)
            spaces += *p == ' ';
        snprintf(files[fileCount].path, sizeof(files[0].path), "rumble/%s%s.rmb", name, suffix[i]);
        snprintf(files[fileCount].text, sizeof(files[0].text), "RUMBLEGRAPHFILE\n%d\n%s\n", (spaces + 1) / 2,
                 knots[i]);
        fileCount++;
    }
}

static char token[64];
const char *Com_Parse(const char **buf)
{
    const char *p = *buf;
    int n = 0;
    while (*p == ' ' || *p == '\n')
        p++;
    while (*p && *p != ' ' && *p != '\n' && n < 63)
        token[n++] = *p++;
    token[n] = '\0';
    *buf = p;
    return token;
}
void Com_BeginParseSession(const char *name) { (void)name; }
void Com_EndParseSession(void) {}
static int prints;
void Com_Printf(const char *fmt, ...) { (void)fmt; prints++; }
void Com_DPrintf(const char *fmt, ...) { (void)fmt; }
static dvar_t debugDvar;
const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags)
{
    (void)name; (void)flags; debugDvar.current.enabled = value; return &debugDvar;
}
static int commands;
static void (*playrumble)(void);
void Cmd_AddCommand(const char *cmd, void (*func)(void)) { assert(!strcmp(cmd, "playrumble")); commands++; playrumble = func; }
static const char *argument = "";
int Cmd_Argc(void) { return 2; }
void Cmd_ArgvBuffer(int arg, char *buf, int size) { assert(arg == 1); snprintf(buf, size, "%s", argument); }
static WeaponDef weapons[16];
static int numWeapons;
WeaponDef *BG_GetWeaponDef(int index) { assert(index > 0 && index <= numWeapons); return &weapons[index]; }
int BG_GetNumWeapons(void) { return numWeapons; }
float Vec3Distance(const vec_t *a, const vec_t *b)
{
    float x = a[0] - b[0], y = a[1] - b[1], z = a[2] - b[2];
    return sqrtf(x * x + y * y + z * z);
}
''' + header

checks = r'''
enum { THOMPSON = 1, KAR98K, GARAND, MG42, CAL30, PANZERSCHRECK, FRAG, SMOKE, SMINE, FASTGUN, CUSTOM, BINOCULARS,
       MISSING, SLOWAUTO, WEAPON_COUNT };
static void weapon(int index, const char *name, int type, int cls, int fireTime, int semi, int bolt, int radius,
                   const char *fire, const char *melee)
{
    weapons[index] = (WeaponDef){ name, type, cls, fireTime, semi, bolt, radius, (char *)fire, (char *)melee };
}
static int activeCount(void)
{
    int n = 0;
    for (int i = 0; i < MAX_ACTIVE_RUMBLES; ++i)
        n += g_activeRumbles[i].active;
    return n;
}
static void frame(int ms) { nowMs = ms; CG_UpdateRumble(); }
static int near(float value, float expected) { return fabsf(value - expected) < 0.011f; }
static void restart(int ms) { CG_InitRumble(); frame(ms); }
static void fire(int number, int eType, int index)
{
    entityState_t es = { number, eType, index };
    CG_RumbleFireEvent(&es);
}
static void at(float x) { cg->refdef.vieworg[0] = x; }

int main(void)
{
    static const vec3_t origin = { 0, 0, 0 }, close = { 50, 0, 0 }, away = { 300, 0, 0 };
    graph("autogun_fire", "0 0.6 0.25 0.6 0.4 0", "0 0.5 0.2 0.5 0.85 0");
    graph("rifle_fire", "0 0.5 1 0", "0 1 0.19 1 0.23 0");
    graph("mg42_fire", "0 0.4 1 0", "0 0.8 0.5 0");
    graph("30cal_fire", "0 0.4 1 0", "0 0.8 0.5 0");
    graph("panzerschreck_fire", "0 0.4 1 0", "0 0.8 0.5 0");
    graph("defaultweapon_fire", "0 0.4 1 0", "0 0.8 0.5 0");
    graph("artillery_quake", "0 0.55 0.16 0.55 0.75 0", "0 1 0.2 1 0.21 0");
    graph("damage_heavy", "0 0.5 0.105 0.3 1 0", "0 1 0.125 1 0.43 0");
    graph("melee_medium", "0 1 0.06 1 0.3 0", "0 1 0.08 1 0.16 0");
    graph("melee_light", "0 1 0.07 1 0.15 0", "0 1 0.03 1 0.1 0");
    for (int i = 0; i < 40; ++i) {
        char name[16];
        snprintf(name, sizeof(name), "extra%d", i);
        graph(name, "0 0.1 1 0", "0 0.1 1 0");
    }
    weapon(THOMPSON, "thompson_mp", WEAPTYPE_BULLET, WEAPCLASS_SMG, 86, 0, 0, 0, "autogun_fire", "melee_medium");
    weapon(KAR98K, "kar98k_mp", WEAPTYPE_BULLET, WEAPCLASS_RIFLE, 330, 1, 1, 0, "rifle_fire", "melee_light");
    weapon(GARAND, "m1garand_mp", WEAPTYPE_BULLET, WEAPCLASS_RIFLE, 135, 1, 0, 0, "rifle_fire", "melee_medium");
    weapon(MG42, "mg42_bipod_stand_mp", WEAPTYPE_BULLET, WEAPCLASS_TURRET, 50, 0, 0, 0, "", NULL);
    weapon(CAL30, "30cal_stand_mp", WEAPTYPE_BULLET, WEAPCLASS_TURRET, 100, 0, 0, 0, "", NULL);
    weapon(PANZERSCHRECK, "panzerschreck_mp", WEAPTYPE_PROJECTILE, WEAPCLASS_ROCKETLAUNCHER, 150, 0, 0, 256, "", "");
    weapon(FRAG, "frag_grenade_american_mp", WEAPTYPE_GRENADE, WEAPCLASS_GRENADE, 300, 0, 0, 256, NULL, NULL);
    weapon(SMOKE, "smoke_grenade_american_mp", WEAPTYPE_GRENADE, WEAPCLASS_GRENADE, 300, 0, 0, 0, NULL, NULL);
    weapon(SMINE, "browser_landmine_mp", WEAPTYPE_GRENADE, WEAPCLASS_GRENADE, 450, 0, 0, 192, NULL, NULL);
    weapon(FASTGUN, "fastgun_mp", WEAPTYPE_BULLET, WEAPCLASS_SMG, 30, 0, 0, 0, "autogun_fire", "melee_medium");
    weapon(CUSTOM, "custom_mp", WEAPTYPE_BULLET, WEAPCLASS_PISTOL, 125, 1, 0, 0, NULL, "");
    weapon(BINOCULARS, "binoculars_mp", WEAPTYPE_BINOCULARS, WEAPCLASS_ITEM, 100, 0, 0, 0, "autogun_fire", NULL);
    weapon(MISSING, "missing_mp", WEAPTYPE_BULLET, WEAPCLASS_RIFLE, 135, 1, 0, 0, "missing_fire", NULL);
    weapon(SLOWAUTO, "slowauto_mp", WEAPTYPE_BULLET, WEAPCLASS_MG, 400, 0, 0, 0, "autogun_fire", NULL);
    numWeapons = WEAPON_COUNT - 1;
    cg->clientNum = 3;
    snap.ps.clientNum = 3;
    cg->nextSnap = &snap;

    /* Automatic fire: each round kicks with its graph, spanning the fire
     * interval, over a floor held until 60 ms after the next round is due, so
     * a burst buzzes without a break and stops soon after its last round. */
    restart(1000);
    int edges = 0, wasOn = 0;
    for (int shot = 0; shot < 10; ++shot) {
        int start = 1000 + shot * 86;
        nowMs = start;
        fire(3, 1, THOMPSON);
        for (int t = start; t < start + 86; ++t) {
            frame(t);
            int on = outLow > 0.0f || outHigh > 0.0f;
            edges += on && !wasOn;
            wasOn = on;
            if (t == start)
                assert(near(outLow, 0.5f) && near(outHigh, 0.6f));
            assert(outLow >= 0.4f && outHigh >= 0.3f);
        }
    }
    assert(edges == 1);
    frame(1000 + 9 * 86 + 86 + 59);
    assert(outLow == 0.4f && outHigh == 0.3f && activeCount() == 0);
    frame(1000 + 9 * 86 + 86 + 60);
    assert(outLow == 0.0f && outHigh == 0.0f);

    /* A burst's span by action: single shots and bolts keep longer tails. */
    assert(CG_FireRumbleDuration(&weapons[THOMPSON]) == 86 && CG_FireRumbleDuration(&weapons[MG42]) == 50);
    assert(CG_FireRumbleDuration(&weapons[CAL30]) == 100 && CG_FireRumbleDuration(&weapons[FASTGUN]) == 50);
    assert(CG_FireRumbleDuration(&weapons[SLOWAUTO]) == 250 && CG_FireRumbleDuration(&weapons[GARAND]) == 250);
    assert(CG_FireRumbleDuration(&weapons[KAR98K]) == 400 && CG_FireRumbleDuration(&weapons[PANZERSCHRECK]) == 700);
    /* A single shot holds the strong motor at 0.7 and the weak at 0.5 for
     * 100 ms, under its graph: a short graph is still felt. */
    restart(3000);
    fire(3, 1, KAR98K);
    frame(3080);
    assert(near(outLow, 0.75f) && outHigh == 0.5f);
    frame(3099);
    assert(outLow == 0.7f && outHigh == 0.5f);
    frame(3100);
    assert(outLow == 0.0f && near(outHigh, 0.375f));
    frame(3400);
    assert(activeCount() == 0);
    restart(3500);
    fire(3, 1, GARAND);
    frame(3590);
    assert(outLow == 0.7f && outHigh == 0.5f);
    /* A new map, or the cgame shutting down, leaves no floor behind. */
    restart(3591);
    assert(outLow == 0.0f && outHigh == 0.0f);
    fire(3, 1, THOMPSON);
    CG_ShutdownRumble();
    frame(3592);
    assert(outLow == 0.0f && outHigh == 0.0f);

    /* The retail turrets and Panzerschreck name no fire rumble; their graphs
     * still play, and a gun with none feels the default weapon's. */
    restart(4000);
    snap.ps.eFlags = 0x100;
    snap.ps.viewlocked_entNum = 70;
    fire(70, 9, MG42);
    fire(70, 9, CAL30);
    fire(3, 1, PANZERSCHRECK);
    fire(3, 1, CUSTOM);
    assert(opens("rumble/mg42_fire_h.rmb") == 1 && opens("rumble/30cal_fire_h.rmb") == 1);
    assert(opens("rumble/panzerschreck_fire_h.rmb") == 1 && opens("rumble/defaultweapon_fire_h.rmb") == 1);
    assert(activeCount() == 4);
    frame(4000);
    assert(outLow == 1.0f && outHigh == 1.0f);

    /* A turret's rounds are the local player's only while they man it. */
    restart(5000);
    snap.ps.eFlags = 0;
    fire(70, 9, MG42);
    snap.ps.eFlags = 0x200;
    snap.ps.viewlocked_entNum = 71;
    fire(70, 9, MG42);
    assert(activeCount() == 0);
    snap.ps.viewlocked_entNum = 70;
    fire(70, 9, MG42);
    assert(activeCount() == 1);
    snap.ps.eFlags = 0;

    /* Another player's shots, a followed player's and the killcam's stay
     * still; so do grenades, binoculars and unknown weapons. */
    restart(6000);
    fire(4, 1, THOMPSON);
    snap.ps.clientNum = 4;
    fire(4, 1, THOMPSON);
    fire(3, 1, THOMPSON);
    snap.ps.clientNum = 5;
    fire(3, 1, THOMPSON);
    cg->nextSnap = NULL;
    fire(3, 1, THOMPSON);
    cg->nextSnap = &snap;
    snap.ps.clientNum = 3;
    fire(3, 1, FRAG);
    fire(3, 1, BINOCULARS);
    fire(3, 1, SMINE);
    fire(3, 1, 0);
    fire(3, 1, WEAPON_COUNT);
    assert(activeCount() == 0);
    fire(3, 1, THOMPSON);
    assert(activeCount() == 1);

    /* Blasts: full at the centre, half at half the reach of three radii,
     * nothing past it; smoke never; two at once saturate the motors. */
    restart(7000);
    at(0);
    CG_RumbleExplosion(origin, FRAG);
    frame(7000);
    assert(near(outLow, 1.0f) && near(outHigh, 0.55f));
    frame(7500);
    assert(outLow == 0.0f && near(outHigh, 0.233f));
    frame(8000);
    assert(activeCount() == 0);
    restart(9000);
    at(384);
    CG_RumbleExplosion(origin, FRAG);
    frame(9000);
    assert(near(outLow, 0.5f) && near(outHigh, 0.275f));
    restart(10000);
    at(768);
    CG_RumbleExplosion(origin, FRAG);
    at(0);
    CG_RumbleExplosion(origin, SMOKE);
    CG_RumbleExplosion(origin, 0);
    assert(activeCount() == 0);
    at(288);
    CG_RumbleExplosion(origin, SMINE);
    frame(10000);
    assert(near(outLow, 0.5f));
    restart(11000);
    at(0);
    CG_RumbleExplosion(origin, PANZERSCHRECK);
    CG_RumbleExplosion(origin, FRAG);
    frame(11000);
    assert(outLow == 1.0f && outHigh == 1.0f);
    restart(12000);
    snap.ps.clientNum = 5;
    CG_RumbleExplosion(origin, FRAG);
    snap.ps.clientNum = 3;
    assert(activeCount() == 0);

    /* earthquake(): its scale four times over within its radius, for its
     * duration. */
    restart(13000);
    CG_RumbleEarthquake(origin, 0.15f, 500, 250.0f);
    frame(13000);
    assert(near(outLow, 0.6f) && near(outHigh, 0.33f));
    frame(13499);
    assert(activeCount() == 1);
    frame(13500);
    assert(activeCount() == 0);
    at(250);
    CG_RumbleEarthquake(origin, 0.15f, 500, 250.0f);
    at(125);
    CG_RumbleEarthquake(origin, 0.15f, 500, -250.0f);
    CG_RumbleEarthquake(origin, 0.15f, 0, 250.0f);
    snap.ps.clientNum = 5;
    CG_RumbleEarthquake(origin, 0.15f, 500, 250.0f);
    snap.ps.clientNum = 3;
    assert(activeCount() == 0);
    CG_RumbleEarthquake(origin, 1.0f, 500, 250.0f);
    frame(13500);
    assert(near(outLow, 1.0f));
    at(0);

    /* Hits taken: 40% plus 1.2% a point, full from half the health. */
    restart(14000);
    CG_RumbleDamage(10);
    frame(14000);
    assert(near(outLow, 0.52f) && near(outHigh, 0.26f));
    restart(15000);
    CG_RumbleDamage(50);
    frame(15000);
    assert(near(outLow, 1.0f) && near(outHigh, 0.5f));
    restart(16000);
    CG_RumbleDamage(100);
    frame(16105);
    assert(near(outLow, 1.0f) && near(outHigh, 0.3f));
    restart(17000);
    CG_RumbleDamage(0);
    CG_RumbleDamage(-5);
    snap.ps.clientNum = 4;
    CG_RumbleDamage(50);
    snap.ps.clientNum = 3;
    assert(activeCount() == 0);

    /* Dying kicks fully, the killing hit having no damage feedback; staying
     * dead, respawning, spectating, the intermission, another player's state
     * and the killcam do not. */
    playerState_t alive = { 3, 0, 0, 0 }, dead = { 3, 0, 0, 6 }, spectator = { 3, 0, 0, 4 };
    playerState_t intermission = { 3, 0, 0, 5 }, otherDead = { 4, 0, 0, 6 };
    restart(18000);
    CG_RumbleTransition(&dead, &alive);
    frame(18000);
    assert(activeCount() == 1 && near(outLow, 1.0f) && near(outHigh, 0.5f));
    restart(19000);
    CG_RumbleTransition(&dead, &dead);
    CG_RumbleTransition(&alive, &dead);
    CG_RumbleTransition(&dead, &spectator);
    CG_RumbleTransition(&intermission, &alive);
    CG_RumbleTransition(&otherDead, &alive);
    snap.ps.clientNum = 5;
    CG_RumbleTransition(&dead, &alive);
    snap.ps.clientNum = 3;
    assert(activeCount() == 0);
    dead.pm_type = 7;
    CG_RumbleTransition(&dead, &alive);
    assert(activeCount() == 1);

    /* Melee: an impact near the view soon after the local player's swing is
     * theirs, once; far, late, someone else's swing, a killcam or a new map
     * feel nothing. */
    entityState_t swing = { 3, 1, KAR98K }, other = { 4, 1, KAR98K }, plain = { 3, 1, CUSTOM };
    restart(20000);
    int light = opens("rumble/melee_light_h.rmb");
    CG_RumbleMeleeSwing(&swing);
    nowMs = 20300;
    CG_RumbleMeleeImpact(close);
    assert(activeCount() == 1 && opens("rumble/melee_light_h.rmb") == light + 1);
    nowMs = 20400;
    CG_RumbleMeleeImpact(close);
    assert(activeCount() == 1);
    restart(21000);
    CG_RumbleMeleeSwing(&swing);
    CG_RumbleMeleeImpact(away);
    assert(activeCount() == 0);
    nowMs = 22001;
    CG_RumbleMeleeImpact(close);
    assert(activeCount() == 0);
    CG_RumbleMeleeSwing(&other);
    CG_RumbleMeleeImpact(close);
    assert(activeCount() == 0);
    CG_RumbleMeleeSwing(&swing);
    snap.ps.clientNum = 5;
    CG_RumbleMeleeImpact(close);
    snap.ps.clientNum = 3;
    assert(activeCount() == 0);
    CG_InitRumble();
    CG_RumbleMeleeImpact(close);
    assert(activeCount() == 0);
    int medium = opens("rumble/melee_medium_h.rmb");
    CG_RumbleMeleeSwing(&plain);
    CG_RumbleMeleeImpact(close);
    assert(activeCount() == 1 && opens("rumble/melee_medium_h.rmb") == medium + 1);

    /* A missing graph is looked for and reported once, never per shot;
     * playrumble still says so on the console each time it is asked. */
    restart(30000);
    prints = 0;
    int missing = opens("rumble/missing_fire_h.rmb");
    for (int i = 0; i < 10; ++i) {
        nowMs += 200;
        fire(3, 1, MISSING);
    }
    assert(prints == 2 && opens("rumble/missing_fire_h.rmb") == missing + 1 && activeCount() == 0);
    assert(CG_LoadRumble("missing_fire") < 0 && CG_LoadRumble("missing_fire") < 0);
    argument = "missing_fire";
    playrumble();
    playrumble();
    assert(prints == 4);
    argument = "autogun_fire";
    playrumble();
    assert(prints == 4 && activeCount() == 1 && g_activeRumbles[0].duration == 1000);
    CG_PlayRumble("rifle_fire");
    assert(g_activeRumbles[1].duration == 1000 && g_activeRumbles[1].scale == 1.0f);

    /* Room for every retail pair, and the command added once for all maps. */
    restart(40000);
    for (int i = 0; i < 40; ++i) {
        char name[16];
        snprintf(name, sizeof(name), "extra%d", i);
        assert(CG_LoadRumble(name) >= 0);
    }
    assert(commands == 1);

    /* With every slot taken, a new one replaces the closest to its own end. */
    restart(50000);
    for (int i = 0; i < 31; ++i)
        CG_RumbleExplosion(origin, FRAG);
    nowMs = 50010;
    fire(3, 1, THOMPSON);
    assert(activeCount() == 32 && g_activeRumbles[31].duration == 86);
    nowMs = 50050;
    CG_RumbleDamage(50);
    assert(g_activeRumbles[31].duration == 1000 && g_activeRumbles[31].startTime == 50050);
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
    # No floor, a burst's ending with its next round or long after, a single
    # shot's floor as a burst's or none, and floors that outlive a map.
    ('    if (now - g_rumbleFloorEnd < 0) {', '    if (0) {'),
    ('Sys_Milliseconds() + weapDef->iFireTime + RUMBLE_BURST_SLACK_MS', 'Sys_Milliseconds() + weapDef->iFireTime'),
    ('#    define RUMBLE_BURST_SLACK_MS 60', '#    define RUMBLE_BURST_SLACK_MS 600'),
    ('    if (!weapDef->bSemiAuto && !weapDef->bBoltAction && weapDef->weapClass',
     '    if (!weapDef->bBoltAction && weapDef->weapClass'),
    ('        g_rumbleFloorEnd = Sys_Milliseconds() + RUMBLE_KICK_MS;', '        g_rumbleFloorEnd = Sys_Milliseconds();'),
    ('    g_rumbleFloorLow = g_rumbleFloorHigh = 0.0f;\n    for (i = 0; i < MAX_ACTIVE_RUMBLES; ++i)\n        CG_InvalidateActiveRumble(i);\n\n',
     '    for (i = 0; i < MAX_ACTIVE_RUMBLES; ++i)\n        CG_InvalidateActiveRumble(i);\n\n'),
    ('    g_rumbleFloorLow = g_rumbleFloorHigh = 0.0f;\n    CL_Gamepad_SetRumble(0.0f, 0.0f);',
     '    CL_Gamepad_SetRumble(0.0f, 0.0f);'),
    ('    return weapDef->iFireTime < RUMBLE_SEMI_FIRE_MS ? weapDef->iFireTime : RUMBLE_SEMI_FIRE_MS;',
     '    return RUMBLE_DURATION_MS;'),
    ('        t = (float)(now - ar->startTime) / (float)ar->duration;',
     '        t = (float)(now - ar->startTime) / (float)RUMBLE_DURATION_MS;'),
    ('        high += CG_EvalRumbleGraph(&info->high, t) * ar->scale;',
     '        high += CG_EvalRumbleGraph(&info->high, t);'),
    ('        low += CG_EvalRumbleGraph(&info->low, t) * ar->scale;', '        low += CG_EvalRumbleGraph(&info->low, t);'),
    ('    return cg->nextSnap && cg->nextSnap->ps.clientNum == cg->clientNum;', '    return cg->nextSnap != 0;'),
    ('!(snap->ps.eFlags & 0x300) || snap->ps.viewlocked_entNum != es->number', '!(snap->ps.eFlags & 0x300)'),
    ('!(snap->ps.eFlags & 0x300) || snap->ps.viewlocked_entNum != es->number', 'snap->ps.viewlocked_entNum != es->number'),
    ('                       : es->number != cg->clientNum)', '                       : 0)'),
    ('(weapDef->weapType != WEAPTYPE_BULLET && weapDef->weapClass != WEAPCLASS_ROCKETLAUNCHER)',
     '(weapDef->weapClass == WEAPCLASS_GRENADE)'),
    ('(weapDef->weapType != WEAPTYPE_BULLET && weapDef->weapClass != WEAPCLASS_ROCKETLAUNCHER)',
     '(weapDef->weapType != WEAPTYPE_BULLET)'),
    ('    if (weapDef->fireRumble && weapDef->fireRumble[0])', '    if (weapDef->fireRumble)'),
    ('        { "mg42", "mg42_fire" },\n', ''),
    ('    return "defaultweapon_fire";', '    return "";'),
    ('    if (weapDef->weapClass == WEAPCLASS_ROCKETLAUNCHER)\n        return RUMBLE_ROCKET_FIRE_MS;\n', ''),
    ('    if (weapDef->bBoltAction)\n        return RUMBLE_BOLT_FIRE_MS;\n', ''),
    ('    if (weapDef->iFireTime < RUMBLE_SHOT_MIN_MS)\n        return RUMBLE_SHOT_MIN_MS;\n', ''),
    ('#    define RUMBLE_EXPLOSION_REACH 3.0f', '#    define RUMBLE_EXPLOSION_REACH 2.0f'),
    ('    if (!weapDef || weapDef->iExplosionRadius <= 0 || !CG_RumbleOwnView())',
     '    if (!weapDef || weapDef->iExplosionRadius <= 0)'),
    ('#    define RUMBLE_EARTHQUAKE_GAIN 4.0f', '#    define RUMBLE_EARTHQUAKE_GAIN 1.0f'),
    ('    if (!(radius > 0.0f) || !CG_RumbleOwnView())', '    if (!CG_RumbleOwnView())'),
    ('    if (!(radius > 0.0f) || !CG_RumbleOwnView())', '    if (!(radius > 0.0f))'),
    ('                          RUMBLE_DAMAGE_BASE + (float)damage * RUMBLE_DAMAGE_PER_POINT);', '                          1.0f);'),
    ('    if (damage <= 0 || !CG_RumbleOwnView())', '    if (damage <= 0)'),
    ('ps->clientNum == ops->clientNum && ops->pm_type < 4', 'ops->pm_type < 4'),
    ('ops->pm_type < 4 && ps->pm_type > 5', 'ps->pm_type > 5'),
    ('ops->pm_type < 4 && ps->pm_type > 5', 'ops->pm_type < 4 && ps->pm_type >= 5'),
    ('ps->pm_type > 5 && CG_RumbleOwnView())', 'ps->pm_type > 5)'),
    ('    g_activeRumbles[slot].scale = scale < 1.0f ? scale : 1.0f;', '    g_activeRumbles[slot].scale = scale;'),
    ('Sys_Milliseconds() - g_rumbleMeleeTime > RUMBLE_MELEE_WINDOW_MS || ', ''),
    ('RUMBLE_MELEE_WINDOW_MS || !CG_RumbleOwnView() ||', 'RUMBLE_MELEE_WINDOW_MS ||'),
    ('        Vec3Distance(origin, cg->refdef.vieworg) > RUMBLE_MELEE_REACH)', '        0)'),
    ('    g_rumbleMeleeWeapon = 0;\n    CG_PlayRumbleInternal(', '    CG_PlayRumbleInternal('),
    ('    if (es->number != cg->clientNum || !CG_RumbleOwnView())\n        return;\n    g_rumbleMeleeTime',
     '    if (!CG_RumbleOwnView())\n        return;\n    g_rumbleMeleeTime'),
    ('    g_rumbleInfoCount = 0;\n    g_rumbleMeleeWeapon = 0;\n', '    g_rumbleInfoCount = 0;\n'),
    ('                              : RUMBLE_MELEE_GRAPH,', '                              : "",'),
    ('        return g_rumbleInfos[idx].loaded ? idx : -1;', '        return idx;'),
    ('''        Com_Printf("^1Could not load rumble info file '%s'\\n", name);\n        return -1;''',
     '''        Com_Printf("^1Could not load rumble info file '%s'\\n", name);\n        g_rumbleInfoCount--;\n'''
     '''        return -1;'''),
    ('#    define MAX_RUMBLE_INFOS 64', '#    define MAX_RUMBLE_INFOS 32'),
    ('    if (!commandAdded) {', '    if (1) {'),
    ('        if (now - g_activeRumbles[i].startTime >= g_activeRumbles[i].duration)',
     '        if (now - g_activeRumbles[i].startTime >= RUMBLE_DURATION_MS)'),
    ('        remaining = g_activeRumbles[i].duration - (now - g_activeRumbles[i].startTime);',
     '        remaining = RUMBLE_DURATION_MS - (now - g_activeRumbles[i].startTime);'),
]
quake_mutants = [
    ('            float intensity = es->angles2[0];\n            float radius = es->angles2[1];\n'
     '            CG_StartShakeCamera(intensity, es->time, (const vec_t *)position, radius);',
     '            int duration = es->angles2[1];\n            float intensity = es->angles2[0];\n'
     '            float radius = es->angles2[1];\n'
     '            CG_StartShakeCamera(intensity, es->time, (const vec_t *)position, *(float *)&duration);'),
    ('CG_RumbleEarthquake((const vec_t *)position, intensity, es->time, radius);',
     'CG_RumbleEarthquake((const vec_t *)position, intensity, 1000, radius);'),
]
variants = [(support + body + '\n' + checks, None)]
variants += [(support + mutate(body, old, new) + '\n' + checks, (old, new)) for old, new in mutants]
variants += [(mutate(quake, old, new), (old, new)) for old, new in quake_mutants]
variants.insert(1, (quake, None))
with tempfile.TemporaryDirectory(prefix='cod2-rumble-') as directory:
    path = Path(directory) / 'rumble.c'
    binary = Path(directory) / 'rumble'
    for index, (source, mutant) in enumerate(variants):
        path.write_text(source)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wall', '-Werror' if mutant is None else '-w', '-Wno-unused-function', str(path), '-o',
                        str(binary), '-lm'], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (mutant is None), (index, mutant, result.stdout[-500:],
                                                               result.stderr[-2000:])
print('PASS: the local player feels every round (an automatic gun\'s graph spans its fire interval over a floor held '
      'until 60 ms past its next round, so a burst never gaps; single shots 250 ms over a 100 ms kick, bolts 400, '
      'rockets 700), turrets they man, the Panzerschreck and guns naming no graph included; '
      'blasts full at the centre and fading to nothing at three radii, smoke never; earthquake() by its scale, '
      'radius and duration, its camera shake taking the radius as sent; hits by their size, dying fully; their own melee '
      'impacts once a swing; nothing for other players, followed players or the killcam; missing graphs reported '
      f'once; room for every retail pair; {len(mutants) + len(quake_mutants)} failing regression mutants')
