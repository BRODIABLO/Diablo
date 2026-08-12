import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';

import {
  assertSafeOutputPath,
  compilePreview,
  parseCli,
} from './pd2-skills-decisions-preview.mjs';
import {
  applyBulk,
  createEntry,
  entryState,
  exportEnvelope,
} from './pd2-skills-review-runtime.mjs';
import { FROZEN_CONTRACT_HASH, REVIEW_ID } from './pd2-skills-review-contracts.mjs';
import {
  FROZEN_ORIENTATION_CONTRACT_HASH,
  GLOBAL_SCHEMA_POLICIES,
  ORIENTATION_ID,
} from './pd2-skills-schema-orientation-contracts.mjs';
import { createPolicyEnvelope } from './pd2-skills-schema-policy-runtime.mjs';
import { generateOracleData } from './pd2-skills-review-data.mjs';

const HASH_A = 'A'.repeat(64);
const HASH_B = 'B'.repeat(64);
const HASH_C = 'C'.repeat(64);

function sha256(file) {
  return crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex').toUpperCase();
}

function makeWorkspace(t) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'pd2-skills-preview-'));
  t.after(() => fs.rmSync(root, { recursive: true, force: true }));
  fs.mkdirSync(path.join(root, 'Mission'), { recursive: true });
  fs.mkdirSync(path.join(root, 'analysis-cache'), { recursive: true });
  fs.mkdirSync(path.join(root, 'data-BKVince'), { recursive: true });
  fs.mkdirSync(path.join(root, 'data-vanilla3.2'), { recursive: true });
  const sourceFiles = {
    vanilla32: path.join(root, 'data-vanilla3.2', 'skills.txt'),
    bkvince: path.join(root, 'data-BKVince', 'skills.txt'),
    pd2: path.join(root, 'Mission', 'pd2-skills-source-fixture.txt'),
  };
  fs.writeFileSync(sourceFiles.vanilla32, 'vanilla fixture\n');
  fs.writeFileSync(sourceFiles.bkvince, 'bkvince fixture\r\n');
  fs.writeFileSync(sourceFiles.pd2, 'pd2 fixture\n');
  return { root, sourceFiles };
}

function canonicalField(id, values, extra = {}) {
  const header = extra.header ?? id;
  return {
    id: `skills.txt:${id}`,
    table: 'skills.txt',
    header,
    label: id,
    values: { vanilla32: values[0], bkvince: values[1], pd2: values[2] },
    displayValues: { vanilla32: values[0], bkvince: values[1], pd2: values[2] },
    changed: values[1] !== values[2],
    protected: false,
    protectionReasons: [],
    proofStatus: 'EXACT_TABLE',
    dependencyIds: [],
    sourceLocators: {
      vanilla32: { source: 'vanilla32', table: 'skills.txt', row: 0, key: 'fixture', header },
      bkvince: { source: 'bkvince', table: 'skills.txt', row: extra.bkvinceRow ?? 0, key: 'fixture', header },
      pd2: { source: 'pd2', table: 'skills.txt', row: extra.pd2Row ?? 0, key: 'fixture', header },
    },
    ...extra,
  };
}

function group(id, fields, extra = {}) {
  return {
    id,
    label: id,
    fingerprint: HASH_C,
    proofStatus: extra.proofStatus ?? 'EXACT_TABLE',
    portability: extra.portability ?? ['DATA_ONLY_PROVEN'],
    changed: fields.some((candidate) => candidate.changed),
    fields,
  };
}

function canonicalSkill(stableId, name, ordinal, components, extra = {}) {
  return {
    stableId,
    fingerprint: HASH_B,
    canonicalName: name,
    names: { vanilla32: name, bkvince: name, pd2: name },
    classCode: extra.classCode ?? 'sor',
    scope: extra.scope ?? 'sor',
    playerSkill: true,
    newPd2PlayerSkill: false,
    bkvinceOnlyPlayerSkill: false,
    nodeIds: { vanilla32: `vanilla32:skills.txt:${ordinal}`, bkvince: `bkvince:skills.txt:${ordinal}`, pd2: `pd2:skills.txt:${ordinal}` },
    ordinals: { vanilla32: ordinal, bkvince: ordinal, pd2: ordinal },
    mappingTypes: ['SAME_SKILL_SAME_ORDINAL'],
    identical: false,
    readOnly: false,
    collisionIds: [],
    components,
    dependencies: [],
    consumers: [],
    ...extra,
  };
}

function fixture(t, selectedSkills = []) {
  const workspace = makeWorkspace(t);
  const nodes = [];
  for (let ordinal = 0; ordinal < 3; ordinal += 1) {
    for (const source of ['vanilla32', 'bkvince', 'pd2']) nodes.push({
      id: `${source}:skills.txt:${ordinal}`,
      source,
      ordinal,
      name: `Base ${ordinal}`,
      raw: {},
    });
  }
  for (const skill of selectedSkills) {
    for (const source of ['vanilla32', 'bkvince', 'pd2']) {
      const ordinal = skill.ordinals?.[source];
      if (!Number.isInteger(ordinal) || nodes.some((node) => node.source === source && node.ordinal === ordinal)) continue;
      nodes.push({ id: `${source}:skills.txt:${ordinal}`, source, ordinal, name: skill.names?.[source] ?? skill.canonicalName, raw: {} });
    }
  }
  const sourceManifest = Object.fromEntries(Object.entries(workspace.sourceFiles).map(([source, file]) => [source, {
    tables: {
      'skills.txt': { path: path.relative(workspace.root, file).replaceAll('\\', '/'), sha256: sha256(file) },
    },
  }]));
  const sourceHashes = Object.fromEntries(Object.entries(workspace.sourceFiles).map(([source, file]) => [source, { 'skills.txt': sha256(file) }]));
  const report = {
    schemaVersion: 1,
    reviewId: REVIEW_ID,
    comparisonHash: HASH_A,
    frozenContractHash: FROZEN_CONTRACT_HASH,
    sourceManifest,
    sourceHashes,
    coverage: { nextAppendOrdinal: 3 },
    nodes,
    skills: selectedSkills,
    collisions: [
      { id: 'collision:40', ordinal: 40, pd2StableId: 'skill:sor:cold-enchant', bkvinceStableId: 'skill:sor:frozen-armor' },
      { id: 'collision:376', ordinal: 376, pd2StableId: 'skill:sor:combustion', bkvinceStableId: 'skill:war:summon-tainted' },
      { id: 'collision:369', ordinal: 369, pd2StableId: 'skill:sor:ice-barrage', bkvinceStableId: 'skill:technical:monholyshock' },
    ],
  };
  return { ...workspace, report };
}

function decisionsFor(report, entries, scope = 'COMPLETE_ONLY') {
  return exportEnvelope(report, entries, { scope });
}

function attachOrientation(report) {
  report.schemaOrientation = {
    schemaVersion: 1,
    orientationId: ORIENTATION_ID,
    orientationHash: HASH_C,
    frozenContractHash: FROZEN_ORIENTATION_CONTRACT_HASH,
    sourceHashes: structuredClone(report.sourceHashes),
    policies: GLOBAL_SCHEMA_POLICIES.map((policy, index) => ({
      ...policy,
      fingerprint: String(index + 1).repeat(64).slice(0, 64),
    })),
  };
  return report.schemaOrientation;
}

function approveSchemaPolicies(orientation) {
  const envelope = createPolicyEnvelope(orientation);
  for (const policy of orientation.policies) {
    envelope.decisions[policy.id] = {
      fingerprint: policy.fingerprint,
      decision: 'APPROVE',
      justification: `Approve ${policy.id} before skill preview.`,
    };
  }
  return envelope;
}

function keepEntry(skill, justification = 'Keep the governed BKVince behavior.') {
  const entry = createEntry(skill);
  entry.globalDecision = 'KEEP_BKVINCE';
  entry.notes.finalJustification = justification;
  for (const component of skill.components) entry.componentDecisions[component.id] = { decision: 'KEEP_BKVINCE' };
  return entry;
}

function adoptEntry(skill, testPlan = 'Run focused technical and gameplay validation.') {
  const entry = createEntry(skill);
  entry.globalDecision = 'ADAPT_PD2_SELECTIVELY';
  entry.notes.finalJustification = 'Adopt the governed selected PD2 component.';
  entry.notes.testPlan = testPlan;
  for (const component of skill.components) entry.componentDecisions[component.id] = { decision: 'KEEP_BKVINCE' };
  return entry;
}

test('Amplify Damage hybrid emits exact kept, adopted and CUSTOM cells with tests', (t) => {
  const skill = canonicalSkill('skill:nec:amplify-damage', 'Amplify Damage', 66, [
    group('damage_model', [canonicalField('power', ['-100', '-100', '-50'], { bkvinceRow: 66, pd2Row: 66 })]),
    group('area_targeting', [canonicalField('radius', ['4', '5', '7'], { bkvinceRow: 66, pd2Row: 66 })]),
    group('cost_timing', [canonicalField('mana', ['4', '4', '8'], { bkvinceRow: 66, pd2Row: 66 })]),
    group('buffs_debuffs_auras_passives', [canonicalField('duration', ['200', '200', '300'], { bkvinceRow: 66, pd2Row: 66 })]),
    group('synergies', [canonicalField('curse_mastery', ['', '', "skill('Curse Mastery'.blvl)"], { bkvinceRow: 66, pd2Row: 66 })]),
  ], { classCode: 'nec', scope: 'nec' });
  const { root, report } = fixture(t, [skill]);
  const entry = adoptEntry(skill, 'Compare curse power, radius, duration, mana and Curse Mastery at L1/L20/L40.');
  entry.fieldDecisions['skills.txt:radius'] = { decision: 'ADOPT_PD2' };
  entry.fieldDecisions['skills.txt:mana'] = {
    decision: 'CUSTOM',
    customValue: '6',
    justification: 'Use a midpoint cost for the hybrid.',
    gameplayObjective: 'Balance the larger radius.',
    testPlan: 'Count casts-to-empty at representative levels.',
  };
  const preview = compilePreview(report, decisionsFor(report, { [skill.stableId]: entry }), { repoRoot: root });
  assert.equal(preview.ready, true, JSON.stringify({ conflicts: preview.conflicts, incomplete: preview.incomplete }));
  assert.equal(preview.proposedManifest.changedCells, 2);
  assert.equal(preview.adoptedCells[0].header, 'radius');
  assert.equal(preview.customCells[0].after, '6');
  assert(preview.keptCells.some((cell) => cell.header === 'power'));
  assert(preview.textualDiff.includes('[PD2]'));
  assert(preview.textualDiff.includes('[CUSTOM]'));
});

test('Fire Ball and Fire Wall preserve MALFORMED_SOURCE and never auto-translate delay', (t) => {
  const fireBall = canonicalSkill('skill:sor:fire-ball', 'Fire Ball', 47, [
    group('projectiles_collisions', [canonicalField('multishot', ['1', '1', '(lvl>10?2:1'], {
      bkvinceRow: 47,
      pd2Row: 47,
      protected: true,
      proofStatus: 'MALFORMED_SOURCE',
      protectionReasons: ['malformedFormula'],
    })], { proofStatus: 'MALFORMED_SOURCE' }),
  ]);
  const fireWall = canonicalSkill('skill:sor:fire-wall', 'Fire Wall', 51, [
    group('cost_timing', [canonicalField('delay', ['25', '20', '12'], {
      header: 'localdelay',
      pd2Header: 'delay',
      bkvinceRow: 51,
      pd2Row: 51,
      protected: true,
      protectionReasons: ['delay'],
    })]),
  ]);
  const { root, report } = fixture(t, [fireBall, fireWall]);
  const ballEntry = adoptEntry(fireBall);
  ballEntry.fieldDecisions['skills.txt:multishot'] = {
    decision: 'ADOPT_PD2',
    protectedOverride: {
      approved: true,
      justification: 'Acknowledge the malformed source without supplying the required governed resolution.',
      acknowledgedProofStatus: 'MALFORMED_SOURCE',
      malformedResolution: 'Fixture placeholder removed after valid envelope construction.',
    },
  };
  const wallEntry = adoptEntry(fireWall);
  wallEntry.fieldDecisions['skills.txt:delay'] = {
    decision: 'ADOPT_PD2',
    protectedOverride: { approved: true, justification: 'Must still be rejected as automatic translation.', acknowledgedProofStatus: 'EXACT_TABLE' },
  };
  const malformedDecisions = decisionsFor(report, {
    [fireBall.stableId]: ballEntry,
    [fireWall.stableId]: wallEntry,
  }, 'ALL');
  delete malformedDecisions.entries[fireBall.stableId].fieldDecisions['skills.txt:multishot'].protectedOverride.malformedResolution;
  const preview = compilePreview(report, malformedDecisions, { repoRoot: root });
  assert.equal(preview.ready, false);
  assert.equal(preview.proposedManifest, null);
  assert.deepEqual(preview.adoptedCells, []);
  assert(preview.incomplete.some((item) => JSON.stringify(item).includes('malformedResolution')));
  assert(preview.conflicts.some((item) => item.code === 'AUTOMATIC_DELAY_TRANSLATION'));
});

test('Cold Enchant moved ordinal and collision never produce an automatic merge or row move', (t) => {
  const coldEnchant = canonicalSkill('skill:sor:cold-enchant', 'Cold Enchant', 408, [
    group('identity_availability', [canonicalField('runtimeOrdinal', [null, '408', '40'], {
      bkvinceRow: 408,
      pd2Row: 40,
      protected: true,
      protectionReasons: ['runtimeOrdinal', 'ordinalCollision'],
    })]),
  ], {
    nodeIds: { vanilla32: null, bkvince: 'bkvince:skills.txt:408', pd2: 'pd2:skills.txt:40' },
    ordinals: { vanilla32: null, bkvince: 408, pd2: 40 },
    mappingTypes: ['SAME_SKILL_MOVED_ORDINAL', 'SAME_ORDINAL_DIFFERENT_SKILL'],
    collisionIds: ['collision:40'],
  });
  const { root, report } = fixture(t, [coldEnchant]);
  const entry = adoptEntry(coldEnchant);
  entry.fieldDecisions['skills.txt:runtimeOrdinal'] = {
    decision: 'ADOPT_PD2',
    protectedOverride: { approved: true, justification: 'Witness only; row moves remain forbidden.', acknowledgedProofStatus: 'EXACT_TABLE' },
  };
  const preview = compilePreview(report, decisionsFor(report, { [coldEnchant.stableId]: entry }), { repoRoot: root });
  assert.equal(preview.ready, false);
  assert(preview.conflicts.some((item) => item.code === 'BKVINCE_ROW_MOVE_FORBIDDEN'));
  assert.deepEqual(preview.exactChangesByTable, {});
  assert.equal(preview.proposedManifest, null);
});

function newSkillWitness(name, sourceOrdinal, proposedOrdinal, extra = {}) {
  const slug = name.toLowerCase().replace(/[^a-z0-9]+/g, '-');
  return canonicalSkill(`skill:sor:${slug}`, name, sourceOrdinal, [
    group('identity_availability', [
      canonicalField('skill', [null, null, name], { bkvinceRow: null, pd2Row: sourceOrdinal }),
      canonicalField('Id', [null, null, String(sourceOrdinal)], {
        bkvinceRow: null,
        pd2Row: sourceOrdinal,
        protected: true,
        protectionReasons: ['runtimeOrdinal'],
      }),
    ]),
    group('engine_functions', [canonicalField('srvdofunc', [null, null, extra.callback ?? '75'], {
      bkvinceRow: null,
      pd2Row: sourceOrdinal,
      protected: true,
      proofStatus: extra.proofStatus ?? 'EXACT_TABLE',
      protectionReasons: extra.proofStatus === 'NATIVE_UNPROVEN' ? ['native callback'] : [],
    })], { proofStatus: extra.proofStatus ?? 'EXACT_TABLE' }),
  ], {
    nodeIds: { vanilla32: null, bkvince: null, pd2: `pd2:skills.txt:${sourceOrdinal}` },
    ordinals: { vanilla32: null, bkvince: null, pd2: sourceOrdinal },
    mappingTypes: ['PD2_ONLY_PLAYER_SKILL', 'SAME_ORDINAL_DIFFERENT_SKILL'],
    newPd2PlayerSkill: true,
    collisionIds: extra.collisionIds ?? [],
    dependencies: extra.dependencies ?? [],
    consumers: extra.consumers ?? [],
    newSkillPlan: {
      state: 'PREVIEW_ONLY_NOT_APPROVED',
      sourceOrdinal,
      proposedTargetOrdinal: proposedOrdinal,
      appendOnly: true,
      insertionBeforeExistingRowsForbidden: true,
      dependencyClosure: extra.dependencyClosure ?? { completeForBkvince: true },
      consumerClosure: extra.consumerClosure ?? { status: 'VERIFIED', complete: true, reasons: [] },
      localizations: extra.localizations ?? [{ key: `${name}Name`, value: name, status: 'MISSING' }],
      proposedRow: {
        sourceNodeId: `pd2:skills.txt:${sourceOrdinal}`,
        targetTable: 'skills.txt',
        targetOrdinal: proposedOrdinal,
        targetHeaders: ['*Id', 'skill', 'SrvDoFunc'],
        values: { '*Id': String(proposedOrdinal), skill: name, SrvDoFunc: extra.callback ?? '75' },
        mappingProvenance: {
          '*Id': { mode: 'APPEND_PREVIEW_DOCUMENTARY_VALUE' },
          skill: { mode: 'EXACT_CANONICAL_HEADER', sourceHeader: 'skill' },
          SrvDoFunc: { mode: 'EXACT_CANONICAL_HEADER', sourceHeader: 'SrvDoFunc' },
        },
      },
      testsRequired: ['dependency-closure', 'ordinal-collision', 'native-functions', 'client-server', 'localization', 'consumer-remap'],
    },
  });
}

function newImportEntry(skill, options = {}) {
  const entry = createEntry(skill);
  entry.globalDecision = 'IMPORT_NEW_PD2_SKILL';
  entry.newSkillLineDecision = options.lineDecision ?? 'IMPORT_APPEND_ONLY';
  entry.notes.finalJustification = `Import ${skill.canonicalName} only as a governed append-only candidate.`;
  entry.notes.testPlan = 'Close dependencies, remap ordinals, validate localization and client/server behavior.';
  for (const component of skill.components) entry.componentDecisions[component.id] = { decision: 'ADOPT_PD2' };
  const documentary = skill.components.flatMap((component) => component.fields).find((field) => field.id === 'skills.txt:Id');
  if (documentary) entry.fieldDecisions[documentary.id] = { decision: 'NOT_APPLICABLE' };
  const callback = skill.components.flatMap((component) => component.fields).find((field) => field.id === 'skills.txt:srvdofunc');
  if (callback) {
    entry.fieldDecisions['skills.txt:srvdofunc'] = {
      decision: 'ADOPT_PD2',
      protectedOverride: {
        approved: true,
        justification: 'Explicitly govern the required new-skill callback.',
        acknowledgedProofStatus: callback.proofStatus,
        ...(callback.proofStatus === 'NATIVE_UNPROVEN' ? { nativeRiskAccepted: true } : {}),
      },
    };
  }
  return entry;
}

test('Combustion append-only plan succeeds only with complete dependencies/localization/remaps and contiguous ordinal', (t) => {
  const combustion = newSkillWitness('Combustion', 376, 3, {
    collisionIds: ['collision:376'],
    dependencies: [{ id: 'dependency:missiles:combustion', source: 'pd2', table: 'missiles.txt', key: 'combustion', required: true, closed: true, targetAvailability: { bkvince: true } }],
    consumers: [{ id: 'consumer:ordinal', source: 'pd2', table: 'skills.txt', row: 47, header: 'calc1', encoding: 'ORDINAL', remapPlan: { complete: true, targetOrdinal: 3 } }],
  });
  const { root, report } = fixture(t, [combustion]);
  const entry = newImportEntry(combustion);
  const preview = compilePreview(report, decisionsFor(report, { [combustion.stableId]: entry }), { repoRoot: root });
  assert.equal(preview.ready, true, JSON.stringify({ conflicts: preview.conflicts, incomplete: preview.incomplete }));
  assert.equal(preview.appendOnlyRows[0].proposedOrdinal, 3);
  assert.equal(preview.appendOnlyRows[0].row['*Id'], '3', 'documentary Id follows the governed target but never allocates it');
  assert.equal(preview.appendOnlyRows[0].row.SrvDoFunc, '75', 'canonical field locators resolve to exact case-preserving target headers');
  assert(!preview.appendOnlyRows[0].fieldChanges.some((change) => change.fieldId === 'skills.txt:Id'));
  assert.equal(preview.proposedManifest.appendedRows, 1);
  assert(preview.collisions.some((collision) => collision.id === 'collision:376'));
  assert.equal(preview.implementationAuthorized, false);
  assert.equal(preview.proposedManifest.implementationAuthorized, false);

  const customized = newImportEntry(combustion, { lineDecision: 'IMPORT_CUSTOMIZED' });
  customized.fieldDecisions['skills.txt:Id'] = {
    decision: 'CUSTOM',
    customValue: '3',
    justification: 'Keep the documentary Id aligned to the calculated append target.',
    testPlan: 'Verify the documentary Id never allocates or moves a runtime ordinal.',
    protectedOverride: {
      approved: true,
      justification: 'Explicitly acknowledge the protected documentary field.',
      acknowledgedProofStatus: 'EXACT_TABLE',
    },
  };
  const customPreview = compilePreview(report, decisionsFor(report, { [combustion.stableId]: customized }), { repoRoot: root });
  assert.equal(customPreview.ready, true, JSON.stringify({ conflicts: customPreview.conflicts, incomplete: customPreview.incomplete }));
  const customId = customPreview.appendOnlyRows[0].fieldChanges.find((change) => change.fieldId === 'skills.txt:Id');
  assert.equal(customId.header, '*Id', 'documentary mode maps only canonical Id/*Id without requiring a PD2 sourceHeader');
  assert.equal(customPreview.appendOnlyRows[0].row['*Id'], '3');
});

test('real oracle Combustion CUSTOM documentary Id maps only to exact governed *Id preview cell', () => {
  const report = generateOracleData();
  const combustion = report.skills.find((skill) => skill.stableId === 'skill:sor:combustion');
  assert(combustion?.newPd2PlayerSkill);
  const entry = applyBulk(report, [combustion.stableId], {}, 'ADOPT_PD2')[combustion.stableId];
  entry.globalDecision = 'IMPORT_NEW_PD2_SKILL';
  entry.newSkillLineDecision = 'IMPORT_CUSTOMIZED';
  entry.implementationStatus = 'DECISION_COMPLETE';
  entry.notes.finalJustification = 'Exercise the exact real-oracle append projection without authorizing implementation.';
  entry.notes.testPlan = 'Verify every dependency and protected field in a separately governed prototype.';
  for (const component of combustion.components ?? []) {
    for (const field of component.fields ?? []) {
      if (field.changed === false || field.protected !== true) continue;
      entry.fieldDecisions[field.id] = {
        decision: 'ADOPT_PD2',
        protectedOverride: {
          approved: true,
          justification: 'Explicit preview-only acknowledgement of this protected source cell.',
          acknowledgedProofStatus: field.proofStatus ?? component.proofStatus,
          nativeRiskAccepted: true,
          malformedResolution: 'No silent repair; retain evidence pending a separate governed resolution.',
        },
      };
    }
  }
  const documentaryId = combustion.components.flatMap((component) => component.fields)
    .find((field) => field.id === 'skills.txt:id');
  assert(documentaryId);
  entry.fieldDecisions[documentaryId.id] = {
    decision: 'CUSTOM',
    customValue: String(combustion.newSkillPlan.proposedTargetOrdinal),
    justification: 'Mirror the calculated append target as a documentary value only.',
    testPlan: 'Assert the PD2 Id was never used to allocate the runtime ordinal.',
    protectedOverride: {
      approved: true,
      justification: 'Explicitly govern the documentary Id projection.',
      acknowledgedProofStatus: documentaryId.proofStatus,
    },
  };
  const state = entryState(report, combustion, entry);
  assert.equal(state.complete, true, state.reasons.join('\n'));
  const preview = compilePreview(report, exportEnvelope(report, {
    [combustion.stableId]: entry,
  }, { scope: 'COMPLETE_ONLY' }));
  assert.equal(preview.ready, false, 'the real candidate remains blocked by its intentionally open dependency/consumer proof');
  assert(!preview.conflicts.some((item) => (
    item.code === 'APPEND_FIELD_LOCATOR_MISSING' && item.fieldId === documentaryId.id
  )), 'the canonical documentary Id must resolve exactly to the governed *Id target cell');
  assert(preview.conflicts.some((item) => item.code === 'CONSUMER_CLOSURE_INCOMPLETE'));
  assert.equal(preview.proposedManifest, null);
  assert.deepEqual(preview.proposedRows, []);
});

test('Ice Barrage blocks on missing missiles/skilldesc and missing ordinal consumer remap atomically', (t) => {
  const barrage = newSkillWitness('Ice Barrage', 369, 3, {
    collisionIds: ['collision:369'],
    dependencies: [
      { id: 'dependency:missiles:icebarrage', source: 'pd2', table: 'missiles.txt', key: 'icebarrage', required: true, closed: true, targetAvailability: { bkvince: false }, status: 'RESOLVED_IN_SOURCE' },
      { id: 'dependency:skilldesc:icebarrage', source: 'pd2', table: 'skilldesc.txt', key: 'icebarrage', required: true, closed: true, targetAvailability: { bkvince: false }, status: 'RESOLVED_IN_SOURCE' },
    ],
    consumers: [{ id: 'consumer:ordinal', source: 'pd2', table: 'monstats.txt', row: 1, header: 'skill1', encoding: 'ORDINAL' }],
    consumerClosure: { status: 'NATIVE_UNPROVEN', complete: false, reasons: ['Ordinal encoding audit remains open.'] },
    dependencyClosure: { completeForBkvince: false, reason: 'Missile and skilldesc rows are absent from BKVince.' },
  });
  const { root, report } = fixture(t, [barrage]);
  const preview = compilePreview(report, decisionsFor(report, { [barrage.stableId]: newImportEntry(barrage) }), { repoRoot: root });
  assert.equal(preview.ready, false);
  assert(preview.conflicts.some((item) => item.code === 'DEPENDENCY_NOT_CLOSED'));
  assert(preview.conflicts.some((item) => item.code === 'DEPENDENCY_CLOSURE_INCOMPLETE'));
  assert(preview.conflicts.some((item) => item.code === 'ORDINAL_CONSUMER_NOT_REMAPPED'));
  assert(preview.conflicts.some((item) => item.code === 'CONSUMER_CLOSURE_INCOMPLETE'));
  assert.deepEqual(preview.appendOnlyRows, []);
  assert.equal(preview.proposedManifest, null);
});

test('native-unproven adoption requires explicit override; Raven stays symbolic and Hydra closes summon tables', (t) => {
  const native = newSkillWitness('Combustion', 376, 3, { proofStatus: 'NATIVE_UNPROVEN', collisionIds: ['collision:376'] });
  const raven = canonicalSkill('skill:dru:raven', 'Raven', 221, [
    group('summons', [canonicalField('calc2', ['lvl', 'lvl', 'ulvl + par1 + lvl'], { bkvinceRow: 221, pd2Row: 221, proofStatus: 'UNSUPPORTED_IDENTIFIER' })], { proofStatus: 'UNSUPPORTED_IDENTIFIER' }),
  ], { classCode: 'dru', scope: 'dru' });
  const hydra = canonicalSkill('skill:sor:hydra', 'Hydra', 62, [
    group('summons', [canonicalField('sumskill1', ['HydraMissile', 'HydraMissile', 'HydraFireball'], { bkvinceRow: 62, pd2Row: 62, dependencyIds: [] })]),
  ], {
    dependencies: [
      { id: 'dep:pettype', source: 'pd2', table: 'pettype.txt', field: 'sumskill1', key: 'hydra', required: true, closed: true, targetAvailability: { bkvince: true } },
      { id: 'dep:monstats', source: 'pd2', table: 'monstats.txt', field: 'sumskill1', key: 'hydra', required: true, closed: true, targetAvailability: { bkvince: true } },
    ],
  });
  const { root, report } = fixture(t, [native, raven, hydra]);
  const nativeEntry = newImportEntry(native);
  delete nativeEntry.fieldDecisions['skills.txt:srvdofunc'];
  const ravenEntry = keepEntry(raven, 'Keep the symbolic ulvl-dependent Raven formula without inventing a number.');
  const hydraEntry = adoptEntry(hydra, 'Validate pettype, monstats, inherited masteries and AI cadence.');
  hydraEntry.fieldDecisions['skills.txt:sumskill1'] = { decision: 'ADOPT_PD2' };
  let preview = compilePreview(report, decisionsFor(report, {
    [native.stableId]: nativeEntry,
    [raven.stableId]: ravenEntry,
    [hydra.stableId]: hydraEntry,
  }, 'ALL'), { repoRoot: root });
  assert.equal(preview.ready, false);
  assert(preview.incomplete.some((item) => JSON.stringify(item).includes('protected override')));
  assert.deepEqual(preview.adoptedCells, []);

  nativeEntry.fieldDecisions['skills.txt:srvdofunc'] = {
    decision: 'ADOPT_PD2',
    protectedOverride: {
      approved: true,
      justification: 'Explicitly accept the risk for a separately authorized native proof prototype.',
      acknowledgedProofStatus: 'NATIVE_UNPROVEN',
      nativeRiskAccepted: true,
    },
  };
  preview = compilePreview(report, decisionsFor(report, {
    [native.stableId]: nativeEntry,
    [raven.stableId]: ravenEntry,
    [hydra.stableId]: hydraEntry,
  }), { repoRoot: root });
  assert.equal(preview.ready, true, JSON.stringify({ conflicts: preview.conflicts, incomplete: preview.incomplete }));
  assert(preview.nativeProofs.some((item) => item.ready));
  assert(preview.dependencyGraph.nodes.some((item) => item.table === 'pettype.txt'));
  assert(preview.dependencyGraph.nodes.some((item) => item.table === 'monstats.txt'));
  assert(!preview.textualDiff.includes('ulvl + par1 + lvl'), 'kept symbolic Raven formula must not be projected as a numeric result');
});

test('identical skills are auto-resolved read-only and BKVince-only Warlock remains kept by default', (t) => {
  const identical = canonicalSkill('skill:ama:critical-strike', 'Critical Strike', 9, [], { identical: true, readOnly: true });
  const warlock = canonicalSkill('skill:war:bkv-fire-raven', 'BKV Fire Raven', 443, [
    group('identity_availability', [canonicalField('warlockExistingRow', [null, '9999', null], {
      bkvinceRow: 443,
      pd2Row: 443,
      protected: true,
      protectionReasons: ['warlockExistingRow', 'ordinalCollision'],
    })]),
  ], { classCode: 'war', scope: 'war', bkvinceOnlyPlayerSkill: true });
  const { root, report } = fixture(t, [identical, warlock]);
  const preview = compilePreview(report, decisionsFor(report, { [warlock.stableId]: keepEntry(warlock) }), { repoRoot: root });
  assert.equal(preview.ready, true, JSON.stringify({ conflicts: preview.conflicts, incomplete: preview.incomplete }));
  assert.equal(preview.proposedManifest.selectedSkills, 1);
  assert.equal(preview.proposedManifest.changedCells, 0);
  assert.equal(preview.implementationAuthorized, false);
});

test('an empty COMPLETE_ONLY export cannot emit an applicable empty manifest', (t) => {
  const skill = canonicalSkill('skill:nec:amplify-damage', 'Amplify Damage', 1, [group('damage_model', [canonicalField('power', ['1', '1', '2'], { bkvinceRow: 1, pd2Row: 1 })])], { classCode: 'nec', scope: 'nec' });
  const { root, report } = fixture(t, [skill]);
  const empty = decisionsFor(report, {}, 'COMPLETE_ONLY');
  const preview = compilePreview(report, empty, { repoRoot: root });
  assert.equal(preview.ready, false);
  assert.equal(preview.proposedManifest, null);
  assert(preview.incomplete.some((item) => item.code === 'NO_SELECTED_DECISIONS'));
});

test('preview remains atomically read-only while any required Phase 0 schema policy is open', (t) => {
  const skill = canonicalSkill('skill:nec:amplify-damage', 'Amplify Damage', 1, [
    group('damage_model', [canonicalField('power', ['1', '1', '2'], { bkvinceRow: 1, pd2Row: 1 })]),
  ], { classCode: 'nec', scope: 'nec' });
  const { root, report } = fixture(t, [skill]);
  const orientation = attachOrientation(report);
  const entry = keepEntry(skill);
  const pending = createPolicyEnvelope(orientation);
  const openDecisions = exportEnvelope(report, { [skill.stableId]: entry }, {
    scope: 'ALL',
    schemaPolicy: pending,
  });
  let preview = compilePreview(report, openDecisions, { repoRoot: root });
  assert.equal(preview.ready, false);
  assert.equal(preview.proposedManifest, null);
  assert.deepEqual(preview.proposedCells, []);
  assert.deepEqual(preview.proposedRows, []);
  assert(preview.incomplete.some((item) => item.code === 'PHASE_0_POLICY_GATE_OPEN'));

  const approved = approveSchemaPolicies(orientation);
  const closedDecisions = exportEnvelope(report, { [skill.stableId]: entry }, {
    scope: 'COMPLETE_ONLY',
    schemaPolicy: approved,
  });
  preview = compilePreview(report, closedDecisions, { repoRoot: root });
  assert.equal(preview.ready, true, JSON.stringify({ conflicts: preview.conflicts, incomplete: preview.incomplete }));
  assert.equal(preview.implementationAuthorized, false);
});

test('stale comparison, fingerprints and source hashes fail before projection', (t) => {
  const skill = canonicalSkill('skill:nec:amplify-damage', 'Amplify Damage', 1, [group('damage_model', [canonicalField('power', ['1', '1', '2'], { bkvinceRow: 1, pd2Row: 1 })])], { classCode: 'nec', scope: 'nec' });
  const { root, report, sourceFiles } = fixture(t, [skill]);
  const decisions = decisionsFor(report, { [skill.stableId]: keepEntry(skill) });
  const staleComparison = structuredClone(decisions);
  staleComparison.comparisonHash = HASH_B;
  assert.throws(() => compilePreview(report, staleComparison, { repoRoot: root }), /stale comparison hash/);
  const staleFingerprint = structuredClone(decisions);
  staleFingerprint.entries[skill.stableId].fingerprint = HASH_A;
  assert.throws(() => compilePreview(report, staleFingerprint, { repoRoot: root }), /stale fingerprint/);
  fs.appendFileSync(sourceFiles.bkvince, 'changed');
  assert.throws(() => compilePreview(report, decisions, { repoRoot: root }), /does not match governed/);
});

test('append ordinal collisions, non-append insertion and absent row projections are hard atomic gates', (t) => {
  const occupied = newSkillWitness('Occupied', 376, 2);
  const { root, report } = fixture(t, [occupied]);
  let preview = compilePreview(report, decisionsFor(report, { [occupied.stableId]: newImportEntry(occupied) }), { repoRoot: root });
  assert.equal(preview.ready, false);
  assert(preview.conflicts.some((item) => item.code === 'NON_APPEND_INSERTION'));
  assert(preview.conflicts.some((item) => item.code === 'NON_APPEND_INSERTION'));
  assert.equal(preview.proposedManifest, null);

  occupied.newSkillPlan.proposedTargetOrdinal = 3;
  delete occupied.newSkillPlan.proposedRow;
  const withoutProjection = newImportEntry(occupied);
  withoutProjection.fieldDecisions['skills.txt:Id'] = {
    decision: 'ADOPT_PD2',
    protectedOverride: {
      approved: true,
      justification: 'Exercise the missing row-projection gate after a structurally complete protected choice.',
      acknowledgedProofStatus: 'EXACT_TABLE',
    },
  };
  preview = compilePreview(report, decisionsFor(report, { [occupied.stableId]: withoutProjection }), { repoRoot: root });
  assert(preview.conflicts.some((item) => item.code === 'APPEND_ROW_PROJECTION_MISSING'));
  assert.deepEqual(preview.appendOnlyRows, []);
});

test('selected append-only targets must be unique and form a contiguous suffix', (t) => {
  const first = newSkillWitness('First Candidate', 376, 3);
  const second = newSkillWitness('Second Candidate', 369, 3);
  let scoped = fixture(t, [first, second]);
  let preview = compilePreview(scoped.report, decisionsFor(scoped.report, {
    [first.stableId]: newImportEntry(first),
    [second.stableId]: newImportEntry(second),
  }), { repoRoot: scoped.root });
  assert.equal(preview.ready, false);
  assert(preview.conflicts.some((item) => item.code === 'APPEND_ORDINAL_COLLISION'));
  assert.equal(preview.proposedManifest, null);
  assert.deepEqual(preview.appendOnlyRows, []);

  const gap = newSkillWitness('Gap Candidate', 376, 4);
  scoped = fixture(t, [gap]);
  preview = compilePreview(scoped.report, decisionsFor(scoped.report, {
    [gap.stableId]: newImportEntry(gap),
  }), { repoRoot: scoped.root });
  assert.equal(preview.ready, false);
  assert(preview.conflicts.some((item) => item.code === 'APPEND_SEQUENCE_GAP'));
  assert.equal(preview.proposedManifest, null);
});

test('CLI rejects every apply spelling and output writes outside documentary roots', (t) => {
  const { root } = makeWorkspace(t);
  for (const args of [['--apply'], ['decisions.json', '--apply'], ['decisions.json', '--apply=true']]) {
    assert.throws(() => parseCli(args), /application is forbidden/);
  }
  assert.deepEqual(parseCli(['decisions.json', '--report=oracle.json', '--output=Mission/preview.json']).output, 'Mission/preview.json');
  assert.throws(() => assertSafeOutputPath('data-BKVince/preview.json', { repoRoot: root, cwd: root }), /must stay under/);
  assert.throws(() => assertSafeOutputPath('data-vanilla3.2/preview.json', { repoRoot: root, cwd: root }), /must stay under/);
  assert.throws(() => assertSafeOutputPath('../runtime/preview.json', { repoRoot: root, cwd: root }), /must stay under/);
  assert.equal(assertSafeOutputPath('Mission/preview.json', { repoRoot: root, cwd: root }), path.join(root, 'Mission', 'preview.json'));
});

test('compiler source contains no gameplay writer or apply implementation path', () => {
  const sourcePath = path.join(import.meta.dirname, 'pd2-skills-decisions-preview.mjs');
  const source = fs.readFileSync(sourcePath, 'utf8');
  assert(!/function\s+apply\w*\s*\(/i.test(source), 'preview must not implement gameplay application');
  assert(!/appendFile|createWriteStream|renameSync|copyFile|truncate|unlink|rmSync/.test(source), 'preview contains an unexpected filesystem mutation primitive');
  assert.equal((source.match(/writeFileSync/g) ?? []).length, 1, 'the only writer must be the explicit safe preview artifact output');
  assert(!/data-BKVince.*writeFileSync|writeFileSync.*data-BKVince/s.test(source));
  assert(!/D2RLoader|Saved Games|mods[/\\]/i.test(source));
});
