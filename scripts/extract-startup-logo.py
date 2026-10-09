#!/usr/bin/env python3
"""Decode the owner's original logo texture to a private PNG for browser startup."""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile
import zipfile
import zlib

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('archive', type=Path, help='Original iw_09.iwd')
parser.add_argument('--output', type=Path, default=root / 'data/browser/web/cod2-startup.png')
args = parser.parse_args()
with zipfile.ZipFile(args.archive) as archive:
    source = archive.read('images/logo_cod2.iwi')
if source[:5] != b'IWi\x05\x06':
    raise SystemExit('Expected the original wavelet RGBA logo texture')
width, height = struct.unpack_from('<HH', source, 6)
if (width, height) != (512, 128):
    raise SystemExit('Unexpected logo dimensions')
# Reuse the engine decoder and its original Huffman tables, without modifying pixels.
codec = (root / 'src/PC/gfx_d3d/r_image_wavelet.c').read_text()
codec = '\n'.join(line for line in codec.splitlines() if not line.startswith('#include'))
header = '''
#include <stdio.h>
#include <stdlib.h>
typedef unsigned char byte, Bool;
typedef struct { unsigned short value,bit; const byte *data; int width,height,channels,bpp,mipLevel; Bool dataInitialized; } WaveletDecode;
typedef struct { short value,bits; } WaveletHuffmanDecode;
'''
main = r'''
int main(int argc,char **argv) {
 if(argc!=3)return 1;
 FILE *file=fopen(argv[1],"rb");if(!file)return 2;
 fseek(file,0,SEEK_END);long len=ftell(file);rewind(file);
 byte *input=calloc((size_t)len+16,1);if(fread(input,1,len,file)!=(size_t)len)return 3;fclose(file);
 WaveletDecode decode={0};decode.data=input+28;decode.width=512;decode.height=128;decode.channels=decode.bpp=4;
 byte *previous=NULL;
 for(int level=9;level>=0;--level){
  int w=512>>level,h=128>>level;if(w<1)w=1;if(h<1)h=1;
  byte *next=calloc(w*h,4);decode.mipLevel=level;
  Wavelet_DecompressLevel(previous,next,&decode);free(previous);previous=next;
 }
 file=fopen(argv[2],"wb");if(!file)return 4;
 fwrite(previous,4,512*128,file);fclose(file);free(previous);free(input);return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-logo-') as directory:
    temporary = Path(directory)
    (temporary/'logo.iwi').write_bytes(source)
    (temporary/'decode.c').write_text(header+codec+main)
    subprocess.run(['cc','-O2',str(temporary/'decode.c'),'-o',str(temporary/'decode')],check=True)
    subprocess.run([str(temporary/'decode'),str(temporary/'logo.iwi'),str(temporary/'logo.bgra')],check=True)
    bgra = (temporary/'logo.bgra').read_bytes()
rgba = bytearray(bgra)
rgba[0::4], rgba[2::4] = bgra[2::4], bgra[0::4]
def chunk(kind, data):
    return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
rows = b''.join(b'\0'+rgba[y*width*4:(y+1)*width*4] for y in range(height))
png = b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b'')
args.output.parent.mkdir(parents=True,exist_ok=True)
args.output.write_bytes(png)
print(f'Original logo decoded to {args.output} ({width}x{height}, {len(png)} bytes)')
