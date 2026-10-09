#!/usr/bin/env python3
"""Guard the real FX line constructor against scaled pointer writes."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parent.parent
s=(root/'src/PC/EffectsCore/FxUtil.c').read_text();a=s.index('void FX_AddLine(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch)\n{');b=s.index('\nextern void Particle_Particle',a)
body=s[a:b]
support=r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
typedef unsigned char byte;typedef float vec_t;typedef float vec3_t[3];typedef void *MaterialHandle;
typedef struct {void *vtable;float origin[3];struct{MaterialHandle customMaterial;}mRefEnt;int mSortGroup;}Effect;
typedef struct {Effect base;float blendWeight[4],endpoint[3];} Particle;
typedef Particle Tail;
typedef struct {int mMediaHandles,mAttributeFlags;}PrimitiveTemplate;
typedef struct {PrimitiveTemplate *primTemp;struct{void *value;}boltFrame;}EffectPrimitive;
#define PART_ANCHOR_X(p) (((Effect*)(p))->origin[0])
#define PART_ANCHOR_Y(p) (((Effect*)(p))->origin[1])
#define PART_ANCHOR_Z(p) (((Effect*)(p))->origin[2])
static void *allocated,*theFxHelper;static int material,refractive;
static void *__Znam(int size){return allocated=malloc(size);}
static void Line_Line(void*p){}
static int FX_AddPrimitive(void*p,void*q,const float*o){return 1;}
static void FX_CalcOriginAndAxis(void*p,float*out,const float*in,float(*ax)[3]){memcpy(out,in,12);}
static void Particle_SetAxis(void*p,float(*a)[3]){}
static void FX_CalcOrigin2(const void*p,float*o,float*out,const float*in,float(*ax)[3]){for(int i=0;i<3;i++)out[i]=in[i]+ax[0][i]*20;}
static void *MediaHandles_GetHandle(void*p){return &material;}
static void *FxBoltFrame_GetOrientation(void*p){return p;}
static void OrientationPosFromWorldPos(void*p,float*in,float*out){memcpy(out,in,12);}
static float flrand(float a,float b){return (a+b)*.5f;}
static int FxHelper_IsMaterialRefractive(void*p,void*m){return refractive;}
'''
checks=r'''
int main(void){for(int i=0;i<2;i++){
 PrimitiveTemplate temp={.mAttributeFlags=0x1e000};EffectPrimitive prim={.primTemp=&temp};
 vec3_t axes[3]={{1,0,0},{0,1,0},{0,0,1}},origin={123,456,789};refractive=i;
 FX_AddLine(&prim,axes,origin,0,0);Tail *line=allocated;
 assert(line->base.mRefEnt.customMaterial==&material);assert(line->base.mSortGroup==-i);
 assert(line->base.origin[0]==123&&line->endpoint[0]==143&&line->endpoint[2]==789);
 for(int j=0;j<4;j++)assert(line->blendWeight[j]==.5f);free(allocated);
}return 0;}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.c').write_text(support+body+checks)
 subprocess.run(['cc','-O1','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: FX line material, endpoints and sorting stay inside the allocated effect')
