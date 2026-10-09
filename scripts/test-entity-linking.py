#!/usr/bin/env python3
"""Exercise actual temporary collision models and world-sector entity lists."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent

def function(source, name):
    match = re.search(r'(?m)^[^\n;]*\b' + name + r'\([^;]*?\)\s*\{', source)
    assert match, name
    start = match.start()
    if source[start:].lstrip().startswith(name + '('):
        start = source.rfind('\n', 0, start - 1) + 1
    end = source.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'

world = (root / 'src/PC/qcommon/cm_world.c').read_text()
trace = (root / 'src/PC/qcommon/cm_trace.c').read_text()
support = r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#define __attribute_regparm__(n)
typedef unsigned char byte;
typedef float vec_t;
typedef float vec2_t[2];
typedef float vec3_t[3];
typedef float vec4_t[4];
typedef int clipHandle_t;
typedef struct { vec3_t mins,maxs; int contents; } cbrush_t;
typedef struct { vec3_t mins,maxs; struct { int brushContents,terrainContents; } leaf; } cmodel_t;
typedef struct { cbrush_t *box_brush; cmodel_t *box_model; } TraceThreadInfo;
typedef struct { unsigned short worldSector,nextEntityInWorldSector; int linkcontents; vec2_t linkmin,linkmax; } svEntity_t;
typedef struct { struct { int contents; vec3_t absmin,absmax; } r; } gentity_t;
typedef struct { svEntity_t svEntities[1024]; } server_t;
typedef struct { struct { int contentsEntities,contentsStaticModels; unsigned short entities,staticModels; } contents;
 struct { unsigned short axis,child[2]; float dist; union { unsigned short parent,nextFree; } u; } tree; } worldSector_t;
typedef struct { int lockTree; unsigned short freeHead; vec3_t mins,maxs; worldSector_t sectors[1024]; } cm_world_t;
typedef struct { vec3_t absmin,absmax; void *xmodel; struct { unsigned short nextModelInWorldSector; } writable; } cStaticModel_t;
typedef struct { int numStaticModels; cStaticModel_t *staticModelList; } clipMap_t;
typedef struct { vec3_t start,end; } TraceExtents;
typedef struct { TraceExtents extents; int contentmask; } pointtrace_t;
typedef struct { float fraction; } trace_t;
typedef struct { const float *mins,*maxs; int *list,maxcount,count,contentmask; } areaParms_t;
static server_t sv; static void *imp_sv=&sv; static clipMap_t cm; static cm_world_t cm_world;
static gentity_t entities[1024]; static cbrush_t brush; static cmodel_t model;
static TraceThreadInfo info={&brush,&model}; static int seen[1024];
#define CM_TEMP_BOX_MODEL 1023
static TraceThreadInfo *CM_GetThreadInfo(void) { return &info; }
static void CM_CopyVec3(const float *s,float *d) { memcpy(d,s,12); }
static cmodel_t *CM_ClipHandleToModel(int h) { assert(h==1023);return &model; }
static gentity_t *SV_GEntityForSvEntity(svEntity_t *e) { return &entities[e-sv.svEntities]; }
static int XModelGetContents(void *m) { return 0; }
static void Com_DPrintf(const char *f,...) {}
static void CM_ModelBounds(int h,float *lo,float *hi) { for(int a=0;a<3;a++){lo[a]=-4096;hi[a]=4096;} }
static void SV_PointTraceToEntity(const pointtrace_t *c,svEntity_t *e,trace_t *t) { seen[e-sv.svEntities]++; }
'''
names = ['CM_UnlinkEntity', 'CM_WorldEntityIndex', 'CM_WorldEntityForIndex',
         'CM_AllocWorldSector', 'CM_EnsureChildNode', 'CM_InsertEntityIntoWorldSector',
         'CM_InsertStaticModelIntoWorldSector', 'CM_SortNode', 'CM_LinkWorld',
         'CM_LinkEntity', 'CM_AreaEntities_r', 'CM_AreaEntities',
         'CM_PointTraceToEntities_r', 'CM_PointTraceToEntities']
body = function(trace, 'CM_TempBoxModel') + ''.join(function(world, n) for n in names)
checks = r'''
static void linkPlayer(int n,int mask,float x,float y) {
 gentity_t *g=&entities[n];g->r.contents=mask;
 for(int a=0;a<3;a++) {float p=a==0?x:a==1?y:0;g->r.absmin[a]=p-15;g->r.absmax[a]=p+15;}
 int h=CM_TempBoxModel(g->r.absmin,g->r.absmax,mask);
 assert(model.leaf.brushContents==mask && brush.contents==mask);
 CM_LinkEntity(&sv.svEntities[n],g->r.absmin,g->r.absmax,h);
}
static void checkArea(int count) {
 int list[1024],found[1024]={0};float lo[3]={-9999,-9999,-9999},hi[3]={9999,9999,9999};
 int n=CM_AreaEntities(lo,hi,list,1024,0x2000001);assert(n==count);
 for(int i=0;i<n;i++){assert(list[i]>=0&&list[i]<64);assert(!found[list[i]]++);}
 for(int i=0;i<64;i++)assert(found[i]==!!sv.svEntities[i].worldSector);
}
int main(void) {
 CM_LinkWorld();cm_world.lockTree=1;
 for(int n=0;n<64;n++) {
  assert(CM_WorldEntityIndex(&sv.svEntities[n])==n+1);
  assert(CM_WorldEntityForIndex(n+1)==&sv.svEntities[n]);
 }
 for(int n=63;n>=0;n--)linkPlayer(n,0x2000000,0,0);
 assert(cm_world.sectors[1].contents.entities==1);
 for(int n=0;n<64;n++)assert(sv.svEntities[n].nextEntityInWorldSector==(n==63?0:n+2));
 pointtrace_t c={.extents={{-100,0,0},{100,0,0}},.contentmask=0x2000000};trace_t t={1};
 CM_PointTraceToEntities(&c,&t);
 for(int n=0;n<64;n++)assert(seen[n]==1);
 checkArea(64);
 for(int n=0;n<64;n++)CM_UnlinkEntity(&sv.svEntities[n]);
 checkArea(0);assert(cm_world.sectors[1].contents.contentsEntities==0);
 cm_world.lockTree=0;
 for(int round=0;round<32;round++) {
  for(int n=0;n<64;n++)linkPlayer(n,(n&1)?1:0x2000000,((n*79+round*31)%63-31)*100,((n*47+round*13)%61-30)*100);
  checkArea(64);
  for(int n=0;n<64;n+=2)CM_UnlinkEntity(&sv.svEntities[n]);
  checkArea(32);
  for(int n=1;n<64;n+=2)linkPlayer(n,0,0,0);
  checkArea(0);
 }
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-entity-links-') as directory:
    path = Path(directory)
    variants = [body,
                body.replace('model->leaf.brushContents = contents;', ''),
                body.replace('ent - sv->svEntities + 1', 'ent - sv->svEntities'),
                body.replace('[(unsigned int)entIndex - 1]', '[(unsigned int)entIndex]')]
    for index, variant in enumerate(variants):
        (path / 'test.c').write_text(support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(path / 'test.c'), '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, timeout=10)
        assert (result.returncode == 0) == (index == 0), result.stderr.decode()
print('PASS: 64 players, 32 sector relink cycles, zero-content unlink, point trace; three old-code mutants rejected')
