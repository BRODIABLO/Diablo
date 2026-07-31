import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const { ENCODING, parseTable, serializeTable } = require('../build-data/tsv.js');

const slots = (prefixes, count) => Array.from({ length: count }, (_, index) => {
  const slot = index + 1;
  return prefixes.map((prefix) => `${prefix}${slot}`);
}).flat();

const spacedSlots = (count) => Array.from({ length: count }, (_, index) => {
  const slot = index + 1;
  return [`mod ${slot}`, `mod ${slot} param`, `mod ${slot} min`, `mod ${slot} max`];
}).flat();

const normalizedMagicHeaders = ['name', ...Array.from({ length: 3 }, (_, index) => {
  const slot = index + 1;
  return [`mod${slot}code`, `mod${slot}param`, `mod${slot}min`, `mod${slot}max`];
}).flat()];

const contracts = {
  'itemstatcost.txt': ['stat', 'descpriority'],
  'properties.txt': ['code', '*tooltip', 'func1', 'stat1'],
  'magicsuffix.txt': normalizedMagicHeaders,
  'magicprefix.txt': normalizedMagicHeaders,
  'automagic.txt': normalizedMagicHeaders,
  'qualityitems.txt': Array.from({ length: 2 }, (_, index) => {
    const slot = index + 1;
    return [`mod${slot}code`, `mod${slot}param`, `mod${slot}min`, `mod${slot}max`];
  }).flat(),
  'uniqueitems.txt': ['*id', ...slots(['prop', 'par', 'min', 'max'], 12)],
  'setitems.txt': ['*id', ...slots(['prop', 'par', 'min', 'max'], 9)],
  'armor.txt': ['code', 'type', 'minac', 'maxac'],
  'itemtypes.txt': ['code', 'equiv1', 'equiv2'],
  'weapons.txt': ['code', 'type'],
  'misc.txt': ['code', 'type'],
  'gems.txt': ['code', ...['weapon', 'helm', 'shield'].flatMap((group) =>
    Array.from({ length: 3 }, (_, index) => {
      const slot = index + 1;
      return [`${group}mod${slot}code`, `${group}mod${slot}param`,
        `${group}mod${slot}min`, `${group}mod${slot}max`];
    }).flat())],
  'runes.txt': ['name', 'complete', ...slots(['rune'], 6),
    ...Array.from({ length: 7 }, (_, index) => {
      const slot = index + 1;
      return [`t1code${slot}`, `t1param${slot}`, `t1min${slot}`, `t1max${slot}`];
    }).flat()],
  'cubemain.txt': ['enabled', 'output', 'input 1', ...spacedSlots(5)],
};

const featureDependencies = {
  maxSockets: [],
  baseDefense: ['armor.txt'],
  magicAffixes: ['itemstatcost.txt', 'properties.txt', 'magicsuffix.txt', 'magicprefix.txt'],
  automagic: ['itemstatcost.txt', 'properties.txt', 'automagic.txt'],
  superior: ['itemstatcost.txt', 'properties.txt', 'qualityitems.txt'],
  uniques: ['itemstatcost.txt', 'properties.txt', 'uniqueitems.txt'],
  sets: ['itemstatcost.txt', 'properties.txt', 'setitems.txt'],
  itemCategories: ['itemtypes.txt', 'weapons.txt', 'armor.txt', 'misc.txt'],
  craftedProperties: ['itemstatcost.txt', 'properties.txt', 'cubemain.txt',
    'itemtypes.txt', 'weapons.txt', 'armor.txt', 'misc.txt'],
  runewordIntrinsicProperties: ['itemstatcost.txt', 'properties.txt', 'runes.txt'],
  socketContributionMetadata: ['itemstatcost.txt', 'properties.txt', 'gems.txt',
    'itemtypes.txt', 'weapons.txt', 'armor.txt', 'misc.txt'],
  combinedRunewordRanges: ['itemstatcost.txt', 'properties.txt', 'runes.txt', 'gems.txt',
    'itemtypes.txt', 'weapons.txt', 'armor.txt', 'misc.txt'],
};

function normalizeHeader(value) {
  return value.trim().toLowerCase();
}

function headerStatus(headers, required) {
  const normalized = headers.map(normalizeHeader);
  const duplicates = [...new Set(normalized.filter((header, index) =>
    normalized.indexOf(header) !== index))].sort();
  const available = new Set(normalized);
  const missing = required.filter((header) => !available.has(header)).sort();
  return { ready: duplicates.length === 0 && missing.length === 0, missing, duplicates };
}

function rowObjects(table) {
  const headers = table.headers.map(normalizeHeader);
  return table.rows.map((values) => Object.fromEntries(headers.map((header, index) =>
    [header, values[index] ?? ''])));
}

function duplicateKeys(rows, key, predicate = () => true) {
  const seen = new Set();
  const duplicates = new Set();
  for (const row of rows) {
    if (!predicate(row)) continue;
    const value = (row[key] ?? '').trim();
    if (!value) continue;
    const normalized = value.toLowerCase();
    if (!seen.add(normalized)) duplicates.add(normalized);
  }
  return [...duplicates].sort();
}

function strictInteger(value) {
  const text = String(value ?? '').trim();
  return /^[-+]?\d+$/u.test(text) ? Number.parseInt(text, 10) : null;
}

function semanticStatus(file, rows) {
  const result = { ready: true, issues: [], observations: {} };
  const rejectDuplicates = (key, predicate) => {
    const duplicates = duplicateKeys(rows, key, predicate);
    if (duplicates.length) {
      result.ready = false;
      result.issues.push(`duplicate ${key}: ${duplicates.join(', ')}`);
    }
  };

  if (file === 'itemstatcost.txt') rejectDuplicates('stat');
  if (file === 'properties.txt' || file === 'gems.txt'
      || file === 'armor.txt' || file === 'weapons.txt' || file === 'misc.txt') {
    rejectDuplicates('code');
  }
  if (file === 'itemtypes.txt') rejectDuplicates('code');
  if (['magicprefix.txt', 'magicsuffix.txt', 'automagic.txt'].includes(file)) {
    result.observations.expansionSeparators = rows.filter((row) =>
      (row.name ?? '').trim().toLowerCase() === 'expansion').length;
  }
  if (file === 'uniqueitems.txt' || file === 'setitems.txt') {
    const ids = new Set();
    let blankIds = 0;
    for (const row of rows) {
      const text = (row['*id'] ?? '').trim();
      if (!text) {
        blankIds += 1;
        continue;
      }
      const id = strictInteger(text);
      if (id === null || id < 0) {
        result.ready = false;
        result.issues.push(`invalid *ID: ${text}`);
      } else if (ids.has(id)) {
        result.ready = false;
        result.issues.push(`duplicate *ID: ${id}`);
      } else {
        ids.add(id);
      }
    }
    result.observations.explicitIds = ids.size;
    result.observations.blankSectionRows = blankIds;
  }
  if (file === 'runes.txt') {
    const active = (row) => {
      const complete = strictInteger(row.complete);
      return complete !== null && complete !== 0;
    };
    rejectDuplicates('name', active);
    result.observations.activeRows = rows.filter(active).length;
  }
  return result;
}

function auditRoot(label, root) {
  const tables = {};
  for (const [file, required] of Object.entries(contracts)) {
    const fullPath = path.join(root, file);
    if (!fs.existsSync(fullPath)) {
      tables[file] = { ready: false, exists: false, missing: required, duplicates: [],
        semantic: { ready: false, issues: ['table missing'], observations: {} } };
      continue;
    }
    const table = parseTable(fullPath);
    const raw = fs.readFileSync(fullPath, ENCODING);
    const headers = headerStatus(table.headers, required);
    const semantic = headers.ready
      ? semanticStatus(file, rowObjects(table))
      : { ready: false, issues: ['header contract failed'], observations: {} };
    tables[file] = {
      ready: headers.ready && semantic.ready,
      exists: true,
      rows: table.rows.length,
      headers: table.headers.length,
      eol: table.eol === '\r\n' ? 'CRLF' : 'LF',
      byteExactRoundTrip: serializeTable(table) === raw,
      missing: headers.missing,
      duplicates: headers.duplicates,
      semantic,
    };
  }

  const features = Object.fromEntries(Object.entries(featureDependencies).map(([feature, files]) => {
    const blockers = files.filter((file) => !tables[file]?.ready);
    return [feature, { ready: blockers.length === 0, blockers }];
  }));
  return { label, root: path.resolve(root), tables, features };
}

function selfTest() {
  assert.deepEqual(headerStatus(['Code', 'Type'], ['code', 'type']),
    { ready: true, missing: [], duplicates: [] });
  assert.equal(headerStatus(['TYPE', 'extra', 'CODE'], ['code', 'type']).ready, true);
  assert.deepEqual(headerStatus(['code'], ['code', 'type']).missing, ['type']);
  assert.deepEqual(headerStatus(['Code', ' code '], ['code']).duplicates, ['code']);

  const complete = Object.fromEntries(Object.keys(contracts).map((file) =>
    [file, { ready: true }]));
  const evaluate = (tables) => Object.fromEntries(Object.entries(featureDependencies)
    .map(([feature, files]) => [feature, files.every((file) => tables[file]?.ready)]));
  const withoutGems = structuredClone(complete);
  withoutGems['gems.txt'].ready = false;
  const scoped = evaluate(withoutGems);
  assert.equal(scoped.maxSockets, true);
  assert.equal(scoped.baseDefense, true);
  assert.equal(scoped.magicAffixes, true);
  assert.equal(scoped.runewordIntrinsicProperties, true);
  assert.equal(scoped.socketContributionMetadata, false);
  assert.equal(scoped.combinedRunewordRanges, false);

  const withoutProperties = structuredClone(complete);
  withoutProperties['properties.txt'].ready = false;
  const propertyFailure = evaluate(withoutProperties);
  assert.equal(propertyFailure.maxSockets, true);
  assert.equal(propertyFailure.baseDefense, true);
  assert.equal(propertyFailure.magicAffixes, false);
  assert.equal(propertyFailure.uniques, false);
  return { passed: true, cases: 12 };
}

const requestedRoots = [];
for (let index = 2; index < process.argv.length; index += 1) {
  if (process.argv[index] === '--root') {
    const value = process.argv[++index];
    if (!value) throw new Error('--root requires a path.');
    requestedRoots.push({ label: path.basename(path.resolve(value)), root: value });
  } else if (process.argv[index] !== '--self-test') {
    throw new Error(`Unknown argument: ${process.argv[index]}`);
  }
}

const roots = requestedRoots.length ? requestedRoots : [
  { label: 'BKVince', root: 'data-BKVince/BKVince.mpq/data/global/excel' },
  { label: 'Vanilla 3.2', root: 'data-vanilla3.2/data/data/global/excel' },
];

const report = {
  schemaVersion: 1,
  policy: {
    missingOrInvalidTable: 'disable only dependent annotations',
    reorderedHeaders: 'accepted',
    extraHeaders: 'accepted',
    duplicateNormalizedHeaders: 'reject table',
    duplicateStableKeys: 'reject table',
    rowCounts: 'informational only',
  },
  selfTest: selfTest(),
  roots: roots.map(({ label, root }) => auditRoot(label, root)),
};

console.log(JSON.stringify(report, null, 2));
if (report.roots.some((root) => Object.values(root.tables).some((table) => !table.ready))) {
  process.exitCode = 1;
}
