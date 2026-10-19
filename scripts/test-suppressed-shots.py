#!/usr/bin/env python3
"""Exercise the real client fire path for suppressed and ordinary shots."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
weapons_path = root / 'src/PC/cgame_mp/cg_weapons.c'


def function(source, signature):
    start = source.index(signature + '\n{')
    end = start + len(signature) + 2
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


def extract(source):
    cache = source[source.index('#define CG_SUPPRESSED_ALIAS_COUNT'):source.index(
        'static const snd_alias_t *CG_SuppressedAlias(')]
    return (cache + function(source, 'static const snd_alias_t *CG_SuppressedAlias(const snd_alias_t *alias)') +
            function(source, 'void CG_FireWeapon(centity_t *cent, int event, int barrel)'))


support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int qboolean;
typedef float vec_t;
typedef vec_t vec3_t[3];
typedef enum { SASYS_UI, SASYS_CGAME, SASYS_GAME } snd_alias_system_t;
typedef struct {
    const char *pszAliasName, *pszSecondaryAliasName;
    float fVolMin, fVolMax, fDistMin, fDistMax;
    int flags, priority;
} snd_alias_t;
typedef struct { snd_alias_t *head; } snd_alias_list_t;
typedef struct { int number, eType, weapon, eventParm; } entityState_t;
typedef struct { entityState_t nextState; int bMuzzleFlash; } centity_t;
typedef struct { snd_alias_list_t *flashSound, *flashSoundPlayer, *lastShotSound, *lastShotSoundPlayer; } weaponInfo_t;
typedef struct { int bBoltAction; } WeaponDef;

static weaponInfo_t weaponInfos[4], *weaponInfoTable = weaponInfos;
#define imp_cg_weapons ((void *)&weaponInfoTable)
static WeaponDef weaponDefs[4];
static struct { int time, predictedPlayerState; vec3_t vGunSpeed, kickAVel; } cgStorage, *cg = &cgStorage;
static int localNumber, recoils, effects, brass, loudPlays, quietPlays, pings, pingDuration;
/* The browser's sample effect, as the client sets it, and at each play. */
static int sampleEffect, quietEffect = -1, loudEffect = -1;
int s_pendingLocalFireWeapon, s_pendingLocalFireBarrel, s_pendingLocalFireTime;
void WebAudio_SetSampleEffect(int effect) { sampleEffect = effect; }
static const snd_alias_list_t *loudList;
static const snd_alias_t *quietAlias;

static int CG_IsLocalClientEntity_inl(void *game, const entityState_t *ent)
{
    assert(game == cg);
    return ent->number == localNumber;
}
static int BG_GetNumWeapons(void) { return 3; }
static void Com_Error(int code, const char *message) { (void)code; (void)message; abort(); }
static void *BG_GetWeaponDef(int weapon) { return &weaponDefs[weapon]; }
static void BG_WeaponFireRecoil(int *ps, vec_t *speed, vec_t *kick)
{
    assert(ps == &cg->predictedPlayerState && speed == cg->vGunSpeed && kick == cg->kickAVel);
    recoils++;
}
static void CG_PlayFireWeaponEffect(centity_t *cent, entityState_t *ent, int barrel)
{
    assert(ent == &cent->nextState && !barrel);
    effects++;
}
static void CG_GetFireWeaponSoundOrigin(centity_t *cent, entityState_t *ent, weaponInfo_t *info, vec_t *origin)
{
    (void)cent; (void)ent; (void)info;
    origin[0] = origin[1] = origin[2] = 1.0f;
}
static snd_alias_t *Com_PickSoundAliasFromList(snd_alias_list_t *list) { return list->head; }
static int SND_PlaySoundAlias(const snd_alias_t *alias, int entnum, const vec_t *origin, int timeshift,
                              snd_alias_system_t system)
{
    assert(entnum == 5 && origin[0] == 1.0f && !timeshift && system == SASYS_CGAME);
    quietAlias = alias;
    quietEffect = sampleEffect;
    quietPlays++;
    return 0;
}
static int CG_PlaySoundAlias(int entnum, const vec_t *origin, snd_alias_list_t *list)
{
    assert(entnum == 5 && origin[0] == 1.0f);
    loudList = list;
    loudEffect = sampleEffect;
    loudPlays++;
    return 321;
}
static void CG_CompassAddWeaponPingInfo(centity_t *cent, const vec_t *origin, int duration)
{
    (void)cent;
    assert(origin[0] == 1.0f);
    pingDuration = duration;
    pings++;
}
static void CG_EjectWeaponBrass(entityState_t *ent, int event) { (void)ent; (void)event; brass++; }
'''

checks = r'''
#define NEAR(a, b) (fabsf((a) - (b)) <= 1e-5f * (1.0f + fabsf(b)))
static snd_alias_t aliases[4][4];
static snd_alias_list_t lists[4][4];

static centity_t Fire(int eType, int weapon, int event, int parm)
{
    centity_t cent = { { 5, eType, weapon, parm }, -1 };

    recoils = effects = brass = loudPlays = quietPlays = pings = pingDuration = 0;
    loudList = NULL;
    quietAlias = NULL;
    quietEffect = loudEffect = -1;
    CG_FireWeapon(&cent, event, 0);
    assert(sampleEffect == 0);
    return cent;
}

int main(void)
{
    static const int events[] = { 0x9e, 0x9f, 0xa0, 0xaf, 0xb0 };
    const snd_alias_t *first;
    snd_alias_t reloaded;
    centity_t cent;
    int e, type, parm, local, weapon;

    /* Each weapon's world, player, last-shot and player last-shot sounds. */
    for (weapon = 1; weapon <= 3; weapon++) {
        for (e = 0; e < 4; e++) {
            aliases[weapon][e] = (snd_alias_t){ "weap_fire", "weap_fire_distant", 0.8f + 0.01f * e, 1.0f,
                                                120.0f + weapon, 4000.0f + e, weapon * 10 + e, 7 };
            lists[weapon][e].head = &aliases[weapon][e];
        }
        weaponInfos[weapon] = (weaponInfo_t){ &lists[weapon][0], &lists[weapon][1], &lists[weapon][2], &lists[weapon][3] };
    }
    weaponDefs[2].bBoltAction = 1;

    /* Only a player's ordinary or last shot marked by player movement is quiet. */
    for (e = 0; e < 5; e++)
        for (type = 0; type < 2; type++)
            for (parm = 0; parm < 3; parm++)
                for (local = 0; local < 2; local++)
                    for (weapon = 1; weapon <= 2; weapon++) {
                        int eType = type ? 1 : 11, event = events[e];
                        int suppressed = eType == 1 && (event == 0x9e || event == 0xa0) && parm == 1;
                        const snd_alias_list_t *expected = &lists[weapon][(event == 0xa0 ? 2 : 0) + local];
                        snd_alias_t before = *expected->head;

                        localNumber = local ? 5 : 6;
                        cent = Fire(eType, weapon, event, parm);
                        assert(cent.bMuzzleFlash == !suppressed);
                        assert(effects == 1 && recoils == local && brass == (weapon != 2));
                        assert(!memcmp(expected->head, &before, sizeof(before)));
                        if (suppressed) {
                            assert(quietPlays == 1 && !loudPlays && !pings && quietAlias != expected->head);
                            assert(quietAlias->pszAliasName == before.pszAliasName && !quietAlias->pszSecondaryAliasName);
                            assert(NEAR(quietAlias->fVolMin, before.fVolMin * 0.35f) &&
                                   NEAR(quietAlias->fVolMax, before.fVolMax * 0.35f));
                            assert(NEAR(quietAlias->fDistMin, before.fDistMin * 0.33f) &&
                                   NEAR(quietAlias->fDistMax, before.fDistMax * 0.33f));
                            assert(quietAlias->flags == before.flags && quietAlias->priority == before.priority);
                            assert(quietEffect == 1 && loudEffect == -1);
                        } else {
                            assert(!quietPlays && loudPlays == 1 && loudList == expected && loudEffect == 0);
                            assert(pings == (eType == 1) && pingDuration == (eType == 1 ? 321 : 0));
                        }
                    }

    /* One quiet copy per sound: again it is reused, a reloaded copy of the same
     * alias finds it, and a changed alias gets its own. */
    localNumber = 6;
    Fire(1, 1, 0x9e, 1);
    first = quietAlias;
    Fire(1, 1, 0x9e, 1);
    assert(quietAlias == first);
    reloaded = aliases[1][0];
    lists[1][0].head = &reloaded;
    Fire(1, 1, 0x9e, 1);
    assert(quietAlias == first);
    reloaded.fVolMax = 0.5f;
    Fire(1, 1, 0x9e, 1);
    assert(quietAlias != first && NEAR(quietAlias->fVolMax, 0.175f) && NEAR(first->fVolMax, 0.35f));
    lists[1][0].head = &aliases[1][0];

    /* Nothing to play, quiet or not. */
    lists[3][0].head = NULL;
    cent = Fire(1, 3, 0x9e, 1);
    assert(!cent.bMuzzleFlash && !quietPlays && !loudPlays && !pings && brass == 1);
    weaponInfos[3].flashSound = NULL;
    cent = Fire(1, 3, 0x9e, 0);
    assert(cent.bMuzzleFlash == 1 && !quietPlays && !loudPlays && !pings);
    return 0;
}
'''

mutants = [
    ('ent->eventParm == 1;', 'ent->eventParm != 0;'),
    ('suppressed = ent->eType == 1 && (event == 0x9e', 'suppressed = (event == 0x9e'),
    ('(event == 0x9e || event == 0xa0) && ent->eventParm == 1', 'event == 0x9e && ent->eventParm == 1'),
    ('    cent->bMuzzleFlash = !suppressed;', '    cent->bMuzzleFlash = 1;'),
    ('    s_suppressedAliases[i].quiet = *alias;\n', ''),
    ('    s_suppressedAliases[i].quiet.fVolMax *= 0.35f;\n', ''),
    ('    s_suppressedAliases[i].quiet.fDistMax *= 0.33f;\n', ''),
    ('    s_suppressedAliases[i].quiet.pszSecondaryAliasName = NULL;\n', ''),
    ('!memcmp(&s_suppressedAliases[i].source, alias, sizeof(*alias))',
     's_suppressedAliases[i].source.pszAliasName == alias->pszAliasName'),
    ('    for (i = 0; i < CG_SUPPRESSED_ALIAS_COUNT; i++) {', '    for (i = 0; i < 0; i++) {'),
    ('SND_PlaySoundAlias(CG_SuppressedAlias(alias), ent->number', 'SND_PlaySoundAlias(alias, ent->number'),
    ('SND_PlaySoundAlias(CG_SuppressedAlias(alias), ent->number, origin, 0, SASYS_CGAME);',
     'SND_PlaySoundAlias(CG_SuppressedAlias(alias), ent->number, origin, 0, SASYS_CGAME), '
     'CG_CompassAddWeaponPingInfo(cent, origin, 0);'),
    ('                WebAudio_SetSampleEffect(WEB_AUDIO_EFFECT_SUPPRESSED);\n', ''),
    ('                WebAudio_SetSampleEffect(WEB_AUDIO_EFFECT_NONE);\n', ''),
]

# The browser driver gives every sample begun meanwhile the effect, as the 2D and 3D starts both initialise one.
driver = (root / 'downstream/wasm/web_audio.c').read_text()
assert 'void WebAudio_SetSampleEffect(int effect){webSampleEffect=effect;}' in driver
assert 'void AIL_init_sample(WebSound *s){if(!s)return;web_audio_reset(s->id);web_audio_effect(s->id,webSampleEffect);' in driver
assert 'int AIL_set_3D_sample_info(WebSound *s,const AILSOUNDINFO *info){if(!s||!info)return 0;AIL_init_sample(s);' in driver
assert 'EM_JS(void, web_audio_effect, (int id, int effect), { Module.cod2Audio.effect(id,effect); });' in driver

with tempfile.TemporaryDirectory(prefix='cod2-suppressed-shots-') as tmp:
    path = Path(tmp)
    source = weapons_path.read_text()
    # The browser build's path, with its sample effect.
    flags = ['-std=gnu99', '-D__EMSCRIPTEN__', '-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    (path / 'test.c').write_text(support + extract(source) + checks)
    subprocess.run(['cc', *flags, '-O1', '-g', '-Wall', '-Werror', str(path / 'test.c'), '-lm', '-o',
                    str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
    for index, (before, after) in enumerate(mutants):
        assert source.count(before) == 1, before
        (path / f'mutant{index}.c').write_text(support + extract(source.replace(before, after)) + checks)
        subprocess.run(['cc', *flags, '-O0', '-w', str(path / f'mutant{index}.c'), '-lm', '-o',
                        str(path / f'mutant{index}')], check=True)
        result = subprocess.run([str(path / f'mutant{index}')], capture_output=True)
        assert result.returncode != 0, f'mutant survived: {after!r}'
print(f'PASS: 120 fire events, the quiet copies, their cache and the browser\'s suppressor effect around each '
      f'quiet one in the real CG_FireWeapon; '
      f'{len(mutants)} mutants fail')
