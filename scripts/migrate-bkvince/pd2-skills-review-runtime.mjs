import {
  COMPONENT_DECISIONS,
  DECISION_BUNDLE_SCOPES,
  DECISION_EXPORT_KIND,
  DECISION_SCHEMA_VERSION,
  FROZEN_CONTRACT_HASH,
  GLOBAL_DECISIONS,
  IMPLEMENTATION_STATUSES,
  LEGACY_DECISION_SCHEMA_VERSIONS,
  LEGACY_STORAGE_KEY_PREFIXES,
  NEW_SKILL_LINE_DECISIONS,
  PROOF_STATUSES,
  REVIEW_ID,
  STORAGE_KEY_PREFIX,
  TECHNICAL_AUTO_RESOLUTIONS,
} from './pd2-skills-review-contracts.mjs';
import {
  buildBrowserPolicyRuntimeSource,
  schemaPolicyRuntime,
} from './pd2-skills-schema-policy-runtime.mjs';

const RUNTIME_CONSTANTS = Object.freeze({
  componentDecisions: COMPONENT_DECISIONS,
  decisionBundleScopes: DECISION_BUNDLE_SCOPES,
  decisionExportKind: DECISION_EXPORT_KIND,
  decisionSchemaVersion: DECISION_SCHEMA_VERSION,
  frozenContractHash: FROZEN_CONTRACT_HASH,
  globalDecisions: GLOBAL_DECISIONS,
  implementationStatuses: IMPLEMENTATION_STATUSES,
  legacyDecisionSchemaVersions: LEGACY_DECISION_SCHEMA_VERSIONS,
  legacyStorageKeyPrefixes: LEGACY_STORAGE_KEY_PREFIXES,
  newSkillLineDecisions: NEW_SKILL_LINE_DECISIONS,
  proofStatuses: PROOF_STATUSES,
  reviewId: REVIEW_ID,
  storageKeyPrefix: STORAGE_KEY_PREFIX,
  technicalAutoResolutions: TECHNICAL_AUTO_RESOLUTIONS,
});

function decisionRuntimeFactory(constants, policyRuntime) {
  'use strict';

  const EMPTY_NOTES = Object.freeze({
    general: '',
    designObjective: '',
    bkvinceProblem: '',
    finalJustification: '',
    testPlan: '',
  });
  const ENTRY_KEYS = new Set([
    'fingerprint',
    'globalDecision',
    'newSkillLineDecision',
    'implementationStatus',
    'bundleDecisions',
    'fieldDecisions',
    'notes',
  ]);
  const LEGACY_ENTRY_KEYS = new Set([
    'fingerprint',
    'globalDecision',
    'newSkillLineDecision',
    'implementationStatus',
    'componentDecisions',
    'fieldDecisions',
    'notes',
  ]);
  const CHOICE_KEYS = new Set([
    'decision',
    'customValue',
    'customValues',
    'justification',
    'gameplayObjective',
    'testPlan',
    'protectedOverride',
    'expertOverride',
  ]);
  const OVERRIDE_KEYS = new Set([
    'approved',
    'justification',
    'acknowledgedProofStatus',
    'nativeRiskAccepted',
    'malformedResolution',
  ]);
  const EXPERT_OVERRIDE_KEYS = new Set(['enabled', 'justification']);
  const NOTE_KEYS = new Set(Object.keys(EMPTY_NOTES));
  const ENVELOPE_KEYS = new Set([
    'schemaVersion',
    'kind',
    'reviewId',
    'comparisonHash',
    'frozenContractHash',
    'sourceHashes',
    'exportedAt',
    'exportScope',
    'schemaPolicy',
    'entries',
  ]);

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

  function isReadOnlySkill(skill) {
    return skill?.readOnly === true || skill?.identical === true;
  }

  function isNewPlayerSkill(skill) {
    return skill?.newPd2PlayerSkill === true;
  }

  function newSkillDerivedField(skill, field) {
    const proposal = skill?.newSkillPlan?.proposedRow;
    const provenance = proposal?.mappingProvenance;
    if (!isObject(provenance)) return null;
    const canonicalHeader = (value) => {
      const header = String(value ?? '').trim().toLowerCase();
      return header === '*id' ? 'id' : header;
    };
    const sourceHeader = canonicalHeader(field?.pd2Source?.header ?? field?.sourceLocators?.pd2?.header ?? field?.header);
    const targetHeader = canonicalHeader(field?.target?.header ?? field?.sourceLocators?.bkvince?.header ?? field?.header);
    const matches = Object.entries(provenance).filter(([candidateHeader, item]) => (
      item?.mode === 'APPEND_PREVIEW_DOCUMENTARY_VALUE'
      && (canonicalHeader(candidateHeader) === targetHeader
        || canonicalHeader(item?.sourceHeader) === sourceHeader)
    ));
    return matches.length === 1 ? { targetHeader: matches[0][0], provenance: matches[0][1], proposal } : null;
  }

  function skillMap(report) {
    return new Map((report?.skills ?? []).map((skill) => [skill.stableId, skill]));
  }

  function componentMap(skill) {
    return new Map((skill?.components ?? []).map((component) => [component.id, component]));
  }

  function bundleMap(skill) {
    return new Map((skill?.decisionBundles ?? []).map((bundle) => [bundle.id, bundle]));
  }

  function fieldMap(skill) {
    const result = new Map();
    for (const component of skill?.components ?? []) {
      for (const field of component.fields ?? []) {
        result.set(field.id, { component, field, bundleId: field.decisionOwnerBundleId ?? null });
      }
    }
    return result;
  }

  function createEntry(skill) {
    if (!skill || !nonBlank(skill.stableId) || !nonBlank(skill.fingerprint)) {
      throw new Error('A canonical skill with stableId and fingerprint is required');
    }
    const entry = {
      fingerprint: skill.fingerprint,
      globalDecision: null,
      implementationStatus: 'NOT_REVIEWED',
      bundleDecisions: {},
      fieldDecisions: {},
      notes: { ...EMPTY_NOTES },
    };
    if (isNewPlayerSkill(skill)) entry.newSkillLineDecision = null;
    return entry;
  }

  function createEmptyEnvelope(report) {
    if (!report || !nonBlank(report.comparisonHash)) throw new Error('The governed report comparisonHash is required');
    const entries = {};
    for (const skill of report.skills ?? []) {
      if (!isReadOnlySkill(skill)) entries[skill.stableId] = createEntry(skill);
    }
    return {
      schemaVersion: constants.decisionSchemaVersion,
      kind: constants.decisionExportKind,
      reviewId: report.reviewId ?? constants.reviewId,
      comparisonHash: report.comparisonHash,
      frozenContractHash: report.frozenContractHash ?? constants.frozenContractHash,
      sourceHashes: clone(report.sourceHashes ?? {}),
      exportedAt: new Date().toISOString(),
      exportScope: 'ALL',
      schemaPolicy: report.schemaPolicy?.envelope
        ? clone(report.schemaPolicy.envelope)
        : report.schemaOrientation
          ? policyRuntime.createPolicyEnvelope(report.schemaOrientation)
          : null,
      entries,
    };
  }

  function storageKey(reportOrHash) {
    const comparisonHash = typeof reportOrHash === 'string'
      ? reportOrHash
      : reportOrHash?.comparisonHash;
    if (!nonBlank(comparisonHash)) throw new Error('comparisonHash is required for localStorage');
    return `${constants.storageKeyPrefix}${comparisonHash}`;
  }

  function legacyStorageKeys(reportOrHash) {
    const comparisonHash = typeof reportOrHash === 'string'
      ? reportOrHash
      : reportOrHash?.comparisonHash;
    if (!nonBlank(comparisonHash)) throw new Error('comparisonHash is required for legacy localStorage discovery');
    return constants.legacyStorageKeyPrefixes.map((prefix) => `${prefix}${comparisonHash}`);
  }

  function schemaPolicyFor(report, explicit = undefined) {
    if (!report?.schemaOrientation) return null;
    return explicit ?? report.schemaPolicy?.envelope ?? null;
  }

  function schemaPolicyGate(report, explicit = undefined) {
    if (!report?.schemaOrientation) return null;
    const envelope = schemaPolicyFor(report, explicit);
    if (!envelope) {
      const required = (report.schemaOrientation.policies ?? [])
        .filter((policy) => policy.requiredForSkillCompletion === true).length;
      return {
        complete: false,
        open: true,
        required,
        closed: 0,
        remaining: required,
        reasons: ['schemaPolicy: governed Phase 0 policy envelope is required'],
        policies: [],
      };
    }
    return policyRuntime.policyGate(report.schemaOrientation, envelope);
  }

  function proofStatusFor(field, component) {
    return field?.proofStatus ?? component?.proofStatus ?? 'EXACT_TABLE';
  }

  function portabilityValues(field, component) {
    const candidates = [field?.portability, component?.portability];
    const values = [];
    for (const candidate of candidates) {
      if (Array.isArray(candidate)) values.push(...candidate);
      else if (Array.isArray(candidate?.categories)) values.push(...candidate.categories);
      else if (typeof candidate === 'string') values.push(candidate);
    }
    return values;
  }

  function hasNativeRisk(field, component) {
    const proof = proofStatusFor(field, component);
    if (proof === 'NATIVE_UNPROVEN') return true;
    const portability = field?.proofStatus
      ? portabilityValues(field, null)
      : portabilityValues(field, component);
    if (portability.some((value) => (
      value === 'NATIVE_UNPROVEN' || value === 'NATIVE_FUNCTION_MISMATCH'
    ))) return true;
    return (field?.protectionReasons ?? []).some((reason) => /native|callback|srv|clt|hitfunc/i.test(String(reason)));
  }

  function hasMalformedRisk(field, component) {
    if (proofStatusFor(field, component) === 'MALFORMED_SOURCE') return true;
    return (field?.protectionReasons ?? []).some((reason) => /malform/i.test(String(reason)));
  }

  function validateOverride(override, field, component, path = 'choice.protectedOverride') {
    const errors = [];
    if (!isObject(override)) return [`${path}: an explicit protected override is required`];
    addUnknownKeyErrors(override, OVERRIDE_KEYS, path, errors);
    if (override.approved !== true) errors.push(`${path}.approved: must be true`);
    if (!nonBlank(override.justification)) errors.push(`${path}.justification: a justification is required`);
    if (!constants.proofStatuses.includes(override.acknowledgedProofStatus)) {
      errors.push(`${path}.acknowledgedProofStatus: unknown proof status`);
    } else {
      const expected = proofStatusFor(field, component);
      if (field && override.acknowledgedProofStatus !== expected) {
        errors.push(`${path}.acknowledgedProofStatus: must acknowledge ${expected}`);
      }
    }
    if (hasNativeRisk(field, component) && override.nativeRiskAccepted !== true) {
      errors.push(`${path}.nativeRiskAccepted: must be true for a native-unproven or divergent callback`);
    }
    if (hasMalformedRisk(field, component) && !nonBlank(override.malformedResolution)) {
      errors.push(`${path}.malformedResolution: a governed resolution is required for MALFORMED_SOURCE`);
    }
    return errors;
  }

  function validateExpertOverride(override, path) {
    const errors = [];
    if (!isObject(override)) return [`${path}: an explicit expert override is required`];
    addUnknownKeyErrors(override, EXPERT_OVERRIDE_KEYS, path, errors);
    if (override.enabled !== true) errors.push(`${path}.enabled: must be true`);
    if (!nonBlank(override.justification)) errors.push(`${path}.justification: a justification is required`);
    return errors;
  }

  function validateChoice(choice, {
    field = null,
    component = null,
    bundle = null,
    expert = false,
    path = 'choice',
  } = {}) {
    const errors = [];
    if (!isObject(choice)) return { valid: false, errors: [`${path}: choice must be an object`] };
    addUnknownKeyErrors(choice, CHOICE_KEYS, path, errors);
    if (!constants.componentDecisions.includes(choice.decision)) {
      errors.push(`${path}.decision: unknown bundle/field decision`);
      return { valid: false, errors };
    }
    if (choice.decision === 'CUSTOM') {
      if (bundle) {
        if (!isObject(choice.customValues) || Object.keys(choice.customValues).length === 0) {
          errors.push(`${path}.customValues: bundle CUSTOM requires an explicit non-empty value/formula map`);
        } else {
          for (const [key, value] of Object.entries(choice.customValues)) {
            if (!nonBlank(String(value ?? ''))) errors.push(`${path}.customValues.${key}: an explicit value or formula is required`);
          }
          const requiredCustomKeys = bundle?.customSchema?.required ?? [];
          for (const key of requiredCustomKeys) {
            if (!Object.hasOwn(choice.customValues, key) || !nonBlank(String(choice.customValues[key] ?? ''))) {
              errors.push(`${path}.customValues.${key}: required by the bundle custom schema`);
            }
          }
        }
        if (choice.customValue !== undefined) errors.push(`${path}.customValue: bundle CUSTOM uses customValues, not customValue`);
      } else if (!nonBlank(choice.customValue)) {
        errors.push(`${path}.customValue: field CUSTOM requires an explicit value or formula`);
      }
      if (!nonBlank(choice.justification)) errors.push(`${path}.justification: CUSTOM requires a justification`);
      if (!nonBlank(choice.testPlan)) errors.push(`${path}.testPlan: CUSTOM requires a test plan`);
    } else {
      if (choice.customValue !== undefined) errors.push(`${path}.customValue: is permitted only for CUSTOM`);
      if (choice.customValues !== undefined) errors.push(`${path}.customValues: is permitted only for CUSTOM`);
    }
    if (expert || field) errors.push(...validateExpertOverride(choice.expertOverride, `${path}.expertOverride`));
    else if (choice.expertOverride !== undefined) errors.push(`${path}.expertOverride: is permitted only for field decisions`);
    const requiresOverride = field?.protected === true
      && (choice.decision === 'ADOPT_PD2' || choice.decision === 'CUSTOM');
    if (requiresOverride) {
      errors.push(...validateOverride(choice.protectedOverride, field, component, `${path}.protectedOverride`));
    } else if (field && choice.protectedOverride !== undefined) {
      errors.push(`${path}.protectedOverride: override is allowed only when adopting or customizing a protected field`);
    }
    return { valid: errors.length === 0, errors };
  }

  function validateChoiceShape(choice, path = 'choice', options = {}) {
    const errors = [];
    if (!isObject(choice)) return { valid: false, errors: [`${path}: choice must be an object`] };
    addUnknownKeyErrors(choice, CHOICE_KEYS, path, errors);
    if (!constants.componentDecisions.includes(choice.decision)) errors.push(`${path}.decision: unknown component/field decision`);
    for (const key of ['customValue', 'justification', 'gameplayObjective', 'testPlan']) {
      if (choice[key] !== undefined && typeof choice[key] !== 'string') errors.push(`${path}.${key}: must be a string`);
    }
    if (choice.customValues !== undefined && !isObject(choice.customValues)) {
      errors.push(`${path}.customValues: must be an object`);
    } else if (isObject(choice.customValues)) {
      for (const [key, value] of Object.entries(choice.customValues)) {
        if (typeof value !== 'string' && typeof value !== 'number' && typeof value !== 'boolean' && value !== null) {
          errors.push(`${path}.customValues.${key}: must be a scalar value or formula`);
        }
      }
    }
    if (choice.protectedOverride !== undefined) {
      const override = choice.protectedOverride;
      if (!isObject(override)) errors.push(`${path}.protectedOverride: must be an object`);
      else {
        addUnknownKeyErrors(override, OVERRIDE_KEYS, `${path}.protectedOverride`, errors);
        if (override.approved !== undefined && typeof override.approved !== 'boolean') errors.push(`${path}.protectedOverride.approved: must be a boolean`);
        if (override.justification !== undefined && typeof override.justification !== 'string') errors.push(`${path}.protectedOverride.justification: must be a string`);
        if (override.acknowledgedProofStatus !== undefined && !constants.proofStatuses.includes(override.acknowledgedProofStatus)) {
          errors.push(`${path}.protectedOverride.acknowledgedProofStatus: unknown proof status`);
        }
        if (override.nativeRiskAccepted !== undefined && typeof override.nativeRiskAccepted !== 'boolean') errors.push(`${path}.protectedOverride.nativeRiskAccepted: must be a boolean`);
        if (override.malformedResolution !== undefined && typeof override.malformedResolution !== 'string') errors.push(`${path}.protectedOverride.malformedResolution: must be a string`);
      }
    }
    if (choice.expertOverride !== undefined) {
      const override = choice.expertOverride;
      if (!isObject(override)) errors.push(`${path}.expertOverride: must be an object`);
      else {
        addUnknownKeyErrors(override, EXPERT_OVERRIDE_KEYS, `${path}.expertOverride`, errors);
        if (override.enabled !== undefined && typeof override.enabled !== 'boolean') errors.push(`${path}.expertOverride.enabled: must be a boolean`);
        if (override.justification !== undefined && typeof override.justification !== 'string') errors.push(`${path}.expertOverride.justification: must be a string`);
      }
    }
    if (options.expert === true) errors.push(...validateExpertOverride(choice.expertOverride, `${path}.expertOverride`));
    return { valid: errors.length === 0, errors };
  }

  function bundleAutoResolution(bundle) {
    const resolution = bundle?.autoResolution ?? bundle?.defaultResolution ?? null;
    if (!constants.technicalAutoResolutions.includes(resolution)) return null;
    if (resolution === 'NOT_APPLICABLE') return { decision: 'NOT_APPLICABLE', autoResolution: resolution };
    return { decision: 'KEEP_BKVINCE', autoResolution: resolution };
  }

  function resolveBundleChoice(skill, entry, bundleOrId) {
    const bundle = typeof bundleOrId === 'string' ? bundleMap(skill).get(bundleOrId) : bundleOrId;
    if (!bundle) return null;
    if (bundle.scope === 'TECHNICAL' || bundle.manualDecisionRequired === false) {
      return bundleAutoResolution(bundle);
    }
    return entry?.bundleDecisions?.[bundle.id] ?? null;
  }

  function bundleForField(skill, field) {
    if (!field) return null;
    const direct = field.decisionOwnerBundleId ? bundleMap(skill).get(field.decisionOwnerBundleId) : null;
    if (direct) return direct;
    const matches = (skill?.decisionBundles ?? []).filter((bundle) => (bundle.fieldIds ?? []).includes(field.id));
    return matches.length === 1 ? matches[0] : null;
  }

  function customValueForField(choice, field) {
    if (!isObject(choice?.customValues)) return undefined;
    for (const key of [field?.id, field?.header, field?.locator?.header, field?.target?.header]) {
      if (key && Object.hasOwn(choice.customValues, key)) return choice.customValues[key];
    }
    return undefined;
  }

  function resolveFieldChoice(skill, entry, componentOrId, fieldOrId) {
    const components = componentMap(skill);
    const fields = fieldMap(skill);
    const component = typeof componentOrId === 'string' ? components.get(componentOrId) : componentOrId;
    const found = typeof fieldOrId === 'string' ? fields.get(fieldOrId) : null;
    const field = found?.field ?? fieldOrId;
    const owner = found?.component ?? component;
    if (!field || !owner) return null;
    const expertChoice = entry?.fieldDecisions?.[field.id];
    if (expertChoice) return expertChoice;
    const bundle = bundleForField(skill, field);
    const choice = resolveBundleChoice(skill, entry, bundle);
    if (!choice) return null;
    if (choice.decision !== 'CUSTOM') return choice;
    return { ...choice, customValue: customValueForField(choice, field) };
  }

  function rawEvidenceValue(field, source) {
    const evidence = field?.rawEvidence?.[source];
    if (isObject(evidence) && Object.hasOwn(evidence, 'rawValue')) return evidence.rawValue;
    if (isObject(evidence) && Object.hasOwn(evidence, 'value')) return evidence.value;
    if (evidence !== undefined && !isObject(evidence)) return evidence;
    return field?.values?.[source];
  }

  function projectProposedResult(report, skillOrId, entry) {
    const skill = typeof skillOrId === 'string' ? skillMap(report).get(skillOrId) : skillOrId;
    if (!skill) return { valid: false, stableId: null, errors: ['Unknown canonical skill'], cells: [], changedCells: [], keptCells: [] };
    const fields = fieldMap(skill);
    const errors = [];
    const ownerCounts = new Map();
    for (const bundle of skill.decisionBundles ?? []) {
      for (const fieldId of bundle.fieldIds ?? []) ownerCounts.set(fieldId, (ownerCounts.get(fieldId) ?? 0) + 1);
    }
    const cells = [];
    for (const [fieldId, known] of fields) {
      const { field, component } = known;
      const bundle = bundleForField(skill, field);
      const decisionRelevant = field.decisionRelevant !== false
        && (field.rawChanged === true || field.semanticChanged === true || field.changed !== false || Boolean(bundle));
      if (decisionRelevant && (!bundle || ownerCounts.get(fieldId) !== 1)) {
        errors.push(`${fieldId}: decision-relevant field must have exactly one decision owner bundle`);
      }
      const before = rawEvidenceValue(field, 'bkvince');
      let after = before;
      let decision = 'KEEP_BKVINCE';
      let source = 'BKVINCE_BASELINE';
      let choice = null;
      if (bundle) {
        choice = resolveBundleChoice(skill, entry, bundle);
        if (choice) {
          decision = choice.decision;
          source = bundle.scope === 'TECHNICAL' || bundle.manualDecisionRequired === false
            ? 'TECHNICAL_AUTO_RESOLUTION'
            : 'PLAYER_BUNDLE_DECISION';
          if (choice.decision === 'ADOPT_PD2') after = rawEvidenceValue(field, 'pd2');
          else if (choice.decision === 'CUSTOM') {
            const customValue = customValueForField(choice, field);
            if (customValue === undefined) errors.push(`${bundle.id}.${fieldId}: CUSTOM has no projected value`);
            else after = customValue;
          }
          if ((choice.decision === 'ADOPT_PD2' || choice.decision === 'CUSTOM') && field.protected === true) {
            errors.push(`${fieldId}: protected field requires an explicit expert field override`);
            after = before;
            decision = 'KEEP_BKVINCE';
            source = 'PROTECTED_BKVINCE_BASELINE';
          }
        }
      }
      const expertChoice = entry?.fieldDecisions?.[fieldId];
      if (expertChoice) {
        const validation = validateChoice(expertChoice, {
          field,
          component,
          expert: true,
          path: `fieldDecisions.${fieldId}`,
        });
        if (!validation.valid) errors.push(...validation.errors);
        else {
          choice = expertChoice;
          decision = expertChoice.decision;
          source = 'EXPERT_FIELD_OVERRIDE';
          if (decision === 'ADOPT_PD2') after = rawEvidenceValue(field, 'pd2');
          else if (decision === 'CUSTOM') after = expertChoice.customValue;
          else after = before;
        }
      }
      if (decision === 'ADOPT_PD2' && after === undefined) errors.push(`${fieldId}: selected PD2 value is absent`);
      cells.push({
        stableId: skill.stableId,
        fieldId,
        componentId: component.id,
        bundleId: bundle?.id ?? null,
        bundleScope: bundle?.scope ?? null,
        table: field.table ?? field.locator?.table ?? null,
        header: field.header ?? field.locator?.header ?? null,
        rowKey: field.rowKey ?? field.locator?.rowKey ?? field.target?.rowKey ?? null,
        rowOrdinal: field.rowOrdinal ?? field.locator?.rowOrdinal ?? field.target?.rowOrdinal ?? null,
        locator: clone(field.locator ?? field.target ?? null),
        before,
        after,
        decision,
        source,
        choice: clone(choice),
        autoResolution: choice?.autoResolution ?? null,
        proofStatus: proofStatusFor(field, component),
        protected: field.protected === true,
        protectionReasons: clone(field.protectionReasons ?? []),
        rawEvidence: clone(field.rawEvidence ?? field.values ?? {}),
      });
    }
    const changedCells = cells.filter((cell) => !sameValue(cell.before, cell.after));
    return {
      valid: errors.length === 0,
      stableId: skill.stableId,
      fingerprint: skill.fingerprint,
      baseline: 'BKVINCE',
      errors: [...new Set(errors)],
      cells,
      byField: Object.fromEntries(cells.map((cell) => [cell.fieldId, cell])),
      changedCells,
      keptCells: cells.filter((cell) => sameValue(cell.before, cell.after)),
    };
  }

  function shouldRequireDetails(skill, entry) {
    if (!entry?.globalDecision) return true;
    if (['REJECT_PD2', 'DEFER_NATIVE_PROOF', 'DISCUSS'].includes(entry.globalDecision)) return false;
    if (isNewPlayerSkill(skill)) {
      return ['IMPORT_APPEND_ONLY', 'IMPORT_CUSTOMIZED'].includes(entry.newSkillLineDecision);
    }
    return true;
  }

  function entryState(report, skillOrId, entry, schemaPolicy = undefined) {
    const skill = typeof skillOrId === 'string' ? skillMap(report).get(skillOrId) : skillOrId;
    if (!skill) {
      return {
        required: true,
        complete: false,
        readOnly: false,
        reasons: ['Unknown canonical skill'],
        requirements: { total: 1, complete: 0 },
        components: [],
      };
    }
    if (isReadOnlySkill(skill)) {
      return {
        required: false,
        complete: true,
        readOnly: true,
        autoResolved: 'KEEP_BKVINCE',
        reasons: [],
        requirements: { total: 0, complete: 0 },
        components: [],
      };
    }

    const current = entry ?? createEntry(skill);
    const reasons = [];
    const componentStates = [];
    let total = 1;
    let completed = 0;
    const phase0Gate = schemaPolicyGate(report, schemaPolicy);
    if (phase0Gate) {
      total += 1;
      if (phase0Gate.complete) completed += 1;
      if (!phase0Gate.complete) reasons.push(...phase0Gate.reasons.map((reason) => `Phase 0 policy gate: ${reason}`));
    }
    if (constants.globalDecisions.includes(current.globalDecision) && current.globalDecision !== 'DISCUSS') completed += 1;
    else if (current.globalDecision === 'DISCUSS') reasons.push('Global decision remains DISCUSS');
    else reasons.push('Global decision is required');

    if (current.fingerprint !== skill.fingerprint) reasons.push('Skill fingerprint is stale');
    if (!constants.implementationStatuses.includes(current.implementationStatus)) {
      reasons.push('Implementation status is invalid');
    }
    if (current.globalDecision === 'IMPORT_NEW_PD2_SKILL' && !isNewPlayerSkill(skill)) {
      reasons.push('IMPORT_NEW_PD2_SKILL is valid only for a governed new PD2 player skill');
    }

    const terminalDecisions = new Set([
      'KEEP_BKVINCE',
      'ADAPT_PD2_SELECTIVELY',
      'ADOPT_PD2_MODEL',
      'IMPORT_NEW_PD2_SKILL',
      'REJECT_PD2',
    ]);
    if (terminalDecisions.has(current.globalDecision)) {
      total += 1;
      if (nonBlank(current.notes?.finalJustification)) completed += 1;
      else reasons.push('notes.finalJustification is required for a terminal design decision');
    }
    if (['DISCUSS', 'DEFER_NATIVE_PROOF'].includes(current.globalDecision)) {
      total += 1;
      if (nonBlank(current.notes?.general) || nonBlank(current.notes?.finalJustification)) completed += 1;
      else reasons.push('DISCUSS/DEFER_NATIVE_PROOF requires notes.general or notes.finalJustification');
    }
    const implementationNeedsTests = [
      'SELECTED_FOR_PROTOTYPE',
      'IMPLEMENTATION_NOT_AUTHORIZED',
      'IMPLEMENTATION_AUTHORIZED',
      'IMPLEMENTED',
      'TESTED',
    ].includes(current.implementationStatus);
    if (['ADAPT_PD2_SELECTIVELY', 'ADOPT_PD2_MODEL', 'IMPORT_NEW_PD2_SKILL'].includes(current.globalDecision)
      || implementationNeedsTests) {
      total += 1;
      if (nonBlank(current.notes?.testPlan)) completed += 1;
      else reasons.push('notes.testPlan is required for an adopted/prototype design');
    }

    if (isNewPlayerSkill(skill)) {
      total += 1;
      if (constants.newSkillLineDecisions.includes(current.newSkillLineDecision)
        && current.newSkillLineDecision !== 'DISCUSS') completed += 1;
      else if (current.newSkillLineDecision === 'DISCUSS') reasons.push('New-skill line decision remains DISCUSS');
      else reasons.push('New-skill line decision is required before field decisions');
      if (['IMPORT_APPEND_ONLY', 'IMPORT_CUSTOMIZED'].includes(current.newSkillLineDecision)
        && current.globalDecision !== 'IMPORT_NEW_PD2_SKILL') {
        reasons.push('An imported new row requires global decision IMPORT_NEW_PD2_SKILL');
      }
      if (current.newSkillLineDecision === 'REJECT_PD2_SKILL'
        && current.globalDecision !== 'REJECT_PD2') {
        reasons.push('REJECT_PD2_SKILL requires global decision REJECT_PD2');
      }
      if (current.newSkillLineDecision === 'DEFER_NATIVE_PROOF'
        && current.globalDecision !== 'DEFER_NATIVE_PROOF') {
        reasons.push('New-skill line DEFER_NATIVE_PROOF requires the matching global decision');
      }
      if (current.globalDecision && ![
        'IMPORT_NEW_PD2_SKILL',
        'REJECT_PD2',
        'DEFER_NATIVE_PROOF',
        'DISCUSS',
      ].includes(current.globalDecision)) {
        reasons.push(`${current.globalDecision} is invalid for a new PD2-only player skill`);
      }
    } else if (current.newSkillLineDecision !== undefined) {
      reasons.push('newSkillLineDecision is forbidden for an existing skill');
    }

    const detailsRequired = shouldRequireDetails(skill, current);
    let customChoices = 0;
    const bundleStates = [];
    for (const bundle of skill.decisionBundles ?? []) {
      const technical = bundle.scope === 'TECHNICAL' || bundle.manualDecisionRequired === false;
      const required = detailsRequired && !technical && bundle.manualDecisionRequired !== false;
      const choice = resolveBundleChoice(skill, current, bundle);
      const validation = required
        ? validateChoice(choice, { bundle, path: `bundleDecisions.${bundle.id}` })
        : { valid: true, errors: [] };
      const choiceErrors = [];
      if (required) {
        total += 1;
        if (validation.valid && choice?.decision !== 'DISCUSS') completed += 1;
        else {
          choiceErrors.push(...(choice?.decision === 'DISCUSS'
            ? [`bundleDecisions.${bundle.id}: decision remains DISCUSS`]
            : validation.errors));
          reasons.push(...choiceErrors);
        }
      }
      if (choice?.decision === 'CUSTOM') customChoices += 1;
      bundleStates.push({
        id: bundle.id,
        scope: bundle.scope,
        required,
        complete: !required || (validation.valid && choice?.decision !== 'DISCUSS'),
        technicalAutoResolution: technical ? (bundle.autoResolution ?? bundle.defaultResolution ?? null) : null,
        effectiveChoice: clone(choice),
        fieldIds: clone(bundle.fieldIds ?? []),
        reasons: choiceErrors,
      });
    }

    const knownFields = fieldMap(skill);
    for (const [fieldId, choice] of Object.entries(current.fieldDecisions ?? {})) {
      const known = knownFields.get(fieldId);
      if (!known) continue;
      const validation = validateChoice(choice, {
        field: known.field,
        component: known.component,
        expert: true,
        path: `fieldDecisions.${fieldId}`,
      });
      if (!validation.valid) reasons.push(...validation.errors);
      if (choice?.decision === 'CUSTOM') customChoices += 1;
      const derivedField = newSkillDerivedField(skill, known.field);
      if (derivedField && choice?.decision === 'ADOPT_PD2') {
        reasons.push(`fieldDecisions.${fieldId}: a derived documentary target value cannot adopt the PD2 source ordinal`);
      }
      if (derivedField && choice?.decision === 'CUSTOM'
        && String(choice.customValue) !== String(derivedField.proposal.targetOrdinal)) {
        reasons.push(`fieldDecisions.${fieldId}: documentary Id must remain equal to proposed append ordinal ${derivedField.proposal.targetOrdinal}`);
      }
    }

    if (isNewPlayerSkill(skill) && current.newSkillLineDecision === 'IMPORT_APPEND_ONLY' && customChoices > 0) {
      reasons.push('IMPORT_APPEND_ONLY cannot contain CUSTOM decisions; use IMPORT_CUSTOMIZED');
    }
    if (isNewPlayerSkill(skill) && current.newSkillLineDecision === 'IMPORT_CUSTOMIZED' && customChoices === 0) {
      reasons.push('IMPORT_CUSTOMIZED requires at least one governed CUSTOM value');
    }

    const projection = projectProposedResult(report, skill, current);
    if (detailsRequired && !projection.valid) reasons.push(...projection.errors.map((error) => `proposedResult: ${error}`));

    const maps = [
      ['bundleDecisions', bundleMap(skill)],
      ['fieldDecisions', knownFields],
    ];
    for (const [key, known] of maps) {
      for (const id of Object.keys(current[key] ?? {})) {
        if (!known.has(id)) reasons.push(`${key}.${id}: unknown decision target`);
      }
    }

    return {
      required: true,
      complete: reasons.length === 0 && completed === total,
      readOnly: false,
      autoResolved: null,
      reasons: [...new Set(reasons)],
      requirements: {
        total,
        complete: Math.min(completed, total),
        remaining: Math.max(0, total - completed),
        percent: total === 0 ? 100 : Math.round((completed / total) * 100),
      },
      bundles: bundleStates,
      components: componentStates,
      proposedResult: projection,
      schemaPolicyGate: phase0Gate ? clone(phase0Gate) : null,
    };
  }

  function normalizeEntry(skill, value) {
    const base = createEntry(skill);
    if (!isObject(value)) return base;
    return {
      ...base,
      ...clone(value),
      bundleDecisions: clone(value.bundleDecisions ?? {}),
      fieldDecisions: clone(value.fieldDecisions ?? {}),
      notes: { ...EMPTY_NOTES, ...(clone(value.notes) ?? {}) },
    };
  }

  function applyBulk(report, skillIds, entries, action, options = {}) {
    const replace = options.replace === true;
    if (replace && options.confirmed !== true) {
      throw new Error('Replacing existing decisions requires confirmed:true');
    }
    const allowedActions = new Set(['KEEP_BKVINCE', 'ADOPT_PD2', 'DISCUSS', 'CLEAR_UNRESOLVED']);
    if (!allowedActions.has(action)) throw new Error(`Unknown bulk action ${action}`);
    const wanted = new Set(Array.isArray(skillIds) ? skillIds : [skillIds]);
    const byId = skillMap(report);
    const result = clone(entries?.entries ?? entries ?? {});

    for (const id of wanted) {
      const skill = byId.get(id);
      if (!skill || isReadOnlySkill(skill)) continue;
      let current = normalizeEntry(skill, result[id]);

      if (action === 'CLEAR_UNRESOLVED') {
        if (current.globalDecision === 'DISCUSS') current.globalDecision = null;
        if (current.newSkillLineDecision === 'DISCUSS') current.newSkillLineDecision = null;
        for (const key of ['bundleDecisions', 'fieldDecisions']) {
          for (const [decisionId, choice] of Object.entries(current[key])) {
            if (choice?.decision === 'DISCUSS') delete current[key][decisionId];
          }
        }
        result[id] = current;
        continue;
      }

      if (replace) {
        current.bundleDecisions = {};
        current.fieldDecisions = {};
        current.globalDecision = null;
        if (isNewPlayerSkill(skill)) current.newSkillLineDecision = null;
      }

      const setIfOpen = (collection, key, choice) => {
        if (replace || !collection[key]?.decision) collection[key] = choice;
      };

      if (!isNewPlayerSkill(skill) && (replace || !current.globalDecision)) {
        current.globalDecision = action === 'KEEP_BKVINCE'
          ? 'KEEP_BKVINCE'
          : action === 'ADOPT_PD2'
            ? 'ADAPT_PD2_SELECTIVELY'
            : 'DISCUSS';
      }

      for (const bundle of skill.decisionBundles ?? []) {
        if (bundle.scope === 'TECHNICAL' || bundle.manualDecisionRequired === false) continue;
        const bundleChoice = action === 'KEEP_BKVINCE'
          ? { decision: 'KEEP_BKVINCE' }
          : action === 'DISCUSS'
            ? { decision: 'DISCUSS' }
            : { decision: 'ADOPT_PD2' };
        setIfOpen(current.bundleDecisions, bundle.id, bundleChoice);
      }
      result[id] = current;
    }
    return result;
  }

  function progress(report, entries, classCode = null, schemaPolicy = undefined) {
    const source = entries?.entries ?? entries ?? {};
    const governedSchemaPolicy = schemaPolicy ?? entries?.schemaPolicy;
    const phase0Gate = schemaPolicyGate(report, governedSchemaPolicy);
    const navigationView = classCode === null
      ? null
      : (report?.navigation ?? []).find((view) => view.id === classCode);
    const navigationIds = navigationView ? new Set(navigationView.skillIds ?? []) : null;
    const skills = (report?.skills ?? []).filter((skill) => (
      classCode === null
      || navigationIds?.has(skill.stableId)
      || (!navigationView && (
        skill.classCode === classCode
        || skill.scope === classCode
        || skill.tree?.classCode === classCode
      ))
    ));
    let required = 0;
    let complete = 0;
    let autoResolved = 0;
    let requirementCount = 0;
    let completedRequirements = 0;
    for (const skill of skills) {
      const state = entryState(report, skill, source[skill.stableId], governedSchemaPolicy);
      if (state.readOnly) {
        autoResolved += 1;
        complete += 1;
      } else {
        required += 1;
        if (state.complete) complete += 1;
        requirementCount += state.requirements.total;
        completedRequirements += state.requirements.complete;
      }
    }
    return {
      total: skills.length,
      required,
      autoResolved,
      complete,
      remaining: Math.max(0, skills.length - complete),
      percent: skills.length === 0 ? 100 : Math.round((complete / skills.length) * 100),
      requirements: {
        total: requirementCount,
        complete: completedRequirements,
        percent: requirementCount === 0 ? 100 : Math.round((completedRequirements / requirementCount) * 100),
      },
      schemaPolicyGate: phase0Gate ? clone(phase0Gate) : null,
    };
  }

  function validateHashTree(value, path, errors) {
    if (typeof value === 'string') {
      if (!/^[A-Fa-f0-9]{64}$/.test(value)) errors.push(`${path}: expected a SHA-256 hex digest`);
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

  function validateNotes(notes, path, errors) {
    if (!isObject(notes)) {
      errors.push(`${path}: notes must be an object`);
      return;
    }
    addUnknownKeyErrors(notes, NOTE_KEYS, path, errors);
    for (const key of NOTE_KEYS) {
      if (typeof notes[key] !== 'string') errors.push(`${path}.${key}: must be a string`);
    }
  }

  function validateEntryShape(entry, path, errors) {
    if (!isObject(entry)) {
      errors.push(`${path}: entry must be an object`);
      return;
    }
    addUnknownKeyErrors(entry, ENTRY_KEYS, path, errors);
    if (!isSha256(entry.fingerprint)) errors.push(`${path}.fingerprint: a SHA-256 digest is required`);
    if (entry.globalDecision !== null && !constants.globalDecisions.includes(entry.globalDecision)) {
      errors.push(`${path}.globalDecision: invalid decision`);
    }
    if (entry.newSkillLineDecision !== undefined && entry.newSkillLineDecision !== null
      && !constants.newSkillLineDecisions.includes(entry.newSkillLineDecision)) {
      errors.push(`${path}.newSkillLineDecision: invalid line decision`);
    }
    if (!constants.implementationStatuses.includes(entry.implementationStatus)) {
      errors.push(`${path}.implementationStatus: invalid status`);
    }
    for (const key of ['bundleDecisions', 'fieldDecisions']) {
      if (!isObject(entry[key])) {
        errors.push(`${path}.${key}: must be an object`);
        continue;
      }
      for (const [id, choice] of Object.entries(entry[key])) {
        const validation = validateChoiceShape(choice, `${path}.${key}.${id}`, { expert: key === 'fieldDecisions' });
        errors.push(...validation.errors);
      }
    }
    validateNotes(entry.notes, `${path}.notes`, errors);
  }

  function validateLegacyEntryShape(entry, path, errors) {
    if (!isObject(entry)) {
      errors.push(`${path}: legacy entry must be an object`);
      return;
    }
    addUnknownKeyErrors(entry, LEGACY_ENTRY_KEYS, path, errors);
    if (!isSha256(entry.fingerprint)) errors.push(`${path}.fingerprint: a SHA-256 digest is required`);
    if (entry.globalDecision !== null && !constants.globalDecisions.includes(entry.globalDecision)) {
      errors.push(`${path}.globalDecision: invalid decision`);
    }
    if (!constants.implementationStatuses.includes(entry.implementationStatus)) {
      errors.push(`${path}.implementationStatus: invalid status`);
    }
    for (const key of ['componentDecisions', 'fieldDecisions']) {
      if (!isObject(entry[key])) errors.push(`${path}.${key}: must be an object`);
      else for (const [id, choice] of Object.entries(entry[key])) {
        const validation = validateChoiceShape(choice, `${path}.${key}.${id}`);
        errors.push(...validation.errors);
      }
    }
    validateNotes(entry.notes, `${path}.notes`, errors);
  }

  function validateEnvelopeShape(payload, options = {}) {
    const errors = [];
    if (!isObject(payload)) return { valid: false, errors: ['$: decision envelope must be an object'] };
    addUnknownKeyErrors(payload, ENVELOPE_KEYS, '$', errors);
    const legacyAllowed = options.allowLegacy === true
      && constants.legacyDecisionSchemaVersions.includes(payload.schemaVersion);
    if (payload.schemaVersion !== constants.decisionSchemaVersion && !legacyAllowed) {
      errors.push('$.schemaVersion: unsupported schema version');
    }
    if (payload.kind !== constants.decisionExportKind) errors.push('$.kind: invalid export kind');
    if (!nonBlank(payload.reviewId)) errors.push('$.reviewId: required');
    if (!isSha256(payload.comparisonHash)) errors.push('$.comparisonHash: a SHA-256 digest is required');
    if (!isSha256(payload.frozenContractHash)) errors.push('$.frozenContractHash: a SHA-256 digest is required');
    validateHashTree(payload.sourceHashes, '$.sourceHashes', errors);
    if (typeof payload.exportedAt !== 'string' || Number.isNaN(Date.parse(payload.exportedAt))) {
      errors.push('$.exportedAt: an ISO date-time is required');
    }
    if (!['ALL', 'COMPLETE_ONLY'].includes(payload.exportScope)) errors.push('$.exportScope: invalid scope');
    if (payload.schemaVersion === constants.decisionSchemaVersion && !Object.hasOwn(payload, 'schemaPolicy')) {
      errors.push('$.schemaPolicy: required in decision schema v3');
    }
    if (payload.schemaPolicy !== undefined && payload.schemaPolicy !== null && !isObject(payload.schemaPolicy)) {
      errors.push('$.schemaPolicy: must be an object or null');
    }
    if (!isObject(payload.entries)) errors.push('$.entries: must be an object');
    else {
      for (const [id, entry] of Object.entries(payload.entries)) {
        if (legacyAllowed) validateLegacyEntryShape(entry, `$.entries.${id}`, errors);
        else validateEntryShape(entry, `$.entries.${id}`, errors);
      }
    }
    return { valid: errors.length === 0, errors };
  }

  function validateImport(report, payload) {
    const shape = validateEnvelopeShape(payload);
    const errors = [...shape.errors];
    const warnings = [];
    if (!report || !isObject(report)) errors.push('report: governed oracle is required');
    if (payload?.reviewId !== (report?.reviewId ?? constants.reviewId)) errors.push('$.reviewId: does not match the governed report');
    if (payload?.comparisonHash !== report?.comparisonHash) errors.push('$.comparisonHash: stale comparison hash');
    if (payload?.frozenContractHash !== (report?.frozenContractHash ?? constants.frozenContractHash)) {
      errors.push('$.frozenContractHash: frozen contract mismatch');
    }
    if (!sameValue(payload?.sourceHashes, report?.sourceHashes)) errors.push('$.sourceHashes: governed source hashes do not match');
    if (report?.schemaOrientation) {
      if (!payload?.schemaPolicy) errors.push('$.schemaPolicy: governed Phase 0 policy envelope is required');
      else {
        const policyValidation = policyRuntime.validatePolicyEnvelope(report.schemaOrientation, payload.schemaPolicy);
        errors.push(...policyValidation.errors.map((error) => `$.schemaPolicy${error.startsWith('$') ? error.slice(1) : `: ${error}`}`));
      }
    } else if (payload?.schemaPolicy !== null) {
      errors.push('$.schemaPolicy: must be null when the report has no governed schema orientation');
    }

    const skills = skillMap(report);
    for (const [id, entry] of Object.entries(payload?.entries ?? {})) {
      const skill = skills.get(id);
      if (!skill) {
        errors.push(`$.entries.${id}: unknown stableId`);
        continue;
      }
      if (isReadOnlySkill(skill)) {
        errors.push(`$.entries.${id}: decisions are forbidden for a read-only identical skill`);
        continue;
      }
      if (entry.fingerprint !== skill.fingerprint) errors.push(`$.entries.${id}.fingerprint: stale fingerprint`);
      const bundles = bundleMap(skill);
      const fields = fieldMap(skill);
      for (const [bundleId, choice] of Object.entries(entry.bundleDecisions ?? {})) {
        const bundle = bundles.get(bundleId);
        if (!bundle) {
          errors.push(`$.entries.${id}.bundleDecisions.${bundleId}: unknown bundle`);
          continue;
        }
        if (bundle.scope === 'TECHNICAL' || bundle.manualDecisionRequired === false) {
          errors.push(`$.entries.${id}.bundleDecisions.${bundleId}: technical packages are auto-resolved and cannot carry a manual decision`);
          continue;
        }
        const choiceValidation = validateChoice(choice, {
          bundle,
          path: `$.entries.${id}.bundleDecisions.${bundleId}`,
        });
        if (payload.exportScope === 'COMPLETE_ONLY') errors.push(...choiceValidation.errors);
      }
      for (const [fieldId, choice] of Object.entries(entry.fieldDecisions ?? {})) {
        const known = fields.get(fieldId);
        if (!known) {
          errors.push(`$.entries.${id}.fieldDecisions.${fieldId}: unknown field`);
          continue;
        }
        const choiceValidation = validateChoice(choice, {
          field: known.field,
          component: known.component,
          expert: true,
          path: `$.entries.${id}.fieldDecisions.${fieldId}`,
        });
        if (payload.exportScope === 'COMPLETE_ONLY') errors.push(...choiceValidation.errors);
      }
      if (isNewPlayerSkill(skill) && entry.newSkillLineDecision === undefined) {
        errors.push(`$.entries.${id}.newSkillLineDecision: required for a new PD2 player skill`);
      }
      if (!isNewPlayerSkill(skill) && entry.newSkillLineDecision !== undefined) {
        errors.push(`$.entries.${id}.newSkillLineDecision: forbidden for an existing skill`);
      }
      const state = entryState(report, skill, entry, payload.schemaPolicy);
      if (payload.exportScope === 'COMPLETE_ONLY' && !state.complete) {
        errors.push(`$.entries.${id}: COMPLETE_ONLY contains an incomplete decision (${state.reasons.join('; ')})`);
      } else if (!state.complete) {
        warnings.push({ stableId: id, reasons: state.reasons });
      }
    }
    return {
      valid: errors.length === 0,
      errors: [...new Set(errors)],
      warnings,
      envelope: errors.length === 0 ? clone(payload) : null,
    };
  }

  function assertValidImport(report, payload) {
    const validation = validateImport(report, payload);
    if (!validation.valid) throw new Error(validation.errors.join('\n'));
    return validation.envelope;
  }

  function exportEnvelope(report, entries, options = {}) {
    const scope = options.scope ?? 'ALL';
    if (!['ALL', 'COMPLETE_ONLY'].includes(scope)) throw new Error(`Unknown export scope ${scope}`);
    const source = entries?.entries ?? entries ?? {};
    const governedSchemaPolicy = options.schemaPolicy ?? entries?.schemaPolicy ?? null;
    const output = {};
    const skills = skillMap(report);
    for (const skill of report.skills ?? []) {
      if (isReadOnlySkill(skill)) continue;
      const id = skill.stableId;
      const raw = source[id];
      if (raw === undefined) {
        if (scope === 'ALL') output[id] = createEntry(skill);
        continue;
      }
      const entry = normalizeEntry(skill, raw);
      if (scope === 'COMPLETE_ONLY' && !entryState(report, skill, entry, governedSchemaPolicy).complete) continue;
      output[id] = entry;
    }
    const envelope = {
      schemaVersion: constants.decisionSchemaVersion,
      kind: constants.decisionExportKind,
      reviewId: report.reviewId ?? constants.reviewId,
      comparisonHash: report.comparisonHash,
      frozenContractHash: report.frozenContractHash ?? constants.frozenContractHash,
      sourceHashes: clone(report.sourceHashes ?? {}),
      exportedAt: options.exportedAt ?? new Date().toISOString(),
      exportScope: scope,
      schemaPolicy: clone(governedSchemaPolicy),
      entries: output,
    };
    const validation = validateImport(report, envelope);
    if (!validation.valid) throw new Error(`Cannot export invalid decisions:\n${validation.errors.join('\n')}`);
    return envelope;
  }

  function sameStringSet(left, right) {
    const a = [...new Set(left ?? [])].sort();
    const b = [...new Set(right ?? [])].sort();
    return a.length === b.length && a.every((value, index) => value === b[index]);
  }

  function migrateLegacyEntry(skill, raw) {
    const conflicts = [];
    const transformations = [];
    const candidate = createEntry(skill);
    candidate.globalDecision = raw.globalDecision ?? null;
    candidate.implementationStatus = raw.implementationStatus ?? 'NOT_REVIEWED';
    candidate.notes = { ...EMPTY_NOTES, ...(clone(raw.notes) ?? {}) };
    if (isNewPlayerSkill(skill)) candidate.newSkillLineDecision = raw.newSkillLineDecision ?? null;

    for (const fieldId of Object.keys(raw.fieldDecisions ?? {})) {
      conflicts.push({
        code: 'LEGACY_FIELD_DECISION_REQUIRES_EXPERT_RECONFIRMATION',
        fieldId,
        reason: 'v1/v2 field decisions cannot become v3 expert overrides without explicit enabled=true and a new justification',
      });
    }

    const components = componentMap(skill);
    const bundles = [...bundleMap(skill).values()];
    for (const [componentId, choice] of Object.entries(raw.componentDecisions ?? {})) {
      const component = components.get(componentId);
      if (!component) {
        conflicts.push({ code: 'LEGACY_COMPONENT_REMOVED', componentId });
        continue;
      }
      if (choice?.decision === 'CUSTOM') {
        conflicts.push({
          code: 'LEGACY_CUSTOM_REQUIRES_BUNDLE_VALUE_MAP',
          componentId,
          reason: 'A scalar legacy CUSTOM value cannot be silently projected onto an atomic bundle',
        });
        continue;
      }
      const changedFieldIds = (component.fields ?? [])
        .filter((field) => field.decisionRelevant !== false && (field.changed !== false || field.semanticChanged === true || field.rawChanged === true))
        .map((field) => field.id);
      const matches = bundles.filter((bundle) => (
        bundle.scope !== 'TECHNICAL'
        && bundle.manualDecisionRequired !== false
        && (bundle.id === componentId
          || bundle.componentId === componentId
          || sameStringSet(bundle.fieldIds, changedFieldIds))
      ));
      if (matches.length !== 1) {
        conflicts.push({
          code: matches.length === 0 ? 'LEGACY_COMPONENT_HAS_NO_EXACT_BUNDLE' : 'LEGACY_COMPONENT_BUNDLE_AMBIGUOUS',
          componentId,
          candidateBundleIds: matches.map((bundle) => bundle.id),
        });
        continue;
      }
      candidate.bundleDecisions[matches[0].id] = clone(choice);
      transformations.push({
        code: 'LEGACY_COMPONENT_TO_EXACT_BUNDLE',
        from: componentId,
        to: matches[0].id,
      });
    }
    return { candidate, conflicts, transformations };
  }

  function migrateEnvelope(report, previous, options = {}) {
    const shape = validateEnvelopeShape(previous, { allowLegacy: true });
    if (!shape.valid) throw new Error(`Cannot migrate an invalid decision envelope:\n${shape.errors.join('\n')}`);
    if (previous.reviewId !== (report.reviewId ?? constants.reviewId)) {
      throw new Error('Cannot migrate decisions from another reviewId');
    }
    const legacySource = constants.legacyDecisionSchemaVersions.includes(previous.schemaVersion);
    let migratedSchemaPolicy = null;
    let policyMigration = null;
    if (report.schemaOrientation) {
      if (report.schemaPolicy?.envelope) {
        migratedSchemaPolicy = clone(report.schemaPolicy.envelope);
        policyMigration = {
          envelope: migratedSchemaPolicy,
          report: {
            reason: 'CANONICAL_PROJECT_POLICY_SEEDED',
            fromOrientationHash: previous.schemaPolicy?.orientationHash ?? null,
            toOrientationHash: report.schemaOrientation.orientationHash,
          },
        };
      } else if (previous.schemaPolicy) {
        policyMigration = policyRuntime.migratePolicyEnvelope(report.schemaOrientation, previous.schemaPolicy, {
          exportedAt: options.exportedAt,
        });
        migratedSchemaPolicy = policyMigration.envelope;
      } else {
        migratedSchemaPolicy = policyRuntime.createPolicyEnvelope(report.schemaOrientation, {
          exportedAt: options.exportedAt,
        });
        policyMigration = {
          envelope: migratedSchemaPolicy,
          report: {
            fromOrientationHash: null,
            toOrientationHash: report.schemaOrientation.orientationHash,
            retained: [],
            stale: [],
            dropped: [],
            counts: { retained: 0, stale: 0, dropped: 0 },
            reason: legacySource ? 'LEGACY_ENVELOPE_HAS_NO_SCHEMA_POLICY' : 'SCHEMA_POLICY_MISSING',
          },
        };
      }
    }
    const currentSkills = skillMap(report);
    const retained = [];
    const stale = [];
    const dropped = [];
    const conflicts = [];
    const transformations = [];
    const entries = {};
    for (const [stableId, raw] of Object.entries(previous.entries ?? {})) {
      const skill = currentSkills.get(stableId);
      if (!skill) {
        dropped.push({ stableId, reason: 'STABLE_ID_REMOVED' });
        continue;
      }
      if (isReadOnlySkill(skill)) {
        dropped.push({ stableId, reason: 'NOW_READ_ONLY' });
        continue;
      }
      if (raw.fingerprint !== skill.fingerprint) {
        stale.push({
          stableId,
          reason: 'FINGERPRINT_CHANGED',
          previousFingerprint: raw.fingerprint,
          currentFingerprint: skill.fingerprint,
        });
        continue;
      }
      let candidate;
      if (legacySource) {
        const migration = migrateLegacyEntry(skill, raw);
        candidate = migration.candidate;
        transformations.push(...migration.transformations.map((item) => ({ stableId, ...item })));
        if (migration.conflicts.length > 0) {
          const conflict = {
            stableId,
            reason: 'LEGACY_DECISION_REQUIRES_REVIEW',
            conflicts: migration.conflicts,
          };
          conflicts.push(conflict);
          stale.push(conflict);
          continue;
        }
      } else candidate = normalizeEntry(skill, raw);
      const scoped = createEmptyEnvelope(report);
      scoped.schemaPolicy = clone(migratedSchemaPolicy);
      scoped.entries = { [stableId]: candidate };
      const validation = validateImport(report, scoped);
      if (!validation.valid) {
        stale.push({ stableId, reason: 'DECISION_SHAPE_NO_LONGER_COMPATIBLE', errors: validation.errors });
        continue;
      }
      entries[stableId] = candidate;
      retained.push({ stableId, fingerprint: skill.fingerprint });
    }
    const fresh = createEmptyEnvelope(report);
    fresh.schemaPolicy = clone(migratedSchemaPolicy);
    Object.assign(fresh.entries, entries);
    const envelope = exportEnvelope(report, fresh, {
      scope: 'ALL',
      schemaPolicy: migratedSchemaPolicy,
    });
    return {
      envelope,
      report: {
        fromSchemaVersion: previous.schemaVersion,
        toSchemaVersion: constants.decisionSchemaVersion,
        fromComparisonHash: previous.comparisonHash,
        toComparisonHash: report.comparisonHash,
        fromFrozenContractHash: previous.frozenContractHash,
        toFrozenContractHash: report.frozenContractHash ?? constants.frozenContractHash,
        policyMigration: policyMigration?.report ?? null,
        retained,
        stale,
        dropped,
        conflicts,
        transformations,
        counts: {
          retained: retained.length,
          stale: stale.length,
          dropped: dropped.length,
          conflicts: conflicts.length,
          transformations: transformations.length,
        },
      },
    };
  }

  return Object.freeze({
    constants: Object.freeze(clone(constants)),
    createEntry,
    createEmptyEnvelope,
    storageKey,
    legacyStorageKeys,
    schemaPolicyGate,
    validateChoice,
    validateChoiceShape,
    resolveBundleChoice,
    resolveFieldChoice,
    projectProposedResult,
    entryState,
    applyBulk,
    progress,
    validateEnvelopeShape,
    validateImport,
    assertValidImport,
    exportEnvelope,
    migrateEnvelope,
  });
}

const runtime = decisionRuntimeFactory(RUNTIME_CONSTANTS, schemaPolicyRuntime);

export const createEntry = runtime.createEntry;
export const createEmptyEnvelope = runtime.createEmptyEnvelope;
export const storageKey = runtime.storageKey;
export const legacyStorageKeys = runtime.legacyStorageKeys;
export const schemaPolicyGate = runtime.schemaPolicyGate;
export const validateChoice = runtime.validateChoice;
export const validateChoiceShape = runtime.validateChoiceShape;
export const resolveBundleChoice = runtime.resolveBundleChoice;
export const resolveFieldChoice = runtime.resolveFieldChoice;
export const projectProposedResult = runtime.projectProposedResult;
export const entryState = runtime.entryState;
export const applyBulk = runtime.applyBulk;
export const progress = runtime.progress;
export const validateEnvelopeShape = runtime.validateEnvelopeShape;
export const validateImport = runtime.validateImport;
export const assertValidImport = runtime.assertValidImport;
export const exportEnvelope = runtime.exportEnvelope;
export const migrateEnvelope = runtime.migrateEnvelope;

export function buildBrowserRuntimeSource() {
  return `${buildBrowserPolicyRuntimeSource()};globalThis.decisionRuntime=(${decisionRuntimeFactory.toString()})(${JSON.stringify(RUNTIME_CONSTANTS)},globalThis.schemaPolicyRuntime);`;
}

export const browserRuntimeSource = buildBrowserRuntimeSource();
