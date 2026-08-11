import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const repoRoot = fileURLToPath(new URL('../../', import.meta.url));
const reportPath = path.join(repoRoot, 'Mission', 'pd2-affixes-review.json');

function fail(message) { throw new Error(message); }
function readJson(file) { return JSON.parse(fs.readFileSync(file, 'utf8').replace(/^\uFEFF/, '')); }

export function validateDecisionExport(report, decisions) {
  if (decisions.schemaVersion !== 2) fail('unsupported decision schemaVersion');
  for (const key of ['reviewId', 'comparisonHash', 'targetBaselineCommit']) {
    if (decisions[key] !== report[key]) fail(`${key} does not match the governed comparison`);
  }
  if (JSON.stringify(decisions.sourceHashes) !== JSON.stringify(report.sourceHashes)) fail('PD2 source hashes do not match');
  const byId = new Map(report.entries.map((entry) => [entry.id, entry]));
  for (const [id, choice] of Object.entries(decisions.entries ?? {})) {
    const entry = byId.get(id);
    if (!entry || choice.fingerprint !== entry.fingerprint) fail(`occurrence fingerprint does not match: ${id}`);
  }
  return byId;
}

export function compilePreview(report, decisions) {
  const byId = validateDecisionExport(report, decisions);
  const cells = [], rows = [], conflicts = [], incomplete = [], dependencies = new Set(), localizations = new Set();
  for (const entry of report.entries) {
    if (entry.deferred || entry.table === 'automagic.txt') continue;
    const selected = decisions.entries?.[entry.id];
    if (!selected) { if (entry.fieldDifferences.length) incomplete.push({ id: entry.id, reason: 'no decisions' }); continue; }
    let unresolved = false;
    const projected = { ...(entry.rows.bkvince ?? {}) };
    for (const diff of entry.fieldDifferences) {
      const choice = selected.fields?.[diff.field];
      const decision = choice?.decision ?? diff.defaultDecision;
      if (!decision || decision === 'DISCUSS') { unresolved = true; incomplete.push({ id: entry.id, field: diff.field, reason: decision || 'undecided' }); continue; }
      if (decision === 'KEEP_BKVINCE') continue;
      if (decision === 'ADOPT_PD2') {
        if (diff.protected && choice?.protectedOverride !== true) { conflicts.push({ id: entry.id, field: diff.field, reason: 'protected MaxLevel requires explicit override' }); continue; }
        projected[diff.field] = diff.pd2 ?? '';
      } else if (decision === 'CUSTOM') {
        if (choice.customValue === undefined || !String(choice.notes ?? '').trim()) { incomplete.push({ id: entry.id, field: diff.field, reason: 'CUSTOM requires value and notes' }); unresolved = true; continue; }
        projected[diff.field] = String(choice.customValue);
      } else conflicts.push({ id: entry.id, field: diff.field, reason: `unknown decision ${decision}` });
      if (entry.rows.bkvince) cells.push({ table: entry.table, targetRow: entry.targetRow, id: entry.id, field: diff.field, before: diff.bkvince, after: projected[diff.field], decision });
    }
    if (!entry.rows.bkvince && !unresolved && Object.keys(projected).length) {
      rows.push({ table: entry.table, sourceRow: entry.sourceRow, id: entry.id, fingerprint: entry.fingerprint, row: projected });
      for (let i = 1; i <= 3; i += 1) if (projected[`mod${i}code`]) dependencies.add(`property:${projected[`mod${i}code`]}`);
      for (let i = 1; i <= 7; i += 1) if (projected[`itype${i}`]) dependencies.add(`itemtype:${projected[`itype${i}`]}`);
      if (projected.Name) localizations.add(projected.Name);
    }
  }
  return {
    schemaVersion: 1, previewId: 'pd2-affixes-decisions-preview-v1', state: 'PREVIEW_ONLY_APPLICATION_FORBIDDEN',
    reviewId: report.reviewId, comparisonHash: report.comparisonHash, targetBaselineCommit: report.targetBaselineCommit,
    proposedManifest: { changedCells: cells.length, appendedRows: rows.length, dependencyCount: dependencies.size, localizationCount: localizations.size },
    cells, rows, dependencies: [...dependencies].sort(), localizations: [...localizations].sort(), conflicts, incomplete,
    diffPreview: [...cells.map((c) => `${c.table}[${c.targetRow}].${c.field}: ${JSON.stringify(c.before)} -> ${JSON.stringify(c.after)}`), ...rows.map((r) => `${r.table}: append PD2 source row ${r.sourceRow} (${r.id})`)],
  };
}

function run(args = process.argv.slice(2)) {
  if (args.includes('--apply')) fail('gameplay application is forbidden; this compiler is preview-only');
  const input = args.find((arg) => !arg.startsWith('--'));
  if (!input) fail('usage: node pd2-affixes-decisions-preview.mjs <decisions.json> [--output=file]');
  const report = readJson(reportPath), decisions = readJson(path.resolve(input));
  const preview = compilePreview(report, decisions), raw = `${JSON.stringify(preview, null, 2)}\n`;
  const output = args.find((arg) => arg.startsWith('--output='));
  if (output) fs.writeFileSync(path.resolve(output.slice('--output='.length)), raw, 'utf8'); else process.stdout.write(raw);
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try { run(); } catch (error) { console.error(`INVALID PD2 Affix Decisions: ${error.message}`); process.exitCode = 1; }
}
