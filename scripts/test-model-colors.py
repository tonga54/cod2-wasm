#!/usr/bin/env python3
"""Run the renderer's real color conversion and light handoff under sanitizers."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
device = (root / 'src/Mac/DirectX_9/CDirect3DDevice.c').read_text()
cache = (root / 'src/PC/gfx_d3d/r_staticmodelcache.c').read_text()
converter = (root / 'src/Mac/DirectX_9/CColorConverter.c').read_text()

def function(source, name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'

static_color = cache[cache.index('                        float oneOver255 ='):cache.index('\n                    }', cache.index('                        float oneOver255 ='))]
functions = ''.join(function(device, name) for name in (
    'CDirect3DDevice_ColorByteOrder', 'CDirect3DDevice_ConvertColorArray',
    'CDirect3DDevice_SetMaterial', 'CDirect3DDevice_SetLight',
    'CDirect3DDevice_LightEnable', 'CDirect3DDevice_GetLightEnable',
    'CDirect3DDevice_ApplyModelLights'))
functions += function((root/'src/PC/gfx_d3d/rb_shade.c').read_text(), 'RB_CopyVerticesWithColorConvert')
functions += function(converter, 'CColorConverter_ByteSwap32')
functions += function(converter, 'StdConverterARGB_Convert')
functions += '\nstatic void StaticColor(byte *dstColor,const byte *srcColor,float r,float g,float b){\n'+static_color+'\n}\n'

support = r'''
#define __EMSCRIPTEN__ 1
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte;
typedef unsigned int UINT,UINT32,DWORD;
typedef int BOOL,HRESULT;
typedef void StdConverterARGB;
typedef struct {float r,g,b,a;} D3DCOLORVALUE;
typedef struct {float x,y,z;} D3DVECTOR;
typedef struct {int Type;D3DCOLORVALUE Diffuse,Specular,Ambient;D3DVECTOR Position,Direction;float Range,Falloff,Attenuation0,Attenuation1,Attenuation2,Theta,Phi;} D3DLIGHT9;
typedef struct {D3DCOLORVALUE Diffuse,Ambient,Specular,Emissive;float Power;} D3DMATERIAL9;
typedef struct {union{byte bytes[0x5A0];struct{D3DLIGHT9 lights[8];BOOL enabled[8];D3DMATERIAL9 material;}state;}lightData;} DeviceImpl;
typedef DeviceImpl CDirect3DDevice;
enum { COLOR_BYTES_RGBA,COLOR_BYTES_BGRA,COLOR_BYTES_ARGB };
static struct { int projection2D; } backEnd;
static byte *g_colorArrayScratch;static UINT g_colorArrayScratchCapacity;
static float viewSent[16],ambientSent[4],diffuseSent[4],lightDirections[8][4],lightAmbient[8][4],lightDiffuse[8][4];
static int lightsEnabled[8],lightingEnabled;
static void glLoadMatrixf(const float *view){memcpy(viewSent,view,64);}
static void glLightModelfv(unsigned int p,const float *v){assert(p==0x0B53);for(int i=0;i<4;i++)assert(v[i]==0);}
static void glMaterialfv(unsigned int face,unsigned int p,const float *v){
 assert(face==0x408);if(p==0x1200)memcpy(ambientSent,v,16);else if(p==0x1201)memcpy(diffuseSent,v,16);else {assert(p==0x1202);for(int i=0;i<4;i++)assert(v[i]==0);}
}
static void glEnable(unsigned int p){if(p==0xB50)lightingEnabled=1;else{assert(p>=0x4000&&p<0x4008);lightsEnabled[p-0x4000]=1;}}
static void glDisable(unsigned int p){assert(p>=0x4000&&p<0x4008);lightsEnabled[p-0x4000]=0;}
static void glLightfv(unsigned int id,unsigned int p,const float *v){
 assert(id>=0x4000&&id<0x4008);id-=0x4000;
 if(p==0x1200)memcpy(lightAmbient[id],v,16);else if(p==0x1201)memcpy(lightDiffuse[id],v,16);else if(p==0x1203)memcpy(lightDirections[id],v,16);else assert(p==0x1202);
}
'''
checks = r'''
int main(void){
 /* Dynamic smoke and HUD uploads must preserve alpha and all color channels. */
 for(int stride=32;stride<=68;stride+=4){
  if(stride!=32&&stride!=36&&stride!=68)continue;
  byte src[68*256],dst[68*256];for(int i=0;i<stride*256;i++)src[i]=(byte)(i*37);
  RB_CopyVerticesWithColorConvert(src,dst,256,stride,stride==32?12:24,0);
  assert(!memcmp(src,dst,stride*256));
  backEnd.projection2D=1;
  int ofs=stride==32?12:24;
  RB_CopyVerticesWithColorConvert(src,dst,256,stride,ofs,0);
  for(int n=0;n<256;n++)for(int c=0;c<4;c++)
   assert(dst[n*stride+ofs+c]==src[n*stride+ofs+(c+1)%4]);
  backEnd.projection2D=0;
 }

 for(int stride=24;stride<=68;stride+=4){
  if(stride!=24&&stride!=32&&stride!=36&&stride!=64&&stride!=68)continue;
  int order=CDirect3DDevice_ColorByteOrder(stride,3);
  assert(order==((stride==32||stride==36||stride==68)?COLOR_BYTES_RGBA:COLOR_BYTES_ARGB));
  byte vertices[257*68];memset(vertices,0xcc,sizeof(vertices));
  for(int n=0;n<257;n++){byte *v=vertices+n*stride+12;v[0]=n%256;v[1]=17;v[2]=93;v[3]=201;}
  const byte *out=CDirect3DDevice_ConvertColorArray(vertices,stride,12,257,order);
  if(order==COLOR_BYTES_RGBA)assert(!out);else for(int n=0;n<257;n++)assert(out[n*4]==17&&out[n*4+1]==93&&out[n*4+2]==201&&out[n*4+3]==n%256);
 }
 assert(CDirect3DDevice_ColorByteOrder(64,4)==COLOR_BYTES_BGRA);
 for(int a=0;a<256;a++)for(int n=0;n<32;n++){
  byte original[4]={(n*17)%256,(n*93)%256,(n*201)%256,a},bgra[4],argb[4];
  float r=.25f+(n%5)*.5f,g=.15f+(n%3)*.5f,b=.05f+(n%7)*.5f;
  StaticColor(bgra,original,r,g,b);StdConverterARGB_Convert(0,argb,bgra);
  const byte *rgba=CDirect3DDevice_ConvertColorArray(argb,4,0,1,COLOR_BYTES_ARGB);
  assert(rgba[0]==(int)fminf(255,floorf(r*original[0]+.5f)));
  assert(rgba[1]==(int)fminf(255,floorf(g*original[1]+.5f)));
  assert(rgba[2]==(int)fminf(255,floorf(b*original[2]+.5f)));
  assert(rgba[3]==a);
 }
 struct {uint64_t before;DeviceImpl dev;uint64_t after;} guard={.before=0xabcdef,.after=0xfedcba};
 DeviceImpl *dev=&guard.dev;
 D3DMATERIAL9 material={.Ambient={1,.5f,.25f,1},.Diffuse={.2f,.4f,.6f,.8f}};
 assert(!CDirect3DDevice_SetMaterial(dev,&material));
 assert(CDirect3DDevice_SetMaterial(dev,0));
 for(int mask=0;mask<256;mask++){
  for(int i=0;i<8;i++){
   D3DLIGHT9 light={.Type=3,.Ambient={i*.01f,.1f,.2f,1},.Diffuse={.3f,.4f,i*.01f,1},.Direction={i+1,i+2,i+3}};
   assert(!CDirect3DDevice_SetLight(dev,i,&light));
   assert(!CDirect3DDevice_LightEnable(dev,i,mask&(1<<i)));
   BOOL enabled;assert(!CDirect3DDevice_GetLightEnable(dev,i,&enabled));assert(enabled==!!(mask&(1<<i)));
  }
  float view[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
  CDirect3DDevice_ApplyModelLights(dev,view);
  assert(lightingEnabled&&!memcmp(viewSent,view,64));
  assert(!memcmp(ambientSent,&material.Ambient,16)&&!memcmp(diffuseSent,&material.Diffuse,16));
  for(int i=0;i<8;i++){
   assert(lightsEnabled[i]==!!(mask&(1<<i)));
   if(lightsEnabled[i]){for(int j=0;j<3;j++)assert(lightDirections[i][j]==-(float)(i+j+1));assert(!lightDirections[i][3]);}
  }
 }
 assert(CDirect3DDevice_SetLight(dev,8,&dev->lightData.state.lights[0]));
 assert(CDirect3DDevice_SetLight(dev,0,0));assert(CDirect3DDevice_LightEnable(dev,8,1));
 BOOL enabled;assert(CDirect3DDevice_GetLightEnable(dev,8,&enabled));assert(CDirect3DDevice_GetLightEnable(dev,0,0));
 assert(guard.before==0xabcdef&&guard.after==0xfedcba);
 free(g_colorArrayScratch);return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-model-colors-') as folder:
    path=Path(folder)
    (path/'test.c').write_text(support+functions+checks)
    subprocess.run(['cc','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(path/'test.c'),'-lm','-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
print('PASS: model/world color layouts, 8192 static color/alpha cases and all 256 eight-light enable masks under ASan/UBSan')
