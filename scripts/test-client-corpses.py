#!/usr/bin/env python3
"""Exercise corpse snapshot copying independently of the cgs binary layout."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_snapshot_mp.c').read_text()
start = source.index('static void CG_ResetCorpseEntity(centity_t *cent)\n{')
body = source[start:source.index('\nstatic void CG_ResetEntity(char *cent)\n{', start)]
assert '0x6bf0' not in source
assert '0x6bec' not in (root / 'src/PC/cgame_mp/cg_players_mp.c').read_text()
support = r'''
#include <assert.h>
#include <string.h>
#include <strings.h>
typedef struct { int clientNum,dobjDirty;char model[64],attachModelNames[6][64],attachTagNames[6][64];void *pXAnimTree;struct {int animationNumber;} legs; } clientInfo_t;
typedef struct { char padding[131071];struct { clientInfo_t clientinfo[64]; } bgs; } cg_t;
typedef struct { char padding[164756];clientInfo_t corpseinfo[8];char tail[64]; } cgs_t;
typedef struct { struct { int number,clientNum,eFlags,eventSequence,legsAnim; } nextState;int previousEventSequence; } centity_t;
static cg_t cgData;static cgs_t cgsData;
static void *cg=&cgData,*cgs=&cgsData;
static int sourceTrees[64],corpseTrees[8],clones;
static int I_stricmp(const char *a,const char *b) { return strcasecmp(a,b); }
static void XAnimCloneAnimTree(void *from,void *to) {
 assert(from>= (void *)sourceTrees && from<(void *)(sourceTrees+64));
 assert(to>= (void *)corpseTrees && to<(void *)(corpseTrees+8));*(int *)to=*(int *)from;clones++;
}
'''
checks = r'''
int main(void) {
 memset(&cgsData,0xA5,sizeof(cgsData));
 for(int client=0;client<64;client++) {
  clientInfo_t *s=&cgData.bgs.clientinfo[client];s->clientNum=client;
  s->pXAnimTree=&sourceTrees[client];strcpy(s->model,"player");
  strcpy(s->attachModelNames[0],"helmet");strcpy(s->attachTagNames[0],"J_Head");
  strcpy(s->attachModelNames[1],"rifle");strcpy(s->attachTagNames[1],"J_Spine4");
  s->legs.animationNumber=77;sourceTrees[client]=1000+client;
  for(int stance=0;stance<3;stance++) for(int slot=0;slot<8;slot++) {
   clientInfo_t *c=&cgsData.corpseinfo[slot];memset(c,0,sizeof(*c));c->pXAnimTree=&corpseTrees[slot];
   /* Recycled slots contain a different player's already-finished tree. */
   strcpy(c->model,"previous player");c->clientNum=client;c->legs.animationNumber=77;
   corpseTrees[slot]=-99;
   centity_t cent={.nextState={64+slot,client,0x80000 | (stance==2?8:stance==1?4:0),17,77}};
   CG_ResetCorpseEntity(&cent);
   assert(c->clientNum==client && c->pXAnimTree==&corpseTrees[slot] && c->dobjDirty);
   assert(!strcmp(c->model,"player") && !strcmp(c->attachModelNames[0],"helmet"));
   assert(!c->attachModelNames[1][0] && !c->attachTagNames[1][0]);
   assert(!cent.previousEventSequence);
   assert(corpseTrees[slot]==sourceTrees[client]);
   assert(c->legs.animationNumber!=cent.nextState.legsAnim);
   assert((c->legs.animationNumber&~0x200)==77);
   /* A completed prone body must retain its own pose after owner respawn. */
   cent.nextState.eFlags=8;corpseTrees[slot]=42;
   CG_ResetCorpseEntity(&cent);assert(cent.previousEventSequence==17 && corpseTrees[slot]==42);
  }
 }
 assert(clones==1536);
 for(int i=0;i<sizeof(cgsData.padding);i++)assert((unsigned char)cgsData.padding[i]==0xA5);
 for(int i=0;i<sizeof(cgsData.tail);i++)assert((unsigned char)cgsData.tail[i]==0xA5);
 for(int n=-1;n<1024;n++)if(n<64 || n>=72) {
  centity_t c={.nextState={n,0,0x80000,1,77}};CG_ResetCorpseEntity(&c);
 }
 assert(clones==1536);return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-client-corpses-') as directory:
    path = Path(directory)
    old = body.replace('&((cgs_t *)cgs)->corpseinfo[corpseIndex]',
                       '(clientInfo_t *)((char *)cgs + cent->nextState.number * 1208 - 0x6bec)')
    wrong_flag = body.replace('int clone = cent->nextState.eFlags & 0x80000;',
                              'int clone = cent->nextState.eFlags & 8;')
    skipped_transition = body.replace('corpse->legs.animationNumber ^= 0x200;', '')
    for index, variant in enumerate((body, old, wrong_flag, skipped_transition)):
        (path / 'test.c').write_text(support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(path / 'test.c'), '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True)
        assert (result.returncode == 0) == (index == 0), result.stderr.decode()
print('PASS: 1536 recycled corpse clones in three stances, fresh death transitions, completed prone poses, attachments and bounds; three broken variants fail')
