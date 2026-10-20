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
typedef struct {int cl_stance,cl_stanceTemp;} LegacyHacks;
static LegacyHacks legacy,*legacyPtr=&legacy;
static void *imp_legacyHacks=&legacyPtr;
typedef struct {int keyCatchers,cgameInShellshock,stanceHeld,stancePosition,stanceTime,usingAds;struct {struct {int pm_type,pm_flags,viewHeightTarget;}ps;}snap;} clientActive_t;
static clientActive_t client,*clientPtr=&client;
static void *imp_cl=&clientPtr;
static struct {struct {int enabled;}current;} bypass,*cl_bypassMouseInput=&bypass;
static unsigned int frameMsec=16;
static void *imp_frame_msec=&frameMsec;
static int frameTime;
static void *imp_com_frameTime=&frameTime;
static struct {struct {int integer;}current;} stanceHold={.current.integer=300},*cl_stanceHoldTime=&stanceHold;
#define BUTTON_SPRINT 2
static int CL_KeyMoveValue(kbutton_t *b){return b->active?127:0;}
static int ClampChar(int n){return n>127?127:n<-127?-127:n;}
static int PM_IsBinocularsADS(void *ps){return 0;}
static const char *Cmd_Argv(int n) {return n==1 ? "32" : "1000";}
#define Com_Printf(...) ((void)0)
'''
input_body = ''.join(function('src/PC/client_mp/cl_input.c', n) for n in (
    'IN_KeyDown', 'IN_KeyUp', 'IN_ToggleCrouch', 'IN_GoProne', 'IN_GoStandDown',
    'IN_GoStandUp', 'IN_Stance_Down', 'CL_UpdateHeldStance', 'IN_Stance_Up',
    'CL_ConsumeButtonPress', 'CL_CmdButtons', 'CL_KeyMove'))
input_checks = r'''
int main(void) {
    // Tap alternates crouch/stand (also stands from prone); holding goes
    // prone at 300ms, including a release after a delayed render frame.
    for(int posture=0;posture<=2;posture++)for(int duration=0;duration<=600;duration+=10){
        memset(kb,0,sizeof(kb));client.stanceHeld=0;legacy.cl_stance=posture;frameTime=1000;
        IN_Stance_Down();frameTime+=duration;
        if(duration>=300){CL_UpdateHeldStance();assert(legacy.cl_stance==2);}
        IN_Stance_Up();assert(legacy.cl_stance==(duration>=300?2:(posture==0?1:0)));
        assert(!client.stanceHeld);
        legacy.cl_stance=posture;frameTime=2000;IN_Stance_Down();frameTime+=duration;
        IN_Stance_Up();assert(legacy.cl_stance==(duration>=300?2:(posture==0?1:0)));
    }
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
    // Space cancels both held and toggled stance inputs, including a low
    // server posture while the local toggle already says standing.
    for(int key=0;key<4;key++) {
        memset(kb,0,sizeof(kb));client.snap.ps.pm_type=0;
        client.snap.ps.pm_flags=key==3?1:0;legacy.cl_stance=0;
        if(key<3)kb[key==0?11:key==1?25:26].active=1;
        IN_GoStandDown();assert(!kb[10].active&&!kb[25].active&&!kb[26].active&&!kb[11].active);
        assert(!legacy.cl_stance&&!legacy.cl_stanceTemp);
        IN_GoStandDown();assert(!kb[10].active); // Key repeat cannot turn stand into jump.
        IN_GoStandUp();client.snap.ps.pm_flags=0;client.snap.ps.viewHeightTarget=60;
        IN_GoStandDown();assert(kb[10].active);IN_GoStandUp();
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

sprint_checks = r'''
int main(void){
 for(int posture=1;posture<=2;posture++)for(int held=0;held<3;held++){
  memset(kb,0,sizeof(kb));client=(clientActive_t){0};legacy=(LegacyHacks){posture,1};
  client.stanceHeld=1;kb[(int[]){11,25,26}[held]].active=1;kb[2].active=1;
  kb[(int[]){11,25,26}[held]].down[0]=32;
  usercmd_t cmd={.buttons=BUTTON_SPRINT|0x300};CL_KeyMove(&cmd);
  assert(cmd.forwardmove==127&&!(cmd.buttons&0x300)&&!legacy.cl_stance&&!legacy.cl_stanceTemp);
  assert(!client.stanceHeld&&!kb[11].active&&!kb[25].active&&!kb[26].active);
  assert(!kb[11].down[0]&&!kb[25].down[0]&&!kb[26].down[0]);
  cmd=(usercmd_t){0};CL_KeyMove(&cmd);assert(!(cmd.buttons&0x300));
  IN_KeyDown(&kb[(int[]){11,25,26}[held]]);
  assert((kb[(int[]){11,25,26}[held]].active));
 }
 /* Shift while aiming keeps the posture used for scoped Hold Breath. */
 client.usingAds=1;legacy.cl_stance=2;usercmd_t cmd={.buttons=BUTTON_SPRINT};CL_KeyMove(&cmd);
 assert((cmd.buttons&0x1000)&&(cmd.buttons&0x100)&&legacy.cl_stance==2);
 return 0;
}
'''
sprint_source = input_support + input_body + sprint_checks
check(sprint_source, (sprint_source.replace('cl->snap.ps.pm_type == 0 && (cmd->buttons & BUTTON_SPRINT)', '0'),))

movement_support = r'''
#include <assert.h>
#include <math.h>
#include <string.h>
#define PM_REGPARM2_ABI
#define BUTTON_SPRINT 2
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

# Replay the real view-height timer, rather than completing camera transitions
# manually, through both crouch->stand and prone->crouch->stand.
move_source = (root / 'src/PC/bgame/bg_pmove.c').read_text()
timed_support = movement_support.replace('viewHeightLerpTime,movementDir;',
    'viewHeightLerpTime,viewHeightLerpTarget,viewHeightLerpDown,movementDir;')
timed_support = timed_support.replace('vec3_t origin,mins,maxs,viewangles;',
    'vec3_t origin,mins,maxs,viewangles,velocity;')
timed_support = timed_support.replace('proneDirectionPitch,proneTorsoPitch;',
    'proneDirectionPitch,proneTorsoPitch,viewHeightLerpPosAdj;')
timed_support = timed_support.replace('typedef struct {int unused;} pml_t;',
    'typedef struct {float frametime;vec3_t forward;} pml_t;')
start = timed_support.index('/* Pause/resume the camera lerp explicitly:')
timed_support = timed_support[:start] + r'''
typedef struct {int iFrac;float fViewHeight;int iOffset;} viewLerpWaypoint_t;
static void Vec3Normalize(float *v){}
static void PM_StepSlideMove(pmove_t *pm,pml_t *pml,int gravity){assert(0);}
'''
tables = move_source[move_source.index('viewLerpWaypoint_t viewLerp_CrouchProne[13] ='):]
timed_body = ''.join(function('src/PC/bgame/bg_pmove.c', n) for n in (
    'PM_GetViewHeightLerp', 'PM_ViewHeightAdjust', 'PM_CheckDuck'))
timed_checks = r'''
int main(void){
 for(int posture=1;posture<=2;posture++)for(int dt=4;dt<=66;dt++)for(int sprint=0;sprint<2;sprint++){
  playerState_t ps={.groundEntityNum=1022,.pm_flags=posture==1?2:1,
    .viewHeightTarget=posture==1?40:11,.viewHeightCurrent=posture==1?40:11,
    .mins={-15,-15,0},.maxs={15,15,70}};
  pmove_t pm={.ps=&ps};pml_t pml={.frametime=dt*.001f};eventCount=blockedHeight=0;
  for(int t=1000;t<2500;t+=dt){
   pm.cmd.serverTime=t;pm.cmd.forwardmove=sprint?127:0;
   pm.cmd.buttons=sprint?(BUTTON_SPRINT|(posture==1?0x200:0x100)):0;
   PM_CheckDuck(&pm,&pml);
  }
  assert(ps.viewHeightTarget==60&&ps.viewHeightCurrent==60&&!(ps.pm_flags&3));
  assert(eventCount==(posture==1?1:2));
 }
 playerState_t ps={.groundEntityNum=1022,.pm_flags=1,.viewHeightTarget=11,.viewHeightCurrent=11,
    .mins={-15,-15,0},.maxs={15,15,70}};
 pmove_t pm={.ps=&ps,.cmd={.buttons=BUTTON_SPRINT,.forwardmove=127}};pml_t pml={.frametime=.016f};eventCount=0;blockedHeight=60;
 for(int t=1000;t<2000;t+=16){pm.cmd.serverTime=t;PM_CheckDuck(&pm,&pml);}
 assert(ps.viewHeightTarget==40&&ps.viewHeightCurrent==40); // Low ceiling still blocks standing.
 blockedHeight=0;
 for(int t=2000;t<3000;t+=16){pm.cmd.serverTime=t;PM_CheckDuck(&pm,&pml);}
 assert(ps.viewHeightTarget==60&&ps.viewHeightCurrent==60);
 return 0;
}
'''
check(timed_support + tables + timed_body + timed_checks)

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
enum {TR_STATIONARY,TR_INTERPOLATE,TR_LINEAR,TR_LINEAR_STOP};
typedef struct {int eFlags;struct {int trType,trTime;float trBase[3];}pos;} entityState_t;
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
    /* A smoothed runner's snapshots carry none, one or several of its
     * commands; its stride keeps the pace of its command time. */
    {
        const int commands[]={33,16,50,0,33,17,49,66,0,33,16,17};
        clientInfo_t ci={0};ci.legs.animationNumber=1;ci.legs.animation=&a[1];
        entityState_t run={0};run.pos.trType=TR_LINEAR_STOP;
        int command=1000;bgs->latestSnapshotTime=1000;run.pos.trTime=command;
        BG_RunLerpFrameRate(&ci,&ci.legs,1,&run);
        for(int i=0;i<120;i++) {
            float before=ci.legs.animSpeedScale;
            bgs->latestSnapshotTime+=33;command+=commands[i%12];
            run.pos.trTime=command;run.pos.trBase[0]=190*.001f*(command-1000);
            BG_RunLerpFrameRate(&ci,&ci.legs,1,&run);
            if(commands[i%12]) assert(fabsf(ci.legs.animSpeedScale-1)<.001f);
            else assert(ci.legs.animSpeedScale==before);
        }
        /* Unsmoothed positions keep measuring over snapshot time, and a switch
         * to command time restarts the measurement instead of reversing it. */
        run.pos.trType=TR_INTERPOLATE;run.pos.trTime=0;
        bgs->latestSnapshotTime+=33;BG_RunLerpFrameRate(&ci,&ci.legs,1,&run);
        for(int i=0;i<10;i++) {
            int gap=i%2?66:33;
            bgs->latestSnapshotTime+=gap;run.pos.trBase[0]+=190*.001f*gap;
            BG_RunLerpFrameRate(&ci,&ci.legs,1,&run);
            assert(fabsf(ci.legs.animSpeedScale-1)<.001f);
        }
        run.pos.trType=TR_LINEAR_STOP;run.pos.trTime=bgs->latestSnapshotTime-100;
        run.pos.trBase[0]+=500;BG_RunLerpFrameRate(&ci,&ci.legs,1,&run);
        assert(fabsf(ci.legs.animSpeedScale-1)<.001f);
        run.pos.trTime+=33;run.pos.trBase[0]+=190*.001f*33;
        BG_RunLerpFrameRate(&ci,&ci.legs,1,&run);
        assert(fabsf(ci.legs.animSpeedScale-1)<.001f);
    }
    return 0;
}
'''
anim_source = anim_support + anim_body + anim_checks
check(anim_source, (anim_source.replace('lf->animationTime = anim != NULL && anim->moveSpeed != 0.0f ? 120 :',
                                        'lf->animationTime = -1; (void)(anim != NULL && anim->moveSpeed != 0.0f ? 120 :')
                            .replace('oldAnimation != NULL && oldAnimation->moveSpeed != 0.0f ? 250 : 170;',
                                     'oldAnimation != NULL && oldAnimation->moveSpeed != 0.0f ? 250 : 170);'),
                    anim_source.replace('XAnimSetTime(pAnimTree, animNum, 0.0f);', ''),
                    anim_source.replace('sampleTime = es->pos.trType == TR_LINEAR_STOP ? es->pos.trTime : bgs->latestSnapshotTime;',
                                        'sampleTime = bgs->latestSnapshotTime;'),
                    anim_source.replace('} else if (lf->oldFrameSnapshotTime - sampleTime > 0) {', '} else if (0) {')))

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
print('PASS: posture inputs/transitions, movement directions and blends, six recycled death clips with 20 continuous fall frames each and completed death poses, strides at the pace of irregular command times; eight mutants fail')
