#!/usr/bin/env python3
"""Replay remote players through the production sample playback, network clock
and trajectory export, over simulated LAN and internet links."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_ents_mp.c').read_text()
trajectory = (root / 'src/PC/bgame/bg_misc.c').read_text()
active = (root / 'src/PC/game_mp/g_active_mp.c').read_text()
clock = (root / 'src/PC/client_mp/cl_cgame_mp.c').read_text()


def function(text, name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;]*?\)\n\{', text)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


support = r'''
#define __EMSCRIPTEN__ 1
#include <assert.h>
#include <limits.h>
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
typedef struct { vec3_t origin, velocity, viewangles; int movementDir; } playerState_t;
typedef struct { entityState_t currentState, nextState; vec3_t lerpOrigin, lerpAngles; } centity_t;
typedef struct { float lerpMoveDir, lerpLean; vec3_t playerAngles; } clientInfo_t;
typedef struct { int serverTime; struct { int clientNum; } ps; } snapshot_t;
static struct {
    int time;
    float frameInterpolation;
    snapshot_t *snap, *nextSnap;
    struct { clientInfo_t clientinfo[64]; } bgs;
} state, *cg = &state;
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
static void Com_Printf(const char *f,...) {}
static float LerpAngle(float a,float b,float f) {
    float d=fmodf(b-a+540,360)-180;
    return a+d*f;
}
static void CG_TraceRemoteMotion(const centity_t *cent) {}
static int com_frameTime;
/* The browser's network clock, as test-client-clock-recoil.py drives it. */
typedef struct {int newSnapshots,serverTime,oldServerTime,oldSnapServerTime,serverTimeDelta,extrapolatedSnapshot;struct{int serverTime;}snap;} clientActive_t;
typedef struct {int demoplaying;} clientConnection_t;
typedef struct {int realtime;} clientStatic_t;
static clientActive_t client; static clientConnection_t conn; static clientStatic_t clsStorage;
#define CL_LOCAL (&client)
#define CLUI_STATE (&conn)
#define CLS (&clsStorage)
typedef struct {struct {int enabled;} current;} dvar_t;
static dvar_t zero; static dvar_t *zerop=&zero; static void *imp_cl_showTimeDelta=&zerop;
static int CL_DvarCurrentBool(void *p) { return 0; }
static float CL_ComTimescaleValue(void) { return 1; }
'''

checks = r'''
#define PLAYER 2
#define VIEWER 1
#define SPEED 190.0f
#define SNAPS 1000
#define PACKETS 2400

static centity_t player;
static snapshot_t views[SNAPS];
static unsigned seed;
static int Jit(int max) {
    seed = seed * 1664525u + 1013904223u;
    return max ? (int)((seed >> 8) % (unsigned)(max + 1)) : 0;
}
static cgRemotePlayback_t *Playback(void) { return &cg_remotePlayback[PLAYER]; }
static const cgRemoteSample_t *Newest(void) { return &Playback()->samples[Playback()->newest]; }

static void Sample(entityState_t *s,int command,float x,float vx,int eFlags) {
    memset(s,0,sizeof(*s));
    s->number=s->clientNum=PLAYER; s->eType=1; s->eFlags=eFlags;
    s->pos.trType=TR_LINEAR_STOP; s->pos.trTime=command; s->pos.trDuration=50;
    s->pos.trBase[0]=x; s->pos.trDelta[0]=vx;
    s->apos.trType=TR_INTERPOLATE; s->apos.trBase[0]=10; s->apos.trBase[1]=350;
}
/* What CG_SetNextSnap does to one player entity. */
static void Arrive(const entityState_t *s,snapshot_t *view) {
    int reset=(player.currentState.eFlags^s->eFlags)&2;
    player.nextState=*s;
    if(reset) player.currentState=player.nextState;
    CG_RecordRemotePlayerSample(&player,view->serverTime,view->ps.clientNum);
}
/* The frame drawn at serverTime reads that snapshot first, as
 * CG_DrawActiveFrame sets the time before CG_ProcessSnapshots. */
static void Deliver(int serverTime,int command,float x,float vx,int eFlags) {
    entityState_t s; Sample(&s,command,x,vx,eFlags);
    state.time=com_frameTime=serverTime;
    views[0].serverTime=serverTime; views[0].ps.clientNum=VIEWER;
    state.snap=state.nextSnap=&views[0];
    player.currentState=player.nextState;
    Arrive(&s,&views[0]);
}
static void Render(int time) { state.time=com_frameTime=time; CG_InterpolateEntityPosition(&player); }
static void Reset(void) {
    memset(cg_remotePlayback,0,sizeof(cg_remotePlayback));
    memset(&player,0,sizeof(player));
    memset(&state,0,sizeof(state));
}

typedef struct {
    const char *name;
    int up, down, jitter;        /* the remote player's link to the server */
    int toViewer, viewerJitter;  /* the server's link to the viewer */
    int serverJitter;            /* server frames run late, as on a loaded Pi */
    int frameA, frameB;          /* the viewer's alternating frame times */
    int bot;                     /* commands carry the server frame's time */
    int base;                    /* server time when the replay starts */
    int stallEvery, stallLength; /* the viewer's link stalls, as TCP retransmits */
} scenario_t;
typedef struct {
    int frames, held, bridged, slow, fast, lagMax;
    double minRatio, maxRatio, offsetSum, offsetMin, offsetMax, ahead;
    float delay;
} metrics_t;

static int serverTimes[SNAPS], commands[SNAPS], arrivals[SNAPS];
static int packetCommand[PACKETS], packetArrival[PACKETS];
static int replayBase;
static float Path(int command) { return 100.0f + SPEED * 0.001f * (float)(command - replayBase); }
static void Export(entityState_t *s,int command) {
    playerState_t ps={0};
    memset(s,0,sizeof(*s));
    s->number=s->clientNum=PLAYER; s->eType=1;
    ps.origin[0]=Path(command); ps.velocity[0]=SPEED; ps.viewangles[1]=350.75f;
    ExportTrajectory(&ps,s,command,1);
}

static void Replay(const scenario_t *sc,metrics_t *m) {
    int base=replayBase=sc->base;
    Reset();
    memset(m,0,sizeof(*m)); m->minRatio=m->offsetMin=1e9; m->maxRatio=m->offsetMax=m->ahead=-1e9;
    /* The remote browser stamps commands one interval and 5 ms behind its
     * newest snapshot and sends the newest 60 times a second, in order. */
    double next=base-1000; int last=INT_MIN;
    for(int p=0;p<PACKETS;p++,next+=1000.0/60.0) {
        int sent=(int)next, arrive=sent+sc->up+Jit(sc->jitter);
        packetCommand[p]=sent-sc->down-38;
        last=packetArrival[p]=arrive<last?last:arrive;
    }
    int ran=INT_MIN, delivered=INT_MIN, read=0, latest=base-200;
    for(int k=0;k<SNAPS;k++) {
        int run=base+33*k+Jit(sc->serverJitter);
        ran=run<ran?ran:run;
        while(read<PACKETS && packetArrival[read]<=ran) latest=packetCommand[read++];
        serverTimes[k]=base+33*k;
        commands[k]=sc->bot?serverTimes[k]:latest;
        int lag=serverTimes[k]-commands[k];
        if(k>30 && lag>m->lagMax) m->lagMax=lag;
        int a=ran+sc->toViewer+Jit(sc->viewerJitter);
        if(sc->stallEvery && k%sc->stallEvery==sc->stallEvery-1) a+=sc->stallLength;
        delivered=arrivals[k]=a<delivered?delivered:a;
    }
    memset(&client,0,sizeof(client));
    int received=0, processed=0, cur=-1, nextSnap=-1, frame=0, started=0, previousTime=0, previousReal=0;
    float previous=0;
    for(int t=arrivals[0];t<arrivals[SNAPS-1]-300;t+=(frame++&1)?sc->frameB:sc->frameA) {
        clsStorage.realtime=com_frameTime=t;
        for(;received<SNAPS && arrivals[received]<=t;received++) {
            if(!received) {
                client.snap.serverTime=serverTimes[0]; client.oldSnapServerTime=serverTimes[0]-33;
                client.serverTimeDelta=serverTimes[0]-t;
                client.serverTime=client.oldServerTime=serverTimes[0];
            } else {
                client.oldSnapServerTime=client.snap.serverTime;
                client.snap.serverTime=serverTimes[received]; client.newSnapshots=1;
            }
            views[received].serverTime=serverTimes[received];
            views[received].ps.clientNum=VIEWER;
        }
        StepClock();
        state.time=client.serverTime;
        /* CG_ProcessSnapshots: the cgame reads a snapshot only once the
         * clock has passed the one before it. */
        for(;;) {
            if(cur>=0 && nextSnap!=cur) {
                if(state.time<serverTimes[cur] || state.time>=serverTimes[nextSnap]) {
                    player.currentState=player.nextState; cur=nextSnap; continue;
                }
                break;
            }
            if(processed<received) {
                entityState_t s; Export(&s,commands[processed]);
                nextSnap=processed++;
                state.nextSnap=&views[nextSnap];
                Arrive(&s,&views[nextSnap]);
                if(cur<0) { player.currentState=player.nextState; cur=nextSnap; }
                continue;
            }
            if(state.time<serverTimes[cur]) state.time=serverTimes[cur];
            break;
        }
        state.snap=&views[cur]; state.nextSnap=&views[nextSnap];
        state.frameInterpolation=serverTimes[nextSnap]>serverTimes[cur]?
            fmaxf(0,(float)(state.time-serverTimes[cur])/(serverTimes[nextSnap]-serverTimes[cur])):0;
        CG_InterpolateEntityPosition(&player);
        if(state.time>=base+3000 && started && state.time>previousTime) {
            /* Speed as seen on screen: over the frame's real time. */
            double step=player.lerpOrigin[0]-previous, ratio=step/(SPEED*.001*(t-previousReal));
            ++m->frames;
            if(step==0) ++m->held;
            if(ratio<.9) ++m->slow;
            if(ratio>1.06) ++m->fast;
            if(ratio<m->minRatio) m->minRatio=ratio;
            if(ratio>m->maxRatio) m->maxRatio=ratio;
            double ahead=(float)(state.time-Newest()->time)-Playback()->delay;
            if(ahead>=0) ++m->bridged;
            if(ahead>m->ahead) m->ahead=ahead;
            /* A shot fired now is rewound to the archived server frames. */
            int i=(state.time-base)/33;
            if(i>=0 && i+1<SNAPS) {
                float f=(float)(state.time-serverTimes[i])/33.0f;
                float hit=Path(commands[i])+(Path(commands[i+1])-Path(commands[i]))*f;
                double offset=(player.lerpOrigin[0]-hit)/(SPEED*.001);
                m->offsetSum+=offset;
                if(offset<m->offsetMin) m->offsetMin=offset;
                if(offset>m->offsetMax) m->offsetMax=offset;
            }
        }
        previous=player.lerpOrigin[0]; previousTime=state.time; previousReal=t; started=1;
    }
    m->delay=Playback()->delay;
}

static void OldEvaluate(float *out) {
    vec3_t a,b;
    BG_EvaluateTrajectory(&player.currentState.pos,state.snap->serverTime,a);
    BG_EvaluateTrajectory(&player.nextState.pos,state.nextSnap->serverTime,b);
    for(int i=0;i<3;i++) out[i]=a[i]+(b[i]-a[i])*state.frameInterpolation;
}

int main(int argc,char **argv) {
    const scenario_t scenarios[]={
        {"LAN, 120 Hz",1,1,0,1,0,0,8,9,0,50000},
        {"LAN bot, 120 Hz",1,1,0,1,0,0,8,9,1,50000},
        {"60 ms internet, 120 Hz",30,30,12,30,12,4,8,9,0,50000},
        {"150 ms internet, 60 Hz",75,75,30,70,30,8,16,17,0,50000},
        {"loaded Pi, 300 ms, 144 Hz",150,150,40,140,40,25,7,7,0,50000},
        {"60 ms internet, server up 28 hours",30,30,12,30,12,4,8,9,0,100000000},
        {"60 ms with a stall every 4 s",30,30,12,30,12,4,8,9,0,50000,120,150},
    };
    int total=0;
    for(int n=0;n<7;n++) {
        metrics_t m; seed=1234+n; Replay(&scenarios[n],&m);
        double mean=m.offsetSum/m.frames;
        printf("  %-32s lag<=%3d ms delay %5.1f ms; %d frames: held %d, bridged %d (<=%.1f ms), slowed %d, jumps %d, speed %.2f-%.2f, behind hitbox %.1f ms (%.1f..%.1f)\n",
            scenarios[n].name,m.lagMax,m.delay,m.frames,m.held,m.bridged,fmax(0,m.ahead),m.slow,m.fast,m.minRatio,m.maxRatio,-mean,-m.offsetMax,-m.offsetMin);
        total+=m.frames;
        if(scenarios[n].stallEvery) {
            /* A 150 ms stall every 4 s cannot be hidden by 1 s of history,
             * but the player is never frozen for long and never runs ahead. */
            assert(m.held<=m.frames/50 && m.minRatio>=0 && m.offsetMax<10);
        } else {
            assert(m.held==0 && m.fast==0);
            assert(m.minRatio>.7 && m.maxRatio<1.04);
            /* Past the newest sample only briefly, within about a frame. */
            assert(m.bridged<=m.frames/50 && m.slow<=m.frames/50 && m.ahead<20);
            /* Drawn where a shot is rewound to, within the command spacing. */
            assert(mean<=2 && mean>=-28 && m.offsetMax<16);
            assert(m.delay<=m.lagMax+30);
        }
        if(scenarios[n].bot) assert(m.delay<4 && fabs(mean)<3);
    }
    printf("PASS: %d replayed frames over LAN, internet, loaded-Pi and stalled links run without a held frame or a jump, drawn within the shot rewind\n",total);

    /* Steady runs through the production export, including its snap=true path,
     * at 20, 30 and 50 Hz: fractional origins keep every frame's step. */
    const int intervals[]={50,33,20};
    for(int k=0;k<3;k++) {
        Reset(); replayBase=0;
        float previous=0;
        for(int s=0,time=1000;time<12000;time+=8) {
            state.time=time;
            while(1000+s*intervals[k]<=time) {
                entityState_t e; int serverTime=1000+s*intervals[k];
                Export(&e,serverTime-(s%2?80:65));
                views[0].serverTime=serverTime; views[0].ps.clientNum=VIEWER;
                state.snap=state.nextSnap=&views[0];
                player.currentState=player.nextState;
                Arrive(&e,&views[0]); ++s;
            }
            Render(time);
            if(time>3000) {
                float ideal=Path(time)-SPEED*.001f*Playback()->delay;
                assert(fabsf(player.lerpOrigin[0]-ideal)<.01f);
                assert(fabsf(player.lerpOrigin[0]-previous-SPEED*.008f)<.002f+SPEED*.001f*8*.08f);
                assert(player.nextState.apos.trBase[1]==350.0f);
            }
            previous=player.lerpOrigin[0];
        }
    }
    puts("PASS: exported fractional origins replay exactly at 20, 30 and 50 Hz; angle snapping preserved");

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

    /* A stop is reached exactly, without passing it. */
    Reset();
    for(int s=0;s<40;s++) {
        int c=1000+s*33; float x=s<30?100+SPEED*.001f*(c-1000):100+SPEED*.001f*(29*33);
        Deliver(c+70,c,x,s<30?SPEED:0,0);
        for(int t=c+70;t<c+103;t+=8) {
            Render(t); assert(player.lerpOrigin[0]<=100+SPEED*.001f*(29*33)+.001f);
        }
    }
    assert(fabsf(player.lerpOrigin[0]-(100+SPEED*.001f*(29*33)))<.001f);

    /* Missing samples bridge at most 50 ms past the newest, then hold, and
     * the playback slows so the resumed samples are rejoined with less of a jump. */
    Reset();
    for(int s=0;s<40;s++) {
        int c=1000+s*33;
        Deliver(c+80,c,100+SPEED*.001f*(c-1000),SPEED,0);
        for(int t=c+80;t<c+113;t+=8) Render(t);
    }
    float before=Playback()->delay, newestX=Newest()->origin[0];
    for(int t=1000+40*33+80;t<1000+40*33+680;t+=8) {
        Render(t);
        assert(player.lerpOrigin[0]<=newestX+SPEED*.05f+.001f);
    }
    assert(fabsf(player.lerpOrigin[0]-(newestX+SPEED*.05f))<.0001f);
    assert(Playback()->delay>before+100);

    /* A teleport is drawn at once and never blended with the old life, even
     * a respawn beside the old position. */
    Reset();
    for(int s=0;s<40;s++) {
        int c=1000+s*33;
        Deliver(c+80,c,100+SPEED*.001f*(c-1000),SPEED,0);
        for(int t=c+80;t<c+113;t+=8) Render(t);
    }
    float respawn=Newest()->origin[0]-50;
    for(int s=40;s<60;s++) {
        int c=1000+s*33;
        Deliver(c+80,c,respawn+SPEED*.001f*(c-2320),SPEED,2);
        for(int t=c+80;t<c+113;t+=8) {
            Render(t);
            assert(player.lerpOrigin[0]>=respawn && player.lerpOrigin[0]<=respawn+SPEED*.001f*(c-2320)+.001f);
            if(s==40) assert(player.lerpOrigin[0]==respawn);
        }
    }
    /* So is an unflagged jump across the map, such as entering a killcam. */
    Deliver(3080,3000,-3000,SPEED,2); Render(3080);
    assert(player.lerpOrigin[0]==-3000);

    /* One late sample a second is bridged rather than delaying every frame. */
    {
        cgRemotePlayback_t pb={0};
        for(int i=0;i<CG_REMOTE_LEADS;i++) pb.leads[i]=-60;
        pb.leadCount=CG_REMOTE_LEADS; pb.leads[7]=-260;
        assert(CG_RemoteTargetDelay(&pb)==62);
        pb.leads[9]=-200; assert(CG_RemoteTargetDelay(&pb)==202);
        pb.leadCount=3; pb.leads[1]=-90; assert(CG_RemoteTargetDelay(&pb)==92);
        pb.leadCount=CG_REMOTE_LEADS;
        for(int i=0;i<CG_REMOTE_LEADS;i++) pb.leads[i]=40;
        assert(CG_RemoteTargetDelay(&pb)==0);
        for(int i=0;i<CG_REMOTE_LEADS;i++) pb.leads[i]=-900;
        assert(CG_RemoteTargetDelay(&pb)==CG_REMOTE_MAX_DELAY);
    }

    /* A map change restarts the measurement; so does a player seen again
     * after a long absence. */
    Reset();
    Deliver(90000,89900,100,0,0); Render(89950);
    Deliver(1000,920,100,0,0);
    assert(Playback()->count==1 && Playback()->delay==100 && Playback()->leadCount==0);

    /* Followed, local, mounted, dead, non-player and unsmoothed entities keep
     * the snapshot interpolation, even with samples recorded earlier. */
    for(int guard=0;guard<7;guard++) {
        Reset();
        for(int s=0;s<20;s++) { int c=1000+s*33; Deliver(c+70,c,100+SPEED*.001f*(c-1000),SPEED,0); Render(c+70); }
        entityState_t e; Sample(&e,1000+20*33,100+SPEED*.001f*(20*33),SPEED,0);
        switch(guard) {
        case 1: e.eType=2; break;
        case 2: e.eFlags|=0x20000; break;
        case 3: e.eFlags|=0x100; break;
        case 4: e.pos.trType=TR_INTERPOLATE; break;
        case 5: e.number=64; break;
        }
        views[1].serverTime=views[0].serverTime+33; views[1].ps.clientNum=VIEWER;
        player.currentState=player.nextState;
        Arrive(&e,&views[1]);
        if(!guard) views[1].ps.clientNum=PLAYER; /* Now followed in spectator mode. */
        state.snap=&views[0]; state.nextSnap=&views[1]; state.time=views[0].serverTime+10;
        state.frameInterpolation=10/33.0f;
        vec3_t old; OldEvaluate(old); CG_InterpolateEntityPosition(&player);
        if(guard==6) assert(fabsf(player.lerpOrigin[0]-old[0])>.01f);
        else for(int i=0;i<3;i++) assert(fabsf(player.lerpOrigin[i]-old[i])<.001f);
    }
    /* The sample's angles drive the body: pitch and roll leave the entity. */
    assert(fabsf(state.bgs.clientinfo[PLAYER].playerAngles[0]-10)<.001f);
    assert(fabsf(state.bgs.clientinfo[PLAYER].playerAngles[1]-350)<.001f);
    assert(player.lerpAngles[0]==0 && player.lerpAngles[2]==0);
    puts("PASS: monotone turns and stops, bounded bridging, teleports, map changes, robust delay and every snapshot-interpolation guard");
}
'''

code = support + function(trajectory, 'BG_EvaluateTrajectory')
code += function(clock, 'CL_AdjustTimeDelta')
a = clock.index('            int timeDelta = cl->serverTimeDelta;')
b = clock.index('\n        } else {', a)
code += 'static void StepClock(void){clientActive_t *cl=CL_LOCAL;clientStatic_t *cls=CLS;' + clock[a:b] + '\nif(cl->newSnapshots)CL_AdjustTimeDelta();}\n'
start = source.index('#define CG_REMOTE_SAMPLES')
end = source.index('static cgRemotePlayback_t cg_remotePlayback[64];')
code += source[start:end] + 'static cgRemotePlayback_t cg_remotePlayback[64];\n'
for name in ('CG_PlayerMotionBlend', 'CG_RemoteSampleJumped', 'CG_RemoteTargetDelay',
             'CG_RecordRemotePlayerSample', 'CG_PlayRemotePlayer', 'CG_InterpolateEntityPosition'):
    code += function(source, name)
# The actual trajectory assignments and snap block, without unrelated
# event/stance exports.
export = function(active, 'G_PlayerStateToEntityStateExtrapolate')
assignments = export[export.index('    s->pos.trType'):export.index('    s->angles2[1]')]
snap = export[export.index('    if (snap) {'):export.index('    s->legsAnim =')]
code += 'static void ExportTrajectory(playerState_t *ps,entityState_t *s,int time,int snap){int i;\n' + assignments + snap + '}\n'
code += checks
mutants = {
    'integer origins': code.replace('s->apos.trBase[i] = (float)(int)s->apos.trBase[i];',
                                    's->pos.trBase[i] = (float)(int)s->pos.trBase[i]; s->apos.trBase[i] = (float)(int)s->apos.trBase[i];'),
    'drawn at the snapshot clock': code.replace('ahead = (float)(cg->time - newest->time) - pb->delay;',
                                                'ahead = (float)(cg->time - newest->time);'),
    'absolute float times': code.replace('ahead = (float)(cg->time - newest->time) - pb->delay;',
                                         'ahead = ((float)cg->time - pb->delay) - (float)newest->time;'),
    'every late sample sets the delay': code.replace('pb->leadCount >= CG_REMOTE_LEADS / 2 ? second : lowest', 'lowest'),
    'unbounded bridging': code.replace('(ahead < (float)CG_REMOTE_BRIDGE ? ahead : (float)CG_REMOTE_BRIDGE)', 'ahead'),
    'blended teleports': code.replace('if ((es->eFlags & 2) != pb->teleport || gap < 0', 'if (gap < 0'),
    'no stall slowdown': code.replace('pb->delay += 0.25f * (float)elapsed;', 'pb->delay += 0.0f;'),
    'clock corrections drawn': code.replace('if (real > 0 && abs(elapsed - real) <= 50) {', 'if (0) {'),
    'followed player smoothed': code.replace('es->number == cg->nextSnap->ps.clientNum ||\n', ''),
    'unchecked jumps': code.replace('CG_RemoteSampleJumped(newest, es, gap)) {', '0) {'),
}
assert all(mutant != code for mutant in mutants.values()), [k for k, v in mutants.items() if v == code]
with tempfile.TemporaryDirectory(prefix='cod2-remote-motion-') as directory:
    path = Path(directory)
    for index, (name, variant) in enumerate([('production', code)] + list(mutants.items())):
        (path / 'test.c').write_text(variant)
        subprocess.run(['cc', '-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=undefined',
                        str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (name, result.stdout, result.stderr)
        if not index:
            print(result.stdout, end='')
print('PASS: %d broken variants fail: %s' % (len(mutants), ', '.join(mutants)))
