#!/usr/bin/env python3
"""Exercise update identity, admission races, transactions and rollback failures."""
import copy
import hashlib
import importlib.util
import io
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('pi_updater', Path(__file__).with_name('update-pi.py'))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
OLD, NEW = 'a' * 40, 'b' * 40


def manifest():
    return {'schemaVersion': 2, 'revision': NEW, 'platform': 'linux/arm64',
            'build': {'revision': NEW, 'buildId': 'c' * 64, 'dirty': False},
            'assetManifestSha256': 'd' * 64,
            'images': {s: {'tag': f'{module.RELEASE_REPOS[s]}:release-{NEW}', 'id': 'sha256:' + 'e' * 64, 'fingerprint':'1'*64} for s in module.SERVICES},
            'archive': {'name': 'pi-images.tar.gz', 'size': 100, 'sha256': 'f' * 64}}


class FakeUpdater(module.Updater):
    def __init__(self, directory):
        base = Path(directory)
        super().__init__(base/'repo', base/'state', base/'originals')
        self.root.mkdir()
        (self.assets/'main').mkdir(parents=True)
        (self.assets/'web').mkdir()
        (self.assets/'main/value').write_text('old private assets')
        self.originals.mkdir()
        (self.originals/'owner-original').write_text('preserve the owner archive')
        for name in ('compose.yaml','compose.pi.yaml'):
            (self.root/name).write_text('old compose')
        self.release = self.state/'releases'/NEW
        (self.release/'assets/main').mkdir(parents=True)
        (self.release/'assets/web').mkdir()
        (self.release/'assets/main/value').write_text('new private assets')
        (self.release/'source').mkdir()
        self.head = OLD
        self.dirty = False
        self.clients = 0
        self.calls = []
        self.fail_verify = False
        self.edit_source = False
        self.old_images = {s:'sha256:'+'a'*64 for s in module.SERVICES}

    def guard_source(self):
        if self.dirty:
            raise module.Deferred('Human edits')
        return self.head

    def git(self,*args):
        self.calls.append(('git',args))
        if args==('rev-parse','origin/main'):
            return NEW
        if args[:1]==('diff',):
            return 'src/PC/bgame/bg_weapons.c'
        if args[:1]==('merge',):
            self.head=NEW
        if args[:1]==('reset',):
            self.head=OLD
        return ''

    def run(self,*args,**kwargs):
        self.calls.append(('run',args))
        return ''

    def compose(self,source,project,assets,images,*args,**kwargs):
        self.calls.append(('compose',str(source),project,args,dict(images)))
        return ''

    def current_images(self):
        return self.old_images

    def health(self):
        return {'gateway':'ready','clients':self.clients,'pendingClients':0,'updating':True}

    def verify(self,*args,**kwargs):
        if self.edit_source:
            self.dirty=True
        if self.fail_verify:
            raise RuntimeError('New server failed its smoke check')
        return 'passed'

    def installed_revision(self):
        return self.head


class Tests(unittest.TestCase):
    def test_image_identity_is_portable_across_docker_stores(self):
        classic={'Id':'sha256:'+'a'*64,'Architecture':'arm64','Os':'linux',
                 'RootFS':{'Layers':['sha256:'+'b'*64]},'Config':{'Cmd':['node','server.mjs'],'Labels':{'revision':NEW}}}
        containerd=copy.deepcopy(classic)
        containerd['Id']='sha256:'+'c'*64
        containerd['Descriptor']={'digest':containerd['Id']}
        containerd['Config']['OnBuild']=[]
        self.assertEqual(module.image_fingerprint(classic),module.image_fingerprint(containerd))
        containerd['RootFS']['Layers'].append('sha256:'+'d'*64)
        self.assertNotEqual(module.image_fingerprint(classic),module.image_fingerprint(containerd))
        containerd=copy.deepcopy(classic);containerd['Config']['Cmd']=['different']
        self.assertNotEqual(module.image_fingerprint(classic),module.image_fingerprint(containerd))
    def test_manifest_identity(self):
        self.assertEqual(module.validate_manifest(manifest(),NEW)['revision'],NEW)
        corruptions=[('platform','linux/amd64'),('revision',OLD),('assetManifestSha256','invalid')]
        for key,value in corruptions:
            data=manifest();data[key]=value
            with self.assertRaises(ValueError):module.validate_manifest(data,NEW)
        for change in ('dirty','tag','size','name'):
            data=manifest()
            if change=='dirty':data['build']['dirty']=True
            if change=='tag':data['images']['cod2-web']['tag']='other/image:latest'
            if change=='size':data['archive']['size']=2*1024**3
            if change=='name':data['archive']['name']='../../outside'
            with self.assertRaises(ValueError):module.validate_manifest(data,NEW)

    def test_idle_checks_pending_upgrades_and_invalid_health(self):
        self.assertTrue(module.is_idle({'gateway':'ready','clients':0}))
        for health in [{},{'gateway':'down','clients':0},{'gateway':'ready','clients':1},
                       {'gateway':'ready','clients':0,'pendingClients':1},
                       {'gateway':'ready','clients':False},{'gateway':'ready','clients':'0'}]:
            self.assertFalse(module.is_idle(health))

    def test_download_hash_and_partial_file_cleanup(self):
        with tempfile.TemporaryDirectory() as directory:
            updater=FakeUpdater(directory)
            path=updater.state/'download'
            payload=b'new artifact'
            with patch.object(module.urllib.request,'urlopen',return_value=io.BytesIO(payload)):
                updater.download('https://example.invalid',path,len(payload),hashlib.sha256(payload).hexdigest())
            self.assertEqual(path.read_bytes(),payload)
            with patch.object(module.urllib.request,'urlopen',return_value=io.BytesIO(b'corrupt')):
                with self.assertRaises(ValueError):updater.download('https://example.invalid',path,2,'f'*64)
            self.assertEqual(path.read_bytes(),payload)
            self.assertFalse(path.with_suffix('.part').exists())

    def test_success_preserves_originals_and_pins_previous_images(self):
        with tempfile.TemporaryDirectory() as directory:
            updater=FakeUpdater(directory)
            updater.activate(updater.release,manifest())
            self.assertEqual((updater.assets/'main/value').read_text(),'new private assets')
            self.assertEqual((updater.release/'previous-assets/main/value').read_text(),'old private assets')
            self.assertEqual((updater.originals/'owner-original').read_text(),'preserve the owner archive')
            self.assertEqual(updater.head,NEW)
            self.assertFalse(updater.transaction_path.exists())
            self.assertFalse((updater.assets/'web/host-maintenance.json').exists())
            self.assertTrue(any('rollback-'+OLD in ' '.join(call[1]) for call in updater.calls if call[0]=='run'))

    def test_failed_health_restores_assets_and_previous_images(self):
        with tempfile.TemporaryDirectory() as directory:
            updater=FakeUpdater(directory);updater.fail_verify=True
            with self.assertRaises(RuntimeError):updater.activate(updater.release,manifest())
            self.assertEqual((updater.assets/'main/value').read_text(),'old private assets')
            self.assertEqual(updater.head,OLD)
            restores=[c for c in updater.calls if c[0]=='compose' and c[1].endswith('previous-source')]
            self.assertEqual(restores[-1][-1],module.ALIASES)
            self.assertFalse(updater.transaction_path.exists())
            self.assertFalse((updater.assets/'web/host-maintenance.json').exists())

    def test_human_edits_survive_failed_activation(self):
        with tempfile.TemporaryDirectory() as directory:
            updater=FakeUpdater(directory);updater.edit_source=True
            with self.assertRaises(module.Deferred):updater.activate(updater.release,manifest())
            self.assertTrue(updater.dirty)
            self.assertFalse(any(c[0]=='git' and c[1][0]=='reset' for c in updater.calls))
            self.assertEqual((updater.assets/'main/value').read_text(),'old private assets')

    def test_new_player_defers_without_replacing_anything(self):
        with tempfile.TemporaryDirectory() as directory:
            updater=FakeUpdater(directory);updater.clients=1
            with self.assertRaises(module.Deferred):updater.activate(updater.release,manifest())
            self.assertEqual((updater.assets/'main/value').read_text(),'old private assets')
            self.assertFalse(updater.transaction_path.exists())
            self.assertFalse((updater.assets/'web/host-maintenance.json').exists())
            self.assertFalse((updater.release/'assets/web/host-maintenance.json').exists())

    def test_invalid_candidate_never_reaches_activation(self):
        with tempfile.TemporaryDirectory() as directory:
            updater=FakeUpdater(directory)
            with patch.object(updater,'prepare',return_value=(updater.release,manifest())), \
                 patch.object(updater,'validate_candidate',side_effect=RuntimeError('Candidate failed')), \
                 patch.object(updater,'activate') as activate:
                with self.assertRaises(RuntimeError):updater.check()
            activate.assert_not_called()
            self.assertEqual(updater.record['failedRevision'],NEW)
            self.assertEqual((updater.assets/'main/value').read_text(),'old private assets')

    def test_pending_ci_is_retried_without_quarantining_main(self):
        with tempfile.TemporaryDirectory() as directory:
            updater=FakeUpdater(directory)
            with patch.object(updater,'prepare',side_effect=module.Deferred('CI is pending')):
                updater.check()
            self.assertEqual(updater.record['status'],'waiting')
            self.assertNotEqual(updater.record.get('failedRevision'),NEW)
            self.assertEqual((updater.assets/'main/value').read_text(),'old private assets')

    def test_hard_killed_activation_recovers_on_next_check(self):
        with tempfile.TemporaryDirectory() as directory:
            updater=FakeUpdater(directory)
            snapshot=updater.release/'previous-source';snapshot.mkdir()
            for name in ('compose.yaml','compose.pi.yaml'):
                (snapshot/name).write_text('old compose')
            updater.assets.replace(updater.release/'previous-assets')
            (updater.release/'assets').replace(updater.assets)
            updater.head=NEW
            module.atomic_json(updater.transaction_path,{'revision':NEW,'previousRevision':OLD,
                'previousImages':updater.old_images,'release':str(updater.release)})
            updater.check()
            self.assertEqual(updater.head,OLD)
            self.assertEqual((updater.assets/'main/value').read_text(),'old private assets')
            self.assertFalse(updater.transaction_path.exists())
            self.assertEqual(updater.record['status'],'rejected')


if __name__=='__main__':unittest.main()
