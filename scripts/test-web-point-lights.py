#!/usr/bin/env python3
"""Validate production point-light packing under ASan/UBSan."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root/'downstream/wasm/web_point_lights.c').read_text()
start = source.index('void WebPointLights_SetScene(')
end = source.index('\nEM_JS', start)
shadow_start = source.index('void WebPointLights_SetShadowCasters(')
shadow_end = len(source)
support = r'''
#include <assert.h>
#include <math.h>
#include <string.h>
typedef struct {void *def;float position[4],color[3];} GfxLight;
typedef struct {int reType,renderFxFlags;struct {float baseCoords[3];} lighting;float origin[3];} GfxEntity;
static float output[32];static int outputCount;
static float shadowOutput[16];static int shadowOutputCount;
static void WebPointLights_Set(const float *packed,int count) {
 assert(count>=0&&count<=4);outputCount=count;memcpy(output,packed,count*8*sizeof(float));
}
static void WebPointLights_SetShadows(const float *packed,int count) {
 assert(count>=0&&count<=4);shadowOutputCount=count;memcpy(shadowOutput,packed,count*4*sizeof(float));
}
'''
checks = r'''
int main(void) {
 GfxLight lights[32]={0};
 for(int i=0;i<32;i++){lights[i].position[0]=i;lights[i].position[3]=200+i;lights[i].color[0]=1;lights[i].color[1]=-.2;}
 for(int count=0;count<=32;count++) {
  WebPointLights_SetScene(lights,count);assert(outputCount==(count<4?count:4));
  for(int i=0;i<outputCount;i++){assert(output[i*8]==i);assert(output[i*8+3]==200+i);assert(output[i*8+4]==1);assert(!output[i*8+5]);assert(!output[i*8+7]);}
 }
 for(int field=0;field<7;field++)for(int invalid=0;invalid<3;invalid++){
  GfxLight copy[32];memcpy(copy,lights,sizeof(copy));
  float *slot=field<4?copy[0].position+field:copy[0].color+field-4;
  *slot=invalid==0?NAN:invalid==1?INFINITY:-INFINITY;
  WebPointLights_SetScene(copy,32);assert(outputCount==4&&output[0]==1);
 }
 lights[0].position[3]=0;WebPointLights_SetScene(lights,1);assert(!outputCount);
 lights[0].position[3]=-1;WebPointLights_SetScene(lights,1);assert(!outputCount);
 WebPointLights_SetScene(0,0);assert(!outputCount);

 GfxEntity ents[8]={0};float viewOrg[3]={0,0,0};
 for(int i=0;i<6;i++){
  ents[i].reType=0;ents[i].renderFxFlags=0x80;
  ents[i].origin[0]=(float)((6-i)*100);
  ents[i].lighting.baseCoords[2]=(i%3==0)?12.f:(i%3==1?20.f:32.f);
 }
 ents[6].reType=0;ents[6].renderFxFlags=0x8c; /* Viewmodel must not cast a world floor shadow. */
 ents[7].reType=2;ents[7].renderFxFlags=0x80; /* Static models use baked lightmaps. */
 WebPointLights_SetShadowCasters(ents,8,viewOrg);
 assert(shadowOutputCount==4);
 assert(shadowOutput[0]==100.f && shadowOutput[3]==22.f);
 assert(shadowOutput[4]==200.f && shadowOutput[7]==25.f);
 assert(shadowOutput[8]==300.f && shadowOutput[11]==30.f);
 assert(shadowOutput[12]==400.f && shadowOutput[15]==22.f);
 ents[5].origin[0]=2000.f; /* Beyond 1600-unit shadow cull distance. */
 WebPointLights_SetShadowCasters(ents,8,viewOrg);
 assert(shadowOutputCount==4 && shadowOutput[0]==200.f);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-point-lights-') as tmp:
    path=Path(tmp); (path/'test.c').write_text(support+source[start:end]+source[shadow_start:shadow_end]+checks)
    subprocess.run(['cc','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(path/'test.c'),'-lm','-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
print('PASS: point lights bound to four, invalid values rejected, original radius/colors preserved under ASan/UBSan')
