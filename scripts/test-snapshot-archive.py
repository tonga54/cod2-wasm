#!/usr/bin/env python3
"""Exercise actual archive ring storage/retrieval, including wrap and truncation."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parent.parent
s=(root/'src/PC/server_mp/sv_snapshot_mp.c').read_text()
a=s.index('static __attribute_regparm__(1)\n    cachedSnapshot_t *SV_GetCachedSnapshotInternal')
b=s.index('\n\nstatic int signedMod512',a)
reader=s[a:b].replace('__attribute_regparm__(1)','')
a=s.index('    {\n        serverStatic_t *svs2 = (serverStatic_t *)imp_svs;\n        int archivedFrameCount =')
b=s.index('\ncleanup:',a)
writer='static void storeArchive(msg_t *input){byte *msg=(byte*)input;'+s[a:b]+'}\n'
helpers=s[s.index('static int signedMod33554432'):s.index('void SV_ArchiveSnapshot(void)\n{')]
support=r'''
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte;
typedef struct {int overflowed;byte*data;int maxsize,cursize,readcount,bit;} msg_t;
typedef struct{int archivedFrame,num_entities,first_entity,num_clients,first_client,usesDelta,time;}cachedSnapshot_t;
typedef struct{int active;int cs[23];struct{int commandTime;}ps;}cachedClient_t;
typedef struct{byte baseline[276];}svEntity_t;
typedef struct{svEntity_t svEntities[1024];}server_t;
typedef struct{int nextCachedSnapshotFrames,nextCachedSnapshotEntities,nextCachedSnapshotClients,nextArchivedSnapshotFrames,nextArchivedSnapshotBuffer;
 int (*archivedSnapshotFrames)[2];byte*archivedSnapshotBuffer,*cachedSnapshotEntities,*cachedSnapshotClients;cachedSnapshot_t*cachedSnapshotFrames;}serverStatic_t;
static serverStatic_t svs;static server_t sv;static void*imp_svs=&svs,*imp_sv=&sv;
#define CACHEDCLIENT_STRIDE 9992
static void LargeLocal_LargeLocal(byte *ll,int n){void*p=malloc(n);memcpy(ll,&p,sizeof(p));}
static byte *LargeLocal_GetBuf(byte*ll){byte*p;memcpy(&p,ll,sizeof(p));return p;}
static void ZN10LargeLocalD1Ev(byte*ll){free(LargeLocal_GetBuf(ll));}
static void Com_Error(int n,const char*s){fprintf(stderr,"%s\n",s);abort();}
static void MSG_Init(msg_t*m,byte*d,int n){memset(m,0,sizeof(*m));m->data=d;m->maxsize=n;}
/* Simple bit codec: storage/retrieval logic is from the engine, codecs are
 * fixtures. Exhaustion deliberately leaves readcount==cursize, like MSG. */
static int MSG_ReadBits(msg_t*m,int n){if(m->bit+n>m->cursize*8){m->overflowed=1;return -1;}unsigned v=0;for(int i=0;i<n;i++){int b=m->bit++;v|=((m->data[b/8]>>(b%8))&1u)<<i;}m->readcount=(m->bit+7)/8;return (int)v;}
static int MSG_ReadBit(msg_t*m){return MSG_ReadBits(m,1);}
static int MSG_ReadLong(msg_t*m){return MSG_ReadBits(m,32);}
static void put(msg_t*m,unsigned v,int n){for(int i=0;i<n;i++){int b=m->bit++;if(!(b%8))m->data[b/8]=0;m->data[b/8]|=((v>>i)&1u)<<(b%8);}m->cursize=(m->bit+7)/8;}
static void MSG_ReadDeltaClient(msg_t*m,byte*f,byte*t,int n){*(int*)t=n;}
static void MSG_ReadDeltaPlayerstate(msg_t*m,void*f,void*t){}
static void MSG_ReadDeltaArchivedEntity(msg_t*m,byte*f,byte*t,int n){memset(t,0,276);*(int*)t=n;*(int*)(t+4)=MSG_ReadLong(m);}
'''
checks=r'''
static void packet(msg_t*m,byte*data,int f,int delta){MSG_Init(m,data,131072);put(m,!delta,1);if(delta)put(m,f-1,32);put(m,f*50,32);put(m,0,1);put(m,7,10);put(m,f*13,32);put(m,1023,10);}
int main(void){
 svs.archivedSnapshotFrames=calloc(1200,8);svs.archivedSnapshotBuffer=calloc(1,0x2000000);
 svs.cachedSnapshotFrames=calloc(512,28);svs.cachedSnapshotEntities=calloc(16384,276);svs.cachedSnapshotClients=calloc(4096,9992);
 /* Start near the boundary to exercise a message split over the ring end. */
 svs.nextArchivedSnapshotBuffer=0x2000000-3;byte data[131072];msg_t m;
 for(int f=0;f<1600;f++){packet(&m,data,f,f%2);int prev=svs.nextArchivedSnapshotBuffer;storeArchive(&m);assert(svs.nextArchivedSnapshotBuffer==prev+m.cursize);}
 for(int f=400;f<1600;f++){
  cachedSnapshot_t*c=SV_GetCachedSnapshotInternal(f);assert(c&&c->time==f*50&&c->num_entities==1);
  int*e=(int*)(svs.cachedSnapshotEntities+(c->first_entity%16384)*276);assert(e[0]==7&&e[1]==f*13);
 }
 assert(!SV_GetCachedSnapshotInternal(-1));assert(!SV_GetCachedSnapshotInternal(399));assert(!SV_GetCachedSnapshotInternal(1600));
 /* Evict decoded records and ensure truncated and cyclic records fail safely. */
 svs.nextCachedSnapshotFrames=0;
 svs.archivedSnapshotFrames[400][1]=5;assert(!SV_GetCachedSnapshotInternal(400));
 packet(&m,data,1600,0);m.cursize--;storeArchive(&m);assert(!SV_GetCachedSnapshotInternal(1600));
 MSG_Init(&m,data,131072);put(&m,0,1);put(&m,1601,32);storeArchive(&m);assert(!SV_GetCachedSnapshotInternal(1601));
 puts("PASS: 1600 archive writes, 1200 reads after cache eviction, ring wrap, truncated and cyclic snapshots");
 free(svs.archivedSnapshotFrames);free(svs.archivedSnapshotBuffer);free(svs.cachedSnapshotFrames);free(svs.cachedSnapshotEntities);free(svs.cachedSnapshotClients);
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);code=support+helpers+reader+writer+checks
 for mutant in (False,True):
  (p/'test.c').write_text(code.replace('svs2->nextArchivedSnapshotBuffer = bufOffset + msgDataLen;','/* old missing increment */') if mutant else code)
  subprocess.run(['cc','-O1','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
  r=subprocess.run([str(p/'test')],capture_output=True,text=True,timeout=15)
  assert (r.returncode==0)!=mutant,r.stderr
  if not mutant:print(r.stdout,end='')
