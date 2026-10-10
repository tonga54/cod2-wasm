#!/usr/bin/env python3
"""Replay the production movement/breath timer handoff and reject infinite holds."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
move = (root / 'src/PC/bgame/bg_pmove.c').read_text()
weapons = (root / 'src/PC/bgame/bg_weapons.c').read_text()
math = (root / 'src/PC/universal/com_math.c').read_text()
misc = (root / 'src/PC/bgame/bg_misc.c').read_text()


def function(source, name):
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


drop = function(move, 'PM_DropTimers')
breath = function(weapons, 'PM_UpdateHoldBreath')
track = function(math, 'DiffTrack')
defaults = {}
for name in ('hold_time', 'gasp_time', 'gasp_scale', 'hold_lerp', 'gasp_lerp'):
    match = re.search(r'Dvar_RegisterFloat\("player_breath_' + name + r'", ([\d.]+)f', misc)
    assert match, name
    defaults[name] = float(match[1])
assert defaults['hold_time'] == 4.5 and defaults['gasp_time'] == 1.0

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define PMF_JUMPING 0x80000
#define PM_WEAPON_FLAG_HOLDBREATH 0x4000
#define PM_WEAPON_BUTTON_HOLDBREATH 0x8000
typedef struct {int pm_flags,pm_time,legsTimer,torsoTimer,foliageSoundTime,
 damageTimer,damageDuration,holdBreathTimer,weapon;float fWeaponPosFrac,holdBreathScale;} playerState_t;
typedef struct {playerState_t *ps;struct {int buttons;} cmd;} pmove_t;
typedef struct {int msec;float frametime;} pml_t;
typedef struct {int overlayReticle,weapClass;} WeaponDef;
typedef struct {struct {float value;} current;} dvar_t;
static WeaponDef scoped={1,0},plain={0,0},excluded={1,9};
static WeaponDef *bg_weaponDefs[]={&plain,&scoped,&excluded};
static int BG_GetViewmodelWeaponIndex(playerState_t *ps){return ps->weapon;}
'''
for name, value in defaults.items():
    support += f'static dvar_t {name}={{{{{value}f}}}},*player_breath_{name}=&{name};\n'

checks = r'''
static playerState_t fresh(void){
 return (playerState_t){.weapon=1,.fWeaponPosFrac=1,.holdBreathScale=1};
}
static int holding(const playerState_t *ps){return !!(ps->pm_flags&PM_WEAPON_FLAG_HOLDBREATH);}
static void step(playerState_t *ps,int msec,int pressed){
 pmove_t pm={.ps=ps,.cmd={pressed?PM_WEAPON_BUTTON_HOLDBREATH:0}};
 pml_t pml={.msec=msec,.frametime=msec*.001f};
 /* Production Pmove drops movement timers before PM_Weapon updates breath. */
 PM_DropTimers(ps,msec);PM_UpdateHoldBreath(&pm,&pml);
 assert(isfinite(ps->holdBreathScale)&&ps->holdBreathScale>=0&&ps->holdBreathScale<=4.5f);
 assert(ps->holdBreathTimer>=0&&ps->holdBreathTimer<=5500);
}
int main(void){
 int schedules=0;
 for(int dt=1;dt<=66;dt++){
  playerState_t ps=fresh();int elapsed=0;
  while(elapsed<=4500){
   step(&ps,dt,1);elapsed+=dt;
   if(elapsed<=4500){assert(holding(&ps));assert(ps.holdBreathTimer==elapsed);}
  }
  assert(elapsed<=4500+dt&&!holding(&ps)&&ps.holdBreathTimer==5500);
  /* Holding Shift continuously cannot skip the exhausted recovery period. */
  int recovery=0;
  while(recovery<5500){
   step(&ps,dt,1);recovery+=dt;assert(!holding(&ps));
   assert(ps.holdBreathTimer==(recovery<5500?5500-recovery:0));
  }
  assert(ps.holdBreathScale>.9f);step(&ps,dt,1);
  assert(holding(&ps)&&ps.holdBreathTimer==dt);
  /* A short release and re-press recovers spent time instead of resetting it. */
  ps=fresh();step(&ps,1000,1);assert(holding(&ps)&&ps.holdBreathTimer==1000);
  step(&ps,dt,0);assert(!holding(&ps)&&ps.holdBreathTimer==1000-dt);
  recovery=dt;
  while(recovery<1000){step(&ps,dt,1);recovery+=dt;assert(!holding(&ps));}
  step(&ps,dt,1);assert(holding(&ps)&&ps.holdBreathTimer==dt);
  /* Lowering the scope, changing guns or partial ADS cannot bypass exhaustion. */
  for(int interrupted=0;interrupted<4;interrupted++){
   ps=fresh();step(&ps,4501,1);assert(!holding(&ps)&&ps.holdBreathTimer==5500);
   if(interrupted==0)ps.fWeaponPosFrac=0;
   else if(interrupted==1)ps.fWeaponPosFrac=.9f;
   else ps.weapon=interrupted==2?0:2;
   step(&ps,dt,1);assert(!holding(&ps)&&ps.holdBreathTimer==5500-dt);
   ps.weapon=1;ps.fWeaponPosFrac=1;step(&ps,dt,1);
   assert(!holding(&ps)&&ps.holdBreathTimer==5500-2*dt);
  }
  /* Unscoped, incomplete ADS and class-9 weapons never start a hold. */
  for(int ineligible=0;ineligible<3;ineligible++){
   ps=fresh();if(ineligible<2)ps.weapon=ineligible?2:0;else ps.fWeaponPosFrac=.99f;
   for(int n=0;n<200;n++){step(&ps,dt,1);assert(!holding(&ps)&&!ps.holdBreathTimer);}
  }
  schedules++;
 }
 /* Long holds with fluctuating command lengths must still consume real time. */
 playerState_t client=fresh(),server=client;int use=0,rest=0,exhaustions=0;
 for(int n=0;n<20000;n++){
  int ms=1+(n*17)%66,wasHolding=holding(&server);
  step(&client,ms,1);step(&server,ms,1);
  assert(client.pm_flags==server.pm_flags&&client.holdBreathTimer==server.holdBreathTimer);
  assert(client.holdBreathScale==server.holdBreathScale);
  if(holding(&server)){if(!wasHolding){use=0;assert(rest==0||rest>=5500);}use+=ms;assert(use<=4500);}
  else if(wasHolding){use+=ms;assert(use>4500&&use<=4566);exhaustions++;rest=0;}
  else rest+=ms;
 }
 assert(exhaustions>50);
 /* Releasing the scope returns the sway smoothly to the normal idle scale. */
 client=fresh();for(int n=0;n<200;n++)step(&client,16,1);
 assert(client.holdBreathScale<.01f);client.fWeaponPosFrac=0;
 for(int n=0;n<400;n++)step(&client,16,0);
 assert(!holding(&client)&&!client.holdBreathTimer&&fabsf(client.holdBreathScale-1)<.001f);
 hold_time.current.value=0;client=fresh();client.holdBreathTimer=3000;
 client.pm_flags=PM_WEAPON_FLAG_HOLDBREATH;step(&client,16,1);
 assert(!holding(&client)&&!client.holdBreathTimer&&client.holdBreathScale==1);
 printf("PASS: %d command schedules, finite 4.5-second holds, 5.5-second exhaustion recovery, early release, scope/weapon changes, sway and identical client/server replay\n",schedules);
}
'''

old_drop = drop[:-2] + '''
    if (ps->holdBreathTimer > 0) {
        ps->holdBreathTimer -= msec;
        if (ps->holdBreathTimer < 0) ps->holdBreathTimer = 0;
    }
}
'''
variants = [
    ('production', drop, breath),
    ('duplicate decrement', old_drop, breath),
    ('unlimited hold', drop, breath.replace('ps->holdBreathTimer > holdTime', '0')),
    ('no exhaustion recovery', drop, breath.replace('ps->holdBreathTimer = holdTime + gaspTime;', 'ps->holdBreathTimer = 0;')),
]
with tempfile.TemporaryDirectory(prefix='cod2-hold-breath-') as directory:
    path = Path(directory)
    for name, timers, hold in variants:
        (path / 'test.c').write_text(support + track + timers + hold + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (name == 'production'), (name, result.stdout, result.stderr)
        print(result.stdout.strip() if name == 'production' else f'PASS: {name} regression rejected')

pmove = function(move, 'Pmove')
assert pmove.count('PM_DropTimers(ps, msec);') == 1
assert pmove.index('PM_DropTimers(ps, msec);') < pmove.index('PM_Weapon(pm, &pml);')
assert function(weapons, 'PM_Weapon').count('PM_UpdateHoldBreath(pm, pml);') == 1
print('PASS: production dispatch has one breath-timer owner on both network sides')
