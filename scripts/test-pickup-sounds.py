#!/usr/bin/env python3
"""The client's item pickup events (cg_event_mp.c): picking up a new gun plays its pickup sound; ammo taken
from a gun on the ground, as walking over one the player carries takes it, plays none; the player's own pickups
still select the weapon as the stock client does."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
events = (root / 'src/PC/cgame_mp/cg_event_mp.c').read_text()

head = '        case 0x90:\n        case 0x91: {'
assert events.count(head) == 1
start = events.index(head)
end, depth = events.index('{', start) + 1, 1
while depth:
    depth += (events[end] == '{') - (events[end] == '}')
    end += 1
block = events[start:end] + '\n'

support = r'''
#include <assert.h>
#include <string.h>
typedef struct { int number; int eventParm; } entityState_t;
typedef struct { int weapClass; int offhandClass; } WeaponDef;
#define SNAP_FLAGS 0x18
#define SNAP_PS_CLIENTNUM 0xd8
#define ITEMS 4
static int numItems = ITEMS;
static void *imp_bg_numItems = &numItems;
/* Each item's client info, nine ints: its pickup sound at 0x1c, its ammo pickup sound at 0x20. */
static int itemInfos[ITEMS][9];
static char *CG_ItemInfoBase(void) { return (char *)itemInfos; }
/* Each item, eleven ints: its type at 0x1c, its weapon at 0x20. */
static int items[ITEMS][11];
static char *BG_ItemListBase(void) { return (char *)items; }
static WeaponDef weaponDefs[8];
static void *BG_GetWeaponDef(int weapon) { return &weaponDefs[weapon]; }
static int sounds[8], soundCount, soundEntity = -1;
static int CG_PlayEntitySoundAlias(int entNum, int alias) {
    soundEntity = entNum;
    sounds[soundCount++] = alias;
    return 0;
}
static int selected = -1, equipped = -1;
static void CG_SelectWeaponIndex(int weapon) { selected = weapon; }
static void CG_SetEquippedOffHand(int weapon) { equipped = weapon; }
static char nextSnap[0x100];
static struct { char *nextSnap; int equippedOffHand; int weaponSelect; } cgState = { nextSnap }, *cg = &cgState;
static void Event(int event, entityState_t *es) {
    char *snap;
    switch (event) {
'''
checks = r'''
    }
}
static void reset(void) {
    soundCount = 0;
    soundEntity = -1;
    selected = equipped = -1;
}
int main(void) {
    entityState_t es = { 5, 2 };

    /* Item 2 is the Thompson, weapon 3: its pickup sound and its ammo's. */
    itemInfos[2][7] = 101;
    itemInfos[2][8] = 202;
    items[2][7] = 1;
    items[2][8] = 3;

    /* Picking it up plays its pickup sound on the player who did. */
    reset();
    Event(0x90, &es);
    assert(soundCount == 1 && sounds[0] == 101 && soundEntity == 5);
    /* Its ammo taken off the ground plays nothing. */
    reset();
    Event(0x91, &es);
    assert(soundCount == 0);
    /* Nor does an item out of range. */
    static const int outside[] = { -1, 0, ITEMS, ITEMS + 7 };
    for (unsigned i = 0; i < sizeof(outside) / sizeof(outside[0]); i++) {
        entityState_t bad = { 5, outside[i] };
        reset();
        Event(0x90, &bad);
        Event(0x91, &bad);
        assert(soundCount == 0 && selected == -1);
    }

    /* The viewer's own pickup with weapon selection pending selects the gun,
     * the ammo's as before; another player's does not. */
    *(int *)(nextSnap + SNAP_FLAGS) = 0x400000;
    *(int *)(nextSnap + SNAP_PS_CLIENTNUM) = 5;
    reset();
    Event(0x91, &es);
    assert(soundCount == 0 && selected == 3);
    reset();
    Event(0x90, &es);
    assert(soundCount == 1 && selected == 3);
    entityState_t other = { 6, 2 };
    reset();
    Event(0x90, &other);
    assert(soundCount == 1 && soundEntity == 6 && selected == -1);
    return 0;
}
'''

mutants = [
    ('            if (event == 0x90)\n', '            if (1)\n'),
    ('            if (event == 0x90)\n                CG_PlayEntitySoundAlias(es->number, *(int *)(itemData + 0x1c));\n',
     ''),
    ('*(int *)(itemData + 0x1c)', '*(int *)(itemData + 0x20)'),
    ('CG_PlayEntitySoundAlias(es->number, *(int', 'CG_PlayEntitySoundAlias(0, *(int'),
]
with tempfile.TemporaryDirectory(prefix='cod2-pickup-sounds-') as directory:
    for index, source in enumerate([block] + [block.replace(old, new) for old, new in mutants]):
        if index:
            assert block.count(mutants[index - 1][0]) == 1, mutants[index - 1][0]
        path = Path(directory) / f'test{index}.c'
        path.write_text(support + source + checks)
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wall', '-Werror' if not index else '-w', '-Wno-unused-function', str(path), '-o',
                        str(path.with_suffix(''))], check=True)
        result = subprocess.run([str(path.with_suffix(''))], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-1500:])
print(f'PASS: a new gun picked up plays its pickup sound on the player who took it; ammo taken off a gun on the '
      f'ground plays none; out-of-range items play nothing; the viewer\'s own pickups still select the gun; '
      f'{len(mutants)} mutants fail')
