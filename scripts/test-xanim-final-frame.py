#!/usr/bin/env python3
"""A finished anim holds its last key: an aimed weapon's sights on the view axis, not a frame short."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root/'src/PC/xanim/xanim.c').read_text()


def function(name):
    start = source.index(name + '(')
    # Skip the declarations near the top of the file.
    while source.find(';', start) < source.find('{', start):
        start = source.index(name + '(', start + len(name))
    # The return type may sit on the line above the name.
    start = source.rfind('\n\n', 0, start) + 2
    brace = source.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#define __attribute_regparm__(n)
typedef unsigned char byte;
typedef struct { float time; int frameCount; float frameFrac; int frameIndex; } XAnimTime;
typedef union { byte _1[16]; unsigned short _2[8]; } XAnimDynamicIndices;
typedef struct { unsigned short numframes; } XAnimParts;
'''
body = ''.join(function(name) for name in (
    'Z18XAnim_GetTimeIndexIhEvPK9XAnimTimePK19XAnimDynamicIndicesiPiPf',
    'Z28XAnim_GetTimeIndexCompressedItEvPK9XAnimTimePKT_iPiPf',
    'XAnimCalcMakeTimeLocal', 'XAnimCalcGetFrameIndexLocal'))
checks = r'''
/* A track's value at time: keys[] at the frames its table names. */
static float sample(int frameCount, const XAnimDynamicIndices *indices, int tableSize, const float *keys, float time) {
    XAnimParts parts = { (unsigned short)frameCount };
    XAnimTime animTime;
    int index;
    float lerp;

    XAnimCalcMakeTimeLocal(&parts, time, &animTime);
    XAnimCalcGetFrameIndexLocal(&animTime, indices, tableSize, &index, &lerp);
    assert(index >= 0 && index < (tableSize > 0 ? tableSize : 1));
    assert(lerp >= -1e-6f && lerp <= 1.0f + 1e-6f);
    return tableSize > 0 ? keys[index] + (keys[index + 1] - keys[index]) * lerp : keys[0];
}
static int near(float a, float b) { return fabsf(a - b) <= 1e-4f; }
int main(void) {
    /* A key on each of 10 frames, as a viewmodel's ADS up anim has. */
    float squares[10];
    for (int k = 0; k < 10; k++)
        squares[k] = (float)(k * k);
    XAnimDynamicIndices none = { { 0 } };
    assert(near(sample(9, &none, 9, squares, 1.0f), 81.0f));
    assert(near(sample(9, &none, 9, squares, 0.0f), 0.0f));
    assert(near(sample(9, &none, 9, squares, 0.5f), 20.5f));
    assert(fabsf(sample(9, &none, 9, squares, 0.999f) - 81.0f) < 0.2f);
    /* Keys on frames 0, 3 and 9 only. */
    XAnimDynamicIndices sparse = { { 0, 3, 9 } };
    const float thirds[3] = { 0.0f, 30.0f, 90.0f };
    assert(near(sample(9, &sparse, 2, thirds, 1.0f), 90.0f));
    assert(near(sample(9, &sparse, 2, thirds, 0.5f), 45.0f));
    assert(near(sample(9, &sparse, 2, thirds, 0.0f), 0.0f));
    /* Over 255 frames the table holds shorts. */
    XAnimDynamicIndices wide;
    wide._2[0] = 0; wide._2[1] = 100; wide._2[2] = 300;
    assert(near(sample(300, &wide, 2, thirds, 1.0f), 90.0f));
    assert(near(sample(300, &wide, 2, thirds, 0.5f), 45.0f));
    /* A single-frame anim, and a time past the end, stay in range. */
    assert(near(sample(0, &none, 0, squares, 1.0f), 0.0f));
    assert(near(sample(9, &none, 9, squares, 1.5f), 81.0f));
    puts("ok");
    return 0;
}
'''
fixed = '        animTime->frameFrac = (float)animTime->frameCount;\n'
assert fixed in body
with tempfile.TemporaryDirectory(prefix='cod2-xanim-final-frame-') as directory:
    path = Path(directory)
    variants = [body, body.replace(fixed, '        animTime->frameFrac = (float)animTime->frameIndex;\n')]
    for index, variant in enumerate(variants):
        (path/'test.c').write_text(support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(path/'test.c'), '-o', str(path/'test'), '-lm'], check=True)
        result = subprocess.run([str(path/'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), result.stderr
print('PASS: 11 key samples: a finished anim holds its last key over every table, mid-anim lerps unchanged; '
      'the frame-short mutant fails')
