#!/usr/bin/env python3
"""Exercise the real FX flag parser, world origins and tracer scheduling."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parent.parent
def function(path,name):
 s=(root/path).read_text();m=re.search(r'[^\n;]*\b'+name+r'\([^;]*?\)\n\{',s);assert m,name
 end=m.end();depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[m.start():end]+'\n'
support=r'''
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <math.h>
typedef unsigned char byte;typedef int Bool;typedef float vec_t;typedef float vec3_t[3];
typedef struct {float mMin,mMax;} FxRange;
typedef struct {int mAttributeFlags,mSpawnFlags;FxRange mOrigin1X,mOrigin1Y,mOrigin1Z,mRadius,mHeight;} PrimitiveTemplate;
typedef struct {const char *flag;unsigned masks[2];} FxFlagEntry;
typedef struct {void *fx;PrimitiveTemplate *primTemp;struct {void *value;} boltFrame;} EffectPrimitive;
#define stricmp strcasecmp
static void *Hunk_AllocateTempMemoryInternal(int n){return malloc(n);}
static void Hunk_FreeTempMemory(void *p){free(p);}
static float FxRange_GetVal(void *p){FxRange *r=p;return (r->mMin+r->mMax)*.5f;}
static float flrand(float a,float b){return(a+b)*.5f;}
static void AxisTransformVector(void *p,float x,float y,float z,float*out){float(*a)[3]=p;for(int i=0;i<3;i++)out[i]=x*a[0][i]+y*a[1][i]+z*a[2][i];}
static void Vec3Cross(const float*a,const float*b,float*c){float t[3]={a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};memcpy(c,t,sizeof(t));}
static float Vec3Normalize(float*v){float n=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(n)for(int i=0;i<3;i++)v[i]/=n;return n;}
static void MakeNormalVectors(float*a,float*b,float*c){b[0]=0;b[1]=1;b[2]=0;Vec3Cross(a,b,c);}
static void RotatePointAroundVector(float*out,float*axis,float*p,float angle){memcpy(out,p,12);}
static void *FxBoltFrame_GetOrientation(void*p){return p;}
static void OrientationPosFromWorldPos(void*p,float*in,float*out){float*v=p;for(int i=0;i<3;i++)out[i]=in[i]-v[i];}
typedef struct {int leType,endTime;float tracerClipDist;struct {int trType,trTime;float trBase[3],trDelta[3];}pos;} localEntity_t;
static localEntity_t entity;static int allocations;
static localEntity_t *CG_AllocLocalEntity(void){allocations++;return &entity;}
#define LE_MOVING_TRACER 4
#define TR_LINEAR 2
static struct {int time,frametime;} state,*cg=&state;
static struct {struct{float value;}current;} speedDvar,*cg_tracerSpeed=&speedDvar;
'''
body=function(Path('src/PC/EffectsCore/FxTemplate.c'),'PrimitiveTemplate_ParseFlags')+function(Path('src/PC/EffectsCore/FxUtil.c'),'FX_CalcOriginAndAxis_impl')+function(Path('src/PC/cgame_mp/cg_weapons.c'),'CG_SpawnTracer')
checks=r'''
int main(void){
 FxFlagEntry flags[]={{"usePhysics",{32,0}},{"useAlpha",{128,0}},{"frustumCull",{0,1024}}};
 for(int spaces=1;spaces<=64;spaces++){
  char line[256];memset(line,' ',sizeof(line));memcpy(line,"usePhysics",10);memcpy(line+10+spaces,"useAlpha",8);line[18+spaces]=0;
  PrimitiveTemplate t={0};assert(PrimitiveTemplate_ParseFlags(&t,line,flags,3));assert(t.mAttributeFlags==160);
 }
 PrimitiveTemplate t={0};assert(PrimitiveTemplate_ParseFlags(&t," \tuseAlpha   frustumCull\t ",flags,3));assert(t.mAttributeFlags==128&&t.mSpawnFlags==1024);
 assert(!PrimitiveTemplate_ParseFlags(&t,"unknown",flags,3));
 for(int j=-128;j<=128;j++)for(int flags=0;flags<=64;flags+=64){
  PrimitiveTemplate p={.mSpawnFlags=flags,.mOrigin1X={2,2},.mOrigin1Y={3,3},.mOrigin1Z={4,4}};
  EffectPrimitive prim={.primTemp=&p};float origin[3]={j*13,j*7,j*3},out[3];vec3_t axis[3]={{0,1,0},{-1,0,0},{0,0,1}};
  FX_CalcOriginAndAxis_impl((byte*)&prim,out,origin,axis);
  assert(out[0]==origin[0]+(flags?2:-3));assert(out[1]==origin[1]+(flags?3:2));assert(out[2]==origin[2]+4);
 }
 float start[3]={1000,1200,64},end[3];state.time=100000;
 for(int frame=1;frame<=66;frame++)for(int distance=1;distance<=4096;distance+=31){
  state.frametime=frame;speedDvar.current.value=4500;memcpy(end,start,12);end[0]+=distance;allocations=0;CG_SpawnTracer(start,end);
  assert(allocations==1&&isfinite(entity.tracerClipDist));assert(fabsf(entity.tracerClipDist-distance)<.01);assert(entity.pos.trBase[0]==1000);assert(entity.pos.trDelta[0]==4500);
  assert(entity.endTime>=entity.pos.trTime);assert(state.time-entity.pos.trTime<=frame/2);
 }
 allocations=0;speedDvar.current.value=0;CG_SpawnTracer(start,end);assert(!allocations);
 speedDvar.current.value=4500;CG_SpawnTracer(start,start);assert(!allocations);
 puts("PASS: FX flag whitespace, 514 world-space origins, 8,778 tracer schedules and zero-speed guards");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'fx.c';p.write_text(support+body+checks)
 subprocess.run(['cc','-std=c11','-fsanitize=address,undefined','-g',str(p),'-lm','-o',str(Path(d)/'test')],check=True)
 subprocess.run([str(Path(d)/'test')],check=True)
