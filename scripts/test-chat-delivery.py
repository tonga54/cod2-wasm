#!/usr/bin/env python3
"""Run actual server chat routing with sparse/reconnected client slots."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/game_mp/g_cmds_mp.c').read_text()


def extract(signature):
    start = re.search(re.escape(signature) + r'\s*\{', source).start()
    cursor = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[cursor] == '{') - (source[cursor] == '}')
        cursor += 1
    return source[start:cursor]


body = extract('static void G_SayTo(gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message)')
body += extract('void G_Say(gentity_t *ent, gentity_t *target, int mode, const char *chatText)')
support = r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
enum {CON_CONNECTED=2,SESS_STATE_PLAYING=0,TEAM_ALLIES=1,TEAM_AXIS=2,TEAM_SPECTATOR=3,SV_CMD_CAN_IGNORE=0};
typedef struct {struct {int connected,sessionState;struct {int team;char name[64];}cs;}sess;} client_t;
typedef struct {struct{int inuse;}r;client_t *client;struct{int number;}s;} gentity_t;
static gentity_t g_entities[64];static client_t clients[64];
static struct{int numConnectedClients,sortedClients[64];}level;
typedef struct{struct{int enabled,integer;}current;}dvar_t;
static dvar_t dead, dedicated;static const dvar_t *dead_ptr=&dead,*dedicated_ptr=&dedicated;
static void *imp_g_deadChat=&dead_ptr,*imp_g_dedicated=&dedicated_ptr;
static int recipients[64],count;
static int OnSameTeam(gentity_t *a,gentity_t *b){return a->client->sess.cs.team==b->client->sess.cs.team;}
static void SV_GameSendServerCommand(int slot,int mode,const char *text){
 assert(slot>=0 && slot<64 && count<64 && mode==SV_CMD_CAN_IGNORE);assert(text[0]=='h' || text[0]=='i');recipients[count++]=slot;}
static const char *va(const char *format,...){static char buffer[1024];va_list args;va_start(args,format);vsnprintf(buffer,sizeof(buffer),format,args);va_end(args);return buffer;}
static void Com_sprintf(char *buffer,int size,const char *format,...){va_list args;va_start(args,format);vsnprintf(buffer,size,format,args);va_end(args);}
static void I_strncpyz(char *to,const char *from,int n){snprintf(to,n,"%s",from);}
static void I_CleanStr(char *s){}
static void G_LogPrintf(const char *format,...){}
static void Com_Printf(const char *format,...){}
static int SV_GetGuid(int n){return n;}
static void reset(void){memset(g_entities,0,sizeof(g_entities));memset(clients,0,sizeof(clients));memset(&level,0,sizeof(level));count=0;dead.current.enabled=1;}
static void connect(int slot,int team,int playing){g_entities[slot].r.inuse=1;g_entities[slot].client=&clients[slot];g_entities[slot].s.number=slot;
 clients[slot].sess.connected=CON_CONNECTED;clients[slot].sess.cs.team=team;clients[slot].sess.sessionState=playing?0:1;
 snprintf(clients[slot].sess.cs.name,64,"Player %d",slot);level.sortedClients[level.numConnectedClients++]=slot;}
'''
checks = r'''
int main(void){
 for(int first=0;first<64;first++)for(int second=0;second<64;second++)if(first!=second){
  reset();connect(second,TEAM_AXIS,1);connect(first,TEAM_ALLIES,1);
  G_Say(&g_entities[first],NULL,0,"global chat");assert(count==2 && recipients[0]==second && recipients[1]==first);
  count=0;G_Say(&g_entities[first],NULL,1,"team chat");assert(count==1 && recipients[0]==first);
  count=0;G_Say(&g_entities[first],&g_entities[second],2,"target chat");assert(count==1 && recipients[0]==second);
 }
 reset();for(int i=63;i>=0;i--)connect(i,i%2?TEAM_ALLIES:TEAM_AXIS,1);
 G_Say(&g_entities[63],NULL,0,"all players");assert(count==64);for(int i=0;i<64;i++)assert(recipients[i]==63-i);
 reset();connect(63,TEAM_SPECTATOR,0);connect(17,TEAM_ALLIES,1);connect(42,TEAM_SPECTATOR,0);dead.current.enabled=0;
 G_Say(&g_entities[63],NULL,0,"spectator chat");assert(count==2 && recipients[0]==63 && recipients[1]==42);
 count=0;g_entities[42].r.inuse=0;G_Say(&g_entities[63],NULL,0,"inactive slot");assert(count==1 && recipients[0]==63);
 puts("PASS: 4032 sparse/reconnected slot pairs, global/team/target chat, all 64 slots, spectators and disconnected recipients");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-chat-delivery-') as directory:
    folder = Path(directory)
    for mutant in (False, True):
        actual = body.replace('&g_entities[level.sortedClients[j]]', '&g_entities[j]') if mutant else body
        (folder / 'test.c').write_text(support + actual + checks)
        subprocess.run(['cc', '-O1', '-fsanitize=address,undefined', str(folder / 'test.c'), '-o', str(folder / 'test')], check=True)
        result = subprocess.run([str(folder / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) != mutant, result.stderr
        if not mutant: print(result.stdout, end='')
print('PASS: broadcasting to the first N slots instead of connected players fails')
