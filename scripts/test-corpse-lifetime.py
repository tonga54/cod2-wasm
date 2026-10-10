#!/usr/bin/env python3
"""Replay production corpse settling, animation completion and timed cleanup."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
items = (root / 'src/PC/game_mp/g_items_mp.c').read_text()
commands = (root / 'src/PC/game_mp/g_client_script_cmd_mp.c').read_text()
main = (root / 'src/PC/game_mp/g_main_mp.c').read_text()


def function(source, name):
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


support = r'''
#include <assert.h>
#include <math.h>
#include <string.h>
typedef int qboolean, XAnimTree; typedef float vec_t,vec3_t[3];
typedef struct {int trType,trTime,trDuration;vec3_t trBase,trDelta;} trajectory_t;
typedef struct {int number,eFlags,groundEntityNum;trajectory_t pos;} entityState_t;
typedef struct {float fraction;int startsolid,allsolid,entityNum;vec3_t normal;} trace_t;
typedef struct {int slot;} clientInfo_t;
typedef struct {XAnimTree *tree;clientInfo_t ci;int entnum,falling;} corpseInfo_t;
typedef struct {entityState_t s;struct {int inuse,contents,svFlags,ownerNum;vec3_t currentOrigin,currentAngles,mins,maxs;}r;
 int active,clipmask,nextthink,handler;struct {int deathAnimStartTime;}corpse;}gentity_t;
struct DObj_s {int slot;};
static struct {int time;}level;
static struct {corpseInfo_t playerCorpseInfo[8];}g_scr_data;
static gentity_t bodies[8];static struct DObj_s objects[8];static XAnimTree trees[8];
static int deathVolume,thinkCalls,animationCalls,frees;static float rootMotion;
static int G_GetPlayerCorpseIndex(gentity_t *e) {assert(e->r.inuse);return e-bodies;}
static void G_FreeEntity(gentity_t *e) {assert(e->r.inuse);e->r.inuse=0;g_scr_data.playerCorpseInfo[e-bodies].entnum=-1;frees++;}
static void SV_LinkEntity(gentity_t *e) {assert(e->r.inuse);}
static void XAnimCalcDelta(XAnimTree *t,int n,float *r,float *d,int goal) {
 r[0]=r[1]=0;d[0]=rootMotion;d[1]=d[2]=0;
}
static void BG_EvaluateTrajectory(trajectory_t *t,int time,float *o) {
 float dt=(time-t->trTime)*.001f;
 for(int i=0;i<3;i++)o[i]=t->trBase[i]+(t->trType==5?t->trDelta[i]*dt:0);
 if(t->trType==5)o[2]-=400*dt*dt;
}
static void G_TraceCapsule(trace_t *t,const float *a,const float *mi,const float *ma,const float *b,int owner,int mask) {
 memset(t,0,sizeof(*t));t->fraction=1;t->entityNum=1023;t->normal[2]=1;
 if(b[2]<0){t->fraction=a[2]>0?a[2]/(a[2]-b[2]):0;t->entityNum=1022;}
}
static int G_TraceCapsuleComplete(const float *a,const float *mi,const float *ma,const float *b,int owner,int mask) {return b[2]>=0;}
static int SV_PointContents(const float *o,int n,int mask) {return deathVolume;}
static void AngleVectors(const float *a,float *f,float *r,float *u) {
 if(f){f[0]=1;f[1]=f[2]=0;}if(r){r[1]=-1;r[0]=r[2]=0;}
}
static float Vec3Normalize(float *v) {float n=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(n)for(int i=0;i<3;i++)v[i]/=n;return n;}
static void Vec3Cross(const float *a,const float *b,float *c) {
 c[0]=a[1]*b[2]-a[2]*b[1];c[1]=a[2]*b[0]-a[0]*b[2];c[2]=a[0]*b[1]-a[1]*b[0];
}
static void AxisToAngles(const float *axis,float *angles) {
 assert(axis[6]==0 && axis[7]==0 && axis[8]==1);
 angles[0]=0;angles[1]=0;angles[2]=0;
}
static void G_SetAngle(gentity_t *e,const float *a) {assert(e->r.inuse);memcpy(e->r.currentAngles,a,sizeof(vec3_t));}
static struct DObj_s *Com_GetServerDObj(int n) {assert(n>=64&&n<72&&bodies[n-64].r.inuse);return &objects[n-64];}
static void BG_UpdatePlayerDObj(struct DObj_s *o,entityState_t *s,clientInfo_t *c,int n) {assert(bodies[s->number-64].r.inuse);}
static void BG_PlayerAnimation(struct DObj_s *o,entityState_t *s,clientInfo_t *c) {assert(bodies[s->number-64].r.inuse);animationCalls++;}
static int G_RunThink(gentity_t *e);
'''

body_end = function(commands, 'BodyEnd')
move = function(items, 'G_RunCorpseMove')
run = function(items, 'G_RunCorpse')
think = function(main, 'G_RunThink_core')
# Use the production deadline dispatch too, with the same handler numbers.
think = think.replace('static inline __attribute__((always_inline)) ', 'static ')
dispatch = r'''
typedef void (*fn_think)(gentity_t *);
static struct {fn_think think;}entityHandlers[20];
static void Com_Error(int code,const char *s) {assert(0);}
'''
wrapper = r'''
static int G_RunThink(gentity_t *e) {thinkCalls++;G_RunThink_core(e);return 0;}
'''
checks = r'''
static void spawn(int slot,int birth,int duration,float height) {
 gentity_t *e=&bodies[slot];memset(e,0,sizeof(*e));e->r.inuse=1;e->s.number=64+slot;
 e->s.eFlags=0xa0000;e->r.contents=0x4002000;e->r.svFlags=2;
 e->s.pos.trType=5;e->s.pos.trTime=e->corpse.deathAnimStartTime=birth;
 e->r.currentOrigin[2]=e->s.pos.trBase[2]=height;e->nextthink=birth+duration;e->handler=12;
 g_scr_data.playerCorpseInfo[slot]=(corpseInfo_t){.tree=&trees[slot],.entnum=64+slot,.falling=1};
}
int main(void) {
 entityHandlers[12].think=BodyEnd;entityHandlers[19].think=G_FreeEntity;
 int scenarios=0;
 for(int round=0;round<30;round++)for(int slot=0;slot<8;slot++) {
  int birth=1000+round*10000,duration=50+slot*350;
  deathVolume=0;rootMotion=0;spawn(slot,birth,duration,slot*5);
  gentity_t *e=&bodies[slot];int before=frees;
  for(int t=50;t<8000;t+=50) {
   level.time=birth+t;int calls=thinkCalls;G_RunCorpse(e);
   assert(e->r.inuse&&thinkCalls==calls+1);
   if(t>=1000){assert(!g_scr_data.playerCorpseInfo[slot].falling);assert(e->s.pos.trType==1);assert(fabsf(e->r.currentOrigin[2])<.01f);}
   if(t>=duration){assert(!(e->s.eFlags&0x80000));assert(e->handler==19&&e->nextthink==birth+8000);}
   /* Owner respawn at 3 seconds has no access to this separate clone. */
  }
  assert(frees==before);level.time=birth+8000;G_RunCorpse(e);
  assert(!e->r.inuse&&frees==before+1&&g_scr_data.playerCorpseInfo[slot].entnum==-1);scenarios++;
 }
 /* Gravity and root motion still settle normally, even with active == 0. */
 deathVolume=0;rootMotion=2;spawn(0,1000,1500,0);level.time=1050;G_RunCorpse(&bodies[0]);
 level.time=1100;G_RunCorpse(&bodies[0]);assert(fabsf(bodies[0].r.currentOrigin[0]-2)<.01f);
 /* A death-volume removal must not query the freed animation tree. */
 rootMotion=0;deathVolume=1;spawn(0,1000,500,0);level.time=1050;
 int animations=animationCalls,calls=thinkCalls;G_RunCorpse(&bodies[0]);
 assert(!bodies[0].r.inuse&&animationCalls==animations&&thinkCalls==calls);
 assert(scenarios==240);return 0;
}
'''

variants = [
    (body_end, move, run),
    (body_end.replace('ent->nextthink = ent->corpse.deathAnimStartTime + 8000;', 'ent->nextthink = 0;'), move, run),
    (body_end, move.replace('    if (tr.fraction == 1.0f)', '    if (!ent->active) return;\n    if (tr.fraction == 1.0f)', 1), run),
    (body_end, move, run.replace('    if (!ent->r.inuse)\n        return;', '')),
    (body_end, move.replace('    SV_LinkEntity(ent);', '    SV_LinkEntity(ent);\n    G_RunThink(ent);', 1), run),
    (body_end, move.replace('AxisToAngles((const vec_t *)axis, groundAngles);', 'AxisToAngles((const vec_t *)axis, rot);'), run),
]
with tempfile.TemporaryDirectory(prefix='cod2-corpse-lifetime-') as directory:
    path = Path(directory)
    for index, (end, movement, frame) in enumerate(variants):
        (path / 'test.c').write_text(support + end + dispatch + think + wrapper + movement + frame + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr.decode())
print('PASS: 240 corpse lifecycles, floor settling/root motion, 8-second cleanup and safe death-volume removal; five broken variants fail')
