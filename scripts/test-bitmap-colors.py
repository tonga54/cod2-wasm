#!/usr/bin/env python3
"""Exercise the actual IWI BGR expansion and WebGL upload conversion."""
from pathlib import Path
import subprocess
import tempfile
import zipfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/gfx_d3d/r_image_load_obj.c').read_text()
start = source.index('                    for (p = 0; p < mipPixels; p++) {')
end = source.index('\n                    Image_UploadData', start)
loop = source[start:end]
with zipfile.ZipFile(root / 'data/browser/main/cod2_browser_renderer.iwd') as archive:
    tracer = archive.read('images/tracer.iwi')
assert tracer[:6] == b'IWi\x05\x02\x00'
assert len(tracer[28:]) == (1 + 4 + 16 + 64 + 256) * 3

test = r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "downstream/wasm/web_pixels.h"
typedef unsigned char byte;
static void expand(byte *dst, const byte *src, int mipPixels) {
 int p;
LOOP
}
static const byte asset[] = {ASSET};
int main(void) {
 byte bgr[1024*3], converted[1024*4+2], rgba[1024*4];
 /* All original tracer mip pixels, then every byte value in each channel. */
 for(int pass=0; pass<2; pass++) {
  int count=pass ? 1024 : sizeof(asset)/3;
  for(int i=0;i<count*3;i++) bgr[i]=pass ? (byte)(i*37+i/3) : asset[i];
  memset(converted,0xa5,sizeof(converted));
  expand(converted+1,bgr,count);
  assert(converted[0]==0xa5 && converted[count*4+1]==0xa5);
  web_bgra_to_rgba(rgba,converted+1,count);
  for(int i=0;i<count;i++) {
   assert(rgba[i*4]==bgr[i*3+2]);
   assert(rgba[i*4+1]==bgr[i*3+1]);
   assert(rgba[i*4+2]==bgr[i*3]);
   assert(rgba[i*4+3]==255);
  }
 }
 return 0;
}
'''.replace('LOOP', loop).replace('ASSET', ','.join(map(str, tracer[28:])))
with tempfile.TemporaryDirectory(prefix='cod2-bitmap-colors-') as folder:
    path = Path(folder)
    (path / 'test.c').write_text(test)
    subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                    '-I', str(root), str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('PASS: all 341 original tracer pixels plus 1,024 BGR samples preserve RGB, opacity and buffer bounds')
