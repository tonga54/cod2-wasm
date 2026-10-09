#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include <string.h>

extern const dvar_t *cg_debugPosition;
extern const dvar_t *cg_headIconMinScreenRadius;
extern scr_const_t scr_const;
extern float fabsf(float);

COD2_ASSERT_FIELD(WeaponDef, fAnimHorRotateInc, 0x564);

extern void *Com_GetClientDObj(int clientNum, int localClientNum);
extern void BG_UpdatePlayerDObj(void *obj, entityState_t *es, byte *ci, int p4);
extern void BG_PlayerAnimation(void *obj, entityState_t *es, byte *ci);
extern void XAnimClearTreeGoalWeights(XAnimTree *pAnimTree, unsigned int animIndex, float blendTime);
extern void XAnimSetCompleteGoalWeight(void *pAnimTree, unsigned short animIndex, float goalWeight, float goalTime, float rate, int p6, int p7, int p8);
extern void Com_Printf(const char *fmt, ...);
extern void AnglesToAxis(float *angles, float *axis);
extern void CG_AddCEntityToScene(byte *body, void *obj, centity_t *cent);
extern void CG_AddPlayerWeapon(byte *body, int unused, centity_t *cent, int flag);
extern qboolean CG_DObjGetWorldTagPos(centity_t *cent, void *obj, unsigned short tag, float *origin);
extern void CL_DrawSprite(MaterialHandle material, byte *rgbaColor, float *origin, float radius, float minScreenRadius, int flags);
extern void XAnimClearTreeGoalWeightsStrict(void *pAnimTree, unsigned int animIndex, float blendTime);
extern int XAnimSetGoalWeight(void *pAnimTree, unsigned short animIndex, float goalWeight, float goalTime, float rate, int p6, int p7, int p8);
extern float XAnimGetWeight(void *pAnimTree, unsigned short animIndex);
extern int XAnimGetNumChildren(void *pXAnims, unsigned short animIndex);
extern unsigned short XAnimGetChildAt(void *pXAnims, unsigned short parentIndex, int childNum);
extern const char *XAnimGetAnimDebugName(void *pXAnims, unsigned short animIndex);
extern void XAnimCalcAbsDelta(void *pAnimTree, unsigned short animIndex, float *rot, float *trans);
extern void Com_Error(int level, const char *fmt, ...);
extern void *BG_GetWeaponDef(int weaponIndex);
extern float vectosignedyaw(float *axis);
extern float RotationToYaw(float *rot);
extern void YawToAxis(float yaw, float *axis);
extern void MatrixMultiply43(float *a, float *b, float *out);
extern void AxisToAngles(float *axis, float *angles);
extern void VectorAngleMultiply(float *trans, float yaw);
extern void CG_TraceCapsule(byte *trace, float *start, float *mins, float *maxs, float *end, int entityNum, int contentMask);
extern DObjAnimMat *CG_DObjGetLocalTagMatrix(const centity_t *cent, void *obj, unsigned int tagName);
extern const char *CL_GetConfigString(int index);
extern MaterialHandle CL_RegisterMaterial(const char *name, int flags);

extern void *imp_cg_headIconMinScreenRadius;
extern void *imp_cg_voiceIconSize;
extern void *imp_cg_scriptIconSize;
extern void *imp_cg_constantSizeHeadIcons;
extern void *imp_cg_youInKillCamSize;
extern void *imp_cg_connectionIconSize;
extern byte *cg_pmove_mins;

void CG_UpdatePlayerDObj(centity_t *cent);
void CG_ResetPlayerEntity(centity_t *cent);
static void CG_PlayerFloatSprite(centity_t *cent, MaterialHandle material, float additionalRadiusSize, int height, Bool fixedScreenSize);
void CG_PlayerSprites(centity_t *cent);
void CG_Player(centity_t *cent);
void CG_Corpse(centity_t *cent);

#define CI_STRIDE 1208

void CG_UpdatePlayerDObj(centity_t *cent)
{
    entityState_t *es;
    int clientNum;
    byte *ci;
    void *obj;

    if (!cent->nextValid)
        return;

    es = &cent->nextState;
    clientNum = es->clientNum;

    ci = ((char *)cg + offsetof(cg_t, bgs.clientinfo[0].infoValid)) + clientNum * CI_STRIDE;

    obj = Com_GetClientDObj(clientNum, cent->localClientNum);
    BG_UpdatePlayerDObj(obj, es, ci, 0);
}

void CG_ResetPlayerEntity(centity_t *cent)
{
    entityState_t *es;
    byte *ciBase;
    byte *ci;
    int clientNum;
    int deadFlag;
    void *pAnimTree;

    es = &cent->nextState;
    clientNum = es->clientNum;

    ciBase = ((char *)cg + offsetof(cg_t, bgs.anim_user)) + clientNum * CI_STRIDE;
    ci = ciBase + 0x14;

    deadFlag = es->eFlags & 0x20000;

    if (deadFlag) {

        if (cg_debugPosition->current.enabled)
            Com_Printf("%i ResetPlayerEntity yaw=%i\n", cent->nextState.number,
                       ((clientInfo_t *)ci)->torso.yawAngle);
        return;
    }

    {
        pAnimTree = ((clientInfo_t *)ci)->pXAnimTree ;
        if (pAnimTree) {

            XAnimClearTreeGoalWeights(pAnimTree, (*(unsigned short *)&cg->bgs.generic_human.root.index) , 0);

            XAnimSetCompleteGoalWeight(pAnimTree, (*(unsigned short *)&cg->bgs.animScriptData.torsoAnim) , 0.0f, 0.0f, 1.0f, 0, 0, 0);
            XAnimSetCompleteGoalWeight(pAnimTree, (*(unsigned short *)&cg->bgs.animScriptData.legsAnim) , 1.0f, 0.0f, 1.0f, 0, 0, 0);
            XAnimSetCompleteGoalWeight(pAnimTree, (*(unsigned short *)&cg->bgs.animScriptData.turningAnim) , 0.0f, 0.0f, 1.0f, 0, 0, 0);
        }

        memset(ciBase + 0x394, 0, 48);
        ((clientInfo_t *)ci)->legs.yawAngle = ((clientInfo_t *)ci)->playerAngles[1];
        ((clientInfo_t *)ci)->legs.yawing = 0;
        ((clientInfo_t *)ci)->legs.pitchAngle = 0;
        ((clientInfo_t *)ci)->legs.pitching = 0;

        memset(ciBase + 0x3c4, 0, 48);
        ((clientInfo_t *)ci)->torso.yawAngle = ((clientInfo_t *)ci)->playerAngles[1];
        ((clientInfo_t *)ci)->torso.yawing = 0;
        (*(int *)&((clientInfo_t *)ci)->torso.pitchAngle) = (*(int *)&((clientInfo_t *)ci)->playerAngles[0]);
        ((clientInfo_t *)ci)->torso.pitching = 0;
    }
}

static void CG_PlayerFloatSprite(centity_t *cent, MaterialHandle material, float additionalRadiusSize, int height, Bool fixedScreenSize)
{
    byte *snap;
    int clientNum;
    void *obj;
    vec3_t origin;
    byte rgbaColor[4];
    float radius;
    int drawFlags;
    float minScreenRadius;

    snap = (*(byte **)&cg->nextSnap);

    if (*(int *)(snap + 0x18) & 0xc00000) {
        clientNum = cent->nextState.number;
        if (clientNum == ((snapshot_t *)snap)->ps.clientNum) {
            if (!cg->renderingThirdPerson)
                return;
        }
    } else {
        clientNum = cent->nextState.number;
    }

    obj = Com_GetClientDObj(clientNum, cent->localClientNum);
    if (obj) {
        unsigned short tag = scr_const.j_head;
        if (CG_DObjGetWorldTagPos(cent, obj, tag, origin)) {
            origin[2] += (float)height + 21.0f;
            goto draw;
        }
    }

    origin[0] = cent->lerpOrigin[0];
    origin[1] = cent->lerpOrigin[1];
    origin[2] = cent->lerpOrigin[2];
    origin[2] += (float)height + 82.0f;

draw:
    if (fixedScreenSize) {
        radius = (additionalRadiusSize + 10.0f) * 0.0043f;
        drawFlags = 0x3000;
    } else {
        radius = additionalRadiusSize + 10.0f;
        drawFlags = 0;
    }

    rgbaColor[0] = 0xFF;
    rgbaColor[1] = 0xFF;
    rgbaColor[2] = 0xFF;
    rgbaColor[3] = 0xFF;

    minScreenRadius = cg_headIconMinScreenRadius->current.value;
    CL_DrawSprite(material, rgbaColor, origin, radius, minScreenRadius, drawFlags);
}

void CG_PlayerSprites(centity_t *cent)
{
    byte *snap;
    byte *ci;
    int iTeam;
    int localTeam;
    float additionalRadiusSize;
    int eFlags;
    int height;
    MaterialHandle material;

    ci = (char *)cg + cent->nextState.clientNum * CI_STRIDE;
    if (!*(int *)(ci + 0xe0914))
        return;
    iTeam = *(int *)(ci + 0xe0940);

    snap = (*(byte **)&cg->nextSnap);
    ci = (char *)cg + ((snapshot_t *)snap)->ps.clientNum * CI_STRIDE;
    if (!*(int *)(ci + 0xe0914))
        return;
    localTeam = *(int *)(ci + 0xe0940);

    {
        int headicon = cent->nextState.iHeadIcon;
        if (headicon) {
            int headiconTeam = cent->nextState.iHeadIconTeam;
            if (!headiconTeam || localTeam == 3 || localTeam == headiconTeam) {
                const char *str = CL_GetConfigString(headicon + 30);
                material = CL_RegisterMaterial(str, 7);
                if (material) {
                    CG_PlayerFloatSprite(cent, material, ((*(const dvar_t **)(imp_cg_scriptIconSize))->current.value), 0,
                                         (Bool)((*(const dvar_t **)(imp_cg_constantSizeHeadIcons))->current.enabled));
                    additionalRadiusSize = 16.0f + ((*(const dvar_t **)(imp_cg_scriptIconSize))->current.value);
                    goto check_local_player;
                }
            }
        }
    }

    additionalRadiusSize = 0.0f;

check_local_player:

    if (cent->nextState.number == cg->clientNum) {
        if (cg->inKillCam ) {

            height = (int)additionalRadiusSize;
            material = cgs->media.youInKillCamMaterial;
            CG_PlayerFloatSprite(cent, material, ((*(const dvar_t **)(imp_cg_youInKillCamSize))->current.value), height, 1);
            return;
        }
    }

    eFlags = cent->nextState.eFlags;
    if (eFlags & 0x80) {
        height = (int)additionalRadiusSize;
        material = cgs->media.connectionMaterial;
        CG_PlayerFloatSprite(cent, material, ((*(const dvar_t **)(imp_cg_connectionIconSize))->current.value), height, 0);
        return;
    }

    if (iTeam != localTeam && localTeam != 3)
        return;

    if (cent->voiceChatSpriteTime > cg->time ) {
        height = (int)additionalRadiusSize;
        material = cent->voiceChatSprite;
        CG_PlayerFloatSprite(cent, material, ((*(const dvar_t **)(imp_cg_voiceIconSize))->current.value), height, 0);
        return;
    }

    if (!(eFlags & 0x200000))
        return;

    additionalRadiusSize -= 5.0f;
    height = (int)additionalRadiusSize;
    material = cgs->media.balloonMaterial;
    CG_PlayerFloatSprite(cent, material, ((*(const dvar_t **)(imp_cg_voiceIconSize))->current.value), height, 0);
}

void CG_Player(centity_t *cent)
{
    entityState_t *es;
    int iClientNum;
    void *obj;
    byte body[0x74];

    es = &cent->nextState;

    if (es->eFlags & 0x20)
        return;

    if (es->eFlags & 0x20000)
        return;

    {
        byte *snap = (*(byte **)&cg->nextSnap);
        if (*(int *)(snap + 0x18) & 0xc00000) {
            if (es->number == ((snapshot_t *)snap)->ps.clientNum) {
                if (!cg->renderingThirdPerson)
                    return;
            }
        }
    }

    iClientNum = es->clientNum;

    obj = Com_GetClientDObj(es->number, cent->localClientNum);
    if (!obj)
        return;

    memset(body, 0, 0x74);
    (*(unsigned char *)&((centity_t *)body)->currentState.time2) = 0xFF;
    body[0x59] = 0xFF;
    body[0x5A] = 0xFF;
    body[0x5B] = 0xFF;

    {
        byte *ci = ((char *)cg + offsetof(cg_t, bgs.clientinfo[0].infoValid)) + iClientNum * CI_STRIDE;
        BG_PlayerAnimation(obj, es, ci);
    }

    if (es->eFlags & 0x300) {
        int turretEntNum = es->otherEntityNum;

        if (turretEntNum <= 0x3f) {

            goto render;
        }
        if (turretEntNum == 0x3ff) {
            goto render;
        }

        {
            byte *ciBase = ((char *)cg + offsetof(cg_t, bgs.anim_user)) + iClientNum * CI_STRIDE;
            byte *turretCi = ciBase + 0x14;
            byte *pLerpAnim;
            int animValue;
            byte *animPtr;
            centity_t *pTurretCEnt;
            void *turretObj;
            DObjAnimMat *tagMat;
            void *weapDef;
            void *pAnimTree;
            void *pXAnims;
            unsigned short baseAnim;
            unsigned short heightAnim;
            unsigned short leafAnim1;
            unsigned short leafAnim2;
            int numVertChildren, numHorChildren;
            int i, iPrevBlend, iHorChild;
            float localYaw, fDelta, fPrevTransZ, fPrevBlend;
            float prevHeight, fHeightRatio, tagHeight, frac;
            float rot[4], trans[3];
            float tagAxis[9];
            float turretAxis4x3[12];
            float localAxis4x3[12];
            float finalAxis4x3[12];
            int frameDuration;
            int broke;

            if (!*(int *)turretCi)
                goto render;

            pLerpAnim = ciBase + 0x394;

            animValue = ((lerpFrame_t *)pLerpAnim)->animationNumber;
            if (!animValue)
                goto render;

            animPtr = (byte *)((lerpFrame_t *)pLerpAnim)->animation;
            if (!animPtr)
                goto render;

            if (!(animPtr[0x50] & 4))
                goto render;

            pTurretCEnt = &cg_entities[turretEntNum];

            if (!pTurretCEnt->nextValid)
                goto render;

            turretObj = Com_GetClientDObj(pTurretCEnt->nextState.number, pTurretCEnt->localClientNum);
            if (!turretObj)
                goto render;

            tagMat = CG_DObjGetLocalTagMatrix(pTurretCEnt, turretObj,
                                              (unsigned int)scr_const.tag_weapon);
            if (!tagMat) {

                Com_Printf("CG_Player: turret tag not found\n");
                goto render;
            }

            frameDuration = cg->frametime;
            if (!frameDuration)
                goto render;

            weapDef = BG_GetWeaponDef(pTurretCEnt->nextState.weapon);

            pAnimTree = ((clientInfo_t *)turretCi)->pXAnimTree;
            pXAnims = (*(void **)&cg->bgs.animScriptData.animTree.anims) ;
            baseAnim = (unsigned short)(animValue & ~0x200);
            if (!pAnimTree)
                goto render;

            {
                float s = tagMat->transWeight;
                float sx = s * tagMat->quat[0];
                float sy = s * tagMat->quat[1];
                float sz = s * tagMat->quat[2];
                float xx = sx * tagMat->quat[0];
                float xy = sx * tagMat->quat[1];
                float xz = sx * tagMat->quat[2];
                float xw = sx * tagMat->quat[3];
                float yy = sy * tagMat->quat[1];
                float yz = sy * tagMat->quat[2];
                float yw = sy * tagMat->quat[3];
                float zz = sz * tagMat->quat[2];
                float zw = sz * tagMat->quat[3];

                tagAxis[0] = 1.0f - (yy + zz);
                tagAxis[1] = xy + zw;
                tagAxis[2] = xz - yw;
                tagAxis[3] = xy - zw;
                tagAxis[4] = 1.0f - (xx + zz);
                tagAxis[5] = xw + yz;
                tagAxis[6] = xz + yw;
                tagAxis[7] = yz - xw;
                tagAxis[8] = 1.0f - (xx + yy);
            }

            localYaw = vectosignedyaw(tagAxis);

            AnglesToAxis(pTurretCEnt->lerpAngles, turretAxis4x3);
            turretAxis4x3[9] = pTurretCEnt->lerpOrigin[0];
            turretAxis4x3[10] = pTurretCEnt->lerpOrigin[1];
            turretAxis4x3[11] = pTurretCEnt->lerpOrigin[2];

            tagHeight = (cent->lerpOrigin[0] - turretAxis4x3[9]) * turretAxis4x3[6] + (cent->lerpOrigin[1] - turretAxis4x3[10]) * turretAxis4x3[7] + (cent->lerpOrigin[2] - turretAxis4x3[11]) * turretAxis4x3[8];

            fDelta = tagHeight - tagMat->trans[2];

            XAnimClearTreeGoalWeightsStrict(pAnimTree, baseAnim, 0.0f);

            numVertChildren = XAnimGetNumChildren(pXAnims, baseAnim);
            if (!numVertChildren)
                Com_Error(1, "Player anim '%s' has no children",
                          XAnimGetAnimDebugName(pXAnims, baseAnim));

            fPrevBlend = 0.0f;
            prevHeight = 0.0f;
            iPrevBlend = 0;
            leafAnim2 = 0;
            leafAnim1 = 0;
            broke = 0;
            frac = 0.0f;

            for (i = 0; i < numVertChildren; i++) {
                float rawBlend;

                heightAnim = XAnimGetChildAt(pXAnims, baseAnim, i);

                XAnimSetGoalWeight(pAnimTree, heightAnim, 1.0f, 1.0f, 1.0f, 0, 0, 0);

                numHorChildren = XAnimGetNumChildren(pXAnims, heightAnim);
                if (!numHorChildren)
                    Com_Error(1, "Player anim '%s' has no children",
                              XAnimGetAnimDebugName(pXAnims, heightAnim));

                rawBlend = (float)numHorChildren * 0.5f - localYaw / ((WeaponDef *)weapDef)->fAnimHorRotateInc;

                if (rawBlend < 0.0f) {
                    iHorChild = 0;
                    rawBlend = 0.0f;
                } else if (rawBlend >= (float)(numHorChildren - 1)) {
                    iHorChild = numHorChildren - 1;
                    rawBlend = (float)iHorChild;
                } else {
                    iHorChild = (int)rawBlend;
                }

                frac = rawBlend - (float)iHorChild;

                leafAnim1 = XAnimGetChildAt(pXAnims, heightAnim, iHorChild);
                XAnimSetGoalWeight(pAnimTree, leafAnim1, 1.0f - frac,
                                   1.0f, 1.0f, 0, 0, 0);

                if (frac != 0.0f) {
                    leafAnim2 = XAnimGetChildAt(pXAnims, heightAnim, iHorChild + 1);
                    XAnimSetGoalWeight(pAnimTree, leafAnim2, frac,
                                       1.0f, 1.0f, 0, 0, 0);
                }

                XAnimCalcAbsDelta(pAnimTree, heightAnim, rot, trans);

                if (trans[2] >= fDelta) {
                    fPrevTransZ = prevHeight;
                    broke = 1;
                    break;
                }

                prevHeight = trans[2];
                fPrevBlend = frac;
                iPrevBlend = iHorChild;
            }

            if (!broke) {

                fPrevBlend = frac;
                iPrevBlend = iHorChild;
                fPrevTransZ = prevHeight;
            }

            XAnimClearTreeGoalWeightsStrict(pAnimTree, baseAnim, 0.0f);

            {
                float cw = XAnimGetWeight(pAnimTree, leafAnim1);
                float tw = 1.0f - frac;
                float spd = fabsf(cw - tw) * 1000.0f / (float)frameDuration;
                float gt = 0.0f;
                if (spd > 0.0f)
                    gt = 1.0f / spd;
                XAnimSetGoalWeight(pAnimTree, leafAnim1, tw, gt, 1.0f, 0, 0, 0);
            }

            if (frac != 0.0f) {
                float cw = XAnimGetWeight(pAnimTree, leafAnim2);
                float spd = fabsf(cw - frac) * 1000.0f / (float)frameDuration;
                float gt = 0.0f;
                if (spd > 0.0f)
                    gt = 1.0f / spd;
                XAnimSetGoalWeight(pAnimTree, leafAnim2, frac, gt, 1.0f, 0, 0, 0);
            }

            if (i == 0 || i == numVertChildren) {

                float cw = XAnimGetWeight(pAnimTree, heightAnim);
                float spd = fabsf(cw - 1.0f) * 1000.0f / (float)frameDuration;
                float gt = 0.0f;
                if (spd > 0.0f)
                    gt = 1.0f / spd;
                XAnimSetGoalWeight(pAnimTree, heightAnim, 1.0f, gt, 1.0f, 0, 0, 0);
            } else {

                unsigned short prevHeightAnim;

                fHeightRatio = (fDelta - fPrevTransZ) / (trans[2] - fPrevTransZ);

                {
                    float cw = XAnimGetWeight(pAnimTree, heightAnim);
                    float spd = fabsf(cw - fHeightRatio) * 1000.0f / (float)frameDuration;
                    float gt = 0.0f;
                    if (spd > 0.0f)
                        gt = 1.0f / spd;
                    XAnimSetGoalWeight(pAnimTree, heightAnim, fHeightRatio, gt, 1.0f, 0, 0, 0);
                }

                prevHeightAnim = XAnimGetChildAt(pXAnims, baseAnim, i - 1);

                {
                    float gw = 1.0f - fHeightRatio;
                    float cw = XAnimGetWeight(pAnimTree, prevHeightAnim);
                    float spd = fabsf(cw - gw) * 1000.0f / (float)frameDuration;
                    float gt = 0.0f;
                    if (spd > 0.0f)
                        gt = 1.0f / spd;
                    XAnimSetGoalWeight(pAnimTree, prevHeightAnim, gw, gt, 1.0f, 0, 0, 0);
                }

                {
                    unsigned short prevLeaf = XAnimGetChildAt(pXAnims, prevHeightAnim, iPrevBlend);
                    float gw = 1.0f - fPrevBlend;
                    float cw = XAnimGetWeight(pAnimTree, prevLeaf);
                    float spd = fabsf(cw - gw) * 1000.0f / (float)frameDuration;
                    float gt = 0.0f;
                    if (spd > 0.0f)
                        gt = 1.0f / spd;
                    XAnimSetGoalWeight(pAnimTree, prevLeaf, gw, gt, 1.0f, 0, 0, 0);

                    if (fPrevBlend != 0.0f) {
                        unsigned short prevLeaf2 = XAnimGetChildAt(pXAnims, prevHeightAnim, iPrevBlend + 1);
                        cw = XAnimGetWeight(pAnimTree, prevLeaf2);
                        spd = fabsf(cw - fPrevBlend) * 1000.0f / (float)frameDuration;
                        gt = 0.0f;
                        if (spd > 0.0f)
                            gt = 1.0f / spd;
                        XAnimSetGoalWeight(pAnimTree, prevLeaf2, fPrevBlend, gt, 1.0f, 0, 0, 0);
                    }
                }
            }

            XAnimCalcAbsDelta(pAnimTree, baseAnim, rot, trans);

            VectorAngleMultiply(trans, localYaw);

            {
                float rotYaw = RotationToYaw(rot);
                YawToAxis(localYaw + rotYaw, localAxis4x3);
            }
            localAxis4x3[9] = trans[0] + tagMat->trans[0];
            localAxis4x3[10] = trans[1] + tagMat->trans[1];
            localAxis4x3[11] = tagHeight;

            MatrixMultiply43(localAxis4x3, turretAxis4x3, finalAxis4x3);

            AxisToAngles(finalAxis4x3, cent->lerpAngles);
            cent->lerpOrigin[0] = finalAxis4x3[9];
            cent->lerpOrigin[1] = finalAxis4x3[10];
            cent->lerpOrigin[2] = finalAxis4x3[11];

            {
                float start[3], end[3];
                byte trace[0x44];
                float *traceBounds = (float *)cg_pmove_mins;

                start[0] = cent->lerpOrigin[0];
                start[1] = cent->lerpOrigin[1];
                start[2] = pTurretCEnt->lerpOrigin[2];

                end[0] = cent->lerpOrigin[0];
                end[1] = cent->lerpOrigin[1];
                end[2] = cent->lerpOrigin[2];

                CG_TraceCapsule(trace, start, traceBounds, traceBounds,
                                end, cent->nextState.number, 0x2810011);

                if (*(float *)trace < 1.0f) {
                    cent->lerpOrigin[2] = start[2] + (end[2] - start[2]) * (*(float *)trace);
                }
            }
        }
    }

render:

    AnglesToAxis(cent->lerpAngles, (float *)&((centity_t *)body)->currentState.pos.trDuration);

    ((GfxEntity *)body)->origin[0] = cent->lerpOrigin[0];
    ((GfxEntity *)body)->origin[1] = cent->lerpOrigin[1];
    ((GfxEntity *)body)->origin[2] = cent->lerpOrigin[2];

    ((GfxEntity *)body)->lighting.baseCoords[0] = cent->lerpOrigin[0];
    ((GfxEntity *)body)->lighting.baseCoords[1] = cent->lerpOrigin[1];

    {
        float z = cent->lerpOrigin[2] + es->fTorsoHeight;

        if (es->eFlags & 8)
            z += 12.0f;
        else if (es->eFlags & 4)
            z += 20.0f;
        else
            z += 32.0f;

        ((GfxEntity *)body)->lighting.baseCoords[2] = z;
    }

    ((GfxEntity *)body)->reType = 0;

    ((GfxEntity *)body)->renderFxFlags = 0x80;

    CG_AddCEntityToScene(body, obj, cent);

    if (es->eFlags & 0x2)
        return;

    CG_AddPlayerWeapon(body, 0, cent, 1);
}

void CG_Corpse(centity_t *cent)
{
    entityState_t *es;
    byte *ci;
    int clientNum;
    void *obj;
    byte body[0x74];

    es = &cent->nextState;

    if (es->eFlags & 0x20)
        return;

    clientNum = cent->nextState.number;

    if ((unsigned)(clientNum - 64) >= 8)
        return;
    ci = (byte *)&((cgs_t *)cgs)->corpseinfo[clientNum - 64];

    obj = Com_GetClientDObj(clientNum, cent->localClientNum);
    BG_UpdatePlayerDObj(obj, es, ci, 0);

    obj = Com_GetClientDObj(cent->nextState.number, cent->localClientNum);
    if (!obj)
        return;

    memset(body, 0, 0x74);

    (*(unsigned char *)&((centity_t *)body)->currentState.time2) = 0xFF;
    body[0x59] = 0xFF;
    body[0x5A] = 0xFF;
    body[0x5B] = 0xFF;

    BG_PlayerAnimation(obj, es, ci);

    AnglesToAxis(cent->lerpAngles, (float *)&((centity_t *)body)->currentState.pos.trDuration);

    {
        float *lerpOrigin = cent->lerpOrigin;
        ((GfxEntity *)body)->origin[0] = lerpOrigin[0];
        ((GfxEntity *)body)->origin[1] = lerpOrigin[1];
        ((GfxEntity *)body)->origin[2] = lerpOrigin[2];

        ((GfxEntity *)body)->lighting.baseCoords[0] = lerpOrigin[0];
        ((GfxEntity *)body)->lighting.baseCoords[1] = lerpOrigin[1];

        ((GfxEntity *)body)->lighting.baseCoords[2] = lerpOrigin[2] + es->fTorsoHeight;
    }

    if (es->eFlags & 8) {
        ((GfxEntity *)body)->lighting.baseCoords[2] += 12.0f;
    } else if (es->eFlags & 4) {
        ((GfxEntity *)body)->lighting.baseCoords[2] += 20.0f;
    } else {
        ((GfxEntity *)body)->lighting.baseCoords[2] += 32.0f;
    }

    ((GfxEntity *)body)->reType = 0;

    ((GfxEntity *)body)->renderFxFlags = 0x80;

    CG_AddCEntityToScene(body, obj, cent);
}
