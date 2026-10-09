#!/usr/bin/env python3
"""Exercise the actual server event export, end-frame handoff and client ring."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
active = (root/'src/PC/game_mp/g_active_mp.c').read_text()
events = (root/'src/PC/cgame_mp/cg_event_mp.c').read_text()

def function(source, name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'

export = function(active, 'G_PlayerStateToEntityStateExtrapolate')
export = export[export.index('    if (ps->entityEventSequence'):export.index('    s->weapon =')]
frame = function(active, 'ClientEndFrame')
frame = frame[frame.index('    client->ps.stats[0] = ent->health;'):frame.index('    if (ent->health > 0 && StuckInClient')]
body = ('static void G_PlayerStateToEntityStateExtrapolate(playerState_t *ps,entityState_t *s,int time,int snap){\nint eventSequence;\n'+export+'}\n'
        'static void BG_PlayerStateToEntityState(playerState_t *ps,void *s,int snap,int handler){G_PlayerStateToEntityStateExtrapolate(ps,s,0,snap);}\n'
        'static void EndFrame(gentity_t *ent){gclient_t *client=ent->client;\n'+frame+'}\n'
        +function(events,'CG_CheckEvents'))
support = r'''
#include <assert.h>
#include <string.h>
#define qtrue 1
typedef unsigned char byte;
typedef float vec3_t[3];
typedef struct {int entityEventSequence,eventSequence,oldEventSequence,events[4],eventParms[4],stats[1],commandTime;} playerState_t;
typedef struct {int number,eType,eventSequence,eventParm,events[4],eventParms[4];} entityState_t;
typedef struct {entityState_t nextState;int previousEventSequence;} centity_t;
typedef struct {playerState_t ps;vec3_t vGunSpeed;} gclient_t;
typedef struct {entityState_t s;gclient_t *client;int health;} gentity_t;
static gentity_t g_entities[64];
static struct {struct {int enabled;} current;} smooth={.current.enabled=1},*g_smoothClients=&smooth;
static const int singles[]={140,141,142,143,185,186,-1};
static const void *imp_singleClientEvents=singles;
static void BG_WeaponFireRecoil(playerState_t *ps,float *speed,float *kick){}
static int received, receivedEvents[4],receivedParms[4];
static void CG_CalcEntityLerpPositions(centity_t *cent){}
static void CG_EntityEvent(centity_t *cent,int event){
 assert(received<4);receivedEvents[received]=event;receivedParms[received++]=cent->nextState.eventParm;
}
'''
checks = r'''
int main(void){
 for(int n=0;n<64;n++){
  gclient_t client={0};gentity_t *ent=&g_entities[n];
  ent->client=&client;ent->health=100;ent->s.number=n;ent->s.eType=1;
  centity_t cent={0};
  for(int frame=0;frame<600;frame++){
   int burst=frame%7;
   for(int i=0;i<burst;i++){
    int seq=client.ps.eventSequence++;
    client.ps.events[seq&3]=seq&1?158:1;client.ps.eventParms[seq&3]=seq&255;
   }
   G_PlayerStateToEntityStateExtrapolate(&client.ps,&ent->s,0,1);
   EndFrame(ent); /* Must preserve events exported by ClientThink. */
   cent.nextState=ent->s;cent.nextState.eventSequence&=255;
   received=0;CG_CheckEvents(&cent);
   int expected=burst>4?4:burst;assert(received==expected);
   for(int i=0;i<expected;i++){
    int seq=client.ps.eventSequence-expected+i;
    assert(receivedEvents[i]==(seq&1?158:1));assert(receivedParms[i]==(seq&255));
   }
   received=0;CG_CheckEvents(&cent);assert(!received);
  }
 }
 centity_t effect={.nextState.eType=191};
 received=0;CG_CheckEvents(&effect);assert(received==1 && receivedEvents[0]==181);
 received=0;CG_CheckEvents(&effect);assert(!received);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-remote-events-') as folder:
    path=Path(folder)
    old = body.replace('    client->ps.stats[0] = ent->health;',
                       '    client->ps.stats[0] = ent->health; ent->s.eventSequence=0;')
    for i, variant in enumerate((body, old)):
        (path/'test.c').write_text(support+variant+checks)
        subprocess.run(['cc','-O1','-g','-fsanitize=address,undefined',str(path/'test.c'),'-o',str(path/'test')],check=True)
        result=subprocess.run([str(path/'test')],capture_output=True)
        assert (result.returncode==0)==(i==0), result.stderr.decode()
print('PASS: 38400 server/client frames, event bursts, eight-bit wrap, no duplicates; old frame reset fails under ASan/UBSan')
