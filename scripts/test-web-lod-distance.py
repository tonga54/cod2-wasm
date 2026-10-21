#!/usr/bin/env python3
"""Verify the production LOD distance setup against model transitions/culling."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
scene = (root / 'src/PC/gfx_d3d/r_scene.c').read_text()
models = (root / 'src/PC/xanim/xmodel_utils.c').read_text()

def function(source, name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'

support = '''
#include <assert.h>
#include <math.h>
#include <stdio.h>
typedef unsigned char byte;
typedef struct { int modified; union { int enabled; float value; } current; } dvar_t;
typedef struct { float vieworg[3], fov_x; } refdef_t;
typedef struct { float origin[3]; } GfxViewParms;
typedef struct { struct { float origin[3], scale, bias; } lodParms; } r_globals_t;
typedef struct { int numLods; struct { float dist; } lodInfo[4]; } XModel;
static struct { byte enabled, pad[3]; float dist; } g_testLods[4];
static dvar_t lock, scale, bias, distance;
static const dvar_t *lockPtr = &lock, *scalePtr = &scale, *biasPtr = &bias;
static const dvar_t *r_lodDistanceScale = &distance;
static const void *imp_r_lockPvs = &lockPtr, *imp_r_lodScale = &scalePtr, *imp_r_lodBias = &biasPtr;
static r_globals_t rg;
static GfxViewParms lockPvsViewParms;
static void clear(const dvar_t *p) { ((dvar_t *)p)->modified = 0; }
static struct { void (*Dvar_ClearModified)(const dvar_t *); } ri = {clear};
static void R_SetViewParmsForScene(const refdef_t *p, GfxViewParms *v) { for(int i=0;i<3;i++)v->origin[i]=p->vieworg[i]; }
'''
checks = '''
int main(void) {
    refdef_t view = {{10, 20, 30}, 80}; scale.current.value = 1.f;
    XModel prop = {3, {{500}, {1000}, {0}}}, grass = {1, {{1000}}};
    for(int i=0;i<=300;i++) {
        distance.current.value = 1.f + i * .01f;
        R_SetLodOrigin(&view);
        assert(fabsf(rg.lodParms.scale * distance.current.value - 1.f) < 1e-6f);
        float limit = 1000.f * distance.current.value;
        float near = (limit - 1.f) * rg.lodParms.scale;
        float far = (limit + 1.f) * rg.lodParms.scale;
        assert(XModelGetLodForDist(&grass, near) == 0);
        assert(XModelGetLodForDist(&grass, far) == -1);
        assert(near < grass.lodInfo[0].dist && far > grass.lodInfo[0].dist);
        assert(XModelGetLodForDist(&prop, (500.f * distance.current.value - 1.f) * rg.lodParms.scale) == 0);
        assert(XModelGetLodForDist(&prop, (500.f * distance.current.value + 1.f) * rg.lodParms.scale) == 1);
        assert(XModelGetLodForDist(&prop, far) == 2);
    }
    distance.current.value=1.5f;scale.current.value=2.f;bias.current.value=-100.f;
    view.fov_x=40.f;R_SetLodOrigin(&view);
    float fov=tanf(40.f*.5f*.017453292519943295f)/tanf(.69813170079773179f);
    assert(fabsf(rg.lodParms.scale - 2.f/1.5f*fov)<1e-6f);
    assert(fabsf(rg.lodParms.bias + 100.f*fov)<1e-4f);
    assert(rg.lodParms.origin[0]==10 && rg.lodParms.origin[2]==30);
    lock.current.enabled=1;lock.modified=1;view.vieworg[0]=40;R_SetLodOrigin(&view);
    assert(!lock.modified && rg.lodParms.origin[0]==40);
    r_lodDistanceScale=0;view.fov_x=80;R_SetLodOrigin(&view);assert(rg.lodParms.scale==2.f);
    puts("PASS: model detail/grass distance extended 1-4x (2x by default), bounded disappearance, original lower LODs, saved quality/bias and FOV/PVS behavior");
}
'''
# Model detail and grass reach twice their authored distance by default, up to
# four times; the browser profile's old 1.5x default is replaced once.
dvars = (root / 'src/PC/gfx_d3d/r_dvars.c').read_text()
assert 'r_lodDistanceScale = ri.Dvar_RegisterFloat("r_lodDistanceScale", 2.0f, 1.0f, 4.0f, 0x2001);' in dvars
adapter = (root / 'site/native-game-adapter.js').read_text()
assert "saved = saved.replace(/^seta r_lodDistanceScale \"1\\.5\"\\r?$/gm, '');" in adapter
# Both static-model cull paths must use the same adjusted distance as LOD selection.
dpvs = (root / 'src/PC/gfx_d3d/r_dpvs.c').read_text()
assert dpvs.count('float scaledDist = dist * rg.lodParms.scale + rg.lodParms.bias;') == 2
with tempfile.TemporaryDirectory(prefix='cod2-web-lod-') as folder:
    path = Path(folder)
    (path / 'test.c').write_text(support + function(scene, 'R_SetLodOrigin') +
                                 function(models, 'XModelGetLodForDist') + checks)
    subprocess.run(['cc', '-D__EMSCRIPTEN__', '-O1', '-g', '-fsanitize=address,undefined',
                    str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
