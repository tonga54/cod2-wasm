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
typedef struct {void **vtable;unsigned refCount,lengthBytes;unsigned char*data;unsigned usage,webBuffer;int webDirty;}CDirect3DVertexBufferClean;
typedef CDirect3DVertexBufferClean CDirect3DVertexBuffer;typedef int HRESULT;typedef unsigned UINT32,DWORD;typedef int D3DPOOL;
static void *vtbl_CDirect3DVertexBuffer[16];static int generated,uploaded,deleted,bound;
static void glGenBuffers(int n,unsigned*id){generated++;*id=123;}
static void glDeleteBuffers(int n,const unsigned*id){assert(n==1&&*id==123);deleted++;}
static void glBindBuffer(unsigned target,unsigned id){assert(target==0x8892&&id==123);bound++;}
static void glBufferData(unsigned t,ptrdiff_t len,const void*d,unsigned usage){assert(t==0x8892&&len==1024&&d&&usage==0x88e4);uploaded++;}
'''
checks=r'''
int main(void){CDirect3DVertexBufferClean vb={0};
 CDirect3DVertexBuffer_CDirect3DVertexBuffer(&vb,1024,8,0);
 for(int i=0;i<600;i++)assert(CDirect3DVertexBuffer_BindStaticWeb(&vb)==123);
 assert(generated==1&&uploaded==1&&bound==600);
 CDirect3DVertexBuffer_Unlock(&vb);assert(CDirect3DVertexBuffer_BindStaticWeb(&vb)==123);assert(uploaded==2&&generated==1);
 vb.usage=512;assert(!CDirect3DVertexBuffer_BindStaticWeb(&vb));assert(uploaded==2&&bound==601);
 ZN21CDirect3DVertexBufferD1Ev(&vb);assert(deleted==1&&!vb.data&&!vb.webBuffer);
 CDirect3DVertexBuffer_CDirect3DVertexBuffer(&vb,1024,512,0);assert(!CDirect3DVertexBuffer_BindStaticWeb(&vb));ZN21CDirect3DVertexBufferD1Ev(&vb);assert(deleted==1);
 return 0;
}
'''
source=support+extract('CDirect3DVertexBuffer_BindStaticWeb')+extract('CDirect3DVertexBuffer_Unlock')+extract('ZN21CDirect3DVertexBufferD1Ev')+extract('CDirect3DVertexBuffer_CDirect3DVertexBuffer')+checks
with tempfile.TemporaryDirectory(prefix='cod2-static-vbo-') as d:
 p=Path(d);(p/'test.c').write_text(source);subprocess.run(['cc','-D__EMSCRIPTEN__','-std=c99','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True);subprocess.run([str(p/'test')],check=True)
print('PASS: 600 static draws upload once; writes invalidate; dynamic buffers bypass cache; destruction releases GPU storage')
