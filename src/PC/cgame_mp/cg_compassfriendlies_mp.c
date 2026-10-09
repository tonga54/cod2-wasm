#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"

extern float floorf(float x);
extern float sinf(float x);
extern float cosf(float x);
extern float sqrtf(float x);

extern const dvar_t **dvar_compassSize;
extern const dvar_t **dvar_compassMinRange;
extern const dvar_t **dvar_compassMinSaturation;
extern const dvar_t **dvar_compassMaxRange;
extern const dvar_t **dvar_compassClampDist;
extern const dvar_t **dvar_compassClampSaturation;
extern const dvar_t **dvar_compassPingFadeTime;
extern const dvar_t **dvar_compassFade;

extern float CG_FadeHudMenu(const dvar_t *fadeDvar, int displayStartTime, int duration);
extern void CG_UpdateCompPointerOrientation(void);
extern void CG_ApplySplitScreenCompassScale(float *x, float *y, float *w, float *h);
extern void UI_DrawHandlePic(float x, float y, float w, float h, int horzAlign, int vertAlign, const vec_t *color, MaterialHandle hMaterial);
extern void CG_DrawRotatedPic(float x, float y, float width, float height, int horzAlign, int vertAlign, float angle, const vec_t *color, MaterialHandle material);
extern const float vectoyaw(const vec_t *vec);
extern const float AngleNormalize360(const float angle);
extern const vec_t Vec2Normalize(vec_t *v);

#define COMPASS_ACTOR_COUNT 64

#define CGS_COMPASS_BACK 0xc1e0
#define CGS_COMPASS_PING 0xc1e8
#define CGS_COMPASS_DIR(i) (0xbc6c + (i) * 4)
#define CGS_COMPASS_DOT 0xbc70

#define SNAP_PS_CLIENTNUM 0xd8

#define ES_EFLAGS 0x08
#define ES_CLIENTNUM 0xf0
#define ES_ETYPE 0xf4
#define ES_ORIGIN_X 0x1ec
#define ES_ORIGIN_Y 0x1f0
#define ES_LEAN 0x1fc

#define CENTITY_BIN_SIZE 0x224

#define PS_COMPASS_PACKED 0x5b0
#define PS_COMPASS_FRIEND_YAW 0x5b3
#define PS_COMPASS_EFLAGS 0xae

#define SNAP_PS_NUMCLIENTS 0x26b4
#define SNAP_PS_CLIENTBASE 0x26bc
#define SNAP_PI_STRIDE 0xf0

static inline __attribute__((always_inline)) float dvar_value(const dvar_t **dvpp)
{
    const dvar_t *dv = *dvpp;
    return dv->current.value;
}

void CG_ApplyCompassPointerRadiusScale(float *radiusScale);
void CG_CompassAddWeaponPingInfo(centity_t *cent, const vec_t *origin, int msec);
void CG_DrawCompassFriendlies(rectDef_t *rect, MaterialHandle material, vec_t *color);

void CG_ApplyCompassPointerRadiusScale(float *radiusScale)
{
    double rhs = (double)dvar_value(dvar_compassSize) * 43.75;
    *radiusScale = (float)((double)*radiusScale * rhs);
}

void CG_CompassAddWeaponPingInfo(centity_t *cent, const vec_t *origin, int msec)
{
    int eType = cent->nextState.eType;

    if (eType == 2)
        return;

    int localClientNum = cg->clientNum;

    clientInfo_t *localClientInfo = &cg->bgs.clientinfo[localClientNum];

    int entClientNum = cent->nextState.number;

    clientInfo_t *entClientInfo = &cg->bgs.clientinfo[entClientNum];

    if (localClientInfo == entClientInfo)
        return;

    int entTeam = entClientInfo->team;
    if (entTeam == 3)
        return;

    int cgTime = cg->time;
    compassactor_t *actor = &cg->compassActors[entClientNum];
    actor->beginFadeTime = cgTime + msec;

    int localTeam = localClientInfo->team;
    byte enemyFlag;
    if (localTeam == 0) {

        enemyFlag = 1;
    } else if (localTeam == entTeam) {

        enemyFlag = 0;
    } else {

        enemyFlag = 1;
    }
    actor->enemy = enemyFlag;

    if (actor->enemy == 0)
        return;

    actor->vLastPos[0] = origin[0];
    actor->vLastPos[1] = origin[1];
}

static float CompassDistToRadius(float dist)
{
    float maxRange = dvar_value(dvar_compassMaxRange);
    float minRange = dvar_value(dvar_compassMinRange);
    float minSat = dvar_value(dvar_compassMinSaturation);
    float compassSize = dvar_value(dvar_compassSize);

    float clampedDist = dist;
    if (clampedDist > maxRange)
        clampedDist = maxRange;

    float rangeFrac = (clampedDist - minRange) / (maxRange - minRange);
    float saturation = minSat + rangeFrac * (1.0f - minSat);

    return (float)((double)compassSize * 43.75 * (double)saturation);
}

void CG_DrawCompassFriendlies(rectDef_t *rect, MaterialHandle material, vec_t *color)
{
    float compassFadeOutAlpha;
    float centerX, centerY;
    int team;
    int i;

    const dvar_t *fadeDvar = *dvar_compassFade;
    int duration = (int)floorf(fadeDvar->current.value * 1000.0f + 0.5f);

    int displayStartTime = cg->compassFadeTime;
    compassFadeOutAlpha = CG_FadeHudMenu(fadeDvar, displayStartTime, duration);

    if (compassFadeOutAlpha == 0.0f)
        return;

    snapshot_t *snapPtr = cg->snap;
    int ourClientNum = snapPtr->ps.clientNum;
    clientInfo_t *ourCI = &cg->bgs.clientinfo[ourClientNum];
    int infoValid = ourCI->infoValid;

    if (infoValid == 0)
        return;

    team = ourCI->team;

    if (team == 3)
        return;

    CG_UpdateCompPointerOrientation();

    float compassSize = dvar_value(dvar_compassSize);
    centerX = rect->x + 0.5f * compassSize * rect->w;
    centerY = rect->y + rect->h + (-0.5f) * compassSize * rect->h;

    vec4_t fadedColor;
    fadedColor[0] = color[0];
    fadedColor[1] = color[1];
    fadedColor[2] = color[2];
    fadedColor[3] = color[3];

    if (team != 0) {

        snapshot_t *snapPtr2 = cg->snap;
        int numClients = snapPtr2->numEntities;

        for (i = 0; i < numClients; i++) {
            snapshot_t *snap = cg->snap;

            int clientIdx = snap->entities[i].number;

            centity_t *centEnt = &cg_entities[clientIdx];

            if (centEnt->nextState.eType != 1)
                continue;

            if (((centEnt->nextState.eFlags >> 16) & 0x02))
                continue;

            if ((int)clientIdx < 0)
                continue;

            clientInfo_t *ci = &cg->bgs.clientinfo[clientIdx];

            if (ci->infoValid == 0)
                continue;

            if (ci->team != team)
                continue;

            compassactor_t *actor = &cg->compassActors[clientIdx];
            int cgTime = cg->time;
            actor->iLastUpdate = cgTime;

            actor->vLastPos[0] = centEnt->lerpOrigin[0];
            actor->vLastPos[1] = centEnt->lerpOrigin[1];

            actor->fLastYaw = centEnt->lerpAngles[1];

            if (!(((centEnt->nextState.eFlags >> 16) & 0x40)))
                continue;

            cgTime = cg->time;
            if (actor->pingTime <= cgTime) {
                actor->pingTime = cgTime + 3000;
            }
        }

        snapPtr2 = cg->snap;

        int compassPacked = snapPtr2->ps.iCompassFriendInfo;
        if (compassPacked == 0)
            goto draw_friendlies;

        int packedClientNum = compassPacked & 0x3f;

        compassactor_t *packedActor = &cg->compassActors[packedClientNum];
        packedActor->iLastUpdate = cg->time;

        snapPtr2 = cg->snap;
        compassPacked = snapPtr2->ps.iCompassFriendInfo;
        int packedX = ((compassPacked & 0x7fc0) >> 4) - 0x3fc;
        int packedY = ((compassPacked & 0xff8000) >> 13) - 0x3fc;
        float deltaX = (float)packedX;
        float deltaY = (float)packedY;

        if (deltaX == 1024.0f || deltaX == -1020.0f ||
            deltaY == 1024.0f || deltaY == -1020.0f) {

            vec2_t dir;
            dir[0] = deltaX;
            dir[1] = deltaY;
            Vec2Normalize(dir);

            packedActor = &cg->compassActors[packedClientNum];
            packedActor->vLastPos[0] = dir[0];
            packedActor->vLastPos[1] = dir[1];
        } else {

            packedActor = &cg->compassActors[packedClientNum];
            float playerX = cg->refdef.vieworg[0];
            float playerY = cg->refdef.vieworg[1];
            packedActor->vLastPos[0] = playerX + deltaX;
            packedActor->vLastPos[1] = playerY + deltaY;
        }

        snapPtr2 = cg->snap;
        signed char packedYaw = (signed char)(snapPtr2->ps.iCompassFriendInfo >> 24);
        cg->compassActors[packedClientNum].fLastYaw =
            (float)packedYaw * 1.40625f;

        snapPtr2 = cg->snap;
        if (((snapPtr2->ps.eFlags >> 16) & 0x80)) {

            int cgTime2 = cg->time;
            compassactor_t *pActor2 = &cg->compassActors[packedClientNum];
            if (pActor2->pingTime <= cgTime2) {
                pActor2->pingTime = cgTime2 + 3000;
            }
        }
    }

draw_friendlies:

    {
        float origAlpha = color[3];
        if (origAlpha > compassFadeOutAlpha)
            color[3] = compassFadeOutAlpha;
    }

    {
        float a = fadedColor[3];
        if (a > compassFadeOutAlpha)
            a = compassFadeOutAlpha;
        fadedColor[3] = a;
    }

    for (i = 0; i < COMPASS_ACTOR_COUNT; i++) {
        compassactor_t *actor = &cg->compassActors[i];

        int lastUpdate = actor->iLastUpdate;
        int cgTime = cg->time;
        if (lastUpdate > cgTime) {
            actor->iLastUpdate = 0;
        }

        cgTime = cg->time;
        if (actor->iLastUpdate < cgTime - 0x320)
            continue;

        snapshot_t *snap2 = cg->snap;
        if (i == snap2->ps.clientNum)
            continue;

        float posX = actor->vLastPos[0];
        float posY = actor->vLastPos[1];
        unsigned int absX = *(unsigned int *)&actor->vLastPos[0] & 0x7fffffff;
        unsigned int absY = *(unsigned int *)&actor->vLastPos[1] & 0x7fffffff;
        float fabsX, fabsY;
        *(unsigned int *)&fabsX = absX;
        *(unsigned int *)&fabsY = absY;

        float iconAlpha2 = 1.0f;
        float radius;

        if (fabsX <= 1.0f && fabsY <= 1.0f) {

            float dirVec[2];
            dirVec[0] = posX;
            dirVec[1] = posY;
            float yawTo = vectoyaw(dirVec);
            yawTo = AngleNormalize360(yawTo - cg->compPointerYaw);

            float clampSat = dvar_value(dvar_compassClampSaturation);
            iconAlpha2 = clampSat + (1.0f - clampSat) * 0.5f;

            float compassSize2 = dvar_value(dvar_compassSize);
            float minRange = dvar_value(dvar_compassMinRange);
            float maxRange = dvar_value(dvar_compassMaxRange);
            float minSat = dvar_value(dvar_compassMinSaturation);
            float rangeFrac = (maxRange - minRange) / (maxRange - minRange);
            float saturation = minSat + rangeFrac * (1.0f - minSat);
            float radius = (float)((double)compassSize2 * 43.75 * (double)saturation);

            float radians = (float)((double)yawTo * 0.017453292519943295);
            float sinVal = sinf(radians);
            float cosVal = cosf(radians);

            float w = compassSize2 * 10.0f;
            float h = dvar_value(dvar_compassSize) * 10.0f;
            float x = centerX + w * (-0.5f) - radius * sinVal;
            float y = centerY + h * (-0.5f) - radius * cosVal;

            CG_ApplySplitScreenCompassScale(&x, &y, &w, &h);

            float refYaw = cg->refdefViewAngles[1];
            float actorYaw = actor->fLastYaw;
            float drawAngle = AngleNormalize360(refYaw - actorYaw);

            int pingTime = actor->pingTime;
            cgTime = cg->time;
            int pingFlash = 0;
            if (pingTime > cgTime) {
                int elapsed = pingTime - cgTime;
                int mod = elapsed % 500;
                if (mod > 249) {
                    pingFlash = 1;
                }
            }

            int beginFade = actor->beginFadeTime;
            float pingFadeTime = dvar_value(dvar_compassPingFadeTime);
            float fadeEnd = (float)beginFade + pingFadeTime * 1000.0f;
            MaterialHandle friendMat = NULL;
            float iconAlpha;

            if ((float)cgTime < fadeEnd) {

                if (cgTime >= beginFade) {
                    iconAlpha = 1.0f;
                } else {

                    iconAlpha = 1.0f + (float)(cgTime - beginFade) / (pingFadeTime * -1000.0f);
                }

                friendMat = cgs->media.compassping_friendlyfiring;
            } else {
                friendMat = NULL;
            }

            vec4_t actorColor;
            actorColor[0] = color[0];
            actorColor[1] = color[1];
            actorColor[2] = color[2];

            actorColor[3] = color[3] < compassFadeOutAlpha ? color[3] : compassFadeOutAlpha;

            iconAlpha = compassFadeOutAlpha < iconAlpha ? compassFadeOutAlpha : iconAlpha;

            if (pingFlash == 1) {

                MaterialHandle dotMat = cgs->media.friendMaterials[1];
                UI_DrawHandlePic(x, y, w, h, rect->horzAlign, rect->vertAlign, fadedColor, dotMat);
            } else {

                if (friendMat == NULL || iconAlpha != 1.0f) {

                    MaterialHandle dirMat = cgs->media.friendMaterials[pingFlash];
                    CG_DrawRotatedPic(x, y, w, h, rect->horzAlign, rect->vertAlign, drawAngle, actorColor, dirMat);
                }
                if (friendMat != NULL) {

                    CG_DrawRotatedPic(x, y, w, h, rect->horzAlign, rect->vertAlign, drawAngle, fadedColor, friendMat);
                }
            }

            continue;
        }

        vec2_t posDelta;
        posDelta[0] = posX - cg->refdef.vieworg[0];
        posDelta[1] = posY - cg->refdef.vieworg[1];

        float yawTo = vectoyaw(posDelta);
        yawTo = AngleNormalize360(yawTo - cg->compPointerYaw);

        float dist = sqrtf(posDelta[0] * posDelta[0] + posDelta[1] * posDelta[1]);

        float clampDistVal = dvar_value(dvar_compassClampDist);
        float maxRangeVal = dvar_value(dvar_compassMaxRange);
        float minRangeVal = dvar_value(dvar_compassMinRange);
        float minSatVal = dvar_value(dvar_compassMinSaturation);
        float clampSatVal = dvar_value(dvar_compassClampSaturation);
        float compassSzVal = dvar_value(dvar_compassSize);

        float alphaClampedDist;
        if (dist > clampDistVal) {
            alphaClampedDist = clampDistVal;
        } else if (dist >= maxRangeVal) {
            alphaClampedDist = dist;
        } else {
            alphaClampedDist = maxRangeVal;
        }
        {
            float alphaFrac = (alphaClampedDist - maxRangeVal) / (clampDistVal - maxRangeVal);
            iconAlpha2 = 1.0f + alphaFrac * (clampSatVal - 1.0f);
        }

        {
            float radiusDist = dist;
            if (radiusDist > maxRangeVal)
                radiusDist = maxRangeVal;
            if (radiusDist < minRangeVal)
                radiusDist = minRangeVal;
            float rangeFrac = (radiusDist - minRangeVal) / (maxRangeVal - minRangeVal);
            float saturation = minSatVal + rangeFrac * (1.0f - minSatVal);
            radius = (float)((double)compassSzVal * 43.75 * (double)saturation);
        }

        float radians2 = (float)((double)yawTo * 0.017453292519943295);
        float sinVal2 = sinf(radians2);
        float cosVal2 = cosf(radians2);

        float compassSzFinal = dvar_value(dvar_compassSize);
        float w2 = compassSzFinal * 10.0f;
        float h2 = dvar_value(dvar_compassSize) * 10.0f;

        float x2 = centerX + w2 * (-0.5f) - radius * sinVal2;
        float y2 = centerY + h2 * (-0.5f) - radius * cosVal2;

        CG_ApplySplitScreenCompassScale(&x2, &y2, &w2, &h2);

        float refYaw2 = cg->refdefViewAngles[1];
        float actorYaw2 = cg->compassActors[i].fLastYaw;
        float drawAngle2 = AngleNormalize360(refYaw2 - actorYaw2);

        int pingTime2 = cg->compassActors[i].pingTime;
        cgTime = cg->time;
        int pingFlash2 = 0;
        if (pingTime2 > cgTime) {
            int elapsed2 = pingTime2 - cgTime;
            int mod2 = elapsed2 % 500;
            if (mod2 > 249) {
                pingFlash2 = 1;
            }
        }

        int beginFade2 = cg->compassActors[i].beginFadeTime;
        float pingFadeTime2 = dvar_value(dvar_compassPingFadeTime);
        float fadeEnd2 = (float)beginFade2 + pingFadeTime2 * 1000.0f;
        MaterialHandle friendMat2 = NULL;

        if ((float)cgTime < fadeEnd2) {
            if (cgTime >= beginFade2) {
                iconAlpha2 = 1.0f;
            } else {
                iconAlpha2 = 1.0f + (float)(cgTime - beginFade2) / (pingFadeTime2 * -1000.0f);
            }
            friendMat2 = cgs->media.compassping_friendlyfiring;
        }

        vec4_t actorColor2;
        actorColor2[0] = color[0];
        actorColor2[1] = color[1];
        actorColor2[2] = color[2];
        actorColor2[3] = color[3] < compassFadeOutAlpha ? color[3] : compassFadeOutAlpha;

        iconAlpha2 = compassFadeOutAlpha < iconAlpha2 ? compassFadeOutAlpha : iconAlpha2;

        if (pingFlash2 == 1) {
            MaterialHandle dotMat2 = cgs->media.friendMaterials[1];
            UI_DrawHandlePic(x2, y2, w2, h2, rect->horzAlign, rect->vertAlign, fadedColor, dotMat2);
        } else {
            if (friendMat2 == NULL || iconAlpha2 != 1.0f) {
                MaterialHandle dirMat2 = cgs->media.friendMaterials[pingFlash2];
                CG_DrawRotatedPic(x2, y2, w2, h2, rect->horzAlign, rect->vertAlign, drawAngle2, actorColor2, dirMat2);
            }
            if (friendMat2 != NULL) {
                CG_DrawRotatedPic(x2, y2, w2, h2, rect->horzAlign, rect->vertAlign, drawAngle2, fadedColor, friendMat2);
            }
        }

    }

    {
        for (i = 0; i < COMPASS_ACTOR_COUNT; i++) {
            compassactor_t *actor2 = &cg->compassActors[i];

            if (actor2->enemy == 0)
                continue;

            int pingBeginFade = actor2->beginFadeTime;
            cg_t *cg2 = cg;
            float pingFadeVal = dvar_value(dvar_compassPingFadeTime);
            float pingEnd = (float)pingBeginFade + pingFadeVal * 1000.0f;
            int cgTime2 = cg2->time;
            if ((float)cgTime2 > pingEnd)
                continue;
            if (pingBeginFade == 0)
                continue;

            vec2_t posDelta2;
            posDelta2[0] = actor2->vLastPos[0] - cg2->refdef.vieworg[0];
            posDelta2[1] = actor2->vLastPos[1] - cg2->refdef.vieworg[1];

            float yaw = vectoyaw(posDelta2);
            yaw = AngleNormalize360(yaw - cg2->compPointerYaw);

            float pingDist = sqrtf(posDelta2[0] * posDelta2[0] + posDelta2[1] * posDelta2[1]);

            float pingRadius = CompassDistToRadius(pingDist);

            float pingRadians = (float)((double)yaw * 0.017453292519943295);
            float pingSin = sinf(pingRadians);
            float pingCos = cosf(pingRadians);

            float compassSz3 = dvar_value(dvar_compassSize);
            float pw = compassSz3 * 10.0f;
            float ph = dvar_value(dvar_compassSize) * 10.0f;

            float px = centerX + pw * (-0.5f) - pingRadius * pingSin;
            float py = centerY + ph * (-0.5f) - pingRadius * pingCos;

            CG_ApplySplitScreenCompassScale(&px, &py, &pw, &ph);

            float pingAlpha;
            int cgTime3 = cg2->time;
            if (pingBeginFade >= cgTime3) {
                pingAlpha = 1.0f;
            } else {
                float elapsed = (float)(cgTime3 - pingBeginFade);
                float fadeRange = dvar_value(dvar_compassPingFadeTime) * -1000.0f;
                pingAlpha = 1.0f + elapsed / fadeRange;
            }

            pingAlpha = compassFadeOutAlpha < pingAlpha ? compassFadeOutAlpha : pingAlpha;
            fadedColor[3] = pingAlpha;

            MaterialHandle pingMat = cgs->media.compassping_enemyfiring;
            UI_DrawHandlePic(px, py, pw, ph, rect->horzAlign, rect->vertAlign, fadedColor, pingMat);
        }
    }
}
