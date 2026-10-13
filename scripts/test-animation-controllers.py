#!/usr/bin/env python3
"""Exercise model-space aim/lean through the actual skeletal hierarchy."""
from pathlib import Path
import subprocess, tempfile
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
        depth += (source[end] == '{') - (source[end] == '}'); end += 1
    return source[start:end] + '\n'
support = r'''
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef float vec_t; typedef int qboolean;
typedef struct { float quat[4], trans[3], transWeight; } DObjAnimMat;
typedef struct { int animPartBits[4],skelPartBits[4],controlPartBits[4]; DObjAnimMat mat[]; } DSkel;
typedef struct { unsigned char parentList[127]; } XBoneHierarchy;
typedef struct { int numRootBones,numBones; XBoneHierarchy *hierarchy; } XModelParts;
typedef struct { XModelParts *parts; } XModel;
typedef struct { DSkel *skel; int numBones,numModels,duplicateParts; XModel *models[2]; unsigned char modelParents[2]; } DObj;
static DObjAnimMat sampled[127];
static unsigned char duplicates[32];
static const char *SL_ConvertToString(int id) { return (const char *)duplicates; }
static int DObjFindBoneIndexForTag(const DObj *obj,unsigned int tag) { return tag < obj->numBones ? (int)tag : -1; }
static void DObjCalcAnim(const DObj *obj,int *bits) {
    for(int b=0;b<obj->numBones;b++) if((bits[b>>5] & (1u<<(b&31))) && !(obj->skel->animPartBits[b>>5] & (1u<<(b&31)))) {
        obj->skel->mat[b]=sampled[b]; obj->skel->animPartBits[b>>5]|=1u<<(b&31);
    }
}
static void closef(float a,float b) { assert(fabsf(a-b)<.002f); }
'''
body = ''.join(function(name) for name in [
    'DObjGetAllocSkelSize','DObjAnglesToQuatLocal','DObjSetLocalTagInternal',
    'DObjSetLocalTag','DObjSetControlTagAngles','DObjPartBitTestLocal',
    'DObjPartBitSetLocal','DObjCopyAnimMatLocal','DObjFinalizeQuatWeightLocal',
    'DObjComposeAnimMatLocal','DObjApplyControlLocal','DObjApplyDuplicatePartLocal',
    'DObjCalcSkel'])
checks = r'''
/* Independent Rodrigues vector rotations: pitch points forward down and roll
 * moves the top of the torso sideways, regardless of the parent joint axes. */
static void rotateAxis(float *v,int axis,float degrees) {
    float a=degrees*.01745329252f,c=cosf(a),s=sinf(a);
    int j=(axis+1)%3,k=(axis+2)%3;float x=v[j],y=v[k];v[j]=c*x-s*y;v[k]=s*x+c*y;
}
static void rotateAngles(float *v,const float *a) {rotateAxis(v,0,a[2]);rotateAxis(v,1,a[0]);rotateAxis(v,2,a[1]);}
static void rotateQuat(const DObjAnimMat *m,const float *v,float *out) {
    const float *q=m->quat;float t[3],cross[3],norm=0;
    for(int j=0;j<4;j++)norm+=q[j]*q[j];
    for(int j=0;j<3;j++)t[j]=2*(q[(j+1)%3]*v[(j+2)%3]-q[(j+2)%3]*v[(j+1)%3]);
    for(int j=0;j<3;j++)cross[j]=q[(j+1)%3]*t[(j+2)%3]-q[(j+2)%3]*t[(j+1)%3];
    for(int j=0;j<3;j++)out[j]=v[j]+(q[3]*t[j]+cross[j])/norm;
}
int main(void) {
    XBoneHierarchy hierarchy={.parentList={1,1,1,1,1,1,1}};
    XModelParts parts={1,8,&hierarchy};XModel model={&parts};
    DObj obj={.numBones=8,.numModels=1,.models={&model},.modelParents={255,255}};
    DSkel *skel=calloc(1,DObjGetAllocSkelSize(&obj));obj.skel=skel;
    int bits[4]={255,0,0,0};float zero[3]={0},yaw[3]={0,90,0};
    for(int frame=0;frame<120;frame++) {
        memset(skel,0,DObjGetAllocSkelSize(&obj));
        for(int b=0;b<8;b++) {
            float angles[3]={frame*.3f,b*30.f,(frame+b)*2.f};
            DObjAnglesToQuatLocal(angles,sampled[b].quat);
            sampled[b].trans[0]=b+1;sampled[b].trans[1]=frame*.1f;sampled[b].trans[2]=8+b;
            sampled[b].transWeight=1;
        }
        for(int b=0;b<6;b++) {
            assert(DObjSetControlTagAngles(&obj,bits,b,zero));
            assert(!memcmp(&skel->mat[b],&sampled[b],sizeof(sampled[b])));
            assert(!DObjSetControlTagAngles(&obj,bits,b,yaw));
        }
        DObjCalcSkel(&obj,bits);DObjAnimMat saved[8];memcpy(saved,skel->mat,sizeof(saved));
        DObjCalcSkel(&obj,bits);assert(!memcmp(saved,skel->mat,sizeof(saved)));
        memset(skel,0,DObjGetAllocSkelSize(&obj));DObjCalcSkel(&obj,bits);
        assert(!memcmp(saved,skel->mat,sizeof(saved))); /* Neutral controls preserve walking. */
    }
    int cases=0;
    for(int attached=0;attached<2;attached++)for(int parent=0;parent<24;parent++)for(int axis=0;axis<3;axis++)for(int sign=-1;sign<=1;sign+=2) {
        obj.numModels=attached?2:1;parts.numBones=attached?4:8;obj.models[1]=&model;obj.modelParents[1]=2;
        int controlled=attached?4:1;
        memset(sampled,0,sizeof(sampled));
        for(int b=0;b<8;b++){sampled[b].quat[3]=1;sampled[b].trans[2]=b?4:35;}
        float p[3]={parent*13.f,parent*31.f,parent*19.f};
        float a[3]={17,-29,51};DObjAnglesToQuatLocal(p,sampled[0].quat);DObjAnglesToQuatLocal(a,sampled[1].quat);
        memset(skel,0,DObjGetAllocSkelSize(&obj));DObjCalcSkel(&obj,bits);
        DObjAnimMat neutral[8];memcpy(neutral,skel->mat,sizeof(neutral));
        memset(skel,0,DObjGetAllocSkelSize(&obj));float control[3]={0};control[axis]=sign*35;
        assert(DObjSetControlTagAngles(&obj,bits,controlled,control));DObjCalcSkel(&obj,bits);
        for(int basis=0;basis<3;basis++){
            float unit[3]={0},expected[3],actual[3];unit[basis]=1;
            rotateQuat(&neutral[controlled],unit,expected);rotateAngles(expected,control);
            rotateQuat(&skel->mat[controlled],unit,actual);
            for(int j=0;j<3;j++)closef(actual[j],expected[j]);
        }
        for(int b=controlled;b<8;b++) {
            float delta[3];for(int j=0;j<3;j++)delta[j]=neutral[b].trans[j]-neutral[controlled].trans[j];
            rotateAngles(delta,control);
            for(int j=0;j<3;j++)closef(skel->mat[b].trans[j],neutral[controlled].trans[j]+delta[j]);
        }
        assert(!memcmp(&neutral[0],&skel->mat[0],sizeof(neutral[0])));cases++;
    }
    obj.numModels=1;parts.numBones=8;
    memset(skel,0,DObjGetAllocSkelSize(&obj));
    float placement[3]={100,200,300};
    assert(DObjSetLocalTag(&obj,bits,0,placement,zero));DObjCalcSkel(&obj,bits);
    for(int i=0;i<3;i++)assert(skel->mat[0].trans[i]==placement[i]);
    /* Highest supported bone: queued controllers must fit their allocation. */
    free(skel);obj.numBones=127;parts.numBones=127;memset(hierarchy.parentList,1,127);
    skel=calloc(1,DObjGetAllocSkelSize(&obj));obj.skel=skel;
    int all[4]={-1,-1,-1,0x7fffffff};
    for(int b=0;b<127;b++)sampled[b]=(DObjAnimMat){.quat={0,0,0,1},.trans={0,0,1}};
    assert(DObjSetControlTagAngles(&obj,all,126,yaw));DObjCalcSkel(&obj,all);
    assert(!DObjSetControlTagAngles(&obj,all,127,yaw));
    free(skel);
    printf("PASS: 120 animated poses, %d pitch/yaw/lean cases with rotated parents, attached models, pivots, descendants, cached frames and maximum-size skeleton\n",cases);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-animation-control-') as directory:
    p=Path(directory)
    # Reproduce the former parent-local application, not merely a missing call.
    old=body.replace('DObjApplyControlLocal(obj, globalBoneIndex);','')
    marker='        skel->controlPartBits[boneIndexHigh] |= boneIndexLow;'
    old=old.replace(marker,marker+'\n        DObjApplyControlLocal(obj, boneIndex);')
    old='static void DObjApplyControlLocal(const DObj *obj,int boneIndex);\n'+old
    for index,variant in enumerate((body,old)):
        (p/'test.c').write_text(support+variant+checks)
        subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test'),'-lm'],check=True)
        result=subprocess.run([str(p/'test')],capture_output=True,text=True)
        assert (result.returncode==0)==(index==0),result.stderr
        if index==0:print(result.stdout.strip())
print('PASS: former parent-local rotation is rejected')
