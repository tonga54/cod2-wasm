#!/usr/bin/env python3
"""Exercise the server melee trace and player damage with every packaged weapon."""
from pathlib import Path
import re
import subprocess
import tempfile
import zipfile
from weapon_balance import weapon_fields

root = Path(__file__).resolve().parent.parent
weapon = (root / 'src/PC/game_mp/g_weapon_mp.c').read_text()
combat = (root / 'src/PC/game_mp/g_combat_mp.c').read_text()

def function(source, name):
    match = re.search(r'(?:static )?[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'

locations = ('None', 'Helmet', 'Head', 'Neck', 'TorsoUpper', 'TorsoLower',
             'RightArmUpper', 'LeftArmUpper', 'RightArmLower', 'LeftArmLower',
             'RightHand', 'LeftHand', 'RightLegUpper', 'LeftLegUpper',
             'RightLegLower', 'LeftLegLower', 'RightFoot', 'LeftFoot', 'Gun')
metrics = []
with zipfile.ZipFile(root / 'data/browser/main/cod2_browser_renderer.iwd') as archive:
    table = archive.read('info/mp_lochit_dmgtable').decode('ascii').split('\\')
    fallback = dict(zip(table[1::2], table[2::2]))
    for name in archive.namelist():
        if not name.startswith('weapons/mp/'):
            continue
        fields = weapon_fields(archive.read(name))
        damage = int(fields.get('meleeDamage', '0'))
        if damage == 0:
            continue
        multipliers = [fields.get('loc' + loc, '1') for loc in locations]
        metrics.append('{' + str(int(fields['weaponType'] != 'bullet')) + ',' +
                       str(damage) + ',{' + ','.join(multipliers) + '}}')
assert len(metrics) >= 20, 'Audit all packaged melee-capable weapons'

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int hitLocation_t,Bool;
typedef unsigned char byte;
typedef float vec_t,vec3_t[3],vec2_t[2];
#define WEAPTYPE_BULLET 0
typedef struct {int weapType,iMeleeDamage;float locationDamageMultipliers[19];} WeaponDef;
typedef struct {int noclip,ufo;struct {int connected;} sess;} gclient_t;
typedef struct entity {struct {int number,weapon,eventParm,otherEntityNum;} s;gclient_t *client;int takedamage,flags,health,handler;} gentity_t,gentity_s;
typedef struct {float fraction,normal[3];int entityNum,surfaceFlags,partGroup;} trace_t;
typedef struct {vec3_t muzzleTrace,forward,right,up;} weaponParms;
typedef struct {void (*pain)(gentity_t*,gentity_t*,int,const vec_t*,int,hitLocation_t);void (*die)(gentity_t*,gentity_t*,gentity_t*,int,int,int,const vec_t*,hitLocation_t,int);} entityHandler_t;
typedef struct {struct {int enabled;} current;} dvar_t;
typedef struct {int damage,death;} scr_const_t;
static gentity_t entities[1024],temp;
static gclient_t client={.sess={.connected=2}};
static void *imp_g_entities=entities,*imp_bulletPriorityMap,*imp_entityHandlers;
static dvar_t debug,*debugPtr=&debug;
static void *imp_g_debugDamage=&debugPtr;
static scr_const_t constants;
#define SCR_CONST() (&constants)
#define g_entities entities
static WeaponDef *activeWeapon;
static int hitLocation,receivedDamage,damageCalls,expectedMOD,expectedFlags,traceTarget=1,traceCalls,triggerDamage;
static float targetDistance=32;
static void *BG_GetWeaponDef(int weapon) {assert(weapon==1);return activeWeapon;}
static void Scr_PlayerDamage(gentity_t *t,gentity_t *i,gentity_t *a,int damage,int flags,int mod,int weapon,const vec_t *p,const vec_t *d,int hit,int offset) {
 assert(t==entities+1 && hit==hitLocation && mod==expectedMOD && flags==expectedFlags && weapon==1);
 receivedDamage=damage;damageCalls++;t->health-=damage;
}
static void Com_DPrintf(const char *fmt,...) {}
static void Com_Printf(const char *fmt,...) {assert(0);}
static void Scr_AddEntity(gentity_t *e) {assert(0);}
static void Scr_AddInt(int x) {assert(0);}
static void Scr_Notify(gentity_t *e,int name,int n) {assert(0);}
static float Vec3NormalizeTo(const float *v,float *out) {memcpy(out,v,sizeof(vec3_t));return 1;}
static void G_LocationalTrace(trace_t *tr,const vec_t *a,const vec_t *b,int n,int mask,unsigned char *p) {
 assert(mask==0x2802831 && n==0);traceCalls++;
 *tr=(trace_t){.fraction=(targetDistance<b[0] ? targetDistance/b[0] : 1),.entityNum=traceTarget,.normal={-1,0,0},.partGroup=hitLocation};
}
static void G_CheckHitTriggerDamage(gentity_t *a,vec_t *s,vec_t *e,int d,int mod) {assert(mod==7);triggerDamage=d;}
static gentity_t *G_TempEntity(const vec_t *v,int event) {assert(event==0xad || event==0xae);return &temp;}
static byte DirToByte(const vec_t *v) {return 0;}
'''
offsets = weapon[weapon.index('static vec2_t traceOffsets'):weapon.index('extern float floorf')]
body = ''.join(function(combat, n) for n in ('G_GetHitLocDamageMult', 'G_IsPlayerDamageable', 'G_Damage'))
body += offsets + function(weapon, 'Melee_Trace') + function(weapon, 'Weapon_Melee')
checks = r'''
static weaponParms wp={.forward={1,0,0},.right={0,1,0},.up={0,0,1}};
static void strike(void) {
 receivedDamage=damageCalls=traceCalls=0;expectedMOD=7;expectedFlags=0;
 Weapon_Melee((gentity_s (*)[16])entities,&wp,64,10,10);
}
int main(void) {
 entities[0].s.weapon=1;entities[1].takedamage=1;entities[1].client=&client;
 int cases=0;
 for(int w=0;w<sizeof(weapons)/sizeof(weapons[0]);w++) {
  activeWeapon=weapons+w;int expected=activeWeapon->iMeleeDamage>50 ? 50 : activeWeapon->iMeleeDamage;
  for(hitLocation=0;hitLocation<19;hitLocation++) for(int seed=0;seed<20;seed++) {
   srand(seed);entities[1].health=100;strike();
   assert(damageCalls==1 && receivedDamage==expected && entities[1].health==100-expected);
   assert(entities[1].health>0 && triggerDamage==expected && traceCalls==1 && temp.s.otherEntityNum==1);
   if(expected==50) {strike();assert(entities[1].health==0);}
   entities[1].health=expected;strike();assert(entities[1].health==0);
   cases++;
  }
  // The trace still decides reach, walls and damageable targets.
  hitLocation=4;targetDistance=80;strike();assert(!damageCalls && traceCalls==5);targetDistance=32;
  traceTarget=1022;strike();assert(!damageCalls);traceTarget=1;
  entities[1].takedamage=0;strike();assert(!damageCalls);entities[1].takedamage=1;
  client.noclip=1;strike();assert(!damageCalls);client.noclip=0;
  client.ufo=1;strike();assert(!damageCalls);client.ufo=0;
  client.sess.connected=1;strike();assert(!damageCalls);client.sess.connected=2;
  // Other damage keeps its existing multipliers and damage flags.
  for(int mod=1;mod<=8;mod++) for(hitLocation=0;hitLocation<19;hitLocation++) {
   float mult=activeWeapon->weapType==WEAPTYPE_BULLET ? activeWeapon->locationDamageMultipliers[hitLocation] : g_fHitLocDamageMult[hitLocation];
   int expectedDamage=mod==7 ? 40 : (int)(40*mult);
   expectedMOD=mod;expectedFlags=32;receivedDamage=damageCalls=0;
   G_Damage(entities+1,entities,entities,wp.forward,wp.muzzleTrace,40,32,mod,hitLocation,0);
   assert(receivedDamage==expectedDamage && damageCalls==(expectedDamage>0));
  }
 }
 // A weaker custom weapon is not promoted to 50 damage.
 WeaponDef weak={.iMeleeDamage=35};activeWeapon=&weak;hitLocation=2;entities[1].health=100;strike();assert(entities[1].health==65);
 printf("PASS: %d melee strikes across %zu packaged weapons and 19 hit locations; two-hit kills, wounded targets, trace/gates, MOD_MELEE and other damage preserved\n",cases,sizeof(weapons)/sizeof(weapons[0]));
}
'''
array = 'static WeaponDef weapons[]={' + ',\n'.join(metrics) + '};\n'
array += 'static float g_fHitLocDamageMult[19]={' + ','.join(
    fallback[re.sub(r'(?<!^)([A-Z])', r'_\1', loc).lower()].strip()
    for loc in locations) + '};\n'
variants = [body,
            body.replace('if (damage > 50)\n        damage = 50;', ''),
            body.replace('damage, 0, 7, tr.partGroup', 'damage + (rand() % 5), 0, 7, tr.partGroup'),
            body.replace('damage, 0, 7, tr.partGroup', 'damage, 7, 0, tr.partGroup'),
            body.replace('if (mod != 7)', 'if (1)')]
assert len(set(variants)) == len(variants)
with tempfile.TemporaryDirectory(prefix='cod2-melee-damage-') as directory:
    path = Path(directory)
    for index, variant in enumerate(variants):
        (path / 'test.c').write_text(support + array + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(path / 'test.c'), '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), result.stderr
        if index == 0:
            print(result.stdout, end='')
print('PASS: removing the cap, restoring random bonus, swapping MOD/flags or applying bullet multipliers all fail')
