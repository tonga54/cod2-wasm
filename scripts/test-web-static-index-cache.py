#!/usr/bin/env python3
"""Exercise real static-index lifecycle and GPU/CPU vertex-address equivalence."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/Mac/DirectX_9/CDirect3DIndexBuffer.c').read_text()
device = (root / 'src/Mac/DirectX_9/CDirect3DDevice.c').read_text()

def extract(text, name):
    start = text.index(name + '(')
    while text.find(';', start) < text.find('{', start):
        start = text.index(name + '(', start + len(name))
    start = text.rfind('\n', 0, start) + 1
    end, depth = text.index('{', start) + 1, 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

types = source[source.index('#ifdef __EMSCRIPTEN__'):source.index('extern void glGenBuffers')]
types = types[:types.rfind('#ifdef __EMSCRIPTEN__')]
support = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
typedef unsigned char byte;
typedef unsigned UINT,UINT32,DWORD,ULONG;
typedef int INT,HRESULT,D3DFORMAT,D3DPOOL;
#define D3DFMT_INDEX16 101
static void *vtbl_CDirect3DIndexBuffer[16];
''' + types + r'''
typedef CDirect3DIndexBufferClean CDirect3DIndexBuffer;
static int generated, uploaded, deleted, bound, rangeScans, failGeneration;
static byte uploadedData[1024];
static void glGenBuffers(int n,unsigned *id){assert(n==1);generated++;*id=failGeneration?0:123;}
static void glDeleteBuffers(int n,const unsigned *id){assert(n==1&&*id==123);deleted++;}
static void glBindBuffer(unsigned target,unsigned id){assert(target==0x8893&&id==123);bound++;}
static void glBufferData(unsigned target,ptrdiff_t length,const void *data,unsigned usage){
 assert(target==0x8893&&length==1024&&data&&usage==0x88e4);uploaded++;memcpy(uploadedData,data,length);
}
'''
functions = ''.join(extract(source, name) for name in (
    'CDirect3DIndexBuffer_StaticWebRange', 'CDirect3DIndexBuffer_BindStaticWeb',
    'CDirect3DIndexBuffer_Lock', 'CDirect3DIndexBuffer_Unlock',
    'ZN20CDirect3DIndexBufferD1Ev', 'CDirect3DIndexBuffer_CDirect3DIndexBuffer'))
functions = functions.replace('indices = (const unsigned short *)ib->data + start;',
                              'rangeScans++; indices = (const unsigned short *)ib->data + start;')
functions += extract(device, 'CDirect3DDevice_BindStaticWebIndices')
checks = r'''
int main(void){
 CDirect3DIndexBufferClean ib={0};UINT lo,count;
 assert(sizeof(ib)<=0x34+(sizeof(void*)>4?0x100:0));
 CDirect3DIndexBuffer_CDirect3DIndexBuffer(&ib,1024,D3DFMT_INDEX16,8,0);
 unsigned short *indices=(unsigned short *)ib.data;
 for(int n=0;n<512;n++)indices[n]=64000+n;
 for(int n=0;n<600;n++){
  assert(CDirect3DIndexBuffer_StaticWebRange(&ib,6,6,&lo,&count));assert(lo==64006&&count==6);
  assert(CDirect3DIndexBuffer_BindStaticWeb(&ib)==123);
 }
 assert(rangeScans==1&&generated==1&&uploaded==1&&bound==600);
 assert(!memcmp(uploadedData,ib.data,1024));
 /* Partial writes invalidate ranges and uploads before data can change. */
 void *write;CDirect3DIndexBuffer_Lock(&ib,12,2,&write,0);*(unsigned short *)write=32;
 assert(!CDirect3DIndexBuffer_StaticWebRange(&ib,6,6,&lo,&count));
 assert(!CDirect3DIndexBuffer_BindStaticWeb(&ib));
 CDirect3DIndexBuffer_Unlock(&ib);
 assert(CDirect3DIndexBuffer_StaticWebRange(&ib,6,6,&lo,&count));assert(lo==32&&count==63980);
 assert(CDirect3DIndexBuffer_BindStaticWeb(&ib)==123&&uploaded==2&&generated==1);
 assert(!memcmp(uploadedData,ib.data,1024));
 /* Different/colliding ranges must always return their actual min/max. */
 uint32_t seed=1234567;
 for(int n=0;n<40000;n++){
  seed=seed*1664525u+1013904223u;unsigned start=(seed>>16)%480;
  unsigned length=1+(seed%32),actualLo=65535,actualHi=0;
  assert(CDirect3DIndexBuffer_StaticWebRange(&ib,start,length,&lo,&count));
  for(unsigned i=0;i<length;i++){unsigned v=indices[start+i];if(v<actualLo)actualLo=v;if(v>actualHi)actualHi=v;}
  assert(lo==actualLo&&count==actualHi-actualLo+1);
  const byte *attributes=(const byte *)(uintptr_t)17;const unsigned short *draw=(const unsigned short *)(uintptr_t)19;
  int base=n%13;unsigned offset=n%71,stride=24+(n%4)*4;
  assert(CDirect3DDevice_BindStaticWebIndices(&ib,1,offset,base,stride,start,&attributes,&draw));
  assert((uintptr_t)draw==start*2);
  for(unsigned i=0;i<length;i++){
   uintptr_t gpu=(uintptr_t)attributes+(uintptr_t)indices[start+i]*stride;
   uintptr_t cpu=offset+(uintptr_t)(base+lo)*stride+(uintptr_t)(indices[start+i]-lo)*stride;
   assert(gpu==cpu);
  }
 }
 const byte *attributes=(const byte *)(uintptr_t)17;const unsigned short *draw=(const unsigned short *)(uintptr_t)19;
 assert(!CDirect3DDevice_BindStaticWebIndices(&ib,0,0,0,32,0,&attributes,&draw));
 assert(!CDirect3DDevice_BindStaticWebIndices(&ib,1,0,-5,32,0,&attributes,&draw));
 assert((uintptr_t)attributes==17&&(uintptr_t)draw==19);
 assert(!CDirect3DIndexBuffer_StaticWebRange(&ib,511,2,&lo,&count));
 assert(!CDirect3DIndexBuffer_StaticWebRange(&ib,0,0,&lo,&count));
 ib.usage=512;assert(!CDirect3DIndexBuffer_BindStaticWeb(&ib));
 assert(!CDirect3DIndexBuffer_StaticWebRange(&ib,6,6,&lo,&count));
 ib.usage=8;ib.indexSizeBytes=4;assert(!CDirect3DIndexBuffer_BindStaticWeb(&ib));
 assert(!CDirect3DIndexBuffer_StaticWebRange(&ib,6,6,&lo,&count));
 ZN20CDirect3DIndexBufferD1Ev(&ib);assert(deleted==1&&!ib.data&&!ib.webBuffer&&!ib.webRanges);
 /* Reusing the object/address cannot reuse the previous map's geometry. */
 CDirect3DIndexBuffer_CDirect3DIndexBuffer(&ib,1024,D3DFMT_INDEX16,8,0);
 indices=(unsigned short *)ib.data;indices[0]=65535;
 assert(CDirect3DIndexBuffer_StaticWebRange(&ib,0,3,&lo,&count));assert(lo==0&&count==65536);
 failGeneration=1;assert(!CDirect3DDevice_BindStaticWebIndices(&ib,1,0,0,32,0,&attributes,&draw));
 failGeneration=0;assert(CDirect3DIndexBuffer_BindStaticWeb(&ib)==123);
 assert(!memcmp(uploadedData,ib.data,1024));ZN20CDirect3DIndexBufferD1Ev(&ib);assert(deleted==2);
 puts("PASS: 600 static draws scan/upload once; partial writes, locks, dynamic/32-bit buffers, allocation failure and map/address reuse stay fresh");
 puts("PASS: 40000 mesh ranges retain every indexed vertex address; CPU conversions and negative bases keep the rebased path");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-static-indices-') as directory:
    path = Path(directory)
    (path / 'test.c').write_text(support + functions + checks)
    subprocess.run(['cc', '-D__EMSCRIPTEN__', '-std=c99', '-O1', '-g',
                    '-fsanitize=address,undefined', str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
