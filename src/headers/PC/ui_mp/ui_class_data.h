#ifndef UI_CLASS_DATA_H
#define UI_CLASS_DATA_H

/* Five custom classes: a name, one weapon for each slot and a camouflage for
 * each gun. The same lists and defaults are in downstream/gateway/classes.json
 * and the server's browser_classes.gsc. */
#define CLASS_COUNT 5
#define CLASS_NAME_MAX 20

enum { CLASS_SLOT_PRIMARY, CLASS_SLOT_SECONDARY, CLASS_SLOT_LETHAL, CLASS_SLOT_TACTICAL, CLASS_SLOT_COUNT };
/* The guns' slots, which take a camouflage. */
#define CLASS_CAMO_SLOTS 2
enum {
    CLASS_GROUP_SMG, CLASS_GROUP_RIFLE, CLASS_GROUP_AUTOMATIC, CLASS_GROUP_SNIPER, CLASS_GROUP_SHOTGUN,
    CLASS_GROUP_PISTOL, CLASS_GROUP_FRAG, CLASS_GROUP_SMOKE, CLASS_GROUP_MINE, CLASS_GROUP_CHARGE
};
/* The browser's own explosives belong to no army. */
enum { CLASS_NATION_US, CLASS_NATION_UK, CLASS_NATION_USSR, CLASS_NATION_DE, CLASS_NATION_NONE };
enum { CLASS_STAT_DAMAGE, CLASS_STAT_RANGE, CLASS_STAT_ACCURACY, CLASS_STAT_FIRE_RATE, CLASS_STAT_MOBILITY,
       CLASS_STAT_COUNT };

typedef struct {
    const char *weapon;
    int slot;
    int group;
    int nation;
    /* Localized references, or NULL. */
    const char *displayName;
    const char *description;
    /* The retail weapon menu's image, or the weapon's HUD icon. */
    const char *image;
} ClassWeapon;

typedef struct {
    char name[CLASS_NAME_MAX + 1];
    /* Indexes into g_classWeapons. */
    int weapon[CLASS_SLOT_COUNT];
    /* Indexes into g_weaponCamos, for the primary and the secondary. */
    int camo[CLASS_CAMO_SLOTS];
} ClassLoadout;

typedef struct {
    int valid;
    float value[CLASS_STAT_COUNT];
    int clip;
    int ammo;
    /* A grenade's fuseTime, in seconds. */
    float fuse;
} ClassStats;

extern const ClassWeapon g_classWeapons[];
extern const int g_classWeaponCount;

int ClassData_Find(const char *weapon);
/* The slot's weapons as table indexes, in table order; the count. */
int ClassData_SlotWeapons(int slot, int *indexes, int max);
void ClassData_Defaults(ClassLoadout *classes);
/* Storage lines, "name\tprimary\tsecondary\tlethal\ttactical\tprimaryCamo\t
 * secondaryCamo\n" for each class. A missing or invalid field takes its
 * slot's default. 1 when every class and field was valid. */
int ClassData_Parse(const char *text, ClassLoadout *classes);
/* The same lines; their length, or -1 when they do not fit. */
int ClassData_Serialize(const ClassLoadout *classes, char *out, int size);
int ClassData_ValidName(const char *name);
/* A typed character (with the UI's 0x400 flag) or Backspace for a name being
 * edited; 1 when the key was taken. */
int ClassData_EditName(char *name, int key);
/* "class,primary,secondary,lethal,tactical", the class menu's response, then
 * ",primaryCamo,secondaryCamo" when a gun is camouflaged: a server built
 * before camouflage still takes an unpainted class. */
int ClassData_Response(const ClassLoadout *loadout, char *out, int size);
/* A weapon file's values behind the stat bars. */
void ClassData_ReadStats(const char *weaponFile, ClassStats *stats);
/* 1 to 10 between the lowest and highest value of all the guns. */
int ClassData_Rate(float value, float lowest, float highest);

#endif
