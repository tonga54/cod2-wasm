#!/usr/bin/env python3
"""Run the real offhand state machine and server event dispatch with original timings."""
from pathlib import Path
import re
import subprocess
import tempfile
root=Path(__file__).resolve().parent.parent
weapons=(root/'src/PC/bgame/bg_weapons.c').read_text()
active=(root/'src/PC/game_mp/g_active_mp.c').read_text()
def function(source,name):
 m=re.search(r'[^\n;]*\b'+name+r'\([^;]*?\)\n\{',source)
 assert m,name
 end=m.end();depth=1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[m.start():end]+'\n'
defs='\n'.join(s for s in weapons.splitlines() if s.startswith('#define WEAPON_') or s.startswith('#define PM_WEAPON_'))
support=r'''
#include <assert.h>
#include <string.h>
typedef int qboolean;typedef unsigned char byte;
#define WEAPTYPE_GRENADE 2
typedef struct {int weapon,offHandIndex,weaponstate,weaponTime,weaponDelay,weaponRestrictKickTime,pm_flags,grenadeTimeLeft,eFlags,ammoclip[128],eventSequence,events[4],eventParms[4],stats[1];} playerState_t;
typedef struct {int iFireTime,iFireDelay,iFuseTime,quickRaiseTime,iHoldFireTime,iClipIndex,weapType,bCookOffHold,bBoltAction;} WeaponDef;
typedef struct {playerState_t *ps;struct {int buttons,weapon;} cmd;} pmove_t;
typedef struct {int msec;} pml_t;
typedef struct {playerState_t ps;int lastServerTime;} gclient_t;
typedef struct {gclient_t *client;int health,flags;struct{int eType;}s;} gentity_t;
typedef struct {struct{int enabled;}current;} dvar_t;
static dvar_t anti={0},*antiPtr=&anti;static void *imp_g_antilag=&antiPtr;
static struct {int time;}level;
static struct {int binocular_enter,binocular_exit,binocular_fire,binocular_release,binocular_drop;} scr;
#define SCR_CONST() (&scr)
static WeaponDef *bg_weaponDefs[128];
static int shots,throws,melees,deaths,notifications,anim,animTime,throwTime,throwFuse,now;
static void BG_AddPredictableEventToPlayerstate(int e,int p,playerState_t *ps){int i=ps->eventSequence++&3;ps->events[i]=e;ps->eventParms[i]=p;}
static void PM_AddEvent(playerState_t *ps,int e){BG_AddPredictableEventToPlayerstate(e,0,ps);}
static void PM_WeaponSetAnim(playerState_t *ps,int a){anim=a;if(a==2)animTime=now;}
static void PM_SetProneMovementOverride(playerState_t *ps){}
static void PM_ResetWeaponState(playerState_t *ps){ps->weaponstate=0;ps->pm_flags&=~0x810;}
static void BG_AnimScriptEvent(playerState_t *ps,int a,int b,int c){}
static void FireWeaponAntiLag(gentity_t *e,int t){shots++;}
static void FireWeaponMelee(gentity_t *e){melees++;}
static void G_UseOffHand(gentity_t *e){throws++;throwTime=now;throwFuse=e->client->ps.grenadeTimeLeft;}
static void Scr_Notify(gentity_t *e,int s,int n){notifications++;}
static void G_Damage(gentity_t*a,void*b,void*c,void*d,void*e,int f,int g,int h,int i,int j){}
static void player_die(gentity_t*a,gentity_t*b,gentity_t*c,int d,int e,int f,void*g,int h,int i){deaths++;}
'''
body=''.join(function(weapons,n) for n in ['PM_UpdateOffhandCook','PM_UpdateWeaponTimers','PM_StartOffhandPrepare','PM_ReleaseOffhand','PM_RunOffhandState'])+function(active,'ClientEvents')
checks=r'''
static void tick(gentity_t *ent,pmove_t *pm,int step){
 pml_t pml={.msec=step};playerState_t *ps=pm->ps;int old=ps->eventSequence;now+=step;
 if(!PM_UpdateOffhandCook(pm,&pml)){
  int delayed=PM_UpdateWeaponTimers(pm,&pml);
  if(delayed||(!ps->weaponTime&&!ps->weaponDelay))PM_RunOffhandState(pm,delayed);
 }
 ClientEvents(ent,old);
}
int main(void){
 WeaponDef gun={.quickRaiseTime=250},grenade={.iClipIndex=2,.weapType=2,.bCookOffHold=1,.iFireTime=300,.iFireDelay=100,.iHoldFireTime=600,.iFuseTime=3500};
 bg_weaponDefs[1]=&gun;bg_weaponDefs[2]=&grenade;
 for(int step=1;step<=66;step++)for(int hold=0;hold<=1000;hold+=100)for(int smoke=0;smoke<2;smoke++){
  grenade.bCookOffHold=!smoke;grenade.iFuseTime=smoke?1000:3500;
  gclient_t client={.ps={.weapon=1,.offHandIndex=2,.ammoclip={[2]=2}}};gentity_t ent={.client=&client};pmove_t pm={.ps=&client.ps};
  now=throws=shots=melees=notifications=deaths=0;animTime=throwTime=-1;
  BG_AddPredictableEventToPlayerstate(0xa7,2,pm.ps);ClientEvents(&ent,0);assert(!throws);
  int old=pm.ps->eventSequence;PM_StartOffhandPrepare(pm.ps);ClientEvents(&ent,old);assert(!throws && anim==19);
  pm.cmd.buttons=PM_WEAPON_BUTTON_FRAG;
  while(pm.ps->weaponstate!=WEAPON_OFFHAND_HOLD)tick(&ent,&pm,step);
  int end=now+hold;while(now<end)tick(&ent,&pm,step);
  assert(!throws && pm.ps->ammoclip[2]==2);pm.cmd.buttons=0;
  while(pm.ps->weaponstate)tick(&ent,&pm,step);
  assert(throws==1 && pm.ps->ammoclip[2]==1 && !deaths);
  assert(throwTime-animTime>=100 && throwTime-animTime<100+step);
  assert(throwFuse>0 && throwFuse<=grenade.iFuseTime);
  int fuse=pm.ps->grenadeTimeLeft;for(int i=0;i<10;i++)tick(&ent,&pm,step);assert(pm.ps->grenadeTimeLeft==fuse);
 }
 for(int event=0x9e;event<=0xc5;event++){
  gclient_t c={.ps={.eventSequence=1,.events={event}}};gentity_t e={.client=&c};
  throws=shots=melees=notifications=deaths=0;ClientEvents(&e,0);
  assert(throws==(event==0xa6));assert(melees==(event==0xa4));
  assert(shots==(event==0x9e||event==0x9f||event==0xa0||event==0xaf));
  assert(deaths==(event==0xc5));assert(notifications==(event>=0xa8&&event<=0xac));
 }
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-offhand-') as folder:
 p=Path(folder);(p/'test.c').write_text(defs+'\n'+support+body+checks)
 subprocess.run(['cc','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: 1452 frag/smoke schedules; pull-pin, hold, throw animation, 100 ms release, single ammo use; all server event codes under ASan/UBSan')
