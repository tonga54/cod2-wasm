#!/usr/bin/env python3
"""Verify one owner for local shot effects in the real snapshot transition."""
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/cgame_mp/cg_snapshot_mp.c').read_text()
a=source.index('        int isDemo = cg->demoType;')
b=source.index('\n}\n\nvoid CG_SetInitialSnapshot',a)
block=source[a:b].rstrip().removesuffix('}')
support=r'''
#include <assert.h>
#include <string.h>
typedef struct {int pm_flags;} playerState_t;
typedef struct {playerState_t ps;} snapshot_t;
typedef struct {int demoType;snapshot_t *snap,*nextSnap;} cg_t;
typedef struct {struct {int enabled;} current;} dvar_t;
static cg_t storage,*cg=&storage;static snapshot_t old,next;
static dvar_t no,sync;static dvar_t *np=&no,*sp=&sync;
static char **cg_dvar1=(char**)&np,**cg_dvar2=(char**)&sp;
static int effects;
static void CG_TransitionPlayerState(void *n,void *o){effects++;}
'''
checks=r'''
int main(void){
 cg->snap=&old;cg->nextSnap=&next;
 for(int demo=0;demo<2;demo++)for(int npred=0;npred<2;npred++)
 for(int synchronous=0;synchronous<2;synchronous++)for(int followed=0;followed<2;followed++)
 for(int ads=0;ads<2;ads++) {
  cg->demoType=demo;no.current.enabled=npred;sync.current.enabled=synchronous;
  next.ps.pm_flags=(followed?0x400000:0)|(ads?0x40:0);effects=0;
  TransitionSnapshot();
  int predicted=!(demo||npred||synchronous||followed);
  if(predicted)effects++; /* CG_PredictPlayerState owns live effects. */
  assert(effects==1);
 }
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-shot-effects-') as directory:
 d=Path(directory)
 for mutant in (False,True):
  tested=block.replace(' & 0x400000)', ' & 0x40)') if mutant else block
  (d/'test.c').write_text(support+'static void TransitionSnapshot(void) {\n'+tested+'\n}\n'+checks)
  subprocess.run(['cc','-O1','-fsanitize=address,undefined',str(d/'test.c'),'-o',str(d/'test')],check=True)
  result=subprocess.run([str(d/'test')],capture_output=True)
  assert (result.returncode==0)!=mutant,result.stderr.decode()
print('PASS: 32 ADS/prediction/follow/demo combinations deliver one local effect; former ADS gate fails')
