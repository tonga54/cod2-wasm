#include "common_types.h"
#include "imports.h"
#include <string.h>

extern float Scr_GetFloat(int param);
extern void Scr_GetVector(int param, vec3_t out);
extern unsigned int Scr_GetNumParam(void);
extern void Scr_ParamError(int param, const char *msg);
extern void Scr_Error(const char *msg);
extern void Scr_ObjectError(const char *msg);
extern void Scr_Notify(gentity_t *ent, unsigned short name, int numArgs);
extern const char *va(const char *fmt, ...);
extern void Com_DPrintf(const char *fmt, ...);
extern void SV_LinkEntity(gentity_t *ent);
extern void SV_SetBrushModel(gentity_t *ent);
extern void G_DObjUpdate(gentity_t *ent);
extern qboolean G_SpawnFloat(const char *key, const char *defaultValue, float *out);
extern qboolean G_SpawnVector(const char *key, const char *defaultValue, vec3_t out);
extern void BG_EvaluateTrajectory(trajectory_t *tr, int time, vec3_t result);
extern float Vec3Normalize(vec3_t v);
extern float Vec3NormalizeTo(const vec3_t v, vec3_t out);
extern float AngleNormalize180(float angle);
extern float AngleNormalize360(float angle);
extern float AngleSubtract(float a, float b);


#define g_entities ((gentity_t *)imp_g_entities)
extern level_locals_t level;

#define SCR_CONST() ((const scr_const_t *)imp_scr_const)

void ScriptEntCmd_MoveTo(scr_entref_t entref);
void ScriptEntCmd_MoveX(scr_entref_t entref);
void ScriptEntCmd_MoveY(scr_entref_t entref);
void ScriptEntCmd_MoveZ(scr_entref_t entref);
void ScriptEntCmd_GravityMove(scr_entref_t entref);
void ScriptEntCmd_RotateTo(scr_entref_t entref);
void ScriptEntCmd_RotatePitch(scr_entref_t entref);
void ScriptEntCmd_RotateYaw(scr_entref_t entref);
void ScriptEntCmd_RotateRoll(scr_entref_t entref);
void ScriptEntCmd_RotateVelocity(scr_entref_t entref);
void ScriptEntCmd_Solid(scr_entref_t entref);
void ScriptEntCmd_NotSolid(scr_entref_t entref);

static const BuiltinMethodDef methods[] = {
    { "moveto", (BuiltinMethod)ScriptEntCmd_MoveTo, 0 },
    { "movex", (BuiltinMethod)ScriptEntCmd_MoveX, 0 },
    { "movey", (BuiltinMethod)ScriptEntCmd_MoveY, 0 },
    { "movez", (BuiltinMethod)ScriptEntCmd_MoveZ, 0 },
    { "movegravity", (BuiltinMethod)ScriptEntCmd_GravityMove, 0 },
    { "rotateto", (BuiltinMethod)ScriptEntCmd_RotateTo, 0 },
    { "rotatepitch", (BuiltinMethod)ScriptEntCmd_RotatePitch, 0 },
    { "rotateyaw", (BuiltinMethod)ScriptEntCmd_RotateYaw, 0 },
    { "rotateroll", (BuiltinMethod)ScriptEntCmd_RotateRoll, 0 },
    { "rotatevelocity", (BuiltinMethod)ScriptEntCmd_RotateVelocity, 0 },
    { "solid", (BuiltinMethod)ScriptEntCmd_Solid, 0 },
    { "notsolid", (BuiltinMethod)ScriptEntCmd_NotSolid, 0 },
};

static inline __attribute__((always_inline)) void VectorCopy(const vec3_t src, vec3_t dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

static inline __attribute__((always_inline)) void VectorScale(const vec3_t v, float scale, vec3_t out)
{
    out[0] = v[0] * scale;
    out[1] = v[1] * scale;
    out[2] = v[2] * scale;
}

static inline __attribute__((always_inline)) void VectorMA(const vec3_t base, float scale, const vec3_t dir, vec3_t out)
{
    out[0] = base[0] + scale * dir[0];
    out[1] = base[1] + scale * dir[1];
    out[2] = base[2] + scale * dir[2];
}

extern float sqrtf(float x);

static inline __attribute__((always_inline)) float VectorLength(const vec3_t v)
{
    return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

static inline __attribute__((always_inline)) void VectorSubtract(const vec3_t a, const vec3_t b, vec3_t out)
{
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

static inline __attribute__((always_inline)) gentity_t *GetScriptMoverEntity(scr_entref_t entref)
{
    unsigned short entnum = entref.entnum;
    unsigned short classnum = entref.classnum;
    gentity_t *pSelf;
    const scr_const_t *sc;

    if (classnum != 0) {
        Scr_ObjectError("not an entity");
        return NULL;
    }

    pSelf = &g_entities[entnum];

    sc = SCR_CONST();
    if (pSelf->classname != sc->script_brushmodel &&
        pSelf->classname != sc->script_model &&
        pSelf->classname != sc->script_origin) {
        Scr_ObjectError(va("entity %i is not a script_brushmodel, script_model, or script_origin", entnum));
    }

    return pSelf;
}

void ScriptEntCmdGetCommandTimes(float *pfTotalTime, float *pfAccelTime, float *pfDecelTime);
void ScriptEntCmd_Solid(scr_entref_t entref);
void ScriptEntCmd_NotSolid(scr_entref_t entref);
BuiltinMethod ScriptEnt_GetMethod(const char **pName);
void InitScriptMover(gentity_t *pSelf);
void SP_script_origin(gentity_t *pSelf);
void SP_script_model(gentity_t *pSelf);
void SP_script_brushmodel(gentity_t *pSelf);
void ScriptEntCmd_GravityMove(scr_entref_t entref);
void ScriptEntCmd_RotateVelocity(scr_entref_t entref);
static qboolean ScriptMover_Updatemove(trajectory_t *pTr, float *vCurrPos, float fSpeed, float fMidTime, float fDecelTime, const vec3_t vPos1, const vec3_t vPos2, const vec3_t vPos3);
void Reached_ScriptMover(gentity_t *pEnt);
static void ScriptMover_SetupMove(trajectory_t *pTr, const vec3_t vPos, vec3_t vCurrPos, float fTotalTime, float fAccelTime, float fDecelTime, float *pfSpeed, float *pfMidTime, float *pfDecelTime, vec3_t vPos1, vec3_t vPos2, vec3_t vPos3);
void ScriptEnt_RotateAxis(scr_entref_t entref, int iAxis);
void ScriptEntCmd_RotateRoll(scr_entref_t entref);
void ScriptEntCmd_RotateYaw(scr_entref_t entref);
void ScriptEntCmd_RotatePitch(scr_entref_t entref);
void ScriptEnt_MoveAxis(scr_entref_t entref, int iAxis);
void ScriptEntCmd_MoveZ(scr_entref_t entref);
void ScriptEntCmd_MoveY(scr_entref_t entref);
void ScriptEntCmd_MoveX(scr_entref_t entref);
void ScriptEntCmd_RotateTo(scr_entref_t entref);
void ScriptEntCmd_MoveTo(scr_entref_t entref);

void ScriptEntCmdGetCommandTimes(float *pfTotalTime, float *pfAccelTime, float *pfDecelTime)
{
    int iNumParms;

    *pfTotalTime = Scr_GetFloat(1);
    if (!(*pfTotalTime > 0.0f)) {
        if (*pfTotalTime == 0.0f) {
            Scr_ParamError(1, "total time must be positive");
        }
    }

    iNumParms = Scr_GetNumParam();
    if (iNumParms > 2) {
        *pfAccelTime = Scr_GetFloat(2);
        if (0.0f > *pfAccelTime) {
            Scr_ParamError(2, "accel time must be nonnegative");
        }

        if (iNumParms > 3) {
            *pfDecelTime = Scr_GetFloat(3);
            if (0.0f > *pfDecelTime) {
                Scr_ParamError(3, "decel time must be nonnegative");
            }
        } else {
            *pfDecelTime = 0.0f;
        }
    } else {
        *pfAccelTime = 0.0f;
        *pfDecelTime = 0.0f;
    }

    if (*pfAccelTime + *pfDecelTime > *pfTotalTime) {
        Scr_Error("accel time plus decel time is greater than total time");
    }
}

void ScriptEntCmd_Solid(scr_entref_t entref)
{
    gentity_t *pSelf;
    unsigned short entnum = entref.entnum;
    unsigned short classnum = entref.classnum;
    const scr_const_t *sc = SCR_CONST();

    if (classnum != 0) {
        Scr_ObjectError("not an entity");
        pSelf = (gentity_t *)0;
    } else {
        pSelf = &g_entities[entnum];
    }

    if (pSelf->classname != sc->script_brushmodel &&
        pSelf->classname != sc->script_model &&
        pSelf->classname != sc->script_origin) {
        Scr_ObjectError(va("entity %i is not a script_brushmodel, script_model, or script_origin", entnum));
    }

    if (pSelf->classname == sc->script_origin) {

        Com_DPrintf("cannot use the solid/notsolid commands on a script_origin entity( number %i )\n", pSelf->s.number);
        return;
    }

    if (pSelf->classname == sc->script_model) {

        pSelf->r.contents = 0x2080;
    } else {

        pSelf->r.contents = 1;
        pSelf->s.eFlags &= ~1;
    }

    SV_LinkEntity(pSelf);
}

void ScriptEntCmd_NotSolid(scr_entref_t entref)
{
    gentity_t *pSelf;
    unsigned short entnum = entref.entnum;
    unsigned short classnum = entref.classnum;
    const scr_const_t *sc = SCR_CONST();

    if (classnum != 0) {
        Scr_ObjectError("not an entity");
        pSelf = (gentity_t *)0;
    } else {
        pSelf = &g_entities[entnum];
    }

    if (pSelf->classname != sc->script_brushmodel &&
        pSelf->classname != sc->script_model &&
        pSelf->classname != sc->script_origin) {
        Scr_ObjectError(va("entity %i is not a script_brushmodel, script_model, or script_origin", entnum));
    }

    if (pSelf->classname == sc->script_origin) {

        Com_DPrintf("cannot use the solid/notsolid commands on a script_origin entity( number %i )\n", pSelf->s.number);
        return;
    }

    pSelf->r.contents = 0;
    if (pSelf->classname != sc->script_model) {

        pSelf->s.eFlags |= 1;
    }

    SV_LinkEntity(pSelf);
}

BuiltinMethod ScriptEnt_GetMethod(const char **pName)
{
    const char *name;
    int i;

    name = *pName;
    for (i = 0; i < 12; i++) {
        if (methods[i].actionString && strcmp(name, methods[i].actionString) == 0) {
            *pName = methods[i].actionString;
            return methods[i].actionFunc;
        }
    }
    return NULL;
}

void InitScriptMover(gentity_t *pSelf)
{
    float fLight;
    vec3_t vColor;
    qboolean bLightSet;
    int r, g, b, a;

    if (level.spawnVar.spawnVarsValid) {
        bLightSet = G_SpawnFloat("light", "100", &fLight);
        if (bLightSet | G_SpawnVector("color", "1 1 1", vColor)) {
            r = (int)(vColor[0] * 255.0f);
            if (r >= 0x100)
                r = 0xff;

            g = (int)(vColor[1] * 255.0f);
            if (g > 0xff)
                g = 0xff;

            b = (int)(vColor[2] * 255.0f);
            if (b > 0xff)
                b = 0xff;

            a = (int)(fLight * 0.25f);
            if (a > 0xff)
                a = 0xff;

            pSelf->s.constantLight = r | (g << 8) | (b << 16) | (a << 24);
        }
    }

    pSelf->handler = 5;
    pSelf->r.svFlags = 0;
    pSelf->s.eType = 6;

    VectorCopy(pSelf->r.currentOrigin, pSelf->s.pos.trBase);
    pSelf->s.pos.trType = TR_STATIONARY;

    VectorCopy(pSelf->r.currentAngles, pSelf->s.apos.trBase);
    pSelf->s.apos.trType = TR_STATIONARY;

    pSelf->flags |= 0x1000;
}

void SP_script_origin(gentity_t *pSelf)
{
    InitScriptMover(pSelf);
    pSelf->r.contents = 0;
    SV_LinkEntity(pSelf);

    if (pSelf->s.constantLight != 0) {
        pSelf->s.eFlags |= 0x20;
    } else {
        pSelf->r.svFlags |= 1;
    }
}

void SP_script_model(gentity_t *pSelf)
{
    G_DObjUpdate(pSelf);
    InitScriptMover(pSelf);
    pSelf->r.svFlags |= 4;
    pSelf->r.contents = 0x2080;
    SV_LinkEntity(pSelf);
}

void SP_script_brushmodel(gentity_t *pSelf)
{
    SV_SetBrushModel(pSelf);
    InitScriptMover(pSelf);
    pSelf->r.contents = 1;
    SV_LinkEntity(pSelf);
}

void ScriptEntCmd_GravityMove(scr_entref_t entref)
{
    gentity_t *pSelf;
    vec3_t vVel;
    float fTime;
    trajectory_t *pTr;
    level_locals_t *lvl;

    pSelf = GetScriptMoverEntity(entref);

    Scr_GetVector(0, vVel);
    fTime = Scr_GetFloat(1);

    pTr = &pSelf->s.pos;
    lvl = &level;
    pTr->trTime = lvl->time;
    pTr->trDuration = (int)(fTime * 1000.0f);

    VectorCopy(pSelf->r.currentOrigin, pTr->trBase);
    VectorCopy(vVel, pTr->trDelta);
    pSelf->s.pos.trType = TR_GRAVITY;

    BG_EvaluateTrajectory(pTr, lvl->time, pSelf->r.currentOrigin);
    SV_LinkEntity(pSelf);
}

static inline __attribute__((always_inline)) void SetupVelocityTrajectory(gentity_t *pSelf, trajectory_t *pTr, vec3_t vCurrPos,
                                                                          const vec3_t vSpeed, float fTotalTime, float fAccelTime, float fDecelTime,
                                                                          float *pfDecelTime, float *pfSpeed, float *pfMidTime,
                                                                          vec3_t to, vec3_t result, vec3_t endPos)
{
    float speed;

    if (pTr->trType != TR_STATIONARY) {
        BG_EvaluateTrajectory(pTr, level.time, vCurrPos);
    }

    if (fAccelTime == 0.0f && fDecelTime == 0.0f) {

        pTr->trTime = level.time;
        pTr->trDuration = (int)(fTotalTime * 1000.0f);
        *pfMidTime = fTotalTime;
        *pfDecelTime = 0.0f;

        VectorCopy(vCurrPos, pTr->trBase);
        VectorCopy(vSpeed, pTr->trDelta);
        pTr->trType = TR_LINEAR_STOP;

        BG_EvaluateTrajectory(pTr, level.time, vCurrPos);
        BG_EvaluateTrajectory(pTr, level.time + pTr->trDuration, endPos);

        SV_LinkEntity(pSelf);
        return;
    }

    *pfMidTime = fTotalTime - fAccelTime - fDecelTime;
    *pfDecelTime = fDecelTime;
    speed = VectorLength(vSpeed);
    *pfSpeed = speed;

    if (fAccelTime == 0.0f) {

        VectorCopy(vCurrPos, to);

        if (*pfMidTime != 0.0f) {
            pTr->trTime = level.time;
            pTr->trDuration = (int)(*pfMidTime * 1000.0f);
            VectorCopy(vCurrPos, pTr->trBase);
            VectorCopy(vSpeed, pTr->trDelta);
            pTr->trType = TR_LINEAR_STOP;
        } else {
            pTr->trTime = level.time;
            pTr->trDuration = (int)(*pfDecelTime * 1000.0f);
            VectorCopy(vCurrPos, pTr->trBase);
            VectorCopy(vSpeed, pTr->trDelta);
            pTr->trType = TR_DECELERATE;
        }
    } else {

        pTr->trTime = level.time;
        pTr->trDuration = (int)(fAccelTime * 1000.0f);
        VectorCopy(vCurrPos, pTr->trBase);
        VectorCopy(vSpeed, pTr->trDelta);
        pTr->trType = TR_ACCELERATE;

        BG_EvaluateTrajectory(pTr, level.time + pTr->trDuration, to);
    }

    VectorMA(to, *pfMidTime, vSpeed, result);

    if (*pfDecelTime == 0.0f) {
        VectorCopy(result, endPos);
    } else {

        trajectory_t tr;
        tr.trType = TR_DECELERATE;
        tr.trTime = level.time;
        tr.trDuration = (int)(*pfDecelTime * 1000.0f);
        VectorCopy(result, tr.trBase);
        VectorCopy(vSpeed, tr.trDelta);
        BG_EvaluateTrajectory(&tr, level.time + tr.trDuration, endPos);
    }

    BG_EvaluateTrajectory(pTr, level.time, vCurrPos);
    SV_LinkEntity(pSelf);
}

void ScriptEntCmd_RotateVelocity(scr_entref_t entref)
{
    gentity_t *pSelf;
    vec3_t vSpeed;
    float fTotalTime, fAccelTime, fDecelTime;
    mover_ent_t *mover;
    trajectory_t *pTr;

    pSelf = GetScriptMoverEntity(entref);

    Scr_GetVector(0, vSpeed);
    ScriptEntCmdGetCommandTimes(&fTotalTime, &fAccelTime, &fDecelTime);

    mover = (&(pSelf)->mover);
    pTr = &pSelf->s.apos;

    SetupVelocityTrajectory(pSelf, pTr, pSelf->r.currentAngles, vSpeed,
                            fTotalTime, fAccelTime, fDecelTime,
                            &mover->aDecelTime, &mover->aSpeed, &mover->aMidTime,
                            mover->apos1, mover->apos2, mover->apos3);
}

static qboolean ScriptMover_Updatemove(trajectory_t *pTr, float *vCurrPos, float fSpeed, float fMidTime, float fDecelTime, const vec3_t vPos1, const vec3_t vPos2, const vec3_t vPos3)
{
    int trDuration;
    (void)vCurrPos;
    vec3_t vMove;
    float scale;

    trDuration = (int)(fMidTime * 1000.0f);

    if (pTr->trType == TR_ACCELERATE) {
        if (trDuration > 0) {

            pTr->trTime = level.time;
            pTr->trDuration = trDuration;
            VectorCopy(vPos1, pTr->trBase);

            VectorSubtract(vPos2, vPos1, vMove);
            scale = 1000.0f / (float)trDuration;
            VectorScale(vMove, scale, pTr->trDelta);
            pTr->trType = TR_LINEAR_STOP;
            return 0;
        }

    }

    if (pTr->trType == TR_LINEAR_STOP || pTr->trType == TR_ACCELERATE) {
        if (fDecelTime > 0.0f) {

            pTr->trTime = level.time;
            pTr->trDuration = (int)(fDecelTime * 1000.0f);
            VectorCopy(vPos2, pTr->trBase);

            VectorSubtract(vPos3, vPos2, vMove);
            Vec3Normalize(vMove);
            VectorScale(vMove, fSpeed, vMove);
            VectorCopy(vMove, pTr->trDelta);
            pTr->trType = TR_DECELERATE;
            return 0;
        }
    }

    if (pTr->trType == TR_GRAVITY) {

        BG_EvaluateTrajectory(pTr, level.time, pTr->trBase);
    }

    VectorCopy(vPos3, pTr->trBase);
    pTr->trTime = level.time;
    pTr->trType = TR_STATIONARY;
    return 1;
}

void Reached_ScriptMover(gentity_t *pEnt)
{
    qboolean bMoveFinished;
    mover_ent_t *mover = (&(pEnt)->mover);

    bMoveFinished = pEnt->s.pos.trType;
    if (bMoveFinished) {
        if (pEnt->s.pos.trTime + pEnt->s.pos.trDuration <= level.time) {
            bMoveFinished = ScriptMover_Updatemove(&pEnt->s.pos, pEnt->r.currentOrigin,
                                                   mover->speed, mover->midTime, mover->decelTime,
                                                   mover->pos1, mover->pos2, mover->pos3);

            BG_EvaluateTrajectory(&pEnt->s.pos, level.time, pEnt->r.currentOrigin);
            SV_LinkEntity(pEnt);

            if (bMoveFinished) {
                Scr_Notify(pEnt, SCR_CONST()->movedone, 0);
            }
        }
    }

    if (!pEnt->s.apos.trType)
        return;

    if (pEnt->s.apos.trTime + pEnt->s.apos.trDuration > level.time)
        return;

    bMoveFinished = ScriptMover_Updatemove(&pEnt->s.apos, pEnt->r.currentAngles,
                                           mover->aSpeed, mover->aMidTime, mover->aDecelTime,
                                           mover->apos1, mover->apos2, mover->apos3);

    BG_EvaluateTrajectory(&pEnt->s.apos, level.time, pEnt->r.currentAngles);
    SV_LinkEntity(pEnt);

    if (!bMoveFinished)
        return;

    pEnt->r.currentAngles[0] = AngleNormalize180(pEnt->r.currentAngles[0]);
    pEnt->r.currentAngles[1] = AngleNormalize360(pEnt->r.currentAngles[1]);
    pEnt->r.currentAngles[2] = AngleNormalize180(pEnt->r.currentAngles[2]);

    Scr_Notify(pEnt, SCR_CONST()->rotatedone, 0);
}

static void ScriptMover_SetupMove(trajectory_t *pTr, const vec3_t vPos, vec3_t vCurrPos,
                                  float fTotalTime, float fAccelTime, float fDecelTime,
                                  float *pfSpeed, float *pfMidTime, float *pfDecelTime,
                                  vec3_t vPos1, vec3_t vPos2, vec3_t vPos3)
{
    vec3_t vMove;
    vec3_t vMaxSpeed;
    float speed;
    float scale;

    VectorSubtract(vPos, vCurrPos, vMove);

    if (pTr->trType != TR_STATIONARY) {
        BG_EvaluateTrajectory(pTr, level.time, vCurrPos);
        VectorSubtract(vPos, vCurrPos, vMove);
    }

    if (fAccelTime == 0.0f && fDecelTime == 0.0f) {

        pTr->trTime = level.time;
        pTr->trDuration = (int)(fTotalTime * 1000.0f);
        *pfMidTime = fTotalTime;
        *pfDecelTime = 0.0f;
        VectorCopy(vPos, vPos3);

        VectorCopy(vCurrPos, pTr->trBase);
        scale = 1000.0f / (float)pTr->trDuration;
        VectorScale(vMove, scale, pTr->trDelta);
        pTr->trType = TR_LINEAR_STOP;

        BG_EvaluateTrajectory(pTr, level.time, vCurrPos);
    } else {

        *pfMidTime = fTotalTime - fAccelTime - fDecelTime;
        *pfDecelTime = fDecelTime;

        speed = VectorLength(vMove) * 2.0f / (fTotalTime * 2.0f - fAccelTime - fDecelTime);
        *pfSpeed = speed;

        Vec3NormalizeTo(vMove, vMaxSpeed);
        VectorScale(vMaxSpeed, speed, vMaxSpeed);

        if (fAccelTime == 0.0f) {

            VectorCopy(vCurrPos, vPos1);

            if (*pfMidTime != 0.0f) {

                pTr->trTime = level.time;
                pTr->trDuration = (int)(*pfMidTime * 1000.0f);
                VectorCopy(vCurrPos, pTr->trBase);

                VectorScale(vMaxSpeed, *pfMidTime, vMove);
                scale = 1000.0f / (float)pTr->trDuration;
                VectorScale(vMove, scale, pTr->trDelta);
                pTr->trType = TR_LINEAR_STOP;
            } else {

                pTr->trTime = level.time;
                pTr->trDuration = (int)(*pfDecelTime * 1000.0f);
                VectorCopy(vCurrPos, pTr->trBase);
                VectorCopy(vMaxSpeed, pTr->trDelta);
                pTr->trType = TR_DECELERATE;
            }
        } else {

            pTr->trTime = level.time;
            pTr->trDuration = (int)(fAccelTime * 1000.0f);
            VectorCopy(vCurrPos, pTr->trBase);
            VectorScale(vMaxSpeed, fAccelTime, vMove);
            scale = 1000.0f / (float)pTr->trDuration;
            VectorScale(vMove, scale, pTr->trDelta);
            pTr->trType = TR_ACCELERATE;

            BG_EvaluateTrajectory(pTr, level.time + pTr->trDuration, vPos1);
        }

        VectorMA(vPos1, *pfMidTime, vMaxSpeed, vPos2);
        VectorCopy(vPos, vPos3);

        BG_EvaluateTrajectory(pTr, level.time, vCurrPos);
    }
}

void ScriptEnt_RotateAxis(scr_entref_t entref, int iAxis)
{
    gentity_t *pSelf;
    float fMove;
    float fTotalTime, fAccelTime, fDecelTime;
    vec3_t vRot;
    mover_ent_t *mover;

    pSelf = GetScriptMoverEntity(entref);

    fMove = Scr_GetFloat(0);
    ScriptEntCmdGetCommandTimes(&fTotalTime, &fAccelTime, &fDecelTime);

    VectorCopy(pSelf->r.currentAngles, vRot);
    vRot[iAxis] += fMove;

    mover = (&(pSelf)->mover);
    ScriptMover_SetupMove(&pSelf->s.apos, vRot, pSelf->r.currentAngles,
                          fTotalTime, fAccelTime, fDecelTime,
                          &mover->aSpeed, &mover->aMidTime, &mover->aDecelTime,
                          mover->apos1, mover->apos2, mover->apos3);

    SV_LinkEntity(pSelf);
}

void ScriptEntCmd_RotateRoll(scr_entref_t entref)
{
    ScriptEnt_RotateAxis(entref, 2);
}

void ScriptEntCmd_RotateYaw(scr_entref_t entref)
{
    ScriptEnt_RotateAxis(entref, 1);
}

void ScriptEntCmd_RotatePitch(scr_entref_t entref)
{
    ScriptEnt_RotateAxis(entref, 0);
}

void ScriptEnt_MoveAxis(scr_entref_t entref, int iAxis)
{
    gentity_t *pSelf;
    float fMove;
    float fDecelTime, fAccelTime, fTotalTime;
    vec3_t vPos;
    mover_ent_t *mover;

    pSelf = GetScriptMoverEntity(entref);

    fMove = Scr_GetFloat(0);
    ScriptEntCmdGetCommandTimes(&fTotalTime, &fAccelTime, &fDecelTime);

    VectorCopy(pSelf->r.currentOrigin, vPos);
    vPos[iAxis] += fMove;

    mover = (&(pSelf)->mover);
    ScriptMover_SetupMove(&pSelf->s.pos, vPos, pSelf->r.currentOrigin,
                          fTotalTime, fAccelTime, fDecelTime,
                          &mover->speed, &mover->midTime, &mover->decelTime,
                          mover->pos1, mover->pos2, mover->pos3);

    SV_LinkEntity(pSelf);
}

void ScriptEntCmd_MoveZ(scr_entref_t entref)
{
    ScriptEnt_MoveAxis(entref, 2);
}

void ScriptEntCmd_MoveY(scr_entref_t entref)
{
    ScriptEnt_MoveAxis(entref, 1);
}

void ScriptEntCmd_MoveX(scr_entref_t entref)
{
    ScriptEnt_MoveAxis(entref, 0);
}

void ScriptEntCmd_RotateTo(scr_entref_t entref)
{
    gentity_t *pSelf;
    vec3_t vDest;
    float fDecelTime, fAccelTime, fTotalTime;
    vec3_t vRot;
    mover_ent_t *mover;
    int i;

    pSelf = GetScriptMoverEntity(entref);

    Scr_GetVector(0, vDest);
    ScriptEntCmdGetCommandTimes(&fTotalTime, &fAccelTime, &fDecelTime);

    for (i = 1; i < 4; i++) {
        vRot[i - 1] = pSelf->r.currentAngles[i - 1] + AngleSubtract(vDest[i - 1], pSelf->r.currentAngles[i - 1]);
    }

    mover = (&(pSelf)->mover);
    ScriptMover_SetupMove(&pSelf->s.apos, vRot, pSelf->r.currentAngles,
                          fTotalTime, fAccelTime, fDecelTime,
                          &mover->aSpeed, &mover->aMidTime, &mover->aDecelTime,
                          mover->apos1, mover->apos2, mover->apos3);

    SV_LinkEntity(pSelf);
}

void ScriptEntCmd_MoveTo(scr_entref_t entref)
{
    gentity_t *pSelf;
    vec3_t vPos;
    float fDecelTime, fAccelTime, fTotalTime;
    mover_ent_t *mover;

    pSelf = GetScriptMoverEntity(entref);

    Scr_GetVector(0, vPos);
    ScriptEntCmdGetCommandTimes(&fTotalTime, &fAccelTime, &fDecelTime);

    mover = (&(pSelf)->mover);
    ScriptMover_SetupMove(&pSelf->s.pos, vPos, pSelf->r.currentOrigin,
                          fTotalTime, fAccelTime, fDecelTime,
                          &mover->speed, &mover->midTime, &mover->decelTime,
                          mover->pos1, mover->pos2, mover->pos3);

    SV_LinkEntity(pSelf);
}
