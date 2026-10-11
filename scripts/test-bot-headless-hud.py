#!/usr/bin/env python3
"""Exercise original human health/name labels and the headless-only shortcut."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/game_mp/g_team_mp.c').read_text()
start = source.index('void TeamplayInfoMessage(gentity_t *ent)\n{')
end = source.index('\nvoid CheckTeamStatus', start)
body = source[start:end]
support = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef int qboolean;typedef float vec_t;typedef float vec3_t[3];
#define SESS_STATE_PLAYING 0
#define STAT_IDENT_CLIENT_NUM 3
#define STAT_IDENT_CLIENT_HEALTH 4
typedef struct {struct {int sessionState;struct {int team;}cs;}sess;
 struct {int stats[5],clientNum;float viewHeightCurrent;}ps;}gclient_t;
typedef struct {gclient_t *client;int health;struct {int number;}s;}gentity_t;
typedef struct {int entityNum;}trace_t;
static gentity_t entities[4];static gclient_t clients[4];
static void *imp_g_entities=entities;
static vec3_t zero;static vec3_t *trace_mins=&zero;
static int traces;
static qboolean SV_BotIsTestClient(int num){return num==2;}
static void G_GetPlayerViewOrigin(gentity_t *ent,vec3_t origin){memset(origin,0,sizeof(vec3_t));}
static void G_GetPlayerViewDirection(gentity_t *ent,vec3_t forward,vec3_t right,vec3_t up){forward[0]=1;forward[1]=forward[2]=0;}
static int G_IsPlaying(gentity_t *ent){return ent->client->sess.sessionState==0;}
static void G_TraceCapsule(trace_t *trace,vec_t *start,vec_t *mins,vec_t *maxs,vec_t *end,int pass,int mask){
 assert(mask==0x2000001&&end[0]==8192);trace->entityNum=1;traces++;
}
'''
checks = r'''
int main(void){
 for(int i=0;i<4;i++){entities[i].client=&clients[i];entities[i].s.number=clients[i].ps.clientNum=i;clients[i].sess.cs.team=1;}
 entities[1].health=66;
 TeamplayInfoMessage(&entities[0]);assert(traces==1&&clients[0].ps.stats[3]==1&&clients[0].ps.stats[4]==66);
 clients[2].ps.stats[3]=1;clients[2].ps.stats[4]=66;
 TeamplayInfoMessage(&entities[2]);assert(traces==1&&clients[2].ps.stats[3]==-1&&clients[2].ps.stats[4]==0);
 clients[1].sess.cs.team=2;TeamplayInfoMessage(&entities[0]);
 assert(traces==2&&clients[0].ps.stats[3]==-1&&clients[0].ps.stats[4]==0);
 clients[0].sess.sessionState=1;TeamplayInfoMessage(&entities[0]);assert(traces==3&&clients[0].ps.stats[4]==66);
 clients[0].sess.sessionState=0;clients[0].sess.cs.team=0;
 TeamplayInfoMessage(&entities[0]);assert(traces==3&&clients[0].ps.stats[3]==-1);
 puts("PASS: human team/spectator/DM name and health labels unchanged; headless bots skip the unused 8192-unit HUD trace");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-bot-hud-') as directory:
    path = Path(directory)
    (path / 'test.c').write_text(support + body + checks)
    subprocess.run(['cc', '-std=c99', '-O2', '-fsanitize=undefined',
                    str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True, timeout=10)
