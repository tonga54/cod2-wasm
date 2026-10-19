#!/usr/bin/env python3
"""Exercise the owner's sound pack replacing original alias rows in the private audio archive."""
from pathlib import Path
import contextlib
import io
import struct
import subprocess
import tempfile
import types
import zipfile

from browser_explosives import ALIASES as EXPLOSIVE_ALIASES

root = Path(__file__).resolve().parent.parent
module_path = root / 'scripts/browser_sound_assets.py'
module_source = module_path.read_text()

HEADER = ['Name', 'sequence', 'file', 'vol_min', 'vol_max', 'vol_mod', 'pitch_min', 'pitch_max',
          'dist_min', 'dist_max', 'channel', 'type', 'probability', 'loop', 'masterslave', 'loadspec',
          'subtitle', 'compression', 'secondaryaliasname', 'volumefalloffcurve', 'startdelay', 'speakermap']
LOWER = [name.lower() for name in HEADER]


def row(**values):
    return [values.get(name.lower(), '') for name in HEADER]


FIRE_A = row(name='weap_test_fire', file='weapons\\test_fire_a.wav', vol_min='0.6', vol_max='0.7', vol_mod='wpnai',
             pitch_min='1.3', pitch_max='1.4', dist_min='35', dist_max='8400', channel='weapon', loadspec='all_mp',
             secondaryaliasname='weap_test_layer', probability='10')
FIRE_B = row(name='weap_test_fire', file='weapons/test_fire_b.wav', vol_min='0.62', vol_max='0.72', vol_mod='wpnai',
             pitch_min='1.2', pitch_max='1.3', dist_min='35', dist_max='8400', channel='weapon', loadspec='all_mp',
             secondaryaliasname='weap_test_layer', probability='10')
LAYER = row(name='weap_test_layer', file='weapons/test_layer.wav', vol_min='0.5', vol_max='0.5', channel='weapon',
            loadspec='all_mp')
RELOAD = row(name='weap_test_reload', file='weapons/test_reload.wav', vol_min='0.83', vol_max='1', channel='music',
             loadspec='all_mp', startdelay='300', pitch_min='0.9', pitch_max='0.9', sequence='7')
KEPT = row(name='weap_test_kept', file='weapons/test_fire_a.wav', vol_min='1', vol_max='1', channel='auto',
           loadspec='all_mp')
OTHER_MAP = row(name='weap_test_other', file='weapons/other_map.wav', channel='weapon', loadspec='mp_other')
UNUSED = row(name='weap_test_unused', file='weapons/unused.wav', channel='weapon', loadspec='all_mp')
HIT = row(name='MP_hit_alert', file='misc/hit.wav', vol_min='1', vol_max='1', channel='auto2d', loadspec='all_mp')
# The retail rows the explosives' aliases start from, in a table of their own.
GRENADE = {source: row(name=source, file=f'weapons/{source}.wav', vol_min='0.8', vol_max='0.9', channel='auto',
                       loadspec='all_mp', dist_max='1250')
           for source in sorted(set(EXPLOSIVE_ALIASES.values()))}


def csv_text(rows):
    return '\r\n'.join(','.join(values) for values in rows) + '\r\n'


def wav(samples, rate=48000, channels=1, bits=16, tag=1, align=None, declared=None, extra=b''):
    data = struct.pack(f'<{len(samples)}h', *samples)
    align = channels * bits // 8 if align is None else align
    fmt = struct.pack('<HHIIHH', tag, channels, rate, rate * align, align, bits)
    size = len(data) if declared is None else declared
    body = b'WAVE' + b'fmt ' + struct.pack('<I', len(fmt)) + fmt + extra + b'data' + struct.pack('<I', size) + data
    return b'RIFF' + struct.pack('<I', len(body)) + body


def tone(count, step=7, channels=1):
    return [((i * step) % 2000 - 1000) * 16 for i in range(count) for _ in range(channels)]


RETAIL = {'sound/weapons/test_fire_a.wav': wav(tone(400), 44100),
          'sound/weapons/test_fire_b.wav': wav(tone(410), 44100),
          'sound/weapons/test_layer.wav': wav(tone(300), 22050),
          'sound/weapons/test_reload.wav': wav(tone(900), 44100),
          'sound/weapons/other_map.wav': wav(tone(100)),
          'sound/weapons/unused.wav': wav(tone(100)),
          'sound/misc/hit.wav': wav(tone(200)),
          **{f'sound/weapons/{source}.wav': wav(tone(150 + number)) for number, source in enumerate(GRENADE)}}
OWNER = {'w/fire_1.wav': wav(tone(4800)),
         'w/fire_2.wav': wav(tone(4810, 9)),
         'w/fire_3.wav': wav(tone(2400, 5, 2), 48000, 2),
         'w/reload.wav': wav(tone(1600, 3), 32000),
         'g/layer.wav': wav(tone(1200, 11), 11025)}
LISTING = ('alias,file,pitch_min,pitch_max,vol_max\n'
           '# the owner\'s comment\n'
           'WEAP_TEST_FIRE,w/fire_1.wav,0.97,1.03,\n'
           'weap_test_fire,w\\fire_2.wav,0.97,1.03,\n'
           'weap_test_fire,w/fire_3.wav,0.97,1.03,\n'
           'weap_test_layer,w/fire_1.wav,,,\n'
           'weap_test_layer,g/layer.wav,,,0.6\n'
           'weap_test_reload,w/reload.wav,1,1,\n')

accepted = {
    'mono 48 kHz': wav(tone(4800)),
    'stereo 11025 Hz': wav(tone(1100, 3, 2), 11025, 2),
    'a padded LIST chunk': wav(tone(480), extra=b'LIST' + struct.pack('<I', 3) + b'abc\0'),
    'exactly 20 s at 11025 Hz': wav(tone(11025 * 20), 11025),
}
rejected = {
    'empty': b'',
    'not RIFF': b'RIFX' + accepted['mono 48 kHz'][4:],
    'not WAVE': accepted['mono 48 kHz'][:8] + b'AVI ' + accepted['mono 48 kHz'][12:],
    'no format': b'RIFF' + struct.pack('<I', 12) + b'WAVE' + b'data' + struct.pack('<I', 0),
    'no data': accepted['mono 48 kHz'][:36],
    'no samples': wav([]),
    '8-bit': wav(tone(480), bits=8, align=1),
    '24-bit': wav(tone(480), bits=24),
    'float': wav(tone(480), bits=32, tag=3),
    'a compressed 16-bit format': wav(tone(480), tag=2),
    'three channels': wav(tone(480, channels=3), channels=3),
    '8000 Hz': wav(tone(480), 8000),
    '96000 Hz': wav(tone(480), 96000),
    'wrong block align': wav(tone(480), align=4),
    'truncated chunk': wav(tone(480), declared=len(tone(480)) * 2 + 100),
    'partial stereo frame': wav(tone(480, channels=2)[:-1], channels=2),
    'over 20 s': wav(tone(11025 * 20 + 1), 11025),
}

# Each bad name names a file that exists, so only the name check can refuse it.
BAD_LISTINGS = {
    'no file column': (LISTING.replace('alias,file,', 'alias,path,'), {}, 'alias,file'),
    'file before alias': (LISTING.replace('alias,file,', 'file,alias,'), {}, 'alias,file'),
    'a name column': (LISTING.replace(',vol_max\n', ',name\n'), {}, 'alias,file'),
    'a column twice': (LISTING.replace(',vol_max\n', ',pitch_max\n'), {}, 'alias,file'),
    'a dotted name': (LISTING + 'weap_test_fire,w/fire.1.wav\n', {'w/fire.1.wav': OWNER['w/fire_1.wav']}, 'bad row'),
    'a parent path': (LISTING + 'weap_test_fire,../fire_1.wav\n', {'../fire_1.wav': OWNER['w/fire_1.wav']}, 'bad row'),
    'an absolute path': (LISTING + 'weap_test_fire,{absolute}\n', {}, 'bad row'),
    'not a WAV name': (LISTING + 'weap_test_fire,w/fire_1.mp3\n', {'w/fire_1.mp3': OWNER['w/fire_1.wav']}, 'bad row'),
    'a space': (LISTING + 'weap_test_fire,w/fire 1.wav\n', {'w/fire 1.wav': OWNER['w/fire_1.wav']}, 'bad row'),
    'a missing file': (LISTING + 'weap_test_fire,w/fire_9.wav\n', {}, 'missing'),
}
COUNTS = {}

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


def raises(error, call, message=''):
    try:
        call()
    except error as caught:
        assert message in str(caught), (message, str(caught))
        return
    raise AssertionError(f'expected {error.__name__}')


def write_pack(folder, listing=LISTING, files=OWNER):
    folder.mkdir(parents=True, exist_ok=True)
    (folder / 'aliases.csv').write_text(listing)
    for name, data in files.items():
        (folder / name).parent.mkdir(parents=True, exist_ok=True)
        (folder / name).write_bytes(data)
    return folder


def check(m, archive_path, folder):
    with contextlib.redirect_stdout(io.StringIO()):
        checks(m, archive_path, folder)


def owner_row(base, file, **values):
    fields = dict(zip(LOWER, base), file='browser/owner/' + file, **values)
    return [fields[name] for name in LOWER]


def checks(m, archive_path, folder):
    for name, data in accepted.items():
        assert m.check_owner_sound(data, 'x.wav') is data, name
    for name, data in rejected.items():
        raises(ValueError, lambda: m.check_owner_sound(data, 'x.wav'))

    assert m.load_owner_sounds(folder / 'none') is None
    pack = m.load_owner_sounds(write_pack(folder / 'pack'))
    assert pack['aliases'] == {
        'weap_test_fire': [('w/fire_1.wav', {'pitch_min': '0.97', 'pitch_max': '1.03'}),
                           ('w/fire_2.wav', {'pitch_min': '0.97', 'pitch_max': '1.03'}),
                           ('w/fire_3.wav', {'pitch_min': '0.97', 'pitch_max': '1.03'})],
        'weap_test_layer': [('w/fire_1.wav', {}), ('g/layer.wav', {'vol_max': '0.6'})],
        'weap_test_reload': [('w/reload.wav', {'pitch_min': '1', 'pitch_max': '1'})]}
    assert pack['files'] == OWNER
    for name, (listing, extra, message) in BAD_LISTINGS.items():
        target = folder / name.replace(' ', '_')
        listing = listing.replace('{absolute}', str(folder / 'pack/w/fire_1.wav'))
        raises(ValueError, lambda: m.load_owner_sounds(write_pack(target, listing, dict(OWNER, **extra))), message)
    raises(ValueError, lambda: m.load_owner_sounds(write_pack(
        folder / 'bad_wav', files=dict(OWNER, **{'w/fire_2.wav': rejected['float']}))))

    original = zipfile.ZipFile(archive_path)
    try:
        index = {info.filename.lower(): (archive_path, info.filename) for info in original.infolist()}
        originals = {archive_path: original}
        renderer = {'weapons/mp/test_mp': b'WEAPONFILE\\fireSound\\weap_test_fire\\reloadSound\\weap_test_reload'
                                          b'\\pickupSound\\weap_test_kept\\loopSound\\weap_test_other',
                    'maps/mp/gametypes/_damagefeedback.gsc': b'\tself playlocalsound("MP_hit_alert");\n'}
        maps = {'mp_toujane': 'Toujane, Tunisia'}
        header, rows = m.alias_table(original.read('soundaliases/test_mp.csv').decode())
        original_rows = {}
        for values in rows:
            fields = dict(zip(header, values))
            original_rows.setdefault(fields['name'].lower(), []).append((header, fields))

        # Without the pack the archive keeps the retail rows and files.
        stock = m.collect_sound_assets(root, index, originals, renderer, maps)
        _, rows = m.alias_table(stock['soundaliases/test_mp.csv'].decode())
        assert sorted(rows) == sorted([FIRE_A, FIRE_B, LAYER, RELOAD, KEPT, HIT])
        assert not any(name.startswith(('sound/browser/owner/', 'soundaliases/browser_owner')) for name in stock)
        m.verify_owner_sounds(stock, original_rows, None)

        # With it, each listed alias plays only the owner's files, one row a file, on the alias's own columns.
        owned = m.collect_sound_assets(root, index, originals, renderer, maps, owner_sounds=pack)
        _, rows = m.alias_table(owned['soundaliases/test_mp.csv'].decode())
        assert sorted(rows) == sorted([KEPT, HIT])
        table_header, rows = m.alias_table(owned[m.OWNER_SOUNDS_TABLE].decode())
        assert table_header == LOWER
        # The engine drops rows of an alias that share a sequence number: several rows are numbered from 1.
        assert rows == [
            owner_row(FIRE_A, 'w/fire_1.wav', pitch_min='0.97', pitch_max='1.03', sequence='1'),
            owner_row(FIRE_B, 'w/fire_2.wav', pitch_min='0.97', pitch_max='1.03', sequence='2'),
            owner_row(FIRE_A, 'w/fire_3.wav', pitch_min='0.97', pitch_max='1.03', sequence='3'),
            owner_row(LAYER, 'w/fire_1.wav', sequence='1'),
            owner_row(LAYER, 'g/layer.wav', vol_max='0.6', sequence='2'),
            owner_row(RELOAD, 'w/reload.wav', pitch_min='1', pitch_max='1', sequence='7')]
        numbered = m.collect_sound_assets(root, index, originals, renderer, maps, owner_sounds=m.load_owner_sounds(
            write_pack(folder / 'numbered', LISTING.replace(',vol_max\n', ',sequence\n').replace(',,0.6\n', ',,9\n'))))
        _, rows = m.alias_table(numbered[m.OWNER_SOUNDS_TABLE].decode())
        assert [values[1] for values in rows] == ['1', '2', '3', '1', '9', '7']
        assert m.OWNER_SOUNDS_TABLE.startswith('soundaliases/') and m.OWNER_SOUNDS_TABLE.endswith('.csv')
        for name, data in OWNER.items():
            assert owned['sound/browser/owner/' + name] == data
        # Retail files only the replaced rows played are gone; ones other rows play stay.
        assert owned['sound/weapons/test_fire_a.wav'] == RETAIL['sound/weapons/test_fire_a.wav']
        for name in ('sound/weapons/test_fire_b.wav', 'sound/weapons/test_layer.wav', 'sound/weapons/test_reload.wav'):
            assert name not in owned and name in stock
        changed = {'soundaliases/test_mp.csv', 'sound/weapons/test_fire_b.wav', 'sound/weapons/test_layer.wav',
                   'sound/weapons/test_reload.wav'}
        assert {name: data for name, data in stock.items() if name not in changed} == \
            {name: data for name, data in owned.items() if name != 'soundaliases/test_mp.csv'
             and not name.startswith(('sound/browser/owner/', m.OWNER_SOUNDS_TABLE))}
        m.verify_owner_sounds(owned, original_rows, pack)

        # The checker accepts exactly that.
        def edited(**changes):
            package = dict(owned)
            for name, data in changes.items():
                if data is None:
                    package.pop(name)
                else:
                    package[name] = data
            return package
        table = owned[m.OWNER_SOUNDS_TABLE]
        bad_packages = [
            edited(**{'sound/browser/owner/w/fire_2.wav': OWNER['w/fire_1.wav']}),
            edited(**{'sound/browser/owner/g/layer.wav': None}),
            edited(**{'sound/browser/owner/w/extra.wav': OWNER['w/fire_1.wav']}),
            edited(**{m.OWNER_SOUNDS_TABLE: table.replace(b'w/fire_2.wav', b'w/fire_1.wav')}),
            edited(**{m.OWNER_SOUNDS_TABLE: table.replace(b',0.97,1.03,', b',1.3,1.4,', 1)}),
            edited(**{m.OWNER_SOUNDS_TABLE: table.replace(b',300,', b',0,')}),
            edited(**{m.OWNER_SOUNDS_TABLE: table.replace(b'weap_test_fire,3,', b'weap_test_fire,1,')}),
            edited(**{m.OWNER_SOUNDS_TABLE: table + table.split(b'\r\n', 1)[1]}),
            edited(**{m.OWNER_SOUNDS_TABLE: b'\r\n'.join(table.split(b'\r\n')[:-2]) + b'\r\n'}),
            edited(**{m.OWNER_SOUNDS_TABLE: None}),
            edited(**{'soundaliases/test_mp.csv': stock['soundaliases/test_mp.csv']}),
            stock,
        ]
        for package in bad_packages:
            raises(AssertionError, lambda: m.verify_owner_sounds(package, original_rows, pack))
        raises(AssertionError, lambda: m.verify_owner_sounds(owned, original_rows, None))
        COUNTS['packages'] = len(bad_packages) + 1

        # It refuses a pack it cannot place.
        def pack_with(listing):
            return m.load_owner_sounds(write_pack(folder / f'p{abs(hash(listing))}', listing))
        for listing, message in ((LISTING + 'weap_test_unused,w/fire_1.wav\n', 'weap_test_unused'),
                                 (LISTING + 'weap_test_other,w/fire_1.wav\n', 'weap_test_other'),
                                 (LISTING + 'weap_test_nowhere,w/fire_1.wav\n', 'weap_test_nowhere'),
                                 (LISTING.replace(',vol_max\n', ',bogus\n'), 'bogus')):
            raises(ValueError, lambda: m.collect_sound_assets(root, index, originals, renderer, maps,
                                                              owner_sounds=pack_with(listing)), message)
        with_hit = pack_with(LISTING + 'mp_hit_alert,w/fire_1.wav\n')
        raises(ValueError, lambda: m.collect_sound_assets(root, index, originals, renderer, maps,
                                                          hit_sound=accepted['mono 48 kHz'], owner_sounds=with_hit),
               'hit sound or menu music')
        assert 'mp_hit_alert' in m.collect_sound_assets(root, index, originals, renderer, maps,
                                                        owner_sounds=with_hit)[m.OWNER_SOUNDS_TABLE].decode().lower()
        with_music = pack_with(LISTING + 'music_mainmenu,w/fire_1.wav\n')
        raises(ValueError, lambda: m.collect_sound_assets(root, index, originals, renderer, maps,
                                                          menu_music=b'ID3', owner_sounds=with_music),
               'hit sound or menu music')

        # An explosive's alias plays its retail source's rows under its own name, in its own table, once
        # something names it; the pack replaces it as any other alias.
        mine = dict(renderer, **{'weapons/mp/mine_mp': b'WEAPONFILE\\armSound\\weap_mine_arm'})
        source = EXPLOSIVE_ALIASES['weap_mine_arm']
        source_sound = f'sound/weapons/{source}.wav'
        derived = ['weap_mine_arm' if column == 'name' else value for column, value in zip(LOWER, GRENADE[source])]
        armed = m.collect_sound_assets(root, index, originals, mine, maps)
        table_header, rows = m.alias_table(armed[m.ALIAS_TABLE].decode())
        assert table_header == LOWER and rows == [derived]
        assert armed[source_sound] == RETAIL[source_sound]
        assert {name: data for name, data in armed.items() if name not in {m.ALIAS_TABLE, source_sound}} == stock
        mine_pack = pack_with(LISTING + 'weap_mine_arm,w/reload.wav\n')
        replaced = m.collect_sound_assets(root, index, originals, mine, maps, owner_sounds=mine_pack)
        assert m.ALIAS_TABLE not in replaced and source_sound not in replaced
        table_header, rows = m.alias_table(replaced[m.OWNER_SOUNDS_TABLE].decode())
        assert table_header == LOWER and rows == [owner_row(derived, 'w/reload.wav')]
        # The checker knows the explosive rows only from the originals it is given.
        m.verify_owner_sounds(replaced, dict(original_rows, weap_mine_arm=[(LOWER, dict(zip(LOWER, derived)))]),
                              mine_pack)
        raises(AssertionError, lambda: m.verify_owner_sounds(replaced, original_rows, mine_pack))
        # No package without the rows they start from.
        bare = {name: entry for name, entry in index.items() if name != 'soundaliases/test_grenades.csv'}
        raises(ValueError, lambda: m.collect_sound_assets(root, bare, originals, renderer, maps),
               'source aliases')
    finally:
        original.close()


with tempfile.TemporaryDirectory(prefix='cod2-owner-sounds-') as directory:
    folder = Path(directory)
    archive_path = folder / 'iw_00.iwd'
    with zipfile.ZipFile(archive_path, 'w') as archive:
        archive.writestr('soundaliases/test_mp.csv', csv_text(
            [HEADER, ['# retail comment'], [], FIRE_A, FIRE_B, LAYER, RELOAD, KEPT, OTHER_MAP, UNUSED, HIT]))
        archive.writestr('soundaliases/test_grenades.csv', csv_text([HEADER, *GRENADE.values()]))
        for name, data in RETAIL.items():
            archive.writestr(name, data)

    check(load(module_source), archive_path, folder / 'live')

    (folder / 'test.c').write_text(harness)
    subprocess.run(['cc', '-O1', '-g', '-fsanitize=address,undefined', str(folder / 'test.c'),
                    '-o', str(folder / 'test')], check=True)
    arguments = [str(folder / 'test')]
    for number, data in enumerate(list(accepted.values()) + list(OWNER.values())):
        path = folder / f'{number}.wav'
        path.write_bytes(data)
        rate, channels = struct.unpack_from('<I', data, 24)[0], struct.unpack_from('<H', data, 22)[0]
        size = struct.unpack_from('<I', data, data.index(b'data') + 4)[0]
        arguments += [str(path), str(rate), str(channels), str(size // (2 * channels))]
    subprocess.run(arguments, check=True)

    mutants = [
        ("if (tag, bits) != (1, 16) or channels not in (1, 2)", "if bits != 16 or channels not in (1, 2)"),
        ("channels not in (1, 2) or rate < 11025", "channels not in (1, 2, 3) or rate < 11025"),
        ("rate < 11025 or rate > 48000 or", "rate < 8000 or rate > 96000 or"),
        ("rate > 48000 or align != 2 * channels:", "rate > 48000 or align < 2:"),
        ("    if not frames or frames % align:", "    if frames % align:"),
        ("if frames > OWNER_SOUNDS_MAX_SECONDS * rate * align:", "if frames > 2 * OWNER_SOUNDS_MAX_SECONDS * rate * align:"),
        ("        if position + 8 + size > len(data):\n            raise invalid('a chunk runs past the end of the file')\n", ""),
        ("if header[:2] != ['alias', 'file'] or len(set(header)) != len(header) or 'name' in header:",
         "if 'alias' not in header or 'file' not in header:"),
        ("r'[a-z0-9_]+(/[a-z0-9_]+)*\\.wav'", "r'[^ ]+'"),
        ("fields.pop('file').replace('\\\\', '/').lower()", "fields.pop('file').lower()"),
        ("{key: value for key, value in fields.items() if value}", "dict(fields)"),
        ("    columns = dict(fields, **values, file=OWNER_SOUNDS_FOLDER + name)",
         "    columns = dict(fields, file=OWNER_SOUNDS_FOLDER + name)"),
        ("            if alias in pack:\n                owned.setdefault(alias, []).append((table, fields))\n"
         "                if secondary in aliases:\n                    pending.append(secondary)\n                continue\n",
         "            if alias in pack:\n                owned.setdefault(alias, []).append((table, fields))\n"
         "                continue\n"),
        ("            if alias in pack:\n                owned.setdefault(alias, []).append((table, fields))\n"
         "                if secondary in aliases:\n                    pending.append(secondary)\n                continue\n",
         "            if alias in pack:\n                owned.setdefault(alias, []).append((table, fields))\n"
         "                if secondary in aliases:\n                    pending.append(secondary)\n"),
        ("table, fields = owned[alias][number % len(owned[alias])]", "table, fields = owned[alias][0]"),
        ("    if sequence is not None and 'sequence' not in values:", "    if False:"),
        ("    if sequence is not None and 'sequence' not in values:", "    if sequence is not None:"),
        ("owner_sound_row(header, fields, name, values, number + 1 if len(wanted) > 1 else None))",
         "owner_sound_row(header, fields, name, values, number + 1))"),
        ("row == owner_sound_row(header, fields, name, values, number + 1 if len(wanted) > 1 else None)",
         "row == owner_sound_row(header, fields, name, values, None)"),
        ("        if unused:\n            raise ValueError", "        if False:\n            raise ValueError"),
        ("                if unknown:\n                    raise ValueError", "                if False:\n                    raise ValueError"),
        ("files['sound/' + OWNER_SOUNDS_FOLDER + name] = owner_sounds['files'][name]",
         "files[OWNER_SOUNDS_FOLDER + name] = owner_sounds['files'][name]"),
        ("hit_sound is not None and HIT_SOUND_ALIAS in pack or\n", "hit_sound is None and HIT_SOUND_ALIAS in pack or\n"),
        ("menu_music is not None and set(MENU_MUSIC_ALIASES) & set(pack)):", "False):"),
        ("        assert not tables and not sounds, 'Owner sounds packaged without the owner pack'\n", ""),
        ("    assert sorted(played) == sorted(owner['aliases']), 'Owner sound aliases differ from the owner pack'\n", ""),
        ("        assert len(rows) == len(wanted) and all(", "        assert all("),
        ("any(original_header == header and\n                row == owner_sound_row(",
         "any(original_header == header or\n                row == owner_sound_row("),
        ("    assert sounds == {'sound/' + OWNER_SOUNDS_FOLDER + name: owner['files'][name] for name in used}, \\",
         "    assert set(sounds) <= {'sound/' + OWNER_SOUNDS_FOLDER + name for name in used}, \\"),
        ("not in owner['aliases']\n                       for row in other_rows), 'Original rows kept", "is not None\n                       for row in other_rows), 'Original rows kept"),
        ("        aliases.setdefault(fields['name'].lower(), []).append((ALIAS_TABLE, row, fields))\n", ""),
        ("    tables[ALIAS_TABLE] = (header, [])\n", "    tables[ALIAS_TABLE] = (header, rows)\n"),
        ("    if len(headers) != 1:\n", "    if len(headers) > 1:\n"),
    ]
    for old, new in mutants:
        assert module_source.count(old) == 1, old
        try:
            check(load(module_source.replace(old, new)), archive_path, folder / f'mutant{mutants.index((old, new))}')
        except (AssertionError, ValueError, IndexError, KeyError, TypeError):
            continue
        raise AssertionError(f'mutant survived: {old!r} -> {new!r}')

print(f'PASS: without the owner pack the retail rows stay; with it each listed alias plays only the owner\'s files, '
      f'one row a file cycling the alias\'s own rows and columns, several rows numbered for the engine\'s '
      f'duplicate check, its secondary alias still followed, unplayed '
      f'retail files dropped and the rest of the archive unchanged; an explosive\'s alias plays its retail source\'s '
      f'rows under its own name and the pack replaces it; {len(accepted)} accepted and {len(rejected)} '
      f'rejected WAVs and {len(BAD_LISTINGS) + 1} rejected lists, the accepted WAVs read alike by the engine '
      f'parser under ASan/UBSan; the checker rejects {COUNTS["packages"]} altered packages; all {len(mutants)} '
      f'mutants fail')
