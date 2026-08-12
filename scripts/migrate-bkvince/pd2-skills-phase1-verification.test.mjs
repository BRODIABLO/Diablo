import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';

import {
  FROZEN_CONTRACT_HASH,
  ORACLE_SCHEMA_VERSION,
  PHASE1_DECISION_MODEL,
  sha256Canonical,
} from './pd2-skills-review-contracts.mjs';
import {
  DEFAULT_SOURCE_ROOTS,
  repoRoot,
} from './pd2-skills-review-data.mjs';
import {
  loadCanonicalSchemaPolicy,
  PHASE1_MODEL_ID,
} from './pd2-skills-phase1.mjs';
import {
  createEmptyEnvelope,
  projectProposedResult,
} from './pd2-skills-review-runtime.mjs';
import { buildIntegratedWorkbenchReport } from './pd2-skills-review.mjs';
import {
  policyGate,
  validatePolicyEnvelope,
} from './pd2-skills-schema-policy-runtime.mjs';
import { parseCli as parsePreviewCli } from './pd2-skills-decisions-preview.mjs';

const APPROVED_POLICY_PATH = path.join(repoRoot, 'Mission', 'pd2-skills-schema-policy-approved.json');
const CANONICAL_POLICY_PATH = path.join(repoRoot, 'Mission', 'pd2-skills-schema-policy.json');
const REPORT_PATH = path.join(repoRoot, 'Mission', 'pd2-skills-review.json');

const APPROVED_SOURCE_HASHES = Object.freeze({
  skills: Object.freeze({
    vanilla32: 'EFAF7AC4BA0493109C698EF32ACF4A2B3A577E13500D0B50258C80B600986F51',
    bkvince: '08497CC0BD8B2B5CBD895F7477AD0CBF272571FB67B78061EDFABC31C48B8B77',
    pd2: 'AEEFC3F2C0C80811D62FC1A17C3B031DE2164E5606BF9779F34024B35BC87B8B',
  }),
  references: Object.freeze({
    skillsSchema: '944A40AA17BF44C8D5B262482925FAD26CF3B20FBE0B941D779D6E60F36DE742',
    analyticalAudit: 'BE9385A532CBD3DF80D94E83F04293FB9238DFD10101C5A9F442DB8DAC07D565',
    nativeFindings: '0768EF47A41793AB5C1D30C06F64A79A794870AAD0E835628385DBE646587C3D',
    knownRvas: '1FA2981D14D3BA2B8F4F6E8ECF6CFDE5EE8D6E24AC742E8838B65FA46F31E086',
  }),
});

const APPROVED_POLICY_FINGERPRINTS = Object.freeze({
  PRESERVE_ALL_D2R_BKVINCE_COLUMNS: '5B6B5E308F2B8D94AEAC2B67B7F1B9E567E0878DEE92DF39918FB4BE3CC41140',
  IGNORE_ALL_EMPTY_PD2_ONLY_COLUMNS: 'B98AA31CC1AFFBD286BE2C17B027066946303D93D9487810AC6CFC8CAA597207',
  NO_PD2_HEADER_IMPORT_WITHOUT_NATIVE_PROOF: '133016232D83CA600BA170D4B3F05289EFCAC956BA2BD5AEE5492C6DE79E18B6',
  NO_AUTOMATIC_DELAY_TRANSLATION: '229A2054D3039368203F246099BB33FEEEF0DCE394C606B448FE68AD8D0BBA0B',
  SEMANTIC_BLANKS_REQUIRE_NO_DECISION: 'C1E042EEFE8A1709111DFCE334B0EC402DE06B43E4060474A765ADE4D929DA34',
  ITEM_ECONOMY_FIELDS_PRESERVE_BKVINCE: 'E9E2385AE2067753B4441C6C1EAD5C7941437B3671990090A7CCCD59FA01C07A',
  NATIVE_CALLBACKS_PRESERVE_OR_DEFER: '7AF76741FBB8DF2FF7F663359D718677AA2F165BEB6FC330FA59F5E17FAE3AB5',
  RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE: '96B22A8F9A486CAFF2D67CFD8E575E05285B9EFCF3A26310270B74083DCFAC36',
});

const GAMEPLAY_AUTHORITIES = Object.freeze({
  vanilla32: path.join(DEFAULT_SOURCE_ROOTS.vanilla32, 'skills.txt'),
  bkvince: path.join(DEFAULT_SOURCE_ROOTS.bkvince, 'skills.txt'),
  pd2: path.join(DEFAULT_SOURCE_ROOTS.pd2, 'Skills.txt'),
});

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8').replace(/^\uFEFF/, ''));
}

function sha256(value) {
  return crypto.createHash('sha256').update(value).digest('hex').toUpperCase();
}

function sha256File(filePath) {
  return sha256(fs.readFileSync(filePath));
}

const gameplayBytesBefore = Object.fromEntries(
  Object.entries(GAMEPLAY_AUTHORITIES).map(([source, filePath]) => [source, fs.readFileSync(filePath)]),
);

let integrated;
function generatedReport() {
  integrated ??= buildIntegratedWorkbenchReport(DEFAULT_SOURCE_ROOTS);
  return integrated.report;
}

function fieldMap(skill) {
  return new Map((skill.components ?? []).flatMap((component) => (
    (component.fields ?? []).map((field) => [field.id, { component, field }])
  )));
}

function fireBolt(report = generatedReport()) {
  const matches = report.skills.filter((skill) => skill.stableId === 'skill:sor:fire-bolt');
  assert.equal(matches.length, 1, 'Fire Bolt must have one stable semantic identity');
  return matches[0];
}

function metric(scenario, id) {
  const matches = scenario.series.filter((series) => series.id === id);
  assert.equal(matches.length, 1, `${scenario.id}/${id} must resolve to one metric`);
  return matches[0];
}

test.after(() => {
  for (const [source, filePath] of Object.entries(GAMEPLAY_AUTHORITIES)) {
    assert.deepEqual(fs.readFileSync(filePath), gameplayBytesBefore[source], `${source} Skills.txt bytes changed`);
  }
});

test('approved Phase 0 policy is imported strictly, migrated 8/8, and canonicalized once', () => {
  const report = generatedReport();
  const orientation = report.schemaOrientation;
  const approved = readJson(APPROVED_POLICY_PATH);
  const canonical = readJson(CANONICAL_POLICY_PATH);
  const loaded = loadCanonicalSchemaPolicy(orientation);

  assert.equal(approved.orientationId, 'pd2-skills-schema-orientation-v1');
  assert.equal(approved.orientationHash, '09B657C5AF91B5155BC613B422CBFEF4FD8FAD215A6B9F082ADF318010B01DD3');
  assert.equal(approved.frozenContractHash, '3133A16AD42315181599DBB5DE29C4C7DAEBC5DFB7F1110639C2CEBFEA13EC6B');
  assert.equal(sha256File(APPROVED_POLICY_PATH), '35EF03B53536DC8D0835716D675DAE9E701A1E6CD6B4A0DE1F19EEE871978A96');
  assert.deepEqual(approved.sourceHashes, APPROVED_SOURCE_HASHES);
  assert.deepEqual(
    Object.fromEntries(Object.entries(approved.decisions).map(([id, decision]) => [id, decision.fingerprint])),
    APPROVED_POLICY_FINGERPRINTS,
  );
  assert.equal(Object.keys(approved.decisions).length, 8);
  assert.ok(Object.values(approved.decisions).every((decision) => (
    decision.decision === 'APPROVE' && String(decision.justification).trim().length > 0
  )));

  assert.deepEqual(loaded.envelope, canonical);
  assert.equal(loaded.canonicalPolicyHash, sha256Canonical(canonical));
  assert.equal(loaded.approvedExportHash, sha256File(APPROVED_POLICY_PATH));
  assert.deepEqual(loaded.migrationReport.counts, { retained: 8, stale: 0, dropped: 0 });
  assert.equal(loaded.migrationReport.fromOrientationHash, approved.orientationHash);
  assert.equal(loaded.migrationReport.toOrientationHash, orientation.orientationHash);
  assert.equal(canonical.orientationId, orientation.orientationId);
  assert.equal(canonical.orientationHash, orientation.orientationHash);
  assert.equal(canonical.frozenContractHash, orientation.frozenContractHash);
  assert.deepEqual(canonical.sourceHashes, orientation.sourceHashes);
  assert.deepEqual(
    Object.fromEntries(orientation.policies.map((policy) => [policy.id, policy.fingerprint])),
    APPROVED_POLICY_FINGERPRINTS,
  );
  assert.equal(validatePolicyEnvelope(orientation, canonical).valid, true);
  assert.deepEqual(policyGate(orientation, canonical), {
    ...policyGate(orientation, canonical),
    complete: true,
    open: false,
    required: 8,
    closed: 8,
    remaining: 0,
  });
});

test('the checked oracle is the deterministic Phase 1 report bound to the frozen contract', () => {
  const report = generatedReport();
  const checked = readJson(REPORT_PATH);
  assert.equal(PHASE1_MODEL_ID, PHASE1_DECISION_MODEL);
  assert.equal(report.phase1Model, PHASE1_DECISION_MODEL);
  assert.equal(report.schemaVersion, ORACLE_SCHEMA_VERSION);
  assert.equal(report.frozenContractHash, FROZEN_CONTRACT_HASH);
  assert.equal(checked.phase1Model, PHASE1_DECISION_MODEL);
  assert.equal(checked.comparisonHash, report.comparisonHash);
  assert.deepEqual(checked.schemaPolicy, report.schemaPolicy);
  assert.equal(report.policyHashes.canonicalSchemaPolicy, report.schemaPolicy.canonicalPolicyHash);
});

test('Fire Bolt reduces the historical 83 decisions to exactly four player bundles and two technical packages', () => {
  const report = generatedReport();
  const skill = fireBolt(report);
  const player = skill.decisionBundles.filter((bundle) => bundle.scope === 'PLAYER');
  const technical = skill.decisionBundles.filter((bundle) => bundle.scope === 'TECHNICAL');

  assert.equal(report.schemaOrientation.fireBoltImpact.currentRequiredDecisions, 83);
  assert.equal(report.schemaOrientation.fireBoltImpact.currentModifiedFields, 82);
  assert.deepEqual(player.map((bundle) => bundle.id).sort(), [
    'DAMAGE_SYNERGIES',
    'ELEMENTAL_DAMAGE_CURVE',
    'MANA_CURVE',
    'PROJECTILE_PHYSICS',
  ]);
  assert.deepEqual(technical.map((bundle) => bundle.id).sort(), [
    'ITEM_TRIGGER_EXECUTION',
    'NATIVE_EXECUTION',
  ]);
  assert.ok(player.every((bundle) => bundle.manualDecisionRequired === true && bundle.autoResolution == null));
  assert.ok(technical.every((bundle) => bundle.manualDecisionRequired === false));
  assert.equal(technical.find((bundle) => bundle.id === 'ITEM_TRIGGER_EXECUTION').autoResolution, 'PRESERVE_BKVINCE');
  assert.equal(technical.find((bundle) => bundle.id === 'NATIVE_EXECUTION').autoResolution, 'DEFER_NATIVE_PROOF');
  assert.deepEqual(skill.policyApplication.counts, {
    rawFields: 98,
    decisionRelevant: 20,
    playerBundles: 4,
    technicalBundles: 2,
    autoResolvedFields: 82,
  });
  assert.equal(skill.policyApplication.reductions.semanticBlank.length, 61);

  const fields = fieldMap(skill);
  const owned = new Map();
  for (const bundle of skill.decisionBundles) {
    for (const fieldId of bundle.fieldIds) {
      assert.ok(fields.has(fieldId), `${bundle.id} references missing ${fieldId}`);
      assert.equal(owned.has(fieldId), false, `${fieldId} has multiple bundle owners`);
      owned.set(fieldId, bundle.id);
      assert.equal(fields.get(fieldId).field.decisionOwnerBundleId, bundle.id);
    }
  }
  assert.equal(owned.size, 20);
});

test('semantic blanks preserve physical evidence while creating no player decision', () => {
  const fields = fieldMap(fireBolt());
  const auraevent4 = fields.get('skills.txt:auraevent4').field;
  assert.deepEqual(
    ['vanilla32', 'bkvince', 'pd2'].map((source) => auraevent4.rawEvidence[source].rawState),
    ['EMPTY_STRING', 'EMPTY_STRING', 'ABSENT_COLUMN'],
  );
  assert.ok(['vanilla32', 'bkvince', 'pd2'].every((source) => auraevent4.rawEvidence[source].semanticBlank === true));
  assert.equal(auraevent4.rawChanged, true);
  assert.equal(auraevent4.semanticChanged, false);
  assert.equal(auraevent4.decisionRelevant, false);
  assert.equal(auraevent4.decisionOwnerBundleId, null);
  assert.equal(auraevent4.policyResolution, 'NO_DECISION');
  assert.equal(auraevent4.semanticDifferenceReason, 'ALL_SEMANTIC_BLANK');

  for (const { field } of fields.values()) {
    for (const source of ['vanilla32', 'bkvince', 'pd2']) {
      const evidence = field.rawEvidence[source];
      for (const key of ['columnPresent', 'rowPresent', 'rawHeader', 'rawValue', 'rawState', 'semanticBlank']) {
        assert.ok(Object.hasOwn(evidence, key), `${field.id}/${source} lacks ${key}`);
      }
      if (evidence.rawState === 'ZERO') assert.equal(evidence.semanticBlank, false, `${field.id}/${source}`);
    }
  }
});

test('D2R preservation, item economy, PD2-only and native callback policies auto-resolve safely', () => {
  const report = generatedReport();
  const skill = fireBolt(report);
  const fields = fieldMap(skill);

  const d2rOnly = fields.get('skills.txt:rightskill').field;
  assert.equal(d2rOnly.rawEvidence.pd2.columnPresent, false);
  assert.equal(d2rOnly.rawEvidence.bkvince.semanticBlank, false);
  assert.equal(d2rOnly.policyResolution, 'PRESERVE_BKVINCE');
  assert.equal(d2rOnly.decisionOwnerBundleId, null);

  const economy = fields.get('skills.txt:cost add').field;
  assert.equal(economy.policyResolution, 'PRESERVE_BKVINCE');
  assert.equal(economy.semanticDifferenceReason, 'AUTO_RESOLVED_ITEM_ECONOMY');
  assert.equal(economy.decisionOwnerBundleId, null);

  for (const id of ['skills.txt:itemeffect', 'skills.txt:itemclteffect']) {
    const itemTrigger = fields.get(id).field;
    assert.equal(itemTrigger.decisionOwnerBundleId, 'ITEM_TRIGGER_EXECUTION');
    assert.equal(itemTrigger.policyResolution, 'PRESERVE_BKVINCE');
  }
  for (const id of ['missiles.txt:firebolt:psrvhitfunc', 'missiles.txt:firebolt:shitpar1']) {
    const native = fields.get(id).field;
    assert.equal(native.decisionOwnerBundleId, 'NATIVE_EXECUTION');
    assert.equal(native.policyResolution, 'DEFER_NATIVE_PROOF');
    assert.equal(native.proofStatus, 'NATIVE_UNPROVEN');
  }

  const populatedCheckfunc = report.skills
    .flatMap((candidate) => [...fieldMap(candidate).values()].map(({ field }) => ({ candidate, field })))
    .find(({ field }) => field.id === 'skills.txt:checkfunc' && !field.rawEvidence.pd2.semanticBlank);
  assert.ok(populatedCheckfunc, 'a populated PD2 checkfunc portability witness is required');
  assert.equal(populatedCheckfunc.field.policyResolution, 'DEFER_NATIVE_PROOF');
  assert.equal(populatedCheckfunc.field.semanticDifferenceReason, 'GLOBAL_PORTABILITY_GATE_PRECEDES_SKILL_DECISION');
  assert.equal(populatedCheckfunc.field.decisionOwnerBundleId, null);
});

test('Fire Bolt projectile Range is linked table evidence owned by PROJECTILE_PHYSICS', () => {
  const field = fieldMap(fireBolt()).get('missiles.txt:firebolt:range').field;
  assert.equal(field.table, 'missiles.txt');
  assert.equal(field.rowKey, 'firebolt');
  assert.equal(field.rawEvidence.vanilla32.rawValue, '50');
  assert.equal(field.rawEvidence.bkvince.rawValue, '50');
  assert.equal(field.rawEvidence.pd2.rawValue, '30');
  assert.equal(field.proofStatus, 'EXACT_TABLE');
  assert.equal(field.decisionOwnerBundleId, 'PROJECTILE_PHYSICS');
  assert.equal(field.decisionRelevant, true);
});

test('Fire Bolt synergy scenarios calculate exact finite curves and omit inapplicable poison/delay metrics', () => {
  const curves = fireBolt().curves;
  const expected = {
    damage_min: {
      vanilla32: [22.19921875, 66.59765625, 129.5, 336.69921875, 1195.09765625, 3193.09765625],
      bkvince: [22.19921875, 66.59765625, 129.5, 336.69921875, 1195.09765625, 3193.09765625],
      pd2: [24.59765625, 90.19921875, 270.59765625, 1156.19921875, 2911, 4797],
    },
    damage_max: {
      vanilla32: [44.3984375, 88.796875, 166.5, 447.69921875, 1380.09765625, 3452.09765625],
      bkvince: [44.3984375, 88.796875, 166.5, 447.69921875, 1380.09765625, 3452.09765625],
      pd2: [49.19921875, 147.59765625, 369, 1336.59765625, 3173.3984375, 5141.3984375],
    },
  };
  assert.deepEqual(curves.synergies20.levels, [1, 5, 10, 20, 30, 40]);
  assert.equal(curves.synergies20.proofStatus, 'EXACT_FORMULA');
  assert.deepEqual(curves.synergies20.hardPointsBySkill, {
    vanilla32: { 'fire-ball': 20, meteor: 20 },
    bkvince: { 'fire-ball': 20, meteor: 20 },
    pd2: { combustion: 20, 'fire-ball': 20 },
  });
  for (const [id, values] of Object.entries(expected)) {
    assert.deepEqual(metric(curves.synergies20, id).values, values);
    assert.ok(Object.values(values).flat().every(Number.isFinite), id);
  }

  const forbidden = /(?:poison|delay|cooldown)/i;
  for (const scenario of [curves.standard, curves.synergies20, curves.custom]) {
    assert.ok(scenario.series.every((series) => !forbidden.test(series.id)), `${scenario.id} exposes an inapplicable metric`);
    assert.doesNotMatch(JSON.stringify(scenario), /undefined\s*:\s*undefined/i);
  }
});

test('proposed-result projection is pure, honors bundles, and keeps technical packages on BKVince', () => {
  const report = generatedReport();
  const skill = fireBolt(report);
  const envelope = createEmptyEnvelope(report);
  const entry = envelope.entries[skill.stableId];
  for (const bundle of skill.decisionBundles.filter((candidate) => candidate.scope === 'PLAYER')) {
    entry.bundleDecisions[bundle.id] = { decision: 'KEEP_BKVINCE' };
  }
  entry.bundleDecisions.ELEMENTAL_DAMAGE_CURVE = { decision: 'ADOPT_PD2' };
  entry.bundleDecisions.PROJECTILE_PHYSICS = { decision: 'ADOPT_PD2' };

  const reportHash = sha256Canonical(report);
  const entryHash = sha256Canonical(entry);
  const projection = projectProposedResult(report, skill, entry);
  assert.equal(projection.valid, true, projection.errors.join('\n'));
  assert.equal(sha256Canonical(report), reportHash, 'projection mutated the oracle');
  assert.equal(sha256Canonical(entry), entryHash, 'projection mutated the decision entry');

  assert.equal(projection.byField['skills.txt:emaxlev1'].source, 'PLAYER_BUNDLE_DECISION');
  assert.equal(projection.byField['skills.txt:emaxlev1'].before, '3');
  assert.equal(projection.byField['skills.txt:emaxlev1'].after, '6');
  assert.equal(projection.byField['skills.txt:mana'].after, projection.byField['skills.txt:mana'].before);
  assert.equal(projection.byField['missiles.txt:firebolt:range'].before, '50');
  assert.equal(projection.byField['missiles.txt:firebolt:range'].after, '30');
  assert.equal(projection.byField['skills.txt:cost add'].after, projection.byField['skills.txt:cost add'].before);

  for (const id of [
    'skills.txt:itemeffect',
    'skills.txt:itemclteffect',
    'missiles.txt:firebolt:psrvhitfunc',
    'missiles.txt:firebolt:shitpar1',
  ]) {
    const cell = projection.byField[id];
    assert.equal(cell.source, 'TECHNICAL_AUTO_RESOLUTION');
    assert.equal(cell.decision, 'KEEP_BKVINCE');
    assert.equal(cell.after, cell.before);
  }
  assert.equal(projection.byField['missiles.txt:firebolt:psrvhitfunc'].autoResolution, 'DEFER_NATIVE_PROOF');

  assert.throws(
    () => parsePreviewCli(['--apply']),
    /Gameplay application is forbidden; this compiler is preview-only/,
  );
  for (const [source, filePath] of Object.entries(GAMEPLAY_AUTHORITIES)) {
    assert.equal(sha256File(filePath), APPROVED_SOURCE_HASHES.skills[source]);
    assert.deepEqual(fs.readFileSync(filePath), gameplayBytesBefore[source]);
  }
});
