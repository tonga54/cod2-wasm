#!/usr/bin/env python3
"""Exercise the actual obituary event with all multiplayer client indices."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parent.parent
source = (root/'src/PC/cgame_mp/cg_event_mp.c').read_text()
start=source.index('        case 0xc6:')
start=source.index('{',start)
end=start+1;depth=1
while depth:
 depth+=(source[end]=='{')-(source[end]=='}');end+=1
body=source[start:end]
support=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
typedef float vec_t;
typedef struct {int infoValid,nextValid,clientNum;char name[32];int team,oldteam;char rest[1300];} clientInfo_t;
typedef struct {int otherEntityNum,attackerEntityNum,eventParm;} entityState_t;
typedef struct {const char *killIcon;int wideKillIcon,flipKillIcon;} WeaponDef;
static WeaponDef weapon={"killiconsten",1,1};
static struct {struct {clientInfo_t clientinfo[64];} bgs;int clientNum,inKillCam;char killerName[32];} state,*cg=&state;
static const float f_1_4=1.4f,f_2_8=2.8f;
static char killedBy[34],killed[34],icon[64],center[200];static int printed;
static int teams[2],teamCount;
static void Com_Printf(const char *fmt,...){ }
static WeaponDef *BG_GetWeaponDef(int n){return &weapon;}
static void Com_Error(int n,const char *s){abort();}
static void I_strncpyz(char *out,const char *in,int n){snprintf(out,n,"%s",in);}
static void I_strncat(char *out,int n,const char *in){strncat(out,in,n-strlen(out)-1);}
static void CG_DrawScoreboard_GetTeamColor(int team,float *color){teams[teamCount++]=team;color[0]=team;}
static const char *va(const char *format,...){static char s[200];va_list args;va_start(args,format);vsnprintf(s,sizeof(s),format,args);va_end(args);return s;}
static void CG_PriorityCenterPrint(const char *msg,float y,int prio){snprintf(center,sizeof(center),"%s",msg);}
static void CL_DeathMessagePrint(const char *a,const vec_t *ac,const char *v,const vec_t *vc,const char *i,float w,float h,const vec_t *ic,int flip){
 snprintf(killedBy,sizeof(killedBy),"%s",a);snprintf(killed,sizeof(killed),"%s",v);snprintf(icon,sizeof(icon),"%s",i);printed++;
}
static void obituary(entityState_t *es) {
 float attackerColor[4],victimColor[4],iconColor[4],iconWidth;
 int target,attacker,iconHorzFlip;char targetName[34],attackerName[34];const char *iconShader;
 clientInfo_t *victimCI,*attackerCI;
'''
checks=r'''
}
static void reset(void){teamCount=printed=0;center[0]=0;}
int main(void){
 for(int n=0;n<64;n++){clientInfo_t *ci=&cg->bgs.clientinfo[n];ci->infoValid=1;ci->team=1+n%2;ci->oldteam=3;snprintf(ci->name,32,"Player %d",n);}
 for(int v=0;v<64;v++)for(int a=0;a<64;a++){
  reset();cg->clientNum=v;entityState_t es={v,a,1};obituary(&es);
  char expected[34];snprintf(expected,34,"Player %d^7",v);assert(!strcmp(killed,expected));
  snprintf(expected,34,"Player %d^7",a);assert(!strcmp(killedBy,a==v?"":expected));
  assert(printed==1 && teams[0]==1+v%2 && teams[1]==1+a%2);
  assert(!strcmp(icon,"killiconsten"));
  if(a!=v)assert(!strcmp(cg->killerName,expected));
 }
 reset();entityState_t es={63,62,0x80|9};cg->clientNum=0;obituary(&es);
 assert(printed==1 && !strcmp(icon,"killiconheadshot"));
 reset();es.attackerEntityNum=1022;obituary(&es);assert(printed==1 && !killedBy[0]);
 reset();es.attackerEntityNum=62;cg->inKillCam=1;obituary(&es);assert(!printed && !center[0]);cg->inKillCam=0;
 reset();cg->bgs.clientinfo[63].infoValid=0;obituary(&es);assert(!printed);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-obituary-') as directory:
 p=Path(directory)
 for index,variant in enumerate((body,body.replace('victimCI->name, 32','victimCI->name + 20, 32'),body.replace('attackerCI->team','attackerCI->oldteam'))):
  (p/'test.c').write_text(support+variant+checks)
  subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
  result=subprocess.run([str(p/'test')],capture_output=True)
  assert (result.returncode==0)==(index==0),result.stderr.decode()
print('PASS: 4096 killer/victim combinations, correct names and teams, headshots/world deaths and no replay duplicate; both offset mutants fail')
