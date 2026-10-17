#!/usr/bin/env python3
"""Exercise the real emitter model spin, scale and view culling under ASan/UBSan."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
primitives = (root / 'src/PC/EffectsCore/FxPrimitives.c').read_text()
util = (root / 'src/PC/EffectsCore/FxUtil.c').read_text()
system = (root / 'src/PC/EffectsCore/FxSystem.c').read_text()
math_source = (root / 'src/PC/universal/com_math.c').read_text()
header = (root / 'src/headers/PC/EffectsCore/fx_public.h').read_text()

def function(source, name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'

# FX_AddEmitter stores the spawn angles and their deltas where the update
# reads them, and Emitter_Update feeds it the size and whether the model moved.
for store in ('((Emitter *)p)->spawnSize = ang0;', '(*(float *)&((Emitter *)p)->_gap2[0]) = ang1;',
              '(*(float *)&((Emitter *)p)->_gap2[4]) = ang2;', '((Emitter *)p)->spawnDensity = pos0;',
              '(*(float *)&((Emitter *)p)->_gap3[0]) = pos1;', '((Emitter *)p)->spawnStep = pos2;'):
    assert store in util, store
update = function(primitives, 'Emitter_Update')
saved = update.index('memcpy(lastOrigin, ((Effect *)self)->mRefEnt.origin, sizeof(lastOrigin));')
moved = update.index('Emitter_UpdateEmitFx(')
spun = update.index('Emitter_UpdateModel(self, radius, memcmp(lastOrigin, ((Effect *)self)->mRefEnt.origin, sizeof(lastOrigin)) != 0);')
assert update.index('Particle_UpdateOrigin(') < saved < moved < update.index('if (radius == 0.0f)') < spun
assert 'mRefEnt.scale) == 0.0f)\n        return;' in function(primitives, 'Emitter_Draw')

emitter = re.search(r'struct Emitter \{.*?\n\};', header, re.S).group(0)
model = function(primitives, 'Emitter_UpdateModel')
cull = function(primitives, 'Emitter_Cull')
support = r'''
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char byte; typedef unsigned char Bool; typedef float vec_t; typedef vec_t vec3_t[3]; typedef vec_t vec4_t[4];
struct XModel { vec3_t mins, maxs; };
typedef struct { vec3_t axis[3]; float scale; vec3_t origin; } FxGfxEntity;
/* Smaller than 0xab, so the flag byte Emitter_Cull reads stays clear. */
typedef struct { int vptr; vec3_t origin; FxGfxEntity mRefEnt; struct XModel *mModelPtr; } Effect;
typedef struct Emitter Emitter;
''' + emitter + r'''
_Static_assert(offsetof(Emitter, spawnSize) == 0x278, "angles");
_Static_assert(offsetof(Emitter, spawnDensity) == 0x284, "angle deltas");
_Static_assert(offsetof(Emitter, spawnStep) == 0x28c, "third angle delta");
_Static_assert(sizeof(Effect) < 0xab, "flag byte");
typedef struct { int mFrameTime; struct { vec3_t vieworg; vec4_t frustum[6]; int numPlanes; } mCamera; } FxHelper;
static FxHelper helper, *helperPtr = &helper;
static void *imp_theFxHelper = &helperPtr;
#define FX_HELPER (*(FxHelper **)imp_theFxHelper)
''' + function(math_source, 'AngleVectors') + function(math_source, 'AnglesToAxis') + function(system, 'FxHelper_CullSphere')
checks = r'''
static _Alignas(16) byte storage[sizeof(Emitter)];
static float *angles(void) { return (float *)(storage + offsetof(Emitter, spawnSize)); }
static float *deltas(void) { return (float *)(storage + offsetof(Emitter, spawnDensity)); }
static Effect *fx(void) { return (Effect *)storage; }
static void reset(float a0, float a1, float a2, float d0, float d1, float d2) {
    memset(storage, 0, sizeof(storage));
    angles()[0] = a0; angles()[1] = a1; angles()[2] = a2;
    deltas()[0] = d0; deltas()[1] = d1; deltas()[2] = d2;
}
static void step(int ms, Bool moved, float size) { helper.mFrameTime = ms; Emitter_UpdateModel(storage, size, moved); }
static void near(float actual, float expected, float tolerance) { assert(fabsf(actual - expected) <= tolerance); }
static float dot(const float *a, const float *b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
static void axis_follows_angles(void) {
    vec3_t expected[3], *axis = fx()->mRefEnt.axis;
    AnglesToAxis(angles(), expected);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) near(axis[i][j], expected[i][j], 1e-6f);
    for (int i = 0; i < 3; i++) {
        near(dot(axis[i], axis[i]), 1, 1e-5f);
        near(dot(axis[i], axis[(i + 1) % 3]), 0, 1e-5f);
    }
    /* Forward, left, up: a right-handed frame, never mirrored. */
    float det = axis[0][0] * (axis[1][1] * axis[2][2] - axis[1][2] * axis[2][1])
              - axis[0][1] * (axis[1][0] * axis[2][2] - axis[1][2] * axis[2][0])
              + axis[0][2] * (axis[1][0] * axis[2][1] - axis[1][1] * axis[2][0]);
    near(det, 1, 1e-5f);
}
int main(void) {
    /* The model draws at the size curve's scale, facing its spawn angles. */
    reset(0, 180, 0, 0, 0, 0);
    step(16, 1, 1.0f);
    assert(fx()->mRefEnt.scale == 1.0f);
    axis_follows_angles();
    near(fx()->mRefEnt.axis[0][0], -1, 1e-6f);
    step(16, 1, 0.5f);
    assert(fx()->mRefEnt.scale == 0.5f);

    /* In flight the angles advance by the delta in degrees per 100 ms. */
    reset(10, 180, -5, 233, -233, 100);
    step(16, 1, 1.0f);
    near(angles()[0], 10 + 233 * 0.16f, 1e-4f);
    near(angles()[1], 180 - 233 * 0.16f, 1e-4f);
    near(angles()[2], -5 + 16, 1e-4f);
    near(deltas()[0], 233, 0); near(deltas()[1], -233, 0); near(deltas()[2], 100, 0);
    axis_follows_angles();

    /* A casing tumbles visibly within a few frames. */
    reset(0, 180, 0, 233, 233, 233);
    step(16, 1, 1.0f);
    float forward[3]; memcpy(forward, fx()->mRefEnt.axis[0], sizeof(forward));
    for (int frame = 0; frame < 30; frame++) {
        step(16, 1, 1.0f);
        axis_follows_angles();
    }
    assert(dot(forward, fx()->mRefEnt.axis[0]) < 0.9f);

    /* At rest the spin dies down like 0.7 per 16 ms frame, then stops. */
    reset(5, 90, 0, 200, -150, 100);
    step(16, 0, 1.0f);
    near(deltas()[0], 140, 1e-3f); near(deltas()[1], -105, 1e-3f); near(deltas()[2], 70, 1e-3f);
    near(angles()[0], 5 + 140 * 0.16f, 1e-3f);
    near(angles()[1], 90 - 105 * 0.16f, 1e-3f);
    axis_follows_angles();
    for (int frame = 0; frame < 60; frame++) step(16, 0, 1.0f);
    float settled[3]; memcpy(settled, angles(), sizeof(settled));
    for (int frame = 0; frame < 60; frame++) step(16, 0, 1.0f);
    for (int i = 0; i < 3; i++) near(angles()[i], settled[i], 1e-4f);
    axis_follows_angles();

    /* The decay is per millisecond, so frame rate does not change it. */
    reset(0, 0, 0, 200, 200, 200);
    step(8, 0, 1.0f); step(8, 0, 1.0f);
    float halfFrames = deltas()[0];
    reset(0, 0, 0, 200, 200, 200);
    step(16, 0, 1.0f);
    near(halfFrames, deltas()[0], 1e-3f);

    /* fx_freeze runs updates with no elapsed time: the casing holds still
     * but keeps drawing. */
    reset(30, 60, 90, 233, -233, 233);
    step(0, 0, 1.0f);
    assert(angles()[0] == 30 && angles()[1] == 60 && angles()[2] == 90);
    assert(deltas()[0] == 233 && deltas()[1] == -233 && deltas()[2] == 233);
    assert(fx()->mRefEnt.scale == 1.0f);
    axis_follows_angles();

    /* The view looks down +x through a 90 degree frustum, near plane at the eye. */
    const float h = sqrtf(0.5f);
    const vec4_t planes[5] = {{1, 0, 0, 0}, {h, h, 0, 0}, {h, -h, 0, 0}, {h, 0, h, 0}, {h, 0, -h, 0}};
    memcpy(helper.mCamera.frustum, planes, sizeof(planes));
    helper.mCamera.numPlanes = 5;
    /* Extents 0.6, 0.3, 0.4 from the model origin: a 0.781 sphere. */
    struct XModel shell = {{-0.2f, -0.3f, -0.4f}, {0.6f, 0.3f, 0.2f}}, rifle = {{-1, -1, -1}, {8, 1, 1}};
    struct { struct XModel *model; float scale, x, y, z; Bool culled; } cases[] = {
        {&shell, 1, 10, 0, 0, 0},       /* ahead */
        {&shell, 1, -10, 0, 0, 1},      /* behind */
        {&shell, 1, -0.7f, 0, 0, 0},    /* straddles the near plane */
        {&shell, 0.5f, -0.7f, 0, 0, 1}, /* at half size it is wholly behind */
        {&shell, 1, 5, 5.9f, 0, 0},     /* straddles the left plane */
        {&shell, 1, 5, 6.2f, 0, 1},     /* wholly left of it */
        {&shell, 1, 5, 0, -6.2f, 1},    /* wholly below the view */
        {&rifle, 1, -7, 0, 0, 0},       /* the long model's far end reaches the view */
        {0, 1, -10, 0, 0, 0},           /* without a model there is nothing to cull */
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        reset(0, 0, 0, 0, 0, 0);
        fx()->mModelPtr = cases[i].model;
        fx()->mRefEnt.scale = cases[i].scale;
        fx()->mRefEnt.origin[0] = cases[i].x; fx()->mRefEnt.origin[1] = cases[i].y; fx()->mRefEnt.origin[2] = cases[i].z;
        assert(Emitter_Cull((const Emitter *)storage) == cases[i].culled);
    }
    puts("PASS");
    return 0;
}
'''
radius = 'sqrtf(radiusSq) * (((Effect *)(p))->mRefEnt.scale)'
mutations = {
    'current': (model, cull),
    'no_scale': (model.replace('((Effect *)self)->mRefEnt.scale = size;', ''), cull),
    'no_spin': (model.replace('angles[i] += angleDelta[i] * frameTime * 0.01f;', '(void)0;'), cull),
    'no_axis': (model.replace('AnglesToAxis(angles, ((Effect *)self)->mRefEnt.axis);', ''), cull),
    'no_rest_decay': (model.replace('moved ? 1.0f : powf(0.7f, frameTime / 16.0f)', '1.0f'), cull),
    'per_frame_decay': (model.replace('powf(0.7f, frameTime / 16.0f)', '0.7f'), cull),
    'never_cull': (model, cull.replace('if (!model)\n        return 0;', 'return 0;')),
    'cull_origin_only': (model, cull.replace(radius, '0.0f')),
    'cull_ignores_scale': (model, cull.replace(radius, 'sqrtf(radiusSq)')),
    'cull_min_corner': (model, cull.replace('fmaxf(fabsf(model->mins[i]), fabsf(model->maxs[i]))', 'fabsf(model->mins[i])')),
}
for name, actual in mutations.items():
    assert name == 'current' or actual != (model, cull), name
with tempfile.TemporaryDirectory(prefix='cod2-fx-emitter-model-') as directory:
    path = Path(directory)
    for name, (update_source, cull_source) in mutations.items():
        (path / 'test.c').write_text(support + update_source + cull_source + checks)
        subprocess.run(['cc', '-std=c11', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True)
        assert (result.returncode == 0) == (name == 'current'), (name, result.stderr.decode())
print('PASS: emitter models draw at their size, tumble in flight, settle at rest, hold under fx_freeze '
      'and cull by their bounds; all nine regressions detected')
