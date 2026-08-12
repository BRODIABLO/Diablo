import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';

import Ajv2020 from 'ajv/dist/2020.js';
import addFormats from 'ajv-formats';

import {
  DEFAULT_SOURCE_ROOTS,
  repoRoot,
} from './pd2-skills-review-data.mjs';
import {
  FROZEN_ORIENTATION_CONTRACT_HASH,
  GLOBAL_SCHEMA_POLICIES,
  RAW_VALUE_STATES,
  rawValueState,
  semanticBlank,
} from './pd2-skills-schema-orientation-contracts.mjs';
import { sha256Canonical } from './pd2-skills-review-contracts.mjs';
import { validatePolicyEnvelope } from './pd2-skills-schema-policy-runtime.mjs';
import {
  buildCellEvidence,
  buildFieldDictionary,
  buildPolicyEnvelope,
  buildSchemaOrientationData,
} from './pd2-skills-schema-orientation-data.mjs';
import {
  ORIENTATION_ARTIFACT_PATHS,
  buildSchemaOrientationArtifacts,
  checkSchemaOrientationArtifacts,
  loadOrientationReferences,
} from './pd2-skills-schema-orientation.mjs';

function fileHash(filePath) {
  return crypto.createHash('sha256').update(fs.readFileSync(filePath)).digest('hex').toUpperCase();
}

const gameplayPaths = [
  DEFAULT_SOURCE_ROOTS.vanilla32,
  DEFAULT_SOURCE_ROOTS.bkvince,
  DEFAULT_SOURCE_ROOTS.pd2,
].map((root) => {
  const entry = fs.readdirSync(root).find((name) => name.toLowerCase() === 'skills.txt');
  assert.ok(entry, `${root} must contain Skills.txt`);
  return path.join(root, entry);
});
const gameplayBefore = gameplayPaths.map(fileHash);
const generated = buildSchemaOrientationArtifacts(DEFAULT_SOURCE_ROOTS);
const orientation = generated.orientation;
const columns = new Map(orientation.columns.map((column) => [column.canonicalHeader, column]));
const gameplayAfter = gameplayPaths.map(fileHash);

test('semanticBlank and physical raw states distinguish absent, null, empty, zero and value', () => {
  assert.equal(semanticBlank(undefined), true);
  assert.equal(semanticBlank(null), true);
  assert.equal(semanticBlank('  '), true);
  assert.equal(semanticBlank(0), false);
  assert.equal(semanticBlank('0'), false);
  assert.deepEqual([
    rawValueState(false, undefined),
    rawValueState(true, null),
    rawValueState(true, ''),
    rawValueState(true, '0'),
    rawValueState(true, 'value'),
  ], RAW_VALUE_STATES);

  const synthetic = {
    table: { headers: ['skill', 'field'], rows: [['Synthetic', null]] },
  };
  const record = { row: synthetic.table.rows[0] };
  assert.equal(buildCellEvidence('pd2', synthetic, record, 'missing').rawState, 'ABSENT_COLUMN');
  assert.equal(buildCellEvidence('pd2', synthetic, record, 'field').rawState, 'NULL_VALUE');
});

test('matrix exhaustively covers the three governed Skills.txt schemas', () => {
  assert.equal(orientation.coverage.canonicalHeaders, 330);
  assert.deepEqual(orientation.coverage.sourceHeaders, { vanilla32: 322, bkvince: 322, pd2: 256 });
  assert.deepEqual(orientation.coverage.sourceRows, { vanilla32: 429, bkvince: 451, pd2: 603 });
  assert.deepEqual(orientation.coverage.sourceOnlyHeaders.vanilla32, []);
  assert.deepEqual(orientation.coverage.sourceOnlyHeaders.bkvince, []);
  assert.deepEqual(orientation.coverage.sourceOnlyHeaders.pd2, orientation.coverage.pd2OnlyHeaders);
  assert.equal(orientation.coverage.documentedFields, 287);
  assert.equal(orientation.coverage.headerOnlyDocumentaryFields, 35);
  assert.equal(orientation.columns.length, 330);
  assert.equal(new Set(orientation.columns.map((column) => column.canonicalHeader)).size, 330);
  for (const column of orientation.columns) {
    for (const key of ['rawHeaders', 'presence', 'usage', 'examples', 'documentation', 'playerLabelFr', 'shortHelpFr', 'family', 'consumer', 'classifications', 'primaryClassification', 'potentialEquivalent', 'decisionScope', 'defaultPolicy', 'technicalOnly', 'protected', 'proofStatus']) {
      assert.ok(Object.hasOwn(column, key), `${column.canonicalHeader} lacks ${key}`);
    }
    for (const source of ['vanilla32', 'bkvince', 'pd2']) {
      const usage = column.usage.bySource[source];
      assert.equal(usage.nonBlankCells, usage.playerSkills + usage.technicalOrMonsterRows);
    }
  }
});

test('PD2-only used and semantic-blank columns are classified without cell decisions', () => {
  assert.deepEqual(orientation.coverage.pd2OnlyUsed, ['delay', 'checkfunc', 'nocostinstate', 'general']);
  assert.deepEqual(orientation.coverage.pd2OnlySemanticBlank, ['auratgtevent', 'auratgteventfunc', 'passiveevent', 'passiveeventfunc']);
  for (const header of orientation.coverage.pd2OnlySemanticBlank) {
    const column = columns.get(header);
    assert.equal(column.primaryClassification, 'PD2_SCHEMA_UNUSED');
    assert.equal(column.decisionScope, 'NO_SKILL_DECISION');
    assert.equal(column.usage.totalNonBlankCells, 0);
  }
  for (const header of orientation.coverage.pd2OnlyUsed) {
    const column = columns.get(header);
    assert.ok(column.classifications.includes('NATIVE_EXTENSION_REQUIRED'));
    assert.equal(column.decisionScope, 'GLOBAL_POLICY');
    assert.equal(column.proofStatus, 'NATIVE_UNPROVEN');
  }
});

test('field dictionary is exhaustive, concise and governed by the orientation hash', () => {
  const dictionary = buildFieldDictionary(orientation);
  assert.equal(dictionary.orientationHash, orientation.orientationHash);
  assert.equal(dictionary.fields.length, 330);
  assert.equal(dictionary.fields.find((field) => field.rawHeader === 'cost add').playerLabelFr, 'Influence sur la valeur en or');
  assert.equal(dictionary.fields.find((field) => field.rawHeader === 'itemeffect').groupId, 'ITEM_TRIGGER_EXECUTION');
  assert.equal(dictionary.fields.find((field) => field.rawHeader === 'emaxlev5').groupId, 'ELEMENTAL_DAMAGE_CURVE');
});

test('ten mechanical contracts and fifteen atomic bundles match the frozen contract', () => {
  assert.equal(orientation.frozenContractHash, FROZEN_ORIENTATION_CONTRACT_HASH);
  assert.equal(orientation.mechanicalContracts.length, 10);
  assert.equal(orientation.bundles.length, 15);
  assert.equal(new Set(orientation.mechanicalContracts.map((contract) => contract.id)).size, 10);
  assert.equal(new Set(orientation.bundles.map((bundle) => bundle.id)).size, 15);
  assert.equal(orientation.mechanicalContracts.find((contract) => contract.id === 'cooldowns').translationPolicy, 'NO_AUTOMATIC_DELAY_TRANSLATION');
});

test('same-header equivalence proves only schema concepts, never cross-engine callback behavior', () => {
  const srvDoFunc = orientation.equivalences.proven.find((entry) => entry.sourceHeader === 'srvdofunc');
  assert.equal(srvDoFunc.relation, 'SAME_CANONICAL_HEADER');
  assert.equal(srvDoFunc.proofScope, 'SCHEMA_CONCEPT_ONLY');
  assert.equal(srvDoFunc.valueSemanticsProven, false);
  assert.equal(srvDoFunc.notProvenReference, 'CROSS_ENGINE_CALLBACK_NUMBERS');
  assert.ok(orientation.equivalences.notProven.some((entry) => entry.sourceHeader === 'numeric callbacks'));
});

test('required witnesses preserve proof and do not claim unproven mappings', () => {
  const cooldowns = orientation.witnesses.cooldowns;
  assert.equal(cooldowns.automaticMappingAllowed, false);
  assert.equal(cooldowns.policy, 'NO_AUTOMATIC_DELAY_TRANSLATION');
  assert.equal(orientation.witnesses.costAdd.skillBalanceDecision, false);
  assert.equal(orientation.witnesses.itemTriggerExecution.bundleId, 'ITEM_TRIGGER_EXECUTION');
  assert.equal(orientation.witnesses.elementalMaximumCurve.decisions, 1);
  assert.equal(orientation.witnesses.checkfunc.nonBlankPd2Cells, 38);
  assert.equal(orientation.witnesses.checkfunc.d2rSupportClaimed, false);

  const auraevent4 = orientation.witnesses.auraevent4;
  assert.deepEqual([
    auraevent4.evidence.vanilla32.rawState,
    auraevent4.evidence.bkvince.rawState,
    auraevent4.evidence.pd2.rawState,
  ], ['EMPTY_STRING', 'EMPTY_STRING', 'ABSENT_COLUMN']);
  assert.equal(auraevent4.rawChanged, true);
  assert.equal(auraevent4.semanticChanged, false);
  assert.equal(auraevent4.decisionRelevant, false);
});

test('Fire Bolt reduction accounts for all 82 current fields and yields six behavior decisions', () => {
  const impact = orientation.fireBoltImpact;
  assert.equal(impact.currentModifiedFields, 82);
  assert.equal(impact.currentRequiredDecisions, 83);
  assert.equal(impact.reductions.semanticBlank.count, 61);
  assert.equal(impact.reductions.preserveD2rColumnAbsentFromPd2.count, 2);
  assert.equal(impact.reductions.vanillaHistoricalOnly.count, 1);
  assert.equal(impact.reductions.technicalOrDocumentary.count, 1);
  assert.equal(impact.reductions.bundled.count, 17);
  assert.equal(impact.finalPlayerDecisions, 6);
  assert.equal(impact.targetRange.met, true);
  assert.equal(impact.accounting.noRelevantDifferenceHidden, true);
  assert.deepEqual(impact.accounting.unaccountedCurrentFieldIds, []);
  assert.deepEqual(impact.bundleProjection.map((bundle) => bundle.groupId), [
    'DAMAGE_SYNERGIES',
    'ELEMENTAL_DAMAGE_CURVE',
    'ITEM_TRIGGER_EXECUTION',
    'MANA_CURVE',
    'NATIVE_EXECUTION',
    'PROJECTILE_PHYSICS',
  ]);
});

test('all global policies remain pending and carry definition fingerprints', () => {
  assert.equal(orientation.policies.length, 8);
  for (const policy of orientation.policies) {
    assert.equal(policy.decision, 'PENDING');
    const definition = GLOBAL_SCHEMA_POLICIES.find((candidate) => candidate.id === policy.id);
    assert.equal(policy.fingerprint, sha256Canonical(definition));
  }
});

test('policy schema validates pending envelopes and rejects stale fingerprints or unjustified approval', () => {
  const ajv = new Ajv2020({ allErrors: true, strict: false });
  addFormats(ajv);
  const validate = ajv.compile(generated.policySchema);
  const envelope = buildPolicyEnvelope(orientation);
  assert.equal(validate(envelope), true, JSON.stringify(validate.errors));
  const runtimeValidation = validatePolicyEnvelope(orientation, envelope);
  assert.equal(runtimeValidation.valid, true, JSON.stringify(runtimeValidation.errors));

  const policyId = orientation.policies[0].id;
  const stale = structuredClone(envelope);
  stale.decisions[policyId].fingerprint = '0'.repeat(64);
  assert.equal(validate(stale), false);

  const unjustified = structuredClone(envelope);
  unjustified.decisions[policyId].decision = 'APPROVE';
  assert.equal(validate(unjustified), false);
});

test('orientation hashes only leaf source hashes and binds all required documentary/native evidence', () => {
  assert.deepEqual(Object.keys(orientation.sourceHashes.references).sort(), [
    'analyticalAudit',
    'knownRvas',
    'nativeFindings',
    'skillsSchema',
  ]);
  for (const hash of Object.values(orientation.sourceHashes.skills)) assert.match(hash, /^[0-9A-F]{64}$/);
  for (const hash of Object.values(orientation.sourceHashes.references)) assert.match(hash, /^[0-9A-F]{64}$/);
  assert.equal(orientation.sourceManifest.references.skillsSchema.path, 'schemas/skills.json');
  assert.equal(orientation.sourceManifest.references.nativeFindings.path, 'reverse-engineering/d2r-3.2.92777/findings.md');
  assert.equal(orientation.sourceManifest.references.knownRvas.path, 'reverse-engineering/d2r-3.2.92777/known-rvas.json');
});

test('orientationHash is deterministic and independent from workbenchBinding comparisonHash', () => {
  const references = loadOrientationReferences();
  const first = buildSchemaOrientationData(generated.sources, generated.skillReport, {
    references,
    workbenchBinding: { reviewId: 'a', comparisonHash: 'A'.repeat(64) },
  });
  const second = buildSchemaOrientationData(generated.sources, generated.skillReport, {
    references,
    workbenchBinding: { reviewId: 'b', comparisonHash: 'B'.repeat(64) },
  });
  assert.equal(first.orientationHash, second.orientationHash);
  assert.notDeepEqual(first.workbenchBinding, second.workbenchBinding);
});

test('PD2 source honors its canonical case-sensitive Skills.txt and artifacts are deterministic', () => {
  assert.equal(path.basename(generated.sources.sourceManifest.pd2.tables['skills.txt'].path), 'Skills.txt');
  assert.equal(checkSchemaOrientationArtifacts(generated), true);
  for (const [id, filePath] of Object.entries(ORIENTATION_ARTIFACT_PATHS)) {
    assert.equal(fs.readFileSync(filePath, 'utf8'), generated.artifacts[id], id);
  }
});

test('Phase 0 generation leaves all governed gameplay Skills.txt files byte-identical', () => {
  assert.deepEqual(gameplayAfter, gameplayBefore);
  assert.deepEqual(gameplayPaths.map((filePath) => path.relative(repoRoot, filePath).replaceAll('\\', '/')), [
    'data-vanilla3.2/data/data/global/excel/skills.txt',
    'data-BKVince/BKVince.mpq/data/global/excel/skills.txt',
    '../PD2 Single PLayer/PD2-Single-Player-Plus-mod-main/data/global/excel/Skills.txt',
  ]);
});
