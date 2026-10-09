#!/usr/bin/env python3
"""Aim/lean controllers must preserve a moving player's sampled skeleton."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/xanim/dobj.c').read_text()

def function(name):
    start = source.index(name + '(')
    while source.find(';', start) < source.find('{', start):
        start = source.index(name + '(', start + len(name))
    start = source.rfind('\n', 0, start) + 1
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'

support = r'''
#include <assert.h>
#include <math.h>
#include <string.h>
typedef float vec_t;
typedef int qboolean;
typedef struct { float quat[4], trans[3], transWeight; } DObjAnimMat;
typedef struct { int animPartBits[4],skelPartBits[4],controlPartBits[4]; DObjAnimMat *mat; } DSkel;
typedef struct { DSkel *skel; } DObj;
static DObjAnimMat sampled[8];
static int samples;
static int DObjFindBoneIndexForTag(const DObj *obj,unsigned int tag) { return tag < 8 ? tag : -1; }
static void DObjCalcAnim(const DObj *obj,int *bits) {
    samples++;
    for(int b=0;b<8;b++) if((bits[0] & (1<<b)) && !(obj->skel->animPartBits[0] & (1<<b))) {
        obj->skel->mat[b]=sampled[b]; obj->skel->animPartBits[0]|=1<<b;
    }
}
static void closef(float a,float b) { assert(fabsf(a-b)<.00001f); }
'''
body = ''.join(function(name) for name in [
    'DObjAnglesToQuatLocal', 'DObjSetLocalTagInternal', 'DObjSetLocalTag', 'DObjSetControlTagAngles'])
checks = r'''
int main(void) {
    DObjAnimMat mats[8]={0}; DSkel skel={.mat=mats}; DObj obj={&skel};
    int bits[4]={255,0,0,0}; float zero[3]={0}, yaw[3]={0,90,0};
    for(int frame=0;frame<120;frame++) {
        memset(&skel,0,sizeof(skel)); skel.mat=mats;
        for(int b=0;b<8;b++) {
            float angle=(frame+b)*.03f;
            sampled[b]=(DObjAnimMat){.quat={sinf(angle),0,0,cosf(angle)},
                .trans={b+1,frame*.1f,8+b},.transWeight=1};
        }
        /* Six player controls may be neutral, but animation and bone lengths are not. */
        for(int b=0;b<6;b++) {
            assert(DObjSetControlTagAngles(&obj,bits,b,zero));
            assert(!memcmp(&mats[b],&sampled[b],sizeof(mats[b])));
            assert(!DObjSetControlTagAngles(&obj,bits,b,yaw));
            assert(!memcmp(&mats[b],&sampled[b],sizeof(mats[b])));
        }
    }
    assert(samples==120);
    memset(&skel,0,sizeof(skel)); skel.mat=mats;
    float pitch[3]={90,0,0}; DObjAnglesToQuatLocal(pitch,sampled[0].quat);
    assert(DObjSetControlTagAngles(&obj,bits,0,yaw));
    closef(mats[0].quat[0],-.5f); closef(mats[0].quat[1],.5f);
    closef(mats[0].quat[2],.5f); closef(mats[0].quat[3],.5f);
    for(int a=0;a<3;a++) assert(mats[0].trans[a]==sampled[0].trans[a]);
    DObjAnimMat saved[8]; memcpy(saved,mats,sizeof(mats));
    int subset[4]={0};
    assert(!DObjSetControlTagAngles(&obj,subset,1,yaw));
    assert(!DObjSetControlTagAngles(&obj,bits,8,yaw));
    skel.skelPartBits[0]|=2;
    assert(!DObjSetControlTagAngles(&obj,bits,1,yaw));
    assert(!memcmp(saved,mats,sizeof(mats)));
    /* Root placement remains an absolute override rather than an additive control. */
    float position[3]={100,200,300};
    assert(DObjSetLocalTag(&obj,bits,7,position,zero));
    assert(mats[7].quat[3]==1 && mats[7].quat[0]==0);
    for(int a=0;a<3;a++) assert(mats[7].trans[a]==position[a]);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-animation-control-') as directory:
    path = Path(directory)
    overwrite = body.replace('skel->controlPartBits[boneIndexHigh] |= boneIndexLow;',
        'memcpy(mat->quat,control,sizeof(control)); skel->controlPartBits[boneIndexHigh] |= boneIndexLow;')
    collapse = body.replace('skel->controlPartBits[boneIndexHigh] |= boneIndexLow;',
        'memset(mat->trans,0,sizeof(mat->trans)); skel->controlPartBits[boneIndexHigh] |= boneIndexLow;')
    for index, variant in enumerate((body, overwrite, collapse)):
        (path / 'test.c').write_text(support + variant + checks)
        subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',
                        str(path/'test.c'),'-o',str(path/'test'),'-lm'],check=True)
        result = subprocess.run([str(path/'test')],capture_output=True,text=True)
        assert (result.returncode == 0) == (index == 0),result.stderr
print('PASS: 120 moving poses preserve six controlled bones, aim composition, frame cache and absolute root placement; pose/translation mutants fail')
