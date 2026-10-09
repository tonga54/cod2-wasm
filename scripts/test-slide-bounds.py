#!/usr/bin/env python3
"""Exercise the actual slide loop's plane capacity and its old overflow."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/bgame/bg_slidemove.c').read_text()
start = source.index('static qboolean PM_SLIDEMOVE_ABI PM_SlideMove(',
                     source.index('void PM_StepSlideMove('))
end = source.index('\nstatic void PM_ApplyStepEvent', start)
body = source[start:end]
capacity = source[source.index('#define MAX_CLIP_PLANES'):].splitlines()[0]
support = r'''
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
typedef int qboolean;
typedef float vec3_t[3];
typedef struct { float fraction; vec3_t normal; int allsolid, entityNum, startsolid, contents, surfaceFlags; char *material; } trace_t;
typedef struct { vec3_t velocity, origin; int gravity, clientNum, pm_time; } playerState_t;
typedef struct { playerState_t *ps; vec3_t mins, maxs; int tracemask; } pmove_t;
typedef struct { float frametime, impactSpeed; int groundPlane; trace_t groundTrace; } pml_t;
#define PM_SLIDEMOVE_ABI
static int calls, hitCount, duplicate;
static const vec3_t normals[] = {{1,0,0},{0,1,0},{0.7071068f,0.7071068f,0},{0.6f,0,0.8f}};
static void PM_playerTrace(pmove_t *pm, trace_t *t, const float *start,
    const float *mins, const float *maxs, const float *end, int client, int mask) {
    (void)pm;(void)start;(void)mins;(void)maxs;(void)end;(void)client;(void)mask;
    memset(t, 0, sizeof(*t));
    t->fraction = calls < hitCount ? 0.1f : 1.0f;
    memcpy(t->normal, normals[duplicate ? 0 : calls % 4], sizeof(vec3_t));
    ++calls;
}
static void PM_AddTouchEnt(pmove_t *pm, int entity) { (void)pm;(void)entity; }
static float Vec3NormalizeTo(const float *v, float *out) {
    float len = sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
    for (int i=0;i<3;i++) out[i]=len ? v[i]/len : 0;
    return len;
}
static float Vec3Normalize(float *v) { return Vec3NormalizeTo(v,v); }
static void Vec3Cross(const float *a,const float *b,float *out) {
    out[0]=a[1]*b[2]-a[2]*b[1];out[1]=a[2]*b[0]-a[0]*b[2];out[2]=a[0]*b[1]-a[1]*b[0];
}
static void PM_ClipVelocity(const float *v, const float *n, float *out) {
    float d=v[0]*n[0]+v[1]*n[1]+v[2]*n[2];
    for(int i=0;i<3;i++) out[i]=v[i]-d*n[i];
}
'''
checks = r'''
int main(void) {
    for(int ground=0;ground<2;ground++) for(int repeated=0;repeated<2;repeated++) {
        for(hitCount=0;hitCount<=4;hitCount++) {
            playerState_t ps = {.velocity={100,100,100}};
            pmove_t pm = {.ps=&ps};
            pml_t pml = {.frametime=0.016f,.groundPlane=ground,.groundTrace={.normal={0,0,1}}};
            calls=0;duplicate=repeated;
            PM_SlideMove(&pm,&pml,0);
            assert(calls==(hitCount<4 ? hitCount+1 : 4));
            if(ground && !repeated && hitCount==4)
                assert(ps.velocity[0]==0 && ps.velocity[1]==0 && ps.velocity[2]==0);
            else assert(ps.velocity[0]>0 && ps.velocity[1]>0 && ps.velocity[2]>0);
        }
    }
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-slide-bounds-') as directory:
    path = Path(directory)
    for mutant in (False, True):
        loop = body.replace('numplanes >= MAX_CLIP_PLANES', 'numplanes > 7') if mutant else body
        (path / 'test.c').write_text(capacity + '\n' + support + loop + checks)
        subprocess.run(['cc', '-std=c99', '-g', '-O1', '-fsanitize=address',
                        str(path / 'test.c'), '-o', str(path / 'test'), '-lm'], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True)
        if mutant:
            assert result.returncode != 0 and 'stack-buffer-overflow' in result.stderr, result.stderr
        else:
            assert result.returncode == 0, result.stderr
print('PASS: 20 slide cases; capacity stops safely; old guard causes an ASan stack overflow')
