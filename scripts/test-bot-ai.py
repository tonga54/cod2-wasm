#!/usr/bin/env python3
"""Run production bot navigation and combat decisions against a small world."""
from pathlib import Path
import subprocess
import argparse
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize', action='store_true')
parser.add_argument('--benchmark', action='store_true')
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/server_mp/sv_bot_ai.inc').read_text()
source = source.replace('#include "cod2_sprint.h"', '#define BUTTON_SPRINT 2')
support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
typedef int qboolean; typedef unsigned char byte; typedef float vec_t; typedef float vec3_t[3];
#define COD2_BOT_MAX_CLIENTS 64
#define SESS_STATE_PLAYING 0
#define SESS_STATE_DEAD 1
#define SESS_STATE_SPECTATOR 2
#define TEAM_FREE 0
#define TEAM_AXIS 1
#define TEAM_ALLIES 2
#define TEAM_SPECTATOR 3
#define WEAPCLASS_SMG 2
#define WEAPCLASS_MG 1
typedef struct { float fraction; vec3_t normal; unsigned short entityNum; int startsolid,allsolid,surfaceFlags,contents; } trace_t;
typedef struct { vec3_t origin,viewangles,velocity; float viewHeightCurrent,fWeaponPosFrac; int pm_type,weapon,weaponTime,pm_flags,groundEntityNum,cursorHintEntIndex,ammoclip[128],ammo[128],stats[6];char weaponslots[8]; } playerState_t;
typedef struct { playerState_t ps; vec3_t damage_from; int damage_fromWorld, bFrozen; struct {int connected,sessionState; struct {int team;}cs;}sess;} gclient_t;
typedef struct {gclient_t *client; int health; unsigned classname; struct {int inuse;vec3_t currentOrigin,absmin,absmax;}r;} gentity_t;
typedef struct { int maxclients,num_entities; } level_locals_t;
typedef struct {const char *szInternalName; int iClipIndex,iAmmoIndex,bSemiAuto,overlayReticle,weapClass,iClipSize;} WeaponDef;
typedef struct {int buttons,hasWeapon,hasAngles,weapon; signed char forwardmove,rightmove; vec3_t angles;} cod2BotCommandState_t;
static struct {int time;}svs;
static cod2BotCommandState_t s_botCmdState[64];
level_locals_t level={3,3}; gentity_t g_entities[64];
static gclient_t clients[64]; static WeaponDef weapon={"test",0,0,0,0,0,30},secondary={"secondary",1,1,0,0,0,10};
static int humanClient=-1;
static int traceCount, wall, wallDoor, smoke, friendly,crowd,stairs,ladderWorld,navClock,dropWorld,tacticalWall;
static int teammateBlocker=-1,perchWorld,courtesyWall;
static int slowBackground,traceMs,pedestalWorld,contactCount;
static int hiddenNear;
static float fallMin=256;
int Sys_Milliseconds(void){return slowBackground?traceMs:navClock?navClock++:0;}
static int Dvar_GetInt(const char *name){(void)name;return 0;}
static playerState_t *SV_GameClientNum(int num){return &clients[num].ps;}
static int SV_BotIsTestClient(int num){return num>=0&&num<level.maxclients&&num!=humanClient;}
static void Com_Printf(const char *fmt,...){(void)fmt;}
const char *SL_ConvertToString(unsigned int value){(void)value;return "mp_tdm_spawn";}
WeaponDef *BG_GetWeaponDef(int index){return index==2?&secondary:&weapon;}
float SV_FX_GetVisibility(const vec_t *a,const vec_t *b){(void)a;(void)b;return smoke?0:1;}
qboolean SV_EntityContact(const vec_t *mins,const vec_t *maxs,const gentity_t *ent){
 contactCount++;
 for(int i=0;i<3;i++)if(mins[i]>=ent->r.absmax[i]||maxs[i]<=ent->r.absmin[i])return 0;
 return 1;
}
void G_TraceCapsule(trace_t *t,const vec_t *a,const vec_t *mi,const vec_t *ma,const vec_t *b,int pass,int mask){
 (void)mi;(void)ma;(void)pass;(void)mask;traceCount++;t->fraction=1;t->normal[2]=1;t->startsolid=t->allsolid=t->contents=0;
 if(hiddenNear && mask==0x02802831 && b[0]==128){t->fraction=.5f;t->entityNum=1022;return;}
 if(slowBackground && a[0]<-1024)traceMs+=4;
 if(courtesyWall && mi[0]<0 && mask==0x02810011 && b[1]<a[1]){t->fraction=.5f;t->entityNum=1022;return;}
 if(perchWorld && a[0]<256 && a[2]==41 && a[2]==b[2]){t->fraction=b[0]>a[0]?1:.08f;return;}
 if(perchWorld && a[0]<256 && a[2]==19 && b[0]>a[0]){t->fraction=.25f;return;}
 if(tacticalWall && a[1]<16 && b[0]>a[0]+256 && a[2]>40){t->fraction=.05f;return;}
 if(teammateBlocker>=0 && mi[0]<0 && (mask&0x02000000) && b[0]>a[0]){t->fraction=.2f;t->entityNum=teammateBlocker;return;}
 if(crowd && (mask&0x02000000)){t->startsolid=1;t->contents=0x02000000;t->fraction=0;return;}
 float floor=stairs?floorf(b[0]/16.0f)*12.0f:ladderWorld&&b[0]>=32?152:0;
 if(pedestalWorld && fabsf(b[0]-128)<8 && fabsf(b[1])<8)floor=36;
 if(dropWorld)floor=b[0]<32?160:dropWorld==2?-200:0;
 if((dropWorld==3||dropWorld==4)&&a[0]<32&&b[0]>=32&&a[2]<192&&b[2]>=a[2]){
  t->fraction=.5f;t->normal[0]=-1;t->normal[2]=0;return;
 }
 if(dropWorld==4&&ma[2]>=70&&(a[2]+ma[2]>240||b[2]+ma[2]>240))t->fraction=.5f;
 if(ladderWorld&&mask==0x10000&&a[0]<32&&b[0]>=32&&a[2]<140){
  t->fraction=.8f;t->surfaceFlags=8;t->normal[0]=-1;t->normal[2]=0;return;
 }
 if(b[2]<floor&&a[2]>floor)t->fraction=(a[2]-floor)/(a[2]-b[2]);
 if(wall&&a[0]<256&&b[0]>=256&&(!wallDoor||(wallDoor==1&&b[1]<96)||
    (wallDoor==2&&fabsf(b[1]-16)>4))){t->fraction=.5f;t->entityNum=1022;}
 if(friendly&&mask==0x02802831){t->fraction=.3f;t->entityNum=1;}
}
void G_LocationalTrace(trace_t *t,const vec_t *a,const vec_t *b,int pass,int mask,unsigned char *priority){
 (void)pass;(void)mask;(void)priority;traceCount++;t->fraction=1;
 if(wall&&a[0]<256&&b[0]>=256&&(!wallDoor||b[1]<96)){t->fraction=.5f;t->entityNum=1022;}
}
'''
checks = r'''
// Observe actual generated attack/aim commands against a stationary visible
// target, including the strongest case: a sniper with ADS already complete.
static int combatSample(int difficulty,int scoped,float range,unsigned int seed,float *yaw) {
 SV_BotResetNavigation();memset(clients,0,sizeof(clients));
 memset(g_entities,0,sizeof(g_entities));level.maxclients=2;
 for(int c=0;c<2;c++) {
  g_entities[c].client=&clients[c];g_entities[c].health=100;
  clients[c].sess.connected=2;clients[c].sess.cs.team=c?TEAM_AXIS:TEAM_ALLIES;
  clients[c].ps.origin[2]=1;clients[c].ps.viewHeightCurrent=60;
 }
 clients[1].ps.origin[0]=range;
 clients[0].ps.weapon=1;clients[0].ps.ammoclip[0]=20;
 clients[0].ps.fWeaponPosFrac=1;clients[0].ps.groundEntityNum=1022;
 weapon.overlayReticle=scoped;weapon.bSemiAuto=scoped;
 svs.time=1000;SV_BotEnableAI(0,difficulty);s_botAI[0].random=seed;
 for(int elapsed=0;elapsed<=3000;elapsed+=50) {
  svs.time=1000+elapsed;s_botNavFrame=svs.time;
  SV_BotSetGoal(0,clients[0].ps.origin,32,0,-1);
  SV_BotRunAI(0,&clients[0].ps);
  if(s_botCmdState[0].buttons&1) {*yaw=s_botCmdState[0].angles[1];return elapsed;}
 }
 assert(!"visible target never received a shot");return -1;
}

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
 // Player obstruction identity comes from the existing movement sweep and
 // cannot leak into the next successful step.
 teammateBlocker=1;assert(!Bot_CanMove(0,(float[3]){0,0,1},0,127,0) && s_botMoveBlocker==1);
 teammateBlocker=-1;assert(Bot_CanMove(0,(float[3]){0,0,1},0,127,0) && s_botMoveBlocker==-1);
 // Crowded-spawn overlap uses the same player-body retry as native pmove;
 // the retry retains world collision rather than walking through a wall.
 crowd=1;assert(Bot_CanMove(0,(float[3]){8,0,1},0,127,0));
 wall=1;assert(!Bot_CanMove(0,(float[3]){250,0,1},0,127,0));wall=0;crowd=0;
 SV_BotResetNavigation();s_botNavSeeded=1;Bot_NavAdd(zero);
 for(i=0;i<100;i++){svs.time=i*50;traceCount=0;Bot_NavFrame();assert(traceCount<=BOT_NAV_EDGES_PER_FRAME*15);}
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
 assert(s_botAI[0].avoidUntil==2400 && s_botAI[0].avoidSide!=0);
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

 // Cached routes must keep making forward progress between grid centers;
 // the old nearest-cell recentering sent bots backwards on every replan.
 SV_BotResetNavigation();s_botNavSeeded=1;s_botNavFrame=0;
 for(i=0;i<8;i++) {
  int n=Bot_NavAdd((float[3]){i*32,0,1});
  if(i){s_botNav[n-1].edge[0]=n;s_botNav[n].edge[4]=n-1;}
 }
 svs.time=0;SV_BotEnableAI(0,1);clients[0].ps.origin[0]=17;
 assert(SV_BotSetGoal(0,(float[3]){224,0,1},12,0,-1));
 Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].waypoint>=0 && s_botNav[s_botAI[0].waypoint].origin[0]>17);
 traceCount=0;Bot_AdvanceRoute(&s_botAI[0],&clients[0].ps);assert(traceCount<=1);
 for(i=1;i<=6;i++) {
  clients[0].ps.origin[0]=17+i*30;
  Bot_AdvanceRoute(&s_botAI[0],&clients[0].ps);
  if(s_botAI[0].waypoint>=0) assert(s_botNav[s_botAI[0].waypoint].origin[0]>clients[0].ps.origin[0]-18);
 }
 // A blocked edge must choose another route rather than repeat its loop.
 SV_BotResetNavigation();
 Bot_NavAdd((float[3]){0,0,1});Bot_NavAdd((float[3]){32,0,1});
 Bot_NavAdd((float[3]){32,32,1});Bot_NavAdd((float[3]){64,0,1});
 s_botNav[0].edge[0]=1;s_botNav[0].edge[1]=2;
 s_botNav[1].edge[0]=3;s_botNav[2].edge[7]=3;
 clients[0].ps.origin[0]=clients[0].ps.origin[1]=0;
 svs.time=200;SV_BotEnableAI(0,1);SV_BotSetGoal(0,(float[3]){64,0,1},12,0,-1);
 s_botAI[0].random=8;Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].route[0]==1);Bot_BlockRoute(&s_botAI[0]);
 assert(Bot_EdgeBlocked(&s_botAI[0],0,1));
 Bot_FindPath(&s_botAI[0],&clients[0].ps);assert(s_botAI[0].route[0]==2);
 svs.time+=8001;assert(!Bot_EdgeBlocked(&s_botAI[0],0,1));

 // A remote isolated spawn may scout a route before the global frontier reaches
 // it. It must not wait motionless for complete arena navigation.
 SV_BotResetNavigation();Bot_NavAdd((float[3]){0,0,1});
 clients[0].ps.origin[0]=clients[0].ps.origin[1]=0;svs.time+=1000;
 SV_BotEnableAI(0,1);SV_BotSetGoal(0,(float[3]){224,0,1},12,0,-1);
 Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].routeCount>0&&s_botNav[s_botAI[0].waypoint].origin[0]>0);

 // A cold spawn with no known exit retries promptly without increasing the
 // shared search budget. Arrival at an objective retains the slower interval.
 SV_BotResetNavigation();svs.time+=1000;Bot_NavAdd((float[3]){0,0,1});
 SV_BotEnableAI(0,1);SV_BotSetGoal(0,(float[3]){512,0,1},12,0,-1);
 Bot_WorkFrame();s_botWorkEdges=BOT_NAV_EDGES_PER_FRAME;
 s_botAI[0].nextPath=svs.time+1200;Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(!s_botAI[0].routeCount && s_botAI[0].nextPath==svs.time+200);
 svs.time+=50;SV_BotSetGoal(0,clients[0].ps.origin,12,0,-1);
 s_botAI[0].nextPath=svs.time+1200;Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(!s_botAI[0].routeCount && s_botAI[0].nextPath==svs.time+1200);

 // A local branch must go away from its objective to find a doorway around
 // a wall. Refusing every temporary detour trapped bots in small components.
 SV_BotResetNavigation();Bot_NavAdd((float[3]){224,0,1});
 wall=wallDoor=1;Bot_NavScout(0,(float[3]){384,0,1});wall=wallDoor=0;
 int doorway=0;for(i=0;i<s_botNavCount;i++)if(s_botNav[i].origin[0]>=256&&s_botNav[i].origin[1]>=96)doorway=1;
 assert(doorway);
 clients[0].ps.origin[0]=224;clients[0].ps.origin[1]=0;
 SV_BotEnableAI(0,1);SV_BotSetGoal(0,(float[3]){384,0,1},12,0,-1);
 Bot_FindPath(&s_botAI[0],&clients[0].ps);assert(s_botAI[0].routeCount>0);
 // A local half-cell repair reaches a doorway whose center lies between
 // regular grid lines. The nearest-node hash must include those portal cells.
 SV_BotResetNavigation();wall=1;wallDoor=2;
 int portalStart=Bot_NavAdd((float[3]){224,0,1});
 int portal=Bot_NavEdge(portalStart,1);
 assert(portal>=0&&s_botNav[portal].origin[0]==240&&s_botNav[portal].origin[1]==16);
 int crossing=Bot_NavEdge(portal,0);
 assert(crossing>=0&&s_botNav[crossing].origin[0]>=256&&s_botNav[crossing].origin[1]==16);
 assert(Bot_NearestNode(s_botNav[crossing].origin,0)==crossing);
 wall=wallDoor=0;
 // The priority queue must preserve every node and expand central geometry first.
 SV_BotResetNavigation();s_botNavBounded=1;
 s_botNavMin[0]=s_botNavMin[1]=-512;s_botNavMax[0]=s_botNavMax[1]=512;
 Bot_NavAdd((float[3]){256,0,1});Bot_NavAdd((float[3]){64,0,1});Bot_NavAdd((float[3]){0,0,1});
 assert(Bot_NavPop()==2&&Bot_NavPop()==1&&Bot_NavPop()==0&&Bot_NavPop()==-1);
 // Promoting local work uses one queue entry per node and keeps reverse
 // positions exact through decrease-key and pop. It never requeues closed work.
 SV_BotResetNavigation();s_botNavBounded=1;
 s_botNavMin[0]=s_botNavMin[1]=-8192;s_botNavMax[0]=s_botNavMax[1]=8192;
 for(i=0;i<200;i++)Bot_NavAdd((float[3]){i*32,0,1});
 Bot_NavPromote(190,-8);Bot_NavPromote(150,-7);Bot_NavPromote(190,-6);
 assert(s_botNavOpenCount==200 && Bot_NavPop()==190 && Bot_NavPop()==150);
 Bot_NavPromote(190,-10);assert(s_botNavOpenCount==198);
 int seenQueue[200]={0};seenQueue[190]=seenQueue[150]=1;
 while(s_botNavOpenCount) {
  for(int at=0;at<s_botNavOpenCount;at++)assert(s_botNavPosition[s_botNavOpen[at]]==at);
  int node=Bot_NavPop();assert(!seenQueue[node]++ && s_botNavPosition[node]==-1);
 }
 for(i=0;i<200;i++)assert(seenQueue[i]==1);
 // Promotion propagates only a bounded number of rings, even across new
 // geometry. It cannot assign permanent priority to the entire map flood.
 SV_BotResetNavigation();int ring=Bot_NavAdd((float[3]){0,0,1});
 Bot_NavPromote(ring,-8);
 for(i=1;i<=10;i++) {
  ring=Bot_NavEdge(ring,0);assert(ring>=0);
  if(i<8)assert(s_botNav[ring].priority==-8+i);
  else assert(s_botNav[ring].priority>=0);
 }
 // Distant route cells never issue precise trigger contact queries.
 memset(&g_entities[40],0,sizeof(g_entities[40]));
 for(i=0;i<3;i++){g_entities[40].r.absmin[i]=1000;g_entities[40].r.absmax[i]=1100;}
 contactCount=0;assert(!Bot_InObjective((float[3]){0,0,1},40)&&contactCount==0);
 assert(Bot_InObjective((float[3]){1050,1050,1050},40)&&contactCount==1);
 // Continuous new central nodes must not starve an older roof frontier.
 SV_BotResetNavigation();s_botNavBounded=1;
 s_botNavMin[0]=s_botNavMin[1]=-2048;s_botNavMax[0]=s_botNavMax[1]=2048;
 Bot_NavAdd((float[3]){1024,0,1});int fair=0;
 for(i=0;i<700;i++){
  Bot_NavAdd((float[3]){(i%15-7)*32,(i/15%15-7)*32,1+(i/225)*100});
  if(Bot_NavPop()==0){fair=1;break;}
 }
 assert(fair);
 // Guard/support slots occupy separate reachable places around the objective.
 SV_BotResetNavigation();
 for(int x=-5;x<=5;x++)for(int y=-5;y<=5;y++)Bot_NavAdd((float[3]){x*32,y*32,1});
 svs.time=10000;SV_BotEnableAI(0,1);SV_BotEnableAI(1,1);
 SV_BotSetGoal(0,(float[3]){0,0,1},160,1,-1);SV_BotSetGoal(1,(float[3]){0,0,1},160,1,-1);
 Bot_GuardPoint(0,&s_botAI[0],&clients[0].ps);Bot_GuardPoint(1,&s_botAI[1],&clients[1].ps);
 assert(Bot_DistanceSq(s_botAI[0].guardPoint,s_botAI[1].guardPoint)>64*64);
 // Brief loss of sight while a player jumps does not restart reaction time.
 clients[1].sess.cs.team=TEAM_AXIS;clients[1].ps.origin[0]=400;clients[1].ps.origin[1]=0;
 clients[2].sess.sessionState=SESS_STATE_DEAD;
 SV_BotEnableAI(0,2);Bot_Think(0,&s_botAI[0],&clients[0].ps);
 int acquired=s_botAI[0].acquiredAt;
 wall=1;svs.time+=200;Bot_Think(0,&s_botAI[0],&clients[0].ps);assert(s_botAI[0].target==-1);
 wall=0;svs.time+=200;Bot_Think(0,&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].target==1&&s_botAI[0].acquiredAt==acquired);
 // Hard tracking follows vertical velocity, still requiring visibility; a
 // scoped shot must wait for the actual original ADS transition to finish.
 clients[1].ps.origin[2]=65;clients[1].ps.velocity[2]=220;
 weapon.overlayReticle=1;weapon.bSemiAuto=0;clients[0].ps.fWeaponPosFrac=0;
 svs.time+=1000;s_botNavSeeded=1;s_botNavFrame=svs.time;
 s_botAI[0].aim[0]=-10;s_botAI[0].aim[1]=0;s_botAI[0].pitchError=s_botAI[0].yawError=0;
 s_botAI[0].nextError=svs.time+2000;
 SV_BotRunAI(0,&clients[0].ps);
 assert(s_botCmdState[0].buttons&0x1000);assert(!(s_botCmdState[0].buttons&1));
 assert(s_botAI[0].aim[0]<-5);
 clients[0].ps.fWeaponPosFrac=1;svs.time+=50;s_botNavFrame=svs.time;
 SV_BotRunAI(0,&clients[0].ps);assert(s_botCmdState[0].buttons&1);
 weapon.overlayReticle=0;clients[1].ps.origin[2]=1;clients[1].ps.velocity[2]=0;
 // A ranged fight must include forward approach, not permanent orbiting.
 clients[1].ps.origin[0]=1200;clients[0].ps.origin[0]=0;svs.time+=1000;
 SV_BotEnableAI(0,1);SV_BotRunAI(0,&clients[0].ps);
 assert(s_botCmdState[0].forwardmove>0);

 // Detect original ladder material and route both ways. Commands must climb
 // and descend through normal pmove, with no synthetic origin/velocity writes.
 SV_BotResetNavigation();ladderWorld=1;
 int bottom=Bot_NavAdd((float[3]){0,0,1});Bot_NavLadder(bottom,0);
 int top=s_botNav[bottom].edge[BOT_NAV_DIRECTIONS];
 assert(top>=0&&s_botNav[top].origin[2]==153&&s_botNav[top].edge[BOT_NAV_DIRECTIONS]==bottom);
 clients[0].ps.origin[0]=clients[0].ps.origin[1]=0;clients[0].ps.origin[2]=1;
 clients[1].sess.sessionState=SESS_STATE_DEAD;svs.time=20000;SV_BotEnableAI(0,1);
 SV_BotSetGoal(0,s_botNav[top].origin,12,0,-1);Bot_FindPath(&s_botAI[0],&clients[0].ps);
 s_botNavSeeded=1;s_botNavFrame=svs.time;SV_BotRunAI(0,&clients[0].ps);
 assert(s_botCmdState[0].forwardmove==127&&!(s_botCmdState[0].buttons&1));
 clients[0].ps.origin[2]=90;
 Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].waypoint==top&&s_botAI[0].routeCount>0);
 // Attachment may precede graph discovery: resume the validated climb past
 // its now-unreachable floor approach, but never skip it for an airborne bot.
 s_botAI[0].routeIndex=0;s_botAI[0].routeCount=2;
 s_botAI[0].route[0]=bottom;s_botAI[0].route[1]=top;s_botAI[0].routeFrom=bottom;
 clients[0].ps.pm_flags=0;Bot_AdvanceRoute(&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].waypoint==bottom);
 clients[0].ps.pm_flags=0x20;traceCount=0;
 Bot_AdvanceRoute(&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].waypoint==top&&s_botAI[0].routeIndex==1&&traceCount==0);
 memcpy(clients[0].ps.origin,s_botNav[top].origin,sizeof(vec3_t));
 svs.time+=200;SV_BotSetGoal(0,s_botNav[bottom].origin,12,0,-1);
 Bot_FindPath(&s_botAI[0],&clients[0].ps);clients[0].ps.pm_flags=0x20;
 s_botNav[top].edge[BOT_NAV_TRAVERSAL+4]=(short)bottom;
 s_botNavFrame=svs.time;SV_BotRunAI(0,&clients[0].ps);
 assert(s_botCmdState[0].forwardmove==-100&&!(s_botCmdState[0].buttons&1));
 assert(fabsf(Bot_AngleDelta(s_botNav[top].ladderYaw,s_botCmdState[0].angles[1]))<.001f);
 clients[0].ps.pm_flags=0;ladderWorld=0;
 // Discover the ladder from its roof landing before any lower spawn/node
 // exists. A missing ladder material must not invent an unsafe descent.
 SV_BotResetNavigation();ladderWorld=1;
 int roofFirst=Bot_NavAdd((float[3]){48,0,153});
 int lowerFirst=Bot_NavLadderDown(roofFirst,4);
 assert(lowerFirst>=0 && s_botNav[roofFirst].edge[BOT_NAV_DIRECTIONS]==lowerFirst);
 assert(s_botNav[lowerFirst].edge[BOT_NAV_DIRECTIONS]==roofFirst);
 assert(fabsf(s_botNav[lowerFirst].origin[2]-1)<.001f);
 traceCount=0;Bot_NavLadderDown(roofFirst,4);assert(traceCount==0);
 ladderWorld=0;SV_BotResetNavigation();roofFirst=Bot_NavAdd((float[3]){48,0,153});
 assert(Bot_NavLadderDown(roofFirst,4)<0 && s_botNavCount==1);
 // Roof exits can descend safely, but never invent a reverse climb or accept
 // a damaging fall. A low parapet requests an ordinary, bounded jump pulse.
 SV_BotResetNavigation();dropWorld=1;
 int roof=Bot_NavAdd((float[3]){0,0,161});int landing=Bot_NavDrop(roof,0);
 assert(landing>=0&&fabsf(s_botNav[landing].origin[2]-1)<.001f);
 assert(Bot_TraversalDirection(roof,landing)==0&&!s_botNav[roof].jumpMask);
 assert(Bot_TraversalDirection(landing,roof)<0);
 int approach=Bot_NavEdge(roof,0);assert(approach==landing);
 traceCount=0;assert(Bot_NavEdge(roof,0)==approach&&traceCount==0);
 SV_BotResetNavigation();dropWorld=2;roof=Bot_NavAdd((float[3]){0,0,161});
 assert(Bot_NavDrop(roof,0)<0);
 SV_BotResetNavigation();dropWorld=1;fallMin=128;roof=Bot_NavAdd((float[3]){0,0,161});
 assert(Bot_NavDrop(roof,0)<0);fallMin=256;
 SV_BotResetNavigation();dropWorld=4;roof=Bot_NavAdd((float[3]){0,0,161});
 assert(Bot_NavDrop(roof,0)<0);
 SV_BotResetNavigation();dropWorld=3;roof=Bot_NavAdd((float[3]){0,0,161});
 landing=Bot_NavDrop(roof,0);assert(landing>=0&&(s_botNav[roof].jumpMask&1));
 memcpy(clients[0].ps.origin,s_botNav[roof].origin,sizeof(vec3_t));
 clients[0].ps.groundEntityNum=1022;svs.time+=2000;SV_BotEnableAI(0,1);
 SV_BotSetGoal(0,s_botNav[landing].origin,12,3,-1);
 Bot_RequestPath(&s_botAI[0]);SV_BotPlanFrame();
 s_botNavSeeded=1;s_botNavFrame=svs.time;SV_BotRunAI(0,&clients[0].ps);
 assert(s_botCmdState[0].buttons&0x400);
 svs.time+=50;s_botNavFrame=svs.time;SV_BotRunAI(0,&clients[0].ps);
 assert(!(s_botCmdState[0].buttons&0x400));
 clients[0].ps.origin[0]=20;clients[0].ps.origin[2]=130;clients[0].ps.groundEntityNum=1023;
 Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].waypoint==landing&&s_botAI[0].routeCount>0);
 dropWorld=0;clients[0].ps.groundEntityNum=1022;
 // A roof's unexplored exit can be farther from the ground-level objective.
 // Choose that reachable frontier even when navigation work is exhausted;
 // do not orbit the closest roof edge or invent a link to the ground.
 SV_BotResetNavigation();svs.time+=5000;
 Bot_NavAdd((float[3]){64,0,161});Bot_NavAdd((float[3]){96,0,161});
 Bot_NavAdd((float[3]){0,0,161});Bot_NavAdd((float[3]){96,0,1});
 for(i=0;i<4;i++)for(int d=0;d<8;d++)s_botNav[i].edge[d]=-2;
 s_botNav[0].edge[0]=1;s_botNav[1].edge[4]=0;
 s_botNav[0].edge[4]=2;s_botNav[2].edge[0]=0;s_botNav[2].edge[2]=-1;
 memcpy(clients[0].ps.origin,s_botNav[0].origin,sizeof(vec3_t));SV_BotEnableAI(0,1);
 s_botAI[0].objectiveUntil=svs.time+10000;s_botAI[0].objectiveBehavior=3;
 memcpy(s_botAI[0].objective,s_botNav[3].origin,sizeof(vec3_t));
 Bot_WorkFrame();s_botWorkEdges=BOT_NAV_EDGES_PER_FRAME;s_botWorkNavMs=BOT_NAV_TIME_MS;
 Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].goal==2 && s_botAI[0].failedPaths==0 && !s_botAI[0].detourUntil);
 assert(s_botWorkEdges==BOT_NAV_EDGES_PER_FRAME && s_botNav[2].edge[2]==-1);
 svs.time+=50;Bot_ExpandFrontier(2);
 assert(s_botNav[2].edge[2]!=-1 && s_botWorkEdges<=8);
 traceCount=0;Bot_ExpandFrontier(2);assert(traceCount==0);
 // Prefer exploring an already-descending branch to another nearby roof
 // edge when the objective is below; this does not create a traversal link.
 SV_BotResetNavigation();
 Bot_NavAdd((float[3]){64,0,161});Bot_NavAdd((float[3]){96,0,161});
 Bot_NavAdd((float[3]){0,256,65});Bot_NavAdd((float[3]){96,0,1});
 for(i=0;i<4;i++)for(int d=0;d<8;d++)s_botNav[i].edge[d]=-2;
 s_botNav[0].edge[0]=1;s_botNav[1].edge[4]=0;
 s_botNav[0].edge[4]=2;s_botNav[2].edge[0]=0;
 s_botNav[1].edge[2]=-1;s_botNav[2].edge[2]=-1;
 SV_BotEnableAI(0,1);s_botAI[0].objectiveUntil=svs.time+10000;
 s_botAI[0].objectiveBehavior=3;memcpy(s_botAI[0].objective,s_botNav[3].origin,sizeof(vec3_t));
 svs.time+=50;Bot_WorkFrame();s_botWorkEdges=BOT_NAV_EDGES_PER_FRAME;
 s_botAI[0].routeCount=0;Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].goal==2 && s_botAI[0].failedPaths==0);
 // Finishing a tiny projected route cannot suppress recovery forever when
 // the strategic objective is outside the reachable component.
 SV_BotResetNavigation();s_botNavBounded=1;
 s_botNavMin[0]=160;s_botNavMax[0]=256;s_botNavMin[1]=s_botNavMax[1]=0;
 Bot_NavAdd((float[3]){224,0,1});Bot_NavAdd((float[3]){256,0,1});
 Bot_NavAdd((float[3]){192,0,1});s_botNav[0].edge[0]=1;s_botNav[0].edge[4]=2;
 s_botNav[1].edge[4]=s_botNav[2].edge[0]=0;
 // This fixture is fully explored: unlike a pending frontier, a closed
 // component must eventually abandon its failed approach and recover.
 for(i=0;i<3;i++)for(int d=0;d<8;d++)if(s_botNav[i].edge[d]<0)s_botNav[i].edge[d]=-2;
 clients[0].ps.origin[0]=250;clients[0].ps.origin[1]=0;clients[0].ps.origin[2]=1;
 svs.time+=1000;SV_BotEnableAI(0,1);wall=1;
 SV_BotSetGoal(0,(float[3]){512,0,1},12,3,-1);
 for(i=0;i<4;i++){svs.time+=200;s_botAI[0].routeCount=0;Bot_FindPath(&s_botAI[0],&clients[0].ps);Bot_AdvanceRoute(&s_botAI[0],&clients[0].ps);}
 assert(s_botAI[0].detourUntil>svs.time&&s_botAI[0].goal!=1);
 // Incomplete routes during graph warm-up must not abandon an objective when
 // the actor is actually approaching it. Stagnant projections still recover.
 SV_BotEnableAI(0,1);SV_BotSetGoal(0,(float[3]){512,0,1},12,3,-1);
 for(i=0;i<4;i++){
  clients[0].ps.origin[0]=160+i*32;
  Bot_FindPath(&s_botAI[0],&clients[0].ps);
  assert(s_botAI[0].detourUntil<=svs.time&&s_botAI[0].failedPaths==1);
 }
 wall=0;
 // An announced radio/model above its floor must not select a nearby roof or
 // report a disconnected objective when the ground route is fully connected.
 SV_BotResetNavigation();int last=-1;
 for(i=0;i<=16;i++){
  int n=Bot_NavAdd((float[3]){i*32,0,1});
  if(last>=0){s_botNav[last].edge[0]=(short)n;s_botNav[n].edge[4]=(short)last;}last=n;
 }
 Bot_NavAdd((float[3]){512,0,96});clients[0].ps.origin[0]=0;
 SV_BotEnableAI(0,1);SV_BotSetGoal(0,(float[3]){512,0,65},48,0,-1);
 Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].goal>=0 && Bot_AtPoint(s_botNav[s_botAI[0].goal].origin,s_botAI[0].objective,48) && s_botAI[0].failedPaths==0);
 // The exact nearest node can be on top of a radio pedestal with no
 // incoming walk edge. Stop at a reachable point inside the capture radius;
 // do not turn a complete route into repeated failed searches for its model.
 SV_BotResetNavigation();svs.time+=1000;
 pedestalWorld=1;
 int captureStart=Bot_NavAdd((float[3]){0,0,1});
 int captureFloor=Bot_NavAdd((float[3]){96,0,1});
 int pedestal=Bot_NavAdd((float[3]){128,0,37});
 for(int n=0;n<3;n++)for(int edge=0;edge<BOT_NAV_LINKS;edge++)s_botNav[n].edge[edge]=-2;
 s_botNav[captureStart].edge[0]=captureFloor;
 memcpy(clients[0].ps.origin,(float[3]){0,0,1},sizeof(vec3_t));
 SV_BotEnableAI(0,1);SV_BotSetGoal(0,(float[3]){128,0,37},48,0,-1);
 Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].goal==captureFloor && s_botAI[0].goal!=pedestal && s_botAI[0].failedPaths==0);
 assert(s_botAI[0].routeCount==1 && s_botAI[0].route[0]==captureFloor);
 pedestalWorld=0;
 // A radio over a ledge must keep the nearby upper landing as its destination,
 // not project onto the ground floor 160 units below the interaction radius.
 SV_BotResetNavigation();svs.time+=1000;dropWorld=1;
 int upper=Bot_NavAdd((float[3]){0,0,161});
 int lower=Bot_NavAdd((float[3]){64,0,1});
 // Abstract a previously verified stair route; this case tests floor choice.
 s_botNav[lower].edge[4]=upper;s_botNav[upper].edge[0]=lower;
 memcpy(clients[0].ps.origin,(float[3]){64,0,1},sizeof(vec3_t));
 SV_BotEnableAI(0,1);SV_BotSetGoal(0,(float[3]){64,0,161},48,0,-1);
 Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].goal==upper && s_botAI[0].planPoint[2]==161);
 s_botNavFrame=svs.time;s_botAI[0].nextThink=svs.time+200;
 SV_BotRunAI(0,&clients[0].ps);assert(!s_botAI[0].approachUntil);
 dropWorld=0;
 // A bounded search of a large lower floor cannot reach an isolated upper
 // goal yet. Follow the known route to an unexplored landing instead of
 // repeatedly ending at the cell directly underneath that goal.
 SV_BotResetNavigation();svs.time+=1000;short frontierGrid[48][48];
 for(int x=0;x<48;x++)for(int y=0;y<48;y++) {
  int node=frontierGrid[x][y]=Bot_NavAdd((float[3]){x*32,y*32,1});
  for(int edge=0;edge<BOT_NAV_LINKS;edge++)s_botNav[node].edge[edge]=-2;
 }
 for(int x=0;x<48;x++)for(int y=0;y<48;y++)for(int edge=0;edge<8;edge+=2) {
  int nx=x+s_botDirections[edge][0],ny=y+s_botDirections[edge][1];
  if(nx>=0&&nx<48&&ny>=0&&ny<48)s_botNav[frontierGrid[x][y]].edge[edge]=frontierGrid[nx][ny];
 }
 int highFrontier=Bot_NavAdd((float[3]){1152,768,41});
 int disconnectedUpper=Bot_NavAdd((float[3]){768,768,161});
 for(int edge=0;edge<BOT_NAV_LINKS;edge++) {
  s_botNav[highFrontier].edge[edge]=-2;s_botNav[disconnectedUpper].edge[edge]=-2;
 }
 s_botNav[highFrontier].edge[0]=-1;s_botNav[frontierGrid[36][24]].edge[8]=highFrontier;
 memcpy(clients[0].ps.origin,(float[3]){768,768,1},sizeof(vec3_t));
 SV_BotEnableAI(0,1);SV_BotSetGoal(0,(float[3]){768,768,161},48,0,-1);
 Bot_WorkFrame();s_botWorkEdges=BOT_NAV_EDGES_PER_FRAME;
 Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(s_botPathExpanded==BOT_PATH_NODES && s_botAI[0].goal==highFrontier && s_botAI[0].routeCount>0);
 assert(s_botWorkPaths==1 && s_botWorkEdges==BOT_NAV_EDGES_PER_FRAME);
 puts("PASS: elevated objective floor selection and upper-frontier progress within the unchanged search budget");
 // An expensive remote background probe must not consume the local route's
 // entire allowance before an isolated actor can discover its first exit.
 SV_BotResetNavigation();s_botNavSeeded=s_botNavBounded=1;svs.time+=50;
 s_botNavMin[0]=-4096;s_botNavMax[0]=512;s_botNavMin[1]=-512;s_botNavMax[1]=512;
 Bot_NavAdd((float[3]){-2048,0,1});Bot_NavAdd((float[3]){0,0,1});
 memcpy(clients[0].ps.origin,(float[3]){0,0,1},sizeof(vec3_t));
 clients[0].sess.sessionState=SESS_STATE_PLAYING;g_entities[0].health=100;
 SV_BotEnableAI(0,1);SV_BotSetGoal(0,(float[3]){224,0,1},12,0,-1);
 Bot_RequestPath(&s_botAI[0]);slowBackground=1;traceMs=0;SV_BotPlanFrame();slowBackground=0;
 assert(s_botAI[0].routeCount>0 && s_botNav[s_botAI[0].waypoint].origin[0]>0);
 assert(s_botWorkEdges<=BOT_NAV_EDGES_PER_FRAME && s_botWorkPaths<=BOT_PATHS_PER_FRAME);
 // The warm-up budget must yield to a slow host before exhausting all edges.
 SV_BotResetNavigation();s_botNavSeeded=1;Bot_NavAdd((float[3]){0,0,1});
 svs.time+=50;navClock=1;traceCount=0;Bot_NavFrame();navClock=0;
 assert(s_botNavWorkEdges<=32);
 // A large connected map yields a useful partial route within the visit
 // budget instead of scanning every cell in one server frame.
 SV_BotResetNavigation();s_botNavSeeded=1;
 for(i=0;i<3000;i++){int n=Bot_NavAdd((float[3]){i*32,0,1});if(i)s_botNav[n-1].edge[0]=n;}
 clients[0].ps.origin[0]=clients[0].ps.origin[1]=0;clients[0].ps.origin[2]=1;
 SV_BotEnableAI(0,1);SV_BotSetGoal(0,(float[3]){2999*32,0,1},12,0,-1);
 s_botProfile.enabled=1;s_botProfile.visited=0;Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(s_botProfile.visited<=BOT_PATH_NODES&&s_botAI[0].routeCount>0);
 s_botProfile.enabled=0;
 // Guided search crosses a wide connected floor within the visit budget;
 // breadth-first expansion would exhaust the budget before the far corner.
 SV_BotResetNavigation();s_botNavSeeded=1;
 for(int y=0;y<64;y++)for(int x=0;x<64;x++)Bot_NavAdd((float[3]){x*32,y*32,1});
 for(int y=0;y<64;y++)for(int x=0;x<64;x++)for(int d=0;d<8;d++){
  int nx=x+s_botDirections[d][0],ny=y+s_botDirections[d][1];
  if(nx>=0&&nx<64&&ny>=0&&ny<64)s_botNav[y*64+x].edge[d]=(short)(ny*64+nx);
 }
 memcpy(clients[0].ps.origin,(float[3]){0,0,1},sizeof(vec3_t));
 SV_BotEnableAI(0,1);SV_BotSetGoal(0,(float[3]){2016,2016,1},12,0,-1);
 s_botProfile.enabled=1;s_botProfile.visited=0;Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].goal==4095&&s_botAI[0].routeCount==63&&s_botProfile.visited<100);
 s_botProfile.enabled=0;
 // Three blocked near contacts cannot permanently mask the next visible
 // enemy. Rotating perception finds it without extra traces or hidden memory.
 SV_BotResetNavigation();level.maxclients=5;svs.time+=1000;SV_BotEnableAI(0,2);
 for(i=1;i<5;i++){
  g_entities[i].client=&clients[i];g_entities[i].health=100;
  clients[i].sess.connected=2;clients[i].sess.sessionState=0;clients[i].sess.cs.team=TEAM_AXIS;
  clients[i].ps.origin[0]=400;clients[i].ps.origin[1]=i<4?i*10:200;
  clients[i].ps.origin[2]=1;clients[i].ps.viewHeightCurrent=60;
 }
 wall=wallDoor=1;
 assert(Bot_AcquireTarget(0,&s_botAI[0],&clients[0].ps)==-1);
 svs.time+=200;assert(Bot_AcquireTarget(0,&s_botAI[0],&clients[0].ps)==4);
 traceCount=0;assert(Bot_ObserveTarget(0,&s_botAI[0],4)&&traceCount==0);
 wall=wallDoor=0;level.maxclients=3;
 // A close visible threat can interrupt tunnel vision on a distant enemy.
 // Hidden contacts, teammates and opponents outside the view cone cannot.
 SV_BotResetNavigation();svs.time+=1000;SV_BotEnableAI(0,1);
 clients[1].ps.origin[0]=600;clients[1].ps.origin[1]=0;
 clients[2].ps.origin[0]=128;clients[2].ps.origin[1]=0;
 s_botAI[0].target=s_botAI[0].lastTarget=1;s_botAI[0].acquiredAt=svs.time-3000;
 traceCount=0;Bot_Think(0,&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].target==2 && s_botAI[0].acquiredAt==svs.time && traceCount==1);
 assert(s_botAI[0].lastSeen[0]==128); // Only the observed threat enters memory.
 s_botAI[0].target=1;svs.time+=200;hiddenNear=1;traceCount=0;
 assert(Bot_AcquireTarget(0,&s_botAI[0],&clients[0].ps)==1 && traceCount==2);
 hiddenNear=0;clients[2].ps.origin[0]=-128;svs.time+=200;traceCount=0;
 assert(Bot_AcquireTarget(0,&s_botAI[0],&clients[0].ps)==1 && traceCount==1);
 clients[2].ps.origin[0]=128;clients[2].sess.cs.team=TEAM_ALLIES;svs.time+=200;
 assert(Bot_AcquireTarget(0,&s_botAI[0],&clients[0].ps)==1);
 clients[2].sess.cs.team=TEAM_AXIS;clients[1].ps.origin[0]=200;svs.time+=200;traceCount=0;
 assert(Bot_AcquireTarget(0,&s_botAI[0],&clients[0].ps)==1 && traceCount==1);
 clients[1].ps.origin[0]=600;smoke=1;svs.time+=200;traceCount=0;
 assert(Bot_AcquireTarget(0,&s_botAI[0],&clients[0].ps)==-1 && traceCount<=3);smoke=0;
 puts("PASS: urgent visible threats interrupt distant aim with original reaction delay; hidden/behind/team contacts rejected; close fights stable; at most three perception rays");
 // A live route is reused without a BFS, nearest-node scan or scout work.
 SV_BotResetNavigation();s_botNavSeeded=1;
 for(i=0;i<8;i++){int n=Bot_NavAdd((float[3]){i*32,0,1});if(i)s_botNav[n-1].edge[0]=n;}
 clients[0].ps.origin[0]=clients[0].ps.origin[1]=0;clients[0].ps.origin[2]=1;
 SV_BotEnableAI(0,1);SV_BotSetGoal(0,(float[3]){224,0,1},12,0,-1);
 Bot_FindPath(&s_botAI[0],&clients[0].ps);assert(s_botAI[0].routeCount>0);
 traceCount=0;s_botProfile.enabled=1;s_botProfile.visited=0;
 Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(traceCount==0&&s_botProfile.visited==0);s_botProfile.enabled=0;
 // Planning is bounded across ALL clients and rotates fairly between ticks.
 for(i=0;i<3;i++) {
  g_entities[i].health=100;clients[i].sess.sessionState=SESS_STATE_PLAYING;
  memcpy(clients[i].ps.origin,clients[0].ps.origin,sizeof(vec3_t));
  SV_BotEnableAI(i,1);Bot_RequestPath(&s_botAI[i]);
 }
 s_botNavFrame=svs.time;SV_BotPlanFrame();
 assert(!s_botAI[0].pathPending&&!s_botAI[1].pathPending&&s_botAI[2].pathPending);
 svs.time+=50;s_botNavFrame=svs.time;Bot_RequestPath(&s_botAI[0]);Bot_RequestPath(&s_botAI[1]);
 SV_BotPlanFrame();assert(!s_botAI[2].pathPending&&!s_botAI[0].pathPending&&s_botAI[1].pathPending);
 // Multiple scouts share one wall-time allowance, rather than adding a
 // separate allowance for every bot. Cached links are still available.
 SV_BotResetNavigation();Bot_NavAdd((float[3]){0,0,1});navClock=1;
 Bot_NavScout(0,(float[3]){224,0,1});int firstCount=s_botNavCount;
 Bot_NavScout(0,(float[3]){-224,0,1});navClock=0;
 assert(s_botNavCount==firstCount);
 // A half-cell waypoint has an eight-unit arrival radius. At nine units
 // the bot must continue walking rather than idle forever outside arrival.
 SV_BotResetNavigation();s_botNavSeeded=1;svs.time+=1000;
 for(i=1;i<3;i++)clients[i].sess.sessionState=SESS_STATE_DEAD;
 Bot_NavAdd((float[3]){0,0,1});Bot_NavAdd((float[3]){16,0,1});
 clients[0].ps.origin[0]=7;clients[0].ps.origin[1]=0;clients[0].ps.origin[2]=1;
 SV_BotEnableAI(0,1);s_botAI[0].routeFrom=0;s_botAI[0].route[0]=1;
 s_botAI[0].routeCount=1;s_botAI[0].goal=s_botAI[0].waypoint=1;
 s_botAI[0].nextThink=s_botAI[0].nextPath=svs.time+1000;s_botNavFrame=svs.time;
 SV_BotRunAI(0,&clients[0].ps);assert(s_botCmdState[0].forwardmove>0);
 clients[0].ps.origin[0]=10;clients[0].ps.origin[2]=10;Bot_AdvanceRoute(&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].waypoint<0);
 // No navigation growth across the authored playable envelope.
 s_botNavBounded=1;s_botNavMin[0]=s_botNavMin[1]=-160;s_botNavMax[0]=s_botNavMax[1]=160;
 assert(Bot_NavAdd((float[3]){192,0,1})==-1);

 // Flat link batching retains every floor probe and uses a single body
 // sweep; stairs still use piecewise clearance.
 traceCount=0;assert(Bot_WalkEdge((float[3]){0,0,1},(float[3]){32,0,1}));
 assert(traceCount==5);
 stairs=1;traceCount=0;assert(Bot_WalkEdge((float[3]){0,0,1},(float[3]){32,0,25}));
 assert(traceCount>5);stairs=0;

 // An offset narrow doorway must be connected, then merge back into the
 // ordinary grid. Nearest-node lookup must include the refined cells.
 SV_BotResetNavigation();wall=1;wallDoor=2;
 int doorStart=Bot_NavAdd((float[3]){224,0,1});
 int doorSide=Bot_NavEdge(doorStart,1);
 assert(doorSide>=0 && s_botNav[doorSide].origin[1]==16);
 int doorEntry=Bot_NavEdge(doorSide,0);
 assert(doorEntry>=0 && s_botNav[doorEntry].origin[0]==256 && s_botNav[doorEntry].origin[1]==16);
 assert(Bot_NearestNode((float[3]){256,16,1},1)==doorEntry);
 int doorExit=Bot_NavEdge(doorEntry,0);
 assert(doorExit>=0 && s_botNav[doorExit].origin[0]==288 && s_botNav[doorExit].origin[1]==32);
 wall=wallDoor=0;

 // Tight links cannot stop in the gap between movement and arrival radii.
 SV_BotResetNavigation();svs.time+=5000;
 Bot_NavAdd((float[3]){0,0,1});Bot_NavAdd((float[3]){16,0,1});
 s_botNav[0].edge[0]=1;s_botNav[1].edge[4]=0;
 for(i=1;i<3;i++)clients[i].sess.sessionState=SESS_STATE_DEAD;
 clients[0].ps.origin[0]=7;clients[0].ps.origin[1]=0;clients[0].ps.origin[2]=1;
 SV_BotEnableAI(0,1);SV_BotSetGoal(0,(float[3]){1000,0,1},12,0,-1);
 s_botAI[0].route[0]=1;s_botAI[0].routeCount=1;s_botAI[0].routeFrom=0;s_botAI[0].waypoint=1;
 s_botAI[0].nextThink=s_botAI[0].nextPath=svs.time+1000;
 s_botNavSeeded=1;s_botNavFrame=svs.time;
 SV_BotRunAI(0,&clients[0].ps);
 assert(s_botCmdState[0].forwardmove || s_botCmdState[0].rightmove);
 // Ordinary 32-unit links retain running speed; only tight entries brake.
 s_botNav[1].origin[0]=32;int distant=Bot_NavAdd((float[3]){512,0,1});
 s_botAI[0].goal=distant;svs.time+=50;s_botNavFrame=svs.time;
 SV_BotRunAI(0,&clients[0].ps);
 assert(s_botCmdState[0].forwardmove==127 && (s_botCmdState[0].buttons&BUTTON_SPRINT));

 // Complete navigation workload is shared, including repeated local scouts
 // and seed requests from all 32 bots in the SAME server frame.
 SV_BotResetNavigation();svs.time+=5000;s_botNavSeeded=1;
 Bot_NavAdd((float[3]){0,0,1});Bot_NavFrame();
 for(i=0;i<32;i++) {
  Bot_NavScout(0,(float[3]){1000,(i-8)*100,1});
  Bot_NavSeedPoint((float[3]){i*32,0,1});
 }
 assert(s_botWorkEdges<=BOT_NAV_EDGES_PER_FRAME);

 // Replanning a valid route is free and cannot change its first corner when
 // aim error consumes the bot RNG. Shared A* admission eventually serves all.
 SV_BotResetNavigation();svs.time+=5000;level.maxclients=32;level.num_entities=32;
 for(i=0;i<33;i++) {
  int n=Bot_NavAdd((float[3]){i*32,0,1});
  if(i){s_botNav[n-1].edge[0]=n;s_botNav[n].edge[4]=n-1;}
 }
 for(i=0;i<32;i++) {
  memset(&clients[i],0,sizeof(clients[i]));g_entities[i].client=&clients[i];g_entities[i].health=100;
  clients[i].sess.connected=2;clients[i].sess.cs.team=TEAM_ALLIES;
  clients[i].ps.origin[2]=1;clients[i].ps.weapon=1;
  SV_BotEnableAI(i,1);SV_BotSetGoal(i,(float[3]){1024,0,1},12,0,-1);
 }
 for(int tick=0;tick<16;tick++) {
  svs.time+=50;
  for(i=0;i<32;i++) Bot_FindPath(&s_botAI[i],&clients[i].ps);
  assert(s_botWorkPaths<=BOT_PATHS_PER_FRAME);
 }
 for(i=0;i<32;i++) assert(s_botAI[i].routeCount>0);
 svs.time+=50;
 int searches=s_botPathSearches, first=s_botAI[0].waypoint;
 for(i=0;i<20;i++){Bot_Random(&s_botAI[0]);Bot_FindPath(&s_botAI[0],&clients[0].ps);}
 assert(s_botPathSearches==searches && s_botAI[0].waypoint==first);
 // A drifting carrier/goal invalidates against the planned destination, not
 // only the difference between successive small script updates.
 svs.time+=50;SV_BotSetGoal(0,(float[3]){1080,0,1},12,0,-1);
 SV_BotSetGoal(0,(float[3]){1140,0,1},12,0,-1);
 Bot_FindPath(&s_botAI[0],&clients[0].ps);assert(s_botPathSearches==searches+1);

 // Movement around a small circuit is not useful route progress. It must
 // reject the blocked route even with >12 units of displacement per check.
 botAIState_t *actor=&s_botAI[0];
 actor->objectiveUntil=svs.time+10000;actor->nextPath=svs.time+10000;
 actor->progressNode=actor->waypoint;actor->progressDistance=0;actor->progressAt=svs.time-2500;
 actor->wantsMove=1;actor->lastMoveCheck=svs.time;actor->target=-1;
 int recovered=actor->recoveries;
 Bot_Think(0,actor,&clients[0].ps);
 assert(actor->recoveries==recovered+1);

 // Local team contact reports contain only a prior visible position, expire,
 // and never become fire permission or reach an opposing/DM player.
 SV_BotResetNavigation();svs.time+=5000;level.maxclients=3;
 for(i=0;i<3;i++) SV_BotEnableAI(i,1);
 clients[0].ps.origin[0]=0;clients[1].ps.origin[0]=400;clients[2].ps.origin[0]=600;
 clients[1].sess.cs.team=TEAM_AXIS;clients[2].sess.cs.team=TEAM_ALLIES;
 clients[2].ps.viewangles[1]=180;
 Bot_Think(0,&s_botAI[0],&clients[0].ps);assert(s_botAI[0].target==1);
 wall=1;s_botAI[2].aim[1]=180;clients[2].ps.origin[0]=32;
 s_botAI[2].nextPath=svs.time+10000;
 Bot_Think(2,&s_botAI[2],&clients[2].ps);
 assert(s_botAI[2].target<0 && s_botAI[2].lastSeen[0]==400 && s_botAI[2].memoryUntil>svs.time);
 clients[1].ps.origin[0]=800;svs.time+=1800;Bot_Think(2,&s_botAI[2],&clients[2].ps);
 assert(s_botAI[2].lastSeen[0]==400 && s_botAI[2].memoryUntil<=svs.time);wall=0;


 // Moving one grid cell sideways stays in the same reserved corridor: do
 // not take a pointless tiny detour. Enemies/DM do not
 // leak route information into the planner.
 SV_BotResetNavigation();svs.time+=5000;level.maxclients=3;
 for(i=0;i<3;i++) {
  clients[i].sess.cs.team=TEAM_ALLIES;clients[i].sess.sessionState=SESS_STATE_PLAYING;
  g_entities[i].health=100;SV_BotEnableAI(i,1);
 }
 clients[0].ps.origin[0]=clients[0].ps.origin[1]=0;clients[0].ps.origin[2]=1;
 Bot_NavAdd((float[3]){0,0,1});Bot_NavAdd((float[3]){32,0,1});
 Bot_NavAdd((float[3]){32,32,1});Bot_NavAdd((float[3]){64,0,1});
 s_botNav[0].edge[0]=1;s_botNav[0].edge[1]=2;
 s_botNav[1].edge[0]=s_botNav[2].edge[7]=3;
 SV_BotSetGoal(0,(float[3]){64,0,1},12,0,-1);
 s_botAI[1].routeCount=2;s_botAI[1].route[0]=1;s_botAI[1].route[1]=3;
 Bot_WorkFrame();s_botWorkEdges=BOT_NAV_EDGES_PER_FRAME;
 traceCount=0;Bot_RouteTraffic(&s_botAI[0]);assert(traceCount==0);
 Bot_FindPath(&s_botAI[0],&clients[0].ps);assert(s_botAI[0].route[0]==1);
 s_botNav[0].edge[1]=-1;s_botAI[0].routeCount=0;
 Bot_FindPath(&s_botAI[0],&clients[0].ps);assert(s_botAI[0].route[0]==1);
 svs.time+=50;Bot_WorkFrame();s_botWorkEdges=BOT_NAV_EDGES_PER_FRAME;
 s_botNav[0].edge[1]=2;s_botAI[0].routeCount=0;clients[1].sess.cs.team=TEAM_AXIS;
 Bot_FindPath(&s_botAI[0],&clients[0].ps);assert(s_botAI[0].route[0]==1);
 clients[0].sess.cs.team=clients[1].sess.cs.team=TEAM_FREE;
 s_botAI[0].routeCount=0;Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].route[0]==1);
 s_botTrafficGeneration=~0u;Bot_RouteTraffic(&s_botAI[0]);assert(s_botTrafficGeneration==1);

 // A wide main corridor has two adjacent rows. Four peers occupying it
 // must send the next bot down the actual alternate street, not the adjacent
 // grid row in that same crowd. The alternate rejoins the same objective.
 SV_BotResetNavigation();svs.time+=5000;level.maxclients=5;
 for(i=0;i<5;i++) {
  g_entities[i].client=&clients[i];g_entities[i].health=100;
  clients[i].sess.connected=2;clients[i].sess.sessionState=SESS_STATE_PLAYING;
  clients[i].sess.cs.team=TEAM_ALLIES;clients[i].ps.origin[0]=0;
  clients[i].ps.origin[1]=0;clients[i].ps.origin[2]=1;SV_BotEnableAI(i,1);
 }
 for(int x=0;x<33;x++)for(int y=0;y<13;y++)Bot_NavAdd((float[3]){x*32,y*32,1});
 for(int x=0;x<33;x++)for(int y=0;y<13;y++) {
  int n=x*13+y;
  for(int d=0;d<8;d++)s_botNav[n].edge[d]=-2;
  if(x<32 && (y<2 || y==12))s_botNav[n].edge[0]=(x+1)*13+y;
  if(x>0 && (y<2 || y==12))s_botNav[n].edge[4]=(x-1)*13+y;
  if(y<12 && (x==0 || x==32 || y==0))s_botNav[n].edge[2]=n+1;
  if(y>0 && (x==0 || x==32 || y==1))s_botNav[n].edge[6]=n-1;
 }
 for(int peer=1;peer<5;peer++) {
  s_botAI[peer].routeCount=32;
  for(i=0;i<32;i++)s_botAI[peer].route[i]=(i+1)*13;
 }
 SV_BotSetGoal(0,(float[3]){1024,0,1},12,0,-1);
 Bot_FindPath(&s_botAI[0],&clients[0].ps);
 int alternate=0;for(i=0;i<s_botAI[0].routeCount;i++)
  if(s_botNav[s_botAI[0].route[i]].origin[1]==384)alternate=1;
 assert(alternate && s_botAI[0].goal==32*13);
 // Removing the alternate exit still permits the crowded main street.
 s_botNav[0].edge[2]=-2;svs.time+=50;s_botAI[0].routeCount=0;
 Bot_FindPath(&s_botAI[0],&clients[0].ps);assert(s_botAI[0].goal==32*13);
 assert(s_botAI[0].route[0]==13);
 // Reservation counts are per peer, not per sampled route cell.
 Bot_RouteTraffic(&s_botAI[0]);assert(Bot_TrafficCost(13*16)==48);
 // Two strategic patrol goals sharing a partial frontier still reserve two
 // separate destinations. Even a large unexplored region cannot attract all.
 s_botAI[1].patrolGoal=13*8;s_botAI[2].patrolGoal=13*32;
 s_botAI[1].patrolUntil=s_botAI[2].patrolUntil=svs.time+20000;
 s_botAI[1].goal=s_botAI[2].goal=0;s_botAI[0].goal=-1;
 short choices[3]={13*8,13*32,13*16+12};
 for(unsigned seed=1;seed<=32;seed++) {
  s_botAI[0].random=seed*7919;
  assert(Bot_PatrolGoal(&s_botAI[0],&clients[0].ps,choices,3)==13*16+12);
 }
 puts("PASS: spatial corridor reservations, real alternate street, unavoidable passage, per-peer counts, persistent patrol destination spacing");

 // A useful firing lane beats a nominal guard slot facing straight into a
 // wall. Holding the assignment uses no new traces; ALL 32 bots and sniper
 // lookouts share the tactical cap in the same frame.
 SV_BotResetNavigation();svs.time+=5000;level.maxclients=32;
 Bot_NavAdd((float[3]){128,0,1});Bot_NavAdd((float[3]){128,64,1});
 for(i=0;i<32;i++) {
  g_entities[i].client=&clients[i];g_entities[i].health=100;
  clients[i].sess.connected=2;clients[i].sess.sessionState=SESS_STATE_PLAYING;
  clients[i].sess.cs.team=TEAM_ALLIES;SV_BotEnableAI(i,1);
  s_botAI[i].objectiveRadius=160;s_botAI[i].objective[2]=1;
  s_botAI[i].objectiveUntil=svs.time+1000;s_botAI[i].objectiveBehavior=1;
 }
 tacticalWall=1;traceCount=0;
 Bot_GuardPoint(0,&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].guardPoint[1]==64 && traceCount==6);
 traceCount=0;Bot_GuardPoint(0,&s_botAI[0],&clients[0].ps);assert(traceCount==0);
 for(i=1;i<32;i++)Bot_GuardPoint(i,&s_botAI[i],&clients[i].ps);
 weapon.overlayReticle=1;Bot_PatrolGoal(&s_botAI[0],&clients[0].ps,NULL,s_botNavCount);
 weapon.overlayReticle=0;tacticalWall=0;
 assert(s_botWorkTacticalTraces<=BOT_TACTICAL_TRACES_PER_FRAME && traceCount<=6);
 // A busy frame preserves the prior position; deferral is not a new route.
 vec3_t guardSaved;memcpy(guardSaved,s_botAI[0].guardPoint,sizeof(vec3_t));
 s_botAI[0].guardUntil=svs.time;s_botAI[0].nextPath=svs.time+1000;
 Bot_GuardPoint(0,&s_botAI[0],&clients[0].ps);
 assert(!memcmp(guardSaved,s_botAI[0].guardPoint,sizeof(vec3_t)) && s_botAI[0].nextPath>svs.time);

 // A full defense squad spreads beyond the small escort ring. A small
 // escort stays close; both use the same candidate count and ray budget.
 SV_BotResetNavigation();svs.time+=5000;level.maxclients=16;
 for(int x=-384;x<=384;x+=32)for(int y=-384;y<=384;y+=32)
  Bot_NavAdd((float[3]){x,y,1});
 for(i=0;i<16;i++) {
  clients[i].sess.cs.team=TEAM_ALLIES;clients[i].sess.sessionState=SESS_STATE_PLAYING;
  g_entities[i].health=100;SV_BotEnableAI(i,1);
  s_botAI[i].objectiveUntil=svs.time+1000;s_botAI[i].objectiveBehavior=1;
  s_botAI[i].objectiveRadius=160;s_botAI[i].objective[2]=1;
  s_botAI[i].guardUntil=svs.time+1000;s_botAI[i].guardPoint[2]=1;
 }
 s_botAI[0].guardUntil=0;traceCount=0;
 Bot_GuardPoint(0,&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].guardPoint[0]>=256 && traceCount<=6);
 level.maxclients=3;svs.time+=50;s_botAI[0].guardUntil=0;traceCount=0;
 Bot_GuardPoint(0,&s_botAI[0],&clients[0].ps);
 assert(Bot_DistanceSq(s_botAI[0].guardPoint,s_botAI[0].objective)<=160*160 && traceCount<=6);
 puts("PASS: large objective squads expand coverage while small escorts keep the close ring and shared trace budget");

 // At most two reinforcements pursue a reported contact. Death or a new
 // strategic duty releases that slot immediately; leases also expire.
 SV_BotResetNavigation();svs.time+=5000;level.maxclients=5;
 for(i=0;i<5;i++)SV_BotEnableAI(i,1);
 botContact_t *report=&s_botContacts[TEAM_ALLIES];report->reporter=0;report->until=svs.time+1600;
 assert(Bot_ClaimContact(&s_botAI[1],report));assert(Bot_ClaimContact(&s_botAI[2],report));
 assert(!Bot_ClaimContact(&s_botAI[3],report));
 g_entities[1].health=0;assert(Bot_ClaimContact(&s_botAI[3],report));g_entities[1].health=100;
 s_botAI[2].objectiveUntil=svs.time+1600;assert(Bot_ClaimContact(&s_botAI[4],report));
 svs.time+=1700;assert(!Bot_ClaimContact(&s_botAI[1],report));
 report->until=svs.time+1600;assert(Bot_ClaimContact(&s_botAI[1],report));

 // Collision-driven yielding has no extra trace or roster scan. Only one
 // side backs off; objective carriers/users and humans retain priority.
 for(i=0;i<5;i++) {s_botAI[i].objectiveUntil=0;s_botAI[i].nextYield=s_botAI[i].yieldUntil=0;}
 traceCount=0;Bot_YieldToBlocker(&s_botAI[1],2);assert(!s_botAI[1].yieldUntil);
 Bot_YieldToBlocker(&s_botAI[2],1);assert(s_botAI[2].yieldUntil==svs.time+700);
 s_botAI[3].objectiveUntil=svs.time+1000;s_botAI[3].objectiveBehavior=3;
 Bot_YieldToBlocker(&s_botAI[3],1);assert(!s_botAI[3].yieldUntil);
 Bot_YieldToBlocker(&s_botAI[1],3);assert(s_botAI[1].yieldUntil>svs.time);
 s_botAI[0].enabled=0;Bot_YieldToBlocker(&s_botAI[3],0);assert(s_botAI[3].yieldUntil>svs.time);
 assert(traceCount==0);
 // The yield produces ordinary backward input and ends after its lease;
 // neither a paused bot nor the teammate's origin is moved by the AI.
 SV_BotEnableAI(2,1);clients[2].ps.origin[0]=clients[2].ps.origin[1]=0;clients[2].ps.origin[2]=1;
 clients[1].ps.origin[0]=40;clients[1].ps.origin[1]=0;clients[1].ps.origin[2]=1;
 s_botAI[2].aim[1]=0;s_botAI[2].nextThink=s_botAI[2].nextPath=svs.time+10000;
 Bot_YieldToBlocker(&s_botAI[2],1);s_botNavSeeded=1;s_botNavFrame=svs.time;
 SV_BotRunAI(2,&clients[2].ps);
 assert(s_botCmdState[2].forwardmove<0 && clients[2].ps.origin[0]==0 && clients[1].ps.origin[0]==40);
 svs.time+=800;s_botNavFrame=svs.time;SV_BotRunAI(2,&clients[2].ps);
 assert(!s_botCmdState[2].forwardmove && !s_botCmdState[2].rightmove);

 // Seeing an enemy cannot disable recovery while following an objective
 // route. Deliberately stopping to shoot does not count as a blocked route.
 SV_BotResetNavigation();svs.time+=5000;level.maxclients=2;
 clients[1].sess.cs.team=TEAM_AXIS;clients[1].ps.origin[0]=400;
 clients[1].ps.origin[1]=0;clients[1].ps.origin[2]=1;
 clients[0].ps.origin[0]=clients[0].ps.origin[1]=0;
 SV_BotEnableAI(0,2);Bot_NavAdd((float[3]){128,0,1});
 actor=&s_botAI[0];actor->waypoint=0;actor->route[0]=0;actor->routeCount=1;
 actor->progressNode=0;actor->progressDistance=0;actor->progressAt=svs.time-2500;
 actor->wantsMove=1;actor->routeMove=0;actor->nextPath=svs.time+10000;
 Bot_Think(0,actor,&clients[0].ps);assert(actor->target==1 && actor->recoveries==0);
 actor->progressNode=0;actor->progressAt=svs.time-2500;actor->routeMove=1;
 Bot_Think(0,actor,&clients[0].ps);assert(actor->recoveries==1);
 actor->patrolGoal=0;actor->patrolUntil=actor->perchUntil=svs.time+10000;
 Bot_StartDetour(actor);assert(actor->patrolGoal==-1 && !actor->patrolUntil && !actor->perchUntil);
 puts("PASS: soft team route reservations, firing-lane selection, shared 32-bot tactical trace cap, contact reinforcement limits, combat route recovery");

 // A sheltered crouched lane beats a completely exposed rooftop. Static
 // geometry is cached once and shared by all bots within the same ray cap.
 SV_BotResetNavigation();svs.time+=5000;level.maxclients=2;weapon.overlayReticle=1;
 int sheltered=Bot_NavAdd((float[3]){0,0,1});
 int exposed=Bot_NavAdd((float[3]){512,0,1});
 perchWorld=1;traceCount=0;Bot_EvaluatePerch(sheltered);
 assert(traceCount==8 && s_botNav[sheltered].lookout>600);
 assert(sizeof(botNavNode_t)==60); // Cached facing must not enlarge 32K nodes.
 assert(s_botNav[sheltered].lookoutDir==0);
 Bot_EvaluatePerch(exposed);assert(traceCount==8 && s_botNav[exposed].lookout<0);
 svs.time+=50;Bot_EvaluatePerch(exposed);assert(traceCount==16);
 assert(s_botNav[sheltered].lookout>s_botNav[exposed].lookout+200);
 Bot_EvaluatePerch(sheltered);assert(traceCount==16);perchWorld=0;
 // Isolated nodes are not advertised as perches. Registration is bounded
 // and trace-free; within a sector the higher accessible landing survives.
 Bot_RegisterPerch(sheltered);assert(!s_botPerchCount);
 s_botNav[sheltered].edge[0]=exposed;Bot_RegisterPerch(sheltered);assert(s_botPerchCount==1);
 int elevated=Bot_NavAdd((float[3]){0,0,161});s_botNav[elevated].edge[0]=exposed;
 Bot_RegisterPerch(elevated);assert(s_botPerches[0]==elevated && s_botPerchCount==1 && traceCount==16);
 // Guarding an objective can choose an elevated firing position outside the
 // infantry ring without moving the objective or bypassing route finding.
 svs.time+=50;SV_BotEnableAI(0,2);actor=&s_botAI[0];
 actor->objectiveUntil=svs.time+10000;actor->objectiveBehavior=1;
 actor->objectiveRadius=160;actor->objective[2]=1;
 Bot_GuardPoint(0,actor,&clients[0].ps);assert(actor->guardPoint[2]==161);
 actor->goal=sheltered;Bot_StartDetour(actor);
 assert(actor->rejectedGoal==elevated);
 actor->detourUntil=actor->rejectedUntil=0;
 // Visible distant enemies do not make a perched sniper walk off the roof.
 actor->objectiveUntil=0;actor->goal=actor->patrolGoal=sheltered;
 actor->perchUntil=svs.time+10000;actor->memoryUntil=svs.time+1000;
 actor->target=1;actor->acquiredAt=svs.time-1000;actor->nextThink=actor->nextPath=svs.time+10000;
 actor->guardAt=0;actor->aim[0]=actor->aim[1]=0;
 clients[0].ps.origin[0]=clients[0].ps.origin[1]=0;clients[0].ps.origin[2]=1;
 clients[0].ps.fWeaponPosFrac=1;clients[0].ps.weapon=1;
 clients[1].ps.origin[0]=1600;clients[1].ps.origin[1]=0;clients[1].ps.origin[2]=1;
 clients[1].sess.cs.team=TEAM_AXIS;clients[1].sess.sessionState=SESS_STATE_PLAYING;
 g_entities[0].health=g_entities[1].health=100;
 memcpy(actor->lastSeen,clients[1].ps.origin,sizeof(vec3_t));
 s_botNavSeeded=1;s_botNavFrame=svs.time;SV_BotRunAI(0,&clients[0].ps);
 assert(!s_botCmdState[0].forwardmove && !s_botCmdState[0].rightmove && (s_botCmdState[0].buttons&0x200));
 assert(fabsf(actor->guardYaw)<=45);
 int idleSearches=s_botWorkPaths;Bot_BuildPath(actor,&clients[0].ps);
 assert(s_botWorkPaths==idleSearches && !actor->routeCount);
 // Holding an overwatch slot is free, but a position on the floor below
 // must not be accepted merely because its XY coordinates match.
 actor->objectiveUntil=svs.time+10000;actor->objectiveBehavior=1;
 memcpy(actor->guardPoint,clients[0].ps.origin,sizeof(vec3_t));
 Bot_BuildPath(actor,&clients[0].ps);assert(s_botWorkPaths==idleSearches);
 actor->guardPoint[2]+=64;actor->guardUntil=svs.time+10000;
 memcpy(actor->guardAnchor,actor->objective,sizeof(vec3_t));
 actor->perchUntil=0;actor->waypoint=elevated;actor->route[0]=elevated;actor->routeCount=1;
 actor->routeIndex=0;actor->routeFrom=sheltered;
 SV_BotRunAI(0,&clients[0].ps);
 assert(!(s_botCmdState[0].buttons&0x200));
 actor->objectiveUntil=0;actor->perchUntil=svs.time+10000;
 // Incoming damage releases the position once, avoids its area for 15s and
 // does not repeatedly destroy the replacement route during the cooldown.
 g_entities[0].health=90;Bot_Think(0,actor,&clients[0].ps);
 assert(!actor->perchUntil && actor->rejectedGoal==sheltered && actor->rejectedUntil>svs.time);
 actor->patrolGoal=exposed;Bot_RepositionSniper(actor);assert(actor->patrolGoal==exposed);
 svs.time+=6000;actor->objectiveUntil=svs.time+10000;actor->objectiveBehavior=3;
 Bot_RepositionSniper(actor);assert(actor->patrolGoal==exposed);
 actor->objectiveBehavior=2;Bot_RepositionSniper(actor);assert(actor->patrolGoal==exposed);
 // Distant contact memory cannot replace a committed sniper route, while
 // an active objective still takes precedence over that same perch.
 actor->objectiveUntil=0;actor->perchUntil=0;actor->routeCount=0;actor->goal=-1;
 actor->patrolGoal=exposed;actor->patrolUntil=svs.time+10000;actor->memoryUntil=svs.time+5000;
 Bot_FindPath(actor,&clients[0].ps);assert(actor->planKind==0 && actor->goal==exposed);
 actor->objectiveUntil=svs.time+10000;actor->objectiveBehavior=2;actor->objectiveEntity=-1;
 memcpy(actor->objective,s_botNav[exposed].origin,sizeof(vec3_t));
 Bot_FindPath(actor,&clients[0].ps);assert(actor->planKind==2);
 // Close visible threats also abandon a compromised perch without damage.
 actor->objectiveUntil=0;actor->perchUntil=svs.time+10000;actor->sniperRepositionAt=0;
 actor->nextPath=svs.time+10000;clients[1].ps.origin[0]=128;
 Bot_Think(0,actor,&clients[0].ps);assert(!actor->perchUntil && actor->sniperRepositionAt>svs.time);
 actor->perchUntil=svs.time+18000;
 assert(SV_BotSetGoal(0,(float[3]){512,0,1},12,2,-1));
 assert(!actor->perchUntil); // A new interaction must not wait out overwatch.
 actor->objectiveUntil=0;actor->perchUntil=svs.time+18000;actor->nextCover=0;actor->target=1;
 clients[0].ps.origin[0]=384;actor->lastSeen[0]=actor->lastSeen[1]=0;actor->lastSeen[2]=1;
 wall=1;Bot_FindCover(actor,&clients[0].ps);wall=0;
 assert(actor->coverUntil>svs.time && !actor->perchUntil);
 weapon.overlayReticle=0;g_entities[0].health=100;
 puts("PASS: sheltered sniper sightlines, cached tactical budget, accessible height catalogue, elevated objective overwatch, stable crouched perch, damage relocation and critical objective priority");

 // On a fully connected 9,216-node map, A* takes the diagonal corridor;
 // breadth-first search used to visit essentially the whole arena.
 SV_BotResetNavigation();svs.time+=5000;clients[0].ps.origin[0]=0;
 for(int x=0;x<96;x++)for(int y=0;y<96;y++)Bot_NavAdd((float[3]){x*32,y*32,1});
 for(int x=0;x<96;x++)for(int y=0;y<96;y++)for(int d=0;d<8;d++) {
  int nx=x+s_botDirections[d][0],ny=y+s_botDirections[d][1];
  if(nx>=0&&nx<96&&ny>=0&&ny<96)s_botNav[x*96+y].edge[d]=nx*96+ny;
 }
 SV_BotEnableAI(0,1);SV_BotSetGoal(0,(float[3]){3040,3040,1},12,0,-1);
 Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(s_botAI[0].goal==9215 && s_botPathExpanded<256 && s_botAI[0].routeCount==BOT_ROUTE_MAX);
#ifdef BOT_BENCHMARK
 clock_t measured=clock();int runs=1000;
 for(i=0;i<runs;i++) {
  svs.time+=50;s_botAI[0].objectiveUntil=svs.time+1600;
  s_botAI[0].detourUntil=s_botAI[0].failedPaths=0;
  s_botAI[0].routeCount=0;Bot_FindPath(&s_botAI[0],&clients[0].ps);
 }
 printf("BENCH: %d searches on %d nodes: %.3f ms/search, %.1f expanded/search; caps=%d searches + %d nav attempts/frame\n",
  runs,s_botNavCount,(double)(clock()-measured)*1000/CLOCKS_PER_SEC/runs,
  (double)s_botPathExpanded/s_botPathSearches,BOT_PATHS_PER_FRAME,BOT_NAV_EDGES_PER_FRAME);
 // Dense traffic benchmark: all 31 peers reserve the same 48 cells. Route
 // costs remain one O(1) lookup per edge, even with every bot in one lane.
 level.maxclients=32;
 for(int peer=1;peer<32;peer++) {
  clients[peer].sess.cs.team=TEAM_ALLIES;clients[peer].sess.sessionState=SESS_STATE_PLAYING;
  g_entities[peer].health=100;SV_BotEnableAI(peer,1);
  s_botAI[peer].routeCount=BOT_TRAFFIC_ROUTE_NODES;
  for(int step=0;step<BOT_TRAFFIC_ROUTE_NODES;step++)s_botAI[peer].route[step]=(short)((step+1)*97);
 }
 measured=clock();int trafficExpanded=s_botPathExpanded;
 for(i=0;i<runs;i++) {
  svs.time+=50;s_botAI[0].objectiveUntil=svs.time+1600;
  s_botAI[0].detourUntil=s_botAI[0].failedPaths=0;
  s_botAI[0].routeCount=0;Bot_FindPath(&s_botAI[0],&clients[0].ps);
 }
 printf("BENCH: 32 bots reserving one lane: %.3f ms/search, %.1f expanded/search; reservation storage=%zu KiB\n",
  (double)(clock()-measured)*1000/CLOCKS_PER_SEC/runs,
  (double)(s_botPathExpanded-trafficExpanded)/runs,(sizeof(s_botTrafficStamp)+sizeof(s_botTraffic)+sizeof(s_botTrafficSectors))/1024);
 level.maxclients=2;clients[1].sess.cs.team=TEAM_AXIS;
#endif
 // A disconnected destination cannot turn A* back into an unbounded flood.
 // Reinitialize after the stationary benchmark; it may enter a detour.
 SV_BotEnableAI(0,1);
 svs.time+=50;s_botAI[0].routeCount=0;
 SV_BotSetGoal(0,(float[3]){9000,9000,1},12,0,-1);
 int expandedBefore=s_botPathExpanded;
 Bot_FindPath(&s_botAI[0],&clients[0].ps);
 assert(s_botPathExpanded-expandedBefore<=BOT_PATH_NODES);
 assert(s_botPathExpanded-expandedBefore>1000);
 // Wall-time exhaustion also defers later bots without erasing their route.
 svs.time+=50;s_botAI[0].routeCount=0;navClock=1;
 Bot_FindPath(&s_botAI[0],&clients[0].ps);navClock=0;
 assert(s_botWorkPathMs>=BOT_PATH_TIME_MS);
 int count=s_botWorkPaths;s_botAI[1].routeCount=0;
 Bot_FindPath(&s_botAI[1],&clients[1].ps);assert(s_botWorkPaths==count);
 puts("PASS: offset doors, shared 32-bot budgets/fair admission, stable routes, moving goals, circular-motion recovery, observed team reports, bounded A* map search");

 // Balance regression: Normal keeps tactical decisions but has slower first
 // shots and less accurate attack commands; Easy must not become deadlier.
 float rifleError[3]={0},scopeError[3]={0};int misses[3]={0};
 for(int d=0;d<3;d++)for(unsigned int seed=1;seed<=64;seed++) {
  float yaw;int first=combatSample(d,0,600,seed*7919,&yaw);
  assert(first>=(d==0?700:d==1?650:200));rifleError[d]+=fabsf(yaw);
  first=combatSample(d,1,128,seed*7919,&yaw);
  assert(first>=(d==0?1350:d==1?1100:200));scopeError[d]+=fabsf(yaw);
  if(fabsf(tanf(yaw/BOT_RAD_TO_DEG)*128)>15)misses[d]++;
 }
 assert(rifleError[1]>rifleError[2]*3 && scopeError[1]>scopeError[2]*8);
 assert(misses[1]>=16 && misses[2]==0 && scopeError[0]>scopeError[1]);
 printf("PASS: Normal balance, 64 seeds: mean rifle yaw error %.2f vs Hard %.2f degrees; close scoped horizontal misses %d/64 vs Hard %d/64 (command geometry, not match hit rate)\n",
  rifleError[1]/64,rifleError[2]/64,misses[1],misses[2]);
 float yaw;combatSample(1,1,600,7919,&yaw);
 clients[0].ps.weaponTime=200;svs.time+=50;s_botNavFrame=svs.time;
 SV_BotRunAI(0,&clients[0].ps);assert(!s_botAI[0].scopeReadyAt&&!(s_botCmdState[0].buttons&1));
 clients[0].ps.weaponTime=0;svs.time+=50;s_botNavFrame=svs.time;
 SV_BotRunAI(0,&clients[0].ps);assert(s_botAI[0].scopeReadyAt>svs.time&&!(s_botCmdState[0].buttons&1));
 clients[0].ps.fWeaponPosFrac=.5f;svs.time+=50;s_botNavFrame=svs.time;
 SV_BotRunAI(0,&clients[0].ps);assert(!s_botAI[0].scopeReadyAt&&!(s_botCmdState[0].buttons&1));
 clients[0].ps.fWeaponPosFrac=1;svs.time+=50;s_botNavFrame=svs.time;
 SV_BotRunAI(0,&clients[0].ps);assert(s_botAI[0].scopeReadyAt>svs.time);
 wall=1;svs.time+=50;s_botNavFrame=svs.time;
 SV_BotRunAI(0,&clients[0].ps);assert(!s_botAI[0].scopeReadyAt&&!(s_botCmdState[0].buttons&1));wall=0;
 // A stationary objective guard must give an approaching human room without
 // quitting the objective. No scan traces for retreating/other-floor humans.
 SV_BotResetNavigation();svs.time+=5000;humanClient=1;weapon.overlayReticle=0;
 clients[1].sess.cs.team=TEAM_ALLIES;clients[1].ps.origin[0]=-64;
 clients[1].ps.origin[1]=0;clients[1].ps.origin[2]=1;clients[1].ps.velocity[0]=120;
 clients[0].ps.origin[0]=clients[0].ps.origin[1]=0;clients[0].ps.origin[2]=1;
 SV_BotEnableAI(0,1);actor=&s_botAI[0];traceCount=0;
 Bot_HumanCourtesy(actor,&clients[0].ps);
 assert(actor->yieldForHuman && actor->yieldUntil>svs.time && traceCount==1);
 Bot_HumanCourtesy(actor,&clients[0].ps);assert(traceCount==1);
 SV_BotSetGoal(0,clients[0].ps.origin,160,1,-1);
 memcpy(actor->guardPoint,clients[0].ps.origin,sizeof(vec3_t));
 memcpy(actor->guardAnchor,actor->objective,sizeof(vec3_t));
 actor->guardUntil=actor->guardAt=actor->nextThink=actor->nextPath=svs.time+10000;
 actor->guardYaw=actor->aim[1]=0;s_botNavFrame=svs.time;
 Bot_BeginHumanYield(actor,&clients[0].ps,1);assert(!actor->guardUntil);
 actor->guardUntil=svs.time+10000;
 SV_BotRunAI(0,&clients[0].ps);
 assert(fabsf(s_botCmdState[0].rightmove)>60 && !(s_botCmdState[0].buttons&1));
 assert(actor->objectiveUntil>svs.time && actor->objectiveBehavior==1 && clients[0].ps.origin[0]==0);
 courtesyWall=1;SV_BotRunAI(0,&clients[0].ps);courtesyWall=0;
 assert(s_botCmdState[0].rightmove< -60 && actor->yieldYaw>0); // Other side is usable.
 actor->objectiveUntil=0;actor->yieldUntil=actor->nextYield=0;
 clients[1].ps.velocity[0]=-120;traceCount=0;Bot_HumanCourtesy(actor,&clients[0].ps);
 assert(!actor->yieldUntil && traceCount==0);
 clients[1].ps.velocity[0]=120;clients[1].ps.origin[2]=81;
 Bot_HumanCourtesy(actor,&clients[0].ps);assert(!actor->yieldUntil && traceCount==0);
 clients[1].ps.origin[2]=1;actor->objectiveUntil=svs.time+10000;actor->objectiveBehavior=2;
 Bot_HumanCourtesy(actor,&clients[0].ps);assert(!actor->yieldUntil && traceCount==0);
 actor->objectiveUntil=0;clients[0].ps.origin[0]=300;clients[1].ps.origin[0]=236;
 wall=1;Bot_HumanCourtesy(actor,&clients[0].ps);assert(!actor->yieldUntil && traceCount==1);wall=0;
 svs.time+=50;Bot_WorkFrame();s_botWorkTacticalTraces=BOT_TACTICAL_TRACES_PER_FRAME;traceCount=0;
 Bot_HumanCourtesy(actor,&clients[0].ps);assert(!actor->yieldUntil && traceCount==0);
 svs.time+=50;Bot_HumanCourtesy(actor,&clients[0].ps);assert(actor->yieldUntil>svs.time);
 actor->yieldUntil=actor->nextYield=0;humanClient=-1;traceCount=0;
 Bot_HumanCourtesy(actor,&clients[0].ps);assert(!actor->yieldUntil && traceCount==0);
 // Human occupancy participates in guard spacing without extra roster scans.
 SV_BotResetNavigation();svs.time+=5000;humanClient=1;SV_BotEnableAI(0,1);actor=&s_botAI[0];
 clients[0].ps.origin[0]=0;clients[1].ps.origin[0]=1000;
 for(int x=-160;x<=160;x+=32)for(int y=-160;y<=160;y+=32)Bot_NavAdd((float[3]){x,y,1});
 SV_BotSetGoal(0,clients[0].ps.origin,160,1,-1);Bot_GuardPoint(0,actor,&clients[0].ps);
 memcpy(clients[1].ps.origin,actor->guardPoint,sizeof(vec3_t));
 svs.time+=50;actor->guardUntil=0;Bot_GuardPoint(0,actor,&clients[0].ps);
 assert(Bot_DistanceSq(actor->guardPoint,clients[1].ps.origin)>48*48);humanClient=-1;
 // Thirty-two simultaneous wounded bots share cover rays with all other
 // tactical work. Open candidates need no floor sweep; deferred bots resume.
 SV_BotResetNavigation();svs.time+=5000;level.maxclients=32;
 for(int x=-128;x<=128;x+=64)for(int y=-128;y<=128;y+=64)Bot_NavAdd((float[3]){x,y,1});
 for(i=0;i<32;i++) {
  g_entities[i].client=&clients[i];g_entities[i].health=30;
  clients[i].sess.connected=2;clients[i].sess.sessionState=SESS_STATE_PLAYING;
  clients[i].sess.cs.team=TEAM_ALLIES;memcpy(clients[i].ps.origin,(float[3]){0,0,1},sizeof(vec3_t));
  SV_BotEnableAI(i,1);s_botAI[i].target=31;s_botAI[i].lastSeen[0]=1000;
 }
 int coverQueries[32]={0};
 for(int frame=0;frame<20;frame++) {
  svs.time+=50;Bot_WorkFrame();traceCount=0;
  for(i=0;i<32;i++) {
   int before=traceCount;Bot_FindCover(&s_botAI[i],&clients[i].ps);coverQueries[i]+=traceCount-before;
  }
  assert(traceCount<=BOT_TACTICAL_TRACES_PER_FRAME && !s_botWorkEdges);
 }
 for(i=0;i<32;i++)assert(coverQueries[i]>0);
 svs.time+=500;Bot_WorkFrame();s_botWorkNavMs=BOT_NAV_TIME_MS;
 int savedCursor=s_botAI[0].coverCursor;traceCount=0;Bot_FindCover(&s_botAI[0],&clients[0].ps);
 assert(traceCount==0 && s_botAI[0].coverCursor==savedCursor);
 // Partial magazines are topped up only when no current/recent contact or
 // critical interaction competes. The command cannot invent any ammunition.
 SV_BotResetNavigation();svs.time+=5000;level.maxclients=2;SV_BotEnableAI(0,1);actor=&s_botAI[0];
 clients[1].sess.sessionState=SESS_STATE_DEAD;clients[0].ps.weapon=1;clients[0].ps.weaponTime=0;
 clients[0].ps.ammoclip[0]=5;clients[0].ps.ammo[0]=40;weapon.iClipSize=30;
 actor->nextThink=actor->nextPath=svs.time+10000;s_botNavFrame=svs.time;
 SV_BotRunAI(0,&clients[0].ps);
 assert((s_botCmdState[0].buttons&0x10) && clients[0].ps.ammoclip[0]==5 && clients[0].ps.ammo[0]==40);
 actor->memoryUntil=svs.time+1000;SV_BotRunAI(0,&clients[0].ps);assert(!(s_botCmdState[0].buttons&0x10));
 actor->memoryUntil=0;actor->lastVisibleAt=svs.time-2000;
 SV_BotRunAI(0,&clients[0].ps);assert(!(s_botCmdState[0].buttons&0x10));
 actor->lastVisibleAt=0;SV_BotSetGoal(0,clients[0].ps.origin,32,3,-1);
 SV_BotRunAI(0,&clients[0].ps);assert(!(s_botCmdState[0].buttons&0x10));
 actor->objectiveUntil=0;clients[0].ps.ammoclip[0]=20;
 SV_BotRunAI(0,&clients[0].ps);assert(!(s_botCmdState[0].buttons&0x10));
 clients[0].ps.ammoclip[0]=5;clients[0].ps.ammo[0]=0;
 SV_BotRunAI(0,&clients[0].ps);assert(!(s_botCmdState[0].buttons&0x10));
 puts("PASS: human right of way while guarding, wall/floor/interaction exclusions, occupied guard slots, shared 32-bot cover budgets/deferred work, safe proactive reloads");
 // An actual ClientSpawn increments stats[5], even if death was not observed
 // between server ticks. Pending work and cached movement belong to that life.
 SV_BotResetNavigation();svs.time+=5000;level.maxclients=2;
 clients[0].sess.sessionState=SESS_STATE_PLAYING;g_entities[0].health=100;
 clients[1].sess.sessionState=SESS_STATE_DEAD;clients[0].ps.weapon=0;
 SV_BotEnableAI(0,1);actor=&s_botAI[0];
 Bot_NavAdd((float[3]){0,0,1});Bot_NavAdd((float[3]){32,0,1});
 actor->routeFrom=0;actor->route[0]=1;actor->routeCount=1;actor->waypoint=1;
 actor->target=actor->lastTarget=1;actor->observedTarget=1;actor->observedVisible=1;
 actor->observedFrame=svs.time;actor->memoryUntil=svs.time+3000;
 actor->nextThink=actor->nextPath=actor->nextTraversalJump=svs.time+9000;
 actor->approachUntil=actor->avoidUntil=svs.time+3000;actor->blockedUntil[0]=svs.time+8000;
 actor->recentCount=8;actor->pathPending=1;actor->lastOrigin[0]=-1000;
 clients[0].ps.stats[5]++;clients[0].ps.origin[0]=1024;
 s_botNavSeeded=1;s_botNavFrame=svs.time;SV_BotPlanFrame();
 assert(!actor->pathPending && !s_botPathSearches); // No obsolete route search.
 SV_BotRunAI(0,&clients[0].ps);
 assert(actor->spawnCount==clients[0].ps.stats[5] && actor->difficulty==1 && actor->enabled);
 assert(actor->target==-1 && actor->lastTarget==-1 && !actor->memoryUntil);
 assert(actor->waypoint==-1 && !actor->routeCount && !actor->blockedUntil[0]);
 assert(!actor->approachUntil && !actor->avoidUntil && !actor->nextTraversalJump);
 assert(actor->recentCount<=1 && actor->lastOrigin[0]==1024 && actor->lastMoveCheck==svs.time);
 assert(actor->pathPending && actor->nextThink<=svs.time+200);
 assert(!(s_botCmdState[0].buttons&1));
 // Spectator/dead periods never resume navigation or bypass original respawn rules.
 clients[0].sess.sessionState=SESS_STATE_DEAD;svs.time+=50;
 SV_BotRunAI(0,&clients[0].ps);
 assert(s_botCmdState[0].buttons==8 && !actor->pathPending && !actor->wantsMove);
 clients[0].sess.sessionState=SESS_STATE_SPECTATOR;SV_BotRunAI(0,&clients[0].ps);
 assert(s_botCmdState[0].buttons==8 && !s_botCmdState[0].forwardmove);
 clients[0].sess.cs.team=TEAM_SPECTATOR;SV_BotRunAI(0,&clients[0].ps);
 assert(!s_botCmdState[0].buttons && !s_botCmdState[0].forwardmove);
 clients[0].sess.cs.team=TEAM_ALLIES;clients[0].sess.sessionState=3;
 SV_BotRunAI(0,&clients[0].ps);assert(!s_botCmdState[0].buttons);
 // An observed inactive-to-live transition resets even with a restored counter.
 clients[0].sess.sessionState=SESS_STATE_PLAYING;
 clients[0].ps.origin[0]=2048;svs.time+=500;s_botNavFrame=svs.time;
 SV_BotRunAI(0,&clients[0].ps);
 assert(actor->lastOrigin[0]==2048 && actor->pathPending && !actor->stuckCount);
 puts("PASS: original spawn counter resets stale routes/perception/cooldowns, rejects old pending work, respects death/spectator rules");
 // Held by freezeControls until a match begins: a new spawn starts its fresh
 // life at once, then it neither moves, presses nor turns and its clocks keep
 // up, so the release resumes as a new spawn would.
 clients[0].bFrozen=1;++clients[0].ps.stats[5];
 clients[0].ps.viewangles[0]=12;clients[0].ps.viewangles[1]=34;
 s_botCmdState[0].hasAngles=1;
 for(i=0;i<40;i++){
  svs.time+=50;SV_BotRunAI(0,&clients[0].ps);
  assert(!s_botCmdState[0].buttons && !s_botCmdState[0].forwardmove && !s_botCmdState[0].rightmove &&
         !s_botCmdState[0].hasAngles && !s_botCmdState[0].hasWeapon);
  assert(actor->spawnCount==clients[0].ps.stats[5] && actor->lastMoveCheck==svs.time && actor->lastFrame==svs.time &&
         actor->nextThink==svs.time && actor->aim[0]==12 && actor->aim[1]==34 && !actor->pathPending &&
         !actor->wantsMove && !actor->stuckCount && actor->target==-1);
 }
 clients[0].bFrozen=0;svs.time+=50;s_botNavFrame=svs.time;SV_BotRunAI(0,&clients[0].ps);
 assert(actor->spawnCount==clients[0].ps.stats[5] && actor->pathPending && !actor->stuckCount &&
        s_botCmdState[0].hasAngles);
 puts("PASS: a bot held by freezeControls stands still on a fresh life and resumes as from a new spawn");
 SV_BotResetAIClient(0);assert(!s_botAI[0].enabled);
 puts("PASS: bounded/hash navigation, enemy teams, fair perception/memory, difficulty, route progress/recovery, guard spacing, ladders/safe directed drops/time budget, jump tracking/scoped ADS, weapon-range pursuit, friendly-fire avoidance, objective priority/expiry, original use holds, reload/cadence, death/reset");
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-bot-ai-') as directory:
    path = Path(directory)
    # Keep this external API definition after production code: a missing or
    # incorrect declaration must fail here instead of being hidden by the stub.
    api_support = '\nfloat Dvar_GetFloat(const char *name){return !strcmp(name,"jump_height")?39:fallMin;}\n'
    (path/'test.c').write_text(support + source + api_support + checks)
    flags = (['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g'] if args.sanitize else []) + (['-DBOT_BENCHMARK'] if args.benchmark else [])
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-O2',*flags,str(path/'test.c'),'-lm','-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True,timeout=60)
