#!/usr/bin/env python3
"""Texture handles must not depend on a native executable's address range."""
from pathlib import Path
import re,subprocess,tempfile
root=Path(__file__).resolve().parent.parent
s=(root/'src/Mac/DirectX_9/CDirect3DDevice.c').read_text()
m=re.search(r'static unsigned int CDirect3DDevice_GetTextureGLId\([^;]*?\)\n\{',s);e=m.end();d=1
while d:d+=(s[e]=='{')-(s[e]=='}');e+=1
body=s[m.start():e]
pre='''#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char byte;
typedef void IDirect3DBaseTexture9;
void *vtbl_CDirect3DTexture[1],*vtbl_CDirect3DCubeTexture[1];
'''
checks='''
int main(void){
 _Alignas(void*) byte texture[5*sizeof(void*)+68]={0};
 assert(!CDirect3DDevice_GetTextureGLId(NULL));
 assert(!CDirect3DDevice_GetTextureGLId(texture));
 for(int cube=0;cube<2;cube++)for(unsigned id=1;id<4096;id++){
  *(void***)texture=cube?vtbl_CDirect3DCubeTexture:vtbl_CDirect3DTexture;
  *(unsigned*)(texture+5*sizeof(void*)+64)=id;
  assert(CDirect3DDevice_GetTextureGLId(texture)==id);
 }
 puts("PASS: 8,190 typed 2D/cube texture handles and no native-address cutoff");
}
'''
assert '0x08000000' not in body
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'test.c';p.write_text(pre+body+checks)
 subprocess.run(['cc','-fsanitize=address,undefined',str(p),'-o',str(Path(d)/'test')],check=True)
 subprocess.run([str(Path(d)/'test')],check=True)
