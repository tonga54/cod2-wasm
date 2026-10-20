#!/usr/bin/env python3
"""The players' health: the five modes' patch, the engine's copy of the number and, with the owner's files, the
retail scripts it relies on."""
import re
import sys
import types
import zipfile
from pathlib import Path

root = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(root / 'scripts'))
import browser_player_health  # noqa: E402

SPAWN = b'\tself.maxhealth = 100;\r\n\tself.health = self.maxhealth;\r\n'
HEAL = b'\t\t\t\tplayers[i].maxhealth = 100;\r\n\t\t\t\tplayers[i].health = players[i].maxhealth;\r\n'


def check(module):
    """Each mode's players at 105 as they spawn, and HQ's as it heals them; nothing else changed."""
    retail = {f'maps/mp/gametypes/{gametype}.gsc': b'spawnPlayer()\r\n{\r\n' + SPAWN + b'}\r\n'
              for gametype in module.GAMETYPES}
    assert set(retail) == {f'maps/mp/gametypes/{gametype}.gsc' for gametype in ('ctf', 'dm', 'hq', 'sd', 'tdm')}
    retail['maps/mp/gametypes/hq.gsc'] += HEAL + HEAL
    for name, data in retail.items():
        patched = module.apply_player_health(name, data)
        assert patched == data.replace(b'maxhealth = 100;', b'maxhealth = 105;'), name
        for unexpected in (data.replace(b'maxhealth = 100;', b'maxhealth = 90;', 1), data + SPAWN):
            try:
                module.apply_player_health(name, unexpected)
            except ValueError:
                continue
            raise AssertionError('unexpected retail health accepted: ' + name)
    for name in ('maps/mp/gametypes/_healthoverlay.gsc', 'maps/mp/mp_toujane.gsc', 'weapons/mp/kar98k_mp'):
        assert module.apply_player_health(name, SPAWN) == SPAWN, name


check(browser_player_health)
# The engine's copy: High Caliber keeps an automatic weapon's headshot a point short of it, and two butt strikes
# take it.
header = (root / 'src/headers/PC/bgame/bg_player_health.h').read_text()
assert re.search(r'^#define BG_PLAYER_HEALTH (\d+)$', header, re.M).group(1) == str(browser_player_health.PLAYER_HEALTH)

MUTANTS = [
    ("RETAIL_COUNTS = {'maps/mp/gametypes/hq.gsc': 3}", "RETAIL_COUNTS = {}"),
    ("    if original.count(RETAIL_HEALTH) != RETAIL_COUNTS.get(name, 1):\n"
     "        raise ValueError('Unexpected retail health in ' + name)\n", ""),
    ("    if name not in SCRIPTS:\n        return original\n", ""),
    ("GAMETYPES = ('ctf', 'dm', 'hq', 'sd', 'tdm')", "GAMETYPES = ('ctf', 'dm', 'hq', 'tdm')"),
    ("PLAYER_HEALTH = 105", "PLAYER_HEALTH = 150"),
]
source = (root / 'scripts/browser_player_health.py').read_text()
for old, new in MUTANTS:
    assert source.count(old) == 1, old
    mutant = types.ModuleType('mutant')
    exec(compile(source.replace(old, new), 'browser_player_health.py', 'exec'), mutant.__dict__)
    try:
        check(mutant)
    except (AssertionError, ValueError):
        continue
    raise AssertionError('mutant survived: ' + new)

# With the owner's files: the regeneration takes the health a player has at "spawned_player" as full, and every
# mode sets it before that.
owner = sorted((root / 'data/main').glob('*.iwd'))
if owner:
    index = {}
    for path in owner:
        archive = zipfile.ZipFile(path)
        for name in archive.namelist():
            index[name.lower()] = (archive, name)
    archive, name = index['maps/mp/gametypes/_healthoverlay.gsc']
    overlay = archive.read(name)
    assert b'self waittill("spawned_player");\r\n\t\tself thread playerHealthRegen();' in overlay
    assert b'\tmaxhealth = self.health;' in overlay
    for script in sorted(browser_player_health.SCRIPTS):
        archive, name = index[script]
        data = archive.read(name)
        browser_player_health.apply_player_health(script, data)
        spawn = data[data.index(b'\nspawnPlayer()'):]
        spawn = spawn[:spawn.index(b'\n}')]
        assert spawn.index(b'\tself.maxhealth = 100;') < spawn.index(b'self notify("spawned_player");'), script

print(f'PASS: players at {browser_player_health.PLAYER_HEALTH} in the five modes and as HQ heals them, unexpected '
      f'retail refused, the engine\'s copy alike; {len(MUTANTS)} mutants fail' +
      ('; the owner\'s modes set it before the regeneration reads it' if owner else '; no owner files'))
