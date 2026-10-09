#!/usr/bin/env python3
"""Exercise real Git ancestry and the updater's safety boundaries."""
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('updater', Path(__file__).with_name('update-local.py'))
updater = importlib.util.module_from_spec(spec)
spec.loader.exec_module(updater)


class UpdateTests(unittest.TestCase):
    def test_real_git_ancestry(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            def git(*args):
                return updater.git(*args, root=root).stdout.strip()
            git('init', '-b', 'master')
            git('config', 'user.name', 'Fixture')
            git('config', 'user.email', 'fixture@example.invalid')
            git('remote', 'add', 'origin', 'https://github.com/tonga54/cod2-wasm.git')
            git('commit', '--allow-empty', '-m', 'base')
            base = git('rev-parse', 'HEAD')
            git('update-ref', updater.REMOTE, base)
            self.assertEqual(updater.comparison(root), ('current', 0))
            git('commit', '--allow-empty', '-m', 'upstream')
            latest = git('rev-parse', 'HEAD')
            git('update-ref', updater.REMOTE, latest)
            git('checkout', '--detach', base)
            self.assertEqual(updater.comparison(root), ('available', 1))
            git('commit', '--allow-empty', '-m', 'local')
            self.assertEqual(updater.comparison(root), ('diverged', 0))
            git('checkout', 'master')
            git('commit', '--allow-empty', '-m', 'ahead')
            self.assertEqual(updater.comparison(root), ('local-ahead', 0))
            self.assertEqual(updater.source_status(root), ('master', False))
            (root / 'local.txt').write_text('do not overwrite')
            self.assertEqual(updater.source_status(root), ('master', True))
            git('remote', 'set-url', 'origin', 'https://github.com/elsewhere/other.git')
            with self.assertRaises(RuntimeError): updater.source_status(root)

    def test_changes_and_wrong_branch_block_before_fetch(self):
        for status in [('master', True), ('my-work', False)]:
            with patch.object(sys, 'argv', ['update-local.py', '--apply']), \
                    patch.object(updater, 'source_status', return_value=status), \
                    patch.object(updater, 'run') as run:
                with self.assertRaises(RuntimeError): updater.main()
                run.assert_not_called()

    def test_active_or_unreachable_host_blocks_install(self):
        with patch.object(updater.subprocess, 'check_output', return_value='container'), \
                patch.object(updater.urllib.request, 'urlopen') as request:
            request.return_value.__enter__.return_value.read.return_value = b'{"gateway":"ready","clients":2}'
            with self.assertRaisesRegex(RuntimeError, 'personas conectadas'): updater.ensure_idle()
            request.side_effect = OSError('offline')
            with self.assertRaisesRegex(RuntimeError, 'comprobar'): updater.ensure_idle()

    def test_check_only_never_builds_or_merges(self):
        with patch.object(sys, 'argv', ['update-local.py']), \
                patch.object(updater, 'source_status', return_value=('master', False)), \
                patch.object(updater, 'comparison', return_value=('available', 3)), \
                patch.object(updater, 'run') as run:
            self.assertEqual(updater.main(), 1)
            self.assertEqual(run.call_count, 1)
            self.assertEqual(run.call_args.args[:2], ('git', 'fetch'))

    def test_failed_build_never_restarts_containers(self):
        calls = []
        def run(*args):
            calls.append(args)
            if args == ('bash', 'scripts/build-docker.sh'):
                raise subprocess.CalledProcessError(1, args)
        with patch.object(sys, 'argv', ['update-local.py', '--apply']), \
                patch.object(updater, 'source_status', return_value=('master', False)), \
                patch.object(updater, 'comparison', return_value=('available', 1)), \
                patch.object(updater, 'ensure_idle'), patch.object(updater.Path, 'exists', return_value=True), \
                patch.object(updater.Path, 'is_file', return_value=True), patch.object(updater, 'run', side_effect=run):
            with self.assertRaises(subprocess.CalledProcessError): updater.main()
            self.assertIn(('git', 'merge', '--ff-only', updater.REMOTE), calls)
            self.assertNotIn(('docker', 'compose', 'up', '-d'), calls)


if __name__ == '__main__':
    unittest.main()
