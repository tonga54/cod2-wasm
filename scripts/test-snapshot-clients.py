#!/usr/bin/env python3
"""Check real snapshot client iteration against the current 32-bit layouts.

Run with the native builder (32-bit libc headers/compiler required).
"""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/server_mp/sv_snapshot_mp.c').read_text()


def function(name):
    start = source.index('static ', source.index(name) - 20)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


support = r'''
#include "common_types.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static serverStatic_t server;
static void *imp_svs = &server;
static int maxClients;
static byte *sv_maxclients_dvar;
static clientState_t states[64], output[64];
static int SV_DvarIntLocal(byte *ignored) { return maxClients; }
static clientState_t *SV_SnapshotClientLocal(serverStatic_t *s, int n) {
    assert(n >= 0 && n < 64);
    return &output[n];
}
static clientState_t *G_GetClientState(int n) { return &states[n]; }
static void Com_Error(int code, const char *message) { abort(); }
'''
checks = r'''
int main(void) {
    server.clients = calloc(64, sizeof(client_t));
    assert(server.clients);
    for (int count = 1; count <= 64; ++count) {
        maxClients = count;
        for (int pattern = 0; pattern < 4; ++pattern) {
            clientSnapshot_t frame = {0};
            int expected = 0;
            memset(output, 0, sizeof(output));
            server.nextSnapshotClients = 0;
            for (int i = 0; i < count; ++i) {
                assert(SV_ClientIndexLocal(&server.clients[i]) == i);
                states[i].clientIndex = i;
                states[i].modelindex = 100 + i;
                states[i].team = (i % 2) + 1;
                server.clients[i].state = pattern == 0 ? 4 : (i + pattern) % 5;
                if (server.clients[i].state > 1) ++expected;
            }
            SV_CopyCurrentClientsToSnapshotLocal(&frame);
            assert(frame.num_clients == expected);
            assert(server.nextSnapshotClients == expected);
            int n = 0;
            for (int i = 0; i < count; ++i) {
                if (server.clients[i].state <= 1) continue;
                assert(memcmp(&output[n++], &states[i], sizeof(clientState_t)) == 0);
            }
        }
    }
    free(server.clients);
    return 0;
}
'''
body = function('SV_ClientIndexLocal') + '\n' + function('SV_CopyCurrentClientsToSnapshotLocal')
compiler = shlex.split(os.environ.get('CC', 'cc'))
with tempfile.TemporaryDirectory(prefix='cod2-snapshot-clients-') as directory:
    path = Path(directory)
    for patch in ('COD2_PATCH_10', 'COD2_PATCH_13'):
        for mutant in ('none', 'index', 'iteration'):
            altered = body
            if mutant == 'index':
                altered = altered.replace('(client - svs->clients)',
                    '(((byte *)client - (byte *)svs->clients) / 0x78f0c)')
            if mutant == 'iteration':
                altered = altered.replace('&svs->clients[clientNum]',
                    '(client_t *)((byte *)svs->clients + clientNum * 0x78f0c)')
            (path / 'test.c').write_text(support + altered + checks)
            subprocess.run(compiler + ['-m32', '-std=gnu99', '-D' + patch,
                '-I' + str(root / 'src/headers'), '-I' + str(root / 'src'),
                str(path / 'test.c'), '-o', str(path / 'test')], check=True)
            result = subprocess.run([str(path / 'test')], capture_output=True)
            # The literal is the 1.0 layout. It breaks when the 1.3 network
            # buffers and download state increase client_t, as in our build.
            should_pass = mutant == 'none' or patch == 'COD2_PATCH_10'
            assert (result.returncode == 0) == should_pass, (patch, mutant, result.stderr)
print('PASS: both patch layouts, 1–64 clients, mixed states; old strides fail on 1.3')
