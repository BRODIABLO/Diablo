import assert from 'node:assert/strict';
import fs from 'node:fs';
import test from 'node:test';

import {
  FROZEN_CONTRACT_HASH,
  MAPPING_TYPES,
  PORTABILITY_CATEGORIES,
  PROOF_STATUSES,
} from './pd2-skills-review-contracts.mjs';
import {
  OUTPUT_PATHS,
  buildDocumentationMap,
  checkSkillReviewArtifacts,
  generateSkillReviewArtifacts,
} from './pd2-skills-review.mjs';

const generated = generateSkillReviewArtifacts();
const byName = new Map(generated.report.skills.map((skill) => [skill.canonicalName.toLowerCase(), skill]));

test('orchestration preserves the frozen contract and deterministic comparison identity', () => {
  assert.equal(generated.report.frozenContractHash, FROZEN_CONTRACT_HASH);
  assert.match(generated.report.comparisonHash, /^[A-F0-9]{64}$/);
  const second = generateSkillReviewArtifacts();
  assert.deepEqual(second.hashes, generated.hashes);
  assert.equal(second.raw.report, generated.raw.report);
  assert.equal(second.raw.html, generated.raw.html);
});

test('all governed enums exposed by the oracle belong to the frozen contracts', () => {
  for (const skill of generated.report.skills) {
    for (const mapping of skill.mappingTypes) assert(MAPPING_TYPES.includes(mapping), `${skill.stableId}: ${mapping}`);
    assert(PROOF_STATUSES.includes(skill.evidence.overall), `${skill.stableId}: ${skill.evidence.overall}`);
    for (const category of skill.portability.categories) {
      assert(PORTABILITY_CATEGORIES.includes(category), `${skill.stableId}: ${category}`);
    }
  }
});

test('documentation map is a complete fingerprint-bound derivative of the oracle', () => {
  const map = buildDocumentationMap(generated.report);
  assert.equal(Object.keys(map.entries).length, generated.report.skills.length);
  for (const skill of generated.report.skills) {
    assert.equal(map.entries[skill.stableId].fingerprint, skill.fingerprint);
  }
});

test('mandatory product witnesses survive integrated generation', () => {
  for (const name of [
    'Amplify Damage', 'Fire Ball', 'Fire Wall', 'Cold Enchant', 'Combustion',
    'Ice Barrage', 'Raven', 'Hydra', 'Frozen Orb',
  ]) assert(byName.has(name.toLowerCase()), `missing witness ${name}`);
  assert(generated.report.skills.some((skill) => skill.identical && skill.readOnly), 'missing identical read-only witness');
  assert(generated.report.skills.some((skill) => skill.classCode === 'war' && skill.bkvinceOnlyPlayerSkill), 'missing BKVince-only Warlock witness');
});

test('checked-in artifacts are byte-identical to the deterministic generator', () => {
  for (const filePath of Object.values(OUTPUT_PATHS)) assert(fs.existsSync(filePath), `missing ${filePath}`);
  const checks = checkSkillReviewArtifacts(generated);
  assert(Object.values(checks).every((check) => check.matches));
});

test('generated HTML is standalone and contains no network dependency', () => {
  assert.match(generated.raw.html, /^<!doctype html>/i);
  assert.doesNotMatch(generated.raw.html, /<script[^>]+src=|<link[^>]+rel=["']?stylesheet|\bfetch\s*\(/i);
  assert.match(generated.raw.html, /pd2-skills-review-decisions-v1:/);
});
