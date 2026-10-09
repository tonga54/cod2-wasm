#!/usr/bin/env python3
"""Exercise the actual native server-list insertion with a small C harness."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/ui_mp/ui_main_mp.c').read_text()
functions = []
for name in ('UI_InsertServerAtPosition', 'UI_BinaryInsertServer'):
    start = source.index('static void ' + name + '(')
    end = source.index('\n}', start) + 2
    functions.append(source[start:end])
harness = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
struct { struct {
    int numDisplayServers, displayServers[128], currentServer, sortKey, sortDir;
} serverStatus; } sharedUiInfo;
struct { struct { int integer; } current; } sourceDvar;
#define ui_netSource (&sourceDvar)
static int values[128];
static int LAN_CompareServers(int source, int key, int direction, int a, int b) {
    (void)source; (void)key;
    int cmp = (values[a] > values[b]) - (values[a] < values[b]);
    return direction ? -cmp : cmp;
}
'''
harness += '\n'.join(functions)
harness += r'''
int main(void) {
    int checks = 0;
    for (int direction = 0; direction < 2; ++direction) {
        for (int seed = 0; seed < 100; ++seed) {
            memset(&sharedUiInfo, 0, sizeof(sharedUiInfo));
            sharedUiInfo.serverStatus.sortDir = direction;
            for (int i = 0; i < 128; ++i) {
                values[i] = (i * 37 + seed * 13) % 17;
                UI_BinaryInsertServer(i);
                assert(sharedUiInfo.serverStatus.numDisplayServers == i + 1);
                for (int j = 1; j <= i; ++j)
                    assert(LAN_CompareServers(0, 0, direction,
                        sharedUiInfo.serverStatus.displayServers[j - 1],
                        sharedUiInfo.serverStatus.displayServers[j]) <= 0);
                ++checks;
            }
        }
    }
    printf("PASS: %d server insertions, empty list, duplicates, both sort directions\n", checks);
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-server-list-') as directory:
    path = Path(directory)
    (path / 'test.c').write_text(harness)
    subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                    str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
