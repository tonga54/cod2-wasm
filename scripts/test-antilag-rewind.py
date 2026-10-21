#!/usr/bin/env python3
"""A shot meets other players where its shooter saw them.

Remote players are drawn at the shooter's command time, so Bullet_Fire moves
every playing client to its archived position at that time for the bullet's
traces, however little the command trails the server, and puts them back
after. This runs the production Bullet_Fire with the archive, linking and
traces stubbed: lags under and over a server frame, none or negative (a bot's
command carries the coming frame's time), spectators, connecting clients and
clients without history, a disabled g_antilag, and pellets.
"""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/game_mp/g_weapon_mp.c').read_text()


def function(text, name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;{]*?\)\n\{', text)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


bullet_fire = function(source, 'Bullet_Fire')
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
typedef struct { struct { int enabled; } current; } dvar_t;
enum { CON_DISCONNECTED, CON_CONNECTING, CON_CONNECTED };
enum { SESS_STATE_PLAYING, SESS_STATE_DEAD, SESS_STATE_SPECTATOR, SESS_STATE_INTERMISSION };
enum { WEAPCLASS_RIFLE, WEAPCLASS_SPREAD = 3 };
typedef struct { struct { int connected, sessionState; } sess; } gclient_t;
typedef struct gentity_s { struct { vec3_t currentOrigin; } r; int linked; } gentity_s;
typedef gentity_s gentity_t;
typedef struct { int weapClass, shotCount; float fMinDamageRange; } WeaponDef;
typedef struct { WeaponDef *weapDef; vec3_t muzzleTrace; } weaponParms;

#define CLIENTS 8
static gclient_t clients[CLIENTS];
static gentity_t entities[CLIENTS];
static void *imp_g_entities = entities;
static struct { int time, maxclients; gclient_t *clients; } level;
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
static int unbalanced;
static void SV_UnlinkEntity(gentity_t *ent) { unbalanced += !ent->linked; ent->linked = 0; }
static void SV_LinkEntity(gentity_t *ent) { unbalanced += ent->linked; ent->linked = 1; }
static void Bullet_Endpos(float spread, vec_t *end, const weaponParms *wp, float maxRange)
{
    (void)spread; (void)wp;
    end[0] = end[1] = end[2] = maxRange;
}
/* Where every client stood for each trace. */
static vec3_t traced[16][CLIENTS];
static int traces;
static void Bullet_Fire_Extended(const gentity_t *source, gentity_s (*attacker)[16], vec_t *start, vec_t *end,
                                 float damageMultiplier, int recursion, const weaponParms *wp,
                                 const gentity_t *weaponEnt, const int gametime)
{
    (void)source; (void)attacker; (void)start; (void)end; (void)damageMultiplier; (void)recursion; (void)wp;
    (void)weaponEnt; (void)gametime;
    for (int n = 0; n < CLIENTS; n++)
        memcpy(traced[traces][n], entities[n].r.currentOrigin, sizeof(vec3_t));
    traces++;
}
'''

checks = r'''
static void Current(int n, vec_t *pos)
{
    for (int i = 0; i < 3; i++)
        pos[i] = -50.0f - 10.0f * n - (float)i;
}

int main(void)
{
    static const int lags[] = { -33, 0, 1, 10, 32, 33, 34, 100, 1000 };
    WeaponDef rifle = { WEAPCLASS_RIFLE, 1, 1024.0f }, shotgun = { WEAPCLASS_SPREAD, 5, 384.0f };
    int failures = 0, shots = 0;

    level.maxclients = CLIENTS;
    level.clients = clients;
    for (int enabled = 0; enabled < 2; enabled++) {
        for (unsigned k = 0; k < sizeof(lags) / sizeof(lags[0]); k++) {
            for (int pellets = 0; pellets < 2; pellets++) {
                weaponParms wp;
                int expectRewind = enabled && lags[k] > 0;

                memset(&wp, 0, sizeof(wp));
                wp.weapDef = pellets ? &shotgun : &rifle;
                antilagDvar.current.enabled = enabled;
                level.time = 50000;
                /* 0 shoots; 1-4 and 7 play, 7 without history; 5 watches; 6 connects. */
                for (int n = 0; n < CLIENTS; n++) {
                    clients[n].sess.connected = n == 6 ? CON_CONNECTING : CON_CONNECTED;
                    clients[n].sess.sessionState = n == 5 ? SESS_STATE_SPECTATOR : SESS_STATE_PLAYING;
                    Current(n, entities[n].r.currentOrigin);
                    entities[n].linked = 1;
                }
                traces = unbalanced = 0;
                askedTime = -1;
                Bullet_Fire((gentity_s (*)[16])&entities[0], 0.0f, &wp, (gentity_s (*)[16])&entities[0],
                            level.time - lags[k]);
                if (traces != (pellets ? shotgun.shotCount : 1) || unbalanced) {
                    failures++;
                    printf("lag %d: %d traces, %d unbalanced links\n", lags[k], traces, unbalanced);
                }
                if (expectRewind && askedTime != level.time - lags[k]) {
                    failures++;
                    printf("lag %d: history asked for %d\n", lags[k], askedTime);
                }
                for (int t = 0; t < traces; t++) {
                    for (int n = 0; n < CLIENTS; n++) {
                        vec3_t want;
                        if (expectRewind && n != 5 && n != 6 && n != 7)
                            Archived(n, level.time - lags[k], want);
                        else
                            Current(n, want);
                        if (memcmp(want, traced[t][n], sizeof(vec3_t))) {
                            if (failures++ < 8)
                                printf("antilag %d lag %d trace %d: client %d at %.2f, expected %.2f\n", enabled,
                                       lags[k], t, n, traced[t][n][0], want[0]);
                        }
                    }
                }
                /* Back where they are, linked, after the shot. */
                for (int n = 0; n < CLIENTS; n++) {
                    vec3_t now;
                    Current(n, now);
                    if (memcmp(now, entities[n].r.currentOrigin, sizeof(vec3_t)) || !entities[n].linked) {
                        if (failures++ < 8)
                            printf("lag %d: client %d left at %.2f\n", lags[k], n, entities[n].r.currentOrigin[0]);
                    }
                }
                shots++;
            }
        }
    }
    printf("%d\n", shots);
    return failures != 0;
}
'''

mutants = [
    # As it was: no rewind for a command less than a 30 Hz frame behind.
    ('        if (level.time - gametime > 0) {', '        if (level.time - gametime > 1000 / 30) {'),
    # A bot's command, which carries the coming frame's time, rewinds nothing.
    ('        if (level.time - gametime > 0) {', '        if (level.time - gametime >= -33) {'),
    ('            if (!movedClients[clientNum])\n                continue;\n', ''),
    ('                if (client->sess.sessionState != SESS_STATE_PLAYING)\n                    continue;\n', ''),
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
            shots = int(result.stdout.split()[-1])
        else:
            assert result.returncode == 1, f'mutant survived or crashed: {after!r}\n{result.stdout}{result.stderr[-2000:]}'

print(f'PASS: {shots} shots rewind every playing client with history to the shooter\'s command time for each trace, '
      f'from 1 ms behind the server, none with no lag, a bot\'s command time or g_antilag off, and put them back; '
      f'{len(mutants)} mutants fail')
