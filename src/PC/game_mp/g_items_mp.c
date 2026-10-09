#include "common_types.h"
#include "imports.h"
#include <string.h>
extern scr_data_t g_scr_data;
extern level_locals_t level;
extern scr_const_t scr_const;

extern qboolean itemRegistered[256];

#define level (*&level)

#define g_entities ((gentity_t *)imp_g_entities)
extern int BG_GetNumWeapons(void);
extern WeaponDef *BG_GetWeaponDef(int weaponIndex);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern unsigned char G_SetConstString(scr_string_t *to, const char *from);
extern void I_strncat(char *dest, int size, const char *src);
extern void SV_SetConfigstring(int index, const char *val);
extern void *imp_g_scr_data;
extern int G_GetPlayerCorpseIndex(gentity_t *ent);
extern struct DObj_s *Com_GetServerDObj(int entNum);
extern int G_RunThink(gentity_t *ent);

extern char *va(const char *format, ...);
extern void Scr_Error(const char *error);
extern int G_ModelIndex(const char *name);
extern unsigned char G_SetModel(gentity_t *ent, const char *modelName);
extern void G_DObjUpdate(gentity_t *ent);
extern unsigned char G_SetAngle(gentity_t *ent, const vec_t *angle);
extern unsigned char G_SetOrigin(gentity_t *ent, const vec_t *origin);
extern void SV_LinkEntity(gentity_t *gEnt);
extern int G_GetHintStringIndex(int *piIndex, const char *pszString);
extern void Com_Error(int code, const char *fmt, ...);
extern qboolean G_XModelBad(int index);
extern unsigned char G_OverrideModel(int modelIndex, const char *defaultModelName);
extern void BG_UpdatePlayerDObj(struct DObj_s *pDObj, entityState_t *es, clientInfo_t *ci, int attachIgnoreCollision);
extern void BG_PlayerAnimation(const struct DObj_s *pDObj, entityState_t *es, clientInfo_t *ci);
extern int BG_AmmoForWeapon(int weapon);
extern int BG_ClipForWeapon(int weapon);
extern qboolean BG_WeaponIsClipOnly(int weapon);
extern int BG_GetAmmoTypeMax(int iAmmoIndex);
extern int BG_GetAmmoClipSize(int iClipIndex);
extern int BG_GetMaxPickupableAmmo(playerState_t *ps, int weapon);
extern qboolean G_GivePlayerWeapon(playerState_t *pPS, int iWeaponIndex);
extern qboolean BG_TakePlayerWeapon(playerState_t *pPS, int iWeaponIndex);
extern void G_TraceCapsule(trace_t *results, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentmask);
extern void BG_EvaluateTrajectory(trajectory_t *tr, int atTime, vec_t *result);
extern float Vec3DistanceSq(const vec_t *a, const vec_t *b);
extern int SV_PointContents(vec_t *point, int passEntityNum, int contentMask);
extern char *vtos(const vec_t *v);
extern const char *SL_ConvertToString(unsigned int stringValue);
extern unsigned char G_FreeEntity(gentity_t *ed);
extern gentity_t *G_Spawn(void);
extern void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);
extern void Vec3Cross(const vec_t *v0, const vec_t *v1, vec_t *cross);
extern void AxisToAngles(vec3_t *axis, vec_t *angles);
extern void XAnimCalcDelta(XAnimTree *tree, unsigned int animIndex, vec_t *rot, vec_t *trans, int bUseGoalWeight);
extern const vec_t Vec3Normalize(vec_t *v);
extern qboolean G_TraceCapsuleComplete(const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentmask);

#define g_maxDroppedWeapons (*(const dvar_t **)imp_g_maxDroppedWeapons)
#define g_dropForwardSpeed (*(const dvar_t **)imp_g_dropForwardSpeed)
#define g_dropUpSpeedBase (*(const dvar_t **)imp_g_dropUpSpeedBase)
#define g_dropUpSpeedRand (*(const dvar_t **)imp_g_dropUpSpeedRand)
extern float crandom(void);
extern float randomf(void);
extern float floorf(float);
extern int rand(void);
extern qboolean G_DObjGetWorldTagMatrix(gentity_t *ent, unsigned int tagName, vec3_t *tagMat);
extern qboolean BG_CanItemBeGrabbed(const entityState_t *ent, const playerState_t *ps, qboolean bTouched);
extern void SV_GameSendServerCommand(int clientNum, int type, const char *text);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern char *I_CleanStr(char *str);
extern int SV_GetGuid(int clientNum);
extern void G_LogPrintf(const char *fmt, ...);
extern Bool BG_DoesWeaponNeedSlot(int weapIndex);
extern int BG_IsPlayerWeaponInSlot(const playerState_t *pPS, int iWeaponIndex, qboolean bAnyMode);
extern int BG_GetEmptySlotForWeapon(const playerState_t *pPS, int iWeaponIndex);
extern int BG_GetStackSlotForWeapon(const playerState_t *pPS, int iWeaponIndex, weapSlot_t preferredSlot);
extern void G_SelectWeaponIndex(int clientNum, int iWeaponIndex);
extern void Scr_AddEntity(gentity_t *ent);
extern void Scr_AddUndefined(void);
extern void Scr_Notify(gentity_t *ent, int stringValue, unsigned int paramcount);
extern void G_AddPredictableEvent(gentity_t *ent, int event, int eventParm);
extern unsigned char G_AddEvent(gentity_t *ent, int event, int eventParm);
extern void Com_Printf(const char *fmt, ...);

#define _ENT(e) ((gentity_t *)(e))

enum {
    GITEMS_ENTITYNUM_WORLD = 0x3fe,
    GITEMS_CS_WEAPONS = 7,
    GITEMS_CS_ITEMS = 8
};

static int G_ItemIndexFromPointer(const gitem_t *item)
{
    return (int)(item - (const gitem_t *)imp_bg_itemlist);
}

static inline __attribute__((always_inline)) void G_SetVec3(vec3_t v, float x, float y, float z)
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
}

static void G_ItemGroundAngles(const vec_t *currentAngles, const vec_t *normal,
                              qboolean weapon, vec_t *angles)
{
    vec3_t axis[3];
    vec3_t heading = { 0.0f, currentAngles[1], 0.0f };

    /* AxisToAngles needs a complete orthonormal basis, including the up row.
     * Preserve yaw while laying the item along the collision plane. */
    G_SetVec3(axis[2], normal[0], normal[1], normal[2]);
    Vec3Normalize(axis[2]);
    AngleVectors(heading, axis[0], NULL, NULL);
    Vec3Cross(axis[2], axis[0], axis[1]);
    if (Vec3Normalize(axis[1]) < 0.001f) {
        heading[1] += 90.0f;
        AngleVectors(heading, axis[0], NULL, NULL);
        Vec3Cross(axis[2], axis[0], axis[1]);
        Vec3Normalize(axis[1]);
    }
    Vec3Cross(axis[1], axis[2], axis[0]);
    AxisToAngles(axis, angles);
    if (weapon)
        angles[2] += 90.0f;
}

void DroppedItemClearOwner(gentity_t *pSelf);
void G_GetItemClassname(const gitem_t *item, scr_string_t *out);
void ClearRegisteredItems(void);
void SaveRegisteredWeapons(void);
void SaveRegisteredItems(void);
void RegisterItem(int iItemIndex, qboolean bUpdateCS);
void G_RegisterWeapon(int weapIndex);
qboolean IsItemRegistered(int iItemIndex);
void FinishSpawningItem(gentity_t *ent);
void G_RunCorpseMove(gentity_t *ent);
void G_RunCorpse(gentity_t *ent);
void G_RunItem(gentity_t *ent);
int Add_Ammo(gentity_t *ent, int weapon, int count, qboolean fillClip);
gentity_t *Drop_Item(gentity_t *ent, const gitem_t *item, float angle, qboolean novelocity);
gentity_t *Drop_Weapon(gentity_t *pEnt, int iWeaponIndex, unsigned int tag);
void Touch_Item(gentity_t *ent, gentity_t *other, qboolean bTouched);
void Touch_Item_Auto(gentity_t *ent, gentity_t *other, qboolean bTouched);
void G_SpawnItem(gentity_t *ent, const gitem_t *item);

void DroppedItemClearOwner(gentity_t *pSelf)
{
    (_ENT(pSelf)->s.clientNum) = GITEMS_ENTITYNUM_WORLD;
}

void G_GetItemClassname(const gitem_t *item, scr_string_t *out)
{
    char classname[256];
    int index;
    WeaponDef *weapDef;

    index = item - (gitem_t *)imp_bg_itemlist;

    if (index <= BG_GetNumWeapons()) {
        weapDef = BG_GetWeaponDef(index);
        Com_sprintf(classname, sizeof(classname), "weapon_%s", weapDef->szInternalName);
        G_SetConstString(out, classname);
        return;
    }

    G_SetConstString(out, item->classname);
}

void ClearRegisteredItems(void)
{
    memset(itemRegistered, 0, sizeof(itemRegistered));
    itemRegistered[0] = 1;
}

void SaveRegisteredWeapons(void)
{
    char string[8192];
    WeaponDef *weapDef;
    int weapIndex;

    level.registerWeapons = 0;
    string[0] = '\0';
    weapDef = NULL;

    for (weapIndex = 1; weapIndex <= BG_GetNumWeapons(); ++weapIndex) {
        if (weapDef) {
            I_strncat(string, sizeof(string), " ");
        }

        weapDef = BG_GetWeaponDef(weapIndex);
        I_strncat(string, sizeof(string), weapDef->szInternalName);
    }

    SV_SetConfigstring(GITEMS_CS_WEAPONS, string);
}

void SaveRegisteredItems(void)
{
    int bits;
    int digit;
    int itemIdx;
    int lastNonZeroChar;
    int n;
    int numItems;
    char string[256];

    level.bRegisterItems = 0;
    bits = 0;
    digit = 0;
    n = 0;
    lastNonZeroChar = 0;

    numItems = *(int *)imp_bg_numItems;
    for (itemIdx = 0; itemIdx < numItems; ++itemIdx) {
        if (itemRegistered[itemIdx]) {
            digit += 1 << bits;
        }

        ++bits;

        if (bits == 4) {
            string[n] = digit + (digit < 10 ? '0' : 'W');
            ++n;
            lastNonZeroChar = n;
            digit = 0;
            bits = 0;
        }
    }

    if (bits) {
        string[n] = digit + (digit < 10 ? '0' : 'W');
        ++n;
        lastNonZeroChar = n;
    }

    string[lastNonZeroChar] = '\0';
    SV_SetConfigstring(GITEMS_CS_ITEMS, string);
}

void RegisterItem(int iItemIndex, qboolean bUpdateCS)
{
    gitem_t *item;
    const char *name;

    if (itemRegistered[iItemIndex])
        return;

    if (!level.initializing) {
        item = (gitem_t *)imp_bg_itemlist + iItemIndex;
        name = item->pickup_name;
        if (!name || *name == '\0') {
            name = "<<unknown>>";
        }
        Scr_Error(va("game tried to register the item '%s' after initialization finished\n", name));
    }

    itemRegistered[iItemIndex] = 1;

    item = (gitem_t *)imp_bg_itemlist + iItemIndex;

    if (item->world_model[0]) {
        G_ModelIndex(item->world_model[0]);
    }

    if (item->world_model[1]) {
        G_ModelIndex(item->world_model[1]);
    }

    if (bUpdateCS) {
        level.bRegisterItems = 1;
    }
}

void G_RegisterWeapon(int weapIndex)
{
    WeaponDef *weapDef;
    int modelIndex;
    level_locals_t *l;

    itemRegistered[weapIndex] = 1;

    l = &level;
    l->bRegisterItems = 1;
    l->registerWeapons = 1;

    weapDef = BG_GetWeaponDef(weapIndex);

    if (weapDef->szUseHintString[0] &&
        !G_GetHintStringIndex(&weapDef->iUseHintStringIndex, weapDef->szUseHintString)) {
        Com_Error(1, "Too many different hintstring values on weapons. Max allowed is %i different strings", 32);
    }

    if (weapDef->dropHintString[0] &&
        !G_GetHintStringIndex(&weapDef->dropHintStringIndex, weapDef->dropHintString)) {
        Com_Error(1, "Too many different hintstring values on weapons. Max allowed is %i different strings", 32);
    }

    modelIndex = G_ModelIndex(weapDef->szWorldModel);
    if (modelIndex && G_XModelBad(modelIndex))
        G_OverrideModel(modelIndex, "xmodel/defaultweapon");

    G_ModelIndex(weapDef->szProjectileModel);
}

qboolean IsItemRegistered(int iItemIndex)
{
    return itemRegistered[iItemIndex];
}

void FinishSpawningItem(gentity_t *ent)
{
    trace_t tr;
    vec3_t mins;
    vec3_t maxs;
    vec3_t start;
    vec3_t dest;
    vec3_t endpos;
    vec3_t angles;
    int clipMask;
    const gitem_t *item;

    ent->handler = 0x11;

    if (ent->spawnflags & 1) {
        G_SetOrigin(ent, ent->r.currentOrigin);
        SV_LinkEntity(ent);
        return;
    }

    item = (const gitem_t *)imp_bg_itemlist + ent->item.index;
    if (item->giType == 1) {
        mins[0] = -1.0f;
        mins[1] = -1.0f;
        mins[2] = -1.0f;
        maxs[0] = 1.0f;
        maxs[1] = 1.0f;
        maxs[2] = 1.0f;
    } else {
        mins[0] = -1.0f;
        mins[1] = -1.0f;
        mins[2] = 0.0f;
        maxs[0] = 1.0f;
        maxs[1] = 1.0f;
        maxs[2] = 2.0f;
    }

    clipMask = ent->clipmask ? ent->clipmask : 0x491;

    start[0] = ent->r.currentOrigin[0];
    start[1] = ent->r.currentOrigin[1];
    start[2] = ent->r.currentOrigin[2];
    dest[0] = ent->r.currentOrigin[0];
    dest[1] = ent->r.currentOrigin[1];
    dest[2] = ent->r.currentOrigin[2] - 4096.0f;

    G_TraceCapsule(&tr, start, mins, maxs, dest, ent->s.number, clipMask);

    if (tr.startsolid) {
        start[0] = ent->r.currentOrigin[0];
        start[1] = ent->r.currentOrigin[1];
        start[2] = ent->r.currentOrigin[2] - 15.0f;
        dest[0] = ent->r.currentOrigin[0];
        dest[1] = ent->r.currentOrigin[1];
        dest[2] = ent->r.currentOrigin[2] - 4096.0f;

        G_TraceCapsule(&tr, start, mins, maxs, dest, ent->s.number, clipMask);

        if (tr.startsolid) {
            Com_Printf("FinishSpawningItem: %s startsolid at %s\n",
                       SL_ConvertToString(ent->classname), vtos(ent->r.currentOrigin));
            G_FreeEntity(ent);
            return;
        }
    }

    ent->s.groundEntityNum = tr.entityNum;
    endpos[0] = start[0] + (dest[0] - start[0]) * tr.fraction;
    endpos[1] = start[1] + (dest[1] - start[1]) * tr.fraction;
    endpos[2] = start[2] + (dest[2] - start[2]) * tr.fraction;
    G_SetOrigin(ent, endpos);

    if (tr.fraction < 1.0f) {
        G_ItemGroundAngles(ent->r.currentAngles, tr.normal, item->giType == IT_WEAPON, angles);
        G_SetAngle(ent, angles);
    }

    SV_LinkEntity(ent);
}

void G_RunCorpseMove(gentity_t *ent)
{
    corpseInfo_t *corpseInfo;
    trace_t tr;
    vec_t rot[2];
    vec3_t deltaChange;
    vec3_t origin;
    vec3_t endpos;
    vec3_t forward;
    vec3_t right;
    vec3_t left;
    vec3_t start;
    vec3_t normal;
    vec3_t axis[3];
    int mask;
    qboolean applyDelta;

    corpseInfo = &g_scr_data.playerCorpseInfo[G_GetPlayerCorpseIndex(ent)];

    XAnimCalcDelta(corpseInfo->tree, 0, rot, deltaChange, 1);

    if (corpseInfo->falling) {
        applyDelta = 0;
    } else {
        float deltaLenSq;

        deltaLenSq = deltaChange[0] * deltaChange[0] + deltaChange[1] * deltaChange[1] + deltaChange[2] * deltaChange[2];
        if (deltaLenSq <= 1.0f) {
            return;
        }
        applyDelta = 1;
    }

    BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);

    if (applyDelta) {
        AngleVectors(ent->r.currentAngles, forward, right, NULL);
        left[0] = -right[0];
        left[1] = -right[1];
        left[2] = -right[2];
        Vec3Normalize(forward);
        Vec3Normalize(left);

        origin[0] += deltaChange[0] * forward[0];
        origin[1] += deltaChange[0] * forward[1];
        origin[2] += deltaChange[0] * forward[2];

        origin[0] += deltaChange[1] * left[0];
        origin[1] += deltaChange[1] * left[1];
        origin[2] += deltaChange[1] * left[2];
    }

    mask = ent->clipmask;
    G_TraceCapsule(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin,
                   ent->r.ownerNum, mask);

    endpos[0] = ent->r.currentOrigin[0] + (origin[0] - ent->r.currentOrigin[0]) * tr.fraction;
    endpos[1] = ent->r.currentOrigin[1] + (origin[1] - ent->r.currentOrigin[1]) * tr.fraction;
    endpos[2] = ent->r.currentOrigin[2] + (origin[2] - ent->r.currentOrigin[2]) * tr.fraction;

    ent->r.currentOrigin[0] = endpos[0];
    ent->r.currentOrigin[1] = endpos[1];
    ent->r.currentOrigin[2] = endpos[2];

    if (tr.startsolid) {
        tr.fraction = 0.0f;
    }

    SV_LinkEntity(ent);
    G_RunThink(ent);

    if (!ent->active) {
        return;
    }

    if (tr.fraction == 1.0f) {
        if (corpseInfo->falling) {
            return;
        }

        if (!applyDelta) {
            return;
        }

        ent->s.pos.trType = 1;
        ent->s.pos.trBase[0] = endpos[0];
        ent->s.pos.trBase[1] = endpos[1];
        ent->s.pos.trBase[2] = endpos[2];
        ent->s.pos.trTime = 0;
        ent->s.pos.trDuration = 0;
        ent->s.pos.trDelta[0] = 0.0f;
        ent->s.pos.trDelta[1] = 0.0f;
        ent->s.pos.trDelta[2] = 0.0f;

        origin[2] -= 1.0f;
        if (!G_TraceCapsuleComplete(ent->r.currentOrigin, ent->r.mins, ent->r.maxs,
                                    origin, ent->r.ownerNum, mask)) {
            return;
        }

        corpseInfo->falling = 1;
        ent->s.pos.trType = 5;
        ent->s.pos.trBase[0] = endpos[0];
        ent->s.pos.trBase[1] = endpos[1];
        ent->s.pos.trBase[2] = endpos[2];
        ent->s.pos.trDelta[0] = 0.0f;
        ent->s.pos.trDelta[1] = 0.0f;
        ent->s.pos.trDelta[2] = 0.0f;
        ent->s.pos.trDelta[0] += deltaChange[0] * forward[0];
        ent->s.pos.trDelta[1] += deltaChange[0] * forward[1];
        ent->s.pos.trDelta[2] += deltaChange[0] * forward[2];
        ent->s.pos.trDelta[0] += deltaChange[1] * left[0];
        ent->s.pos.trDelta[1] += deltaChange[1] * left[1];
        ent->s.pos.trDelta[2] += deltaChange[1] * left[2];
        ent->s.pos.trTime = level.time;
        ent->s.pos.trDuration = 0;
        return;
    }

    if (SV_PointContents(ent->r.currentOrigin, -1, 0x80000000)) {
        G_FreeEntity(ent);
        return;
    }

    if (!corpseInfo->falling) {
        return;
    }

    if (tr.allsolid) {
        start[0] = ent->r.currentOrigin[0];
        start[1] = ent->r.currentOrigin[1];
        start[2] = ent->r.currentOrigin[2] + 64.0f;

        G_TraceCapsule(&tr, start, ent->r.mins, ent->r.maxs, ent->r.currentOrigin,
                       ent->r.ownerNum, mask & ~0x10000);

        if (!tr.allsolid) {
            endpos[0] = start[0] + (ent->r.currentOrigin[0] - start[0]) * tr.fraction;
            endpos[1] = start[1] + (ent->r.currentOrigin[1] - start[1]) * tr.fraction;
            endpos[2] = start[2] + (ent->r.currentOrigin[2] - start[2]) * tr.fraction;

            ent->r.currentOrigin[0] = endpos[0];
            ent->r.currentOrigin[1] = endpos[1];
            ent->r.currentOrigin[2] = endpos[2];
        }
    }

    ent->s.pos.trDelta[0] = 0.0f;
    ent->s.pos.trDelta[1] = 0.0f;
    ent->s.pos.trDelta[2] = 0.0f;

    if (!tr.allsolid && tr.normal[2] <= 0.0f) {
        ent->r.currentOrigin[0] += tr.normal[0];
        ent->r.currentOrigin[1] += tr.normal[1];
        ent->r.currentOrigin[2] += tr.normal[2];
        ent->s.pos.trBase[0] = ent->r.currentOrigin[0];
        ent->s.pos.trBase[1] = ent->r.currentOrigin[1];
        ent->s.pos.trBase[2] = ent->r.currentOrigin[2];
        ent->s.pos.trTime = level.time;
        return;
    }

    corpseInfo->falling = 0;
    ent->s.pos.trType = 1;
    ent->s.pos.trBase[0] = endpos[0];
    ent->s.pos.trBase[1] = endpos[1];
    ent->s.pos.trBase[2] = endpos[2];
    ent->s.pos.trTime = 0;
    ent->s.pos.trDuration = 0;
    ent->s.pos.trDelta[0] = 0.0f;
    ent->s.pos.trDelta[1] = 0.0f;
    ent->s.pos.trDelta[2] = 0.0f;
    ent->s.groundEntityNum = tr.entityNum;

    if (tr.allsolid) {
        G_SetAngle(ent, ent->r.currentAngles);
    } else {
        normal[0] = tr.normal[0];
        normal[1] = tr.normal[1];
        normal[2] = tr.normal[2];

        AngleVectors(ent->r.currentAngles, axis[0], NULL, NULL);
        Vec3Cross(normal, axis[0], axis[1]);
        Vec3Cross(axis[1], normal, axis[0]);
        AxisToAngles((const vec_t *)axis, rot);
        G_SetAngle(ent, rot);
    }

    SV_LinkEntity(ent);
}

void G_RunCorpse(gentity_t *ent)
{
    corpseInfo_t *corpseInfo;
    struct DObj_s *obj;

    G_RunCorpseMove(ent);

    corpseInfo = &g_scr_data.playerCorpseInfo[G_GetPlayerCorpseIndex(ent)];
    obj = Com_GetServerDObj(ent->s.number);
    BG_UpdatePlayerDObj(obj, &ent->s, &corpseInfo->ci, 0);

    obj = Com_GetServerDObj(ent->s.number);
    if (obj)
        BG_PlayerAnimation(obj, &ent->s, &corpseInfo->ci);

    G_RunThink(ent);
}

void G_RunItem(gentity_t *ent)
{
    trace_t tr;
    vec3_t origin;
    vec3_t endpos;
    vec3_t angles;
    int mask;

    if (ent->s.groundEntityNum == 0x3ff || level.gentities[ent->s.groundEntityNum].s.pos.trType != 0) {
        if (ent->s.pos.trType != 5 && !(ent->spawnflags & 1)) {
            ent->s.pos.trType = 5;
            ent->s.pos.trTime = level.time;
            ent->s.pos.trBase[0] = ent->r.currentOrigin[0];
            ent->s.pos.trBase[1] = ent->r.currentOrigin[1];
            ent->s.pos.trBase[2] = ent->r.currentOrigin[2];
            ent->s.pos.trDelta[0] = 0.0f;
            ent->s.pos.trDelta[1] = 0.0f;
            ent->s.pos.trDelta[2] = 0.0f;
        }
    }

    if (ent->s.pos.trType != 0 && ent->s.pos.trType != 6 && ent->tagInfo == 0) {
        BG_EvaluateTrajectory(&ent->s.apos, level.time + 50, ent->r.currentAngles);
        BG_EvaluateTrajectory(&ent->s.pos, level.time + 50, origin);

        mask = ent->clipmask;
        if (!mask)
            mask = 0x491;

        if (Vec3DistanceSq(ent->r.currentOrigin, origin) < 0.1f)
            origin[2] -= 1.0f;

        G_TraceCapsule(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs,
                       origin, ent->r.ownerNum, mask);

        if (tr.fraction < 1.0f) {
            endpos[0] = ent->r.currentOrigin[0] + (origin[0] - ent->r.currentOrigin[0]) * tr.fraction;
            endpos[1] = ent->r.currentOrigin[1] + (origin[1] - ent->r.currentOrigin[1]) * tr.fraction;
            endpos[2] = ent->r.currentOrigin[2] + (origin[2] - ent->r.currentOrigin[2]) * tr.fraction;

            if (!tr.startsolid && tr.fraction < 0.01f && tr.normal[2] < 0.5f) {
                float dot = (origin[0] - ent->r.currentOrigin[0]) * tr.normal[0] + (origin[1] - ent->r.currentOrigin[1]) * tr.normal[1] + (origin[2] - ent->r.currentOrigin[2]) * tr.normal[2];
                float push = 1.0f - dot;

                origin[0] += tr.normal[0] * push;
                origin[1] += tr.normal[1] * push;
                origin[2] += tr.normal[2] * push;

                G_TraceCapsule(&tr, endpos, ent->r.mins, ent->r.maxs,
                               origin, ent->r.ownerNum, mask);

                endpos[0] += (origin[0] - endpos[0]) * tr.fraction;
                endpos[1] += (origin[1] - endpos[1]) * tr.fraction;
                endpos[2] += (origin[2] - endpos[2]) * tr.fraction;
            }

            ent->s.pos.trType = 3;
            ent->s.pos.trTime = level.time;
            ent->s.pos.trDuration = 50;
            ent->s.pos.trBase[0] = ent->r.currentOrigin[0];
            ent->s.pos.trBase[1] = ent->r.currentOrigin[1];
            ent->s.pos.trBase[2] = ent->r.currentOrigin[2];
            ent->s.pos.trDelta[0] = (endpos[0] - ent->r.currentOrigin[0]) * 20.0f;
            ent->s.pos.trDelta[1] = (endpos[1] - ent->r.currentOrigin[1]) * 20.0f;
            ent->s.pos.trDelta[2] = (endpos[2] - ent->r.currentOrigin[2]) * 20.0f;
            ent->r.currentOrigin[0] = endpos[0];
            ent->r.currentOrigin[1] = endpos[1];
            ent->r.currentOrigin[2] = endpos[2];
        } else {
            ent->r.currentOrigin[0] = origin[0];
            ent->r.currentOrigin[1] = origin[1];
            ent->r.currentOrigin[2] = origin[2];
        }

        SV_LinkEntity(ent);
        G_RunThink(ent);

        /* active gates pickup/use; it does not indicate a live entity. */
        if (ent->r.inuse && tr.fraction < 0.01f) {
            if (tr.normal[2] <= 0.0f || SV_PointContents(ent->r.currentOrigin, -1, 0x80000000)) {
                G_FreeEntity(ent);
                return;
            }

            G_ItemGroundAngles(ent->r.currentAngles, tr.normal,
                               ((const gitem_t *)imp_bg_itemlist + ent->s.index.item)->giType == IT_WEAPON,
                               angles);

            G_SetAngle(ent, angles);
            G_SetOrigin(ent, endpos);
            ent->s.groundEntityNum = tr.entityNum;
            SV_LinkEntity(ent);
        }
        return;
    }

    G_RunThink(ent);
}

gentity_t *Drop_Item(gentity_t *ent, const gitem_t *item, float angle, qboolean novelocity)
{
    vec3_t angles;
    vec3_t velocity;
    vec3_t vPos;
    gentity_t *dropped;
    int itemIndex;
    int slot;
    int maxDroppedWeapons;
    int ownerNum;

    ownerNum = ent->s.number;

    angles[0] = 0.0f;
    angles[1] = ent->r.currentAngles[1] + angle;
    angles[2] = 0.0f;

    if (novelocity) {
        G_SetVec3(velocity, 0.0f, 0.0f, 0.0f);
    } else {
        AngleVectors(angles, velocity, NULL, NULL);
        velocity[0] *= g_dropForwardSpeed->current.value;
        velocity[1] *= g_dropForwardSpeed->current.value;
        velocity[2] *= g_dropForwardSpeed->current.value;
        velocity[2] += g_dropUpSpeedBase->current.value + crandom() * g_dropUpSpeedRand->current.value;
    }

    vPos[0] = ent->r.currentOrigin[0];
    vPos[1] = ent->r.currentOrigin[1];
    vPos[2] = ent->r.currentOrigin[2] + (ent->r.maxs[2] - ent->r.mins[2]) * 0.5f;

    itemIndex = G_ItemIndexFromPointer(item);
    RegisterItem(itemIndex, 1);

    dropped = G_Spawn();

    maxDroppedWeapons = g_maxDroppedWeapons->current.integer;
    if (maxDroppedWeapons > 0) {
        slot = 0;
        if (level.droppedWeaponCue[0]) {
            float bestDistSq;
            int bestSlot;
            int i;

            bestDistSq = -1.0f;
            bestSlot = 0;
            for (i = 0; i < maxDroppedWeapons; ++i) {
                gentity_t *queued = level.droppedWeaponCue[i];
                float nearestClientDistSq;
                int clientNum;

                if (!queued) {
                    slot = i;
                    goto found_drop_slot;
                }

                nearestClientDistSq = 999998029824.0f;
                for (clientNum = 0; clientNum < level.maxclients; ++clientNum) {
                    gclient_t *client = &level.clients[clientNum];

                    if (client->sess.connected == CON_CONNECTED && client->sess.sessionState == SESS_STATE_PLAYING) {
                        float distSq = Vec3DistanceSq(g_entities[clientNum].r.currentOrigin, queued->r.currentOrigin);
                        if (distSq < nearestClientDistSq) {
                            nearestClientDistSq = distSq;
                        }
                    }
                }

                if (nearestClientDistSq > bestDistSq) {
                    bestSlot = i;
                    bestDistSq = nearestClientDistSq;
                }
            }

            slot = bestSlot;
            G_FreeEntity(level.droppedWeaponCue[slot]);
            level.droppedWeaponCue[slot] = NULL;
        }
    } else {
        slot = 0;
    }

found_drop_slot:
    level.droppedWeaponCue[slot] = dropped;

    dropped->s.eType = 3;
    dropped->s.index.item = itemIndex;
    G_GetItemClassname(item, &dropped->classname);
    dropped->item.index = (unsigned short)itemIndex;

    if (item->giType == IT_WEAPON) {
        G_SetVec3(dropped->r.mins, -1.0f, -1.0f, -1.0f);
        G_SetVec3(dropped->r.maxs, 1.0f, 1.0f, 1.0f);
    } else {
        G_SetVec3(dropped->r.mins, -1.0f, -1.0f, 0.0f);
        G_SetVec3(dropped->r.maxs, 1.0f, 1.0f, 2.0f);
    }

    dropped->r.contents = (item->giType == IT_AMMO) ? 0x405c0108 : 0x407c0108;
    dropped->s.clientNum = ownerNum;

    G_SetModel(dropped, item->world_model[0]);
    G_DObjUpdate(dropped);

    dropped->handler = 0x0f;
    G_SetOrigin(dropped, vPos);
    G_SetAngle(dropped, angles);
    dropped->s.groundEntityNum = 0x3ff;

    dropped->s.pos.trType = TR_GRAVITY;
    dropped->s.pos.trTime = level.time;
    dropped->s.pos.trDelta[0] = velocity[0];
    dropped->s.pos.trDelta[1] = velocity[1];
    dropped->s.pos.trDelta[2] = velocity[2];

    dropped->nextthink = level.time + 1000;
    dropped->flags = 0x10;

    SV_LinkEntity(dropped);
    return dropped;
}

gentity_t *Drop_Weapon(gentity_t *pEnt, int iWeaponIndex, unsigned int tag)
{
    const gitem_t *pWeapItem;
    WeaponDef *weapDef;
    playerState_t *ps;
    gentity_t *pDrop;
    int iAmmoIndex;
    int iClipIndex;
    int iMin;
    int iMax;

    pWeapItem = (const gitem_t *)imp_bg_itemlist + iWeaponIndex;
    ps = pEnt->client ? &pEnt->client->ps : NULL;

    if (ps && !(ps->weapons[iWeaponIndex >> 5] & (1 << (iWeaponIndex & 31)))) {
        BG_TakePlayerWeapon(ps, iWeaponIndex);
        return NULL;
    }

    iAmmoIndex = BG_AmmoForWeapon(iWeaponIndex);
    iClipIndex = BG_ClipForWeapon(iWeaponIndex);
    weapDef = BG_GetWeaponDef(iWeaponIndex);

    if (weapDef->bClipOnly && ps && !ps->ammoclip[iClipIndex]) {
        BG_TakePlayerWeapon(ps, iWeaponIndex);
        return NULL;
    }

    pDrop = Drop_Item(pEnt, pWeapItem, 0.0f, 0);

    if (ps) {
        iMin = ps->ammo[iAmmoIndex];
        ps->ammo[iAmmoIndex] = 0;

        iMax = ps->ammoclip[iClipIndex];
        ps->ammoclip[iClipIndex] = 0;

        BG_TakePlayerWeapon(ps, iWeaponIndex);
    } else {
        iMin = weapDef->iDropAmmoMin;
        iMax = weapDef->iDropAmmoMax;

        if (iMax < iMin) {
            int tmp = iMax;
            iMax = iMin;
            iMin = tmp;
        }

        if (iMax == 0 && iMin == 0) {
            int clipSize = BG_GetAmmoClipSize(iClipIndex);
            int total;

            total = (int)floorf(((randomf() + 1.0f) * 0.5f) * (float)(clipSize - 1) + 0.5f) + 1;
            iMax = (int)floorf((randomf() * 0.5f + 0.25f) * (float)total + 0.5f);
            iMin = total - iMax;
        } else if (iMax < 0) {
            iMin = 0;
            iMax = 0;
        } else {
            if (iMax != iMin) {
                iMin += rand() % (iMax - iMin);
            }

            if (iMin > 0) {
                int clipSize = BG_GetAmmoClipSize(iClipIndex);

                if (clipSize) {
                    iMax = rand() % clipSize;
                    if (iMin > iMax) {
                        iMin -= iMax;
                    } else {
                        iMax = iMin;
                        iMin = 0;
                    }
                } else {
                    iMax = 0;
                }
            } else {
                iMin = 0;
                iMax = 0;
            }
        }
    }

    pDrop->count = iMin ? iMin : -1;
    pDrop->item.count2 = iMax ? iMax : -1;

    if (!iMin && !iMax) {
        pDrop->r.contents &= ~0x200000;
    }

    if (tag) {
        vec3_t tagMat[4];
        vec3_t vCenter;
        vec3_t vAngles;
        trace_t trace;
        vec3_t endpos;
        int clipMask;

        if (G_DObjGetWorldTagMatrix(pEnt, tag, tagMat)) {
            vCenter[0] = pEnt->r.currentOrigin[0] + (pEnt->r.mins[0] + pEnt->r.maxs[0]) * 0.5f;
            vCenter[1] = pEnt->r.currentOrigin[1] + (pEnt->r.mins[1] + pEnt->r.maxs[1]) * 0.5f;
            vCenter[2] = pEnt->r.currentOrigin[2] + (pEnt->r.mins[2] + pEnt->r.maxs[2]) * 0.5f;

            clipMask = pDrop->clipmask ? pDrop->clipmask : 0x491;
            G_TraceCapsule(&trace, vCenter, pDrop->r.mins, pDrop->r.maxs, tagMat[3], pEnt->s.number, clipMask);

            endpos[0] = vCenter[0] + (tagMat[3][0] - vCenter[0]) * trace.fraction;
            endpos[1] = vCenter[1] + (tagMat[3][1] - vCenter[1]) * trace.fraction;
            endpos[2] = vCenter[2] + (tagMat[3][2] - vCenter[2]) * trace.fraction;

            pDrop->s.pos.trBase[0] = endpos[0];
            pDrop->s.pos.trBase[1] = endpos[1];
            pDrop->s.pos.trBase[2] = endpos[2];
            pDrop->r.currentOrigin[0] = endpos[0];
            pDrop->r.currentOrigin[1] = endpos[1];
            pDrop->r.currentOrigin[2] = endpos[2];
            pDrop->s.pos.trTime = level.time;

            AxisToAngles(tagMat, vAngles);
        } else {
            vAngles[0] = pEnt->r.currentAngles[0];
            vAngles[1] = pEnt->r.currentAngles[1];
            vAngles[2] = pEnt->r.currentAngles[2];
        }

        vAngles[2] += 90.0f;
        G_SetAngle(pDrop, vAngles);

        pDrop->s.apos.trType = TR_LINEAR;
        pDrop->s.apos.trTime = level.time;
        pDrop->s.apos.trDelta[0] = crandom() * 50.0f;
        pDrop->s.apos.trDelta[1] = crandom() * 40.0f;
        pDrop->s.apos.trDelta[2] = crandom() * 60.0f;
    }

    return pDrop;
}

void Touch_Item(gentity_t *ent, gentity_t *other, qboolean bTouched)
{
    playerState_t *ps;
    const gitem_t *item;
    int clientNum;
    int makeNoise;
    int eventParm;
    char cleanname[64];

    if (!ent->active)
        return;

    ent->active = 0;

    ps = other->client ? &other->client->ps : NULL;
    if (!ps || other->health <= 0 || level.clientIsSpawning)
        return;

    item = &((const gitem_t *)imp_bg_itemlist)[ent->item.index];
    makeNoise = bTouched;
    clientNum = (int)(other - (gentity_t *)imp_g_entities);

    if (!BG_CanItemBeGrabbed(&ent->s, ps, bTouched)) {
        if (makeNoise)
            return;

        if (ent->s.clientNum == other->s.number || item->giType != IT_WEAPON)
            return;

        if (ps->weapons[item->giTag >> 5] & (1 << (item->giTag & 31))) {
            SV_GameSendServerCommand(clientNum, 0,
                                     va("%c \"GAME_PICKUP_CANTCARRYMOREAMMO\x14%s\"", 0x66, BG_GetWeaponDef(item->giTag)->szDisplayName));
            return;
        }

        switch (BG_GetWeaponDef(item->giTag)->weapSlot) {
        case WEAPSLOT_PRIMARY:
        case WEAPSLOT_PRIMARYB:
            SV_GameSendServerCommand(clientNum, 0, va("%c \"GAME_CANT_GET_PRIMARY_WEAP_MESSAGE\"", 0x66));
            return;
        default:
            return;
        }
    }

    I_strncpyz(cleanname, other->client->sess.cs.name, sizeof(cleanname));
    I_CleanStr(cleanname);

    if (item->giType == IT_WEAPON) {
        G_LogPrintf("Weapon;%d;%d;%s;%s\n", SV_GetGuid(other->s.number), other->s.number,
                    cleanname, BG_GetWeaponDef(item->giTag)->szInternalName);
    } else {
        G_LogPrintf("Item;%d;%d;%s;%s\n", SV_GetGuid(other->s.number), other->s.number,
                    cleanname, item->classname);
    }

    switch (item->giType) {
    case IT_AMMO: {
        int quantity = ent->count ? ent->count : item->quantity;
        int added = Add_Ammo(other, item->giTag, quantity, 0);

        if (!added)
            return;

        if (BG_WeaponIsClipOnly(item->giTag)) {
            SV_GameSendServerCommand(clientNum, 0,
                                     va("%c \"GAME_PICKUP_CLIPONLY_AMMO\x14%s\"", 0x66, BG_GetWeaponDef(item->giTag)->szDisplayName));
        } else {
            SV_GameSendServerCommand(clientNum, 0,
                                     va("%c \"GAME_PICKUP_AMMO\x14%s\"", 0x66, BG_GetWeaponDef(item->giTag)->szDisplayName));
        }

        Scr_AddEntity(other);
        Scr_Notify(ent, scr_const.trigger, 1);
        SV_GameSendServerCommand(clientNum, 0,
                                 va("%c \"%i\"", 0x49, BG_GetWeaponDef(item->giTag)->bSlotStackable ? 4 : 1));
        eventParm = 0x90;
        break;
    }

    case IT_HEALTH: {
        int quantity = ent->count ? ent->count : item->giTag;
        int maxHealth = ps->stats[2];
        int oldHealth = other->health;
        int normalHealth;
        int oldNormalHealth;
        int targetNormalHealth;

        if (item->giTag == 5 || item->giTag == 100)
            maxHealth *= 2;

        other->health += (int)((float)quantity * (float)ps->stats[2] * 0.01f);
        if (other->health > maxHealth)
            other->health = maxHealth;

        normalHealth = (int)((float)(other->health * 100) / (float)ps->stats[2]);
        if (normalHealth <= 0)
            normalHealth = 1;
        if (normalHealth > 100)
            normalHealth = 100;

        oldNormalHealth = (int)((float)(oldHealth * 100) / (float)ps->stats[2]);
        if (oldNormalHealth <= 0)
            oldNormalHealth = 1;

        targetNormalHealth = oldNormalHealth + quantity;
        if (targetNormalHealth > 100)
            targetNormalHealth = 100;

        if (normalHealth != targetNormalHealth)
            other->health = (ps->stats[2] * targetNormalHealth) / 100;

        ps->stats[0] = other->health;

        SV_GameSendServerCommand(clientNum, 0, va("%c \"GAME_PICKUP_HEALTH\x15%i\"", 0x66, quantity));
        SV_GameSendServerCommand(clientNum, 0, va("%c \"%i\"", 0x49, 0));
        Scr_AddEntity(other);
        Scr_Notify(ent, scr_const.trigger, 1);
        eventParm = 0x90;
        break;
    }

    case IT_WEAPON: {
        int weapon = item->giTag;
        WeaponDef *weapDef = BG_GetWeaponDef(weapon);
        int quantity;
        int clipAmmo;
        gentity_t *dropped = NULL;

        if (ent->count < 0) {
            quantity = 0;
        } else if (ent->count != 0) {
            quantity = ent->count;
        } else {
            int low = weapDef->iDropAmmoMin;
            int high = weapDef->iDropAmmoMax;

            if (high < low) {
                int tmp = high;
                high = low;
                low = tmp;
            }

            if (high == 0 && low == 0) {
                int clipSize = BG_GetAmmoClipSize(BG_ClipForWeapon(weapon));
                quantity = (int)floorf(((randomf() + 1.0f) * 0.5f) * (float)(clipSize - 1) + 0.5f) + 1;
            } else if (high < 0) {
                quantity = 0;
            } else if (high != low) {
                quantity = low + rand() % (high - low);
            } else {
                quantity = high;
            }

            ent->count = quantity;
        }

        if (quantity > BG_GetAmmoTypeMax(BG_AmmoForWeapon(weapon))) {
            ent->count = BG_GetAmmoTypeMax(BG_AmmoForWeapon(weapon));
            quantity = ent->count;
        }

        if (ent->chain < 0) {
            clipAmmo = 0;
        } else if (ent->chain != 0) {
            clipAmmo = ent->chain;
        } else if (ent->count < 0) {
            ent->chain = 0;
            clipAmmo = 0;
        } else {
            clipAmmo = BG_GetAmmoClipSize(BG_ClipForWeapon(weapon));
            if (clipAmmo > ent->count)
                clipAmmo = ent->count;
            ent->chain = clipAmmo;
            ent->count -= clipAmmo;
            quantity = ent->count;
        }

        if (clipAmmo > BG_GetAmmoClipSize(BG_ClipForWeapon(weapon))) {
            clipAmmo -= BG_GetAmmoClipSize(BG_ClipForWeapon(weapon));
            quantity += clipAmmo;
            clipAmmo = BG_GetAmmoClipSize(BG_ClipForWeapon(weapon));
        }

        if (ps->weapons[weapon >> 5] & (1 << (weapon & 31))) {
            int total = quantity + clipAmmo;
            int added = Add_Ammo(other, weapon, total, 0);

            if (added) {
                if (BG_WeaponIsClipOnly(weapon)) {
                    SV_GameSendServerCommand(clientNum, 0,
                                             va("%c \"GAME_PICKUP_CLIPONLY_AMMO\x14%s\"", 0x66, weapDef->szDisplayName));
                } else {
                    SV_GameSendServerCommand(clientNum, 0,
                                             va("%c \"GAME_PICKUP_AMMO\x14%s\"", 0x66, weapDef->szDisplayName));
                }
            }

            if (added != total) {
                ent->count -= added;
                if (ent->count <= 0) {
                    ent->chain += ent->count;
                    ent->count = -1;
                    if (ent->chain <= 0)
                        ent->chain = -1;
                }

                if (ent->count > 0 || ent->chain > 0) {
                    const dvar_t *weaponAmmoPools = *(const dvar_t **)imp_g_weaponAmmoPools;
                    if (!weaponAmmoPools->current.enabled)
                        return;
                }
            }

            Scr_AddUndefined();
            eventParm = 0x91;
        } else {
            if (BG_DoesWeaponNeedSlot(weapon)) {
                int currentWeapon = ps->weapon;

                if (currentWeapon) {
                    if (!(ps->weapons[currentWeapon >> 5] & (1 << (currentWeapon & 31))))
                        return;

                    if (!BG_IsPlayerWeaponInSlot(ps, currentWeapon, 1)) {
                        if (!BG_GetStackSlotForWeapon(ps, currentWeapon, BG_GetWeaponDef(currentWeapon)->weapSlot) &&
                            !BG_GetEmptySlotForWeapon(ps, weapon)) {
                            Com_Printf("WARNING: cannot swap out a debug weapon (can result from too many weapons given to the player)\n");
                            return;
                        }
                    }
                }

                if (!BG_GetEmptySlotForWeapon(ps, weapon) &&
                    !BG_GetStackSlotForWeapon(ps, weapon, BG_GetWeaponDef(ps->weapon)->weapSlot)) {
                    int dropSlot = 1;

                    if (weapDef->weapSlot == BG_GetWeaponDef(ps->weapon)->weapSlot) {
                        dropped = Drop_Weapon(other, ps->weapon, 0);
                    } else {
                        for (dropSlot = 1; dropSlot < 3; ++dropSlot) {
                            int dropWeapon = ps->weaponslots[dropSlot];

                            if (!ps->ammo[BG_AmmoForWeapon(weapon)] &&
                                !ps->ammoclip[BG_ClipForWeapon(weapon)]) {
                                dropped = Drop_Weapon(other, dropWeapon, 0);
                                break;
                            }
                        }

                        if (!dropped) {
                            SV_GameSendServerCommand(clientNum, 0, va("%c \"GAME_CANT_GET_PRIMARY_WEAP_MESSAGE\"", 0x66));
                            return;
                        }
                    }

                    if (!dropped)
                        return;

                    dropped->tagInfo = ent->tagInfo;
                    dropped->parent = ent->parent;
                    G_SetOrigin(dropped, ent->r.currentOrigin);
                    G_SetAngle(dropped, ent->r.currentAngles);
                    SV_LinkEntity(dropped);
                }
            }

            G_GivePlayerWeapon(ps, weapon);
            if (!bTouched)
                G_SelectWeaponIndex(clientNum, weapon);

            if (clipAmmo >= 0) {
                if (clipAmmo > BG_GetAmmoClipSize(BG_ClipForWeapon(weapon))) {
                    clipAmmo -= BG_GetAmmoClipSize(BG_ClipForWeapon(weapon));
                    quantity += clipAmmo;
                    clipAmmo = BG_GetAmmoClipSize(BG_ClipForWeapon(weapon));
                }
                ps->ammoclip[BG_ClipForWeapon(weapon)] = clipAmmo;
            }

            Add_Ammo(other, weapon, quantity, clipAmmo == -1);

            if (dropped)
                Scr_AddEntity(dropped);
            else
                Scr_AddUndefined();

            eventParm = 0x90;
        }

        Scr_AddEntity(other);
        Scr_Notify(ent, scr_const.trigger, 2);
        SV_GameSendServerCommand(clientNum, 0,
                                 va("%c \"%i\"", 0x49, weapDef->bSlotStackable ? 4 : 1));
        break;
    }

    default:
        return;
    }

    if (other->client->sess.predictItemPickup)
        G_AddPredictableEvent(other, eventParm, ent->s.eventParm);
    else
        G_AddEvent(other, eventParm, ent->s.eventParm);

    G_FreeEntity(ent);
}

void Touch_Item_Auto(gentity_t *ent, gentity_t *other, qboolean bTouched)
{
    (_ENT(ent)->active) = 1;
    Touch_Item(ent, other, bTouched);
}

void G_SpawnItem(gentity_t *ent, const gitem_t *item)
{
    int itemIndex;

    itemIndex = (int)(item - (const gitem_t *)imp_bg_itemlist);
    RegisterItem(itemIndex, 0);

    ent->item.index = (unsigned short)itemIndex;
    G_SetModel(ent, item->world_model[0]);

    if (item->giType == IT_WEAPON) {
        ent->r.mins[0] = -1.0f;
        ent->r.mins[1] = -1.0f;
        ent->r.mins[2] = -1.0f;
        ent->r.maxs[0] = 1.0f;
        ent->r.maxs[1] = 1.0f;
        ent->r.maxs[2] = 1.0f;
    } else {
        ent->r.mins[0] = -1.0f;
        ent->r.mins[1] = -1.0f;
        ent->r.mins[2] = 0.0f;
        ent->r.maxs[0] = 1.0f;
        ent->r.maxs[1] = 1.0f;
        ent->r.maxs[2] = 2.0f;
    }

    ent->r.contents = (item->giType == IT_AMMO) ? 0x405c0108 : 0x407c0108;
    ent->s.eType = 3;
    ent->s.index.item = itemIndex;

    G_DObjUpdate(ent);

    ent->s.clientNum = 0x3fe;
    ent->flags |= 0x1000;

    if (level.initializing) {
        G_SetAngle(ent, ent->r.currentAngles);
        ent->nextthink = level.time + 100;
        ent->handler = 0x10;
        return;
    }

    ent->handler = 0x11;
    if (!(ent->spawnflags & 1)) {
        ent->s.groundEntityNum = 0x3ff;
        if (item->giType == IT_WEAPON) {
            ent->r.currentAngles[2] += 90.0f;
        }
    }

    G_SetAngle(ent, ent->r.currentAngles);
    G_SetOrigin(ent, ent->r.currentOrigin);
    SV_LinkEntity(ent);
}

int Add_Ammo(gentity_t *ent, int weapon, int count, qboolean fillClip)
{
    int ammoIdx = BG_AmmoForWeapon(weapon);
    int clipIdx = BG_ClipForWeapon(weapon);
    playerState_t *ps = (playerState_t *)ent->client;
    int oldAmmo = ps->ammo[ammoIdx];
    int oldClip = ps->ammoclip[clipIdx];
    playerState_t *cl2;
    int gaveWeapon, need, avail, r, ammoIdx2, clipIdx2;

    ps->ammo[ammoIdx] = oldAmmo + count;
    if (BG_WeaponIsClipOnly(weapon))
        goto give_weapon;
    gaveWeapon = 0;

loop:
    if (fillClip)
        goto fill;
    if (gaveWeapon)
        goto fill;
clamp_ammo_chk:
    ps = (playerState_t *)ent->client;
    if (ps->ammo[ammoIdx] > BG_GetAmmoTypeMax(ammoIdx))
        goto clamp_ammo;
clamp_clip_chk:
    ps = (playerState_t *)ent->client;
    if (ps->ammoclip[clipIdx] > BG_GetAmmoClipSize(clipIdx))
        goto clamp_clip;
wd_chk:
    if (*(int *)((char *)BG_GetWeaponDef(weapon) + 0x1e4) < 0)
        goto ret;
pickup:
    ps = (playerState_t *)ent->client;
    r = BG_GetMaxPickupableAmmo(ps, weapon);
    if (r < 0)
        goto pickup_cap;
ret:
    ps = (playerState_t *)ent->client;
    return (ps->ammo[ammoIdx] - oldAmmo) + (ps->ammoclip[clipIdx] - oldClip);

clamp_ammo:
    ps = (playerState_t *)ent->client;
    ps->ammo[ammoIdx] = BG_GetAmmoTypeMax(ammoIdx);
    goto clamp_clip_chk;

clamp_clip:
    ps = (playerState_t *)ent->client;
    ps->ammoclip[clipIdx] = BG_GetAmmoClipSize(clipIdx);
    if (*(int *)((char *)BG_GetWeaponDef(weapon) + 0x1e4) >= 0)
        goto pickup;
    ps = (playerState_t *)ent->client;
    goto ret;

give_weapon:
    G_GivePlayerWeapon((playerState_t *)ent->client, weapon);
    gaveWeapon = 1;
    goto loop;

fill:
    cl2 = (playerState_t *)ent->client;
    ammoIdx2 = BG_AmmoForWeapon(weapon);
    clipIdx2 = BG_ClipForWeapon(weapon);
    if (weapon <= 0)
        goto gave_chk;
    if (weapon <= BG_GetNumWeapons())
        goto valid_fill;
gave_chk:
    if (gaveWeapon == 0)
        goto clamp_ammo_chk;
    ps = (playerState_t *)ent->client;
    ps->ammo[ammoIdx] = 0;
    ps = (playerState_t *)ent->client;
    if (ps->ammoclip[clipIdx] <= BG_GetAmmoClipSize(clipIdx))
        goto wd_chk;
    goto clamp_clip;

valid_fill:
    avail = cl2->ammo[ammoIdx2];
    need = BG_GetAmmoClipSize(clipIdx2) - cl2->ammoclip[clipIdx2];
    if (need > avail)
        need = avail;
    if (need == 0)
        goto gave_chk;
    cl2->ammo[ammoIdx2] = avail - need;
    cl2->ammoclip[clipIdx2] += need;
    goto gave_chk;

pickup_cap:
    if (BG_WeaponIsClipOnly(weapon))
        goto cliponly_cap;
    ps = (playerState_t *)ent->client;
    ps->ammo[ammoIdx] += r;
    if (ps->ammo[ammoIdx] < 0)
        ps->ammo[ammoIdx] = 0;
    goto ret;

cliponly_cap:
    ps = (playerState_t *)ent->client;
    ps->ammoclip[clipIdx] += r;
    if (ps->ammoclip[clipIdx] > 0)
        goto ret;
    ps->ammoclip[clipIdx] = 0;
    BG_TakePlayerWeapon((playerState_t *)ent->client, weapon);
    return 0;
}
