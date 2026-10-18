// Custom classes keyed by gametag. There are no accounts: a name owns its
// classes the way a browser profile owns the name.
import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';

// Only the store needs SQLite (Node 22.5+); the rules load in any Node.
const require = createRequire(import.meta.url);

export const classRules = JSON.parse(readFileSync(new URL('./classes.json', import.meta.url), 'utf8'));
const slots = ['primary', 'secondary', 'lethal', 'tactical'];
// Every new browser starts as "Player"; their classes would overwrite each other.
const sharedNames = new Set(['player', 'unknown soldier']);

export class ClassError extends Error {}

export function playerKey(name) {
  if (typeof name !== 'string' || new TextEncoder().encode(name).length > 31 ||
      /[\x00-\x1f\x7f";+\\]/.test(name)) return null;
  const key = name.replace(/\^[0-9]/g, '').trim().toLowerCase();
  return key && !sharedNames.has(key) ? key : null;
}

function validName(name) {
  return typeof name === 'string' && name.trim().length > 0 && name.trim().length <= classRules.nameMaxLength &&
    /^[\x20-\x7e]+$/.test(name) && !/["\\;%^]/.test(name);
}

export function normalizeClasses(value) {
  if (!Array.isArray(value) || value.length !== classRules.classCount)
    throw new ClassError(`Send ${classRules.classCount} classes.`);
  return value.map((item, index) => {
    if (!item || typeof item !== 'object' || !validName(item.name))
      throw new ClassError(`Invalid name for class ${index + 1}.`);
    const normalized = { name: item.name.trim() };
    for (const slot of slots) {
      if (!classRules[slot].includes(item[slot])) throw new ClassError(`Invalid ${slot} for class ${index + 1}.`);
      normalized[slot] = item[slot];
    }
    return normalized;
  });
}

// Rows written before a list changed keep their valid fields; the rest
// come from the slot's default class.
function restore(rows) {
  return classRules.defaults.map((fallback, index) => {
    const row = rows.find(candidate => candidate.slot === index + 1);
    const restored = { name: row && validName(row.name) ? row.name : fallback.name };
    for (const slot of slots) restored[slot] = row && classRules[slot].includes(row[slot]) ? row[slot] : fallback[slot];
    return restored;
  });
}

export function createClassStore(path) {
  const { DatabaseSync } = require('node:sqlite');
  const db = new DatabaseSync(path);
  db.exec(`PRAGMA journal_mode = WAL;
    CREATE TABLE IF NOT EXISTS classes (
      player TEXT NOT NULL,
      slot INTEGER NOT NULL CHECK (slot BETWEEN 1 AND ${classRules.classCount}),
      name TEXT NOT NULL,
      primary_weapon TEXT NOT NULL,
      secondary_weapon TEXT NOT NULL,
      lethal TEXT NOT NULL,
      tactical TEXT NOT NULL,
      updated_at INTEGER NOT NULL,
      PRIMARY KEY (player, slot)
    ) WITHOUT ROWID;`);
  const select = db.prepare(`SELECT slot, name, primary_weapon AS "primary", secondary_weapon AS secondary,
    lethal, tactical FROM classes WHERE player = ? ORDER BY slot`);
  const upsert = db.prepare(`INSERT INTO classes
    (player, slot, name, primary_weapon, secondary_weapon, lethal, tactical, updated_at)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    ON CONFLICT (player, slot) DO UPDATE SET name = excluded.name, primary_weapon = excluded.primary_weapon,
      secondary_weapon = excluded.secondary_weapon, lethal = excluded.lethal, tactical = excluded.tactical,
      updated_at = excluded.updated_at`);
  return {
    get(key) {
      const rows = select.all(key);
      return rows.length ? restore(rows) : null;
    },
    put(key, classes, now = Date.now()) {
      db.exec('BEGIN IMMEDIATE');
      try {
        classes.forEach((item, index) => upsert.run(key, index + 1, item.name, item.primary, item.secondary,
          item.lethal, item.tactical, now));
        db.exec('COMMIT');
      } catch (error) {
        db.exec('ROLLBACK');
        throw error;
      }
    },
    close() { db.close(); }
  };
}

export function defaultClasses() {
  return classRules.defaults.map(item => ({ ...item }));
}
