"""Follow the supported multiplayer maps' original sound aliases."""
import csv
import io
import re
import struct
from browser_explosives import ALIAS_TABLE, ALIASES, alias_rows

# The owner may keep a hit confirmation sound of their own beside the original
# IWDs, never in Git. It replaces the sound of the scripts' MP_hit_alert.
HIT_SOUND_OWNER_FILE = 'hitmarker.wav'
HIT_SOUND_ALIAS = 'mp_hit_alert'
HIT_SOUND_FILE = 'browser/hitmarker.wav'
HIT_SOUND_TABLE = 'soundaliases/browser_hitmarker.csv'
# Main menu music of the owner's may sit there too, likewise never in Git. It
# replaces the stream of the menus' music aliases, quieter than their 0.3.
MENU_MUSIC_OWNER_FILE = 'menumusic.mp3'
MENU_MUSIC_ALIASES = ('music_mainmenu', 'music_mainmenu_mp')
MENU_MUSIC_FILE = 'browser/menumusic.mp3'
MENU_MUSIC_TABLE = 'soundaliases/browser_menumusic.csv'
MENU_MUSIC_VOLUME = '0.2'
# The browser decodes a stream whole, 384 KB a second at 48 kHz in stereo.
MENU_MUSIC_MAX_SECONDS = 240
# Music of the owner's for the countdown before a match may sit there too,
# likewise never in Git. It streams as an alias of its own, which takes the
# columns of the retail mission opening music, quieter than its 0.8.
PREMATCH_MUSIC_OWNER_FILE = 'prematchmusic.mp3'
PREMATCH_MUSIC_ALIAS = 'browser_prematch_music'
PREMATCH_MUSIC_SOURCE = 'eldaba_intro_music'
PREMATCH_MUSIC_FILE = 'browser/prematchmusic.mp3'
PREMATCH_MUSIC_TABLE = 'soundaliases/browser_prematchmusic.csv'
PREMATCH_MUSIC_VOLUME = '0.55'
# The longest countdown the server holds, 60 s, and the 5 s the music outlasts
# it: the archive keeps that much of the music, about 25 MB decoded.
PREMATCH_MUSIC_SECONDS = 65
# Music of the owner's for the end of a match, likewise never in Git, streams
# the same way as an alias of its own, at the retail row's volume.
ENDGAME_MUSIC_OWNER_FILE = 'endgamemusic.mp3'
ENDGAME_MUSIC_ALIAS = 'browser_endgame_music'
ENDGAME_MUSIC_FILE = 'browser/endgamemusic.mp3'
ENDGAME_MUSIC_TABLE = 'soundaliases/browser_endgamemusic.csv'
# The 24 s intermission the server holds (browser_endgame.py) and a margin,
# about 11 MB decoded.
ENDGAME_MUSIC_SECONDS = 28
# Music for the last minute of a match streams the same way as an alias of its
# own, as quiet as the countdown's, entering LASTMINUTE_MUSIC_START seconds
# into the file: the owner's own track for it, likewise never in Git, or else
# the countdown's.
LASTMINUTE_MUSIC_OWNER_FILE = 'lastminutemusic.mp3'
LASTMINUTE_MUSIC_ALIAS = 'browser_lastminute_music'
LASTMINUTE_MUSIC_FILE = 'browser/lastminutemusic.mp3'
LASTMINUTE_MUSIC_TABLE = 'soundaliases/browser_lastminutemusic.csv'
LASTMINUTE_MUSIC_VOLUME = '0.55'
LASTMINUTE_MUSIC_START = 30
# The minute it plays and 5 s more, about 25 MB decoded.
LASTMINUTE_MUSIC_SECONDS = 65
# A sound pack of the owner's may sit there too, likewise never in Git: a
# folder of 16-bit PCM WAVs and a list of the original aliases whose rows they
# replace, a row a file, with values for any other alias column.
OWNER_SOUNDS_DIR = 'owner_sounds'
OWNER_SOUNDS_LIST = 'aliases.csv'
OWNER_SOUNDS_FOLDER = 'browser/owner/'
OWNER_SOUNDS_TABLE = 'soundaliases/browser_owner.csv'
OWNER_SOUNDS_MAX_SECONDS = 20


def alias_table(text):
    """The lowercased header and the alias rows of a sound alias CSV."""
    rows = [row for row in csv.reader(io.StringIO(text))
            if row and row[0].strip() and not row[0].lstrip().startswith('#')]
    return [value.lower().strip() for value in rows[0]], rows[1:]


def package_hit_sound(data):
    """The owner's hit sound as the archive holds it, if the engine loads it and it starts at once.

    Loaded aliases must be PCM WAV. The sound must reach a tenth of its peak
    within 50 ms, to land with the hit marker, and last at most 2 s. The browser
    plays a mono 2D sound at about 0.7 of its level on each side, through the
    stereo panner's centre, and each channel of a stereo one at full level, so a
    mono sound is stored as stereo, both channels the same."""
    def invalid(reason):
        return ValueError(f'{HIT_SOUND_OWNER_FILE}: {reason}')

    if len(data) < 12 or data[:4] != b'RIFF' or data[8:12] != b'WAVE':
        raise invalid('not a WAV file')
    position, fmt, samples = 12, None, None
    while position + 8 <= len(data):
        chunk = data[position:position + 4]
        size = struct.unpack_from('<I', data, position + 4)[0]
        start = position + 8
        if size > len(data) - start:
            raise invalid('truncated chunk')
        if chunk == b'fmt ' and size >= 16:
            fmt = struct.unpack_from('<HHIIHH', data, start)
        elif chunk == b'data':
            samples = data[start:start + size]
        position = start + size + (size & 1)
    if fmt is None or samples is None:
        raise invalid('missing format or sample data')
    tag, channels, rate, _, align, bits = fmt
    if tag != 1 or bits != 16 or channels not in (1, 2) or not 11025 <= rate <= 48000 or align != 2 * channels:
        raise invalid('use 16-bit PCM, mono or stereo, at 11025 to 48000 Hz')
    if not samples or len(samples) % align:
        raise invalid('empty or partial sample data')
    if len(samples) // align > 2 * rate:
        raise invalid('longer than 2 s')
    values = struct.unpack(f'<{len(samples) // 2}h', samples)
    peak = max(abs(value) for value in values)
    if not peak:
        raise invalid('silent')
    onset = next(index for index, value in enumerate(values) if abs(value) * 10 >= peak) // channels
    if onset * 20 > rate:
        raise invalid(f'starts with {onset * 1000 // rate} ms of silence; trim it so the hit is heard at once')
    if channels == 2:
        return data
    stereo = bytearray(2 * len(samples))
    for byte in (0, 1):
        stereo[byte::4] = stereo[byte + 2::4] = samples[byte::2]
    fmt = struct.pack('<HHIIHH', 1, 2, rate, 4 * rate, 4, 16)
    body = b'WAVE' + b'fmt ' + struct.pack('<I', len(fmt)) + fmt + b'data' + struct.pack('<I', len(stereo)) + stereo
    return b'RIFF' + struct.pack('<I', len(body)) + body


def mp3_spans(data):
    """An MP3's ID3v2 tag length, rate, channels and each frame's start, length and seconds, walked as the
    engine's WebAudio_Mp3Info walks them before the browser decodes the stream, or None for a tag it refuses."""
    rates = (44100, 48000, 32000)
    v1 = (0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0)
    v2 = (0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0)
    position = tag = rate = channels = 0
    spans = []
    if len(data) >= 10 and data[:3] == b'ID3':
        if any(byte & 0x80 for byte in data[6:10]):
            return None
        position = tag = 10 + (data[6] << 21) + (data[7] << 14) + (data[8] << 7) + data[9]
    while position + 4 <= len(data):
        header = int.from_bytes(data[position:position + 4], 'big')
        version, layer, bitrate, rate_index = header >> 19 & 3, header >> 17 & 3, header >> 12 & 15, header >> 10 & 3
        if header & 0xffe00000 != 0xffe00000 or version == 1 or layer != 1 or rate_index == 3 or bitrate in (0, 15):
            position += 1
            continue
        hz = rates[rate_index] // (1 if version == 3 else 2 if version == 2 else 4)
        length = (144000 if version == 3 else 72000) * (v1 if version == 3 else v2)[bitrate] // hz + (header >> 9 & 1)
        if length < 4 or length > len(data) - position:
            break
        if not spans:
            rate, channels = hz, 1 if header >> 6 & 3 == 3 else 2
        spans.append((position, length, (1152 if version == 3 else 576) / hz))
        position += length
    return tag, rate, channels, spans


def mp3_frames(data):
    """An MP3's ID3v2 tag length, rate, channels, frames, seconds and frame bytes, read as the engine's
    WebAudio_Mp3Info reads them before the browser decodes the stream, or None for a tag it refuses."""
    walked = mp3_spans(data)
    if walked is None:
        return None
    tag, rate, channels, spans = walked
    seconds = 0.0
    for _, _, duration in spans:
        seconds += duration
    return tag, rate, channels, len(spans), seconds, sum(length for _, length, _ in spans)


def vbr_header(data, start):
    """Whether the frame at start holds an encoder's Xing, Info or VBRI header, which counts the whole file's
    frames, in place of audio."""
    header = int.from_bytes(data[start:start + 4], 'big')
    mono = header >> 6 & 3 == 3
    side = (17 if mono else 32) if header >> 19 & 3 == 3 else (9 if mono else 17)
    xing = start + 4 + (0 if header >> 16 & 1 else 2) + side
    return data[xing:xing + 4] in (b'Xing', b'Info') or data[start + 36:start + 40] == b'VBRI'


def lastminute_owner_file(main):
    """The owner's track for the last minute of a match in the folder main: their lastminutemusic.mp3, else the
    countdown's prematchmusic.mp3; None without either."""
    for name in (LASTMINUTE_MUSIC_OWNER_FILE, PREMATCH_MUSIC_OWNER_FILE):
        if (main / name).is_file():
            return main / name
    return None


def package_prematch_music(data, limit=PREMATCH_MUSIC_SECONDS, owner=PREMATCH_MUSIC_OWNER_FILE, skip=0):
    """The owner's countdown music, or other owner music given its limit and file name, as the archive holds it:
    its MPEG audio layer III frames for limit seconds from the first frame skip seconds in, without its tags or an
    encoder's header frame, which would count frames no longer there."""
    def invalid(reason):
        return ValueError(f'{owner}: {reason}')

    walked = mp3_spans(data)
    if walked is None or not walked[3]:
        raise invalid('not an MP3 file')
    tag, _, _, spans = walked
    audio = len(data) - tag - (128 if data[-128:-125] == b'TAG' else 0)
    if sum(length for _, length, _ in spans) * 10 < audio * 9:
        raise invalid('not MPEG audio layer III throughout')
    if vbr_header(data, spans[0][0]):
        spans = spans[1:]
    kept, seconds, skipped = [], 0.0, 0.0
    for start, length, duration in spans:
        if skipped < skip:
            skipped += duration
            continue
        if seconds >= limit:
            break
        kept.append(data[start:start + length])
        seconds += duration
    if seconds < 1:
        raise invalid(f'shorter than {skip + 1} s')
    return b''.join(kept)


def check_menu_music(data):
    """Return the owner's menu music if the engine opens it as a stream and the browser can hold it decoded.

    It must be MPEG audio layer III, frames from its ID3v2 tag on but for an
    ID3v1 tag at its end, from 1 s to MENU_MUSIC_MAX_SECONDS long."""
    def invalid(reason):
        return ValueError(f'{MENU_MUSIC_OWNER_FILE}: {reason}')

    info = mp3_frames(data)
    if info is None or not info[3]:
        raise invalid('not an MP3 file')
    tag, _, _, _, seconds, covered = info
    audio = len(data) - tag - (128 if data[-128:-125] == b'TAG' else 0)
    if covered * 10 < audio * 9:
        raise invalid('not MPEG audio layer III throughout')
    if not 1 <= seconds <= MENU_MUSIC_MAX_SECONDS:
        raise invalid(f'{seconds:.0f} s long; use 1 to {MENU_MUSIC_MAX_SECONDS} s')
    return data


def check_owner_sound(data, name):
    """Return a file of the owner's sound pack if the engine loads it: a 16-bit PCM WAV, mono or stereo, at 11025
    to 48000 Hz, at most OWNER_SOUNDS_MAX_SECONDS long."""
    def invalid(reason):
        return ValueError(f'{OWNER_SOUNDS_DIR}/{name}: {reason}')

    if data[:4] != b'RIFF' or data[8:12] != b'WAVE':
        raise invalid('not a WAV file')
    position, fmt, frames = 12, None, None
    while position + 8 <= len(data):
        kind, size = data[position:position + 4], struct.unpack_from('<I', data, position + 4)[0]
        if position + 8 + size > len(data):
            raise invalid('a chunk runs past the end of the file')
        if kind == b'fmt ' and size >= 16:
            fmt = struct.unpack_from('<HHIIHH', data, position + 8)
        elif kind == b'data':
            frames = size
        position += 8 + size + (size & 1)
    if fmt is None or frames is None:
        raise invalid('missing format or sample data')
    tag, channels, rate, _, align, bits = fmt
    if (tag, bits) != (1, 16) or channels not in (1, 2) or rate < 11025 or rate > 48000 or align != 2 * channels:
        raise invalid('use 16-bit PCM, mono or stereo, at 11025 to 48000 Hz')
    if not frames or frames % align:
        raise invalid('empty or partial sample data')
    if frames > OWNER_SOUNDS_MAX_SECONDS * rate * align:
        raise invalid(f'longer than {OWNER_SOUNDS_MAX_SECONDS} s')
    return data


def load_owner_sounds(folder):
    """The owner's sound pack in folder, or None without its list: for each alias, its files in order with the
    alias columns they set, and each file's content."""
    listing = folder / OWNER_SOUNDS_LIST
    if not listing.is_file():
        return None
    rows = [row for row in csv.reader(io.StringIO(listing.read_text('utf-8-sig')))
            if row and row[0].strip() and not row[0].lstrip().startswith('#')]
    header = [value.lower().strip() for value in rows[0]]
    if header[:2] != ['alias', 'file'] or len(set(header)) != len(header) or 'name' in header:
        raise ValueError(f'{OWNER_SOUNDS_DIR}/{OWNER_SOUNDS_LIST}: start with alias,file columns, named once each')
    aliases, files = {}, {}
    for row in rows[1:]:
        fields = dict(zip(header, (value.strip() for value in row)))
        alias, name = fields.pop('alias').lower(), fields.pop('file').replace('\\', '/').lower()
        if not alias or not re.fullmatch(r'[a-z0-9_]+(/[a-z0-9_]+)*\.wav', name):
            raise ValueError(f'{OWNER_SOUNDS_DIR}/{OWNER_SOUNDS_LIST}: bad row {row}')
        if name not in files:
            path = folder / name
            if not path.is_file():
                raise ValueError(f'{OWNER_SOUNDS_DIR}/{name}: missing')
            files[name] = check_owner_sound(path.read_bytes(), name)
        aliases.setdefault(alias, []).append((name, {key: value for key, value in fields.items() if value}))
    return {'aliases': aliases, 'files': files}


def owner_sound_row(header, fields, name, values, sequence=None):
    """An original row of an alias playing a file of the owner's pack, with the columns the pack sets. The engine
    drops rows of an alias that share a sequence number, so an alias of several rows numbers them unless the pack
    does."""
    columns = dict(fields, **values, file=OWNER_SOUNDS_FOLDER + name)
    if sequence is not None and 'sequence' not in values:
        columns['sequence'] = str(sequence)
    return [columns.get(column, '') for column in header]


def menu_music_row(header, fields):
    """An original menu music row streaming the owner's music, at MENU_MUSIC_VOLUME."""
    values = dict(fields, file=MENU_MUSIC_FILE, vol_min=MENU_MUSIC_VOLUME, vol_max=MENU_MUSIC_VOLUME)
    return [values.get(name, '') for name in header]


def prematch_music_row(header, fields, alias=PREMATCH_MUSIC_ALIAS, file=PREMATCH_MUSIC_FILE, volume=None):
    """The retail mission opening music's row, streaming the owner's countdown music, or other owner music as its
    alias, at once in every multiplayer map, and at volume when given in place of the row's own."""
    values = dict(fields, name=alias, file=file, loadspec='all_mp', startdelay='')
    if volume is not None:
        values.update(vol_min=volume, vol_max=volume)
    return [values.get(name, '') for name in header]


def hit_sound_row(header, fields):
    """An original MP_hit_alert row playing the owner's sound, without its delay."""
    values = dict(fields, file=HIT_SOUND_FILE, startdelay='', secondaryaliasname='')
    return [values.get(name, '') for name in header]


def verify_hit_sound(entries, original_rows, owner):
    """Check that a package plays the owner's sound, and only it, for MP_hit_alert.

    original_rows holds (header, fields) for every original MP_hit_alert row;
    owner is the owner's file content, or None without one."""
    table = entries.get(HIT_SOUND_TABLE)
    sound = entries.get('sound/' + HIT_SOUND_FILE)
    if owner is None:
        assert table is None and sound is None, 'Hit sound packaged without the owner file'
        return
    assert sound == package_hit_sound(owner), 'Packaged hit sound differs from the owner file'
    assert table is not None, 'Missing the hit sound alias'
    header, rows = alias_table(table.decode('utf-8'))
    assert len(rows) == 1 and any(header == original_header and rows[0] == hit_sound_row(original_header, fields)
                                  for original_header, fields in original_rows), \
        'The hit sound alias differs from the original MP_hit_alert'
    for name, data in entries.items():
        if name.startswith('soundaliases/') and name.endswith('.csv') and name != HIT_SOUND_TABLE:
            other_header, other_rows = alias_table(data.decode('utf-8'))
            assert all(dict(zip(other_header, row)).get('name', '').lower() != HIT_SOUND_ALIAS
                       for row in other_rows), 'Original MP_hit_alert kept beside the hit sound: ' + name


def verify_menu_music(entries, original_rows, owner):
    """Check that a package streams the owner's music, and only it, for the menus' music aliases.

    original_rows holds (header, fields) for every original row of those
    aliases; owner is the owner's file content, or None without one."""
    table = entries.get(MENU_MUSIC_TABLE)
    music = entries.get('sound/' + MENU_MUSIC_FILE)
    if owner is None:
        assert table is None and music is None, 'Menu music packaged without the owner file'
        return
    assert music == check_menu_music(owner), 'Packaged menu music differs from the owner file'
    assert table is not None, 'Missing the menu music aliases'
    header, rows = alias_table(table.decode('utf-8'))
    names = [dict(zip(header, row)).get('name', '').lower() for row in rows]
    assert names == list(MENU_MUSIC_ALIASES) and all(
        any(original_header == header and fields.get('name', '').lower() == name and row == menu_music_row(header, fields)
            for original_header, fields in original_rows)
        for name, row in zip(names, rows)), 'The menu music aliases differ from the originals'
    for name, data in entries.items():
        if name.startswith('soundaliases/') and name.endswith('.csv') and name != MENU_MUSIC_TABLE:
            other_header, other_rows = alias_table(data.decode('utf-8'))
            assert all(dict(zip(other_header, row)).get('name', '').lower() not in MENU_MUSIC_ALIASES
                       for row in other_rows), 'Original menu music kept beside the owner\'s: ' + name


def verify_owner_music(entries, source_rows, owner, table_name, file_name, alias, limit, owner_file, what, skip=0,
                       volume=None):
    """Check that a package streams the owner's music, and only it, as its alias.

    source_rows holds (header, fields) for every original row of the mission
    opening music; owner is the owner's file content, or None without one."""
    table = entries.get(table_name)
    music = entries.get('sound/' + file_name)
    if owner is None:
        assert table is None and music is None, f'{what} packaged without the owner file'
        return
    assert music == package_prematch_music(owner, limit, owner_file, skip), f'Packaged {what} differs from the owner file'
    assert table is not None, f'Missing the {what} alias'
    header, rows = alias_table(table.decode('utf-8'))
    assert len(rows) == 1 and any(source_header == header and rows[0] == prematch_music_row(header, fields, alias,
                                                                                            file_name, volume)
                                  for source_header, fields in source_rows), \
        f'The {what} alias differs from the original mission opening music'
    for name, data in entries.items():
        if name.startswith('soundaliases/') and name.endswith('.csv') and name != table_name:
            other_header, other_rows = alias_table(data.decode('utf-8'))
            assert all(dict(zip(other_header, row)).get('name', '').lower() != alias
                       for row in other_rows), f'The {what} alias is in another table: ' + name


def verify_prematch_music(entries, source_rows, owner):
    verify_owner_music(entries, source_rows, owner, PREMATCH_MUSIC_TABLE, PREMATCH_MUSIC_FILE, PREMATCH_MUSIC_ALIAS,
                       PREMATCH_MUSIC_SECONDS, PREMATCH_MUSIC_OWNER_FILE, 'countdown music',
                       volume=PREMATCH_MUSIC_VOLUME)


def verify_endgame_music(entries, source_rows, owner):
    verify_owner_music(entries, source_rows, owner, ENDGAME_MUSIC_TABLE, ENDGAME_MUSIC_FILE, ENDGAME_MUSIC_ALIAS,
                       ENDGAME_MUSIC_SECONDS, ENDGAME_MUSIC_OWNER_FILE, 'end of match music')


def verify_lastminute_music(entries, source_rows, owner):
    verify_owner_music(entries, source_rows, owner, LASTMINUTE_MUSIC_TABLE, LASTMINUTE_MUSIC_FILE,
                       LASTMINUTE_MUSIC_ALIAS, LASTMINUTE_MUSIC_SECONDS, LASTMINUTE_MUSIC_OWNER_FILE,
                       'last minute music', LASTMINUTE_MUSIC_START, LASTMINUTE_MUSIC_VOLUME)


def verify_owner_sounds(entries, original_rows, owner):
    """Check that a package plays the owner's pack, and only it, for the aliases the pack lists.

    original_rows maps each alias to (header, fields) for its original rows; owner is the loaded pack, or None
    without one."""
    tables = {name: data for name, data in entries.items()
              if name.startswith(OWNER_SOUNDS_TABLE[:-4]) and name.endswith('.csv')}
    sounds = {name: data for name, data in entries.items() if name.startswith('sound/' + OWNER_SOUNDS_FOLDER)}
    if owner is None:
        assert not tables and not sounds, 'Owner sounds packaged without the owner pack'
        return
    used = {name for entries_of_alias in owner['aliases'].values() for name, _ in entries_of_alias}
    assert sounds == {'sound/' + OWNER_SOUNDS_FOLDER + name: owner['files'][name] for name in used}, \
        'Packaged owner sounds differ from the owner pack'
    played = {}
    for data in tables.values():
        header, rows = alias_table(data.decode('utf-8'))
        for row in rows:
            played.setdefault(dict(zip(header, row)).get('name', '').lower(), []).append((header, row))
    assert sorted(played) == sorted(owner['aliases']), 'Owner sound aliases differ from the owner pack'
    for alias, wanted in owner['aliases'].items():
        rows = played[alias]
        assert len(rows) == len(wanted) and all(
            any(original_header == header and
                row == owner_sound_row(header, fields, name, values, number + 1 if len(wanted) > 1 else None)
                for original_header, fields in original_rows.get(alias, []))
            for number, ((header, row), (name, values)) in enumerate(zip(rows, wanted))), \
            'Owner sound rows differ for ' + alias
    for name, data in entries.items():
        if name.startswith('soundaliases/') and name.endswith('.csv') and name not in tables:
            other_header, other_rows = alias_table(data.decode('utf-8'))
            assert all(dict(zip(other_header, row)).get('name', '').lower() not in owner['aliases']
                       for row in other_rows), 'Original rows kept beside the owner pack: ' + name


def explosive_alias_table(rows_by_alias):
    """The explosives' table: its header and rows, each new alias starting
    from its retail source's rows. rows_by_alias maps an alias to a list of
    (header, fields) for its original rows."""
    headers = {tuple(header) for source in ALIASES.values() for header, _ in rows_by_alias.get(source, [])}
    if len(headers) != 1:
        raise ValueError('The explosives\' source aliases need one original alias table layout')
    header = list(headers.pop())
    return header, alias_rows(header, lambda source: [fields for _, fields in rows_by_alias.get(source, [])])


def add_explosive_aliases(tables, aliases):
    rows_by_alias = {alias: [(tables[table][0], fields) for table, _, fields in entries]
                     for alias, entries in aliases.items()}
    header, rows = explosive_alias_table(rows_by_alias)
    tables[ALIAS_TABLE] = (header, [])
    for row in rows:
        fields = dict(zip(header, row))
        aliases.setdefault(fields['name'].lower(), []).append((ALIAS_TABLE, row, fields))


def collect_sound_assets(root, asset_index, originals, renderer, maps, hit_sound=None, menu_music=None,
                         owner_sounds=None, prematch_music=None, endgame_music=None, lastminute_music=None):
    def read(name):
        path, original_name = asset_index[name]
        return originals[path].read(original_name)

    tables, aliases = {}, {}
    for name in sorted(asset_index):
        if not name.startswith('soundaliases/') or not name.endswith('.csv'):
            continue
        header, rows = alias_table(read(name).decode('utf-8-sig', 'replace'))
        tables[name] = (header, [])
        for row in rows:
            fields = dict(zip(header, row))
            alias = fields.get('name', '').lower()
            if alias:
                aliases.setdefault(alias, []).append((name, row, fields))
    add_explosive_aliases(tables, aliases)

    # Authored weapon/animation/FX/menu/script references and engine constants.
    references = {'null', 'ambient_africa', 'ambient_france', 'music_mainmenu'}
    for name, data in renderer.items():
        if not name.startswith(('images/', 'xmodelsurfs/', 'xmodelparts/', 'materials/')):
            references.update(token.decode('ascii').lower() for token in
                              re.findall(rb'[A-Za-z0-9_]+', data))
    for folder in ('cgame_mp', 'ui_mp', 'bgame'):
        for path in (root / 'src/PC' / folder).glob('*.c'):
            references.update(re.findall(r'"([A-Za-z0-9_]+)"', path.read_text()))
    cgame = (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()
    surface_prefixes = re.findall(r'CG_RegisterSurfaceSoundAliases\([^;\"]+"([^"\r\n]+)"', cgame)
    references.update(alias for alias in aliases
                      if any(alias.startswith(prefix + '_') for prefix in surface_prefixes))
    # Dynamic quick-message names are assembled by faction at runtime.
    references.update(alias for alias in aliases if alias.startswith(('uk_mp_', 'us_mp_', 'ge_mp_')))
    pending = sorted(references & aliases.keys())
    chosen, files, missing, replaced, music, owned = set(), {}, set(), [], {}, {}
    pack = owner_sounds['aliases'] if owner_sounds is not None else {}
    if (hit_sound is not None and HIT_SOUND_ALIAS in pack or
            menu_music is not None and set(MENU_MUSIC_ALIASES) & set(pack)):
        raise ValueError('The owner sound pack lists an alias the owner hit sound or menu music replaces')
    while pending:
        alias = pending.pop()
        if alias in chosen:
            continue
        chosen.add(alias)
        for table, row, fields in aliases[alias]:
            spec = fields.get('loadspec', '').lower().split()
            if spec and not spec[0].startswith('!') and not set(spec) & {'all_mp', 'menu', *maps}:
                continue
            if '!all_mp' in spec or all('!' + mapname in spec for mapname in maps):
                continue
            if hit_sound is not None and alias == HIT_SOUND_ALIAS:
                replaced.append((tables[table][0], fields))
                continue
            if menu_music is not None and alias in MENU_MUSIC_ALIASES:
                music.setdefault(alias, (tables[table][0], fields))
                continue
            secondary = fields.get('secondaryaliasname', '').lower()
            if alias in pack:
                owned.setdefault(alias, []).append((table, fields))
                if secondary in aliases:
                    pending.append(secondary)
                continue
            sound = 'sound/' + fields.get('file', '').replace('\\', '/').lower()
            if sound not in asset_index:
                missing.add(sound)
                continue
            tables[table][1].append(row)
            files[sound] = read(sound)
            if secondary in aliases:
                pending.append(secondary)
    if missing:
        raise ValueError('Missing original sound dependencies: ' + ', '.join(sorted(missing)))
    if hit_sound is not None:
        if not replaced:
            raise ValueError('No original MP_hit_alert alias for the owner hit sound to replace')
        header, fields = replaced[0]
        output = io.StringIO(newline='')
        writer = csv.writer(output, lineterminator='\r\n')
        writer.writerow(header)
        writer.writerow(hit_sound_row(header, fields))
        files[HIT_SOUND_TABLE] = output.getvalue().encode('utf-8')
        files['sound/' + HIT_SOUND_FILE] = package_hit_sound(hit_sound)
        print(f'Hit confirmation: the owner\'s {HIT_SOUND_OWNER_FILE} replaces MP_hit_alert')
    if menu_music is not None:
        if len(music) != len(MENU_MUSIC_ALIASES):
            raise ValueError('No original ' + ', '.join(sorted(set(MENU_MUSIC_ALIASES) - set(music))) +
                             ' alias for the owner menu music to replace')
        header = music[MENU_MUSIC_ALIASES[0]][0]
        if any(music[alias][0] != header for alias in MENU_MUSIC_ALIASES):
            raise ValueError('The original menu music aliases have different columns')
        output = io.StringIO(newline='')
        writer = csv.writer(output, lineterminator='\r\n')
        writer.writerow(header)
        writer.writerows(menu_music_row(header, music[alias][1]) for alias in MENU_MUSIC_ALIASES)
        files[MENU_MUSIC_TABLE] = output.getvalue().encode('utf-8')
        files['sound/' + MENU_MUSIC_FILE] = check_menu_music(menu_music)
        print(f'Menu music: the owner\'s {MENU_MUSIC_OWNER_FILE} replaces {" and ".join(MENU_MUSIC_ALIASES)}')
    if prematch_music is not None:
        if PREMATCH_MUSIC_SOURCE not in aliases:
            raise ValueError(f'No original {PREMATCH_MUSIC_SOURCE} alias for the owner countdown music to copy')
        table, _, fields = aliases[PREMATCH_MUSIC_SOURCE][0]
        header = tables[table][0]
        output = io.StringIO(newline='')
        writer = csv.writer(output, lineterminator='\r\n')
        writer.writerow(header)
        writer.writerow(prematch_music_row(header, fields, volume=PREMATCH_MUSIC_VOLUME))
        files[PREMATCH_MUSIC_TABLE] = output.getvalue().encode('utf-8')
        files['sound/' + PREMATCH_MUSIC_FILE] = package_prematch_music(prematch_music)
        print(f'Countdown music: the owner\'s {PREMATCH_MUSIC_OWNER_FILE} streams as {PREMATCH_MUSIC_ALIAS}')
    if endgame_music is not None:
        if PREMATCH_MUSIC_SOURCE not in aliases:
            raise ValueError(f'No original {PREMATCH_MUSIC_SOURCE} alias for the owner end of match music to copy')
        table, _, fields = aliases[PREMATCH_MUSIC_SOURCE][0]
        header = tables[table][0]
        output = io.StringIO(newline='')
        writer = csv.writer(output, lineterminator='\r\n')
        writer.writerow(header)
        writer.writerow(prematch_music_row(header, fields, ENDGAME_MUSIC_ALIAS, ENDGAME_MUSIC_FILE))
        files[ENDGAME_MUSIC_TABLE] = output.getvalue().encode('utf-8')
        files['sound/' + ENDGAME_MUSIC_FILE] = package_prematch_music(endgame_music, ENDGAME_MUSIC_SECONDS,
                                                                      ENDGAME_MUSIC_OWNER_FILE)
        print(f'End of match music: the owner\'s {ENDGAME_MUSIC_OWNER_FILE} streams as {ENDGAME_MUSIC_ALIAS}')
    if lastminute_music is not None:
        if PREMATCH_MUSIC_SOURCE not in aliases:
            raise ValueError(f'No original {PREMATCH_MUSIC_SOURCE} alias for the owner last minute music to copy')
        table, _, fields = aliases[PREMATCH_MUSIC_SOURCE][0]
        header = tables[table][0]
        output = io.StringIO(newline='')
        writer = csv.writer(output, lineterminator='\r\n')
        writer.writerow(header)
        writer.writerow(prematch_music_row(header, fields, LASTMINUTE_MUSIC_ALIAS, LASTMINUTE_MUSIC_FILE,
                                           LASTMINUTE_MUSIC_VOLUME))
        files[LASTMINUTE_MUSIC_TABLE] = output.getvalue().encode('utf-8')
        files['sound/' + LASTMINUTE_MUSIC_FILE] = package_prematch_music(
            lastminute_music, LASTMINUTE_MUSIC_SECONDS, LASTMINUTE_MUSIC_OWNER_FILE, LASTMINUTE_MUSIC_START)
        print(f'Last minute music: the owner\'s track streams as {LASTMINUTE_MUSIC_ALIAS} '
              f'from {LASTMINUTE_MUSIC_START} s in')
    if owner_sounds is not None:
        unused = sorted(set(pack) - set(owned))
        if unused:
            raise ValueError('The owner sound pack lists aliases the supported maps do not play: ' + ', '.join(unused))
        by_table = {}
        for alias, wanted in pack.items():
            for number, (name, values) in enumerate(wanted):
                table, fields = owned[alias][number % len(owned[alias])]
                header = tables[table][0]
                unknown = sorted(set(values) - set(header))
                if unknown:
                    raise ValueError(f'The owner sound pack sets {", ".join(unknown)}, not a column of {table}')
                by_table.setdefault(table, (header, []))[1].append(
                    owner_sound_row(header, fields, name, values, number + 1 if len(wanted) > 1 else None))
                files['sound/' + OWNER_SOUNDS_FOLDER + name] = owner_sounds['files'][name]
        for number, table in enumerate(sorted(by_table)):
            header, rows = by_table[table]
            output = io.StringIO(newline='')
            writer = csv.writer(output, lineterminator='\r\n')
            writer.writerow(header)
            writer.writerows(rows)
            name = OWNER_SOUNDS_TABLE if not number else OWNER_SOUNDS_TABLE[:-4] + f'_{number + 1}.csv'
            files[name] = output.getvalue().encode('utf-8')
        print(f'Owner sound pack: {len({name for wanted in pack.values() for name, _ in wanted})} files replace '
              f'{len(pack)} aliases')
    for name, (header, rows) in tables.items():
        if rows:
            output = io.StringIO(newline='')
            writer = csv.writer(output, lineterminator='\r\n')
            writer.writerow(header)
            writer.writerows(rows)
            files[name] = output.getvalue().encode('utf-8')
    for name in asset_index:
        if name.startswith('soundaliases/') and name.endswith(('.vfcurve', '.spkrmap', '.def')):
            files[name] = read(name)
    print(f'Multiplayer sounds: {sum(name.startswith("sound/") for name in files)} files, '
          f'{sum(len(data) for data in files.values())} uncompressed bytes')
    return files
