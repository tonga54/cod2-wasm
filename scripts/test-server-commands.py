#!/usr/bin/env python3
"""Verify actual client dispatch against the dedicated server's command letters."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/client_mp/cl_cgame_mp.c').read_text()
a=source.index('qboolean CL_GetServerCommand(int serverCommandNumber)\n{')
body=source[a:source.index('\nvoid CL_SetExpectedHunkUsage',a)]
support=r'''
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
typedef int qboolean;
typedef struct {int serverCommandSequence,demoplaying,lastExecutedServerCommand;char serverCommands[128][1024];} clientConnection_t;
typedef struct {struct {int enabled;} current;} dvar_t;
static clientConnection_t cluiStorage;
#define CLUI_STATE (&cluiStorage)
static struct {int cmds[8];} local;
#define CL_LOCAL (&local)
static dvar_t show; static const dvar_t *showPtr=&show; static void *imp_cl_showServerCommands=&showPtr;
static int notify, subtitles, flares, modified;
static void ClearFlares(void){flares++;}
static struct {void (*ClearFlares)(void);} renderer={ClearFlares};
#define RE (&renderer)
#define ERR_DROP 1
#define ERR_SERVERDISCONNECT 2
static char bigConfigString[8192],tokenBuf[8192],*args[16],config[8192];static int argc,configIndex;
static jmp_buf errors;static int lastError;
static void Com_Error(int kind,const char *fmt,...){lastError=kind;longjmp(errors,1);}
static void Com_Printf(const char *fmt,...){}
static void Com_DPrintf(const char *fmt,...){}
#define Com_sprintf snprintf
static const char *SEH_SafeTranslateString(const char *s){return s;}
static const char *UI_ReplaceConversionString(const char *a,const char *b){return b;}
static void tokenize(const char *s,int limit){
    strcpy(tokenBuf,s);argc=0;char *p=tokenBuf;
    while(*p && argc<16){while(*p==' ')p++;if(!*p)break;args[argc++]=p;if(argc==limit)break;while(*p && *p!=' ')p++;if(*p)*p++=0;}
}
static void Cmd_TokenizeString(const char *s){tokenize(s,16);}
static void Cmd_TokenizeString2(const char *s,int limit){tokenize(s,limit);}
static const char *Cmd_Argv(int i){return i<argc?args[i]:"";}
static int Cmd_Argc(void){return argc;}
static void CL_ConfigstringModified(void){modified++;configIndex=atoi(Cmd_Argv(1));strcpy(config,Cmd_Argv(2));}
static void Con_ClearNotify(void){notify++;}
static void Con_ClearSubtitles(void){subtitles++;}
'''
checks=r'''
static int dispatch(const char *s){int seq=++cluiStorage.serverCommandSequence;strcpy(cluiStorage.serverCommands[seq&127],s);return CL_GetServerCommand(seq);}
int main(void){
 if(setjmp(errors))return 1;
 assert(dispatch("d 595 round is over")==1);assert(modified==1&&configIndex==595&&!strcmp(config,"round is over"));
 assert(dispatch("b 2 1 0 5 0")==1);assert(dispatch("c message")==1);assert(dispatch("C 3")==1);assert(modified==1);
 assert(dispatch("x 595 first part ")==0);assert(dispatch("y 595 middle ")==0);assert(dispatch("z 595 last")==1);
 assert(modified==2&&configIndex==595&&!strcmp(config,"first part middle last"));assert(!notify);
 for(int r=0;r<2;r++){for(int i=0;i<8;i++)local.cmds[i]=9;assert(dispatch(r?"n":"B")==1);for(int i=0;i<8;i++)assert(local.cmds[i]==0);}
 assert(notify==2&&subtitles==2&&flares==2);
 if(!setjmp(errors)){dispatch("w EXE_PLAYERKICKED");return 2;}assert(lastError==ERR_SERVERDISCONNECT);
 if(!setjmp(errors)){dispatch("w");return 3;}assert(lastError==ERR_SERVERDISCONNECT);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-commands-') as directory:
 p=Path(directory)
 for mutant in (False,True):
  altered=body.replace("case 'w':","case 'd':\n    case 'w':").replace("    case 'd':\n        Cmd_TokenizeString2", "    case 'C':\n        Cmd_TokenizeString2") if mutant else body
  (p/'test.c').write_text(support+altered+checks)
  subprocess.run(['cc','-std=c99',str(p/'test.c'),'-o',str(p/'test')],check=True)
  result=subprocess.run([str(p/'test')],capture_output=True)
  assert (result.returncode==0)!=mutant,result.stderr
print('PASS: configstrings/fragments, scores, announcements, weapon commands, restart and real disconnect; old dispatch fails')
