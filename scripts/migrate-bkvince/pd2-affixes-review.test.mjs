import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';
import { fileURLToPath } from 'node:url';

import { compilePreview, validateDecisionExport } from './pd2-affixes-decisions-preview.mjs';

const repoRoot = fileURLToPath(new URL('../../', import.meta.url));
const report = JSON.parse(fs.readFileSync(path.join(repoRoot, 'Mission', 'pd2-affixes-review.json'), 'utf8'));
const highest = JSON.parse(fs.readFileSync(path.join(repoRoot, 'Mission', 'pd2-affixes-highest-level.json'), 'utf8'));
const html = fs.readFileSync(path.join(repoRoot, 'Mission', 'pd2-affixes-review.html'), 'utf8');

function exportFor(entries = {}) {
  return {
    schemaVersion: 2,
    reviewId: report.reviewId,
    comparisonHash: report.comparisonHash,
    sourceHashes: report.sourceHashes,
    targetBaselineCommit: report.targetBaselineCommit,
    exportedAt: '2026-08-10T00:00:00.000Z',
    entries,
  };
}

test('every mapped occurrence exposes Vanilla, BKVince, PD2 and all pairwise comparisons', () => {
  const mapped = report.entries.filter((entry) => entry.sourceRow !== null && entry.targetRow !== null);
  assert.ok(mapped.length > 1000);
  assert.ok(mapped.every((entry) => entry.rows.vanilla && entry.rows.bkvince && entry.rows.pd2));
  assert.ok(mapped.every((entry) => entry.effects.bkvince && entry.effects.pd2));
  assert.ok(mapped.every((entry) => ['pd2VsVanilla', 'bkvVsVanilla', 'bkvVsPd2'].every((key) => Array.isArray(entry.comparisons[key]))));
  assert.match(html, /Vanilla D2R 3\.2, BKVince restauré et PD2 S13/);
  assert.match(html, /fieldDifferences\.map/);
  assert.doesNotMatch(html, /slice\(0,\s*6\)/);
});

test('statuses are explicit and never use the ambiguous shared status', () => {
  const allowed = new Set(['ALL_THREE_IDENTICAL', 'PD2_EQUALS_VANILLA_BKV_DIFFERS', 'BKV_EQUALS_VANILLA_PD2_DIFFERS', 'BKV_EQUALS_PD2', 'ALL_THREE_DIFFER', 'PD2_DELETED', 'PD2_NEW', 'BKV_ONLY']);
  assert.ok(report.entries.every((entry) => allowed.has(entry.status)));
  assert.ok(!report.entries.some((entry) => entry.status === 'shared'));
});

test('one occurrence supports independent decisions for multiple fields', () => {
  const entry = report.entries.find((candidate) => candidate.table !== 'automagic.txt' && candidate.rows.bkvince && candidate.fieldDifferences.filter((d) => !d.protected && d.bkvince !== d.pd2).length >= 2);
  const [first, second] = entry.fieldDifferences.filter((d) => !d.protected && d.bkvince !== d.pd2);
  const choices = exportFor({
    [entry.id]: {
      fingerprint: entry.fingerprint,
      notes: 'mixed field decision',
      fields: {
        [first.field]: { decision: 'ADOPT_PD2' },
        [second.field]: { decision: 'CUSTOM', customValue: '42', notes: 'product choice' },
      },
    },
  });
  validateDecisionExport(report, choices);
  const preview = compilePreview(report, choices);
  assert.ok(preview.cells.some((cell) => cell.id === entry.id && cell.field === first.field));
  assert.ok(preview.cells.some((cell) => cell.id === entry.id && cell.field === second.field && cell.after === '42'));
});

test('BKVince MaxLevel is protected by default and bulk adoption skips it', () => {
  const entry = report.entries.find((candidate) => candidate.fieldDifferences.some((diff) => diff.protected));
  assert.ok(entry, 'expected a protected MaxLevel witness');
  const max = entry.fieldDifferences.find((diff) => diff.protected);
  assert.equal(max.field.toLowerCase(), 'maxlevel');
  assert.equal(max.defaultDecision, 'KEEP_BKVINCE');
  assert.match(html, /if\(mode==='ADOPT_ALL'&&!d\.protected\)/);
  const unsafe = exportFor({ [entry.id]: { fingerprint: entry.fingerprint, fields: { [max.field]: { decision: 'ADOPT_PD2' } }, notes: '' } });
  assert.ok(compilePreview(report, unsafe).conflicts.some((conflict) => conflict.field === max.field));
});

test('decision exports can be reimported and incompatible hashes or baselines are refused', () => {
  const entry = report.entries.find((candidate) => candidate.fieldDifferences.length);
  const valid = exportFor({ [entry.id]: { fingerprint: entry.fingerprint, fields: {}, notes: 'resume elsewhere' } });
  assert.doesNotThrow(() => validateDecisionExport(report, JSON.parse(JSON.stringify(valid))));
  assert.throws(() => validateDecisionExport(report, { ...valid, comparisonHash: 'BAD' }), /comparisonHash/);
  assert.throws(() => validateDecisionExport(report, { ...valid, targetBaselineCommit: 'BAD' }), /targetBaselineCommit/);
  assert.throws(() => validateDecisionExport(report, { ...valid, sourceHashes: { ...valid.sourceHashes, 'magicprefix.txt': 'BAD' } }), /source hashes/);
});

test('review categories, family grouping, notes, progress and incomplete filters are present', () => {
  for (const category of ['PD2_DELETED', 'PD2_MODIFIED', 'PD2_NEW_PORTABLE', 'PD2_NEW_REVIEW', 'BKV_ONLY', 'AUTOMAGIC_DEFERRED']) assert.ok(report.entries.some((entry) => entry.category === category));
  assert.ok(report.entries.every((entry) => entry.family?.id && entry.family?.label));
  assert.match(html, /Notes de ligne/);
  assert.match(html, /décisions incomplètes seulement/);
  assert.match(html, /occurrences complètes/);
  assert.match(html, /Importer décisions/);
});

test('documentation coverage is explicit and table-only evidence is not presented as documented', () => {
  assert.ok(report.entries.some((entry) => entry.documentation.coverage === 'DOCUMENTED'));
  assert.ok(report.entries.some((entry) => entry.documentation.coverage === 'TABLE_ONLY'));
  assert.ok(report.entries.filter((entry) => entry.documentation.coverage === 'TABLE_ONLY').every((entry) => entry.documentation.url === null));
});

test('AutoMagic remains deferred and excluded from preview compilation', () => {
  const auto = report.entries.filter((entry) => entry.table === 'automagic.txt');
  assert.ok(auto.length > 0 && auto.every((entry) => entry.deferred && entry.category === 'AUTOMAGIC_DEFERRED'));
  const entry = auto.find((candidate) => candidate.fieldDifferences.length);
  const fields = Object.fromEntries(entry.fieldDifferences.map((diff) => [diff.field, { decision: 'ADOPT_PD2', protectedOverride: true }]));
  const preview = compilePreview(report, exportFor({ [entry.id]: { fingerprint: entry.fingerprint, fields, notes: '' } }));
  assert.ok(!preview.cells.some((cell) => cell.id === entry.id));
  assert.ok(!preview.rows.some((row) => row.id === entry.id));
});

test('Highest-Level Affixes is separate, candid about limits, and classifies alvl above 71', () => {
  assert.ok(highest.entries.length > 0);
  assert.ok(highest.entries.every((entry) => Number(entry.alvl) > 71));
  assert.ok(highest.entries.every((entry) => Number.isInteger(entry.accessibility.eligibleBaseCount)));
  assert.ok(highest.entries.some((entry) => entry.accessibility.minimumRequiredIlvl !== null));
  assert.ok(highest.entries.every((entry) => Object.keys(entry.accessibility.paths).length === 4));
  assert.ok(['INFORMATIONAL_ONLY', 'PARTIALLY_RELEVANT', 'ACTION_REQUIRED'].includes(highest.conclusion));
  assert.match(highest.formula.normal, /magic_lvl/);
  assert.ok(highest.limitations.some((line) => line.includes('does not claim complete')));
  assert.match(html, /pd2-affixes-highest-level\.html/);
});

test('both application paths remain fail-closed', () => {
  const merge = spawnSync(process.execPath, [path.join(repoRoot, 'scripts', 'migrate-bkvince', 'pd2-affixes-merge.mjs'), '--apply'], { cwd: repoRoot, encoding: 'utf8' });
  assert.notEqual(merge.status, 0);
  assert.match(merge.stderr, /Affix import is not approved/);
  const preview = spawnSync(process.execPath, [path.join(repoRoot, 'scripts', 'migrate-bkvince', 'pd2-affixes-decisions-preview.mjs'), '--apply'], { cwd: repoRoot, encoding: 'utf8' });
  assert.notEqual(preview.status, 0);
  assert.match(preview.stderr, /application is forbidden/);
});
