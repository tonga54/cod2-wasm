#ifndef BG_WEAPON_CAMO_H
#define BG_WEAPON_CAMO_H

/* Camouflage a class paints on its primary and its pistol. Index 0 keeps the
 * gun's own finish. The ids are stored with the classes and sent to the
 * server: downstream/gateway/classes.json and browser_classes.gsc list them
 * in this order. */
#define WEAPON_CAMO_COUNT 11
#define WEAPON_CAMO_NONE 0
#define WEAPON_CAMO_ID_MAX 15

/* One configstring per client, "weapon camo weapon camo": the class's
 * primary and pistol with the ids of their camouflage. */
#define CS_WEAPON_CAMOS 0x7c0
#define WEAPON_CAMO_CLIENTS 64
#define WEAPON_CAMO_STRING_MAX 96

/* The part of a gun that takes the paint: its wooden furniture, as the WWII
 * rifles wear it, or the whole gun, for the metal ones. */
enum { WEAPON_CAMO_WOOD, WEAPON_CAMO_BODY };

typedef struct {
    const char *id;
    const char *name;
    const char *note;
} WeaponCamo;

typedef struct {
    const char *weapon;
    /* The one material of the gun's view and world models that is painted;
     * rounds, clips, scopes and cloth wraps keep theirs. */
    const char *material;
    int style;
} WeaponCamoGun;

extern const WeaponCamo g_weaponCamos[WEAPON_CAMO_COUNT];
extern const WeaponCamoGun g_weaponCamoGuns[];
extern const int g_weaponCamoGunCount;

/* The camouflage's index, or -1. */
int BG_WeaponCamo_Find(const char *id);
/* A class primary's or pistol's entry, or NULL for any other weapon. */
const WeaponCamoGun *BG_WeaponCamo_Gun(const char *weapon);
/* The camouflage's pattern as size * size texels in memory order B, G, R,
 * A, seamless when tiled. size is a power of two from 8 to 1024. 1 when
 * written. */
int BG_WeaponCamo_Generate(int camo, int size, unsigned char *bgra);

#endif
