"""Follow the supported multiplayer maps' original sound aliases."""
import csv
import io
import re


def collect_sound_assets(root, asset_index, originals, renderer, maps):
    def read(name):
        path, original_name = asset_index[name]
        return originals[path].read(original_name)

    tables, aliases = {}, {}
    for name in sorted(asset_index):
        if not name.startswith('soundaliases/') or not name.endswith('.csv'):
            continue
        rows = [row for row in csv.reader(io.StringIO(read(name).decode('utf-8-sig', 'replace')))
                if row and row[0].strip() and not row[0].lstrip().startswith('#')]
        header = [value.lower().strip() for value in rows[0]]
        tables[name] = (header, [])
        for row in rows[1:]:
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
    chosen, files, missing = set(), {}, set()
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
