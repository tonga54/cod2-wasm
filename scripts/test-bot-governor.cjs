#!/usr/bin/env node
'use strict';
// Execute the production bot governor (browser_bots.gsc) second by second
// against sv_lateFrames / sv_cpuLoad samples. The engine side is
// scripts/test-server-load.py; the full match is scripts/test-bot-runtime.py.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(process.argv[2] || path.join(__dirname, '../downstream/server/browser_bots.gsc'), 'utf8');

function extract(name) {
  const begin = source.indexOf(`\n${name}\n{`);
  assert(begin >= 0, name);
  let end = source.indexOf('{', begin), depth = 0;
  do { depth += (source[end] === '{') - (source[end] === '}'); ++end; } while (depth);
  return source.slice(begin, end);
}
// GSC to JS: waits become yields, method calls become property calls.
const program = (extract('governor()') + extract('governorChoice(bots)'))
  .replace(/\/\/[^\n]*/g, '')
  .replace('\ngovernor()', '\nfunction* governor()')
  .replace('\ngovernorChoice(bots)', '\nfunction governorChoice(bots)')
  .replace('level endon("intermission");', '')
  .replace(/wait ([.\d]+);/g, 'yield $1;')
  .replace('members["allies"] = 0;', 'var members = {}; members["allies"] = 0;')
  .replace(/bot thread joinGame\(difficulty\);/g, 'bot.joinGame(difficulty);')
  .replace(/([\w\]\)]+) (isBot|getEntityNumber|botRemove)\(\)/g, '$1.$2()');
assert.match(source, /level thread governor\(\);/, 'main() starts the governor');
assert.match(source, /if\(getCvar\("scr_bot_capacity"\) != "" && getCvarInt\("scr_bot_capacity"\) < count\)\s+count = getCvarInt\("scr_bot_capacity"\);/,
  'populate() keeps the carried count across the rotation');
const engine = fs.readFileSync(path.join(__dirname, '../src/PC/game_mp/g_client_script_cmd_mp.c'), 'utf8');
assert.match(engine, /\{ "botremove", \(BuiltinMethod\)PlayerCmd_BotRemove, 0 \}/);
assert.match(engine, /PlayerCmd_GetBotEntity\(entref\)\)\s+return;\s+Cbuf_ExecuteText\(2, va\("botkick %i\\n", \(int\)entref\.entnum\)\);/,
  'botRemove only drops test clients, after the script frame, without kick()\'s temporary ban');
const commands = fs.readFileSync(path.join(__dirname, '../src/PC/server_mp/sv_ccmds_mp.c'), 'utf8');
assert.match(commands, /if \(cl && cl->state && cl->bIsTestClient && cl->netchan\.remoteAddress\.type == NA_BOT\)\s+SV_DropClient\(cl, "EXE_DISCONNECTED"\);/,
  'botkick spares a slot a human has taken since the script chose it');
assert.match(commands, /Cmd_AddCommand\("botkick", [^;]+SV_BotKick_f\);/);
assert.match(commands, /Cmd_RemoveCommand\("botkick"\);/);

function match({ bots = 8, humans = 2, target = 8, deferKicks = false } = {}) {
  let next = 0;
  const players = [], events = [], cvars = { scr_bot_count: String(target), scr_bot_difficulty: '2' };
  const sample = { late: 0, cpu: 0 }, queued = [];
  const drop = entity => players.splice(players.indexOf(entity), 1);
  const player = (bot, team) => {
    const entity = { number: next++, bot, pers: { team }, sessionstate: 'playing', alive: true,
      isBot() { return this.bot; }, getEntityNumber() { return this.number; },
      // botkick runs from the command buffer, after the script frame.
      botRemove() { events.push(['remove', this.number]); if (deferKicks) queued.push(this); else drop(this); },
      joinGame(difficulty) { events.push(['join', this.number, difficulty]); } };
    players.push(entity);
    return entity;
  };
  for (let i = 0; i < humans; i++) player(false, i % 2 ? 'axis' : 'allies');
  for (let i = 0; i < bots; i++) player(true, i % 2 ? 'axis' : 'allies');
  const context = vm.createContext({
    getCvar: name => cvars[name] ?? '', setCvar: (name, value) => { cvars[name] = String(value); },
    getCvarInt: name => name === 'sv_lateFrames' ? sample.late : name === 'sv_cpuLoad' ? sample.cpu :
      parseInt(cvars[name] ?? '0', 10) || 0,
    getEntArray: () => context.gscArray(players), isDefined: value => value !== undefined,
    isAlive: entity => entity.alive, println: () => {}, iprintln: text => events.push(['print', text]),
    addTestClient: () => players.length < 64 ? player(true, 'allies') : undefined
  });
  // GSC arrays have .size; build them in the script's realm.
  vm.runInContext('Object.defineProperty(Array.prototype, "size", {get() { return this.length; }});' +
    'var gscArray = list => Array.from(list);', context);
  vm.runInContext(program, context);
  const governor = context.governor();
  assert.equal(governor.next().value, 20, 'the governor waits out map loading first');
  assert.equal(governor.next().value, 1, 'then samples the load once a second');
  return {
    players, events, cvars, bots: () => players.filter(entity => entity.bot).length,
    executeCommands() { for (const entity of queued.splice(0)) drop(entity); },
    // One second of server time with the given load.
    second(late, cpu) { sample.late = late; sample.cpu = cpu; assert.equal(governor.next().value, 1); },
    seconds(count, late, cpu) { for (let i = 0; i < count; i++) this.second(late, cpu); }
  };
}

let room = match();
room.seconds(2, 40, 99);
room.second(0, 50);
room.seconds(2, 40, 99);
assert.equal(room.bots(), 8, 'short spikes do not cost a bot');
room.second(15, 60);
assert.equal(room.bots(), 7, 'three seconds behind schedule remove one bot');
assert.equal(room.cvars.scr_bot_capacity, '7');
assert.deepEqual(room.events.filter(event => event[0] === 'print'),
  [['print', 'Server busy: a bot left to keep the match smooth.']]);
room.seconds(3, 0, 93);
assert.equal(room.bots(), 6, 'three seconds at more than 90% of a core remove another');
room.seconds(3, 0, 80);
assert.equal(room.bots(), 6, 'a busy but on-time server keeps its bots');
room.seconds(56, 0, 40);
assert.equal(room.bots(), 6, 'no bot returns within a minute of the last removal');
room.second(0, 40);
assert.equal(room.bots(), 7, 'a minute later, after at least 20 calm seconds, one bot comes back');
assert.equal(room.cvars.scr_bot_capacity, '7');
assert.deepEqual([room.events.at(-1)[0], room.events.at(-1)[2]], ['join', 2], 'at the room\'s difficulty');
room.seconds(10, 0, 70);
room.second(1, 40);
room.seconds(19, 0, 40);
assert.equal(room.bots(), 7, 'any late frame restarts the calm');
room.second(0, 40);
assert.equal(room.bots(), 8, 'each further bot needs another 20 calm seconds');
room.seconds(200, 0, 10);
assert.equal(room.bots(), 8, 'never more than the room\'s bot count');
room.seconds(3 * 8 + 6, 50, 100);
assert.equal(room.bots(), 0, 'a host that cannot carry bots ends up with none');
assert.equal(room.cvars.scr_bot_capacity, '0');
assert.equal(room.players.length, 2, 'players are never removed');

// Catching up after a long hitch runs several script seconds in one server
// frame, before the queued botkick commands execute: each removal still takes
// a different bot and counts the ones already leaving.
room = match({ bots: 6, humans: 0, target: 6, deferKicks: true });
room.seconds(9, 99, 30);
const removed = room.events.filter(event => event[0] === 'remove').map(event => event[1]);
assert.equal(removed.length, 3);
assert.equal(new Set(removed).size, 3, 'no bot is chosen twice before its kick executes');
assert.equal(room.cvars.scr_bot_capacity, '3', 'bots already leaving are not counted');
room.executeCommands();
assert.equal(room.bots(), 3);

// The bot that leaves is from the larger team and out of play if possible.
room = match({ bots: 4, humans: 1, target: 4 });
const [, , , alliedBot, axisBot] = room.players;
axisBot.alive = false;
room.players[1].pers.team = 'axis';
room.players[2].pers.team = 'axis';
room.seconds(3, 30, 50);
assert.deepEqual(room.events.filter(event => event[0] === 'remove'), [['remove', axisBot.number]]);
assert.ok(room.players.includes(alliedBot));
console.log('PASS: the production bot governor removes one bot per 3 s behind schedule or above 90% CPU, keeps them ' +
  'through spikes and busy on-time play, returns them after a minute and 20 calm seconds up to the room\'s count, ' +
  'records the carried count, never chooses a bot whose kick is still queued, and prefers an idle bot from the ' +
  'larger team');
