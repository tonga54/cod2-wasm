#include "common_types.h"
#include "headers/PC/bgame/bg_attachments.h"
#include "headers/PC/bgame/bg_weapon_camo.h"
#include <string.h>

extern int bg_iNumWeapons;
extern WeaponDef *bg_weaponDefs[128];
extern int BG_GetAmmoClipSize(int iClipIndex);
extern int BG_GetAmmoTypeMax(int iAmmoIndex);

/* Distinct weapons and attachment sets in use at once; a set evicted while
 * a player still holds it is built again on its next use. */
#define ATTACHED_DEF_COUNT 128
#define ATTACHED_DEF_HASH 64

typedef struct {
    int weapon;
    unsigned mask;
    unsigned lastUse;
    WeaponDef def;
} AttachedWeaponDef;

static AttachedWeaponDef s_attachedDefs[ATTACHED_DEF_COUNT];
static int s_attachedDefCount;
static unsigned s_attachedDefClock;
/* An entry's index plus one by its key, checked against the entry. */
static unsigned char s_attachedDefHash[ATTACHED_DEF_HASH];

unsigned BG_GetSlotAttachments(const playerState_t *ps, int slot)
{
    if (slot != 1 && slot != 2)
        return 0;
    return (unsigned char)ps->weaponslots[2 + 2 * slot] | (unsigned)(unsigned char)ps->weaponslots[3 + 2 * slot] << 8;
}

void BG_SetSlotAttachments(playerState_t *ps, int slot, unsigned mask)
{
    if (slot != 1 && slot != 2)
        return;
    ps->weaponslots[2 + 2 * slot] = (char)(mask & 0xff);
    ps->weaponslots[3 + 2 * slot] = (char)(mask >> 8 & 0xff);
}

unsigned BG_GetWeaponAttachments(const playerState_t *ps, int weapon)
{
    if (weapon <= 0)
        return 0;
    if ((unsigned char)ps->weaponslots[1] == weapon)
        return BG_GetSlotAttachments(ps, 1);
    if ((unsigned char)ps->weaponslots[2] == weapon)
        return BG_GetSlotAttachments(ps, 2);
    return 0;
}

static void BG_AttachmentStatsFromDef(const WeaponDef *def, AttachmentStats *stats)
{
    memset(stats, 0, sizeof(*stats));
    stats->clipSize = def->iClipSize;
    stats->maxAmmo = def->iMaxAmmo;
    stats->startAmmo = def->iStartAmmo;
    stats->fireTime = def->iFireTime;
    stats->rechamberTime = def->iRechamberTime;
    stats->rechamberBoltTime = def->iRechamberBoltTime;
    stats->adsTransInTime = def->iAdsTransInTime;
    stats->adsZoomFov = def->fAdsZoomFov;
    stats->adsZoomInFrac = def->fAdsZoomInFrac;
    stats->adsZoomOutFrac = def->fAdsZoomOutFrac;
    stats->adsSpread = def->fAdsSpread;
    stats->adsIdleAmount = def->fAdsIdleAmount;
    stats->adsSway[0] = def->adsSwayMaxAngle;
    stats->adsSway[1] = def->adsSwayPitchScale;
    stats->adsSway[2] = def->adsSwayYawScale;
    stats->adsSway[3] = def->adsSwayHorizScale;
    stats->adsSway[4] = def->adsSwayVertScale;
    stats->hipSpread[0] = def->fHipSpreadStandMin;
    stats->hipSpread[1] = def->fHipSpreadDuckedMin;
    stats->hipSpread[2] = def->fHipSpreadProneMin;
    stats->hipSpread[3] = def->hipSpreadStandMax;
    stats->hipSpread[4] = def->hipSpreadDuckedMax;
    stats->hipSpread[5] = def->hipSpreadProneMax;
    stats->adsKick[0] = def->fAdsViewKickPitchMin;
    stats->adsKick[1] = def->fAdsViewKickPitchMax;
    stats->adsKick[2] = def->fAdsViewKickYawMin;
    stats->adsKick[3] = def->fAdsViewKickYawMax;
    stats->adsKick[4] = def->fAdsGunKickPitchMin;
    stats->adsKick[5] = def->fAdsGunKickPitchMax;
    stats->adsKick[6] = def->fAdsGunKickYawMin;
    stats->adsKick[7] = def->fAdsGunKickYawMax;
    stats->maxDamageRange = def->fMaxDamageRange;
    stats->minDamageRange = def->fMinDamageRange;
    stats->headMultiplier[0] = def->locationDamageMultipliers[HITLOC_HELMET];
    stats->headMultiplier[1] = def->locationDamageMultipliers[HITLOC_HEAD];
    stats->damage = def->damage;
    stats->semiAuto = def->bSemiAuto;
    stats->boltAction = def->bBoltAction;
    stats->armorPiercing = def->armorPiercing;
    stats->suppressed = 0;
    stats->overlay = def->szOverlayMaterial;
    stats->overlayReticle = (int)def->overlayReticle;
    stats->overlayWidth = def->overlayWidth;
    stats->overlayHeight = def->overlayHeight;
}

static void BG_AttachmentStatsToDef(const AttachmentStats *stats, WeaponDef *def)
{
    def->iClipSize = stats->clipSize;
    def->iMaxAmmo = stats->maxAmmo;
    def->iStartAmmo = stats->startAmmo;
    def->iFireTime = stats->fireTime;
    def->iRechamberTime = stats->rechamberTime;
    def->iRechamberBoltTime = stats->rechamberBoltTime;
    def->iAdsTransInTime = stats->adsTransInTime;
    def->fAdsZoomFov = stats->adsZoomFov;
    def->fAdsZoomInFrac = stats->adsZoomInFrac;
    def->fAdsZoomOutFrac = stats->adsZoomOutFrac;
    def->fAdsSpread = stats->adsSpread;
    def->fAdsIdleAmount = stats->adsIdleAmount;
    def->adsSwayMaxAngle = stats->adsSway[0];
    def->adsSwayPitchScale = stats->adsSway[1];
    def->adsSwayYawScale = stats->adsSway[2];
    def->adsSwayHorizScale = stats->adsSway[3];
    def->adsSwayVertScale = stats->adsSway[4];
    def->fHipSpreadStandMin = stats->hipSpread[0];
    def->fHipSpreadDuckedMin = stats->hipSpread[1];
    def->fHipSpreadProneMin = stats->hipSpread[2];
    def->hipSpreadStandMax = stats->hipSpread[3];
    def->hipSpreadDuckedMax = stats->hipSpread[4];
    def->hipSpreadProneMax = stats->hipSpread[5];
    def->fAdsViewKickPitchMin = stats->adsKick[0];
    def->fAdsViewKickPitchMax = stats->adsKick[1];
    def->fAdsViewKickYawMin = stats->adsKick[2];
    def->fAdsViewKickYawMax = stats->adsKick[3];
    def->fAdsGunKickPitchMin = stats->adsKick[4];
    def->fAdsGunKickPitchMax = stats->adsKick[5];
    def->fAdsGunKickYawMin = stats->adsKick[6];
    def->fAdsGunKickYawMax = stats->adsKick[7];
    def->fMaxDamageRange = stats->maxDamageRange;
    def->fMinDamageRange = stats->minDamageRange;
    def->locationDamageMultipliers[HITLOC_HELMET] = stats->headMultiplier[0];
    def->locationDamageMultipliers[HITLOC_HEAD] = stats->headMultiplier[1];
    def->armorPiercing = stats->armorPiercing;
    def->szOverlayMaterial = (char *)stats->overlay;
    def->overlayReticle = (weapOverlayReticle_t)stats->overlayReticle;
    def->overlayWidth = stats->overlayWidth;
    def->overlayHeight = stats->overlayHeight;
}

void BG_ApplyAttachments(WeaponDef *def, unsigned mask)
{
    AttachmentStats stats;

    BG_AttachmentStatsFromDef(def, &stats);
    BG_ApplyAttachmentStats(def->szInternalName, mask, &stats);
    BG_AttachmentStatsToDef(&stats, def);
    /* The loader's reciprocal that advances the aim-in pose. */
    if (def->iAdsTransInTime > 0)
        def->fOOPosAnimLength[0] = 1.0f / (float)def->iAdsTransInTime;
}

void BG_ResetAttachedWeaponDefs(void)
{
    s_attachedDefCount = 0;
    s_attachedDefClock = 0;
    memset(s_attachedDefHash, 0, sizeof(s_attachedDefHash));
}

WeaponDef *BG_GetAttachedWeaponDef(int weapon, unsigned mask)
{
    unsigned key = ((unsigned)weapon * 0x9e3779b1u ^ mask * 0x85ebca6bu) >> 26;
    AttachedWeaponDef *entry = NULL;
    int index = s_attachedDefHash[key] - 1, i;

    if (index >= 0 && s_attachedDefs[index].weapon == weapon && s_attachedDefs[index].mask == mask) {
        entry = &s_attachedDefs[index];
    } else {
        for (i = 0; i < s_attachedDefCount; i++) {
            if (s_attachedDefs[i].weapon == weapon && s_attachedDefs[i].mask == mask)
                break;
        }
        if (i == s_attachedDefCount) {
            if (s_attachedDefCount < ATTACHED_DEF_COUNT) {
                s_attachedDefCount++;
            } else {
                for (i = 0, index = 1; index < ATTACHED_DEF_COUNT; index++) {
                    if (s_attachedDefs[index].lastUse < s_attachedDefs[i].lastUse)
                        i = index;
                }
            }
            s_attachedDefs[i].weapon = weapon;
            s_attachedDefs[i].mask = mask;
            s_attachedDefs[i].def = *bg_weaponDefs[weapon];
            BG_ApplyAttachments(&s_attachedDefs[i].def, mask);
        }
        entry = &s_attachedDefs[i];
        s_attachedDefHash[key] = (unsigned char)(i + 1);
    }
    entry->lastUse = ++s_attachedDefClock;
    return &entry->def;
}

WeaponDef *BG_GetPlayerWeaponDef(const playerState_t *ps, int weapon)
{
    unsigned mask = ps ? BG_GetWeaponAttachments(ps, weapon) : 0;

    if (!mask || weapon > bg_iNumWeapons)
        return bg_weaponDefs[weapon];
    return BG_GetAttachedWeaponDef(weapon, mask);
}

int BG_GetPlayerClipSize(const playerState_t *ps, int weapon)
{
    unsigned mask = ps ? BG_GetWeaponAttachments(ps, weapon) : 0;

    if (!mask || weapon > bg_iNumWeapons)
        return BG_GetAmmoClipSize(bg_weaponDefs[weapon]->iClipIndex);
    return BG_GetAttachedWeaponDef(weapon, mask)->iClipSize;
}

int BG_GetPlayerMaxAmmo(const playerState_t *ps, int weapon)
{
    unsigned mask = ps ? BG_GetWeaponAttachments(ps, weapon) : 0;

    if (!mask || weapon > bg_iNumWeapons)
        return BG_GetAmmoTypeMax(bg_weaponDefs[weapon]->iAmmoIndex);
    return BG_GetAttachedWeaponDef(weapon, mask)->iMaxAmmo;
}

/* legsAnim and torsoAnim travel in 10 bits each. */
typedef char BG_ItemAttachmentsFit[ATT_COUNT <= 16 ? 1 : -1];
typedef char BG_ItemCamoFits[WEAPON_CAMO_COUNT <= 16 ? 1 : -1];

void BG_SetItemWeaponState(entityState_t *es, int camo, unsigned attachments)
{
    es->legsAnim = (int)(attachments & 0x3ff);
    es->torsoAnim = (int)((attachments >> 10 & 0x3f) | (unsigned)(camo & 0xf) << 6);
}

int BG_GetItemWeaponCamo(const entityState_t *es)
{
    return (int)((unsigned)es->torsoAnim >> 6 & 0xf);
}

unsigned BG_GetItemWeaponAttachments(const entityState_t *es)
{
    return ((unsigned)es->legsAnim & 0x3ff) | ((unsigned)es->torsoAnim & 0x3f) << 10;
}
