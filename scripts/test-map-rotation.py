#!/usr/bin/env python3
"""Exercise real map rotation with private assets in an isolated disposable server."""
import argparse
import os
from pathlib import Path
import pty
import select
import subprocess
import time

parser = argparse.ArgumentParser()
parser.add_argument('--rounds', type=int, default=12)
args = parser.parse_args()
assert 1 <= args.rounds <= 100
root = Path(__file__).resolve().parent.parent
name = f'cod2-rotation-check-{os.getpid()}'
master, slave = pty.openpty()
command = [
    'docker', 'run', '--rm', '-it', '--name', name,
    '--platform', 'linux/amd64', '--cpus', '1', '--memory', '256m',
    '--ulimit', 'core=0', '--network', 'none', '--read-only',
    '--tmpfs', '/profile:uid=1000,gid=1000,size=32m',
    '-v', f'{root}/data/browser/main:/game/main:ro',
    '-v', f'{root}/out/cod2-native/cod2_lnxded:/usr/local/bin/cod2_lnxded:ro',
    '--entrypoint', '/usr/local/bin/cod2_lnxded', 'local/cod2-native-server:dev',
    '+set', 'dedicated', '1', '+set', 'fs_basepath', '/game',
    '+set', 'fs_homepath', '/profile', '+set', 'sv_maxclients', '2',
    '+set', 'g_gametype', 'tdm', '+set', 'sv_mapRotation',
    'gametype tdm map mp_toujane', '+map', 'mp_toujane',
]
child = subprocess.Popen(command, stdin=slave, stdout=slave, stderr=slave)
os.close(slave)
log = (root / 'out/map-rotation-check.log').open('wb')


def wait_for_baseline():
    pending = b''
    deadline = time.monotonic() + 20
    while time.monotonic() < deadline:
        if not select.select([master], [], [], 0.2)[0]:
            if child.poll() is not None:
                raise RuntimeError(f'Server exited: {child.returncode}')
            continue
        block = os.read(master, 65536)
        log.write(block)
        log.flush()
        pending += block
        if b'CRASH REPORT' in pending or b'ERROR:' in pending:
            raise RuntimeError(pending[-2500:].decode(errors='replace'))
        if b'[ckpt] baseline done' in pending:
            return
    raise TimeoutError('Server did not finish the map within 20 seconds')


try:
    wait_for_baseline()
    for round_number in range(1, args.rounds + 1):
        os.write(master, b'map_rotate\n')
        wait_for_baseline()
        print(f'PASS: map rotation {round_number}/{args.rounds}', flush=True)
    result = subprocess.run(['docker', 'stats', '--no-stream', '--format',
                             '{{.CPUPerc}} CPU / {{.MemUsage}}', name],
                            check=True, capture_output=True, text=True)
    print(result.stdout.strip())
    os.write(master, b'quit\n')
    child.wait(timeout=5)
    assert child.returncode == 0, child.returncode
finally:
    if child.poll() is None:
        subprocess.run(['docker', 'stop', '-t', '2', name],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        child.wait(timeout=5)
    os.close(master)
    log.close()
