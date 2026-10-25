#!/usr/bin/env python3
"""A shot meets its target where the shooter saw it, to the bit.

Replays a remote player who strafes, reverses, hops, turns, leans and
respawns through the production client playback (cg_ents_mp.c) over simulated
LAN, internet, stalling and loaded links. Each frame the shooter samples a
command with attack held (cl_input.c), whose view names how the players near
its aim were drawn in the frame on screen; the views ride the production
client message (msg_mp.c) to the production server parser (sv_client_mp.c),
and the production server rebuilds the pose from its own snapshot history
(g_weapon_mp.c). Every rebuilt pose must equal the drawn one exactly. Then
the wire format, malformed and forged views, the views' ring, the commands
that carry views and the players a view names, with mutants that must fail.
"""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
read = lambda path: (root / path).read_text()
cgame = read('src/PC/cgame_mp/cg_ents_mp.c')
bgame = read('src/PC/bgame/bg_misc.c')
client = read('src/PC/client_mp/cl_input.c')
server = read('src/PC/server_mp/sv_client_mp.c')
game = read('src/PC/game_mp/g_weapon_mp.c')
messages = read('src/PC/qcommon/msg_mp.c')


def function(text, name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;{]*?\)\n\{', text)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


def between(text, start, end, inclusive=True):
    a = text.index(start)
    b = text.index(end, a) + (len(end) if inclusive else 0)
    return text[a:b] + '\n'


support = r'''
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef float vec_t;
typedef float vec3_t[3];
typedef int qboolean;
typedef unsigned char byte;
typedef struct { qboolean overflowed; byte *data; int maxsize, cursize, readcount, bit; } msg_t;
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
    int time, oldTime;
    float frameInterpolation;
    snapshot_t *snap, *nextSnap;
    struct { clientInfo_t clientinfo[64]; } bgs;
    struct { vec3_t vieworg; vec3_t viewaxis[3]; } refdef;
} cgState, *cg = &cgState;
static int com_frameTime;
static void BG_Vec3Copy(const float *a, float *b) { memcpy(b, a, sizeof(vec3_t)); }
static void BG_Vec3Mad(const float *a, float f, const float *v, float *b) { for (int i = 0; i < 3; i++) b[i] = a[i] + f * v[i]; }
static float Vec3NormalizeTo(const float *a, float *b)
{
    float n = sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    for (int i = 0; i < 3; i++) b[i] = n ? a[i] / n : 0;
    return n;
}
static void Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
static float LerpAngle(float from, float to, float frac)
{
    float a = to - from;
    if (a > 180.0f) a -= 360.0f;
    if (a < -180.0f) a += 360.0f;
    return from + frac * a;
}
static void CG_TraceRemoteMotion(const centity_t *cent) { (void)cent; }
static void Com_Printf(const char *fmt, ...) { (void)fmt; }
/* The client's commands and network clock, as test-client-clock-recoil.py drives it. */
typedef struct { int serverTime, buttons; } usercmd_t;
typedef struct {
    int newSnapshots, serverTime, oldServerTime, oldSnapServerTime, serverTimeDelta, extrapolatedSnapshot;
    struct { int serverTime; } snap;
    int cmdNumber;
    usercmd_t cmds[128];
} clientActive_t;
typedef struct { int demoplaying; } clientConnection_t;
typedef struct { int realtime; } clientStatic_t;
static clientActive_t shooter;
static clientConnection_t conn;
static clientStatic_t cls;
static clientStatic_t *const clsPointer = &cls;
#define CL_LOCAL (&shooter)
#define CLUI_STATE (&conn)
#define CLS clsPointer
typedef struct { struct { int enabled; } current; } dvar_t;
static dvar_t zero;
static dvar_t *zerop = &zero;
static void *imp_cl_showTimeDelta = &zerop;
static int CL_DvarCurrentBool(void *p) { (void)p; return 0; }
static float CL_ComTimescaleValue(void) { return 1; }
/* The server's game. */
enum { CON_DISCONNECTED, CON_CONNECTING, CON_CONNECTED };
enum { SESS_STATE_PLAYING, SESS_STATE_DEAD, SESS_STATE_SPECTATOR, SESS_STATE_INTERMISSION };
typedef struct { struct { int connected, sessionState; } sess; } gclient_t;
typedef struct { entityState_t s; } gentity_t;
static gentity_t gentities[64];
static void *imp_g_entities = gentities;
static gclient_t gclients[64];
static struct { int time, maxclients; gclient_t *clients; } level = { 0, 4, gclients };
'''

clock = read('src/PC/client_mp/cl_cgame_mp.c')
code = support + read('src/headers/PC/bgame/bg_shot_view.h')
code += function(clock, 'CL_AdjustTimeDelta')
step_start = clock.index('            int timeDelta = cl->serverTimeDelta;')
code += ('static void StepClock(void){clientActive_t *cl=CL_LOCAL;clientStatic_t *cls=CLS;' +
         clock[step_start:clock.index('\n        } else {', step_start)] + '\nif(cl->newSnapshots)CL_AdjustTimeDelta();}\n')
for name in ('MSG_WriteBits_core', 'MSG_WriteBits', 'MSG_ReadBits_core', 'MSG_ReadBits', 'MSG_WriteByte_core',
             'MSG_WriteByte', 'MSG_ReadByte_core', 'MSG_ReadByte', 'MSG_ReadShort_core', 'MSG_ReadShort',
             'MSG_ReadLong_core', 'MSG_ReadLong', 'MSG_WriteShort', 'MSG_WriteLong'):
    code += function(messages, name)
for name in ('BG_EvaluateTrajectory', 'BG_PlayerMotionBlend', 'BG_ShotViewPose'):
    code += function(bgame, name)
code += between(cgame, '#define CG_REMOTE_SAMPLES', 'static cgShotViewDrawn_t cg_shotViewDrawn[64][2];')
for name in ('CG_RemoteSampleJumped', 'CG_RemoteTargetDelay', 'CG_RecordRemotePlayerSample', 'CG_DrawRemotePlayer',
             'CG_PlayRemotePlayer', 'CG_InterpolateEntityPosition'):
    code += function(cgame, name)
code += between(cgame, '#define CG_SHOT_VIEW_CHEST', '#define CG_SHOT_VIEW_CONE 0.1745329f')
code += function(cgame, 'CG_GetShotView')
code += between(client, '#define CL_SHOT_VIEW_AFTER_ATTACK', 'static int cl_lastAttackTime = -100000;')
code += function(client, 'CL_WriteShotViews') + function(client, 'CL_SampleShotView')
code += between(server, '#define SV_SHOT_VIEWS', 'static int sv_shotViewNext[64];')
for name in ('SV_ReadShotViews', 'SV_GetShotView', 'SV_ClearShotViews'):
    code += function(server, name)
code += between(game, '#define G_SHOT_SAMPLES', 'static gShotHistory_t g_shotHistory[64];')
for name in ('G_ShotSampleAngle', 'G_RecordShotSamples', 'G_ShotSample', 'G_ShotViewPose'):
    code += function(game, name)
# Snapshots carry angles as 16-bit shares of a turn and every other field
# the samples hold exactly; the replay's link does the same.
assert 'MSG_WriteShort_core(msg, (int)(*(float *)toF * 182.04444885253906f));' in messages
assert '*(float *)toF = (float)MSG_ReadShort_core(msg) * 0.0054931640625f;' in messages
for field in ('pos.trTime), 32', 'pos.trBase[0]), 0', 'pos.trBase[1]), 0', 'pos.trBase[2]), 0', 'pos.trDelta[0]), 0',
              'pos.trDelta[1]), 0', 'pos.trDelta[2]), 0', 'angles2[1]), 0', 'leanf), 0', 'apos.trBase[0]), -100',
              'apos.trBase[1]), -100', 'apos.trBase[2]), -100'):
    assert '{ ESF(' + field + ' }' in messages, field

checks = r'''
#define SHOOTER 1
#define TARGET 2
#define FRAME 33
#define MAX_EVENTS 4096

static centity_t player;
static snapshot_t views[MAX_EVENTS];
static unsigned seed;
static int Jit(int max)
{
    seed = seed * 1664525u + 1013904223u;
    return max ? (int)((seed >> 8) % (unsigned)(max + 1)) : 0;
}

/* The target's motion at its command time: a strafe reversing every 0.65 s,
 * a drift, hops, turns, a lean and a respawn at the given time. */
static int base, respawnAt;
static void Motion(int command, entityState_t *s)
{
    float t = (float)(command - base) * 0.001f, phase = fmodf(t, 1.3f), hop = fmodf(t, 2.0f);
    int respawned = respawnAt && command >= respawnAt;
    memset(s, 0, sizeof(*s));
    s->number = s->clientNum = TARGET;
    s->eType = 1;
    s->eFlags = respawned ? 2 : 0;
    s->pos.trType = TR_LINEAR_STOP;
    s->pos.trTime = command;
    s->pos.trDuration = 50;
    s->pos.trBase[0] = phase < 0.65f ? 190.0f * phase : 190.0f * (1.3f - phase);
    s->pos.trDelta[0] = phase < 0.65f ? 190.0f : -190.0f;
    s->pos.trBase[1] = 40.0f * t + (respawned ? 500.0f : 0.0f);
    s->pos.trDelta[1] = 40.0f;
    if (hop < 0.6f) {
        s->pos.trBase[2] = 270.0f * hop - 450.0f * hop * hop;
        s->pos.trDelta[2] = 270.0f - 900.0f * hop;
    }
    s->apos.trType = TR_INTERPOLATE;
    s->apos.trBase[0] = (float)(int)(10.0f * sinf(0.7f * t));
    s->apos.trBase[1] = (float)(int)(90.0f + 170.0f * sinf(1.9f * t));
    s->angles2[1] = s->pos.trDelta[0] > 0.0f ? 90.0f : -90.0f;
    s->leanf = 0.6f * sinf(2.2f * t);
}

typedef struct {
    const char *name;
    int up, down, jitter;        /* the target's link to the server */
    int toViewer, viewerJitter;  /* the server's link to the shooter */
    int toServer, serverJitter;  /* the shooter's link to the server */
    int frameA, frameB;          /* the shooter's alternating frame times */
    int bot;                     /* commands carry the server frame's time */
    int stallEvery, stallLength; /* the shooter's link stalls, as TCP retransmits */
    int respawn;                 /* the target respawns 6 s in */
} scenario_t;

typedef struct { int time; vec3_t origin, angles; float moveDir, lean; } drawn_t;
typedef struct { int arrive, count, newest; msg_t msg; byte data[2048]; } packet_t;

static int serverTimes[MAX_EVENTS], commands[MAX_EVENTS], arrivals[MAX_EVENTS];
static int packetCommand[MAX_EVENTS * 2], packetArrival[MAX_EVENTS * 2];
static drawn_t expected[1 << 16];
static packet_t packets[MAX_EVENTS * 2];

typedef struct {
    int verified, refused, carried, shown, blends, bridges, samples, legacyCount, legacyOver;
    double legacyMax, legacySum;
} result_t;

static void Draw(drawn_t *d)
{
    clientInfo_t *ci = &cg->bgs.clientinfo[TARGET];
    d->time = cg->time;
    memcpy(d->origin, player.lerpOrigin, sizeof(vec3_t));
    memcpy(d->angles, ci->playerAngles, sizeof(vec3_t));
    d->moveDir = ci->lerpMoveDir;
    d->lean = ci->lerpLean;
}

/* What CG_SetNextSnap does to the target's entity, as the snapshot carried
 * it: angles in 65536ths of a turn, as msg_mp.c writes and reads them. */
static void Arrive(const entityState_t *s, snapshot_t *view)
{
    int reset = (player.currentState.eFlags ^ s->eFlags) & 2;
    player.nextState = *s;
    for (int i = 0; i < 3; i++) {
        float a = s->apos.trBase[i];
        player.nextState.apos.trBase[i] = a == 0.0f ? 0.0f : (float)(short)(int)(a * 182.04444885253906f) * 0.0054931640625f;
    }
    if (reset)
        player.currentState = player.nextState;
    CG_RecordRemotePlayerSample(&player, view->serverTime, view->ps.clientNum);
}

/* The server takes a packet: its views, then each command it has not run. */
static int lastExecuted, serverNow;
static void Execute(const scenario_t *sc, packet_t *p, result_t *r)
{
    msg_t *m = &p->msg;
    m->readcount = m->bit = 0;
    assert(MSG_ReadBits(m, 11) == 0x2a5);
    SV_ReadShotViews(SHOOTER, m);
    level.time = serverNow;
    for (int c = p->newest - p->count + 1; c <= p->newest; c++) {
        const usercmd_t *cmd = &shooter.cmds[c & 127];
        const drawn_t *want = &expected[c & 0xffff];
        const bgShotView_t *view;
        if (cmd->serverTime <= lastExecuted)
            continue;
        lastExecuted = cmd->serverTime;
        if (!want->time)
            continue;
        view = SV_GetShotView(SHOOTER, cmd->serverTime);
        for (int e = 0; view && e < view->count; e++) {
            const bgShotViewEntry_t *entry = &view->entries[e];
            bgPlayerPose_t pose;
            if (entry->clientNum != TARGET)
                continue;
            if (!G_ShotViewPose(TARGET, entry, &pose)) {
                /* Only the life before a respawn is refused. */
                assert(sc->respawn && entry->time < respawnAt && serverNow >= respawnAt);
                ++r->refused;
                continue;
            }
            if (memcmp(pose.origin, want->origin, sizeof(vec3_t)) || memcmp(pose.angles, want->angles, sizeof(vec3_t)) ||
                pose.moveDir != want->moveDir || pose.lean != want->lean) {
                printf("%s: command %d at %d rebuilt %.4f %.4f %.4f, drawn %.4f %.4f %.4f\n", sc->name, c,
                       cmd->serverTime, pose.origin[0], pose.origin[1], pose.origin[2], want->origin[0],
                       want->origin[1], want->origin[2]);
                abort();
            }
            ++r->verified;
            r->blends += entry->mode == BG_SHOT_VIEW_BLEND;
            r->bridges += entry->mode == BG_SHOT_VIEW_BRIDGE;
            r->samples += entry->mode == BG_SHOT_VIEW_SAMPLE;
        }
    }
}

static void Replay(const scenario_t *sc, result_t *r)
{
    int frames = 900, sent = 0, received = 0, processed = 0, consumed = 0, read = 0, latest, packetCount = 0;
    int nextPacketCmd = 1, previousNew = 0, cur = -1, nextSnap = -1, hasPrevious = 0;
    double nextPacket;
    drawn_t previous = { 0 };

    memset(r, 0, sizeof(*r));
    memset(cg_remotePlayback, 0, sizeof(cg_remotePlayback));
    memset(cg_shotViewDrawn, 0, sizeof(cg_shotViewDrawn));
    memset(g_shotHistory, 0, sizeof(g_shotHistory));
    memset(sv_shotViews, 0, sizeof(sv_shotViews));
    memset(sv_shotViewNext, 0, sizeof(sv_shotViewNext));
    memset(expected, 0, sizeof(expected));
    memset(&player, 0, sizeof(player));
    memset(&cgState, 0, sizeof(cgState));
    memset(&shooter, 0, sizeof(shooter));
    memset(gclients, 0, sizeof(gclients));
    gclients[TARGET].sess.connected = CON_CONNECTED;
    gclients[TARGET].sess.sessionState = SESS_STATE_PLAYING;
    cl_lastAttackTime = -100000;
    lastExecuted = INT_MIN;
    base = 50000;
    respawnAt = sc->respawn ? base + 6000 : 0;
    /* The target's browser stamps commands behind its newest snapshot and
     * sends 60 packets a second, in order. */
    {
        double next = base - 1000;
        int last = INT_MIN;
        for (int p = 0; p < MAX_EVENTS * 2; p++, next += 1000.0 / 60.0) {
            int at = (int)next, arrive = at + sc->up + Jit(sc->jitter);
            packetCommand[p] = at - sc->down - 38;
            last = packetArrival[p] = arrive < last ? last : arrive;
        }
    }
    latest = base - 200;
    for (int k = 0, delivered = INT_MIN; k < frames; k++) {
        int a;
        serverTimes[k] = base + FRAME * k;
        while (read < MAX_EVENTS * 2 && packetArrival[read] <= serverTimes[k])
            latest = packetCommand[read++];
        commands[k] = sc->bot ? serverTimes[k] : latest;
        a = serverTimes[k] + sc->toViewer + Jit(sc->viewerJitter);
        if (sc->stallEvery && k % sc->stallEvery == sc->stallEvery - 1)
            a += sc->stallLength;
        delivered = arrivals[k] = a < delivered ? delivered : a;
    }
    nextPacket = arrivals[0];
    for (int t = arrivals[0], frame = 0; t < arrivals[frames - 1] - 300; t += (frame++ & 1) ? sc->frameB : sc->frameA) {
        /* The server runs its frames, recording the target, and takes the
         * shooter's packets as they come, in time order. */
        for (;;) {
            int frameDue = sent < frames ? serverTimes[sent] : INT_MAX;
            int packetDue = processed < packetCount ? packets[processed].arrive : INT_MAX;
            if (frameDue > t && packetDue > t)
                break;
            if (frameDue <= packetDue) {
                Motion(commands[sent], &gentities[TARGET].s);
                serverNow = level.time = serverTimes[sent++];
                G_RecordShotSamples();
            } else {
                Execute(sc, &packets[processed++], r);
            }
        }
        /* The shooter's frame, as the browser runs it: the network clock,
         * the snapshots the cgame reads once its clock passes the one before,
         * the target drawn, then the command sampled after the frame. */
        cls.realtime = com_frameTime = t;
        for (; received < frames && arrivals[received] <= t; received++) {
            if (!received) {
                shooter.snap.serverTime = serverTimes[0];
                shooter.oldSnapServerTime = serverTimes[0] - FRAME;
                shooter.serverTimeDelta = serverTimes[0] - t;
                shooter.serverTime = shooter.oldServerTime = serverTimes[0];
            } else {
                shooter.oldSnapServerTime = shooter.snap.serverTime;
                shooter.snap.serverTime = serverTimes[received];
                shooter.newSnapshots = 1;
            }
            views[received].serverTime = serverTimes[received];
            views[received].ps.clientNum = SHOOTER;
        }
        if (!received)
            continue;
        StepClock();
        cg->oldTime = cg->time;
        cg->time = shooter.serverTime;
        for (;;) {
            if (cur >= 0 && nextSnap != cur) {
                if (cg->time < serverTimes[cur] || cg->time >= serverTimes[nextSnap]) {
                    player.currentState = player.nextState;
                    cur = nextSnap;
                    continue;
                }
                break;
            }
            if (consumed < received) {
                entityState_t s;
                Motion(commands[consumed], &s);
                nextSnap = consumed++;
                cg->nextSnap = &views[nextSnap];
                Arrive(&s, &views[nextSnap]);
                if (cur < 0) {
                    player.currentState = player.nextState;
                    cur = nextSnap;
                }
                continue;
            }
            if (cg->time < serverTimes[cur])
                cg->time = serverTimes[cur];
            break;
        }
        cg->snap = &views[cur];
        cg->nextSnap = &views[nextSnap];
        cg->frameInterpolation = serverTimes[nextSnap] > serverTimes[cur] ?
            fmaxf(0, (float)(cg->time - serverTimes[cur]) / (float)(serverTimes[nextSnap] - serverTimes[cur])) : 0;
        CG_InterpolateEntityPosition(&player);
        {
            /* Aimed at the target as the frame on screen drew it. */
            float d[3], n;
            cg->refdef.vieworg[0] = -400.0f; cg->refdef.vieworg[1] = 30.0f; cg->refdef.vieworg[2] = 60.0f;
            for (int i = 0; i < 3; i++)
                d[i] = (hasPrevious ? previous.origin[i] : player.lerpOrigin[i]) + (i == 2 ? 40.0f : 0.0f) -
                       cg->refdef.vieworg[i];
            n = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
            for (int i = 0; i < 3; i++)
                cg->refdef.viewaxis[0][i] = d[i] / n;
        }
        ++shooter.cmdNumber;
        shooter.cmds[shooter.cmdNumber & 127].serverTime = shooter.serverTime;
        shooter.cmds[shooter.cmdNumber & 127].buttons = 1;
        cl_sampleShotViews = 1;
        CL_SampleShotView(&shooter);
        cl_sampleShotViews = 0;
        expected[shooter.cmdNumber & 0xffff] = hasPrevious && previous.time == cg->oldTime ? previous : (drawn_t){ 0 };
        if (expected[shooter.cmdNumber & 0xffff].time) {
            const bgShotView_t *v = &cl_shotViews[shooter.cmdNumber & 127];
            int i = (shooter.serverTime - base) / FRAME;
            ++r->shown;
            r->carried += v->count == 1 && v->entries[0].clientNum == TARGET;
            /* Where CoD2's rewind met the target: its archived frames
             * interpolated at the command time. */
            if (i >= 0 && i + 1 < frames && serverTimes[i + 1] <= serverNow) {
                entityState_t a, b;
                float f = (float)(shooter.serverTime - serverTimes[i]) / FRAME, err = 0;
                Motion(commands[i], &a);
                Motion(commands[i + 1], &b);
                for (int k = 0; k < 3; k++) {
                    float d = a.pos.trBase[k] + (b.pos.trBase[k] - a.pos.trBase[k]) * f - previous.origin[k];
                    err += d * d;
                }
                err = sqrtf(err);
                if (!((a.eFlags ^ b.eFlags) & 2) && !((a.eFlags ^ player.nextState.eFlags) & 2)) {
                    if (err > r->legacyMax)
                        r->legacyMax = err;
                    r->legacySum += err;
                    r->legacyOver += err > 5.0f;
                    ++r->legacyCount;
                }
            }
        }
        Draw(&previous);
        hasPrevious = 1;
        /* Packets 60 times a second carry the new commands and the last
         * packet's again, as cl_packetdup 1 does. */
        if (t >= nextPacket) {
            packet_t *p = &packets[packetCount];
            int fresh = shooter.cmdNumber - nextPacketCmd + 1, count = fresh + previousNew;
            nextPacket += 1000.0 / 60.0;
            if (nextPacket <= t)
                nextPacket = t + 1000.0 / 60.0;
            if (count > 32)
                count = 32;
            memset(p, 0, sizeof(*p));
            p->msg.data = p->data;
            p->msg.maxsize = sizeof(p->data);
            p->count = count;
            p->newest = shooter.cmdNumber;
            MSG_WriteBits(&p->msg, 0x2a5, 11);
            CL_WriteShotViews(&p->msg, &shooter, count);
            MSG_WriteBits(&p->msg, 3, 3);
            assert(!p->msg.overflowed);
            p->arrive = t + sc->toServer + Jit(sc->serverJitter);
            if (sc->stallEvery && packetCount % (sc->stallEvery * 2) == sc->stallEvery * 2 - 1)
                p->arrive += sc->stallLength;
            if (packetCount && p->arrive < packets[packetCount - 1].arrive)
                p->arrive = packets[packetCount - 1].arrive;
            previousNew = fresh;
            nextPacketCmd = shooter.cmdNumber + 1;
            ++packetCount;
            assert(packetCount < MAX_EVENTS * 2);
        }
    }
}

static void Entry(bgShotViewEntry_t *e, int clientNum, int mode, int time, int span, int frac)
{
    e->clientNum = clientNum; e->mode = mode; e->time = time; e->span = span; e->frac = frac;
}

/* A packet with one view per command, read back by the server. */
static void Send(const bgShotView_t *list, int count, int truncate)
{
    static byte data[4096];
    msg_t m = { 0, data, sizeof(data), 0, 0, 0 };
    memset(&shooter, 0, sizeof(shooter));
    for (int i = 0; i < count; i++) {
        shooter.cmdNumber = 1000 + i;
        cl_shotViews[shooter.cmdNumber & 127] = list[i];
        shooter.cmds[shooter.cmdNumber & 127].serverTime = list[i].serverTime;
    }
    MSG_WriteBits(&m, 5, 3);
    CL_WriteShotViews(&m, &shooter, count);
    MSG_WriteBits(&m, 3, 3);
    m.cursize -= truncate;
    m.readcount = m.bit = 0;
    assert(MSG_ReadBits(&m, 3) == 5);
    SV_ReadShotViews(SHOOTER, &m);
}

/* A raw view block, as a client of its own might send one. */
static void Raw(const int *bytes, int count)
{
    static byte data[256];
    msg_t m = { 0, data, sizeof(data), 0, 0, 0 };
    MSG_WriteBits(&m, BG_SHOT_VIEW_OP, 3);
    for (int i = 0; i < count; i++) {
        if (bytes[i] >= 0x10000) MSG_WriteLong(&m, bytes[i] - 0x10000);
        else MSG_WriteByte(&m, bytes[i]);
    }
    m.readcount = m.bit = 0;
    SV_ReadShotViews(SHOOTER, &m);
}

static void Record(int clientNum, int time, float x, int eFlags)
{
    entityState_t *s = &gentities[clientNum].s;
    memset(s, 0, sizeof(*s));
    s->number = s->clientNum = clientNum; s->eType = 1; s->eFlags = eFlags;
    s->pos.trType = TR_LINEAR_STOP; s->pos.trTime = time; s->pos.trBase[0] = x; s->pos.trDelta[0] = 190.0f;
    G_RecordShotSamples();
}

int main(void)
{
    const scenario_t scenarios[] = {
        { "LAN, 120 Hz", 1, 1, 0, 1, 0, 1, 0, 8, 9, 0, 0, 0, 0 },
        { "LAN bot, 144 Hz", 1, 1, 0, 1, 0, 1, 0, 7, 7, 1, 0, 0, 0 },
        { "60 ms internet, 120 Hz", 30, 30, 12, 30, 12, 30, 12, 8, 9, 0, 0, 0, 0 },
        { "150 ms internet, 60 Hz", 75, 75, 30, 70, 30, 75, 30, 16, 17, 0, 0, 0, 0 },
        { "loaded Pi, 300 ms, 144 Hz", 150, 150, 40, 140, 40, 150, 40, 7, 7, 0, 0, 0, 0 },
        { "60 ms, a stall every 4 s", 30, 30, 12, 30, 12, 30, 12, 8, 9, 0, 120, 150, 0 },
        { "60 ms, a respawn", 30, 30, 12, 30, 12, 30, 12, 8, 9, 0, 0, 0, 1 },
    };
    int total = 0, bridges = 0, samples = 0;

    for (unsigned n = 0; n < sizeof(scenarios) / sizeof(scenarios[0]); n++) {
        result_t r;
        seed = 77 + n;
        Replay(&scenarios[n], &r);
        printf("  %-28s %5d shots rebuilt exactly (%d blended, %d bridged, %d on a first sample, %d of a life "
               "before a respawn refused); CoD2's rewind %.1f units off on average, up to %.1f, over 5 for %.0f%%\n",
               scenarios[n].name, r.verified, r.blends, r.bridges, r.samples, r.refused,
               r.legacySum / r.legacyCount, r.legacyMax, 100.0 * r.legacyOver / r.legacyCount);
        /* Every command sampled against a frame that drew the target names
         * it, and every one the server executed met it exactly. */
        assert(r.carried == r.shown && r.verified + r.refused > r.shown * 9 / 10);
        assert(r.refused <= (scenarios[n].respawn ? 40 : 0));
        total += r.verified;
        bridges += r.bridges;
        samples += r.samples;
    }
    assert(bridges > 0 && samples > 0);
    printf("PASS: %d shots over LAN, internet, loaded-Pi, stalled and respawning links meet the target in the pose "
           "the shooter's frame drew, to the bit, blended, bridged and on a first sample\n", total);

    /* The wire: every field at its bounds, after commands' bits. */
    {
        bgShotView_t list[3];
        memset(list, 0, sizeof(list));
        list[0].serverTime = 123456; list[0].count = 3;
        Entry(&list[0].entries[0], 63, BG_SHOT_VIEW_BLEND, 123000, BG_SHOT_VIEW_MAX_SPAN, BG_SHOT_VIEW_FRAC_ONE);
        Entry(&list[0].entries[1], 0, BG_SHOT_VIEW_BRIDGE, -5, 0, BG_SHOT_VIEW_BRIDGE_MAX * BG_SHOT_VIEW_BRIDGE_STEPS);
        Entry(&list[0].entries[2], 17, BG_SHOT_VIEW_SAMPLE, 0x7fffffff, 0, 0);
        list[1].serverTime = 123460; list[1].count = 0;
        list[2].serverTime = 123470; list[2].count = 4;
        for (int e = 0; e < 4; e++)
            Entry(&list[2].entries[e], 4 + e, BG_SHOT_VIEW_BLEND, 122000 + e, 1 + e, 32768 + e);
        SV_ClearShotViews(SHOOTER);
        Send(list, 3, 0);
        for (int i = 0; i < 3; i += 2) {
            const bgShotView_t *got = SV_GetShotView(SHOOTER, list[i].serverTime);
            assert(got && got->count == list[i].count);
            for (int e = 0; e < got->count; e++)
                assert(!memcmp(&got->entries[e], &list[i].entries[e], sizeof(bgShotViewEntry_t)));
        }
        assert(!SV_GetShotView(SHOOTER, 123460) && !SV_GetShotView(SHOOTER, 123465) && !SV_GetShotView(0, 123456));
        /* Its last byte holds only the end's bits; cut anywhere into the
         * views, nothing of them is kept. */
        SV_ClearShotViews(SHOOTER);
        Send(list, 3, 1);
        assert(SV_GetShotView(SHOOTER, 123456) && SV_GetShotView(SHOOTER, 123470));
        for (int cut = 2; cut < 40; cut++) {
            SV_ClearShotViews(SHOOTER);
            Send(list, 3, cut);
            assert(!SV_GetShotView(SHOOTER, 123456) && !SV_GetShotView(SHOOTER, 123470));
        }
    }
    /* A packet of twenty commands carries the newest sixteen's views. */
    {
        bgShotView_t list[20];
        memset(list, 0, sizeof(list));
        for (int i = 0; i < 20; i++) {
            list[i].serverTime = 2000 + 7 * i; list[i].count = 1;
            Entry(&list[i].entries[0], TARGET, BG_SHOT_VIEW_SAMPLE, 1900 + i, 0, 0);
        }
        SV_ClearShotViews(SHOOTER);
        Send(list, 20, 0);
        for (int i = 0; i < 20; i++)
            assert(!SV_GetShotView(SHOOTER, list[i].serverTime) == (i < 4));
        /* The ring keeps the latest 32, a command sent again in its slot. */
        SV_ClearShotViews(SHOOTER);
        for (int i = 0; i < 40; i++) {
            list[0].serverTime = 5000 + i;
            Send(list, 1, 0);
            Send(list, 1, 0);
        }
        for (int i = 0; i < 40; i++)
            assert(!SV_GetShotView(SHOOTER, 5000 + i) == (i < 8));
    }
    /* Malformed blocks: none, empty, too many records or entries, an unknown
     * mode, or no block at all from a client that sends none. */
    {
        const int L = 0x10000;
        const int none[] = { 0 };
        const int many[] = { 17 };
        const int empty[] = { 1, L + 900, 0 };
        const int crowded[] = { 1, L + 901, 5 };
        const int unknown[] = { 1, L + 902, 1, TARGET | 3 << 6, L + 800 };
        SV_ClearShotViews(SHOOTER);
        Raw(none, 1); Raw(many, 1); Raw(empty, 3); Raw(crowded, 3); Raw(unknown, 5);
        for (int k = 0; k < SV_SHOT_VIEWS; k++)
            assert(!sv_shotViews[SHOOTER][k].count);
        {
            static byte data[8];
            msg_t m = { 0, data, sizeof(data), 0, 0, 0 };
            MSG_WriteBits(&m, 3, 3);
            m.readcount = m.bit = 0;
            SV_ReadShotViews(SHOOTER, &m);
            SV_ReadShotViews(64, &m);
        }
        for (int k = 0; k < SV_SHOT_VIEWS; k++)
            assert(!sv_shotViews[SHOOTER][k].count);
    }
    puts("PASS: views round-trip at their bounds after commands' bits; truncated, empty, crowded and unknown blocks keep "
         "nothing; the newest 16 commands of a packet carry theirs; the ring keeps 32, a resend in its slot");

    /* The server's history: one sample per command time, a life ending at a
     * teleport, death, a mounted gun or anything not drawn from samples. */
    {
        bgPlayerPose_t pose;
        bgShotViewEntry_t e;
        memset(g_shotHistory, 0, sizeof(g_shotHistory));
        memset(gclients, 0, sizeof(gclients));
        gclients[TARGET].sess.connected = CON_CONNECTED;
        gclients[TARGET].sess.sessionState = SESS_STATE_PLAYING;
        level.time = 20000;
        for (int i = 0; i < 70; i++) {
            Record(TARGET, 18000 + 30 * i, (float)i, 0);
            Record(TARGET, 18000 + 30 * i, 1000.0f, 0);
        }
        assert(g_shotHistory[TARGET].count == G_SHOT_SAMPLES);
        assert(g_shotHistory[TARGET].samples[g_shotHistory[TARGET].newest].origin[0] == 69.0f);
        Entry(&e, TARGET, BG_SHOT_VIEW_BLEND, 18000 + 30 * 60, 30, 32768);
        assert(G_ShotViewPose(TARGET, &e, &pose) && pose.origin[0] > 60.0f && pose.origin[0] < 61.0f);
        Entry(&e, TARGET, BG_SHOT_VIEW_BLEND, 18000 + 30 * 60, 60, 0);
        assert(G_ShotViewPose(TARGET, &e, &pose) && pose.origin[0] == 60.0f);
        /* Forged: a time never recorded, a sample older than the rewind,
         * spans and reaches the playback never takes, an unknown mode, a
         * client out of range or without samples. */
        Entry(&e, TARGET, BG_SHOT_VIEW_SAMPLE, 18000 + 30 * 60 + 1, 0, 0); assert(!G_ShotViewPose(TARGET, &e, &pose));
        Entry(&e, TARGET, BG_SHOT_VIEW_SAMPLE, 18000 + 30 * 10, 0, 0); assert(!G_ShotViewPose(TARGET, &e, &pose));
        Entry(&e, TARGET, BG_SHOT_VIEW_SAMPLE, 18000 + 30 * 32, 0, 0); assert(!G_ShotViewPose(TARGET, &e, &pose));
        Entry(&e, TARGET, BG_SHOT_VIEW_SAMPLE, 18000 + 30 * 34, 0, 0); assert(G_ShotViewPose(TARGET, &e, &pose));
        Entry(&e, TARGET, BG_SHOT_VIEW_BLEND, 18000 + 30 * 60, 0, 0); assert(!G_ShotViewPose(TARGET, &e, &pose));
        Entry(&e, TARGET, BG_SHOT_VIEW_BLEND, 18000 + 30 * 60, 1001, 0); assert(!G_ShotViewPose(TARGET, &e, &pose));
        Entry(&e, TARGET, BG_SHOT_VIEW_BLEND, 18000 + 30 * 60, 30, 65536); assert(!G_ShotViewPose(TARGET, &e, &pose));
        Entry(&e, TARGET, BG_SHOT_VIEW_BRIDGE, 18000 + 30 * 69, 0, 3200); assert(G_ShotViewPose(TARGET, &e, &pose));
        assert(fabsf(pose.origin[0] - (69.0f + 190.0f * 0.05f)) < 1e-4f);
        Entry(&e, TARGET, BG_SHOT_VIEW_BRIDGE, 18000 + 30 * 69, 0, 3201); assert(!G_ShotViewPose(TARGET, &e, &pose));
        Entry(&e, TARGET, BG_SHOT_VIEW_MODES, 18000 + 30 * 69, 0, 0); assert(!G_ShotViewPose(TARGET, &e, &pose));
        Entry(&e, 64, BG_SHOT_VIEW_SAMPLE, 18000 + 30 * 69, 0, 0); assert(!G_ShotViewPose(64, &e, &pose));
        Entry(&e, 3, BG_SHOT_VIEW_SAMPLE, 18000 + 30 * 69, 0, 0); assert(!G_ShotViewPose(3, &e, &pose));
        /* A teleport ends the life; the samples before it are gone. */
        Record(TARGET, 18000 + 30 * 70, 70.0f, 2);
        Entry(&e, TARGET, BG_SHOT_VIEW_SAMPLE, 18000 + 30 * 69, 0, 0); assert(!G_ShotViewPose(TARGET, &e, &pose));
        Entry(&e, TARGET, BG_SHOT_VIEW_SAMPLE, 18000 + 30 * 70, 0, 0); assert(G_ShotViewPose(TARGET, &e, &pose));
        /* So does time going back, death, a turret or a corpse, an entity
         * not a player, unsmoothed export and leaving the game. */
        for (int guard = 0; guard < 7; guard++) {
            Record(TARGET, 30000, 1.0f, 2);
            assert(g_shotHistory[TARGET].count >= 1);
            switch (guard) {
            case 0: Record(TARGET, 29000, 1.0f, 2); assert(g_shotHistory[TARGET].count == 1); continue;
            case 1: gclients[TARGET].sess.sessionState = SESS_STATE_DEAD; break;
            case 2: gclients[TARGET].sess.connected = CON_CONNECTING; break;
            case 3: gentities[TARGET].s.eFlags |= 0x100; break;
            case 4: gentities[TARGET].s.eType = 2; break;
            case 5: gentities[TARGET].s.pos.trType = TR_INTERPOLATE; break;
            case 6: gentities[TARGET].s.eFlags |= 0x20000; break;
            }
            G_RecordShotSamples();
            assert(g_shotHistory[TARGET].count == 0);
            gclients[TARGET].sess.connected = CON_CONNECTED;
            gclients[TARGET].sess.sessionState = SESS_STATE_PLAYING;
        }
    }
    puts("PASS: the server keeps one sample per command time for the current life and refuses times never recorded, "
         "older than a second, spans and bridges the playback never takes, unknown modes and clients without samples");

    /* Which commands carry a view: attack held or just released, sampled in
     * the cgame's frame; and which players: drawn in the frame on screen,
     * not the shooter, within 10 degrees of the aim or within reach, the
     * nearest four first. */
    {
        bgShotView_t *v;
        memset(cg_shotViewDrawn, 0, sizeof(cg_shotViewDrawn));
        memset(&cgState, 0, sizeof(cgState));
        views[0].ps.clientNum = SHOOTER;
        cg->nextSnap = &views[0];
        cg->oldTime = 700; cg->time = 708;
        cg->refdef.viewaxis[0][0] = 1.0f;
        for (int n = 0; n < 12; n++) {
            float angle = 1.0f + 1.6f * n; /* degrees off the aim */
            cgShotViewDrawn_t *d = &cg_shotViewDrawn[n][n == 9 ? 0 : 1];
            d->frame = n == 10 ? cg->time : cg->oldTime;
            d->entry.clientNum = n;
            d->entry.mode = BG_SHOT_VIEW_SAMPLE;
            d->origin[0] = 1000.0f * cosf(angle * 0.01745329f);
            d->origin[1] = 1000.0f * sinf(angle * 0.01745329f);
            d->origin[2] = -40.0f;
        }
        /* Behind the shooter, but within reach of the eye. */
        cg_shotViewDrawn[11][1].origin[0] = -20.0f; cg_shotViewDrawn[11][1].origin[1] = 0.0f;
        v = &cl_shotViews[1];
        memset(&shooter, 0, sizeof(shooter));
        shooter.cmdNumber = 1;
        shooter.cmds[1].serverTime = cg->time;
        cl_lastAttackTime = -100000;
        cls.realtime = 5000;
        cl_sampleShotViews = 1;
        CL_SampleShotView(&shooter);
        assert(v->count == 0 && v->serverTime == cg->time);
        shooter.cmds[1].buttons = 1;
        CL_SampleShotView(&shooter);
        /* 11 within reach, then 0, 2, 3 by angle: 1 shoots, 10 is only in the
         * frame being drawn; 4 and beyond are past the cone or the four. */
        assert(v->count == 4 && v->entries[0].clientNum == 11 && v->entries[1].clientNum == 0 &&
               v->entries[2].clientNum == 2 && v->entries[3].clientNum == 3);
        shooter.cmds[1].buttons = 0;
        cls.realtime = 5300;
        CL_SampleShotView(&shooter);
        assert(v->count == 4);
        cls.realtime = 5301;
        CL_SampleShotView(&shooter);
        assert(v->count == 0);
        shooter.cmds[1].buttons = 1;
        cl_sampleShotViews = 0;
        CL_SampleShotView(&shooter);
        assert(v->count == 0);
        cl_sampleShotViews = 1;
        cg_shotViewDrawn[11][1].frame = 0;
        cg->refdef.viewaxis[0][0] = cosf(13.0f * 0.01745329f); cg->refdef.viewaxis[0][1] = sinf(13.0f * 0.01745329f);
        CL_SampleShotView(&shooter);
        /* Aimed 13 degrees over: 7 and 8 first, then 6 and 9 at most. */
        assert(v->count == 4 && v->entries[0].clientNum == 7 && v->entries[1].clientNum == 8);
        /* Aimed away from all of them, past the cone: none. */
        cg->refdef.viewaxis[0][0] = 0.0f; cg->refdef.viewaxis[0][1] = 1.0f;
        CL_SampleShotView(&shooter);
        assert(v->count == 0);
    }
    puts("PASS: a command carries a view with attack held or released within 300 ms, sampled in the cgame's frame, "
         "naming the four players nearest its aim drawn in the frame on screen, the shooter never");
}
'''

mutants = {
    'views of the frame being drawn': ('drawn[0].frame == cg->oldTime ? &drawn[0] : drawn[1].frame == cg->oldTime ? &drawn[1] : NULL',
                                       'drawn[0].frame == cg->time ? &drawn[0] : NULL'),
    'no cone': ('            if (!(miss <= CG_SHOT_VIEW_CONE))\n                continue;\n', ''),
    'farthest first': ('k > 0 && misses[k - 1] > miss', 'k > 0 && misses[k - 1] < miss'),
    'every command': ('    else if (cls.realtime - cl_lastAttackTime > CL_SHOT_VIEW_AFTER_ATTACK)\n        return;\n', ''),
    'the oldest commands': ('int first = count > BG_SHOT_VIEW_RECORDS ? count - BG_SHOT_VIEW_RECORDS : 0;', 'int first = 0;'),
    'span and fraction swapped': ('                MSG_WriteShort(buf, entry->span);\n                MSG_WriteShort(buf, entry->frac);',
                                  '                MSG_WriteShort(buf, entry->frac);\n                MSG_WriteShort(buf, entry->span);'),
    'signed fractions': ('                entry->frac = MSG_ReadShort(msg) & 0xffff;\n            } else if (entry->mode == BG_SHOT_VIEW_BRIDGE)',
                         '                entry->frac = MSG_ReadShort(msg);\n            } else if (entry->mode == BG_SHOT_VIEW_BRIDGE)'),
    'truncated kept': ('    if (msg->overflowed)\n        return;\n', ''),
    'any command\'s view': ('        if (view->count && view->serverTime == serverTime)\n            return view;',
                            '        if (view->count)\n            return view;'),
    'nearest sample': ('        if (sample->time == time)\n            return sample;', '        if (sample->time <= time)\n            return sample;'),
    'teleports kept': ('            if ((es->eFlags & 2) != history->teleport || es->pos.trTime < newest)',
                       '            if (es->pos.trTime < newest)'),
    'unbounded rewind': ('    if (!history->count || level.time - entry->time > G_SHOT_MAX_REWIND)', '    if (!history->count)'),
    'unbounded bridge': ('        if (entry->frac < 0 || entry->frac > BG_SHOT_VIEW_BRIDGE_MAX * BG_SHOT_VIEW_BRIDGE_STEPS)\n            return 0;\n', ''),
    'angles kept finer than sent': ('            sample->angles[i] = G_ShotSampleAngle(es->apos.trBase[i]);',
                                    '            sample->angles[i] = es->apos.trBase[i];'),
    'resends duplicated': ('            if (sv_shotViews[clientNum][k].count && sv_shotViews[clientNum][k].serverTime == views[i].serverTime)\n                slot = &sv_shotViews[clientNum][k];\n', ''),
}

with tempfile.TemporaryDirectory(prefix='cod2-shot-views-') as directory:
    path = Path(directory)
    for index, (name, change) in enumerate([('production', None)] + list(mutants.items())):
        variant = code + checks
        if change:
            assert variant.count(change[0]) == 1, (name, variant.count(change[0]))
            variant = variant.replace(*change)
        (path / 'test.c').write_text(variant)
        flags = ['-Wall', '-Wno-unused-function', '-Werror'] if not index else ['-w']
        # The message writes words unaligned, as CoD2's does.
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize=alignment',
                        '-fno-sanitize-recover=all', *flags, str(path / 'test.c'), '-lm', '-o', str(path / 'test')],
                       check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True, timeout=600)
        if not index:
            assert result.returncode == 0, result.stdout + result.stderr[-4000:]
            print(result.stdout, end='')
        else:
            assert result.returncode != 0, f'mutant survived: {name}\n{result.stdout[-2000:]}'
print('PASS: %d broken variants fail: %s' % (len(mutants), ', '.join(mutants)))
