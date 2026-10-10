#!/usr/bin/env python3
"""Exercise actual finite mesh borders and their capsule corner normals."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/qcommon/cm_mesh.c').read_text()
def function(name):
    m = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert m, name
    end, depth = m.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[m.start():end] + '\n'

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CM_MESH_REGPARM3_ABI
typedef unsigned short ushort;
typedef struct {float fraction,normal[3];int startsolid,allsolid;} trace_t;
typedef struct {float plane[4],svec[4],tvec[4];} CollisionTriangle;
typedef struct {float distEq[3],zBase,zSlope,start,length;} CollisionBorder;
typedef struct {int triCount,borderCount;CollisionTriangle *tris;CollisionBorder *borders;} CollisionPartition;
typedef struct {int childCount;union {int firstChildIndex,partitionIndex;}u;float origin[3],halfSize[3];} CollisionAabbTree;
typedef struct {struct {float start[3],end[3];}extents;float delta[3],deltaLen,deltaLenSq,radius,offsetZ;int isPoint;struct {struct {ushort *partitions;int global;}checkcount;}threadInfo;} traceWork_t;
static CollisionBorder border;
static CollisionPartition partition;
static CollisionAabbTree tree;
static struct {CollisionAabbTree *aabbTrees;CollisionPartition *partitions;} cm={&tree,&partition};
#define cm_ptr (&cm)
static int CM_CullBox(const traceWork_t *tw,const float *o,const float *s) {return 0;}
static void CM_TraceCapsuleThroughTriangle(const traceWork_t *tw,const CollisionTriangle *t,trace_t *r,float h) {assert(0);}
'''
body = function('CM_BorderCornerNormal') + function('CM_TraceThroughAabbTree_r')
checks = r'''
static trace_t sweep(float x,float y,float z,float ex,float ey,float ez,float rotation,float slope) {
 float a=rotation*.01745329252f,c=cosf(a),s=sinf(a);
 border=(CollisionBorder){.distEq={c,s,0},.length=100,.zBase=12,.zSlope=slope};
 partition=(CollisionPartition){.borderCount=1,.borders=&border};tree=(CollisionAabbTree){0};
 ushort count=0;traceWork_t tw={.radius=10,.offsetZ=35,.threadInfo={.checkcount={&count,1}}};
 tw.extents.start[0]=x*c-y*s;tw.extents.start[1]=x*s+y*c;tw.extents.start[2]=z;
 tw.extents.end[0]=ex*c-ey*s;tw.extents.end[1]=ex*s+ey*c;tw.extents.end[2]=ez;
 for(int i=0;i<3;i++){tw.delta[i]=tw.extents.end[i]-tw.extents.start[i];tw.deltaLenSq+=tw.delta[i]*tw.delta[i];}
 tw.deltaLen=sqrtf(tw.deltaLenSq);
 trace_t t={.fraction=1};CM_TraceThroughAabbTree_r(&tw,&tree,&t);
 assert(isfinite(t.fraction)&&t.fraction>=0&&t.fraction<=1);
 if(t.fraction<1)assert(fabsf(t.normal[0]*t.normal[0]+t.normal[1]*t.normal[1]-1)<.0001f);
 /* Undo rotation for a consistent expected normal. */
 float nx=t.normal[0]*c+t.normal[1]*s,ny=-t.normal[0]*s+t.normal[1]*c;
 t.normal[0]=nx;t.normal[1]=ny;return t;
}
static void near(float a,float b){assert(fabsf(a-b)<.0001f);}
int main(void) {
 int cases=0;
 float hitX=sqrtf(10.125f*10.125f-25),fraction=(20-hitX)/40;
 for(int rotation=0;rotation<360;rotation+=15)for(int side=0;side<2;side++)
 for(int slope=-1;slope<=1;slope++)for(int rise=-1;rise<=1;rise++) {
  float y=side?-105:5,z=12+(side?100*.2f*slope:0);
  trace_t t=sweep(20,y,z,-20,y,z+rise*30,rotation,.2f*slope);
  near(t.fraction,fraction);near(t.normal[0],hitX/10.125f);
  near(t.normal[1],(side?-5:5)/10.125f);assert(!t.startsolid);
  /* With this actual radial normal, projecting a movement toward the
   * endpoint retains tangential velocity around the corner. */
  float vx=-190,vy=0,d=vx*t.normal[0]+vy*t.normal[1];
  vx-=d*t.normal[0];vy-=d*t.normal[1];
  assert(vx<0&&fabsf(vy)>70);
  t=sweep(3,y,z,-20,y,z,rotation,.2f*slope);
  near(t.fraction,0);near(t.normal[0],3/sqrtf(34));
  near(t.normal[1],(side?-5:5)/sqrtf(34));assert(t.startsolid);
  t=sweep(20,side?-111:11,z,-20,side?-111:11,z,rotation,.2f*slope);near(t.fraction,1);
  t=sweep(20,y,z,40,y,z,rotation,.2f*slope);near(t.fraction,1);
  /* Below/above the border's vertical span must stay clear. */
  t=sweep(20,y,z+100,-20,y,z+100,rotation,.2f*slope);near(t.fraction,1);
  cases+=5;
 }
 for(int rotation=0;rotation<360;rotation+=15) {
  trace_t t=sweep(20,-50,12,-20,-50,12,rotation,0);
  near(t.fraction,(20-10.125f)/40);near(t.normal[0],1);near(t.normal[1],0);
  t=sweep(5,-50,12,-15,-50,12,rotation,0);near(t.fraction,0);assert(t.startsolid);
  cases+=2;
 }
 printf("PASS: %d border cases, both endpoints, 24 rotations, vertical motion/slopes, radial slide normals and nonnegative overlap fractions\n",cases);
}
'''
variants = [body,
            body.replace('dirX * cX + dirY * cY', 'dirX * cY + dirY * cX'),
            body.replace('float a = dirX * dirX + dirY * dirY;', 'float a = tw->deltaLenSq;'),
            re.sub(r'CM_BorderCornerNormal\(trace, cX.*?bNormX, bNormY\);',
                   'trace->normal[0]=bNormX;trace->normal[1]=bNormY;trace->normal[2]=0;', body, flags=re.S),
            body.replace('trace->fraction = frac_clamped;', 'trace->fraction = enterFrac;')]
with tempfile.TemporaryDirectory(prefix='cod2-mesh-borders-') as directory:
    d = Path(directory)
    for index, variant in enumerate(variants):
        (d / 'test.c').write_text(support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(d / 'test.c'), '-o', str(d / 'test'), '-lm'], check=True)
        result = subprocess.run([str(d / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr)
        if index == 0:
            print(result.stdout.strip())
print('PASS: swapped approach, 3D cylinder quadratic, flat corner normals and negative-fraction mutants fail')
