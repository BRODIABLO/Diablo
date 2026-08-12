import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';

import { assertSafeOutputPath, compilePreview, parseCli } from './pd2-skills-decisions-preview.mjs';
import { createEmptyEnvelope, createEntry } from './pd2-skills-review-runtime.mjs';
import { FROZEN_CONTRACT_HASH, REVIEW_ID } from './pd2-skills-review-contracts.mjs';
import {
  FROZEN_ORIENTATION_CONTRACT_HASH,
  GLOBAL_SCHEMA_POLICIES,
  ORIENTATION_ID,
} from './pd2-skills-schema-orientation-contracts.mjs';
import { createPolicyEnvelope } from './pd2-skills-schema-policy-runtime.mjs';

const H_A = 'A'.repeat(64);
const H_B = 'B'.repeat(64);
const H_C = 'C'.repeat(64);

function sha(file) {
  return crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex').toUpperCase();
}

function workspace(t) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'pd2-preview-v3-'));
  t.after(() => fs.rmSync(root, { recursive: true, force: true }));
  for (const directory of ['Mission', 'analysis-cache', 'data-BKVince', 'data-vanilla3.2']) fs.mkdirSync(path.join(root, directory), { recursive: true });
  const sources = {
    vanilla32: path.join(root, 'data-vanilla3.2', 'skills.txt'),
    bkvince: path.join(root, 'data-BKVince', 'skills.txt'),
    pd2: path.join(root, 'Mission', 'PD2-Skills.txt'),
  };
  for (const [source, file] of Object.entries(sources)) fs.writeFileSync(file, `${source}\n`);
  return { root, sources };
}

function field(id, before, after, bundleId, extra = {}) {
  const header = id.replace(/^skills\.txt:/, '');
  return {
    id,
    table: 'skills.txt',
    header,
    values: { vanilla32: before, bkvince: before, pd2: after },
    rawEvidence: {
      vanilla32: { presence: 'VALUE', rawValue: before },
      bkvince: { presence: 'VALUE', rawValue: before },
      pd2: { presence: 'VALUE', rawValue: after },
    },
    changed: before !== after,
    rawChanged: before !== after,
    semanticChanged: before !== after,
    decisionRelevant: true,
    decisionOwnerBundleId: bundleId,
    protected: false,
    protectionReasons: [],
    proofStatus: 'EXACT_TABLE',
    dependencyIds: [],
    sourceLocators: {
      bkvince: { source: 'bkvince', table: 'skills.txt', row: 36, key: 'Fire Bolt', header },
      pd2: { source: 'pd2', table: 'skills.txt', row: 36, key: 'Fire Bolt', header },
    },
    ...extra,
  };
}

function component(id, fields) {
  return { id, label: id, fingerprint: H_C, changed: true, proofStatus: 'EXACT_TABLE', portability: ['DATA_ONLY_PROVEN'], fields };
}

function fireBolt() {
  const damage = [field('skills.txt:emin', '3', '6', 'ELEMENTAL_DAMAGE_CURVE'), field('skills.txt:emax', '6', '10', 'ELEMENTAL_DAMAGE_CURVE')];
  const mana = [field('skills.txt:mana', '3', '4', 'MANA_CURVE')];
  const synergy = [field('skills.txt:edmgsympercalc', 'Fire Ball', 'Fire Ball+Combustion', 'DAMAGE_SYNERGIES')];
  const projectile = [field('skills.txt:vel', '24', '30', 'PROJECTILE_PHYSICS')];
  const item = [field('skills.txt:itemeffect', '1', '2', 'ITEM_TRIGGER_EXECUTION')];
  const native = [field('skills.txt:srvdofunc', '8', '62', 'NATIVE_EXECUTION', { protected: true, proofStatus: 'NATIVE_UNPROVEN', protectionReasons: ['native callback'] })];
  const bundles = [
    ['ELEMENTAL_DAMAGE_CURVE', 'PLAYER', damage],
    ['MANA_CURVE', 'PLAYER', mana],
    ['DAMAGE_SYNERGIES', 'PLAYER', synergy],
    ['PROJECTILE_PHYSICS', 'PLAYER', projectile],
    ['ITEM_TRIGGER_EXECUTION', 'TECHNICAL', item, 'PRESERVE_BKVINCE'],
    ['NATIVE_EXECUTION', 'TECHNICAL', native, 'DEFER_NATIVE_PROOF'],
  ].map(([id, scope, fields, autoResolution]) => ({
    id,
    scope,
    fieldIds: fields.map((candidate) => candidate.id),
    playerLabelFr: id,
    shortHelpFr: id,
    manualDecisionRequired: scope === 'PLAYER',
    autoResolution,
    proofStatus: scope === 'TECHNICAL' ? 'NATIVE_UNPROVEN' : 'EXACT_TABLE',
    fingerprint: H_C,
  }));
  return {
    stableId: 'skill:sor:fire-bolt',
    fingerprint: H_B,
    canonicalName: 'Fire Bolt',
    names: { vanilla32: 'Fire Bolt', bkvince: 'Fire Bolt', pd2: 'Fire Bolt' },
    classCode: 'sor',
    scope: 'sor',
    playerSkill: true,
    newPd2PlayerSkill: false,
    bkvinceOnlyPlayerSkill: false,
    nodeIds: { vanilla32: 'vanilla32:skills.txt:36', bkvince: 'bkvince:skills.txt:36', pd2: 'pd2:skills.txt:36' },
    ordinals: { vanilla32: 36, bkvince: 36, pd2: 36 },
    mappingTypes: ['SAME_SKILL_SAME_ORDINAL'],
    identical: false,
    readOnly: false,
    collisionIds: [],
    dependencies: [],
    consumers: [],
    components: [
      component('damage_model', damage), component('cost_timing', mana), component('synergies', synergy),
      component('projectiles_collisions', projectile), component('interface_localization', item), component('engine_functions', native),
    ],
    decisionBundles: bundles,
  };
}

function reportFixture(t) {
  const { root, sources } = workspace(t);
  const skill = fireBolt();
  const sourceManifest = Object.fromEntries(Object.entries(sources).map(([source, file]) => [source, {
    tables: { 'skills.txt': { path: path.relative(root, file).replaceAll('\\', '/'), sha256: sha(file) } },
  }]));
  const sourceHashes = Object.fromEntries(Object.entries(sources).map(([source, file]) => [source, { 'skills.txt': sha(file) }]));
  const schemaOrientation = {
    schemaVersion: 1,
    orientationId: ORIENTATION_ID,
    orientationHash: H_C,
    frozenContractHash: FROZEN_ORIENTATION_CONTRACT_HASH,
    sourceHashes: structuredClone(sourceHashes),
    policies: GLOBAL_SCHEMA_POLICIES.map((policy, index) => ({ ...policy, fingerprint: String(index + 1).repeat(64).slice(0, 64) })),
  };
  const policy = createPolicyEnvelope(schemaOrientation, { exportedAt: '2026-08-12T00:00:00.000Z' });
  for (const item of schemaOrientation.policies) policy.decisions[item.id] = { fingerprint: item.fingerprint, decision: 'APPROVE', justification: `Approve ${item.id}.` };
  const nodes = [
    { id: 'vanilla32:skills.txt:36', source: 'vanilla32', ordinal: 36, name: 'Fire Bolt', raw: {} },
    { id: 'bkvince:skills.txt:36', source: 'bkvince', ordinal: 36, name: 'Fire Bolt', raw: {} },
    { id: 'pd2:skills.txt:36', source: 'pd2', ordinal: 36, name: 'Fire Bolt', raw: {} },
  ];
  return {
    root,
    report: {
      schemaVersion: 2,
      reviewId: REVIEW_ID,
      comparisonHash: H_A,
      frozenContractHash: FROZEN_CONTRACT_HASH,
      sourceManifest,
      sourceHashes,
      coverage: { nextAppendOrdinal: 37 },
      nodes,
      skills: [skill],
      collisions: [],
      schemaOrientation,
      schemaPolicy: { envelope: policy, canonicalPolicyHash: H_A, approvedExportHash: H_B, provenance: {}, migrationReport: {} },
    },
    skill,
  };
}

function completeEntry(report, skill) {
  const entry = createEntry(skill);
  entry.globalDecision = 'ADAPT_PD2_SELECTIVELY';
  entry.implementationStatus = 'DECISION_COMPLETE';
  entry.notes.finalJustification = 'Governed bundle review complete.';
  entry.notes.testPlan = 'Preview-only six-level comparison.';
  for (const bundle of skill.decisionBundles.filter((item) => item.scope === 'PLAYER')) entry.bundleDecisions[bundle.id] = { decision: 'KEEP_BKVINCE' };
  return entry;
}

function envelope(report, skill, entry, scope = 'COMPLETE_ONLY') {
  const result = createEmptyEnvelope(report);
  result.exportScope = scope;
  result.entries = { [skill.stableId]: entry };
  result.exportedAt = '2026-08-12T01:00:00.000Z';
  return result;
}

test('preview projects atomic player bundles while preserving technical packages', (t) => {
  const { root, report, skill } = reportFixture(t);
  const entry = completeEntry(report, skill);
  entry.bundleDecisions.ELEMENTAL_DAMAGE_CURVE = { decision: 'ADOPT_PD2' };
  entry.bundleDecisions.MANA_CURVE = {
    decision: 'CUSTOM', customValues: { 'skills.txt:mana': '3.5' }, justification: 'Intermediate mana.', testPlan: 'Verify mana display.',
  };
  const preview = compilePreview(report, envelope(report, skill, entry), { repoRoot: root });
  assert.equal(preview.ready, true, JSON.stringify(preview.conflicts));
  assert.equal(preview.adoptedCells.length, 2);
  assert.equal(preview.customCells.length, 1);
  assert(preview.keptCells.some((cell) => cell.fieldId === 'skills.txt:itemeffect'));
  assert(preview.keptCells.some((cell) => cell.fieldId === 'skills.txt:srvdofunc'));
  assert(preview.adoptedCells.every((cell) => cell.bundleId === 'ELEMENTAL_DAMAGE_CURVE'));
});

test('preview rejects incomplete player bundles atomically', (t) => {
  const { root, report, skill } = reportFixture(t);
  const entry = completeEntry(report, skill);
  delete entry.bundleDecisions.PROJECTILE_PHYSICS;
  const preview = compilePreview(report, envelope(report, skill, entry, 'ALL'), { repoRoot: root });
  assert.equal(preview.ready, false);
  assert(preview.incomplete.some((item) => item.code === 'DECISION_INCOMPLETE'));
  assert.deepEqual(preview.adoptedCells, []);
});

test('preview requires the canonical approved Phase 0 policy', (t) => {
  const { root, report, skill } = reportFixture(t);
  const decisions = envelope(report, skill, completeEntry(report, skill), 'ALL');
  decisions.schemaPolicy.decisions.PRESERVE_ALL_D2R_BKVINCE_COLUMNS.decision = 'PENDING';
  decisions.schemaPolicy.decisions.PRESERVE_ALL_D2R_BKVINCE_COLUMNS.justification = '';
  const preview = compilePreview(report, decisions, { repoRoot: root });
  assert.equal(preview.ready, false);
  assert(preview.incomplete.some((item) => item.code === 'PHASE_0_POLICY_GATE_OPEN'));
});

test('stale comparison, source hashes and fingerprints fail before projection', (t) => {
  const { root, report, skill } = reportFixture(t);
  for (const mutate of [
    (value) => { value.comparisonHash = H_B; },
    (value) => { value.sourceHashes.bkvince['skills.txt'] = H_C; },
    (value) => { value.entries[skill.stableId].fingerprint = H_C; },
  ]) {
    const decisions = envelope(report, skill, completeEntry(report, skill));
    mutate(decisions);
    assert.throws(() => compilePreview(report, decisions, { repoRoot: root }), /Governed decision import failed/);
  }
});

test('expert native override stays blocked without the governed proof acknowledgement', (t) => {
  const { root, report, skill } = reportFixture(t);
  const entry = completeEntry(report, skill);
  entry.fieldDecisions['skills.txt:srvdofunc'] = {
    decision: 'ADOPT_PD2', expertOverride: { enabled: true, justification: 'Explicit experiment.' },
  };
  const preview = compilePreview(report, envelope(report, skill, entry, 'ALL'), { repoRoot: root });
  assert.equal(preview.ready, false);
  assert(preview.incomplete.some((item) => item.code === 'INVALID_DECISION' || item.code === 'DECISION_INCOMPLETE'));
  assert.deepEqual(preview.adoptedCells, []);
});

test('raw evidence remains present in projected preview cells', (t) => {
  const { root, report, skill } = reportFixture(t);
  const entry = completeEntry(report, skill);
  entry.bundleDecisions.PROJECTILE_PHYSICS = { decision: 'ADOPT_PD2' };
  const preview = compilePreview(report, envelope(report, skill, entry), { repoRoot: root });
  assert.equal(preview.ready, true, JSON.stringify(preview.conflicts));
  const cell = preview.adoptedCells.find((item) => item.fieldId === 'skills.txt:vel');
  assert.equal(cell.before, '24');
  assert.equal(cell.after, '30');
});

test('CLI and output policy remain strictly read-only', (t) => {
  const { root } = reportFixture(t);
  for (const args of [['decisions.json', '--apply'], ['decisions.json', '--apply=true']]) assert.throws(() => parseCli(args), /forbidden/);
  assert.throws(() => assertSafeOutputPath(path.join(root, 'data-BKVince', 'preview.json'), { repoRoot: root }), /Mission|analysis-cache/);
  assert.doesNotThrow(() => assertSafeOutputPath(path.join(root, 'analysis-cache', 'preview.json'), { repoRoot: root }));
  const source = fs.readFileSync(path.resolve(import.meta.dirname, 'pd2-skills-decisions-preview.mjs'), 'utf8');
  assert.doesNotMatch(source, /function\s+applyGameplay|--apply\s+mode/i);
});
