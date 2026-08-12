import {
  FROZEN_ORIENTATION_CONTRACT_HASH,
  GLOBAL_SCHEMA_POLICIES,
  ORIENTATION_ID,
  POLICY_DECISIONS,
  POLICY_KIND,
  POLICY_SCHEMA_VERSION,
  POLICY_STORAGE_PREFIX,
} from './pd2-skills-schema-orientation-contracts.mjs';

const POLICY_RUNTIME_CONSTANTS = Object.freeze({
  frozenContractHash: FROZEN_ORIENTATION_CONTRACT_HASH,
  orientationId: ORIENTATION_ID,
  policyDecisions: POLICY_DECISIONS,
  policyKind: POLICY_KIND,
  policySchemaVersion: POLICY_SCHEMA_VERSION,
  policyStoragePrefix: POLICY_STORAGE_PREFIX,
  policies: GLOBAL_SCHEMA_POLICIES,
});

function schemaPolicyRuntimeFactory(constants) {
  'use strict';

  const ENVELOPE_KEYS = new Set([
    'schemaVersion',
    'kind',
    'orientationId',
    'orientationHash',
    'frozenContractHash',
    'sourceHashes',
    'exportedAt',
    'decisions',
  ]);
  const DECISION_KEYS = new Set(['fingerprint', 'decision', 'justification', 'customPolicy']);

  function clone(value) {
    return value === undefined ? undefined : JSON.parse(JSON.stringify(value));
  }

  function isObject(value) {
    return value !== null && typeof value === 'object' && !Array.isArray(value);
  }

  function nonBlank(value) {
    return typeof value === 'string' && value.trim().length > 0;
  }

  function isSha256(value) {
    return typeof value === 'string' && /^[A-Fa-f0-9]{64}$/.test(value);
  }

  function canonical(value) {
    if (Array.isArray(value)) return `[${value.map(canonical).join(',')}]`;
    if (isObject(value)) {
      return `{${Object.keys(value).sort().map((key) => `${JSON.stringify(key)}:${canonical(value[key])}`).join(',')}}`;
    }
    return JSON.stringify(value);
  }

  function sameValue(left, right) {
    return canonical(left) === canonical(right);
  }

  function addUnknownKeyErrors(value, allowed, path, errors) {
    if (!isObject(value)) return;
    for (const key of Object.keys(value)) {
      if (!allowed.has(key)) errors.push(`${path}.${key}: unknown property`);
    }
  }

  function validateHashTree(value, path, errors) {
    if (typeof value === 'string') {
      if (!isSha256(value)) errors.push(`${path}: expected a SHA-256 hex digest`);
      return;
    }
    if (Array.isArray(value)) {
      if (value.length === 0) errors.push(`${path}: hash array must not be empty`);
      value.forEach((item, index) => validateHashTree(item, `${path}[${index}]`, errors));
      return;
    }
    if (isObject(value)) {
      if (Object.keys(value).length === 0) errors.push(`${path}: hash object must not be empty`);
      for (const [key, item] of Object.entries(value)) validateHashTree(item, `${path}.${key}`, errors);
      return;
    }
    errors.push(`${path}: expected a SHA-256 digest or a nested collection of digests`);
  }

  function orientationObject(value) {
    if (isObject(value?.orientation) && !value.orientationHash) return value.orientation;
    return value;
  }

  function policyList(orientation) {
    const governed = orientationObject(orientation);
    return Array.isArray(governed?.policies) ? governed.policies : [];
  }

  function policyFingerprint(orientation, policy) {
    const governed = orientationObject(orientation);
    return policy?.fingerprint
      ?? governed?.policyFingerprints?.[policy?.id]
      ?? null;
  }

  function assertOrientation(orientation) {
    const governed = orientationObject(orientation);
    if (!isObject(governed)) throw new Error('A governed schema orientation oracle is required');
    if (!nonBlank(governed.orientationId)) throw new Error('orientationId is required');
    if (governed.orientationId !== constants.orientationId) throw new Error('orientationId does not match the frozen Phase 0 contract');
    if (!isSha256(governed.orientationHash)) throw new Error('orientationHash must be a SHA-256 digest');
    if (!isSha256(governed.frozenContractHash)) throw new Error('frozenContractHash must be a SHA-256 digest');
    if (governed.frozenContractHash !== constants.frozenContractHash) throw new Error('frozenContractHash does not match the frozen Phase 0 contract');
    if (!isObject(governed.sourceHashes) || Object.keys(governed.sourceHashes).length === 0) {
      throw new Error('sourceHashes are required');
    }
    const policies = policyList(governed);
    if (policies.length === 0) throw new Error('The governed orientation contains no policies');
    const contractPolicies = new Map(constants.policies.map((policy) => [policy.id, policy]));
    if (policies.length !== contractPolicies.size) throw new Error('The governed orientation policy set is incomplete or extended');
    const ids = new Set();
    for (const policy of policies) {
      if (!nonBlank(policy?.id)) throw new Error('A governed policy lacks an id');
      if (ids.has(policy.id)) throw new Error(`Duplicate governed policy id ${policy.id}`);
      ids.add(policy.id);
      const contractPolicy = contractPolicies.get(policy.id);
      if (!contractPolicy) throw new Error(`Unknown governed policy id ${policy.id}`);
      if (policy.requiredForSkillCompletion !== contractPolicy.requiredForSkillCompletion) {
        throw new Error(`Policy ${policy.id} changes its frozen completion scope`);
      }
      if (!isSha256(policyFingerprint(governed, policy))) {
        throw new Error(`Policy ${policy.id} lacks its governed SHA-256 fingerprint`);
      }
    }
    return governed;
  }

  function defaultDecision(orientation, policy) {
    return {
      fingerprint: policyFingerprint(orientation, policy),
      decision: 'PENDING',
      justification: '',
    };
  }

  function decisionSource(binding) {
    if (!isObject(binding)) return {};
    if (isObject(binding.decisions)) return binding.decisions;
    return binding;
  }

  function createPolicyEnvelope(orientation, binding = undefined) {
    const governed = assertOrientation(orientation);
    const supplied = decisionSource(binding);
    const decisions = {};
    for (const policy of policyList(governed)) {
      const candidate = supplied[policy.id];
      decisions[policy.id] = isObject(candidate)
        ? { ...defaultDecision(governed, policy), ...clone(candidate) }
        : defaultDecision(governed, policy);
    }
    return {
      schemaVersion: constants.policySchemaVersion,
      kind: constants.policyKind,
      orientationId: governed.orientationId,
      orientationHash: governed.orientationHash,
      frozenContractHash: governed.frozenContractHash,
      sourceHashes: clone(governed.sourceHashes),
      exportedAt: isObject(binding) && nonBlank(binding.exportedAt)
        ? binding.exportedAt
        : new Date().toISOString(),
      decisions,
    };
  }

  function validateDecisionShape(entry, path, errors) {
    if (!isObject(entry)) {
      errors.push(`${path}: policy decision must be an object`);
      return;
    }
    addUnknownKeyErrors(entry, DECISION_KEYS, path, errors);
    if (!isSha256(entry.fingerprint)) errors.push(`${path}.fingerprint: a SHA-256 digest is required`);
    if (!constants.policyDecisions.includes(entry.decision)) errors.push(`${path}.decision: invalid policy decision`);
    if (typeof entry.justification !== 'string') errors.push(`${path}.justification: must be a string`);
    if (entry.customPolicy !== undefined && typeof entry.customPolicy !== 'string') {
      errors.push(`${path}.customPolicy: must be a string`);
    }
    if (entry.decision !== 'MODIFY' && entry.customPolicy !== undefined) {
      errors.push(`${path}.customPolicy: is permitted only for MODIFY`);
    }
    if (entry.decision === 'APPROVE' && !nonBlank(entry.justification)) {
      errors.push(`${path}.justification: APPROVE requires a justification`);
    }
    if (entry.decision === 'MODIFY') {
      if (!nonBlank(entry.justification)) errors.push(`${path}.justification: MODIFY requires a justification`);
      if (!nonBlank(entry.customPolicy)) errors.push(`${path}.customPolicy: MODIFY requires an explicit policy`);
    }
  }

  function validateEnvelopeShape(payload) {
    const errors = [];
    if (!isObject(payload)) return { valid: false, errors: ['$: policy envelope must be an object'] };
    addUnknownKeyErrors(payload, ENVELOPE_KEYS, '$', errors);
    if (payload.schemaVersion !== constants.policySchemaVersion) errors.push('$.schemaVersion: unsupported policy schema version');
    if (payload.kind !== constants.policyKind) errors.push('$.kind: invalid policy kind');
    if (!nonBlank(payload.orientationId)) errors.push('$.orientationId: required');
    if (!isSha256(payload.orientationHash)) errors.push('$.orientationHash: a SHA-256 digest is required');
    if (!isSha256(payload.frozenContractHash)) errors.push('$.frozenContractHash: a SHA-256 digest is required');
    validateHashTree(payload.sourceHashes, '$.sourceHashes', errors);
    if (typeof payload.exportedAt !== 'string' || Number.isNaN(Date.parse(payload.exportedAt))) {
      errors.push('$.exportedAt: an ISO date-time is required');
    }
    if (!isObject(payload.decisions)) errors.push('$.decisions: must be an object');
    else {
      for (const [id, entry] of Object.entries(payload.decisions)) {
        validateDecisionShape(entry, `$.decisions.${id}`, errors);
      }
    }
    return { valid: errors.length === 0, errors };
  }

  function validatePolicyEnvelope(orientation, payload) {
    let governed;
    try {
      governed = assertOrientation(orientation);
    } catch (error) {
      return { valid: false, errors: [`orientation: ${error.message}`], envelope: null };
    }
    const shape = validateEnvelopeShape(payload);
    const errors = [...shape.errors];
    if (payload?.orientationId !== governed.orientationId) errors.push('$.orientationId: does not match the governed orientation');
    if (payload?.orientationHash !== governed.orientationHash) errors.push('$.orientationHash: stale orientation hash');
    if (payload?.frozenContractHash !== governed.frozenContractHash) errors.push('$.frozenContractHash: frozen orientation contract mismatch');
    if (!sameValue(payload?.sourceHashes, governed.sourceHashes)) errors.push('$.sourceHashes: governed source hashes do not match');

    const expected = new Map(policyList(governed).map((policy) => [policy.id, policy]));
    for (const [id, entry] of Object.entries(payload?.decisions ?? {})) {
      const policy = expected.get(id);
      if (!policy) {
        errors.push(`$.decisions.${id}: unknown policy id`);
        continue;
      }
      const fingerprint = policyFingerprint(governed, policy);
      if (entry?.fingerprint !== fingerprint) errors.push(`$.decisions.${id}.fingerprint: stale policy fingerprint`);
    }
    for (const id of expected.keys()) {
      if (!Object.hasOwn(payload?.decisions ?? {}, id)) errors.push(`$.decisions.${id}: governed policy decision is missing`);
    }
    return {
      valid: errors.length === 0,
      errors: [...new Set(errors)],
      envelope: errors.length === 0 ? clone(payload) : null,
    };
  }

  function policyGate(orientation, envelope) {
    let governed;
    try {
      governed = assertOrientation(orientation);
    } catch (error) {
      return {
        complete: false,
        open: true,
        required: 0,
        closed: 0,
        remaining: 0,
        reasons: [`orientation: ${error.message}`],
        policies: [],
      };
    }
    const validation = validatePolicyEnvelope(governed, envelope);
    const states = policyList(governed).map((policy) => {
      const entry = envelope?.decisions?.[policy.id];
      const fingerprintMatches = entry?.fingerprint === policyFingerprint(governed, policy);
      const approved = entry?.decision === 'APPROVE' && nonBlank(entry?.justification);
      const modified = entry?.decision === 'MODIFY'
        && nonBlank(entry?.justification)
        && nonBlank(entry?.customPolicy);
      const closed = validation.valid && fingerprintMatches && (approved || modified);
      const required = policy.requiredForSkillCompletion === true;
      return {
        id: policy.id,
        fingerprint: policyFingerprint(governed, policy),
        required,
        decision: entry?.decision ?? null,
        closed,
        reason: closed
          ? null
          : !fingerprintMatches
            ? 'POLICY_FINGERPRINT_STALE'
            : entry?.decision === 'APPROVE'
              ? 'APPROVAL_JUSTIFICATION_REQUIRED'
              : entry?.decision === 'MODIFY'
                ? 'MODIFIED_POLICY_AND_JUSTIFICATION_REQUIRED'
                : `POLICY_${entry?.decision ?? 'MISSING'}_IS_OPEN`,
      };
    });
    const requiredStates = states.filter((state) => state.required);
    const closed = requiredStates.filter((state) => state.closed).length;
    const reasons = [
      ...validation.errors,
      ...requiredStates.filter((state) => !state.closed).map((state) => `${state.id}: ${state.reason}`),
    ];
    return {
      complete: validation.valid && closed === requiredStates.length,
      open: !validation.valid || closed !== requiredStates.length,
      required: requiredStates.length,
      closed,
      remaining: Math.max(0, requiredStates.length - closed),
      reasons: [...new Set(reasons)],
      policies: states,
    };
  }

  function exportPolicyEnvelope(orientation, value = undefined, options = {}) {
    const binding = isObject(value?.decisions) ? value : { decisions: value ?? {} };
    const envelope = createPolicyEnvelope(orientation, {
      ...binding,
      exportedAt: options.exportedAt ?? binding.exportedAt,
    });
    const validation = validatePolicyEnvelope(orientation, envelope);
    if (!validation.valid) throw new Error(`Cannot export invalid schema policies:\n${validation.errors.join('\n')}`);
    return envelope;
  }

  function migratePolicyEnvelope(orientation, previous, options = {}) {
    const governed = assertOrientation(orientation);
    const shape = validateEnvelopeShape(previous);
    if (!shape.valid) throw new Error(`Cannot migrate an invalid policy envelope:\n${shape.errors.join('\n')}`);
    if (previous.orientationId !== governed.orientationId) {
      throw new Error('Cannot migrate policies from another orientationId');
    }
    const retained = [];
    const stale = [];
    const dropped = [];
    const expected = new Map(policyList(governed).map((policy) => [policy.id, policy]));
    const decisions = {};
    for (const [id, entry] of Object.entries(previous.decisions ?? {})) {
      const policy = expected.get(id);
      if (!policy) {
        dropped.push({ policyId: id, reason: 'POLICY_REMOVED' });
        continue;
      }
      const currentFingerprint = policyFingerprint(governed, policy);
      if (entry.fingerprint !== currentFingerprint) {
        stale.push({
          policyId: id,
          reason: 'POLICY_FINGERPRINT_CHANGED',
          previousFingerprint: entry.fingerprint,
          currentFingerprint,
        });
        continue;
      }
      decisions[id] = clone(entry);
      retained.push({ policyId: id, fingerprint: currentFingerprint });
    }
    const envelope = createPolicyEnvelope(governed, {
      decisions,
      exportedAt: options.exportedAt ?? new Date().toISOString(),
    });
    const validation = validatePolicyEnvelope(governed, envelope);
    if (!validation.valid) throw new Error(`Migrated policy envelope is invalid:\n${validation.errors.join('\n')}`);
    return {
      envelope,
      report: {
        fromOrientationHash: previous.orientationHash,
        toOrientationHash: governed.orientationHash,
        retained,
        stale,
        dropped,
        counts: { retained: retained.length, stale: stale.length, dropped: dropped.length },
      },
    };
  }

  function policyStorageKey(orientation) {
    const governed = assertOrientation(orientation);
    return `${constants.policyStoragePrefix}${governed.orientationHash}`;
  }

  function updateDecision(orientation, envelope, policyId, patch) {
    const governed = assertOrientation(orientation);
    const policy = policyList(governed).find((candidate) => candidate.id === policyId);
    if (!policy) throw new Error(`Unknown governed policy ${policyId}`);
    const current = isObject(envelope)
      ? clone(envelope)
      : createPolicyEnvelope(governed);
    current.decisions[policyId] = {
      ...defaultDecision(governed, policy),
      ...(current.decisions?.[policyId] ?? {}),
      ...(clone(patch) ?? {}),
      fingerprint: policyFingerprint(governed, policy),
    };
    if (current.decisions[policyId].decision !== 'MODIFY') {
      delete current.decisions[policyId].customPolicy;
    }
    current.exportedAt = new Date().toISOString();
    return current;
  }

  const api = {
    constants: Object.freeze(clone(constants)),
    createPolicyEnvelope,
    validateEnvelopeShape,
    validatePolicyEnvelope,
    policyGate,
    exportPolicyEnvelope,
    migratePolicyEnvelope,
    policyStorageKey,
    updateDecision,
  };
  api.createEmptyEnvelope = createPolicyEnvelope;
  api.validateImport = validatePolicyEnvelope;
  api.storageKey = policyStorageKey;
  api.exportEnvelope = exportPolicyEnvelope;
  return Object.freeze(api);
}

export const schemaPolicyRuntime = schemaPolicyRuntimeFactory(POLICY_RUNTIME_CONSTANTS);

export const createPolicyEnvelope = schemaPolicyRuntime.createPolicyEnvelope;
export const validatePolicyEnvelopeShape = schemaPolicyRuntime.validateEnvelopeShape;
export const validatePolicyEnvelope = schemaPolicyRuntime.validatePolicyEnvelope;
export const policyGate = schemaPolicyRuntime.policyGate;
export const exportPolicyEnvelope = schemaPolicyRuntime.exportPolicyEnvelope;
export const migratePolicyEnvelope = schemaPolicyRuntime.migratePolicyEnvelope;
export const policyStorageKey = schemaPolicyRuntime.policyStorageKey;
export const updatePolicyDecision = schemaPolicyRuntime.updateDecision;
export const createEmptyEnvelope = schemaPolicyRuntime.createEmptyEnvelope;
export const validateImport = schemaPolicyRuntime.validateImport;
export const storageKey = schemaPolicyRuntime.storageKey;
export const exportEnvelope = schemaPolicyRuntime.exportEnvelope;

export function buildBrowserPolicyRuntimeSource() {
  return `;globalThis.schemaPolicyRuntime=(${schemaPolicyRuntimeFactory.toString()})(${JSON.stringify(POLICY_RUNTIME_CONSTANTS)});`;
}

export const browserPolicyRuntimeSource = buildBrowserPolicyRuntimeSource();
