#!/usr/bin/env python3
"""Regression for server history sampling and finite anti-lag positions."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/server_mp/sv_snapshot_mp.c').read_text()
def function(name):
    start = source.index(name + '(')
    while source.find(';', start) < source.find('{', start):
        start = source.index(name + '(', start + len(name))
    start = source.rfind('\n', 0, start) + 1
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'

support = r'''
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <string.h>
typedef int qboolean;typedef unsigned char Bool,byte;typedef float vec_t;
typedef struct { int time,fadeStartTime,scaleStartTime,moveStartTime; } hudelem_t;
typedef struct { int commandTime,pm_time,foliageSoundTime,jumpTime,legsTimer,adsDelayTime,viewHeightLerpTime,shellshockTime,deltaTime;
 float origin[3];struct { hudelem_t archival[31]; } hud; } playerState_t;
typedef struct { int number; } clientState_t;
typedef struct { int active;clientState_t cs;playerState_t ps; } cachedClient_t;
typedef struct { int state; } client_t;
typedef struct { int time,num_clients,first_client; } cachedSnapshot_t;
typedef struct { int archiveEnabled,nextArchivedSnapshotFrames,time;cachedClient_t *cachedSnapshotClients;client_t clients[64]; } serverStatic_t;
typedef struct { struct { int integer; } current; } dvar_t;
static dvar_t fps={.current.integer=20},bandwidth={.current.integer=30000};
static dvar_t *fpsptr=&fps,*rateptr=&bandwidth;
static byte *sv_fps_dvar=(byte*)&fpsptr,*sv_maxRate_dvar=(byte*)&rateptr;
static serverStatic_t svs;static void *imp_svs=&svs;
static cachedClient_t cache[4096];static cachedSnapshot_t snap;
static int lastFrame;
#define CACHEDCLIENT_STRIDE sizeof(cachedClient_t)
static cachedSnapshot_t *SV_GetCachedSnapshotInternal(int frame) { lastFrame=frame;return &snap; }
static int GetFollowPlayerState(int n,byte *ps) { memcpy(ps,&cache[n].ps,sizeof(playerState_t));return 1; }
static clientState_t *G_GetClientState(int n) { return &cache[n].cs; }
'''
archive_check = r'''
static void checkArchive(void) {
 svs.archiveEnabled=1;svs.nextArchivedSnapshotFrames=100;svs.time=5000;svs.cachedSnapshotClients=cache;
 snap.time=4850;snap.num_clients=64;snap.first_client=0;
 for(int n=0;n<64;n++) {cache[n].active=1;cache[n].cs.number=n;cache[n].ps.origin[0]=n*3.25f;}
 for(int n=0;n<64;n++) {
  playerState_t ps;clientState_t cs;int age=100;
  assert(SV_GetArchivedClientInfo(n,&age,(int(*)[4])&ps,(void(*)())&cs));
  assert(lastFrame==98);assert(age==150);assert(cs.number==n);assert(ps.origin[0]==n*3.25f);
 }
}
'''
sampling = r'''
static int mode;
static int sampleHistory(int n,int *age,int (*p)[4],void (*c)()) {
 playerState_t *ps=(playerState_t*)p;
 if(mode==1)*age=100;
 if(mode==2 && *age<100)return 0;
 if(mode==3 && *age>50)return 0;
 if(mode==4)return 0;
 if(mode==5 && (*age==50||*age==100))return 0;
 if(*age<0)*age=0;
 for(int a=0;a<3;a++)ps->origin[a]=(5000-*age)*.25f+a*13+n;
 return 1;
}
'''
checks = r'''
int main(void) {
 checkArchive();float p[3];svs.time=5000;
 for(int n=0;n<64;n++)for(int age=0;age<500;age++) {
  mode=0;assert(SV_GetClientPositionAtTime(n,5000-age,p));
  for(int a=0;a<3;a++)assert(isfinite(p[a])&&fabsf(p[a]-((5000-age)*.25f+a*13+n))<.001f);
 }
 for(int n=0;n<64;n++) {
  for(mode=1;mode<=5;mode++) {
   int result=SV_GetClientPositionAtTime(n,4925,p);
   if(mode==4){assert(!result);continue;}
   assert(result);for(int a=0;a<3;a++)assert(isfinite(p[a]));
   float age=mode==1||mode==2?100:mode==3?50:75;
   assert(fabsf(p[0]-((5000-age)*.25f+n))<.001f);
  }
 }
 fps.current.integer=0;assert(!SV_GetClientPositionAtTime(0,4900,p));
 return 0;
}
'''
archive = function('SV_GetArchivedClientInfo')
sample = function('SV_GetClientPositionAtTime').replace('SV_GetArchivedClientInfo(', 'sampleHistory(')
with tempfile.TemporaryDirectory(prefix='cod2-antilag-') as directory:
    path = Path(directory)
    variants = [(archive,sample),
                (archive.replace('(sv_fps_dvar)', '(sv_maxRate_dvar)'),sample),
                (archive,sample.replace(' || startTime == endTime', ''))]
    for index,(a,s) in enumerate(variants):
        (path/'test.c').write_text(support+a+archive_check+sampling+s+checks)
        subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(path/'test.c'),'-lm','-o',str(path/'test')],check=True)
        result=subprocess.run([str(path/'test')],capture_output=True,timeout=10)
        assert (result.returncode==0)==(index==0),result.stderr.decode()
print('PASS: 64 client histories, 32000 interpolations, identical/missing/gapped samples; old bandwidth and zero-division variants fail')
