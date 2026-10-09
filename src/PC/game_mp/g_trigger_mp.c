#include "common_types.h"
#include "imports.h"

extern int Scr_IsSystemActive(int flag);
extern void Scr_AddEntity(gentity_t *ent);
extern void Scr_Notify(gentity_t *ent, unsigned int stringId, int numArgs);
extern void Scr_AddInt(int value);
extern unsigned int Scr_GetNumParam(void);
extern float Scr_GetFloat(int param);
extern void Scr_Error(const char *msg);

extern void SV_SetBrushModel(gentity_t *ent);
extern void SV_LinkEntity(gentity_t *ent);
extern int SV_SightTraceToEntity(vec_t *start, vec_t *mins, vec_t *maxs, vec_t *end, int entityNum, int contentMask);

extern int G_SpawnString(const char *key, const char *defaultValue, const char **out);
extern int G_SpawnFloat(const char *key, const char *defaultValue, float *out);
extern qboolean G_SpawnInt(const char *key, const char *defaultValue, int *out);
extern void G_Damage(gentity_t *target, gentity_t *inflictor, gentity_t *attacker,
                     const vec_t *dir, const vec_t *point, int damage, int dflags,
                     int mod, int hitLoc, int timeOffset);
extern void G_FreeEntityDelay(gentity_t *ent);

extern const char *va(const char *fmt, ...);
extern void Com_Error(int code, const char *fmt, ...);
extern void AddPointToBounds(vec_t *point, vec_t *mins, vec_t *maxs);
extern int CM_AreaEntities(vec_t *mins, vec_t *maxs, int *list, int maxCount, int mask);

extern level_locals_t level;

extern gentity_t g_entities[];
extern byte *g_trace_zero_ptr;

#define ENTITY_STRIDE sizeof(gentity_s) /* was hardcoded 560 = x86 sizeof; wrong on x64 */

#define LEVEL ((level_locals_t *)imp_level)
#define G_ENTITIES ((gentity_t *)imp_g_entities)
#define SCR_CONST() ((const scr_const_t *)imp_scr_const)

void G_Trigger(gentity_t *self, gentity_t *other);
void hurt_use(gentity_t *self, gentity_t *other, gentity_t *activator);
void SP_trigger_lookat(gentity_t *self);
void SP_trigger_hurt(gentity_t *self);
void SP_trigger_damage(gentity_t *pSelf);
void SP_trigger_multiple(gentity_t *ent);
void SP_trigger_radius(gentity_t *ent);
void SP_trigger_disk(gentity_t *ent);
void SP_trigger_once(gentity_t *ent);
void Touch_Multi(gentity_t *self, gentity_t *other, qboolean bTouched);
void hurt_touch(gentity_t *self, gentity_t *other, qboolean bTouched);
void Activate_trigger_damage(gentity_t *pEnt, gentity_t *pOther, int iDamage, int iMOD);
void G_GrenadeTouchTriggerDamage(gentity_t *pActivator, vec_t *vStart, vec_t *vEnd, int iDamage, int iMOD);
void G_CheckHitTriggerDamage(gentity_t *pActivator, vec_t *vStart, vec_t *vEnd, int iDamage, int iMOD);
void Die_trigger_damage(gentity_t *pSelf, gentity_t *pInflictor, gentity_t *pAttacker, int iDamage, int iMod, int iWeapon, const vec_t *vDir, const hitLocation_t hitLoc, int timeOffset);
void Pain_trigger_damage(gentity_t *pSelf, gentity_t *pAttacker, int iDamage, const vec_t *vPoint, const int iMod, const vec_t *vDir, const hitLocation_t hitLoc);
void Use_trigger_damage(gentity_t *pEnt, gentity_t *pOther, gentity_t *pActivator);

static inline __attribute__((always_inline)) void G_Trigger_core(gentity_t *self, gentity_t *other)
{
    level_locals_t *level;
    int triggerCount;
    trigger_info_t *entry;

    if (!Scr_IsSystemActive(1))
        return;

    level = LEVEL;
    triggerCount = level->pendingTriggerListSize;

    if (triggerCount == 0x100) {
        Scr_AddEntity(other);
        Scr_Notify(self, SCR_CONST()->trigger, 1);
        return;
    }

    entry = &level->pendingTriggerList[triggerCount];
    level->pendingTriggerListSize = triggerCount + 1;
    entry->entnum = (unsigned short)self->s.number;
    entry->otherEntnum = (unsigned short)other->s.number;
    entry->useCount = self->useCount;
    entry->otherUseCount = other->useCount;
}

void G_Trigger(gentity_t *self, gentity_t *other)
{
    G_Trigger_core(self, other);
}

void hurt_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
    byte state = self->handler;

    self->handler = (state != 3) ? 3 : 2;
}

void SP_trigger_lookat(gentity_t *self)
{
    SV_SetBrushModel(self);
    self->r.contents = 0x20000000;
    self->r.svFlags = 1;
    self->s.eFlags |= 1;
    SV_LinkEntity(self);
}

void SP_trigger_hurt(gentity_t *self)
{
    const char *sound;
    int spawnflags;

    SV_SetBrushModel(self);
    self->r.contents = 0x405c0008;
    self->r.svFlags = 1;
    self->s.eFlags |= 1;

    G_SpawnString("sound", "world_hurt_me", &sound);

    if (self->damage == 0)
        self->damage = 5;

    self->r.contents = 0x405c0008;

    spawnflags = self->spawnflags & 1;
    {
        byte sf = (byte)spawnflags;
        sf = (byte)(-sf);
        self->handler = (byte)(sf + 3);
    }
}

void SP_trigger_damage(gentity_t *pSelf)
{
    level_locals_t *level;
    float wait;

    G_SpawnInt("accumulate", "0", &pSelf->trigger.accumulate);
    G_SpawnInt("threshold", "0", &pSelf->trigger.threshold);

    pSelf->health = 0x7d00;
    pSelf->takedamage = 1;
    pSelf->handler = 4;

    level = LEVEL;
    if (level->spawnVar.spawnVarsValid != 0) {
        if (G_SpawnFloat("wait", "", &wait) && !(0.0f < wait))
            pSelf->spawnflags |= 0x200;
    }

    SV_SetBrushModel(pSelf);
    pSelf->r.contents = 0x405c0008;
    pSelf->r.svFlags = 1;
    pSelf->s.eFlags |= 1;
    SV_LinkEntity(pSelf);
}

void SP_trigger_multiple(gentity_t *ent)
{
    level_locals_t *level;
    float wait;
    int spawnflags;

    ent->handler = 1;

    level = LEVEL;
    if (level->spawnVar.spawnVarsValid != 0) {
        if (G_SpawnFloat("wait", "", &wait) && !(0.0f < wait))
            ent->spawnflags |= 0x10;
    }

    SV_SetBrushModel(ent);
    ent->r.svFlags = 1;
    ent->s.eFlags |= 1;

    ent->r.contents = 0;
    spawnflags = ent->spawnflags;
    if (!(spawnflags & 8))
        ent->r.contents = 0x40000000;
    if (spawnflags & 1)
        ent->r.contents |= 0x40000;
    if (spawnflags & 2)
        ent->r.contents |= 0x80000;
    if (spawnflags & 4)
        ent->r.contents |= 0x100000;

    SV_LinkEntity(ent);
}

void SP_trigger_radius(gentity_t *ent)
{
    float radius;
    float height;
    float wait;
    float neg_radius;
    int spawnflags;

    if (LEVEL->spawnVar.spawnVarsValid != 0) {

        if (!G_SpawnFloat("radius", "", &radius)) {
            Com_Error(1, va("radius not specified for trigger_radius at (%g %g %g)",
                            ent->r.currentOrigin[0], ent->r.currentOrigin[1], ent->r.currentOrigin[2]));
        }
        if (!G_SpawnFloat("height", "", &height)) {
            Com_Error(1, va("height not specified for trigger_radius at (%g %g %g)",
                            ent->r.currentOrigin[0], ent->r.currentOrigin[1], ent->r.currentOrigin[2]));
        }
    } else {

        if (Scr_GetNumParam() <= 4)
            Scr_Error("USAGE: spawn( \"trigger_radius\", <origin>, <spawnflags>, <radius>, <height> )");
        radius = Scr_GetFloat(3);
        height = Scr_GetFloat(4);
    }

    ent->handler = 1;

    neg_radius = -radius;
    ent->r.mins[0] = neg_radius;
    ent->r.mins[1] = neg_radius;
    ent->r.mins[2] = 0.0f;
    ent->r.maxs[0] = radius;
    ent->r.maxs[1] = radius;
    ent->r.maxs[2] = height;

    ent->r.svFlags = 0x21;

    if (LEVEL->spawnVar.spawnVarsValid != 0) {
        if (G_SpawnFloat("wait", "", &wait) && !(0.0f < wait)) {
            ent->spawnflags |= 0x10;
        }
    }

    spawnflags = ent->spawnflags;
    ent->r.contents = 0;
    if (!(spawnflags & 8))
        ent->r.contents = 0x40000000;
    if (spawnflags & 1)
        ent->r.contents |= 0x40000;
    if (spawnflags & 2)
        ent->r.contents |= 0x80000;
    if (spawnflags & 4)
        ent->r.contents |= 0x100000;

    SV_LinkEntity(ent);
}

void SP_trigger_disk(gentity_t *ent)
{
    level_locals_t *level;
    float radius;
    float wait;
    int spawnflags;

    if (!G_SpawnFloat("radius", "", &radius)) {
        Com_Error(1, va("radius not specified for trigger_radius at (%g %g %g)",
                        ent->r.currentOrigin[0], ent->r.currentOrigin[1], ent->r.currentOrigin[2]));
    }

    ent->handler = 1;

    radius += 64.0f;
    ent->r.mins[0] = -radius;
    ent->r.mins[1] = -radius;
    ent->r.mins[2] = -100000.0f;
    ent->r.maxs[0] = radius;
    ent->r.maxs[1] = radius;
    ent->r.maxs[2] = 100000.0f;

    ent->r.svFlags = 0x41;

    level = LEVEL;
    if (level->spawnVar.spawnVarsValid != 0) {
        if (G_SpawnFloat("wait", "", &wait) && !(0.0f < wait)) {
            ent->spawnflags |= 0x10;
        }
    }

    spawnflags = ent->spawnflags;
    ent->r.contents = 0;
    if (!(spawnflags & 8))
        ent->r.contents = 0x40000000;
    if (spawnflags & 1)
        ent->r.contents |= 0x40000;
    if (spawnflags & 2)
        ent->r.contents |= 0x80000;
    if (spawnflags & 4)
        ent->r.contents |= 0x100000;

    SV_LinkEntity(ent);
}

void SP_trigger_once(gentity_t *ent)
{
    int spawnflags;

    ent->handler = 1;
    ent->spawnflags |= 0x10;

    SV_SetBrushModel(ent);
    ent->r.svFlags = 1;
    ent->s.eFlags |= 1;

    ent->r.contents = 0;
    spawnflags = ent->spawnflags;
    if (!(spawnflags & 8))
        ent->r.contents = 0x40000000;
    if (spawnflags & 1)
        ent->r.contents |= 0x40000;
    if (spawnflags & 2)
        ent->r.contents |= 0x80000;
    if (spawnflags & 4)
        ent->r.contents |= 0x100000;

    SV_LinkEntity(ent);
}

void Touch_Multi(gentity_t *self, gentity_t *other, qboolean bTouched)
{
    G_Trigger_core(self, other);

    if (self->spawnflags & 0x10)
        G_FreeEntityDelay(self);
}

void hurt_touch(gentity_t *self, gentity_t *other, qboolean bTouched)
{
    level_locals_t *level;
    int spawnflags;
    int dflags;

    if (other->takedamage == 0)
        return;

    level = LEVEL;
    if (self->trigger.timestamp > level->time)
        return;

    G_Trigger_core(self, other);

    spawnflags = self->spawnflags;

    if (spawnflags & 0x10) {
        self->trigger.timestamp = LEVEL->time + 1000;
    } else {
        self->trigger.timestamp = LEVEL->time + 50;
    }

    dflags = (spawnflags & 8) ? 0x10 : 0;

    G_Damage(other, self, self, NULL, NULL, self->damage, dflags, 0xd, 0, 0);

    if (self->spawnflags & 0x20)
        self->handler = 2;
}

void Activate_trigger_damage(gentity_t *pEnt, gentity_t *pOther, int iDamage, int iMOD)
{
    int threshold;
    int spawnflags;
    int accumulate;

    threshold = pEnt->trigger.threshold;
    if (threshold > 0 && threshold > iDamage)
        return;

    spawnflags = pEnt->spawnflags;

    if (spawnflags & 1) {
        if (iMOD == 1)
            return;
    }
    if (spawnflags & 2) {
        if (iMOD == 2)
            return;
    }
    if (spawnflags & 4) {
        if ((unsigned)(iMOD - 3) <= 3)
            return;
    }
    if (spawnflags & 8) {
        if (iMOD <= 14 && ((1 << iMOD) & 0x4078))
            return;
    }
    if (spawnflags & 0x10) {
        if (iMOD == 4 || iMOD == 6)
            return;
    }
    if (spawnflags & 0x20) {
        if (iMOD == 7)
            return;
    }
    if (spawnflags & 0x100) {
        if (iMOD <= 13 && ((1 << iMOD) & 0x3e01))
            return;
    }

    accumulate = pEnt->trigger.accumulate;
    if (accumulate != 0) {
        if (accumulate > 0x7d00 - pEnt->health)
            return;
    }

    if (iMOD != -1)
        G_Trigger_core(pEnt, pOther);

    spawnflags = pEnt->spawnflags;
    pEnt->health = 0x7d00;

    if (spawnflags & 0x200)
        G_FreeEntityDelay(pEnt);
}

void G_GrenadeTouchTriggerDamage(gentity_t *pActivator, vec_t *vStart, vec_t *vEnd, int iDamage, int iMOD)
{
    vec_t vMins[3], vMaxs[3];
    int iTouch[1024];
    int iNum;
    int i;
    gentity_t *pHit;

    vMins[0] = vStart[0];
    vMins[1] = vStart[1];
    vMins[2] = vStart[2];
    vMaxs[0] = vStart[0];
    vMaxs[1] = vStart[1];
    vMaxs[2] = vStart[2];

    AddPointToBounds(vEnd, vMins, vMaxs);

    iNum = CM_AreaEntities(vMins, vMaxs, iTouch, 1024, 0x400000);
    if (iNum <= 0)
        return;

    for (i = 0; i < iNum; i++) {
        pHit = &G_ENTITIES[iTouch[i]];

        if (pHit->classname != SCR_CONST()->trigger_damage)
            continue;

        if (!(pHit->flags & 0x4000))
            continue;

        if (!SV_SightTraceToEntity(vStart, (vec_t *)g_trace_zero_ptr, (vec_t *)g_trace_zero_ptr,
                                   vEnd, pHit->s.number, -1))
            continue;

        Scr_AddEntity(pActivator);
        Scr_AddInt(iDamage);
        Scr_Notify(pHit, SCR_CONST()->damage, 2);

        Activate_trigger_damage(pHit, pActivator, iDamage, iMOD);

        if (pHit->trigger.accumulate == 0)
            pHit->health = 0x7d00;
    }
}

void G_CheckHitTriggerDamage(gentity_t *pActivator, vec_t *vStart, vec_t *vEnd, int iDamage, int iMOD)
{
    vec_t vMins[3], vMaxs[3];
    int iTouch[1024];
    int iNum;
    int i;
    gentity_t *pHit;

    vMins[0] = vStart[0];
    vMins[1] = vStart[1];
    vMins[2] = vStart[2];
    vMaxs[0] = vStart[0];
    vMaxs[1] = vStart[1];
    vMaxs[2] = vStart[2];

    AddPointToBounds(vEnd, vMins, vMaxs);

    iNum = CM_AreaEntities(vMins, vMaxs, iTouch, 1024, 0x400000);
    if (iNum <= 0)
        return;

    for (i = 0; i < iNum; i++) {
        pHit = &G_ENTITIES[iTouch[i]];

        if (pHit->classname != SCR_CONST()->trigger_damage)
            continue;

        if (!SV_SightTraceToEntity(vStart, (vec_t *)g_trace_zero_ptr, (vec_t *)g_trace_zero_ptr,
                                   vEnd, pHit->s.number, -1))
            continue;

        Scr_AddEntity(pActivator);
        Scr_AddInt(iDamage);
        Scr_Notify(pHit, SCR_CONST()->damage, 2);

        Activate_trigger_damage(pHit, pActivator, iDamage, iMOD);

        if (pHit->trigger.accumulate == 0) {
            pHit->health = 0x7d00;
        }
    }
}

void Die_trigger_damage(gentity_t *pSelf, gentity_t *pInflictor, gentity_t *pAttacker, int iDamage, int iMod, int iWeapon, const vec_t *vDir, const hitLocation_t hitLoc, int timeOffset)
{
    Activate_trigger_damage(pSelf, pAttacker, iDamage, iMod);

    if (pSelf->trigger.accumulate == 0)
        pSelf->health = 0x7d00;
}

void Pain_trigger_damage(gentity_t *pSelf, gentity_t *pAttacker, int iDamage, const vec_t *vPoint, const int iMod, const vec_t *vDir, const hitLocation_t hitLoc)
{
    Activate_trigger_damage(pSelf, pAttacker, iDamage, iMod);

    if (pSelf->trigger.accumulate == 0)
        pSelf->health = 0x7d00;
}

void Use_trigger_damage(gentity_t *pEnt, gentity_t *pOther, gentity_t *pActivator)
{
    Activate_trigger_damage(pEnt, pOther, pEnt->trigger.accumulate + 1, -1);
}
