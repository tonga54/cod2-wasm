#!/usr/bin/env python3
"""Compare prediction's selected inputs against the full command-ring replay."""
from pathlib import Path
import re
import subprocess
import tempfile
root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/cgame_mp/cg_predict_mp.c').read_text()
match=re.search(r'static int CG_FirstPredictionCommand\([^;]*?\)\n\{',source)
end,depth=match.end(),1
while depth:
    depth+=(source[end]=='{')-(source[end]=='}');end+=1
body=source[match.start():end]
assert source.index('oldest = CG_FirstPredictionCommand(oldest, cmdNum, ps->commandTime);') < source.index('for (; oldest <= cmdNum; oldest++)')
support=r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef struct {int serverTime,buttons,weapon,angles[3];} usercmd_t;
static usercmd_t ring[128];static int latest,reads,failAt=-999999;
static int CL_GetUserCmd(int n,usercmd_t *cmd){reads++;assert(n<=latest);if(n<=latest-128||n==failAt)return 0;*cmd=ring[n&127];return 1;}
static uint32_t rng=19273643;
static unsigned Random(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
'''
checks=r'''
int main(void){
 for(int schedule=0;schedule<10000;schedule++){
  latest=127+schedule;int oldest=latest-127,t=1000+schedule;
  for(int n=oldest;n<=latest;n++){
   t+=Random()%51;usercmd_t cmd={.serverTime=t,.buttons=(int)Random(),.weapon=(int)Random()};
   for(int j=0;j<3;j++)cmd.angles[j]=(int)Random();ring[n&127]=cmd;
  }
  for(int boundary=-1;boundary<=128;boundary++){
   int ack=boundary<0?ring[oldest&127].serverTime-1:boundary>127?t+1:ring[(oldest+boundary)&127].serverTime;
   reads=0;int first=CG_FirstPredictionCommand(oldest,latest,ack);assert(reads<=8);
   // The unchanged production loop also checks the predecessor's availability.
   usercmd_t old[128],fresh[128],cmd,previous;int a=0,b=0;
   for(int n=oldest;n<=latest;n++)if(CL_GetUserCmd(n,&cmd)&&cmd.serverTime>ack&&cmd.serverTime<=t&&CL_GetUserCmd(n-1,&previous))old[a++]=cmd;
   for(int n=first;n<=latest;n++)if(CL_GetUserCmd(n,&cmd)&&cmd.serverTime>ack&&cmd.serverTime<=t&&CL_GetUserCmd(n-1,&previous))fresh[b++]=cmd;
   assert(a==b&&!memcmp(old,fresh,a*sizeof(*old)));
  }
  // Invalid intermediate history retains the original bounded scan fallback.
  failAt=oldest+63;assert(CG_FirstPredictionCommand(oldest,latest,t-1)==oldest);failAt=-999999;
 }
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-prediction-window-') as tmp:
    p=Path(tmp);(p/'test.c').write_text(support+body+checks)
    subprocess.run(['cc','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
print('PASS: 1.3 million command windows; identical replayed inputs, duplicate timestamps, ring wraps, missing history and all ack boundaries; at most eight lookup copies')
