#!/usr/bin/env python3
"""Sprites spin by their rotationDelta channel and step through their atlas
sequences as retail's 0x495ed0 and 0x495f70 do; tails step but do not spin."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
fx = (root / 'src/PC/EffectsCore/FxPrimitives.c').read_text()


def function(source, name):
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


def calls(source, name):
    """The particle-step calls at the end of an update, in order."""
    body = function(source, name)
    return re.findall(r'Particle_Update(RGB|Alpha|Rotation|Subimage)\(', body)


support = r'''
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char byte;
typedef struct Material {int frames;} Material;
typedef Material *MaterialHandle;
typedef struct {float start, end;} Curve;
typedef struct {struct {const Curve *master;} curveIterator; float scale;} FxChannelInstance;
typedef struct {
 float normTime; int mFlags; int mTimeStart;
 struct {MaterialHandle customMaterial; float rotation; int materialSubimageIndex;} mRefEnt;
} Effect;
typedef struct {
 Effect base; FxChannelInstance rotationDeltaChannelInstance, rotationDeltaRandChannelInstance;
 float blendWeight[5]; int startFrame; float frameRate; int sequenceLoopMode, sequenceLoopTimes;
} Particle;
typedef struct {int mTime, mFrameTime;} FxHelper;
static FxHelper helper, *helperPointer = &helper;
static void *imp_theFxHelper = &helperPointer;
#define FXH_TIME(p) (((FxHelper *)(p))->mTime)
/* A channel's curve, straight from its start to its end over the life. */
static float EvalCurve1(char *channel, float normTime) {
 const Curve *curve = ((FxChannelInstance *)channel)->curveIterator.master;
 return curve->start + (curve->end - curve->start) * normTime;
}
static int countCalls;
int FxHelper_GetMaterialSubimageCount(void *helperArg, void *material) {
 assert(helperArg == helperPointer && material);
 countCalls++;
 return ((Material *)material)->frames;
}
'''

checks = r'''
static Material one = {1}, atlas16 = {16}, atlas4 = {4};
static Curve spin = {10, 10}, spinRand = {30, 30}, slowing = {20, 0};
static Particle particle;
static void spawn(MaterialHandle material, int startFrame, float frameRate, int loopMode, int loopTimes) {
 memset(&particle, 0, sizeof(particle));
 particle.base.mRefEnt.customMaterial = material;
 particle.base.mRefEnt.materialSubimageIndex = -7;
 particle.base.mTimeStart = 5000;
 particle.startFrame = startFrame;
 particle.frameRate = frameRate;
 particle.sequenceLoopMode = loopMode;
 particle.sequenceLoopTimes = loopTimes;
}
static int frameAt(int lived) {
 helper.mTime = 5000 + lived;
 Particle_UpdateSubimage(&particle);
 return particle.base.mRefEnt.materialSubimageIndex;
}
static float near(float a, float b) {return a - b < 0.0005f && b - a < 0.0005f;}
int main(void) {
 int tested = 0;
 /* One frame, or no material at all: always the first. */
 spawn(&one, 5, 0.024f, 0, 1);
 assert(frameAt(900) == 0); tested++;
 spawn(NULL, 5, 0.024f, 0, 1);
 assert(frameAt(900) == 0 && countCalls == 1); tested++;
 /* A looping 24 fps sequence from frame 4 of 16 wraps past the last frame. */
 spawn(&atlas16, 3, 0.024f, 0, 1);
 assert(frameAt(0) == 3 && frameAt(100) == 5 && frameAt(520) == 15); tested += 3;
 assert(frameAt(550) == 0 && frameAt(600) == 1 && frameAt(1200) == (3 + 28) % 16); tested += 3;
 /* Played over its life: the frame rate EffectsEd's rate mode 1 gives, 16 frames over 400 ms. */
 spawn(&atlas16, 0, 16.0f / 400.0f, 0, 1);
 assert(frameAt(0) == 0 && frameAt(199) == 7 && frameAt(399) == 15); tested += 3;
 /* A random start frame held still. */
 spawn(&atlas16, 11, 0.0f, 0, 1);
 assert(frameAt(0) == 11 && frameAt(3000) == 11); tested += 2;
 /* Two loops left: wrap through them, then hold the last frame for good. */
 spawn(&atlas4, 0, 0.01f, 1, 2);
 assert(frameAt(0) == 0 && frameAt(350) == 3 && frameAt(450) == 0 && frameAt(1150) == 3); tested += 4;
 assert(particle.sequenceLoopTimes == 2);
 assert(frameAt(1250) == 3 && particle.sequenceLoopTimes == 0); tested++;
 assert(frameAt(1300) == 3 && frameAt(2600) == 3); tested += 2;
 /* No loops: play once, then hold the last frame. */
 spawn(&atlas4, 1, 0.01f, 1, 0);
 assert(frameAt(150) == 2 && frameAt(250) == 3 && frameAt(450) == 3 && frameAt(5000) == 3); tested += 4;

 /* rotationDelta turns the sprite that many degrees every 100 ms, by its scale. */
 memset(&particle, 0, sizeof(particle));
 particle.rotationDeltaChannelInstance.curveIterator.master = &spin;
 particle.rotationDeltaRandChannelInstance.curveIterator.master = &spinRand;
 particle.rotationDeltaChannelInstance.scale = 2.0f;
 particle.rotationDeltaRandChannelInstance.scale = 100.0f;
 particle.blendWeight[3] = 0.9f;
 particle.blendWeight[4] = 0.25f;
 particle.base.mRefEnt.rotation = 45.0f;
 helper.mFrameTime = 8;
 for (int i = 0; i < 10; i++) Particle_UpdateRotation(&particle);
 assert(near(particle.base.mRefEnt.rotation, 45.0f + 10 * 10 * 2 * 8 * 0.01f)); tested++;
 /* useRandomRotationDelta blends toward the second curve by the particle's own weight. */
 particle.base.mFlags = 0x40000;
 particle.base.mRefEnt.rotation = 0.0f;
 Particle_UpdateRotation(&particle);
 assert(near(particle.base.mRefEnt.rotation, (10 + (30 - 10) * 0.25f) * 2 * 8 * 0.01f)); tested++;
 /* The rate follows its curve over the life: spinning down to rest. */
 particle.base.mFlags = 0;
 particle.rotationDeltaChannelInstance.curveIterator.master = &slowing;
 particle.base.mRefEnt.rotation = 0.0f;
 particle.base.normTime = 0.5f;
 helper.mFrameTime = 100;
 Particle_UpdateRotation(&particle);
 assert(near(particle.base.mRefEnt.rotation, 10 * 2)); tested++;
 particle.base.normTime = 1.0f;
 Particle_UpdateRotation(&particle);
 assert(near(particle.base.mRefEnt.rotation, 10 * 2)); tested++;
 printf("PASS: %d atlas frames and turns: wrapped, counted and held loops, one-frame and missing materials, "
        "random and played-over-life starts, spin rates and their random blend\n", tested);
 return 0;
}
'''


def run(code, directory):
    path = Path(directory)
    (path / 'test.c').write_text(code)
    built = subprocess.run(['cc', '-std=c11', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                            str(path / 'test.c'), '-o', str(path / 'test')], capture_output=True, text=True)
    if built.returncode:
        return built
    return subprocess.run([str(path / 'test')], capture_output=True, text=True)


rotation = function(fx, 'Particle_UpdateRotation')
subimage = function(fx, 'Particle_UpdateSubimage')
order = {name: calls(fx, name) for name in ('Particle_Update', 'OrientedParticle_Update', 'Tail_Update',
                                            'Cloud_Update', 'Line_Update', 'Cylinder_Update', 'Emitter_Update')}


def static_checks(order):
    assert order['Particle_Update'] == ['RGB', 'Alpha', 'Rotation', 'Subimage'], order['Particle_Update']
    assert order['OrientedParticle_Update'] == ['RGB', 'Alpha', 'Rotation', 'Subimage'], order['OrientedParticle_Update']
    assert order['Tail_Update'] == ['RGB', 'Alpha', 'Subimage'], 'tails step their sequence but never spin'
    for name in ('Cloud_Update', 'Line_Update', 'Cylinder_Update', 'Emitter_Update'):
        assert 'Rotation' not in order[name] and 'Subimage' not in order[name], name


static_checks(order)
for field, offset in (('mRefEnt.customMaterial', '0x40'), ('mRefEnt.rotation', '0x44'),
                      ('mRefEnt.materialSubimageIndex', '0x94')):
    assert re.search(r'COD2_ASSERT_FIELD\(struct Effect, ' + re.escape(field) + r',\s+' + offset + r'\);', fx), field
for field, offset in (('startFrame', '0x108'), ('frameRate', '0x10c'), ('sequenceLoopMode', '0x110'),
                      ('sequenceLoopTimes', '0x114'), ('blendWeight', '0x118'),
                      ('rotationDeltaChannelInstance', '0x1a4'), ('rotationDeltaRandChannelInstance', '0x1b0')):
    assert re.search(r'COD2_ASSERT_FIELD\(Particle, ' + field + r',\s+' + offset + r'\);', fx), field

MUTANTS = [
    (subimage, 'frame % count;\n    } else if', 'frame;\n    } else if', 'a looping sequence that runs off its atlas'),
    (subimage, 'material ? FxHelper_GetMaterialSubimageCount(*(FxHelper **)imp_theFxHelper, material) : 1',
     'FxHelper_GetMaterialSubimageCount(*(FxHelper **)imp_theFxHelper, material)', 'asking a missing material'),
    (subimage, '(particle->sequenceLoopTimes + 1) * count', 'particle->sequenceLoopTimes * count', 'one loop short'),
    (subimage, '                    particle->sequenceLoopTimes = 0;\n', '', 'loops that never run out'),
    (subimage, '= count - 1;\n                    particle', '= 0;\n                    particle', 'ending on the first frame'),
    (subimage, '        } else if (frame >= count) {\n            ((Effect *)self)->mRefEnt.materialSubimageIndex = count - 1;',
     '        } else if (frame >= count) {\n            ((Effect *)self)->mRefEnt.materialSubimageIndex = frame % count;',
     'a sequence without loops that wraps'),
    (subimage, 'particle->startFrame +\n', '0 +\n', 'ignoring the start frame'),
    (subimage, '- ((Effect *)self)->mTimeStart)', ')', 'the time since the map began instead of since the spawn'),
    (rotation, '* 0.01f;', '* 0.001f;', 'a tenth of the spin'),
    (rotation, 'blendWeight[4]', 'blendWeight[3]', 'the size2 weight for the spin'),
    (rotation, '    delta *= ((Particle *)self)->rotationDeltaChannelInstance.scale;\n', '', 'an unscaled spin'),
    (rotation, 'mFlags & 0x40000', 'mFlags & 0x4000', 'the alpha flag for the spin'),
    (rotation, 'mRefEnt.rotation +=', 'mRefEnt.rotation =', 'a turn that does not accumulate'),
]

with tempfile.TemporaryDirectory(prefix='cod2-fx-sequence-') as directory:
    code = support + rotation + 'extern int FxHelper_GetMaterialSubimageCount(void *helper, void *material);\n' + \
        subimage + checks
    result = run(code, directory)
    assert result.returncode == 0, result.stdout + result.stderr
    print(result.stdout, end='')
    for source, old, new, label in MUTANTS:
        assert source.count(old) == 1, label
        mutant = code.replace(source, source.replace(old, new))
        assert run(mutant, directory).returncode != 0, label + ' must be rejected'
    for name, drop in (('Tail_Update', 'Subimage'), ('Particle_Update', 'Rotation'),
                       ('OrientedParticle_Update', 'Subimage')):
        broken = dict(order)
        broken[name] = [step for step in order[name] if step != drop]
        try:
            static_checks(broken)
        except AssertionError:
            continue
        raise AssertionError(f'{name} without its {drop} step must be rejected')
    broken = dict(order)
    broken['Tail_Update'] = ['RGB', 'Alpha', 'Rotation', 'Subimage']
    try:
        static_checks(broken)
    except AssertionError:
        pass
    else:
        raise AssertionError('a spinning tail must be rejected')
    print(f'PASS: {len(MUTANTS)} sequence and spin mutants and 4 call-order mutants rejected')
