#!/usr/bin/env node
'use strict';
// Execute the production scheduler's entry path with frame-quantized GSC waits.
// The full engine/asset integration check is scripts/test-bot-runtime.py --fps 30.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(process.argv[2] || path.join(__dirname, '../downstream/server/browser_bots.gsc'), 'utf8');
const begin = source.indexOf('objectiveState()\n{');
const end = source.indexOf('// Keep the shared cache', begin);
assert(begin >= 0 && end > begin, 'Locate the production objective scheduler');
const prefix = source.slice(begin, end)
  .replace(/\/\/[^\n]*/g, '')
  .replace('objectiveState()', 'function* objectiveState()')
  .replace('level endon("intermission");', 'let phase, nextRoster;')
  .replace(/wait ([.\d]+);/g, 'yield $1;');
const braces = [...prefix].reduce((n, c) => n + (c === '{') - (c === '}'), 0);
// Both rebuilding a roster and accessing an existing objective bucket are
// forbidden in DM/TDM. Instrument those two boundaries without mocking the guard.
const program = prefix + 'throw Error("DM/TDM entered roster rebuild");\n' +
  '}\nthrow Error("DM/TDM entered objective buckets");\n' + '}'.repeat(braces - 1) +
  '\nobjectiveState();';
let schedules = 0;
for (const mode of ['dm', 'tdm']) {
  for (const fps of [20, 30, 40, 60]) {
    for (const debug of [0, 1]) {
      let now = 0;
      const level = {};
      const iterator = vm.runInNewContext(program, {
        level, getTime: () => now, getCvar: () => mode,
        getCvarInt: () => debug, isDefined: value => value !== undefined,
        getTeamScore: () => 0, println: () => {}
      });
      for (let step = 0; step < 80; step++) {
        const resumed = iterator.next();
        assert.equal(resumed.done, false);
        assert.equal(resumed.value, .5);
        // The VM encodes waits in 20 Hz ticks; the server advances one script
        // tick per frame. At 30 Hz this resumes before a 500 ms roster deadline.
        now += Math.round(resumed.value * 20) * Math.floor(1000 / fps);
        if (step % 7 === 0) now += 73; // Also cross delayed/catch-up deadlines.
      }
      assert.equal(level.bot_phases, undefined);
      schedules++;
    }
  }
}
console.log(`PASS: production DM/TDM scheduler bypasses objective rosters in ${schedules} clock/debug schedules`);
