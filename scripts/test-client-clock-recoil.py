#!/usr/bin/env python3
"""Exercise the real network clock and recoil integrator across frame rates."""
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
 for(int frame=1;frame<=100;frame++){
  float angle[3]={0},velocity[3]={-80,30,-15};
  for(int t=0;t<1000;t+=frame)CG_IntegrateViewKick(angle,velocity,1600,frame*.001f);
  for(int i=0;i<3;i++)assert(isfinite(angle[i])&&fabsf(angle[i])<.00001f);
  float a[3]={.4f,-.2f,.1f},v[3]={-80,30,-15},b[3],w[3];memcpy(b,a,12);memcpy(w,v,12);
  CG_IntegrateViewKick(a,v,1600,.1f);
  for(int i=0;i<10;i++)CG_IntegrateViewKick(b,w,1600,.01f);
  for(int i=0;i<3;i++){assert(fabsf(a[i]-b[i])<.00001f);assert(fabsf(v[i]-w[i])<.0001f);}
 }
 float a[3]={0},v[3]={-80,30,-15};CG_IntegrateViewKick(a,v,1600,.025f);
 assert(a[0]<-.5f&&a[1]>.2f); /* Aim receives a real, finite camera kick. */
 puts("PASS: 42 three-minute network clock schedules, monotonic time and FPS-independent recoil/return");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)
 code=support+function(clock,'CL_AdjustTimeDelta')+step+function(view,'CG_IntegrateViewKick')+checks
 for mutant in (False,True):
  (p/'test.c').write_text(code.replace('cl->serverTime >= serverTime - 5','cl->serverTime < serverTime - 5') if mutant else code)
  subprocess.run(['cc','-O1','-fsanitize=address,undefined',str(p/'test.c'),'-lm','-o',str(p/'test')],check=True)
  r=subprocess.run([str(p/'test')],capture_output=True,text=True)
  assert (r.returncode==0)!=mutant,r.stderr
  if not mutant:print(r.stdout,end='')
