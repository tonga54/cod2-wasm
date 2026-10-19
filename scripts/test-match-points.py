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
scores_limit = re.search(r'^    GCMDS_SCORES_LIMIT = (\d+)$', commands, re.M).group(1)

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
#include <stdlib.h>
#include <string.h>
#include <strings.h>
enum { TEAM_FREE = 0, TEAM_AXIS = 1, TEAM_ALLIES = 2, TEAM_SPECTATOR = 3 };
enum { CON_DISCONNECTED = 0, CON_CONNECTING = 1, CON_CONNECTED = 2 };
enum { SV_CMD_CAN_IGNORE = 0, SV_CMD_RELIABLE = 1 };
enum { GCMDS_MAX_CLIENTS = 64, GCMDS_SCORES_LIMIT = ''' + scores_limit + r''' };
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
/* The killcam's attacker lists are kept apart: the last one and how many. */
static char list[256];
static int listTo, lists;
void SV_GameSendServerCommand(int clientNum, int type, const char *text) {
    assert(type == SV_CMD_RELIABLE);
    if (!strncmp(text, "cod2_attackers ", 15)) {
        listTo = clientNum;
        snprintf(list, sizeof(list), "%s", text + 15);
        ++lists;
        return;
    }
    assert(sentCount < 512);
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
    assert(G_PointsKills(1) == 3 && G_PointsKills(2) == 1 && G_PointsKills(0) == 0);

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
    /* Each assist and each headshot counts once; a melee hit to the head is no headshot. */
    assert(G_PointsAssists(2) == 2 && G_PointsAssists(1) == 0 && G_PointsAssists(-1) == 0);
    assert(G_PointsHeadshots(1) == 2 && G_PointsHeadshots(2) == 0 && G_PointsHeadshots(64) == 0);

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
    /* Only enemies count as kills: not a teammate, yourself or the world. */
    assert(G_PointsKills(0) == 1 && G_PointsKills(1) == 5 && G_PointsKills(3) == 0);

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
    assert(G_PointsGet(2) == 100 && G_PointsKills(2) == 1);
    damage(3, 1, 10);
    kill(3, 2, RIFLE, HEAD);
    expect(1, "cod2_points 3 25");
    expect(2, "cod2_points 2 150");
    G_PointsClientConnect(2, 100 + 2);
    assert(G_PointsHeadshots(2) == 1);
    G_PointsClientConnect(2, 900);
    assert(G_PointsGet(2) == 0 && G_PointsKills(2) == 0 && G_PointsHeadshots(2) == 0);
    quiet();

    /* Retail score changes only matter in CTF and DM; team score zeroing at
     * runtime starts the match over. */
    G_PointsScoreChanged(1, 3, 13);
    G_PointsScoreChanged(1, 13, 14);
    quiet();
    level.initializing = 1;
    G_PointsTeamScoresChanged(level.initializing, 0, 0);
    assert(G_PointsGet(1) > 0 && G_PointsKills(1) > 0);
    level.initializing = 0;
    G_PointsTeamScoresChanged(level.initializing, 0, 3);
    G_PointsTeamScoresChanged(level.initializing, 2, 0);
    assert(G_PointsGet(1) > 0);
    assert(G_PointsAssists(1) > 0 && G_PointsHeadshots(2) == 0);
    G_PointsTeamScoresChanged(level.initializing, 0, 0);
    assert(G_PointsGet(0) == 0 && G_PointsGet(1) == 0 && G_PointsGet(2) == 0);
    assert(G_PointsKills(0) == 0 && G_PointsKills(1) == 0);
    assert(G_PointsAssists(1) == 0 && G_PointsHeadshots(1) == 0);

    /* Points survive SD rounds, nothing else. */
    kill(0, 1, RIFLE, TORSO);
    expect(1, "cod2_points 0 100");
    G_PointsInitGame(1, "tdm");
    assert(G_PointsGet(1) == 0 && G_PointsKills(1) == 0);
    G_PointsInitGame(0, "sd");
    clients[2].sess.connected = CON_CONNECTED;
    damage(0, 2, 20);
    kill(0, 1, RIFLE, HEAD);
    expect(2, "cod2_points 3 25");
    expect(1, "cod2_points 2 150");
    damage(0, 2, 20);
    G_PointsInitGame(1, "SD");
    assert(G_PointsGet(1) == 150 && G_PointsKills(1) == 1);
    assert(G_PointsHeadshots(1) == 1 && G_PointsAssists(2) == 1);
    kill(0, 1, RIFLE, TORSO);
    expect(1, "cod2_points 0 100");
    G_PointsInitGame(0, "sd");
    assert(G_PointsGet(1) == 0 && G_PointsKills(1) == 0);
    assert(G_PointsHeadshots(1) == 0 && G_PointsAssists(2) == 0);
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

    /* The scoreboard sends points in the score field, players first, best
     * first, then the same players' kills for our client. */
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
    expect(2, "b 5 2 1 1 150 60 2 0 3 100 -1 0 0 0 100 40 1 0 2 0 80 0 3 4 200 20 0 0 1 1 1 0 0 0 0");
    /* The receiver's own assists and headshots end it. */
    damage(3, 2, 10);
    kill(3, 1, RIFLE, TORSO);
    sentCount = sentIndex = 0;
    DeathmatchScoreboardMessage(ent(1));
    expect(1, "b 5 2 1 1 225 60 2 0 3 100 -1 0 0 0 100 40 1 0 2 25 80 0 3 4 200 20 0 0 2 1 1 0 0 0 1");
    DeathmatchScoreboardMessage(ent(2));
    expect(2, "b 5 2 1 1 225 60 2 0 3 100 -1 0 0 0 100 40 1 0 2 25 80 0 3 4 200 20 0 0 2 1 1 0 0 1 0");
    quiet();

    /* A full server stays within one reliable command; the players that do not
     * fit are left out with their kills. */
    int many[64];
    for (int i = 0; i < 64; ++i)
        many[i] = (i & 1) ? TEAM_ALLIES : TEAM_AXIS;
    setup("tdm", many, 64);
    level.maxclients = 64;
    for (int i = 0; i < 64; ++i) {
        clients[i].sess.deaths = 123456789;
        pings[i] = 999;
        level.sortedClients[i] = 63 - i;
    }
    for (int i = 0; i < 12; ++i)
        kill(2 * i, 2 * i + 1, RIFLE, TORSO);
    kill(2, 1, RIFLE, TORSO);
    level.numConnectedClients = 64;
    sentCount = sentIndex = 0;
    DeathmatchScoreboardMessage(ent(0));
    assert(sentCount == 1 && sentTo[0] == 0 && strlen(sent[0]) < 1024);
    char copy[1400];
    char *tokens[512];
    int tokenCount = 0;
    snprintf(copy, sizeof(copy), "%s", sent[0]);
    for (char *t = strtok(copy, " "); t; t = strtok(NULL, " "))
        tokens[tokenCount++] = t;
    int listed = atoi(tokens[1]);
    assert(listed > 30 && listed < 64 && tokenCount == 6 + 6 * listed);
    assert(!strcmp(tokens[4], "1") && !strcmp(tokens[4 + 5 * listed], "2"));
    for (int i = 0; i < listed; ++i)
        assert(atoi(tokens[4 + 5 * listed + i]) == G_PointsKills(atoi(tokens[4 + 5 * i])));
    assert(!strcmp(tokens[4 + 6 * listed], "0") && !strcmp(tokens[5 + 6 * listed], "0"));
    /* The players' fields and the receiver's stats share the room after the
     * header: 22 characters a player fill it exactly, so the stats push the
     * last one out. */
    setup("tdm", many, 64);
    level.maxclients = 64;
    for (int i = 0; i < 64; ++i) {
        clients[i].sess.deaths = 12345678;
        pings[i] = 999;
        level.sortedClients[i] = 63 - i;
    }
    level.numConnectedClients = 64;
    DeathmatchScoreboardMessage(ent(0));
    {
        const char *fields = sent[0];
        for (int spaces = 0; spaces < 4; ++fields)
            spaces += *fields == ' ';
        assert(atoi(sent[0] + 2) == 44 && (int)strlen(fields - 1) == 44 * 22 + 4);
        assert(!strcmp(sent[0] + strlen(sent[0]) - 4, " 0 0"));
    }
    sentCount = sentIndex = 0;

    /* Every death by another player tells the victim who hurt them: the
     * killer, then the other enemies by their first hit this life, connected
     * ones only; a teammate's kill too, a death by the world or by their own
     * hand not. */
    const int sides[] = { TEAM_AXIS, TEAM_ALLIES, TEAM_ALLIES, TEAM_ALLIES, TEAM_AXIS, TEAM_ALLIES };
    setup("tdm", sides, 6);
    lists = 0;
    kill(0, 1, RIFLE, TORSO);
    assert(lists == 1 && listTo == 0 && !strcmp(list, "1"));
    damage(0, 3, 10);
    damage(0, 1, 10);
    damage(0, 5, 10);
    damage(0, 3, 10);
    damage(0, 4, 10);
    damage(0, 2, 10);
    kill(0, 1, RIFLE, TORSO);
    assert(lists == 2 && listTo == 0 && !strcmp(list, "1 3 5 2"));
    damage(0, 2, 10);
    damage(0, 5, 10);
    damage(0, 3, 10);
    clients[5].sess.connected = CON_DISCONNECTED;
    kill(0, 1, RIFLE, HEAD);
    assert(lists == 3 && !strcmp(list, "1 2 3"));
    clients[5].sess.connected = CON_CONNECTED;
    damage(0, 3, 10);
    kill(0, 4, RIFLE, TORSO);
    assert(lists == 4 && listTo == 0 && !strcmp(list, "4 3"));
    damage(0, 1, 10);
    kill(0, -1, 11, 0);
    kill(0, 0, RIFLE, TORSO);
    assert(lists == 4);
    damage(0, 5, 10);
    G_PointsClientSpawn(0);
    damage(0, 2, 0);
    kill(3, 2, RIFLE, TORSO);
    assert(lists == 5 && listTo == 3 && !strcmp(list, "2"));
    kill(0, 1, RIFLE, TORSO);
    assert(lists == 6 && listTo == 0 && !strcmp(list, "1"));
    sentCount = sentIndex = 0;
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
    ('    ++s_pointsClients[attackerNum].kills;\n', ''),
    ('            ++s_pointsClients[i].assists;\n', ''),
    ('        ++s_pointsClients[attackerNum].headshots;\n', ''),
    ('    slot->kills = 0;\n', ''),
    ('    slot->assists = 0;\n', ''),
    ('    slot->headshots = 0;\n', ''),
    ('            G_PointsClear(&s_pointsClients[i]);\n', '            s_pointsClients[i].points = 0;\n'),
    ('    for (i = 0; i < POINTS_MAX_CLIENTS; ++i)\n        G_PointsClear(&s_pointsClients[i]);\n}',
     '    for (i = 0; i < POINTS_MAX_CLIENTS; ++i)\n        s_pointsClients[i].points = 0;\n}'),
    ('        G_PointsClear(slot);\n', '        slot->points = 0;\n'),
    ('return s_pointsClients[clientNum].kills;', 'return s_pointsClients[clientNum].points;'),
    ('return s_pointsClients[clientNum].assists;', 'return s_pointsClients[clientNum].kills;'),
    ('return s_pointsClients[clientNum].headshots;', 'return s_pointsClients[clientNum].assists;'),
    ('slot->damageOrder[order[j - 1]] > slot->damageOrder[i]', 'slot->damageOrder[order[j - 1]] < slot->damageOrder[i]'),
    ('    if (!(s_pointsClients[victimNum].damagedBy & (1ull << attackerNum)))\n        s_pointsClients[victimNum].damageOrder',
     '    s_pointsClients[victimNum].damageOrder'),
    ('if (i == attackerNum || !(damagedBy & (1ull << i)) || !G_PointsConnected(i))',
     'if (!(damagedBy & (1ull << i)) || !G_PointsConnected(i))'),
    ('if (i == attackerNum || !(damagedBy & (1ull << i)) || !G_PointsConnected(i))',
     'if (i == attackerNum || !(damagedBy & (1ull << i)))'),
    ('    if (attackerNum != victimNum && G_PointsValidClient(attackerNum))\n        G_PointsSendAttackers',
     '    if (G_PointsValidClient(attackerNum))\n        G_PointsSendAttackers'),
    ('    if (attackerNum != victimNum && G_PointsValidClient(attackerNum))\n        G_PointsSendAttackers',
     '    if (attackerNum != victimNum && G_PointsValidClient(attackerNum) && G_PointsEnemies(attackerNum, victimNum))\n'
     '        G_PointsSendAttackers'),
]
scoreboard_mutants = [
    ('order[i] = lvl->sortedClients[i];\n    G_PointsSortClients(order, numSorted);', 'order[i] = lvl->sortedClients[i];'),
    ('clientNum, G_PointsGet(clientNum), ping,', 'clientNum, client->sess.score, ping,'),
    ('clientNum, G_PointsGet(clientNum), -1,', 'clientNum, client->sess.score, -1,'),
    ('lvl->teamScores[TEAM_ALLIES], string, kills, own);', 'lvl->teamScores[TEAM_ALLIES], string, "", own);'),
    ('lvl->teamScores[TEAM_ALLIES], string, kills, own);', 'lvl->teamScores[TEAM_ALLIES], string, kills, "");'),
    ('if (stringlength + j + killslength + k + ownlength > GCMDS_SCORES_LIMIT)',
     'if (stringlength + j > GCMDS_SCORES_LIMIT)'),
    ('if (stringlength + j + killslength + k + ownlength > GCMDS_SCORES_LIMIT)',
     'if (stringlength + j + killslength + k > GCMDS_SCORES_LIMIT)'),
    ('" %i", G_PointsKills(clientNum));', '" %i", G_PointsGet(clientNum));'),
    ('" %i %i", G_PointsAssists(ent - g_entities),', '" %i %i", G_PointsHeadshots(ent - g_entities),'),
    ('                            G_PointsHeadshots(ent - g_entities));', '                            0);'),
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
      f'sends points ranked players first, then their kills of enemies (kept with the points) and the receiver\'s '
      f'own assists and headshots, and fits a full server in one reliable command; a victim learns their killer and '
      f'the other enemies that hurt them, by first '
      f'hit; {len(hooks) + 1} hooks wired; {len(variants) - 1} failing regression mutants')
