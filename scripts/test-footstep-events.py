#!/usr/bin/env python3
"""Exercise the movement code that emits networked surface footstep events."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/bgame/bg_pmove.c').read_text()

def function(name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'

body = function('PM_GroundSurfaceType') + function('PM_FootstepEvent')
support = r'''
#include <assert.h>
#define ENTITYNUM_NONE 1023
typedef int qboolean;
typedef struct {int groundEntityNum,viewHeightTarget;} playerState_t;
typedef struct {playerState_t *ps;} pmove_t;
typedef struct {int walking;struct {unsigned surfaceFlags;} groundTrace;} pml_t;
static int events,lastEvent;
static void PM_AddEvent(playerState_t *ps,int event){events++;lastEvent=event;}
'''
checks = r'''
int main(void){
 playerState_t ps={.groundEntityNum=1022,.viewHeightTarget=60};
 pmove_t pm={.ps=&ps};pml_t pml={.walking=1};
 int heights[]={60,40,11},bases[]={1,24,47};
 for(int stance=0;stance<3;stance++)for(int surface=0;surface<32;surface++){
  ps.viewHeightTarget=heights[stance];pml.groundTrace.surfaceFlags=surface<<20;
  for(int old=0;old<256;old++)for(int step=0;step<16;step++){
   int next=(old+step)&255,before=events;
   PM_FootstepEvent(&pm,&pml,old,next,1);
   assert(events-before==!!((old^next)&64));
   if(events>before)assert(lastEvent==bases[stance]+(surface<23?surface:0));
  }
 }
 for(int condition=0;condition<4;condition++){
  int before=events;pml.walking=condition!=0;ps.groundEntityNum=condition==1?1023:1022;
  pml.groundTrace.surfaceFlags=condition==2?0x2000:0;
  PM_FootstepEvent(&pm,&pml,63,64,condition!=3);assert(events==before);
 }
 return 0;
}
'''
assert re.search(r'pm->xyspeed\s*=\s*PM_VectorLength2D\(ps->velocity\);\s+PM_Footsteps\(pm,\s*&pml\);', source)
with tempfile.TemporaryDirectory(prefix='cod2-footsteps-') as folder:
    path = Path(folder)
    (path / 'test.c').write_text(support + body + checks)
    subprocess.run(['cc', '-O1', '-g', '-fsanitize=address,undefined', str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('PASS: 393216 footstep transitions, all surface/stance variants, no airborne or silent-surface steps under ASan/UBSan')
