#!/usr/bin/env python3
"""Replay real spectator state copying, sprint carry and packet bit I/O."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent


def function(path, name):
    source = (root / path).read_text()
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


net = (root / 'src/PC/qcommon/msg_mp.c').read_text()
bits = int(re.search(r'\{ PSF\(pm_flags\), (\d+) \}', net)[1])
assert bits == 28, 'Sprint must survive playerstate snapshots and archives'
header = (root / 'src/headers/cod2_sprint.h').read_text()
flag = int(re.search(r'#define PMF_SPRINT (0x\w+)', header)[1], 16)
assert not flag & 0x07ffffff and flag < (1 << bits)
active = function('src/PC/game_mp/g_active_mp.c', 'ClientEndFrame')
assert 'client->ps.pm_flags &= 0xfcffffff;' in active
assert flag & 0xfcffffff == flag

support = r'''
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cod2_sprint.h"
typedef unsigned char byte;
typedef int qboolean;
typedef struct {int pm_flags,eFlags,pm_type,weaponstate,viewmodelIndex,clientNum;float fWeaponPosFrac;} playerState_t;
typedef struct {int team;} clientState_t;
typedef struct {
    playerState_t ps;
    struct {int forceSpectatorClient,noSpectate,archiveTime,psOffsetTime;} sess;
    int spectatorClient;float fGunPitch,fGunYaw;
} gclient_t;
typedef struct {gclient_t *client;struct {int svFlags,contents;} r;struct {int number,eType;} s;int takedamage;} gentity_t;
static playerState_t archived;
static int canArchive=1;
static int SV_GetArchivedClientInfo(int n,int *time,playerState_t *ps,clientState_t *cs) {
    assert(n==0);*ps=archived;cs->team=1;return canArchive;
}
static void HudElem_UpdateClient(gclient_t *c,int n,int flags) {}
static void StopFollowing(gentity_t *e) {e->client->ps.pm_flags&=~0x400000;e->client->ps.pm_type=4;}
static void Cmd_FollowCycle_f(gentity_t *e,int dir) {e->client->spectatorClient=0;}
static float sprintViewBlend;static int sprintViewTime;
typedef struct {int overflowed;byte *data;int maxsize,cursize,readcount,bit;} msg_t;
'''
body = function('src/PC/game_mp/g_active_mp.c', 'SpectatorClientEndFrame')
body += ''.join(function('src/PC/cgame_mp/cg_weapons.c', n) for n in ('CG_ResetSprintView', 'CG_SprintViewBlend'))
body += ''.join(function('src/PC/qcommon/msg_mp.c', n) for n in (
    'MSG_WriteBits_core', 'MSG_WriteBits', 'MSG_ReadBits_core', 'MSG_ReadBits'))
checks = r'''
int main(void) {
    int cases=0;
    for(int ms=1;ms<=66;ms++)for(int forced=0;forced<2;forced++)for(int restricted=0;restricted<2;restricted++) {
        gclient_t client={.sess={.forceSpectatorClient=forced?0:-1,.noSpectate=restricted?0x10:0},.spectatorClient=0};
        gentity_t ent={.client=&client,.s.number=1};
        CG_ResetSprintView();
        /* A target who is standing still must remain in the normal pose for
         * every follow mode, although spectator permission bits are set. */
        archived=(playerState_t){.pm_flags=0x800000};
        for(int t=1000;t<1600;t+=ms) {
            SpectatorClientEndFrame(&ent);
            assert(client.ps.pm_flags&0x400000);
            assert(!(client.ps.pm_flags&PMF_SPRINT));
            assert(CG_SprintViewBlend(&client.ps,t)==0);
        }
        /* This matches the active player's end-frame permission cleanup.
         * Both normal and restricted spectators must retain actual sprint. */
        archived.pm_flags=(0x800000|PMF_SPRINT)&0xfcffffff;
        for(int t=1600;t<2100;t+=ms) {
            SpectatorClientEndFrame(&ent);
            assert(client.ps.pm_flags&PMF_SPRINT);
            CG_SprintViewBlend(&client.ps,t);
        }
        assert(sprintViewBlend==1);
        archived.pm_flags=0x800000;
        for(int t=2100;t<2500;t+=ms) {
            SpectatorClientEndFrame(&ent);CG_SprintViewBlend(&client.ps,t);
        }
        assert(sprintViewBlend==0);
        /* Switching to free spectate does not leave a raised carry pose. */
        client.spectatorClient=client.sess.forceSpectatorClient=-1;
        client.sess.noSpectate=0;
        SpectatorClientEndFrame(&ent);
        assert(!(client.ps.pm_flags&PMF_SPRINT));
        cases++;
    }
    for(int offset=0;offset<8;offset++)for(int spectator=0;spectator<4;spectator++) {
        byte data[64]={0};msg_t msg={.data=data,.maxsize=64};
        int flags=PMF_SPRINT|0x800000|(spectator<<24);
        MSG_WriteBits(&msg,5,offset);MSG_WriteBits(&msg,flags,FLAG_BITS);MSG_WriteBits(&msg,153,10);
        msg.bit=0;msg.readcount=0;
        assert(MSG_ReadBits(&msg,offset)==(5&((1<<offset)-1)));
        assert(MSG_ReadBits(&msg,FLAG_BITS)==flags);
        assert(MSG_ReadBits(&msg,10)==153&&!msg.overflowed);
    }
    printf("PASS: %d spectator carry replays, idle/sprint/stop/free follow, preserved active sprint and 32 packet flag roundtrips\n",cases);
}
'''
source = support + f'\n#define FLAG_BITS {bits}\n' + body + checks
mutants = (
    source.replace('#include "cod2_sprint.h"', '#define PMF_SPRINT 0x02000000'),
    source.replace(f'#define FLAG_BITS {bits}', '#define FLAG_BITS 27'),
    source.replace('memcpy(client, &ps, sizeof(playerState_t));',
                   'memcpy(client, &ps, sizeof(playerState_t)); client->ps.pm_flags &= ~PMF_SPRINT;'),
)
with tempfile.TemporaryDirectory(prefix='cod2-spectator-sprint-') as directory:
    d = Path(directory)
    for index, variant in enumerate((source, *mutants)):
        (d / 'test.c').write_text(variant)
        subprocess.run(['cc', '-I', str(root / 'src/headers'), '-std=c99', '-O1',
                        '-fsanitize=address,undefined', str(d / 'test.c'), '-lm', '-o', str(d / 'test')], check=True)
        result = subprocess.run([str(d / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stdout, result.stderr)
        if not index:
            print(result.stdout, end='')
print('PASS: spectator/sprint overlap, truncated flags and discarded archived sprint mutants fail')
