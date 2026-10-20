const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');

const production = fs.readFileSync(path.join(__dirname, '../downstream/wasm/web_haptics.js'), 'utf8');
let unhandled = 0;
process.on('unhandledRejection', () => { unhandled++; });

// A pad as the Gamepad API reports it: Chrome's dual-rumble actuator, Firefox's
// haptic pulse, or none.
function pad(index, { timestamp = 0, held = false, kind = 'chrome', fail = false, connected = true } = {}) {
  const log = [];
  const result = value => fail === 'throw' ? (() => { throw new Error('NotAllowedError'); })()
    : fail ? Promise.reject(new Error('NotSupportedError')) : Promise.resolve(value);
  return {
    index, connected, timestamp, log,
    buttons: [{ pressed: false, value: 0 }, { pressed: held, value: held ? 1 : 0 }],
    axes: [0, 0, 0, 0],
    vibrationActuator: kind === 'chrome' ? {
      playEffect(type, params) { log.push(['play', type, { ...params }]); return result('complete'); },
      reset() { log.push(['reset']); return result('complete'); },
    } : undefined,
    hapticActuators: kind === 'firefox' ? [{ pulse(value, ms) { log.push(['pulse', value, ms]); return result(true); } }] : undefined,
  };
}
const effect = (strong, weak) => ['play', 'dual-rumble', { startDelay: 0, duration: 150, strongMagnitude: strong, weakMagnitude: weak }];

async function scenario(source) {
  const clock = { now: 1000, queries: 0 };
  let pads = [pad(0)];
  const scope = { Module: {}, performance: { now: () => clock.now } };
  scope.globalThis = { navigator: { getGamepads: () => { clock.queries++; return pads; } } };
  vm.runInNewContext(source, scope);
  const output = scope.Module.cod2CreateHaptics();
  unhandled = 0;

  // Each command plays for 150 ms. A change of 0.1 or more goes out 30 ms
  // after the last command at the soonest, a smaller one waits for the 60 ms
  // refresh that keeps a steady rumble from gapping, and a rise of 0.15 or
  // more, a round's kick, goes out at once.
  output.rumble(1, 0.5);
  assert.deepEqual(pads[0].log, [effect(1, 0.5)]);
  clock.now += 20; output.rumble(1, 0.5); output.rumble(0.95, 0.55); output.rumble(0.85, 0.5);
  assert.equal(pads[0].log.length, 1);
  clock.now += 10; output.rumble(0.85, 0.5);
  assert.deepEqual(pads[0].log[1], effect(0.85, 0.5));
  clock.now += 35; output.rumble(0.8, 0.52);
  assert.equal(pads[0].log.length, 2);
  clock.now += 5; output.rumble(0.8, 0.72);
  assert.deepEqual(pads[0].log[2], effect(0.8, 0.72));
  clock.now += 5; output.rumble(0.8, 0.92);
  assert.deepEqual(pads[0].log[3], effect(0.8, 0.92));
  clock.now += 59; output.rumble(0.8, 0.92);
  assert.equal(pads[0].log.length, 4);
  clock.now += 1; output.rumble(0.8, 0.92);
  assert.deepEqual(pads[0].log[4], effect(0.8, 0.92));
  // Out-of-range and invalid values are held to 0..1.
  output.rumble(2, -1); output.rumble(NaN, 0.25);
  assert.deepEqual(pads[0].log.slice(5), [effect(1, 0), effect(0, 0.25)]);
  // Still motors are reset once; while still nothing asks for the pads.
  output.rumble(0, 0);
  assert.deepEqual(pads[0].log.at(-1), ['reset']);
  const queries = clock.queries, sent = pads[0].log.length;
  for (let i = 0; i < 100; i++) output.rumble(0, 0);
  assert.equal(clock.queries, queries); assert.equal(pads[0].log.length, sent);

  // The pad being played with: one held or moved before the latest to report;
  // pads that cannot vibrate, or are gone, are passed over.
  pads = [pad(0, { timestamp: 50, held: true }), pad(1, { timestamp: 90 }), pad(2, { timestamp: 99, kind: 'none' }),
    pad(3, { timestamp: 120, connected: false }), null];
  output.rumble(0.5, 0.5);
  assert.equal(pads[0].log.length, 1); assert.equal(pads[1].log.length, 0);
  // A burst stays on its pad while another starts being used.
  pads[1].buttons[1] = { pressed: true, value: 1 }; pads[1].timestamp = 200;
  clock.now += 100; output.rumble(0.6, 0.6);
  assert.equal(pads[0].log.length, 2); assert.equal(pads[1].log.length, 0);
  output.rumble(0, 0);
  assert.deepEqual(pads[0].log.at(-1), ['reset']);
  pads[0].buttons[1] = { pressed: false, value: 0 };
  output.rumble(0.7, 0.2);
  assert.deepEqual(pads[1].log, [effect(0.7, 0.2)]);
  // Its pad unplugged mid-burst, the burst moves to the one left.
  pads[1] = null;
  clock.now += 100; output.rumble(0.8, 0.2);
  assert.deepEqual(pads[0].log.at(-1), effect(0.8, 0.2));
  output.rumble(0, 0);
  pads = [pad(0, { timestamp: 10 }), pad(1, { timestamp: 20 }), pad(2, { timestamp: 30, connected: false })];
  output.rumble(0.3, 0.3);
  assert.equal(pads[0].log.length, 0); assert.deepEqual(pads[1].log, [effect(0.3, 0.3)]);
  assert.equal(pads[2].log.length, 0);
  output.rumble(0, 0);

  // Firefox's single actuator pulses the stronger motor, and stops at zero.
  pads = [pad(0, { kind: 'firefox' })];
  output.rumble(0.4, 0.9); output.rumble(0, 0);
  assert.deepEqual(pads[0].log, [['pulse', 0.9, 150], ['pulse', 0, 0]]);

  // Refusals, rejected or thrown, never escape; no pads at all is fine.
  pads = [pad(0, { fail: true })];
  output.rumble(1, 1); output.rumble(0, 0);
  pads = [pad(0, { fail: 'throw' })];
  output.rumble(1, 1); output.rumble(0, 0);
  pads = [];
  output.rumble(1, 1); output.rumble(0, 0);
  await new Promise(resolve => setTimeout(resolve, 0));
  assert.equal(unhandled, 0);
}

const mutants = [
  ['EFFECT_MS = 150', 'EFFECT_MS = 1000'],
  ['REFRESH_MS = 60', 'REFRESH_MS = 600'],
  ['STEP = 0.1', 'STEP = 0'],
  ['MIN_GAP_MS = 30', 'MIN_GAP_MS = 0'],
  ['KICK = 0.15', 'KICK = 1'],
  ['if (result && typeof result.catch === \'function\') result.catch(() => {});', ''],
  ['pad.connected !== false && ', ''],
  ['(active && !bestBusy) || ', ''],
  ['if (actuator && !ms && actuator.reset) quiet(actuator.reset());\n      else if (actuator)', 'if (actuator)'],
  ['quiet(pad.hapticActuators[0].pulse(Math.max(s, w), ms));', 'quiet(pad.hapticActuators[0].pulse(s, ms));'],
  ['} catch (error) {}', '} catch (error) { throw error; }'],
  ['s = Math.min(1, Math.max(0, +s || 0));', 's = +s;'],
  ['const pad = target === null ? null : pads().find(', 'const pad = pads().find('],
  ['const pad = (target !== null && list.find(p => vibrates(p) && p.index === target)) || pick(list);',
    'const pad = pick(list);'],
];

(async () => {
  await scenario(production);
  for (const [before, after] of mutants) {
    assert.equal(production.split(before).length, 2, before);
    const survived = await scenario(production.replace(before, after)).then(() => true, () => false);
    assert(!survived, `mutant survived: ${before}`);
  }
  console.log('PASS: controller vibration plays each command for 150 ms, a kick at once and other changes 30 ms '
    + 'apart at the soonest, refreshes steady motors every 60 ms, '
    + 'resets once when still and stays idle after; the pad held or latest used, kept for a burst; Firefox pulses; '
    + `refusals contained; ${mutants.length} failing regression mutants`);
})().catch(error => { console.error(error); process.exit(1); });
