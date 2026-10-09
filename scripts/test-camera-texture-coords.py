#!/usr/bin/env python3
"""Check the D3D camera-position texgen used by the sky cube material."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parent.parent
s=(root/'src/Mac/DirectX_9/CDirect3DDevice.c').read_text()
a=s.index('static const float *CDirect3DDevice_CameraTexCoords(')
b=s.index('\nstatic GLenum CDirect3DDevice_MapCompareFunc',a)
body=s[a:b]
support='''#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
typedef unsigned char byte;typedef unsigned int UINT;
static float *g_texCoordScratch;static UINT g_texCoordScratchCapacity;
'''
checks=r'''
int main(void){
 float verts[64][8],matrix[16]={0};
 for(int i=0;i<64;i++){verts[i][0]=-123;verts[i][1]=100+i*3;verts[i][2]=-50+i;verts[i][3]=200-i*2;}
 for(int angle=0;angle<360;angle+=15) {
  float c=cosf(angle*.0174532925f),s=sinf(angle*.0174532925f);
  matrix[0]=c;matrix[1]=s;matrix[4]=-s;matrix[5]=c;matrix[10]=1;matrix[15]=1;
  matrix[12]=25;matrix[13]=-80;matrix[14]=-100;
  const float *v=CDirect3DDevice_CameraTexCoords((byte *)verts,32,4,64,matrix);
  for(int i=0;i<64;i++) {
   assert(fabsf(v[3*i]-(verts[i][1]*c-verts[i][2]*s+25))<.0001f);
   assert(fabsf(v[3*i+1]-(verts[i][1]*s+verts[i][2]*c-80))<.0001f);
   assert(v[3*i+2]==verts[i][3]-100);
  }
  assert(CDirect3DDevice_CameraTexCoords((byte *)verts,32,4,1,matrix)==v);
 }
 free(g_texCoordScratch);return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-camera-coords-') as d:
 p=Path(d);(p/'test.c').write_text(support+body+checks)
 subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: 1,536 camera-space texture coordinates, translated/rotated views, interleaved vertices and scratch reuse')
