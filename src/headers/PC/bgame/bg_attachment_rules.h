#ifndef BG_ATTACHMENT_RULES_H
#define BG_ATTACHMENT_RULES_H

/* Call of Duty: WWII's weapon attachments, fitted to CoD2's weapons: which
 * weapon takes which, how many a slot holds, and what each one changes. The
 * same lists and limits are in downstream/gateway/classes.json. Nothing here
 * depends on the engine, so the menus, the game and the tests share it. */

enum {
    ATT_REFLEX,
    ATT_LENS,
    ATT_OPTIC4X,
    ATT_QUICKDRAW,
    ATT_GRIP,
    ATT_STEADYAIM,
    ATT_CALIBRATION,
    ATT_RAPIDFIRE,
    ATT_EXTMAG,
    ATT_FMJ,
    ATT_HIGHCAL,
    ATT_RIFLING,
    ATT_SUPPRESSOR,
    ATT_COUNT
};
#define ATT_BIT(attachment) (1u << (attachment))
/* A weapon carries one optic at a time. */
#define ATT_OPTICS (ATT_BIT(ATT_REFLEX) | ATT_BIT(ATT_LENS) | ATT_BIT(ATT_OPTIC4X))
/* WWII after its April 2018 update: three on the primary, one on the pistol. */
#define ATT_PRIMARY_MAX 3
#define ATT_SECONDARY_MAX 1

enum {
    ATT_KIND_SMG,
    /* The Thompson and PPSh, whose Steady Aim WWII made weaker. */
    ATT_KIND_FAST_SMG,
    /* Semi-automatic rifles and the MP44, WWII's rifle class. */
    ATT_KIND_RIFLE,
    ATT_KIND_BAR,
    ATT_KIND_LMG,
    /* Bolt-action rifles on their iron sights. */
    ATT_KIND_BOLT,
    ATT_KIND_SNIPER,
    ATT_KIND_SHOTGUN,
    ATT_KIND_PISTOL
};

enum { ATT_NATION_US, ATT_NATION_UK, ATT_NATION_USSR, ATT_NATION_DE };

typedef struct {
    const char *name;
    const char *displayName;
    const char *description;
} AttachmentInfo;

typedef struct {
    const char *weapon;
    int kind;
    int nation;
    unsigned allowed;
} AttachmentWeapon;

/* The weapon file values attachments change, in the engine's units. */
typedef struct {
    int clipSize, maxAmmo, startAmmo;
    /* Milliseconds. */
    int fireTime, rechamberTime, rechamberBoltTime, adsTransInTime;
    float adsZoomFov, adsZoomInFrac, adsZoomOutFrac;
    float adsSpread, adsIdleAmount;
    /* Maximum angle, then pitch, yaw, horizontal and vertical scales. */
    float adsSway[5];
    /* Standing, ducked and prone minimums, then the same maximums. */
    float hipSpread[6];
    /* View kick pitch and yaw minimum and maximum, then the gun's. */
    float adsKick[8];
    float maxDamageRange, minDamageRange;
    /* Helmet and head multipliers. */
    float headMultiplier[2];
    /* The weapon's own damage and how it fires, which attachments never
     * change: High Caliber leaves an automatic weapon's headshot short of a
     * kill. */
    int damage, semiAuto, boltAction;
    int armorPiercing;
    int suppressed;
    /* The scope material, or "". */
    const char *overlay;
    int overlayReticle;
    float overlayWidth, overlayHeight;
} AttachmentStats;

extern const AttachmentInfo g_attachments[ATT_COUNT];
extern const AttachmentWeapon g_attachmentWeapons[];
extern const int g_attachmentWeaponCount;

int BG_AttachmentFind(const char *name);
/* The weapon's entry, matched without case as the engine names weapons. */
const AttachmentWeapon *BG_AttachmentWeapon(const char *weapon);
unsigned BG_AttachmentsAllowed(const char *weapon);
int BG_AttachmentCount(unsigned mask);
/* 1 when the weapon takes every attachment of the set, at most max of them
 * and one optic. */
int BG_AttachmentsValid(const char *weapon, unsigned mask, int max);
/* What keeps candidate off a weapon whose other slots hold others: the
 * attachment itself when it is already there, the other optic, or -1. */
int BG_AttachmentConflict(unsigned others, int candidate);
/* "", "none" or names joined by '+', in slot order: the number of
 * attachments, written to ids, or -1 when a name is unknown or repeated or the
 * set is not valid for the weapon with at most max. */
int BG_AttachmentsParseList(const char *weapon, const char *list, int max, int *ids);
/* The mask of a list as above, or -1. */
int BG_AttachmentsParse(const char *weapon, const char *list, int max);
/* The names joined by '+', "" for none; the length, or -1 when it does not
 * fit or an id is not an attachment. */
int BG_AttachmentsFormatList(const int *ids, int count, char *out, int size);
/* Applies the weapon's attachments of the set to its values. */
void BG_ApplyAttachmentStats(const char *weapon, unsigned mask, AttachmentStats *stats);

#endif
