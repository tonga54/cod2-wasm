#!/usr/bin/env python3
"""Archive delta bases must remain decodable throughout a long match."""
from pathlib import Path
import subprocess,tempfile
s=(Path(__file__).resolve().parent.parent/'src/PC/server_mp/sv_snapshot_mp.c').read_text()
a=s.index('                if (((cachedSnapshot_t *)cf)->archivedFrame >= newnum)')
b=s.index('                        MSG_WriteBit0',a)
choice=s[a:b]+'''return candidate;}break;}idx--;}return -1;}'''
code=r'''
#include <assert.h>
#include <stdio.h>
typedef struct {int archivedFrame,num_entities,first_entity,num_clients,first_client,usesDelta,time;}cachedSnapshot_t;
typedef struct {int nextCachedSnapshotEntities,nextCachedSnapshotClients;}serverStatic_t;
static serverStatic_t svs;static void *imp_svs=&svs;
static cachedSnapshot_t frames[512];static int count;
static int pick(int next,int fps){int newnum=next-fps;int oldindex=count-512;if(oldindex<0)oldindex=0;
 int idx=count-1;while(idx>=oldindex){int candidate=idx%512;cachedSnapshot_t *cf=&frames[candidate];
'''+choice+r'''
int main(void){int full=0,delta=0;
 for(int f=0;f<10000;f++){
  int slot=pick(f,20);
  if(slot<0){frames[count%512]=(cachedSnapshot_t){.archivedFrame=f};count++;full++;}
  else {int base=frames[slot].archivedFrame;assert(base<f && f-base<=20 && base>=f-1199);delta++;}
 }
 assert(full>=475 && delta>9000);
 /* Reject bases whose cached entity/client rings have been reused. */
 svs.nextCachedSnapshotEntities=20000;assert(pick(10000,20)==-1);
 svs.nextCachedSnapshotEntities=0;svs.nextCachedSnapshotClients=5000;assert(pick(10000,20)==-1);
 printf("PASS: 10000 archived frames with %d fresh bases, bounded deltas and expired client/entity rings\n",full);
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)
 for mutant in (False,True):
  (p/'t.c').write_text(code.replace('archivedFrame >= newnum','archivedFrame <= newnum') if mutant else code)
  subprocess.run(['cc','-O1','-fsanitize=address,undefined',str(p/'t.c'),'-o',str(p/'t')],check=True)
  r=subprocess.run([str(p/'t')],capture_output=True,text=True)
  assert (r.returncode==0)!=mutant,r.stderr
  if not mutant:print(r.stdout,end='')
