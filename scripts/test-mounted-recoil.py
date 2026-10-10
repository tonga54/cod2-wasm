#!/usr/bin/env python3
"""Execute mounted recoil, firing gates and replicated view-angle updates."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
misc = (root / 'src/PC/game_mp/g_misc_mp.c').read_text()
client = (root / 'src/PC/game_mp/g_client_mp.c').read_text()


def function(source, name):
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef int qboolean;typedef float vec_t;typedef float vec3_t[3];
typedef struct {
 int pm_flags,eFlags,delta_angles[3],viewlocked,viewlocked_entNum;
 float viewangles[3],proneDirection,proneTorsoPitch;
} playerState_t;
typedef struct {playerState_t ps;int buttons;struct {struct {int angles[3];}cmd;}sess;} gclient_s;
typedef struct {int fireSndDelay,fireTime,flags;float arcmin[2],arcmax[2];}turretInfo_s;
typedef struct {int heat,lastTime;qboolean overheated;}turretHeat_t;
typedef struct {gclient_s *client;turretInfo_s *pTurretInfo;
 struct {int number,weapon,eFlags,time2;float angles2[3];}s;
 struct {float currentAngles[3];}r;}gentity_t;
typedef struct {int iFireTime;}WeaponDef;
static turretHeat_t turretHeat[32];static turretInfo_s turretInfo[32];
static struct {int time;}level;
enum {GMISC_PLAYERVIEWLOCK_FULL=1,GMISC_PLAYERVIEWLOCK_WEAPONJITTER=2,
 GMISC_EF_TELEPORT_BIT=2,GMISC_EF_FIRING=64,GMISC_BUTTON_ATTACK=1,
 GMISC_TURRET_HEAT_MAX=40000,GMISC_TURRET_SHOT_HEAT_SCALE=8,GMISC_TURRET_COOLING_PER_MS=5};
static WeaponDef weapon={50};static int shots,rngCalls;static float shotAngles[128][3];
static float rngValue=-1;static unsigned seed=1;
static float randomf(void){rngCalls++;if(rngValue>=0)return rngValue;
 seed=seed*1664525u+1013904223u;return (float)(seed>>8)/16777216.0f;}
static float AngleNormalize360(float a){a=fmodf(a,360);return a<0?a+360:a;}
static float AngleNormalize180(float a){a=AngleNormalize360(a);return a>180?a-360:a;}
static float AngleSubtract(float a,float b){return AngleNormalize180(a-b);}
static float AngleDelta(float a,float b){return AngleSubtract(a,b);}
static WeaponDef *BG_GetWeaponDef(int index){assert(index==42);return &weapon;}
static void G_PlayerTurretPositionAndBlend(gentity_t *player,gentity_t *gun){}
static void Fire_Lead(gentity_t *gun,gentity_t *player){
 assert(shots<128);memcpy(shotAngles[shots++],player->client->ps.viewangles,sizeof(vec3_t));}
'''
production = ''.join(function(misc, name) for name in
    ('GMisc_Clamp', 'turret_UpdateHeat', 'turret_clientaim'))
production += function(client, 'SetClientViewAngle')
production += ''.join(function(misc, name) for name in
    ('turret_ApplyRecoil', 'turret_shoot_internal', 'turret_track'))
checks = r'''
static void near(float a,float b){assert(fabsf(AngleSubtract(a,b))<.006f);}
static void reset(gentity_t *gun,gentity_t *player,gclient_s *c,int slot,float yaw){
 memset(c,0,sizeof(*c));memset(gun,0,sizeof(*gun));memset(player,0,sizeof(*player));
 memset(turretInfo,0,sizeof(turretInfo));memset(turretHeat,0,sizeof(turretHeat));
 shots=rngCalls=0;rngValue=-1;seed=1;level.time=0;weapon.iFireTime=50;
 gun->pTurretInfo=&turretInfo[slot];gun->s.number=100+slot;gun->s.weapon=42;
 gun->r.currentAngles[1]=yaw;gun->pTurretInfo->arcmin[0]=-80;gun->pTurretInfo->arcmax[0]=80;
 gun->pTurretInfo->arcmin[1]=-60;gun->pTurretInfo->arcmax[1]=60;
 player->client=c;c->buttons=1;c->ps.eFlags=0x300;c->ps.viewangles[1]=yaw;
 for(int i=0;i<3;i++)c->sess.cmd.angles[i]=(int)(c->ps.viewangles[i]*65536/360);
}
/* Reconstruct the network angle shorts as the next movement command does.
 * Unchanged raw mouse angles must not erase accumulated recoil. */
static void replay(gclient_s *c){
 for(int i=0;i<3;i++){
  float a=(float)(short)(c->sess.cmd.angles[i]+c->ps.delta_angles[i])*(360.0f/65536);
  near(a,c->ps.viewangles[i]);c->ps.viewangles[i]=a;
 }
}
int main(void){
 gentity_t gun,player;gclient_s c;int cases=0;
 for(int slot=0;slot<32;slot++)for(int yaw=0;yaw<360;yaw+=45){
  reset(&gun,&player,&c,slot,(float)yaw);rngValue=.5f;
  level.time+=50;turret_track(&gun,&player);
  assert(shots==1&&rngCalls==2&&c.ps.viewlocked==2);
  near(shotAngles[0][0],0);near(shotAngles[0][1],yaw);
  assert(c.ps.viewangles[0]<-.33f&&c.ps.viewangles[0]>-.35f);
  near(gun.s.angles2[0],c.ps.viewangles[0]);near(gun.s.angles2[1],0);
  replay(&c);float first=c.ps.viewangles[0];
  level.time+=50;turret_track(&gun,&player);assert(shots==2&&c.ps.viewangles[0]<first);
  near(shotAngles[1][0],first);replay(&c);
  /* Pulling the mouse down compensates the actual aim after recoil. */
  c.sess.cmd.angles[0]+=(int)(.34f*65536/360);
  float compensated=(float)(short)(c.sess.cmd.angles[0]+c.ps.delta_angles[0])*(360.0f/65536);
  assert(compensated>c.ps.viewangles[0]+.33f);c.ps.viewangles[0]=compensated;
  c.buttons=0;int before=shots,calls=rngCalls;vec3_t idle;memcpy(idle,c.ps.viewangles,sizeof(idle));
  for(int tick=0;tick<200;tick++){level.time+=50;turret_track(&gun,&player);}
  assert(shots==before&&rngCalls==calls);for(int i=0;i<3;i++)near(c.ps.viewangles[i],idle[i]);
  cases++;
 }
 /* Sustained fire climbs, wanders both ways, then cooling adds no recoil. */
 reset(&gun,&player,&c,0,0);int left=0,right=0;float minPitch=1,maxPitch=0;
 for(int tick=0;tick<100;tick++){
  vec3_t before;memcpy(before,c.ps.viewangles,sizeof(before));level.time+=50;turret_track(&gun,&player);
  float pitch=before[0]-c.ps.viewangles[0],yaw=AngleSubtract(c.ps.viewangles[1],before[1]);
  assert(pitch>=.279f&&pitch<=.52f);assert(fabsf(yaw)<=.181f);
  if(yaw<0)left++;else if(yaw>0)right++;
  minPitch=fminf(minPitch,pitch);maxPitch=fmaxf(maxPitch,pitch);replay(&c);
 }
 assert(left>20&&right>20&&maxPitch>minPitch+.1f&&c.ps.viewangles[0]<-35);
 assert(shots==100&&turretHeat[0].overheated&&rngCalls==200);
 vec3_t hot;memcpy(hot,c.ps.viewangles,sizeof(hot));
 for(int tick=0;tick<159;tick++){level.time+=50;turret_track(&gun,&player);assert(shots==100&&rngCalls==200);}
 for(int i=0;i<3;i++)near(c.ps.viewangles[i],hot[i]);
 level.time+=50;turret_track(&gun,&player);assert(shots==101&&rngCalls==202);
 /* Arc limits, wrapped base angles, roll, slow guns and non-player callers. */
 for(int extreme=0;extreme<2;extreme++){
  reset(&gun,&player,&c,31,350);rngValue=(float)extreme;
  gun.r.currentAngles[0]=10;c.ps.viewangles[0]=-70;c.ps.viewangles[1]=extreme?410:290;c.ps.viewangles[2]=3;
  turret_ApplyRecoil(&gun,&player);near(c.ps.viewangles[0],-70);
  near(c.ps.viewangles[1],extreme?410:290);near(c.ps.viewangles[2],3);
  near(gun.s.angles2[0],-80);near(gun.s.angles2[1],extreme?60:-60);replay(&c);
 }
 reset(&gun,&player,&c,0,0);weapon.iFireTime=100;rngValue=.5f;
 for(int tick=0;tick<40;tick++){level.time+=50;turret_track(&gun,&player);}
 assert(shots==20&&rngCalls==40);player.client=0;turret_shoot_internal(&gun,&player);assert(shots==20&&rngCalls==40);
 printf("PASS: %d mounted schedules; authoritative aim, next-shot direction, mouse compensation, yaw variation, arcs, idle/cooling and 50/100 ms cadence\n",cases);
 return 0;
}
'''


def run(source, name, should_pass):
    with tempfile.TemporaryDirectory(prefix='cod2-mounted-recoil-') as directory:
        p = Path(directory)
        (p / 'test.c').write_text(source)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(p / 'test.c'), '-lm', '-o', str(p / 'test')], check=True)
        result = subprocess.run([str(p / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == should_pass, (name, result.stdout, result.stderr)
        if should_pass:
            print(result.stdout.strip())


run(support + production + checks, 'production', True)
mutants = {
    'missing shot recoil': production.replace('        turret_ApplyRecoil(self, other);', ''),
    'camera-only angles': production.replace('        client->ps.delta_angles[i] = cmdAngle - client->sess.cmd.angles[i];', ''),
    'kick before shot': production.replace('        Fire_Lead(self, other);\n        turret_ApplyRecoil(self, other);',
                                           '        turret_ApplyRecoil(self, other);\n        Fire_Lead(self, other);'),
    'idle kick': production.replace('    turret_clientaim(self, other);\n    G_PlayerTurretPositionAndBlend',
                                     '    turret_ApplyRecoil(self, other);\n    turret_clientaim(self, other);\n    G_PlayerTurretPositionAndBlend'),
}
for name, mutant in mutants.items():
    assert mutant != production, name
    run(support + mutant + checks, name, False)
print('PASS: missing recoil, camera-only, pre-shot and idle-recoil mutants rejected')
