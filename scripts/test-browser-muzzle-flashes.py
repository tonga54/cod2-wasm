#!/usr/bin/env python3
"""Rewrite muzzle flashes with the production generator (browser_muzzle_flashes.py) and parse every generated flag
line and curve with the production EFX parser. Each original's lights and lasting smoke stay byte for byte and in
order; its fireball puffs and other brief elements give way to CoD WWII's glow, a flame core drawn from eight shapes,
fanning tongues, sparks and a puff of smoke, each shot drawing its own shape, size, proportion, strength and warmth.
The generated textures are deterministic, warm, edged in black and different frame to frame; their IWIs carry the
mip chain the production loader counts; their materials follow the engine's 32-bit layout. Mutants fail."""
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import types

root = Path(__file__).resolve().parent.parent
generator = (root / 'scripts/browser_muzzle_flashes.py').read_text()
template = (root / 'src/PC/EffectsCore/FxTemplate.c').read_text()
stubs = (root / 'src/stubs/link_stubs.c').read_text()
curve_source = (root / 'src/PC/EffectsCore/FxCurve_load_obj.c').read_text()
loader = (root / 'src/PC/EffectsCore/FxScheduler_load_obj.c').read_text()
image_loader = (root / 'src/PC/gfx_d3d/r_image_load_obj.c').read_text()

# The weapon files' view and world flashes (weapons/mp/*: viewFlashEffect, worldFlashEffect).
FLASH_FILES = {'heavy_view.efx', 'heavy.efx', 'mg42hv_view.efx', 'mg42hv.efx', 'pistolflash_view.efx',
               'pistolflash.efx', 'rifleflash.efx', 'shotgunflash.efx', 'standardflashview.efx',
               'standardflashworld.efx', 'thompson_view.efx', 'thompson.efx'}
PUFFS = {'gfx_exp_fireball_atlas', 'gfx_muzflash2', 'gfx_muzflash2a', 'gfx_fire_ball'}
PARSER_KEYS = {key.lower() for key in re.findall(r'stricmp\(key, "([^"]+)"\)', template)}
PRIMITIVES = {name.lower() for name in re.findall(r'\{ "(\w+)", PT_\w+ \}', loader)}
assert {'particle', 'tail', 'light'} <= PRIMITIVES and {'life', 'rgb', 'velocityx', 'nonuniformscale'} <= PARSER_KEYS
FAMILY_ORDER = ('pistol', 'smg', 'standard', 'heavy', 'rifle', 'mg', 'shotgun')
LIVES = {'glow': 75, 'core': 45, 'tongue': 40, 'sparks': 140, 'puff': 750}
# Calls whose last bit the platform's libm decides; the Pi must rebuild the textures byte for byte.
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


def varies(channels, base, rand, column=1):
    """The shot's random pick spans at least a fifth of the base at the start."""
    low, high = values(channels[base][0], column)[0], values(channels[rand][0], column)[0]
    return abs(high - low) >= 0.2 * max(abs(low), abs(high))


def verify_effects(module):
    """Every design check on the generated effects; the parser cases for the harness."""
    assert set(module.EFFECTS) == FLASH_FILES and set(module.EFFECTS.values()) <= set(module.FAMILIES)
    assert set(module.FAMILIES) == set(FAMILY_ORDER)
    cases = []
    for name, family in module.EFFECTS.items():
        text = module.rewrite_flash(ORIGINAL, family).decode('latin-1')
        assert '\n' not in text.replace('\r\n', ''), 'mixed line endings'
        elements = module.split_elements(text)
        assert [body for kind, body in elements[:len(KEPT)]] == KEPT, 'lights and smoke kept, verbatim, in order'
        for brief in BRIEF:
            assert brief not in text and brief.split('\r\n')[2] not in text, 'brief original element kept'
        f = module.FAMILIES[family]
        parsed = {}
        for kind, body in elements[len(KEPT):]:
            assert kind.lower() in PRIMITIVES
            fields, channels, shaders = parse_new(body)
            for key in list(fields) + list(channels):
                assert key.lower() in PARSER_KEYS, key
            assert len(shaders) == 1 and not set(shaders) & PUFFS
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
        assert list(parsed) == ['glow', 'core', 'tongue'] + ['sparks'] * bool(f.sparks[1]) + ['puff'] * bool(f.puff)
        for name, (kind, fields, channels, shader) in parsed.items():
            assert max(float(v) for v in fields['life'].split()) <= LIVES[name], (name, fields['life'])
            for channel in ('alpha', 'alphaRand'):
                alpha = values(channels[channel][0])
                assert alpha[-1] == 0, (name, channel, 'fades out')
                if name == 'puff':
                    assert alpha[0] == 0 < max(alpha) <= 0.2, (name, channel, 'thin smoke fading in')
                else:
                    assert alpha[0] == max(alpha) > 0, (name, channel, 'at full strength at once')
            red, green, blue = (values(channels['rgb'][0], column) for column in (1, 2, 3))
            assert all(r >= g >= b for r, g, b in zip(red, green, blue)), (name, 'warm')
            assert ('relative' in fields['flags'].split()) == (name != 'puff'), (name, 'follows the muzzle')
        kind, fields, channels, shader = parsed['glow']
        assert kind == 'Particle' and shader == module.GLOW and float(channels['size'][1]) == f.glow
        assert {'useRandomColors', 'useRandomAlpha', 'useRandomSize'} <= set(fields['flags'].split())
        assert varies(channels, 'alpha', 'alphaRand') and varies(channels, 'size', 'sizeRand')
        assert varies(channels, 'rgb', 'rgbRand', 3), 'each glow its own warmth'
        kind, fields, channels, shader = parsed['core']
        assert kind == 'Particle' and shader == module.BURST and fields['sequenceStartFrameMode'] == '1'
        assert fields['nonUniformScale'] == '1' and {'useRandomSize', 'useRandomSize2'} <= set(fields['flags'].split())
        assert float(channels['size'][1]) == float(channels['size2'][1]) == f.burst
        assert varies(channels, 'size', 'sizeRand') and varies(channels, 'size2', 'size2Rand'), 'its own proportion'
        assert fields['rotation'] == '180 -180' and varies(channels, 'rgb', 'rgbRand', 3)
        kind, fields, channels, shader = parsed['tongue']
        assert kind == 'Tail' and shader == module.TONGUE and fields['sequenceStartFrameMode'] == '1'
        assert fields['count'] == f'{f.tongues[0]} {f.tongues[1]}' and f.tongues[1] >= 1
        assert float(channels['length'][1]) == f.tongue_length and float(channels['size'][1]) == f.tongue_width
        assert max(values(channels['velocityX'][0]) + values(channels['velocityXRand'][0])) < 0, \
            'the tongue moves back, so its tail streaks forward out of the barrel'
        for axis in 'YZ':
            low, high = values(channels[f'velocity{axis}'][0])[0], values(channels[f'velocity{axis}Rand'][0])[0]
            assert low == -high != 0, 'tongues fan to either side'
        if f.sparks[1]:
            kind, fields, channels, shader = parsed['sparks']
            assert kind == 'Tail' and shader == 'gfx_spark_atlas' and fields['count'] == f'{f.sparks[0]} {f.sparks[1]}'
            assert min(values(channels['velocityX'][0]) + values(channels['velocityXRand'][0])) > 0
            assert float(channels['velocityX'][1]) == f.spark_speed, 'sparks fly forward'
        if f.puff:
            kind, fields, channels, shader = parsed['puff']
            assert kind == 'Particle' and shader == 'gfx_smokepuff_atlas' and fields['sequenceStartFrameMode'] == '1'
            assert float(channels['size'][1]) == f.puff and values(channels['size'][0])[0] < values(channels['size'][0])[-1]
            assert min(values(channels['velocityZ'][0]) + values(channels['velocityZRand'][0])) > 0, 'rises'
            assert min(values(channels['velocityX'][0]) + values(channels['velocityXRand'][0])) > 0, 'drifts forward'
    for field in ('glow', 'burst'):
        sizes = [getattr(module.FAMILIES[name], field) for name in FAMILY_ORDER]
        assert sizes == sorted(sizes) and sizes[0] < sizes[-1], field
    assert max(module.FAMILIES, key=lambda name: module.FAMILIES[name].tongue_length) == 'mg'
    return cases


def frames(module, name, width, height, rgb):
    frame_width, frame_height, columns, count = module.TEXTURES[name]
    assert (width, height) == (frame_width * columns, frame_height * ((count + columns - 1) // columns))
    out = []
    for n in range(count):
        left, top = (n % columns) * frame_width, (n // columns) * frame_height
        out.append([rgb[((top + y) * width + left + x) * 3:((top + y) * width + left + x) * 3 + 3]
                    for y in range(frame_height) for x in range(frame_width)])
    return frame_width, frame_height, out


MATERIAL_TEMPLATE = (struct.pack('<IIHHBBBBf', 0x61, 0x71, 0, 0, 0, 0x1b, 4, 4, 0.0) +
                     struct.pack('<BBHIHHfii', 0, 0, 0x60, 0, 256, 256, 0.0, 0, 1) +
                     struct.pack('<iiHHIII', 0x08128922, 4, 1, 0, 0x50, 0x44, 0x50) + struct.pack('<IBBBBI', 0x7d, 2, 2, 0, 0, 0x71) +
                     b'effect_add_nofog\0gfx_spark_atlas\0spark_atlas\0colorMap\0')
assert len(MATERIAL_TEMPLATE) == 134


def cstring(blob, offset):
    assert 0x50 <= offset < len(blob)
    return blob[offset:blob.index(b'\0', offset)].decode()


def verify_textures(module, mips, twice=False):
    """Textures, IWIs and materials; mips(width, height) is the production loader's level count less one."""
    assert not LIBM.search(generator_texture_source(module)), 'textures only by correctly rounded operations'
    assert set(module.TEXTURES) == {module.GLOW, module.BURST, module.TONGUE}
    generated = {name: module.texture_pixels(name) for name in module.TEXTURES}
    module.texture_pixels = generated.__getitem__
    assets = module.muzzle_flash_assets(lambda name: MATERIAL_TEMPLATE if name == module.MATERIAL_TEMPLATE
                                        else ORIGINAL.encode('latin-1'))
    if twice:
        assert generated == {name: load(module.__source__).texture_pixels(name) for name in module.TEXTURES}, \
            'deterministic'
    for name, (frame_width, frame_height, columns, count) in module.TEXTURES.items():
        width, height, rgb = generated[name]
        fw, fh, shapes = frames(module, name, width, height, rgb)
        for shape in shapes:
            ring = [shape[y * fw + x] for y in range(fh) for x in range(fw) if x in (0, fw - 1) or y in (0, fh - 1)]
            assert max(max(p) for p in ring) <= 2, (name, 'black at the frame edge')
            assert max(p[0] for p in shape) >= 250 and max(p[2] for p in shape) >= 120, (name, 'a white-hot heart')
            assert all(p[0] >= p[1] >= p[2] for p in shape), (name, 'warm')
        for a in range(len(shapes)):
            for b in range(a):
                difference = sum(abs(x - y) for p, q in zip(shapes[a], shapes[b]) for x, y in zip(p, q))
                assert difference / (fw * fh * 3) >= 4, (name, a, b, 'frames differ')
        if name == module.GLOW:
            row = shapes[0][(fh // 2) * fw + fw // 2:(fh // 2 + 1) * fw]
            assert all(sum(row[i]) >= sum(row[i + 1]) for i in range(len(row) - 1)), 'glow falls from its heart'
        if name == module.TONGUE:
            for shape in shapes:
                base = sum(sum(p) for p in shape[(fh - fh // 8) * fw:])
                tip = sum(sum(p) for p in shape[:fh // 8 * fw])
                assert base > 4 * tip, 'the base, at the bottom, outshines the tip'
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
        assert top == b''.join(bytes((rgb[i + 2], rgb[i + 1], rgb[i], 255)) for i in range(0, len(rgb), 3)), \
            'the full level last, BGRA'
        smallest = iwi[28:32]
        assert smallest[3] == 255 and smallest[2] >= smallest[1] >= smallest[0], 'the 1x1 level first, warm'
        material = assets['materials/' + name]
        info = struct.unpack_from('<IIHHBBBB', material, 0)
        assert cstring(material, info[0]) == cstring(material, info[1]) == name
        assert (info[6], info[7]) == ((count + columns - 1) // columns, columns), (name, 'atlas rows, columns')
        assert struct.unpack_from('<HH', material, 28) == (width, height)
        assert material[44:52] == MATERIAL_TEMPLATE[44:52], 'additive state of the template'
        assert struct.unpack_from('<HH', material, 52) == (1, 0)
        technique, textures = struct.unpack_from('<II', material, 56)
        assert cstring(material, technique) == 'effect_add_nofog' and textures == 0x44
        texture_name, sampler, semantic, image = struct.unpack_from('<IBBxxI', material, 0x44)
        assert cstring(material, texture_name) == 'colorMap' and cstring(material, image) == name
        assert (sampler, semantic) == (2, 2)
    try:
        module.effect_material(MATERIAL_TEMPLATE.replace(b'effect_add_nofog', b'2d\0\0\0\0\0\0\0\0\0\0\0\0\0\0'), 'x',
                               1, 1, 1, 1)
    except ValueError:
        pass
    else:
        raise AssertionError('a non-additive template is refused')
    return assets


def generator_texture_source(module):
    source = module.__dict__.get('__source__', '')
    return source[source.index('def _decay('):]


EFFECT_MUTANTS = [
    ("if kind.lower() == 'light' or life_max(element) > FLASH_LIFE_MS]", "if life_max(element) > FLASH_LIFE_MS]"),
    ('life_max(element) > FLASH_LIFE_MS]', 'life_max(element) >= 0]'),
    ('FLASH_LIFE_MS = 200', 'FLASH_LIFE_MS = 199'),
    ("r'^[ \\t]*life[ \\t]+([\\d.]+)(?:[ \\t]+([\\d.]+))?'", "r'^[ \\t]*life[ \\t]+([\\d.]+)'"),
    ('if start >= i:', 'if start > i:'),
    ("newline = '\\r\\n' if '\\r\\n' in original else '\\n'", "newline = '\\n'"),
    ("('life', '50 75')", "('life', '50 120')"),
    ("('life', '25 40')", "('life', '25 60')"),
    ("((0, 1), (0.3, 0.75), (0.7, 0.2), (1, 0))", "((0, 0.2), (0.3, 0.75), (0.7, 0.2), (1, 0))"),
    ("((0, 0.95), (0.35, 0.6), (1, 0))", "((0, 0.95), (0.35, 0.6), (1, 0.1))"),
    ("'useRandomColors useRandomAlpha useRandomSize relative'", "'useRandomAlpha useRandomSize relative'"),
    ("f.glow, rgb_rand=((0, 1, 0.84, 0.66), (1, 1, 0.68, 0.45))), GLOW)", "f.glow), GLOW)"),
    ("((0, 0.55), (0.12, 0.45), (0.45, 0.15), (1, 0))", "((0, 0.95), (0.12, 0.8), (0.45, 0.28), (1, 0))"),
    ("('nonUniformScale', '1'), ", ""),
    ("f.burst, ((0, 0.8), (1, 1.15)), ((0, 0.55), (1, 0.8)), f.burst,",
     "f.burst, ((0, 0.8), (1, 1.15)), ((0, 0.8), (1, 1.15)), f.burst,"),
    ("('rotation', '180 -180'), ('nonUniformScale', '1'), ('sequenceStartFrameMode', '1'),",
     "('rotation', '180 -180'), ('nonUniformScale', '1'), ('sequenceStartFrameMode', '0'),"),
    ("{'velocityX': (((0, -0.5), (1, -0.5)), 50)", "{'velocityX': (((0, 0.5), (1, 0.5)), 50)"),
    ("'velocityY': (((0, -0.08), (1, -0.08)), 50), 'velocityYRand': (((0, 0.08), (1, 0.08)), None)",
     "'velocityY': (((0, 0), (1, 0)), 50), 'velocityYRand': (((0, 0), (1, 0)), None)"),
    ("'velocityX': (((0, 1), (1, 1)), f.spark_speed)", "'velocityX': (((0, -1), (1, -1)), f.spark_speed)"),
    ("('name', 'puff'), ('flags', 'useAlpha useRandomAlpha useRandomSize useRandomVelocity'),",
     "('name', 'puff'), ('flags', 'useAlpha useRandomAlpha useRandomSize useRandomVelocity relative'),"),
    ("'velocityZ': (((0, 0.07), (1, 0.07)), 60)", "'velocityZ': (((0, -0.07), (1, -0.07)), 60)"),
    ("((0, 0), (0.08, 0.16), (1, 0))", "((0, 0.16), (0.08, 0.16), (1, 0))"),
    ("useRandomLength useRandomVelocity relative'),\n            ('count', f'{f.tongues[0]} {f.tongues[1]}')",
     "useRandomLength useRandomVelocty relative'),\n            ('count', f'{f.tongues[0]} {f.tongues[1]}')"),
    ("('spawnFlags', 'evenDistribution'), ('count', '1'), ('life', '30 45')",
     "('spawnFlags', 'evenDistributon'), ('count', '1'), ('life', '30 45')"),
    ("' '.join(_number(v) for v in key) for key in keys]", "' '.join(_number(v) for v in key[1:]) for key in keys]"),
    ("    'heavy': Family(14.0, 5.5,", "    'heavy': Family(18.0, 5.5,"),
    ("'thompson_view.efx': 'smg', ", ""),
]
TEXTURE_MUTANTS = [
    ("pixels.append(_flame(heat * (1 - _smoothstep(0.85, 1.0, r))))", "pixels.append(_flame(heat))"),
    ("        v = 1 - (y + 0.5) / height\n", "        v = (y + 0.5) / height\n"),
    ("frames = [burst_frame(101 + n, frame_width) for n in range(count)]",
     "frames = [burst_frame(101, frame_width) for n in range(count)]"),
    ("frames = [tongue_frame(201 + n, frame_width, frame_height) for n in range(count)]",
     "frames = [tongue_frame(201 + n % 2, frame_width, frame_height) for n in range(count)]"),
    ("            b''.join(reversed(blobs)))", "            b''.join(blobs))"),
    ("    while levels[-1][0] > 1 or levels[-1][1] > 1:", "    while levels[-1][0] > 1 and levels[-1][1] > 1:"),
    ("bytes((data[i + 2], data[i + 1], data[i], 255))", "bytes((data[i], data[i + 1], data[i + 2], 255))"),
    ("    head[14], head[15] = rows, columns\n", "    head[14], head[15] = columns, rows\n"),
    ("struct.pack_into('<HH', head, 0x1c, width, height)", "struct.pack_into('<HH', head, 0x1c, height, width)"),
    ("    y = 1.0 / (1.0 + x * 0.125)\n", "    return math.exp(-x)\n"),
    ("min(1.0, max(0.0, (heat - 0.55) * 1.6)) * 0.8))", "min(1.0, max(0.0, (heat - 0.3) * 1.6))))"),
]

with tempfile.TemporaryDirectory(prefix='cod2-muzzle-flashes-') as tmp:
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

    def module_of(text):
        module = load(text)
        module.__source__ = text
        return module

    cases = verify_effects(module_of(generator))
    result = parse(cases)
    assert result.returncode == 0, result.stderr
    flag_lines, curve_count = map(int, result.stdout.split())
    assets = verify_textures(module_of(generator), mips, twice=True)
    killed = 0
    for old, new in EFFECT_MUTANTS:
        assert generator.count(old) == 1, old
        try:
            mutant_cases = verify_effects(module_of(generator.replace(old, new)))
        except (AssertionError, KeyError, ValueError, IndexError):
            killed += 1
            continue
        if parse(mutant_cases).returncode != 0:
            killed += 1
            continue
        raise AssertionError('mutant survived: ' + new)
    for old, new in TEXTURE_MUTANTS:
        assert generator.count(old) == 1, old
        try:
            verify_textures(module_of(generator.replace(old, new)), mips)
        except (AssertionError, KeyError, ValueError, IndexError, struct.error):
            killed += 1
            continue
        raise AssertionError('mutant survived: ' + new)
print(f'PASS: {len(FLASH_FILES)} muzzle flashes rewritten; {flag_lines} flag lines and {curve_count} curves through the '
      f'production EFX parser; three deterministic textures with the loader\'s mip chains and their materials; '
      f'lights and smoke kept verbatim; {killed} mutants fail')
