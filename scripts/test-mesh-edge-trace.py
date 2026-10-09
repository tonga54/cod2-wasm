#!/usr/bin/env python3
"""Exercise the actual mesh sweep against finite edges and their end points."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/qcommon/cm_mesh.c').read_text()
begin = source.index('static void CM_MESH_REGPARM3_ABI CM_PositionTestCapsuleInTriangle(', source.index('static Bool CM_MESH_REGPARM3_ABI CM_CullBox(', source.index('short int CM_MeshTestInLeaf(')))
end = source.index('\nstatic void CM_MESH_REGPARM3_ABI CM_TraceThroughAabbTree_r', begin)
body = source[begin:end]
support = r'''
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#define CM_MESH_REGPARM3_SSE_ABI
#define CM_MESH_REGPARM3_ABI
typedef unsigned char byte;
typedef struct { float origin[3], axis[3][3]; } CollisionEdge;
typedef struct { float xyz[3]; } CollisionVertex;
typedef struct { float plane[4], svec[4], tvec[4]; int verts[3], edges[3]; } CollisionTriangle;
typedef struct { float fraction, normal[3]; int startsolid, allsolid; } trace_t;
typedef struct {
    struct { float start[3], end[3]; } extents;
    float radius, delta[3], deltaLenSq, offsetZ;
    struct { struct { int *verts, *edges, global; } checkcount; } threadInfo;
} traceWork_t;
static CollisionEdge edge;
static CollisionVertex vertices[2];
typedef struct { CollisionEdge *edges; CollisionVertex *verts; } clipMap_t;
static clipMap_t cm = {&edge, vertices};
#define cm_ptr (&cm)
static float Vec3Normalize(float *v) {
    float len = sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
    if (len) for (int i=0;i<3;i++) v[i]/=len;
    return len;
}
'''
checks = r'''
static int stationary;
static trace_t sweep(float x, float y, float z, float ex, float ey, float ez, int rotate) {
    int vertexChecks[2]={0}, edgeChecks[1]={0};
    traceWork_t tw={.radius=10, .extents={{x,y,z},{ex,ey,ez}},
        .threadInfo={.checkcount={vertexChecks,edgeChecks,1}}};
    CollisionTriangle tri={.plane={0,0,1,0}, .svec={.01f,0,0,0}, .tvec={0,.01f,0,0},
        .verts={0,1,-1}, .edges={-1,-1,0}};
    edge=(CollisionEdge){.axis={{0,1,0},{0,0,1},{.01f,0,0}}};
    vertices[0]=(CollisionVertex){{0,0,0}};
    vertices[1]=(CollisionVertex){{100,0,0}};
    if (rotate) {
        /* Rigidly rotate the complete fixture around Z. */
        for(int i=0;i<2;i++) {
            float *v=i ? tw.extents.end : tw.extents.start;
            float t=v[0];v[0]=-v[1];v[1]=t;
        }
        for(int i=0;i<3;i++) {
            float *v=edge.axis[i];float t=v[0];v[0]=-v[1];v[1]=t;
        }
        tri.svec[0]=0;tri.svec[1]=.01f;
        tri.tvec[0]=-.01f;tri.tvec[1]=0;
        vertices[1].xyz[0]=0;vertices[1].xyz[1]=100;
    }
    for(int i=0;i<3;i++) {
        tw.delta[i]=tw.extents.end[i]-tw.extents.start[i];
        tw.deltaLenSq+=tw.delta[i]*tw.delta[i];
    }
    trace_t result={.fraction=1};
    if(stationary) CM_PositionTestCapsuleInTriangle(&tw,&tri,&result);
    else CM_TraceCapsuleThroughTriangle(&tw,&tri,&result,0);
    return result;
}
static void near(float actual,float expected) { assert(fabsf(actual-expected)<.0001f); }
int main(void) {
    for(int rotation=0;rotation<2;rotation++) {
        for(int i=1;i<10;i++) {
            float x=10.0f*i;
            trace_t t=sweep(x,-5,20,x,-5,-20,rotation);
            near(t.fraction,(20-sqrtf(10.125f*10.125f-25))/40);
            near(t.normal[0]*t.normal[0]+t.normal[1]*t.normal[1]+t.normal[2]*t.normal[2],1);
            assert(!t.startsolid);
            t=sweep(x,-5,5,x,-5,-20,rotation);
            near(t.fraction,0);assert(t.startsolid);
            t=sweep(x,-15,20,x,-15,-20,rotation);
            near(t.fraction,1);assert(!t.startsolid);
            t=sweep(x,-5,5,x,-5,20,rotation);
            near(t.fraction,1);
        }
        /* End points, face interior, and a miss past the finite edge. */
        trace_t t=sweep(-5,-5,20,-5,-5,-20,rotation);
        near(t.fraction,(20-sqrtf(10.125f*10.125f-50))/40);
        t=sweep(105,-5,20,105,-5,-20,rotation);
        near(t.fraction,(20-sqrtf(10.125f*10.125f-50))/40);
        t=sweep(120,-5,20,120,-5,-20,rotation);near(t.fraction,1);
        t=sweep(20,20,20,20,20,-20,rotation);near(t.fraction,(20-10.125f)/40);
        stationary=1;
        t=sweep(50,-5,5,50,-5,5,rotation);assert(t.startsolid && t.allsolid);
        t=sweep(50,-11,0,50,-11,0,rotation);assert(!t.startsolid && !t.allsolid);
        t=sweep(120,-5,0,120,-5,0,rotation);assert(!t.startsolid && !t.allsolid);
        stationary=0;
    }
    return 0;
}
'''
mutants = [
    body,
    body.replace('float approach = projA * dirA + projB * dirB;',
                 'float approach = projA * dirB - projB * dirA;'),
    body.replace('edge->axis[2]', 'edge->axis[0]'),
]
with tempfile.TemporaryDirectory(prefix='cod2-mesh-edges-') as directory:
    path=Path(directory)
    for index, variant in enumerate(mutants):
        (path/'test.c').write_text(support+variant+checks)
        subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',
                        str(path/'test.c'),'-o',str(path/'test'),'-lm'],check=True)
        result=subprocess.run([str(path/'test')],capture_output=True,text=True)
        assert (result.returncode==0)==(index==0), (index,result.stderr)
print('PASS: 86 edge/vertex/face sweep and overlap cases including rotation; wrong-axis and cross-product mutants fail')
