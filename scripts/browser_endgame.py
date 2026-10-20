"""The end of a match's length on the server, for the announcement and the scoreboard cg_endgame_mp.c shows."""

GAMETYPES = ('ctf', 'dm', 'hq', 'sd', 'tdm')
SCRIPTS = {f'maps/mp/gametypes/{gametype}.gsc' for gametype in GAMETYPES}
RETAIL_WAIT = b'\twait 10;\r\n\texitLevel(false);'
# The announcement's 5 s, its half-second handover and 10.5 s of scoreboard
# (ENDGAME_INTERMISSION_MSEC).
INTERMISSION_SECONDS = 16


def apply_endgame_timing(name: str, original: bytes) -> bytes:
    """Hold a retail mode's intermission INTERMISSION_SECONDS before the next map, in place of its 10."""
    if name not in SCRIPTS:
        return original
    if original.count(RETAIL_WAIT) != 1:
        raise ValueError('Unexpected retail end of a match in ' + name)
    return original.replace(RETAIL_WAIT, b'\twait %d;\r\n\texitLevel(false);' % INTERMISSION_SECONDS)
