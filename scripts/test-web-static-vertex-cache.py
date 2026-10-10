#!/usr/bin/env python3
"""Exercise static GPU reuse, write invalidation and release of real VB code."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parent.parent
s=(root/'src/Mac/DirectX_9/CDirect3DVertexBuffer.c').read_text()
def extract(name):
 start=s.index(name+'(')
 while s.find(';',start)<s.find('{',start):start=s.index(name+'(',start+len(name))
 start=s.rfind('\n',0,start)+1;end=s.index('{',start)+1;depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
support=r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
typedef unsigned char byte;
typedef unsigned UINT,ULONG;

typedef int HRESULT;typedef unsigned UINT32,DWORD;typedef int D3DPOOL;
''' + s[s.index('typedef struct {'):s.index('} CDirect3DVertexBufferClean;') + len('} CDirect3DVertexBufferClean;')] + r'''
typedef CDirect3DVertexBufferClean CDirect3DVertexBuffer;
static void *vtbl_CDirect3DVertexBuffer[16];static int generated,uploaded,deleted,bound,failGeneration;static byte gpu[1024];
static void glGenBuffers(int n,unsigned*id){generated++;*id=failGeneration?0:123;}
static void glDeleteBuffers(int n,const unsigned*id){assert(n==1&&*id==123);deleted++;}
static void glBindBuffer(unsigned target,unsigned id){assert(target==0x8892&&id==123);bound++;}
static void glBufferData(unsigned t,ptrdiff_t len,const void*d,unsigned usage){assert(t==0x8892&&len==1024&&d&&usage==0x88e4);uploaded++;memcpy(gpu,d,len);}
'''
checks=r'''
static void checkColors(CDirect3DVertexBufferClean *vb,unsigned stride,int offset,int order,unsigned phase){
 byte expected[1024];memcpy(expected,vb->data,1024);
 for(unsigned i=phase+offset;i+4<=1024;i+=stride){
  const byte *src=vb->data+i;byte *dst=expected+i;
  if(order==2){dst[0]=src[1];dst[1]=src[2];dst[2]=src[3];dst[3]=src[0];}
  else{dst[0]=src[2];dst[1]=src[1];dst[2]=src[0];dst[3]=src[3];}
 }
 assert(!memcmp(gpu,expected,1024));
}
int main(void){CDirect3DVertexBufferClean vb={0};
 assert(sizeof(vb)<=0x3c+(sizeof(void*)>4?0x100:0));
 CDirect3DVertexBuffer_CDirect3DVertexBuffer(&vb,1024,8,0);
 for(int i=0;i<600;i++)assert(CDirect3DVertexBuffer_BindStaticWeb(&vb)==123);
 assert(generated==1&&uploaded==1&&bound==600);
 CDirect3DVertexBuffer_Unlock(&vb);assert(CDirect3DVertexBuffer_BindStaticWeb(&vb)==123);assert(uploaded==2&&generated==1);
 vb.usage=512;assert(!CDirect3DVertexBuffer_BindStaticWeb(&vb));assert(uploaded==2&&bound==601);
 ZN21CDirect3DVertexBufferD1Ev(&vb);assert(deleted==1&&!vb.data&&!vb.webBuffer);
 CDirect3DVertexBuffer_CDirect3DVertexBuffer(&vb,1024,512,0);assert(!CDirect3DVertexBuffer_BindStaticWeb(&vb));ZN21CDirect3DVertexBufferD1Ev(&vb);assert(deleted==1);
 CDirect3DVertexBuffer_CDirect3DVertexBuffer(&vb,1024,8,0);
 for(unsigned i=0;i<1024;i++)vb.data[i]=(i*73+11)%256;
 byte original[1024];memcpy(original,vb.data,1024);
 int before=uploaded;
 for(int i=0;i<600;i++)assert(CDirect3DVertexBuffer_BindStaticWebColors(&vb,24,12,2,0)==123);
 assert(uploaded==before+1);checkColors(&vb,24,12,2,0);assert(!memcmp(original,vb.data,1024));
 /* Distinct layout phases/formats must convert exactly, even in one buffer. */
 for(unsigned stride=16;stride<=68;stride+=4)for(unsigned phase=0;phase<stride;phase++)for(int order=1;order<=2;order++){
  unsigned offset=stride-4;
  assert(CDirect3DVertexBuffer_BindStaticWebColors(&vb,stride,offset,order,phase)==123);
  checkColors(&vb,stride,offset,order,phase);
 }
 assert(CDirect3DVertexBuffer_BindStaticWeb(&vb)==123);assert(!memcmp(gpu,original,1024));
 void *write;before=uploaded;CDirect3DVertexBuffer_Lock(&vb,3,1,&write,0);*(byte *)write=99;
 assert(!CDirect3DVertexBuffer_BindStaticWebColors(&vb,24,12,2,0));assert(uploaded==before);
 CDirect3DVertexBuffer_Unlock(&vb);assert(CDirect3DVertexBuffer_BindStaticWebColors(&vb,24,12,2,0)==123);checkColors(&vb,24,12,2,0);
 assert(!CDirect3DVertexBuffer_BindStaticWebColors(&vb,0,12,2,0));
 assert(!CDirect3DVertexBuffer_BindStaticWebColors(&vb,24,23,2,0));
 assert(!CDirect3DVertexBuffer_BindStaticWebColors(&vb,24,12,2,24));
 assert(!CDirect3DVertexBuffer_BindStaticWebColors(&vb,24,12,3,0));
 ZN21CDirect3DVertexBufferD1Ev(&vb);
 CDirect3DVertexBuffer_CDirect3DVertexBuffer(&vb,1024,8,0);failGeneration=1;
 assert(!CDirect3DVertexBuffer_BindStaticWebColors(&vb,24,12,2,0));failGeneration=0;
 assert(CDirect3DVertexBuffer_BindStaticWebColors(&vb,24,12,2,0)==123);checkColors(&vb,24,12,2,0);
 ZN21CDirect3DVertexBufferD1Ev(&vb);
 return 0;
}
'''
source=support+extract('CDirect3DVertexBuffer_BindStaticWebColors')+extract('CDirect3DVertexBuffer_BindStaticWeb')+extract('CDirect3DVertexBuffer_Lock')+extract('CDirect3DVertexBuffer_Unlock')+extract('ZN21CDirect3DVertexBufferD1Ev')+extract('CDirect3DVertexBuffer_CDirect3DVertexBuffer')+checks
with tempfile.TemporaryDirectory(prefix='cod2-static-vbo-') as d:
 p=Path(d);(p/'test.c').write_text(source);subprocess.run(['cc','-D__EMSCRIPTEN__','-std=c99','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True);subprocess.run([str(p/'test')],check=True)
print('PASS: 600 static draws upload/convert once; exact RGBA/ARGB/BGRA bytes, layout phases, locks, writes, dynamic bypass, allocation failure and release')
