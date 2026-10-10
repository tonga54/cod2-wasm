/* First-party multiplayer AI. Included after the test-client command API.
 * No fields are added to client_t, gclient_t or the network playerstate ABI.
 * Navigation is a bounded, incremental flood of the actual collision world;
 * decisions are staggered, while normal usercmds/aim run at the server tick. */
#include <math.h>
#include "cod2_sprint.h"

extern int Sys_Milliseconds(void);
extern level_locals_t level;
extern gentity_t g_entities[];
extern const char *SL_ConvertToString(unsigned int stringValue);
extern void G_TraceCapsule(trace_t *, const vec_t *, const vec_t *, const vec_t *, const vec_t *, int, int);
extern float SV_FX_GetVisibility(const vec_t *, const vec_t *);
extern WeaponDef *BG_GetWeaponDef(int weapon);
extern qboolean SV_EntityContact(const vec_t *, const vec_t *, const gentity_t *);

#define BOT_NAV_MAX 32768
#define BOT_NAV_HASH 65536
#define BOT_NAV_STEP 32.0f
#define BOT_NAV_EDGES_PER_FRAME 128
#define BOT_NAV_TIME_MS 4
#define BOT_NAV_DIRECTIONS 8
#define BOT_NAV_LINKS (BOT_NAV_DIRECTIONS+1)
#define BOT_ROUTE_MAX 64
#define BOT_BLOCKED_MAX 8
#define BOT_THINK_MS 200
#define BOT_WORLD_MASK 0x00810011
#define BOT_RAD_TO_DEG 57.2957795f

typedef struct {
    vec3_t origin;
    short edge[BOT_NAV_LINKS];
    float ladderYaw, priority;
} botNavNode_t;

typedef struct {
    qboolean enabled, wantsMove;
    int difficulty, target, acquiredAt, nextThink, nextPath, nextError;
    int goal, waypoint, lastMoveCheck, avoidUntil, lastFrame;
    int objectiveUntil, objectiveBehavior, objectiveEntity, memoryUntil, coverUntil;
    unsigned int objectiveClassname;
    int nextCover, burstUntil, nextBurst, lastHealth;
    int alertUntil;
    int guardAt, nextDodge, dodgeUntil, lastTarget, lastVisibleAt;
    int routeCount, routeIndex, routeFrom, blockedIndex, stuckCount, detourUntil;
    int perchUntil, guardUntil;
    short route[BOT_ROUTE_MAX], blockedFrom[BOT_BLOCKED_MAX], blockedTo[BOT_BLOCKED_MAX];
    int blockedUntil[BOT_BLOCKED_MAX];
    unsigned int random;
    vec3_t aim, lastOrigin, objective, lastSeen, cover, guardPoint, guardAnchor, perchOrigin;
    float yawError, pitchError, objectiveRadius, alertYaw, guardYaw;
    signed char avoidSide;
} botAIState_t;

static botAIState_t s_botAI[COD2_BOT_MAX_CLIENTS];
static botNavNode_t s_botNav[BOT_NAV_MAX];
static unsigned short s_botNavHash[BOT_NAV_HASH];
static short s_botNavOpen[BOT_NAV_MAX];
static int s_botNavOpenCount, s_botNavWorkEdges;
static int s_botNavCount, s_botNavNode, s_botNavDirection, s_botNavFrame, s_botDebugAt;
static qboolean s_botNavSeeded, s_botNavBounded;
static vec3_t s_botNavMin, s_botNavMax;
static const vec3_t s_botZero = {0, 0, 0};
static const vec3_t s_botMins = {-15, -15, 0};
static const vec3_t s_botMaxs = {15, 15, 52};
static const int s_botDirections[BOT_NAV_DIRECTIONS][2] =
    {{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1}};

static void SV_BotResetAIClient(int clientNum)
{
    memset(&s_botAI[clientNum], 0, sizeof(s_botAI[clientNum]));
}

void SV_BotResetNavigation(void)
{
    memset(s_botAI, 0, sizeof(s_botAI));
    memset(s_botNavHash, 0, sizeof(s_botNavHash));
    s_botNavCount = s_botNavDirection = s_botNavOpenCount = s_botNavWorkEdges = 0;
    s_botNavNode = -1;
    s_botNavFrame = -1;
    s_botNavSeeded = s_botNavBounded = 0;
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
    vec3_t start = {at[0], at[1], at[2]+18.5f};
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
    int i, samples=(int)ceilf(sqrtf(Bot_DistanceSq(a,b))/8.0f);
    if(samples<4) samples=4;
    if(samples>32) return 0;
    // Stair-height steps, headroom and floor checks also reject cliffs/gaps.
    for (i = 1; i <= samples; ++i) {
        sample[0] = a[0]+(b[0]-a[0])*i/samples;
        sample[1] = a[1]+(b[1]-a[1])*i/samples;
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
    unsigned int hash = ((unsigned int)(int)floorf(origin[0]/BOT_NAV_STEP)*73856093u ^
                         (unsigned int)(int)floorf(origin[1]/BOT_NAV_STEP)*19349663u) & (BOT_NAV_HASH-1);
    int i;
    while (s_botNavHash[hash]) {
        i = s_botNavHash[hash]-1;
        if (s_botNav[i].origin[0] == origin[0] && s_botNav[i].origin[1] == origin[1] &&
            fabsf(s_botNav[i].origin[2]-origin[2]) < 24)
            return i;
        hash = (hash+1)&(BOT_NAV_HASH-1);
    }
    return -1;
}

static qboolean Bot_NavBefore(int a, int b)
{
    return s_botNav[a].priority<s_botNav[b].priority ||
        (s_botNav[a].priority==s_botNav[b].priority && a<b);
}

static void Bot_NavQueue(int node)
{
    int at=s_botNavOpenCount++;
    float x=s_botNav[node].origin[0]-(s_botNavMin[0]+s_botNavMax[0])*.5f;
    float y=s_botNav[node].origin[1]-(s_botNavMin[1]+s_botNavMax[1])*.5f;
    s_botNav[node].priority=s_botNavBounded?x*x+y*y:0;
    while(at && Bot_NavBefore(node,s_botNavOpen[(at-1)/2])) {
        s_botNavOpen[at]=s_botNavOpen[(at-1)/2]; at=(at-1)/2;
    }
    s_botNavOpen[at]=(short)node;
}

static int Bot_NavPop(void)
{
    int result, last, at=0;
    if(!s_botNavOpenCount) return -1;
    result=s_botNavOpen[0]; last=s_botNavOpen[--s_botNavOpenCount];
    while(at*2+1<s_botNavOpenCount) {
        int child=at*2+1;
        if(child+1<s_botNavOpenCount && Bot_NavBefore(s_botNavOpen[child+1],s_botNavOpen[child])) ++child;
        if(!Bot_NavBefore(s_botNavOpen[child],last)) break;
        s_botNavOpen[at]=s_botNavOpen[child]; at=child;
    }
    if(s_botNavOpenCount) s_botNavOpen[at]=(short)last;
    return result;
}

static int Bot_NavAdd(const vec_t *origin)
{
    int i = Bot_NavFind(origin), j;
    unsigned int hash;
    if (i >= 0 || s_botNavCount >= BOT_NAV_MAX) return i;
    if(s_botNavBounded && (origin[0]<s_botNavMin[0] || origin[0]>s_botNavMax[0] ||
       origin[1]<s_botNavMin[1] || origin[1]>s_botNavMax[1])) return -1;
    i = s_botNavCount++;
    memcpy(s_botNav[i].origin, origin, sizeof(vec3_t));
    for (j = 0; j < BOT_NAV_LINKS; ++j) s_botNav[i].edge[j] = -1;
    hash = ((unsigned int)(int)floorf(origin[0]/BOT_NAV_STEP)*73856093u ^
            (unsigned int)(int)floorf(origin[1]/BOT_NAV_STEP)*19349663u) & (BOT_NAV_HASH-1);
    while (s_botNavHash[hash]) hash = (hash+1)&(BOT_NAV_HASH-1);
    s_botNavHash[hash] = (unsigned short)(i+1);
    Bot_NavQueue(i);
    return i;
}

static void Bot_NavSeedPoint(const vec_t *origin)
{
    int x,y;
    vec3_t point,ground;
    for(x=-1;x<=1;++x) for(y=-1;y<=1;++y) {
        point[0]=(floorf(origin[0]/BOT_NAV_STEP+.5f)+x)*BOT_NAV_STEP;
        point[1]=(floorf(origin[1]/BOT_NAV_STEP+.5f)+y)*BOT_NAV_STEP;
        point[2]=origin[2];
        if(Bot_NavFind(point)<0 && Bot_Ground(point,ground) &&
           Bot_Clear(ground,ground,1023,BOT_WORLD_MASK)) Bot_NavAdd(ground);
    }
}

static void Bot_NavSeed(void)
{
    int i;
    s_botNavSeeded = 1;
    // Collision can contain an unbounded ground plane outside the arena.
    // Restrict exploration to the authored spawn envelope plus its buildings.
    for(i=0;i<3;++i) { s_botNavMin[i]=1e20f; s_botNavMax[i]=-1e20f; }
    for(i=level.maxclients;i<level.num_entities;++i) {
        gentity_t *ent=&g_entities[i];
        const char *name=ent->r.inuse && ent->classname?SL_ConvertToString(ent->classname):NULL;
        int j;
        if(!name || strncmp(name,"mp_",3) || !strstr(name,"_spawn")) continue;
        for(j=0;j<3;++j) {
            if(ent->r.currentOrigin[j]<s_botNavMin[j]) s_botNavMin[j]=ent->r.currentOrigin[j];
            if(ent->r.currentOrigin[j]>s_botNavMax[j]) s_botNavMax[j]=ent->r.currentOrigin[j];
        }
    }
    if(s_botNavMin[0]<s_botNavMax[0]) {
        for(i=0;i<3;++i) { s_botNavMin[i]-=512; s_botNavMax[i]+=512; }
        s_botNavBounded=1;
    }
    // Prioritize shared combat lanes before filling remote spawn courtyards.
    // Rebuild priorities for any objectives seeded before the first AI tick.
    s_botNavOpenCount=0;
    for(i=0;i<s_botNavCount;++i) Bot_NavQueue(i);
    for (i = level.maxclients; i < level.num_entities; ++i) {
        gentity_t *ent = &g_entities[i];
        const char *name;
        if (!ent->r.inuse || !ent->classname) continue;
        name = SL_ConvertToString(ent->classname);
        if (!name || strncmp(name, "mp_", 3) || !strstr(name, "_spawn")) continue;
        Bot_NavSeedPoint(ent->r.currentOrigin);
    }
    Com_Printf("[bots] navigation seeded: %d nodes\n", s_botNavCount);
}

static void Bot_NavLadder(int from, int direction)
{
    botNavNode_t *node=&s_botNav[from];
    vec3_t start={node->origin[0],node->origin[1],node->origin[2]+40},end;
    trace_t tr;
    int height,last=0,offset,next;
    if(node->edge[BOT_NAV_DIRECTIONS]>=0 || (direction&1)) return;
    end[0]=start[0]+s_botDirections[direction][0]*32;
    end[1]=start[1]+s_botDirections[direction][1]*32;
    end[2]=start[2];
    memset(&tr,0,sizeof(tr));
    G_TraceCapsule(&tr,start,s_botZero,s_botZero,end,1023,0x10000);
    if(tr.fraction>=1 || tr.startsolid || !(tr.surfaceFlags&8) || fabsf(tr.normal[2])>.3f) return;
    for(height=16;height<=384;height+=16) {
        start[2]=end[2]=node->origin[2]+40+height;
        memset(&tr,0,sizeof(tr));
        G_TraceCapsule(&tr,start,s_botZero,s_botZero,end,1023,0x10000);
        if(tr.fraction>=1 || tr.startsolid || !(tr.surfaceFlags&8)) break;
        last=height;
    }
    if(last<32 || height>384) return;
    for(offset=32;offset<=96;offset+=32) {
        vec3_t at={node->origin[0]+s_botDirections[direction][0]*offset,
                   node->origin[1]+s_botDirections[direction][1]*offset,node->origin[2]+last+72},ground;
        if(!Bot_Ground(at,ground) || ground[2]<node->origin[2]+48 ||
           ground[2]>node->origin[2]+384 || !Bot_Clear(ground,ground,1023,BOT_WORLD_MASK)) continue;
        next=Bot_NavAdd(ground);
        if(next<0 || s_botNav[next].edge[BOT_NAV_DIRECTIONS]>=0) continue;
        node->edge[BOT_NAV_DIRECTIONS]=(short)next;
        s_botNav[next].edge[BOT_NAV_DIRECTIONS]=(short)from;
        node->ladderYaw=s_botNav[next].ladderYaw=atan2f(s_botDirections[direction][1],s_botDirections[direction][0])*BOT_RAD_TO_DEG;
        if(Dvar_GetInt("scr_bot_debug")) Com_Printf("[bots] ladder %d -> %d at %.0f,%.0f,%.0f -> %.0f,%.0f,%.0f\n",
            from,next,node->origin[0],node->origin[1],node->origin[2],ground[0],ground[1],ground[2]);
        return;
    }
}

static int Bot_NavEdge(int from, int direction)
{
    botNavNode_t *node=&s_botNav[from];
    int next;
    vec3_t candidate={node->origin[0]+s_botDirections[direction][0]*BOT_NAV_STEP,
                      node->origin[1]+s_botDirections[direction][1]*BOT_NAV_STEP,node->origin[2]},ground;
    if(node->edge[direction]>=0) return node->edge[direction];
    if(Bot_Ground(candidate,ground) && Bot_WalkEdge(node->origin,ground)) {
        next=Bot_NavAdd(ground);
        if(next>=0) {
            node->edge[direction]=(short)next;
            s_botNav[next].edge[(direction+4)%BOT_NAV_DIRECTIONS]=(short)from;
            return next;
        }
    }
    Bot_NavLadder(from,direction);
    return -1;
}

static void Bot_NavScout(int start, const vec_t *desired)
{
    int current=start, step, started=Sys_Milliseconds(), visited[7];
    visited[0]=start;
    // Extend a short local branch, including a turn away from the objective
    // when a wall requires it. Never bounce between the same two scout cells.
    for(step=0;step<6;++step) {
        int attempt, best=-1;
        float bestDistance=1e30f;
        float yaw=atan2f(desired[1]-s_botNav[current].origin[1],desired[0]-s_botNav[current].origin[0]);
        int direction=((int)floorf(yaw*4/3.14159265f+.5f)+8)%8;
        static const int offsets[8]={0,1,-1,2,-2,3,-3,4};
        for(attempt=0;attempt<8;++attempt) {
            int next=Bot_NavEdge(current,(direction+offsets[attempt]+8)%8),i;
            float distance;
            if(next>=0) {
                for(i=0;i<=step && visited[i]!=next;++i) {}
                distance=Bot_DistanceSq(s_botNav[next].origin,desired);
                if(i>step && distance<bestDistance) { best=next; bestDistance=distance; }
                if(best>=0 && distance<Bot_DistanceSq(s_botNav[current].origin,desired)) break;
            }
            if(Sys_Milliseconds()-started>=BOT_NAV_TIME_MS) return;
        }
        if(best<0 || Sys_Milliseconds()-started>=BOT_NAV_TIME_MS) return;
        current=visited[step+1]=best;
    }
}

static void Bot_NavFrame(void)
{
    int budget, started=Sys_Milliseconds();
    if (s_botNavFrame == svs.time) return;
    s_botNavFrame = svs.time;
    if (!s_botNavSeeded) Bot_NavSeed();
    for (budget = 0; budget < BOT_NAV_EDGES_PER_FRAME; ++budget) {
        if(s_botNavNode<0) s_botNavNode=Bot_NavPop();
        if(s_botNavNode<0) break;
        ++s_botNavWorkEdges;
        Bot_NavEdge(s_botNavNode,s_botNavDirection);
        if (++s_botNavDirection == BOT_NAV_DIRECTIONS) {
            s_botNavDirection = 0;
            s_botNavNode=-1;
            if (!s_botNavOpenCount)
                Com_Printf("[bots] navigation ready: %d nodes\n", s_botNavCount);
        }
        // Accelerate warm-up when traces are cheap, with a wall-time bound to
        // protect the multiplayer tick on the Pi and on slower hosts.
        if((budget&7)==7 && Sys_Milliseconds()-started>=BOT_NAV_TIME_MS) break;
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
    ai->target = ai->lastTarget = ai->goal = ai->waypoint = ai->routeFrom = -1;
    ai->objectiveEntity = -1;
    ai->lastHealth = g_entities[clientNum].health;
    ai->random = 0x9e3779b9u ^ (unsigned int)(clientNum+1)*0x85ebca6bu ^ (unsigned int)svs.time;
    if (!ai->random) ai->random = 1;
    ai->nextThink = svs.time+(clientNum%4)*50;
    ai->lastFrame = ai->lastMoveCheck = svs.time;
    ps = SV_GameClientNum(clientNum);
    if (ps) {
        memcpy(ai->aim, ps->viewangles, sizeof(vec3_t));
        memcpy(ai->lastOrigin, ps->origin, sizeof(vec3_t));
        memcpy(ai->perchOrigin, ps->origin, sizeof(vec3_t));
    }
    Com_Printf("[bots] client %d enabled, difficulty %d\n", clientNum, difficulty);
    return 1;
}

qboolean SV_BotSetGoal(int clientNum, const vec_t *origin, float radius, int behavior, int entityNum)
{
    botAIState_t *ai;
    int i;
    if(!origin || !SV_BotIsTestClient(clientNum) || !s_botAI[clientNum].enabled ||
       !isfinite(radius) || radius<8 || radius>512 || behavior<0 || behavior>3 ||
       entityNum < -1 || entityNum >= 1023) return 0;
    for(i=0;i<3;++i) if(!isfinite(origin[i]) || fabsf(origin[i])>131072) return 0;
    ai=&s_botAI[clientNum];
    if(ai->objectiveUntil<=svs.time || ai->objectiveBehavior!=behavior ||
       Bot_DistanceSq(ai->objective,origin)>32*32) {
        ai->nextPath=0;
        ai->goal=-1; ai->routeCount=0; ai->guardUntil=0;
        Bot_NavSeedPoint(origin);
        if(entityNum>=0) {
            vec3_t entry={origin[0],origin[1],g_entities[entityNum].r.absmin[2]+8};
            // A bomb volume's origin can be high enough to seed the roof above
            // it. Also seed from its lower plane, inside the intended floor.
            Bot_NavSeedPoint(entry);
        }
    }
    memcpy(ai->objective,origin,sizeof(vec3_t));
    ai->objectiveRadius=radius; ai->objectiveBehavior=behavior; ai->objectiveEntity=entityNum;
    ai->objectiveClassname=entityNum>=0?g_entities[entityNum].classname:0;
    ai->objectiveUntil=svs.time+1600;
    return 1;
}

qboolean SV_BotClearGoal(int clientNum)
{
    if(!SV_BotIsTestClient(clientNum) || !s_botAI[clientNum].enabled) return 0;
    if(s_botAI[clientNum].objectiveUntil) {
        s_botAI[clientNum].objectiveUntil=0;
        s_botAI[clientNum].objectiveEntity=-1;
        s_botAI[clientNum].goal=-1;
        s_botAI[clientNum].nextPath=0;
    }
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
        // A new enemy must be in the bot's view cone. Close, visible footsteps
        // can alert it; a hidden opponent's live position never enters memory.
        if (distance > 96*96) {
            playerState_t *other=SV_GameClientNum(i);
            float yaw=atan2f(other->origin[1]-ps->origin[1],other->origin[0]-ps->origin[0])*BOT_RAD_TO_DEG;
            if(fabsf(Bot_AngleDelta(yaw,ai->aim[1]))>(ai->difficulty==0?55:ai->difficulty==1?65:75)) continue;
        }
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

static qboolean Bot_InObjective(const vec_t *origin, int entityNum)
{
    vec3_t mins={origin[0]-15,origin[1]-15,origin[2]},
           maxs={origin[0]+15,origin[1]+15,origin[2]+70};
    return SV_EntityContact(mins,maxs,&g_entities[entityNum]);
}

static int Bot_NearestNodeForGoal(const vec_t *origin, qboolean requireClear, int entityNum)
{
    // More candidates let a narrow corridor reach past closer nodes behind a wall.
    enum { candidates = 12 };
    int nearest[candidates], i, j, k, x, y;
    float distances[candidates];
    for (i = 0; i < candidates; ++i) { nearest[i] = -1; distances[i] = 1e20f; }
    // Look up nearby grid cells instead of rescanning the whole map for every
    // bot waypoint. Hash probes still distinguish floors at the same x/y.
    for(x=-6;x<=6;++x) for(y=-6;y<=6;++y) {
      float gridX=(floorf(origin[0]/BOT_NAV_STEP)+x)*BOT_NAV_STEP;
      float gridY=(floorf(origin[1]/BOT_NAV_STEP)+y)*BOT_NAV_STEP;
      unsigned int hash=((unsigned int)(int)(gridX/BOT_NAV_STEP)*73856093u ^
                         (unsigned int)(int)(gridY/BOT_NAV_STEP)*19349663u)&(BOT_NAV_HASH-1);
      while(s_botNavHash[hash]) {
        i=s_botNavHash[hash]-1;
        hash=(hash+1)&(BOT_NAV_HASH-1);
        if(s_botNav[i].origin[0]!=gridX || s_botNav[i].origin[1]!=gridY) continue;
        float d = Bot_DistanceSq(origin, s_botNav[i].origin);
        if (fabsf(origin[2]-s_botNav[i].origin[2]) > (entityNum>=0?96:48)) continue;
        if(entityNum>=0 && (s_botNav[i].origin[2]>=g_entities[entityNum].r.absmax[2] ||
           s_botNav[i].origin[2]+70<=g_entities[entityNum].r.absmin[2])) continue;
        for (j=0; j<candidates; ++j) if (d<distances[j]) {
            for(k=candidates-1;k>j;--k) { nearest[k]=nearest[k-1]; distances[k]=distances[k-1]; }
            nearest[j]=i; distances[j]=d; break;
        }
      }
    }
    for(i=0;i<candidates;++i)
        if(nearest[i]>=0 && (entityNum<0 || Bot_InObjective(s_botNav[nearest[i]].origin,entityNum)) &&
           (!requireClear || (distances[i]<192*192 &&
            Bot_Clear(origin,s_botNav[nearest[i]].origin,1023,BOT_WORLD_MASK)))) return nearest[i];
    return -1;
}

static int Bot_NearestNode(const vec_t *origin, qboolean requireClear)
{
    return Bot_NearestNodeForGoal(origin,requireClear,-1);
}

static qboolean Bot_IsSniper(playerState_t *ps)
{
    WeaponDef *weapon=ps->weapon>0?BG_GetWeaponDef(ps->weapon):NULL;
    return weapon && weapon->overlayReticle;
}

static qboolean Bot_EdgeBlocked(botAIState_t *ai, int from, int to)
{
    int i;
    for(i=0;i<BOT_BLOCKED_MAX;++i)
        if(ai->blockedUntil[i]>svs.time && ai->blockedFrom[i]==from && ai->blockedTo[i]==to) return 1;
    return 0;
}

static void Bot_BlockRoute(botAIState_t *ai)
{
    if(ai->routeFrom>=0 && ai->routeIndex<ai->routeCount) {
        int index=ai->blockedIndex++%BOT_BLOCKED_MAX;
        ai->blockedFrom[index]=ai->routeIndex>0?ai->route[ai->routeIndex-1]:(short)ai->routeFrom;
        ai->blockedTo[index]=ai->route[ai->routeIndex];
        ai->blockedUntil[index]=svs.time+8000;
    }
    ai->routeCount=0; ai->waypoint=-1; ai->nextPath=0;
}

static int Bot_PatrolGoal(botAIState_t *ai, playerState_t *ps, const short *nodes, int count)
{
    int i,best=-1;
    float bestScore=-1e30f;
    if(!count) return -1;
    for(i=0;i<16;++i) {
        int node=nodes?nodes[Bot_Random(ai)%(unsigned int)count]:(int)(Bot_Random(ai)%(unsigned int)count);
        float distance=Bot_DistanceSq(ps->origin,s_botNav[node].origin);
        float score=sqrtf(distance)*.25f;
        if(distance<256*256 || distance>2500*2500) score-=1000;
        if(distance<24*24) score-=10000; // An idle cell is not a patrol destination.
        if(node==ai->goal) score-=500;
        if(s_botNavBounded) {
            vec3_t center={(s_botNavMin[0]+s_botNavMax[0])*.5f,
                           (s_botNavMin[1]+s_botNavMax[1])*.5f,s_botNav[node].origin[2]};
            // Search the arena's shared fighting area rather than repeatedly
            // wandering around distant team spawns. Only map geometry is used.
            score-=sqrtf(Bot_DistanceSq(center,s_botNav[node].origin))*.6f;
        }
        if(Bot_IsSniper(ps)) {
            // Geometry, not enemy locations, defines useful lookout positions.
            int direction;
            score+=fmaxf(-128,fminf(256,s_botNav[node].origin[2]-ps->origin[2]))*2;
            for(direction=0;direction<4;++direction) {
                vec3_t start={s_botNav[node].origin[0],s_botNav[node].origin[1],s_botNav[node].origin[2]+55};
                vec3_t end={start[0]+s_botDirections[direction*2][0]*1024,
                           start[1]+s_botDirections[direction*2][1]*1024,start[2]};
                trace_t tr;
                memset(&tr,0,sizeof(tr));
                G_TraceCapsule(&tr,start,s_botZero,s_botZero,end,1023,BOT_WORLD_MASK);
                score+=tr.fraction*120;
            }
        }
        if(score>bestScore) { best=node; bestScore=score; }
    }
    return best;
}

static void Bot_GuardPoint(int clientNum, botAIState_t *ai, playerState_t *ps)
{
    int i,best=-1;
    float bestScore=1e30f;
    float angle=(clientNum*.618034f-floorf(clientNum*.618034f))*6.2831853f;
    float radius=fminf(160,ai->objectiveRadius*.8f);
    vec3_t desired={ai->objective[0]+cosf(angle)*radius,ai->objective[1]+sinf(angle)*radius,ai->objective[2]};
    if(ai->guardUntil>svs.time && Bot_DistanceSq(ai->guardAnchor,ai->objective)<64*64) return;
    for(i=0;i<24;++i) {
        float a=angle+i*6.2831853f/24;
        vec3_t at={ai->objective[0]+cosf(a)*radius,ai->objective[1]+sinf(a)*radius,ai->objective[2]};
        vec3_t ground;
        int node=Bot_Ground(at,ground)?Bot_NearestNode(ground,0):-1;
        float score;
        if(node<0 || Bot_DistanceSq(ground,s_botNav[node].origin)>64*64) continue;
        score=Bot_DistanceSq(desired,s_botNav[node].origin);
        if(score<bestScore) { best=node; bestScore=score; }
    }
    ai->nextPath=0;
    memcpy(ai->guardPoint,best>=0?s_botNav[best].origin:ai->objective,sizeof(vec3_t));
    memcpy(ai->guardAnchor,ai->objective,sizeof(vec3_t));
    ai->guardUntil=svs.time+3000;
    (void)ps;
}

static void Bot_FindPath(botAIState_t *ai, playerState_t *ps)
{
    if(ai->routeIndex<ai->routeCount) {
        int from=ai->routeIndex?ai->route[ai->routeIndex-1]:ai->routeFrom;
        int to=ai->route[ai->routeIndex];
        // In mid-climb there is no nearby walkable floor. Keep the active
        // ladder link until the landing instead of erasing it on a replan.
        if(from>=0 && s_botNav[from].edge[BOT_NAV_DIRECTIONS]==to) return;
    }
    // Cache a route instead of recentering on the nearest cell every frame.
    // Searches and reverse paths share bounded buffers; commands allocate nothing.
    static short parent[BOT_NAV_MAX], queue[BOT_NAV_MAX], reverse[BOT_NAV_MAX];
    int start=Bot_NearestNode(ps->origin,1), head=0, tail=0, i, current, next, closest=-1, length=0;
    const vec_t *desired=ai->coverUntil>svs.time?ai->cover:
        ai->detourUntil>svs.time?NULL:ai->objectiveUntil>svs.time?
        (ai->objectiveBehavior==1?ai->guardPoint:ai->objective):ai->memoryUntil>svs.time?ai->lastSeen:NULL;
    float closestDistance=1e30f;
    ai->waypoint=-1; ai->routeCount=ai->routeIndex=0;
    if(start<0) { Bot_NavSeedPoint(ps->origin); return; }
    if(desired) Bot_NavScout(start,desired);
    else if(s_botNavBounded) {
        vec3_t center={(s_botNavMin[0]+s_botNavMax[0])*.5f,
                       (s_botNavMin[1]+s_botNavMax[1])*.5f,ps->origin[2]};
        Bot_NavScout(start,center);
    }
    if(!desired && ai->perchUntil>svs.time) return;
    if(!desired && Bot_IsSniper(ps) && ai->goal>=0 && ai->goal<s_botNavCount &&
       Bot_DistanceSq(ps->origin,ai->perchOrigin)>256*256 &&
       Bot_DistanceSq(ps->origin,s_botNav[ai->goal].origin)<64*64) {
        ai->perchUntil=svs.time+4000+(int)(Bot_Random(ai)%4000);
        memcpy(ai->perchOrigin,ps->origin,sizeof(vec3_t));
        return;
    }
    if(desired) ai->goal=Bot_NearestNodeForGoal(desired,0,
        ai->coverUntil<=svs.time && ai->objectiveUntil>svs.time && ai->objectiveBehavior!=1?ai->objectiveEntity:-1);
    else if(ai->goal<0 || ai->goal>=s_botNavCount ||
       Bot_DistanceSq(ps->origin,s_botNav[ai->goal].origin)<64*64)
        ai->goal=Bot_PatrolGoal(ai,ps,NULL,s_botNavCount);
    memset(parent,0xff,(size_t)s_botNavCount*sizeof(parent[0]));
    queue[tail++]=(short)start; parent[start]=(short)start;
    while(head<tail) {
        current=queue[head++];
        if(desired) {
            float distance=Bot_DistanceSq(s_botNav[current].origin,desired);
            if(distance<closestDistance) { closest=current; closestDistance=distance; }
        }
        if(current==ai->goal) break;
        // Different neighbour order avoids every teammate choosing the same lane.
        for(i=0;i<BOT_NAV_LINKS;++i) {
            next=s_botNav[current].edge[i==BOT_NAV_DIRECTIONS?i:(i+(ai->random&7))%BOT_NAV_DIRECTIONS];
            if(next>=0 && parent[next]<0 && !Bot_EdgeBlocked(ai,current,next)) {
                parent[next]=(short)current; queue[tail++]=(short)next;
            }
        }
    }
    if(ai->goal<0 || parent[ai->goal]<0)
        ai->goal=desired?closest:Bot_PatrolGoal(ai,ps,queue,tail);
    if(ai->goal<0) return;
    current=ai->goal;
    while(current!=start) { reverse[length++]=(short)current; current=parent[current]; }
    if(!length && desired && Bot_DistanceSq(ps->origin,desired)>96*96 && ++ai->stuckCount>=3) {
        // A disconnected objective can project onto the cell we're standing on.
        // Explore another reachable lane, then retry as navigation grows.
        ai->detourUntil=svs.time+6000; ai->goal=-1; ai->stuckCount=0;
    }
    // Enter the route directly when safe; don't walk backwards to its grid center.
    ai->routeFrom=start;
    if(length && !Bot_WalkEdge(ps->origin,s_botNav[reverse[length-1]].origin))
        ai->route[ai->routeCount++]=(short)start;
    while(length && ai->routeCount<BOT_ROUTE_MAX) ai->route[ai->routeCount++]=reverse[--length];
    if(ai->routeCount) ai->waypoint=ai->route[0];
}

static void Bot_AdvanceRoute(botAIState_t *ai, playerState_t *ps)
{
    while(ai->routeIndex<ai->routeCount &&
          Bot_DistanceSq(ps->origin,s_botNav[ai->route[ai->routeIndex]].origin)<18*18) {
        ai->routeFrom=ai->route[ai->routeIndex++];
        ai->stuckCount=0;
    }
    ai->waypoint=ai->routeIndex<ai->routeCount?ai->route[ai->routeIndex]:-1;
    if(ai->routeCount && ai->waypoint<0) { ai->routeCount=0; ai->nextPath=0; }
    // Smooth at most three short cells, checking floors and corners once per
    // decision tick. The movement tick then follows the cached waypoint.
    if(ai->waypoint>=0 && svs.time>=ai->nextThink) {
        int next=ai->routeIndex+2;
        if(next>=ai->routeCount) next=ai->routeCount-1;
        if(next>ai->routeIndex && Bot_WalkEdge(ps->origin,s_botNav[ai->route[next]].origin)) {
            ai->routeIndex=next;
            ai->waypoint=ai->route[next];
        }
    }
}

static void Bot_Think(int clientNum, botAIState_t *ai, playerState_t *ps)
{
    int target=Bot_AcquireTarget(clientNum,ai,ps);
    if(target!=ai->target) {
        if(target>=0 && (target!=ai->lastTarget || svs.time-ai->lastVisibleAt>600)) ai->acquiredAt=svs.time;
        ai->target=target; ai->nextPath=0; ai->burstUntil=0;
    }
    // Pursue only a position that passed visibility checks; never follow an
    // unseen opponent's live position through buildings or smoke.
    if (target >= 0) {
        ai->lastTarget=target; ai->lastVisibleAt=svs.time;
        memcpy(ai->lastSeen,SV_GameClientNum(target)->origin,sizeof(vec3_t));
        ai->memoryUntil=svs.time+(ai->difficulty==0?1600:ai->difficulty==1?2500:3500);
    }
    if(g_entities[clientNum].health<ai->lastHealth) {
        gclient_t *client=g_entities[clientNum].client;
        if(!client->damage_fromWorld && (client->damage_from[0] || client->damage_from[1])) {
            ai->alertYaw=atan2f(-client->damage_from[1],-client->damage_from[0])*BOT_RAD_TO_DEG;
            ai->alertUntil=svs.time+1800;
        }
    }
    ai->lastHealth=g_entities[clientNum].health;
    if(svs.time>=ai->nextError) {
        static const float error[3]={3.0f,1.2f,.4f};
        ai->yawError=((int)(Bot_Random(ai)%2001)-1000)*.001f*error[ai->difficulty];
        ai->pitchError=((int)(Bot_Random(ai)%2001)-1000)*.001f*error[ai->difficulty];
        ai->nextError=svs.time+400;
    }
    if(svs.time>=ai->nextPath) {
        Bot_FindPath(ai,ps);
        ai->nextPath=svs.time+1200;
    }
    if(svs.time-ai->lastMoveCheck>=900) {
        if(Bot_DistanceSq(ps->origin,ai->lastOrigin)<12*12 &&
           ai->wantsMove) {
            ai->avoidUntil=svs.time+400;
            ai->avoidSide=(Bot_Random(ai)&1)?100:-100;
            Bot_BlockRoute(ai);
            ai->coverUntil=0;
            if(++ai->stuckCount>=3) {
                // Leave a dead end before retrying the same strategic goal.
                ai->detourUntil=svs.time+6000; ai->goal=-1; ai->stuckCount=0;
            }
        }
        memcpy(ai->lastOrigin,ps->origin,sizeof(vec3_t));
        ai->lastMoveCheck=svs.time;
    }
}

static qboolean Bot_AtPoint(const vec_t *origin, const vec_t *point, float radius)
{
    float x=origin[0]-point[0], y=origin[1]-point[1];
    return x*x+y*y<=radius*radius && fabsf(origin[2]-point[2])<80;
}

static void Bot_FindCover(botAIState_t *ai, playerState_t *ps)
{
    int x,y,best=-1;
    float bestDistance=1e30f;
    vec3_t enemyEye;
    if(ai->target<0 || ai->difficulty==0 || svs.time<ai->nextCover ||
       (ai->objectiveUntil>svs.time && ai->objectiveBehavior>=2)) return;
    ai->nextCover=svs.time+1800;
    memcpy(enemyEye,ai->lastSeen,sizeof(vec3_t)); enemyEye[2]+=55;
    for(x=-4;x<=4;x+=2) for(y=-4;y<=4;y+=2) {
        vec3_t at={(floorf(ps->origin[0]/BOT_NAV_STEP+.5f)+x)*BOT_NAV_STEP,
                   (floorf(ps->origin[1]/BOT_NAV_STEP+.5f)+y)*BOT_NAV_STEP,ps->origin[2]};
        int node=Bot_NavFind(at);
        trace_t tr;
        float distance;
        if(node<0) continue;
        distance=Bot_DistanceSq(ps->origin,s_botNav[node].origin);
        if(distance<24*24 || distance>bestDistance ||
           !Bot_WalkEdge(ps->origin,s_botNav[node].origin)) continue;
        memcpy(at,s_botNav[node].origin,sizeof(vec3_t)); at[2]+=38;
        memset(&tr,0,sizeof(tr));
        G_TraceCapsule(&tr,enemyEye,s_botZero,s_botZero,at,1023,BOT_WORLD_MASK);
        if(tr.fraction<1 && !tr.startsolid) { best=node; bestDistance=distance; }
    }
    if(best>=0) {
        memcpy(ai->cover,s_botNav[best].origin,sizeof(vec3_t));
        ai->coverUntil=svs.time+2400; ai->nextPath=0;
    }
}

static qboolean Bot_FriendlyInAim(int clientNum, botAIState_t *ai, playerState_t *ps)
{
    vec3_t start={ps->origin[0],ps->origin[1],ps->origin[2]+ps->viewHeightCurrent},end;
    float yaw=ai->aim[1]/BOT_RAD_TO_DEG,pitch=ai->aim[0]/BOT_RAD_TO_DEG;
    float range=sqrtf(Bot_DistanceSq(ps->origin,ai->lastSeen))+64;
    trace_t tr;
    end[0]=start[0]+cosf(yaw)*cosf(pitch)*range;
    end[1]=start[1]+sinf(yaw)*cosf(pitch)*range;
    end[2]=start[2]-sinf(pitch)*range;
    memset(&tr,0,sizeof(tr));
    G_TraceCapsule(&tr,start,s_botZero,s_botZero,end,clientNum,0x02802831);
    return tr.entityNum<level.maxclients && g_entities[tr.entityNum].client &&
        tr.entityNum!=clientNum && !Bot_IsEnemy(clientNum,tr.entityNum);
}

static qboolean Bot_CanMove(int clientNum, const vec_t *origin, float yaw,
                           int forward, int right)
{
    vec3_t step={origin[0]+(cosf(yaw)*forward+sinf(yaw)*right)*.10f,
                 origin[1]+(sinf(yaw)*forward-cosf(yaw)*right)*.10f,origin[2]},ground;
    // Check the next movement step, not several stairs at once. A long look
    // ahead rejected ordinary stairs whose combined rise exceeded one step.
    return Bot_Ground(step,ground) && fabsf(ground[2]-origin[2])<=18.5f &&
        Bot_Clear(origin,ground,clientNum,0x02810011);
}

static void SV_BotRunAI(int clientNum, playerState_t *ps)
{
    botAIState_t *ai=&s_botAI[clientNum];
    cod2BotCommandState_t *cmd=&s_botCmdState[clientNum];
    gclient_t *client=g_entities[clientNum].client;
    float desiredYaw, desiredPitch=0, moveYaw, turnStep, distance=0;
    int oldButtons=cmd->buttons, elapsed, firing=0;
    qboolean sniper=Bot_IsSniper(ps);
    qboolean visible=0, objective=0, holding=0, using=0, ladder=0;
    int ladderFrom=-1;
    if(!ai->enabled || !client) return;
    cmd->buttons=0; cmd->forwardmove=cmd->rightmove=0; cmd->hasWeapon=0;
    if(client->sess.sessionState!=SESS_STATE_PLAYING || g_entities[clientNum].health<=0) {
        ai->target=-1; ai->acquiredAt=svs.time; ai->wantsMove=0;
        ai->memoryUntil=ai->coverUntil=ai->objectiveUntil=0;
        ai->burstUntil=ai->nextBurst=ai->alertUntil=ai->perchUntil=0;
        ai->routeCount=0; ai->waypoint=ai->goal=-1;
        ai->lastHealth=g_entities[clientNum].health;
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
                if (s_botAI[i].enabled) {
                    Com_Printf("[bots] state %d team=%d state=%d health=%d pos=%.0f,%.0f,%.0f target=%d goal=%d waypoint=%d buttons=%x pm=%d ground=%d velocity=%.0f,%.0f,%.0f\n",
                        i, g_entities[i].client->sess.cs.team, g_entities[i].client->sess.sessionState,
                        g_entities[i].health, SV_GameClientNum(i)->origin[0], SV_GameClientNum(i)->origin[1],
                        SV_GameClientNum(i)->origin[2], s_botAI[i].target, s_botAI[i].goal, s_botAI[i].waypoint,
                        i==clientNum?oldButtons:s_botCmdState[i].buttons,SV_GameClientNum(i)->pm_type,
                        SV_GameClientNum(i)->groundEntityNum,SV_GameClientNum(i)->velocity[0],
                        SV_GameClientNum(i)->velocity[1],SV_GameClientNum(i)->velocity[2]);
                    Com_Printf("[bots] route %d nodes=%d path=%d/%d stuck=%d detour=%d weapon=%s\n",
                        i,s_botNavCount,s_botAI[i].routeIndex,s_botAI[i].routeCount,s_botAI[i].stuckCount,
                        s_botAI[i].detourUntil>svs.time,SV_GameClientNum(i)->weapon>0 && BG_GetWeaponDef(SV_GameClientNum(i)->weapon)?
                        BG_GetWeaponDef(SV_GameClientNum(i)->weapon)->szInternalName:"none");
                    if(s_botAI[i].objectiveEntity>=0) {
                        gentity_t *entity=&g_entities[s_botAI[i].objectiveEntity];
                        Com_Printf("[bots] interaction client=%d behavior=%d entity=%d hint=%d bounds=%.0f,%.0f,%.0f/%.0f,%.0f,%.0f\n",
                            i,s_botAI[i].objectiveBehavior,s_botAI[i].objectiveEntity,SV_GameClientNum(i)->cursorHintEntIndex,
                            entity->r.absmin[0],entity->r.absmin[1],entity->r.absmin[2],
                            entity->r.absmax[0],entity->r.absmax[1],entity->r.absmax[2]);
                    }
                }
    }
    elapsed=svs.time-ai->lastFrame;
    if(elapsed<0 || elapsed>100) elapsed=50;
    ai->lastFrame=svs.time;
    objective=ai->objectiveUntil>svs.time;
    if(objective && ai->objectiveBehavior==1) Bot_GuardPoint(clientNum,ai,ps);
    Bot_AdvanceRoute(ai,ps);
    if(svs.time>=ai->nextThink) {
        Bot_Think(clientNum,ai,ps);
        ai->nextThink=svs.time+BOT_THINK_MS;
    }
    objective=ai->objectiveUntil>svs.time;
    holding=objective && Bot_AtPoint(ps->origin,ai->objectiveBehavior==1?ai->guardPoint:ai->objective,
        ai->objectiveBehavior==1?24:ai->objectiveRadius);
    if(holding && ai->objectiveBehavior==0 && ai->objectiveEntity>=0)
        holding=Bot_InObjective(ps->origin,ai->objectiveEntity);
    using=holding && ai->objectiveBehavior==2 && ai->objectiveEntity>=0 &&
        g_entities[ai->objectiveEntity].r.inuse &&
        g_entities[ai->objectiveEntity].classname==ai->objectiveClassname;
    if(ai->waypoint<0 && !holding && svs.time>=ai->perchUntil && svs.time>=ai->nextPath) {
        Bot_FindPath(ai,ps); ai->nextPath=svs.time+1200;
    }
    if(ai->perchUntil && svs.time>=ai->perchUntil) {
        ai->perchUntil=0; ai->goal=-1; ai->nextPath=0;
    }
    Bot_AdvanceRoute(ai,ps); // Also advance a route created by this tick's think.
    desiredYaw=ai->aim[1];
    if(ai->waypoint>=0) {
        const vec_t *waypoint=s_botNav[ai->waypoint].origin;
        moveYaw=atan2f(waypoint[1]-ps->origin[1],waypoint[0]-ps->origin[0])*BOT_RAD_TO_DEG;
        desiredYaw=moveYaw;
        distance=Bot_DistanceSq(ps->origin,waypoint);
    } else moveYaw=desiredYaw;
    // Finish the approach at the exact trigger, rather than stopping at the
    // nearest navigation cell outside a small flag/bomb volume.
    if(objective && ai->objectiveBehavior!=1 && !holding && ai->coverUntil<=svs.time && ai->detourUntil<=svs.time &&
       Bot_DistanceSq(ps->origin,ai->objective)<192*192) {
        vec3_t ground;
        if(Bot_Ground(ai->objective,ground) && Bot_WalkEdge(ps->origin,ground)) {
            moveYaw=atan2f(ground[1]-ps->origin[1],ground[0]-ps->origin[0])*BOT_RAD_TO_DEG;
            desiredYaw=moveYaw; distance=Bot_DistanceSq(ps->origin,ground);
        }
    }
    if(ai->waypoint>=0 && ai->routeIndex<ai->routeCount) {
        ladderFrom=ai->routeIndex>0?ai->route[ai->routeIndex-1]:ai->routeFrom;
        ladder=ladderFrom>=0 && s_botNav[ladderFrom].edge[BOT_NAV_DIRECTIONS]==ai->waypoint;
    }
    if(holding || (sniper && ai->perchUntil>svs.time && ai->memoryUntil<=svs.time)) {
        distance=0;
        // Pause between bounded scans instead of continuously spinning.
        if(svs.time>=ai->guardAt) {
            ai->guardAt=svs.time+1500+(int)(Bot_Random(ai)%1500);
            ai->guardYaw=ai->aim[1]+(int)(Bot_Random(ai)%141)-70;
        }
        desiredYaw=ai->guardYaw;
    } else ai->guardAt=0;
    if(ai->alertUntil>svs.time) desiredYaw=ai->alertYaw;
    if(ai->target>=0 && Bot_IsEnemy(clientNum,ai->target)) {
        playerState_t *enemy=SV_GameClientNum(ai->target);
        float lead=ai->difficulty==0?0:ai->difficulty==1?.025f:.035f;
        float x=enemy->origin[0]+enemy->velocity[0]*lead-ps->origin[0];
        float y=enemy->origin[1]+enemy->velocity[1]*lead-ps->origin[1];
        float z=enemy->origin[2]+enemy->viewHeightCurrent*.72f+enemy->velocity[2]*lead-
            (enemy->groundEntityNum==1023?400*lead*lead:0)-ps->origin[2]-ps->viewHeightCurrent;
        visible=Bot_CanSee(clientNum,ai->target);
        if(visible) {
            desiredYaw=atan2f(y,x)*BOT_RAD_TO_DEG+ai->yawError;
            desiredPitch=-atan2f(z,sqrtf(x*x+y*y))*BOT_RAD_TO_DEG+ai->pitchError;
            static const int reaction[3]={700,400,200};
            if(svs.time-ai->acquiredAt>=reaction[ai->difficulty] && !using &&
               fabsf(Bot_AngleDelta(desiredYaw,ai->aim[1]))<6 &&
               fabsf(Bot_AngleDelta(desiredPitch,ai->aim[0]))<6) firing=1;
            if(sniper || x*x+y*y>350*350) cmd->buttons|=0x1000;
            if(sniper && ps->fWeaponPosFrac<.9f) firing=0;
        }
    }
    if(visible) {
        WeaponDef *weapon=ps->weapon>0?BG_GetWeaponDef(ps->weapon):NULL;
        if(g_entities[clientNum].health<45 || (weapon && weapon->iClipIndex>=0 &&
           weapon->iClipIndex<128 && !ps->ammoclip[weapon->iClipIndex] && ps->weaponTime>500))
            Bot_FindCover(ai,ps);
    }
    if(ai->coverUntil>svs.time) {
        holding=Bot_AtPoint(ps->origin,ai->cover,20);
        if(holding) { distance=0; cmd->buttons|=0x200; }
    }
    if(using) {
        float x=ai->objective[0]-ps->origin[0],y=ai->objective[1]-ps->origin[1];
        desiredYaw=atan2f(y,x)*BOT_RAD_TO_DEG;
        desiredPitch=-atan2f(ai->objective[2]+12-ps->origin[2]-ps->viewHeightCurrent,sqrtf(x*x+y*y))*BOT_RAD_TO_DEG;
        firing=0; distance=0; cmd->buttons=0;
        // Activate only the selected objective, then keep holding through the
        // original 5s plant/10s defuse. Never claim progress or score directly.
        // The original bomb callback ignores a first press made in the air.
        // Wait to land before pressing, then preserve use while it links the
        // player to the trigger for the original progress timer.
        if((ps->cursorHintEntIndex==ai->objectiveEntity && ps->groundEntityNum!=1023) ||
           (oldButtons&8)) cmd->buttons|=8;
    }
    if(ladder && !using) {
        desiredYaw=s_botNav[ladderFrom].ladderYaw;
        desiredPitch=s_botNav[ai->waypoint].origin[2]>s_botNav[ladderFrom].origin[2]?-25:0;
        firing=0; cmd->buttons=0;
    }
    turnStep=(ai->difficulty==0?150.0f:ai->difficulty==1?220.0f:300.0f)*elapsed*.001f;
    ai->aim[0]=Bot_ApproachAngle(ai->aim[0],desiredPitch,turnStep);
    ai->aim[1]=Bot_ApproachAngle(ai->aim[1],desiredYaw,turnStep);
    ai->aim[2]=0;
    memcpy(cmd->angles,ai->aim,sizeof(vec3_t)); cmd->hasAngles=1;
    if(distance>10*10) {
        float delta=(moveYaw-ai->aim[1])/BOT_RAD_TO_DEG;
        int speed=visible?65:127;
        cmd->forwardmove=(signed char)(cosf(delta)*speed);
        cmd->rightmove=(signed char)(-sinf(delta)*speed);
        if(!visible && ai->goal>=0 && Bot_DistanceSq(ps->origin,s_botNav[ai->goal].origin)>240*240 &&
           cmd->forwardmove>110 && ai->avoidUntil<=svs.time) cmd->buttons|=BUTTON_SPRINT;
    }
    // Advance until the weapon has a useful engagement range. Brief dodges
    // have pauses; constant sideways movement made bots orbit and hit walls.
    if(visible && !ladder && !holding && !using && !(objective && ai->objectiveBehavior>=2) && ai->coverUntil<=svs.time) {
        WeaponDef *weapon=ps->weapon>0?BG_GetWeaponDef(ps->weapon):NULL;
        float range=sniper?900:weapon && weapon->weapClass==WEAPCLASS_SMG?240:
            weapon && weapon->weapClass==WEAPCLASS_MG?600:480;
        float enemyDistance=Bot_DistanceSq(ps->origin,ai->lastSeen);
        if(enemyDistance>=range*range && !objective &&
           Bot_Clear(ps->origin,ai->lastSeen,clientNum,BOT_WORLD_MASK)) {
            cmd->forwardmove=65; cmd->rightmove=0;
        }
        if(enemyDistance<range*range && !objective) {
            cmd->forwardmove=enemyDistance<96*96?-60:0;
            cmd->rightmove=0;
            if(!sniper && ai->difficulty>0) {
                if(svs.time>=ai->nextDodge) {
                    ai->avoidSide=(Bot_Random(ai)&1)?60:-60;
                    ai->dodgeUntil=svs.time+250;
                    ai->nextDodge=svs.time+1400+(int)(Bot_Random(ai)%600);
                }
                if(svs.time<ai->dodgeUntil) cmd->rightmove=ai->avoidSide;
            }
        }
        if(sniper && !cmd->forwardmove && !cmd->rightmove) {
            cmd->buttons|=0x200;
            if((svs.time-ai->acquiredAt)%2200<1000) cmd->buttons|=0x8000;
        }
        cmd->buttons&=~BUTTON_SPRINT;
    }
    if(ai->avoidUntil>svs.time && !ladder && !holding && !using) {
        cmd->forwardmove=-60; cmd->rightmove=ai->avoidSide; cmd->buttons&=~BUTTON_SPRINT;
    }
    if(ladder && !using) {
        // Use the same ladder-facing and movement commands as a human. Normal
        // pmove owns attachment, collision and climbing; no origin/velocity writes.
        qboolean up=s_botNav[ai->waypoint].origin[2]>s_botNav[ladderFrom].origin[2];
        float yaw=s_botNav[ladderFrom].ladderYaw/BOT_RAD_TO_DEG;
        float side=(s_botNav[ai->waypoint].origin[0]-ps->origin[0])*sinf(yaw)-
                   (s_botNav[ai->waypoint].origin[1]-ps->origin[1])*cosf(yaw);
        cmd->forwardmove=up?127:(ps->pm_flags&0x20)?-100:-80;
        cmd->rightmove=(signed char)fmaxf(-80,fminf(80,side*4));
        if(fabsf(Bot_AngleDelta(desiredYaw,ai->aim[1]))>15) cmd->forwardmove=cmd->rightmove=0;
        cmd->buttons&=~BUTTON_SPRINT;
        ai->wantsMove=cmd->forwardmove || cmd->rightmove;
    }
    if((cmd->forwardmove || cmd->rightmove) && !ladder) {
        float yaw=ai->aim[1]/BOT_RAD_TO_DEG;
        if(!Bot_CanMove(clientNum,ps->origin,yaw,cmd->forwardmove,cmd->rightmove)) {
            int forward=cmd->forwardmove, right=cmd->rightmove, side;
            cmd->forwardmove=cmd->rightmove=0; cmd->buttons&=~BUTTON_SPRINT;
            // Bounded local steering slides past a teammate or a corner; it
            // never ignores collision or changes another player's movement.
            for(side=0;side<4;++side) {
                float offset=(side<2?35:70)*((side&1)?-1:1);
                float delta=offset/BOT_RAD_TO_DEG;
                int f=(int)(forward*cosf(delta)-right*sinf(delta));
                int r=(int)(forward*sinf(delta)+right*cosf(delta));
                if(Bot_CanMove(clientNum,ps->origin,yaw,f,r)) {
                    cmd->forwardmove=(signed char)f; cmd->rightmove=(signed char)r; break;
                }
            }
            ai->wantsMove=1; // A blocked request must still recover.
        } else ai->wantsMove=1;
    } else if(!ladder) ai->wantsMove=0;
    if(!(cmd->buttons&0x200) && (ps->pm_flags&3)) cmd->buttons|=0x400;
    if(using) return;
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
            else if(firing && !Bot_FriendlyInAim(clientNum,ai,ps) &&
                    (!weapon->bSemiAuto || (!(oldButtons&1) && ps->weaponTime<=0)))
            {
                static const int burst[3]={250,450,650}, pause[3]={500,280,160};
                if(svs.time>=ai->nextBurst) {
                    ai->burstUntil=svs.time+burst[ai->difficulty];
                    ai->nextBurst=ai->burstUntil+pause[ai->difficulty];
                }
                if(svs.time<ai->burstUntil) cmd->buttons|=1;
            }
        }
    }
}
