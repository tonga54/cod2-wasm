#!/usr/bin/env python3
"""Rewrite grenade explosions with the production generator (browser_grenade_explosions.py) and parse every generated
flag line and curve with the production EFX parser. Each surface keeps its debris, dust and crater byte for byte and
in order, and runs the shared fire and its own smoke in place of its fire, light and smoke. The fire flashes, rolls
out as fireballs that play their frames once over their lives and rise, throws sparks and embers that fall, and lights
its surroundings briefly; the smoke fades in, billows out, climbs and thins over seconds in its surface's dust. The
textures are deterministic: the fireball cools frame to frame, the smoke's alpha is its density; their IWIs carry the
mip chain the production loader counts, the smoke's with its alpha; their materials keep the soft-edged templates'
state and feather parameters. Mutants fail."""
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import types

root = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(root / 'scripts'))
generator = (root / 'scripts/browser_grenade_explosions.py').read_text()
template = (root / 'src/PC/EffectsCore/FxTemplate.c').read_text()
stubs = (root / 'src/stubs/link_stubs.c').read_text()
curve_source = (root / 'src/PC/EffectsCore/FxCurve_load_obj.c').read_text()
loader = (root / 'src/PC/EffectsCore/FxScheduler_load_obj.c').read_text()
image_loader = (root / 'src/PC/gfx_d3d/r_image_load_obj.c').read_text()
impacts = (root / 'src/PC/cgame/cg_effects_load_obj.c').read_text()

PARSER_KEYS = {key.lower() for key in re.findall(r'stricmp\(key, "([^"]+)"\)', template)}
PRIMITIVES = {name.lower() for name in re.findall(r'\{ "(\w+)", PT_\w+ \}', loader)}
assert {'particle', 'tail', 'light', 'fxrunner'} <= PRIMITIVES
assert {'life', 'rgb', 'velocityx', 'playfx', 'sequenceplayratemode', 'gravity'} <= PARSER_KEYS
# The surfaces the impact tables give grenade_explode, but water, whose splash is kept, and the
# debris, dust and craters each retail grenadeExp effect names.
SURFACES = ('blacktop', 'concrete', 'dirt', 'mud', 'snow', 'wood')
KEPT = {
    'blacktop': ('Decal', 'grain'),
    'concrete': ('decal', 'grain'),
    'dirt': ('dirt_eject', 'spinning_dust', 'blast_decal', 'rockychunks_high', 'rockychunks_out', 'dirt_chunks'),
    'mud': ('mud_out', 'splat_decal', 'dirt_eject', 'dirt_chunks', 'spinning_smoke', 'blast_decal', 'rockychunks_high',
            'rockychunks_out'),
    'snow': ('ring_outter', 'snow_flakes_up', 'spinning_mist', 'blast_decal', 'rockychunks_high', 'rockychunks_out',
             'chunks', 'grain'),
    'wood': ('decal', 'grain', 'wood_splinter'),
}
assert '"grenade_explode"' in impacts
LIBM = re.compile(r'\bmath\.(exp|expm1|log\w*|pow|sin|cos|tan|asin|acos|atan2?|sinh|cosh|tanh|hypot|dist|erf\w*|'
                  r'gamma|lgamma|cbrt|exp2)\b|\*\*|\bpow\(')


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
harness += function(image_loader, 'Image_ComputeMipCount') + r'''
static char *next_line(void)
{
    static char line[1024];
    if (!fgets(line, sizeof(line), stdin))
        return NULL;
    line[strcspn(line, "\n")] = 0;
    return line;
}

/* "flags 0|1 <names>"; "curve <dimensions> <keys> <scale|->" with the key
 * lines, the expected keys and the expected scale range; "mips <w> <h>". */
int main(void)
{
    int flags = 0, curves = 0;
    char *line;
    while ((line = next_line())) {
        int width, height;
        if (sscanf(line, "mips %d %d", &width, &height) == 2) {
            printf("mips %d\n", Image_ComputeMipCount(width, height, 1));
            continue;
        }
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
    module = types.ModuleType('browser_grenade_explosions')
    module.__source__ = source
    exec(compile(source, 'browser_grenade_explosions.py', 'exec'), module.__dict__)
    return module


def element(kind, name, shader=None, extra=()):
    lines = [kind, '{', f'\tname\t\t\t\t{name}', '', '\tlife\t\t\t\t1000 1200', ''] + list(extra)
    lines += ['\trgb', '\t{', '\t\tcurve', '\t\t[', '\t\t\t0 0.3559 0.3294 0.3', '\t\t\t1 0.3559 0.3294 0.3', '\t\t]',
              '\t}', '']
    if shader:
        lines += ['\tshaders', '\t[', f'\t\t{shader}', '\t]']
    lines.append('}')
    return '\r\n'.join(lines)


# Retail-shaped originals: CRLF, the surface's debris, dust and crater, and the
# fire, light, smoke and runners the generated effects replace.
REPLACED = [element('Particle', 'fire_flash', 'gfx_exp_fireball_atlas', ('\tcount\t\t\t\t3', '')),
            element('Light', 'Light'), element('Particle', 'high_smoke', 'gfx_smokepuff_atlas'),
            element('Particle', 'low_smoke', 'gfx_whisp_spiral'),
            element('FxRunner', 'concusive_flash', extra=('\tplayfx\r\n\t[\r\n\t\t/fx/explosions/grenade_flash\r\n\t]', '')),
            element('FxRunner', 'ground_smoke', extra=('\tplayfx\r\n\t[\r\n\t\t/fx/smoke/grenade_smoke\r\n\t]', ''))]


GRIT_NAMES = ('grain', 'dirt_chunks')


def original(module, surface):
    """A retail-shaped original and its kept elements as the rewrite leaves them: grit starting after the flash."""
    kept = [element('Decal' if 'decal' in name.lower() else 'Particle', name,
                    'gfx_dirtchunks' if name in GRIT_NAMES else 'gfx_rockdebri_01',
                    ('\tdelay\t\t\t\t0 200', '') if name in GRIT_NAMES else ()) for name in KEPT[surface]]
    mixed = []
    for i in range(max(len(kept), len(REPLACED))):
        mixed += REPLACED[i:i + 1] + kept[i:i + 1]
    return '\r\n\r\n'.join(mixed) + '\r\n', [settled(body) for body in kept], kept


def settled(body):
    """A kept element as the rewrite leaves it: grit after the flash, a crater after the light."""
    body = body.replace('\tdelay\t\t\t\t0 200', '\tdelay\t\t\t\t180 200')
    if body.startswith('Decal'):
        name = body.split('\r\n')[2]
        body = body.replace(name + '\r\n', name + '\r\n\r\n\tdelay\t\t\t\t250\r\n', 1)
    return body


def parse_new(text):
    """Fields, channels (keys, scale) and bracketed lists of a generated element."""
    lines = [line.strip() for line in text.splitlines()]
    fields, channels, lists, i = {}, {}, {}, 2
    while lines[i] != '}':
        line = lines[i]
        if not line:
            i += 1
        elif lines[i + 1] == '[':
            i += 2
            lists[line] = []
            while lines[i] != ']':
                lists[line].append(lines[i])
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
    return fields, channels, lists


def values(keys, column=1):
    return [float(key.split()[column]) for key in keys]


def numbers(field):
    return [float(v) for v in field.split()]


def parsed_effect(module, text, cases):
    """Each element of a generated effect by name, with its parser cases."""
    assert '\n' not in text.replace('\r\n', ''), 'CRLF throughout'
    parsed = {}
    for kind, body in module.split_elements(text):
        assert kind.lower() in PRIMITIVES, kind
        fields, channels, lists = parse_new(body)
        for key in list(fields) + list(channels) + list(lists):
            assert key.lower() in PARSER_KEYS, key
        assert fields['name'] not in parsed
        parsed[fields['name']] = (kind, fields, channels, lists)
        if 'flags' in fields:
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
    return parsed


def luminance(rgb):
    return 0.3 * rgb[0] + 0.59 * rgb[1] + 0.11 * rgb[2]


def verify_effects(module):
    """Every design check on the generated effects; the parser cases for the harness."""
    assert tuple(module.SURFACES) == SURFACES
    assert {surface: tuple(module.SURFACES[surface].kept) for surface in SURFACES} == KEPT, 'each keeps all its debris'
    cases = []
    effects = {}
    for surface in SURFACES:
        text, kept, raw = original(module, surface)
        rewritten = module.rewrite_surface(text, surface).decode('latin-1')
        assert '\n' not in rewritten.replace('\r\n', ''), 'the original line endings'
        elements = module.split_elements(rewritten)
        assert [body for kind, body in elements[:len(kept)]] == kept, 'debris, dust and crater kept, verbatim, in order'
        for replaced in REPLACED:
            assert replaced not in rewritten and replaced.split('\r\n')[2] not in rewritten, 'replaced element kept'
        runners = parsed_effect(module, '\r\n\r\n'.join(body for kind, body in elements[len(kept):]) + '\r\n', cases)
        assert [kind for kind, *_ in runners.values()] == ['FxRunner', 'FxRunner']
        targets = [lists['playfx'] for kind, fields, channels, lists in runners.values()]
        assert targets == [['/' + module.FIRE[:-4]], ['/' + module.SMOKE.format(surface)[:-4]]], targets
        try:
            module.rewrite_surface(text.replace(raw[0], ''), surface)
        except ValueError:
            pass
        else:
            raise AssertionError('a surface missing an element it keeps is refused')
    grit = '\r\n'.join(['Particle', '{', '\tname\t\t\t\tdirt_chunks', '', '\tdelay\t\t\t\t0 250', '',
                        '\tsize', '\t{', '\t\tcurve', '\t\t[', '\t\t\t0 0.2812 1 0.75', '\t\t]', '', '\t\tscale\t\t\t100',
                        '\t}', '', '\tsizeRand', '\t{', '\t\tcurve', '\t\t[', '\t\t\t0 1 1 0', '\t\t]', '',
                        '\t\tscale\t\t\t100', '\t}', '', '\tshaders', '\t[', '\t\tgfx_dirtchunks', '\t]', '}'])
    settled = module.settle_grit('Particle', grit)
    assert settled == grit.replace('\tdelay\t\t\t\t0 250', '\tdelay\t\t\t\t180 250').replace(
        '\t\tscale\t\t\t100\r\n\t}\r\n\r\n\tsizeRand', '\t\tscale\t\t\t50\r\n\t}\r\n\r\n\tsizeRand'), \
        'grit waits out the flash and shrinks, its random size alone'
    late = grit.replace('0 250', '190 260').replace('scale\t\t\t100\r\n\t}\r\n\r\n\tsizeRand', 'scale\t\t\t78.33 88.33'
                                                                                               '\r\n\t}\r\n\r\n\tsizeRand')
    assert module.settle_grit('Particle', late) == late.replace('78.33 88.33', '39.165 44.165'), 'later grit only shrinks'
    assert module.settle_grit('Tail', grit) == settled, 'streaks of grit too'
    assert module.settle_grit('Emitter', grit) == grit, 'models of debris stay'
    crater = '\r\n'.join(['Decal', '{', '\tname\t\t\t\tblast_decal', '', '\tlife\t\t\t\t4e+004', '', '\tdelay\t\t\t\t100', '',
                          '\tshaders', '\t[', '\t\tgfx_crater_dirt', '\t]', '}'])
    assert module.settle_grit('Decal', crater) == crater.replace('\tdelay\t\t\t\t100', '\tdelay\t\t\t\t250'), 'crater later'
    undated = crater.replace('\tdelay\t\t\t\t100\r\n\r\n', '')
    assert module.settle_grit('Decal', undated) == undated.replace(
        '\tname\t\t\t\tblast_decal\r\n', '\tname\t\t\t\tblast_decal\r\n\r\n\tdelay\t\t\t\t250\r\n'), 'a delay added'
    assert module.settle_grit('Decal', crater.replace('\tdelay\t\t\t\t100', '\tdelay\t\t\t\t300')) == \
        crater.replace('\tdelay\t\t\t\t100', '\tdelay\t\t\t\t300'), 'a later crater as it is'
    assert module.settle_grit('Particle', grit.replace('gfx_dirtchunks', 'gfx_rockdebri_01')) == \
        grit.replace('gfx_dirtchunks', 'gfx_rockdebri_01'), 'other debris stays'
    fire = parsed_effect(module, module.fire_effect().replace('\n', '\r\n'), cases)
    assert list(fire) == ['flash', 'fireball', 'sparks', 'embers', 'light']
    assert [kind for kind, *_ in fire.values()] == ['Particle', 'Particle', 'Tail', 'Particle', 'Light']
    for name, (kind, fields, channels, lists) in fire.items():
        assert 'useAlpha' not in fields.get('flags', '').split(), (name, 'added light fades by its color')
        if kind != 'Light':
            assert lists['shaders'] and all(values(channels[c][0])[-1] == 0 for c in ('alpha', 'alphaRand')), \
                (name, 'fades out')
            red, green, blue = (values(channels['rgb'][0], column) for column in (1, 2, 3))
            assert all(r >= g >= b for r, g, b in zip(red, green, blue)), (name, 'warm')
    kind, fields, channels, lists = fire['flash']
    assert lists['shaders'] == [module.GLOW] and max(numbers(fields['life'])) <= 100, 'a flash of a tenth of a second'
    assert values(channels['alpha'][0])[0] == max(values(channels['alpha'][0])) == 1, 'at full strength at once'
    kind, fields, channels, lists = fire['fireball']
    assert lists['shaders'] == [module.FIREBALL]
    assert (fields['sequenceStartFrameMode'], fields['sequencePlayRateMode'], fields['sequenceLoopMode'],
            fields['sequenceLoopTimes']) == ('0', '1', '1', '0'), 'its frames once over its life, holding the last'
    assert fields['sequenceFixedFrameValue'] == '1' and 300 <= min(numbers(fields['life'])) <= max(
        numbers(fields['life'])) <= 600
    assert min(numbers(fields['count'])) >= 4 and min(numbers(fields['gravity'])) > 0, 'several balls of fire, rising'
    assert 'orgOnSphere' in fields['spawnFlags'] and 'axisFromSphere' in fields['spawnFlags'], 'rolling outward'
    for channel in ('velocityX', 'velocityXRand'):
        speed = values(channels[channel][0])
        assert speed == sorted(speed, reverse=True) and speed[0] > 2 * speed[-1] > 0, (channel, 'slowing')
    for channel in ('size', 'sizeRand'):
        size = values(channels[channel][0])
        assert size == sorted(size) and size[-1] >= 2 * size[0], (channel, 'swelling')
    spin = values(channels['rotationDelta'][0]) + values(channels['rotationDeltaRand'][0])
    assert min(spin) < 0 < max(spin) and 'useRandomRotationDelta' in fields['flags'], 'turning either way'
    for name in ('sparks', 'embers'):
        kind, fields, channels, lists = fire[name]
        assert lists['shaders'] == ['gfx_spark_atlas'] and max(numbers(fields['gravity'])) < 0, (name, 'falling')
        assert (fields['sequenceStartFrameMode'], fields['sequenceFixedFpsValue']) == ('1', '0.000000')
        assert min(values(channels['velocityX'][0]) + values(channels['velocityXRand'][0])) > 0, (name, 'thrown up')
        for axis in 'YZ':
            assert values(channels[f'velocity{axis}'][0])[0] == -values(channels[f'velocity{axis}Rand'][0])[0] != 0, \
                (name, 'to every side')
    assert max(numbers(fire['sparks'][1]['life'])) < min(numbers(fire['embers'][1]['life'])), 'embers outlast sparks'
    kind, fields, channels, lists = fire['light']
    assert 'shaders' not in lists and max(numbers(fields['life'])) <= 300
    assert float(channels['size'][1]) >= 300 and values(channels['size'][0])[-1] == 0, 'a broad light that dies out'
    red, green, blue = (values(channels['rgb'][0], column) for column in (1, 2, 3))
    assert all(r >= g >= b for r, g, b in zip(red, green, blue)) and blue[-1] < blue[0], 'warm, reddening'
    smokes = {}
    for surface in SURFACES:
        text = module.smoke_effect(module.SURFACES[surface].dust).replace('\n', '\r\n')
        smokes[surface] = parsed_effect(module, text, cases)
        smoke = smokes[surface]
        assert list(smoke) == ['burst', 'plume', 'dust_ring', 'haze']
        for name, (kind, fields, channels, lists) in smoke.items():
            assert kind == 'Particle' and lists['shaders'] == [module.SMOKE_TEXTURE]
            assert 'useAlpha' in fields['flags'].split(), (name, 'blended smoke fades by its alpha, not to black')
            assert (fields['sequenceStartFrameMode'], fields['sequenceFixedFpsValue']) == ('1', '0.000000')
            assert min(numbers(fields['gravity'])) > 0, (name, 'climbing')
            for channel in ('alpha', 'alphaRand'):
                alpha = values(channels[channel][0])
                assert alpha[0] == 0 < max(alpha) and alpha[-1] == 0, (name, channel, 'fades in and out')
            if name != 'haze':
                for channel in ('velocityX', 'velocityXRand'):
                    speed = values(channels[channel][0])
                    assert speed == sorted(speed, reverse=True) and speed[0] > 4 * speed[-1], (name, 'billows, slows')
        kind, fields, channels, lists = smoke['burst']
        assert min(numbers(fields['life'])) >= 2000 and max(values(channels['alpha'][0])) >= 0.5, 'thick, seconds long'
        assert min(numbers(smoke['haze'][1]['life'])) >= 4000, 'a haze left drifting'
        kind, fields, channels, lists = smoke['dust_ring']
        assert 'orgOnCylinder' in fields['spawnFlags'] and fields['height'] == '0', 'over the ground'
        dust = tuple(values(channels['rgb'][0], column)[0] for column in (1, 2, 3))
        assert dust == module.SURFACES[surface].dust, (surface, 'its surface dust')
        lit = tuple(values(smoke['burst'][2]['rgb'][0], column)[0] for column in (1, 2, 3))
        dark = tuple(values(smoke['burst'][2]['rgb'][0], column)[1] for column in (1, 2, 3))
        assert lit[0] > lit[1] > lit[2] and luminance(lit) > luminance(dark), 'lit orange by the fire as it forms'
    for surface in SURFACES:
        assert {name: (fields, {c: k for c, k in channels.items() if not c.startswith('rgb')})
                for name, (kind, fields, channels, lists) in smokes[surface].items()} == \
            {name: (fields, {c: k for c, k in channels.items() if not c.startswith('rgb')})
             for name, (kind, fields, channels, lists) in smokes['dirt'].items()}, 'surfaces differ in color only'
    shades = {surface: luminance(module.SURFACES[surface].dust) for surface in SURFACES}
    assert shades['snow'] > shades['concrete'] > shades['mud'] and shades['dirt'] > shades['mud'], shades
    return cases


def frames(module, name, width, height, pixels):
    frame_width, frame_height, columns, count, _ = module.TEXTURES[name]
    channels = 3 if name == module.FIREBALL else 4
    assert (width, height) == (frame_width * columns, frame_height * ((count + columns - 1) // columns))
    out = []
    for n in range(count):
        left, top = (n % columns) * frame_width, (n // columns) * frame_height
        out.append([pixels[((top + y) * width + left + x) * channels:((top + y) * width + left + x + 1) * channels]
                    for y in range(frame_height) for x in range(frame_width)])
    return frame_width, frame_height, out


def feathered_template(technique, name, image, rows, columns, state):
    strings = technique + b'\0'
    material = 0x64 + len(strings)
    strings += name + b'\0'
    picture = 0x64 + len(strings)
    strings += image + b'\0'
    colour = 0x64 + len(strings)
    strings += b'colorMap\0'
    feather = 0x64 + len(strings)
    strings += b'featherParms\0'
    head = (struct.pack('<IIHHBBBBf', material, picture, 0, 0, 0, state[0], rows, columns, 0.0) +
            struct.pack('<BBHIHHfii', 0, 0, state[1], 0, 256, 256, 0.0, 0, 1) +
            struct.pack('<iiHHIII', 0x08128922, 4, 1, 1, 0x64, 0x44, 0x50) + struct.pack('<IBBBBI', colour, 2, 2, 0, 0, picture))
    return head + struct.pack('<Iffff', feather, 0.1, 40.0, 0.0, 0.0) + strings


TEMPLATES = {'materials/gfx_fire_ball': feathered_template(b'effect_zfeather_add_nofog', b'gfx_fire_ball', b'fire_ball',
                                                           1, 1, (0x1b, 0x60)),
             'materials/gfx_smokepuff_atlas': feathered_template(b'effect_zfeather', b'gfx_smokepuff_atlas',
                                                                 b'smokpuff_atlased', 4, 4, (0x1a, 0x20))}


def cstring(blob, offset):
    assert 0x64 <= offset < len(blob)
    return blob[offset:blob.index(b'\0', offset)].decode()


def verify_textures(module, mips, twice=False):
    """Textures, IWIs and materials; mips(width, height) is the production loader's level count less one."""
    source = module.__source__
    assert not LIBM.search(source[source.index('def fireball_frame('):source.index('def write_iwi_rgba(')]), \
        'textures only by correctly rounded operations'
    assert set(module.TEXTURES) == {module.FIREBALL, module.SMOKE_TEXTURE}
    generated = {name: module.texture_pixels(name) for name in module.TEXTURES}
    if twice:
        assert generated == {name: load(source).texture_pixels(name) for name in module.TEXTURES}, 'deterministic'
    module.texture_pixels = generated.__getitem__

    def read(name):
        if name in TEMPLATES:
            return TEMPLATES[name]
        surface = name.removeprefix('fx/explosions/grenadeexp_').removesuffix('.efx')
        return original(module, surface)[0].encode('latin-1')
    assets = module.grenade_explosion_assets(read)
    assert set(assets) == ({f'fx/explosions/grenadeexp_{s}.efx' for s in SURFACES} | {module.FIRE} |
                           {module.SMOKE.format(s) for s in SURFACES} |
                           {f'{kind}/{name}{suffix}' for name in module.TEXTURES
                            for kind, suffix in (('images', '.iwi'), ('materials', ''))})
    for name, (frame_width, frame_height, columns, count, template) in module.TEXTURES.items():
        width, height, pixels = generated[name]
        fw, fh, shapes = frames(module, name, width, height, pixels)
        for shape in shapes:
            ring = [shape[y * fw + x] for y in range(fh) for x in range(fw) if x in (0, fw - 1) or y in (0, fh - 1)]
            assert max(p[-1] if name == module.SMOKE_TEXTURE else max(p) for p in ring) <= 2, (name, 'clear edges')
        if name == module.FIREBALL:
            heat = [sum(sum(p) for p in shape) / len(shape) for shape in shapes]
            for a in range(1, len(shapes)):
                difference = sum(abs(x - y) for p, q in zip(shapes[a], shapes[a - 1]) for x, y in zip(p, q))
                assert difference / len(shapes[a]) >= 0.05 * heat[a - 1], (name, a, 'each frame moves on')
        else:
            for a in range(len(shapes)):
                for b in range(a):
                    difference = sum(abs(x - y) for p, q in zip(shapes[a], shapes[b]) for x, y in zip(p, q))
                    assert difference / (fw * fh * 4) >= 1, (name, a, b, 'shapes differ')
        if name == module.FIREBALL:
            assert max(p[0] for p in shapes[0]) >= 250 and max(p[2] for p in shapes[0]) >= 150, 'white-hot at first'
            assert all(p[0] >= p[1] >= p[2] for shape in shapes for p in shape), 'warm'
            peak = heat.index(max(heat))
            assert peak <= 6 and heat[peak:] == sorted(heat[peak:], reverse=True) and heat[-1] < 0.2 * heat[peak], \
                ('cooling', heat)
            assert max(p[1] for p in shapes[-1]) < max(p[1] for p in shapes[0]) // 2, 'red by its end'
        else:
            for shape in shapes:
                assert all(p[0] == p[1] == p[2] for p in shape), 'grey, tinted by the effects'
                alpha = [p[3] for p in shape]
                assert max(alpha) >= 200 and 0.15 <= sum(a > 8 for a in alpha) / len(alpha) <= 0.8, 'dense, soft'
                lit = [p[0] for p in shape if p[3] > 128]
                assert 100 <= min(lit) and max(lit) - min(lit) >= 60, 'shaded'
        iwi = assets['images/' + name + '.iwi']
        assert iwi[:6] == b'IWi\x05\x01\x00' and struct.unpack_from('<HHH', iwi, 6) == (width, height, 1)
        levels, w, h = [], width, height
        while True:
            levels.append((w, h))
            if w == 1 and h == 1:
                break
            w, h = max(1, w // 2), max(1, h // 2)
        assert len(levels) == mips(width, height) + 1, (name, 'the loader reads every level')
        assert len(iwi) == 28 + 4 * sum(w * h for w, h in levels) == struct.unpack_from('<i', iwi, 12)[0]
        top = iwi[len(iwi) - width * height * 4:]
        if name == module.FIREBALL:
            expected = b''.join(bytes((pixels[i + 2], pixels[i + 1], pixels[i], 255)) for i in range(0, len(pixels), 3))
        else:
            expected = b''.join(bytes((pixels[i + 2], pixels[i + 1], pixels[i], pixels[i + 3]))
                                for i in range(0, len(pixels), 4))
            assert 0 < iwi[31] < 255, 'the 1x1 level averages the smoke and its clear edges'
        assert top == expected, (name, 'the full level last, BGRA')
        material = assets['materials/' + name]
        source_material = TEMPLATES[template]
        info = struct.unpack_from('<IIHHBBBB', material, 0)
        assert cstring(material, info[0]) == cstring(material, info[1]) == name
        assert (info[6], info[7]) == ((count + columns - 1) // columns, columns), (name, 'atlas rows, columns')
        assert material[12:14] == source_material[12:14] and material[20:24] == source_material[20:24], 'its state'
        assert struct.unpack_from('<HH', material, 28) == (width, height)
        assert material[44:52] == source_material[44:52] and struct.unpack_from('<HH', material, 52) == (1, 1)
        technique, textures, constants = struct.unpack_from('<III', material, 56)
        assert cstring(material, technique) == cstring(source_material, struct.unpack_from('<I', source_material, 56)[0])
        assert (textures, constants) == (0x44, 0x50)
        texture_name, sampler, semantic, image = struct.unpack_from('<IBBxxI', material, 0x44)
        assert cstring(material, texture_name) == 'colorMap' and cstring(material, image) == name
        assert (sampler, semantic) == (2, 2)
        feather, *parameters = struct.unpack_from('<Iffff', material, 0x50)
        assert cstring(material, feather) == 'featherParms' and parameters == [struct.unpack('<f', struct.pack(
            '<f', 0.1))[0], 40.0, 0.0, 0.0], 'the template feather'
    narrow = module.write_iwi_rgba(8, 2, bytes(range(64)))
    assert mips(8, 2) + 1 == 4 and len(narrow) == 28 + 4 * (16 + 4 + 2 + 1), 'every level of a narrow image'
    try:
        module.feathered_material(TEMPLATES['materials/gfx_fire_ball'].replace(b'effect_zfeather_add_nofog',
                                                                              b'effect_add_nofog\0\0\0\0\0\0\0\0\0'),
                                  'x', 1, 1, 1, 1)
    except ValueError:
        pass
    else:
        raise AssertionError('a template without a feather is refused')
    return assets


EFFECT_MUTANTS = [
    ("('sequenceLoopMode', '1'), ('sequenceLoopTimes', '0'))", "('sequenceLoopMode', '0'), ('sequenceLoopTimes', '0'))"),
    ("('sequencePlayRateMode', '1'),", "('sequencePlayRateMode', '0'),"),
    ("return fields + (('sequenceStartFrameMode', '1'), ('sequenceFixedFpsValue', '0.000000'))",
     "return fields + (('sequenceStartFrameMode', '1'),)"),
    ("('rotation', '0 360'), ('gravity', '40 90'))),", "('rotation', '0 360'), ('gravity', '-40 90'))),"),
    ("('gravity', '-400 -250'))),", "('gravity', '-400 250'))),"),
    ("('rotation', '0 360'), ('gravity', '18 36'))),", "('rotation', '0 360'), ('gravity', '0 36'))),"),
    ("('name', 'flash'), ('flags', 'useRandomSize'), ('count', '1'), ('life', '70 100'),",
     "('name', 'flash'), ('flags', 'useRandomSize'), ('count', '1'), ('life', '70 160'),"),
    ("spin=_constant(-0.35), spin_rand=_constant(0.35)", "spin=_constant(0.35), spin_rand=_constant(0.35)"),
    ("velocity=(((0, 1), (0.3, 0.35), (1, 0.1)), _ZERO, _ZERO),", "velocity=(_constant(1), _ZERO, _ZERO),"),
    ("size=((0, 0.35), (0.15, 0.75), (0.5, 0.95), (1, 1)),", "size=_constant(1),"),
    ("velocity=(_constant(0.15), _constant(-1), _constant(-1)),", "velocity=(_constant(-0.15), _constant(-1), _constant(-1)),"),
    ("velocity_rand=(_constant(1), _constant(0.5), _constant(0.5)), velocity_scale=520),",
     "velocity_rand=(_constant(1), _constant(0.3), _constant(0.5)), velocity_scale=520),"),
    ("size=((0, 1), (0.25, 0.55), (0.6, 0.15), (1, 0)), size_scale=420)",
     "size=((0, 1), (0.25, 0.55), (0.6, 0.15), (1, 0.2)), size_scale=420)"),
    ("alpha=((0, 0), (0.08, 0.85), (0.4, 0.65), (1, 0))", "alpha=((0, 0.85), (0.4, 0.65), (1, 0))"),
    ("('life', '2800 4200')", "('life', '1400 4200')"),
    ("('life', '4500 6500')", "('life', '2500 6500')"),
    ("('radius', '16 24'),\n            ('height', '0'),", "('radius', '16 24'),\n            ('height', '20'),"),
    ("rgb=_constant_rgb(dust), rgb_rand", "rgb=_constant_rgb(dark), rgb_rand"),
    ("('name', 'haze'), ('flags', 'useAlpha useRandomSize useRandomVelocity relative'),",
     "('name', 'haze'), ('flags', 'useRandomSize useRandomVelocity relative'),"),
    ("cloud = ((0, *lit), (0.08, *dark),", "cloud = ((0, *dark), (0.08, *dark),"),
    ("'snow': Surface((0.88, 0.9, 0.93),", "'snow': Surface((0.38, 0.4, 0.43),"),
    ("'blast_decal', 'rockychunks_high',\n                                         'rockychunks_out', 'dirt_chunks')),",
     "'blast_decal', 'rockychunks_high',\n                                         'dirt_chunks')),"),
    ("    runners = '\\n'.join((_runner('browser_fire', FIRE), _runner('browser_smoke', SMOKE.format(surface))))",
     "    runners = _runner('browser_fire', FIRE)"),
    ("    newline = '\\r\\n' if '\\r\\n' in original else '\\n'\n    kept_names", "    newline = '\\n'\n    kept_names"),
    ("playfx='/' + effect.removesuffix('.efx'))", "playfx=effect)"),
    ("        raise ValueError(f'grenadeExp_{surface}: expected", "        ValueError(f'grenadeExp_{surface}: expected"),
    ("            (FIREBALL,)),", "            ('gfx_exp_fireball_atlas',)),"),
    ("GRIT_DELAY_MS = 180", "GRIT_DELAY_MS = 0"),
    ("GRIT_SIZE = 0.5", "GRIT_SIZE = 1"),
    ("CRATER_DELAY_MS = 250", "CRATER_DELAY_MS = 100"),
    ("    if kind == 'Decal':\n        return _delayed(element, CRATER_DELAY_MS)\n", ""),
    ("('name', 'light'), ('life', '160')", "('name', 'light'), ('life', '360')"),
    ("    if kind not in ('Particle', 'Tail') or GRIT not in element.split():", "    if GRIT not in element.split():"),
    ("    if kind not in ('Particle', 'Tail') or GRIT", "    if kind not in ('Particle',) or GRIT"),
    ("                    kept.append(settle_grit(kind, element))", "                    kept.append(element)"),
]
TEXTURE_MUTANTS = [
    ("    heat_scale = 1.0 - 0.72 * t", "    heat_scale = 1.0"),
    ("frames = [smoke_frame(401 + n, frame_width) for n in range(count)]",
     "frames = [smoke_frame(401, frame_width) for n in range(count)]"),
    ("            pixels.append((grey, grey, grey, alpha))", "            pixels.append((grey, grey, grey, 1.0))"),
    ("bytes((data[i + 2], data[i + 1], data[i], data[i + 3]))", "bytes((data[i + 2], data[i + 1], data[i], 255))"),
    ("            b''.join(reversed(blobs)))", "            b''.join(blobs))"),
    ("    while levels[-1][0] > 1 or levels[-1][1] > 1:\n        w, h, source = levels[-1]\n        half_w, half_h = max(1, w // 2), max(1, h // 2)\n        level = bytearray(half_w * half_h * 4)",
     "    while levels[-1][0] > 1 and levels[-1][1] > 1:\n        w, h, source = levels[-1]\n        half_w, half_h = max(1, w // 2), max(1, h // 2)\n        level = bytearray(half_w * half_h * 4)"),
    ("    head[14], head[15] = rows, columns\n    struct.pack_into('<HH', head, 0x1c, width, height)\n    struct.pack_into('<I', head, 0x38, 0x64)",
     "    head[14], head[15] = columns, 1\n    struct.pack_into('<HH', head, 0x1c, width, height)\n    struct.pack_into('<I', head, 0x38, 0x64)"),
    ("    struct.pack_into('<I', constant, 0, feather)\n", ""),
    ("    constant = bytearray(template[0x50:0x64])", "    constant = bytearray(struct.pack('<Iffff', 0, 1.0, 1.0, 0.0, 0.0))"),
    ("        assets['images/' + name + '.iwi'] = (write_iwi(width, height, pixels) if name == FIREBALL else\n                                             write_iwi_rgba(width, height, pixels))",
     "        assets['images/' + name + '.iwi'] = write_iwi_rgba(width, height, pixels)"),
    ("            rim = math.sqrt(dx * dx + dy * dy)\n            if rim >= 1:", "            rim = math.hypot(dx, dy)\n            if rim >= 1:"),
]

with tempfile.TemporaryDirectory(prefix='cod2-grenade-explosions-') as tmp:
    source, binary = Path(tmp) / 'parser.c', Path(tmp) / 'parser'
    source.write_text(harness)
    subprocess.run(['cc', '-std=c11', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    str(source), '-lm', '-o', str(binary)], check=True)

    def parse(cases):
        return subprocess.run([str(binary)], input='\n'.join(cases) + '\n', text=True, capture_output=True)

    mip_counts = {}

    def mips(width, height):
        if (width, height) not in mip_counts:
            result = parse([f'mips {width} {height}'])
            assert result.returncode == 0, result.stderr
            mip_counts[(width, height)] = int(result.stdout.split()[1])
        return mip_counts[(width, height)]

    cases = verify_effects(load(generator))
    result = parse(cases)
    assert result.returncode == 0, result.stderr
    flag_lines, curve_count = map(int, result.stdout.split())
    pristine = load(generator)
    textures = {name: pristine.texture_pixels(name) for name in pristine.TEXTURES}
    assets = verify_textures(load(generator), mips, twice=True)
    killed = 0
    for old, new in EFFECT_MUTANTS:
        assert generator.count(old) == 1, old
        try:
            mutant_cases = verify_effects(load(generator.replace(old, new)))
        except (AssertionError, KeyError, ValueError, IndexError):
            killed += 1
            continue
        if parse(mutant_cases).returncode != 0:
            killed += 1
            continue
        raise AssertionError('mutant survived: ' + new)
    for old, new in TEXTURE_MUTANTS:
        assert generator.count(old) == 1, old
        mutant = load(generator.replace(old, new))
        if 'def texture_pixels' not in old and 'frame(' not in old and 'pixels.append' not in old and \
                'heat_scale' not in old and 'rim = ' not in old and 'alpha = ' not in old:
            mutant.texture_pixels = textures.__getitem__
            mutant.__source__ = generator.replace(old, new)
        try:
            verify_textures(mutant, mips)
        except (AssertionError, ValueError, KeyError, IndexError, struct.error):
            killed += 1
            continue
        raise AssertionError('texture mutant survived: ' + new)
    print(f'PASS: {len(SURFACES)} grenade explosions rewritten around their debris; the fire, {len(SURFACES)} smokes '
          f'and 2 runners each; {flag_lines} flag lines and {curve_count} curves through the production EFX parser; two '
          f'deterministic textures, {sum(name.startswith("images/") for name in assets)} IWIs with their mip chains '
          f'and soft-edged materials')
    print(f'PASS: {killed} explosion mutants rejected')
