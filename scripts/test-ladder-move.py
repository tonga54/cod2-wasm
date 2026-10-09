#!/usr/bin/env python3
"""Exercise actual ladder detection and movement, including jump/collision handoff."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/bgame/bg_pmove.c').read_text()
def function(name):
 m=re.search(r'[^\n;]*\b'+name+r'\([^;]*?\)\n\{',source);assert m,name
 end=m.end();depth=1
 while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[m.start():end]+'\n'
support=r'''
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#define PM_REGPARM2_ABI
#define PM_ACCELERATE_ABI
#define PMF_LADDER 0x20
#define PMF_JUMPING 0x80000
typedef int qboolean;typedef float vec_t;typedef float vec3_t[3];typedef float vec2_t[2];
typedef struct {float fraction;int startsolid,surfaceFlags;vec3_t normal;} trace_t;
typedef struct {int pm_flags,pm_type,jumpTime,clientNum,groundEntityNum,speed;vec3_t origin,velocity,vLadderVec,oldVelocity;} playerState_t;
typedef struct {playerState_t *ps;vec3_t mins,maxs;int tracemask;struct{int serverTime,forwardmove,rightmove,buttons;}cmd;}pmove_t;
typedef struct {int walking,groundPlane;float frametime;vec3_t forward,right;trace_t groundTrace;}pml_t;
typedef struct {struct{float value;int enabled;}current;}dvar_t;
static dvar_t frictionVal={.current.value=5.5},stopVal={.current.value=100},inertiaVal={.current.value=1000},*friction=&frictionVal,*stopspeed=&stopVal,*inertiaMax=&inertiaVal,*inertiaAngle=&inertiaVal,*inertiaDebug=&inertiaVal;
static float PM_DvarFloat(dvar_t *d,float f){return d?d->current.value:f;}
static void Com_Printf(const char*f,... ){}
static float Vec3Normalize(float *v){float l=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(l)for(int i=0;i<3;i++)v[i]/=l;return l;}
static float Vec2Normalize(float *v){float l=hypotf(v[0],v[1]);if(l){v[0]/=l;v[1]/=l;}return l;}
static float PM_DotProduct(const float*a,const float*b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
static void PM_VectorCopy(const float*a,float*b){memcpy(b,a,12);}
static void Jump_ClearState(playerState_t *p){p->pm_flags&=~PMF_JUMPING;}
static float Jump_ReduceFriction(playerState_t*p){return 1;}
static trace_t hit={.fraction=.5f,.surfaceFlags=8,.normal={-1,0,0}};static int traces,slides,airMoves,woodRung;
static void PM_playerTrace(pmove_t *pm,trace_t *r,const float*a,const float*b,const float*c,const float*d,int e,int mask){traces++;assert(mask==pm->tracemask || mask==0x10000);assert(hypotf(d[0]-a[0],d[1]-a[1])<2.01f);*r=hit;if(woodRung && mask!=0x10000)r->surfaceFlags=0;}
static int Jump_Check(pmove_t *pm,pml_t *pml){if(pm->cmd.buttons&0x400){pm->ps->pm_flags&=~PMF_LADDER;return 1;}return 0;}
static void PM_AirMove(pmove_t *pm,pml_t *pml){airMoves++;}
static float PM_CmdScale(playerState_t*p,void*c){return p->speed/127.0f;}
static void PM_ApplyGroundPlane(playerState_t*p,pml_t*l){}
static void PM_StepSlideMove(pmove_t *pm,pml_t *pml,int gravity){assert(!gravity);slides++;for(int i=0;i<3;i++)pm->ps->origin[i]+=pm->ps->velocity[i]*pml->frametime;float into=PM_DotProduct(pm->ps->velocity,pm->ps->vLadderVec);for(int i=0;i<3;i++)pm->ps->velocity[i]-=into*pm->ps->vLadderVec[i];}
static void PM_SetMovementDir(pmove_t*pm,pml_t*pml){}
'''
body=''.join(function(n) for n in ['PM_Accelerate','PM_Friction','PM_CheckLadderMove','PM_LadderMove'])
checks=r'''
int main(void){
 for(int facing=0;facing<4;facing++)for(int step=1;step<=66;step++){
  playerState_t ps={.speed=190};pmove_t pm={.ps=&ps,.tracemask=0x28010011,.cmd={.serverTime=1000,.forwardmove=127}};
  pml_t pml={.frametime=step*.001f,.forward={1,0,0},.right={0,-1,0}};
  float yaw=facing*1.570796327f;pml.forward[0]=cosf(yaw);pml.forward[1]=sinf(yaw);
  hit=(trace_t){.fraction=.5f,.surfaceFlags=8,.normal={-pml.forward[0],-pml.forward[1],0}};
  PM_CheckLadderMove(&pm,&pml);assert(ps.pm_flags&PMF_LADDER);
  for(int i=0;i<2000/step;i++)PM_LadderMove(&pm,&pml);
  if(!(ps.origin[2]>140 && ps.origin[2]<210)){fprintf(stderr,"facing=%d step=%d z=%f vz=%f\n",facing,step,ps.origin[2],ps.velocity[2]);return 1;}
  float high=ps.origin[2];pm.cmd.forwardmove=0;
  for(int i=0;i<1000/step;i++)PM_LadderMove(&pm,&pml);
  assert(fabsf(ps.velocity[2])<.001f);assert(ps.origin[2]-high<12);
  high=ps.origin[2];pm.cmd.forwardmove=-127;
  for(int i=0;i<1000/step;i++)PM_LadderMove(&pm,&pml);
  assert(ps.origin[2]<high-50);
  pm.cmd.buttons=0x400;int old=airMoves;PM_LadderMove(&pm,&pml);assert(airMoves==old+1 && !(ps.pm_flags&PMF_LADDER));
 }
 playerState_t ps={0};pmove_t pm={.ps=&ps,.cmd={.serverTime=1000,.forwardmove=127}};pml_t pml={.forward={1,0,0}};
 for(int flags=0;flags<256;flags++){
  hit=(trace_t){.fraction=.5,.surfaceFlags=flags,.normal={-1,0,0}};ps.pm_flags=0;
  PM_CheckLadderMove(&pm,&pml);assert(!!(ps.pm_flags&PMF_LADDER)==!!(flags&8));
 }
 hit.surfaceFlags=8;
 ps.pm_flags=0;pm.tracemask=0x2810011;woodRung=1;int before=traces;PM_CheckLadderMove(&pm,&pml);assert((ps.pm_flags&PMF_LADDER) && traces==before+2);woodRung=0;
 for(int mode=1;mode<10;mode++){ps.pm_type=mode;ps.pm_flags=PMF_LADDER;PM_CheckLadderMove(&pm,&pml);assert(!(ps.pm_flags&PMF_LADDER));}ps.pm_type=0;
 for(int reason=0;reason<6;reason++){
  hit.fraction=reason==0?1:.5;hit.startsolid=reason==1;hit.normal[2]=reason==2?1:0;
  ps.pm_flags=reason==3?1:reason==4?2:reason==5?PMF_JUMPING:0;ps.jumpTime=900;
  PM_CheckLadderMove(&pm,&pml);assert(!(ps.pm_flags&PMF_LADDER));
 }
 assert(slides>10000);return 0;
}
'''
assert re.search(r'PM_GroundTrace\(pm, &pml\);\s+PM_CheckLadderMove\(pm, &pml\);', source)
with tempfile.TemporaryDirectory(prefix='cod2-ladder-') as folder:
 p=Path(folder);(p/'test.c').write_text(support+body+checks)
 subprocess.run(['cc','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-lm','-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: 264 climb/hold/descend/jump schedules, 256 surface flags, collision/mode/jump rejection under ASan/UBSan')
