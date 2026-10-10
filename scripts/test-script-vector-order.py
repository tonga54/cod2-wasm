#!/usr/bin/env python3
"""Constant-folded GSC vectors must agree with the real runtime stack path."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
compiler = (root / 'src/PC/script/scr_compiler.c').read_text()
variable = (root / 'src/PC/script/scr_variable.c').read_text()

def extract(source, pattern):
    match = re.search(pattern, source)
    assert match
    end = source.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]

fold = extract(compiler, r'static Bool __attribute_regparm__\(2\)\s+EvalPrimitiveExpression\([^;]*?\)\s*\{')
cast = extract(variable, r'void Scr_CastVector\([^;]*?\)\s*\{')
emitter_start = compiler.rfind('            if (success) {', 0, compiler.index('SCRCP->value_count -= 3;'))
emitter = extract(compiler[emitter_start:], r'if \(success\)\s*\{')
emitter = 'static Bool FoldEmitter(VariableCompileValue *constValue){ int success=1;sval_t node[3]={0};' + emitter + 'return 0;}'
support = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __attribute_regparm__(n)
#define SCRVL_VAR_UNDEFINED 0
#define SCRVL_VAR_VECTOR 4
#define SCRVL_VAR_FLOAT 5
#define SCRVL_VAR_INTEGER 6
#define SCR_VEC_ENC(v) ((void)(v),1u)
typedef int Bool; typedef unsigned char byte;
typedef union {int intValue; unsigned stringValue,vectorValue; float floatValue;} VariableUnion;
typedef struct {VariableUnion u; int type;} VariableValue;
typedef union {intptr_t node; int type,intValue; unsigned stringValue,sourcePosValue; float floatValue;} sval_t;
typedef struct {VariableValue value;sval_t sourcePos;} VariableCompileValue;
static struct {int value_count;}compilePub;
static struct {VariableCompileValue value_start[32];}compileGlob;
#define SCRCP (&compilePub)
#define SCRCG (&compileGlob)
static float allocated[3];
static struct {int error_index;}scrVarPub;
static const char *var_typename[24];
static const void *imp_var_typename=var_typename;
static const float *Scr_AllocVector(const float *v){memcpy(allocated,v,sizeof(allocated));return allocated;}
static const float *Scr_AllocVector_core(const float *v){return Scr_AllocVector(v);}
static void CompileError(unsigned pos,const char *fmt,...){(void)pos;(void)fmt;abort();}
static void Scr_Error(const char *msg){(void)msg;abort();}
static char *va(const char *fmt,...){return (char *)fmt;}
static void RemoveRefToValue_core(int type,VariableUnion value){(void)type;(void)value;}
static Bool EvalBinaryOperatorExpression(sval_t a,sval_t b,sval_t c,sval_t d,VariableCompileValue *v){
 (void)a;(void)b;(void)c;(void)d;(void)v;return 0;
}
'''
checks = r'''
int main(void){
 _Static_assert(sizeof(VariableValue)==8,"runtime stack ABI");
 sval_t primitive[3][3]={0},expression[3][2]={0},wrapper[3][1]={0},list[3][2]={0},holder[1]={0},vector[3]={0};
 VariableCompileValue result;
 // Actual parser order: the last source component is at the list head.
 primitive[0][0].type=8;primitive[0][1].floatValue=1282.54f;
 primitive[1][0].type=7;primitive[1][1].intValue=731;
 primitive[2][0].type=10;primitive[2][1].floatValue=1.99999f;
 for(int i=0;i<3;i++){
  expression[i][0].type=6;expression[i][1].node=(intptr_t)primitive[i];
  wrapper[i][0].node=(intptr_t)expression[i];list[i][0].node=(intptr_t)wrapper[i];
  if(i)list[i][1].node=(intptr_t)list[i-1];
 }
 holder[0].node=(intptr_t)list[2];vector[0].type=46;vector[1].node=(intptr_t)holder;
 assert(EvalPrimitiveExpression((sval_t){.node=(intptr_t)vector},&result));
 assert(result.value.type==4&&allocated[0]==1282.54f&&allocated[1]==731&&allocated[2]==-1.99999f);
 float folded[3];memcpy(folded,allocated,sizeof(folded));
 compilePub.value_count=3;
 compileGlob.value_start[0].value=(VariableValue){{.floatValue=-1.99999f},5};
 compileGlob.value_start[1].value=(VariableValue){{.intValue=731},6};
 compileGlob.value_start[2].value=(VariableValue){{.floatValue=1282.54f},5};
 assert(FoldEmitter(&result)&&result.value.type==4);
 assert(!memcmp(folded,allocated,sizeof(folded)));
 VariableValue stack[3]={{{.floatValue=-1.99999f},5},{{.intValue=731},6},{{.floatValue=1282.54f},5}};
 Scr_CastVector(stack);
 assert(stack[0].type==4&&!memcmp(folded,allocated,sizeof(folded)));
 // Parenthesized scalar expressions must keep their ordinary value.
 holder[0].node=(intptr_t)list[0];
 assert(EvalPrimitiveExpression((sval_t){.node=(intptr_t)vector},&result));
 assert(result.value.type==5&&result.value.u.floatValue==1282.54f);
 puts("PASS: mixed constant x/y/z, runtime vector agreement and scalar parentheses");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-script-vector-') as directory:
    directory = Path(directory)
    for name, body, emitted in [('fixed', fold, emitter),
            ('reversed-eval', fold.replace('constValue2[2-i]', 'constValue2[i]'), emitter),
            ('reversed-emitter', fold, emitter.replace('first + 2 - i', 'first + i'))]:
        source = directory / (name + '.c')
        binary = directory / name
        source.write_text(support + body + emitted + cast + checks)
        subprocess.run(['cc', '-std=c11', '-O1', '-g', '-fsanitize=address,undefined',
                        str(source), '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
        if name == 'fixed':
            assert result.returncode == 0, result.stderr
            print(result.stdout.strip())
        else:
            assert result.returncode != 0, 'Reversed-axis mutation unexpectedly passed'
            print('PASS: ' + name + ' fails the vector regression')
