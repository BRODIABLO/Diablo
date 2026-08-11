import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';

import {
  auditAffixProjection,
  buildAffixDependencyAuditContext,
  loadTable,
  resolvePd2AffixSourceRoot,
} from './pd2-affixes-merge.mjs';
import { compilePreview } from './pd2-affixes-decisions-preview.mjs';

const repoRoot = path.resolve(import.meta.dirname, '..', '..');
const sourceRoot = resolvePd2AffixSourceRoot();
const targetRoot = path.join(
  repoRoot,
  'data-BKVince',
  'BKVince.mpq',
  'data',
  'global',
  'excel',
);
const report = JSON.parse(fs.readFileSync(
  path.join(repoRoot, 'Mission', 'pd2-affixes-review.json'),
  'utf8',
));
const catalog = JSON.parse(fs.readFileSync(
  path.join(repoRoot, 'Mission', 'pd2-affixes-merge.catalog.json'),
  'utf8',
));
const context = buildAffixDependencyAuditContext(sourceRoot, targetRoot);
const affixTables = ['magicprefix.txt', 'magicsuffix.txt', 'automagic.txt'];
const currentTargetHashes = Object.fromEntries(affixTables.map((table) => [
  table,
  loadTable(targetRoot, table).sha256,
]));

function entry(id) {
  const found = report.entries.find((candidate) => candidate.id === id);
  assert(found, `Missing governed review entry ${id}`);
  return structuredClone(found);
}

function withPd2Field(reviewEntry, field, value) {
  const changed = structuredClone(reviewEntry);
  changed.rows.pd2[field] = value;
  const difference = changed.fieldDifferences.find((candidate) => candidate.field === field);
  if (difference) difference.pd2 = value;
  else {
    changed.fieldDifferences.push({
      field,
      vanilla: changed.rows.vanilla?.[field] ?? null,
      bkvince: changed.rows.bkvince?.[field] ?? null,
      pd2: value,
      protected: false,
      defaultDecision: null,
    });
  }
  return changed;
}

function scopedReport(reviewEntry) {
  return {
    schemaVersion: 3,
    reviewId: 'pd2-affixes-preview-test-v3',
    comparisonHash: 'TEST-COMPARISON-HASH',
    targetBaselineCommit: 'TEST-TARGET-COMMIT',
    sourceHashes: report.sourceHashes,
    dependencyHashes: context.dependencyHashes,
    targetBaselineHashes: currentTargetHashes,
    entries: [reviewEntry],
  };
}

function decisionsFor(scoped, reviewEntry, selection) {
  return {
    schemaVersion: 3,
    reviewId: scoped.reviewId,
    comparisonHash: scoped.comparisonHash,
    targetBaselineCommit: scoped.targetBaselineCommit,
    sourceHashes: scoped.sourceHashes,
    dependencyHashes: scoped.dependencyHashes,
    targetBaselineHashes: scoped.targetBaselineHashes,
    entries: {
      [reviewEntry.id]: {
        fingerprint: reviewEntry.fingerprint,
        notes: '',
        fields: {},
        ...selection,
      },
    },
  };
}

function previewFor(reviewEntry, selection, previewContext = context) {
  const scoped = scopedReport(reviewEntry);
  return compilePreview(scoped, decisionsFor(scoped, reviewEntry, selection), {
    catalog,
    context: previewContext,
  });
}

function withExactTargetRow(reviewEntry) {
  const cloned = structuredClone(context);
  const table = cloned.tables[reviewEntry.table].target.table;
  const normalized = { ...reviewEntry.rows.pd2, multiply: '0' };
  table.rows.push(table.headers.map((header) => normalized[header] ?? ''));
  return cloned;
}

function directAudit(reviewEntry, projected = reviewEntry.rows.pd2) {
  return auditAffixProjection(context, {
    tableName: reviewEntry.table,
    sourceRow: reviewEntry.sourceRow,
    targetRow: reviewEntry.targetRow,
    projected,
    sourceOriginal: reviewEntry.rows.pd2,
    kind: reviewEntry.targetRow === null ? 'append' : 'existing',
    catalog,
  });
}

function setAtPath(value, pathParts, replacement) {
  const cloned = structuredClone(value);
  let cursor = cloned;
  for (const part of pathParts.slice(0, -1)) cursor = cursor[part];
  cursor[pathParts.at(-1)] = replacement;
  return cloned;
}

test('a compatible retune emits only its completed governed cell', () => {
  const reviewEntry = entry('magicprefix.txt:143');
  const preview = previewFor(reviewEntry, {
    fields: {
      frequency: { decision: 'ADOPT_PD2' },
    },
  });
  assert.equal(preview.ready, true);
  assert.equal(preview.rows.length, 0);
  assert.equal(preview.cells.length, 1);
  assert.deepEqual(
    Object.fromEntries(['field', 'before', 'after'].map((key) => [key, preview.cells[0][key]])),
    { field: 'frequency', before: '9', after: '14' },
  );
  assert.deepEqual(preview.conflicts, []);
  assert.deepEqual(preview.incomplete, []);
});

test('a compatible append audits both itype and etype and normalizes multiply', () => {
  const reviewEntry = entry('magicsuffix.txt:865');
  const preview = previewFor(reviewEntry, { lineDecision: 'IMPORT_PD2_AFFIX' });
  assert.equal(preview.ready, true);
  assert.equal(preview.rows.length, 1);
  assert.equal(preview.rows[0].row.multiply, '0');
  const itemTypes = preview.dependencyAudit.occurrences[0].dependencies
    .filter((dependency) => dependency.kind === 'ItemType')
    .map((dependency) => dependency.code)
    .sort();
  assert.deepEqual(itemTypes, ['orb', 'staf', 'wand', 'weap']);
  assert(preview.dependencyAudit.occurrences[0].dependencies.some((dependency) => (
    dependency.kind === 'SkillParameter'
      && dependency.code === '49'
      && dependency.status === 'compatible'
  )));
});

test('new-affix line decisions reject exact-import CUSTOM conflicts and accept a real customization', () => {
  const reviewEntry = entry('magicsuffix.txt:865');
  const realCustom = {
    level: { decision: 'CUSTOM', customValue: '37', notes: 'intentional customized tier' },
  };

  const exactWithCustom = previewFor(reviewEntry, {
    lineDecision: 'IMPORT_PD2_AFFIX',
    fields: realCustom,
  });
  assert.equal(exactWithCustom.ready, false);
  assert.deepEqual(exactWithCustom.cells, []);
  assert.deepEqual(exactWithCustom.rows, []);
  assert.deepEqual(exactWithCustom.dependencyAudit.occurrences, []);
  assert(exactWithCustom.incomplete.some((item) => (
    item.id === reviewEntry.id && item.reasons.some((reason) => /IMPORT_PD2_AFFIX.*CUSTOM/.test(reason))
  )));

  const exactWithUnknownCustom = previewFor(reviewEntry, {
    lineDecision: 'IMPORT_PD2_AFFIX',
    fields: {
      unknownField: { decision: 'CUSTOM', customValue: 'injected', notes: 'must be rejected' },
    },
  });
  assert.equal(exactWithUnknownCustom.ready, false);
  assert.deepEqual(exactWithUnknownCustom.cells, []);
  assert.deepEqual(exactWithUnknownCustom.rows, []);
  assert.deepEqual(exactWithUnknownCustom.dependencyAudit.occurrences, []);
  assert(exactWithUnknownCustom.incomplete.some((item) => (
    item.id === reviewEntry.id && item.reasons.some((reason) => /unknownField|unknown field|IMPORT_PD2_AFFIX.*CUSTOM/i.test(reason))
  )));

  const customizedWithUnknownOnly = previewFor(reviewEntry, {
    lineDecision: 'IMPORT_CUSTOMIZED',
    fields: {
      unknownField: { decision: 'CUSTOM', customValue: 'injected', notes: 'not a governed field' },
    },
  });
  assert.equal(customizedWithUnknownOnly.ready, false);
  assert.deepEqual(customizedWithUnknownOnly.rows, []);
  assert(customizedWithUnknownOnly.incomplete.some((item) => (
    item.id === reviewEntry.id && item.reasons.some((reason) => /real CUSTOM field|unknownField|unknown field/i.test(reason))
  )));

  const customized = previewFor(reviewEntry, {
    lineDecision: 'IMPORT_CUSTOMIZED',
    fields: realCustom,
  });
  assert.equal(customized.ready, true);
  assert.deepEqual(customized.incomplete, []);
  assert.deepEqual(customized.conflicts, []);
  assert.equal(customized.rows.length, 1);
  assert.equal(customized.rows[0].row.level, '37');
  assert.equal(customized.rows[0].provenanceByField.level, 'CUSTOM');
  assert.equal(customized.rows[0].lineDecision, 'IMPORT_CUSTOMIZED');
});

test('read-only UNCHANGED and AutoMagic occurrences ignore every injected decision', () => {
  const witnesses = [
    report.entries.find((candidate) => candidate.category === 'UNCHANGED_BY_PD2'),
    report.entries.find((candidate) => candidate.category === 'AUTOMAGIC_DEFERRED' && candidate.fieldDifferences.length > 0),
  ];
  assert(witnesses.every(Boolean));

  const scoped = {
    ...scopedReport(witnesses[0]),
    entries: witnesses,
  };
  const injectedEntries = Object.fromEntries(witnesses.map((reviewEntry) => [reviewEntry.id, {
    fingerprint: reviewEntry.fingerprint,
    lineDecision: 'IMPORT_CUSTOMIZED',
    fields: Object.fromEntries([
      ...reviewEntry.fieldDifferences.map((difference) => [difference.field, {
        decision: 'CUSTOM',
        customValue: 'injected',
        notes: 'must never be projected',
        protectedOverride: true,
      }]),
      ['unknownField', { decision: 'CUSTOM', customValue: 'injected', notes: 'must never be projected' }],
    ]),
    notes: 'must never be projected',
  }]));
  const decisions = {
    ...decisionsFor(scoped, witnesses[0], {}),
    entries: injectedEntries,
  };
  const preview = compilePreview(scoped, decisions, { catalog, context });
  assert.equal(preview.ready, true);
  assert.equal(preview.autoResolved.length, 2);
  assert.deepEqual(preview.cells, []);
  assert.deepEqual(preview.rows, []);
  assert.deepEqual(preview.rejectedRows, []);
  assert.deepEqual(preview.dependencyAudit.occurrences, []);
  assert.deepEqual(preview.incomplete, []);
  assert.deepEqual(preview.conflicts, []);
});

test('all eight TXT dependencies and both localization namespaces fail before projection', () => {
  const reviewEntry = entry('magicsuffix.txt:865');
  const scoped = scopedReport(reviewEntry);
  const decisions = decisionsFor(scoped, reviewEntry, { lineDecision: 'IMPORT_PD2_AFFIX' });
  const dependencyPaths = [
    ...['properties.txt', 'itemtypes.txt', 'itemstatcost.txt', 'skills.txt'].map((table) => ['source', table]),
    ...['properties.txt', 'itemtypes.txt', 'itemstatcost.txt', 'skills.txt'].map((table) => ['bkvince', table]),
    ['bkvince', 'localization', 'modern', 'baseSha256'],
    ['bkvince', 'localization', 'modern', 'manifestSha256'],
    ['bkvince', 'localization', 'legacy', 'baseSha256'],
    ['bkvince', 'localization', 'legacy', 'manifestSha256'],
  ];
  assert.equal(dependencyPaths.filter((parts) => parts.at(-1).endsWith('.txt')).length, 8);

  for (const dependencyPath of dependencyPaths) {
    const mismatchedHashes = setAtPath(context.dependencyHashes, dependencyPath, 'BAD');
    const failBeforeProjection = new Proxy({ dependencyHashes: mismatchedHashes }, {
      get(target, property) {
        if (property === 'dependencyHashes') return target.dependencyHashes;
        throw new Error(`projection phase reached through ${String(property)}`);
      },
    });
    assert.throws(
      () => compilePreview(scoped, decisions, { catalog, context: failBeforeProjection }),
      /dependencyHashes do not match/,
      dependencyPath.join('.'),
    );
  }
});

test('Property and ItemType failures never leak partial retune or append outputs', () => {
  const propertyEntry = withPd2Field(entry('magicprefix.txt:143'), 'mod1code', 'splash');
  const propertyPreview = previewFor(propertyEntry, {
    fields: {
      frequency: { decision: 'KEEP_BKVINCE' },
      mod1code: { decision: 'ADOPT_PD2' },
    },
  });
  assert.equal(propertyPreview.ready, false);
  assert.equal(propertyPreview.cells.length, 0);
  assert.equal(propertyPreview.rows.length, 0);
  assert(propertyPreview.conflicts.some((conflict) => (
    conflict.kind === 'Property' && conflict.code === 'splash'
  )));

  const typeEntry = withPd2Field(entry('magicsuffix.txt:865'), 'etype1', 't1m');
  const typePreview = previewFor(typeEntry, { lineDecision: 'IMPORT_PD2_AFFIX' });
  assert.equal(typePreview.ready, false);
  assert.equal(typePreview.cells.length, 0);
  assert.equal(typePreview.rows.length, 0);
  assert(typePreview.conflicts.some((conflict) => (
    conflict.kind === 'ItemType' && conflict.code === 't1m'
  )));
});

test('ItemStatCost overflow rejects the complete append without partial output', () => {
  const reviewEntry = entry('magicprefix.txt:814');
  const preview = previewFor(reviewEntry, { lineDecision: 'IMPORT_PD2_AFFIX' });
  assert.equal(preview.ready, false);
  assert.equal(preview.rows.length, 0);
  assert(preview.conflicts.some((conflict) => (
    conflict.kind === 'serialization'
      && /outside 0\.\.1023/.test(conflict.reason)
  )));
});

test('skill parameters accept name-stable IDs and reject absent PD2 skills', () => {
  const stable = directAudit(entry('magicsuffix.txt:859'));
  assert.equal(stable.status, 'compatible');
  assert(stable.dependencies.some((dependency) => (
    dependency.kind === 'SkillParameter'
      && dependency.code === '47'
      && dependency.sourceSkill === 'Fire Ball'
      && dependency.targetSkill === 'Fire Ball'
      && dependency.status === 'compatible'
  )));

  const missingEntry = entry('magicsuffix.txt:895');
  const blocked = previewFor(missingEntry, { lineDecision: 'IMPORT_PD2_AFFIX' });
  assert.equal(blocked.ready, false);
  assert.equal(blocked.rows.length, 0);
  assert(blocked.conflicts.some((conflict) => (
    conflict.kind === 'SkillParameter' && conflict.code === '369'
  )));
});

test('an unanchored group collision rejects the append', () => {
  const reviewEntry = withPd2Field(entry('magicsuffix.txt:865'), 'group', '307');
  const preview = previewFor(reviewEntry, { lineDecision: 'IMPORT_PD2_AFFIX' });
  assert.equal(preview.ready, false);
  assert.equal(preview.rows.length, 0);
  assert(preview.conflicts.some((conflict) => (
    conflict.kind === 'Group' && conflict.code === '307'
  )));
});

test('normalization makes a previously imported append an exact duplicate', () => {
  const reviewEntry = entry('magicprefix.txt:796');
  const preview = previewFor(
    reviewEntry,
    { lineDecision: 'IMPORT_PD2_AFFIX' },
    withExactTargetRow(reviewEntry),
  );
  assert.equal(preview.ready, false);
  assert.equal(preview.rows.length, 0);
  assert(preview.conflicts.some((conflict) => conflict.kind === 'duplicate'));
});

test('a missing localization receives a stable complete preview plan', () => {
  const reviewEntry = entry('magicsuffix.txt:859');
  const preview = previewFor(reviewEntry, { lineDecision: 'IMPORT_PD2_AFFIX' });
  assert.equal(preview.rows.length, 1);
  assert.equal(preview.localizations.status, 'planned');
  assert.deepEqual(preview.localizations.format, {
    encoding: 'UTF-8 BOM',
    lineEndings: 'LF',
    finalEol: false,
  });
  const localization = preview.localizations.entries.find((candidate) => (
    candidate.key === 'of Fireball'
  ));
  assert(Number.isSafeInteger(localization.id));
  assert.equal(localization.modern.status, 'addition-planned');
  assert.equal(localization.legacy.status, 'addition-planned');
  assert.equal(localization.modern.entry.id, localization.id);
  assert.equal(localization.legacy.entry.id, localization.id);
  assert.equal(preview.ready, true);
});

test('projected table counts remain inside the governed serialization fields', () => {
  const preview = previewFor(entry('magicsuffix.txt:865'), {
    lineDecision: 'IMPORT_PD2_AFFIX',
  });
  const suffixes = preview.serializationPlan.tables['magicsuffix.txt'];
  assert.equal(suffixes.appendedRows, 1);
  assert.equal(
    suffixes.projectedCompiledRows,
    suffixes.baselineCompiledRows + suffixes.appendedRows,
  );
  assert.equal(suffixes.within11BitLimit, true);
  assert.equal(preview.serializationPlan.withinUint16Limit, true);
});
