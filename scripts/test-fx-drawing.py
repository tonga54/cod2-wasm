#!/usr/bin/env python3
"""Exercise the real scheduled FX drawing pass and particle color handoff."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/EffectsCore/FxUtil.c').read_text()

def function(name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'

support = r'''
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char byte; typedef int Bool;
typedef struct {struct {int enabled;} current;} dvar_t;
typedef struct Effect {
 void **vtable; int mFlags,mTimeEnd,mClusterId,culled,draws,deaths,frees,updates;
 struct {float origin[3];} mRefEnt;
} Effect;
typedef struct {Effect *effect;float distSq;} SortedEffect;
static dvar_t yes={{1}},no={{0}},*yesp=&yes,*nop=&no;
static void *imp_fx_enable=&yesp,*imp_fx_cull=&yesp,*imp_fx_sort=&yesp,
 *imp_fx_draw=&yesp,*imp_fx_debug=&nop;
static int cameraValid=1;static void *imp_fx_camera_valid=&cameraValid;
static struct {int mTime;struct{float vieworg[3];}mCamera;} helper,*theFxHelper=&helper;
typedef struct {int mScheduledCount;} FxScheduler;
static FxScheduler scheduler,*schedulerp=&scheduler;
static void *imp_theFxScheduler=&schedulerp;
static Effect *effectListNonBolt[1800],*effectListBolt[1800];
static SortedEffect visibleEffectsNonBolt[1800],visibleEffectsBolt[1800];
typedef struct {float origin[3];int refCount;} EffectCluster;
static EffectCluster effectClusters[1800];
static int effectClusterCount=1,*clusterSort;
static int cullEffectCountNonBolt,cullEffectCountBolt,visibleEffectCountNonBolt,visibleEffectCountBolt;
static int privateEffectActiveCountNonBolt,privateEffectActiveCountBolt;
static int effectActiveCountNonBolt,effectActiveCountBolt,effectActiveCount;
static int initialEffectActiveCountNonBolt,initialEffectActiveCountBolt;
static int effectBlockSightCount,g_effectVisArrayCount;
static void FX_Print(const char *f,...){}
static void FX_AddScheduledEffects(void){}
static void FX_RemoveCluster(int id){assert(!"unexpected empty cluster");}
static float Vec3DistanceSq(const float*a,const float*b){float v=0;for(int i=0;i<3;i++)v+=(a[i]-b[i])*(a[i]-b[i]);return v;}
static int CompareSortedClusters(const void*a,const void*b){return 0;}
static int CompareSortedEffects(const void*a,const void*b){
 float x=((const SortedEffect*)a)->distSq,y=((const SortedEffect*)b)->distSq;
 return (x<y)-(x>y);
}
static void dispose(void*p){((Effect*)p)->frees++;}
static void die(void*p){((Effect*)p)->deaths++;}
static Bool update(void*p){((Effect*)p)->updates++;return 1;}
static Bool cull(void*p){return ((Effect*)p)->culled;}
static void draw(void*p){((Effect*)p)->draws++;}
static void visibility(void*p){}
static void *vtable[]={0,dispose,die,update,cull,draw,0,visibility};
'''
checks = r'''
int main(void){
 for(int count=1;count<=128;count++){
  Effect particles[128]={0}; helper.mTime=100;
  effectActiveCountNonBolt=(count+1)/2;effectActiveCountBolt=count/2;
  privateEffectActiveCountNonBolt=initialEffectActiveCountNonBolt=effectActiveCountNonBolt;
  privateEffectActiveCountBolt=initialEffectActiveCountBolt=effectActiveCountBolt;
  effectActiveCount=count;effectClusters[0].refCount=count+1;
  for(int i=0;i<count;i++){
   particles[i].vtable=vtable;particles[i].mTimeEnd=i%5==0?90:1000;
   particles[i].culled=i%7==0;particles[i].mRefEnt.origin[0]=i;
   if(i%2)effectListBolt[i/2]=&particles[i];else effectListNonBolt[i/2]=&particles[i];
  }
  /* Stale visibility from the earlier update must be discarded. */
  cullEffectCountNonBolt=cullEffectCountBolt=1700;
  visibleEffectCountNonBolt=visibleEffectCountBolt=1700;
  FX_UpdateScheduledEffectsNonBolt();
  FX_UpdateScheduledEffectsBolt();
  FX_DrawScheduledEffects();
  for(int i=0;i<count;i++)assert(particles[i].draws==(i%5!=0&&i%7!=0));
  FX_UpdateScheduledEffectsNonBolt();
  FX_UpdateScheduledEffectsBolt();
  FX_DrawScheduledEffects();
  for(int i=0;i<count;i++){
   assert(particles[i].updates==2*(i%5!=0));
   assert(particles[i].draws==2*(i%5!=0&&i%7!=0));
   assert(particles[i].deaths==(i%5==0));assert(particles[i].frees==(i%5==0));
  }
 }
 puts("PASS: 8,256 scheduled particles, expiry/culling, visibility rebuild and draw submission");
}
'''

# Quad, oriented sprite and line paths must preserve the same RGBA layout as
# world vertices. Alpha-first packing made neutral smoke appear yellow/green.
tess = (root / 'src/PC/gfx_d3d/rb_tess.c').read_text()
assert 'memcpy(&nativeColor, ((GfxEntity *)re)->materialRGBA, sizeof(nativeColor));' in tess
assert 'memcpy(&nativeColor, ((GfxEntity *)ent)->materialRGBA, sizeof(nativeColor));' in tess
assert 'memcpy(&color, ((GfxEntity *)ent)->materialRGBA, sizeof(color));' in tess

with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)
    (path / 'fx.c').write_text(support + function('FX_UpdateScheduledEffectsNonBolt') + function('FX_UpdateScheduledEffectsBolt') + function('FX_DrawAll') + function('FX_DrawScheduledEffects') + checks)
    subprocess.run(['cc', '-std=c11', '-fsanitize=address,undefined', '-g', str(path / 'fx.c'), '-o', str(path / 'fx')], check=True)
    subprocess.run([str(path / 'fx')], check=True)
