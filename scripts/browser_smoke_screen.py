"""Dedicated neutral smoke-screen materials; preserve the original coverage."""
import re
import struct

EFFECT = 'fx/explosions/smoke_grenade.efx'
PREFIX = 'browser_smoke_screen_'


def smoke_screen_assets(read):
    original = read(EFFECT).decode('latin-1')
    assets = {}

    def shaders(match):
        body = match[2]
        for name in body.split():
            if not re.fullmatch(r'[\w/.-]+', name):
                raise ValueError('Unexpected smoke-screen material: ' + name)
            if 'smoke' not in name:
                continue
            replacement = PREFIX + name
            material = bytearray(read('materials/' + name))
            struct.pack_into('<I', material, 0, len(material))
            material.extend(replacement.encode('ascii') + b'\0')
            assets['materials/' + replacement] = bytes(material)
            body = re.sub(r'(?<!\S)' + re.escape(name) + r'(?!\S)', replacement, body)
        return match[1] + body + match[3]

    changed, count = re.subn(r'(\bshaders\s*\[)([^\]]*)(\])', shaders, original)
    if not count:
        raise ValueError('Smoke grenade has no material bindings')
    assets[EFFECT] = changed.encode('latin-1')
    return assets
