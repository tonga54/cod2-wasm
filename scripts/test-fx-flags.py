#!/usr/bin/env python3
"""Run the production EFX flag parser on the flags retail effects name: each key takes its own table's names and
then the other table's, each entry setting its own word, so the snow, fires and barrel explosion that name
cheapOrgCalc under spawnFlags load; unknown names still fail the segment. impactFx alone sets the impact bit and not
usePhysics, as CoD2MP_s.exe's table does: the snow grenade's ring names it without an effect to play, and colliding
would play the Missing FX sprite."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
template = (root / 'src/PC/EffectsCore/FxTemplate.c').read_text()
stubs = (root / 'src/stubs/link_stubs.c').read_text()


def function(text, head):
    assert text.count(head) == 1, head
    start = text.index(head)
    depth, end = 1, text.index('{', start) + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


def table(name, count):
    match = re.search(r'\} ' + name + r'\[' + str(count) + r'\] __attribute__\(\(aligned\(4\)\)\) = \{(.*?)\n\};', stubs,
                      re.S)
    assert match, name
    return f'const FxFlagEntry {name}[{count}] = {{{match[1]}\n}};\n'


parser = function(template, 'static const FxFlagEntry *PrimitiveTemplate_FindFlag(') + function(
    template, 'Bool PrimitiveTemplate_ParseFlags(const PrimitiveTemplate *_this, const char *line, '
              'const FxFlagEntry *flagEntries, int flagEntryCount)\n{')
tables = table('fxAttributeFlags', 27) + table('fxSpawnFlags', 13)
assert template.count('PrimitiveTemplate_ParseFlags(_this, val, fxAttributeFlags, 27)') == 1
assert template.count('PrimitiveTemplate_ParseFlags(_this, val, fxSpawnFlags, 13)') == 1

support = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
typedef int Bool;
typedef unsigned char byte;
typedef struct { const char *flag; unsigned int masks[2]; } FxFlagEntry;
typedef struct { unsigned int mAttributeFlags, mSpawnFlags; } PrimitiveTemplate;
#define stricmp strcasecmp
static int allocations;
static void *Hunk_AllocateTempMemoryInternal(int size) { ++allocations; return malloc((size_t)size); }
static void Hunk_FreeTempMemory(void *buffer) { --allocations; free(buffer); }
'''

checks = r'''
static const FxFlagEntry *find(const FxFlagEntry *entries, int count, const char *name)
{
    for (int i = 0; i < count; ++i)
        if (!strcasecmp(entries[i].flag, name))
            return &entries[i];
    return NULL;
}

/* The flag line's words, as the key's parse leaves the template. */
static int parse(const char *line, int spawn, unsigned int *attribute, unsigned int *spawnWord)
{
    PrimitiveTemplate p = { 0, 0 };
    int ok = spawn ? PrimitiveTemplate_ParseFlags(&p, line, fxSpawnFlags, 13)
                   : PrimitiveTemplate_ParseFlags(&p, line, fxAttributeFlags, 27);
    assert(allocations == 0);
    *attribute = p.mAttributeFlags;
    *spawnWord = p.mSpawnFlags;
    return ok;
}

int main(void)
{
    unsigned int a, s;
    int cases = 0;
    /* Every name retail effects put under each key parses there and sets
     * the bits of the entry that names it, wherever it is listed. */
    static const char *const spawnNames[] = { "orgOnCylinder", "axisFromSphere", "orgOnSphere", "evenDistribution",
        "frustumCull", "randrotaroundfwd", "absoluteVel", "absoluteAccel", "cheapOrgCalc" };
    static const char *const attributeNames[] = { "useAlpha", "useRandomRotationDelta", "usePhysics", "impactKills",
        "useModel", "useRandomSize", "absoluteVel2", "relative", "useRandomVelocity", "setShaderTime", "useRandomColors",
        "depthHack", "useRandomVelocity2", "absoluteVel", "useBBox", "useRandomAlpha", "emitFx", "blocksSight",
        "impactFx", "linear", "useRandomSize2", "useRandomLength", "deathFx", "disableFarPlaneCulling" };
    for (int key = 0; key < 2; ++key) {
        const char *const *names = key ? spawnNames : attributeNames;
        int count = key ? (int)(sizeof(spawnNames) / sizeof(*spawnNames))
                        : (int)(sizeof(attributeNames) / sizeof(*attributeNames));
        for (int i = 0; i < count; ++i) {
            const FxFlagEntry *own = find(key ? fxSpawnFlags : fxAttributeFlags, key ? 13 : 27, names[i]);
            const FxFlagEntry *entry = own ? own : find(key ? fxAttributeFlags : fxSpawnFlags, key ? 27 : 13, names[i]);
            assert(entry && parse(names[i], key, &a, &s) && a == entry->masks[0] && s == entry->masks[1]);
            cases++;
        }
    }
    /* Stalingrad's snow and the fires' and barrel explosion's spawn lines:
     * the cheap origin and their distribution bits, nothing else. */
    assert(parse("cheapOrgCalc evenDistribution", 1, &a, &s) && a == 0 && s == (0x040u | 0x200u));
    assert(parse("cheapOrgCalc axisFromSphere orgOnSphere", 1, &a, &s) && a == 0 && s == (0x040u | 0x002u | 0x001u));
    assert(parse("\tcheapOrgCalc  orgOnCylinder\t", 1, &a, &s) && a == 0 && s == (0x040u | 0x004u));
    /* Names keep their bits under their own key: depthHack and deathFx, which
     * effects list under flags, set the attribute word. */
    assert(parse("useAlpha depthHack deathFx", 0, &a, &s) && a == (0x080u | 0x001u | 0x200u) && s == 0);
    assert(parse("cheapOrgCalc", 0, &a, &s) && a == 0 && s == 0x040u);
    assert(parse("linear disableFarPlaneCulling", 1, &a, &s) && a == (0x4000000u | 0x2000000u) && s == 0);
    assert(parse("CHEAPORGCALC EVENDISTRIBUTION", 1, &a, &s) && a == 0 && s == (0x040u | 0x200u));
    cases += 7;
    /* impactFx is the impact bit alone under either key; only an impactfx
     * list adds usePhysics. grenadeExp_snow's ring_outter names it with no
     * list, so its flakes never trace the world for an effect to play. */
    assert(parse("impactFx", 0, &a, &s) && a == 0x800u && s == 0);
    assert(parse("impactFx", 1, &a, &s) && a == 0x800u && s == 0);
    assert(parse("impactKills impactFx useAlpha useRandomColors useRandomAlpha useRandomSize", 0, &a, &s) &&
           a == (0x400u | 0x800u | 0x080u | 0x2000u | 0x4000u | 0x8000u) && !(a & 0x020u) && s == 0);
    assert(parse("usePhysics impactKills impactFx", 0, &a, &s) && a == (0x020u | 0x400u | 0x800u) && s == 0);
    cases += 4;
    /* Names neither table knows still fail the line, and so the segment. */
    for (int key = 0; key < 2; ++key) {
        assert(!parse("rgbComponentInterpolation", key, &a, &s));
        assert(!parse("cheapOrgCalc bogusFlag", key, &a, &s));
        assert(!parse("", key, &a, &s));
        cases += 3;
    }
    assert(parse("   ", 1, &a, &s) && a == 0 && s == 0);
    printf("%d\n", cases + 1);
    return 0;
}
'''

mutants = [
    ('    for (entryIndex = 0; entryIndex < otherCount; ++entryIndex) {\n        if (!stricmp(other[entryIndex].flag, flag))\n'
     '            return &other[entryIndex];\n    }\n', ''),
    ('flagEntries == fxSpawnFlags ? fxAttributeFlags : fxSpawnFlags;', 'flagEntries == fxSpawnFlags ? fxSpawnFlags : fxAttributeFlags;'),
    ('(int)(sizeof(fxAttributeFlags) / sizeof(fxAttributeFlags[0]))', '13'),
    ('->mAttributeFlags |= entry->masks[0];', '->mAttributeFlags |= entry->masks[1];'),
    ('->mSpawnFlags |= entry->masks[1];', '->mSpawnFlags |= entry->masks[0];'),
    ('        if (!entry) {\n            Hunk_FreeTempMemory(flag);\n            return 0;\n        }\n',
     '        if (!entry)\n            continue;\n'),
    ('        if (!stricmp(flagEntries[entryIndex].flag, flag))\n            return &flagEntries[entryIndex];\n',
     '        if (!strcmp(flagEntries[entryIndex].flag, flag))\n            return &flagEntries[entryIndex];\n'),
    ('{ "impactFx", { 0x000800, 0 } },', '{ "impactFx", { 0x000820, 0 } },'),
    ('{ "impactFx", { 0x800, 0 } },', '{ "impactFx", { 0x820, 0 } },'),
]

with tempfile.TemporaryDirectory(prefix='cod2-fx-flags-') as directory:
    path, binary = Path(directory) / 'flags.c', Path(directory) / 'flags'
    for index, (old, new) in enumerate([(None, None)] + mutants):
        code = tables + parser
        if old is not None:
            assert code.count(old) == 1, old
            code = code.replace(old, new)
        path.write_text(support + code + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wall', '-Werror' if index == 0 else '-w', str(path), '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        if index == 0:
            assert result.returncode == 0, result.stderr[-2000:]
            cases = int(result.stdout)
        else:
            assert result.returncode != 0, f'mutant survived: {old!r} -> {new!r}'

print(f'PASS: the production EFX flag parser takes every name retail effects put under flags and spawnFlags, the '
      f'snow, fires and barrel explosion\'s cheapOrgCalc among them, sets each entry\'s own word, leaves usePhysics '
      f'out of impactFx and still refuses unknown names ({cases} cases); all {len(mutants)} mutants fail')
