#include "headers/PC/ui_mp/ui_class_data.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Every multiplayer weapon of the four factions. Primaries show their retail
 * weapon menu image; the others, their HUD icon. Descriptions are the retail
 * menus' where they wrote one. */
const ClassWeapon g_classWeapons[] = {
    { "thompson_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_SMG, CLASS_NATION_US, "WEAPON_THOMPSON",
      "MPUI_THE_THOMPSON_IS_DEADLY", "weapon_thompson" },
    { "greasegun_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_SMG, CLASS_NATION_US, "WEAPON_GREASEGUN", NULL,
      "weapon_greasegun" },
    { "sten_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_SMG, CLASS_NATION_UK, "WEAPON_STEN", "MPUI_THE_STEN_MARK_2_IS_A",
      "weapon_sten" },
    { "PPS42_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_SMG, CLASS_NATION_USSR, "WEAPON_PPS42", NULL, "weapon_pps42" },
    { "ppsh_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_SMG, CLASS_NATION_USSR, "WEAPON_PPSH", "MPUI_THE_PPSH41_SUBMACHINE",
      "weapon_ppsh" },
    { "mp40_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_SMG, CLASS_NATION_DE, "WEAPON_MP40", "MPUI_THE_MP40_SUBMACHINE_GUN",
      "weapon_mp40" },
    { "m1garand_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_RIFLE, CLASS_NATION_US, "WEAPON_M1GARAND",
      "MPUI_THE_M1_GARAND_IS_A_POWERFUL", "weapon_m1garand" },
    { "m1carbine_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_RIFLE, CLASS_NATION_US, "WEAPON_M1A1CARBINE",
      "MPUI_THE_M1A1_CARBINE_IS_A", "weapon_m1carbine" },
    { "enfield_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_RIFLE, CLASS_NATION_UK, "WEAPON_LEEENFIELD",
      "MPUI_THE_LEEENFIELD_RIFLE", "weapon_enfield" },
    { "mosin_nagant_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_RIFLE, CLASS_NATION_USSR, "WEAPON_MOSINNAGANT",
      "MPUI_THE_MOSINNAGANT_IS_A", "weapon_nagant" },
    { "SVT40_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_RIFLE, CLASS_NATION_USSR, "WEAPON_SVT40", NULL, "weapon_svt40" },
    { "kar98k_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_RIFLE, CLASS_NATION_DE, "WEAPON_KAR98K",
      "MPUI_THE_KAR98K_IS_THE_STANDARD", "weapon_kar98" },
    { "g43_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_RIFLE, CLASS_NATION_DE, "WEAPON_G43", NULL, "weapon_g43" },
    { "bar_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_AUTOMATIC, CLASS_NATION_US, "WEAPON_BAR", "MPUI_THE_BROWNING_AUTOMATIC",
      "weapon_bar" },
    { "bren_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_AUTOMATIC, CLASS_NATION_UK, "WEAPON_BREN",
      "MPUI_THE_BREN_IS_AN_EXCELLENT", "weapon_bren" },
    { "mp44_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_AUTOMATIC, CLASS_NATION_DE, "WEAPON_MP44", "MPUI_THE_MP44_IS_AN_ASSAULT",
      "weapon_mp44" },
    { "springfield_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_SNIPER, CLASS_NATION_US, "WEAPON_SPRINGFIELD",
      "MPUI_THE_SPRINGFIELD_M1903", "weapon_springfield" },
    { "enfield_scope_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_SNIPER, CLASS_NATION_UK, "WEAPON_SCOPEDLEEENFIELD",
      "MPUI_THIS_IS_A_SNIPER_RIFLE", "weapon_enfieldscoped" },
    { "mosin_nagant_sniper_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_SNIPER, CLASS_NATION_USSR, "WEAPON_SCOPEDMOSINNAGANT",
      "MPUI_THE_SCOPED_MOSINNAGANT", "weapon_nagantscoped" },
    { "kar98k_sniper_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_SNIPER, CLASS_NATION_DE, "WEAPON_SCOPEDKAR98K",
      "MPUI_THIS_KAR98K_VARIANT_USES", "weapon_kar98scoped" },
    { "shotgun_mp", CLASS_SLOT_PRIMARY, CLASS_GROUP_SHOTGUN, CLASS_NATION_US, "WEAPON_SHOTGUN", NULL, "weapon_shotgun" },
    { "colt_mp", CLASS_SLOT_SECONDARY, CLASS_GROUP_PISTOL, CLASS_NATION_US, "WEAPON_COLT45", NULL,
      "gfx/icons/hud@colt45.tga" },
    { "webley_mp", CLASS_SLOT_SECONDARY, CLASS_GROUP_PISTOL, CLASS_NATION_UK, "WEAPON_WEBLEY", NULL,
      "gfx/icons/hud@webley.tga" },
    { "TT30_mp", CLASS_SLOT_SECONDARY, CLASS_GROUP_PISTOL, CLASS_NATION_USSR, "WEAPON_TT30", NULL,
      "gfx/icons/hud@tt30.tga" },
    { "luger_mp", CLASS_SLOT_SECONDARY, CLASS_GROUP_PISTOL, CLASS_NATION_DE, "WEAPON_LUGER", NULL,
      "gfx/icons/hud@luger.tga" },
    { "frag_grenade_american_mp", CLASS_SLOT_LETHAL, CLASS_GROUP_FRAG, CLASS_NATION_US, "WEAPON_M2FRAGGRENADE", NULL,
      "gfx/icons/hud@us_grenade.tga" },
    { "frag_grenade_british_mp", CLASS_SLOT_LETHAL, CLASS_GROUP_FRAG, CLASS_NATION_UK, "WEAPON_MK1_FRAG_GRENADE", NULL,
      "gfx/icons/hud@british_grenade.tga" },
    { "frag_grenade_russian_mp", CLASS_SLOT_LETHAL, CLASS_GROUP_FRAG, CLASS_NATION_USSR, "WEAPON_RUSSIANGRENADE", NULL,
      "gfx/icons/hud@russian_grenade.tga" },
    { "frag_grenade_german_mp", CLASS_SLOT_LETHAL, CLASS_GROUP_FRAG, CLASS_NATION_DE, "WEAPON_GERMANGRENADE", NULL,
      "gfx/icons/hud@steilhandgrenate.tga" },
    { "smoke_grenade_american_mp", CLASS_SLOT_TACTICAL, CLASS_GROUP_SMOKE, CLASS_NATION_US,
      "WEAPON_ANM8_SMOKE_GRENADE", NULL, "hud_us_smokegrenade" },
    { "smoke_grenade_british_mp", CLASS_SLOT_TACTICAL, CLASS_GROUP_SMOKE, CLASS_NATION_UK,
      "WEAPON_NO77_WP_SMOKE_GRENADE", NULL, "hud_us_smokegrenade" },
    { "smoke_grenade_russian_mp", CLASS_SLOT_TACTICAL, CLASS_GROUP_SMOKE, CLASS_NATION_USSR,
      "WEAPON_RGD1_SMOKE_GRENADE", NULL, "hud_us_smokegrenade" },
    { "smoke_grenade_german_mp", CLASS_SLOT_TACTICAL, CLASS_GROUP_SMOKE, CLASS_NATION_DE, "WEAPON_NEBELHANDGRANATE",
      NULL, "hud_us_smokegrenade" },
};
const int g_classWeaponCount = (int)(sizeof(g_classWeapons) / sizeof(g_classWeapons[0]));

/* The primaries' attachments follow the Prima guide's suggestions for their
 * WWII counterparts. */
static const struct {
    const char *name;
    const char *weapon[CLASS_SLOT_COUNT];
    const char *attachments;
} s_classDefaults[CLASS_COUNT] = {
    { "Assault", { "mp44_mp", "luger_mp", "frag_grenade_german_mp", "smoke_grenade_german_mp" }, "grip+extmag" },
    { "Submachine Gunner", { "thompson_mp", "colt_mp", "frag_grenade_american_mp", "smoke_grenade_american_mp" },
      "steadyaim+extmag" },
    { "Rifleman", { "m1garand_mp", "colt_mp", "frag_grenade_american_mp", "smoke_grenade_american_mp" },
      "quickdraw+extmag" },
    { "Sniper", { "kar98k_sniper_mp", "luger_mp", "frag_grenade_german_mp", "smoke_grenade_german_mp" }, "fmj+extmag" },
    { "Close Quarters", { "shotgun_mp", "webley_mp", "frag_grenade_british_mp", "smoke_grenade_british_mp" },
      "steadyaim+rapidfire" },
};

int ClassData_Find(const char *weapon)
{
    int i;

    for (i = 0; weapon && i < g_classWeaponCount; i++) {
        if (!strcmp(g_classWeapons[i].weapon, weapon))
            return i;
    }
    return -1;
}

int ClassData_SlotWeapons(int slot, int *indexes, int max)
{
    int i, count = 0;

    for (i = 0; i < g_classWeaponCount && count < max; i++) {
        if (g_classWeapons[i].slot == slot)
            indexes[count++] = i;
    }
    return count;
}

/* A weapon slot's attachments from a list, none when it is not valid; 1 when
 * it was. */
static int ClassData_ReadAttachments(ClassLoadout *loadout, int slot, const char *list)
{
    int ids[ATT_COUNT], count, i;

    for (i = 0; i < ATT_PRIMARY_MAX; i++)
        loadout->attachment[slot][i] = -1;
    if (loadout->weapon[slot] < 0)
        return 0;
    count = BG_AttachmentsParseList(g_classWeapons[loadout->weapon[slot]].weapon, list, ClassData_AttachmentMax(slot),
                                    ids);
    for (i = 0; i < count; i++)
        loadout->attachment[slot][i] = ids[i];
    return count >= 0;
}

static int ClassData_FormatAttachments(const ClassLoadout *loadout, int slot, char *out, int size)
{
    return BG_AttachmentsFormatList(loadout->attachment[slot], ClassData_AttachmentCount(loadout, slot), out, size);
}

void ClassData_Defaults(ClassLoadout *classes)
{
    int i, slot;

    for (i = 0; i < CLASS_COUNT; i++) {
        snprintf(classes[i].name, sizeof(classes[i].name), "%s", s_classDefaults[i].name);
        for (slot = 0; slot < CLASS_SLOT_COUNT; slot++)
            classes[i].weapon[slot] = ClassData_Find(s_classDefaults[i].weapon[slot]);
        ClassData_ReadAttachments(&classes[i], CLASS_SLOT_PRIMARY, s_classDefaults[i].attachments);
        ClassData_ReadAttachments(&classes[i], CLASS_SLOT_SECONDARY, NULL);
    }
}

int ClassData_AttachmentMax(int slot)
{
    return slot == CLASS_SLOT_PRIMARY ? ATT_PRIMARY_MAX : slot == CLASS_SLOT_SECONDARY ? ATT_SECONDARY_MAX : 0;
}

int ClassData_AttachmentCount(const ClassLoadout *loadout, int slot)
{
    int count = 0;

    while (count < ClassData_AttachmentMax(slot) && loadout->attachment[slot][count] >= 0)
        count++;
    return count;
}

unsigned ClassData_AttachmentMask(const ClassLoadout *loadout, int slot)
{
    unsigned mask = 0;
    int i, count = ClassData_AttachmentCount(loadout, slot);

    for (i = 0; i < count; i++)
        mask |= ATT_BIT(loadout->attachment[slot][i]);
    return mask;
}

void ClassData_SetWeapon(ClassLoadout *loadout, int slot, int weapon)
{
    int i;

    if (loadout->weapon[slot] == weapon)
        return;
    loadout->weapon[slot] = weapon;
    if (slot <= CLASS_SLOT_SECONDARY) {
        for (i = 0; i < ATT_PRIMARY_MAX; i++)
            loadout->attachment[slot][i] = -1;
    }
}

int ClassData_EquipAttachment(ClassLoadout *loadout, int slot, int position, int attachment)
{
    int count = ClassData_AttachmentCount(loadout, slot);
    unsigned others = ClassData_AttachmentMask(loadout, slot);

    if (position < 0 || position >= ClassData_AttachmentMax(slot) || attachment < 0 || attachment >= ATT_COUNT ||
        loadout->weapon[slot] < 0 ||
        !(BG_AttachmentsAllowed(g_classWeapons[loadout->weapon[slot]].weapon) & ATT_BIT(attachment)))
        return 0;
    if (position >= count)
        position = count;
    else
        others &= ~ATT_BIT(loadout->attachment[slot][position]);
    if (BG_AttachmentConflict(others, attachment) >= 0)
        return 0;
    loadout->attachment[slot][position] = attachment;
    return 1;
}

void ClassData_RemoveAttachment(ClassLoadout *loadout, int slot, int position)
{
    int count = ClassData_AttachmentCount(loadout, slot), i;

    if (position < 0 || position >= count)
        return;
    for (i = position; i + 1 < count; i++)
        loadout->attachment[slot][i] = loadout->attachment[slot][i + 1];
    loadout->attachment[slot][count - 1] = -1;
}

static int ClassData_NameChar(int c)
{
    return c >= 0x20 && c <= 0x7e && !strchr("\"\\;%^", c);
}

int ClassData_ValidName(const char *name)
{
    size_t i, length = name ? strlen(name) : 0;

    if (!length || length > CLASS_NAME_MAX || name[0] == ' ' || name[length - 1] == ' ')
        return 0;
    for (i = 0; i < length; i++) {
        if (!ClassData_NameChar((unsigned char)name[i]))
            return 0;
    }
    return 1;
}

int ClassData_Parse(const char *text, ClassLoadout *classes)
{
    int valid = 1, i, slot;

    ClassData_Defaults(classes);
    for (i = 0; i < CLASS_COUNT; i++) {
        char fields[3 + CLASS_SLOT_COUNT][64];
        const char *end = text ? strchr(text, '\n') : NULL;
        int count = 0;

        memset(fields, 0, sizeof(fields));
        if (!text || !*text) {
            valid = 0;
            continue;
        }
        if (!end)
            end = text + strlen(text);
        /* An empty last field, as no secondary attachments, still counts. */
        while (count < 3 + CLASS_SLOT_COUNT) {
            const char *tab = memchr(text, '\t', (size_t)(end - text));
            size_t length = (size_t)((tab ? tab : end) - text);

            if (length < sizeof(fields[0]))
                memcpy(fields[count], text, length);
            else
                valid = 0;
            count++;
            text += length + (tab ? 1 : 0);
            if (!tab)
                break;
        }
        if (count != 1 + CLASS_SLOT_COUNT && count != 3 + CLASS_SLOT_COUNT)
            valid = 0;
        if (ClassData_ValidName(fields[0]))
            snprintf(classes[i].name, sizeof(classes[i].name), "%s", fields[0]);
        else
            valid = 0;
        for (slot = 0; slot < CLASS_SLOT_COUNT; slot++) {
            int weapon = ClassData_Find(fields[1 + slot]);

            if (weapon >= 0 && g_classWeapons[weapon].slot == slot)
                classes[i].weapon[slot] = weapon;
            else
                valid = 0;
        }
        /* Lines from before attachments carry none. */
        for (slot = CLASS_SLOT_PRIMARY; slot <= CLASS_SLOT_SECONDARY; slot++) {
            if (!ClassData_ReadAttachments(&classes[i], slot, fields[1 + CLASS_SLOT_COUNT + slot]))
                valid = 0;
        }
        text = *end ? end + 1 : end;
    }
    return valid;
}

int ClassData_Serialize(const ClassLoadout *classes, char *out, int size)
{
    int i, length = 0;

    if (size <= 0)
        return -1;
    out[0] = '\0';
    for (i = 0; i < CLASS_COUNT; i++) {
        const int *weapon = classes[i].weapon;
        char attachments[2][CLASS_ATTACHMENTS_MAX];
        int written;

        if (weapon[0] < 0 || weapon[1] < 0 || weapon[2] < 0 || weapon[3] < 0 ||
            ClassData_FormatAttachments(&classes[i], CLASS_SLOT_PRIMARY, attachments[0], CLASS_ATTACHMENTS_MAX) < 0 ||
            ClassData_FormatAttachments(&classes[i], CLASS_SLOT_SECONDARY, attachments[1], CLASS_ATTACHMENTS_MAX) < 0)
            return -1;
        written = snprintf(out + length, (size_t)(size - length), "%s\t%s\t%s\t%s\t%s\t%s\t%s\n", classes[i].name,
                           g_classWeapons[weapon[0]].weapon, g_classWeapons[weapon[1]].weapon,
                           g_classWeapons[weapon[2]].weapon, g_classWeapons[weapon[3]].weapon, attachments[0],
                           attachments[1]);
        if (written < 0 || written >= size - length)
            return -1;
        length += written;
    }
    return length;
}

int ClassData_EditName(char *name, int key)
{
    size_t length = strlen(name);

    if (!(key & 0x400))
        return 0;
    key &= ~0x400;
    if (key == 8) {
        if (length)
            name[length - 1] = '\0';
    } else if (ClassData_NameChar(key) && length < CLASS_NAME_MAX && (key != ' ' || length)) {
        name[length] = (char)key;
        name[length + 1] = '\0';
    }
    return 1;
}

int ClassData_Response(const ClassLoadout *loadout, char *out, int size)
{
    const int *weapon = loadout->weapon;
    char attachments[2][CLASS_ATTACHMENTS_MAX];
    int written, slot;

    if (weapon[0] < 0 || weapon[1] < 0 || weapon[2] < 0 || weapon[3] < 0)
        return -1;
    for (slot = CLASS_SLOT_PRIMARY; slot <= CLASS_SLOT_SECONDARY; slot++) {
        int length = ClassData_FormatAttachments(loadout, slot, attachments[slot], CLASS_ATTACHMENTS_MAX);

        if (length < 0)
            return -1;
        /* The server splits on commas and drops empty tokens. */
        if (!length)
            snprintf(attachments[slot], CLASS_ATTACHMENTS_MAX, "none");
    }
    written = snprintf(out, (size_t)size, "class,%s,%s,%s,%s,%s,%s", g_classWeapons[weapon[0]].weapon,
                       g_classWeapons[weapon[1]].weapon, g_classWeapons[weapon[2]].weapon,
                       g_classWeapons[weapon[3]].weapon, attachments[0], attachments[1]);
    return written < 0 || written >= size ? -1 : written;
}

enum { CLASS_RAW_INT, CLASS_RAW_MSEC, CLASS_RAW_FLOAT };

/* The weapon file keys behind AttachmentStats, read as the engine's loader
 * reads them: times in seconds become whole milliseconds. */
static const struct {
    const char *key;
    int type;
    size_t offset;
} s_classRawKeys[] = {
#define CLASS_RAW(key, type, field) { key, type, offsetof(AttachmentStats, field) }
    CLASS_RAW("clipSize", CLASS_RAW_INT, clipSize),
    CLASS_RAW("maxAmmo", CLASS_RAW_INT, maxAmmo),
    CLASS_RAW("startAmmo", CLASS_RAW_INT, startAmmo),
    CLASS_RAW("fireTime", CLASS_RAW_MSEC, fireTime),
    CLASS_RAW("rechamberTime", CLASS_RAW_MSEC, rechamberTime),
    CLASS_RAW("rechamberBoltTime", CLASS_RAW_MSEC, rechamberBoltTime),
    CLASS_RAW("adsTransInTime", CLASS_RAW_MSEC, adsTransInTime),
    CLASS_RAW("adsZoomFov", CLASS_RAW_FLOAT, adsZoomFov),
    CLASS_RAW("adsZoomInFrac", CLASS_RAW_FLOAT, adsZoomInFrac),
    CLASS_RAW("adsZoomOutFrac", CLASS_RAW_FLOAT, adsZoomOutFrac),
    CLASS_RAW("adsSpread", CLASS_RAW_FLOAT, adsSpread),
    CLASS_RAW("adsIdleAmount", CLASS_RAW_FLOAT, adsIdleAmount),
    CLASS_RAW("adsSwayMaxAngle", CLASS_RAW_FLOAT, adsSway[0]),
    CLASS_RAW("adsSwayPitchScale", CLASS_RAW_FLOAT, adsSway[1]),
    CLASS_RAW("adsSwayYawScale", CLASS_RAW_FLOAT, adsSway[2]),
    CLASS_RAW("adsSwayHorizScale", CLASS_RAW_FLOAT, adsSway[3]),
    CLASS_RAW("adsSwayVertScale", CLASS_RAW_FLOAT, adsSway[4]),
    CLASS_RAW("hipSpreadStandMin", CLASS_RAW_FLOAT, hipSpread[0]),
    CLASS_RAW("hipSpreadDuckedMin", CLASS_RAW_FLOAT, hipSpread[1]),
    CLASS_RAW("hipSpreadProneMin", CLASS_RAW_FLOAT, hipSpread[2]),
    CLASS_RAW("hipSpreadMax", CLASS_RAW_FLOAT, hipSpread[3]),
    CLASS_RAW("hipSpreadDuckedMax", CLASS_RAW_FLOAT, hipSpread[4]),
    CLASS_RAW("hipSpreadProneMax", CLASS_RAW_FLOAT, hipSpread[5]),
    CLASS_RAW("adsViewKickPitchMin", CLASS_RAW_FLOAT, adsKick[0]),
    CLASS_RAW("adsViewKickPitchMax", CLASS_RAW_FLOAT, adsKick[1]),
    CLASS_RAW("adsViewKickYawMin", CLASS_RAW_FLOAT, adsKick[2]),
    CLASS_RAW("adsViewKickYawMax", CLASS_RAW_FLOAT, adsKick[3]),
    CLASS_RAW("adsGunKickPitchMin", CLASS_RAW_FLOAT, adsKick[4]),
    CLASS_RAW("adsGunKickPitchMax", CLASS_RAW_FLOAT, adsKick[5]),
    CLASS_RAW("adsGunKickYawMin", CLASS_RAW_FLOAT, adsKick[6]),
    CLASS_RAW("adsGunKickYawMax", CLASS_RAW_FLOAT, adsKick[7]),
    CLASS_RAW("maxDamageRange", CLASS_RAW_FLOAT, maxDamageRange),
    CLASS_RAW("minDamageRange", CLASS_RAW_FLOAT, minDamageRange),
    CLASS_RAW("locHelmet", CLASS_RAW_FLOAT, headMultiplier[0]),
    CLASS_RAW("locHead", CLASS_RAW_FLOAT, headMultiplier[1]),
    CLASS_RAW("armorPiercing", CLASS_RAW_INT, armorPiercing),
#undef CLASS_RAW
};

static void ClassData_ReadRaw(AttachmentStats *raw, const char *key, size_t keyLength, const char *text)
{
    size_t i;

    for (i = 0; i < sizeof(s_classRawKeys) / sizeof(s_classRawKeys[0]); i++) {
        char *field = (char *)raw + s_classRawKeys[i].offset;

        if (strlen(s_classRawKeys[i].key) != keyLength || memcmp(s_classRawKeys[i].key, key, keyLength))
            continue;
        if (s_classRawKeys[i].type == CLASS_RAW_INT)
            *(int *)field = atoi(text);
        else if (s_classRawKeys[i].type == CLASS_RAW_MSEC)
            *(int *)field = (int)((float)atof(text) * 1000.0f);
        else
            *(float *)field = (float)atof(text);
        return;
    }
}

/* The menus only need to know whether a gun has a scope. */
static const char s_classScope[] = "scope";

static float ClassData_Accuracy(const AttachmentStats *raw)
{
    /* Aimed spread decides a shot's fate; a scope adds a fifth. */
    return (raw->overlay[0] ? 1.2f : 1.0f) / (0.25f + (raw->adsSpread > 0.0f ? raw->adsSpread : 0.0f));
}

/* The file is "WEAPONFILE\key\value\key\value...". */
void ClassData_ReadStats(const char *weaponFile, ClassStats *stats)
{
    float fireTime = 0.0f;
    const char *p = weaponFile;

    memset(stats, 0, sizeof(*stats));
    stats->raw.overlay = "";
    if (!p || strncmp(p, "WEAPONFILE", 10))
        return;
    p += 10;
    while (*p == '\\') {
        const char *key = p + 1, *value = strchr(key, '\\'), *end;
        char text[64];
        size_t keyLength, valueLength;

        if (!value)
            break;
        keyLength = (size_t)(value - key);
        value++;
        end = strchr(value, '\\');
        valueLength = end ? (size_t)(end - value) : strlen(value);
        if (valueLength < sizeof(text)) {
            memcpy(text, value, valueLength);
            text[valueLength] = '\0';
#define CLASS_KEY(name) (keyLength == sizeof(name) - 1 && !memcmp(key, name, keyLength))
            if (CLASS_KEY("damage"))
                stats->value[CLASS_STAT_DAMAGE] = (float)atof(text);
            else if (CLASS_KEY("maxDamageRange"))
                stats->value[CLASS_STAT_RANGE] = (float)atof(text);
            else if (CLASS_KEY("fireTime"))
                fireTime = (float)atof(text);
            else if (CLASS_KEY("moveSpeedScale"))
                stats->value[CLASS_STAT_MOBILITY] = (float)atof(text);
            else if (CLASS_KEY("clipSize"))
                stats->clip = atoi(text);
            else if (CLASS_KEY("maxAmmo"))
                stats->ammo = atoi(text);
            else if (CLASS_KEY("adsOverlayShader"))
                stats->raw.overlay = text[0] ? s_classScope : "";
            else if (CLASS_KEY("fuseTime"))
                stats->fuse = (float)atof(text);
#undef CLASS_KEY
            ClassData_ReadRaw(&stats->raw, key, keyLength, text);
        }
        if (!end)
            break;
        p = end;
    }
    stats->value[CLASS_STAT_ACCURACY] = ClassData_Accuracy(&stats->raw);
    stats->value[CLASS_STAT_FIRE_RATE] = fireTime > 0.0f ? 1.0f / fireTime : 0.0f;
    stats->valid = stats->value[CLASS_STAT_DAMAGE] > 0.0f;
}

void ClassData_AttachedStats(const ClassStats *base, const char *weapon, unsigned mask, ClassStats *attached)
{
    *attached = *base;
    BG_ApplyAttachmentStats(weapon, mask, &attached->raw);
    attached->clip = attached->raw.clipSize;
    attached->ammo = attached->raw.maxAmmo;
    attached->value[CLASS_STAT_RANGE] = attached->raw.maxDamageRange;
    attached->value[CLASS_STAT_ACCURACY] = ClassData_Accuracy(&attached->raw);
    /* The bars keep the file's seconds; the attachment scales the engine's
     * milliseconds. */
    if (base->raw.fireTime > 0 && attached->raw.fireTime > 0)
        attached->value[CLASS_STAT_FIRE_RATE] =
            base->value[CLASS_STAT_FIRE_RATE] * (float)base->raw.fireTime / (float)attached->raw.fireTime;
}

static int ClassData_Percent(float ratio)
{
    return (int)floorf((ratio - 1.0f) * 100.0f + 0.5f);
}

/* Magnification against the default 80-degree view. */
static float ClassData_Zoom(float fov)
{
    return fov > 0.0f ? tanf(40.0f * 0.01745329252f) / tanf(0.5f * fov * 0.01745329252f) : 1.0f;
}

int ClassData_AttachmentEffects(const AttachmentStats *base, const AttachmentStats *attached, char (*lines)[48],
                                int *tones, int max)
{
    int count = 0;

#define CLASS_EFFECT(better, ...)                                                                                 \
    do {                                                                                                          \
        if (count < max) {                                                                                        \
            tones[count] = (better);                                                                              \
            snprintf(lines[count++], sizeof(lines[0]), __VA_ARGS__);                                              \
        }                                                                                                         \
    } while (0)
#define CLASS_SIGN(a, b) ((a) > (b) ? 1 : -1)
    if (attached->clipSize != base->clipSize)
        CLASS_EFFECT(CLASS_SIGN(attached->clipSize, base->clipSize), "Magazine %d, from %d", attached->clipSize,
                     base->clipSize);
    if (attached->maxAmmo != base->maxAmmo)
        CLASS_EFFECT(CLASS_SIGN(attached->maxAmmo, base->maxAmmo), "Reserve %d, from %d", attached->maxAmmo,
                     base->maxAmmo);
    if (attached->fireTime != base->fireTime && attached->fireTime > 0)
        CLASS_EFFECT(CLASS_SIGN(base->fireTime, attached->fireTime), "Fire rate %+d%%",
                     ClassData_Percent((float)base->fireTime / (float)attached->fireTime));
    if (attached->adsTransInTime != base->adsTransInTime)
        CLASS_EFFECT(CLASS_SIGN(base->adsTransInTime, attached->adsTransInTime), "Aims in %d ms, from %d",
                     attached->adsTransInTime, base->adsTransInTime);
    if (attached->adsZoomFov != base->adsZoomFov)
        CLASS_EFFECT(0, "Zoom %.1fx, from %.1fx", (double)ClassData_Zoom(attached->adsZoomFov),
                     (double)ClassData_Zoom(base->adsZoomFov));
    if (attached->adsSpread != base->adsSpread && base->adsSpread > 0.0f)
        CLASS_EFFECT(CLASS_SIGN(base->adsSpread, attached->adsSpread), "Aimed spread %+d%%",
                     ClassData_Percent(attached->adsSpread / base->adsSpread));
    if (attached->hipSpread[0] != base->hipSpread[0] && base->hipSpread[0] > 0.0f)
        CLASS_EFFECT(CLASS_SIGN(base->hipSpread[0], attached->hipSpread[0]), "Hip-fire spread %+d%%",
                     ClassData_Percent(attached->hipSpread[0] / base->hipSpread[0]));
    if (attached->adsKick[1] != base->adsKick[1] && base->adsKick[1] != 0.0f)
        CLASS_EFFECT(CLASS_SIGN(fabsf(base->adsKick[1]), fabsf(attached->adsKick[1])), "Aimed recoil %+d%%",
                     ClassData_Percent(attached->adsKick[1] / base->adsKick[1]));
    if (attached->adsIdleAmount != base->adsIdleAmount && base->adsIdleAmount > 0.0f)
        CLASS_EFFECT(CLASS_SIGN(base->adsIdleAmount, attached->adsIdleAmount), "Aimed sway %+d%%",
                     ClassData_Percent(attached->adsIdleAmount / base->adsIdleAmount));
    if (attached->maxDamageRange != base->maxDamageRange && base->maxDamageRange > 0.0f)
        CLASS_EFFECT(CLASS_SIGN(attached->maxDamageRange, base->maxDamageRange), "Damage range %+d%%",
                     ClassData_Percent(attached->maxDamageRange / base->maxDamageRange));
    if (attached->headMultiplier[1] != base->headMultiplier[1])
        CLASS_EFFECT(CLASS_SIGN(attached->headMultiplier[1], base->headMultiplier[1]), "Head damage x%g, from x%g",
                     (double)attached->headMultiplier[1], (double)base->headMultiplier[1]);
    if (attached->armorPiercing && !base->armorPiercing)
        CLASS_EFFECT(1, "Keeps more damage through cover");
    if (attached->suppressed && !base->suppressed)
        CLASS_EFFECT(1, "Quiet, no flash, off the minimap");
#undef CLASS_SIGN
#undef CLASS_EFFECT
    return count;
}

int ClassData_Rate(float value, float lowest, float highest)
{
    int rating;

    if (!(highest > lowest))
        return 5;
    rating = 1 + (int)(9.0f * (value - lowest) / (highest - lowest) + 0.5f);
    return rating < 1 ? 1 : rating > 10 ? 10 : rating;
}
