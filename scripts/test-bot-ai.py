#!/usr/bin/env python3
"""Run production bot navigation and combat decisions against a small world."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/server_mp/sv_bot_ai.inc').read_text()
source = source.replace('#include "cod2_sprint.h"', '#define BUTTON_SPRINT 2')
support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
typedef int qboolean; typedef unsigned char byte; typedef float vec_t; typedef float vec3_t[3];
#define COD2_BOT_MAX_CLIENTS 64
#define SESS_STATE_PLAYING 0
#define SESS_STATE_DEAD 1
#define TEAM_FREE 0
#define TEAM_AXIS 1
#define TEAM_ALLIES 2
typedef struct { float fraction; vec3_t normal; unsigned short entityNum; int startsolid,allsolid; } trace_t;
typedef struct { vec3_t origin,viewangles,velocity; float viewHeightCurrent; int pm_type,weapon,weaponTime,pm_flags,groundEntityNum,cursorHintEntIndex,ammoclip[128],ammo[128];char weaponslots[8]; } playerState_t;
typedef struct { playerState_t ps; vec3_t damage_from; int damage_fromWorld; struct {int connected,sessionState; struct {int team;}cs;}sess;} gclient_t;
typedef struct {gclient_t *client; int health; unsigned classname; struct {int inuse;vec3_t currentOrigin,absmin,absmax;}r;} gentity_t;
typedef struct { int maxclients,num_entities; } level_locals_t;
typedef struct {int iClipIndex,iAmmoIndex,bSemiAuto;} WeaponDef;
typedef struct {int buttons,hasWeapon,hasAngles,weapon; signed char forwardmove,rightmove; vec3_t angles;} cod2BotCommandState_t;
static struct {int time;}svs;
static cod2BotCommandState_t s_botCmdState[64];
level_locals_t level={3,3}; gentity_t g_entities[64];
static gclient_t clients[64]; static WeaponDef weapon={0,0,0},secondary={1,1,0};
static int traceCount, wall, smoke, friendly,stairs;
static int Dvar_GetInt(const char *name){(void)name;return 0;}
static playerState_t *SV_GameClientNum(int num){return &clients[num].ps;}
static int SV_BotIsTestClient(int num){return num>=0&&num<16;}
static void Com_Printf(const char *fmt,...){(void)fmt;}
const char *SL_ConvertToString(unsigned int value){(void)value;return "mp_tdm_spawn";}
WeaponDef *BG_GetWeaponDef(int index){return index==2?&secondary:&weapon;}
float SV_FX_GetVisibility(const vec_t *a,const vec_t *b){(void)a;(void)b;return smoke?0:1;}
qboolean SV_EntityContact(const vec_t *mins,const vec_t *maxs,const gentity_t *ent){
 for(int i=0;i<3;i++)if(mins[i]>=ent->r.absmax[i]||maxs[i]<=ent->r.absmin[i])return 0;
 return 1;
}
void G_TraceCapsule(trace_t *t,const vec_t *a,const vec_t *mi,const vec_t *ma,const vec_t *b,int pass,int mask){
 (void)mi;(void)ma;(void)pass;(void)mask;traceCount++;t->fraction=1;t->normal[2]=1;
 float floor=stairs?floorf(b[0]/16.0f)*12.0f:0;
 if(b[2]<floor&&a[2]>floor)t->fraction=(a[2]-floor)/(a[2]-b[2]);
 if(wall&&a[0]<256&&b[0]>=256){t->fraction=.5f;t->entityNum=1022;}
 if(friendly&&mask==0x02802831){t->fraction=.3f;t->entityNum=1;}
}
void G_LocationalTrace(trace_t *t,const vec_t *a,const vec_t *b,int pass,int mask,unsigned char *priority){
 (void)pass;(void)mask;(void)priority;traceCount++;t->fraction=1;
 if(wall&&a[0]<256&&b[0]>=256){t->fraction=.5f;t->entityNum=1022;}
}
'''
checks = r'''
int main(void){
 int i,fired[3]={0};vec3_t zero={0,0,1};
 for(i=0;i<3;i++){g_entities[i].client=&clients[i];g_entities[i].health=100;
  clients[i].sess.connected=2;clients[i].sess.cs.team=i?TEAM_AXIS:TEAM_ALLIES;
  clients[i].ps.origin[0]=i*400;clients[i].ps.origin[2]=1;clients[i].ps.viewHeightCurrent=60;
  clients[i].ps.weapon=1;clients[i].ps.ammoclip[0]=20;clients[i].ps.ammo[0]=100;}
 assert(!SV_BotEnableAI(17,1)&&!SV_BotEnableAI(0,3));
 assert(Bot_IsEnemy(0,1)&&!Bot_IsEnemy(0,0));clients[1].sess.cs.team=TEAM_ALLIES;
 assert(!Bot_IsEnemy(0,1));clients[1].sess.cs.team=TEAM_FREE;clients[0].sess.cs.team=TEAM_FREE;
 assert(Bot_IsEnemy(0,1));clients[0].sess.cs.team=TEAM_ALLIES;clients[1].sess.cs.team=TEAM_AXIS;
 wall=1;assert(!Bot_CanSee(0,1));wall=0;smoke=1;assert(!Bot_CanSee(0,1));smoke=0;
 assert(Bot_CanSee(0,1));clients[1].sess.sessionState=SESS_STATE_DEAD;assert(!Bot_IsEnemy(0,1));
 clients[1].sess.sessionState=SESS_STATE_PLAYING;
 // No corner teleports; wrap-around aim takes the shortest arc and is rate-limited.
 assert(fabsf(Bot_ApproachAngle(179,-179,1)-180)<.001f);
 assert(fabsf(Bot_ApproachAngle(0,90,7)-7)<.001f);
 // A legal first stair must not be rejected by looking across several risers.
 stairs=1;assert(Bot_CanMove(0,(float[3]){8,0,1},0,127,0));stairs=0;
 wall=1;assert(!Bot_CanMove(0,(float[3]){250,0,1},0,127,0));wall=0;
 SV_BotResetNavigation();s_botNavSeeded=1;Bot_NavAdd(zero);
 for(i=0;i<100;i++){svs.time=i*50;traceCount=0;Bot_NavFrame();assert(traceCount<=BOT_NAV_EDGES_PER_FRAME*9);}
 assert(s_botNavCount>100&&s_botNavCount<=BOT_NAV_MAX);
 while(s_botNavCount<BOT_NAV_MAX) {zero[0]+=64;Bot_NavAdd(zero);}
 assert(Bot_NavAdd((float[3]){999999,0,1})==-1&&s_botNavCount==BOT_NAV_MAX);
 // Spatial hashing distinguishes floors and signed grid coordinates.
 SV_BotResetNavigation();
 assert(Bot_NavAdd((float[3]){-32,-64,1})==0);
 assert(Bot_NavAdd((float[3]){-32,-64,65})==1);
 assert(Bot_NavFind((float[3]){-32,-64,2})==0);
 assert(Bot_NavFind((float[3]){-32,-64,66})==1);
 // A bomb's center must not select the roof above its usable volume. Its
 // valid lower floor may be more than 48 units below the brush origin.
 SV_BotResetNavigation();
 Bot_NavAdd((float[3]){0,0,65});Bot_NavAdd((float[3]){128,0,-32});
 memcpy(g_entities[5].r.absmin,(float[3]){-200,-200,-32},sizeof(vec3_t));
 memcpy(g_entities[5].r.absmax,(float[3]){200,200,40},sizeof(vec3_t));
 assert(Bot_NearestNode((float[3]){0,0,18},0)==0);
 assert(Bot_NearestNodeForGoal((float[3]){0,0,18},0,5)==1);
 SV_BotResetNavigation();
 // Fair perception: hard widens the view cone, not collision visibility.
 SV_BotEnableAI(0,0);clients[1].ps.origin[0]=400;clients[1].ps.origin[1]=1000;
 clients[2].sess.sessionState=SESS_STATE_DEAD;s_botAI[0].target=-1;
 assert(Bot_AcquireTarget(0,&s_botAI[0],&clients[0].ps)==-1);
 s_botAI[0].difficulty=2;assert(Bot_AcquireTarget(0,&s_botAI[0],&clients[0].ps)==1);
 clients[1].ps.origin[1]=0;clients[2].sess.sessionState=SESS_STATE_PLAYING;
 // A lost target freezes the last observed location; moving behind a wall
 // must not update it or permit a shot through the obstruction.
 Bot_Think(0,&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].lastSeen[0]==400);wall=1;clients[1].ps.origin[0]=800;
 svs.time+=200;Bot_Think(0,&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].target==-1&&s_botAI[0].lastSeen[0]==400);
 wall=0;clients[1].ps.origin[0]=400;
 // Reaction time changes across difficulties, with the same native weapon rules.
 for(i=0;i<3;i++){
  int time;SV_BotResetNavigation();s_botNavSeeded=1;svs.time=0;SV_BotEnableAI(0,i);
  for(time=0;time<=1400;time+=50){svs.time=time;SV_BotRunAI(0,&clients[0].ps);
   if(s_botCmdState[0].buttons&1){fired[i]=time;break;}}
  assert(fired[i]>0);
 }
 assert(fired[2]<fired[1]&&fired[1]<fired[0]);
 clients[0].ps.ammoclip[0]=0;svs.time+=50;SV_BotRunAI(0,&clients[0].ps);
 assert((s_botCmdState[0].buttons&0x10)&&!(s_botCmdState[0].buttons&1));
 clients[0].ps.ammo[0]=0;clients[0].ps.ammoclip[1]=5;clients[0].ps.weaponslots[1]=2;
 svs.time+=50;SV_BotRunAI(0,&clients[0].ps);
 assert(s_botCmdState[0].hasWeapon&&s_botCmdState[0].weapon==2);
 clients[0].ps.ammo[0]=100;
 clients[0].ps.ammoclip[0]=20;weapon.bSemiAuto=1;clients[0].ps.weaponTime=150;
 svs.time+=50;SV_BotRunAI(0,&clients[0].ps);assert(!(s_botCmdState[0].buttons&1));
 clients[0].sess.sessionState=SESS_STATE_DEAD;svs.time+=50;SV_BotRunAI(0,&clients[0].ps);
 assert(s_botCmdState[0].buttons==8&&!s_botCmdState[0].forwardmove&&!s_botCmdState[0].rightmove);
 // Stuck detection reads the previous movement intent even though this frame's
 // command has already been cleared. Dead/stationary bots do not trigger it.
 clients[0].sess.sessionState=SESS_STATE_PLAYING;
 svs.time=2000; s_botAI[0].lastMoveCheck=0; s_botAI[0].wantsMove=1;
 memcpy(s_botAI[0].lastOrigin,clients[0].ps.origin,sizeof(vec3_t));
 s_botCmdState[0].forwardmove=s_botCmdState[0].rightmove=0;
 Bot_Think(0,&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].avoidUntil==2600 && s_botAI[0].avoidSide!=0);
 // Objective priority and original input interaction: the bot stops inside
 // the capture radius, holds only the selected use hint, and expires its goal.
 SV_BotResetNavigation();svs.time=3000;SV_BotEnableAI(0,1);
 assert(!SV_BotSetGoal(0,NULL,12,0,-1));
 assert(!SV_BotSetGoal(0,(float[3]){NAN,0,0},12,0,-1));
 assert(!SV_BotSetGoal(0,clients[0].ps.origin,4,0,-1));
 assert(SV_BotSetGoal(0,clients[0].ps.origin,48,0,-1));
 svs.time+=50;SV_BotRunAI(0,&clients[0].ps);
 assert(!s_botCmdState[0].forwardmove&&!s_botCmdState[0].rightmove);
 g_entities[5].r.inuse=1;g_entities[5].classname=55;
 assert(SV_BotSetGoal(0,clients[0].ps.origin,48,2,5));
 clients[0].ps.cursorHintEntIndex=6;svs.time+=50;SV_BotRunAI(0,&clients[0].ps);
 assert(!(s_botCmdState[0].buttons&8));
 clients[0].ps.cursorHintEntIndex=5;clients[0].ps.groundEntityNum=1023;
 svs.time+=50;SV_BotRunAI(0,&clients[0].ps);assert(!(s_botCmdState[0].buttons&8));
 clients[0].ps.groundEntityNum=1022;svs.time+=50;SV_BotRunAI(0,&clients[0].ps);
 assert(s_botCmdState[0].buttons==8&&!s_botCmdState[0].forwardmove&&!s_botCmdState[0].rightmove);
 clients[0].ps.cursorHintEntIndex=1023;clients[0].ps.groundEntityNum=1023;
 svs.time+=50;SV_BotRunAI(0,&clients[0].ps);
 assert(s_botCmdState[0].buttons==8); // Keep the original hold timer running.
 g_entities[5].classname=56;svs.time+=50;SV_BotRunAI(0,&clients[0].ps);
 assert(!(s_botCmdState[0].buttons&8)); // Deleted/reused entity cannot be used.
 svs.time+=1700;SV_BotRunAI(0,&clients[0].ps);assert(!(s_botCmdState[0].buttons&8));
 assert(SV_BotClearGoal(0)&&!s_botAI[0].objectiveUntil);
 clients[1].sess.cs.team=TEAM_ALLIES;friendly=1;
 assert(Bot_FriendlyInAim(0,&s_botAI[0],&clients[0].ps));friendly=0;
 SV_BotResetAIClient(0);assert(!s_botAI[0].enabled);
 puts("PASS: bounded/hash navigation, enemy teams, fair perception/memory, difficulty, friendly-fire avoidance, objective priority/expiry, original use holds, reload/cadence, death/reset");
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-bot-ai-') as directory:
    path = Path(directory)
    (path/'test.c').write_text(support + source + checks)
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-O2',str(path/'test.c'),'-lm','-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True,timeout=10)
