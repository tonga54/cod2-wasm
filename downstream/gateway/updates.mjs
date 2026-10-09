import http from 'node:http';

const repository = 'https://github.com/tonga54/cod2-wasm';
const api = 'https://api.github.com/repos/tonga54/cod2-wasm';
const sha = /^[a-f0-9]{40}$/;

export function readBuildInfo(webHost, webPort) {
  return new Promise((resolve, reject) => {
    const request = http.get({ hostname: webHost, port: webPort,
      path: '/build-info.json', timeout: 2000 }, response => {
      const chunks = []; let bytes = 0;
      response.on('data', chunk => {
        bytes += chunk.length;
        if (bytes > 4096) response.destroy(new Error('Build metadata too large'));
        else chunks.push(chunk);
      });
      response.on('error', reject);
      response.on('end', () => {
        try {
          if (response.statusCode !== 200) throw new Error('Build metadata unavailable');
          const build = JSON.parse(Buffer.concat(chunks));
          if (!sha.test(build.revision) || !/^[a-f0-9]{64}$/.test(build.buildId) ||
              typeof build.dirty !== 'boolean') throw new Error('Invalid build metadata');
          resolve({ revision: build.revision, buildId: build.buildId, dirty: build.dirty });
        } catch (error) { reject(error); }
      });
    });
    request.on('timeout', () => request.destroy(new Error('Build metadata timed out')));
    request.on('error', reject);
  });
}

// One shared GitHub check per host every five minutes, including on failure.
// Browsers cannot supply a repository, ref, URL, or update command.
export function createUpdateChecker({ readBuild, fetchImpl = fetch, now = Date.now,
  interval = 300000 } = {}) {
  let cached, pending, expires = 0;
  async function github(path) {
    const response = await fetchImpl(api + path, {
      headers: { Accept: 'application/vnd.github+json', 'User-Agent': 'cod2-wasm-update-check',
        'X-GitHub-Api-Version': '2022-11-28' },
      signal: AbortSignal.timeout(4000), redirect: 'error'
    });
    if (!response.ok) throw new Error('GitHub check unavailable');
    const text = await response.text();
    if (text.length > 131072) throw new Error('GitHub response too large');
    return JSON.parse(text);
  }
  async function compare(revision) {
    try {
      const ref = await github('/git/ref/heads/master');
      const latest = ref.object?.sha;
      if (!sha.test(latest)) throw new Error('Invalid repository revision');
      if (latest === revision) return { status: 'current', available: false, revision: latest };
      // Page two excludes the potentially large list of changed file patches.
      const result = await github(`/compare/${revision}...${latest}?per_page=1&page=2`);
      const status = { ahead: 'available', behind: 'local-ahead',
        diverged: 'diverged', identical: 'current' }[result.status];
      if (!status) throw new Error('Invalid comparison');
      return { status, available: status === 'available', revision: latest };
    } catch {
      // Offline/rate-limited is unknown, never a false "up to date" result.
      return { status: 'unknown', available: false, revision: null };
    }
  }
  return async function check() {
    let current;
    try { current = await readBuild(); }
    catch { return { current: null, repository, update: { status: 'unknown', available: false } }; }
    if (!cached || cached.base !== current.revision || now() >= expires) {
      if (!pending || pending.base !== current.revision) {
        const base = current.revision;
        const task = { base };
        task.promise = compare(base).then(update => {
          cached = { base, update }; expires = now() + interval;
          return update;
        }).finally(() => { if (pending === task) pending = null; });
        pending = task;
      }
      const update = await pending.promise;
      return { current, repository, update };
    }
    return { current, repository, update: cached.update };
  };
}
