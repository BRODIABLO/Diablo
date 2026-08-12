import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import test from 'node:test';

import {
  buildSchemaOrientationData,
} from './pd2-skills-schema-orientation-data.mjs';
import { loadOrientationReferences } from './pd2-skills-schema-orientation.mjs';
import {
  applyPhase1DataModel,
  loadCanonicalSchemaPolicy,
  validateAndMigrateApprovedPolicy,
} from './pd2-skills-phase1.mjs';
import {
  DEFAULT_SOURCE_ROOTS,
  buildOracleData,
  loadWorkbenchSources,
} from './pd2-skills-review-data.mjs';

function sha256File(filePath) {
  return crypto.createHash('sha256').update(fs.readFileSync(filePath)).digest('hex').toUpperCase();
}

const gameplayPaths = [
  `${DEFAULT_SOURCE_ROOTS.vanilla32}/skills.txt`,
  `${DEFAULT_SOURCE_ROOTS.bkvince}/skills.txt`,
  `${DEFAULT_SOURCE_ROOTS.pd2}/Skills.txt`,
];
const gameplayBefore = gameplayPaths.map(sha256File);
const sources = loadWorkbenchSources();
const baseReport = buildOracleData(sources);
const orientation = buildSchemaOrientationData(sources, baseReport, {
  references: loadOrientationReferences(),
});
const schemaPolicy = loadCanonicalSchemaPolicy(orientation);
const report = applyPhase1DataModel(baseReport, { orientation, schemaPolicy, sources });
const fireBolt = report.skills.find((skill) => skill.canonicalName === 'Fire Bolt');

test('approved policy migration is strict, canonical, and retains all eight governed decisions', () => {
  assert.equal(report.phase1Model, 'behavior-bundles-v1');
  assert.deepEqual(schemaPolicy.migrationReport.counts, { retained: 8, stale: 0, dropped: 0 });
  assert.equal(schemaPolicy.migrationReport.sourceDriftClassification, 'AUDITED_UNRELATED_NATIVE_EVIDENCE_DRIFT');
  assert.equal(Object.keys(schemaPolicy.envelope.decisions).length, 8);
  assert.ok(Object.values(schemaPolicy.envelope.decisions).every((entry) => entry.decision === 'APPROVE' && entry.justification.trim()));
  assert.equal(report.policyHashes.canonicalSchemaPolicy, schemaPolicy.canonicalPolicyHash);
  assert.equal(report.schemaPolicy.canonicalPolicyHash, schemaPolicy.canonicalPolicyHash);
});

test('any source drift outside native findings and known RVAs is rejected', () => {
  const approved = JSON.parse(fs.readFileSync('Mission/pd2-skills-schema-policy-approved.json', 'utf8'));
  const changed = structuredClone(orientation);
  changed.sourceHashes.skills.bkvince = 'A'.repeat(64);
  assert.throws(
    () => validateAndMigrateApprovedPolicy(changed, approved),
    /source drift is not allowed/,
  );
});

test('Fire Bolt exposes exactly four player bundles and two visible prefilled technical packages', () => {
  assert.ok(fireBolt);
  assert.deepEqual(
    fireBolt.decisionBundles.filter((bundle) => bundle.scope === 'PLAYER').map((bundle) => bundle.id).sort(),
    ['DAMAGE_SYNERGIES', 'ELEMENTAL_DAMAGE_CURVE', 'MANA_CURVE', 'PROJECTILE_PHYSICS'],
  );
  assert.deepEqual(
    fireBolt.decisionBundles.filter((bundle) => bundle.scope === 'TECHNICAL').map((bundle) => [bundle.id, bundle.manualDecisionRequired, bundle.autoResolution]).sort(),
    [
      ['ITEM_TRIGGER_EXECUTION', false, 'PRESERVE_BKVINCE'],
      ['NATIVE_EXECUTION', false, 'DEFER_NATIVE_PROOF'],
    ],
  );
  assert.deepEqual(fireBolt.policyApplication.counts.playerBundles, 4);
  assert.deepEqual(fireBolt.policyApplication.counts.technicalBundles, 2);
});

test('bundle ownership is unique and every owned raw field preserves three-way physical evidence', () => {
  const fields = fireBolt.components.flatMap((component) => component.fields);
  const byId = new Map(fields.map((field) => [field.id, field]));
  const owned = new Set();
  for (const bundle of fireBolt.decisionBundles) {
    for (const fieldId of bundle.fieldIds) {
      assert.ok(!owned.has(fieldId), fieldId);
      owned.add(fieldId);
      const field = byId.get(fieldId);
      assert.ok(field, fieldId);
      assert.equal(field.decisionOwnerBundleId, bundle.id);
      assert.deepEqual(Object.keys(field.rawEvidence), ['vanilla32', 'bkvince', 'pd2']);
      for (const evidence of Object.values(field.rawEvidence)) {
        for (const key of ['columnPresent', 'rowPresent', 'rawHeader', 'rawValue', 'rawState', 'semanticBlank']) {
          assert.ok(Object.hasOwn(evidence, key), `${fieldId}/${key}`);
        }
      }
    }
  }
  assert.ok(byId.has('missiles.txt:firebolt:range'));
  assert.ok(byId.has('missiles.txt:firebolt:psrvhitfunc'));
});

test('the eight policies eliminate irrelevant fields without erasing their raw evidence', () => {
  assert.ok(fireBolt.policyApplication.reductions.semanticBlank.includes('skills.txt:auraevent4'));
  assert.ok(fireBolt.policyApplication.reductions.preserveD2rColumnAbsentFromPd2.includes('skills.txt:rightskill'));
  assert.ok(fireBolt.policyApplication.reductions.itemEconomy.includes('skills.txt:cost add'));
  const fields = new Map(fireBolt.components.flatMap((component) => component.fields).map((field) => [field.id, field]));
  assert.equal(fields.get('skills.txt:auraevent4').decisionRelevant, false);
  assert.equal(fields.get('skills.txt:auraevent4').rawEvidence.pd2.columnPresent, false);
  assert.equal(fields.get('skills.txt:auraevent4').rawEvidence.bkvince.rawValue, '');
  assert.equal(fields.get('skills.txt:cost add').policyResolution, 'PRESERVE_BKVINCE');
});

test('Fire Bolt metrics omit inapplicable poison and delay series', () => {
  const metricIds = fireBolt.curves.standard.series.map((series) => series.id);
  assert.deepEqual(metricIds, ['damage_min', 'damage_max', 'damage_average', 'mana']);
  assert.ok(metricIds.every((id) => !id.includes('poison') && !id.includes('delay')));
});

test('Fire Bolt synergy20 computes finite per-source curves from per-skill hard point maps', () => {
  const scenario = fireBolt.curves.synergies20;
  assert.equal(scenario.proofStatus, 'EXACT_FORMULA');
  assert.equal(scenario.hardPointsBySkill.pd2.combustion, 20);
  assert.equal(scenario.hardPointsBySkill.pd2['fire-ball'], 20);
  assert.equal(scenario.hardPointsBySkill.bkvince.meteor, 20);
  assert.deepEqual(scenario.series.map((series) => series.id), ['damage_min', 'damage_max', 'damage_average', 'mana']);
  for (const series of scenario.series) {
    for (const values of Object.values(series.values)) {
      assert.equal(values.length, 6);
      assert.ok(values.every(Number.isFinite), `${series.id} contains a non-finite value`);
    }
  }
  assert.notDeepEqual(
    scenario.series.find((series) => series.id === 'damage_average').values.pd2,
    fireBolt.curves.standard.series.find((series) => series.id === 'damage_average').values.pd2,
  );
  assert.deepEqual(scenario.series.find((series) => series.id === 'damage_min').values.bkvince,
    [22.19921875, 66.59765625, 129.5, 336.69921875, 1195.09765625, 3193.09765625]);
  assert.deepEqual(scenario.series.find((series) => series.id === 'damage_min').values.pd2,
    [24.59765625, 90.19921875, 270.59765625, 1156.19921875, 2911, 4797]);
  assert.deepEqual(scenario.series.find((series) => series.id === 'damage_max').values.bkvince,
    [44.3984375, 88.796875, 166.5, 447.69921875, 1380.09765625, 3452.09765625]);
  assert.deepEqual(scenario.series.find((series) => series.id === 'damage_max').values.pd2,
    [49.19921875, 147.59765625, 369, 1336.59765625, 3173.3984375, 5141.3984375]);
  assert.doesNotMatch(JSON.stringify(scenario), /undefined/);
});

test('linked native callback differences propagate into the skill portability gate', () => {
  const mercFireBall = report.skills.find((skill) => skill.stableId === 'skill:classless:a3-merc-fire-ball');
  assert.ok(mercFireBall);
  for (const category of ['NATIVE_FUNCTION_MISMATCH', 'NATIVE_UNPROVEN', 'NETWORK_OR_CLIENT_SERVER_RISK']) {
    assert.ok(mercFireBall.portability.categories.includes(category), category);
  }
  assert.ok(mercFireBall.portability.divergentFunctions.includes('missiles.txt:a3-merc-fireball:pcltdofunc'));
  assert.ok(mercFireBall.portability.proofRequired.includes('D2R_3_2_NATIVE_FUNCTION_AUDIT'));
  assert.equal(mercFireBall.portability.networkRisk, 'CLIENT_SERVER_BEHAVIOR_UNPROVEN');
});

test('Phase 1 is deterministic and leaves all three Skills.txt sources byte-identical', () => {
  const second = applyPhase1DataModel(baseReport, { orientation, schemaPolicy, sources });
  assert.equal(second.comparisonHash, report.comparisonHash);
  assert.deepEqual(gameplayPaths.map(sha256File), gameplayBefore);
});
