import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';
import { fileURLToPath } from 'node:url';
import { validateCatalog } from './validate.mjs';

const repoRoot = fileURLToPath(new URL('../../', import.meta.url));
const catalog = JSON.parse(fs.readFileSync(path.join(repoRoot, 'Mission', 'tde-inspiration-bkvince.catalog.json'), 'utf8'));
const schema = JSON.parse(fs.readFileSync(path.join(repoRoot, 'Mission', 'tde-inspiration-bkvince.schema.json'), 'utf8'));

test('accepts the governed catalog', () => {
  assert.deepEqual(validateCatalog(catalog, schema), []);
});

test('rejects a score that bypasses the formula', () => {
  const invalid = structuredClone(catalog);
  invalid.entries[0].scores.priority -= 1;
  assert.match(validateCatalog(invalid, schema).join('\n'), /does not match formula/);
});

test('rejects incomplete README coverage', () => {
  const invalid = structuredClone(catalog);
  invalid.coverage.readmeChunks[1].start += 1;
  assert.match(validateCatalog(invalid, schema).join('\n'), /coverage gap or overlap/);
});

test('rejects a native candidate without a future destination gate', () => {
  const invalid = structuredClone(catalog);
  const native = invalid.entries.find((entry) => entry.lane === 'native');
  native.futureDestinationGateRequired = false;
  assert.match(validateCatalog(invalid, schema).join('\n'), /future plugin destination gate/);
});
