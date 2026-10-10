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
support = r'''
#include <assert.h>
#include <math.h>
#include <string.h>
typedef struct {void *def;float position[4],color[3];} GfxLight;
static float output[32];static int outputCount;
static void WebPointLights_Set(const float *packed,int count) {
 assert(count>=0&&count<=4);outputCount=count;memcpy(output,packed,count*8*sizeof(float));
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
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-point-lights-') as tmp:
    path=Path(tmp); (path/'test.c').write_text(support+source[start:end]+checks)
    subprocess.run(['cc','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(path/'test.c'),'-lm','-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
print('PASS: point lights bound to four, invalid values rejected, original radius/colors preserved under ASan/UBSan')
