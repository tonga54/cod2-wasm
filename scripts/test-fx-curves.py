#!/usr/bin/env python3
"""Check the original EFX scalar/RGB keys and scale ranges with the real parser."""
from pathlib import Path
import json,re,subprocess,tempfile,zipfile
root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/EffectsCore/FxTemplate.c').read_text()
def fun(s,name):
 m=re.search(r'[^\n;]*\b'+name+r'\([^;]*?\)\n\{',s);assert m,name
 e=m.end();d=1
 while d:d+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[m.start():e]+'\n'
pre=r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <math.h>
typedef int Bool,FxChannelId;
enum {FXCHAN_COLOR,FXCHAN_COLOR_RAND,FXCHAN_COUNT=24};
typedef struct {float mMin,mMax;} FxRange;
typedef struct {int dimensionCount,keyCount;float keys[];} FxCurve;
typedef struct {const FxCurve *curve;FxRange scaleRange;} FxChannel;
typedef struct {FxChannel mFxChannels[24];} PrimitiveTemplate;
typedef struct GPValue {const char *text;struct GPValue *next,*list;const char *value;} GPValue;
typedef struct {GPValue *pairs;} GPGroup;
typedef struct {FxRange start[3],end[3],parm;int flags,containsData;} FxChannelBackwardCompatible;
typedef struct {FxChannelBackwardCompatible fxChannels[24];} BackCompatibleParameters;
#define GPV_STRING(p) ((p)->text)
#define GPV_NEXT(p) ((p)->next)
#define GPV_LIST(p) ((p)->list)
#define GPG_PAIRS(p) ((p)->pairs)
#define stricmp strcasecmp
static const char *GPValue_GetTopValue(GPValue*p){return p->value;}
static void *Hunk_AllocateTempMemoryInternal(int n){return malloc(n);}
static void *Hunk_AllocAlignInternal(int n,int align){return malloc(n);}
static void Hunk_FreeTempMemory(void*p){free(p);}
'''
functions=fun((root/'src/PC/EffectsCore/FxCurve_load_obj.c').read_text(),'FxCurve_AllocAndCreateWithKeys')
for n in ['EnsureMinMax','ParseFloatRange','ParseVec3Range','PrimitiveTemplate_ParseChannelCurve','PrimitiveTemplate_ChannelDimensions','PrimitiveTemplate_ParseChannelRanges','PrimitiveTemplate_CopyChannelRanges','PrimitiveTemplate_ParseChannelFlags','PrimitiveTemplate_ParseChannel']:
 functions+=fun(source,n)
checks=r'''
static void check(int dim,int count,const char **lines,const float *expected,const char *scale,float min,float max){
 GPValue *values=calloc(count,sizeof(*values));
 for(int i=0;i<count;i++){values[i].text=lines[i];if(i+1<count)values[i].next=&values[i+1];}
 GPValue sp={.text="scale",.value=scale};GPValue curve={.text="curve",.list=values,.next=scale?&sp:NULL};GPGroup group={&curve};
 PrimitiveTemplate p={0};BackCompatibleParameters back={0};int ch=dim==3?0:4;
 p.mFxChannels[ch].scaleRange=(FxRange){1,1};
 assert(PrimitiveTemplate_ParseChannel(&p,&back,&group,ch,0,0,0,0,0,0,0,0));
 const FxCurve*c=p.mFxChannels[ch].curve;assert(c&&c->dimensionCount==dim);
 int stride=dim+1,first=expected[0]!=0,last=expected[(count-1)*stride]!=1;
 assert(c->keyCount==count+first+last);
 for(int i=0;i<count*stride;i++)assert(fabsf(c->keys[first*stride+i]-expected[i])<.00001f);
 assert(p.mFxChannels[ch].scaleRange.mMin==min&&p.mFxChannels[ch].scaleRange.mMax==max);
 if(last)for(int i=1;i<=dim;i++)assert(c->keys[(c->keyCount-1)*stride+i]==expected[(count-1)*stride+i]);
 free((void*)c);free(values);
}
int main(void){
'''
num=0
z=zipfile.ZipFile(root/'data/browser/main/cod2_browser_renderer.iwd')
for name in z.namelist():
 if not name.endswith('.efx'):continue
 s=z.read(name).decode('latin1')
 for m in re.finditer(r'(\w+)\s*\{\s*curve\s*\[([^]]+)\]\s*(?:scale\s+([^}\r\n]+))?\s*\}',s):
  channel,rows,scale=m.groups();dim=3 if channel.lower() in ('rgb','rgbrand') else 1
  lines=[x.strip() for x in rows.splitlines() if x.strip()];vals=[]
  try:
   for row in lines: vals+=list(map(float,row.split()))[:dim+1]
   scales=list(map(float,scale.split())) if scale else [1,1]
  except ValueError:continue
  if len(scales)==1:scales*=2
  if not lines:continue
  checks+='check(%d,%d,(const char*[]){%s},(const float[]){%s},%s,%s,%s);\n'%(dim,len(lines),','.join(json.dumps(x) for x in lines),','.join(format(v,'.9g')+'f' if '.' in format(v,'.9g') or 'e' in format(v,'.9g') else str(int(v))+'.0f' for v in vals),json.dumps(scale) if scale else 'NULL',min(scales),max(scales))
  num+=1
checks+=f'puts("PASS: {num} original FX curves, RGB dimensions, scalar scales and endpoint allocation");}}\n'
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'curves.c';p.write_text(pre+functions+checks)
 subprocess.run(['cc','-std=c11','-fsanitize=address,undefined','-g',str(p),'-lm','-o',str(Path(d)/'test')],check=True)
 subprocess.run([str(Path(d)/'test')],check=True)
