#!/usr/bin/env python3
"""Check real linked-player transforms and catch the old three-row stack read."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent


def extract(source, name):
    match = re.search(r'^[^;\n]*\b' + name + r'\([^;\n]*\)\n\{', source, re.M)
    assert match, name
    end = match.end()
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]


math_source = (root / 'src/PC/universal/com_math.c').read_text()
names = ('MatrixTranspose_core', 'MatrixTransformVector_core', 'MatrixMultiply',
         'MatrixMultiply43', 'MatrixTranspose', 'MatrixInverseOrthogonal43',
         'MatrixTransformVector43')
math_code = '\n'.join(extract(math_source, name) for name in names)
for name in names[2:]:
    math_code = re.sub(r'\b' + name + r'\b', 'Real' + name, math_code)
source = extract((root / 'src/PC/game_mp/g_utils_mp.c').read_text(), 'G_CalcTagAxis')
support = r'''
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
typedef float vec_t;typedef float vec3_t[3];typedef int qboolean;
typedef struct {void *parent;float axis[4][3];}tagInfo_t;
typedef struct {tagInfo_t *tagInfo;struct {float currentOrigin[3],currentAngles[3];}r;}gentity_t;
#define _ENT(e) (e)
#define COD2_TagInfoFromHandle(h) (h)
#define COD2_TagInfoHandle(h) (h)
#define VectorCopy(a,b) memcpy(b,a,sizeof(vec3_t))
static float parent[4][3];
static void AnglesToAxis(const float *angles,float *out){
 float yaw=angles[1]*.017453292519943295f,c=cosf(yaw),s=sinf(yaw);
 float rotation[3][3]={{c,s,0},{-s,c,0},{0,0,1}};memcpy(out,rotation,sizeof(rotation));
}
static void G_CalcTagParentAxis(gentity_t *ent,vec3_t *out){(void)ent;memcpy(out,parent,sizeof(parent));}
'''
wrappers = r'''
#define MatrixTranspose(a,b) RealMatrixTranspose((const float(*)[3])(a),(float(*)[3])(b))
#define MatrixMultiply(a,b,c) RealMatrixMultiply((const float(*)[3])(a),(const float(*)[3])(b),(float(*)[3])(c))
#define MatrixMultiply43(a,b,c) RealMatrixMultiply43((const float(*)[3])(a),(const float(*)[3])(b),(float(*)[3])(c))
#define MatrixInverseOrthogonal43(a,b) RealMatrixInverseOrthogonal43((const float(*)[3])(a),(float(*)[3])(b))
'''
checks = r'''
int main(void){
 tagInfo_t info={0};gentity_t ent={0};ent.tagInfo=&info;info.parent=&ent;int count=0;
 for(int rotation=0;rotation<24;rotation++)for(int at=0;at<32;at++){
  float angles[3]={0,rotation*15.0f,0},position[3];
  AnglesToAxis(angles,(float*)parent);parent[3][0]=1000+at*17;parent[3][1]=-500+at*7;parent[3][2]=91;
  ent.r.currentOrigin[0]=parent[3][0]+64;ent.r.currentOrigin[1]=parent[3][1]-37;ent.r.currentOrigin[2]=113+at;
  ent.r.currentAngles[1]=at*11;
  assert(G_CalcTagAxis(&ent,0));
  RealMatrixTransformVector43(info.axis[3],parent,position);
  for(int i=0;i<3;i++)assert(fabsf(position[i]-ent.r.currentOrigin[i])<.01f);
  float saved[3];memcpy(saved,info.axis[3],sizeof(saved));
  assert(G_CalcTagAxis(&ent,1));
  for(int i=0;i<3;i++)assert(saved[i]==info.axis[3][i]);
  count++;
 }
 ent.tagInfo=NULL;assert(!G_CalcTagAxis(&ent,0));
 printf("PASS: %d linked-player poses preserve world position and angle-only translation under ASan/UBSan\n",count);
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-link-origin-') as directory:
    path = Path(directory)
    for name in ('fixed', 'three-row-read'):
        current = source if name == 'fixed' else source.replace('vec3_t axis[4];', 'vec3_t axis[3];').replace(
            'VectorCopy((_ENT(ent)->r.currentOrigin), axis[3]);', '')
        code = path / (name + '.c')
        binary = path / name
        code.write_text(support + math_code + wrappers + current + checks)
        subprocess.run(['cc', '-std=c99', '-g', '-O1', '-fsanitize=address,undefined',
                        '-fno-omit-frame-pointer', str(code), '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
        if name == 'fixed':
            assert result.returncode == 0, result.stderr
            print(result.stdout.strip())
        else:
            assert result.returncode != 0 and 'AddressSanitizer' in result.stderr, result.stderr
            print('PASS: former three-row transform causes an ASan stack-buffer-overflow')
