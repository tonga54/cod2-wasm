#!/usr/bin/env python3
"""Measure an isolated native/Pi bot match (requires Docker and private map assets)."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import platform
import re
import subprocess
import time
import uuid

from bot_flow_metrics import summarize_flow

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map', choices=['mp_carentan', 'mp_toujane', 'mp_dawnville', 'mp_burgundy'], default='mp_carentan')
    parser.add_argument('--mode', choices=['dm', 'tdm', 'ctf', 'hq', 'sd'], default='tdm')
    parser.add_argument('--bots', type=int, choices=range(1, 33), default=16)
    parser.add_argument('--seconds', type=int, default=90)
    parser.add_argument('--fps', type=int, choices=[20, 30, 40, 60], default=30,
                        help='Server simulation rate; exercise script scheduling beyond the original 20 Hz')
    parser.add_argument('--expect-passes', type=int, default=0, help='Minimum completed controlled scenarios')
    parser.add_argument('--stop-after-passes', action='store_true', help='End a controlled scenario once all expected passes arrive')
    parser.add_argument('--image', default='local/cod2-native-server:dev')
    parser.add_argument('--runtime', choices=['native', 'pi'], default='native')
    parser.add_argument('--difficulty', type=int, choices=range(3), default=1)
    parser.add_argument('--assets', type=Path, default=ROOT / 'data/browser/main')
    parser.add_argument('--config', type=Path, default=ROOT / 'server.cfg')
    parser.add_argument('--binary', type=Path, default=ROOT / 'out/cod2-native/cod2_lnxded')
    parser.add_argument('--policy', type=Path, default=ROOT / 'downstream/server/browser_bots.gsc')
    parser.add_argument('--output', type=Path, default=ROOT / 'out/bot-runtime')
    args = parser.parse_args()
    if args.seconds < 5:
        parser.error('--seconds must be at least 5')
    if args.stop_after_passes and args.expect_passes < 1:
        parser.error('--stop-after-passes requires a positive --expect-passes')
    for item in (args.binary, args.policy, args.assets, args.config):
        if not item.exists():
            parser.error(f'missing {item}')
    args.output.mkdir(parents=True, exist_ok=True)
    model_path = Path('/proc/device-tree/model')
    host = {'system': platform.system(), 'machine': platform.machine(),
            'model': model_path.read_bytes().rstrip(b'\0').decode() if model_path.exists() else None}
    name = 'cod2-bot-qa-' + uuid.uuid4().hex[:10]

    def docker(*command):
        return subprocess.check_output(['docker', *command], text=True, stderr=subprocess.STDOUT)

    def cpu():
        fields = docker('exec', name, 'cat', '/sys/fs/cgroup/cpu.stat').split()
        return int(dict(zip(fields[::2], fields[1::2]))['usage_usec'])

    engine = '/usr/local/bin/qemu-i386 -L /opt/cod2-i386 /qa-engine' if args.runtime == 'pi' else '/qa-engine'
    docker_platform = 'linux/arm64' if args.runtime == 'pi' else 'linux/amd64'
    log = ''
    try:
        docker('run', '-d', '--name', name, '--platform', docker_platform, '--network', 'none', '--ulimit', 'core=0',
               '--read-only', '--tmpfs', '/profile:uid=1000,gid=1000,size=32m', '--tmpfs', '/tmp:size=16m',
               '-v', f'{args.assets.resolve()}:/game/main:ro',
               '-v', f'{args.binary.resolve()}:/qa-engine:ro',
               '-v', f'{args.policy.resolve()}:/qa-bots.gsc:ro',
               '-v', f'{args.config.resolve()}:/qa-server.cfg:ro',
               '--entrypoint', '/bin/sh', args.image, '-c',
               'mkdir -p /profile/raw/maps/mp; cp /qa-bots.gsc /profile/raw/maps/mp/browser_bots.gsc; '
               f'cp /qa-server.cfg /profile/raw/server.cfg; exec {engine} '
               '+set dedicated 1 +set fs_basepath /game +set fs_homepath /profile '
               '+set net_ip 127.0.0.1 +set sv_maxclients 64 +exec server.cfg '
               f'+set sv_fps {args.fps} '
               f'+set scr_bot_count {args.bots} +set scr_bot_difficulty {args.difficulty} +set scr_bot_debug 1 +set scr_bot_profile 1 '
               '+set scr_tdm_scorelimit 0 +set scr_tdm_timelimit 0 '
               f'+set g_gametype {args.mode} +map {args.map}')
        deadline = time.monotonic() + 60
        while time.monotonic() < deadline:
            log = docker('logs', name)
            if len(re.findall(r'Going from CS_FREE to CS_CONNECTED', log)) >= args.bots:
                break
            if re.search(r'(?:script compile error|script runtime error|ERROR:|Error during initialization)', log, re.I):
                raise RuntimeError('Engine/script error; see saved server log')
            time.sleep(1)
        else:
            raise RuntimeError('Bots did not finish joining within 60 seconds')
        started = time.monotonic()
        before = cpu()
        print(f'{args.map}/{args.mode}: {args.bots} bots joined; measuring {args.seconds}s', flush=True)
        if args.stop_after_passes:
            deadline = started + args.seconds
            while time.monotonic() < deadline:
                time.sleep(min(5, max(0, deadline - time.monotonic())))
                log = docker('logs', name)
                if re.search(r'\[bot(?:-nav)?-qa\] FAIL |script compile error|script runtime error|ERROR:', log, re.I) or len(re.findall(
                        r'\[bot(?:-nav)?-qa\] PASS ', log)) >= args.expect_passes:
                    break
        else:
            time.sleep(args.seconds)
        after = cpu()
        elapsed = time.monotonic() - started
        log = docker('logs', name)
        if re.search(r'(?:script compile error|script runtime error|ERROR:|Error during initialization)', log, re.I):
            raise RuntimeError('Engine/script error; see saved server log')
        if docker('inspect', '-f', '{{.State.Running}}', name).strip() != 'true':
            raise RuntimeError('Engine exited during the match')
        positions = {}
        for match in re.finditer(r'\[bots\] state (\d+) team=\d+ state=(\d+) health=(\d+) pos=(-?\d+),(-?\d+),(-?\d+)', log):
            bot, state, health, x, y, z = map(int, match.groups())
            if state == 0 and health > 0:
                positions.setdefault(bot, []).append((x, y, z))
        cells = {bot: len({(math.floor(x / 256), math.floor(y / 256), math.floor(z / 128))
                          for x, y, z in samples}) for bot, samples in positions.items()}
        profiles = [dict((key, int(value)) for key, value in re.findall(r'(\w+)=(\d+)', line))
                    for line in re.findall(r'\[bots\] profile (.+)', log)]
        windows = [p for p in profiles[1:] if p.get('wall', 0) >= 4500]
        sniper_samples = [tuple(map(int, m)) for m in re.findall(
            r'\[bots\] sniper (\d+) catalogue=(\d+) perched=(\d+) guarding=(\d+) relativeHeight=(-?\d+)', log)]
        held_heights = [height for _, _, perched, guarding, height in sniper_samples if perched or guarding]
        result = {
            'map': args.map, 'mode': args.mode, 'bots': args.bots, 'seconds': round(elapsed, 2),
            'requestedServerFps': args.fps,
            'runtime': args.runtime, 'difficulty': args.difficulty,
            'host': host,
            'engineSha256': hashlib.sha256(args.binary.read_bytes()).hexdigest(),
            'policySha256': hashlib.sha256(args.policy.read_bytes()).hexdigest(),
            'cpuPercentOfOneCore': round((after - before) / (elapsed * 10000), 2),
            'sampled256UnitCellsPerBot': cells,
            'lastMatchState': re.findall(r'\[bots\] match (.+)', log)[-1:],
            'matchStates': list(dict.fromkeys(re.findall(r'\[bots\] match (.+)', log))),
            'respawnResets': len(re.findall(r'\[bots\] respawn client=', log)),
            'lastWorkCounters': re.findall(r'\[bots\] work (.+)', log)[-1:],
            'simulationFps': round(sum(p['frames'] for p in windows) * 1000 / sum(p['wall'] for p in windows), 2) if windows else None,
            'maxGameFrameMs': max((p['maxGame'] for p in windows), default=None),
            'maxBotAiMs': max((p['maxAI'] for p in windows), default=None),
            'profile': re.findall(r'\[bots\] profile (.+)', log),
            'sniperOverwatch': {
                'catalogueSlotsSeen': max((sample[1] for sample in sniper_samples), default=0),
                'holdingSamples': len(held_heights),
                'maxHoldingHeightAboveAnchor': max(held_heights, default=None),
                'scope': '5s samples; height is relative to the guarded objective or lowest spawn for patrols'
            },
            'flow': summarize_flow(log),
            'intents': sorted(set(re.findall(r'intent=(\w+)', log))),
            'controlledPasses': re.findall(r'\[bot(?:-nav)?-qa\] PASS (.+)', log),
            'controlledFailures': re.findall(r'\[bot(?:-nav)?-qa\] FAIL (.+)', log),
            'scope': f'{args.runtime} Docker match, including navigation warm-up; no browser/network/rendering load. '
                     'Position samples every 5s are diagnostic coverage, not proof of all routes.'
        }
        (args.output / f'{args.map}-{args.mode}.json').write_text(json.dumps(result, indent=2) + '\n')
        print(json.dumps(result, indent=2))
        if result['controlledFailures']:
            raise RuntimeError('Controlled scenario failed; see saved server log')
        if len(result['controlledPasses']) < args.expect_passes:
            raise RuntimeError('Controlled scenarios did not finish before the measurement ended')
    finally:
        try:
            log = docker('logs', name)
        except subprocess.CalledProcessError:
            pass
        (args.output / f'{args.map}-{args.mode}.log').write_text(log)
        subprocess.run(['docker', 'rm', '-f', name], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


if __name__ == '__main__':
    main()
