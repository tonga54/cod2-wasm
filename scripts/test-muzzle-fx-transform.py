#!/usr/bin/env python3
"""Check production FX tag transforms against independently rotated points."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent

def function(path, name):
    source = (root / path).read_text()
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'

mathfile = 'src/PC/universal/com_math.c'
mathcode = ''.join(function(mathfile, name) for name in ('AngleVectors', 'AnglesToAxis', 'MatrixMultiply', 'MatrixTransformVector43', 'ConvertQuatToMat'))
provider = function('src/PC/cgame_mp/cg_main_mp.c', 'CG_GetDObjOrientation')
bone = function('src/PC/EffectsCore/FxUtil.c', 'FX_GetBoneOrientation')
cache = function('src/PC/EffectsCore/FxPrimitives.c', 'FxBoltFrame_GetOrientation')
support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define __attribute_regparm__(x)
#define __attribute_sseregparm__
typedef unsigned char byte; typedef int Bool; typedef float vec_t; typedef float vec3_t[3];
typedef struct {vec3_t origin,axis[3];} orientation_t;
typedef struct {int dobjHandle,boneIndex;} FxBoltInfo;
typedef struct {float quat[4],trans[3],transWeight;} DObjAnimMat;
typedef struct {vec3_t lerpOrigin,lerpAngles;} centity_t;
static centity_t cg_entities[1024];
static struct {vec3_t viewModelOrigin,viewModelAxis[3];} state,*cg=&state;
static DObjAnimMat bones[4];
static int hasObject=1,hasMatrices=1,calcCalls;
static void *Com_GetClientDObj(int h,int local) {return hasObject?bones:NULL;}
static int DObjNumBones(void *o) {return 4;}
static void CG_DObjCalcBoneGeneric(int h,int l,int b) {calcCalls++;}
static void *DObjGetRotTransArray(void *o) {return hasMatrices?bones:NULL;}
static void AxisCopy(void *a,void *b) {memcpy(b,a,9*sizeof(float));}
typedef struct {struct {float value;}current;} dvar_t;
static dvar_t debug,*debugp=&debug;
static void *imp_fx_debugBolt=&debugp,*imp_colorRed,*imp_colorGreen,*imp_colorBlue;
static void CL_AddDebugLine(const float*a,const float*b,const float*c,int d,int e,int f) {}
typedef struct {int skelTimeStamp;} clientActive_t;
static clientActive_t client,*cl=&client;static void *imp_cl=&cl;
typedef struct {int cachedServerTime;FxBoltInfo mBolt;orientation_t orientation;} FxBoltFrame;
'''
checks = r'''
static void near3(const float *a,const float *b) {for(int i=0;i<3;i++)assert(fabsf(a[i]-b[i])<.002f);}
/* Rodrigues rotation supplies an independent reference for bone basis rows. */
static void rotate(const float *v,const float *unit,float angle,float *out) {
 float c=cosf(angle),s=sinf(angle),dot=v[0]*unit[0]+v[1]*unit[1]+v[2]*unit[2];
 float cross[3]={unit[1]*v[2]-unit[2]*v[1],unit[2]*v[0]-unit[0]*v[2],unit[0]*v[1]-unit[1]*v[0]};
 for(int i=0;i<3;i++)out[i]=v[i]*c+cross[i]*s+unit[i]*dot*(1-c);
}
static void worldDir(const float *v,float axis[3][3],float *out) {
 for(int i=0;i<3;i++)out[i]=v[0]*axis[0][i]+v[1]*axis[1][i]+v[2]*axis[2][i];
}
int main(void) {
 int tested=0;
 for(int handle=0;handle<2;handle++)for(int pitch=-60;pitch<=60;pitch+=15)
 for(int yaw=-180;yaw<=180;yaw+=20)for(int roll=-30;roll<=30;roll+=30) {
  vec3_t angles={pitch,yaw,roll},parentAxis[3];AnglesToAxis(angles,parentAxis);
  vec3_t parentOrigin={800+pitch,333+yaw,231+roll};
  memcpy(cg->viewModelOrigin,parentOrigin,sizeof(parentOrigin));AxisCopy(parentAxis,cg->viewModelAxis);
  memcpy(cg_entities[63].lerpOrigin,parentOrigin,sizeof(parentOrigin));memcpy(cg_entities[63].lerpAngles,angles,sizeof(angles));
  FxBoltInfo bolt={handle?0x47f:63,2};
  const float unit[3]={.26726124f,.53452248f,.80178373f};float angle=(yaw+roll)*.01745329252f;
  float qscale=1.7f;for(int i=0;i<3;i++)bones[2].quat[i]=unit[i]*sinf(angle/2)*qscale;
  bones[2].quat[3]=cosf(angle/2)*qscale;bones[2].transWeight=2/(qscale*qscale);
  bones[2].trans[0]=28;bones[2].trans[1]=-3;bones[2].trans[2]=-5;
  orientation_t got;assert(FX_GetBoneOrientation(&bolt,&got));
  vec3_t expected;worldDir(bones[2].trans,parentAxis,expected);
  for(int i=0;i<3;i++)expected[i]+=parentOrigin[i];near3(got.origin,expected);
  for(int row=0;row<3;row++) {
   vec3_t unitRow={0,0,0},local;unitRow[row]=1;rotate(unitRow,unit,angle,local);worldDir(local,parentAxis,expected);
   near3(got.axis[row],expected);
   assert(fabsf(got.axis[row][0]*got.axis[row][0]+got.axis[row][1]*got.axis[row][1]+got.axis[row][2]*got.axis[row][2]-1)<.0001f);
  }
  bolt.boneIndex=-1;assert(FX_GetBoneOrientation(&bolt,&got));near3(got.origin,parentOrigin);
  for(int row=0;row<3;row++)near3(got.axis[row],parentAxis[row]);tested++;
 }
 FxBoltInfo bolt={1024,4};orientation_t out;assert(!FX_GetBoneOrientation(&bolt,&out));
 bolt.boneIndex=0;hasObject=0;assert(!FX_GetBoneOrientation(&bolt,&out));hasObject=1;
 hasMatrices=0;assert(!FX_GetBoneOrientation(&bolt,&out));hasMatrices=1;
 bolt.dobjHandle=-1;assert(!FX_GetBoneOrientation(&bolt,&out));bolt.dobjHandle=0x480;assert(!FX_GetBoneOrientation(&bolt,&out));
 FxBoltFrame frame={.mBolt={1024,2}};calcCalls=0;
 for(int time=1;time<=100;time++) {
  client.skelTimeStamp=time;cg->viewModelOrigin[0]=time*3;
  const orientation_t *first=FxBoltFrame_GetOrientation(&frame);assert(first==&frame.orientation);
  orientation_t copy=*first;
  for(int i=0;i<10;i++) {assert(FxBoltFrame_GetOrientation(&frame)==first);assert(!memcmp(first,&copy,sizeof(copy)));}
  assert(calcCalls==time);
 }
 hasObject=0;client.skelTimeStamp++;assert(!FxBoltFrame_GetOrientation(&frame));assert(frame.mBolt.dobjHandle==-1);
 assert(!FxBoltFrame_GetOrientation(&frame));
 printf("PASS: %d world/viewmodel transforms, nonunit quaternions, missing bones and 1,100 cached FX orientations\n",tested);
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-muzzle-fx-') as directory:
    path = Path(directory)
    code = support + mathcode + provider + bone + cache + checks
    variants = [code,
                code.replace('MatrixMultiply(tagAxis, parent.axis, orient->axis);', 'MatrixMultiply(tagAxis, (float (*)[3])&parent, orient->axis);'),
                code.replace('MatrixTransformVector43(mtx->trans, parentMatrix, orient->origin);', 'MatrixTransformVector43(mtx->trans, (float (*)[3])&parent, orient->origin);'),
                code.replace('    return orient;\n}', '}')]
    for index, variant in enumerate(variants):
        (path / 'test.c').write_text(variant)
        build = subprocess.run(['cc', '-std=c11', '-O1', '-g', '-Werror=return-type', '-fsanitize=address,undefined',
                                str(path / 'test.c'), '-lm', '-o', str(path / 'test')], capture_output=True)
        if index == 3:
            assert build.returncode != 0, 'Missing cache return must be rejected'
            continue
        assert build.returncode == 0, build.stderr.decode()
        result = subprocess.run([str(path / 'test')], capture_output=True)
        assert (result.returncode == 0) == (index == 0), result.stderr.decode()
        if index == 0:
            print(result.stdout.decode(), end='')
    print('PASS: origin/axis layout and missing-cache-return regressions rejected')
