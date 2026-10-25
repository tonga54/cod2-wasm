"""The players' full health on the server, in place of retail's 100, with a small increase over retail."""

GAMETYPES = ('ctf', 'dm', 'hq', 'sd', 'tdm')
SCRIPTS = {f'maps/mp/gametypes/{gametype}.gsc' for gametype in GAMETYPES}
RETAIL_HEALTH = b'maxhealth = 100;'
# Every mode sets it as a player spawns; HQ also as it heals the players when a radio is taken or lost.
RETAIL_COUNTS = {'maps/mp/gametypes/hq.gsc': 3}
# At most 135, the 135 a bolt-action rifle's shot to the chest and a semi-automatic's to the head deal, so they
# still kill alone. bg_player_health.h's BG_PLAYER_HEALTH.
PLAYER_HEALTH = 105


def apply_player_health(name: str, original: bytes) -> bytes:
    """A retail mode's players at PLAYER_HEALTH instead of 100."""
    if name not in SCRIPTS:
        return original
    if original.count(RETAIL_HEALTH) != RETAIL_COUNTS.get(name, 1):
        raise ValueError('Unexpected retail health in ' + name)
    return original.replace(RETAIL_HEALTH, b'maxhealth = %d;' % PLAYER_HEALTH)
