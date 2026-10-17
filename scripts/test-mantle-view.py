#!/usr/bin/env python3
"""Exercise the production mantle-start camera easing from cg_view_mp.c."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_view_mp.c').read_text()
start = source.index('static struct {\n    vec3_t offset;\n    vec3_t lastOrigin;')
match = re.search(r'static void CG_SmoothMantleView\([^;]*?\)\n\{', source[start:])
assert match, 'CG_SmoothMantleView'
end, depth = start + match.end(), 1
while depth:
    depth += (source[end] == '{') - (source[end] == '}')
    end += 1
assert 'CG_SmoothMantleView(ps, cg->time, cg->refdef.vieworg);' in source

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef int qboolean;
typedef float vec_t, vec3_t[3];
typedef struct {int pm_flags; vec3_t origin;} playerState_t;
'''

checks = r'''
static playerState_t ps;
static float eye[3];
static void frame(int time, float x, float y, float z, int mantling) {
    ps.origin[0] = x; ps.origin[1] = y; ps.origin[2] = z;
    ps.pm_flags = mantling ? 4 | 0x100 : 0x100;
    eye[0] = x; eye[1] = y; eye[2] = z + 60.0f;
    CG_SmoothMantleView(&ps, time, eye);
}
static float offset(int axis, float base) { return eye[axis] - base; }
int main(void) {
    int time = 1000;
    for (; time < 1200; time += 16) {
        frame(time, 0.25f * (time - 1000), 0, 0, 0);
        assert(eye[0] == ps.origin[0] && eye[2] == 60.0f);
    }
    /* Mantle_Start aligns the player 9 units back and 14 up in one move. */
    float before[3] = {eye[0], eye[1], eye[2]};
    frame(time, before[0] - 9.0f, 0, 14.0f, 1);
    for (int i = 0; i < 3; ++i)
        assert(fabsf(eye[i] - before[i]) < 1e-4f);
    float previous = 9.0f;
    for (int elapsed = 16; elapsed < 200; elapsed += 16) {
        float climb = 0.5f * elapsed;
        frame(time + elapsed, before[0] - 9.0f, 0, 14.0f + climb, 1);
        float back = offset(0, ps.origin[0]);
        assert(back >= 0.0f && back < previous);
        assert(fabsf(offset(2, ps.origin[2] + 60.0f) * 9.0f + back * 14.0f) < 1e-3f);
        if (elapsed == 96)
            assert(fabsf(back - 9.0f * (1.0f - 0.48f) * (1.0f - 0.48f) * (1.0f + 2.0f * 0.48f)) < 1e-3f);
        previous = back;
    }
    frame(time + 200, before[0] - 9.0f, 0, 120.0f, 1);
    assert(eye[0] == ps.origin[0] && eye[2] == 180.0f);
    /* Root motion during the climb and its end are never eased. */
    frame(time + 216, before[0] + 20.0f, 0, 140.0f, 1);
    frame(time + 232, before[0] + 31.0f, 0, 122.0f, 0);
    assert(eye[0] == ps.origin[0] && eye[2] == 182.0f);

    /* Spawn-sized teleports and stationary starts stay exact. */
    time += 1000;
    frame(time, 0, 0, 0, 0);
    frame(time + 16, 300.0f, 0, 0, 1);
    assert(eye[0] == 300.0f && eye[2] == 60.0f);
    frame(time + 32, 300.0f, 0, 0, 0);
    frame(time + 48, 300.2f, 0, 0.2f, 1);
    assert(eye[0] == ps.origin[0] && fabsf(eye[2] - 60.2f) < 1e-4f);

    /* A long pause before the mantle is not a jump between rendered frames. */
    frame(time + 64, 300.2f, 0, 0.2f, 0);
    frame(time + 400, 290.0f, 0, 12.0f, 1);
    assert(eye[0] == 290.0f && eye[2] == 72.0f);

    /* A restarted clock drops the previous level's state. */
    frame(time + 416, 290.0f, 0, 12.0f, 0);
    frame(50, 0, 0, 0, 0);
    frame(66, 0, 0, 10.0f, 1);
    assert(fabsf(eye[2] - 60.0f) < 1e-4f);
    frame(30, 5.0f, 0, 0, 1);
    assert(eye[0] == 5.0f && eye[2] == 60.0f);
    puts("PASS: mantle start eases a 16.6-unit alignment over 200 ms; climb motion, teleports, stalls and clock resets stay exact");
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix='cod2-mantle-view-') as tmp:
    path = Path(tmp)
    (path / 'test.c').write_text(support + source[start:end] + '\n' + checks)
    subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Werror', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
