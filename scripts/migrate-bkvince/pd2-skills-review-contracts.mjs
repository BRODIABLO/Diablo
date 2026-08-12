import crypto from 'node:crypto';

export const ORACLE_SCHEMA_VERSION = 1;
export const DECISION_SCHEMA_VERSION = 1;
export const PREVIEW_SCHEMA_VERSION = 1;
export const REVIEW_ID = 'pd2-skills-review-v1';
export const PREVIEW_ID = 'pd2-skills-decisions-preview-v1';
export const DECISION_EXPORT_KIND = 'pd2-skills-review-decisions';
export const STORAGE_KEY_PREFIX = 'pd2-skills-review-decisions-v1:';

export const SOURCE_ORDER = Object.freeze(['vanilla32', 'bkvince', 'pd2']);
export const CLASS_ORDER = Object.freeze(['ama', 'sor', 'nec', 'pal', 'bar', 'dru', 'ass', 'war']);
export const PLAYER_CLASS_CODES = Object.freeze(['ama', 'sor', 'nec', 'pal', 'bar', 'dru', 'ass']);
export const CLASS_NAMES = Object.freeze({
  ama: 'Amazon',
  sor: 'Sorceress',
  nec: 'Necromancer',
  pal: 'Paladin',
  bar: 'Barbarian',
  dru: 'Druid',
  ass: 'Assassin',
  war: 'Warlock',
  pd2_new: 'Nouveaux skills PD2',
  bkv_only: 'Skills propres à BKVince',
  collisions: 'Collisions et remplacements',
  technical: 'Skills techniques / classless',
});

export const MAPPING_TYPES = Object.freeze([
  'SAME_SKILL_SAME_ORDINAL',
  'SAME_SKILL_MOVED_ORDINAL',
  'RENAMED_ALIAS',
  'SLOT_REPLACEMENT',
  'PD2_ONLY_PLAYER_SKILL',
  'BKV_ONLY_PLAYER_SKILL',
  'SAME_ORDINAL_DIFFERENT_SKILL',
  'TECHNICAL_OR_CLASSLESS',
  'IDENTICAL',
]);

export const PROOF_STATUSES = Object.freeze([
  'EXACT_TABLE',
  'EXACT_FORMULA',
  'EXACT_DERIVED',
  'SYMBOLIC',
  'MALFORMED_SOURCE',
  'UNSUPPORTED_IDENTIFIER',
  'NATIVE_UNPROVEN',
]);

export const PORTABILITY_CATEGORIES = Object.freeze([
  'DATA_ONLY_PROVEN',
  'DATA_WITH_LINKED_TABLES',
  'APPEND_ONLY_REQUIRED',
  'NATIVE_FUNCTION_MISMATCH',
  'NATIVE_UNPROVEN',
  'BLOCKED_DEPENDENCY',
  'SAVE_OR_ID_RISK',
  'NETWORK_OR_CLIENT_SERVER_RISK',
  'NOT_APPLICABLE',
]);

export const GLOBAL_DECISIONS = Object.freeze([
  'KEEP_BKVINCE',
  'ADAPT_PD2_SELECTIVELY',
  'ADOPT_PD2_MODEL',
  'IMPORT_NEW_PD2_SKILL',
  'REJECT_PD2',
  'DEFER_NATIVE_PROOF',
  'DISCUSS',
]);

export const COMPONENT_DECISIONS = Object.freeze([
  'KEEP_BKVINCE',
  'ADOPT_PD2',
  'CUSTOM',
  'DISCUSS',
  'NOT_APPLICABLE',
]);

export const NEW_SKILL_LINE_DECISIONS = Object.freeze([
  'IMPORT_APPEND_ONLY',
  'IMPORT_CUSTOMIZED',
  'REJECT_PD2_SKILL',
  'DEFER_NATIVE_PROOF',
  'DISCUSS',
]);

export const IMPLEMENTATION_STATUSES = Object.freeze([
  'NOT_REVIEWED',
  'DECISION_INCOMPLETE',
  'DECISION_COMPLETE',
  'SELECTED_FOR_PROTOTYPE',
  'IMPLEMENTATION_NOT_AUTHORIZED',
  'IMPLEMENTATION_AUTHORIZED',
  'IMPLEMENTED',
  'TESTED',
  'REJECTED',
]);

export const DOCUMENTATION_STATUSES = Object.freeze(['DOCUMENTED', 'TABLE_ONLY', 'UNMAPPED']);

export const BEHAVIOR_GROUPS = Object.freeze([
  Object.freeze({ id: 'identity_availability', label: 'Identité et disponibilité' }),
  Object.freeze({ id: 'cost_timing', label: 'Coût et timing' }),
  Object.freeze({ id: 'damage_model', label: 'Modèle de dégâts' }),
  Object.freeze({ id: 'area_targeting', label: 'Zone et ciblage' }),
  Object.freeze({ id: 'projectiles_collisions', label: 'Projectiles et collisions' }),
  Object.freeze({ id: 'animation_sequence', label: 'Animation et séquence' }),
  Object.freeze({ id: 'buffs_debuffs_auras_passives', label: 'Buffs, debuffs, auras et passifs' }),
  Object.freeze({ id: 'synergies', label: 'Synergies' }),
  Object.freeze({ id: 'summons', label: 'Summons' }),
  Object.freeze({ id: 'engine_functions', label: 'Fonctions moteur' }),
  Object.freeze({ id: 'interface_localization', label: 'Interface et localisation' }),
  Object.freeze({ id: 'consumers', label: 'Consommateurs' }),
]);

export const PROTECTED_FIELD_RULES = Object.freeze([
  Object.freeze({ id: 'runtime_ordinal', match: Object.freeze(['runtimeOrdinal']), reason: "L'ordinal réel est l'identité runtime et reste immuable pour toute ligne BKVince existante." }),
  Object.freeze({ id: 'row_order', match: Object.freeze(['rowOrder']), reason: "L'ordre des lignes BKVince existantes ne peut pas changer." }),
  Object.freeze({ id: 'warlock', match: Object.freeze(['warlockExistingRow']), reason: 'Les lignes Warlock BKVince existantes sont conservées par défaut.' }),
  Object.freeze({ id: 'persistent_ids', match: Object.freeze(['stateId', 'itemStatCostId', 'saveBits', 'sendBits']), reason: 'Les identifiants persistants et leurs bits de sérialisation sont protégés.' }),
  Object.freeze({ id: 'maxlvl', match: Object.freeze(['maxlvl']), reason: 'Le maxlvl BKVince est protégé par défaut.' }),
  Object.freeze({ id: 'charclass', match: Object.freeze(['charclass']), reason: "L'appartenance de classe ne peut pas être adoptée en lot." }),
  Object.freeze({ id: 'native_functions', pattern: '^(srv|clt).*(func|function)$|^.*hitfunc$', reason: 'Tout callback natif divergent exige une preuve D2R 3.2 suffisante.' }),
  Object.freeze({ id: 'delay_translation', match: Object.freeze(['delay', 'localdelay', 'globaldelay']), reason: 'PD2 delay ne peut jamais être traduit automatiquement en localdelay/globaldelay.' }),
  Object.freeze({ id: 'malformed_formula', match: Object.freeze(['malformedFormula']), reason: "Une formule malformée reste une preuve source tant que son intention n'est pas gouvernée." }),
  Object.freeze({ id: 'ordinal_collision', match: Object.freeze(['ordinalCollision']), reason: "Une cellule liée à une collision d'ordinal non résolue est protégée." }),
]);

export const NON_MUTATION_RULES = Object.freeze({
  allowedReadRoots: Object.freeze([
    'data-vanilla3.2/',
    'data-BKVince/',
    '../PD2 Single PLayer/PD2-Single-Player-Plus-mod-main/',
    'Mission/',
    'reverse-engineering/',
    'schemas/',
    'scripts/',
  ]),
  allowedGeneratedRoots: Object.freeze(['Mission/', 'analysis-cache/']),
  forbiddenWriteRoots: Object.freeze([
    'data-BKVince/',
    'data-TCP/',
    'data-BK/',
    'data-BT/',
    'data-VNP/',
    'data-vanilla3.2/',
    '../PD2 Single PLayer/',
  ]),
  forbiddenCliFlags: Object.freeze(['--apply']),
  statement: 'The workbench selects and previews decisions only; it never mutates gameplay, saves, or an installed runtime profile.',
});

export const GENERATOR_INTERFACE = Object.freeze({
  input: Object.freeze({
    sourceTables: 'raw governed TSV/JSON sources plus pinned wiki metadata',
    policies: 'frozen mapping, proof, portability, presentation, and protection contracts',
  }),
  output: Object.freeze({
    oracle: 'Mission/pd2-skills-review.json',
    html: 'Mission/pd2-skills-review.html',
    documentationMap: 'Mission/pd2-skills-documentation-map.json',
  }),
  invariants: Object.freeze([
    'Every source row is represented by exactly one physical node.',
    'Semantic identity and runtime-slot occupancy remain independent graphs.',
    'Every player-skill node is reachable from a governed class/tree view.',
    'Every collision is represented explicitly and never auto-merged.',
    'The comparison hash excludes export timestamps and includes all governed source and policy hashes.',
  ]),
});

export const HTML_INTERFACE = Object.freeze({
  input: 'the complete oracle object plus the pure browser decision runtime',
  output: 'one dependency-free UTF-8 HTML document that works under file://',
  persistence: `${STORAGE_KEY_PREFIX}<comparisonHash>`,
  mutation: 'decisions exist only in browser memory/localStorage or explicit downloads',
});

export const PREVIEW_INTERFACE = Object.freeze({
  inputs: Object.freeze(['Mission/pd2-skills-review.json', 'a governed exported decision envelope']),
  output: 'a preview-only manifest and textual diff on stdout or an explicitly safe output path',
  state: 'PREVIEW_ONLY_GAMEPLAY_APPLICATION_FORBIDDEN',
  atomicity: 'No applicable manifest is emitted unless the complete selected lot passes every gate.',
});

export function canonicalize(value) {
  if (Array.isArray(value)) return `[${value.map(canonicalize).join(',')}]`;
  if (value && typeof value === 'object') {
    return `{${Object.keys(value).sort().map((key) => `${JSON.stringify(key)}:${canonicalize(value[key])}`).join(',')}}`;
  }
  return JSON.stringify(value);
}

export function sha256Canonical(value) {
  return crypto.createHash('sha256').update(canonicalize(value)).digest('hex').toUpperCase();
}

export function normalizeSkillName(value) {
  return String(value ?? '')
    .normalize('NFKD')
    .replace(/[\u0300-\u036f]/g, '')
    .trim()
    .toLowerCase()
    .replace(/[^a-z0-9]+/g, '-').replace(/^-+|-+$/g, '');
}

export function physicalNodeId(source, ordinal) {
  if (!SOURCE_ORDER.includes(source)) throw new Error(`Unknown skill source ${source}`);
  if (!Number.isInteger(ordinal) || ordinal < 0) throw new Error(`Invalid runtime ordinal ${ordinal}`);
  return `${source}:skills.txt:${ordinal}`;
}

export function stableSkillId(scope, canonicalName, discriminator = '') {
  const normalizedScope = normalizeSkillName(scope || 'classless');
  const normalizedName = normalizeSkillName(canonicalName || 'unnamed');
  const normalizedDiscriminator = normalizeSkillName(discriminator);
  return `skill:${normalizedScope}:${normalizedName}${normalizedDiscriminator ? `:${normalizedDiscriminator}` : ''}`;
}

export const FROZEN_CONTRACT_HASH = sha256Canonical({
  ORACLE_SCHEMA_VERSION,
  DECISION_SCHEMA_VERSION,
  PREVIEW_SCHEMA_VERSION,
  REVIEW_ID,
  DECISION_EXPORT_KIND,
  SOURCE_ORDER,
  CLASS_ORDER,
  PLAYER_CLASS_CODES,
  MAPPING_TYPES,
  PROOF_STATUSES,
  PORTABILITY_CATEGORIES,
  GLOBAL_DECISIONS,
  COMPONENT_DECISIONS,
  NEW_SKILL_LINE_DECISIONS,
  IMPLEMENTATION_STATUSES,
  DOCUMENTATION_STATUSES,
  BEHAVIOR_GROUPS,
  PROTECTED_FIELD_RULES,
  NON_MUTATION_RULES,
  GENERATOR_INTERFACE,
  HTML_INTERFACE,
  PREVIEW_INTERFACE,
});
