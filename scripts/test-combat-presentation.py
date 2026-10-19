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
typedef int qboolean;
typedef struct {int clientNum;float fWeaponPosFrac,aimSpreadScale;} playerState_t;
typedef struct {float fraction;} trace_t;
typedef struct {int weapType,weapClass,shotCount;float fAdsSpread,fMinDamageRange;} WeaponDef;
typedef struct {void *viewModelDObj;} weaponInfo_t;
typedef struct {int leType,endTime;float tracerClipDist;struct {int trType,trTime;float trBase[3],trDelta[3];} pos;} localEntity_t;
#define LE_MOVING_TRACER 4
#define TR_LINEAR 2
static struct {struct {float vieworg[3],viewaxis[3][3],fov_x,fov_y;} refdef;int time,frametime;float gunPitch,gunYaw,viewModelAxis[3][3];} cgState,*cg=&cgState;
typedef struct {struct {int eType,clientNum;} nextState;int nextValid;float lerpOrigin[3];} centity_t;
static centity_t entities[64],*cg_entities=entities;
static dvar_t width,scale,minDist,range,chance,blood,speed;
static dvar_t *cg_tracerWidth=&width,*cg_tracerScale=&scale,*cg_tracerScaleMinDist=&minDist,*cg_tracerScaleDistRange=&range,*cg_tracerChance=&chance,*cg_blood=&blood,*cg_tracerSpeed=&speed;
static struct {struct {int tracerMaterial; snd_alias_list_t *bulletHitSmallSound[23],*bulletHitLargeSound[23],*shotgunHitSound[23]; struct {void *table;} *fx; void *fxNoBloodFleshHit;}media;} cgsState,*cgs=&cgsState;
typedef struct {char *mEffectName;int mPrimitiveCount;} EffectTemplate;
static GfxWorldVertex polys[2][4];static int draws,whizzes,spawns,impacts,muzzleReady=1,immediateSpawns,traces;
static localEntity_t immediateLe;
static unsigned short tag;static void *s_barrelTags[]={&tag,&tag,&tag,&tag};
static weaponInfo_t weapInfos[4];static weaponInfo_t *weapInfoPtr=weapInfos;static void *imp_cg_weapons=&weapInfoPtr;
static WeaponDef testWeapon={.weapType=0,.weapClass=0,.shotCount=1,.fAdsSpread=0.5f,.fMinDamageRange=1500.f};
static float traceFraction=0.25f,traceEnd[3];static int traceSkip,traceMask;
static float g_webAdsDofFrac,g_webAdsViewmodelStart=6.f,g_webAdsViewmodelEnd=15.f,g_webAdsNearEnd=512.f;
static int s_pendingLocalFireWeapon,s_pendingLocalFireBarrel,s_pendingLocalFireTime,s_localTracerSpawnedTime;
typedef struct {float center[2],axes[2][2],presence,open,blur,dark,projection[2];} CgWwiiScopeLens;
static CgWwiiScopeLens scopeLens;
static const CgWwiiScopeLens *CG_WwiiScopeLens(void){return &scopeLens;}
static localEntity_t *CG_AllocLocalEntity(void){immediateSpawns++;return &immediateLe;}
static int BG_IsAimDownSightWeapon(int w){return w==1;}
static void *BG_GetWeaponDef(int w){return &testWeapon;}
static void *BG_GetPlayerWeaponDef(const playerState_t *ps,int w){return &testWeapon;}
static int CG_DObjGetViewModelTagPos(void *obj,unsigned t,float *out){out[0]=cg->refdef.vieworg[0]+12;out[1]=cg->refdef.vieworg[1]-2;out[2]=cg->refdef.vieworg[2]-1;return 1;}
static void AngleVectors(const float *ang,float *f,float *r,float *u){f[0]=1;f[1]=f[2]=0;r[0]=r[2]=0;r[1]=1;u[0]=u[1]=0;u[2]=1;}
static void BG_GetSpreadForWeapon(const playerState_t *ps,int w,float *lo,float *hi){*lo=1.f;*hi=4.f;}
static float randomf(void){return 0.5f;}
static void CG_TraceCapsule(trace_t *tr,const float *s,const float *mn,const float *mx,const float *e,int skip,int mask){traces++;memcpy(traceEnd,e,12);traceSkip=skip;traceMask=mask;tr->fraction=traceFraction;}
static float Vec3Normalize(float *v){float len=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(len)for(int i=0;i<3;i++)v[i]/=len;return len;}
static float Vec3Distance(const float *a,const float *b){float s=0;for(int i=0;i<3;i++)s+=(a[i]-b[i])*(a[i]-b[i]);return sqrtf(s);}
static void Vec3Cross(const float *a,const float *b,float *c){c[0]=a[1]*b[2]-a[2]*b[1];c[1]=a[2]*b[0]-a[0]*b[2];c[2]=a[0]*b[1]-a[1]*b[0];}
static void CL_DefaultVertexFrames(int count,GfxWorldVertex *v){assert(count==4);}
static void CL_AddPolyToScene(int mat,int flags,int count,GfxWorldVertex *v){assert(count==4&&draws<2);memcpy(polys[draws++],v,sizeof(polys[0]));}
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
    'CG_TracerScaleForDistance', 'CG_SetTracerVertex', 'CG_DrawTracer', 'CG_BulletHitClientEvent',
    'CG_BulletHitEvent', 'CG_SpawnTracerImmediate', 'CG_WebAdsNearEnd', 'CG_WebAdsDofApproach',
    'CG_WebAdsNearestPlayer', 'CG_UpdateWebWeaponPresentation'))
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
   draws=0;CG_DrawTracer(start,end);assert(draws==2);
   GfxWorldVertex *streak=polys[0],*glow=polys[1];
   float w=Vec3Distance(streak[0].xyz,streak[1].xyz);
   assert(fabsf(w-1.6f)<.003f);
   assert(Vec3Distance(streak[2].xyz,streak[3].xyz)<w);
   float toward[3]={start[0]-translation,start[1]-translation,start[2]-translation};
   float side[3];for(int i=0;i<3;i++)side[i]=streak[0].xyz[i]-end[i];
   assert(fabsf(side[0]*toward[0]+side[1]*toward[1]+side[2]*toward[2])<.01f);
   // A hot core along the whole streak, across the image's centre column.
   for(int v=0;v<4;v++)assert(streak[v].color.packed==0xffffffff);
   for(int v=0;v<4;v++){
    assert(streak[v].texCoord[0]==.5f && streak[v].lmapCoord[0]==.5f);
    assert(streak[v].texCoord[1]==(v==0||v==3) && streak[v].lmapCoord[1]==streak[v].texCoord[1]);
   }
   // The round itself: a screen-aligned glow centred on the head.
   float centre[3]={0,0,0};
   for(int v=0;v<4;v++)for(int i=0;i<3;i++)centre[i]+=glow[v].xyz[i]*.25f;
   assert(Vec3Distance(centre,end)<.002f);
   assert(fabsf(Vec3Distance(glow[0].xyz,glow[1].xyz)-3.2f)<.003f);
   assert(fabsf(Vec3Distance(glow[1].xyz,glow[2].xyz)-3.2f)<.003f);
   for(int v=0;v<4;v++){
    assert(fabsf(glow[v].xyz[0]-end[0])<.002f);
    assert(glow[v].color.packed==0xc0c0c0c0);
    assert((glow[v].texCoord[0]==0||glow[v].texCoord[0]==1)&&(glow[v].texCoord[1]==0||glow[v].texCoord[1]==1));
   }
   assert(glow[0].texCoord[0]!=glow[1].texCoord[0] && glow[1].texCoord[1]!=glow[2].texCoord[1]);
   for(int p=0;p<2;p++)for(int v=0;v<4;v++)for(int i=0;i<3;i++)assert(isfinite(polys[p][v].xyz[i]));
  }
 }
 memset(cg->refdef.vieworg,0,12);
 float start[3]={20,0,0},end[3]={180,0,0},normal[3]={0,0,1};
 draws=0;CG_DrawTracer(start,end);assert(draws==2);assert(Vec3Distance(polys[0][0].xyz,polys[0][1].xyz)>1.59f);
 draws=0;CG_DrawTracer(start,start);assert(draws==0);
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
 speed.current.value=6500.f;chance.current.value=0.85f;
 cg->time=5000;cg->frametime=16;cg->viewModelAxis[0][0]=1.f;
 weapInfos[1].viewModelDObj=&testWeapon;
 playerState_t ps={.clientNum=0,.fWeaponPosFrac=1.f,.aimSpreadScale=0.f};
 traceFraction=0.125f;
 s_pendingLocalFireWeapon=1;s_pendingLocalFireBarrel=0;s_pendingLocalFireTime=5000;
 immediateSpawns=0;g_webAdsNearEnd=77.f;
 CG_UpdateWebWeaponPresentation(&ps,1,1);
 assert(immediateSpawns==1 && s_localTracerSpawnedTime==5000);
 assert(immediateLe.pos.trTime==5000 && immediateLe.pos.trBase[0]==18.f);
 // The viewmodel is sharp from 5/8 of the muzzle distance on; the 12-unit
 // fixture muzzle clamps to the 10-unit minimum.
 assert(fabsf(g_webAdsDofFrac-1.f)<1e-6f && g_webAdsViewmodelEnd==10.f);
 assert(fabsf(g_webAdsViewmodelStart-2.6f)<1e-6f);
 // A wall 1024 units down the crosshair with nobody on screen: the world blurs
 // up to 512 units, entered at once rather than eased from the last aim.
 assert(g_webAdsNearEnd==512.f);
 // Aiming traces once for focus, 8192 units down the view axis past the player.
 traces=0;cg->time=5016;CG_UpdateWebWeaponPresentation(&ps,1,1);
 assert(traces==1 && immediateSpawns==1 && g_webAdsDofFrac==1.f && g_webAdsNearEnd==512.f);
 assert(traceEnd[0]==8192.f && traceEnd[1]==0.f && traceEnd[2]==0.f && traceSkip==0 && traceMask==0x2802831);
 // A round that rolls above the tracer chance draws nothing, and the late
 // server copy of the shot stays suppressed.
 chance.current.value=0.25f;cg->time=5032;s_pendingLocalFireWeapon=1;s_pendingLocalFireTime=5032;
 traces=0;immediateSpawns=0;CG_UpdateWebWeaponPresentation(&ps,1,1);
 assert(immediateSpawns==0 && traces==1 && s_localTracerSpawnedTime==5032);
 chance.current.value=0.85f;
 testWeapon.weapClass=3;testWeapon.shotCount=8;
 cg->time=5050;s_pendingLocalFireWeapon=1;s_pendingLocalFireTime=5050;immediateSpawns=0;
 CG_UpdateWebWeaponPresentation(&ps,1,1);
 assert(immediateSpawns==3);
 // Through a WWII scope's window the shooter sees no tracer of their own,
 // and the server's copy of the shot stays suppressed all the same.
 scopeLens.presence=0.4f;cg->time=5066;s_pendingLocalFireWeapon=1;s_pendingLocalFireTime=5066;
 immediateSpawns=0;traces=0;CG_UpdateWebWeaponPresentation(&ps,1,1);
 assert(immediateSpawns==0 && traces==1 && s_localTracerSpawnedTime==5066);
 scopeLens.presence=0.f;
 ps.fWeaponPosFrac=0.f;CG_UpdateWebWeaponPresentation(&ps,1,1);
 assert(g_webAdsDofFrac==0.f);
 // Only other live players around the view keep the near blur short of them.
 testWeapon.weapClass=0;testWeapon.shotCount=1;
 cg->refdef.fov_x=80.f;cg->refdef.fov_y=64.f;traceFraction=1.f;
 centity_t ignored[]={
  {{1,3},0,{150,0,-32}},    // stale
  {{2,4},1,{150,0,-32}},    // not a player
  {{1,0},1,{150,0,-32}},    // the local player
  {{1,6},1,{-150,0,-32}},   // behind the eye
  {{1,7},1,{150,400,-32}},  // well off the side of the view
 };
 for(int i=0;i<5;i++)entities[i]=ignored[i];
 ps.fWeaponPosFrac=0.5f;CG_UpdateWebWeaponPresentation(&ps,1,1);
 assert(g_webAdsNearEnd==512.f);
 // Still aiming in: the focus follows a player 200 units away straight away.
 entities[8]=(centity_t){{1,8},1,{200,60,-32}};
 CG_UpdateWebWeaponPresentation(&ps,1,1);assert(g_webAdsNearEnd==170.f);
 // Fully aimed in, it eases: half the gap per frame, at most 50 units per 50 ms.
 ps.fWeaponPosFrac=1.f;cg->frametime=16;
 CG_UpdateWebWeaponPresentation(&ps,1,1);assert(g_webAdsNearEnd==170.f);
 entities[8].lerpOrigin[0]=400.f;
 CG_UpdateWebWeaponPresentation(&ps,1,1);assert(fabsf(g_webAdsNearEnd-186.f)<1e-3f);
 float last=g_webAdsNearEnd;
 for(int frame=0;frame<40;frame++){
  CG_UpdateWebWeaponPresentation(&ps,1,1);
  assert(g_webAdsNearEnd>=last && g_webAdsNearEnd<=370.f);last=g_webAdsNearEnd;
 }
 assert(g_webAdsNearEnd==370.f);
 // A surface under the crosshair nearer than the player stays sharp as well.
 traceFraction=100.f/8192.f;ps.fWeaponPosFrac=0.9f;
 CG_UpdateWebWeaponPresentation(&ps,1,1);assert(fabsf(g_webAdsNearEnd-70.f)<1e-3f);
 assert(CG_WebAdsNearEnd(1000.f,60.f)==50.f && CG_WebAdsNearEnd(1000.f,2000.f)==512.f);
 assert(CG_WebAdsNearEnd(20.f,1.0e30f)==1.f && CG_WebAdsNearEnd(NAN,NAN)==512.f);
 assert(CG_WebAdsDofApproach(100.f,100.4f,50.f,.016f)==100.4f && CG_WebAdsDofApproach(100.f,99.6f,50.f,.016f)==99.6f);
 assert(fabsf(CG_WebAdsDofApproach(0.f,1000.f,50.f,.016f)-16.f)<1e-3f && CG_WebAdsDofApproach(100.f,200.f,50.f,0.f)==100.f);
 puts("PASS: 1512 translated/off-axis tracer streaks and head glows, immediate first-person tracers at the tracer chance, CoD4 ADS focus from the crosshair trace and on-screen players eased once aimed in, audio independent of visuals, 100 light-selection frames, all 0-32 limits and nonfinite rejection");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-combat-presentation-') as tmp:
    path = Path(tmp)
    (path / 'test.c').write_text(support + code + checks)
    subprocess.run(['cc', '-D__EMSCRIPTEN__', '-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
