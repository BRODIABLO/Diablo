import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';

import {
  DECISION_BUNDLES,
  SOURCE_KEYS,
  bundleForHeader,
  canonicalFieldHeader,
  rawValueState,
  semanticBlank,
} from './pd2-skills-schema-orientation-contracts.mjs';
import {
  migratePolicyEnvelope,
  policyGate,
  validatePolicyEnvelope,
} from './pd2-skills-schema-policy-runtime.mjs';
import { PHASE1_DECISION_MODEL, sha256Canonical } from './pd2-skills-review-contracts.mjs';
import { repoRoot } from './pd2-skills-review-data.mjs';

export const PHASE1_MODEL_ID = PHASE1_DECISION_MODEL;
export const APPROVED_POLICY_EXPORT_PATH = path.join(repoRoot, 'Mission', 'pd2-skills-schema-policy-approved.json');
export const CANONICAL_POLICY_PATH = path.join(repoRoot, 'Mission', 'pd2-skills-schema-policy.json');
export const APPROVED_POLICY_EXPORT_SHA256 = '35EF03B53536DC8D0835716D675DAE9E701A1E6CD6B4A0DE1F19EEE871978A96';

const APPROVED_NATIVE_EVIDENCE_DRIFT = Object.freeze({
  'references.nativeFindings': Object.freeze({
    previous: '0768EF47A41793AB5C1D30C06F64A79A794870AAD0E835628385DBE646587C3D',
    current: '7A4E737FECA37F8213C31F808113E0321AC55EE7252B891F5F982CD556C67131',
  }),
  'references.knownRvas': Object.freeze({
    previous: '1FA2981D14D3BA2B8F4F6E8ECF6CFDE5EE8D6E24AC742E8838B65FA46F31E086',
    current: 'F19DB68098A5CD5F3568305FB6C12F9CA48374FFBFBF856099DB8510027CD26A',
  }),
});

const TECHNICAL_BUNDLES = new Set(['ITEM_TRIGGER_EXECUTION', 'NATIVE_EXECUTION']);
const ITEM_ECONOMY_PATTERN = /^cost (?:add|mult)$/i;
const NATIVE_PATTERN = /^(?:(?:srv|clt|p?srv|p?clt).*(?:func|function)\d*|(?:p?srv|p?clt)?hitfunc\d*|item(?:clt)?effect|checkfunc)$/i;
const NATIVE_PARAMETER_PATTERN = /^(?:s|c|d|p)?(?:hit|dmg|clt|srv)par\d+$/i;
const MISSILE_PHYSICS_PATTERN = /^(?:vel|maxvel|range|size|collidekill|pierce|nexthit|nextdelay)$/i;
const PD2_ONLY_HEADERS = new Set(['auratgtevent', 'auratgteventfunc', 'checkfunc', 'delay', 'general', 'nocostinstate', 'passiveevent', 'passiveeventfunc']);

const LINKED_LABELS_FR = Object.freeze({
  vel: 'Vitesse du projectile',
  maxvel: 'Vitesse maximale du projectile',
  range: 'Portée du projectile',
  size: 'Taille de la hitbox',
  collidekill: 'Destruction à la collision',
  pierce: 'Perforation du projectile',
  nexthit: 'Next Hit Delay',
  nextdelay: 'Délai entre impacts',
});

function clone(value) {
  return JSON.parse(JSON.stringify(value));
}

function sha256Buffer(value) {
  return crypto.createHash('sha256').update(value).digest('hex').toUpperCase();
}

function normalize(value) {
  return String(value ?? '').trim().toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '');
}

function rawPolicy(filePath) {
  const raw = fs.readFileSync(filePath);
  return { raw, document: JSON.parse(raw.toString('utf8').replace(/^\uFEFF/, '')) };
}

function sameCanonical(left, right) {
  return sha256Canonical(left) === sha256Canonical(right);
}

function classifySourceDrift(previous, current) {
  const changed = [];
  const visit = (left, right, prefix = '') => {
    for (const key of new Set([...Object.keys(left ?? {}), ...Object.keys(right ?? {})])) {
      const pathKey = prefix ? `${prefix}.${key}` : key;
      if (left?.[key] && typeof left[key] === 'object' && right?.[key] && typeof right[key] === 'object') {
        visit(left[key], right[key], pathKey);
      } else if (left?.[key] !== right?.[key]) {
        changed.push({ path: pathKey, previous: left?.[key] ?? null, current: right?.[key] ?? null });
      }
    }
  };
  visit(previous, current);
  if (changed.length === 0) return { classification: 'IDENTICAL_GOVERNED_SOURCE_HASHES', changed };
  if (changed.every((entry) => {
    const approved = APPROVED_NATIVE_EVIDENCE_DRIFT[entry.path];
    return approved?.previous === entry.previous && approved.current === entry.current;
  })) {
    return {
      classification: 'AUDITED_UNRELATED_NATIVE_EVIDENCE_DRIFT',
      changed,
      reason: 'Only governed reverse-engineering evidence hashes changed; the three Skills.txt sources, schema documentation, analytical audit, policy definitions, and policy fingerprints are unchanged.',
    };
  }
  return {
    classification: 'UNAPPROVED_SOURCE_DRIFT',
    changed,
    reason: 'A gameplay/schema/audit source changed outside the approved native-evidence-only migration allowance.',
  };
}

export function validateAndMigrateApprovedPolicy(orientation, approvedEnvelope, options = {}) {
  const approvalOrientation = options.approvalOrientation ?? {
    ...orientation,
    orientationId: approvedEnvelope.orientationId,
    orientationHash: approvedEnvelope.orientationHash,
    frozenContractHash: approvedEnvelope.frozenContractHash,
    sourceHashes: clone(approvedEnvelope.sourceHashes),
  };
  const validation = validatePolicyEnvelope(approvalOrientation, approvedEnvelope);
  if (!validation.valid) {
    throw new Error(`Approved schema policy export is not valid for the governed orientation:\n${validation.errors.join('\n')}`);
  }
  const migration = migratePolicyEnvelope(orientation, approvedEnvelope, {
    exportedAt: approvedEnvelope.exportedAt,
  });
  const canonicalValidation = validatePolicyEnvelope(orientation, migration.envelope);
  const gate = policyGate(orientation, migration.envelope);
  if (!canonicalValidation.valid || !gate.complete) {
    throw new Error(`Migrated canonical schema policy did not close the Phase 0 gate:\n${[...canonicalValidation.errors, ...gate.reasons].join('\n')}`);
  }
  const counts = migration.report.counts;
  if (counts.retained !== 8 || counts.stale !== 0 || counts.dropped !== 0) {
    throw new Error(`Schema policy migration must retain 8/8 with no stale or dropped policy, got ${counts.retained}/${counts.stale}/${counts.dropped}`);
  }
  const sourceDrift = classifySourceDrift(approvedEnvelope.sourceHashes, orientation.sourceHashes);
  if (sourceDrift.classification === 'UNAPPROVED_SOURCE_DRIFT') {
    throw new Error(`Approved schema policy source drift is not allowed: ${sourceDrift.changed.map((entry) => entry.path).join(', ')}`);
  }
  if (options.expectedCanonical && !sameCanonical(options.expectedCanonical, migration.envelope)) {
    throw new Error('Mission/pd2-skills-schema-policy.json is not the exact canonical migrated policy envelope');
  }
  return {
    envelope: migration.envelope,
    gate,
    migrationReport: {
      ...migration.report,
      sourceDriftClassification: sourceDrift.classification,
      sourceDrift,
    },
  };
}

export function loadCanonicalSchemaPolicy(orientation, options = {}) {
  const approvedPath = options.approvedExportPath ?? APPROVED_POLICY_EXPORT_PATH;
  const canonicalPath = options.canonicalPolicyPath ?? CANONICAL_POLICY_PATH;
  const approved = rawPolicy(approvedPath);
  const approvedExportHash = sha256Buffer(approved.raw);
  if (approvedExportHash !== APPROVED_POLICY_EXPORT_SHA256) {
    throw new Error(`Approved policy export byte hash mismatch: expected ${APPROVED_POLICY_EXPORT_SHA256}, got ${approvedExportHash}`);
  }
  const canonical = rawPolicy(canonicalPath);
  const validated = validateAndMigrateApprovedPolicy(orientation, approved.document, {
    expectedCanonical: canonical.document,
  });
  return {
    envelope: validated.envelope,
    canonicalPolicyHash: sha256Canonical(validated.envelope),
    approvedExportHash,
    provenance: {
      approvedExportPath: path.relative(repoRoot, approvedPath).replaceAll('\\', '/'),
      approvedExportHash,
      canonicalPolicyPath: path.relative(repoRoot, canonicalPath).replaceAll('\\', '/'),
      canonicalPolicyHash: sha256Canonical(validated.envelope),
      fromOrientationHash: validated.migrationReport.fromOrientationHash,
      toOrientationHash: validated.migrationReport.toOrientationHash,
      sourceDriftClassification: validated.migrationReport.sourceDriftClassification,
      sourceDrift: validated.migrationReport.sourceDrift,
    },
    migrationReport: validated.migrationReport,
  };
}

function dictionaryMap(orientation) {
  return new Map((orientation.columns ?? []).map((field) => [canonicalFieldHeader(field.canonicalHeader), field]));
}

function sourceRecord(skill, source, sources) {
  const ordinal = skill.ordinals?.[source];
  return Number.isInteger(ordinal) ? sources.documents[source]['skills.txt'].records[ordinal] ?? null : null;
}

function evidenceForRecord(document, record, header, source) {
  const canonical = canonicalFieldHeader(header);
  const index = document.indexes.get(canonical);
  const columnPresent = index !== undefined;
  const rowPresent = Boolean(record);
  const rawValue = columnPresent && rowPresent ? record.row[index] : null;
  return {
    source,
    columnPresent,
    rowPresent,
    rawHeader: columnPresent ? document.table.headers[index] : null,
    rawValue,
    rawState: rawValueState(columnPresent, rawValue),
    semanticBlank: semanticBlank(rawValue),
  };
}

function comparison(rawEvidence) {
  const semanticToken = (item) => item.semanticBlank ? 'SEMANTIC_BLANK' : String(item.rawValue).trim();
  const rawToken = (item) => `${item.columnPresent}\u0000${item.rowPresent}\u0000${item.rawState}\u0000${item.rawValue ?? ''}`;
  const evidence = SOURCE_KEYS.map((source) => rawEvidence[source]);
  return {
    rawChanged: new Set(evidence.map(rawToken)).size > 1,
    semanticChanged: new Set(evidence.map(semanticToken)).size > 1,
    bkvincePd2Changed: semanticToken(rawEvidence.bkvince) !== semanticToken(rawEvidence.pd2),
    allSemanticBlank: evidence.every((item) => item.semanticBlank),
  };
}

function skillsFieldEvidence(skill, header, sources) {
  return Object.fromEntries(SOURCE_KEYS.map((source) => {
    const document = sources.documents[source]['skills.txt'];
    return [source, evidenceForRecord(document, sourceRecord(skill, source, sources), header, source)];
  }));
}

function exactLinkedRecord(document, key) {
  const records = document.records.filter((record) => String(record.key ?? '').trim().toLowerCase() === String(key ?? '').trim().toLowerCase());
  return records.length === 1 ? records[0] : null;
}

function linkedFieldEvidence(table, key, header, sources) {
  return Object.fromEntries(SOURCE_KEYS.map((source) => {
    const document = sources.documents[source][table];
    return [source, evidenceForRecord(document, exactLinkedRecord(document, key), header, source)];
  }));
}

function directBundleId(header) {
  return bundleForHeader(header)?.id ?? null;
}

function linkedBundleId(table, header, dependency) {
  if (table === 'missiles.txt') {
    if (MISSILE_PHYSICS_PATTERN.test(header)) return 'PROJECTILE_PHYSICS';
    if (NATIVE_PATTERN.test(header) || NATIVE_PARAMETER_PATTERN.test(header)) return 'NATIVE_EXECUTION';
  }
  if (['pettype.txt', 'monstats.txt'].includes(table)) return 'SUMMON_PACKAGE';
  if (['states.txt', 'itemstatcost.txt'].includes(table)) {
    return /^(?:passive|aura)/i.test(dependency.field ?? '') ? 'PASSIVE_PACKAGE' : 'SUMMON_PACKAGE';
  }
  return null;
}

function technicalResolution(bundleId, fields) {
  if (bundleId === 'NATIVE_EXECUTION') {
    return fields.some((field) => !field.rawEvidence.pd2.semanticBlank && field.proofStatus === 'NATIVE_UNPROVEN')
      ? 'DEFER_NATIVE_PROOF'
      : 'PRESERVE_BKVINCE';
  }
  return 'PRESERVE_BKVINCE';
}

function proofStatusFor(fields) {
  for (const status of ['MALFORMED_SOURCE', 'NATIVE_UNPROVEN', 'UNSUPPORTED_IDENTIFIER', 'SYMBOLIC', 'EXACT_FORMULA', 'EXACT_TABLE']) {
    if (fields.some((field) => field.proofStatus === status)) return status;
  }
  return 'EXACT_TABLE';
}

function enhanceSkillsFields(skill, sources, dictionary) {
  const fields = [];
  for (const component of skill.components) {
    for (const field of component.fields) {
      const header = canonicalFieldHeader(field.header);
      const dictionaryField = dictionary.get(header) ?? null;
      const rawEvidence = skillsFieldEvidence(skill, header, sources);
      const compared = comparison(rawEvidence);
      fields.push({
        ...field,
        table: 'skills.txt',
        header,
        locator: {
          table: 'skills.txt',
          rows: Object.fromEntries(SOURCE_KEYS.map((source) => [source, skill.ordinals?.[source] ?? null])),
        },
        playerLabelFr: dictionaryField?.playerLabelFr ?? field.label ?? header,
        shortHelpFr: dictionaryField?.shortHelpFr ?? '',
        rawEvidence,
        rawChanged: compared.rawChanged,
        semanticChanged: compared.semanticChanged,
        decisionRelevant: false,
        semanticDifferenceReason: compared.allSemanticBlank ? 'ALL_SEMANTIC_BLANK' : 'UNROUTED',
        decisionOwnerBundleId: null,
        policyResolution: null,
        _directBundleId: directBundleId(header),
        _compared: compared,
      });
    }
  }
  return fields;
}

function collectLinkedFields(skill, sources) {
  const result = [];
  const seen = new Set();
  for (const dependency of skill.dependencies ?? []) {
    const candidateHeaders = dependency.table === 'missiles.txt'
      ? [...new Set(SOURCE_KEYS.flatMap((source) => sources.documents[source][dependency.table].table.headers.map(canonicalFieldHeader)))]
      : (dependency.facts ?? []).map((fact) => canonicalFieldHeader(fact.header));
    for (const header of candidateHeaders) {
      const bundleId = linkedBundleId(dependency.table, header, dependency);
      if (!bundleId) continue;
      const id = `${dependency.table}:${normalize(dependency.key)}:${header}`;
      if (seen.has(id)) continue;
      seen.add(id);
      const rawEvidence = linkedFieldEvidence(dependency.table, dependency.key, header, sources);
      const compared = comparison(rawEvidence);
      if (!compared.bkvincePd2Changed || compared.allSemanticBlank) continue;
      result.push({
        id,
        table: dependency.table,
        rowKey: dependency.key,
        header,
        label: LINKED_LABELS_FR[header] ?? header,
        playerLabelFr: LINKED_LABELS_FR[header] ?? header,
        shortHelpFr: `Preuve brute liée dans ${dependency.table}.`,
        locator: { table: dependency.table, key: dependency.key },
        values: Object.fromEntries(SOURCE_KEYS.map((source) => [source, rawEvidence[source].rawValue])),
        rawEvidence,
        rawChanged: compared.rawChanged,
        semanticChanged: compared.semanticChanged,
        changed: compared.semanticChanged,
        protected: bundleId === 'NATIVE_EXECUTION',
        protectionReasons: bundleId === 'NATIVE_EXECUTION' ? ['native_functions'] : [],
        proofStatus: bundleId === 'NATIVE_EXECUTION' ? 'NATIVE_UNPROVEN' : 'EXACT_TABLE',
        dependencyIds: [dependency.id],
        decisionRelevant: true,
        semanticDifferenceReason: bundleId === 'NATIVE_EXECUTION' ? 'NATIVE_CALLBACK_DIFFERS_AUTO_DEFERRED' : 'PLAYER_BEHAVIOR_DIFFERS',
        decisionOwnerBundleId: bundleId,
        policyResolution: bundleId === 'NATIVE_EXECUTION' ? 'DEFER_NATIVE_PROOF' : null,
        _directBundleId: bundleId,
        _compared: compared,
      });
    }
  }
  return result;
}

function dynamicParameterOwners(fields) {
  const owners = new Map();
  const byHeader = new Map(fields.filter((field) => field.table === 'skills.txt').map((field) => [field.header, field]));
  for (const field of fields) {
    const owner = field._directBundleId;
    if (!owner || !/(?:calc|sympercalc)$/i.test(field.header)) continue;
    for (const evidence of Object.values(field.rawEvidence)) {
      for (const match of String(evidence.rawValue ?? '').matchAll(/\bpar(?:am)?(\d{1,2})\b/gi)) {
        const target = byHeader.get(`param${Number(match[1])}`);
        if (!target) continue;
        if (!owners.has(target.id)) owners.set(target.id, new Set());
        owners.get(target.id).add(owner);
      }
    }
  }
  for (const [fieldId, candidates] of owners) {
    if (candidates.size > 1) throw new Error(`Ambiguous Phase 1 decision ownership for ${fieldId}: ${[...candidates].join(', ')}`);
  }
  return new Map([...owners].map(([fieldId, candidates]) => [fieldId, [...candidates][0]]));
}

function applyPolicies(fields) {
  const dynamicOwners = dynamicParameterOwners(fields);
  const reductions = {
    semanticBlank: [],
    preserveD2rColumnAbsentFromPd2: [],
    noPd2BkvinceDifference: [],
    itemEconomy: [],
    globalPortabilityGate: [],
    technicalAutoResolution: [],
    bundled: [],
    unownedTechnical: [],
  };
  for (const field of fields) {
    const compared = field._compared;
    const header = field.header;
    if (compared.allSemanticBlank) {
      field.semanticDifferenceReason = 'ALL_SEMANTIC_BLANK';
      field.policyResolution = 'NO_DECISION';
      reductions.semanticBlank.push(field.id);
      continue;
    }
    if (!compared.bkvincePd2Changed) {
      field.semanticDifferenceReason = 'NO_PD2_BKVINCE_DIFFERENCE';
      field.policyResolution = 'NO_DECISION';
      reductions.noPd2BkvinceDifference.push(field.id);
      continue;
    }
    if (!field.rawEvidence.pd2.columnPresent && !field.rawEvidence.bkvince.semanticBlank) {
      field.semanticDifferenceReason = 'PD2_HEADER_ABSENT_PRESERVE_D2R_BKVINCE';
      field.policyResolution = 'PRESERVE_BKVINCE';
      reductions.preserveD2rColumnAbsentFromPd2.push(field.id);
      continue;
    }
    if (ITEM_ECONOMY_PATTERN.test(header)) {
      field.semanticDifferenceReason = 'AUTO_RESOLVED_ITEM_ECONOMY';
      field.policyResolution = 'PRESERVE_BKVINCE';
      reductions.itemEconomy.push(field.id);
      continue;
    }
    if (PD2_ONLY_HEADERS.has(header) && field.table === 'skills.txt') {
      field.semanticDifferenceReason = 'GLOBAL_PORTABILITY_GATE_PRECEDES_SKILL_DECISION';
      field.policyResolution = 'DEFER_NATIVE_PROOF';
      reductions.globalPortabilityGate.push(field.id);
      continue;
    }
    const owner = field._directBundleId ?? dynamicOwners.get(field.id) ?? null;
    if (!owner) {
      field.semanticDifferenceReason = 'AUTO_RESOLVED_RAW_TECHNICAL';
      field.policyResolution = 'PRESERVE_BKVINCE';
      reductions.unownedTechnical.push(field.id);
      continue;
    }
    field.decisionOwnerBundleId = owner;
    field.decisionRelevant = true;
    if (TECHNICAL_BUNDLES.has(owner)) {
      field.semanticDifferenceReason = owner === 'NATIVE_EXECUTION'
        ? 'NATIVE_CALLBACK_DIFFERS_AUTO_DEFERRED'
        : 'TECHNICAL_PACKAGE_AUTO_PRESERVED';
      field.policyResolution = owner === 'NATIVE_EXECUTION' ? 'DEFER_NATIVE_PROOF' : 'PRESERVE_BKVINCE';
      reductions.technicalAutoResolution.push(field.id);
    } else {
      field.semanticDifferenceReason = 'PLAYER_BEHAVIOR_DIFFERS';
      reductions.bundled.push(field.id);
    }
  }
  return reductions;
}

function buildBundles(skill, fields) {
  const definitions = new Map(DECISION_BUNDLES.map((definition) => [definition.id, definition]));
  const byOwner = new Map();
  for (const field of fields.filter((candidate) => candidate.decisionOwnerBundleId)) {
    if (!byOwner.has(field.decisionOwnerBundleId)) byOwner.set(field.decisionOwnerBundleId, []);
    byOwner.get(field.decisionOwnerBundleId).push(field);
  }
  return [...byOwner.entries()].map(([id, owned]) => {
    const definition = definitions.get(id);
    if (!definition) throw new Error(`Unknown Phase 1 decision bundle ${id}`);
    const scope = TECHNICAL_BUNDLES.has(id) ? 'TECHNICAL' : 'PLAYER';
    const manualDecisionRequired = scope === 'PLAYER';
    const autoResolution = manualDecisionRequired ? null : technicalResolution(id, owned);
    return {
      id,
      stableId: `bundle:${id}`,
      scope,
      fieldIds: owned.map((field) => field.id).sort(),
      playerLabelFr: definition.labelFr,
      shortHelpFr: manualDecisionRequired
        ? 'Une décision de comportement projette toutes les cellules brutes de ce package.'
        : 'Package technique visible et prérempli; un override expert explicite est requis pour le modifier.',
      manualDecisionRequired,
      defaultResolution: manualDecisionRequired ? 'KEEP_BKVINCE' : autoResolution,
      ...(autoResolution ? { autoResolution } : {}),
      proofStatus: proofStatusFor(owned),
      protected: Boolean(definition.protected),
      customSchema: manualDecisionRequired ? {
        required: ['valueOrFormula', 'justification'],
        optional: ['gameplayObjective', 'testPlan'],
      } : null,
      proposedResult: {
        resolution: manualDecisionRequired ? 'KEEP_BKVINCE' : autoResolution,
        fields: Object.fromEntries(owned.map((field) => [field.id, field.rawEvidence.bkvince.rawValue])),
      },
      fingerprint: sha256Canonical({
        skillStableId: skill.stableId,
        id,
        scope,
        fields: owned.map((field) => ({ id: field.id, rawEvidence: field.rawEvidence, proofStatus: field.proofStatus })),
      }),
    };
  }).sort((left, right) => left.id.localeCompare(right.id, 'en'));
}

function addLinkedFieldsToComponents(skill, linked) {
  if (!linked.length) return;
  const componentFor = (field) => field.decisionOwnerBundleId === 'NATIVE_EXECUTION'
    ? 'engine_functions'
    : field.table === 'missiles.txt' ? 'projectiles_collisions'
      : ['SUMMON_PACKAGE', 'PASSIVE_PACKAGE'].includes(field.decisionOwnerBundleId) ? 'summons'
        : 'consumers';
  for (const field of linked) {
    let component = skill.components.find((candidate) => candidate.id === componentFor(field));
    if (!component) component = skill.components.find((candidate) => candidate.id === 'consumers');
    component.fields.push(field);
    component.changed = true;
  }
}

function applyLinkedPortability(skill, linked) {
  const native = linked.filter((field) => field.decisionOwnerBundleId === 'NATIVE_EXECUTION' && field.semanticChanged);
  if (!native.length) return;
  const portability = skill.portability;
  const addUnique = (key, value) => {
    if (!portability[key].includes(value)) portability[key].push(value);
  };
  for (const category of ['NATIVE_FUNCTION_MISMATCH', 'NATIVE_UNPROVEN', 'NETWORK_OR_CLIENT_SERVER_RISK']) {
    addUnique('categories', category);
    addUnique('classification', category);
  }
  const reason = 'Changed callbacks or callback parameters in linked tables are source-specific and require D2R 3.2 native proof.';
  if (!portability.reasons.includes(reason)) portability.reasons.push(reason);
  portability.divergentFunctions = [...new Set([
    ...portability.divergentFunctions,
    ...native.map((field) => field.id),
  ])].sort();
  portability.networkRisk = 'CLIENT_SERVER_BEHAVIOR_UNPROVEN';
  portability.effort = portability.effort === 'HIGH' ? 'HIGH' : 'MEDIUM_OR_HIGH';
  if (!portability.proofRequired.includes('D2R_3_2_NATIVE_FUNCTION_AUDIT')) {
    portability.proofRequired.push('D2R_3_2_NATIVE_FUNCTION_AUDIT');
  }
}

function stripInternals(field) {
  const { _directBundleId, _compared, ...publicField } = field;
  return publicField;
}

export function applyPhase1DataModel(baseReport, options) {
  const { orientation, schemaPolicy, sources } = options ?? {};
  if (!baseReport || !orientation || !schemaPolicy?.envelope || !sources) {
    throw new Error('Phase 1 requires a base report, orientation, canonical schema policy wrapper, and governed sources');
  }
  const dictionary = dictionaryMap(orientation);
  const report = clone(baseReport);
  report.phase1Model = PHASE1_MODEL_ID;
  report.schemaPolicy = clone(schemaPolicy);
  for (const skill of report.skills) {
    const fields = enhanceSkillsFields(skill, sources, dictionary);
    const linked = collectLinkedFields(skill, sources);
    const allFields = [...fields, ...linked];
    const reductions = applyPolicies(allFields);
    addLinkedFieldsToComponents(skill, linked);
    applyLinkedPortability(skill, linked);
    const publicById = new Map(allFields.map((field) => [field.id, stripInternals(field)]));
    for (const component of skill.components) {
      component.fields = component.fields.map((field) => publicById.get(field.id) ?? field);
    }
    skill.decisionBundles = buildBundles(skill, allFields);
    const owned = new Set();
    for (const bundle of skill.decisionBundles) {
      for (const fieldId of bundle.fieldIds) {
        if (owned.has(fieldId)) throw new Error(`Ambiguous Phase 1 decision ownership for ${skill.stableId}/${fieldId}`);
        owned.add(fieldId);
      }
    }
    const relevant = allFields.filter((field) => field.decisionRelevant);
    for (const field of relevant) {
      if (!field.decisionOwnerBundleId || !owned.has(field.id)) {
        throw new Error(`Decision-relevant field has no unique bundle owner: ${skill.stableId}/${field.id}`);
      }
    }
    skill.policyApplication = {
      canonicalPolicyHash: schemaPolicy.canonicalPolicyHash,
      rules: Object.entries(schemaPolicy.envelope.decisions).map(([id, decision]) => ({ id, decision: decision.decision })),
      reductions,
      counts: {
        rawFields: allFields.length,
        decisionRelevant: relevant.length,
        playerBundles: skill.decisionBundles.filter((bundle) => bundle.scope === 'PLAYER').length,
        technicalBundles: skill.decisionBundles.filter((bundle) => bundle.scope === 'TECHNICAL').length,
        autoResolvedFields: allFields.filter((field) => field.policyResolution).length,
      },
    };
    skill.fingerprint = sha256Canonical({
      previousFingerprint: skill.fingerprint,
      decisionBundles: skill.decisionBundles,
      policyApplication: skill.policyApplication,
      curves: skill.curves,
    });
  }
  const previousComparisonHash = report.comparisonHash;
  report.policyHashes = {
    ...report.policyHashes,
    canonicalSchemaPolicy: schemaPolicy.canonicalPolicyHash,
  };
  report.comparisonHash = sha256Canonical({
    composition: PHASE1_MODEL_ID,
    previousComparisonHash,
    frozenContractHash: report.frozenContractHash,
    sourceHashes: report.sourceHashes,
    policyHashes: report.policyHashes,
    skillFingerprints: report.skills.map((skill) => [skill.stableId, skill.fingerprint]),
  });
  if (report.schemaOrientation?.workbenchBinding) {
    report.schemaOrientation.workbenchBinding.comparisonHash = report.comparisonHash;
  }
  return report;
}
