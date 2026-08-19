import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import {
  auditAffixProjection,
  buildAffixDependencyAuditContext,
  loadTable,
  planAffixLocalization,
  resolvePd2AffixSourceRoot,
} from './pd2-affixes-merge.mjs';
import {
  effectiveFieldChoice,
  entryReviewState,
  fieldChoiceComplete,
  isNewAffix,
  validateDecisionEnvelope,
} from './pd2-affixes-decision-rules.mjs';

const repoRoot = fileURLToPath(new URL('../../', import.meta.url));
const reportPath = path.join(repoRoot, 'Mission', 'pd2-affixes-review.json');
const catalogPath = path.join(repoRoot, 'Mission', 'pd2-affixes-merge.catalog.json');
const targetRoot = path.join(repoRoot, 'data-BKVince', 'BKVince.mpq', 'data', 'global', 'excel');
export const APPROVED_MAP_EXCLUSION_NOTE = 'NOT APPLICABLE TO CURRENT BKVINCE';

function fail(message) { throw new Error(message); }
function readJson(file) { return JSON.parse(fs.readFileSync(file, 'utf8').replace(/^\uFEFF/, '')); }

export function validateDecisionExport(report, decisions) {
  validateDecisionEnvelope(report, decisions);
  const byId = new Map(report.entries.map((entry) => [entry.id, entry]));
  return byId;
}

export function applyApprovedMapExclusions(report, decisions, mapAffixes) {
  if (mapAffixes?.occurrenceCount !== 248 || !Array.isArray(mapAffixes.families)) {
    fail('the governed map exclusion must contain exactly 248 occurrences');
  }
  const occurrenceIds = [...new Set(mapAffixes.families.flatMap((family) => family.occurrenceIds ?? []))];
  if (occurrenceIds.length !== mapAffixes.occurrenceCount) {
    fail(`the governed map exclusion resolves ${occurrenceIds.length}/248 unique occurrences`);
  }
  const byId = new Map(report.entries.map((entry) => [entry.id, entry]));
  const updated = structuredClone(decisions);
  updated.entries ??= {};
  for (const id of occurrenceIds) {
    const entry = byId.get(id);
    if (!entry || !isNewAffix(entry)) fail(`map exclusion references a non-new or missing affix: ${id}`);
    updated.entries[id] = {
      ...(updated.entries[id] ?? {}),
      fingerprint: entry.fingerprint,
      lineDecision: 'EXCLUDE_PD2_AFFIX',
      notes: APPROVED_MAP_EXCLUSION_NOTE,
      fields: { ...(updated.entries[id]?.fields ?? {}) },
    };
  }
  return updated;
}

export function poisonDamageMetrics(encodedDamage, durationFrames) {
  if (!Number.isSafeInteger(encodedDamage) || encodedDamage < 0) fail('poison encoded damage must be a non-negative integer');
  if (!Number.isSafeInteger(durationFrames) || durationFrames <= 0) fail('poison duration must be a positive frame count');
  return {
    encodedDamage,
    durationFrames,
    durationSeconds: durationFrames / 25,
    totalDamage: (encodedDamage * durationFrames) / 256,
    damagePerSecond: (encodedDamage * 25) / 256,
  };
}

export function closestPoisonTotalEquivalent(encodedDamage, durationFrames, {
  maximumEncodedDamage = 1023,
  maximumDurationFrames = 511,
} = {}) {
  const wantedProduct = encodedDamage * durationFrames;
  let best = null;
  for (let candidateDuration = 1; candidateDuration <= maximumDurationFrames; candidateDuration += 1) {
    const candidateDamage = Math.min(
      maximumEncodedDamage,
      Math.max(0, Math.round(wantedProduct / candidateDuration)),
    );
    const error = Math.abs((candidateDamage * candidateDuration) - wantedProduct);
    const candidate = {
      ...poisonDamageMetrics(candidateDamage, candidateDuration),
      productError: error,
      durationDistance: Math.abs(candidateDuration - durationFrames),
    };
    if (!best
      || candidate.productError < best.productError
      || (candidate.productError === best.productError && candidate.durationDistance < best.durationDistance)
      || (candidate.productError === best.productError
        && candidate.durationDistance === best.durationDistance
        && candidate.encodedDamage > best.encodedDamage)) best = candidate;
  }
  return best;
}

function assertCurrentBaseline(report) {
  for (const [table, expected] of Object.entries(report.targetBaselineHashes)) {
    const actual = loadTable(targetRoot, table).sha256;
    if (actual !== expected) fail(`${table}: current BKVince baseline hash ${actual} does not match review ${expected}`);
  }
}

function assertDependencyHashes(report, context) {
  if (!report.dependencyHashes) fail('review dependencyHashes are missing');
  if (!context?.dependencyHashes) fail('dependency audit context hashes are missing');
  if (JSON.stringify(context.dependencyHashes) !== JSON.stringify(report.dependencyHashes)) {
    fail('source or BKVince dependencyHashes do not match the governed review');
  }
}

function applyFields(entry, selection, projected) {
  const provenanceByField = Object.fromEntries(Object.keys(projected).map((field) => [field, entry.rows.bkvince ? 'BKVINCE' : 'PD2']));
  const changed = [];
  const conflicts = [];
  for (const diff of entry.fieldDifferences) {
    const choice = effectiveFieldChoice(entry, selection, diff);
    if (!fieldChoiceComplete(diff, choice)) continue;
    if (choice.decision === 'KEEP_BKVINCE') {
      if (!entry.rows.bkvince) conflicts.push({ id: entry.id, field: diff.field, reason: 'KEEP_BKVINCE is invalid for a row that does not exist in BKVince' });
      continue;
    }
    let after;
    if (choice.decision === 'ADOPT_PD2') after = diff.pd2 ?? '';
    else if (choice.decision === 'CUSTOM') after = choice.customValue;
    else continue;
    const before = projected[diff.field] ?? '';
    projected[diff.field] = after;
    provenanceByField[diff.field] = choice.decision === 'CUSTOM' ? 'CUSTOM' : 'PD2';
    if (before !== after) changed.push({ field: diff.field, before, after, decision: choice.decision, notes: choice.notes ?? null });
  }
  return { projected, provenanceByField, changed, conflicts };
}

export function compilePreview(report, decisions, { catalog = readJson(catalogPath), context = null, verifyBaseline = false, sourceRoot = null } = {}) {
  validateDecisionExport(report, decisions);
  if (verifyBaseline) assertCurrentBaseline(report);
  const auditContext = context ?? buildAffixDependencyAuditContext(sourceRoot ?? resolvePd2AffixSourceRoot(), targetRoot);
  assertDependencyHashes(report, auditContext);
  const cells = [], rows = [], conflicts = [], incomplete = [], rejectedRows = [], autoResolved = [], audits = [];
  for (const entry of report.entries) {
    const selection = decisions.entries?.[entry.id] ?? { fingerprint: entry.fingerprint, fields: {}, notes: '' };
    const state = entryReviewState(entry, selection);
    if (!state.required) {
      autoResolved.push({ id: entry.id, category: entry.category, resolution: entry.category === 'UNCHANGED_BY_PD2' ? 'KEEP_BKVINCE' : 'INFORMATIONAL_OR_DEFERRED' });
      continue;
    }
    if (isNewAffix(entry) && selection.lineDecision === 'EXCLUDE_PD2_AFFIX') {
      rejectedRows.push({ id: entry.id, fingerprint: entry.fingerprint, table: entry.table, sourceRow: entry.sourceRow, decision: selection.lineDecision, notes: selection.notes ?? '' });
      continue;
    }
    if (!state.complete) {
      incomplete.push({ id: entry.id, reasons: state.reasons });
      continue;
    }
    const kind = isNewAffix(entry) ? 'append' : 'existing';
    const base = { ...(kind === 'append' ? entry.rows.pd2 : entry.rows.bkvince) };
    const projection = applyFields(entry, selection, base);
    if (kind === 'append' && projection.projected.multiply === '') projection.projected.multiply = '0';
    if (projection.conflicts.length) {
      conflicts.push(...projection.conflicts);
      continue;
    }
    const audit = auditAffixProjection(auditContext, {
      tableName: entry.table,
      sourceRow: entry.sourceRow,
      targetRow: entry.targetRow,
      projected: projection.projected,
      sourceOriginal: entry.rows.pd2,
      kind,
      catalog,
      provenanceByField: projection.provenanceByField,
    });
    audits.push({ id: entry.id, table: entry.table, sourceRow: entry.sourceRow, targetRow: entry.targetRow, ...audit });
    if (audit.conflicts.length) {
      conflicts.push(...audit.conflicts.map((conflict) => ({ id: entry.id, ...conflict })));
      continue;
    }
    if (kind === 'append') {
      rows.push({ table: entry.table, sourceRow: entry.sourceRow, id: entry.id, fingerprint: entry.fingerprint, row: projection.projected, provenanceByField: projection.provenanceByField, lineDecision: selection.lineDecision });
    } else {
      for (const change of projection.changed) cells.push({ table: entry.table, targetRow: entry.targetRow, sourceRow: entry.sourceRow, id: entry.id, ...change });
    }
  }
  const dependencySummary = audits.flatMap((audit) => audit.dependencies.map((dependency) => ({ id: audit.id, ...dependency })))
    .reduce((summary, dependency) => {
      const key = `${dependency.kind}:${dependency.status}`;
      if (!summary[key]) summary[key] = [];
      summary[key].push(dependency);
      return summary;
    }, {});
  const localizationPlan = planAffixLocalization(audits.map((audit) => audit.localization.key), catalog, auditContext);
  if (localizationPlan.conflicts.length) conflicts.push(...localizationPlan.conflicts);
  const appendCounts = Object.groupBy(rows, (row) => row.table);
  const serializationPlan = Object.fromEntries(Object.entries(report.targetBaselineHashes).map(([table]) => {
    const targetTable = auditContext.tables[table]?.target?.table ?? loadTable(targetRoot, table).table;
    const baseline = targetTable.rows.filter((row) => row[0] !== 'Expansion').length;
    const appended = appendCounts[table]?.length ?? 0;
    return [table, { baselineCompiledRows: baseline, appendedRows: appended, projectedCompiledRows: baseline + appended, within11BitLimit: baseline + appended <= 2047 }];
  }));
  const unifiedCount = Object.values(serializationPlan).reduce((total, table) => total + table.projectedCompiledRows, 0);
  const ready = conflicts.length === 0 && incomplete.length === 0 && Object.values(serializationPlan).every((table) => table.within11BitLimit) && unifiedCount <= 65535;
  return {
    schemaVersion: 3,
    previewId: 'pd2-affixes-decisions-preview-v3',
    state: 'PREVIEW_ONLY_APPLICATION_FORBIDDEN',
    ready,
    reviewId: report.reviewId,
    comparisonHash: report.comparisonHash,
    dependencyHashes: report.dependencyHashes,
    targetBaselineCommit: report.targetBaselineCommit,
    targetBaselineHashes: report.targetBaselineHashes,
    proposedManifest: { changedCells: cells.length, appendedRows: rows.length, rejectedRows: rejectedRows.length, auditedOccurrences: audits.length, conflicts: conflicts.length, incomplete: incomplete.length },
    cells,
    rows,
    rejectedRows,
    autoResolved,
    dependencyAudit: { summary: dependencySummary, occurrences: audits },
    localizations: localizationPlan,
    serializationPlan: { tables: serializationPlan, projectedUnifiedCount: unifiedCount, withinUint16Limit: unifiedCount <= 65535 },
    conflicts,
    incomplete,
    diffPreview: [...cells.map((cell) => `${cell.table}[${cell.targetRow}].${cell.field}: ${JSON.stringify(cell.before)} -> ${JSON.stringify(cell.after)}`), ...rows.map((row) => `${row.table}: append PD2 source row ${row.sourceRow} (${row.id})`), ...rejectedRows.map((row) => `${row.table}: exclude PD2 source row ${row.sourceRow} (${row.id})`)],
  };
}

function run(args = process.argv.slice(2)) {
  if (args.includes('--apply')) fail('gameplay application is forbidden; this compiler is preview-only');
  const input = args.find((arg) => !arg.startsWith('--'));
  if (!input) fail('usage: node pd2-affixes-decisions-preview.mjs <decisions.json> [--output=file]');
  const report = readJson(reportPath), decisions = readJson(path.resolve(input));
  const preview = compilePreview(report, decisions, {
    verifyBaseline: true,
    sourceRoot: resolvePd2AffixSourceRoot(args),
  }), raw = `${JSON.stringify(preview, null, 2)}\n`;
  const output = args.find((arg) => arg.startsWith('--output='));
  if (output) fs.writeFileSync(path.resolve(output.slice('--output='.length)), raw, 'utf8'); else process.stdout.write(raw);
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try { run(); } catch (error) { console.error(`INVALID PD2 Affix Decisions: ${error.message}`); process.exitCode = 1; }
}
