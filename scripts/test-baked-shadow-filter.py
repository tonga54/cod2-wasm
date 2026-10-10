#!/usr/bin/env python3
"""Check sun-map reduction, constant illumination and all atlas boundaries."""
from pathlib import Path
import re,subprocess,tempfile
root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/gfx_d3d/r_loadworld_new.c').read_text()
match=re.search(r'static float R_FilterSunVisibility\([^;]*?\)\n\{',source)
end,depth=match.end(),1
while depth:
    depth+=(source[end]=='{')-(source[end]=='}');end+=1
body=source[match.start():end]
assert 'R_FilterSunVisibility(hiResPtr, pixWidth * 2)' in source
assert 'hiResPtr += 2;' in source and 'hiResBase + y * pixWidth * 4' in source
support=r'''
#include <assert.h>
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
  }
  for(int value=0;value<256;value++){
   memset(pixels,value,(size_t)stride*height*2);
   assert(R_FilterSunVisibility(pixels,stride)==value);
   assert(R_FilterSunVisibility(pixels+(height*2-2)*stride+stride-2,stride)==value);
  }
  free(pixels);
 }
 byte edge[]={0,255,0,255};assert(R_FilterSunVisibility(edge,2)==127.5f);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-baked-shadow-') as tmp:
    p=Path(tmp);(p/'test.c').write_text(support+body+checks)
    subprocess.run(['cc','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
print('PASS: full 2x2 sun footprint, constant light/shadow intensity and atlas edges under ASan/UBSan; no frame shader/pass changes')
