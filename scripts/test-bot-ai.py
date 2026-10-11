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
#include <string.h>
#include <stdarg.h>
#include <time.h>
typedef int qboolean; typedef unsigned char byte; typedef float vec_t; typedef float vec3_t[3];
#define COD2_BOT_MAX_CLIENTS 64
#define SESS_STATE_PLAYING 0
#define SESS_STATE_DEAD 1
#define TEAM_FREE 0
#define TEAM_AXIS 1
#define TEAM_ALLIES 2
#define WEAPCLASS_SMG 2
#define WEAPCLASS_MG 1
typedef struct { float fraction; vec3_t normal; unsigned short entityNum; int startsolid,allsolid,surfaceFlags,contents; } trace_t;
typedef struct { vec3_t origin,viewangles,velocity; float viewHeightCurrent,fWeaponPosFrac; int pm_type,weapon,weaponTime,pm_flags,groundEntityNum,cursorHintEntIndex,ammoclip[128],ammo[128];char weaponslots[8]; } playerState_t;
typedef struct { playerState_t ps; vec3_t damage_from; int damage_fromWorld; struct {int connected,sessionState; struct {int team;}cs;}sess;} gclient_t;
typedef struct {gclient_t *client; int health; unsigned classname; struct {int inuse;vec3_t currentOrigin,absmin,absmax;}r;} gentity_t;
typedef struct { int maxclients,num_entities; } level_locals_t;
typedef struct {const char *szInternalName; int iClipIndex,iAmmoIndex,bSemiAuto,overlayReticle,weapClass;} WeaponDef;
typedef struct {int buttons,hasWeapon,hasAngles,weapon; signed char forwardmove,rightmove; vec3_t angles;} cod2BotCommandState_t;
static struct {int time;}svs;
static cod2BotCommandState_t s_botCmdState[64];
level_locals_t level={3,3}; gentity_t g_entities[64];
static gclient_t clients[64]; static WeaponDef weapon={"test",0,0,0,0,0},secondary={"secondary",1,1,0,0,0};
static int traceCount, wall, wallDoor, smoke, friendly,crowd,stairs,ladderWorld,navClock,dropWorld;
static float fallMin=256;
int Sys_Milliseconds(void){return navClock?navClock++:0;}
static int Dvar_GetInt(const char *name){(void)name;return 0;}
static float Dvar_GetFloat(const char *name){return !strcmp(name,"jump_height")?39:fallMin;}
static playerState_t *SV_GameClientNum(int num){return &clients[num].ps;}
static int SV_BotIsTestClient(int num){return num>=0&&num<level.maxclients;}
static void Com_Printf(const char *fmt,...){(void)fmt;}
const char *SL_ConvertToString(unsigned int value){(void)value;return "mp_tdm_spawn";}
WeaponDef *BG_GetWeaponDef(int index){return index==2?&secondary:&weapon;}
float SV_FX_GetVisibility(const vec_t *a,const vec_t *b){(void)a;(void)b;return smoke?0:1;}
qboolean SV_EntityContact(const vec_t *mins,const vec_t *maxs,const gentity_t *ent){
 for(int i=0;i<3;i++)if(mins[i]>=ent->r.absmax[i]||maxs[i]<=ent->r.absmin[i])return 0;
 return 1;
}
void G_TraceCapsule(trace_t *t,const vec_t *a,const vec_t *mi,const vec_t *ma,const vec_t *b,int pass,int mask){
 (void)mi;(void)ma;(void)pass;(void)mask;traceCount++;t->fraction=1;t->normal[2]=1;t->startsolid=t->allsolid=t->contents=0;
 if(crowd && (mask&0x02000000)){t->startsolid=1;t->contents=0x02000000;t->fraction=0;return;}
 float floor=stairs?floorf(b[0]/16.0f)*12.0f:ladderWorld&&b[0]>=32?152:0;
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
 memcpy(clients[0].ps.origin,s_botNav[top].origin,sizeof(vec3_t));
 svs.time+=200;SV_BotSetGoal(0,s_botNav[bottom].origin,12,0,-1);
 Bot_FindPath(&s_botAI[0],&clients[0].ps);clients[0].ps.pm_flags=0x20;
 s_botNav[top].edge[BOT_NAV_TRAVERSAL+4]=(short)bottom;
 s_botNavFrame=svs.time;SV_BotRunAI(0,&clients[0].ps);
 assert(s_botCmdState[0].forwardmove==-100&&!(s_botCmdState[0].buttons&1));
 assert(fabsf(Bot_AngleDelta(s_botNav[top].ladderYaw,s_botCmdState[0].angles[1]))<.001f);
 clients[0].ps.pm_flags=0;ladderWorld=0;
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
 // Finishing a tiny projected route cannot suppress recovery forever when
 // the strategic objective is outside the reachable component.
 SV_BotResetNavigation();s_botNavBounded=1;
 s_botNavMin[0]=160;s_botNavMax[0]=256;s_botNavMin[1]=s_botNavMax[1]=0;
 Bot_NavAdd((float[3]){224,0,1});Bot_NavAdd((float[3]){256,0,1});
 Bot_NavAdd((float[3]){192,0,1});s_botNav[0].edge[0]=1;s_botNav[0].edge[4]=2;
 s_botNav[1].edge[4]=s_botNav[2].edge[0]=0;
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
 assert(s_botAI[0].goal==last&&s_botAI[0].failedPaths==0);
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
  s_botAI[0].routeCount=0;Bot_FindPath(&s_botAI[0],&clients[0].ps);
 }
 printf("BENCH: %d searches on %d nodes: %.3f ms/search, %.1f expanded/search; caps=%d searches + %d nav attempts/frame\n",
  runs,s_botNavCount,(double)(clock()-measured)*1000/CLOCKS_PER_SEC/runs,
  (double)s_botPathExpanded/s_botPathSearches,BOT_PATHS_PER_FRAME,BOT_NAV_EDGES_PER_FRAME);
#endif
 // A disconnected destination cannot turn A* back into an unbounded flood.
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

 SV_BotResetAIClient(0);assert(!s_botAI[0].enabled);
 puts("PASS: bounded/hash navigation, enemy teams, fair perception/memory, difficulty, route progress/recovery, guard spacing, ladders/safe directed drops/time budget, jump tracking/scoped ADS, weapon-range pursuit, friendly-fire avoidance, objective priority/expiry, original use holds, reload/cadence, death/reset");
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-bot-ai-') as directory:
    path = Path(directory)
    (path/'test.c').write_text(support + source + checks)
    flags = (['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g'] if args.sanitize else []) + (['-DBOT_BENCHMARK'] if args.benchmark else [])
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-O2',*flags,str(path/'test.c'),'-lm','-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True,timeout=60)
