#!/usr/bin/env python3
"""Publish the Pi's prepared game archives to the Cloudflare Worker that serves their downloads."""
import argparse
import contextlib
import fcntl
import hashlib
import hmac
import json
import os
from pathlib import Path, PurePosixPath
import re
import secrets
import shutil
import subprocess
import sys
import time
import urllib.error
import urllib.request

# downstream/cdn/handler.mjs joins parts of this size.
PART_BYTES = 24 * 1024 * 1024
WORKER_SOURCES = ('worker.mjs', 'handler.mjs')
WORKER = 'cod2-data'
WRANGLER = '4.151.2'
WRANGLER_IMAGE = 'local/cod2-wrangler:' + WRANGLER
GATEWAY = 'cod2-wasm-pi-cod2-gateway-1'
SHA = re.compile(r'^[a-f0-9]{64}$')
WORKER_URL = re.compile(r'https://' + WORKER + r'\.[a-z0-9-]+\.workers\.dev')
WRANGLER_CONFIG = {
    'name': WORKER, 'main': 'worker.mjs', 'compatibility_date': '2025-09-01',
    'workers_dev': True, 'preview_urls': False,
    'assets': {'directory': 'assets', 'binding': 'ASSETS', 'run_worker_first': True,
               'html_handling': 'none', 'not_found_handling': 'none'},
}


class Deferred(RuntimeError):
    pass


def atomic_write(path, text, mode):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + '.tmp')
    with temporary.open('w') as stream:
        stream.write(text)
        stream.flush()
        os.fsync(stream.fileno())
    temporary.chmod(mode)
    temporary.replace(path)


def archives(root):
    """The archives the checkout's asset manifest lists: SHA-256 -> (size, file)."""
    manifest = json.loads((root / 'site/wasm-game-data.json').read_text())
    found = {}
    for variant in manifest['variants'].values():
        for file in variant['files']:
            relative = PurePosixPath(str(file.get('path')))
            if not SHA.fullmatch(str(file.get('sha256'))) or type(file.get('size')) is not int or file['size'] <= 0 \
                    or relative.is_absolute() or '..' in relative.parts:
                raise ValueError(f'Invalid asset manifest entry: {file.get("key")}')
            found[file['sha256']] = (file['size'], root / 'data/browser' / relative)
    return found


def split(source, sha256, size, assets, part_bytes=PART_BYTES):
    """Copies an archive into the Worker's numbered parts, checking it is the manifest's."""
    target = assets / 'p' / sha256
    target.mkdir(parents=True)
    digest, total, index = hashlib.sha256(), 0, 0
    with source.open('rb') as stream:
        while chunk := stream.read(part_bytes):
            digest.update(chunk)
            total += len(chunk)
            (target / f'{index}.part').write_bytes(chunk)
            index += 1
    if total != size or digest.hexdigest() != sha256:
        raise ValueError(f'{source.name} is not the archive the asset manifest lists')
    return index


def worker_id(root):
    """What a deployment runs besides the archives: the Worker's code, its configuration and Wrangler."""
    digest = hashlib.sha256(json.dumps([WRANGLER, WRANGLER_CONFIG], sort_keys=True).encode())
    for name in WORKER_SOURCES:
        digest.update(b'\0' + name.encode() + b'\0' + (root / 'downstream/cdn' / name).read_bytes())
    return digest.hexdigest()[:16]


def sign(secret, sha256, size, expires):
    # downstream/gateway/cdn.mjs signs, and the Worker checks, the same message.
    return hmac.new(secret.encode(), f'{sha256}:{size}:{expires}'.encode(), hashlib.sha256).hexdigest()


def download(url, timeout=60):
    """A URL's status, body length and SHA-256, and its CORS origin header."""
    request = urllib.request.Request(url, headers={'User-Agent': 'cod2-cdn-publisher'})
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            digest, total = hashlib.sha256(), 0
            while chunk := response.read(1 << 20):
                digest.update(chunk)
                total += len(chunk)
            return response.status, total, digest.hexdigest(), response.headers.get('Access-Control-Allow-Origin')
    except urllib.error.HTTPError as error:
        return error.code, 0, None, error.headers.get('Access-Control-Allow-Origin')


class Publisher:
    def __init__(self, root, config, state, run=None, fetch=download, wait=time.sleep, now=time.time,
                 part_bytes=PART_BYTES):
        self.root, self.config, self.state = root.resolve(), config.resolve(), state.resolve()
        # The gateway mounts only this directory: never the Cloudflare token.
        self.shared = self.config / 'gateway'
        self.published_path = self.shared / 'published.json'
        self.key_path = self.shared / 'signing-key'
        self.credentials = self.config / 'cloudflare.env'
        self.stage = self.state / 'cdn-stage'
        self.run = run or self.command
        self.fetch, self.wait, self.now, self.part_bytes = fetch, wait, now, part_bytes

    @staticmethod
    def command(*arguments, input=None, timeout=3600):
        result = subprocess.run(arguments, input=input, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=timeout)
        if result.returncode:
            raise RuntimeError(f'{arguments[0]} failed ({result.returncode}): {result.stdout[-3000:]}')
        return result.stdout

    @contextlib.contextmanager
    def exclusive(self):
        # update-pi.py's lock: it replaces data/browser while it holds it.
        self.state.mkdir(parents=True, exist_ok=True)
        with (self.state / 'update.lock').open('a') as lock:
            try:
                fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except BlockingIOError:
                raise Deferred('An update is running; publishing after it') from None
            yield

    def key(self):
        if not self.key_path.exists():
            atomic_write(self.key_path, secrets.token_hex(32) + '\n', 0o600)
        value = self.key_path.read_text().strip()
        if not SHA.fullmatch(value):
            raise ValueError('The signing key must be 64 lowercase hex digits')
        return value

    def idle(self):
        code = ("fetch('http://127.0.0.1:8088/gateway/health').then(async r=>{if(!r.ok)process.exit(1);"
                "console.log(await r.text());}).catch(()=>process.exit(1))")
        try:
            health = json.loads(self.run('docker', 'exec', GATEWAY, 'node', '-e', code, timeout=10))
        except (RuntimeError, ValueError) as error:
            raise Deferred('The gateway is not answering') from error
        return health.get('clients') == 0 and health.get('pendingClients', 0) == 0

    def wrangler(self, *arguments, input=None):
        try:
            self.run('docker', 'image', 'inspect', WRANGLER_IMAGE, timeout=30)
        except RuntimeError:
            self.run('docker', 'build', '--build-arg', 'WRANGLER=' + WRANGLER, '-t', WRANGLER_IMAGE,
                     str(self.root / 'downstream/cdn'))
        return self.run('docker', 'run', '--rm', '-i', '--env-file', str(self.credentials),
                        '-e', 'WRANGLER_SEND_METRICS=false', '-v', f'{self.stage}:/work', '-w', '/work',
                        WRANGLER_IMAGE, *arguments, input=input)

    def verify(self, url, secret, files, attempts=4):
        """Downloads each archive through a signed link and checks unsigned requests are refused."""
        sample = next(iter(files))
        for attempt in range(attempts):
            failures = []
            expires = int(self.now()) + 900
            for sha256, size in files.items():
                status, length, digest, cors = self.fetch(
                    f'{url}/a/{sha256}/{size}?exp={expires}&sig={sign(secret, sha256, size, expires)}')
                if (status, length, digest, cors) != (200, size, sha256, '*'):
                    failures.append(f'{sha256[:12]} answered HTTP {status} with {length} bytes')
            for probe in (f'{url}/p/{sample}/0.part', f'{url}/a/{sample}/{files[sample]}?exp={expires}&sig={"0" * 64}'):
                status = self.fetch(probe)[0]
                if status != 403:
                    failures.append(f'{probe.split("?")[0]} answered HTTP {status} without a valid signature')
            if not failures:
                return
            if attempt + 1 < attempts:
                # A new deployment takes seconds to reach every Cloudflare location.
                self.wait(15)
        raise RuntimeError('The Worker does not serve the archives correctly: ' + '; '.join(failures))

    def publish(self, force=False):
        wanted = archives(self.root)
        files = {sha256: size for sha256, (size, _) in wanted.items()}
        published = json.loads(self.published_path.read_text()) if self.published_path.exists() else {}
        secret = self.key()
        key_id = hashlib.sha256(secret.encode()).hexdigest()[:16]
        worker = worker_id(self.root)
        if not force and published.get('files') == files and published.get('keyId') == key_id \
                and published.get('worker') == worker and WORKER_URL.fullmatch(str(published.get('url'))):
            return {'status': 'current', 'url': published['url'], 'files': len(files)}
        if not self.credentials.is_file():
            raise Deferred(f'Cloudflare credentials are missing: {self.credentials}')
        # The upload shares the home connection with the game.
        if not self.idle():
            raise Deferred('Players are connected; waiting for the server to be idle')
        shutil.rmtree(self.stage, ignore_errors=True)
        try:
            with self.exclusive():
                (self.stage / 'assets').mkdir(parents=True)
                parts = sum(split(source, sha256, size, self.stage / 'assets', self.part_bytes)
                            for sha256, (size, source) in wanted.items())
            for name in WORKER_SOURCES:
                shutil.copyfile(self.root / 'downstream/cdn' / name, self.stage / name)
            (self.stage / 'wrangler.json').write_text(json.dumps(WRANGLER_CONFIG, indent=2) + '\n')
            match = WORKER_URL.search(self.wrangler('deploy'))
            if not match:
                raise RuntimeError('Wrangler did not report the Worker address on workers.dev')
            url = match.group(0)
            if published.get('keyId') != key_id or published.get('url') != url:
                self.wrangler('secret', 'put', 'SIGNING_KEY', input=secret)
            self.verify(url, secret, files)
            atomic_write(self.published_path, json.dumps({'url': url, 'keyId': key_id, 'worker': worker, 'files': files,
                                                          'publishedAt': int(self.now())}, indent=2) + '\n', 0o644)
            return {'status': 'published', 'url': url, 'files': len(files), 'parts': parts}
        finally:
            shutil.rmtree(self.stage, ignore_errors=True)


def install(root, config, state):
    units = Path.home() / '.config/systemd/user'
    units.mkdir(parents=True, exist_ok=True)
    arguments = ' '.join(json.dumps(str(value)).replace('%', '%%') for value in (
        root / 'scripts/publish-cdn.py', '--root', root, '--config', config, '--state', state))
    (units / 'cod2-cdn-publish.service').write_text(
        '[Unit]\nDescription=Publish the CoD2 game archives to Cloudflare\nAfter=network-online.target\n\n'
        '[Service]\nType=oneshot\nExecStart=/usr/bin/python3 ' + arguments + '\nTimeoutStartSec=90min\nNice=10\n')
    (units / 'cod2-cdn-publish.timer').write_text(
        '[Unit]\nDescription=Check the CoD2 game archives on Cloudflare\n\n'
        '[Timer]\nOnBootSec=4min\nOnUnitInactiveSec=5min\nRandomizedDelaySec=30s\n\n[Install]\nWantedBy=timers.target\n')
    subprocess.run(['systemctl', '--user', 'daemon-reload'], check=True)
    subprocess.run(['systemctl', '--user', 'enable', '--now', 'cod2-cdn-publish.timer'], check=True)
    print('Installed: the archives are checked every five minutes and published when they change')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path.home() / 'cod2-wasm')
    parser.add_argument('--config', type=Path, default=Path.home() / 'cod2-edge/cdn')
    parser.add_argument('--state', type=Path, default=Path.home() / '.local/state/cod2-updater')
    parser.add_argument('--force', action='store_true', help='deploy and verify even when nothing changed')
    parser.add_argument('--install', action='store_true')
    args = parser.parse_args()
    if args.install:
        install(args.root.resolve(), args.config.resolve(), args.state.resolve())
        return
    try:
        print(json.dumps(Publisher(args.root, args.config, args.state).publish(args.force)), flush=True)
    except Deferred as error:
        print(json.dumps({'status': 'waiting', 'reason': str(error)}), flush=True)


if __name__ == '__main__':
    try:
        main()
    except Exception as error:
        print(json.dumps({'status': 'failed', 'error': str(error)}), flush=True)
        sys.exit(1)
