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
static int recording,seen,seenParm[8],seenWeapon[8];
static void CG_EntityEvent(centity_t *event,int code){
 assert(deliveringObituary&&code==198);
 if(recording){assert(seen<8);seenParm[seen]=event->nextState.eventParm;seenWeapon[seen++]=deliveringObituaryWeapon;return;}
 assert(event->nextState.otherEntityNum==63&&event->nextState.attackerEntityNum==62&&event->nextState.eventParm==17);
 assert(!deliveringObituaryWeapon);
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
 /* A headshot carries MOD_HEAD_SHOT, so its weapon follows in a fourth argument. */
 broadcasts=0;modIndex=8;GScr_Obituary();
 assert(broadcasts==1&&reliability==1&&!strcmp(messages[0],"cod2_obituary 63 62 136 17")&&event.s.eventParm==136);
 broadcasts=0;weaponIndex=0;GScr_Obituary();assert(broadcasts==1&&!strcmp(messages[0],"cod2_obituary 63 62 136"));
 broadcasts=0;weaponIndex=17;modIndex=7;GScr_Obituary();assert(broadcasts==1&&!strcmp(messages[0],"cod2_obituary 63 62 135"));
 for(local=0;local<64;local++){
  CG_QueueObituary(64,62,17,0);CG_QueueObituary(63,64,17,0);CG_QueueObituary(63,62,256,0);CG_FlushObituaries();assert(!printed[local]);
  for(int i=0;i<10;i++)receive("cod2_obituary 63 62 17");
  CG_FlushObituaries();assert(printed[local]==10&&!deliveringObituary);
  CG_FlushObituaries();assert(printed[local]==10);
 }
 recording=1;
 /* Each obituary keeps its own weapon; one outside the weapon table is dropped. */
 receive("cod2_obituary 63 62 136 17");receive("cod2_obituary 63 62 136");
 receive("cod2_obituary 63 62 136 17 9");receive("cod2_obituary 63 62");receive("cod2_obituary 64 62 136 21");
 receive("cod2_obituary 63 62 136 128");receive("cod2_obituary 63 62 136 -3");receive("cod2_obituary 63 62 135 0");
 receive("cod2_obituary 63 62 136 127");
 CG_FlushObituaries();
 const int parms[6]={136,136,136,136,135,136},weapons[6]={17,0,0,0,0,127};
 assert(seen==6&&!deliveringObituary&&!deliveringObituaryWeapon);
 for(int i=0;i<6;i++)assert(seenParm[i]==parms[i]&&seenWeapon[i]==weapons[i]);
 return 0;
}
'''
code=queue+server+extract('src/PC/game_mp/g_scr_main_mp.c','GScr_Obituary')+parse
mutants=[('    if (iMODNum == 8 && iWeaponNum > 0)\n','    if (iMODNum == 8)\n'),
         ('pEnt->s.eventParm, iWeaponNum));','pEnt->s.eventParm, pEnt->s.eventParm));'),
         ('Cmd_Argc() == 4 || Cmd_Argc() == 5','Cmd_Argc() == 4'),
         ('Cmd_Argc() == 4 || Cmd_Argc() == 5','Cmd_Argc() >= 4'),
         ('int headshotWeapon = Cmd_Argc() == 5 ? atoi(CG_Argv(4)) : 0;','int headshotWeapon = 0;'),
         ('    if (headshotWeapon <= 0 || headshotWeapon >= 128)\n        headshotWeapon = 0;\n',''),
         ('headshotWeapon >= 128','headshotWeapon > 128'),
         ('    pendingObituaries[pendingObituaryCount].headshotWeapon = headshotWeapon;\n',''),
         ('        deliveringObituaryWeapon = 0;\n',''),
         ('        deliveringObituaryWeapon = pendingObituaries[i].headshotWeapon;\n','')]
with tempfile.TemporaryDirectory(prefix='cod2-reliable-obituary-') as d:
 p=Path(d)
 for index,(old,new) in enumerate([(None,None)]+mutants):
  if old:assert code.count(old)==1,old
  (p/'test.c').write_text(support+(code.replace(old,new) if old else code)+checks)
  subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
  result=subprocess.run([str(p/'test')],capture_output=True)
  assert (result.returncode==0)==(index==0),(index,result.stderr.decode()[-1500:])
print('PASS: reliable broadcast to all clients; a headshot carries its weapon as a fourth argument; the client '
      'parses three or four arguments only; 10 consecutive kills for each of 64 viewers; invalid events and '
      'weapons rejected; queue drains without duplicates and keeps each obituary\'s weapon; %d failing mutants'
      % len(mutants))
