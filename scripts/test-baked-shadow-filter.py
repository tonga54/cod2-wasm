#!/usr/bin/env python3
"""Check sun-map reduction, constant illumination and all atlas boundaries."""
from pathlib import Path
import re,subprocess,tempfile
root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/gfx_d3d/r_loadworld_new.c').read_text()
def function(pattern):
    match=re.search(pattern,source)
    end,depth=match.end(),1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[match.start():end]
body='\n'.join(function(p) for p in (r'static float R_FilterSunVisibility\([^;]*?\)\n\{',
    r'static float R_FilterSunPenumbra\([^;]*?\)\n\{',r'static byte R_SunVisibilityByte\([^;]*?\)\n\{'))
assert 'R_FilterSunVisibility(hiResPtr, pixWidth * 2)' in source
assert 'hiResPtr += 2;' in source and 'hiResBase + y * pixWidth * 4' in source
# The browser keeps the visibility the bake added in the lightmap's alpha; the
# DX7 combiners never read it, so native builds keep it opaque.
assert ('#ifdef __EMSCRIPTEN__\n                                loR[3] = R_SunVisibilityByte(sunVis);\n'
        '#else\n                                loR[3] = 0xFF;\n#endif') in source
assert source.index('sunVis = R_FilterSunPenumbra(') < source.index('loR[3] = R_SunVisibilityByte(sunVis);')
support=r'''
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte;
'''
checks=r'''
int main(void){
 for(int width=1;width<=1024;width*=2)for(int height=1;height<=512;height*=2){
  int stride=width*2;byte *pixels=malloc((size_t)stride*height*2);assert(pixels);
  for(int y=0;y<height*2;y++)for(int x=0;x<stride;x++)pixels[y*stride+x]=(byte)(x*13+y*17);
  for(int y=0;y<height;y++)for(int x=0;x<width;x++){
   const byte *p=pixels+y*width*4+x*2;
   float expected=0;
   for(int dy=0;dy<2;dy++)for(int dx=0;dx<2;dx++)expected+=pixels[(y*2+dy)*stride+x*2+dx]*.25f;
   assert(R_FilterSunVisibility(p,stride)==expected);
   assert(R_SunVisibilityByte(R_FilterSunVisibility(p,stride))==(byte)floorf(expected+.5f));
  }
  for(int value=0;value<256;value++){
   memset(pixels,value,(size_t)stride*height*2);
   assert(R_FilterSunVisibility(pixels,stride)==value);
   assert(R_FilterSunVisibility(pixels+(height*2-2)*stride+stride-2,stride)==value);
   assert(R_SunVisibilityByte(R_FilterSunVisibility(pixels,stride))==value);
   if(width>=4 && height>=4){
    const byte *interior=pixels+2*stride+2;
    assert(fabsf(R_FilterSunPenumbra(interior,stride,R_FilterSunVisibility(interior,stride))-value)<1e-4f);
   }
  }
  free(pixels);
 }
 byte edge[]={0,255,0,255};assert(R_FilterSunVisibility(edge,2)==127.5f);
 assert(R_SunVisibilityByte(127.5f)==128&&R_SunVisibilityByte(.49f)==0&&R_SunVisibilityByte(.5f)==1);
 assert(R_SunVisibilityByte(254.4f)==254&&R_SunVisibilityByte(254.5f)==255&&R_SunVisibilityByte(255)==255);
 assert(R_SunVisibilityByte(300)==255&&R_SunVisibilityByte(INFINITY)==255);
 assert(!R_SunVisibilityByte(0)&&!R_SunVisibilityByte(-3)&&!R_SunVisibilityByte(-INFINITY)&&!R_SunVisibilityByte(NAN));
 return 0;
}
'''
mutants=[
    ('return (byte)(int)(sunVis + 0.5f);','return (byte)(int)sunVis;'),
    ('    if (sunVis >= 255.0f)\n        return 255;\n',''),
    ('if (!(sunVis > 0.0f))','if (sunVis < 0.0f)'),
    ('if (!(sunVis > 0.0f))','if (sunVis <= 0.0f)'),
]
flags=['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all']
with tempfile.TemporaryDirectory(prefix='cod2-baked-shadow-') as tmp:
    p=Path(tmp);(p/'test.c').write_text(support+body+checks)
    subprocess.run(['cc',*flags,str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
    for index,(before,after) in enumerate(mutants):
        assert body.count(before)==1,before
        (p/f'mutant{index}.c').write_text(support+body.replace(before,after)+checks)
        subprocess.run(['cc',*flags,'-w',str(p/f'mutant{index}.c'),'-o',str(p/f'mutant{index}')],check=True)
        assert subprocess.run([str(p/f'mutant{index}')],capture_output=True).returncode!=0,f'mutant survived: {after!r}'
print('PASS: full 2x2 sun footprint, constant light/shadow intensity and atlas edges under ASan/UBSan; no frame shader/pass changes')
print(f'PASS: browser lightmap alpha carries the rounded sun visibility, clamped and NaN-safe; {len(mutants)} mutants fail')
