#!/usr/bin/env python3
"""Exercise actual objective collision and use eligibility; reject old bugs."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent


def function(source, signature):
    start = source.index(signature + '\n{')
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


contact = function((root / 'src/PC/server_mp/sv_game_mp.c').read_text(),
                   'qboolean SV_EntityContact(const vec_t *mins, const vec_t *maxs, const gentity_t *gEnt)')
usable = function((root / 'src/PC/game_mp/player_use_mp.c').read_text(),
                  'static qboolean Player_IsUsableEntity(const gentity_t *ent)')
hint = function((root / 'src/PC/game_mp/player_use_mp.c').read_text(),
                'static qboolean Player_GetUseTriggerHint(const gentity_t *trigger, const gentity_t *player,\n                                        int *type, int *string)')
support = r'''
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
typedef float vec_t;
typedef int qboolean;
typedef struct { int startsolid; } trace_t;
typedef struct { struct {int clientNum;}ps; struct {struct {int team;}cs;}sess;}gclient_t;
typedef struct { unsigned classname; int team; gclient_t *client;
 struct {int singleUserEntIndex;}trigger; struct {int eType,dmgFlags,animMovetype;unsigned char scale;}s;
 struct {unsigned char svFlags; float currentOrigin[3],currentAngles[3],mins[3],maxs[3];}r;
} gentity_t;
static const struct {unsigned trigger_use,trigger_use_touch;}constants={100,101};
#define SCR_CONST() (&constants)
static float origin[3];
static void *imp_vec3_origin=origin;
static int brushOverlap,brushCalls;
static int SV_ClipHandleForEntity(const gentity_t *ent){(void)ent;return 42;}
static float Vec2DistanceSq(float *a,float *b){float x=a[0]-b[0],y=a[1]-b[1];return x*x+y*y;}
static void CM_TransformedBoxTraceExternal(trace_t *tr,const float *start,const float *end,
 const float *mins,const float *maxs,int handle,int mask,const float *at,const float *angles){
 assert(start==origin&&end==origin&&handle==42&&mask==-1);
 assert(mins[0]<=maxs[0]&&at&&angles);brushCalls++;tr->startsolid=brushOverlap;
}
'''
checks = r'''
int main(void){
 gentity_t ent={0};float mins[3],maxs[3];int x,y,z,n=0;(void)constants;
 ent.r.svFlags=0x21;ent.r.currentOrigin[0]=100;ent.r.currentOrigin[1]=-200;
 ent.r.currentOrigin[2]=30;ent.r.maxs[0]=ent.r.maxs[1]=48;ent.r.maxs[2]=128;
 for(x=-96;x<=96;x+=8)for(y=-96;y<=96;y+=8)for(z=-48;z<=192;z+=24){
  mins[0]=100+x-15;maxs[0]=100+x+15;
  mins[1]=-200+y-15;maxs[1]=-200+y+15;
  mins[2]=30+z;maxs[2]=mins[2]+70;
  int expected=maxs[2]>30&&mins[2]<158&&x*x+y*y<=63*63;
  assert(SV_EntityContact(mins,maxs,&ent)==expected);n++;
 }
 ent.r.svFlags=1;brushOverlap=1;assert(SV_EntityContact(mins,maxs,&ent));
 brushOverlap=0;assert(!SV_EntityContact(mins,maxs,&ent)&&brushCalls==2);
 ent.r.svFlags=0x41;ent.r.maxs[0]=112;
 mins[0]=85;maxs[0]=115;mins[1]=-215;maxs[1]=-185;
 assert(SV_EntityContact(mins,maxs,&ent));
 mins[0]=180;maxs[0]=210;assert(!SV_EntityContact(mins,maxs,&ent));
 for(int flags=0;flags<256;flags++){
  ent.r.svFlags=flags;ent.s.eType=0;ent.classname=100;assert(Player_IsUsableEntity(&ent));
  ent.classname=101;assert(Player_IsUsableEntity(&ent));
  ent.classname=102;assert(!Player_IsUsableEntity(&ent));
  ent.s.eType=3;assert(Player_IsUsableEntity(&ent));
  ent.s.eType=9;assert(Player_IsUsableEntity(&ent));
 }
 gclient_t client={0};gentity_t player={0};player.client=&client;client.ps.clientNum=7;
 for(int team=0;team<3;team++)for(int side=0;side<3;side++)for(int claim=0;claim<3;claim++){
  int type=-999,string=-999;ent.team=team;client.sess.cs.team=side;
  ent.trigger.singleUserEntIndex=claim==0?1023:claim==1?7:8;
  ent.s.dmgFlags=2;ent.s.animMovetype=0;ent.s.scale=31;
  int allowed=(!team||team==side)&&claim!=2;
  assert(Player_GetUseTriggerHint(&ent,&player,&type,&string)==allowed);
  if(allowed)assert(type==2&&string==31);
 }
 ent.team=0;ent.trigger.singleUserEntIndex=1023;ent.s.scale=255;ent.s.dmgFlags=-1;
 int type=0,string=0;assert(Player_GetUseTriggerHint(&ent,&player,&type,&string));
 assert(type==-1&&string==-1);
 printf("PASS: %d flag cylinder/height contacts, brush/disk contacts, bomb/use triggers, items and turrets across 256 shape flags, team/claim restrictions and original hint fields\n",n);
}
'''
variants = {
    'fixed': (contact, usable, hint),
    'outside-cylinder': (contact.replace('return distSq <= radius * radius;',
                                        'return distSq > radius * radius;', 1), usable, hint),
    'shape-as-use-flag': (contact, usable.replace(
        'ent->classname == SCR_CONST()->trigger_use ||\n        ent->classname == SCR_CONST()->trigger_use_touch',
        '(ent->r.svFlags & 0x20)'), hint),
    'opposite-team': (contact, usable, hint.replace('trigger->team != player->client->sess.cs.team', 'trigger->team == player->client->sess.cs.team')),
    'animation-as-hint': (contact, usable, hint.replace('*type = trigger->s.dmgFlags;', '*type = trigger->s.animMovetype;')),
}
with tempfile.TemporaryDirectory(prefix='cod2-objective-contacts-') as directory:
    path = Path(directory)
    for name, (collision, eligibility, trigger_hint) in variants.items():
        source = path / (name + '.c')
        binary = path / name
        source.write_text(support + collision + '\n' + eligibility + '\n' + trigger_hint + checks)
        subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-O2', str(source),
                        '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
        if name == 'fixed':
            assert result.returncode == 0, result.stderr
            print(result.stdout.strip())
        else:
            assert result.returncode != 0, 'Regression unexpectedly passed: ' + name
            print('PASS: rejected ' + name)
