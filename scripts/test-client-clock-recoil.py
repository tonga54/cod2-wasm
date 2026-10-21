#!/usr/bin/env python3
"""Exercise the real network clock across frame rates, and the camera kick as
the original game integrates it: bursts climb, the sight returns to centre."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parent.parent
clock=(root/'src/PC/client_mp/cl_cgame_mp.c').read_text()
view=(root/'src/PC/cgame_mp/cg_view_mp.c').read_text()
def function(s,name):
 m=re.search(r'[^\n;]*\b'+name+r'\([^;]*?\)\n\{',s); end=m.end(); depth=1
 while depth:
  depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[m.start():end]+'\n'
a=clock.index('            int timeDelta = cl->serverTimeDelta;')
b=clock.index('\n        } else {',a)
step='static void StepClock(void){clientActive_t *cl=CL_LOCAL;clientStatic_t *cls=CLS;'+clock[a:b]+'\nif(cl->newSnapshots)CL_AdjustTimeDelta();}\n'
support=r'''
#define __EMSCRIPTEN__ 1
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {int newSnapshots,serverTime,oldServerTime,oldSnapServerTime,serverTimeDelta,extrapolatedSnapshot;struct{int serverTime;}snap;}clientActive_t;
typedef struct{int demoplaying;}clientConnection_t;
typedef struct{int realtime;}clientStatic_t;
static clientActive_t client;static clientConnection_t conn;static clientStatic_t clsStorage;
#define CL_LOCAL (&client)
#define CLUI_STATE (&conn)
#define CLS (&clsStorage)
typedef struct{struct{int enabled;}current;}dvar_t;
static dvar_t zero;static dvar_t *zerop=&zero;static void *imp_cl_showTimeDelta=&zerop;
static int CL_DvarCurrentBool(void*p){return 0;}
static float CL_ComTimescaleValue(void){return 1;}
static void Com_Printf(const char*f,...){}
typedef float vec_t;typedef vec_t vec3_t[3];
typedef struct{int pm_type,pm_flags;float fWeaponPosFrac;}playerState_t;
typedef struct{float fAdsViewKickCenterSpeed,fHipViewKickCenterSpeed;}WeaponDef;
typedef struct{playerState_t predictedPlayerState;vec3_t kickAVel,kickAngles,offsetAngles;int frametime,weapIdleTime;
 struct{float fLastIdleFactor;}playerEntity;}cg_t;
typedef struct{playerState_t *ps;int damageTime,time;float v_dmg_pitch,v_dmg_roll,xyspeed,frametime,fLastIdleFactor;
 int *weapIdleTime;}viewState_t;
static cg_t cgState;static cg_t *const cg=&cgState;
/* None in hand centres nothing; the rifle's sights centre ten times faster. */
static WeaponDef defs[2]={{0,0},{1000,100}};static int viewWeapon;
static int BG_GetViewmodelWeaponIndex(void*ps){assert(ps==&cg->predictedPlayerState);return viewWeapon;}
static void *BG_GetWeaponDef(int i){assert(i>=0&&i<2);return &defs[i];}
static vec3_t sent;
static void CL_SetUserCmdAimValues(vec_t*a){memcpy(sent,a,sizeof(sent));}
/* The scope's idle sway, from the frame's prediction, phase and time. */
static const vec3_t idleSway={.0625f,-.125f,0};static int idleCalls;
static void BG_CalculateViewIdleAngles(viewState_t*vs,vec_t*a){
 assert(vs->ps==&cg->predictedPlayerState&&vs->weapIdleTime==&cg->weapIdleTime);
 assert(vs->frametime==cg->frametime*.001f&&vs->fLastIdleFactor==cg->playerEntity.fLastIdleFactor);
 idleCalls++;memcpy(a,idleSway,sizeof(vec3_t));}
'''
checks=r'''
int main(void){
 for(int frame=8;frame<=50;frame+=7)for(int latency=0;latency<=150;latency+=30){
  memset(&client,0,sizeof(client));clsStorage.realtime=0;
  client.snap.serverTime=10000;client.oldSnapServerTime=9950;
  client.serverTimeDelta=10000-latency-55;
  client.serverTime=client.oldServerTime=10000-latency-55;
  int nextArrival=latency+50;
  for(int now=frame;now<=180000;now+=frame){
   clsStorage.realtime=now;
   if(now>=nextArrival){
    client.oldSnapServerTime=client.snap.serverTime;
    client.snap.serverTime=10000+((now-latency)/50)*50;
    client.newSnapshots=1;nextArrival+=50+(now/50%3-1)*3;
   }
   int prev=client.serverTime;StepClock();
   assert(client.serverTime>=prev);
   assert(abs(client.serverTimeDelta-(10000-latency-55))<150);
  }
 }
 /* Both a paused server and queued, stale snapshots can trigger a clock
  * rebase. Commands must stay monotonic and within the server's horizon. */
 const int delays[]={120,650,1800};
 for(int frame=8;frame<=50;frame+=7)for(int kind=0;kind<2;kind++)for(int d=0;d<3;d++){
  memset(&client,0,sizeof(client));clsStorage.realtime=0;
  client.snap.serverTime=10000;client.oldSnapServerTime=9950;
  client.serverTimeDelta=9945;client.serverTime=client.oldServerTime=9945;
  int lastArrival=0,serverClock=0,delay=delays[d];
  for(int now=frame;now<=6000;now+=frame){
   clsStorage.realtime=now;
   int stalled=now>=1500&&now<1500+delay;
   serverClock=now<1500?now:(stalled?1500:now-delay+(kind==0?(delay<250?delay:250):0));
   if(!stalled&&now-lastArrival>=50){
    client.oldSnapServerTime=client.snap.serverTime;
    client.snap.serverTime=10000+serverClock;
    client.newSnapshots=1;lastArrival=now;
   }
   int previous=client.serverTime;StepClock();
   assert(client.serverTime>=previous);
   assert(client.serverTime<=client.snap.serverTime+200);
  }
  assert(abs(client.serverTime-(10000+serverClock))<200);
 }
 /* One kick goes out against the centring speed to about v*v/(2c), then
  * back at a fraction of it, to exactly 0 within a second, at any frame
  * rate; frames of whole steps take the same path, others stay close. */
 for(int frame=1;frame<=100;frame++){
  float angle[3]={0},velocity[3]={-80,30,-15},ref[3]={0},refV[3]={-80,30,-15},peak=0;int peakTime=0,back=-1,done=0;
  for(int t=frame;t<=1000;t+=frame){
   CG_IntegrateViewKick(angle,velocity,1600,frame);
   for(;done+5<=t;done+=5)CG_IntegrateViewKick(ref,refV,1600,5);
   if(-angle[0]>peak){peak=-angle[0];peakTime=t;}
   if(back<0&&peak>0&&angle[0]==0)back=t;
   for(int i=0;i<3;i++){
    assert(fabsf(angle[i])<=10);
    if(done==t&&frame%5==0)assert(angle[i]==ref[i]&&velocity[i]==refV[i]);
    if(done==t)assert(fabsf(angle[i]-ref[i])<.25f);
   }
  }
  for(int i=0;i<3;i++)assert(angle[i]==0&&velocity[i]==0);
  assert(peak>1.95f&&peak<2.3f&&peakTime>=40&&peakTime<=55+frame);
  /* It went out in about 50 ms and takes several times as long back. */
  assert(back>=peakTime+150&&back<=peakTime+300);
 }
 /* A burst climbs: ten kicks 86 ms apart, as a Thompson fires, leave the
  * sight more than 5 degrees up, then it settles back at the centre. */
 {
  float a[3]={0},v[3]={0};
  for(int t=0,shots=0;t<1860;t+=16){
   if(shots<10&&t>=shots*86){v[0]=-65;v[1]=v[2]=0;shots++;}
   CG_IntegrateViewKick(a,v,1800,16);
   if(t==864)assert(a[0]<-5&&a[0]>=-10);
  }
  assert(a[0]==0&&v[0]==0);
 }
 /* None passes 10 degrees: past it, an angle is held there and returns. */
 {
  float a[3]={0},v[3]={-3000,3000,0};
  CG_IntegrateViewKick(a,v,1600,5);
  assert(a[0]==-10&&a[1]==10&&v[0]==0&&v[1]==0);
  CG_IntegrateViewKick(a,v,1600,45);
  assert(a[0]>-10&&a[0]<-9.8f&&a[1]<10&&a[1]>9.8f);
  CG_IntegrateViewKick(a,v,1600,1000);
  assert(a[0]==0&&a[1]==0&&v[0]==0&&v[1]==0);
 }
 /* An angle crossing the centre stops there instead of swinging past. */
 {
  float a[3]={.01f,0,0},v[3]={-100,0,0};
  CG_IntegrateViewKick(a,v,1600,5);
  assert(a[0]==0&&v[0]==0);
 }
 /* The frame's kick takes the sights' centring from halfway into them, the
  * hip's before, a fixed one with nothing in hand; dead or watching, none.
  * The command aims where the kick and the offset point. */
 {
  float peaks[3];
  const float fracs[3]={.6f,.5f,1};const int weapons[3]={1,1,0};
  for(int c=0;c<3;c++){
   memset(&cgState,0,sizeof(cgState));
   cg->predictedPlayerState.fWeaponPosFrac=fracs[c];viewWeapon=weapons[c];
   cg->kickAVel[0]=-80;cg->frametime=16;peaks[c]=0;
   for(int f=0;f<30;f++){CG_UpdateViewKick();if(-cg->kickAngles[0]>peaks[c])peaks[c]=-cg->kickAngles[0];}
  }
  assert(peaks[0]>2.8f&&peaks[0]<3.8f);
  assert(peaks[1]>9.99f&&peaks[1]<=10);
  assert(peaks[2]>1.2f&&peaks[2]<1.7f);
  memset(&cgState,0,sizeof(cgState));viewWeapon=1;
  cg->kickAVel[0]=-80;cg->frametime=16;cg->offsetAngles[0]=1;cg->offsetAngles[1]=2;cg->offsetAngles[2]=3;
  cg->playerEntity.fLastIdleFactor=.75f;
  CG_UpdateViewKick();
  for(int i=0;i<3;i++)assert(sent[i]==cg->kickAngles[i]+cg->offsetAngles[i]+idleSway[i]);
  assert(cg->kickAngles[0]<0);
  /* Following another player, whose sway is in their view angles, or dead:
   * no sway of the player's own; dead, no kick either. */
  int calls=idleCalls;
  cg->predictedPlayerState.pm_flags=0x400000;
  CG_UpdateViewKick();
  for(int i=0;i<3;i++)assert(sent[i]==cg->kickAngles[i]+cg->offsetAngles[i]);
  assert(idleCalls==calls);
  cg->predictedPlayerState.pm_flags=0;
  CG_UpdateViewKick();
  assert(idleCalls==calls+1);
  cg->predictedPlayerState.pm_type=5;
  CG_UpdateViewKick();
  for(int i=0;i<3;i++)assert(cg->kickAngles[i]==0&&cg->kickAVel[i]==0&&sent[i]==cg->offsetAngles[i]);
 }
 puts("PASS: 42 three-minute clocks and 42 delayed-update recoveries stay monotonic within the server horizon; the original camera kick: out fast and back slowly, bursts climbing, 10 degrees at most, stopping at the centre, the sights' centring from halfway in, the same at any frame rate; the command carries the kick and the scope's sway, none of it following or dead");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)
 kick=''.join(line+'\n' for line in view.splitlines() if line.startswith('#define CG_KICK_'))
 sway=re.search(r'^static vec3_t s_scopeSway;\n',view,re.M)[0]
 code=(support+function(clock,'CL_AdjustTimeDelta')+step+kick+function(view,'CG_IntegrateViewKick')+sway+
       function(view,'CG_UpdateScopeSway')+function(view,'CG_SetUserCmdAim')+function(view,'CG_UpdateViewKick')+checks)
 reset='''        if (cl->serverTime < cl->snap.serverTime)
            cl->serverTime = cl->snap.serverTime;
        cl->oldServerTime = cl->serverTime;'''
 horizon='''            if (cl->serverTime > cl->snap.serverTime + 200)
                cl->serverTime = cl->snap.serverTime + 200;'''
 variants=[code,
  code.replace('cl->serverTime >= serverTime - 5','cl->serverTime < serverTime - 5'),
  code.replace(reset,'cl->serverTime = cl->oldServerTime = cl->snap.serverTime;'),
  code.replace(horizon,''),
  code.replace('                change *= CG_KICK_RETURN;','                ;'),
  code.replace('fabsf(next) > CG_KICK_MAX','0'),
  code.replace('if (next * angles[i] < 0.0f || next == 0.0f) {','if (next == 0.0f) {'),
  code.replace('#define CG_KICK_STEP_MSEC 5','#define CG_KICK_STEP_MSEC 10'),
  code.replace('if (angles[i] != 0.0f)\n                velocity[i] +=','if (0)\n                velocity[i] +='),
  code.replace('ps->fWeaponPosFrac > 0.5f','ps->fWeaponPosFrac == 1.0f'),
  code.replace('!weaponIndex ? CG_KICK_CENTER_SPEED','0 ? CG_KICK_CENTER_SPEED'),
  code.replace(' + s_scopeSway[i];',';'),
  code.replace('if (ps->pm_flags & 0x400000)','if (0)'),
  code.replace('    vs.fLastIdleFactor = cg->playerEntity.fLastIdleFactor;\n','')]
 assert all(v!=code for v in variants[1:])
 for mutant,variant in enumerate(variants):
  (p/'test.c').write_text(variant)
  subprocess.run(['cc','-O1','-fsanitize=address,undefined',str(p/'test.c'),'-lm','-o',str(p/'test')],check=True)
  r=subprocess.run([str(p/'test')],capture_output=True,text=True)
  assert (r.returncode==0)==(mutant==0),r.stderr
  if not mutant:print(r.stdout,end='')
