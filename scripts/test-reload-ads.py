#!/usr/bin/env python3
"""Exercise ADS input across the production reload entry and interpolation."""
from pathlib import Path
import re
import subprocess
import tempfile
root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/bgame/bg_weapons.c').read_text()
def function(name):
 m=re.search(r'^[^\n;]*\b'+name+r'\([^;]*?\)\n\{',source,re.M)
 assert m,name
 end,depth=m.end(),1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[m.start():end]+'\n'
support=r'''#include <assert.h>
#include <math.h>
#include <stdio.h>
#define __attribute_regparm__(n)
typedef int qboolean;
typedef struct {int weapon,weaponstate,pm_type,pm_flags,weapAnim,weaponTime,weaponDelay,
 clientNum,damageCount,adsDelayTime,ammoclip[2];float fWeaponPosFrac;} playerState_t;
typedef struct {int bADSPositionInfo,bClipOnly,bSegmentedReload,iReloadStartTime,
 iClipIndex,weapType,iReloadEmptyTime,iReloadTime,overlayReticle,iPositionReloadTransTime,
 bRechamberWhileAds,bADSFire;float fOOPosAnimLength[2];} WeaponDef;
typedef struct {int buttons,forwardmove,rightmove,serverTime;} usercmd_t;
typedef struct {playerState_t *ps;usercmd_t cmd,oldcmd;} pmove_t;
typedef struct {int almostGroundPlane,msec;} pml_t;
typedef struct {struct {int enabled,integer;}current;} dvar_t;
static dvar_t d,*player_scopeExitOnDamage=&d,*player_adsExitDelay=&d;
static WeaponDef *bg_weaponDefs[2];static int bg_iNumWeapons=1,toggleAim,resets,reloadEvents;
static int BG_GetViewmodelWeaponIndex_core(playerState_t *ps){return ps->weapon;}
static int BG_GetViewmodelWeaponIndex(playerState_t *ps){return ps->weapon;}
static int PM_IsBinocularsADS_core(playerState_t *ps){return 0;}
static void BG_UpdateConditionValue(int c,int i,int v,int b){}
static void BG_AnimScriptEvent(playerState_t *ps,int a,int b,int c){}
static void PM_SetReloadDelay(playerState_t *ps){ps->weaponDelay=0;}
/* Dispatch the real RESET_ADS event's client effect; reload sound events remain. */
static void PM_AddEvent(playerState_t *ps,int e){if(e==0x95){toggleAim=0;resets++;}else reloadEvents++;}
'''
body=''.join(function(n) for n in ('PM_SetReloadingState','PM_BeginWeaponReload',
 'PM_UpdateAimDownSightFlag','PM_UpdateAimDownSightLerp'))
checks=r'''
static void tick(pmove_t *pm,int dt,int aim){
 pml_t pml={.msec=dt};pm->oldcmd=pm->cmd;
 pm->cmd.buttons=aim?0x1000:0;pm->cmd.serverTime+=dt;
 PM_UpdateAimDownSightFlag(pm,&pml);PM_UpdateAimDownSightLerp(pm,&pml);
 assert(isfinite(pm->ps->fWeaponPosFrac));
}
int main(void){int count=0;
 for(int segmented=0;segmented<2;segmented++)for(int empty=0;empty<2;empty++)
 for(int state=0;state<=4;state++)if(state==0||state==3||state==4)
 for(int dt=1;dt<=66;dt++)for(int input=0;input<3;input++){
  WeaponDef w={.bADSPositionInfo=1,.bSegmentedReload=segmented,.iReloadStartTime=segmented?300:0,
   .iReloadTime=1500,.iReloadEmptyTime=2300,.iClipIndex=1,.iPositionReloadTransTime=100,
   .fOOPosAnimLength={1.0f/300,1.0f/220}};bg_weaponDefs[1]=&w;
  playerState_t ps={.weapon=1,.weaponstate=state,.pm_flags=0x40,.fWeaponPosFrac=1};
  ps.ammoclip[1]=empty?0:5;pmove_t pm={.ps=&ps};toggleAim=input!=0;
  int before=resets,events=reloadEvents;PM_BeginWeaponReload(&ps);
  assert(resets==before&&reloadEvents==events+1);
  assert(ps.weaponstate==(segmented?7:5));
  for(int elapsed=0;elapsed<400;elapsed+=dt)tick(&pm,dt,input==0||toggleAim);
  assert(ps.fWeaponPosFrac==0); /* reload visibly lowers the gun */
  if(input==2)toggleAim=0; /* user cancels aim during reload */
  ps.weaponTime=0;ps.weaponstate=0;
  for(int elapsed=0;elapsed<400;elapsed+=dt)tick(&pm,dt,input==0||toggleAim);
  assert(ps.fWeaponPosFrac==(input==2?0:1));
  tick(&pm,dt,0);assert(ps.fWeaponPosFrac<1);count++;
 }
 printf("PASS: %d held/toggle/cancel ADS reload schedules, magazine/empty/segmented reload; no forced aim after cancellation\n",count);
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-reload-ads-') as d:
 p=Path(d)
 mutant=body.replace('    /* ADS interpolation lowers', '    PM_AddEvent(ps, 0x95);\n    /* ADS interpolation lowers')
 assert mutant!=body
 for i,b in enumerate((body,mutant)):
  (p/'test.c').write_text(support+b+checks)
  subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-lm','-o',str(p/'test')],check=True)
  r=subprocess.run([str(p/'test')],capture_output=True,text=True)
  assert (r.returncode==0)==(i==0),r.stderr
  if i==0:print(r.stdout.strip())
print('PASS: restoring reload RESET_ADS fails the regression')
