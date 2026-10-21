#!/usr/bin/env python3
"""Exercise the browser's static model queue (src/PC/gfx_d3d/r_dpvs.c): culling
queues static models, and after the effects they fill the scene's slots left,
the largest on screen first, so a map with grass by the thousand keeps its
muzzle flashes, explosions and nearest grass."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
dpvs = (root / 'src/PC/gfx_d3d/r_dpvs.c').read_text()
scene = (root / 'src/PC/gfx_d3d/r_scene.c').read_text()
BEGIN = '#ifdef __EMSCRIPTEN__\n/* A scene holds 2,046 entities'


def queue_block(source):
    start = source.index(BEGIN)
    return source[start:source.index('#endif\n', start) + len('#endif\n')]


def function(source, name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]


def check_sources(dpvs, scene):
    # Both culling paths queue in the browser and keep the original immediate add elsewhere.
    for name in ('R_AddStaticModelWithCull_impl', 'R_AddStaticModelDirect'):
        body = function(dpvs, name)
        browser = body[body.index('#ifdef __EMSCRIPTEN__'):body.index('#else')]
        native = body[body.index('#else'):body.index('#endif')]
        assert 'R_QueueStaticModel(smodelIndex);' in browser and 'R_AddStaticModelToScene' not in browser, name
        assert 'R_AddStaticModelToScene(smodelIndex);' in native, name
        assert body.index('smodelDync[0] = viewCount;') < body.index('R_QueueStaticModel(smodelIndex);'), name
    # Culling opens the queue before it walks a cell.
    cull = function(dpvs, 'R_AddWorldSurfacesDpvs_impl')
    assert cull.index('R_BeginStaticModelQueue();') < cull.index('R_FrustumClipPlanes(') < \
        cull.index('R_AddVisibleSurfacesInCell_impl(&cells[i]') < cull.index('R_VisitPortals(cameraCell')
    # The scene adds the queue once the world, the marks and the effects are in, before anything reads its models.
    render = function(scene, 'R_RenderScene')
    assert render.index('R_AddWorldSurfacesDpvs(viewParmsDraw, cellIdx);') < render.index('FX_DrawScheduledEffects();') < \
        render.index('    R_AddQueuedStaticModels();') < render.index('WebPointLights_SetScene(') < \
        render.index('WebSunShadows_EndCapture(') < render.index('qsortDrawSurfs(')
    at = render.index('    R_AddQueuedStaticModels();')
    assert render.rfind('#ifdef __EMSCRIPTEN__', 0, at) > render.rfind('#endif', 0, at)
    assert dpvs.count('R_QueueStaticModel(smodelIndex);') == 2 and scene.count('R_AddQueuedStaticModels();') == 1


check_sources(dpvs, scene)
source_mutants = [
    ('dpvs', '#ifdef __EMSCRIPTEN__\n    R_QueueStaticModel(smodelIndex);\n#else\n    int entIndex',
     '#ifdef __EMSCRIPTEN__\n    R_AddStaticModelSurfaces(smodelIndex);\n#else\n    int entIndex'),
    ('dpvs', '#ifdef __EMSCRIPTEN__\n    R_BeginStaticModelQueue();\n#endif\n', ''),
    ('scene', '    CG_AddMarks();\n    FX_DrawScheduledEffects();\n\n#ifdef __EMSCRIPTEN__\n'
     '    extern void R_AddQueuedStaticModels(void);\n    R_AddQueuedStaticModels();\n',
     '#ifdef __EMSCRIPTEN__\n    extern void R_AddQueuedStaticModels(void);\n    R_AddQueuedStaticModels();\n#endif\n'
     '    CG_AddMarks();\n    FX_DrawScheduledEffects();\n\n#ifdef __EMSCRIPTEN__\n'),
]
for which, before, after in source_mutants:
    sources = {'dpvs': dpvs, 'scene': scene}
    assert sources[which].count(before) >= 1, before
    sources[which] = sources[which].replace(before, after, 1)
    try:
        check_sources(sources['dpvs'], sources['scene'])
    except (AssertionError, ValueError):
        continue
    raise AssertionError(f'source mutant survived: {after!r}')

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef float vec3_t[3];
typedef struct {float cullDist; vec3_t origin; void *model; vec3_t mins, maxs;} GfxStaticModelInstance;
typedef struct {int smodelCount; GfxStaticModelInstance *smodelInsts;} GfxWorld;
typedef struct {int cullState;} GfxSceneEntity;
typedef struct {int reType, smodel;} GfxEntity;
static GfxEntity frameEntities[0x2000];
static struct {
    int viewCount;
    struct {int entityCount; GfxEntity *entities;} def;
    GfxSceneEntity sceneEnts[2046];
} scene;
static struct {int entityCount;} frontEnd, *frontEndDataOut = &frontEnd;
static struct {struct {vec3_t origin;} lodParms;} rg;
static struct {GfxWorld *world;} rgp;
static int refused, skins, failNextAlloc;
static int drawnOrder[0x2000], drawnCount;
static float Vec3Distance(const float *a, const float *b)
{
    float x = a[0] - b[0], y = a[1] - b[1], z = a[2] - b[2];
    return sqrtf(x * x + y * y + z * z);
}
/* R_AddStaticModelToScene's and R_AddRefEntityToScene's limits. */
static int R_TakeSceneSlot(int reType, int smodel)
{
    int entIndex = scene.def.entityCount;
    if (entIndex > 0x7fd || frontEnd.entityCount > 0x1ff7) {
        ++refused;
        return -1;
    }
    frontEnd.entityCount++;
    scene.def.entityCount++;
    scene.def.entities[entIndex].reType = reType;
    scene.def.entities[entIndex].smodel = smodel;
    scene.sceneEnts[entIndex].cullState = 2;
    return entIndex;
}
static int R_AddStaticModelToScene(int smodelIndex) { return R_TakeSceneSlot(2, smodelIndex); }
static void R_SkinStaticModel(GfxSceneEntity *sceneEnt, GfxEntity *ent, int smodelIndex)
{
    assert(sceneEnt == &scene.sceneEnts[ent - scene.def.entities] && ent->smodel == smodelIndex && ent->reType == 2);
    ++skins;
}
static void R_AddXModelSurfaces(int entIndex)
{
    assert(scene.sceneEnts[entIndex].cullState == 5);
    drawnOrder[drawnCount++] = scene.def.entities[entIndex].smodel;
}
static void *test_realloc(void *p, size_t n)
{
    if (failNextAlloc) {
        --failNextAlloc;
        return NULL;
    }
    return realloc(p, n);
}
#define realloc test_realloc
'''

test = r'''
#include "QUEUE"

#define MAX_SMODELS 9000
static GfxStaticModelInstance insts[MAX_SMODELS];
static GfxWorld world = {0, insts};
static int drawnFlag[MAX_SMODELS];

static void place(int i, float x, float y, float size)
{
    insts[i].origin[0] = x; insts[i].origin[1] = y; insts[i].origin[2] = 0.0f;
    for (int k = 0; k < 3; k++) {
        insts[i].mins[k] = insts[i].origin[k] - size * 0.5f / sqrtf(3.0f);
        insts[i].maxs[k] = insts[i].origin[k] + size * 0.5f / sqrtf(3.0f);
    }
}

static void newScene(int cgameEntities)
{
    scene.viewCount++;
    scene.def.entityCount = 0;
    scene.def.entities = &frameEntities[frontEnd.entityCount];
    refused = skins = drawnCount = 0;
    memset(drawnFlag, 0, sizeof(drawnFlag));
    for (int i = 0; i < cgameEntities; i++)
        assert(R_TakeSceneSlot(0, -1) >= 0);
}

static int addEffects(int count)
{
    int taken = 0;
    for (int i = 0; i < count; i++)
        taken += R_TakeSceneSlot(5, -1) >= 0;
    return taken;
}

static void flush(void)
{
    R_AddQueuedStaticModels();
    for (int i = 0; i < drawnCount; i++) {
        assert(!drawnFlag[drawnOrder[i]]);
        drawnFlag[drawnOrder[i]] = 1;
    }
    assert(skins == drawnCount);
}

static int rankAt(float distance, float size)
{
    GfxStaticModelInstance inst;
    memset(&inst, 0, sizeof(inst));
    inst.origin[0] = distance;
    inst.mins[0] = distance - size; inst.maxs[0] = distance;
    return R_StaticModelRank(&inst);
}

int main(void)
{
    rgp.world = &world;

    /* Distance over size, 16 steps a doubling: farther ranks later, larger sooner. */
    assert(rankAt(0.0f, 50.0f) == 0 && rankAt(3.0f, 50.0f) == 0);
    assert(rankAt(50.0f, 50.0f) == 64 && rankAt(100.0f, 50.0f) == 80 && rankAt(200.0f, 50.0f) == 96);
    assert(rankAt(400.0f, 100.0f) == rankAt(200.0f, 50.0f));
    for (float d = 1.0f; d < 1.0e6f; d *= 1.07f) {
        assert(rankAt(d, 50.0f) <= rankAt(d * 1.07f, 50.0f));
        assert(rankAt(d, 400.0f) <= rankAt(d, 50.0f));
        assert(rankAt(d, 50.0f) >= 0 && rankAt(d, 50.0f) < R_SMODEL_RANKS);
    }
    assert(rankAt(1.0e9f, 0.0f) == R_SMODEL_RANKS - 1 && rankAt(4096.0f, 0.5f) == R_SMODEL_RANKS - 1);
    assert(rankAt(NAN, 50.0f) == R_SMODEL_RANKS - 1);

    /* A scene with room for every static model keeps them all, in culling order, after the effects. */
    world.smodelCount = 1000;
    for (int i = 0; i < 1000; i++)
        place(i, 100.0f + 7.0f * i, (float)(i % 13), 60.0f);
    newScene(40);
    R_BeginStaticModelQueue();
    for (int i = 999; i >= 0; i -= 2)
        R_QueueStaticModel(i);
    assert(scene.def.entityCount == 40 && drawnCount == 0);
    assert(addEffects(300) == 300);
    flush();
    assert(drawnCount == 500 && refused == 0 && scene.def.entityCount == 840);
    for (int i = 0; i < 500; i++)
        assert(drawnOrder[i] == 999 - 2 * i);
    for (int e = 0; e < 340; e++)
        assert(scene.def.entities[e].reType != 2);

    /* Brecourt: 8,500 grass clumps and 60 hedges pass culling. The effects keep every particle and
       the static models fill exactly the slots left, nearest and largest first; nothing is refused. */
    world.smodelCount = 8560;
    for (int i = 0; i < 8500; i++) {
        float angle = i * 2.39996323f, distance = 40.0f + 6000.0f * sqrtf((i + 0.5f) / 8500.0f);
        place(i, distance * cosf(angle), distance * sinf(angle), 45.0f);
    }
    for (int i = 8500; i < 8560; i++)
        place(i, 3000.0f + 40.0f * (i - 8500), 500.0f, 900.0f);
    newScene(120);
    R_BeginStaticModelQueue();
    for (int i = 0; i < 8560; i++)
        R_QueueStaticModel((i * 7919) % 8560);
    assert(drawnCount == 0 && scene.def.entityCount == 120);
    assert(addEffects(400) == 400);
    flush();
    assert(refused == 0 && scene.def.entityCount == 2046 && drawnCount == 2046 - 520);
    int worstKept = 0, bestDropped = R_SMODEL_RANKS;
    for (int i = 0; i < 8560; i++) {
        int rank = R_StaticModelRank(&insts[i]);
        if (drawnFlag[i] && rank > worstKept)
            worstKept = rank;
        if (!drawnFlag[i] && rank < bestDropped)
            bestDropped = rank;
    }
    assert(worstKept <= bestDropped);
    for (int i = 0; i < drawnCount - 1; i++)
        assert(R_StaticModelRank(&insts[drawnOrder[i]]) <= R_StaticModelRank(&insts[drawnOrder[i + 1]]));
    for (int i = 0; i < 8500; i++) {
        float distance = Vec3Distance(insts[i].origin, rg.lodParms.origin);
        if (distance < 1500.0f)
            assert(drawnFlag[i]);
        if (distance > 4000.0f)
            assert(!drawnFlag[i]);
    }
    for (int i = 8500; i < 8560; i++)
        assert(drawnFlag[i]);

    /* Whichever order culling walks the cells in, as aiming changes which pass, the nearest stay. */
    newScene(120);
    R_BeginStaticModelQueue();
    for (int i = 8559; i >= 0; i--)
        R_QueueStaticModel(i);
    assert(addEffects(400) == 400);
    flush();
    assert(refused == 0 && scene.def.entityCount == 2046);
    for (int i = 0; i < 8500; i++)
        if (Vec3Distance(insts[i].origin, rg.lodParms.origin) < 1500.0f)
            assert(drawnFlag[i]);

    /* The frame's 8,184 entities bound the room as well as the scene's 2,046. */
    frontEnd.entityCount = 0x1ff8 - 700;
    newScene(100);
    R_BeginStaticModelQueue();
    for (int i = 0; i < 2000; i++)
        R_QueueStaticModel(i);
    assert(addEffects(100) == 100);
    flush();
    assert(refused == 0 && drawnCount == 500 && frontEnd.entityCount == 0x1ff8);
    frontEnd.entityCount = 0;

    /* A full scene keeps no static model; a second flush, or one without culling, adds nothing. */
    newScene(2000);
    R_BeginStaticModelQueue();
    for (int i = 0; i < 100; i++)
        R_QueueStaticModel(i);
    assert(addEffects(46) == 46);
    flush();
    assert(drawnCount == 0 && refused == 0);
    flush();
    assert(drawnCount == 0 && refused == 0);

    /* Outside culling the queue is closed: a static model goes in at once. */
    newScene(0);
    R_QueueStaticModel(17);
    assert(drawnCount == 1 && drawnOrder[0] == 17 && scene.def.entityCount == 1);

    /* A scene that never reached its flush leaves nothing for the next. */
    newScene(0);
    R_BeginStaticModelQueue();
    for (int i = 0; i < 50; i++)
        R_QueueStaticModel(i);
    newScene(0);
    R_BeginStaticModelQueue();
    R_QueueStaticModel(3000);
    flush();
    assert(drawnCount == 1 && drawnOrder[0] == 3000);

    /* A larger world grows the queue; when it cannot, its models go in at once as before. */
    world.smodelCount = MAX_SMODELS;
    failNextAlloc = 1;
    newScene(0);
    R_BeginStaticModelQueue();
    assert(s_smodelQueueSize == 8560);
    for (int i = 0; i < 8600; i++)
        R_QueueStaticModel(i);
    assert(drawnCount == 40);
    flush();
    assert(drawnCount == 2046 && refused == 0);
    newScene(0);
    R_BeginStaticModelQueue();
    assert(s_smodelQueueSize == MAX_SMODELS);
    free(s_smodelQueue);
    free(s_smodelRanked);

    puts("PASS: static models queue during culling and fill the slots the effects leave, ranked by distance over "
         "size in 16 steps a doubling; all of them in culling order when they fit; the nearest grass and far hedges "
         "kept, aimed or not; the scene's and the frame's limits, none refused; a full, closed, stale or unallocated "
         "queue");
    return 0;
}
'''

block = queue_block(dpvs)
mutants = [
    ('    rank = (int)(bits >> 19) - (123 << 4);', '    rank = (int)(bits >> 20) - (123 << 3);'),
    ('    float ratio = Vec3Distance(smodelInst->origin, rg.lodParms.origin) / (size > 1.0f ? size : 1.0f);',
     '    float ratio = Vec3Distance(smodelInst->origin, rg.lodParms.origin);'),
    ('(size > 1.0f ? size : 1.0f)', '(size + 1.0f)'),
    ('    if (rank < 0)\n        return 0;\n', ''),
    ('    return rank < R_SMODEL_RANKS ? rank : R_SMODEL_RANKS - 1;', '    return rank;'),
    ('    if (count <= room) {', '    if (1) {'),
    ('    room = 0x7fe - scene.def.entityCount;', '    room = 0x7fe;'),
    ('    if (room > 0x1ff8 - frontEndDataOut->entityCount)\n        room = 0x1ff8 - frontEndDataOut->entityCount;\n', ''),
    ('        s_smodelRanked[rankStart[s_smodelQueue[i].rank]++] = s_smodelQueue[i].smodelIndex;',
     '        s_smodelRanked[i] = s_smodelQueue[i].smodelIndex;'),
    ('        rankStart[i] = total;\n', ''),
    ('    for (i = 0; i < room; i++)\n        R_AddStaticModelSurfaces(s_smodelRanked[i]);',
     '    for (i = 0; i < count; i++)\n        R_AddStaticModelSurfaces(s_smodelRanked[i]);'),
    ('    if (!s_smodelQueueOpen || s_smodelQueueCount >= s_smodelQueueSize) {',
     '    if (s_smodelQueueCount >= s_smodelQueueSize) {'),
    ('    s_smodelQueueOpen = 0;\n    s_smodelQueueCount = 0;\n', '    s_smodelQueueCount = 0;\n'),
    ('    s_smodelQueueCount = 0;\n    s_smodelQueueOpen = 1;\n', '    s_smodelQueueOpen = 1;\n'),
    ('    if (queue && ranked)\n        s_smodelQueueSize = smodelCount;', '    s_smodelQueueSize = smodelCount;'),
    ('    if (smodelCount <= s_smodelQueueSize)\n        return;\n', '    if (s_smodelQueueSize)\n        return;\n'),
    ('    sceneEnt->cullState = 5;\n    R_AddXModelSurfaces(entIndex);\n}\n\n/* Distance',
     '    R_AddXModelSurfaces(entIndex);\n}\n\n/* Distance'),
]

with tempfile.TemporaryDirectory(prefix='cod2-smodel-queue-') as tmp:
    path = Path(tmp)

    def build(queue, name, sanitize):
        (path / f'{name}.c').write_text(support + queue.replace('#ifdef __EMSCRIPTEN__\n', '', 1)
                                        .rsplit('#endif\n', 1)[0])
        (path / f'{name}-test.c').write_text(test.replace('QUEUE', str(path / f'{name}.c')))
        flags = ['-O1', '-g', '-Wall', '-Werror', '-Wno-unused-function', '-fsanitize=address,undefined',
                 '-fno-sanitize-recover=all'] if sanitize else ['-O0', '-w']
        subprocess.run(['cc', '-std=c99', *flags, str(path / f'{name}-test.c'), '-lm', '-o', str(path / name)],
                       check=True)
        return subprocess.run([str(path / name)], capture_output=True, text=True)

    result = build(block, 'queue', True)
    print(result.stdout, end='')
    assert result.returncode == 0, result.stderr
    for index, (before, after) in enumerate(mutants):
        assert block.count(before) == 1, before
        result = build(block.replace(before, after), f'mutant{index}', False)
        assert result.returncode != 0, f'mutant survived: {after!r}'
    print(f'PASS: {len(mutants)} static model queue mutants and {len(source_mutants)} source mutants fail')
