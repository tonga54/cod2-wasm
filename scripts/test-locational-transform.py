#!/usr/bin/env python3
"""Check the actual server trace entry points use a complete affine transform."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/server_mp/sv_world_mp.c').read_text()
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
#include <stdlib.h>
#define Com_Printf(...) ((void)0)
typedef struct { void *skel; int numBones; } DObj;
typedef float vec_t;
typedef float vec3_t[3];
typedef int clipHandle_t;
typedef struct { vec3_t start,end; } TraceExtents;
typedef struct { TraceExtents extents; int contentmask,passEntityNum,passOwnerNum,bLocational; unsigned char *priorityMap; } pointtrace_t;
typedef struct { vec3_t start,end; int contentmask,passEntityNum[2],locational; } sightpointtrace_t;
typedef struct { float fraction; int surfaceFlags,partName,partGroup,entityNum,contents; vec3_t normal; void *material; } trace_t;
typedef struct { float fraction; int surfaceflags,partName,partGroup; vec3_t normal; } DObjTrace;
typedef struct { int linkcontents; } svEntity_t;
typedef struct { svEntity_t svEntities[64]; } server_t;
typedef struct { struct { int number; struct { int brushmodel; } index; } s;
 struct { int contents,ownerNum,svFlags,bmodel; vec3_t currentOrigin,currentAngles,absmin,absmax,mins,maxs; } r; } gentity_t;
static server_t server; static void *imp_sv=&server;
static gentity_t entity;
static vec3_t zero,actorLocationalMins={-64,-64,0},actorLocationalMaxs={64,64,72};
static void *imp_vec3_origin=zero;
static int calls;
static gentity_t *SV_GentityNum(int n) { return &entity; }
static void *Com_GetServerDObj(int n) { return &entity; }
static int DObjHasContents(void *o,int mask) { return 1; }
static void DObjGetBounds(void *o,float *lo,float *hi) { memcpy(lo,actorLocationalMins,12);memcpy(hi,actorLocationalMaxs,12); }
static int CM_TraceBox(const void *e,const float *lo,const float *hi,float f) { return 0; }
static void CM_CalcTraceEntents(void *e) {}
static void G_DObjCalcPose(void *e) {}
static void AnglesToAxis(const float *a,float m[][3]) {
 float t=a[1]*.017453292519943f,c=cosf(t),s=sinf(t);
 m[0][0]=c;m[0][1]=s;m[0][2]=0;
 m[1][0]=-s;m[1][1]=c;m[1][2]=0;
 m[2][0]=0;m[2][1]=0;m[2][2]=1;
}
static void MatrixTransformVector(const float *p,const float m[][3],float *out) {
 for(int a=0;a<3;a++)out[a]=p[0]*m[0][a]+p[1]*m[1][a]+p[2]*m[2][a];
}
static void MatrixTransposeTransformVector43(const float *p,const float *m,float *out) {
 float v[3];for(int a=0;a<3;a++)v[a]=p[a]-m[9+a];
 for(int a=0;a<3;a++)out[a]=v[0]*m[3*a]+v[1]*m[3*a+1]+v[2]*m[3*a+2];
}
static void checkTrace(const float *s,const float *e,DObjTrace *t) {
 assert(fabsf(s[0]+100)<.001f && fabsf(e[0]-100)<.001f);
 assert(fabsf(s[1])<.001f && fabsf(e[1])<.001f);
 assert(fabsf(s[2]-40)<.001f && fabsf(e[2]-40)<.001f);
 t->fraction=.45f;t->surfaceflags=0;t->partName=1;t->partGroup=4;
 t->normal[0]=-1;t->normal[1]=t->normal[2]=0;calls++;
}
static void DObjTraceline(void *o,const float *s,const float *e,unsigned char *p,DObjTrace *t) { checkTrace(s,e,t); }
static void DObjGeomTraceline(void *o,const float *s,const float *e,int p,DObjTrace *t) { checkTrace(s,e,t); }
static int CM_TempBoxModel(const float *lo,const float *hi,int mask) { assert(0);return 0; }
static void CM_TransformedBoxTrace(trace_t *t,const float *s,const float *e,const float *lo,const float *hi,int handle,int mask,const float *o,const float *a) { assert(0); }
static int CM_TransformedBoxSightTrace(int n,const float *s,const float *e,const float *lo,const float *hi,int handle,int mask,const float *o,const float *a) { assert(0);return 0; }
'''
checks = r'''
int main(void) {
 unsigned char priorities[32]={0};entity.s.number=17;entity.r.contents=1;entity.r.ownerNum=1023;
 for(int n=0;n<32;n++) {
  float m[3][3],s[3]={-100,0,40},e[3]={100,0,40};
  entity.r.currentAngles[1]=n*11.25f;
  for(int a=0;a<3;a++)entity.r.currentOrigin[a]=(a+1)*113.7f+n*17;
  AnglesToAxis(entity.r.currentAngles,m);
  pointtrace_t clip={.contentmask=1,.passEntityNum=1023,.passOwnerNum=1023,.bLocational=1,.priorityMap=priorities};
  MatrixTransformVector(s,m,clip.extents.start);MatrixTransformVector(e,m,clip.extents.end);
  for(int a=0;a<3;a++){clip.extents.start[a]+=entity.r.currentOrigin[a];clip.extents.end[a]+=entity.r.currentOrigin[a];}
  for(int flags=2;flags<=4;flags+=2) {
   entity.r.svFlags=flags;trace_t trace={.fraction=1};
   SV_PointTraceToEntity(&clip,&server.svEntities[17],&trace);
   assert(trace.entityNum==17 && trace.partGroup==4 && trace.fraction==.45f);
  }
  sightpointtrace_t sight={.contentmask=1,.passEntityNum={1023,1023},.locational=1};
  memcpy(sight.start,clip.extents.start,12);memcpy(sight.end,clip.extents.end,12);
  assert(SV_PointSightTraceToEntity(&sight,&server.svEntities[17])==-1);
 }
 assert(calls==96);return 0;
}
'''
body = function('SV_PointTraceToEntity') + function('SV_PointSightTraceToEntity')
with tempfile.TemporaryDirectory(prefix='cod2-locational-transform-') as directory:
    path = Path(directory)
    variants = [body, body.replace('float entAxis[4][3];', 'float entAxis[3][3];')
                .replace('memcpy(entAxis[3], origin, sizeof(origin));', '')]
    for index, variant in enumerate(variants):
        (path / 'test.c').write_text(support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True)
        assert (result.returncode == 0) == (index == 0), result.stderr.decode()
print('PASS: 96 translated/rotated actor and model traces; old 3x3 matrix fails ASan')
