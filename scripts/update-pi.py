#!/usr/bin/env python3
"""Validate a main release in isolation, apply it when idle, and roll back failures."""
import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import time
import urllib.error
import urllib.request

REPOSITORY = 'https://github.com/tonga54/cod2-wasm.git'
SERVICES = ('cod2-web', 'cod2-server', 'cod2-gateway')
ALIASES = dict(zip(SERVICES, ('local/cod2-wasm:pi', 'local/cod2-pi-server:dev', 'local/cod2-gateway:pi')))
RELEASE_REPOS = dict(zip(SERVICES, ('local/cod2-wasm', 'local/cod2-pi-server', 'local/cod2-gateway')))
SHA = re.compile(r'^[a-f0-9]{40}$')
HASH = re.compile(r'^[a-f0-9]{64}$')


class Deferred(RuntimeError):
    pass


def atomic_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.tmp')
    with temporary.open('w') as stream:
        json.dump(value, stream, indent=2)
        stream.write('\n')
        stream.flush()
        os.fsync(stream.fileno())
    temporary.chmod(0o600)
    temporary.replace(path)


def validate_manifest(value, revision):
    if not SHA.fullmatch(revision) or value.get('schemaVersion') != 2 or value.get('revision') != revision:
        raise ValueError('Release revision/schema mismatch')
    if value.get('platform') != 'linux/arm64' or value.get('build', {}).get('revision') != revision:
        raise ValueError('Release architecture/build mismatch')
    if value['build'].get('dirty') is not False or not HASH.fullmatch(value['build'].get('buildId', '')):
        raise ValueError('Release must contain a clean, identified browser build')
    if not HASH.fullmatch(value.get('assetManifestSha256', '')) or set(value.get('images', {})) != set(SERVICES):
        raise ValueError('Release asset/image metadata is invalid')
    for service in SERVICES:
        image = value['images'][service]
        if image.get('tag') != f'{RELEASE_REPOS[service]}:release-{revision}' or not re.fullmatch(r'sha256:[a-f0-9]{64}', image.get('id', '')):
            raise ValueError('Release image identity is invalid')
        if not HASH.fullmatch(image.get('fingerprint', '')):
            raise ValueError('Release image content fingerprint is invalid')
    archive = value.get('archive', {})
    if archive.get('name') != 'pi-images.tar.gz' or not HASH.fullmatch(archive.get('sha256', '')):
        raise ValueError('Release archive metadata is invalid')
    if type(archive.get('size')) is not int or not 0 < archive['size'] <= 1536 * 1024**2:
        raise ValueError('Release archive size is invalid')
    return value


def image_fingerprint(image):
    # Docker's classic store reports a config digest as Id; containerd reports
    # a manifest digest. Ordered filesystem layers and runtime configuration
    # identify the same content on both stores without relying on that Id.
    config = image.get('Config') or {}
    fields = {}
    for name in ('Env', 'Entrypoint', 'Cmd', 'User', 'WorkingDir', 'Labels',
                 'ExposedPorts', 'Volumes', 'Healthcheck', 'StopSignal', 'OnBuild'):
        value = config.get(name)
        if value in ({}, []):
            value = None
        fields[name] = value
    value = {'architecture': image['Architecture'], 'os': image['Os'],
             'layers': image['RootFS']['Layers'], 'config': fields}
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def is_idle(health):
    pending = health.get('pendingClients', 0)
    return health.get('gateway') == 'ready' and type(health.get('clients')) is int and health['clients'] == 0 and type(pending) is int and pending == 0


class Updater:
    def __init__(self, root, state, originals):
        self.root, self.state, self.originals = root.resolve(), state.resolve(), originals.resolve()
        self.assets = self.root / 'data/browser'
        self.overlay = Path.home() / 'cod2-edge/compose.security.yaml'
        self.state.mkdir(parents=True, exist_ok=True)
        self.state.chmod(0o700)
        self.record_path = self.state / 'status.json'
        self.transaction_path = self.state / 'transaction.json'
        self.record = json.loads(self.record_path.read_text()) if self.record_path.exists() else {}

    def run(self, *command, input=None, timeout=1200, env=None):
        result = subprocess.run(command, input=input, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=timeout, env=env)
        if result.returncode:
            raise RuntimeError(f'{command[0]} failed ({result.returncode}): {result.stdout[-3500:]}')
        return result.stdout.strip()

    def git(self, *args):
        return self.run('git', '-C', str(self.root), *args, timeout=60)

    def status(self, status, **fields):
        self.record.update(status=status, checkedAt=int(time.time()), **fields)
        atomic_json(self.record_path, self.record)
        print(json.dumps({'status': status, **fields}), flush=True)

    def guard_source(self):
        if self.git('remote', 'get-url', 'origin') != REPOSITORY:
            raise RuntimeError('Unexpected origin; updater only follows the owner repository')
        if self.git('branch', '--show-current') != 'main' or self.git('status', '--porcelain'):
            raise Deferred('The Pi checkout has local changes or is not on main')
        return self.git('rev-parse', 'HEAD')

    def compose(self, source, project, assets, images, *arguments, qa=False):
        override = self.state / f'{project}-images.yaml'
        lines = ['services:']
        for service in SERVICES:
            lines.extend((f'  {service}:', f'    image: {images[service]}'))
            if qa and service == 'cod2-server':
                lines.append('    cpus: 1.0')
            if qa and service == 'cod2-gateway':
                lines.extend(('    ports: !override', '      - "127.0.0.1:0:8088"'))
        override.write_text('\n'.join(lines) + '\n')
        command = ['docker', 'compose', '--project-directory', str(source), '-p', project,
                   '-f', str(source / 'compose.yaml'), '-f', str(source / 'compose.pi.yaml')]
        if not qa and self.overlay.is_file():
            command.extend(('-f', str(self.overlay)))
        command.extend(('-f', str(override), *arguments))
        env = dict(os.environ, COD2_WEB_DATA_DIR=str(assets))
        return self.run(*command, env=env)

    def health(self):
        code = "fetch('http://127.0.0.1:8088/gateway/health').then(async r=>{if(!r.ok)process.exit(1);console.log(await r.text());}).catch(()=>process.exit(1))"
        return json.loads(self.run('docker', 'exec', 'cod2-wasm-pi-cod2-gateway-1', 'node', '-e', code, timeout=10))

    def ensure_idle(self):
        if not is_idle(self.health()):
            raise Deferred('Players are connected; waiting for the server to be idle')

    def installed_revision(self):
        code = "fetch('http://127.0.0.1:8088/build-info.json').then(async r=>{if(!r.ok)process.exit(1);console.log((await r.json()).revision);}).catch(()=>process.exit(1))"
        revision = self.run('docker','exec','cod2-wasm-pi-cod2-gateway-1','node','-e',code,timeout=10)
        if not SHA.fullmatch(revision):
            raise ValueError('Installed build metadata is invalid')
        return revision

    def fence(self, assets, enabled):
        path = assets / 'web/host-maintenance.json'
        if enabled:
            atomic_json(path, {'until': int(time.time() * 1000) + 600000})
        else:
            path.unlink(missing_ok=True)

    def download(self, url, path, size=None, digest=None):
        limit = size if size is not None else 65536
        temporary = path.with_suffix(path.suffix + '.part')
        try:
            with urllib.request.urlopen(urllib.request.Request(url, headers={'User-Agent':'cod2-pi-updater'}), timeout=45) as response, temporary.open('wb') as output:
                total, actual = 0, hashlib.sha256()
                while chunk := response.read(1024 * 1024):
                    total += len(chunk)
                    if total > limit:
                        raise ValueError('Release download exceeded its size limit')
                    output.write(chunk)
                    actual.update(chunk)
            if size is not None and (total != size or actual.hexdigest() != digest):
                raise ValueError('Release archive failed its size/SHA-256 check')
            temporary.replace(path)
        finally:
            temporary.unlink(missing_ok=True)

    def prepare(self, revision):
        release = self.state / 'releases' / revision
        release.mkdir(parents=True, exist_ok=True)
        base = f'https://github.com/tonga54/cod2-wasm/releases/download/pi-{revision}/'
        try:
            self.download(base + 'manifest.json', release / 'manifest.json')
        except urllib.error.HTTPError as error:
            if error.code == 404:
                raise Deferred('The main build is still pending CI validation') from error
            if error.code in (403, 429, 500, 502, 503, 504):
                raise Deferred('Release delivery is temporarily unavailable; retrying later') from error
            raise
        except (urllib.error.URLError, TimeoutError) as error:
            raise Deferred('Release delivery is temporarily unavailable; retrying later') from error
        manifest = validate_manifest(json.loads((release / 'manifest.json').read_text()), revision)
        if shutil.disk_usage(self.state).free < 1536 * 1024**2:
            raise Deferred('At least 1.5 GiB free space is needed for a staged update')
        if not (self.originals / 'iw_00.iwd').is_file():
            raise Deferred('Private original IWDs are missing from the configured owner directory')
        archive = release / 'pi-images.tar.gz'
        valid = archive.exists() and archive.stat().st_size == manifest['archive']['size'] and hashlib.sha256(archive.read_bytes()).hexdigest() == manifest['archive']['sha256']
        if not valid:
            try:
                self.download(base + 'pi-images.tar.gz', archive, manifest['archive']['size'], manifest['archive']['sha256'])
            except (urllib.error.URLError, TimeoutError) as error:
                raise Deferred('Release download interrupted; retrying later') from error
        self.run('docker', 'load', '-i', str(archive))
        for service in SERVICES:
            image = json.loads(self.run('docker', 'image', 'inspect', manifest['images'][service]['tag']))[0]
            if image_fingerprint(image) != manifest['images'][service]['fingerprint'] or image['Architecture'] != 'arm64' or image['Config']['Labels'].get('org.opencontainers.image.revision') != revision:
                raise ValueError('Loaded image does not match its validated release')
        source = release / 'source'
        if not source.exists():
            self.git('worktree', 'add', '--detach', str(source), revision)
        if self.run('git', '-C', str(source), 'rev-parse', 'HEAD') != revision or self.run('git', '-C', str(source), 'status', '--porcelain'):
            raise ValueError('Staged source is not the clean release commit')
        if hashlib.sha256((source / 'site/wasm-game-data.json').read_bytes()).hexdigest() != manifest['assetManifestSha256']:
            raise ValueError('Source and release asset manifests differ')
        (source / 'data').mkdir(exist_ok=True)
        original_link = source / 'data/main'
        if not original_link.exists():
            original_link.symlink_to(self.originals, target_is_directory=True)
        if original_link.resolve() != self.originals:
            raise ValueError('Staged originals must point to the configured owner archive directory')
        assets = release / 'assets'
        assets.mkdir(exist_ok=True)
        shutil.copytree(self.assets / 'web', assets / 'web', dirs_exist_ok=True,
                        ignore=shutil.ignore_patterns('host-maintenance.json'))
        generated = release / 'generated-assets.json'
        self.run(sys.executable, str(source / 'scripts/prepare-browser-bootstrap.py'),
                 '--main-dir', str(assets / 'main'), '--manifest', str(generated))
        if json.loads(generated.read_text()) != json.loads((source / 'site/wasm-game-data.json').read_text()):
            raise ValueError('Prepared owner assets differ from the published build; refusing to mix versions')
        self.run(sys.executable, str(source / 'scripts/check-private-assets.py'),
                 '--main-dir', str(assets / 'main'), '--manifest', str(generated))
        return release, manifest

    def verify(self, source, project, assets, images, build, qa=False):
        container = self.compose(source, project, assets, images, 'ps', '-q', 'cod2-gateway', qa=qa)
        if not container:
            raise RuntimeError('Gateway did not start')
        code = (source / 'scripts/verify-pi-stack.mjs').read_text()
        return self.run('docker', 'exec', '-i', container, 'node', '--input-type=module', '-',
                        build['revision'], build['buildId'], 'candidate' if qa else 'production',
                        input=code, timeout=240)

    def validate_candidate(self, release, manifest):
        project = 'cod2-update-qa-' + manifest['revision'][:12]
        source, assets = release / 'source', release / 'assets'
        images = {s: manifest['images'][s]['tag'] for s in SERVICES}
        try:
            self.compose(source, project, assets, images, 'config', '--quiet', qa=True)
            self.compose(source, project, assets, images, 'up', '-d', '--no-build', '--wait', '--wait-timeout', '90', *SERVICES, qa=True)
            print(self.verify(source, project, assets, images, manifest['build'], qa=True), flush=True)
        finally:
            self.compose(source, project, assets, images, 'down', '--remove-orphans', qa=True)

    def current_images(self):
        names = ['cod2-wasm-pi-' + s + '-1' for s in SERVICES]
        values = json.loads(self.run('docker', 'inspect', *names))
        if not all(v['State']['Running'] for v in values):
            raise Deferred('The running stack must be healthy before automatic updates')
        return {s: v['Image'] for s, v in zip(SERVICES, values)}

    def rollback(self, transaction):
        release = Path(transaction['release'])
        backup = release / 'previous-assets'
        if backup.exists():
            if self.assets.exists():
                failed = release / ('failed-assets-' + str(time.time_ns()))
                self.assets.replace(failed)
            backup.replace(self.assets)
        for service, image in transaction['previousImages'].items():
            self.run('docker', 'tag', image, ALIASES[service])
        # Snapshotted Compose files restore the old stack even if someone edited
        # the checkout while validation was running. Human source changes survive.
        snapshot = release / 'previous-source'
        self.compose(snapshot, 'cod2-wasm-pi', self.assets, ALIASES, 'up', '-d', '--no-build',
                     '--force-recreate', '--wait', '--wait-timeout', '90', *SERVICES)
        try:
            current_source = self.guard_source()
        except (Deferred, RuntimeError):
            current_source = None
        if current_source == transaction['revision'] and current_source != transaction['previousRevision']:
            self.git('reset', '--hard', transaction['previousRevision'])
        self.fence(self.assets, False)
        self.transaction_path.unlink(missing_ok=True)

    def activate(self, release, manifest):
        previous = self.guard_source()
        self.fence(self.assets, True)
        self.fence(release / 'assets', True)
        try:
            self.ensure_idle()
            # Reject an obsolete build if another main push arrived during QA.
            try:
                self.git('fetch', '--no-tags', 'origin', 'refs/heads/main:refs/remotes/origin/main')
            except RuntimeError as error:
                raise Deferred('Unable to recheck main before activation; retrying later') from error
            if self.git('rev-parse', 'origin/main') != manifest['revision']:
                raise Deferred('A newer main commit arrived; waiting for its validated package')
            transaction = {'revision': manifest['revision'], 'previousRevision': previous,
                           'previousImages': self.current_images(), 'release': str(release)}
            for service, image in transaction['previousImages'].items():
                self.run('docker', 'tag', image, f'{RELEASE_REPOS[service]}:rollback-{previous}')
            snapshot = release / 'previous-source'
            snapshot.mkdir(exist_ok=True)
            for name in ('compose.yaml', 'compose.pi.yaml'):
                shutil.copyfile(self.root / name, snapshot / name)
            atomic_json(self.transaction_path, transaction)
            try:
                if self.health().get('updating') is not True:
                    # One-time bootstrap from hosts predating the admission gate.
                    self.ensure_idle()
                    self.compose(snapshot, 'cod2-wasm-pi', self.assets, ALIASES, 'stop', 'cod2-gateway')
                self.assets.replace(release / 'previous-assets')
                (release / 'assets').replace(self.assets)
                for service in SERVICES:
                    self.run('docker', 'tag', manifest['images'][service]['tag'], ALIASES[service])
                self.compose(release / 'source', 'cod2-wasm-pi', self.assets, ALIASES,
                             'up', '-d', '--no-build', '--force-recreate', '--wait', '--wait-timeout', '90', *SERVICES)
                print(self.verify(release / 'source', 'cod2-wasm-pi', self.assets, ALIASES, manifest['build']), flush=True)
                if self.guard_source() != previous:
                    raise RuntimeError('Pi source changed during the update; keeping human changes')
                self.git('merge', '--ff-only', manifest['revision'])
                self.status('updated', revision=manifest['revision'], previousRevision=previous, failedRevision=None, error=None)
                self.transaction_path.unlink()
            except BaseException:
                self.rollback(transaction)
                raise
        finally:
            self.fence(self.assets, False)
            self.fence(release / 'assets', False)

    def cleanup(self, current):
        releases = self.state / 'releases'
        previous = self.record.get('previousRevision')
        for path in releases.iterdir():
            if not SHA.fullmatch(path.name) or path.name in (current, previous):
                continue
            if (path / 'source').exists():
                self.git('worktree', 'remove', '--force', str(path / 'source'))
            for service in SERVICES:
                subprocess.run(['docker', 'image', 'rm', f'{RELEASE_REPOS[service]}:release-{path.name}', f'{RELEASE_REPOS[service]}:rollback-{path.name}'],
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            shutil.rmtree(path)

    def check(self, retry=False):
        revision = None
        try:
            if self.transaction_path.exists():
                transaction = json.loads(self.transaction_path.read_text())
                self.rollback(transaction)
                self.status('recovered', failedRevision=transaction['revision'])
            current = self.guard_source()
            self.git('fetch', '--no-tags', 'origin', 'refs/heads/main:refs/remotes/origin/main')
            revision = self.git('rev-parse', 'origin/main')
            if not SHA.fullmatch(revision):
                raise ValueError('Invalid main revision')
            if current == revision and self.installed_revision() == revision:
                self.status('current', revision=revision)
                return
            self.git('merge-base', '--is-ancestor', current, revision)
            installed = self.installed_revision()
            changed = self.git('diff', '--name-only', installed + '..' + revision).splitlines()
            if changed and all(p.endswith('.md') or p.startswith('docs/') for p in changed):
                self.status('code-current', revision=installed, targetRevision=revision)
                return
            if self.record.get('failedRevision') == revision and not retry:
                self.status('rejected', targetRevision=revision, reason='This commit failed validation; awaiting a new main commit')
                return
            self.ensure_idle()
            release, manifest = self.prepare(revision)
            self.status('validating', targetRevision=revision)
            self.validate_candidate(release, manifest)
            self.activate(release, manifest)
            self.cleanup(revision)
            # The running Python process retains its code. The next timer run
            # can use an updated, successfully deployed updater implementation.
            installed = Path.home() / '.local/lib/cod2/update-pi.py'
            if installed.exists():
                shutil.copyfile(self.root / 'scripts/update-pi.py', installed.with_suffix('.next'))
                installed.with_suffix('.next').replace(installed)
        except Deferred as error:
            self.status('waiting', targetRevision=revision, reason=str(error))
        except BaseException as error:
            self.status('failed', failedRevision=revision, error=str(error))
            raise


def install(root, state, originals):
    if subprocess.check_output(['loginctl','show-user',str(os.getuid()),'-p','Linger','--value'],text=True).strip() != 'yes':
        raise RuntimeError('Enable lingering for this user before installing the timer')
    library = Path.home() / '.local/lib/cod2'
    library.mkdir(parents=True, exist_ok=True)
    library.chmod(0o700)
    shutil.copyfile(Path(__file__), library / 'update-pi.py')
    units = Path.home() / '.config/systemd/user'
    units.mkdir(parents=True, exist_ok=True)
    arguments = ' '.join(json.dumps(str(p)).replace('%','%%') for p in (library/'update-pi.py',))
    arguments += ' --root ' + json.dumps(str(root)) + ' --state ' + json.dumps(str(state)) + ' --originals ' + json.dumps(str(originals))
    (units / 'cod2-auto-update.service').write_text('[Unit]\nDescription=Validate and apply idle CoD2 main releases\nAfter=network-online.target\n\n[Service]\nType=oneshot\nExecStart=/usr/bin/python3 '+arguments+'\nTimeoutStartSec=25min\nTimeoutStopSec=120\nNice=10\n')
    (units / 'cod2-auto-update.timer').write_text('[Unit]\nDescription=Check validated CoD2 main releases\n\n[Timer]\nOnBootSec=2min\nOnUnitInactiveSec=5min\nRandomizedDelaySec=30s\n\n[Install]\nWantedBy=timers.target\n')
    subprocess.run(['systemctl','--user','daemon-reload'],check=True)
    subprocess.run(['systemctl','--user','enable','--now','cod2-auto-update.timer'],check=True)
    print('Installed: validated main updates are checked every five minutes, including after boot')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path.home()/'cod2-wasm')
    parser.add_argument('--state', type=Path, default=Path.home()/'.local/state/cod2-updater')
    parser.add_argument('--originals', type=Path, default=Path.home()/'cod2-original/main')
    parser.add_argument('--install', action='store_true')
    parser.add_argument('--retry', action='store_true')
    args = parser.parse_args()
    if args.install:
        install(args.root.resolve(), args.state.resolve(), args.originals.resolve())
        return
    def stop(_signal, _frame):
        raise InterruptedError('Updater was stopped; restoring an unfinished transaction')
    signal.signal(signal.SIGTERM, stop)
    updater = Updater(args.root, args.state, args.originals)
    with (updater.state/'update.lock').open('a') as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            print('Another update is already running')
            return
        updater.check(args.retry)


if __name__ == '__main__':
    try:
        main()
    except Exception as error:
        print('Pi update failed: '+str(error), file=sys.stderr)
        sys.exit(1)
