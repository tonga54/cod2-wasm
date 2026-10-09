#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include <math.h>
extern level_locals_t level;

const char *g_he_alignx[3] = { (const char *)&str_00227904, (const char *)&str_002b4d20, (const char *)&str_002b4d28 };
const char *g_he_aligny[3] = { (const char *)&str_002b4ce0, (const char *)&str_002b4ce4, (const char *)&str_002b4cec };
const char *g_he_horzalign[8] = { (const char *)&str_002b4d18, (const char *)&str_00227904, (const char *)&str_002b4d20, (const char *)&str_002b4d28, (const char *)&str_002b3f60, (const char *)&str_002b4cf4, (const char *)&str_002b4d30, (const char *)&str_002b4d08 };
const char *g_he_vertalign[8] = { (const char *)&str_002b4cd8, (const char *)&str_002b4ce0, (const char *)&str_002b4ce4, (const char *)&str_002b4cec, (const char *)&str_002b3f60, (const char *)&str_002b4cf4, (const char *)&str_002b4cfc, (const char *)&str_002b4d08 };

#define ARRAY_COUNT(x) ((int)(sizeof(x) / sizeof((x)[0])))

extern game_hudelem_t g_hudelems[1024];
static const game_hudelem_field_t fields[16];
/* g_he_font: migrated from the ILP32 data blob to typed C (re-lays-out per
 * target; trailing blob bytes were inter-symbol padding). (x64 port Stage 2.) */
const char *g_he_font[3] = {
    "default", "bigfixed", "smallfixed",
};
extern const char *g_he_alignx[3];
extern const char *g_he_aligny[3];
extern const char *g_he_horzalign[8];
extern const char *g_he_vertalign[8];
static void HECmd_SetText(scr_entref_t entref);
static void HECmd_SetPlayerNameString(scr_entref_t entref);
static void HECmd_SetMapNameString(scr_entref_t entref);
static void HECmd_SetGameTypeString(scr_entref_t entref);
static void HECmd_SetShader(scr_entref_t entref);
static void HECmd_SetTimer(scr_entref_t entref);
static void HECmd_SetTimerUp(scr_entref_t entref);
static void HECmd_SetTenthsTimer(scr_entref_t entref);
static void HECmd_SetTenthsTimerUp(scr_entref_t entref);
static void HECmd_SetClock(scr_entref_t entref);
static void HECmd_SetClockUp(scr_entref_t entref);
static void HECmd_SetValue(scr_entref_t entref);
static void HECmd_SetWaypoint(scr_entref_t entref);
static void HECmd_FadeOverTime(scr_entref_t entref);
static void HECmd_ScaleOverTime(scr_entref_t entref);
static void HECmd_MoveOverTime(scr_entref_t entref);
static void HECmd_Reset(scr_entref_t entref);
static void HECmd_Destroy(struct scr_entref_t entref);

static const BuiltinMethodDef methods[] __attribute__((used)) = {
    { "settext", (BuiltinMethod)HECmd_SetText, 0 },
    { "setplayernamestring", (BuiltinMethod)HECmd_SetPlayerNameString, 0 },
    { "setmapnamestring", (BuiltinMethod)HECmd_SetMapNameString, 0 },
    { "setgametypestring", (BuiltinMethod)HECmd_SetGameTypeString, 0 },
    { "setshader", (BuiltinMethod)HECmd_SetShader, 0 },
    { "settimer", (BuiltinMethod)HECmd_SetTimer, 0 },
    { "settimerup", (BuiltinMethod)HECmd_SetTimerUp, 0 },
    { "settenthstimer", (BuiltinMethod)HECmd_SetTenthsTimer, 0 },
    { "settenthstimerup", (BuiltinMethod)HECmd_SetTenthsTimerUp, 0 },
    { "setclock", (BuiltinMethod)HECmd_SetClock, 0 },
    { "setclockup", (BuiltinMethod)HECmd_SetClockUp, 0 },
    { "setvalue", (BuiltinMethod)HECmd_SetValue, 0 },
    { "setwaypoint", (BuiltinMethod)HECmd_SetWaypoint, 0 },
    { "fadeovertime", (BuiltinMethod)HECmd_FadeOverTime, 0 },
    { "scaleovertime", (BuiltinMethod)HECmd_ScaleOverTime, 0 },
    { "moveovertime", (BuiltinMethod)HECmd_MoveOverTime, 0 },
    { "reset", (BuiltinMethod)HECmd_Reset, 0 },
    { "destroy", (BuiltinMethod)HECmd_Destroy, 0 },
};

extern void Scr_AddFloat(float value);
extern void Scr_GetVector(unsigned int index, float *vectorValue);
extern void Scr_GetGenericField(byte *b, fieldtype_t type, int ofs);
extern void Scr_SetGenericField(byte *b, fieldtype_t type, int ofs);
extern void Scr_AddClassField(int classnum, const char *name, unsigned int offset);
extern float Scr_GetFloat(unsigned int index);
extern int Scr_GetInt(unsigned int index);
extern const char *Scr_GetIString(unsigned int index);
extern unsigned int Scr_GetConstString(unsigned int index);
extern int G_LocalizedStringIndex(const char *str);
extern void Scr_AddHudElem(game_hudelem_t *hud);
extern void Scr_AddVector(vec_t *vec);
extern void Scr_Error(const char *msg);
extern const char *va(const char *fmt, ...);
extern void Scr_ObjectError(const char *msg);
extern void Scr_FreeHudElem(game_hudelem_t *hud);
extern gentity_t *Scr_GetEntity(unsigned int index);
extern unsigned int Scr_GetNumParam(void);
extern void Scr_ParamError(unsigned int index, const char *msg);
extern const char *Scr_GetGameTypeNameForScript(const char *pszGameTypeScript);
extern qboolean SV_MapExists(const char *name);
extern void SV_SetConfigstring(int index, const char *val);
extern void Com_Printf(const char *fmt, ...);
extern void Scr_SetString(scr_string_t *to, unsigned int from);

static void HudElem_SetLocalizedString(game_hudelem_t *hud, int offset);
static void HudElem_SetBoolean(game_hudelem_t *hud, int offset);
static void HudElem_GetColor(game_hudelem_t *hud, int offset);
static void HudElem_GetAlpha(game_hudelem_t *hud, int offset);
static void HudElem_SetFontScale(game_hudelem_t *hud, int offset);
static void HudElem_SetFont(game_hudelem_t *hud, int offset);
static void HudElem_SetAlignX(game_hudelem_t *hud, int offset);
static void HudElem_SetAlignY(game_hudelem_t *hud, int offset);
static void HudElem_SetHorzAlign(game_hudelem_t *hud, int offset);
static void HudElem_SetVertAlign(game_hudelem_t *hud, int offset);
static void HudElem_GetVertAlign(game_hudelem_t *hud, int offset);
void Scr_GetHudElemField(int entnum, int offset);
void Scr_SetHudElemField(int entnum, int offset);
void Scr_FreeHudElemConstStrings(game_hudelem_t *hud);
void GScr_AddFieldsForHudElems(void);
BuiltinMethod HudElem_GetMethod(const char **pName);
void HudElem_UpdateClient(gclient_t *client, int clientNum, hudelem_update_t which);
static void HudElem_SetColor(game_hudelem_t *hud, int offset);
static void HudElem_SetAlpha(game_hudelem_t *hud, int offset);
static void HudElem_GetFont(game_hudelem_t *hud, int offset);
static void HudElem_GetAlignX(game_hudelem_t *hud, int offset);
static void HudElem_GetAlignY(game_hudelem_t *hud, int offset);
static void HudElem_GetHorzAlign(game_hudelem_t *hud, int offset);

static const game_hudelem_field_t fields[16] = {
    { "x", 0x04, F_FLOAT, 0x00000000, 0, NULL, NULL },
    { "y", 0x08, F_FLOAT, 0x00000000, 0, NULL, NULL },
    { "z", 0x0c, F_FLOAT, 0x00000000, 0, NULL, NULL },
    { "fontscale", 0x10, F_FLOAT, 0xffffffff, 0, (ScriptCallbackHudElem)HudElem_SetFontScale, NULL },
    { "font", 0x14, F_INT, 0xffffffff, 0, (ScriptCallbackHudElem)HudElem_SetFont, (ScriptCallbackHudElem)HudElem_GetFont },
    { "alignx", 0x18, F_INT, 0x00000003, 2, (ScriptCallbackHudElem)HudElem_SetAlignX, (ScriptCallbackHudElem)HudElem_GetAlignX },
    { "aligny", 0x18, F_INT, 0x00000003, 0, (ScriptCallbackHudElem)HudElem_SetAlignY, (ScriptCallbackHudElem)HudElem_GetAlignY },
    { "horzalign", 0x1c, F_INT, 0x00000007, 3, (ScriptCallbackHudElem)HudElem_SetHorzAlign, (ScriptCallbackHudElem)HudElem_GetHorzAlign },
    { "vertalign", 0x1c, F_INT, 0x00000007, 0, (ScriptCallbackHudElem)HudElem_SetVertAlign, (ScriptCallbackHudElem)HudElem_GetVertAlign },
    { "color", 0x20, F_INT, 0xffffffff, 0, (ScriptCallbackHudElem)HudElem_SetColor, (ScriptCallbackHudElem)HudElem_GetColor },
    { "alpha", 0x20, F_INT, 0xffffffff, 0, (ScriptCallbackHudElem)HudElem_SetAlpha, (ScriptCallbackHudElem)HudElem_GetAlpha },
    { "label", 0x30, F_INT, 0xffffffff, 0, (ScriptCallbackHudElem)HudElem_SetLocalizedString, NULL },
    { "sort", 0x78, F_FLOAT, 0x00000000, 0, NULL, NULL },
    { "foreground", 0x7c, F_INT, 0xffffffff, 0, (ScriptCallbackHudElem)HudElem_SetBoolean, NULL },
    { "archived", 0x88, F_INT, 0xffffffff, 0, (ScriptCallbackHudElem)HudElem_SetBoolean, NULL },
    { NULL, 0x00, F_INT, 0x00000000, 0, NULL, NULL },
};

static game_hudelem_t *HECmd_GetHudElem(scr_entref_t entref);
static void HudElem_ResetDefaults(game_hudelem_t *hud);
static void HECmd_ClearTimingFieldsForValue(hudelem_t *elem);
void HudElem_ClientDisconnect(gentity_t *ent);
void HudElem_DestroyAll(void);
static void HECmd_Destroy(struct scr_entref_t entref);
static void HECmd_SetText(scr_entref_t entref);
static void HECmd_SetPlayerNameString(scr_entref_t entref);
static void HECmd_SetMapNameString(scr_entref_t entref);
static void HECmd_SetGameTypeString(scr_entref_t entref);
static void HECmd_SetShader(scr_entref_t entref);
static void HECmd_SetTimer_Internal(scr_entref_t entref, he_type_t type, const char *cmdName);
static void HECmd_SetTenthsTimerUp(scr_entref_t entref);
static void HECmd_SetTenthsTimer(scr_entref_t entref);
static void HECmd_SetTimerUp(scr_entref_t entref);
static void HECmd_SetTimer(scr_entref_t entref);
static void HECmd_SetClock_Internal(scr_entref_t entref, he_type_t type, const char *cmdName);
static void HECmd_SetClockUp(scr_entref_t entref);
static void HECmd_SetClock(scr_entref_t entref);
static void HECmd_SetValue(scr_entref_t entref);
static void HECmd_SetWaypoint(scr_entref_t entref);
static void HECmd_FadeOverTime(scr_entref_t entref);
static void HECmd_ScaleOverTime(scr_entref_t entref);
static void HECmd_MoveOverTime(scr_entref_t entref);
static void HECmd_Reset(scr_entref_t entref);
void GScr_NewHudElem(void);
void GScr_NewClientHudElem(void);
void GScr_NewTeamHudElem(void);

extern const char *Scr_GetString(unsigned int index);
extern int I_stricmp(const char *s1, const char *s2);
extern void Scr_AddString(const char *s);
extern int G_ShaderIndex(const char *name);

static void HudElem_SetEnumString_impl(game_hudelem_t *hud_base, const game_hudelem_field_t *f, const char **names, int nameCount)
{
    char errormsg[0x800];
    int *value;
    const char *selectedName;
    int nameIndex;
    int fieldOffset;
    int mask;
    int shift;

    fieldOffset = f->ofs;
    value = (int *)((byte *)hud_base + fieldOffset);
    selectedName = (const char *)Scr_GetString(0);

    for (nameIndex = 0; nameIndex < nameCount; nameIndex++) {
        if (I_stricmp(selectedName, names[nameIndex]) == 0) {

            mask = f->mask;
            shift = f->shift;
            *value = *value & ~(mask << shift);
            *value = *value | (nameIndex << shift);
            return;
        }
    }

    sprintf(errormsg, "\"%s\" is not a valid value for hudelem field \"%s\"\nShould be one of:", selectedName, f->name);
    for (nameIndex = 0; nameIndex < nameCount; nameIndex++) {
        strncat(errormsg, va(" %s", names[nameIndex]), 0x800);
        errormsg[0x800 - 1] = '\0';
    }
    Scr_Error(errormsg);
}

static void HudElem_SetLocalizedString(game_hudelem_t *hud, int offset)
{
    const char *str;
    int fieldOffset;
    int idx;

    str = Scr_GetIString(0);
    fieldOffset = fields[offset].ofs;
    idx = G_LocalizedStringIndex(str);
    *(int *)((byte *)hud + fieldOffset) = idx;
}

static void HudElem_SetBoolean(game_hudelem_t *hud, int offset)
{
    int value;
    int fieldOffset;

    value = Scr_GetInt(0);
    fieldOffset = fields[offset].ofs;
    *(int *)((byte *)hud + fieldOffset) = value;
}

static void HudElem_GetColor(game_hudelem_t *hud, int offset)
{
    vec3_t color;

    color[0] = (float)hud->elem.color.r * (1.0f / 255.0f);
    color[1] = (float)hud->elem.color.g * (1.0f / 255.0f);
    color[2] = (float)hud->elem.color.b * (1.0f / 255.0f);
    Scr_AddVector(color);
}

static void HudElem_GetAlpha(game_hudelem_t *hud, int offset)
{
    Scr_AddFloat((float)hud->elem.color.a * (1.0f / 255.0f));
}

static void HudElem_SetFontScale(game_hudelem_t *hud, int offset)
{
    float scale = Scr_GetFloat(0);

    if (scale <= 0.0f)
        Scr_Error(va("font scale was %g; should be > 0", (double)scale));
    hud->elem.fontScale = scale;
}

static void HudElem_SetFont(game_hudelem_t *hud, int offset)
{
    const game_hudelem_field_t *f = &fields[offset];
    HudElem_SetEnumString_impl(hud, f, g_he_font, 3);
}

static void HudElem_SetAlignX(game_hudelem_t *hud, int offset)
{
    const game_hudelem_field_t *f = &fields[offset];
    HudElem_SetEnumString_impl(hud, f, g_he_alignx, 3);
}

static void HudElem_SetAlignY(game_hudelem_t *hud, int offset)
{
    const game_hudelem_field_t *f = &fields[offset];
    HudElem_SetEnumString_impl(hud, f, g_he_aligny, 3);
}

static void HudElem_SetHorzAlign(game_hudelem_t *hud, int offset)
{
    const game_hudelem_field_t *f = &fields[offset];
    HudElem_SetEnumString_impl(hud, f, g_he_horzalign, 8);
}

static void HudElem_SetVertAlign(game_hudelem_t *hud, int offset)
{
    const game_hudelem_field_t *f = &fields[offset];
    HudElem_SetEnumString_impl(hud, f, g_he_vertalign, 8);
}

static void HudElem_GetVertAlign(game_hudelem_t *hud, int offset)
{
    const game_hudelem_field_t *f = &fields[offset];
    int shift = f->shift;
    int mask = f->mask;
    int value = *(int *)((byte *)hud + f->ofs);
    value = (value >> shift) & mask;
    Scr_AddString(g_he_vertalign[value]);
}

void Scr_GetHudElemField(int entnum, int offset)
{
    const game_hudelem_field_t *f = &fields[offset];
    game_hudelem_t *hud = &g_hudelems[entnum];

    if (f->getter) {
        f->getter(hud, offset);
        return;
    }

    Scr_GetGenericField((byte *)hud, f->type, f->ofs);
}

void Scr_SetHudElemField(int entnum, int offset)
{
    const game_hudelem_field_t *f = &fields[offset];
    game_hudelem_t *hud = &g_hudelems[entnum];

    if (f->setter) {
        f->setter(hud, offset);
        return;
    }

    Scr_SetGenericField((byte *)hud, f->type, f->ofs);
}

void Scr_FreeHudElemConstStrings(game_hudelem_t *hud)
{
    int i;

    for (i = 0; fields[i].name; ++i) {
        if (fields[i].type == F_STRING)
            Scr_SetString((scr_string_t *)((byte *)hud + fields[i].ofs), 0);
    }
}

void GScr_AddFieldsForHudElems(void)
{
    int fieldIndex;

    for (fieldIndex = 0; fields[fieldIndex].name; ++fieldIndex) {
        /* The original encodes the offset as idx*stride*inv(stride) == fieldIndex via a hash,
           where stride = sizeof(game_hudelem_field_t)/sizeof(int). On x64 the struct's 3 pointers
           make that stride even (10), which has no inverse mod 2^16, so the hash no longer recovers
           the index and Scr_SetHudElemField indexes the wrong field ("string is not a float" on
           level.clock.vertAlign). The decode (fields[offset]) just wants the field index, so store
           it directly -- identical to what the hash yields on x86. */
        Scr_AddClassField(1, fields[fieldIndex].name, (unsigned short)fieldIndex);
    }
}

BuiltinMethod HudElem_GetMethod(const char **pName)
{
    int i;
    for (i = 0; i < (int)ARRAY_COUNT(methods); i++) {
        if (I_stricmp(*pName, methods[i].actionString) == 0) {
            *pName = methods[i].actionString;
            return methods[i].actionFunc;
        }
    }
    return (BuiltinMethod)0;
}

static int HudElem_ClientTeam(gclient_t *client)
{
    return client->sess.cs.team;
}

static qboolean HudElem_MatchesClient(game_hudelem_t *hud, gclient_t *client, int clientNum)
{
    if (hud->elem.type == HE_TYPE_FREE)
        return 0;

    if (hud->team && hud->team != HudElem_ClientTeam(client))
        return 0;

    if (hud->clientNum != 0x3ff && hud->clientNum != clientNum)
        return 0;

    return 1;
}

static void HudElem_CopyToClient(hudelem_t *dest, int *count, game_hudelem_t *hud)
{
    int slot = *count;

    ++*count;
    if (*count <= 31)
        memcpy(&dest[slot], &hud->elem, sizeof(hudelem_t));
}

void HudElem_UpdateClient(gclient_t *client, int clientNum, hudelem_update_t which)
{
    int updateArchival = which & HUDELEM_UPDATE_ARCHIVAL;
    int updateCurrent = which & HUDELEM_UPDATE_CURRENT;
    int archivalCount = 0;
    int currentCount = 0;
    int i;

    if (updateArchival)
        memset(client->ps.hud.archival, 0, sizeof(client->ps.hud.archival));

    if (updateCurrent)
        memset(client->ps.hud.current, 0, sizeof(client->ps.hud.current));

    if (!updateArchival && !updateCurrent)
        return;

    for (i = 0; i < 1024; ++i) {
        game_hudelem_t *hud = &g_hudelems[i];

        if (!HudElem_MatchesClient(hud, client, clientNum))
            continue;

        if (hud->archived) {
            if (updateArchival)
                HudElem_CopyToClient(client->ps.hud.archival, &archivalCount, hud);
        } else if (updateCurrent) {
            HudElem_CopyToClient(client->ps.hud.current, &currentCount, hud);
        }
    }
}

static byte HudElem_ColorFloatToByte(float value)
{
    if (!(value < 1.0f))
        return 255;
    if (!(value > 0.0f))
        return 0;

    return (byte)(int)floorf(value * 255.0f + 0.5f);
}

static void HudElem_SetColor(game_hudelem_t *hud, int offset)
{
    vec3_t color;

    Scr_GetVector(0, color);
    hud->elem.color.r = HudElem_ColorFloatToByte(color[0]);
    hud->elem.color.g = HudElem_ColorFloatToByte(color[1]);
    hud->elem.color.b = HudElem_ColorFloatToByte(color[2]);
}

static void HudElem_SetAlpha(game_hudelem_t *hud, int offset)
{
    hud->elem.color.a = HudElem_ColorFloatToByte(Scr_GetFloat(0));
}

static int HudElem_GetEnumFieldValue(game_hudelem_t *hud, int offset)
{
    const game_hudelem_field_t *f = &fields[offset];
    int value = *(int *)((byte *)hud + f->ofs);

    return (value >> f->shift) & f->mask;
}

void HudElem_GetFont(game_hudelem_t *hud, int offset)
{
    Scr_AddString(g_he_font[HudElem_GetEnumFieldValue(hud, offset)]);
}

void HudElem_GetAlignX(game_hudelem_t *hud, int offset)
{
    Scr_AddString(g_he_alignx[HudElem_GetEnumFieldValue(hud, offset)]);
}

void HudElem_GetAlignY(game_hudelem_t *hud, int offset)
{
    Scr_AddString(g_he_aligny[HudElem_GetEnumFieldValue(hud, offset)]);
}

void HudElem_GetHorzAlign(game_hudelem_t *hud, int offset)
{
    Scr_AddString(g_he_horzalign[HudElem_GetEnumFieldValue(hud, offset)]);
}

void HudElem_ClientDisconnect(gentity_t *ent)
{
    int i;

    for (i = 0; i < 1024; ++i) {
        game_hudelem_t *hud = &g_hudelems[i];
        int type = hud->elem.type;

        if (type && hud->clientNum == ent->s.number) {
            Scr_FreeHudElem(hud);
            hud->elem.type = HE_TYPE_FREE;
        }
    }
}

void HudElem_DestroyAll(void)
{
    int i;

    for (i = 0; i < 1024; ++i) {
        if (g_hudelems[i].elem.type) {
            Scr_FreeHudElem(&g_hudelems[i]);
            g_hudelems[i].elem.type = HE_TYPE_FREE;
        }
    }

    memset(g_hudelems, 0, sizeof(g_hudelems));
}

static void HECmd_Destroy(struct scr_entref_t entref)
{
    game_hudelem_t *hud = HECmd_GetHudElem(entref);

    if (!hud)
        return;

    Scr_FreeHudElem(hud);
    hud->elem.type = HE_TYPE_FREE;
}

static void HECmd_SetText(scr_entref_t entref)
{
    game_hudelem_t *hud = HECmd_GetHudElem(entref);
    const char *text = Scr_GetIString(0);

    if (!hud)
        return;

    HECmd_ClearTimingFieldsForValue(&hud->elem);
    hud->elem.type = HE_TYPE_TEXT;
    hud->elem.text = G_LocalizedStringIndex(text);
}

static void HECmd_SetPlayerNameString(scr_entref_t entref)
{
    game_hudelem_t *hud = HECmd_GetHudElem(entref);
    gentity_t *ent = Scr_GetEntity(0);

    if (!hud)
        return;

    if (!ent) {
        Com_Printf((const char *)"Invalid entity passed to hudelem setplayernamestring()\n");
        return;
    }

    if (!ent->client) {
        Com_Printf((const char *)"Invalid entity passed to hudelem setplayernamestring(), entity is not a client\n");
        return;
    }

    HECmd_ClearTimingFieldsForValue(&hud->elem);
    hud->elem.type = HE_TYPE_PLAYERNAME;
    hud->elem.value = (float)ent->s.number;
}

static void HECmd_SetMapNameString(scr_entref_t entref)
{
    game_hudelem_t *hud = HECmd_GetHudElem(entref);
    const char *mapname = Scr_GetString(0);

    if (!hud)
        return;

    if (!mapname) {
        Com_Printf((const char *)"Invalid mapname passed to hudelem setmapnamestring()\n");
        return;
    }

    if (!SV_MapExists(mapname)) {
        Com_Printf((const char *)"Invalid map name passed to hudelem setmapnamestring(), map not found\n");
        return;
    }

    SV_SetConfigstring(0x13, mapname);
    HECmd_ClearTimingFieldsForValue(&hud->elem);
    hud->elem.type = HE_TYPE_MAPNAME;
}

static void HECmd_SetGameTypeString(scr_entref_t entref)
{
    game_hudelem_t *hud = HECmd_GetHudElem(entref);
    const char *gametype = Scr_GetString(0);

    if (!hud)
        return;

    if (!gametype) {
        Com_Printf((const char *)"Invalid entity passed to hudelem setgametypestring()\n");
        return;
    }

    if (!Scr_GetGameTypeNameForScript(gametype)) {
        Com_Printf((const char *)"Invalid gametype '%s'\n", gametype);
        return;
    }

    SV_SetConfigstring(0x14, gametype);
    HECmd_ClearTimingFieldsForValue(&hud->elem);
    hud->elem.type = HE_TYPE_GAMETYPE;
    hud->elem.value = 20.0f;
}

static void HECmd_SetShader(scr_entref_t entref)
{
    game_hudelem_t *hud = HECmd_GetHudElem(entref);
    unsigned int paramCount = Scr_GetNumParam();
    int materialIndex;
    int width;
    int height;

    if (!hud)
        return;

    if (paramCount != 1 && paramCount != 3)
        Scr_Error((const char *)"USAGE: <hudelem> setShader(\"shadername\"[, optional_width, optional_height]);");

    materialIndex = G_ShaderIndex(Scr_GetString(0));

    if (paramCount == 1) {
        width = 0;
        height = 0;
    } else {
        width = Scr_GetInt(1);
        if (width < 0)
            Scr_ParamError(1, va((const char *)"width %i < 0", width));

        height = Scr_GetInt(2);
        if (height < 0)
            Scr_ParamError(2, va((const char *)"height %i < 0", height));
    }

    HECmd_ClearTimingFieldsForValue(&hud->elem);
    hud->elem.type = HE_TYPE_MATERIAL;
    hud->elem.materialIndex = materialIndex;
    hud->elem.width = width;
    hud->elem.height = height;
}

static void HECmd_SetTimer_Internal(scr_entref_t entref, he_type_t type, const char *cmdName)
{
    game_hudelem_t *hud = HECmd_GetHudElem(entref);
    int time;

    if (!hud)
        return;

    if (Scr_GetNumParam() != 1)
        Scr_Error(va((const char *)"USAGE: <hudelem> %s(time_in_seconds);\n", cmdName));

    time = (int)ceilf(Scr_GetFloat(0) * 1000.0f);
    if (time <= 0 && type != HE_TYPE_TIMER_UP)
        Scr_ParamError(0, va((const char *)"time %g should be > 0", (double)((float)time * 0.001f)));

    HECmd_ClearTimingFieldsForValue(&hud->elem);
    hud->elem.type = type;
    hud->elem.time = level.time + time;
}

static game_hudelem_t *HECmd_GetHudElem(scr_entref_t entref)
{
    if (entref.classnum != 1) {
        Scr_ObjectError((const char *)"not a hud element");
        return 0;
    }

    return &g_hudelems[entref.entnum];
}

static void HudElem_ResetDefaults(game_hudelem_t *hud)
{
    memset(&hud->elem, 0, sizeof(hud->elem));
    hud->elem.type = HE_TYPE_TEXT;
    hud->elem.fontScale = 1.0f;
    hud->elem.color.rgba = -1;
    hud->archived = 1;
}

static void HECmd_ClearTimingFieldsForValue(hudelem_t *elem)
{
    elem->width = 0;
    elem->height = 0;
    elem->materialIndex = 0;
    elem->fromWidth = 0;
    elem->fromHeight = 0;
    elem->scaleStartTime = 0;
    elem->scaleTime = 0;
    elem->fromX = 0.0f;
    elem->fromY = 0.0f;
    elem->fromAlignOrg = 0;
    elem->fromAlignScreen = 0;
    elem->time = 0;
    elem->duration = 0;
    elem->text = 0;
}

static int HECmd_MillisecondsFromSecondsRounded(float seconds)
{
    return (int)floorf(seconds * 1000.0f + 0.5f);
}

static void HECmd_ValidateTime(unsigned int paramIndex, float time, const char *tooSmallMsg, const char *tooLargeMsg)
{
    if (time <= 0.0f)
        Scr_ParamError(paramIndex, va(tooSmallMsg, (double)time));

    if (time > 60.0f)
        Scr_ParamError(paramIndex, va(tooLargeMsg, (double)time));
}

void HECmd_SetTenthsTimerUp(scr_entref_t entref)
{
    HECmd_SetTimer_Internal(entref, HE_TYPE_TENTHS_TIMER_UP, (const char *)"setTenthsTimerUp");
}

void HECmd_SetTenthsTimer(scr_entref_t entref)
{
    HECmd_SetTimer_Internal(entref, HE_TYPE_TENTHS_TIMER_DOWN, (const char *)"setTenthsTimer");
}

void HECmd_SetTimerUp(scr_entref_t entref)
{
    HECmd_SetTimer_Internal(entref, HE_TYPE_TIMER_UP, (const char *)"setTimerUp");
}

void HECmd_SetTimer(scr_entref_t entref)
{
    HECmd_SetTimer_Internal(entref, HE_TYPE_TIMER_DOWN, (const char *)"setTimer");
}

static void HECmd_SetClock_Internal(scr_entref_t entref, he_type_t type, const char *cmdName)
{
    game_hudelem_t *hud = HECmd_GetHudElem(entref);
    unsigned int paramCount;
    int time;
    int duration;
    int materialIndex;
    int width;
    int height;

    if (!hud)
        return;

    paramCount = Scr_GetNumParam();
    if (paramCount != 3 && paramCount != 5)
        Scr_Error(va((const char *)"USAGE: <hudelem> %s(time_in_seconds, total_clock_time_in_seconds, shadername[, width, height]);\n", cmdName));

    time = (int)ceilf(Scr_GetFloat(0) * 1000.0f);
    if (time <= 0 && type != HE_TYPE_CLOCK_UP)
        Scr_ParamError(0, va((const char *)"time %g should be > 0", (double)((float)time * 0.001f)));

    duration = (int)ceilf(Scr_GetFloat(1) * 1000.0f);
    if (duration <= 0)
        Scr_ParamError(1, va((const char *)"duration %g should be > 0", (double)((float)duration * 0.001f)));

    materialIndex = G_ShaderIndex(Scr_GetString(2));

    if (paramCount == 3) {
        width = 0;
        height = 0;
    } else {
        width = Scr_GetInt(3);
        if (width < 0)
            Scr_ParamError(3, va((const char *)"width %i < 0", width));

        height = Scr_GetInt(4);
        if (height < 0)
            Scr_ParamError(4, va((const char *)"height %i < 0", height));
    }

    HECmd_ClearTimingFieldsForValue(&hud->elem);
    hud->elem.type = type;
    hud->elem.time = level.time + time;
    hud->elem.duration = duration;
    hud->elem.materialIndex = materialIndex;
    hud->elem.width = width;
    hud->elem.height = height;
}

void HECmd_SetClockUp(scr_entref_t entref)
{
    HECmd_SetClock_Internal(entref, HE_TYPE_CLOCK_UP, (const char *)"setClockUp");
}

void HECmd_SetClock(scr_entref_t entref)
{
    HECmd_SetClock_Internal(entref, HE_TYPE_CLOCK_DOWN, (const char *)"setClock");
}

void HECmd_SetValue(scr_entref_t entref)
{
    game_hudelem_t *hud = HECmd_GetHudElem(entref);
    float value = Scr_GetFloat(0);

    if (!hud)
        return;

    HECmd_ClearTimingFieldsForValue(&hud->elem);
    hud->elem.type = HE_TYPE_VALUE;
    hud->elem.value = value;
}

void HECmd_SetWaypoint(scr_entref_t entref)
{
    game_hudelem_t *hud = HECmd_GetHudElem(entref);
    int value = Scr_GetInt(0);

    if (!hud)
        return;

    hud->elem.type = HE_TYPE_WAYPOINT;
    hud->elem.value = (float)value;
}

static void HECmd_FadeOverTime(scr_entref_t entref)
{
    game_hudelem_t *hud = HECmd_GetHudElem(entref);
    float fadeTime = Scr_GetFloat(0);

    if (!hud)
        return;

    HECmd_ValidateTime(0, fadeTime, (const char *)"fade time %g <= 0", (const char *)"fade time %g > 60");
    hud->elem.fadeStartTime = level.time;
    hud->elem.fadeTime = HECmd_MillisecondsFromSecondsRounded(fadeTime);
    hud->elem.fromColor = hud->elem.color;
}

static void HECmd_ScaleOverTime(scr_entref_t entref)
{
    game_hudelem_t *hud = HECmd_GetHudElem(entref);
    float scaleTime;
    int width;
    int height;

    if (!hud)
        return;

    if (Scr_GetNumParam() != 3)
        Scr_Error((const char *)"hudelem scaleOverTime(time_in_seconds, new_width, new_height)");

    scaleTime = Scr_GetFloat(0);
    HECmd_ValidateTime(0, scaleTime, (const char *)"scale time %g <= 0", (const char *)"scale time %g > 60");
    width = Scr_GetInt(1);
    height = Scr_GetInt(2);

    hud->elem.scaleStartTime = level.time;
    hud->elem.scaleTime = HECmd_MillisecondsFromSecondsRounded(scaleTime);
    hud->elem.fromWidth = hud->elem.width;
    hud->elem.fromHeight = hud->elem.height;
    hud->elem.width = width;
    hud->elem.height = height;
}

static void HECmd_MoveOverTime(scr_entref_t entref)
{
    game_hudelem_t *hud = HECmd_GetHudElem(entref);
    float moveTime = Scr_GetFloat(0);

    if (!hud)
        return;

    HECmd_ValidateTime(0, moveTime, (const char *)"move time %g <= 0", (const char *)"move time %g > 60");
    hud->elem.moveStartTime = level.time;
    hud->elem.moveTime = HECmd_MillisecondsFromSecondsRounded(moveTime);
    hud->elem.fromX = hud->elem.x;
    hud->elem.fromY = hud->elem.y;
    hud->elem.fromAlignOrg = hud->elem.alignOrg;
    hud->elem.fromAlignScreen = hud->elem.alignScreen;
}

static void HECmd_Reset(scr_entref_t entref)
{
    game_hudelem_t *hud = HECmd_GetHudElem(entref);

    if (!hud)
        return;

    HudElem_ResetDefaults(hud);
}

static game_hudelem_t *GScr_AllocHudElem(int clientNum, int team)
{
    int i;
    game_hudelem_t *hud;

    for (i = 0; i < 1024; ++i) {
        if (!g_hudelems[i].elem.type)
            break;
    }

    if (i == 1024) {
        Scr_Error((const char *)"out of hudelems");
        return 0;
    }

    hud = &g_hudelems[i];
    HudElem_ResetDefaults(hud);
    hud->clientNum = clientNum;
    hud->team = team;
    return hud;
}

void GScr_NewHudElem(void)
{
    game_hudelem_t *hud = GScr_AllocHudElem(0x3ff, 0);

    if (hud)
        Scr_AddHudElem(hud);
}

void GScr_NewClientHudElem(void)
{
    gentity_t *ent = Scr_GetEntity(0);
    game_hudelem_t *hud;

    if (!ent->client)
        Scr_ParamError(0, (const char *)"not a client");

    hud = GScr_AllocHudElem(ent->s.number, 0);
    if (hud)
        Scr_AddHudElem(hud);
}

void GScr_NewTeamHudElem(void)
{
    scr_string_t teamName = Scr_GetConstString(0);
    scr_const_t *scrConst = (scr_const_t *)imp_scr_const;
    int team;
    game_hudelem_t *hud;

    if (teamName == scrConst->allies) {
        team = TEAM_ALLIES;
    } else if (teamName == scrConst->axis) {
        team = TEAM_AXIS;
    } else if (teamName == scrConst->spectator) {
        team = TEAM_SPECTATOR;
    } else {
        Scr_ParamError(0, va((const char *)"team \"%s\" should be \"allies\", \"axis\", or \"spectator\"", Scr_GetString(0)));
        team = TEAM_FREE;
    }

    hud = GScr_AllocHudElem(0x3ff, team);
    if (hud)
        Scr_AddHudElem(hud);
}

const char str_00227904[] = "left";
const char str_002b4cd8[] = "subtop";
const char str_002b4ce0[] = "top";
const char str_002b4ce4[] = "middle";
const char str_002b4cec[] = "bottom";
const char str_002b4cf4[] = "noscale";
const char str_002b4cfc[] = "alignto480";
const char str_002b4d08[] = "center_safearea";
const char str_002b4d18[] = "subleft";
const char str_002b4d20[] = "center";
const char str_002b4d28[] = "right";
const char str_002b4d30[] = "alignto640";
