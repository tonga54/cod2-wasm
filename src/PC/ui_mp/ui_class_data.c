#include "headers/PC/ui_mp/ui_class_data.h"
#include "headers/PC/bgame/bg_weapon_camo.h"
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

static const struct {
    const char *name;
    const char *weapon[CLASS_SLOT_COUNT];
} s_classDefaults[CLASS_COUNT] = {
    { "Assault", { "mp44_mp", "luger_mp", "frag_grenade_german_mp", "smoke_grenade_german_mp" } },
    { "Submachine Gunner", { "thompson_mp", "colt_mp", "frag_grenade_american_mp", "smoke_grenade_american_mp" } },
    { "Rifleman", { "m1garand_mp", "colt_mp", "frag_grenade_american_mp", "smoke_grenade_american_mp" } },
    { "Sniper", { "kar98k_sniper_mp", "luger_mp", "frag_grenade_german_mp", "smoke_grenade_german_mp" } },
    { "Close Quarters", { "shotgun_mp", "webley_mp", "frag_grenade_british_mp", "smoke_grenade_british_mp" } },
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

void ClassData_Defaults(ClassLoadout *classes)
{
    int i, slot;

    for (i = 0; i < CLASS_COUNT; i++) {
        snprintf(classes[i].name, sizeof(classes[i].name), "%s", s_classDefaults[i].name);
        for (slot = 0; slot < CLASS_SLOT_COUNT; slot++)
            classes[i].weapon[slot] = ClassData_Find(s_classDefaults[i].weapon[slot]);
        for (slot = 0; slot < CLASS_CAMO_SLOTS; slot++)
            classes[i].camo[slot] = WEAPON_CAMO_NONE;
    }
}

static int ClassData_Camos(const ClassLoadout *loadout)
{
    return loadout->camo[0] >= 0 && loadout->camo[0] < WEAPON_CAMO_COUNT && loadout->camo[1] >= 0 &&
           loadout->camo[1] < WEAPON_CAMO_COUNT;
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
        char fields[1 + CLASS_SLOT_COUNT + CLASS_CAMO_SLOTS][64];
        const char *end = text ? strchr(text, '\n') : NULL;
        int count = 0;

        memset(fields, 0, sizeof(fields));
        if (!text || !*text) {
            valid = 0;
            continue;
        }
        if (!end)
            end = text + strlen(text);
        while (text < end && count < 1 + CLASS_SLOT_COUNT + CLASS_CAMO_SLOTS) {
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
        if (count != 1 + CLASS_SLOT_COUNT + CLASS_CAMO_SLOTS)
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
        for (slot = 0; slot < CLASS_CAMO_SLOTS; slot++) {
            int camo = BG_WeaponCamo_Find(fields[1 + CLASS_SLOT_COUNT + slot]);

            if (camo >= 0)
                classes[i].camo[slot] = camo;
            else
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
        int written;

        if (weapon[0] < 0 || weapon[1] < 0 || weapon[2] < 0 || weapon[3] < 0 || !ClassData_Camos(&classes[i]))
            return -1;
        written = snprintf(out + length, (size_t)(size - length), "%s\t%s\t%s\t%s\t%s\t%s\t%s\n", classes[i].name,
                           g_classWeapons[weapon[0]].weapon, g_classWeapons[weapon[1]].weapon,
                           g_classWeapons[weapon[2]].weapon, g_classWeapons[weapon[3]].weapon,
                           g_weaponCamos[classes[i].camo[0]].id, g_weaponCamos[classes[i].camo[1]].id);
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
    int written;

    if (weapon[0] < 0 || weapon[1] < 0 || weapon[2] < 0 || weapon[3] < 0 || !ClassData_Camos(loadout))
        return -1;
    written = snprintf(out, (size_t)size, "class,%s,%s,%s,%s", g_classWeapons[weapon[0]].weapon,
                       g_classWeapons[weapon[1]].weapon, g_classWeapons[weapon[2]].weapon,
                       g_classWeapons[weapon[3]].weapon);
    if (written >= 0 && written < size && (loadout->camo[0] || loadout->camo[1])) {
        int camos = snprintf(out + written, (size_t)(size - written), ",%s,%s", g_weaponCamos[loadout->camo[0]].id,
                             g_weaponCamos[loadout->camo[1]].id);

        written = camos < 0 ? -1 : written + camos;
    }
    return written < 0 || written >= size ? -1 : written;
}

/* The file is "WEAPONFILE\key\value\key\value...". */
void ClassData_ReadStats(const char *weaponFile, ClassStats *stats)
{
    float spread = 0.0f, fireTime = 0.0f;
    int scoped = 0;
    const char *p = weaponFile;

    memset(stats, 0, sizeof(*stats));
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
            else if (CLASS_KEY("adsSpread"))
                spread = (float)atof(text);
            else if (CLASS_KEY("fireTime"))
                fireTime = (float)atof(text);
            else if (CLASS_KEY("moveSpeedScale"))
                stats->value[CLASS_STAT_MOBILITY] = (float)atof(text);
            else if (CLASS_KEY("clipSize"))
                stats->clip = atoi(text);
            else if (CLASS_KEY("maxAmmo"))
                stats->ammo = atoi(text);
            else if (CLASS_KEY("adsOverlayShader"))
                scoped = text[0] != '\0';
            else if (CLASS_KEY("fuseTime"))
                stats->fuse = (float)atof(text);
#undef CLASS_KEY
        }
        if (!end)
            break;
        p = end;
    }
    /* Aimed spread decides a shot's fate; a scope adds a fifth. */
    stats->value[CLASS_STAT_ACCURACY] = (scoped ? 1.2f : 1.0f) / (0.25f + (spread > 0.0f ? spread : 0.0f));
    stats->value[CLASS_STAT_FIRE_RATE] = fireTime > 0.0f ? 1.0f / fireTime : 0.0f;
    stats->valid = stats->value[CLASS_STAT_DAMAGE] > 0.0f;
}

int ClassData_Rate(float value, float lowest, float highest)
{
    int rating;

    if (!(highest > lowest))
        return 5;
    rating = 1 + (int)(9.0f * (value - lowest) / (highest - lowest) + 0.5f);
    return rating < 1 ? 1 : rating > 10 ? 10 : rating;
}
