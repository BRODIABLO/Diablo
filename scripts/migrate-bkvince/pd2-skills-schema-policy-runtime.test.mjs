import assert from 'node:assert/strict';
import test from 'node:test';

import {
  buildBrowserPolicyRuntimeSource,
  createPolicyEnvelope,
  exportPolicyEnvelope,
  migratePolicyEnvelope,
  policyGate,
  policyStorageKey,
  validatePolicyEnvelope,
} from './pd2-skills-schema-policy-runtime.mjs';
import {
  FROZEN_ORIENTATION_CONTRACT_HASH,
  GLOBAL_SCHEMA_POLICIES,
  ORIENTATION_ID,
} from './pd2-skills-schema-orientation-contracts.mjs';

const HASH_A = 'A'.repeat(64);
const HASH_B = 'B'.repeat(64);
const HASH_C = 'C'.repeat(64);
const EXPORTED_AT = '2026-08-12T16:00:00.000Z';

function orientation(overrides = {}) {
  return {
    schemaVersion: 1,
    orientationId: ORIENTATION_ID,
    orientationHash: HASH_A,
    frozenContractHash: FROZEN_ORIENTATION_CONTRACT_HASH,
    sourceHashes: { vanilla32: HASH_A, bkvince: HASH_B, pd2: HASH_C },
    policies: GLOBAL_SCHEMA_POLICIES.map((policy, index) => ({
      ...policy,
      fingerprint: String(index + 1).repeat(64).slice(0, 64),
    })),
    ...overrides,
  };
}

function approveAll(governed) {
  const envelope = createPolicyEnvelope(governed, { exportedAt: EXPORTED_AT });
  for (const policy of governed.policies) {
    envelope.decisions[policy.id] = {
      fingerprint: policy.fingerprint,
      decision: 'APPROVE',
      justification: `Approve governed policy ${policy.id}.`,
    };
  }
  return envelope;
}

test('default policy envelope is hash-bound, PENDING, deterministic with an explicit timestamp and stored by orientation hash', () => {
  const governed = orientation();
  const first = createPolicyEnvelope(governed, { exportedAt: EXPORTED_AT });
  const second = createPolicyEnvelope(governed, { exportedAt: EXPORTED_AT });
  assert.deepEqual(first, second);
  assert.equal(first.orientationId, ORIENTATION_ID);
  assert.equal(first.frozenContractHash, FROZEN_ORIENTATION_CONTRACT_HASH);
  assert.equal(Object.keys(first.decisions).length, GLOBAL_SCHEMA_POLICIES.length);
  assert(Object.values(first.decisions).every((entry) => entry.decision === 'PENDING'));
  assert(Object.values(first.decisions).every((entry) => /^[A-Fa-f0-9]{64}$/.test(entry.fingerprint)));
  assert.equal(policyStorageKey(governed), `pd2-skills-schema-policy-v1:${HASH_A}`);
  assert.equal(validatePolicyEnvelope(governed, first).valid, true);
  assert.equal(policyGate(governed, first).complete, false);
});

test('only justified APPROVE or justified explicit MODIFY closes a required policy', () => {
  const governed = orientation();
  const envelope = approveAll(governed);
  assert.equal(policyGate(governed, envelope).complete, true);

  const modifiedId = governed.policies[0].id;
  envelope.decisions[modifiedId] = {
    fingerprint: governed.policies[0].fingerprint,
    decision: 'MODIFY',
    justification: 'Use a narrower governed exception.',
    customPolicy: 'Preserve every D2R/BKVince header, with documentary aliases displayed separately.',
  };
  assert.equal(policyGate(governed, envelope).complete, true);

  envelope.decisions[modifiedId].customPolicy = '';
  const invalidModify = validatePolicyEnvelope(governed, envelope);
  assert.equal(invalidModify.valid, false);
  assert(invalidModify.errors.some((error) => /customPolicy.*requires/.test(error)));

  envelope.decisions[modifiedId] = {
    fingerprint: governed.policies[0].fingerprint,
    decision: 'APPROVE',
    justification: '',
  };
  assert(validatePolicyEnvelope(governed, envelope).errors.some((error) => /APPROVE requires/.test(error)));
  assert.equal(policyGate(governed, envelope).complete, false);
});

test('validation rejects stale orientation/source hashes, stale fingerprints, unknown policies and missing decisions', () => {
  const governed = orientation();
  const envelope = approveAll(governed);

  const staleOrientation = structuredClone(envelope);
  staleOrientation.orientationHash = HASH_B;
  assert(validatePolicyEnvelope(governed, staleOrientation).errors.some((error) => /stale orientation hash/.test(error)));

  const staleSources = structuredClone(envelope);
  staleSources.sourceHashes.bkvince = HASH_A;
  assert(validatePolicyEnvelope(governed, staleSources).errors.some((error) => /source hashes/.test(error)));

  const policyId = governed.policies[0].id;
  const staleFingerprint = structuredClone(envelope);
  staleFingerprint.decisions[policyId].fingerprint = HASH_A;
  assert(validatePolicyEnvelope(governed, staleFingerprint).errors.some((error) => /stale policy fingerprint/.test(error)));

  const unknown = structuredClone(envelope);
  unknown.decisions.UNKNOWN_POLICY = {
    fingerprint: HASH_A,
    decision: 'PENDING',
    justification: '',
  };
  assert(validatePolicyEnvelope(governed, unknown).errors.some((error) => /unknown policy id/.test(error)));

  const missing = structuredClone(envelope);
  delete missing.decisions[policyId];
  assert(validatePolicyEnvelope(governed, missing).errors.some((error) => /decision is missing/.test(error)));
});

test('migration retains only identical policy ids and fingerprints and reports stale/dropped decisions', () => {
  const previousOrientation = orientation();
  const previous = approveAll(previousOrientation);
  previous.decisions.REMOVED_POLICY = {
    fingerprint: HASH_C,
    decision: 'DISCUSS',
    justification: 'Historical policy.',
  };

  const changedPolicies = previousOrientation.policies.map((policy, index) => (
    index === 1 ? { ...policy, fingerprint: HASH_A } : policy
  ));
  const current = orientation({ orientationHash: HASH_B, policies: changedPolicies });
  const migration = migratePolicyEnvelope(current, previous, { exportedAt: EXPORTED_AT });
  assert.equal(migration.envelope.orientationHash, HASH_B);
  assert.equal(migration.report.counts.retained, changedPolicies.length - 1);
  assert.deepEqual(migration.report.stale.map((entry) => entry.policyId), [changedPolicies[1].id]);
  assert.deepEqual(migration.report.dropped.map((entry) => entry.policyId), ['REMOVED_POLICY']);
  assert.equal(migration.envelope.decisions[changedPolicies[1].id].decision, 'PENDING');
  assert.equal(migration.envelope.decisions[changedPolicies[0].id].decision, 'APPROVE');
  assert.equal(validatePolicyEnvelope(current, migration.envelope).valid, true);
});

test('export and browser runtime preserve the exact policy gate semantics', () => {
  const governed = orientation();
  const envelope = approveAll(governed);
  const exported = exportPolicyEnvelope(governed, envelope, { exportedAt: EXPORTED_AT });
  assert.deepEqual(exported, envelope);

  const isolated = {};
  new Function('globalThis', buildBrowserPolicyRuntimeSource())(isolated);
  assert(isolated.schemaPolicyRuntime);
  assert.equal(isolated.schemaPolicyRuntime.createEmptyEnvelope, isolated.schemaPolicyRuntime.createPolicyEnvelope);
  assert.equal(isolated.schemaPolicyRuntime.validateImport, isolated.schemaPolicyRuntime.validatePolicyEnvelope);
  const policyId = governed.policies[0].id;
  const modified = isolated.schemaPolicyRuntime.updateDecision(governed, envelope, policyId, {
    decision: 'MODIFY',
    customPolicy: 'A governed browser replacement.',
  });
  const approved = isolated.schemaPolicyRuntime.updateDecision(governed, modified, policyId, { decision: 'APPROVE' });
  assert.equal(approved.decisions[policyId].customPolicy, undefined, 'leaving MODIFY removes its exclusive customPolicy');
  assert.deepEqual(
    isolated.schemaPolicyRuntime.policyGate(governed, envelope),
    policyGate(governed, envelope),
  );
});
