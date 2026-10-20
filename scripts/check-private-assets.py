#!/usr/bin/env python3
"""Verify the private package scope and original resource bytes, not gameplay."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import struct
import zipfile
import csv
import io
from weapon_balance import apply_weapon_balance, load_profile
from browser_bot_menu import apply_bot_menu
from browser_class_menus import CLASS_MENU_LINE, class_menus
from browser_class_pictures import OWNER_PICTURES_DIR, PICTURE_FOLDER, load_owner_pictures, verify_owner_pictures
from browser_attachment_models import attachment_model_assets
from browser_explosives import (ALIAS_TABLE as EXPLOSIVE_ALIAS_TABLE, ALIASES as EXPLOSIVE_ALIASES,
                                WEAPONS as EXPLOSIVE_WEAPONS, explosive_assets)
from browser_settings_menu import apply_settings_menu
from browser_objective_assets import objective_asset_roots
from browser_spawn_safety import apply_spawn_safety
from browser_endgame import SCRIPTS as ENDGAME_SCRIPTS, apply_endgame_timing
from browser_sound_assets import (ENDGAME_MUSIC_FILE, ENDGAME_MUSIC_OWNER_FILE, ENDGAME_MUSIC_TABLE,
                                  HIT_SOUND_ALIAS, HIT_SOUND_FILE, HIT_SOUND_OWNER_FILE, HIT_SOUND_TABLE,
                                  LASTMINUTE_MUSIC_FILE, LASTMINUTE_MUSIC_TABLE, lastminute_owner_file,
                                  MENU_MUSIC_ALIASES, MENU_MUSIC_FILE, MENU_MUSIC_OWNER_FILE, MENU_MUSIC_TABLE,
                                  OWNER_SOUNDS_DIR, OWNER_SOUNDS_FOLDER, OWNER_SOUNDS_TABLE, PREMATCH_MUSIC_FILE,
                                  PREMATCH_MUSIC_OWNER_FILE, PREMATCH_MUSIC_SOURCE, PREMATCH_MUSIC_TABLE, alias_table,
                                  explosive_alias_table, load_owner_sounds, verify_endgame_music, verify_hit_sound,
                                  verify_lastminute_music, verify_menu_music, verify_owner_sounds,
                                  verify_prematch_music)

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--main-dir', type=Path, default=root / 'data/browser/main')
parser.add_argument('--manifest', type=Path, default=root / 'site/wasm-game-data.json')
args = parser.parse_args()
maps = {"mp_toujane": "Toujane, Tunisia", "mp_carentan": "Carentan, France",
        "mp_dawnville": "Dawnville, France"}
gametypes = ("dm", "tdm", "ctf", "hq", "sd")
balance_profile = load_profile()
policy = json.loads(args.manifest.read_text())["variants"]["cod2-mp"]
originals = [zipfile.ZipFile(p) for p in sorted((root / "data/main").glob("*.iwd"))]
try:
    index = {n.lower(): (z, n) for z in originals for n in z.namelist() if not n.endswith("/")}
    explosives = explosive_assets(lambda name: index[name][0].read(index[name][1]))
    attachment_models = attachment_model_assets(lambda name: index[name.lower()][0].read(index[name.lower()][1]))
    retail_alias_rows = {}
    for name, (source, original_name) in index.items():
        if name.startswith("soundaliases/") and name.endswith(".csv"):
            header, rows = alias_table(source.read(original_name).decode("utf-8-sig", "replace"))
            for fields in (dict(zip(header, row)) for row in rows):
                retail_alias_rows.setdefault(fields.get("name", "").lower(), []).append((header, fields))
    explosive_header, explosive_rows = explosive_alias_table(retail_alias_rows)
    entries = {}
    total = 0
    for item in policy["files"]:
        path = args.main_dir / item["name"]
        assert path.stat().st_size == item["size"], path
        assert hashlib.sha256(path.read_bytes()).hexdigest() == item["sha256"], path
        total += item["size"]
        with zipfile.ZipFile(path) as archive:
            for name in archive.namelist():
                data = archive.read(name)
                entries[name] = data
                if name in (HIT_SOUND_TABLE, "sound/" + HIT_SOUND_FILE, MENU_MUSIC_TABLE, "sound/" + MENU_MUSIC_FILE,
                            PREMATCH_MUSIC_TABLE, "sound/" + PREMATCH_MUSIC_FILE, ENDGAME_MUSIC_TABLE,
                            "sound/" + ENDGAME_MUSIC_FILE, LASTMINUTE_MUSIC_TABLE, "sound/" + LASTMINUTE_MUSIC_FILE):
                    continue
                if name.startswith(("sound/" + OWNER_SOUNDS_FOLDER, OWNER_SOUNDS_TABLE[:-4], PICTURE_FOLDER)):
                    continue
                if name in class_menus():
                    assert data == class_menus()[name], name
                    continue
                if name in explosives:
                    assert data == explosives[name], name
                    continue
                if name in attachment_models:
                    assert data == attachment_models[name], name
                    continue
                if name == EXPLOSIVE_ALIAS_TABLE:
                    selected_rows = list(csv.reader(io.StringIO(data.decode("utf-8"))))
                    assert selected_rows[0] == explosive_header, name
                    assert all(row in explosive_rows for row in selected_rows[1:]), name
                    continue
                if name.endswith('.arena'):
                    mapname = 'mp_' + Path(name).stem
                    assert mapname in maps, name
                    assert data == f'{{ map "{mapname}" longname "{maps[mapname]}" gametype "{" ".join(gametypes)}" }}\n'.encode()
                    continue
                source, original_name = index[name]
                original = source.read(original_name)
                original = apply_settings_menu(name, original)
                if name == "default_mp.cfg":
                    expected = original.replace(b'bind SHIFT\t"+melee_breath"', b'bind SHIFT\t"+sprint"')
                    expected = expected.replace(b'bind v\t"mp_QuickMessage"',
                                                b'bind v\t"+melee"\r\nbind h "mp_QuickMessage"')
                    expected = expected.replace(b'bind c\t\t"gocrouch"',
                                                b'bind c\t\t"togglecrouch"')
                    assert data == expected, name
                elif name == "ui/options_shoot.menu":
                    expected = original.replace(b'"@MENU_MELEE_STEADY"', b'"Sprint / Hold Breath"')
                    expected = expected.replace(b'"+melee_breath"', b'"+sprint"')
                    assert data == expected, name
                elif name.startswith("weapons/mp/"):
                    assert data == apply_weapon_balance(name, original, balance_profile), name
                elif name == "maps/mp/gametypes/_spawnlogic.gsc":
                    assert data == apply_spawn_safety(name, original), name
                elif name == "maps/mp/gametypes/tdm.gsc":
                    # Existing reviewed death/spawn UI deadline adaptation.
                    assert original.count(b"delay = 2;") == 1
                    assert original.count(b'self.sessionstate = "playing";') == 1
                    expected = original.replace(b"delay = 2;", b'delay = 3;\n\tself setClientCvar("cg_respawnDeadline", getTime() + 3000);')
                    expected = expected.replace(b'self.sessionstate = "playing";', b'self.sessionstate = "playing";\n\tself setClientCvar("cg_respawnDeadline", 0);')
                    assert data == apply_endgame_timing(name, expected), name
                elif name in ENDGAME_SCRIPTS:
                    assert data == apply_endgame_timing(name, original), name
                elif name == "ui_mp/createserver.menu":
                    assert data == apply_bot_menu(original), name
                elif name == "ui_mp/joinserver.menu":
                    # Strip the reviewed owner action and compare every original byte.
                    normalized = original.replace(b'\r\n', b'\n')
                    start = data.index(b'\n        itemDef {\n            name deleteOwnServer')
                    end = data.index(b'\n//\tBACK AND NEXT BUTTONS', start)
                    button = data[start:end]
                    assert b'text "Delete my server"' in button
                    assert b'dvarTest "ui_canDeleteServer"' in button
                    assert b'enableDvar { "1" }' in button
                    assert b'uiScript DeleteServer;' in button
                    assert data[:start] + data[end:] == normalized, name
                elif name == "ui_mp/menus.txt":
                    original_lines = set(original.splitlines())
                    assert all(line in original_lines or line == CLASS_MENU_LINE for line in data.splitlines()), name
                    assert data.splitlines().count(CLASS_MENU_LINE) == 1, name
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
    assert sorted(n for n in entries if n.endswith(".d3dbsp")) == sorted(
        f"maps/mp/{mapname}.d3dbsp" for mapname in maps)
    assert sorted(n for n in entries if n.endswith('.arena')) == sorted(
        f'mp/{mapname[3:]}.arena' for mapname in maps)
    modes = [n for n in entries if n.startswith("maps/mp/gametypes/")
             and n.endswith(".gsc") and not Path(n).name.startswith("_")]
    assert sorted(modes) == sorted(f"maps/mp/gametypes/{mode}.gsc" for mode in gametypes), modes
    for mode in gametypes:
        for dependency in (f"maps/mp/gametypes/{mode}.txt",
                           f"ui_mp/scriptmenus/serverinfo_{mode}.menu",
                           f"ui_mp/settings_{mode}.menu"):
            assert dependency in entries, "Missing game mode dependency: " + dependency
    for name in objective_asset_roots(index):
        assert name in entries, "Missing dynamic objective resource: " + name
    # Custom classes carry any faction's weapon on every map.
    classes = json.loads((root / "downstream/gateway/classes.json").read_text())
    for weapon in classes["primary"] + classes["secondary"] + classes["lethal"] + classes["tactical"]:
        assert "weapons/mp/" + weapon.lower() in entries, "Missing class weapon: " + weapon
    for name in class_menus():
        assert name in entries, "Missing class menu: " + name
    owner_pictures = load_owner_pictures(root / "data/main" / OWNER_PICTURES_DIR, classes["primary"] +
                                         classes["secondary"] + classes["lethal"] + classes["tactical"],
                                         classes["attachments"])
    verify_owner_pictures(entries, owner_pictures)
    for name in list(explosives) + ["weapons/mp/" + weapon for weapon in EXPLOSIVE_WEAPONS]:
        assert name in entries, "Missing explosive resource: " + name
    for name in attachment_models:
        assert name in entries, "Missing attachment model: " + name
    aliases = {}
    for name, data in entries.items():
        if name.startswith("soundaliases/") and name.endswith(".csv"):
            for row in csv.DictReader(io.StringIO(data.decode())):
                aliases.setdefault(row["name"].lower(), []).append(row)
                sound = "sound/" + row["file"].replace("\\", "/").lower()
                assert sound in entries, "Missing sound file: " + sound
    original_hit_rows, original_music_rows, original_prematch_rows, original_rows = [], [], [], {}
    for name, (source, original_name) in index.items():
        if name.startswith("soundaliases/") and name.endswith(".csv"):
            header, rows = alias_table(source.read(original_name).decode("utf-8-sig", "replace"))
            for fields in (dict(zip(header, row)) for row in rows):
                alias = fields.get("name", "").lower()
                original_rows.setdefault(alias, []).append((header, fields))
                if alias == HIT_SOUND_ALIAS:
                    original_hit_rows.append((header, fields))
                elif alias in MENU_MUSIC_ALIASES:
                    original_music_rows.append((header, fields))
                elif alias == PREMATCH_MUSIC_SOURCE:
                    original_prematch_rows.append((header, fields))
    # The explosives' aliases stand in for originals of their own.
    for row in explosive_rows:
        fields = dict(zip(explosive_header, row))
        original_rows.setdefault(fields["name"].lower(), []).append((explosive_header, fields))
    owner_hit_sound = root / "data/main" / HIT_SOUND_OWNER_FILE
    verify_hit_sound(entries, original_hit_rows,
                     owner_hit_sound.read_bytes() if owner_hit_sound.is_file() else None)
    owner_menu_music = root / "data/main" / MENU_MUSIC_OWNER_FILE
    verify_menu_music(entries, original_music_rows,
                      owner_menu_music.read_bytes() if owner_menu_music.is_file() else None)
    owner_prematch_music = root / "data/main" / PREMATCH_MUSIC_OWNER_FILE
    verify_prematch_music(entries, original_prematch_rows,
                          owner_prematch_music.read_bytes() if owner_prematch_music.is_file() else None)
    owner_endgame_music = root / "data/main" / ENDGAME_MUSIC_OWNER_FILE
    verify_endgame_music(entries, original_prematch_rows,
                         owner_endgame_music.read_bytes() if owner_endgame_music.is_file() else None)
    owner_lastminute_music = lastminute_owner_file(root / "data/main")
    verify_lastminute_music(entries, original_prematch_rows,
                            owner_lastminute_music.read_bytes() if owner_lastminute_music else None)
    owner_sounds = load_owner_sounds(root / "data/main" / OWNER_SOUNDS_DIR)
    verify_owner_sounds(entries, original_rows, owner_sounds)
    # From the explosives' own table, or the owner's where the pack replaces them.
    for name in EXPLOSIVE_ALIASES:
        assert name in aliases, "Missing explosive sound alias: " + name
    for name in ("weap_sten_fire_plr", "weap_mp40_fire", "step_run_plr_default",
                 "bullet_small_flesh", "ambient_africa", "ambient_france", "music_mainmenu_mp",
                 "us_mp_cmd_followme"):
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
                 "ui_mp/scriptmenus/team_americangerman.menu",
                 "ui_mp/scriptmenus/weapon_british.menu",
                 "ui_mp/scriptmenus/weapon_american.menu",
                 "ui_mp/scriptmenus/weapon_german.menu",
                 "ui_mp/scriptmenus/serverinfo_tdm.menu"):
        assert name in entries, "Missing client dependency: " + name
    for soldiertype in ("british_africa", "german_africa", "american_normandy", "german_normandy"):
        assert f"mptype/{soldiertype}.gsc" in entries, soldiertype
    for name, data in entries.items():
        if name.startswith('character/mp_') and any(team in name for team in ('british_africa', 'german_africa', 'american_normandy', 'german_normandy')):
            for model in re.findall(rb'precacheModel\("([^\"]+)"\)', data):
                modelname = model.decode().lower()
                assert (modelname if modelname.startswith('xmodel/') else 'xmodel/' + modelname) in entries, model
    for weapon in ('frag_grenade_american_mp', 'smoke_grenade_american_mp', 'colt_mp',
                   'm1carbine_mp', 'm1garand_mp', 'thompson_mp', 'bar_mp', 'springfield_mp', 'greasegun_mp'):
        assert 'weapons/mp/' + weapon in entries, weapon
    for mapname in maps:
        for dependency in (f'maps/mp/{mapname}.gsc', f'maps/mp/{mapname}_fx.gsc',
                           f'materials/loadscreen_{mapname}', f'materials/background_{mapname}'):
            assert dependency in entries, dependency
        bsp = entries[f"maps/mp/{mapname}.d3dbsp"]
        length, offset = struct.unpack_from("<ii", bsp, 8)
        for position in range(offset, offset + length, 72):
            material = bsp[position:position + 64].split(b"\0", 1)[0].decode().lower()
            assert "materials/" + material in entries, "Missing BSP material: " + material
    hit_sound = "the owner's" if owner_hit_sound.is_file() else "the retail"
    menu_music = "the owner's" if owner_menu_music.is_file() else "the retail"
    prematch_music = "the owner's countdown music" if owner_prematch_music.is_file() else "no countdown music"
    if owner_endgame_music.is_file():
        prematch_music += ", the owner's end of match music"
    pack = f"the owner's sounds for {len(owner_sounds['aliases'])} aliases" if owner_sounds else "no owner sound pack"
    pictures = f"the owner's {len(owner_pictures)} class pictures" if owner_pictures else "no owner class pictures"
    print(f"PASS: {len(entries)} private entries, {total} bytes, retail bytes/reviewed overrides, Toujane/Carentan/Dawnville, five retail modes, all three BSPs and factions, {hit_sound} hit sound, {menu_music} menu music, {prematch_music}, {pack}, {pictures}")
finally:
    for archive in originals:
        archive.close()
