#!/usr/bin/env python3
"""A shot meets other players where its shooter saw them.

Players the shooter's command names in its view (bg_shot_view.h) are moved
into the pose its playback drew them in: origin, facing, view angles, lean and
move direction, with their skeletons rebuilt for the traces. Every other
playing client is moved to its archived position at the command time, however
little the command trails the server. All are put back after, their client
info whole. This runs the production Bullet_Fire with the archive, the views,
the poses, linking and traces stubbed: lags under and over a server frame,
none or negative (a bot's command carries the coming frame's time),
spectators, connecting clients and clients without history, poses that fail,
a disabled g_antilag, and pellets.
"""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/game_mp/g_weapon_mp.c').read_text()
header = (root / 'src/headers/PC/bgame/bg_shot_view.h').read_text()


def function(text, name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;{]*?\)\n\{', text)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


bullet_fire = function(source, 'Bullet_Fire')
posed_start = source.index('typedef struct {\n    int clientNum;\n    vec3_t angles;\n    clientInfo_t ci;\n} gShotPosed_t;')
posed_type = source[posed_start:source.index('} gShotPosed_t;') + len('} gShotPosed_t;')] + '\n'
# The shooter's command time is its last one, which FireWeaponAntiLag hands on.
assert 'FireWeaponAntiLag(ent, client->lastServerTime);' in (root / 'src/PC/game_mp/g_active_mp.c').read_text()
assert 'Bullet_Fire(ent, spread, &wp, ent, gametime);' in function(source, 'FireWeaponAntiLag')

support = r'''
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef float vec_t;
typedef vec_t vec3_t[3];
typedef unsigned char byte;
typedef int Bool;
typedef int qboolean;
typedef struct { struct { int enabled; } current; } dvar_t;
enum { CON_DISCONNECTED, CON_CONNECTING, CON_CONNECTED };
enum { SESS_STATE_PLAYING, SESS_STATE_DEAD, SESS_STATE_SPECTATOR, SESS_STATE_INTERMISSION };
enum { WEAPCLASS_RIFLE, WEAPCLASS_SPREAD = 3 };
typedef struct { struct { int connected, sessionState; } sess; } gclient_t;
typedef struct gentity_s {
    struct { int number, clientNum; } s;
    struct { vec3_t currentOrigin, currentAngles; } r;
    int linked;
} gentity_s;
typedef gentity_s gentity_t;
/* The fields a pose sets, and others it must leave as they were. */
typedef struct { int legsYaw; vec3_t playerAngles; float lerpLean, lerpMoveDir; int torsoAnim; } clientInfo_t;
typedef struct { int weapClass, shotCount; float fMinDamageRange; } WeaponDef;
typedef struct { WeaponDef *weapDef; vec3_t muzzleTrace; } weaponParms;
''' + header + r'''
#define CLIENTS 8
static gclient_t clients[CLIENTS];
static gentity_t entities[CLIENTS];
static void *imp_g_entities = entities;
static struct { int time, maxclients; gclient_t *clients; } level;
static struct { clientInfo_t clientinfo[64]; } level_bgs;
static dvar_t antilagDvar;
static const dvar_t *antilagPointer = &antilagDvar;
static void *imp_g_antilag = &antilagPointer;

/* Where the archive has each client at a time; client 7 has no history. */
static void Archived(int clientNum, int gametime, vec_t *pos)
{
    for (int i = 0; i < 3; i++)
        pos[i] = 100.0f * clientNum + 0.19f * (float)(gametime % 10000) + 7.0f * i;
}
static int askedTime;
static Bool SV_GetClientPositionAtTime(int clientNum, int gametime, vec_t *pos)
{
    askedTime = gametime;
    if (clientNum == 7)
        return 0;
    Archived(clientNum, gametime, pos);
    return 1;
}
/* The shooter's view for its command, and where it drew each player. */
static bgShotView_t shotView;
static int viewShooter = -1, viewTime;
static const bgShotView_t *SV_GetShotView(int clientNum, int serverTime)
{
    viewShooter = clientNum;
    viewTime = serverTime;
    return shotView.count ? &shotView : NULL;
}
static void Drawn(int clientNum, bgPlayerPose_t *pose)
{
    for (int i = 0; i < 3; i++) {
        pose->origin[i] = -1000.0f - 50.0f * clientNum - (float)i;
        pose->angles[i] = 10.0f * clientNum + (float)i + 1.0f;
    }
    pose->lean = 0.25f * clientNum;
    pose->moveDir = 45.0f + clientNum;
}
/* Client 4's samples are gone, so its pose fails. */
static qboolean G_ShotViewPose(int clientNum, const bgShotViewEntry_t *entry, bgPlayerPose_t *pose)
{
    if (clientNum == 4 || entry->clientNum != clientNum)
        return 0;
    Drawn(clientNum, pose);
    return 1;
}
static qboolean G_ShotTrace(void) { return 0; }
static void G_TraceShotPose(int shooter, int target, const bgShotViewEntry_t *entry, const bgPlayerPose_t *pose,
                            int gametime)
{
    (void)shooter; (void)target; (void)entry; (void)pose; (void)gametime;
}
static int unbalanced;
static void SV_UnlinkEntity(gentity_t *ent) { unbalanced += !ent->linked; ent->linked = 0; }
static void SV_LinkEntity(gentity_t *ent) { unbalanced += ent->linked; ent->linked = 1; }
static void Bullet_Endpos(float spread, vec_t *end, const weaponParms *wp, float maxRange)
{
    (void)spread; (void)wp;
    end[0] = end[1] = end[2] = maxRange;
}
/* Where every client stood, faced and posed for each trace, and whether its
 * skeleton had been rebuilt since it was posed. */
static vec3_t traced[16][CLIENTS], tracedAngles[16][CLIENTS];
static clientInfo_t tracedInfo[16][CLIENTS];
static int traces, resets, resetsBeforeTrace[16];
static void SV_ResetSkeletonCache(void) { resets++; }
static void Bullet_Fire_Extended(const gentity_t *source, gentity_s (*attacker)[16], vec_t *start, vec_t *end,
                                 float damageMultiplier, int recursion, const weaponParms *wp,
                                 const gentity_t *weaponEnt, const int gametime)
{
    (void)source; (void)attacker; (void)start; (void)end; (void)damageMultiplier; (void)recursion; (void)wp;
    (void)weaponEnt; (void)gametime;
    for (int n = 0; n < CLIENTS; n++) {
        memcpy(traced[traces][n], entities[n].r.currentOrigin, sizeof(vec3_t));
        memcpy(tracedAngles[traces][n], entities[n].r.currentAngles, sizeof(vec3_t));
        tracedInfo[traces][n] = level_bgs.clientinfo[n];
    }
    resetsBeforeTrace[traces] = resets;
    traces++;
}
''' + posed_type

checks = r'''
static void Current(int n, vec_t *pos, vec_t *angles, clientInfo_t *ci)
{
    for (int i = 0; i < 3; i++) {
        pos[i] = -50.0f - 10.0f * n - (float)i;
        angles[i] = (i == 1) ? 90.0f + n : 0.0f;
    }
    memset(ci, 0, sizeof(*ci));
    ci->legsYaw = 7 * n;
    ci->torsoAnim = 11 * n;
    for (int i = 0; i < 3; i++)
        ci->playerAngles[i] = 3.0f * n + (float)i;
    ci->lerpLean = -0.5f;
    ci->lerpMoveDir = -n;
}

static int failures;
static void Fail(const char *what, int lag, int view, int n)
{
    if (failures++ < 12)
        printf("%s: lag %d view %d client %d\n", what, lag, view, n);
}

int main(void)
{
    static const int lags[] = { -33, 0, 1, 10, 32, 33, 34, 100, 1000 };
    WeaponDef rifle = { WEAPCLASS_RIFLE, 1, 1024.0f }, shotgun = { WEAPCLASS_SPREAD, 5, 384.0f };
    int shots = 0, posedShots = 0;

    level.maxclients = CLIENTS;
    level.clients = clients;
    for (int enabled = 0; enabled < 2; enabled++) {
        for (unsigned k = 0; k < sizeof(lags) / sizeof(lags[0]); k++) {
            for (int pellets = 0; pellets < 2; pellets++) {
                for (int viewed = 0; viewed < 2; viewed++) {
                    weaponParms wp;
                    int expectRewind = enabled && lags[k] > 0;

                    memset(&wp, 0, sizeof(wp));
                    wp.weapDef = pellets ? &shotgun : &rifle;
                    antilagDvar.current.enabled = enabled;
                    level.time = 50000;
                    /* 0 shoots; 1-4 and 7 play, 7 without history; 5 watches; 6 connects.
                     * The view names 1 and 3, which pose, 4, whose pose fails, and the
                     * watcher and the connecting client, which must not move. */
                    memset(&shotView, 0, sizeof(shotView));
                    if (viewed) {
                        static const int named[] = { 3, 5, 4, 1 };
                        shotView.serverTime = level.time - lags[k];
                        shotView.count = 4;
                        for (int e = 0; e < 4; e++)
                            shotView.entries[e].clientNum = named[e];
                        shotView.entries[1].clientNum = 6;
                    }
                    for (int n = 0; n < CLIENTS; n++) {
                        clients[n].sess.connected = n == 6 ? CON_CONNECTING : CON_CONNECTED;
                        clients[n].sess.sessionState = n == 5 ? SESS_STATE_SPECTATOR : SESS_STATE_PLAYING;
                        entities[n].s.number = entities[n].s.clientNum = n;
                        Current(n, entities[n].r.currentOrigin, entities[n].r.currentAngles, &level_bgs.clientinfo[n]);
                        entities[n].linked = 1;
                    }
                    traces = unbalanced = resets = 0;
                    askedTime = viewShooter = -1;
                    Bullet_Fire((gentity_s (*)[16])&entities[0], 0.0f, &wp, (gentity_s (*)[16])&entities[0],
                                level.time - lags[k]);
                    int expectPose = enabled && viewed;
                    if (traces != (pellets ? shotgun.shotCount : 1) || unbalanced)
                        Fail("traces or links", lags[k], viewed, -1);
                    if (enabled && (viewShooter != 0 || viewTime != level.time - lags[k]))
                        Fail("view asked for the wrong command", lags[k], viewed, viewShooter);
                    if (!enabled && viewShooter != -1)
                        Fail("view asked with g_antilag off", lags[k], viewed, viewShooter);
                    if (expectRewind && askedTime != level.time - lags[k])
                        Fail("history asked for the wrong time", lags[k], viewed, askedTime);
                    /* Skeletons rebuilt before the posed traces and after, only then. */
                    if (resets != (expectPose ? 2 : 0) || (expectPose && resetsBeforeTrace[0] != 1))
                        Fail("skeleton cache", lags[k], viewed, resets);
                    for (int t = 0; t < traces; t++) {
                        for (int n = 0; n < CLIENTS; n++) {
                            vec3_t want, wantAngles;
                            clientInfo_t wantInfo;
                            Current(n, want, wantAngles, &wantInfo);
                            if (expectPose && (n == 1 || n == 3)) {
                                bgPlayerPose_t pose;
                                Drawn(n, &pose);
                                memcpy(want, pose.origin, sizeof(vec3_t));
                                wantAngles[0] = wantAngles[2] = 0.0f;
                                wantAngles[1] = pose.angles[1];
                                memcpy(wantInfo.playerAngles, pose.angles, sizeof(vec3_t));
                                wantInfo.lerpLean = pose.lean;
                                wantInfo.lerpMoveDir = pose.moveDir;
                            } else if (expectRewind && n != 5 && n != 6 && n != 7) {
                                Archived(n, level.time - lags[k], want);
                            }
                            if (memcmp(want, traced[t][n], sizeof(vec3_t)))
                                Fail("traced position", lags[k], viewed, n);
                            if (memcmp(wantAngles, tracedAngles[t][n], sizeof(vec3_t)))
                                Fail("traced facing", lags[k], viewed, n);
                            if (memcmp(&wantInfo, &tracedInfo[t][n], sizeof(clientInfo_t)))
                                Fail("traced pose", lags[k], viewed, n);
                        }
                    }
                    /* Back where they were, as they were, linked, after the shot. */
                    for (int n = 0; n < CLIENTS; n++) {
                        vec3_t now, nowAngles;
                        clientInfo_t nowInfo;
                        Current(n, now, nowAngles, &nowInfo);
                        if (memcmp(now, entities[n].r.currentOrigin, sizeof(vec3_t)) ||
                            memcmp(nowAngles, entities[n].r.currentAngles, sizeof(vec3_t)) ||
                            memcmp(&nowInfo, &level_bgs.clientinfo[n], sizeof(clientInfo_t)) || !entities[n].linked)
                            Fail("left moved", lags[k], viewed, n);
                    }
                    shots++;
                    posedShots += expectPose;
                }
            }
        }
    }
    printf("%d %d\n", shots, posedShots);
    return failures != 0;
}
'''

mutants = [
    # As it was: no rewind for a command less than a 30 Hz frame behind.
    ('            if (level.time - gametime <= 0)\n', '            if (level.time - gametime <= 1000 / 30)\n'),
    # A bot's command, which carries the coming frame's time, rewinds nothing.
    ('            if (level.time - gametime <= 0)\n', '            if (level.time - gametime < -33)\n'),
    ('            if (!movedClients[clientNum])\n                continue;\n', ''),
    ('            if (client->sess.sessionState != SESS_STATE_PLAYING)\n                continue;\n', ''),
    # The pose only for a command behind the server.
    ('            if (entry && posedCount < BG_SHOT_VIEW_ENTRIES && G_ShotViewPose(clientNum, entry, &pose)) {',
     '            if (level.time - gametime > 0 && entry && posedCount < BG_SHOT_VIEW_ENTRIES &&\n'
     '                G_ShotViewPose(clientNum, entry, &pose)) {'),
    # Traced with the skeletons built from the pose they had.
    ('        if (posedCount)\n            SV_ResetSkeletonCache();\n    }\n\n    if (wp->weapDef', '    }\n\n    if (wp->weapDef'),
    # Later traces in the frame with the skeletons of the pose.
    ('        if (posedCount)\n            SV_ResetSkeletonCache();\n    }\n}', '    }\n}'),
    ('                    level_bgs.clientinfo[clientEnt->s.clientNum] = posed[i].ci;\n', ''),
    ('                clientEnt->r.currentAngles[0] = 0.0f;\n', '                clientEnt->r.currentAngles[0] = pose.angles[0];\n'),
    ('                ci->lerpLean = pose.lean;\n', ''),
    ('                    clientEnt->r.currentAngles[1] = posed[i].angles[1];\n', ''),
]

with tempfile.TemporaryDirectory(prefix='cod2-antilag-rewind-') as directory:
    path = Path(directory)
    for index, (before, after) in enumerate([(None, None)] + mutants):
        if before is None:
            body = bullet_fire
        else:
            assert bullet_fire.count(before) == 1, before
            body = bullet_fire.replace(before, after)
        (path / 'test.c').write_text(support + body + checks)
        flags = ['-Wall', '-Werror'] if before is None else ['-w']
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        *flags, str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True)
        if before is None:
            assert result.returncode == 0, result.stdout + result.stderr
            shots, posed = map(int, result.stdout.split()[-2:])
        else:
            assert result.returncode == 1, f'mutant survived or crashed: {after!r}\n{result.stdout}{result.stderr[-2000:]}'

print(f'PASS: {shots} shots, {posed} with a view: players the view names are traced in the pose the shooter drew '
      f'them in, origin, facing, view angles, lean and move direction, on skeletons rebuilt before and after; the '
      f'other playing clients with history at the shooter\'s command time from 1 ms behind the server, none with no '
      f'lag, a bot\'s command time or g_antilag off; all put back whole; {len(mutants)} mutants fail')
