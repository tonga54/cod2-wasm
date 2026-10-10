#!/usr/bin/env python3
"""Replay the production stance inputs, transition boundaries and anim blends."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent


def function(path, name):
    source = (root / path).read_text()
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end = match.end()
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


def check(source, mutants=()):
    with tempfile.TemporaryDirectory(prefix='cod2-player-stance-') as directory:
        p = Path(directory)
        for index, variant in enumerate((source, *mutants)):
            (p / 'test.c').write_text(variant)
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                            str(p / 'test.c'), '-lm', '-o', str(p / 'test')], check=True)
            result = subprocess.run([str(p / 'test')], capture_output=True, text=True)
            assert (result.returncode == 0) == (index == 0), result.stderr


input_support = r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef unsigned char byte;
typedef int qboolean;
typedef struct {int down[2],downtime,msec;byte active,wasPressed,padding[2];} kbutton_t;
static kbutton_t kb[28],sprintButton;
typedef struct {int buttons,forwardmove,rightmove;} usercmd_t;
typedef struct {int cl_stance;} LegacyHacks;
static LegacyHacks legacy,*legacyPtr=&legacy;
static void *imp_legacyHacks=&legacyPtr;
typedef struct {int keyCatchers,cgameInShellshock;struct {struct {int pm_type;}ps;}snap;} clientActive_t;
static clientActive_t client,*clientPtr=&client;
static void *imp_cl=&clientPtr;
static struct {struct {int enabled;}current;} bypass,*cl_bypassMouseInput=&bypass;
static unsigned int frameMsec=16;
static void *imp_frame_msec=&frameMsec;
#define BUTTON_SPRINT 2
static const char *Cmd_Argv(int n) {return n==1 ? "32" : "1000";}
#define Com_Printf(...) ((void)0)
'''
input_body = ''.join(function('src/PC/client_mp/cl_input.c', n) for n in (
    'IN_KeyDown', 'IN_KeyUp', 'IN_ToggleCrouch', 'IN_GoProne', 'IN_GoStandDown',
    'IN_GoStandUp', 'CL_ConsumeButtonPress', 'CL_CmdButtons'))
input_checks = r'''
int main(void) {
    for(int posture=1;posture<=2;posture++) for(int type=0;type<8;type++) {
        memset(kb,0,sizeof(kb));legacy.cl_stance=posture;client.snap.ps.pm_type=type;
        IN_GoStandDown(); assert(legacy.cl_stance==0 && !kb[10].active);
        for(int frame=0;frame<100;frame++) {
            usercmd_t cmd={0};CL_CmdButtons(&cmd);
            assert(!!(cmd.buttons&0x400)==(type>=2 && type<=4));
        }
        IN_GoStandUp(); assert(!kb[10].active && !kb[12].active);
        if(type==0) {
            IN_GoStandDown();usercmd_t cmd={0};CL_CmdButtons(&cmd);
            assert(cmd.buttons&0x400);IN_GoStandUp();
        }
    }
    legacy.cl_stance=0;IN_ToggleCrouch();assert(legacy.cl_stance==1);
    IN_ToggleCrouch();assert(legacy.cl_stance==0);
    IN_GoProne();assert(legacy.cl_stance==2);
    IN_ToggleCrouch();assert(legacy.cl_stance==1);
    IN_ToggleCrouch();assert(legacy.cl_stance==0);
    for(int blocked=0;blocked<2;blocked++) {
        kb[blocked ? 25 : 11].active=1;IN_ToggleCrouch();assert(legacy.cl_stance==0);
        kb[blocked ? 25 : 11].active=0;
    }
    return 0;
}
'''
input_source = input_support + input_body + input_checks
check(input_source, (input_source.replace('(unsigned int)((*clp)->snap.ps.pm_type - 2)',
                                         '(int)((*clp)->snap.ps.pm_type - 2)'),))

movement_support = r'''
#include <assert.h>
#include <math.h>
#include <string.h>
#define PM_REGPARM2_ABI
#define ANIM_ET_CROUCH_TO_PRONE 11
#define ANIM_ET_PRONE_TO_CROUCH 12
#define ANIM_ET_STAND_TO_CROUCH 13
#define ANIM_ET_CROUCH_TO_STAND 14
typedef int qboolean;
typedef float vec3_t[3];
typedef struct {int allsolid,startsolid;float fraction;vec3_t normal;} trace_t;
typedef struct {
    int pm_type,pm_flags,eFlags,clientNum,groundEntityNum,viewHeightTarget,
        viewHeightLerpTime,movementDir;
    vec3_t origin,mins,maxs,viewangles;
    float fTorsoHeight,fTorsoPitch,fWaistPitch,viewHeightCurrent,proneDirection,
        proneDirectionPitch,proneTorsoPitch;
} playerState_t;
typedef struct {int buttons,forwardmove,rightmove,serverTime;} usercmd_t;
typedef struct {playerState_t *ps;usercmd_t cmd;vec3_t mins,maxs;int proneChange,handler,tracemask;} pmove_t;
typedef struct {int unused;} pml_t;
static int events[32],eventCount,blockedHeight;
static int BG_AnimScriptEvent(playerState_t *p,int e,int c,int f) {events[eventCount++]=e;return 500;}
static void BG_AddPredictableEventToPlayerstate(int e,int parm,playerState_t *p) {}
static void BG_PlayAnim(playerState_t *p,int n,int b,int d,int t,int c,int f) {}
static void Jump_ActivateSlowdown(playerState_t *p) {}
static void PM_ExitAimDownSight(playerState_t *p) {}
static float PitchForYawOnNormal(float yaw,vec3_t normal) {return 0;}
static float AngleNormalize180Accurate(float a) {while(a>180)a-=360;while(a<-180)a+=360;return a;}
static float AngleDelta(float a,float b) {return AngleNormalize180Accurate(a-b);}
static int PM_AbsInt(int a) {return a<0 ? -a : a;}
static int BG_CheckProne(int n,vec3_t p,float s,float h,float y,float *t,float *tp,
    float *wp,int a,int g,void *normal,int handler,int type,float dist) {return 1;}
static void PM_playerTrace(pmove_t *p,trace_t *t,vec3_t a,vec3_t mins,vec3_t maxs,
    vec3_t b,int n,int mask) {
    memset(t,0,sizeof(*t));t->fraction=1;t->normal[2]=1;
    t->allsolid=blockedHeight && maxs[2]>=blockedHeight;
}
/* Pause/resume the camera lerp explicitly: the test checks the actual duck
 * boundaries, rather than replacing them with a second stance implementation. */
static void PM_ViewHeightAdjust(pmove_t *p,pml_t *l) {
    if(p->ps->viewHeightCurrent!=p->ps->viewHeightTarget)p->ps->viewHeightLerpTime=1;
}
'''
movement_body = ''.join(function('src/PC/bgame/bg_pmove.c', n) for n in (
    'PM_SetMovementDir', 'PM_CheckDuck'))
movement_checks = r'''
static void finished(playerState_t *p) {p->viewHeightCurrent=p->viewHeightTarget;p->viewHeightLerpTime=0;}
int main(void) {
    playerState_t p={.groundEntityNum=1022,.viewHeightTarget=60,.viewHeightCurrent=60,
        .mins={-15,-15,0},.maxs={15,15,70}};
    pmove_t pm={.ps=&p,.cmd={.buttons=0x100}};pml_t l={0};
    PM_CheckDuck(&pm,&l);assert(p.viewHeightTarget==40 && events[0]==13);
    for(int i=0;i<50;i++)PM_CheckDuck(&pm,&l);assert(eventCount==1);
    finished(&p);PM_CheckDuck(&pm,&l);assert(p.viewHeightTarget==11 && events[1]==11);
    for(int i=0;i<50;i++)PM_CheckDuck(&pm,&l);assert(eventCount==2);
    finished(&p);pm.cmd.buttons=0;PM_CheckDuck(&pm,&l);
    assert(p.viewHeightTarget==40 && events[2]==12);
    for(int i=0;i<50;i++)PM_CheckDuck(&pm,&l);assert(eventCount==3);
    finished(&p);PM_CheckDuck(&pm,&l);assert(p.viewHeightTarget==60 && events[3]==14);
    finished(&p);pm.cmd.buttons=0x200;PM_CheckDuck(&pm,&l);
    assert(p.viewHeightTarget==40 && events[4]==13);finished(&p);
    for(int i=0;i<50;i++)PM_CheckDuck(&pm,&l);assert(eventCount==5);
    blockedHeight=60;pm.cmd.buttons=0;PM_CheckDuck(&pm,&l);
    assert(p.viewHeightTarget==40 && eventCount==5);
    blockedHeight=0;PM_CheckDuck(&pm,&l);assert(p.viewHeightTarget==60 && events[5]==14);
    int directions[][3]={{127,0,0},{127,-127,45},{127,127,-45},{-127,0,0},
        {-127,-127,-45},{-127,127,45},{0,-127,0},{0,127,0}};
    for(int i=0;i<8;i++) {
        pm.cmd.forwardmove=directions[i][0];pm.cmd.rightmove=directions[i][1];
        PM_SetMovementDir(&pm,&l);assert(abs(p.movementDir-directions[i][2])<=1);
    }
    p.movementDir=45;pm.cmd.forwardmove=pm.cmd.rightmove=0;
    PM_SetMovementDir(&pm,&l);assert(p.movementDir==45);
    return 0;
}
'''
movement_source = movement_support.replace('#include <string.h>', '#include <string.h>\n#include <stdlib.h>') + movement_body + movement_checks
check(movement_source, (movement_source.replace('BG_AnimScriptEvent(ps, event, 0, 0);', ''),
                        movement_source.replace('ps->movementDir = (int)yaw;', 'ps->movementDir = 7;')))

anim_support = r'''
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define __attribute_regparm__(n)
typedef int qboolean;
typedef struct {int duration,initialLerp,flags,noteType;float moveSpeed;long long movetype;char *name;} animation_t;
typedef struct {animation_t *animation;int animationNumber,animationTime,oldFrameSnapshotTime;float oldFramePos[3],animSpeedScale;} lerpFrame_t;
typedef struct {lerpFrame_t legs,torso;void *pXAnimTree;int stanceTransitionTime,clientNum,leftHandGun,dobjDirty;} clientInfo_t;
typedef struct {int eFlags;struct {float trBase[3];}pos;} entityState_t;
typedef struct {int clientNum,eFlags;float fWeaponPosFrac;} playerState_t;
typedef struct {playerState_t *ps;struct {int buttons,forwardmove,rightmove;}cmd;} pmove_t;
typedef struct {int playerAnimType,weapClass;} WeaponDef;
static WeaponDef weapon={2,3};
static int conditions[9];
static int BG_GetViewmodelWeaponIndex(playerState_t *p) {return 1;}
static WeaponDef *BG_GetWeaponDef(int n) {return &weapon;}
static void BG_UpdateConditionValue_core(int client,int n,int value,int convert) {conditions[n]=value;}
static struct {int numAnimations,torsoAnim,legsAnim;struct {void *anims;}animTree;animation_t animations[4];} script;
static struct {int time,latestSnapshotTime;typeof(script) animScriptData;struct {struct {int index;}root;}generic_human;} store,*bgs=&store;
#define globalScriptData (&bgs->animScriptData)
static int BG_AnimIndexNoToggle(int n) {return n&~0x200;}
#define Com_Error(...) abort()
static float cleared,goal;
static float weights[4];
static float clipTimes[4];
static int timeSets;
static int XAnimIsLooped(void *tree,int n) {return n==1 || n==2;}
static int XAnimIsPrimitive(void *tree,int n) {return 1;}
static int XAnimGetLengthMsec(void *tree,int n) {return 1000;}
static float XAnimGetTime(void *tree,int n) {return .5f;}
static void XAnimSetTime(void *tree,int n,float t) {clipTimes[n]=t;timeSets++;}
static void XAnimClearGoalWeight(void *tree,int n,float t) {cleared=t;}
static void XAnimSetCompleteGoalWeight(void *tree,int n,float w,float t,float r,int name,int type,int restart) {goal=t;}
static void XAnimSetCompleteGoalWeightKnobAll(void *tree,int n,int root,float w,float t,float r,int name,int restart) {}
static float XAnimGetWeight(void *tree,int n) {return weights[n];}
static void XAnimSetAnimRate(void *tree,int n,float r) {}
static float BG_Fabsf(float f) {return fabsf(f);}
static float Vec3Distance(float *a,float *b) {float x=a[0]-b[0],y=a[1]-b[1],z=a[2]-b[2];return sqrtf(x*x+y*y+z*z);}
'''
anim_support = anim_support.replace('typeof(script)', '__typeof__(script)')
anim_body = ''.join(function('src/PC/bgame/bg_animation_mp.c', n) for n in (
    'BG_AnimUpdatePlayerStateConditions', 'BG_GetAnimationForIndexChecked',
    'BG_AnimationHasMoveClass', 'BG_AnimationHasTurnClass',
    'BG_LerpAnimBlendSeconds', 'BG_CopyLerpFramePosition', 'BG_RunLerpFrameRate'))
anim_checks = r'''
int main(void) {
    playerState_t p={0};pmove_t pm={.ps=&p};
    int cases[][3]={{127,0,0},{0,-127,1},{0,127,2},{127,-127,0},{127,127,0},
        {-127,-127,0},{-127,127,0},{40,-127,1},{40,127,2}};
    for(int i=0;i<9;i++) {
        pm.cmd.forwardmove=cases[i][0];pm.cmd.rightmove=cases[i][1];
        BG_AnimUpdatePlayerStateConditions(&pm);assert(conditions[8]==cases[i][2]);
    }
    p.eFlags=4;BG_AnimUpdatePlayerStateConditions(&pm);assert(conditions[5]==1);
    bgs->animScriptData.numAnimations=4;bgs->time=1000;bgs->latestSnapshotTime=1000;
    animation_t *a=bgs->animScriptData.animations;
    a[1]=(animation_t){.initialLerp=-1,.moveSpeed=190,.movetype=1LL<<10};
    a[2]=(animation_t){.initialLerp=-1,.movetype=1LL<<1};
    a[3]=(animation_t){.initialLerp=-1,.movetype=1LL<<2};
    int crouchTypes[]={2,6,7,12,13,16,17,39,40};
    for(int i=0;i<9;i++) {a[3].movetype=1LL<<crouchTypes[i];assert(BG_AnimationHasMoveClass(3));}
    a[3].movetype=1LL<<2;
    entityState_t es={0};
    int transitions[][3]={{1,2,250},{2,1,120},{2,3,200},{3,2,200},{1,3,250}};
    for(int i=0;i<5;i++) {
        clientInfo_t ci={0};ci.legs.animationNumber=transitions[i][0];ci.legs.animation=&a[transitions[i][0]];
        cleared=goal=-100;BG_RunLerpFrameRate(&ci,&ci.legs,transitions[i][1],&es);
        assert(ci.legs.animationTime==transitions[i][2]);
        assert(fabsf(cleared-transitions[i][2]*.001f)<.00001f && fabsf(goal-cleared)<.00001f);
    }
    for(int explicit=0;explicit<=50;explicit+=50) {
        clientInfo_t ci={0};ci.legs.animationNumber=1;ci.legs.animation=&a[1];a[3].initialLerp=explicit;
        BG_RunLerpFrameRate(&ci,&ci.legs,3,&es);assert(ci.legs.animationTime==explicit);
    }
    a[3].initialLerp=-1;clientInfo_t fresh={0};BG_RunLerpFrameRate(&fresh,&fresh.legs,3,&es);
    assert(fresh.legs.animationTime==0);
    /* Reusing the same death clip must show its intermediate fall frames.
     * Run the real transition path for both toggle bits and every stance. */
    a[3].flags=0x40;
    for(int stance=0;stance<3;stance++) for(int toggle=0;toggle<2;toggle++) {
        int deathAnim=3 | (toggle?0x200:0);
        clientInfo_t ci={0};ci.legs.animationNumber=deathAnim^0x200;ci.legs.animation=&a[3];
        clipTimes[3]=1;weights[3]=1;es.eFlags=0x80000 | (stance==2?8:stance==1?4:0);
        int before=timeSets;BG_RunLerpFrameRate(&ci,&ci.legs,deathAnim,&es);
        assert(clipTimes[3]==0 && timeSets==before+1);
        for(int frame=1;frame<=20;frame++) {
            clipTimes[3]=frame*.05f;
            BG_RunLerpFrameRate(&ci,&ci.legs,deathAnim,&es);
            assert(fabsf(clipTimes[3]-frame*.05f)<.00001f && timeSets==before+1);
            assert(ci.legs.animSpeedScale==1);
        }
    }
    /* Bodies first seen after the death has ended still use the final pose. */
    clientInfo_t finished={0};es.eFlags=0;clipTimes[3]=0;
    BG_RunLerpFrameRate(&finished,&finished.legs,3,&es);assert(clipTimes[3]==1);
    return 0;
}
'''
anim_source = anim_support + anim_body + anim_checks
check(anim_source, (anim_source.replace('lf->animationTime = anim != NULL && anim->moveSpeed != 0.0f ? 120 :',
                                        'lf->animationTime = -1; (void)(anim != NULL && anim->moveSpeed != 0.0f ? 120 :')
                            .replace('oldAnimation != NULL && oldAnimation->moveSpeed != 0.0f ? 250 : 170;',
                                     'oldAnimation != NULL && oldAnimation->moveSpeed != 0.0f ? 250 : 170);'),
                    anim_source.replace('XAnimSetTime(pAnimTree, animNum, 0.0f);', '')))

prepare = (root / 'scripts/prepare-browser-bootstrap.py').read_text()
start = prepare.index('default_cfg = archives[')
end = prepare.index('\n\n# Follow asset names', start)
archives = {'cod2_browser_bootstrap.iwd': {'default_mp.cfg':
    b'bind SHIFT\t"+melee_breath"\r\nbind v\t"mp_QuickMessage"\r\n'
    b'bind c\t\t"gocrouch"\r\nbind SPACE\t"+gostand"\r\n'}}
exec(prepare[start:end], {'archives': archives})
cfg = archives['cod2_browser_bootstrap.iwd']['default_mp.cfg']
assert b'bind c\t\t"togglecrouch"' in cfg
assert b'bind SPACE\t"+gostand"' in cfg
print('PASS: posture inputs/transitions, movement directions and blends, six recycled death clips with 20 continuous fall frames each and completed death poses; five mutants fail')
