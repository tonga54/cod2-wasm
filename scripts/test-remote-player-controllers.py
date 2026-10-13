#!/usr/bin/env python3
"""Respawn/teleport parity must not disable a living player's aim and lean."""
from pathlib import Path
import re,subprocess,tempfile
root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/bgame/bg_animation_mp.c').read_text()
def function(name):
 m=re.search(r'^[^\n;]*\b'+name+r'\([^;]*?\)\n\{',source,re.M);end=m.end();depth=1
 while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[m.start():end]+'\n'
support=r'''
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
typedef float vec_t;typedef float vec3_t[3];struct DObj_s {int unused;};
typedef struct {int eFlags;float fTorsoHeight,fTorsoPitch,fWaistPitch;}entityState_t;
typedef struct {struct {float yawAngle,pitchAngle;}legs,torso;vec3_t playerAngles,angles[6],tag_origin_angles,tag_origin_offset;int clientConditions[12][2];float lerpLean;}clientInfo_t;
typedef struct {unsigned short tag_origin;}scr_const_t;
static scr_const_t sc={6};static void *imp_scr_const=&sc;
static unsigned short names[]={0,1,2,3,4,5};
static void *controller_names[]={names,names+1,names+2,names+3,names+4,names+5};
static float AngleNormalize180(float a){while(a>180)a-=360;while(a<-180)a+=360;return a;}
static float AngleSubtract(float a,float b){return AngleNormalize180(a-b);}
static void AnglesSubtract(const float*a,const float*b,float*c){for(int i=0;i<3;i++)c[i]=AngleSubtract(a[i],b[i]);}
static float GetLeanFraction(float f){return (2-fabsf(f))*f;}
static float applied[7][3];
static void DObjSetControlTagAngles(const struct DObj_s *o,int *bits,unsigned int tag,float *a){memcpy(applied[tag],a,12);}
static void DObjSetLocalTag(const struct DObj_s *o,int *bits,unsigned int tag,const float*t,const float*a){memcpy(applied[tag],a,12);}
'''
body=''.join(function(n) for n in ('BG_LerpOffset','BG_SmoothControllerAngles','BG_ControllerTagName','BG_Player_DoControllers'))
checks=r'''
int main(void){int count=0;struct DObj_s obj={0};int bits[4]={0};
 for(int stance=0;stance<3;stance++)for(int pitch=-75;pitch<=75;pitch+=15)for(int lean=-1;lean<=1;lean++){
  entityState_t es={.eFlags=stance?1<<(stance+1):0};
  clientInfo_t neutral={.playerAngles={pitch,25,0},.legs.yawAngle=10,.torso={25,pitch*.6f},.lerpLean=lean};
  clientInfo_t teleported=neutral;float expected[7][3];
  BG_Player_DoControllers(&obj,&es,bits,&neutral,1000);memcpy(expected,applied,sizeof(expected));
  es.eFlags^=2;BG_Player_DoControllers(&obj,&es,bits,&teleported,1000);
  assert(!memcmp(expected,applied,sizeof(expected)));assert(!memcmp(&neutral,&teleported,sizeof(neutral)));
  if(pitch)assert(fabsf(applied[0][0])+fabsf(applied[1][0])+fabsf(applied[2][0])+fabsf(applied[3][0])+fabsf(applied[4][0])>1);
  if(lean)assert(fabsf(applied[0][1])+fabsf(applied[0][2])+fabsf(applied[6][2])>1);
  es.eFlags|=1;BG_Player_DoControllers(&obj,&es,bits,&teleported,1000);
  for(int i=0;i<7;i++)for(int j=0;j<3;j++)assert(applied[i][j]==0);count++;
 }
 printf("PASS: %d standing/crouched/prone pitch and lean combinations survive respawn/teleport parity; dead poses reset\n",count);
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)
 for mutant in (False,True):
  code=body.replace('(es->eFlags & 0x1)', '(es->eFlags & 0x3)') if mutant else body
  (p/'test.c').write_text(support+code+checks)
  subprocess.run(['cc','-fsanitize=address,undefined',str(p/'test.c'),'-lm','-o',str(p/'test')],check=True)
  r=subprocess.run([str(p/'test')],capture_output=True,text=True)
  assert (r.returncode==0)!=mutant,r.stderr
  if not mutant:print(r.stdout.strip())
assert '(eFlags & 0x3)' not in function('BG_PlayerAnimation')
print('PASS: former teleport-as-dead mask is rejected')
