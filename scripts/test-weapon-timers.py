#!/usr/bin/env python3
"""Exercise the actual movement/weapon timer handoff and magazine reload action."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parent.parent
move=(root/'src/PC/bgame/bg_pmove.c').read_text()
weapons=(root/'src/PC/bgame/bg_weapons.c').read_text()
def function(source, name):
    import re
    m=re.search(r'(?:static )?[^\n;]*\b'+name+r'\([^;]*?\)\n\{', source)
    assert m, name
    end=m.end(); depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}'); end+=1
    return source[m.start():end]+'\n'
drop=function(move,'PM_DropTimers')
body=''.join(function(weapons,n) for n in ['PM_ReloadClip','PM_Weapon_ReloadDelayedAction','PM_UpdateOffhandCook','PM_UpdateWeaponTimers'])
support=r'''
#include <assert.h>
#include <string.h>
#define __attribute_regparm__(n)
#define PMF_JUMPING 0x80000
#define PM_WEAPON_FLAG_OFFHAND 0x10
#define PM_WEAPON_BUTTON_ATTACK 1
#define WEAPTYPE_GRENADE 2
#define WEAPON_OFFHAND_INIT 12
#define WEAPON_OFFHAND_FIRE 15
#define WEAPON_OFFHAND_END 16
#define WEAPON_FIRING 3
#define PM_WEAPON_FLAG_TRIGGER_HELD 0x40000
typedef int qboolean;
typedef struct { int pm_time,pm_flags,legsTimer,torsoTimer,weaponTime,weaponDelay,grenadeTimeLeft,weaponRestrictKickTime,foliageSoundTime,damageTimer,damageDuration,holdBreathTimer,weapon,weaponstate,offHandIndex,weaponrechamber[4],ammo[128],ammoclip[128]; } playerState_t;
typedef struct { int bBoltAction,bSemiAuto,iReloadStartAddTime,iReloadStartTime,iReloadTime,iReloadEmptyTime,iReloadAddTime,iRechamberBoltTime,iClipIndex,iAmmoIndex,iClipSize,iReloadStartAdd,iReloadAmmoAdd,weapType,offhandClass,bCookOffHold; } WeaponDef;
typedef struct { playerState_t *ps;struct {int buttons,weapon;} cmd; } pmove_t;
typedef struct { int msec; } pml_t;
static WeaponDef weapon,grenade,*bg_weaponDefs[128],*bg_weapClips[128];
static int events;
static void PM_AddEvent(playerState_t *ps,int event) { events++; }
'''
body = '#define OFFHAND_CLASS_FRAG_GRENADE 1\n' + (root/'src/headers/cod2_grenade.h').read_text() + body
checks=r'''
int main(void){
 weapon=(WeaponDef){.iClipIndex=1,.iAmmoIndex=1,.iClipSize=32};
 bg_weaponDefs[1]=bg_weapClips[1]=&weapon;
 for(int step=1;step<=200;step++) for(int duration=1;duration<=300;duration++) {
  playerState_t ps={.weapon=1,.weaponstate=5,.weaponTime=duration+100,.weaponDelay=duration,.weaponRestrictKickTime=duration+100,.pm_time=duration};
  ps.ammo[1]=192;
  pmove_t pm={.ps=&ps};pml_t pml={.msec=step};int actions=0;
  for(int time=0;time<duration+400;time+=step) {
   int before=ps.weaponTime;
   PM_DropTimers(&ps,step);
   assert(ps.weaponTime==before);
   if(PM_UpdateWeaponTimers(&pm,&pml)) { actions++;PM_Weapon_ReloadDelayedAction(&ps); }
   assert(ps.weaponTime==(before>step ? before-step : 0));
  }
  assert(actions==1 && ps.ammoclip[1]==32 && ps.ammo[1]==160);
  assert(!ps.pm_time && !ps.weaponTime && !ps.weaponDelay && !ps.weaponRestrictKickTime);
 }
 grenade=(WeaponDef){.weapType=WEAPTYPE_GRENADE,.bCookOffHold=1,.iClipIndex=2};bg_weaponDefs[2]=&grenade;
 playerState_t ps={.offHandIndex=2,.pm_flags=PM_WEAPON_FLAG_OFFHAND,.grenadeTimeLeft=100};
 ps.ammoclip[2]=1;pmove_t pm={.ps=&ps};pml_t pml={.msec=20};
 for(int i=0;i<5;i++){PM_DropTimers(&ps,20);assert(PM_UpdateOffhandCook(&pm,&pml)==(i==4));}
 assert(ps.ammoclip[2]==0 && events==1);return 0;
}
'''
old=drop.replace('    if (ps->pm_time > 0)', '''    if (ps->weaponTime > 0) { ps->weaponTime-=msec; if(ps->weaponTime<0)ps->weaponTime=0; }
    if (ps->weaponDelay > 0) { ps->weaponDelay-=msec; if(ps->weaponDelay<0)ps->weaponDelay=0; }
    if (ps->pm_time > 0)''')
with tempfile.TemporaryDirectory(prefix='cod2-weapon-timers-') as d:
 p=Path(d)
 for i,variant in enumerate((drop,old)):
  (p/'test.c').write_text(support+variant+body+checks)
  subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
  result=subprocess.run([str(p/'test')],capture_output=True)
  assert (result.returncode==0)==(i==0),result.stderr.decode()
print('PASS: 60,000 timer/reload schedules, one ammo transfer, single-speed cook timer; old double decrement fails')
