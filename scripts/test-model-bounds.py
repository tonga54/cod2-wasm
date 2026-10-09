#!/usr/bin/env python3
"""Exercise the renderer's actual skeletal bounds function under ASan/UBSan."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/gfx_d3d/r_model.c').read_text()
start = source.index('void R_UpdateXModelBounds(GfxSceneEntity *sceneEnt, GfxEntity *ent)\n{')
body = source[start:source.index('\nstatic int R_PreSkinXSurface', start)]
support = r'''
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
typedef unsigned char byte;
typedef struct { short modelIndex, subMatIndex; } DSurface;
typedef struct { float bounds[2][3], offset[3], radiusSquared; } XBoneInfo;
typedef struct { float quat[4], trans[3], transWeight; } DObjAnimMat;
typedef struct { int reType; float origin[3], axis[3][3]; } GfxEntity;
typedef struct { int cullState; union { void *data; } u; void *cent; float curMins[3], curMaxs[3]; } GfxSceneEntity;
typedef struct { void *modelDObj; } r_globals_t;
static r_globals_t rg; static void *imp_rg = &rg;
static struct { struct { int integer; } current; } dev, *developer = &dev;
static XBoneInfo bones[128];
static DObjAnimMat mats[128];
static int boneCount = 128, surfaceCount = 64;
static int InterlockedCompareExchange(int *p, int exchange, int compare) { int old=*p; if(old==compare)*p=exchange; return old; }
static void DObjSetModel(void *o, void *m) {}
static int DObjBad(void *o) { return 0; }
static void R_XModelDebugBoxes_impl(const byte *s, const byte *e, void *o) {}
static void R_XModelDebugAxes_impl(const byte *s, const byte *e, void *o) {}
static int R_GetSurfaceData_impl(const byte *e, void *o, void *surfs, int *bits, char *lods) {
    DSurface *surfaces = surfs;
    for(int i=0;i<surfaceCount;i++) surfaces[i]=(DSurface){i/8,i%8};
    for(int i=0;i<4;i++) bits[i]=-1;
    return surfaceCount;
}
static void CG_DObjCalcPose(void *c, void *o, int *bits) {}
static void *DObjGetRotTransArray(void *o) { return mats; }
static void ClearBounds(float *lo, float *hi) { for(int j=0;j<3;j++){lo[j]=1e30f;hi[j]=-1e30f;} }
static void DObjGetBoneInfo(void *o, void **b) { for(int i=0;i<boneCount;i++)b[i]=&bones[i]; }
static int DObjNumBones(void *o) { return boneCount; }
static void GetRotatedBounds(float *b, float *org, float *axis, float *out) {
    /* Entity axes are identity here; skeletal rotations are tested below. */
    for(int j=0;j<6;j++) out[j]=b[j]+org[j%3];
}
'''
checks = r'''
int main(void) {
    for(int trial=0;trial<40;trial++) {
        GfxEntity ent={.origin={1200,1300,-5}};
        GfxSceneEntity scene={0};
        float expectedLo[3], expectedHi[3]; ClearBounds(expectedLo,expectedHi);
        boneCount=trial%2 ? 128 : 61;
        for(int i=0;i<boneCount;i++) {
            float q[4]={sinf(i+trial),cosf(i*.7f),sinf(trial*.3f),.6f};
            float norm=0; for(int j=0;j<4;j++)norm+=q[j]*q[j]; norm=sqrtf(norm);
            for(int j=0;j<4;j++)mats[i].quat[j]=q[j]/norm;
            mats[i].transWeight=2;
            for(int j=0;j<3;j++) {
                bones[i].bounds[0][j]=-1.0f-(i%7)-j*3.0f;
                bones[i].bounds[1][j]=2.0f+(i%5)+j*7.0f;
                mats[i].trans[j]=(i%11)*2-j*8;
            }
            /* Rotate all eight AABB corners using q*v*q^-1 independently. */
            for(int corner=0;corner<8;corner++) {
                float v[3],t[3],cross[3];
                for(int j=0;j<3;j++)v[j]=bones[i].bounds[(corner>>j)&1][j];
                const float *u=mats[i].quat;
                for(int j=0;j<3;j++)t[j]=2*(u[(j+1)%3]*v[(j+2)%3]-u[(j+2)%3]*v[(j+1)%3]);
                for(int j=0;j<3;j++)cross[j]=u[(j+1)%3]*t[(j+2)%3]-u[(j+2)%3]*t[(j+1)%3];
                for(int j=0;j<3;j++) {
                    float p=v[j]+u[3]*t[j]+cross[j]+mats[i].trans[j]+ent.origin[j];
                    if(p<expectedLo[j])expectedLo[j]=p;
                    if(p>expectedHi[j])expectedHi[j]=p;
                }
            }
        }
        R_UpdateXModelBounds(&scene,&ent);
        assert(scene.cullState==2);
        for(int j=0;j<3;j++) {
            assert(isfinite(scene.curMins[j]) && isfinite(scene.curMaxs[j]));
            assert(fabsf(scene.curMins[j]-expectedLo[j])<.001f);
            assert(fabsf(scene.curMaxs[j]-expectedHi[j])<.001f);
        }
    }
    return 0;
}
'''
mutations = {
    'current': body,
    'undersized_surface_array': body.replace('DSurface surfaces[64]', 'short surfaces[67]'),
    'wrong_bone_pointer': body.replace('const XBoneInfo *bi = boneInfo[i];', 'const XBoneInfo *bi = (const XBoneInfo *)&boneInfo[i * 4];'),
    'wrong_axis': body.replace('[sel][biOfs]', '[sel][0]').replace('[1 - sel][biOfs]', '[1 - sel][0]'),
}
with tempfile.TemporaryDirectory(prefix='cod2-model-bounds-') as directory:
    path=Path(directory)
    for name, actual in mutations.items():
        (path/'test.c').write_text(support+actual+checks)
        subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(path/'test.c'),'-lm','-o',str(path/'test')],check=True)
        result=subprocess.run([str(path/'test')],capture_output=True)
        assert (result.returncode==0)==(name=='current'), (name,result.stderr.decode())
print('PASS: 40 skeletal bounds cases, 61/128 bones, 64 surfaces; all three regressions detected')
