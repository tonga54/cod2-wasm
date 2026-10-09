#include "common_types.h"
#include "imports.h"
extern level_locals_t level;

#ifndef qtrue
#    define qtrue 1
#    define qfalse 0
#endif

#define _ENT(e) ((gentity_t *)(e))

#define ENTITY_STRIDE sizeof(gentity_s)
#define GMOVER_CONTENTS_0X04000000 0x04000000

extern level_locals_t level;
extern gentity_t g_entities[];
extern entityHandler_t entityHandlers[20];

#define LEVEL_PTR_M (&level)
#define LEVEL_TIME (LEVEL_PTR_M->time)
#define LEVEL_PREVIOUSTIME (LEVEL_PTR_M->previousTime)

#define HANDLER_REACHED(h) (entityHandlers[(h)].reached)
#define HANDLER_BLOCKED(h) (entityHandlers[(h)].blocked)

#define G_ENTITY(num) ((gentity_t *)((byte *)imp_g_entities + (num) * ENTITY_STRIDE))

#define VectorCopy(a, b) ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2])

/* migrated from the ILP32 data blob to typed C (x64 port Stage 2). */
char *hintStrings[6] = {
    "",
    "HINT_NONE",
    "HINT_ACTIVATE",
    "HINT_HEALTH",
    "HINT_FRIENDLY",
    NULL,
};

static pushed_t pushed[1024];
static pushed_t *pushed_p;

extern void SV_SetBrushModel(gentity_t *ent);
extern void SV_LinkEntity(gentity_t *ent);
extern void SV_UnlinkEntity(gentity_t *ent);
extern qboolean G_SpawnString(const char *key, const char *defaultString, const char **out);
extern int I_stricmp(const char *s1, const char *s2);
extern void SV_GetConfigstring(int index, char *buffer, int bufferSize);
extern void SV_SetConfigstring(int index, const char *val);
extern void Com_Error(int code, const char *fmt, ...);
extern void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);
extern void G_TraceCapsule(trace_t *result, vec_t *start, vec_t *mins, vec_t *maxs, vec_t *end, int skipNumber, int mask);
extern float RadiusFromBounds(vec_t *mins, vec_t *maxs);
extern int CM_AreaEntities(vec_t *mins, vec_t *maxs, int *entityList, int maxcount, int contentmask);
extern void BG_EvaluateTrajectory(trajectory_t *tr, int atTime, vec_t *result);
extern void G_Damage(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, const vec_t *dir, const vec_t *point, int damage, int dflags, int mod, int hitLoc, int timeOffset);
extern void G_GeneralLink(gentity_t *ent);
extern int G_RunThink(gentity_t *ent);

void use_trigger_use(gentity_t *ent, gentity_t *other, gentity_t *activator);
static void trigger_use_shared(gentity_t *ent);
void trigger_use(gentity_t *ent);
void trigger_use_touch(gentity_t *ent);
qboolean G_TryPushingEntity(gentity_t *check, gentity_t *pusher, vec_t *move, vec_t *amove);
void G_MoverTeam(gentity_t *ent);
void G_RunMover(gentity_t *ent);

static void G_TraceCapsuleForEntity(trace_t *tr, gentity_t *check, vec_t *origin, vec_t *mins, vec_t *maxs)
{
    int passEntityNum;

    if ((_ENT(check)->clipmask) != 0) {
        if ((_ENT(check)->r.contents) & 0x4) {
            tr->allsolid = 0;
            tr->startsolid = 0;
            return;
        }
    }

    if ((_ENT(check)->s.eType) == 4) {
        passEntityNum = (_ENT(check)->r.ownerNum);
    } else {
        passEntityNum = (_ENT(check)->s.number);
    }
    G_TraceCapsule(tr, origin, mins, maxs, origin, passEntityNum, 0x811);
}

void use_trigger_use(gentity_t *ent, gentity_t *other, gentity_t *activator)
{

    (void)ent;
    (void)other;
    (void)activator;
}

static void trigger_use_shared(gentity_t *ent)
{
    const char *cursorhint;
    char szConfigString[1024];
    int i;

    SV_SetBrushModel(ent);

    SV_LinkEntity(ent);

    ent->trigger.singleUserEntIndex = 0x3ff;

    (_ENT(ent)->s.pos.trType) = 0;

    VectorCopy((_ENT(ent)->r.currentOrigin), (_ENT(ent)->s.pos.trBase));

    (_ENT(ent)->r.contents) = 0x200000;

    (_ENT(ent)->r.svFlags) = 1;

    (_ENT(ent)->handler) = 0x12;

    (_ENT(ent)->s.dmgFlags) = 2;

    if (G_SpawnString("cursorhint", "", &cursorhint)) {

        if (I_stricmp(cursorhint, "HINT_NOICON") == 0) {

            (_ENT(ent)->s.dmgFlags) = -1;
        } else {
            for (i = 1; i < 6; i++) {

                if (I_stricmp(cursorhint, hintStrings[i]) == 0) {

                    (_ENT(ent)->s.dmgFlags) = i;
                    break;
                }
            }
        }
    }

    (_ENT(ent)->s.scale) = 0xff;

    if (G_SpawnString("hintstring", "", &cursorhint)) {

        for (i = 0; i < 32; i++) {

            SV_GetConfigstring(0x4fe + i, szConfigString, 1024);

            if (szConfigString[0] == '\0') {

                SV_SetConfigstring(0x4fe + i, cursorhint);

                (_ENT(ent)->s.scale) = (unsigned char)i;
                return;
            }

            if (strcmp(cursorhint, szConfigString) == 0) {

                (_ENT(ent)->s.scale) = (unsigned char)i;
                return;
            }
        }

        Com_Error(1, "Too many different hintstring key values on trigger_use ent", 32);
    }
}

void trigger_use(gentity_t *ent)
{

    trigger_use_shared(ent);
}

void trigger_use_touch(gentity_t *ent)
{

    trigger_use_shared(ent);
}

qboolean G_TryPushingEntity(gentity_t *check, gentity_t *pusher, vec_t *move, vec_t *amove)
{
    trace_t tr;
    vec3_t vOrigin;
    vec3_t org2;
    vec3_t forward, right, up;
    float matrix[9];
    vec3_t org, org2_rel;
    int i, j;
    float fx, fy, fz;
    float halfSize;
    gentity_t *hitEnt;
    gclient_t *client;
    vec_t *savedOrigin;

    savedOrigin = (_ENT(check)->r.currentOrigin);

    vOrigin[0] = (_ENT(check)->r.currentOrigin)[0] + move[0];
    vOrigin[1] = (_ENT(check)->r.currentOrigin)[1] + move[1];
    vOrigin[2] = (_ENT(check)->r.currentOrigin)[2] + move[2];

    AngleVectors(amove, forward, right, up);

    right[0] = -right[0];
    right[1] = -right[1];
    right[2] = -right[2];

    for (i = 0; i < 3; i++) {
        vec_t *src;
        if (i == 0)
            src = forward;
        else if (i == 1)
            src = right;
        else
            src = up;
        for (j = 0; j < 3; j++) {
            matrix[j * 3 + i] = src[j];
        }
    }

    org[0] = vOrigin[0] - (_ENT(pusher)->r.currentOrigin)[0];
    org[1] = vOrigin[1] - (_ENT(pusher)->r.currentOrigin)[1];
    org[2] = vOrigin[2] - (_ENT(pusher)->r.currentOrigin)[2];

    org2[0] = org[0] * matrix[0] + org[1] * matrix[1] + org[2] * matrix[2];
    org2[1] = org[0] * matrix[3] + org[1] * matrix[4] + org[2] * matrix[5];
    org2[2] = org[0] * matrix[6] + org[1] * matrix[7] + org[2] * matrix[8];

    vOrigin[0] += org2[0] - org[0];
    vOrigin[1] += org2[1] - org[1];
    vOrigin[2] += org2[2] - org[2];

    G_TraceCapsuleForEntity(&tr, check, vOrigin, (_ENT(check)->r.mins), (_ENT(check)->r.maxs));

    if (((&tr)->allsolid || (&tr)->startsolid)) {

        hitEnt = G_ENTITY((int)tr.entityNum);
        if (hitEnt != NULL) {

            halfSize = (double)(_ENT(check)->r.maxs)[0] * 0.5;
            if (halfSize > 4.0 && halfSize > 0.0) {
                float neg_fz, neg_fx, neg_fy;
                float fz_step, fx_step, fy_step;

                for (fz = 0.0f; (double)fz < (double)(_ENT(check)->r.maxs)[0] * 0.5; fz += 4.0f) {
                    neg_fz = -fz;
                    fz_step = fz + fz;
                    for (;;) {
                        for (fx = 4.0f; (double)fx < (double)(_ENT(check)->r.maxs)[0] * 0.5; fx += 4.0f) {
                            neg_fx = -fx;
                            fx_step = fx + fx;
                            for (;;) {
                                for (fy = 4.0f; (double)fy < (double)(_ENT(check)->r.maxs)[0] * 0.5; fy += 4.0f) {
                                    neg_fy = -fy;
                                    fy_step = fy + fy;
                                    for (;;) {

                                        org2[0] = neg_fx + vOrigin[0];
                                        org2[1] = neg_fy + vOrigin[1];
                                        org2[2] = neg_fz + vOrigin[2];
                                        G_TraceCapsuleForEntity(&tr, check, org2, (_ENT(check)->r.mins), (_ENT(check)->r.maxs));

                                        if (((&tr)->allsolid || (&tr)->startsolid)) {
                                            hitEnt = G_ENTITY((int)tr.entityNum);
                                            if (hitEnt != NULL) {

                                                neg_fy += fy_step;
                                                if (neg_fy <= fy) {
                                                    continue;
                                                }
                                                break;
                                            }
                                        }

                                        goto try_push_success;
                                    }
                                }

                                neg_fx += fx_step;
                                if (fx < neg_fx) {
                                    break;
                                }
                            }
                        }

                        if (neg_fz == 0.0f) {
                            break;
                        }
                        neg_fz += fz_step;
                        if (fz < neg_fz) {
                            break;
                        }
                    }
                }
            }

            G_TraceCapsuleForEntity(&tr, check, (_ENT(check)->r.currentOrigin), (_ENT(check)->r.mins), (_ENT(check)->r.maxs));

            if (((&tr)->allsolid || (&tr)->startsolid)) {
                hitEnt = G_ENTITY((int)tr.entityNum);
                if (hitEnt != NULL) {

                    return qfalse;
                }
            }

            (_ENT(check)->s.groundEntityNum) = 0x3ff;
            return qtrue;
        }
    }

    if ((_ENT(check)->s.groundEntityNum) != (_ENT(pusher)->s.number)) {
        (_ENT(check)->s.groundEntityNum) = 0x3ff;
    }

    VectorCopy(vOrigin, (_ENT(check)->r.currentOrigin));
    VectorCopy(vOrigin, (_ENT(check)->s.pos.trBase));

    client = (_ENT(check)->client);
    if (client != NULL) {

        int deltaYaw = (int)(amove[1] * 182.04444885253906f);
        deltaYaw &= 0xffff;
        client->ps.delta_angles[1] += deltaYaw;

        client = (_ENT(check)->client);
        VectorCopy(vOrigin, client->ps.origin);
    }

    pushed_p++;
    return qtrue;

try_push_success:

    if ((_ENT(check)->s.groundEntityNum) != (_ENT(pusher)->s.number)) {
        (_ENT(check)->s.groundEntityNum) = 0x3ff;
    }

    VectorCopy(org2, (_ENT(check)->r.currentOrigin));
    VectorCopy(org2, (_ENT(check)->s.pos.trBase));

    client = (_ENT(check)->client);
    if (client != NULL) {
        int deltaYaw = (int)(amove[1] * 182.04444885253906f);
        deltaYaw &= 0xffff;
        client->ps.delta_angles[1] += deltaYaw;

        client = (_ENT(check)->client);
        VectorCopy(org2, client->ps.origin);
    }

    pushed_p++;
    return qtrue;
}

void G_MoverTeam(gentity_t *ent)
{
    vec3_t origin, angles;
    vec3_t move, amove;
    vec3_t mins, maxs;
    vec3_t totalMins, totalMaxs;
    int entityList[1024];
    int listedEntities;
    int moveEntities;
    int moveEntityList[1024];
    int e;
    gentity_t *check;
    pushed_t *p;
    qboolean success;
    gentity_t *obstacle;
    int i;
    float radius;
    gclient_t *client;

    pushed_p = pushed;

    { extern const char *SL_ConvertToString(unsigned int); extern void Com_Printf(const char *, ...);
      Com_Printf("[mover] num=%d eType=%d posTr=%d aposTr=%d class=%s\n",
                 _ENT(ent)->s.number, _ENT(ent)->s.eType, _ENT(ent)->s.pos.trType, _ENT(ent)->s.apos.trType,
                 SL_ConvertToString((unsigned int)_ENT(ent)->classname)); }
    BG_EvaluateTrajectory((&_ENT(ent)->s.pos), LEVEL_TIME, origin);
    BG_EvaluateTrajectory((&_ENT(ent)->s.apos), LEVEL_TIME, angles);

    move[0] = origin[0] - (_ENT(ent)->r.currentOrigin)[0];
    move[1] = origin[1] - (_ENT(ent)->r.currentOrigin)[1];
    move[2] = origin[2] - (_ENT(ent)->r.currentOrigin)[2];

    amove[0] = angles[0] - (_ENT(ent)->r.currentAngles)[0];
    amove[1] = angles[1] - (_ENT(ent)->r.currentAngles)[1];
    amove[2] = angles[2] - (_ENT(ent)->r.currentAngles)[2];

    if ((_ENT(ent)->r.currentAngles)[0] != 0.0f || (_ENT(ent)->r.currentAngles)[1] != 0.0f ||
        (_ENT(ent)->r.currentAngles)[2] != 0.0f || amove[0] != 0.0f || amove[1] != 0.0f || amove[2] != 0.0f) {

        radius = RadiusFromBounds((_ENT(ent)->r.mins), (_ENT(ent)->r.maxs));

        for (i = 0; i < 3; i++) {
            float curOrigin = (_ENT(ent)->r.currentOrigin)[i];
            float moveI = move[i];
            mins[i] = curOrigin - radius + moveI;
            maxs[i] = curOrigin + radius + moveI;
            totalMins[i] = curOrigin - radius;
            totalMaxs[i] = curOrigin + radius;
        }
    } else {

        for (i = 0; i < 3; i++) {
            float moveI = move[i];
            mins[i] = moveI + (_ENT(ent)->r.absmin)[i];
            maxs[i] = moveI + (_ENT(ent)->r.absmax)[i];
        }
        VectorCopy((_ENT(ent)->r.absmin), totalMins);
        VectorCopy((_ENT(ent)->r.absmax), totalMaxs);
    }

    for (i = 0; i < 3; i++) {
        if (move[i] > 0.0f) {
            totalMaxs[i] += move[i];
        } else {
            totalMins[i] += move[i];
        }
    }

    SV_UnlinkEntity(ent);

    listedEntities = CM_AreaEntities(totalMins, totalMaxs, entityList, 1024, 0x2000180);

    (_ENT(ent)->r.currentOrigin)
    [0] += move[0];
    (_ENT(ent)->r.currentOrigin)
    [1] += move[1];
    (_ENT(ent)->r.currentOrigin)
    [2] += move[2];

    (_ENT(ent)->r.currentAngles)
    [0] += amove[0];
    (_ENT(ent)->r.currentAngles)
    [1] += amove[1];
    (_ENT(ent)->r.currentAngles)
    [2] += amove[2];

    SV_LinkEntity(ent);

    if (listedEntities <= 0) {
        obstacle = NULL;
        success = qtrue;
    } else {
        moveEntities = 0;

        for (e = 0; e < listedEntities; e++) {
            int entNum = entityList[e];
            check = G_ENTITY(entNum);

            {
                int eType = (_ENT(check)->s.eType);
                if (!((unsigned)(eType - 3) <= 1 || eType == 1 || (_ENT(check)->physicsObject) != 0)) {
                    goto skip_entity;
                }
            }

            if ((_ENT(check)->s.groundEntityNum) == (_ENT(ent)->s.number)) {
                goto add_to_move_list;
            }

            if ((_ENT(check)->r.absmin)[0] >= maxs[0])
                goto skip_entity;
            if ((_ENT(check)->r.absmin)[1] >= maxs[1])
                goto skip_entity;
            if ((_ENT(check)->r.absmin)[2] >= maxs[2])
                goto skip_entity;
            if (mins[0] >= (_ENT(check)->r.absmax)[0])
                goto skip_entity;
            if (mins[1] >= (_ENT(check)->r.absmax)[1])
                goto skip_entity;
            if (mins[2] >= (_ENT(check)->r.absmax)[2])
                goto skip_entity;

            {
                trace_t tr2;
                vec_t *checkOrigin = (_ENT(check)->r.currentOrigin);

                G_TraceCapsuleForEntity(&tr2, check, checkOrigin, (_ENT(check)->r.mins), (_ENT(check)->r.maxs));

                if (((&tr2)->allsolid || (&tr2)->startsolid)) {
                    int hitNum = (int)tr2.entityNum;
                    check = G_ENTITY(hitNum);
                } else {
                    check = NULL;
                }

                if (check != ent) {
                    goto skip_entity;
                }

                entNum = entityList[e];
            }

        add_to_move_list:

            moveEntityList[moveEntities] = entNum;
            moveEntities++;
            continue;

        skip_entity:
            continue;
        }

        if (moveEntities <= 0) {
            obstacle = NULL;
            success = qtrue;
        } else {

            for (i = 0; i < moveEntities; i++) {
                int num = moveEntityList[i];
                gentity_t *moveEnt = G_ENTITY(num);
                SV_UnlinkEntity(moveEnt);
            }

            obstacle = NULL;
            success = qtrue;

            for (i = 0; i < moveEntities; i++) {
                int num = moveEntityList[i];
                gentity_t *pushEnt = G_ENTITY(num);

                pushed_p->ent = pushEnt;
                VectorCopy((_ENT(pushEnt)->r.currentOrigin), pushed_p->origin);

                pushed_p->deltayaw = amove[1];

                if (!G_TryPushingEntity(pushEnt, ent, move, amove)) {

                    if ((_ENT(pushEnt)->s.eType) == 3) {

                        SV_LinkEntity(pushEnt);
                    } else {

                        if ((_ENT(ent)->s.pos.trType) == 4 || (_ENT(ent)->s.apos.trType) == 4) {

                            G_Damage(pushEnt, ent, ent, NULL, NULL, 99999, 0, 9, 0, 0);
                        } else {
                            obstacle = pushEnt;
                            success = qfalse;
                        }
                    }
                } else {

                    SV_LinkEntity(pushEnt);
                }
            }

            for (i = 0; i < moveEntities; i++) {
                int num = moveEntityList[i];
                gentity_t *moveEnt = G_ENTITY(num);
                SV_LinkEntity(moveEnt);
            }
        }
    }

    if (!success) {

        for (p = pushed_p - 1; p >= pushed; p--) {
            gentity_t *checkEnt = p->ent;

            VectorCopy(p->origin, (_ENT(checkEnt)->r.currentOrigin));

            VectorCopy(p->origin, (_ENT(checkEnt)->s.pos.trBase));

            client = (_ENT(checkEnt)->client);
            if (client != NULL) {

                int deltaYaw = (int)(p->deltayaw * 182.04444885253906f);
                deltaYaw &= 0xffff;
                client->ps.delta_angles[1] -= deltaYaw;

                client = (_ENT(checkEnt)->client);
                VectorCopy(p->origin, client->ps.origin);
            }

            SV_LinkEntity(checkEnt);
        }

        {
            int timeDelta = LEVEL_TIME - LEVEL_PREVIOUSTIME;
            (_ENT(ent)->s.pos.trTime) += timeDelta;
            (_ENT(ent)->s.apos.trTime) += timeDelta;
        }

        BG_EvaluateTrajectory((&_ENT(ent)->s.pos), LEVEL_TIME, (_ENT(ent)->r.currentOrigin));
        BG_EvaluateTrajectory((&_ENT(ent)->s.apos), LEVEL_TIME, (_ENT(ent)->r.currentAngles));

        SV_LinkEntity(ent);

        {
            byte handlerIdx = (_ENT(ent)->handler);
            void (*blocked)(gentity_t *, gentity_t *) = HANDLER_BLOCKED(handlerIdx);
            if (blocked != NULL) {
                blocked(ent, obstacle);
            }
        }
    } else {

        if ((_ENT(ent)->s.pos.trType) != 0) {

            if ((_ENT(ent)->s.pos.trTime) + (_ENT(ent)->s.pos.trDuration) <= LEVEL_TIME) {

                byte handlerIdx = (_ENT(ent)->handler);
                void (*reached)(gentity_t *) = HANDLER_REACHED(handlerIdx);
                if (reached != NULL) {
                    reached(ent);
                }
            }
        }

        if ((_ENT(ent)->s.apos.trType) != 0) {

            if ((_ENT(ent)->s.apos.trTime) + (_ENT(ent)->s.apos.trDuration) <= LEVEL_TIME) {

                byte handlerIdx = (_ENT(ent)->handler);
                void (*reached)(gentity_t *) = HANDLER_REACHED(handlerIdx);
                if (reached != NULL) {
                    reached(ent);
                }
            }
        }
    }
}

void G_RunMover(gentity_t *ent)
{
    if ((_ENT(ent)->tagInfo) != 0) {
        G_GeneralLink(ent);
    } else if ((_ENT(ent)->s.pos.trType) != 0 || (_ENT(ent)->s.apos.trType) != 0) {
        G_MoverTeam(ent);
    }
    G_RunThink(ent);
}
