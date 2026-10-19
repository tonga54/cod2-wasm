#!/usr/bin/env python3
"""Muzzle tails trail their own motion in the muzzle's axes at every view angle."""
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


def body_after(text, marker):
    start = text.index(marker) + len(marker)
    return text[start:text.rindex('}')]


math = (function((root / 'src/PC/universal/q_shared.c').read_text(), 'OrientationPosToWorldPos') +
        function((root / 'src/PC/universal/q_shared.c').read_text(), 'OrientationDirFromWorldDir') +
        function((root / 'src/PC/universal/com_math.c').read_text(), 'AxisTransformVector')
        .replace('void AxisTransformVector(', 'static void AxisTransformVector_axes('))
# Only the frame space of the velocity: its curves are evaluated before this.
velocity = body_after(function(fx, 'Particle_CalcVelocityValue'), '\n    }\n\n')
update = function(fx, 'Tail_Update')
bolt_end = update.index('\n    }\n\n', update.index('orient = (&((FxBoltFrame'))
step = update[bolt_end + len('\n    }\n'):update.index('    float radius;')]

support = r'''
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char byte; typedef int Bool; typedef float vec_t; typedef float vec3_t[3];
typedef struct {vec3_t origin, axis[3];} orientation_t;
typedef struct {union {const void *master; byte bytes[8];} curveIterator; float scale;} FxChannelInstance;
/* The anchor and flag bytes sit at their retail offsets, which the tail update reads directly. */
typedef struct {
 int vtable; float origin[3]; struct {float origin[3], endpos[3];} mRefEnt; byte pad[0xa8 - 40]; int mFlags;
} Effect;
typedef struct {
 struct {Effect base;} base; float endpoint[3], tailLength, lengthBlendFactor;
 FxChannelInstance lengthChannelInstance, lengthRandChannelInstance;
} Tail;
typedef struct {byte head[0xb0]; float displayAxis[3][3];} Particle;
_Static_assert(offsetof(Effect, origin) == 4 && offsetof(Effect, mFlags) == 0xa8, "retail effect offsets");
#define PART_ANCHOR_X(p) (((Effect *)(p))->origin[0])
#define PART_ANCHOR_Y(p) (((Effect *)(p))->origin[1])
#define PART_ANCHOR_Z(p) (((Effect *)(p))->origin[2])
static float EvalCurve1(const char *channel, float normTime) {(void)channel; (void)normTime; return 1.0f;}
'''

harness = r'''
static void AxisTransformVector(void *axes, float x, float y, float z, vec_t *out) {AxisTransformVector_axes((vec3_t *)axes, x, y, z, out);}
static void VelocityInFrame(const byte *self, float vx, float vy, float vz, vec_t *outVector, const orientation_t *or_) {
''' + velocity + r'''
}
static Particle motion; static float authored[3]; static const float frameSeconds = 0.016f;
/* Particle_UpdateOrigin's step without traces, velocity2 or gravity, which are zero here. */
static Bool Particle_UpdateOrigin(const Particle *particle, const orientation_t *or_) {
 Effect *effect = (Effect *)particle; vec3_t v;
 VelocityInFrame((const byte *)&motion, authored[0], authored[1], authored[2], v, or_);
 for (int i = 0; i < 3; i++) effect->origin[i] += v[i] * frameSeconds;
 return 1;
}
''' + function(fx, 'Tail_CalcNewEndpoint') + r'''
static Bool Step(const Tail *_this, void *orient, float normTime) {
 byte *self = (byte *)_this;
''' + step + r'''
 return 1;
}
'''

checks = r'''
static int tested, orientations;
static void basis(float yaw, float pitch, float roll, vec3_t axis[3]) {
 float y = yaw * 0.01745329252f, p = pitch * 0.01745329252f, r = roll * 0.01745329252f;
 float cy = cosf(y), sy = sinf(y), cp = cosf(p), sp = sinf(p), cr = cosf(r), sr = sinf(r);
 float forward[3] = {cp * cy, cp * sy, -sp}, left[3] = {-sy, cy, 0}, up[3] = {sp * cy, sp * sy, cp};
 for (int i = 0; i < 3; i++) {
  axis[0][i] = forward[i];
  axis[1][i] = left[i] * cr + up[i] * sr;
  axis[2][i] = up[i] * cr - left[i] * sr;
 }
}
/* A tail as FX_AddTail spawns it: a bolted anchor in the bolt's axes, and the
 * provisional end one unit back along the effect's world axis. */
static void spawn(Tail *tail, const vec3_t anchor, vec3_t axis[3], int flags) {
 memset(tail, 0, sizeof(*tail));
 memset(&motion, 0, sizeof(motion));
 memcpy(tail->base.base.origin, anchor, sizeof(vec3_t));
 for (int i = 0; i < 3; i++) tail->endpoint[i] = anchor[i] - axis[0][i];
 tail->base.base.mFlags = flags;
 memcpy(motion.head + 0xa8, &flags, sizeof(flags));
 memcpy(motion.displayAxis, axis, sizeof(motion.displayAxis));
 tail->lengthChannelInstance.scale = 40.0f;
}
static void fly(Tail *tail, const orientation_t *bolt, const float velocity[3], const float expected[3]) {
 memcpy(authored, velocity, sizeof(authored));
 for (int frame = 1; frame <= 6; frame++) {
  assert(Step(tail, (void *)bolt, frame / 6.0f));
  const float *start = tail->base.base.mRefEnt.origin, *end = tail->base.base.mRefEnt.endpos;
  for (int i = 0; i < 3; i++) {
   float got = end[i] - start[i];
   if (fabsf(got - expected[i] * 40.0f) > 0.02f) {
    fprintf(stderr, "frame %d axis %d: tail %g, expected %g\n", frame, i, got, expected[i] * 40.0f);
    assert(!"tail direction");
   }
  }
  tested++;
 }
}
int main(void) {
 for (int yaw = -180; yaw <= 180; yaw += 15)
 for (int pitch = -60; pitch <= 60; pitch += 30)
 for (int roll = -30; roll <= 30; roll += 30) {
  orientation_t bolt = {{1300.0f + yaw, -2500.0f + pitch, 270.0f + roll}};
  basis((float)yaw, (float)pitch, (float)roll, bolt.axis);
  Tail tail;
  orientations++;
  /* rifleflash's center_line: it drifts back into the barrel, so it streaks forward. */
  const vec3_t barrel = {-5, 0, 0};
  spawn(&tail, barrel, bolt.axis, 0);
  fly(&tail, &bolt, (const float[3]){-23, 0, 0}, bolt.axis[0]);
  vec3_t world;
  OrientationPosToWorldPos(&bolt, tail.base.base.origin, world);
  for (int i = 0; i < 3; i++) assert(fabsf(tail.base.base.mRefEnt.origin[i] - world[i]) < 0.001f);
  /* mg42hv_view's star: barely moving rays point away from their slow drift. */
  spawn(&tail, (const float[3]){0, 0, 0}, bolt.axis, 0);
  fly(&tail, &bolt, (const float[3]){0, -1, 0}, bolt.axis[1]);
  spawn(&tail, (const float[3]){0, 0, 0}, bolt.axis, 0);
  fly(&tail, &bolt, (const float[3]){0, 0, -2}, bolt.axis[2]);
  /* An absolute velocity is in world axes even when bolted: rising, it trails down. */
  spawn(&tail, barrel, bolt.axis, 0x200000);
  fly(&tail, &bolt, (const float[3]){0, 0, 50}, (const float[3]){0, 0, -1});
  /* Unbolted tails move in their spawn axes, in world space. */
  spawn(&tail, bolt.origin, bolt.axis, 0);
  fly(&tail, NULL, (const float[3]){-23, 0, 0}, bolt.axis[0]);
 }
 printf("PASS: %d tail frames at %d muzzle orientations: bolted, radial, absolute and unbolted motion\n", tested, orientations);
}
'''


def run(code, directory):
    path = Path(directory)
    (path / 'test.c').write_text(code)
    subprocess.run(['cc', '-std=c11', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    return subprocess.run([str(path / 'test')], capture_output=True, text=True)


with tempfile.TemporaryDirectory(prefix='cod2-tail-motion-') as directory:
    code = support + math + harness + checks
    result = run(code, directory)
    assert result.returncode == 0, result.stdout + result.stderr
    print(result.stdout, end='')
    trail = re.search(r'\n    /\* The tail trails[^\n]*\n[^\n]*\n(?:    \(\(\(Tail \*\)\(self\)\)->endpoint\[\d\]\) = [^\n]*\n){3}', code)
    assert trail, 'expected the tail to record its anchor before moving'
    assert run(code.replace(trail.group(0), '\n'), directory).returncode != 0, \
        'a tail that keeps its spawn direction must be rejected'
    world_velocity = ('    if (!(*(byte *)(self + 0xaa) & 0x20)) {\n'
                      '        AxisTransformVector(((char *)self + offsetof(Particle, displayAxis[0][0])), vx, vy, vz, outVector);\n'
                      '    } else {\n        outVector[0] = vx;\n        outVector[1] = vy;\n        outVector[2] = vz;\n    }\n')
    assert run(code.replace(velocity, world_velocity), directory).returncode != 0, \
        'bolted velocities in world axes must be rejected'
    print('PASS: spawn-direction tails and world-axis bolted velocities rejected')
