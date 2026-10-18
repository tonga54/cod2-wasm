#!/usr/bin/env python3
"""Exercise the owner's menu music replacing the menus' music streams in the private audio archive."""
from pathlib import Path
import contextlib
import io
import struct
import subprocess
import tempfile
import types
import zipfile

root = Path(__file__).resolve().parent.parent
module_path = root / 'scripts/browser_sound_assets.py'
module_source = module_path.read_text()

HEADER = ['name', 'sequence', 'file', 'vol_min', 'vol_max', 'vol_mod', 'pitch_min', 'pitch_max', 'dist_min',
          'dist_max', 'channel', 'type', 'probability', 'loop', 'masterslave', 'loadspec', 'subtitle', 'compression',
          'secondaryaliasname', 'volumefalloffcurve', 'startdelay', 'speakermap', 'reverb']


def row(**values):
    return [values.get(name, '') for name in HEADER]


MUSIC = dict(file='music\\menu_GRTEMP.mp3', vol_min='0.3', vol_mod='music', channel='menu', type='streamed',
             loop='looping', loadspec='menu', speakermap='music')
MENU_ROW = row(name='music_mainmenu', **MUSIC)
MENU_MP_ROW = row(name='music_mainmenu_mp', **MUSIC)
AMBIENT_ROW = row(name='ambient_africa', file='ambient/africa.mp3', vol_min='0.5', channel='ambient',
                  type='streamed', loop='looping', loadspec='all_mp')


def csv_text(rows):
    return '\r\n'.join(','.join(values) for values in rows) + '\r\n'


V1 = (0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320)
V2 = (0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160)
RATES = {3: (44100, 48000, 32000), 2: (22050, 24000, 16000), 0: (11025, 12000, 8000)}


def mp3(seconds, rate=44100, kbps=128, mono=False, version=3, layer=1, id3=b'', tail=b''):
    """MPEG audio frames of silence lasting at least seconds; returns the file, its frames and their length in s."""
    rate_index = RATES[version].index(rate)
    bitrate_index = (V1 if version == 3 else V2).index(kbps)
    samples = 1152 if version == 3 else 576
    count = -(-int(seconds * rate) // samples)
    frames = b''
    for number in range(count):
        padding = number % 3 == 1
        header = (0xffe00000 | version << 19 | layer << 17 | 1 << 16 | bitrate_index << 12 | rate_index << 10 |
                  padding << 9 | (3 if mono else 0) << 6)
        length = (144000 if version == 3 else 72000) * kbps // rate + padding
        frames += struct.pack('>I', header) + bytes(length - 4)
    return id3 + frames + tail, count, count * samples / rate


def id3(payload, size=None):
    size = len(payload) if size is None else size
    return b'ID3\x03\x00\x00' + bytes((size >> 21 & 0x7f, size >> 14 & 0x7f, size >> 7 & 0x7f, size & 0x7f)) + payload


ORIGINAL, _, _ = mp3(3)
AMBIENT, _, _ = mp3(2, rate=48000)
OWNER, OWNER_FRAMES, OWNER_SECONDS = mp3(30, id3=id3(b'TIT2' + bytes(60)))

# name: (file, rate, channels, frames, seconds)
accepted = {
    'stereo 44.1 kHz with an ID3v2 tag': (OWNER, 44100, 2, OWNER_FRAMES, OWNER_SECONDS),
    'mono 48 kHz at 320 kbps': (*mp3(10, rate=48000, kbps=320, mono=True)[:1], 48000, 1,
                               *mp3(10, rate=48000, kbps=320, mono=True)[1:]),
    'MPEG-2 22.05 kHz with an ID3v1 tag': (*mp3(5, rate=22050, kbps=64, version=2, tail=b'TAG' + bytes(125))[:1],
                                           22050, 2, *mp3(5, rate=22050, kbps=64, version=2)[1:]),
    'MPEG-2.5 8 kHz, 1 s, where the ID3v1 tag is a tenth': (
        *mp3(1, rate=8000, kbps=8, version=0, tail=b'TAG' + bytes(125))[:1], 8000, 2,
        *mp3(1, rate=8000, kbps=8, version=0)[1:]),
    'just under 240 s at 32 kbps': (*mp3(239.9, kbps=32)[:1], 44100, 2, *mp3(239.9, kbps=32)[1:]),
}
refused_tag = id3(bytes(128), 0) + mp3(3)[0]
refused_tag = refused_tag[:9] + b'\x80' + refused_tag[10:]
rejected = {
    'empty': b'',
    'a WAV file': b'RIFF' + struct.pack('<I', 36) + b'WAVEfmt ' + bytes(28),
    'an ID3v2 size the engine refuses': refused_tag,
    'layer II': mp3(10, layer=2)[0],
    'free-format bit rate': b''.join(struct.pack('>I', 0xfffb0000) + bytes(400) for _ in range(100)),
    'mostly not audio': mp3(3)[0] + bytes(len(mp3(3)[0])),
    'under 1 s': mp3(0.9)[0],
    'over 240 s': mp3(240.1, kbps=32)[0],
}

# The engine's own MP3 reader must see every accepted file as the check does.
engine = (root / 'downstream/wasm/web_audio.c').read_text()
start = engine.index('static int WebAudio_Mp3Info')
end = engine.index('\n}\n', start) + 3
harness = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte;
''' + engine[start:end] + r'''
int main(int argc,char **argv){
 for(int a=1;a<argc;a+=5){
  FILE *f=fopen(argv[a],"rb");assert(f);fseek(f,0,SEEK_END);int n=ftell(f);rewind(f);
  byte *b=malloc(n?n:1);assert(fread(b,1,n,f)==(size_t)n);fclose(f);
  int rate=0,channels=0,duration=0,ok=WebAudio_Mp3Info(b,(unsigned)n,&rate,&channels,&duration);
  assert(ok==atoi(argv[a+1]));
  if(ok)assert(rate==atoi(argv[a+2])&&channels==atoi(argv[a+3])&&duration==atoi(argv[a+4]));
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
    for name, (data, rate, channels, frames, seconds) in accepted.items():
        assert m.check_menu_music(data) is data, name
        info = m.mp3_frames(data)
        audio = len(data) - (74 if data.startswith(b'ID3') else 0) - (128 if b'TAG' in data[-128:] else 0)
        assert info[1:4] == (rate, channels, frames) and abs(info[4] - seconds) < 1e-9 and info[5] == audio, name
    for name, data in rejected.items():
        raises(ValueError, lambda: m.check_menu_music(data))

    original = zipfile.ZipFile(archive_path)
    try:
        index = {info.filename.lower(): (archive_path, info.filename) for info in original.infolist()}
        originals = {archive_path: original}
        renderer = {'ui_mp/main.menu': b'\t\tsoundloop "music_mainmenu_mp"\n'}
        maps = {'mp_toujane': 'Toujane, Tunisia'}
        music_rows = []
        for table in ('soundaliases/iw_code.csv', 'soundaliases/iw_sound2.csv'):
            header, rows = m.alias_table(original.read(table).decode())
            music_rows += [(header, dict(zip(header, values))) for values in rows
                           if values[0].lower() in ('music_mainmenu', 'music_mainmenu_mp')]
        assert len(music_rows) == 2

        # Without the owner's file, the menus keep the retail stream.
        stock = m.collect_sound_assets(root, index, originals, renderer, maps)
        assert m.MENU_MUSIC_TABLE not in stock and 'sound/' + m.MENU_MUSIC_FILE not in stock
        _, rows = m.alias_table(stock['soundaliases/iw_code.csv'].decode())
        assert sorted(rows) == sorted([MENU_ROW, MENU_MP_ROW, AMBIENT_ROW])
        assert stock['sound/music/menu_grtemp.mp3'] == ORIGINAL
        m.verify_menu_music(stock, music_rows, None)

        # With it, both aliases stream it, as the retail rows did, and nothing else changes.
        owned = m.collect_sound_assets(root, index, originals, renderer, maps, menu_music=OWNER)
        assert owned['sound/' + m.MENU_MUSIC_FILE] == OWNER
        _, rows = m.alias_table(owned['soundaliases/iw_code.csv'].decode())
        assert rows == [AMBIENT_ROW]
        table_header, rows = m.alias_table(owned[m.MENU_MUSIC_TABLE].decode())
        assert table_header == HEADER and rows == [
            row(name='music_mainmenu', **dict(MUSIC, file='browser/menumusic.mp3')),
            row(name='music_mainmenu_mp', **dict(MUSIC, file='browser/menumusic.mp3'))]
        assert m.MENU_MUSIC_TABLE.startswith('soundaliases/') and m.MENU_MUSIC_TABLE.endswith('.csv')
        replaced = {'soundaliases/iw_code.csv', 'sound/music/menu_grtemp.mp3'}
        assert {name: data for name, data in stock.items() if name not in replaced} == \
            {name: data for name, data in owned.items()
             if name not in {'soundaliases/iw_code.csv', m.MENU_MUSIC_TABLE, 'sound/' + m.MENU_MUSIC_FILE}}
        m.verify_menu_music(owned, music_rows, OWNER)

        # The checker accepts exactly that.
        louder = dict(owned)
        louder[m.MENU_MUSIC_TABLE] = owned[m.MENU_MUSIC_TABLE].replace(b',0.3,', b',0.6,')
        assert louder[m.MENU_MUSIC_TABLE] != owned[m.MENU_MUSIC_TABLE]
        one = dict(owned)
        one[m.MENU_MUSIC_TABLE] = owned[m.MENU_MUSIC_TABLE].rsplit(b'music_mainmenu_mp', 1)[0].rsplit(b'\r\n', 1)[0] + b'\r\n'
        doubled = dict(owned)
        doubled[m.MENU_MUSIC_TABLE] = owned[m.MENU_MUSIC_TABLE] + owned[m.MENU_MUSIC_TABLE].split(b'\r\n', 2)[2]
        kept = dict(owned)
        kept['soundaliases/iw_code.csv'] = stock['soundaliases/iw_code.csv']
        swapped = dict(owned)
        swapped['sound/' + m.MENU_MUSIC_FILE] = mp3(30)[0]
        missing = {name: data for name, data in owned.items() if name != m.MENU_MUSIC_TABLE}
        for package, owner in ((louder, OWNER), (one, OWNER), (doubled, OWNER), (kept, OWNER), (swapped, OWNER),
                               (missing, OWNER), (stock, OWNER), (owned, None)):
            raises(AssertionError, lambda: m.verify_menu_music(package, music_rows, owner))
        raises(ValueError, lambda: m.verify_menu_music(owned, music_rows, rejected['under 1 s']))

        # It refuses music it cannot place or the browser cannot hold.
        raises(ValueError, lambda: m.collect_sound_assets(root, index, originals, {}, maps, menu_music=OWNER))
        raises(ValueError, lambda: m.collect_sound_assets(
            root, index, originals, renderer, maps, menu_music=rejected['over 240 s']))
    finally:
        original.close()


with tempfile.TemporaryDirectory(prefix='cod2-menu-music-') as directory:
    folder = Path(directory)
    archive_path = folder / 'iw_00.iwd'
    with zipfile.ZipFile(archive_path, 'w') as archive:
        archive.writestr('soundaliases/iw_code.csv', csv_text([HEADER, MENU_ROW, MENU_MP_ROW, AMBIENT_ROW]))
        archive.writestr('soundaliases/iw_sound2.csv', csv_text(
            [HEADER, ['#music_mainmenu', '', 'music\\other.mp3'], ['#music_mainmenu_mp', '', 'music\\other.mp3']]))
        archive.writestr('sound/music/menu_grtemp.mp3', ORIGINAL)
        archive.writestr('sound/ambient/africa.mp3', AMBIENT)

    check(load(module_source), archive_path)

    (folder / 'test.c').write_text(harness)
    subprocess.run(['cc', '-O1', '-g', '-fsanitize=address,undefined', str(folder / 'test.c'),
                    '-o', str(folder / 'test')], check=True)
    module = load(module_source)
    arguments = [str(folder / 'test')]
    cases = [(data, 1, rate, channels, int(module.mp3_frames(data)[4] * 1000))
             for data, rate, channels, _, _ in accepted.values()]
    cases += [(rejected[name], 0, 0, 0, 0) for name in ('empty', 'a WAV file', 'an ID3v2 size the engine refuses',
                                                        'layer II', 'free-format bit rate')]
    for number, (data, ok, rate, channels, duration) in enumerate(cases):
        path = folder / f'{number}.mp3'
        path.write_bytes(data)
        arguments += [str(path), str(ok), str(rate), str(channels), str(duration)]
    subprocess.run(arguments, check=True)

    mutants = [
        ("if menu_music is not None and alias in MENU_MUSIC_ALIASES:",
         "if menu_music is not None and alias == MENU_MUSIC_ALIASES[0]:"),
        ("                music.setdefault(alias, (tables[table][0], fields))\n                continue\n",
         "                music.setdefault(alias, (tables[table][0], fields))\n"),
        ("values = dict(fields, file=MENU_MUSIC_FILE)", "values = dict(fields)"),
        ("files['sound/' + MENU_MUSIC_FILE] = check_menu_music(menu_music)",
         "files[MENU_MUSIC_FILE] = check_menu_music(menu_music)"),
        ("        if len(music) != len(MENU_MUSIC_ALIASES):\n", "        if not music:\n"),
        ("    if covered * 10 < audio * 9:", "    if False:"),
        ("(128 if data[-128:-125] == b'TAG' else 0)", "0"),
        ("    if not 1 <= seconds <= MENU_MUSIC_MAX_SECONDS:", "    if not 1 <= seconds:"),
        ("MENU_MUSIC_MAX_SECONDS = 240", "MENU_MUSIC_MAX_SECONDS = 300"),
        ("        if any(byte & 0x80 for byte in data[6:10]):\n            return None\n", ""),
        ("(144000 if version == 3 else 72000)", "(144000 if version >= 2 else 72000)"),
        ("(1152 if version == 3 else 576) / hz", "1152 / hz"),
        ("hz = rates[rate_index] // (1 if version == 3 else 2 if version == 2 else 4)", "hz = rates[rate_index]"),
        ("rate, channels = hz, 1 if header >> 6 & 3 == 3 else 2", "rate, channels = hz, 2"),
        ("+ (header >> 9 & 1)", ""),
        ("assert table is None and music is None", "assert True"),
        ("assert music == check_menu_music(owner)", "assert music is not None"),
        ("    assert names == list(MENU_MUSIC_ALIASES) and all(", "    assert all("),
        ("dict(zip(other_header, row)).get('name', '').lower() not in MENU_MUSIC_ALIASES", "row is not None"),
    ]
    for old, new in mutants:
        assert module_source.count(old) == 1, old
        try:
            check(load(module_source.replace(old, new)), archive_path)
        except (AssertionError, ValueError, IndexError, KeyError):
            continue
        raise AssertionError(f'mutant survived: {old!r} -> {new!r}')

print(f'PASS: without the owner file the menus keep the retail stream; with it music_mainmenu and '
      f'music_mainmenu_mp stream only the owner\'s MP3, as the retail rows did, the rest of the archive unchanged; '
      f'{len(accepted)} accepted and {len(rejected)} rejected MP3s, read as the engine reads them under ASan/UBSan; '
      f'the checker rejects 8 altered packages; all {len(mutants)} mutants fail')
