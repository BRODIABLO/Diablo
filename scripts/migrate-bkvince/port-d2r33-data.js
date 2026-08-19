'use strict';

const crypto = require('crypto');
const fs = require('fs');
const path = require('path');

const {
  ENCODING,
  parseTable,
  serializeTable,
  writeTable,
} = require('../build-data/tsv.js');

const ROOT = path.resolve(__dirname, '..', '..');
const APPLY = process.argv.includes('--apply');
const CHECK = process.argv.includes('--check') || !APPLY;

const TARGET_ROOT = path.join(
  ROOT,
  'data-BKVince',
  'BKVince.mpq',
  'data',
  'global',
  'excel',
);
const VANILLA32_ROOT = path.join(ROOT, 'data-vanilla3.2', 'data', 'data', 'global', 'excel');
const VANILLA33_ROOT = path.join(ROOT, 'data-vanilla3.3', 'data', 'data', 'global', 'excel');

const TABLES = Object.freeze({
  cubemain: 'cubemain.txt',
  monstats: 'monstats.txt',
  treasureclassex: 'treasureclassex.txt',
});

const VANILLA33_HASHES = Object.freeze({
  'cubemain.txt': '785AE4A250244068CD1A5625760C03B86EA5DC73C23B08F6020FA2A240AB028D',
  'monstats.txt': '8831FCE35FC549A4849B13A1BD467EB34913F58B8269C0C1BBFA76AEABB6D32E',
  'treasureclassex.txt': '3ABBFC59535E67CF52B0256B3518623EDBA072EB4235E7D1824A7F847ABECA6D',
});

const EXCLUDED_TARGET_HASHES = Object.freeze({
  'skills.txt': '99AF76F35A995731FE7011EC4FA0EC52666EF1CCFD568CFAB7E4C573BAA2593A',
  'uniqueitems.txt': '5C1963B4C7653A1DB1A9EF045F8B308FE30987482EDA83F554CA1E70FBB56CB3',
  'setitems.txt': 'B867A42B65EFCFB001C08D7223D9C18A95F18E0C8679DACB942058433C0E0870',
  'sets.txt': '51A01BBE0060004370D68C19294CDFAC60894CED5F831E7390B0617E7ED264A5',
  'runes.txt': 'AD4189AB28DDADAD7EA67BD93F9BE583E829CA20CC31448230C1DF842E4AD0C9',
  'states.txt': '722B142A2124E14151128937629387D8E6ADC4B752AC943B5620274778393BC8',
});

const SUNDER_OUTPUTS = Object.freeze([
  'Crafted Rotting Fissure',
  'Crafted Cold Rupture',
  'Crafted Crack of the Heavens',
  'Crafted Flame Rift',
  'Crafted Bone Break',
  'Crafted Black Cleft',
]);

const COUNCIL_IDS = Object.freeze([
  'councilmember1',
  'councilmember2',
  'councilmember3',
]);

const ACT_LETTERS = Object.freeze({
  1: ['A', 'B', 'C'],
  2: ['A', 'B', 'C'],
  3: ['A', 'B', 'C'],
  4: ['A', 'B'],
  5: ['A', 'B', 'C'],
});

const HERALD_ROOTS = Object.freeze([
  'Act 1 (H) Herald Item A',
  'Act 1 (H) Herald Item B',
  'Act 1 (H) Herald Item C',
  'Act 2 (H) Herald Item D',
  'Act 2 (H) Herald Item E',
  'Act 2 (H) Herald Item F',
  'Act 3 (H) Herald Item G',
  'Act 3 (H) Herald Item H',
  'Act 3 (H) Herald Item I',
  'Act 4 (H) Herald Item J',
  'Act 4 (H) Herald Item K',
  'Act 5 (H) Herald Item L',
  'Act 5 (H) Herald Item M',
  'Act 5 (H) Herald Item N',
]);

const NEW_TC_GROUPS = Object.freeze([
  Object.freeze({
    anchor: 'Act 5 (H) Wraith C',
    before: 'Act 1 Citem A',
    rows: Object.freeze([
      'Act 1 (H) Citem A Shard',
      'Act 1 Worldstone Shard Parent Citem A',
      'Act 2 Worldstone Shard Parent Citem A',
      'Act 3 Worldstone Shard Parent Citem A',
      'Act 4 Worldstone Shard Parent Citem A',
      'Act 5 Worldstone Shard Parent Citem A',
    ]),
  }),
  Object.freeze({
    anchor: 'Act 5 (H) Champ C',
    before: 'Act 1 Uitem A',
    rows: Object.freeze([
      'Act 1 (H) Uitem A Shard',
      'Act 1 Worldstone Shard Parent Uitem A',
      'Act 2 Worldstone Shard Parent Uitem A',
      'Act 3 Worldstone Shard Parent Uitem A',
      'Act 4 Worldstone Shard Parent Uitem A',
      'Act 5 Worldstone Shard Parent Uitem A',
    ]),
  }),
  Object.freeze({
    anchor: 'Act 5 (H) Good Desecrated Herald',
    before: 'Act 1 (H) Herald Item A',
    rows: Object.freeze(['Act 1 (H) Herald Item Shard']),
  }),
]);

function fail(message) {
  throw new Error(message);
}

function sha256(bytes) {
  return crypto.createHash('sha256').update(bytes).digest('hex').toUpperCase();
}

function assert(condition, message) {
  if (!condition) fail(message);
}

function assertSourceHash(fileName) {
  const filePath = path.join(VANILLA33_ROOT, fileName);
  const actual = sha256(fs.readFileSync(filePath));
  assert(actual === VANILLA33_HASHES[fileName], `${fileName}: unexpected Vanilla 3.3 hash ${actual}`);
}

function assertExcludedTargets() {
  for (const [fileName, expected] of Object.entries(EXCLUDED_TARGET_HASHES)) {
    const filePath = path.join(TARGET_ROOT, fileName);
    const actual = sha256(fs.readFileSync(filePath));
    assert(actual === expected, `${fileName}: excluded BKVince table changed (${actual})`);
  }
  assert(
    !fs.existsSync(path.join(TARGET_ROOT, 'propertygroups.txt')),
    'propertygroups.txt: BKVince must continue inheriting this excluded Vanilla table',
  );
}

function load(filePath, label) {
  const bytes = fs.readFileSync(filePath);
  assert(bytes.length > 0, `${label}: empty file`);
  assert(!bytes.subarray(0, 3).equals(Buffer.from([0xef, 0xbb, 0xbf])), `${label}: UTF-8 BOM found`);
  const table = parseTable(filePath);
  assert(table.eol === '\r\n', `${label}: CRLF required`);
  assert(table.hasFinalEol, `${label}: final CRLF required`);
  assert(Buffer.from(serializeTable(table), ENCODING).equals(bytes), `${label}: byte-exact round-trip failed`);
  return { bytes, filePath, label, table };
}

function headerIndex(table, wanted, label) {
  const normalized = wanted.toLowerCase();
  const matches = table.headers
    .map((header, index) => ({ header, index }))
    .filter(({ header }) => header.toLowerCase() === normalized);
  assert(matches.length === 1, `${label}: expected one header ${wanted}, found ${matches.length}`);
  return matches[0].index;
}

function uniqueRow(table, keyHeader, key, label) {
  const keyIndex = headerIndex(table, keyHeader, label);
  const matches = table.rows
    .map((row, index) => ({ row, index }))
    .filter(({ row }) => (row[keyIndex] || '') === key);
  assert(matches.length === 1, `${label}: expected one ${keyHeader}=${key}, found ${matches.length}`);
  return matches[0];
}

function cell(table, keyHeader, key, column, label) {
  const found = uniqueRow(table, keyHeader, key, label);
  const columnIndex = headerIndex(table, column, label);
  return found.row[columnIndex] || '';
}

const target = Object.fromEntries(Object.entries(TABLES).map(([name, fileName]) => (
  [name, load(path.join(TARGET_ROOT, fileName), `BKVince ${fileName}`)]
)));
const vanilla32 = Object.fromEntries(Object.entries(TABLES).map(([name, fileName]) => (
  [name, load(path.join(VANILLA32_ROOT, fileName), `Vanilla 3.2 ${fileName}`)]
)));
const vanilla33 = Object.fromEntries(Object.entries(TABLES).map(([name, fileName]) => (
  [name, load(path.join(VANILLA33_ROOT, fileName), `Vanilla 3.3 ${fileName}`)]
)));

for (const fileName of Object.values(TABLES)) assertSourceHash(fileName);
assertExcludedTargets();

for (const name of Object.keys(TABLES)) {
  assert(
    JSON.stringify(vanilla32[name].table.headers) === JSON.stringify(vanilla33[name].table.headers),
    `${TABLES[name]}: Vanilla 3.2/3.3 header drift`,
  );
  assert(
    JSON.stringify(target[name].table.headers) === JSON.stringify(vanilla33[name].table.headers),
    `${TABLES[name]}: BKVince/Vanilla 3.3 header drift`,
  );
}

const mutations = [];
const addedRows = [];

function portCell(tableName, keyHeader, key, column) {
  const targetDoc = target[tableName];
  const before = cell(vanilla32[tableName].table, keyHeader, key, column, `Vanilla 3.2 ${TABLES[tableName]}`);
  const after = cell(vanilla33[tableName].table, keyHeader, key, column, `Vanilla 3.3 ${TABLES[tableName]}`);
  assert(before !== after, `${TABLES[tableName]} ${key}.${column}: source versions do not differ`);

  const found = uniqueRow(targetDoc.table, keyHeader, key, `BKVince ${TABLES[tableName]}`);
  const columnIndex = headerIndex(targetDoc.table, column, `BKVince ${TABLES[tableName]}`);
  const current = found.row[columnIndex] || '';
  if (current === after) return;
  if (!APPLY) fail(`${TABLES[tableName]} ${key}.${column}: expected ${JSON.stringify(after)}, found ${JSON.stringify(current)}`);
  assert(
    current === before,
    `${TABLES[tableName]} ${key}.${column}: refusing unexpected value ${JSON.stringify(current)}; expected ${JSON.stringify(before)}`,
  );
  found.row[columnIndex] = after;
  mutations.push({ table: TABLES[tableName], key, column, before, after });
}

for (const output of SUNDER_OUTPUTS) portCell('cubemain', 'output', output, 'lvl');
for (const id of COUNCIL_IDS) portCell('monstats', 'Id', id, 'TreasureClassHerald(H)');

for (const [act, letters] of Object.entries(ACT_LETTERS)) {
  for (const letter of letters) {
    const citem = `Act ${act} (H) Citem ${letter}`;
    for (const column of ['Prob1', 'Prob2', 'Prob3', 'Item4']) {
      portCell('treasureclassex', 'Treasure Class', citem, column);
    }
    const uitem = `Act ${act} (H) Uitem ${letter}`;
    for (const column of ['Prob1', 'Prob2', 'Item3']) {
      portCell('treasureclassex', 'Treasure Class', uitem, column);
    }
  }
}

for (const root of HERALD_ROOTS) {
  portCell('treasureclassex', 'Treasure Class', root, 'Item3');
  for (const suffix of [' - 1 Extra', ' - 2 Extra']) {
    for (const column of ['Unique', 'Set', 'Rare']) {
      portCell('treasureclassex', 'Treasure Class', `${root}${suffix}`, column);
    }
  }
}

function expectedNewRow(key) {
  const source = uniqueRow(
    vanilla33.treasureclassex.table,
    'Treasure Class',
    key,
    'Vanilla 3.3 treasureclassex.txt',
  ).row.slice();
  if (key === 'Act 1 (H) Citem A Shard') {
    const item1 = headerIndex(vanilla33.treasureclassex.table, 'Item1', 'Vanilla 3.3 treasureclassex.txt');
    assert(source[item1] === '"gld,mul=1280"', `${key}: unexpected official Item1 ${source[item1]}`);
    source[item1] = 'gld,mul=1280';
  }
  return source;
}

function rowsEqual(left, right) {
  return left.length === right.length && left.every((value, index) => value === right[index]);
}

function addGroup(group) {
  const table = target.treasureclassex.table;
  const keyIndex = headerIndex(table, 'Treasure Class', 'BKVince treasureclassex.txt');
  const existing = group.rows.map((key) => table.rows.filter((row) => row[keyIndex] === key));
  const existingCount = existing.reduce((sum, matches) => sum + matches.length, 0);
  if (existingCount === 0) {
    if (!APPLY) fail(`treasureclassex.txt: missing new rows ${group.rows.join(', ')}`);
    const anchor = uniqueRow(table, 'Treasure Class', group.anchor, 'BKVince treasureclassex.txt');
    const before = uniqueRow(table, 'Treasure Class', group.before, 'BKVince treasureclassex.txt');
    assert(before.index === anchor.index + 1, `${group.anchor}: expected ${group.before} immediately after anchor`);
    const rows = group.rows.map(expectedNewRow);
    table.rows.splice(anchor.index + 1, 0, ...rows);
    addedRows.push(...group.rows);
  } else {
    assert(existingCount === group.rows.length, `treasureclassex.txt: partial/duplicate new group near ${group.anchor}`);
    for (let index = 0; index < group.rows.length; index += 1) {
      assert(existing[index].length === 1, `treasureclassex.txt: duplicate ${group.rows[index]}`);
      assert(
        rowsEqual(existing[index][0], expectedNewRow(group.rows[index])),
        `treasureclassex.txt: existing new row differs from governed 3.3 row: ${group.rows[index]}`,
      );
    }
  }

  const refreshedAnchor = uniqueRow(table, 'Treasure Class', group.anchor, 'BKVince treasureclassex.txt');
  group.rows.forEach((key, offset) => {
    const row = uniqueRow(table, 'Treasure Class', key, 'BKVince treasureclassex.txt');
    assert(row.index === refreshedAnchor.index + 1 + offset, `${key}: new row order drift`);
  });
  const refreshedBefore = uniqueRow(table, 'Treasure Class', group.before, 'BKVince treasureclassex.txt');
  assert(
    refreshedBefore.index === refreshedAnchor.index + group.rows.length + 1,
    `${group.before}: expected immediately after the new ${group.anchor} group`,
  );
}

for (const group of NEW_TC_GROUPS) addGroup(group);

const tc = target.treasureclassex.table;
assert(
  cell(tc, 'Treasure Class', 'All Acts Terrorize Consumable', 'NoDrop', 'BKVince treasureclassex.txt') === '5',
  'All Acts Terrorize Consumable.NoDrop must preserve the explicit BK value 5',
);
const sunderWeights = [1, 2, 3, 4, 5, 6].map((index) => (
  cell(tc, 'Treasure Class', 'Sunder Charms', `Prob${index}`, 'BKVince treasureclassex.txt')
));
assert(
  JSON.stringify(sunderWeights) === JSON.stringify(['20', '20', '18', '12', '10', '10']),
  `Sunder Charms: BK weights drifted (${sunderWeights.join('/')})`,
);

if (APPLY) {
  assert(mutations.length === 205, `Expected 205 existing-cell mutations, got ${mutations.length}`);
  assert(addedRows.length === 13, `Expected 13 new TC rows, got ${addedRows.length}`);
  for (const doc of Object.values(target)) writeTable(doc.filePath, doc.table);
}

for (const [name, doc] of Object.entries(target)) {
  const serialized = Buffer.from(serializeTable(doc.table), ENCODING);
  if (APPLY) {
    const written = fs.readFileSync(doc.filePath);
    assert(written.equals(serialized), `${TABLES[name]}: write verification failed`);
  } else {
    assert(doc.bytes.equals(serialized), `${TABLES[name]}: check unexpectedly mutated in memory`);
  }
}

assertExcludedTargets();

console.log(JSON.stringify({
  mode: APPLY ? 'apply' : (CHECK ? 'check' : 'unknown'),
  vanilla33: {
    version: '3.3.93847',
    hashes: VANILLA33_HASHES,
  },
  target: {
    mutations: mutations.length,
    addedRows: addedRows.length,
    expectedExistingCells: 205,
    expectedNewRows: 13,
    preserved: {
      allActsTerrorizeConsumableNoDrop: '5',
      sunderCharmsProbabilities: sunderWeights,
    },
  },
  integrity: {
    crlf: true,
    finalEol: true,
    bom: false,
    roundTripByteExact: true,
    excludedTablesHashExact: true,
  },
}, null, 2));
