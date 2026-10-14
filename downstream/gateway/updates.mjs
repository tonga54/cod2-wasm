import http from 'node:http';

const repository = 'https://github.com/tonga54/cod2-wasm';
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

// Browsers only need the installed content ID. Host updates are handled by the
// Pi service; available=false also silences legacy repository-update popups.
export function createUpdateChecker({ readBuild } = {}) {
  return async function check() {
    try {
      return { current: await readBuild(), repository,
        update: { status: 'host-managed', available: false } };
    } catch {
      return { current: null, repository,
        update: { status: 'unknown', available: false } };
    }
  };
}
