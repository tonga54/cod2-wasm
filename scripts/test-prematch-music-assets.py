#!/usr/bin/env python3
"""Exercise the owner's countdown music in the private audio archive: the first 65 s of its frames, streamed as an
alias of its own that takes the retail mission opening music's columns; and beside it the end of a match's music
and the last minute's, 65 s of it from 30 s in."""
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

HEADER = ['name', 'sequence', 'file', 'vol_min', 'vol_max', 'vol_mod', 'pitch_min', 'pitch_max', 'dist_min',
          'dist_max', 'channel', 'type', 'probability', 'loop', 'masterslave', 'loadspec', 'subtitle', 'compression',
          'secondaryaliasname', 'volumefalloffcurve', 'startdelay', 'speakermap', 'reverb']


def row(**values):
    return [values.get(name, '') for name in HEADER]


# The retail rows as iw_sound2.csv has them: El Daba's opening music loads in
# that mission alone and starts after half a second.
INTRO = dict(file='music/eldaba_ridein2_GRFINAL.mp3', vol_min='0.8', vol_mod='music', channel='music',
             type='streamed', loadspec='eldaba', startdelay='500', speakermap='music')
INTRO_ROW = row(name='eldaba_intro_music', **INTRO)
AMBIENT_ROW = row(name='ambient_africa', file='ambient/africa.mp3', vol_min='0.5', channel='ambient',
                  type='streamed', loop='looping', loadspec='all_mp')
GRENADE_ROWS = [row(name=source, file=f'weapons/{source}.wav', channel='auto', loadspec='all_mp')
                for source in sorted(set(EXPLOSIVE_ALIASES.values()))]
GRENADE_SOUND = b'RIFF' + struct.pack('<I', 36) + b'WAVEfmt ' + bytes(28)
# The countdown's and the last minute's music stream quieter than the retail
# 0.8, which the end of a match's keeps.
PACKAGED_ROW = row(name='browser_prematch_music', **dict(INTRO, file='browser/prematchmusic.mp3', loadspec='all_mp',
                                                          startdelay='', vol_min='0.55', vol_max='0.55'))
ENDGAME_ROW = row(name='browser_endgame_music', **dict(INTRO, file='browser/endgamemusic.mp3', loadspec='all_mp',
                                                       startdelay=''))
LASTMINUTE_ROW = row(name='browser_lastminute_music', **dict(INTRO, file='browser/lastminutemusic.mp3',
                                                             loadspec='all_mp', startdelay='', vol_min='0.55',
                                                             vol_max='0.55'))


def csv_text(rows):
    return '\r\n'.join(','.join(values) for values in rows) + '\r\n'


V1 = (0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320)
V2 = (0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160)
RATES = {3: (44100, 48000, 32000), 2: (22050, 24000, 16000), 0: (11025, 12000, 8000)}


def frames(seconds, rate=44100, kbps=320, mono=False, version=3, layer=1, crc=False, encoder=None):
    """MPEG audio frames lasting at least seconds, each filled with its number; encoder puts an encoder's header
    tag ('Xing', 'Info' or 'VBRI') into the first frame where its kind of frame carries it."""
    rate_index = RATES[version].index(rate)
    bitrate_index = (V1 if version == 3 else V2).index(kbps)
    samples = 1152 if version == 3 else 576
    count = -(-int(seconds * rate) // samples)
    out = []
    for number in range(count):
        padding = number % 3 == 1
        header = (0xffe00000 | version << 19 | layer << 17 | (0 if crc else 1) << 16 | bitrate_index << 12 |
                  rate_index << 10 | padding << 9 | (3 if mono else 0) << 6)
        length = (144000 if version == 3 else 72000) * kbps // rate + padding
        body = bytearray(bytes([number % 251 + 1]) * (length - 4))
        if number == 0 and encoder:
            side = (17 if mono else 32) if version == 3 else (9 if mono else 17)
            at = 36 - 4 if encoder == b'VBRI' else (2 if crc else 0) + side
            body[at:at + 4] = encoder
        out.append(struct.pack('>I', header) + bytes(body))
    return out, samples / rate


def id3(payload):
    size = len(payload)
    return b'ID3\x03\x00\x00' + bytes((size >> 21 & 0x7f, size >> 14 & 0x7f, size >> 7 & 0x7f, size & 0x7f)) + payload


def kept(frame_list, frame_seconds, start, limit=65):
    """The frames from start on that the archive keeps: until they last limit seconds."""
    out, seconds = [], 0.0
    for frame in frame_list[start:]:
        if seconds >= limit:
            break
        out.append(frame)
        seconds += frame_seconds
    return b''.join(out)


# The owner's file as music stores sell it: a large ID3v2 tag with the cover,
# the encoder's Info frame, three minutes of 320 kbps stereo and an ID3v1 tag.
OWNER_FRAMES, OWNER_STEP = frames(192, encoder=b'Info')
OWNER = id3(b'APIC' + bytes(40000)) + b''.join(OWNER_FRAMES) + b'TAG' + bytes(125)
OWNER_KEPT = kept(OWNER_FRAMES, OWNER_STEP, 1)
# The last minute's music: 65 s from the first frame 30 s into the audio.
LASTMINUTE_SKIPPED = int(-(-30 // OWNER_STEP))
LASTMINUTE_KEPT = kept(OWNER_FRAMES, OWNER_STEP, 1 + LASTMINUTE_SKIPPED)

# name: (file, what the archive keeps, rate, channels)
cases = {
    'three minutes with tags and an Info frame': (OWNER, OWNER_KEPT, 44100, 2),
}
for name, args, encoder in (
        ('mono MPEG-1 VBR with a Xing frame', dict(mono=True, kbps=128), b'Xing'),
        ('MPEG-2 stereo with a Xing frame', dict(rate=22050, kbps=64, version=2), b'Xing'),
        ('MPEG-2 mono with an Info frame', dict(rate=24000, kbps=32, version=2, mono=True), b'Info'),
        ('CRC-protected with an Info frame', dict(crc=True, kbps=192), b'Info'),
        ('a VBRI frame', dict(kbps=160), b'VBRI')):
    frame_list, step = frames(80, encoder=encoder, **args)
    cases[name] = (b''.join(frame_list), kept(frame_list, step, 1), args.get('rate', 44100),
                   1 if args.get('mono') else 2)
short, short_step = frames(20)
cases['20 s without an encoder frame, kept whole'] = (b''.join(short), b''.join(short), 44100, 2)
just, just_step = frames(1.01, rate=48000, kbps=128)
cases['just over a second'] = (b''.join(just), b''.join(just), 48000, 2)
tiny, _ = frames(1.01, rate=8000, kbps=8, version=0)
cases['MPEG-2.5 8 kHz, a second, where the ID3v1 tag is a tenth'] = (b''.join(tiny) + b'TAG' + bytes(125),
                                                                     b''.join(tiny), 8000, 2)
# An audio frame that only looks like one the encoder wrote is kept.
lookalike, _ = frames(70)
lookalike[0] = lookalike[0][:4] + bytes(36) + b'Info' + lookalike[0][44:]
cases['an audio frame with Info at another offset'] = (b''.join(lookalike), kept(lookalike, OWNER_STEP, 0), 44100, 2)

refused_tag = id3(bytes(128)) + b''.join(frames(3)[0])
refused_tag = refused_tag[:9] + b'\x80' + refused_tag[10:]
rejected = {
    'empty': b'',
    'a WAV file': GRENADE_SOUND,
    'an ID3v2 size the engine refuses': refused_tag,
    'layer II': b''.join(frames(10, layer=2)[0]),
    'mostly not audio': b''.join(frames(3)[0]) + bytes(len(b''.join(frames(3)[0]))),
    'under 1 s': b''.join(frames(0.9)[0]),
    'only an encoder frame and under 1 s of audio': b''.join(frames(1.0, encoder=b'Info')[0][:38]),
}

# The engine's own MP3 reader must see the packaged music as the check does.
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
 for(int a=1;a<argc;a+=4){
  FILE *f=fopen(argv[a],"rb");assert(f);fseek(f,0,SEEK_END);int n=ftell(f);rewind(f);
  byte *b=malloc(n?n:1);assert(fread(b,1,n,f)==(size_t)n);fclose(f);
  int rate=0,channels=0,duration=0;
  assert(WebAudio_Mp3Info(b,(unsigned)n,&rate,&channels,&duration));
  assert(rate==atoi(argv[a+1])&&channels==atoi(argv[a+2])&&duration==atoi(argv[a+3]));
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


def check(m, archive_path, no_source_path):
    with contextlib.redirect_stdout(io.StringIO()):
        checks(m, archive_path, no_source_path)


def checks(m, archive_path, no_source_path):
    for name, (data, packaged, rate, channels) in cases.items():
        assert m.package_prematch_music(data) == packaged, name
        info = m.mp3_frames(packaged)
        assert info[0] == 0 and info[1:3] == (rate, channels), name
        assert not packaged.startswith(b'ID3') and packaged[-128:-125] != b'TAG', name
    # The longest countdown and its tail fit, and not a frame more.
    seconds = m.mp3_frames(OWNER_KEPT)[4]
    assert 65 <= seconds < 65 + OWNER_STEP
    for name, data in rejected.items():
        raises(ValueError, lambda: m.package_prematch_music(data))

    original = zipfile.ZipFile(archive_path)
    try:
        index = {info.filename.lower(): (archive_path, info.filename) for info in original.infolist()}
        originals = {archive_path: original}
        renderer = {}
        maps = {'mp_toujane': 'Toujane, Tunisia'}
        header, rows = m.alias_table(original.read('soundaliases/iw_sound2.csv').decode())
        source_rows = [(header, dict(zip(header, values))) for values in rows if values[0] == 'eldaba_intro_music']
        assert len(source_rows) == 1

        # Without the owner's file the archive has no countdown music, and
        # the mission's own music stays out of multiplayer.
        stock = m.collect_sound_assets(root, index, originals, renderer, maps)
        assert m.PREMATCH_MUSIC_TABLE not in stock and 'sound/' + m.PREMATCH_MUSIC_FILE not in stock
        assert 'sound/music/eldaba_ridein2_grfinal.mp3' not in stock
        m.verify_prematch_music(stock, source_rows, None)

        # With it, its own alias streams what the archive keeps of it, in
        # every map and at once, and nothing else changes.
        owned = m.collect_sound_assets(root, index, originals, renderer, maps, prematch_music=OWNER)
        assert owned['sound/' + m.PREMATCH_MUSIC_FILE] == OWNER_KEPT
        table_header, rows = m.alias_table(owned[m.PREMATCH_MUSIC_TABLE].decode())
        assert table_header == HEADER and rows == [PACKAGED_ROW]
        assert {name: data for name, data in owned.items()
                if name not in (m.PREMATCH_MUSIC_TABLE, 'sound/' + m.PREMATCH_MUSIC_FILE)} == stock
        m.verify_prematch_music(owned, source_rows, OWNER)

        # The checker accepts exactly that.
        altered = []
        for old, new in ((b',0.55,0.55,', b',0.8,,'), (b',0.55,0.55,', b',0.55,0.8,'), (b',all_mp,', b',eldaba,'),
                         (b',music,streamed', b',menu,streamed'), (b'browser_prematch_music', b'eldaba_intro_music'),
                         (b'prematchmusic.mp3', b'menumusic.mp3')):
            package = dict(owned)
            package[m.PREMATCH_MUSIC_TABLE] = owned[m.PREMATCH_MUSIC_TABLE].replace(old, new)
            assert package[m.PREMATCH_MUSIC_TABLE] != owned[m.PREMATCH_MUSIC_TABLE], old
            altered.append(package)
        delayed = dict(owned)
        delayed[m.PREMATCH_MUSIC_TABLE] = owned[m.PREMATCH_MUSIC_TABLE].replace(b'all_mp,,,,,,music,',
                                                                                b'all_mp,,,,,500,music,')
        assert delayed[m.PREMATCH_MUSIC_TABLE] != owned[m.PREMATCH_MUSIC_TABLE]
        doubled = dict(owned)
        doubled[m.PREMATCH_MUSIC_TABLE] = owned[m.PREMATCH_MUSIC_TABLE] + csv_text([PACKAGED_ROW]).encode()
        elsewhere = dict(owned)
        elsewhere['soundaliases/iw_sound2.csv'] = csv_text([HEADER, PACKAGED_ROW]).encode()
        whole = dict(owned)
        whole['sound/' + m.PREMATCH_MUSIC_FILE] = OWNER
        untrimmed = dict(owned)
        untrimmed['sound/' + m.PREMATCH_MUSIC_FILE] = b''.join(OWNER_FRAMES[1:])
        missing = {name: data for name, data in owned.items() if name != m.PREMATCH_MUSIC_TABLE}
        for package, owner in ([(package, OWNER) for package in altered] +
                               [(delayed, OWNER), (doubled, OWNER), (elsewhere, OWNER), (whole, OWNER),
                                (untrimmed, OWNER), (missing, OWNER), (stock, OWNER), (owned, None)]):
            raises(AssertionError, lambda: m.verify_prematch_music(package, source_rows, owner))
        raises(ValueError, lambda: m.verify_prematch_music(owned, source_rows, rejected['under 1 s']))

        # The end of a match's music beside it: its first 28 s as an alias of
        # its own, the rest unchanged; the checker takes exactly that.
        both = m.collect_sound_assets(root, index, originals, renderer, maps, prematch_music=OWNER,
                                      endgame_music=OWNER)
        kept = both['sound/' + m.ENDGAME_MUSIC_FILE]
        assert 28 <= m.mp3_frames(kept)[4] < 28 + OWNER_STEP and OWNER_KEPT.startswith(kept)
        table_header, rows = m.alias_table(both[m.ENDGAME_MUSIC_TABLE].decode())
        assert table_header == HEADER and rows == [ENDGAME_ROW]
        assert {name: data for name, data in both.items()
                if name not in (m.ENDGAME_MUSIC_TABLE, 'sound/' + m.ENDGAME_MUSIC_FILE)} == owned
        m.verify_endgame_music(both, source_rows, OWNER)
        m.verify_prematch_music(both, source_rows, OWNER)
        m.verify_endgame_music(owned, source_rows, None)
        longer = dict(both)
        longer['sound/' + m.ENDGAME_MUSIC_FILE] = OWNER_KEPT
        quieter = dict(both)
        quieter[m.ENDGAME_MUSIC_TABLE] = both[m.ENDGAME_MUSIC_TABLE].replace(b',0.8,,', b',0.55,0.55,')
        assert quieter[m.ENDGAME_MUSIC_TABLE] != both[m.ENDGAME_MUSIC_TABLE]
        for package, owner in ((owned, OWNER), (both, None), (longer, OWNER), (quieter, OWNER)):
            raises(AssertionError, lambda: m.verify_endgame_music(package, source_rows, owner))

        # The last minute's music beside them: 65 s of its frames from the
        # first 30 s in, the rest unchanged; the checker takes exactly that.
        all_three = m.collect_sound_assets(root, index, originals, renderer, maps, prematch_music=OWNER,
                                           endgame_music=OWNER, lastminute_music=OWNER)
        assert (LASTMINUTE_SKIPPED - 1) * OWNER_STEP < 30 <= LASTMINUTE_SKIPPED * OWNER_STEP
        assert all_three['sound/' + m.LASTMINUTE_MUSIC_FILE] == LASTMINUTE_KEPT
        assert 65 <= m.mp3_frames(LASTMINUTE_KEPT)[4] < 65 + OWNER_STEP
        table_header, rows = m.alias_table(all_three[m.LASTMINUTE_MUSIC_TABLE].decode())
        assert table_header == HEADER and rows == [LASTMINUTE_ROW]
        assert {name: data for name, data in all_three.items()
                if name not in (m.LASTMINUTE_MUSIC_TABLE, 'sound/' + m.LASTMINUTE_MUSIC_FILE)} == both
        m.verify_lastminute_music(all_three, source_rows, OWNER)
        m.verify_endgame_music(all_three, source_rows, OWNER)
        m.verify_lastminute_music(both, source_rows, None)
        from_start = dict(all_three)
        from_start['sound/' + m.LASTMINUTE_MUSIC_FILE] = OWNER_KEPT
        louder = dict(all_three)
        louder[m.LASTMINUTE_MUSIC_TABLE] = all_three[m.LASTMINUTE_MUSIC_TABLE].replace(b',0.55,0.55,', b',0.8,,')
        assert louder[m.LASTMINUTE_MUSIC_TABLE] != all_three[m.LASTMINUTE_MUSIC_TABLE]
        for package, owner in ((both, OWNER), (all_three, None), (from_start, OWNER), (louder, OWNER)):
            raises(AssertionError, lambda: m.verify_lastminute_music(package, source_rows, owner))
        # A track that ends before a second past its entry is refused.
        brief, _ = frames(30.5)
        raises(ValueError, lambda: m.collect_sound_assets(root, index, originals, renderer, maps,
                                                          lastminute_music=b''.join(brief)))
        # The last minute's track is the owner's own for it, else the
        # countdown's; a folder by either name is neither.
        with tempfile.TemporaryDirectory(prefix='cod2-lastminute-') as folder:
            main = Path(folder)
            assert m.lastminute_owner_file(main) is None
            (main / m.LASTMINUTE_MUSIC_OWNER_FILE).mkdir()
            assert m.lastminute_owner_file(main) is None
            (main / m.PREMATCH_MUSIC_OWNER_FILE).write_bytes(b'countdown')
            assert m.lastminute_owner_file(main) == main / m.PREMATCH_MUSIC_OWNER_FILE
            (main / m.LASTMINUTE_MUSIC_OWNER_FILE).rmdir()
            (main / m.LASTMINUTE_MUSIC_OWNER_FILE).write_bytes(b'own')
            assert m.lastminute_owner_file(main) == main / m.LASTMINUTE_MUSIC_OWNER_FILE
            (main / m.PREMATCH_MUSIC_OWNER_FILE).unlink()
            assert m.lastminute_owner_file(main) == main / m.LASTMINUTE_MUSIC_OWNER_FILE

        # It refuses music it cannot read, and a game without the alias to copy.
        raises(ValueError, lambda: m.collect_sound_assets(root, index, originals, renderer, maps,
                                                          prematch_music=rejected['layer II']))
        other = zipfile.ZipFile(no_source_path)
        try:
            other_index = {info.filename.lower(): (no_source_path, info.filename) for info in other.infolist()}
            raises(ValueError, lambda: m.collect_sound_assets(root, other_index, {no_source_path: other}, renderer,
                                                              maps, prematch_music=OWNER))
        finally:
            other.close()
    finally:
        original.close()


def write_archive(path, intro):
    with zipfile.ZipFile(path, 'w') as archive:
        archive.writestr('soundaliases/iw_code.csv', csv_text([HEADER, AMBIENT_ROW]))
        archive.writestr('soundaliases/iw_sound2.csv', csv_text([HEADER, *([INTRO_ROW] if intro else [])]))
        archive.writestr('soundaliases/test_grenades.csv', csv_text([HEADER, *GRENADE_ROWS]))
        for values in GRENADE_ROWS:
            archive.writestr('sound/' + values[2], GRENADE_SOUND)
        archive.writestr('sound/ambient/africa.mp3', b''.join(frames(2, rate=48000, kbps=128)[0]))
        archive.writestr('sound/music/eldaba_ridein2_GRFINAL.mp3', b''.join(frames(3)[0]))


with tempfile.TemporaryDirectory(prefix='cod2-prematch-music-') as directory:
    folder = Path(directory)
    archive_path, no_source_path = folder / 'iw_00.iwd', folder / 'iw_01.iwd'
    write_archive(archive_path, True)
    write_archive(no_source_path, False)
    check(load(module_source), archive_path, no_source_path)

    (folder / 'test.c').write_text(harness)
    subprocess.run(['cc', '-O1', '-g', '-fsanitize=address,undefined', str(folder / 'test.c'),
                    '-o', str(folder / 'test')], check=True)
    module = load(module_source)
    arguments = [str(folder / 'test')]
    for number, (data, packaged, rate, channels) in enumerate(cases.values()):
        path = folder / f'{number}.mp3'
        path.write_bytes(packaged)
        arguments += [str(path), str(rate), str(channels), str(int(module.mp3_frames(packaged)[4] * 1000))]
    subprocess.run(arguments, check=True)

    mutants = [
        ("PREMATCH_MUSIC_SECONDS = 65", "PREMATCH_MUSIC_SECONDS = 60"),
        ("        if seconds >= limit:\n            break\n", ""),
        ("    if vbr_header(data, spans[0][0]):\n        spans = spans[1:]\n", ""),
        ("in (b'Xing', b'Info') or", "== b'Xing' or"),
        (" or data[start + 36:start + 40] == b'VBRI'", ""),
        ("(0 if header >> 16 & 1 else 2)", "0"),
        ("side = (17 if mono else 32) if header >> 19 & 3 == 3 else (9 if mono else 17)", "side = 32"),
        ("side = (17 if mono else 32) if header >> 19 & 3 == 3 else (9 if mono else 17)",
         "side = 17 if mono else 32"),
        ("    if sum(length for _, length, _ in spans) * 10 < audio * 9:", "    if False:"),
        ("    audio = len(data) - tag - (128 if data[-128:-125] == b'TAG' else 0)\n    if sum(",
         "    audio = len(data) - tag\n    if sum("),
        ("    if seconds < 1:\n        raise invalid(f'shorter than {skip + 1} s')",
         "    if not seconds:\n        raise invalid('empty')"),
        ("    if walked is None or not walked[3]:\n        raise invalid('not an MP3 file')",
         "    if walked is None:\n        raise invalid('not an MP3 file')"),
        ("name=alias, file=file, loadspec='all_mp', startdelay=''",
         "name=alias, file=file, startdelay=''"),
        ("name=alias, file=file, loadspec='all_mp', startdelay=''",
         "name=alias, file=file, loadspec='all_mp'"),
        ("PREMATCH_MUSIC_SOURCE = 'eldaba_intro_music'", "PREMATCH_MUSIC_SOURCE = 'ambient_africa'"),
        ("        files['sound/' + PREMATCH_MUSIC_FILE] = package_prematch_music(prematch_music)",
         "        files['sound/' + PREMATCH_MUSIC_FILE] = prematch_music"),
        ("        assert table is None and music is None, f'{what} packaged without the owner file'",
         "        assert True"),
        ("    assert music == package_prematch_music(owner, limit, owner_file, skip), f'Packaged {what} differs from the owner file'",
         "    assert music is not None"),
        ("    assert music == package_prematch_music(owner, limit, owner_file, skip), f'Packaged {what} differs from the owner file'",
         "    assert music == package_prematch_music(owner, limit, owner_file), f'Packaged {what} differs from the owner file'"),
        ("    assert len(rows) == 1 and any(source_header == header", "    assert any(source_header == header"),
        ("            assert all(dict(zip(other_header, row)).get('name', '').lower() != alias",
         "            assert all(row is not None"),
        ("ENDGAME_MUSIC_SECONDS = 28", "ENDGAME_MUSIC_SECONDS = 65"),
        ("ENDGAME_MUSIC_SECONDS = 28", "ENDGAME_MUSIC_SECONDS = 20"),
        ("        writer.writerow(prematch_music_row(header, fields, ENDGAME_MUSIC_ALIAS, ENDGAME_MUSIC_FILE))",
         "        writer.writerow(prematch_music_row(header, fields))"),
        ("package_prematch_music(endgame_music, ENDGAME_MUSIC_SECONDS,\n"
         "                                                                      ENDGAME_MUSIC_OWNER_FILE)",
         "package_prematch_music(endgame_music)"),
        ("LASTMINUTE_MUSIC_START = 30", "LASTMINUTE_MUSIC_START = 29"),
        ("LASTMINUTE_MUSIC_SECONDS = 65", "LASTMINUTE_MUSIC_SECONDS = 60"),
        ("        if skipped < skip:\n            skipped += duration\n            continue\n", ""),
        ("        if skipped < skip:\n", "        if skipped + duration < skip:\n"),
        ("prematch_music_row(header, fields, LASTMINUTE_MUSIC_ALIAS, LASTMINUTE_MUSIC_FILE,",
         "prematch_music_row(header, fields, ENDGAME_MUSIC_ALIAS, LASTMINUTE_MUSIC_FILE,"),
        ("            lastminute_music, LASTMINUTE_MUSIC_SECONDS, LASTMINUTE_MUSIC_OWNER_FILE, LASTMINUTE_MUSIC_START)",
         "            lastminute_music, LASTMINUTE_MUSIC_SECONDS, LASTMINUTE_MUSIC_OWNER_FILE)"),
        ("'last minute music', LASTMINUTE_MUSIC_START, LASTMINUTE_MUSIC_VOLUME)",
         "'last minute music', volume=LASTMINUTE_MUSIC_VOLUME)"),
        # The volumes: the countdown's and the last minute's below the retail
        # row's, the end of a match's at it, and the checker holding each.
        ("PREMATCH_MUSIC_VOLUME = '0.55'", "PREMATCH_MUSIC_VOLUME = '0.8'"),
        ("LASTMINUTE_MUSIC_VOLUME = '0.55'", "LASTMINUTE_MUSIC_VOLUME = '0.8'"),
        ("    if volume is not None:\n        values.update(vol_min=volume, vol_max=volume)\n", ""),
        ("values.update(vol_min=volume, vol_max=volume)", "values.update(vol_max=volume)"),
        ("file=PREMATCH_MUSIC_FILE, volume=None):", "file=PREMATCH_MUSIC_FILE, volume=PREMATCH_MUSIC_VOLUME):"),
        ("        writer.writerow(prematch_music_row(header, fields, volume=PREMATCH_MUSIC_VOLUME))",
         "        writer.writerow(prematch_music_row(header, fields))"),
        ("LASTMINUTE_MUSIC_FILE,\n                                           LASTMINUTE_MUSIC_VOLUME))",
         "LASTMINUTE_MUSIC_FILE))"),
        ("ENDGAME_MUSIC_ALIAS, ENDGAME_MUSIC_FILE))", "ENDGAME_MUSIC_ALIAS, ENDGAME_MUSIC_FILE, PREMATCH_MUSIC_VOLUME))"),
        ("'countdown music',\n                       volume=PREMATCH_MUSIC_VOLUME)", "'countdown music')"),
        ("'last minute music', LASTMINUTE_MUSIC_START, LASTMINUTE_MUSIC_VOLUME)",
         "'last minute music', LASTMINUTE_MUSIC_START)"),
        ("what, skip=0,\n                       volume=None):", "what, skip=0,\n                       volume='0.55'):"),
        ("file_name, volume)", "file_name)"),
        ("    for name in (LASTMINUTE_MUSIC_OWNER_FILE, PREMATCH_MUSIC_OWNER_FILE):",
         "    for name in (PREMATCH_MUSIC_OWNER_FILE, LASTMINUTE_MUSIC_OWNER_FILE):"),
        ("    for name in (LASTMINUTE_MUSIC_OWNER_FILE, PREMATCH_MUSIC_OWNER_FILE):",
         "    for name in (LASTMINUTE_MUSIC_OWNER_FILE,):"),
        ("        if (main / name).is_file():", "        if (main / name).exists():"),
    ]
    for old, new in mutants:
        assert module_source.count(old) == 1, old
        try:
            check(load(module_source.replace(old, new)), archive_path, no_source_path)
        except (AssertionError, ValueError, IndexError, KeyError):
            continue
        raise AssertionError(f'mutant survived: {old!r} -> {new!r}')

print(f'PASS: without the owner file the archive has no countdown music; with it browser_prematch_music streams the '
      f'first 65 s of its frames, tags and encoder frame left out, as El Daba\'s opening music in every map and at '
      f'once but at 0.55 in place of 0.8, the rest of the archive unchanged; {len(cases)} kept and {len(rejected)} '
      f'refused MP3s, the kept ones read as the engine reads them under ASan/UBSan; the checker rejects 14 altered '
      f'packages; the last minute\'s music streams 65 s of the owner\'s track from 30 s in at 0.55, the countdown\'s '
      f'unless they keep one for it, and the end of a match\'s at 0.8; all {len(mutants)} mutants fail')
