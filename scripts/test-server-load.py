#!/usr/bin/env python3
"""Compile the dedicated server's real load meter: late simulation frames and
process CPU time published once a second as sv_lateFrames and sv_cpuLoad."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
sv = (root / 'src/PC/server_mp/sv_main_mp.c').read_text()


def function(source, name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


state = sv[sv.index('static const dvar_t *sv_cpuLoad, *sv_lateFrames;'):sv.index('static double SV_ProcessCpuSeconds')]
frame = function(sv, 'SV_Frame')
# Every simulated frame of one SV_Frame call is counted, and the meter runs once
# per call after the snapshots are sent.
assert '++simulatedFrames;' in frame[frame.index('do {'):frame.index('} while (frameMsec <= sv.timeResidual);')]
assert frame.index('SV_SendClientMessages();') < frame.index('SV_UpdateLoad(simulatedFrames, Sys_Milliseconds());')
assert re.search(r'Dvar_RegisterInt\("sv_cpuLoad", 0, 0, 1000, 0x1040\)', sv)
assert re.search(r'Dvar_RegisterInt\("sv_lateFrames", 0, 0, 100, 0x1040\)', sv)

support = r'''
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
typedef struct { const char *name; int value, registered; } dvar_t;
static dvar_t dvars[2];
static dvar_t *Dvar_RegisterInt(const char *name, int value, int min, int max, int flags) {
  dvar_t *dvar = &dvars[!strcmp(name, "sv_lateFrames")];
  assert(!dvar->registered && flags == 0x1040 && min == 0 && max >= 100);
  dvar->name = name; dvar->value = value; dvar->registered = 1; return dvar;
}
static int sets;
static void Dvar_SetInt(const dvar_t *dvar, int value) { ((dvar_t *)dvar)->value = value; ++sets; }
'''
checks = r'''
static void spin(double seconds) {
  struct timespec start, now; clock_gettime(CLOCK_MONOTONIC, &start);
  do clock_gettime(CLOCK_MONOTONIC, &now);
  while ((now.tv_sec - start.tv_sec) + (now.tv_nsec - start.tv_nsec) * 1e-9 < seconds);
}
static int cpu(void) { return dvars[0].value; }
static int late(void) { return dvars[1].value; }
int main(void) {
  int now = 1000;
  // 30 on-time calls in one second publish nothing late.
  for (int i = 0; i < 30; ++i) { SV_UpdateLoad(1, now); now += 33; }
  SV_UpdateLoad(1, now += 10);
  assert(dvars[0].registered && dvars[1].registered && sets == 2 && late() == 0);
  // Ten calls each running three frames, two of them behind schedule: 20 of
  // the window's 31 frames ran late.
  for (int i = 0; i < 10; ++i) { SV_UpdateLoad(3, now); now += 100; }
  SV_UpdateLoad(1, now += 1);
  assert(late() == 20 * 100 / 31);
  // Nothing is published within a window.
  int before = sets;
  SV_UpdateLoad(1, now + 500);
  assert(sets == before);
  // A second spent mostly computing reads as most of a core; a second asleep
  // as almost none.
  SV_UpdateLoad(1, now += 1000);
  spin(0.6);
  SV_UpdateLoad(1, now += 1000);
  assert(cpu() >= 45 && cpu() <= 75);
  struct timespec rest = {0, 300000000}; nanosleep(&rest, 0);
  SV_UpdateLoad(1, now += 1000);
  assert(cpu() <= 10);
  puts("PASS: late simulation frames and process CPU per second, published once a window to read-only dvars");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-server-load-') as directory:
    path = Path(directory)
    body = state + function(sv, 'SV_ProcessCpuSeconds') + function(sv, 'SV_UpdateLoad')
    (path / 'test.c').write_text(support + body + checks)
    subprocess.run(['cc', '-O1', '-g', '-fsanitize=address,undefined', str(path / 'test.c'), '-o', str(path / 'test')],
                   check=True)
    subprocess.run([str(path / 'test')], check=True)
    mutants = {
        'late frames counted from every frame': body.replace('frames - 1;', 'frames;'),
        'CPU time not measured': body.replace('return now.tv_sec + now.tv_nsec * 1e-9;', 'return -1.0;'),
        'published every call': body.replace('if (elapsed < 1000)\n        return;', ''),
    }
    for name, mutant in mutants.items():
        assert mutant != body, name
        (path / 'mutant.c').write_text(support + mutant + checks)
        subprocess.run(['cc', '-O1', str(path / 'mutant.c'), '-o', str(path / 'mutant')], check=True)
        assert subprocess.run([str(path / 'mutant')], capture_output=True).returncode != 0, name
    print(f'PASS: {len(mutants)} load meter mutants fail')
