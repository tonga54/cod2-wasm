#!/usr/bin/env python3
"""Replay actual weapon dispatch across reload cancellation and timer boundaries."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/bgame/bg_weapons.c').read_text()


def function(name):
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


constants = source[source.index('#define WEAPON_READY'):source.index('static qboolean PM_WeaponHasInventoryWeapon')]
constants += re.search(r'#define EV_DETONATE [^\n]+\n', (root / 'src/headers/cod2_grenade.h').read_text())[0]
support = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define BM_NOINLINE
#define BM_ALWAYS_INLINE inline
#define __attribute_regparm__(n)
#define WEAPTYPE_BINOCULARS 3
#define PM_WEAPON_FLAG_TRIGGER_HELD 0x40000
typedef int qboolean;
typedef struct {int weapon,weaponstate,weaponTime,weaponDelay,weaponRestrictKickTime,
 pm_type,pm_flags,eFlags,weapAnim,clientNum,grenadeTimeLeft,weaponrechamber[4],
 weapons[4],ammo[128],ammoclip[128];char weaponslots[3];float fWeaponPosFrac,aimSpreadScale;} playerState_t;
typedef struct {int bSemiAuto,bBoltAction,bSegmentedReload,bClipOnly,bNoPartialReload,
 iAmmoIndex,iClipIndex,iClipSize,iReloadStartAdd,iReloadAmmoAdd,iReloadStartAddTime,
 iReloadStartTime,iReloadTime,iReloadEmptyTime,iReloadAddTime,iRechamberBoltTime,
 iReloadEndTime,iMeleeDamage,iMeleeDelay,iMeleeTime,iAltWeaponIndex,iDropTime,iAltDropTime,
 iRaiseTime,iAltRaiseTime,offhandClass,weapType,weapSlot,hasDetonator,iHoldFireTime;} WeaponDef;
typedef struct {int weapon,buttons,serverTime; signed char forwardmove,rightmove;} usercmd_t;
typedef struct {playerState_t *ps;usercmd_t cmd,oldcmd;} pmove_t;
typedef struct {int msec;} pml_t;
static WeaponDef defs[3],*bg_weaponDefs[3],*bg_weapClips[3];static int bg_iNumWeapons=2;
static struct {struct {int enabled;}current;} reloadDvar,*player_allowfire=&reloadDvar;
static int strikes;
static void PM_AddEvent(playerState_t *ps,int event){if(event==0xa4)strikes++;}
static void BG_AnimScriptEvent(playerState_t *ps,int a,int b,int c){}
static void BG_UpdateConditionValue(int a,int b,int c,int d){}
static void PM_UpdateWeaponConditions(playerState_t *ps,WeaponDef *def){}
static int BG_IsPlayerWeaponInSlot_core(playerState_t *ps,int w,int flag){return 1;}
static void PM_SetProneMovementOverride(playerState_t *ps){}
static void PM_ResetWeaponState(playerState_t *ps){ps->weaponstate=ps->weaponTime=ps->weaponDelay=0;}
static void PM_UpdateHoldBreath(pmove_t *pm,pml_t *pml){}
static int PM_UpdateOffhandCook(pmove_t *pm,pml_t *pml){return 0;}
static int PM_TryStartOffhand(pmove_t *pm){return 0;}
static int PM_TryToggleBinoculars(pmove_t *pm){return 0;}
static void PM_CheckReloadStart(pmove_t *pm,int delayed){}
static int PM_TryFireWeapon(pmove_t *pm,int delayed,int trigger){return 0;}
static void PM_FinishFire(playerState_t *ps){}
static void PM_FinishRechamber(playerState_t *ps){}
static void PM_RunOffhandState(pmove_t *pm,int delayed){assert(0);}
static void PM_RunBinocularState(playerState_t *ps){assert(0);}
'''
names = ('PM_WeaponHasInventoryWeapon', 'PM_WeaponSetAnim', 'PM_WeaponClearAnim',
         'PM_WeaponSetIdle', 'PM_WeaponStateIsBinoculars', 'PM_WeaponStateIsOffhand',
         'PM_WeaponStateIsReloading', 'PM_WeaponStateBlocksChange',
         'PM_FindFirstSelectableWeapon', 'PM_ResolveWeaponSelection',
         'PM_BeginWeaponChange', 'PM_FinishWeaponChange', 'PM_ShouldChangeWeapon',
         'PM_CheckWeaponChange', 'PM_ReloadClip', 'PM_Weapon_AllowReload',
         'PM_Weapon_ReloadDelayedAction', 'PM_SetReloadDelay', 'PM_SetReloadingState',
         'PM_BeginWeaponReload', 'PM_UpdateWeaponTimers', 'PM_StartMelee',
         'PM_CheckMeleeStart', 'PM_FinishMeleeDelay', 'PM_RunReloadState', 'PM_TryDetonate', 'PM_Weapon')
body = ''.join(function(name) for name in names)
checks = r'''
static void tick(pmove_t *pm,int dt){pml_t pml={dt};pm->cmd.serverTime+=dt;PM_Weapon(pm,&pml);pm->oldcmd=pm->cmd;}
static void reset(playerState_t *ps,pmove_t *pm,int empty,int bolt,int meleeDelay){
 *ps=(playerState_t){.weapon=1,.weapons={6},.weaponslots={0,1,2},.fWeaponPosFrac=1};
 ps->ammo[1]=96;ps->ammoclip[1]=empty?0:7;ps->ammoclip[2]=3;
 defs[1]=(WeaponDef){.bBoltAction=bolt,.iAmmoIndex=1,.iClipIndex=1,.iClipSize=32,
  .iReloadTime=900,.iReloadEmptyTime=1100,.iReloadAddTime=350,.iRechamberBoltTime=100,
  .iMeleeDamage=50,.iMeleeTime=300,.iMeleeDelay=meleeDelay,.iDropTime=90,.iRaiseTime=100};
 defs[2]=defs[1];defs[2].iAmmoIndex=defs[2].iClipIndex=2;
 if(bolt)ps->weaponrechamber[0]=2;
 *pm=(pmove_t){.ps=ps,.cmd={.weapon=1}};strikes=0;PM_BeginWeaponReload(ps);
}
int main(void){
 bg_weaponDefs[1]=bg_weapClips[1]=&defs[1];bg_weaponDefs[2]=bg_weapClips[2]=&defs[2];int cases=0;
 for(int dt=1;dt<=66;dt++)for(int empty=0;empty<2;empty++)for(int bolt=0;bolt<2;bolt++)
 for(int mode=0;mode<3;mode++)for(int when=0;when<5;when++){
  playerState_t ps;pmove_t pm;reset(&ps,&pm,empty,bolt,mode==2?125:0);int clip=ps.ammoclip[1];
  int duration=ps.weaponTime,cancelAt=(int[]){0,99,349,700,duration-1}[when];
  while(pm.cmd.serverTime+dt<cancelAt){tick(&pm,dt);assert(ps.ammoclip[1]==clip&&ps.ammo[1]==96);}
  if(!mode)pm.cmd.weapon=2;else pm.cmd.buttons=PM_WEAPON_BUTTON_MELEE;
  tick(&pm,dt);
  assert(ps.ammoclip[1]==clip&&ps.ammo[1]==96&&!PM_WeaponStateIsReloading(ps.weaponstate));
  if(!mode)assert(ps.weaponstate==WEAPON_DROPPING&&ps.weaponDelay==0);
  else assert(ps.weaponstate==(mode==2?WEAPON_MELEE_INIT:WEAPON_MELEE_FIRE));
  for(int t=0;t<1500;t+=dt)tick(&pm,dt);
  assert(ps.ammoclip[1]==clip&&ps.ammo[1]==96&&ps.weaponstate==WEAPON_READY);
  if(!mode){assert(ps.weapon==2&&!strikes);pm.cmd.weapon=1;for(int t=0;t<500;t+=dt)tick(&pm,dt);assert(ps.weapon==1);}
  else {assert(ps.weapon==1&&strikes==1);pm.cmd.buttons=0;tick(&pm,dt);}
  /* A fresh reload after cancelling still completes normally, once. */
  PM_BeginWeaponReload(&ps);for(int t=0;t<2000;t+=dt)tick(&pm,dt);
  assert(ps.ammoclip[1]==32&&ps.ammo[1]==96-(32-clip));cases++;
 }
 /* Segmented reload: an unfinished insertion is cancelled, while rounds
  * from a previous completed insertion survive. */
 for(int mode=0;mode<2;mode++){
  playerState_t ps;pmove_t pm;reset(&ps,&pm,1,0,100);defs[1].bSegmentedReload=1;defs[1].iReloadAmmoAdd=1;
  while(ps.ammoclip[1]==0)tick(&pm,10);assert(ps.ammoclip[1]==1&&ps.ammo[1]==95);
  for(int t=0;t<500;t+=10)tick(&pm,10);
  if(mode)pm.cmd.buttons=PM_WEAPON_BUTTON_MELEE;else pm.cmd.weapon=2;
  tick(&pm,10);for(int t=0;t<1500;t+=10)tick(&pm,10);
  assert(ps.ammoclip[1]==1&&ps.ammo[1]==95);
 }
 printf("PASS: %d magazine reload cancellations, switch-back, immediate/delayed melee, timer boundaries, bolt and empty reloads; completed segmented rounds retained\n",cases);
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-reload-cancel-') as directory:
    path = Path(directory)
    variants = (body,
                body.replace('if (!ps->weaponTime)\n        PM_ReloadClip(ps);', 'PM_ReloadClip(ps);', 1),
                body.replace('PM_WeaponStateBlocksChange(ps->weaponstate) &&\n        !PM_WeaponStateIsReloading(ps->weaponstate)', 'PM_WeaponStateBlocksChange(ps->weaponstate)', 1),
                body.replace('!PM_WeaponStateIsReloading(ps->weaponstate)) ||', '1) ||'))
    for index, code in enumerate(variants):
        (path / 'test.c').write_text(support + constants + code + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(path / 'test.c'), '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True, timeout=30)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr)
        if index == 0:
            print(result.stdout.strip())
print('PASS: premature ammo transfer, blocked switch and blocked melee mutations fail')
