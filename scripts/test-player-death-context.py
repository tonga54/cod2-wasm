#!/usr/bin/env python3
"""Exercise player_die with and without an active server animation context."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/game_mp/g_combat_mp.c').read_text()
start = source.index('void player_die(')
end = source.index('\nconst char str_002b64e0', start)
body = source[start:end]
support = r'''
#include <assert.h>
#include <string.h>
typedef float vec_t;
typedef float vec3_t[3];
typedef int hitLocation_t;
typedef struct { int time; } bgs_t;
typedef struct { struct { int clientNum,pm_type,pm_flags,eFlags,grenadeTimeLeft,offHandIndex,stats[6];vec3_t viewangles; } ps;
 struct { int connected,sessionState; } sess;int spectatorClient; } gclient_t;
typedef struct gentity { gclient_t *client;
 struct { int eType,number,otherEntityNum,weapon,loopSound; } s;
 struct { int ownerNum,contents;vec3_t currentOrigin,currentAngles,maxs; } r;
 int takedamage,health,handler; } gentity_t;
typedef struct { int death; } scr_const_t;
static scr_const_t scriptConstants;
#define SCR_CONST() (&scriptConstants)
static bgs_t level_bgs={123}, otherBgs={456}, *context;
static void *imp_bgs=&context;
static gentity_t g_entities[64];
static gclient_t clients[64];
static struct { int maxclients; gclient_t *clients; } level={64,clients};
static int hasDobj=1,animations,killed;
static int Com_GetServerDObj(int n) { return hasDobj; }
static void Scr_AddEntity(gentity_t *e) { assert(context==&level_bgs); }
static void Scr_Notify(gentity_t *e,int key,int count) { assert(context==&level_bgs); }
static float crandom(void) { return 0; }
static float randomf(void) { return 0; }
static void fire_grenade(gentity_t *e,vec3_t p,vec3_t v,int a,int b) { assert(context==&level_bgs); }
static int BG_AnimScriptEvent(void *p,int a,int b,int c) { assert(context==&level_bgs);animations++;return 700; }
static void Scr_PlayerKilled(gentity_t *s,gentity_t *i,gentity_t *a,int d,int m,int w,const vec_t *dir,hitLocation_t h,int t,int duration) { assert(context==&level_bgs && duration==700);killed++; }
static void Cmd_Score_f(gentity_t *e) { assert(context==&level_bgs); }
static float vectoyaw(const vec_t *v) { return 90; }
static void SV_UnlinkEntity(gentity_t *e) { assert(context==&level_bgs); }
static void SV_LinkEntity(gentity_t *e) { assert(context==&level_bgs); }
'''
checks = r'''
int main(void) {
 for(int mode=0;mode<3;mode++) for(int n=0;n<64;n++) {
  bgs_t *saved=mode==0?0:mode==1?&otherBgs:&level_bgs;
  context=saved;gclient_t c={0};gentity_t e={.client=&c,.health=100};
  c.ps.clientNum=e.s.number=n;
  player_die(&e,&e,&e,100000,12,0,0,0,0);
  assert(context==saved && otherBgs.time==456 && level_bgs.time==123);
  assert(e.health==0 && e.handler==11 && c.ps.pm_type==6);
  player_die(&e,&e,&e,100000,12,0,0,0,0);assert(context==saved);
  c.ps.pm_type=0;c.ps.pm_flags=0x400000;
  player_die(&e,&e,&e,100000,12,0,0,0,0);assert(context==saved);
  c.ps.pm_flags=0;hasDobj=0;
  player_die(&e,&e,&e,100000,12,0,0,0,0);assert(context==saved);hasDobj=1;
 }
 assert(animations==192 && killed==192);
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-death-context-') as directory:
    p=Path(directory)
    broken=body.replace('*(bgs_t **)imp_bgs = &level_bgs;', '(*(bgs_t **)imp_bgs)->time = level_bgs.time;')
    unrestored=body.replace('*(bgs_t **)imp_bgs = savedBgs;', '')
    for i, variant in enumerate((body,broken,unrestored)):
        (p/'test.c').write_text(support+variant+checks)
        subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
        result=subprocess.run([str(p/'test')],capture_output=True)
        assert (result.returncode==0)==(i==0), result.stderr.decode()
print('PASS: 192 deaths with null/server/other animation contexts; context restored and both old variants fail')
