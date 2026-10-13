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
typedef struct {float start[3],end[3],invDelta[3];} TraceExtents;
#define PM_REGPARM2_ABI
#define ENTITYNUM_NONE 1023
#define PM_SLIDEMOVE_ABI
#define MAX_CLIP_PLANES 5
typedef struct {float fraction,normal[3];int startsolid,allsolid,contents,entityNum,surfaceFlags;const char *material;} trace_t;
typedef struct {TraceExtents extents;
 float size[3],midpoint[3],delta[3],halfDelta[3],halfDeltaAbs[3],deltaLenSq,deltaLen,
       radius,offsetZ,bounds[2][3],radiusOffset[3];int contents,isPoint,axialCullOnly;} traceWork_t;
typedef struct {float normal[3],dist;} cplane_t;
typedef struct {cplane_t *plane;int materialNum;} cbrushside_t;
typedef struct {float mins[3],maxs[3];int contents,numsides,axialMaterialNum[2][3];cbrushside_t *sides;} cbrush_t;
typedef struct {float origin[3],velocity[3];int gravity,clientNum,pm_time,pm_flags,groundEntityNum,pm_type,eFlags;float proneDirection,fTorsoHeight,fTorsoPitch,fWaistPitch;vec3_t vLadderVec;} playerState_t;
typedef struct {playerState_t *ps;vec3_t mins,maxs;int tracemask,handler,numtouch,touchents[32];struct {int forwardmove;}cmd;} pmove_t;
typedef struct {float frametime,impactSpeed;int groundPlane,almostGroundPlane,walking;vec3_t previous_origin,previous_velocity;trace_t groundTrace;} pml_t;
static cbrush_t wall,floorBrush,cornerWall;
static cplane_t face,back;
static cbrushside_t sides[2];
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
static void Jump_ClearState(playerState_t *ps) {ps->pm_flags&=~0x80000;}
static int Jump_GetStepHeight(playerState_t *ps,const float *p,float *h) {return 0;}
static int Jump_IsPlayerAboveMax(playerState_t *ps) {return 0;}
static void Jump_ClampVelocity(playerState_t *ps,const float *p) {}

static void PM_ApplyStepEvent(pmove_t *pm,pml_t *pml,playerState_t *ps,const float *o,float h) {}
'''
brush_code = ''.join(function(trace, n) for n in (
    'CM_DotProduct', 'CM_AbsFloat', 'CM_MinFloat', 'CM_InitTraceWork',
    'CM_TraceMayHitBounds', 'CM_ClipBrushPlaneDistances', 'CM_TraceBrushPlane', 'CM_TraceBrushAxialPlane', 'CM_TraceBoundsOverlap', 'CM_BrushBoundsOverlap', 'CM_TraceBrushPlanes', 'CM_TraceThroughBrush'))
box_code = (root / 'src/PC/qcommon/cm_tracebox.c').read_text()
brush_code = ''.join(function(box_code, n) for n in
                     ('CM_CalcTraceEntents', 'CM_TraceBox')) + brush_code
player_trace = r'''
static void PM_playerTrace(pmove_t *pm,trace_t *t,const float *start,
 const float *mins,const float *maxs,const float *end,int client,int mask) {
 traceWork_t tw;CM_InitTraceWork(&tw,start,end,mins,maxs,mask);
 *t=(trace_t){.fraction=1,.entityNum=1023};
 CM_TraceThroughBrush(&tw,&wall,t);
 if(!t->allsolid)CM_TraceThroughBrush(&tw,&cornerWall,t);
 if(!t->allsolid)CM_TraceThroughBrush(&tw,&floorBrush,t);
 if(t->fraction<1||t->startsolid)t->entityNum=1022;
}
'''
prone_support = r'''
#define PCT_ACTOR 1
typedef int proneCheckType_t;
typedef struct {void (*trace)(trace_t *,const float *,const float *,const float *,const float *,int,int);}pmoveHandler_t;
static pmove_t *pronePm;
static void proneTrace(trace_t *t,const float *s,const float *mi,const float *ma,const float *e,int cl,int mask){PM_playerTrace(pronePm,t,s,mi,ma,e,cl,mask);}
static pmoveHandler_t proneHandlers[]={{proneTrace}};static void *imp_pmoveHandlers=proneHandlers;
static void AngleVectors(const float *a,float *f,float *r,float *u){
 float yaw=a[1]*.01745329252f;
 if(f){f[0]=cosf(yaw);f[1]=sinf(yaw);f[2]=0;}
 if(r){r[0]=sinf(yaw);r[1]=-cosf(yaw);r[2]=0;}
 if(u){u[0]=u[1]=0;u[2]=1;}
}
static float vectopitch(const float *v){return -atan2f(v[2],hypotf(v[0],v[1]))*57.295779513f;}
static float AngleNormalize180(float x){while(x>180)x-=360;while(x<-180)x+=360;return x;}
static float AngleSubtract(float a,float b){return AngleNormalize180(a-b);}
'''
prone_source=(root/'src/PC/bgame/bg_misc.c').read_text()
prone_support += function(prone_source,'BG_CheckProneValid') + function(prone_source,'BG_CheckProne')
prone_support += function(slide,'PM_StepCheckProne')
ground_support = r'''
typedef struct {struct {float value;}current;}dvar_t;
static dvar_t minFall={{256}},maxFall={{480}},*bg_fallDamageMinHeight=&minFall,*bg_fallDamageMaxHeight=&maxFall;
static int fallHits;
static void BG_AnimScriptEvent(playerState_t *ps,int a,int b,int c){}
static void BG_AddPredictableEventToPlayerstate(int event,int parm,playerState_t *ps){if(event>=116&&event<=138)fallHits++;}
static void Com_Printf(const char *s){}
'''
ground_support += re.search(r'static const vec3_t CorrectSolidDeltas\[26\] = \{.*?\n\};',move,re.S)[0]
ground_support += function(move,'PM_GroundTrace')
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
 /* Real prone support traces, with a zero ladder vector, must permit
  * crawling over flat ground in every direction at all command steps. */
 geometry(0);
 for(int yaw=0;yaw<360;yaw+=45)for(int dt=4;dt<=66;dt++) {
  float angle=yaw*.01745329252f;
  playerState_t ps={.origin={200,200,.125f},.groundEntityNum=1022,.pm_flags=1,.proneDirection=yaw};
  pmove_t pm={.ps=&ps,.mins={-15,-15,0},.maxs={15,15,30},.tracemask=1};pronePm=&pm;
  pml_t pml={.frametime=dt*.001f,.groundPlane=1,.groundTrace={.normal={0,0,1}}};
  float duration=0;
  for(int time=0;time<2000;time+=dt) {
   ps.velocity[0]=28.5f*cosf(angle);ps.velocity[1]=28.5f*sinf(angle);ps.velocity[2]=0;
   PM_StepSlideMove(&pm,&pml,0);duration+=pml.frametime;
   assert(fabsf(ps.origin[2]-.125f)<.01f);
  }
  assert(hypotf(ps.origin[0]-200,ps.origin[1]-200)>28.5f*duration*.98f);
 }
 /* Repeated real gravity/ground/slide sweeps against a car-height box
  * and a narrow elevated sill: no oscillation may accumulate lethal energy. */
 for(int object=0;object<2;object++)for(int dt=4;dt<=66;dt++) {
  geometry(0);
  wall=(cbrush_t){.mins={-50,-100,-100},.maxs={50,100,36},.contents=1};
  if(object)wall=(cbrush_t){.mins={-8,-300,-100},.maxs={8,300,128},.contents=1};
  playerState_t ps={.origin={-65.25f,0,.125f},.groundEntityNum=1023,.gravity=800};
  if(object){ps.origin[0]=7.9f;ps.origin[1]=-150;ps.origin[2]=128.125f;}
  pmove_t pm={.ps=&ps,.mins={-15,-15,0},.maxs={15,15,70},.tracemask=1};
  pml_t pml={.frametime=dt*.001f};fallHits=0;
  for(int time=0;time<6000;time+=dt) {
   memcpy(pml.previous_origin,ps.origin,sizeof(vec3_t));memcpy(pml.previous_velocity,ps.velocity,sizeof(vec3_t));
   PM_GroundTrace(&pm,&pml);
   if(!time&&!object){ps.velocity[2]=sqrtf(2*800*39);ps.groundEntityNum=1023;pml.walking=pml.groundPlane=0;}
   ps.velocity[0]=object?0:75;ps.velocity[1]=object?50:0;
   if(pml.walking)ps.velocity[2]=0;
   PM_StepSlideMove(&pm,&pml,!pml.walking);PM_GroundTrace(&pm,&pml);
   for(int a=0;a<3;a++){assert(isfinite(ps.origin[a]));assert(isfinite(ps.velocity[a]));}
   assert(!fallHits&&ps.origin[2]>-.01f&&ps.origin[2]<(object?129:40)&&fabsf(ps.velocity[2])<700);
  }
  if(object)assert(ps.origin[1]>100&&fabsf(ps.origin[2]-128.125f)<.5f);
  else assert(ps.origin[0]>200&&ps.origin[2]<.3f); // Crosses the car and lands normally.
 }
 /* A shallow overlap at both faces of a corner needs the second recovery
  * shell. Once clear, normal slide movement must immediately work again. */
 for(int stance=0;stance<3;stance++)for(int angle=0;angle<360;angle+=15) {
  geometry(angle);
  cplane_t cornerFace={.normal={-face.normal[1],face.normal[0],0}};
  cplane_t cornerBack={.normal={face.normal[1],-face.normal[0],0},.dist=128};
  cbrushside_t cornerSides[2]={{.plane=&cornerFace},{.plane=&cornerBack}};
  cornerWall=wall;cornerWall.sides=cornerSides;
  playerState_t ps={.groundEntityNum=1022,.gravity=800};
  for(int i=0;i<2;i++)ps.origin[i]=(face.normal[i]+cornerFace.normal[i])*13.6f;
  ps.origin[2]=.125f;
  pmove_t pm={.ps=&ps,.mins={-15,-15,0},.maxs={15,15,30+20*stance},.tracemask=1};
  pml_t pml={.frametime=.033f};fallHits=0;
  trace_t t;PM_playerTrace(&pm,&t,ps.origin,pm.mins,pm.maxs,ps.origin,0,1);assert(t.allsolid);
  PM_GroundTrace(&pm,&pml);
  PM_playerTrace(&pm,&t,ps.origin,pm.mins,pm.maxs,ps.origin,0,1);
  if(t.startsolid||t.allsolid)fprintf(stderr,"corner stance=%d angle=%d origin=%.4f,%.4f,%.4f\n",stance,angle,ps.origin[0],ps.origin[1],ps.origin[2]);
  assert(!t.startsolid&&!t.allsolid);
  float before[3];memcpy(before,ps.origin,sizeof(before));
  for(int i=0;i<2;i++)ps.velocity[i]=(face.normal[i]+cornerFace.normal[i])*100;
  PM_SlideMove(&pm,&pml,0);
  assert(hypotf(ps.origin[0]-before[0],ps.origin[1]-before[1])>4&&!fallHits);
  /* Deep embedding is not permission to teleport through solid walls. */
  for(int i=0;i<2;i++)ps.origin[i]=(face.normal[i]+cornerFace.normal[i])*10;
  ps.origin[2]=.125f;PM_GroundTrace(&pm,&pml);
  PM_playerTrace(&pm,&t,ps.origin,pm.mins,pm.maxs,ps.origin,0,1);assert(t.allsolid);
  assert(!pml.walking&&!fallHits);
 }
 cornerWall=(cbrush_t){0};
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
 printf("PASS: %d wall-slide schedules, 504 real prone-support crawls, 126 car/sill gravity replays, overlaps and slope/point boundaries\n",cases);
}
'''
old_support = '''support = CM_AbsFloat(normal[0]) * CM_AbsFloat(tw->size[0]) +
              CM_AbsFloat(normal[1]) * CM_AbsFloat(tw->size[1]) +
              CM_AbsFloat(normal[2]) * CM_AbsFloat(tw->size[2]);'''
with tempfile.TemporaryDirectory(prefix='cod2-corners-') as directory:
    d = Path(directory)
    variants = (brush_code, brush_code.replace(
            'support = tw->radius + CM_AbsFloat(normal[2]) * tw->offsetZ;', old_support), brush_code)
    for index, variant in enumerate(variants):
        ground = ground_support
        if index == 2:
            ground = ground.replace('i < 84', 'i < 26').replace('i == 84', 'i == 26')
        (d / 'test.c').write_text(support + variant + player_trace + prone_support + ground + movement + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(d / 'test.c'), '-o', str(d / 'test'), '-lm'], check=True)
        result = subprocess.run([str(d / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr)
        if index == 0:
            print(result.stdout.strip())
print('PASS: original square-shoulder collision mutant reproduces the corner overlap and fails')
print('PASS: 72 shallow corner recoveries preserve movement; one-unit recovery fails and deeper embedding stays blocked')
