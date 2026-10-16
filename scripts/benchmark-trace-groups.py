#!/usr/bin/env python3
"""Time identical synthetic brush workloads using two production selectors."""
from pathlib import Path
import argparse, runpy, subprocess, tempfile
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('baseline_source',type=Path)
a=p.parse_args()
root=Path(__file__).resolve().parents[1]
t=runpy.run_path(str(root/'scripts/test-trace-pruning.py'),run_name='benchmark')
old=t['function'](a.baseline_source.read_text(),'CM_TraceLeafBrushNode_r').replace('CM_TraceLeafBrushNode_r','previous_selector')
body=r'''
#include <time.h>
static double seconds(void){struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return ts.tv_sec+ts.tv_nsec*1e-9;}
int main(void){
 cbrush_t brushes[1024]={0};unsigned short indices[1024];
 cLeafBrushNode_t node={.contents=1,.leafBrushCount=1024,.data.leaf.brushes=indices};
 cm.brushes=brushes;cm.numBrushes=1024;
 for(int i=0;i<1024;i++){
  indices[i]=i;int x=i%32,y=i/32;
  brushes[i]=(cbrush_t){.contents=1,.mins={x*128.f,y*128.f,0},.maxs={x*128.f+32,y*128.f+32,128}};
 }
 const char *names[]={"short movement","long diagonal","long axial","stationary overlap"};
 traceWork_t queries[4096];
 for(int category=0;category<4;category++){
  seed=7281;
  for(int i=0;i<4096;i++){
   vec3_t start={rnd(4096),rnd(4096),rnd(160)},end,mins={-15,-15,-15},maxs={15,15,55};
   for(int j=0;j<3;j++)end[j]=start[j];
   if(category==0){end[0]+=rnd(32)-16;end[1]+=rnd(32)-16;end[2]+=rnd(16)-8;}
   if(category==1){end[0]=rnd(4096);end[1]=rnd(4096);end[2]=rnd(160);}
   if(category==2)end[0]=rnd(4096);
   if(category==1||category==2)memset(mins,0,sizeof(mins)),memset(maxs,0,sizeof(maxs));
   CM_InitTraceWork(&queries[i],start,end,mins,maxs,1);
   trace_t before={.fraction=1},after=before;byte seen[8192]={0};
   previous_selector(&queries[i],&node,&before,0,seen);memset(seen,0,sizeof(seen));
   CM_TraceLeafBrushNode_r(&queries[i],&node,&after,0,seen);equal(before,after);
  }
  for(int run=0;run<6;run++){
   int after=run==1||run==2||run==5;double begin=seconds(),sum=0;brushTests=0;
   for(int repeat=0;repeat<8;repeat++)for(int i=0;i<4096;i++){
    trace_t hit={.fraction=1};byte seen[8192]={0};
    if(after)CM_TraceLeafBrushNode_r(&queries[i],&node,&hit,0,seen);else previous_selector(&queries[i],&node,&hit,0,seen);
    sum+=hit.fraction;
   }
   printf("%s,%s,%.3f,%lu,%.9f\n",names[category],after?"after":"before",(seconds()-begin)*1000,brushTests,sum);
  }
 }
}
'''
# Reuse the existing independent hit comparator, but not its test main.
helpers=t['checks'][:t['checks'].index('static void *cacheThread')]
source='\n'.join(t[k] for k in ['support','code','clip','plane','axial','bounds','planes','brush','world'])+old+helpers+body
with tempfile.TemporaryDirectory(prefix='cod2-brush-bench-') as d:
 path=Path(d);(path/'bench.c').write_text(source)
 subprocess.run(['cc','-O2','-std=c99','-D_POSIX_C_SOURCE=200809L',str(path/'bench.c'),'-lm','-pthread','-o',str(path/'bench')],check=True)
 subprocess.run([str(path/'bench')],check=True)
