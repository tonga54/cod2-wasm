// The class bridge as linked: the requests it makes, the lines it hands the
// UI, and the bodies the gateway's own rules accept.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const root = path.join(__dirname, '..');
const linked = fs.readFileSync(process.argv[2] || path.join(root, 'out/cod2-wasm-core/site/cod2.js'), 'utf8');
const bridge = fs.readFileSync(path.join(root, 'downstream/wasm/web_classes.c'), 'utf8');

// The UI's callbacks are the exports the bridge calls, with the arguments C takes.
assert.match(bridge, /EMSCRIPTEN_KEEPALIVE void web_classes_loaded\(int generation, int status, const char \*lines\)/);
assert.match(bridge, /EMSCRIPTEN_KEEPALIVE void web_classes_saved\(int generation, int status\)/);
for (const name of ['_web_classes_loaded', '_web_classes_saved'])
  assert(linked.includes(`var ${name} = Module["${name}"]`), `${name} is exported`);

function linkedFunction(source, name) {
  const head = `function ${name}(`, start = source.indexOf(head);
  assert(start >= 0, `${name} is linked`);
  const params = source.slice(start + head.length, source.indexOf(')', start)).split(',').map(p => p.trim());
  let end = source.indexOf('{', start) + 1, depth = 1;
  const open = end;
  while (depth) { depth += (source[end] === '{') - (source[end] === '}'); ++end; }
  return {params, body: source.slice(open, end - 1)};
}
const reply = (status, body) => Promise.resolve({ok: status >= 200 && status < 300, status,
  json: () => typeof body === 'string' ? Promise.reject(SyntaxError(body)) : Promise.resolve(body)});
const settle = () => new Promise(resolve => setTimeout(resolve, 0));

async function scenario(source, {normalizeClasses, defaultClasses}) {
  const strings = new Map(), freed = new Set();
  let nextPointer = 16;
  const UTF8ToString = pointer => { assert(strings.has(pointer), 'a live string'); return strings.get(pointer); };
  const stringToNewUTF8 = text => { const pointer = nextPointer += 64; strings.set(pointer, text); return pointer; };
  const _free = pointer => { assert(!freed.has(pointer)); freed.add(pointer); };
  const loaded = [], saved = [], requests = [];
  let respond = () => { throw Error('no response planned'); };
  const fetch = (url, options = {}) => { requests.push({url, options}); return respond(url, options); };
  const _web_classes_loaded = (generation, status, lines) => loaded.push({generation, status,
    text: lines ? (assert(!freed.has(lines), 'read before free'), strings.get(lines)) : null, lines});
  const _web_classes_saved = (generation, status) => saved.push({generation, status});
  const scope = {Module: {}, fetch, UTF8ToString, stringToNewUTF8, _free, _web_classes_loaded, _web_classes_saved};
  const bind = name => {
    const {params, body} = linkedFunction(source, name);
    return Function(...Object.keys(scope), ...params, body).bind(null, ...Object.values(scope));
  };
  const load = bind('Web_ClassesLoad'), save = bind('Web_ClassesSave');
  const classes = defaultClasses();
  classes[1] = {...classes[1], name: 'Kar\tRusher\nTwo', primaryCamo: 'frogskin', secondaryCamo: 'ember'};
  // A host from before camouflage sends classes without it.
  const {primaryCamo, secondaryCamo, ...unpainted} = classes[2];
  assert.deepEqual([primaryCamo, secondaryCamo], ['none', 'none']);
  classes[2] = unpainted;

  // A stored list: one line a class, tabs and line breaks in a field become
  // spaces; a class without camouflage keeps the guns' own finish.
  respond = () => reply(200, {classes, saved: true});
  load(stringToNewUTF8('Kar Rusher^1'), 7);
  await settle();
  assert.equal(requests[0].url, '/classes?player=Kar%20Rusher%5E1');
  assert.equal(requests[0].options.cache, 'no-store');
  const expected = classes.map((c, i) => [c.name.replace(/[\t\n]/g, ' '), c.primary, c.secondary, c.lethal, c.tactical,
    ...(i === 1 ? ['frogskin', 'ember'] : ['none', 'none'])].join('\t')).join('\n') + '\n';
  assert.deepEqual([loaded[0].generation, loaded[0].status, loaded[0].text], [7, 200, expected]);
  assert(freed.has(loaded[0].lines), 'the lines are freed after the call');

  // Refusals keep their status; a reply without classes is unavailable.
  for (const [status, body, expectedStatus] of [[400, {error: 'Choose a gametag in Options to save classes.'}, 400],
                                               [503, 'Service Unavailable', 503], [200, '<html>', 0],
                                               [200, {saved: false}, 0], [200, null, 0]]) {
    respond = () => reply(status, body);
    load(stringToNewUTF8('Kar Rusher'), 8);
    await settle();
    assert.deepEqual([loaded.at(-1).status, loaded.at(-1).text], [expectedStatus, '']);
  }
  respond = () => Promise.reject(TypeError('Failed to fetch'));
  load(stringToNewUTF8('Kar Rusher'), 9);
  await settle();
  assert.deepEqual(loaded.at(-1), {generation: 9, status: 0, text: null, lines: 0});

  // A host far past the limit is cut at 4096 bytes for the UI's buffer.
  respond = () => reply(200, {classes: Array(5).fill({name: 'x'.repeat(5000), primary: 'mp44_mp'})});
  load(stringToNewUTF8('Kar Rusher'), 10);
  await settle();
  assert.equal(loaded.at(-1).text.length, 4096);

  // Saves send what the gateway accepts, one at a time and in order.
  const lines = expected.replace('Kar Rusher Two', 'Kar Rusher');
  let release;
  respond = () => new Promise(resolve => { release = () => resolve(reply(200, {saved: true})); });
  requests.length = 0;
  save(stringToNewUTF8('Kar Rusher'), stringToNewUTF8(lines), 11);
  await settle();
  respond = () => reply(503, {error: 'Class storage is unavailable.'});
  save(stringToNewUTF8('Kar Rusher'), stringToNewUTF8(lines.replace('Assault', 'Point Man')), 11);
  await settle();
  assert.equal(requests.length, 1, 'the second save waits for the first');
  const first = requests[0];
  assert.equal(first.url, '/classes');
  assert.deepEqual([first.options.method, first.options.headers?.['Content-Type']], ['PUT', 'application/json']);
  const body = JSON.parse(first.options.body);
  assert.equal(body.player, 'Kar Rusher');
  assert.deepEqual(normalizeClasses(body.classes), body.classes, 'the gateway takes the body as sent');
  assert.equal(body.classes[1].name, 'Kar Rusher');
  assert.deepEqual([body.classes[1].primaryCamo, body.classes[1].secondaryCamo, body.classes[2].primaryCamo,
    body.classes[2].secondaryCamo], ['frogskin', 'ember', 'none', 'none']);
  release();
  await settle(); await settle();
  assert.equal(requests.length, 2);
  assert.equal(JSON.parse(requests[1].options.body).classes[0].name, 'Point Man');
  assert.deepEqual(saved, [{generation: 11, status: 200}, {generation: 11, status: 503}]);

  // An unreachable host reports 0, and the next save still goes out.
  respond = () => Promise.reject(TypeError('Failed to fetch'));
  save(stringToNewUTF8('Kar Rusher'), stringToNewUTF8(lines), 12);
  await settle(); await settle();
  respond = () => reply(200, {saved: true});
  save(stringToNewUTF8('Kar Rusher'), stringToNewUTF8(lines), 12);
  await settle(); await settle();
  assert.deepEqual(saved.slice(2), [{generation: 12, status: 0}, {generation: 12, status: 200}]);
}

// Edits of the linked code, as Emscripten prints it.
const mutants = [
  ['const status = response.ok && !classes ? 0 : response.status;', 'const status = response.status;'],
  ['encodeURIComponent(name)', 'name'],
  ['.split("\\t").join(" ")', ''],
  ['text.slice(0, 4096)', 'text'],
  ['_free(lines);', ''],
  ['.catch(() => _web_classes_loaded(generation, 0, 0))', ''],
  ['Module.cod2ClassSave = (Module.cod2ClassSave || Promise.resolve()).then(() =>', '(Promise.resolve()).then(() =>'],
  ['.catch(() => _web_classes_saved(generation, 0))', ''],
  ['method: "PUT",\n    headers: {\n      "Content-Type": "application/json"\n    },', 'method: "PUT",'],
  ['.filter(Boolean)', ''],
  ['item?.primaryCamo ?? "none"', 'item?.primaryCamo'],
  ['item?.secondaryCamo ?? "none"', 'item?.secondaryCamo'],
  ['        primaryCamo: primaryCamo,\n        secondaryCamo: secondaryCamo\n', ''],
];
(async () => {
  const gateway = await import(path.join(root, 'downstream/gateway/classes.mjs'));
  await scenario(linked, gateway);
  // A mutant without its catch leaves rejections nobody handles, or handles late.
  process.on('unhandledRejection', () => {});
  process.removeAllListeners('warning');
  let killed = 0;
  for (const [before, after] of mutants) {
    assert.equal(linked.split(before).length, 2, `one linked ${before}`);
    const failed = await Promise.race([scenario(linked.replace(before, after), gateway).then(() => false, () => true),
      new Promise(resolve => setTimeout(() => resolve(true), 2000))]);
    assert(failed, `the mutant survives: ${before}`);
    killed++;
  }
  console.log(`PASS: the linked class bridge loads lines for the UI with each gun's camouflage, none from a host ` +
    `before it, reports refusals and outages, and saves bodies the gateway accepts one at a time; ${killed} bridge ` +
    `mutants fail`);
})().catch(error => { console.error(error); process.exit(1); });
