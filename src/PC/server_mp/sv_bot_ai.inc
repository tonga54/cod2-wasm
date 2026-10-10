/* First-party multiplayer AI. Included after the test-client command API.
 * No fields are added to client_t, gclient_t or the network playerstate ABI.
 * Navigation is a bounded, incremental flood of the actual collision world;
 * decisions are staggered, while normal usercmds/aim run at the server tick. */
#include <math.h>
#include "cod2_sprint.h"

extern level_locals_t level;
extern gentity_t g_entities[];
extern const char *SL_ConvertToString(unsigned int stringValue);
extern void G_TraceCapsule(trace_t *, const vec_t *, const vec_t *, const vec_t *, const vec_t *, int, int);
extern float SV_FX_GetVisibility(const vec_t *, const vec_t *);
extern WeaponDef *BG_GetWeaponDef(int weapon);

#define BOT_NAV_MAX 8192
#define BOT_NAV_STEP 32.0f
#define BOT_NAV_EDGES_PER_FRAME 8
#define BOT_THINK_MS 200
#define BOT_WORLD_MASK 0x00810011
#define BOT_RAD_TO_DEG 57.2957795f

typedef struct {
    vec3_t origin;
    short edge[4];
} botNavNode_t;

typedef struct {
    qboolean enabled, wantsMove;
    int difficulty, target, acquiredAt, nextThink, nextPath, nextError;
    int goal, waypoint, lastMoveCheck, avoidUntil, lastFrame;
    unsigned int random;
    vec3_t aim, lastOrigin;
    float yawError, pitchError;
    signed char avoidSide;
} botAIState_t;

static botAIState_t s_botAI[COD2_BOT_MAX_CLIENTS];
static botNavNode_t s_botNav[BOT_NAV_MAX];
static int s_botNavCount, s_botNavNode, s_botNavDirection, s_botNavFrame, s_botDebugAt;
static qboolean s_botNavSeeded;
static const vec3_t s_botZero = {0, 0, 0};
static const vec3_t s_botMins = {-15, -15, 0};
static const vec3_t s_botMaxs = {15, 15, 52};
static const int s_botDirections[4][2] = {{1,0},{0,1},{-1,0},{0,-1}};

static void SV_BotResetAIClient(int clientNum)
{
    memset(&s_botAI[clientNum], 0, sizeof(s_botAI[clientNum]));
}

void SV_BotResetNavigation(void)
{
    memset(s_botAI, 0, sizeof(s_botAI));
    s_botNavCount = s_botNavNode = s_botNavDirection = 0;
    s_botNavFrame = -1;
    s_botNavSeeded = 0;
    s_botDebugAt = 0;
}

static unsigned int Bot_Random(botAIState_t *ai)
{
    /* Separate deterministic RNG: AI never consumes gameplay's random stream. */
    ai->random ^= ai->random << 13;
    ai->random ^= ai->random >> 17;
    ai->random ^= ai->random << 5;
    return ai->random;
}

static float Bot_DistanceSq(const vec_t *a, const vec_t *b)
{
    float x=a[0]-b[0], y=a[1]-b[1], z=a[2]-b[2];
    return x*x+y*y+z*z;
}

static float Bot_AngleDelta(float desired, float current)
{
    float delta = fmodf(desired - current, 360.0f);
    if (delta > 180) delta -= 360;
    if (delta < -180) delta += 360;
    return delta;
}

static float Bot_ApproachAngle(float current, float desired, float step)
{
    float delta = Bot_AngleDelta(desired, current);
    if (delta > step) delta = step;
    if (delta < -step) delta = -step;
    return current + delta;
}

static qboolean Bot_Ground(const vec_t *at, vec_t *ground)
{
    vec3_t start = {at[0], at[1], at[2]+48};
    vec3_t end = {at[0], at[1], at[2]-96};
    trace_t tr;
    memset(&tr, 0, sizeof(tr));
    G_TraceCapsule(&tr, start, s_botZero, s_botZero, end, 1023, BOT_WORLD_MASK);
    if (tr.startsolid || tr.allsolid || tr.fraction >= 1 || tr.normal[2] < .7f)
        return 0;
    ground[0] = at[0]; ground[1] = at[1];
    ground[2] = start[2] + (end[2]-start[2])*tr.fraction + 1;
    return 1;
}

static qboolean Bot_Clear(const vec_t *a, const vec_t *b, int clientNum, int mask)
{
    vec3_t start = {a[0], a[1], a[2]+18};
    vec3_t end = {b[0], b[1], b[2]+18};
    trace_t tr;
    memset(&tr, 0, sizeof(tr));
    G_TraceCapsule(&tr, start, s_botMins, s_botMaxs, end, clientNum, mask);
    return !tr.startsolid && !tr.allsolid && tr.fraction >= 1;
}

static qboolean Bot_WalkEdge(const vec_t *a, const vec_t *b)
{
    vec3_t previous = {a[0], a[1], a[2]}, sample, floor;
    int i;
    // Stair-height steps, headroom and floor checks also reject cliffs/gaps.
    for (i = 1; i <= 4; ++i) {
        sample[0] = a[0]+(b[0]-a[0])*i*.25f;
        sample[1] = a[1]+(b[1]-a[1])*i*.25f;
        sample[2] = previous[2];
        if (!Bot_Ground(sample, floor) || fabsf(floor[2]-previous[2]) > 18.5f ||
            !Bot_Clear(previous, floor, 1023, BOT_WORLD_MASK))
            return 0;
        memcpy(previous, floor, sizeof(previous));
    }
    return fabsf(previous[2]-b[2]) < 20;
}

static int Bot_NavFind(const vec_t *origin)
{
    int i;
    for (i = 0; i < s_botNavCount; ++i)
        if (s_botNav[i].origin[0] == origin[0] && s_botNav[i].origin[1] == origin[1] &&
            fabsf(s_botNav[i].origin[2]-origin[2]) < 24)
            return i;
    return -1;
}

static int Bot_NavAdd(const vec_t *origin)
{
    int i = Bot_NavFind(origin), j;
    if (i >= 0 || s_botNavCount >= BOT_NAV_MAX) return i;
    i = s_botNavCount++;
    memcpy(s_botNav[i].origin, origin, sizeof(vec3_t));
    for (j = 0; j < 4; ++j) s_botNav[i].edge[j] = -1;
    return i;
}

static void Bot_NavSeed(void)
{
    int i;
    s_botNavSeeded = 1;
    for (i = level.maxclients; i < level.num_entities; ++i) {
        gentity_t *ent = &g_entities[i];
        const char *name;
        vec3_t seed, ground;
        if (!ent->r.inuse || !ent->classname) continue;
        name = SL_ConvertToString(ent->classname);
        if (!name || strncmp(name, "mp_", 3) || !strstr(name, "_spawn")) continue;
        seed[0] = floorf(ent->r.currentOrigin[0]/BOT_NAV_STEP+.5f)*BOT_NAV_STEP;
        seed[1] = floorf(ent->r.currentOrigin[1]/BOT_NAV_STEP+.5f)*BOT_NAV_STEP;
        seed[2] = ent->r.currentOrigin[2];
        if (Bot_Ground(seed, ground) && Bot_Clear(ground, ground, 1023, BOT_WORLD_MASK))
            Bot_NavAdd(ground);
    }
    Com_Printf("[bots] navigation seeded: %d nodes\n", s_botNavCount);
}

static void Bot_NavFrame(void)
{
    int budget;
    if (s_botNavFrame == svs.time) return;
    s_botNavFrame = svs.time;
    if (!s_botNavSeeded) Bot_NavSeed();
    for (budget = 0; budget < BOT_NAV_EDGES_PER_FRAME && s_botNavNode < s_botNavCount; ++budget) {
        botNavNode_t *node = &s_botNav[s_botNavNode];
        int direction = s_botNavDirection, next;
        vec3_t candidate = {node->origin[0]+s_botDirections[direction][0]*BOT_NAV_STEP,
                           node->origin[1]+s_botDirections[direction][1]*BOT_NAV_STEP,
                           node->origin[2]}, ground;
        if (node->edge[direction] < 0 && Bot_Ground(candidate, ground) &&
            Bot_WalkEdge(node->origin, ground)) {
            next = Bot_NavAdd(ground);
            if (next >= 0) {
                node->edge[direction] = (short)next;
                s_botNav[next].edge[(direction+2)%4] = (short)s_botNavNode;
            }
        }
        if (++s_botNavDirection == 4) {
            s_botNavDirection = 0;
            ++s_botNavNode;
            if (s_botNavNode == s_botNavCount)
                Com_Printf("[bots] navigation ready: %d nodes\n", s_botNavCount);
        }
    }
}

qboolean SV_BotEnableAI(int clientNum, int difficulty)
{
    botAIState_t *ai;
    playerState_t *ps;
    if (!SV_BotIsTestClient(clientNum) || difficulty < 0 || difficulty > 2) return 0;
    SV_BotResetAIClient(clientNum);
    ai = &s_botAI[clientNum];
    ai->enabled = 1; ai->difficulty = difficulty;
    ai->target = ai->goal = ai->waypoint = -1;
    ai->random = 0x9e3779b9u ^ (unsigned int)(clientNum+1)*0x85ebca6bu ^ (unsigned int)svs.time;
    if (!ai->random) ai->random = 1;
    ai->nextThink = svs.time+(clientNum%4)*50;
    ai->lastFrame = ai->lastMoveCheck = svs.time;
    ps = SV_GameClientNum(clientNum);
    if (ps) {
        memcpy(ai->aim, ps->viewangles, sizeof(vec3_t));
        memcpy(ai->lastOrigin, ps->origin, sizeof(vec3_t));
    }
    Com_Printf("[bots] client %d enabled, difficulty %d\n", clientNum, difficulty);
    return 1;
}

static qboolean Bot_IsEnemy(int selfNum, int otherNum)
{
    gclient_t *self = g_entities[selfNum].client, *other = g_entities[otherNum].client;
    return otherNum != selfNum && other && self &&
        other->sess.connected == 2 && other->sess.sessionState == SESS_STATE_PLAYING &&
        g_entities[otherNum].health > 0 &&
        (self->sess.cs.team == TEAM_FREE || self->sess.cs.team != other->sess.cs.team);
}

static qboolean Bot_CanSee(int selfNum, int otherNum)
{
    playerState_t *self = SV_GameClientNum(selfNum), *other = SV_GameClientNum(otherNum);
    vec3_t eye, target;
    trace_t tr;
    int i;
    if (!self || !other) return 0;
    for (i = 0; i < 3; ++i) { eye[i]=self->origin[i]; target[i]=other->origin[i]; }
    eye[2] += self->viewHeightCurrent;
    target[2] += other->viewHeightCurrent*.72f;
    memset(&tr, 0, sizeof(tr));
    // Visibility needs collision capsules, not animated hit-location skeletons.
    // Actual bullets still use the original weapon trace and damage hitboxes.
    G_TraceCapsule(&tr, eye, s_botZero, s_botZero, target, selfNum, 0x02802831);
    return (tr.fraction >= 1 || tr.entityNum == otherNum) && SV_FX_GetVisibility(eye, target) >= .2f;
}

static int Bot_AcquireTarget(int clientNum, botAIState_t *ai, playerState_t *ps)
{
    int nearest[3] = {-1,-1,-1}, i, j, k;
    float distances[3] = {6250000,6250000,6250000};
    if (ai->target >= 0 && Bot_IsEnemy(clientNum, ai->target) && Bot_CanSee(clientNum, ai->target))
        return ai->target;
    for (i = 0; i < level.maxclients; ++i) {
        float distance;
        if (!Bot_IsEnemy(clientNum, i)) continue;
        distance = Bot_DistanceSq(ps->origin, SV_GameClientNum(i)->origin);
        for (j = 0; j < 3; ++j) {
            if (distance >= distances[j]) continue;
            for (k=2; k>j; --k) { nearest[k]=nearest[k-1]; distances[k]=distances[k-1]; }
            nearest[j]=i; distances[j]=distance; break;
        }
    }
    for (i = 0; i < 3; ++i)
        if (nearest[i] >= 0 && Bot_CanSee(clientNum, nearest[i])) return nearest[i];
    return -1;
}

static int Bot_NearestNode(const vec_t *origin, qboolean requireClear)
{
    // More candidates let a narrow corridor reach past closer nodes behind a wall.
    enum { candidates = 12 };
    int nearest[candidates], i, j, k;
    float distances[candidates];
    for (i = 0; i < candidates; ++i) { nearest[i] = -1; distances[i] = 1e20f; }
    for (i=0; i<s_botNavCount; ++i) {
        float d = Bot_DistanceSq(origin, s_botNav[i].origin);
        if (fabsf(origin[2]-s_botNav[i].origin[2]) > 48) continue;
        for (j=0; j<candidates; ++j) if (d<distances[j]) {
            for(k=candidates-1;k>j;--k) { nearest[k]=nearest[k-1]; distances[k]=distances[k-1]; }
            nearest[j]=i; distances[j]=d; break;
        }
    }
    for(i=0;i<candidates;++i)
        if(nearest[i]>=0 && (!requireClear || (distances[i]<192*192 &&
            Bot_Clear(origin,s_botNav[nearest[i]].origin,1023,BOT_WORLD_MASK)))) return nearest[i];
    return -1;
}

static void Bot_FindPath(botAIState_t *ai, playerState_t *ps)
{
    /* Unweighted BFS is linear in this fixed four-neighbour graph. Workspace
     * is shared because bots run sequentially; no per-frame heap allocation. */
    static short parent[BOT_NAV_MAX], queue[BOT_NAV_MAX];
    int start=Bot_NearestNode(ps->origin,1), head=0, tail=0, i, current, next;
    ai->waypoint=-1;
    if(start<0) return;
    if(ai->goal<0 || ai->goal>=s_botNavCount ||
       Bot_DistanceSq(ps->origin,s_botNav[ai->goal].origin)<80*80)
        ai->goal=s_botNavCount ? (int)(Bot_Random(ai)%s_botNavCount) : start;
    memset(parent,0xff,(size_t)s_botNavCount*sizeof(parent[0]));
    queue[tail++]=(short)start; parent[start]=(short)start;
    while(head<tail) {
        current=queue[head++];
        if(current==ai->goal) break;
        for(i=0;i<4;++i) {
            next=s_botNav[current].edge[i];
            if(next>=0 && parent[next]<0) { parent[next]=(short)current; queue[tail++]=(short)next; }
        }
    }
    if(parent[ai->goal]<0) {
        // Pick a reachable patrol goal rather than walking into an obstruction.
        ai->goal=queue[Bot_Random(ai)%(unsigned int)tail];
    }
    current=ai->goal;
    while(parent[current]!=start && current!=start) current=parent[current];
    ai->waypoint=Bot_DistanceSq(ps->origin,s_botNav[start].origin)>12*12 ? start : current;
}

static void Bot_Think(int clientNum, botAIState_t *ai, playerState_t *ps)
{
    int target=Bot_AcquireTarget(clientNum,ai,ps);
    if(target!=ai->target) { ai->target=target; ai->acquiredAt=svs.time; ai->nextPath=0; }
    // Pursue only a position that passed visibility checks; never follow an
    // unseen opponent's live position through buildings or smoke.
    if (target >= 0) {
        int goal = Bot_NearestNode(SV_GameClientNum(target)->origin, 0);
        if (goal >= 0) ai->goal = goal;
    }
    if(svs.time>=ai->nextError) {
        static const float error[3]={4.5f,2.0f,.7f};
        ai->yawError=((int)(Bot_Random(ai)%2001)-1000)*.001f*error[ai->difficulty];
        ai->pitchError=((int)(Bot_Random(ai)%2001)-1000)*.001f*error[ai->difficulty];
        ai->nextError=svs.time+400;
    }
    if(svs.time>=ai->nextPath) {
        Bot_FindPath(ai,ps);
        ai->nextPath=svs.time+400;
    }
    if(svs.time-ai->lastMoveCheck>=900) {
        if(Bot_DistanceSq(ps->origin,ai->lastOrigin)<12*12 &&
           ai->wantsMove) {
            ai->avoidUntil=svs.time+600;
            ai->avoidSide=(Bot_Random(ai)&1)?100:-100;
            ai->goal=-1; ai->nextPath=0;
        }
        memcpy(ai->lastOrigin,ps->origin,sizeof(vec3_t));
        ai->lastMoveCheck=svs.time;
    }
}

static void SV_BotRunAI(int clientNum, playerState_t *ps)
{
    botAIState_t *ai=&s_botAI[clientNum];
    cod2BotCommandState_t *cmd=&s_botCmdState[clientNum];
    gclient_t *client=g_entities[clientNum].client;
    float desiredYaw, desiredPitch=0, moveYaw, turnStep, distance=0;
    int oldButtons=cmd->buttons, elapsed, firing=0;
    qboolean visible=0;
    if(!ai->enabled || !client) return;
    cmd->buttons=0; cmd->forwardmove=cmd->rightmove=0; cmd->hasWeapon=0;
    if(client->sess.sessionState!=SESS_STATE_PLAYING || g_entities[clientNum].health<=0) {
        ai->target=-1; ai->acquiredAt=svs.time; ai->wantsMove=0;
        // Original gametype respawn/round logic consumes use, not a forced spawn.
        if(client->sess.sessionState==SESS_STATE_DEAD) cmd->buttons=0x8;
        memcpy(ai->aim,ps->viewangles,sizeof(vec3_t));
        ai->lastFrame=svs.time;
        return;
    }
    Bot_NavFrame();
    if (svs.time >= s_botDebugAt) {
        int i;
        s_botDebugAt = svs.time + 5000;
        if (Dvar_GetInt("scr_bot_debug"))
            for (i = 0; i < level.maxclients; ++i)
                if (s_botAI[i].enabled)
                    Com_Printf("[bots] state %d team=%d state=%d health=%d pos=%.0f,%.0f,%.0f target=%d goal=%d waypoint=%d\n",
                        i, g_entities[i].client->sess.cs.team, g_entities[i].client->sess.sessionState,
                        g_entities[i].health, SV_GameClientNum(i)->origin[0], SV_GameClientNum(i)->origin[1],
                        SV_GameClientNum(i)->origin[2], s_botAI[i].target, s_botAI[i].goal, s_botAI[i].waypoint);
    }
    elapsed=svs.time-ai->lastFrame;
    if(elapsed<0 || elapsed>100) elapsed=50;
    ai->lastFrame=svs.time;
    if(svs.time>=ai->nextThink) {
        Bot_Think(clientNum,ai,ps);
        ai->nextThink=svs.time+BOT_THINK_MS;
    }
    // Advance a reached waypoint immediately; waiting for the slower decision
    // cadence produces a visible stop at every navigation cell.
    if (ai->waypoint >= 0 && Bot_DistanceSq(ps->origin, s_botNav[ai->waypoint].origin) < 12*12) {
        Bot_FindPath(ai, ps);
        ai->nextPath = svs.time + 400;
    }
    desiredYaw=ai->aim[1];
    if(ai->waypoint>=0) {
        const vec_t *waypoint=s_botNav[ai->waypoint].origin;
        moveYaw=atan2f(waypoint[1]-ps->origin[1],waypoint[0]-ps->origin[0])*BOT_RAD_TO_DEG;
        desiredYaw=moveYaw;
        distance=Bot_DistanceSq(ps->origin,waypoint);
    } else moveYaw=desiredYaw;
    if(ai->target>=0 && Bot_IsEnemy(clientNum,ai->target)) {
        playerState_t *enemy=SV_GameClientNum(ai->target);
        float x=enemy->origin[0]-ps->origin[0], y=enemy->origin[1]-ps->origin[1];
        float z=enemy->origin[2]+enemy->viewHeightCurrent*.72f-ps->origin[2]-ps->viewHeightCurrent;
        visible=Bot_CanSee(clientNum,ai->target);
        if(visible) {
            desiredYaw=atan2f(y,x)*BOT_RAD_TO_DEG+ai->yawError;
            desiredPitch=-atan2f(z,sqrtf(x*x+y*y))*BOT_RAD_TO_DEG+ai->pitchError;
            static const int reaction[3]={700,400,200};
            if(svs.time-ai->acquiredAt>=reaction[ai->difficulty] &&
               fabsf(Bot_AngleDelta(desiredYaw,ai->aim[1]))<6 &&
               fabsf(Bot_AngleDelta(desiredPitch,ai->aim[0]))<6) firing=1;
            if(x*x+y*y>350*350) cmd->buttons|=0x1000;
        }
    }
    turnStep=(ai->difficulty==0?150.0f:ai->difficulty==1?220.0f:300.0f)*elapsed*.001f;
    ai->aim[0]=Bot_ApproachAngle(ai->aim[0],desiredPitch,turnStep);
    ai->aim[1]=Bot_ApproachAngle(ai->aim[1],desiredYaw,turnStep);
    ai->aim[2]=0;
    memcpy(cmd->angles,ai->aim,sizeof(vec3_t)); cmd->hasAngles=1;
    if(ai->waypoint>=0 && distance>10*10) {
        float delta=(moveYaw-ai->aim[1])/BOT_RAD_TO_DEG;
        int speed=visible?65:127;
        cmd->forwardmove=(signed char)(cosf(delta)*speed);
        cmd->rightmove=(signed char)(-sinf(delta)*speed);
        if(!visible && distance>120*120 && cmd->forwardmove>110) cmd->buttons|=BUTTON_SPRINT;
    }
    if(ai->avoidUntil>svs.time) {
        cmd->forwardmove=-60; cmd->rightmove=ai->avoidSide; cmd->buttons&=~BUTTON_SPRINT;
    }
    ai->wantsMove = cmd->forwardmove || cmd->rightmove;
    if(ps->weapon>0) {
        WeaponDef *weapon=BG_GetWeaponDef(ps->weapon);
        if(weapon && weapon->iClipIndex>=0 && weapon->iClipIndex<128) {
            if (!ps->ammoclip[weapon->iClipIndex] && weapon->iAmmoIndex >= 0 &&
                weapon->iAmmoIndex < 128 && ps->ammo[weapon->iAmmoIndex] <= 0) {
                int slot;
                // Switch to an owned loaded weapon instead of inventing ammo.
                for (slot = 0; slot < 8; ++slot) {
                    int index = (unsigned char)ps->weaponslots[slot];
                    WeaponDef *other;
                    if (!index || index == ps->weapon) continue;
                    other = BG_GetWeaponDef(index);
                    if (other && other->iClipIndex >= 0 && other->iClipIndex < 128 &&
                        ps->ammoclip[other->iClipIndex] > 0) {
                        cmd->hasWeapon = 1; cmd->weapon = (byte)index; break;
                    }
                }
            }
            else if(!ps->ammoclip[weapon->iClipIndex] && weapon->iAmmoIndex>=0 &&
               weapon->iAmmoIndex<128 && ps->ammo[weapon->iAmmoIndex]>0) cmd->buttons|=0x10;
            else if(firing && svs.time%850<550 &&
                    (!weapon->bSemiAuto || (!(oldButtons&1) && ps->weaponTime<=0)))
                cmd->buttons|=1;
        }
    }
}
