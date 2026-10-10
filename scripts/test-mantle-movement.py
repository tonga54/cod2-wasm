#!/usr/bin/env python3
"""Exercise production mantle traces, root motion, Pmove dispatch and jumps."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent


def function(path, name):
    source = (root / path).read_text()
    match = re.search(r'^([^\n]*\b' + name + r'\([^;]*?\))\s*\{', source, re.M)
    assert match, name
    brace = source.index('{', match.start())
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


common = r'''
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define __attribute_regparm__(x)
#define PMF_MANTLE 4
#define PMF_LADDER 32
#define PMF_SPRINT 0x02000000
#define ENTITYNUM_NONE 1023
#define qtrue 1
#define qfalse 0
typedef int Bool,qboolean;typedef unsigned char byte;typedef float vec_t,vec3_t[3];
typedef struct {float yaw;int timer,transIndex,flags;} MantleState;
typedef struct {int upAnimIndex,overAnimIndex;float height;} MantleAnimTransition;
typedef struct {vec3_t dir,startPos,ledgePos,endPos;int flags,duration;} MantleResults;
typedef struct {float fraction;vec3_t normal;int surfaceFlags,contents,allsolid,startsolid;} trace_t;
typedef struct {struct {int enabled;float value;} current;} dvar_t;
typedef struct {int pm_type,pm_flags,eFlags,weaponstate,clientNum,groundEntityNum;
    int commandTime,gravity,pm_time,jumpTime,delta_angles[3];
    vec3_t origin,velocity,mins,maxs,viewangles,vLadderVec;
    float aimSpreadScale,jumpOriginZ,leanf;MantleState mantleState;} playerState_t;
typedef struct {int serverTime,buttons,forwardmove,rightmove;} usercmd_t;
typedef struct {playerState_t *ps;usercmd_t cmd,oldcmd;int numtouch,mantleStarted;
    int tracemask,handler,mantleDuration;vec3_t mins,maxs,mantleEndPos;float xyspeed;} pmove_t;
typedef struct {int msec,walking,groundPlane,almostGroundPlane,previous_waterlevel;
    float frametime;vec3_t forward,right,up,previous_origin,previous_velocity;} pml_t;
static float Vec3Normalize(float *v) {
    float len=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
    if(len)for(int i=0;i<3;i++)v[i]/=len;return len;
}
static float vectoyaw(float *v) {return atan2f(v[1],v[0])*57.295779513f;}
static float fabsf_local(float f) {return fabsf(f);}
static void VectorAngleMultiply(float *v,float yaw) {
    float x=v[0],y=v[1],a=yaw*.01745329252f;
    v[0]=x*cosf(a)-y*sinf(a);v[1]=x*sinf(a)+y*cosf(a);
}
static int BG_AnimScriptAnimation(playerState_t *ps,int a,int b,int c) {return 0;}
static int BG_AnimScriptEvent(playerState_t *ps,int a,int b,int c) {return 0;}
static void BG_AddPredictableEventToPlayerstate(int a,int b,playerState_t *ps) {}
static void Com_Printf(const char *fmt,...) {}
static const char *va(const char *fmt,...) {return fmt;}
'''
mantle_source = (root / 'src/PC/bgame/bg_mantle.c').read_text()
table = re.search(r'UInt32 s_mantleTrans\[24\] = \{.*?\n\};', mantle_source, re.S)[0]
mantle_support = table.replace('UInt32', 'uint32_t') + r'''
#define s_mantleTrans ((MantleAnimTransition *)s_mantleTrans)
static void *s_mantleAnims=(void *)1;
static dvar_t on={.current.enabled=1},off,range={.current.value=20};
static dvar_t radius={.current.value=.1f},angle={.current.value=60};
static dvar_t *mantle_enable=&on,*mantle_debug=&off,*mantle_check_range=&range;
static dvar_t *mantle_check_radius=&radius,*mantle_check_angle=&angle;
static int wallFlags=0x2000000,solid,headroom=1,overClear=1;
static float wallFraction=.5f,ledgeHeight=139,normalZ=1,heading;
static int XAnimGetLengthMsec(void *anims,int index) {assert(anims);return index<8?200:100;}
static void XAnimGetAbsDelta(void *anims,int index,float *rot,float *delta,float frac) {
    assert(anims&&frac>=0&&frac<=1);
    delta[0]=(index<8?16:31)*frac;delta[1]=0;
    delta[2]=(index<8?s_mantleTrans[index-1].height:-18)*frac;
}
static void PM_trace(pmove_t *pm,void *raw,float *start,float *mins,float *maxs,
    float *end,int client,int mask) {
    trace_t *tr=raw;memset(tr,0,sizeof(*tr));tr->fraction=1;
    if(mask==0x1000000) {
        tr->fraction=wallFraction;tr->surfaceFlags=wallFlags;
        tr->allsolid=(solid==1);tr->startsolid=(solid==2);
        tr->normal[0]=-cosf(heading);tr->normal[1]=-sinf(heading);return;
    }
    if(end[2]<start[2]&&start[0]==end[0]&&start[1]==end[1]) {
        if(ledgeHeight<start[2]&&ledgeHeight>end[2]) {
            tr->fraction=(start[2]-ledgeHeight)/(start[2]-end[2]);tr->normal[2]=normalZ;
        }
    } else if(!memcmp(start,end,sizeof(vec3_t))) {
        tr->startsolid=!headroom;
    } else if(end[2]==start[2]) {
        if(start[2]<ledgeHeight)tr->fraction=.5f;
    } else if(!overClear)tr->fraction=.5f;
}
'''
mantle_body = ''.join(function('src/PC/bgame/bg_mantle.c', name) for name in [
    'Mantle_GetAnimDelta', 'Mantle_Move', 'Mantle_CheckLedge', 'Mantle_Check'])
mantle_checks = r'''
static playerState_t player(void) {
    playerState_t ps={.origin={20,30,100},.mins={-15,-15,0},.maxs={15,15,72}};
    return ps;
}
int main(void) {
    /* Reachable ledges include heights above a normal 39-unit jump. */
    for(int h=21;h<=57;h+=6)for(int yaw=0;yaw<360;yaw+=30)for(int over=0;over<2;over++) {
        playerState_t ps=player();pmove_t pm={.ps=&ps,.tracemask=1};
        pml_t pml={.msec=17};heading=yaw*.01745329252f;
        pml.forward[0]=cosf(heading);pml.forward[1]=sinf(heading);
        ledgeHeight=100+h;wallFlags=over?0x4000000:0x2000000;
        Mantle_Check(&pm,&pml);
        assert(!(ps.pm_flags&4)&&(ps.mantleState.flags&8));
        assert(ps.origin[2]==100&&!pm.mantleStarted);
        pm.cmd.buttons=0x400;Mantle_Check(&pm,&pml);
        assert((ps.pm_flags&4)&&pm.mantleStarted);
        int total=pm.mantleDuration;assert(total==(over?300:200));
        for(int frame=0;ps.pm_flags&4;frame++) {
            assert(frame<20);Mantle_Move(&pm,&ps,&pml);
            for(int a=0;a<3;a++)assert(isfinite(ps.origin[a])&&isfinite(ps.velocity[a]));
        }
        float distance=over?47:16;
        assert(fabsf(ps.origin[0]-(20+distance*cosf(heading)))<.001f);
        assert(fabsf(ps.origin[1]-(30+distance*sinf(heading)))<.001f);
        assert(fabsf(ps.origin[2]-(ledgeHeight-(over?18:0)))<.001f);
        assert(ps.mantleState.timer==total&&!pm.mantleStarted);
    }
    for(int reason=0;reason<9;reason++) {
        playerState_t ps=player();pmove_t pm={.ps=&ps,.tracemask=1,.cmd.buttons=0x400};
        pml_t pml={.forward={1,0,0}};heading=0;solid=0;wallFraction=.5f;
        wallFlags=0x2000000;headroom=1;normalZ=1;ledgeHeight=139;
        if(reason==0)solid=1;if(reason==1)solid=2;if(reason==2)wallFraction=1;
        if(reason==3)wallFlags=0;if(reason==4)headroom=0;if(reason==5)normalZ=.5f;
        if(reason==6)ledgeHeight=161;if(reason==7)ps.pm_type=6;if(reason==8)ps.eFlags=8;
        Mantle_Check(&pm,&pml);assert(!(ps.pm_flags&4)&&!pm.mantleStarted);
    }
    return 0;
}
'''
dispatch_support = r'''
static int checks,moves,slides,weapons,groundCalls,noclip,canMantle=1,adsTicks,adsTime,adsMsec;
static void PM_VectorCopy(const float *a,float *b) {memcpy(b,a,sizeof(vec3_t));}
static float PM_VectorLength2D(float *a) {return hypotf(a[0],a[1]);}
static void BG_AnimUpdatePlayerStateConditions(pmove_t *pm) {}
int PM_WeaponAmmoAvailable(playerState_t *ps) {return 1;}
static void PM_ResetWeaponState(playerState_t *ps) {}
static void PM_DropTimers(playerState_t *ps,int ms) {}
static void PM_AdjustAimSpreadScale(pmove_t *pm,pml_t *pml) {}
static void PM_UpdateViewAngles(playerState_t *ps,float ms,usercmd_t *cmd,int handler) {}
static void AngleVectors(float *a,float *f,float *r,float *u) {f[0]=1;r[1]=1;u[2]=1;}
static void PM_CheckDuck(pmove_t *pm,pml_t *pml) {}
static void PM_UpdateAimDownSightFlag(pmove_t *pm,pml_t *pml) {}
static void PM_GroundTrace(pmove_t *pm,pml_t *pml) {groundCalls++;pml->walking=1;pm->ps->groundEntityNum=1022;}
static void PM_CheckLadderMove(pmove_t *pm,pml_t *pml) {}
static void PM_UpdateSprint(pmove_t *pm,pml_t *pml) {}
static void PM_LadderMove(pmove_t *pm,pml_t *pml) {slides++;}
static void PM_WalkMove(pmove_t *pm,pml_t *pml) {slides++;}
static void PM_AirMove(pmove_t *pm,pml_t *pml) {slides++;}
static void PM_NoclipMove(pmove_t *pm,pml_t *pml) {noclip++;}
static void PM_Footsteps(pmove_t *pm,pml_t *pml) {}
static void PM_Weapon(pmove_t *pm,pml_t *pml) {assert(adsTime==pm->cmd.serverTime);weapons++;}
static void PM_UpdateAimDownSightLerp(pmove_t *pm,pml_t *pml) {adsTicks++;adsMsec+=pml->msec;adsTime=pm->cmd.serverTime;}
static void PM_ViewHeightAdjust(pmove_t *pm,pml_t *pml) {}
static void Sys_SnapVector(float *v) {}
static void Mantle_Check(pmove_t *pm,pml_t *pml) {
    checks++;
    if(canMantle&&(pm->cmd.buttons&0x400)&&!(pm->ps->pm_flags&4)) {
        pm->ps->pm_flags|=4;pm->mantleStarted=1;
    }
}
static void Mantle_Move(pmove_t *pm,playerState_t *ps,pml_t *pml) {
    moves++;assert(!(ps->pm_flags&32)&&ps->groundEntityNum==1023);
    ps->mantleState.timer+=pml->msec;ps->origin[2]+=pml->msec;
    if(ps->mantleState.timer>=200){ps->pm_flags&=~4;pm->mantleStarted=0;}
}
'''
dispatch_body = function('src/PC/bgame/bg_pmove.c', 'Pmove')
dispatch_checks = r'''
int main(void) {
    for(int packet=1;packet<200;packet++) {
        playerState_t ps={.commandTime=1000,.pm_flags=32};
        pmove_t pm={.ps=&ps,.cmd={.serverTime=1000+packet,.buttons=0x400}};
        checks=moves=slides=weapons=groundCalls=noclip=adsTicks=adsTime=adsMsec=0;Pmove(&pm);
        assert(ps.origin[2]==packet&&ps.mantleState.timer==packet&&ps.commandTime==1000+packet);
        assert(moves==(packet+65)/66&&checks==moves&&weapons==moves&&!slides&&!groundCalls);
        assert(pm.mantleStarted&&(ps.pm_flags&4));
        assert(adsTicks==moves&&adsMsec==packet);
    }
    for(int mode=0;mode<10;mode++)for(int attempt=0;attempt<2;attempt++) {
        playerState_t ps={.commandTime=1000,.pm_type=mode,.leanf=1};
        pmove_t pm={.ps=&ps,.mantleStarted=1,.cmd={.serverTime=1200,.buttons=0x400}};
        canMantle=attempt;checks=moves=slides=weapons=groundCalls=noclip=adsTicks=adsTime=adsMsec=0;Pmove(&pm);
        assert(ps.commandTime==1200&&!pm.mantleStarted);
        if(mode==6||mode==7)assert(ps.leanf==0);
        if(mode<=5){assert(checks==4&&weapons==4);assert(attempt?moves==4&&slides==0:moves==0&&slides==4);}
        else {assert(!checks&&!moves&&!slides&&!weapons);assert(noclip==(mode>=8?4:0));}
        assert(adsTicks==(mode==6||mode==7?0:4));
        assert(adsMsec==(mode==6||mode==7?0:200));
    }
    return 0;
}
'''
jump_support = r'''
static const dvar_t *jump_height,*jump_stepSize,*jump_slowdownEnable,*jump_ladderPushVel,*jump_spreadAdd;
static dvar_t vars[5];static int varsUsed;
static const dvar_t *Dvar_RegisterFloat(const char *s,float v,float lo,float hi,int flags) {
    vars[varsUsed].current.value=v;return &vars[varsUsed++];
}
static const dvar_t *Dvar_RegisterBool(const char *s,int v,int flags) {
    vars[varsUsed].current.enabled=v;return &vars[varsUsed++];
}
static int PM_GetEffectiveStance(playerState_t *ps) {return 0;}
static void PM_AddEvent(playerState_t *ps,int event) {}
static int PM_GroundSurfaceType(pml_t *pml) {return 0;}
'''
jump_body = ''.join(function('src/PC/bgame/bg_jump.c', name) for name in [
    'Jump_RegisterDvars', 'Jump_Check'])
jump_checks = r'''
int main(void) {
    Jump_RegisterDvars();assert(jump_height->current.value==39);
    for(int gravity=400;gravity<=1200;gravity+=100) {
        playerState_t ps={.origin={0,0,100},.gravity=gravity};
        pmove_t pm={.ps=&ps,.cmd={.serverTime=1000,.buttons=0x400}};
        pml_t pml={.walking=1,.groundPlane=1};
        assert(Jump_Check(&pm,&pml)&&!pml.walking&&!pml.groundPlane);
        assert(fabsf(ps.velocity[2]*ps.velocity[2]/(2*gravity)-39)<.00001f);
        assert(ps.jumpOriginZ==100&&ps.groundEntityNum==1023);
    }
    return 0;
}
'''
suites = [
    ('ledge', mantle_support, mantle_body, mantle_checks, [
        mantle_body.replace('if (tr->allsolid || tr->startsolid)',
                            'if (tr->allsolid == 0 && tr->startsolid == 0)')]),
    ('dispatch', dispatch_support, dispatch_body, dispatch_checks, [
        dispatch_body.replace('Mantle_Check(pm, &pml);', ''),
        dispatch_body.replace('if (ps->pm_flags & PMF_MANTLE)', 'if (0)'),
        dispatch_body.replace('PM_UpdateAimDownSightLerp(pm, &pml);', 'PM_UpdateAimDownSightLerp(pm, &pml); PM_UpdateAimDownSightLerp(pm, &pml);')]),
    ('jump', jump_support, jump_body, jump_checks, []),
]
with tempfile.TemporaryDirectory(prefix='cod2-mantle-') as directory:
    path = Path(directory)
    for name, support, body, checks, mutants in suites:
        assert all(mutant != body for mutant in mutants)
        for index, variant in enumerate([body] + mutants):
            source, binary = path / (name + '.c'), path / name
            source.write_text(common + support + variant + checks)
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                            str(source), '-lm', '-o', str(binary)], check=True)
            result = subprocess.run([str(binary)], capture_output=True, text=True)
            assert (result.returncode == 0) == (index == 0), (name, index, result.stderr)
print('PASS: 168 ledge/root-motion paths, 9 invalid ledges, 219 movement schedules with one ADS integration before firing, original jump height and 4 failing regression mutants')
