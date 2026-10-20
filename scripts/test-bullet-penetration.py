#!/usr/bin/env python3
"""Fire production bullets through production brush collision under sanitizers."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent


def function(path, name):
    source = (root / path).read_text()
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef float vec_t,vec3_t[3];typedef int qboolean;
typedef unsigned char byte;
typedef struct {vec3_t start,end,invDelta;} TraceExtents;
typedef struct {TraceExtents extents;vec3_t size,midpoint,delta,halfDelta,halfDeltaAbs;
 float deltaLenSq,deltaLen,radius,offsetZ;vec3_t bounds[2],radiusOffset;
 int contents,isPoint,axialCullOnly;} traceWork_t;
typedef struct {float fraction;vec3_t normal;int surfaceFlags,contents,entityNum,partGroup,startsolid,allsolid;} trace_t;
typedef struct {vec3_t normal;float dist;} cplane_t;
typedef struct {cplane_t *plane;int materialNum;} cbrushside_t;
typedef struct {vec3_t mins,maxs;int contents,numsides,axialMaterialNum[2][3];cbrushside_t *sides;} cbrush_t;
static void CM_InitTraceThreadInfo(traceWork_t *tw) {}
static void CM_SetTraceMaterial(trace_t *t,int m,int c){t->contents=c;if(m>=0)t->surfaceFlags=m;}
#define WEAPCLASS_SPREAD 3
typedef struct {int bRifleBullet,armorPiercing,weapClass,damage,minDamage;float fMaxDamageRange,fMinDamageRange;} WeaponDef;
typedef struct {WeaponDef *weapDef;vec3_t muzzleTrace;} weaponParms;
typedef struct entity {struct {int number,eType,eventParm,eventParm2,surfType,otherEntityNum;} s;void *client;int takedamage;} gentity_t,gentity_s;
static gentity_t entities[1024],effect;
static void *imp_g_entities=entities,*imp_riflePriorityMap,*imp_bulletPriorityMap;
static struct {int time;} level;
static int traces,damageCalls,receivedDamage,events,brushCount,hitPlayer=1,team=1;
static float targetX=200;
static cbrush_t brushes[8];
static void Com_DPrintf(const char *s,...) {assert(0);}
static float Vec3Normalize(float *v){float n=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(n)for(int i=0;i<3;i++)v[i]/=n;return n;}
static void G_CheckHitTriggerDamage(gentity_t *a,vec_t *s,vec_t *e,int d,int mod) {}
/* Each stretch the bullet flies, for the mines along it. */
static int segments;static vec3_t segmentStart[8],segmentEnd[8];
static void G_BulletHitsMines(gentity_t *a,const vec_t *s,const vec_t *e) {
 assert(a==entities&&segments<8);memcpy(segmentStart[segments],s,sizeof(vec3_t));memcpy(segmentEnd[segments++],e,sizeof(vec3_t));}
static byte DirToByte(const vec_t *v){return 0;}
static gentity_t *G_TempEntity(const vec_t *p,int event){events++;return &effect;}
static void G_Damage(gentity_t *t,gentity_t *i,gentity_t *a,const void *dir,const vec_t *p,int d,int flags,int mod,int loc,int offset){assert(t==entities+1);damageCalls++;receivedDamage=d;}
static int Dvar_GetInt(const char *name){return 0;}
static int OnSameTeam(gentity_t *a,gentity_t *b){return team;}
'''
collision = ''.join(function('src/PC/qcommon/cm_tracebox.c', n) for n in (
    'CM_CalcTraceEntents', 'CM_TraceBox'))
collision += ''.join(function('src/PC/qcommon/cm_trace.c', n) for n in (
    'CM_DotProduct', 'CM_AbsFloat', 'CM_MinFloat', 'CM_InitTraceWork',
    'CM_TraceMayHitBounds', 'CM_ClipBrushPlaneDistances',
    'CM_TraceBrushAxialPlane', 'CM_TraceBrushPlane', 'CM_TraceBrushPlanes'))
trace_adapter = r'''
static void G_LocationalTrace(trace_t *tr,const vec_t *start,const vec_t *end,int skip,int mask,unsigned char *priority) {
 traces++;assert(traces<=10);
 *tr=(trace_t){.fraction=1,.entityNum=1023};
 traceWork_t tw;vec3_t zero={0};CM_InitTraceWork(&tw,start,end,zero,zero,mask);
 for(int i=0;i<brushCount;i++)if(brushes[i].contents&mask)CM_TraceBrushPlanes(&tw,brushes+i,tr);
 if(tr->fraction<1)tr->entityNum=1022;
 if(hitPlayer && skip!=1 && (mask&0x2000000) && start[0]<targetX && end[0]>=targetX) {
  float f=(targetX-start[0])/(end[0]-start[0]);
  if(f<tr->fraction){tr->fraction=f;tr->normal[0]=-1;tr->entityNum=1;tr->surfaceFlags=tr->contents=0;}
 }
}
'''
bullet = ''.join(function('src/PC/game_mp/g_weapon_mp.c', n) for n in (
    'Bullet_PenetrationDepth', 'Bullet_FindExit', 'Bullet_Fire_Extended'))
checks = r'''
static void slab(float x,float thick,int material) {
 assert(brushCount<8);cbrush_t *b=brushes+brushCount++;
 *b=(cbrush_t){.contents=material==9?16:1,.mins={x,-10000,-10000},.maxs={x+thick,10000,10000}};
 for(int i=0;i<3;i++)b->axialMaterialNum[0][i]=b->axialMaterialNum[1][i]=material<<20;
}
static int fire(WeaponDef *w,float y,float startX) {
 weaponParms wp={.weapDef=w,.muzzleTrace={startX,0,0}};
 vec3_t end={8192,y,0},start;memcpy(start,wp.muzzleTrace,sizeof(start));
 damageCalls=receivedDamage=traces=events=segments=0;
 Bullet_Fire_Extended(entities,(gentity_s (*)[16])entities,start,end,1,0,&wp,entities,0);
 assert(!memcmp(start,wp.muzzleTrace,sizeof(start))); // Each pellet keeps its original muzzle.
 assert(damageCalls<=1 && events<=4);
 return receivedDamage;
}
int main(void) {
 entities[0].s.number=0;entities[1]=(gentity_t){.s.number=1,.client=entities,.takedamage=1};
 WeaponDef gun={.damage=100,.minDamage=20,.fMaxDamageRange=400,.fMinDamageRange=2000};
 assert(fire(&gun,0,0)==100 && traces==1);
 hitPlayer=0;assert(fire(&gun,0,0)==0 && traces==1);hitPlayer=1;
 int cases=0;
 for(int rifle=0;rifle<2;rifle++)for(int material=0;material<32;material++) {
  gun.bRifleBullet=rifle;
  float depth=Bullet_PenetrationDepth(&gun,material<<20);
  int last=100;
  for(int quarter=1;quarter<=320;quarter++) {
   float thick=quarter*.25f;brushCount=0;slab(100,thick,material);
   int damage=fire(&gun,0,0);
   assert(damage<=last);last=damage;
   if(depth==0){assert(damage==0 && traces==1);}
   else if(thick<depth*.5f)assert(damage>0 && damage<100);
   if(thick>=depth)assert(!damage);
   assert(traces<=4);cases++;
  }
 }
 gun.bRifleBullet=1;brushCount=0;slab(100,8,21);
 int wood=fire(&gun,0,0);assert(wood>0);
 int oblique=fire(&gun,8192,0);assert(oblique>0 && oblique<wood);
 assert(!fire(&gun,8192*4,0)); // Same board, much longer path at a grazing angle.
 gun.bRifleBullet=0;assert(fire(&gun,0,0)<wood);
 gun.weapClass=WEAPCLASS_SPREAD;assert(!fire(&gun,0,0));gun.weapClass=0;
 gun.bRifleBullet=1;gun.armorPiercing=1;assert(fire(&gun,0,0)>wood);gun.armorPiercing=0;
 // A player immediately behind the surface isn't swallowed by the exit probe.
 targetX=108.5f;assert(fire(&gun,0,0)==wood);targetX=200;
 // A hard wall behind soft cover must stop the shot, including tiny air gaps.
 for(int gap=1;gap<=100;gap++) {
  brushCount=0;slab(100,2,21);slab(102+gap*.125f,50,17);
  assert(!fire(&gun,0,0));assert(traces<=4);
 }
 // A different hard material on the back face cannot inherit the soft entry.
 brushCount=0;slab(100,4,21);brushes[0].axialMaterialNum[1][0]=17<<20;
 assert(!fire(&gun,0,0));
 // Layer limit, accumulated energy, and no skipping a close second board.
 int previous=100;
 for(int layers=1;layers<=4;layers++) {
  brushCount=0;for(int n=0;n<layers;n++)slab(100+n*3,1,21);
  int damage=fire(&gun,0,0);assert(damage<previous);previous=damage;
  if(layers<4)assert(damage>0);else assert(!damage);
 }
 // Range damage uses distance from the muzzle, not from the last exit.
 brushCount=0;slab(100,8,21);targetX=1900;int far=fire(&gun,0,0);
 assert(far>0&&far<wood/2);targetX=200;
 // Never escape a solid in which the shot originated; sky/noimpact stay solid.
 assert(!fire(&gun,0,104));
 for(int flag=4;flag<=16;flag+=12) {
  for(int i=0;i<3;i++)brushes[0].axialMaterialNum[0][i]=brushes[0].axialMaterialNum[1][i]=(21<<20)|flag;
  assert(!fire(&gun,0,0)&&traces==1);
 }
 // Existing rifle flesh penetration still respects friendly fire.
 brushCount=0;team=0;assert(fire(&gun,0,0)==100 && traces==2);team=1;
 // Mines are checked along each stretch the bullet flies, from the muzzle to
 // the board, then from its far side to the player, for the shooter.
 brushCount=0;slab(100,8,21);assert(fire(&gun,0,0)==wood&&segments==2);
 assert(segmentStart[0][0]==0&&segmentEnd[0][0]>99.5f&&segmentEnd[0][0]<=100&&segmentStart[1][0]>=108&&
  segmentStart[1][0]<108.5f&&fabsf(segmentEnd[1][0]-200)<.01f);
 hitPlayer=0;brushCount=0;fire(&gun,0,0);assert(segments==1&&segmentEnd[0][0]==8192);hitPlayer=1;
 printf("PASS: %d material/thickness shots using actual brush collision; angles, layers, hard back faces, close targets, full range, glass, shotgun, trace budgets and flesh\n",cases);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-penetration-') as directory:
    path = Path(directory)
    (path / 'test.c').write_text(support + collision + trace_adapter + bullet + checks)
    subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                    str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
