#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include <stdlib.h>
extern level_locals_t level;

extern qboolean Scr_IsSystemActive(int localClientNum);
extern void Scr_AddEntity(gentity_t *ent);
extern void Scr_Notify(gentity_t *ent, int stringValue, unsigned int paramcount);
extern void G_GetPlayerViewOrigin(gentity_t *ent, vec3_t origin);
extern void G_GetPlayerViewDirection(gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up);
extern int CM_AreaEntities(vec3_t mins, vec3_t maxs, int *entityList, int maxcount, int contentmask);
extern float Vec3Normalize(vec_t *v);
extern qboolean SV_EntityContact(vec3_t mins, vec3_t maxs, gentity_t *ent);
extern qboolean G_IsTurretUsable(gentity_t *turret, gentity_t *player);
extern qboolean BG_CanItemBeGrabbed(const entityState_t *ent, const playerState_t *ps, qboolean bTouched);
extern int G_TraceCapsuleComplete(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passEntityNum, int contentMask);
extern int G_DObjGetWorldTagPos(gentity_t *ent, unsigned int tagName, vec_t *pos);
extern void G_LocationalTrace(trace_t *trace, vec3_t start, vec3_t end, int passEntityNum, int contentMask, unsigned char *priorityMap);
extern float SV_FX_GetVisibility(vec3_t start, vec3_t end);
extern void G_Trigger(gentity_t *self, gentity_t *other);
extern void *BG_GetWeaponDef(int weapon);

extern gentity_t g_entities[];
extern level_locals_t level;
extern scr_const_t scr_const;
extern entityHandler_t entityHandlers[20];
extern byte *vec3_origin_ptr;
extern byte *bg_itemlist_ptr;
extern const dvar_t *g_useholdspawndelay;
extern const dvar_t *g_useholdtime;
extern byte *pPriorityMap;
extern byte *pPriorityMapAlt;
extern const dvar_t *g_friendlyNameDist;
extern const dvar_t *g_friendlyfireDist;

void Player_UpdateActivate(gentity_t *ent);
static int compare_use(const void *pe1, const void *pe2);
static int Player_GetUseList(gentity_t *ent, useList_t *useList) BM_REGPARM(0);
void Player_UpdateCursorHints(gentity_t *ent);
void Player_UpdateLookAtEntity(gentity_t *ent);

#define GENTITY_SIZE sizeof(gentity_s) /* was 0x230 = x86 sizeof(gentity_s); wrong on x64 */

#define SCR_CONST() ((const scr_const_t *)((byte *)&scr_const))

void Player_UpdateActivate(gentity_t *ent)
{
    gclient_t *client;
    gentity_t *useEnt;
    entityHandler_t *handlers;
    fn_use useFn;
    fn_touch touchFn;
    int useEntNum;
    int levelTime;
    int pm_type;
    int eType;
    int activated;

    client = ent->client;

    client->ps.pm_flags &= ~0x8;

    pm_type = client->ps.weaponstate;
    if ((unsigned)(pm_type - 0x11) <= 5)
        return;

    if ((((gclient_t *)(client))->useHoldEntity) != 0x3FF) {

        if (!((((gclient_t *)(client))->oldbuttons) & 0x20))
            goto check_activate;
        if (!((((gclient_t *)(client))->buttons) & 0x20)) {

            client->ps.pm_flags |= 0x8;
            return;
        }
    }

check_activate:

    if ((((gclient_t *)(client))->latched_buttons) & 0x28) {

        if (!Scr_IsSystemActive(1)) {
            client = ent->client;
            activated = 0;
            goto check_use_hold;
        }

        (((gclient_t *)(ent->client))->useHoldEntity) = 0x3FF;

        if ((((gentity_t *)(ent))->active) != 0) {
            client = ent->client;
            if (client->ps.eFlags & 0x300) {

                (((gentity_t *)(ent))->active) = 2;
                activated = 1;
                goto check_use_hold;
            }

            (((gentity_t *)(ent))->active) = 0;
            activated = 1;
            client = ent->client;
            goto check_use_hold;
        }

        client = ent->client;
        if (!(client->ps.pm_flags & 0x4)) {

            if (client->ps.cursorHintEntIndex == 0x3FF) {
                activated = 0;
                goto check_use_hold;
            }

            (((gclient_t *)(client))->useHoldEntity) = client->ps.cursorHintEntIndex;
            client = ent->client;
            levelTime = level.time;
            (((gclient_t *)(client))->useHoldTime) = levelTime;
            activated = 1;
            client = ent->client;
            goto check_use_hold;
        }
        activated = 1;
        goto check_use_hold;
    }

    activated = 0;

check_use_hold:

    if ((((gclient_t *)(client))->useHoldEntity) == 0x3FF) {
        if (!activated) {
            if ((((gclient_t *)(client))->latched_buttons) & 0x20) {

                client->ps.pm_flags |= 0x8;
            }
            return;
        }

        goto check_held_use;
    }

check_held_use:

    if (!((((gclient_t *)(client))->buttons) & 0x28))
        return;

    if (!Scr_IsSystemActive(1))
        return;

    client = ent->client;
    useEntNum = (((gclient_t *)(client))->useHoldEntity);
    if (useEntNum == 0x3FF)
        return;

    levelTime = level.time;
    if (levelTime - (((gclient_t *)(client))->lastSpawnTime) < g_useholdspawndelay->current.integer)
        return;

    if (levelTime - (((gclient_t *)(client))->useHoldTime) < g_useholdtime->current.integer)
        return;

    useEnt = ((gentity_t *)((byte *)level.gentities + (useEntNum) * GENTITY_SIZE));

    handlers = &entityHandlers[(((gentity_t *)(useEnt))->handler)];
    useFn = handlers->use;
    touchFn = handlers->touch;

    eType = (((gentity_t *)(useEnt))->s.eType);
    if (eType == 3) {

        Scr_AddEntity(ent);
        Scr_Notify(useEnt, SCR_CONST()->touch, 1);
        (((gentity_t *)(useEnt))->active) = 1;
        if (touchFn) {
            touchFn(useEnt, ent, 0);
        }
    } else if (eType == 9) {

        if (!G_IsTurretUsable(useEnt, ent))
            goto finish_use;

        goto send_trigger;
    } else {
    send_trigger:

        Scr_AddEntity(ent);
        Scr_Notify(useEnt, SCR_CONST()->trigger, 1);
        if (useFn) {
            useFn(useEnt, ent, ent);
        }
    }

finish_use:

    (((gclient_t *)(ent->client))->useHoldEntity) = 0x3FF;
}

static int compare_use(const void *pe1, const void *pe2)
{
    const useList_t *a = (const useList_t *)pe1;
    const useList_t *b = (const useList_t *)pe2;
    return (int)(a->score - b->score);
}

static int BM_REGPARM(0) Player_GetUseList(gentity_t *ent, useList_t *useList)
{
    playerState_t *ps;
    vec3_t origin;
    vec3_t forward;
    vec3_t playerMin, playerMax;
    vec3_t mins, maxs;
    vec3_t usePos, useDir;
    int touchEnts[1024];
    int num;
    int i;
    int useCount;
    int ignoredFullItems;
    int invalidUseCount;
    float dist;
    float dot;
    gentity_t *hitEnt;
    gentity_t *traceEnt;
    useList_t *curUse;

    ps = &ent->client->ps;

    G_GetPlayerViewOrigin(ent, origin);
    G_GetPlayerViewDirection(ent, forward, NULL, NULL);

    playerMin[0] = ps->origin[0] + ps->mins[0];
    playerMin[1] = ps->origin[1] + ps->mins[1];
    playerMin[2] = ps->origin[2] + ps->mins[2];

    playerMax[0] = ps->origin[0] + ps->maxs[0];
    playerMax[1] = ps->origin[1] + ps->maxs[1];
    playerMax[2] = ps->origin[2] + ps->maxs[2];

    mins[0] = origin[0] - 192.0f;
    mins[1] = origin[1] - 192.0f;
    mins[2] = origin[2] - 96.0f;
    maxs[0] = origin[0] + 192.0f;
    maxs[1] = origin[1] + 192.0f;
    maxs[2] = origin[2] + 96.0f;

    num = CM_AreaEntities(mins, maxs, touchEnts, 0x400, 0x200000);

    useCount = 0;
    ignoredFullItems = 0;

    if (num <= 0)
        goto sort_and_validate;

    curUse = useList;
    for (i = 0; i < num; i++) {
        hitEnt = ((gentity_t *)((byte *)level.gentities + (touchEnts[i]) * GENTITY_SIZE));

        if (hitEnt == ent)
            continue;

        /* Items and mounted weapons are usable entities in their own right;
         * only script triggers need the use-trigger svFlag. Turrets have
         * svFlags == 0, so filtering them here hid both their hint and use. */
        if (hitEnt->s.eType != 3 && hitEnt->s.eType != 9) {
            if (!((((gentity_t *)(hitEnt))->r.svFlags) & 0x20))
                continue;
        }

        if ((((gentity_t *)(hitEnt))->classname) == SCR_CONST()->trigger_use_touch) {

            if ((((gentity_t *)(hitEnt))->r.absmin)[0] > playerMax[0])
                continue;
            if (playerMin[0] > (((gentity_t *)(hitEnt))->r.absmax)[0])
                continue;
            if ((((gentity_t *)(hitEnt))->r.absmin)[1] > playerMax[1])
                continue;
            if (playerMin[1] > (((gentity_t *)(hitEnt))->r.absmax)[1])
                continue;
            if ((((gentity_t *)(hitEnt))->r.absmin)[2] > playerMax[2])
                continue;
            if (playerMin[2] > (((gentity_t *)(hitEnt))->r.absmax)[2])
                continue;

            if (!SV_EntityContact(playerMin, playerMax, hitEnt))
                continue;

            curUse->score = -256.0f;
            curUse->ent = hitEnt;
            useCount++;
            curUse++;
            continue;
        }

        usePos[0] = ((((gentity_t *)(hitEnt))->r.absmin)[0] + (((gentity_t *)(hitEnt))->r.absmax)[0]) * 0.5f;
        usePos[1] = ((((gentity_t *)(hitEnt))->r.absmin)[1] + (((gentity_t *)(hitEnt))->r.absmax)[1]) * 0.5f;
        usePos[2] = ((((gentity_t *)(hitEnt))->r.absmin)[2] + (((gentity_t *)(hitEnt))->r.absmax)[2]) * 0.5f;

        useDir[0] = usePos[0] - origin[0];
        useDir[1] = usePos[1] - origin[1];
        useDir[2] = usePos[2] - origin[2];

        dist = Vec3Normalize(useDir);
        if (dist > 128.0f)
            continue;

        dot = useDir[0] * forward[0] + useDir[1] * forward[1] + useDir[2] * forward[2];

        curUse->score = ((1.0f + dot) * -0.5f + 1.0f) * 256.0f;

        if ((((gentity_t *)(hitEnt))->classname) == SCR_CONST()->trigger_use) {
            curUse->score -= 256.0f;
        }

        if ((((gentity_t *)(hitEnt))->s.eType) == 3) {
            if (!BG_CanItemBeGrabbed(&hitEnt->s, ps, 0)) {
                curUse->score += 10000.0f;
                ignoredFullItems++;
            }
        }

        curUse->ent = hitEnt;
        curUse->score += dist;
        useCount++;
        curUse++;
    }

sort_and_validate:

    qsort(useList, useCount - ignoredFullItems, sizeof(useList_t), compare_use);

    if (useCount - ignoredFullItems > 0) {
        invalidUseCount = 0;
        for (i = 0; i < useCount - ignoredFullItems; i++) {
            traceEnt = useList[i].ent;

            if ((((gentity_t *)(traceEnt))->classname) == SCR_CONST()->trigger_use_touch)
                continue;

            usePos[0] = ((((gentity_t *)(traceEnt))->r.absmin)[0] + (((gentity_t *)(traceEnt))->r.absmax)[0]) * 0.5f;
            usePos[1] = ((((gentity_t *)(traceEnt))->r.absmin)[1] + (((gentity_t *)(traceEnt))->r.absmax)[1]) * 0.5f;
            usePos[2] = ((((gentity_t *)(traceEnt))->r.absmin)[2] + (((gentity_t *)(traceEnt))->r.absmax)[2]) * 0.5f;

            if ((((gentity_t *)(traceEnt))->s.eType) == 9) {
                G_DObjGetWorldTagPos(traceEnt, SCR_CONST()->tag_aim, usePos);
            }

            if (!G_TraceCapsuleComplete(origin, (vec_t *)vec3_origin_ptr, (vec_t *)vec3_origin_ptr, usePos, ps->clientNum, 0x11)) {

                useList[i].score += 10000.0f;
                invalidUseCount++;
            }
        }

        qsort(useList, useCount - ignoredFullItems, sizeof(useList_t), compare_use);
        useCount -= ignoredFullItems;
        useCount -= invalidUseCount;
    } else {
        useCount -= ignoredFullItems;
    }

    return useCount;
}

void Player_UpdateCursorHints(gentity_t *ent)
{
    playerState_t *ps;
    gclient_t *client;
    gentity_t *traceEnt;
    gentity_t *turretEnt;
    gitem_t *itemEntry;
    useList_t useList[1024];
    int numUsable;
    int i;
    int hintType;
    int hintString;
    int weaponIndex;
    int team;
    WeaponDef *weapDef;

    ps = &ent->client->ps;

    ps->cursorHint = 0;
    ps->cursorHintString = -1;
    ps->cursorHintEntIndex = 0x3FF;

    if ((((gentity_t *)(ent))->health) <= 0)
        return;

    client = ent->client;
    if ((unsigned)(client->ps.weaponstate - 0x11) <= 5)
        return;

    if ((((gentity_t *)(ent))->active) != 0) {

        if (!(ps->eFlags & 0x300))
            return;

        turretEnt = ((gentity_t *)((byte *)level.gentities + (ps->viewlocked_entNum) * GENTITY_SIZE));
        weaponIndex = (((gentity_t *)(turretEnt))->s.weapon);
        weapDef = (WeaponDef *)BG_GetWeaponDef(weaponIndex);
        if (*(char *)weapDef->dropHintString == '\0')
            return;

        ps->cursorHintEntIndex = 0x3FF;
        ps->cursorHint = weaponIndex + 4;
        weapDef = (WeaponDef *)BG_GetWeaponDef(weaponIndex);
        ps->cursorHintString = weapDef->dropHintStringIndex;
        return;
    }

    if (client->ps.pm_flags & 0x4)
        return;

    numUsable = Player_GetUseList(ent, useList);
    if (numUsable <= 0)
        return;

    for (i = 0; i < numUsable; i++) {
        traceEnt = useList[i].ent;

        if ((((gentity_t *)(traceEnt))->s.eType) == 3) {

            client = ent->client;

            weaponIndex = (((gentity_t *)(traceEnt))->item.index);

            itemEntry = (gitem_t *)(bg_itemlist_ptr + weaponIndex * 44);

            if (itemEntry->giType != 1)
                continue;

            weapDef = (WeaponDef *)BG_GetWeaponDef(itemEntry->giTag);
            if (weapDef->weapType == 1)
                continue;

            weaponIndex = itemEntry->giTag;
            if ((client->ps.weapons[weaponIndex >> 5] >> (weaponIndex & 0x1F)) & 1)
                continue;

            hintType = weaponIndex + 4;
            if (hintType == 0)
                continue;
            hintString = -1;
        } else if ((((gentity_t *)(traceEnt))->s.eType) == 9) {

            if (!G_IsTurretUsable(traceEnt, ent))
                continue;

            weaponIndex = (((gentity_t *)(traceEnt))->s.weapon);
            hintType = weaponIndex + 4;

            weapDef = (WeaponDef *)BG_GetWeaponDef(weaponIndex);
            if (*(char *)weapDef->szUseHintString != '\0') {
                weapDef = (WeaponDef *)BG_GetWeaponDef((((gentity_t *)(traceEnt))->s.weapon));
                hintString = weapDef->iUseHintStringIndex;
            } else {
                hintString = -1;
            }
        } else if ((((gentity_t *)(traceEnt))->s.eType) == 0) {

            if ((((gentity_t *)(traceEnt))->classname) != SCR_CONST()->trigger_use &&
                (((gentity_t *)(traceEnt))->classname) != SCR_CONST()->trigger_use_touch) {

                hintType = 0;
                hintString = -1;
            } else {

                if ((((gentity_t *)(traceEnt))->team) == 0) {

                    if ((((gentity_t *)(traceEnt))->trigger.singleUserEntIndex) == 0x3FF) {

                    } else {

                        if ((((gentity_t *)(traceEnt))->trigger.singleUserEntIndex) != ent->client->ps.clientNum)
                            continue;
                    }

                    hintType = (((gentity_t *)(traceEnt))->s.animMovetype);
                    if (hintType == 0) {
                        hintString = -1;
                    } else {
                        hintString = (((gentity_t *)(traceEnt))->s.dmgFlags);
                        if (hintString == 0xFF)
                            hintString = -1;
                    }
                } else {

                    team = (((gentity_t *)(traceEnt))->team);
                    if (team != (((gclient_t *)(ent->client))->sess.cs.team)) {

                        if ((((gentity_t *)(traceEnt))->trigger.singleUserEntIndex) != 0x3FF &&
                            (((gentity_t *)(traceEnt))->trigger.singleUserEntIndex) != ent->client->ps.clientNum)
                            continue;

                        hintType = (((gentity_t *)(traceEnt))->s.animMovetype);
                        if (hintType == 0) {
                            hintString = -1;
                        } else {
                            hintString = (((gentity_t *)(traceEnt))->s.dmgFlags);
                            if (hintString == 0xFF)
                                hintString = -1;
                        }
                    } else {
                        continue;
                    }
                }
            }
        } else {
            continue;
        }

        ps->cursorHintEntIndex = (((gentity_t *)(traceEnt))->s.number);
        ps->cursorHint = hintType;
        ps->cursorHintString = hintString;

        if (hintType == 0) {
            ps->cursorHintEntIndex = 0x3FF;
        }
        return;
    }
}

void Player_UpdateLookAtEntity(gentity_t *ent)
{
    playerState_t *ps;
    gentity_t *hitEnt;
    gclient_t *hitClient;
    vec3_t start, end, forward;
    vec3_t contactEnd;
    trace_t trace;
    float vis;
    float scale;
    float distSq;
    float rangeSq;
    float dx, dy, dz;
    WeaponDef *weapDef;
    unsigned char *priorityMap;
    int traceEntNum;

    ps = &ent->client->ps;

    ps->pm_flags &= ~0x300000;

    (((gclient_t *)(ent->client))->pLookatEnt) = NULL;

    G_GetPlayerViewOrigin(ent, start);
    G_GetPlayerViewDirection(ent, forward, NULL, NULL);

    if (ps->eFlags & 0x300) {

        hitEnt = ((gentity_t *)((byte *)level.gentities + (ps->viewlocked_entNum) * GENTITY_SIZE));
        weapDef = (WeaponDef *)BG_GetWeaponDef((((gentity_t *)(hitEnt))->s.weapon));
    } else {
        weapDef = (WeaponDef *)BG_GetWeaponDef(ps->weapon);
    }

    if (ps->weapon == 0 || !weapDef->bRifleBullet) {
        priorityMap = (unsigned char *)pPriorityMap;
    } else {
        priorityMap = (unsigned char *)pPriorityMapAlt;
    }

    end[0] = start[0] + forward[0] * 15000.0f;
    end[1] = start[1] + forward[1] * 15000.0f;
    end[2] = start[2] + forward[2] * 15000.0f;

    G_LocationalTrace(&trace, start, end, ent->s.number, 0x22802801, priorityMap);
    if (trace.entityNum > 0x3FD)
        return;

    scale = trace.fraction * 15000.0f;
    contactEnd[0] = start[0] + forward[0] * scale;
    contactEnd[1] = start[1] + forward[1] * scale;
    contactEnd[2] = start[2] + forward[2] * scale;

    vis = SV_FX_GetVisibility(start, contactEnd);
    if (!(vis >= 0.2f))
        return;

    traceEntNum = trace.entityNum;
    hitEnt = ((gentity_t *)((byte *)level.gentities + (traceEntNum) * GENTITY_SIZE));
    if (hitEnt == NULL)
        return;

    if ((((gentity_t *)(hitEnt))->classname) == SCR_CONST()->trigger_lookat) {

        (((gclient_t *)(ent->client))->pLookatEnt) = hitEnt;

        G_Trigger(hitEnt, ent);

        G_LocationalTrace(&trace, start, end, ent->s.number, 0x2802801, priorityMap);
        if (trace.entityNum > 0x3FD)
            return;

        scale = 15000.0f * trace.fraction;
        contactEnd[0] = start[0] + forward[0] * scale;
        contactEnd[1] = start[1] + forward[1] * scale;
        contactEnd[2] = start[2] + forward[2] * scale;

        vis = SV_FX_GetVisibility(start, contactEnd);
        if (vis < 0.2f)
            return;

        hitEnt = ((gentity_t *)((byte *)level.gentities + (trace.entityNum) * GENTITY_SIZE));
        if (hitEnt == NULL)
            return;
    }

    if ((((gentity_t *)(hitEnt))->s.eType) != 1)
        return;

    if (trace.surfaceFlags & 0x10)
        return;

    dx = (((gentity_t *)(hitEnt))->r.currentOrigin)[0] - start[0];
    dy = (((gentity_t *)(hitEnt))->r.currentOrigin)[1] - start[1];
    dz = (((gentity_t *)(hitEnt))->r.currentOrigin)[2] - start[2];

    hitClient = (((gentity_t *)(hitEnt))->client);
    if ((((gclient_t *)(hitClient))->sess.cs.team) == (((gclient_t *)(ent->client))->sess.cs.team) && (((gclient_t *)(hitClient))->sess.cs.team) != 0) {

        distSq = dx * dx + dy * dy + dz * dz;

        rangeSq = g_friendlyNameDist->current.value;
        rangeSq *= rangeSq;
        if (rangeSq > distSq) {
            if ((((gclient_t *)(ent->client))->pLookatEnt) == NULL) {
                (((gclient_t *)(ent->client))->pLookatEnt) = hitEnt;
            }
        }

        rangeSq = g_friendlyfireDist->current.value;
        rangeSq *= rangeSq;
        if (rangeSq > distSq) {
            ps->pm_flags |= 0x100000;
        }
        return;
    }

    distSq = dx * dx + dy * dy + dz * dz;
    rangeSq = weapDef->enemyCrosshairRange;
    rangeSq *= rangeSq;
    if (rangeSq <= distSq)
        return;

    if ((((gclient_t *)(ent->client))->pLookatEnt) == NULL) {
        (((gclient_t *)(ent->client))->pLookatEnt) = hitEnt;
    }
    ps->pm_flags |= 0x200000;
}
