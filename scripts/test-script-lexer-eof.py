#!/usr/bin/env python3
"""Run the real GSC lexer through EOF, including the HQ/SD objective script."""
from pathlib import Path
import os
import subprocess
import tempfile
import zipfile
root=Path(__file__).resolve().parent.parent
lexer=(root/'src/PC/script/yyparse_impl.h').read_text().split('\nint yyparse(void)\n',1)[0]
yacc=(root/'src/PC/script/scr_yacc.c').read_text()
tables=yacc[yacc.index('const unsigned char yydefgoto[64]'):]
main=(root/'src/PC/script/scr_main.c').read_text()
scan=main.split('int Scr_ScanFile(char *buf, int max_size)\n',1)[1].split('\nunsigned int Scr_LoadScript',1)[0]
code=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef union {int type,intValue;unsigned int pos;float floatValue;intptr_t node;} sval_t;
typedef struct {sval_t val;unsigned int pos;} stype_t;
struct yy_buffer_state {FILE *yy_input_file;char *yy_ch_buf,*yy_buf_pos;int yy_buf_size,yy_n_chars,yy_is_our_buffer,yy_is_interactive,yy_at_bol,yy_fill_buffer,yy_buffer_status;};
static stype_t yylval;static unsigned int g_out_pos,g_sourcePos;static unsigned char g_parse_user;
static FILE *yyin,*yyout;static struct yy_buffer_state *yy_current_buffer;
static int yy_start,yy_n_chars,yy_init,yy_did_buffer_switch_on_eof,yyleng,yy_last_accepting_state;
static char *yy_c_buf_p,*yytext,*yy_last_accepting_cpos,yy_hold_char;
struct scrCompilePub_t {const char *in_ptr,*parseBuf;};
static struct scrCompilePub_t input;void *imp_scrCompilePub=&input;
static char ch_buf[16386];
static struct yy_buffer_state *yy_create_buffer(FILE *file,int size){(void)file;(void)size;abort();}
void CompileError(unsigned int pos,const char *fmt,...){fprintf(stderr,"Lexer error at %u: %s\n",pos,fmt);exit(2);}
unsigned int SL_GetString_(const char *s,unsigned int user,int type){(void)s;(void)user;(void)type;return 1;}
unsigned int SL_GetStringOfLen(const char *s,unsigned int user,unsigned int len,int type){(void)s;(void)user;(void)len;(void)type;return 1;}
int Scr_ScanFile(char *buf,int max_size)
'''+scan+'\n'+tables+'\n'+lexer+r'''
static void run(const char *source,int *tokens,int *count){
 struct yy_buffer_state b={0};b.yy_buf_size=16384;b.yy_ch_buf=ch_buf;b.yy_buf_pos=ch_buf;b.yy_at_bol=1;b.yy_fill_buffer=1;
 ch_buf[0]=ch_buf[1]=0;yy_current_buffer=&b;yy_start=3;yy_init=1;yy_hold_char=0;
 yy_last_accepting_state=0;yy_last_accepting_cpos=ch_buf;yyout=stdout;g_out_pos=0xffffffff;
 input.in_ptr="+";input.parseBuf=source;*count=0;
 while(1){int token=yyparse_yylex(ch_buf+16384); /* use a separate string scratch below */
  if(!token)break;assert(*count<99999);tokens[(*count)++]=token;
 }
}
int main(int argc,char **argv){
 assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);
 char *source=calloc(n+2,1);assert(fread(source,1,n,f)==(size_t)n);fclose(f);
 static int first[100000],second[100000];int a,b;
 run(source,first,&a);source[n]='\n';run(source,second,&b);
 assert(a==b && !memcmp(first,second,a*sizeof(int)));free(source);return 0;
}
'''
# Quoted string unescaping requires independent scratch storage, like yyparse.
code=code.replace('while(1){int token=yyparse_yylex(ch_buf+16384); /* use a separate string scratch below */', 'char strings[8192];while(1){int token=yyparse_yylex(strings);')
with tempfile.TemporaryDirectory(prefix='cod2-lexer-eof-') as directory:
 p=Path(directory);(p/'test.c').write_text(code)
 subprocess.run(['cc','-std=c99','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 sources=[b'main() {}',b'main() {} // trailing comment',b'main() { text = "// quoted"; } // end',b'final_identifier',b'main(){ value = .61; } // end']
 data_root=Path(os.environ.get('COD2_WEB_DATA_DIR',root/'data/browser'))
 with zipfile.ZipFile(data_root/'main/cod2_browser_renderer.iwd') as z:
  sources += [z.read(name) for name in z.namelist() if name.endswith('.gsc')]
 for i,source in enumerate(sources):
  (p/'input.gsc').write_bytes(source)
  subprocess.run([str(p/'test'),str(p/'input.gsc')],check=True,timeout=3)
print(f'PASS: real GSC lexer terminates and preserves all tokens with/without a final newline in {len(sources)} scripts/cases')
