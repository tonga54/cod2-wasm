#!/usr/bin/env python3
"""A partial animation weight must not scale the skeleton's bone lengths."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root/'src/PC/xanim/xanim.c').read_text()
def function(name):
    start=source.index(name+'(')
    # Skip the declarations near the top of the file.
    while source.find(';',start)<source.find('{',start):
        start=source.index(name+'(',start+len(name))
    start=source.rfind('\n',0,start)+1
    brace=source.index('{',start);depth=1;end=brace+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]+'\n'

support=r'''
#include <assert.h>
#include <math.h>
#include <string.h>
typedef struct { float quat[4],trans[3],transWeight; } DObjAnimMat;
typedef struct { int animPartBits[4]; DObjAnimMat *mat; } DSkel;
typedef struct { int numRootBones,numBones; short *quats; float *trans; } XModelParts;
typedef struct { XModelParts *parts; } XModel;
typedef struct { int numBones,numModels; DSkel *skel; void *tree; XModel **models; } DObj;
typedef struct { int unused; } XAnimPart;
typedef struct { DObjAnimMat rotTransArray[512]; int animPartBits[4],ignorePartBits[4]; } XAnimCalcAnimInfo;
static float weight;
static void XAnimCalc(const DObj *obj,unsigned int index,float scale,XAnimPart (*raw)(),
    int clear,int norm,XAnimCalcAnimInfo *info,int offset) {
    DObjAnimMat *mat=(DObjAnimMat *)raw;
    for(int b=0;b<3;b++) {
        if(info->ignorePartBits[0]&(1<<b)) continue;
        memset(&mat[b],0,sizeof(mat[b]));
        if(b==2) continue; /* Missing animation uses the model's bind pose. */
        info->animPartBits[0]|=1<<b;
        mat[b].quat[3]=weight;
        mat[b].trans[0]=(10+b)*weight;
        mat[b].trans[1]=(20+b)*weight;
        mat[b].trans[2]=(30+b)*weight;
        mat[b].transWeight=weight;
    }
}
'''
body=''.join(function(name) for name in [
    'XAnimCalcBitTestLocal','XAnimCalcInvSqrtLocal','XAnimCalcNormalizeRotTransLocal',
    'XAnimCalcNormalizeBonesLocal','DObjCalcAnim'])
checks=r'''
int main(void) {
    short quats[8]={0,0,0,32767,0,0,0,32767};float trans[6]={1,2,3,4,5,6};
    XModelParts parts={1,3,quats,trans};XModel model={&parts};XModel *models[]={&model};
    float weights[]={.01f,.1f,.25f,.5f,1.0f,1.01f,2.0f};
    for(int w=0;w<7;w++) for(int ignored=0;ignored<2;ignored++) {
        DObjAnimMat mats[3]={0};DSkel skel={{0},mats};
        DObj obj={3,1,&skel,&obj,models};int bits[4]={7,0,0,0};weight=weights[w];
        if(ignored) {skel.animPartBits[0]=1;mats[0].trans[2]=99;}
        DObjCalcAnim(&obj,bits);
        for(int b=ignored;b<2;b++) for(int a=0;a<3;a++)
            assert(fabsf(mats[b].trans[a]-((a+1)*10+b+(b ? trans[a] : 0)))<.0001f);
        if(ignored) assert(mats[0].trans[2]==99);
        for(int a=0;a<3;a++) assert(mats[2].trans[a]==trans[a+3]);
        assert(mats[2].quat[3]==1);
        DObjAnimMat saved[3];memcpy(saved,mats,sizeof(mats));
        DObjCalcAnim(&obj,bits);assert(!memcmp(saved,mats,sizeof(mats)));
    }
    /* Unequal source contributions form a weighted average, even above 1. */
    DObjAnimMat mat={.quat={0,0,0,2},.trans={1,8,14},.transWeight=2};
    XAnimCalcNormalizeRotTransLocal(&mat,1);
    assert(mat.trans[0]==.5f && mat.trans[1]==4 && mat.trans[2]==7);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-animation-weight-') as directory:
    path=Path(directory)
    variants = [body,
        body.replace('XAnimCalcNormalizeBonesLocal(obj, mat, 1.0f, &info);',''),
        body.replace('mat->trans[0] += trans[0];','')
            .replace('mat->trans[1] += trans[1];','')
            .replace('mat->trans[2] += trans[2];','')]
    for index,variant in enumerate(variants):
        (path/'test.c').write_text(support+variant+checks)
        subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',
                        str(path/'test.c'),'-o',str(path/'test'),'-lm'],check=True)
        result=subprocess.run([str(path/'test')],capture_output=True,text=True)
        assert (result.returncode==0)==(index==0),result.stderr
print('PASS: 14 partial-weight/ignored-bone cases, bind offsets and cached skeleton preserved; both collapse mutants fail')
