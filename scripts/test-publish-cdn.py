#!/usr/bin/env python3
"""Exercise the Cloudflare publisher: parts, signatures, verification and when it must not publish."""
import contextlib
import fcntl
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import re
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('publish_cdn', Path(__file__).with_name('publish-cdn.py'))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
REPO = Path(__file__).resolve().parent.parent
URL = 'https://cod2-data.tonga.workers.dev'


class Cloudflare:
    """Docker, Wrangler and the deployed Worker, as the publisher reaches them."""

    def __init__(self, publisher):
        self.publisher = publisher
        self.clients, self.image, self.stale, self.public_parts = 0, True, False, False
        self.output = f'Uploaded cod2-data (4.20 sec)\nDeployed cod2-data triggers\n  {URL}\nCurrent Version ID: 1\n'
        self.commands, self.secrets, self.deployed, self.staged, self.corrupt = [], [], {}, {}, set()

    def run(self, *command, input=None, timeout=None):
        self.commands.append(command)
        if command[:2] == ('docker', 'exec'):
            return json.dumps({'gateway': 'ready', 'clients': self.clients, 'pendingClients': 0})
        if command[:3] == ('docker', 'image', 'inspect'):
            if not self.image:
                raise RuntimeError('docker failed (1): No such image')
            return '[]'
        if command[:2] == ('docker', 'build'):
            self.image = True
            return ''
        if command[:2] == ('docker', 'run'):
            arguments = command[command.index(module.WRANGLER_IMAGE) + 1:]
            stage = self.publisher.stage
            if arguments == ('deploy',):
                self.staged = {str(p.relative_to(stage)): p.read_bytes() for p in stage.rglob('*') if p.is_file()}
                self.deployed = {}
                for folder in (stage / 'assets/p').iterdir():
                    count = len(list(folder.iterdir()))
                    self.deployed[folder.name] = b''.join((folder / f'{i}.part').read_bytes() for i in range(count))
                return self.output
            if arguments == ('secret', 'put', 'SIGNING_KEY'):
                self.secrets.append(input)
                return ''
        raise AssertionError(f'Unexpected command: {command}')

    def fetch(self, url):
        match = re.fullmatch(re.escape(URL) + r'/a/([0-9a-f]{64})/([0-9]+)\?exp=([0-9]+)&sig=([0-9a-f]{64})', url)
        if match:
            sha256, size, expires, signature = match.groups()
            if not self.secrets or signature != module.sign(self.secrets[-1], sha256, int(size), int(expires)):
                return 403, 0, None, '*'
            data = None if self.stale else self.deployed.get(sha256)
            if data is None:
                return 404, 0, None, '*'
            if sha256 in self.corrupt:
                data = data[:-1] + b'?'
            return 200, len(data), hashlib.sha256(data).hexdigest(), '*'
        if url.startswith(URL + '/p/'):
            return (200 if self.public_parts else 403), 0, None, '*'
        return 404, 0, None, '*'


class PublisherTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        base = Path(self.directory.name).resolve()
        self.root, self.config, self.state = base / 'repo', base / 'edge/cdn', base / 'state'
        for folder in ('site', 'data/browser/main', 'downstream/cdn'):
            (self.root / folder).mkdir(parents=True)
        for name in module.WORKER_SOURCES:
            (self.root / 'downstream/cdn' / name).write_text(f'// {name}\n')
        self.config.mkdir(parents=True)
        (self.config / 'cloudflare.env').write_text('CLOUDFLARE_API_TOKEN=test\n')
        self.files = self.write({'a.iwd': b'abcdefghij', 'b.iwd': b'012345678'})
        self.waits = []
        self.publisher = module.Publisher(self.root, self.config, self.state, fetch=lambda url: self.cloud.fetch(url),
                                          wait=self.wait, now=lambda: 1700000000, part_bytes=4)
        self.cloud = Cloudflare(self.publisher)
        self.publisher.run = self.cloud.run

    def tearDown(self):
        self.directory.cleanup()

    def wait(self, seconds):
        self.waits.append(seconds)
        self.cloud.stale = False

    def write(self, archives):
        entries = []
        for name, data in archives.items():
            (self.root / 'data/browser/main' / name).write_bytes(data)
            entries.append({'key': name, 'name': name, 'path': f'main/{name}', 'size': len(data),
                            'sha256': hashlib.sha256(data).hexdigest()})
        (self.root / 'site/wasm-game-data.json').write_text(json.dumps({'variants': {'cod2-mp': {'files': entries}}}))
        return {entry['sha256']: entry['size'] for entry in entries}

    def published(self):
        return json.loads((self.config / 'gateway/published.json').read_text())

    def wrangler_calls(self):
        return [c[c.index(module.WRANGLER_IMAGE) + 1:] for c in self.cloud.commands if c[:2] == ('docker', 'run')]

    def test_the_worker_joins_parts_of_the_size_published(self):
        self.assertEqual(sorted(p.name for p in (REPO / 'downstream/cdn').glob('*.mjs') if '.test.' not in p.name),
                         sorted(module.WORKER_SOURCES), 'every Worker module is uploaded')
        source = (REPO / 'downstream/cdn/handler.mjs').read_text()
        factors = re.search(r'export const PART_BYTES = ([0-9 *]+);', source).group(1).split('*')
        size = 1
        for factor in factors:
            size *= int(factor)
        self.assertEqual(size, module.PART_BYTES)
        self.assertLess(module.PART_BYTES, 25 * 1024 * 1024)

    def test_signatures_match_the_vector_the_gateway_and_worker_check(self):
        self.assertEqual(module.sign('0123456789abcdef' * 4, 'a' * 64, 123, 1700000000),
                         '15e99cc456780c5fa327c94e4e7252b2bca0cad9079c3f8fac227ab5239838d8')

    def test_parts_are_numbered_slices_of_the_manifest_archive(self):
        source = self.root / 'data/browser/main/a.iwd'
        sha256 = hashlib.sha256(source.read_bytes()).hexdigest()
        assets = self.state / 'assets'
        self.assertEqual(module.split(source, sha256, 10, assets, 4), 3)
        self.assertEqual([(assets / f'p/{sha256}/{i}.part').read_bytes() for i in range(3)], [b'abcd', b'efgh', b'ij'])
        for size, digest in ((11, sha256), (10, 'f' * 64)):
            with self.assertRaisesRegex(ValueError, 'not the archive'):
                module.split(source, digest, size, self.state / f'other-{size}', 4)

    def test_manifest_entries_must_stay_inside_the_assets(self):
        for entry in ({'path': '../escape.iwd'}, {'path': '/etc/passwd'}, {'sha256': 'A' * 64}, {'size': 0},
                      {'size': '10'}):
            manifest = {'key': 'a', 'path': 'main/a.iwd', 'size': 10, 'sha256': 'a' * 64, **entry}
            (self.root / 'site/wasm-game-data.json').write_text(json.dumps({'variants': {'v': {'files': [manifest]}}}))
            with self.assertRaisesRegex(ValueError, 'Invalid asset manifest entry'):
                module.archives(self.root)

    def test_first_publish_deploys_sets_the_key_verifies_and_then_advertises(self):
        result = self.publisher.publish()
        self.assertEqual(result, {'status': 'published', 'url': URL, 'files': 2, 'parts': 6})
        self.assertEqual(self.wrangler_calls(), [('deploy',), ('secret', 'put', 'SIGNING_KEY')])
        key = (self.config / 'gateway/signing-key').read_text().strip()
        self.assertRegex(key, r'^[0-9a-f]{64}$')
        self.assertEqual(self.cloud.secrets, [key])
        self.assertEqual((self.config / 'gateway/signing-key').stat().st_mode & 0o777, 0o600)
        self.assertEqual((self.config / 'gateway/published.json').stat().st_mode & 0o777, 0o644)
        self.assertEqual(self.published(), {'url': URL, 'keyId': hashlib.sha256(key.encode()).hexdigest()[:16],
                                            'files': self.files, 'publishedAt': 1700000000})
        staged = self.cloud.staged
        self.assertEqual(json.loads(staged['wrangler.json']), module.WRANGLER_CONFIG)
        self.assertTrue(module.WRANGLER_CONFIG['assets']['run_worker_first'], 'the parts are never public')
        self.assertEqual(staged['worker.mjs'], b'// worker.mjs\n')
        self.assertEqual(staged['handler.mjs'], b'// handler.mjs\n')
        a, b = (hashlib.sha256(d).hexdigest() for d in (b'abcdefghij', b'012345678'))
        self.assertEqual(staged[f'assets/p/{b}/2.part'], b'8')
        self.assertEqual(sorted(staged), sorted(['wrangler.json', 'worker.mjs', 'handler.mjs'] +
                         [f'assets/p/{a}/{i}.part' for i in range(3)] + [f'assets/p/{b}/{i}.part' for i in range(3)]))
        self.assertFalse(self.publisher.stage.exists())
        self.assertEqual(self.waits, [])
        deploy = next(c for c in self.cloud.commands if c[-1] == 'deploy')
        self.assertIn(str(self.config / 'cloudflare.env'), deploy)
        self.assertNotIn(str(self.config / 'gateway'), ' '.join(deploy))

    def test_nothing_changed_means_no_docker_and_no_network(self):
        self.publisher.publish()
        self.cloud.commands.clear()
        self.cloud.fetch = lambda url: self.fail('A current publish must not download')
        self.assertEqual(self.publisher.publish(), {'status': 'current', 'url': URL, 'files': 2})
        self.assertEqual(self.cloud.commands, [])

    def test_a_changed_archive_redeploys_with_the_same_key(self):
        self.publisher.publish()
        self.files = self.write({'a.iwd': b'abcdefghij', 'b.iwd': b'0123456789xyz'})
        self.cloud.commands.clear()
        self.assertEqual(self.publisher.publish()['status'], 'published')
        self.assertEqual(self.wrangler_calls(), [('deploy',)])
        self.assertEqual(self.published()['files'], self.files)
        self.assertEqual(len(self.cloud.secrets), 1)

    def test_a_new_signing_key_is_sent_to_the_worker(self):
        self.publisher.publish()
        (self.config / 'gateway/signing-key').unlink()
        self.assertEqual(self.publisher.publish()['status'], 'published')
        self.assertEqual(len(self.cloud.secrets), 2)
        self.assertEqual(self.cloud.secrets[-1], (self.config / 'gateway/signing-key').read_text().strip())

    def test_a_new_deployment_is_retried_until_every_location_has_it(self):
        self.cloud.stale = True
        self.assertEqual(self.publisher.publish()['status'], 'published')
        self.assertEqual(self.waits, [15])

    def assert_not_advertised(self):
        with self.assertRaisesRegex(RuntimeError, 'does not serve the archives correctly'):
            self.publisher.publish()
        self.assertFalse((self.config / 'gateway/published.json').exists())
        self.assertFalse(self.publisher.stage.exists())
        self.assertEqual(self.waits, [15, 15, 15])

    def test_a_wrong_download_is_never_advertised(self):
        self.cloud.corrupt.add(next(iter(self.files)))
        self.assert_not_advertised()

    def test_parts_served_without_a_signature_are_never_advertised(self):
        self.cloud.public_parts = True
        self.assert_not_advertised()

    def test_without_the_worker_address_nothing_is_advertised(self):
        self.cloud.output = 'Uploaded cod2-data\nNo deploy targets\n'
        with self.assertRaisesRegex(RuntimeError, 'did not report the Worker address'):
            self.publisher.publish()
        self.assertFalse((self.config / 'gateway/published.json').exists())

    def test_an_archive_that_differs_from_the_manifest_is_not_uploaded(self):
        (self.root / 'data/browser/main/b.iwd').write_bytes(b'012345670')
        with self.assertRaisesRegex(ValueError, 'b.iwd is not the archive'):
            self.publisher.publish()
        self.assertEqual(self.wrangler_calls(), [])
        self.assertFalse(self.publisher.stage.exists())

    def test_players_an_update_or_missing_credentials_defer_the_upload(self):
        self.cloud.clients = 1
        with self.assertRaisesRegex(module.Deferred, 'Players are connected'):
            self.publisher.publish()
        self.cloud.clients = 0
        self.state.mkdir(parents=True, exist_ok=True)
        with (self.state / 'update.lock').open('a') as lock:
            fcntl.flock(lock, fcntl.LOCK_EX)
            with self.assertRaisesRegex(module.Deferred, 'An update is running'):
                self.publisher.publish()
        (self.config / 'cloudflare.env').unlink()
        with self.assertRaisesRegex(module.Deferred, 'credentials are missing'):
            self.publisher.publish()
        self.assertEqual(self.wrangler_calls(), [])
        self.assertFalse(self.publisher.stage.exists())

    def test_a_stopped_gateway_defers_rather_than_uploading(self):
        def run(*command, **_):
            if command[:2] == ('docker', 'exec'):
                raise RuntimeError('docker failed (1): No such container')
            return self.cloud.run(*command)
        self.publisher.run = run
        with self.assertRaisesRegex(module.Deferred, 'gateway is not answering'):
            self.publisher.publish()
        self.assertEqual(self.wrangler_calls(), [])

    def test_the_pinned_wrangler_image_is_built_once_when_missing(self):
        self.cloud.image = False
        self.publisher.publish()
        builds = [c for c in self.cloud.commands if c[:2] == ('docker', 'build')]
        self.assertEqual(builds, [('docker', 'build', '--build-arg', 'WRANGLER=' + module.WRANGLER, '-t',
                                   module.WRANGLER_IMAGE, str(self.root / 'downstream/cdn'))])
        self.assertIn('ARG WRANGLER', (REPO / 'downstream/cdn/Dockerfile').read_text())

    def test_install_runs_the_checkout_script_every_five_minutes(self):
        home = Path(self.directory.name) / 'home'
        with patch.object(module.Path, 'home', return_value=home), patch.object(module.subprocess, 'run') as run, \
                contextlib.redirect_stdout(io.StringIO()):
            module.install(self.root, self.config, self.state)
        service = (home / '.config/systemd/user/cod2-cdn-publish.service').read_text()
        self.assertIn(f'ExecStart=/usr/bin/python3 "{self.root}/scripts/publish-cdn.py" "--root" "{self.root}" '
                      f'"--config" "{self.config}" "--state" "{self.state}"', service)
        self.assertIn('OnUnitInactiveSec=5min', (home / '.config/systemd/user/cod2-cdn-publish.timer').read_text())
        self.assertEqual([c.args[0] for c in run.call_args_list], [['systemctl', '--user', 'daemon-reload'],
                         ['systemctl', '--user', 'enable', '--now', 'cod2-cdn-publish.timer']])


if __name__ == '__main__':
    unittest.main()
