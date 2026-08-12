import assert from 'node:assert/strict';
import test from 'node:test';
import vm from 'node:vm';
import zlib from 'node:zlib';

import { buildSkillReviewHtml } from './pd2-skills-review-ui.mjs';
import { buildBrowserRuntimeSource } from './pd2-skills-review-runtime.mjs';

const runtimeSource = `
globalThis.decisionRuntime = {
  constants: {},
  createEmptyEnvelope(report) { return { schemaVersion: 1, reviewId: report.reviewId, comparisonHash: report.comparisonHash, entries: {} }; },
  createEntry(skill) { return { fingerprint: skill.fingerprint, implementationStatus: 'NOT_REVIEWED', componentDecisions: {}, fieldDecisions: {}, notes: {} }; },
  entryState(report, skill, entry) { return { required: !skill.readOnly, complete: Boolean(entry.globalDecision), reasons: entry.globalDecision ? [] : ['Décision globale manquante'] }; },
  applyBulk(report, ids, entries) { return { ...entries }; },
  validateImport(report, payload) { return payload; },
  migrateEnvelope(report, payload) { return { envelope: payload, report: { retained: [], stale: [], dropped: [] } }; },
  exportEnvelope(report, entries, options) { return { reviewId: report.reviewId, comparisonHash: report.comparisonHash, exportScope: options.scope, entries }; },
  storageKey(report) { return 'pd2-skills-review-decisions-v1:' + report.comparisonHash; },
};`;

function mockReport(overrides = {}) {
  const skill = {
    stableId: 'skill:sor:fire-ball',
    fingerprint: 'FIREBALL-FP',
    canonicalName: 'Fire Ball',
    aliases: ['Boule de feu'],
    names: { vanilla32: 'Fire Ball', bkvince: 'Fire Ball', pd2: 'Fire Ball' },
    classCode: 'sor',
    scope: 'sor',
    playerSkill: true,
    newPd2PlayerSkill: false,
    bkvinceOnlyPlayerSkill: false,
    nodeIds: { vanilla32: 'vanilla32:skills.txt:47', bkvince: 'bkvince:skills.txt:47', pd2: 'pd2:skills.txt:47' },
    ordinals: { vanilla32: 47, bkvince: 47, pd2: 47 },
    tree: { id: 'fire', label: 'Fire Spells', page: 1, row: 4, column: 2 },
    mappingTypes: ['SAME_SKILL_SAME_ORDINAL'],
    primaryMappingType: 'SAME_SKILL_SAME_ORDINAL',
    identical: false,
    readOnly: false,
    status: 'MODIFIED',
    collisionIds: ['collision:47'],
    summary: { player: 'Davantage de projectiles; formule multishot malformée conservée.' },
    evidence: {
      overall: 'MALFORMED_SOURCE',
      statuses: ['EXACT_FORMULA', 'MALFORMED_SOURCE', 'EXACT_TABLE', 'NATIVE_UNPROVEN'],
    },
    portability: {
      categories: ['NATIVE_UNPROVEN'], reasons: ['Fonction native différente'], tables: ['skills.txt', 'missiles.txt'],
      missingDependencies: ['missile:test'], requiredProof: ['callback D2R 3.2'], effort: 'élevé',
    },
    components: [{
      id: 'damage_model', label: 'Modèle de dégâts', fingerprint: 'CMP', proofStatus: 'MALFORMED_SOURCE', portability: ['NATIVE_UNPROVEN'], changed: true,
      fields: [{
        id: 'skills.txt:calc1', table: 'skills.txt', header: 'calc1', label: 'Dégâts feu',
        values: { vanilla32: '10', bkvince: '12', pd2: '20' }, displayValues: { vanilla32: '10', bkvince: '12', pd2: '20' },
        changed: true, protected: true, protectionReasons: ['Formule malformée'], proofStatus: 'MALFORMED_SOURCE',
        formula: { raw: '(skill("Fire Bolt".blvl)*par8', status: 'MALFORMED_SOURCE' }, dependencyIds: ['skill:fire-bolt'],
      }],
    }],
    curves: { scenarios: [{ id: 'standard', label: 'Sans synergie', levels: [1, 5, 10, 20, 30, 40], metrics: { damage: { label: 'Dégâts moyens', values: { vanilla32: [8, 12, 20, 40, 70, 100], bkvince: [9, 14, 24, 45, 75, 110], pd2: [10, 16, 28, 50, 80, 120] } } } }] },
    dependencies: [{ id: 'skill:fire-bolt', label: 'Fire Bolt', closed: true, provenance: 'skills.txt' }],
    consumers: [{ type: 'missile', name: 'fireball', table: 'missiles.txt' }],
    documentation: [{ section: 'Sorceress — Fire Ball', revision: '23785', season: '13', summary: 'Multishot documenté.', status: 'DOCUMENTED', url: 'https://wiki.projectdiablo2.com/wiki/Skills' }],
  };
  return {
    schemaVersion: 1,
    reviewId: 'pd2-skills-review-v1',
    productName: 'PD2 Skills Merge Workbench',
    state: 'REVIEW_ONLY',
    frozenContractHash: 'CONTRACT',
    comparisonHash: 'ABCDEF0123456789',
    sourceManifest: {},
    sourceHashes: { bkvince: 'BKV', pd2: 'PD2', vanilla32: 'VANILLA' },
    levels: [1, 5, 10, 20, 30, 40],
    enums: {
      mappingTypes: ['SAME_SKILL_SAME_ORDINAL', 'PD2_ONLY_PLAYER_SKILL'],
      proofStatuses: ['EXACT_TABLE', 'MALFORMED_SOURCE', 'NATIVE_UNPROVEN'],
      portabilityCategories: ['DATA_ONLY_PROVEN', 'APPEND_ONLY_REQUIRED', 'NATIVE_UNPROVEN'],
      globalDecisions: ['KEEP_BKVINCE', 'ADAPT_PD2_SELECTIVELY', 'DISCUSS'],
      componentDecisions: ['KEEP_BKVINCE', 'ADOPT_PD2', 'CUSTOM', 'DISCUSS', 'NOT_APPLICABLE'],
      newSkillLineDecisions: ['IMPORT_APPEND_ONLY', 'IMPORT_CUSTOMIZED', 'REJECT_PD2_SKILL', 'DEFER_NATIVE_PROOF', 'DISCUSS'],
      implementationStatuses: ['NOT_REVIEWED', 'DECISION_COMPLETE', 'SELECTED_FOR_PROTOTYPE', 'IMPLEMENTATION_NOT_AUTHORIZED'],
    },
    coverage: {
      historicalBaseline: { bkvinceRows: 449, collisions: 108 },
      currentBaseline: { bkvinceRows: 451, collisions: 110 },
      nextAppendOrdinal: 451,
    },
    navigation: [{ id: 'sor', label: 'Sorceress', classCode: 'sor', trees: [{ id: 'fire', label: 'Fire Spells', skillIds: [skill.stableId] }], skillIds: [skill.stableId] }],
    nodes: [
      { id: 'vanilla32:skills.txt:47', source: 'vanilla32', ordinal: 47, name: 'Fire Ball', raw: { skill: 'Fire Ball' } },
      { id: 'bkvince:skills.txt:47', source: 'bkvince', ordinal: 47, name: 'Fire Ball', raw: { skill: 'Fire Ball' } },
      { id: 'pd2:skills.txt:47', source: 'pd2', ordinal: 47, name: 'Fire Ball', raw: { skill: 'Fire Ball' } },
    ],
    skills: [skill],
    collisions: [{ id: 'collision:47', type: 'SAME_ORDINAL_DIFFERENT_SKILL', summary: 'Ordinal 47 occupé différemment.' }],
    documentation: { revision: '23785', season: '13' },
    ...overrides,
  };
}

function mockSchemaOrientation() {
  return {
    schemaVersion: 1,
    orientationId: 'pd2-skills-schema-orientation-v1',
    productName: 'PD2 Skills Schema and Engine Orientation',
    orientationHash: 'SCHEMA-ORIENTATION-HASH',
    frozenContractHash: 'SCHEMA-CONTRACT',
    policySchemaVersion: 2,
    policyKind: 'pd2-skills-schema-policy',
    sourceHashes: { vanilla32: 'VANILLA', bkvince: 'BKV', pd2: 'PD2' },
    columns: [{
      id: 'delay', canonicalHeader: 'delay', rawHeaders: { pd2: 'delay' },
      presence: { vanilla32: false, bkvince: false, pd2: true },
      usage: { vanilla32: { nonEmptyCells: 0 }, bkvince: { nonEmptyCells: 0 }, pd2: { nonEmptyCells: 12, playerSkills: 10 }, totalNonEmptyCells: 12, playerSkills: 10 },
      examples: { pd2: [{ skill: 'Fire Wall', rawValue: '25', rawState: 'VALUE' }] },
      playerLabelFr: 'Délai PD2', shortHelpFr: 'Aucune traduction automatique.', family: 'timing',
      classifications: ['PD2_SEMANTIC_SOURCE_ONLY'], primaryClassification: 'PD2_SEMANTIC_SOURCE_ONLY',
      decisionScope: 'GLOBAL_POLICY', defaultPolicy: 'NO_AUTOMATIC_DELAY_TRANSLATION', protected: true,
      groupId: 'COOLDOWN_MODEL', proofStatus: 'NATIVE_UNPROVEN',
    }],
    mechanicalContracts: [{
      id: 'cooldowns', titleFr: 'Cooldowns', fields: ['delay', 'localdelay', 'globaldelay'],
      consumerFr: 'Cadence des skills.', provenRelations: [], hypotheses: ['Équivalence non prouvée.'],
      translationPolicy: 'NO_AUTOMATIC_DELAY_TRANSLATION', proofStatus: 'NATIVE_UNPROVEN',
    }],
    policies: [{
      id: 'NO_AUTOMATIC_DELAY_TRANSLATION', fingerprint: 'DELAY-POLICY-FP',
      titleFr: 'Ne jamais traduire delay automatiquement', statementFr: 'Les modèles restent distincts.',
      proposedDecision: 'APPROVE', requiredForSkillCompletion: true,
    }],
    fireBoltImpact: { currentModifiedFields: 82, currentRequiredDecisions: 83, bundleCount: 6, finalPlayerDecisions: 6 },
    unresolvedNativeQuestions: [{ id: 'delay', question: 'Le consumer PD2 delay n’est pas prouvé dans D2R.', proofStatus: 'NATIVE_UNPROVEN' }],
  };
}

function embeddedScript(html) {
  const match = html.match(/<script>([\s\S]*)<\/script>/);
  assert.ok(match, 'one embedded application script should exist');
  return match[1];
}

function buildHtml(report, source = runtimeSource) {
  const compressedOracleBase64 = zlib.gzipSync(Buffer.from(JSON.stringify(report)), { level: 9, mtime: 0 }).toString('base64');
  return buildSkillReviewHtml(report, source, { compressedOracleBase64 });
}

test('builds one standalone file:// compatible document with the complete workbench controls', () => {
  const html = buildHtml(mockReport());
  assert.match(html, /^<!doctype html>/);
  assert.match(html, /PD2 Skills Merge Workbench/);
  assert.match(html, /Recherche globale/);
  assert.match(html, /Décisions incomplètes seulement/);
  assert.match(html, /Différences significatives seulement/);
  assert.match(html, /Portabilité/);
  assert.match(html, /Type de mapping/);
  assert.match(html, /Statut de preuve/);
  assert.match(html, /Décision finale/);
  assert.match(html, /Risque natif/);
  assert.match(html, /Nouveau skill PD2/);
  assert.match(html, /Vue joueur/);
  assert.match(html, /Vue technique/);
  assert.match(html, /Skill suivant à décider/);
  assert.match(html, /Développer tout/);
  assert.match(html, /Réduire tout/);
  assert.match(html, /Exporter le dossier de révision de cette classe/);
  assert.match(html, /Copier le briefing du skill/);
  assert.match(html, /Exporter ce skill en Markdown/);
  assert.match(html, /currentOccupantAtPd2Ordinal/);
  assert.match(html, /testsRequired/);
  assert.match(html, /pd2-skills-review-decisions-v1:/);
  assert.match(html, /file:\/\//, 'file protocol is explicitly supported by the document contract');
});

test('embeds accessible local SVG curves with underlying tables and tri-way labels', () => {
  const html = buildHtml(mockReport());
  assert.match(html, /<svg viewBox=/);
  assert.match(html, /role=\\?"img\\?"/);
  assert.match(html, /<desc id=/);
  assert.match(html, /Données accessibles/);
  assert.match(html, /Vanilla D2R 3\.2/);
  assert.match(html, /BKVince actuel/);
  assert.match(html, /Project Diablo 2/);
  assert.match(html, /Niveau effectif L/);
});

test('contains no external script, stylesheet, module, fetch, XHR, CDN or network loader', () => {
  const html = buildHtml(mockReport());
  assert.doesNotMatch(html, /<script[^>]+src=/i);
  assert.doesNotMatch(html, /<link[^>]+rel=["']?stylesheet/i);
  assert.doesNotMatch(html, /type=["']module["']/i);
  assert.doesNotMatch(embeddedScript(html), /\bfetch\s*\(|XMLHttpRequest|import\s*\(/);
  assert.doesNotMatch(html, /cdn\./i);
});

test('embedded application JavaScript compiles as a classic script', () => {
  const html = buildHtml(mockReport());
  assert.doesNotThrow(() => new vm.Script(embeddedScript(html), { filename: 'pd2-skills-review.html' }));
});

test('boots asynchronously from the deterministic local gzip oracle and renders governed content', async () => {
  const html = buildHtml(mockReport());
  const listeners = {};
  const root = {
    innerHTML: '',
    addEventListener(type, listener) { listeners[type] = listener; },
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
      body: { innerHTML: '' },
      querySelector(selector) { return selector === '#workbench' ? root : null; },
    },
    localStorage: {
      getItem() { return null; },
      setItem() {},
      removeItem() {},
    },
    navigator: {},
    setTimeout() { return 1; },
    clearTimeout() {},
    confirm() { return true; },
  };
  context.window = context;
  vm.runInNewContext(embeddedScript(html), context, { filename: 'pd2-skills-review.html' });
  await context.__PD2_SKILLS_WORKBENCH_READY__;
  assert.deepEqual(JSON.parse(JSON.stringify(context.__PD2_SKILLS_REPORT__)), mockReport());
  assert.equal(context.__PD2_SKILLS_ORACLE_GZIP_BASE64__, undefined, 'compressed transport is released after bootstrap');
  assert.match(root.innerHTML, /Fire Ball/);
  assert.match(root.innerHTML, /Sorceress/);
  assert.match(root.innerHTML, /Baseline courante \(HEAD\).*451 lignes BKVince.*110 collisions.*prochain ordinal 451/s);
  assert.match(root.innerHTML, /Audit historique du 8 août.*449 lignes.*108 collisions.*prochain ordinal 449/s);
  for (const proof of ['EXACT_FORMULA', 'MALFORMED_SOURCE', 'EXACT_TABLE', 'NATIVE_UNPROVEN']) {
    assert.match(root.innerHTML, new RegExp(proof));
  }
  assert.match(root.innerHTML, /Décision globale de Vincent/);
  assert.equal(typeof listeners.click, 'function');
  assert.equal(typeof listeners.change, 'function');

  const malformedFilter = {
    dataset: { filter: 'proof' },
    type: 'select-one',
    value: 'MALFORMED_SOURCE',
    matches(selector) { return selector === '[data-filter]'; },
  };
  listeners.change({ target: malformedFilter });
  assert.match(root.innerHTML, /Fire Ball/);
  assert.match(root.innerHTML, /proof: MALFORMED_SOURCE/);

  const button = { dataset: { action: 'toggle-skill', skillId: 'skill:sor:fire-ball' } };
  listeners.click({ target: { closest() { return button; } } });
  assert.match(root.innerHTML, /Comparaison à trois voies/);
  assert.match(root.innerHTML, /<svg viewBox=/);
  assert.match(root.innerHTML, /Formule source/);
  assert.match(root.innerHTML, /Champ protégé/);
  assert.match(root.innerHTML, /Dépendances et fermeture/);
  assert.match(root.innerHTML, /Références Wiki PD2 épinglées/);
});

test('places Architecture globale before classes, opens it by default and leaves skill views unchanged', async () => {
  const report = mockReport({ schemaOrientation: mockSchemaOrientation() });
  const html = buildHtml(report);
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
      body: { innerHTML: '' },
      querySelector(selector) { return selector === '#workbench' ? root : null; },
      createElement() { return { click() {} }; },
    },
    localStorage: { getItem() { return null; }, setItem() {}, removeItem() {} },
    navigator: {},
    setTimeout() { return 1; },
    clearTimeout() {},
    confirm() { return true; },
  };
  context.window = context;
  vm.runInNewContext(embeddedScript(html), context, { filename: 'pd2-skills-review.html' });
  await context.__PD2_SKILLS_WORKBENCH_READY__;
  assert.ok(root.innerHTML.indexOf('Architecture globale') < root.innerHTML.indexOf('Sorceress'));
  assert.match(root.innerHTML, /PD2 Skills Schema and Engine Orientation/);
  assert.match(root.innerHTML, /id="schema-overview"/);
  assert.match(root.innerHTML, /82[\s\S]*Champs bruts modifiés/);
  assert.doesNotMatch(root.innerHTML, /class="skill-card/);

  const sorceressButton = { dataset: { view: 'sor' } };
  listeners.click({ target: { closest(selector) { return selector === 'button' ? sorceressButton : null; } } });
  assert.match(root.innerHTML, /Fire Ball/);
  assert.match(root.innerHTML, /class="skill-card/);
  assert.doesNotMatch(root.innerHTML, /id="schema-overview"/);

  const architectureButton = { dataset: { view: 'architecture' } };
  listeners.click({ target: { closest(selector) { return selector === 'button' ? architectureButton : null; } } });
  assert.match(root.innerHTML, /id="schema-overview"/);
  assert.doesNotMatch(root.innerHTML, /class="skill-card/);
});

test('integrates with the canonical browser decision runtime API', async () => {
  const html = buildHtml(mockReport(), buildBrowserRuntimeSource());
  const root = { innerHTML: '', addEventListener() {} };
  const context = {
    console,
    Blob,
    Response,
    DecompressionStream,
    Uint8Array,
    atob,
    URL,
    document: {
      body: { innerHTML: '' },
      querySelector(selector) { return selector === '#workbench' ? root : null; },
    },
    localStorage: {
      getItem() { return JSON.stringify({ comparisonHash: 'STALE', entries: {} }); },
      setItem() {},
      removeItem() {},
    },
    navigator: {},
    setTimeout() { return 1; },
    clearTimeout() {},
    confirm() { return true; },
  };
  context.window = context;
  vm.runInNewContext(embeddedScript(html), context, { filename: 'pd2-skills-review.html' });
  await context.__PD2_SKILLS_WORKBENCH_READY__;
  assert.match(root.innerHTML, /Fire Ball/);
  assert.doesNotMatch(root.innerHTML, /moteur de décisions embarqué est incomplet/);
});

test('threads the Phase 0 policy envelope through skill state and governed exports', async () => {
  const policyAwareRuntime = `
globalThis.schemaPolicyRuntime = {
  storageKey() { return 'policy-test-key'; },
  createEmptyEnvelope(orientation) { return { orientationId: orientation.orientationId, orientationHash: orientation.orientationHash, frozenContractHash: orientation.frozenContractHash, decisions: {} }; },
  validateImport(orientation, candidate) { return { valid: true, envelope: candidate }; },
};
globalThis.decisionRuntime = {
  constants: {},
  createEmptyEnvelope(report) { return { schemaVersion: 2, reviewId: report.reviewId, comparisonHash: report.comparisonHash, schemaPolicy: { marker: 'PHASE0' }, entries: {} }; },
  createEntry(skill) { return { fingerprint: skill.fingerprint, implementationStatus: 'NOT_REVIEWED', componentDecisions: {}, fieldDecisions: {}, notes: {} }; },
  entryState(report, skill, entry, schemaPolicy) {
    globalThis.__ENTRY_POLICY_SEEN__ = schemaPolicy?.marker;
    return { required: !skill.readOnly, complete: false, reasons: ['Phase 0 policy gate open'], schemaPolicyGate: { required: 8, closed: 0, complete: false } };
  },
  applyBulk(report, ids, entries) { return { ...entries }; },
  validateImport(report, payload) { return payload; },
  migrateEnvelope(report, payload) { return { envelope: payload, report: { retained: [], stale: [], dropped: [] } }; },
  exportEnvelope(report, entries, options) {
    globalThis.__EXPORT_POLICY_SEEN__ = options.schemaPolicy?.marker;
    return { reviewId: report.reviewId, comparisonHash: report.comparisonHash, schemaPolicy: options.schemaPolicy, exportScope: options.scope, entries };
  },
  storageKey(report) { return 'pd2-skills-review-decisions-v2:' + report.comparisonHash; },
};`;
  const report = mockReport({ schemaOrientation: mockSchemaOrientation() });
  const html = buildHtml(report, policyAwareRuntime);
  const listeners = {};
  const root = { innerHTML: '', addEventListener(type, listener) { listeners[type] = listener; }, querySelector() { return null; } };
  const context = {
    console, Blob, Response, DecompressionStream, Uint8Array, atob, URL,
    document: {
      body: root,
      querySelector(selector) { return selector === '#workbench' ? root : null; },
      createElement() { return { click() {} }; },
    },
    localStorage: { getItem() { return null; }, setItem() {}, removeItem() {} },
    navigator: {}, setTimeout() { return 1; }, clearTimeout() {}, confirm() { return true; },
  };
  context.window = context;
  vm.runInNewContext(embeddedScript(html), context);
  await context.__PD2_SKILLS_WORKBENCH_READY__;
  const sorceressButton = { dataset: { view: 'sor' } };
  listeners.click({ target: { closest(selector) { return selector === 'button' ? sorceressButton : null; } } });
  assert.equal(context.__ENTRY_POLICY_SEEN__, 'PHASE0');
  assert.match(root.innerHTML, /Phase 0 policy gate open/);
  const exportButton = { dataset: { action: 'export-all' } };
  listeners.click({ target: { closest(selector) { return selector === 'button' ? exportButton : null; } } });
  assert.equal(context.__EXPORT_POLICY_SEEN__, 'PHASE0');
});

test('shows an explicit local compatibility error when gzip decompression is unavailable', async () => {
  const html = buildHtml(mockReport());
  const details = { textContent: '' };
  const root = {
    innerHTML: '',
    querySelector(selector) { return selector === 'pre' ? details : null; },
  };
  const context = {
    console,
    Blob,
    Response,
    Uint8Array,
    atob,
    document: {
      body: root,
      querySelector(selector) { return selector === '#workbench' ? root : null; },
    },
  };
  context.window = context;
  vm.runInNewContext(embeddedScript(html), context, { filename: 'pd2-skills-review.html' });
  await assert.rejects(context.__PD2_SKILLS_WORKBENCH_READY__, /DecompressionStream/);
  assert.match(root.innerHTML, /Impossible de décompresser l’oracle local/);
  assert.match(details.textContent, /version récente de Chromium, Edge ou Firefox/);
  assert.match(context.__PD2_SKILLS_WORKBENCH_ERROR__, /DecompressionStream/);
});

test('escapes report and runtime closing-script injection without losing deterministic output', () => {
  const report = mockReport();
  report.skills[0].canonicalName = '</script><script>globalThis.pwned=true</script>';
  const hostileRuntime = runtimeSource + '\n/* </script><script>globalThis.runtimePwned=true</script> */';
  const first = buildHtml(report, hostileRuntime);
  const second = buildHtml(report, hostileRuntime);
  assert.equal(first, second);
  assert.doesNotMatch(first, /<script>globalThis\.pwned/);
  assert.doesNotMatch(first, /<script>globalThis\.runtimePwned/);
  assert.equal((first.match(/<script>/g) || []).length, 1);
  assert.doesNotThrow(() => new vm.Script(embeddedScript(first)));
});

test('includes protected override, CUSTOM governance, bulk safety and read-only wording', () => {
  const html = buildHtml(mockReport());
  for (const expected of [
    'Valeur ou formule CUSTOM', 'Justification', 'Objectif de gameplay', 'Plan de test',
    'Override protégé obligatoire', 'J’autorise explicitement cette exception',
    'Seulement les décisions indécises', 'Remplacer toutes les décisions',
    'Vider uniquement les décisions indécises', 'Aucune fusion automatique',
    'DECISION_COMPLETE ne devient jamais automatiquement IMPLEMENTATION_AUTHORIZED',
  ]) assert.match(html, new RegExp(expected.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')));
});

test('rejects invalid generator inputs', () => {
  assert.throws(() => buildSkillReviewHtml(null, runtimeSource), /report/);
  assert.throws(() => buildSkillReviewHtml({}, runtimeSource), /report\.skills/);
  assert.throws(() => buildSkillReviewHtml({ skills: [], comparisonHash: 'x' }, ''), /browserRuntimeSource/);
  assert.throws(() => buildSkillReviewHtml({ skills: [], comparisonHash: 'x' }, runtimeSource), /compressedOracleBase64/);
});
