"""Apply reviewed numeric LAN overrides to private, derived weapon assets."""
import json
from pathlib import Path
import re

PROFILE_PATH = Path(__file__).resolve().parent.parent / "downstream/weapon-balance.json"
ALLOWED_FIELDS = {
    "locHelmet", "locHead", "minDamage", "maxDamageRange", "adsSpread",
    "hipSpreadStandMin", "hipSpreadMax",
}


def load_profile():
    profile = json.loads(PROFILE_PATH.read_text())
    for name, fields in profile["weapons"].items():
        if not re.fullmatch(r"[a-z0-9_]+_mp", name) or not fields:
            raise ValueError("Invalid balance weapon: " + name)
        for key, values in fields.items():
            if key not in ALLOWED_FIELDS or not isinstance(values, list) or len(values) != 2:
                raise ValueError("Invalid balance field: " + name + "/" + key)
            if not all(isinstance(v, str) and re.fullmatch(r"\d+(?:\.\d+)?", v) for v in values):
                raise ValueError("Invalid balance value: " + name + "/" + key)
    return profile


def weapon_fields(data):
    parts = data.decode("latin1").split("\\")
    if parts[0] != "WEAPONFILE" or len(parts) % 2 != 1:
        raise ValueError("Malformed weapon definition")
    fields = dict(zip(parts[1::2], parts[2::2]))
    if len(fields) != (len(parts) - 1) // 2:
        raise ValueError("Duplicate weapon field")
    return fields


def apply_weapon_balance(name, data, profile):
    overrides = profile["weapons"].get(name.removeprefix("weapons/mp/"))
    if not name.startswith("weapons/mp/") or overrides is None:
        return data
    fields = weapon_fields(data)
    parts = data.decode("latin1").split("\\")
    for key, (expected, replacement) in overrides.items():
        if fields.get(key) != expected:
            raise ValueError(f"Unexpected retail value: {name}/{key}: {fields.get(key)!r}")
        # Replace the value by its field position; never replace other equal
        # numeric values, animations, sounds or models in the weapon file.
        field_index = next(i for i in range(1, len(parts), 2) if parts[i] == key)
        parts[field_index + 1] = replacement
    return "\\".join(parts).encode("latin1")
