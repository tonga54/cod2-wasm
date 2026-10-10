#!/usr/bin/env python3
"""Replay remote movement through the production trajectory/interpolation code."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_ents_mp.c').read_text()
trajectory = (root / 'src/PC/bgame/bg_misc.c').read_text()


def function(text, name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;]*?\)\n\{', text)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef float vec_t;
typedef float vec3_t[3];
typedef int qboolean;
enum { TR_STATIONARY, TR_INTERPOLATE, TR_LINEAR, TR_LINEAR_STOP, TR_SINE,
       TR_GRAVITY, TR_GRAVITY_PAUSED, TR_ACCELERATE, TR_DECELERATE };
typedef struct { int trType, trTime, trDuration; vec3_t trBase, trDelta; } trajectory_t;
typedef struct {
    int number, eType, eFlags, clientNum;
    trajectory_t pos, apos;
    vec3_t angles2;
    float leanf;
} entityState_t;
typedef struct { entityState_t currentState, nextState; vec3_t lerpOrigin, lerpAngles; } centity_t;
typedef struct { float lerpMoveDir, lerpLean; vec3_t playerAngles; } clientInfo_t;
typedef struct { int serverTime; struct { int clientNum; } ps; } snapshot_t;
static struct {
    int time;
    float frameInterpolation;
    snapshot_t *snap, *nextSnap;
    struct { clientInfo_t clientinfo[64]; } bgs;
} state, *cg = &state;
static snapshot_t snaps[2];
static void BG_Vec3Copy(const float *a, float *b) { memcpy(b,a,sizeof(vec3_t)); }
static void BG_Vec3Mad(const float *a,float f,const float *v,float *b) {
    for(int i=0;i<3;i++) b[i]=a[i]+f*v[i];
}
static float Vec3NormalizeTo(const float *a,float *b) {
    float n=sqrtf(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
    for(int i=0;i<3;i++) b[i]=n?a[i]/n:0;
    return n;
}
static void Com_Error(int code,const char *fmt,int type) { abort(); }
static float LerpAngle(float a,float b,float f) {
    float d=fmodf(b-a+540,360)-180;
    return a+d*f;
}
static void CG_TraceRemoteMotion(const centity_t *cent) {}
'''

checks = r'''
static centity_t player;
static void Sample(entityState_t *s,int stamp,int command,float speed,int quantized) {
    memset(s,0,sizeof(*s));
    s->number=s->clientNum=2; s->eType=1;
    s->pos.trType=TR_LINEAR_STOP; s->pos.trTime=command; s->pos.trDuration=50;
    s->pos.trDelta[0]=speed;
    float p=100.0f+speed*command*.001f;
    s->pos.trBase[0]=quantized?(float)(int)p:p;
    s->apos.trBase[0]=10; s->apos.trBase[1]=350;
}
static void Setup(int s0,int s1,int c0,int c1,int time,float speed,int quantized) {
    memset(&state,0,sizeof(state));
    snaps[0].serverTime=s0; snaps[1].serverTime=s1;
    snaps[0].ps.clientNum=snaps[1].ps.clientNum=1;
    state.snap=&snaps[0]; state.nextSnap=&snaps[1]; state.time=time;
    state.frameInterpolation=s1>s0?(float)(time-s0)/(s1-s0):0;
    Sample(&player.currentState,s0,c0,speed,quantized);
    Sample(&player.nextState,s1,c1,speed,quantized);
}
static void Evaluate(void) { CG_InterpolateEntityPosition(&player); }
static void OldEvaluate(float *out) {
    vec3_t a,b;
    BG_EvaluateTrajectory(&player.currentState.pos,state.snap->serverTime,a);
    BG_EvaluateTrajectory(&player.nextState.pos,state.nextSnap->serverTime,b);
    for(int i=0;i<3;i++) out[i]=a[i]+(b[i]-a[i])*state.frameInterpolation;
}
static void Fallback(void) {
    vec3_t old;
    OldEvaluate(old); Evaluate();
    for(int i=0;i<3;i++) assert(fabsf(player.lerpOrigin[i]-old[i])<.001f);
}
int main(void) {
    const int frames[]={7,8,16,17,33};
    const float speeds[]={100,190,250};
    double oldError=0,newError=0;
    int count=0;
    /* Actual 20 Hz snapshots, alternating 35/65 ms command samples. Their
     * integer-quantized positions reproduce live Carentan trajectory traces. */
    for(int q=0;q<2;q++) for(int f=0;f<5;f++) for(int v=0;v<3;v++) {
        float previous=0,oldPrevious=0;
        for(int time=1000;time<7000;time+=frames[f]) {
            int s0=time/50*50,s1=s0+50;
            int c0=s0-(s0/50%2?80:65),c1=s1-(s1/50%2?80:65);
            Setup(s0,s1,c0,c1,time,speeds[v],q);
            vec3_t old; OldEvaluate(old); Evaluate();
            float ideal=100+speeds[v]*time*.001f;
            assert(fabsf(player.lerpOrigin[0]-ideal)<(q?1.1f:.002f));
            if(time>1000) {
                double step=player.lerpOrigin[0]-previous;
                double oldStep=old[0]-oldPrevious;
                double expected=speeds[v]*frames[f]*.001;
                assert(step>0); /* No frozen frames during normal running. */
                oldError+=(oldStep-expected)*(oldStep-expected);
                newError+=(step-expected)*(step-expected);
                ++count;
            }
            previous=player.lerpOrigin[0]; oldPrevious=old[0];
        }
    }
    assert(newError<oldError*.30);
    printf("PASS: %d movement frames; step-error RMS %.4f -> %.4f units (%.1f%% lower)\n",
        count,sqrt(oldError/count),sqrt(newError/count),100*(1-sqrt(newError/oldError)));

    /* C1 velocity continuity and no overshoot when stops/reversals require
     * tangent clipping. Include all tangent signs and high sprint velocities. */
    for(int va=-1000;va<=1000;va+=100) for(int vb=-1000;vb<=1000;vb+=100)
    for(int dir=-1;dir<=1;dir+=2) for(int f=0;f<=100;f++) {
        float a=10,b=a+dir*12;
        float p=CG_PlayerMotionBlend(a,b,va,vb,.065f,f*.01f);
        assert(p>=fminf(a,b)-.001f && p<=fmaxf(a,b)+.001f);
        if(f) {
            float prev=CG_PlayerMotionBlend(a,b,va,vb,.065f,(f-1)*.01f);
            assert((p-prev)*dir>=-.001f);
        }
    }
    float da=(CG_PlayerMotionBlend(0,9.5f,190,190,.05f,.001f)-0)/.00005f;
    float db=(9.5f-CG_PlayerMotionBlend(0,9.5f,190,190,.05f,.999f))/.00005f;
    assert(fabsf(da-190)<1 && fabsf(db-190)<1);

    /* A missing next packet bridges one additional 50 ms interval, then stops.
     * Use diagonal/horizontal/vertical motion and a long network outage. */
    for(int time=1020;time<8000;time+=10) {
        Setup(1000,1000,950,950,time,250,0);
        player.currentState.pos.trDelta[1]=player.nextState.pos.trDelta[1]=-100;
        player.currentState.pos.trDelta[2]=player.nextState.pos.trDelta[2]=40;
        Evaluate();
        int ahead=time-1000; if(ahead>50) ahead=50;
        assert(fabsf(player.lerpOrigin[0]-(350+250*ahead*.001f))<.001f);
        assert(fabsf(player.lerpOrigin[1]-(-100*(50+ahead)*.001f))<.001f);
        assert(fabsf(player.lerpOrigin[2]-(40*(50+ahead)*.001f))<.001f);
    }
    Setup(1000,1050,935,985,1040,190,0);
    player.nextState.pos.trDelta[0]=0;
    player.nextState.pos.trBase[0]=280;
    Evaluate(); assert(player.lerpOrigin[0]==280); /* Stop immediately on the new sample. */

    /* Teleports/mount/death transitions, local prediction and non-player
     * trajectories keep their original path rather than receiving smoothing. */
    for(int guard=0;guard<13;guard++) {
        Setup(1000,1050,935,970,1010,190,0);
        switch(guard) {
        case 0: player.nextState.number=player.currentState.number=1; break;
        case 1: player.nextState.eType=2; break;
        case 2: player.currentState.eType=2; break;
        case 3: player.nextState.eFlags=0x20000; break;
        case 4: player.currentState.eFlags=0x20000; break;
        case 5: player.nextState.eFlags=0x100; break;
        case 6: player.nextState.eFlags=2; break;
        case 7: player.nextState.pos.trType=TR_GRAVITY; break;
        case 8: player.nextState.pos.trTime=1051; break;
        case 9: player.currentState.pos.trTime=1001; break;
        case 10: player.nextState.pos.trTime=900; break;
        case 11: player.nextState.pos.trDuration=0; break;
        case 12: player.currentState.number=3; break;
        }
        Fallback();
    }
    /* Snapshot reset copies the new state: never blend from an old life. */
    Setup(1000,1050,935,970,1010,190,0);
    player.nextState.pos.trBase[0]=10000;
    player.currentState=player.nextState;
    Evaluate(); assert(player.lerpOrigin[0]>=10000 && player.lerpOrigin[0]<10020);
    assert(state.bgs.clientinfo[2].playerAngles[0]==10);
    assert(fabsf(state.bgs.clientinfo[2].playerAngles[1]-350)<.001f);
    assert(player.lerpAngles[0]==0 && player.lerpAngles[2]==0);
    puts("PASS: 89,082 turn/stop samples; bounded packet-loss prediction; reset, local, mounted, dead, angular and malformed-state guards");
}
'''

code = support + function(trajectory, 'BG_EvaluateTrajectory')
for name in ('CG_PlayerTrajectorySampleTime', 'CG_PlayerMotionBlend',
             'CG_InterpolateRemotePlayerPosition', 'CG_InterpolateEntityPosition'):
    code += function(source, name)
code += checks
mutants = [
    code.replace('CG_PlayerTrajectorySampleTime(a, cg->snap->serverTime)', 'cg->snap->serverTime')
        .replace('CG_PlayerTrajectorySampleTime(b, cg->nextSnap->serverTime)', 'cg->nextSnap->serverTime'),
    code.replace('if (ahead > 50) ahead = 50;', ''),
    code.replace('cent->nextState.number == cg->nextSnap->ps.clientNum ||', ''),
    code.replace('((cent->currentState.eFlags ^ cent->nextState.eFlags) & 2) ||', ''),
]
assert all(mutant != code for mutant in mutants)
with tempfile.TemporaryDirectory(prefix='cod2-remote-motion-') as directory:
    path = Path(directory)
    for index, variant in enumerate([code] + mutants):
        (path / 'test.c').write_text(variant)
        subprocess.run(['cc', '-O1', '-fsanitize=address,undefined', str(path / 'test.c'),
                        '-lm', '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), result.stderr
        if not index:
            print(result.stdout, end='')
print('PASS: timestamp, unbounded extrapolation, local-prediction and teleport regressions are detected')
