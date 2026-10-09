#!/usr/bin/env python3
"""Prepare private startup and Toujane assets; gameplay still needs verification."""
from pathlib import Path
import hashlib
import json
import re
import struct
import zipfile
from browser_sound_assets import collect_sound_assets
from weapon_balance import apply_weapon_balance, load_profile

root = Path(__file__).resolve().parent.parent
balance_profile = load_profile()
main = root / "data/browser/main"
main.mkdir(parents=True, exist_ok=True)
archives = {}
with zipfile.ZipFile(root / "data/main/iw_00.iwd") as original:
    archives["cod2_browser_bootstrap.iwd"] = {
        name: original.read(name) for name in (
            "default_mp.cfg", "configure_mp.cfg", "configure_mp.csv", "safemode_mp.cfg"
        )
    }
strings = {
    "cgame", "exe", "game", "key", "menu", "messagebox", "mp", "mpui",
    "quickmessage", "weapon", "platform", "script_platform", "pc_patch_1_1"
}
localized = {}
for path in sorted((root / "data/main").glob("localized_english_*.iwd")):
    with zipfile.ZipFile(path) as original:
        for info in original.infolist():
            name = info.filename
            if name.startswith("localizedstrings/") and Path(name).stem in strings:
                localized[name] = original.read(name)
archives["localized_english_cod2_browser.iwd"] = localized

# Follow asset names from the original material/font blobs and DX7 text files.
# Later IWDs override earlier ones, exactly as in the engine's search path.
# Keep these private; no original file is copied into the public site or image.
asset_index = {}
originals = {}
try:
    # Localized IWDs also hold map models/materials and their image data.
    for path in sorted((root / "data/main").glob("*.iwd")):
        original = originals[path] = zipfile.ZipFile(path)
        for info in original.infolist():
            if not info.is_dir():
                asset_index[info.filename.lower()] = (path, info.filename)

    materials_source = (root / "src/PC/gfx_d3d/r_material.c").read_text()
    material_tables = materials_source.split("static BuiltInMaterialTable", 1)[1]
    material_tables = material_tables.split("extern int R_HashAssetName", 1)[0]
    pending = ["materials/" + name for name in re.findall(
        r'\{ "([^\"]+)",', material_tables
    )]
    ui_source = (root / "src/PC/ui_mp/ui_main_mp.c").read_text()
    pending.extend("materials/" + name for name in re.findall(
        r'CL_RegisterMaterialNoMip\("([^\"]+)"', ui_source
    ))
    # Cgame registers these HUD materials directly, outside menu/BSP assets.
    cgame_source = "\n".join(path.read_text() for path in
                             (root / "src/PC/cgame_mp").glob("*.c"))
    pending.extend("materials/" + name for name in re.findall(
        r'CL_RegisterMaterial(?:NoMip)?\(\s*(?:\(const char \*\))?\s*"([^\"]+)"', cgame_source
    ))
    # Mantling has its own engine-created animation tree and therefore cannot
    # be discovered through multiplayer.atr. Preserve every original clip.
    mantle_source = (root / "src/PC/bgame/bg_mantle.c").read_text()
    mantle_names = mantle_source.split("s_mantleAnimNames[] = {", 1)[1].split("};", 1)[0]
    pending.extend("xanim/" + name for name in re.findall(r'"([^\"]+)"', mantle_names)[1:])
    pending.extend(("xanim/void", "accuracy/aivsai/noweapon.accu"))
    pending.append("fx/misc/missing_fx.efx")
    graphics_source = (root / "src/PC/cgame_mp/cg_registergraphics_new.c").read_text()
    pending.extend(re.findall(r'"(fx/[^\"]+\.efx)"', graphics_source))
    pending.extend(("shock/default.shock", "shock/hold_breath.shock"))
    pending.append("info/mp_lochit_dmgtable")
    # _menus.gsc builds these names from the fixed factions and gametype.
    pending.extend("ui_mp/scriptmenus/" + menu + ".menu" for menu in (
        "team_britishgerman", "weapon_british", "weapon_german", "serverinfo_tdm"
    ))
    pending.extend(name for name in asset_index if name.endswith(".csv")
                   and (name.count("/") == 1 and name.startswith("fx/")
                        or name.startswith("fx/maps/mp/mp_toujane/")))
    pending.extend((
        "materials/$raw", "fonts/consolefont", "fonts/smalldevfont",
        "fonts/bigdevfont", "fonts/smalldevfont", "fonts/bigfont",
        "fonts/smallfont", "fonts/normalfont", "fonts/boldfont",
        "fonts/extrabigfont", "lights/default", "lights/light_dynamic",
        "materials/console", "maps/mp/gametypes/tdm.gsc",
        # Loaded directly by GScr_LoadScripts, outside the GSC import graph.
        "codescripts/delete.gsc", "codescripts/struct.gsc",
        # Entity keys drive gametype filtering, including overlapping MG42s.
        "radiant/keys.txt",
        "maps/mp/gametypes/tdm.txt", "maps/mp/mp_toujane.csv",
        "ui_mp/menus.txt", "ui_mp/hud.txt", "ui_mp/ingame.txt"
    ))
    # Add the selected BSP and follow its real material/entity dependencies.
    # Other map names occurring in menus must never pull in another BSP.
    pending.extend(("maps/mp/mp_toujane.d3dbsp", "maps/mp/mp_toujane.gsc",
                    "maps/mp/mp_toujane_fx.gsc", "mp/playeranim.script",
                    "mp/playeranimtypes.txt", "animtrees/multiplayer.atr"))
    # British/Afrika Korps loadouts used by the original Toujane TDM scripts.
    weapons = (
        "frag_grenade_british_mp", "smoke_grenade_british_mp", "webley_mp",
        "enfield_mp", "sten_mp", "bren_mp", "enfield_scope_mp", "m1garand_mp",
        "thompson_mp", "shotgun_mp", "frag_grenade_german_mp",
        "smoke_grenade_german_mp", "luger_mp", "kar98k_mp", "g43_mp",
        "mp40_mp", "mp44_mp", "kar98k_sniper_mp", "binoculars_mp",
        "defaultweapon_mp", "mg42_bipod_stand_mp",
    )
    pending.extend("weapons/mp/" + weapon for weapon in weapons)
    renderer = {}
    while pending:
        name = pending.pop().lower()
        if (name not in asset_index and name.startswith("materials/")
                and name.endswith((".tga", ".jpg", ".iwi"))):
            name = name[:-4]
        if name in renderer or name not in asset_index:
            continue
        path, original_name = asset_index[name]
        data = originals[path].read(original_name)
        data = apply_weapon_balance(name, data, balance_profile)
        if name == "maps/mp/gametypes/tdm.gsc":
            # The browser TDM death view uses the server's exact deadline.
            # Leave the owner's retail archive intact and patch the private
            # derived script shared by every room/client.
            old = b"delay = 2;"
            if data.count(old) != 1:
                raise ValueError("Unexpected TDM death delay")
            data = data.replace(old, b'delay = 3;\n\tself setClientCvar("cg_respawnDeadline", getTime() + 3000);')
            old = b'self.sessionstate = "playing";'
            if data.count(old) != 1:
                raise ValueError("Unexpected TDM spawn state")
            data = data.replace(old, old + b'\n\tself setClientCvar("cg_respawnDeadline", 0);')
        if name == "ui_mp/menus.txt":
            # Preserve the menu files; restrict their manifest to multiplayer
            # settings and TDM. The owner archives remain untouched.
            excluded = {"single_player", "settings_dm", "settings_ctf",
                        "settings_hq", "settings_sd", "settings_sw",
                        "options_voice", "auto_update", "pb_popmenus",
                        "mods"}
            data = b"\n".join(line for line in data.splitlines()
                              if not any((stem + ".menu").encode() in line
                                         for stem in excluded)) + b"\n"
        renderer[name] = data
        if name.startswith(("images/", "xmodelsurfs/", "xmodelparts/", "xanim/")):
            continue
        dependency_data = data
        if name.endswith(".gsc"):
            # Ignore comments while retaining quoted asset strings. Some
            # retail scripts contain commented-out SP/prototype references.
            dependency_data = re.sub(
                rb'"(?:\\.|[^"\\])*"|//[^\r\n]*|/\*.*?\*/',
                lambda match: match[0] if match[0].startswith(b'"') else b" ",
                data, flags=re.S)
        if name.endswith(".d3dbsp"):
            if name != "maps/mp/mp_toujane.d3dbsp":
                raise ValueError("Unexpected map dependency: " + name)
            if data[:8] != b"IBSP\x04\x00\x00\x00":
                raise ValueError("Expected the original CoD2 BSP version 4")
            material_length, material_offset = struct.unpack_from("<ii", data, 8)
            entity_length, entity_offset = struct.unpack_from("<ii", data, 8 + 37 * 8)
            if (material_length % 72 or material_offset < 320
                    or material_offset + material_length > len(data)
                    or entity_offset < 320 or entity_offset + entity_length > len(data)):
                raise ValueError("Invalid Toujane BSP material/entity lump")
            tokens = [data[offset:offset + 64].split(b"\0", 1)[0]
                      for offset in range(material_offset, material_offset + material_length, 72)]
            tokens += re.findall(rb'"([^"\r\n]+)"',
                                 data[entity_offset:entity_offset + entity_length])
        else:
            # '~' and '&' occur in original packed normal/specular image names.
            tokens = re.findall(rb'[A-Za-z0-9_/$#.@~&-]{2,}', dependency_data)
        if name.endswith(".gsc"):
            # Script dependencies are required even when a function is not
            # called: the original compiler resolves them while linking.
            scripts = re.findall(rb'([A-Za-z0-9_]+(?:\\[A-Za-z0-9_]+)+)\s*::', dependency_data)
            scripts += re.findall(rb'#include\s+([A-Za-z0-9_\\]+)\s*;', dependency_data)
            for script in scripts:
                dependency = script.decode("ascii").replace("\\", "/").lower() + ".gsc"
                if dependency not in asset_index:
                    raise ValueError(f"Missing script {dependency} required by {name}")
                pending.append(dependency)
            # Keep all compile-time script dependencies but do not load models
            # for teams/uniforms which the fixed Toujane match never selects.
            if name.startswith(("mptype/", "character/", "xmodelalias/")):
                if "british_africa" not in name and "german_africa" not in name:
                    tokens = []
        if name.startswith("xmodel/"):
            # LOD names are length-delimited by NUL following a float distance.
            # Parse the header instead of relying on printable binary runs.
            if data[:2] != b"\x14\0":
                raise ValueError("Unexpected xmodel version: " + name)
            position = 27
            for lod in range(4):
                position += 4
                end = data.index(b"\0", position)
                lod_name = data[position:end].decode("ascii").lower()
                if lod_name:
                    required = ["xmodelsurfs/" + lod_name]
                    if lod == 0:
                        required.append("xmodelparts/" + lod_name)
                    for dependency in required:
                        if dependency not in asset_index:
                            raise ValueError(f"Missing model data {dependency} for {name}")
                        pending.append(dependency)
                position = end + 1
        if name.startswith("lights/"):
            # Light files use {type, sampler, cookie\0, sampler, attenuation\0}.
            # The sampler byte can itself be ASCII and must not join the name.
            cookie, attenuation = data[2:].split(b"\0", 1)
            tokens.extend((cookie, attenuation[1:].split(b"\0", 1)[0]))
        for raw_token in tokens:
            token = raw_token.decode("ascii").replace("\\", "/").lower().lstrip("/")
            candidates = (
                token, "materials/" + token, "images/" + token + ".iwi",
                "materials_dx7/techniquesets/" + token + ".techset",
                "materials_dx7/techniques/" + token + ".tech",
                "materials/statemaps/" + token + ".sm",
            )
            if name.startswith(("weapons/", "animtrees/", "mp/")):
                candidates += ("xmodel/" + token, "xanim/" + token,
                               "fx/" + token + ".efx", "accuracy/aivsai/" + token,
                               "accuracy/aivsplayer/" + token)
            if name.endswith(".d3dbsp") or name.startswith("weapons/"):
                candidates += ("weapons/mp/" + token,)
            if name.endswith(".gsc") or name.startswith("fx/"):
                # Nested original effects use /fx/name without an extension.
                if token.startswith("fx/") and not token.endswith(".efx"):
                    candidates += (token + ".efx",)
                candidates += ("fx/" + token + ".efx", "xmodel/" + token,
                               "shock/" + token + ".shock")
                if token.endswith(".efx"):
                    candidates += ("fx/" + token,)
            if name.endswith(".gsc"):
                candidates += ("ui_mp/scriptmenus/" + token + ".menu",)
            # The renderer accepts legacy .tga/.jpg/.iwi material aliases.
            if token.endswith((".tga", ".jpg", ".iwi")):
                candidates += ("materials/" + token[:-4],)
            pending.extend(candidate for candidate in candidates
                           if candidate in asset_index and candidate not in renderer)
    archives["cod2_browser_renderer.iwd"] = renderer
    archives["cod2_browser_audio.iwd"] = collect_sound_assets(root, asset_index, originals, renderer)
    archives["cod2_browser_bootstrap.iwd"]["mp/toujane.arena"] = (
        b'{ map "mp_toujane" longname "Toujane, Tunisia" gametype "tdm" }\n'
    )
finally:
    for original in originals.values():
        original.close()

files = []
for archive_name, entries in archives.items():
    target = main / archive_name
    with zipfile.ZipFile(target, "w") as output:
        for name, data in sorted(entries.items()):
            info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            output.writestr(info, data)
    files.append({
        "key": Path(archive_name).stem.replace("_", "-"),
        "name": archive_name, "path": "main/" + archive_name,
        "size": target.stat().st_size, "magic": [80, 75, 3, 4],
        "sha256": hashlib.sha256(target.read_bytes()).hexdigest()
    })
version = "bootstrap-" + hashlib.sha256(
    "".join(item["sha256"] for item in files).encode()
).hexdigest()[:16]
policy = {
    "namespace": "cod2-browser", "version": version,
    "variants": {"cod2-mp": {
        "namespace": "cod2-toujane-tdm", "version": version,
        "phase": "toujane-dependencies-unverified", "files": files
    }}
}
(root / "site/wasm-game-data.json").write_text(json.dumps(policy, indent=2) + "\n")
print(f"Private Toujane archives: {sum(item['size'] for item in files)} bytes; "
      f"asset closure: {len(renderer)} files; gameplay still unverified")
print(f"Weapon balance: {balance_profile['id']} ({len(balance_profile['weapons'])} automatic weapons)")
