#!/usr/bin/env python3
"""Exercise actual sound channel indexing and lifetime/position metadata."""
from pathlib import Path
import re
import subprocess
import tempfile
root=Path(__file__).resolve().parent.parent
def function(source,name):
    match=re.search(r'(?:static )?[^\n;]*\b'+name+r'\([^;]*?\)\n\{',source)
    end=match.end();depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[match.start():end]+'\n'
driver=(root/'src/PC/win32/snd_driver.c').read_text()
snd=(root/'src/PC/snd.c').read_text()
body=(function(driver,'SND_2DHandle')+function(snd,'SND_SetChannelInfo')+
      function(snd,'SND_GetCurrent3DPosition')+function(snd,'SND_Attenuate')+
      function(driver,'MSS_SpatializeStreamImpl'))
support=r'''
#include <assert.h>
#include <string.h>
#include <math.h>
typedef float vec_t;typedef float vec3_t[3];typedef int snd_alias_system_t;typedef void *HSAMPLE;
typedef struct {int unused;} SndCurve;
typedef struct {int flags;float fDistMin,fDistMax;SndCurve *volumeFalloffCurve;} snd_alias_t;
typedef struct {int entnum,entchannel,startDelay,looptime,endtime;float basevolume;int baserate;float pitch;int srcChannelCount;const snd_alias_t *pAlias0,*pAlias1;float lerp;vec3_t org,offset;int paused,master,system;} snd_channel_info_t;
typedef struct {struct {vec3_t origin,axis[3];} orient;} snd_listener;
typedef struct {snd_channel_info_t chaninfo[53];int time,looptime,paused,pauseSettings[11];snd_listener listeners[1];} snd_local_t;
static snd_local_t g_snd;
static HSAMPLE handle_2D_004a3ad4[8];
static int SND_GetListenerIndexNearestToOrigin(const float *org){return 0;}
static float Vec3Normalize(float *v){float d=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(d)for(int i=0;i<3;i++)v[i]/=d;return d;}
static float Com_GetVolumeFalloffCurveValue(SndCurve *curve,float fraction){return 1-fraction;}
static void CG_GetEntityOrientation(int entnum,vec_t *org,vec3_t *axis){
 memset(axis,0,sizeof(vec3_t)*3);axis[0][1]=1;axis[1][0]=-1;axis[2][2]=1;
 org[0]=100;org[1]=-50;org[2]=20;
}
'''
checks=r'''
int main(void){
 for(int i=0;i<8;i++)handle_2D_004a3ad4[i]=&handle_2D_004a3ad4[i];
 for(int i=-1;i<=53;i++)assert(SND_2DHandle(i)==(i>=45 && i<53?handle_2D_004a3ad4[i-45]:NULL));
 snd_alias_t alias={.flags=9<<7};vec3_t origin={250,200,70},position;
 for(int pause=0;pause<2;pause++)for(int i=0;i<53;i++){
  g_snd.time=1000;g_snd.looptime=950;g_snd.paused=pause;g_snd.pauseSettings[9]=1;
  SND_SetChannelInfo(i,3,&alias,&alias,.3f,origin,.6f,1.2f,1,44100,700,50,10,1,1);
  snd_channel_info_t *s=&g_snd.chaninfo[i];
  assert(s->pAlias0==&alias && s->pAlias1==&alias && s->entchannel==9 && s->entnum==3);
  assert(s->paused==pause && s->startDelay==10 && s->endtime==1650 && s->looptime==950);
  assert(s->basevolume==.6f && s->baserate==44100 && s->pitch==1.2f && s->master && s->system==1);
  SND_GetCurrent3DPosition(3,s->offset,position);
  for(int n=0;n<3;n++)assert(fabsf(position[n]-origin[n])<.001f);
 }
 SND_SetChannelInfo(-1,0,&alias,&alias,0,NULL,1,1,1,1,1,0,0,0,0);
 SND_SetChannelInfo(53,0,&alias,&alias,0,NULL,1,1,1,1,1,0,0,0,0);
 alias.fDistMin=100;alias.fDistMax=1100;
 for(int channel=32;channel<45;channel++)for(int rotation=0;rotation<8;rotation++)for(int side=-1;side<=1;side++)for(int d=0;d<=1200;d+=200){
  float angle=rotation*.785398163f, left[3]={-sinf(angle),cosf(angle),0};
  for(int n=0;n<3;n++){
   g_snd.listeners[0].orient.axis[1][n]=left[n];
   g_snd.chaninfo[channel].org[n]=side?side*d*left[n]:(n==2?d:0);
  }
  g_snd.chaninfo[channel].pAlias0=g_snd.chaninfo[channel].pAlias1=&alias;
  g_snd.chaninfo[channel].lerp=0;
  float volume=.7f,pan=-1;
  MSS_SpatializeStreamImpl(channel,&volume,&pan);
  float expected=.7f*fmaxf(0,fminf(1,1-(d-100)/1000.0f));
  assert(fabsf(volume-expected)<.0001f);
  assert(fabsf(pan-(d?(1-side)*.5f:.5f))<.0001f);
 }
 return 0;
}
'''
assert 'imp_g_snd' not in driver, 'sound state is a structure, not a pointer variable'
assert 'MSS_SpatializeStreamImpl(streamIdx,' not in driver, 'stream position uses global channel id'
assert 'SND_GetCurrent3DPosition(ci->entnum, ci->offset, org);' in driver
with tempfile.TemporaryDirectory(prefix='cod2-sound-channels-') as folder:
    path=Path(folder);(path/'test.c').write_text(support+body+checks)
    subprocess.run(['cc','-O1','-g','-fsanitize=address,undefined',str(path/'test.c'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
print('PASS: 2D ids 45..52, all 53 channel lifetimes/positions, 2184 spatial stream positions/distances under ASan/UBSan')
