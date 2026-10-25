#!/usr/bin/env python3
"""Wavelet-compressed IWi images (formats 6 to 10) upload the bytes their bitmaps would: Wavelet_DecompressLevel writes
each texel as B, G, R, A, the order of the bitmap formats and of the D3D surfaces, which the WebGL path turns into RGBA.
Image_LoadWavelet reversed each level's four-byte groups before the upload, a step of the Mac build: RGB textures took
the alpha byte as blue, so Matmata's well and Toujane's vases came out blue, and a level smaller than four bytes of a
one- or two-byte format was copied into a buffer rounded down to none. The real loaders and decoder run here on
generated images, their streams coded with the decoder's own Huffman tables."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
loaders = (root / 'src/PC/gfx_d3d/r_image_load_obj.c').read_text()
wavelet = (root / 'src/PC/gfx_d3d/r_image_wavelet.c').read_text()
assert wavelet.startswith('#include "common_types.h"\n#include "imports.h"\n')
wavelet = wavelet.split('\n', 2)[2]


def definition(name):
    match = re.search(r'^(?:static )?void (?:__attribute_regparm__\(3\) )?' + name + r'\([^;{]*\)\n\{', loaders, re.M)
    assert match, name
    start, depth, end = match.start(), 1, match.end()
    while depth:
        depth += (loaders[end] == '{') - (loaders[end] == '}')
        end += 1
    return loaders[start:end] + '\n'


functions = ''.join(definition(name) for name in ('Image_LoadBitmap', 'Image_LoadDxtc', 'Image_LoadWavelet',
                                                  'Image_LoadFromData'))
mip_count = re.search(r'static int Image_ComputeMipCount\(int w, int h, int d\)\n\{.*?\n\}\n', loaders, re.S)[0]


def table(name):
    body = re.search(r'const WaveletHuffmanDecode ' + name + r'\[4096\] = \{(.*?)\};', wavelet, re.S)[1]
    entries = [(int(value), int(bits)) for value, bits in re.findall(r'\{ (-?\d+), (\d+) \}', body)]
    assert len(entries) == 4096, name
    return entries


def code(entries, value):
    """The shortest bits that decode to value, whatever bits follow them."""
    best = None
    for index, (decoded, bits) in enumerate(entries):
        mask = (1 << bits) - 1
        if decoded == value and (best is None or bits < best[1]) and all(
                entries[other] == (value, bits) for other in range(index & mask, 4096, 1 << bits)):
            best = (index & mask, bits)
    assert best, value
    return [best[0] >> bit & 1 for bit in range(best[1])]


blue, red_green, alpha = table('waveletDecodeBlue'), table('waveletDecodeRedGreen'), table('waveletDecodeAlpha')


def stream(with_alpha):
    """One 2 by 2 block from the 1 by 1 level: blue +2 then 0, 0, the red and green differences -2 then 0, 0 so that
    only blue changes, and alpha +2 then 0, 0; each parity bit 0. Bits go least significant first."""
    bits = [0, 0] + code(blue, 2) + code(blue, 0) * 2
    for _ in range(2):
        bits += [0] + code(red_green, -2) + code(red_green, 0) * 2
    if with_alpha:
        bits += [0] + code(alpha, 2) + code(alpha, 0) * 2
    bits += [0] * (-len(bits) % 8)
    return [sum(bits[i + bit] << bit for bit in range(8)) for i in range(0, len(bits), 8)]


def array(values):
    return '{ ' + ', '.join(str(value) for value in values) + ' }'


B, G, R, A = 0x40, 0x80, 0xc0, 0x60
rgb_top = [B + 1, G, R, B + 1, G, R, B - 1, G, R, B - 1, G, R]
rgba_top = [B + 1, G, R, A + 1, B + 1, G, R, A + 1, B - 1, G, R, A - 1, B - 1, G, R, A - 1]
cases = [
    # name, wavelet format, bitmap format, width, height, wavelet data, bitmap data, uploads (mip, bytes)
    ('RGB 1x1', 7, 2, 1, 1, [B, G, R], [B, G, R], [(0, [B, G, R, 255])]),
    ('RGBA 1x1', 6, 1, 1, 1, [B, G, R, A], [B, G, R, A], [(0, [B, G, R, A])]),
    ('RGB 2x2', 7, 2, 2, 2, [B, G, R] + stream(False), [B, G, R] + rgb_top,
     [(1, [B, G, R, 255]), (0, [value for i in range(4) for value in rgb_top[3 * i:3 * i + 3] + [255]])]),
    ('RGBA 2x2', 6, 1, 2, 2, [B, G, R, A] + stream(True), [B, G, R, A] + rgba_top,
     [(1, [B, G, R, A]), (0, rgba_top)]),
    ('luminance 4x1', 9, 4, 4, 1, [0x11, 0x21, 0x22, 0x31, 0x32, 0x33, 0x34],
     [0x11, 0x21, 0x22, 0x31, 0x32, 0x33, 0x34], [(2, [0x11]), (1, [0x21, 0x22]), (0, [0x31, 0x32, 0x33, 0x34])]),
]

support = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte;
typedef int Bool;
typedef unsigned int DWORD;
typedef int D3DFORMAT;
#define __attribute_regparm__(n)
typedef struct WaveletDecode {
    unsigned short value, bit;
    const byte *data;
    int width, height, channels, bpp, mipLevel;
    Bool dataInitialized;
} WaveletDecode;
typedef struct WaveletHuffmanDecode { short value, bits; } WaveletHuffmanDecode;
/* The fields the loaders read, the picmip at byte 8 as in the 32-bit build. */
typedef struct GfxImage { int mapType; int texture; struct { byte platform[2]; } picmip; unsigned short width, height, depth; } GfxImage;
typedef struct GfxImageFileHeader { char tag[3]; byte version, format, flags; short dimensions[3]; int fileSizeForPicmip[4]; } GfxImageFileHeader;

static int setups, liveTemp;
static D3DFORMAT setupFormat;
static void Image_Setup(GfxImage *image, int width, int height, int depth, int imageFlags, DWORD usage, D3DFORMAT imageFormat)
{
    (void)imageFlags; (void)usage;
    image->width = (unsigned short)width; image->height = (unsigned short)height; image->depth = (unsigned short)depth;
    setupFormat = imageFormat;
    ++setups;
}
typedef struct { D3DFORMAT format; int face, mip, size; byte bytes[64]; } Upload;
static Upload uploads[8];
static int uploadCount;
static void Image_UploadData(GfxImage *image, D3DFORMAT format, int face, int mipLevel, const byte *src)
{
    int w = image->width >> mipLevel, h = image->height >> mipLevel;
    Upload *upload = &uploads[uploadCount++];
    assert(uploadCount <= 8);
    upload->format = format; upload->face = face; upload->mip = mipLevel;
    upload->size = (w < 1 ? 1 : w) * (h < 1 ? 1 : h) * (format == 0x15 || format == 0x16 ? 4 : format == 0x33 ? 2 : 1);
    assert(upload->size <= 64);
    memcpy(upload->bytes, src, (size_t)upload->size);
}
static void *Hunk_AllocateTempMemoryInternal(int size) { ++liveTemp; return malloc((size_t)size); }
static void Hunk_FreeTempMemory(void *buffer) { --liveTemp; free(buffer); }
static void *__Znam(unsigned int size) { return malloc(size); }
static void __ZdaPv(void *buffer) { free(buffer); }
static int Image_CubemapFace(int faceIndex) { return faceIndex; }
'''

checks = r'''
typedef struct { int mip, size; byte bytes[64]; } Expected;
static byte file[512] __attribute__((aligned(4)));

static void load(int format, int width, int height, const byte *data, int size, Upload *out, int *count)
{
    GfxImage image;
    memset(file, 0, sizeof(file));
    memset(&image, 0, sizeof(image));
    memcpy(file, "IWi", 3);
    file[3] = 5;
    file[4] = (byte)format;
    ((GfxImageFileHeader *)file)->dimensions[0] = (short)width;
    ((GfxImageFileHeader *)file)->dimensions[1] = (short)height;
    ((GfxImageFileHeader *)file)->dimensions[2] = 1;
    memcpy(file + sizeof(GfxImageFileHeader), data, (size_t)size);
    setups = uploadCount = 0;
    Image_LoadFromData(&image, (GfxImageFileHeader *)file, file + sizeof(GfxImageFileHeader));
    assert(setups == 1 && !liveTemp);
    memcpy(out, uploads, sizeof(uploads));
    *count = uploadCount;
}

static void check(const char *name, int waveletFormat, int bitmapFormat, int width, int height, const byte *waveletData,
                  int waveletSize, const byte *bitmapData, int bitmapSize, const Expected *expected, int expectedCount)
{
    Upload fromWavelet[8], fromBitmap[8];
    int waveletCount, bitmapCount;
    load(waveletFormat, width, height, waveletData, waveletSize, fromWavelet, &waveletCount);
    D3DFORMAT waveletSurface = setupFormat;
    load(bitmapFormat, width, height, bitmapData, bitmapSize, fromBitmap, &bitmapCount);
    assert(setupFormat == waveletSurface);
    if (waveletCount != expectedCount || bitmapCount != expectedCount) {
        fprintf(stderr, "%s: %d and %d uploads\n", name, waveletCount, bitmapCount);
        exit(1);
    }
    for (int i = 0; i < expectedCount; ++i) {
        const Upload *w = &fromWavelet[i], *b = &fromBitmap[i];
        if (w->mip != expected[i].mip || w->size != expected[i].size || w->face || w->format != waveletSurface ||
            memcmp(w->bytes, expected[i].bytes, (size_t)w->size) || b->mip != w->mip || b->size != w->size ||
            memcmp(b->bytes, w->bytes, (size_t)w->size)) {
            fprintf(stderr, "%s: mip %d differs:", name, expected[i].mip);
            for (int k = 0; k < w->size; ++k)
                fprintf(stderr, " %02x/%02x", w->bytes[k], expected[i].bytes[k]);
            fputc('\n', stderr);
            exit(1);
        }
    }
}

int main(void)
{
CASES
    puts("ok");
    return 0;
}
'''

calls = []
for name, wavelet_format, bitmap_format, width, height, wavelet_data, bitmap_data, expected in cases:
    tag = re.sub(r'\W', '_', name)
    calls.append(f'    static const byte {tag}_wavelet[] = {array(wavelet_data)};\n'
                 f'    static const byte {tag}_bitmap[] = {array(bitmap_data)};\n'
                 f'    static const Expected {tag}_uploads[] = {{ ' +
                 ', '.join(f'{{ {mip}, {len(data)}, {array(data)} }}' for mip, data in expected) + ' };\n'
                 f'    check("{name}", {wavelet_format}, {bitmap_format}, {width}, {height}, {tag}_wavelet, '
                 f'(int)sizeof({tag}_wavelet), {tag}_bitmap, (int)sizeof({tag}_bitmap), {tag}_uploads, '
                 f'{len(expected)});\n')
checks = checks.replace('CASES\n', ''.join(calls))

upload = '            Image_UploadData(image, format, Image_CubemapFace(face), level - picmip, newAddr);\n'
assert functions.count(upload) == 1
mutants = [
    # The Mac build's step: a copy rounded down to whole four-byte groups, each reversed.
    (upload, '            void *pTemp = __Znam((unsigned int)(sizeForLevel / 4) * 4);\n'
             '            memcpy(pTemp, newAddr, sizeForLevel);\n'
             '            for (unsigned int u = 0; u < (unsigned int)sizeForLevel / 4; u++) {\n'
             '                DWORD v = ((DWORD *)pTemp)[u];\n'
             '                ((DWORD *)pTemp)[u] = (v >> 24) | (v << 24) | ((v << 8) & 0xff0000) | ((v >> 8) & 0xff00);\n'
             '            }\n'
             '            Image_UploadData(image, format, Image_CubemapFace(face), level - picmip, (const byte *)pTemp);\n'
             '            __ZdaPv(pTemp);\n'),
    # Red and blue exchanged.
    (upload, '            for (int t = 0; pixelStride == 4 && t < sizeForLevel; t += 4) {\n'
             '                byte s = newAddr[t]; newAddr[t] = newAddr[t + 2]; newAddr[t + 2] = s;\n'
             '            }\n' + upload),
    # Each level as the next smaller one, or the level before it.
    (upload, upload.replace('level - picmip', 'startLevel - level')),
    (upload, upload.replace(', newAddr);', ', oldAddr ? oldAddr : newAddr);')),
]
wavelet_mutants = [
    # RGB texels without their opaque alpha.
    ('                    dst[alpha0] = 0xff;\n', '                    dst[alpha0] = 0x00;\n'),
]

with tempfile.TemporaryDirectory(prefix='cod2-wavelet-') as directory:
    variants = [(functions, wavelet)] + [(functions.replace(old, new), wavelet) for old, new in mutants]
    for old, new in wavelet_mutants:
        assert wavelet.count(old) == 1, old
        variants.append((functions, wavelet.replace(old, new)))
    for index, (loader_source, wavelet_source) in enumerate(variants):
        path = Path(directory) / f'test{index}.c'
        path.write_text(support + wavelet_source + '\n' + mip_count + loader_source + checks)
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wall', '-Werror' if not index else '-w', '-Wno-unused-function', '-Wno-unused-variable',
                        '-Wno-unused-but-set-variable', str(path), '-o', str(path.with_suffix(''))], check=True)
        result = subprocess.run([str(path.with_suffix(''))], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-1500:])
print(f'PASS: wavelet RGB, RGBA and luminance images upload each mip level with the bytes and surface format of the '
      f'bitmap of the same texels, B, G, R, A, with opaque alpha for RGB; {len(variants) - 1} mutants fail, the Mac '
      f'build\'s byte reversal among them')
