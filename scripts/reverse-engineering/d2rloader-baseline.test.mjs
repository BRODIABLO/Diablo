import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import test from 'node:test';
import assert from 'node:assert/strict';
import {
  captureArtifacts,
  createAnnouncement,
  loadRegistry,
  promoteBaseline,
  setGate,
  validateRegistry,
} from './d2rloader-baseline.mjs';

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const registryPath = path.join(repoRoot, 'reverse-engineering', 'd2rloader-baselines.json');
const schemaPath = path.join(repoRoot, 'reverse-engineering', 'd2rloader-baselines.schema.json');
const schema = JSON.parse(fs.readFileSync(schemaPath, 'utf8'));

test('the governed registry is valid and has one promoted baseline', () => {
  const registry = loadRegistry(registryPath, schemaPath);
  assert.equal(registry.promotedBaselineId, 'd2rloader-1.2.1-public');
  assert.equal(
    registry.baselines.filter((baseline) => baseline.stage === 'promoted').length,
    1,
  );
});

test('an announcement remains a candidate and does not replace the promoted baseline', () => {
  const current = loadRegistry(registryPath, schemaPath);
  const next = createAnnouncement(current, {
    version: '9.9.9',
    sourceUrl: 'https://example.test/d2rloader/9.9.9',
    observedAt: '2026-09-04',
  });
  validateRegistry(next, schema);
  assert.equal(next.promotedBaselineId, current.promotedBaselineId);
  assert.equal(next.baselines.at(-1).stage, 'announced');
});

test('promotion fails closed while qualification gates are open', () => {
  const current = loadRegistry(registryPath, schemaPath);
  const next = createAnnouncement(current, {
    version: '9.9.9',
    sourceUrl: 'https://example.test/d2rloader/9.9.9',
    observedAt: '2026-09-04',
  });
  assert.throws(
    () => promoteBaseline(next, 'd2rloader-9.9.9-public'),
    /open gates/,
  );
});

test('capturing artifacts hashes exactly the three governed loader files', async () => {
  const current = loadRegistry(registryPath, schemaPath);
  const candidate = createAnnouncement(current, {
    version: '9.9.9',
    sourceUrl: 'https://example.test/d2rloader/9.9.9',
    observedAt: '2026-09-04',
  });
  const temporaryDirectory = fs.mkdtempSync(path.join(os.tmpdir(), 'd2rloader-baseline-'));
  try {
    for (const name of ['D2RLoader.exe', 'D2RCore.dll', 'd2rloader.mpq']) {
      fs.writeFileSync(path.join(temporaryDirectory, name), `fixture:${name}`, 'utf8');
    }
    const captured = await captureArtifacts(
      candidate,
      'd2rloader-9.9.9-public',
      temporaryDirectory,
    );
    const artifacts = captured.baselines.at(-1).artifacts;
    assert.deepEqual(
      artifacts.map((artifact) => artifact.name),
      ['D2RLoader.exe', 'D2RCore.dll', 'd2rloader.mpq'],
    );
    assert.ok(artifacts.every((artifact) => /^[0-9A-F]{64}$/.test(artifact.sha256)));
  } finally {
    fs.rmSync(temporaryDirectory, { recursive: true, force: true });
  }
});

test('a fully evidenced candidate can replace the promoted pointer without deleting history', () => {
  const current = loadRegistry(registryPath, schemaPath);
  let next = createAnnouncement(current, {
    version: '9.9.9',
    sourceUrl: 'https://example.test/d2rloader/9.9.9',
    observedAt: '2026-09-04',
  });
  const candidate = next.baselines.at(-1);
  const promoted = current.baselines.find(
    (baseline) => baseline.id === current.promotedBaselineId,
  );
  candidate.artifacts = structuredClone(promoted.artifacts);
  candidate.sdk = structuredClone(promoted.sdk);
  candidate.contracts = structuredClone(promoted.contracts);
  for (const gateName of [
    'sourceVerified',
    'artifactIntegrity',
    'sdkAudit',
    'contractAudit',
    'staticCompatibility',
    'runtimeQualification',
    'fullStackCoexistence',
  ]) {
    next = setGate(
      next,
      candidate.id,
      gateName,
      'passed',
      `fixture evidence for ${gateName}`,
    );
  }
  next = promoteBaseline(next, candidate.id, '2026-09-04');
  validateRegistry(next, schema);
  assert.equal(next.promotedBaselineId, candidate.id);
  assert.equal(next.baselines[0].stage, 'superseded');
  assert.equal(next.baselines.length, current.baselines.length + 1);
});
