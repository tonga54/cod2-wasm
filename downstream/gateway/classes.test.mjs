import assert from 'node:assert/strict';
import { once } from 'node:events';
import { mkdtempSync, rmSync } from 'node:fs';
import { createRequire } from 'node:module';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { createGateway } from './server.mjs';
import { classRules, createClassStore, defaultClasses, normalizeClasses, playerKey,
  validAttachments } from './classes.mjs';

const { DatabaseSync } = createRequire(import.meta.url)('node:sqlite');

const directory = mkdtempSync(join(tmpdir(), 'cod2-classes-'));
const path = join(directory, 'classes.sqlite');
try {
  // The rules: five classes, one valid weapon per slot, every default playable.
  assert.equal(classRules.classCount, 5);
  assert.equal(classRules.defaults.length, 5);
  assert.deepEqual(normalizeClasses(defaultClasses()), defaultClasses());
  assert.equal(new Set([...classRules.primary, ...classRules.secondary, ...classRules.lethal,
    ...classRules.tactical]).size, 21 + 4 + 6 + 4);
  // Ten camouflages and the guns' own finish, which every default keeps.
  assert.equal(new Set(classRules.camos).size, 11);
  assert.equal(classRules.camos[0], 'none');
  for (const item of classRules.defaults) assert.deepEqual([item.primaryCamo, item.secondaryCamo], ['none', 'none']);
  // The S-Mine and C4 are lethal items beside the frags.
  for (const lethal of ['mine_mp', 'c4_mp']) {
    const placed = defaultClasses().map((item, index) => index === 3 ? { ...item, lethal } : item);
    assert.deepEqual(normalizeClasses(placed)[3], { ...defaultClasses()[3], lethal });
  }

  // A gametag owns its classes, without color codes, case or spaces around it.
  assert.equal(playerKey('^1Chapu^754 '), 'chapu54');
  assert.equal(playerKey('CHAPU54'), 'chapu54');
  for (const name of ['Player', ' player ', 'Unknown Soldier', '', '   ', '^1', 'x;quit', 'a"b', 'a\\b', 'a+b',
                      'tab\tname', 'é'.repeat(16), null, 42, {}])
    assert.equal(playerKey(name), null, String(name));

  const valid = defaultClasses();
  valid[1] = { name: '  Kar Rusher ', primary: 'kar98k_mp', secondary: 'TT30_mp',
    lethal: 'frag_grenade_russian_mp', tactical: 'smoke_grenade_american_mp',
    primaryCamo: 'frogskin', secondaryCamo: 'ember' };
  assert.equal(normalizeClasses(valid)[1].name, 'Kar Rusher');
  assert.deepEqual([normalizeClasses(valid)[1].primaryCamo, normalizeClasses(valid)[1].secondaryCamo],
    ['frogskin', 'ember']);
  // A client built before camouflage sends none: the guns keep their finish.
  const unpainted = defaultClasses().map(({ primaryCamo, secondaryCamo, ...item }) => item);
  assert.deepEqual(normalizeClasses(unpainted), defaultClasses());
  const invalid = [
    [], defaultClasses().slice(0, 4), [...defaultClasses(), defaultClasses()[0]], null, 'x',
    ...[
      { name: '' }, { name: '   ' }, { name: 'x'.repeat(21) }, { name: 'semi;colon' }, { name: 'quo"te' },
      { name: 'back\\slash' }, { name: 'per%cent' }, { name: '^1red' }, { name: 'ñandú' }, { name: 7 },
      { primary: 'colt_mp' }, { primary: 'mg42_bipod_stand_mp' }, { primary: 'kar98k_mp;quit' },
      { secondary: 'kar98k_mp' }, { secondary: 'tt30_mp' }, { lethal: 'smoke_grenade_german_mp' },
      { tactical: 'frag_grenade_german_mp' }, { tactical: 'mine_mp' }, { tactical: 'c4_mp' }, { lethal: 'MINE_MP' },
      { tactical: undefined }, { primaryCamo: 'tiger' }, { primaryCamo: 'Frogskin' }, { secondaryCamo: '' },
      { secondaryCamo: 7 }, { primaryCamo: null }, { secondaryCamo: 'none;quit' }
    ].map(change => defaultClasses().map((item, index) => index === 2 ? { ...item, ...change } : item))
  ];
  for (const value of invalid) assert.throws(() => normalizeClasses(value), /class|classes/, JSON.stringify(value));

  // Attachments: each gun's own list, three on the primary, one on the pistol, one optic.
  for (const weapon of [...classRules.primary, ...classRules.secondary]) {
    assert(classRules.weaponAttachments[weapon]?.length > 0, weapon);
    assert(classRules.weaponAttachments[weapon].every(name => classRules.attachments.includes(name)), weapon);
  }
  assert.deepEqual(Object.keys(classRules.weaponAttachments).sort(),
    [...classRules.primary, ...classRules.secondary].sort());
  assert(classRules.optics.every(name => classRules.attachments.includes(name)));
  assert(validAttachments('thompson_mp', ['suppressor', 'extmag', 'reflex'], 'primary'));
  assert(validAttachments('colt_mp', ['suppressor'], 'secondary'));
  assert(validAttachments('m1garand_mp', [], 'primary'));
  for (const [weapon, list, gun] of [
    ['thompson_mp', ['suppressor', 'extmag', 'reflex', 'grip'], 'primary'],
    ['colt_mp', ['suppressor', 'extmag'], 'secondary'],
    ['m1garand_mp', ['suppressor'], 'primary'],
    ['webley_mp', ['extmag'], 'secondary'],
    ['m1garand_mp', ['reflex', 'optic4x'], 'primary'],
    ['m1garand_mp', ['grip', 'grip'], 'primary'],
    ['m1garand_mp', ['laser'], 'primary'],
    ['m1garand_mp', 'grip', 'primary'],
    ['m1garand_mp', [3], 'primary'],
    ['not_a_gun_mp', ['grip'], 'primary'],
  ]) assert(!validAttachments(weapon, list, gun), JSON.stringify([weapon, list]));
  const fitted = defaultClasses();
  fitted[0] = { ...fitted[0], primary: 'thompson_mp', primaryAttachments: ['suppressor', 'reflex', 'rapidfire'],
    secondary: 'colt_mp', secondaryAttachments: ['suppressor'] };
  assert.deepEqual(normalizeClasses(fitted)[0].primaryAttachments, ['suppressor', 'reflex', 'rapidfire']);
  const older = defaultClasses().map(({ primaryAttachments, secondaryAttachments, ...rest }) => rest);
  assert(normalizeClasses(older).every(item => !item.primaryAttachments.length && !item.secondaryAttachments.length),
    'a client from before attachments sends none');
  for (const change of [{ primaryAttachments: ['suppressor'] }, { secondaryAttachments: ['grip'] },
                        { primaryAttachments: ['grip', 'extmag', 'quickdraw', 'fmj'] },
                        { primaryAttachments: null }, { secondaryAttachments: 'none' }])
    assert.throws(() => normalizeClasses(defaultClasses().map((item, index) => index === 2 ? { ...item, ...change } : item)),
      /attachments for class 3/, JSON.stringify(change));

  // The store keeps all five slots per player and survives reopening.
  let store = createClassStore(path);
  assert.equal(store.get('chapu54'), null);
  store.put('chapu54', normalizeClasses(valid), 1000);
  store.put('other', defaultClasses(), 1000);
  store.close();
  store = createClassStore(path);
  assert.deepEqual(store.get('chapu54'), normalizeClasses(valid));
  assert.deepEqual(store.get('other'), defaultClasses());
  const edited = normalizeClasses(valid);
  edited[4] = { ...edited[4], name: 'Shotty', primary: 'ppsh_mp', secondaryCamo: 'winter',
    primaryAttachments: ['suppressor', 'extmag'] };
  store.put('chapu54', edited, 2000);
  assert.deepEqual(store.get('chapu54'), edited);
  assert.deepEqual(store.get('other'), defaultClasses(), 'one player cannot change another');
  store.close();

  // A store from before attachments gains empty columns and keeps its rows.
  const oldPath = join(directory, 'old.sqlite');
  const old = new DatabaseSync(oldPath);
  old.exec(`CREATE TABLE classes (player TEXT NOT NULL, slot INTEGER NOT NULL, name TEXT NOT NULL,
    primary_weapon TEXT NOT NULL, secondary_weapon TEXT NOT NULL, lethal TEXT NOT NULL, tactical TEXT NOT NULL,
    updated_at INTEGER NOT NULL, PRIMARY KEY (player, slot)) WITHOUT ROWID;
    INSERT INTO classes VALUES ('veteran', 1, 'Old Guard', 'bar_mp', 'colt_mp', 'frag_grenade_american_mp',
      'smoke_grenade_american_mp', 500);`);
  old.close();
  const migrated = createClassStore(oldPath);
  const veteran = migrated.get('veteran');
  assert.deepEqual([veteran[0].name, veteran[0].primary, veteran[0].primaryAttachments, veteran[0].secondaryAttachments],
    ['Old Guard', 'bar_mp', [], []]);
  assert.deepEqual(veteran[1], defaultClasses()[1], 'missing slots take their defaults with their attachments');
  migrated.put('veteran', normalizeClasses(fitted), 600);
  assert.deepEqual(migrated.get('veteran'), normalizeClasses(fitted));
  migrated.close();

  // Stored attachments the gun no longer takes are dropped, the rest of the row kept.
  const raw = new DatabaseSync(path);
  raw.exec(`UPDATE classes SET primary_attachments = 'suppressor', secondary_attachments = 'grip'
    WHERE player = 'chapu54' AND slot = 2`);
  raw.close();
  store = createClassStore(path);
  const dropped = store.get('chapu54');
  assert.deepEqual([dropped[1].name, dropped[1].primary, dropped[1].primaryAttachments, dropped[1].secondaryAttachments],
    ['Kar Rusher', 'kar98k_mp', [], []]);
  store.put('chapu54', edited, 2000);

  // A store from before camouflage gains its columns; its rows keep their
  // weapons, unpainted, and take camouflage from then on.
  {
    const legacyPath = join(directory, 'legacy.sqlite');
    const { DatabaseSync } = createRequire(import.meta.url)('node:sqlite');
    const legacy = new DatabaseSync(legacyPath);
    legacy.exec(`CREATE TABLE classes (player TEXT NOT NULL, slot INTEGER NOT NULL CHECK (slot BETWEEN 1 AND 5),
      name TEXT NOT NULL, primary_weapon TEXT NOT NULL, secondary_weapon TEXT NOT NULL, lethal TEXT NOT NULL,
      tactical TEXT NOT NULL, updated_at INTEGER NOT NULL, PRIMARY KEY (player, slot)) WITHOUT ROWID;
      INSERT INTO classes VALUES ('veteran', 2, 'Old Kar', 'kar98k_mp', 'luger_mp', 'frag_grenade_german_mp',
      'smoke_grenade_german_mp', 500);`);
    legacy.close();
    let migrated = createClassStore(legacyPath);
    const veteran = migrated.get('veteran');
    assert.deepEqual(veteran[1], { name: 'Old Kar', primary: 'kar98k_mp', secondary: 'luger_mp',
      lethal: 'frag_grenade_german_mp', tactical: 'smoke_grenade_german_mp', primaryCamo: 'none',
      secondaryCamo: 'none', primaryAttachments: [], secondaryAttachments: [] });
    assert.deepEqual(veteran[0], defaultClasses()[0]);
    veteran[1] = { ...veteran[1], primaryCamo: 'splinter' };
    migrated.put('veteran', veteran, 600);
    migrated.close();
    migrated = createClassStore(legacyPath);
    assert.equal(migrated.get('veteran')[1].primaryCamo, 'splinter', 'reopening keeps the new columns');
    migrated.close();
  }

  // HTTP: reads by gametag; same-origin, bounded, validated writes.
  const gateway = createGateway({ classStore: store });
  gateway.server.listen(0, '127.0.0.1'); await once(gateway.server, 'listening');
  const origin = `http://127.0.0.1:${gateway.server.address().port}`;
  const put = (body, headers = { Origin: origin }) => fetch(origin + '/classes', { method: 'PUT', headers,
    body: typeof body === 'string' ? body : JSON.stringify(body) });
  try {
    let response = await fetch(origin + '/classes?player=' + encodeURIComponent('^2Chapu54'));
    assert.equal(response.status, 200);
    assert.equal(response.headers.get('cache-control'), 'no-store');
    assert.deepEqual(await response.json(), { classes: edited, saved: true });
    response = await fetch(origin + '/classes?player=Newcomer');
    assert.deepEqual(await response.json(), { classes: defaultClasses(), saved: false });
    for (const query of ['', '?player=', '?player=Player', '?player=a%22b'])
      assert.equal((await fetch(origin + '/classes' + query)).status, 400, query);

    response = await put({ player: 'Newcomer', classes: valid });
    assert.equal(response.status, 200);
    assert.deepEqual(await response.json(), { classes: normalizeClasses(valid), saved: true });
    assert.deepEqual(store.get('newcomer'), normalizeClasses(valid));
    response = await fetch(origin + '/classes?player=Newcomer');
    assert.deepEqual((await response.json()).classes[1], normalizeClasses(valid)[1], 'a read returns the camouflage');
    response = await put({ player: 'Newcomer', classes: fitted });
    assert.deepEqual((await response.json()).classes[0].secondaryAttachments, ['suppressor']);
    response = await fetch(origin + '/classes?player=Newcomer');
    assert.deepEqual((await response.json()).classes, normalizeClasses(fitted));
    const twoOptics = defaultClasses();
    twoOptics[3] = { ...twoOptics[3], primaryAttachments: ['optic4x', 'fmj'] };
    assert.equal((await put({ player: 'Newcomer', classes: twoOptics })).status, 200);
    twoOptics[0] = { ...twoOptics[0], primaryAttachments: ['reflex', 'lens'] };
    assert.equal((await put({ player: 'Newcomer', classes: twoOptics })).status, 400);
    response = await put({ player: 'Newcomer', classes: valid });
    assert.equal(response.status, 200);

    const before = store.get('newcomer');
    assert.equal((await put({ player: 'Newcomer', classes: defaultClasses() }, { Origin: 'http://evil.invalid' })).status, 403);
    assert.equal((await put({ player: 'Newcomer', classes: defaultClasses() }, {})).status, 403);
    assert.equal((await put('{not json', { Origin: origin })).status, 400);
    assert.equal((await put({ player: 'Player', classes: defaultClasses() })).status, 400);
    assert.equal((await put({ player: 'Newcomer', classes: invalid[7] })).status, 400);
    assert.equal((await put({ player: 'Newcomer', classes: defaultClasses(), pad: 'x'.repeat(5000) })).status, 413);
    assert.equal((await fetch(origin + '/classes?player=Newcomer', { method: 'DELETE', headers: { Origin: origin } })).status, 405);
    assert.deepEqual(store.get('newcomer'), before, 'rejected writes change nothing');

    // A host without storage answers instead of failing the page.
    const bare = createGateway({});
    bare.server.listen(0, '127.0.0.1'); await once(bare.server, 'listening');
    const bareOrigin = `http://127.0.0.1:${bare.server.address().port}`;
    try {
      assert.equal((await fetch(bareOrigin + '/classes?player=Newcomer')).status, 503);
      assert.equal((await fetch(bareOrigin + '/classes', { method: 'PUT', headers: { Origin: bareOrigin },
        body: JSON.stringify({ player: 'Newcomer', classes: defaultClasses() }) })).status, 503);
    } finally { await bare.close(); }
  } finally {
    await gateway.close();
    store.close();
  }
  console.log('PASS: classes keyed by gametag, five validated slots, two camouflages and the guns\' attachments, ' +
    'persisted across reopen and migrations from the stores before camouflage and before attachments, ' +
    'same-origin bounded writes');
} finally {
  rmSync(directory, { recursive: true, force: true });
}
