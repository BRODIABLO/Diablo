import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';
import { fileURLToPath } from 'node:url';
import vm from 'node:vm';

import {
  applyLineFieldAction,
  decisionExportPayload,
  effectiveFieldChoice,
  entryReviewState,
  fieldChoiceComplete,
  NEW_AFFIX_LINE_DECISIONS,
} from './pd2-affixes-decision-rules.mjs';
import { compilePreview, validateDecisionExport } from './pd2-affixes-decisions-preview.mjs';
import { buildAffixDependencyAuditContext } from './pd2-affixes-merge.mjs';

const repoRoot = fileURLToPath(new URL('../../', import.meta.url));
const missionRoot = path.join(repoRoot, 'Mission');
const targetRoot = path.join(repoRoot, 'data-BKVince', 'BKVince.mpq', 'data', 'global', 'excel');
const officialSourceRoot = path.join(repoRoot, 'analysis-cache', 'pd2-affixes-merge', 'official-s13');
const mirrorSourceRoot = path.resolve(repoRoot, '..', 'PD2 Single PLayer', 'PD2-Single-Player-Plus-mod-main', 'data', 'global', 'excel');
const sourceRoot = fs.existsSync(officialSourceRoot) ? officialSourceRoot : mirrorSourceRoot;
const report = JSON.parse(fs.readFileSync(path.join(missionRoot, 'pd2-affixes-review.json'), 'utf8'));
const highest = JSON.parse(fs.readFileSync(path.join(missionRoot, 'pd2-affixes-highest-level.json'), 'utf8'));
const catalog = JSON.parse(fs.readFileSync(path.join(missionRoot, 'pd2-affixes-merge.catalog.json'), 'utf8'));
const documentationMap = JSON.parse(fs.readFileSync(path.join(missionRoot, 'pd2-affixes-documentation-map.json'), 'utf8'));
const html = fs.readFileSync(path.join(missionRoot, 'pd2-affixes-review.html'), 'utf8');
let sharedDependencyContext;

function dependencyContext() {
  sharedDependencyContext ??= buildAffixDependencyAuditContext(sourceRoot, targetRoot);
  return sharedDependencyContext;
}

function exportFor(entries = {}, governedReport = report) {
  return {
    schemaVersion: 3,
    reviewId: governedReport.reviewId,
    comparisonHash: governedReport.comparisonHash,
    sourceHashes: governedReport.sourceHashes,
    dependencyHashes: governedReport.dependencyHashes,
    targetBaselineCommit: governedReport.targetBaselineCommit,
    targetBaselineHashes: governedReport.targetBaselineHashes,
    exportedAt: '2026-08-10T00:00:00.000Z',
    entries,
  };
}

function compileEntries(entries, selections, context = dependencyContext()) {
  const governedReport = { ...report, entries };
  return compilePreview(governedReport, exportFor(selections, governedReport), {
    catalog,
    context,
    verifyBaseline: false,
  });
}

function normalizedCell(field, value) {
  return field.toLowerCase() === 'multiply' ? (value || '0') : value;
}

test('V3 governed artifacts expose three versions and all pairwise comparisons', () => {
  assert.equal(report.schemaVersion, 3);
  assert.equal(report.reviewId, 'pd2-affixes-review-v3');
  assert.equal(highest.schemaVersion, 3);
  assert.equal(highest.reportId, 'pd2-affixes-highest-level-v3');
  assert.deepEqual(report.newAffixLineDecisionOptions, [...NEW_AFFIX_LINE_DECISIONS]);
  assert.deepEqual(report.sourceHashes, Object.fromEntries([
    'magicprefix.txt', 'magicsuffix.txt', 'automagic.txt',
  ].map((table) => [table, catalog.source.tables[table].officialSha256])));
  assert.deepEqual(report.dependencyHashes, dependencyContext().dependencyHashes);
  assert.deepEqual(Object.keys(report.dependencyHashes.source).sort(), [
    'itemstatcost.txt', 'itemtypes.txt', 'properties.txt', 'skills.txt',
  ]);
  assert.deepEqual(Object.keys(report.dependencyHashes.bkvince).sort(), [
    'itemstatcost.txt', 'itemtypes.txt', 'localization', 'properties.txt', 'skills.txt',
  ]);
  const dependencyTxtHashes = [
    ...Object.values(report.dependencyHashes.source),
    ...Object.entries(report.dependencyHashes.bkvince)
      .filter(([key]) => key !== 'localization')
      .map(([, value]) => value),
  ];
  assert.equal(dependencyTxtHashes.length, 8);
  assert.ok(dependencyTxtHashes.every((hash) => /^[A-F0-9]{64}$/.test(hash)));
  assert.deepEqual(Object.keys(report.dependencyHashes.bkvince.localization).sort(), ['legacy', 'modern']);
  for (const namespace of ['modern', 'legacy']) {
    const hashes = report.dependencyHashes.bkvince.localization[namespace];
    assert.deepEqual(Object.keys(hashes).sort(), ['baseSha256', 'manifestSha256']);
    assert.ok(Object.values(hashes).every((hash) => /^[A-F0-9]{64}$/.test(hash)));
  }

  const mapped = report.entries.filter((entry) => entry.sourceRow !== null && entry.targetRow !== null);
  assert.ok(mapped.length > 1000);
  assert.ok(mapped.every((entry) => entry.rows.vanilla && entry.rows.bkvince && entry.rows.pd2));
  assert.ok(mapped.every((entry) => entry.effects.vanilla && entry.effects.bkvince && entry.effects.pd2));
  assert.ok(mapped.every((entry) => ['pd2VsVanilla', 'bkvVsVanilla', 'bkvVsPd2']
    .every((key) => Array.isArray(entry.comparisons[key]))));
  assert.match(html, /pd2-affixes-review-decisions-v3/);
  assert.match(html, /pd2-affixes-decisions-v3\.json/);
  assert.match(html, /payload\?\.schemaVersion\s*!==\s*3/);
  assert.match(html, /\['sourceHashes',\s*'dependencyHashes',\s*'targetBaselineHashes'\]/);
  assert.match(html, /READ_ONLY_CATEGORIES=\["UNCHANGED_BY_PD2","AUTOMAGIC_DEFERRED"\]/);
  assert.match(html, /Cat[^<]*gorie en lecture seule/);
  assert.match(html, /if\(reviewState\(entry\)\.readOnly\)return/);
  const inlineScript = html.match(/<script>([\s\S]*)<\/script>/)?.[1];
  assert.ok(inlineScript);
  assert.doesNotThrow(() => new vm.Script(inlineScript));
  const runtimeStart = inlineScript.indexOf('const FIELD_DECISIONS=');
  const runtimeEnd = inlineScript.indexOf("const storageKey=");
  assert.ok(runtimeStart >= 0 && runtimeEnd > runtimeStart);
  const decisionRuntime = inlineScript.slice(runtimeStart, runtimeEnd);
  const unknownFieldHelperIndex = decisionRuntime.indexOf('function selectedUnknownFields');
  const entryReviewStateIndex = decisionRuntime.indexOf('function entryReviewState');
  assert.ok(unknownFieldHelperIndex >= 0);
  assert.ok(unknownFieldHelperIndex < entryReviewStateIndex);
  const browserRuntime = {};
  new vm.Script(`${decisionRuntime}\nglobalThis.entryReviewStateForTest=entryReviewState;`)
    .runInNewContext(browserRuntime);
  const embeddedState = browserRuntime.entryReviewStateForTest({
    category: 'PD2_NEW_PORTABLE',
    deferred: false,
    fieldDifferences: [{ field: 'level', protected: false, defaultDecision: null, pd2: '80' }],
  }, {
    lineDecision: 'IMPORT_PD2_AFFIX',
    fields: { unknownField: { decision: 'CUSTOM', customValue: '82', notes: 'injected' } },
  });
  assert.equal(embeddedState.complete, false);
  assert.match(embeddedState.reasons.join(' '), /unknownField/);
});

test('status, pairwise comparisons and field differences share canonical normalization', () => {
  const allowed = new Set([
    'ALL_THREE_IDENTICAL',
    'PD2_EQUALS_VANILLA_BKV_DIFFERS',
    'BKV_EQUALS_VANILLA_PD2_DIFFERS',
    'BKV_EQUALS_PD2',
    'ALL_THREE_DIFFER',
    'PD2_DELETED',
    'PD2_NEW',
    'BKV_ONLY',
  ]);
  assert.ok(report.entries.every((entry) => allowed.has(entry.status)));

  const identical = report.entries.filter((entry) => entry.status === 'ALL_THREE_IDENTICAL');
  assert.ok(identical.length > 0);
  assert.ok(identical.every((entry) => entry.fieldDifferences.length === 0));
  assert.ok(identical.every((entry) => Object.values(entry.comparisons).every((diffs) => diffs.length === 0)));

  const mappedWithNoPairDiff = report.entries.filter((entry) => entry.rows.vanilla && entry.rows.bkvince && entry.rows.pd2
    && Object.values(entry.comparisons).every((diffs) => diffs.length === 0));
  assert.ok(mappedWithNoPairDiff.every((entry) => entry.fieldDifferences.length === 0));

  const phantomMultiply = report.entries.flatMap((entry) => entry.fieldDifferences.map((diff) => ({ entry, diff })))
    .filter(({ diff }) => diff.field.toLowerCase() === 'multiply')
    .filter(({ diff }) => new Set(['vanilla', 'bkvince', 'pd2'].map((version) => normalizedCell(diff.field, diff[version]))).size === 1);
  assert.deepEqual(phantomMultiply, []);
});

test('UNCHANGED entries are automatically resolved and never enter actionable progress', () => {
  const unchanged = report.entries.filter((entry) => entry.category === 'UNCHANGED_BY_PD2');
  assert.ok(unchanged.length > 500);
  assert.ok(unchanged.every((entry) => entry.fieldDifferences.every((diff) => diff.defaultDecision === 'KEEP_BKVINCE')));
  assert.ok(unchanged.every((entry) => {
    const state = entryReviewState(entry, { fingerprint: entry.fingerprint, fields: {} });
    return !state.required && state.complete && state.readOnly && state.reasons.length === 0;
  }));

  const witnesses = unchanged.slice(0, 3);
  const injected = Object.fromEntries(witnesses.map((entry) => [entry.id, {
    fingerprint: entry.fingerprint,
    lineDecision: 'IMPORT_CUSTOMIZED',
    fields: {
      __injected__: { decision: 'CUSTOM', customValue: 'must-not-apply', notes: 'legacy state injection' },
    },
    notes: 'must not survive export',
  }]));
  for (const entry of witnesses) {
    assert.strictEqual(applyLineFieldAction(entry, injected[entry.id], 'ADOPT_PD2', true), injected[entry.id]);
  }
  const exported = decisionExportPayload(report, injected, '2026-08-10T00:00:00.000Z');
  assert.deepEqual(exported.entries, {});

  const preview = compileEntries(witnesses, injected);
  assert.equal(preview.autoResolved.length, witnesses.length);
  assert.ok(preview.autoResolved.every((entry) => entry.resolution === 'KEEP_BKVINCE'));
  assert.deepEqual(preview.incomplete, []);
  assert.deepEqual(preview.cells, []);
  assert.deepEqual(preview.rows, []);
  assert.deepEqual(preview.dependencyAudit.occurrences, []);
  assert.equal(preview.ready, true);
});

test('CUSTOM requires a typed value, a note and protected-field consent', () => {
  const ordinary = { field: 'level', protected: false, defaultDecision: null };
  assert.equal(fieldChoiceComplete(ordinary, { decision: 'CUSTOM', customValue: '42' }), false);
  assert.equal(fieldChoiceComplete(ordinary, { decision: 'CUSTOM', customValue: null, notes: 'invalid null' }), false);
  assert.equal(fieldChoiceComplete(ordinary, { decision: 'CUSTOM', customValue: '', notes: 'explicitly clear the cell' }), true);
  assert.equal(fieldChoiceComplete(ordinary, { decision: 'CUSTOM', customValue: '42', notes: 'intentional retune' }), true);

  const protectedField = { field: 'MaxLevel', protected: true, defaultDecision: 'KEEP_BKVINCE' };
  assert.equal(fieldChoiceComplete(protectedField, { decision: 'CUSTOM', customValue: '99', notes: 'override' }), false);
  assert.equal(fieldChoiceComplete(protectedField, { decision: 'CUSTOM', customValue: '99', notes: 'override', protectedOverride: true }), true);

  const state = entryReviewState({
    category: 'PD2_MODIFIED',
    deferred: false,
    fieldDifferences: [ordinary],
  }, { fields: { level: { decision: 'CUSTOM', customValue: '42' } } });
  assert.equal(state.complete, false);
  assert.match(state.reasons.join(' '), /level incomplete/);
});

test('bulk actions distinguish fill-unresolved from replace-all', () => {
  const entry = {
    category: 'PD2_MODIFIED',
    deferred: false,
    fieldDifferences: [
      { field: 'level', protected: false, defaultDecision: null },
      { field: 'frequency', protected: false, defaultDecision: null },
      { field: 'MaxLevel', protected: true, defaultDecision: 'KEEP_BKVINCE' },
    ],
  };
  const initial = {
    fields: {
      level: { decision: 'CUSTOM', customValue: '42', notes: 'keep this explicit value' },
      frequency: { decision: 'DISCUSS' },
    },
  };

  const filled = applyLineFieldAction(entry, initial, 'ADOPT_PD2', false);
  assert.deepEqual(filled.fields.level, initial.fields.level);
  assert.deepEqual(filled.fields.frequency, initial.fields.frequency);
  assert.deepEqual(filled.fields.MaxLevel, { decision: 'KEEP_BKVINCE' });
  assert.equal(entryReviewState(entry, filled).complete, false, 'DISCUSS must remain unresolved in fill-only mode');

  const replaced = applyLineFieldAction(entry, initial, 'KEEP_BKVINCE', true);
  assert.ok(Object.values(replaced.fields).every((choice) => choice.decision === 'KEEP_BKVINCE'));
  assert.equal(entryReviewState(entry, replaced).complete, true);

  const safeAdoption = applyLineFieldAction(entry, initial, 'ADOPT_PD2', true);
  assert.equal(safeAdoption.fields.level.decision, 'ADOPT_PD2');
  assert.equal(safeAdoption.fields.frequency.decision, 'ADOPT_PD2');
  assert.equal(safeAdoption.fields.MaxLevel.decision, 'KEEP_BKVINCE');
});

test('new affixes require an explicit line decision', () => {
  const diff = { field: 'level', protected: false, defaultDecision: null, pd2: '80' };
  const entry = {
    category: 'PD2_NEW_PORTABLE',
    deferred: false,
    blockedReason: null,
    fieldDifferences: [diff],
  };
  assert.equal(entryReviewState(entry, {}).complete, false);
  assert.equal(entryReviewState(entry, { lineDecision: 'DISCUSS' }).complete, false);
  assert.equal(entryReviewState(entry, { lineDecision: 'EXCLUDE_PD2_AFFIX' }).complete, true);
  assert.equal(entryReviewState(entry, { lineDecision: 'IMPORT_PD2_AFFIX' }).complete, true);
  assert.equal(effectiveFieldChoice(entry, { lineDecision: 'IMPORT_PD2_AFFIX' }, diff).decision, 'ADOPT_PD2');
  const exactWithCustom = entryReviewState(entry, {
    lineDecision: 'IMPORT_PD2_AFFIX',
    fields: { level: { decision: 'CUSTOM', customValue: '82', notes: 'custom tier' } },
  });
  assert.equal(exactWithCustom.complete, false);
  assert.match(exactWithCustom.reasons.join(' '), /IMPORT_PD2_AFFIX.*CUSTOM/);
  const exactWithUnknownCustom = entryReviewState(entry, {
    lineDecision: 'IMPORT_PD2_AFFIX',
    fields: { unknownField: { decision: 'CUSTOM', customValue: '82', notes: 'injected field' } },
  });
  assert.equal(exactWithUnknownCustom.complete, false);
  assert.match(exactWithUnknownCustom.reasons.join(' '), /unknownField|unknown field|IMPORT_PD2_AFFIX.*CUSTOM/i);
  assert.equal(entryReviewState(entry, { lineDecision: 'IMPORT_CUSTOMIZED' }).complete, false);
  const customizedWithUnknownOnly = entryReviewState(entry, {
    lineDecision: 'IMPORT_CUSTOMIZED',
    fields: { unknownField: { decision: 'CUSTOM', customValue: '82', notes: 'injected field' } },
  });
  assert.equal(customizedWithUnknownOnly.complete, false);
  assert.match(customizedWithUnknownOnly.reasons.join(' '), /real CUSTOM field|unknownField|unknown field/i);
  assert.equal(entryReviewState(entry, {
    lineDecision: 'IMPORT_CUSTOMIZED',
    fields: { level: { decision: 'CUSTOM', customValue: '82', notes: 'custom tier' } },
  }).complete, true);
});

test('portable new-affix import and exclusion produce explicit row outcomes', () => {
  const entry = report.entries.find((candidate) => candidate.category === 'PD2_NEW_PORTABLE'
    && candidate.technicalAudit?.status === 'compatible');
  assert.ok(entry, 'expected a technically compatible portable new affix');

  const imported = compileEntries([entry], {
    [entry.id]: {
      fingerprint: entry.fingerprint,
      lineDecision: 'IMPORT_PD2_AFFIX',
      fields: {},
      notes: 'portable governed import',
    },
  });
  assert.deepEqual(imported.incomplete, []);
  assert.deepEqual(imported.conflicts, []);
  assert.equal(imported.rows.length, 1);
  assert.deepEqual(imported.rows[0].row, { ...entry.rows.pd2, multiply: '0' });
  assert.equal(imported.rows[0].lineDecision, 'IMPORT_PD2_AFFIX');
  assert.equal(imported.rejectedRows.length, 0);
  assert.ok(!imported.cells.some((cell) => cell.field.toLowerCase() === 'multiply'
    && cell.before === '0' && cell.after === ''));

  const excluded = compileEntries([entry], {
    [entry.id]: {
      fingerprint: entry.fingerprint,
      lineDecision: 'EXCLUDE_PD2_AFFIX',
      fields: {},
      notes: 'product rejection remains traceable',
    },
  });
  assert.deepEqual(excluded.rows, []);
  assert.equal(excluded.rejectedRows.length, 1);
  assert.equal(excluded.rejectedRows[0].id, entry.id);
  assert.equal(excluded.rejectedRows[0].decision, 'EXCLUDE_PD2_AFFIX');
});

test('blockedReason prevents a governed new-affix append', () => {
  const entry = report.entries.find((candidate) => candidate.table === 'magicprefix.txt' && candidate.sourceRow === 766);
  assert.ok(entry, 'expected relocated-vanilla witness magicprefix.txt:766');
  assert.equal(entry.category, 'PD2_NEW_REVIEW');
  assert.equal(entry.portable, false);
  assert.ok(entry.blockedReason);
  assert.ok(entry.blockedReasons.some((reason) => reason.category === 'relocatedVanilla'));

  const preview = compileEntries([entry], {
    [entry.id]: {
      fingerprint: entry.fingerprint,
      lineDecision: 'IMPORT_PD2_AFFIX',
      fields: {},
      notes: 'must remain blocked',
    },
  });
  assert.deepEqual(preview.rows, []);
  assert.equal(preview.ready, false);
  assert.ok(preview.conflicts.some((conflict) => conflict.id === entry.id && conflict.kind === 'relocatedVanilla')
    || preview.incomplete.some((item) => item.id === entry.id && item.reasons.some((reason) => /blocked|relocatedVanilla/i.test(reason))));
});

test('a decided retune is dependency-audited before any cell is proposed', () => {
  const entry = report.entries.find((candidate) => candidate.table === 'magicprefix.txt' && candidate.sourceRow === 143);
  assert.ok(entry && entry.category === 'PD2_MODIFIED', 'expected governed retune witness magicprefix.txt:143');
  const selection = applyLineFieldAction(entry, {
    fingerprint: entry.fingerprint,
    fields: {},
    notes: 'dependency-audited retune',
  }, 'ADOPT_PD2', true);
  const preview = compileEntries([entry], { [entry.id]: selection });
  assert.deepEqual(preview.incomplete, []);
  assert.deepEqual(preview.conflicts, []);
  assert.ok(preview.cells.some((cell) => cell.id === entry.id));

  const audit = preview.dependencyAudit.occurrences.find((occurrence) => occurrence.id === entry.id);
  assert.ok(audit, 'retune projection must have a dependency audit occurrence');
  assert.equal(audit.kind, 'existing');
  assert.equal(audit.status, 'compatible');
  assert.equal(audit.serialization.ok, true);
  assert.ok(audit.dependencies.some((dependency) => dependency.kind === 'Property'));
  assert.ok(audit.dependencies.some((dependency) => dependency.kind === 'ItemStatCost'));
  assert.equal(audit.localization.modern, 'existing-compatible');
  assert.equal(audit.localization.legacy, 'existing-compatible');
});

test('an incomplete retune cannot leak partial cells into the preview', () => {
  const entry = report.entries.find((candidate) => candidate.category === 'PD2_MODIFIED'
    && candidate.fieldDifferences.filter((diff) => diff.defaultDecision === null).length >= 2);
  assert.ok(entry, 'expected a retune with at least two unresolved fields');
  const [chosen] = entry.fieldDifferences.filter((diff) => diff.defaultDecision === null);
  const preview = compileEntries([entry], {
    [entry.id]: {
      fingerprint: entry.fingerprint,
      fields: { [chosen.field]: { decision: 'ADOPT_PD2', protectedOverride: true } },
      notes: 'deliberately incomplete witness',
    },
  });
  assert.deepEqual(preview.cells, []);
  assert.deepEqual(preview.rows, []);
  assert.deepEqual(preview.dependencyAudit.occurrences, []);
  assert.ok(preview.incomplete.some((item) => item.id === entry.id));
  assert.equal(preview.ready, false);
});

test('AutoMagic remains deferred in review, preview and highest-level analysis', () => {
  const auto = report.entries.filter((entry) => entry.table === 'automagic.txt');
  assert.ok(auto.length > 0);
  assert.ok(auto.every((entry) => entry.deferred && entry.category === 'AUTOMAGIC_DEFERRED'));
  assert.ok(auto.every((entry) => {
    const state = entryReviewState(entry, { fingerprint: entry.fingerprint, fields: {} });
    return !state.required && state.complete && state.readOnly;
  }));

  const entry = auto.find((candidate) => candidate.fieldDifferences.length);
  assert.ok(entry);
  const fields = Object.fromEntries(entry.fieldDifferences.map((diff) => [diff.field, {
    decision: 'ADOPT_PD2',
    protectedOverride: true,
  }]));
  const preview = compileEntries([entry], {
    [entry.id]: { fingerprint: entry.fingerprint, fields, notes: 'still deferred' },
  });
  assert.strictEqual(applyLineFieldAction(entry, { fingerprint: entry.fingerprint, fields }, 'KEEP_BKVINCE', true).fields, fields);
  assert.deepEqual(decisionExportPayload(report, {
    [entry.id]: { fingerprint: entry.fingerprint, fields, notes: 'still deferred' },
  }, '2026-08-10T00:00:00.000Z').entries, {});
  assert.deepEqual(preview.cells, []);
  assert.deepEqual(preview.rows, []);
  assert.deepEqual(preview.dependencyAudit.occurrences, []);
  assert.ok(preview.autoResolved.some((item) => item.id === entry.id));
  assert.ok(highest.entries.some((candidate) => candidate.table === 'automagic.txt'));
});

test('Highest-Level Affixes classifies each version and excludes nonspawnable rows from accessibility', () => {
  assert.ok(highest.entries.length > 0);
  assert.deepEqual(Object.keys(highest.baseCatalogs).sort(), ['bkvince', 'pd2', 'vanilla']);
  assert.ok(highest.entries.every((entry) => ['vanilla', 'bkvince', 'pd2']
    .every((version) => entry.versions[version]?.state)));

  const analyses = highest.entries.flatMap((entry) => Object.values(entry.versions));
  const nonspawnable = analyses.filter((analysis) => analysis.state === 'NON_SPAWNABLE_INTERNAL');
  assert.ok(nonspawnable.length > 0);
  assert.ok(nonspawnable.every((analysis) => analysis.spawnable === false
    && analysis.relevant === false && analysis.accessibility === null));

  const active = analyses.filter((analysis) => analysis.state === 'ACTIVE_HIGH_LEVEL');
  assert.ok(active.length > 0);
  assert.ok(active.every((analysis) => analysis.spawnable === true
    && analysis.relevant === true
    && analysis.level > 71
    && analysis.level <= 99
    && Number.isInteger(analysis.accessibility.eligibleBaseCount)
    && Object.keys(analysis.accessibility.paths).length === 4));

  const vita110 = highest.specialFindings.find((finding) => finding.table === 'magicsuffix.txt' && finding.sourceRow === 340);
  assert.ok(vita110);
  assert.equal(vita110.conclusion, 'INFORMATIONAL_ONLY_PENDING_HISTORICAL_CONFIRMATION');
  assert.match(vita110.probableReason, /alvl is capped at 99/);
  assert.ok(highest.limitations.some((line) => line.includes('does not claim complete')));
});

test('decision exports reimport only with the complete V3 governed identity', () => {
  const entry = report.entries.find((candidate) => candidate.fieldDifferences.length);
  const valid = exportFor({
    [entry.id]: { fingerprint: entry.fingerprint, fields: {}, notes: 'resume elsewhere' },
  });
  assert.doesNotThrow(() => validateDecisionExport(report, JSON.parse(JSON.stringify(valid))));
  assert.throws(() => validateDecisionExport(report, { ...valid, schemaVersion: 2 }), /schemaVersion/);
  assert.throws(() => validateDecisionExport(report, { ...valid, comparisonHash: 'BAD' }), /comparisonHash/);
  assert.throws(() => validateDecisionExport(report, { ...valid, targetBaselineCommit: 'BAD' }), /targetBaselineCommit/);
  assert.throws(() => validateDecisionExport(report, {
    ...valid,
    sourceHashes: { ...valid.sourceHashes, 'magicprefix.txt': 'BAD' },
  }), /sourceHashes|source hashes/);
  assert.throws(() => validateDecisionExport(report, {
    ...valid,
    dependencyHashes: undefined,
  }), /dependencyHashes|dependency hashes/);
  assert.throws(() => validateDecisionExport(report, {
    ...valid,
    dependencyHashes: {
      ...valid.dependencyHashes,
      source: { ...valid.dependencyHashes.source, 'skills.txt': 'BAD' },
    },
  }), /dependencyHashes|dependency hashes/);
  assert.throws(() => validateDecisionExport(report, {
    ...valid,
    dependencyHashes: {
      ...valid.dependencyHashes,
      bkvince: {
        ...valid.dependencyHashes.bkvince,
        localization: {
          ...valid.dependencyHashes.bkvince.localization,
          modern: {
            ...valid.dependencyHashes.bkvince.localization.modern,
            manifestSha256: 'BAD',
          },
        },
      },
    },
  }), /dependencyHashes|dependency hashes/);
  assert.throws(() => validateDecisionExport(report, {
    ...valid,
    targetBaselineHashes: { ...valid.targetBaselineHashes, 'magicprefix.txt': 'BAD' },
  }), /targetBaselineHashes|target baseline hashes/);
  assert.throws(() => validateDecisionExport(report, {
    ...valid,
    entries: { [entry.id]: { ...valid.entries[entry.id], fingerprint: 'BAD' } },
  }), /fingerprint/);
});

test('review categories and documentation evidence remain explicit', () => {
  for (const category of [
    'PD2_DELETED',
    'PD2_MODIFIED',
    'PD2_NEW_PORTABLE',
    'PD2_NEW_REVIEW',
    'BKV_ONLY',
    'AUTOMAGIC_DEFERRED',
  ]) assert.ok(report.entries.some((entry) => entry.category === category));
  assert.ok(report.entries.every((entry) => entry.family?.id && entry.family?.label));
  const removedMappings = documentationMap.entries.filter((entry) => entry.reference.section === 'Removed Affixes');
  const compilationMappings = documentationMap.entries.filter((entry) => entry.reference.section === 'Affix Changes Compilation');
  assert.equal(removedMappings.length, 125);
  assert.ok(compilationMappings.length > 0);
  assert.ok(Array.isArray(documentationMap.claims) && documentationMap.claims.length > 0);
  assert.ok(Array.isArray(documentationMap.rules) && documentationMap.rules.length > 0);
  assert.deepEqual(report.documentationMap.claims, documentationMap.claims);
  assert.deepEqual(report.documentationMap.rules, documentationMap.rules);
  assert.equal(documentationMap.identityContract.nameOnlyMatchesForbidden, true);
  assert.deepEqual(documentationMap.identityContract.required, [
    'table', 'sourceRow', 'fingerprint', 'name', 'properties', 'itemTypes',
  ]);

  const claimsById = new Map();
  for (const claim of documentationMap.claims) {
    assert.equal(typeof claim.id, 'string');
    assert.ok(claim.id.length > 0);
    assert.ok(!claimsById.has(claim.id), `duplicate documentation claim ${claim.id}`);
    claimsById.set(claim.id, claim);
  }
  const rulesById = new Map();
  for (const rule of documentationMap.rules) {
    assert.equal(typeof rule.id, 'string');
    assert.ok(rule.id.length > 0);
    assert.ok(!rulesById.has(rule.id), `duplicate documentation rule ${rule.id}`);
    rulesById.set(rule.id, rule);
    assert.ok(claimsById.has(rule.claimId), `${rule.id}: unknown claim ${rule.claimId}`);
    assert.equal(rule.predicate?.notNameOnly, true, `${rule.id}: Name-only matching must be forbidden`);
    assert.equal(rule.expectedOccurrenceCount, rule.materializedOccurrenceCount, `${rule.id}: count drift`);
    assert.match(rule.occurrenceSetSha256, /^[a-f0-9]{64}$/i);
  }
  for (const rule of documentationMap.rules) {
    const actualCount = documentationMap.entries.filter((entry) => entry.ruleIds.includes(rule.id)).length;
    assert.equal(actualCount, rule.materializedOccurrenceCount, `${rule.id}: materialized occurrence count`);
  }

  const reportByOccurrence = new Map(report.entries
    .filter((entry) => entry.sourceRow !== null)
    .map((entry) => [`${entry.table}:${entry.sourceRow}`, entry]));
  const mapOccurrenceKeys = new Set();
  const exactMapKeys = new Set();
  const mappedNames = new Set();
  for (const mapped of documentationMap.entries) {
    const occurrenceKey = `${mapped.table}:${mapped.sourceRow}`;
    const exactKey = `${occurrenceKey}:${mapped.fingerprint}`;
    assert.ok(!mapOccurrenceKeys.has(occurrenceKey), `duplicate documentation occurrence ${occurrenceKey}`);
    mapOccurrenceKeys.add(occurrenceKey);
    assert.ok(!exactMapKeys.has(exactKey), `duplicate exact documentation mapping ${exactKey}`);
    exactMapKeys.add(exactKey);
    mappedNames.add(mapped.name);
    assert.ok(Array.isArray(mapped.claimIds) && mapped.claimIds.length > 0, `${exactKey}: claimIds`);
    assert.ok(Array.isArray(mapped.ruleIds) && mapped.ruleIds.length > 0, `${exactKey}: ruleIds`);
    for (const claimId of mapped.claimIds) assert.ok(claimsById.has(claimId), `${exactKey}: unknown claim ${claimId}`);
    for (const ruleId of mapped.ruleIds) {
      const rule = rulesById.get(ruleId);
      assert.ok(rule, `${exactKey}: unknown rule ${ruleId}`);
      assert.ok(mapped.claimIds.includes(rule.claimId), `${exactKey}: rule ${ruleId} claim is not referenced`);
    }
    assert.ok(mapped.claimIds.includes(mapped.reference.claimId), `${exactKey}: primary claim is not referenced`);
    const governedRules = mapped.ruleIds.map((ruleId) => rulesById.get(ruleId));
    const documentedFields = [...new Set(governedRules.flatMap((rule) => rule.documentedFields))];
    const documentedFacets = [...new Set(governedRules.flatMap((rule) => rule.facets))];

    const actual = reportByOccurrence.get(occurrenceKey);
    assert.ok(actual, `${exactKey}: report occurrence missing`);
    assert.equal(actual.fingerprint, mapped.fingerprint);
    assert.equal(actual.name, mapped.name);
    assert.deepEqual([1, 2, 3]
      .map((slot) => actual.rows.pd2?.[`mod${slot}code`])
      .filter(Boolean), mapped.properties);
    assert.deepEqual(actual.itemTypes, mapped.itemTypes);
    assert.equal(actual.documentation.coverage, 'DOCUMENTED');
    assert.deepEqual(actual.documentation.mapping, {
      table: mapped.table,
      sourceRow: mapped.sourceRow,
      fingerprint: mapped.fingerprint,
      name: mapped.name,
      properties: mapped.properties,
      itemTypes: mapped.itemTypes,
    });
    assert.deepEqual(actual.documentation.claimIds, mapped.claimIds);
    assert.deepEqual(actual.documentation.ruleIds, mapped.ruleIds);
    assert.deepEqual(actual.documentation.documentedFields, documentedFields);
    assert.deepEqual(actual.documentation.documentedFacets, documentedFacets);
  }
  assert.ok(compilationMappings.every((mapped) => mapped.ruleIds.every((ruleId) => (
    claimsById.get(rulesById.get(ruleId).claimId).section === 'Affix Changes Compilation'
  ))));

  const documented = report.entries.filter((entry) => entry.documentation.coverage === 'DOCUMENTED');
  assert.equal(documented.length, documentationMap.entries.length);
  for (const entry of report.entries.filter((candidate) => mappedNames.has(candidate.name))) {
    const exactKey = `${entry.table}:${entry.sourceRow}:${entry.fingerprint}`;
    assert.equal(entry.documentation.coverage === 'DOCUMENTED', exactMapKeys.has(exactKey),
      `${exactKey}: documentation must never match by Name only`);
  }
  assert.equal(report.documentationMap.source.revisionId, 23938);
  assert.match(report.documentationMap.source.revisionUrl, /oldid=23938/);
  assert.ok(documented.every((entry) => entry.documentation.revisionId === 23938
    && /oldid=23938/.test(entry.documentation.url)));
  assert.ok(report.entries.some((entry) => entry.documentation.coverage === 'TABLE_ONLY'));
  assert.ok(report.entries.filter((entry) => entry.documentation.coverage === 'TABLE_ONLY')
    .every((entry) => entry.documentation.url === null));
  assert.match(html, /Importer d[^<]*cisions/);
  assert.match(html, /occurrences auto-r[^<]*solues/);
});

test('all --apply paths remain forbidden', () => {
  const merge = spawnSync(process.execPath, [
    path.join(repoRoot, 'scripts', 'migrate-bkvince', 'pd2-affixes-merge.mjs'),
    '--apply',
  ], { cwd: repoRoot, encoding: 'utf8' });
  assert.notEqual(merge.status, 0);
  assert.match(merge.stderr, /Affix import is not approved/);

  const preview = spawnSync(process.execPath, [
    path.join(repoRoot, 'scripts', 'migrate-bkvince', 'pd2-affixes-decisions-preview.mjs'),
    '--apply',
  ], { cwd: repoRoot, encoding: 'utf8' });
  assert.notEqual(preview.status, 0);
  assert.match(preview.stderr, /application is forbidden/);
});
