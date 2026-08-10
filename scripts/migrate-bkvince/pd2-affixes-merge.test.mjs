import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';

import {
  assertPinnedOutput,
  canonicalPropertySignature,
  headerIndexes,
  itemTypeReaches,
  parseOrdinalSpec,
  propertyFunctionValues,
  storedValueRange,
  transactionalWriteFiles,
} from './pd2-affixes-merge.mjs';

test('ordinal ranges preserve the explicit deterministic order', () => {
  assert.deepEqual(parseOrdinalSpec('1-3,7,10-11'), [1, 2, 3, 7, 10, 11]);
  assert.throws(() => parseOrdinalSpec('3-1'), /Descending/);
  assert.throws(() => parseOrdinalSpec('1,1'), /Duplicate/);
});

test('headers are resolved case-insensitively without accepting ambiguity', () => {
  const indexes = headerIndexes({ headers: ['Name', 'mod1code'], rows: [] });
  assert.equal(indexes.get('name'), 0);
  assert.equal(indexes.get('mod1code'), 1);
  assert.throws(
    () => headerIndexes({ headers: ['Code', 'code'], rows: [] }),
    /Duplicate case-insensitive header/,
  );
});

test('Property compatibility fingerprint preserves tuple order and raw bytes', () => {
  const headers = [];
  for (let slot = 1; slot <= 7; slot += 1) {
    headers.push(`func${slot}`, `stat${slot}`, `set${slot}`, `val${slot}`);
  }
  const row = headers.map((header) => `${header}-value`);
  const signature = canonicalPropertySignature(row, headerIndexes({ headers, rows: [row] }));
  assert.equal(signature.length, 7);
  assert.deepEqual(signature[0], [
    'func1-value',
    'stat1-value',
    'set1-value',
    'val1-value',
  ]);
  assert.deepEqual(signature[6], [
    'func7-value',
    'stat7-value',
    'set7-value',
    'val7-value',
  ]);
});

test('map exclusion follows transitive ItemTypes equivalence ancestry', () => {
  const itemTypes = new Map([
    ['map', { code: 'map', equiv1: '', equiv2: '' }],
    ['t1m', { code: 't1m', equiv1: 'map', equiv2: '' }],
    ['custom-map', { code: 'custom-map', equiv1: 't1m', equiv2: '' }],
    ['ring', { code: 'ring', equiv1: 'jewelry', equiv2: '' }],
    ['jewelry', { code: 'jewelry', equiv1: '', equiv2: '' }],
  ]);
  assert.equal(itemTypeReaches(itemTypes, 'custom-map', 'map'), true);
  assert.equal(itemTypeReaches(itemTypes, 'ring', 'map'), false);
});

test('ItemStatCost bounds use target Save Bits/Add and property value sources', () => {
  const table = {
    headers: ['Stat', 'Save Bits', 'Save Add', 'Save Param Bits'],
    rows: [['poisonmaxdam', '10', '0', '']],
  };
  const range = storedValueRange(
    { rawKey: 'poisonmaxdam', row: table.rows[0] },
    headerIndexes(table),
  );
  assert.deepEqual(range, { minimum: 0, maximum: 1023, bits: 10, add: 0 });
  assert.deepEqual(propertyFunctionValues('15', 626, 800, 150, null), [626]);
  assert.deepEqual(propertyFunctionValues('16', 626, 800, 150, null), [800]);
  assert.deepEqual(propertyFunctionValues('17', 626, 800, 150, null), [150]);
});

test('apply pins fail closed and cover predicted table, projection and localization hashes', () => {
  const hashes = Object.fromEntries(
    ['selection', 'table', 'identity', 'projection', 'modern', 'legacy']
      .map((name, index) => [name, String(index + 1).repeat(64)]),
  );
  const catalog = {
    targetBaseline: {
      'magicprefix.txt': {
        physicalRows: 10,
        compiledRows: 9,
        nextCompiledId: 10,
        identitySha256: hashes.identity,
      },
    },
    expected: {
      selectionSha256: hashes.selection,
      retunes: {
        'magicprefix.txt': { rows: 1, cells: 2, byColumn: { level: 2 } },
      },
      appends: {
        'magicprefix.txt': {
          rows: 2,
          firstCompiledId: 10,
          projectionSha256: hashes.projection,
        },
      },
      localization: {
        uniqueSelectedKeys: 3,
        newModernKeys: 1,
        newLegacyKeys: 2,
        legacyOnlyGaps: ['existing-modern-key'],
      },
      final: {
        'magicprefix.txt': hashes.table,
        modernLocalization: hashes.modern,
        legacyLocalization: hashes.legacy,
      },
    },
  };
  const summary = {
    tables: {
      'magicprefix.txt': {
        sha256: hashes.table,
        physicalRows: 12,
        compiledRows: 11,
        identitySha256: hashes.identity,
        retuneRows: 1,
        retuneCells: 2,
        retunesByColumn: { level: 2 },
        appendedRows: 2,
        appendedProjectionSha256: hashes.projection,
      },
    },
    localization: {
      uniqueSelectedKeys: 3,
      modernEntries: 1,
      legacyEntries: 2,
      modernSha256: hashes.modern,
      legacySha256: hashes.legacy,
      legacyOnlyGaps: ['existing-modern-key'],
    },
  };

  assert.doesNotThrow(() => assertPinnedOutput(catalog, hashes.selection, summary));
  catalog.expected.selectionSha256 = null;
  assert.throws(
    () => assertPinnedOutput(catalog, hashes.selection, summary),
    /expected a pinned uppercase SHA-256/,
  );
  catalog.expected.selectionSha256 = hashes.selection;
  catalog.expected.appends['magicprefix.txt'].projectionSha256 = null;
  assert.throws(
    () => assertPinnedOutput(catalog, hashes.selection, summary),
    /append projection: expected a pinned uppercase SHA-256/,
  );
});

test('transactional writes restore all original bytes after a later write fails', () => {
  const temporaryRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'pd2-affixes-transaction-'));
  const files = Array.from({ length: 4 }, (_, index) => path.join(temporaryRoot, `${index}.dat`));
  try {
    files.forEach((filePath, index) => fs.writeFileSync(filePath, `original-${index}`));
    assert.throws(
      () => transactionalWriteFiles(files.map((filePath, index) => ({
        filePath,
        write() {
          fs.writeFileSync(filePath, `changed-${index}`);
          if (index === 2) throw new Error('injected third-write failure');
        },
      }))),
      /all original bytes were restored: injected third-write failure/,
    );
    files.forEach((filePath, index) => {
      assert.equal(fs.readFileSync(filePath, 'utf8'), `original-${index}`);
    });
  } finally {
    assert(temporaryRoot.startsWith(path.resolve(os.tmpdir())));
    fs.rmSync(temporaryRoot, { recursive: true, force: true });
  }
});
