#!/usr/bin/env python3
"""A gun dropped with a camouflage and attachments keeps them: Drop_Weapon writes them into the item, and whoever
picks it up gets the attachments in the slot it lands in, the camouflage in their configstring, and the clip and
reserve the attachments give, as g_items_mp.c's helpers and its two call sites do.

The gun also keeps its rounds. Drop_Weapon leaves the reserve in count and the clip in item.count2, the field
CoD2MP_s.exe's Drop_Weapon writes and its Pickup_Weapon reads (0x1a8 past count's 0x1a0). Touch_Item's weapon case,
compiled here as it is, hands those rounds over; it read the clip from chain, which nothing writes, so the dead
player's clip was lost and the picker's was filled from the reserve."""
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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct { char weaponslots[8]; int weapons[8], weapon, ammo[2300], ammoclip[1300]; } playerState_t;
typedef struct { int number, legsAnim, torsoAnim; } entityState_t;
typedef struct { playerState_t ps; } gclient_t;
typedef struct gentity_s {
    entityState_t s;
    gclient_t *client;
    struct { float currentOrigin[3], currentAngles[3]; } r;
    int count, chain;
    struct { int count2; unsigned short index; } item;
    int tagInfo, parent;
} gentity_t;
typedef struct {
    const char *szInternalName;
    int iClipSize, iMaxAmmo, iDropAmmoMin, iDropAmmoMax, weapSlot, bSlotStackable;
    const char *szDisplayName;
} WeaponDef;
typedef struct { int giType, giTag; } gitem_t;
typedef struct { struct { int enabled; } current; } dvar_t;
enum { IT_BAD, IT_WEAPON };
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

/* What Touch_Item's weapon case calls. The taker has a free primary slot, so no gun of theirs is dropped. */
static const char *va(const char *format, ...) { return format; }
static void SV_GameSendServerCommand(int clientNum, int type, const char *text) { (void)clientNum; (void)type; (void)text; }
static float randomf(void) { return 0.0f; }
int BG_WeaponIsClipOnly(int weapon) { (void)weapon; return 0; }
static dvar_t ammoPools;
static const dvar_t *ammoPoolsDvar = &ammoPools;
#define imp_g_weaponAmmoPools (&ammoPoolsDvar)
static struct { int trigger; } scr_const;
static void Scr_AddUndefined(void) { }
static void Scr_AddEntity(gentity_t *ent) { (void)ent; }
static void Scr_Notify(gentity_t *ent, int name, int params) { (void)ent; (void)name; (void)params; }
int BG_DoesWeaponNeedSlot(int weapon) { (void)weapon; return 0; }
int BG_IsPlayerWeaponInSlot(const playerState_t *ps, int weapon, int slot) { (void)ps; (void)weapon; (void)slot; abort(); }
int BG_GetStackSlotForWeapon(const playerState_t *ps, int weapon, int slot) { (void)ps; (void)weapon; (void)slot; abort(); }
int BG_GetEmptySlotForWeapon(const playerState_t *ps, int weapon) { (void)ps; (void)weapon; abort(); }
static void Com_Printf(const char *format, ...) { (void)format; }
static gentity_t *Drop_Weapon(gentity_t *ent, int weapon, unsigned tag) { (void)ent; (void)weapon; (void)tag; abort(); }
static void G_SetOrigin(gentity_t *ent, const float *origin) { (void)ent; (void)origin; abort(); }
static void G_SetAngle(gentity_t *ent, const float *angles) { (void)ent; (void)angles; abort(); }
static void SV_LinkEntity(gentity_t *ent) { (void)ent; abort(); }
static void G_SelectWeaponIndex(int clientNum, int weapon) { (void)clientNum; (void)weapon; }
int G_GivePlayerWeapon(playerState_t *ps, int weapon)
{
    ps->weapons[weapon >> 5] |= 1 << (weapon & 31);
    ps->weaponslots[ps->weaponslots[1] ? 2 : 1] = (char)weapon;
    return 1;
}
int BG_GetPlayerClipSize(const playerState_t *ps, int weapon)
{
    int slot;
    for (slot = 1; slot <= 2; slot++) {
        if ((unsigned char)ps->weaponslots[slot] == weapon && slotMasks[slot])
            return BG_GetAttachedWeaponDef(weapon, slotMasks[slot])->iClipSize;
    }
    return defs[weapon].iClipSize;
}
/* Takes up to addCap rounds into the reserve; addTotal sums what it was offered. */
static int addCap, addTotal;
int Add_Ammo(gentity_t *ent, int weapon, int count, int fillClip)
{
    int added = count < addCap ? count : addCap;
    assert(!fillClip);
    addTotal += count;
    ent->client->ps.ammo[BG_AmmoForWeapon(weapon)] += added;
    return added;
}
'''

checks = r'''
static gclient_t taker;
static gentity_t takerEnt = { { 6, 0, 0 }, &taker };

/* A Thompson on the ground holding what Drop_Weapon gives it, for a taker with a Colt in their second slot and, if
 * carrying, a Thompson in their first. */
static gentity_t *gunOnGround(unsigned attachments, int count, int clip, int carrying)
{
    static gentity_t gun;

    memset(&gun, 0, sizeof(gun));
    gun.item.index = 1;
    gun.s.legsAnim = (int)attachments;
    gun.count = count;
    gun.item.count2 = clip;
    memset(&taker, 0, sizeof(taker));
    memset(slotMasks, 0, sizeof(slotMasks));
    taker.ps.weaponslots[2] = COLT;
    taker.ps.weapons[0] = 1 << COLT;
    if (carrying) {
        taker.ps.weaponslots[1] = THOMPSON;
        taker.ps.weapons[0] |= 1 << THOMPSON;
    }
    addCap = 1 << 30;
    addTotal = 0;
    return &gun;
}

#define CLIP taker.ps.ammoclip[BG_ClipForWeapon(THOMPSON)]
#define RESERVE taker.ps.ammo[BG_AmmoForWeapon(THOMPSON)]

int main(void)
{
    gclient_t client;
    gentity_t picker = { { 5, 0, 0 }, &client }, item = { { 140, 0, 0 }, NULL };
    gentity_t *gun;
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

    /* Dropped by a player killed with 23 rounds in its Extended Mag and 95 in reserve: the taker gets those, in a
     * slot with the magazine. Read from chain, the clip was lost and a full one taken from the reserve: 30 and 65. */
    gun = gunOnGround(EXTMAG, 95, 23, 0);
    PickUp(gun, &takerEnt, 0);
    assert(eventParm == 0x90 && CLIP == 23 && RESERVE == 95 && slotMasks[1] == EXTMAG);
    /* A full magazine and nothing in reserve, which gave an empty gun. */
    gun = gunOnGround(EXTMAG, -1, 30, 0);
    PickUp(gun, &takerEnt, 1);
    assert(eventParm == 0x90 && CLIP == 30 && RESERVE == 0);
    /* An empty magazine and 40 in reserve: the taker reloads, as its player would have. */
    gun = gunOnGround(EXTMAG, 40, -1, 0);
    PickUp(gun, &takerEnt, 0);
    assert(eventParm == 0x90 && CLIP == 0 && RESERVE == 40);
    /* More in the clip than the gun holds goes into the reserve. */
    gun = gunOnGround(0, 50, 30, 0);
    PickUp(gun, &takerEnt, 0);
    assert(eventParm == 0x90 && CLIP == 20 && RESERVE == 60 && slotMasks[1] == 0);
    cases++;

    /* A gun the map placed holds neither: it draws its dropAmmo rounds and fills its clip from them. */
    defs[THOMPSON].iDropAmmoMin = defs[THOMPSON].iDropAmmoMax = 60;
    gun = gunOnGround(0, 0, 0, 0);
    PickUp(gun, &takerEnt, 0);
    assert(eventParm == 0x90 && CLIP == 20 && RESERVE == 40 && gun->count == 40 && gun->item.count2 == 20);
    cases++;

    /* A taker carrying the gun takes its clip and reserve as ammunition. What they cannot carry stays in the gun,
     * from its reserve first. */
    gun = gunOnGround(0, 95, 17, 1);
    PickUp(gun, &takerEnt, 0);
    assert(eventParm == 0x91 && addTotal == 112 && RESERVE == 112);
    gun = gunOnGround(0, 95, 17, 1);
    addCap = 50;
    PickUp(gun, &takerEnt, 0);
    assert(eventParm == -1 && addTotal == 112 && gun->count == 45 && gun->item.count2 == 17);
    gun = gunOnGround(0, 95, 17, 1);
    addCap = 100;
    PickUp(gun, &takerEnt, 0);
    assert(eventParm == -1 && gun->count == -1 && gun->item.count2 == 12);
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
    ('        } else if (ent->item.count2 != 0) {\n            clipAmmo = ent->item.count2;\n',
     '        } else if (ent->chain != 0) {\n            clipAmmo = ent->chain;\n'),
    ('                    ent->item.count2 += ent->count;\n', '                    ent->chain += ent->count;\n'),
]


def pickup(source):
    touch = function(source, 'Touch_Item')
    case = touch[touch.index('    case IT_WEAPON: {'):]
    case = case[:case.index('\n    default:')]
    return ('static const gitem_t itemlist[] = { { IT_BAD, 0 }, { IT_WEAPON, THOMPSON } };\n'
            'static int eventParm;\n'
            '/* Touch_Item\'s weapon case as it is; eventParm left at -1 is a gun left on the ground. */\n'
            'static void PickUp(gentity_t *ent, gentity_t *other, int bTouched)\n{\n'
            '    playerState_t *ps = &other->client->ps;\n'
            '    const gitem_t *item = &itemlist[ent->item.index];\n'
            '    int clientNum = other->s.number;\n\n'
            '    eventParm = -1;\n'
            '    switch (item->giType) {\n' + case + '\n    }\n}\n')


def program(source):
    return support + ''.join(function(source, name) for name in HELPERS) + pickup(source) + checks


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
    # The server before this change: every clip read and written in chain.
    assert run(items.replace('ent->item.count2', 'ent->chain'), 'chain', False).returncode != 0, 'chain survived'

# Drop_Weapon writes the camouflage the dropper's configstring gives the gun and its slot's attachments into the
# item, before its ammunition leaves the player.
drop = function(items, 'Drop_Weapon')
write = drop.index('        BG_SetItemWeaponState(&pDrop->s, G_GetClientWeaponCamo(pEnt->s.number, iWeaponIndex),\n'
                   '                              BG_GetWeaponAttachments(ps, iWeaponIndex));\n')
assert drop.index('    pDrop = Drop_Item(pEnt, pWeapItem, 0.0f, 0);\n') < drop.index('    if (ps) {\n') < write
assert write < drop.index('        ps->ammo[iAmmoIndex] = 0;\n')
# A player's gun takes their reserve into count and their clip into item.count2, which Touch_Item reads; nothing
# in either uses chain.
assert write < drop.index('        iMin = ps->ammo[iAmmoIndex];\n') < drop.index('        iMax = ps->ammoclip[iClipIndex];\n')
assert '    pDrop->count = iMin ? iMin : -1;\n    pDrop->item.count2 = iMax ? iMax : -1;\n' in drop
assert '->chain' not in drop and '->chain' not in function(items, 'Touch_Item')
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
      f'given to the slot it lands in with its camouflage, and the rounds its player left in its clip and reserve '
      f'handed over by Touch_Item\'s weapon case; {len(mutants) + 1} mutants fail, the clip read from chain among '
      f'them; Drop_Weapon and Touch_Item write and read them where the ammunition is moved')
