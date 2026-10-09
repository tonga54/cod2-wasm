#!/usr/bin/env python3
"""Audit retail/derived stats and exercise actual server bullet/player damage."""
from pathlib import Path
import json
import re
import subprocess
import tempfile
import zipfile
from weapon_balance import apply_weapon_balance, load_profile, weapon_fields

root = Path(__file__).resolve().parent.parent
profile = load_profile()
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

changed = []
report = []
metrics = []
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
            'locHead', 'locTorsoUpper', 'boltAction', 'rechamberTime')
    report.append({'weapon': name.removeprefix('weapons/mp/'), 'retailSource': sources[name],
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
    metrics.append('{' + initializer(before) + ',' + initializer(after) + ',' +
                   str(int(bool(differences))) + ',' + str(int(name.endswith(('bren_mp', 'mp44_mp')))) + '}')

assert sorted(changed) == sorted(profile['weapons'])
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
body += function(weapon_source, 'Bullet_Fire_Extended')
support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef int hitLocation_t;
typedef unsigned char byte;
typedef float vec_t,vec3_t[3];
#define WEAPTYPE_BULLET 0
typedef struct {int weapType,bRifleBullet,armorPiercing,damage,minDamage;float fMaxDamageRange,fMinDamageRange,locationDamageMultipliers[19];} WeaponDef;
typedef struct {int noclip,ufo;struct {int connected;} sess;} gclient_t;
typedef struct entity {struct {int number,weapon,eType,eventParm,eventParm2,surfType,otherEntityNum;} s;gclient_t *client;int takedamage,flags,health,handler;} gentity_t,gentity_s;
typedef struct {float fraction,normal[3];int entityNum,surfaceFlags,contents,partGroup;} trace_t;
typedef struct {WeaponDef *weapDef;} weaponParms;
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
static gentity_t *G_TempEntity(const vec_t *v,int event) {assert(0);return 0;}
static byte DirToByte(const vec_t *v) {assert(0);return 0;}
static int Dvar_GetInt(const char *name) {return 0;}
static int OnSameTeam(gentity_t *a,gentity_t *b) {return 1;}
'''
checks = r'''
typedef struct {WeaponDef retail,balanced;int changed,rangedAutomatic;} Metric;
static int shot(WeaponDef *weapon,float distance,int loc) {
 activeWeapon=weapon;hitLocation=loc;receivedDamage=damageCalls=0;
 vec3_t start={0,0,0},end={distance*2,0,0};weaponParms wp={weapon};
 Bullet_Fire_Extended(entities,(gentity_s (*)[16])entities,start,end,1,0,&wp,entities,0);
 assert(damageCalls==(loc!=18));return receivedDamage;
}
int main(void) {
 entities[0].s.weapon=1;entities[1].takedamage=1;entities[1].client=&client;
 int cases=0;
 for(int w=0;w<sizeof(metrics)/sizeof(metrics[0]);w++) {
  Metric *m=metrics+w;int previous[19];for(int i=0;i<19;i++)previous[i]=10000;
  for(int distance=1;distance<=8192;distance+=31) for(int loc=0;loc<19;loc++) {
   int old=shot(&m->retail,distance,loc),now=shot(&m->balanced,distance,loc);
   assert(now<=old && now<=previous[loc]);previous[loc]=now;
   if(!m->changed)assert(now==old);
   if(m->changed && distance<100) {
    if(loc==1 || loc==2) {assert(now<100 && now>=50 && old>=100);}
    else assert(now==old);
   }
   cases++;
  }
  for(int side=-1;side<=1;side++) for(int loc=0;loc<19;loc++) {
   shot(&m->balanced,m->balanced.fMaxDamageRange+side,loc);
   shot(&m->balanced,m->balanced.fMinDamageRange+side,loc);
  }
  if(m->rangedAutomatic) {assert(shot(&m->balanced,3000,4)==28);assert(shot(&m->balanced,3000,2)==56);}
 }
 printf("PASS: %d original/balanced server bullet/player-damage cases, 19 hit locations, range boundaries, close body damage preserved, no automatic one-headshot kill\n",cases);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-weapon-balance-') as directory:
    path = Path(directory)
    table = 'static Metric metrics[]={' + ',\n'.join(metrics) + '};\n'
    checks = checks.replace('int main(void)', table + 'int main(void)')
    (path / 'test.c').write_text(support + body + checks)
    subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                    str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)

output = root / 'out/weapon-balance-audit.json'
output.parent.mkdir(exist_ok=True)
output.write_text(json.dumps({'profile': profile['id'], 'healthReference': 100,
    'note': 'Body/head damage uses the server range curve and hit multipliers. Shotgun damage is per pellet; turret file stats do not assert a player TTK.',
    'weapons': report}, indent=2) + '\n')
print(f'PASS: {len(report)} packaged weapon definitions audited; only {len(changed)} reviewed automatic definitions changed; retail mismatch rejected')
print(output)
