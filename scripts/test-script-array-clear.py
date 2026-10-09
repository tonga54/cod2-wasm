#!/usr/bin/env python3
"""Exercise real script-variable allocation and absent-key clearing under ASan/UBSan."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / 'src/PC/script/scr_variable.c').read_text()


def extract(name):
    match = re.search(r'\b' + name + r'\([^;]*?\)\s*\{', SOURCE)
    assert match, name
    start = SOURCE.rfind('\n', 0, match.start()) + 1
    if not SOURCE[start:match.start()].strip():
        start = SOURCE.rfind('\n', 0, start - 1) + 1
    end = SOURCE.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (SOURCE[end] == '{') - (SOURCE[end] == '}')
        end += 1
    return SOURCE[start:end] + '\n'


SUPPORT = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __attribute_regparm__(n)
typedef unsigned char byte;
typedef int Bool;
typedef union { int intValue; unsigned pointerValue, stringValue; } VariableUnion;
typedef struct { VariableUnion u; int type; } VariableValue;
typedef struct {
    struct { unsigned short id; union { unsigned short prev, prevSibling; } u; } hash;
    union { unsigned short next; struct { unsigned short refCount;
        union { unsigned short size, self, entnum; } u; } o; } u;
    union { unsigned status; } w;
    union { unsigned short index, next; } v;
    unsigned short nextSibling;
} VariableValueInternal;
static unsigned char scrVarGlob[1048608];
static unsigned s_varNetDbg;
static struct { int error_index; } scrVarPub;
static const char *var_typename[32];
static int stringRefs[65536];
static void Scr_TerminalError(const char *msg) { fprintf(stderr, "%s\n", msg); abort(); }
static void Scr_Error(const char *msg) { Scr_TerminalError(msg); }
static char *va(const char *fmt, ...) { return (char *)fmt; }
static void SL_RemoveRefToString(unsigned name) { assert(stringRefs[name] > 0); stringRefs[name]--; }
static void Scr_AddRefToArrayNameValue(unsigned name) { if (name <= 65535) stringRefs[name]++; }
static void Scr_RemoveRefToArrayNameValue(unsigned name) { if (name <= 65535) SL_RemoveRefToString(name); }
static void RemoveRefToValue_core(int type, VariableUnion value) { assert(type == 0 || type == 6); }
static unsigned Scr_CopyArrayForWrite(unsigned field, unsigned array) { return array; }
static unsigned Scr_EvalArrayRef(unsigned field) { abort(); }
'''
NAMES = ['Scr_VariableHash', 'Var_ResetAll', 'AllocVariable', 'FreeVariable',
         'FindVariableIndexInternal', 'Scr_LinkVariableToParent', 'ScrVarEntry',
         'ScrVarEntryIndex', 'GetNewVariableIndexInternal3', 'AllocObject_core',
         'Scr_AllocArray_core', 'GetVariable', 'MakeVariableExternal',
         'FreeChildValue_core', 'SafeRemoveVariable_core', 'SafeRemoveVariable',
         'RemoveVariable', 'IsValidArrayIndex', 'GetInternalVariableIndex_core', 'ClearArray']
BODY = '\n'.join(extract(name) for name in NAMES)
MACROS = SOURCE[SOURCE.index('#define SCRVL_SL'):SOURCE.index('COD2_ASSERT_FIELD')]
CHECKS = r'''
static void checkPool(unsigned expected) {
    unsigned count = 0, allocated = 0;
    unsigned char seen[65536] = {0};
    for (unsigned index = VG_U16(0); index; index = VG_U16(VG_ID(index))) {
        assert(index <= SCRVL_MAX_VARIABLES && !seen[index]);
        seen[index] = 1;
        assert(!(VG_STATUS(VG_ID(index)) & SCRVL_VAR_ALLOCATED));
        ++count;
    }
    for (unsigned id = 1; id <= SCRVL_MAX_VARIABLES; id++)
        allocated += !!(VG_STATUS(id) & SCRVL_VAR_ALLOCATED);
    assert(allocated == expected && allocated + count == SCRVL_MAX_VARIABLES);
    assert(VG_ID(0) == 0 && VG_STATUS(0) == 0);
}
int main(int argc, char **argv) {
    _Static_assert(sizeof(VariableValueInternal) == 16, "script variable layout");
    Var_ResetAll();
    unsigned owner = AllocObject_core();
    unsigned field = GetVariable(owner, 300);
    unsigned array = Scr_AllocArray_core();
    VG_STATUS(field) |= SCRVL_VAR_POINTER;
    VG_U32(field) = array;
    checkPool(3);
    unsigned char before[sizeof(scrVarGlob)];
    for (int pass = 0; pass < 20000; ++pass) {
        VariableValue key = {{ .intValue = pass % 2 ? -7 : 123 }, SCRVL_VAR_INTEGER};
        if (pass % 3 == 0) {
            key.type = SCRVL_VAR_STRING;
            key.u.stringValue = 501;
            stringRefs[501]++;
        }
        memcpy(before, scrVarGlob, sizeof(before));
        ClearArray(field, &key);
        assert(memcmp(before, scrVarGlob, sizeof(before)) == 0);
        if (key.type == SCRVL_VAR_STRING) assert(stringRefs[501] == 0);
        unsigned name = key.type == SCRVL_VAR_STRING ? key.u.stringValue : GetInternalVariableIndex_core(key.u.intValue);
        unsigned child = GetVariable(array, name);
        VG_STATUS(child) |= SCRVL_VAR_INTEGER;
        VG_U32(child) = 42;
        assert(VG_SIBLING(array) == 1);
        if (key.type == SCRVL_VAR_STRING) stringRefs[501]++;
        ClearArray(field, &key);
        assert(!FindVariableIndexInternal(array, name) && VG_SIBLING(array) == 0);
        if (key.type == SCRVL_VAR_STRING) assert(stringRefs[501] == 0);
        if (pass % 128 == 0) checkPool(3);
    }
    /* The generic remover must also leave the sentinel intact for missing keys. */
    memcpy(before, scrVarGlob, sizeof(before));
    RemoveVariable(array, 902);
    assert(memcmp(before, scrVarGlob, sizeof(before)) == 0);
    checkPool(3);
    puts("PASS: 20,000 absent/present integer and string clears preserve the free list and array size");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-array-clear-') as temp:
    temp = Path(temp)
    for variant in ('fixed', 'old-clear', 'unguarded-remove'):
        body = BODY
        if variant != 'fixed':
            remover = extract('RemoveVariable')
            unguarded = remover.replace('    if (!index)\n        return;\n', '')
            body = body.replace(remover, unguarded)
            if variant == 'old-clear':
                body = body.replace('SafeRemoveVariable(arrayId, name);', 'RemoveVariable(arrayId, name);')
        source = temp / (variant + '.c')
        binary = temp / variant
        source.write_text(SUPPORT + MACROS + body + CHECKS)
        subprocess.run(['cc', '-std=c11', '-O1', '-g', '-fsanitize=address,undefined',
                        '-fno-sanitize-recover=all', str(source), '-o', str(binary)], check=True)
        run = subprocess.run([str(binary)], text=True, capture_output=True, timeout=30)
        if variant == 'fixed':
            assert run.returncode == 0, run.stderr
            print(run.stdout.strip())
        else:
            assert run.returncode != 0, f'{variant} mutation unexpectedly passed'
            print(f'PASS: {variant} mutation reproduces sentinel corruption')
