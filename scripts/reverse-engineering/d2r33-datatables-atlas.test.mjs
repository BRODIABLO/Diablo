import assert from 'node:assert/strict';
import fs from 'node:fs';
import test from 'node:test';
import {
  defaultCatalogPath,
  validateCatalog,
} from './d2r33-datatables-atlas.mjs';

function fixture() {
  return JSON.parse(fs.readFileSync(defaultCatalogPath, 'utf8'));
}

function table(catalog, id) {
  return catalog.tables.find((entry) => entry.id === id);
}

function evidence(catalog, id) {
  return catalog.evidence.find((entry) => entry.id === id);
}

function issueCodes(result) {
  return new Set(result.issues.map((issue) => issue.code));
}

test('accepts the governed A1 catalog', () => {
  const result = validateCatalog(fixture());
  assert.equal(result.valid, true, JSON.stringify(result.issues, null, 2));
  assert.equal(result.summary.tableCount, 7);
  assert.equal(result.summary.provenTriplets, 7);
  assert.deepEqual(result.summary.coveredBuilds, [92777, 93847]);
});

test('rejects a fact that no longer matches its evidence claim', () => {
  const catalog = fixture();
  table(catalog, 'states').count.value = '0x2A0';
  const result = validateCatalog(catalog);
  assert.equal(result.valid, false);
  assert.ok(issueCodes(result).has('EVIDENCE_CLAIM'));
});

test('rejects overlapping DataTables slots even when claims are changed together', () => {
  const catalog = fixture();
  table(catalog, 'skills').records.value = '0x298';
  evidence(catalog, 'skills-layout').claims['skills:records'] = '0x298';
  const result = validateCatalog(catalog);
  assert.equal(result.valid, false);
  assert.ok(issueCodes(result).has('SLOT_OVERLAP'));
});

test('rejects a record field outside its proven stride', () => {
  const catalog = fixture();
  const repair = table(catalog, 'item-types').recordFields.find((field) => field.id === 'repair');
  repair.offset = '0xE8';
  evidence(catalog, 'item-types-repair').claims['item-types:field:repair'].offset = '0xE8';
  const result = validateCatalog(catalog);
  assert.equal(result.valid, false);
  assert.ok(issueCodes(result).has('FIELD_OUTSIDE_RECORD'));
});

test('rejects a proven fact without evidence', () => {
  const catalog = fixture();
  table(catalog, 'items').recordSize.evidenceIds = [];
  const result = validateCatalog(catalog);
  assert.equal(result.valid, false);
  assert.ok(issueCodes(result).has('FACT_EVIDENCE'));
});

test('rejects an unknown fact carrying a concrete value', () => {
  const catalog = fixture();
  table(catalog, 'states').recordSize = {
    status: 'unknown',
    value: '0x44',
    evidenceIds: [],
  };
  const result = validateCatalog(catalog);
  assert.equal(result.valid, false);
  assert.ok(issueCodes(result).has('UNKNOWN_CONCRETE'));
});

test('rejects build generalization without matching byte-exact evidence', () => {
  const catalog = fixture();
  catalog.corpus.coveredBuilds.push({
    build: 99999,
    status: 'byte-exact',
    evidenceIds: ['workbench-manifest'],
  });
  const result = validateCatalog(catalog);
  assert.equal(result.valid, false);
  assert.ok(issueCodes(result).has('BUILD_GENERALIZATION'));
});

test('rejects a stale governed RVA citation', () => {
  const catalog = fixture();
  evidence(catalog, 'states-stride').rva = '0x3083D8';
  const result = validateCatalog(catalog);
  assert.equal(result.valid, false);
  assert.ok(issueCodes(result).has('EVIDENCE_MISMATCH'));
});

test('returns deterministic diagnostics', () => {
  const catalog = fixture();
  table(catalog, 'states').count.value = '0x290';
  const first = validateCatalog(catalog);
  const second = validateCatalog(catalog);
  assert.deepEqual(first, second);
});
