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
