#!/usr/bin/env python3
"""Votes are counted among the people playing, never the bots.

Runs the production CalculateRanks and CheckVote: a player alone with bots
passes a vote by calling it, two players need both, three need two; the
spectators, the connecting, the disconnected and the bots never count; a vote
run out of time passes with the abstainers' half weighed against it; a passed
vote's command runs three seconds later.
"""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/game_mp/g_main_mp.c').read_text()


def function(text, name):
    match = re.search(r'^void ' + name + r'\(void\)\n\{', text, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


code = r'''
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int qboolean;
enum { CON_DISCONNECTED, CON_CONNECTING, CON_CONNECTED };
enum { TEAM_FREE, TEAM_AXIS, TEAM_ALLIES, TEAM_SPECTATOR };
#define SV_CMD_CAN_IGNORE 0
#define GMAIN_CS_VOTE_TIME 15
typedef struct { struct { int connected; struct { int team; } cs; } sess; } gclient_t;
static gclient_t clients[64];
static struct {
    int maxclients, numConnectedClients, numVotingClients, sortedClients[64], bUpdateScoresForIntermission;
    int time, voteTime, voteExecuteTime, voteYes, voteNo;
    char voteString[1024];
    gclient_t *clients;
} level = { 64 };
typedef struct { struct { float value; } current; } dvar_t;
static dvar_t abstain = { { 0.5f } };
static const dvar_t *g_voteAbstainWeight = &abstain;
static int bots[64];
qboolean SV_BotIsTestClient(int clientNum) { return bots[clientNum]; }
static int SortRanks(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }
static char said[8][64];
static int saidCount;
static void SV_GameSendServerCommand(int clientNum, int type, const char *text)
{
    (void)type; assert(clientNum == -1 && saidCount < 8);
    snprintf(said[saidCount++], sizeof(said[0]), "%s", text);
}
static char executed[128];
static void Cbuf_ExecuteText(int when, const char *text) { assert(when == 2); snprintf(executed, sizeof(executed), "%s", text); }
static char voteTimeString[16] = "x";
static void SV_SetConfigstring(int index, const char *value)
{
    assert(index == GMAIN_CS_VOTE_TIME); snprintf(voteTimeString, sizeof(voteTimeString), "%s", value);
}
static const char *va(const char *format, ...)
{
    static char buffer[128]; va_list ap; va_start(ap, format); vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap); return buffer;
}
''' + function(source, 'CalculateRanks') + function(source, 'CheckVote') + r'''
/* people playing, people watching, people connecting, bots playing. */
static void Room(int playing, int watching, int connecting, int botCount)
{
    int n = 0;
    memset(clients, 0, sizeof(clients));
    memset(bots, 0, sizeof(bots));
    level.clients = clients;
    for (int i = 0; i < playing; i++, n++) { clients[n].sess.connected = CON_CONNECTED; clients[n].sess.cs.team = TEAM_ALLIES; }
    for (int i = 0; i < watching; i++, n++) { clients[n].sess.connected = CON_CONNECTED; clients[n].sess.cs.team = TEAM_SPECTATOR; }
    for (int i = 0; i < connecting; i++, n++) { clients[n].sess.connected = CON_CONNECTING; clients[n].sess.cs.team = TEAM_AXIS; }
    for (int i = 0; i < botCount; i++, n++) {
        clients[n].sess.connected = CON_CONNECTED; clients[n].sess.cs.team = i & 1 ? TEAM_AXIS : TEAM_FREE; bots[n] = 1;
    }
    CalculateRanks();
}
/* A vote called at 10 s, the caller's yes counted, as Cmd_CallVote_f leaves it. */
static void Call(void)
{
    level.time = 10000; level.voteTime = level.time + 30000; level.voteYes = 1; level.voteNo = 0;
    level.voteExecuteTime = 0; snprintf(level.voteString, sizeof(level.voteString), "map mp_matmata");
    saidCount = 0; executed[0] = '\0';
}
static int Passed(void) { return saidCount && !strcmp(said[saidCount - 1], "e \"GAME_VOTEPASSED\""); }
static int Failed(void) { return saidCount && !strcmp(said[saidCount - 1], "e \"GAME_VOTEFAILED\""); }

int main(void)
{
    int cases = 0;
    /* Alone with six bots: the caller's own yes passes it at once; its map
     * loads three seconds later. */
    Room(1, 0, 0, 6);
    assert(level.numConnectedClients == 7 && level.numVotingClients == 1);
    Call(); CheckVote();
    assert(Passed() && !level.voteTime && level.voteExecuteTime == 13000 && !strcmp(voteTimeString, ""));
    level.time = 13000; CheckVote(); assert(!executed[0]);
    level.time = 13001; CheckVote(); assert(!strcmp(executed, "map mp_matmata\n") && !level.voteExecuteTime);
    cases++;
    /* Two players and four bots: both must say yes. */
    Room(2, 0, 0, 4); assert(level.numVotingClients == 2);
    Call(); CheckVote(); assert(!saidCount && level.voteTime);
    level.voteYes = 2; CheckVote(); assert(Passed());
    Call(); level.voteNo = 1; CheckVote(); assert(Failed() && !level.voteExecuteTime);
    cases++;
    /* Three players: two yes pass, two no fail; watchers, joiners and bots
     * never count. */
    Room(3, 2, 2, 8); assert(level.numVotingClients == 3 && level.numConnectedClients == 15);
    Call(); CheckVote(); assert(!saidCount);
    level.voteYes = 2; CheckVote(); assert(Passed());
    Call(); level.voteNo = 2; CheckVote(); assert(Failed());
    cases++;
    /* Run out of time with four players: two yes against no one pass, the two
     * abstainers weighing one; one yes alone does not. */
    Room(4, 0, 0, 0);
    Call(); level.voteYes = 2; level.time = level.voteTime; CheckVote(); assert(Passed());
    Call(); level.time = level.voteTime; CheckVote(); assert(Failed());
    cases++;
    /* A room of bots and watchers has no one to vote. */
    Room(0, 1, 0, 6); assert(level.numVotingClients == 0);
    cases++;
    printf("%d\n", cases);
    return 0;
}
'''

mutants = [
    ('level.clients[i].sess.connected == CON_CONNECTED && !SV_BotIsTestClient(i)', 'level.clients[i].sess.connected == CON_CONNECTED'),
    ('            if (level.clients[i].sess.cs.team != TEAM_SPECTATOR) {', '            if (1) {'),
    ('        passCount = level.numVotingClients / 2 + 1;', '        passCount = level.numVotingClients / 2;'),
]

with tempfile.TemporaryDirectory(prefix='cod2-vote-count-') as directory:
    path = Path(directory)
    for index, change in enumerate([None] + mutants):
        variant = code
        if change:
            assert variant.count(change[0]) == 1, change[0]
            variant = variant.replace(*change)
        (path / 'test.c').write_text(variant)
        flags = ['-Wall', '-Werror', '-Wno-unused-function'] if change is None else ['-w']
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        *flags, str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True)
        if change is None:
            assert result.returncode == 0, result.stdout + result.stderr
            cases = int(result.stdout.split()[-1])
        else:
            assert result.returncode != 0, f'mutant survived: {change[1]!r}'

print(f'PASS: {cases} rooms: a player alone with bots passes a vote by calling it, two players need both, three need '
      f'two, watchers, joiners and bots never count, a vote out of time weighs the abstainers\' half against it, and '
      f'a passed vote runs three seconds later; {len(mutants)} mutants fail')

# The pause menu's view of it: the cgame's vote and the people it may kick.
overview = (root / 'src/PC/cgame_mp/cg_overview_mp.c').read_text()


def cfunction(text, name):
    match = re.search(r'^(?:static )?(?:int|void) ' + name + r'\([^)]*\)\n\{', text, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


defines = ''.join(line + '\n' for line in overview.splitlines()
                  if line.startswith(('#define OVERVIEW_MAX_CLIENTS', '#define OVERVIEW_VOTED', '#define OVERVIEW_BOT_PREFIX')))
state = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef float vec_t;
typedef vec_t vec4_t[4];
typedef struct Material *MaterialHandle;
enum { TEAM_FREE, TEAM_AXIS, TEAM_ALLIES, TEAM_SPECTATOR };
typedef struct { int infoValid; int team; char name[32]; } clientInfo_t;
typedef struct { struct { int clientNum; int eFlags; } ps; } snapshot_t;
static snapshot_t snapState, nextSnapState;
static struct { snapshot_t *snap, *nextSnap; int time; struct { clientInfo_t clientinfo[64]; } bgs; } cgState, *cg = &cgState;
static struct { char gametype[32]; int voteTime, voteYes, voteNo; char voteString[256]; } cgsState, *cgs = &cgsState;
#include "headers/PC/cgame_mp/cg_overview.h"
static const char *CL_GetConfigString(int index) { assert(index == 0); return "\\g_gametype\\sd\\mapname\\mp_brecourt"; }
static const char *Info_ValueForKey(const char *s, const char *key)
{
    static char value[64];
    char pattern[64];
    const char *at;
    snprintf(pattern, sizeof(pattern), "\\%s\\", key);
    at = strstr(s, pattern);
    if (!at) return "";
    at += strlen(pattern);
    snprintf(value, sizeof(value), "%.*s", (int)strcspn(at, "\\"), at);
    return value;
}
static void I_strncpyz(char *dest, const char *src, int size) { snprintf(dest, (size_t)size, "%s", src); }
static char *I_CleanStr(char *string)
{
    char *d = string;
    for (const char *s = string; *s; s++) {
        if (*s == '^' && s[1] >= '0' && s[1] <= '9') { s++; continue; }
        *d++ = *s;
    }
    *d = '\0';
    return string;
}
''' + defines + cfunction(overview, 'CG_OverviewTeam') + cfunction(overview, 'CG_OverviewVote') + \
    cfunction(overview, 'CG_OverviewVotePlayers') + r'''
int main(void)
{
    CG_Vote vote;
    CG_VotePlayer players[64];
    assert(!CG_OverviewVote(&vote) && !vote.active);
    cg->snap = &snapState; cg->nextSnap = &nextSnapState; cg->time = 50000;
    snapState.ps.clientNum = nextSnapState.ps.clientNum = 2;
    snprintf(cgs->gametype, sizeof(cgs->gametype), "sd");
    cg->bgs.clientinfo[2] = (clientInfo_t){ 1, TEAM_AXIS, "Me" };
    cg->bgs.clientinfo[0] = (clientInfo_t){ 1, TEAM_ALLIES, "^1Ali^7ce" };
    cg->bgs.clientinfo[4] = (clientInfo_t){ 1, TEAM_AXIS, "[BOT] Recruit4" };
    cg->bgs.clientinfo[5] = (clientInfo_t){ 0, TEAM_AXIS, "Gone" };
    cg->bgs.clientinfo[9] = (clientInfo_t){ 1, TEAM_SPECTATOR, "Watcher" };
    /* No vote: the map and mode played. */
    assert(CG_OverviewVote(&vote) && !vote.active && !vote.spectator);
    assert(!strcmp(vote.map, "mp_brecourt") && !strcmp(vote.gametype, "sd"));
    /* A vote: its seconds, counts, text cleaned, and the viewer's voted flag. */
    cgs->voteTime = 73500; cgs->voteYes = 3; cgs->voteNo = 1;
    snprintf(cgs->voteString, sizeof(cgs->voteString), "Kick Player: (0)^1Ali^7ce");
    assert(CG_OverviewVote(&vote) && vote.active && vote.secondsLeft == 23 && vote.yes == 3 && vote.no == 1);
    assert(!strcmp(vote.text, "Kick Player: (0)Alice") && !vote.voted);
    nextSnapState.ps.eFlags = 0x100000; assert(CG_OverviewVote(&vote) && vote.voted);
    cg->time = 80000; assert(CG_OverviewVote(&vote) && vote.secondsLeft == 0);
    cg->bgs.clientinfo[2].team = TEAM_SPECTATOR; assert(CG_OverviewVote(&vote) && vote.spectator);
    /* The people a vote may kick: not the viewer, the gone or the bots; watchers too. */
    int count = CG_OverviewVotePlayers(players, 64);
    assert(count == 2 && players[0].clientNum == 0 && !strcmp(players[0].name, "Alice"));
    assert(players[1].clientNum == 9 && !strcmp(players[1].name, "Watcher"));
    assert(CG_OverviewVotePlayers(players, 1) == 1);
    cg->snap = NULL; assert(!CG_OverviewVotePlayers(players, 64));
    puts("ok");
    return 0;
}
'''
state_mutants = [
    ('    vote->voted = (cg->nextSnap->ps.eFlags & OVERVIEW_VOTED) != 0;', '    vote->voted = 0;'),
    ('!strncmp(info->name, OVERVIEW_BOT_PREFIX, sizeof(OVERVIEW_BOT_PREFIX) - 1)', '0'),
    ('i == cg->snap->ps.clientNum ||', '0 ||'),
    ('    if (vote->secondsLeft < 0)\n        vote->secondsLeft = 0;\n', ''),
    ('        I_CleanStr(players[count].name);\n', ''),
]
with tempfile.TemporaryDirectory(prefix='cod2-vote-state-') as directory:
    path = Path(directory)
    for index, change in enumerate([None] + state_mutants):
        variant = state
        if change:
            assert variant.count(change[0]) == 1, change[0]
            variant = variant.replace(*change)
        (path / 'test.c').write_text(variant)
        flags = ['-Wall', '-Werror', '-Wno-unused-function'] if change is None else ['-w']
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-I', str(root / 'src'), '-fsanitize=address,undefined',
                        '-fno-sanitize-recover=all', *flags, str(path / 'test.c'), '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True)
        if change is None:
            assert result.returncode == 0 and result.stdout.strip() == 'ok', result.stdout + result.stderr
        else:
            assert result.returncode != 0, f'mutant survived: {change[1]!r}'
print(f'PASS: the pause menu sees the map and mode played, a vote\'s seconds, counts, cleaned text and the viewer\'s '
      f'own vote, and may kick the people other than the viewer, bots left out; {len(state_mutants)} mutants fail')
