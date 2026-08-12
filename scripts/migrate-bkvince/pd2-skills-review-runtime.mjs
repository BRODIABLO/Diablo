import {
  COMPONENT_DECISIONS,
  DECISION_EXPORT_KIND,
  DECISION_SCHEMA_VERSION,
  FROZEN_CONTRACT_HASH,
  GLOBAL_DECISIONS,
  IMPLEMENTATION_STATUSES,
  NEW_SKILL_LINE_DECISIONS,
  PROOF_STATUSES,
  REVIEW_ID,
  STORAGE_KEY_PREFIX,
} from './pd2-skills-review-contracts.mjs';

const RUNTIME_CONSTANTS = Object.freeze({
  componentDecisions: COMPONENT_DECISIONS,
  decisionExportKind: DECISION_EXPORT_KIND,
  decisionSchemaVersion: DECISION_SCHEMA_VERSION,
  frozenContractHash: FROZEN_CONTRACT_HASH,
  globalDecisions: GLOBAL_DECISIONS,
  implementationStatuses: IMPLEMENTATION_STATUSES,
  newSkillLineDecisions: NEW_SKILL_LINE_DECISIONS,
  proofStatuses: PROOF_STATUSES,
  reviewId: REVIEW_ID,
  storageKeyPrefix: STORAGE_KEY_PREFIX,
});

function decisionRuntimeFactory(constants) {
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
    'componentDecisions',
    'fieldDecisions',
    'notes',
  ]);
  const CHOICE_KEYS = new Set([
    'decision',
    'customValue',
    'justification',
    'gameplayObjective',
    'testPlan',
    'protectedOverride',
  ]);
  const OVERRIDE_KEYS = new Set([
    'approved',
    'justification',
    'acknowledgedProofStatus',
    'nativeRiskAccepted',
    'malformedResolution',
  ]);
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

  function fieldMap(skill) {
    const result = new Map();
    for (const component of skill?.components ?? []) {
      for (const field of component.fields ?? []) {
        result.set(field.id, { component, field });
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
      componentDecisions: {},
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

  function validateChoice(choice, { field = null, component = null, path = 'choice' } = {}) {
    const errors = [];
    if (!isObject(choice)) return { valid: false, errors: [`${path}: choice must be an object`] };
    addUnknownKeyErrors(choice, CHOICE_KEYS, path, errors);
    if (!constants.componentDecisions.includes(choice.decision)) {
      errors.push(`${path}.decision: unknown component/field decision`);
      return { valid: false, errors };
    }
    if (choice.decision === 'CUSTOM') {
      if (!nonBlank(choice.customValue)) errors.push(`${path}.customValue: CUSTOM requires an explicit value or formula`);
      if (!nonBlank(choice.justification)) errors.push(`${path}.justification: CUSTOM requires a justification`);
      if (!nonBlank(choice.testPlan)) errors.push(`${path}.testPlan: CUSTOM requires a test plan`);
    } else if (choice.customValue !== undefined) {
      errors.push(`${path}.customValue: is permitted only for CUSTOM`);
    }
    const requiresOverride = field?.protected === true
      && (choice.decision === 'ADOPT_PD2' || choice.decision === 'CUSTOM');
    if (requiresOverride) {
      errors.push(...validateOverride(choice.protectedOverride, field, component, `${path}.protectedOverride`));
    } else if (field && choice.protectedOverride !== undefined) {
      errors.push(`${path}.protectedOverride: override is allowed only when adopting or customizing a protected field`);
    }
    return { valid: errors.length === 0, errors };
  }

  function validateChoiceShape(choice, path = 'choice') {
    const errors = [];
    if (!isObject(choice)) return { valid: false, errors: [`${path}: choice must be an object`] };
    addUnknownKeyErrors(choice, CHOICE_KEYS, path, errors);
    if (!constants.componentDecisions.includes(choice.decision)) errors.push(`${path}.decision: unknown component/field decision`);
    for (const key of ['customValue', 'justification', 'gameplayObjective', 'testPlan']) {
      if (choice[key] !== undefined && typeof choice[key] !== 'string') errors.push(`${path}.${key}: must be a string`);
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
    return { valid: errors.length === 0, errors };
  }

  function resolveFieldChoice(skill, entry, componentOrId, fieldOrId) {
    const components = componentMap(skill);
    const fields = fieldMap(skill);
    const component = typeof componentOrId === 'string' ? components.get(componentOrId) : componentOrId;
    const found = typeof fieldOrId === 'string' ? fields.get(fieldOrId) : null;
    const field = found?.field ?? fieldOrId;
    const owner = found?.component ?? component;
    if (!field || !owner) return null;
    return entry?.fieldDecisions?.[field.id] ?? entry?.componentDecisions?.[owner.id] ?? null;
  }

  function shouldRequireDetails(skill, entry) {
    if (!entry?.globalDecision) return true;
    if (['REJECT_PD2', 'DEFER_NATIVE_PROOF', 'DISCUSS'].includes(entry.globalDecision)) return false;
    if (isNewPlayerSkill(skill)) {
      return ['IMPORT_APPEND_ONLY', 'IMPORT_CUSTOMIZED'].includes(entry.newSkillLineDecision);
    }
    return true;
  }

  function entryState(report, skillOrId, entry) {
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
    let customFields = 0;
    for (const component of skill.components ?? []) {
      const changedFields = (component.fields ?? []).filter((field) => field.changed !== false);
      const componentRequired = detailsRequired && component.changed !== false
        && (changedFields.length > 0 || component.changed === true);
      const state = {
        id: component.id,
        required: componentRequired,
        complete: !componentRequired,
        fields: [],
        reasons: [],
      };

      if (componentRequired && changedFields.length === 0) {
        total += 1;
        const choice = current.componentDecisions?.[component.id];
        const validation = validateChoice(choice, { component, path: `componentDecisions.${component.id}` });
        if (validation.valid && choice.decision !== 'DISCUSS') {
          completed += 1;
          state.complete = true;
        } else {
          const choiceErrors = choice?.decision === 'DISCUSS'
            ? [`componentDecisions.${component.id}: decision remains DISCUSS`]
            : validation.errors;
          state.reasons.push(...choiceErrors);
          reasons.push(...choiceErrors);
        }
      }

      for (const field of changedFields) {
        const required = componentRequired;
        const choice = resolveFieldChoice(skill, current, component, field);
        const validation = required
          ? validateChoice(choice, { field, component, path: `fieldDecisions.${field.id}` })
          : { valid: true, errors: [] };
        const completeChoice = validation.valid && choice?.decision !== 'DISCUSS';
        if (required) {
          total += 1;
          if (completeChoice) completed += 1;
          else {
            const choiceErrors = choice?.decision === 'DISCUSS'
              ? [`fieldDecisions.${field.id}: decision remains DISCUSS`]
              : validation.errors;
            state.reasons.push(...choiceErrors);
            reasons.push(...choiceErrors);
          }
        }
        if (choice?.decision === 'CUSTOM') customFields += 1;
        const derivedField = newSkillDerivedField(skill, field);
        if (required && isNewPlayerSkill(skill)
          && ['IMPORT_APPEND_ONLY', 'IMPORT_CUSTOMIZED'].includes(current.newSkillLineDecision)
          && (choice?.decision === 'KEEP_BKVINCE'
            || (choice?.decision === 'NOT_APPLICABLE' && !derivedField))) {
          const choiceError = `fieldDecisions.${field.id}: ${choice.decision} is invalid because no BKVince row exists for an imported new skill`;
          if (completeChoice) completed -= 1;
          state.reasons.push(choiceError);
          reasons.push(choiceError);
        }
        if (required && derivedField && choice?.decision === 'ADOPT_PD2') {
          const choiceError = `fieldDecisions.${field.id}: a derived documentary target value cannot adopt the PD2 source ordinal`;
          if (completeChoice) completed -= 1;
          state.reasons.push(choiceError);
          reasons.push(choiceError);
        }
        if (required && derivedField && choice?.decision === 'CUSTOM'
          && String(choice.customValue) !== String(derivedField.proposal.targetOrdinal)) {
          const choiceError = `fieldDecisions.${field.id}: documentary Id must remain equal to proposed append ordinal ${derivedField.proposal.targetOrdinal}`;
          if (completeChoice) completed -= 1;
          state.reasons.push(choiceError);
          reasons.push(choiceError);
        }
        state.fields.push({
          id: field.id,
          required,
          complete: !required || (completeChoice && !(isNewPlayerSkill(skill)
            && ['IMPORT_APPEND_ONLY', 'IMPORT_CUSTOMIZED'].includes(current.newSkillLineDecision)
            && (choice?.decision === 'KEEP_BKVINCE'
              || (choice?.decision === 'NOT_APPLICABLE' && !derivedField)))
            && !(derivedField && choice?.decision === 'ADOPT_PD2')
            && !(derivedField && choice?.decision === 'CUSTOM'
              && String(choice.customValue) !== String(derivedField.proposal.targetOrdinal))),
          effectiveChoice: clone(choice),
          source: current.fieldDecisions?.[field.id] ? 'FIELD' : choice ? 'COMPONENT' : null,
          errors: validation.errors,
        });
      }
      if (componentRequired && changedFields.length > 0) {
        state.complete = state.fields.every((field) => field.complete);
      }
      componentStates.push(state);
    }

    if (isNewPlayerSkill(skill) && current.newSkillLineDecision === 'IMPORT_APPEND_ONLY' && customFields > 0) {
      reasons.push('IMPORT_APPEND_ONLY cannot contain CUSTOM field decisions; use IMPORT_CUSTOMIZED');
    }
    if (isNewPlayerSkill(skill) && current.newSkillLineDecision === 'IMPORT_CUSTOMIZED' && customFields === 0) {
      reasons.push('IMPORT_CUSTOMIZED requires at least one governed CUSTOM field');
    }

    const maps = [
      ['componentDecisions', componentMap(skill)],
      ['fieldDecisions', fieldMap(skill)],
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
      components: componentStates,
    };
  }

  function normalizeEntry(skill, value) {
    const base = createEntry(skill);
    if (!isObject(value)) return base;
    return {
      ...base,
      ...clone(value),
      componentDecisions: clone(value.componentDecisions ?? {}),
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
        for (const key of ['componentDecisions', 'fieldDecisions']) {
          for (const [decisionId, choice] of Object.entries(current[key])) {
            if (choice?.decision === 'DISCUSS') delete current[key][decisionId];
          }
        }
        result[id] = current;
        continue;
      }

      if (replace) {
        current.componentDecisions = {};
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

      for (const component of skill.components ?? []) {
        if (component.changed === false) continue;
        const componentChoice = action === 'KEEP_BKVINCE'
          ? { decision: 'KEEP_BKVINCE' }
          : action === 'DISCUSS'
            ? { decision: 'DISCUSS' }
            : { decision: 'ADOPT_PD2' };
        setIfOpen(current.componentDecisions, component.id, componentChoice);

        if (action === 'ADOPT_PD2') {
          for (const field of component.fields ?? []) {
            if (field.changed === false) continue;
            if (field.protected === true) {
              setIfOpen(current.fieldDecisions, field.id, {
                decision: isNewPlayerSkill(skill) && newSkillDerivedField(skill, field)
                  ? 'NOT_APPLICABLE'
                  : 'KEEP_BKVINCE',
              });
            }
          }
        }
      }
      result[id] = current;
    }
    return result;
  }

  function progress(report, entries, classCode = null) {
    const source = entries?.entries ?? entries ?? {};
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
      const state = entryState(report, skill, source[skill.stableId]);
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
    for (const key of ['componentDecisions', 'fieldDecisions']) {
      if (!isObject(entry[key])) {
        errors.push(`${path}.${key}: must be an object`);
        continue;
      }
      for (const [id, choice] of Object.entries(entry[key])) {
        const validation = validateChoiceShape(choice, `${path}.${key}.${id}`);
        errors.push(...validation.errors);
      }
    }
    validateNotes(entry.notes, `${path}.notes`, errors);
  }

  function validateEnvelopeShape(payload) {
    const errors = [];
    if (!isObject(payload)) return { valid: false, errors: ['$: decision envelope must be an object'] };
    addUnknownKeyErrors(payload, ENVELOPE_KEYS, '$', errors);
    if (payload.schemaVersion !== constants.decisionSchemaVersion) errors.push('$.schemaVersion: unsupported schema version');
    if (payload.kind !== constants.decisionExportKind) errors.push('$.kind: invalid export kind');
    if (!nonBlank(payload.reviewId)) errors.push('$.reviewId: required');
    if (!isSha256(payload.comparisonHash)) errors.push('$.comparisonHash: a SHA-256 digest is required');
    if (!isSha256(payload.frozenContractHash)) errors.push('$.frozenContractHash: a SHA-256 digest is required');
    validateHashTree(payload.sourceHashes, '$.sourceHashes', errors);
    if (typeof payload.exportedAt !== 'string' || Number.isNaN(Date.parse(payload.exportedAt))) {
      errors.push('$.exportedAt: an ISO date-time is required');
    }
    if (!['ALL', 'COMPLETE_ONLY'].includes(payload.exportScope)) errors.push('$.exportScope: invalid scope');
    if (!isObject(payload.entries)) errors.push('$.entries: must be an object');
    else {
      for (const [id, entry] of Object.entries(payload.entries)) validateEntryShape(entry, `$.entries.${id}`, errors);
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
      const components = componentMap(skill);
      const fields = fieldMap(skill);
      for (const [componentId, choice] of Object.entries(entry.componentDecisions ?? {})) {
        const component = components.get(componentId);
        if (!component) {
          errors.push(`$.entries.${id}.componentDecisions.${componentId}: unknown component`);
          continue;
        }
        const choiceValidation = validateChoice(choice, {
          component,
          path: `$.entries.${id}.componentDecisions.${componentId}`,
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
      const state = entryState(report, skill, entry);
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
      if (scope === 'COMPLETE_ONLY' && !entryState(report, skill, entry).complete) continue;
      output[id] = entry;
    }
    const envelope = {
      schemaVersion: constants.decisionSchemaVersion,
      kind: constants.decisionExportKind,
      reviewId: report.reviewId ?? constants.reviewId,
      comparisonHash: report.comparisonHash,
      frozenContractHash: report.frozenContractHash ?? constants.frozenContractHash,
      sourceHashes: clone(report.sourceHashes ?? {}),
      exportedAt: new Date().toISOString(),
      exportScope: scope,
      entries: output,
    };
    const validation = validateImport(report, envelope);
    if (!validation.valid) throw new Error(`Cannot export invalid decisions:\n${validation.errors.join('\n')}`);
    return envelope;
  }

  function migrateEnvelope(report, previous) {
    const shape = validateEnvelopeShape(previous);
    if (!shape.valid) throw new Error(`Cannot migrate an invalid decision envelope:\n${shape.errors.join('\n')}`);
    if (previous.reviewId !== (report.reviewId ?? constants.reviewId)) {
      throw new Error('Cannot migrate decisions from another reviewId');
    }
    if (previous.frozenContractHash !== (report.frozenContractHash ?? constants.frozenContractHash)) {
      throw new Error('Cannot migrate decisions governed by another frozen contract');
    }
    const currentSkills = skillMap(report);
    const retained = [];
    const stale = [];
    const dropped = [];
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
      const candidate = normalizeEntry(skill, raw);
      const scoped = createEmptyEnvelope(report);
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
    Object.assign(fresh.entries, entries);
    const envelope = exportEnvelope(report, fresh.entries, { scope: 'ALL' });
    return {
      envelope,
      report: {
        fromComparisonHash: previous.comparisonHash,
        toComparisonHash: report.comparisonHash,
        retained,
        stale,
        dropped,
        counts: { retained: retained.length, stale: stale.length, dropped: dropped.length },
      },
    };
  }

  return Object.freeze({
    constants: Object.freeze(clone(constants)),
    createEntry,
    createEmptyEnvelope,
    storageKey,
    validateChoice,
    validateChoiceShape,
    resolveFieldChoice,
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

const runtime = decisionRuntimeFactory(RUNTIME_CONSTANTS);

export const createEntry = runtime.createEntry;
export const createEmptyEnvelope = runtime.createEmptyEnvelope;
export const storageKey = runtime.storageKey;
export const validateChoice = runtime.validateChoice;
export const validateChoiceShape = runtime.validateChoiceShape;
export const resolveFieldChoice = runtime.resolveFieldChoice;
export const entryState = runtime.entryState;
export const applyBulk = runtime.applyBulk;
export const progress = runtime.progress;
export const validateEnvelopeShape = runtime.validateEnvelopeShape;
export const validateImport = runtime.validateImport;
export const assertValidImport = runtime.assertValidImport;
export const exportEnvelope = runtime.exportEnvelope;
export const migrateEnvelope = runtime.migrateEnvelope;

export function buildBrowserRuntimeSource() {
  return `;globalThis.decisionRuntime=(${decisionRuntimeFactory.toString()})(${JSON.stringify(RUNTIME_CONSTANTS)});`;
}

export const browserRuntimeSource = buildBrowserRuntimeSource();
