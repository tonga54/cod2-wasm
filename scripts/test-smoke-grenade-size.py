#!/usr/bin/env python3
"""A smoke grenade's cloud spreads 60% as far as CoD2's. FX_TryRegisterEffect, compiled here as it is with
FX_ShrinkSmokeGrenade, shrinks explosions/smoke_grenade the first time it parses it: each particle's spawn offset,
ring radius and height, gravity, size, length and velocity scales and frustum radius by 0.6, and no other field.
The effect's FxRunner, every other effect and the effect's length, which the server gives the smoke entity, are
unchanged, and registering it again returns the template as it is."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
scheduler = (root / 'src/PC/EffectsCore/FxScheduler.c').read_text()


def function(text, head):
    assert text.count(head) == 1, head
    start = text.index(head)
    depth, end = 1, text.index('{', start) + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


def code(text):
    scale = re.search(r'^#define FX_SMOKE_GRENADE_SCALE .*\n', text, re.M)
    assert scale
    return (scale.group(0) + function(text, 'static void FxRange_Scale(') +
            function(text, 'static void FX_ShrinkSmokeGrenade(') +
            function(text, 'float FxScheduler_GetEffectLength(const FxScheduler *_this, EffectTemplate *fx)\n{') +
            function(text, 'EffectTemplate *FX_TryRegisterEffect(const char *name)\n{'))


support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
enum PrimType { PT_NONE, PT_PARTICLE, PT_LINE, PT_TAIL, PT_CYLINDER, PT_EMITTER, PT_DECAL, PT_ORIENTEDPARTICLE,
                PT_FXRUNNER, PT_LIGHT, PT_CAMERASHAKE, PT_SCREENFLASH, PT_CLOUD };
enum { FXCHAN_COLOR, FXCHAN_COLOR_RAND, FXCHAN_ALPHA, FXCHAN_ALPHA_RAND, FXCHAN_SIZE, FXCHAN_SIZE_RAND, FXCHAN_SIZE2,
       FXCHAN_SIZE2_RAND, FXCHAN_LENGTH, FXCHAN_LENGTH_RAND, FXCHAN_ROTATION_DELTA, FXCHAN_ROTATION_DELTA_RAND,
       FXCHAN_VELOCITY_X, FXCHAN_VELOCITY_Y, FXCHAN_VELOCITY_Z, FXCHAN_VELOCITY_X_RAND, FXCHAN_VELOCITY_Y_RAND,
       FXCHAN_VELOCITY_Z_RAND, FXCHAN_VELOCITY2_X, FXCHAN_VELOCITY2_Y, FXCHAN_VELOCITY2_Z, FXCHAN_VELOCITY2_X_RAND,
       FXCHAN_VELOCITY2_Y_RAND, FXCHAN_VELOCITY2_Z_RAND, FXCHAN_COUNT };
typedef struct { float mMin, mMax; } FxRange;
typedef struct { const void *curve; FxRange scaleRange; } FxChannel;
/* Every range a primitive template has. */
typedef struct {
    char mName[32];
    int mType;
    FxRange mSpawnDelay, mSpawnCount, mLife, mSpawnRange;
    FxRange mOrigin1X, mOrigin1Y, mOrigin1Z, mOrigin2X, mOrigin2Y, mOrigin2Z, mRadius, mHeight, mWindModifier;
    FxChannel mFxChannels[24];
    FxRange mRotation, mAngle1, mAngle2, mAngle3, mAngle1Delta, mAngle2Delta, mAngle3Delta, mGravity, mDensity,
        mVariance, mTexCoordS, mTexCoordT, mElasticity;
    float spawnFrustumCullRadius;
} PrimitiveTemplate;
typedef struct { const char *mEffectName; int mPrimitiveCount; PrimitiveTemplate *mPrimitives[24]; } EffectTemplate;
typedef struct { int unused; } FxScheduler;
typedef struct { int open; } GenericParser2;
static EffectTemplate *effectTemplateArray[256];
static int effectTemplateArrayCount;
/* What the next parse makes of its file; parses counts them. */
static EffectTemplate *nextParse;
static int parses, parsersOpen;
static void GenericParser2_GenericParser2(GenericParser2 *parser) { parser->open = 1; parsersOpen++; }
static void ZN14GenericParser2D1Ev(GenericParser2 *parser) { assert(parser->open); parser->open = 0; parsersOpen--; }
static EffectTemplate *FX_ParseEffect(GenericParser2 *parser, const char *name)
{
    assert(parser->open);
    parses++;
    if (nextParse)
        nextParse->mEffectName = name;
    return nextParse;
}
static void FX_Print(const char *format, ...) { (void)format; }
'''

checks = r'''
enum { SPAWN_DELAY, SPAWN_COUNT, LIFE, SPAWN_RANGE, ORIGIN1X, ORIGIN1Y, ORIGIN1Z, ORIGIN2X, ORIGIN2Y, ORIGIN2Z, RADIUS,
       HEIGHT, WIND, ROTATION, ANGLE1, ANGLE2, ANGLE3, ANGLE1_DELTA, ANGLE2_DELTA, ANGLE3_DELTA, GRAVITY, DENSITY,
       VARIANCE, TEXCOORD_S, TEXCOORD_T, ELASTICITY, CHANNEL0, RANGES = CHANNEL0 + FXCHAN_COUNT };

static FxRange *range(PrimitiveTemplate *p, int which)
{
    FxRange *fields[CHANNEL0] = { &p->mSpawnDelay, &p->mSpawnCount, &p->mLife, &p->mSpawnRange, &p->mOrigin1X,
        &p->mOrigin1Y, &p->mOrigin1Z, &p->mOrigin2X, &p->mOrigin2Y, &p->mOrigin2Z, &p->mRadius, &p->mHeight,
        &p->mWindModifier, &p->mRotation, &p->mAngle1, &p->mAngle2, &p->mAngle3, &p->mAngle1Delta, &p->mAngle2Delta,
        &p->mAngle3Delta, &p->mGravity, &p->mDensity, &p->mVariance, &p->mTexCoordS, &p->mTexCoordT, &p->mElasticity };
    return which < CHANNEL0 ? fields[which] : &p->mFxChannels[which - CHANNEL0].scaleRange;
}

/* Where the cloud is and how it moves and grows: its spawn offset, ring and height, gravity, sizes, lengths and
 * velocities. Not its timing, count, colour, alpha, spin, angles or second origin. */
static int spatial(int which)
{
    int channel = which - CHANNEL0;
    if (which >= CHANNEL0)
        return (channel >= FXCHAN_SIZE && channel <= FXCHAN_LENGTH_RAND) || channel >= FXCHAN_VELOCITY_X;
    return which == ORIGIN1X || which == ORIGIN1Y || which == ORIGIN1Z || which == RADIUS || which == HEIGHT ||
           which == GRAVITY;
}

static int particle(int type) { return type == PT_PARTICLE || type == PT_ORIENTEDPARTICLE || type == PT_CLOUD; }

/* Effects, each 24 primitives of every type in turn with every value its own. */
static PrimitiveTemplate store[8][24], original[8][24];
static EffectTemplate effects[8];

static EffectTemplate *build(int index)
{
    int i, which;
    memset(store[index], 0, sizeof(store[index]));
    memset(&effects[index], 0, sizeof(effects[index]));
    for (i = 0; i < 24; i++) {
        PrimitiveTemplate *p = &store[index][i];
        p->mType = i % 13;
        snprintf(p->mName, sizeof(p->mName), "p%d", i);
        for (which = 0; which < RANGES; which++) {
            range(p, which)->mMin = 1.0f + (float)(index * 3000 + i * 100 + which);
            range(p, which)->mMax = 1.5f + (float)(index * 3000 + i * 100 + which);
        }
        for (which = 0; which < FXCHAN_COUNT; which++)
            p->mFxChannels[which].curve = &original[index][i];
        p->spawnFrustumCullRadius = 30.0f + (float)i;
        effects[index].mPrimitives[i] = p;
    }
    effects[index].mPrimitiveCount = 24;
    memcpy(original[index], store[index], sizeof(original[index]));
    nextParse = &effects[index];
    return &effects[index];
}

static int same(float value, float expected) { return fabsf(value - expected) <= 1e-6f * fabsf(expected); }

/* Effect index's primitives: its particles' spatial ranges and frustum radius times factor, all else as built. */
static void expect(int index, float factor)
{
    int i, which;
    for (i = 0; i < 24; i++) {
        PrimitiveTemplate *now = &store[index][i], *was = &original[index][i];
        float f = particle(was->mType) ? factor : 1.0f;
        assert(now->mType == was->mType && !strcmp(now->mName, was->mName));
        for (which = 0; which < RANGES; which++) {
            float g = spatial(which) ? f : 1.0f;
            assert(same(range(now, which)->mMin, range(was, which)->mMin * g));
            assert(same(range(now, which)->mMax, range(was, which)->mMax * g));
        }
        for (which = 0; which < FXCHAN_COUNT; which++)
            assert(now->mFxChannels[which].curve == was->mFxChannels[which].curve);
        assert(same(now->spawnFrustumCullRadius, was->spawnFrustumCullRadius * f));
        if (f == 1.0f)
            assert(!memcmp(now, was, sizeof(*now)));
    }
}

int main(void)
{
    static const char *const others[] = { "explosions/smoke_grenade_night", "explosions/grenade_flash",
                                          "props/american_smoke_grenade", "explosions/smoke_grenad" };
    EffectTemplate *smoke, wasSmoke;
    int i, cases = 0;

    /* The first registration parses the smoke grenade's cloud and shrinks its particles to 0.6. */
    smoke = build(0);
    assert(FX_TryRegisterEffect("explosions/smoke_grenade") == smoke && parses == 1 && effectTemplateArrayCount == 1);
    expect(0, 0.6f);
    cases++;
    /* Its length, the latest a primitive's delay and life end, is CoD2's. */
    wasSmoke = *smoke;
    for (i = 0; i < 24; i++)
        wasSmoke.mPrimitives[i] = &original[0][i];
    assert(FxScheduler_GetEffectLength(NULL, smoke) == FxScheduler_GetEffectLength(NULL, &wasSmoke));
    assert(FxScheduler_GetEffectLength(NULL, smoke) > 0.0f);
    cases++;
    /* Registered again, as the weapon, the burnt canister's runner and the killcam do, it is the same template,
     * neither parsed nor shrunk again. */
    nextParse = NULL;
    for (i = 0; i < 3; i++)
        assert(FX_TryRegisterEffect("explosions/smoke_grenade") == smoke);
    assert(parses == 1 && effectTemplateArrayCount == 1);
    expect(0, 0.6f);
    cases++;
    /* Every other effect, the night smoke and the grenade's own runner among them, loads as it is. */
    for (i = 0; i < 4; i++) {
        EffectTemplate *other = build(1 + i);
        assert(FX_TryRegisterEffect(others[i]) == other && parses == 2 + i);
        expect(1 + i, 1.0f);
        cases++;
    }
    expect(0, 0.6f);
    /* A file that fails to parse registers nothing. */
    nextParse = NULL;
    assert(!FX_TryRegisterEffect("explosions/missing") && effectTemplateArrayCount == 5 && parsersOpen == 0);
    cases++;
    printf("%d\n", cases);
    return 0;
}
'''

mutants = [
    ('    FX_ShrinkSmokeGrenade(fx);\n', ''),
    ('            ZN14GenericParser2D1Ev(&parser);\n            return registeredTemplate;',
     '            ZN14GenericParser2D1Ev(&parser);\n            FX_ShrinkSmokeGrenade(registeredTemplate);\n'
     '            return registeredTemplate;'),
    ('    if (strcmp(fx->mEffectName, "explosions/smoke_grenade"))',
     '    if (strncmp(fx->mEffectName, "explosions/smoke_grenade", 24))'),
    ('        if (prim->mType != PT_PARTICLE && prim->mType != PT_ORIENTEDPARTICLE && prim->mType != PT_CLOUD)\n'
     '            continue;\n', ''),
    ('prim->mType != PT_ORIENTEDPARTICLE && ', ''),
    ('        FxRange_Scale(&prim->mOrigin1X, FX_SMOKE_GRENADE_SCALE);\n', ''),
    ('        FxRange_Scale(&prim->mOrigin1Z, FX_SMOKE_GRENADE_SCALE);\n', ''),
    ('        FxRange_Scale(&prim->mRadius, FX_SMOKE_GRENADE_SCALE);\n', ''),
    ('        FxRange_Scale(&prim->mHeight, FX_SMOKE_GRENADE_SCALE);\n', ''),
    ('        FxRange_Scale(&prim->mGravity, FX_SMOKE_GRENADE_SCALE);\n', ''),
    ('channel = FXCHAN_SIZE; channel <= FXCHAN_LENGTH_RAND;', 'channel = FXCHAN_SIZE; channel < FXCHAN_LENGTH_RAND;'),
    ('channel = FXCHAN_SIZE; channel <= FXCHAN_LENGTH_RAND;', 'channel = FXCHAN_ALPHA_RAND; channel <= FXCHAN_LENGTH_RAND;'),
    ('channel = FXCHAN_VELOCITY_X; channel < FXCHAN_COUNT;', 'channel = FXCHAN_VELOCITY_X; channel < FXCHAN_VELOCITY2_X;'),
    ('channel = FXCHAN_VELOCITY_X; channel < FXCHAN_COUNT;', 'channel = FXCHAN_ROTATION_DELTA; channel < FXCHAN_COUNT;'),
    ('        prim->spawnFrustumCullRadius *= FX_SMOKE_GRENADE_SCALE;\n', ''),
    ('    range->mMax *= scale;\n', ''),
    ('#define FX_SMOKE_GRENADE_SCALE 0.6f', '#define FX_SMOKE_GRENADE_SCALE 0.5f'),
]

with tempfile.TemporaryDirectory(prefix='cod2-smoke-grenade-size-') as directory:
    path, binary = Path(directory) / 'smoke.c', Path(directory) / 'smoke'
    for index, (old, new) in enumerate([(None, None)] + mutants):
        source = scheduler
        if old is not None:
            assert source.count(old) == 1, old
            source = source.replace(old, new)
        path.write_text(support + code(source) + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wall', '-Werror' if index == 0 else '-w', str(path), '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        if index == 0:
            assert result.returncode == 0, result.stderr[-3000:]
            cases = int(result.stdout)
        else:
            assert result.returncode != 0, f'mutant survived: {old!r} -> {new!r}'

# Effects register through FX_TryRegisterEffect alone, so the cloud is shrunk wherever it is played from.
register = function(scheduler, 'EffectTemplate *FX_TryRegisterEffect(const char *name)\n{')
assert register.index('    fx = FX_ParseEffect(&parser, name);\n') < register.index('    FX_ShrinkSmokeGrenade(fx);\n') < \
    register.index('    effectTemplateArray[effectTemplateArrayCount] = fx;\n')
print(f'PASS: {cases} cases: the smoke grenade\'s cloud registers once with its particles\' spawn ring, offset, '
      f'height, gravity, sizes, velocities and frustum radius at 0.6, its runner, timing and length unchanged, and '
      f'every other effect as it is; all {len(mutants)} mutants fail')
