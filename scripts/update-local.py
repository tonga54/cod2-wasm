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
        raise RuntimeError('No se pudo comprobar si hay jugadores. Revisá el servidor antes de actualizar.') from error
    if health['clients']:
        raise RuntimeError('Hay personas conectadas. Volvé a actualizar cuando la partida esté vacía.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true', help='Actualizar, compilar y reiniciar un host vacío')
    args = parser.parse_args()
    branch, dirty = source_status()
    if args.apply:
        if branch != 'master':
            raise RuntimeError('La actualización automática requiere la rama master.')
        if dirty:
            raise RuntimeError('Hay cambios locales. Guardalos o confirmalos antes de actualizar; no se sobrescribió nada.')
        framework = Path(os.environ.get('COD2_WASM_FRAMEWORK_DIR', ROOT.parent / 'wasm-game-framework'))
        if not (framework / '.git').exists():
            raise RuntimeError('Falta wasm-game-framework. Consultá docs/UPDATES.md para preparar el checkout.')
        if not (ROOT / 'data/main/iw_00.iwd').is_file():
            raise RuntimeError('Faltan los archivos originales en data/main/. Consultá docs/UPDATES.md.')
        ensure_idle()
    # A failed/forced/non-fast-forward fetch never changes the working tree.
    run('git', 'fetch', '--no-tags', 'origin', f'refs/heads/master:{REMOTE}')
    status, count = comparison()
    if status in ('local-ahead', 'diverged'):
        raise RuntimeError('Tu historial tiene commits propios. Revisá la actualización manualmente; no se hizo merge ni reset.')
    if status == 'available':
        print(f'Hay una versión nueva: {count} commit(s) en origin/master.', flush=True)
        if dirty:
            print('Tu copia tiene cambios locales; guardalos antes de actualizar.', flush=True)
        if not args.apply:
            print('Para instalarla: python3 scripts/update-local.py --apply', flush=True)
            return 1
    else:
        print('El código local está actualizado.', flush=True)
    if args.apply:
        # Check again after network access and before changing source/assets.
        if source_status() != ('master', False):
            raise RuntimeError('La copia cambió durante la comprobación. No se actualizó.')
        ensure_idle()
        run('git', 'merge', '--ff-only', REMOTE)
        run(sys.executable, 'scripts/prepare-browser-bootstrap.py')
        run('bash', 'scripts/build-docker.sh')
        ensure_idle()
        run('docker', 'compose', 'up', '-d')
        print('Servidor actualizado. Los navegadores avisarán cuando puedan recargar.', flush=True)
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (RuntimeError, subprocess.CalledProcessError, OSError, ValueError) as error:
        print(f'No se pudo actualizar: {error}', file=sys.stderr)
        sys.exit(2)
