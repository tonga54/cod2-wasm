#!/usr/bin/env python3
"""Exercise the real coarse collision-box rejection with shortened traces."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parent.parent
body=(root/'src/PC/qcommon/cm_tracebox.c').read_text()
body='\n'.join(line for line in body.splitlines() if not line.startswith('#include'))
support=r'''
#include <assert.h>
#include <math.h>
typedef float vec_t;
typedef int qboolean;
typedef struct { float start[3],end[3],invDelta[3]; } TraceExtents;
'''
checks=r'''
int main(void) {
 float lo[3]={1282,2797,14.7f},hi[3]={1410,2925,118.7f};
 TraceExtents shot={{1189,2851,105.8f},{9329,3392.3f,-641.2f}};
 CM_CalcTraceEntents(&shot);assert(!CM_TraceBox(&shot,lo,hi,.05777f));
 assert(CM_TraceBox(&shot,lo,hi,.001f));
 for(int axis=0;axis<3;axis++)for(int dir=-1;dir<=1;dir+=2)for(int n=0;n<100;n++) {
  float min[3],max[3];TraceExtents ray;
  for(int a=0;a<3;a++){min[a]=n*11.5f+a*17-64;max[a]=min[a]+128;ray.start[a]=ray.end[a]=min[a]+64;}
  ray.start[axis]-=dir*200;ray.end[axis]+=dir*10000;
  CM_CalcTraceEntents(&ray);
  assert(!CM_TraceBox(&ray,min,max,1));
  assert(!CM_TraceBox(&ray,min,max,.05f));
  assert(CM_TraceBox(&ray,min,max,.001f));
  int cross=(axis+1)%3;ray.start[cross]=ray.end[cross]=max[cross]+1;
  CM_CalcTraceEntents(&ray);assert(CM_TraceBox(&ray,min,max,1));
  ray.start[cross]=ray.end[cross]=max[cross];CM_CalcTraceEntents(&ray);
  assert(!CM_TraceBox(&ray,min,max,1));
 }
 float min[3]={-1,-1,-1},max[3]={1,1,1};
 TraceExtents diag={{-2,-2,-2},{2,2,2}};CM_CalcTraceEntents(&diag);
 assert(!CM_TraceBox(&diag,min,max,.5));assert(CM_TraceBox(&diag,min,max,.2));
 TraceExtents still={{0,0,0},{0,0,0}};CM_CalcTraceEntents(&still);
 assert(!CM_TraceBox(&still,min,max,0));
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-trace-box-') as directory:
    path=Path(directory)
    old=body.replace('return 0;\n}', 'return (leave == 1.0f) ? 0 : 1;\n}')
    for index,variant in enumerate((body,old)):
        (path/'test.c').write_text(support+variant+checks)
        subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(path/'test.c'),'-o',str(path/'test')],check=True)
        result=subprocess.run([str(path/'test')],capture_output=True)
        assert (result.returncode==0)==(index==0),result.stderr.decode()
print('PASS: captured bullet, 600 translated bidirectional rays, parallel/tangent/shortened traces; old rejection rule fails')
