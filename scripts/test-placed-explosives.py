#!/usr/bin/env python3
"""Run the S-Mine's and C4's production code: setting the mine down at its
owner's feet, arming and the enemy's step, an enemy shooting it, the owner
taking it back, sticking, the detonator, owners and limits, who is sent one
through walls, weapon file keys, the HUD's pulsing squad emblems and G
toggle."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
missiles = (root / 'src/PC/game_mp/g_missile_mp.c').read_text()
team = (root / 'src/PC/game_mp/g_team_mp.c').read_text()
weapons = (root / 'src/PC/bgame/bg_weapons.c').read_text()
loader = (root / 'src/PC/bgame/bg_weapons_load_obj.c').read_text()
shared = (root / 'src/PC/universal/q_shared.c').read_text()
hud = (root / 'src/PC/cgame_mp/cg_explosives_mp.c').read_text()
snapshot = (root / 'src/PC/server_mp/sv_snapshot_mp.c').read_text()
header = (root / 'src/headers/cod2_grenade.h').read_text()


def function(source, name):
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


def defines(source, prefix):
    return '\n'.join(line for line in source.splitlines() if line.startswith('#define ' + prefix)) + '\n'


common = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
typedef int qboolean;typedef int Bool;typedef unsigned char byte;typedef float vec_t;typedef float vec3_t[3];typedef float vec4_t[4];
#define WEAPTYPE_GRENADE 1
#define OFFHAND_CLASS_FRAG_GRENADE 1
typedef struct {int weapType,offhandClass,bCookOffHold,iFuseTime,damage,iExplosionRadius,iExplosionOuterDamage,
 iExplosionInnerDamage,weapClass,projExplosion,iClipIndex,iClipSize,iHoldFireTime,iMaxAmmo,quickDropTime,iProjectileSpeed,
 hasDetonator,stickiness,iProximityArmTime,iProximityDelay,iAltWeaponIndex;float proximityRadius;
 float parallelBounce[32],perpendicularBounce[32],fAdsAimPitch,destabilizeDistance,destabilizationTimeReductionRatio,
 destabilizationBaseTime;const char *szProjExplosionEffect,*killIcon,*szInternalName;
 char *szBounceSound,*szArmSound,*szTriggerSound,*szDetonateSound,*szPickupSound;vec3_t vGunOfs;}WeaponDef;
'''

# The server: g_missile_mp.c's placed explosives with the missile code around
# them, and the real OnSameTeam.
server_support = common + r'''
typedef struct {int trType,trTime,trDuration;vec3_t trBase,trDelta;} trajectory_t;
typedef struct {float fraction;int startsolid,entityNum,surfaceFlags,partName;vec3_t normal;}trace_t;
typedef struct {int pm_flags,pm_type,weaponstate,weapon,weaponTime,weaponDelay,offHandIndex,grenadeTimeLeft,eFlags,
 weapAnim,cursorHint,cursorHintString,cursorHintEntIndex,commandTime,stats[6],ammoclip[128],weapons[4];vec3_t origin,
 viewangles;float viewHeightCurrent;}playerState_t;
typedef struct {int serverTime,buttons,weapon;}usercmd_t;
typedef struct {playerState_t ps;struct {int connected;usercmd_t oldcmd;struct {int team;}cs;int sessionState;}sess;
 int buttons,lastSpawnTime;}gclient_t;
#define SESS_STATE_SPECTATOR 2
typedef struct {int number,eType,eFlags,weapon,otherEntityNum,groundEntityNum,surfType,time,time2,eventParm,scale;
 vec3_t origin2;trajectory_t pos,apos;} entityState_t;
typedef struct gentity_s {
 entityState_t s;
 struct {int ownerNum,inuse,svFlags,contents,linked;vec3_t currentOrigin,currentAngles;}r;
 gclient_t *client;int takedamage,useCount,handler,flags,health,parent,nextthink,classname,damage,clipmask,freeAfterEvent;
 struct {float time;}grenade;int team,count,spawnflags,maxHealth;
}gentity_t;
typedef gentity_t gentity_s;typedef int SoundAlias;
static gentity_t g_entities[1024];static gclient_t clients[8];
static struct {int time,previousTime,num_entities;}level;
static struct {int grenade;}scr_const;
static struct {int methodOfDeath,splashMethodOfDeath;}entityHandlers[20];
static vec3_t zero;static byte *vec3_origin_ptr=(byte*)zero,*pPriorityMap;
static WeaponDef defs[4];
static int explosions,radiusCalls,bounces,sounds;static gentity_t *lastAttacker;static vec3_t blast;
static trace_t hit={.fraction=1,.entityNum=1023};
static gentity_t temps[64];static int tempCount;
#define _ENT(e) ((gentity_t *)(e))
#define G_ENTITY(n) (&g_entities[n])
#define LEVEL_TIME level.time
#define LEVEL_PREVIOUSTIME level.previousTime
#define HANDLER_MOD(n) entityHandlers[n].methodOfDeath
#define HANDLER_SPLASHMOD(n) entityHandlers[n].splashMethodOfDeath
enum {GMISSILE_ENTITYNUM_WORLD=1022,GMISSILE_EF_GRENADE_BOUNCE=0x1000000,GMISSILE_FL_GUIDED=0x10000,
 GMISSILE_FL_TURRET=0x20000,GMISSILE_FL_HELD=0x40000};
typedef struct {gentity_t *missile;int missileUseCount,playerSpawnCount;}heldGrenade_t;
static heldGrenade_t heldGrenades[64];
static WeaponDef *BG_GetWeaponDef(int w){assert(w>0&&w<4);return &defs[w];}
static int BG_GetNumWeapons(void){return 3;}
static int COD2_GEntityHandle(const gentity_t *e){return e ? (int)(e-g_entities)+1 : 0;}
static gentity_t *COD2_GEntityFromHandle(int n){return n ? &g_entities[n-1] : NULL;}
static gentity_t *G_Spawn(void){int n=level.num_entities++;assert(n<1022);gentity_t *e=&g_entities[n];memset(e,0,sizeof(*e));
 e->s.number=n;e->r.inuse=1;return e;}
static void G_GetPlayerViewOrigin(gentity_t *e,vec_t *v){memcpy(v,e->client->ps.origin,sizeof(vec3_t));}
static int G_SetOrigin(gentity_t *e,const vec_t *v){memmove(e->r.currentOrigin,v,sizeof(vec3_t));
 memmove(e->s.pos.trBase,v,sizeof(vec3_t));e->s.pos.trType=0;memset(e->s.pos.trDelta,0,sizeof(vec3_t));return 1;}
static int G_SetAngle(gentity_t *e,vec_t *v){vec3_t a;memcpy(a,v,sizeof(a));memcpy(e->r.currentAngles,a,sizeof(a));
 memcpy(e->s.apos.trBase,a,sizeof(a));e->s.apos.trType=0;return 1;}
static void SV_LinkEntity(gentity_t *e){e->r.linked=1;}
static void Scr_SetString(int *s,int v){*s=v;}
static void vectoangles(vec_t *v,vec_t *a){a[0]=0;a[1]=atan2f(v[1],v[0])*57.2957795f;a[2]=0;}
static float AngleNormalize360(float a){return a;}
static float flrand(float a,float b){return (a+b)*.5f;}
/* Stationary, linear, or falling (TR_GRAVITY, 5) at 800 units/s/s. */
static void BG_EvaluateTrajectory(trajectory_t *tr,int t,vec_t *v){
 float d=(t-tr->trTime)*.001f;for(int i=0;i<3;i++)v[i]=tr->trBase[i]+(tr->trType?tr->trDelta[i]*d:0);
 if(tr->trType==5)v[2]-=400.0f*d*d;}
static void BG_EvaluateTrajectoryDelta(trajectory_t *tr,int t,vec_t *v){memcpy(v,tr->trDelta,sizeof(vec3_t));
 if(tr->trType==5)v[2]-=800.0f*(t-tr->trTime)*.001f;}
static int SV_PointContents(vec_t *p,int n,int c){return 0;}
static int world,blockSight;static float floorZ,wallX=1e9f,floorNormal[3]={0,0,1};
static void G_TraceCapsule(trace_t *t,vec_t *a,vec_t *b,vec_t *c,vec_t *d,int skip,int mask){memset(t,0,sizeof(*t));
 t->fraction=blockSight?.5f:1;t->entityNum=blockSight?1022:1023;}
/* Without the world, every trace is `hit`; with it, a floor at floorZ and a
 * wall facing -x at wallX. */
static void G_LocationalTrace(trace_t *t,vec_t *a,vec_t *b,int skip,int mask,byte *p){
 if(!world){*t=hit;return;}
 memset(t,0,sizeof(*t));t->fraction=1;t->entityNum=1023;
 if(a[2]>=floorZ&&b[2]<floorZ){t->fraction=(a[2]-floorZ)/(a[2]-b[2]);memcpy(t->normal,floorNormal,sizeof(vec3_t));t->entityNum=1022;}
 if(a[0]<wallX&&b[0]>=wallX){float f=(wallX-a[0])/(b[0]-a[0]);
  if(f<t->fraction){t->fraction=f;t->normal[0]=-1;t->normal[1]=t->normal[2]=0;t->entityNum=1022;}}}
static int DirToByte(vec_t *v){return 0;}
static void G_AddEvent(gentity_t *e,int a,int b){if(a==0xbc){assert(e->s.eType==0);explosions++;}if(a==0xbb)bounces++;}
static int G_RadiusDamage(vec_t *o,gentity_t *e,gentity_t *a,float inner,float outer,float radius,gentity_t *i,int m){
 assert(inner==defs[e->s.weapon].iExplosionInnerDamage&&radius==defs[e->s.weapon].iExplosionRadius);
 lastAttacker=a;radiusCalls++;memcpy(blast,o,sizeof(blast));return 0;}
typedef void EffectTemplate;
static void Server_SwitchToValidFxScheduler(void){}
static EffectTemplate *FX_RegisterEffect(const char *name){return NULL;}
static float FX_GetEffectLength(EffectTemplate *effect){return 0;}
static float randomf(void){return .5f;}
static int LogAccuracyHit(gentity_t *e,gentity_t *a){return 0;}
static void G_Damage(gentity_t *e,gentity_t *i,gentity_t *a,vec_t *d,vec_t *p,int n,int f,int m,int c,int l){}
static void G_CheckHitTriggerDamage(gentity_t *a,vec_t *b,vec_t *c,int d,int m){}
static void G_GrenadeTouchTriggerDamage(gentity_t *e,vec_t *a,vec_t *b,int r,int m){}
static gentity_t *G_TempEntity(vec_t *o,int n){assert(n==0xb3&&tempCount<64);gentity_t *t=&temps[tempCount++];
 memset(t,0,sizeof(*t));sounds++;return t;}
static void G_FreeEntity(gentity_t *e){e->r.inuse=0;}
static void SnapVectorTowards(vec_t *a,vec_t *b){}
static float Vec3Normalize(vec_t *v){float n=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(n)for(int i=0;i<3;i++)v[i]/=n;return n;}
static float Vec3NormalizeTo(vec_t *a,vec_t *b){memcpy(b,a,sizeof(vec3_t));return Vec3Normalize(b);}
static void G_MissileLandAngles(gentity_t *e,trace_t *t,vec_t *a,qboolean b){memset(a,0,sizeof(vec3_t));}
void G_ExplodeMissile(gentity_t *e);
static int G_RunThink(gentity_t *e){if(e->nextthink>0&&e->nextthink<=level.time){e->nextthink=0;G_ExplodeMissile(e);}return 0;}
static SoundAlias G_SoundAliasIndex(const char *n){assert(n&&*n);return 1;}
static int s_placedSequence,s_placedSpawnTime[1024];
static unsigned char s_mineShotBy[1024];
static int s_mineListTime=-1,s_mineCount,s_minePickupStart[64],s_minePickupEntity[64];
static unsigned short s_mineList[128];
gentity_t *fire_grenade(gentity_t *,vec_t *,vec_t *,int,int);
static void G_GrenadeCookOff(gentity_t *p){assert(0);}
'''
server_body = function(team, 'OnSameTeam').replace('qboolean OnSameTeam', 'static qboolean OnSameTeam') + ''.join(
    function(missiles, name) for name in (
        'VectorCopy', 'VectorSubtract', 'VectorAdd', 'VectorScale', 'VectorClear', 'VectorLength', 'DotProduct',
        'VectorMA', 'SnapFloat', 'SnapVector', 'LerpPosition', 'G_GetHeldGrenade', 'G_CanPickUpGrenade',
        'G_FindNearbyGrenade', 'G_PlacedTeam', 'G_PlacedSound', 'G_PlacedOwner', 'G_SetDownMine',
        'G_PlaceExplosive', 'G_StickMissile', 'G_LaunchMine', 'G_RunPlacedExplosive', 'G_DetonateExplosives',
        'G_CanTakeMine',
        'G_ListMines', 'G_FindOwnMine', 'G_UpdateMinePickup', 'G_BulletHitsMines', 'G_PlacedExplosiveSentTo',
        'G_ExplodeMissile',
        'fire_grenade', 'G_BounceMissile', 'G_RunMissile'))
server_checks = r'''
enum {MINE=1,CHARGE=2,FRAG=3,WORLD=1022};
static gentity_t *player(int n,int side){gentity_t *p=&g_entities[n];memset(p,0,sizeof(*p));p->s.number=n;p->r.inuse=1;
 p->health=100;p->client=&clients[n];memset(p->client,0,sizeof(gclient_t));p->client->sess.connected=2;
 p->client->sess.cs.team=side;p->client->lastSpawnTime=700+n;return p;}
static void at(gentity_t *p,float x,float y,float z){p->r.currentOrigin[0]=x;p->r.currentOrigin[1]=y;p->r.currentOrigin[2]=z;}
static void reset(void){
 memset(g_entities,0,sizeof(g_entities));memset(clients,0,sizeof(clients));memset(defs,0,sizeof(defs));
 level.time=1000;level.previousTime=950;level.num_entities=72;s_placedSequence=0;s_mineListTime=-1;
 memset(s_placedSpawnTime,0,sizeof(s_placedSpawnTime));memset(s_mineShotBy,0,sizeof(s_mineShotBy));
 memset(s_minePickupStart,0,sizeof(s_minePickupStart));world=blockSight=0;floorZ=0;wallX=1e9f;
 floorNormal[0]=floorNormal[1]=0;floorNormal[2]=1;
 explosions=radiusCalls=bounces=sounds=tempCount=0;lastAttacker=NULL;hit=(trace_t){.fraction=1,.entityNum=1023};
 for(int i=1;i<4;i++){defs[i].weapType=WEAPTYPE_GRENADE;defs[i].offhandClass=1;defs[i].iMaxAmmo=2;
  for(int s=0;s<32;s++)defs[i].parallelBounce[s]=defs[i].perpendicularBounce[s]=.5f;}
 defs[MINE].proximityRadius=40;defs[MINE].iProximityArmTime=1200;defs[MINE].iProximityDelay=300;
 defs[MINE].iExplosionRadius=192;defs[MINE].iExplosionInnerDamage=300;defs[MINE].iExplosionOuterDamage=60;
 defs[MINE].szArmSound="arm";defs[MINE].szTriggerSound="click";defs[MINE].szBounceSound="land";
 defs[MINE].szPickupSound="pickup";defs[MINE].iClipIndex=5;defs[MINE].iClipSize=2;
 defs[CHARGE].hasDetonator=1;defs[CHARGE].stickiness=WEAPSTICKINESS_ALL;defs[CHARGE].offhandClass=0;
 defs[CHARGE].iExplosionRadius=256;defs[CHARGE].iExplosionInnerDamage=330;defs[CHARGE].iExplosionOuterDamage=70;
 defs[CHARGE].szBounceSound="stick";defs[FRAG].iFuseTime=3500;defs[FRAG].iExplosionRadius=256;
 defs[FRAG].iExplosionInnerDamage=200;entityHandlers[7].methodOfDeath=3;entityHandlers[7].splashMethodOfDeath=4;
}
static gentity_t *place(gentity_t *owner,int weapon){vec3_t start={0,0,0},dir={200,0,0};
 return fire_grenade(owner,start,dir,weapon,defs[weapon].iFuseTime);}
static void rest(gentity_t *m,float x,float y){vec3_t spot={x,y,0};G_SetOrigin(m,spot);m->s.groundEntityNum=WORLD;}
static void frame(gentity_t *m,int ms){level.previousTime=level.time;level.time+=ms;G_RunMissile(m);}
static gentity_t *armedMine(gentity_t *owner){for(int i=0;i<8;i++)at(&g_entities[i],5000+100*i,5000,0);
 gentity_t *m=place(owner,MINE);rest(m,0,0);frame(m,50);
 for(int t=0;t<1300;t+=50)frame(m,50);return m;}
/* Q3's AngleVectors: forward, left (axis[1]) and up. */
static void axes(const vec_t *a,vec_t *f,vec_t *l,vec_t *u){
 float p=a[0]*.0174532925f,y=a[1]*.0174532925f,r=a[2]*.0174532925f,sp=sinf(p),cp=cosf(p),sy=sinf(y),cy=cosf(y),
 sr=sinf(r),cr=cosf(r);
 f[0]=cp*cy;f[1]=cp*sy;f[2]=-sp;l[0]=sr*sp*cy-cr*sy;l[1]=sr*sp*sy+cr*cy;l[2]=sr*cp;u[0]=cr*sp*cy+sr*sy;
 u[1]=cr*sp*sy-sr*cy;u[2]=cr*cp;}
int main(void){
 int cases=0;
 /* Placed: no fuse, the owner and side recorded, a mine set down unspun. */
 reset();gentity_t *owner=player(0,2),*enemy=player(1,1),*mate=player(2,2);
 gentity_t *m=place(owner,MINE);
 assert(m->nextthink==0&&m->s.otherEntityNum==0&&m->team==2&&m->s.apos.trType==0&&m->r.currentAngles[0]==0);
 /* Arms 1.2 s after it comes to rest, with its plant sound; then an enemy's
  * step clicks it, 300 ms later it springs straight up, turning and through
  * anyone over it, and it goes off as it stops rising, 44 units up, a
  * standing player's chest: its owner's blast. */
 rest(m,0,0);at(enemy,10,0,0);frame(m,50);assert(m->count==level.time+1200&&sounds==1&&!(m->spawnflags&2));
 for(int t=0;t<1100;t+=50){frame(m,50);assert(!(m->spawnflags&2));}
 frame(m,100);assert((m->spawnflags&2)&&m->count==level.time+300&&!m->nextthink&&sounds==2);
 int launch=m->count;
 frame(m,250);assert(m->s.pos.trType==0&&m->r.currentOrigin[2]==0&&!explosions);
 frame(m,50);
 assert(m->s.pos.trType==5&&m->s.pos.trTime==launch&&!m->s.pos.trDelta[0]&&!m->s.pos.trDelta[1]&&
  fabsf(m->s.pos.trDelta[2]-sqrtf(2*800*44.0f))<1e-3f&&m->s.groundEntityNum==1023);
 assert(m->nextthink==launch+332&&!(m->clipmask&0x2000000)&&m->s.apos.trType==2&&m->s.apos.trDelta[1]==360);
 {
  float lastZ=m->r.currentOrigin[2];
  while(!explosions&&level.time<launch+1000){
   frame(m,50);
   if(!explosions){assert(m->r.currentOrigin[2]>lastZ&&!m->r.currentOrigin[0]&&!m->r.currentOrigin[1]);
    lastZ=m->r.currentOrigin[2];}
  }
 }
 assert(explosions==1&&radiusCalls==1&&lastAttacker==owner&&level.time>=launch+332&&level.time<launch+382);
 assert(fabsf(blast[2]-44)<1.5f&&!blast[0]&&!blast[1]);
 cases+=4;
 /* Brought back to rest before it goes off, it springs up only once. */
 reset();owner=player(0,2);enemy=player(1,1);m=armedMine(owner);at(enemy,10,0,0);frame(m,50);
 for(int k=0;k<6;k++)frame(m,50);
 assert(m->s.pos.trType==5);
 launch=m->s.pos.trTime;int due=m->nextthink;m->s.pos.trType=0;m->s.groundEntityNum=WORLD;
 frame(m,50);assert(m->nextthink==due&&m->s.pos.trTime==launch&&m->s.pos.trType==0);
 while(!explosions&&level.time<due+100)frame(m,50);
 assert(explosions==1&&level.time>=due&&level.time<due+50);cases++;
 /* Only a living, playing, connected enemy's feet within 40 units across and
  * 24 below to 48 above. */
 struct {int who,dead,state,connected;float x,z;int fires;} steps[]={
  {1,0,0,2,39.9f,0,1},{1,0,0,2,40.1f,0,0},{1,0,0,2,0,48,1},{1,0,0,2,0,48.5f,0},{1,0,0,2,0,-24,1},
  {1,0,0,2,0,-24.5f,0},{2,0,0,2,0,0,0},{0,0,0,2,0,0,0},{1,1,0,2,0,0,0},{1,0,2,2,0,0,0},{1,0,0,0,0,0,0}};
 for(unsigned i=0;i<sizeof(steps)/sizeof(steps[0]);i++){
  reset();owner=player(0,2);enemy=player(1,1);mate=player(2,2);m=armedMine(owner);
  gentity_t *who=&g_entities[steps[i].who];at(who,steps[i].x,0,steps[i].z);
  if(steps[i].dead)who->health=0;if(who!=owner||steps[i].state)who->client->sess.sessionState=steps[i].state;
  if(who!=owner)who->client->sess.connected=steps[i].connected;
  at(owner,500,0,0);if(who==owner)at(owner,0,0,0);if(who!=mate)at(mate,500,500,0);if(who!=enemy)at(enemy,-500,0,0);
  frame(m,50);assert(!!(m->spawnflags&2)==steps[i].fires);cases++;
 }
 /* Free-for-all: anyone but the owner, even on top of it. */
 reset();owner=player(0,0);enemy=player(1,0);m=armedMine(owner);at(owner,0,0,0);frame(m,50);assert(!(m->spawnflags&2));
 at(enemy,5,5,0);frame(m,50);assert(m->spawnflags&2);cases+=2;
 /* Gone with its owner, when the owner changes sides, spawns again or watches
  * as a spectator; there through the owner's death and its killcam. */
 for(int loss=0;loss<5;loss++){
  reset();owner=player(0,2);m=place(owner,loss&1?CHARGE:MINE);rest(m,0,0);frame(m,50);assert(m->r.inuse);
  if(loss==0)owner->client->sess.connected=0;
  if(loss==1)owner->client->sess.cs.team=1;
  if(loss==2)owner->client->lastSpawnTime=level.time;
  if(loss==3)owner->client->sess.sessionState=SESS_STATE_SPECTATOR;
  if(loss==4){owner->health=0;owner->client->sess.sessionState=1;}
  for(int k=0;k<4;k++)frame(m,50);
  assert(!m->r.inuse==(loss<4)&&!explosions);cases++;
 }
 /* Two of a kind each: a third replaces the oldest, apart from other kinds and players. */
 reset();owner=player(0,2);enemy=player(1,1);
 gentity_t *first=place(owner,MINE),*second=place(owner,MINE),*charge=place(owner,CHARGE),*theirs=place(enemy,MINE);
 gentity_t *third=place(owner,MINE);
 assert(!first->r.inuse&&second->r.inuse&&third->r.inuse&&charge->r.inuse&&theirs->r.inuse);cases++;
 /* C4 stays where it first hits a wall or floor, its flat side (-y) on it and
  * its top up the wall or along the throw; no bounce. */
 vec3_t normals[]={{-1,0,0},{0,1,0},{0,0,1},{0,0,-1},{.6f,0,.8f},{0,-.8f,.6f}};
 gentity_t *c;
 for(int n=0;n<6;n++){
  reset();owner=player(0,2);c=place(owner,CHARGE);
  hit=(trace_t){.fraction=.5f,.entityNum=WORLD};memcpy(hit.normal,normals[n],sizeof(vec3_t));
  frame(c,50);assert((c->spawnflags&1)&&c->s.pos.trType==0&&!bounces&&sounds==1&&c->s.groundEntityNum==WORLD);
  vec3_t f,l,u,spot;memcpy(spot,c->r.currentOrigin,sizeof(spot));axes(c->r.currentAngles,f,l,u);
  for(int k=0;k<3;k++)assert(fabsf(l[k]+normals[n][k])<1e-3f);
  assert(fabsf(u[0]*normals[n][0]+u[1]*normals[n][1]+u[2]*normals[n][2])<1e-3f);
  if(fabsf(normals[n][2])<.9f)assert(u[2]>0);else assert(fabsf(u[0]-1)<1e-3f);
  hit=(trace_t){.fraction=1,.entityNum=1023};
  for(int t=0;t<40;t++)frame(c,50);
  assert(!memcmp(spot,c->r.currentOrigin,sizeof(spot))&&!explosions&&c->r.inuse);cases++;
 }
 /* Stuck to a mover or door it stays put too, though nothing is under it. */
 reset();owner=player(0,2);c=place(owner,CHARGE);hit=(trace_t){.fraction=.5f,.entityNum=500};hit.normal[0]=-1;
 frame(c,50);assert((c->spawnflags&1)&&c->s.groundEntityNum==500);vec3_t still;memcpy(still,c->r.currentOrigin,sizeof(still));
 hit=(trace_t){.fraction=1,.entityNum=1023};for(int t=0;t<10;t++)frame(c,50);
 assert(!memcmp(still,c->r.currentOrigin,sizeof(still)));cases++;
 /* A player it hits bounces it off, as any grenade. */
 reset();owner=player(0,2);enemy=player(1,1);c=place(owner,CHARGE);
 hit=(trace_t){.fraction=.5f,.entityNum=1};hit.normal[0]=-1;frame(c,50);assert(!(c->spawnflags&1));cases++;
 /* The detonator sets off its owner's charges only, anywhere; not mines. */
 reset();owner=player(0,2);enemy=player(1,1);
 gentity_t *a=place(owner,CHARGE),*b=place(owner,CHARGE),*e=place(enemy,CHARGE),*mm=place(owner,MINE);
 hit=(trace_t){.fraction=.5f,.entityNum=WORLD};hit.normal[2]=1;frame(a,50);frame(b,50);frame(e,50);
 b->r.currentOrigin[0]=b->s.pos.trBase[0]=90000;rest(mm,0,0);
 G_DetonateExplosives(owner);assert(a->nextthink==level.time&&b->nextthink==level.time&&!e->nextthink&&!mm->nextthink);
 frame(a,0);frame(b,0);frame(e,0);assert(explosions==2&&lastAttacker==owner&&e->r.inuse);cases++;
 /* A charge thrown and set off before it lands goes off in the air. */
 reset();owner=player(0,2);c=place(owner,CHARGE);G_DetonateExplosives(owner);frame(c,50);assert(explosions==1);cases++;
 /* Placed explosives are never picked up and thrown back; frags still are. */
 reset();owner=player(0,2);gentity_t *receiver=player(1,1);at(receiver,0,0,0);receiver->client->ps.viewHeightCurrent=60;
 m=place(owner,MINE);rest(m,0,0);m->nextthink=level.time+100;c=place(owner,CHARGE);c->nextthink=level.time+100;
 assert(!G_FindNearbyGrenade(receiver));gentity_t *f=place(owner,FRAG);assert(f->nextthink>level.time&&G_FindNearbyGrenade(receiver)==f);
 cases++;
 /* The S-Mine is set down upright a step ahead of the feet, on the ground,
  * facing as its owner does; short of a wall; down a step; and falling from
  * the hand over a drop or onto a steep slope. */
 struct {float yaw,wall,floor,normal;int lands;float x,y,z;} downs[]={
  {0,1e9f,0,1,1,20,0,1.5f},{90,1e9f,0,1,1,0,20,1.5f},{0,10,0,1,1,6,0,1.5f},{0,1e9f,-30,1,1,20,0,-28.5f},
  {0,1e9f,-70,1,0,20,0,24},{0,1e9f,0,.6f,0,20,0,24}};
 for(unsigned i=0;i<sizeof(downs)/sizeof(downs[0]);i++){
  reset();owner=player(0,2);owner->client->ps.viewangles[1]=downs[i].yaw;world=1;wallX=downs[i].wall;
  floorZ=downs[i].floor;floorNormal[2]=downs[i].normal;floorNormal[0]=sqrtf(1-downs[i].normal*downs[i].normal);
  m=place(owner,MINE);
  assert(fabsf(m->r.currentOrigin[0]-downs[i].x)<1e-3f&&fabsf(m->r.currentOrigin[1]-downs[i].y)<1e-3f&&
   fabsf(m->r.currentOrigin[2]-downs[i].z)<1e-3f);
  assert(m->r.currentAngles[0]==0&&m->r.currentAngles[2]==0&&fabsf(m->r.currentAngles[1]-downs[i].yaw)<1e-3f&&
   m->s.apos.trType==0);
  /* Linked at once: at rest the missile code never would, and no client would see it. */
  assert(m->r.linked);
  if(downs[i].lands)assert(m->s.pos.trType==0&&m->s.groundEntityNum==1022);
  else assert(m->s.pos.trType==5&&m->s.groundEntityNum==1023&&m->s.pos.trTime==level.time);
  cases++;
 }
 /* Its owner takes a resting mine back by holding Use half a second within
  * reach: the hint names the mine's weapon, then the mine is gone and its
  * ammunition back, with the pickup sound. */
 const int USE=0x8;
 reset();owner=player(0,2);owner->client->ps.weapons[0]=1<<MINE;owner->client->ps.ammoclip[5]=1;
 m=place(owner,MINE);rest(m,20,0);
 level.time+=50;owner->client->ps.cursorHint=0;G_UpdateMinePickup(owner);
 assert(owner->client->ps.cursorHint==PLACED_PICKUP_HINT&&owner->client->ps.cursorHintString==MINE&&
  owner->client->ps.cursorHintEntIndex==1023&&m->r.inuse);
 owner->client->buttons=USE;
 int calls=0;
 while(m->r.inuse&&calls<20){level.time+=50;owner->client->ps.cursorHint=0;G_UpdateMinePickup(owner);calls++;}
 assert(calls==11&&!m->r.inuse&&owner->client->ps.ammoclip[5]==2&&sounds==1);cases++;
 /* Letting go starts the half second again; +usereload holds too. */
 reset();owner=player(0,2);owner->client->ps.weapons[0]=1<<MINE;m=place(owner,MINE);rest(m,20,0);
 owner->client->buttons=0x20;
 for(int k=0;k<8;k++){level.time+=50;owner->client->ps.cursorHint=0;G_UpdateMinePickup(owner);}
 owner->client->buttons=0;level.time+=50;owner->client->ps.cursorHint=0;G_UpdateMinePickup(owner);
 owner->client->buttons=0x20;calls=0;
 while(m->r.inuse&&calls<20){level.time+=50;owner->client->ps.cursorHint=0;G_UpdateMinePickup(owner);calls++;}
 assert(calls==11&&owner->client->ps.ammoclip[5]==1);cases++;
 /* A full kit, or a kit without the mine, takes it back too, without going
  * past the clip. */
 for(int c=0;c<2;c++){
  reset();owner=player(0,2);owner->client->ps.weapons[0]=c?1<<CHARGE:1<<MINE;owner->client->ps.ammoclip[5]=c?0:2;
  owner->client->buttons=USE;m=place(owner,MINE);rest(m,20,0);
  level.time+=50;owner->client->ps.cursorHint=0;G_UpdateMinePickup(owner);
  assert(owner->client->ps.cursorHint==PLACED_PICKUP_HINT&&owner->client->ps.cursorHintString==MINE&&m->r.inuse);
  calls=0;
  while(m->r.inuse&&calls<20){level.time+=50;owner->client->ps.cursorHint=0;G_UpdateMinePickup(owner);calls++;}
  assert(calls==10&&!m->r.inuse&&owner->client->ps.ammoclip[5]==(c?0:2)&&sounds==1);cases++;
 }
 /* Nobody else, nor out of reach, too far above or below, once it has
  * clicked, still falling, out of sight, dead, where another hint is
  * offered, or a charge. */
 for(int c=0;c<10;c++){
  reset();owner=player(0,2);mate=player(1,2);enemy=player(2,1);
  for(int k=0;k<3;k++){g_entities[k].client->ps.weapons[0]=1<<MINE|1<<CHARGE;g_entities[k].client->buttons=USE;}
  gentity_t *who=c==0?mate:c==1?enemy:owner;
  m=place(owner,c==9?CHARGE:MINE);rest(m,20,0);
  if(c==2)at(owner,-40,0,0);if(c==3)at(owner,0,0,-50);if(c==4)m->spawnflags|=2;if(c==5)m->s.pos.trType=5;
  if(c==6)blockSight=1;if(c==7)owner->health=0;
  for(int k=0;k<24;k++){level.time+=50;who->client->ps.cursorHint=c==8?7:0;G_UpdateMinePickup(who);
   assert(who->client->ps.cursorHint==(c==8?7:0));}
  assert(m->r.inuse);cases++;
 }
 /* Of two in reach, the nearer goes first. */
 reset();owner=player(0,2);owner->client->ps.weapons[0]=1<<MINE;owner->client->buttons=USE;
 gentity_t *near=place(owner,MINE),*far=place(owner,MINE);rest(near,10,0);rest(far,40,0);
 for(int k=0;k<11;k++){level.time+=50;owner->client->ps.cursorHint=0;G_UpdateMinePickup(owner);}
 assert(!near->r.inuse&&far->r.inuse);cases++;
 /* Sent wherever it lies within 1600 units of the eyes: a resting mine or
  * charge to its owner and teammates; never to enemies or spectators, nor a
  * frag, one still moving, one farther off, or one with no owner or weapon. */
 reset();owner=player(0,2);mate=player(1,2);enemy=player(2,1);player(3,3);
 m=place(owner,MINE);rest(m,0,0);c=place(owner,CHARGE);rest(c,0,0);f=place(owner,FRAG);rest(f,0,0);
 f->s.otherEntityNum=0;vec3_t within={1500,500,60},beyond={1600,0,60};
 assert(G_PlacedExplosiveSentTo(&m->s,within,0)&&G_PlacedExplosiveSentTo(&m->s,within,1)&&
  G_PlacedExplosiveSentTo(&c->s,within,1));
 assert(!G_PlacedExplosiveSentTo(&m->s,within,2)&&!G_PlacedExplosiveSentTo(&c->s,within,2)&&
  !G_PlacedExplosiveSentTo(&m->s,within,3));
 assert(!G_PlacedExplosiveSentTo(&f->s,within,0)&&!G_PlacedExplosiveSentTo(&m->s,beyond,0));
 m->s.pos.trType=5;assert(!G_PlacedExplosiveSentTo(&m->s,within,0));m->s.pos.trType=0;
 m->s.otherEntityNum=64;assert(!G_PlacedExplosiveSentTo(&m->s,within,0));m->s.otherEntityNum=0;
 m->s.weapon=0;assert(!G_PlacedExplosiveSentTo(&m->s,within,0));m->s.weapon=MINE;
 m->s.eType=1;assert(!G_PlacedExplosiveSentTo(&m->s,within,0));cases+=8;
 /* Free-for-all: only to its owner. */
 reset();owner=player(0,0);enemy=player(1,0);m=place(owner,MINE);rest(m,0,0);
 assert(G_PlacedExplosiveSentTo(&m->s,within,0)&&!G_PlacedExplosiveSentTo(&m->s,within,1));cases++;
 {
  /* An enemy's bullet through the can sets a resting mine off, armed or
   * still arming: on the mine's next frame, not during the shot, and the
   * shooter's blast. */
  vec3_t from={-300,0,2},to={300,0,2};
  for(int armed=0;armed<2;armed++){
   reset();owner=player(0,2);enemy=player(1,1);
   if(armed)m=armedMine(owner);else{m=place(owner,MINE);rest(m,0,0);frame(m,50);}
   at(owner,500,0,0);at(enemy,-500,0,0);
   G_BulletHitsMines(enemy,from,to);assert(!explosions&&m->s.eType==4);
   frame(m,50);assert(explosions==1&&radiusCalls==1&&lastAttacker==enemy&&m->s.eType==0);cases++;
  }
  /* Within 5 units of a point 2 over its origin, along the bullet's own
   * path only: not where it stopped short, nor before it started. */
  struct {vec3_t a,b;int fires;} paths[]={
   {{-300,4.9f,2},{300,4.9f,2},1},{{-300,5.1f,2},{300,5.1f,2},0},{{0,0,300},{0,0,6.9f},1},{{0,0,300},{0,0,7.1f},0},
   {{-300,0,2},{-4.9f,0,2},1},{{-300,0,2},{-5.1f,0,2},0},{{5.1f,0,2},{300,0,2},0},{{4.9f,0,2},{300,0,2},1}};
  for(unsigned i=0;i<sizeof(paths)/sizeof(paths[0]);i++){
   reset();owner=player(0,2);enemy=player(1,1);m=place(owner,MINE);rest(m,0,0);at(owner,500,0,0);at(enemy,-500,0,0);
   frame(m,50);G_BulletHitsMines(enemy,paths[i].a,paths[i].b);frame(m,50);assert(explosions==paths[i].fires);cases++;
  }
  /* Only an enemy's bullet: not a teammate's or its owner's; anyone's but
   * the owner's in free-for-all. */
  struct {int side,who,fires;} shooters[]={{2,2,0},{2,0,0},{2,1,1},{0,1,1},{0,0,0}};
  for(unsigned i=0;i<sizeof(shooters)/sizeof(shooters[0]);i++){
   reset();owner=player(0,shooters[i].side);enemy=player(1,shooters[i].side?1:0);mate=player(2,shooters[i].side);
   m=place(owner,MINE);rest(m,0,0);at(owner,500,0,0);at(enemy,-500,0,0);at(mate,0,500,0);frame(m,50);
   G_BulletHitsMines(&g_entities[shooters[i].who],from,to);frame(m,50);assert(explosions==shooters[i].fires);cases++;
  }
  /* Not C4, nor a mine still falling, nor one that has clicked: that one
   * goes off when it would, its owner's. */
  reset();owner=player(0,2);enemy=player(1,1);at(owner,500,0,0);at(enemy,-500,0,0);
  c=place(owner,CHARGE);rest(c,0,0);frame(c,50);G_BulletHitsMines(enemy,from,to);frame(c,50);
  assert(!explosions&&c->r.inuse&&c->s.eType==4);
  m=place(owner,MINE);rest(m,0,0);m->s.pos.trType=5;G_BulletHitsMines(enemy,from,to);assert(!s_mineShotBy[m->s.number]);
  reset();owner=player(0,2);enemy=player(1,1);gentity_t *gunner=player(3,1);m=armedMine(owner);
  at(enemy,10,0,0);frame(m,50);assert(m->spawnflags&2);at(enemy,-500,0,0);at(gunner,-500,0,0);
  G_BulletHitsMines(gunner,from,to);assert(!s_mineShotBy[m->s.number]);frame(m,100);assert(!explosions);
  for(int k=0;k<20&&!explosions;k++)frame(m,50);
  assert(explosions==1&&lastAttacker==owner&&fabsf(blast[2]-44)<1.5f);cases+=3;
  /* Of two on the bullet's path, the nearer goes off; the other stays. */
  reset();owner=player(0,2);enemy=player(1,1);at(owner,500,500,0);at(enemy,-500,0,0);
  gentity_t *nearMine=place(owner,MINE),*farMine=place(owner,MINE);rest(nearMine,-100,0);rest(farMine,100,0);
  frame(nearMine,50);frame(farMine,0);G_BulletHitsMines(enemy,from,to);frame(nearMine,50);frame(farMine,0);
  assert(explosions==1&&nearMine->s.eType==0&&farMine->s.eType==4&&farMine->r.inuse);cases++;
  /* The first shooter's; the owner's once that shooter has left; gone
   * without a blast with an owner who has left. */
  for(int leave=0;leave<3;leave++){
   reset();owner=player(0,2);enemy=player(1,1);gunner=player(3,1);m=place(owner,MINE);rest(m,0,0);
   at(owner,500,0,0);at(enemy,-500,0,0);at(gunner,-500,50,0);frame(m,50);
   G_BulletHitsMines(enemy,from,to);G_BulletHitsMines(gunner,from,to);
   if(leave==1)enemy->client->sess.connected=0;
   if(leave==2)owner->client->sess.connected=0;
   frame(m,50);
   if(leave<2)assert(explosions==1&&lastAttacker==(leave?owner:enemy));else assert(!explosions&&!m->r.inuse);cases++;
  }
  /* A new mine where a shot one lay starts unshot. */
  reset();owner=player(0,2);at(owner,500,0,0);s_mineShotBy[level.num_entities]=2;m=place(owner,MINE);rest(m,0,0);
  frame(m,50);frame(m,50);assert(!explosions&&m->s.eType==4&&m->r.inuse);cases++;
 }
 printf("PASS: %d server cases: a mine arms 1.2 s after it lies still, clicks under an enemy's feet within 40 units and 24 below to 48 above, springs up 300 ms later, turning and through players, and goes off at its top 44 units up for its owner, once, never for teammates, its owner, the dead, spectators or the disconnected; anyone in free-for-all; C4 sticks flat to walls, floors, ceilings and slopes, bounces off players, and goes off only by its owner's detonator, anywhere, in flight too; two of each kind per owner; both go with an owner who leaves or changes sides; neither is thrown back; the S-Mine is set down upright a step ahead of its owner's feet, short of walls, down steps, falling over drops and steep slopes; only its owner takes it back, holding Use half a second within reach, in sight, where no other hint is offered, the nearer first, with a full kit or none too, refunded only into room in a kit that carries it; a resting one is sent to its owner and teammates within 1600 units wherever it lies, never to enemies, spectators or others in free-for-all; an enemy's bullet within 5 units of a resting mine's can, along its own path, sets the nearer of two off on the mine's next frame, armed or arming, never a teammate's or its owner's, nor C4, a falling or clicked mine, the first shooter's blast unless they left\n",cases);
 return 0;
}
'''

# Pmove: the detonator on the aim button, G's quiet fallback, no cooking.
pmove_support = common + r'''
#define OFFHAND_CLASS_NONE 0
#define OFFHAND_CLASS_SMOKE_GRENADE 2
#define BM_NOINLINE
#define BM_ALWAYS_INLINE inline
#define __attribute_regparm__(n)
typedef struct {int weapon,weaponstate,weaponTime,weaponDelay,weapAnim,pm_flags,eFlags,offHandIndex,cursorHint,
 weapons[4],eventSequence,events[4],eventParms[4];signed char weaponslots[3];} playerState_t;
typedef struct {int buttons,weapon,serverTime;} usercmd_t;
typedef struct {playerState_t *ps;usercmd_t cmd,oldcmd;} pmove_t;
static WeaponDef defs[4],*bg_weaponDefs[4]={0,&defs[1],&defs[2],&defs[3]};static int bg_iNumWeapons=3;
static int firstOffhand,prepared;
static void BG_AddPredictableEventToPlayerstate(int e,int p,playerState_t *ps){int i=ps->eventSequence++&3;ps->events[i]=e;ps->eventParms[i]=p;}
static void PM_AddEvent(playerState_t *ps,int e){BG_AddPredictableEventToPlayerstate(e,0,ps);}
static void PM_WeaponSetAnim(playerState_t *ps,int a){ps->weapAnim=a;}
static int BG_GetFirstAvailableOffhand(playerState_t *ps,int offhandClass){return firstOffhand;}
static void PM_StartOffhandPrepare(playerState_t *ps){prepared++;}
static void PM_SetProneMovementOverride(playerState_t *ps){}
static void Com_Error(int code,const char *fmt,...){assert(0);}
static int PM_WeaponStateBlocksChange(int state){return 0;}
static int PM_WeaponStateIsBinoculars(int state){return 0;}
'''
pmove_constants = weapons[weapons.index('#define WEAPON_READY'):weapons.index('static qboolean PM_WeaponHasInventoryWeapon')]
pmove_body = ''.join(function(weapons, name) for name in (
    'BG_IsWeaponValid', 'BG_PlayerDetonatorWeapon', 'PM_OffhandClassForButtons', 'PM_TryStartOffhand',
    'PM_TryDetonate'))
pmove_checks = r'''
enum {GUN=1,CHARGE=2,MINE=3};
static int detonations(const playerState_t *ps){int n=0;for(int i=0;i<4&&i<ps->eventSequence;i++)n+=ps->events[i]==EV_DETONATE;return n;}
int main(void){
 defs[GUN].weapType=0;defs[CHARGE].weapType=WEAPTYPE_GRENADE;defs[CHARGE].hasDetonator=1;defs[CHARGE].iHoldFireTime=350;
 defs[MINE].weapType=WEAPTYPE_GRENADE;defs[MINE].offhandClass=1;defs[MINE].proximityRadius=40;
 /* A press of aim with the detonator out sets it off once, with the hand's press. */
 playerState_t ps={.weapon=CHARGE,.weapons={1<<CHARGE|1<<GUN}};pmove_t pm={.ps=&ps};
 pm.cmd.buttons=PM_WEAPON_BUTTON_ADS;assert(PM_TryDetonate(&pm)&&detonations(&ps)==1);
 assert(ps.weaponstate==WEAPON_RAISING&&ps.weaponTime==350&&ps.weapAnim==0x13);
 ps.weaponstate=WEAPON_READY;ps.weaponTime=0;pm.oldcmd=pm.cmd;assert(!PM_TryDetonate(&pm)&&detonations(&ps)==1);
 pm.oldcmd.buttons=0;ps.weaponTime=10;assert(!PM_TryDetonate(&pm));ps.weaponTime=0;ps.weaponstate=WEAPON_FIRING;
 assert(!PM_TryDetonate(&pm));ps.weaponstate=WEAPON_READY;ps.weapon=GUN;assert(!PM_TryDetonate(&pm));
 pm.cmd.buttons=PM_WEAPON_BUTTON_ATTACK;ps.weapon=CHARGE;assert(!PM_TryDetonate(&pm));
 /* G with C4 and no frags: no empty-offhand notice; without C4 the notice. */
 playerState_t g={.weapon=GUN,.weapons={1<<CHARGE|1<<GUN}};pmove_t gm={.ps=&g,.cmd={.buttons=PM_WEAPON_BUTTON_FRAG}};
 firstOffhand=0;assert(!PM_TryStartOffhand(&gm)&&g.eventSequence==0);
 g.weapons[0]=1<<GUN;assert(!PM_TryStartOffhand(&gm)&&g.eventSequence==1&&g.events[0]==0x94);
 assert(BG_PlayerDetonatorWeapon(&g)==0);g.weapons[0]|=1<<CHARGE;assert(BG_PlayerDetonatorWeapon(&g)==CHARGE);
 /* The server takes a command naming the carried detonator, which has no slot;
    any other slotless weapon still names the current one. */
 playerState_t v={.weapon=GUN,.weapons={1<<CHARGE}};assert(BG_IsWeaponValid(&v,CHARGE)&&!BG_IsWeaponValid(&v,GUN));
 v.weapons[0]=1<<GUN;assert(!BG_IsWeaponValid(&v,CHARGE)&&!BG_IsWeaponValid(&v,GUN));v.weaponslots[1]=GUN;
 assert(BG_IsWeaponValid(&v,GUN));
 /* Neither cooks in the hand; a frag does. */
 WeaponDef frag={.weapType=WEAPTYPE_GRENADE,.offhandClass=1};
 assert(BG_GrenadeCanCook(&frag)&&!BG_GrenadeCanCook(&defs[MINE])&&!BG_GrenadeCanCook(&defs[CHARGE]));
 assert(BG_IsPlacedExplosive(&defs[MINE])&&BG_IsPlacedExplosive(&defs[CHARGE])&&!BG_IsPlacedExplosive(&frag));
 puts("PASS: a fresh aim press with the detonator out sets it off once, with the hand's press, and nothing else does; G without frags takes no empty-offhand notice when C4 is carried; the server takes the carried detonator as the current weapon; mines and C4 never cook");
 return 0;
}
'''

# The weapon file's keys, through the real info parser.
loader_support = common + r'''
static void Com_Error(int code,const char *fmt,...){assert(0);}
static void *Hunk_AllocLowAlignInternal(int size,int align){return malloc(size);}
static int stricmp(const char *a,const char *b){return strcasecmp(a,b);}
static int valueindex;static char value1[2][8192];
'''
loader_body = (function(shared, 'I_strnicmp_core') + function(shared, 'Info_ValueForKey') +
               function(loader, 'SetConfigString_core') + function(loader, 'BG_ParsePlacedExplosiveKeys'))
loader_checks = r'''
int main(void){
 WeaponDef d;memset(&d,0,sizeof(d));
 BG_ParsePlacedExplosiveKeys(&d,"\\damage\\0\\hasDetonator\\1\\stickiness\\All\\detonateSound\\weap_c4_detonate\\bounceSound\\weap_c4_land");
 assert(d.hasDetonator==1&&d.stickiness==WEAPSTICKINESS_ALL&&d.proximityRadius==0&&!strcmp(d.szDetonateSound,"weap_c4_detonate"));
 assert(!strcmp(d.szBounceSound,"weap_c4_land")&&!strcmp(d.szArmSound,"")&&!strcmp(d.szTriggerSound,""));
 BG_ParsePlacedExplosiveKeys(&d,"\\proximityRadius\\40\\proximityArmTime\\1.2\\proximityDelay\\0.15\\stickiness\\ground\\armSound\\arm\\triggerSound\\minefield_click");
 assert(!d.hasDetonator&&d.stickiness==WEAPSTICKINESS_GROUND&&d.proximityRadius==40&&d.iProximityArmTime==1200&&d.iProximityDelay==150);
 assert(!strcmp(d.szArmSound,"arm")&&!strcmp(d.szTriggerSound,"minefield_click")&&!strcmp(d.szDetonateSound,""));
 /* The view model's own offset, forward, right and up. */
 BG_ParsePlacedExplosiveKeys(&d,"\\gunOfsF\\1\\gunOfsR\\3.5\\gunOfsU\\-8.5");
 assert(d.vGunOfs[0]==1&&d.vGunOfs[1]==3.5f&&d.vGunOfs[2]==-8.5f);
 /* A retail file has none of the keys. */
 BG_ParsePlacedExplosiveKeys(&d,"\\displayName\\WEAPON_M2FRAGGRENADE\\stickiness\\sometimes\\fuseTime\\3.5");
 assert(!d.hasDetonator&&!d.stickiness&&!d.proximityRadius&&!d.iProximityArmTime&&!d.iProximityDelay);
 assert(!d.vGunOfs[0]&&!d.vGunOfs[1]&&!d.vGunOfs[2]);
 puts("PASS: hasDetonator, stickiness by CoD4's names in any case, the mine's radius and its arm and click delays in seconds, and their sounds, and the view offset; none in a retail file");
 return 0;
}
'''

# The HUD: who sees which emblems, where, how large and how bright, the
# emblems' shapes, and G's detonator toggle.
hud_support = common + r"""
typedef struct Material *MaterialHandle;
static char emblemMaterial;
#define EMBLEMS ((MaterialHandle)&emblemMaterial)
typedef struct {struct {int enabled;} current;} dvar_t;
typedef struct {int weapon,pm_type,clientNum,cursorHint,stats[1],weapons[4];signed char weaponslots[3];} playerState_t;
typedef struct {int number,eType,eFlags,weapon,otherEntityNum;struct {int trType;} pos;} entityState_t;
typedef struct {entityState_t nextState;int nextValid;vec3_t lerpOrigin;} centity_t;
typedef struct {int numEntities;entityState_t entities[256];} snapshot_t;
typedef struct {int team;} clientInfo_t;
typedef struct {snapshot_t *snap,*nextSnap;playerState_t predictedPlayerState;int renderingThirdPerson,weaponSelect,time;
 struct {float fov_x,fov_y;int width,height;vec3_t vieworg,viewaxis[3];} refdef;struct {clientInfo_t clientinfo[64];} bgs;} cg_t;
typedef struct {int buttons;} usercmd_t;
#define R_HUD_IMAGE_SQUAD_EMBLEMS 15
static cg_t cgState,*cg=&cgState;static centity_t entities[1024],*cg_entities=entities;static snapshot_t snap;
static WeaponDef defs[4],*bg_weaponDefs[4]={0,&defs[1],&defs[2],&defs[3]};static int bg_iNumWeapons=3;
static dvar_t iconsDvar={{1}};static int hidden,firstOffhand,heldButtons,draws,selected,builds,builtWidth,builtHeight;
static float drawX,drawY,drawW,drawH,drawAlpha,drawS1,drawS2;static byte built[128*2*128*4];
static const dvar_t *Dvar_RegisterBool_mac(const char *n,int v,int f){return &iconsDvar;}
static WeaponDef *BG_GetWeaponDef(int w){return w>0&&w<4?&defs[w]:NULL;}
static int BG_GetNumWeapons(void){return 3;}
static int BG_GetFirstAvailableOffhand(const playerState_t *ps,int c){return firstOffhand;}
static float GetVirtualWidthFromRealWidth(float w){return w*640.0f/1280.0f;}
static float GetVirtualHeightFromRealHeight(float h){return h*480.0f/720.0f;}
static float GetRealHeightFromVirtualHeight(float h){return h*720.0f/480.0f;}
static MaterialHandle R_CreateHudImageMaterial(int slot,const byte *bgra,int w,int h){
 assert(slot==R_HUD_IMAGE_SQUAD_EMBLEMS&&w==2*h&&w*h*4<=(int)sizeof(built));memcpy(built,bgra,w*h*4);
 builtWidth=w;builtHeight=h;builds++;return EMBLEMS;}
static void CL_DrawStretchPic(float x,float y,float w,float h,int ha,int va,float s1,float t1,float s2,float t2,const vec_t *c,MaterialHandle m){
 assert(ha==7&&va==7&&m==EMBLEMS&&t1==0&&t2==1&&c[0]==1&&c[1]==1&&c[2]==1);draws++;
 drawX=x+w*.5f;drawY=y+h*.5f;drawW=w;drawH=h;drawAlpha=c[3];drawS1=s1;drawS2=s2;}
static int CL_GetCurrentCmdNumber(void){return 5;}
static qboolean CL_GetUserCmd(int n,usercmd_t *c){c->buttons=heldButtons;return 1;}
static Bool CG_AreHudMenusHidden(void){return hidden;}
static void CG_SelectWeaponIndex(int w){selected=w;cg->weaponSelect=w;}
"""
hud_body = (function(weapons, 'BG_PlayerDetonatorWeapon') + hud[hud.index('#define EXPLOSIVE_EMBLEM_RANGE'):]
            .replace('extern ', '// extern '))
hud_checks = r"""
enum {MINE=1,CHARGE=2,FRAG=3};
static void view(void){
 memset(cg,0,sizeof(*cg));memset(entities,0,sizeof(entities));memset(&snap,0,sizeof(snap));
 cg->snap=cg->nextSnap=&snap;cg->refdef.fov_x=90;cg->refdef.fov_y=73.74f;cg->refdef.width=1280;cg->refdef.height=720;
 cg->refdef.viewaxis[0][0]=cg->refdef.viewaxis[1][1]=cg->refdef.viewaxis[2][2]=1;cg->predictedPlayerState.clientNum=0;
 cg->bgs.clientinfo[0].team=2;cg->bgs.clientinfo[1].team=2;cg->bgs.clientinfo[2].team=1;draws=0;hidden=0;
 iconsDvar.current.enabled=1;
}
static void lay(int number,int weapon,int owner,float x,float y,float z){
 centity_t *c=&entities[number];c->nextValid=1;c->nextState=(entityState_t){number,4,0,weapon,owner,{0}};
 c->lerpOrigin[0]=x;c->lerpOrigin[1]=y;c->lerpOrigin[2]=z;snap.entities[snap.numEntities++]=c->nextState;}
/* The texel of an emblem cell at (u, v) radii from its middle, y down. */
static const byte *texel(int kind,float u,float v){
 int size=builtHeight,x=(int)((u/EXPLOSIVE_EMBLEM_CELL*.5f+.5f)*size),y=(int)((v/EXPLOSIVE_EMBLEM_CELL*.5f+.5f)*size);
 return built+(y*size*2+kind*size+x)*4;}
int main(void){
 CG_ExplosivesInit();
 defs[MINE].weapType=defs[CHARGE].weapType=defs[FRAG].weapType=WEAPTYPE_GRENADE;defs[MINE].proximityRadius=40;
 defs[CHARGE].hasDetonator=1;
 /* The player's own and a teammate's, never an enemy's; a 16-unit disc 17
  * above it, scaled with distance: 100 ahead it spans 16 * 240 / (100 tan 36.87). */
 for(int owner=0;owner<3;owner++){view();lay(100,MINE,owner,100,0,-17);CG_DrawExplosiveIcons();
  assert(draws==(owner<2));
  if(draws)assert(fabsf(drawX)<1e-3f&&fabsf(drawY)<1e-3f&&fabsf(drawW-51.2f)<.01f&&drawW==drawH&&
   drawAlpha==EXPLOSIVE_EMBLEM_FULL&&drawS1==0&&drawS2==.5f);}
 assert(builds==1&&builtWidth==2*builtHeight&&builtHeight==128);
 view();lay(100,CHARGE,1,100,-100,0);CG_DrawExplosiveIcons();assert(draws==1&&drawX>0&&drawY<0);
 /* The Axis see their cross, the second cell. */
 view();cg->bgs.clientinfo[0].team=1;lay(100,MINE,0,100,0,0);CG_DrawExplosiveIcons();assert(draws==1&&drawS1==.5f&&drawS2==1);
 /* Never smaller than the least size far off, nor larger than the greatest up close. */
 view();lay(100,MINE,0,1200,0,0);CG_DrawExplosiveIcons();assert(draws==1&&drawW==EXPLOSIVE_EMBLEM_MIN);
 view();lay(100,MINE,0,20,0,-17);CG_DrawExplosiveIcons();assert(draws==1&&drawW==EXPLOSIVE_EMBLEM_MAX);
 /* Pulsing slowly: full, faint half a period on, between at a quarter. */
 for(int t=0;t<3;t++){view();cg->time=1800*7+(t==0?0:t==1?900:450);lay(100,MINE,0,100,0,0);CG_DrawExplosiveIcons();
  float want=t==0?EXPLOSIVE_EMBLEM_FULL:t==1?EXPLOSIVE_EMBLEM_FAINT:.5f*(EXPLOSIVE_EMBLEM_FULL+EXPLOSIVE_EMBLEM_FAINT);
  assert(draws==1&&fabsf(drawAlpha-want)<1e-3f);}
 /* Free-for-all: only one's own. */
 view();cg->bgs.clientinfo[0].team=0;cg->bgs.clientinfo[1].team=0;lay(100,MINE,1,100,0,0);lay(101,MINE,0,100,0,0);
 CG_DrawExplosiveIcons();assert(draws==1);
 /* Behind, beyond 1600 units, exploded, a frag, an owner out of range, hidden menus, off, third person or still moving: nothing. */
 for(int c=0;c<9;c++){view();lay(100,c==3?FRAG:MINE,c==4?64:0,c==0?-100:c==1?1601:100,0,0);
  if(c==2)entities[100].nextState.eFlags=0x20;if(c==5)hidden=1;if(c==6)iconsDvar.current.enabled=0;
  if(c==7)cg->renderingThirdPerson=1;if(c==8)entities[100].nextState.pos.trType=5;CG_DrawExplosiveIcons();assert(!draws);}
 /* Fading over the last quarter of the range. */
 view();lay(100,MINE,0,1500,0,0);CG_DrawExplosiveIcons();
 assert(draws==1&&fabsf(drawAlpha-.25f*EXPLOSIVE_EMBLEM_FULL)<1e-3f);
 /* The emblems: white figures and ring, the dark gap where the star's points
  * meet the ring, clear between them, and nothing outside. */
 const byte *starMiddle=texel(0,0,0),*ring=texel(0,0,.92f),*between=texel(0,0,.6f),*gap=texel(0,-.054f,-.895f),
  *outside=texel(0,1.08f,0),*crossArm=texel(1,0,-.7f),*crossRing=texel(1,.65f,-.65f),*crossBetween=texel(1,.42f,-.42f);
 assert(starMiddle[0]==255&&starMiddle[3]>200&&ring[0]==255&&ring[3]>200&&between[3]==0&&outside[3]==0);
 assert(gap[0]<80&&gap[3]>60&&gap[3]<160);
 assert(crossArm[0]==255&&crossArm[3]>200&&crossRing[0]==255&&crossRing[3]>200&&crossBetween[3]==0);
 /* G takes out the detonator when C4 stands in for frags, and puts it away. */
 playerState_t *ps=&cg->predictedPlayerState;
 view();ps->weapons[0]=1<<CHARGE|1<<FRAG|(1<<5);ps->weaponslots[1]=5;cg->weaponSelect=5;firstOffhand=0;ps->stats[0]=100;
 for(int i=0;i<3;i++){heldButtons=0x10000;CG_UpdateDetonatorSelect();}assert(cg->weaponSelect==CHARGE);
 heldButtons=0;CG_UpdateDetonatorSelect();heldButtons=0x10000;CG_UpdateDetonatorSelect();assert(cg->weaponSelect==5);
 /* Not with frags to throw, near a live frag, or dead. */
 for(int c=0;c<3;c++){heldButtons=0;CG_UpdateDetonatorSelect();selected=0;firstOffhand=c==0;
  ps->cursorHint=c==1?GRENADE_THROWBACK_HINT:0;ps->stats[0]=c==2?0:100;heldButtons=0x10000;CG_UpdateDetonatorSelect();
  assert(!selected&&cg->weaponSelect==5);}
 puts("PASS: the player's own and teammates' mines and charges at rest carry the squad emblem, the Allies' star or the Axis' cross, a 16-unit disc 17 above them between 22 and 96 virtual units, pulsing every 1.8 s from 0.3 to 0.95, through walls, fading over the last 400 of 1600 units; never enemies' or others' in free-for-all, nor behind, moving, exploded, frags or with the HUD hidden; the emblems are white with dark gaps and edges; G takes out the detonator and puts it away only with C4 for frags, alive and away from a live frag");
 return 0;
}
"""

# The snapshot: the real visibility tests ask the placed explosive rule only
# for what is out of sight, after a hidden entity's and a client's mask.
snapshot_support = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char byte;typedef float vec_t;typedef float vec3_t[3];
typedef struct {int trType;vec3_t trBase;} trajectory_t;
typedef struct {int number,eType,weapon,otherEntityNum;trajectory_t pos;} entityState_t;
typedef struct {int linked,broadcastTime,svFlags,clientMask[2];vec3_t absmin,absmax;} entityShared_t;
typedef struct {entityState_t s;entityShared_t r;} gentity_t;
typedef struct {int svFlags,clientMask[2];vec3_t absmin,absmax;} archivedEntityShared_t;
typedef struct {entityState_t s;archivedEntityShared_t r;} archivedEntity_t;
typedef struct {int numClusters,lastCluster,clusternums[16];} svEntity_t;
typedef struct {int time;} serverStatic_t;
static serverStatic_t svsState;
#define imp_svs (&svsState)
/* Everything stands in cluster 9. */
static svEntity_t svEntity={1,0,{9}};
static byte *SV_SvEntityForGentity(byte *e){return (byte *)&svEntity;}
static int BoxDistSqrdExceeds(byte *a,byte *b,byte *o,float d){return 0;}
static int CM_BoxLeafnums(byte *a,byte *b,int *l,int n,int *last){l[0]=9;*last=0;return 1;}
static int CM_LeafCluster(int l){return l;}
static int answer;
static int G_PlacedExplosiveSentTo(const entityState_t *es,const vec_t *eye,int clientNum){
 assert(es->number==200&&clientNum==5&&eye[2]==60);return answer;}
'''
snapshot_body = ''.join(function(snapshot, name) for name in (
    'SV_BitVectorTestLocal', 'SV_EntityIsVisibleToClientLocal', 'SV_CachedEntityIsVisibleLocal'))
snapshot_checks = r'''
int main(void){
 byte none[4]={0},seen[4]={0,1<<1};vec3_t eye={0,0,60};gentity_t e;archivedEntity_t a;
 /* Live: out of sight, sent as the rule says; in sight, sent anyway; never hidden, masked or unlinked. */
 struct {int rule,sight,hidden,masked,unlinked,sent;} live[]={
  {1,0,0,0,0,1},{0,0,0,0,0,0},{0,1,0,0,0,1},{1,0,1,0,0,0},{1,0,0,1,0,0},{1,0,0,0,1,0}};
 for(unsigned i=0;i<sizeof(live)/sizeof(live[0]);i++){
  memset(&e,0,sizeof(e));e.s.number=200;e.r.linked=!live[i].unlinked;e.r.svFlags=live[i].hidden;
  e.r.clientMask[0]=live[i].masked<<5;answer=live[i].rule;
  assert(SV_EntityIsVisibleToClientLocal((byte *)&e,eye,live[i].sight?seen:none,0,5)==live[i].sent);}
 /* Archived, for a killcam: the same, masked never. */
 struct {int rule,sight,masked,sent;} archived[]={{1,0,0,1},{0,0,0,0},{0,1,0,1},{1,0,1,0}};
 for(unsigned i=0;i<sizeof(archived)/sizeof(archived[0]);i++){
  memset(&a,0,sizeof(a));a.s.number=200;a.r.clientMask[0]=archived[i].masked<<5;answer=archived[i].rule;
  assert(SV_CachedEntityIsVisibleLocal(&a,eye,archived[i].sight?seen:none,0,5)==archived[i].sent);}
 puts("PASS: live and killcam snapshots send what is out of sight when the placed explosive rule says so, after hidden, masked and unlinked entities, and what is in sight as before");
 return 0;
}
'''

programs = {
    'server': (defines(missiles, 'GMISSILE_') + server_support + header + server_body + server_checks),
    'snapshot': (snapshot_support + snapshot_body + snapshot_checks),
    'pmove': (pmove_support + header + pmove_constants + pmove_body + pmove_checks),
    'loader': (loader_support + header + loader_body + loader_checks),
    'hud': (hud_support + header + hud_body + hud_checks),
}
mutants = [
    ('server', 'OnSameTeam(owner, other))', '0)'),
    ('server', 'if (!other->r.inuse || !other->client || other == owner ||', 'if (!other->r.inuse || !other->client ||'),
    ('server', '(weapDef->iProximityArmTime > 0 ? weapDef->iProximityArmTime : 1)', '1'),
    ('server', 'delta[2] < -GMISSILE_PROXIMITY_BELOW || delta[2] > GMISSILE_PROXIMITY_ABOVE', '0'),
    ('server', '    if (ent->spawnflags & GMISSILE_PLACED_STUCK) {\n        G_RunThink(ent);\n        return 1;\n    }\n', ''),
    ('server', '        G_PlacedTeam(owner) != ent->team ||', '        0 ||'),
    ('server', ' || owner->client->sess.sessionState == SESS_STATE_SPECTATOR ||', ' ||'),
    ('server', '        owner->client->lastSpawnTime != s_placedSpawnTime[ent->s.number & 1023])', '        0)'),
    ('server', '    s_placedSpawnTime[ent->s.number & 1023] = owner->client ? owner->client->lastSpawnTime : 0;\n', ''),
    ('server', '        if (count < limit || !oldest)', '        if (count <= limit || !oldest)'),
    ('server', ' || COD2_GEntityFromHandle(ent->parent) != player)', ')'),
    ('server', 'weapDef->stickiness == WEAPSTICKINESS_ALL && !(_ENT(other)->client)', 'weapDef->stickiness == WEAPSTICKINESS_ALL'),
    ('server', '    VectorScale(trace->normal, -1.0f, left);', '    VectorScale(trace->normal, 1.0f, left);'),
    ('server', '    ent->nextthink = 0;\n    ent->s.otherEntityNum = owner->s.number;', '    ent->s.otherEntityNum = owner->s.number;'),
    ('server', 'BG_IsPlacedExplosive(BG_GetWeaponDef(missile->s.weapon)))', '0)'),
    ('pmove', ' ||\n        (pm->oldcmd.buttons & PM_WEAPON_BUTTON_ADS))', ')'),
    ('pmove', 'if (offhandClass == OFFHAND_CLASS_FRAG_GRENADE && BG_PlayerDetonatorWeapon(ps))', 'if (0)'),
    ('pmove', '    if (weapDef->hasDetonator)\n        return owned;\n', ''),
    ('loader', '(int)(atof(Info_ValueForKey(info, "proximityArmTime")) * 1000.0 + 0.5)',
     '(int)atof(Info_ValueForKey(info, "proximityArmTime"))'),
    ('loader', '        if (!stricmp(value, stickinessNames[i]))', '        if (!strcmp(value, stickinessNames[i]))'),
    ('hud', ' || cg->bgs.clientinfo[owner].team != team))', '))'),
    ('hud', '    if (!detonator || BG_GetFirstAvailableOffhand(ps, OFFHAND_CLASS_FRAG_GRENADE))', '    if (!detonator)'),
    ('hud', '        color[3] = (color[3] > 1.0f ? 1.0f : color[3]) * pulse;', '        color[3] = color[3] > 1.0f ? 1.0f : color[3];'),
    ('hud', 'size = size < EXPLOSIVE_EMBLEM_MIN ? EXPLOSIVE_EMBLEM_MIN : size', 'size = size'),
    ('hud', ' || cent->nextState.pos.trType != 0)', ')'),
    ('hud', '(float)(team == 1 ? EXPLOSIVE_EMBLEM_AXIS : EXPLOSIVE_EMBLEM_ALLIES)', '(float)EXPLOSIVE_EMBLEM_ALLIES'),
    ('hud', '(ring <= 0.0f && figure > EXPLOSIVE_EMBLEM_GAP)', '(ring <= 0.0f)'),
    ('server', '    mine = ps->cursorHint ? NULL : G_FindOwnMine(player);', '    mine = G_FindOwnMine(player);'),
    ('server', '            COD2_GEntityFromHandle(mine->parent) != player || mine->s.pos.trType != 0 ||', '            mine->s.pos.trType != 0 ||'),
    ('server', '    } else if (LEVEL_TIME - s_minePickupStart[clientNum] >= GMISSILE_PICKUP_MSEC) {', '    } else {'),
    ('server', ' &&\n            ps->ammoclip[weapDef->iClipIndex] < weapDef->iClipSize)', ')'),
    ('server', 'if (((ps->weapons[mine->s.weapon >> 5] >> (mine->s.weapon & 0x1f)) & 1) &&', 'if (1 &&'),
    ('server', '    if (!trace.startsolid && trace.fraction < 1.0f && trace.normal[2] > 0.7f) {', '    if (!trace.startsolid && trace.fraction < 1.0f) {'),
    ('server', '        spot[2] += GMISSILE_MINE_REST;', ''),
    ('server', '    angles[1] = yaw;', '    angles[1] = 0.0f;'),
    ('server', '    G_SetAngle(ent, angles);\n    /* A mine at rest never moves, so the missile code would never link it\n     * and no client would be sent it. */\n    SV_LinkEntity(ent);\n}',
     '    G_SetAngle(ent, angles);\n}'),
    ('server', '        if ((team != 1 && team != 2) || G_PlacedTeam(&g_entities[es->otherEntityNum]) != team)',
     '        if (team != 1 && team != 2)'),
    ('server', '(team != 1 && team != 2) || G_PlacedTeam(', 'G_PlacedTeam('),
    ('server', '    return DotProduct(delta, delta) <= PLACED_EMBLEM_RANGE * PLACED_EMBLEM_RANGE;', '    return 1;'),
    ('server', 'if (es->eType != 4 || es->pos.trType != 0 ||', 'if (es->eType != 4 ||'),
    ('server', '!BG_IsPlacedExplosive(BG_GetWeaponDef(es->weapon)))', '0)'),
    ('snapshot', '    if (G_PlacedExplosiveSentTo(es, org, clientNum)) {\n        return 1;\n    }\n\n    svEnt', '\n    svEnt'),
    ('snapshot', '    if (G_PlacedExplosiveSentTo(es, org, clientNum)) {\n        return 1;\n    }\n\n    numLeafs', '\n    numLeafs'),
    ('snapshot', '    if (shared->svFlags & 1) {\n        return 0;\n    }\n'
     '    if ((shared->clientMask[clientNum >> 5] >> (clientNum & 31)) & 1) {\n        return 0;\n    }\n'
     '    if (shared->svFlags & 0x18) {\n        return 1;\n    }\n'
     '    if (G_PlacedExplosiveSentTo(es, org, clientNum)) {\n        return 1;\n    }\n',
     '    if (G_PlacedExplosiveSentTo(es, org, clientNum)) {\n        return 1;\n    }\n'
     '    if (shared->svFlags & 1) {\n        return 0;\n    }\n'
     '    if ((shared->clientMask[clientNum >> 5] >> (clientNum & 31)) & 1) {\n        return 0;\n    }\n'
     '    if (shared->svFlags & 0x18) {\n        return 1;\n    }\n'),
    # Springing up after the click.
    ('server', '            G_LaunchMine(ent);', '            ent->nextthink = LEVEL_TIME;'),
    ('server', '        if (!ent->nextthink && LEVEL_TIME >= ent->count)', '        if (LEVEL_TIME >= ent->count)'),
    ('server', '        ent->count = LEVEL_TIME + weapDef->iProximityDelay;', '        ent->count = LEVEL_TIME;'),
    ('server', '    ent->s.pos.trDelta[2] = speed;', '    ent->s.pos.trDelta[2] = speed * 1.5f;'),
    ('server', '    ent->nextthink = LEVEL_TIME + (int)(1000.0f * speed / GMISSILE_GRAVITY + 0.5f);',
     '    ent->nextthink = LEVEL_TIME + 500;'),
    ('server', '    ent->clipmask &= ~GMISSILE_CONTENTS_BODY;\n', ''),
    ('server', '    ent->s.apos.trDelta[1] = GMISSILE_MINE_SPIN;\n', ''),
    ('server', '    ent->s.groundEntityNum = 1023;\n    ent->clipmask', '    ent->clipmask'),
    # Shooting a mine.
    ('server', 'owner == attacker || OnSameTeam(owner, attacker)', 'owner == attacker'),
    ('server', '!owner || owner == attacker ||', '!owner ||'),
    ('server', '> GMISSILE_MINE_HIT_RADIUS * GMISSILE_MINE_HIT_RADIUS ||', '> 1e9f ||'),
    ('server', '        along = along < 0.0f ? 0.0f : along > length ? length : along;\n', ''),
    ('server', ' || mine->s.pos.trType != 0 ||\n            (mine->spawnflags & GMISSILE_PLACED_TRIGGERED))\n            continue;\n        owner',
     ' ||\n            (mine->spawnflags & GMISSILE_PLACED_TRIGGERED))\n            continue;\n        owner'),
    ('server', 'mine->s.pos.trType != 0 ||\n            (mine->spawnflags & GMISSILE_PLACED_TRIGGERED))\n            continue;\n        owner',
     'mine->s.pos.trType != 0)\n            continue;\n        owner'),
    ('server', '            (hit && along >= nearest))', '            0)'),
    ('server', '    if (hit && !s_mineShotBy[hit->s.number & 1023])', '    if (hit)'),
    ('server', '            ent->parent = COD2_GEntityHandle(shooter);\n', ''),
    ('server', 'if (shooter->r.inuse && shooter->client && shooter->client->sess.connected == 2)', 'if (1)'),
    ('server', '    s_mineShotBy[ent->s.number & 1023] = 0;\n    ent->count = 0;', '    ent->count = 0;'),
    ('server', '        G_ExplodeMissile(ent);\n        return 1;\n    }\n    if (ent->spawnflags & GMISSILE_PLACED_STUCK) {',
     '        return 1;\n    }\n    if (ent->spawnflags & GMISSILE_PLACED_STUCK) {'),
    ('server', '    center[2] += GMISSILE_MINE_HIT_RISE;', ''),
]
header_mutants = [
    ('pmove', 'return def->weapType == WEAPTYPE_GRENADE && !BG_IsPlacedExplosive(def) &&',
     'return def->weapType == WEAPTYPE_GRENADE &&'),
]


def run(directory, name, text, expect):
    source = Path(directory) / f'{name}.c'
    binary = Path(directory) / name
    source.write_text(text)
    subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    *(['-Wall', '-Werror', '-Wno-unused-function', '-Wno-unused-variable', '-Wno-unused-but-set-variable'] if expect else ['-w']),
                    str(source), '-lm', '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert (result.returncode == 0) == expect, (name, result.stdout[-400:], result.stderr[-1500:])
    return result.stdout.strip()


with tempfile.TemporaryDirectory(prefix='cod2-placed-explosives-') as directory:
    for name, text in programs.items():
        print(run(directory, name, text, True))
    failed = 0
    for index, (program, before, after) in enumerate(mutants):
        text = programs[program]
        assert text.count(before) == 1, (program, before)
        run(directory, f'mutant{index}', text.replace(before, after), False)
        failed += 1
    for index, (program, before, after) in enumerate(header_mutants):
        text = programs[program]
        assert text.count(before) == 1, before
        run(directory, f'header{index}', text.replace(before, after), False)
        failed += 1
    print(f'PASS: {failed} placed explosive mutants fail')
