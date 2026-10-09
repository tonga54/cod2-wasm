#!/usr/bin/env python3
"""Exercise the actual frame-state transitions and player damage eligibility."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
active = (root / 'src/PC/game_mp/g_active_mp.c').read_text()
combat = (root / 'src/PC/game_mp/g_combat_mp.c').read_text()

def function(source, signature):
    start = source.index(signature + '\n{')
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'

# Stop after the state decisions, before animation, physics and HUD updates.
frame = function(active, 'void ClientEndFrame(gentity_t *ent)')
frame = frame[:frame.index('#ifdef PM_STATETRACE')] + '}\n'
spectator = function(active, 'void SpectatorClientEndFrame(gentity_t *ent)')
spectator = spectator[:spectator.index('    if (client->sess.forceSpectatorClient')] + '}\n'
intermission = function(active, 'static void G_ClientEndFrameIntermission(gentity_t *ent, gclient_t *client)')
eligible = function(combat, 'static int G_IsPlayerDamageable(gentity_t *targ)')
support = r'''
#include <assert.h>
#include <string.h>
typedef int qboolean;
typedef int playerState_t, clientState_t;
typedef float vec3_t[3];
enum { CON_CONNECTED=2, SESS_STATE_PLAYING=0, SESS_STATE_DEAD=1,
       SESS_STATE_SPECTATOR=2, SESS_STATE_INTERMISSION=3 };
typedef struct {
 struct { int deltaTime,pm_flags,pm_type,eFlags,damageCount,clientNum,viewmodelIndex;
          vec3_t origin,viewangles; } ps;
 struct { int connected,sessionState,viewmodelIndex; } sess;
 int buttonsSinceLastFrame,dropWeaponTime,compassPingTime,noclip,ufo;
 float fGunPitch,fGunYaw;
} gclient_t;
typedef struct { struct { int number,eType; } s;
 struct { int svFlags,contents; } r;
 gclient_t *client; int handler,active,takedamage,count,tagInfo;
} gentity_t;
typedef struct { int dummy; } clientInfo_t;
static struct { int time,teamScores[3]; } level;
static clientInfo_t ci;
static int spawns;
static clientInfo_t *G_ClientInfoForEntity(gentity_t *e) { return &ci; }
static int G_UpdateClientInfoModel(gentity_t *e,gclient_t *c,clientInfo_t *i) { return 0; }
static const char *va(const char *fmt,int n) { return "0"; }
static void SV_SetConfigstring(int n,const char *s) {}
static void ClientSpawn(gentity_t *e,const float *p,const float *a) { spawns++; }
static void G_SetClientContents(gentity_t *e) {}
'''
checks = r'''
int main(void) {
 for(int n=0;n<64;n++) for(int cycle=0;cycle<100;cycle++) {
  gclient_t c={0}; gentity_t e={.client=&c,.active=77};
  e.s.number=c.ps.clientNum=n;c.sess.connected=CON_CONNECTED;
  c.sess.viewmodelIndex=19;c.ps.damageCount=35;
  c.sess.sessionState=SESS_STATE_PLAYING;
  ClientEndFrame(&e);assert(e.takedamage && G_IsPlayerDamageable(&e));
  assert(c.ps.viewmodelIndex==19 && c.ps.damageCount==35);
  assert(e.active==77);
  c.noclip=1;ClientEndFrame(&e);assert(!G_IsPlayerDamageable(&e));
  c.noclip=0;c.ufo=1;ClientEndFrame(&e);assert(!G_IsPlayerDamageable(&e));
  c.ufo=0;
  c.sess.sessionState=SESS_STATE_DEAD;ClientEndFrame(&e);
  assert(!e.takedamage && !G_IsPlayerDamageable(&e));
  c.sess.sessionState=SESS_STATE_PLAYING;ClientEndFrame(&e);
  assert(G_IsPlayerDamageable(&e));
  c.sess.sessionState=SESS_STATE_SPECTATOR;ClientEndFrame(&e);
  assert(!e.takedamage && !G_IsPlayerDamageable(&e));
  c.sess.sessionState=SESS_STATE_PLAYING;ClientEndFrame(&e);
  assert(G_IsPlayerDamageable(&e));
  c.sess.sessionState=SESS_STATE_INTERMISSION;ClientEndFrame(&e);
  assert(!e.takedamage && !G_IsPlayerDamageable(&e));
  c.sess.sessionState=SESS_STATE_PLAYING;ClientEndFrame(&e);
  assert(G_IsPlayerDamageable(&e));
  c.sess.connected=0;assert(!G_IsPlayerDamageable(&e));
  ClientEndFrame(&e);assert(e.active==77);
 }
 assert(spawns==0);return 0;
}
'''
body = spectator + intermission + frame + eligible
with tempfile.TemporaryDirectory(prefix='cod2-player-damage-') as directory:
    path = Path(directory)
    mutant = body.replace('ent->takedamage = 1;', 'ent->active = 1;')
    wrong_viewmodel = body.replace('client->ps.viewmodelIndex = client->sess.viewmodelIndex;', 'client->ps.damageCount = client->sess.viewmodelIndex;')
    for index, variant in enumerate((body, mutant, wrong_viewmodel)):
        (path / 'test.c').write_text(support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(path / 'test.c'), '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True)
        assert (result.returncode == 0) == (index == 0), result.stderr.decode()
print('PASS: 6400 player lifecycle transitions; live/dead/spectator/intermission and noclip/ufo eligibility; old state assignment fails')
