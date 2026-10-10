#include "common_types.h"
#include "imports.h"
#include "bytematch.h"

extern unsigned char G_FreeEntity(gentity_t *ent);
extern unsigned char G_SetOrigin(gentity_t *ent, const vec_t *origin);
extern qboolean G_SpawnString(const char *key, const char *defaultString, const char **out);
extern qboolean G_SpawnFloat(const char *key, const char *defaultString, float *out);
extern qboolean G_SpawnInt(const char *key, const char *defaultString, int *out);
extern void Com_Error(int code, const char *fmt, ...);
extern DObj_s *Com_GetServerDObj(int entNum);
extern qboolean DObjSetControlTagAngles(DObj_s *obj, int *partBits, unsigned short tag, vec_t *angles);
extern void SV_UnlinkEntity(gentity_t *ent);
extern void SV_LinkEntity(gentity_t *ent);
extern void SetClientViewAngle(gentity_t *ent, vec_t *angles);
extern void BG_PlayerStateToEntityState(playerState_t *ps, gentity_t *ent, qboolean snap, qboolean forceSnap);
extern unsigned char G_AddEvent(gentity_t *ent, int event, int eventParm);
extern WeaponDef *BG_GetWeaponDef(int iWeapon);
extern float AngleSubtract(float a, float b);
extern unsigned char G_PlaySoundAlias(gentity_t *ent, int index);
extern float AngleNormalize180(float angle);
extern void YawVectors(const vec_t yaw, vec_t *forward, vec_t *right);
extern float Vec3Normalize(vec_t *v);
extern float Q_acos(float x);
extern void G_GeneralLink(gentity_t *ent);
extern DObjAnimMat_s *G_DObjGetLocalTagMatrix(gentity_t *ent, unsigned int tagName);
extern void AnglesToAxis(const vec_t *angles, vec_t *axis);
extern void MatrixTransformVector(const vec_t *in1, const vec_t *in2, vec_t *out);
extern void MatrixTransformVector43(const vec_t *in, const vec_t *mat, vec_t *out);
extern void G_LocationalTrace(trace_t *results, const vec_t *start, const vec_t *end, int passEntityNum, int contentmask, unsigned char *priorityMap);
extern unsigned char bulletPriorityMap[19];
extern float AngleNormalize360Accurate(float angle);
extern int G_GetWeaponIndexForName(const char *name);
extern void Scr_Error(const char *msg);
extern const char *va(const char *fmt, ...);
extern qboolean IsItemRegistered(unsigned int item);
extern SoundAlias G_SoundAliasIndex(const char *name);
extern void G_DObjUpdate(gentity_t *ent);
extern unsigned char G_SetAngle(gentity_t *ent, const vec_t *angles);
extern struct level_locals_t level;
extern struct bgs_t level_bgs;

#define g_entities ((gentity_t *)imp_g_entities)
extern void Com_Printf(const char *fmt, ...);
extern qboolean G_DObjGetWorldTagMatrix(gentity_t *ent, unsigned int tagName, vec3_t *tagMat);
extern void G_GetPlayerViewOrigin(const gentity_t *ent, vec_t *origin);
extern void G_GetPlayerViewDirection(const gentity_t *ent, vec_t *forward, vec_t *right, vec_t *up);
extern void G_TraceCapsule(trace_t *results, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentmask);
extern void Weapon_RocketLauncher_Fire(gentity_t *ent, float spread, weaponParms *wp);
extern void Bullet_Fire(gentity_t *attacker, float spread, weaponParms *wp, gentity_t *weaponEnt, int gametime);
extern float randomf(void);
extern const char *SL_ConvertToString(unsigned int stringValue);
extern float vectosignedyaw(const vec3_t vec);
extern void VectorAngleMultiply(vec3_t vec, float angle);
extern float RotationToYaw(const vec2_t rot);
extern void YawToAxis(float yaw, vec3_t axis[3]);
extern void MatrixMultiply43(const vec_t *in1, const vec_t *in2, vec_t *out);
extern void AxisToAngles(const vec_t *axis, vec_t *angles);
extern void ConvertQuatToMat(const DObjAnimMat *mat, float axis[3][3]);
extern void XAnimClearTreeGoalWeightsStrict(XAnimTree_s *tree, unsigned int animIndex, float blendTime);
extern int XAnimGetNumChildren(const XAnim_s *anims, unsigned int animIndex);
extern unsigned int XAnimGetChildAt(const XAnim_s *anims, unsigned int animIndex, unsigned int childIndex);
extern int XAnimSetGoalWeight(XAnimTree_s *tree, unsigned int animIndex, float goalWeight, float goalTime, float rate, unsigned int notifyName, unsigned int notifyType, int bRestart);
extern void XAnimCalcAbsDelta(XAnimTree_s *tree, unsigned int animIndex, float *rot, float *trans);
extern float XAnimGetWeight(const XAnimTree_s *tree, unsigned int animIndex);
extern const char *XAnimGetAnimDebugName(const XAnim_s *anims, unsigned int animIndex);

extern turretInfo_s turretInfo[32];

enum {
    GMISC_TURRET_STANCE_INVALID = -1,
    GMISC_TURRET_STANCE_CROUCH = 1,
    GMISC_TURRET_STANCE_PRONE = 2,
    GMISC_EV_STANCE_FORCE_STAND = 0x8c,
    GMISC_EV_STANCE_FORCE_CROUCH = 0x8d,
    GMISC_EV_STANCE_FORCE_PRONE = 0x8e,
    GMISC_ENTITYNUM_NONE = 0x3ff,
    GMISC_ENTITYNUM_WORLD = 0x3fe,
    GMISC_EF_FIRING = 0x40,
    GMISC_EF_TELEPORT_BIT = 0x2,
    GMISC_PMF_PRONE = 0x1,
    GMISC_PMF_DUCKED = 0x2,
    GMISC_EF_TURRET_PRONE = 0x100,
    GMISC_EF_TURRET_DUCK = 0x200,
    GMISC_EF_TURRET_ACTIVE = 0x300,
    GMISC_ENT_HANDLER_TURRET_INIT = 13,
    GMISC_ENT_HANDLER_TURRET = 14,
    GMISC_ET_TURRET = 9,
    GMISC_CONTENTS_SOLID = 0x1,
    GMISC_CONTENTS_NONCOLLIDING = 0x4,
    GMISC_CONTENTS_DONOTENTER = 0x200000,
    GMISC_TURRET_TRACE_MASK = 0x811,
    GMISC_MASK_PLAYERSOLID = 0x2810011,
    GMISC_PLAYERVIEWLOCK_FULL = 0x1,
    GMISC_PLAYERVIEWLOCK_WEAPONJITTER = 0x2,
    GMISC_BUTTON_ATTACK = 0x1,
    GMISC_PMOVE_HANDLER_SERVER = 0x1,
    GMISC_DEFAULT_VIEWHEIGHT = 60,
    GMISC_ANIM_TOGGLEBIT = 0x200,
    GMISC_EV_FIRE_WEAPON_MG42 = 0xaf,
    /* Eight units per firing millisecond: 5s firing, 8s full cooling. */
    GMISC_TURRET_HEAT_MAX = 40000,
    GMISC_TURRET_SHOT_HEAT_SCALE = 8,
    GMISC_TURRET_COOLING_PER_MS = 5
};

static const vec3_t g_misc_vec3_origin = { 0.0f, 0.0f, 0.0f };

/* Keep heat separate from the retail turretInfo ABI. It belongs to the gun,
 * so changing users cannot bypass its cooling period. time2 is replicated. */
typedef struct { int heat, lastTime; qboolean overheated; } turretHeat_t;
static turretHeat_t turretHeat[32];

static turretHeat_t *turret_UpdateHeat(gentity_t *self, qboolean firing)
{
    int index = self->pTurretInfo - turretInfo;
    turretHeat_t *heat = &turretHeat[index];
    int elapsed = level.time - heat->lastTime;
    if (elapsed < 0) elapsed = 0;
    if (elapsed > 8000) elapsed = 8000;
    if (!firing || heat->overheated) {
        heat->heat -= elapsed * GMISC_TURRET_COOLING_PER_MS;
        if (heat->heat <= 0) {
            heat->heat = 0;
            heat->overheated = 0;
        }
    }
    heat->lastTime = level.time;
    self->s.time2 = heat->heat * 100 / GMISC_TURRET_HEAT_MAX | (heat->overheated ? 0x100 : 0);
    return heat;
}

#define SCR_CONST() ((const scr_const_t *)imp_scr_const)

static inline float GMisc_Fabs(float value)
{
    return value < 0.0f ? -value : value;
}

static inline float GMisc_Clamp(float value, float minValue, float maxValue)
{
    if (value < minValue) {
        return minValue;
    }

    if (value > maxValue) {
        return maxValue;
    }

    return value;
}

static inline void GMisc_VectorCopy(const vec_t *src, vec_t *dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

static inline void GMisc_VectorSubtract(const vec_t *a, const vec_t *b, vec_t *out)
{
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

static inline float GMisc_DotProduct(const vec_t *a, const vec_t *b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline void GMisc_VectorMA(const vec_t *base, float scale, const vec_t *dir, vec_t *out)
{
    out[0] = base[0] + scale * dir[0];
    out[1] = base[1] + scale * dir[1];
    out[2] = base[2] + scale * dir[2];
}

static inline void GMisc_Vec3Lerp(const vec_t *start, const vec_t *end, float fraction, vec_t *out)
{
    out[0] = start[0] + (end[0] - start[0]) * fraction;
    out[1] = start[1] + (end[1] - start[1]) * fraction;
    out[2] = start[2] + (end[2] - start[2]) * fraction;
}

static inline float GMisc_XAnimGoalTime(const XAnimTree_s *tree, unsigned int animIndex, float goalWeight)
{
    float delta;

    delta = GMisc_Fabs(XAnimGetWeight(tree, animIndex) - goalWeight) * (1000.0f / level.frametime);
    return delta > 0.0f ? 1.0f / delta : 0.0f;
}

static inline void GMisc_XAnimSetGoalWeight(XAnimTree_s *tree, unsigned int animIndex, float goalWeight, float goalTime)
{
    XAnimSetGoalWeight(tree, animIndex, goalWeight, goalTime, 1.0f, 0, 0, 0);
}

void SP_info_null(gentity_t *self);
void SP_info_notnull(gentity_t *self);
void SP_light(gentity_t *self);
void SP_misc_model(gentity_t *ent);
void SP_corona(gentity_t *ent);
void G_InitTurrets(void);
void turret_think(gentity_t *self);
void turret_controller(gentity_t *self, int *partBits);
void TeleportPlayer(gentity_t *player, vec_t *origin, vec_t *angles);
void G_ClientStopUsingTurret(gentity_t *self);
void G_FreeTurret(gentity_t *self);
void turret_think_init(gentity_t *self);
qboolean G_IsTurretUsable(gentity_t *self, gentity_t *owner);
void turret_use(gentity_t *self, gentity_t *owner, gentity_t *activator);
void G_SpawnTurret(gentity_t *self, const char *weaponinfoname);
void SP_turret(gentity_t *self);
void turret_think_client(gentity_t *self);

void SP_info_null(gentity_t *self)
{
    G_FreeEntity(self);
}

void SP_info_notnull(gentity_t *self)
{
    G_SetOrigin(self, self->r.currentOrigin);
}

void SP_light(gentity_t *self)
{
    G_FreeEntity(self);
}

void SP_misc_model(gentity_t *ent)
{
    G_FreeEntity(ent);
}

void SP_corona(gentity_t *ent)
{
    G_FreeEntity(ent);
}

void G_InitTurrets(void)
{
    int i;
    memset(turretHeat, 0, sizeof(turretHeat));
    for (i = 0; i < 32; i++)
        turretInfo[i].inuse = 0;
}

static qboolean turret_UpdateTargetAngles(gentity_t *self, const float *desiredAngles, qboolean bManned)
{
    vec2_t downAngles;
    vec2_t speed;
    turretInfo_s *info;
    qboolean complete;
    float delta;
    int i;

    info = self->pTurretInfo;
    complete = 1;

    downAngles[1] = self->s.angles2[0];
    self->s.angles2[0] = downAngles[1] + self->s.angles2[2];

    if (bManned) {
        WeaponDef *weapDef = BG_GetWeaponDef(self->s.weapon);
        speed[0] = weapDef->maxTurnSpeed[0];
        speed[1] = weapDef->maxTurnSpeed[1];
    } else {
        speed[0] = 200.0f;
        speed[1] = 200.0f;
    }

    if ((info->flags & 0x200) && (info->flags & 0x100) && speed[0] < 360.0f) {
        speed[0] = 360.0f;
    }

    for (i = 0; i < 2; ++i) {
        speed[i] = speed[i] * 0.050000001f;
        delta = AngleSubtract(desiredAngles[i], self->s.angles2[i]);

        if (delta > speed[i]) {
            complete = 0;
            delta = speed[i];
        } else if (-speed[i] > delta) {
            complete = 0;
            delta = -speed[i];
        }

        self->s.angles2[i] += delta;
    }

    downAngles[0] = self->s.angles2[0];
    self->s.angles2[2] = downAngles[0];

    if (info->flags & 0x200) {
        if (info->flags & 0x400) {
            if (((info)->pitchCap) > self->s.angles2[0]) {
                downAngles[0] = ((info)->pitchCap);
            } else {
                info->flags &= ~0x100u;
            }
        } else if (self->s.angles2[0] > ((info)->pitchCap)) {
            downAngles[0] = ((info)->pitchCap);
        } else {
            info->flags &= ~0x100u;
        }
    }

    delta = AngleSubtract(downAngles[0], downAngles[1]);
    if (delta > speed[0]) {
        complete = 0;
        delta = speed[0];
    } else if (-speed[0] > delta) {
        complete = 0;
        delta = -speed[0];
    }

    self->s.angles2[0] = downAngles[1] + delta;
    self->s.angles2[2] = self->s.angles2[2] - self->s.angles2[0];
    return complete;
}

static void turret_UpdateSound(gentity_t *self)
{
    turretInfo_s *info;

    info = self->pTurretInfo;
    self->s.loopSound = 0;

    if (info->fireSndDelay <= 0) {
        return;
    }

    self->s.loopSound = (unsigned char)info->fireSnd;
    info->fireSndDelay -= 50;
    if (info->fireSndDelay > 0 || !info->stopSnd) {
        return;
    }

    self->s.loopSound = 0;
    G_PlaySoundAlias(self, info->stopSnd);
}

static qboolean turret_ReturnToDefaultPos(gentity_t *self, qboolean bManned)
{
    vec2_t desiredAngles;
    turretInfo_s *info;

    info = self->pTurretInfo;
    desiredAngles[0] = bManned ? 0.0f : info->dropPitch;
    desiredAngles[1] = 0.0f;
    return turret_UpdateTargetAngles(self, desiredAngles, bManned);
}

static qboolean turret_behind(gentity_t *self, gentity_t *other)
{
    vec3_t dir;
    vec3_t forward;
    turretInfo_s *info;
    float angle;
    float centerYaw;
    float dot;
    float minYaw;
    float yawSpan;

    info = self->pTurretInfo;
    minYaw = self->r.currentAngles[1] + info->arcmin[1];
    yawSpan = ((info->arcmax[1] < 0.0f ? -info->arcmax[1] : info->arcmax[1]) +
               (info->arcmin[1] < 0.0f ? -info->arcmin[1] : info->arcmin[1])) *
              0.5f;
    centerYaw = AngleNormalize180(minYaw + yawSpan);

    YawVectors(centerYaw, forward, 0);
    Vec3Normalize(forward);

    dir[0] = self->r.currentOrigin[0] - other->r.currentOrigin[0];
    dir[1] = self->r.currentOrigin[1] - other->r.currentOrigin[1];
    dir[2] = 0.0f;
    Vec3Normalize(dir);

    dot = forward[0] * dir[0] + forward[1] * dir[1] + forward[2] * dir[2];
    if (dot < -1.0f) {
        dot = -1.0f;
    } else if (dot > 1.0f) {
        dot = 1.0f;
    }

    angle = Q_acos(dot) * 57.29577951308232f;
    return yawSpan >= angle;
}

void turret_think(gentity_t *self)
{
    gentity_t *heatOwner = &((gentity_t *)imp_g_entities)[self->r.ownerNum];
    turret_UpdateHeat(self, heatOwner->client && (heatOwner->client->buttons & GMISC_BUTTON_ATTACK));
    self->nextthink = level.time + 50;

    if (self->tagInfo) {
        G_GeneralLink(self);
    }

    if (!((gentity_t *)imp_g_entities)[self->r.ownerNum].client) {
        turret_UpdateSound(self);
        self->s.eFlags &= ~GMISC_EF_FIRING;
        turret_ReturnToDefaultPos(self, 0);
    }
}

void turret_controller(gentity_t *self, int *partBits)
{
    vec3_t angles;
    DObj_s *obj;
    const scr_const_t *scr;
    vec_t *anglesPtr = angles;

    angles[1] = self->s.angles2[1];
    angles[0] = self->s.angles2[0];
    angles[2] = 0;

    obj = Com_GetServerDObj(self->s.number);
    scr = SCR_CONST();

    DObjSetControlTagAngles(obj, partBits, scr->tag_aim, anglesPtr);
    DObjSetControlTagAngles(obj, partBits, scr->tag_aim_animated, anglesPtr);

    angles[0] = self->s.angles2[2];
    angles[1] = 0;

    DObjSetControlTagAngles(obj, partBits, scr->tag_flash, anglesPtr);
}

void TeleportPlayer(gentity_t *player, vec_t *origin, vec_t *angles)
{
    unsigned char linked;
    playerState_t *ps;
    vec_t *o;
    vec_t *co;

    linked = player->r.linked;

    SV_UnlinkEntity(player);

    ps = &player->client->ps;
    o = ps->origin;
    o[0] = origin[0];
    o[1] = origin[1];
    o[2] = origin[2];

    ps = &player->client->ps;
    ps->origin[2] += 1.0f;

    ps = &player->client->ps;
    ps->eFlags ^= 2;

    SetClientViewAngle(player, angles);

    BG_PlayerStateToEntityState(&player->client->ps, player, 1, 1);

    co = player->r.currentOrigin;
    ps = &player->client->ps;
    o = ps->origin;
    co[0] = o[0];
    co[1] = o[1];
    co[2] = o[2];

    if (linked)
        SV_LinkEntity(player);
}

void G_ClientStopUsingTurret(gentity_t *self)
{
    turretInfo_s *pTurretInfo;
    gentity_t *owner;

    pTurretInfo = self->pTurretInfo;
    owner = &g_entities[self->r.ownerNum];

    pTurretInfo->fireSndDelay = 0;
    self->s.loopSound = 0;

    if (pTurretInfo->prevStance != GMISC_TURRET_STANCE_INVALID) {
        if (pTurretInfo->prevStance == GMISC_TURRET_STANCE_PRONE) {
            G_AddEvent(owner, GMISC_EV_STANCE_FORCE_PRONE, 0);
        } else if (pTurretInfo->prevStance == GMISC_TURRET_STANCE_CROUCH) {
            G_AddEvent(owner, GMISC_EV_STANCE_FORCE_CROUCH, 0);
        } else {
            G_AddEvent(owner, GMISC_EV_STANCE_FORCE_STAND, 0);
        }

        pTurretInfo->prevStance = GMISC_TURRET_STANCE_INVALID;
    }

    TeleportPlayer(owner, pTurretInfo->userOrigin, owner->r.currentAngles);

    owner->client->ps.eFlags &= ~0x300;
    owner->client->ps.viewlocked = 0;
    owner->client->ps.viewlocked_entNum = GMISC_ENTITYNUM_NONE;
    owner->active = 0;
    owner->s.otherEntityNum = 0;

    self->active = 0;
    self->r.ownerNum = GMISC_ENTITYNUM_NONE;
    pTurretInfo->flags &= ~0x800u;
}

void G_FreeTurret(gentity_t *self)
{
    if (((gentity_t *)imp_g_entities)[self->r.ownerNum].client) {
        G_ClientStopUsingTurret(self);
    }

    self->active = 0;
    self->pTurretInfo->inuse = 0;
    self->pTurretInfo = 0;
}

void turret_think_init(gentity_t *self)
{
    float baseMtx[4][3];
    float mtx[3][3];
    vec3_t dir;
    vec3_t start;
    vec3_t end;
    vec3_t transDir;
    vec3_t angles;
    trace_t trace;
    DObjAnimMat_s *aimMtx;
    DObjAnimMat_s *weaponMtx;
    turretInfo_s *info;
    int i;

    const scr_const_t *scr;

    info = self->pTurretInfo;
    self->handler = GMISC_ENT_HANDLER_TURRET;
    self->nextthink = level.time + 50;

    scr = SCR_CONST();
    Com_Printf("[ckpt] turret_think_init before tag_aim\n");
    aimMtx = G_DObjGetLocalTagMatrix(self, scr->tag_aim);
    Com_Printf("[ckpt] turret_think_init after tag_aim aimMtx=%p\n", (void *)aimMtx);
    if (!aimMtx) {
        return;
    }

    weaponMtx = G_DObjGetLocalTagMatrix(self, scr->tag_butt);
    if (!weaponMtx) {
        return;
    }

    AnglesToAxis(self->r.currentAngles, (vec_t *)baseMtx);
    baseMtx[3][0] = self->r.currentOrigin[0];
    baseMtx[3][1] = self->r.currentOrigin[1];
    baseMtx[3][2] = self->r.currentOrigin[2];

    dir[0] = weaponMtx->trans[0] - aimMtx->trans[0];
    dir[1] = weaponMtx->trans[1] - aimMtx->trans[1];
    dir[2] = weaponMtx->trans[2] - aimMtx->trans[2];

    MatrixTransformVector43(aimMtx->trans, (vec_t *)baseMtx, start);

    for (i = 0; i <= 30; ++i) {
        angles[0] = -3.0f * (float)i;
        angles[1] = 0.0f;
        angles[2] = 0.0f;

        AnglesToAxis(angles, (vec_t *)mtx);
        MatrixTransformVector(dir, (vec_t *)mtx, transDir);

        transDir[0] += aimMtx->trans[0];
        transDir[1] += aimMtx->trans[1];
        transDir[2] += aimMtx->trans[2];

        MatrixTransformVector43(transDir, (vec_t *)baseMtx, end);
        G_LocationalTrace(&trace, start, end, self->s.number, GMISC_TURRET_TRACE_MASK, (unsigned char *)imp_bulletPriorityMap);

        if (trace.fraction < 1.0f) {
            info->dropPitch = angles[0];
            return;
        }
    }
}

qboolean G_IsTurretUsable(gentity_t *self, gentity_t *owner)
{
    if (self->active || !self->pTurretInfo) {
        return 0;
    }

    if (!turret_behind(self, owner)) {
        return 0;
    }

    if (owner->client->ps.grenadeTimeLeft) {
        return 0;
    }

    if (owner->client->ps.groundEntityNum == GMISC_ENTITYNUM_NONE) {
        return 0;
    }

    return 1;
}

void turret_use(gentity_t *self, gentity_t *owner, gentity_t *activator)
{
    playerState_t *ps;
    turretInfo_s *info;
    float pitch;
    float yaw;

    (void)activator;

    info = self->pTurretInfo;
    ps = &owner->client->ps;

    owner->active = 1;
    self->active = 1;
    self->r.ownerNum = owner->s.number;

    ps->viewlocked = 1;
    ps->viewlocked_entNum = self->s.number;

    info->flags |= 0x800u;

    {
        vec_t *uo = info->userOrigin;
        vec_t *co = owner->r.currentOrigin;
        uo[0] = co[0];
        uo[1] = co[1];
        uo[2] = co[2];
    }

    owner->s.otherEntityNum = self->s.number;
    self->s.otherEntityNum = owner->s.number;

    if (ps->pm_flags & GMISC_PMF_PRONE) {
        info->prevStance = GMISC_TURRET_STANCE_PRONE;
    } else {
        info->prevStance = (ps->pm_flags & GMISC_PMF_DUCKED) ? GMISC_TURRET_STANCE_CROUCH : 0;
    }

    if (info->stance == GMISC_TURRET_STANCE_PRONE) {
        ps->eFlags |= GMISC_EF_TURRET_PRONE;
        ps->eFlags &= ~GMISC_EF_TURRET_DUCK;
    } else if (info->stance == GMISC_TURRET_STANCE_CROUCH) {
        ps->eFlags |= GMISC_EF_TURRET_DUCK;
        ps->eFlags &= ~GMISC_EF_TURRET_PRONE;
    } else {
        ps->eFlags |= GMISC_EF_TURRET_ACTIVE;
    }

    pitch = AngleSubtract(ps->viewangles[0], self->r.currentAngles[0]);
    self->s.angles2[0] = pitch;
    {
        float hi = info->arcmax[0];
        float lo = info->arcmin[0];
        float clamped = (pitch - hi < 0.0f) ? pitch : hi;
        self->s.angles2[0] = (lo - pitch < 0.0f) ? clamped : lo;
    }

    yaw = AngleSubtract(ps->viewangles[1], self->r.currentAngles[1]);
    self->s.angles2[1] = yaw;
    {
        float hi = info->arcmax[1];
        float lo = info->arcmin[1];
        float clamped = (yaw - hi < 0.0f) ? yaw : hi;
        self->s.angles2[1] = (lo - yaw < 0.0f) ? clamped : lo;
    }
    self->s.angles2[2] = 0.0f;

    ps->viewAngleClampRange[0] = AngleSubtract(info->arcmax[0], info->arcmin[0]) * 0.5f;
    ps->viewAngleClampBase[0] = self->r.currentAngles[0] + info->arcmax[0];
    ps->viewAngleClampBase[0] = AngleNormalize360Accurate(ps->viewAngleClampBase[0] - ps->viewAngleClampRange[0]);

    ps->viewAngleClampRange[1] = AngleSubtract(info->arcmax[1], info->arcmin[1]) * 0.5f;
    ps->viewAngleClampBase[1] = self->r.currentAngles[1] + info->arcmax[1];
    ps->viewAngleClampBase[1] = AngleNormalize360Accurate(ps->viewAngleClampBase[1] - ps->viewAngleClampRange[1]);
}

void G_SpawnTurret(gentity_t *self, const char *weaponinfoname)
{
    turretInfo_s *info;
    WeaponDef *weapDef;
    int i;

    info = 0;
    for (i = 0; i < 32; ++i) {
        info = &turretInfo[i];
        if (!info->inuse) {
            break;
        }
    }

    if (i == 32) {
        Com_Error(1, "G_SpawnTurret: max number of turrets (%d) exceeded", 32);
    }

    memset(info, 0, sizeof(*info));
    memset(&turretHeat[i], 0, sizeof(turretHeat[i]));
    turretHeat[i].lastTime = level.time;
    self->pTurretInfo = info;
    info->inuse = 1;

    self->s.weapon = G_GetWeaponIndexForName(weaponinfoname);
    if (!self->s.weapon) {
        Com_Error(1, "bad weaponinfo '%s' specified for turret", weaponinfoname);
    }

    weapDef = BG_GetWeaponDef(self->s.weapon);
    if (weapDef->weapClass != WEAPCLASS_TURRET) {
        Scr_Error(va("G_SpawnTurret: weapon '%s' isn't a turret. This usually indicates that the weapon failed to load.", weaponinfoname));
    }

    if (!level.initializing && !IsItemRegistered((unsigned int)self->s.weapon)) {
        Scr_Error(va("turret '%s' not precached", weaponinfoname));
    }

    info->fireTime = 0;
    info->stance = weapDef->stance;
    info->prevStance = GMISC_TURRET_STANCE_INVALID;
    info->fireSndDelay = 0;

    if (weapDef->szFireLoopSound && *weapDef->szFireLoopSound) {
        info->fireSnd = G_SoundAliasIndex(weapDef->szFireLoopSound);
    }
    if (weapDef->szFireLoopSoundPlayer && *weapDef->szFireLoopSoundPlayer) {
        info->fireSndPlayer = G_SoundAliasIndex(weapDef->szFireLoopSoundPlayer);
    }
    if (weapDef->szFireStopSound && *weapDef->szFireStopSound) {
        info->stopSnd = G_SoundAliasIndex(weapDef->szFireStopSound);
    }
    if (weapDef->szFireStopSoundPlayer && *weapDef->szFireStopSoundPlayer) {
        info->stopSndPlayer = G_SoundAliasIndex(weapDef->szFireStopSoundPlayer);
    }

    if (!level.spawnVar.spawnVarsValid || !G_SpawnFloat("rightarc", "", &info->arcmin[1])) {
        info->arcmin[1] = weapDef->rightArc;
    }
    info->arcmin[1] = -info->arcmin[1];
    if (info->arcmin[1] > 0.0f) {
        info->arcmin[1] = 0.0f;
    }

    if (!level.spawnVar.spawnVarsValid || !G_SpawnFloat("leftarc", "", &info->arcmax[1])) {
        info->arcmax[1] = weapDef->leftArc;
    }
    if (info->arcmax[1] < 0.0f) {
        info->arcmax[1] = 0.0f;
    }

    if (!level.spawnVar.spawnVarsValid || !G_SpawnFloat("toparc", "", &info->arcmin[0])) {
        info->arcmin[0] = weapDef->topArc;
    }
    info->arcmin[0] = -info->arcmin[0];
    if (info->arcmin[0] > 0.0f) {
        info->arcmin[0] = 0.0f;
    }

    if (!level.spawnVar.spawnVarsValid || !G_SpawnFloat("bottomarc", "", &info->arcmax[0])) {
        info->arcmax[0] = weapDef->bottomArc;
    }
    if (info->arcmax[0] < 0.0f) {
        info->arcmax[0] = 0.0f;
    }

    info->dropPitch = -90.0f;

    if (!self->health) {
        self->health = 100;
    }

    if (!level.spawnVar.spawnVarsValid || !G_SpawnInt("damage", "0", &self->damage)) {
        self->damage = weapDef->damage;
    }
    if (self->damage < 0) {
        self->damage = 0;
    }

    if (!level.spawnVar.spawnVarsValid || !G_SpawnFloat("playerSpread", "1", &info->playerSpread)) {
        info->playerSpread = weapDef->playerSpread;
    }
    if (info->playerSpread < 0.0f) {
        info->playerSpread = 0.0f;
    }

    info->flags = 3;

    self->clipmask = GMISC_CONTENTS_SOLID;
    self->r.contents = GMISC_CONTENTS_NONCOLLIDING | GMISC_CONTENTS_DONOTENTER;
    self->r.svFlags = 0;
    self->s.eType = GMISC_ET_TURRET;
    self->flags |= 0x1000u;

    G_DObjUpdate(self);

    self->r.mins[0] = -32.0f;
    self->r.mins[1] = -32.0f;
    self->r.mins[2] = 0.0f;
    self->r.maxs[0] = 32.0f;
    self->r.maxs[1] = 32.0f;
    self->r.maxs[2] = 56.0f;

    G_SetOrigin(self, self->r.currentOrigin);
    G_SetAngle(self, self->r.currentAngles);

    self->s.angles2[0] = 0.0f;
    self->s.angles2[1] = 0.0f;
    self->s.angles2[2] = 0.0f;

    self->handler = GMISC_ENT_HANDLER_TURRET_INIT;
    self->nextthink = level.time + 50;

    self->s.apos.trType = TR_LINEAR_STOP;
    self->takedamage = 0;

    SV_LinkEntity(self);
}

void SP_turret(gentity_t *self)
{
    const char *weaponinfoname;

    if (!G_SpawnString("weaponinfo", "", &weaponinfoname)) {

        Com_Error(1, "\025no weaponinfo specified for turret");
    }
    G_SpawnTurret(self, weaponinfoname);
}

static void turret_clientaim(gentity_t *self, gentity_t *other)
{
    playerState_t *ps;
    turretInfo_s *info;

    info = self->pTurretInfo;
    ps = &other->client->ps;

    ps->viewlocked = GMISC_PLAYERVIEWLOCK_FULL;
    ps->viewlocked_entNum = self->s.number;

    self->s.angles2[0] = GMisc_Clamp(AngleSubtract(ps->viewangles[0], self->r.currentAngles[0]), info->arcmin[0], info->arcmax[0]);
    self->s.angles2[1] = GMisc_Clamp(AngleSubtract(ps->viewangles[1], self->r.currentAngles[1]), info->arcmin[1], info->arcmax[1]);
    self->s.angles2[2] = 0.0f;

    if (info->flags & 0x800) {
        info->flags &= ~0x800u;
        self->s.eFlags ^= GMISC_EF_TELEPORT_BIT;
    }
}

static void Turret_FillWeaponParms(gentity_t *ent, gentity_t *activator, weaponParms *wp)
{
    vec3_t diff;
    vec3_t playerPos;
    float flashTag[4][3];

    if (!G_DObjGetWorldTagMatrix(ent, SCR_CONST()->tag_flash, (vec3_t *)flashTag)) {
        Com_Error(1, "Couldn't find %s on turret (entity %d, classname '%s').\n",
                  "tag_flash", ent->s.number, SL_ConvertToString(ent->classname));
    }

    G_GetPlayerViewOrigin(activator, playerPos);
    G_GetPlayerViewDirection(activator, wp->forward, wp->right, wp->up);

    GMisc_VectorCopy(wp->forward, wp->gunForward);
    GMisc_VectorSubtract(flashTag[3], playerPos, diff);
    GMisc_VectorMA(playerPos, Vec3Normalize(diff), wp->forward, wp->muzzleTrace);
}

static void G_PlayerTurretPositionAndBlend(gentity_t *ent, gentity_t *pTurretEnt)
{
    float axis[4][3];
    clientInfo_t *ci;
    float fBlend;
    float fDelta;
    float fHeightRatio;
    float fPrevBlend;
    float fPrevTransZ;
    unsigned int baseAnim;
    unsigned int heightAnim;
    int i;
    int iBlend;
    int iPrevBlend;
    unsigned int leafAnim1;
    unsigned int leafAnim2;
    float localAxis[4][3];
    float localYaw;
    int numHorChildren;
    int numVertChildren;
    XAnimTree_s *pAnimTree;
    lerpFrame_t *pLerpAnim;
    XAnim_s *pXAnims;
    trace_t trace;
    vec2_t rot;
    vec3_t end;
    vec3_t endPos;
    DObjAnimMat_s *tagMat;
    vec3_t start;
    vec3_t tagAxis[3];
    float tagHeight;
    vec3_t trans;
    float turretAxis[4][3];
    vec3_t vDelta;
    WeaponDef *weapDef;

    ci = &level_bgs.clientinfo[ent->s.clientNum];
    pLerpAnim = &ci->legs;

    if (!pLerpAnim->animationNumber || !pLerpAnim->animation || !(pLerpAnim->animation->flags & 4)) {
        return;
    }

    tagMat = G_DObjGetLocalTagMatrix(pTurretEnt, SCR_CONST()->tag_weapon);
    if (!tagMat) {
        Com_Printf("WARNING: aborting player positioning on turret since 'tag_weapon' does not exist\n");
        return;
    }

    weapDef = BG_GetWeaponDef(pTurretEnt->s.weapon);
    pAnimTree = ci->pXAnimTree;
    pXAnims = level_bgs.animScriptData.animTree.anims;
    baseAnim = pLerpAnim->animationNumber & ~GMISC_ANIM_TOGGLEBIT;

    ConvertQuatToMat(tagMat, tagAxis);
    localYaw = vectosignedyaw(tagAxis[0]);

    AnglesToAxis(pTurretEnt->r.currentAngles, (vec_t *)turretAxis);
    GMisc_VectorCopy(pTurretEnt->r.currentOrigin, turretAxis[3]);
    GMisc_VectorSubtract(ent->r.currentOrigin, turretAxis[3], vDelta);

    tagHeight = GMisc_DotProduct(vDelta, turretAxis[2]);
    fDelta = tagHeight - tagMat->trans[2];

    XAnimClearTreeGoalWeightsStrict(pAnimTree, baseAnim, 0.0f);

    numVertChildren = XAnimGetNumChildren(pXAnims, baseAnim);
    if (!numVertChildren) {
        Com_Error(1, "Player anim '%s' has no children", XAnimGetAnimDebugName(pXAnims, baseAnim));
    }

    fPrevTransZ = 0.0f;
    fPrevBlend = 0.0f;
    iPrevBlend = 0;
    leafAnim2 = 0;
    i = 0;

    do {
        heightAnim = XAnimGetChildAt(pXAnims, baseAnim, i);
        GMisc_XAnimSetGoalWeight(pAnimTree, heightAnim, 1.0f, 1.0f);

        numHorChildren = XAnimGetNumChildren(pXAnims, heightAnim);
        if (!numHorChildren) {
            Com_Error(1, "Player anim '%s' has no children", XAnimGetAnimDebugName(pXAnims, heightAnim));
        }

        fBlend = (float)numHorChildren * 0.5f - localYaw / weapDef->fAnimHorRotateInc;
        if (fBlend < 0.0f) {
            fBlend = 0.0f;
        } else if (fBlend >= (float)(numHorChildren - 1)) {
            fBlend = (float)(numHorChildren - 1);
        }

        iBlend = (int)fBlend;
        fBlend -= (float)iBlend;

        leafAnim1 = XAnimGetChildAt(pXAnims, heightAnim, iBlend);
        GMisc_XAnimSetGoalWeight(pAnimTree, leafAnim1, 1.0f - fBlend, 1.0f);

        if (fBlend != 0.0f) {
            leafAnim2 = XAnimGetChildAt(pXAnims, heightAnim, iBlend + 1);
            GMisc_XAnimSetGoalWeight(pAnimTree, leafAnim2, fBlend, 1.0f);
        }

        XAnimCalcAbsDelta(pAnimTree, heightAnim, rot, trans);
        if (trans[2] >= fDelta) {
            break;
        }

        fPrevTransZ = trans[2];
        iPrevBlend = iBlend;
        fPrevBlend = fBlend;
        ++i;
    } while (i < numVertChildren);

    XAnimClearTreeGoalWeightsStrict(pAnimTree, baseAnim, 0.0f);
    GMisc_XAnimSetGoalWeight(pAnimTree, leafAnim1, 1.0f - fBlend, GMisc_XAnimGoalTime(pAnimTree, leafAnim1, 1.0f - fBlend));

    if (fBlend != 0.0f) {
        GMisc_XAnimSetGoalWeight(pAnimTree, leafAnim2, fBlend, GMisc_XAnimGoalTime(pAnimTree, leafAnim2, fBlend));
    }

    if (!i || i == numVertChildren) {
        GMisc_XAnimSetGoalWeight(pAnimTree, heightAnim, 1.0f, GMisc_XAnimGoalTime(pAnimTree, heightAnim, 1.0f));
    } else {
        fHeightRatio = (fDelta - fPrevTransZ) / (trans[2] - fPrevTransZ);
        GMisc_XAnimSetGoalWeight(pAnimTree, heightAnim, fHeightRatio, GMisc_XAnimGoalTime(pAnimTree, heightAnim, fHeightRatio));

        heightAnim = XAnimGetChildAt(pXAnims, baseAnim, i - 1);
        GMisc_XAnimSetGoalWeight(pAnimTree, heightAnim, 1.0f - fHeightRatio, GMisc_XAnimGoalTime(pAnimTree, heightAnim, 1.0f - fHeightRatio));

        leafAnim1 = XAnimGetChildAt(pXAnims, heightAnim, iPrevBlend);
        GMisc_XAnimSetGoalWeight(pAnimTree, leafAnim1, 1.0f - fPrevBlend, GMisc_XAnimGoalTime(pAnimTree, leafAnim1, 1.0f - fPrevBlend));

        if (fPrevBlend != 0.0f) {
            leafAnim2 = XAnimGetChildAt(pXAnims, heightAnim, iPrevBlend + 1);
            GMisc_XAnimSetGoalWeight(pAnimTree, leafAnim2, fPrevBlend, GMisc_XAnimGoalTime(pAnimTree, leafAnim2, fPrevBlend));
        }
    }

    XAnimCalcAbsDelta(pAnimTree, baseAnim, rot, trans);
    VectorAngleMultiply(trans, localYaw);

    localAxis[3][0] = trans[0] + tagMat->trans[0];
    localAxis[3][1] = trans[1] + tagMat->trans[1];
    localAxis[3][2] = tagHeight;

    YawToAxis(RotationToYaw(rot) + localYaw, localAxis);
    MatrixMultiply43((vec_t *)localAxis, (vec_t *)turretAxis, (vec_t *)axis);

    GMisc_VectorCopy(axis[3], ent->client->ps.origin);
    GMisc_VectorCopy(ent->client->ps.origin, start);
    GMisc_VectorCopy(ent->client->ps.origin, end);

    start[2] += ent->client->ps.viewHeightCurrent;
    end[2] -= GMISC_DEFAULT_VIEWHEIGHT;

    G_TraceCapsule(&trace, start, g_misc_vec3_origin, g_misc_vec3_origin, end, ent->s.number, GMISC_MASK_PLAYERSOLID);
    if (trace.fraction < 1.0f) {
        GMisc_Vec3Lerp(start, end, trace.fraction, endPos);
        ent->client->ps.origin[2] = endPos[2];
    }

    BG_PlayerStateToEntityState(&ent->client->ps, ent, 1, GMISC_PMOVE_HANDLER_SERVER);
    GMisc_VectorCopy(ent->client->ps.origin, ent->r.currentOrigin);

    AxisToAngles((vec_t *)axis, ent->r.currentAngles);
    SV_LinkEntity(ent);
}

static void Fire_Lead(gentity_t *ent, gentity_t *activator)
{
    weaponParms wp;

    if (activator == &((gentity_t *)imp_g_entities)[GMISC_ENTITYNUM_NONE]) {
        activator = &((gentity_t *)imp_g_entities)[GMISC_ENTITYNUM_WORLD];
    }

    Turret_FillWeaponParms(ent, activator, &wp);
    wp.weapDef = BG_GetWeaponDef(ent->s.weapon);

    if (wp.weapDef->weapType) {
        Weapon_RocketLauncher_Fire(ent, 0.0f, &wp);
    } else {
        Bullet_Fire(activator, ent->pTurretInfo->playerSpread, &wp, ent, level.time);
    }

    G_AddEvent(ent, GMISC_EV_FIRE_WEAPON_MG42, activator->s.number);
}

static void turret_ApplyRecoil(gentity_t *self, gentity_t *other)
{
    turretInfo_s *info = self->pTurretInfo;
    playerState_t *ps = &other->client->ps;
    float heat = (float)turretHeat[info - turretInfo].heat / GMISC_TURRET_HEAT_MAX;
    float pitchKick = 0.28f + randomf() * 0.12f + heat * 0.12f;
    float yawKick = (randomf() * 2.0f - 1.0f) * 0.18f;
    vec3_t angles;

    /* Move authoritative aim after the shot, not just the camera effect.
     * delta_angles preserves the kick across commands and lets mouse input
     * compensate. A sustained burst climbs further as the gun heats up. */
    angles[0] = self->r.currentAngles[0] + GMisc_Clamp(
        AngleSubtract(ps->viewangles[0], self->r.currentAngles[0]) - pitchKick,
        info->arcmin[0], info->arcmax[0]);
    angles[1] = self->r.currentAngles[1] + GMisc_Clamp(
        AngleSubtract(ps->viewangles[1], self->r.currentAngles[1]) + yawKick,
        info->arcmin[1], info->arcmax[1]);
    angles[2] = ps->viewangles[2];
    SetClientViewAngle(other, angles);
    turret_clientaim(self, other);
}

static void turret_shoot_internal(gentity_t *self, gentity_t *other)
{
    self->pTurretInfo->fireSndDelay = 3 * BG_GetWeaponDef(self->s.weapon)->iFireTime;

    if (other->client) {
        Fire_Lead(self, other);
        turret_ApplyRecoil(self, other);
        other->client->ps.viewlocked = GMISC_PLAYERVIEWLOCK_WEAPONJITTER;
    }
}

static void turret_track(gentity_t *self, gentity_t *other)
{
    turretInfo_s *info;
    WeaponDef *weapDef;
    turretHeat_t *heat = turret_UpdateHeat(self, other->client->buttons & GMISC_BUTTON_ATTACK);

    info = self->pTurretInfo;

    turret_clientaim(self, other);
    G_PlayerTurretPositionAndBlend(other, self);

    weapDef = BG_GetWeaponDef(self->s.weapon);

    other->client->ps.viewlocked = GMISC_PLAYERVIEWLOCK_FULL;
    self->s.eFlags &= ~GMISC_EF_FIRING;
    info->fireTime -= 50;

    if (info->fireTime > 0) {
        return;
    }

    info->fireTime = 0;
    if (!(other->client->buttons & GMISC_BUTTON_ATTACK) || heat->overheated) {
        return;
    }

    info->fireTime = weapDef->iFireTime;
    turret_shoot_internal(self, other);
    self->s.eFlags |= GMISC_EF_FIRING;
    heat->heat += weapDef->iFireTime * GMISC_TURRET_SHOT_HEAT_SCALE;
    if (heat->heat >= GMISC_TURRET_HEAT_MAX) {
        heat->heat = GMISC_TURRET_HEAT_MAX;
        heat->overheated = 1;
    }
    self->s.time2 = heat->heat * 100 / GMISC_TURRET_HEAT_MAX | (heat->overheated ? 0x100 : 0);
}

void turret_think_client(gentity_t *self)
{
    gentity_t *owner;

    owner = &((gentity_t *)imp_g_entities)[self->r.ownerNum];

    if (owner->active != 1 || owner->client->sess.sessionState != SESS_STATE_PLAYING) {
        G_ClientStopUsingTurret(self);
        return;
    }

    turret_track(self, owner);
    turret_UpdateSound(self);
}
