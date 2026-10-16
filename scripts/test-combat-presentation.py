#!/usr/bin/env python3
"""Check production tracer geometry, bullet audio and bounded light selection."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def function(path, name):
    source = (ROOT / path).read_text()
    match = re.search(r'[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


support = r'''
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
typedef float vec_t;
typedef float vec3_t[3];
typedef unsigned char byte;
typedef int snapshot_t;
typedef int snd_alias_list_t;
typedef struct {struct {float value; int integer,enabled;} current;} dvar_t;
typedef struct {float xyz[3]; union {unsigned packed;} color; float texCoord[2],lmapCoord[2];} GfxWorldVertex;
static struct {struct {float vieworg[3],viewaxis[3][3];} refdef;} cgState,*cg=&cgState;
static dvar_t width,scale,minDist,range,chance,blood;
static dvar_t *cg_tracerWidth=&width,*cg_tracerScale=&scale,*cg_tracerScaleMinDist=&minDist,*cg_tracerScaleDistRange=&range,*cg_tracerChance=&chance,*cg_blood=&blood;
static struct {struct {int tracerMaterial; snd_alias_list_t *bulletHitSmallSound[23],*bulletHitLargeSound[23],*shotgunHitSound[23]; struct {void *table;} *fx; void *fxNoBloodFleshHit;}media;} cgsState,*cgs=&cgsState;
typedef struct {char *mEffectName;int mPrimitiveCount;} EffectTemplate;
static GfxWorldVertex output[4];static int draws,whizzes,spawns,impacts,muzzleReady=1;
static unsigned short tag;static void *s_barrelTags[]={&tag};
static float Vec3Normalize(float *v){float len=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(len)for(int i=0;i<3;i++)v[i]/=len;return len;}
static float Vec3Distance(const float *a,const float *b){float s=0;for(int i=0;i<3;i++)s+=(a[i]-b[i])*(a[i]-b[i]);return sqrtf(s);}
static void Vec3Cross(const float *a,const float *b,float *c){c[0]=a[1]*b[2]-a[2]*b[1];c[1]=a[2]*b[0]-a[0]*b[2];c[2]=a[0]*b[1]-a[1]*b[0];}
static void CL_DefaultVertexFrames(int count,GfxWorldVertex *v){assert(count==4);}
static void CL_AddPolyToScene(int mat,int flags,int count,GfxWorldVertex *v){assert(count==4);draws++;memcpy(output,v,sizeof(output));}
static void CG_PlaySoundAlias(int ent,float *pos,snd_alias_list_t *alias){impacts++;}
static int CG_CalcMuzzlePoint(int ent,float *out,unsigned tag){memset(out,0,12);return muzzleReady;}
static void CG_WhizbySound(float *a,float *b){whizzes++;}
static void CG_SpawnTracer(float *a,float *b){spawns++;}
static void FX_PlayEffect(void *fx,float *pos,float *normal){}
static void Com_Printf(const char *fmt,...){}
typedef struct {void *def;float position[4],color[3];} GfxLight;
typedef struct {void *world,*dlightDef;} r_global_permanent_t;
static r_global_permanent_t rgp;
static struct {int registered;float viewOrg[3];} rg;
static struct {int dlightCount;GfxLight dlights[32];} scene;
static dvar_t limit;static const dvar_t *limitPtr=&limit;
static const void *imp_r_dlightLimit=&limitPtr;
'''
weapon = 'src/PC/cgame_mp/cg_weapons.c'
code = ''.join(function(weapon, name) for name in (
    'CG_TracerScaleForDistance', 'CG_DrawTracer', 'CG_BulletHitClientEvent', 'CG_BulletHitEvent'))
code += function('src/PC/gfx_d3d/r_scene.c', 'R_AddLightToScene')
checks = r'''
int main(void){
 width.current.value=.8f;scale.current.value=1;cg->refdef.viewaxis[0][0]=cg->refdef.viewaxis[1][1]=cg->refdef.viewaxis[2][2]=1;
 for(int translation=-10000;translation<=10000;translation+=1000){
  cg->refdef.vieworg[0]=translation;cg->refdef.vieworg[1]=translation;cg->refdef.vieworg[2]=translation;
  for(int angle=0;angle<360;angle+=5){
   float a=angle*.01745329252f;
   float start[3]={translation+20,translation+5*cosf(a),translation+5*sinf(a)};
   float end[3]={translation+180,start[1],start[2]};
   draws=0;CG_DrawTracer(start,end);assert(draws==1);
   float w=Vec3Distance(output[0].xyz,output[1].xyz);
   assert(fabsf(w-1.6f)<.003f);
   assert(Vec3Distance(output[2].xyz,output[3].xyz)<w);
   float toward[3]={start[0]-translation,start[1]-translation,start[2]-translation};
   float side[3];for(int i=0;i<3;i++)side[i]=output[0].xyz[i]-end[i];
   assert(fabsf(side[0]*toward[0]+side[1]*toward[1]+side[2]*toward[2])<.01f);
   for(int v=0;v<4;v++)for(int i=0;i<3;i++)assert(isfinite(output[v].xyz[i]));
   assert(output[0].color.packed==0xffffffff && output[2].color.packed==0x80808080);
  }
 }
 memset(cg->refdef.vieworg,0,12);
 float start[3]={20,0,0},end[3]={180,0,0},normal[3]={0,0,1};
 draws=0;CG_DrawTracer(start,end);assert(draws==1);assert(Vec3Distance(output[0].xyz,output[1].xyz)>1.59f);
 CG_DrawTracer(start,start);assert(draws==1);
 for(int event=0;event<2;event++)for(int enabled=0;enabled<2;enabled++){
  chance.current.value=enabled;whizzes=spawns=impacts=0;
  if(event)CG_BulletHitEvent(1,end,normal,normal,0,0xb5);else CG_BulletHitClientEvent(1,end,0,0xb9);
  assert(impacts==1 && whizzes==1 && spawns==enabled);
 }
 muzzleReady=0;whizzes=spawns=0;CG_BulletHitClientEvent(1,end,0,0xb9);assert(!whizzes&&!spawns);
 rg.registered=1;rgp.world=&rgp;limit.current.integer=4;
 for(int frame=0;frame<100;frame++){
  scene.dlightCount=0;
  for(int i=100;i>=1;i--){float pos[3]={i*100,0,0};R_AddLightToScene(pos,200,1,.7,.3);assert(scene.dlightCount<=4);}
  assert(scene.dlightCount==4);
  for(int i=0;i<4;i++)assert(scene.dlights[i].position[0]<=400);
  float pos[3]={1,0,0};R_AddLightToScene(pos,200,0,0,0);
  for(int i=0;i<4;i++)assert(scene.dlights[i].position[0]>=100);
  pos[0]=1000;R_AddLightToScene(pos,10000,1,.7,.3);
  int explosion=0;for(int i=0;i<4;i++)explosion+=scene.dlights[i].position[3]==10000;assert(explosion==1);
 }
 for(int limitValue=0;limitValue<=32;limitValue++){
  scene.dlightCount=0;limit.current.integer=limitValue;
  for(int i=0;i<100;i++)R_AddLightToScene(start,200,1,1,1);
  assert(scene.dlightCount==limitValue);
 }
 scene.dlightCount=0;limit.current.integer=4;
 for(int f=0;f<7;f++){
  float v[7]={1,2,3,200,1,1,1};v[f]=NAN;R_AddLightToScene(v,v[3],v[4],v[5],v[6]);
  assert(scene.dlightCount==0);
 }
 puts("PASS: 1512 translated/off-axis tracer ribbons, end-on/empty beams, audio independent of visuals, 100 light-selection frames, all 0-32 limits and nonfinite rejection");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-combat-presentation-') as tmp:
    path = Path(tmp)
    (path / 'test.c').write_text(support + code + checks)
    subprocess.run(['cc', '-D__EMSCRIPTEN__', '-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
