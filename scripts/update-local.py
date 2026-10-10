#!/usr/bin/env python3
"""Check for upstream updates; --apply explicitly rebuilds an idle local host."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import urllib.request

ROOT = Path(__file__).resolve().parent.parent
REMOTE = 'refs/remotes/origin/master'


def git(*args, root=ROOT, check=True):
    return subprocess.run(['git', *args], cwd=root, check=check,
                          text=True, capture_output=True)


def source_status(root=ROOT):
    origin = git('remote', 'get-url', 'origin', root=root).stdout.strip().rstrip('/')
    if origin.removesuffix('.git').lower() not in (
            'https://github.com/tonga54/cod2-wasm', 'git@github.com:tonga54/cod2-wasm',
            'ssh://git@github.com/tonga54/cod2-wasm'):
        raise RuntimeError('origin debe apuntar a https://github.com/tonga54/cod2-wasm.git')
    return git('branch', '--show-current', root=root).stdout.strip(), bool(
        git('status', '--porcelain', root=root).stdout.strip())


def comparison(root=ROOT):
    head = git('rev-parse', 'HEAD', root=root).stdout.strip()
    latest = git('rev-parse', REMOTE, root=root).stdout.strip()
    if head == latest:
        return 'current', 0
    if git('merge-base', '--is-ancestor', 'HEAD', REMOTE, root=root, check=False).returncode == 0:
        count = int(git('rev-list', '--count', f'HEAD..{REMOTE}', root=root).stdout)
        return 'available', count
    if git('merge-base', '--is-ancestor', REMOTE, 'HEAD', root=root, check=False).returncode == 0:
        return 'local-ahead', 0
    return 'diverged', 0


def run(*args):
    subprocess.run(args, cwd=ROOT, check=True)


def ensure_idle():
    running = subprocess.check_output(
        ['docker', 'compose', 'ps', '--status', 'running', '-q'], cwd=ROOT, text=True).strip()
    if not running:
        return
    port = int(os.environ.get('COD2_WEB_PORT', '8088'))
    try:
        with urllib.request.urlopen(f'http://127.0.0.1:{port}/gateway/health', timeout=3) as response:
            health = json.load(response)
        if health.get('gateway') != 'ready' or not isinstance(health.get('clients'), int):
            raise ValueError('invalid health response')
    except Exception as error:
        raise RuntimeError('Unable to check for active players. Check the server before updating.') from error
    if health['clients']:
        raise RuntimeError('Players are connected. Try updating again when the server is empty.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true', help='Update, build and restart an empty host')
    args = parser.parse_args()
    branch, dirty = source_status()
    if args.apply:
        if branch != 'master':
            raise RuntimeError('Automatic updates require the master branch.')
        if dirty:
            raise RuntimeError('Local changes found. Save or commit them before updating; nothing was overwritten.')
        framework = Path(os.environ.get('COD2_WASM_FRAMEWORK_DIR', ROOT.parent / 'wasm-game-framework'))
        if not (framework / '.git').exists():
            raise RuntimeError('wasm-game-framework is missing. See docs/UPDATES.md to prepare the checkout.')
        if not (ROOT / 'data/main/iw_00.iwd').is_file():
            raise RuntimeError('The original files are missing from data/main/. See docs/UPDATES.md.')
        ensure_idle()
    # A failed/forced/non-fast-forward fetch never changes the working tree.
    run('git', 'fetch', '--no-tags', 'origin', f'refs/heads/master:{REMOTE}')
    status, count = comparison()
    if status in ('local-ahead', 'diverged'):
        raise RuntimeError('Your history contains local commits. Review the update manually; no merge or reset was performed.')
    if status == 'available':
        print(f'A new version is available: {count} commit(s) in origin/master.', flush=True)
        if dirty:
            print('Your checkout has local changes; save them before updating.', flush=True)
        if not args.apply:
            print('To install it: python3 scripts/update-local.py --apply', flush=True)
            return 1
    else:
        print('The local code is up to date.', flush=True)
    if args.apply:
        # Check again after network access and before changing source/assets.
        if source_status() != ('master', False):
            raise RuntimeError('The checkout changed during the check. No update was applied.')
        ensure_idle()
        run('git', 'merge', '--ff-only', REMOTE)
        run(sys.executable, 'scripts/prepare-browser-bootstrap.py')
        run('bash', 'scripts/build-docker.sh')
        ensure_idle()
        run('docker', 'compose', 'up', '-d')
        print('Server updated. Browsers will notify players when they can reload.', flush=True)
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (RuntimeError, subprocess.CalledProcessError, OSError, ValueError) as error:
        print(f'Unable to update: {error}', file=sys.stderr)
        sys.exit(2)
