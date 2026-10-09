#!/usr/bin/env python3
"""Bound the real model-cache conversion range under ASan/UBSan."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parent.parent
s=(root/'src/Mac/DirectX_9/CDirect3DDevice.c').read_text()
m=re.search(r'static const unsigned short \*CDirect3DDevice_IndexRange\([^;]+?\)\n\{',s)
e=m.end();depth=1
while depth:depth+=(s[e]=='{')-(s[e]=='}');e+=1
code='''#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef unsigned int UINT;
static unsigned short *g_indexArrayScratch;
static UINT g_indexArrayScratchCapacity;
'''+s[m.start():e]+'''
int main(void){
 unsigned short in[513],copy[513];unsigned lo,count;
 for(unsigned base=0;base<65536;base+=61){
  for(unsigned n=1;n<=513;n+=16){
   for(unsigned i=0;i<n;i++)in[i]=(base+i*7)%65536;
   memcpy(copy,in,sizeof(in));
   const unsigned short *out=CDirect3DDevice_IndexRange(in,n,&lo,&count);
   assert(out&&count>0&&count<=65536);
   unsigned actualLo=65535,actualHi=0;
   for(unsigned i=0;i<n;i++){assert(out[i]+lo==in[i]);assert(out[i]<count);if(in[i]<actualLo)actualLo=in[i];if(in[i]>actualHi)actualHi=in[i];}
   assert(lo==actualLo&&count==actualHi-actualLo+1);assert(!memcmp(copy,in,sizeof(in)));
  }
 }
 unsigned short cached[]={64000,64001,64002,64002,64001,64003};
 assert(CDirect3DDevice_IndexRange(cached,6,&lo,&count));assert(lo==64000&&count==4);
 assert(!CDirect3DDevice_IndexRange(cached,0,&lo,&count));
 free(g_indexArrayScratch);puts("PASS: 35,475 indexed mesh ranges, unchanged inputs and 65,536-to-4 vertex cache reduction");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'test.c';p.write_text(code)
 subprocess.run(['cc','-std=c11','-fsanitize=address,undefined','-g',str(p),'-o',str(Path(d)/'test')],check=True)
 subprocess.run([str(Path(d)/'test')],check=True)
