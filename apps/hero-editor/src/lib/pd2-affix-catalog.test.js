import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';
import { fileURLToPath } from 'node:url';

import tsv from '../../../../scripts/build-data/tsv.js';
import {
  generatedSource,
  itemCatalog,
} from '../data/bkvince-constants.generated.js';

const { ENCODING, parseTable, serializeTable } = tsv;
const repositoryRoot = path.resolve(
  path.dirname(fileURLToPath(import.meta.url)),
  '..',
  '..',
  '..',
  '..',
);
const excelRoot = path.join(
  repositoryRoot,
  'data-BKVince',
  'BKVince.mpq',
  'data',
  'global',
  'excel',
);

const expectedAffixes = Object.freeze({
  prefixes: Object.freeze({
    table: 'magicprefix.txt',
    legacyLastId: 742,
    legacyEntries: 710,
    legacyIdentitySha256: '507EA4D7A0CF2E8EEC51530DA4DF764E16C2ED9F4C37FFF5BFFB243445FC50A1',
    firstPd2Id: 743,
    lastPd2Id: 849,
    pd2Entries: 107,
    blockedPd2Names: Object.freeze(["Artificer's", 'Virulent']),
    firstPd2: Object.freeze({
      id: 743,
      sourceName: 'Blood Letting',
      row: 745,
      level: 13,
      levelRequirement: 12,
      group: 200,
      allowedTypes: Object.freeze(['weap', 'circ', 'tors', 'helm', 'shld', 'misl', 'glov']),
      mods: Object.freeze([
        Object.freeze({ code: 'heal-kill', parameter: null, minimum: 1, maximum: 2 }),
      ]),
    }),
    lastPd2: Object.freeze({
      id: 849,
      sourceName: 'Deadly',
      row: 851,
      level: 41,
      levelRequirement: 34,
      group: 105,
      allowedTypes: Object.freeze(['helm']),
      mods: Object.freeze([
        Object.freeze({ code: 'dmg%', parameter: null, minimum: 21, maximum: 30 }),
      ]),
    }),
  }),
  suffixes: Object.freeze({
    table: 'magicsuffix.txt',
    legacyLastId: 794,
    legacyEntries: 788,
    legacyIdentitySha256: 'B4EEC78D1AD5DEE58A814816B00D3F74FFA98B17057F9C6C30FFFA51761CEAC1',
    firstPd2Id: 795,
    lastPd2Id: 887,
    pd2Entries: 93,
    blockedPd2Names: Object.freeze(['of Swords', 'of Decay']),
    firstPd2: Object.freeze({
      id: 795,
      sourceName: 'of Defending',
      row: 797,
      level: 45,
      levelRequirement: 20,
      group: 251,
      allowedTypes: Object.freeze(['tors', 'shld']),
      mods: Object.freeze([
        Object.freeze({ code: 'red-dmg%', parameter: null, minimum: 8, maximum: 14 }),
      ]),
    }),
    lastPd2: Object.freeze({
      id: 887,
      sourceName: 'of Protecting',
      row: 889,
      level: 65,
      levelRequirement: 40,
      group: 251,
      allowedTypes: Object.freeze(['helm']),
      mods: Object.freeze([
        Object.freeze({ code: 'red-dmg%', parameter: null, minimum: 10, maximum: 15 }),
      ]),
    }),
  }),
});

function loadAffixTable(fileName) {
  const filePath = path.join(excelRoot, fileName);
  const raw = fs.readFileSync(filePath);
  const table = parseTable(filePath);
  assert.equal(table.eol, '\r\n', `${fileName} must retain governed CRLF line endings`);
  assert.ok(
    Buffer.from(serializeTable(table), ENCODING).equals(raw),
    `${fileName} must round-trip byte-exactly`,
  );
  return table;
}

function affixRowsByAbiId(table) {
  const nameIndex = headerIndex(table, 'Name');
  let id = 1;
  const rows = new Map();
  table.rows.forEach((row, rowIndex) => {
    if (row[nameIndex] === 'Expansion') return;
    rows.set(id, { id, row, rowIndex });
    id += 1;
  });
  return rows;
}

function headerIndex(table, header) {
  const matches = table.headers
    .map((candidate, index) => ({ candidate, index }))
    .filter(({ candidate }) => candidate.toLowerCase() === header.toLowerCase());
  assert.equal(matches.length, 1, `Expected exactly one ${header} column`);
  return matches[0].index;
}

function cell(table, entry, header) {
  return entry.row[headerIndex(table, header)];
}

function governedRow(rows, id, label) {
  const entry = rows.get(id);
  assert.ok(entry, `${label} is missing governed ABI ID ${id}`);
  return entry;
}

function affixIdentitySha256(entries, legacyLastId) {
  const identities = entries
    .filter(({ id }) => id <= legacyLastId)
    .map((entry) => [
      entry.id,
      entry.sourceName,
      entry.group,
      entry.classSpecific,
      entry.classCode,
      entry.allowedTypes,
      entry.excludedTypes,
      entry.mods.map(({ code, parameter }) => [code, parameter]),
    ]);
  return createHash('sha256')
    .update(JSON.stringify(identities))
    .digest('hex')
    .toUpperCase();
}

function range(first, last) {
  return Array.from({ length: last - first + 1 }, (_, index) => first + index);
}

function affixSnapshot(entry) {
  return {
    id: entry.id,
    sourceName: entry.sourceName,
    row: entry.row,
    level: entry.level,
    levelRequirement: entry.levelRequirement,
    group: entry.group,
    allowedTypes: entry.allowedTypes,
    mods: entry.mods,
  };
}

function retuneSnapshot(entry) {
  return {
    sourceName: entry.sourceName,
    level: entry.level,
    levelRequirement: entry.levelRequirement,
    mods: entry.mods,
  };
}

test('keeps every legacy Hero Editor affix ID bound to its pre-merge identity', () => {
  for (const [catalogName, expected] of Object.entries(expectedAffixes)) {
    const entries = itemCatalog[catalogName];
    const legacyEntries = entries.filter(({ id }) => id <= expected.legacyLastId);
    assert.equal(legacyEntries.length, expected.legacyEntries, `${catalogName} legacy entry count`);
    assert.equal(
      affixIdentitySha256(entries, expected.legacyLastId),
      expected.legacyIdentitySha256,
      `${catalogName} legacy ID identity fingerprint`,
    );
  }
});

test('publishes the selected PD2 affixes only after the BKVince legacy ID boundary', () => {
  for (const [catalogName, expected] of Object.entries(expectedAffixes)) {
    const table = loadAffixTable(expected.table);
    const governedRows = affixRowsByAbiId(table);
    assert.equal(
      cell(table, governedRow(governedRows, expected.legacyLastId, catalogName), 'Name'),
      expected.firstPd2Id === 743 ? 'White' : 'of Townportal',
    );
    assert.equal(
      cell(table, governedRow(governedRows, expected.firstPd2Id, catalogName), 'Name'),
      expected.firstPd2.sourceName,
    );
    assert.equal(
      cell(table, governedRow(governedRows, expected.lastPd2Id, catalogName), 'Name'),
      expected.lastPd2.sourceName,
    );
    assert.equal(Math.max(...governedRows.keys()), expected.lastPd2Id);

    const appended = itemCatalog[catalogName].filter(({ id }) => id > expected.legacyLastId);
    assert.equal(appended.length, expected.pd2Entries, `${catalogName} appended entry count`);
    assert.deepEqual(
      appended.map(({ id }) => id),
      range(expected.firstPd2Id, expected.lastPd2Id),
      `${catalogName} appended IDs must be contiguous`,
    );
    assert.deepEqual(affixSnapshot(appended[0]), expected.firstPd2);
    assert.deepEqual(affixSnapshot(appended.at(-1)), expected.lastPd2);
    for (const blockedName of expected.blockedPd2Names) {
      assert.ok(
        !appended.some(({ sourceName }) => sourceName === blockedName),
        `${blockedName} must not be appended as dependency-safe PD2 content`,
      );
    }
  }

  assert.equal(generatedSource.itemCatalog.prefixes, 817);
  assert.equal(generatedSource.itemCatalog.suffixes, 881);
});

test('carries representative PD2 retunes from governed tables into generated affixes', () => {
  const prefixTable = loadAffixTable(expectedAffixes.prefixes.table);
  const prefixRows = affixRowsByAbiId(prefixTable);
  assert.equal(cell(prefixTable, prefixRows.get(143), 'Name'), 'Sturdy');
  assert.equal(cell(prefixTable, prefixRows.get(143), 'frequency'), '14');

  const suffixTable = loadAffixTable(expectedAffixes.suffixes.table);
  const suffixRows = affixRowsByAbiId(suffixTable);
  assert.equal(cell(suffixTable, suffixRows.get(172), 'Name'), 'of Blocking');
  assert.equal(cell(suffixTable, suffixRows.get(172), 'frequency'), '11');
  assert.equal(cell(suffixTable, suffixRows.get(173), 'Name'), 'of Deflecting');
  assert.equal(cell(suffixTable, suffixRows.get(173), 'frequency'), '9');

  assert.deepEqual(
    retuneSnapshot(itemCatalog.prefixes.find(({ id }) => id === 614)),
    {
      sourceName: 'Snowflake',
      level: 27,
      levelRequirement: 20,
      mods: [
        { code: 'cold-len', parameter: null, minimum: 25, maximum: 25 },
        { code: 'cold-min', parameter: null, minimum: 3, maximum: 5 },
        { code: 'cold-max', parameter: null, minimum: 5, maximum: 11 },
      ],
    },
  );
  assert.deepEqual(
    retuneSnapshot(itemCatalog.prefixes.find(({ id }) => id === 647)),
    {
      sourceName: 'Glowing',
      level: 14,
      levelRequirement: 10,
      mods: [
        { code: 'ltng-min', parameter: null, minimum: 1, maximum: 1 },
        { code: 'ltng-max', parameter: null, minimum: 8, maximum: 17 },
      ],
    },
  );
  assert.deepEqual(
    retuneSnapshot(itemCatalog.suffixes.find(({ id }) => id === 49)),
    {
      sourceName: 'of Blight',
      level: 5,
      levelRequirement: 3,
      mods: [
        { code: 'pois-min', parameter: null, minimum: 8, maximum: 8 },
        { code: 'pois-max', parameter: null, minimum: 35, maximum: 35 },
        { code: 'pois-len', parameter: null, minimum: 50, maximum: 50 },
      ],
    },
  );
  assert.deepEqual(
    retuneSnapshot(itemCatalog.suffixes.find(({ id }) => id === 180)),
    {
      sourceName: 'of Frost',
      level: 45,
      levelRequirement: 37,
      mods: [
        { code: 'cold-min', parameter: null, minimum: 1, maximum: 1 },
        { code: 'cold-max', parameter: null, minimum: 3, maximum: 6 },
        { code: 'cold-len', parameter: null, minimum: 50, maximum: 50 },
      ],
    },
  );
  assert.deepEqual(
    retuneSnapshot(itemCatalog.suffixes.find(({ id }) => id === 701)),
    {
      sourceName: 'of Frost',
      level: 14,
      levelRequirement: 10,
      mods: [
        { code: 'cold-min', parameter: null, minimum: 3, maximum: 3 },
        { code: 'cold-max', parameter: null, minimum: 4, maximum: 5 },
        { code: 'cold-len', parameter: null, minimum: 25, maximum: 25 },
      ],
    },
  );
  assert.deepEqual(
    retuneSnapshot(itemCatalog.suffixes.find(({ id }) => id === 754)),
    {
      sourceName: 'of the Vampire',
      level: 76,
      levelRequirement: 64,
      mods: [
        { code: 'manasteal', parameter: null, minimum: 6, maximum: 6 },
      ],
    },
  );
});
