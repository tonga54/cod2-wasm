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
static unsigned long planeTests,brushTests;
static void CM_InitTraceThreadInfo(traceWork_t *tw) {}
static void CM_SetTraceMaterial(trace_t *t,int material,int contents) {if(material>=0)t->material=material;t->contents=contents;}
static cmodel_t *CM_ModelForHandle(int model) {return NULL;}
static cbrush_t *CM_BoxBrush(void) {return cm.brushes;}
static void CM_TraceThroughAabbTree(const traceWork_t *tw,CollisionAabbTree *tree,trace_t *t) {assert(0);}
static int CM_BoxLeafnums(const vec_t *lo,const vec_t *hi,int *list,int count,int *last) {
 assert(count==1024);for(int i=0;i<1024;i++)list[i]=0;return 1024;
}
"""
code = ''.join(function(box, n) for n in ('CM_CalcTraceEntents', 'CM_TraceBox'))
code += ''.join(function(trace, n) for n in ('CM_DotProduct', 'CM_AbsFloat',
    'CM_MinFloat', 'CM_InitTraceWork', 'CM_TraceMayHitBounds'))
plane = function(trace, 'CM_TraceBrushPlane').replace('    float support;', '    planeTests++;\n    float support;')
brush = function(trace, 'CM_TraceThroughBrush').replace('    int axis;', '    brushTests++;\n    int axis;')
reference = brush.replace('CM_TraceThroughBrush(', 'CM_TraceThroughBrush_reference(')
reference = reference.replace('''    if (!CM_TraceMayHitBounds(tw, brush->mins, brush->maxs, trace))
        return;''', '')
assert reference != brush
world = ''.join(function(trace, n) for n in ('CM_TraceLeafBrushNode_r',
    'CM_TraceLeafBrushes', 'CM_TraceLeafTerrain', 'CM_TraceLeaf', 'CM_Trace'))
checks = r"""
static uint32_t seed=123456;
static float rnd(float scale) {seed=seed*1664525u+1013904223u;return (seed>>8)*(1.f/16777216.f)*scale;}
static void equal(trace_t a,trace_t b) {
 assert(a.fraction==b.fraction && a.startsolid==b.startsolid && a.allsolid==b.allsolid);
 assert(a.contents==b.contents && a.material==b.material);
 for(int i=0;i<3;i++)assert(a.normal[i]==b.normal[i]);
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
 printf("PASS: 100000 exhaustive/accelerated sweep comparisons; city planes %lu -> %lu with identical hit\n",oldPlanes,newPlanes);
 puts("PASS: 1024 shared BSP leaves, recursive shared nodes, independent traces, overlaps and masks");
}
"""
with tempfile.TemporaryDirectory(prefix='cod2-trace-pruning-') as directory:
    d = Path(directory)
    (d / 'test.c').write_text(support + code + plane + brush + reference + world + checks)
    subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                    str(d / 'test.c'), '-o', str(d / 'test'), '-lm'], check=True)
    subprocess.run([str(d / 'test')], check=True)
