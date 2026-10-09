#!/usr/bin/env python3
"""Exercise the real step selection for world surfaces and player blockers."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/bgame/bg_slidemove.c').read_text()
body = source[source.index('void PM_StepSlideMove(pmove_t *pm, pml_t *pml, qboolean gravity)\n{'):]
support = r'''
#include <stdio.h>
#include <assert.h>
typedef int qboolean;
typedef float vec3_t[3];
typedef struct { float fraction; int entityNum; vec3_t normal; } trace_t;
typedef struct { vec3_t origin,velocity; int pm_flags,pm_time,groundEntityNum,clientNum,pm_type; } playerState_t;
typedef struct { playerState_t *ps; vec3_t mins,maxs; int tracemask; } pmove_t;
typedef struct { int groundPlane; } pml_t;
static int slideCalls,traceCalls,entity;
static int PM_SlideMove(pmove_t *pm,pml_t *pml,int gravity) {
    pm->ps->origin[0]=++slideCalls;
    return 1;
}
static void PM_playerTrace(pmove_t *pm,trace_t *t,const float *start,const float *mins,
    const float *maxs,const float *end,int client,int mask) {
    t->fraction=traceCalls++ ? 0.5f : 1.0f;
    t->entityNum=entity;t->normal[0]=t->normal[1]=0;t->normal[2]=1;
}
static void Jump_ClearState(playerState_t *ps) {}
static int Jump_GetStepHeight(playerState_t *ps,const float *o,float *step) { *step=18;return 1; }
static int Jump_IsPlayerAboveMax(playerState_t *ps) { return 0; }
static void Jump_ClampVelocity(playerState_t *ps,const float *o) {}
static void PM_ClipVelocity(const float *v,const float *n,float *out) {}
static int PM_StepCheckProne(pmove_t *pm,playerState_t *ps,const float *o,const float *v) { return 1; }
static void PM_ApplyStepEvent(pmove_t *pm,pml_t *pml,playerState_t *ps,const float *o,float height) {}
'''
checks = r'''
int main(void) {
    for(entity=0;entity<1024;entity++) {
        playerState_t ps={.velocity={10,0,0},.groundEntityNum=1022};
        pmove_t pm={.ps=&ps};pml_t pml={.groundPlane=1};
        slideCalls=traceCalls=0;
        PM_StepSlideMove(&pm,&pml,0);
        if(ps.origin[0] != (entity<64 ? 1 : 2)) return 1;
    }
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-step-move-') as directory:
    path=Path(directory)
    for mutant in (False,True):
        loop=body.replace('trace.entityNum < 64','trace.entityNum > 0x3f') if mutant else body
        (path/'test.c').write_text(support+loop+checks)
        subprocess.run(['cc','-std=c99',str(path/'test.c'),'-o',str(path/'test')],check=True)
        result=subprocess.run([str(path/'test')])
        assert result.returncode==(1 if mutant else 0),result.returncode
print('PASS: steps accept world/brush entities and reject players; old comparison fails (1024 cases)')
