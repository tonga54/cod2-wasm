#!/usr/bin/env python3
"""Compare accelerated brush sweeps with the exhaustive solver under sanitizers."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
trace = (root / 'src/PC/qcommon/cm_trace.c').read_text()
box = (root / 'src/PC/qcommon/cm_tracebox.c').read_text()

def function(source, name):
    m = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert m, name
    end, depth = m.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[m.start():end] + '\n'

support = r"""
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
typedef float vec_t,vec3_t[3];typedef int qboolean;
typedef unsigned char byte;typedef int clipHandle_t;
#define __attribute_regparm__(n)
#define CM_TEMP_BOX_MODEL 0x3ff
typedef struct {vec3_t start,end,invDelta;} TraceExtents;
typedef struct {TraceExtents extents;vec3_t size,midpoint,delta,halfDelta,halfDeltaAbs;
 float deltaLenSq,deltaLen,radius,offsetZ;vec3_t bounds[2],radiusOffset;
 int contents,isPoint,axialCullOnly;} traceWork_t;
typedef struct {float fraction;vec3_t normal;int contents,startsolid,allsolid,material;} trace_t;
typedef struct {vec3_t normal;float dist;} cplane_t;
typedef struct {cplane_t *plane;int materialNum;} cbrushside_t;
typedef struct {vec3_t mins,maxs;int contents,numsides,axialMaterialNum[2][3];cbrushside_t *sides;} cbrush_t;
typedef struct {int leafBrushCount,contents;union {struct {unsigned short *brushes;} leaf;
 struct {unsigned short childOffset[2];} children;} data;} cLeafBrushNode_t;
typedef struct {int brushContents,terrainContents,leafBrushNode,firstCollAabbIndex,collAabbCount;vec3_t mins,maxs;} cLeaf_t;
typedef struct {cLeaf_t leaf;} cmodel_t;typedef struct {int unused;} CollisionAabbTree;
static struct {int numBrushes,leafbrushNodesCount,numLeafs,aabbTreeCount;cbrush_t *brushes;
 cLeafBrushNode_t *leafbrushNodes;cLeaf_t *leafs;CollisionAabbTree *aabbTrees;} cm;
static __thread unsigned long planeTests,brushTests,worldTraces;
static int leafRepeat=1024;
static void CM_InitTraceThreadInfo(traceWork_t *tw) {}
static void CM_SetTraceMaterial(trace_t *t,int material,int contents) {if(material>=0)t->material=material;t->contents=contents;}
static cmodel_t *CM_ModelForHandle(int model) {return NULL;}
static cbrush_t *CM_BoxBrush(void) {return cm.brushes;}
static void CM_TraceThroughAabbTree(const traceWork_t *tw,CollisionAabbTree *tree,trace_t *t) {assert(0);}
static int CM_BoxLeafnums(const vec_t *lo,const vec_t *hi,int *list,int count,int *last) {
 assert(count==1024);for(int i=0;i<leafRepeat;i++)list[i]=0;return leafRepeat;
}
"""
code = ''.join(function(box, n) for n in ('CM_CalcTraceEntents', 'CM_TraceBox'))
code += trace[trace.index('#define CM_WORLD_TRACE_CACHE_SIZE'):trace.index('static float CM_DotProduct')]
code += ''.join(function(trace, n) for n in ('CM_DotProduct', 'CM_AbsFloat',
    'CM_MinFloat', 'CM_InitTraceWork', 'CM_TraceMayHitBounds', 'CM_ClearTraceResult'))
clip = function(trace, 'CM_ClipBrushPlaneDistances')
axial = function(trace, 'CM_TraceBrushAxialPlane').replace('    vec3_t normal', '    planeTests++;\n    vec3_t normal')
plane = function(trace, 'CM_TraceBrushPlane').replace('    float support;', '    planeTests++;\n    float support;')
bounds = function(trace, 'CM_TraceBoundsOverlap') + function(trace, 'CM_BrushBoundsOverlap') + function(trace, 'CM_LeafBrushBlocks')
planes = function(trace, 'CM_TraceBrushPlanes')
brush = function(trace, 'CM_TraceThroughBrush').replace('    if (CM_BrushBoundsOverlap', '    brushTests++;\n    if (CM_BrushBoundsOverlap')
reference = planes.replace('CM_TraceBrushPlanes(', 'CM_TraceThroughBrush_reference(')
reference = reference.replace('    if (!CM_TraceMayHitBounds', '    if (!CM_BrushBoundsOverlap(tw, brush)) return;\n\n    if (!CM_TraceMayHitBounds')
reference = reference.replace('CM_TraceBrushAxialPlane(', 'CM_TraceBrushAxialReference(')
reference = reference.replace('''    if (!CM_TraceMayHitBounds(tw, brush->mins, brush->maxs, trace))
        return;''', '')
assert reference != brush
world = ''.join(function(trace, n) for n in ('CM_TraceLeafBrushNode_r',
    'CM_TraceLeafBrushes', 'CM_TraceLeafTerrain', 'CM_TraceLeaf', 'CM_Trace', 'CM_BoxTrace'))
world = world.replace('    traceWork_t tw;', '    worldTraces++;\n    traceWork_t tw;')
axialReference = r'''
static void CM_TraceBrushAxialReference(const traceWork_t *tw,int axis,float sign,
 float dist,int materialNum,float *enterFrac,float *leaveFrac,vec3_t enterNormal,
 int *enterMaterial,int *startsOut,int *getOut) {
 vec3_t normal={0};normal[axis]=sign;
 CM_TraceBrushPlane(tw,normal,dist,materialNum,enterFrac,leaveFrac,enterNormal,enterMaterial,startsOut,getOut);
}
'''
checks = r"""
static uint32_t seed=123456;
static float rnd(float scale) {seed=seed*1664525u+1013904223u;return (seed>>8)*(1.f/16777216.f)*scale;}
static void equal(trace_t a,trace_t b) {
 assert(a.fraction==b.fraction && a.startsolid==b.startsolid && a.allsolid==b.allsolid);
 assert(a.contents==b.contents && a.material==b.material);
 for(int i=0;i<3;i++)assert(a.normal[i]==b.normal[i]);
}
static void *cacheThread(void *unused) {
 vec3_t a={-10,0,0},b={11000,0,0},zero={0};trace_t result;
 for(int n=0;n<1000;n++)CM_BoxTrace(&result,a,b,zero,zero,0,1);
 assert(worldTraces==1);assert(result.fraction<1);
 return NULL;
}
static void checkBrushBlocks(void) {
 cbrush_t brushes[1025]={0};unsigned short indices[1025];
 cLeafBrushNode_t node={.contents=3,.data.leaf.brushes=indices};
 cm.brushes=brushes;cm.numBrushes=1025;
 for(int i=0;i<1025;i++) {
  indices[i]=i;brushes[i].contents=1+(i%2);
  for(int a=0;a<3;a++){brushes[i].mins[a]=rnd(4096)-2048;brushes[i].maxs[a]=brushes[i].mins[a]+rnd(128)+1;}
 }
 int counts[]={1,15,16,17,31,32,33,1023,1024,1025};
 for(int run=0;run<4000;run++) {
  node.leafBrushCount=counts[run%10];
  if(run%20==0) { // Reload at the same addresses with changed geometry/masks.
   int n=run%1025;brushes[n].mins[0]-=32;brushes[n].contents^=3;
   CM_ClearWorldTraceCache();
  }
  if(run%50==0) { // Unordered duplicates and invalid indices, as in raw lists.
   for(int i=0;i<1025;i++)indices[i]=(unsigned short)(rnd(1050));
   CM_ClearWorldTraceCache();
  }
  vec3_t start,end,mins={-15,-15,-15},maxs={15,15,55};
  for(int a=0;a<3;a++){start[a]=rnd(6000)-3000;end[a]=rnd(6000)-3000;}
  if(run%3==0)memset(mins,0,sizeof(mins)),memset(maxs,0,sizeof(maxs));
  if(run%5==0)memcpy(end,start,sizeof(end));
  traceWork_t tw;CM_InitTraceWork(&tw,start,end,mins,maxs,1+run%3);
  trace_t ref={.fraction=run%4==0?.25f:1},fast=ref;
  CM_TraceLeafBrushNode_r(&tw,&node,&ref,0,NULL);
  byte seen[8192]={0};CM_TraceLeafBrushNode_r(&tw,&node,&fast,0,seen);equal(ref,fast);
 }
 // Spatially separated consecutive groups must avoid visiting far brushes.
 for(int i=0;i<1024;i++) {
  indices[i]=i;brushes[i]=(cbrush_t){.contents=1,.mins={i*100.f,0,0},.maxs={i*100.f+20,20,20}};
 }
 node.leafBrushCount=1024;CM_ClearWorldTraceCache();
 traceWork_t tw;vec3_t a={-10,10,10},b={21,10,10},z={0};CM_InitTraceWork(&tw,a,b,z,z,1);
 trace_t ref={.fraction=1},fast=ref;brushTests=0;
 CM_TraceLeafBrushNode_r(&tw,&node,&ref,0,NULL);unsigned long exhaustive=brushTests;
 byte seen[8192]={0};brushTests=0;CM_TraceLeafBrushNode_r(&tw,&node,&fast,0,seen);equal(ref,fast);
 assert(exhaustive==1024 && brushTests<=8);
 unsigned long separatedVisits=brushTests;
 // Broad swept bounds contain these off-axis groups, but the ray misses all.
 for(int i=0;i<1024;i++)brushes[i]=(cbrush_t){.contents=1,
  .mins={i*4.f,i*4.f+128,0},.maxs={i*4.f+2,i*4.f+130,20}};
 CM_ClearWorldTraceCache();a[0]=a[1]=-10;b[0]=b[1]=8192;a[2]=b[2]=10;
 CM_InitTraceWork(&tw,a,b,z,z,1);ref=(trace_t){.fraction=1};fast=ref;
 CM_TraceLeafBrushNode_r(&tw,&node,&ref,0,NULL);
 memset(seen,0,sizeof(seen));brushTests=0;
 CM_TraceLeafBrushNode_r(&tw,&node,&fast,0,seen);equal(ref,fast);
 assert(brushTests==0);
 puts("PASS: off-axis brush groups rejected before all 1024 individual brush visits; exact miss retained");
 printf("PASS: 4000 ordered brush-block comparisons, partial/max/uncached lists, masks, overlaps, duplicate/invalid entries and reloads; distant brush visits %lu -> %lu\n",exhaustive,separatedVisits);
}
int main(void) {
 cplane_t face; cbrushside_t side={.plane=&face,.materialNum=13};
 for(int n=0;n<100000;n++) {
  vec3_t start,end,mins,maxs;cbrush_t b={.contents=1,.sides=&side,.numsides=n%2};
  for(int a=0;a<3;a++) {
   b.mins[a]=rnd(2000)-1000;b.maxs[a]=b.mins[a]+rnd(300)+1;
   start[a]=rnd(4000)-2000;end[a]=rnd(16000)-8000;
   if(n%7==0)start[a]=b.mins[a]+rnd(b.maxs[a]-b.mins[a]);
   if(n%11==0)end[a]=start[a];
   mins[a]=-15;maxs[a]=a==2?rnd(60)+15:15;
   if(n%3==0)mins[a]=maxs[a]=0;
  }
  float angle=rnd(6.2831853f);face=(cplane_t){.normal={cosf(angle),sinf(angle),0},.dist=rnd(500)};
  traceWork_t tw;CM_InitTraceWork(&tw,start,end,mins,maxs,1);
  trace_t ref={.fraction=(n%5==0?rnd(1):1)},fast=ref;
  CM_TraceThroughBrush_reference(&tw,&b,&ref);CM_TraceThroughBrush(&tw,&b,&fast);equal(ref,fast);
 }
 /* A long diagonal sightline's bounding box contains an entire city. The
  * exhaustive solver and accelerated solver must choose exactly the same hit. */
 vec3_t start={-10,-10,20},end={8192,8192,20},zero={0};traceWork_t tw;
 CM_InitTraceWork(&tw,start,end,zero,zero,1);trace_t ref={.fraction=1},fast=ref;
 unsigned long oldPlanes,newPlanes;planeTests=0;
 for(int x=0;x<100;x++)for(int y=0;y<100;y++) {
  cbrush_t b={.mins={x*80.f,y*80.f,0},.maxs={x*80.f+40,y*80.f+40,100},.contents=1};
  CM_TraceThroughBrush_reference(&tw,&b,&ref);
 }
 oldPlanes=planeTests;planeTests=0;
 for(int x=0;x<100;x++)for(int y=0;y<100;y++) {
  cbrush_t b={.mins={x*80.f,y*80.f,0},.maxs={x*80.f+40,y*80.f+40,100},.contents=1};
  CM_TraceThroughBrush(&tw,&b,&fast);
 }
 newPlanes=planeTests;equal(ref,fast);assert(newPlanes*100<oldPlanes);
 /* Bounds rejection includes parallel rays, grazing, capsule extent, static
  * overlaps and the contact epsilon before an axial wall. */
 vec3_t lo={0,0,0},hi={40,40,100};
 assert(CM_TraceMayHitBounds(&tw,lo,hi,&fast));
 vec3_t farLo={80,80,0},farHi={120,120,100};
 assert(!CM_TraceMayHitBounds(&tw,farLo,farHi,&fast));
 /* The actual world trace must visit each shared BSP brush only once, use
  * its own bitmap on every invocation, and preserve overlaps and masks. */
 cbrush_t shared[256];unsigned short indexes[256];
 for(int i=0;i<256;i++) {
  shared[i]=(cbrush_t){.mins={i*40.f,-100,-100},.maxs={i*40.f+20,100,100},.contents=1};indexes[i]=i;
 }
 cLeafBrushNode_t nodes[4]={{.leafBrushCount=-1,.contents=1,.data.children.childOffset={2,3}},
  {.leafBrushCount=256,.contents=1,.data.leaf.brushes=indexes},
  {.leafBrushCount=256,.contents=1,.data.leaf.brushes=indexes},
  {.leafBrushCount=256,.contents=1,.data.leaf.brushes=indexes}};
 cLeaf_t leaf={.brushContents=1,.leafBrushNode=0,.mins={-1,-101,-101},.maxs={10250,101,101}};
 cm.numBrushes=256;cm.brushes=shared;cm.numLeafs=1;cm.leafs=&leaf;
 cm.leafbrushNodesCount=4;cm.leafbrushNodes=nodes;
 for(int direction=0;direction<2;direction++)for(int overlap=0;overlap<2;overlap++) {
  vec3_t a={direction?11000.f:-10.f,0,0},b={direction?-10.f:11000.f,0,0};
  if(overlap)a[0]=10;
  CM_InitTraceWork(&tw,a,b,zero,zero,1);ref=(trace_t){.fraction=1};
  CM_TraceLeafBrushNode_r(&tw,nodes,&ref,0,NULL);
  for(int repetition=0;repetition<8;repetition++) {
   fast=(trace_t){.fraction=1};brushTests=0;
   CM_Trace(&fast,a,b,zero,zero,0,1);equal(ref,fast);if(!overlap)assert(brushTests<=256);
  }
  fast=(trace_t){.fraction=1};CM_Trace(&fast,a,b,zero,zero,0,2);assert(fast.fraction==1&&!fast.startsolid);
 }
 /* Exact-key world caching must preserve all fields, start/end directions,
  * capsule dimensions, contents masks, overlaps, and cache-slot collisions. */
 leafRepeat=1;
 for(int n=0;n<10000;n++) {
  vec3_t a={rnd(12000)-500,rnd(400)-200,rnd(400)-200};
  vec3_t b={rnd(12000)-500,rnd(400)-200,rnd(400)-200};
  vec3_t mins={-15,-15,-15},maxs={15,15,55};
  if(n%3==0)memset(mins,0,sizeof(mins)),memset(maxs,0,sizeof(maxs));
  if(n%7==0)memcpy(b,a,sizeof(b));
  int mask=n%5==0?2:1;ref=(trace_t){.fraction=1};
  CM_Trace(&ref,a,b,mins,maxs,0,mask);
  for(int repeat=0;repeat<4;repeat++) {
   CM_BoxTrace(&fast,a,b,mins,maxs,0,mask);equal(ref,fast);
  }
 }
 vec3_t a={-10,0,0},b={11000,0,0};trace_t cached;
 CM_ClearWorldTraceCache();unsigned long calls=worldTraces;
 for(int n=0;n<1000;n++)CM_BoxTrace(&cached,a,b,zero,zero,0,1);
 assert(worldTraces==calls+1);
 /* A one-bit coordinate difference and signed zero are distinct keys. */
 vec3_t adjacent;memcpy(adjacent,a,sizeof(adjacent));
 adjacent[0]=nextafterf(adjacent[0],0.f);calls=worldTraces;
 CM_BoxTrace(&fast,adjacent,b,zero,zero,0,1);assert(worldTraces==calls+1);
 adjacent[0]=a[0];adjacent[1]=-0.f;calls=worldTraces;
 CM_BoxTrace(&fast,adjacent,b,zero,zero,0,1);assert(worldTraces==calls+1);
 /* Caller edits never poison cached results; moving entities use a temporary
  * box and cannot reuse a stale world result. */
 cached.normal[0]=123;CM_BoxTrace(&cached,a,b,zero,zero,0,1);assert(cached.normal[0]!=123);
 calls=worldTraces;
 for(int n=0;n<2;n++)CM_BoxTrace(&fast,a,b,zero,zero,CM_TEMP_BOX_MODEL,1);
 assert(worldTraces==calls+2);
 /* A map reload can reuse the same addresses and coordinates. */
 shared[0].mins[0]=200;shared[0].maxs[0]=220;
 CM_ClearWorldTraceCache();CM_BoxTrace(&fast,a,b,zero,zero,0,1);
 assert(fast.fraction!=cached.fraction);
 shared[0].mins[0]=0;shared[0].maxs[0]=20;CM_ClearWorldTraceCache();
 for(int round=0;round<2;round++) {
  pthread_t threads[8];for(int n=0;n<8;n++)assert(!pthread_create(&threads[n],NULL,cacheThread,NULL));
  for(int n=0;n<8;n++)assert(!pthread_join(threads[n],NULL));CM_ClearWorldTraceCache();
 }
 checkBrushBlocks();
 printf("PASS: 100000 exhaustive/accelerated sweep comparisons; city planes %lu -> %lu with identical hit\n",oldPlanes,newPlanes);
 puts("PASS: 1024 shared BSP leaves, recursive shared nodes, independent traces, overlaps and masks");
 puts("PASS: 40000 cached/uncached exact-hit comparisons, 1000-to-1 repeated sweeps, dynamic box exclusion, map invalidation and independent thread caches");
}
"""
if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix='cod2-trace-pruning-') as directory:
        d = Path(directory)
        (d / 'test.c').write_text(support + code + clip + plane + axial + axialReference + bounds + planes + brush + reference + world + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(d / 'test.c'), '-o', str(d / 'test'), '-lm', '-pthread'], check=True)
        subprocess.run([str(d / 'test')], check=True)
