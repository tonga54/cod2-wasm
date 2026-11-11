#!/usr/bin/env python3
"""Check the names the server lends anonymous clients: Player, Player 2 and so
on in arrival order, the lowest free name, kept while the server runs and freed
with the slot; placeholders aside, including the browser's, the engine's, a
blank one and one of color codes."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/game_mp/g_client_mp.c').read_text()


def function(signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


body = ''.join(function(signature) for signature in (
    'static int G_AnonymousPlaceholder(const char *name)\n{',
    'static int G_AnonymousNameTaken(const char *name, int clientNum)\n{',
    'static void G_AnonymousName(int clientNum, const char *rawName)\n{',
    'qboolean G_ClientIsAnonymous(int clientNum)\n{',
    'void G_AnonymousClientDisconnect(int clientNum)\n{',
))

# The userinfo change lends the name and the disconnect gives the slot back.
assert 'G_AnonymousName(clientNum, Info_ValueForKey(userinfo, "name"));' in function(
    'void ClientUserinfoChanged(int clientNum)\n{')
assert 'G_AnonymousClientDisconnect(clientNum);' in function('void ClientDisconnect(int clientNum)\n{')

support = r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
typedef int qboolean;
typedef struct { char name[32]; } clientState_t;
typedef struct { int connected; clientState_t cs; char newnetname[32]; } clientSession_t;
typedef struct gclient_s { clientSession_t sess; } gclient_t;
typedef struct gclient_s gclient_s;
typedef struct { gclient_t *clients; int maxclients; } level_locals_t;
static gclient_t clients[64];
level_locals_t level;
enum { ANONYMOUS_MAX_CLIENTS = 64, ANONYMOUS_NAME_SIZE = 32 };
static char s_anonymousName[ANONYMOUS_MAX_CLIENTS][ANONYMOUS_NAME_SIZE];
static int I_stricmp(const char *a, const char *b) { return strcasecmp(a, b); }
static void I_strncpyz(char *dest, const char *src, int size) { snprintf(dest, (size_t)size, "%s", src); }
static const char *va(const char *fmt, ...) {
    static char buffers[4][64];
    static int next;
    char *out = buffers[next++ & 3];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(out, sizeof(buffers[0]), fmt, ap);
    va_end(ap);
    return out;
}
'''

checks = r'''
static void reset(void) {
    memset(clients, 0, sizeof(clients));
    memset(s_anonymousName, 0, sizeof(s_anonymousName));
    level.clients = clients;
    level.maxclients = 16;
}
static void joins(int n, const char *raw) {
    clients[n].sess.connected = 1;
    G_AnonymousName(n, raw);
}
static void shows(int n, const char *want) {
    assert(!strcmp(clients[n].sess.cs.name, want));
    assert(!strcmp(clients[n].sess.newnetname, want));
}
/* The parser writes a custom name before the slot is lent one. */
static void custom(int n, const char *name) {
    clients[n].sess.connected = 1;
    snprintf(clients[n].sess.cs.name, sizeof(clients[n].sess.cs.name), "%s", name);
    snprintf(clients[n].sess.newnetname, sizeof(clients[n].sess.newnetname), "%s", name);
    G_AnonymousName(n, name);
}
int main(void) {
    /* Arrival order, every placeholder the browser and the engine use. */
    reset();
    joins(0, "Player"); shows(0, "Player");
    joins(1, "Player"); shows(1, "Player 2");
    joins(2, " Unknown Soldier "); shows(2, "Player 3");
    joins(3, ""); shows(3, "Player 4");
    joins(4, "^1Player"); shows(4, "Player 5");
    assert(G_ClientIsAnonymous(0) && G_ClientIsAnonymous(4));

    /* A new map or another userinfo change keeps the name it was lent. */
    memset(&clients[1].sess.cs, 0, sizeof(clients[1].sess.cs));
    memset(clients[1].sess.newnetname, 0, sizeof(clients[1].sess.newnetname));
    joins(1, "Player"); shows(1, "Player 2");
    joins(1, "UnnamedPlayer"); shows(1, "Player 2");

    /* A custom name is its own and frees the number for the next anonymous. */
    custom(1, "Fabi");
    assert(!strcmp(clients[1].sess.cs.name, "Fabi"));
    assert(!G_ClientIsAnonymous(1));
    joins(5, "Player"); shows(5, "Player 2");

    /* Disconnecting frees the slot; its old name no longer blocks. */
    G_AnonymousClientDisconnect(5);
    assert(!G_ClientIsAnonymous(5));
    clients[5].sess.connected = 0;
    joins(6, "Player"); shows(6, "Player 2");

    /* A custom name that reads as a lent one blocks it, in any case. */
    reset();
    custom(7, "player 2");
    joins(0, "Player"); shows(0, "Player");
    joins(1, "Player"); shows(1, "Player 3");
    joins(2, "Player"); shows(2, "Player 4");
}
'''

mutants = [
    ('!length || !I_stricmp(key, "Player") || !I_stricmp(key, "Unknown Soldier") ||\n           !I_stricmp(key, "UnnamedPlayer");',
     '!length || !I_stricmp(key, "Unknown Soldier") ||\n           !I_stricmp(key, "UnnamedPlayer");'),
    ('return !length || !I_stricmp(key, "Player")', 'return !I_stricmp(key, "Player")'),
    ('if (name[0] == \'^\' && name[1] >= \'0\' && name[1] <= \'9\')\n            ++name;',
     'if (0)\n            ++name;'),
    ('    while (length && key[length - 1] == \' \')\n        --length;\n', ''),
    ('if (i == clientNum || level.clients[i].sess.connected == 0)\n            continue;',
     'if (i == clientNum)\n            continue;'),
    ('if (!G_AnonymousNameTaken(candidate, clientNum)) {', 'if (G_AnonymousNameTaken(candidate, clientNum)) {'),
    ('    if (clientNum >= 0 && clientNum < ANONYMOUS_MAX_CLIENTS)\n        s_anonymousName[clientNum][0] = \'\\0\';\n}',
     '    if (clientNum >= 0 && clientNum < ANONYMOUS_MAX_CLIENTS)\n        ;\n}'),
    ('    if (!G_AnonymousPlaceholder(rawName)) {\n        s_anonymousName[clientNum][0] = \'\\0\';\n        return;\n    }',
     '    if (!G_AnonymousPlaceholder(rawName)) {\n        return;\n    }'),
]


def run(directory, name, text, warnings, expect_pass):
    path = Path(directory) / f'{name}.c'
    binary = Path(directory) / name
    path.write_text(text)
    subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    *warnings, str(path), '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert (result.returncode == 0) == expect_pass, (name, result.stdout[-500:], result.stderr[-3000:])
    return result


def program(text):
    return support + text + checks


strict = ['-Wall', '-Werror', '-Wno-unused-function']
with tempfile.TemporaryDirectory(prefix='cod2-anonymous-names-') as directory:
    run(directory, 'names', program(body), strict, True)
    for index, (old, new) in enumerate(mutants):
        assert body.count(old) == 1, old
        run(directory, f'names{index}', program(body.replace(old, new)), ['-w'], False)

print('PASS: anonymous clients show as Player, Player 2 and so on in arrival order, the lowest free name, kept across '
      'userinfo changes and map restarts and freed with the slot; a custom name takes its own and frees the number; '
      'the browser placeholder, the engine\'s, a blank name and color codes only count as anonymous; '
      f'{len(mutants)} failing regression mutants')
