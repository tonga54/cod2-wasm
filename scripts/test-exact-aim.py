#!/usr/bin/env python3
"""A shot leaves along the sights once the sight picture is up.

Runs the production BG_GetShotSpread with the weapon files' own numbers: a
scoped Kar98k after a run or a shot, an MP40 held down, a shotgun, through
the whole raise. With g_exactAim the spread fades from the hip's over the
raise's middle to none from 80%, where the WWII scope's glass is whole;
without it, or for pellets, CoD2's spread stays as it was. The server aims
with the dvar and the first-person tracer with the same rule.
"""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
weapons = (root / 'src/PC/bgame/bg_weapons.c').read_text()
game = (root / 'src/PC/game_mp/g_weapon_mp.c').read_text()
cgame = (root / 'src/PC/cgame_mp/cg_weapons.c').read_text()
main = (root / 'src/PC/game_mp/g_main_mp.c').read_text()
scope = (root / 'src/PC/cgame_mp/cg_wwiiscope_mp.c').read_text()


def function(text, name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;{]*?\)\n\{', text)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


fire = function(game, 'FireWeaponAntiLag')
assert ('spread = BG_GetShotSpread(&entity->client->ps, weapDef, minSpread, maxSpread, aimSpreadScale,\n'
        '                              g_exactAim && g_exactAim->current.enabled);') in fire
assert 'fAdsSpread' not in fire
assert ('spread = BG_GetShotSpread(ps, weapDef, minSpread, maxSpread, ps->aimSpreadScale * (1.0f / 255.0f), 1);'
        in cgame)
assert 'g_exactAim = Dvar_RegisterBool_mac("g_exactAim", 1, 0x1005);' in main
# The sight picture is whole where the scope's glass is.
assert '#define CG_WWIISCOPE_PRESENCE_START 0.5f' in scope and '#define CG_WWIISCOPE_PRESENCE_END 0.8f' in scope

rule = weapons[weapons.index('#define BG_SIGHTS_UP_START'):weapons.index('/* The spread a shot leaves with.')]
rule += function(weapons, 'BG_GetShotSpread')

code = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
typedef int qboolean;
enum { WEAPCLASS_RIFLE, WEAPCLASS_MG, WEAPCLASS_SMG, WEAPCLASS_SPREAD };
typedef struct { float fWeaponPosFrac; } playerState_t;
typedef struct { int weapClass; float fAdsSpread; } WeaponDef;
''' + rule + r'''
static float Retail(const playerState_t *ps, const WeaponDef *w, float lo, float hi, float scale)
{
    if (ps->fWeaponPosFrac == 1.0f)
        return w->fAdsSpread + (hi - w->fAdsSpread) * scale;
    return lo + (hi - lo) * scale;
}

int main(void)
{
    /* weapons/mp: kar98k_sniper_mp 3-6 degrees from the hip, none aimed;
     * mp40_mp 1.5-5 and 0.5 aimed; shotgun_mp 6 and 5.5 aimed. */
    const WeaponDef sniper = { WEAPCLASS_RIFLE, 0.0f }, mp40 = { WEAPCLASS_SMG, 0.5f }, shotgun = { WEAPCLASS_SPREAD, 5.5f };
    const struct { const WeaponDef *w; float lo, hi; } guns[] = { { &sniper, 3, 6 }, { &mp40, 1.5f, 5 }, { &shotgun, 6, 6 } };
    int checks = 0;

    for (unsigned g = 0; g < 3; g++) {
        for (int s = 0; s <= 20; s++) {
            float scale = s / 20.0f, previous = 1e9f;
            for (int f = 0; f <= 1000; f++) {
                playerState_t ps = { f / 1000.0f };
                float exact = BG_GetShotSpread(&ps, guns[g].w, guns[g].lo, guns[g].hi, scale, 1);
                float off = BG_GetShotSpread(&ps, guns[g].w, guns[g].lo, guns[g].hi, scale, 0);
                float hip = guns[g].lo + (guns[g].hi - guns[g].lo) * scale;
                /* Without the dvar, and for pellets, CoD2's spread exactly. */
                assert(off == Retail(&ps, guns[g].w, guns[g].lo, guns[g].hi, scale));
                if (guns[g].w->weapClass == WEAPCLASS_SPREAD) {
                    assert(exact == off);
                } else {
                    /* The hip's through the first half of the raise, none from
                     * 80% on, fading between without a step back up. */
                    if (ps.fWeaponPosFrac <= 0.5f) assert(exact == hip);
                    if (ps.fWeaponPosFrac >= 0.8f) assert(exact == 0.0f);
                    assert(exact <= hip && exact >= 0.0f && exact <= previous + 1e-6f);
                    if (ps.fWeaponPosFrac > 0.5f && ps.fWeaponPosFrac < 0.8f && hip > 0.0f)
                        assert(exact < hip && exact > 0.0f);
                    if (f == 650) assert(fabsf(exact - hip * 0.5f) < 1e-4f);
                }
                previous = exact;
                checks++;
            }
        }
    }
    /* What CoD2 did through the scope: a sniper fully aimed straight after a
     * run or its last shot left up to 6 degrees off the reticle, 83 units
     * at 20 m; an MP40 held down through its sights, 5 degrees. */
    {
        playerState_t aimed = { 1.0f };
        assert(BG_GetShotSpread(&aimed, &sniper, 3, 6, 1.0f, 0) == 6.0f);
        assert(fabsf(tanf(6.0f * 0.01745329f) * 787.4f - 82.8f) < 0.5f);
        assert(BG_GetShotSpread(&aimed, &mp40, 1.5f, 5, 1.0f, 0) == 5.0f);
        assert(BG_GetShotSpread(&aimed, &sniper, 3, 6, 1.0f, 1) == 0.0f);
        assert(BG_GetShotSpread(&aimed, &mp40, 1.5f, 5, 1.0f, 1) == 0.0f);
    }
    printf("%d\n", checks);
    return 0;
}
'''

mutants = {
    'sights up only at full aim': ('#define BG_SIGHTS_UP_END 0.8f', '#define BG_SIGHTS_UP_END 1.0f'),
    'a step at the threshold': ('    return hip * (1.0f - t * t * (3.0f - 2.0f * t));', '    return hip;'),
    'pellets made exact': ('    if (!exactAim || weapDef->weapClass == WEAPCLASS_SPREAD) {', '    if (!exactAim) {'),
    'the dvar ignored': ('    if (!exactAim || weapDef->weapClass == WEAPCLASS_SPREAD) {',
                         '    if (weapDef->weapClass == WEAPCLASS_SPREAD) {'),
    'the aimed spread kept': ('    if (t >= 1.0f)\n        return 0.0f;', '    if (t >= 1.0f)\n        return weapDef->fAdsSpread;'),
}

with tempfile.TemporaryDirectory(prefix='cod2-exact-aim-') as directory:
    path = Path(directory)
    for index, (name, change) in enumerate([('production', None)] + list(mutants.items())):
        variant = code
        if change:
            assert variant.count(change[0]) == 1, name
            variant = variant.replace(*change)
        (path / 'test.c').write_text(variant)
        flags = ['-Wall', '-Werror'] if not index else ['-w']
        subprocess.run(['cc', '-std=gnu99', '-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        *flags, str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True)
        if not index:
            assert result.returncode == 0, result.stdout + result.stderr
            checks = int(result.stdout.split()[-1])
        else:
            assert result.returncode != 0, f'mutant survived: {name}'

print(f'PASS: {checks} raises of a sniper, an MP40 and a shotgun: with g_exactAim a shot leaves along the sights from '
      f'80% of the raise, the hip\'s spread fading from half, pellets and g_exactAim 0 as CoD2 had them; the server '
      f'and the first-person tracer share the rule; {len(mutants)} mutants fail')
