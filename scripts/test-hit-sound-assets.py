#!/usr/bin/env python3
"""Exercise the owner's hit sound replacing MP_hit_alert in the private audio archive."""
from pathlib import Path
import contextlib
import io
import math
import struct
import subprocess
import tempfile
import types
import zipfile

root = Path(__file__).resolve().parent.parent
module_path = root / 'scripts/browser_sound_assets.py'
module_source = module_path.read_text()

HEADER = ['Name', 'sequence', 'file', 'vol_min', 'vol_max', 'vol_mod', 'pitch_min', 'pitch_max',
          'dist_min', 'dist_max', 'channel', 'type', 'probability', 'loop', 'masterslave', 'loadspec',
          'subtitle', 'compression', 'secondaryaliasname', 'volumefalloffcurve', 'startdelay', 'speakermap']


def row(**values):
    return [values.get(name.lower(), '') for name in HEADER]


STOCK_ROW = row(name='MP_hit_alert', file='misc\\mp_hit_indication_3c.wav', vol_min='1', vol_max='1',
                pitch_min='1', pitch_max='1', channel='auto2d', loadspec='all_mp',
                secondaryaliasname='mp_hit_alert_tail', startdelay='35')
TAIL_ROW = row(name='mp_hit_alert_tail', file='misc/tail.wav', vol_min='1', vol_max='1', channel='auto2d',
               loadspec='all_mp')
FIRE_ROW = row(name='weap_test_fire', file='weapons/test_fire.wav', vol_min='0.9', vol_max='1', pitch_min='1',
               pitch_max='1', dist_min='10', dist_max='1000', channel='weapon', loadspec='all_mp')
OTHER_MAP_ROW = row(name='weap_test_fire', file='weapons/other_map.wav', channel='weapon', loadspec='mp_other')


def csv_text(rows):
    return '\r\n'.join(','.join(values) for values in rows) + '\r\n'


def wav(samples, rate=44100, channels=1, bits=16, tag=1, align=None, extra=b'', declared=None):
    data = struct.pack(f'<{len(samples)}h', *samples)
    align = channels * bits // 8 if align is None else align
    fmt = struct.pack('<HHIIHH', tag, channels, rate, rate * align, align, bits)
    size = len(data) if declared is None else declared
    body = (b'WAVE' + b'fmt ' + struct.pack('<I', len(fmt)) + fmt + extra +
            b'data' + struct.pack('<I', size) + data)
    return b'RIFF' + struct.pack('<I', len(body)) + body


def click(lead_ms=0, attack_ms=1, rate=44100, channels=1, length_ms=80):
    """Silence, a linear rise from a tenth of the peak to it, then a decaying tone."""
    lead, attack = rate * lead_ms // 1000, max(1, rate * attack_ms // 1000)
    samples = [0] * lead
    for index in range(rate * length_ms // 1000):
        envelope = 0.1 + 0.9 * index / attack if index < attack else math.exp(-(index - attack) / (rate * 0.01))
        samples.append(round(20000 * envelope * (1 if index % 8 < 4 else -1)))
    return [value for value in samples for _ in range(channels)]


def sound(samples, rate=44100, channels=1, extra=b''):
    return wav(samples, rate, channels, extra=extra), rate, channels, samples


def packaged(data, rate, channels, samples):
    """What the archive holds: a stereo sound as it is, a mono one with both channels the same."""
    return data if channels == 2 else wav([value for value in samples for _ in (0, 1)], rate, 2)


STOCK_WAV = wav([0, 9000, -9000, 4000] * 300)
TAIL_WAV = wav([0, 100, -100] * 50)
FIRE_WAV = wav([0, 12000, -12000] * 400)
OWNER_SOUND = sound(click())
OWNER, PACKAGED_OWNER = OWNER_SOUND[0], packaged(*OWNER_SOUND)

accepted = {
    'mono 44.1 kHz': OWNER_SOUND,
    'stereo 48 kHz, 40 ms of silence': sound(click(40, rate=48000, channels=2), 48000, 2),
    'a tenth of the peak at once, the peak after 60 ms': sound(click(0, 60)),
    'mono 11025 Hz with a padded LIST chunk': sound(click(rate=11025), 11025,
                                                    extra=b'LIST' + struct.pack('<I', 3) + b'abc\0'),
}
rejected = {
    'empty': b'',
    'not RIFF': b'RIFX' + OWNER[4:],
    'not WAVE': OWNER[:8] + b'AVI ' + OWNER[12:],
    'no format': b'RIFF' + struct.pack('<I', 12) + b'WAVE' + b'data' + struct.pack('<I', 0),
    'no data': OWNER[:36],
    '12-bit': wav(click(), bits=12, align=2),
    '24-bit': wav(click(), bits=24),
    'float': wav(click(), bits=32, tag=3),
    'three channels': wav(click(channels=3), channels=3),
    '8000 Hz': wav(click(rate=8000), 8000),
    '96000 Hz': wav(click(rate=96000), 96000),
    'wrong block align': wav(click(), align=4),
    'truncated data': wav(click(), declared=len(click()) * 2 + 100),
    'partial stereo frame': wav(click(channels=2)[:-1], channels=2),
    'silent': wav([0] * 4410),
    '60 ms of silence': wav(click(60)),
    'over 2 s': wav(click(length_ms=2010)),
}

# The engine's own WAV parser must load every sound the check accepts.
engine = (root / 'downstream/wasm/web_audio.c').read_text()
engine = engine[engine.index('static unsigned web_u16'):engine.index('static WebSound *web_new')]
harness = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte;
typedef struct {int format;const void *data_ptr;unsigned data_len,rate;int bits,channels;unsigned samples,block_size;const void *initial_ptr;} AILSOUNDINFO;
''' + engine + r'''
int main(int argc,char **argv){
 for(int a=1;a<argc;a+=4){
  FILE *f=fopen(argv[a],"rb");assert(f);fseek(f,0,SEEK_END);int n=ftell(f);rewind(f);
  byte *b=malloc(n);assert(fread(b,1,n,f)==(size_t)n);fclose(f);
  AILSOUNDINFO info;assert(WebAudio_WavInfo(b,n,&info));
  assert(info.rate==(unsigned)atoi(argv[a+1])&&info.channels==atoi(argv[a+2])&&info.bits==16);
  assert(info.samples==(unsigned)atoi(argv[a+3]));
  free(b);
 }
 return 0;
}
'''


def load(source):
    module = types.ModuleType('browser_sound_assets')
    exec(compile(source, str(module_path), 'exec'), module.__dict__)
    return module


def raises(error, call):
    try:
        call()
    except error:
        return
    raise AssertionError(f'expected {error.__name__}')


def check(m, archive_path):
    with contextlib.redirect_stdout(io.StringIO()):
        checks(m, archive_path)


def checks(m, archive_path):
    for name, accepted_sound in accepted.items():
        result = m.package_hit_sound(accepted_sound[0])
        assert result == packaged(*accepted_sound) and (result is accepted_sound[0]) == (accepted_sound[2] == 2), name
    for name, data in rejected.items():
        raises(ValueError, lambda: m.package_hit_sound(data))

    original = zipfile.ZipFile(archive_path)
    try:
        index = {info.filename.lower(): (archive_path, info.filename) for info in original.infolist()}
        originals = {archive_path: original}
        renderer = {'maps/mp/gametypes/_damagefeedback.gsc': b'\tself playlocalsound("MP_hit_alert");\n',
                    'weapons/mp/test_mp': b'WEAPONFILE\\fireSound\\weap_test_fire'}
        maps = {'mp_toujane': 'Toujane, Tunisia'}
        header, rows = m.alias_table(original.read('soundaliases/test_mp.csv').decode())
        hit_rows = [(header, dict(zip(header, values))) for values in rows if values[0].lower() == 'mp_hit_alert']

        # Without the owner's file, the archive keeps the retail alias.
        stock = m.collect_sound_assets(root, index, originals, renderer, maps)
        assert m.HIT_SOUND_TABLE not in stock and 'sound/' + m.HIT_SOUND_FILE not in stock
        _, rows = m.alias_table(stock['soundaliases/test_mp.csv'].decode())
        assert sorted(rows) == sorted([STOCK_ROW, TAIL_ROW, FIRE_ROW])
        assert stock['sound/misc/mp_hit_indication_3c.wav'] == STOCK_WAV
        assert stock['sound/misc/tail.wav'] == TAIL_WAV
        m.verify_hit_sound(stock, hit_rows, None)

        # With it, the same alias plays it, loaded, at once and alone.
        owned = m.collect_sound_assets(root, index, originals, renderer, maps, OWNER)
        assert owned['sound/' + m.HIT_SOUND_FILE] == PACKAGED_OWNER
        _, rows = m.alias_table(owned['soundaliases/test_mp.csv'].decode())
        assert rows == [FIRE_ROW]
        table_header, rows = m.alias_table(owned[m.HIT_SOUND_TABLE].decode())
        assert table_header == [name.lower() for name in HEADER] and len(rows) == 1
        assert dict(zip(table_header, rows[0])) == dict(zip(table_header, row(
            name='MP_hit_alert', file='browser/hitmarker.wav', vol_min='1', vol_max='1', pitch_min='1',
            pitch_max='1', channel='auto2d', loadspec='all_mp')))
        assert m.HIT_SOUND_TABLE.startswith('soundaliases/') and m.HIT_SOUND_TABLE.endswith('.csv')
        assert 'sound/' + m.HIT_SOUND_FILE in owned and m.HIT_SOUND_FILE == rows[0][2]
        replaced = {'soundaliases/test_mp.csv', 'sound/misc/mp_hit_indication_3c.wav', 'sound/misc/tail.wav'}
        assert {name: data for name, data in stock.items() if name not in replaced} == \
            {name: data for name, data in owned.items()
             if name not in {'soundaliases/test_mp.csv', m.HIT_SOUND_TABLE, 'sound/' + m.HIT_SOUND_FILE}}
        m.verify_hit_sound(owned, hit_rows, OWNER)

        # The checker accepts exactly that.
        delayed = dict(owned)
        delayed[m.HIT_SOUND_TABLE] = owned[m.HIT_SOUND_TABLE].replace(b',all_mp,,,,,,', b',all_mp,,,,,35,')
        assert delayed[m.HIT_SOUND_TABLE] != owned[m.HIT_SOUND_TABLE]
        doubled = dict(owned)
        doubled[m.HIT_SOUND_TABLE] = owned[m.HIT_SOUND_TABLE] + owned[m.HIT_SOUND_TABLE].split(b'\r\n', 1)[1]
        kept = dict(owned)
        kept['soundaliases/test_mp.csv'] = stock['soundaliases/test_mp.csv']
        swapped = dict(owned)
        swapped['sound/' + m.HIT_SOUND_FILE] = wav(click(attack_ms=2))
        unpackaged = dict(owned)
        unpackaged['sound/' + m.HIT_SOUND_FILE] = OWNER
        missing = {name: data for name, data in owned.items() if name != m.HIT_SOUND_TABLE}
        for package, owner in ((delayed, OWNER), (doubled, OWNER), (kept, OWNER), (swapped, OWNER),
                               (unpackaged, OWNER), (missing, OWNER), (stock, OWNER), (owned, None)):
            raises(AssertionError, lambda: m.verify_hit_sound(package, hit_rows, owner))
        raises(ValueError, lambda: m.verify_hit_sound(owned, hit_rows, rejected['60 ms of silence']))

        # It refuses an owner sound it cannot place or play promptly.
        raises(ValueError, lambda: m.collect_sound_assets(root, index, originals, {}, maps, OWNER))
        raises(ValueError, lambda: m.collect_sound_assets(
            root, index, originals, renderer, maps, rejected['60 ms of silence']))
    finally:
        original.close()


with tempfile.TemporaryDirectory(prefix='cod2-hit-sound-') as directory:
    folder = Path(directory)
    archive_path = folder / 'iw_00.iwd'
    with zipfile.ZipFile(archive_path, 'w') as archive:
        archive.writestr('soundaliases/test_mp.csv', csv_text(
            [HEADER, ['# retail comment'], [], STOCK_ROW, TAIL_ROW, FIRE_ROW, OTHER_MAP_ROW]))
        archive.writestr('sound/misc/mp_hit_indication_3c.wav', STOCK_WAV)
        archive.writestr('sound/misc/tail.wav', TAIL_WAV)
        archive.writestr('sound/weapons/test_fire.wav', FIRE_WAV)
        archive.writestr('sound/weapons/other_map.wav', FIRE_WAV)

    check(load(module_source), archive_path)

    (folder / 'test.c').write_text(harness)
    subprocess.run(['cc', '-O1', '-g', '-fsanitize=address,undefined', str(folder / 'test.c'),
                    '-o', str(folder / 'test')], check=True)
    # The owner's files and the archive's copies of them.
    arguments = [str(folder / 'test')]
    for number, accepted_sound in enumerate(accepted.values()):
        data, rate, channels, samples = accepted_sound
        for suffix, content, count in (('owner', data, channels), ('packaged', packaged(*accepted_sound), 2)):
            path = folder / f'{number}-{suffix}.wav'
            path.write_bytes(content)
            arguments += [str(path), str(rate), str(count), str(len(samples) // channels)]
    subprocess.run(arguments, check=True)

    mutants = [
        ("values = dict(fields, file=HIT_SOUND_FILE, startdelay='', secondaryaliasname='')",
         "values = dict(fields, file=HIT_SOUND_FILE, secondaryaliasname='')"),
        ("values = dict(fields, file=HIT_SOUND_FILE, startdelay='', secondaryaliasname='')",
         "values = dict(fields, file=HIT_SOUND_FILE, startdelay='')"),
        ("secondaryaliasname='')\n    return [values.get(name, '') for name in header]",
         "secondaryaliasname='')\n    return [values.get(name, '') for name in sorted(header)]"),
        ("                replaced.append((tables[table][0], fields))\n                continue\n",
         "                replaced.append((tables[table][0], fields))\n"),
        ("if hit_sound is not None and alias == HIT_SOUND_ALIAS:", "if hit_sound is not None:"),
        ("        if not replaced:\n", "        if False:\n"),
        ("files['sound/' + HIT_SOUND_FILE] = package_hit_sound(hit_sound)", "files[HIT_SOUND_FILE] = package_hit_sound(hit_sound)"),
        ("files['sound/' + HIT_SOUND_FILE] = package_hit_sound(hit_sound)", "files['sound/' + HIT_SOUND_FILE] = hit_sound"),
        ("    if channels == 2:\n        return data\n", "    if channels:\n        return data\n"),
        ("stereo[byte::4] = stereo[byte + 2::4] = samples[byte::2]", "stereo[byte::4] = samples[byte::2]"),
        ("struct.pack('<HHIIHH', 1, 2, rate, 4 * rate, 4, 16)", "struct.pack('<HHIIHH', 1, 2, rate, 2 * rate, 4, 16)"),
        ("    if onset * 20 > rate:", "    if onset * 2 > rate:"),
        ("abs(value) * 10 >= peak", "abs(value) >= peak"),
        ("tag != 1 or bits != 16 or ", "tag != 1 or "),
        ("not 11025 <= rate <= 48000", "not 1 <= rate <= 480000"),
        ("if len(samples) // align > 2 * rate:", "if len(samples) // align > 20 * rate:"),
        ("        if size > len(data) - start:\n            raise invalid('truncated chunk')\n", ""),
        ("assert table is None and sound is None", "assert True"),
        ("assert sound == package_hit_sound(owner)", "assert sound is not None"),
        ("assert sound == package_hit_sound(owner)", "assert sound == owner"),
        ("dict(zip(other_header, row)).get('name', '').lower() != HIT_SOUND_ALIAS", "row is not None"),
    ]
    for old, new in mutants:
        assert module_source.count(old) == 1, old
        try:
            check(load(module_source.replace(old, new)), archive_path)
        except (AssertionError, ValueError, IndexError, KeyError):
            continue
        raise AssertionError('mutant survived: ' + new)

print(f'PASS: without the owner file the retail MP_hit_alert stays; with it the alias plays only the owner\'s '
      f'loaded sound at once, a mono one as stereo with both channels the same, the rest of the archive unchanged; '
      f'{len(accepted)} accepted and {len(rejected)} rejected WAVs, the accepted ones and their packaged copies read '
      f'alike by the engine parser under ASan/UBSan; the checker rejects 9 altered packages; all {len(mutants)} '
      f'mutants fail')
