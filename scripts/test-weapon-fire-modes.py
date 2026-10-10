#!/usr/bin/env python3
"""Replay the real firing/reload state machine using every packaged retail gun."""
from pathlib import Path
import json
import re
import subprocess
import tempfile
import zipfile
from weapon_balance import apply_weapon_balance, load_profile, weapon_fields

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/bgame/bg_weapons.c').read_text()

def function(name):
    m = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert m, name
    end, depth = m.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[m.start():end] + '\n'

original = {}
for path in sorted((root / 'data/main').glob('*.iwd')):
    with zipfile.ZipFile(path) as archive:
        for name in archive.namelist():
            if name.startswith('weapons/mp/'):
                original[name] = (path.name, archive.read(name))
profile = load_profile()
report, rows = [], []
fields = {
    'semiAuto': 'bSemiAuto', 'boltAction': 'bBoltAction',
    'clipSize': 'iClipSize', 'fireTime': 'iFireTime', 'fireDelay': 'iFireDelay',
    'rechamberTime': 'iRechamberTime', 'rechamberBoltTime': 'iRechamberBoltTime',
    'reloadTime': 'iReloadTime', 'reloadEmptyTime': 'iReloadEmptyTime',
    'reloadAddTime': 'iReloadAddTime', 'reloadStartTime': 'iReloadStartTime',
    'reloadStartAddTime': 'iReloadStartAddTime', 'reloadEndTime': 'iReloadEndTime',
    'reloadAmmoAdd': 'iReloadAmmoAdd', 'reloadStartAdd': 'iReloadStartAdd',
    'segmentedReload': 'bSegmentedReload', 'noPartialReload': 'bNoPartialReload',
    'hipSpreadFireAdd': 'fHipSpreadFireAdd',
}
with zipfile.ZipFile(root / 'data/browser/main/cod2_browser_renderer.iwd') as archive:
    for name in sorted(n for n in archive.namelist() if n.startswith('weapons/mp/')):
        data = archive.read(name)
        retail_source, retail = original[name]
        assert data == apply_weapon_balance(name, retail, profile), name
        f = weapon_fields(data)
        before = weapon_fields(retail)
        mode = ('mounted automatic' if f.get('weaponClass') == 'turret' else
                'grenade' if f.get('weaponType') == 'grenade' else
                'item' if f.get('weaponType') != 'bullet' else
                'pump' if f.get('weaponClass') == 'spread' else
                'bolt action' if f.get('boltAction') == '1' else
                'semi-automatic' if f.get('semiAuto') == '1' else 'automatic')
        report.append({'weapon': name.removeprefix('weapons/mp/'), 'mode': mode,
                       'retailSource': retail_source,
                       'originalTimingPreserved': all(f.get(k) == before.get(k) for k in fields),
                       'definition': {k: f[k] for k in fields if k in f},
                       'reviewedBalanceChanges': {k: [before[k], f[k]] for k in f if before[k] != f[k]}})
        assert report[-1]['originalTimingPreserved']
        if f.get('weaponType') != 'bullet' or f.get('weaponClass') == 'turret':
            continue
        values = []
        for key, member in fields.items():
            if key in f:
                value = float(f[key])
                if key.endswith('Time') or key.endswith('Delay'):
                    value = int(value * 1000)
                values.append(f'.{member}={value}')
        rows.append('{.name=' + json.dumps(name) + ',.def={' + ','.join(values) + '}}')

defines = '\n'.join(s for s in source.splitlines() if s.startswith(('#define WEAPON_', '#define PM_WEAPON_')))
support = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __attribute_regparm__(n)
#define BM_ALWAYS_INLINE inline
#define WEAPTYPE_GRENADE 2
typedef int qboolean;
typedef struct {
 int weapon,weaponstate,weaponTime,weaponDelay,weaponRestrictKickTime,
     pm_type,pm_flags,eFlags,weapAnim,clientNum,offHandIndex,grenadeTimeLeft,
     weaponrechamber[4],ammo[128],ammoclip[128];
 float fWeaponPosFrac,aimSpreadScale;
} playerState_t;
typedef struct {
 int bSemiAuto,bBoltAction,iClipSize,iFireTime,iFireDelay,iRechamberTime,
     iRechamberBoltTime,iReloadTime,iReloadEmptyTime,iReloadAddTime,
     iReloadStartTime,iReloadStartAddTime,iReloadEndTime,iReloadAmmoAdd,
     iReloadStartAdd,bSegmentedReload,bNoPartialReload,iAmmoIndex,iClipIndex,
     weapType,offhandClass,bCookOffHold,bClipOnly,bADSFire,iHoldFireTime,iFuseTime,
     adsGunKickReducedKickBullets,hipGunKickReducedKickBullets;
 float fHipSpreadFireAdd,fOOPosAnimLength[2];
} WeaponDef;
typedef struct {int buttons,weapon,serverTime; signed char forwardmove,rightmove;} usercmd_t;
typedef struct {playerState_t *ps;usercmd_t cmd,oldcmd;} pmove_t;
typedef struct {int msec;float frametime;} pml_t;
static WeaponDef *bg_weaponDefs[128], *bg_weapClips[128];
static int bg_iNumWeapons=1, now, shots, shotTimes[256], rechambers, boltEvents;
static void PM_AddEvent(playerState_t *ps,int e) {
    if(e==0x9e||e==0xa0) {assert(shots<256);shotTimes[shots++]=now;}
    if(e==0xa1)rechambers++;
    if(e==0xa2)boltEvents++;
}
static int BG_AnimScriptEvent(playerState_t *ps,int a,int b,int c) {return 0;}
static void PM_SetProneMovementOverride(playerState_t *ps) {}
static int BG_TakePlayerWeapon(playerState_t *ps,int w) {assert(0);return 0;}
static void PM_UpdateAimDownSightLerp(pmove_t *pm,pml_t *pml) {}
static void PM_UpdateHoldBreath(pmove_t *pm,pml_t *pml) {}
static void PM_CheckWeaponChange(pmove_t *pm) {}
static int PM_TryStartOffhand(pmove_t *pm) {return 0;}
static int PM_TryToggleBinoculars(pmove_t *pm) {return 0;}
static int PM_CheckMeleeStart(pmove_t *pm,int d) {return 0;}
static void PM_FinishWeaponChange(pmove_t *pm) {assert(0);}
static void PM_FinishMeleeDelay(playerState_t *ps) {assert(0);}
static void PM_RunOffhandState(pmove_t *pm,int d) {assert(0);}
static void PM_RunBinocularState(playerState_t *ps) {assert(0);}
'''
names = ('PM_WeaponSetAnim', 'PM_WeaponClearAnim', 'PM_WeaponSetIdle',
         'PM_WeaponStateIsBinoculars', 'PM_WeaponStateIsOffhand', 'PM_WeaponStateIsReloading',
         'PM_SetWeaponRechamberBit', 'PM_ClearWeaponRechamberBit', 'PM_HasWeaponRechamberBit',
         'PM_ReloadClip', 'PM_Weapon_AllowReload', 'PM_Weapon_ReloadDelayedAction',
         'PM_SetReloadDelay', 'PM_SetReloadingState', 'PM_BeginWeaponReload',
         'PM_WeaponCanReloadNow', 'PM_CheckReloadStart', 'PM_UpdateOffhandCook',
         'PM_UpdateWeaponTimers', 'PM_StartRechamber', 'PM_FinishRechamber',
         'PM_AddFireSpread', 'PM_TryTakeEmptyClipOnlyWeapon', 'PM_ConsumeFireAmmo',
         'PM_StartFireTimers', 'PM_TryStartGrenadeFire', 'PM_TryFireWeapon',
         'PM_FinishFire', 'PM_RunReloadState', 'PM_ResetWeaponState', 'PM_Weapon')
grenade_header = '#define OFFHAND_CLASS_FRAG_GRENADE 1\n' + (root/'src/headers/cod2_grenade.h').read_text()
body = ''.join(function(n) for n in names)
checks = r'''
typedef struct {const char *name;WeaponDef def;} Gun;
static void reset(pmove_t *pm,playerState_t *ps,WeaponDef *w,int ads,int clip) {
    *ps=(playerState_t){.weapon=1,.fWeaponPosFrac=ads};
    ps->ammoclip[1]=clip;ps->ammo[1]=128;
    *pm=(pmove_t){.ps=ps,.cmd={.weapon=1}};
    bg_weaponDefs[1]=bg_weapClips[1]=w;
    w->iAmmoIndex=w->iClipIndex=1;
    now=shots=rechambers=boltEvents=0;
}
static void tick(pmove_t *pm,int dt,int buttons) {
    pml_t pml={.msec=dt,.frametime=dt*.001f};
    now+=dt;pm->cmd.buttons=buttons;pm->cmd.serverTime=now;
    PM_Weapon(pm,&pml);pm->oldcmd=pm->cmd;
    assert(pm->ps->weaponTime>=0&&pm->ps->weaponDelay>=0);
}
static void cadence(WeaponDef *w,int step,int bolt,int continuous) {
    for(int i=1;i<shots;i++) {
        int min=w->iFireTime+(bolt?w->iRechamberTime:0);
        assert(shotTimes[i]-shotTimes[i-1]>=min);
        if(continuous)assert(shotTimes[i]-shotTimes[i-1]<min+step);
    }
}
int main(void) {
 int cases=0;
 for(int g=0;g<sizeof(guns)/sizeof(guns[0]);g++) {
  WeaponDef w=guns[g].def;assert(w.iFireTime>0&&w.iClipSize>0);
  /* Every remaining clip count, not just an empty/one-round-short magazine.
   * Garand needs an empty clip; Enfield needs room for a complete five-round
   * charger. Other segmented rifles transfer single rounds. */
  for(int clip=0;clip<=w.iClipSize;clip++)for(int reserve=0;reserve<=1;reserve++) {
   playerState_t ps;pmove_t pm;reset(&pm,&ps,&w,0,clip);ps.ammo[1]=reserve?200:0;
   int allowed=reserve && clip<w.iClipSize;
   if(w.bNoPartialReload) {
    int add=w.iReloadAmmoAdd;
    allowed &= (!add||add>=w.iClipSize)?clip==0:clip+add<=w.iClipSize;
   }
   assert(!!PM_Weapon_AllowReload(&ps)==allowed);
   tick(&pm,16,PM_WEAPON_BUTTON_RELOAD);
   assert(!!PM_WeaponStateIsReloading(ps.weaponstate)==allowed);
   if(allowed) {
    int total=ps.ammoclip[1]+ps.ammo[1],limit=now+30000;
    while(ps.weaponstate!=WEAPON_READY&&now<limit) {
     tick(&pm,16,0);assert(ps.ammoclip[1]+ps.ammo[1]==total);
    }
    assert(now<limit&&ps.ammoclip[1]>clip);
   } else assert(ps.ammoclip[1]==clip);
  }
  for(int step=1;step<=66;step++)for(int ads=0;ads<2;ads++) {
   playerState_t ps;pmove_t pm;reset(&pm,&ps,&w,ads,w.iClipSize);
   /* A held semi-auto trigger fires only once, including the last round.
    * Automatics repeat at the file's rate without an extra READY frame. */
   int duration=w.iFireTime*4+w.iRechamberTime+500;
   for(int t=0;t<duration;t+=step)tick(&pm,step,1);
   if(w.bSemiAuto)assert(shots==1&&ps.ammoclip[1]==w.iClipSize-1);
   else {assert(shots>1);cadence(&w,step,0,1);}
   reset(&pm,&ps,&w,ads,1);ps.ammo[1]=0;
   if(w.bSemiAuto) {
    for(int t=0;t<duration;t+=step)tick(&pm,step,1);
    assert(shots==1&&ps.ammoclip[1]==0);
   }
   /* A semi-auto press during recovery is discarded, not buffered into
    * another shot as soon as the timer expires. Release and press once ready. */
   reset(&pm,&ps,&w,ads,w.iClipSize);
   tick(&pm,step,1);tick(&pm,step,0);
   int end=now+w.iFireTime+w.iRechamberTime+step*4;
   while(now<end)tick(&pm,step,1);
   if(w.bSemiAuto && step*2<w.iFireTime) {
    assert(shots==1);
    tick(&pm,step,0);tick(&pm,step,1);
    assert(shots==2);cadence(&w,step,w.bBoltAction,0);
   }
   if(w.bBoltAction) {assert(rechambers==1&&boltEvents==1);}
   /* Repeated clicks during recovery must never shorten the shot interval.
    * Vary both press/release lengths and the command step (1..66 ms). */
   for(int pattern=0;pattern<12;pattern++) {
    reset(&pm,&ps,&w,ads,w.iClipSize);ps.ammo[1]=0;
    for(int n=0;n<1000;n++) {
     int dt=pattern<6?step:1+(n*17+step*13)%66;
     int period=2+pattern%6;
     int buttons=n%period<1+pattern%3?1:0;
     tick(&pm,dt,buttons);
     cadence(&w,66,w.bBoltAction,0);
     assert(ps.ammoclip[1]==w.iClipSize-shots);
    }
   }
   /* The firing helper enforces recovery without depending on its caller. */
   reset(&pm,&ps,&w,ads,w.iClipSize);
   tick(&pm,step,1);
   if(ps.weaponTime) {
    int ammo=ps.ammoclip[1];
    assert(!PM_TryFireWeapon(&pm,0,1));
    assert(shots==1&&ps.ammoclip[1]==ammo);
   }
   if(w.bSemiAuto) {
    /* Holding through raising cannot queue a shot. */
    reset(&pm,&ps,&w,ads,w.iClipSize);
    ps.weaponstate=WEAPON_RAISING;ps.weaponTime=w.iFireTime;
    for(int t=0;t<duration;t+=step)tick(&pm,step,1);
    assert(!shots&&ps.weaponstate==WEAPON_READY);
    tick(&pm,step,0);tick(&pm,step,1);assert(shots==1);
    /* Reload still completes while attack is held; it requires a fresh press. */
    reset(&pm,&ps,&w,ads,0);
    tick(&pm,step,PM_WEAPON_BUTTON_RELOAD);
    int limit=now+30000;
    while(ps.weaponstate!=WEAPON_READY&&now<limit)tick(&pm,step,1);
    assert(now<limit&&ps.ammoclip[1]>0&&!shots);
    tick(&pm,step,1);assert(!shots);
    tick(&pm,step,0);tick(&pm,step,1);assert(shots==1);
   }
   /* Complete a full magazine with distinct clicks and mandatory cycling. */
   reset(&pm,&ps,&w,ads,w.iClipSize);ps.ammo[1]=0;
   for(int n=0;n<w.iClipSize;n++) {
    int before=shots;tick(&pm,step,1);
    assert(shots==before+1);
    while(ps.weaponstate!=WEAPON_READY)tick(&pm,step,0);
   }
   assert(shots==w.iClipSize&&ps.ammoclip[1]==0);
   cadence(&w,step,w.bBoltAction,0);
   if(w.bBoltAction)assert(rechambers==w.iClipSize-1&&boltEvents==rechambers);
   /* Partial and empty reloads conserve ammo; segmented rifles/shotgun
    * transfer their original per-stage amounts and finish completely. */
   for(int empty=0;empty<2;empty++) {
    reset(&pm,&ps,&w,ads,empty?0:w.iClipSize-1);
    int total=ps.ammoclip[1]+ps.ammo[1],expected=w.iClipSize;
    if(!PM_Weapon_AllowReload(&ps)) {
        tick(&pm,step,PM_WEAPON_BUTTON_RELOAD);
        assert(ps.weaponstate==WEAPON_READY&&ps.ammoclip[1]==w.iClipSize-1);
        continue;
    }
    if(!w.bSegmentedReload&&w.iReloadAmmoAdd>0&&ps.ammoclip[1]+w.iReloadAmmoAdd<expected)
        expected=ps.ammoclip[1]+w.iReloadAmmoAdd;
    tick(&pm,step,PM_WEAPON_BUTTON_RELOAD);
    assert(PM_WeaponStateIsReloading(ps.weaponstate));
    int limit=now+30000;
    while(ps.weaponstate!=WEAPON_READY&&now<limit) {
     tick(&pm,step,0);assert(ps.ammoclip[1]+ps.ammo[1]==total);
    }
    if(!(now<limit&&ps.ammoclip[1]==expected))fprintf(stderr,"reload %s step=%d empty=%d clip=%d/%d state=%d time=%d\n",guns[g].name,step,empty,ps.ammoclip[1],expected,ps.weaponstate,now);
    assert(now<limit&&ps.ammoclip[1]==expected);
    assert(!ps.weaponTime&&!ps.weaponDelay);
   }
   cases++;
  }
 }
 printf("PASS: %d guns, %d configurations including %d rapid-click schedules; unbuffered semi-auto presses, original cadence and ammo conservation\n",
        (int)(sizeof(guns)/sizeof(guns[0])),cases,cases*12);
}
'''
checks = checks.replace('int main(void)', 'static Gun guns[]={' + ',\n'.join(rows) + '};\nint main(void)')
with tempfile.TemporaryDirectory(prefix='cod2-fire-modes-') as directory:
    d = Path(directory)
    buffered = body.replace('        ps->pm_flags |= PM_WEAPON_FLAG_TRIGGER_HELD;', '        ;')
    buffered = buffered.replace('    ps->weaponDelay = weapDef->iFireDelay;',
        '    if (weapDef->bSemiAuto) ps->pm_flags |= PM_WEAPON_FLAG_TRIGGER_HELD;\n    ps->weaponDelay = weapDef->iFireDelay;')
    variants = [body,
                body.replace('weapDef->bSemiAuto', '0'),
                body.replace('if (ps->weaponstate == WEAPON_READY)\n                PM_TryFireWeapon(pm, 0, triggerPressed);', ''),
                buffered,
                body.replace('if (!delayedAction && (ps->weaponTime || ps->weaponDelay))', 'if (0)')]
    assert all(v != body for v in variants[1:])
    for index, variant in enumerate(variants):
        (d / 'test.c').write_text(defines + '\n' + support + grenade_header + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(d / 'test.c'), '-o', str(d / 'test'), '-lm'], check=True)
        result = subprocess.run([str(d / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr)
        if index == 0:
            print(result.stdout.strip())
(root / 'out/weapon-fire-modes-audit.json').write_text(json.dumps({'weapons': report}, indent=2) + '\n')
print(f'PASS: all {len(report)} packaged definitions audited; missing semi-auto, extra READY-frame, buffered-press and missing recovery-guard mutants fail')
