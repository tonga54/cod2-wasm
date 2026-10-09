#!/usr/bin/env python3
"""Check the actual voice HUD branch without invoking the renderer."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/ui_mp/ui_main_mp.c').read_text()
start = source.index('    {', source.index('    case 270:'))
end = source.index('\n    default:', start)
branch = source[start:end]
support = r'''
#include <assert.h>
typedef int MaterialHandle;
typedef struct { struct { int enabled; } current; } dvar_t;
static dvar_t server = {{1}}, client = {{1}}, *serverPtr=&server,*clientPtr=&client;
static void *imp_sv_voice=&serverPtr,*imp_cl_voice=&clientPtr;
static struct { int playerClientNums[64]; char playerNames[64][32]; } sharedUiInfo;
static int ownerDraw, talker=-1, pictures, texts;
static float rect[6],color[4],scale=1;
static int font,textStyle;
static int CL_IsPlayerTalking(int index) { return index==talker; }
static void UI_BuildPlayerList(void) {}
static int CL_RegisterMaterialNoMip(const char *s,int flags) { return 1; }
static float CL_NormalizedTextScale(int font,float scale) { return scale; }
static int CL_TextHeight(int font) { return 12; }
static void UI_DrawHandlePic(float x,float y,float w,float h,int a,int b,float *color,int material) { pictures++; }
static void UI_DrawText(const char *s,int n,int font,float x,float y,int a,int b,float scale,float *color,int style) {
    assert(s==sharedUiInfo.playerNames[0]); texts++;
}
static void draw(void)
'''
checks = r'''
int main(void) {
    for(int i=0;i<64;i++) sharedUiInfo.playerClientNums[i]=-1;
    for(ownerDraw=265;ownerDraw<=270;ownerDraw++) draw();
    assert(pictures==0 && texts==0);
    talker=2;
    for(ownerDraw=265;ownerDraw<=270;ownerDraw++) draw();
    assert(pictures==0 && texts==0);
    sharedUiInfo.playerClientNums[0]=2;
    for(ownerDraw=265;ownerDraw<=270;ownerDraw++) draw();
    assert(pictures==1 && texts==1);
    server.current.enabled=0;
    for(ownerDraw=265;ownerDraw<=270;ownerDraw++) draw();
    server.current.enabled=1;client.current.enabled=0;
    for(ownerDraw=265;ownerDraw<=270;ownerDraw++) draw();
    assert(pictures==1 && texts==1);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-voice-hud-') as directory:
    path = Path(directory)
    (path / 'test.c').write_text(support + branch + checks)
    subprocess.run(['cc', '-std=c99', '-fsanitize=address', str(path / 'test.c'),
                    '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('PASS: no phantom talkers, no missing-player access, valid talker draws, disabled voice stays hidden')
