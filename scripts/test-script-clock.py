#!/usr/bin/env python3
"""Run the production script clock at several server rates: waits, counted in 20 ticks a second, last as long as
they say, so the killcam's replay reaches the death and a few seconds past it."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
vm = (root / 'src/PC/script/scr_vm.c').read_text()
game = (root / 'src/PC/game_mp/g_main_mp.c').read_text()


def function(text, name):
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', text, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


# The frame loop advances the clock by the frame's game time where the stock
# loop ticked once, after the triggers' and animations' threads ran and before
# entities think; nothing else ticks the VM.
frame = function(game, 'G_RunFrame')
assert frame.count('Scr_AdvanceTime(level.frametime);') == 1 and 'Scr_IncTime' not in frame
assert frame.index('G_DObjUpdateServerTime(entPtr, 1)') < frame.index('Scr_AdvanceTime(') < \
    frame.index('level.currentEntityThink = 0;')
assert frame.index('level.frametime = levelTime - level.previousTime;') < frame.index('Scr_AdvanceTime(')
assert 'Scr_IncTime' not in game and vm.count('Scr_IncTime();') == 1
assert function(vm, 'Scr_AdvanceTime').count('Scr_IncTime();') == 1
# Waits are counted in twentieths of a second.
assert 'ticks = (int)floorf(seconds * 20.0f + 0.5f);' in function(vm, 'VM_CandidateWaitTimeFromValue')

define = re.search(r'#define SCR_TICK_MSEC \d+\n', vm).group(0)
support = r'''
#include <assert.h>
#include <stdio.h>
static int ticks, pending, freed;
static void Scr_RunPendingThreads(void) { pending++; }
static void Scr_FreeEntityList(void) { freed++; }
static void Scr_IncTime(void) { Scr_RunPendingThreads(); Scr_FreeEntityList(); ticks++; }
''' + define
checks = r'''
/* One server frame of the given length; whether the VM ticked. */
static int step(int msec) {
    int before = ticks, p = pending, f = freed;
    Scr_AdvanceTime(msec);
    /* Pending threads run and freed entities go every frame, ticking or not. */
    assert(pending == p + 1 && freed == f + 1 && ticks - before <= 1);
    return ticks - before;
}
/* Game time from now until the VM has ticked n more times, at a frame size. */
static int waitTicks(int n, int msec) {
    int elapsed = 0, start = ticks;
    while (ticks - start < n) { step(msec); elapsed += msec; }
    return elapsed;
}
int main(void) {
    int rates[] = {20, 25, 30, 40, 60}, checks = 0;
    for (int r = 0; r < 5; ++r) {
        int msec = 1000 / rates[r], start = ticks, elapsed = 0;
        /* A minute of frames ticks 20 times a second of game time. */
        while (elapsed < 60000) { step(msec); elapsed += msec; }
        int expected = elapsed / 50;
        assert(ticks - start >= expected - 1 && ticks - start <= expected + 1);
        /* The killcam: three seconds dead, a frame for the archive, then the
         * replay's 9.95 s, all against a ten-second archive delay. */
        int dead = waitTicks(60, msec), trim = waitTicks(1, msec), replay = waitTicks(199, msec);
        assert(dead >= 2950 && dead <= 3050 + msec);
        int end = dead + trim + replay;
        int lastShown = end - 10000;
        assert(lastShown >= 2900 && lastShown <= 3100 + 2 * msec);
        checks += 3;
    }
    /* A 20 Hz server ticks every frame, as the stock loop did. */
    for (int i = 0; i < 100; ++i) assert(step(50) == 1);
    /* A slower server ticks every frame without banking the time, so a later
     * faster rate does not run scripts ahead. */
    for (int i = 0; i < 200; ++i) assert(step(100) == 1);
    int start = ticks, elapsed = 0;
    while (elapsed < 3000) { step(33); elapsed += 33; }
    assert(ticks - start >= 58 && ticks - start <= 61);
    /* A frame of no time, or a clock that went back, does not tick; a tick
     * comes once 50 ms have passed since the last. */
    for (int i = 0; i < 3; ++i) assert(step(0) == 0 && step(-200) == 0);
    assert(step(100) == 1 && step(0) == 0 && step(49) == 0 && step(1) == 1 && step(0) == 0);
    printf("%d\n", checks + 5);
    return 0;
}
'''
body = function(vm, 'Scr_AdvanceTime')
mutants = [
    ('    residual -= SCR_TICK_MSEC;\n', ''),
    ('    if (residual < SCR_TICK_MSEC) {', '    if (residual < 0) {'),
    ('    if (residual >= SCR_TICK_MSEC)\n        residual = 0;\n', ''),
    ('    residual += msec > 0 ? msec : 0;', '    residual += msec;'),
    ('        Scr_RunPendingThreads();\n        Scr_FreeEntityList();\n        return;', '        return;'),
]
with tempfile.TemporaryDirectory(prefix='cod2-script-clock-') as directory:
    path = Path(directory)
    for index, (before, after) in enumerate([(None, None)] + mutants):
        code = body if before is None else body.replace(before, after)
        assert before is None or body.count(before) == 1, before
        (path / 'clock.c').write_text(support + code + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Werror', '-fsanitize=address,undefined',
                        '-fno-sanitize-recover=all', str(path / 'clock.c'), '-o', str(path / 'clock')], check=True)
        result = subprocess.run([str(path / 'clock')], capture_output=True, text=True, timeout=60)
        assert (result.returncode == 0) == (index == 0), (index, after, result.stderr[-1500:])
        if index == 0:
            cases = int(result.stdout)
print(f'PASS: the script clock ticks 20 times a second of game time at 20 to 60 Hz, once a frame at most, running '
      f'pending threads and freeing entities every frame; three seconds dead last three seconds and the killcam\'s '
      f'replay ends three seconds past the death; slower rates bank nothing; {cases} cases; '
      f'{len(mutants)} failing mutants')
