#ifndef UI_CLASS_DATA_H
#define UI_CLASS_DATA_H

#include "headers/PC/bgame/bg_attachment_rules.h"

/* Five custom classes: a name, one weapon for each slot, and a camouflage and
 * attachments for each gun. The same lists and defaults are in
 * downstream/gateway/classes.json and the server's browser_classes.gsc. */
#define CLASS_COUNT 5
#define CLASS_NAME_MAX 20
/* Up to three on the primary and one on the secondary, joined by '+'. */
#define CLASS_ATTACHMENTS_MAX 48

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
    /* The primary's and the secondary's attachments in slot order, then -1. */
    int attachment[CLASS_SLOT_SECONDARY + 1][ATT_PRIMARY_MAX];
} ClassLoadout;

typedef struct {
    int valid;
    float value[CLASS_STAT_COUNT];
    int clip;
    int ammo;
    /* A grenade's fuseTime, in seconds. */
    float fuse;
    /* The file's moveSpeedScale. */
    float moveSpeed;
    /* The values attachments change, as the engine reads them. */
    AttachmentStats raw;
} ClassStats;

extern const ClassWeapon g_classWeapons[];
extern const int g_classWeaponCount;

int ClassData_Find(const char *weapon);
/* The slot's weapons as table indexes, in table order; the count. */
int ClassData_SlotWeapons(int slot, int *indexes, int max);
void ClassData_Defaults(ClassLoadout *classes);
/* Storage lines, "name\tprimary\tsecondary\tlethal\ttactical\tprimaryCamo\t
 * secondaryCamo\tprimary attachments\tsecondary attachments\n" for each class,
 * attachments joined by '+'. Lines from before attachments end after the
 * camouflage, lines from before camouflage after the tactical. A missing or
 * invalid field takes its slot's default, invalid attachments none. 1 when
 * every class and field was valid. */
int ClassData_Parse(const char *text, ClassLoadout *classes);
/* The same lines; their length, or -1 when they do not fit. */
int ClassData_Serialize(const ClassLoadout *classes, char *out, int size);
int ClassData_ValidName(const char *name);
/* A typed character (with the UI's 0x400 flag) or Backspace for a name being
 * edited; 1 when the key was taken. */
int ClassData_EditName(char *name, int key);
/* "class,primary,secondary,lethal,tactical", the class menu's response, then
 * ",primaryCamo,secondaryCamo,primary attachments,secondary attachments" when a
 * gun is camouflaged or has attachments, "none" for no attachments: a server
 * built before either still takes a plain class. */
int ClassData_Response(const ClassLoadout *loadout, char *out, int size);

/* How many attachments the primary and secondary slots hold; 0 for others. */
int ClassData_AttachmentMax(int slot);
int ClassData_AttachmentCount(const ClassLoadout *loadout, int slot);
unsigned ClassData_AttachmentMask(const ClassLoadout *loadout, int slot);
/* A new weapon in a slot starts without attachments. */
void ClassData_SetWeapon(ClassLoadout *loadout, int slot, int weapon);
/* Puts the attachment at a position, replacing the one there or after the
 * last; 0, changing nothing, when the weapon does not take it, or another
 * position holds it or another optic. */
int ClassData_EquipAttachment(ClassLoadout *loadout, int slot, int position, int attachment);
/* Takes out the attachment at a position; the later ones move up. */
void ClassData_RemoveAttachment(ClassLoadout *loadout, int slot, int position);

/* A weapon file's values behind the stat bars. */
void ClassData_ReadStats(const char *weaponFile, ClassStats *stats);
/* The bars and values of a gun with the attachments. */
void ClassData_AttachedStats(const ClassStats *base, const char *weapon, unsigned mask, ClassStats *attached);
/* Lines naming what changed from base to attached, each with its tone: 1
 * better, -1 worse, 0 neither. The count written. */
int ClassData_AttachmentEffects(const AttachmentStats *base, const AttachmentStats *attached, char (*lines)[48],
                                int *tones, int max);
/* A bar's rating before rounding: 2 to 8 between the lowest and highest value
 * of all the guns, moved about a point for each tenth the attachments change
 * the value, within 0 to 10. */
float ClassData_AttachedRating(float bare, float attached, float lowest, float highest);

#endif
