#!/usr/bin/env python3
"""Exercise the production chat ring/layout and browser chat state under ASan/UBSan."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent


def extract(source, signature):
    start = re.search(re.escape(signature) + r'\s*\{', source).start()
    cursor = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[cursor] == '{') - (source[cursor] == '}')
        cursor += 1
    return source[start:cursor]


add = extract((root / 'src/PC/cgame_mp/cg_servercmds_mp.c').read_text(),
              'static void __attribute_regparm__(1) CG_AddToTeamChat(const char *str)')
draw = extract((root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text(),
               'unsigned int CG_DrawChatMessages(void)')
platform = (root / 'downstream/wasm/web_platform.c').read_text()
state = extract(platform, 'EMSCRIPTEN_KEEPALIVE int web_client_state(void)')
lost = extract(platform, 'EMSCRIPTEN_KEEPALIVE void web_capture_lost(void)')
character = extract(platform, 'EMSCRIPTEN_KEEPALIVE void web_chat_char(int character)')
support = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define __attribute_regparm__(n)
#define EMSCRIPTEN_KEEPALIVE
typedef void *FontHandle;
typedef float vec4_t[4];
typedef struct { struct { int integer; float vector[2]; } current; } dvar_t;
static dvar_t height, lifetime, position;
static const dvar_t *cg_chatHeight=&height, *cg_chatTime=&lifetime, *cg_hudChatPosition=&position;
static struct { int time; } cg_storage, *cg=&cg_storage;
static struct { uint64_t first;
    struct { char teamChatMsgs[8][271]; int teamChatMsgTimes[8],teamChatPos,teamLastChatPos;
        struct {void *whiteMaterial;} media; } data;
    uint64_t last;
} guarded;
#define cgs (&guarded.data)
static int texts,pictures;
static float lastBaseline;
static char rendered[8][271];
static FontHandle UI_GetFontHandle(int kind,float scale){return (void *)1;}
static int UI_TextHeight(FontHandle font,float scale){return (int)(48*scale+.01f);}
static int UI_TextWidth(const char *s,int maxChars,FontHandle font,float scale){
    float width=0;int chars=0;
    while(*s && (!maxChars || chars<maxChars)){
        if(s[0]=='^' && s[1]>='0' && s[1]<='9'){s+=2;continue;}
        width+=(*s=='W'?43:*s=='i'?10:24)*scale; ++s;++chars;
    }return (int)width;
}
static void UI_DrawHandlePic(float x,float y,float w,float h,int ha,int va,const float *color,void *mat){
    assert(w<=COD2_CHAT_WIDTH+6 && w>=6 && h>0 && h<=8*14+4);
    assert(color[3]>0 && color[3]<=.3f);++pictures;
}
static void UI_DrawText(const char *s,int maxChars,FontHandle f,float x,float y,int ha,int va,float scale,const float *color,int style){
    assert(texts<8);assert(scale<=.25f);assert(color[3]>0 && color[3]<=1);
    assert(UI_TextWidth(s,0,f,scale)<=COD2_CHAT_WIDTH);
    if(texts)assert(y-lastBaseline>=UI_TextHeight(f,scale)+4);
    lastBaseline=y;snprintf(rendered[texts++],271,"%s",s);
}
static void reset(void){memset(&guarded,0,sizeof(guarded));guarded.first=guarded.last=UINT64_C(0xdeadc0de1234);
    height.current.integer=5;lifetime.current.integer=8000;cg->time=1000;
    position.current.vector[0]=5;position.current.vector[1]=150;}
static void render(void){texts=pictures=0;CG_DrawChatMessages();
    assert(guarded.first==UINT64_C(0xdeadc0de1234) && guarded.last==guarded.first);}
static void line(const char *s){CG_AddToTeamChat(s);
    assert(guarded.first==UINT64_C(0xdeadc0de1234) && guarded.last==guarded.first);}
static struct{int rendererStarted,uiStarted;}cls;
static struct{int state;}clientConnections[1];
enum{CA_CONNECTING=2,CA_ACTIVE=8,SE_KEY=1,SE_CHAR=2};
static int catchers,events,lastType,lastValue;
static int CL_GetKeyCatchers(void){return catchers;}
static void Sys_QueEvent(int time,int type,int value,int value2,int length,void *ptr){
    ++events;lastType=type;lastValue=value;}
'''
checks = r'''
int main(void){
 reset();
 for(int n=0;n<100;n++){char msg[100];snprintf(msg,sizeof(msg),"^2Player %d^7: Message %d",n%3,n);line(msg);render();assert(texts==(n<5?n+1:5));assert(pictures==1);}
 assert(strstr(rendered[4],"Message 99"));
 height.current.integer=3;render();assert(texts==3 && strstr(rendered[0],"Message 97"));
 height.current.integer=8;line("Last message");render();assert(texts==4 && !strcmp(rendered[3],"Last message"));
 cg->time=8800;render();assert(texts==4);cg->time=9000;render();assert(texts==0 && pictures==0);
 assert(cgs->teamLastChatPos==cgs->teamChatPos);
 reset();char longword[701];memset(longword,'W',700);longword[700]=0;line(longword);render();assert(texts==5);
 reset();line("^1Player: WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW ^2green words WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW");
 render();assert(texts>=2);for(int i=1;i<texts;i++)assert(rendered[i][0]=='^');
 reset();line("First\nSecond\nThird");render();assert(texts==3 && !strcmp(rendered[1],"^7Second"));
 reset();for(int n=0;n<20;n++)line("Concurrent message");height.current.integer=99;render();assert(texts==5);
 height.current.integer=0;render();assert(texts==0);line("disabled");assert(cgs->teamChatPos==0);
 height.current.integer=5;lifetime.current.integer=0;line("disabled");render();assert(texts==0);
 cls.rendererStarted=cls.uiStarted=1;clientConnections[0].state=CA_ACTIVE;
 for(catchers=0;catchers<32;catchers++){
  assert(web_client_state()==(catchers==0?2:catchers==16?3:1));
  events=0;web_capture_lost();assert(events==(catchers==0?2:0));
  for(int ch=0;ch<256;ch++){events=0;web_chat_char(ch);
   assert(events==((catchers==16 && (ch==' ' || ch=='/'))?1:0));
   if(events)assert(lastType==SE_CHAR && lastValue==ch);
  }
 }
 clientConnections[0].state=0;catchers=16;events=0;web_chat_char(' ');assert(events==0 && web_client_state()==1);
 cls.uiStarted=0;assert(web_client_state()==0);
 puts("PASS: multi-player bursts, compact non-overlapping rows, measured wrapping/colors, fixed ring/height changes, expiry, bounded storage and captured chat character/state routing");
}
'''
# The support's render wrapper calls the extracted function defined later.
support = support.replace('static void render(void)', 'unsigned int CG_DrawChatMessages(void);\nstatic void render(void)')
support = support.replace('static void line(const char *s)', 'static void CG_AddToTeamChat(const char *str);\nstatic void line(const char *s)')
code = (root / 'src/headers/cod2_chat.h').read_text() + support + add + draw + state + lost + character + checks
with tempfile.TemporaryDirectory(prefix='cod2-chat-') as directory:
    folder = Path(directory)
    for name, body in [('actual', code),
                       ('overlap', code.replace('lineHeight = textHeight + 4;', 'lineHeight = 10;')),
                       ('ring', code.replace('% COD2_CHAT_ROWS', '% chatHeight')),
                       ('capture', code.replace('if (catchers == 0x10) return 3;', 'if (catchers == 0x10) return 1;'))]:
        (folder / 'test.c').write_text(body)
        subprocess.run(['cc', '-O1', '-fsanitize=address,undefined', str(folder / 'test.c'), '-o', str(folder / 'test')], check=True)
        result = subprocess.run([str(folder / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (name == 'actual'), (name, result.stderr)
        if name == 'actual': print(result.stdout, end='')
print('PASS: overlapping rows, unstable ring indexing and chat-as-menu regressions fail')
