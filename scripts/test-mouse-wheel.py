#!/usr/bin/env python3
"""Run the production mouse wheel translation on browser scrolls as SDL 2.28 reports them."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/unix/linux_input.c').read_text()

# The event loop hands SDL's precise scroll to the translation and nothing else.
case = source[source.index('        case SDL_MOUSEWHEEL: {'):source.index('        case SDL_KEYDOWN: {')]
assert case.count('IN_MouseWheel(ev.wheel.preciseY);') == 1 and 'DoMouseWheel' not in case, case
assert source.count('CCallOfDutyEngine_DoMouseWheel(get_engine(), steps, point, 0);') == 1
defines = '\n'.join(re.findall(r'^#define IN_(?:WHEEL_NOTCH|MAX_WHEEL_STEPS) [\d.]+f?$', source, re.M))
assert defines.count('#define') == 2, defines
start = source.index('static float s_wheelPending;')
function = source[start:source.index('\n}\n', start) + 3]

support = r'''
#include <assert.h>
#include <stdio.h>
typedef int SInt32;
typedef unsigned int UInt32;
typedef struct { short v, h; } Point;
typedef struct CCallOfDutyEngine CCallOfDutyEngine;
static const CCallOfDutyEngine *get_engine(void) { return 0; }
static int up, down;
/* The engine's own mapping (mac_input.c): anything not upward is downward. */
void CCallOfDutyEngine_DoMouseWheel(const CCallOfDutyEngine *engine, SInt32 delta, Point point, UInt32 modifiers)
{
    (void)engine; (void)point; (void)modifiers;
    if (delta > 0)
        up++;
    else
        down++;
}
'''

checks = r'''
/* SDL 2.28's browser wheel posts every scroll with preciseY, the page's
 * pixels a hundredth of a step, negated: scrolling up is positive. */
static void scroll(float pixels, int times)
{
    while (times-- > 0)
        IN_MouseWheel(-pixels / 100.0f);
}
static void expect(int wantUp, int wantDown)
{
    assert(up == wantUp && down == wantDown);
    up = down = 0;
}
/* A notch drops whatever small scrolls had gathered; its own step is not counted. */
static void reset(void)
{
    IN_MouseWheel(1.0f);
    up = down = 0;
}

int main(void)
{
    /* A trackpad's small scrolls add up to a step every 100 pixels, the way
     * the fingers went. */
    scroll(-12.5f, 32);
    expect(4, 0);
    scroll(12.5f, 32);
    expect(0, 4);
    /* A notch is a step: Chrome's 40 pixels on a Mac, 100 on Windows; a
     * quick flick a few, a page at most three. */
    scroll(-40.0f, 5);
    expect(5, 0);
    scroll(100.0f, 2);
    expect(0, 2);
    scroll(-250.0f, 1);
    expect(3, 0);
    scroll(8000.0f, 1);
    expect(0, 3);
    /* A notch drops what small scrolls had gathered. */
    reset();
    scroll(-25.0f, 3);
    scroll(40.0f, 1);
    scroll(-25.0f, 1);
    expect(0, 1);
    /* Sideways scrolls do nothing; small scrolls turned back start over. */
    reset();
    for (int i = 0; i < 10; ++i)
        IN_MouseWheel(0.0f);
    scroll(-25.0f, 3);
    scroll(25.0f, 2);
    expect(0, 0);
    scroll(25.0f, 2);
    expect(0, 1);
    puts("PASS");
    return 0;
}
'''


def mutate(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


mutants = [
    ('    for (i = 0; i < steps || i < -steps; ++i)\n        CCallOfDutyEngine_DoMouseWheel(get_engine(), steps, point, 0);',
     '    (void)i;\n    CCallOfDutyEngine_DoMouseWheel(get_engine(), steps, point, 0);'),
    ('    for (i = 0; i < steps || i < -steps; ++i)', '    for (i = 0; i < steps; ++i)'),
    ('        CCallOfDutyEngine_DoMouseWheel(get_engine(), steps, point, 0);',
     '        CCallOfDutyEngine_DoMouseWheel(get_engine(), 1, point, 0);'),
    ('    if (steps > IN_MAX_WHEEL_STEPS)\n        steps = IN_MAX_WHEEL_STEPS;\n'
     '    else if (steps < -IN_MAX_WHEEL_STEPS)\n        steps = -IN_MAX_WHEEL_STEPS;\n', ''),
    ('    if (size >= IN_WHEEL_NOTCH) {', '    if (size >= 1.0f) {'),
    ('        steps = (int)(size + 0.5f);', '        steps = (int)size;'),
    ('        s_wheelPending = 0.0f;\n    } else {', '    } else {'),
    ('        if ((precise < 0.0f && s_wheelPending > 0.0f) || (precise > 0.0f && s_wheelPending < 0.0f))\n'
     '            s_wheelPending = 0.0f;\n', ''),
]
variants = [(function, None)] + [(mutate(function, old, new), (old, new)) for old, new in mutants]
with tempfile.TemporaryDirectory(prefix='cod2-wheel-') as directory:
    path = Path(directory) / 'wheel.c'
    binary = Path(directory) / 'wheel'
    for body, mutant in variants:
        path.write_text(support + defines + '\n' + body + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wall', '-Werror' if mutant is None else '-w', str(path), '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (mutant is None), (mutant, result.stderr[-1500:])
print('PASS: browser scrolls turn the wheel the way the fingers went: a notch a step (40 pixels on a Mac, 100 on '
      'Windows), a flick a few, three at most; a trackpad a step every 100 pixels, starting over when it turns '
      f'back; sideways scrolls nothing; {len(mutants)} failing regression mutants')
