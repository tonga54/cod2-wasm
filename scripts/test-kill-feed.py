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
static int feedEnabled,fed,fedAttacker,fedVictim,fedWide,fedFlip,fedHeadshot;static char fedIcon[64];
static int CG_KillFeedEnabled(void){return feedEnabled;}
static void CG_KillFeedAdd(int a,int v,const char *i,int wide,int flip,int headshot){fed++;fedAttacker=a;fedVictim=v;snprintf(fedIcon,sizeof(fedIcon),"%s",i);fedWide=wide;fedFlip=flip;fedHeadshot=headshot;}
static int killcamCalls,killcamAttacker,killcamTarget,killcamParm;
static void CG_KillcamObituary(int a,int t,int ep){killcamCalls++;killcamAttacker=a;killcamTarget=t;killcamParm=ep;}
static int deliveringObituary;
static void obituary(entityState_t *es) {
 float attackerColor[4],victimColor[4],iconColor[4],iconWidth;
 int target,attacker,iconHorzFlip;char targetName[34],attackerName[34];const char *iconShader;
 clientInfo_t *victimCI,*attackerCI;
'''
checks=r'''
}
static void reset(void){teamCount=printed=fed=killcamCalls=0;center[0]=0;}
int main(void){
#ifdef __EMSCRIPTEN__
 deliveringObituary=1;
#endif
 for(int n=0;n<64;n++){clientInfo_t *ci=&cg->bgs.clientinfo[n];ci->infoValid=1;ci->team=1+n%2;ci->oldteam=3;snprintf(ci->name,32,"Player %d",n);}
 for(int v=0;v<64;v++)for(int a=0;a<64;a++){
  reset();cg->clientNum=v;entityState_t es={v,a,1};obituary(&es);
  char expected[34];snprintf(expected,34,"Player %d^7",v);assert(!strcmp(killed,expected));
  snprintf(expected,34,"Player %d^7",a);assert(!strcmp(killedBy,a==v?"":expected));
  assert(printed==1 && teams[0]==1+v%2 && teams[1]==1+a%2);
  assert(!strcmp(icon,"killiconsten"));
  if(a!=v)assert(!strcmp(cg->killerName,expected));
 }
 /* The retail jump table for MOD_MELEE (7) through MOD_TRIGGER_HURT (13). */
 const char *modIcons[]={"killiconmelee","killiconheadshot","killiconcrush","killicondied","killiconfalling","killiconsuicide","killicondied","killicondied"};
 for(int mod=7;mod<=14;mod++){
  reset();entityState_t es={63,62,0x80|mod};cg->clientNum=0;obituary(&es);
  assert(printed==1 && !strcmp(icon,modIcons[mod-7]));
 }
 reset();entityState_t es={63,62,0x80|8};cg->clientNum=0;obituary(&es);
 assert(printed==1 && !strcmp(icon,"killiconheadshot"));
 reset();es.attackerEntityNum=1022;obituary(&es);assert(printed==1 && !killedBy[0]);
 reset();es.attackerEntityNum=62;cg->inKillCam=1;obituary(&es);assert(printed==deliveringObituary && !center[0]);cg->inKillCam=0;
 reset();cg->bgs.clientinfo[63].infoValid=0;obituary(&es);assert(!printed);
 cg->bgs.clientinfo[63].infoValid=1;feedEnabled=1;
 reset();cg->clientNum=5;es=(entityState_t){5,6,1};obituary(&es);
 assert(fed==1 && !printed && fedAttacker==6 && fedVictim==5 && !strcmp(fedIcon,"killiconsten") && fedWide && fedFlip);
 assert(!strcmp(cg->killerName,"Player 6^7"));
 reset();weapon.wideKillIcon=weapon.flipKillIcon=0;obituary(&es);assert(fed==1 && !fedWide && !fedFlip);
 reset();es=(entityState_t){5,1022,0x80|8};obituary(&es);assert(fed==1 && fedAttacker==-1 && !strcmp(fedIcon,"killiconheadshot") && !fedWide && fedHeadshot);
 reset();es=(entityState_t){5,5,0x80|12};obituary(&es);assert(fed==1 && fedAttacker==-1 && !strcmp(fedIcon,"killiconsuicide") && !fedHeadshot);
 /* Every headshot, natively and in the browser, reaches the feed as one with
  * the stock headshot icon, so the feed shows its symbol; other special deaths
  * and weapon kills, weapon 8's too, do not. */
 reset();es=(entityState_t){5,6,0x80|8};obituary(&es);
 assert(fed==1 && fedAttacker==6 && fedVictim==5 && !strcmp(fedIcon,"killiconheadshot") && !fedWide && !fedFlip && fedHeadshot);
 reset();es=(entityState_t){5,6,0x80|7};obituary(&es);assert(fed==1 && !strcmp(fedIcon,"killiconmelee") && !fedHeadshot);
 reset();es=(entityState_t){5,6,0x80|9};obituary(&es);assert(fed==1 && !strcmp(fedIcon,"killiconcrush") && !fedHeadshot);
 weapon.wideKillIcon=weapon.flipKillIcon=1;
 reset();es=(entityState_t){5,6,8};obituary(&es);assert(fed==1 && !strcmp(fedIcon,"killiconsten") && fedWide && fedFlip && !fedHeadshot);
 reset();es=(entityState_t){5,6,1};obituary(&es);assert(fed==1 && !strcmp(fedIcon,"killiconsten") && !fedHeadshot);
 reset();es=(entityState_t){5,6,1};cg->inKillCam=1;obituary(&es);assert(fed==deliveringObituary && !printed);cg->inKillCam=0;
 /* The killcam hears of every obituary once, with the feed on or off, as
  * the event has it, a death without a killer having none; of a replayed
  * one, never. */
 feedEnabled=0;
 reset();es=(entityState_t){5,6,1};obituary(&es);
 assert(killcamCalls==1 && killcamAttacker==6 && killcamTarget==5 && killcamParm==1);
 feedEnabled=1;
 reset();es=(entityState_t){5,6,0x80|8};obituary(&es);assert(killcamCalls==1 && killcamAttacker==6 && killcamParm==(0x80|8));
 reset();es=(entityState_t){5,1022,0x80|8};obituary(&es);assert(killcamCalls==1 && killcamAttacker==-1 && killcamTarget==5);
 reset();es=(entityState_t){5,5,0x80|12};obituary(&es);assert(killcamCalls==1 && killcamAttacker==-1);
 reset();es=(entityState_t){5,6,1};cg->inKillCam=1;obituary(&es);assert(killcamCalls==deliveringObituary);cg->inKillCam=0;
 return 0;
}
'''
def run(source,name,web):
 (p/(name+'.c')).write_text(source)
 subprocess.run(['cc']+(['-D__EMSCRIPTEN__'] if web else [])+['-std=c99','-O1','-g','-fsanitize=address,undefined',
                 str(p/(name+'.c')),'-o',str(p/name)],check=True)
 return subprocess.run([str(p/name)],capture_output=True)
with tempfile.TemporaryDirectory(prefix='cod2-obituary-') as directory:
 p=Path(directory)
 mutants=[('victimCI->name, 32','victimCI->name + 20, 32'),('attackerCI->team','attackerCI->oldteam'),
          ('attackerName[0] ? attacker : -1','attacker'),('iconWidth > 2.0f','iconWidth > 1.0f'),
          ('if (CG_KillFeedEnabled())','if (0)'),
          ('iconShader = (const char *)"killiconheadshot";','iconShader = (const char *)"killiconcrush";'),
          ('                    case 5:\n','                    case 3:\n'),
          ('(ep & 0x7f) == 8','(ep & 0x7f) == 9'),
          ('(ep & 0x80) && (ep & 0x7f) == 8','(ep & 0x7f) == 8'),
          ('iconWidth > 2.0f, iconHorzFlip,','iconWidth > 2.0f, 0,'),
          ('            CG_KillcamObituary(attackerName[0] ? attacker : -1, target, ep);\n',''),
          ('CG_KillcamObituary(attackerName[0] ? attacker : -1,','CG_KillcamObituary(attacker,'),
          ('target, ep);','target, ep & 0x7f);')]
 for old,new in mutants:
  assert body.count(old)>=1,old
 for index,variant in enumerate([body]+[body.replace(old,new) for old,new in mutants]):
  results=[run(support+variant+checks,'test-%d-%s'%(index,'web' if web else 'native'),web) for web in (0,1)]
  passed=all(result.returncode==0 for result in results)
  assert passed==(index==0),(index,[result.stderr.decode()[-1500:] for result in results])
print('PASS: native and reliable browser feed; 4096 killer/victim combinations, correct names and teams, retail icons for '
      'melee, headshot, crush, falling and suicide deaths, world deaths and no replay duplicate; with cg_hudKillFeed the '
      'obituary goes to the WWII feed with the weapon icon, its width and mirroring, every headshot is marked as one '
      'with the stock headshot icon in both builds, and world deaths and suicides have no attacker; the killcam hears '
      'of each obituary once, feed or not; all %d mutants fail' % len(mutants))
