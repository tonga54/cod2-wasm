#!/usr/bin/env python3
"""Exercise the production fixed-tick bot loop, including catch-up frames."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/server_mp/sv_main_mp.c').read_text()
start = source.index('    do {\n        sv.timeResidual -= frameMsec;')
end = source.index('    } while (frameMsec <= sv.timeResidual);', start) + len('    } while (frameMsec <= sv.timeResidual);')
loop = source[start:end]
assert source.index('SV_BotUserMove(cl);') >= start
support = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define NA_BOT 1
static struct {int timeResidual;}sv;
static struct {int time;}svs;
typedef struct {int state;struct {struct {int type;}remoteAddress;}netchan;}client_t;
static client_t clients[3];
static struct {struct {int integer;}current;}maxClients={ {3} };
static const void *unused;
#define sv_maxclients (&maxClients)
static struct {int enabled,planMs,frames,gameMs,maxGameMs;}s_botProfile;
static int gameCalls,planCalls,moves,archives,lastMoveTime;
static int Sys_Milliseconds(void){return 0;}
static void CL_FlushDebugData(int value){assert(value==1);}
static void SV_ResetSkeletonCache(void){}
static void SV_BotPlanFrame(void){assert(svs.time==50*(planCalls+1));planCalls++;}
static void SV_BotUserMove(client_t *cl){assert(cl!=&clients[1]);assert(svs.time>=lastMoveTime);lastMoveTime=svs.time;moves++;}
static void G_RunFrame(int time){assert(time==svs.time&&moves==2*(gameCalls+1));gameCalls++;}
static void Scr_SetLoading(int value){assert(!value);}
static void SV_ArchiveSnapshot(void){archives++;}
'''
# The actual source refers to svs.clients: retain that access in a minimal fixture.
support = support.replace('static struct {int time;}svs;', '')
support = support.replace('static client_t clients[3];', 'static client_t clients[3];\nstatic struct {int time;client_t *clients;}svs={0,clients};')
checks = r'''
static void run(int residual){int frameMsec=50,i;client_t *cl;
 sv.timeResidual=residual;
LOOP
}
int main(void){
 clients[0].state=clients[2].state=4;
 clients[0].netchan.remoteAddress.type=clients[2].netchan.remoteAddress.type=NA_BOT;
 clients[1].state=4;
 for(int residual=50;residual<=275;residual+=25){
  gameCalls=planCalls=moves=archives=lastMoveTime=svs.time=0;
  run(residual);
  assert(gameCalls==residual/50&&planCalls==gameCalls&&moves==gameCalls*2);
  assert(sv.timeResidual==residual%50&&archives==gameCalls-1);
 }
 puts("PASS: each bot moves once per 50ms simulation tick, including 2..5 catch-up ticks; human commands are untouched and archive order is preserved");
}
'''.replace('LOOP', loop)
with tempfile.TemporaryDirectory(prefix='cod2-bot-cadence-') as directory:
    path = Path(directory)
    (path / 'test.c').write_text(support + checks)
    subprocess.run(['cc', '-std=c99', '-O2', '-fsanitize=undefined',
                    str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True, timeout=10)
