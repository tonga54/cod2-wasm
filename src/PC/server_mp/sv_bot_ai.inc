/* First-party multiplayer AI. Included after the test-client command API.
 * No fields are added to client_t, gclient_t or the network playerstate ABI.
 * Navigation is a bounded, incremental flood of the actual collision world;
 * decisions are staggered, while normal usercmds/aim run at the server tick. */
#include <math.h>
#include "cod2_sprint.h"

extern int Sys_Milliseconds(void);
// Required for the native float-return ABI; an implicit int disables safe drops.
extern float Dvar_GetFloat(const char *dvarName);
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
#define BOT_PATH_NODES 2048
#define BOT_PATHS_PER_FRAME 2
#define BOT_PATH_TIME_MS 2
#define BOT_TACTICAL_TRACES_PER_FRAME 12
#define BOT_TRAFFIC_ROUTE_NODES 24
#define BOT_PERCH_MAX 96
#define BOT_RECENT_AREAS 8
#define BOT_NAV_DIRECTIONS 8
#define BOT_NAV_TRAVERSAL (BOT_NAV_DIRECTIONS+1)
#define BOT_NAV_LINKS (BOT_NAV_TRAVERSAL+BOT_NAV_DIRECTIONS)
#define BOT_ROUTE_MAX 64
#define BOT_BLOCKED_MAX 8
#define BOT_THINK_MS 200
#define BOT_WORLD_MASK 0x00810011
#define BOT_RAD_TO_DEG 57.2957795f

typedef struct {
    vec3_t origin;
    short edge[BOT_NAV_LINKS];
    unsigned char jumpMask, lookoutDir; // Direction reuses the existing alignment byte.
    float ladderYaw, priority, lookout;
} botNavNode_t;

typedef struct {
    qboolean enabled, wantsMove, routeMove;
    int difficulty, target, acquiredAt, nextThink, nextPath, nextError, spawnCount;
    int goal, waypoint, lastMoveCheck, avoidUntil, lastFrame;
    int objectiveUntil, objectiveBehavior, objectiveEntity, memoryUntil, coverUntil;
    unsigned int objectiveClassname;
    int nextCover, coverCursor, burstUntil, nextBurst, lastHealth, scopeReadyAt;
    int alertUntil;
    int guardAt, nextDodge, dodgeUntil, lastTarget, lastVisibleAt;
    int routeCount, routeIndex, routeFrom, blockedIndex, stuckCount, detourUntil;
    int perchUntil, guardUntil;
    int nextTraversalJump, failedPaths;
    int clientNum, planKind, progressNode, progressAt, recoveries;
    int recentAt, recentIndex, recentCount, nextApproach, approachUntil;
    int patrolGoal, patrolUntil;
    int yieldUntil, nextYield, yieldClient;
    qboolean yieldForHuman;
    float yieldYaw;
    int rejectedGoal, rejectedUntil, sniperRepositionAt;
    float progressDistance;
    vec3_t planPoint, recent[BOT_RECENT_AREAS], approach;
    int perceptionSlot, observedFrame, observedTarget;
    qboolean observedVisible, pathPending;
    vec3_t approachOrigin, approachGoal;
    float failedPathDistance;
    short route[BOT_ROUTE_MAX], blockedFrom[BOT_BLOCKED_MAX], blockedTo[BOT_BLOCKED_MAX];
    int blockedUntil[BOT_BLOCKED_MAX];
    unsigned int random;
    vec3_t aim, lastOrigin, objective, lastSeen, cover, guardPoint, guardAnchor, perchOrigin, routeDestination;
    float yawError, pitchError, objectiveRadius, alertYaw, guardYaw;
    signed char avoidSide;
} botAIState_t;

/* Opt-in timing is aggregated, never emitted per client or collision trace. */
static struct {
    int enabled, at, frames, aiMs, moveMs, gameMs, snapshotMs;
    int maxAiMs, maxGameMs, pathMs, planMs, paths, visited;
} s_botProfile;

static botAIState_t s_botAI[COD2_BOT_MAX_CLIENTS];
typedef struct { vec3_t origin; int until, reporter, responders[2], responseUntil[2]; } botContact_t;
static botContact_t s_botContacts[3];
static botNavNode_t s_botNav[BOT_NAV_MAX];
static unsigned short s_botNavHash[BOT_NAV_HASH];
static short s_botNavOpen[BOT_NAV_MAX];
static int s_botNavOpenCount, s_botNavWorkEdges;
static int s_botPlanCursor;
static int s_botNavCount, s_botNavNode, s_botNavDirection, s_botNavFrame, s_botDebugAt;
static int s_botNavSeedEntity;
static qboolean s_botNavSeeded, s_botNavBounded;
static int s_botWorkFrame=-1, s_botWorkEdges, s_botWorkNavMs, s_botWorkPaths, s_botWorkPathMs;
static int s_botWorkTacticalTraces;
static short s_botPerches[BOT_PERCH_MAX];
static int s_botPerchCount;
static int s_botMoveBlocker=-1;
static short s_botHumans[COD2_BOT_MAX_CLIENTS];
static int s_botHumanCount, s_botHumanScanAt;
static int s_botPathSearches, s_botPathExpanded, s_botPathDeferred;
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
    memset(s_botContacts,0,sizeof(s_botContacts));
    memset(s_botNavHash, 0, sizeof(s_botNavHash));
    s_botNavCount = s_botNavDirection = s_botNavOpenCount = s_botNavWorkEdges = 0;
    s_botPerchCount=0;
    memset(s_botPerches,-1,sizeof(s_botPerches));
    s_botNavNode = -1;
    s_botNavFrame = -1;
    s_botNavSeeded = s_botNavBounded = 0;
    s_botNavSeedEntity=0; s_botPlanCursor=0;
    s_botHumanCount=s_botHumanScanAt=0;
    s_botDebugAt = 0;
    s_botWorkFrame=-1;
    s_botPathSearches=s_botPathExpanded=s_botPathDeferred=0;
}

static void Bot_WorkFrame(void)
{
    if(s_botWorkFrame==svs.time) return;
    s_botWorkFrame=svs.time;
    s_botWorkEdges=s_botWorkNavMs=s_botWorkPaths=s_botWorkPathMs=0;
    s_botWorkTacticalTraces=0;
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

static qboolean Bot_GroundDepth(const vec_t *at, vec_t *ground, float depth)
{
    vec3_t start = {at[0], at[1], at[2]+18.5f};
    vec3_t end = {at[0], at[1], at[2]-depth};
    trace_t tr;
    memset(&tr, 0, sizeof(tr));
    G_TraceCapsule(&tr, start, s_botZero, s_botZero, end, 1023, BOT_WORLD_MASK);
    if (tr.startsolid || tr.allsolid || tr.fraction >= 1 || tr.normal[2] < .7f)
        return 0;
    ground[0] = at[0]; ground[1] = at[1];
    ground[2] = start[2] + (end[2]-start[2])*tr.fraction + 1;
    return 1;
}

static qboolean Bot_Ground(const vec_t *at, vec_t *ground)
{
    return Bot_GroundDepth(at,ground,96);
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
    vec3_t previous = {a[0], a[1], a[2]}, sample, floors[32];
    qboolean flat=1;
    int i, samples=(int)ceilf(sqrtf(Bot_DistanceSq(a,b))/8.0f);
    if(samples<4) samples=4;
    if(samples>32) return 0;
    // Stair-height steps, headroom and floor checks also reject cliffs/gaps.
    for (i = 1; i <= samples; ++i) {
        sample[0] = a[0]+(b[0]-a[0])*i/samples;
        sample[1] = a[1]+(b[1]-a[1])*i/samples;
        sample[2] = previous[2];
        if (!Bot_Ground(sample, floors[i-1]) || fabsf(floors[i-1][2]-previous[2]) > 18.5f) return 0;
        if(fabsf(floors[i-1][2]-a[2])>.001f) flat=0;
        memcpy(previous, floors[i-1], sizeof(previous));
    }
    if(fabsf(previous[2]-b[2])>=20) return 0;
    // The union of collinear flat capsule sweeps is exactly one sweep. Keep
    // every floor sample (holes still fail), but avoid 4..32 repeated body
    // traces per flat link. Stairs retain the original piecewise clearance.
    if(flat) return Bot_Clear(a,previous,1023,BOT_WORLD_MASK);
    memcpy(previous,a,sizeof(previous));
    for(i=0;i<samples;++i) {
        if(!Bot_Clear(previous,floors[i],1023,BOT_WORLD_MASK)) return 0;
        memcpy(previous,floors[i],sizeof(previous));
    }
    return 1;
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
    // Central lanes lead, but continuous central growth cannot starve an
    // older spawn/roof frontier indefinitely. Insertion order adds fairness.
    s_botNav[node].priority=s_botNavBounded?x*x+y*y+node*2048.0f:0;
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
    s_botNav[i].jumpMask=s_botNav[i].lookoutDir=0;
    s_botNav[i].lookout=-1;
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
    int x,y,started=Sys_Milliseconds();
    vec3_t point,ground;
    Bot_WorkFrame();
    for(x=-1;x<=1;++x) for(y=-1;y<=1;++y) {
        point[0]=(floorf(origin[0]/BOT_NAV_STEP+.5f)+x)*BOT_NAV_STEP;
        point[1]=(floorf(origin[1]/BOT_NAV_STEP+.5f)+y)*BOT_NAV_STEP;
        point[2]=origin[2];
        if(Bot_NavFind(point)>=0) continue;
        if(s_botWorkEdges>=BOT_NAV_EDGES_PER_FRAME ||
           s_botWorkNavMs+Sys_Milliseconds()-started>=BOT_NAV_TIME_MS) goto done;
        ++s_botWorkEdges;
        if(Bot_Ground(point,ground) && Bot_Clear(ground,ground,1023,BOT_WORLD_MASK)) Bot_NavAdd(ground);
    }
done:
    s_botWorkNavMs+=Sys_Milliseconds()-started;
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
    // Collision work for spawn seeds is incremental too; large maps must not
    // trace every spawn's nine cells before the first frame budget check.
    s_botNavSeedEntity=level.maxclients;
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

static int Bot_TraversalDirection(int from, int to)
{
    int direction;
    if(from<0 || to<0) return -1;
    for(direction=0;direction<BOT_NAV_DIRECTIONS;++direction)
        if(s_botNav[from].edge[BOT_NAV_TRAVERSAL+direction]==to) return direction;
    return -1;
}

static qboolean Bot_TraversalClear(const vec_t *a, const vec_t *b)
{
    const vec3_t maxs={15,15,70};
    trace_t tr;
    memset(&tr,0,sizeof(tr));
    // Departure/fall checks include the feet, unlike step-height walk checks.
    G_TraceCapsule(&tr,a,s_botMins,maxs,b,1023,BOT_WORLD_MASK);
    return !tr.startsolid && !tr.allsolid && tr.fraction>=1;
}

static int Bot_NavDrop(int from, int direction)
{
    botNavNode_t *node=&s_botNav[from];
    float limit=fminf(192,Dvar_GetFloat("bg_fallDamageMinHeight")-48);
    float jumpRise=fmaxf(0,fminf(36,Dvar_GetFloat("jump_height")-3));
    int offset;
    // Drops are directed: a walkable landing does not imply a way back up.
    // Reserve height for the original jump arc and reject damaging falls.
    if(limit<=18.5f) return -1;
    for(offset=32;offset<=96;offset+=32) {
        vec3_t departure={node->origin[0]+s_botDirections[direction][0]*offset,
                          node->origin[1]+s_botDirections[direction][1]*offset,node->origin[2]},ground;
        vec3_t raised={node->origin[0],node->origin[1],node->origin[2]+jumpRise};
        qboolean jump;
        int next;
        if(!Bot_GroundDepth(departure,ground,limit) ||
           node->origin[2]-ground[2]<=18.5f || node->origin[2]-ground[2]>limit ||
           !Bot_TraversalClear(ground,ground)) continue;
        jump=!Bot_TraversalClear(node->origin,departure);
        if(jump) {
            // A low parapet can use the normal jump button, provided the
            // standing capsule has headroom and a clear raised departure.
            departure[2]+=jumpRise;
            if(jumpRise<=0 || !Bot_TraversalClear(node->origin,raised) ||
               !Bot_TraversalClear(raised,departure)) continue;
        }
        if(!Bot_TraversalClear(departure,ground)) continue;
        next=Bot_NavAdd(ground);
        if(next<0) continue;
        node->edge[BOT_NAV_TRAVERSAL+direction]=(short)next;
        if(jump) node->jumpMask|=(unsigned char)(1u<<direction);
        return next;
    }
    return -1;
}

static int Bot_NavWalkLink(int from, int direction, const vec_t *candidate)
{
    vec3_t ground;
    int next,reverse=(direction+4)%BOT_NAV_DIRECTIONS;
    if(!Bot_Ground(candidate,ground) || !Bot_WalkEdge(s_botNav[from].origin,ground)) return -1;
    next=Bot_NavAdd(ground);
    if(next<0 || next==from) return -1;
    s_botNav[from].edge[direction]=(short)next;
    // A half-cell portal must not overwrite an established regular-grid lane.
    if(s_botNav[next].edge[reverse]<0) s_botNav[next].edge[reverse]=(short)from;
    return next;
}

static int Bot_NavLadderDown(int from, int direction)
{
    botNavNode_t *node=&s_botNav[from];
    int offset;
    if((direction&1) || node->edge[BOT_NAV_DIRECTIONS]>=0) return -1;
    // A bot can spawn above a ladder before the ground-side flood finds it.
    // Look from a grounded lower approach back toward its original material,
    // then reuse the normal ladder/landing validator. Never create a fall link.
    for(offset=32;offset<=96;offset+=32) {
        vec3_t at={node->origin[0]+s_botDirections[direction][0]*offset,
                   node->origin[1]+s_botDirections[direction][1]*offset,node->origin[2]},ground;
        vec3_t start,end;
        trace_t tr;
        int lower,upper;
        if(!Bot_GroundDepth(at,ground,384) || node->origin[2]-ground[2]<48 ||
           node->origin[2]-ground[2]>384) continue;
        memcpy(start,ground,sizeof(vec3_t));start[2]+=40;
        memcpy(end,start,sizeof(vec3_t));
        end[0]-=s_botDirections[direction][0]*32;
        end[1]-=s_botDirections[direction][1]*32;
        memset(&tr,0,sizeof(tr));
        G_TraceCapsule(&tr,start,s_botZero,s_botZero,end,1023,0x10000);
        if(tr.fraction>=1 || tr.startsolid || !(tr.surfaceFlags&8) || fabsf(tr.normal[2])>.3f) continue;
        lower=Bot_NavAdd(ground);
        if(lower<0) continue;
        Bot_NavLadder(lower,(direction+4)%BOT_NAV_DIRECTIONS);
        upper=s_botNav[lower].edge[BOT_NAV_DIRECTIONS];
        if(upper==from) return lower;
        if(upper>=0 && Bot_NavWalkLink(from,direction,s_botNav[upper].origin)>=0) return upper;
    }
    return -1;
}

static int Bot_NavEdge(int from, int direction)
{
    botNavNode_t *node=&s_botNav[from];
    int next,j;
    vec3_t candidate;
    if(node->edge[direction]!=-1) return node->edge[direction]>=0?
        node->edge[direction]:node->edge[BOT_NAV_TRAVERSAL+direction];
    // Rejoin the regular grid as soon as there is a legal doorway exit. Fine
    // cells are local portal/step repairs, never a second flood of the arena.
    for(j=0;j<2;++j) {
        int d=s_botDirections[direction][j];
        candidate[j]=(d>0?floorf(node->origin[j]/BOT_NAV_STEP)+1:
            d<0?ceilf(node->origin[j]/BOT_NAV_STEP)-1:
            floorf(node->origin[j]/BOT_NAV_STEP+.5f))*BOT_NAV_STEP;
    }
    candidate[2]=node->origin[2];
    next=Bot_NavWalkLink(from,direction,candidate);
    if(next>=0) return next;
    node->edge[direction]=-2;
    Bot_NavLadder(from,direction);
    next=Bot_NavLadderDown(from,direction);
    if(next>=0) return next;
    next=Bot_NavDrop(from,direction);
    if(next>=0) return next;
    // A 32-unit cell can miss a legal 30-unit doorway or a short stair. A
    // validated half step lets normal pmove reach it without teleporting,
    // shrinking player collision or turning the entire map into 16-unit cells.
    candidate[0]=node->origin[0]+s_botDirections[direction][0]*(BOT_NAV_STEP*.5f);
    candidate[1]=node->origin[1]+s_botDirections[direction][1]*(BOT_NAV_STEP*.5f);
    return Bot_NavWalkLink(from,direction,candidate);
}

static void Bot_NavScout(int start, const vec_t *desired)
{
    int current=start, step, started=Sys_Milliseconds(), visited[7];
    Bot_WorkFrame();
    if(s_botWorkEdges>=BOT_NAV_EDGES_PER_FRAME || s_botWorkNavMs>=BOT_NAV_TIME_MS) return;
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
            int dir=(direction+offsets[attempt]+8)%8,i,next;
            if(s_botNav[current].edge[dir]==-1) {
                if(s_botWorkEdges>=BOT_NAV_EDGES_PER_FRAME ||
                   s_botWorkNavMs+Sys_Milliseconds()-started>=BOT_NAV_TIME_MS) goto done;
                ++s_botWorkEdges;
            }
            next=Bot_NavEdge(current,dir);
            float distance;
            if(next>=0) {
                for(i=0;i<=step && visited[i]!=next;++i) {}
                distance=Bot_DistanceSq(s_botNav[next].origin,desired);
                if(i>step && distance<bestDistance) { best=next; bestDistance=distance; }
                if(best>=0 && distance<Bot_DistanceSq(s_botNav[current].origin,desired)) break;
            }
        }
        if(best<0) break;
        current=visited[step+1]=best;
    }
done:
    s_botWorkNavMs+=Sys_Milliseconds()-started;
}

// One representative per hashed spatial sector. Registration is O(1), uses
// existing traversal links, and never probes geometry or scans the graph.
static void Bot_RegisterPerch(int node)
{
    int i;
    botNavNode_t *nav=&s_botNav[node];
    if(s_botNavBounded && nav->origin[2]<s_botNavMin[2]+512+48) return;
    for(i=0;i<BOT_NAV_LINKS && nav->edge[i]<0;++i) {}
    if(i==BOT_NAV_LINKS) return;
    unsigned int hash=(unsigned int)(int)floorf(nav->origin[0]/256)*73856093u ^
        (unsigned int)(int)floorf(nav->origin[1]/256)*19349663u;
    int slot=(int)(hash%BOT_PERCH_MAX), previous=s_botPerches[slot];
    if(previous>=0 && s_botNav[previous].origin[2]>=nav->origin[2]) return;
    if(previous<0) ++s_botPerchCount;
    s_botPerches[slot]=(short)node;
}

static void Bot_ExpandFrontier(int node)
{
    int direction, started=Sys_Milliseconds();
    Bot_WorkFrame();
    // A disconnected spawn gets one local frontier, not another map flood.
    // These probes spend the same global navigation time and edge allowance.
    for(direction=0;direction<BOT_NAV_DIRECTIONS;++direction) {
        if(s_botNav[node].edge[direction]!=-1) continue;
        if(s_botWorkEdges>=BOT_NAV_EDGES_PER_FRAME ||
           s_botWorkNavMs+Sys_Milliseconds()-started>=BOT_NAV_TIME_MS) break;
        ++s_botWorkEdges; ++s_botNavWorkEdges;
        Bot_NavEdge(node,direction);
    }
    s_botWorkNavMs+=Sys_Milliseconds()-started;
    Bot_RegisterPerch(node);
}

static void Bot_NavFrame(void)
{
    int budget, started, previousMs;
    if (s_botNavFrame == svs.time) return;
    Bot_WorkFrame();
    s_botNavFrame = svs.time;
    if (!s_botNavSeeded) Bot_NavSeed();
    started=Sys_Milliseconds(); previousMs=s_botWorkNavMs;
    // Reserve part of the single frame budget for bots leaving isolated spawns.
    for (budget = 0; budget < BOT_NAV_EDGES_PER_FRAME*3/4; ++budget) {
        if(s_botWorkEdges>=BOT_NAV_EDGES_PER_FRAME ||
           previousMs+Sys_Milliseconds()-started>=BOT_NAV_TIME_MS-1) break;
        if(s_botNavSeedEntity>=level.maxclients && s_botNavSeedEntity<level.num_entities) {
            while(s_botNavSeedEntity<level.num_entities) {
                gentity_t *ent=&g_entities[s_botNavSeedEntity++];
                const char *name=ent->r.inuse && ent->classname?SL_ConvertToString(ent->classname):NULL;
                if(!name || strncmp(name,"mp_",3) || !strstr(name,"_spawn")) continue;
                s_botWorkNavMs=previousMs+Sys_Milliseconds()-started;
                Bot_NavSeedPoint(ent->r.currentOrigin);
                budget+=8;
                break;
            }
            continue;
        }
        if(s_botNavNode<0) s_botNavNode=Bot_NavPop();
        if(s_botNavNode<0) break;
        ++s_botNavWorkEdges;
        ++s_botWorkEdges;
        Bot_NavEdge(s_botNavNode,s_botNavDirection);
        if (++s_botNavDirection == BOT_NAV_DIRECTIONS) {
            Bot_RegisterPerch(s_botNavNode);
            s_botNavDirection = 0;
            s_botNavNode=-1;
            if (!s_botNavOpenCount)
                Com_Printf("[bots] navigation ready: %d nodes\n", s_botNavCount);
        }
        // Accelerate warm-up when traces are cheap, with a wall-time bound to
        // protect the multiplayer tick on the Pi and on slower hosts.
    }
    s_botWorkNavMs=previousMs+Sys_Milliseconds()-started;
}

static void Bot_ResetLife(botAIState_t *ai, playerState_t *ps)
{
    int clientNum=ai->clientNum, difficulty=ai->difficulty, recoveries=ai->recoveries;
    unsigned int random=ai->random;
    // ClientSpawn increments stats[5], including round spawns that occur
    // between AI ticks. Never carry a route, blocked doorway, cached sighting
    // or traversal cooldown from the previous body into the new spawn.
    memset(ai,0,sizeof(*ai));
    ai->enabled = 1; ai->difficulty = difficulty;
    ai->random=random; ai->recoveries=recoveries;
    ai->clientNum=clientNum; ai->progressNode=-1;
    ai->target = ai->lastTarget = ai->goal = ai->waypoint = ai->routeFrom = -1;
    ai->patrolGoal=ai->rejectedGoal=-1;
    ai->objectiveEntity = -1;
    ai->lastHealth = g_entities[clientNum].health;
    ai->observedFrame=-1; ai->observedTarget=-1;
    ai->nextThink = svs.time+(clientNum%4)*50;
    ai->lastFrame = ai->lastMoveCheck = svs.time;
    if (ps) {
        ai->spawnCount=ps->stats[5];
        memcpy(ai->aim, ps->viewangles, sizeof(vec3_t));
        memcpy(ai->lastOrigin, ps->origin, sizeof(vec3_t));
        memcpy(ai->perchOrigin, ps->origin, sizeof(vec3_t));
    }
}

qboolean SV_BotEnableAI(int clientNum, int difficulty)
{
    botAIState_t *ai;
    if (!SV_BotIsTestClient(clientNum) || difficulty < 0 || difficulty > 2) return 0;
    SV_BotResetAIClient(clientNum);
    ai=&s_botAI[clientNum];
    ai->clientNum=clientNum; ai->difficulty=difficulty;
    ai->random = 0x9e3779b9u ^ (unsigned int)(clientNum+1)*0x85ebca6bu ^ (unsigned int)svs.time;
    if (!ai->random) ai->random = 1;
    Bot_ResetLife(ai,SV_GameClientNum(clientNum));
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
       ai->objectiveEntity!=entityNum || Bot_DistanceSq(ai->objective,origin)>96*96) {
        ai->nextPath=0;
        ai->goal=-1; ai->routeCount=0; ai->waypoint=-1; ai->guardUntil=0; ai->failedPaths=0;
        ai->perchUntil=0; // A new tactical duty takes priority over patrol overwatch.
        ai->approachUntil=ai->nextApproach=0;
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
        s_botAI[clientNum].routeCount=0;
        s_botAI[clientNum].waypoint=-1;
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

static qboolean Bot_ObserveTarget(int clientNum, botAIState_t *ai, int target)
{
    if(ai->observedFrame!=svs.time || ai->observedTarget!=target) {
        ai->observedFrame=svs.time; ai->observedTarget=target;
        ai->observedVisible=Bot_CanSee(clientNum,target);
    }
    return ai->observedVisible;
}

static int Bot_AcquireTarget(int clientNum, botAIState_t *ai, playerState_t *ps)
{
    int nearest[8], i, j, k;
    float distances[8];
    for(i=0;i<8;++i) { nearest[i]=-1; distances[i]=6250000; }
    float yaw=ai->aim[1]/BOT_RAD_TO_DEG,viewX=cosf(yaw),viewY=sinf(yaw);
    static const float cone[3]={.57357644f,.42261826f,.25881905f};
    if (ai->target >= 0 && Bot_IsEnemy(clientNum, ai->target) && Bot_ObserveTarget(clientNum, ai, ai->target))
        return ai->target;
    for (i = 0; i < level.maxclients; ++i) {
        float distance;
        if (!Bot_IsEnemy(clientNum, i)) continue;
        distance = Bot_DistanceSq(ps->origin, SV_GameClientNum(i)->origin);
        // A new enemy must be in the bot's view cone. Close, visible footsteps
        // can alert it; a hidden opponent's live position never enters memory.
        if (distance > 96*96) {
            playerState_t *other=SV_GameClientNum(i);
            float x=other->origin[0]-ps->origin[0],y=other->origin[1]-ps->origin[1];
            float dot=x*viewX+y*viewY;
            if(dot<=0 || dot*dot<(x*x+y*y)*cone[ai->difficulty]*cone[ai->difficulty]) continue;
        }
        for (j = 0; j < 8; ++j) {
            if (distance >= distances[j]) continue;
            for (k=7; k>j; --k) { nearest[k]=nearest[k-1]; distances[k]=distances[k-1]; }
            nearest[j]=i; distances[j]=distance; break;
        }
    }
    // Keep three visibility queries: two near contacts plus a rotating
    // candidate. Nearby enemies behind cover cannot hide a visible fourth.
    int candidates=0;
    while(candidates<8 && nearest[candidates]>=0) ++candidates;
    for(i=0;i<3 && i<candidates;++i) {
        int slot=i<2?i:2+(ai->perceptionSlot++%(candidates-2));
        if(nearest[slot]>=0 && Bot_ObserveTarget(clientNum,ai,nearest[slot])) return nearest[slot];
    }
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
      unsigned int hash=((unsigned int)(int)floorf(gridX/BOT_NAV_STEP)*73856093u ^
                         (unsigned int)(int)floorf(gridY/BOT_NAV_STEP)*19349663u)&(BOT_NAV_HASH-1);
      while(s_botNavHash[hash]) {
        i=s_botNavHash[hash]-1;
        hash=(hash+1)&(BOT_NAV_HASH-1);
        if(s_botNav[i].origin[0]<gridX || s_botNav[i].origin[0]>=gridX+BOT_NAV_STEP ||
           s_botNav[i].origin[1]<gridY || s_botNav[i].origin[1]>=gridY+BOT_NAV_STEP) continue;
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

enum { BOT_ASSAULT, BOT_FLANK, BOT_SUPPORT, BOT_MARKSMAN };
static int Bot_Role(playerState_t *ps)
{
    WeaponDef *weapon=ps->weapon>0?BG_GetWeaponDef(ps->weapon):NULL;
    if(weapon && weapon->overlayReticle) return BOT_MARKSMAN;
    if(weapon && weapon->weapClass==WEAPCLASS_MG) return BOT_SUPPORT;
    return weapon && weapon->weapClass==WEAPCLASS_SMG?BOT_ASSAULT:BOT_FLANK;
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

static qboolean Bot_Teammate(int clientNum, int other)
{
    gclient_t *self=g_entities[clientNum].client, *peer=g_entities[other].client;
    return clientNum!=other && self && peer && self->sess.cs.team!=TEAM_FREE &&
        self->sess.cs.team==peer->sess.cs.team && peer->sess.connected==2 &&
        peer->sess.sessionState==SESS_STATE_PLAYING && g_entities[other].health>0;
}

static void Bot_StartDetour(botAIState_t *ai)
{
    if(ai->objectiveUntil>svs.time && ai->objectiveBehavior==1) {
        // A partial search ends before the requested guard slot. Reject the
        // actual slot rather than its innocent partial-route endpoint.
        ai->rejectedGoal=Bot_NavFind(ai->guardPoint); ai->rejectedUntil=svs.time+15000;
    }
    ai->detourUntil=svs.time+6000; ai->goal=-1; ai->stuckCount=0;
    // Retrying the same cached patrol/perch is not an escape from a dead end.
    ai->patrolUntil=ai->perchUntil=ai->guardUntil=0;
    ai->patrolGoal=-1;
}

// Static geometry only: crouched sightlines, low cover, and protected flanks.
// Eight rays once per node; every bot shares the existing per-frame ceiling.
static void Bot_EvaluatePerch(int node)
{
    botNavNode_t *nav=&s_botNav[node];
    int d, exposed=0;
    float longest=0, best=-1;
    if(nav->lookout>=0) return;
    Bot_WorkFrame();
    if(s_botWorkTacticalTraces+8>BOT_TACTICAL_TRACES_PER_FRAME) return;
    s_botWorkTacticalTraces+=8;
    for(d=0;d<4;++d) {
        vec3_t eye={nav->origin[0],nav->origin[1],nav->origin[2]+40};
        vec3_t end={eye[0]+s_botDirections[d*2][0]*1024,
                   eye[1]+s_botDirections[d*2][1]*1024,eye[2]};
        trace_t tr;
        memset(&tr,0,sizeof(tr));
        G_TraceCapsule(&tr,eye,s_botZero,s_botZero,end,1023,BOT_WORLD_MASK);
        float view=tr.startsolid?0:tr.fraction;
        if(view>.4f) ++exposed;
        if(view>longest) longest=view;
        eye[2]-=22; end[2]=eye[2];
        end[0]=eye[0]+s_botDirections[d*2][0]*128;
        end[1]=eye[1]+s_botDirections[d*2][1]*128;
        memset(&tr,0,sizeof(tr));
        G_TraceCapsule(&tr,eye,s_botZero,s_botZero,end,1023,BOT_WORLD_MASK);
        qboolean cover=!tr.startsolid && tr.fraction<1 && view>.4f;
        float lane=view*240+(cover?180:0);
        if(lane>best) {best=lane; nav->lookoutDir=(unsigned char)d;}
    }
    // A sheltered firing lane beats an equally high exposed rooftop; a
    // completely enclosed corner does not become a good sniper position.
    nav->lookout=longest>.4f?best+(4-exposed)*70:0;
}

static void Bot_RepositionSniper(botAIState_t *ai)
{
    if(svs.time<ai->sniperRepositionAt ||
       (ai->objectiveUntil>svs.time && ai->objectiveBehavior!=1)) return;
    ai->sniperRepositionAt=svs.time+5000;
    ai->rejectedGoal=ai->objectiveUntil>svs.time?Bot_NavFind(ai->guardPoint):
        ai->patrolGoal>=0?ai->patrolGoal:ai->goal;
    ai->rejectedUntil=svs.time+15000;
    ai->perchUntil=ai->patrolUntil=ai->guardUntil=0;
    ai->patrolGoal=ai->goal=ai->waypoint=-1;
    ai->routeCount=ai->routeIndex=0; ai->nextPath=0;
}

static int Bot_PatrolGoal(botAIState_t *ai, playerState_t *ps, const short *nodes, int count)
{
    int i,j,best=-1, candidates[3]={-1,-1,-1};
    float scores[3]={-1e30f,-1e30f,-1e30f};
    qboolean sniper=Bot_IsSniper(ps);
    if(!count) return -1;
    for(i=0;i<(sniper && !nodes && s_botPerchCount?48:32);++i) {
        int node=i>=32?s_botPerches[Bot_Random(ai)%BOT_PERCH_MAX]:nodes?nodes[Bot_Random(ai)%(unsigned int)count]:(int)(Bot_Random(ai)%(unsigned int)count);
        if(node<0) continue;
        float distance=Bot_DistanceSq(ps->origin,s_botNav[node].origin);
        float score=fminf(sqrtf(distance),1600)*.35f+(Bot_Random(ai)%128);
        if(distance<256*256 || distance>2500*2500) score-=1000;
        if(distance<24*24) score-=10000;
        if(node==ai->goal) score-=500;
        if(ai->rejectedUntil>svs.time && ai->rejectedGoal>=0 &&
           Bot_DistanceSq(s_botNav[ai->rejectedGoal].origin,s_botNav[node].origin)<192*192) score-=2000;
        for(j=0;j<ai->recentCount;++j)
            if(Bot_DistanceSq(ai->recent[j],s_botNav[node].origin)<384*384) score-=180;
        // Reserve destinations, not player positions. Teammates search separate
        // areas without learning anything about an unseen enemy.
        for(j=0;j<level.maxclients;++j) {
            botAIState_t *other=&s_botAI[j];
            if(!other->enabled || other->goal<0 || other->goal>=s_botNavCount ||
               !Bot_Teammate(ai->clientNum,j)) continue;
            if(Bot_DistanceSq(s_botNav[other->goal].origin,s_botNav[node].origin)<384*384) { score-=400; break; }
        }
        if(s_botNavBounded) {
            vec3_t center={(s_botNavMin[0]+s_botNavMax[0])*.5f,
                           (s_botNavMin[1]+s_botNavMax[1])*.5f,s_botNav[node].origin[2]};
            score-=sqrtf(Bot_DistanceSq(center,s_botNav[node].origin))*.08f;
        }
        if(sniper) score+=fmaxf(-128,fminf(256,s_botNav[node].origin[2]-ps->origin[2]))*2;
        for(j=0;j<3;++j) if(score>scores[j]) {
            int k;
            for(k=2;k>j;--k) { candidates[k]=candidates[k-1]; scores[k]=scores[k-1]; }
            candidates[j]=node; scores[j]=score; break;
        }
    }
    float bestScore=-1e30f;
    // Evaluate only the three strongest candidates, caching static sightlines.
    for(i=0;i<3 && candidates[i]>=0;++i) {
        int node=candidates[i];
        float score=scores[i];
        if(sniper) {
            Bot_EvaluatePerch(node);
            score+=fmaxf(0,s_botNav[node].lookout);
        }
        if(score>bestScore) { best=node; bestScore=score; }
    }
    return best;
}

// Shared fixed storage: one bounded A* search at a time, no graph-wide clear
// and no heap allocation. Generation stamps also cover nodes added by scouting.
static unsigned int s_botSearchGeneration, s_botSearchStamp[BOT_NAV_MAX];
static short s_botSearchParent[BOT_NAV_MAX], s_botSearchHeap[BOT_NAV_MAX];
static int s_botSearchPosition[BOT_NAV_MAX], s_botSearchCount;
static float s_botSearchCost[BOT_NAV_MAX], s_botSearchScore[BOT_NAV_MAX];
static unsigned int s_botTrafficStamp[BOT_NAV_MAX], s_botTrafficGeneration;
static unsigned char s_botTraffic[BOT_NAV_MAX];

static void Bot_RouteTraffic(botAIState_t *ai)
{
    int i,j;
    if(++s_botTrafficGeneration==0) {
        memset(s_botTrafficStamp,0,sizeof(s_botTrafficStamp)); s_botTrafficGeneration=1;
    }
    if(g_entities[ai->clientNum].client->sess.cs.team==TEAM_FREE) return;
    // Sparse reservations, built once per admitted search. No client scan in
    // A*'s inner loop, no collision queries and no graph-wide clearing.
    for(i=0;i<level.maxclients;++i) {
        botAIState_t *other=&s_botAI[i];
        if(!other->enabled || !Bot_Teammate(ai->clientNum,i)) continue;
        for(j=other->routeIndex;j<other->routeCount && j<other->routeIndex+BOT_TRAFFIC_ROUTE_NODES;++j) {
            int node=other->route[j];
            if(node<0 || node>=s_botNavCount) continue;
            if(s_botTrafficStamp[node]!=s_botTrafficGeneration) {
                s_botTrafficStamp[node]=s_botTrafficGeneration; s_botTraffic[node]=0;
            }
            if(s_botTraffic[node]<3) ++s_botTraffic[node];
        }
    }
}

static void Bot_SearchPush(int node)
{
    int at=s_botSearchPosition[node];
    if(at<0) at=s_botSearchCount++;
    while(at>0) {
        int parent=(at-1)/2, other=s_botSearchHeap[parent];
        if(s_botSearchScore[other]<=s_botSearchScore[node]) break;
        s_botSearchHeap[at]=(short)other; s_botSearchPosition[other]=at; at=parent;
    }
    s_botSearchHeap[at]=(short)node; s_botSearchPosition[node]=at;
}

static int Bot_SearchPop(void)
{
    int result=s_botSearchHeap[0], last=s_botSearchHeap[--s_botSearchCount], at=0;
    while(at*2+1<s_botSearchCount) {
        int child=at*2+1;
        if(child+1<s_botSearchCount && s_botSearchScore[s_botSearchHeap[child+1]]<
           s_botSearchScore[s_botSearchHeap[child]]) ++child;
        if(s_botSearchScore[last]<=s_botSearchScore[s_botSearchHeap[child]]) break;
        s_botSearchHeap[at]=s_botSearchHeap[child];
        s_botSearchPosition[s_botSearchHeap[at]]=at; at=child;
    }
    if(s_botSearchCount) { s_botSearchHeap[at]=(short)last; s_botSearchPosition[last]=at; }
    s_botSearchPosition[result]=-2;
    return result;
}

static void Bot_GuardPoint(int clientNum, botAIState_t *ai, playerState_t *ps)
{
    int i,j,best=-1, reserved=0, candidates[2]={-1,-1};
    const vec_t *reservations[COD2_BOT_MAX_CLIENTS];
    float scores[2]={1e30f,1e30f};
    float bestScore=1e30f;
    qboolean sniper=Bot_IsSniper(ps);
    if(ai->guardUntil>svs.time && Bot_DistanceSq(ai->guardAnchor,ai->objective)<96*96) return;
    Bot_WorkFrame();
    qboolean evaluate=s_botWorkTacticalTraces+6<=BOT_TACTICAL_TRACES_PER_FRAME;
    // Keep the current assignment when the shared tactical budget is busy.
    if(!evaluate && ai->guardUntil && Bot_DistanceSq(ai->guardAnchor,ai->objective)<96*96) return;
    float angle=(clientNum*.618034f-floorf(clientNum*.618034f))*6.2831853f;
    float radius=fminf(160,ai->objectiveRadius*.8f);
    vec3_t desired={ai->objective[0]+cosf(angle)*radius,ai->objective[1]+sinf(angle)*radius,ai->objective[2]};
    // Filter the roster once, outside the candidate loop.
    for(j=0;j<level.maxclients;++j) {
        botAIState_t *other=&s_botAI[j];
        if(!Bot_Teammate(clientNum,j)) continue;
        if(!SV_BotIsTestClient(j)) reservations[reserved++]=SV_GameClientNum(j)->origin;
        else if(other->enabled && other->objectiveUntil>svs.time && other->objectiveBehavior==1 &&
           other->guardUntil) reservations[reserved++]=other->guardPoint;
    }
    for(i=0;i<(sniper?32+BOT_PERCH_MAX:32);++i) {
        float a=angle+(i%16)*6.2831853f/16;
        float r=radius*(i<16?1:.5f);
        vec3_t at={(floorf((ai->objective[0]+cosf(a)*r)/BOT_NAV_STEP+.5f))*BOT_NAV_STEP,
                   (floorf((ai->objective[1]+sinf(a)*r)/BOT_NAV_STEP+.5f))*BOT_NAV_STEP,ai->objective[2]};
        int node=i<32?Bot_NavFind(at):s_botPerches[i-32];
        float score;
        if(node<0) continue;
        if(i>=32 && Bot_DistanceSq(ai->objective,s_botNav[node].origin)>768*768) continue;
        score=sqrtf(Bot_DistanceSq(desired,s_botNav[node].origin))*.4f+
            sqrtf(Bot_DistanceSq(ps->origin,s_botNav[node].origin))*.08f;
        if(sniper) {
            score-=fmaxf(0,fminf(256,s_botNav[node].origin[2]-ai->objective[2]))*1.5f;
            score-=fmaxf(0,s_botNav[node].lookout)*.25f;
        }
        if(ai->rejectedUntil>svs.time && ai->rejectedGoal>=0 &&
           Bot_DistanceSq(s_botNav[ai->rejectedGoal].origin,s_botNav[node].origin)<192*192) score+=2000;
        // Guards reserve actual positions, including around adjacent objectives.
        for(j=0;j<reserved;++j) {
            float separation=Bot_DistanceSq(reservations[j],s_botNav[node].origin);
            if(separation<96*96) score+=240*(1-separation/(96*96));
        }
        if(node==candidates[0] || node==candidates[1]) continue;
        for(j=0;j<2;++j) if(score<scores[j]) {
            if(!j) { candidates[1]=candidates[0]; scores[1]=scores[0]; }
            candidates[j]=node; scores[j]=score; break;
        }
    }
    for(i=0;i<2 && candidates[i]>=0;++i) {
        int node=candidates[i];
        float score=scores[i];
        if(evaluate) {
            vec3_t eye={s_botNav[node].origin[0],s_botNav[node].origin[1],s_botNav[node].origin[2]+(sniper?40:55)};
            vec3_t anchor={ai->objective[0],ai->objective[1],ai->objective[2]+55}, end;
            float facing=atan2f(eye[1]-anchor[1],eye[0]-anchor[0]);
            if(sniper) facing+=3.14159265f;
            trace_t tr;
            s_botWorkTacticalTraces+=3;
            memset(&tr,0,sizeof(tr));
            G_TraceCapsule(&tr,eye,s_botZero,s_botZero,anchor,1023,BOT_WORLD_MASK);
            if(tr.startsolid || tr.fraction<1) score+=sniper?1000:300; // Must watch the objective.
            end[0]=eye[0]+cosf(facing)*512; end[1]=eye[1]+sinf(facing)*512; end[2]=eye[2];
            memset(&tr,0,sizeof(tr));
            G_TraceCapsule(&tr,eye,s_botZero,s_botZero,end,1023,BOT_WORLD_MASK);
            float view=tr.startsolid?0:tr.fraction;
            score-=view*100; // Prefer a usable outward firing lane over a wall.
            eye[2]-=sniper?22:35; end[2]-=sniper?22:35;
            memset(&tr,0,sizeof(tr));
            G_TraceCapsule(&tr,eye,s_botZero,s_botZero,end,1023,BOT_WORLD_MASK);
            if(!tr.startsolid && tr.fraction<.3f && view>.6f) score-=60; // Low cover.
        }
        if(score<bestScore) { best=node; bestScore=score; }
    }
    if(best<0) best=Bot_NearestNode(desired,0);
    const vec_t *point=best>=0?s_botNav[best].origin:ai->objective;
    if(!ai->guardUntil || Bot_DistanceSq(point,ai->guardPoint)>24*24) ai->nextPath=0;
    memcpy(ai->guardPoint,best>=0?s_botNav[best].origin:ai->objective,sizeof(vec3_t));
    memcpy(ai->guardAnchor,ai->objective,sizeof(vec3_t));
    ai->guardUntil=svs.time+(evaluate?(sniper?8000:3000)+(clientNum%5)*50:50);
}

static void Bot_BuildPath(botAIState_t *ai, playerState_t *ps)
{
    if(ai->routeIndex<ai->routeCount) {
        int from=ai->routeIndex?ai->route[ai->routeIndex-1]:ai->routeFrom;
        int to=ai->route[ai->routeIndex];
        // In mid-climb there is no nearby walkable floor. Keep the active
        // ladder link until the landing instead of erasing it on a replan.
        if(from>=0 && (s_botNav[from].edge[BOT_NAV_DIRECTIONS]==to ||
           (ps->groundEntityNum==1023 && Bot_TraversalDirection(from,to)>=0))) return;
    }
    int kind=ai->coverUntil>svs.time?1:ai->detourUntil>svs.time?0:
        ai->objectiveUntil>svs.time?2:ai->memoryUntil>svs.time && !Bot_IsSniper(ps)?3:0;
    const vec_t *desired=kind==1?ai->cover:kind==2?
        (ai->objectiveBehavior==1?ai->guardPoint:ai->objective):kind==3?ai->lastSeen:NULL;
    // Holding an assigned firing position needs neither nearest-node lookup
    // nor an A* admission. Reactions and objective changes still run normally.
    if((!desired && ai->perchUntil>svs.time) ||
       (kind==2 && ai->objectiveBehavior==1 && Bot_DistanceSq(ps->origin,desired)<24*24)) {
        ai->waypoint=-1; ai->routeCount=ai->routeIndex=0;
        return;
    }
    // A good route is a commitment. Recomputing it while moving changed the
    // neighbor order, reversed corners and made local avoidance into circles.
    if(ai->routeIndex<ai->routeCount && ai->planKind==kind &&
       (!desired || Bot_DistanceSq(ai->planPoint,desired)<96*96) &&
       Bot_DistanceSq(ps->origin,s_botNav[ai->route[ai->routeIndex]].origin)<256*256) return;
    Bot_WorkFrame();
    if(s_botWorkPaths>=BOT_PATHS_PER_FRAME || s_botWorkPathMs>=BOT_PATH_TIME_MS) {
        ++s_botPathDeferred;
        ai->nextPath=svs.time+50+(ai->clientNum%4)*50;
        return; // Keep the existing route until this bot can plan again.
    }
    ++s_botWorkPaths; ++s_botPathSearches;
    int started=Sys_Milliseconds();
    static short reverse[BOT_NAV_MAX], reached[BOT_PATH_NODES];
    int start=Bot_NearestNode(ps->origin,1), expanded=0, i, current, next, closest=-1, length=0, frontier=-1;
    vec3_t projected, flank;
    float closestDistance=1e30f;
    float frontierScore=1e30f;
    qboolean found=0;
    if(kind==2 && ai->objectiveBehavior!=1 && ai->objectiveEntity<0 &&
       Bot_GroundDepth(desired,projected,192) && fabsf(projected[2]-desired[2])<80) desired=projected;
    // A radio above a ledge/opening may trace to the floor below. That floor
    // is outside Bot_AtPoint's vertical tolerance and is not a valid arrival.
    ai->waypoint=-1; ai->routeCount=ai->routeIndex=0;
    ai->planKind=kind;
    if(desired) memcpy(ai->planPoint,desired,sizeof(vec3_t));
    if(kind==3 && ai->difficulty>0 && Bot_Role(ps)==BOT_FLANK &&
       Bot_DistanceSq(ps->origin,desired)>256*256) {
        float yaw=atan2f(desired[1]-ps->origin[1],desired[0]-ps->origin[0]);
        float side=(ai->clientNum&2)?192:-192;
        vec3_t at={desired[0]-sinf(yaw)*side,desired[1]+cosf(yaw)*side,desired[2]};
        if(Bot_GroundDepth(at,flank,96)) desired=flank;
    }
    if(start<0) { Bot_NavSeedPoint(ps->origin); goto done; }
    if(!desired && Bot_IsSniper(ps) && ai->patrolGoal>=0 &&
       Bot_DistanceSq(ps->origin,s_botNav[ai->patrolGoal].origin)<64*64)
        Bot_EvaluatePerch(ai->patrolGoal);
    if(!desired && Bot_IsSniper(ps) && ai->goal>=0 && ai->goal==ai->patrolGoal &&
       s_botNav[ai->goal].lookout>0 &&
       Bot_DistanceSq(ps->origin,s_botNav[ai->goal].origin)<24*24) {
        ai->perchUntil=svs.time+10000+(int)(Bot_Random(ai)%8000);
        ai->guardAt=0;
        memcpy(ai->perchOrigin,ps->origin,sizeof(vec3_t));
        goto done;
    }
    if(!desired && (ai->patrolGoal<0 || ai->patrolGoal>=s_botNavCount ||
       ai->patrolUntil<=svs.time || Bot_DistanceSq(ps->origin,s_botNav[ai->patrolGoal].origin)<64*64)) {
        ai->patrolGoal=Bot_PatrolGoal(ai,ps,NULL,s_botNavCount);
        ai->patrolUntil=svs.time+20000;
    }
    vec3_t center={(s_botNavMin[0]+s_botNavMax[0])*.5f,
                   (s_botNavMin[1]+s_botNavMax[1])*.5f,ps->origin[2]};
    const vec_t *scout=desired?desired:ai->patrolGoal>=0?s_botNav[ai->patrolGoal].origin:
        s_botNavBounded?center:ps->origin;
    Bot_NavScout(start,scout);
    if(desired) ai->goal=Bot_NearestNodeForGoal(desired,0,kind==2 &&
        ai->objectiveBehavior!=1?ai->objectiveEntity:-1);
    else ai->goal=ai->patrolGoal;
    const vec_t *destination=desired?desired:ai->goal>=0?s_botNav[ai->goal].origin:ps->origin;
    Bot_RouteTraffic(ai);
    if(++s_botSearchGeneration==0) {
        memset(s_botSearchStamp,0,sizeof(s_botSearchStamp)); s_botSearchGeneration=1;
    }
    s_botSearchCount=0;
    s_botSearchStamp[start]=s_botSearchGeneration;
    s_botSearchParent[start]=(short)start;
    s_botSearchCost[start]=0;
    s_botSearchScore[start]=sqrtf(Bot_DistanceSq(ps->origin,destination));
    s_botSearchPosition[start]=-1; Bot_SearchPush(start);
    while(s_botSearchCount && expanded<BOT_PATH_NODES) {
        current=Bot_SearchPop(); reached[expanded++]=(short)current;
        if(s_botProfile.enabled) ++s_botProfile.visited;
        float distance=Bot_DistanceSq(s_botNav[current].origin,destination);
        if(distance<closestDistance) { closest=current; closestDistance=distance; }
        if(current==ai->goal) { found=1; break; }
        for(i=0;i<BOT_NAV_LINKS;++i) {
            // Stable ordering across replans; aiming's RNG cannot change lanes.
            next=s_botNav[current].edge[i>=BOT_NAV_DIRECTIONS?i:(i+ai->clientNum*3)%BOT_NAV_DIRECTIONS];
            if(next==-1 && i<BOT_NAV_DIRECTIONS) {
                float score=sqrtf(distance)+s_botSearchCost[current]*.3f;
                // When escaping an elevated component, keep discovering its
                // descending side even if it initially leads away from the
                // objective. Horizontal proximity alone revisits the roof.
                if(s_botNav[start].origin[2]>destination[2]+48)
                    score+=fmaxf(0,s_botNav[current].origin[2]-destination[2])*4;
                else if(s_botNav[start].origin[2]<destination[2]-48)
                    score+=fmaxf(0,destination[2]-s_botNav[current].origin[2])*4;
                if(score<frontierScore) {frontier=current; frontierScore=score;}
            }
            if(next<0 || Bot_EdgeBlocked(ai,current,next)) continue;
            float cost=s_botSearchCost[current]+sqrtf(Bot_DistanceSq(s_botNav[current].origin,s_botNav[next].origin));
            if(s_botTrafficStamp[next]==s_botTrafficGeneration) cost+=s_botTraffic[next]*32;
            if(i>=BOT_NAV_DIRECTIONS) cost+=48; // Prefer walking over special traversals.
            if(s_botSearchStamp[next]==s_botSearchGeneration && cost>=s_botSearchCost[next]) continue;
            if(s_botSearchStamp[next]!=s_botSearchGeneration) s_botSearchPosition[next]=-1;
            s_botSearchStamp[next]=s_botSearchGeneration;
            s_botSearchParent[next]=(short)current; s_botSearchCost[next]=cost;
            s_botSearchScore[next]=cost+sqrtf(Bot_DistanceSq(s_botNav[next].origin,destination));
            Bot_SearchPush(next);
        }
        if((expanded&31)==0 && s_botWorkPathMs+Sys_Milliseconds()-started>=BOT_PATH_TIME_MS) break;
    }
    s_botPathExpanded+=expanded;
    if(!found) {
        // The entire known component was searched, but still has unexplored
        // exits. Walk to a reachable frontier instead of repeatedly choosing
        // the roof edge nearest a ground-level goal, then randomizing patrols.
        // A truncated search below an upper-floor objective must not keep
        // returning to the same XY point. Discover a reachable exit when the
        // partial route made no meaningful progress toward that floor.
        qboolean below=desired && destination[2]>ps->origin[2]+80 &&
            closestDistance>=Bot_DistanceSq(s_botNav[start].origin,destination)-32*32;
        qboolean exploring=frontier>=0 && (!s_botSearchCount || below);
        if(exploring) {
            Bot_ExpandFrontier(frontier);
            ai->goal=frontier; ai->failedPaths=ai->stuckCount=0;
            ai->nextPath=svs.time+200;
        }
        // Keep the strategic patrol destination across partial routes. Picking
        // another random cell from a tiny spawn component created local loops.
        else ai->goal=closest;
        if(!exploring && !desired && (closest==start || ai->failedPaths>=3)) {
            ai->rejectedGoal=ai->patrolGoal; ai->rejectedUntil=svs.time+15000;
            ai->goal=Bot_PatrolGoal(ai,ps,reached,expanded);
            ai->patrolUntil=0;
        }
        if(!exploring && desired && Bot_DistanceSq(ps->origin,desired)>96*96) {
            float remaining=sqrtf(Bot_DistanceSq(ps->origin,desired));
            if(!ai->failedPaths || remaining<ai->failedPathDistance-24) {
                ai->failedPathDistance=remaining; ai->failedPaths=1;
            } else ++ai->failedPaths;
            if(ai->failedPaths>=4) {
                Bot_StartDetour(ai); ai->failedPaths=0;
                ai->goal=Bot_PatrolGoal(ai,ps,reached,expanded);
            }
        }
    } else ai->failedPaths=0;
    if(ai->goal<0) goto done;
    current=ai->goal;
    while(current!=start && length<BOT_NAV_MAX) {
        reverse[length++]=(short)current; current=s_botSearchParent[current];
    }
    if(!length && desired && Bot_DistanceSq(ps->origin,desired)>96*96 && ++ai->stuckCount>=3) {
        Bot_StartDetour(ai);
    }
    ai->routeFrom=start;
    if(length && !Bot_WalkEdge(ps->origin,s_botNav[reverse[length-1]].origin))
        ai->route[ai->routeCount++]=(short)start;
    while(length && ai->routeCount<BOT_ROUTE_MAX) ai->route[ai->routeCount++]=reverse[--length];
    if(ai->routeCount) ai->waypoint=ai->route[0];
    ai->progressNode=-1;
done:
    s_botWorkPathMs+=Sys_Milliseconds()-started;
}

static void Bot_FindPath(botAIState_t *ai, playerState_t *ps)
{
    int started=s_botProfile.enabled?Sys_Milliseconds():0;
    Bot_BuildPath(ai,ps);
    if(s_botProfile.enabled) {
        ++s_botProfile.paths;
        s_botProfile.pathMs+=Sys_Milliseconds()-started;
    }
}

static void Bot_RequestPath(botAIState_t *ai)
{
    ai->pathPending=1;
}

static void SV_BotPlanFrame(void)
{
    int checked=0, plans=0,i;
    for(i=0;i<level.maxclients && !s_botAI[i].enabled;++i) {}
    if(i==level.maxclients) return;
    // Give pending routes the first share of the existing navigation budget.
    // Background discovery can exhaust it with a single expensive roof probe
    // on slower hosts, leaving an isolated bot waiting for the whole map flood.
    Bot_WorkFrame();
    if(!s_botNavSeeded) Bot_NavSeed();
    while(checked++<level.maxclients && plans<BOT_PATHS_PER_FRAME) {
        int index=s_botPlanCursor;
        s_botPlanCursor=(s_botPlanCursor+1)%level.maxclients;
        botAIState_t *ai=&s_botAI[index];
        if(!ai->enabled || !ai->pathPending) continue;
        ai->pathPending=0;
        if(!g_entities[index].client || g_entities[index].health<=0 ||
           g_entities[index].client->sess.sessionState!=SESS_STATE_PLAYING ||
           ai->spawnCount!=SV_GameClientNum(index)->stats[5]) continue;
        ai->nextPath=svs.time+1200;
        Bot_FindPath(ai,SV_GameClientNum(index));
        if(s_botWorkPathMs>=BOT_PATH_TIME_MS) break;
        ++plans;
    }
    Bot_NavFrame();
}

static void Bot_AdvanceRoute(botAIState_t *ai, playerState_t *ps)
{
    while(ai->routeIndex<ai->routeCount) {
        int from=ai->routeIndex?ai->route[ai->routeIndex-1]:ai->routeFrom;
        int to=ai->route[ai->routeIndex];
        float radius=from>=0 && Bot_DistanceSq(s_botNav[from].origin,s_botNav[to].origin)<24*24?8:18;
        const vec_t *at=s_botNav[to].origin;
        float dx=ps->origin[0]-at[0],dy=ps->origin[1]-at[1];
        // Pmove can attach to a ladder while the graph is still discovering
        // its endpoints. A rebuilt route then starts at its lower approach,
        // which cannot be reached while hanging above it. Resume the verified
        // ladder link when already attached between its two landings.
        if((ps->pm_flags&0x20) && ai->routeIndex+1<ai->routeCount &&
           dx*dx+dy*dy<48*48) {
            int landing=ai->route[ai->routeIndex+1];
            float endZ=s_botNav[landing].origin[2];
            if(s_botNav[to].edge[BOT_NAV_DIRECTIONS]==landing &&
               ps->origin[2]>=fminf(at[2],endZ)-20 &&
               ps->origin[2]<=fmaxf(at[2],endZ)+20) {
                ai->routeFrom=ai->route[ai->routeIndex++];
                ai->stuckCount=0;
                continue;
            }
        }
        // Ground samples within step height may share one node. Arrival on a
        // walking link must tolerate that vertical difference at a stair lip;
        // ladders/drop links retain their full 3D landing checks.
        qboolean traversal=from>=0 && Bot_TraversalDirection(from,to)>=0;
        if(traversal?Bot_DistanceSq(ps->origin,at)>=radius*radius:
           dx*dx+dy*dy>=radius*radius || fabsf(ps->origin[2]-at[2])>20) break;
        ai->routeFrom=ai->route[ai->routeIndex++];
        ai->stuckCount=0;
    }
    ai->waypoint=ai->routeIndex<ai->routeCount?ai->route[ai->routeIndex]:-1;
    if(ai->routeCount && ai->waypoint<0) {
        ai->routeCount=0; ai->nextPath=0;
    }
    // Smooth at most three short cells, checking floors and corners once per
    // decision tick. The movement tick then follows the cached waypoint.
    if(ai->waypoint>=0 && svs.time>=ai->nextThink) {
        int next=ai->routeIndex+2;
        if(next>=ai->routeCount) next=ai->routeCount-1;
        if(next>ai->routeIndex) {
            const vec_t *a=s_botNav[ai->route[ai->routeIndex]].origin;
            const vec_t *b=s_botNav[ai->route[next]].origin;
            int i;
            float dx=b[0]-a[0],dy=b[1]-a[1];
            qboolean straight=fabsf(ps->origin[2]-a[2])<8 &&
                fabsf((ps->origin[0]-a[0])*dy-(ps->origin[1]-a[1])*dx)<=sqrtf(dx*dx+dy*dy)*4;
            // Collinear flat links already contain sampled floor/cliff checks.
            // Reuse those checks; only the shortcut capsule needs retracing.
            for(i=ai->routeIndex;i<=next && straight;++i) {
                const vec_t *at=s_botNav[ai->route[i]].origin;
                int from=i?ai->route[i-1]:ai->routeFrom;
                if(from>=0 && (s_botNav[from].edge[BOT_NAV_DIRECTIONS]==ai->route[i] ||
                   Bot_TraversalDirection(from,ai->route[i])>=0)) straight=0;
                if(fabsf(at[2]-a[2])>2 || fabsf((at[0]-a[0])*(b[1]-a[1])-
                   (at[1]-a[1])*(b[0]-a[0]))>.1f) straight=0;
            }
            if(straight && Bot_Clear(ps->origin,b,1023,BOT_WORLD_MASK)) {
                ai->routeIndex=next;
                ai->waypoint=ai->route[next];
            }
        }
    }
}

static qboolean Bot_ClaimContact(botAIState_t *ai, botContact_t *contact)
{
    int i,available=-1;
    if(contact->until<=svs.time) return 0;
    for(i=0;i<2;++i) {
        int responder=contact->responders[i];
        if(contact->responseUntil[i]>svs.time && responder==ai->clientNum) return 1;
        if(contact->responseUntil[i]<=svs.time || !g_entities[responder].client ||
           g_entities[responder].health<=0 || s_botAI[responder].objectiveUntil>svs.time ||
           !Bot_Teammate(contact->reporter,responder)) available=i;
    }
    if(available<0) return 0;
    contact->responders[available]=ai->clientNum;
    contact->responseUntil[available]=contact->until;
    return 1;
}

static void Bot_HumanCourtesy(botAIState_t *ai, playerState_t *ps);

static void Bot_Think(int clientNum, botAIState_t *ai, playerState_t *ps)
{
    Bot_HumanCourtesy(ai,ps);
    int target=Bot_AcquireTarget(clientNum,ai,ps);
    if(target!=ai->target) {
        if(target>=0 && (target!=ai->lastTarget || svs.time-ai->lastVisibleAt>600)) ai->acquiredAt=svs.time;
        ai->target=target; ai->nextPath=0; ai->burstUntil=ai->scopeReadyAt=0;
    }
    // Pursue only a position that passed visibility checks; never follow an
    // unseen opponent's live position through buildings or smoke.
    if (target >= 0) {
        ai->lastTarget=target; ai->lastVisibleAt=svs.time;
        memcpy(ai->lastSeen,SV_GameClientNum(target)->origin,sizeof(vec3_t));
        ai->memoryUntil=svs.time+(ai->difficulty==0?1600:ai->difficulty==1?2500:3500);
        int team=g_entities[clientNum].client->sess.cs.team;
        if(team==TEAM_ALLIES || team==TEAM_AXIS) {
            if(s_botContacts[team].until<=svs.time ||
               Bot_DistanceSq(s_botContacts[team].origin,ai->lastSeen)>256*256)
                memset(s_botContacts[team].responseUntil,0,sizeof(s_botContacts[team].responseUntil));
            memcpy(s_botContacts[team].origin,ai->lastSeen,sizeof(vec3_t));
            s_botContacts[team].until=svs.time+1600; s_botContacts[team].reporter=clientNum;
        }
    } else if(ai->memoryUntil>svs.time && Bot_DistanceSq(ps->origin,ai->lastSeen)<64*64) {
        ai->memoryUntil=0; ai->goal=-1; ai->routeCount=0; ai->waypoint=-1;
    }
    if(target<0 && ai->memoryUntil<=svs.time && ai->objectiveUntil<=svs.time &&
       ai->difficulty>0 && Bot_Role(ps)!=BOT_MARKSMAN) {
        int team=g_entities[clientNum].client->sess.cs.team;
        if(team==TEAM_ALLIES || team==TEAM_AXIS) {
            botContact_t *contact=&s_botContacts[team];
            if(contact->until>svs.time && contact->reporter!=clientNum &&
               Bot_Teammate(clientNum,contact->reporter) &&
               Bot_DistanceSq(ps->origin,SV_GameClientNum(contact->reporter)->origin)<768*768 &&
               Bot_DistanceSq(ps->origin,contact->origin)>128*128 && Bot_ClaimContact(ai,contact)) {
                memcpy(ai->lastSeen,contact->origin,sizeof(vec3_t));
                ai->memoryUntil=contact->until; ai->nextPath=0;
            }
        }
    }
    if(svs.time>=ai->recentAt) {
        memcpy(ai->recent[ai->recentIndex++%BOT_RECENT_AREAS],ps->origin,sizeof(vec3_t));
        if(ai->recentCount<BOT_RECENT_AREAS) ++ai->recentCount;
        ai->recentAt=svs.time+2500;
    }
    // Moving is not necessarily progressing: a bot can orbit a corner while
    // passing the old displacement-only stuck test. Watch the route itself.
    if(ai->wantsMove && (target<0 || ai->routeMove) && ai->waypoint>=0) {
        float remaining=sqrtf(Bot_DistanceSq(ps->origin,s_botNav[ai->waypoint].origin));
        if(ai->progressNode!=ai->waypoint || remaining<ai->progressDistance-8) {
            ai->progressNode=ai->waypoint; ai->progressDistance=remaining; ai->progressAt=svs.time;
        } else if(svs.time-ai->progressAt>=2400) {
            Bot_BlockRoute(ai); ++ai->recoveries;
            ai->progressNode=-1; ai->coverUntil=0;
            if(++ai->stuckCount>=2) {
                Bot_StartDetour(ai);
            }
        }
    } else ai->progressNode=-1;
    if(g_entities[clientNum].health<ai->lastHealth) {
        gclient_t *client=g_entities[clientNum].client;
        if(Bot_IsSniper(ps)) Bot_RepositionSniper(ai);
        if(!client->damage_fromWorld && (client->damage_from[0] || client->damage_from[1])) {
            ai->alertYaw=atan2f(-client->damage_from[1],-client->damage_from[0])*BOT_RAD_TO_DEG;
            ai->alertUntil=svs.time+1800;
        }
    }
    if(Bot_IsSniper(ps) && target>=0 &&
       Bot_DistanceSq(ps->origin,ai->lastSeen)<224*224) Bot_RepositionSniper(ai);
    ai->lastHealth=g_entities[clientNum].health;
    if(svs.time>=ai->nextError) {
        static const float error[3]={3.0f,2.6f,.4f};
        ai->yawError=((int)(Bot_Random(ai)%2001)-1000)*.001f*error[ai->difficulty];
        ai->pitchError=((int)(Bot_Random(ai)%2001)-1000)*.001f*error[ai->difficulty];
        ai->nextError=svs.time+(ai->difficulty==2?400:600);
    }
    if(svs.time>=ai->nextPath) {
        Bot_RequestPath(ai);
    }
    if(svs.time-ai->lastMoveCheck>=900) {
        if(Bot_DistanceSq(ps->origin,ai->lastOrigin)<12*12 &&
           ai->wantsMove) {
            ai->avoidUntil=svs.time+400;
            ai->avoidSide=(Bot_Random(ai)&1)?100:-100;
            Bot_BlockRoute(ai);
            ++ai->recoveries;
            ai->coverUntil=0;
            if(++ai->stuckCount>=3) {
                // Leave a dead end before retrying the same strategic goal.
                Bot_StartDetour(ai);
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
    int scanned,checked=0;
    vec3_t enemyEye;
    if(ai->target<0 || ai->difficulty==0 || svs.time<ai->nextCover ||
       (ai->objectiveUntil>svs.time && ai->objectiveBehavior>=2)) return;
    Bot_WorkFrame();
    // Budget deferral retries sooner than a bot that already spent its share;
    // otherwise fixed client order can starve the final clients indefinitely.
    ai->nextCover=svs.time+50;
    memcpy(enemyEye,ai->lastSeen,sizeof(vec3_t)); enemyEye[2]+=55;
    // Continue a tiny local scan over decision ticks. Test occlusion before
    // floor/clearance work; both spend existing shared budgets, not one full
    // 25-position scan for every wounded bot on the same frame.
    for(scanned=0;scanned<25 && checked<2;++scanned) {
        int cell=ai->coverCursor, x=(cell%5)*2-4, y=(cell/5)*2-4;
        vec3_t at={(floorf(ps->origin[0]/BOT_NAV_STEP+.5f)+x)*BOT_NAV_STEP,
                   (floorf(ps->origin[1]/BOT_NAV_STEP+.5f)+y)*BOT_NAV_STEP,ps->origin[2]};
        int node=Bot_NavFind(at);
        trace_t tr;
        int started,j;
        qboolean reserved=0,reachable;
        if(s_botWorkTacticalTraces>=BOT_TACTICAL_TRACES_PER_FRAME ||
           s_botWorkEdges>=BOT_NAV_EDGES_PER_FRAME || s_botWorkNavMs>=BOT_NAV_TIME_MS) return;
        ai->coverCursor=(cell+1)%25;
        if(node<0) continue;
        if(Bot_DistanceSq(ps->origin,s_botNav[node].origin)<24*24) continue;
        ++checked;
        ai->nextCover=svs.time+200+(ai->clientNum%4)*50;
        for(j=0;j<level.maxclients;++j) {
            if(s_botAI[j].coverUntil>svs.time && Bot_Teammate(ai->clientNum,j) &&
               Bot_DistanceSq(s_botAI[j].cover,s_botNav[node].origin)<48*48) {reserved=1;break;}
        }
        if(reserved) continue;
        started=Sys_Milliseconds();
        memcpy(at,s_botNav[node].origin,sizeof(vec3_t)); at[2]+=38;
        memset(&tr,0,sizeof(tr));
        ++s_botWorkTacticalTraces;
        G_TraceCapsule(&tr,enemyEye,s_botZero,s_botZero,at,1023,BOT_WORLD_MASK);
        reachable=0;
        if(tr.fraction<1 && !tr.startsolid && !tr.allsolid &&
           s_botWorkNavMs+Sys_Milliseconds()-started<BOT_NAV_TIME_MS) {
            ++s_botWorkEdges; ++s_botNavWorkEdges;
            reachable=Bot_WalkEdge(ps->origin,s_botNav[node].origin);
        }
        s_botWorkNavMs+=Sys_Milliseconds()-started;
        if(reachable) {
            memcpy(ai->cover,s_botNav[node].origin,sizeof(vec3_t));
            ai->coverUntil=svs.time+2400; ai->nextPath=0; ai->perchUntil=0;
            ai->nextCover=svs.time+1800;
            return;
        }
    }
    if(!checked) ai->nextCover=svs.time+600;
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
    s_botMoveBlocker=-1;
    vec3_t step={origin[0]+(cosf(yaw)*forward+sinf(yaw)*right)*.10f,
                 origin[1]+(sinf(yaw)*forward-cosf(yaw)*right)*.10f,origin[2]},ground;
    // Check the next movement step, not several stairs at once. A long look
    // ahead rejected ordinary stairs whose combined rise exceeded one step.
    if(!Bot_Ground(step,ground) || fabsf(ground[2]-origin[2])>18.5f) return 0;
    vec3_t start={origin[0],origin[1],origin[2]+18};
    vec3_t end={ground[0],ground[1],ground[2]+18};
    trace_t tr;
    memset(&tr,0,sizeof(tr));
    G_TraceCapsule(&tr,start,s_botMins,s_botMaxs,end,clientNum,0x02810011);
    // Match PM_playerTrace's original escape from an overlapping player body.
    // Reject ordinary player obstructions, but don't veto every direction at
    // a crowded spawn before native pmove can resolve the existing overlap.
    if(tr.startsolid && (tr.contents&0x02000000))
        G_TraceCapsule(&tr,start,s_botMins,s_botMaxs,end,clientNum,0x00810011);
    if(tr.fraction<1 && tr.entityNum<level.maxclients && tr.entityNum!=clientNum &&
       g_entities[tr.entityNum].client) s_botMoveBlocker=tr.entityNum;
    return !tr.startsolid && !tr.allsolid && tr.fraction>=1;
}

static void Bot_BeginHumanYield(botAIState_t *ai, playerState_t *ps, int human)
{
    playerState_t *peer=SV_GameClientNum(human);
    float speed=peer->velocity[0]*peer->velocity[0]+peer->velocity[1]*peer->velocity[1];
    float yaw=speed>40*40?atan2f(peer->velocity[1],peer->velocity[0]):peer->viewangles[1]/BOT_RAD_TO_DEG;
    float side=-(ps->origin[0]-peer->origin[0])*sinf(yaw)+(ps->origin[1]-peer->origin[1])*cosf(yaw);
    if(fabsf(side)<1) side=(ai->clientNum&1)?1:-1;
    ai->yieldYaw=yaw+(side>0?1:-1)*1.57079633f;
    ai->yieldForHuman=1;ai->yieldClient=human;
    ai->yieldUntil=svs.time+700;ai->nextYield=svs.time+1700;
    if(ai->objectiveUntil>svs.time && ai->objectiveBehavior==1) ai->guardUntil=0;
}

static void Bot_HumanCourtesy(botAIState_t *ai, playerState_t *ps)
{
    int i,best=-1;
    float nearest=80*80;
    if(svs.time<ai->nextYield || ps->groundEntityNum==1023 ||
       (ai->objectiveUntil>svs.time && ai->objectiveBehavior!=1)) return;
    // The common bots-only server pays one roster refresh per half second.
    // Paused test clients are not mistaken for human players.
    if(svs.time>=s_botHumanScanAt) {
        s_botHumanScanAt=svs.time+500;s_botHumanCount=0;
        for(i=0;i<level.maxclients;++i)
            if(g_entities[i].client && g_entities[i].client->sess.connected==2 && !SV_BotIsTestClient(i))
                s_botHumans[s_botHumanCount++]=(short)i;
    }
    for(i=0;i<s_botHumanCount;++i) {
        int human=s_botHumans[i];
        if(human>=level.maxclients || !Bot_Teammate(ai->clientNum,human) || SV_BotIsTestClient(human)) continue;
        playerState_t *peer=SV_GameClientNum(human);
        float x=ps->origin[0]-peer->origin[0],y=ps->origin[1]-peer->origin[1];
        float distance=x*x+y*y, vx=peer->velocity[0],vy=peer->velocity[1],speed=vx*vx+vy*vy;
        if(distance>=nearest || fabsf(ps->origin[2]-peer->origin[2])>40) continue;
        if(speed>40*40) {
            float across=x*vy-y*vx;
            if(x*vx+y*vy<=0 || across*across>32*32*speed) continue;
        } else {
            // Also release an already-blocked player looking into the bot.
            float yaw=peer->viewangles[1]/BOT_RAD_TO_DEG, facing=x*cosf(yaw)+y*sinf(yaw);
            if(distance>36*36 || facing<=0 || facing*facing<distance*.5f) continue;
        }
        nearest=distance;best=human;
    }
    if(best<0) return;
    Bot_WorkFrame();
    if(s_botWorkTacticalTraces>=BOT_TACTICAL_TRACES_PER_FRAME) return;
    ++s_botWorkTacticalTraces;
    if(Bot_Clear(SV_GameClientNum(best)->origin,ps->origin,best,BOT_WORLD_MASK))
        Bot_BeginHumanYield(ai,ps,best);
}

static void Bot_YieldToBlocker(botAIState_t *ai, int blocker)
{
    int priority,otherPriority;
    if(blocker<0 || !Bot_Teammate(ai->clientNum,blocker) || svs.time<ai->nextYield) return;
    botAIState_t *other=&s_botAI[blocker];
    priority=ai->objectiveUntil>svs.time && ai->objectiveBehavior>=2;
    otherPriority=other->objectiveUntil>svs.time && other->objectiveBehavior>=2;
    // A stable right of way breaks symmetric doorway/stair deadlocks. Human
    // players and objective users/carriers go first; ties use the client id.
    if(other->enabled && (priority>otherPriority ||
       (priority==otherPriority && ai->clientNum<blocker))) return;
    if(!SV_BotIsTestClient(blocker)) {
        Bot_BeginHumanYield(ai,SV_GameClientNum(ai->clientNum),blocker);
        return;
    }
    ai->yieldForHuman=0;ai->yieldClient=blocker; ai->yieldUntil=svs.time+700; ai->nextYield=svs.time+1700;
}

static void SV_BotRunAI(int clientNum, playerState_t *ps)
{
    botAIState_t *ai=&s_botAI[clientNum];
    cod2BotCommandState_t *cmd=&s_botCmdState[clientNum];
    gclient_t *client=g_entities[clientNum].client;
    float desiredYaw, desiredPitch=0, moveYaw, turnStep, distance=0;
    int oldButtons=cmd->buttons, elapsed, firing=0;
    qboolean sniper=Bot_IsSniper(ps);
    qboolean visible=0, objective=0, holding=0, using=0, ladder=0, traversing=0;
    int ladderFrom=-1, traversalDirection=-1;
    if(!ai->enabled || !client) return;
    cmd->buttons=0; cmd->forwardmove=cmd->rightmove=0; cmd->hasWeapon=0;
    if(client->sess.sessionState!=SESS_STATE_PLAYING || g_entities[clientNum].health<=0) {
        // Also reset after an observed inactive phase; do not depend only
        // on spawn-counter continuity when restoring spectator state.
        ai->spawnCount=-1;
        ai->target=-1; ai->acquiredAt=svs.time; ai->wantsMove=ai->routeMove=0;
        ai->memoryUntil=ai->coverUntil=ai->objectiveUntil=0;
        ai->burstUntil=ai->nextBurst=ai->alertUntil=ai->perchUntil=0;
        ai->scopeReadyAt=0;
        ai->pathPending=0; ai->routeCount=0; ai->waypoint=ai->goal=-1;
        ai->detourUntil=ai->guardUntil=ai->patrolUntil=ai->avoidUntil=ai->yieldUntil=ai->nextYield=0;
        ai->stuckCount=ai->failedPaths=0; ai->patrolGoal=ai->progressNode=-1;
        ai->rejectedGoal=-1; ai->rejectedUntil=ai->sniperRepositionAt=0;
        ai->lastHealth=g_entities[clientNum].health;
        // CTF/HQ wait for use in spectator state after their respawn timer.
        // Only original callbacks can admit the player: this also respects
        // HQ defender lockout and S&D's prohibition on mid-round respawns.
        if(client->sess.sessionState==SESS_STATE_DEAD ||
           (client->sess.sessionState==SESS_STATE_SPECTATOR &&
            client->sess.cs.team!=TEAM_SPECTATOR)) cmd->buttons=0x8;
        memcpy(ai->aim,ps->viewangles,sizeof(vec3_t));
        ai->lastFrame=svs.time;
        return;
    }
    if(ai->spawnCount!=ps->stats[5]) {
        Bot_ResetLife(ai,ps);
        if(Dvar_GetInt("scr_bot_debug"))
            Com_Printf("[bots] respawn client=%d spawn=%d\n",clientNum,ai->spawnCount);
    }
    Bot_NavFrame();
    if (svs.time >= s_botDebugAt) {
        int i;
        s_botDebugAt = svs.time + 5000;
        if (Dvar_GetInt("scr_bot_debug")) {
            Com_Printf("[bots] work time=%d searches=%d expanded=%d deferred=%d navEdges=%d\n",
                svs.time,s_botPathSearches,s_botPathExpanded,s_botPathDeferred,s_botNavWorkEdges);
            for (i = 0; i < level.maxclients; ++i)
                if (s_botAI[i].enabled) {
                    Com_Printf("[bots] state %d team=%d state=%d health=%d pos=%.0f,%.0f,%.0f target=%d goal=%d waypoint=%d buttons=%x pm=%d ground=%d velocity=%.0f,%.0f,%.0f\n",
                        i, g_entities[i].client->sess.cs.team, g_entities[i].client->sess.sessionState,
                        g_entities[i].health, SV_GameClientNum(i)->origin[0], SV_GameClientNum(i)->origin[1],
                        SV_GameClientNum(i)->origin[2], s_botAI[i].target, s_botAI[i].goal, s_botAI[i].waypoint,
                        i==clientNum?oldButtons:s_botCmdState[i].buttons,SV_GameClientNum(i)->pm_type,
                        SV_GameClientNum(i)->groundEntityNum,SV_GameClientNum(i)->velocity[0],
                        SV_GameClientNum(i)->velocity[1],SV_GameClientNum(i)->velocity[2]);
                    Com_Printf("[bots] route %d nodes=%d path=%d/%d stuck=%d detour=%d recoveries=%d weapon=%s\n",
                        i,s_botNavCount,s_botAI[i].routeIndex,s_botAI[i].routeCount,s_botAI[i].stuckCount,
                        s_botAI[i].detourUntil>svs.time,s_botAI[i].recoveries,SV_GameClientNum(i)->weapon>0 && BG_GetWeaponDef(SV_GameClientNum(i)->weapon)?
                        BG_GetWeaponDef(SV_GameClientNum(i)->weapon)->szInternalName:"none");
                    if(Bot_IsSniper(SV_GameClientNum(i))) {
                        botAIState_t *watch=&s_botAI[i];
                        qboolean guard=watch->objectiveUntil>svs.time && watch->objectiveBehavior==1;
                        Com_Printf("[bots] sniper %d catalogue=%d perched=%d guarding=%d relativeHeight=%.0f\n",
                            i,s_botPerchCount,watch->perchUntil>svs.time,
                            guard && Bot_DistanceSq(SV_GameClientNum(i)->origin,watch->guardPoint)<24*24,
                            SV_GameClientNum(i)->origin[2]-(guard?watch->objective[2]:s_botNavMin[2]+512));
                    }
                    if(s_botAI[i].waypoint>=0) {
                        const vec_t *at=s_botNav[s_botAI[i].waypoint].origin;
                        Com_Printf("[bots] step %d at=%.0f,%.0f,%.0f move=%d,%d wants=%d\n",i,
                            at[0],at[1],at[2],s_botCmdState[i].forwardmove,s_botCmdState[i].rightmove,s_botAI[i].wantsMove);
                    }
                    if(s_botAI[i].objectiveEntity>=0) {
                        gentity_t *entity=&g_entities[s_botAI[i].objectiveEntity];
                        Com_Printf("[bots] interaction client=%d behavior=%d entity=%d hint=%d bounds=%.0f,%.0f,%.0f/%.0f,%.0f,%.0f\n",
                            i,s_botAI[i].objectiveBehavior,s_botAI[i].objectiveEntity,SV_GameClientNum(i)->cursorHintEntIndex,
                            entity->r.absmin[0],entity->r.absmin[1],entity->r.absmin[2],
                            entity->r.absmax[0],entity->r.absmax[1],entity->r.absmax[2]);
                    }
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
    // A guard below a balcony has not reached its firing position. Objective
    // interaction retains the original trigger-height tolerance separately.
    holding=objective && (ai->objectiveBehavior==1?
        Bot_DistanceSq(ps->origin,ai->guardPoint)<24*24:Bot_AtPoint(ps->origin,ai->objective,ai->objectiveRadius));
    if(holding && ai->objectiveBehavior==0 && ai->objectiveEntity>=0)
        holding=Bot_InObjective(ps->origin,ai->objectiveEntity);
    using=holding && ai->objectiveBehavior==2 && ai->objectiveEntity>=0 &&
        g_entities[ai->objectiveEntity].r.inuse &&
        g_entities[ai->objectiveEntity].classname==ai->objectiveClassname;
    if(ai->waypoint<0 && !holding && svs.time>=ai->perchUntil && svs.time>=ai->nextPath) {
        Bot_RequestPath(ai);
    }
    if(ai->perchUntil && svs.time>=ai->perchUntil) {
        ai->perchUntil=0; ai->patrolUntil=0; ai->goal=-1; ai->nextPath=0;
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
        if(svs.time>=ai->nextApproach || Bot_DistanceSq(ai->approachGoal,ai->objective)>8*8 ||
           Bot_DistanceSq(ai->approachOrigin,ps->origin)>64*64) {
            ai->nextApproach=svs.time+400; ai->approachUntil=0;
            memcpy(ai->approachGoal,ai->objective,sizeof(vec3_t));
            memcpy(ai->approachOrigin,ps->origin,sizeof(vec3_t));
            if(Bot_GroundDepth(ai->objective,ai->approach,192) &&
               (ai->objectiveEntity>=0 || fabsf(ai->approach[2]-ai->objective[2])<80) &&
               Bot_WalkEdge(ps->origin,ai->approach))
                ai->approachUntil=svs.time+400;
        }
        if(ai->approachUntil>svs.time) {
            moveYaw=atan2f(ai->approach[1]-ps->origin[1],ai->approach[0]-ps->origin[0])*BOT_RAD_TO_DEG;
            desiredYaw=moveYaw; distance=Bot_DistanceSq(ps->origin,ai->approach);
        }
    }
    if(ai->waypoint>=0 && ai->routeIndex<ai->routeCount) {
        ladderFrom=ai->routeIndex>0?ai->route[ai->routeIndex-1]:ai->routeFrom;
        ladder=ladderFrom>=0 && s_botNav[ladderFrom].edge[BOT_NAV_DIRECTIONS]==ai->waypoint;
        traversalDirection=Bot_TraversalDirection(ladderFrom,ai->waypoint);
        traversing=traversalDirection>=0 && !ladder;
    }
    if(holding || (sniper && !objective && ai->perchUntil>svs.time)) {
        distance=0;
        if(sniper) cmd->buttons|=0x200;
        // Pause between bounded scans instead of continuously spinning.
        if(svs.time>=ai->guardAt) {
            ai->guardAt=svs.time+1500+(int)(Bot_Random(ai)%1500);
            float facing=objective && ai->objectiveBehavior==1?
                atan2f(ai->guardPoint[1]-ai->objective[1],ai->guardPoint[0]-ai->objective[0])*BOT_RAD_TO_DEG:
                sniper && ai->goal>=0?s_botNav[ai->goal].lookoutDir*90.0f:ai->guardYaw;
            if(sniper && objective && ai->objectiveBehavior==1) facing+=180;
            ai->guardYaw=facing+(int)(Bot_Random(ai)%91)-45;
        }
        desiredYaw=ai->guardYaw;
    } else ai->guardAt=0;
    if(ai->alertUntil>svs.time) desiredYaw=ai->alertYaw;
    if(ai->target>=0 && Bot_IsEnemy(clientNum,ai->target)) {
        playerState_t *enemy=SV_GameClientNum(ai->target);
        float lead=ai->difficulty==0?0:ai->difficulty==1?.01f:.035f;
        float x=enemy->origin[0]+enemy->velocity[0]*lead-ps->origin[0];
        float y=enemy->origin[1]+enemy->velocity[1]*lead-ps->origin[1];
        float z=enemy->origin[2]+enemy->viewHeightCurrent*.72f+enemy->velocity[2]*lead-
            (enemy->groundEntityNum==1023?400*lead*lead:0)-ps->origin[2]-ps->viewHeightCurrent;
        visible=Bot_ObserveTarget(clientNum,ai,ai->target);
        if(visible) {
            float rangeSq=x*x+y*y, errorScale=1;
            static const int reaction[3]={700,650,200};
            int delay=reaction[ai->difficulty];
            if(sniper && ai->difficulty<2) {
                // Angular noise alone barely misses a nearby body. Increase
                // it smoothly at close range and prevent instant scoped shots.
                errorScale=1.25f+2.75f*(1-fminf(1,rangeSq/(384*384)));
                delay+=rangeSq<384*384?(ai->difficulty==0?650:450):
                    (ai->difficulty==0?350:200);
                if(ps->fWeaponPosFrac<.9f || ps->weaponTime>0) ai->scopeReadyAt=0;
                else if(!ai->scopeReadyAt)
                    ai->scopeReadyAt=svs.time+(ai->difficulty==0?650:450);
            } else ai->scopeReadyAt=0;
            desiredYaw=atan2f(y,x)*BOT_RAD_TO_DEG+ai->yawError*errorScale;
            desiredPitch=-atan2f(z,sqrtf(rangeSq))*BOT_RAD_TO_DEG+ai->pitchError*errorScale;
            if(svs.time-ai->acquiredAt>=delay && !using &&
               fabsf(Bot_AngleDelta(desiredYaw,ai->aim[1]))<6 &&
               fabsf(Bot_AngleDelta(desiredPitch,ai->aim[0]))<6) firing=1;
            if(sniper || x*x+y*y>350*350) cmd->buttons|=0x1000;
            if(sniper && ps->fWeaponPosFrac<.9f) firing=0;
            if(sniper && ai->difficulty<2 && (!ai->scopeReadyAt || svs.time<ai->scopeReadyAt)) firing=0;
        }
    }
    if(!visible) ai->scopeReadyAt=0;
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
    if(traversing && !using) {
        desiredYaw=moveYaw; desiredPitch=0;
        firing=0; cmd->buttons=0;
    }
    turnStep=(ai->difficulty==0?150.0f:ai->difficulty==1?170.0f:300.0f)*elapsed*.001f;
    ai->aim[0]=Bot_ApproachAngle(ai->aim[0],desiredPitch,turnStep);
    ai->aim[1]=Bot_ApproachAngle(ai->aim[1],desiredYaw,turnStep);
    ai->aim[2]=0;
    memcpy(cmd->angles,ai->aim,sizeof(vec3_t)); cmd->hasAngles=1;
    // Fine doorway links arrive within eight units; stopping at ten leaves
    // a dead zone where the bot can neither advance nor reach the waypoint.
    if(distance>4*4) {
        float delta=(moveYaw-ai->aim[1])/BOT_RAD_TO_DEG;
        qboolean tight=ai->waypoint>=0 && ladderFrom>=0 &&
            Bot_DistanceSq(s_botNav[ladderFrom].origin,s_botNav[ai->waypoint].origin)<24*24;
        int speed=visible?65:tight?(int)fmaxf(40,fminf(127,sqrtf(distance)*6)):127;
        cmd->forwardmove=(signed char)(cosf(delta)*speed);
        cmd->rightmove=(signed char)(-sinf(delta)*speed);
        if(!visible && ai->goal>=0 && Bot_DistanceSq(ps->origin,s_botNav[ai->goal].origin)>240*240 &&
           !tight && cmd->forwardmove>110 && ai->avoidUntil<=svs.time) cmd->buttons|=BUTTON_SPRINT;
    }
    ai->routeMove=distance>4*4 && ai->waypoint>=0;
    // Advance until the weapon has a useful engagement range. Brief dodges
    // have pauses; constant sideways movement made bots orbit and hit walls.
    if(visible && !ladder && !traversing && !holding && !using &&
       !(sniper && (ai->perchUntil>svs.time || ai->waypoint>=0)) && !(objective && ai->objectiveBehavior>=2) && ai->coverUntil<=svs.time) {
        WeaponDef *weapon=ps->weapon>0?BG_GetWeaponDef(ps->weapon):NULL;
        float range=sniper?900:weapon && weapon->weapClass==WEAPCLASS_SMG?240:
            weapon && weapon->weapClass==WEAPCLASS_MG?600:480;
        float enemyDistance=Bot_DistanceSq(ps->origin,ai->lastSeen);
        if(enemyDistance>=range*range && !objective &&
           (ai->difficulty==0 || Bot_Role(ps)!=BOT_FLANK || ai->waypoint<0) &&
           Bot_Clear(ps->origin,ai->lastSeen,clientNum,BOT_WORLD_MASK)) {
            float delta=atan2f(ai->lastSeen[1]-ps->origin[1],ai->lastSeen[0]-ps->origin[0])-ai->aim[1]/BOT_RAD_TO_DEG;
            cmd->forwardmove=(signed char)(cosf(delta)*65); cmd->rightmove=(signed char)(-sinf(delta)*65);
            ai->routeMove=0;
        }
        if(enemyDistance<range*range && !objective) {
            cmd->forwardmove=enemyDistance<96*96?-60:0;
            cmd->rightmove=0;
            ai->routeMove=0;
            if(!sniper && ai->difficulty>0) {
                if(svs.time>=ai->nextDodge) {
                    ai->avoidSide=(Bot_Random(ai)&1)?60:-60;
                    ai->dodgeUntil=svs.time+250;
                    ai->nextDodge=svs.time+1400+(int)(Bot_Random(ai)%600);
                }
                if(svs.time<ai->dodgeUntil) cmd->rightmove=ai->avoidSide;
            }
        }
        cmd->buttons&=~BUTTON_SPRINT;
    }
    if(visible && sniper && !using && !ladder && !traversing && !cmd->forwardmove && !cmd->rightmove) {
        cmd->buttons|=0x200;
        if((svs.time-ai->acquiredAt)%2200<1000) cmd->buttons|=0x8000;
    }
    if(ai->avoidUntil>svs.time && !ladder && !traversing && !holding && !using) {
        cmd->forwardmove=-60; cmd->rightmove=ai->avoidSide; cmd->buttons&=~BUTTON_SPRINT;
    }
    if(ai->yieldUntil>svs.time && !ladder && !traversing && !using &&
       (!holding || (ai->yieldForHuman && objective && ai->objectiveBehavior==1))) {
        if(!Bot_Teammate(clientNum,ai->yieldClient)) ai->yieldUntil=0;
        else if(Bot_DistanceSq(ps->origin,SV_GameClientNum(ai->yieldClient)->origin)>96*96) ai->yieldUntil=0;
        else {
            const vec_t *peer=SV_GameClientNum(ai->yieldClient)->origin;
            float delta=(ai->yieldForHuman?ai->yieldYaw:
                atan2f(ps->origin[1]-peer[1],ps->origin[0]-peer[0]))-ai->aim[1]/BOT_RAD_TO_DEG;
            cmd->forwardmove=(signed char)(cosf(delta)*90);
            cmd->rightmove=(signed char)(-sinf(delta)*90);
            cmd->buttons&=~BUTTON_SPRINT; ai->routeMove=0;
            if(ai->yieldForHuman) {
                cmd->buttons&=~(0x200|0x1000|0x8000);
                firing=0;ai->scopeReadyAt=0;
            }
        }
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
    if(traversing && !using) {
        float delta=(moveYaw-ai->aim[1])/BOT_RAD_TO_DEG;
        cmd->forwardmove=(signed char)(cosf(delta)*127);
        cmd->rightmove=(signed char)(-sinf(delta)*127);
        cmd->buttons&=~BUTTON_SPRINT;
        if((s_botNav[ladderFrom].jumpMask&(1u<<traversalDirection)) &&
           ps->groundEntityNum!=1023 && svs.time>=ai->nextTraversalJump) {
            cmd->buttons|=0x400;
            ai->nextTraversalJump=svs.time+1200;
        }
        ai->wantsMove=1;
    }
    if((cmd->forwardmove || cmd->rightmove) && !ladder && !traversing) {
        float yaw=ai->aim[1]/BOT_RAD_TO_DEG;
        if(!Bot_CanMove(clientNum,ps->origin,yaw,cmd->forwardmove,cmd->rightmove)) {
            int forward=cmd->forwardmove, right=cmd->rightmove, side;
            Bot_YieldToBlocker(ai,s_botMoveBlocker);
            cmd->forwardmove=cmd->rightmove=0; cmd->buttons&=~BUTTON_SPRINT;
            // Bounded local steering slides past a teammate or a corner; it
            // never ignores collision or changes another player's movement.
            for(side=0;side<4;++side) {
                float offset=ai->yieldForHuman && ai->yieldUntil>svs.time && side==0?180:
                    (side<2?35:70)*((side&1)?-1:1);
                float delta=offset/BOT_RAD_TO_DEG;
                int f=(int)(forward*cosf(delta)-right*sinf(delta));
                int r=(int)(forward*sinf(delta)+right*cosf(delta));
                if(Bot_CanMove(clientNum,ps->origin,yaw,f,r)) {
                    cmd->forwardmove=(signed char)f; cmd->rightmove=(signed char)r;
                    if(offset==180) ai->yieldYaw+=3.14159265f;
                    break;
                }
            }
            ai->wantsMove=1; // A blocked request must still recover.
        } else ai->wantsMove=1;
    } else if(!ladder && !traversing) ai->wantsMove=0;
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
            else if(!visible && ai->target<0 && ai->memoryUntil<=svs.time &&
                    svs.time-ai->lastVisibleAt>=2500 && ps->weaponTime<=0 &&
                    !ladder && !traversing && !(objective && ai->objectiveBehavior>=2) &&
                    weapon->iClipSize>0 && ps->ammoclip[weapon->iClipIndex]*3<=weapon->iClipSize &&
                    weapon->iAmmoIndex>=0 && weapon->iAmmoIndex<128 && ps->ammo[weapon->iAmmoIndex]>0)
                cmd->buttons|=0x10; // Top up during a quiet interval, using original reload rules.
            else if(firing &&
                    (!weapon->bSemiAuto || (!(oldButtons&1) && ps->weaponTime<=0)))
            {
                static const int burst[3]={250,300,650}, pause[3]={500,450,160};
                if(svs.time>=ai->nextBurst) {
                    ai->burstUntil=svs.time+burst[ai->difficulty];
                    ai->nextBurst=ai->burstUntil+pause[ai->difficulty];
                }
                if(svs.time<ai->burstUntil && !Bot_FriendlyInAim(clientNum,ai,ps)) cmd->buttons|=1;
            }
        }
    }
}
