import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import {
  NON_MUTATION_RULES,
  PREVIEW_ID,
  PREVIEW_SCHEMA_VERSION,
} from './pd2-skills-review-contracts.mjs';
import {
  entryState,
  resolveFieldChoice,
  validateImport,
} from './pd2-skills-review-runtime.mjs';

const modulePath = fileURLToPath(import.meta.url);
const repoRoot = path.resolve(path.dirname(modulePath), '..', '..');
const defaultReportPath = path.join(repoRoot, 'Mission', 'pd2-skills-review.json');

function fail(message) {
  throw new Error(message);
}

function isObject(value) {
  return value !== null && typeof value === 'object' && !Array.isArray(value);
}

function clone(value) {
  return value === undefined ? undefined : JSON.parse(JSON.stringify(value));
}

function nonBlank(value) {
  return typeof value === 'string' && value.trim().length > 0;
}

function normalizeTable(value) {
  return String(value ?? '').replaceAll('\\', '/').split('/').at(-1).toLowerCase();
}

function readJson(file) {
  return JSON.parse(fs.readFileSync(file, 'utf8').replace(/^\uFEFF/, ''));
}

function sha256File(file) {
  return crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex').toUpperCase();
}

function isWithin(candidate, root) {
  const relative = path.relative(root, candidate);
  return relative === '' || (!relative.startsWith(`..${path.sep}`) && relative !== '..' && !path.isAbsolute(relative));
}

function allowedReadRoots(root) {
  return NON_MUTATION_RULES.allowedReadRoots.map((relative) => path.resolve(root, relative));
}

function allowedOutputRoots(root) {
  return NON_MUTATION_RULES.allowedGeneratedRoots.map((relative) => path.resolve(root, relative));
}

function resolveGovernedPath(root, rawPath) {
  if (!nonBlank(rawPath) || /^[a-z]+:\/\//i.test(rawPath)) return null;
  const resolved = path.resolve(root, rawPath);
  if (!allowedReadRoots(root).some((allowed) => isWithin(resolved, allowed))) {
    fail(`Source manifest path is outside governed read roots: ${rawPath}`);
  }
  return resolved;
}

function sourceDescriptors(value, trail = [], seen = new Set(), result = []) {
  if (Array.isArray(value)) {
    value.forEach((item, index) => sourceDescriptors(item, [...trail, String(index)], seen, result));
    return result;
  }
  if (!isObject(value)) return result;
  const rawPath = value.path ?? value.file ?? value.relativePath;
  const expectedHash = value.sha256 ?? value.hashSha256 ?? value.fileSha256;
  if (nonBlank(rawPath) && (/^[A-Fa-f0-9]{64}$/.test(expectedHash ?? '') || value.exists === false || value.missing === true)) {
    const key = `${rawPath}\0${expectedHash ?? 'MISSING'}`;
    if (!seen.has(key)) {
      seen.add(key);
      result.push({
        id: value.id ?? value.source ?? trail.join('.'),
        path: rawPath,
        sha256: expectedHash ? String(expectedHash).toUpperCase() : null,
        expectedMissing: value.exists === false || value.missing === true,
      });
    }
  }
  for (const [key, item] of Object.entries(value)) {
    if (key !== 'raw' && key !== 'headers') sourceDescriptors(item, [...trail, key], seen, result);
  }
  return result;
}

function hashLeaves(value, result = []) {
  if (typeof value === 'string' && /^[A-Fa-f0-9]{64}$/.test(value)) result.push(value.toUpperCase());
  else if (Array.isArray(value)) value.forEach((item) => hashLeaves(item, result));
  else if (isObject(value)) Object.values(value).forEach((item) => hashLeaves(item, result));
  return result;
}

export function verifyCurrentSources(report, options = {}) {
  const root = path.resolve(options.repoRoot ?? repoRoot);
  const descriptors = sourceDescriptors(report?.sourceManifest);
  if (descriptors.length === 0) fail('sourceManifest contains no governed local file descriptor to verify');
  const declaredHashes = new Set(hashLeaves(report?.sourceHashes));
  const verified = [];
  for (const descriptor of descriptors) {
    const resolved = resolveGovernedPath(root, descriptor.path);
    if (!resolved) continue;
    const exists = fs.existsSync(resolved);
    if (descriptor.expectedMissing) {
      if (exists) fail(`${descriptor.path}: expected governed absence but the path now exists`);
      verified.push({ ...descriptor, resolvedPath: resolved, status: 'EXPECTED_MISSING' });
      continue;
    }
    if (!exists || !fs.statSync(resolved).isFile()) fail(`${descriptor.path}: governed source file is missing`);
    if (!declaredHashes.has(descriptor.sha256)) {
      fail(`${descriptor.path}: sourceManifest hash is absent from sourceHashes`);
    }
    const actual = (options.hashFile ?? sha256File)(resolved).toUpperCase();
    if (actual !== descriptor.sha256) {
      fail(`${descriptor.path}: current SHA-256 ${actual} does not match governed ${descriptor.sha256}`);
    }
    verified.push({ ...descriptor, resolvedPath: resolved, actualSha256: actual, status: 'VERIFIED' });
  }
  if (!verified.some((item) => item.status === 'VERIFIED')) fail('No current governed source file could be verified');
  return verified;
}

function assertGovernedReport(report) {
  if (!isObject(report)) fail('A governed skill review oracle is required');
  for (const key of ['reviewId', 'comparisonHash', 'frozenContractHash']) {
    if (!nonBlank(report[key])) fail(`Review oracle ${key} is missing`);
  }
  if (!isObject(report.sourceHashes) || Object.keys(report.sourceHashes).length === 0) fail('Review oracle sourceHashes are missing');
  if (!Array.isArray(report.skills) || !Array.isArray(report.nodes) || !Array.isArray(report.collisions)) {
    fail('Review oracle canonical skills, nodes, or collisions are missing');
  }
  const stableIds = new Set();
  for (const skill of report.skills) {
    if (!nonBlank(skill.stableId) || !nonBlank(skill.fingerprint)) fail('A canonical skill lacks stableId or fingerprint');
    if (stableIds.has(skill.stableId)) fail(`Ambiguous canonical stableId ${skill.stableId}`);
    stableIds.add(skill.stableId);
  }
}

function nextAppendOrdinal(report) {
  const candidates = [
    report?.coverage?.nextAppendOrdinal,
    report?.coverage?.appendOnly?.nextOrdinal,
    report?.appendOnly?.nextOrdinal,
    report?.nextAppendOrdinal,
  ];
  const direct = candidates.find((value) => Number.isInteger(value) && value >= 0);
  if (direct !== undefined) return direct;
  const occupied = (report?.nodes ?? [])
    .filter((node) => node.source === 'bkvince' && Number.isInteger(node.ordinal))
    .map((node) => node.ordinal);
  if (occupied.length === 0) fail('Cannot calculate append-only ordinal without BKVince physical nodes');
  return Math.max(...occupied) + 1;
}

function nodeMap(report) {
  return new Map((report.nodes ?? []).map((node) => [node.id, node]));
}

function sourceNode(skill, source, nodes) {
  const ref = skill?.nodeIds?.[source]
    ?? (Array.isArray(skill?.nodeIds)
      ? skill.nodeIds.find((id) => String(id).startsWith(`${source}:`))
      : null);
  if (ref && nodes.has(ref)) return nodes.get(ref);
  const ordinal = skill?.ordinals?.[source];
  return [...nodes.values()].find((node) => node.source === source && node.ordinal === ordinal) ?? null;
}

function fieldValue(field, source) {
  if (isObject(field?.values) && Object.hasOwn(field.values, source)) return field.values[source];
  if (isObject(field?.displayValues) && Object.hasOwn(field.displayValues, source)) return field.displayValues[source];
  return undefined;
}

function fieldHeader(field, source = null) {
  if (source && isObject(field?.headers) && nonBlank(field.headers[source])) return field.headers[source];
  if (source && nonBlank(field?.[`${source}Header`])) return field[`${source}Header`];
  return field?.header ?? field?.id;
}

function canonicalHeader(value) {
  const header = String(value ?? '').trim().toLowerCase();
  return header === '*id' ? 'id' : header;
}

function fieldTargetRow(skill, field, nodes) {
  const candidates = [
    field?.target?.row,
    field?.target?.ordinal,
    field?.sourceLocators?.bkvince?.row,
    field?.targetRow,
    field?.bkvinceRow,
  ];
  const explicit = candidates.find(Number.isInteger);
  if (explicit !== undefined) return explicit;
  if (normalizeTable(field?.table) === 'skills.txt') {
    const node = sourceNode(skill, 'bkvince', nodes);
    return node?.ordinal ?? skill?.ordinals?.bkvince ?? null;
  }
  return null;
}

function fieldSourceRow(skill, field, nodes) {
  const candidates = [field?.pd2Source?.row, field?.sourceLocators?.pd2?.row, field?.source?.row, field?.source?.ordinal, field?.sourceRow, field?.pd2Row];
  const explicit = candidates.find(Number.isInteger);
  if (explicit !== undefined) return explicit;
  if (normalizeTable(field?.table) === 'skills.txt') {
    const node = sourceNode(skill, 'pd2', nodes);
    return node?.ordinal ?? skill?.ordinals?.pd2 ?? null;
  }
  return null;
}

function portabilityCategories(value) {
  if (Array.isArray(value)) return value;
  if (Array.isArray(value?.categories)) return value.categories;
  if (typeof value === 'string') return [value];
  return [];
}

function nativeRisk(field, component) {
  const proofStatus = field?.proofStatus ?? component?.proofStatus;
  const portability = field?.proofStatus
    ? portabilityCategories(field?.portability)
    : [...portabilityCategories(field?.portability), ...portabilityCategories(component?.portability)];
  return proofStatus === 'NATIVE_UNPROVEN'
    || portability.some((category) => category === 'NATIVE_UNPROVEN' || category === 'NATIVE_FUNCTION_MISMATCH');
}

function malformedRisk(field, component) {
  return (field?.proofStatus ?? component?.proofStatus) === 'MALFORMED_SOURCE';
}

function automaticDelayTranslation(field, choice) {
  if (choice?.decision !== 'ADOPT_PD2') return false;
  const source = String(fieldHeader(field, 'pd2') ?? '').toLowerCase();
  const target = String(fieldHeader(field, 'bkvince') ?? field?.header ?? '').toLowerCase();
  if (source === 'delay' && (target === 'localdelay' || target === 'globaldelay')) return true;
  return field?.automaticDelayTranslation === true || field?.mappingCategory === 'DELAY_TRANSLATION';
}

function protectedRiskErrors(skill, component, field, choice) {
  const result = [];
  if (!choice || !['ADOPT_PD2', 'CUSTOM'].includes(choice.decision)) return result;
  if (automaticDelayTranslation(field, choice)) {
    result.push({
      code: 'AUTOMATIC_DELAY_TRANSLATION',
      stableId: skill.stableId,
      fieldId: field.id,
      reason: 'PD2 delay cannot be translated automatically to D2R localdelay/globaldelay',
    });
  }
  if (nativeRisk(field, component)) {
    const override = choice.protectedOverride;
    if (override?.approved !== true || override.nativeRiskAccepted !== true) {
      result.push({
        code: 'NATIVE_PROOF_REQUIRED',
        stableId: skill.stableId,
        fieldId: field.id,
        proofStatus: field.proofStatus ?? component.proofStatus,
        reason: 'A native-unproven or divergent behavior was selected without an explicit native-risk override',
      });
    }
  }
  if (malformedRisk(field, component)) {
    const override = choice.protectedOverride;
    if (override?.approved !== true || !nonBlank(override.malformedResolution)) {
      result.push({
        code: 'MALFORMED_RESOLUTION_REQUIRED',
        stableId: skill.stableId,
        fieldId: field.id,
        proofStatus: 'MALFORMED_SOURCE',
        reason: 'A malformed source formula was selected without a governed resolution',
      });
    }
  }
  return result;
}

function dependencyIdentity(dependency, index = 0) {
  return dependency?.id
    ?? [dependency?.table, dependency?.row ?? dependency?.key ?? dependency?.name ?? index].filter((value) => value !== undefined).join(':')
    ?? `dependency:${index}`;
}

function dependencyClosed(dependency) {
  if (dependency?.required === false || dependency?.notApplicable === true) return true;
  if (dependency?.source === 'pd2' && isObject(dependency?.targetAvailability)
    && Object.hasOwn(dependency.targetAvailability, 'bkvince')) {
    if (dependency.targetAvailability.bkvince === true) return true;
    return dependency?.remapPlan?.complete === true || dependency?.plan?.complete === true;
  }
  if (dependency?.closed === true || dependency?.available === true || dependency?.presentInBkvince === true || dependency?.targetExists === true) return true;
  const status = String(dependency?.closureStatus ?? dependency?.status ?? '').toUpperCase();
  if (['CLOSED', 'PROVEN', 'AVAILABLE', 'COMPATIBLE', 'PRESENT', 'PRESENT_IN_BKVINCE', 'RESOLVED', 'PLANNED_APPEND', 'PLANNED_LOCALIZATION'].includes(status)) return true;
  return dependency?.remapPlan?.complete === true || dependency?.plan?.complete === true;
}

function dependencyAudit(skill, selectedFieldIds, importingNew) {
  const dependencies = (skill.dependencies ?? []).map((dependency, index) => ({
    ...clone(dependency),
    id: dependencyIdentity(dependency, index),
  }));
  const wanted = new Set();
  if (importingNew) dependencies.forEach((dependency) => wanted.add(dependency.id));
  for (const component of skill.components ?? []) {
    for (const field of component.fields ?? []) {
      if (!selectedFieldIds.has(field.id)) continue;
      (field.dependencyIds ?? []).forEach((id) => wanted.add(id));
      const header = String(fieldHeader(field, 'pd2') ?? field.header ?? '').trim().toLowerCase();
      for (const dependency of dependencies) {
        const dependencyHeader = String(dependency.sourceFieldId ?? dependency.field ?? dependency.header ?? '').trim().toLowerCase();
        if (dependency.source === 'pd2' && dependencyHeader && dependencyHeader === header) wanted.add(dependency.id);
      }
    }
  }
  const byId = new Map(dependencies.map((dependency) => [dependency.id, dependency]));
  const missingReferences = [...wanted]
    .filter((id) => !byId.has(id))
    .map((id) => ({
      code: 'DEPENDENCY_REFERENCE_MISSING',
      stableId: skill.stableId,
      dependencyId: id,
      reason: 'A selected field references a dependency absent from the governed closure graph',
    }));
  const selected = dependencies.filter((dependency) => wanted.has(dependency.id));
  const unclosed = selected.filter((dependency) => !dependencyClosed(dependency)).map((dependency) => ({
    code: 'DEPENDENCY_NOT_CLOSED',
    stableId: skill.stableId,
    dependencyId: dependency.id,
    table: dependency.table ?? null,
    status: dependency.closureStatus ?? dependency.status ?? null,
    reason: dependency.reason ?? 'The selected dependency has no explicit compatible target or governed remap plan',
  }));
  if (importingNew && (skill?.newSkillPlan?.dependencyClosure?.complete === false
    || skill?.newSkillPlan?.dependencyClosure?.completeForBkvince === false)) {
    unclosed.push({
      code: 'DEPENDENCY_CLOSURE_INCOMPLETE',
      stableId: skill.stableId,
      reason: skill.newSkillPlan.dependencyClosure.reason ?? 'The new skill dependency closure is explicitly incomplete',
    });
  }
  return { dependencies: selected, missingReferences, unclosed };
}

function normalizeLocalization(item, index) {
  if (typeof item === 'string') return { id: item, key: item, required: true };
  return { id: item?.id ?? item?.key ?? `localization:${index}`, required: item?.required !== false, ...clone(item) };
}

function localizationAudit(skill, importingNew) {
  const plan = skill?.newSkillPlan ?? {};
  const raw = plan.localizations ?? plan.stringsRequired ?? plan.strings ?? [];
  const items = (Array.isArray(raw) ? raw : Object.entries(raw).map(([key, value]) => (
    isObject(value) ? { key, ...value } : { key, value }
  ))).map(normalizeLocalization);
  const missing = [];
  const output = [];
  for (const item of items) {
    if (item.required === false) continue;
    const key = item.key ?? item.name ?? item.stringKey;
    const present = item.presentInBkvince === true || ['PRESENT', 'COMPATIBLE', 'RESOLVED'].includes(String(item.status ?? '').toUpperCase());
    const value = item.value ?? item.text ?? item.pd2Value ?? item.sourceValue ?? item.sourceText;
    if (!nonBlank(key)) {
      missing.push({ code: 'LOCALIZATION_KEY_MISSING', stableId: skill.stableId, localizationId: item.id });
      continue;
    }
    if (!present && !nonBlank(value)) {
      missing.push({
        code: 'LOCALIZATION_VALUE_MISSING',
        stableId: skill.stableId,
        localizationId: item.id,
        key,
        reason: 'A required localization is absent from BKVince and has no governed source text',
      });
      continue;
    }
    output.push({ ...item, key, action: present ? 'KEEP' : 'ADD_REQUIRED', value: value ?? null });
  }
  if (importingNew && plan.localizationClosure?.complete === false) {
    missing.push({
      code: 'LOCALIZATION_CLOSURE_INCOMPLETE',
      stableId: skill.stableId,
      reason: plan.localizationClosure.reason ?? 'The new skill localization closure is explicitly incomplete',
    });
  }
  if (importingNew && items.length === 0 && plan.localizationClosure?.complete !== true) {
    missing.push({
      code: 'LOCALIZATION_CLOSURE_UNPROVEN',
      stableId: skill.stableId,
      reason: 'A new player skill requires explicit skilldesc/string closure evidence',
    });
  }
  return { items: output, missing };
}

function consumerUsesOrdinal(consumer) {
  const kind = String(consumer?.referenceKind ?? consumer?.encoding ?? consumer?.kind ?? '').toUpperCase();
  return consumer?.encodedByOrdinal === true || consumer?.usesOrdinal === true || kind === 'ORDINAL' || kind === 'SKILL_ORDINAL';
}

function consumerAudit(skill, targetOrdinal, pd2ModelSelected, importingNew = false) {
  const consumers = [
    ...(skill.consumers ?? []),
    ...(skill?.newSkillPlan?.consumers ?? []),
  ].map((consumer, index) => ({ id: consumer?.id ?? `consumer:${index}`, ...clone(consumer) }));
  const failures = [];
  for (const consumer of consumers) {
    if (!pd2ModelSelected || !consumerUsesOrdinal(consumer)) continue;
    const source = consumer.source ?? consumer.ownerSource;
    if (source && source !== 'pd2') continue;
    const planned = consumer.remapPlan ?? consumer.remap ?? {};
    const remappedOrdinal = planned.targetOrdinal ?? consumer.remappedOrdinal;
    const complete = planned.complete === true || consumer.remapped === true;
    if (!complete || !Number.isInteger(remappedOrdinal) || remappedOrdinal !== targetOrdinal) {
      failures.push({
        code: 'ORDINAL_CONSUMER_NOT_REMAPPED',
        stableId: skill.stableId,
        consumerId: consumer.id,
        targetOrdinal,
        reason: 'A consumer encoded by PD2 runtime ordinal lacks an exact remap to the proposed BKVince ordinal',
      });
    }
  }
  const closure = skill?.newSkillPlan?.consumerClosure ?? skill?.consumerClosure;
  if (importingNew && closure && closure.complete !== true) {
    failures.push({
      code: 'CONSUMER_CLOSURE_INCOMPLETE',
      stableId: skill.stableId,
      status: closure.status ?? null,
      reasons: clone(closure.reasons ?? []),
      reason: 'The new skill has no governed complete proof for all ordinal-encoded consumers',
    });
  }
  if (importingNew && !closure) {
    failures.push({
      code: 'CONSUMER_CLOSURE_UNPROVEN',
      stableId: skill.stableId,
      reason: 'A new player skill requires an explicit governed consumer-closure audit',
    });
  }
  return { consumers, failures };
}

function proposedOrdinal(skill) {
  const plan = skill?.newSkillPlan ?? {};
  return [
    plan.proposedOrdinal,
    plan.proposedTargetOrdinal,
    plan.targetOrdinal,
    plan.appendOnly?.proposedOrdinal,
    plan.appendOnly?.targetOrdinal,
  ].find(Number.isInteger) ?? null;
}

function proposedAppendRow(skill) {
  const plan = skill?.newSkillPlan ?? {};
  const proposal = plan.proposedRow ?? plan.row ?? plan.appendOnly?.row ?? plan.projection?.row;
  if (!isObject(proposal)) return null;
  const values = isObject(proposal.values) ? proposal.values : proposal;
  return {
    values: clone(values),
    metadata: proposal === values ? null : clone({
      sourceNodeId: proposal.sourceNodeId ?? null,
      targetTable: proposal.targetTable ?? null,
      targetOrdinal: proposal.targetOrdinal ?? null,
      targetHeaders: proposal.targetHeaders ?? null,
      mappingProvenance: proposal.mappingProvenance ?? null,
    }),
  };
}

function collisionObjects(report, skill) {
  const ids = new Set(skill.collisionIds ?? []);
  return (report.collisions ?? []).filter((collision) => (
    ids.has(collision.id)
    || collision.stableId === skill.stableId
    || collision.skillStableId === skill.stableId
    || collision.semanticStableId === skill.stableId
  )).map(clone);
}

function selectedSkillIds(report, decisions) {
  if (decisions.exportScope === 'COMPLETE_ONLY') return Object.keys(decisions.entries ?? {});
  return (report.skills ?? []).filter((skill) => !(skill.readOnly === true || skill.identical === true)).map((skill) => skill.stableId);
}

function emptyPreview(report, decisions, verifiedSources) {
  return {
    schemaVersion: PREVIEW_SCHEMA_VERSION,
    previewId: PREVIEW_ID,
    state: 'PREVIEW_ONLY_GAMEPLAY_APPLICATION_FORBIDDEN',
    ready: false,
    applicable: false,
    implementationAuthorized: false,
    reviewId: report.reviewId,
    comparisonHash: report.comparisonHash,
    frozenContractHash: report.frozenContractHash,
    sourceHashes: clone(report.sourceHashes),
    sourceVerification: verifiedSources.map((item) => ({
      id: item.id,
      path: item.path,
      expectedSha256: item.sha256,
      actualSha256: item.actualSha256 ?? null,
      status: item.status,
    })),
    decisionExport: {
      exportedAt: decisions.exportedAt,
      exportScope: decisions.exportScope,
      implementationStatusesAreInformationalOnly: true,
    },
    proposedManifest: null,
    exactChangesByTable: {},
    proposedCells: [],
    proposedRows: [],
    keptCells: [],
    adoptedCells: [],
    customCells: [],
    appendOnlyRows: [],
    dependencyGraph: { nodes: [], edges: [] },
    localizations: [],
    consumers: [],
    nativeProofs: [],
    collisions: [],
    conflicts: [],
    incomplete: [],
    tests: [],
    commitSlices: [],
    textualDiff: '',
    diffPreview: [],
  };
}

function fatalImportErrors(errors) {
  return errors.filter((error) => /(?:schemaVersion|kind|reviewId|comparisonHash|frozenContractHash|sourceHashes|fingerprint|unknown stableId|read-only identical)/i.test(error));
}

export function compilePreview(report, decisions, options = {}) {
  assertGovernedReport(report);
  const verifiedSources = verifyCurrentSources(report, options);
  const validation = validateImport(report, decisions);
  const fatal = fatalImportErrors(validation.errors);
  if (fatal.length > 0) fail(`Governed decision import failed:\n${fatal.join('\n')}`);

  const preview = emptyPreview(report, decisions, verifiedSources);
  if (validation.errors.length > 0) {
    preview.incomplete.push(...validation.errors.map((reason) => ({ code: 'INVALID_DECISION', reason })));
  }

  const skills = new Map(report.skills.map((skill) => [skill.stableId, skill]));
  const nodes = nodeMap(report);
  const occupiedOrdinals = new Map((report.nodes ?? [])
    .filter((node) => node.source === 'bkvince' && Number.isInteger(node.ordinal))
    .map((node) => [node.ordinal, node]));
  const appendStart = nextAppendOrdinal(report);
  const ids = selectedSkillIds(report, decisions);
  if (ids.length === 0) {
    preview.incomplete.push({
      code: 'NO_SELECTED_DECISIONS',
      reason: 'The decision export contains no governed skill lot to preview',
    });
  }
  const draft = {
    keptCells: [],
    adoptedCells: [],
    customCells: [],
    appendOnlyRows: [],
    dependencies: [],
    localizations: [],
    consumers: [],
    nativeProofs: [],
    collisions: [],
    tests: [],
  };

  for (const stableId of ids) {
    const skill = skills.get(stableId);
    const entry = decisions.entries?.[stableId];
    if (!skill) {
      preview.conflicts.push({ code: 'UNKNOWN_STABLE_ID', stableId });
      continue;
    }
    if (!entry) {
      preview.incomplete.push({ code: 'DECISION_ENTRY_MISSING', stableId, reason: 'ALL export requires an entry for every governed non-read-only skill' });
      continue;
    }
    draft.collisions.push(...collisionObjects(report, skill));
    const state = entryState(report, skill, entry);
    if (!state.complete) {
      preview.incomplete.push({ code: 'DECISION_INCOMPLETE', stableId, reasons: state.reasons });
      continue;
    }

    const importingNew = skill.newPd2PlayerSkill === true
      && ['IMPORT_APPEND_ONLY', 'IMPORT_CUSTOMIZED'].includes(entry.newSkillLineDecision);
    const pd2ModelSelected = importingNew || ['ADAPT_PD2_SELECTIVELY', 'ADOPT_PD2_MODEL'].includes(entry.globalDecision);
    const targetOrdinal = importingNew
      ? proposedOrdinal(skill)
      : (sourceNode(skill, 'bkvince', nodes)?.ordinal ?? skill.ordinals?.bkvince ?? null);
    const selectedFieldIds = new Set();
    const pendingFieldChanges = [];

    if (entry.globalDecision === 'REJECT_PD2' || entry.globalDecision === 'DEFER_NATIVE_PROOF') {
      draft.tests.push(...(nonBlank(entry.notes?.testPlan) ? [{ stableId, plan: entry.notes.testPlan }] : []));
      continue;
    }

    for (const component of skill.components ?? []) {
      for (const field of component.fields ?? []) {
        if (field.changed === false) continue;
        const choice = resolveFieldChoice(skill, entry, component, field);
        if (!choice || choice.decision === 'DISCUSS') continue;
        const base = {
          stableId,
          skillFingerprint: skill.fingerprint,
          componentId: component.id,
          componentFingerprint: component.fingerprint ?? null,
          fieldId: field.id,
          table: field.table,
          header: fieldHeader(field, 'bkvince'),
          sourceHeader: fieldHeader(field, 'pd2'),
          sourceRow: fieldSourceRow(skill, field, nodes),
          targetRow: fieldTargetRow(skill, field, nodes),
          before: fieldValue(field, 'bkvince'),
          proofStatus: field.proofStatus ?? component.proofStatus,
          protected: field.protected === true,
          protectionReasons: clone(field.protectionReasons ?? []),
          decision: choice.decision,
        };
        if (choice.decision === 'KEEP_BKVINCE' || choice.decision === 'NOT_APPLICABLE') {
          draft.keptCells.push({ ...base, reason: choice.decision });
          continue;
        }
        if (choice.decision !== 'ADOPT_PD2' && choice.decision !== 'CUSTOM') continue;
        selectedFieldIds.add(field.id);
        const risks = protectedRiskErrors(skill, component, field, choice);
        preview.conflicts.push(...risks);
        if (nativeRisk(field, component)) {
          draft.nativeProofs.push({
            stableId,
            fieldId: field.id,
            proofStatus: field.proofStatus ?? component.proofStatus,
            override: clone(choice.protectedOverride ?? null),
            ready: !risks.some((risk) => risk.code === 'NATIVE_PROOF_REQUIRED'),
          });
        }
        const after = choice.decision === 'CUSTOM' ? choice.customValue : fieldValue(field, 'pd2');
        if (after === undefined) {
          preview.conflicts.push({ code: 'PD2_VALUE_MISSING', stableId, fieldId: field.id, reason: 'Selected field has no exact governed PD2 value' });
          continue;
        }
        if (!importingNew && (!nonBlank(field.table) || base.targetRow === null)) {
          preview.conflicts.push({ code: 'TARGET_LOCATOR_MISSING', stableId, fieldId: field.id, table: field.table ?? null, reason: 'An exact target table/row locator is required' });
          continue;
        }
        if (!importingNew && base.targetRow !== targetOrdinal && normalizeTable(field.table) === 'skills.txt') {
          preview.conflicts.push({ code: 'BKVINCE_ROW_MOVE_FORBIDDEN', stableId, fieldId: field.id, targetRow: base.targetRow, actualOrdinal: targetOrdinal });
          continue;
        }
        if (!importingNew && /runtime.?ordinal|row.?order/i.test(`${field.id} ${field.header}`) && after !== base.before) {
          preview.conflicts.push({ code: 'BKVINCE_ROW_MOVE_FORBIDDEN', stableId, fieldId: field.id, reason: 'An existing BKVince row cannot move' });
          continue;
        }
        pendingFieldChanges.push({
          ...base,
          after,
          justification: choice.justification ?? null,
          gameplayObjective: choice.gameplayObjective ?? null,
          testPlan: choice.testPlan ?? null,
          protectedOverride: clone(choice.protectedOverride ?? null),
        });
      }
    }

    const dependencies = dependencyAudit(skill, selectedFieldIds, importingNew);
    draft.dependencies.push(...dependencies.dependencies.map((dependency) => ({ stableId, ...dependency })));
    preview.conflicts.push(...dependencies.missingReferences, ...dependencies.unclosed);

    const localization = localizationAudit(skill, importingNew);
    draft.localizations.push(...localization.items.map((item) => ({ stableId, ...item })));
    preview.conflicts.push(...localization.missing);

    const consumer = consumerAudit(skill, targetOrdinal, pd2ModelSelected, importingNew);
    draft.consumers.push(...consumer.consumers.map((item) => ({ stableId, ...item })));
    preview.conflicts.push(...consumer.failures);

    if (nonBlank(entry.notes?.testPlan)) draft.tests.push({ stableId, source: 'skill-notes', plan: entry.notes.testPlan });
    for (const change of pendingFieldChanges) {
      if (nonBlank(change.testPlan)) draft.tests.push({ stableId, fieldId: change.fieldId, source: 'field-choice', plan: change.testPlan });
    }
    for (const item of skill?.newSkillPlan?.tests ?? skill?.newSkillPlan?.testsRequired ?? []) {
      draft.tests.push({ stableId, source: 'new-skill-plan', plan: typeof item === 'string' ? item : item.plan ?? item.description ?? JSON.stringify(item) });
    }

    if (importingNew) {
      if (!Number.isInteger(targetOrdinal)) {
        preview.conflicts.push({ code: 'APPEND_ORDINAL_MISSING', stableId, reason: 'The generator must propose a calculated append-only ordinal' });
      } else if (targetOrdinal < appendStart) {
        preview.conflicts.push({ code: 'NON_APPEND_INSERTION', stableId, proposedOrdinal: targetOrdinal, nextAppendOrdinal: appendStart });
      } else if (occupiedOrdinals.has(targetOrdinal)) {
        preview.conflicts.push({ code: 'APPEND_ORDINAL_COLLISION', stableId, proposedOrdinal: targetOrdinal, occupant: occupiedOrdinals.get(targetOrdinal).id });
      }
      const rowProjection = proposedAppendRow(skill);
      if (!rowProjection) {
        preview.conflicts.push({ code: 'APPEND_ROW_PROJECTION_MISSING', stableId, reason: 'No governed BKVince-schema row projection exists for this new skill' });
      } else {
        const row = rowProjection.values;
        const proposalMetadata = rowProjection.metadata;
        if (proposalMetadata?.targetOrdinal !== null && proposalMetadata.targetOrdinal !== targetOrdinal) {
          preview.conflicts.push({
            code: 'APPEND_PROJECTION_ORDINAL_MISMATCH',
            stableId,
            proposedOrdinal: targetOrdinal,
            projectionOrdinal: proposalMetadata.targetOrdinal,
          });
        }
        if (proposalMetadata?.targetTable && normalizeTable(proposalMetadata.targetTable) !== 'skills.txt') {
          preview.conflicts.push({
            code: 'APPEND_PROJECTION_TABLE_MISMATCH',
            stableId,
            targetTable: proposalMetadata.targetTable,
          });
        }
        if (Array.isArray(proposalMetadata?.targetHeaders)) {
          const rowHeaders = Object.keys(row);
          const governedHeaders = proposalMetadata.targetHeaders;
          if (rowHeaders.length !== governedHeaders.length
            || rowHeaders.some((header, index) => header !== governedHeaders[index])) {
            preview.conflicts.push({
              code: 'APPEND_PROJECTION_HEADERS_MISMATCH',
              stableId,
              rowHeaderCount: rowHeaders.length,
              targetHeaderCount: governedHeaders.length,
              reason: 'The proposed row values must preserve the complete governed BKVince header order',
            });
          }
        }
        if (proposalMetadata && !isObject(proposalMetadata.mappingProvenance)) {
          preview.conflicts.push({
            code: 'APPEND_PROJECTION_PROVENANCE_MISSING',
            stableId,
            reason: 'A canonical proposedRow must explain every mapped/defaulted BKVince cell',
          });
        } else if (isObject(proposalMetadata?.mappingProvenance)) {
          const withoutProvenance = Object.keys(row).filter((header) => !Object.hasOwn(proposalMetadata.mappingProvenance, header));
          if (withoutProvenance.length > 0) {
            preview.conflicts.push({
              code: 'APPEND_PROJECTION_PROVENANCE_INCOMPLETE',
              stableId,
              headers: withoutProvenance,
            });
          }
        }
        for (const change of pendingFieldChanges) {
          let header = change.header;
          if (!Object.hasOwn(row, header) && isObject(proposalMetadata?.mappingProvenance)) {
            const matches = Object.entries(proposalMetadata.mappingProvenance)
              .filter(([targetHeader, provenance]) => {
                const targetMatches = canonicalHeader(targetHeader) === canonicalHeader(change.header);
                const documentaryId = provenance?.mode === 'APPEND_PREVIEW_DOCUMENTARY_VALUE'
                  && canonicalHeader(targetHeader) === 'id'
                  && canonicalHeader(change.header) === 'id';
                return targetMatches && (documentaryId
                  || canonicalHeader(provenance?.sourceHeader) === canonicalHeader(change.sourceHeader));
              })
              .map(([targetHeader]) => targetHeader);
            if (matches.length === 1) header = matches[0];
            else if (matches.length > 1) {
              preview.conflicts.push({
                code: 'APPEND_FIELD_MAPPING_AMBIGUOUS',
                stableId,
                fieldId: change.fieldId,
                sourceHeader: change.sourceHeader,
                targetHeaders: matches,
              });
              continue;
            }
          }
          if (!nonBlank(header) || !Object.hasOwn(row, header)) {
            preview.conflicts.push({ code: 'APPEND_FIELD_LOCATOR_MISSING', stableId, fieldId: change.fieldId, header: header ?? null });
            continue;
          }
          change.header = header;
          if (change.decision === 'CUSTOM') row[header] = change.after;
          else if (change.decision === 'ADOPT_PD2') row[header] = change.after;
        }
        draft.appendOnlyRows.push({
          stableId,
          skillFingerprint: skill.fingerprint,
          sourceOrdinal: sourceNode(skill, 'pd2', nodes)?.ordinal ?? skill.ordinals?.pd2 ?? null,
          proposedOrdinal: targetOrdinal,
          table: skill?.newSkillPlan?.table ?? 'skills.txt',
          row,
          lineDecision: entry.newSkillLineDecision,
          projectionMetadata: proposalMetadata,
          fieldChanges: pendingFieldChanges,
        });
      }
    } else {
      for (const change of pendingFieldChanges) {
        if (change.decision === 'CUSTOM') draft.customCells.push(change);
        else draft.adoptedCells.push(change);
      }
    }
  }

  const appendRows = [...draft.appendOnlyRows].sort((left, right) => left.proposedOrdinal - right.proposedOrdinal || left.stableId.localeCompare(right.stableId));
  const appendSeen = new Map();
  appendRows.forEach((row, index) => {
    if (appendSeen.has(row.proposedOrdinal)) {
      preview.conflicts.push({ code: 'APPEND_ORDINAL_COLLISION', stableId: row.stableId, otherStableId: appendSeen.get(row.proposedOrdinal), proposedOrdinal: row.proposedOrdinal });
    } else appendSeen.set(row.proposedOrdinal, row.stableId);
    const expected = appendStart + index;
    if (row.proposedOrdinal !== expected) {
      preview.conflicts.push({ code: 'APPEND_SEQUENCE_GAP', stableId: row.stableId, proposedOrdinal: row.proposedOrdinal, expectedOrdinal: expected, reason: 'Selected append-only rows must form a contiguous suffix' });
    }
  });

  const duplicateConflictKeys = new Set();
  preview.conflicts = preview.conflicts.filter((conflict) => {
    const key = JSON.stringify(conflict);
    if (duplicateConflictKeys.has(key)) return false;
    duplicateConflictKeys.add(key);
    return true;
  });
  const dependencyNodes = new Map();
  const dependencyEdges = [];
  for (const dependency of draft.dependencies) {
    const id = `${dependency.stableId}::${dependency.id}`;
    dependencyNodes.set(id, { id, stableId: dependency.stableId, dependencyId: dependency.id, table: dependency.table ?? null, status: dependency.closureStatus ?? dependency.status ?? null });
    dependencyEdges.push({ from: dependency.stableId, to: id, kind: dependency.kind ?? dependency.type ?? 'REQUIRES' });
  }
  preview.dependencyGraph = { nodes: [...dependencyNodes.values()], edges: dependencyEdges };
  preview.localizations = draft.localizations;
  preview.consumers = draft.consumers;
  preview.nativeProofs = draft.nativeProofs;
  preview.collisions = draft.collisions;
  preview.tests = draft.tests;
  const ready = preview.conflicts.length === 0 && preview.incomplete.length === 0;
  if (!ready) return preview;

  preview.ready = true;
  preview.applicable = true;
  preview.keptCells = draft.keptCells;
  preview.adoptedCells = draft.adoptedCells;
  preview.customCells = draft.customCells;
  preview.appendOnlyRows = appendRows;
  preview.proposedCells = [...draft.adoptedCells, ...draft.customCells];
  preview.proposedRows = appendRows;
  const exactChanges = [...draft.adoptedCells, ...draft.customCells];
  preview.exactChangesByTable = Object.groupBy(exactChanges, (change) => change.table);
  for (const row of appendRows) {
    if (!preview.exactChangesByTable[row.table]) preview.exactChangesByTable[row.table] = [];
    preview.exactChangesByTable[row.table].push({ kind: 'APPEND_ROW', ...row });
  }
  preview.commitSlices = Object.keys(preview.exactChangesByTable).sort().map((table, index) => ({
    order: index + 1,
    id: `table-${String(index + 1).padStart(2, '0')}-${normalizeTable(table).replace(/\.txt$/i, '')}`,
    table,
    changeCount: preview.exactChangesByTable[table].length,
    purpose: 'Preview-only proposed data slice; implementation remains unauthorized',
  }));
  if (preview.localizations.some((item) => item.action === 'ADD_REQUIRED')) {
    preview.commitSlices.push({
      order: preview.commitSlices.length + 1,
      id: 'localization',
      table: null,
      changeCount: preview.localizations.filter((item) => item.action === 'ADD_REQUIRED').length,
      purpose: 'Preview-only required localization slice',
    });
  }
  if (preview.tests.length > 0) {
    preview.commitSlices.push({
      order: preview.commitSlices.length + 1,
      id: 'tests',
      table: null,
      changeCount: preview.tests.length,
      purpose: 'Tests required before any separately authorized prototype',
    });
  }
  preview.diffPreview = [
    ...draft.adoptedCells.map((change) => `${change.table}[${change.targetRow}].${change.header}: ${JSON.stringify(change.before)} -> ${JSON.stringify(change.after)} [PD2]`),
    ...draft.customCells.map((change) => `${change.table}[${change.targetRow}].${change.header}: ${JSON.stringify(change.before)} -> ${JSON.stringify(change.after)} [CUSTOM]`),
    ...appendRows.map((row) => `${row.table}: append proposed ordinal ${row.proposedOrdinal} from PD2 ordinal ${row.sourceOrdinal} (${row.stableId})`),
    ...draft.localizations.filter((item) => item.action === 'ADD_REQUIRED').map((item) => `localization ${item.key}: add ${JSON.stringify(item.value)} (${item.stableId})`),
  ];
  preview.textualDiff = preview.diffPreview.join('\n');
  preview.proposedManifest = {
    previewOnly: true,
    gameplayApplicationForbidden: true,
    implementationAuthorized: false,
    selectedSkills: ids.length,
    changedCells: exactChanges.length,
    keptCells: draft.keptCells.length,
    appendedRows: appendRows.length,
    appendStartOrdinal: appendStart,
    tables: Object.fromEntries(Object.entries(preview.exactChangesByTable).map(([table, changes]) => [table, changes.length])),
    dependencyNodes: preview.dependencyGraph.nodes.length,
    localizationsRequired: preview.localizations.filter((item) => item.action === 'ADD_REQUIRED').length,
    consumersAudited: preview.consumers.length,
    nativeProofsAcknowledged: preview.nativeProofs.filter((item) => item.ready).length,
    collisionsDocumented: preview.collisions.length,
    testsRequired: preview.tests.length,
  };
  return preview;
}

export function assertSafeOutputPath(rawOutput, options = {}) {
  const root = path.resolve(options.repoRoot ?? repoRoot);
  const resolved = path.resolve(options.cwd ?? process.cwd(), rawOutput);
  const lexicalRoots = allowedOutputRoots(root);
  if (!lexicalRoots.some((allowed) => isWithin(resolved, allowed))) {
    fail(`Preview output must stay under ${NON_MUTATION_RULES.allowedGeneratedRoots.join(' or ')}`);
  }
  if (fs.existsSync(resolved) && fs.lstatSync(resolved).isSymbolicLink()) {
    fail('Preview output cannot follow a symbolic link');
  }
  let existing = path.dirname(resolved);
  while (!fs.existsSync(existing)) {
    const parent = path.dirname(existing);
    if (parent === existing) fail('Cannot resolve a safe existing output parent');
    existing = parent;
  }
  const realParent = fs.realpathSync(existing);
  const realRepoRoot = fs.realpathSync(root);
  const realRoots = lexicalRoots.filter(fs.existsSync).map((allowed) => {
    if (fs.lstatSync(allowed).isSymbolicLink()) fail('Governed generated root cannot be a symbolic link');
    const real = fs.realpathSync(allowed);
    if (!isWithin(real, realRepoRoot)) fail('Governed generated root resolves outside the repository');
    return real;
  });
  if (!realRoots.some((allowed) => isWithin(realParent, allowed))) {
    fail('Preview output parent resolves outside governed generated roots');
  }
  for (const protectedInput of options.protectedInputs ?? []) {
    if (path.resolve(protectedInput) === resolved) fail('Preview output cannot overwrite an input or governed report');
  }
  return resolved;
}

export function parseCli(args = process.argv.slice(2)) {
  if (args.some((argument) => argument === '--apply' || argument.startsWith('--apply='))) {
    fail('Gameplay application is forbidden; this compiler is preview-only');
  }
  let decisions = null;
  let report = defaultReportPath;
  let output = null;
  for (let index = 0; index < args.length; index += 1) {
    const argument = args[index];
    if (argument === '--report' || argument === '--output') {
      const value = args[index + 1];
      if (!value || value.startsWith('--')) fail(`${argument} requires a path`);
      if (argument === '--report') report = path.resolve(value);
      else output = value;
      index += 1;
    } else if (argument.startsWith('--report=')) report = path.resolve(argument.slice('--report='.length));
    else if (argument.startsWith('--output=')) output = argument.slice('--output='.length);
    else if (argument.startsWith('--')) fail(`Unknown option ${argument}`);
    else if (decisions === null) decisions = path.resolve(argument);
    else fail(`Unexpected positional argument ${argument}`);
  }
  if (!decisions) fail('usage: node pd2-skills-decisions-preview.mjs <decisions.json> [--report=<oracle.json>] [--output=<Mission|analysis-cache path>]');
  return { decisions, report, output };
}

export function runCli(args = process.argv.slice(2)) {
  const parsed = parseCli(args);
  const report = readJson(parsed.report);
  const decisions = readJson(parsed.decisions);
  const preview = compilePreview(report, decisions, { repoRoot });
  const raw = `${JSON.stringify(preview, null, 2)}\n`;
  if (parsed.output) {
    const output = assertSafeOutputPath(parsed.output, {
      repoRoot,
      protectedInputs: [parsed.report, parsed.decisions],
    });
    fs.writeFileSync(output, raw, 'utf8');
  } else process.stdout.write(raw);
  if (!preview.ready) process.exitCode = 1;
  return preview;
}

if (process.argv[1] && path.resolve(process.argv[1]) === modulePath) {
  try {
    runCli();
  } catch (error) {
    console.error(`INVALID PD2 Skill Decisions: ${error.message}`);
    process.exitCode = 1;
  }
}
