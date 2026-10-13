#!/usr/bin/env python3
"""Replay archived player/world clocks through deaths at multiple tick rates."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/server_mp/sv_snapshot_mp.c').read_text()


def function(name):
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


support = r'''
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
typedef int qboolean;typedef unsigned char byte;
typedef struct {int time,fadeStartTime,scaleStartTime,moveStartTime;} hudelem_t;
typedef struct {int commandTime,pm_time,foliageSoundTime,jumpTime,legsTimer,adsDelayTime,
 viewHeightLerpTime,shellshockTime,deltaTime;float origin[3];struct {hudelem_t archival[31];}hud;}playerState_t;
typedef struct {int number;}clientState_t;
typedef struct {int active;clientState_t cs;playerState_t ps;}cachedClient_t;
typedef struct {int time,num_clients,first_client;}cachedSnapshot_t;
typedef struct {int state;}client_t;
typedef struct {int archiveEnabled,nextArchivedSnapshotFrames,time;cachedClient_t *cachedSnapshotClients;client_t clients[64];}serverStatic_t;
typedef struct {struct {int integer;}current;}dvar_t;
static serverStatic_t svs;static void *imp_svs=&svs;
static cachedSnapshot_t frames[5000];static int missing[5000];static cachedClient_t clients[4096];
static dvar_t fps;static dvar_t *fpsPtr=&fps;static byte *sv_fps_dvar=(byte*)&fpsPtr;
static int delay=9000,probes;
#define CACHEDCLIENT_STRIDE sizeof(cachedClient_t)
static cachedSnapshot_t *SV_GetCachedSnapshotInternal(int frame){
 probes++;int oldest=svs.nextArchivedSnapshotFrames-1200;if(oldest<0)oldest=0;
 return frame<oldest||frame>=svs.nextArchivedSnapshotFrames||missing[frame]?NULL:&frames[frame];
}
static int G_GetClientArchiveTime(int n){assert(n==0);return delay;}
static int GetFollowPlayerState(int n,byte *ps){assert(0);return 0;}
static clientState_t *G_GetClientState(int n){assert(0);return NULL;}
'''
body = ''.join(function(name) for name in ('SV_FindArchivedSnapshotAtTime',
                                          'SV_GetArchivedClientInfo',
                                          'SV_FindArchivedSnapshotForClientLocal'))
checks = r'''
static void store(int time){
 int frame=svs.nextArchivedSnapshotFrames++;frames[frame]=(cachedSnapshot_t){time,1,frame%4096};
 clients[frame%4096]=(cachedClient_t){.active=1,.cs={0},.ps={.commandTime=time,.origin={(float)time},
  .pm_time=200,.legsTimer=300,.jumpTime=time-100,.shellshockTime=time-50}};
}
static int oracle(int age){
 int oldest=svs.nextArchivedSnapshotFrames-1200;if(oldest<0)oldest=0;int first=-1,last=-1;
 for(int i=oldest;i<svs.nextArchivedSnapshotFrames;i++)if(!missing[i]){
  if(first<0)first=i;if(frames[i].time<=svs.time-age)last=i;
 }
 return last>=0?last:first;
}
int main(void){int checks=0;
 for(int mode=0;mode<5;mode++){
  svs=(serverStatic_t){.archiveEnabled=1,.cachedSnapshotClients=clients};memset(missing,0,sizeof(missing));
  fps.current.integer=(int[]){20,30,60,30,30}[mode];int tick=1000/fps.current.integer;
  for(int now=tick;now<32000;now+=tick){
   if(mode==4&&now>18000){tick=50;fps.current.integer=20;}
   svs.time=now;
   if(mode!=3||now/tick%7)store(now);
   if(now<22000||now>31000)continue;delay=9000;
   int expected=oracle(delay),age=delay;playerState_t ps;clientState_t cs;
   probes=0;assert(SV_GetArchivedClientInfo(0,&age,(int(*)[4])&ps,(void(*)())&cs));
   assert(ps.origin[0]==frames[expected].time&&age==now-frames[expected].time);
   assert(ps.commandTime==now&&ps.deltaTime==age&&ps.pm_time==200&&ps.legsTimer==300);
   int adjusted=0;cachedSnapshot_t *world=SV_FindArchivedSnapshotForClientLocal(0,&adjusted);
   assert(world==&frames[expected]&&adjusted==9000&&probes<50);
   if(mode<3)assert(probes<=8); /* Constant-rate history stays on adjacent probes. */
   /* Death at 20s: replay starts about seven seconds before it and includes
    * both the killing frame and two seconds after it before the script ends. */
   assert(ps.origin[0]<=now-delay&&ps.origin[0]>now-delay-2*tick);
   if(now>=29000+2*tick)assert(ps.origin[0]>=20000);
   if(now>=30900)assert(ps.origin[0]>21800);
   checks++;
  }
  /* Trim unavailable startup history once, using its actual first timestamp. */
  delay=50000;int adjusted=0;int first=oracle(delay);
  assert(SV_FindArchivedSnapshotForClientLocal(0,&adjusted)==&frames[first]);
  assert(adjusted==svs.time-frames[first].time);
  /* Missing cached entries cannot select a later replay frame. */
  delay=1000;int target=oracle(delay);missing[target]=1;int expected=oracle(delay);
  assert(SV_FindArchivedSnapshotForClientLocal(0,&adjusted)==&frames[expected]&&adjusted==delay);
 }
 memset(missing,1,sizeof(missing));assert(!SV_FindArchivedSnapshotAtTime(9000));
 svs.archiveEnabled=0;int age=9000;playerState_t ps;clientState_t cs;
 assert(!SV_GetArchivedClientInfo(0,&age,(int(*)[4])&ps,(void(*)())&cs));
 printf("PASS: %d killcam player/world frames at 20/30/60 Hz, archive gaps, changed tick rate, death/endpoints, startup trimming and unchanged animation durations\n",checks);
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-killcam-timing-') as directory:
    path = Path(directory)
    variants = (body,
                body.replace('sample->time <= targetTime', 'sample->time < targetTime - 2000'),
                body.replace('*archiveTime = requestedArchiveTime < availableAge ? requestedArchiveTime : availableAge;', '*archiveTime = availableAge;'),
                body.replace('            if (pps->foliageSoundTime != 0)', '            pps->pm_time += deltaTime; pps->legsTimer += deltaTime;\n            if (pps->foliageSoundTime != 0)'))
    for index, code in enumerate(variants):
        (path / 'test.c').write_text(support + code + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(path / 'test.c'), '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True, timeout=30)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr)
        if index == 0:
            print(result.stdout.strip())
print('PASS: shifted replay, rounded-delay feedback and prolonged-animation mutations fail')
