#!/usr/bin/env python3
"""Replay live grenade impact, pickup, release and fuse deadlines in production C."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
missiles = (root / 'src/PC/game_mp/g_missile_mp.c').read_text()
weapons = (root / 'src/PC/bgame/bg_weapons.c').read_text()
combat = (root / 'src/PC/game_mp/g_combat_mp.c').read_text()


def function(source, name):
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
typedef int qboolean;typedef unsigned char byte;typedef float vec_t;typedef float vec3_t[3];
#define WEAPTYPE_GRENADE 2
#define OFFHAND_CLASS_FRAG_GRENADE 1
typedef struct {int trType,trTime,trDuration;vec3_t trBase,trDelta;} trajectory_t;
typedef struct {float fraction;int startsolid,entityNum,surfaceFlags,partName;vec3_t normal;}trace_t;
typedef struct {int pm_flags,pm_type,weaponstate,weapon,weaponTime,weaponDelay,offHandIndex,grenadeTimeLeft,
 eFlags,weapAnim,cursorHint,cursorHintString,cursorHintEntIndex,commandTime,weaponRestrictKickTime,stats[6],ammoclip[128],eventSequence,events[4],eventParms[4];vec3_t origin;float viewHeightCurrent;}playerState_t;
typedef struct {int serverTime,buttons,weapon;}usercmd_t;
typedef struct {playerState_t *ps;usercmd_t cmd;}pmove_t;typedef struct {int msec;}pml_t;
typedef struct {playerState_t ps;struct {int connected;usercmd_t oldcmd;}sess;}gclient_t;
typedef struct gentity_s {
 struct {int number,eType,eFlags,weapon,otherEntityNum,groundEntityNum,surfType,time,time2,eventParm,scale;vec3_t origin2;trajectory_t pos,apos;}s;
 struct {int ownerNum,inuse,svFlags,contents;vec3_t currentOrigin,currentAngles;}r;
 gclient_t *client;int useCount,handler,flags,health,parent,nextthink,classname,damage,clipmask,freeAfterEvent;struct {float time;}grenade;
}gentity_t;
typedef gentity_t gentity_s;
typedef struct {int weapType,offhandClass,bCookOffHold,bSemiAuto,iFuseTime,damage,iExplosionRadius,iExplosionOuterDamage,iExplosionInnerDamage,
 iProjectileSpeed,weapClass,projExplosion,iClipIndex,iFireTime,iFireDelay,quickRaiseTime,iHoldFireTime;
 float parallelBounce[32],perpendicularBounce[32],fAdsAimPitch,destabilizeDistance,destabilizationTimeReductionRatio,destabilizationBaseTime;const char *szProjExplosionEffect;}WeaponDef;
static gentity_t g_entities[1024];static gclient_t clients[3];
static struct {int time,previousTime,num_entities;}level;
static struct {int grenade;}scr_const;
static struct {int methodOfDeath,splashMethodOfDeath;}entityHandlers[20];
static vec3_t zero;static byte *vec3_origin_ptr=(byte*)zero,*pPriorityMap;
static WeaponDef defs[4],*bg_weaponDefs[128];
static int spawns,explosions,damages,radiusCalls,impactEvents,blocked,collisionEntity=1023;
#define _ENT(e) ((gentity_t *)(e))
#define G_ENTITY(n) (&g_entities[n])
#define LEVEL_TIME level.time
#define LEVEL_PREVIOUSTIME level.previousTime
#define HANDLER_MOD(n) entityHandlers[n].methodOfDeath
#define HANDLER_SPLASHMOD(n) entityHandlers[n].splashMethodOfDeath
enum {GMISSILE_ENTITYNUM_WORLD=1022,GMISSILE_EF_GRENADE_BOUNCE=0x1000000,GMISSILE_FL_GUIDED=0x10000,GMISSILE_FL_TURRET=0x20000,GMISSILE_FL_HELD=0x40000};
typedef struct {gentity_t *missile;int missileUseCount,playerSpawnCount;}heldGrenade_t;
static heldGrenade_t heldGrenades[64];
static WeaponDef *BG_GetWeaponDef(int w){assert(w>0&&w<4);return &defs[w];}
static int COD2_GEntityHandle(const gentity_t *e){return e ? (int)(e-g_entities)+1 : 0;}
static gentity_t *COD2_GEntityFromHandle(int n){return n ? &g_entities[n-1] : NULL;}
static gentity_t *G_Spawn(void){int n=level.num_entities++;assert(n<1022);gentity_t *e=&g_entities[n];memset(e,0,sizeof(*e));e->s.number=n;e->r.inuse=1;spawns++;return e;}
static void G_GetPlayerViewOrigin(gentity_t *e,vec_t *v){memcpy(v,e->client->ps.origin,sizeof(vec3_t));v[2]+=e->client->ps.viewHeightCurrent;}
static int G_SetOrigin(gentity_t *e,const vec_t *v){memmove(e->r.currentOrigin,v,sizeof(vec3_t));memmove(e->s.pos.trBase,v,sizeof(vec3_t));e->s.pos.trType=0;return 1;}
static void G_SetAngle(gentity_t *e,const vec_t *v){memcpy(e->r.currentAngles,v,sizeof(vec3_t));}
static void SV_LinkEntity(gentity_t *e){}
static void Scr_SetString(int *s,int v){*s=v;}
static void vectoangles(vec_t *v,vec_t *a){memset(a,0,sizeof(vec3_t));}
static float AngleNormalize360(float a){return a;}
static float flrand(float a,float b){return (a+b)*.5f;}
static void BG_EvaluateTrajectory(trajectory_t *tr,int t,vec_t *v){
 float d=(t-tr->trTime)*.001f;for(int i=0;i<3;i++)v[i]=tr->trBase[i]+(tr->trType?tr->trDelta[i]*d:0);
}
static void BG_EvaluateTrajectoryDelta(trajectory_t *tr,int t,vec_t *v){memcpy(v,tr->trDelta,sizeof(vec3_t));}
static int SV_PointContents(vec_t *p,int n,int c){return 0;}
static void G_TraceCapsule(trace_t *t,vec_t *a,vec_t *b,vec_t *c,vec_t *d,int skip,int mask){memset(t,0,sizeof(*t));t->fraction=blocked?.5f:1;t->entityNum=blocked?1022:1023;}
static void G_LocationalTrace(trace_t *t,vec_t *a,vec_t *b,int skip,int mask,byte *p){memset(t,0,sizeof(*t));t->entityNum=collisionEntity;t->fraction=collisionEntity==1023?1:.5f;t->normal[0]=-1;}
static int DirToByte(vec_t *v){return 0;}
static void G_AddEvent(gentity_t *e,int a,int b){if(a==0xbd||a==0xbe)impactEvents++;if(a==0xbc){assert(e->s.eType==0);explosions++;}}
static int G_RadiusDamage(vec_t *o,gentity_t *e,gentity_t *a,float inner,float outer,float radius,gentity_t *i,int m){
 assert(inner==defs[e->s.weapon].iExplosionInnerDamage&&outer==defs[e->s.weapon].iExplosionOuterDamage&&radius==defs[e->s.weapon].iExplosionRadius);radiusCalls++;return 0;
}
typedef void EffectTemplate;
static void Server_SwitchToValidFxScheduler(void){}
static EffectTemplate *FX_RegisterEffect(const char *name){return NULL;}
static float FX_GetEffectLength(EffectTemplate *effect){return 0;}
static float crandom(void){return .5f;}
static float randomf(void){return .5f;}
static int LogAccuracyHit(gentity_t *e,gentity_t *a){return e->client!=NULL;}
static void G_Damage(gentity_t *e,gentity_t *i,gentity_t *a,vec_t *d,vec_t *p,int n,int f,int m,int c,int l){damages++;}
static void G_CheckHitTriggerDamage(gentity_t *a,vec_t *b,vec_t *c,int d,int m){}
static void G_GrenadeTouchTriggerDamage(gentity_t *e,vec_t *a,vec_t *b,int r,int m){}
static gentity_t *G_TempEntity(vec_t *o,int n){assert(0);return NULL;}
static void G_FreeEntity(gentity_t *e){e->r.inuse=0;}
static void SnapVectorTowards(vec_t *a,vec_t *b){}
static float Vec3Normalize(vec_t *v){float n=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(n)for(int i=0;i<3;i++)v[i]/=n;return n;}
static float Vec3NormalizeTo(vec_t *a,vec_t *b){memcpy(b,a,sizeof(vec3_t));return Vec3Normalize(b);}
static void G_MissileLandAngles(gentity_t *e,trace_t *t,vec_t *a,qboolean b){memset(a,0,sizeof(vec3_t));}
void G_ExplodeMissile(gentity_t *e);
static int G_RunThink(gentity_t *e){if(e->nextthink<=level.time){e->nextthink=0;G_ExplodeMissile(e);}return 0;}
static void BG_AddPredictableEventToPlayerstate(int e,int p,playerState_t *ps){int i=ps->eventSequence++&3;ps->events[i]=e;ps->eventParms[i]=p;}
static void PM_AddEvent(playerState_t *ps,int e){BG_AddPredictableEventToPlayerstate(e,0,ps);}
static void PM_WeaponSetAnim(playerState_t *ps,int a){ps->weapAnim=a;}
static void PM_SetProneMovementOverride(playerState_t *ps){}
static void PM_ResetWeaponState(playerState_t *ps){ps->weaponstate=0;ps->pm_flags&=~0x810;}
static void BG_AnimScriptEvent(playerState_t *ps,int a,int b,int c){}
static gentity_t *fire_grenade(gentity_t *,vec_t *,vec_t *,int,int);
'''

defines = '\n'.join(s for s in weapons.splitlines() if s.startswith('#define WEAPON_') or s.startswith('#define PM_WEAPON_'))
header = (root / 'src/headers/cod2_grenade.h').read_text()
body = ''.join(function(missiles, n) for n in (
    'VectorCopy', 'VectorSubtract', 'VectorAdd', 'VectorScale', 'VectorClear',
    'VectorLength', 'DotProduct', 'VectorMA', 'SnapFloat', 'SnapVector', 'LerpPosition',
    'G_ResetHeldGrenades', 'G_GetHeldGrenade', 'G_HoldGrenade', 'G_CanPickUpGrenade',
    'G_FindNearbyGrenade', 'G_UpdateGrenadeHint', 'G_GrenadeCookOff', 'G_BeginGrenadeInput',
    'G_EndGrenadeInput', 'G_ExplodeMissile', 'fire_grenade', 'G_BounceMissile', 'G_RunMissile'))
# Production functions are external; give the extracted grenade factory the same linkage.
support = support.replace('static gentity_t *fire_grenade(', 'gentity_t *fire_grenade(')
body += ''.join(function(weapons, n) for n in (
    'PM_UpdateOffhandCook', 'PM_UpdateWeaponTimers', 'PM_StartOffhandPrepare',
    'PM_ReleaseOffhand', 'PM_RunOffhandState'))
# Replay the actual armed-grenade drop branch from player_die, including its
# weapon/fuse argument order; death must reuse the live missile too.
death = function(combat, 'player_die')
start = death.index('    if (cl->ps.grenadeTimeLeft != 0) {')
end, depth = death.index('{', start) + 1, 1
while depth:
    depth += (death[end] == '{') - (death[end] == '}')
    end += 1
body += ('static void DropDeathGrenade(gentity_t *self){\n'
         'gclient_t *cl=self->client;vec3_t launchvel,launchspot;\n' + death[start:end] + '\n}\n')

checks = r'''
static gentity_t *player(int n){
 gentity_t *p=&g_entities[n];p->s.number=n;p->r.inuse=1;p->health=100;p->client=&clients[n];
 p->client->sess.connected=2;p->client->ps.viewHeightCurrent=60;p->client->ps.stats[5]=1;
 p->client->ps.weapon=3;return p;
}
static void reset(void){
 memset(g_entities,0,sizeof(g_entities));memset(clients,0,sizeof(clients));G_ResetHeldGrenades();
 memset(defs,0,sizeof(defs));level.time=1000;level.previousTime=950;level.num_entities=72;
 for(int i=1;i<4;i++){bg_weaponDefs[i]=&defs[i];defs[i].iClipIndex=i;defs[i].iFuseTime=3500;defs[i].iFireTime=300;defs[i].iFireDelay=100;defs[i].iHoldFireTime=600;for(int s=0;s<32;s++)defs[i].parallelBounce[s]=defs[i].perpendicularBounce[s]=.5f;}
 defs[1].weapType=defs[2].weapType=2;defs[1].offhandClass=1;defs[2].offhandClass=2;defs[2].iFuseTime=1000;
 defs[1].iExplosionRadius=100;defs[1].iExplosionInnerDamage=200;defs[1].iExplosionOuterDamage=50;
 entityHandlers[7].methodOfDeath=3;entityHandlers[7].splashMethodOfDeath=4;
 spawns=explosions=damages=radiusCalls=impactEvents=blocked=0;collisionEntity=1023;
}
static gentity_t *loose(gentity_t *p,int fuse){vec3_t start={24,0,2},v={500,0,0};return fire_grenade(p,start,v,1,fuse);}
static void command(gentity_t *p,int buttons,int msec){
 usercmd_t cmd={.buttons=buttons,.serverTime=p->client->ps.commandTime+msec};G_BeginGrenadeInput(p,&cmd);
 pml_t pml={.msec=msec};pmove_t pm={.ps=&p->client->ps,.cmd=cmd};
 if(PM_UpdateOffhandCook(&pm,&pml))G_GrenadeCookOff(p);
 else {int delayed=PM_UpdateWeaponTimers(&pm,&pml);if(delayed||(!pm.ps->weaponTime&&!pm.ps->weaponDelay))PM_RunOffhandState(&pm,delayed);}
 G_EndGrenadeInput(p);p->client->ps.commandTime=cmd.serverTime;p->client->sess.oldcmd=cmd;
}
int main(void){
 for(int target=1;target<=2;target++)for(int damage=0;damage<=100;damage+=100){
  reset();gentity_t *p=player(0);gentity_t *hit=target==1?player(1):&g_entities[1022];hit->takedamage=1;hit->s.number=target==1?1:1022;
  defs[1].damage=damage;gentity_t *g=loose(p,3500);int deadline=g->nextthink;collisionEntity=hit->s.number;
  level.previousTime=level.time;level.time+=50;
  G_RunMissile(g);assert(g->s.eType==4&&!g->freeAfterEvent&&!explosions&&!radiusCalls&&!damages&&!impactEvents);
  assert(g->s.pos.trDelta[0]<0&&g->nextthink==deadline);
 }
 reset();gentity_t *p=player(0),*receiver=player(1),*other=player(2);gentity_t *g=loose(p,2300);int deadline=g->nextthink;
 G_UpdateGrenadeHint(receiver);assert(receiver->client->ps.cursorHint==GRENADE_THROWBACK_HINT);
 for(int denied=0;denied<6;denied++){
  blocked=denied==0;receiver->health=denied==1?0:100;receiver->client->ps.pm_type=denied==2?4:0;
  receiver->client->ps.weaponstate=denied==3?5:0;receiver->client->ps.pm_flags=denied==4?0x8000:0;
  g->nextthink=denied==5?level.time:deadline;usercmd_t cmd={.buttons=0x10000};G_BeginGrenadeInput(receiver,&cmd);
  assert(!(g->flags&GMISSILE_FL_HELD));
 }
 blocked=0;receiver->health=100;receiver->client->ps.pm_type=receiver->client->ps.weaponstate=receiver->client->ps.pm_flags=0;g->nextthink=deadline;
 command(receiver,0x10000,16);assert(g->flags&GMISSILE_FL_HELD);assert(receiver->client->ps.pm_flags&PMF_GRENADE_THROWBACK);
 assert(receiver->client->ps.ammoclip[1]==0&&spawns==1&&g->nextthink==deadline);
 usercmd_t steal={.buttons=0x10000};G_BeginGrenadeInput(other,&steal);assert(!G_GetHeldGrenade(other));
 for(int i=0;i<25;i++){level.time+=10;command(receiver,0x10000,16);assert(g->nextthink==deadline&&receiver->client->ps.grenadeTimeLeft==deadline-level.time);}
 command(receiver,1,16);assert(receiver->client->ps.weaponstate==WEAPON_OFFHAND_FIRE); // G release wins over attack.
 level.time+=100;command(receiver,0,100);assert(!(receiver->client->ps.pm_flags&PMF_GRENADE_THROWBACK));
 vec3_t start={0,0,60},vel={700,0,200};gentity_t *same=fire_grenade(receiver,start,vel,1,3500);
 assert(same==g&&g->nextthink==deadline&&spawns==1&&!receiver->client->ps.ammoclip[1]);
 assert(!(g->flags&GMISSILE_FL_HELD)&&!(g->r.svFlags&1)&&g->r.ownerNum==1);
 level.time=deadline-1;G_RunMissile(g);assert(!explosions);level.time=deadline;G_RunMissile(g);assert(explosions==1);
 // Fresh inventory grenades burn after pin removal and expire even without new commands.
 reset();p=player(0);playerState_t *ps=&p->client->ps;ps->offHandIndex=1;ps->ammoclip[1]=2;PM_StartOffhandPrepare(ps);
 for(int i=0;i<6;i++){level.time+=100;command(p,0x10000,100);}
 g=G_GetHeldGrenade(p);assert(g&&ps->grenadeTimeLeft==3500&&spawns==1);deadline=g->nextthink;
 level.time=deadline;G_RunMissile(g);assert(explosions==1&&ps->grenadeTimeLeft==0&&!(ps->pm_flags&0x10)&&ps->ammoclip[1]==1);
 // Picked-up grenade can explode in hand with zero inventory grenades.
 reset();p=player(0);receiver=player(1);g=loose(p,120);command(receiver,0x10000,16);deadline=g->nextthink;
 level.time=deadline;command(receiver,0x10000,16);assert(explosions==1&&spawns==1&&!receiver->client->ps.ammoclip[1]);
 // Actual player_die branch drops the armed weapon, with the remaining fuse.
 for(int returned=0;returned<2;returned++){
  reset();p=player(0);receiver=player(1);
  if(returned){g=loose(p,800);command(receiver,0x10000,16);p=receiver;}
  else{p->client->ps.offHandIndex=1;p->client->ps.grenadeTimeLeft=800;p->client->ps.pm_flags=0x10;p->client->ps.weaponstate=14;G_EndGrenadeInput(p);g=G_GetHeldGrenade(p);}
  deadline=g->nextthink;DropDeathGrenade(p);
  assert(g->s.weapon==1&&g->nextthink==deadline&&spawns==1&&!(g->flags&GMISSILE_FL_HELD)&&!p->client->ps.grenadeTimeLeft);
 }
 // Death/disconnect/respawn cannot inherit a hidden grenade or reset its deadline.
 for(int loss=0;loss<3;loss++){
  reset();p=player(0);receiver=player(1);g=loose(p,800);command(receiver,0x10000,16);deadline=g->nextthink;
  if(loss==0)receiver->health=0;if(loss==1)receiver->client->sess.connected=0;if(loss==2)receiver->client->ps.stats[5]++;
  G_RunMissile(g);assert(!(g->flags&GMISSILE_FL_HELD)&&!(g->r.svFlags&1)&&g->nextthink==deadline);
  level.time=deadline;G_RunMissile(g);assert(explosions==1);
 }
 // Cook event is emitted once, consumes only inventory grenades and never cooks smoke.
 for(int thrownBack=0;thrownBack<2;thrownBack++)for(int step=1;step<=200;step++){
  reset();p=player(0);ps=&p->client->ps;ps->offHandIndex=1;ps->pm_flags=0x10|(thrownBack?PMF_GRENADE_THROWBACK:0);ps->grenadeTimeLeft=3500;ps->ammoclip[1]=2;
  pmove_t pm={.ps=ps};pml_t pml={.msec=step};int events=0;
  for(int elapsed=0;elapsed<4000;elapsed+=step)events+=PM_UpdateOffhandCook(&pm,&pml);
  assert(events==1&&ps->eventSequence==1&&ps->events[0]==0xc5&&ps->ammoclip[1]==(thrownBack?2:1));
  ps->offHandIndex=2;ps->pm_flags=0x10;ps->grenadeTimeLeft=1000;assert(!PM_UpdateOffhandCook(&pm,&pml)&&ps->grenadeTimeLeft==1000);
 }
 puts("PASS: player/object impacts bounce; original fuse survives pickup/rethrow; zero-ammo throwback; one cook event; wall/expiry/race rejection; death/disconnect/respawn and no-input expiry");
}
'''
# Keep damageable targets in the collision fixture; the factory leaves grenades harmless on contact.
support = support.replace('int useCount,handler,flags,health,parent,', 'int takedamage,useCount,handler,flags,health,parent,')
with tempfile.TemporaryDirectory(prefix='cod2-grenade-lifecycle-') as directory:
    p = Path(directory)
    for index, variant in enumerate((body, body.replace(
        'if (((_ENT(ent)->s.eFlags) & GMISSILE_EF_GRENADE_BOUNCE) || !(_ENT(other)->takedamage))',
        'if (!(_ENT(other)->takedamage))'),
        body.replace('bolt = G_GetHeldGrenade(self);', 'bolt = NULL;'),
        body.replace('ps->grenadeTimeLeft = remaining + pendingMsec;',
                     'ps->grenadeTimeLeft = 3500 + pendingMsec;'),
        body.replace('cl->ps.offHandIndex, cl->ps.grenadeTimeLeft);',
                     'cl->ps.grenadeTimeLeft, cl->ps.offHandIndex);'),
        body.replace('(float)weapDef->iExplosionInnerDamage,\n                       (float)weapDef->iExplosionOuterDamage,\n                       (float)weapDef->iExplosionRadius,',
                     '(float)weapDef->iExplosionRadius,\n                       (float)weapDef->iExplosionOuterDamage,\n                       (float)weapDef->iExplosionInnerDamage,'))):
        (p / 'test.c').write_text(defines + '\n' + support + header + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(p / 'test.c'), '-o', str(p / 'test'), '-lm'], check=True)
        result = subprocess.run([str(p / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr)
        if index == 0:
            print(result.stdout.strip())
