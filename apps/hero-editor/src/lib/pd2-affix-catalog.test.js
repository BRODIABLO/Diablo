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
    lastName: 'White',
    tableSha256: '9A720A7A551489C19EEDF05FE0AF14317B774FEA66F9F13B418B68B05DA4A232',
  }),
  suffixes: Object.freeze({
    table: 'magicsuffix.txt',
    legacyLastId: 794,
    legacyEntries: 788,
    legacyIdentitySha256: 'B4EEC78D1AD5DEE58A814816B00D3F74FFA98B17057F9C6C30FFFA51761CEAC1',
    lastName: 'of Townportal',
    tableSha256: '71725CF1C0AAD191BB35074474EA1B7E08558851D097B3B7283C72A9BE7B0C97',
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
  return { table, raw };
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

function affixSnapshot(entry) {
  return {
    id: entry.id,
    sourceName: entry.sourceName,
    row: entry.row,
    spawnable: entry.spawnable,
    rare: entry.rare,
    level: entry.level,
    levelRequirement: entry.levelRequirement,
    group: entry.group,
    classSpecific: entry.classSpecific,
    classCode: entry.classCode,
    allowedTypes: entry.allowedTypes,
    excludedTypes: entry.excludedTypes,
    mods: entry.mods,
  };
}

function tableAffixSnapshot(table, entry) {
  const integer = (header) => {
    const value = cell(table, entry, header).trim();
    return value === '' ? null : Number.parseInt(value, 10);
  };
  return {
    id: entry.id,
    sourceName: cell(table, entry, 'Name'),
    row: entry.rowIndex + 2,
    spawnable: cell(table, entry, 'spawnable') === '1',
    rare: cell(table, entry, 'rare') === '1',
    level: integer('level'),
    levelRequirement: integer('levelreq'),
    group: integer('group'),
    classSpecific: cell(table, entry, 'classspecific') === '1',
    classCode: cell(table, entry, 'class').trim() || null,
    allowedTypes: Array.from({ length: 7 }, (_, index) => cell(table, entry, `itype${index + 1}`).trim()).filter(Boolean),
    excludedTypes: Array.from({ length: 5 }, (_, index) => cell(table, entry, `etype${index + 1}`).trim()).filter(Boolean),
    mods: Array.from({ length: 3 }, (_, index) => index + 1).flatMap((slot) => {
      const code = cell(table, entry, `mod${slot}code`).trim();
      if (!code) return [];
      return [{
        code,
        parameter: cell(table, entry, `mod${slot}param`).trim() || null,
        minimum: integer(`mod${slot}min`),
        maximum: integer(`mod${slot}max`),
      }];
    }),
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

test('keeps the runtime affix catalog on the approved BKVince baseline with no appended PD2 IDs', () => {
  for (const [catalogName, expected] of Object.entries(expectedAffixes)) {
    const { table, raw } = loadAffixTable(expected.table);
    const governedRows = affixRowsByAbiId(table);
    assert.equal(
      cell(table, governedRow(governedRows, expected.legacyLastId, catalogName), 'Name'),
      expected.lastName,
    );
    assert.equal(Math.max(...governedRows.keys()), expected.legacyLastId);
    assert.equal(itemCatalog[catalogName].length, expected.legacyEntries);
    assert.equal(itemCatalog[catalogName].some(({ id }) => id > expected.legacyLastId), false);
    assert.equal(
      createHash('sha256').update(raw).digest('hex').toUpperCase(),
      expected.tableSha256,
      `${catalogName} governed baseline hash`,
    );
  }

  assert.equal(generatedSource.itemCatalog.prefixes, 710);
  assert.equal(generatedSource.itemCatalog.suffixes, 788);
});

test('keeps PD2 retunes unapplied until an approved decision manifest exists', () => {
  const { table: prefixTable } = loadAffixTable(expectedAffixes.prefixes.table);
  const prefixRows = affixRowsByAbiId(prefixTable);
  assert.equal(cell(prefixTable, prefixRows.get(143), 'Name'), 'Sturdy');
  assert.equal(cell(prefixTable, prefixRows.get(143), 'frequency'), '9');

  const { table: suffixTable } = loadAffixTable(expectedAffixes.suffixes.table);
  const suffixRows = affixRowsByAbiId(suffixTable);
  assert.equal(cell(suffixTable, suffixRows.get(172), 'Name'), 'of Blocking');
  assert.equal(cell(suffixTable, suffixRows.get(172), 'frequency'), '7');
  assert.equal(cell(suffixTable, suffixRows.get(173), 'Name'), 'of Deflecting');
  assert.equal(cell(suffixTable, suffixRows.get(173), 'frequency'), '6');

  for (const [catalogName, table, rows] of [
    ['prefixes', prefixTable, prefixRows],
    ['suffixes', suffixTable, suffixRows],
  ]) {
    const generated = itemCatalog[catalogName].map(affixSnapshot);
    const governed = [...rows.values()]
      .filter((entry) => cell(table, entry, 'Name') !== '')
      .map((entry) => tableAffixSnapshot(table, entry));
    assert.deepEqual(generated, governed, `${catalogName} runtime catalog must mirror BKVince TXT cells`);
  }
});

test('keeps PD2 review and preview artifacts outside the runtime affix catalog', () => {
  const report = JSON.parse(fs.readFileSync(path.join(repositoryRoot, 'Mission', 'pd2-affixes-review.json'), 'utf8'));
  const catalog = JSON.parse(fs.readFileSync(path.join(repositoryRoot, 'Mission', 'pd2-affixes-merge.catalog.json'), 'utf8'));

  assert.equal(report.schemaVersion, 3);
  assert.equal(report.state, 'review_only_no_import_approved');
  assert.equal(catalog.status, 'selection_review_no_import_approved');
  assert.equal(catalog.review.previewOnly, true);
  for (const expected of Object.values(expectedAffixes)) {
    assert.equal(report.targetBaselineHashes[expected.table], expected.tableSha256);
  }
  assert.equal(itemCatalog.prefixes.some(({ id }) => id > expectedAffixes.prefixes.legacyLastId), false);
  assert.equal(itemCatalog.suffixes.some(({ id }) => id > expectedAffixes.suffixes.legacyLastId), false);
});
