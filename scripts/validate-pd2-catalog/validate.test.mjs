import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';
import { fileURLToPath } from 'node:url';
import { validateCatalog } from './validate.mjs';

const repoRoot = fileURLToPath(new URL('../../', import.meta.url));
const catalog = JSON.parse(fs.readFileSync(path.join(repoRoot, 'Mission', 'pd2-inspiration-bkvince.catalog.json'), 'utf8'));
const schema = JSON.parse(fs.readFileSync(path.join(repoRoot, 'Mission', 'pd2-inspiration-bkvince.schema.json'), 'utf8'));

test('accepts the governed PD2 catalog', () => {
  assert.deepEqual(validateCatalog(catalog, schema), []);
});

test('rejects duplicate candidate ids', () => {
  const invalid = structuredClone(catalog);
  invalid.entries[1].id = invalid.entries[0].id;
  assert.match(validateCatalog(invalid, schema).join('\n'), /duplicate entry id/);
});

test('rejects unknown table evidence', () => {
  const invalid = structuredClone(catalog);
  invalid.entries[0].tables.push('invented.txt');
  assert.match(validateCatalog(invalid, schema).join('\n'), /unknown table evidence/);
});

test('rejects a data-only candidate that claims reverse engineering is required', () => {
  const invalid = structuredClone(catalog);
  invalid.entries[0].route = 'data_only';
  invalid.entries[0].disposition = 'needs_re';
  assert.match(validateCatalog(invalid, schema).join('\n'), /needs_re requires|data-only entries cannot/);
});

test('enforces the observed BKVince baseline only when explicitly requested', () => {
  const audit = {
    matrix: catalog.coverage.pd2Tables.map((table) => ({ table })),
    summary: {
      pd2ManifestSha256: catalog.source.tableManifestSha256,
      pd2Tables: catalog.source.tableCount,
      bkvinceTables: catalog.coverage.bkvinceTableCount - 1,
      commonPd2Bkvince: catalog.coverage.commonTables,
      exactNormalizedHeaders: catalog.coverage.exactNormalizedHeaders,
      schemaDifferences: catalog.coverage.schemaDifferences,
      pd2Only: Array(catalog.coverage.pd2OnlyCount).fill('table.txt'),
      bkvinceOnly: catalog.coverage.bkvinceOnly,
      pd2AllRoundTripByteExact: true,
      bkvinceAllRoundTripByteExact: true,
      pd2EolKinds: ['LF'],
      bkvinceEolKinds: ['CRLF'],
    },
  };
  assert.deepEqual(validateCatalog(catalog, schema, audit), []);
  assert.match(validateCatalog(catalog, schema, audit, { requireTargetBaseline: true }).join('\n'), /BKVince table count differs/);
});
