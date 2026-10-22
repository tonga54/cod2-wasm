#!/usr/bin/env python3
"""Bots carry on into the next map of the rotation.

A map change connects every client to the new level again. A person's client
then answers the new gamestate and begins; a test client cannot answer, so
SV_ReconnectClients enters it into the world itself, with the steps
SV_AddTestClient takes for a new bot. Otherwise it stayed connecting, unseen by
the bot script, which added as many bots again on every map. This runs the
production SV_ReconnectClients with the engine calls stubbed over free,
zombie, connecting, primed and active slots, people and bots, refused clients
and a bot past sv_maxclients. scripts/test-bot-rotation-runtime.py runs the
rotation through the dedicated server.
"""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/server_mp/sv_init_mp.c').read_text()


def function(text, name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;{]*?\)\n\{', text)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


reconnect = function(source, 'SV_ReconnectClients')
spawn = function(source, 'SV_SpawnServer')
assert spawn.count('SV_ReconnectClients();') == 1 and 'ClientConnect(' not in spawn, \
    'SV_SpawnServer reconnects its clients only through SV_ReconnectClients'
assert spawn.index('[ckpt] baseline done') < spawn.index('SV_ReconnectClients();'), \
    'clients connect to the new level after its baselines'

support = r'''
#include <stdio.h>
#include <string.h>
typedef int qboolean;
typedef struct { struct { int integer; } current; } dvar_t;
enum { NA_BOT = 0, NA_IP = 4 };
typedef struct { int type; } netadr_t;
typedef struct { netadr_t remoteAddress; } netchan_t;
typedef struct { int serverTime, buttons, angles[3]; char weapon, offHandIndex, forwardmove, rightmove; } usercmd_t;
typedef struct { int state; unsigned short scriptId; qboolean bIsTestClient; netchan_t netchan; } client_t;
typedef struct { client_t *clients; } serverStatic_t;

#define SLOTS 10
static client_t clients[SLOTS];
static serverStatic_t svsData = { clients };
static void *imp_svs = &svsData;
static dvar_t maxclientsDvar = { { SLOTS - 1 } };
static dvar_t *maxclientsPointer = &maxclientsDvar;
static void *imp_sv_maxclients = &maxclientsPointer;
static char calls[512];
static int failures;

static void call(char kind, int clientNum)
{
    char text[8];
    snprintf(text, sizeof text, "%s%c%d", calls[0] ? " " : "", kind, clientNum);
    strcat(calls, text);
}

static int number(const client_t *cl)
{
    return (int)(cl - clients);
}

/* Slots 5 and 8 are refused, as a wrong password refuses a client. */
const char *ClientConnect(int clientNum, unsigned short scriptId)
{
    call('C', clientNum);
    if (scriptId != 100 + clientNum)
        failures++;
    return clientNum == 5 || clientNum == 8 ? "GAME_INVALIDPASSWORD" : NULL;
}

void SV_DropClient(void *client, const char *reason)
{
    client_t *cl = client;
    call('D', number(cl));
    if (strcmp(reason, "GAME_INVALIDPASSWORD"))
        failures++;
    cl->state = 1;
}

void SV_BotResetClient(int clientNum)
{
    call('R', clientNum);
}

void SV_SendClientGameState(client_t *cl)
{
    call('G', number(cl));
    if (cl->state != 2)
        failures++;
    cl->state = 3;
}

void SV_ClientEnterWorld(client_t *cl, const dvar_t *(*cmd)[4])
{
    static const usercmd_t none;
    call('E', number(cl));
    if (cl->state != 3 || memcmp(cmd, &none, sizeof none))
        failures++;
    cl->state = 4;
}
'''
checks = r'''
int main(void)
{
    /* free, zombie, person, bot, connecting person, refused bot, primed person,
     * bot, refused person, and a bot past sv_maxclients. */
    static const int state[SLOTS] = { 0, 1, 4, 4, 2, 4, 3, 4, 4, 4 };
    static const int bot[SLOTS] = { 0, 0, 0, 1, 0, 1, 0, 1, 0, 1 };
    static const int want[SLOTS] = { 0, 1, 2, 4, 2, 1, 2, 4, 1, 4 };
    const char *order = "C2 C3 R3 G3 E3 C4 C5 D5 C6 C7 R7 G7 E7 C8 D8";
    int i;

    for (i = 0; i < SLOTS; i++) {
        clients[i].state = state[i];
        clients[i].scriptId = (unsigned short)(100 + i);
        clients[i].bIsTestClient = bot[i];
        clients[i].netchan.remoteAddress.type = bot[i] ? NA_BOT : NA_IP;
    }
    SV_ReconnectClients();
    if (strcmp(calls, order)) {
        printf("calls %s\nwant  %s\n", calls, order);
        failures++;
    }
    for (i = 0; i < SLOTS; i++) {
        if (clients[i].state != want[i]) {
            printf("client %d: state %d, want %d\n", i, clients[i].state, want[i]);
            failures++;
        }
    }
    return failures != 0;
}
'''

bot_branch = reconnect[reconnect.index('        if (cl->bIsTestClient'):reconnect.index('    }\n}\n')]
mutants = [
    # As it was: a bot stays connecting.
    (bot_branch, ''),
    ('            SV_BotResetClient(i);\n', ''),
    ('            SV_SendClientGameState(cl);\n', ''),
    ('            SV_DropClient(cl, denied);\n            continue;\n', '            SV_DropClient(cl, denied);\n'),
    ('        if (cl->state <= 1)', '        if (cl->state < 1)'),
    ('        cl->state = 2;\n\n', '\n'),
]

with tempfile.TemporaryDirectory(prefix='cod2-bot-map-reconnect-') as directory:
    path = Path(directory)
    for index, (before, after) in enumerate([(None, None)] + mutants):
        if before is None:
            body = reconnect
        else:
            assert reconnect.count(before) == 1, before
            body = reconnect.replace(before, after)
        (path / 'test.c').write_text(support + body + checks)
        flags = ['-Wall', '-Werror'] if before is None else ['-w']
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        *flags, str(path / 'test.c'), '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True)
        if before is None:
            assert result.returncode == 0, result.stdout + result.stderr
        else:
            assert result.returncode == 1, f'mutant survived or crashed: {after!r}\n{result.stdout}{result.stderr[-2000:]}'

print('PASS: on a map change every connected client connects to the new level and people wait to answer its '
      'gamestate, while each bot is reset, primed and begins at once with an empty command; refused clients drop, '
      f'free and zombie slots and slots past sv_maxclients stay as they are; {len(mutants)} mutants fail')
