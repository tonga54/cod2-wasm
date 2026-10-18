"""Follow the supported multiplayer maps' original sound aliases."""
import csv
import io
import re
import struct

# The owner may keep a hit confirmation sound of their own beside the original
# IWDs, never in Git. It replaces the sound of the scripts' MP_hit_alert.
HIT_SOUND_OWNER_FILE = 'hitmarker.wav'
HIT_SOUND_ALIAS = 'mp_hit_alert'
HIT_SOUND_FILE = 'browser/hitmarker.wav'
HIT_SOUND_TABLE = 'soundaliases/browser_hitmarker.csv'


def alias_table(text):
    """The lowercased header and the alias rows of a sound alias CSV."""
    rows = [row for row in csv.reader(io.StringIO(text))
            if row and row[0].strip() and not row[0].lstrip().startswith('#')]
    return [value.lower().strip() for value in rows[0]], rows[1:]


def package_hit_sound(data):
    """The owner's hit sound as the archive holds it, if the engine loads it and it starts at once.

    Loaded aliases must be PCM WAV. The sound must reach a tenth of its peak
    within 50 ms, to land with the hit marker, and last at most 2 s. The engine
    plays a mono 2D sound at half level on each side and a stereo one at full
    level, so a mono sound is stored as stereo, both channels the same."""
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


def collect_sound_assets(root, asset_index, originals, renderer, maps, hit_sound=None):
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
    chosen, files, missing, replaced = set(), {}, set(), []
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
            sound = 'sound/' + fields.get('file', '').replace('\\', '/').lower()
            if sound not in asset_index:
                missing.add(sound)
                continue
            tables[table][1].append(row)
            files[sound] = read(sound)
            secondary = fields.get('secondaryaliasname', '').lower()
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
