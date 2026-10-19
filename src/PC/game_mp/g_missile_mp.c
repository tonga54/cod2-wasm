#include "common_types.h"
#include "imports.h"
#include "cod2_grenade.h"
extern level_locals_t level;
extern scr_const_t scr_const;

extern float sqrtf(float x);
extern float tanf(float x);
extern int rand(void);

#define _ENT(e) ((gentity_t *)(e))

#define ENTITY_STRIDE sizeof(gentity_s)

extern level_locals_t level;
extern scr_const_t scr_const;
extern gentity_t g_entities[];
extern entityHandler_t entityHandlers[20];
extern byte *vec3_origin_ptr;
extern byte *pPriorityMap;

#define LEVEL_TIME (level.time)
#define LEVEL_PREVIOUSTIME (level.previousTime)

#define HANDLER_MOD(h) (entityHandlers[(h)].methodOfDeath)
#define HANDLER_SPLASHMOD(h) (entityHandlers[(h)].splashMethodOfDeath)

#define G_ENTITY(num) ((gentity_t *)(((byte *)g_entities) + (num) * ENTITY_STRIDE))

enum {
    GMISSILE_ENTITYNUM_WORLD = 0x3fe,
    GMISSILE_EF_GRENADE_BOUNCE = 0x1000000,
    GMISSILE_FL_GUIDED = 0x10000,
    GMISSILE_FL_TURRET = 0x20000,
    GMISSILE_FL_HELD = 0x40000,
};

typedef struct {
    gentity_t *missile;
    int missileUseCount;
    int playerSpawnCount;
} heldGrenade_t;
static heldGrenade_t heldGrenades[64];

void G_ExplodeMissile(gentity_t *ent);
gentity_t *fire_grenade(gentity_t *self, vec_t *start, vec_t *dir, int grenadeWPID, int time);
gentity_t *fire_rocket(gentity_t *self, vec_t *start, vec_t *dir);
static void G_MissileLandAngles(gentity_t *ent, trace_t *trace, vec_t *vAngles, qboolean bForceAlign);
static qboolean G_BounceMissile(gentity_t *ent, trace_t *trace);
void G_RunMissile(gentity_t *ent);

extern WeaponDef *BG_GetWeaponDef(int weaponIndex);
extern int BG_GetNumWeapons(void);
extern void BG_EvaluateTrajectory(trajectory_t *tr, int atTime, vec_t *result);
extern void BG_EvaluateTrajectoryDelta(trajectory_t *tr, int atTime, vec_t *result);
extern unsigned char G_SetOrigin(gentity_t *ent, const vec_t *origin);
extern unsigned char G_SetAngle(gentity_t *ent, vec_t *angles);
extern void G_TraceCapsule(trace_t *result, vec_t *start, vec_t *mins, vec_t *maxs, vec_t *end, int skipNumber, int mask);
extern int DirToByte(vec_t *dir);
extern unsigned char G_AddEvent(gentity_t *ent, int event, int eventParm);
extern int SV_PointContents(vec_t *point, int passEntityNum, int contentMask);
extern void SV_LinkEntity(gentity_t *ent);
extern qboolean G_RadiusDamage(const vec_t *origin, gentity_t *ent, gentity_t *attacker, float innerDamage, float outerDamage, float radius, gentity_t *ignore, int splashMod);
extern void Server_SwitchToValidFxScheduler(void);
extern EffectTemplate *FX_RegisterEffect(const char *name);
extern float FX_GetEffectLength(EffectTemplate *handle);
extern gentity_t *G_Spawn(void);
extern void Scr_SetString(scr_string_t *dst, unsigned int str);
extern void vectoangles(vec_t *dir, vec_t *angles);
extern float AngleNormalize360(float angle);
extern float AngleNormalize180(float angle);
extern float AngleSubtract(float a, float b);
extern float PitchForYawOnNormal(float yaw, vec_t *normal);
extern float flrand(float min, float max);
extern float randomf(void);
extern float Vec3Normalize(vec_t *v);
extern float Vec3NormalizeTo(vec_t *v, vec_t *out);
extern void G_LocationalTrace(trace_t *result, vec_t *start, vec_t *end, int skipNumber, int mask, byte *priorityMap);
extern gentity_t *G_TempEntity(vec_t *origin, int event);
extern unsigned char G_FreeEntity(gentity_t *ent);
extern int G_RunThink(gentity_t *ent);
extern int LogAccuracyHit(gentity_t *target, gentity_t *attacker);
extern void G_Damage(gentity_t *target, gentity_t *inflictor, gentity_t *attacker, vec_t *dir, vec_t *point, int damage, int dflags, int mod, int hitClient, int hitLoc);
extern void G_CheckHitTriggerDamage(gentity_t *attacker, vec_t *start, vec_t *end, int damage, int mod);
extern void G_GrenadeTouchTriggerDamage(gentity_t *ent, vec_t *oldOrigin, vec_t *origin, int radius, int mod);
extern void SnapVectorTowards(vec_t *v, vec_t *to);
extern void G_GetPlayerViewOrigin(gentity_t *ent, vec_t *origin);
extern qboolean OnSameTeam(gentity_t *ent1, gentity_t *ent2);
extern SoundAlias G_SoundAliasIndex(const char *name);
extern float atan2f(float y, float x);
extern float asinf(float x);
extern float cosf(float x);
extern float sinf(float x);

/* A placed explosive's spawnflags: a charge stuck where it hit, a mine that
 * has sensed its enemy. */
#define GMISSILE_PLACED_STUCK 1
#define GMISSILE_PLACED_TRIGGERED 2
/* How far above or below a mine an enemy's feet may be. */
#define GMISSILE_PROXIMITY_BELOW 24.0f
#define GMISSILE_PROXIMITY_ABOVE 48.0f
/* The S-Mine is set down this far ahead of its owner's feet, this far short
 * of a wall, found from the hand's height down to this far below them; a
 * resting grenade's origin stands 1.5 above the ground. */
#define GMISSILE_MINE_STEP 20.0f
#define GMISSILE_MINE_WALL 4.0f
#define GMISSILE_MINE_HAND 24.0f
#define GMISSILE_MINE_DEPTH 64.0f
#define GMISSILE_MINE_REST 1.5f
/* Its owner takes it back by holding Use (+activate or +usereload) for half
 * a second within reach of their feet, with it in sight. */
#define GMISSILE_PICKUP_REACH 52.0f
#define GMISSILE_PICKUP_HEIGHT 40.0f
#define GMISSILE_PICKUP_MSEC 500
#define GMISSILE_BUTTONS_USE 0x28

static inline void VectorCopy(const vec_t *src, vec_t *dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

static inline void VectorSubtract(const vec_t *a, const vec_t *b, vec_t *out)
{
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

static inline void VectorAdd(const vec_t *a, const vec_t *b, vec_t *out)
{
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
    out[2] = a[2] + b[2];
}

static inline void VectorScale(const vec_t *v, float scale, vec_t *out)
{
    out[0] = v[0] * scale;
    out[1] = v[1] * scale;
    out[2] = v[2] * scale;
}

static inline void VectorClear(vec_t *v)
{
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
}

static inline float VectorLength(const vec_t *v)
{
    return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

static inline float DotProduct(const vec_t *a, const vec_t *b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline void VectorMA(const vec_t *a, float scale, const vec_t *b, vec_t *out)
{
    out[0] = a[0] + scale * b[0];
    out[1] = a[1] + scale * b[1];
    out[2] = a[2] + scale * b[2];
}

static inline float SnapFloat(float f)
{
    return (float)(int)f;
}

static inline void SnapVector(vec_t *v)
{
    v[0] = SnapFloat(v[0]);
    v[1] = SnapFloat(v[1]);
    v[2] = SnapFloat(v[2]);
}

static inline void LerpPosition(const vec_t *start, const vec_t *end, float fraction, vec_t *out)
{
    out[0] = start[0] + (end[0] - start[0]) * fraction;
    out[1] = start[1] + (end[1] - start[1]) * fraction;
    out[2] = start[2] + (end[2] - start[2]) * fraction;
}

void G_ResetHeldGrenades(void)
{
    memset(heldGrenades, 0, sizeof(heldGrenades));
}

static gentity_t *G_GetHeldGrenade(gentity_t *player)
{
    int clientNum = player->s.number;
    heldGrenade_t *held;
    gentity_t *missile;
    if ((unsigned)clientNum >= 64 || !player->client)
        return NULL;
    held = &heldGrenades[clientNum];
    missile = held->missile;
    if (!missile)
        return NULL;
    if (!missile->r.inuse || missile->useCount != held->missileUseCount ||
        missile->s.eType != 4 || !(missile->flags & GMISSILE_FL_HELD) ||
        missile->s.otherEntityNum != clientNum ||
        player->client->ps.stats[5] != held->playerSpawnCount) {
        held->missile = NULL;
        return NULL;
    }
    return missile;
}

static void G_HoldGrenade(gentity_t *player, gentity_t *missile)
{
    heldGrenade_t *held = &heldGrenades[player->s.number];
    held->missile = missile;
    held->missileUseCount = missile->useCount;
    held->playerSpawnCount = player->client->ps.stats[5];
    missile->flags |= GMISSILE_FL_HELD;
    missile->s.otherEntityNum = player->s.number;
    missile->r.ownerNum = player->s.number;
    missile->parent = COD2_GEntityHandle(player);
    missile->r.svFlags |= 1; /* The viewmodel already draws the held grenade. */
    G_GetPlayerViewOrigin(player, missile->r.currentOrigin);
    G_SetOrigin(missile, missile->r.currentOrigin);
    SV_LinkEntity(missile);
}

static qboolean G_CanPickUpGrenade(const gentity_t *player)
{
    const playerState_t *ps = &player->client->ps;
    return player->health > 0 && ps->pm_type == 0 && ps->weaponstate == 0 &&
        !(ps->eFlags & 0x300) && !(ps->pm_flags & (0x10 | 0x4 | 0x1000 | 0x8000));
}

static gentity_t *G_FindNearbyGrenade(gentity_t *player)
{
    gentity_t *nearest = NULL;
    float nearestDistSq = GRENADE_THROWBACK_RANGE * GRENADE_THROWBACK_RANGE;
    vec3_t eye;
    vec3_t reach;
    int count = level.num_entities < 1022 ? level.num_entities : 1022;
    int i;
    G_GetPlayerViewOrigin(player, eye);
    VectorCopy(player->client->ps.origin, reach);
    reach[2] += player->client->ps.viewHeightCurrent * 0.5f;
    for (i = 64; i < count; i++) {
        gentity_t *missile = &g_entities[i];
        vec3_t delta;
        float distSq;
        trace_t trace;
        if (!missile->r.inuse || missile->s.eType != 4 || missile->handler != 7 ||
            !(missile->s.eFlags & GMISSILE_EF_GRENADE_BOUNCE) ||
            (missile->flags & GMISSILE_FL_HELD) || missile->nextthink <= LEVEL_TIME)
            continue;
        if (BG_GetWeaponDef(missile->s.weapon)->offhandClass != OFFHAND_CLASS_FRAG_GRENADE ||
            BG_IsPlacedExplosive(BG_GetWeaponDef(missile->s.weapon)))
            continue;
        VectorSubtract(missile->r.currentOrigin, reach, delta);
        distSq = DotProduct(delta, delta);
        if (distSq > nearestDistSq)
            continue;
        /* Never claim a live grenade through a wall or a closed door. */
        G_TraceCapsule(&trace, eye, (vec_t *)vec3_origin_ptr, (vec_t *)vec3_origin_ptr,
                       missile->r.currentOrigin, player->s.number, 0x811);
        if (trace.startsolid || (trace.fraction < 1.0f && trace.entityNum != missile->s.number))
            continue;
        nearest = missile;
        nearestDistSq = distSq;
    }
    return nearest;
}

void G_UpdateGrenadeHint(gentity_t *player)
{
    gentity_t *missile;
    if (!G_CanPickUpGrenade(player))
        return;
    missile = G_FindNearbyGrenade(player);
    if (missile) {
        player->client->ps.cursorHint = GRENADE_THROWBACK_HINT;
        player->client->ps.cursorHintString = -1;
        player->client->ps.cursorHintEntIndex = 1023;
    }
}

void G_GrenadeCookOff(gentity_t *player)
{
    playerState_t *ps = &player->client->ps;
    gentity_t *missile = G_GetHeldGrenade(player);
    vec3_t origin;
    vec3_t zero = {0, 0, 0};
    if (!ps->offHandIndex)
        return;
    /* The wall-clock expiry also runs if no usercmd arrives. Consume the
     * inventory grenade here only when Pmove has not already consumed it. */
    if ((ps->pm_flags & 0x10) && !(ps->pm_flags & PMF_GRENADE_THROWBACK)) {
        int clip = BG_GetWeaponDef(ps->offHandIndex)->iClipIndex;
        if (ps->ammoclip[clip] > 0)
            ps->ammoclip[clip]--;
    }
    G_GetPlayerViewOrigin(player, origin);
    ps->grenadeTimeLeft = 0;
    if (!missile)
        missile = fire_grenade(player, origin, zero, ps->offHandIndex, 0);
    heldGrenades[player->s.number].missile = NULL;
    missile->flags &= ~GMISSILE_FL_HELD;
    missile->r.svFlags &= ~1;
    G_SetOrigin(missile, origin);
    missile->nextthink = 0;
    ps->pm_flags &= ~(0x810 | PMF_GRENADE_THROWBACK);
    ps->weaponstate = 16; /* WEAPON_OFFHAND_END */
    ps->weaponTime = ps->weaponDelay = 0;
    G_ExplodeMissile(missile);
}

void G_BeginGrenadeInput(gentity_t *player, const usercmd_t *cmd)
{
    playerState_t *ps = &player->client->ps;
    gentity_t *missile = G_GetHeldGrenade(player);
    int pendingMsec = cmd->serverTime - ps->commandTime;
    if (pendingMsec < 0) pendingMsec = 0;
    if (pendingMsec > 1000) pendingMsec = 1000; /* Pmove's command horizon. */
    if (!missile && (cmd->buttons & 0x10000) &&
        !(player->client->sess.oldcmd.buttons & 0x10000) && G_CanPickUpGrenade(player)) {
        missile = G_FindNearbyGrenade(player);
        /* A stale hint must not block ordinary inventory grenades forever. */
        if (ps->cursorHint == GRENADE_THROWBACK_HINT)
            ps->cursorHint = 0;
        if (missile) {
            G_HoldGrenade(player, missile);
            ps->offHandIndex = missile->s.weapon;
            ps->pm_flags = (ps->pm_flags & ~0x40) | 0x10 | PMF_GRENADE_THROWBACK;
            ps->weaponstate = 14; /* Already armed: WEAPON_OFFHAND_HOLD. */
            ps->weaponTime = ps->weaponDelay = 0;
            ps->weapAnim = ((ps->weapAnim & 0x200) ^ 0x200) | 0x13;
        }
    }
    if (missile) {
        int remaining = missile->nextthink - LEVEL_TIME;
        if (remaining <= 0) {
            G_GrenadeCookOff(player);
        } else {
            /* Pmove will consume pendingMsec; repeated/batched commands may
             * never extend the authoritative, wall-clock explosion deadline. */
            ps->grenadeTimeLeft = remaining + pendingMsec;
        }
    }
}

void G_EndGrenadeInput(gentity_t *player)
{
    playerState_t *ps = &player->client->ps;
    gentity_t *missile;
    vec3_t origin;
    vec3_t zero = {0, 0, 0};
    if ((unsigned)player->s.number >= 64 || player->health <= 0 ||
        !(ps->pm_flags & 0x10) || !ps->offHandIndex || ps->grenadeTimeLeft <= 0 ||
        !(ps->weaponstate == 14 || (ps->weaponstate == 15 && ps->weaponDelay > 0)) ||
        !BG_GrenadeCanCook(BG_GetWeaponDef(ps->offHandIndex)))
        return;
    missile = G_GetHeldGrenade(player);
    if (!missile) {
        int remaining = ps->grenadeTimeLeft;
        G_GetPlayerViewOrigin(player, origin);
        missile = fire_grenade(player, origin, zero, ps->offHandIndex, remaining);
        ps->grenadeTimeLeft = remaining;
        G_HoldGrenade(player, missile);
    }
}

static int s_placedSequence;

static int G_PlacedTeam(const gentity_t *owner)
{
    return owner->client ? owner->client->sess.cs.team : 0;
}

static void G_PlacedSound(gentity_t *ent, const char *alias)
{
    gentity_t *tent;

    if (!alias || !*alias)
        return;
    tent = G_TempEntity(ent->r.currentOrigin, 0xb3); /* EV_SOUND_ALIAS, as playSound sends it */
    tent->r.svFlags |= 8;
    tent->s.eventParm = G_SoundAliasIndex(alias);
}

/* An explosive stays its owner's while they are connected on the side that
 * placed it. */
static gentity_t *G_PlacedOwner(gentity_t *ent)
{
    gentity_t *owner = COD2_GEntityFromHandle(ent->parent);

    if (!owner || !owner->r.inuse || !owner->client || owner->client->sess.connected != 2 ||
        G_PlacedTeam(owner) != ent->team)
        return NULL;
    return owner;
}

/* Upright, facing as its owner does, on the ground a step ahead of their
 * feet, short of any wall; over a drop it falls from the hand. */
static void G_SetDownMine(gentity_t *owner, gentity_t *ent)
{
    vec3_t hand, spot, below, angles;
    trace_t trace;
    float yaw = owner->client ? owner->client->ps.viewangles[1] : owner->r.currentAngles[1];

    VectorCopy(owner->r.currentOrigin, hand);
    hand[2] += GMISSILE_MINE_HAND;
    spot[0] = hand[0] + cosf(yaw * 0.0174532925f) * GMISSILE_MINE_STEP;
    spot[1] = hand[1] + sinf(yaw * 0.0174532925f) * GMISSILE_MINE_STEP;
    spot[2] = hand[2];
    G_LocationalTrace(&trace, hand, spot, owner->s.number, ent->clipmask, pPriorityMap);
    if (trace.startsolid)
        trace.fraction = 0.0f;
    else if (trace.fraction < 1.0f)
        trace.fraction = trace.fraction > GMISSILE_MINE_WALL / GMISSILE_MINE_STEP ?
            trace.fraction - GMISSILE_MINE_WALL / GMISSILE_MINE_STEP : 0.0f;
    LerpPosition(hand, spot, trace.fraction, spot);
    VectorCopy(spot, below);
    below[2] = owner->r.currentOrigin[2] - GMISSILE_MINE_DEPTH;
    G_LocationalTrace(&trace, spot, below, owner->s.number, ent->clipmask, pPriorityMap);
    angles[0] = angles[2] = 0.0f;
    angles[1] = yaw;
    if (!trace.startsolid && trace.fraction < 1.0f && trace.normal[2] > 0.7f) {
        LerpPosition(spot, below, trace.fraction, spot);
        spot[2] += GMISSILE_MINE_REST;
        G_SetOrigin(ent, spot);
        ent->s.groundEntityNum = trace.entityNum;
    } else {
        G_SetOrigin(ent, spot);
        ent->s.pos.trType = 5;
        ent->s.pos.trTime = LEVEL_TIME;
    }
    G_SetAngle(ent, angles);
    /* A mine at rest never moves, so the missile code would never link it
     * and no client would be sent it. */
    SV_LinkEntity(ent);
}

/* Nothing waits on a fuse. The owner keeps as many of a kind as the weapon
 * holds; a newer one replaces the oldest. The owner travels with it so the
 * client can show it to their team. */
static void G_PlaceExplosive(gentity_t *owner, gentity_t *ent, const WeaponDef *weapDef)
{
    int limit = weapDef->iMaxAmmo > 0 ? weapDef->iMaxAmmo : 1;
    int i;

    ent->nextthink = 0;
    ent->s.otherEntityNum = owner->s.number;
    ent->team = (byte)G_PlacedTeam(owner);
    ent->count = 0;
    ent->spawnflags = 0;
    ent->maxHealth = ++s_placedSequence;
    if (weapDef->proximityRadius > 0.0f)
        G_SetDownMine(owner, ent);
    for (;;) {
        gentity_t *oldest = NULL;
        int count = 0;

        for (i = 64; i < level.num_entities; i++) {
            gentity_t *other = &g_entities[i];

            if (other == ent || !other->r.inuse || other->s.eType != 4 || other->handler != 7 ||
                other->s.weapon != ent->s.weapon || other->parent != ent->parent)
                continue;
            count++;
            if (!oldest || other->maxHealth < oldest->maxHealth)
                oldest = other;
        }
        if (count < limit || !oldest)
            return;
        G_FreeEntity(oldest);
    }
}

/* CoD4's stickiness "all": the charge stays where it first hits anything but
 * a player, its flat side to the surface (-y along the normal) and its top
 * up the wall, or along the throw on a floor or ceiling. */
static void G_StickMissile(gentity_t *ent, const trace_t *trace, const vec_t *endpos)
{
    vec3_t left, up, forward, origin, angles;
    float along;

    VectorScale(trace->normal, -1.0f, left);
    up[0] = up[1] = 0.0f;
    up[2] = 1.0f;
    along = DotProduct(up, trace->normal);
    VectorMA(up, -along, trace->normal, up);
    if (VectorLength(up) < 0.1f) {
        VectorCopy(ent->s.pos.trDelta, up);
        along = DotProduct(up, trace->normal);
        VectorMA(up, -along, trace->normal, up);
        if (VectorLength(up) < 0.1f) {
            up[0] = 1.0f;
            up[1] = up[2] = 0.0f;
        }
    }
    Vec3Normalize(up);
    forward[0] = left[1] * up[2] - left[2] * up[1];
    forward[1] = left[2] * up[0] - left[0] * up[2];
    forward[2] = left[0] * up[1] - left[1] * up[0];
    /* AngleVectors' axes: forward (cp cy, cp sy, -sp), left's z sr cp, up's z cr cp. */
    angles[0] = -asinf(forward[2] > 1.0f ? 1.0f : forward[2] < -1.0f ? -1.0f : forward[2]) * 57.2957795f;
    angles[1] = atan2f(forward[1], forward[0]) * 57.2957795f;
    angles[2] = atan2f(left[2], up[2]) * 57.2957795f;
    VectorMA(endpos, 0.25f, trace->normal, origin);
    G_SetOrigin(ent, origin);
    G_SetAngle(ent, angles);
    ent->s.groundEntityNum = trace->entityNum;
    ent->s.eFlags &= ~GMISSILE_EF_GRENADE_BOUNCE;
    ent->spawnflags |= GMISSILE_PLACED_STUCK;
    G_PlacedSound(ent, BG_GetWeaponDef(ent->s.weapon)->szBounceSound);
    SV_LinkEntity(ent);
}

/* A placed explosive's frame before the missile code: 1 when it has nothing
 * left to do. A mine arms once it lies still, then senses enemies' feet. */
static qboolean G_RunPlacedExplosive(gentity_t *ent, const WeaponDef *weapDef)
{
    gentity_t *owner = G_PlacedOwner(ent);
    float radiusSq = weapDef->proximityRadius * weapDef->proximityRadius;
    int i;

    if (!owner) {
        G_FreeEntity(ent);
        return 1;
    }
    if (ent->spawnflags & GMISSILE_PLACED_STUCK) {
        G_RunThink(ent);
        return 1;
    }
    if (weapDef->proximityRadius <= 0.0f || ent->s.pos.trType != 0 || ent->s.groundEntityNum == 1023)
        return 0;
    if (!ent->count) {
        ent->count = LEVEL_TIME + (weapDef->iProximityArmTime > 0 ? weapDef->iProximityArmTime : 1);
        ent->r.currentAngles[2] = 0.0f;
        G_SetAngle(ent, ent->r.currentAngles);
        G_PlacedSound(ent, weapDef->szArmSound);
    }
    if ((ent->spawnflags & GMISSILE_PLACED_TRIGGERED) || LEVEL_TIME < ent->count)
        return 0;
    for (i = 0; i < 64; i++) {
        gentity_t *other = &g_entities[i];
        vec3_t delta;

        if (!other->r.inuse || !other->client || other == owner || other->health <= 0 ||
            other->client->sess.connected != 2 || other->client->sess.sessionState != 0 ||
            OnSameTeam(owner, other))
            continue;
        VectorSubtract(other->r.currentOrigin, ent->r.currentOrigin, delta);
        if (delta[0] * delta[0] + delta[1] * delta[1] > radiusSq ||
            delta[2] < -GMISSILE_PROXIMITY_BELOW || delta[2] > GMISSILE_PROXIMITY_ABOVE)
            continue;
        ent->spawnflags |= GMISSILE_PLACED_TRIGGERED;
        ent->nextthink = LEVEL_TIME + weapDef->iProximityDelay;
        G_PlacedSound(ent, weapDef->szTriggerSound);
        break;
    }
    return 0;
}

/* The player's detonator: every charge of theirs goes off on its next frame,
 * near or far, still in flight or stuck. */
void G_DetonateExplosives(gentity_t *player)
{
    int i;

    for (i = 64; i < level.num_entities; i++) {
        gentity_t *ent = &g_entities[i];

        if (!ent->r.inuse || ent->s.eType != 4 || ent->handler != 7 ||
            !BG_GetWeaponDef(ent->s.weapon)->hasDetonator || COD2_GEntityFromHandle(ent->parent) != player)
            continue;
        ent->nextthink = LEVEL_TIME;
    }
}

static qboolean G_CanTakeMine(const gentity_t *player)
{
    const playerState_t *ps = &player->client->ps;

    return player->health > 0 && ps->pm_type == 0 && player->client->sess.sessionState == 0 &&
        !(ps->eFlags & 0x300) && !(ps->pm_flags & (0x10 | 0x4));
}

static int s_mineListTime = -1;
static int s_mineCount;
static unsigned short s_mineList[128];

/* The mines in the level, listed once a frame for every player's search. */
static void G_ListMines(void)
{
    int i;

    if (s_mineListTime == LEVEL_TIME)
        return;
    s_mineListTime = LEVEL_TIME;
    s_mineCount = 0;
    for (i = 64; i < level.num_entities && s_mineCount < (int)(sizeof(s_mineList) / sizeof(s_mineList[0])); i++) {
        const gentity_t *ent = &g_entities[i];

        if (ent->r.inuse && ent->s.eType == 4 && ent->handler == 7 &&
            BG_GetWeaponDef(ent->s.weapon)->proximityRadius > 0.0f)
            s_mineList[s_mineCount++] = (unsigned short)i;
    }
}

/* The nearest of the player's own mines lying still within reach and in
 * sight, whatever their kit holds (a mine outlives its owner's life, and the
 * next one starts with a full kit); NULL otherwise. */
static gentity_t *G_FindOwnMine(gentity_t *player)
{
    float nearest = GMISSILE_PICKUP_REACH * GMISSILE_PICKUP_REACH;
    gentity_t *found = NULL;
    vec3_t eye;
    int i;

    if (!G_CanTakeMine(player))
        return NULL;
    G_ListMines();
    G_GetPlayerViewOrigin(player, eye);
    for (i = 0; i < s_mineCount; i++) {
        gentity_t *mine = &g_entities[s_mineList[i]];
        const WeaponDef *weapDef;
        vec3_t delta;
        trace_t trace;
        float distance;

        if (!mine->r.inuse || mine->s.eType != 4 || mine->handler != 7 ||
            COD2_GEntityFromHandle(mine->parent) != player || mine->s.pos.trType != 0 ||
            (mine->spawnflags & GMISSILE_PLACED_TRIGGERED))
            continue;
        weapDef = BG_GetWeaponDef(mine->s.weapon);
        if (weapDef->proximityRadius <= 0.0f || weapDef->hasDetonator)
            continue;
        VectorSubtract(mine->r.currentOrigin, player->r.currentOrigin, delta);
        distance = delta[0] * delta[0] + delta[1] * delta[1];
        if (distance > nearest || delta[2] < -GMISSILE_PICKUP_HEIGHT || delta[2] > GMISSILE_PICKUP_HEIGHT)
            continue;
        G_TraceCapsule(&trace, eye, (vec_t *)vec3_origin_ptr, (vec_t *)vec3_origin_ptr, mine->r.currentOrigin,
                       player->s.number, 0x811);
        if (trace.startsolid || (trace.fraction < 1.0f && trace.entityNum != mine->s.number))
            continue;
        nearest = distance;
        found = mine;
    }
    return found;
}

static int s_minePickupStart[64];
static int s_minePickupEntity[64];

/* Each server frame, after the cursor hints: where nothing else is offered,
 * a mine of the player's within reach offers itself and a held Use takes it,
 * back into the kit when they carry the mine with room for one more. */
void G_UpdateMinePickup(gentity_t *player)
{
    playerState_t *ps = &player->client->ps;
    int clientNum = player->s.number;
    gentity_t *mine;
    const WeaponDef *weapDef;

    if ((unsigned)clientNum >= 64)
        return;
    mine = ps->cursorHint ? NULL : G_FindOwnMine(player);
    if (!mine || !(player->client->buttons & GMISSILE_BUTTONS_USE)) {
        s_minePickupStart[clientNum] = 0;
    } else if (!s_minePickupStart[clientNum] || s_minePickupStart[clientNum] > LEVEL_TIME ||
               s_minePickupEntity[clientNum] != mine->s.number) {
        s_minePickupStart[clientNum] = LEVEL_TIME;
        s_minePickupEntity[clientNum] = mine->s.number;
    } else if (LEVEL_TIME - s_minePickupStart[clientNum] >= GMISSILE_PICKUP_MSEC) {
        weapDef = BG_GetWeaponDef(mine->s.weapon);
        if (((ps->weapons[mine->s.weapon >> 5] >> (mine->s.weapon & 0x1f)) & 1) &&
            ps->ammoclip[weapDef->iClipIndex] < weapDef->iClipSize)
            ps->ammoclip[weapDef->iClipIndex]++;
        G_PlacedSound(mine, weapDef->szPickupSound);
        G_FreeEntity(mine);
        s_minePickupStart[clientNum] = 0;
        return;
    }
    if (mine) {
        ps->cursorHint = PLACED_PICKUP_HINT;
        ps->cursorHintString = mine->s.weapon;
        ps->cursorHintEntIndex = 1023;
    }
}

/* Asked by the snapshot before sight: a placed explosive at rest goes to its
 * owner and, in a team mode, their teammates (the Axis 1, the Allies 2)
 * within the emblem's reach of their eyes wherever it lies, so its squad
 * emblem shows through walls. Anyone else gets it only in sight. */
qboolean G_PlacedExplosiveSentTo(const entityState_t *es, const vec_t *eye, int clientNum)
{
    vec3_t delta;
    int team;

    if (es->eType != 4 || es->pos.trType != 0 || es->weapon <= 0 || es->weapon > BG_GetNumWeapons() ||
        (unsigned)es->otherEntityNum >= 64 || (unsigned)clientNum >= 64 ||
        !BG_IsPlacedExplosive(BG_GetWeaponDef(es->weapon)))
        return 0;
    if (es->otherEntityNum != clientNum) {
        if (!g_entities[es->otherEntityNum].client || !g_entities[clientNum].client)
            return 0;
        team = G_PlacedTeam(&g_entities[clientNum]);
        if ((team != 1 && team != 2) || G_PlacedTeam(&g_entities[es->otherEntityNum]) != team)
            return 0;
    }
    VectorSubtract(es->pos.trBase, eye, delta);
    return DotProduct(delta, delta) <= PLACED_EMBLEM_RANGE * PLACED_EMBLEM_RANGE;
}

void G_ExplodeMissile(gentity_t *ent)
{
    WeaponDef *weapDef;
    vec3_t origin;
    vec3_t end;
    trace_t trace;

    weapDef = BG_GetWeaponDef((_ENT(ent)->s.weapon));

    if (weapDef->projExplosion == 2 && (_ENT(ent)->s.groundEntityNum) == 0x3FF) {
        (_ENT(ent)->nextthink) = LEVEL_TIME + 50;
        return;
    }

    BG_EvaluateTrajectory((&_ENT(ent)->s.pos), LEVEL_TIME, origin);

    SnapVector(origin);

    G_SetOrigin(ent, origin);

    (_ENT(ent)->s.eType) = 0;

    (_ENT(ent)->s.eFlags) |= 0x20;

    (_ENT(ent)->flags) |= 0x800;

    (_ENT(ent)->r.svFlags) |= 8;

    VectorCopy((_ENT(ent)->r.currentOrigin), end);
    end[2] -= 16.0f;

    G_TraceCapsule(&trace, (_ENT(ent)->r.currentOrigin), (vec_t *)vec3_origin_ptr, (vec_t *)vec3_origin_ptr, end,
                   (_ENT(ent)->s.number), 0x811);

    if (weapDef->projExplosion == 2) {
        G_AddEvent(ent, 0xBF, DirToByte(trace.normal));
    } else {
        G_AddEvent(ent, 0xBC, DirToByte(trace.normal));
    }

    if (SV_PointContents((_ENT(ent)->r.currentOrigin), -1, 0x20)) {

        (_ENT(ent)->s.surfType) = 0x14;
    } else {

        (_ENT(ent)->s.surfType) = (trace.surfaceFlags & 0x1F00000) >> 20;
    }

    if (weapDef->szProjExplosionEffect && *weapDef->szProjExplosionEffect) {

        (_ENT(ent)->s.eFlags) |= 0x10000;
        Server_SwitchToValidFxScheduler();
        EffectTemplate *fxHandle = FX_RegisterEffect(weapDef->szProjExplosionEffect);
        (_ENT(ent)->s.time) = LEVEL_TIME;
        float fxLength = fxHandle ? FX_GetEffectLength(fxHandle) : 0;
        (_ENT(ent)->s.time2) = LEVEL_TIME + (int)(fxLength + 1.0f);
    } else {

        (_ENT(ent)->freeAfterEvent) = 1;
    }

    if (weapDef->iExplosionRadius > 0) {
        int splashMod = HANDLER_SPLASHMOD((_ENT(ent)->handler));
        G_RadiusDamage((_ENT(ent)->r.currentOrigin), ent, COD2_GEntityFromHandle(_ENT(ent)->parent),
                       (float)weapDef->iExplosionInnerDamage,
                       (float)weapDef->iExplosionOuterDamage,
                       (float)weapDef->iExplosionRadius,
                       ent, splashMod);
    }

    SV_LinkEntity(ent);
}

gentity_t *fire_grenade(gentity_t *self, vec_t *start, vec_t *dir, int grenadeWPID, int time)
{
    gentity_t *bolt;
    WeaponDef *weapDef;
    gclient_t *cl;
    vec3_t angles;

    bolt = G_GetHeldGrenade(self);
    if (bolt) {
        /* Throw the very same missile. Picking up, holding, bouncing and
         * throwing it again all retain its original nextthink deadline. */
        heldGrenades[self->s.number].missile = NULL;
        bolt->flags &= ~GMISSILE_FL_HELD;
    } else {
        bolt = G_Spawn();
        cl = self->client;
        bolt->nextthink = LEVEL_TIME + (cl && cl->ps.grenadeTimeLeft > 0 ?
                                       cl->ps.grenadeTimeLeft : time);
    }

    cl = (_ENT(self)->client);

    if (cl) {
        ((gclient_t *)cl)->ps.grenadeTimeLeft = 0;
    }

    (_ENT(bolt)->handler) = 7;

    (_ENT(bolt)->s.eType) = 4;
    bolt->s.otherEntityNum = 1023;
    bolt->s.groundEntityNum = 1023;

    (_ENT(bolt)->r.svFlags) = 8;

    (_ENT(bolt)->s.weapon) = grenadeWPID;

    (_ENT(bolt)->r.ownerNum) = (_ENT(self)->s.number);

    (_ENT(bolt)->parent) = COD2_GEntityHandle(self);

    weapDef = BG_GetWeaponDef(grenadeWPID);

    Scr_SetString(&(_ENT(bolt)->classname), scr_const.grenade);

    (_ENT(bolt)->damage) = weapDef->damage;

    (_ENT(bolt)->s.eFlags) = GMISSILE_EF_GRENADE_BOUNCE;

    (_ENT(bolt)->clipmask) = 0x2802891;

    (_ENT(bolt)->s.time) = LEVEL_TIME + 50;

    (&_ENT(bolt)->s.pos)->trType = 5;

    (&_ENT(bolt)->s.pos)->trTime = LEVEL_TIME;

    VectorCopy(start, (&_ENT(bolt)->s.pos)->trBase);

    VectorCopy(dir, (&_ENT(bolt)->s.pos)->trDelta);

    SnapVector((&_ENT(bolt)->s.pos)->trDelta);

    (&_ENT(bolt)->s.apos)->trType = 2;

    (&_ENT(bolt)->s.apos)->trTime = LEVEL_TIME;

    vectoangles(dir, (&_ENT(bolt)->s.apos)->trBase);

    (&_ENT(bolt)->s.apos)->trBase[0] = AngleNormalize360((&_ENT(bolt)->s.apos)->trBase[0] - 120.0f);

    (&_ENT(bolt)->s.apos)->trDelta[0] = flrand(-45.0f, 45.0f) + 720.0f;
    (&_ENT(bolt)->s.apos)->trDelta[1] = 0.0f;
    (&_ENT(bolt)->s.apos)->trDelta[2] = flrand(-45.0f, 45.0f) + 360.0f;

    VectorCopy(start, (_ENT(bolt)->r.currentOrigin));

    VectorCopy((&_ENT(bolt)->s.apos)->trBase, (_ENT(bolt)->r.currentAngles));

    if (BG_IsPlacedExplosive(weapDef))
        G_PlaceExplosive(self, bolt, weapDef);

    return bolt;
}

gentity_t *fire_rocket(gentity_t *self, vec_t *start, vec_t *dir)
{
    gentity_t *bolt;
    WeaponDef *weapDef;

    Vec3Normalize(dir);

    weapDef = BG_GetWeaponDef((_ENT(self)->s.weapon));

    bolt = G_Spawn();

    Scr_SetString(&(_ENT(bolt)->classname), scr_const.rocket);

    (_ENT(bolt)->nextthink) = LEVEL_TIME + 30000;

    (_ENT(bolt)->handler) = 8;

    (_ENT(bolt)->s.eType) = 4;

    (_ENT(bolt)->s.eFlags) |= 0x400;

    (_ENT(bolt)->r.svFlags) = 8;

    (_ENT(bolt)->s.weapon) = (_ENT(self)->s.weapon);

    (_ENT(bolt)->r.ownerNum) = (_ENT(self)->s.number);

    (_ENT(bolt)->parent) = COD2_GEntityHandle(self);

    (_ENT(bolt)->damage) = weapDef->damage;

    (_ENT(bolt)->clipmask) = 0x2802891;

    (_ENT(bolt)->s.time) = LEVEL_TIME + 50;

    (&_ENT(bolt)->s.pos)->trType = 2;

    (&_ENT(bolt)->s.pos)->trTime = LEVEL_TIME - 50;

    VectorCopy(start, (&_ENT(bolt)->s.pos)->trBase);

    VectorScale(dir, (float)weapDef->iProjectileSpeed, (&_ENT(bolt)->s.pos)->trDelta);

    SnapVector((&_ENT(bolt)->s.pos)->trDelta);

    VectorCopy(start, (_ENT(bolt)->r.currentOrigin));

    vectoangles(dir, (_ENT(bolt)->r.currentAngles));
    G_SetAngle(bolt, (_ENT(bolt)->r.currentAngles));

    (_ENT(bolt)->grenade.time) = (float)((WeaponDef *)weapDef)->destabilizeDistance / (float)weapDef->iProjectileSpeed * 1000.0f;

    (_ENT(bolt)->flags) |= ((_ENT(self)->flags) & GMISSILE_FL_TURRET);

    return bolt;
}

static void G_MissileLandAngles(gentity_t *ent, trace_t *trace, vec_t *vAngles, qboolean bForceAlign)
{
    int hitTime;
    float fSurfacePitch;
    float fAngleDelta;
    float fAbsAngDelta;

    hitTime = LEVEL_PREVIOUSTIME + (int)((float)(LEVEL_TIME - LEVEL_PREVIOUSTIME) * trace->fraction);

    BG_EvaluateTrajectory((&_ENT(ent)->s.apos), hitTime, vAngles);

    if (trace->normal[2] > 0.1f) {

        fSurfacePitch = PitchForYawOnNormal(vAngles[1], trace->normal);
        fAngleDelta = AngleSubtract(fSurfacePitch, vAngles[0]);
        fAbsAngDelta = fAngleDelta < 0 ? -fAngleDelta : fAngleDelta;

        if (!bForceAlign) {

            VectorCopy(vAngles, (&_ENT(ent)->s.apos)->trBase);
            (&_ENT(ent)->s.apos)->trTime = hitTime;

            if (fAbsAngDelta < 80.0f) {

                float rnd = randomf();
                (&_ENT(ent)->s.apos)->trDelta[0] = -((&_ENT(ent)->s.apos)->trDelta[0] * (rnd * 0.3f + 0.85f));
            } else {

                float rnd = randomf();
                (&_ENT(ent)->s.apos)->trDelta[0] = (&_ENT(ent)->s.apos)->trDelta[0] * (rnd * 0.3f + 0.85f);
            }
        }

        vAngles[0] = AngleNormalize180(vAngles[0]);

        if (bForceAlign || fAbsAngDelta < 45.0f) {

            float absYaw = vAngles[0] < 0 ? -vAngles[0] : vAngles[0];
            if (absYaw > 90.0f) {
                vAngles[0] = AngleNormalize360(fSurfacePitch + 180.0f);
            } else {
                vAngles[0] = AngleNormalize360(fSurfacePitch);
            }
            return;
        }

        if (fAbsAngDelta < 80.0f) {

            vAngles[0] = AngleNormalize360(vAngles[0] + fAngleDelta * 0.25f);
            return;
        }

        vAngles[0] = AngleNormalize360(vAngles[0]);
        return;
    }

    if (bForceAlign) {
        return;
    }

    (&_ENT(ent)->s.apos)->trDelta[0] = AngleNormalize360((&_ENT(ent)->s.apos)->trDelta[0] + (float)((rand() & 0x7F) - 0x3F));
}

static qboolean G_BounceMissile(gentity_t *ent, trace_t *trace)
{
    WeaponDef *weapDef;
    int contents;
    int surfType;
    vec3_t velocity;
    vec3_t vAngles;
    float dot;
    float speed;

    weapDef = BG_GetWeaponDef((_ENT(ent)->s.weapon));

    contents = SV_PointContents((_ENT(ent)->r.currentOrigin), -1, 0x20);

    surfType = (trace->surfaceFlags & 0x1F00000) >> 20;

    int hitTime = LEVEL_PREVIOUSTIME + (int)((float)(LEVEL_TIME - LEVEL_PREVIOUSTIME) * trace->fraction);
    BG_EvaluateTrajectoryDelta((&_ENT(ent)->s.pos), hitTime, velocity);

    dot = DotProduct(velocity, trace->normal);

    VectorMA(velocity, -2.0f * dot, trace->normal, (&_ENT(ent)->s.pos)->trDelta);

    if ((double)trace->normal[2] > 0.7) {
        (_ENT(ent)->s.groundEntityNum) = trace->entityNum;
    }

    if ((_ENT(ent)->s.eFlags) & GMISSILE_EF_GRENADE_BOUNCE) {

        speed = VectorLength(velocity);

        if (speed > 0.0f && dot < 0.0f) {

            float parallelBounce = weapDef->parallelBounce[surfType];
            float perpBounce = weapDef->perpendicularBounce[surfType];

            float bounceFactor = parallelBounce + (perpBounce - parallelBounce) * (dot / -speed);

            VectorScale((&_ENT(ent)->s.pos)->trDelta, bounceFactor, (&_ENT(ent)->s.pos)->trDelta);
        }

        if ((double)trace->normal[2] > 0.7) {

            float newSpeed = VectorLength((&_ENT(ent)->s.pos)->trDelta);
            if (newSpeed < 20.0f) {

                G_SetOrigin(ent, (_ENT(ent)->r.currentOrigin));

                G_MissileLandAngles(ent, trace, vAngles, 1);
                G_SetAngle(ent, vAngles);
                return 0;
            }
        }
    }

    {
        float nudge_z = trace->normal[2] * 0.1f;
        if (nudge_z > 0.0f)
            nudge_z = 0.0f;

        (_ENT(ent)->r.currentOrigin)
        [0] += trace->normal[0] * 0.1f;
        (_ENT(ent)->r.currentOrigin)
        [1] += trace->normal[1] * 0.1f;
        (_ENT(ent)->r.currentOrigin)
        [2] += nudge_z;
    }

    VectorCopy((_ENT(ent)->r.currentOrigin), (&_ENT(ent)->s.pos)->trBase);

    (&_ENT(ent)->s.pos)->trTime = LEVEL_TIME;

    G_MissileLandAngles(ent, trace, vAngles, 0);

    VectorCopy(vAngles, (&_ENT(ent)->s.apos)->trBase);

    (&_ENT(ent)->s.apos)->trTime = LEVEL_TIME;

    if (contents) {
        return 0;
    }

    {
        vec3_t velChange;
        VectorSubtract((&_ENT(ent)->s.pos)->trDelta, velocity, velChange);
        float changeSpeed = VectorLength(velChange);
        if (changeSpeed <= 100.0f) {
            return 0;
        }
        return 1;
    }
}

void G_RunMissile(gentity_t *ent)
{
    vec3_t origin;
    vec3_t vOldOrigin;
    vec3_t dir;
    vec3_t endpos;
    vec3_t trNormal;
    trace_t tr;
    trace_t trDown;
    float fraction;
    int methodOfDeath;
    int hitClient;
    WeaponDef *weapDef;

    if (ent->flags & GMISSILE_FL_HELD) {
        int clientNum = ent->s.otherEntityNum;
        gentity_t *holder = (unsigned)clientNum < 64 ? &g_entities[clientNum] : NULL;
        if (holder && holder->r.inuse && holder->client && holder->health > 0 &&
            holder->client->sess.connected == 2 && G_GetHeldGrenade(holder) == ent &&
            (holder->client->ps.pm_flags & 0x10)) {
            G_GetPlayerViewOrigin(holder, origin);
            G_SetOrigin(ent, origin);
            SV_LinkEntity(ent);
            if (ent->nextthink <= LEVEL_TIME)
                G_GrenadeCookOff(holder);
            return;
        }
        /* Death/disconnect drops the live grenade; the fuse keeps burning. */
        if ((unsigned)clientNum < 64 && heldGrenades[clientNum].missile == ent)
            heldGrenades[clientNum].missile = NULL;
        ent->flags &= ~GMISSILE_FL_HELD;
        ent->r.svFlags &= ~1;
        ent->s.otherEntityNum = 1023;
        ent->s.pos.trType = 5;
        ent->s.pos.trTime = LEVEL_TIME;
        VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
        VectorClear(ent->s.pos.trDelta);
    }

    weapDef = BG_GetWeaponDef(ent->s.weapon);
    if (BG_IsPlacedExplosive(weapDef) && G_RunPlacedExplosive(ent, weapDef))
        return;

    if ((&_ENT(ent)->s.pos)->trType == 0 && (_ENT(ent)->s.groundEntityNum) != 0x3FE) {

        VectorCopy((_ENT(ent)->r.currentOrigin), origin);
        origin[2] -= 1.5f;

        G_LocationalTrace(&tr, (_ENT(ent)->r.currentOrigin), origin,
                          (_ENT(ent)->r.ownerNum), (_ENT(ent)->clipmask), pPriorityMap);
        if (tr.startsolid) {

            tr.fraction = 0.0f;
            VectorSubtract((_ENT(ent)->r.currentOrigin), origin, dir);
            Vec3NormalizeTo(dir, tr.normal);
        }

        if (tr.fraction == 1.0f) {

            (&_ENT(ent)->s.pos)->trType = 5;
            (&_ENT(ent)->s.pos)->trTime = LEVEL_TIME;
            (&_ENT(ent)->s.pos)->trDuration = 0;
            VectorCopy((_ENT(ent)->r.currentOrigin), (&_ENT(ent)->s.pos)->trBase);
            VectorClear((&_ENT(ent)->s.pos)->trDelta);
        }
    }

    VectorCopy((_ENT(ent)->r.currentOrigin), vOldOrigin);

    BG_EvaluateTrajectory((&_ENT(ent)->s.pos), LEVEL_TIME, origin);

    VectorSubtract(origin, (_ENT(ent)->r.currentOrigin), dir);

    float dirLen = Vec3Normalize(dir);
    if (dirLen == 0.0f) {
        goto run_think_check;
    }

    {
        float absVelZ = (&_ENT(ent)->s.pos)->trDelta[2];
        if (absVelZ < 0)
            absVelZ = -absVelZ;

        if (absVelZ > 30.0f) {

            if (!SV_PointContents((_ENT(ent)->r.currentOrigin), -1, 0x20)) {

                G_LocationalTrace(&tr, (_ENT(ent)->r.currentOrigin), origin,
                                  (_ENT(ent)->r.ownerNum), (_ENT(ent)->clipmask) | 0x20, pPriorityMap);

                if (tr.startsolid) {
                    tr.fraction = 0.0f;
                    VectorSubtract((_ENT(ent)->r.currentOrigin), origin, dir);
                    Vec3NormalizeTo(dir, tr.normal);
                }

                goto after_trace;
            }
        }
    }

    G_LocationalTrace(&tr, (_ENT(ent)->r.currentOrigin), origin,
                      (_ENT(ent)->r.ownerNum), (_ENT(ent)->clipmask), pPriorityMap);
    if (tr.startsolid) {
        tr.fraction = 0.0f;
        VectorSubtract((_ENT(ent)->r.currentOrigin), origin, dir);
        Vec3NormalizeTo(dir, tr.normal);
    }

after_trace:

    if ((tr.surfaceFlags & 0x1F00000) == 0x1400000) {

        vec3_t splashDir;
        Vec3NormalizeTo((&_ENT(ent)->s.pos)->trDelta, splashDir);

        if (splashDir[2] < 0.0f) {
            splashDir[2] = -splashDir[2];
        }

        gentity_t *tent = G_TempEntity((_ENT(ent)->r.currentOrigin), 0xB6);

        ((gentity_t *)tent)->s.eventParm = DirToByte(tr.normal);
        ((tent)->s.scale) = DirToByte(splashDir);

        ((tent)->s.surfType) = (tr.surfaceFlags & 0x1F00000) >> 20;

        ((gentity_t *)tent)->s.otherEntityNum = (_ENT(ent)->s.number);

        G_LocationalTrace(&tr, (_ENT(ent)->r.currentOrigin), origin,
                          (_ENT(ent)->r.ownerNum), (_ENT(ent)->clipmask), pPriorityMap);
        if (tr.startsolid) {
            tr.fraction = 0.0f;
            VectorSubtract((_ENT(ent)->r.currentOrigin), origin, dir);
            Vec3NormalizeTo(dir, tr.normal);
        }
    }

    methodOfDeath = HANDLER_MOD((_ENT(ent)->handler));

    if (methodOfDeath == 3) {

        gentity_t *other = G_ENTITY(tr.entityNum);

        if (((other)->flags) < 0) {

            int savedHealth = ((other)->r.contents);
            ((other)->r.contents) = 0;

            G_LocationalTrace(&tr, (_ENT(ent)->r.currentOrigin), origin,
                              (_ENT(ent)->r.ownerNum), (_ENT(ent)->clipmask), pPriorityMap);
            if (tr.startsolid) {
                tr.fraction = 0.0f;
                VectorSubtract((_ENT(ent)->r.currentOrigin), origin, dir);
                Vec3NormalizeTo(dir, tr.normal);
            }

            ((other)->r.contents) = savedHealth;
        }
    }

    fraction = tr.fraction;
    LerpPosition((_ENT(ent)->r.currentOrigin), origin, fraction, endpos);

    VectorCopy(endpos, (_ENT(ent)->r.currentOrigin));

    if ((_ENT(ent)->s.eFlags) & GMISSILE_EF_GRENADE_BOUNCE) {

        if (fraction != 1.0f || (fraction == 1.0f && tr.normal[2] > 0.7f)) {

            VectorCopy(endpos, origin);
            origin[0] = (_ENT(ent)->r.currentOrigin)[0];
            origin[1] = ((ent)->r.currentOrigin[1]);
            origin[2] = ((ent)->r.currentOrigin[2]) - 1.5f;

            G_LocationalTrace(&trDown, (_ENT(ent)->r.currentOrigin), origin,
                              (_ENT(ent)->r.ownerNum), (_ENT(ent)->clipmask), pPriorityMap);
            if (trDown.startsolid) {
                trDown.fraction = 0.0f;
                VectorSubtract((_ENT(ent)->r.currentOrigin), origin, dir);
                Vec3NormalizeTo(dir, trDown.normal);
            }

            if (trDown.fraction != 1.0f && trDown.entityNum == 0x3FE) {

                tr = trDown;
                fraction = tr.fraction;

                LerpPosition((_ENT(ent)->r.currentOrigin), origin, fraction, endpos);

                ((ent)->s.origin2[0]) += endpos[2] + 1.5f - ((ent)->r.currentOrigin[2]);

                VectorCopy(endpos, (_ENT(ent)->r.currentOrigin));

                ((ent)->r.currentOrigin[2]) += 1.5f;
            }
        }
    }

    SV_LinkEntity(ent);

    weapDef = BG_GetWeaponDef((_ENT(ent)->s.weapon));

    if (methodOfDeath == 3) {
        G_GrenadeTouchTriggerDamage(ent, vOldOrigin, (_ENT(ent)->r.currentOrigin),
                                    weapDef->iExplosionRadius, 3);
    }

    if (tr.fraction == 1.0f) {

        float speed = VectorLength((&_ENT(ent)->s.pos)->trDelta);
        if (speed == 0.0f) {
            goto run_think;
        }

        (_ENT(ent)->s.groundEntityNum) = 0x3FF;

        if (weapDef->weapClass == 2 && !((_ENT(ent)->flags) & GMISSILE_FL_TURRET)) {

            int totalTime = (int)(_ENT(ent)->grenade.time) + (&_ENT(ent)->s.pos)->trTime;
            if (totalTime < LEVEL_TIME) {
                goto run_think;
            }

            WeaponDef *weaponDef = BG_GetWeaponDef((_ENT(ent)->s.weapon));

            VectorCopy((&_ENT(ent)->s.pos)->trDelta, dir);
            Vec3Normalize(dir);

            float scale = tanf((float)weaponDef->fAdsAimPitch * 0.017453292519943295f);

            vec3_t perturbation;
            int i;
            for (i = 0; i < 3; i++) {
                perturbation[i] = flrand(-1.0f, 1.0f);
            }

            VectorScale(perturbation, scale, perturbation);
            VectorAdd(dir, perturbation, dir);
            Vec3Normalize(dir);

            VectorScale(dir, (float)weaponDef->iProjectileSpeed, (&_ENT(ent)->s.pos)->trDelta);

            VectorCopy((_ENT(ent)->r.currentOrigin), (&_ENT(ent)->s.pos)->trBase);

            vectoangles(dir, (_ENT(ent)->r.currentAngles));
            G_SetAngle(ent, (_ENT(ent)->r.currentAngles));

            (&_ENT(ent)->s.pos)->trTime = LEVEL_TIME;

            if ((_ENT(ent)->flags) & GMISSILE_FL_GUIDED) {

                (_ENT(ent)->grenade.time) *= ((WeaponDef *)weaponDef)->destabilizationTimeReductionRatio;
            } else {

                (_ENT(ent)->grenade.time) = 1000.0f * ((WeaponDef *)weaponDef)->destabilizationBaseTime;
            }

            (_ENT(ent)->flags) |= GMISSILE_FL_GUIDED;
        }

        goto run_think;
    }

    if (tr.surfaceFlags & 0x10) {
        G_FreeEntity(ent);
        return;
    }

    {
        gentity_t *other = G_ENTITY(tr.entityNum);

        (_ENT(ent)->s.surfType) = (tr.surfaceFlags & 0x1F00000) >> 20;

        if (weapDef->stickiness == WEAPSTICKINESS_ALL && !(_ENT(other)->client) && !tr.startsolid) {
            G_StickMissile(ent, &tr, endpos);
            goto run_think;
        }

        /* Use the same bounce flag set by fire_grenade. The FX lifetime flag
         * (0x10000) does not identify a bouncing projectile. Without this,
         * its trajectory keeps falling through the impact and detonates
         * below the map instead of reflecting off the collision plane. */
        if ((_ENT(other)->takedamage) || ((_ENT(ent)->s.eFlags) & GMISSILE_EF_GRENADE_BOUNCE)) {

            if (((_ENT(ent)->s.eFlags) & GMISSILE_EF_GRENADE_BOUNCE) || !(_ENT(other)->takedamage)) {

                gclient_t *otherClient = (_ENT(other)->client);
                if (otherClient) {

                    if (tr.surfaceFlags == 0) {
                        tr.surfaceFlags = 0x700000;
                    }
                }

                qboolean bounceResult = G_BounceMissile(ent, &tr);
                if (bounceResult && !tr.startsolid) {

                    G_AddEvent(ent, 0xBB, DirToByte(tr.normal));
                }

                if ((_ENT(ent)->s.eType) != 4) {
                    goto done;
                }
                goto run_think;
            }

            WeaponDef *hitWeapDef = BG_GetWeaponDef((_ENT(ent)->s.weapon));
            int hitMOD = HANDLER_MOD((_ENT(ent)->handler));

            hitClient = 0;

            if ((_ENT(ent)->damage)) {

                gentity_t *attacker;
                if ((_ENT(ent)->r.ownerNum) == 0x3FF) {
                    attacker = NULL;
                } else {
                    attacker = G_ENTITY((_ENT(ent)->r.ownerNum));
                }

                hitClient = LogAccuracyHit(other, attacker) ? 1 : 0;

                BG_EvaluateTrajectoryDelta((&_ENT(ent)->s.pos), LEVEL_TIME, dir);

                float speed = VectorLength(dir);

                if (speed != 0.0f) {

                } else {
                    dir[2] = 1.0f;
                }

                gentity_t *damageAttacker;
                if ((_ENT(ent)->r.ownerNum) == 0x3FF) {
                    damageAttacker = NULL;
                } else {
                    damageAttacker = G_ENTITY((_ENT(ent)->r.ownerNum));
                }

                G_Damage(other, ent, damageAttacker, dir, (_ENT(ent)->r.currentOrigin),
                         (_ENT(ent)->damage), 0, hitMOD, 0, 0);
            }

            if ((_ENT(ent)->damage)) {
                gentity_t *trigAttacker;
                if ((_ENT(ent)->r.ownerNum) == 0x3FF) {
                    trigAttacker = G_ENTITY(GMISSILE_ENTITYNUM_WORLD);
                } else {
                    trigAttacker = G_ENTITY((_ENT(ent)->r.ownerNum));
                }
                G_CheckHitTriggerDamage(trigAttacker, (_ENT(ent)->r.currentOrigin), endpos,
                                        (_ENT(ent)->damage), hitMOD);
            }

            {
                int didHitClient = hitClient;
                if (!didHitClient && tr.entityNum == 0) {
                    didHitClient = 0;
                }
                if (didHitClient || tr.partName != 0) {
                    didHitClient = 1;
                }

                int impactEvent = didHitClient ? 0xBD : 0xBE;
                G_AddEvent(ent, impactEvent, DirToByte(tr.normal));
            }

            (_ENT(ent)->s.surfType) = (tr.surfaceFlags & 0x1F00000) >> 20;

            (_ENT(ent)->freeAfterEvent) = 1;

            (_ENT(ent)->s.eType) = 0;
            (_ENT(ent)->s.eFlags) = ((_ENT(ent)->s.eFlags) ^ 2) | 0x20;

            (_ENT(ent)->flags) |= 0x800;

            SnapVectorTowards(endpos, (&_ENT(ent)->s.pos)->trBase);

            G_SetOrigin(ent, endpos);

            if (hitWeapDef->iExplosionRadius > 0) {
                int splashMod = HANDLER_SPLASHMOD((_ENT(ent)->handler));
                G_RadiusDamage(endpos, ent, COD2_GEntityFromHandle(_ENT(ent)->parent),
                               (float)hitWeapDef->iExplosionInnerDamage,
                               (float)hitWeapDef->iExplosionOuterDamage,
                               (float)hitWeapDef->iExplosionRadius,
                               ent, splashMod);
            }

            SV_LinkEntity(ent);
        }
    }

    if ((_ENT(ent)->s.eType) == 4) {
        goto run_think;
    }

done:
    return;

run_think_check:
    if (dirLen != 0.0f || (_ENT(ent)->s.eType) != 4) {
        goto done;
    }

run_think:
    G_RunThink(ent);
}
