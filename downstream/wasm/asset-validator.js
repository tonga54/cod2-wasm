(() => {
  'use strict';

  const button = document.getElementById('validate');
  const output = document.getElementById('asset-log');
  const loopbackNames = new Set(['127.0.0.1', 'localhost', '::1', '[::1]']);
  const expectedNames = [
    ...Array.from({length: 16}, (_, index) => `main/iw_${String(index).padStart(2, '0')}.iwd`),
    ...Array.from({length: 12}, (_, index) => `main/localized_english_iw${String(index).padStart(2, '0')}.iwd`)
  ];
  let manifest = null;

  const log = (message) => { output.textContent += `${message}\n`; };

  function validateManifest(value) {
    if (!value || value.schema !== 1 || value.game !== 'Call of Duty 2' || !Array.isArray(value.files)) {
      throw new Error('owner manifest has the wrong schema');
    }
    if (value.files.length !== expectedNames.length) {
      throw new Error(`owner manifest must contain exactly ${expectedNames.length} files`);
    }
    value.files.forEach((entry, index) => {
      if (entry.path !== expectedNames[index]) throw new Error(`unexpected path at entry ${index}`);
      if (!Number.isSafeInteger(entry.size) || entry.size <= 4) throw new Error(`invalid size for ${entry.path}`);
      if (!/^[0-9a-f]{64}$/.test(entry.sha256)) throw new Error(`invalid SHA-256 for ${entry.path}`);
      if (entry.header !== '504b0304') throw new Error(`invalid IWD header for ${entry.path}`);
    });
  }

  async function validateFile(entry) {
    const url = `/local-data/${entry.path}`;
    const head = await fetch(url, {method: 'HEAD', cache: 'no-store', credentials: 'same-origin'});
    if (!head.ok) throw new Error(`${entry.path}: HTTP ${head.status}`);
    const servedSize = Number(head.headers.get('content-length'));
    if (servedSize !== entry.size) {
      throw new Error(`${entry.path}: expected ${entry.size} bytes, server reported ${servedSize}`);
    }

    const response = await fetch(url, {
      headers: {Range: 'bytes=0-3'}, cache: 'no-store', credentials: 'same-origin'
    });
    if (response.status !== 206) throw new Error(`${entry.path}: byte-range request returned ${response.status}`);
    const bytes = new Uint8Array(await response.arrayBuffer());
    const header = Array.from(bytes, byte => byte.toString(16).padStart(2, '0')).join('');
    if (bytes.length !== 4 || header !== entry.header) {
      throw new Error(`${entry.path}: expected ZIP header ${entry.header}, received ${header}`);
    }
  }

  async function loadManifest() {
    output.textContent = '';
    if (!loopbackNames.has(location.hostname)) {
      log('Blocked: owner data is accepted only from a loopback origin.');
      return;
    }
    try {
      const response = await fetch('owner-manifest.json', {cache: 'no-store', credentials: 'same-origin'});
      if (!response.ok) throw new Error(`manifest HTTP ${response.status}`);
      manifest = await response.json();
      validateManifest(manifest);
      const total = manifest.files.reduce((sum, entry) => sum + entry.size, 0);
      log(`Private manifest ready: ${manifest.files.length} files, ${total.toLocaleString()} bytes.`);
      log('Click Validate to check the read-only loopback mount without downloading whole IWDs.');
      button.disabled = false;
    } catch (error) {
      log(`Owner data unavailable: ${error.message}.`);
      log('Rebuild with COD2_OWNER_DATA set to the legally owned main directory.');
    }
  }

  button.addEventListener('click', async () => {
    button.disabled = true;
    output.textContent = '';
    try {
      for (let index = 0; index < manifest.files.length; ++index) {
        const entry = manifest.files[index];
        await validateFile(entry);
        log(`[${index + 1}/${manifest.files.length}] ${entry.path}`);
      }
      log('Owner data mount validated. No IWD body was copied into MEMFS.');
    } catch (error) {
      log(`Validation failed: ${error.message}`);
    } finally {
      button.disabled = false;
    }
  });

  loadManifest();
})();
