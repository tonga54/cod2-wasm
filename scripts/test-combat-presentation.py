#!/usr/bin/env python3
"""Check production tracer geometry and shape, bullet audio and bounded light selection.

The tracer as drawn is compiled on its own as well, with mutants that must fail."""
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
static dvar_t chance,blood,speed;
static dvar_t *cg_tracerChance=&chance,*cg_blood=&blood,*cg_tracerSpeed=&speed;
static struct {struct {snd_alias_list_t *bulletHitSmallSound[23],*bulletHitLargeSound[23],*shotgunHitSound[23]; struct {void *table;} *fx; void *fxNoBloodFleshHit;}media;} cgsState,*cgs=&cgsState;
typedef struct {char *mEffectName;int mPrimitiveCount;} EffectTemplate;
static int whizzes,spawns,impacts,muzzleReady=1,immediateSpawns,traces;
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
/* The weapon's projection spreads a point half as far again from the view's x axis as the view's. */
static int gunFovPoints;
static void CG_GunFovPoint(float *p){gunFovPoints++;for(int i=1;i<3;i++)p[i]=cg->refdef.vieworg[i]+1.5f*(p[i]-cg->refdef.vieworg[i]);}
static void AngleVectors(const float *ang,float *f,float *r,float *u){f[0]=1;f[1]=f[2]=0;r[0]=r[2]=0;r[1]=1;u[0]=u[1]=0;u[2]=1;}
static void BG_GetSpreadForWeapon(const playerState_t *ps,int w,float *lo,float *hi){*lo=1.f;*hi=4.f;}
static float randomf(void){return 0.5f;}
static void CG_TraceCapsule(trace_t *tr,const float *s,const float *mn,const float *mx,const float *e,int skip,int mask){traces++;memcpy(traceEnd,e,12);traceSkip=skip;traceMask=mask;tr->fraction=traceFraction;}
static float Vec3Normalize(float *v){float len=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(len)for(int i=0;i<3;i++)v[i]/=len;return len;}
static float Vec3Distance(const float *a,const float *b){float s=0;for(int i=0;i<3;i++)s+=(a[i]-b[i])*(a[i]-b[i]);return sqrtf(s);}
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
# The spread the first-person tracer leaves with is the server's own rule.
spread_source = (ROOT / 'src/PC/bgame/bg_weapons.c').read_text()
spread_rule = ('#define WEAPCLASS_SPREAD 3\n' +
               spread_source[spread_source.index('#define BG_SIGHTS_UP_START'):
                             spread_source.index('/* The spread a shot leaves with.')] +
               function('src/PC/bgame/bg_weapons.c', 'BG_GetShotSpread'))
code = spread_rule + ''.join(function(weapon, name) for name in (
    'CG_BulletHitClientEvent', 'CG_BulletHitEvent', 'CG_SpawnTracerImmediate', 'CG_WebAdsNearEnd',
    'CG_WebAdsDofApproach', 'CG_WebAdsNearestPlayer', 'CG_UpdateWebWeaponPresentation'))
code += function('src/PC/gfx_d3d/r_scene.c', 'R_AddLightToScene')
checks = r'''
int main(void){
 cg->refdef.viewaxis[0][0]=cg->refdef.viewaxis[1][1]=cg->refdef.viewaxis[2][2]=1;
 float start[3]={20,0,0},end[3]={180,0,0},normal[3]={0,0,1};
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
 // It leaves where the view's projection draws the muzzle the weapon's own
 // projection shows (cg_gunfov_mp.c).
 assert(gunFovPoints==1 && immediateLe.pos.trBase[1]==-3.f && immediateLe.pos.trBase[2]==-1.5f);
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
 puts("PASS: immediate first-person tracers at the tracer chance, CoD4 ADS focus from the crosshair trace and on-screen players eased once aimed in, audio independent of visuals, 100 light-selection frames, all 0-32 limits and nonfinite rejection");
}
'''

# The tracer as drawn: a round's streak each frame, and its ribbon.
tracer_support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef float vec_t;
typedef float vec3_t[3];
typedef struct {struct {float value;} current;} dvar_t;
typedef struct {float xyz[3]; union {unsigned packed;} color; float texCoord[2],lmapCoord[2];} GfxWorldVertex;
typedef struct {int trType,trTime,trDuration;float trBase[3],trDelta[3];} trajectory_t;
typedef struct {int leType,endTime;trajectory_t pos;float tracerClipDist;} localEntity_t;
#define TR_LINEAR 2
static struct {struct {float vieworg[3],viewaxis[3][3];} refdef;int time;} cgState,*cg=&cgState;
static struct {struct {int tracerMaterial;} media;} cgsState={{7}},*cgs=&cgsState;
static dvar_t width,scale,minDist,range,length;
static const dvar_t *cg_tracerWidth=&width,*cg_tracerScale=&scale,*cg_tracerScaleMinDist=&minDist,
                    *cg_tracerScaleDistRange=&range,*cg_tracerLength=&length;
static GfxWorldVertex polys[9][4],*framedVerts;
static int draws;
static float Vec3Normalize(float *v){float len=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(len)for(int i=0;i<3;i++)v[i]/=len;return len;}
static float Vec3NormalizeTo(const float *v,float *out){memcpy(out,v,12);return Vec3Normalize(out);}
static float Vec3Distance(const float *a,const float *b){float s=0;for(int i=0;i<3;i++)s+=(a[i]-b[i])*(a[i]-b[i]);return sqrtf(s);}
static void Vec3Cross(const float *a,const float *b,float *c){c[0]=a[1]*b[2]-a[2]*b[1];c[1]=a[2]*b[0]-a[0]*b[2];c[2]=a[0]*b[1]-a[1]*b[0];}
static void CL_DefaultVertexFrames(int count,GfxWorldVertex *v){assert(count==4);framedVerts=v;}
static void CL_AddPolyToScene(int material,int lmapIndex,int count,GfxWorldVertex *v){
 assert(material==cgs->media.tracerMaterial&&lmapIndex==0x1f&&count==4&&v==framedVerts&&draws<9);
 framedVerts=NULL;memcpy(polys[draws++],v,sizeof(polys[0]));
}
static void BG_EvaluateTrajectory(const trajectory_t *tr,int time,float *out){
 assert(tr->trType==TR_LINEAR);for(int i=0;i<3;i++)out[i]=tr->trBase[i]+tr->trDelta[i]*(time-tr->trTime)*.001f;
}
static float drawnStart[3],drawnEnd[3];static unsigned drawnSeed;static int drawnCount;
static void CaptureTracer(vec_t *start,vec_t *finish,unsigned int seed){
 memcpy(drawnStart,start,12);memcpy(drawnEnd,finish,12);drawnSeed=seed;drawnCount++;
}
'''
weapon_source = (ROOT / weapon).read_text()
tracer_code = ''.join(re.findall(r'#define CG_TRACER_\w+ [^\n]+\n', weapon_source))
tracer_code += ''.join(function(weapon, name) for name in (
    'CG_TracerScaleForDistance', 'CG_TracerHash', 'CG_TracerSeed', 'CG_SetTracerVertex', 'CG_TracerRandom',
    'CG_TracerBend', 'CG_DrawTracer'))
tracer_code += ('#define CG_DrawTracer CaptureTracer\n' + function('src/PC/cgame_mp/cg_localents_mp.c', 'CG_AddMovingTracer') +
                '#undef CG_DrawTracer\n')
tracer_checks = r'''
static float dot3(const float *a,const float *b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
static void sub3(const float *a,const float *b,float *c){for(int i=0;i<3;i++)c[i]=a[i]-b[i];}
static float angle_between(const float *a,const float *b){float c[3];Vec3Cross(a,b,c);return atan2f(sqrtf(dot3(c,c)),dot3(a,b));}
static int streaks;

/* Eight segments joined into one ribbon facing the eye, then the glow. The
 * ribbon's centre line runs from the rear to the head on the shot's line,
 * evenly spaced as seen, bent across itself by the round's share of the angle
 * it spans, and as wide as a straight streak would be there. eps is how far
 * float rounding may move a vertex at the streak's distance from the origin. */
static void check_streak(const float *start,const float *end,unsigned seed,float rearHalf,float headHalf,float eps){
 const float *eye=cg->refdef.vieworg;
 float centre[9][3],side[9][3],half[9];
 assert(draws==9);
 for(int k=0;k<9;k++){
  const GfxWorldVertex *a=k<8?&polys[k][3]:&polys[7][0],*b=k<8?&polys[k][2]:&polys[7][1];
  for(int i=0;i<3;i++){centre[k][i]=(a->xyz[i]+b->xyz[i])*.5f;side[k][i]=a->xyz[i]-b->xyz[i];}
  half[k]=Vec3Normalize(side[k])*.5f;
 }
 for(int k=1;k<8;k++)
  assert(Vec3Distance(polys[k][3].xyz,polys[k-1][0].xyz)<eps&&Vec3Distance(polys[k][2].xyz,polys[k-1][1].xyz)<eps);
 // A hot core along the whole streak, across the image's centre column.
 for(int k=0;k<8;k++)for(int v=0;v<4;v++){
  assert(polys[k][v].color.packed==0xffffffff);
  assert(polys[k][v].texCoord[0]==.5f&&polys[k][v].lmapCoord[0]==.5f);
  assert(polys[k][v].texCoord[1]==(v==0||v==3)&&polys[k][v].lmapCoord[1]==polys[k][v].texCoord[1]);
 }
 assert(Vec3Distance(centre[0],start)<eps&&Vec3Distance(centre[8],end)<eps);
 float toStart[3],toEnd[3],line[3],normal[3],previous[3];
 sub3(start,eye,toStart);sub3(end,eye,toEnd);sub3(end,start,line);
 const float span=Vec3Normalize(line);
 Vec3Cross(toStart,toEnd,normal);
 const float across=atan2f(Vec3Normalize(normal),dot3(toStart,toEnd));
 for(int k=0;k<9;k++){
  float ray[3],flat[3],rel[3],tangent[3];
  sub3(centre[k],eye,ray);
  const float off=asinf(dot3(ray,normal)/sqrtf(dot3(ray,ray)));
  assert(fabsf(off+atanf(CG_TracerBend(seed,k/8.f)*across))<2e-4f);
  for(int i=0;i<3;i++)flat[i]=ray[i]-dot3(ray,normal)*normal[i];
  if(k)assert(fabsf(angle_between(previous,flat)-across/8)<2e-4f);
  memcpy(previous,flat,12);
  sub3(centre[k],start,rel);
  assert(fabsf(half[k]-(rearHalf+dot3(rel,line)/span*(headHalf-rearHalf)))<eps);
  Vec3Normalize(ray);
  sub3(centre[k<8?k+1:k],centre[k?k-1:k],tangent);Vec3Normalize(tangent);
  // Read from two vertices the width apart, each up to eps off.
  const float facing=2e-3f+eps/(2*half[k]);
  assert(fabsf(dot3(side[k],ray))<facing&&fabsf(dot3(side[k],tangent))<facing);
 }
 // The round itself: a screen-aligned glow centred on the head.
 const GfxWorldVertex *glow=polys[8];
 float middle[3]={0,0,0};
 for(int v=0;v<4;v++)for(int i=0;i<3;i++)middle[i]+=glow[v].xyz[i]*.25f;
 assert(Vec3Distance(middle,end)<eps);
 assert(fabsf(Vec3Distance(glow[0].xyz,glow[1].xyz)-4*headHalf)<eps&&fabsf(Vec3Distance(glow[1].xyz,glow[2].xyz)-4*headHalf)<eps);
 for(int v=0;v<4;v++){
  float d[3];sub3(glow[v].xyz,end,d);
  assert(fabsf(dot3(d,cg->refdef.viewaxis[0]))<eps&&glow[v].color.packed==0xc0c0c0c0);
  assert((glow[v].texCoord[0]==0||glow[v].texCoord[0]==1)&&(glow[v].texCoord[1]==0||glow[v].texCoord[1]==1));
 }
 assert(glow[0].texCoord[0]!=glow[1].texCoord[0]&&glow[1].texCoord[1]!=glow[2].texCoord[1]);
 for(int p=0;p<9;p++)for(int v=0;v<4;v++)for(int i=0;i<3;i++)assert(isfinite(polys[p][v].xyz[i]));
 streaks++;
}

static float along_streak(const float *start,const float *end,int joint){
 float line[3],rel[3],centre[3];
 const GfxWorldVertex *a=joint<8?&polys[joint][3]:&polys[7][0],*b=joint<8?&polys[joint][2]:&polys[7][1];
 for(int i=0;i<3;i++)centre[i]=(a->xyz[i]+b->xyz[i])*.5f;
 sub3(end,start,line);sub3(centre,start,rel);
 return dot3(rel,line)/dot3(line,line);
}

int main(void){
 cg->refdef.viewaxis[0][0]=cg->refdef.viewaxis[1][1]=cg->refdef.viewaxis[2][2]=1;
 width.current.value=.8f;scale.current.value=1;
 // Translated and off-axis ahead of the eye, the rounds' seeds all different.
 for(int translation=-10000;translation<=10000;translation+=1000){
  for(int i=0;i<3;i++)cg->refdef.vieworg[i]=translation;
  for(int angle=0;angle<360;angle+=5){
   const float a=angle*.01745329252f;
   float start[3]={translation+20,translation+5*cosf(a),translation+5*sinf(a)};
   float end[3]={translation+180,start[1],start[2]};
   const unsigned seed=(unsigned)(translation*7919+angle*104729);
   draws=0;CG_DrawTracer(start,end,seed);check_streak(start,end,seed,.28f,.8f,4e-3f);
  }
 }
 // End-on, it stays straight; a streak of no length draws nothing.
 memset(cg->refdef.vieworg,0,12);
 {
  float start[3]={20,0,0},end[3]={180,0,0};
  draws=0;CG_DrawTracer(start,end,12345);check_streak(start,end,12345,.28f,.8f,1e-4f);
  for(int k=0;k<9;k++)assert(fabsf(polys[k<8?k:7][k<8?2:1].xyz[1]+polys[k<8?k:7][k<8?3:0].xyz[1])<1e-6f);
  draws=0;CG_DrawTracer(start,start,12345);assert(draws==0);
 }
 // Through or just past the eye: nothing but finite polygons.
 for(int pass=0;pass<2;pass++){
  float start[3]={-50,pass*.01f,0},end[3]={50,pass*.01f,0};
  draws=0;CG_DrawTracer(start,end,999);assert(draws==9);
  for(int p=0;p<9;p++)for(int v=0;v<4;v++)for(int i=0;i<3;i++)assert(isfinite(polys[p][v].xyz[i]));
 }
 // With the browser's width and distance scale: seen from the side 500 and
 // 1,500 units away, obliquely, coming at the eye, passing beside it, and from
 // the gun at the hip and aimed, 100 units long toward a hit 900 units ahead.
 width.current.value=2;scale.current.value=50;minDist.current.value=5000;range.current.value=25000;
 float views[7][6]={
  {500,150,10,500,50,10},{1500,400,0,1500,300,0},{300,-200,40,380,-140,60},{400,3,-2,300,2,-1},{-40,6,0,60,6,0},
  {24,-7,-6},{22,0,-3},
 };
 for(int v=5;v<7;v++){
  float dir[3]={900-views[v][0],-views[v][1],-views[v][2]};Vec3Normalize(dir);
  for(int i=0;i<3;i++)views[v][3+i]=views[v][i]+100*dir[i];
 }
 for(int v=0;v<7;v++)for(unsigned seed=0;seed<64;seed++){
  draws=0;CG_DrawTracer(views[v],views[v]+3,seed*977u);check_streak(views[v],views[v]+3,seed*977u,.7f,2.f,2e-3f);
 }
 // From the hip the streak runs away from an eye near its rear: evenly spaced
 // as seen, half its joints crowd the fifth of it nearest the muzzle.
 draws=0;CG_DrawTracer(views[5],views[5]+3,1);
 assert(along_streak(views[5],views[5]+3,4)>.1f&&along_streak(views[5],views[5]+3,4)<.2f);

 // Every round bows one way or the other by 0.6 to 1 of 7% of the streak's
 // length as seen, with an S and a ripple of up to a half and a quarter of
 // that: never straight, pinned at both ends, the parts independent.
 const float bend=.07f;
 int rightBows=0,strongS=0,strongRipple=0;
 double bowS=0,bowRipple=0,sRipple=0;
 for(unsigned seed=0;seed<20000;seed++){
  assert(CG_TracerBend(seed,0)==0&&fabsf(CG_TracerBend(seed,1))<1e-6f);
  const float a=CG_TracerBend(seed,1/6.f),m=CG_TracerBend(seed,.5f),b=CG_TracerBend(seed,5/6.f);
  const float ripple=(a+b-m)/(.75f*bend),bow=m/bend+.25f*ripple,s=(a-b)/(.8660254f*bend);
  assert(fabsf(bow)>=.6f-1e-4f&&fabsf(bow)<=1+1e-4f&&fabsf(s)<=1+1e-4f&&fabsf(ripple)<=1+1e-4f);
  assert(fabsf(m)>=.35f*bend-1e-6f);
  for(int i=1;i<20;i++){
   const float f=i/20.f;
   assert(fabsf(CG_TracerBend(seed,f)-bend*(bow*sinf(3.14159265f*f)+.5f*s*sinf(6.2831853f*f)+.25f*ripple*sinf(9.424778f*f)))<1e-5f);
  }
  rightBows+=bow>0;strongS+=fabsf(s)>.5f;strongRipple+=fabsf(ripple)>.5f;
  bowS+=bow*s;bowRipple+=bow*ripple;sRipple+=s*ripple;
 }
 assert(rightBows>9000&&rightBows<11000&&strongS>9000&&strongS<11000&&strongRipple>9000&&strongRipple<11000);
 assert(fabs(bowS)/20000<.03&&fabs(bowRipple)/20000<.03&&fabs(sRipple)/20000<.03);

 // The seed: fixed by the round's flight, its start, muzzle and velocity.
 trajectory_t flight={TR_LINEAR,5000,0,{100,200,300},{6500,10,-20}},other;
 const unsigned seed=CG_TracerSeed(&flight);
 other=flight;other.trType=0;other.trDuration=99;assert(CG_TracerSeed(&other)==seed);
 other=flight;other.trTime++;assert(CG_TracerSeed(&other)!=seed);
 for(int i=0;i<3;i++){
  other=flight;other.trBase[i]=nextafterf(other.trBase[i],INFINITY);assert(CG_TracerSeed(&other)!=seed);
  other=flight;other.trDelta[i]=nextafterf(other.trDelta[i],INFINITY);assert(CG_TracerSeed(&other)!=seed);
 }
 // Rounds fired one after another bow either way about as often.
 int right=0;
 for(int i=0;i<10000;i++){other=flight;other.trTime=5000+50*i;right+=CG_TracerBend(CG_TracerSeed(&other),.5f)>0;}
 assert(right>4500&&right<5500);

 // Each frame a round's streak runs from its place on the trajectory, at most
 // cg_tracerlength long and never past the hit, with one seed all its flight.
 length.current.value=100;
 localEntity_t round={.pos={TR_LINEAR,5000,0,{10,20,30},{6500,0,0}},.tracerClipDist=1000};
 int frames=0;
 for(int t=5000;t<5000+1000000/6500;t++,frames++){
  cg->time=t;drawnCount=0;CG_AddMovingTracer(&round);
  assert(drawnCount==1&&drawnSeed==CG_TracerSeed(&round.pos));
  const float travelled=6.5f*(t-5000);
  assert(fabsf(drawnStart[0]-10-travelled)<2e-3f&&drawnStart[1]==20&&drawnStart[2]==30);
  assert(fabsf(drawnEnd[0]-drawnStart[0]-fminf(100,1000-travelled))<2e-3f&&drawnEnd[1]==20&&drawnEnd[2]==30);
 }
 localEntity_t next=round;next.pos.trTime+=50;cg->time=5100;CG_AddMovingTracer(&next);
 assert(drawnSeed!=CG_TracerSeed(&round.pos));
 printf("%d %d\n",streaks,frames);
 return 0;
}
'''

# The browser's streak is 100 units long; other builds keep the original 160.
main_source = (ROOT / 'src/PC/cgame_mp/cg_main_mp.c').read_text()
web_dvars, native_dvars = re.search(r'#ifdef __EMSCRIPTEN__\n(    cg_tracerChance = .*?)#else\n(.*?)#endif',
                                    main_source, re.S).groups()
assert 'Dvar_RegisterFloat("cg_tracerlength", 100.0f, 0.0f, FLT_MAX, 0x1080);' in web_dvars, web_dvars
assert 'Dvar_RegisterFloat("cg_tracerlength", 160.0f, 0.0f, FLT_MAX, 0x1080);' in native_dvars, native_dvars

tracer_mutants = [
    ('#define CG_TRACER_SEGMENTS 8', '#define CG_TRACER_SEGMENTS 4'),
    ('#define CG_TRACER_BEND 0.07f', '#define CG_TRACER_BEND 0.0f'),
    ('#define CG_TRACER_BEND 0.07f', '#define CG_TRACER_BEND 0.1f'),
    ('(bow < 0.0f ? 0.4f * bow - 0.6f : 0.4f * bow + 0.6f)', 'bow'),
    ('0.5f * s * sinf(2.0f * pi * frac)', '0.0f'),
    (' + 0.25f * ripple * sinf(3.0f * pi * frac)', ''),
    ('sinf(pi * frac)', 'cosf(pi * frac)'),
    ('CG_TracerHash(seed + index * 0x9e3779b9u)', 'CG_TracerHash(seed)'),
    ('memcpy(&words[0], &pos->trTime, sizeof(words[0]));', 'words[0] = 0;'),
    ('memcpy(&words[1], pos->trBase, 3 * sizeof(words[0]));', 'memset(&words[1], 0, 3 * sizeof(words[0]));'),
    ('memcpy(&words[4], pos->trDelta, 3 * sizeof(words[0]));', 'memset(&words[4], 0, 3 * sizeof(words[0]));'),
    ('const float along = nearPart + farPart > 0.0f ? nearPart / (nearPart + farPart) : frac;',
     'const float along = frac;'),
    ('const float nearPart = startDist * sinf(frac * angle);', 'const float nearPart = finishDist * sinf(frac * angle);'),
    ('offset = CG_TracerBend(seed, frac) * angle * Vec3Distance(points[k], viewOrg);',
     'offset = CG_TracerBend(seed, frac) * angle * startDist;'),
    ('offset = CG_TracerBend(seed, frac) * angle * Vec3Distance(points[k], viewOrg);',
     'offset = CG_TracerBend(seed, frac) * Vec3Distance(points[k], viewOrg);'),
    ('points[k][i] += offset * right[i];', 'points[k][i] += offset * cg->refdef.viewaxis[2][i];'),
    ('Vec3Cross(tangent, toPoint, sides[k]);', 'Vec3Cross(dir, toPoint, sides[k]);'),
    ('if (Vec3Normalize(sides[k]) < 0.0001f)', 'if (0)'),
    ('widths[k] = startWidth + along * (finishWidth - startWidth);',
     'widths[k] = startWidth + frac * (finishWidth - startWidth);'),
    ('    startWidth *= 0.35f;', '    startWidth *= 0.5f;'),
    ('CG_DrawTracer(start, end, CG_TracerSeed(&le->pos));', 'CG_DrawTracer(start, end, 0);'),
    ('    if (dist > cg_tracerLength->current.value)\n        dist = cg_tracerLength->current.value;\n', ''),
]


def mutate(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


def run(directory, name, source, flags, expect_pass):
    path = Path(directory) / f'{name}.c'
    binary = Path(directory) / name
    path.write_text(source)
    subprocess.run(['cc', '-std=gnu11', '-D__EMSCRIPTEN__', '-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', *flags, str(path), '-lm', '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert (result.returncode == 0) == expect_pass, (name, result.stdout[-500:], result.stderr[-3000:])
    return result


with tempfile.TemporaryDirectory(prefix='cod2-combat-presentation-') as tmp:
    path = Path(tmp)
    (path / 'test.c').write_text(support + code + checks)
    subprocess.run(['cc', '-D__EMSCRIPTEN__', '-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
    streaks, frames = map(int, run(tmp, 'tracer', tracer_support + tracer_code + tracer_checks,
                                   ['-Wall', '-Werror', '-Wno-unused-function'], True).stdout.split())
    for index, (before, after) in enumerate(tracer_mutants):
        run(tmp, f'tracer{index}', tracer_support + mutate(tracer_code, before, after) + tracer_checks, ['-w'], False)
print(f'PASS: {streaks} tracer streaks, translated, off-axis, end-on, side-on, oblique, oncoming, passing and from '
      f'the hip and aimed: one ribbon of eight segments facing the eye with its glow, evenly spaced as seen, from the '
      f'muzzle to the hit on the shot\'s line and bent across itself by 7% of its length on screen, a bow of 60 to '
      f'100% either way, an S and a ripple, mixed by a seed fixed for each round\'s flight; {frames} frames of a '
      f'round\'s streak at most 100 units of cg_tracerlength and never past the hit; {len(tracer_mutants)} mutants '
      f'fail')
