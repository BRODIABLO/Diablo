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
import {
  applyApprovedMapExclusions,
  APPROVED_MAP_EXCLUSION_NOTE,
  closestPoisonTotalEquivalent,
  compilePreview,
  poisonDamageMetrics,
} from './pd2-affixes-decisions-preview.mjs';

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

test('the governed ItemType exceptions accept Cardinal and fail closed outside exact coverage', () => {
  const cardinal = entry('magicprefix.txt:784');
  const accepted = previewFor(cardinal, { lineDecision: 'IMPORT_PD2_AFFIX' });
  assert.equal(accepted.ready, true);
  assert.equal(accepted.rows.length, 1);
  assert(accepted.dependencyAudit.occurrences[0].dependencies.some((dependency) => (
    dependency.kind === 'ItemType'
      && dependency.code === 'jewl'
      && dependency.status === 'compatible-governed-target'
  )));

  const staleCoverage = structuredClone(context);
  staleCoverage.itemTypeTargetSemantics.get('jewl').verified = false;
  const rejectedCardinal = previewFor(cardinal, { lineDecision: 'IMPORT_PD2_AFFIX' }, staleCoverage);
  assert.equal(rejectedCardinal.ready, false);
  assert(rejectedCardinal.conflicts.some((conflict) => conflict.kind === 'ItemType' && conflict.code === 'jewl'));

  const unauthorized = structuredClone(context);
  unauthorized.sourceItemTypes.set('orb', {
    ...unauthorized.sourceItemTypes.get('orb'),
    equiv1: 'unauthorized-parent',
  });
  const rejectedOtherType = previewFor(entry('magicsuffix.txt:865'), { lineDecision: 'IMPORT_PD2_AFFIX' }, unauthorized);
  assert.equal(rejectedOtherType.ready, false);
  assert(rejectedOtherType.conflicts.some((conflict) => conflict.kind === 'ItemType' && conflict.code === 'orb'));
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

test('the approved map rule excludes exactly 248 occurrences without changing MaxLevel decisions', () => {
  const decisions = JSON.parse(fs.readFileSync(
    path.join(repoRoot, 'Mission', 'pd2-affixes-decisions-v3-round1-remediated-proposed.json'),
    'utf8',
  ));
  const matrix = JSON.parse(fs.readFileSync(
    path.join(repoRoot, 'Mission', 'pd2-affixes-round1-conflict-matrix.json'),
    'utf8',
  ));
  const currentReport = {
    ...report,
    dependencyHashes: context.dependencyHashes,
    targetBaselineHashes: currentTargetHashes,
  };
  const currentDecisions = {
    ...decisions,
    comparisonHash: currentReport.comparisonHash,
    dependencyHashes: context.dependencyHashes,
    targetBaselineHashes: currentTargetHashes,
  };
  const maxLevelDecision = (selection) => structuredClone(
    Object.entries(selection?.fields ?? {}).find(([field]) => field.toLowerCase() === 'maxlevel')?.[1] ?? null,
  );
  const maxLevelBefore = Object.fromEntries(Object.entries(decisions.entries).map(([id, selection]) => [
    id,
    maxLevelDecision(selection),
  ]));
  const updated = applyApprovedMapExclusions(currentReport, currentDecisions, matrix.mapAffixes);
  const excluded = Object.entries(updated.entries).filter(([, selection]) => (
    selection.lineDecision === 'EXCLUDE_PD2_AFFIX' && selection.notes === APPROVED_MAP_EXCLUSION_NOTE
  ));
  assert.equal(excluded.length, 248);
  for (const [id, before] of Object.entries(maxLevelBefore)) {
    assert.deepEqual(maxLevelDecision(updated.entries[id]), before, `${id} MaxLevel changed`);
  }
  assert.deepEqual(updated.entries['magicprefix.txt:350'].fields.itype6, {
    decision: 'CUSTOM', customValue: 'weap', notes: 'weap (Weapon)', automatic: false,
  });
  assert.deepEqual(updated.entries['magicprefix.txt:417'].fields.itype6, {
    decision: 'CUSTOM', customValue: 'armo', notes: 'armo (Armor)', automatic: false,
  });
  assert.deepEqual(updated.entries['magicsuffix.txt:175'].fields.itype4, {
    decision: 'CUSTOM', customValue: '', notes: 'Vider itype4', automatic: false,
  });
  const preview = compilePreview(currentReport, updated, { catalog, context });
  assert.deepEqual(preview.proposedManifest, {
    changedCells: 1292,
    appendedRows: 7,
    rejectedRows: 249,
    auditedOccurrences: 621,
    conflicts: 5,
    incomplete: 330,
  });
  assert.deepEqual(preview.rows.map((row) => row.id).sort(), [
    'magicprefix.txt:784',
    'magicprefix.txt:796',
    'magicprefix.txt:812',
    'magicprefix.txt:813',
    'magicsuffix.txt:914',
    'magicsuffix.txt:915',
    'magicsuffix.txt:916',
  ]);
  assert.equal(preview.conflicts.filter((conflict) => conflict.id === 'magicprefix.txt:558').length, 2);
  assert.equal(preview.conflicts.filter((conflict) => conflict.id === 'magicprefix.txt:559').length, 2);
  assert.equal(preview.conflicts.filter((conflict) => conflict.id === 'magicsuffix.txt:569').length, 1);
});

test('the round 2 checkpoint resolves poison exactly and preserves every other product decision', () => {
  const sourceDecisions = JSON.parse(fs.readFileSync(
    path.join(repoRoot, 'Mission', 'pd2-affixes-decisions-v3-round1-remediated-proposed.json'),
    'utf8',
  ));
  const readyDecisions = JSON.parse(fs.readFileSync(
    path.join(repoRoot, 'Mission', 'pd2-affixes-decisions-v3-round1-ready-for-round2.json'),
    'utf8',
  ));
  const previousPreview = JSON.parse(fs.readFileSync(
    path.join(repoRoot, 'Mission', 'pd2-affixes-decisions-v3-round1-remediated-preview.json'),
    'utf8',
  ));
  const readyPreview = JSON.parse(fs.readFileSync(
    path.join(repoRoot, 'Mission', 'pd2-affixes-decisions-v3-round1-ready-for-round2-preview.json'),
    'utf8',
  ));
  const poisonNote = 'Preserve exact PD2 total poison damage within BKVince serialization';
  const custom = (customValue) => ({
    decision: 'CUSTOM', customValue, notes: poisonNote, automatic: false,
  });
  assert.deepEqual(readyDecisions.entries['magicprefix.txt:558'].fields.mod1param, custom('58'));
  assert.deepEqual(readyDecisions.entries['magicprefix.txt:558'].fields.mod1min, custom('1000'));
  assert.deepEqual(readyDecisions.entries['magicprefix.txt:558'].fields.mod1max, custom('1000'));
  assert.deepEqual(readyDecisions.entries['magicprefix.txt:559'].fields.mod1param, custom('100'));
  assert.deepEqual(readyDecisions.entries['magicprefix.txt:559'].fields.mod1min, custom('985'));
  assert.deepEqual(readyDecisions.entries['magicprefix.txt:559'].fields.mod1max, custom('985'));

  const normalized = structuredClone(readyDecisions);
  for (const [id, fields] of Object.entries({
    'magicprefix.txt:558': ['mod1param', 'mod1min', 'mod1max'],
    'magicprefix.txt:559': ['mod1param', 'mod1min', 'mod1max'],
  })) {
    for (const field of fields) normalized.entries[id].fields[field] = structuredClone(sourceDecisions.entries[id].fields[field]);
  }
  normalized.entries['magicsuffix.txt:569'].notes = sourceDecisions.entries['magicsuffix.txt:569'].notes;
  normalized.exportedAt = sourceDecisions.exportedAt;
  assert.deepEqual(normalized, sourceDecisions, 'a product decision outside the approved poison/dependency changes drifted');

  const maxLevelChoice = (selection) => Object.fromEntries(
    Object.entries(selection.fields ?? {}).filter(([field]) => field.toLowerCase() === 'maxlevel'),
  );
  for (const [id, selection] of Object.entries(sourceDecisions.entries)) {
    assert.deepEqual(maxLevelChoice(readyDecisions.entries[id]), maxLevelChoice(selection), `${id}: MaxLevel changed`);
  }
  assert.deepEqual(readyDecisions.entries['magicprefix.txt:350'].fields.itype6, {
    decision: 'CUSTOM', customValue: 'weap', notes: 'weap (Weapon)', automatic: false,
  });
  assert.deepEqual(readyDecisions.entries['magicprefix.txt:417'].fields.itype6, {
    decision: 'CUSTOM', customValue: 'armo', notes: 'armo (Armor)', automatic: false,
  });
  assert.deepEqual(readyDecisions.entries['magicsuffix.txt:175'].fields.itype4, {
    decision: 'CUSTOM', customValue: '', notes: 'Vider itype4', automatic: false,
  });
  assert.equal(readyDecisions.entries['magicsuffix.txt:913'].lineDecision, 'EXCLUDE_PD2_AFFIX');
  assert.equal(Object.values(readyDecisions.entries).filter((selection) => (
    selection.lineDecision === 'EXCLUDE_PD2_AFFIX' && selection.notes === APPROVED_MAP_EXCLUSION_NOTE
  )).length, 248);
  const automaticSelections = Object.values(readyDecisions.entries)
    .map((selection) => Object.values(selection.fields ?? {}).filter((choice) => choice.automatic === true).length)
    .filter(Boolean);
  assert.equal(automaticSelections.length, 83);
  assert.equal(automaticSelections.reduce((total, count) => total + count, 0), 249);

  const ironMaiden = readyDecisions.entries['magicsuffix.txt:569'];
  assert.equal(ironMaiden.notes, 'DEPENDENCY APPROVED — IMPLEMENTATION DEFERRED');
  assert.deepEqual(Object.fromEntries(Object.entries(ironMaiden.fields).map(([field, choice]) => [field, choice.decision])), {
    mod1code: 'ADOPT_PD2', mod1param: 'ADOPT_PD2', mod1min: 'ADOPT_PD2', mod1max: 'ADOPT_PD2',
  });

  assert.deepEqual(readyPreview.proposedManifest, {
    changedCells: 1383,
    appendedRows: 7,
    rejectedRows: 249,
    auditedOccurrences: 655,
    conflicts: 1,
    incomplete: 296,
  });
  assert.equal(readyPreview.proposedManifest.changedCells - previousPreview.proposedManifest.changedCells, 15);
  assert.equal(readyPreview.cells.filter((cell) => cell.id === 'magicprefix.txt:558').length, 7);
  assert.equal(readyPreview.cells.filter((cell) => cell.id === 'magicprefix.txt:559').length, 8);
  assert.deepEqual(readyPreview.conflicts, [{
    id: 'magicsuffix.txt:569',
    kind: 'SkillParameter',
    code: '444',
    reason: 'Numeric skill parameter is not name-stable (Iron Maiden Proc / unused_bkv_merc_skill_444).',
  }]);
  assert.equal(readyPreview.ready, false);
  assert.equal(readyPreview.serializationPlan.withinUint16Limit, true);
  assert(Object.values(readyPreview.serializationPlan.tables).every((table) => table.within11BitLimit));
});

test('poison calculations expose capped and exact-total-compatible alternatives without choosing one', () => {
  assert.deepEqual(poisonDamageMetrics(308, 125), {
    encodedDamage: 308,
    durationFrames: 125,
    durationSeconds: 5,
    totalDamage: 150.390625,
    damagePerSecond: 30.078125,
  });
  assert.deepEqual(poisonDamageMetrics(1160, 50), {
    encodedDamage: 1160,
    durationFrames: 50,
    durationSeconds: 2,
    totalDamage: 226.5625,
    damagePerSecond: 113.28125,
  });
  assert.deepEqual(closestPoisonTotalEquivalent(1160, 50), {
    encodedDamage: 1000,
    durationFrames: 58,
    durationSeconds: 2.32,
    totalDamage: 226.5625,
    damagePerSecond: 97.65625,
    productError: 0,
    durationDistance: 8,
  });
  assert.deepEqual(closestPoisonTotalEquivalent(1970, 50), {
    encodedDamage: 985,
    durationFrames: 100,
    durationSeconds: 4,
    totalDamage: 384.765625,
    damagePerSecond: 96.19140625,
    productError: 0,
    durationDistance: 50,
  });
});

test('the Iron Maiden Proc dependency plan remains separate, exact and blocked on missing localization', () => {
  const matrix = JSON.parse(fs.readFileSync(
    path.join(repoRoot, 'Mission', 'pd2-affixes-round1-conflict-matrix.json'),
    'utf8',
  ));
  const audit = matrix.skillParameter444;
  assert.equal(audit.state, 'DEPENDENCY_PLAN_ONLY_NO_GAMEPLAY_APPLICATION');
  assert.deepEqual(audit.referenceAudit.activeReferencesToSkill444, []);
  assert.equal(audit.completeRows.pd2Skill444.Id, '444');
  assert.equal(audit.completeRows.pd2Skill444.skill, 'Iron Maiden Proc');
  assert.equal(audit.completeRows.bkvinceSkill76['*Id'], '76');
  assert.equal(audit.completeRows.bkvinceSkill76.skill, 'Iron Maiden');
  assert(audit.completeComparisonToBkvince76.some((field) => (
    field.field.toLowerCase() === 'aurarangecalc'
      && field.pd2Skill444.trim() === 'par1'
      && field.bkvinceSkill76 === 'ln12'
  )));
  assert(audit.completeComparisonToBkvince76.some((field) => (
    field.field.toLowerCase() === 'param3'
      && field.pd2Skill444 === '150'
      && field.bkvinceSkill76 === '300'
  )));
  assert.deepEqual(
    audit.proposedDependencyPatch.missingDependencies.map(({ key }) => key),
    ['CurseMastery', 'StrIncDmgRet', 'StrIncRadiusplev'],
  );
  assert(audit.proposedDependencyPatch.operations.some((operation) => (
    operation.table === 'skills.txt'
      && operation.targetRow === 444
      && operation.action === 'REPLACE_RESERVED_ROW_444'
  )));
  assert(!JSON.stringify(audit.proposedDependencyPatch).includes('444→76'));
});
