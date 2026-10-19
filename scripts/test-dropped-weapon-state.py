#!/usr/bin/env python3
"""A gun dropped with a camouflage and attachments keeps them: Drop_Weapon writes them into the item, and whoever
picks it up gets the attachments in the slot it lands in, the camouflage in their configstring, and the clip and
reserve the attachments give, as g_items_mp.c's helpers and its two call sites do."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
items_path = root / 'src/PC/game_mp/g_items_mp.c'
items = items_path.read_text()


def function(source, name):
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


HELPERS = ('G_ItemClipSize', 'G_ItemMaxAmmo', 'G_ItemWeaponAttachments', 'G_TakeItemWeaponState')

support = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct { char weaponslots[8]; } playerState_t;
typedef struct { int number, legsAnim, torsoAnim; } entityState_t;
typedef struct { playerState_t ps; } gclient_t;
typedef struct { entityState_t s; gclient_t *client; } gentity_t;
typedef struct { const char *szInternalName; int iClipSize, iMaxAmmo; } WeaponDef;
enum { THOMPSON = 3, COLT = 7, LATE = 200 };
#define EXTMAG 1u
#define SUPPRESSOR 2u
#define GRIP 4u
#define OPTIC 8u
/* A weapon numbered past a signed byte's range, as the weapon slots store them. */
static WeaponDef defs[256] = { [THOMPSON] = { "thompson_mp", 20, 300 }, [COLT] = { "colt_mp", 7, 49 },
                               [LATE] = { "thompson_mp", 20, 300 } };
static WeaponDef attached;
WeaponDef *BG_GetWeaponDef(int weapon) { return &defs[weapon]; }
/* The Thompson takes its magazine, can and grip; the 4x Optic is no SMG's. */
unsigned BG_AttachmentsAllowed(const char *weapon) { return !strcmp(weapon, "thompson_mp") ? EXTMAG | SUPPRESSOR | GRIP : SUPPRESSOR; }
WeaponDef *BG_GetAttachedWeaponDef(int weapon, unsigned mask)
{
    attached = defs[weapon];
    if (mask & EXTMAG) {
        attached.iClipSize = attached.iClipSize * 3 / 2;
        attached.iMaxAmmo = attached.iMaxAmmo * 3 / 2;
    }
    return &attached;
}
int BG_ClipForWeapon(int weapon) { return weapon + 1000; }
int BG_AmmoForWeapon(int weapon) { return weapon + 2000; }
int BG_GetAmmoClipSize(int clip) { return defs[clip - 1000].iClipSize; }
int BG_GetAmmoTypeMax(int ammo) { return defs[ammo - 2000].iMaxAmmo; }
unsigned BG_GetItemWeaponAttachments(const entityState_t *es) { return (unsigned)es->legsAnim; }
int BG_GetItemWeaponCamo(const entityState_t *es) { return es->torsoAnim; }
static unsigned slotMasks[3];
static int slotSets;
void BG_SetSlotAttachments(playerState_t *ps, int slot, unsigned mask) { (void)ps; assert(slot == 1 || slot == 2); slotMasks[slot] = mask; slotSets++; }
static struct { int clientNum, weapon, camo; const playerState_t *ps; } camos;
static int camoSets;
void G_SetClientSlotCamos(int clientNum, const playerState_t *ps, int weapon, int camo)
{ camos.clientNum = clientNum; camos.ps = ps; camos.weapon = weapon; camos.camo = camo; camoSets++; }
'''

checks = r'''
int main(void)
{
    gclient_t client;
    gentity_t picker = { { 5, 0, 0 }, &client }, item = { { 140, 0, 0 }, NULL };
    int cases = 0;

    /* Without attachments a gun's own clip and reserve; with them, what they make of the weapon. */
    assert(G_ItemClipSize(THOMPSON, 0) == 20 && G_ItemMaxAmmo(THOMPSON, 0) == 300);
    assert(G_ItemClipSize(THOMPSON, EXTMAG) == 30 && G_ItemMaxAmmo(THOMPSON, EXTMAG) == 450);
    assert(G_ItemClipSize(THOMPSON, GRIP) == 20 && G_ItemClipSize(COLT, SUPPRESSOR) == 7);
    cases++;

    /* What the item carries, of what its weapon takes. */
    item.s.legsAnim = (int)(EXTMAG | GRIP | OPTIC);
    assert(G_ItemWeaponAttachments(&item, THOMPSON) == (EXTMAG | GRIP));
    assert(G_ItemWeaponAttachments(&item, COLT) == 0);
    cases++;

    /* Taken into the second slot: its attachments there, the first slot's left alone, the camouflage to the picker's
     * configstring for that weapon. */
    memset(&client, 0, sizeof(client));
    client.ps.weaponslots[1] = COLT;
    client.ps.weaponslots[2] = THOMPSON;
    item.s.torsoAnim = 6;
    G_TakeItemWeaponState(&picker, &item, THOMPSON);
    assert(slotSets == 1 && slotMasks[2] == (EXTMAG | GRIP) && slotMasks[1] == 0);
    assert(camoSets == 1 && camos.clientNum == 5 && camos.weapon == THOMPSON && camos.camo == 6 && camos.ps == &client.ps);
    /* Into the first; a gun with nothing on it clears the slot's and paints nothing. */
    client.ps.weaponslots[1] = THOMPSON;
    client.ps.weaponslots[2] = COLT;
    slotMasks[1] = slotMasks[2] = 99;
    item.s.legsAnim = 0;
    item.s.torsoAnim = 0;
    G_TakeItemWeaponState(&picker, &item, THOMPSON);
    assert(slotSets == 2 && slotMasks[1] == 0 && slotMasks[2] == 99 && camoSets == 2 && camos.camo == 0);
    /* A weapon numbered past a signed byte is still found in its slot. */
    client.ps.weaponslots[1] = (char)LATE;
    item.s.legsAnim = (int)EXTMAG;
    G_TakeItemWeaponState(&picker, &item, LATE);
    assert(slotSets == 3 && slotMasks[1] == EXTMAG && camos.weapon == LATE);
    cases++;

    printf("%d\n", cases);
    return 0;
}
'''

mutants = [
    ('    return attachments ? BG_GetAttachedWeaponDef(weapon, attachments)->iClipSize\n',
     '    return 0 ? BG_GetAttachedWeaponDef(weapon, attachments)->iClipSize\n'),
    ('    return attachments ? BG_GetAttachedWeaponDef(weapon, attachments)->iMaxAmmo\n',
     '    return 0 ? BG_GetAttachedWeaponDef(weapon, attachments)->iMaxAmmo\n'),
    ('    return BG_GetItemWeaponAttachments(&item->s) & BG_AttachmentsAllowed(BG_GetWeaponDef(weapon)->szInternalName);',
     '    return BG_GetItemWeaponAttachments(&item->s);'),
    ('        if ((unsigned char)ps->weaponslots[slot] == weapon)\n            BG_SetSlotAttachments(ps, slot, attachments);',
     '        BG_SetSlotAttachments(ps, slot, attachments);'),
    ('    for (slot = 1; slot <= 2; slot++) {\n        if ((unsigned char)ps->weaponslots[slot] == weapon)',
     '    for (slot = 1; slot < 2; slot++) {\n        if ((unsigned char)ps->weaponslots[slot] == weapon)'),
    ('        if ((unsigned char)ps->weaponslots[slot] == weapon)\n            BG_SetSlotAttachments',
     '        if (ps->weaponslots[slot] == weapon)\n            BG_SetSlotAttachments'),
    ('    G_SetClientSlotCamos(other->s.number, ps, weapon, BG_GetItemWeaponCamo(&item->s));\n', ''),
]


def program(source):
    return support + ''.join(function(source, name) for name in HELPERS) + checks


with tempfile.TemporaryDirectory(prefix='cod2-dropped-weapon-state-') as directory:
    path = Path(directory)

    def run(source, name, strict):
        (path / f'{name}.c').write_text(program(source))
        warnings = ['-O1', '-Wall', '-Werror', '-Wno-unused-function'] if strict else ['-O0', '-w']
        subprocess.run(['cc', '-std=gnu99', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        *warnings, str(path / f'{name}.c'), '-o', str(path / name)], check=True)
        return subprocess.run([str(path / name)], capture_output=True, text=True)

    result = run(items, 'helpers', True)
    assert result.returncode == 0, result.stderr[-3000:]
    cases = int(result.stdout)
    for index, (old, new) in enumerate(mutants):
        assert items.count(old) == 1, old
        assert run(items.replace(old, new), f'mutant{index}', False).returncode != 0, f'mutant survived: {new!r}'

# Drop_Weapon writes the camouflage the dropper's configstring gives the gun and its slot's attachments into the
# item, before its ammunition leaves the player.
drop = function(items, 'Drop_Weapon')
write = drop.index('        BG_SetItemWeaponState(&pDrop->s, G_GetClientWeaponCamo(pEnt->s.number, iWeaponIndex),\n'
                   '                              BG_GetWeaponAttachments(ps, iWeaponIndex));\n')
assert drop.index('    pDrop = Drop_Item(pEnt, pWeapItem, 0.0f, 0);\n') < drop.index('    if (ps) {\n') < write
assert write < drop.index('        ps->ammo[iAmmoIndex] = 0;\n')
# Touch_Item reads what the gun carries once, clamps its reserve and clip by it, and once the weapon is the
# player's, gives it its state before filling the clip to what the player's attachments allow.
touch = function(items, 'Touch_Item')
weapon_case = touch[touch.index('    case IT_WEAPON: {'):]
weapon_case = weapon_case[:weapon_case.index('\n    default:')]
assert '        unsigned attachments = G_ItemWeaponAttachments(ent, weapon);\n' in weapon_case
# A gun the map places, with no attachments, still draws its ammunition from its own clip.
assert 'BG_GetAmmoTypeMax' not in weapon_case and weapon_case.count('BG_GetAmmoClipSize') == 1
assert '                int clipSize = BG_GetAmmoClipSize(BG_ClipForWeapon(weapon));\n' in weapon_case
assert weapon_case.count('G_ItemMaxAmmo(weapon, attachments)') == 2
assert weapon_case.count('G_ItemClipSize(weapon, attachments)') == 4
give = weapon_case.index('            G_GivePlayerWeapon(ps, weapon);\n            G_TakeItemWeaponState(other, ent, weapon);\n')
assert give < weapon_case.index('                if (clipAmmo > BG_GetPlayerClipSize(ps, weapon)) {', give)
assert weapon_case.count('BG_GetPlayerClipSize(ps, weapon)') == 3
print(f'PASS: {cases} cases: a dropped gun\'s clip and reserve by its attachments, the attachments its weapon takes, '
      f'given to the slot it lands in with its camouflage; {len(mutants)} mutants fail; Drop_Weapon and Touch_Item '
      f'write and read them where the ammunition is moved')
