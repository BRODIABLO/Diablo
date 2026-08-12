import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';

import Ajv2020 from 'ajv/dist/2020.js';
import addFormats from 'ajv-formats';

import {
  applyBulk,
  buildBrowserRuntimeSource,
  createEmptyEnvelope,
  createEntry,
  entryState,
  exportEnvelope,
  legacyStorageKeys,
  migrateEnvelope,
  progress,
  projectProposedResult,
  resolveBundleChoice,
  resolveFieldChoice,
  storageKey,
  validateChoice,
  validateImport,
} from './pd2-skills-review-runtime.mjs';
import { FROZEN_CONTRACT_HASH, REVIEW_ID } from './pd2-skills-review-contracts.mjs';
import {
  FROZEN_ORIENTATION_CONTRACT_HASH,
  GLOBAL_SCHEMA_POLICIES,
  ORIENTATION_ID,
} from './pd2-skills-schema-orientation-contracts.mjs';
import { createPolicyEnvelope } from './pd2-skills-schema-policy-runtime.mjs';

const HASH_A = 'A'.repeat(64);
const HASH_B = 'B'.repeat(64);
const HASH_C = 'C'.repeat(64);

function orientation() {
  return {
    schemaVersion: 1,
    orientationId: ORIENTATION_ID,
    orientationHash: HASH_C,
    frozenContractHash: FROZEN_ORIENTATION_CONTRACT_HASH,
    sourceHashes: { vanilla32: HASH_A, bkvince: HASH_B, pd2: HASH_C },
    policies: GLOBAL_SCHEMA_POLICIES.map((policy, index) => ({
      ...policy,
      fingerprint: String(index + 1).repeat(64).slice(0, 64),
    })),
  };
}

function approvedPolicy(schemaOrientation) {
  const envelope = createPolicyEnvelope(schemaOrientation, { exportedAt: '2026-08-12T00:00:00.000Z' });
  for (const policy of schemaOrientation.policies) {
    envelope.decisions[policy.id] = {
      fingerprint: policy.fingerprint,
      decision: 'APPROVE',
      justification: `Approved ${policy.id}.`,
    };
  }
  return envelope;
}

function rawField(id, values, bundleId, extra = {}) {
  return {
    id,
    table: 'skills.txt',
    header: id.replace(/^skills\.txt:/, ''),
    label: id,
    values: { vanilla32: values[0], bkvince: values[1], pd2: values[2] },
    rawEvidence: {
      vanilla32: { presence: 'VALUE', value: values[0] },
      bkvince: { presence: 'VALUE', value: values[1] },
      pd2: values[2] === undefined
        ? { presence: 'ABSENT' }
        : { presence: 'VALUE', value: values[2] },
    },
    rawChanged: values[1] !== values[2],
    semanticChanged: values[1] !== values[2],
    decisionRelevant: true,
    decisionOwnerBundleId: bundleId,
    changed: values[1] !== values[2],
    protected: false,
    protectionReasons: [],
    proofStatus: 'EXACT_TABLE',
    dependencyIds: [],
    ...extra,
  };
}

function bundle(id, scope, fieldIds, extra = {}) {
  return {
    id,
    scope,
    fieldIds,
    playerLabelFr: id,
    shortHelpFr: id,
    manualDecisionRequired: scope === 'PLAYER',
    proofStatus: scope === 'TECHNICAL' ? 'NATIVE_UNPROVEN' : 'EXACT_TABLE',
    fingerprint: HASH_C,
    ...extra,
  };
}

function component(id, fields) {
  return {
    id,
    label: id,
    fingerprint: HASH_C,
    proofStatus: fields.some((field) => field.proofStatus === 'NATIVE_UNPROVEN') ? 'NATIVE_UNPROVEN' : 'EXACT_TABLE',
    portability: { categories: ['DATA_ONLY_PROVEN'] },
    changed: fields.some((field) => field.changed),
    fields,
  };
}

function fireBoltSkill() {
  const fields = [
    rawField('skills.txt:emin', ['3', '3', '6'], 'ELEMENTAL_DAMAGE_CURVE'),
    rawField('skills.txt:emax', ['6', '6', '10'], 'ELEMENTAL_DAMAGE_CURVE'),
    rawField('skills.txt:mana', ['3', '3', '4'], 'MANA_CURVE'),
    rawField('skills.txt:edmgsympercalc', ['Fire Ball', 'Fire Ball', 'Fire Ball+Combustion'], 'DAMAGE_SYNERGIES'),
    rawField('missiles.txt:vel', ['24', '24', '30'], 'PROJECTILE_PHYSICS'),
    rawField('skills.txt:itemeffect', ['1', '1', '2'], 'ITEM_TRIGGER_EXECUTION'),
    rawField('skills.txt:srvdofunc', ['8', '8', '62'], 'NATIVE_EXECUTION', {
      protected: true,
      protectionReasons: ['native callback'],
      proofStatus: 'NATIVE_UNPROVEN',
    }),
  ];
  return {
    stableId: 'skill:sor:fire-bolt',
    fingerprint: HASH_B,
    canonicalName: 'Fire Bolt',
    classCode: 'sor',
    scope: 'sor',
    playerSkill: true,
    newPd2PlayerSkill: false,
    identical: false,
    readOnly: false,
    components: [
      component('damage_model', fields.slice(0, 2)),
      component('cost_timing', fields.slice(2, 3)),
      component('synergies', fields.slice(3, 4)),
      component('projectiles_collisions', fields.slice(4, 5)),
      component('interface_localization', fields.slice(5, 6)),
      component('engine_functions', fields.slice(6, 7)),
    ],
    decisionBundles: [
      bundle('ELEMENTAL_DAMAGE_CURVE', 'PLAYER', fields.slice(0, 2).map((field) => field.id)),
      bundle('MANA_CURVE', 'PLAYER', [fields[2].id]),
      bundle('DAMAGE_SYNERGIES', 'PLAYER', [fields[3].id]),
      bundle('PROJECTILE_PHYSICS', 'PLAYER', [fields[4].id]),
      bundle('ITEM_TRIGGER_EXECUTION', 'TECHNICAL', [fields[5].id], { autoResolution: 'PRESERVE_BKVINCE' }),
      bundle('NATIVE_EXECUTION', 'TECHNICAL', [fields[6].id], { autoResolution: 'DEFER_NATIVE_PROOF' }),
    ],
  };
}

function identicalSkill() {
  return {
    stableId: 'skill:ama:critical-strike',
    fingerprint: HASH_A,
    canonicalName: 'Critical Strike',
    classCode: 'ama',
    scope: 'ama',
    playerSkill: true,
    identical: true,
    readOnly: true,
    components: [],
    decisionBundles: [],
  };
}

function fixtureReport() {
  const schemaOrientation = orientation();
  const policy = approvedPolicy(schemaOrientation);
  return {
    schemaVersion: 2,
    reviewId: REVIEW_ID,
    comparisonHash: HASH_A,
    frozenContractHash: FROZEN_CONTRACT_HASH,
    sourceHashes: { vanilla32: HASH_A, bkvince: HASH_B, pd2: HASH_C },
    schemaOrientation,
    schemaPolicy: {
      envelope: policy,
      canonicalPolicyHash: HASH_A,
      approvedExportHash: HASH_B,
      provenance: {},
      migrationReport: {},
    },
    navigation: [{ id: 'sor', skillIds: ['skill:sor:fire-bolt'] }],
    skills: [fireBoltSkill(), identicalSkill()],
  };
}

function completeFireBolt(report, overrides = {}) {
  const skill = report.skills[0];
  const entry = createEntry(skill);
  entry.globalDecision = 'ADAPT_PD2_SELECTIVELY';
  entry.implementationStatus = 'DECISION_COMPLETE';
  entry.notes.finalJustification = 'Governed Fire Bolt behavior bundle review complete.';
  entry.notes.testPlan = 'Compare levels 1, 20 and 40 in the preview only.';
  for (const bundle of skill.decisionBundles.filter((candidate) => candidate.scope === 'PLAYER')) {
    entry.bundleDecisions[bundle.id] = { decision: 'KEEP_BKVINCE' };
  }
  Object.assign(entry, overrides);
  return entry;
}

test('schema v3 accepts the canonical empty envelope and uses hash-bound v3 storage', () => {
  const report = fixtureReport();
  const envelope = createEmptyEnvelope(report);
  const schema = JSON.parse(fs.readFileSync(path.resolve(import.meta.dirname, '..', '..', 'Mission', 'pd2-skills-decisions.schema.json'), 'utf8'));
  const ajv = new Ajv2020({ allErrors: true, strict: true });
  addFormats(ajv);
  const validate = ajv.compile(schema);
  assert.equal(validate(envelope), true, JSON.stringify(validate.errors));
  assert.equal(envelope.schemaVersion, 3);
  assert.deepEqual(envelope.schemaPolicy, report.schemaPolicy.envelope);
  assert.equal(envelope.entries['skill:ama:critical-strike'], undefined);
  assert.equal(storageKey(report), `pd2-skills-review-decisions-v3:${HASH_A}`);
  assert.deepEqual(legacyStorageKeys(report), [
    `pd2-skills-review-decisions-v1:${HASH_A}`,
    `pd2-skills-review-decisions-v2:${HASH_A}`,
  ]);
});

test('Fire Bolt completion counts four player decisions and auto-resolves two technical packages', () => {
  const report = fixtureReport();
  const entry = completeFireBolt(report);
  const state = entryState(report, report.skills[0], entry);
  assert.equal(state.complete, true, state.reasons.join('\n'));
  assert.equal(state.bundles.filter((item) => item.required).length, 4);
  assert.equal(state.bundles.filter((item) => item.scope === 'TECHNICAL').length, 2);
  assert.equal(resolveBundleChoice(report.skills[0], entry, 'ITEM_TRIGGER_EXECUTION').decision, 'KEEP_BKVINCE');
  assert.equal(resolveBundleChoice(report.skills[0], entry, 'NATIVE_EXECUTION').autoResolution, 'DEFER_NATIVE_PROOF');
});

test('pure projection applies player bundles then technical preservation', () => {
  const report = fixtureReport();
  const entry = completeFireBolt(report);
  entry.bundleDecisions.ELEMENTAL_DAMAGE_CURVE = { decision: 'ADOPT_PD2' };
  const projected = projectProposedResult(report, report.skills[0], entry);
  assert.equal(projected.valid, true, projected.errors.join('\n'));
  assert.equal(projected.byField['skills.txt:emin'].after, '6');
  assert.equal(projected.byField['skills.txt:emax'].after, '10');
  assert.equal(projected.byField['skills.txt:itemeffect'].after, '1');
  assert.equal(projected.byField['skills.txt:srvdofunc'].after, '8');
  assert.equal(projected.byField['skills.txt:srvdofunc'].source, 'TECHNICAL_AUTO_RESOLUTION');
});

test('bundle CUSTOM requires and projects a governed customValues map', () => {
  const report = fixtureReport();
  const skill = report.skills[0];
  const entry = completeFireBolt(report);
  entry.bundleDecisions.ELEMENTAL_DAMAGE_CURVE = {
    decision: 'CUSTOM',
    customValues: { 'skills.txt:emin': '5', 'skills.txt:emax': '9' },
    justification: 'Use an intermediate curve.',
    testPlan: 'Compare the six governed levels.',
  };
  const state = entryState(report, skill, entry);
  assert.equal(state.complete, true, state.reasons.join('\n'));
  assert.equal(resolveFieldChoice(skill, entry, 'damage_model', 'skills.txt:emin').customValue, '5');
  assert.equal(projectProposedResult(report, skill, entry).byField['skills.txt:emax'].after, '9');
});

test('raw field decisions require an explicit justified expert override', () => {
  const report = fixtureReport();
  const skill = report.skills[0];
  const entry = completeFireBolt(report);
  entry.fieldDecisions['skills.txt:mana'] = { decision: 'ADOPT_PD2' };
  let state = entryState(report, skill, entry);
  assert.equal(state.complete, false);
  assert(state.reasons.some((reason) => /expertOverride/.test(reason)));
  entry.fieldDecisions['skills.txt:mana'].expertOverride = {
    enabled: true,
    justification: 'Expert override intentionally separates mana from its bundle.',
  };
  state = entryState(report, skill, entry);
  assert.equal(state.complete, true, state.reasons.join('\n'));
  assert.equal(projectProposedResult(report, skill, entry).byField['skills.txt:mana'].source, 'EXPERT_FIELD_OVERRIDE');
});

test('protected native callbacks remain preserved unless the full expert and proof override is explicit', () => {
  const report = fixtureReport();
  const skill = report.skills[0];
  const entry = completeFireBolt(report);
  entry.fieldDecisions['skills.txt:srvdofunc'] = {
    decision: 'ADOPT_PD2',
    expertOverride: { enabled: true, justification: 'Explicit native experiment.' },
  };
  let state = entryState(report, skill, entry);
  assert.equal(state.complete, false);
  assert(state.reasons.some((reason) => /protectedOverride/.test(reason)));
  entry.fieldDecisions['skills.txt:srvdofunc'].protectedOverride = {
    approved: true,
    justification: 'Native behavior was separately proven for this governed preview.',
    acknowledgedProofStatus: 'NATIVE_UNPROVEN',
    nativeRiskAccepted: true,
  };
  state = entryState(report, skill, entry);
  assert.equal(state.complete, true, state.reasons.join('\n'));
  assert.equal(projectProposedResult(report, skill, entry).byField['skills.txt:srvdofunc'].after, '62');
});

test('strict import validates v3 hashes, policy, fingerprints and COMPLETE_ONLY completion', () => {
  const report = fixtureReport();
  const envelope = createEmptyEnvelope(report);
  envelope.entries[report.skills[0].stableId] = completeFireBolt(report);
  envelope.exportScope = 'COMPLETE_ONLY';
  assert.equal(validateImport(report, envelope).valid, true);
  const stale = structuredClone(envelope);
  stale.comparisonHash = HASH_B;
  assert.equal(validateImport(report, stale).valid, false);
  const expertless = structuredClone(envelope);
  expertless.entries[report.skills[0].stableId].fieldDecisions['skills.txt:mana'] = { decision: 'ADOPT_PD2' };
  assert.equal(validateImport(report, expertless).valid, false);
});

test('bulk actions target player bundles only and preserve expert choices unless replacement is confirmed', () => {
  const report = fixtureReport();
  const skill = report.skills[0];
  const original = createEntry(skill);
  original.fieldDecisions['skills.txt:mana'] = {
    decision: 'CUSTOM',
    customValue: '3.5',
    justification: 'Existing expert value.',
    testPlan: 'Existing test.',
    expertOverride: { enabled: true, justification: 'Existing expert override.' },
  };
  let entries = applyBulk(report, [skill.stableId], { [skill.stableId]: original }, 'ADOPT_PD2');
  assert.equal(Object.keys(entries[skill.stableId].bundleDecisions).length, 4);
  assert.equal(entries[skill.stableId].fieldDecisions['skills.txt:mana'].decision, 'CUSTOM');
  assert.throws(() => applyBulk(report, [skill.stableId], entries, 'KEEP_BKVINCE', { replace: true }), /confirmed/);
  entries = applyBulk(report, [skill.stableId], entries, 'KEEP_BKVINCE', { replace: true, confirmed: true });
  assert.deepEqual(entries[skill.stableId].fieldDecisions, {});
});

test('controlled v2 migration never silently turns field decisions into expert overrides', () => {
  const report = fixtureReport();
  const legacy = createEmptyEnvelope(report);
  legacy.schemaVersion = 2;
  legacy.frozenContractHash = HASH_C;
  const raw = legacy.entries[report.skills[0].stableId];
  delete raw.bundleDecisions;
  raw.componentDecisions = { damage_model: { decision: 'ADOPT_PD2' } };
  raw.fieldDecisions = { 'skills.txt:mana': { decision: 'ADOPT_PD2' } };
  const migration = migrateEnvelope(report, legacy, { exportedAt: '2026-08-12T01:00:00.000Z' });
  assert.equal(migration.envelope.schemaVersion, 3);
  assert.equal(migration.report.counts.conflicts, 1);
  assert(migration.report.stale.some((item) => item.reason === 'LEGACY_DECISION_REQUIRES_REVIEW'));
  assert.deepEqual(migration.envelope.entries[report.skills[0].stableId].fieldDecisions, {});
});

test('progress and browser source consume the same bundle runtime', () => {
  const report = fixtureReport();
  const envelope = createEmptyEnvelope(report);
  envelope.entries[report.skills[0].stableId] = completeFireBolt(report);
  const result = progress(report, envelope, 'sor');
  assert.equal(result.total, 1);
  assert.equal(result.complete, 1);
  assert.match(buildBrowserRuntimeSource(), /projectProposedResult/);
  const exported = exportEnvelope(report, envelope, { scope: 'COMPLETE_ONLY', exportedAt: '2026-08-12T02:00:00.000Z' });
  assert.equal(exported.schemaVersion, 3);
  assert.equal(Object.keys(exported.entries).length, 1);
});
