export const FIELD_DECISIONS = Object.freeze(['KEEP_BKVINCE', 'ADOPT_PD2', 'CUSTOM', 'DISCUSS']);
export const NEW_AFFIX_LINE_DECISIONS = Object.freeze(['IMPORT_PD2_AFFIX', 'EXCLUDE_PD2_AFFIX', 'DISCUSS', 'IMPORT_CUSTOMIZED']);
export const NEW_AFFIX_CATEGORIES = Object.freeze(['PD2_NEW_PORTABLE', 'PD2_NEW_REVIEW']);
export const REVIEW_REQUIRED_CATEGORIES = Object.freeze(['PD2_DELETED', 'PD2_CONSOLIDATED', 'PD2_MODIFIED', ...NEW_AFFIX_CATEGORIES]);
export const READ_ONLY_CATEGORIES = Object.freeze(['UNCHANGED_BY_PD2', 'AUTOMAGIC_DEFERRED']);

export function isNewAffix(entry) {
  return NEW_AFFIX_CATEGORIES.includes(entry.category);
}

export function isReadOnlyEntry(entry) {
  return READ_ONLY_CATEGORIES.includes(entry.category);
}

export function selectedCustomFields(entry, selection = {}) {
  const fields = new Set(entry.fieldDifferences.map((diff) => diff.field));
  return Object.entries(selection.fields ?? {})
    .filter(([field, choice]) => fields.has(field) && choice?.decision === 'CUSTOM')
    .map(([field]) => field);
}

export function selectedUnknownFields(entry, selection = {}) {
  const fields = new Set(entry.fieldDifferences.map((diff) => diff.field));
  return Object.keys(selection.fields ?? {}).filter((field) => !fields.has(field));
}

export function fieldChoiceComplete(diff, choice) {
  const decision = choice?.decision ?? diff.defaultDecision ?? '';
  if (decision === 'KEEP_BKVINCE') return true;
  if (decision === 'ADOPT_PD2') return !diff.protected || choice?.protectedOverride === true;
  if (decision === 'CUSTOM') {
    return typeof choice?.customValue === 'string'
      && String(choice?.notes ?? '').trim().length > 0
      && (!diff.protected || choice?.protectedOverride === true);
  }
  return false;
}

export function effectiveFieldChoice(entry, selection, diff) {
  const explicit = selection.fields?.[diff.field];
  if (explicit) return explicit;
  if (diff.defaultDecision) return { decision: diff.defaultDecision };
  if (isNewAffix(entry) && ['IMPORT_PD2_AFFIX', 'IMPORT_CUSTOMIZED'].includes(selection.lineDecision)) {
    return { decision: 'ADOPT_PD2' };
  }
  return null;
}

export function entryReviewState(entry, selection = {}) {
  if (isReadOnlyEntry(entry)) {
    return { required: false, complete: true, readOnly: true, reasons: [] };
  }
  if (!REVIEW_REQUIRED_CATEGORIES.includes(entry.category) || entry.deferred) {
    return { required: false, complete: true, readOnly: false, reasons: [] };
  }
  if (isNewAffix(entry)) {
    const lineDecision = selection.lineDecision ?? '';
    if (!lineDecision) return { required: true, complete: false, readOnly: false, reasons: ['line decision missing'] };
    if (lineDecision === 'EXCLUDE_PD2_AFFIX') return { required: true, complete: true, readOnly: false, reasons: [] };
    if (lineDecision === 'DISCUSS') return { required: true, complete: false, readOnly: false, reasons: ['line is marked for discussion'] };
    if (!['IMPORT_PD2_AFFIX', 'IMPORT_CUSTOMIZED'].includes(lineDecision)) {
      return { required: true, complete: false, readOnly: false, reasons: [`unknown line decision ${lineDecision}`] };
    }
  }
  const reasons = [];
  for (const field of selectedUnknownFields(entry, selection)) {
    reasons.push(`${field} is not a governed field difference`);
  }
  const customFields = selectedCustomFields(entry, selection);
  if (isNewAffix(entry) && selection.lineDecision === 'IMPORT_PD2_AFFIX' && customFields.length > 0) {
    reasons.push('IMPORT_PD2_AFFIX forbids CUSTOM fields; use IMPORT_CUSTOMIZED');
  }
  if (isNewAffix(entry) && selection.lineDecision === 'IMPORT_CUSTOMIZED' && customFields.length === 0) {
    reasons.push('IMPORT_CUSTOMIZED requires at least one real CUSTOM field');
  }
  for (const diff of entry.fieldDifferences) {
    if (!fieldChoiceComplete(diff, effectiveFieldChoice(entry, selection, diff))) reasons.push(`${diff.field} incomplete`);
  }
  return { required: true, complete: reasons.length === 0, readOnly: false, reasons };
}

export function applyLineFieldAction(entry, selection = {}, action, replace = false) {
  if (isReadOnlyEntry(entry)) return selection;
  const result = { ...selection, fields: { ...(selection.fields ?? {}) } };
  for (const diff of entry.fieldDifferences) {
    if (!replace && Object.hasOwn(result.fields, diff.field)) continue;
    if (action === 'KEEP_BKVINCE') result.fields[diff.field] = { decision: 'KEEP_BKVINCE' };
    else if (action === 'ADOPT_PD2') result.fields[diff.field] = { decision: diff.protected ? 'KEEP_BKVINCE' : 'ADOPT_PD2' };
    else if (action === 'DISCUSS') result.fields[diff.field] = { decision: 'DISCUSS' };
    else throw new Error(`Unknown line field action ${action}`);
  }
  return result;
}

export function decisionExportPayload(report, entries, exportedAt = new Date().toISOString()) {
  const entryById = new Map(report.entries.map((entry) => [entry.id, entry]));
  const writableEntries = Object.fromEntries(Object.entries(entries)
    .filter(([id]) => !isReadOnlyEntry(entryById.get(id) ?? {})));
  return {
    schemaVersion: 3,
    reviewId: report.reviewId,
    comparisonHash: report.comparisonHash,
    sourceHashes: report.sourceHashes,
    dependencyHashes: report.dependencyHashes,
    targetBaselineCommit: report.targetBaselineCommit,
    targetBaselineHashes: report.targetBaselineHashes,
    exportedAt,
    entries: writableEntries,
  };
}

export function validateDecisionEnvelope(report, payload) {
  if (payload?.schemaVersion !== 3) throw new Error('unsupported decision schemaVersion');
  for (const key of ['reviewId', 'comparisonHash', 'targetBaselineCommit']) {
    if (payload[key] !== report[key]) throw new Error(`${key} does not match the governed comparison`);
  }
  for (const key of ['sourceHashes', 'dependencyHashes', 'targetBaselineHashes']) {
    if (JSON.stringify(payload[key]) !== JSON.stringify(report[key])) throw new Error(`${key} do not match`);
  }
  const byId = new Map(report.entries.map((entry) => [entry.id, entry]));
  for (const [id, choice] of Object.entries(payload.entries ?? {})) {
    const entry = byId.get(id);
    if (!entry || choice?.fingerprint !== entry.fingerprint) throw new Error(`occurrence fingerprint does not match: ${id}`);
  }
  return payload;
}
