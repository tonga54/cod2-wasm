#include "common_types.h"
#include "imports.h"

extern unsigned char G_FreeEntity();
extern void SP_corona();
extern void SP_info_notnull();
extern void SP_info_null();
extern void SP_light();
extern void SP_misc_model();
extern void SP_script_brushmodel();
extern void SP_script_model();
extern void SP_script_origin();
extern void SP_trigger_damage();
extern void SP_trigger_disk();
extern void SP_trigger_hurt();
extern void SP_trigger_lookat();
extern void SP_trigger_multiple();
extern void SP_trigger_once();
extern void SP_trigger_radius();
extern void SP_turret();
extern void trigger_use();
extern void trigger_use_touch();
extern spawn_t spawns[24];

extern void Scr_AddUndefined(void);
extern qboolean G_SpawnStringInternal(SpawnVar *spawnVar, const char *key, const char *defaultString, const char **out);
extern void Scr_AddFields(const char *name, const char *extension);
extern void Scr_AddEntityNum(int entNum, int classnum);
extern int Scr_ExecEntThreadNum(int entNum, int classnum, scr_func_t handle, unsigned int paramcount);
extern void Scr_NotifyNum(int entNum, int classnum, int stringValue, unsigned int paramcount);
extern void Scr_Error(const char *msg);
extern void Com_Printf(const char *fmt, ...);
extern int atoi(const char *str);
extern double atof(const char *str);
extern unsigned int Scr_FindField(const char *name, int *type);
extern void Scr_AddInt(int value);
extern void Scr_AddString(const char *value);
extern void Scr_AddFloat(float value);
extern void Scr_AddVector(const float *value);
extern void Scr_AddObject(unsigned int id);
extern void Scr_AddConstString(unsigned int value);
extern void Scr_SetString(scr_string_t *to, unsigned int value);
extern const char *SL_ConvertToString(unsigned int stringValue);
extern int G_GetWeaponIndexForName(const char *name);
extern void *BG_GetWeaponDef(int weaponIndex);
extern const gitem_t *BG_FindItemForWeapon(int weaponIndex);
extern void G_SpawnItem(gentity_t *ent, const gitem_t *item);
extern void Scr_AddClassField(int classnum, const char *name, unsigned int offset);
extern void GScr_AddFieldsForClient(void);
extern void Scr_ParamError(unsigned int index, const char *error);
extern scr_entref_t Scr_GetEntityRef(unsigned int index);
extern void Scr_FreeHudElemConstStrings(game_hudelem_t *hud);
extern void Scr_FreeEntityNum(int entnum, int classnum);
extern void Scr_SetDynamicEntityField(int entnum, int classnum, unsigned int index);
extern unsigned int G_NewString(const char *string);
extern int I_stricmp(const char *s0, const char *s1);
extern unsigned char G_SetModel(gentity_t *ent, const char *modelName);
extern const char *G_ModelName(int index);
extern unsigned int Scr_GetConstString(unsigned int index);
extern const char *Scr_GetString(unsigned int index);
extern int Scr_GetOffset(int classnum, const char *name);
extern unsigned int Scr_GetNumParam(void);
extern void Scr_MakeArray(void);
extern void Scr_AddArray(void);
extern int Scr_GetInt(unsigned int index);
extern float Scr_GetFloat(unsigned int index);
extern void Scr_GetVector(unsigned int index, float *vectorValue);
extern unsigned int Scr_GetConstStringIncludeNull(unsigned int index);
extern void Scr_SetHudElemField(int entnum, int offset);
extern void Scr_GetHudElemField(int entnum, int offset);
extern void Scr_SetClientField(gclient_t *client, int offset);
extern void Scr_GetClientField(gclient_t *client, int offset);
extern qboolean G_ParseSpawnVars(SpawnVar *spawnVar);
extern gentity_t *G_Spawn(void);
extern unsigned char G_SetOrigin(gentity_t *ent, const vec_t *origin);
extern unsigned char G_SetAngle(gentity_t *ent, const vec_t *angle);
extern void Scr_AddExecThread(scr_func_t handle, unsigned int paramcount);
extern unsigned int Scr_GetObject(unsigned int index);
extern void Scr_SetStructField(unsigned int structId, unsigned int index);
extern scr_thread_t Scr_ExecThread(scr_func_t handle, unsigned int paramcount);
extern void Scr_FreeThread(int handle);
extern void SV_ResetEntityParsePoint(void);
extern void Com_Error(int code, const char *fmt, ...);
extern void SV_SetConfigstring(int index, const char *val);
extern char *va(const char *format, ...);
extern void Dvar_SetFloat(const dvar_t *dvar, float value);
extern const dvar_t *g_gravity;
extern const dvar_t *g_motd;

/* offsets must track the real gentity_s layout; the literals were the x86 ABI and are wrong on x64
   (entityShared r + the client/pTurretInfo pointers shift everything after entityState). offsetof
   reproduces the exact x86 numbers and is correct on x64. */
static const ent_field_t fields[11] = {
    { "classname", (int)offsetof(struct gentity_s, classname), F_STRING, NULL },
    { "origin", (int)offsetof(struct gentity_s, r.currentOrigin), F_VECTOR, NULL },
    { "model", (int)offsetof(struct gentity_s, model), F_MODEL, NULL },
    { "spawnflags", (int)offsetof(struct gentity_s, spawnflags), F_INT, NULL },
    { "target", (int)offsetof(struct gentity_s, target), F_STRING, NULL },
    { "targetname", (int)offsetof(struct gentity_s, targetname), F_STRING, NULL },
    { "count", (int)offsetof(struct gentity_s, count), F_INT, NULL },
    { "health", (int)offsetof(struct gentity_s, health), F_INT, NULL },
    { "dmg", (int)offsetof(struct gentity_s, damage), F_INT, NULL },
    { "angles", (int)offsetof(struct gentity_s, r.currentAngles), F_VECTOR, NULL },
    { NULL, 0, F_INT, NULL }
};

enum {
    GSP_CS_GAME_VERSION = 2,
    GSP_CS_AMBIENT = 3,
    GSP_CS_MESSAGE = 4,
    GSP_CS_NORTHYAW = 11,
    GSP_CS_LEVEL_START_TIME = 13,
    GSP_CS_MOTD = 14,
    GSP_ENTITYNUM_WORLD = 1022
};

qboolean G_SpawnString(const char *key, const char *defaultString, const char **out);
static void Scr_ReadOnlyField(gentity_t *ent, int offset);
static unsigned int __attribute_regparm__(2) G_SetEntityScriptVariableInternal(const char *key, const char *value);
void G_DuplicateEntityFields(gentity_t *dest, const gentity_t *source);
static const gitem_t *__attribute_regparm__(1) G_GetItemForClassname(const char *classname);
qboolean G_CallSpawnEntity(gentity_t *ent);
void GScr_AddFieldsForEntity(void);
void GScr_AddFieldsForRadiant(void);
void Scr_AddEntity(gentity_t *ent);
gentity_t *Scr_GetEntity(unsigned int index);
void Scr_FreeHudElem(game_hudelem_t *hud);
void Scr_AddHudElem(game_hudelem_t *hud);
scr_thread_t Scr_ExecEntThread(gentity_t *ent, scr_func_t handle, unsigned int paramcount);
void Scr_Notify(gentity_t *ent, unsigned short stringValue, unsigned int paramcount);
static void __attribute_regparm__(3) G_ParseEntityField(const char *key, const char *value, gentity_t *ent);
void Scr_GetGenericField(byte *b, fieldtype_t type, int ofs);
void Scr_GetEnt(void);
void Scr_GetEntArray(void);
qboolean G_SpawnFloat(const char *key, const char *defaultString, float *out);
qboolean G_SpawnInt(const char *key, const char *defaultString, int *out);
qboolean G_SpawnVector(const char *key, const char *defaultString, float *out);
void SP_worldspawn(void);
void G_LoadStructs(void);
void Scr_SetGenericField(byte *b, fieldtype_t type, int ofs);
qboolean Scr_SetObjectField(unsigned int classnum, int entnum, int offset);
void Scr_GetObjectField(unsigned int classnum, int entnum, int offset);
void Scr_FreeEntity(gentity_t *ent);
void G_CallSpawn(void);
void G_SpawnEntitiesFromString(void);

static inline __attribute__((always_inline)) level_locals_t *G_Level(void)
{
    return (level_locals_t *)imp_level;
}

static inline __attribute__((always_inline)) SpawnVar *G_LevelSpawnVar(void)
{
    return &G_Level()->spawnVar;
}

static inline __attribute__((always_inline)) scr_data_t *G_ScrData(void)
{
    return (scr_data_t *)imp_g_scr_data;
}

static scr_const_t *G_ScrConst(void)
{
    return (scr_const_t *)imp_scr_const;
}

static inline __attribute__((always_inline)) gentity_t *G_Entities(void)
{
    return (gentity_t *)imp_g_entities;
}

static inline __attribute__((always_inline)) int G_HudElemIndex(const game_hudelem_t *hud)
{
    return (int)(hud - (const game_hudelem_t *)imp_g_hudelems);
}

static qboolean Scr_SetEntityField(int entnum, int offset)
{
    gentity_t *ent = &G_Entities()[entnum];
    const ent_field_t *field;

    if ((offset & 0xC000) == 0xC000) {
        if (!ent || !ent->client)
            return 0;
        Scr_SetClientField(ent->client, offset & 0x3FFF);
        return 1;
    }

    if (offset < 0 || offset >= 10 || !fields[offset].name)
        return 1;

    field = &fields[offset];
    if (field->callback) {
        ((void (*)(gentity_t *, int))field->callback)(ent, offset);
    } else {
        Scr_SetGenericField((byte *)ent, field->type, field->ofs);
    }

    return 1;
}

static void Scr_GetEntityField(int entnum, int offset)
{
    gentity_t *ent = &G_Entities()[entnum];

    if ((offset & 0xC000) == 0xC000) {
        if (ent && ent->client) {
            Scr_GetClientField(ent->client, offset & 0x3FFF);
        } else {
            Scr_AddUndefined();
        }
        return;
    }

    if (offset < 0 || offset >= 10 || !fields[offset].name) {
        Scr_AddUndefined();
        return;
    }

    Scr_GetGenericField((byte *)ent, fields[offset].type, fields[offset].ofs);
}

static void Scr_FreeEntityConstStrings(gentity_t *ent)
{
    const ent_field_t *field;
    int i;

    for (field = fields; field->name; ++field) {
        if (field->type == F_STRING) {
            Scr_SetString((scr_string_t *)((byte *)ent + field->ofs), 0);
        }
    }

    for (i = 0; i < 7; ++i) {
        ent->attachModelNames[i] = 0;
        Scr_SetString(&ent->attachTagNames[i], 0);
    }
}

static inline __attribute__((always_inline)) void G_ParseEntityFields(gentity_t *ent)
{
    int i;

    for (i = 0; i < G_LevelSpawnVar()->numSpawnVars; ++i) {
        G_ParseEntityField(G_LevelSpawnVar()->spawnVars[i][0], G_LevelSpawnVar()->spawnVars[i][1], ent);
    }

    G_SetOrigin(ent, ent->r.currentOrigin);
    G_SetAngle(ent, ent->r.currentAngles);
}

static void G_SpawnStruct(SpawnVar *spawnVar)
{
    unsigned int structId;
    unsigned int index;
    int i;

    Scr_AddExecThread(G_ScrData()->createstruct, 0);
    structId = Scr_GetObject(0);

    for (i = 0; i < spawnVar->numSpawnVars; ++i) {
        index = G_SetEntityScriptVariableInternal(spawnVar->spawnVars[i][0], spawnVar->spawnVars[i][1]);
        if (index) {
            Scr_SetStructField(structId, index);
        }
    }
}

static inline __attribute__((always_inline)) qboolean G_SpawnString_core(const char *key, const char *defaultString, const char **out)
{
    return G_SpawnStringInternal(G_LevelSpawnVar(), key, defaultString, out);
}

qboolean G_SpawnString(const char *key, const char *defaultString, const char **out)
{
    return G_SpawnString_core(key, defaultString, out);
}

static void Scr_ReadOnlyField(gentity_t *ent, int offset)
{
    Scr_Error((const char *)"Tried to set a read only entity field");
}

static unsigned int __attribute_regparm__(2) G_SetEntityScriptVariableInternal(const char *key, const char *value)
{
    int type;
    unsigned int index;

    index = Scr_FindField(key, &type);
    if (!index) {
        return 0;
    }

    switch (type) {
    case 2:
        Scr_AddString(value);
        break;
    case 4: {
        vec3_t vec = { 0.0f, 0.0f, 0.0f };
        sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
        Scr_AddVector(vec);
        break;
    }
    case 5:
        Scr_AddFloat((float)atof(value));
        break;
    case 6:
        Scr_AddInt(atoi(value));
        break;
    default:
        break;
    }

    return index;
}

void G_DuplicateEntityFields(gentity_t *dest, const gentity_t *source)
{
    const ent_field_t *field;

    for (field = fields; field->name; ++field) {
        switch (field->type) {
        case F_INT:
        case F_FLOAT:
            *(int *)((byte *)dest + field->ofs) = *(const int *)((const byte *)source + field->ofs);
            break;
        case F_STRING:
            Scr_SetString((scr_string_t *)((byte *)dest + field->ofs), *(const scr_string_t *)((const byte *)source + field->ofs));
            break;
        case F_VECTOR: {
            vec3_t *destVec = (vec3_t *)((byte *)dest + field->ofs);
            const vec3_t *sourceVec = (const vec3_t *)((const byte *)source + field->ofs);

            (*destVec)[0] = (*sourceVec)[0];
            (*destVec)[1] = (*sourceVec)[1];
            (*destVec)[2] = (*sourceVec)[2];
            break;
        }
        case F_MODEL:
            *(byte *)((byte *)dest + field->ofs) = *(const byte *)((const byte *)source + field->ofs);
            break;
        default:
            break;
        }
    }
}

static const gitem_t *__attribute_regparm__(1) G_GetItemForClassname(const char *classname)
{
    int weaponIndex;
    int itemIndex;
    int bgNumItems;
    gitem_t *bgItemList;

    if (!strncmp(classname, "weapon_", 7)) {
        weaponIndex = G_GetWeaponIndexForName(classname + 7);
        if (weaponIndex) {
            BG_GetWeaponDef(weaponIndex);
            return BG_FindItemForWeapon(weaponIndex);
        }
    }

    bgNumItems = *(int *)imp_bg_numItems;
    if (bgNumItems <= 0x81) {
        return NULL;
    }

    bgItemList = (gitem_t *)imp_bg_itemlist;
    for (itemIndex = 0x81; itemIndex < bgNumItems; ++itemIndex) {
        if (!strcmp(bgItemList[itemIndex].classname, classname)) {
            return &bgItemList[itemIndex];
        }
    }

    return NULL;
}

qboolean G_CallSpawnEntity(gentity_t *ent)
{
    const char *classname;
    const gitem_t *item;
    int i;

    if (!ent->classname) {
        Com_Printf((const char *)"G_CallSpawnEntity: NULL classname\n");
        return 0;
    }

    classname = SL_ConvertToString(ent->classname);
    item = G_GetItemForClassname(classname);
    if (item) {
        G_SpawnItem(ent, item);
        return 1;
    }

    for (i = 0; spawns[i].name; ++i) {
        if (!strcmp(spawns[i].name, classname)) {
            ((void (*)(gentity_t *))spawns[i].spawn)(ent);
            return 1;
        }
    }

    Com_Printf("%s doesn't have a spawn function\n", SL_ConvertToString(ent->classname));
    return 0;
}

void GScr_AddFieldsForEntity(void)
{
    const ent_field_t *field;

    for (field = fields; field->name; ++field) {
        Scr_AddClassField(0, field->name, (unsigned short)(field - fields));
    }

    GScr_AddFieldsForClient();
}

void GScr_AddFieldsForRadiant(void)
{
    Scr_AddFields("radiant", (const void *)"txt");
}

void Scr_AddEntity(gentity_t *ent)
{
    Scr_AddEntityNum(ent->s.number, 0);
}

gentity_t *Scr_GetEntity(unsigned int index)
{
    scr_entref_t ref;

    ref = Scr_GetEntityRef(index);
    if (ref.classnum) {
        Scr_ParamError(index, (const char *)"not an entity");
        return NULL;
    }

    return &G_Entities()[ref.entnum];
}

void Scr_FreeHudElem(game_hudelem_t *hud)
{
    Scr_FreeHudElemConstStrings(hud);
    Scr_FreeEntityNum(G_HudElemIndex(hud), 1);
}

void Scr_AddHudElem(game_hudelem_t *hud)
{
    Scr_AddEntityNum(G_HudElemIndex(hud), 1);
}

scr_thread_t Scr_ExecEntThread(gentity_t *ent, scr_func_t handle, unsigned int paramcount)
{
    return (unsigned short)Scr_ExecEntThreadNum(ent->s.number, 0, handle, paramcount);
}

void Scr_Notify(gentity_t *ent, unsigned short stringValue, unsigned int paramcount)
{
    Scr_NotifyNum(ent->s.number, 0, stringValue, paramcount);
}

static void __attribute_regparm__(3) G_ParseEntityField(const char *key, const char *value, gentity_t *ent)
{
    const ent_field_t *field;
    unsigned int index;

    for (field = fields; field->name; ++field) {
        if (!I_stricmp(field->name, key)) {
            byte *fieldData = (byte *)ent + field->ofs;

            switch (field->type) {
            case F_INT:
                *(int *)fieldData = atoi(value);
                break;
            case F_FLOAT:
                *(float *)fieldData = (float)atof(value);
                break;
            case F_STRING:
                Scr_SetString((scr_string_t *)fieldData, 0);
                *(scr_string_t *)fieldData = (scr_string_t)G_NewString(value);
                break;
            case F_VECTOR: {
                vec3_t vec = { 0.0f, 0.0f, 0.0f };
                sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
                ((vec3_t *)fieldData)[0][0] = vec[0];
                ((vec3_t *)fieldData)[0][1] = vec[1];
                ((vec3_t *)fieldData)[0][2] = vec[2];
                break;
            }
            case F_MODEL:
                if (*value == '*') {
                    /* Inline BSP models are collision/draw handles, not entries
                       in the 8-bit XModel precache table. */
                    ent->s.index.brushmodel = atoi(value + 1);
                    ent->model = 0;
                } else {
                    G_SetModel(ent, value);
                }
                break;
            default:
                break;
            }

            return;
        }
    }

    index = G_SetEntityScriptVariableInternal(key, value);
    if (index) {
        Scr_SetDynamicEntityField(ent->s.number, 0, index);
    }
}

void Scr_GetGenericField(byte *b, fieldtype_t type, int ofs)
{
    switch (type) {
    case F_INT:
        Scr_AddInt(*(int *)(b + ofs));
        break;
    case F_FLOAT:
        Scr_AddFloat(*(float *)(b + ofs));
        break;
    case F_LSTRING:
        Scr_AddString((const char *)(b + ofs));
        break;
    case F_STRING: {
        scr_string_t stringValue = *(scr_string_t *)(b + ofs);
        if (stringValue) {
            Scr_AddConstString(stringValue);
        } else {
            Scr_AddString("");
        }
        break;
    }
    case F_VECTOR:
        Scr_AddVector((float *)(b + ofs));
        break;
    case F_ENTITY: {
        gentity_t *ent = *(gentity_t **)(b + ofs);
        if (ent) {
            Scr_AddEntityNum(ent->s.number, 0);
        } else {
            Scr_AddUndefined();
        }
        break;
    }
    case F_VECTORHACK: {
        vec3_t vec = { 0.0f, *(float *)(b + ofs), 0.0f };
        Scr_AddVector(vec);
        break;
    }
    case F_OBJECT: {
        unsigned int objectId = *(scr_string_t *)(b + ofs);
        if (objectId) {
            Scr_AddObject(objectId);
        }
        break;
    }
    case F_MODEL:
        Scr_AddString(G_ModelName(*(byte *)(b + ofs)));
        break;
    default:
        break;
    }
}

void Scr_GetEnt(void)
{
    scr_string_t name;
    int offset;
    const ent_field_t *field;
    gentity_t *found;
    int i;
    gentity_t *ent;

    name = (scr_string_t)Scr_GetConstString(0);
    offset = Scr_GetOffset(0, Scr_GetString(1));
    if (offset < 0) {
        return;
    }

    field = &fields[offset];
    if (field->type != F_STRING) {
        return;
    }

    found = NULL;
    ent = G_Entities();
    for (i = 0; i < G_Level()->num_entities; ++i, ++ent) {
        scr_string_t fieldValue;

        if (!ent->r.inuse) {
            continue;
        }

        fieldValue = *(scr_string_t *)((byte *)ent + field->ofs);
        if (!fieldValue || fieldValue != name) {
            continue;
        }

        if (found) {
            Scr_Error((const char *)"getent used with more than one entity");
        }
        found = ent;
    }

    if (found) {
        Scr_AddEntityNum(found->s.number, 0);
    }
}

void Scr_GetEntArray(void)
{
    gentity_t *ent;
    int i;

    if (!Scr_GetNumParam()) {
        Scr_MakeArray();
        ent = G_Entities();
        for (i = 0; i < G_Level()->num_entities; ++i, ++ent) {
            if (!ent->r.inuse) {
                continue;
            }
            Scr_AddEntityNum(ent->s.number, 0);
            Scr_AddArray();
        }
        return;
    }

    {
        scr_string_t name = (scr_string_t)Scr_GetConstString(0);
        int offset = Scr_GetOffset(0, Scr_GetString(1));
        const ent_field_t *field;

        if (offset < 0) {
            return;
        }

        field = &fields[offset];
        if (field->type != F_STRING) {
            return;
        }

        Scr_MakeArray();
        ent = G_Entities();
        for (i = 0; i < G_Level()->num_entities; ++i, ++ent) {
            scr_string_t fieldValue;

            if (!ent->r.inuse) {
                continue;
            }

            fieldValue = *(scr_string_t *)((byte *)ent + field->ofs);
            if (!fieldValue || fieldValue != name) {
                continue;
            }

            Scr_AddEntityNum(ent->s.number, 0);
            Scr_AddArray();
        }
    }
}

qboolean G_SpawnFloat(const char *key, const char *defaultString, float *out)
{
    const char *s;
    qboolean present;

    present = G_SpawnStringInternal(G_LevelSpawnVar(), key, defaultString, &s);
    *out = (float)atof(s);
    return present;
}

qboolean G_SpawnInt(const char *key, const char *defaultString, int *out)
{
    const char *s;
    qboolean present;

    present = G_SpawnStringInternal(G_LevelSpawnVar(), key, defaultString, &s);
    *out = atoi(s);
    return present;
}

qboolean G_SpawnVector(const char *key, const char *defaultString, float *out)
{
    const char *s;
    qboolean present;

    present = G_SpawnStringInternal(G_LevelSpawnVar(), key, defaultString, &s);
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
    sscanf(s, "%f %f %f", &out[0], &out[1], &out[2]);
    return present;
}

void SP_worldspawn(void)
{
    gentity_t *world;
    const char *s;

    G_SpawnString_core((const char *)"classname", (const char *)"", &s);
    if (I_stricmp(s, (const char *)"worldspawn")) {
        Com_Error(ERR_DROP, (const char *)"\x15SP_worldspawn: The first entity isn't 'worldspawn'");
    }

    world = &G_Entities()[GSP_ENTITYNUM_WORLD];

    SV_SetConfigstring(GSP_CS_GAME_VERSION, (const char *)"cod");
    SV_SetConfigstring(GSP_CS_LEVEL_START_TIME, va((const char *)"%i", G_Level()->startTime));

    G_SpawnString_core((const char *)"ambienttrack", (const char *)"", &s);
    if (s[0]) {
        SV_SetConfigstring(GSP_CS_AMBIENT, va((const char *)"n\\%s", s));
    } else {
        SV_SetConfigstring(GSP_CS_AMBIENT, (const char *)"");
    }

    G_SpawnString_core((const char *)"message", (const char *)"", &s);
    SV_SetConfigstring(GSP_CS_MESSAGE, s);
    SV_SetConfigstring(GSP_CS_MOTD, g_motd->current.string);

    G_SpawnString_core((const char *)"gravity", (const char *)"800", &s);
    Dvar_SetFloat(g_gravity, (float)atof(s));

    G_SpawnString_core((const char *)"northyaw", (const char *)"", &s);
    if (s[0]) {
        SV_SetConfigstring(GSP_CS_NORTHYAW, s);
    } else {
        SV_SetConfigstring(GSP_CS_NORTHYAW, (const char *)"0");
    }

    G_SpawnString_core((const char *)"spawnflags", (const char *)"0", &s);
    world->spawnflags = atoi(s);
    world->s.number = GSP_ENTITYNUM_WORLD;
    Scr_SetString(&world->classname, G_ScrConst()->worldspawn);
    world->r.inuse = 1;
}

void G_LoadStructs(void)
{
    const char *classname;
    scr_thread_t threadId;
    SpawnVar *spawnVar;

    threadId = Scr_ExecThread(G_ScrData()->initstructs, 0);
    Scr_FreeThread(threadId);

    for (spawnVar = G_LevelSpawnVar(); G_ParseSpawnVars(spawnVar); spawnVar = G_LevelSpawnVar()) {
        G_SpawnStringInternal(spawnVar, (const char *)"classname", (const char *)"", &classname);
        if (!strcmp("script_struct", classname)) {
            G_SpawnStruct(spawnVar);
        }
    }

    SV_ResetEntityParsePoint();
}

void Scr_SetGenericField(byte *b, fieldtype_t type, int ofs)
{
    vec3_t vec;

    switch (type) {
    case F_INT:
        *(int *)(b + ofs) = Scr_GetInt(0);
        break;
    case F_FLOAT:
        *(float *)(b + ofs) = Scr_GetFloat(0);
        break;
    case F_STRING:
        Scr_SetString((scr_string_t *)(b + ofs), Scr_GetConstStringIncludeNull(0));
        break;
    case F_VECTOR:
        Scr_GetVector(0, vec);
        ((float *)(b + ofs))[0] = vec[0];
        ((float *)(b + ofs))[1] = vec[1];
        ((float *)(b + ofs))[2] = vec[2];
        break;
    case F_ENTITY:
        *(gentity_t **)(b + ofs) = Scr_GetEntity(0);
        break;
    case F_VECTORHACK:
        Scr_GetVector(0, vec);
        *(float *)(b + ofs) = vec[1];
        break;
    default:
        break;
    }
}

qboolean Scr_SetObjectField(unsigned int classnum, int entnum, int offset)
{
    switch (classnum) {
    case 0:
        return Scr_SetEntityField(entnum, offset);
    case 1:
        Scr_SetHudElemField(entnum, offset);
        return 1;
    default:
        return 1;
    }
}

void Scr_GetObjectField(unsigned int classnum, int entnum, int offset)
{
    switch (classnum) {
    case 0:
        Scr_GetEntityField(entnum, offset);
        break;
    case 1:
        Scr_GetHudElemField(entnum, offset);
        break;
    default:
        break;
    }
}

void Scr_FreeEntity(gentity_t *ent)
{
    Scr_FreeEntityConstStrings(ent);
    Scr_FreeEntityNum(ent->s.number, 0);
}

void G_CallSpawn(void)
{
    const char *classname;
    const gitem_t *item;
    spawn_t *spawn;
    gentity_t *ent;

    G_SpawnString_core((const char *)"classname", (const char *)"", &classname);

    if (!classname) {
        Com_Printf((const char *)"G_CallSpawn: NULL classname\n");
        return;
    }

    item = G_GetItemForClassname(classname);
    if (item) {
        ent = G_Spawn();
        G_ParseEntityFields(ent);
        G_SpawnItem(ent, item);
        return;
    }

    for (spawn = spawns; spawn->name; ++spawn) {
        if (!strcmp(spawn->name, classname)) {
            break;
        }
    }

    if (!spawn->name) {
        ent = G_Spawn();
        G_ParseEntityFields(ent);
        return;
    }

    if ((void (*)(gentity_t *))spawn->spawn != (void (*)(gentity_t *))imp_G_FreeEntity) {
        ent = G_Spawn();
        G_ParseEntityFields(ent);
        ((void (*)(gentity_t *))spawn->spawn)(ent);
    }
}

void G_SpawnEntitiesFromString(void)
{
    SpawnVar *spawnVar;

    if (!G_ParseSpawnVars(G_LevelSpawnVar())) {
        Com_Error(1, (const char *)"\x15SpawnEntities: no entities");
    }

    SP_worldspawn();

    spawnVar = G_LevelSpawnVar();
    while (G_ParseSpawnVars(spawnVar)) {
        G_CallSpawn();
    }
}

spawn_t spawns[24] = {
    { (const char *)&str_002b3284, &SP_info_null },
    { (const char *)&str_002b3290, &SP_info_notnull },
    { (const char *)&str_002b32a0, &SP_info_null },
    { (const char *)&str_002b32ac, &SP_trigger_multiple },
    { (const char *)&str_002b32c0, &SP_trigger_radius },
    { (const char *)&str_002b32d0, &SP_trigger_disk },
    { (const char *)&str_002b32e0, &SP_trigger_hurt },
    { (const char *)&str_002b32f0, &SP_trigger_once },
    { (const char *)&str_00219e40, &SP_light },
    { (const char *)&str_0021c214, &SP_misc_model },
    { (const char *)&str_002b3300, &SP_turret },
    { (const char *)&str_002b330c, &SP_turret },
    { (const char *)&str_002b3318, &SP_corona },
    { (const char *)&str_002b3320, &trigger_use },
    { (const char *)&str_002b332c, &trigger_use_touch },
    { (const char *)&str_002b3340, &SP_trigger_damage },
    { (const char *)&str_002b3350, &SP_trigger_lookat },
    { (const char *)&str_002b3360, &SP_script_brushmodel },
    { (const char *)&str_002b3374, &SP_script_model },
    { (const char *)&str_002b3384, &SP_script_origin },
    { (const char *)&str_002b3394, &G_FreeEntity },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 }
};

const char str_00219e40[] = "light";
const char str_0021c214[] = "misc_model";
const char str_002b3284[] = "info_null";
const char str_002b3290[] = "info_notnull";
const char str_002b32a0[] = "func_group";
const char str_002b32ac[] = "trigger_multiple";
const char str_002b32c0[] = "trigger_radius";
const char str_002b32d0[] = "trigger_disk";
const char str_002b32e0[] = "trigger_hurt";
const char str_002b32f0[] = "trigger_once";
const char str_002b3300[] = "misc_mg42";
const char str_002b330c[] = "misc_turret";
const char str_002b3318[] = "corona";
const char str_002b3320[] = "trigger_use";
const char str_002b332c[] = "trigger_use_touch";
const char str_002b3340[] = "trigger_damage";
const char str_002b3350[] = "trigger_lookat";
const char str_002b3360[] = "script_brushmodel";
const char str_002b3374[] = "script_model";
const char str_002b3384[] = "script_origin";
const char str_002b3394[] = "script_struct";
