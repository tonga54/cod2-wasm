#!/usr/bin/env python3
"""Rewrite muzzle flashes with the production generator (browser_muzzle_flashes.py) and parse every generated flag
line and curve with the production EFX parser. Each original's lights and lasting smoke stay byte for byte and in
order; its fireball puffs and other brief elements give way to a burst, a core, a forward tongue and sparks that
show at full strength at once and fade out within 55, 35, 45 and 140 ms. Mutants of the generator fail."""
from pathlib import Path
import re
import subprocess
import tempfile
import types

root = Path(__file__).resolve().parent.parent
generator = (root / 'scripts/browser_muzzle_flashes.py').read_text()
template = (root / 'src/PC/EffectsCore/FxTemplate.c').read_text()
stubs = (root / 'src/stubs/link_stubs.c').read_text()
curve_source = (root / 'src/PC/EffectsCore/FxCurve_load_obj.c').read_text()
loader = (root / 'src/PC/EffectsCore/FxScheduler_load_obj.c').read_text()

# The weapon files' view and world flashes (weapons/mp/*: viewFlashEffect, worldFlashEffect).
FLASH_FILES = {'heavy_view.efx', 'heavy.efx', 'mg42hv_view.efx', 'mg42hv.efx', 'pistolflash_view.efx',
               'pistolflash.efx', 'rifleflash.efx', 'shotgunflash.efx', 'standardflashview.efx',
               'standardflashworld.efx', 'thompson_view.efx', 'thompson.efx'}
PUFFS = {'gfx_exp_fireball_atlas', 'gfx_muzflash2', 'gfx_muzflash2a', 'gfx_fire_ball'}
PARSER_KEYS = {key.lower() for key in re.findall(r'stricmp\(key, "([^"]+)"\)', template)}
PRIMITIVES = {name.lower() for name in re.findall(r'\{ "(\w+)", PT_\w+ \}', loader)}
assert {'particle', 'tail', 'light'} <= PRIMITIVES and {'life', 'rgb', 'velocityx', 'shaders'} <= PARSER_KEYS


def function(source, name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


def table(name, count):
    match = re.search(r'\} ' + name + r'\[' + str(count) + r'\] __attribute__\(\(aligned\(4\)\)\) = \{(.*?)\n\};', stubs,
                      re.S)
    assert match, name
    return f'const FxFlagEntry {name}[{count}] = {{{match[1]}\n}};\n'


harness = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
typedef int Bool, FxChannelId;
typedef unsigned char byte;
enum {FXCHAN_COLOR, FXCHAN_COLOR_RAND, FXCHAN_COUNT = 24};
typedef struct {float mMin, mMax;} FxRange;
typedef struct {int dimensionCount, keyCount; float keys[];} FxCurve;
typedef struct {const FxCurve *curve; FxRange scaleRange;} FxChannel;
typedef struct {unsigned int mAttributeFlags, mSpawnFlags; FxChannel mFxChannels[24];} PrimitiveTemplate;
typedef struct {const char *flag; unsigned int masks[2];} FxFlagEntry;
typedef struct GPValue {const char *text; struct GPValue *next, *list; const char *value;} GPValue;
typedef struct {GPValue *pairs;} GPGroup;
typedef struct {FxRange start[3], end[3], parm; int flags, containsData;} FxChannelBackwardCompatible;
typedef struct {FxChannelBackwardCompatible fxChannels[24];} BackCompatibleParameters;
#define GPV_STRING(p) ((p)->text)
#define GPV_NEXT(p) ((p)->next)
#define GPV_LIST(p) ((p)->list)
#define GPG_PAIRS(p) ((p)->pairs)
#define stricmp strcasecmp
static const char *GPValue_GetTopValue(GPValue *p) { return p->value; }
static void *Hunk_AllocateTempMemoryInternal(int size) { return malloc((size_t)size); }
static void *Hunk_AllocAlignInternal(int size, int align) { (void)align; return malloc((size_t)size); }
static void Hunk_FreeTempMemory(void *p) { free(p); }
''' + table('fxAttributeFlags', 27) + table('fxSpawnFlags', 13) + function(curve_source, 'FxCurve_AllocAndCreateWithKeys')
for name in ('PrimitiveTemplate_FindFlag', 'PrimitiveTemplate_ParseFlags', 'EnsureMinMax', 'ParseFloatRange',
             'ParseVec3Range', 'PrimitiveTemplate_ParseChannelCurve', 'PrimitiveTemplate_ChannelDimensions',
             'PrimitiveTemplate_ParseChannelRanges', 'PrimitiveTemplate_CopyChannelRanges',
             'PrimitiveTemplate_ParseChannelFlags', 'PrimitiveTemplate_ParseChannel'):
    harness += function(template, name)
harness += r'''
static char *next_line(void)
{
    static char line[1024];
    if (!fgets(line, sizeof(line), stdin))
        return NULL;
    line[strcspn(line, "\n")] = 0;
    return line;
}

/* "flags 0|1 <names>", then "curve <dimensions> <keys> <scale|->" with the
 * key lines, the expected keys and the expected scale range. */
int main(void)
{
    int flags = 0, curves = 0;
    char *line;
    while ((line = next_line())) {
        if (!strncmp(line, "flags ", 6)) {
            PrimitiveTemplate p = {0};
            int spawn = line[6] == '1';
            if (!(spawn ? PrimitiveTemplate_ParseFlags(&p, line + 8, fxSpawnFlags, 13)
                        : PrimitiveTemplate_ParseFlags(&p, line + 8, fxAttributeFlags, 27))) {
                fprintf(stderr, "rejected flags: %s\n", line + 8);
                return 1;
            }
            flags++;
            continue;
        }
        int dimensions, count;
        char scale[256];
        assert(sscanf(line, "curve %d %d %255[^\n]", &dimensions, &count, scale) == 3);
        int stride = dimensions + 1;
        GPValue *values = calloc((size_t)count, sizeof(*values));
        for (int i = 0; i < count; i++) {
            values[i].text = strdup(next_line());
            if (i + 1 < count)
                values[i].next = &values[i + 1];
        }
        float *expected = malloc(sizeof(float) * (size_t)(count * stride));
        char *cursor = next_line();
        for (int i = 0; i < count * stride; i++)
            expected[i] = strtof(cursor, &cursor);
        float minimum, maximum;
        assert(sscanf(next_line(), "%f %f", &minimum, &maximum) == 2);
        GPValue scaleValue = {.text = "scale", .value = scale};
        GPValue curve = {.text = "curve", .list = values, .next = strcmp(scale, "-") ? &scaleValue : NULL};
        GPGroup group = {&curve};
        PrimitiveTemplate p = {0};
        BackCompatibleParameters back = {0};
        int channel = dimensions == 3 ? FXCHAN_COLOR : 4;
        p.mFxChannels[channel].scaleRange = (FxRange){1, 1};
        assert(PrimitiveTemplate_ParseChannel(&p, &back, &group, channel, 0, 0, 0, 0, 0, 0, 0, 0));
        const FxCurve *c = p.mFxChannels[channel].curve;
        if (!c || c->dimensionCount != dimensions || c->keyCount != count) {
            fprintf(stderr, "curve dropped or reshaped: %s\n", values[0].text);
            return 1;
        }
        for (int i = 0; i < count * stride; i++)
            assert(fabsf(c->keys[i] - expected[i]) < 0.00001f);
        assert(p.mFxChannels[channel].scaleRange.mMin == minimum && p.mFxChannels[channel].scaleRange.mMax == maximum);
        for (int i = 0; i < count; i++)
            free((void *)values[i].text);
        free(values);
        free(expected);
        free((void *)c);
        curves++;
    }
    printf("%d %d\n", flags, curves);
    return 0;
}
'''


def load(source):
    module = types.ModuleType('browser_muzzle_flashes')
    exec(compile(source, 'browser_muzzle_flashes.py', 'exec'), module.__dict__)
    return module


def element(kind, name, life=None, shader=None, extra=()):
    lines = [kind, '{', f'\tname\t\t\t\t{name}', '']
    if life:
        lines += [f'\tlife\t\t\t\t{life}', '']
    lines += list(extra)
    lines += ['\trgb', '\t{', '\t\tcurve', '\t\t[', '\t\t\t0 1 0.7137 0.6196', '\t\t\t1 1 0.7137 0.6196', '\t\t]', '\t}', '']
    if shader:
        lines += ['\tshaders', '\t[', f'\t\t{shader}', '\t]']
    lines.append('}')
    return '\r\n'.join(lines)


# Retail-shaped originals: CRLF, a light with a life and one without, lasting
# smoke, an ember outliving the flash limit, and brief puffs and streaks up to
# and at the limit, one without a life (CoD2's default is 1 ms).
LIGHT = element('Light', 'light', '100', extra=('\torigin\t\t\t\t18 0 0 29 0 0', ''))
SMOKE = element('Particle', 'smoke', '1000 1500', 'gfx_whisp_spiral',
                ('\tflags\t\t\t\tuseAlpha useRandomRotationDelta', '', '\tcullrange\t\t\t\t1000', ''))
EMBER = element('Particle', 'ember', '150 201', 'gfx_spark_atlas')
FLASH_LIGHT = element('Light', 'flash_light')
BRIEF = [element('Particle', 'small_flash', '100', 'gfx_exp_fireball_atlas', ('\tcount\t\t\t\t3', '')),
         element('Tail', 'center_line', '100 50', 'gfx_mg42a'),
         element('Particle', 'edge', '200', 'gfx_muzflash2'),
         element('OrientedParticle', 'lifeless', None, 'gfx_fire_ball')]
ORIGINAL = '\r\n\r\n'.join([BRIEF[0], LIGHT, SMOKE, BRIEF[1], EMBER, BRIEF[2], FLASH_LIGHT, BRIEF[3]]) + '\r\n'
KEPT = [LIGHT, SMOKE, EMBER, FLASH_LIGHT]


def parse_new(text):
    """Fields, channels (keys, scale) and shaders of a generated element."""
    lines = [line.strip() for line in text.splitlines()]
    fields, channels, shaders, i = {}, {}, [], 2
    while lines[i] != '}':
        line = lines[i]
        if not line:
            i += 1
        elif line == 'shaders':
            i += 2
            while lines[i] != ']':
                shaders.append(lines[i])
                i += 1
            i += 1
        elif lines[i + 1] == '{':
            assert lines[i + 2] == 'curve' and lines[i + 3] == '[', line
            i += 4
            keys = []
            while lines[i] != ']':
                keys.append(lines[i])
                i += 1
            i += 1
            scale = None
            while lines[i] != '}':
                if lines[i].startswith('scale'):
                    scale = lines[i].split(None, 1)[1]
                i += 1
            i += 1
            channels[line] = (keys, scale)
        else:
            key, value = (line.split(None, 1) + [''])[:2]
            fields[key] = value
            i += 1
    return fields, channels, shaders


def values(keys, column=1):
    return [float(key.split()[column]) for key in keys]


def verify(module):
    """Every design check on the generator's output; the parser cases for the harness."""
    assert set(module.EFFECTS) == FLASH_FILES
    assert set(module.EFFECTS.values()) <= set(module.FAMILIES)
    cases = []
    read_names = []
    assets = module.muzzle_flash_assets(lambda name: read_names.append(name) or ORIGINAL.encode('latin-1'))
    assert sorted(assets) == sorted(read_names) == sorted('fx/muzzleflashes/' + name for name in FLASH_FILES)
    for name, family in module.EFFECTS.items():
        text = assets['fx/muzzleflashes/' + name].decode('latin-1')
        assert text == module.rewrite_flash(ORIGINAL, family).decode('latin-1')
        assert '\n' not in text.replace('\r\n', ''), 'mixed line endings'
        elements = module.split_elements(text)
        assert [body for kind, body in elements[:len(KEPT)]] == KEPT, 'lights and smoke kept, verbatim, in order'
        for brief in BRIEF:
            assert brief not in text and brief.split('\r\n')[2] not in text, 'brief original element kept'
        new = elements[len(KEPT):]
        burst, core, tongue_length, tongue_width, (sparks_min, sparks_max), spark_speed = module.FAMILIES[family]
        parsed = {}
        for kind, body in new:
            assert kind.lower() in PRIMITIVES
            fields, channels, shaders = parse_new(body)
            for key in list(fields) + list(channels):
                assert key.lower() in PARSER_KEYS, key
            assert len(shaders) == 1 and not set(shaders) & PUFFS
            assert 'relative' in fields['flags'].split(), 'flash follows the muzzle'
            parsed[fields['name']] = (kind, fields, channels, shaders[0])
            cases.append('flags 0 ' + fields['flags'])
            if 'spawnFlags' in fields:
                cases.append('flags 1 ' + fields['spawnFlags'])
            for channel, (keys, scale) in channels.items():
                dimensions = 3 if channel.lower() in ('rgb', 'rgbrand') else 1
                times = values(keys, 0)
                assert times[0] == 0 and times[-1] == 1 and times == sorted(set(times)), (channel, keys)
                scales = [float(v) for v in scale.split()] if scale else [1, 1]
                cases.append(f'curve {dimensions} {len(keys)} {scale or "-"}')
                cases += keys
                cases.append(' '.join(' '.join(key.split()[:dimensions + 1]) for key in keys))
                cases.append(f'{min(scales)} {max(scales)}')
        assert list(parsed) == ['burst', 'core', 'tongue'] + (['sparks'] if sparks_max else [])
        lives = {name: max(float(v) for v in fields['life'].split()) for name, (_, fields, _, _) in parsed.items()}
        assert lives['burst'] <= 55 and lives['core'] <= 35 and lives['tongue'] <= 45, lives
        for name, (kind, fields, channels, shader) in parsed.items():
            for channel in ('alpha', 'alphaRand'):
                alpha = values(channels[channel][0])
                assert alpha[0] == max(alpha) > 0 and alpha[-1] == 0, (name, channel, alpha)
            red, green, blue = (values(channels['rgb'][0], column) for column in (1, 2, 3))
            assert all(r >= g >= b for r, g, b in zip(red, green, blue)), (name, 'warm')
        kind, fields, channels, shader = parsed['burst']
        assert kind == 'Particle' and shader == 'gfx_explosion_flash' and fields['count'] == '1'
        size = values(channels['size'][0])
        assert size[0] >= 0.6 * size[-1] and float(channels['size'][1]) == burst, 'a burst, not a swelling puff'
        kind, fields, channels, shader = parsed['core']
        assert kind == 'Particle' and shader == 'gfx_spark_atlas' and float(channels['size'][1]) == core
        kind, fields, channels, shader = parsed['tongue']
        assert kind == 'Tail' and shader == 'gfx_starburst_1p'
        assert float(channels['length'][1]) == tongue_length and float(channels['size'][1]) == tongue_width
        assert max(values(channels['velocityX'][0]) + values(channels['velocityXRand'][0])) < 0, \
            'the tongue moves back, so its tail streaks forward out of the barrel'
        if sparks_max:
            kind, fields, channels, shader = parsed['sparks']
            assert kind == 'Tail' and shader == 'gfx_spark_atlas' and lives['sparks'] <= 140
            assert fields['count'] == f'{sparks_min} {sparks_max}'
            assert min(values(channels['velocityX'][0]) + values(channels['velocityXRand'][0])) > 0
            assert float(channels['velocityX'][1]) == spark_speed, 'sparks fly forward'
    families = module.FAMILIES
    assert families['pistol'][0] < families['standard'][0] <= families['heavy'][0] < families['rifle'][0]
    assert max(families, key=lambda name: families[name][0]) == 'shotgun'
    assert max(families, key=lambda name: families[name][2]) == 'mg'
    return cases


MUTANTS = [
    ("if kind.lower() == 'light' or life_max(element) > FLASH_LIFE_MS]", "if life_max(element) > FLASH_LIFE_MS]"),
    ('life_max(element) > FLASH_LIFE_MS]', 'life_max(element) >= 0]'),
    ('FLASH_LIFE_MS = 200', 'FLASH_LIFE_MS = 199'),
    ("r'^[ \\t]*life[ \\t]+([\\d.]+)(?:[ \\t]+([\\d.]+))?'", "r'^[ \\t]*life[ \\t]+([\\d.]+)'"),
    ('if start >= i:', 'if start > i:'),
    ("newline = '\\r\\n' if '\\r\\n' in original else '\\n'", "newline = '\\n'"),
    ("('life', '40 55')", "('life', '40 100')"),
    ("('life', '30 45')", "('life', '30 60')"),
    ("((0, 1), (0.25, 0.72), (0.6, 0.24), (1, 0))", "((0, 0.2), (0.25, 0.72), (0.6, 0.24), (1, 0))"),
    ("((0, 0.9), (0.4, 0.55), (1, 0))", "((0, 0.9), (0.4, 0.55), (1, 0.1))"),
    ("((0, 0.8), (1, 1.15))", "((0, 0), (1, 1.15))"),
    ("'gfx_explosion_flash'", "'gfx_exp_fireball_atlas'"),
    ("{'velocityX': (((0, -0.5), (1, -0.5)), 50)", "{'velocityX': (((0, 0.5), (1, 0.5)), 50)"),
    ("'velocityX': (((0, 1), (1, 1)), spark_speed)", "'velocityX': (((0, -1), (1, -1)), spark_speed)"),
    ("('flags', 'useRandomAlpha useRandomSize relative'),\n            ('spawnFlags'",
     "('flags', 'useRandomAlpha useRandomSize'),\n            ('spawnFlags'"),
    ("useRandomLength useRandomVelocity relative'),\n            ('life', '30 45')",
     "useRandomLength useRandomVelocty relative'),\n            ('life', '30 45')"),
    ("('spawnFlags', 'evenDistribution'), ('count', '1'), ('life', '40 55')",
     "('spawnFlags', 'evenDistributon'), ('count', '1'), ('life', '40 55')"),
    ("' '.join(_number(v) for v in key) for key in keys]", "' '.join(_number(v) for v in key[1:]) for key in keys]"),
    ("    'heavy': (5.5,", "    'heavy': (8.5,"),
    ("'thompson_view.efx': 'smg', ", ""),
]

with tempfile.TemporaryDirectory(prefix='cod2-muzzle-flashes-') as tmp:
    source, binary = Path(tmp) / 'parser.c', Path(tmp) / 'parser'
    source.write_text(harness)
    subprocess.run(['cc', '-std=c11', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    str(source), '-lm', '-o', str(binary)], check=True)

    def parse(cases):
        return subprocess.run([str(binary)], input='\n'.join(cases) + '\n', text=True, capture_output=True)

    cases = verify(load(generator))
    result = parse(cases)
    assert result.returncode == 0, result.stderr
    flag_lines, curve_count = map(int, result.stdout.split())
    killed = 0
    for old, new in MUTANTS:
        assert generator.count(old) == 1, old
        try:
            mutant_cases = verify(load(generator.replace(old, new)))
        except (AssertionError, KeyError, ValueError, IndexError):
            killed += 1
            continue
        if parse(mutant_cases).returncode != 0:
            killed += 1
            continue
        raise AssertionError('mutant survived: ' + new)
print(f'PASS: {len(FLASH_FILES)} muzzle flashes rewritten; {flag_lines} flag lines and {curve_count} curves through the '
      f'production EFX parser; lights and smoke kept verbatim; {killed} mutants fail')
