#!/usr/bin/env python3
"""Check the actual flat collision-leaf loader without private map files."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/qcommon/cm_load_obj.c').read_text()
match = re.search(r'^static cLeafBrushNode_t \*CMod_PartionLeafBrushes_r\([^;]+?\)\n\{', source, re.M)
assert match
end, depth = match.end(), 1
while depth:
    depth += (source[end] == '{') - (source[end] == '}')
    end += 1
code = r'''
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef float vec_t;
typedef struct {int contents;} cbrush_t;
typedef struct {
 unsigned char axis; short leafBrushCount; int contents;
 union {struct {unsigned short *brushes;} leaf;
        struct {float dist,range;unsigned short childOffset[2];} children;} data;
} cLeafBrushNode_t;
static struct {cbrush_t *brushes;} map;
#define cm_ptr (&map)
#define ERR_DROP 1
static int allocations;
static void *allocated;
static jmp_buf drop;
static void *TempMalloc(size_t size) {
 assert(size==sizeof(cLeafBrushNode_t));allocations++;
 allocated=malloc(size);assert(allocated);memset(allocated,0xcd,size);return allocated;
}
static void Com_Error(int code,const char *message) {
 assert(code==ERR_DROP);assert(strstr(message,"overflows a short"));longjmp(drop,1);
}
''' + source[match.start():end] + r'''
int main(void) {
 cbrush_t *brushes=calloc(65536,sizeof(*brushes));
 unsigned short *indexes=malloc(32768*sizeof(*indexes)),*copy=malloc(32768*sizeof(*indexes));
 assert(brushes&&indexes&&copy);map.brushes=brushes;
 uint32_t seed=1234567;
 const int counts[]={0,1,2,17,255,1024,32767};
 for(int round=0;round<20;round++)for(unsigned test=0;test<sizeof(counts)/sizeof(*counts);test++) {
  int count=counts[test],contents=0;
  for(int n=0;n<count;n++) {
   seed=seed*1664525u+1013904223u;indexes[n]=seed>>16;
   brushes[indexes[n]].contents=1u<<(n%24);
  }
  for(int n=0;n<count;n++)contents|=brushes[indexes[n]].contents;
  memcpy(copy,indexes,count*sizeof(*copy));allocations=0;
  cLeafBrushNode_t *node=CMod_PartionLeafBrushes_r(indexes,count,NULL,NULL);
  assert(allocations==1&&node->axis==0&&node->leafBrushCount==count);
  assert(node->contents==contents&&node->data.leaf.brushes==indexes);
  assert(!memcmp(copy,indexes,count*sizeof(*copy)));
  assert(node->data.children.childOffset[0]==0&&node->data.children.childOffset[1]==0);
  free(allocated);
 }
 if(!setjmp(drop)) {
  CMod_PartionLeafBrushes_r(indexes,32768,NULL,NULL);assert(!"Expected an overflow error");
 }
 free(allocated);free(brushes);free(indexes);free(copy);
 puts("PASS: collision leaf order, pointers, contents, empty/max-sized leaves and short overflow retain their original behavior");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-collision-loading-') as directory:
    path = Path(directory)
    (path / 'test.c').write_text(code)
    subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                    str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
