#!/usr/bin/env python3
"""Audit retail/derived stats and exercise actual server bullet/player damage and aimed recoil."""
from math import sqrt
from pathlib import Path
import json
import re
import subprocess
import tempfile
import zipfile
from weapon_balance import apply_weapon_balance, free_recoil, load_profile, weapon_fields
from browser_explosives import WEAPONS as EXPLOSIVES
from browser_player_health import PLAYER_HEALTH

root = Path(__file__).resolve().parent.parent
profile = load_profile()
# Up close, at the players' health: the shots that kill to the upper torso and to the head. A 7.62x25mm round
# takes five to the body, 9x19mm, .45 ACP and .455 Webley four, the intermediate and full-power automatic rounds
# three; the bolt-action rifles kill with one to the body, the semi-automatic rifles, Colt and Webley with one to
# the head, the automatic weapons, Luger and TT-30 with two.
KILL_SHOTS = {
    'thompson_mp': (4, 2), 'greasegun_mp': (4, 2), 'mp40_mp': (4, 2), 'sten_mp': (4, 2),
    'ppsh_mp': (5, 2), 'pps42_mp': (5, 2), 'mp44_mp': (3, 2), 'bar_mp': (3, 2), 'bren_mp': (3, 2),
    'colt_mp': (4, 1), 'webley_mp': (4, 1), 'luger_mp': (4, 2), 'tt30_mp': (5, 2),
    'm1garand_mp': (2, 1), 'svt40_mp': (2, 1), 'g43_mp': (2, 1), 'm1carbine_mp': (3, 1),
    'kar98k_mp': (1, 1), 'enfield_mp': (1, 1), 'mosin_nagant_mp': (1, 1), 'springfield_mp': (1, 1),
    'kar98k_sniper_mp': (1, 1), 'enfield_scope_mp': (1, 1), 'mosin_nagant_sniper_mp': (1, 1),
}
# Up close, the damage of each cartridge; full-power rifle rounds by the action that fires them.
CARTRIDGE_DAMAGE = {'7.62x25mm': '30', '9x19mm': '35', '.45 ACP': '40', '.455 Webley': '40',
                    '7.92x33mm Kurz': '42', '.30 Carbine': '42'}
FULL_POWER_DAMAGE = {'automatic': '45', 'semi': '45', 'bolt': '90'}
# The Trench Gun's damage is a pellet's; the default weapon no class gives.
UNRATED = {'shotgun_mp', 'defaultweapon_mp'}
with zipfile.ZipFile(root / 'data/browser/main/cod2_browser_renderer.iwd') as archive:
    current = {n: archive.read(n) for n in archive.namelist() if n.startswith('weapons/mp/')}
original = {}
sources = {}
for path in sorted((root / 'data/main').glob('*.iwd')):
    with zipfile.ZipFile(path) as archive:
        for name in archive.namelist():
            if name in current:
                original[name] = archive.read(name)
                sources[name] = path.name
# The mine and C4 are built from the frag's file and audited with the
# explosives; every other definition has its retail original.
generated = {'weapons/mp/' + weapon for weapon in EXPLOSIVES}
assert current.keys() - original.keys() == generated
for name in generated:
    del current[name]

changed = []
report = []
metrics = []
balanced = {}
retail_defs = {}
hit_fields = ('locNone', 'locHelmet', 'locHead', 'locNeck', 'locTorsoUpper',
              'locTorsoLower', 'locRightArmUpper', 'locLeftArmUpper',
              'locRightArmLower', 'locLeftArmLower', 'locRightHand', 'locLeftHand',
              'locRightLegUpper', 'locLeftLegUpper', 'locRightLegLower',
              'locLeftLegLower', 'locRightFoot', 'locLeftFoot', 'locGun')
for name, data in sorted(current.items()):
    before = weapon_fields(original[name])
    expected = apply_weapon_balance(name, original[name], profile)
    assert data == expected, 'Derived asset differs from reviewed profile: ' + name
    after = weapon_fields(data)
    assert before.keys() == after.keys()
    differences = {k: [before[k], after[k]] for k in before if before[k] != after[k]}
    assert differences == profile['weapons'].get(name.removeprefix('weapons/mp/'), {}), name
    if differences:
        changed.append(name.removeprefix('weapons/mp/'))
    keys = ('weaponClass', 'weaponType', 'damage', 'minDamage', 'maxDamageRange',
            'minDamageRange', 'fireTime', 'clipSize', 'shotCount', 'reloadTime',
            'reloadEmptyTime', 'adsSpread', 'hipSpreadStandMin', 'hipSpreadMax',
            'locHead', 'locTorsoUpper', 'boltAction', 'rechamberTime', 'adsViewKickPitchMin',
            'adsViewKickPitchMax', 'adsViewKickYawMin', 'adsViewKickYawMax', 'adsViewKickCenterSpeed',
            'adsGunKickPitchMin', 'adsGunKickPitchMax')
    reference = profile['recoil']['weapons'].get(name.removeprefix('weapons/mp/'))
    report.append({'weapon': name.removeprefix('weapons/mp/'), 'retailSource': sources[name],
                   'cartridge': reference and reference['cartridge'],
                   'freeRecoilJoules': reference and round(free_recoil(reference)[1], 2),
                   'changes': differences, 'retail': {k: before[k] for k in keys if k in before},
                   'balanced': {k: after[k] for k in keys if k in after}})
    # Turrets use the activator's held weapon for hit-location selection in
    # this reconstruction. Report their file stats, but do not invent a TTK.
    if before.get('weaponType') != 'bullet' or before.get('weaponClass') in ('turret', 'non-player'):
        continue
    if not all(k in before for k in hit_fields):
        continue
    def initializer(fields):
        return '{0,0,0,' + ','.join(fields[k] for k in (
            'damage', 'minDamage', 'maxDamageRange', 'minDamageRange')) + ',{' + \
            ','.join(fields[k] for k in hit_fields) + '}}'
    automatic = before.get('semiAuto') != '1' and before.get('boltAction') != '1'
    reviewed = sum(1 << i for i, key in enumerate(hit_fields) if key in differences)
    balanced[name.removeprefix('weapons/mp/')] = after
    retail_defs[name.removeprefix('weapons/mp/')] = before
    far = int(after['minDamage']) if automatic and after['weaponClass'] == 'rifle' else 0
    metrics.append('{' + initializer(before) + ',' + initializer(after) + ',' +
                   str(int(bool(differences))) + ',' + str(far) + ',' +
                   str(int('damage' in differences)) + ',' + str(int(automatic)) + ',' + str(reviewed) + ',' +
                   ','.join(map(str, KILL_SHOTS.get(name.removeprefix('weapons/mp/'), (0, 0)))) + '}')
assert sorted(changed) == sorted(profile['weapons'])
assert set(KILL_SHOTS) == set(balanced) - UNRATED, sorted(set(KILL_SHOTS) ^ (set(balanced) - UNRATED))

# Congruence: weapons alike in class, damage, rate of fire, zoom and bullet
# differ only in magazines, reloads and handling, never in how they hurt.
model_fields = ('damage', 'minDamage', 'maxDamageRange', 'minDamageRange') + hit_fields
twins = {}
for name, fields in balanced.items():
    key = tuple(fields.get(k) for k in ('weaponClass', 'damage', 'fireTime', 'adsZoomFov', 'rifleBullet'))
    twins.setdefault(key, []).append(name)
for key, names in twins.items():
    assert len({tuple(balanced[name][k] for k in model_fields) for name in names}) == 1, names
assert any(len(names) > 1 for names in twins.values())
# Damage follows the cartridge: a round deals the same up close whatever fires it, a pistol what a
# submachine gun firing its round deals; full-power rifle rounds by the action that fires them.
reference = profile['recoil']['weapons']
assert set(reference) == set(KILL_SHOTS) | {'shotgun_mp'}, sorted(set(reference) ^ (set(KILL_SHOTS) | {'shotgun_mp'}))
for name in KILL_SHOTS:
    fields = balanced[name]
    action = 'bolt' if fields['boltAction'] == '1' else 'semi' if fields['semiAuto'] == '1' else 'automatic'
    expected = CARTRIDGE_DAMAGE.get(reference[name]['cartridge'], FULL_POWER_DAMAGE[action])
    assert fields['damage'] == expected, (name, fields['damage'], expected)
# The automatic weapons' rules: the helmet as the head. Pistol rounds fall to 10, in SMGs firing 700
# rounds a minute or more from 600 to 1,500 units and in slower ones from 700 to 2,000, with 0.5 degrees
# of aimed spread; the intermediate round falls to 32 from 1,200 and full-power rounds to 40 from 1,500,
# both by 2,500, with 0.2.
automatics = [name for name, fields in balanced.items() if fields['semiAuto'] != '1' and fields['boltAction'] != '1']
assert len(automatics) == 9, automatics
for name in automatics:
    fields = balanced[name]
    assert fields['locHead'] == fields['locHelmet'], name
    if fields['weaponClass'] == 'smg':
        expected = ('10', '600', '1500', '0.5') if 60 / float(fields['fireTime']) >= 700 else ('10', '700', '2000', '0.5')
    else:
        assert fields['weaponClass'] == 'rifle', name
        intermediate = reference[name]['cartridge'] == '7.92x33mm Kurz'
        expected = ('32', '1200', '2500', '0.2') if intermediate else ('40', '1500', '2500', '0.2')
    assert (fields['minDamage'], fields['maxDamageRange'], fields['minDamageRange'], fields['adsSpread']) == expected, name
# Refuse a different retail patch instead of silently changing the wrong field.
name = 'weapons/mp/sten_mp'
malformed = original[name].replace(b'\\locHead\\3', b'\\locHead\\4')
try:
    apply_weapon_balance(name, malformed, profile)
except ValueError:
    pass
else:
    raise AssertionError('Unexpected retail value accepted')

def function(source, name):
    match = re.search(r'(?:static )?[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'

weapon_source = (root / 'src/PC/game_mp/g_weapon_mp.c').read_text()
combat_source = (root / 'src/PC/game_mp/g_combat_mp.c').read_text()
body = ''.join(function(combat_source, n) for n in (
    'G_GetHitLocDamageMult', 'G_IsPlayerDamageable', 'G_Damage'))
body += ''.join(function(weapon_source, n) for n in (
    'Bullet_PenetrationDepth', 'Bullet_FindExit', 'Bullet_Fire_Extended'))
support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef int hitLocation_t;
typedef int qboolean;
typedef unsigned char byte;
typedef float vec_t,vec3_t[3];
#define WEAPTYPE_BULLET 0
#define WEAPCLASS_SPREAD 3
typedef struct {int weapType,bRifleBullet,armorPiercing,damage,minDamage;float fMaxDamageRange,fMinDamageRange,locationDamageMultipliers[19];int weapClass;} WeaponDef;
typedef struct {int noclip,ufo;struct {int connected;} sess;int ps;} gclient_t;
typedef struct entity {struct {int number,weapon,eType,eventParm,eventParm2,surfType,otherEntityNum;} s;gclient_t *client;int takedamage,flags,health,handler;} gentity_t,gentity_s;
typedef struct {float fraction,normal[3];int entityNum,surfaceFlags,contents,partGroup,startsolid,allsolid;} trace_t;
typedef struct {WeaponDef *weapDef;vec3_t muzzleTrace;} weaponParms;
typedef struct {void (*pain)(gentity_t*,gentity_t*,int,const vec_t*,int,hitLocation_t);void (*die)(gentity_t*,gentity_t*,gentity_t*,int,int,int,const vec_t*,hitLocation_t,int);} entityHandler_t;
typedef struct {struct {int enabled;} current;} dvar_t;
typedef struct {int damage,death;} scr_const_t;
static gentity_t entities[1024];
static gclient_t client={.sess={.connected=2}};
static void *imp_g_entities=entities,*imp_riflePriorityMap,*imp_bulletPriorityMap,*imp_entityHandlers,*imp_g_debugDamage;
static scr_const_t constants;
#define SCR_CONST() (&constants)
#define g_entities entities
static struct {int time;} level;
static WeaponDef *activeWeapon;
static float g_fHitLocDamageMult[19];
static int hitLocation,receivedDamage,damageCalls;
static void *BG_GetWeaponDef(int weapon) {assert(weapon==1);return activeWeapon;}
static void *BG_GetPlayerWeaponDef(const void *ps,int weapon) {return BG_GetWeaponDef(weapon);}
static void Scr_PlayerDamage(gentity_t *t,gentity_t *i,gentity_t *a,int damage,int flags,int mod,int weapon,const vec_t *p,const vec_t *d,int hit,int offset) {assert(t==entities+1 && hit==hitLocation);receivedDamage=damage;damageCalls++;}
static void Com_DPrintf(const char *fmt,...) {}
static void Com_Printf(const char *fmt,...) {assert(0);}
static void Scr_AddEntity(gentity_t *e) {assert(0);}
static void Scr_AddInt(int x) {assert(0);}
static void Scr_Notify(gentity_t *e,int name,int n) {assert(0);}
static float Vec3Normalize(float *v) {float n=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);assert(n>0);for(int i=0;i<3;i++)v[i]/=n;return n;}
static float Vec3NormalizeTo(const float *v,float *out) {memcpy(out,v,sizeof(vec3_t));return Vec3Normalize(out);}
static void G_LocationalTrace(trace_t *tr,const vec_t *a,const vec_t *b,int n,int mask,unsigned char *p) {*tr=(trace_t){.fraction=.5f,.entityNum=1,.normal={-1,0,0},.partGroup=hitLocation};}
static void G_CheckHitTriggerDamage(gentity_t *a,vec_t *s,vec_t *e,int d,int mod) {}
static void G_BulletHitsMines(gentity_t *a,const vec_t *s,const vec_t *e) {}
static gentity_t *G_TempEntity(const vec_t *v,int event) {assert(0);return 0;}
static byte DirToByte(const vec_t *v) {assert(0);return 0;}
static int Dvar_GetInt(const char *name) {return 0;}
static int OnSameTeam(gentity_t *a,gentity_t *b) {return 1;}
'''
checks = r'''
typedef struct {WeaponDef retail,balanced;int changed,farDamage,damageChanged,automatic,reviewed,bodyShots,headShots;} Metric;
static int shot(WeaponDef *weapon,float distance,int loc) {
 activeWeapon=weapon;hitLocation=loc;receivedDamage=damageCalls=0;
 vec3_t start={0,0,0},end={distance*2,0,0};weaponParms wp={weapon};
 Bullet_Fire_Extended(entities,(gentity_s (*)[16])entities,start,end,1,0,&wp,entities,0);
 assert(damageCalls==(loc!=18));return receivedDamage;
}
static int shotsToKill(int damage) {assert(damage>0);return (HEALTH+damage-1)/damage;}
int main(void) {
 entities[0].s.weapon=1;entities[1].takedamage=1;entities[1].client=&client;
 int cases=0;
 for(int w=0;w<sizeof(metrics)/sizeof(metrics[0]);w++) {
  Metric *m=metrics+w;int previous[19];for(int i=0;i<19;i++)previous[i]=10000;
  for(int distance=1;distance<=8192;distance+=31) for(int loc=0;loc<19;loc++) {
   int old=shot(&m->retail,distance,loc),now=shot(&m->balanced,distance,loc);
   /* No hit takes more of the players' health than retail's of its 100,
    * and none more further away. */
   assert(now*100<=old*HEALTH && now<=previous[loc]);previous[loc]=now;
   if(!m->changed)assert(now==old);
   if(m->changed && distance<100) {
    /* A reviewed hit location deals what it did not. */
    if(m->reviewed>>loc&1) assert(now!=old);
    /* Where the damage is reviewed, every hit deals its cartridge's. */
    else if(m->damageChanged) assert(loc==18 ? now==old : now!=old);
    else assert(now==old);
   }
   /* No automatic weapon kills with one headshot. */
   if(m->automatic && (loc==1 || loc==2)) assert(now<HEALTH);
   cases++;
  }
  for(int side=-1;side<=1;side++) for(int loc=0;loc<19;loc++) {
   shot(&m->balanced,m->balanced.fMaxDamageRange+side,loc);
   shot(&m->balanced,m->balanced.fMinDamageRange+side,loc);
  }
  /* The shots that kill up close, to the upper torso and to the head. */
  if(m->bodyShots) assert(shotsToKill(shot(&m->balanced,10,4))==m->bodyShots &&
                          shotsToKill(shot(&m->balanced,10,2))==m->headShots);
  if(m->farDamage) {assert(shot(&m->balanced,3000,4)==m->farDamage);assert(shot(&m->balanced,3000,2)==2*m->farDamage);}
 }
 printf("PASS: %d original/balanced server bullet/player-damage cases, 19 hit locations, range boundaries, no hit more of the players' %d health than of retail's 100, the shots that kill up close, no automatic one-headshot kill\n",cases,HEALTH);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-weapon-balance-') as directory:
    path = Path(directory)
    table = 'static Metric metrics[]={' + ',\n'.join(metrics) + '};\n'
    checks = f'#define HEALTH {PLAYER_HEALTH}\n' + checks.replace('int main(void)', table + 'int main(void)')
    (path / 'test.c').write_text(support + body + checks)
    subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                    str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)

# Aimed recoil follows real recoil. One aimed shot moves the sights, camera and gun together, by singleShot
# degrees per square root of the gun's free recoil energy; half a second of automatic fire climbs by
# automaticClimb degrees per square root of a joule per round a second. The production camera and gun
# springs run each gun's mean kick in 5 ms steps; a scope's sights follow the camera alone.
view_source = (root / 'src/PC/cgame_mp/cg_view_mp.c').read_text()
gun_source = (root / 'src/PC/bgame/bg_weapons.c').read_text()
recoil_scale = profile['recoil']


def recoil_table(definitions):
    rows = []
    for name, data in sorted(reference.items()):
        fields = definitions[name]
        value = lambda key: float(fields[key])
        automatic = fields['semiAuto'] != '1' and fields['boltAction'] != '1'
        interval = round(value('fireTime') * 1000)
        index = sqrt(free_recoil(data)[1]) * data['muzzleDevice']
        target = recoil_scale['automaticClimb'] * index * 1000 / interval if automatic else \
            recoil_scale['singleShot'] * index
        rows.append('{"%s",%d,%d,%d,%g,%g,%g,%g,%g,%g,%g,%g,%.6f}' % (
            name, automatic, not fields['adsOverlayShader'], interval,
            (value('adsViewKickPitchMin') + value('adsViewKickPitchMax')) / 2, value('adsViewKickCenterSpeed'),
            (value('adsGunKickPitchMin') + value('adsGunKickPitchMax')) / 2, value('adsGunKickAccel'),
            value('adsGunKickSpeedMax'), value('adsGunKickSpeedDecay'), value('adsGunKickStaticDecay'),
            value('gunMaxPitch'), target))
    return 'static const Recoil recoil[]={' + ',\n'.join(rows) + '};\n'


recoil_checks = r'''
#include <math.h>
#include <stdio.h>
typedef int qboolean;
typedef float vec_t;
''' + ''.join(line + '\n' for line in view_source.splitlines() if line.startswith('#define CG_KICK_')) + \
    function(view_source, 'CG_IntegrateViewKick') + \
    function(gun_source, 'BG_CalculateWeaponPosition_GunRecoil_SingleAngle') + r'''
typedef struct {const char *name;int automatic,iron,interval;float pitch,center,gunPitch,accel,speedMax,speedDecay,staticDecay,gunMax;double target;} Recoil;
TABLE
/* Degrees up: one aimed shot's peak, or where the shot fired after half a second of automatic fire goes. */
static double aimed(const Recoil *r) {
 float angles[3]={0},velocity[3]={0},offset=0,speed=0;
 double result=0;
 int shots=r->automatic ? 500/r->interval+1 : 1,fired=0;
 for(int t=0;t<3000;t+=5) {
  if(fired<shots && t>=fired*r->interval) {
   if(r->automatic) result=-(angles[0]+(r->iron ? offset : 0));
   velocity[0]=-r->pitch;speed+=r->gunPitch;fired++;
  }
  CG_IntegrateViewKick(angles,velocity,r->center,5);
  BG_CalculateWeaponPosition_GunRecoil_SingleAngle(&offset,&speed,0.005f,r->gunMax,r->accel,r->speedMax,r->speedDecay,r->staticDecay);
  if(!r->automatic && -(angles[0]+(r->iron ? offset : 0))>result) result=-(angles[0]+(r->iron ? offset : 0));
 }
 return result;
}
int main(void) {
 int off=0,count=sizeof(recoil)/sizeof(recoil[0]);
 for(int i=0;i<count;i++) {
  double got=aimed(recoil+i);
  if(fabs(got/recoil[i].target-1)>0.04) {printf("  %s: %.2f degrees, real recoil %.2f\n",recoil[i].name,got,recoil[i].target);off++;}
 }
 printf("%d of %d guns off their real recoil by more than 4%%\n",off,count);
 return off;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-weapon-recoil-') as directory:
    path = Path(directory)
    runs = []
    for definitions in (balanced, retail_defs):
        (path / 'recoil.c').write_text(recoil_checks.replace('TABLE', recoil_table(definitions)))
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(path / 'recoil.c'), '-lm', '-o', str(path / 'recoil')], check=True)
        runs.append(subprocess.run([str(path / 'recoil')], capture_output=True, text=True))
        assert not runs[-1].stderr, runs[-1].stderr
    # Every gun follows its real recoil; retail CoD2's own kicks miss it for every one.
    assert runs[0].returncode == 0, runs[0].stdout
    assert runs[1].returncode == len(reference), runs[1].stdout
    print(runs[0].stdout, end='')

output = root / 'out/weapon-balance-audit.json'
output.parent.mkdir(exist_ok=True)
output.write_text(json.dumps({'profile': profile['id'], 'healthReference': PLAYER_HEALTH,
    'note': 'Body/head damage uses the server range curve and hit multipliers. Shotgun damage is per pellet; turret file stats do not assert a player TTK.',
    'weapons': report}, indent=2) + '\n')
print(f'PASS: {len(report)} packaged weapon definitions audited; only {len(changed)} reviewed definitions changed; '
      f'damage follows the cartridge; alike weapons share one damage model; every automatic weapon follows its class '
      f'rules; aimed recoil of all {len(reference)} class weapons within 4% of their real free recoil, which retail '
      f'misses for every one; retail mismatch rejected')
print(output)
