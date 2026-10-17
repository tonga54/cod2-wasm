#!/usr/bin/env python3
"""Exercise the server's broadcast and the actual browser obituary queue."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parent.parent

def extract(path,name):
 s=(root/path).read_text();start=s.index(name+'(')
 while s.find(';',start)<s.find('{',start):start=s.index(name+'(',start+len(name))
 start=s.rfind('\n',0,start)+1;end=s.index('{',start)+1;depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
def block(path,opening):
 s=(root/path).read_text();start=s.index(opening);end=s.index('{',start)+1;depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
source=(root/'src/PC/cgame_mp/cg_event_mp.c').read_text()
start=source.index('static qboolean deliveringObituary;');end=source.index('\n#endif',start)
queue=source[start:end]
command=block('src/PC/cgame_mp/cg_servercmds_mp.c','    if (!strcmp(cmd, "cod2_obituary")) {')
assert command.count('#ifdef __EMSCRIPTEN__\n')==1 and command.count('#endif\n')==1
parse='static void serverCommand(void){\n const char *cmd=CG_Argv(0);\n'+command.replace('#ifdef __EMSCRIPTEN__\n','').replace('#endif\n','')+'\n}\n'
support=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
typedef int qboolean;typedef float vec3_t[3];
typedef struct {int otherEntityNum,attackerEntityNum,eventParm;}entityState_t;
typedef struct {entityState_t nextState;}centity_t;
static int printed[64],local;
static void CG_EntityEvent(centity_t *event,int code);
static int argc;static char argv[8][32];
static int Cmd_Argc(void){return argc;}
static const char *CG_Argv(int i){return i<argc?argv[i]:"";}
'''
server=r'''
typedef struct {struct {int number,otherEntityNum,attackerEntityNum,eventParm;}s;struct {int svFlags;}r;}gentity_t;
static vec3_t zero;static void *imp_vec3_origin=&zero;static gentity_t event,victim={.s={63}},attacker={.s={62}};
static int broadcasts,destination,reliability,weaponIndex=17,modIndex=1;static char message[100],messages[4][100];
static int G_GetWeaponIndexForName(const char*s){return weaponIndex;}
static const char* Scr_GetString(int i){return "sten_mp";}
static int G_IndexForMeansOfDeath(const char*s){return modIndex;}
static gentity_t *Scr_GetEntity(int i){return i?&attacker:&victim;}
static gentity_t *G_TempEntity(float *p,int type){assert(type==198);memset(&event,0,sizeof(event));return &event;}
static int Scr_GetType(int i){return 1;}static int Scr_GetPointerType(int i){return 21;}
static const char *va(const char*fmt,...){static char text[100];va_list a;va_start(a,fmt);vsnprintf(text,100,fmt,a);va_end(a);return text;}
static void SV_GameSendServerCommand(int client,int type,const char*text){
 destination=client;reliability=type;assert(broadcasts<4);strcpy(messages[broadcasts++],text);strcpy(message,text);
}
'''
checks=r'''
static int recording,seen,seenParm[8];
static void CG_EntityEvent(centity_t *event,int code){
 assert(deliveringObituary&&code==198);
 assert(event->nextState.otherEntityNum==63&&event->nextState.attackerEntityNum==62);
 if(recording){assert(seen<8);seenParm[seen++]=event->nextState.eventParm;return;}
 assert(event->nextState.eventParm==17);
 printed[local]++;
}
/* What the client does with each broadcast it receives. */
static void receive(const char *line){
 char copy[100];argc=0;snprintf(copy,sizeof(copy),"%s",line);
 for(char *t=strtok(copy," ");t;t=strtok(NULL," ")){assert(argc<8);snprintf(argv[argc++],32,"%s",t);}
 serverCommand();
}
int main(void){
 GScr_Obituary();assert(broadcasts==1&&destination==-1&&reliability==1&&event.r.svFlags==8);assert(!strcmp(message,"cod2_obituary 63 62 17"));
 /* A headshot carries MOD_HEAD_SHOT in the same three arguments, with or
  * without a weapon. */
 broadcasts=0;modIndex=8;GScr_Obituary();
 assert(broadcasts==1&&reliability==1&&!strcmp(messages[0],"cod2_obituary 63 62 136")&&event.s.eventParm==136);
 broadcasts=0;weaponIndex=0;GScr_Obituary();assert(broadcasts==1&&!strcmp(messages[0],"cod2_obituary 63 62 136"));
 broadcasts=0;weaponIndex=17;modIndex=7;GScr_Obituary();assert(broadcasts==1&&!strcmp(messages[0],"cod2_obituary 63 62 135"));
 for(local=0;local<64;local++){
  CG_QueueObituary(64,62,17);CG_QueueObituary(63,64,17);CG_QueueObituary(63,62,256);CG_FlushObituaries();assert(!printed[local]);
  for(int i=0;i<10;i++)receive("cod2_obituary 63 62 17");
  CG_FlushObituaries();assert(printed[local]==10&&!deliveringObituary);
  CG_FlushObituaries();assert(printed[local]==10);
 }
 recording=1;
 /* A fourth argument, the weapon some servers add to a headshot, is ignored;
  * other counts and invalid events are dropped. */
 receive("cod2_obituary 63 62 136 17");receive("cod2_obituary 63 62 136");
 receive("cod2_obituary 63 62 136 17 9");receive("cod2_obituary 63 62");receive("cod2_obituary 64 62 136 21");
 receive("cod2_obituary 63 62 135 0");receive("cod2_obituary 63 62 255 3");receive("cod2_obituary 63 62 256");
 CG_FlushObituaries();
 const int parms[4]={136,136,135,255};
 assert(seen==4&&!deliveringObituary);
 for(int i=0;i<4;i++)assert(seenParm[i]==parms[i]);
 return 0;
}
'''
code=queue+server+extract('src/PC/game_mp/g_scr_main_mp.c','GScr_Obituary')+parse
mutants=[('SV_GameSendServerCommand(-1, 1,','SV_GameSendServerCommand(-1, 0,'),
         ('pEnt->s.attackerEntityNum, pEnt->s.eventParm));','pEnt->s.attackerEntityNum, iWeaponNum));'),
         ('Cmd_Argc() == 4 || Cmd_Argc() == 5','Cmd_Argc() == 4'),
         ('Cmd_Argc() == 4 || Cmd_Argc() == 5','Cmd_Argc() >= 4'),
         ('int weapon = atoi(CG_Argv(3));','int weapon = atoi(CG_Argv(Cmd_Argc() - 1));'),
         ('weapon < 0 || weapon > 255','weapon < 0 || weapon > 256'),
         ('(unsigned)victim >= 64','(unsigned)victim > 64'),
         ('    pendingObituaryCount = 0;\n','')]
with tempfile.TemporaryDirectory(prefix='cod2-reliable-obituary-') as d:
 p=Path(d)
 for index,(old,new) in enumerate([(None,None)]+mutants):
  if old:assert code.count(old)==1,old
  (p/'test.c').write_text(support+(code.replace(old,new) if old else code)+checks)
  subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
  result=subprocess.run([str(p/'test')],capture_output=True)
  assert (result.returncode==0)==(index==0),(index,result.stderr.decode()[-1500:])
print('PASS: reliable broadcast to all clients in three arguments, headshots included; the client parses three, '
      'or four ignoring the last, and nothing else; 10 consecutive kills for each of 64 viewers; invalid events '
      'rejected; queue drains without duplicates; %d failing mutants' % len(mutants))
