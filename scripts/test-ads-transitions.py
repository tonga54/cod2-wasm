#!/usr/bin/env python3
"""Replay production ADS interpolation using the packaged weapon timings."""
from pathlib import Path
import json
import re
import subprocess
import tempfile
import zipfile
from weapon_balance import weapon_fields

root = Path(__file__).resolve().parent.parent
weapon_source = (root / 'src/PC/bgame/bg_weapons.c').read_text()
loader_source = (root / 'src/PC/bgame/bg_weapons_load_obj.c').read_text()
move_source = (root / 'src/PC/bgame/bg_pmove.c').read_text()


def function(source, name):
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


loader = function(loader_source, 'BG_LoadWeaponDefInternal')
a = loader.index('    if (weapDef->i')
b = loader.index('    if (weapDef->destabilizationBaseTime', a)
initialize = 'static void Initialize(WeaponDef *weapDef) {\n' + loader[a:b] + '}\n'
lerp = function(weapon_source, 'PM_UpdateAimDownSightLerp')
rows, report = [], []
with zipfile.ZipFile(root / 'data/browser/main/cod2_browser_renderer.iwd') as archive:
    for name in sorted(n for n in archive.namelist() if n.startswith('weapons/mp/')):
        fields = weapon_fields(archive.read(name))
        if fields.get('weaponType') != 'bullet' or fields.get('weaponClass') == 'turret':
            continue
        times = [int(float(fields.get(k, '0')) * 1000) for k in
                 ('adsTransInTime', 'adsTransOutTime', 'fireTime', 'rechamberTime')]
        assert times[0] > 0 and times[1] > 0, name
        rows.append('{' + ','.join(map(str, times)) + '}')
        report.append({'weapon': name.split('/')[-1], 'enterMs': times[0], 'exitMs': times[1]})

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef int qboolean;
typedef struct {int weapon,pm_flags,damageCount,adsDelayTime,weaponstate,weaponTime,weaponDelay;float fWeaponPosFrac;} playerState_t;
typedef struct {int iAdsTransInTime,iAdsTransOutTime,iFireTime,iRechamberTime,overlayReticle,
 bADSPositionInfo,bSegmentedReload,iPositionReloadTransTime,bRechamberWhileAds,bADSFire;
 float fOOPosAnimLength[2];} WeaponDef;
typedef struct {playerState_t *ps;struct {int serverTime;}cmd;} pmove_t;
typedef struct {int msec;}pml_t;
typedef struct {struct {int enabled,integer;}current;} dvar_t;
static dvar_t damage,delay,*player_scopeExitOnDamage=&damage,*player_adsExitDelay=&delay;
static WeaponDef *bg_weaponDefs[2];static int events;
static int BG_GetViewmodelWeaponIndex(playerState_t *ps){return ps->weapon;}
static void PM_AddEvent(playerState_t *ps,int e){assert(e==0x95);events++;}
'''
checks = r'''
static void step(pmove_t *pm,int dt,int ads){
 pml_t pml={dt};pm->cmd.serverTime+=dt;
 if(ads)pm->ps->pm_flags|=0x40;else pm->ps->pm_flags&=~0x40;
 PM_UpdateAimDownSightLerp(pm,&pml);
 assert(isfinite(pm->ps->fWeaponPosFrac)&&pm->ps->fWeaponPosFrac>=0&&pm->ps->fWeaponPosFrac<=1);
}
static void near(float actual,float expected){assert(fabsf(actual-expected)<.0001f);}
int main(void){
 int cases=0;
 for(int gun=0;gun<sizeof(timings)/sizeof(timings[0]);gun++){
  WeaponDef w={.iAdsTransInTime=timings[gun][0],.iAdsTransOutTime=timings[gun][1],
   .iFireTime=timings[gun][2],.iRechamberTime=timings[gun][3],.bADSPositionInfo=1};
  bg_weaponDefs[1]=&w;Initialize(&w);
  near(w.fOOPosAnimLength[0]*w.iAdsTransInTime,1);
  near(w.fOOPosAnimLength[1]*w.iAdsTransOutTime,1);
  /* Cadence and bolt changes cannot alter an aim animation's duration. */
  float in=w.fOOPosAnimLength[0],out=w.fOOPosAnimLength[1];
  w.iFireTime=1;w.iRechamberTime=9999;Initialize(&w);
  near(w.fOOPosAnimLength[0],in);near(w.fOOPosAnimLength[1],out);
  for(int dt=1;dt<=66;dt++){
   playerState_t ps={.weapon=1};pmove_t pm={.ps=&ps};
   int elapsed=0;
   while(elapsed<w.iAdsTransInTime+dt){
    step(&pm,dt,1);elapsed+=dt;
    near(ps.fWeaponPosFrac,fminf(1,(float)elapsed/w.iAdsTransInTime));
   }
   elapsed=0;
   while(elapsed<w.iAdsTransOutTime+dt){
    step(&pm,dt,0);elapsed+=dt;
    near(ps.fWeaponPosFrac,fmaxf(0,1-(float)elapsed/w.iAdsTransOutTime));
   }
   /* Reverse midway repeatedly without resetting or snapping the blend. */
   for(int n=0;n<100;n++){
    float before=ps.fWeaponPosFrac;int ads=n%3!=0;
    step(&pm,dt,ads);
    near(ps.fWeaponPosFrac,fmaxf(0,fminf(1,before+(ads?dt*in:-dt*out))));
   }
   /* Vary command duration, including the same time split across packets. */
   ps.fWeaponPosFrac=.2f;
   for(int n=0;n<100;n++){
    int ms=1+(n*17+dt)%66;float before=ps.fWeaponPosFrac;int ads=n%2;
    step(&pm,ms,ads);
    near(ps.fWeaponPosFrac,fmaxf(0,fminf(1,before+(ads?ms*in:-ms*out))));
   }
   cases++;
  }
 }
 WeaponDef w={.bADSPositionInfo=1};bg_weaponDefs[1]=&w;Initialize(&w);
 assert(isfinite(w.fOOPosAnimLength[0])&&w.fOOPosAnimLength[0]>0);
 assert(isfinite(w.fOOPosAnimLength[1])&&w.fOOPosAnimLength[1]>0);
 w.iAdsTransInTime=w.iAdsTransOutTime=-1;Initialize(&w);
 assert(isfinite(w.fOOPosAnimLength[0])&&w.fOOPosAnimLength[0]>0);
 playerState_t ps={.weapon=1,.fWeaponPosFrac=1};pmove_t pm={.ps=&ps};
 /* The existing scope damage interruption and no-ADS weapon rules remain. */
 damage.current.enabled=1;w.overlayReticle=1;ps.damageCount=1;
 step(&pm,16,1);assert(!ps.fWeaponPosFrac&&events==1&&!(ps.pm_flags&0x40));
 damage.current.enabled=0;w.overlayReticle=0;ps.damageCount=0;
 w.bADSPositionInfo=0;ps.fWeaponPosFrac=.5f;step(&pm,16,1);assert(!ps.fWeaponPosFrac);
 w.bADSPositionInfo=1;ps.fWeaponPosFrac=1;delay.current.integer=100;
 step(&pm,20,0);assert(ps.fWeaponPosFrac==1);
 for(int n=0;n<4;n++){step(&pm,20,0);assert(ps.fWeaponPosFrac==1);}
 step(&pm,20,0);assert(ps.fWeaponPosFrac<1&&!ps.adsDelayTime);
 delay.current.integer=0;
 for(int reason=0;reason<3;reason++){
  ps.fWeaponPosFrac=.5f;ps.weaponstate=reason==2?4:5;
  ps.weaponTime=200;w.iPositionReloadTransTime=100;
  w.bSegmentedReload=reason==0;w.bRechamberWhileAds=0;
  step(&pm,20,1);assert(ps.fWeaponPosFrac<.5f);
 }
 ps.fWeaponPosFrac=.5f;ps.weaponstate=3;ps.weaponDelay=100;w.bADSFire=1;
 step(&pm,20,0);assert(ps.fWeaponPosFrac>.5f);
 printf("PASS: %d guns, %d ADS schedules; retail enter/exit timing, cadence independence, reversals, variable steps and existing interruptions\n",
  (int)(sizeof(timings)/sizeof(timings[0])),cases);
}
'''
checks = 'static int timings[][4]={' + ','.join(rows) + '};\n' + checks
with tempfile.TemporaryDirectory(prefix='cod2-ads-') as directory:
    path = Path(directory)
    variants = [initialize,
                initialize.replace('iAdsTransInTime', 'iFireTime').replace('iAdsTransOutTime', 'iRechamberTime')]
    for index, variant in enumerate(variants):
        (path / 'test.c').write_text(support + variant + lerp + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr)
        if index == 0:
            print(result.stdout.strip())
# Pmove owns one integration before weapon logic; the movement dispatch test
# replays the actual function, including subdivision and a double-update mutant.
assert function(move_source, 'Pmove').count('PM_UpdateAimDownSightLerp(pm, &pml);') == 1
assert 'PM_UpdateAimDownSightLerp(' not in function(weapon_source, 'PM_Weapon')
(root / 'out/ads-transition-audit.json').write_text(json.dumps({'weapons': report}, indent=2)+'\n')
print('PASS: firing/rechamber-timing mutant rejected; ADS has one movement owner')
