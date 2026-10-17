#!/usr/bin/env python3
"""Run the production match points module and scoreboard message for every mode."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
module = (root / 'src/PC/game_mp/g_points_mp.c').read_text()
header = (root / 'src/headers/PC/game_mp/g_points.h').read_text()
commands = (root / 'src/PC/game_mp/g_cmds_mp.c').read_text()


def function(source, signature):
    assert source.count(signature) == 1, signature
    start = source.index(signature)
    brace = source.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def definition(source, name):
    lines = [line for line in source.splitlines() if re.match(r'(static )?[a-z][\w ]*\*?\s*' + re.escape(name) + r'\(', line)
             and not line.rstrip().endswith(';')]
    assert len(lines) == 1, (name, lines)
    return function(source, lines[0] + '\n{')


for include in ('#include "common_types.h"\n', '#include "imports.h"\n', '#include "headers/PC/game_mp/g_points.h"\n'):
    assert module.count(include) == 1, include
body = module.replace('#include "common_types.h"\n', '').replace('#include "imports.h"\n', '')
body = body.replace('#include "headers/PC/game_mp/g_points.h"\n', header)
scoreboard = function(commands, 'void DeathmatchScoreboardMessage(gentity_t *ent)\n{')

# Every hook sits where the gameplay event happens.
hooks = [
    ('src/PC/game_mp/g_combat_mp.c', 'player_die', 'G_PointsPlayerKilled(self, attacker, meansOfDeath, hitLoc);',
     'attacker = &g_entities[ownerNum];', 'Scr_PlayerKilled(self, inflictor, attacker'),
    ('src/PC/game_mp/g_client_script_cmd_mp.c', 'PlayerCmd_finishPlayerDamage', 'G_PointsPlayerDamaged(pSelf, attacker, damage);',
     'client->ps.damageDuration = client->ps.damageTimer;', 'pSelf->health -= damage;'),
    ('src/PC/game_mp/g_client_fields_mp.c', 'ClientScr_SetScore',
     'G_PointsScoreChanged(client->ps.clientNum, oldScore, client->sess.score);', 'client->sess.score = Scr_GetInt(0);', 'CalculateRanks()'),
    ('src/PC/game_mp/g_scr_main_mp.c', 'GScr_SetTeamScore',
     'G_PointsTeamScoresChanged(level.initializing, level.teamScores[TEAM_AXIS], level.teamScores[TEAM_ALLIES]);',
     "va(\"%c %i\", 'G', score)", 'level.bUpdateScoresForIntermission = 1;'),
    ('src/PC/game_mp/g_scr_main_mp.c', 'GScr_LogPrint', 'G_PointsLogLine(string);', 'I_strncat(string', 'G_LogPrintf(string);'),
    ('src/PC/game_mp/g_client_mp.c', 'ClientConnect',
     'G_PointsClientConnect(clientNum, scriptPersId);', 'memset(client, 0, CLIENT_STRIDE);', 'Scr_PlayerConnect('),
    ('src/PC/game_mp/g_client_mp.c', 'ClientBegin', 'G_PointsClientBegin(clientNum);',
     'client->sess.connected = 2;', 'Scr_Notify(ent, scr->begin, 0);'),
    ('src/PC/game_mp/g_client_mp.c', 'ClientDisconnect', 'G_PointsClientDisconnect(clientNum);',
     'client->sess.connected = 0;', 'CalculateRanks();'),
    ('src/PC/game_mp/g_client_mp.c', 'ClientSpawn', 'G_PointsClientSpawn(clientNum);',
     'clientNum = ent - (gentity_s *)imp_g_entities;', 'G_EntUnlink(ent);'),
]
for path, name, call, before, after in hooks:
    text = definition((root / path).read_text(), name)
    assert text.count(call) == 1, (path, call)
    assert text.index(before) < text.index(call) < text.index(after, text.index(call)), (path, call)
main_source = (root / 'src/PC/game_mp/g_main_mp.c').read_text()
init_call = '    G_LogPrintf("gametype: %s\\n", g_gametype->current.string);\n    G_PointsInitGame(restart, g_gametype->current.string);\n'
assert main_source.count(init_call) == 2, 'both G_InitGame builds reset the points'

support = r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
enum { TEAM_FREE = 0, TEAM_AXIS = 1, TEAM_ALLIES = 2, TEAM_SPECTATOR = 3 };
enum { CON_DISCONNECTED = 0, CON_CONNECTING = 1, CON_CONNECTED = 2 };
enum { SV_CMD_CAN_IGNORE = 0, SV_CMD_RELIABLE = 1 };
enum { GCMDS_MAX_CLIENTS = 64 };
typedef struct { int team; } clientState_t;
typedef struct { int score; int deaths; int status_icon; int connected; clientState_t cs; } clientSession_t;
typedef struct gclient_s { clientSession_t sess; } gclient_t;
typedef struct {
    gclient_t *clients; int maxclients; int initializing; int numConnectedClients;
    int sortedClients[64]; int teamScores[4];
} level_locals_t;
typedef struct gentity_s { struct { int number; } s; gclient_t *client; } gentity_t;
static gclient_t clients[64];
static gentity_t g_entities[1024];
level_locals_t level;
int I_stricmp(const char *a, const char *b) { return strcasecmp(a, b); }
int Com_sprintf(char *dest, int size, const char *fmt, ...) {
    va_list ap; int n;
    va_start(ap, fmt); n = vsnprintf(dest, size, fmt, ap); va_end(ap);
    return n;
}
char *va(const char *fmt, ...) {
    static char buffers[4][2048]; static int next; char *out = buffers[next++ & 3]; va_list ap;
    va_start(ap, fmt); vsnprintf(out, 2048, fmt, ap); va_end(ap);
    return out;
}
static char sent[512][1400];
static int sentTo[512];
static int sentCount;
void SV_GameSendServerCommand(int clientNum, int type, const char *text) {
    assert(type == SV_CMD_RELIABLE && sentCount < 512);
    sentTo[sentCount] = clientNum;
    snprintf(sent[sentCount++], sizeof(sent[0]), "%s", text);
}
static int pings[64];
int SV_GetClientPing(int clientNum) { return pings[clientNum]; }
'''

checks = r'''
static int sentIndex;
static void expect(int clientNum, const char *text) {
    assert(sentIndex < sentCount);
    if (sentTo[sentIndex] != clientNum || strcmp(sent[sentIndex], text)) {
        fprintf(stderr, "expected %d '%s', got %d '%s'\n", clientNum, text, sentTo[sentIndex], sent[sentIndex]);
        assert(0);
    }
    ++sentIndex;
}
static void quiet(void) { assert(sentIndex == sentCount); }
static gentity_t *ent(int n) { return &g_entities[n]; }
static gentity_t world;
static void setup(const char *gametype, const int *teams, int count) {
    memset(clients, 0, sizeof(clients));
    level.clients = clients; level.maxclients = 16; level.initializing = 0;
    for (int i = 0; i < 1024; ++i) { g_entities[i].s.number = i; g_entities[i].client = i < 64 ? &clients[i] : 0; }
    world.s.number = 1022; world.client = 0;
    G_PointsInitGame(0, gametype);
    for (int i = 0; i < count; ++i) {
        G_PointsClientConnect(i, 100 + i);
        clients[i].sess.connected = CON_CONNECTED;
        clients[i].sess.cs.team = teams[i];
    }
    sentCount = sentIndex = 0;
}
static void damage(int victim, int attacker, int amount) { G_PointsPlayerDamaged(ent(victim), ent(attacker), amount); }
static void kill(int victim, int attacker, int mod, int hitLoc) {
    G_PointsPlayerKilled(ent(victim), attacker < 0 ? &world : ent(attacker), mod, hitLoc);
}
enum { RIFLE = 2, MELEE = 7, TORSO = 4, HEAD = 2, HELMET = 1 };

int main(void) {
    const int teams[] = { TEAM_AXIS, TEAM_ALLIES, TEAM_ALLIES, TEAM_AXIS, TEAM_SPECTATOR };
    setup("tdm", teams, 5);

    /* A kill nobody helped with, a headshot, and a melee hit to the head. */
    damage(0, 1, 40);
    kill(0, 1, RIFLE, TORSO);
    expect(1, "cod2_points 0 100");
    kill(0, 1, RIFLE, HEAD);
    expect(1, "cod2_points 2 150");
    kill(3, 1, MELEE, HEAD);
    expect(1, "cod2_points 0 100");
    kill(3, 2, RIFLE, HELMET);
    expect(2, "cod2_points 0 100");
    quiet();
    assert(G_PointsGet(1) == 350 && G_PointsGet(2) == 100);

    /* Assisted kill and assisted headshot. */
    damage(3, 2, 30);
    damage(3, 1, 30);
    damage(3, 1, 30);
    kill(3, 1, RIFLE, TORSO);
    expect(2, "cod2_points 3 25");
    expect(1, "cod2_points 1 75");
    damage(0, 2, 10);
    kill(0, 1, RIFLE, HEAD);
    expect(2, "cod2_points 3 25");
    expect(1, "cod2_points 2 150");
    quiet();
    assert(G_PointsGet(1) == 575 && G_PointsGet(2) == 150);

    /* Team damage is no assist, even after a team switch, and a team kill
     * earns nothing. */
    damage(1, 2, 50);
    damage(1, 1, 50);
    damage(1, 3, 0);
    clients[2].sess.cs.team = TEAM_AXIS;
    kill(1, 0, RIFLE, TORSO);
    clients[2].sess.cs.team = TEAM_ALLIES;
    expect(0, "cod2_points 0 100");
    kill(2, 1, RIFLE, HEAD);
    kill(3, 3, RIFLE, HEAD);
    quiet();

    /* Death clears the damage, with or without a killer, and so does a spawn. */
    damage(0, 1, 60);
    kill(0, -1, 11, 0);
    kill(0, 2, RIFLE, TORSO);
    expect(2, "cod2_points 0 100");
    damage(0, 1, 60);
    G_PointsClientSpawn(0);
    kill(0, 2, RIFLE, TORSO);
    expect(2, "cod2_points 0 100");
    quiet();

    /* A disconnected helper is skipped; a new player in the slot starts from zero. */
    damage(0, 2, 20);
    clients[2].sess.connected = CON_DISCONNECTED;
    G_PointsClientDisconnect(2);
    assert(G_PointsGet(2) == 0);
    kill(0, 1, RIFLE, TORSO);
    expect(1, "cod2_points 0 100");
    quiet();
    G_PointsClientConnect(2, 100 + 2);
    assert(G_PointsGet(2) == 0);
    kill(0, 2, RIFLE, TORSO);
    expect(2, "cod2_points 0 100");
    G_PointsClientConnect(2, 100 + 2);
    assert(G_PointsGet(2) == 100);
    G_PointsClientConnect(2, 900);
    assert(G_PointsGet(2) == 0);
    quiet();

    /* Retail score changes only matter in CTF and DM; team score zeroing at
     * runtime starts the match over. */
    G_PointsScoreChanged(1, 3, 13);
    G_PointsScoreChanged(1, 13, 14);
    quiet();
    level.initializing = 1;
    G_PointsTeamScoresChanged(level.initializing, 0, 0);
    assert(G_PointsGet(1) > 0);
    level.initializing = 0;
    G_PointsTeamScoresChanged(level.initializing, 0, 3);
    G_PointsTeamScoresChanged(level.initializing, 2, 0);
    assert(G_PointsGet(1) > 0);
    G_PointsTeamScoresChanged(level.initializing, 0, 0);
    assert(G_PointsGet(0) == 0 && G_PointsGet(1) == 0 && G_PointsGet(2) == 0);

    /* Points survive SD rounds, nothing else. */
    kill(0, 1, RIFLE, TORSO);
    expect(1, "cod2_points 0 100");
    G_PointsInitGame(1, "tdm");
    assert(G_PointsGet(1) == 0);
    G_PointsInitGame(0, "sd");
    kill(0, 1, RIFLE, TORSO);
    expect(1, "cod2_points 0 100");
    damage(0, 2, 20);
    G_PointsInitGame(1, "SD");
    assert(G_PointsGet(1) == 100);
    kill(0, 1, RIFLE, TORSO);
    expect(1, "cod2_points 0 100");
    G_PointsInitGame(0, "sd");
    assert(G_PointsGet(1) == 0);
    quiet();

    /* SD plants and defuses come from the script log line. */
    G_PointsLogLine("A;8812;3;axis;Bad;Name;bomb_plant\n");
    expect(3, "cod2_points 6 200");
    G_PointsLogLine("A;;1;allies;Defuser;bomb_defuse");
    expect(1, "cod2_points 7 200");
    G_PointsLogLine("A;1;3x;axis;n;bomb_plant\n");
    G_PointsLogLine("A;1;3;axis;n;bomb_planted\n");
    G_PointsLogLine("A;1;;axis;n;bomb_plant\n");
    G_PointsLogLine("A;1;40;axis;n;bomb_plant\n");
    G_PointsLogLine("A;1;5;axis;n;bomb_plant\n");
    G_PointsLogLine("K;1;3;axis;n;bomb_plant\n");
    G_PointsLogLine("A;bomb_plant\n");
    quiet();
    assert(G_PointsGet(3) == 200 && G_PointsGet(1) == 200);

    /* CTF captures add 10 to the retail score and returns add 2. */
    setup("ctf", teams, 5);
    G_PointsScoreChanged(2, 4, 14);
    expect(2, "cod2_points 4 300");
    G_PointsScoreChanged(3, 1, 3);
    expect(3, "cod2_points 5 100");
    G_PointsScoreChanged(3, 3, 4);
    G_PointsScoreChanged(3, 4, 3);
    G_PointsScoreChanged(3, 3, 3);
    quiet();

    /* DM: no assists, and every retail score change reaches everyone. */
    const int free[] = { TEAM_FREE, TEAM_FREE, TEAM_FREE, TEAM_SPECTATOR };
    setup("dm", free, 4);
    damage(0, 1, 50);
    kill(0, 2, RIFLE, TORSO);
    expect(2, "cod2_points 0 100");
    kill(1, 1, RIFLE, HEAD);
    quiet();
    clients[1].sess.score = 7;
    clients[2].sess.score = -2;
    G_PointsScoreChanged(2, -1, -2);
    expect(-1, "cod2_dmscore 2 -2");
    G_PointsScoreChanged(2, -2, -2);
    clients[3].sess.connected = CON_CONNECTING;
    G_PointsClientBegin(3);
    expect(3, "cod2_dmscores 0 0 1 7 2 -2");
    quiet();
    setup("tdm", teams, 5);
    G_PointsClientBegin(1);
    G_PointsScoreChanged(1, 0, 1);
    quiet();

    /* The scoreboard sends points in the score field, players first, best first. */
    setup("tdm", teams, 5);
    kill(0, 1, RIFLE, HEAD);
    kill(1, 3, RIFLE, TORSO);
    kill(1, 0, RIFLE, TORSO);
    G_PointsLogLine("A;1;4;axis;n;bomb_plant\n");
    sentCount = sentIndex = 0;
    clients[0].sess.deaths = 1; clients[1].sess.deaths = 2; clients[2].sess.status_icon = 3;
    clients[3].sess.connected = CON_CONNECTING;
    pings[0] = 40; pings[1] = 60; pings[2] = 80; pings[4] = 20;
    level.teamScores[TEAM_AXIS] = 2; level.teamScores[TEAM_ALLIES] = 1;
    level.numConnectedClients = 5;
    int retailOrder[] = { 4, 3, 2, 1, 0 };
    memcpy(level.sortedClients, retailOrder, sizeof(retailOrder));
    DeathmatchScoreboardMessage(ent(2));
    expect(2, "b 5 2 1 1 150 60 2 0 3 100 -1 0 0 0 100 40 1 0 2 0 80 0 3 4 200 20 0 0");
    quiet();
    return 0;
}
'''


def mutate(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


module_mutants = [
    ('100, /* POINTS_KILL */', '10, /* POINTS_KILL */'),
    ('75,  /* POINTS_ASSISTED_KILL */', '100,  /* POINTS_ASSISTED_KILL */'),
    ('25,  /* POINTS_ASSIST */', '20,  /* POINTS_ASSIST */'),
    ('300, /* POINTS_FLAG_CAPTURE */', '250, /* POINTS_FLAG_CAPTURE */'),
    ('100, /* POINTS_FLAG_RETURN */', '50, /* POINTS_FLAG_RETURN */'),
    ('200, /* POINTS_BOMB_PLANT */', '500, /* POINTS_BOMB_PLANT */'),
    ('200  /* POINTS_BOMB_DEFUSE */', '500  /* POINTS_BOMB_DEFUSE */'),
    ('hitLoc == POINTS_HITLOC_HEAD && meansOfDeath != POINTS_MOD_MELEE', 'hitLoc == POINTS_HITLOC_HEAD'),
    ('hitLoc == POINTS_HITLOC_HEAD && meansOfDeath != POINTS_MOD_MELEE', 'hitLoc <= POINTS_HITLOC_HEAD && meansOfDeath != POINTS_MOD_MELEE'),
    ('assisted ? POINTS_ASSISTED_KILL : POINTS_KILL', 'POINTS_KILL'),
    ('    if (G_PointsEnemies(attackerNum, victimNum))\n        s_pointsClients[victimNum].damagedBy',
     '    if (1)\n        s_pointsClients[victimNum].damagedBy'),
    ('if (damage <= 0 || !victim', 'if (damage < 0 || !victim'),
    ('    s_pointsClients[victimNum].damagedBy = 0;\n\n    if (!attacker', '\n    if (!attacker'),
    ('        s_pointsClients[clientNum].damagedBy = 0;\n}', '        ;\n}'),
    ('if (!G_PointsConnected(i) || !G_PointsEnemies(i, victimNum))', 'if (!G_PointsEnemies(i, victimNum))'),
    ('if (attackerNum == victimNum || !G_PointsValidClient(attackerNum) || !G_PointsEnemies(attackerNum, victimNum))',
     'if (attackerNum == victimNum || !G_PointsValidClient(attackerNum))'),
    ('if (attackerNum == victimNum || !G_PointsValidClient(attackerNum) || !G_PointsEnemies(attackerNum, victimNum))',
     'if (!G_PointsValidClient(attackerNum) || !G_PointsEnemies(attackerNum, victimNum))'),
    ('    if (s_pointsGametype != POINTS_GT_DM) {\n        for', '    if (1) {\n        for'),
    ('if (!restart || s_pointsGametype != POINTS_GT_SD)', 'if (!restart)'),
    ('if (!restart || s_pointsGametype != POINTS_GT_SD)', 'if (s_pointsGametype != POINTS_GT_SD)'),
    ('if (!slot->inUse || slot->persId != scriptPersId)', 'if (!slot->inUse)'),
    ('    memset(&s_pointsClients[clientNum], 0, sizeof(s_pointsClients[clientNum]));\n', ''),
    ('if (initializing || axisScore != 0 || alliesScore != 0)', 'if (axisScore != 0 || alliesScore != 0)'),
    ('if (initializing || axisScore != 0 || alliesScore != 0)', 'if (initializing || axisScore != 0 && alliesScore != 0)'),
    ('POINTS_CTF_CAPTURE_SCORE = 10', 'POINTS_CTF_CAPTURE_SCORE = 11'),
    ('POINTS_CTF_RETURN_SCORE = 2', 'POINTS_CTF_RETURN_SCORE = 1'),
    ('    } else if (s_pointsGametype == POINTS_GT_DM) {\n        G_PointsSend(-1',
     '    } else {\n        G_PointsSend(-1'),
    ('if (!digits || *field != \';\')', 'if (!digits)'),
    ('action = strrchr(line, \';\');', 'action = strchr(field + 1, \';\');'),
    ('if (G_PointsValidClient(clientNum) && G_PointsConnected(clientNum))\n        G_PointsAward(clientNum, event);',
     'if (G_PointsValidClient(clientNum))\n        G_PointsAward(clientNum, event);'),
    ('if (!G_PointsConnected(i) || length > (int)sizeof(text) - 32)', 'if (length > (int)sizeof(text) - 32)'),
    ('G_PointsGet(other) >= points', 'G_PointsGet(other) > points'),
    ('otherSpectator < spectator || (otherSpectator == spectator && G_PointsGet(other) >= points)',
     'G_PointsGet(other) >= points'),
    ('return teamA == TEAM_FREE || teamA != G_PointsTeam(b);', 'return teamA != G_PointsTeam(b);'),
]
scoreboard_mutants = [
    ('order[i] = lvl->sortedClients[i];\n    G_PointsSortClients(order, numSorted);', 'order[i] = lvl->sortedClients[i];'),
    ('clientNum, G_PointsGet(clientNum), ping,', 'clientNum, client->sess.score, ping,'),
    ('clientNum, G_PointsGet(clientNum), -1,', 'clientNum, client->sess.score, -1,'),
]
variants = [(body, scoreboard)]
variants += [(mutate(body, old, new), scoreboard) for old, new in module_mutants]
variants += [(body, mutate(scoreboard, old, new)) for old, new in scoreboard_mutants]
with tempfile.TemporaryDirectory(prefix='cod2-points-') as directory:
    path = Path(directory) / 'points.c'
    binary = Path(directory) / 'points'
    for index, (points_source, scoreboard_source) in enumerate(variants):
        path.write_text(support + points_source + '\n' + scoreboard_source + '\n' + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wno-unused-function', str(path), '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
print('PASS: kills earn 100, assisted kills 75, headshots 150 and assists 25 in team modes; DM has no assists and shares '
      'retail scores; CTF captures/returns and SD plants/defuses score; points survive only SD rounds; the scoreboard '
      f'sends points ranked players first; {len(hooks) + 1} hooks wired; {len(variants) - 1} failing regression mutants')
