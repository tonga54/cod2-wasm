#!/usr/bin/env python3
"""Sprites, oriented particles and tails pick the same atlas frame on every atlas grid."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
tess = (root / 'src/PC/gfx_d3d/rb_tess.c').read_text()


def function(source, name):
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


support = r'''
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#define RB_TESS_REGPARM2_ABI
#define RB_TESS_REGPARM3_SSE_ABI
typedef unsigned char byte; typedef float vec_t; typedef float vec3_t[3]; typedef unsigned int D3DCOLOR;
typedef struct {struct {byte textureAtlasRowCount, textureAtlasColumnCount;} info;} Material;
typedef struct {
 int reType, renderFxFlags; float lighting[3]; vec3_t axis[3]; float scale; vec3_t origin, endpos;
 Material *customMaterial; byte materialRGBA[4]; float materialTime; int materialSubimageIndex;
 float radius[2], rotation, minScreenRadius;
} GfxEntity;
/* RB_TessEntity reads a tail's end at the retail byte offset. */
_Static_assert(offsetof(GfxEntity, endpos) == 0x48, "tail end offset");
typedef struct {float axis[3][3], viewProjectionMatrix[16], inverseViewProjectionMatrix[16];} GfxViewParms;
typedef struct {GfxViewParms *viewParms; struct {float origin[3];} lodParms;} r_backEndGlobals_t;
typedef struct {struct {int integer; float value;} current;} dvar_t;
static GfxViewParms view = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
static r_backEndGlobals_t backEndState = {&view};
static void *imp_backEnd = &backEndState;
static dvar_t renderer, railWidth, *r_rendererInUse = &renderer, *r_railCoreWidth = &railWidth;
static char tessBuffer[64];
static char *RB_TessBase(void) {return tessBuffer;}
static float uv[4]; static int calls;
static void record(float s0, float t0, float s1, float t1) {uv[0] = s0; uv[1] = t0; uv[2] = s1; uv[3] = t1; calls++;}
static void RB_AddQuadStampDx7_impl(const vec_t *o, const vec_t *l, const vec_t *u, int c, float s0, float t0, float s1, float t1) {record(s0, t0, s1, t1);}
static void RB_AddQuadStamp_impl(const vec_t *o, const vec_t *l, const vec_t *u, int c, float s0, float t0, float s1, float t1) {record(s0, t0, s1, t1);}
static void RB_AddLineDx7_impl(const vec_t *a, const vec_t *b, float w, D3DCOLOR c, float s0, float t0, float s1, float t1) {record(s0, t0, s1, t1);}
static void RB_AddLine_impl(const vec_t *a, const vec_t *b, float w, D3DCOLOR c, float s0, float t0, float s1, float t1) {record(s0, t0, s1, t1);}
static void RB_TessParticleCloud(const GfxEntity *re) {assert(!"unexpected cloud");}
static void MakeNormalVectors(const vec_t *forward, vec_t *right, vec_t *up) {
 right[0] = 0; right[1] = 1; right[2] = 0; up[0] = 0; up[1] = 0; up[2] = 1;
}
'''

checks = r'''
static int tested;
static void expect(int reType, int rows, int cols, int frame, int dx7) {
 Material material = {{(byte)rows, (byte)cols}};
 GfxEntity ent = {0};
 ent.reType = reType; ent.customMaterial = &material; ent.materialSubimageIndex = frame;
 ent.radius[0] = ent.radius[1] = 4; ent.axis[0][0] = 1; ent.endpos[0] = 40;
 renderer.current.integer = dx7 ? 2 : 1; calls = 0;
 RB_TessEntity(&ent);
 assert(calls == 1);
 float s0 = 0, t0 = 0, s1 = 1, t1 = 1;
 if (rows * cols > 1) {
  s0 = (float)(frame % cols) / cols; t0 = (float)(frame / cols) / rows;
  s1 = s0 + 1.0f / cols; t1 = t0 + 1.0f / rows;
 }
 if (fabsf(uv[0] - s0) > 1e-6f || fabsf(uv[1] - t0) > 1e-6f || fabsf(uv[2] - s1) > 1e-6f || fabsf(uv[3] - t1) > 1e-6f) {
  fprintf(stderr, "reType %d, %dx%d atlas, frame %d: got %g %g %g %g, expected %g %g %g %g\n",
          reType, rows, cols, frame, uv[0], uv[1], uv[2], uv[3], s0, t0, s1, t1);
  assert(!"atlas frame");
 }
 tested++;
}
int main(void) {
 /* gfx_starburst_1p, the Kar98k muzzle tail, is one row of four frames. */
 const int grids[][2] = {{1, 1}, {1, 4}, {4, 1}, {2, 4}, {4, 2}, {4, 4}, {3, 5}, {5, 3}};
 const int types[] = {4, 7, 8};
 for (unsigned g = 0; g < sizeof(grids) / sizeof(grids[0]); g++)
  for (int frame = 0; frame < grids[g][0] * grids[g][1]; frame++)
   for (unsigned t = 0; t < 3; t++)
    for (int dx7 = 0; dx7 < 2; dx7++)
     expect(types[t], grids[g][0], grids[g][1], frame, dx7);
 printf("PASS: %d sprite, oriented-particle and tail atlas frames across 8 grids and both vertex formats\n", tested);
}
'''

def run(code, directory):
    path = Path(directory)
    (path / 'test.c').write_text(code)
    subprocess.run(['cc', '-std=c11', '-O1', '-g', '-w', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    return subprocess.run([str(path / 'test')], capture_output=True, text=True)


with tempfile.TemporaryDirectory(prefix='cod2-fx-atlas-') as directory:
    code = support + function(tess, 'RB_BuildSprite_impl') + function(tess, 'RB_TessEntity') + checks
    result = run(code, directory)
    assert result.returncode == 0, result.stdout + result.stderr
    print(result.stdout, end='')
    columns_first = ('            int cols = ((Material *)texInfo)->info.textureAtlasColumnCount;\n'
                     '            int rows = ((Material *)texInfo)->info.textureAtlasRowCount;\n')
    assert code.count(columns_first) == 2, 'expected the oriented-particle and tail atlas reads'
    swapped = code.replace(columns_first, '            int cols = ((Material *)texInfo)->info.textureAtlasRowCount;\n'
                                          '            int rows = ((Material *)texInfo)->info.textureAtlasColumnCount;\n')
    assert run(swapped, directory).returncode != 0, 'swapped atlas rows and columns must be rejected'
    print('PASS: swapped atlas rows and columns rejected')
