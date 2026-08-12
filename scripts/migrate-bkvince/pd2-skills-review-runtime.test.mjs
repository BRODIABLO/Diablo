import assert from 'node:assert/strict';
import test from 'node:test';

import Ajv2020 from 'ajv/dist/2020.js';
import addFormats from 'ajv-formats';
import fs from 'node:fs';
import path from 'node:path';

import {
  applyBulk,
  buildBrowserRuntimeSource,
  createEmptyEnvelope,
  createEntry,
  entryState,
  exportEnvelope,
  migrateEnvelope,
  legacyStorageKeys,
  progress,
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
import {
  createPolicyEnvelope,
  policyGate,
} from './pd2-skills-schema-policy-runtime.mjs';

const HASH_A = 'A'.repeat(64);
const HASH_B = 'B'.repeat(64);
const HASH_C = 'C'.repeat(64);

function field(id, values, extra = {}) {
  return {
    id,
    table: 'skills.txt',
    header: id,
    label: id,
    values: { vanilla32: values[0], bkvince: values[1], pd2: values[2] },
    displayValues: { vanilla32: values[0], bkvince: values[1], pd2: values[2] },
    changed: values[1] !== values[2],
    protected: false,
    protectionReasons: [],
    proofStatus: 'EXACT_TABLE',
    dependencyIds: [],
    ...extra,
  };
}

function component(id, fields, extra = {}) {
  return {
    id,
    label: id,
    fingerprint: HASH_C,
    proofStatus: 'EXACT_TABLE',
    portability: { categories: ['DATA_ONLY_PROVEN'] },
    changed: fields.some((candidate) => candidate.changed),
    fields,
    ...extra,
  };
}

function skill(stableId, canonicalName, components, extra = {}) {
  return {
    stableId,
    fingerprint: HASH_B,
    canonicalName,
    classCode: 'nec',
    scope: 'nec',
    playerSkill: true,
    newPd2PlayerSkill: false,
    bkvinceOnlyPlayerSkill: false,
    identical: false,
    readOnly: false,
    components,
    ...extra,
  };
}

function fixtureReport() {
  const amplify = skill('skill:nec:amplify-damage', 'Amplify Damage', [
    component('damage_model', [field('power', ['-100', '-100', '-50'])]),
    component('area_targeting', [field('radius', ['4', '5', '7'])]),
    component('cost_timing', [field('mana', ['4', '4', '8'])]),
    component('buffs_debuffs_auras_passives', [field('duration', ['200', '200', '300'])]),
    component('synergies', [field('curse_mastery', ['', '', "skill('Curse Mastery'.blvl)"])]),
  ]);
  const fireBall = skill('skill:sor:fire-ball', 'Fire Ball', [
    component('projectiles_collisions', [field('multishot', ['', '', '(lvl>10?2:1'], {
      protected: true,
      protectionReasons: ['malformedFormula'],
      proofStatus: 'MALFORMED_SOURCE',
    })], { proofStatus: 'MALFORMED_SOURCE' }),
  ], { classCode: 'sor', scope: 'sor' });
  const fireWall = skill('skill:sor:fire-wall', 'Fire Wall', [
    component('cost_timing', [
      field('localdelay', ['25', '20', undefined], { protected: true, protectionReasons: ['delay'] }),
      field('delay', [undefined, undefined, '12'], { protected: true, protectionReasons: ['delay'] }),
    ]),
    component('damage_model', [field('calc4', ['1', '1', '(par1+lvl'], {
      protected: true,
      proofStatus: 'MALFORMED_SOURCE',
      protectionReasons: ['malformedFormula'],
    })], { proofStatus: 'MALFORMED_SOURCE' }),
  ], { classCode: 'sor', scope: 'sor' });
  const coldEnchant = skill('skill:sor:cold-enchant', 'Cold Enchant', [
    component('identity_availability', [field('runtimeOrdinal', ['-', '408', '40'], {
      protected: true,
      protectionReasons: ['runtimeOrdinal', 'ordinalCollision'],
    })]),
  ], { classCode: 'sor', scope: 'sor', ordinals: { bkvince: 408, pd2: 40 }, collisionIds: ['collision:40'] });
  const combustion = skill('skill:sor:combustion', 'Combustion', [
    component('identity_availability', [field('charclass', [undefined, undefined, 'sor'], { protected: true, protectionReasons: ['charclass'] })]),
    component('engine_functions', [field('srvdofunc', [undefined, undefined, '75'], {
      protected: true,
      proofStatus: 'NATIVE_UNPROVEN',
      protectionReasons: ['native callback'],
    })], { proofStatus: 'NATIVE_UNPROVEN' }),
  ], {
    classCode: 'sor',
    scope: 'sor',
    newPd2PlayerSkill: true,
    ordinals: { bkvince: null, pd2: 376 },
    collisionIds: ['collision:376'],
  });
  const raven = skill('skill:dru:raven', 'Raven', [
    component('summons', [field('calc2', ['lvl', 'lvl', 'ulvl + par1 + lvl'], { proofStatus: 'UNSUPPORTED_IDENTIFIER' })], { proofStatus: 'UNSUPPORTED_IDENTIFIER' }),
  ], { classCode: 'dru', scope: 'dru' });
  const hydra = skill('skill:sor:hydra', 'Hydra', [
    component('summons', [field('pettype', ['hydra', 'hydra', 'hydra2'], { dependencyIds: ['pettype:hydra2'] })]),
  ], { classCode: 'sor', scope: 'sor' });
  const warlock = skill('skill:war:bkv-fire-raven', 'BKV Fire Raven', [
    component('identity_availability', [field('warlockExistingRow', ['-', '1', '-'], { protected: true, protectionReasons: ['warlockExistingRow'] })]),
  ], { classCode: 'war', scope: 'war', bkvinceOnlyPlayerSkill: true });
  const identical = skill('skill:ama:critical-strike', 'Critical Strike', [], {
    classCode: 'ama',
    scope: 'ama',
    identical: true,
    readOnly: true,
    fingerprint: HASH_A,
  });
  return {
    schemaVersion: 1,
    reviewId: REVIEW_ID,
    comparisonHash: HASH_A,
    frozenContractHash: FROZEN_CONTRACT_HASH,
    sourceHashes: { vanilla32: HASH_A, bkvince: HASH_B, pd2: HASH_C },
    skills: [amplify, fireBall, fireWall, coldEnchant, combustion, raven, hydra, warlock, identical],
  };
}

function fixtureOrientation() {
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

function approvedSchemaPolicy(orientation) {
  const envelope = createPolicyEnvelope(orientation);
  for (const policy of orientation.policies) {
    envelope.decisions[policy.id] = {
      fingerprint: policy.fingerprint,
      decision: 'APPROVE',
      justification: `Approve ${policy.id} for governed review.`,
    };
  }
  return envelope;
}

function completeKeepEntry(report, canonicalSkill) {
  const entry = createEntry(canonicalSkill);
  entry.globalDecision = 'KEEP_BKVINCE';
  entry.notes.finalJustification = 'Keep the current BKVince behavior after governed review.';
  for (const group of canonicalSkill.components) {
    if (group.changed !== false) entry.componentDecisions[group.id] = { decision: 'KEEP_BKVINCE' };
  }
  entry.implementationStatus = 'DECISION_COMPLETE';
  assert.equal(entryState(report, canonicalSkill, entry).complete, true);
  return entry;
}

test('formal Draft 2020-12 schema accepts a full default governed envelope', () => {
  const report = fixtureReport();
  const envelope = createEmptyEnvelope(report);
  const schemaPath = path.resolve(import.meta.dirname, '..', '..', 'Mission', 'pd2-skills-decisions.schema.json');
  const schema = JSON.parse(fs.readFileSync(schemaPath, 'utf8'));
  const ajv = new Ajv2020({ allErrors: true, strict: true });
  addFormats(ajv);
  const validate = ajv.compile(schema);
  assert.equal(validate(envelope), true, JSON.stringify(validate.errors));
  assert.equal(envelope.entries['skill:ama:critical-strike'], undefined, 'identical skills must not receive mutable entries');
  assert.equal(envelope.entries['skill:sor:combustion'].newSkillLineDecision, null);
  assert.equal(storageKey(report), `pd2-skills-review-decisions-v2:${HASH_A}`);
  assert.deepEqual(legacyStorageKeys(report), [`pd2-skills-review-decisions-v1:${HASH_A}`]);

  const inProgress = structuredClone(envelope);
  inProgress.entries['skill:nec:amplify-damage'].globalDecision = 'ADAPT_PD2_SELECTIVELY';
  inProgress.entries['skill:nec:amplify-damage'].fieldDecisions.mana = {
    decision: 'CUSTOM',
    customValue: '',
    protectedOverride: { approved: false },
  };
  assert.equal(validate(inProgress), true, JSON.stringify(validate.errors));
  const imported = validateImport(report, inProgress);
  assert.equal(imported.valid, true, imported.errors.join('\n'));
  assert(imported.warnings.some((warning) => warning.stableId === 'skill:nec:amplify-damage'));
  assert.doesNotThrow(() => exportEnvelope(report, inProgress.entries, { scope: 'ALL' }));
  inProgress.exportScope = 'COMPLETE_ONLY';
  assert.equal(validate(inProgress), false, 'the same partial CUSTOM is structurally forbidden in COMPLETE_ONLY');
});

test('Amplify Damage supports a complete hybrid through component fallback plus a field override', () => {
  const report = fixtureReport();
  const amplify = report.skills[0];
  const entry = createEntry(amplify);
  entry.globalDecision = 'ADAPT_PD2_SELECTIVELY';
  entry.notes.finalJustification = 'Keep BKVince power while adopting selected PD2 quality-of-life changes.';
  entry.notes.testPlan = 'Compare power, radius, mana, duration and Curse Mastery interaction at levels 1/20/40.';
  for (const group of amplify.components) entry.componentDecisions[group.id] = { decision: 'KEEP_BKVINCE' };
  entry.fieldDecisions.radius = { decision: 'ADOPT_PD2' };
  entry.fieldDecisions.mana = {
    decision: 'CUSTOM',
    customValue: '6',
    justification: 'Split the PD2 cost increase for the first prototype.',
    gameplayObjective: 'Pay for the larger radius without doubling mana.',
    testPlan: 'Measure casts-to-empty at skill levels 1, 20 and 40.',
  };
  const state = entryState(report, amplify, entry);
  assert.equal(state.complete, true, state.reasons.join('\n'));
  assert.equal(resolveFieldChoice(amplify, entry, 'area_targeting', 'radius').decision, 'ADOPT_PD2');
  assert.equal(resolveFieldChoice(amplify, entry, 'damage_model', 'power').decision, 'KEEP_BKVINCE');
});

test('CUSTOM, native proof, malformed source, ordinal collision and delay protections are strict', () => {
  const report = fixtureReport();
  const fireBall = report.skills.find((candidate) => candidate.canonicalName === 'Fire Ball');
  const malformed = fireBall.components[0].fields[0];
  let validation = validateChoice({ decision: 'CUSTOM', customValue: '(lvl>10?2:1)' }, { field: malformed, component: fireBall.components[0] });
  assert.equal(validation.valid, false);
  assert(validation.errors.some((error) => /justification/.test(error)));
  assert(validation.errors.some((error) => /testPlan/.test(error)));
  assert(validation.errors.some((error) => /protected override/.test(error)));

  validation = validateChoice({
    decision: 'CUSTOM',
    customValue: '(lvl>10?2:1)',
    justification: 'Resolve the malformed PD2 source explicitly.',
    testPlan: 'Verify one and two projectile thresholds.',
    protectedOverride: {
      approved: true,
      justification: 'Use a reviewed replacement, never a silent repair.',
      acknowledgedProofStatus: 'MALFORMED_SOURCE',
      malformedResolution: 'Replace with the explicitly reviewed balanced expression.',
    },
  }, { field: malformed, component: fireBall.components[0] });
  assert.equal(validation.valid, true, validation.errors.join('\n'));

  const combustion = report.skills.find((candidate) => candidate.canonicalName === 'Combustion');
  const native = combustion.components[1].fields[0];
  validation = validateChoice({
    decision: 'ADOPT_PD2',
    protectedOverride: {
      approved: true,
      justification: 'Native behavior will be isolated in a proof prototype.',
      acknowledgedProofStatus: 'NATIVE_UNPROVEN',
    },
  }, { field: native, component: combustion.components[1] });
  assert.equal(validation.valid, false);
  assert(validation.errors.some((error) => /nativeRiskAccepted/.test(error)));

  const coldEnchant = report.skills.find((candidate) => candidate.canonicalName === 'Cold Enchant');
  const movedOrdinal = coldEnchant.components[0].fields[0];
  assert.equal(validateChoice({ decision: 'ADOPT_PD2' }, { field: movedOrdinal, component: coldEnchant.components[0] }).valid, false);

  const fireWall = report.skills.find((candidate) => candidate.canonicalName === 'Fire Wall');
  assert.notEqual(fireWall.components[0].fields[0].id, fireWall.components[0].fields[1].id, 'PD2 delay remains separate from BKVince localdelay');
});

test('DISCUSS stays incomplete, identical skills auto-resolve read-only, and implementation is never authorized automatically', () => {
  const report = fixtureReport();
  const raven = report.skills.find((candidate) => candidate.canonicalName === 'Raven');
  const entry = createEntry(raven);
  entry.globalDecision = 'DISCUSS';
  entry.componentDecisions.summons = { decision: 'DISCUSS' };
  const state = entryState(report, raven, entry);
  assert.equal(state.complete, false);
  assert(state.reasons.some((reason) => /DISCUSS/.test(reason)));
  assert.equal(entry.implementationStatus, 'NOT_REVIEWED');

  const identical = report.skills.find((candidate) => candidate.identical);
  assert.deepEqual(entryState(report, identical, undefined), {
    required: false,
    complete: true,
    readOnly: true,
    autoResolved: 'KEEP_BKVINCE',
    reasons: [],
    requirements: { total: 0, complete: 0 },
    components: [],
  });
});

test('new PD2 skills require a coherent line decision before fields', () => {
  const report = fixtureReport();
  const combustion = report.skills.find((candidate) => candidate.canonicalName === 'Combustion');
  const entry = createEntry(combustion);
  entry.globalDecision = 'IMPORT_NEW_PD2_SKILL';
  entry.notes.finalJustification = 'Review the new player skill as an append-only candidate.';
  entry.notes.testPlan = 'Verify dependency closure, ordinal remaps and client/server behavior.';
  for (const group of combustion.components) entry.componentDecisions[group.id] = { decision: 'KEEP_BKVINCE' };
  entry.newSkillLineDecision = 'IMPORT_APPEND_ONLY';
  assert(entryState(report, combustion, entry).reasons.some((reason) => /no BKVince row exists/.test(reason)));
  for (const group of combustion.components) entry.componentDecisions[group.id] = { decision: 'ADOPT_PD2' };
  entry.fieldDecisions.charclass = {
    decision: 'ADOPT_PD2',
    protectedOverride: {
      approved: true,
      justification: 'Explicitly govern the new player class.',
      acknowledgedProofStatus: 'EXACT_TABLE',
    },
  };
  entry.fieldDecisions.srvdofunc = {
    decision: 'ADOPT_PD2',
    protectedOverride: {
      approved: true,
      justification: 'Explicit native proof prototype gate.',
      acknowledgedProofStatus: 'NATIVE_UNPROVEN',
      nativeRiskAccepted: true,
    },
  };
  entry.newSkillLineDecision = null;
  assert.equal(entryState(report, combustion, entry).complete, false);
  entry.newSkillLineDecision = 'IMPORT_APPEND_ONLY';
  assert.equal(entryState(report, combustion, entry).complete, true);
  entry.fieldDecisions.charclass = {
    decision: 'CUSTOM',
    customValue: 'sor',
    justification: 'Explicit player-class target.',
    testPlan: 'Verify tree placement.',
    protectedOverride: {
      approved: true,
      justification: 'Explicitly govern the protected class field.',
      acknowledgedProofStatus: 'EXACT_TABLE',
    },
  };
  assert.equal(entryState(report, combustion, entry).complete, false);
  assert(entryState(report, combustion, entry).reasons.some((reason) => /IMPORT_APPEND_ONLY.*CUSTOM/.test(reason)));
  entry.newSkillLineDecision = 'IMPORT_CUSTOMIZED';
  assert.equal(entryState(report, combustion, entry).complete, true);
});

test('bulk actions fill safely, preserve CUSTOM and notes, protect Warlock/native fields, and require confirmed replacement', () => {
  const report = fixtureReport();
  const amplify = report.skills[0];
  const existing = createEntry(amplify);
  existing.notes.general = 'Vincent note must survive every bulk action.';
  existing.fieldDecisions.mana = {
    decision: 'CUSTOM',
    customValue: '6',
    justification: 'Intentional hybrid.',
    testPlan: 'Mana test.',
  };
  let entries = applyBulk(report, [amplify.stableId], { [amplify.stableId]: existing }, 'ADOPT_PD2');
  assert.equal(entries[amplify.stableId].fieldDecisions.mana.decision, 'CUSTOM');
  assert.equal(entries[amplify.stableId].notes.general, existing.notes.general);
  assert.throws(() => applyBulk(report, [amplify.stableId], entries, 'KEEP_BKVINCE', { replace: true }), /confirmed:true/);
  entries = applyBulk(report, [amplify.stableId], entries, 'KEEP_BKVINCE', { replace: true, confirmed: true });
  assert.equal(entries[amplify.stableId].fieldDecisions.mana, undefined);
  assert.equal(entries[amplify.stableId].notes.general, existing.notes.general);

  const combustion = report.skills.find((candidate) => candidate.canonicalName === 'Combustion');
  combustion.components[0].fields.unshift(field('Id', [undefined, undefined, '376'], {
    protected: true,
    protectionReasons: ['runtimeOrdinal'],
  }));
  combustion.newSkillPlan = {
    proposedTargetOrdinal: 451,
    proposedRow: {
      sourceNodeId: 'pd2:skills.txt:376',
      targetTable: 'skills.txt',
      targetOrdinal: 451,
      targetHeaders: ['*Id', 'skill'],
      values: { '*Id': '451', skill: 'Combustion' },
      mappingProvenance: {
        '*Id': {
          mode: 'APPEND_PREVIEW_DOCUMENTARY_VALUE',
          statement: 'Generated append preview value; no sourceHeader because PD2 Id never allocates the ordinal.',
        },
        skill: { mode: 'EXACT_CANONICAL_HEADER', sourceHeader: 'skill' },
      },
    },
  };
  entries = applyBulk(report, [combustion.stableId], {}, 'ADOPT_PD2');
  assert.equal(entries[combustion.stableId].fieldDecisions.Id.decision, 'NOT_APPLICABLE');
  assert.equal(entries[combustion.stableId].fieldDecisions.srvdofunc.decision, 'KEEP_BKVINCE');
  const warlock = report.skills.find((candidate) => candidate.classCode === 'war');
  entries = applyBulk(report, [warlock.stableId], {}, 'ADOPT_PD2');
  assert.equal(entries[warlock.stableId].fieldDecisions.warlockExistingRow.decision, 'KEEP_BKVINCE');

  entries[warlock.stableId].componentDecisions.identity_availability = { decision: 'DISCUSS' };
  entries = applyBulk(report, [warlock.stableId], entries, 'CLEAR_UNRESOLVED');
  assert.equal(entries[warlock.stableId].componentDecisions.identity_availability, undefined);
});

test('strict import validates hashes, fingerprints, unknown fields, read-only injection and COMPLETE_ONLY completeness', () => {
  const report = fixtureReport();
  const amplify = report.skills[0];
  const valid = exportEnvelope(report, { [amplify.stableId]: completeKeepEntry(report, amplify) }, { scope: 'COMPLETE_ONLY' });
  assert.equal(validateImport(report, valid).valid, true);

  const staleHash = structuredClone(valid);
  staleHash.comparisonHash = HASH_B;
  assert(validateImport(report, staleHash).errors.some((error) => /stale comparison hash/.test(error)));
  const staleFingerprint = structuredClone(valid);
  staleFingerprint.entries[amplify.stableId].fingerprint = HASH_A;
  assert(validateImport(report, staleFingerprint).errors.some((error) => /stale fingerprint/.test(error)));
  const unknown = structuredClone(valid);
  unknown.entries[amplify.stableId].unknown = true;
  assert(validateImport(report, unknown).errors.some((error) => /unknown property/.test(error)));
  const readOnly = structuredClone(valid);
  readOnly.entries['skill:ama:critical-strike'] = { ...completeKeepEntry(report, amplify), fingerprint: HASH_A };
  assert(validateImport(report, readOnly).errors.some((error) => /read-only identical/.test(error)));
  const incomplete = structuredClone(valid);
  incomplete.entries[amplify.stableId].globalDecision = 'DISCUSS';
  assert(validateImport(report, incomplete).errors.some((error) => /COMPLETE_ONLY.*incomplete/.test(error)));
});

test('migration reports retained, stale and dropped entries explicitly by stableId plus fingerprint', () => {
  const oldReport = fixtureReport();
  const amplify = oldReport.skills[0];
  const raven = oldReport.skills.find((candidate) => candidate.canonicalName === 'Raven');
  const oldEntries = {
    [amplify.stableId]: completeKeepEntry(oldReport, amplify),
    [raven.stableId]: completeKeepEntry(oldReport, raven),
  };
  const previous = exportEnvelope(oldReport, oldEntries, { scope: 'ALL' });
  previous.entries['skill:sor:removed-skill'] = {
    ...structuredClone(oldEntries[amplify.stableId]),
    fingerprint: HASH_C,
  };

  const current = fixtureReport();
  current.comparisonHash = HASH_C;
  current.skills.find((candidate) => candidate.stableId === raven.stableId).fingerprint = HASH_A;
  const migration = migrateEnvelope(current, previous);
  const retainedIds = migration.report.retained.map((item) => item.stableId);
  assert(retainedIds.includes(amplify.stableId));
  assert(!retainedIds.includes(raven.stableId));
  assert.equal(
    retainedIds.length,
    current.skills.filter((skill) => !skill.readOnly && skill.stableId !== raven.stableId).length,
    'ALL exports carry explicit default entries that can be retained when their fingerprints still match',
  );
  assert.deepEqual(migration.report.stale.map((item) => item.stableId), [raven.stableId]);
  assert.deepEqual(migration.report.dropped.map((item) => item.stableId), ['skill:sor:removed-skill']);
  assert.equal(migration.envelope.comparisonHash, HASH_C);
  assert.equal(migration.envelope.entries[raven.stableId].globalDecision, null, 'stale current skills restart as explicit default entries');
  assert(Object.keys(migration.envelope.entries).length > migration.report.retained.length, 'the migrated ALL envelope remains complete for the current report');
});

test('progress is global/class-aware and browser-injected runtime matches Node completion', () => {
  const report = fixtureReport();
  report.navigation = [{ id: 'pd2_new', skillIds: ['skill:sor:combustion'] }];
  const amplify = report.skills[0];
  const entries = { [amplify.stableId]: completeKeepEntry(report, amplify) };
  const global = progress(report, entries);
  assert.equal(global.autoResolved, 1);
  assert(global.complete >= 2);
  const nec = progress(report, entries, 'nec');
  assert.equal(nec.total, 1);
  assert.equal(nec.complete, 1);
  const newSkills = progress(report, entries, 'pd2_new');
  assert.equal(newSkills.total, 1);

  const isolated = {};
  new Function('globalThis', buildBrowserRuntimeSource())(isolated);
  assert(isolated.decisionRuntime);
  assert.deepEqual(
    isolated.decisionRuntime.entryState(report, amplify, entries[amplify.stableId]),
    entryState(report, amplify, entries[amplify.stableId]),
  );
});

test('Phase 0 policies gate every mutable skill but never disturb identical read-only auto-resolution', () => {
  const report = fixtureReport();
  report.schemaOrientation = fixtureOrientation();
  const amplify = report.skills[0];
  const complete = completeKeepEntry({ ...report, schemaOrientation: undefined }, amplify);
  const pending = createPolicyEnvelope(report.schemaOrientation);

  let state = entryState(report, amplify, complete, pending);
  assert.equal(state.complete, false);
  assert.equal(state.schemaPolicyGate.open, true);
  assert.equal(state.requirements.remaining, 1, 'the eight global policies project as one atomic skill gate');
  assert(state.reasons.some((reason) => /Phase 0 policy gate/.test(reason)));

  const approved = approvedSchemaPolicy(report.schemaOrientation);
  assert.equal(policyGate(report.schemaOrientation, approved).complete, true);
  state = entryState(report, amplify, complete, approved);
  assert.equal(state.complete, true, state.reasons.join('\n'));
  const governedProgress = progress(report, { entries: { [amplify.stableId]: complete }, schemaPolicy: approved });
  assert.equal(governedProgress.complete >= 2, true);
  assert.equal(governedProgress.schemaPolicyGate.required, 8, 'progress exposes global policies once');
  assert.equal(governedProgress.schemaPolicyGate.closed, 8);

  const identical = report.skills.find((candidate) => candidate.readOnly);
  assert.equal(entryState(report, identical, undefined, pending).complete, true);
  assert.equal(entryState(report, identical, undefined, pending).readOnly, true);
});

test('formal v2 decision schema accepts the autonomous Phase 0 policy envelope', () => {
  const report = fixtureReport();
  report.schemaOrientation = fixtureOrientation();
  const envelope = createEmptyEnvelope(report);
  const schemaPath = path.resolve(import.meta.dirname, '..', '..', 'Mission', 'pd2-skills-decisions.schema.json');
  const schema = JSON.parse(fs.readFileSync(schemaPath, 'utf8'));
  const ajv = new Ajv2020({ allErrors: true, strict: true });
  addFormats(ajv);
  const validate = ajv.compile(schema);
  assert.equal(validate(envelope), true, JSON.stringify(validate.errors));
  const imported = validateImport(report, envelope);
  assert.equal(imported.valid, true, imported.errors.join('\n'));
  assert.equal(imported.warnings.length, report.skills.filter((skill) => !skill.readOnly).length);
});

test('v2 exports carry autonomous schemaPolicy and legacy v1 is migration-only with open fresh policies', () => {
  const report = fixtureReport();
  report.schemaOrientation = fixtureOrientation();
  const amplify = report.skills[0];
  const entry = completeKeepEntry({ ...report, schemaOrientation: undefined }, amplify);
  const approved = approvedSchemaPolicy(report.schemaOrientation);
  const exported = exportEnvelope(report, { [amplify.stableId]: entry }, {
    scope: 'COMPLETE_ONLY',
    schemaPolicy: approved,
  });
  assert.equal(exported.schemaVersion, 2);
  assert.deepEqual(exported.schemaPolicy, approved);
  assert.equal(validateImport(report, exported).valid, true);

  const legacy = structuredClone(exported);
  legacy.schemaVersion = 1;
  legacy.frozenContractHash = '3A0C347476D16366FE1557446E03BD33705AC7AF14CA6BBA4F172935B675A69C';
  delete legacy.schemaPolicy;
  assert.equal(validateImport(report, legacy).valid, false, 'legacy v1 must never be imported directly');
  const migrated = migrateEnvelope(report, legacy);
  assert.equal(migrated.envelope.schemaVersion, 2);
  assert.equal(migrated.report.fromSchemaVersion, 1);
  assert.equal(migrated.report.policyMigration.reason, 'LEGACY_V1_HAS_NO_SCHEMA_POLICY');
  assert(Object.values(migrated.envelope.schemaPolicy.decisions).every((decision) => decision.decision === 'PENDING'));
  assert.equal(entryState(report, amplify, migrated.envelope.entries[amplify.stableId], migrated.envelope.schemaPolicy).complete, false);
});
