#include "common_types.h"
#include "imports.h"
#include "headers/PC/bgame/bg_attachments.h"
#include "headers/PC/bgame/bg_weapon_camo.h"
#include "headers/PC/cgame_mp/cg_weaponcamo.h"
#include "headers/PC/gfx_d3d/r_weapon_camo.h"
#include <string.h>

extern const char *CL_GetConfigString(int index);
extern WeaponDef *BG_GetWeaponDef(int iWeapon);
extern int BG_GetNumWeapons(void);

#define CG_CAMO_WEAPON_NAME 32

/* A client's two painted weapons, kept by name: weapon indexes can change
 * with the map while the configstring keeps its text. */
typedef struct {
    char text[WEAPON_CAMO_STRING_MAX];
    char weapon[2][CG_CAMO_WEAPON_NAME];
    int camo[2];
} CgWeaponCamos;

static CgWeaponCamos s_cgWeaponCamos[WEAPON_CAMO_CLIENTS];

/* "weapon camo weapon camo"; a pair with an unknown or empty camouflage
 * paints nothing. */
static void CG_ParseWeaponCamos(const char *text, CgWeaponCamos *camos)
{
    char tokens[4][CG_CAMO_WEAPON_NAME];
    int count = 0, i;

    memset(camos, 0, sizeof(*camos));
    if (strlen(text) >= sizeof(camos->text))
        return;
    strcpy(camos->text, text);
    while (*text && count < 4) {
        size_t length;

        while (*text == ' ')
            text++;
        length = strcspn(text, " ");
        if (!length)
            break;
        if (length >= CG_CAMO_WEAPON_NAME)
            return;
        memcpy(tokens[count], text, length);
        tokens[count++][length] = '\0';
        text += length;
    }
    for (i = 0; i + 1 < count; i += 2) {
        int camo = BG_WeaponCamo_Find(tokens[i + 1]);

        if (camo > WEAPON_CAMO_NONE) {
            strcpy(camos->weapon[i / 2], tokens[i]);
            camos->camo[i / 2] = camo;
        }
    }
}

int CG_GetClientWeaponCamo(int clientNum, int weapon)
{
    CgWeaponCamos *camos;
    const WeaponDef *def;
    const char *text;
    int i;

    if (clientNum < 0 || clientNum >= WEAPON_CAMO_CLIENTS || weapon <= 0 || weapon > BG_GetNumWeapons())
        return 0;
    camos = &s_cgWeaponCamos[clientNum];
    text = CL_GetConfigString(CS_WEAPON_CAMOS + clientNum);
    if (!text)
        text = "";
    if (strcmp(camos->text, text))
        CG_ParseWeaponCamos(text, camos);
    def = BG_GetWeaponDef(weapon);
    for (i = 0; def && def->szInternalName && i < 2; i++) {
        if (camos->camo[i] && !strcmp(camos->weapon[i], def->szInternalName))
            return camos->camo[i];
    }
    return 0;
}

void CG_SetWeaponCamo(GfxEntity *sceneEnt, const struct DObj_s *obj, int clientNum, int weapon)
{
    int camo = CG_GetClientWeaponCamo(clientNum, weapon);
    const WeaponCamoGun *gun;

    if (!camo || !sceneEnt || !obj)
        return;
    gun = BG_WeaponCamo_Gun(BG_GetWeaponDef(weapon)->szInternalName);
    if (gun)
        R_SetEntityWeaponCamo(sceneEnt, obj, gun->material, camo, gun->style);
}

void CG_SetItemWeaponCamo(GfxEntity *sceneEnt, const struct DObj_s *obj, const entityState_t *es)
{
    int camo = BG_GetItemWeaponCamo(es), index = es->index.item;
    const gitem_t *item;
    const WeaponCamoGun *gun;

    if (camo <= WEAPON_CAMO_NONE || camo >= WEAPON_CAMO_COUNT || !sceneEnt || !obj || index <= 0 ||
        index >= *(int *)imp_bg_numItems)
        return;
    item = &((const gitem_t *)imp_bg_itemlist)[index];
    if (item->giType != IT_WEAPON || item->giTag <= 0 || item->giTag > BG_GetNumWeapons())
        return;
    gun = BG_WeaponCamo_Gun(BG_GetWeaponDef(item->giTag)->szInternalName);
    if (gun)
        R_SetEntityWeaponCamo(sceneEnt, obj, gun->material, camo, gun->style);
}
