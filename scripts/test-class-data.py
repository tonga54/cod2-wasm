#!/usr/bin/env python3
"""Exercise the client's class table, storage lines, names and stats."""
from pathlib import Path
import json
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = root / 'src/PC/ui_mp/ui_class_data.c'
rules = json.loads((root / 'downstream/gateway/classes.json').read_text())

# One set of lists and defaults: the host's, the client's and the server's.
table = re.findall(r'\{ "([A-Za-z0-9_]+_mp)", CLASS_SLOT_(PRIMARY|SECONDARY|LETHAL|TACTICAL),', source.read_text())
for slot in ('primary', 'secondary', 'lethal', 'tactical'):
    assert [weapon for weapon, kind in table if kind == slot.upper()] == rules[slot], slot
defaults = re.findall(r'\{ "([^"]+)", \{ "([^"]+)", "([^"]+)", "([^"]+)", "([^"]+)" \} \}', source.read_text())
assert [list(item) for item in defaults] == [[item['name'], item['primary'], item['secondary'], item['lethal'],
                                              item['tactical']] for item in rules['defaults']]
assert rules['nameMaxLength'] == int(re.search(r'#define CLASS_NAME_MAX (\d+)',
    (root / 'src/headers/PC/ui_mp/ui_class_data.h').read_text())[1])
gsc = (root / 'downstream/server/browser_classes.gsc').read_text()
for slot, function in (('primary', 'primaryWeapons'), ('secondary', 'secondaryWeapons'),
                       ('lethal', 'lethalWeapons'), ('tactical', 'tacticalWeapons')):
    body = gsc[gsc.index(function + '()\n{'):]
    body = body[:body.index('return weapons;')]
    assert re.findall(r'weapons\[\d+\] = "([^"]+)";', body) == rules[slot], slot
# Every image the screens draw is a retail menu image or HUD icon.
for image in re.findall(r'"(weapon_[a-z0-9_]+|gfx/icons/hud@[a-z0-9_]+\.tga|hud_[a-z0-9_]+)"', source.read_text()):
    assert re.fullmatch(r'weapon_[a-z0-9]+|gfx/icons/hud@[a-z0-9_]+\.tga|hud_us_smokegrenade', image), image

test = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "SOURCE"
static ClassLoadout classes[CLASS_COUNT];
static char text[1024];
static int slotOf(const char *weapon) { int i = ClassData_Find(weapon); return i < 0 ? -1 : g_classWeapons[i].slot; }
int main(void)
{
    int indexes[64];
    assert(ClassData_SlotWeapons(CLASS_SLOT_PRIMARY, indexes, 64) == 21);
    assert(ClassData_SlotWeapons(CLASS_SLOT_SECONDARY, indexes, 64) == 4 && slotOf("luger_mp") == CLASS_SLOT_SECONDARY);
    assert(ClassData_SlotWeapons(CLASS_SLOT_LETHAL, indexes, 64) == 4);
    assert(ClassData_SlotWeapons(CLASS_SLOT_TACTICAL, indexes, 64) == 4);
    assert(ClassData_SlotWeapons(CLASS_SLOT_PRIMARY, indexes, 3) == 3);
    assert(ClassData_Find("tt30_mp") < 0 && ClassData_Find(NULL) < 0 && ClassData_Find("TT30_mp") >= 0);

    /* Lines round-trip; a bad field falls back to its slot's default only. */
    ClassData_Defaults(classes);
    assert(!strcmp(classes[3].name, "Sniper"));
    strcpy(classes[1].name, "Kar Rusher");
    classes[1].weapon[CLASS_SLOT_PRIMARY] = ClassData_Find("kar98k_mp");
    classes[1].weapon[CLASS_SLOT_SECONDARY] = ClassData_Find("TT30_mp");
    assert(ClassData_Serialize(classes, text, sizeof(text)) > 0);
    assert(strstr(text, "Kar Rusher\tkar98k_mp\tTT30_mp\tfrag_grenade_american_mp\tsmoke_grenade_american_mp\n"));
    {
        ClassLoadout read[CLASS_COUNT];
        int i;
        assert(ClassData_Parse(text, read) == 1);
        for (i = 0; i < CLASS_COUNT; i++)
            assert(!strcmp(read[i].name, classes[i].name) && !memcmp(read[i].weapon, classes[i].weapon, sizeof(read[i].weapon)));
    }
    assert(ClassData_Serialize(classes, text, 40) < 0);
    {
        ClassLoadout read[CLASS_COUNT];
        const char *mixed = "Fine\tppsh_mp\tluger_mp\tfrag_grenade_russian_mp\tsmoke_grenade_russian_mp\n"
                            "bad\"name\tcolt_mp\tkar98k_mp\tsmoke_grenade_german_mp\tfrag_grenade_german_mp\n"
                            "Short\tmp40_mp\n";
        assert(ClassData_Parse(mixed, read) == 0);
        assert(!strcmp(read[0].name, "Fine") && read[0].weapon[0] == ClassData_Find("ppsh_mp"));
        assert(!strcmp(read[1].name, "Submachine Gunner"));
        assert(read[1].weapon[0] == ClassData_Find("thompson_mp") && read[1].weapon[1] == ClassData_Find("colt_mp"));
        assert(read[1].weapon[2] == ClassData_Find("frag_grenade_american_mp"));
        assert(read[1].weapon[3] == ClassData_Find("smoke_grenade_american_mp"));
        assert(!strcmp(read[2].name, "Short") && read[2].weapon[0] == ClassData_Find("mp40_mp"));
        assert(read[2].weapon[1] == ClassData_Find("colt_mp"));
        assert(!strcmp(read[4].name, "Close Quarters") && read[4].weapon[0] == ClassData_Find("shotgun_mp"));
        assert(ClassData_Parse(NULL, read) == 0 && !strcmp(read[0].name, "Assault"));
        assert(ClassData_Parse("", read) == 0);
    }
    {
        char longField[512];
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

    /* The menu's response, one token the server splits on commas. */
    assert(ClassData_Response(&classes[1], text, sizeof(text)) > 0);
    assert(!strcmp(text, "class,kar98k_mp,TT30_mp,frag_grenade_american_mp,smoke_grenade_american_mp"));
    assert(!strchr(text, ' ') && !strchr(text, ';'));
    assert(ClassData_Response(&classes[1], text, 20) < 0);

    /* Stats from a weapon file's own fields. */
    {
        ClassStats stats;
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
        assert(!stats.valid && !stats.clip);
        ClassData_ReadStats(NULL, &stats);
        assert(!stats.valid);
    }
    assert(ClassData_Rate(35.0f, 35.0f, 90.0f) == 1 && ClassData_Rate(90.0f, 35.0f, 90.0f) == 10);
    assert(ClassData_Rate(62.5f, 35.0f, 90.0f) == 6 && ClassData_Rate(500.0f, 35.0f, 90.0f) == 10);
    assert(ClassData_Rate(-5.0f, 35.0f, 90.0f) == 1 && ClassData_Rate(7.0f, 7.0f, 7.0f) == 5);
    puts("PASS: 33 weapons in four slots, storage lines round-trip with per-field defaults, host name rules, "
         "bounded rename, the comma response and stats from weapon files");
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
    ("(scoped ? 1.2f : 1.0f)", "1.0f"),
    ('CLASS_KEY("maxDamageRange")', 'CLASS_KEY("minDamageRange")'),
    ('CLASS_KEY("fuseTime")', 'CLASS_KEY("fuse")'),
    ("rating = 1 + (int)(9.0f * (value - lowest) / (highest - lowest) + 0.5f);",
     "rating = 1 + (int)(9.0f * (value - lowest) / (highest - lowest));"),
    ("    if (!(highest > lowest))\n        return 5;\n", ""),
    ("            if (length < sizeof(fields[0]))\n                memcpy(fields[count], text, length);\n            else\n                valid = 0;\n",
     "            memcpy(fields[count], text, length);\n"),
]
with tempfile.TemporaryDirectory(prefix='cod2-class-data-') as tmp:
    path = Path(tmp)
    flags = ['-std=c99', '-I', str(root / 'src'), '-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    (path / 'test.c').write_text(test.replace('SOURCE', str(source)))
    subprocess.run(['cc', *flags, '-O1', '-g', '-Wall', '-Werror', str(path / 'test.c'), '-lm', '-o', str(path / 'test')],
                   check=True)
    subprocess.run([str(path / 'test')], check=True)
    original = source.read_text()
    for index, (before, after) in enumerate(mutants):
        assert original.count(before) == 1, before
        mutant = path / f'mutant{index}.c'
        mutant.write_text(original.replace(before, after))
        (path / f'mutant{index}-test.c').write_text(test.replace('SOURCE', str(mutant)))
        subprocess.run(['cc', *flags, '-O0', '-w', str(path / f'mutant{index}-test.c'), '-lm', '-o',
                        str(path / f'mutant{index}')], check=True)
        result = subprocess.run([str(path / f'mutant{index}')], capture_output=True)
        assert result.returncode != 0, f'mutant survived: {after!r}'
print(f'PASS: classes.json, the client table and browser_classes.gsc agree; {len(mutants)} class data mutants fail')
