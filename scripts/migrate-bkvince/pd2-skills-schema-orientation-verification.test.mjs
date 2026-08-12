import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';
import { pathToFileURL } from 'node:url';

import { chromium } from 'playwright';

import {
  DEFAULT_SOURCE_ROOTS,
  buildOracleData,
  loadWorkbenchSources,
  repoRoot,
} from './pd2-skills-review-data.mjs';
import {
  FROZEN_ORIENTATION_CONTRACT_HASH,
  GLOBAL_SCHEMA_POLICIES,
  NON_MUTATION_RULES as ORIENTATION_NON_MUTATION_RULES,
  POLICY_STORAGE_PREFIX,
  RAW_VALUE_STATES,
} from './pd2-skills-schema-orientation-contracts.mjs';
import {
  FROZEN_CONTRACT_HASH,
  NON_MUTATION_RULES,
  SCHEMA_ORIENTATION_INTERFACE,
  sha256Canonical,
} from './pd2-skills-review-contracts.mjs';
import {
  createEmptyEnvelope,
  entryState,
  progress,
} from './pd2-skills-review-runtime.mjs';
import {
  createPolicyEnvelope,
  policyGate,
} from './pd2-skills-schema-policy-runtime.mjs';
import {
  ORIENTATION_ARTIFACT_PATHS,
  buildSchemaOrientationArtifacts,
  checkSchemaOrientationArtifacts,
} from './pd2-skills-schema-orientation.mjs';

const REPORT_PATH = path.join(repoRoot, 'Mission', 'pd2-skills-review.json');
const WORKBENCH_HTML_PATH = path.join(repoRoot, 'Mission', 'pd2-skills-review.html');
const ORIENTATION_PATH = ORIENTATION_ARTIFACT_PATHS.orientationJson;
const ORIENTATION_HTML_PATH = ORIENTATION_ARTIFACT_PATHS.orientationHtml;
const DICTIONARY_PATH = ORIENTATION_ARTIFACT_PATHS.fieldDictionary;
const POLICY_PATH = ORIENTATION_ARTIFACT_PATHS.policyCurrent;
const POLICY_EXAMPLE_PATH = ORIENTATION_ARTIFACT_PATHS.policyExample;

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

function sha256File(filePath) {
  return crypto.createHash('sha256').update(fs.readFileSync(filePath)).digest('hex').toUpperCase();
}

function governedSkillsPath(root, expectedName) {
  const entries = fs.readdirSync(root);
  assert.ok(entries.includes(expectedName), `${root} must contain the case-exact ${expectedName}`);
  return path.join(root, expectedName);
}

const gameplaySkillsPaths = Object.freeze([
  governedSkillsPath(DEFAULT_SOURCE_ROOTS.vanilla32, 'skills.txt'),
  governedSkillsPath(DEFAULT_SOURCE_ROOTS.bkvince, 'skills.txt'),
  governedSkillsPath(DEFAULT_SOURCE_ROOTS.pd2, 'Skills.txt'),
]);
const gameplayHashesBefore = gameplaySkillsPaths.map(sha256File);

const report = readJson(REPORT_PATH);
const orientation = readJson(ORIENTATION_PATH);
const dictionary = readJson(DICTIONARY_PATH);
const policyDocument = readJson(POLICY_PATH);
const policyExample = readJson(POLICY_EXAMPLE_PATH);
const columns = new Map(orientation.columns.map((column) => [column.canonicalHeader, column]));

test('independent Phase 0 artifact audit binds one deterministic orientation to the Workbench', () => {
  assert.equal(orientation.frozenContractHash, FROZEN_ORIENTATION_CONTRACT_HASH);
  assert.equal(SCHEMA_ORIENTATION_INTERFACE.contractHash, FROZEN_ORIENTATION_CONTRACT_HASH);
  assert.equal(report.frozenContractHash, FROZEN_CONTRACT_HASH);
  assert.equal(report.policyHashes.schemaOrientationContract, FROZEN_ORIENTATION_CONTRACT_HASH);
  assert.equal(report.policyHashes.schemaOrientation, orientation.orientationHash);
  assert.equal(report.schemaOrientation.orientationHash, orientation.orientationHash);
  assert.equal(report.schemaOrientation.workbenchBinding.comparisonHash, report.comparisonHash);
  assert.equal(orientation.workbenchBinding.comparisonHash, report.comparisonHash);
  assert.deepEqual(report.schemaOrientation.sourceHashes, orientation.sourceHashes);

  for (const [source, manifest] of Object.entries(orientation.sourceManifest.skills)) {
    assert.equal(sha256File(path.resolve(repoRoot, manifest.path)), manifest.sha256, source);
    assert.equal(manifest.roundTripByteExact, true, source);
    assert.equal(orientation.sourceHashes.skills[source], manifest.sha256, source);
  }
  for (const [referenceId, manifest] of Object.entries(orientation.sourceManifest.references)) {
    assert.equal(sha256File(path.resolve(repoRoot, manifest.path)), manifest.sha256, referenceId);
    assert.equal(orientation.sourceHashes.references[referenceId], manifest.sha256, referenceId);
  }

  const generated = buildSchemaOrientationArtifacts();
  assert.equal(generated.orientation.orientationHash, orientation.orientationHash);
  assert.equal(checkSchemaOrientationArtifacts(generated), true);
});

test('matrix and dictionary exhaustively cover 330 canonical headers and all required classifications', () => {
  assert.equal(orientation.coverage.canonicalHeaders, 330);
  assert.deepEqual(orientation.coverage.sourceHeaders, { vanilla32: 322, bkvince: 322, pd2: 256 });
  assert.equal(orientation.columns.length, 330);
  assert.equal(dictionary.fields.length, 330);
  assert.equal(new Set(orientation.columns.map((column) => column.canonicalHeader)).size, 330);
  assert.deepEqual(dictionary.fields.map((field) => field.rawHeader), orientation.columns.map((column) => column.canonicalHeader));

  const requiredMatrixKeys = [
    'rawHeaders', 'presence', 'usage', 'examples', 'documentation', 'playerLabelFr', 'shortHelpFr',
    'family', 'consumer', 'classifications', 'primaryClassification', 'potentialEquivalent',
    'decisionScope', 'defaultPolicy', 'technicalOnly', 'protected', 'groupId', 'proofStatus',
    'rawChanged', 'semanticChanged', 'decisionRelevant', 'semanticDifferenceReason',
  ];
  for (const column of orientation.columns) {
    for (const key of requiredMatrixKeys) assert.ok(Object.hasOwn(column, key), `${column.canonicalHeader} lacks ${key}`);
    for (const source of ['vanilla32', 'bkvince', 'pd2']) {
      const usage = column.usage.bySource[source];
      assert.equal(usage.nonBlankCells, usage.playerSkills + usage.technicalOrMonsterRows, `${column.canonicalHeader}/${source}`);
    }
  }

  const requiredClassifications = [
    'D2R_NATIVE_PRESERVE', 'BKVINCE_EXTENSION_PRESERVE', 'PD2_SCHEMA_UNUSED',
    'PD2_SEMANTIC_SOURCE_ONLY', 'MAP_TO_EXISTING_D2R_FIELD', 'NATIVE_EXTENSION_REQUIRED',
    'DOCUMENTARY_ONLY', 'UI_ONLY', 'ITEM_ECONOMY_ONLY', 'RAW_TECHNICAL_OVERRIDE',
    'UNKNOWN_NATIVE_CONSUMER',
  ];
  for (const classification of requiredClassifications) {
    assert.ok(orientation.enums.schemaClassifications.includes(classification), classification);
  }
});

test('PD2-only columns split into four used portability gates and four semantic-blank schema witnesses', () => {
  assert.deepEqual(orientation.coverage.pd2OnlyHeaders, [
    'auratgtevent', 'auratgteventfunc', 'passiveevent', 'passiveeventfunc',
    'delay', 'checkfunc', 'nocostinstate', 'general',
  ]);
  assert.deepEqual(orientation.coverage.pd2OnlyUsed, ['delay', 'checkfunc', 'nocostinstate', 'general']);
  assert.deepEqual(orientation.coverage.pd2OnlySemanticBlank, [
    'auratgtevent', 'auratgteventfunc', 'passiveevent', 'passiveeventfunc',
  ]);

  for (const header of orientation.coverage.pd2OnlySemanticBlank) {
    const column = columns.get(header);
    assert.equal(column.usage.totalNonBlankCells, 0);
    assert.equal(column.primaryClassification, 'PD2_SCHEMA_UNUSED');
    assert.equal(column.decisionScope, 'NO_SKILL_DECISION');
    assert.equal(column.decisionRelevant, false);
  }
  for (const header of orientation.coverage.pd2OnlyUsed) {
    const column = columns.get(header);
    assert.ok(column.usage.totalNonBlankCells > 0);
    assert.ok(column.classifications.includes('NATIVE_EXTENSION_REQUIRED'));
    assert.equal(column.decisionScope, 'GLOBAL_POLICY');
    assert.equal(column.proofStatus, 'NATIVE_UNPROVEN');
  }
});

test('raw-state, delay, item economy, item trigger, five-tier curve and checkfunc witnesses are exact', () => {
  assert.deepEqual(RAW_VALUE_STATES, ['ABSENT_COLUMN', 'NULL_VALUE', 'EMPTY_STRING', 'ZERO', 'VALUE']);
  const auraevent4 = orientation.witnesses.auraevent4;
  assert.deepEqual([
    auraevent4.evidence.vanilla32.rawState,
    auraevent4.evidence.bkvince.rawState,
    auraevent4.evidence.pd2.rawState,
  ], ['EMPTY_STRING', 'EMPTY_STRING', 'ABSENT_COLUMN']);
  assert.equal(auraevent4.rawChanged, true);
  assert.equal(auraevent4.semanticChanged, false);
  assert.equal(auraevent4.decisionRelevant, false);

  assert.equal(orientation.witnesses.cooldowns.automaticMappingAllowed, false);
  assert.equal(orientation.witnesses.cooldowns.policy, 'NO_AUTOMATIC_DELAY_TRANSLATION');
  assert.equal(columns.get('delay').potentialEquivalent.proofStatus, 'NATIVE_UNPROVEN');
  assert.equal(columns.get('delay').potentialEquivalent.automaticMappingAllowed, false);
  assert.equal(orientation.witnesses.costAdd.skillBalanceDecision, false);
  assert.equal(columns.get('cost add').groupId, 'ITEM_ECONOMY');
  assert.equal(orientation.witnesses.itemTriggerExecution.bundleId, 'ITEM_TRIGGER_EXECUTION');
  assert.equal(columns.get('itemeffect').protected, true);
  assert.equal(columns.get('itemclteffect').protected, true);
  assert.equal(orientation.witnesses.elementalMaximumCurve.decisions, 1);
  for (let tier = 1; tier <= 5; tier += 1) {
    assert.equal(columns.get(`emaxlev${tier}`).groupId, 'ELEMENTAL_DAMAGE_CURVE');
  }
  assert.equal(orientation.witnesses.checkfunc.nonBlankPd2Cells, 38);
  assert.equal(orientation.witnesses.checkfunc.d2rSupportClaimed, false);
});

test('same-named callback columns prove schema alignment but never cross-engine behavior', () => {
  const callbacks = [
    'srvstfunc', 'srvdofunc', 'cltstfunc', 'cltdofunc', 'itemeffect', 'itemclteffect',
  ];
  for (const header of callbacks) {
    const equivalence = orientation.equivalences.proven.find((entry) => entry.sourceHeader === header);
    assert.ok(equivalence, `missing schema equivalence for ${header}`);
    assert.equal(equivalence.relation, 'SAME_CANONICAL_HEADER');
    assert.equal(equivalence.proofScope, 'SCHEMA_CONCEPT_ONLY');
    assert.equal(equivalence.valueSemanticsProven, false);
    assert.equal(equivalence.notProvenReference, 'CROSS_ENGINE_CALLBACK_NUMBERS');
  }
  assert.ok(orientation.equivalences.notProven.some((entry) => entry.sourceHeader === 'numeric callbacks'));
  assert.ok(orientation.unresolvedNativeQuestions.some((entry) => entry.id === 'CROSS_ENGINE_CALLBACK_NUMBERS'));
});

test('Fire Bolt accounting reduces 82 raw changes to six explicit behavior bundles without hiding a field', () => {
  const impact = orientation.fireBoltImpact;
  assert.equal(impact.currentModifiedFields, 82);
  assert.equal(impact.currentRequiredDecisions, 83);
  assert.equal(impact.finalPlayerDecisions, 6);
  assert.equal(impact.targetRange.met, true);
  assert.equal(impact.accounting.currentFields, 82);
  assert.equal(impact.accounting.accountedCurrentFields, 82);
  assert.deepEqual(impact.accounting.unaccountedCurrentFieldIds, []);
  assert.equal(impact.accounting.noRelevantDifferenceHidden, true);
  assert.deepEqual(impact.bundleProjection.map((bundle) => bundle.groupId), [
    'DAMAGE_SYNERGIES', 'ELEMENTAL_DAMAGE_CURVE', 'ITEM_TRIGGER_EXECUTION',
    'MANA_CURVE', 'NATIVE_EXECUTION', 'PROJECTILE_PHYSICS',
  ]);
  assert.equal(impact.reductions.semanticBlank.count, 61);
  assert.equal(impact.reductions.preserveD2rColumnAbsentFromPd2.count, 2);
  assert.equal(impact.reductions.vanillaHistoricalOnly.count, 1);
  assert.equal(impact.reductions.technicalOrDocumentary.count, 1);
  assert.equal(impact.reductions.bundled.count, 17);
});

test('all eight policies keep a pending example while the canonical 8/8 approval opens the skill gate', () => {
  assert.equal(orientation.policies.length, 8);
  assert.deepEqual(orientation.policies.map((policy) => policy.id), GLOBAL_SCHEMA_POLICIES.map((policy) => policy.id));
  assert.ok(orientation.policies.every((policy) => policy.decision === 'PENDING'));
  assert.ok(orientation.policies.every((policy) => policy.requiredForSkillCompletion === true));
  assert.deepEqual(policyExample.decisions, createPolicyEnvelope(orientation, {
    exportedAt: policyExample.exportedAt,
  }).decisions);
  assert.ok(Object.values(policyExample.decisions).every((decision) => decision.decision === 'PENDING'));
  assert.ok(Object.values(policyDocument.decisions).every((decision) => (
    decision.decision === 'APPROVE' && decision.justification.trim()
  )));

  const decisions = createEmptyEnvelope(report);
  const pendingGate = policyGate(orientation, policyExample);
  assert.equal(pendingGate.complete, false);
  assert.equal(pendingGate.required, 8);
  assert.equal(pendingGate.closed, 0);
  assert.equal(policyGate(orientation, decisions.schemaPolicy).complete, true);
  assert.equal(progress(report, decisions).schemaPolicyGate.complete, true);

  const mutableSkill = report.skills.find((skill) => !skill.readOnly);
  assert.ok(mutableSkill);
  const state = entryState(report, mutableSkill, decisions.entries[mutableSkill.stableId], policyExample);
  assert.equal(state.complete, false);
  assert.ok(state.reasons.some((reason) => reason.startsWith('Phase 0 policy gate:')));

  for (const policy of orientation.policies) {
    decisions.schemaPolicy.decisions[policy.id] = {
      fingerprint: policy.fingerprint,
      decision: 'APPROVE',
      justification: `Independent verification approves ${policy.id}.`,
    };
  }
  assert.equal(policyGate(orientation, decisions.schemaPolicy).complete, true);
});

test('integration derives every Phase 1 fingerprint from the legacy skill and preserves all governed Skills.txt bytes', () => {
  const sources = loadWorkbenchSources(DEFAULT_SOURCE_ROOTS);
  const base = buildOracleData(sources);
  assert.equal(report.skills.length, base.skills.length);
  const baseById = new Map(base.skills.map((skill) => [skill.stableId, skill]));
  for (const skill of report.skills) {
    assert.equal(skill.fingerprint, sha256Canonical({
      previousFingerprint: baseById.get(skill.stableId).fingerprint,
      decisionBundles: skill.decisionBundles,
      policyApplication: skill.policyApplication,
      curves: skill.curves,
    }), skill.stableId);
  }
  assert.deepEqual(gameplaySkillsPaths.map(sha256File), gameplayHashesBefore);
  assert.equal(path.basename(sources.sourceManifest.pd2.tables['skills.txt'].path), 'Skills.txt');
  assert.ok(ORIENTATION_NON_MUTATION_RULES.forbiddenWriteRoots.includes('data-BKVince/'));
  assert.ok(NON_MUTATION_RULES.forbiddenWriteRoots.includes('data-BKVince/'));
  assert.ok(NON_MUTATION_RULES.forbiddenCliFlags.includes('--apply'));
});

test('standalone and integrated UIs boot in real Chromium under file:// without network access', async (context) => {
  const browser = await chromium.launch({
    headless: true,
    args: ['--allow-file-access-from-files'],
  });
  context.after(async () => browser.close());
  const browserContext = await browser.newContext();
  const page = await browserContext.newPage();
  const errors = [];
  const requests = [];
  page.on('pageerror', (error) => errors.push(error.stack || error.message));
  page.on('console', (message) => {
    if (message.type() === 'error') errors.push(message.text());
  });
  page.on('request', (request) => requests.push(request.url()));

  const orientationUrl = pathToFileURL(ORIENTATION_HTML_PATH).href;
  const response = await page.goto(orientationUrl, { waitUntil: 'load', timeout: 20_000 });
  assert.equal(response?.status(), 200);
  assert.equal(new URL(page.url()).protocol, 'file:');
  const standalone = await page.evaluate(async () => {
    await globalThis.__PD2_SCHEMA_ORIENTATION_READY__;
    return {
      columns: globalThis.__PD2_SCHEMA_ORIENTATION_REPORT__?.columns?.length,
      policies: globalThis.__PD2_SCHEMA_ORIENTATION_REPORT__?.policies?.length,
      sections: [...document.querySelectorAll('.schema-section')].map((element) => element.id),
    };
  });
  assert.equal(standalone.columns, 330);
  assert.equal(standalone.policies, 8);
  for (const section of [
    'schema-overview', 'schema-source-only', 'schema-used-columns', 'schema-mechanical-contracts',
    'schema-field-dictionary', 'schema-global-policies', 'schema-fire-bolt-impact', 'schema-native-questions',
  ]) assert.ok(standalone.sections.includes(section), section);

  const firstPolicy = page.locator('.schema-policy').first();
  const policyId = await firstPolicy.getAttribute('data-policy-id');
  await firstPolicy.locator('[data-schema-policy-decision]').selectOption('APPROVE');
  await firstPolicy.locator('[data-schema-policy-property="justification"]').fill('Independent Chromium file audit.');
  await page.waitForFunction(({ prefix, policyId: id }) => {
    const key = Object.keys(localStorage).find((candidate) => candidate.startsWith(prefix));
    if (!key) return false;
    return JSON.parse(localStorage.getItem(key))?.decisions?.[id]?.justification === 'Independent Chromium file audit.';
  }, { prefix: POLICY_STORAGE_PREFIX, policyId });

  await page.reload({ waitUntil: 'load' });
  await page.evaluate(() => globalThis.__PD2_SCHEMA_ORIENTATION_READY__);
  assert.equal(
    await page.locator(`.schema-policy[data-policy-id="${policyId}"] [data-schema-policy-decision]`).inputValue(),
    'APPROVE',
  );

  const workbenchUrl = pathToFileURL(WORKBENCH_HTML_PATH).href;
  await page.goto(workbenchUrl, { waitUntil: 'load', timeout: 20_000 });
  const integrated = await page.evaluate(async () => {
    await globalThis.__PD2_SKILLS_WORKBENCH_READY__;
    return {
      activeView: document.querySelector('[data-view].active')?.dataset?.view,
      columns: globalThis.__PD2_SKILLS_REPORT__?.schemaOrientation?.columns?.length,
      policyCount: globalThis.__PD2_SKILLS_REPORT__?.schemaOrientation?.policies?.length,
    };
  });
  assert.deepEqual(integrated, { activeView: 'architecture', columns: 330, policyCount: 8 });
  assert.deepEqual(errors, []);
  assert.ok(requests.length >= 2);
  assert.ok(requests.every((url) => new URL(url).protocol === 'file:'), requests.join('\n'));

  for (const htmlPath of [ORIENTATION_HTML_PATH, WORKBENCH_HTML_PATH]) {
    const html = fs.readFileSync(htmlPath, 'utf8');
    assert.doesNotMatch(html, /<script[^>]+src=/iu);
    assert.doesNotMatch(html, /<link[^>]+(?:rel=["']?stylesheet|href=)/iu);
    assert.doesNotMatch(html, /\b(?:fetch|XMLHttpRequest|WebSocket|EventSource|sendBeacon)\s*\(/u);
  }
});
