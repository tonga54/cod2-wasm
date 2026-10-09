#!/usr/bin/env python3
"""Verify the private package scope and original resource bytes, not gameplay."""
from pathlib import Path
import hashlib
import json
import re
import struct
import zipfile
import csv
import io
from weapon_balance import apply_weapon_balance, load_profile

root = Path(__file__).resolve().parent.parent
balance_profile = load_profile()
policy = json.loads((root / "site/wasm-game-data.json").read_text())["variants"]["cod2-mp"]
originals = [zipfile.ZipFile(p) for p in sorted((root / "data/main").glob("*.iwd"))]
try:
    index = {n.lower(): (z, n) for z in originals for n in z.namelist() if not n.endswith("/")}
    entries = {}
    total = 0
    for item in policy["files"]:
        path = root / "data/browser" / item["path"]
        assert path.stat().st_size == item["size"], path
        assert hashlib.sha256(path.read_bytes()).hexdigest() == item["sha256"], path
        total += item["size"]
        with zipfile.ZipFile(path) as archive:
            for name in archive.namelist():
                data = archive.read(name)
                entries[name] = data
                if name == "mp/toujane.arena":
                    assert data == b'{ map "mp_toujane" longname "Toujane, Tunisia" gametype "tdm" }\n'
                    continue
                source, original_name = index[name]
                original = source.read(original_name)
                if name.startswith("weapons/mp/"):
                    assert data == apply_weapon_balance(name, original, balance_profile), name
                elif name == "maps/mp/gametypes/tdm.gsc":
                    # Existing reviewed death/spawn UI deadline adaptation.
                    assert original.count(b"delay = 2;") == 1
                    assert original.count(b'self.sessionstate = "playing";') == 1
                    expected = original.replace(b"delay = 2;", b'delay = 3;\n\tself setClientCvar("cg_respawnDeadline", getTime() + 3000);')
                    expected = expected.replace(b'self.sessionstate = "playing";', b'self.sessionstate = "playing";\n\tself setClientCvar("cg_respawnDeadline", 0);')
                    assert data == expected, name
                elif name == "ui_mp/menus.txt":
                    original_lines = set(original.splitlines())
                    assert all(line in original_lines for line in data.splitlines()), name
                    assert b"single_player.menu" not in data, name
                elif name.startswith("soundaliases/") and name.endswith(".csv"):
                    source_rows = [row for row in csv.reader(io.StringIO(original.decode("utf-8-sig", "replace")))
                                   if row and row[0] and not row[0].startswith("#")]
                    selected_rows = list(csv.reader(io.StringIO(data.decode("utf-8"))))
                    assert selected_rows[0] == source_rows[0], name
                    source_rows = {tuple(row) for row in source_rows[1:]}
                    assert all(tuple(row) in source_rows for row in selected_rows[1:]), name
                else:
                    assert data == original, "Modified original resource: " + name
    assert [n for n in entries if n.endswith(".d3dbsp")] == ["maps/mp/mp_toujane.d3dbsp"]
    modes = [n for n in entries if n.startswith("maps/mp/gametypes/")
             and n.endswith(".gsc") and not Path(n).name.startswith("_")]
    assert modes == ["maps/mp/gametypes/tdm.gsc"], modes
    aliases = {}
    for name, data in entries.items():
        if name.startswith("soundaliases/") and name.endswith(".csv"):
            for row in csv.DictReader(io.StringIO(data.decode())):
                aliases.setdefault(row["name"].lower(), []).append(row)
                sound = "sound/" + row["file"].replace("\\", "/").lower()
                assert sound in entries, "Missing sound file: " + sound
    for name in ("weap_sten_fire_plr", "weap_mp40_fire", "step_run_plr_default",
                 "bullet_small_flesh", "ambient_africa", "music_mainmenu_mp"):
        assert name in aliases, "Missing gameplay sound alias: " + name
    assert not any(n.startswith(("sound/voiceovers/uk/88ridge/", "sound/voiceovers/us/duhoc")) for n in entries)
    mantle_source = (root / "src/PC/bgame/bg_mantle.c").read_text()
    mantle_names = mantle_source.split("s_mantleAnimNames[] = {", 1)[1].split("};", 1)[0]
    for name in re.findall(r'"([^\"]+)"', mantle_names)[1:] + ["void"]:
        assert "xanim/" + name in entries, "Missing movement animation: " + name
    for name in ("materials/mtl_kubel_africa", "materials/mtl_barrel_silver",
                 "materials/mtl_dak_crate", "materials/mtl_dak_crate_decals",
                 "fx/misc/missing_fx.efx", "fx/iw_impacts.csv",
                 "shock/default.shock", "shock/hold_breath.shock",
                 "codescripts/delete.gsc", "codescripts/struct.gsc",
                 "ui_mp/scriptmenus/team_britishgerman.menu",
                 "ui_mp/scriptmenus/weapon_british.menu",
                 "ui_mp/scriptmenus/weapon_german.menu",
                 "ui_mp/scriptmenus/serverinfo_tdm.menu"):
        assert name in entries, "Missing client dependency: " + name
    bsp = entries["maps/mp/mp_toujane.d3dbsp"]
    length, offset = struct.unpack_from("<ii", bsp, 8)
    for position in range(offset, offset + length, 72):
        material = bsp[position:position + 64].split(b"\0", 1)[0].decode().lower()
        assert "materials/" + material in entries, "Missing BSP material: " + material
    print(f"PASS: {len(entries)} private entries, {total} bytes, retail bytes/reviewed overrides, Toujane only, TDM only, all BSP materials")
finally:
    for archive in originals:
        archive.close()
