#!/usr/bin/env python3
"""Regression: muzzle tails transform once; ADS uses the predicted pose clock."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parent.parent
fx=(root/'src/PC/EffectsCore/FxPrimitives.c').read_text()
weapon=(root/'src/PC/cgame_mp/cg_weapons.c').read_text()
def function(source,name):
 m=re.search(r'^[^\n;]*\b'+name+r'\([^;]*?\)\n\{',source,re.M);end=m.end();depth=1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[m.start():end]
calc=function(fx,'Tail_CalcNewEndpoint')
update=function(fx,'Tail_Update')
start=update.index('    Tail_CalcNewEndpoint(');end=update.index('    float radius;',start)
step='void Step(Tail *_this,const orientation_t *orient){byte *self=(byte *)_this;'+update[start:end]+'}'
ads=function(weapon,'CG_PlayADSAnim_impl')
local=function(weapon,'CG_IsLocalClientEntity')
support=r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef int qboolean;
typedef struct {int number;} entityState_t;
typedef struct {struct {int clientNum,pm_flags;}ps;} snapshot_t;
typedef struct {snapshot_t *nextSnap;int renderingThirdPerson;}cg_t;
typedef unsigned char byte;typedef float vec_t;typedef float vec3_t[3];
typedef struct {float origin[3],axis[3][3];} orientation_t;
typedef struct {float origin[3];struct {float endpos[3];}mRefEnt;} Effect;
typedef struct {struct {Effect base;}base;float endpoint[3],tailLength;}Tail;
#define PART_ANCHOR_X(p) (((Effect *)(p))->origin[0])
#define PART_ANCHOR_Y(p) (((Effect *)(p))->origin[1])
#define PART_ANCHOR_Z(p) (((Effect *)(p))->origin[2])
static void OrientationPosToWorldPos(void *raw,float *p,float *out){
 orientation_t *o=raw;float t[3];for(int i=0;i<3;i++)t[i]=o->origin[i]+p[0]*o->axis[0][i]+p[1]*o->axis[1][i]+p[2]*o->axis[2][i];memcpy(out,t,sizeof(t));
}
static struct {struct {float fWeaponPosFrac;}predictedPlayerState;}state,*cg=&state;
static float weights[23],times[23];
static void XAnimSetGoalWeight(void *tree,int i,float weight,float duration,float rate,int n,int type,int restart){assert(duration==0&&rate==0);weights[i]=weight;}
static void XAnimSetTime(void *tree,int i,float time){times[i]=time;}
'''
checks=r'''
int main(void){
 snapshot_t snapshot={.ps.clientNum=7};cg_t view={.nextSnap=&snapshot};entityState_t ent={.number=7};
 for(int flags=0;flags<4;flags++)for(int third=0;third<2;third++)for(int remote=0;remote<2;remote++){
  snapshot.ps.pm_flags=flags*0x400000;view.renderingThirdPerson=third;ent.number=remote?8:7;
  assert(CG_IsLocalClientEntity(&view,&ent)==(!third&&!remote));
 }
 view.nextSnap=NULL;assert(!CG_IsLocalClientEntity(&view,&ent));
 for(int yaw=-180;yaw<=180;yaw+=15){
  float a=yaw*.01745329252f;
  orientation_t o={{1300,-2500,270},{{cosf(a),sinf(a),0},{-sinf(a),cosf(a),0},{0,0,1}}};
  Tail t={.base.base.origin={2,0,0},.endpoint={-1,0,0},.tailLength=6};
  Step(&t,&o);float local[]={-4,0,0},expected[3];OrientationPosToWorldPos(&o,local,expected);
  for(int i=0;i<3;i++)assert(fabsf(t.base.base.mRefEnt.endpos[i]-expected[i])<.001f);
  Step(&t,NULL);assert(t.base.base.mRefEnt.endpos[0]==-4);
 }
 for(int i=0;i<=100;i++)for(int dir=0;dir<2;dir++){
  float f=i/100.f;cg->predictedPlayerState.fWeaponPosFrac=f;CG_PlayADSAnim_impl(NULL,dir?21:22);
  assert(weights[21]+weights[22]==1);assert(weights[dir?21:22]==1);
  assert(fabsf(times[21]-f)<1e-6f&&fabsf(times[22]-(1-f))<1e-6f);
 }
 puts("PASS: 25 muzzle orientations at map coordinates and 202 ADS poses/reversals; no double transform or delayed pose blend");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(support+calc+step+ads+local+checks)
 subprocess.run(['cc','-fsanitize=address,undefined',str(p/'test.c'),'-lm','-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
