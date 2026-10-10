#!/usr/bin/env python3
"""Run real player collision, view-origin and movement timer code together."""
from pathlib import Path
import re
import subprocess
import tempfile
root=Path(__file__).resolve().parent.parent
active=(root/'src/PC/game_mp/g_active_mp.c').read_text()
client=(root/'src/PC/game_mp/g_client_mp.c').read_text()
move=(root/'src/PC/bgame/bg_pmove.c').read_text()
def function(source,name):
 m=re.search(r'^[^\n;]*\b'+name+r'\([^;]*?\)\n\{',source,re.M);assert m,name
 end,depth=m.end(),1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[m.start():end]+'\n'
support=r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CON_CONNECTED 2
#define PMF_JUMPING 0x80000
#define qfalse 0
#define qtrue 1
typedef int qboolean;typedef float vec_t;typedef float vec2_t[2];typedef float vec3_t[3];
typedef struct {int pm_flags,eFlags,pm_time,speed,viewlocked_entNum,legsTimer,torsoTimer,
 weaponTime,weaponDelay,weaponRestrictKickTime,foliageSoundTime,damageTimer,damageDuration,
 holdBreathTimer;vec3_t velocity,origin,viewangles;float viewHeightCurrent,leanf;}playerState_t;
typedef struct {playerState_t ps;struct {int sessionState,connected;}sess;}gclient_s;
typedef gclient_s gclient_t;
typedef struct {struct {int number;}s;struct {int inuse,contents;vec3_t absmin,absmax,maxs,currentOrigin;}r;
 int takedamage,health;gclient_t *client;}gentity_t;
typedef gentity_t gentity_s;
typedef struct {int num_entities,maxclients,time;}level_locals_t;
static level_locals_t level;static gentity_t g_entities[1024];static gclient_t clients[64];
typedef struct {struct {int integer;float value;}current;}dvar_t;
static dvar_t collision={.current.integer=80},bob;
static const dvar_t *collisionPtr=&collision;static void *imp_g_playerCollisionEjectSpeed=&collisionPtr;
static dvar_t *bg_bobMax=&bob;
typedef struct {unsigned int tag_player;}scr_const_t;
static scr_const_t scr={1};
#define SCR_CONST() (&scr)
static gentity_t *G_EntityForNum(int i){assert(i>=0&&i<1024);return &g_entities[i];}
static level_locals_t *G_Level(void){return &level;}
static int G_DObjGetWorldTagPos(gentity_t *e,unsigned int t,vec_t *out){
 if(e->s.number!=200)return 0;
 memcpy(out,e->r.currentOrigin,sizeof(vec3_t));return 1;
}
static void Com_Error(int c,const char *s){fprintf(stderr,"%s\n",s);abort();}
static float crandom(void){return .25f;}
static void Vec2Normalize(vec_t *v){float n=sqrtf(v[0]*v[0]+v[1]*v[1]);if(n){v[0]/=n;v[1]/=n;}}
static float BG_GetBobCycle(playerState_t *p){return 0;}
static float BG_GetSpeed(playerState_t *p,int t){return 0;}
static float BG_GetVerticalBobFactor(playerState_t *p,float a,float b,float c){return 0;}
static float BG_GetHorizontalBobFactor(playerState_t *p,float a,float b,float c){return 0;}
static void AngleVectors(vec_t *a,vec_t *f,vec_t *r,vec_t *u){if(r){r[0]=0;r[1]=1;r[2]=0;}}
static void AddLeanToPosition(vec_t *o,float yaw,float lean,float a,float b){}
'''
body=function(active,'StuckInClient')+function(client,'G_GetPlayerViewOrigin')+function(move,'PM_DropTimers')
checks=r'''
static void player(int n,float x,int mounted,int height){
 gentity_t *e=&g_entities[n];gclient_t *c=&clients[n];
 *c=(gclient_t){.ps={.pm_flags=0x800000,.speed=190,.eFlags=mounted?0x300:0,
 .viewlocked_entNum=mounted?200:1023,.viewHeightCurrent=height},.sess.connected=2};
 *e=(gentity_t){.s.number=n,.r={.inuse=1,.contents=0x2000000},.takedamage=1,.health=100,.client=c};
 for(int a=0;a<3;a++){e->r.absmin[a]=-15;e->r.absmax[a]=15;e->r.maxs[a]=15;}
 e->r.currentOrigin[0]=x;c->ps.origin[0]=x;
}
int main(void){int cases=0;level.maxclients=64;level.num_entities=1022;
 for(int mounted=0;mounted<2;mounted++)for(int height=11;height<=60;height+=7)
 for(int self=0;self<64;self++){
  memset(g_entities,0,sizeof(g_entities));memset(clients,0,sizeof(clients));
  int other=(self+1)%64;player(self,0,mounted,height);player(other,1,0,height);
  /* Damageable non-player entities must never be treated as a player. */
  g_entities[300]=(gentity_t){.takedamage=1,.r.inuse=1};
  g_entities[200].s.number=200;g_entities[200].r.currentOrigin[2]=234;
  assert(StuckInClient(&g_entities[self]));
  assert(clients[self].ps.eFlags==(mounted?0x300:0)&&!clients[other].ps.eFlags);
  assert((clients[self].ps.pm_flags&0x200)&&(clients[other].ps.pm_flags&0x200));
  assert(clients[self].ps.pm_time==300&&clients[other].ps.pm_time==300);
  vec3_t eye;G_GetPlayerViewOrigin(&g_entities[self],eye);
  assert(eye[2]==(mounted?234:height));
  G_GetPlayerViewOrigin(&g_entities[other],eye);assert(eye[2]==height);
  PM_DropTimers(&clients[self].ps,300);PM_DropTimers(&clients[other].ps,300);
  assert(!(clients[self].ps.pm_flags&0x200)&&!clients[self].ps.pm_time);
  assert(clients[self].ps.eFlags==(mounted?0x300:0));cases++;
 }
 for(int reason=0;reason<6;reason++){
  memset(g_entities,0,sizeof(g_entities));player(0,0,0,60);player(1,0,0,60);
  g_entities[300]=(gentity_t){.takedamage=1,.r.inuse=1};
  if(reason==0)g_entities[1].client=0;
  if(reason==1)g_entities[1].r.inuse=0;
  if(reason==2)clients[1].sess.connected=1;
  if(reason==3)clients[1].sess.sessionState=2;
  if(reason==4)g_entities[1].health=0;
  if(reason==5)g_entities[1].r.absmin[2]=100;
  assert(!StuckInClient(&g_entities[0]));assert(!clients[0].ps.eFlags);
 }
 printf("PASS: %d mounted/ordinary player collisions; camera origin, 300ms push expiry, 64 slots and non-player/dead/disconnected exclusion\n",cases);
}
'''
mutants=[body.replace('client->ps.pm_flags |= 0x200;', 'client->ps.eFlags |= 0x200;')
 .replace('selfClient->ps.pm_flags |= 0x200;','selfClient->ps.eFlags |= 0x200;'),
 body.replace('!ent->r.inuse || !ent->takedamage || !ent->client','!ent->takedamage')
 .replace('level.maxclients; ++i','level.num_entities; ++i')]
with tempfile.TemporaryDirectory(prefix='cod2-player-collision-') as d:
 p=Path(d)
 for i,b in enumerate([body]+mutants):
  assert i==0 or b!=body
  (p/'test.c').write_text(support+b+checks)
  subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-lm','-o',str(p/'test')],check=True)
  r=subprocess.run([str(p/'test')],capture_output=True,text=True)
  assert (r.returncode==0)==(i==0),r.stderr
  if i==0:print(r.stdout.strip())
print('PASS: wrong turret-flag and non-player-dereference mutations rejected')
