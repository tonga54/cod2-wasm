#!/usr/bin/env python3
"""Exercise shared production walking/air physics when movement is released."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/bgame/bg_pmove.c').read_text()


def function(name):
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
#include <stdio.h>
#include <string.h>
typedef int qboolean; typedef float vec_t,vec2_t[2],vec3_t[3];
#define PM_REGPARM2_ABI
#define PM_ACCELERATE_ABI
#define PMF_LADDER 32
#define PMF_MANTLE 4
#define PMF_JUMPING 0x80000
#define PMF_SPRINT 0x02000000
#define BUTTON_SPRINT 2
#define SPRINT_SPEED_SCALE 1.45f
#define ENTITYNUM_NONE 1023
typedef struct {struct {float value;int enabled;} current;} dvar_t;
typedef struct {int surfaceFlags;vec3_t normal;} trace_t;
typedef struct {int pm_type,pm_flags,groundEntityNum,eFlags,weaponstate,speed,movementDir;
    float leanf,fWeaponPosFrac;vec3_t origin,velocity;float oldVelocity[2];} playerState_t;
typedef struct {int buttons,forwardmove,rightmove;} usercmd_t;
typedef struct {playerState_t *ps;usercmd_t cmd;} pmove_t;
typedef struct {int walking,groundPlane;float frametime;vec3_t forward,right;trace_t groundTrace;} pml_t;
static dvar_t frictionD={.current.value=5.5f},stopD={.current.value=100},inertiaD={.current.value=50},zeroD,spectateD={.current.value=2};
static const dvar_t *friction=&frictionD,*stopspeed=&stopD,*inertiaMax=&inertiaD,
    *inertiaAngle=&zeroD,*inertiaDebug=&zeroD,*player_spectateSpeedScale=&spectateD;
static const dvar_t **imp_player_spectateSpeedScale=&player_spectateSpeedScale;
static int PM_GetEffectiveStance(playerState_t *ps) {return ps->pm_flags&1?1:ps->pm_flags&2?2:0;}
static float Jump_ReduceFriction(playerState_t *ps) {return .5f;}
static float Vec3Normalize(float *v) {
    float n=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
    if(n)for(int i=0;i<3;i++)v[i]/=n;return n;
}
static void Vec2Normalize(float *v) {float n=hypotf(v[0],v[1]);if(n){v[0]/=n;v[1]/=n;}}
static float AngleNormalize180Accurate(float a) {while(a>180)a-=360;while(a<-180)a+=360;return a;}
static void Com_Printf(const char *s,...) {}
static int jumpRequested;
static int Jump_Check(pmove_t *pm,pml_t *pml) {
    if(!jumpRequested)return 0;
    pml->walking=pml->groundPlane=0;pm->ps->groundEntityNum=1023;
    pm->ps->velocity[2]=240;pm->ps->pm_flags|=PMF_JUMPING;return 1;
}
/* Flat-ground/slope displacement only; collision behavior has separate tests. */
static void PM_StepSlideMove(pmove_t *pm,pml_t *pml,int gravity) {
    for(int i=0;i<3;i++)pm->ps->origin[i]+=pm->ps->velocity[i]*pml->frametime;
}
'''
names = ['PM_DvarFloat', 'PM_DotProduct', 'PM_VectorCopy', 'PM_AbsInt',
         'PM_CmdScale', 'PM_UpdateSprint', 'PM_ClipVelocity', 'PM_Friction',
         'PM_Accelerate', 'PM_SetMovementDir', 'PM_ApplyGroundPlane', 'PM_AirMove']
body = ''.join(function(name) for name in names)
walk = function('PM_WalkMove')
stop = re.search(r'    /\* Releasing movement.*?\n    }\n', walk, re.S)[0]
checks = r'''
static pml_t ground(int msec) {
    return (pml_t){.walking=1,.groundPlane=1,.frametime=msec*.001f,
        .forward={1,0,0},.right={0,-1,0},.groundTrace={.normal={0,0,1}}};
}
static playerState_t player(float speed,int flags,float angle) {
    float a=angle*.01745329252f;
    return (playerState_t){.speed=190,.groundEntityNum=1022,.pm_flags=flags,
        .origin={20,30,40},.velocity={speed*cosf(a),speed*sinf(a),0}};
}
static void legacy_coast(float speed) {
    playerState_t p=player(speed,0,0);pml_t pml=ground(8);
    int elapsed=0;float distance=0;
    while(p.velocity[0]>0&&elapsed<2000) {
        PM_Friction(&p,&pml);distance+=p.velocity[0]*pml.frametime;elapsed+=8;
    }
    printf("Previous ground friction at %.1f units/s: %d ms / %.2f units of coasting\n",speed,elapsed,distance);
}
int main(void) {
    legacy_coast(190);legacy_coast(190*SPRINT_SPEED_SCALE);
    int cases=0;
    for(int msec=1;msec<=66;msec++)for(int angle=0;angle<360;angle+=15)
    for(int stance=0;stance<4;stance++)for(int slope=-1;slope<=1;slope++) {
        int flags=stance==1?2:stance==2?1:stance==3?PMF_SPRINT:0;
        float speed=190*(stance==1?.65f:stance==2?.15f:stance==3?SPRINT_SPEED_SCALE:1);
        playerState_t p=player(speed,flags,angle),server=p;
        pml_t pml=ground(msec);pml.groundTrace.normal[0]=slope*.4f;
        pml.groundTrace.normal[2]=sqrtf(1-pml.groundTrace.normal[0]*pml.groundTrace.normal[0]);
        p.velocity[2]=server.velocity[2]=-p.velocity[0]*pml.groundTrace.normal[0]/pml.groundTrace.normal[2];
        for(int side=0;side<2;side++) {
            pmove_t pm={.ps=side?&server:&p};
            PM_UpdateSprint(&pm,&pml);PM_WalkMove(&pm,&pml);
            assert(pm.ps->velocity[0]==0&&pm.ps->velocity[1]==0&&pm.ps->velocity[2]==0);
            assert(pm.ps->origin[0]==20&&pm.ps->origin[1]==30&&pm.ps->origin[2]==40);
        }
        assert(!memcmp(&p,&server,sizeof(p)));
        cases++;
    }
    /* Held forward/strafe/diagonal/opposite inputs keep accelerating normally. */
    for(int msec=1;msec<=66;msec++)for(int f=-127;f<=127;f+=127)
    for(int r=-127;r<=127;r+=127)if(f||r) {
        playerState_t p=player(190,0,0);pml_t pml=ground(msec);
        pmove_t pm={.ps=&p,.cmd={.forwardmove=f,.rightmove=r}};
        PM_WalkMove(&pm,&pml);
        assert(hypotf(p.velocity[0],p.velocity[1])>0);
        if(f==127&&!r)assert(p.velocity[0]>=180);
    }
    /* Preserve natural friction on slick ground, ladders, landing and pushes. */
    for(int kind=0;kind<6;kind++)for(int msec=1;msec<=66;msec++) {
        playerState_t p=player(275.5f,0,45),expected=p;
        pml_t pml=ground(msec);
        if(kind==0)pml.groundTrace.surfaceFlags=2;
        if(kind==1)p.pm_flags=0x400;
        if(kind==2)p.pm_flags=0x200;
        if(kind==3)p.pm_flags=PMF_JUMPING;
        if(kind==4)p.pm_flags=PMF_LADDER;
        if(kind==5)p.pm_type=4;
        expected=p;PM_Friction(&expected,&pml);
        pmove_t pm={.ps=&p};PM_WalkMove(&pm,&pml);
        for(int i=0;i<3;i++)assert(p.velocity[i]==expected.velocity[i]);
        assert(hypotf(p.velocity[0],p.velocity[1])>0);
    }
    /* Releasing in midair or starting a jump never plants the feet in air. */
    for(int msec=1;msec<=66;msec++)for(int jumping=0;jumping<2;jumping++) {
        playerState_t p=player(190,0,30);p.velocity[2]=200;
        pml_t pml=ground(msec);pmove_t pm={.ps=&p};jumpRequested=jumping;
        if(jumping)PM_WalkMove(&pm,&pml);
        else {pml.walking=pml.groundPlane=0;PM_AirMove(&pm,&pml);}
        assert(fabsf(hypotf(p.velocity[0],p.velocity[1])-190)<.001f);
        assert(p.velocity[2]==(jumping?240:200));
    }
    printf("PASS: %d release schedules, 24 directions, walk/crouch/prone/sprint, slopes, held keys, air/jump/slick/landing/push exclusions and client/server parity\n",cases);
}
'''
variants = [walk, walk.replace(stop, ''),
            walk.replace('if (ps->pm_type == 0 && !pm->cmd.forwardmove && !pm->cmd.rightmove &&',
                         'if (ps->pm_type == 0 &&'),
            walk.replace('!(ps->pm_flags & (PMF_LADDER | PMF_JUMPING | 0x200 | 0x400))', '1')]
with tempfile.TemporaryDirectory(prefix='cod2-movement-release-') as directory:
    path = Path(directory)
    for index, variant in enumerate(variants):
        (path / 'test.c').write_text(support + body + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-fsanitize=address,undefined',
                        str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stdout, result.stderr)
        if index == 0:
            print(result.stdout, end='')
    print('PASS: coasting, braking held inputs and cancelling forced momentum regressions fail')
