#include "headers/PC/bgame/bg_attachment_rules.h"
#include <stddef.h>
#include <string.h>

/* The in-game descriptions, with the minimap for WWII's radar. */
const AttachmentInfo g_attachments[ATT_COUNT] = {
    { "reflex", "Reflex Sight", "Basic reflex sight with clear view." },
    { "lens", "Lens Sight", "Lens sight scope." },
    { "optic4x", "4x Optic", "4x enhanced zoom." },
    { "quickdraw", "Quickdraw", "Aim down sight faster." },
    { "grip", "Grip", "Reduces weapon recoil while aiming down sight." },
    { "steadyaim", "Steady Aim", "Better accuracy when firing from the hip." },
    { "calibration", "Ballistic Calibration", "Reduces sway." },
    { "rapidfire", "Rapid Fire", "Increases weapon fire rate." },
    { "extmag", "Extended Mag", "Increases your weapon's magazine size." },
    { "fmj", "Full Metal Jacket", "Increases damage through surfaces." },
    { "highcal", "High Caliber", "Increases head shot damage." },
    { "rifling", "Advanced Rifling", "Increases damage falloff range." },
    { "suppressor", "Suppressor",
      "Reduces weapon sound and you remain hidden from the minimap when firing." },
};

#define A(name) ATT_BIT(ATT_##name)
/* WWII's lists by weapon class. Bolt-action rifles are its snipers on the
 * Iron Sight attachment: CoD2 already has them as separate weapons. */
#define SMG_ATTACHMENTS                                                                                           \
    (A(REFLEX) | A(LENS) | A(QUICKDRAW) | A(GRIP) | A(STEADYAIM) | A(FMJ) | A(RIFLING) | A(EXTMAG) |             \
     A(RAPIDFIRE) | A(SUPPRESSOR))
#define RIFLE_ATTACHMENTS                                                                                         \
    (A(LENS) | A(QUICKDRAW) | A(GRIP) | A(REFLEX) | A(HIGHCAL) | A(STEADYAIM) | A(FMJ) | A(OPTIC4X) |           \
     A(RAPIDFIRE) | A(EXTMAG) | A(RIFLING))
#define LMG_ATTACHMENTS                                                                                           \
    (A(QUICKDRAW) | A(GRIP) | A(REFLEX) | A(STEADYAIM) | A(FMJ) | A(OPTIC4X) | A(EXTMAG) | A(RAPIDFIRE) |      \
     A(HIGHCAL) | A(RIFLING))
#define SNIPER_ATTACHMENTS (A(OPTIC4X) | A(CALIBRATION) | A(FMJ) | A(EXTMAG) | A(RAPIDFIRE))
#define SHOTGUN_ATTACHMENTS (A(REFLEX) | A(QUICKDRAW) | A(STEADYAIM) | A(EXTMAG) | A(RIFLING) | A(RAPIDFIRE))
#define PISTOL_ATTACHMENTS                                                                                        \
    (A(QUICKDRAW) | A(STEADYAIM) | A(HIGHCAL) | A(FMJ) | A(EXTMAG) | A(RIFLING) | A(SUPPRESSOR))

const AttachmentWeapon g_attachmentWeapons[] = {
    { "thompson_mp", ATT_KIND_FAST_SMG, ATT_NATION_US, SMG_ATTACHMENTS },
    { "greasegun_mp", ATT_KIND_SMG, ATT_NATION_US, SMG_ATTACHMENTS },
    { "sten_mp", ATT_KIND_SMG, ATT_NATION_UK, SMG_ATTACHMENTS },
    { "PPS42_mp", ATT_KIND_SMG, ATT_NATION_USSR, SMG_ATTACHMENTS },
    { "ppsh_mp", ATT_KIND_FAST_SMG, ATT_NATION_USSR, SMG_ATTACHMENTS },
    { "mp40_mp", ATT_KIND_SMG, ATT_NATION_DE, SMG_ATTACHMENTS },
    { "m1garand_mp", ATT_KIND_RIFLE, ATT_NATION_US, RIFLE_ATTACHMENTS },
    { "m1carbine_mp", ATT_KIND_RIFLE, ATT_NATION_US, RIFLE_ATTACHMENTS },
    { "enfield_mp", ATT_KIND_BOLT, ATT_NATION_UK, SNIPER_ATTACHMENTS },
    { "mosin_nagant_mp", ATT_KIND_BOLT, ATT_NATION_USSR, SNIPER_ATTACHMENTS },
    { "SVT40_mp", ATT_KIND_RIFLE, ATT_NATION_USSR, RIFLE_ATTACHMENTS },
    { "kar98k_mp", ATT_KIND_BOLT, ATT_NATION_DE, SNIPER_ATTACHMENTS },
    { "g43_mp", ATT_KIND_RIFLE, ATT_NATION_DE, RIFLE_ATTACHMENTS },
    { "bar_mp", ATT_KIND_BAR, ATT_NATION_US, RIFLE_ATTACHMENTS },
    { "bren_mp", ATT_KIND_LMG, ATT_NATION_UK, LMG_ATTACHMENTS },
    { "mp44_mp", ATT_KIND_RIFLE, ATT_NATION_DE, RIFLE_ATTACHMENTS },
    { "springfield_mp", ATT_KIND_SNIPER, ATT_NATION_US, SNIPER_ATTACHMENTS },
    { "enfield_scope_mp", ATT_KIND_SNIPER, ATT_NATION_UK, SNIPER_ATTACHMENTS },
    { "mosin_nagant_sniper_mp", ATT_KIND_SNIPER, ATT_NATION_USSR, SNIPER_ATTACHMENTS },
    { "kar98k_sniper_mp", ATT_KIND_SNIPER, ATT_NATION_DE, SNIPER_ATTACHMENTS },
    { "shotgun_mp", ATT_KIND_SHOTGUN, ATT_NATION_US, SHOTGUN_ATTACHMENTS },
    { "colt_mp", ATT_KIND_PISTOL, ATT_NATION_US, PISTOL_ATTACHMENTS },
    /* A revolver has no magazine to extend. */
    { "webley_mp", ATT_KIND_PISTOL, ATT_NATION_UK, PISTOL_ATTACHMENTS & ~A(EXTMAG) },
    { "TT30_mp", ATT_KIND_PISTOL, ATT_NATION_USSR, PISTOL_ATTACHMENTS },
    { "luger_mp", ATT_KIND_PISTOL, ATT_NATION_DE, PISTOL_ATTACHMENTS },
};
const int g_attachmentWeaponCount = (int)(sizeof(g_attachmentWeapons) / sizeof(g_attachmentWeapons[0]));
#undef A

/* The sniper rifles' scopes; the reticle numbers are the weapon files'
 * adsOverlayReticle names, FG42 and Springfield. */
static const struct {
    const char *material;
    int reticle;
} s_attachmentScopes[] = {
    { "scope_overlay_american", 3 },
    { "scope_overlay_britain", 2 },
    { "scope_overlay_russian", 2 },
    { "scope_overlay_german", 2 },
};

static int BG_AttachmentNameEqual(const char *a, const char *b)
{
    for (;; a++, b++) {
        int ca = *a >= 'A' && *a <= 'Z' ? *a + 32 : *a, cb = *b >= 'A' && *b <= 'Z' ? *b + 32 : *b;

        if (ca != cb)
            return 0;
        if (!ca)
            return 1;
    }
}

int BG_AttachmentFind(const char *name)
{
    int i;

    for (i = 0; name && i < ATT_COUNT; i++) {
        if (!strcmp(g_attachments[i].name, name))
            return i;
    }
    return -1;
}

const AttachmentWeapon *BG_AttachmentWeapon(const char *weapon)
{
    int i;

    for (i = 0; weapon && i < g_attachmentWeaponCount; i++) {
        if (BG_AttachmentNameEqual(g_attachmentWeapons[i].weapon, weapon))
            return &g_attachmentWeapons[i];
    }
    return NULL;
}

unsigned BG_AttachmentsAllowed(const char *weapon)
{
    const AttachmentWeapon *entry = BG_AttachmentWeapon(weapon);

    return entry ? entry->allowed : 0;
}

int BG_AttachmentCount(unsigned mask)
{
    int count = 0;

    for (; mask; mask &= mask - 1)
        count++;
    return count;
}

int BG_AttachmentsValid(const char *weapon, unsigned mask, int max)
{
    return !(mask & ~BG_AttachmentsAllowed(weapon)) && BG_AttachmentCount(mask) <= max &&
           BG_AttachmentCount(mask & ATT_OPTICS) <= 1;
}

int BG_AttachmentConflict(unsigned others, int candidate)
{
    int i;

    if (candidate < 0 || candidate >= ATT_COUNT)
        return -1;
    if (others & ATT_BIT(candidate))
        return candidate;
    if (ATT_BIT(candidate) & ATT_OPTICS) {
        for (i = 0; i < ATT_COUNT; i++) {
            if (others & ATT_OPTICS & ATT_BIT(i))
                return i;
        }
    }
    return -1;
}

int BG_AttachmentsParseList(const char *weapon, const char *list, int max, int *ids)
{
    unsigned mask = 0;
    int count = 0;

    if (!list || !*list || !strcmp(list, "none"))
        return 0;
    for (;;) {
        const char *end = strchr(list, '+');
        size_t length = end ? (size_t)(end - list) : strlen(list);
        char name[32];
        int id;

        if (!length || length >= sizeof(name) || count >= ATT_COUNT)
            return -1;
        memcpy(name, list, length);
        name[length] = '\0';
        id = BG_AttachmentFind(name);
        if (id < 0 || (mask & ATT_BIT(id)))
            return -1;
        mask |= ATT_BIT(id);
        ids[count++] = id;
        if (!end)
            break;
        list = end + 1;
    }
    return BG_AttachmentsValid(weapon, mask, max) ? count : -1;
}

int BG_AttachmentsParse(const char *weapon, const char *list, int max)
{
    int ids[ATT_COUNT], count = BG_AttachmentsParseList(weapon, list, max, ids), mask = 0, i;

    if (count < 0)
        return -1;
    for (i = 0; i < count; i++)
        mask |= (int)ATT_BIT(ids[i]);
    return mask;
}

int BG_AttachmentsFormatList(const int *ids, int count, char *out, int size)
{
    int length = 0, i;

    if (size <= 0)
        return -1;
    out[0] = '\0';
    for (i = 0; i < count; i++) {
        const char *name;
        int nameLength;

        if (ids[i] < 0 || ids[i] >= ATT_COUNT)
            return -1;
        name = g_attachments[ids[i]].name;
        nameLength = (int)strlen(name);
        if (length + (i ? 1 : 0) + nameLength >= size)
            return -1;
        if (i)
            out[length++] = '+';
        memcpy(out + length, name, (size_t)nameLength + 1);
        length += nameLength;
    }
    return length;
}

static int BG_AttachmentScaledTime(int time, float rate)
{
    return time > 0 ? (int)((float)time / rate + 0.5f) : time;
}

void BG_ApplyAttachmentStats(const char *weapon, unsigned mask, AttachmentStats *stats)
{
    const AttachmentWeapon *entry = BG_AttachmentWeapon(weapon);
    int kind, i;

    if (!entry)
        return;
    mask &= entry->allowed;
    kind = entry->kind;
    if (mask & ATT_BIT(ATT_REFLEX)) {
        stats->adsZoomFov *= 0.9f;
        stats->adsSpread *= 0.75f;
    }
    if (mask & ATT_BIT(ATT_LENS)) {
        stats->adsZoomFov *= 0.8f;
        stats->adsSpread *= 0.85f;
    }
    if (mask & ATT_BIT(ATT_OPTIC4X)) {
        /* Four times a 80-degree view; a sniper scope's 15 degrees zooms
         * further, so on a sniper rifle this is WWII's mid-range scope. */
        stats->adsZoomFov = 23.7f;
        if (!stats->overlay || !stats->overlay[0]) {
            stats->overlay = s_attachmentScopes[entry->nation].material;
            stats->overlayReticle = s_attachmentScopes[entry->nation].reticle;
            stats->overlayWidth = stats->overlayHeight = 480.0f;
            stats->adsZoomInFrac = 0.42f;
            stats->adsZoomOutFrac = 0.08f;
        }
    }
    if (mask & ATT_BIT(ATT_QUICKDRAW)) {
        /* Community measurements: 60% off an SMG's aim time, 20% off the
         * others'. */
        float scale = kind == ATT_KIND_SMG || kind == ATT_KIND_FAST_SMG ? 0.4f
                      : kind == ATT_KIND_PISTOL                         ? 0.6f
                                                                        : 0.8f;

        stats->adsTransInTime = (int)((float)stats->adsTransInTime * scale + 0.5f);
    }
    if (mask & ATT_BIT(ATT_GRIP)) {
        for (i = 0; i < 8; i++)
            stats->adsKick[i] *= 0.65f;
    }
    if (mask & ATT_BIT(ATT_STEADYAIM)) {
        float scale = kind == ATT_KIND_FAST_SMG ? 0.65f : 0.45f;

        for (i = 0; i < 6; i++)
            stats->hipSpread[i] *= scale;
    }
    if (mask & ATT_BIT(ATT_CALIBRATION)) {
        stats->adsIdleAmount *= 0.5f;
        for (i = 0; i < 5; i++)
            stats->adsSway[i] *= 0.5f;
    }
    if (mask & ATT_BIT(ATT_RAPIDFIRE)) {
        /* WWII's increases in rounds per minute. */
        float rate = kind == ATT_KIND_LMG                                 ? 1.0989f
                     : kind == ATT_KIND_SHOTGUN                           ? 1.42f
                     : kind == ATT_KIND_BOLT || kind == ATT_KIND_SNIPER ? 1.054f
                                                                          : 1.0638f;

        stats->fireTime = BG_AttachmentScaledTime(stats->fireTime, rate);
        stats->rechamberTime = BG_AttachmentScaledTime(stats->rechamberTime, rate);
        stats->rechamberBoltTime = BG_AttachmentScaledTime(stats->rechamberBoltTime, rate);
    }
    if (mask & ATT_BIT(ATT_EXTMAG)) {
        stats->clipSize = stats->clipSize * 3 / 2;
        stats->maxAmmo = stats->maxAmmo * 3 / 2;
        stats->startAmmo = stats->startAmmo * 3 / 2;
    }
    if (mask & ATT_BIT(ATT_FMJ))
        stats->armorPiercing = 1;
    if (mask & ATT_BIT(ATT_HIGHCAL)) {
        stats->headMultiplier[0] *= 1.5f;
        stats->headMultiplier[1] *= 1.5f;
    }
    if (mask & ATT_BIT(ATT_RIFLING)) {
        float scale = kind == ATT_KIND_RIFLE                                  ? 2.0f
                      : kind == ATT_KIND_SMG || kind == ATT_KIND_FAST_SMG     ? 1.3f
                      : kind == ATT_KIND_SHOTGUN                              ? 1.13f
                                                                              : 1.25f;

        stats->maxDamageRange *= scale;
        stats->minDamageRange *= scale;
    }
    if (mask & ATT_BIT(ATT_SUPPRESSOR)) {
        float scale = kind == ATT_KIND_PISTOL ? 0.85f : 0.7f;

        stats->maxDamageRange *= scale;
        stats->minDamageRange *= scale;
        stats->suppressed = 1;
    }
}
