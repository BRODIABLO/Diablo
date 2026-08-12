import assert from 'node:assert/strict';
import test from 'node:test';
import vm from 'node:vm';

import {
  buildSchemaOrientationApplicationSource,
  buildSchemaOrientationHtml,
} from './pd2-skills-schema-orientation-ui.mjs';

const policyRuntimeSource = `
globalThis.schemaPolicyRuntime = {
  storageKey(orientation) { return 'pd2-skills-schema-policy-v2:' + orientation.orientationHash; },
  createEmptyEnvelope(orientation) {
    return {
      schemaVersion: orientation.policySchemaVersion,
      kind: orientation.policyKind,
      orientationId: orientation.orientationId,
      orientationHash: orientation.orientationHash,
      frozenContractHash: orientation.frozenContractHash,
      sourceHashes: orientation.sourceHashes,
      exportedAt: null,
      decisions: {},
    };
  },
  validateImport(orientation, payload) {
    return payload.orientationHash === orientation.orientationHash
      ? { valid: true, envelope: payload }
      : { valid: false, errors: ['orientationHash périmé'] };
  },
  exportEnvelope(orientation, envelope) { return { ...envelope, exportedAt: '2026-08-12T00:00:00.000Z' }; },
};`;

function mockOrientation() {
  const policies = [
    {
      id: 'PRESERVE_ALL_D2R_BKVINCE_COLUMNS',
      fingerprint: 'POLICY-KEEP-FP',
      titleFr: 'Préserver toutes les colonnes D2R/BKVince',
      statementFr: 'Le schéma cible conserve toutes les colonnes Vanilla et BKVince.',
      proposedDecision: 'APPROVE',
      requiredForSkillCompletion: true,
    },
    {
      id: 'NO_AUTOMATIC_DELAY_TRANSLATION',
      fingerprint: 'POLICY-DELAY-FP',
      titleFr: 'Ne jamais traduire delay automatiquement',
      statementFr: 'delay, localdelay et globaldelay restent des modèles distincts.',
      proposedDecision: 'APPROVE',
      requiredForSkillCompletion: true,
    },
  ];
  const columns = [
    {
      id: 'delay', canonicalHeader: 'delay', rawHeaders: { pd2: 'delay' },
      presence: { vanilla32: false, bkvince: false, pd2: true },
      usage: {
        vanilla32: { nonEmptyCells: 0, playerSkills: 0, technicalOrMonsterLines: 0 },
        bkvince: { nonEmptyCells: 0, playerSkills: 0, technicalOrMonsterLines: 0 },
        pd2: { nonEmptyCells: 12, playerSkills: 10, technicalOrMonsterLines: 2 },
        totalNonEmptyCells: 12, playerSkills: 10, technicalOrMonsterLines: 2,
      },
      examples: {
        vanilla32: [{ skill: 'Fire Wall', rawState: 'ABSENT_COLUMN', semanticBlank: true }],
        bkvince: [{ skill: 'Fire Wall', rawState: 'ABSENT_COLUMN', semanticBlank: true }],
        pd2: [{ skill: 'Fire Wall', rawValue: '25', rawState: 'VALUE', semanticBlank: false }],
      },
      documentation: { description: 'Délai du modèle PD2.' },
      playerLabelFr: 'Délai PD2', shortHelpFr: 'Aucune traduction automatique vers D2R.', family: 'timing',
      consumer: 'Consumer PD2; consumer D2R inconnu', classifications: ['PD2_SEMANTIC_SOURCE_ONLY'],
      primaryClassification: 'PD2_SEMANTIC_SOURCE_ONLY', potentialEquivalent: 'localdelay/globaldelay, non prouvé',
      decisionScope: 'GLOBAL_POLICY', defaultPolicy: 'NO_AUTOMATIC_DELAY_TRANSLATION', technicalOnly: false,
      protected: true, groupId: 'COOLDOWN_MODEL', proofStatus: 'NATIVE_UNPROVEN',
    },
    {
      id: 'auraevent4', canonicalHeader: 'auraevent4', rawHeaders: { vanilla32: 'auraevent4', bkvince: 'auraevent4' },
      presence: { vanilla32: true, bkvince: true, pd2: false },
      usage: {
        vanilla32: { nonEmptyCells: 0 }, bkvince: { nonEmptyCells: 0 }, pd2: { nonEmptyCells: 0 }, totalNonEmptyCells: 0,
      },
      examples: {
        vanilla32: [{ skill: 'Fire Bolt', rawValue: '', rawState: 'EMPTY_STRING', semanticBlank: true }],
        bkvince: [{ skill: 'Fire Bolt', rawValue: '', rawState: 'EMPTY_STRING', semanticBlank: true }],
        pd2: [{ skill: 'Fire Bolt', rawState: 'ABSENT_COLUMN', semanticBlank: true }],
      },
      documentation: { description: 'Quatrième événement d’aura D2R.' }, playerLabelFr: 'Événement d’aura 4',
      shortHelpFr: 'Champ brut sans décision lorsque toutes les sources sont semanticBlank.', family: 'auras_passives',
      consumer: 'Callback natif d’aura', classifications: ['D2R_NATIVE_PRESERVE'], primaryClassification: 'D2R_NATIVE_PRESERVE',
      decisionScope: 'NO_SKILL_DECISION', defaultPolicy: 'SEMANTIC_BLANKS_REQUIRE_NO_DECISION', technicalOnly: true,
      protected: true, groupId: 'PASSIVE_PACKAGE', proofStatus: 'EXACT_TABLE',
    },
    {
      id: 'cost add', canonicalHeader: 'cost add', rawHeaders: { vanilla32: 'cost add', bkvince: 'cost add', pd2: 'cost add' },
      presence: { vanilla32: true, bkvince: true, pd2: true },
      usage: {
        vanilla32: { nonEmptyCells: 3 }, bkvince: { nonEmptyCells: 3 }, pd2: { nonEmptyCells: 3 }, totalNonEmptyCells: 9,
      },
      examples: { bkvince: [{ skill: 'Fire Bolt', rawValue: '0', rawState: 'ZERO', semanticBlank: false }] },
      documentation: { description: 'Ajout plat à la valeur en or des objets.' }, playerLabelFr: 'Influence sur la valeur en or',
      shortHelpFr: 'Sans rapport avec le mana ou l’apprentissage.', family: 'item_economy', consumer: 'Économie des objets',
      classifications: ['ITEM_ECONOMY_ONLY'], primaryClassification: 'ITEM_ECONOMY_ONLY', decisionScope: 'NO_SKILL_DECISION',
      defaultPolicy: 'ITEM_ECONOMY_FIELDS_PRESERVE_BKVINCE', technicalOnly: true, protected: false, groupId: 'ITEM_ECONOMY', proofStatus: 'EXACT_TABLE',
    },
    {
      id: 'checkfunc', canonicalHeader: 'checkfunc', rawHeaders: { pd2: 'checkfunc' },
      presence: { vanilla32: false, bkvince: false, pd2: true },
      usage: { vanilla32: { nonEmptyCells: 0 }, bkvince: { nonEmptyCells: 0 }, pd2: { nonEmptyCells: 4, playerSkills: 4 }, totalNonEmptyCells: 4, playerSkills: 4 },
      examples: { pd2: [{ skill: 'Combustion', rawValue: '3', rawState: 'VALUE', semanticBlank: false }] },
      documentation: { description: 'Callback conditionnel propre au schéma PD2.' }, playerLabelFr: 'Contrôle natif PD2',
      shortHelpFr: 'Support D2R non prouvé.', family: 'native_execution', consumer: 'UNKNOWN_NATIVE_CONSUMER',
      classifications: ['NATIVE_EXTENSION_REQUIRED', 'UNKNOWN_NATIVE_CONSUMER'], primaryClassification: 'NATIVE_EXTENSION_REQUIRED',
      decisionScope: 'GLOBAL_POLICY', defaultPolicy: 'NO_PD2_HEADER_IMPORT_WITHOUT_NATIVE_PROOF', technicalOnly: true,
      protected: true, groupId: 'NATIVE_EXECUTION', proofStatus: 'NATIVE_UNPROVEN',
    },
  ];
  return {
    schemaVersion: 1,
    orientationId: 'pd2-skills-schema-orientation-v1',
    productName: 'PD2 Skills Schema and Engine Orientation',
    state: 'POLICY_REVIEW_ONLY',
    frozenContractHash: '3133A16AD42315181599DBB5DE29C4C7DAEBC5DFB7F1110639C2CEBFEA13EC6B',
    orientationHash: 'ORIENTATION-HASH-123',
    policySchemaVersion: 2,
    policyKind: 'pd2-skills-schema-policy',
    sourceHashes: { vanilla32: 'VANILLA', bkvince: 'BKV', pd2: 'PD2' },
    columns,
    policies,
    mechanicalContracts: [
      {
        id: 'cooldowns', titleFr: 'Cooldowns', fields: ['delay', 'localdelay', 'globaldelay', 'perdelay'],
        consumerFr: 'Cadence et délais de relance.', provenRelations: ['localdelay est un délai D2R documenté.'],
        hypotheses: ['Aucune équivalence de delay PD2 avec D2R.'], translationPolicy: 'NO_AUTOMATIC_DELAY_TRANSLATION', proofStatus: 'NATIVE_UNPROVEN',
      },
      {
        id: 'mana', titleFr: 'Mana', fields: ['mana', 'lvlmana', 'manashift', 'minmana', 'startmana'],
        consumerFr: 'Coût réel en mana.', provenRelations: ['Les cinq champs forment une seule courbe.'], hypotheses: [],
        translationPolicy: 'RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE', proofStatus: 'EXACT_FORMULA',
      },
    ],
    fireBoltImpact: {
      currentModifiedFields: 82,
      currentRequiredDecisions: 83,
      bundleCount: 6,
      finalPlayerDecisions: 6,
      semanticBlankFields: ['auraevent4', 'auratgtevent'],
      technicalOrDocumentaryFields: ['cost add', 'cost mult'],
      bundles: ['ELEMENTAL_DAMAGE_CURVE', 'MANA_CURVE', 'PROJECTILE_ARCHITECTURE', 'PROJECTILE_PHYSICS', 'COOLDOWN_MODEL', 'UI_ASSIGNMENT'],
      reductions: [
        { reason: 'auraevent4 vide/vide/absent reste dans la preuve brute et ne demande aucune décision.' },
        { reason: 'emaxlev1 à emaxlev5 deviennent une seule décision de courbe.' },
      ],
    },
    witnesses: {
      auraevent4: { titleFr: 'auraevent4 vide/vide/absent', summary: 'Preuve brute conservée; aucune décision.' },
      fireBolt: { titleFr: 'Fire Bolt', summary: '83 exigences actuelles deviennent 6 décisions joueur.' },
    },
    unresolvedNativeQuestions: [
      { id: 'checkfunc-d2r', titleFr: 'D2R consomme-t-il checkfunc ?', question: 'Aucune preuve native D2R 3.2 disponible.', proofStatus: 'NATIVE_UNPROVEN' },
    ],
  };
}

function embeddedScript(html) {
  const match = html.match(/<script>([\s\S]*)<\/script>/);
  assert.ok(match, 'one embedded script should exist');
  return match[1];
}

function browserContext(storageMap = new Map()) {
  const listeners = {};
  const root = {
    innerHTML: '',
    addEventListener(type, listener) { listeners[type] = listener; },
    querySelector() { return null; },
  };
  const context = {
    console,
    Blob,
    Response,
    DecompressionStream,
    Uint8Array,
    atob,
    URL,
    document: {
      body: root,
      querySelector(selector) { return selector === '#schema-orientation' ? root : null; },
      createElement() { return { click() {} }; },
    },
    localStorage: {
      getItem(key) { return storageMap.get(key) ?? null; },
      setItem(key, value) { storageMap.set(key, value); },
      removeItem(key) { storageMap.delete(key); },
    },
    setTimeout(callback) { callback(); return 1; },
    clearTimeout() {},
    confirm() { return true; },
  };
  context.window = context;
  return { context, root, listeners, storageMap };
}

test('builds a deterministic standalone file:// Phase 0 document without network dependencies', () => {
  const orientation = mockOrientation();
  const first = buildSchemaOrientationHtml(orientation, { policyRuntimeSource, workbenchBinding: { reviewId: 'pd2-skills-review-v2' } });
  const second = buildSchemaOrientationHtml(orientation, { policyRuntimeSource, workbenchBinding: { reviewId: 'pd2-skills-review-v2' } });
  assert.equal(first, second);
  assert.match(first, /^<!doctype html>/);
  assert.match(first, /PD2 Skills Schema and Engine Orientation/);
  assert.match(first, /Phase analytique uniquement/);
  assert.doesNotMatch(first, /<script[^>]+src=/i);
  assert.doesNotMatch(first, /<link[^>]+stylesheet/i);
  assert.doesNotMatch(embeddedScript(first), /\bfetch\s*\(|XMLHttpRequest|import\s*\(/);
  assert.doesNotMatch(first, /cdn\./i);
  assert.doesNotThrow(() => new vm.Script(embeddedScript(first), { filename: 'pd2-skills-schema-orientation.html' }));
});

test('boots the structured Phase 0 DOM with all required sections and no skill card as landing view', async () => {
  const html = buildSchemaOrientationHtml(mockOrientation(), { policyRuntimeSource });
  const { context, root, listeners } = browserContext();
  vm.runInNewContext(embeddedScript(html), context, { filename: 'pd2-skills-schema-orientation.html' });
  await context.__PD2_SCHEMA_ORIENTATION_READY__;
  for (const sectionId of [
    'schema-overview', 'schema-source-only', 'schema-used-columns', 'schema-mechanical-contracts',
    'schema-field-dictionary', 'schema-global-policies', 'schema-fire-bolt-impact', 'schema-native-questions',
  ]) assert.match(root.innerHTML, new RegExp('id="' + sectionId + '"'));
  assert.match(root.innerHTML, /Vanilla D2R 3\.2/);
  assert.match(root.innerHTML, /BKVince HEAD/);
  assert.match(root.innerHTML, /PD2 \/ SP\+ épinglé/);
  assert.match(root.innerHTML, /Recherche.*filtres des colonnes|Rechercher une colonne ou un concept/s);
  assert.match(root.innerHTML, /82[\s\S]*Champs bruts modifiés/);
  assert.match(root.innerHTML, /83[\s\S]*Décisions actuelles/);
  assert.match(root.innerHTML, /6[\s\S]*Décisions joueur finales/);
  assert.match(root.innerHTML, /auraevent4 vide\/vide\/absent/);
  assert.match(root.innerHTML, /NATIVE_UNPROVEN/);
  assert.doesNotMatch(root.innerHTML, /class="skill-card/);
  assert.equal(typeof listeners.click, 'function');
  assert.equal(typeof listeners.change, 'function');
  assert.equal(typeof listeners.input, 'function');
});

test('preserves absent, empty, zero and value evidence and filters the column matrix', async () => {
  const html = buildSchemaOrientationHtml(mockOrientation(), { policyRuntimeSource });
  const { context, root } = browserContext();
  vm.runInNewContext(embeddedScript(html), context);
  await context.__PD2_SCHEMA_ORIENTATION_READY__;
  const controller = context.__PD2_SCHEMA_ORIENTATION_CONTROLLER__;
  const toggle = { dataset: { schemaAction: 'toggle-column', schemaColumnId: 'auraevent4' } };
  controller.handleClick({ target: { closest() { return toggle; } } });
  assert.match(root.innerHTML, /ABSENT_COLUMN/);
  assert.match(root.innerHTML, /EMPTY_STRING/);
  assert.match(root.innerHTML, /semanticBlank/);
  controller.handleClick({ target: { closest() { return { dataset: { schemaAction: 'toggle-column', schemaColumnId: 'cost add' } }; } } });
  assert.match(root.innerHTML, /ZERO/);
  assert.match(root.innerHTML, /Sans rapport avec le mana ou l’apprentissage/);
  controller.handleInput({ target: { value: 'checkfunc', matches(selector) { return selector === '[data-schema-search]'; } } });
  assert.match(root.innerHTML, /1 \/ 4 headers/);
  assert.match(root.innerHTML, /Contrôle natif PD2/);
});

test('keeps policies PENDING until explicit governed decisions and preserves policy fingerprints', async () => {
  const orientation = mockOrientation();
  const html = buildSchemaOrientationHtml(orientation, { policyRuntimeSource });
  const { context, root, storageMap } = browserContext();
  vm.runInNewContext(embeddedScript(html), context);
  await context.__PD2_SCHEMA_ORIENTATION_READY__;
  const controller = context.__PD2_SCHEMA_ORIENTATION_CONTROLLER__;
  assert.deepEqual(JSON.parse(JSON.stringify(controller.gateState())), { required: 2, complete: 0, remaining: 2, closed: false });
  assert.match(root.innerHTML, /Orientation proposée:.*APPROVE|Orientation proposée :.*APPROVE/s);
  assert.match(root.innerHTML, /état réel : <strong>PENDING/);

  controller.handleChange({ target: {
    value: 'APPROVE', dataset: { policyId: orientation.policies[0].id },
    matches(selector) { return selector === '[data-schema-policy-decision]'; },
  } });
  assert.equal(controller.gateState().complete, 0, 'APPROVE without justification must leave the gate open');
  controller.handleInput({ target: {
    value: 'Le schéma D2R/BKVince reste la cible.',
    dataset: { policyId: orientation.policies[0].id, schemaPolicyProperty: 'justification' },
    matches(selector) { return selector === '[data-schema-policy-property]'; },
  } });
  assert.equal(controller.gateState().complete, 1);
  assert.equal(controller.getEnvelope().decisions[orientation.policies[0].id].fingerprint, 'POLICY-KEEP-FP');
  const stored = JSON.parse(storageMap.get('pd2-skills-schema-policy-v2:' + orientation.orientationHash));
  assert.equal(stored.decisions[orientation.policies[0].id].fingerprint, 'POLICY-KEEP-FP');

  controller.handleChange({ target: {
    value: 'MODIFY', dataset: { policyId: orientation.policies[1].id },
    matches(selector) { return selector === '[data-schema-policy-decision]'; },
  } });
  controller.handleInput({ target: {
    value: 'Traduction étudiée manuellement.', dataset: { policyId: orientation.policies[1].id, schemaPolicyProperty: 'justification' },
    matches(selector) { return selector === '[data-schema-policy-property]'; },
  } });
  assert.equal(controller.gateState().complete, 1, 'MODIFY also requires an explicit customPolicy');
  controller.handleInput({ target: {
    value: 'Aucune traduction sans preuve et revue du bundle COOLDOWN_MODEL.', dataset: { policyId: orientation.policies[1].id, schemaPolicyProperty: 'customPolicy' },
    matches(selector) { return selector === '[data-schema-policy-property]'; },
  } });
  assert.deepEqual(JSON.parse(JSON.stringify(controller.gateState())), { required: 2, complete: 2, remaining: 0, closed: true });
});

test('selects the meaningful newest policy envelope and synchronizes it with the Workbench callback', () => {
  const orientation = mockOrientation();
  const { context, storageMap } = browserContext();
  vm.runInNewContext(policyRuntimeSource, context);
  vm.runInNewContext(buildSchemaOrientationApplicationSource(), context);
  const base = {
    schemaVersion: orientation.policySchemaVersion,
    kind: orientation.policyKind,
    orientationId: orientation.orientationId,
    orientationHash: orientation.orientationHash,
    frozenContractHash: orientation.frozenContractHash,
    sourceHashes: orientation.sourceHashes,
  };
  const pending = {
    ...base,
    exportedAt: '2026-08-12T12:00:00.000Z',
    decisions: Object.fromEntries(orientation.policies.map((policy) => [policy.id, {
      fingerprint: policy.fingerprint, decision: 'PENDING', justification: '',
    }])),
  };
  const locallyApproved = {
    ...base,
    exportedAt: '2026-08-12T11:00:00.000Z',
    decisions: {
      ...pending.decisions,
      [orientation.policies[0].id]: {
        fingerprint: orientation.policies[0].fingerprint,
        decision: 'APPROVE',
        justification: 'Décision explicite enregistrée dans le standalone.',
      },
    },
  };
  const key = context.schemaPolicyRuntime.storageKey(orientation);
  storageMap.set(key, JSON.stringify(locallyApproved));
  const synchronized = [];
  const controller = context.schemaOrientationUI.createController(orientation, {
    policyRuntime: context.schemaPolicyRuntime,
    storage: context.localStorage,
    initialEnvelope: pending,
    onPolicyChange(envelope) { synchronized.push(envelope); },
  });
  assert.equal(controller.getEnvelope().decisions[orientation.policies[0].id].decision, 'APPROVE', 'a generated all-PENDING envelope must not erase meaningful local policy work');
  assert.equal(synchronized.at(-1).decisions[orientation.policies[0].id].decision, 'APPROVE');

  const newerWorkbench = {
    ...locallyApproved,
    exportedAt: '2026-08-12T13:00:00.000Z',
    decisions: {
      ...locallyApproved.decisions,
      [orientation.policies[1].id]: {
        fingerprint: orientation.policies[1].fingerprint,
        decision: 'APPROVE',
        justification: 'Deuxième politique décidée depuis le Workbench.',
      },
    },
  };
  controller.setEnvelope(newerWorkbench, { persist: false, render: false });
  assert.equal(controller.gateState().complete, 2);
  assert.equal(synchronized.at(-1).exportedAt, newerWorkbench.exportedAt);
});

test('exports a reusable browser application source and rejects malformed builder inputs', () => {
  assert.doesNotThrow(() => new vm.Script(buildSchemaOrientationApplicationSource()));
  assert.throws(() => buildSchemaOrientationHtml(null), /orientation/);
  assert.throws(() => buildSchemaOrientationHtml({}), /orientation\.columns/);
  assert.throws(() => buildSchemaOrientationHtml({ columns: [] }), /orientationId/);
  assert.throws(() => buildSchemaOrientationHtml({ columns: [], orientationId: 'x' }), /orientationHash/);
});
