#!/usr/bin/env python3
"""Sweep the real player/brush/slide/step code along angled wall corners."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
trace = (root / 'src/PC/qcommon/cm_trace.c').read_text()
slide = (root / 'src/PC/bgame/bg_slidemove.c').read_text()
move = (root / 'src/PC/bgame/bg_pmove.c').read_text()

def function(source, name):
    m = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert m, name
    end, depth = m.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[m.start():end] + '\n'

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int qboolean;typedef float vec_t,vec3_t[3];
#define PM_SLIDEMOVE_ABI
#define MAX_CLIP_PLANES 5
typedef struct {float fraction,normal[3];int startsolid,allsolid,contents,entityNum,surfaceFlags;const char *material;} trace_t;
typedef struct {struct {float start[3],end[3],invDelta[3];} extents;
 float size[3],midpoint[3],delta[3],halfDelta[3],halfDeltaAbs[3],deltaLenSq,deltaLen,
       radius,offsetZ,bounds[2][3],radiusOffset[3];int contents,isPoint,axialCullOnly;} traceWork_t;
typedef struct {float normal[3],dist;} cplane_t;
typedef struct {cplane_t *plane;int materialNum;} cbrushside_t;
typedef struct {float mins[3],maxs[3];int contents,numsides,axialMaterialNum[2][3];cbrushside_t *sides;} cbrush_t;
typedef struct {float origin[3],velocity[3];int gravity,clientNum,pm_time,pm_flags,groundEntityNum,pm_type;} playerState_t;
typedef struct {playerState_t *ps;vec3_t mins,maxs;int tracemask;} pmove_t;
typedef struct {float frametime,impactSpeed;int groundPlane;trace_t groundTrace;} pml_t;
static cbrush_t wall,floorBrush;
static cplane_t face,back;
static cbrushside_t sides[2];
static void CM_CalcTraceEntents(void *p) {}
static void CM_InitTraceThreadInfo(traceWork_t *tw) {}
static void CM_SetTraceMaterial(trace_t *t,int m,int contents) {t->contents=contents;}
static float Vec3NormalizeTo(const float *v,float *out) {
 float len=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
 for(int i=0;i<3;i++)out[i]=len?v[i]/len:0;return len;
}
static float Vec3Normalize(float *v) {return Vec3NormalizeTo(v,v);}
static void Vec3Cross(const float *a,const float *b,float *o) {
 o[0]=a[1]*b[2]-a[2]*b[1];o[1]=a[2]*b[0]-a[0]*b[2];o[2]=a[0]*b[1]-a[1]*b[0];
}
static void PM_AddTouchEnt(pmove_t *pm,int ent) {assert(ent==1022);}
static void Jump_ClearState(playerState_t *ps) {}
static int Jump_GetStepHeight(playerState_t *ps,const float *p,float *h) {return 0;}
static int Jump_IsPlayerAboveMax(playerState_t *ps) {return 0;}
static void Jump_ClampVelocity(playerState_t *ps,const float *p) {}
static int PM_StepCheckProne(pmove_t *pm,playerState_t *ps,const float *o,const float *v) {return 1;}
static void PM_ApplyStepEvent(pmove_t *pm,pml_t *pml,playerState_t *ps,const float *o,float h) {}
'''
brush_code = ''.join(function(trace, n) for n in (
    'CM_DotProduct', 'CM_AbsFloat', 'CM_MinFloat', 'CM_InitTraceWork',
    'CM_TraceBrushPlane', 'CM_TraceThroughBrush'))
player_trace = r'''
static void PM_playerTrace(pmove_t *pm,trace_t *t,const float *start,
 const float *mins,const float *maxs,const float *end,int client,int mask) {
 traceWork_t tw;CM_InitTraceWork(&tw,start,end,mins,maxs,mask);
 *t=(trace_t){.fraction=1,.entityNum=1023};
 CM_TraceThroughBrush(&tw,&wall,t);
 if(!t->allsolid)CM_TraceThroughBrush(&tw,&floorBrush,t);
 if(t->fraction<1||t->startsolid)t->entityNum=1022;
}
'''
movement = function(move, 'PM_ClipVelocity')
movement += function(slide, 'PM_SlideMove') + function(slide, 'PM_StepSlideMove')
checks = r'''
static void geometry(float angle) {
 float a=angle*.01745329252f;
 face=(cplane_t){.normal={cosf(a),sinf(a),0}};
 back=(cplane_t){.normal={-cosf(a),-sinf(a),0},.dist=128};
 sides[0]=(cbrushside_t){.plane=&face};sides[1]=(cbrushside_t){.plane=&back};
 wall=(cbrush_t){.mins={-10000,-10000,-100},.maxs={10000,10000,1000},.contents=1,.numsides=2,.sides=sides};
 floorBrush=(cbrush_t){.mins={-10000,-10000,-100},.maxs={10000,10000,0},.contents=1};
}
int main(void) {
 int cases=0;
 for(int angle=0;angle<360;angle+=15)for(int height=30;height<=70;height+=20)
 for(int dt=4;dt<=66;dt+=2)for(int sprint=0;sprint<2;sprint++) {
  geometry(angle);
  playerState_t ps={.origin={face.normal[0]*15.25f,face.normal[1]*15.25f,.125f},.groundEntityNum=1022};
  pmove_t pm={.ps=&ps,.mins={-15,-15,0},.maxs={15,15,height},.tracemask=1};
  pml_t pml={.frametime=dt*.001f,.groundPlane=1,.groundTrace={.normal={0,0,1}}};
  float tangent[2]={-face.normal[1],face.normal[0]},speed=sprint?275.5f:190.f;
  /* Starting clear of the round body, with the same contact used by mesh
   * sweeps, must not be reported embedded in an angled brush. */
  trace_t t;PM_playerTrace(&pm,&t,ps.origin,pm.mins,pm.maxs,ps.origin,0,1);
  assert(!t.startsolid&&!t.allsolid&&t.fraction==1);
  float duration=0;
  for(int time=0;time<1000;time+=dt) {
   ps.velocity[0]=speed*.8f*tangent[0]-speed*.6f*face.normal[0];
   ps.velocity[1]=speed*.8f*tangent[1]-speed*.6f*face.normal[1];ps.velocity[2]=0;
   PM_StepSlideMove(&pm,&pml,0);duration+=pml.frametime;
   assert(ps.origin[0]*face.normal[0]+ps.origin[1]*face.normal[1]>=15.f-.001f);
   assert(ps.origin[2]>=0&&ps.origin[2]<.3f);
  }
  float travel=ps.origin[0]*tangent[0]+ps.origin[1]*tangent[1];
  assert(travel>=speed*.8f*duration*.98f);
  /* A real overlap still blocks movement; this is not a smaller player. */
  ps.origin[0]=face.normal[0]*14;ps.origin[1]=face.normal[1]*14;
  PM_playerTrace(&pm,&t,ps.origin,pm.mins,pm.maxs,ps.origin,0,1);
  assert(t.startsolid&&t.allsolid);
  cases++;
 }
 /* Slanted slopes/walls use capsule support; zero-sized bullet traces and
  * axial wall/floor boundaries retain the same hit distances. */
 for(int z=0;z<=10;z++) {
  geometry(45);face.normal[2]=z*.1f;Vec3Normalize(face.normal);
  back.normal[0]=-face.normal[0];back.normal[1]=-face.normal[1];back.normal[2]=-face.normal[2];
  traceWork_t tw;vec3_t mins={-15,-15,-35},maxs={15,15,35};
  float support=15+20*face.normal[2];
  vec3_t start,end;for(int i=0;i<3;i++){start[i]=face.normal[i]*(support+10);end[i]=face.normal[i]*(support-10);}
  CM_InitTraceWork(&tw,start,end,mins,maxs,1);trace_t t={.fraction=1};
  CM_TraceThroughBrush(&tw,&wall,&t);assert(fabsf(t.fraction-.49375f)<.0001f);
  vec3_t zero={0};for(int i=0;i<3;i++){start[i]=face.normal[i]*10;end[i]=-start[i];}
  CM_InitTraceWork(&tw,start,end,zero,zero,1);t=(trace_t){.fraction=1};
  CM_TraceThroughBrush(&tw,&wall,&t);assert(fabsf(t.fraction-.49375f)<.0001f);
 }
 printf("PASS: %d wall-slide schedules, 24 rotations, three heights, walk/sprint, overlaps and slope/point boundaries\n",cases);
}
'''
old_support = '''support = CM_AbsFloat(normal[0]) * CM_AbsFloat(tw->size[0]) +
              CM_AbsFloat(normal[1]) * CM_AbsFloat(tw->size[1]) +
              CM_AbsFloat(normal[2]) * CM_AbsFloat(tw->size[2]);'''
with tempfile.TemporaryDirectory(prefix='cod2-corners-') as directory:
    d = Path(directory)
    for index, variant in enumerate((brush_code, brush_code.replace(
            'support = tw->radius + CM_AbsFloat(normal[2]) * tw->offsetZ;', old_support))):
        (d / 'test.c').write_text(support + variant + player_trace + movement + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(d / 'test.c'), '-o', str(d / 'test'), '-lm'], check=True)
        result = subprocess.run([str(d / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr)
        if index == 0:
            print(result.stdout.strip())
print('PASS: original square-shoulder collision mutant reproduces the corner overlap and fails')
