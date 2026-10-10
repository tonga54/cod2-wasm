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
source=(root/'src/PC/cgame_mp/cg_event_mp.c').read_text()
start=source.index('static qboolean deliveringObituary;');end=source.index('\n#endif',start)
queue=source[start:end]
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
'''
server=r'''
typedef struct {struct {int number,otherEntityNum,attackerEntityNum,eventParm;}s;struct {int svFlags;}r;}gentity_t;
static vec3_t zero;static void *imp_vec3_origin=&zero;static gentity_t event,victim={.s={63}},attacker={.s={62}};
static int broadcasts,destination,reliability;static char message[100];
static int G_GetWeaponIndexForName(const char*s){return 17;}
static const char* Scr_GetString(int i){return "sten_mp";}
static int G_IndexForMeansOfDeath(const char*s){return 1;}
static gentity_t *Scr_GetEntity(int i){return i?&attacker:&victim;}
static gentity_t *G_TempEntity(float *p,int type){assert(type==198);memset(&event,0,sizeof(event));return &event;}
static int Scr_GetType(int i){return 1;}static int Scr_GetPointerType(int i){return 21;}
static const char *va(const char*fmt,...){static char text[100];va_list a;va_start(a,fmt);vsnprintf(text,100,fmt,a);va_end(a);return text;}
static void SV_GameSendServerCommand(int client,int type,const char*text){destination=client;reliability=type;broadcasts++;strcpy(message,text);}
'''
checks=r'''
static void CG_EntityEvent(centity_t *event,int code){assert(deliveringObituary&&code==198);assert(event->nextState.otherEntityNum==63&&event->nextState.attackerEntityNum==62&&event->nextState.eventParm==17);printed[local]++;}
int main(void){
 GScr_Obituary();assert(broadcasts==1&&destination==-1&&reliability==1&&event.r.svFlags==8);assert(!strcmp(message,"cod2_obituary 63 62 17"));
 for(local=0;local<64;local++){
  CG_QueueObituary(64,62,17);CG_QueueObituary(63,64,17);CG_QueueObituary(63,62,256);CG_FlushObituaries();assert(!printed[local]);
  for(int i=0;i<10;i++)CG_QueueObituary(63,62,17);
  CG_FlushObituaries();assert(printed[local]==10&&!deliveringObituary);
  CG_FlushObituaries();assert(printed[local]==10);
 }
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-reliable-obituary-') as d:
 p=Path(d);(p/'test.c').write_text(support+queue+server+extract('src/PC/game_mp/g_scr_main_mp.c','GScr_Obituary')+checks)
 subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: reliable broadcast to all clients; 10 consecutive kills for each of 64 viewers; invalid events rejected; queue drains without duplicates')
