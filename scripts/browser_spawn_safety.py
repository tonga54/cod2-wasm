"""Apply the first-party enemy-clearance filter to the private retail script."""
from pathlib import Path


def apply_spawn_safety(name: str, original: bytes) -> bytes:
    if name != 'maps/mp/gametypes/_spawnlogic.gsc':
        return original
    marker = b'\tif (isdefined(weights) && weights.size == spawnpoints.size)'
    if original.count(marker) != 1:
        raise ValueError('Unexpected retail final spawn selector')
    newline = b'\r\n' if b'\r\n' in original else b'\n'
    injection = '''\t// Clearance is a constraint; team/death/reuse weights only rank safe spots.
\tif (isdefined(weights) && weights.size == spawnpoints.size)
\t{
\t\tsafe = filterEnemySafeSpawns(spawnpoints, weights);
\t\tspawnpoints = safe.points;
\t\tweights = safe.weights;
\t}

'''.encode().replace(b'\n', newline)
    helper = (Path(__file__).resolve().parents[1] /
              'downstream/server/browser_spawn_safety.gsc').read_bytes()
    return original.replace(marker, injection + marker) + newline + helper.replace(b'\n', newline)
