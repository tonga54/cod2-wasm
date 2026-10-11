"""Summarize sampled bot crowding without treating coverage as route proof."""
import collections
import json
import math
import re
import statistics
import sys
from pathlib import Path


def summarize_flow(log):
    windows = []
    for match in re.finditer(r'\[bots\] work time=(\d+)(.*?)(?=\[bots\] work|\Z)', log, re.S):
        timestamp, block = int(match[1]), match[2]
        players = []
        for state in re.finditer(
                r'\[bots\] state (\d+) team=(\d+) state=0 health=[1-9]\d* '
                r'pos=(-?\d+),(-?\d+),(-?\d+) target=(-?\d+)', block):
            players.append(tuple(map(int, state.groups())))
        if len(players) < 8:
            continue
        neighbors = [sum(a[0] != b[0] and a[1] == b[1] and a[1] in (1, 2) and
                         (a[2]-b[2])**2 + (a[3]-b[3])**2 < 192**2 and abs(a[4]-b[4]) < 64
                         for b in players) for a in players]
        cells = collections.Counter((math.floor(a[2]/256), math.floor(a[3]/256), math.floor(a[4]/128))
                                    for a in players)
        windows.append({'time': timestamp, 'alive': len(players), 'neighborSum': sum(neighbors),
                        'crowded': sum(n >= 3 for n in neighbors), 'occupiedCells': len(cells),
                        'largestCell': max(cells.values())})
    result = {}
    for name, selected in [('firstMinute', [w for w in windows if 10000 <= w['time'] <= 60000]),
                           ('afterFirstMinute', [w for w in windows if w['time'] > 60000])]:
        samples = sum(w['alive'] for w in selected)
        result[name] = {
            'samples': samples,
            'meanNearbyTeammates': round(sum(w['neighborSum'] for w in selected)/samples, 2) if samples else None,
            'percentWithAtLeast3NearbyTeammates': round(100*sum(w['crowded'] for w in selected)/samples, 1) if samples else None,
            'meanOccupied256UnitCells': round(statistics.mean(w['occupiedCells'] for w in selected), 1) if selected else None,
            'maxBotsInOne256UnitCell': max((w['largestCell'] for w in selected), default=None),
        }
    departures = [int(ms) for ms in re.findall(r'\[bot-flow-qa\] departure client=\d+ ms=(\d+)', log)]
    result['firstSpawnDeparture'] = {'samples': len(departures),
        'medianMs': statistics.median(departures) if departures else None,
        'maxMs': max(departures, default=None),
        'timeouts': len(re.findall(r'\[bot-flow-qa\] departure_timeout', log))}
    result['scope'] = ('Crowding uses living teammates within 192 horizontal / 64 vertical units, sampled every 5 s. '
                       'DM has no teammates. Coverage and crowding alone do not prove distinct streets or combat quality. '
                       'Optional first-spawn departure is 256 units from the original spawn, sampled every 250 ms.')
    return result


if __name__ == '__main__':
    for filename in sys.argv[1:]:
        print(json.dumps({'file': filename, **summarize_flow(Path(filename).read_text())}, indent=2))
