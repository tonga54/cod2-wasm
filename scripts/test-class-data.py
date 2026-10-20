#!/usr/bin/env python3
"""Exercise the client's class table, storage lines, names, attachments and stats."""
from pathlib import Path
import json
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = root / 'src/PC/ui_mp/ui_class_data.c'
camo_source = root / 'src/PC/bgame/bg_weapon_camo.c'
rules_source = root / 'src/PC/bgame/bg_attachment_rules.c'
rules = json.loads((root / 'downstream/gateway/classes.json').read_text())

# One set of lists and defaults: the host's, the client's and the server's.
table = re.findall(r'\{ "([A-Za-z0-9_]+_mp)", CLASS_SLOT_(PRIMARY|SECONDARY|LETHAL|TACTICAL),', source.read_text())
for slot in ('primary', 'secondary', 'lethal', 'tactical'):
    assert [weapon for weapon, kind in table if kind == slot.upper()] == rules[slot], slot
defaults = re.findall(r'\{ "([^"]+)", \{ "([^"]+)", "([^"]+)", "([^"]+)", "([^"]+)" \},\s*"([^"]*)" \}',
                      source.read_text())
assert [list(item) for item in defaults] == [[item['name'], item['primary'], item['secondary'], item['lethal'],
                                              item['tactical'], '+'.join(item['primaryAttachments'])]
                                             for item in rules['defaults']]
assert all(not item['secondaryAttachments'] for item in rules['defaults'])
assert rules['nameMaxLength'] == int(re.search(r'#define CLASS_NAME_MAX (\d+)',
    (root / 'src/headers/PC/ui_mp/ui_class_data.h').read_text())[1])
gsc = (root / 'downstream/server/browser_classes.gsc').read_text()
for slot, function in (('primary', 'primaryWeapons'), ('secondary', 'secondaryWeapons'),
                       ('lethal', 'lethalWeapons'), ('tactical', 'tacticalWeapons')):
    body = gsc[gsc.index(function + '()\n{'):]
    body = body[:body.index('return weapons;')]
    assert re.findall(r'weapons\[\d+\] = "([^"]+)";', body) == rules[slot], slot
# The camouflage ids in the same order everywhere, "none" first; each gun of
# the primary and secondary lists names the material it paints.
camo_text = camo_source.read_text()
camo_table = camo_text[camo_text.index('const WeaponCamo g_weaponCamos'):camo_text.index('};')]
assert re.findall(r'\{ "([a-z]+)", "[^"]+", "[^"]+" \}', camo_table) == rules['camos']
body = gsc[gsc.index('camoIds()\n{'):]
assert re.findall(r'camos\[\d+\] = "([a-z]+)";', body[:body.index('return camos;')]) == rules['camos']
assert int(re.search(r'#define WEAPON_CAMO_COUNT (\d+)',
                     (root / 'src/headers/PC/bgame/bg_weapon_camo.h').read_text())[1]) == len(rules['camos'])
guns = re.findall(r'\{ "([A-Za-z0-9_]+_mp)", "(mtl_[a-z0-9_]+)", WEAPON_CAMO_(WOOD|BODY) \}', camo_text)
assert [gun for gun, _, _ in guns] == rules['primary'] + rules['secondary']
assert all(style == 'BODY' for gun, _, style in guns if gun in rules['secondary']), 'pistols are painted whole'
for item in rules['defaults']:
    assert (item['primaryCamo'], item['secondaryCamo']) == ('none', 'none'), item['name']
# The server checks a class's attachments with the engine's own rules and
# fits them before refilling the ammunition they enlarge.
assert 'validWeaponAttachments(tokens[1], tokens[7], "primary")' in gsc
assert 'validWeaponAttachments(tokens[2], tokens[8], "primaryb")' in gsc
apply = gsc[gsc.index('applyClass()\n{'):]
for slot, index, weapon in (('primaryb', 7, 1), ('primary', 6, 0)):
    fit = apply.index(f'self setWeaponSlotAttachments("{slot}", loadout[{index}]);')
    assert apply.index(f'self giveMaxAmmo(loadout[{weapon}]);') > fit, slot


# A spawn that keeps the gametype's loadout clears the camouflage, which a
# painted gun picked up in an earlier life may have left.
def clears_before_keeping(text):
    head = text[text.index('applyClass()\n{'):]
    parts = head[:head.index('self takeAllWeapons();')].split('return;')[:-1]
    return len(parts) == 2 and all(part.rstrip().endswith('self setWeaponCamos("");') for part in parts)


assert clears_before_keeping(gsc)
unpainted = '\t{\n\t\tself setWeaponCamos("");\n\t\treturn;\n\t}\n'
assert gsc.count(unpainted) == 2
for kept in (0, 1):
    first = gsc.index(unpainted) if kept == 0 else gsc.index(unpainted, gsc.index(unpainted) + 1)
    assert not clears_before_keeping(gsc[:first] + '\t\treturn;\n' + gsc[first + len(unpainted):]), kept
# The S-Mine's and C4's icons and strings are the generated ones
# (browser_explosives.py); every other image is a retail menu image or HUD icon.
import sys
sys.path.insert(0, str(root / 'scripts'))
import browser_explosives
explosives = re.findall(r'\{ "(mine_mp|c4_mp)", CLASS_SLOT_LETHAL, CLASS_GROUP_[A-Z]+, CLASS_NATION_NONE,\s+"([A-Z0-9_]+)",\s+"([A-Z0-9_]+)",\s+"([a-z0-9_]+)" \}',
                        source.read_text())
assert [(weapon, image) for weapon, _, _, image in explosives] == [('mine_mp', browser_explosives.MINE_ICON),
                                                                 ('c4_mp', browser_explosives.C4_ICON)], explosives
for _, name, description, _ in explosives:
    for reference in (name, description):
        assert reference.startswith('BROWSER_') and f'REFERENCE           {reference[8:]}\r\n'.encode() in browser_explosives.STRING_TEXT, reference
for image in re.findall(r'"(weapon_[a-z0-9_]+|gfx/icons/hud@[a-z0-9_]+\.tga|hud_[a-z0-9_]+)"', source.read_text()):
    assert re.fullmatch(r'weapon_[a-z0-9]+|gfx/icons/hud@[a-z0-9_]+\.tga|hud_us_smokegrenade', image), image

dump = r'''
#include <stdio.h>
#include "headers/PC/bgame/bg_attachment_rules.h"
int main(void)
{
    int i, a, first;
    printf("{\"attachments\": [");
    for (a = 0; a < ATT_COUNT; a++)
        printf("%s\"%s\"", a ? ", " : "", g_attachments[a].name);
    printf("], \"optics\": [");
    for (a = 0, first = 1; a < ATT_COUNT; a++)
        if (ATT_OPTICS & ATT_BIT(a)) { printf("%s\"%s\"", first ? "" : ", ", g_attachments[a].name); first = 0; }
    printf("], \"attachmentLimits\": {\"primary\": %d, \"secondary\": %d}, \"weaponAttachments\": {",
           ATT_PRIMARY_MAX, ATT_SECONDARY_MAX);
    for (i = 0; i < g_attachmentWeaponCount; i++) {
        printf("%s\"%s\": [", i ? ", " : "", g_attachmentWeapons[i].weapon);
        for (a = 0, first = 1; a < ATT_COUNT; a++)
            if (g_attachmentWeapons[i].allowed & ATT_BIT(a)) { printf("%s\"%s\"", first ? "" : ", ", g_attachments[a].name); first = 0; }
        printf("]");
    }
    printf("}}\n");
    return 0;
}
'''

test = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "CAMO_SOURCE"
#include "RULES"
#include "SOURCE"
static ClassLoadout classes[CLASS_COUNT];
static char text[1024];
static int slotOf(const char *weapon) { int i = ClassData_Find(weapon); return i < 0 ? -1 : g_classWeapons[i].slot; }
static void attachments(const ClassLoadout *c, int slot, const char *expected)
{
    char list[CLASS_ATTACHMENTS_MAX];
    assert(BG_AttachmentsFormatList(c->attachment[slot], ClassData_AttachmentCount(c, slot), list, sizeof(list)) >= 0);
    if (strcmp(list, expected)) { fprintf(stderr, "attachments %s, expected %s\n", list, expected); assert(0); }
}
int main(void)
{
    int indexes[64];
    assert(ClassData_SlotWeapons(CLASS_SLOT_PRIMARY, indexes, 64) == 21);
    assert(ClassData_SlotWeapons(CLASS_SLOT_SECONDARY, indexes, 64) == 4 && slotOf("luger_mp") == CLASS_SLOT_SECONDARY);
    assert(ClassData_SlotWeapons(CLASS_SLOT_LETHAL, indexes, 64) == 6 && slotOf("c4_mp") == CLASS_SLOT_LETHAL);
    assert(ClassData_SlotWeapons(CLASS_SLOT_TACTICAL, indexes, 64) == 4);
    assert(ClassData_SlotWeapons(CLASS_SLOT_PRIMARY, indexes, 3) == 3);
    assert(ClassData_Find("tt30_mp") < 0 && ClassData_Find(NULL) < 0 && ClassData_Find("TT30_mp") >= 0);

    /* Defaults carry the guide's attachments on their primaries. */
    ClassData_Defaults(classes);
    assert(!strcmp(classes[3].name, "Sniper"));
    assert(classes[3].camo[0] == WEAPON_CAMO_NONE && classes[3].camo[1] == WEAPON_CAMO_NONE);
    attachments(&classes[0], CLASS_SLOT_PRIMARY, "grip+extmag");
    attachments(&classes[4], CLASS_SLOT_PRIMARY, "steadyaim+rapidfire");
    attachments(&classes[0], CLASS_SLOT_SECONDARY, "");
    assert(ClassData_AttachmentMask(&classes[0], CLASS_SLOT_PRIMARY) == (ATT_BIT(ATT_GRIP) | ATT_BIT(ATT_EXTMAG)));
    assert(ClassData_AttachmentMax(CLASS_SLOT_PRIMARY) == 3 && ClassData_AttachmentMax(CLASS_SLOT_SECONDARY) == 1);
    assert(ClassData_AttachmentMax(CLASS_SLOT_LETHAL) == 0);

    /* A new gun starts bare; the same gun keeps its attachments. */
    strcpy(classes[1].name, "Kar Rusher");
    ClassData_SetWeapon(&classes[1], CLASS_SLOT_PRIMARY, ClassData_Find("thompson_mp"));
    attachments(&classes[1], CLASS_SLOT_PRIMARY, "steadyaim+extmag");
    ClassData_SetWeapon(&classes[1], CLASS_SLOT_PRIMARY, ClassData_Find("kar98k_mp"));
    attachments(&classes[1], CLASS_SLOT_PRIMARY, "");
    ClassData_SetWeapon(&classes[1], CLASS_SLOT_SECONDARY, ClassData_Find("TT30_mp"));
    ClassData_SetWeapon(&classes[1], CLASS_SLOT_LETHAL, ClassData_Find("frag_grenade_russian_mp"));
    attachments(&classes[1], CLASS_SLOT_SECONDARY, "");

    /* Editing: the gun's own attachments, one optic, none twice, three at most. */
    assert(!ClassData_EquipAttachment(&classes[1], CLASS_SLOT_PRIMARY, 0, ATT_SUPPRESSOR));
    assert(!ClassData_EquipAttachment(&classes[1], CLASS_SLOT_PRIMARY, 0, ATT_COUNT));
    assert(ClassData_EquipAttachment(&classes[1], CLASS_SLOT_PRIMARY, 2, ATT_OPTIC4X));
    attachments(&classes[1], CLASS_SLOT_PRIMARY, "optic4x");
    assert(!ClassData_EquipAttachment(&classes[1], CLASS_SLOT_PRIMARY, 1, ATT_OPTIC4X));
    assert(ClassData_EquipAttachment(&classes[1], CLASS_SLOT_PRIMARY, 1, ATT_FMJ));
    assert(ClassData_EquipAttachment(&classes[1], CLASS_SLOT_PRIMARY, 2, ATT_EXTMAG));
    attachments(&classes[1], CLASS_SLOT_PRIMARY, "optic4x+fmj+extmag");
    assert(!ClassData_EquipAttachment(&classes[1], CLASS_SLOT_PRIMARY, 3, ATT_RAPIDFIRE));
    assert(ClassData_EquipAttachment(&classes[1], CLASS_SLOT_PRIMARY, 1, ATT_RAPIDFIRE));
    attachments(&classes[1], CLASS_SLOT_PRIMARY, "optic4x+rapidfire+extmag");
    ClassData_SetWeapon(&classes[2], CLASS_SLOT_PRIMARY, ClassData_Find("m1garand_mp"));
    assert(ClassData_EquipAttachment(&classes[2], CLASS_SLOT_PRIMARY, 2, ATT_REFLEX));
    attachments(&classes[2], CLASS_SLOT_PRIMARY, "quickdraw+extmag+reflex");
    assert(!ClassData_EquipAttachment(&classes[2], CLASS_SLOT_PRIMARY, 0, ATT_LENS));
    assert(ClassData_EquipAttachment(&classes[2], CLASS_SLOT_PRIMARY, 2, ATT_LENS));
    attachments(&classes[2], CLASS_SLOT_PRIMARY, "quickdraw+extmag+lens");
    ClassData_RemoveAttachment(&classes[2], CLASS_SLOT_PRIMARY, 0);
    attachments(&classes[2], CLASS_SLOT_PRIMARY, "extmag+lens");
    ClassData_RemoveAttachment(&classes[2], CLASS_SLOT_PRIMARY, 5);
    attachments(&classes[2], CLASS_SLOT_PRIMARY, "extmag+lens");
    assert(ClassData_EquipAttachment(&classes[1], CLASS_SLOT_SECONDARY, 0, ATT_SUPPRESSOR));
    assert(!ClassData_EquipAttachment(&classes[1], CLASS_SLOT_SECONDARY, 1, ATT_EXTMAG));
    assert(!ClassData_EquipAttachment(&classes[4], CLASS_SLOT_SECONDARY, 0, ATT_EXTMAG));

    classes[1].camo[CLASS_SLOT_PRIMARY] = BG_WeaponCamo_Find("brownspot");
    classes[2].camo[CLASS_SLOT_SECONDARY] = BG_WeaponCamo_Find("ember");

    /* Lines round-trip; a bad field falls back to its slot's default only. */
    assert(ClassData_Serialize(classes, text, sizeof(text)) > 0);
    assert(strstr(text, "Kar Rusher\tkar98k_mp\tTT30_mp\tfrag_grenade_russian_mp\tsmoke_grenade_american_mp\t"
                        "brownspot\tnone\toptic4x+rapidfire+extmag\tsuppressor\n"));
    assert(strstr(text, "Rifleman\tm1garand_mp\tcolt_mp\tfrag_grenade_american_mp\tsmoke_grenade_american_mp\t"
                        "none\tember\textmag+lens\t\n"));
    assert(strstr(text, "Assault\tmp44_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\tnone\tnone\t"
                        "grip+extmag\t\n"));
    {
        ClassLoadout read[CLASS_COUNT];
        int i;
        assert(ClassData_Parse(text, read) == 1);
        for (i = 0; i < CLASS_COUNT; i++)
            assert(!strcmp(read[i].name, classes[i].name) && !memcmp(read[i].weapon, classes[i].weapon, sizeof(read[i].weapon)) &&
                   !memcmp(read[i].camo, classes[i].camo, sizeof(read[i].camo)) &&
                   !memcmp(read[i].attachment, classes[i].attachment, sizeof(read[i].attachment)));
    }
    assert(ClassData_Serialize(classes, text, 40) < 0);
    {
        /* A camouflage out of the list does not serialize. */
        ClassLoadout broken[CLASS_COUNT];
        memcpy(broken, classes, sizeof(broken));
        broken[4].camo[1] = WEAPON_CAMO_COUNT;
        assert(ClassData_Serialize(broken, text, sizeof(text)) < 0);
        broken[4].camo[1] = -1;
        assert(ClassData_Serialize(broken, text, sizeof(text)) < 0);
    }
    {
        ClassLoadout read[CLASS_COUNT];
        /* Lines from before camouflage keep their weapons, unpainted; an unknown
         * camouflage is the gun's own finish. */
        const char *mixed = "Fine\tppsh_mp\tluger_mp\tfrag_grenade_russian_mp\tsmoke_grenade_russian_mp\n"
                            "bad\"name\tcolt_mp\tkar98k_mp\tsmoke_grenade_german_mp\tfrag_grenade_german_mp\n"
                            "Short\tmp40_mp\n"
                            "Painted\tg43_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\ttiger\twinter\n";
        assert(ClassData_Parse(mixed, read) == 0);
        assert(!strcmp(read[0].name, "Fine") && read[0].weapon[0] == ClassData_Find("ppsh_mp"));
        assert(read[0].camo[0] == WEAPON_CAMO_NONE && read[0].camo[1] == WEAPON_CAMO_NONE);
        attachments(&read[0], CLASS_SLOT_PRIMARY, "");
        assert(!strcmp(read[3].name, "Painted") && read[3].weapon[0] == ClassData_Find("g43_mp"));
        assert(read[3].camo[0] == WEAPON_CAMO_NONE && read[3].camo[1] == BG_WeaponCamo_Find("winter"));
        attachments(&read[3], CLASS_SLOT_PRIMARY, "");
        assert(!strcmp(read[1].name, "Submachine Gunner"));
        assert(read[1].weapon[0] == ClassData_Find("thompson_mp") && read[1].weapon[1] == ClassData_Find("colt_mp"));
        assert(read[1].weapon[2] == ClassData_Find("frag_grenade_american_mp"));
        assert(read[1].weapon[3] == ClassData_Find("smoke_grenade_american_mp"));
        assert(!strcmp(read[2].name, "Short") && read[2].weapon[0] == ClassData_Find("mp40_mp"));
        assert(read[2].weapon[1] == ClassData_Find("colt_mp"));
        assert(!strcmp(read[4].name, "Close Quarters") && read[4].weapon[0] == ClassData_Find("shotgun_mp"));
        attachments(&read[4], CLASS_SLOT_PRIMARY, "steadyaim+rapidfire");
        assert(ClassData_Parse(NULL, read) == 0 && !strcmp(read[0].name, "Assault"));
        assert(ClassData_Parse("", read) == 0);
    }
    {
        /* Lines from before attachments are whole; attachments a gun cannot
         * take are dropped with the rest of the line kept. */
        ClassLoadout read[CLASS_COUNT];
        const char *older = "A\tmp44_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\n"
                            "B\tmp44_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\n"
                            "C\tmp44_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\n"
                            "D\tmp44_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\n"
                            "E\tmp44_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\n";
        const char *bad = "A\tmp44_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\tnone\tnone\tsuppressor\tnone\n"
                          "B\tmp44_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\tnone\tnone\treflex+lens\t\n"
                          "C\tmp44_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\tnone\tnone\tgrip+grip\t\n"
                          "D\tmp44_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\tnone\tnone\tgrip+fmj+extmag+lens\t\n"
                          "E\tmp44_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\tnone\tnone\tgrip\tfmj+extmag\n";
        int i;
        assert(ClassData_Parse(older, read) == 1);
        for (i = 0; i < CLASS_COUNT; i++)
            attachments(&read[i], CLASS_SLOT_PRIMARY, "");
        assert(ClassData_Parse(bad, read) == 0);
        for (i = 0; i < CLASS_COUNT - 1; i++)
            attachments(&read[i], CLASS_SLOT_PRIMARY, "");
        attachments(&read[0], CLASS_SLOT_SECONDARY, "");
        attachments(&read[4], CLASS_SLOT_PRIMARY, "grip");
        attachments(&read[4], CLASS_SLOT_SECONDARY, "");
        assert(!strcmp(read[3].name, "D") && read[3].weapon[0] == ClassData_Find("mp44_mp"));
    }
    {
        /* Lines from before attachments, with their camouflage, are whole too. */
        ClassLoadout read[CLASS_COUNT];
        const char *painted = "A\tmp44_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\twinter\tnone\n"
                              "B\tmp44_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\tnone\tember\n"
                              "C\tmp44_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\tnone\tnone\n"
                              "D\tmp44_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\tnone\tnone\n"
                              "E\tmp44_mp\tluger_mp\tfrag_grenade_german_mp\tsmoke_grenade_german_mp\tnone\tnone\n";
        assert(ClassData_Parse(painted, read) == 1);
        assert(read[0].camo[0] == BG_WeaponCamo_Find("winter") && read[1].camo[1] == BG_WeaponCamo_Find("ember"));
        attachments(&read[0], CLASS_SLOT_PRIMARY, "");
        attachments(&read[1], CLASS_SLOT_SECONDARY, "");
    }
    {
        char longField[1024];
        ClassLoadout read[CLASS_COUNT];
        memset(longField, 'x', sizeof(longField) - 1);
        longField[sizeof(longField) - 1] = '\0';
        assert(ClassData_Parse(longField, read) == 0 && !strcmp(read[0].name, "Assault"));
    }

    /* Names as the host accepts them. */
    assert(ClassData_ValidName("Close Quarters") && ClassData_ValidName("x") && ClassData_ValidName("12345678901234567890"));
    assert(!ClassData_ValidName("") && !ClassData_ValidName(NULL) && !ClassData_ValidName("123456789012345678901"));
    assert(!ClassData_ValidName(" lead") && !ClassData_ValidName("trail ") && !ClassData_ValidName("semi;colon"));
    assert(!ClassData_ValidName("quo\"te") && !ClassData_ValidName("back\\slash") && !ClassData_ValidName("per%cent"));
    assert(!ClassData_ValidName("^1red") && !ClassData_ValidName("tab\tname") && !ClassData_ValidName("\xf1" "and" "\xfa"));
    {
        char name[CLASS_NAME_MAX + 1] = "";
        int i;
        assert(ClassData_EditName(name, 0x400 | ' ') && !name[0]);
        for (i = 0; i < 30; i++)
            assert(ClassData_EditName(name, 0x400 | ('a' + i % 26)));
        assert(strlen(name) == CLASS_NAME_MAX);
        assert(ClassData_EditName(name, 0x400 | 8) && strlen(name) == CLASS_NAME_MAX - 1);
        assert(ClassData_EditName(name, 0x400 | ';') && ClassData_EditName(name, 0x400 | '^'));
        assert(ClassData_EditName(name, 0x400 | 0x1b) && strlen(name) == CLASS_NAME_MAX - 1);
        assert(!ClassData_EditName(name, 0x9a) && !ClassData_EditName(name, 0x0d));
        name[0] = '\0';
        assert(ClassData_EditName(name, 0x400 | 8) && !name[0]);
    }

    /* The menu's response, one token the server splits on commas: a class
     * without camouflage or attachments as before either, any other with both
     * guns' camouflage and attachments, "none" for no attachments. */
    {
        ClassLoadout plain = classes[0], painted;

        ClassData_RemoveAttachment(&plain, CLASS_SLOT_PRIMARY, 0);
        ClassData_RemoveAttachment(&plain, CLASS_SLOT_PRIMARY, 0);
        assert(ClassData_Response(&plain, text, sizeof(text)) > 0);
        assert(!strcmp(text, "class,mp44_mp,luger_mp,frag_grenade_german_mp,smoke_grenade_german_mp"));
        painted = plain;
        painted.camo[CLASS_SLOT_SECONDARY] = BG_WeaponCamo_Find("ember");
        assert(ClassData_Response(&painted, text, sizeof(text)) > 0);
        assert(!strcmp(text, "class,mp44_mp,luger_mp,frag_grenade_german_mp,smoke_grenade_german_mp,none,ember,none,none"));
    }
    assert(ClassData_Response(&classes[0], text, sizeof(text)) > 0);
    assert(!strcmp(text, "class,mp44_mp,luger_mp,frag_grenade_german_mp,smoke_grenade_german_mp,none,none,grip+extmag,"
                         "none"));
    assert(ClassData_Response(&classes[1], text, sizeof(text)) > 0);
    assert(!strcmp(text, "class,kar98k_mp,TT30_mp,frag_grenade_russian_mp,smoke_grenade_american_mp,brownspot,none,"
                         "optic4x+rapidfire+extmag,suppressor"));
    assert(!strchr(text, ' ') && !strchr(text, ';'));
    assert(ClassData_Response(&classes[2], text, sizeof(text)) > 0);
    assert(!strcmp(text, "class,m1garand_mp,colt_mp,frag_grenade_american_mp,smoke_grenade_american_mp,none,ember,"
                         "extmag+lens,none"));
    assert(ClassData_Response(&classes[3], text, sizeof(text)) > 0);
    assert(!strcmp(text, "class,kar98k_sniper_mp,luger_mp,frag_grenade_german_mp,smoke_grenade_german_mp,none,none,"
                         "fmj+extmag,none"));
    assert(ClassData_Response(&classes[1], text, 20) < 0);
    {
        /* Room for all but the response's last byte is no response. */
        const char *full = "class,kar98k_mp,TT30_mp,frag_grenade_russian_mp,smoke_grenade_american_mp,brownspot,none,"
                           "optic4x+rapidfire+extmag,suppressor";
        ClassLoadout broken = classes[1];

        assert(ClassData_Response(&classes[1], text, (int)strlen(full)) < 0);
        assert(ClassData_Response(&classes[1], text, (int)strlen(full) + 1) == (int)strlen(full));
        broken.camo[0] = WEAPON_CAMO_COUNT;
        assert(ClassData_Response(&broken, text, sizeof(text)) < 0);
    }

    /* Stats from a weapon file's own fields. */
    {
        ClassStats stats, attached;
        char lines[8][48];
        int tones[8], count;
        ClassData_ReadStats("WEAPONFILE\\displayName\\WEAPON_X\\damage\\90\\maxDamageRange\\2000\\adsSpread\\0"
                            "\\fireTime\\0.33\\moveSpeedScale\\1\\clipSize\\5\\maxAmmo\\60\\adsOverlayShader\\scope", &stats);
        assert(stats.valid && stats.value[CLASS_STAT_DAMAGE] == 90.0f && stats.value[CLASS_STAT_RANGE] == 2000.0f);
        assert(fabsf(stats.value[CLASS_STAT_ACCURACY] - 4.8f) < 1e-4f);
        assert(fabsf(stats.value[CLASS_STAT_FIRE_RATE] - 1.0f / 0.33f) < 1e-4f);
        assert(stats.value[CLASS_STAT_MOBILITY] == 1.0f && stats.clip == 5 && stats.ammo == 60);
        ClassData_ReadStats("WEAPONFILE\\damage\\35\\adsSpread\\0.5\\adsOverlayShader\\\\fireTime\\0.086", &stats);
        assert(stats.valid && fabsf(stats.value[CLASS_STAT_ACCURACY] - 1.0f / 0.75f) < 1e-4f);
        /* A value equal to a key's name is still a value. */
        ClassData_ReadStats("WEAPONFILE\\weaponType\\damage\\damage\\45", &stats);
        assert(stats.value[CLASS_STAT_DAMAGE] == 45.0f);
        ClassData_ReadStats("WEAPONFILE\\damage\\0\\clipSize\\3", &stats);
        assert(!stats.valid && stats.fuse == 0.0f);
        /* A grenade gives its fuse, not a gun's bars. */
        ClassData_ReadStats("WEAPONFILE\\weaponType\\grenade\\fuseTime\\3.5\\cookOffHold\\0", &stats);
        assert(!stats.valid && stats.fuse == 3.5f);
        ClassData_ReadStats("not a weapon", &stats);
        assert(!stats.valid && !stats.clip && !strcmp(stats.raw.overlay, ""));
        ClassData_ReadStats(NULL, &stats);
        assert(!stats.valid);

        /* The engine's units behind the attachments, and their bars. */
        ClassData_ReadStats("WEAPONFILE\\damage\\35\\minDamage\\10\\maxDamageRange\\800\\minDamageRange\\1500"
                            "\\fireTime\\0.0857\\clipSize\\20\\maxAmmo\\180\\startAmmo\\140\\adsTransInTime\\0.22"
                            "\\adsZoomFov\\65\\adsSpread\\0.5\\hipSpreadStandMin\\1.5\\hipSpreadMax\\5"
                            "\\adsViewKickPitchMax\\70\\locHead\\2\\locHelmet\\2\\moveSpeedScale\\1", &stats);
        assert(stats.raw.fireTime == 85 && stats.raw.adsTransInTime == 220 && stats.raw.clipSize == 20);
        assert(stats.raw.maxDamageRange == 800.0f && stats.raw.hipSpread[0] == 1.5f && stats.raw.hipSpread[3] == 5.0f);
        assert(stats.raw.headMultiplier[0] == 2.0f && stats.raw.headMultiplier[1] == 2.0f && stats.raw.adsKick[1] == 70.0f);
        ClassData_AttachedStats(&stats, "thompson_mp", 0, &attached);
        assert(!memcmp(attached.value, stats.value, sizeof(stats.value)) && attached.clip == 20 && attached.ammo == 180);
        ClassData_AttachedStats(&stats, "thompson_mp",
                                ATT_BIT(ATT_EXTMAG) | ATT_BIT(ATT_RAPIDFIRE) | ATT_BIT(ATT_REFLEX), &attached);
        assert(attached.clip == 30 && attached.ammo == 270 && attached.raw.startAmmo == 210);
        assert(attached.raw.fireTime == 80);
        assert(fabsf(attached.value[CLASS_STAT_FIRE_RATE] - stats.value[CLASS_STAT_FIRE_RATE] * 85.0f / 80.0f) < 1e-3f);
        assert(fabsf(attached.value[CLASS_STAT_ACCURACY] - 1.0f / (0.25f + 0.375f)) < 1e-4f);
        assert(attached.value[CLASS_STAT_DAMAGE] == stats.value[CLASS_STAT_DAMAGE]);
        count = ClassData_AttachmentEffects(&stats.raw, &attached.raw, lines, tones, 8);
        assert(count == 5);
        assert(!strcmp(lines[0], "Magazine 30, from 20") && tones[0] == 1);
        assert(!strcmp(lines[1], "Reserve 270, from 180") && tones[1] == 1);
        assert(!strcmp(lines[2], "Fire rate +6%") && tones[2] == 1);
        assert(!strcmp(lines[3], "Zoom 1.5x, from 1.3x") && tones[3] == 0);
        assert(!strcmp(lines[4], "Aimed spread -25%") && tones[4] == 1);
        ClassData_AttachedStats(&stats, "thompson_mp", ATT_BIT(ATT_SUPPRESSOR) | ATT_BIT(ATT_QUICKDRAW) |
                                ATT_BIT(ATT_STEADYAIM), &attached);
        count = ClassData_AttachmentEffects(&stats.raw, &attached.raw, lines, tones, 8);
        assert(count == 4);
        assert(!strcmp(lines[0], "Aims in 110 ms, from 220") && tones[0] == 1);
        assert(!strcmp(lines[1], "Hip-fire spread -35%") && tones[1] == 1);
        assert(!strcmp(lines[2], "Damage range -30%") && tones[2] == -1);
        assert(!strcmp(lines[3], "Quiet, no flash, off the minimap") && tones[3] == 1);
        assert(attached.value[CLASS_STAT_RANGE] == 560.0f);
        /* Advanced Rifling keeps an SMG's full damage range and its bar; the
         * fall beyond reaches a quarter further. */
        ClassData_AttachedStats(&stats, "thompson_mp", ATT_BIT(ATT_RIFLING), &attached);
        count = ClassData_AttachmentEffects(&stats.raw, &attached.raw, lines, tones, 8);
        assert(count == 1 && !strcmp(lines[0], "Falloff range +25%") && tones[0] == 1);
        assert(attached.value[CLASS_STAT_RANGE] == 800.0f && attached.raw.minDamageRange == 1875.0f);
        /* High Caliber: a semi-automatic rifle's headshot x3; an automatic
         * one stops a point short of a kill. */
        {
            ClassStats rifle;
            ClassData_ReadStats("WEAPONFILE\\damage\\45\\semiAuto\\1\\maxDamageRange\\2000\\minDamageRange\\3000"
                                "\\fireTime\\0.135\\adsViewKickPitchMax\\70\\locHead\\2\\locHelmet\\2", &rifle);
            ClassData_AttachedStats(&rifle, "m1garand_mp", ATT_BIT(ATT_HIGHCAL) | ATT_BIT(ATT_FMJ) | ATT_BIT(ATT_GRIP),
                                    &attached);
            count = ClassData_AttachmentEffects(&rifle.raw, &attached.raw, lines, tones, 8);
            assert(count == 3 && !strcmp(lines[0], "Aimed recoil -10%") && !strcmp(lines[1], "Head damage x3, from x2"));
            assert(!strcmp(lines[2], "Keeps more damage through cover"));
            assert(ClassData_AttachmentEffects(&rifle.raw, &attached.raw, lines, tones, 1) == 1);
        }
        assert(stats.raw.damage == 35 && !stats.raw.semiAuto && !stats.raw.boltAction);
        ClassData_AttachedStats(&stats, "mp44_mp", ATT_BIT(ATT_HIGHCAL), &attached);
        count = ClassData_AttachmentEffects(&stats.raw, &attached.raw, lines, tones, 8);
        assert(count == 1 && !strcmp(lines[0], "Head damage x2.84, from x2") && tones[0] == 1);
        /* A 4x Optic scopes the rifle: its fifth for a scope, and 4x zoom. */
        ClassData_AttachedStats(&stats, "m1garand_mp", ATT_BIT(ATT_OPTIC4X), &attached);
        assert(fabsf(attached.value[CLASS_STAT_ACCURACY] - 1.2f / 0.75f) < 1e-4f && attached.raw.overlay[0]);
        count = ClassData_AttachmentEffects(&stats.raw, &attached.raw, lines, tones, 8);
        assert(count == 1 && !strcmp(lines[0], "Zoom 4.0x, from 1.3x"));
    }
    assert(ClassData_Rate(35.0f, 35.0f, 90.0f) == 1 && ClassData_Rate(90.0f, 35.0f, 90.0f) == 10);
    assert(ClassData_Rate(62.5f, 35.0f, 90.0f) == 6 && ClassData_Rate(500.0f, 35.0f, 90.0f) == 10);
    assert(ClassData_Rate(-5.0f, 35.0f, 90.0f) == 1 && ClassData_Rate(7.0f, 7.0f, 7.0f) == 5);
    puts("PASS: 35 weapons in four slots, two camouflages and the guns' attachments per class, attachments "
         "edited by WWII's rules, storage lines round-trip with per-field defaults, host name rules, bounded "
         "rename, the comma response with camouflage and attachments only when set, and stats with "
         "attachments");
    return 0;
}
'''

mutants = [
    ('for (i = 0; weapon && i < g_classWeaponCount; i++)', 'for (i = 0; i < g_classWeaponCount; i++)'),
    ('if (weapon >= 0 && g_classWeapons[weapon].slot == slot)', 'if (weapon >= 0)'),
    ('        if (ClassData_ValidName(fields[0]))\n', '        if (fields[0][0])\n'),
    ('    return c >= 0x20 && c <= 0x7e && !strchr("\\"\\\\;%^", c);', '    return c >= 0x20 && c <= 0x7e;'),
    ("length > CLASS_NAME_MAX || name[0] == ' '", "length > CLASS_NAME_MAX + 1 || name[0] == ' '"),
    ("&& length < CLASS_NAME_MAX && (key != ' ' || length)", "&& length <= CLASS_NAME_MAX && (key != ' ' || length)"),
    ("    if (key == 8) {", "    if (key == 9) {"),
    ('"class,%s,%s,%s,%s"', '"class %s %s %s %s"'),
    ('            snprintf(attachments[slot], CLASS_ATTACHMENTS_MAX, "none");\n', '            ;\n'),
    ("(raw->overlay[0] ? 1.2f : 1.0f)", "1.0f"),
    ('CLASS_KEY("maxDamageRange")', 'CLASS_KEY("minDamageRange")'),
    ('CLASS_KEY("fuseTime")', 'CLASS_KEY("fuse")'),
    ("rating = 1 + (int)(9.0f * (value - lowest) / (highest - lowest) + 0.5f);",
     "rating = 1 + (int)(9.0f * (value - lowest) / (highest - lowest));"),
    ("    if (!(highest > lowest))\n        return 5;\n", ""),
    ("            if (length < sizeof(fields[0]))\n                memcpy(fields[count], text, length);\n            else\n                valid = 0;\n",
     "            memcpy(fields[count], text, length);\n"),
    ('        while (count < 1 + CLASS_SLOT_COUNT + CLASS_CAMO_SLOTS + 2) {',
     '        while (count < 1 + CLASS_SLOT_COUNT) {'),
    ('            if (camo >= 0)\n                classes[i].camo[slot] = camo;', '            if (camo > 0)\n                classes[i].camo[slot] = camo;'),
    ('            classes[i].camo[slot] = WEAPON_CAMO_NONE;\n', '            classes[i].camo[slot] = 1;\n'),
    ('    extras = loadout->camo[0] || loadout->camo[1];\n', '    extras = loadout->camo[0];\n'),
    ('loadout->camo[1] >= 0 &&\n           loadout->camo[1] < WEAPON_CAMO_COUNT', 'loadout->camo[1] >= 0'),
    ('        written = more < 0 ? -1 : written + more;', '        written = more < 0 ? -1 : written;'),
    ('g_weaponCamos[classes[i].camo[0]].id, g_weaponCamos[classes[i].camo[1]].id, attachments[0],',
     'g_weaponCamos[classes[i].camo[0]].id, g_weaponCamos[classes[i].camo[0]].id, attachments[0],'),
    ("        if (count != 1 + CLASS_SLOT_COUNT && count != 1 + CLASS_SLOT_COUNT + CLASS_CAMO_SLOTS &&\n",
     "        if (count != 1 + CLASS_SLOT_COUNT + CLASS_CAMO_SLOTS &&\n"),
    ("count != 1 + CLASS_SLOT_COUNT + CLASS_CAMO_SLOTS &&\n            count", "count"),
    ("            if (!ClassData_ReadAttachments(&classes[i], slot, fields[1 + CLASS_SLOT_COUNT + CLASS_CAMO_SLOTS + slot]))\n"
     "                valid = 0;\n",
     "            ClassData_ReadAttachments(&classes[i], slot, fields[1 + CLASS_SLOT_COUNT + CLASS_CAMO_SLOTS + slot]);\n"),
    ("    if (loadout->weapon[slot] == weapon)\n        return;\n", ""),
    ("    if (slot <= CLASS_SLOT_SECONDARY) {\n        for (i = 0; i < ATT_PRIMARY_MAX; i++)\n            loadout->attachment[slot][i] = -1;\n    }\n", ""),
    ("    if (BG_AttachmentConflict(others, attachment) >= 0)\n        return 0;\n", ""),
    ("        others &= ~ATT_BIT(loadout->attachment[slot][position]);\n", ""),
    ("    for (i = position; i + 1 < count; i++)\n        loadout->attachment[slot][i] = loadout->attachment[slot][i + 1];\n", ""),
    ("*(int *)field = (int)((float)atof(text) * 1000.0f);", "*(int *)field = (int)((float)atof(text) * 1000.0f + 0.5f);"),
    ("base->value[CLASS_STAT_FIRE_RATE] * (float)base->raw.fireTime / (float)attached->raw.fireTime",
     "base->value[CLASS_STAT_FIRE_RATE]"),
    ("    attached->clip = attached->raw.clipSize;\n", ""),
    ("CLASS_EFFECT(CLASS_SIGN(attached->maxDamageRange, base->maxDamageRange)",
     "CLASS_EFFECT(CLASS_SIGN(base->maxDamageRange, attached->maxDamageRange)"),
]
with tempfile.TemporaryDirectory(prefix='cod2-class-data-') as tmp:
    path = Path(tmp)
    flags = ['-std=c99', '-I', str(root / 'src'), '-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    (path / 'dump.c').write_text(dump)
    subprocess.run(['cc', '-std=c99', '-I', str(root / 'src'), '-Wall', '-Werror', str(path / 'dump.c'),
                    str(rules_source), '-o', str(path / 'dump')], check=True)
    table = json.loads(subprocess.run([str(path / 'dump')], check=True, capture_output=True, text=True).stdout)
    for key in ('attachments', 'optics', 'attachmentLimits', 'weaponAttachments'):
        assert table[key] == rules[key], key
    assert list(table['weaponAttachments']) == rules['primary'] + rules['secondary']

    program = test.replace('CAMO_SOURCE', str(camo_source)).replace('RULES', str(rules_source))
    (path / 'test.c').write_text(program.replace('SOURCE', str(source)))
    subprocess.run(['cc', *flags, '-O1', '-g', '-Wall', '-Werror', str(path / 'test.c'), '-lm', '-o', str(path / 'test')],
                   check=True)
    subprocess.run([str(path / 'test')], check=True)
    original = source.read_text()
    for index, (before, after) in enumerate(mutants):
        assert original.count(before) == 1, before
        mutant = path / f'mutant{index}.c'
        mutant.write_text(original.replace(before, after))
        (path / f'mutant{index}-test.c').write_text(program.replace('SOURCE', str(mutant)))
        subprocess.run(['cc', *flags, '-O0', '-w', str(path / f'mutant{index}-test.c'), '-lm', '-o',
                        str(path / f'mutant{index}')], check=True)
        result = subprocess.run([str(path / f'mutant{index}')], capture_output=True)
        assert result.returncode != 0, f'mutant survived: {after!r}'
print(f'PASS: classes.json, the client tables and browser_classes.gsc agree; {len(mutants)} class data mutants fail')
