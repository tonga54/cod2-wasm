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
parser.add_argument('--main-dir', type=Path)
args = parser.parse_args()
assert 1 <= args.rounds <= 100
root = Path(__file__).resolve().parent.parent
main_dir = (args.main_dir or root / 'data/browser/main').resolve()
name = f'cod2-rotation-check-{os.getpid()}'
master, slave = pty.openpty()
command = [
    'docker', 'run', '--rm', '-it', '--name', name,
    '--platform', 'linux/amd64', '--cpus', '1', '--memory', '256m',
    '--ulimit', 'core=0', '--network', 'none', '--read-only',
    '--tmpfs', '/profile:uid=1000,gid=1000,size=32m',
    '-v', f'{main_dir}:/game/main:ro',
    '-v', f'{root}/server.cfg:/profile/raw/server.cfg:ro',
    '-v', f'{root}/out/cod2-native/cod2_lnxded:/usr/local/bin/cod2_lnxded:ro',
    '--entrypoint', '/usr/local/bin/cod2_lnxded', 'local/cod2-native-server:dev',
    '+set', 'dedicated', '1', '+set', 'fs_basepath', '/game',
    '+set', 'fs_homepath', '/profile', '+set', 'sv_maxclients', '2',
    '+exec', 'server.cfg', '+set', 'sv_mapRotationCurrent',
    'gametype tdm map mp_carentan', '+map', 'mp_toujane',
]
child = subprocess.Popen(command, stdin=slave, stdout=slave, stderr=slave)
os.close(slave)
log = (root / 'out/map-rotation-check.log').open('wb')


def wait_for_output(predicate):
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
        if predicate(pending):
            return
    raise TimeoutError('Server did not produce the expected output within 20 seconds')


def wait_for_baseline():
    wait_for_output(lambda output: b'[ckpt] baseline done' in output)


def check_map(expected):
    query = '''import socket, json
with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
    udp.settimeout(2)
    udp.sendto(b'\\xff\\xff\\xff\\xffgetstatus cod2-rotation', ('127.0.0.1', 28960))
    data = udp.recv(8192).decode('latin1')
    fields = data.split('\\n')[1].split('\\\\')
    info = dict(zip(fields[1::2], fields[2::2]))
print(json.dumps(info))
'''
    result = subprocess.run(['docker', 'exec', name, '/usr/bin/python3', '-c', query],
                            check=True, capture_output=True, text=True)
    import json
    info = json.loads(result.stdout)
    assert info['mapname'] == expected, info
    assert info['g_gametype'] == 'tdm', info
    os.write(master, b'scr_tdm_scorelimit\nscr_tdm_timelimit\n')
    wait_for_output(lambda output: b'"scr_tdm_scorelimit" is: "100^7"' in output and
                    b'"scr_tdm_timelimit" is: "15^7"' in output)


try:
    wait_for_baseline()
    check_map('mp_toujane')
    for round_number in range(1, args.rounds + 1):
        os.write(master, b'map_rotate\n')
        wait_for_baseline()
        expected = 'mp_carentan' if round_number % 2 else 'mp_toujane'
        check_map(expected)
        print(f'PASS: map rotation {round_number}/{args.rounds}: {expected}, TDM, 100 points / 15 minutes', flush=True)
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
