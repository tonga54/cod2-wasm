#!/usr/bin/env python3
"""Exercise the real D3D fog state bridge and world/UI transitions."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/Mac/DirectX_9/CDirect3DDevice.c').read_text()
fog_source = (root / 'src/PC/gfx_d3d/rb_fog.c').read_text()
shade_source = (root / 'src/PC/gfx_d3d/rb_shade.c').read_text()

def function(name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'

functions = ''.join(function(name) for name in (
    'CDirect3DDevice_ApplyFog', 'CDirect3DDevice_SetRenderState',
    'CDirect3DDevice_GetRenderState'))
fields = sorted(set(re.findall(r'dev->(\w+)', functions)))
states = sorted(set(re.findall(r'case (D3DRS_\w+):', functions)))
support = '''
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
typedef unsigned int DWORD;
typedef int HRESULT, D3DRENDERSTATETYPE;
#define COD2_FORCE_ALIGN_ARG_POINTER
''' + '\n'.join(f'#define {name} {28 if name == "D3DRS_FOGENABLE" else 1000 + i}'
                 for i, name in enumerate(states)) + '\ntypedef struct {\n    void **vtable;\n' + '\n'.join(
    f'    {"float" if field == "alphaRef" else "DWORD"} {field};' for field in fields
) + '''
} DeviceImpl;
typedef DeviceImpl CDirect3DDevice;
static DWORD g_alphaTestEnable, g_textureFactor;
static float g_depthBias, g_slopeScaleDepthBias, params[0x10000], color[4];
static int enabled, mode;
static void glEnable(unsigned cap) { assert(cap == 0x0B60); enabled = 1; }
static void glDisable(unsigned cap) { assert(cap == 0x0B60); enabled = 0; }
void glFogf(unsigned pname, float value) {
    params[pname] = value; if (pname == 0x0B65) mode = value;
}
static void glFogfv(unsigned pname, const float *value) {
    assert(pname == 0x0B66); memcpy(color, value, sizeof(color));
}
static void CDirect3DDevice_ApplyDepthBias(void) {}
static unsigned CDirect3DDevice_MapCompareFunc(unsigned value) { return value; }
'''
fog_update_start = fog_source.index('void RB_UpdateFogColor(')
fog_update_end = fog_source.index('\nvoid RB_SetIteratorFog(', fog_update_start)
functions += '''
#define D3DVTCC
enum FogColorSrcEnum { FOG_COLOR_SRC_UNSPECIFIED, FOG_COLOR_SRC_STANDARD, FOG_COLOR_SRC_BLACK };
typedef enum FogColorSrcEnum FogColorSrcEnum;
static struct { int fogIndex; } rg = {1};
static struct { struct { unsigned packed; } fogColor; } backEnd;
static struct { int refStateBits[2]; struct { struct { unsigned packed; } color; } fog; } dxState;
static struct { void *device; } dx;
static int alwaysfails;
''' + fog_source[fog_update_start:fog_update_end]
selection = re.search(r'FogColorSrcEnum fogSrc = .*?;', shade_source, re.S).group()
functions += '''
typedef struct { int fogToBlack; } MaterialPassDx7;
static FogColorSrcEnum materialFog(MaterialPassDx7 *pass) {
''' + selection + '\nreturn fogSrc;\n}\n'
checks = '''
static DWORD bits(float value) { DWORD out; memcpy(&out, &value, 4); return out; }
int main(void) {
    DeviceImpl dev = {0}; DWORD value;
    CDirect3DDevice_SetRenderState(&dev, D3DRS_FOGENABLE, 1);
    CDirect3DDevice_SetRenderState(&dev, 0x8c, 1);
    for (int i = 1; i <= 3; ++i) {
        float density = i * .0001f;
        CDirect3DDevice_SetRenderState(&dev, 0x26, bits(density));
        assert(params[0x0B62] == density);
        CDirect3DDevice_GetRenderState(&dev, 0x26, &value);
        assert(value == bits(density));
        CDirect3DDevice_SetRenderState(&dev, 0x8c, i);
        CDirect3DDevice_ApplyFog(&dev, 0);
        assert(enabled && mode == (i == 3 ? 0x2601 : i == 2 ? 0x0801 : 0x0800));
        CDirect3DDevice_ApplyFog(&dev, 1); assert(!enabled);
        CDirect3DDevice_ApplyFog(&dev, 0); assert(enabled);
        CDirect3DDevice_GetRenderState(&dev, 0x8c, &value); assert(value == i);
    }
    CDirect3DDevice_SetRenderState(&dev, 0x24, bits(100.f));
    CDirect3DDevice_SetRenderState(&dev, 0x25, bits(4000.f));
    assert(params[0x0B63] == 100.f && params[0x0B64] == 4000.f);
    for (int state = 0x24; state <= 0x25; ++state) {
        CDirect3DDevice_GetRenderState(&dev, state, &value);
        assert(value == bits(state == 0x24 ? 100.f : 4000.f));
    }
    CDirect3DDevice_SetRenderState(&dev, 0x23, 2);
    CDirect3DDevice_ApplyFog(&dev, 0); assert(mode == 0x0801);
    CDirect3DDevice_SetRenderState(&dev, 0x23, 0);
    CDirect3DDevice_ApplyFog(&dev, 0); assert(mode == 0x2601);
    CDirect3DDevice_SetRenderState(&dev, 0x22, 0xff8c998cu);
    assert(fabsf(color[0] - 140.f / 255.f) < 1e-6f);
    assert(fabsf(color[1] - 153.f / 255.f) < 1e-6f && color[3] == 1.f);
    CDirect3DDevice_SetRenderState(&dev, 0x22, 0xff000000u);
    assert(!color[0] && !color[1] && !color[2] && color[3] == 1.f);
    CDirect3DDevice_SetRenderState(&dev, D3DRS_FOGENABLE, 0);
    CDirect3DDevice_ApplyFog(&dev, 0); assert(!enabled);
    CDirect3DDevice_SetRenderState(&dev, D3DRS_FOGENABLE, 1);
    CDirect3DDevice_SetRenderState(&dev, 0x8c, 0);
    CDirect3DDevice_ApplyFog(&dev, 0); assert(!enabled);
    /* Run the actual material selector and cached backend color handoff. */
    void *vtable[64] = {0};
    vtable[0xe4/4] = CDirect3DDevice_SetRenderState;
    dev.vtable = vtable;
    dx.device = &dev;
    backEnd.fogColor.packed = 0xff8c998cu;
    MaterialPassDx7 opaque = {0}, additive = {1};
    assert(materialFog(&opaque) == FOG_COLOR_SRC_STANDARD);
    assert(materialFog(&additive) == FOG_COLOR_SRC_BLACK);
    RB_UpdateFogColor(materialFog(&opaque));
    assert(color[0] > .54f && color[1] > .59f && dxState.fog.color.packed == 0xff8c998cu);
    RB_UpdateFogColor(materialFog(&additive));
    assert(!color[0] && !color[1] && !color[2]);
    RB_UpdateFogColor(materialFog(&opaque));
    assert(color[0] > .54f && color[1] > .59f);
    rg.fogIndex = 0; RB_UpdateFogColor(FOG_COLOR_SRC_BLACK);
    assert(color[0] > .54f);
    puts("PASS: exponential/linear fog, exact map densities/colors, additive black, table precedence and world/UI/off transitions");
}
'''
assert 'CDirect3DDevice_ApplyFog(dev, is2D);' in source
with tempfile.TemporaryDirectory(prefix='cod2-web-fog-') as folder:
    path = Path(folder)
    (path / 'test.c').write_text(support + functions + checks)
    subprocess.run(['cc', '-O1', '-g', '-fsanitize=address,undefined',
                    str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
