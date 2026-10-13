#!/usr/bin/env python3
"""Run production weapon recovery, held jump, landing and blood overlay paths."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
def function(path, name):
    source = (root / path).read_text()
    m = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert m, name
    end, depth = m.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[m.start():end] + '\n'

def run(name, source):
    with tempfile.TemporaryDirectory(prefix='cod2-' + name) as d:
        p = Path(d)
        (p/'test.c').write_text(source)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        '-fno-sanitize-recover=all', str(p/'test.c'), '-lm', '-o', str(p/'test')], check=True)
        subprocess.run([str(p/'test')], check=True)
    print('PASS:', name, flush=True)

weapons = 'src/PC/bgame/bg_weapons.c'
constants = (root/weapons).read_text()
constants = constants[constants.index('#define WEAPON_READY'):constants.index('static qboolean PM_WeaponHasInventoryWeapon')]
run('respawn-weapon-selection', r'''
#include <assert.h>
#include <string.h>
#define BM_NOINLINE
#define __attribute_regparm__(n)
#define WEAPTYPE_BINOCULARS 3
typedef int qboolean;
typedef struct {int weapon,weaponstate,weaponTime,weaponDelay,pm_type,pm_flags,weapAnim,
 weapons[4],ammoclip[128],grenadeTimeLeft;char weaponslots[8];float aimSpreadScale;} playerState_t;
typedef struct {playerState_t *ps;struct {int weapon;}cmd;}pmove_t;
typedef struct {int offhandClass,weapType,iAltWeaponIndex,iRaiseTime,iAltRaiseTime,
 iDropTime,iAltDropTime,bClipOnly,iClipIndex,weapSlot,playerAnimType,weapClass;}WeaponDef;
static WeaponDef defs[128],*bg_weaponDefs[128];static int bg_iNumWeapons=127;
static void PM_SetProneMovementOverride(playerState_t *ps){}
static void PM_AddEvent(playerState_t *ps,int event){}
static void BG_AnimScriptEvent(playerState_t *ps,int a,int b,int c){}
static void BG_UpdateConditionValue(int a,int b,int c,int d){}
static int BG_IsPlayerWeaponInSlot_core(playerState_t *ps,int w,int unused){return 1;}
static void PM_UpdateWeaponConditions(playerState_t *ps,WeaponDef *def){}
static void PM_ResetWeaponState(playerState_t *ps){ps->weaponstate=0;ps->weaponTime=ps->weaponDelay=0;}
''' + constants + ''.join(function(weapons,n) for n in (
    'PM_WeaponHasInventoryWeapon','PM_WeaponSetAnim','PM_WeaponClearAnim','PM_WeaponSetIdle',
    'PM_WeaponStateIsBinoculars','PM_WeaponStateIsOffhand','PM_WeaponStateIsReloading',
    'PM_WeaponStateBlocksChange','PM_FindFirstSelectableWeapon','PM_BeginWeaponChange',
    'PM_ResolveWeaponSelection','PM_FinishWeaponChange','PM_ShouldChangeWeapon','PM_CheckWeaponChange')) + r'''
int main(void){
 for(int w=0;w<128;w++){bg_weaponDefs[w]=&defs[w];defs[w].iRaiseTime=300;defs[w].iDropTime=200;}
 defs[1].weapType=3;defs[2].offhandClass=1;
 for(int stale=0;stale<128;stale++)for(int slot=1;slot<=2;slot++)
 for(int state=WEAPON_READY;state<=WEAPON_RELOAD_END;state++){
  playerState_t ps={.weapons={6,0,0,0},.weaponstate=state,.weaponTime=200,.weaponDelay=100};pmove_t pm={.ps=&ps,.cmd={stale}};
  ps.weapons[1]=1u<<31;ps.weaponslots[slot]=63;
  PM_CheckWeaponChange(&pm);assert(ps.weaponstate==WEAPON_DROPPING);
  PM_FinishWeaponChange(&pm);assert(ps.weapon==63&&ps.weaponstate==WEAPON_RAISING&&ps.weaponTime==300);
  PM_WeaponSetIdle(&ps);PM_CheckWeaponChange(&pm);assert(ps.weapon==63&&ps.weaponstate==WEAPON_READY);
  // A weapon removed during the lowering animation must resolve again.
  ps.weaponstate=WEAPON_DROPPING;ps.weapons[1]=0;ps.weapons[2]=1;ps.weaponslots[slot]=64;
  PM_FinishWeaponChange(&pm);assert(ps.weapon==64&&ps.weaponstate==WEAPON_RAISING);
 }
 playerState_t ps={.weapon=10,.weapons={1u<<10}};pmove_t pm={.ps=&ps,.cmd={11}};
 ps.weapons[0]|=1u<<11;PM_CheckWeaponChange(&pm);assert(ps.weaponstate==WEAPON_DROPPING&&ps.weaponTime==200);
 ps.weaponTime=0;PM_FinishWeaponChange(&pm);assert(ps.weapon==11&&ps.weaponTime==300);
 // Invalid stale requests do not switch an existing gun to the first slot.
 pm.cmd.weapon=99;ps.weaponstate=WEAPON_READY;ps.weaponslots[1]=10;
 PM_CheckWeaponChange(&pm);assert(ps.weapon==11&&ps.weaponstate==WEAPON_READY);
 // Reloads cancel for a weapon change; other actions, including a live
 // offhand grenade, still block changes.
 for(int state=WEAPON_FIRING;state<=WEAPON_OFFHAND_END;state++){
  ps.weapon=11;ps.weaponstate=state;pm.cmd.weapon=10;
  PM_CheckWeaponChange(&pm);
  assert(ps.weaponstate==(PM_WeaponStateIsReloading(state)?WEAPON_DROPPING:state)&&ps.weapon==11);
 }
 ps.weapon=0;ps.weaponstate=WEAPON_OFFHAND_HOLD;PM_CheckWeaponChange(&pm);
 assert(ps.weaponstate==WEAPON_OFFHAND_HOLD);
 // Empty/spectator inventory must stay empty, including binoculars/grenades only.
 ps=(playerState_t){.weapons={6}};PM_CheckWeaponChange(&pm);assert(!ps.weapon&&!ps.weaponstate);
 return 0;
}
''')

run('held-jump-no-repeat', r'''
#include <assert.h>
#include <math.h>
#include <stdlib.h>
typedef int Bool;typedef float vec_t,vec3_t[3];
#define qfalse 0
#define qtrue 1
typedef struct {struct {float value;int enabled;}current;}dvar_t;
static dvar_t height={{39,1}},slow={{0,1}},spread={{64,1}},push={{128,1}};
static dvar_t *jump_height=&height,*jump_slowdownEnable=&slow,*jump_spreadAdd=&spread,*jump_ladderPushVel=&push;
typedef struct {int jumpTime,pm_flags,pm_type,pm_time,gravity,groundEntityNum;vec3_t origin,velocity,vLadderVec;float jumpOriginZ,aimSpreadScale;}playerState_t;
typedef struct {int buttons,serverTime,forwardmove;}usercmd_t;
typedef struct {playerState_t *ps;usercmd_t cmd,oldcmd;}pmove_t;
typedef struct {int walking,groundPlane,almostGroundPlane;vec3_t forward;}pml_t;
static int PM_GetEffectiveStance(playerState_t *ps){return 0;}
static int PM_GroundSurfaceType(pml_t *pml){return 1;}
static void PM_AddEvent(playerState_t *ps,int e){}
static void BG_AnimScriptEvent(playerState_t *ps,int a,int b,int c){}
static void Vec3Normalize(float *v){}
''' + function('src/PC/bgame/bg_jump.c','Jump_Check') + r'''
int main(void){
 for(int dt=1;dt<=66;dt++){
  playerState_t ps={.gravity=800};pmove_t pm={.ps=&ps};pml_t pml={0};int jumps=0;
  for(int t=1000;t<5000;t+=dt){
   pm.cmd.buttons=0x400;pm.cmd.serverTime=t;
   jumps+=Jump_Check(&pm,&pml);pm.oldcmd=pm.cmd;
  }
  assert(jumps==1);pm.cmd.buttons=0;pm.cmd.serverTime+=dt;Jump_Check(&pm,&pml);pm.oldcmd=pm.cmd;
  pm.cmd.buttons=0x400;pm.cmd.serverTime+=dt;assert(Jump_Check(&pm,&pml));
 }
 return 0;
}
''')

move = 'src/PC/bgame/bg_pmove.c'
source=(root/move).read_text()
deltas=source[source.index('static const vec3_t CorrectSolidDeltas'):source.index('};',source.index('static const vec3_t CorrectSolidDeltas'))+2]
run('fall-height-and-single-landing', r'''
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define PM_REGPARM2_ABI
#define ENTITYNUM_NONE 1023
typedef int qboolean;typedef float vec3_t[3];
typedef struct {float fraction;vec3_t normal;int allsolid,startsolid,entityNum,surfaceFlags;}trace_t;
typedef struct {int eFlags,clientNum,groundEntityNum,pm_flags,pm_type,gravity,pm_time;vec3_t origin,velocity;}playerState_t;
typedef struct {playerState_t *ps;vec3_t mins,maxs;int tracemask,numtouch,touchents[32];struct {int forwardmove;}cmd;}pmove_t;
typedef struct {trace_t groundTrace;int walking,groundPlane,almostGroundPlane;vec3_t previous_origin,previous_velocity;}pml_t;
typedef struct {struct {float value;}current;}dvar_t;
static dvar_t minHeight={{256}},maxHeight={{480}},*bg_fallDamageMinHeight=&minHeight,*bg_fallDamageMaxHeight=&maxHeight;
static int events,lastEvent,lastDamage,surface,embedded,traces;
static void PM_playerTrace(pmove_t *pm,trace_t *t,const float *start,const float *mins,const float *maxs,const float *end,int client,int mask){
 if(embedded && traces++==0){*t=(trace_t){.allsolid=1,.startsolid=1};return;}
 *t=(trace_t){.fraction=.5f,.normal={0,0,1},.entityNum=1022,.surfaceFlags=surface};
}
static void Jump_ClearState(playerState_t *ps){}
static void BG_AnimScriptEvent(playerState_t *ps,int a,int b,int c){}
static void Com_Printf(const char *s){}
static void BG_AddPredictableEventToPlayerstate(int event,int damage,playerState_t *ps){events++;lastEvent=event;lastDamage=damage;}
''' + deltas + function(move,'PM_GroundTrace') + r'''
int main(void){
 for(int height=1;height<=600;height++)for(int dt=1;dt<=66;dt++){
  float speed=sqrtf(1600.f*height),t=dt*.001f;
  float prior=-speed+800*t; if(prior>=0)continue;
  playerState_t ps={.groundEntityNum=1023,.gravity=800};pmove_t pm={.ps=&ps};
  pml_t pml={.previous_origin={0,0,speed*t-400*t*t},.previous_velocity={0,0,prior}};
  events=0;PM_GroundTrace(&pm,&pml);
  if(height>258){assert(events==1&&lastEvent==116);int want=height>=480?100:(height-256)*100/224;assert(abs(lastDamage-want)<=1);}
  if(height<=256)assert(!events||lastEvent<116);
  int old=events;PM_GroundTrace(&pm,&pml);assert(events==old); // Resting contact isn't a second fall.
 }
 // Walking/jumping upward onto a step must not become a falling impact.
 playerState_t ps={.groundEntityNum=1023,.gravity=800};pmove_t pm={.ps=&ps};
 pml_t pml={.previous_velocity={0,0,1000}};events=0;PM_GroundTrace(&pm,&pml);assert(!events);
 ps.groundEntityNum=1023;ps.gravity=0;events=0;PM_GroundTrace(&pm,&pml);assert(!events);
 // Authored no-fall-damage surfaces remain safe.
 ps.groundEntityNum=1023;ps.gravity=800;pml.previous_velocity[2]=-1000;surface=1;events=0;
 PM_GroundTrace(&pm,&pml);assert(lastEvent<116);
 // Escaping a small overlap must not kill the player from stale velocity.
 surface=0;embedded=1;traces=events=0;ps.groundEntityNum=1023;
 PM_GroundTrace(&pm,&pml);assert(!events&&ps.groundEntityNum==1022);
 return 0;
}
''')

# Execute the production server event branch, with damage delivery captured.
active=(root/'src/PC/game_mp/g_active_mp.c').read_text()
branch=active[active.index('        if ((unsigned)(event - 0x74)'):active.index('        if ((unsigned)(event - 0x9e)')]
run('fall-damage-independent-of-health', r'''
#include <assert.h>
#include <stddef.h>
static struct {struct {int stats[6];}ps;}clientState,*client=&clientState;
static struct {struct {int eType;}s;}entity,*ent=&entity;
static int applied;
static void G_Damage(void *a,void*b,void*c,void*d,void*e,int damage,int flags,int mod,int weapon,int hit){assert(mod==11);applied+=damage;}
static void apply(int event,int eventParm){for(int i=0;i<1;i++){
''' + branch + r'''
}}
int main(void){ent->s.eType=1;client->ps.stats[2]=100;
 for(int health=1;health<=100;health++)for(int event=116;event<=138;event++){
  client->ps.stats[0]=health;applied=0;apply(event,50);assert(applied==50);
  applied=0;apply(event,100);assert(applied>=health);
  applied=0;apply(event,0);assert(!applied);
 }
 return 0;}
''')

run('blood-flash-fade-and-reset', r'''
#include <assert.h>
#include <math.h>
#include <stdlib.h>
typedef float vec_t;typedef int MaterialHandle,rectDef_t;
typedef struct {struct {int enabled;float value;}current;}dvar_t;
static dvar_t blood={.current.enabled=1},end={0},*cg_blood=&blood,*hud_healthOverlay_phaseEnd_toAlpha=&end;
static struct {int healthOverlayHurt,healthOverlayPulseDuration,healthOverlayPulsePhase,healthOverlayPulseIndex,
 healthOverlayPulseTime,time,damageTime;float healthOverlayOldHealth,healthOverlayToAlpha,healthOverlayFromAlpha,damageValue;}state,*cg=&state;
static int draws;static float drawn;
static float CG_CalcPlayerHealth(void){return .9f;}
static void CG_PulseLowHealthOverlay(float health){}
static void CG_CopyColor(float*out,float*in,float alpha){out[3]=alpha;}
static void CG_DrawHudPic(const rectDef_t*r,int mat,float*color){draws++;drawn=color[3];}
''' + function('src/PC/cgame_mp/cg_newDraw_mp.c','CG_ResetLowHealthOverlay') + function('src/PC/cgame_mp/cg_newDraw_mp.c','CG_DrawLowHealthOverlay') + r'''
int main(void){
 rectDef_t rect=0;float color[4]={1,1,1,1};cg->damageTime=1000;cg->damageValue=.3f;
 cg->time=1000;CG_DrawLowHealthOverlay(&rect,1,color);assert(draws==1&&fabsf(drawn-.3f)<.001f);
 cg->time=1325;CG_DrawLowHealthOverlay(&rect,1,color);assert(draws==2&&fabsf(drawn-.15f)<.001f);
 cg->time=1650;CG_DrawLowHealthOverlay(&rect,1,color);assert(draws==2);
 cg->time=1100;blood.current.enabled=0;CG_DrawLowHealthOverlay(&rect,1,color);assert(draws==2);
 blood.current.enabled=1;CG_ResetLowHealthOverlay();CG_DrawLowHealthOverlay(&rect,1,color);assert(draws==2);
 cg->healthOverlayToAlpha=.8f;CG_DrawLowHealthOverlay(&rect,1,color);assert(draws==3&&drawn==.8f);
 return 0;
}
''')
