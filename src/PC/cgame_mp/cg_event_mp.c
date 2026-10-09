#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "bytematch.h"

extern char **cg_dvar_debug;
extern char **cg_dvar_footsteps;
extern char **cg_uiglob;
extern char **cg_dvar_shellshock_min;
extern char **cg_dvar_shellshock_max;
extern char **cg_dvar1;
extern char **cg_dvar2;
extern void *imp_eventnames;

extern void Com_Printf(const char *msg, ...);
extern void Com_DPrintf(const char *msg, ...);
extern void Com_Error(int level, const char *msg, ...);
extern void *BG_GetWeaponDef(int weapon);
extern int BG_WeaponIsClipOnly(int weapon);
extern int CG_PlayEntitySoundAlias(int entNum, int alias);
extern int CG_PlaySoundAlias(int entitynum, const vec_t *origin, snd_alias_list_t *aliasList);
extern int CG_PlaySoundAliasByName(int entNum, const vec_t *origin, const char *name);
extern int CG_PlaySoundAliasAsMasterByName(int entNum, void *origin, const char *name);
extern void CG_FireWeapon(centity_t *cent, int weaponId, int hand);
extern void CG_EjectWeaponBrass(entityState_t *es, int weaponId);
extern void CG_PrepOffHand(entityState_t *es, int weaponId, int eventParm);
extern void CG_UseOffHand(centity_t *cent, int weaponId, int eventParm);
extern void CG_SetEquippedOffHand(int weaponId);
extern void CG_SelectWeaponIndex(int weaponId);
extern void CG_OutOfAmmoChange(void);
extern void CG_SwitchOffHandCmd(void);
extern void CG_MenuShowNotify(int val);

extern void CG_StartShakeCamera(float scale, int duration, const vec_t *src, float radius);
extern void CG_BulletHitEvent(int otherEntNum, void *position, void *dir, void *reflect, int surfType, int event);
extern void CG_BulletHitClientEvent(int otherEntNum, void *position, int surfType, int event);
extern void CG_CompassAddWeaponPingInfo(void *ent, void *position, int duration);
extern void CG_PriorityCenterPrint(const char *msg, float scale, int priority);
extern void CL_DeathMessagePrint(const char *attackerName, float *attackerColor, const char *targetName, float *victimColor, const char *iconShader, float iconWidth, float iconHeight, float *iconColor, int iconHorzFlip);
extern void CG_DrawScoreboard_GetTeamColor(int team, float *color);
extern void CL_SetADS(int val);
extern void CG_CalcEntityLerpPositions(centity_t *cent);
extern void CG_CheckOpenWaitingScriptMenu(void);
extern void ByteToDir(int dirByte, float *dir);
extern void AngleVectors(float *angles, float *forward, void *right, float *up);
extern void FX_PlayEffect(int effectId, void *origin, float *dir);
extern void FX_PlayEntityEffect(int effectId, void *origin, int boneIndex, int *entityInfo);
extern void FX_WarpTime(int time);
extern int FX_GetBoneIndex(int entNum, int tagName);
extern const char *CL_GetConfigString(int index);
extern unsigned short SL_GetString(const char *str, int a2);
extern void Scr_SetString(scr_string_t *str, unsigned int a2);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern void I_strncat(char *dest, int maxlen, const char *src);
extern const char *va(const char *fmt, ...);

#define ES_NUMBER 0x00
#define ES_ETYPE 0x04
#define ES_EFLAGS 0x08
#define ES_APOS_BASE 0x3c
#define ES_TIME 0x54
#define ES_ORIGIN2 0x5c
#define ES_ANGLES2 0x68
#define ES_OTHERENTNUM 0x74
#define ES_ATTACKERENTNUM 0x78
#define ES_SURFTYPE 0x88
#define ES_INDEX 0x8c
#define ES_CLIENTNUM 0x90
#define ES_EVENTPARM 0xa0
#define ES_EVENTSEQ 0xa4
#define ES_WEAPON 0xc8
#define ES_LEANF 0xd4
#define ES_DMGFLAGS 0xd8

#define CENT_NEXTSTATE 0xf0
#define CENT_EVPARM 0x190
#define CENT_EVSEQ 0x194
#define CENT_EVENTS 0x198
#define CENT_EVPARMS 0x1a8
#define CENT_NEXTVALID 0x1e0
#define CENT_PREVEVSEQ 0x1e4
#define CENT_MISCTIME 0x1e8
#define CENT_LERPORIGIN 0x1ec
#define CENT_ETYPE 0xf4

#define CG_CLIENTINFO 0xe0900

#define SNAP_FLAGS 0x18
#define SNAP_PS_CLIENTNUM 0xd8
#define SNAP_PS_WEAPON 0x5a0

#define CGS_SND_FOOTSTEP1_FIRST 0xbeb8
#define CGS_SND_FOOTSTEP1_THIRD 0xbf14
#define CGS_SND_FOOTSTEP2_THIRD 0xbf70
#define CGS_SND_FOOTSTEP2_FIRST 0xbfcc
#define CGS_SND_FOOTSTEP3_FIRST 0xc028
#define CGS_SND_FOOTSTEP3_THIRD 0xc084
#define CGS_SND_SPRINT_FIRST 0xc0e0
#define CGS_SND_SPRINT_THIRD 0xc13c
#define CGS_SND_PRONE_LOOP_3P 0xc198
#define CGS_SND_PRONE_LOOP_1P_NONVIEW 0xc19c
#define CGS_SND_PRONE_LOOP_1P_VIEW 0xc1a0
#define CGS_SND_PRONE_STOP_1P 0xc1a4
#define CGS_SND_WEAPON_PICKUP 0xc1a8
#define CGS_SND_NIGHTVISION_ON 0xc1b8
#define CGS_SND_NIGHTVISION_OFF 0xc1bc
#define CGS_SND_NOAMMO 0xbc84
#define CGS_SND_MELEE_HIT 0xbc88
#define CGS_SND_BULLET_WHIZBY 0xbc90
#define CGS_SND_BULLET_FLESH 0xbcec
#define CGS_SND_GRENADE_BOUNCE 0xbd48
#define CGS_FXLOOKUP 0xc200
#define CGS_FX_CUSTOM 0x5f80
#define CGS_FX_DEATHFX 0x67c0

static const float f_0_01 = 0.009999999776482582f;
static const float f_4_0 = 4.0f;
static const float f_12_0 = 12.0f;
static const float f_24_0 = 24.0f;
static const float f_26_0 = 26.0f;
static const float f_100_0 = 100.0f;
static const float f_0_9 = 0.8999999761581421f;
static const float f_neg16 = -16.0f;
static const float f_1_4 = 1.399999976158142f;
static const float f_2_8 = 2.799999952316284f;

#define WEAPON_DATA_STRIDE 109

static int weaponDataOffset(int weapon)
{
    return weapon * WEAPON_DATA_STRIDE;
}

static char *CG_WeaponInfoBase(void)
{
    return *(char **)imp_cg_weapons;
}

static char *CG_ItemInfoBase(void)
{
    return *(char **)imp_cg_items;
}

static char *BG_ItemListBase(void)
{
    return (char *)imp_bg_itemlist;
}

static char **CG_EventNames(void)
{
    return (char **)imp_eventnames;
}

void CG_EntityEvent(centity_t *cent, int event);
void CG_CheckEvents(centity_t *cent);

static void PlayProneSound(int entNum, int isFirstPerson, int soundOffset)
{
    int alias = *(int *)((char *)cgs + soundOffset);
    CG_PlayEntitySoundAlias(entNum, alias);
}

void CG_EntityEvent(centity_t *cent, int event)
{
    entityState_t *es;
    char *position;
    int eventParm;
    int clientNum;
    char isFirstPerson;
    char isViewerFlag;
    int entNum;
    float dir[3];
    float reflect[3];
    float forward[3];
    float up[3];
    char *snap;
    int idx;
    int weapon;

    float attackerColor[4];
    float victimColor[4];
    float iconColor[4];
    int target;
    int attacker;
    char targetName[34];
    char attackerName[34];
    float iconWidth;
    const char *iconShader;
    int iconHorzFlip;
    char *victimCI;
    char *attackerCI;
    unsigned short tagName;
    int boneIndex;

    if (event == 0) {

        if ((*(dvar_t **)(cg_dvar_debug))->current.enabled != 0) {

            Com_Printf((const char *)"CG_EntityEvent:ZERO EVENT\n");
        }
        return;
    }

    position = (char *)cent + CENT_LERPORIGIN;

    es = &cent->nextState;

    eventParm = es->eventParm;

    snap = (char *)cg->nextSnap;
    isFirstPerson = 0;
    isViewerFlag = 0;
    if (*(int *)(snap + SNAP_FLAGS) & 0xc00000) {
        int esNumber = es->number;
        if (esNumber == *(int *)(snap + SNAP_PS_CLIENTNUM)) {
            isFirstPerson = 1;
            isViewerFlag = 1;
        }
    }

    if ((*(dvar_t **)(cg_dvar_debug))->current.enabled != 0) {

        Com_Printf((const char *)"ent:%3i  event:%3i ", es->number, event);

        if ((*(dvar_t **)(cg_dvar_debug))->current.enabled != 0) {

            char **eventNames = CG_EventNames();
            Com_Printf((const char *)"CG_EntityEvent:%s\n", eventNames[event]);
        }
    }

    clientNum = es->clientNum;

    if ((unsigned int)clientNum >= 0x40) {
        clientNum = 0;
    }

    weapon = es->weapon;
    if (weapon != 0) {
        BG_GetWeaponDef(weapon);
    }

    if (event >= 1 && event <= 23) {

        int sndIdx = event - 1;

        if ((*(dvar_t **)(cg_dvar_footsteps))->current.enabled != 0) {

            if (isFirstPerson) {

                CG_PlayEntitySoundAlias(es->number,
                                        cgs->media.stepRunSoundPlayer[sndIdx]);
            } else {

                CG_PlayEntitySoundAlias(es->number,
                                        cgs->media.stepRunSound[sndIdx]);
            }
        }

        entNum = es->number;

        if (isViewerFlag) {

            CG_PlayEntitySoundAlias(entNum, cgs->media.runningEquipmentSoundPlayer);
        } else {

            CG_PlayEntitySoundAlias(entNum, cgs->media.runningEquipmentSound);
        }
        return;
    }

    if (event >= 24 && event <= 46) {

        int sndIdx = event - 24;

        if ((*(dvar_t **)(cg_dvar_footsteps))->current.enabled != 0) {
            if (isFirstPerson) {

                CG_PlayEntitySoundAlias(es->number,
                                        cgs->media.stepWalkSoundPlayer[sndIdx]);
            } else {

                CG_PlayEntitySoundAlias(es->number,
                                        cgs->media.stepWalkSound[sndIdx]);
            }
        }

        entNum = es->number;

        if (isViewerFlag) {

            CG_PlayEntitySoundAlias(entNum, cgs->media.walkingEquipmentSoundPlayer);
        } else {

            CG_PlayEntitySoundAlias(entNum, cgs->media.walkingEquipmentSound);
        }
        return;
    }

    if (event >= 47 && event <= 69) {

        int sndIdx = event - 47;

        if ((*(dvar_t **)(cg_dvar_footsteps))->current.enabled != 0) {
            if (isFirstPerson) {

                CG_PlayEntitySoundAlias(es->number,
                                        cgs->media.stepProneSoundPlayer[sndIdx]);
            } else {

                CG_PlayEntitySoundAlias(es->number,
                                        cgs->media.stepProneSound[sndIdx]);
            }
        }

        entNum = es->number;

        if (isViewerFlag) {

            CG_PlayEntitySoundAlias(entNum, cgs->media.walkingEquipmentSoundPlayer);
        } else {

            CG_PlayEntitySoundAlias(entNum, cgs->media.walkingEquipmentSound);
        }
        return;
    }

    if (event >= 70 && event <= 92) {

        int sndIdx = event - 70;

        if (isFirstPerson) {

            CG_PlayEntitySoundAlias(es->number,
                                    cgs->media.stepRunSoundPlayer[sndIdx]);
        } else {

            CG_PlayEntitySoundAlias(es->number,
                                    cgs->media.stepRunSound[sndIdx]);
        }

        entNum = es->number;

        if (isViewerFlag) {

            CG_PlayEntitySoundAlias(entNum, cgs->media.runningEquipmentSoundPlayer);
        } else {

            CG_PlayEntitySoundAlias(entNum, cgs->media.runningEquipmentSound);
        }
        return;
    }

    if (event >= 93 && event <= 115) {

        int sndIdx = event - 93;

        if (isFirstPerson) {

            CG_PlayEntitySoundAlias(es->number,
                                    cgs->media.landSoundPlayer[sndIdx]);
        } else {

            CG_PlayEntitySoundAlias(es->number,
                                    cgs->media.landSound[sndIdx]);
        }

        if (clientNum == cg->predictedPlayerState.clientNum) {

            float neg = -(float)eventParm;
            cg->landChange = neg;

            cg->landTime = cg->time;
        }
        return;
    }

    if (event >= 116 && event <= 138) {

        int sndIdx = event - 116;

        if (isFirstPerson) {

            CG_PlayEntitySoundAlias(es->number,
                                    cgs->media.landSoundPlayer[sndIdx]);
        } else {

            CG_PlayEntitySoundAlias(es->number,
                                    cgs->media.landSound[sndIdx]);
        }

        CG_PlayEntitySoundAlias(es->number,
                                cgs->media.landDmgSound);

        if (clientNum == cg->predictedPlayerState.clientNum) {

            float ssMinVal = (*(dvar_t **)(cg_dvar_shellshock_min))->current.value;
            float parm = (float)eventParm * f_0_01;
            float ssMaxVal = (*(dvar_t **)(cg_dvar_shellshock_max))->current.value;
            float delta = ssMaxVal - ssMinVal;
            float shellshock = parm * delta + ssMinVal;

            if (shellshock > f_12_0) {

                float tilt = (shellshock - f_12_0) / f_26_0;
                tilt = tilt * f_4_0 + f_4_0;
                int tiltInt = (int)tilt;

                float tiltVal;
                if (tiltInt > 24) {
                    tiltVal = f_24_0;
                } else if (tiltInt <= 0) {
                    return;
                } else {
                    tiltVal = (float)tiltInt;
                }

                cg->landChange = -tiltVal;

                cg->landTime = cg->time;
            }
        }
        return;
    }

    {
        int jmpIdx = event - 0x8b;
        if ((unsigned int)jmpIdx > 0x3b) {

            char **eventNames = CG_EventNames();

            Com_Error(1, (const char *)"\x15Unknown event: '%s'", eventNames[event]);
            return;
        }

        switch (event) {

        case 0x93:
        case 0xa4:
        case 0xa8:
        case 0xa9:
        case 0xaa:
        case 0xab:
        case 0xac:
            return;

        case 0xb2:
        case 0xb8:
        case 0xc0:
        case 0xc1:
            Com_Error(1, (const char *)"\x15Unknown event: '%s'", CG_EventNames()[event]);
            return;

        case 0x8b: {
            CG_PlayEntitySoundAlias(es->number,
                                    cgs->media.foliageMovement);
            return;
        }

        case 0x8c: {
            if (clientNum != cg->predictedPlayerState.clientNum) {

                char **eventNames2 = CG_EventNames();
                Com_DPrintf((const char *)"Event %s just for client %i was sent to other clients\n", eventNames2[0x8c], clientNum);
                return;
            }

            char *ui = *cg_uiglob;
            if (*(int *)(ui + 0xc)  != 0)
                return;

            *(int *)(ui + 0x8)  = 0;
            return;
        }

        case 0x8d: {
            if (clientNum != cg->predictedPlayerState.clientNum) {
                char **eventNames2 = CG_EventNames();
                Com_DPrintf((const char *)"Event %s just for client %i was sent to other clients\n", eventNames2[0x8d], clientNum);
                return;
            }

            char *ui = *cg_uiglob;
            if (*(int *)(ui + 0xc)  != 0)
                return;

            *(int *)(ui + 0x8)  = 1;
            return;
        }

        case 0x8e: {
            if (clientNum != cg->predictedPlayerState.clientNum) {
                char **eventNames2 = CG_EventNames();
                Com_DPrintf((const char *)"Event %s just for client %i was sent to other clients\n", eventNames2[0x8e], clientNum);
                return;
            }

            char *ui = *cg_uiglob;
            if (*(int *)(ui + 0xc)  != 0)
                return;

            *(int *)(ui + 0x8)  = 2;
            return;
        }

        case 0x8f: {
            if (clientNum != cg->predictedPlayerState.clientNum) {
                char **eventNames2 = CG_EventNames();
                Com_DPrintf((const char *)"Event %s just for client %i was sent to other clients\n", eventNames2[0x8f], clientNum);
                return;
            }

            if (cg->demoType != 0)
                return;
            if ((*(dvar_t **)(cg_dvar1))->current.enabled != 0)
                return;
            if ((*(dvar_t **)(cg_dvar2))->current.enabled != 0)
                return;

            int cgTime = cg->time;
            int prevTime = cg->stepTime;
            int timeDiff = cgTime - prevTime;
            float viewAngle = 0.0f;

            if (timeDiff <= 99) {

                int remaining = 100 - timeDiff;
                viewAngle = (float)remaining * cg->stepChange;
                viewAngle /= f_100_0;
                viewAngle *= f_0_9;
            }

            int parmAdj = eventParm - 128;
            viewAngle += (float)parmAdj;
            cg->stepChange = viewAngle;

            float curAngle = cg->stepChange;
            if (curAngle > f_24_0) {

                cg->stepChange = 24.0f;
            } else if (curAngle < f_neg16) {

                cg->stepChange = -16.0f;
            }

            cg->stepTime = cg->time;
            return;
        }

        case 0x90:
        case 0x91: {
            int itemIndex = es->eventParm;
            if (itemIndex <= 0)
                return;
            if (itemIndex >= *(int *)imp_bg_numItems)
                return;

            char *itemDefs = CG_ItemInfoBase();
            char *itemData = itemDefs + itemIndex * 9 * 4;

            if (event == 0x90) {

                CG_PlayEntitySoundAlias(es->number,
                                        *(int *)(itemData + 0x1c) );
            } else {

                CG_PlayEntitySoundAlias(es->number,
                                        *(int *)(itemData + 0x20) );
            }

            snap = (char *)cg->nextSnap;
            if (!(*(int *)(snap + SNAP_FLAGS) & 0xc00000))
                return;
            if (es->number != *(int *)(snap + SNAP_PS_CLIENTNUM))
                return;

            {
                int itemid = itemIndex;
                char *itemInfoBase = BG_ItemListBase();
                char *item = itemInfoBase + itemid * (1 + 2 * 5) * 4;
                int weapId = *(int *)(item + 0x20) ;
                if (*(int *)(item + 0x1c)  != 1)
                    return;

                char *weapDef = (char *)BG_GetWeaponDef(weapId);

                if ((*(int *)&((WeaponDef *)weapDef)->weapClass)  == 9)
                    return;

                if ((*(int *)&((WeaponDef *)weapDef)->offhandClass)  != 0) {

                    if (cg->equippedOffHand != 0)
                        return;

                    CG_SetEquippedOffHand(weapId);
                } else {

                    if (cg->weaponSelect != 0)
                        return;

                    CG_SelectWeaponIndex(weapId);
                }
            }
            return;
        }

        case 0x96:
        {
            if (isFirstPerson) {
                weapon = es->weapon;
                int wepOff = weaponDataOffset(weapon);
                char *wepDefs = CG_WeaponInfoBase();
                char *wepData = wepDefs + wepOff * 4;
                int alias = (*(int *)&((weaponInfo_t *)wepData)->reloadSoundPlayer) ;
                if (alias != 0) {

                    CG_PlayEntitySoundAlias(es->number, alias);
                    return;
                }

                alias = (*(int *)&((weaponInfo_t *)wepData)->reloadEmptySoundPlayer) ;
                if (alias != 0) {
                    CG_PlayEntitySoundAlias(es->number, alias);
                    return;
                }
            } else {
                weapon = es->weapon;
                int wepOff = weaponDataOffset(weapon);
                char *wepDefs = CG_WeaponInfoBase();
                char *wepData = wepDefs + wepOff * 4;

                int alias = (*(int *)&((weaponInfo_t *)wepData)->reloadSound) ;
                if (alias != 0) {
                    CG_PlayEntitySoundAlias(es->number, alias);
                    return;
                }

                alias = (*(int *)&((weaponInfo_t *)wepData)->reloadEmptySound) ;
                if (alias != 0) {
                    CG_PlayEntitySoundAlias(es->number, alias);
                    return;
                }
            }
            return;
        }

        case 0x97:
        {
            if (isFirstPerson) {
                weapon = es->weapon;
                int wepOff = weaponDataOffset(weapon);
                char *wepDefs = CG_WeaponInfoBase();
                char *wepData = wepDefs + wepOff * 4;
                int alias = (*(int *)&((weaponInfo_t *)wepData)->reloadEmptySoundPlayer) ;
                if (alias != 0) {

                    CG_PlayEntitySoundAlias(es->number, alias);
                    return;
                }

                alias = (*(int *)&((weaponInfo_t *)wepData)->reloadSoundPlayer) ;
                if (alias != 0) {
                    CG_PlayEntitySoundAlias(es->number, alias);
                    return;
                }
            } else {
                weapon = es->weapon;
                int wepOff = weaponDataOffset(weapon);
                char *wepDefs = CG_WeaponInfoBase();
                char *wepData = wepDefs + wepOff * 4;

                int alias = (*(int *)&((weaponInfo_t *)wepData)->reloadEmptySound) ;
                if (alias != 0) {
                    CG_PlayEntitySoundAlias(es->number, alias);
                    return;
                }

                alias = (*(int *)&((weaponInfo_t *)wepData)->reloadSound) ;
                if (alias != 0) {
                    CG_PlayEntitySoundAlias(es->number, alias);
                    return;
                }
            }
            return;
        }

        case 0x98:
        {
            if (isFirstPerson) {
                weapon = es->weapon;
                int wepOff = weaponDataOffset(weapon);
                char *wepDefs = CG_WeaponInfoBase();
                char *wepData = wepDefs + wepOff * 4;
                int alias = (*(int *)&((weaponInfo_t *)wepData)->reloadStartSoundPlayer) ;
                if (alias != 0) {

                    CG_PlayEntitySoundAlias(es->number, alias);
                    return;
                }

                alias = (*(int *)&((weaponInfo_t *)wepData)->reloadStartSound) ;
                if (alias != 0) {
                    CG_PlayEntitySoundAlias(es->number, alias);
                }
            } else {
                weapon = es->weapon;
                int wepOff = weaponDataOffset(weapon);
                char *wepDefs = CG_WeaponInfoBase();
                char *wepData = wepDefs + wepOff * 4;

                int alias = (*(int *)&((weaponInfo_t *)wepData)->reloadEndSound) ;
                if (alias != 0) {
                    CG_PlayEntitySoundAlias(es->number, alias);
                }
            }
            return;
        }

        case 0x99:
        {
            if (isFirstPerson) {
                weapon = es->weapon;
                int wepOff = weaponDataOffset(weapon);
                char *wepDefs = CG_WeaponInfoBase();
                char *wepData = wepDefs + wepOff * 4;
                int alias = (*(int *)&((weaponInfo_t *)wepData)->reloadEndSoundPlayer) ;
                if (alias != 0) {

                    CG_PlayEntitySoundAlias(es->number, alias);
                    return;
                }

                alias = (*(int *)&((weaponInfo_t *)wepData)->reloadEndSound) ;
                if (alias != 0) {
                    CG_PlayEntitySoundAlias(es->number, alias);
                }
            } else {
                weapon = es->weapon;
                int wepOff = weaponDataOffset(weapon);
                char *wepDefs = CG_WeaponInfoBase();
                char *wepData = wepDefs + wepOff * 4;

                int alias = (*(int *)&((weaponInfo_t *)wepData)->reloadStartSound) ;
                if (alias != 0) {
                    CG_PlayEntitySoundAlias(es->number, alias);
                }
            }
            return;
        }

        case 0x92:
        {
            int w = es->weapon;
            if (!BG_WeaponIsClipOnly(w)) {

                CG_PlayEntitySoundAlias(es->number,
                                        cgs->media.noAmmoSound);
            }

            snap = (char *)cg->nextSnap;
            if (!(*(int *)(snap + SNAP_FLAGS) & 0xc00000))
                return;
            if (es->number != *(int *)(snap + SNAP_PS_CLIENTNUM))
                return;

            CG_OutOfAmmoChange();
            return;
        }

        case 0x94:
        {
            snap = (char *)cg->nextSnap;
            if (!(*(int *)(snap + SNAP_FLAGS) & 0xc00000))
                return;
            if (es->number != *(int *)(snap + SNAP_PS_CLIENTNUM))
                return;

            CG_MenuShowNotify(4);

            CG_SwitchOffHandCmd();
            return;
        }

        case 0x95:
        {
            snap = (char *)cg->nextSnap;
            if (!(*(int *)(snap + SNAP_FLAGS) & 0xc00000))
                return;
            if (es->number != *(int *)(snap + SNAP_PS_CLIENTNUM))
                return;

            CL_SetADS(0);
            return;
        }

        case 0x9a:
        {
            weapon = es->weapon;
            int wepOff = weaponDataOffset(weapon);
            char *wepDefs = CG_WeaponInfoBase();
            char *wepData = wepDefs + wepOff * 4;
            int alias = (*(int *)&((weaponInfo_t *)wepData)->raiseSound) ;
            if (alias != 0) {

                CG_PlayEntitySoundAlias(es->number, alias);
            }
            return;
        }

        case 0x9b:
        {
            weapon = es->weapon;
            int wepOff = weaponDataOffset(weapon);
            char *wepDefs = CG_WeaponInfoBase();
            char *wepData = wepDefs + wepOff * 4;
            int alias = (*(int *)&((weaponInfo_t *)wepData)->putawaySound) ;
            if (alias != 0) {

                CG_PlayEntitySoundAlias(es->number, alias);
            }
            return;
        }

        case 0x9c:
        {
            weapon = es->weapon;
            int wepOff = weaponDataOffset(weapon);
            char *wepDefs = CG_WeaponInfoBase();
            char *wepData = wepDefs + wepOff * 4;
            int alias = (*(int *)&((weaponInfo_t *)wepData)->altSwitchSound) ;
            if (alias != 0) {

                CG_PlayEntitySoundAlias(es->number, alias);
            }
            return;
        }

        case 0xb0:
        {

            CG_FireWeapon(cent, 0xb0, 0);

            CG_FireWeapon(cent, 0xb0, 1);
            return;
        }

        case 0xb1:
        {

            CG_FireWeapon(cent, 0xb1, 2);

            CG_FireWeapon(cent, 0xb1, 3);
            return;
        }

        case 0xaf:
        {

            CG_StartShakeCamera(0.05f, 100, (const vec_t *)position, 100.0f);

            CG_FireWeapon(cent, 0xaf, 0);

            {
                int ep = eventParm;
                int entStride = ep * 16 + ep;
                entStride = ep + entStride * 8;
                char *ents = (char *)cg_entities;
                char *entData = ents + entStride * 4;

                if (((centity_t *)entData)->nextValid == 0)
                    return;
                if (((centity_t *)entData)->nextState.eType != 1)
                    return;
                snap = (char *)cg->nextSnap;
                int snapWeapon = *(int *)(snap + SNAP_PS_WEAPON) ;
                if (snapWeapon == cent->nextState.number)
                    return;

                CG_CompassAddWeaponPingInfo(entData, position, 50);
            }
            return;
        }

        case 0x9d:
        {
            weapon = es->weapon;
            int wepOff = weaponDataOffset(weapon);
            char *wepDefs = CG_WeaponInfoBase();
            char *wepData = wepDefs + wepOff * 4;
            int alias = (*(int *)&((weaponInfo_t *)wepData)->pullbackSound) ;
            if (alias != 0) {

                CG_PlayEntitySoundAlias(es->number, alias);
            }
            return;
        }

        case 0x9e:
        case 0x9f:
        case 0xa0:
        {

            CG_FireWeapon(cent, event, 0);
            return;
        }

        case 0xa1: {
            if (isFirstPerson) {
                weapon = es->weapon;
                int wepOff = weaponDataOffset(weapon);
                char *wepDefs = CG_WeaponInfoBase();
                char *wepData = wepDefs + wepOff * 4;
                int alias = (*(int *)&((weaponInfo_t *)wepData)->rechamberSoundPlayer) ;
                if (alias != 0) {

                    CG_PlayEntitySoundAlias(es->number, alias);
                    return;
                }

                alias = (*(int *)&((weaponInfo_t *)wepData)->rechamberSound) ;
                if (alias != 0) {
                    CG_PlayEntitySoundAlias(es->number, alias);
                }
            } else {
                weapon = es->weapon;
                int wepOff = weaponDataOffset(weapon);
                char *wepDefs = CG_WeaponInfoBase();
                char *wepData = wepDefs + wepOff * 4;
                int alias = (*(int *)&((weaponInfo_t *)wepData)->meleeSwipeSound) ;
                if (alias != 0) {

                    CG_PlayEntitySoundAlias(es->number, alias);
                }
            }
            return;
        }

        case 0xa2: {

            CG_EjectWeaponBrass(es, 0xa2);
            return;
        }

        case 0xa3: {
            weapon = es->weapon;
            int wepOff = weaponDataOffset(weapon);
            char *wepDefs = CG_WeaponInfoBase();
            char *wepData = wepDefs + wepOff * 4;
            int alias = (*(int *)&((weaponInfo_t *)wepData)->meleeSwipeSound) ;
            if (alias != 0) {

                CG_PlayEntitySoundAlias(es->number, alias);
            }
            return;
        }

        case 0xa5:
        {

            CG_PrepOffHand(es, 0xa5, es->eventParm);
            return;
        }

        case 0xa6:
        {

            CG_UseOffHand(cent, 0xa6, es->eventParm);
            return;
        }

        case 0xa7:
        {

            if (!isFirstPerson)
                return;
            CG_SetEquippedOffHand(es->eventParm);
            return;
        }

        case 0xad:
        {
            CG_PlayEntitySoundAlias(es->otherEntityNum,
                                    cgs->media.meleeHit);
            return;
        }

        case 0xae:
        {
            CG_PlaySoundAlias(es->otherEntityNum, position,
                              cgs->media.meleeHitOther);
            return;
        }

        case 0xc5:
        {

            dir[0] = 0.0f;
            dir[1] = 0.0f;
            dir[2] = 1.0f;

            CG_PlaySoundAlias(0x3fe, position,
                              cgs->media.grenadeExplodeSound[0]);

            {
                char *fxLookup = (char *)cgs->media.fx;
                char *fxData = *(char **)(fxLookup + 4);
                int fxId = *(int *)(fxData + 0x33c) ;
                if (fxId != 0) {

                    FX_PlayEffect(fxId, position, dir);
                }
            }
            return;
        }

        case 0xb5:
        case 0xb6:
        case 0xb7:
        {

            ByteToDir(es->eventParm, dir);

            ByteToDir(es->eventParm2, reflect);

            CG_BulletHitEvent(es->otherEntityNum, position,
                              dir, reflect, es->surfType, event);
            return;
        }

        case 0xb9:
        case 0xba:
        {

            CG_BulletHitClientEvent(es->otherEntityNum, position,
                                    es->surfType, event);
            return;
        }

        case 0xbb:
        {

            ByteToDir(es->eventParm, dir);

            int surfType = es->surfType;
            CG_PlaySoundAlias(0x3fe, position,
                              cgs->media.grenadeBounceSound[surfType]);

            {
                char *fxLookup = (char *)cgs->media.fx;
                char *fxData = *(char **)(fxLookup + 4);
                int fxId = *(int *)(fxData + 0x2e0 + surfType * 4);
                if (fxId != 0) {

                    FX_PlayEffect(fxId, position, dir);
                }
            }
            return;
        }

        case 0xbc:
        {

            ByteToDir(es->eventParm, dir);

            int surfType2 = es->surfType;
            CG_PlaySoundAlias(0x3fe, position,
                              cgs->media.grenadeExplodeSound[surfType2]);

            {
                char *fxLookup = (char *)cgs->media.fx;
                char *fxData = *(char **)(fxLookup + 4);
                int fxId = *(int *)(fxData + 0x33c + surfType2 * 4);
                if (fxId != 0) {

                    FX_PlayEffect(fxId, position, dir);
                }
            }

            weapon = es->weapon;
            {
                int wepOff = weaponDataOffset(weapon);
                char *wepDefs = CG_WeaponInfoBase();
                int fxId = *(int *)(wepDefs + 0x164 + wepOff * 4);
                if (fxId != 0) {

                    FX_PlayEffect(fxId, position, dir);
                }

                weapon = es->weapon;
                wepOff = weaponDataOffset(weapon);

                int sndAlias = *(int *)(wepDefs + 0x168 + wepOff * 4);
                if (sndAlias != 0) {

                    CG_PlaySoundAlias(0x3fe, position, sndAlias);
                }
            }
            return;
        }

        case 0xbd:
        {

            ByteToDir(es->eventParm, dir);

            weapon = es->weapon;
            {
                int wepOff = weaponDataOffset(weapon);
                char *wepDefs = CG_WeaponInfoBase();

                int surfType3 = es->surfType;
                CG_PlaySoundAlias(0x3fe, position,
                                  cgs->media.rocketExplodeSound[surfType3]);

                {
                    char *fxLookup = (char *)cgs->media.fx;
                    char *fxData = *(char **)(fxLookup + 4);
                    int surfType4 = es->surfType;
                    int fxId = *(int *)(fxData + 0x398 + surfType4 * 4);
                    if (fxId != 0) {

                        FX_PlayEffect(fxId, position, dir);
                    }
                }

                weapon = es->weapon;
                wepOff = weaponDataOffset(weapon);
                int fxId2 = *(int *)(wepDefs + 0x164 + wepOff * 4);
                if (fxId2 != 0) {

                    FX_PlayEffect(fxId2, position, dir);
                }

                weapon = es->weapon;
                wepOff = weaponDataOffset(weapon);
                int sndAlias = *(int *)(wepDefs + 0x168 + wepOff * 4);
                if (sndAlias != 0) {

                    CG_PlaySoundAlias(0x3fe, position, sndAlias);
                }
            }

            cg->nomarks = 0;
            return;
        }

        case 0xbe:
        {

            cg->nomarks = 1;

            ByteToDir(es->eventParm, dir);

            int surfType5 = es->surfType;
            CG_PlaySoundAlias(0x3fe, position,
                              cgs->media.rocketExplodeSound[surfType5]);

            {
                char *fxLookup = (char *)cgs->media.fx;
                char *fxData = *(char **)(fxLookup + 4);
                int surfType6 = es->surfType;
                int fxId = *(int *)(fxData + 0x398 + surfType6 * 4);
                if (fxId != 0) {

                    FX_PlayEffect(fxId, position, dir);
                }
            }

            weapon = es->weapon;
            {
                int wepOff = weaponDataOffset(weapon);
                char *wepDefs = CG_WeaponInfoBase();
                int fxId = *(int *)(wepDefs + 0x164 + wepOff * 4);
                if (fxId != 0) {

                    FX_PlayEffect(fxId, position, dir);
                }

                weapon = es->weapon;
                wepOff = weaponDataOffset(weapon);

                int sndAlias = *(int *)(wepDefs + 0x168 + wepOff * 4);
                if (sndAlias != 0) {

                    CG_PlaySoundAlias(0x3fe, position, sndAlias);
                }
            }

            cg->nomarks = 0;
            return;
        }

        case 0xbf:
        {

            ByteToDir(es->eventParm, dir);

            weapon = es->weapon;
            {
                int wepOff = weaponDataOffset(weapon);
                char *wepDefs = CG_WeaponInfoBase();
                int fxId = *(int *)(wepDefs + 0x164 + wepOff * 4);
                if (fxId != 0) {

                    FX_WarpTime(es->time);

                    weapon = es->weapon;
                    wepOff = weaponDataOffset(weapon);
                    fxId = *(int *)(wepDefs + 0x164 + wepOff * 4);
                    FX_PlayEffect(fxId, position, dir);

                    FX_WarpTime(cg->time);
                }

                weapon = es->weapon;
                wepOff = weaponDataOffset(weapon);
                int sndAlias = *(int *)(wepDefs + 0x168 + wepOff * 4);
                if (sndAlias == 0)
                    return;

                if (*(char *)((char *)es + 0x0a)  & 1) {
                    int cgTime2 = cg->time;
                    int esTime = es->time;
                    if (cgTime2 - esTime > 49)
                        return;
                }

                CG_PlaySoundAlias(0x3fe, position, sndAlias);
            }
            return;
        }

        case 0xb3:
        {
            int csIndex = es->eventParm + 0x24e;
            const char *csStr = CL_GetConfigString(csIndex);
            CG_PlaySoundAliasByName(es->number,
                                    (char *)&es->pos.trBase, csStr);
            return;
        }

        case 0xb4:
        {
            int csIndex = es->eventParm + 0x24e;
            const char *csStr = CL_GetConfigString(csIndex);
            CG_PlaySoundAliasAsMasterByName(es->number,
                                            (char *)&es->pos.trBase, csStr);
            return;
        }

        case 0xc4:
        {
            int duration = es->angles2[1];
            int esTime = es->time;
            float intensity = es->angles2[0];
            CG_StartShakeCamera(intensity, esTime, (const vec_t *)position, *(float *)&duration);
            return;
        }

        case 0xc6:
        {

            attackerColor[0] = 1.0f;
            attackerColor[1] = 1.0f;
            attackerColor[2] = 1.0f;
            attackerColor[3] = 1.0f;
            victimColor[0] = 1.0f;
            victimColor[1] = 1.0f;
            victimColor[2] = 1.0f;
            victimColor[3] = 1.0f;
            iconColor[0] = 1.0f;
            iconColor[1] = 1.0f;
            iconColor[2] = 1.0f;
            iconColor[3] = 1.0f;

            target = es->otherEntityNum;
            attacker = es->attackerEntityNum;

            int ep = es->eventParm;
            if ((ep & 0x80) == 0) {

                char *weapDef = (char *)BG_GetWeaponDef(ep);

                const char *killIcon = ((WeaponDef *)weapDef)->killIcon ;
                if (*killIcon == '\0') {

                    iconWidth = f_1_4;
                    iconShader = (const char *)"killicondied";
                    iconHorzFlip = 0;
                } else {

                    iconShader = killIcon;

                    int isWideIcon = ((WeaponDef *)weapDef)->wideKillIcon ;
                    if (isWideIcon) {
                        iconWidth = f_2_8;
                    } else {
                        iconWidth = f_1_4;
                    }

                    iconHorzFlip = ((WeaponDef *)weapDef)->flipKillIcon  != 0;
                }
            } else {

                int mod = (ep & 0x7f) - 7;
                if ((unsigned int)mod > 5) {

                    iconWidth = f_1_4;
                    iconShader = (const char *)"killicondied";
                    iconHorzFlip = 0;
                } else {
                    switch (mod) {
                    case 0:
                        iconWidth = f_1_4;
                        iconShader = (const char *)"killiconmelee";
                        iconHorzFlip = 0;
                        break;
                    case 1:
                        iconWidth = f_1_4;
                        iconShader = (const char *)"killiconcrush";
                        iconHorzFlip = 0;
                        break;
                    case 2:
                        iconWidth = f_1_4;
                        iconShader = (const char *)"killiconheadshot";
                        iconHorzFlip = 0;
                        break;
                    case 3:
                        iconWidth = f_1_4;
                        iconShader = (const char *)"killiconsuicide";
                        iconHorzFlip = 0;
                        break;
                    case 4:
                        iconWidth = f_1_4;
                        iconShader = (const char *)"killiconfalling";
                        iconHorzFlip = 0;
                        break;
                    default:
                        iconWidth = f_1_4;
                        iconShader = (const char *)"killicondied";
                        iconHorzFlip = 0;
                        break;
                    }
                }
            }

            if ((unsigned int)target > 63) {

                Com_Error(1, (const char *)"\x15"
                                           "CG_Obituary: target out of range");
            }

            {
                int tgt = target;
                int tgtOff = tgt * (1 + (tgt * 5 * 16 - tgt * 5) * 2);

            }
            {
                char *clientInfoBase = (char *)cg + CG_CLIENTINFO + target * 1208;
                victimCI = ((char *)clientInfoBase + offsetof(clientInfo_t, name[8]));

                if (((clientInfo_t *)clientInfoBase)->infoValid == 0)
                    return;

                I_strncpyz(targetName, ((char *)clientInfoBase + offsetof(clientInfo_t, name[20])), 0x20);

                I_strncat(targetName, 0x22, (const char *)"^7");

                CG_DrawScoreboard_GetTeamColor(*(int *)(((char *)victimCI + offsetof(clientInfo_t, oldteam)) - 0x14), victimColor);

                int localClient = cg->clientNum;
                char *localCI = (char *)cg + CG_CLIENTINFO + localClient * 1208;
                if (((clientInfo_t *)localCI)->infoValid == 0)
                    return;
            }

            if ((unsigned int)attacker <= 63) {

                char *atkInfoBase = (char *)cg + CG_CLIENTINFO + attacker * 1208;
                attackerCI = ((char *)atkInfoBase + offsetof(clientInfo_t, name[8]));

                if (((clientInfo_t *)atkInfoBase)->infoValid == 0)
                    return;

                I_strncpyz(attackerName, ((char *)atkInfoBase + offsetof(clientInfo_t, name[20])), 0x20);

                I_strncat(attackerName, 0x22, (const char *)"^7");

                CG_DrawScoreboard_GetTeamColor(*(int *)(((char *)attackerCI + offsetof(clientInfo_t, oldteam)) - 0x14), attackerColor);

                snap = (char *)cg->nextSnap;
                if (target == *(int *)(snap + SNAP_PS_CLIENTNUM)) {

                    I_strncpyz(cg->killerName, attackerName, 0x20);
                }
            } else {

                attackerName[0] = '\0';
                attacker = 0x3fe;
                attackerCI = (char *)0;
            }

            if (attacker == target) {

                attackerName[0] = '\0';
            } else {

                snap = (char *)cg->nextSnap;
                int localClientNum = *(int *)(snap + SNAP_PS_CLIENTNUM);

                if (attacker == localClientNum) {

                    if (attackerCI != (char *)0) {
                        int atkTeam = *(int *)(((char *)attackerCI + offsetof(clientInfo_t, oldteam)) - 0x14);
                        if (atkTeam != 0 && atkTeam == *(int *)(((char *)victimCI + offsetof(clientInfo_t, oldteam)) - 0x14)) {

                            const char *msg = va((const char *)"CGAME_YOUKILLED\x15^1&&2^7 %s\x14%s", targetName, (const char *)"CGAME_TEAMMATE");

                            if (cg->inKillCam == 0) {
                                CG_PriorityCenterPrint(msg, 9.6f, 1);
                            }
                        } else {

                            const char *msg = va((const char *)"CGAME_YOUKILLED\x15%s", targetName);
                            if (cg->inKillCam == 0) {
                                CG_PriorityCenterPrint(msg, 9.6f, 1);
                            }
                        }
                    } else {
                        const char *msg = va((const char *)"CGAME_YOUKILLED\x15%s", targetName);
                        if (cg->inKillCam == 0) {
                            CG_PriorityCenterPrint(msg, 9.6f, 1);
                        }
                    }
                } else if (target == localClientNum) {

                    if (attackerCI != (char *)0) {
                        int atkTeam = *(int *)(((char *)attackerCI + offsetof(clientInfo_t, oldteam)) - 0x14);
                        if (atkTeam != 0 && atkTeam == *(int *)(((char *)victimCI + offsetof(clientInfo_t, oldteam)) - 0x14)) {

                            const char *msg = va((const char *)"CGAME_YOUWEREKILLED\x15^1&&2^7 %s\x14%s", attackerName, (const char *)"CGAME_TEAMMATE");
                            if (cg->inKillCam == 0) {
                                CG_PriorityCenterPrint(msg, 9.6f, 1);
                            }
                        } else {

                            const char *msg = va((const char *)"CGAME_YOUWEREKILLED\x15%s", attackerName);
                            if (cg->inKillCam == 0) {
                                CG_PriorityCenterPrint(msg, 9.6f, 1);
                            }
                        }
                    } else {
                        const char *msg = va((const char *)"CGAME_YOUWEREKILLED\x15%s", attackerName);
                        if (cg->inKillCam == 0) {
                            CG_PriorityCenterPrint(msg, 9.6f, 1);
                        }
                    }
                }
            }

            if (cg->inKillCam != 0)
                return;

            CL_DeathMessagePrint(attackerName, attackerColor, targetName,
                                 victimColor, iconShader, iconWidth, 1.4f, iconColor, iconHorzFlip);
            return;
        }

        case 0xc2:
        {

            float *angles = (float *)es->apos.trBase;

            int fxIndex = cent->nextState.eventParm;
            int fxIdx = fxIndex - 1;
            if ((unsigned int)fxIdx > 62) {

                Com_Printf((const char *)"ERROR: CG_PlayFx called with invalid effect id %i\n", fxIndex);
                return;
            }

            int fxId = *(int *)((char *)cgs + CGS_FX_DEATHFX + fxIdx * 4);

            AngleVectors(angles, forward, (void *)0, up);

            FX_PlayEffect(fxId, position, forward);
            return;
        }

        case 0xc3:
        {
            int csIndex2 = es->eventParm + 0x38e;
            const char *csStr2 = CL_GetConfigString(csIndex2);

            {
                signed char c0 = csStr2[0];
                signed char c1 = csStr2[1];
                int fxIdx2 = c1 + c0 * 10;
                int fxId2 = *(int *)((char *)cgs + CGS_FX_CUSTOM + fxIdx2 * 4);

                int entNum2 = cent->nextState.number;
                int entityInfo = entNum2;

                tagName = SL_GetString(csStr2 + 2, 0);

                boneIndex = FX_GetBoneIndex(entityInfo, (int)tagName);

                Scr_SetString(&tagName, 0);

                if (boneIndex < 0)
                    return;

                FX_PlayEntityEffect(fxId2, position, 0, &entityInfo);
            }
            return;
        }

        default: {
            {
                static unsigned char unhandledSeen[256];
                unsigned int e = (unsigned int)event;
                if (e < 256 && !unhandledSeen[e]) {
                    unhandledSeen[e] = 1;
                    Com_DPrintf("CG_EntityEvent: unhandled event '%s' (%d) -- not yet reconstructed (warned once)\n",
                                CG_EventNames()[event], event);
                }
            }
            return;
        }

        }
    }
}

void CG_CheckEvents(centity_t *cent)
{
    int sequence, count, oldEventParm;

    if (cent->nextState.eType > 10) {
        if (!cent->previousEventSequence) {
            cent->previousEventSequence = 1;
            CG_CalcEntityLerpPositions(cent);
            CG_EntityEvent(cent, cent->nextState.eType - 10);
        }
        return;
    }

    /* Snapshot event sequences are transmitted as eight bits. Zero is also
     * a valid sequence after wrapping, so it must not suppress that event. */
    sequence = cent->nextState.eventSequence & 255;
    count = (sequence - cent->previousEventSequence) & 255;
    if (count > 4)
        count = 4;
    if (!count)
        return;

    CG_CalcEntityLerpPositions(cent);
    oldEventParm = cent->nextState.eventParm;
    for (int i = sequence - count; i < sequence; ++i) {
        cent->nextState.eventParm = cent->nextState.eventParms[i & 3];
        CG_EntityEvent(cent, cent->nextState.events[i & 3]);
    }
    cent->nextState.eventParm = oldEventParm;
    cent->previousEventSequence = sequence;
}
