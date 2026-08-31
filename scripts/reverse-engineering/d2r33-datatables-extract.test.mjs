import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import test from 'node:test';
import assert from 'node:assert/strict';

const repositoryRoot = path.resolve(import.meta.dirname, '..', '..');
const inventoryPath = path.join(
  repositoryRoot,
  'reverse-engineering',
  'd2r-3.2.92777',
  'datatables-atlas',
  'candidates.json',
);
const inventory = JSON.parse(fs.readFileSync(inventoryPath, 'utf8'));

function findCandidate(rva) {
  return inventory.candidates.find((entry) => entry.callsiteRva === rva);
}

function runExtractor(...arguments_) {
  const result = spawnSync(
    'powershell',
    [
      '-NoProfile',
      '-ExecutionPolicy',
      'Bypass',
      '-File',
      'scripts/reverse-engineering/d2r33-datatables-extract.ps1',
      ...arguments_,
    ],
    { cwd: repositoryRoot, encoding: 'utf8' },
  );
  assert.equal(result.status, 0, result.stderr || result.stdout);
  return result;
}

test('inventory is candidate-only and covers every direct CompileTxt call', () => {
  assert.equal(inventory.status, 'candidate-only');
  assert.equal(inventory.policy.promotion, 'none');
  assert.equal(inventory.summary.directCallsites, 88);
  assert.equal(inventory.candidates.length, 88);
  assert.ok(inventory.candidates.every((entry) => entry.status === 'candidate'));
});

test('all call bytes and bounded windows match the canonical image', () => {
  assert.equal(inventory.summary.canonicalByteExactCallsites, 88);
  assert.equal(inventory.summary.canonicalByteExactWindows, 88);
  assert.ok(inventory.candidates.every((entry) => entry.call.canonicalByteExact));
  assert.ok(inventory.candidates.every((entry) => entry.call.canonicalWindowByteExact));
});

test('governed States and ItemStatCost callsites recover their literal strides', () => {
  assert.equal(findCandidate('0x3083ED').recordStride.value, '0x44');
  assert.equal(findCandidate('0x31EC89').recordStride.value, '0x144');
});

test('zero-filled source strings remain governed RVA candidates rather than invented names', () => {
  const itemStatCost = findCandidate('0x31EC89');
  assert.equal(itemStatCost.sourceNameCandidate, null);
  assert.equal(itemStatCost.sourceRvaCandidate, '0x1CFCBD0');
  assert.equal(itemStatCost.sourceArguments.length, 3);
  assert.ok(
    itemStatCost.sourceArguments.every(
      (entry) =>
        entry.value === null &&
        entry.readStatus === 'unavailable-in-governed-image' &&
        entry.definition.canonicalByteExact,
    ),
  );
});

test('descriptor heuristics never emit proven fields or DataTables ownership', () => {
  const serialized = JSON.stringify(inventory);
  assert.doesNotMatch(serialized, /"status":"proven"/u);
  assert.doesNotMatch(serialized, /"recordsOffset"|"countOffset"/u);
  for (const candidate of inventory.candidates) {
    for (const cluster of candidate.descriptorClusters) {
      assert.equal(cluster.status, 'candidate');
      assert.match(cluster.association, /^heuristic-/u);
      assert.ok(cluster.entries.every((entry) => entry.status === 'candidate'));
      assert.ok(
        cluster.entries.every((entry) =>
          Object.values(entry.evidence).every((evidence) => evidence.canonicalByteExact),
        ),
      );
    }
  }
});

test('ItemStatCost exposes byte-exact descriptor shapes without claiming readable names', () => {
  const entries = findCandidate('0x31EC89').descriptorClusters.flatMap(
    (cluster) => cluster.entries,
  );
  assert.ok(entries.length > 0);
  assert.ok(entries.every((entry) => entry.nameReadStatus === 'unavailable-in-governed-image'));
  assert.ok(entries.some((entry) => entry.recordOffset === '0x140'));
  assert.ok(inventory.summary.uniqueDescriptorEntries <= inventory.summary.descriptorEntries);
});

test('checked-in inventory is byte-exactly reproducible', () => {
  runExtractor('--check');
});

test('two independent output files are byte-identical', () => {
  const outputRoot = path.join(repositoryRoot, 'analysis-cache', 'd2r33-datatables-atlas-a3');
  const first = path.join(outputRoot, 'generation-a.json');
  const second = path.join(outputRoot, 'generation-b.json');
  runExtractor('--output', first);
  runExtractor('--output', second);
  const firstBytes = fs.readFileSync(first);
  const secondBytes = fs.readFileSync(second);
  assert.deepEqual(firstBytes, secondBytes);
  assert.equal(
    crypto.createHash('sha256').update(firstBytes).digest('hex'),
    crypto.createHash('sha256').update(secondBytes).digest('hex'),
  );
});
