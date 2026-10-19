#include "common_types.h"
#include "imports.h"
#include "headers/PC/bgame/bg_weapon_camo.h"
#include <string.h>

extern unsigned int Scr_GetNumParam(void);
extern const char *Scr_GetString(unsigned int index);
extern void Scr_Error(const char *msg);
extern void Scr_ObjectError(const char *msg);
extern void Scr_ParamError(unsigned int index, const char *msg);
extern void SV_SetConfigstring(int index, const char *val);
extern const char *SV_GetConfigstringConst(int index);
extern WeaponDef *BG_GetWeaponDef(int iWeapon);
extern int BG_GetNumWeapons(void);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);

/* Weapon names and camouflage ids between spaces, short enough for the
 * clients' copy of them. */
int G_ValidWeaponCamos(const char *text)
{
    size_t length = text ? strlen(text) : 0, i;

    if (!text || length >= WEAPON_CAMO_STRING_MAX)
        return 0;
    for (i = 0; i < length; i++) {
        char c = text[i];

        if (!(c == ' ' || c == '_' || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
            return 0;
    }
    return 1;
}

/* self setWeaponCamos("weapon camo weapon camo"): what every client paints
 * on the player's class weapons, until the next call; "" paints nothing. */
void PlayerCmd_SetWeaponCamos(scr_entref_t entref)
{
    const char *text;

    if (Scr_GetNumParam() != 1) {
        Scr_Error("USAGE: self setWeaponCamos(<\"weapon camo weapon camo\">)\n");
        return;
    }
    if (entref.classnum != 0 || entref.entnum >= WEAPON_CAMO_CLIENTS ||
        !((gentity_t *)((byte *)imp_g_entities + entref.entnum * sizeof(gentity_t)))->client) {
        Scr_ObjectError("not a player");
        return;
    }
    text = Scr_GetString(0);
    if (!G_ValidWeaponCamos(text)) {
        Scr_ParamError(0, "weapon names and camouflage ids between spaces");
        return;
    }
    SV_SetConfigstring(CS_WEAPON_CAMOS + entref.entnum, text);
}

/* The next token between spaces, or 0 at the end and for one too long. */
static int G_WeaponCamoToken(const char **text, char *out, int size)
{
    const char *start = *text;
    int length;

    while (*start == ' ')
        start++;
    length = (int)strcspn(start, " ");
    *text = start + length;
    if (!length || length >= size)
        return 0;
    memcpy(out, start, (size_t)length);
    out[length] = '\0';
    return 1;
}

/* The camouflage the player's configstring gives the weapon, or none. */
int G_GetClientWeaponCamo(int clientNum, int weapon)
{
    char name[32], id[32];
    const char *text;
    const WeaponDef *def;

    if (clientNum < 0 || clientNum >= WEAPON_CAMO_CLIENTS || weapon <= 0 || weapon > BG_GetNumWeapons())
        return WEAPON_CAMO_NONE;
    def = BG_GetWeaponDef(weapon);
    text = SV_GetConfigstringConst(CS_WEAPON_CAMOS + clientNum);
    while (G_WeaponCamoToken(&text, name, sizeof(name)) && G_WeaponCamoToken(&text, id, sizeof(id))) {
        if (def && def->szInternalName && !strcmp(name, def->szInternalName)) {
            int camo = BG_WeaponCamo_Find(id);

            return camo > WEAPON_CAMO_NONE ? camo : WEAPON_CAMO_NONE;
        }
    }
    return WEAPON_CAMO_NONE;
}

/* Rewrites the player's configstring for the guns now in their slots: weapon
 * in camo, the others as it painted them. */
void G_SetClientSlotCamos(int clientNum, const playerState_t *ps, int weapon, int camo)
{
    char text[WEAPON_CAMO_STRING_MAX];
    int length = 0, slot;

    if (clientNum < 0 || clientNum >= WEAPON_CAMO_CLIENTS)
        return;
    text[0] = '\0';
    for (slot = 1; slot <= 2; slot++) {
        int held = (unsigned char)ps->weaponslots[slot];
        int paint;

        if (held <= 0 || held > BG_GetNumWeapons())
            continue;
        paint = held == weapon ? camo : G_GetClientWeaponCamo(clientNum, held);
        if (paint <= WEAPON_CAMO_NONE || paint >= WEAPON_CAMO_COUNT)
            continue;
        length += Com_sprintf(text + length, (int)sizeof(text) - length, "%s%s %s", length ? " " : "",
                              BG_GetWeaponDef(held)->szInternalName, g_weaponCamos[paint].id);
        if (length >= (int)sizeof(text))
            return;
    }
    if (strcmp(text, SV_GetConfigstringConst(CS_WEAPON_CAMOS + clientNum)))
        SV_SetConfigstring(CS_WEAPON_CAMOS + clientNum, text);
}
