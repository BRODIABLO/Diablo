import { sha256Canonical } from './pd2-skills-review-contracts.mjs';

export const ORIENTATION_SCHEMA_VERSION = 1;
export const ORIENTATION_ID = 'pd2-skills-schema-orientation-v1';
export const ORIENTATION_PRODUCT_NAME = 'PD2 Skills Schema and Engine Orientation';
export const POLICY_SCHEMA_VERSION = 1;
export const POLICY_KIND = 'pd2-skills-schema-policy';
export const POLICY_STORAGE_PREFIX = 'pd2-skills-schema-policy-v1:';

export const SOURCE_KEYS = Object.freeze(['vanilla32', 'bkvince', 'pd2']);

export const RAW_VALUE_STATES = Object.freeze([
  'ABSENT_COLUMN',
  'NULL_VALUE',
  'EMPTY_STRING',
  'ZERO',
  'VALUE',
]);

export const SCHEMA_CLASSIFICATIONS = Object.freeze([
  'D2R_NATIVE_PRESERVE',
  'BKVINCE_EXTENSION_PRESERVE',
  'PD2_SCHEMA_UNUSED',
  'PD2_SEMANTIC_SOURCE_ONLY',
  'MAP_TO_EXISTING_D2R_FIELD',
  'NATIVE_EXTENSION_REQUIRED',
  'DOCUMENTARY_ONLY',
  'UI_ONLY',
  'ITEM_ECONOMY_ONLY',
  'RAW_TECHNICAL_OVERRIDE',
  'UNKNOWN_NATIVE_CONSUMER',
]);

export const DECISION_SCOPES = Object.freeze([
  'NO_SKILL_DECISION',
  'GLOBAL_POLICY',
  'BEHAVIOR_BUNDLE',
  'EXPERT_OVERRIDE_ONLY',
]);

export const POLICY_DECISIONS = Object.freeze([
  'PENDING',
  'APPROVE',
  'MODIFY',
  'REJECT',
  'DISCUSS',
]);

export const POLICY_DECISION_ENTRY_CONTRACT = Object.freeze({
  required: Object.freeze(['fingerprint', 'decision', 'justification']),
  optional: Object.freeze(['customPolicy']),
  fingerprint: 'SHA-256 of the individual governed policy definition',
  completion: 'APPROVE requires justification; MODIFY requires justification and an explicit customPolicy; every other state leaves the gate open.',
});

export const GLOBAL_SCHEMA_POLICIES = Object.freeze([
  Object.freeze({
    id: 'PRESERVE_ALL_D2R_BKVINCE_COLUMNS',
    titleFr: 'Préserver toutes les colonnes D2R/BKVince',
    statementFr: 'Le schéma cible conserve toutes les colonnes Vanilla D2R 3.2 et BKVince, sans suppression ni réordonnancement.',
    proposedDecision: 'APPROVE',
    requiredForSkillCompletion: true,
  }),
  Object.freeze({
    id: 'IGNORE_ALL_EMPTY_PD2_ONLY_COLUMNS',
    titleFr: 'Ignorer les colonnes PD2-only entièrement vides',
    statementFr: 'Une colonne propre à PD2 entièrement semanticBlank reste une preuve de schéma et ne crée aucune décision par skill.',
    proposedDecision: 'APPROVE',
    requiredForSkillCompletion: true,
  }),
  Object.freeze({
    id: 'NO_PD2_HEADER_IMPORT_WITHOUT_NATIVE_PROOF',
    titleFr: 'Interdire les headers PD2 sans preuve native',
    statementFr: 'Aucune colonne PD2 absente de D2R/BKVince ne peut être ajoutée sans preuve que le build D2R 3.2 la compile et la consomme.',
    proposedDecision: 'APPROVE',
    requiredForSkillCompletion: true,
  }),
  Object.freeze({
    id: 'NO_AUTOMATIC_DELAY_TRANSLATION',
    titleFr: 'Ne jamais traduire delay automatiquement',
    statementFr: 'PD2 delay, D2R localdelay, globaldelay et perdelay restent des modèles distincts tant qu’une équivalence n’est pas prouvée.',
    proposedDecision: 'APPROVE',
    requiredForSkillCompletion: true,
  }),
  Object.freeze({
    id: 'SEMANTIC_BLANKS_REQUIRE_NO_DECISION',
    titleFr: 'Auto-résoudre les blancs sémantiques',
    statementFr: 'Absent, null et chaîne vide restent distincts dans la preuve brute mais ne demandent aucune décision gameplay lorsqu’ils sont tous semanticBlank.',
    proposedDecision: 'APPROVE',
    requiredForSkillCompletion: true,
  }),
  Object.freeze({
    id: 'ITEM_ECONOMY_FIELDS_PRESERVE_BKVINCE',
    titleFr: 'Préserver les champs d’économie des objets',
    statementFr: 'cost add et cost mult conservent BKVince par défaut et ne deviennent pas des décisions de balance du skill.',
    proposedDecision: 'APPROVE',
    requiredForSkillCompletion: true,
  }),
  Object.freeze({
    id: 'NATIVE_CALLBACKS_PRESERVE_OR_DEFER',
    titleFr: 'Préserver ou différer les callbacks natifs',
    statementFr: 'Les numéros de fonctions serveur/client sont propres au moteur; une divergence reste protégée ou différée sans preuve D2R 3.2.',
    proposedDecision: 'APPROVE',
    requiredForSkillCompletion: true,
  }),
  Object.freeze({
    id: 'RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE',
    titleFr: 'Projeter les bundles vers les cellules brutes',
    statementFr: 'Une décision de comportement gouverne ses cellules constitutives; les cellules individuelles restent accessibles uniquement en mode expert.',
    proposedDecision: 'APPROVE',
    requiredForSkillCompletion: true,
  }),
]);

export const DECISION_BUNDLES = Object.freeze([
  Object.freeze({
    id: 'PHYSICAL_DAMAGE_CURVE',
    labelFr: 'Courbe de dégâts physiques',
    fieldPatterns: Object.freeze(['^(?:mindam|maxdam|minlevdam[1-5]|maxlevdam[1-5]|srcdam)$']),
    linkedTables: Object.freeze([]),
    protected: false,
  }),
  Object.freeze({
    id: 'ELEMENTAL_DAMAGE_CURVE',
    labelFr: 'Courbe de dégâts élémentaires',
    fieldPatterns: Object.freeze(['^(?:etype|hitshift|emin|emax|eminlev[1-5]|emaxlev[1-5]|elen|elevlen[1-3])$']),
    linkedTables: Object.freeze([]),
    protected: false,
  }),
  Object.freeze({
    id: 'MANA_CURVE',
    labelFr: 'Coût en mana réel',
    fieldPatterns: Object.freeze(['^(?:mana|lvlmana|manashift|minmana|startmana)$']),
    linkedTables: Object.freeze([]),
    protected: false,
  }),
  Object.freeze({
    id: 'DAMAGE_SYNERGIES',
    labelFr: 'Synergies de dégâts',
    fieldPatterns: Object.freeze(['^(?:dmgsympercalc|edmgsympercalc)$']),
    dynamicDependencies: 'PARAM_REFERENCES_FROM_SELECTED_FORMULAS',
    linkedTables: Object.freeze(['skills.txt']),
    protected: false,
  }),
  Object.freeze({
    id: 'LENGTH_SYNERGIES',
    labelFr: 'Synergies de durée élémentaire',
    fieldPatterns: Object.freeze(['^elensympercalc$']),
    dynamicDependencies: 'PARAM_REFERENCES_FROM_SELECTED_FORMULAS',
    linkedTables: Object.freeze(['skills.txt']),
    protected: false,
  }),
  Object.freeze({
    id: 'AURA_RADIUS',
    labelFr: 'Rayon d’aura',
    fieldPatterns: Object.freeze(['^aurarangecalc$']),
    dynamicDependencies: 'PARAM_REFERENCES_FROM_SELECTED_FORMULAS',
    linkedTables: Object.freeze([]),
    protected: false,
  }),
  Object.freeze({
    id: 'AURA_DURATION',
    labelFr: 'Durée d’aura ou de buff',
    fieldPatterns: Object.freeze(['^auralencalc$']),
    dynamicDependencies: 'PARAM_REFERENCES_FROM_SELECTED_FORMULAS',
    linkedTables: Object.freeze([]),
    protected: false,
  }),
  Object.freeze({
    id: 'PROJECTILE_ARCHITECTURE',
    labelFr: 'Architecture des projectiles',
    fieldPatterns: Object.freeze(['^(?:srv|clt)missile[a-d]?$']),
    linkedTables: Object.freeze(['missiles.txt']),
    protected: true,
  }),
  Object.freeze({
    id: 'PROJECTILE_PHYSICS',
    labelFr: 'Physique des projectiles',
    fieldPatterns: Object.freeze([]),
    linkedTables: Object.freeze(['missiles.txt:Vel', 'missiles.txt:MaxVel', 'missiles.txt:Range', 'missiles.txt:Size', 'missiles.txt:CollideKill', 'missiles.txt:NextHit']),
    protected: true,
  }),
  Object.freeze({
    id: 'ITEM_TRIGGER_EXECUTION',
    labelFr: 'Exécution déclenchée par un objet',
    fieldPatterns: Object.freeze(['^(?:itemeffect|itemclteffect|itemuserestrict)$']),
    linkedTables: Object.freeze([]),
    protected: true,
  }),
  Object.freeze({
    id: 'NATIVE_EXECUTION',
    labelFr: 'Exécution native serveur/client',
    fieldPatterns: Object.freeze(['^(?:srv|clt).*(?:func|function)\\d*$', '^hitfunc\\d*$']),
    linkedTables: Object.freeze([]),
    protected: true,
  }),
  Object.freeze({
    id: 'SUMMON_PACKAGE',
    labelFr: 'Package d’invocation',
    fieldPatterns: Object.freeze(['^(?:summon|pettype|petmax|requirespettype|sumskill[1-5]|sumsk[1-5]calc)$']),
    linkedTables: Object.freeze(['pettype.txt', 'monstats.txt']),
    protected: true,
  }),
  Object.freeze({
    id: 'PASSIVE_PACKAGE',
    labelFr: 'Package passif',
    fieldPatterns: Object.freeze(['^(?:passivestate|passiveitype|passivereqweaponcount|passivestat(?:[1-9]|1[0-4])|passivecalc(?:[1-9]|1[0-4])|passiveevent|passiveeventfunc)$']),
    linkedTables: Object.freeze(['states.txt', 'itemstatcost.txt']),
    protected: true,
  }),
  Object.freeze({
    id: 'UI_ASSIGNMENT',
    labelFr: 'Assignation et contrôles d’interface',
    fieldPatterns: Object.freeze(['^(?:skilldesc|leftskill|rightskill|keepcursorstateonkill|continuecastunselected|clearselectedonhold)$']),
    linkedTables: Object.freeze(['skilldesc.txt']),
    protected: false,
  }),
  Object.freeze({
    id: 'COOLDOWN_MODEL',
    labelFr: 'Modèle de cooldown et cadence',
    fieldPatterns: Object.freeze(['^(?:delay|localdelay|globaldelay|perdelay)$']),
    linkedTables: Object.freeze([]),
    protected: true,
  }),
]);

export const MECHANICAL_CONTRACTS = Object.freeze([
  Object.freeze({
    id: 'cooldowns',
    titleFr: 'Cooldowns',
    fields: Object.freeze(['delay', 'localdelay', 'globaldelay', 'perdelay']),
    consumerFr: 'Cadence d’exécution et délais de relance du skill.',
    provenRelations: Object.freeze([
      'localdelay contrôle le délai propre au skill dans le schéma D2R documenté.',
      'globaldelay contrôle le délai imposé aux autres skills à délai dans le schéma D2R documenté.',
      'perdelay contrôle la cadence périodique lorsqu’un skill periodic ou aura l’active.',
    ]),
    hypotheses: Object.freeze(['Aucune équivalence numérique ou comportementale entre PD2 delay et les champs D2R n’est prouvée.']),
    translationPolicy: 'NO_AUTOMATIC_DELAY_TRANSLATION',
    proofStatus: 'NATIVE_UNPROVEN',
  }),
  Object.freeze({
    id: 'engine_functions',
    titleFr: 'Fonctions moteur',
    fields: Object.freeze(['srvstfunc', 'srvdofunc', 'srvstopfunc', 'cltstfunc', 'cltdofunc', 'cltstopfunc', 'itemeffect', 'itemclteffect', 'checkfunc']),
    consumerFr: 'Dispatch natif serveur/client; les nombres sont des sélecteurs propres à chaque moteur.',
    provenRelations: Object.freeze([
      'D2R 3.2 srvdofunc 20 consomme calc1, calc2 et aurarangecalc pour Static Field.',
      'D2R 3.2 srvdofunc 30 consomme auratargetstate, aurastat1..6, aurarangecalc et auralencalc pour les malédictions/états de zone.',
      'itemeffect et itemclteffect réutilisent respectivement les familles de fonctions Do serveur et client lors d’un déclenchement par objet.',
    ]),
    hypotheses: Object.freeze(['Aucun numéro de callback PD2 n’est présumé isomorphe à D2R 3.2.', 'checkfunc n’existe pas dans le schéma cible et son consumer D2R n’est pas prouvé.']),
    translationPolicy: 'NATIVE_CALLBACKS_PRESERVE_OR_DEFER',
    proofStatus: 'NATIVE_UNPROVEN',
  }),
  Object.freeze({
    id: 'damage_curves',
    titleFr: 'Courbes de dégâts',
    fields: Object.freeze(['mindam', 'maxdam', 'minlevdam1..5', 'maxlevdam1..5', 'emin', 'emax', 'eminlev1..5', 'emaxlev1..5', 'hitshift', 'dmgsympercalc', 'edmgsympercalc']),
    consumerFr: 'Calcul des dégâts physiques et élémentaires, leurs cinq paliers et leurs synergies.',
    provenRelations: Object.freeze(['Les paliers 1..5 correspondent aux niveaux 2–8, 9–16, 17–22, 23–28 et 29+.', 'HitShift participe à l’encodage/échelle des dégâts et ne se décide jamais isolément de la courbe concernée.']),
    hypotheses: Object.freeze([]),
    translationPolicy: 'RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE',
    proofStatus: 'EXACT_FORMULA',
  }),
  Object.freeze({
    id: 'mana',
    titleFr: 'Mana',
    fields: Object.freeze(['mana', 'lvlmana', 'manashift', 'minmana', 'startmana']),
    consumerFr: 'Calcul du coût réel en mana et de son évolution avec le niveau.',
    provenRelations: Object.freeze(['Les cinq champs forment une seule courbe de coût; une cellule seule ne décrit pas le coût réel.']),
    hypotheses: Object.freeze([]),
    translationPolicy: 'RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE',
    proofStatus: 'EXACT_FORMULA',
  }),
  Object.freeze({
    id: 'projectiles',
    titleFr: 'Projectiles',
    fields: Object.freeze(['srvmissile', 'srvmissilea..c', 'cltmissile', 'cltmissilea..d']),
    consumerFr: 'Création serveur/client des missiles et résolution liée dans missiles.txt.',
    provenRelations: Object.freeze(['Les slots serveur et client ne sont pas interchangeables.', 'Vitesse, portée, hitbox, collisions et hit functions résident dans les lignes missiles liées.']),
    hypotheses: Object.freeze(['L’identité textuelle d’un missile ne prouve pas l’identité de ses callbacks entre PD2 et D2R.']),
    translationPolicy: 'NATIVE_CALLBACKS_PRESERVE_OR_DEFER',
    proofStatus: 'NATIVE_UNPROVEN',
  }),
  Object.freeze({
    id: 'calc_param',
    titleFr: 'Calc et Param',
    fields: Object.freeze(['calc1..10', 'Param1..20', '*calcN desc', '*ParamN Description']),
    consumerFr: 'Entrées génériques dont le sens dépend du callback et de la description documentaire de la ligne.',
    provenRelations: Object.freeze(['Les descriptions propres à la ligne ont priorité pour l’affichage joueur.', 'Un Param référencé par une formule appartient au même bundle que cette formule.']),
    hypotheses: Object.freeze(['Un même numéro calc/Param peut porter une sémantique différente selon la fonction native.']),
    translationPolicy: 'RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE',
    proofStatus: 'NATIVE_UNPROVEN',
  }),
  Object.freeze({
    id: 'auras_passives',
    titleFr: 'Auras et passifs',
    fields: Object.freeze(['aurastate', 'auratargetstate', 'auraevent1..4', 'aurastat1..6', 'passivestate', 'passivestat1..14', 'passiveevent']),
    consumerFr: 'États, événements et statistiques appliqués par les fonctions d’aura ou passives.',
    provenRelations: Object.freeze(['D2R 3.2 expose six slots aurastat et quatorze slots passivestat dans son schéma actuel.']),
    hypotheses: Object.freeze(['Les colonnes PD2 passiveevent et passiveeventfunc sont absentes du schéma D2R/BKVince et leur consommation D2R n’est pas prouvée.']),
    translationPolicy: 'NO_PD2_HEADER_IMPORT_WITHOUT_NATIVE_PROOF',
    proofStatus: 'NATIVE_UNPROVEN',
  }),
  Object.freeze({
    id: 'summons',
    titleFr: 'Summons',
    fields: Object.freeze(['pettype', 'petmax', 'summon', 'sumskill1..5', 'sumsk1calc..5', 'requirespettype']),
    consumerFr: 'Création, limite et package de skills du pet, avec dépendances pettype.txt et monstats.txt.',
    provenRelations: Object.freeze(['Une ligne skills.txt seule ne ferme pas un summon; pettype et monstats font partie du package.']),
    hypotheses: Object.freeze(['La cadence AI et les héritages de mastery exigent des preuves liées, pas une déduction depuis le nom du pet.']),
    translationPolicy: 'RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE',
    proofStatus: 'NATIVE_UNPROVEN',
  }),
  Object.freeze({
    id: 'item_triggered_skills',
    titleFr: 'Skills déclenchés par objet',
    fields: Object.freeze(['ItemEffect', 'ItemCltEffect', 'ItemUseRestrict']),
    consumerFr: 'Exécution serveur/client lorsque le skill provient d’un proc, d’une charge ou d’un objet.',
    provenRelations: Object.freeze(['ItemEffect sélectionne une fonction Do serveur.', 'ItemCltEffect sélectionne une fonction Do client.']),
    hypotheses: Object.freeze(['Une différence de numéro entre PD2 et D2R ne prouve aucune équivalence de comportement.']),
    translationPolicy: 'NATIVE_CALLBACKS_PRESERVE_OR_DEFER',
    proofStatus: 'NATIVE_UNPROVEN',
  }),
  Object.freeze({
    id: 'interface_controls',
    titleFr: 'Interface et contrôles',
    fields: Object.freeze(['leftskill', 'rightskill', 'keepcursorstateonkill', 'continuecastunselected', 'clearselectedonhold', 'skilldesc']),
    consumerFr: 'Assignation des boutons, maintien du curseur et présentation du skill dans l’interface.',
    provenRelations: Object.freeze(['skilldesc pointe vers la ligne d’interface et les coordonnées d’arbre.', 'L’absence d’un header PD2 n’est pas une demande de suppression du header D2R.']),
    hypotheses: Object.freeze([]),
    translationPolicy: 'PRESERVE_ALL_D2R_BKVINCE_COLUMNS',
    proofStatus: 'EXACT_TABLE',
  }),
]);

export const NON_MUTATION_RULES = Object.freeze({
  targetSchema: 'D2R_3_2_BKVINCE',
  preserveHeaders: true,
  preserveRowOrder: true,
  preserveOrdinals: true,
  forbiddenWriteRoots: Object.freeze([
    'data-BKVince/',
    'data-TCP/',
    'data-vanilla3.2/',
    '../PD2 Single PLayer/',
  ]),
  statement: 'Phase 0 is analytical and policy-only. It never mutates gameplay tables, runtime profiles, saves, ordinals, rows, or damage values.',
});

export const ORIENTATION_INTERFACES = Object.freeze({
  generator: Object.freeze({
    inputs: Object.freeze(['three governed Skills.txt tables', 'the governed skill oracle', 'schemas/skills.json', 'the pinned historical audit', 'governed native findings']),
    outputs: Object.freeze([
      'Mission/pd2-skills-schema-orientation.json',
      'Mission/pd2-skills-schema-orientation.md',
      'Mission/pd2-skills-schema-orientation.html',
      'Mission/pd2-skills-field-dictionary.json',
      'Mission/pd2-skills-schema-policy.schema.json',
      'Mission/pd2-skills-schema-policy.example.json',
    ]),
    mutation: 'DOCUMENTARY_OUTPUTS_ONLY',
  }),
  html: Object.freeze({
    transport: 'single dependency-free file:// document with deterministic gzip payload',
    policyStorage: `${POLICY_STORAGE_PREFIX}<orientationHash>`,
    mutation: 'localStorage and explicit downloads only',
  }),
  workbench: Object.freeze({
    navigationId: 'architecture',
    navigationLabel: 'Architecture globale',
    activation: 'before all class views',
    skillDecisionModel: 'unchanged until explicit approval of global policies',
    completionGate: 'non-read-only skill decisions remain incomplete while required Phase 0 policies are not approved or explicitly modified',
  }),
});

export const KEY_FIELD_PRESENTATION = Object.freeze({
  'cost add': Object.freeze({
    playerLabelFr: 'Influence sur la valeur en or',
    shortHelpFr: 'Ajout plat au prix d’achat, de vente et de réparation d’un objet accordant ce skill.',
    family: 'item_economy',
    technicalOnly: true,
    protected: false,
    decisionScope: 'NO_SKILL_DECISION',
    groupId: 'ITEM_ECONOMY',
  }),
  'cost mult': Object.freeze({
    playerLabelFr: 'Multiplicateur de valeur en or',
    shortHelpFr: 'Multiplicateur du prix d’un objet accordant ce skill; sans rapport avec le mana ou l’apprentissage.',
    family: 'item_economy',
    technicalOnly: true,
    protected: false,
    decisionScope: 'NO_SKILL_DECISION',
    groupId: 'ITEM_ECONOMY',
  }),
  itemeffect: Object.freeze({
    playerLabelFr: 'Exécution objet — serveur',
    shortHelpFr: 'Fonction serveur utilisée lorsque le skill est déclenché par un objet.',
    family: 'item_trigger',
    technicalOnly: false,
    protected: true,
    decisionScope: 'BEHAVIOR_BUNDLE',
    groupId: 'ITEM_TRIGGER_EXECUTION',
  }),
  itemclteffect: Object.freeze({
    playerLabelFr: 'Exécution objet — client',
    shortHelpFr: 'Fonction client utilisée lorsque le skill est déclenché par un objet.',
    family: 'item_trigger',
    technicalOnly: false,
    protected: true,
    decisionScope: 'BEHAVIOR_BUNDLE',
    groupId: 'ITEM_TRIGGER_EXECUTION',
  }),
  delay: Object.freeze({
    playerLabelFr: 'Délai PD2',
    shortHelpFr: 'Champ propre au schéma PD2; aucune traduction automatique vers les délais D2R.',
    family: 'timing',
    technicalOnly: false,
    protected: true,
    decisionScope: 'GLOBAL_POLICY',
    groupId: 'COOLDOWN_MODEL',
  }),
  localdelay: Object.freeze({
    playerLabelFr: 'Délai local D2R',
    shortHelpFr: 'Délai de relance du skill lui-même, en frames D2R.',
    family: 'timing',
    technicalOnly: false,
    protected: true,
    decisionScope: 'BEHAVIOR_BUNDLE',
    groupId: 'COOLDOWN_MODEL',
  }),
  globaldelay: Object.freeze({
    playerLabelFr: 'Délai global D2R',
    shortHelpFr: 'Délai imposé aux autres skills à délai après l’utilisation.',
    family: 'timing',
    technicalOnly: false,
    protected: true,
    decisionScope: 'BEHAVIOR_BUNDLE',
    groupId: 'COOLDOWN_MODEL',
  }),
  perdelay: Object.freeze({
    playerLabelFr: 'Cadence périodique',
    shortHelpFr: 'Fréquence d’exécution d’un skill périodique ou d’une aura.',
    family: 'timing',
    technicalOnly: false,
    protected: true,
    decisionScope: 'BEHAVIOR_BUNDLE',
    groupId: 'COOLDOWN_MODEL',
  }),
});

export function semanticBlank(value) {
  return value === null
    || value === undefined
    || String(value).trim() === '';
}

export function canonicalFieldHeader(value) {
  const header = String(value ?? '').trim().toLowerCase();
  return header === 'id' || header === '*id' ? 'id' : header;
}

export function rawValueState(columnPresent, value) {
  if (!columnPresent) return 'ABSENT_COLUMN';
  if (value === null || value === undefined) return 'NULL_VALUE';
  if (String(value) === '') return 'EMPTY_STRING';
  if (String(value).trim() === '0') return 'ZERO';
  return 'VALUE';
}

export function bundleForHeader(header) {
  const canonical = canonicalFieldHeader(header);
  return DECISION_BUNDLES.find((bundle) => bundle.fieldPatterns.some((pattern) => new RegExp(pattern, 'i').test(canonical))) ?? null;
}

export const FROZEN_ORIENTATION_CONTRACT_HASH = sha256Canonical({
  ORIENTATION_SCHEMA_VERSION,
  ORIENTATION_ID,
  ORIENTATION_PRODUCT_NAME,
  POLICY_SCHEMA_VERSION,
  POLICY_KIND,
  POLICY_STORAGE_PREFIX,
  SOURCE_KEYS,
  RAW_VALUE_STATES,
  SCHEMA_CLASSIFICATIONS,
  DECISION_SCOPES,
  POLICY_DECISIONS,
  POLICY_DECISION_ENTRY_CONTRACT,
  GLOBAL_SCHEMA_POLICIES,
  DECISION_BUNDLES,
  MECHANICAL_CONTRACTS,
  NON_MUTATION_RULES,
  ORIENTATION_INTERFACES,
  KEY_FIELD_PRESENTATION,
  semanticBlank: 'value === null || value === undefined || String(value).trim() === ""',
  canonicalId: 'Id and *Id share documentary canonical header id',
});
