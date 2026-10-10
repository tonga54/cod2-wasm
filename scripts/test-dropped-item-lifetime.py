#!/usr/bin/env python3
"""Replay actual drop creation, owner grace and expiry with reused entity slots."""
from pathlib import Path
import re
import subprocess
import tempfile
root=Path(__file__).resolve().parent.parent
items=(root/'src/PC/game_mp/g_items_mp.c').read_text()
main=(root/'src/PC/game_mp/g_main_mp.c').read_text()
def function(source,name):
 m=re.search(r'^[^\n;]*\b'+name+r'\([^;]*?\)\n\{',source,re.M)
 assert m,name
 end,depth=m.end(),1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[m.start():end]+'\n'
support=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define _ENT(e) (e)
#define GITEMS_ENTITYNUM_WORLD 1022
#define TR_GRAVITY 5
#define IT_WEAPON 1
#define IT_AMMO 2
#define CON_CONNECTED 2
#define SESS_STATE_PLAYING 0
typedef float vec_t;typedef float vec3_t[3];typedef int qboolean;
typedef struct {int giType;const char *world_model[1];} gitem_t;
typedef struct {int trType,trTime;vec3_t trDelta;} trajectory_t;
typedef struct {struct {int number,eType,clientNum,groundEntityNum;struct {int item;}index;trajectory_t pos;}s;
 struct {int inuse,contents;vec3_t mins,maxs,currentAngles,currentOrigin;}r;
 int classname,handler,nextthink,flags;struct {unsigned short index;}item;}gentity_t;
typedef struct {struct {int connected,sessionState;}sess;}gclient_t;
static gentity_t g_entities[1024];static gclient_t clients[64];
static struct {int time,maxclients;gclient_t *clients;gentity_t *droppedWeaponCue[32];}level={.clients=clients};
static gitem_t itemlist[3]={{0},{IT_WEAPON,{"gun"}},{IT_AMMO,{"ammo"}}};
static struct {struct {int integer;float value;}current;}maxDrops={.current.integer=32},forwardSpeed,upBase,upRandom;
#define g_maxDroppedWeapons (&maxDrops)
#define g_dropForwardSpeed (&forwardSpeed)
#define g_dropUpSpeedBase (&upBase)
#define g_dropUpSpeedRand (&upRandom)
static int slot=100,freed,links;
static int droppedItemExpiry[1024];
static gentity_t *G_Spawn(void){gentity_t *e=&g_entities[slot];memset(e,0,sizeof(*e));e->s.number=slot;e->r.inuse=1;return e;}
static void G_FreeEntity(gentity_t *e){assert(e->r.inuse);e->r.inuse=0;freed++;for(int i=0;i<32;i++)if(level.droppedWeaponCue[i]==e)level.droppedWeaponCue[i]=0;}
static int G_ItemIndexFromPointer(const gitem_t *i){return i-itemlist;}
static void RegisterItem(int i,int update){}
static void G_GetItemClassname(const gitem_t *i,int *name){*name=i-itemlist;}
static void G_SetModel(gentity_t *e,const char *name){}
static void G_DObjUpdate(gentity_t *e){}
static void G_SetOrigin(gentity_t *e,vec_t *p){memcpy(e->r.currentOrigin,p,sizeof(vec3_t));}
static void G_SetAngle(gentity_t *e,vec_t *p){memcpy(e->r.currentAngles,p,sizeof(vec3_t));}
static void AngleVectors(vec_t *a,vec_t *f,vec_t *r,vec_t *u){if(f){f[0]=1;f[1]=f[2]=0;}}
static float crandom(void){return 0;}
static float Vec3DistanceSq(vec_t *a,vec_t *b){return 1;}
static void SV_LinkEntity(gentity_t *e){assert(e->r.inuse);links++;}
typedef void (*fn_think)(gentity_t *);
static struct {fn_think think;}entityHandlers[20];
static void Com_Error(int code,const char *s){assert(0);}
'''
body=''.join(function(items,n) for n in ('G_SetVec3','DroppedItemClearOwner','Drop_Item'))
body+=function(main,'G_RunThink_core')+function(main,'G_RunThink')
checks=r'''
int main(void){entityHandlers[15].think=DroppedItemClearOwner;int cases=0;
 for(int cycle=0;cycle<40;cycle++)for(int kind=1;kind<=2;kind++)for(int delay=0;delay<=200;delay+=50){
  slot=100+cycle%32;level.time=1000+cycle*25000;int born=level.time,before=freed;
  gentity_t owner={0};owner.s.number=cycle%64;owner.r.inuse=1;
  gentity_t *e=Drop_Item(&owner,&itemlist[kind],0,cycle%2);
  assert(e->r.inuse&&e->handler==15&&e->s.clientNum==owner.s.number);
  assert(e->nextthink==born+1000);level.time=born+999;G_RunThink(e);assert(e->s.clientNum==owner.s.number);
  level.time=born+1000+delay;G_RunThink(e);assert(e->r.inuse&&e->s.clientNum==1022);
  assert(e->nextthink==born+20000&&e->handler==15); /* pickup handler stays available */
  level.time=born+19999;G_RunThink(e);assert(e->r.inuse);
  level.time=born+20000;G_RunThink(e);assert(!e->r.inuse&&freed==before+1);
  for(int i=0;i<32;i++)assert(level.droppedWeaponCue[i]!=e);
  /* A new drop reusing the exact entity receives its own full deadline. */
  level.time=born+21000;e=Drop_Item(&owner,&itemlist[kind],0,1);
  level.time=born+22000;G_RunThink(e);assert(e->r.inuse&&e->nextthink==born+41000);
  G_FreeEntity(e);cases++;
 }
 /* A server hitch skipping the entire grace/expiry window still cleans up. */
 slot=200;level.time=1;gentity_t owner={0};gentity_t *e=Drop_Item(&owner,&itemlist[1],0,1);
 level.time=25001;G_RunThink(e);assert(!e->r.inuse);
 printf("PASS: %d drop/expiry/reuse schedules, weapons and ammo, delayed owner release, queue cleanup and long server hitch\n",cases);
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-dropped-lifetime-') as d:
 p=Path(d)
 mutant=body.replace('    pSelf->nextthink = expires;','')
 assert mutant!=body
 for i,b in enumerate((body,mutant)):
  (p/'test.c').write_text(support+b+checks)
  subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
  r=subprocess.run([str(p/'test')],capture_output=True,text=True)
  assert (r.returncode==0)==(i==0),r.stderr
  if i==0: print(r.stdout.strip())
handlers=main[main.index('entityHandler_t entityHandlers[20] ='):]
assert '{ &DroppedItemClearOwner, 0x0, 0x0, &Touch_Item_Auto' in handlers
print('PASS: disabled expiry mutant rejected; dropped-item touch handler is retained')
