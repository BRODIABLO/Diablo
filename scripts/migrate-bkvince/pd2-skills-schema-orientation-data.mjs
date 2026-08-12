import {
  DECISION_BUNDLES,
  DECISION_SCOPES,
  FROZEN_ORIENTATION_CONTRACT_HASH,
  GLOBAL_SCHEMA_POLICIES,
  KEY_FIELD_PRESENTATION,
  MECHANICAL_CONTRACTS,
  NON_MUTATION_RULES,
  ORIENTATION_ID,
  ORIENTATION_INTERFACES,
  ORIENTATION_PRODUCT_NAME,
  ORIENTATION_SCHEMA_VERSION,
  POLICY_DECISIONS,
  POLICY_DECISION_ENTRY_CONTRACT,
  POLICY_KIND,
  POLICY_SCHEMA_VERSION,
  RAW_VALUE_STATES,
  SCHEMA_CLASSIFICATIONS,
  SOURCE_KEYS,
  bundleForHeader,
  canonicalFieldHeader,
  rawValueState,
  semanticBlank,
} from './pd2-skills-schema-orientation-contracts.mjs';
import { sha256Canonical } from './pd2-skills-review-contracts.mjs';

const SOURCE_LABELS = Object.freeze({
  vanilla32: 'Vanilla D2R 3.2',
  bkvince: 'BKVince HEAD',
  pd2: 'PD2 / Single Player+',
});

const PD2_ONLY_HEADERS = Object.freeze([
  'auratgtevent',
  'auratgteventfunc',
  'checkfunc',
  'delay',
  'general',
  'nocostinstate',
  'passiveevent',
  'passiveeventfunc',
]);

const ITEM_ECONOMY_PATTERN = /^cost (?:add|mult)$/i;
const DOCUMENTARY_PATTERN = /^\*/;
const NATIVE_FUNCTION_PATTERN = /^(?:(?:srv|clt).*(?:func|function)\d*|hitfunc\d*|item(?:clt)?effect|checkfunc)$/i;
const DAMAGE_PATTERN = /^(?:srcdam|hitshift|mindam|maxdam|minlevdam[1-5]|maxlevdam[1-5]|etype|emin|emax|eminlev[1-5]|emaxlev[1-5]|elen|elevlen[1-3]|dmgsympercalc|edmgsympercalc|elensympercalc|tohit|levtohit|tohitcalc)$/i;
const MANA_PATTERN = /^(?:mana|lvlmana|manashift|minmana|startmana)$/i;
const PROJECTILE_PATTERN = /missile/i;
const AURA_PASSIVE_PATTERN = /^(?:aura|passive|state|periodic|event)/i;
const SUMMON_PATTERN = /^(?:summon|pet|sumskill|sumsk|requirespet)/i;
const UI_PATTERN = /^(?:skilldesc|leftskill|rightskill|keepcursorstateonkill|continuecastunselected|clearselectedonhold|scroll|icon)/i;
const IDENTITY_PATTERN = /^(?:skill|id|charclass|maxlvl|reqlevel|reqskill[1-3]|reqstr|reqdex|reqint|reqvit)$/i;
const CALC_PARAM_PATTERN = /^(?:calc(?:[1-9]|10)|param(?:[1-9]|1\d|20)|.*calc)$/i;
const SOUND_VISUAL_PATTERN = /(?:sound|overlay|anim|seq)/i;
const TARGETING_PATTERN = /(?:target|search|lineofsight|range|select|restrict|itype|etype)/i;
const PLAYER_FIELD_PATTERN = new RegExp([
  DAMAGE_PATTERN.source,
  MANA_PATTERN.source,
  PROJECTILE_PATTERN.source,
  AURA_PASSIVE_PATTERN.source,
  SUMMON_PATTERN.source,
  UI_PATTERN.source,
  /^(?:delay|localdelay|globaldelay|perdelay|interrupt|repeat)$/.source,
].map((item) => `(?:${item})`).join('|'), 'i');

function clone(value) {
  return JSON.parse(JSON.stringify(value));
}

function headerIndex(document) {
  return new Map(document.table.headers.map((header, index) => [canonicalFieldHeader(header), index]));
}

function sourceDocument(sources, source, table = 'skills.txt') {
  const document = sources?.documents?.[source]?.[table];
  if (!document?.table?.headers || !document?.table?.rows) {
    throw new Error(`Missing governed ${source} ${table} document`);
  }
  return document;
}

function sourcePlayerOrdinals(skillReport) {
  const result = Object.fromEntries(SOURCE_KEYS.map((source) => [source, new Set()]));
  for (const skill of skillReport?.skills ?? []) {
    if (!skill.playerSkill) continue;
    for (const source of SOURCE_KEYS) {
      const ordinal = skill.ordinals?.[source];
      if (Number.isInteger(ordinal) && ordinal >= 0) result[source].add(ordinal);
    }
  }
  return result;
}

function findSkillRecord(document, skillName) {
  const wanted = String(skillName).trim().toLowerCase();
  return document.records?.find((record) => String(record.get?.('skill') ?? record.key ?? '').trim().toLowerCase() === wanted)
    ?? null;
}

function valueFor(document, record, canonicalHeader) {
  const indexes = headerIndex(document);
  const index = indexes.get(canonicalHeader);
  if (index === undefined) return { present: false, rawHeader: null, rawValue: undefined };
  return {
    present: true,
    rawHeader: document.table.headers[index],
    rawValue: record?.row?.[index],
  };
}

export function buildCellEvidence(source, document, record, header) {
  const canonicalHeader = canonicalFieldHeader(header);
  const value = valueFor(document, record, canonicalHeader);
  return {
    source,
    columnPresent: value.present,
    rawHeader: value.rawHeader,
    rawValue: value.present && value.rawValue !== undefined ? value.rawValue : null,
    rawState: rawValueState(value.present, value.rawValue),
    semanticBlank: semanticBlank(value.rawValue),
  };
}

function semanticToken(evidence) {
  return evidence.semanticBlank ? 'SEMANTIC_BLANK' : String(evidence.rawValue).trim();
}

function compareEvidence(evidenceBySource, field = null) {
  const evidence = SOURCE_KEYS.map((source) => evidenceBySource[source]);
  const rawTokens = evidence.map((item) => `${item.rawState}\u0000${item.rawValue ?? ''}`);
  const semanticTokens = evidence.map(semanticToken);
  const rawChanged = new Set(rawTokens).size > 1;
  const semanticChanged = new Set(semanticTokens).size > 1;
  const pd2 = evidenceBySource.pd2;
  const bkvince = evidenceBySource.bkvince;
  const vanilla = evidenceBySource.vanilla32;
  let decisionRelevant = false;
  let semanticDifferenceReason = 'NO_DIFFERENCE';

  if (evidence.every((item) => item.semanticBlank)) {
    semanticDifferenceReason = rawChanged ? 'RAW_ONLY_ALL_SEMANTIC_BLANK' : 'ALL_SEMANTIC_BLANK';
  } else if (!pd2.columnPresent && !bkvince.semanticBlank) {
    semanticDifferenceReason = 'PD2_HEADER_ABSENT_PRESERVE_D2R_BKVINCE';
  } else if (semanticToken(pd2) === semanticToken(bkvince)) {
    semanticDifferenceReason = semanticToken(vanilla) === semanticToken(bkvince)
      ? 'THREE_WAY_SEMANTICALLY_EQUAL'
      : 'VANILLA_ONLY_HISTORICAL_DIFFERENCE';
  } else if (field?.decisionScope === 'GLOBAL_POLICY') {
    semanticDifferenceReason = 'GLOBAL_PORTABILITY_GATE_PRECEDES_SKILL_DECISION';
  } else if (field?.primaryClassification === 'ITEM_ECONOMY_ONLY') {
    semanticDifferenceReason = 'AUTO_RESOLVED_ITEM_ECONOMY';
  } else if (field?.decisionScope === 'NO_SKILL_DECISION') {
    semanticDifferenceReason = 'AUTO_RESOLVED_TECHNICAL_OR_DOCUMENTARY';
  } else {
    decisionRelevant = true;
    semanticDifferenceReason = 'PLAYER_BEHAVIOR_DIFFERS';
  }

  return { rawChanged, semanticChanged, decisionRelevant, semanticDifferenceReason };
}

function familyForHeader(header) {
  if (ITEM_ECONOMY_PATTERN.test(header)) return 'item_economy';
  if (NATIVE_FUNCTION_PATTERN.test(header)) return 'engine_functions';
  if (/^(?:delay|localdelay|globaldelay|perdelay)$/.test(header)) return 'cooldowns';
  if (MANA_PATTERN.test(header)) return 'mana';
  if (DAMAGE_PATTERN.test(header)) return 'damage_curves';
  if (PROJECTILE_PATTERN.test(header)) return 'projectiles';
  if (AURA_PASSIVE_PATTERN.test(header)) return 'auras_passives';
  if (SUMMON_PATTERN.test(header)) return 'summons';
  if (UI_PATTERN.test(header)) return 'interface_controls';
  if (IDENTITY_PATTERN.test(header)) return 'identity_availability';
  if (CALC_PARAM_PATTERN.test(header)) return 'calc_param';
  if (SOUND_VISUAL_PATTERN.test(header)) return 'animation_audio_visuals';
  if (TARGETING_PATTERN.test(header)) return 'targeting_restrictions';
  if (DOCUMENTARY_PATTERN.test(header)) return 'documentation';
  return 'raw_technical';
}

function numberedLabel(header, pattern, base) {
  const match = header.match(pattern);
  return match ? `${base} — palier ${match[1]}` : null;
}

function playerPresentation(header, family, documented) {
  const key = KEY_FIELD_PRESENTATION[header];
  if (key) return clone(key);
  const numbered = numberedLabel(header, /^eminlev([1-5])$/, 'Dégâts élémentaires minimum')
    ?? numberedLabel(header, /^emaxlev([1-5])$/, 'Dégâts élémentaires maximum')
    ?? numberedLabel(header, /^minlevdam([1-5])$/, 'Dégâts physiques minimum')
    ?? numberedLabel(header, /^maxlevdam([1-5])$/, 'Dégâts physiques maximum')
    ?? numberedLabel(header, /^elevlen([1-3])$/, 'Durée élémentaire')
    ?? numberedLabel(header, /^aurastat([1-6])$/, 'Stat d’aura')
    ?? numberedLabel(header, /^aurastatcalc([1-6])$/, 'Valeur de stat d’aura')
    ?? numberedLabel(header, /^passivestat(\d+)$/, 'Stat passive')
    ?? numberedLabel(header, /^passivecalc(\d+)$/, 'Valeur de stat passive');
  if (numbered) {
    return {
      playerLabelFr: numbered,
      shortHelpFr: `Cellule constitutive du bundle ${family}; elle ne se décide pas isolément.`,
    };
  }

  const exact = {
    skill: ['Nom interne du skill', 'Identité textuelle utilisée par les tables liées.'],
    id: ['ID documentaire', 'Le runtime utilise l’ordinal réel de la ligne, jamais cette cellule documentaire.'],
    charclass: ['Classe', 'Classe à laquelle le skill joueur est rattaché.'],
    skilldesc: ['Présentation du skill', 'Lien vers le tooltip, l’icône et la position réelle dans l’arbre.'],
    maxlvl: ['Niveau maximum', 'Plafond de points du skill dans le schéma cible.'],
    reqlevel: ['Niveau requis', 'Niveau de personnage requis pour apprendre le skill.'],
    mana: ['Mana de base', 'Base encodée de la courbe de coût en mana.'],
    lvlmana: ['Mana par niveau', 'Progression du coût en mana avec le niveau du skill.'],
    manashift: ['Échelle du mana', 'Décalage binaire appliqué à la courbe de mana.'],
    minmana: ['Mana minimum', 'Plancher du coût réel en mana.'],
    startmana: ['Mana initial', 'Coût initial utilisé par certains skills continus.'],
    hitshift: ['Échelle des dégâts', 'Décalage binaire indissociable de la courbe de dégâts.'],
    emin: ['Dégâts élémentaires minimum', 'Base minimum de la courbe élémentaire.'],
    emax: ['Dégâts élémentaires maximum', 'Base maximum de la courbe élémentaire.'],
    mindam: ['Dégâts physiques minimum', 'Base minimum de la courbe physique.'],
    maxdam: ['Dégâts physiques maximum', 'Base maximum de la courbe physique.'],
    edmgsympercalc: ['Synergies de dégâts élémentaires', 'Formule qui relie la courbe élémentaire aux skills sources.'],
    dmgsympercalc: ['Synergies de dégâts physiques', 'Formule qui relie la courbe physique aux skills sources.'],
    elensympercalc: ['Synergies de durée', 'Formule qui prolonge la durée élémentaire.'],
    srvmissile: ['Projectile serveur principal', 'Missile créé par la logique serveur.'],
    cltmissile: ['Projectile client principal', 'Missile affiché par la logique client.'],
    auralencalc: ['Durée de l’aura ou du buff', 'Formule de durée, généralement exprimée en frames.'],
    aurarangecalc: ['Rayon de l’aura', 'Formule de rayon exprimée en sous-tuiles.'],
    pettype: ['Type de familier', 'Lien vers le package pettype du summon.'],
    petmax: ['Nombre maximum de summons', 'Formule déterminant la limite de familiers.'],
    summon: ['Monstre invoqué', 'Lien vers la ligne monstats du summon.'],
  }[header];
  if (exact) return { playerLabelFr: exact[0], shortHelpFr: exact[1] };

  const calc = header.match(/^calc(\d+)$/);
  if (calc) return {
    playerLabelFr: `Calcul ${calc[1]}`,
    shortHelpFr: 'Le sens dépend du callback; utiliser en priorité la description propre à la ligne.',
  };
  const param = header.match(/^param(\d+)$/);
  if (param) return {
    playerLabelFr: `Paramètre ${param[1]}`,
    shortHelpFr: 'Le sens dépend du callback et de la description propre au skill.',
  };
  if (NATIVE_FUNCTION_PATTERN.test(header)) return {
    playerLabelFr: `Fonction moteur — ${header}`,
    shortHelpFr: 'Sélecteur natif protégé; un numéro identique entre moteurs ne prouve pas le même comportement.',
  };
  if (DOCUMENTARY_PATTERN.test(header)) return {
    playerLabelFr: `Documentation — ${header.replace(/^\*/, '')}`,
    shortHelpFr: 'Texte documentaire conservé dans les détails techniques, sans décision de gameplay.',
  };
  return {
    playerLabelFr: header,
    shortHelpFr: documented
      ? `Champ ${family.replaceAll('_', ' ')} documenté par eezstreet; consulter le détail technique.`
      : `Champ ${family.replaceAll('_', ' ')} sans consumer D2R 3.2 prouvé.`,
  };
}

function consumerFor(header, family) {
  const exact = {
    item_economy: 'Calcul de la valeur d’achat, de vente et de réparation des objets accordant le skill.',
    engine_functions: 'Dispatch natif serveur/client; le sens exact dépend du numéro de fonction et du moteur.',
    cooldowns: 'Cadence et délais de relance du skill.',
    mana: 'Calcul du coût réel en mana.',
    damage_curves: 'Calcul des dégâts, paliers, durée élémentaire et synergies.',
    projectiles: 'Fonctions du skill et lignes liées de missiles.txt.',
    auras_passives: 'Système d’états, d’événements et de stats d’aura/passif.',
    summons: 'Système de pets avec pettype.txt et monstats.txt.',
    interface_controls: 'Interface des skills, assignation des boutons et skilldesc.txt.',
    identity_availability: 'Compilateur de données et registre ordinal des skills.',
    calc_param: 'Fonction native sélectionnée par la ligne; consumer variable selon le callback.',
    animation_audio_visuals: 'Systèmes client d’animation, de son ou d’overlay.',
    targeting_restrictions: 'Validation de cible, équipement et restrictions d’utilisation.',
    documentation: 'Aucun consumer gameplay; colonne de commentaire/documentation.',
  }[family];
  return exact ?? `Consumer D2R 3.2 non identifié pour ${header}.`;
}

function isTechnicalOnly(header, family) {
  if (header === 'id' || ITEM_ECONOMY_PATTERN.test(header) || DOCUMENTARY_PATTERN.test(header)) return true;
  if (NATIVE_FUNCTION_PATTERN.test(header) || CALC_PARAM_PATTERN.test(header)) return true;
  if (family === 'raw_technical' || family === 'animation_audio_visuals') return true;
  return !PLAYER_FIELD_PATTERN.test(header) && !IDENTITY_PATTERN.test(header) && !TARGETING_PATTERN.test(header);
}

function isProtected(header, family) {
  return header === 'id'
    || header === 'charclass'
    || header === 'maxlvl'
    || /^(?:delay|localdelay|globaldelay|perdelay)$/.test(header)
    || NATIVE_FUNCTION_PATTERN.test(header)
    || family === 'engine_functions';
}

function classifyColumn(header, presence, usage) {
  const classifications = [];
  const d2rPresent = presence.vanilla32 || presence.bkvince;
  const pd2Only = presence.pd2 && !d2rPresent;
  const total = usage.totalNonBlankCells;
  if (presence.vanilla32) classifications.push('D2R_NATIVE_PRESERVE');
  if (presence.bkvince && !presence.vanilla32) classifications.push('BKVINCE_EXTENSION_PRESERVE');
  if (pd2Only && total === 0) classifications.push('PD2_SCHEMA_UNUSED');
  if (pd2Only && total > 0) {
    classifications.push('PD2_SEMANTIC_SOURCE_ONLY', 'NATIVE_EXTENSION_REQUIRED', 'UNKNOWN_NATIVE_CONSUMER');
  }
  if (ITEM_ECONOMY_PATTERN.test(header)) classifications.push('ITEM_ECONOMY_ONLY');
  if (header === 'id' || DOCUMENTARY_PATTERN.test(header)) classifications.push('DOCUMENTARY_ONLY');
  if (UI_PATTERN.test(header)) classifications.push('UI_ONLY');
  if (NATIVE_FUNCTION_PATTERN.test(header) || CALC_PARAM_PATTERN.test(header)) classifications.push('RAW_TECHNICAL_OVERRIDE');
  if (!classifications.length) classifications.push('UNKNOWN_NATIVE_CONSUMER');
  return [...new Set(classifications)];
}

function primaryClassification(classifications) {
  const priority = [
    'PD2_SCHEMA_UNUSED',
    'NATIVE_EXTENSION_REQUIRED',
    'BKVINCE_EXTENSION_PRESERVE',
    'ITEM_ECONOMY_ONLY',
    'DOCUMENTARY_ONLY',
    'UI_ONLY',
    'RAW_TECHNICAL_OVERRIDE',
    'D2R_NATIVE_PRESERVE',
    'PD2_SEMANTIC_SOURCE_ONLY',
    'UNKNOWN_NATIVE_CONSUMER',
  ];
  return priority.find((value) => classifications.includes(value)) ?? classifications[0];
}

function defaultPolicyFor(header, primary, decisionScope, usage) {
  if (primary === 'PD2_SCHEMA_UNUSED') return 'IGNORE_ALL_EMPTY_PD2_ONLY_COLUMNS';
  if (header === 'delay') return 'NO_AUTOMATIC_DELAY_TRANSLATION';
  if (primary === 'NATIVE_EXTENSION_REQUIRED') return 'NO_PD2_HEADER_IMPORT_WITHOUT_NATIVE_PROOF';
  if (primary === 'ITEM_ECONOMY_ONLY') return 'ITEM_ECONOMY_FIELDS_PRESERVE_BKVINCE';
  if (NATIVE_FUNCTION_PATTERN.test(header)) return 'NATIVE_CALLBACKS_PRESERVE_OR_DEFER';
  if (usage.totalNonBlankCells === 0) return 'SEMANTIC_BLANKS_REQUIRE_NO_DECISION';
  if (decisionScope === 'BEHAVIOR_BUNDLE' || decisionScope === 'EXPERT_OVERRIDE_ONLY') return 'RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE';
  return 'PRESERVE_ALL_D2R_BKVINCE_COLUMNS';
}

function potentialEquivalentFor(header, presence) {
  if (header === 'delay') return {
    fields: ['localdelay', 'globaldelay', 'perdelay'],
    relation: 'POTENTIAL_INTENT_OVERLAP_NOT_FIELD_EQUIVALENCE',
    proofStatus: 'NATIVE_UNPROVEN',
    automaticMappingAllowed: false,
  };
  if (header === 'checkfunc') return {
    fields: [], relation: 'NO_D2R_3_2_HEADER_OR_CONSUMER_PROVEN', proofStatus: 'NATIVE_UNPROVEN', automaticMappingAllowed: false,
  };
  if (presence.vanilla32 && presence.bkvince && presence.pd2) return {
    fields: [header],
    relation: 'SAME_CANONICAL_HEADER',
    proofScope: 'SCHEMA_CONCEPT_ONLY',
    proofStatus: 'EXACT_TABLE',
    valueSemanticsProven: !NATIVE_FUNCTION_PATTERN.test(header) && !CALC_PARAM_PATTERN.test(header),
    automaticMappingAllowed: !NATIVE_FUNCTION_PATTERN.test(header) && !CALC_PARAM_PATTERN.test(header),
  };
  return {
    fields: [],
    relation: presence.pd2 ? 'NO_PROVEN_TARGET_EQUIVALENT' : 'PD2_HEADER_ABSENT_PRESERVE_TARGET',
    proofStatus: presence.pd2 ? 'NATIVE_UNPROVEN' : 'EXACT_TABLE',
    automaticMappingAllowed: false,
  };
}

function decisionScopeFor(header, classifications, bundle, usage, presence) {
  if (usage.totalNonBlankCells === 0) return 'NO_SKILL_DECISION';
  if (header === 'id') return 'NO_SKILL_DECISION';
  if (!presence.pd2 && (presence.vanilla32 || presence.bkvince)) return 'NO_SKILL_DECISION';
  if (classifications.includes('PD2_SCHEMA_UNUSED')) return 'NO_SKILL_DECISION';
  if (classifications.includes('NATIVE_EXTENSION_REQUIRED')) return 'GLOBAL_POLICY';
  if (classifications.includes('ITEM_ECONOMY_ONLY') || classifications.includes('DOCUMENTARY_ONLY')) return 'NO_SKILL_DECISION';
  if (bundle) return 'BEHAVIOR_BUNDLE';
  if (classifications.includes('RAW_TECHNICAL_OVERRIDE')) return 'EXPERT_OVERRIDE_ONLY';
  return 'EXPERT_OVERRIDE_ONLY';
}

function documentedColumns(schemaDocument) {
  const columns = schemaDocument?.columns ?? [];
  return new Map(columns.map((column) => [canonicalFieldHeader(column.name), column]));
}

function usageForColumn(documents, indexes, playerOrdinals, canonicalHeader) {
  const bySource = {};
  for (const source of SOURCE_KEYS) {
    const document = documents[source];
    const index = indexes[source].get(canonicalHeader);
    const present = index !== undefined;
    const playerExamples = [];
    const technicalExamples = [];
    let nonBlankCells = 0;
    let playerSkills = 0;
    let technicalOrMonsterRows = 0;
    let emptyCells = 0;
    let nullCells = 0;
    let zeroCells = 0;
    if (present) {
      document.table.rows.forEach((row, ordinal) => {
        const value = row[index];
        const state = rawValueState(true, value);
        if (state === 'EMPTY_STRING' || (state === 'VALUE' && semanticBlank(value))) emptyCells += 1;
        else if (state === 'NULL_VALUE') nullCells += 1;
        if (!semanticBlank(value)) {
          nonBlankCells += 1;
          if (state === 'ZERO') zeroCells += 1;
          if (playerOrdinals[source].has(ordinal)) playerSkills += 1;
          else technicalOrMonsterRows += 1;
          const playerSkill = playerOrdinals[source].has(ordinal);
          const bucket = playerSkill ? playerExamples : technicalExamples;
          if (bucket.length < 3) {
            const record = document.records?.[ordinal];
            bucket.push({
              ordinal,
              skill: String(record?.get?.('skill') ?? row[0] ?? ''),
              value: String(value),
              playerSkill,
            });
          }
        }
      });
    }
    bySource[source] = {
      rows: document.table.rows.length,
      nonBlankCells,
      playerSkills,
      technicalOrMonsterRows,
      emptyCells,
      nullCells,
      zeroCells,
      examples: [...playerExamples, ...technicalExamples],
    };
  }
  return {
    totalNonBlankCells: SOURCE_KEYS.reduce((total, source) => total + bySource[source].nonBlankCells, 0),
    playerSkills: SOURCE_KEYS.reduce((total, source) => total + bySource[source].playerSkills, 0),
    technicalOrMonsterRows: SOURCE_KEYS.reduce((total, source) => total + bySource[source].technicalOrMonsterRows, 0),
    bySource,
  };
}

function buildColumns(sources, skillReport, schemaDocument) {
  const documents = Object.fromEntries(SOURCE_KEYS.map((source) => [source, sourceDocument(sources, source)]));
  const indexes = Object.fromEntries(SOURCE_KEYS.map((source) => [source, headerIndex(documents[source])]));
  const canonicalHeaders = [];
  const seen = new Set();
  for (const source of SOURCE_KEYS) {
    for (const rawHeader of documents[source].table.headers) {
      const canonical = canonicalFieldHeader(rawHeader);
      if (!seen.has(canonical)) {
        seen.add(canonical);
        canonicalHeaders.push(canonical);
      }
    }
  }
  const playerOrdinals = sourcePlayerOrdinals(skillReport);
  const docs = documentedColumns(schemaDocument);

  return canonicalHeaders.map((canonicalHeader, ordinal) => {
    const rawHeaders = Object.fromEntries(SOURCE_KEYS.map((source) => {
      const index = indexes[source].get(canonicalHeader);
      return [source, index === undefined ? null : documents[source].table.headers[index]];
    }));
    const presence = Object.fromEntries(SOURCE_KEYS.map((source) => [source, rawHeaders[source] !== null]));
    const usage = usageForColumn(documents, indexes, playerOrdinals, canonicalHeader);
    const documentationEntry = docs.get(canonicalHeader) ?? null;
    const documentedByGuide = Boolean(documentationEntry?.documentationSource);
    const family = familyForHeader(canonicalHeader);
    const presentation = playerPresentation(canonicalHeader, family, documentedByGuide);
    const classifications = classifyColumn(canonicalHeader, presence, usage);
    const primary = primaryClassification(classifications);
    const bundle = bundleForHeader(canonicalHeader);
    const decisionScope = decisionScopeFor(canonicalHeader, classifications, bundle, usage, presence);
    const rawChanged = new Set(SOURCE_KEYS.map((source) => presence[source])).size > 1;
    const semanticChanged = rawChanged && SOURCE_KEYS.some((source) => presence[source] && usage.bySource[source].nonBlankCells > 0);
    const decisionRelevant = semanticChanged && decisionScope === 'GLOBAL_POLICY';
    const semanticDifferenceReason = !rawChanged
      ? 'SAME_SCHEMA_PRESENCE'
      : !semanticChanged
        ? 'SCHEMA_PRESENCE_DIFFERS_BUT_ALL_PRESENT_CELLS_SEMANTIC_BLANK'
        : decisionScope === 'GLOBAL_POLICY'
          ? 'SOURCE_ONLY_VALUES_REQUIRE_GLOBAL_PORTABILITY_GATE'
          : 'TARGET_SCHEMA_COLUMN_PRESERVED_WITHOUT_PD2_CELL_DECISIONS';
    const technicalOnly = isTechnicalOnly(canonicalHeader, family);
    const protectedField = isProtected(canonicalHeader, family);
    const proofStatus = classifications.includes('NATIVE_EXTENSION_REQUIRED')
      ? 'NATIVE_UNPROVEN'
      : documentedByGuide
        ? 'DOCUMENTED'
        : 'EXACT_TABLE';
    return {
      id: `skills.txt:${canonicalHeader}`,
      ordinal,
      canonicalHeader,
      rawHeaders,
      presence,
      usage,
      examples: Object.fromEntries(SOURCE_KEYS.map((source) => [source, usage.bySource[source].examples])),
      documentation: documentationEntry ? {
        source: 'schemas/skills.json',
        descriptionEn: documentationEntry.description ?? '',
        guideType: documentationEntry.guideType ?? null,
        guideUrl: documentationEntry.guideUrl ?? null,
        documentationSource: documentationEntry.documentationSource ?? null,
        status: documentedByGuide ? 'DOCUMENTED' : 'HEADER_ONLY',
      } : {
        source: null,
        descriptionEn: null,
        guideType: null,
        guideUrl: null,
        documentationSource: null,
        status: presence.pd2 && !presence.vanilla32 && !presence.bkvince ? 'PD2_SCHEMA_ONLY' : 'UNMAPPED',
      },
      playerLabelFr: presentation.playerLabelFr,
      shortHelpFr: presentation.shortHelpFr,
      family,
      consumer: consumerFor(canonicalHeader, family),
      classifications,
      primaryClassification: primary,
      potentialEquivalent: potentialEquivalentFor(canonicalHeader, presence),
      decisionScope,
      defaultPolicy: defaultPolicyFor(canonicalHeader, primary, decisionScope, usage),
      technicalOnly,
      protected: protectedField,
      groupId: bundle?.id ?? (ITEM_ECONOMY_PATTERN.test(canonicalHeader) ? 'ITEM_ECONOMY' : null),
      proofStatus,
      rawChanged,
      semanticChanged,
      decisionRelevant,
      semanticDifferenceReason,
    };
  });
}

function policyDefinitions() {
  return GLOBAL_SCHEMA_POLICIES.map((definition) => ({
    ...clone(definition),
    fingerprint: sha256Canonical(definition),
    decision: 'PENDING',
    justification: '',
  }));
}

function sourceEvidenceForSkill(sources, skillName, header, field = null, table = 'skills.txt') {
  const evidence = {};
  for (const source of SOURCE_KEYS) {
    const document = sourceDocument(sources, source, table);
    const record = findSkillRecord(document, skillName);
    evidence[source] = buildCellEvidence(source, document, record, header);
  }
  return { header: canonicalFieldHeader(header), evidence, ...compareEvidence(evidence, field) };
}

function fieldByHeader(columns) {
  return new Map(columns.map((column) => [column.canonicalHeader, column]));
}

function fireBoltDynamicBundle(header, evidence, sources) {
  const regular = bundleForHeader(header);
  if (regular) return regular.id;
  const param = header.match(/^param(\d+)$/i);
  const calc = header.match(/^calc(\d+)$/i);
  if (!param && !calc) return null;
  const number = (param ?? calc)[1];
  const descriptionCandidates = param
    ? [`*param${number} description`, `*param${number}description`, `*param${number} description2`]
    : [`*calc${number} desc`, `*calc${number}desc`];
  for (const source of SOURCE_KEYS) {
    const document = sourceDocument(sources, source);
    const record = findSkillRecord(document, 'Fire Bolt');
    for (const candidate of descriptionCandidates) {
      const value = valueFor(document, record, canonicalFieldHeader(candidate));
      if (value.present && /damage synergy/i.test(String(value.rawValue ?? ''))) return 'DAMAGE_SYNERGIES';
    }
  }
  return null;
}

function linkedFireBoltMissileDifferences(sources) {
  const skillMissileFields = ['srvmissile', 'srvmissilea', 'srvmissileb', 'srvmissilec', 'cltmissile', 'cltmissilea', 'cltmissileb', 'cltmissilec', 'cltmissiled'];
  const pairs = new Map();
  for (const source of ['bkvince', 'pd2']) {
    const skills = sourceDocument(sources, source);
    const record = findSkillRecord(skills, 'Fire Bolt');
    for (const field of skillMissileFields) {
      const value = valueFor(skills, record, field);
      if (!semanticBlank(value.rawValue)) {
        const key = String(value.rawValue).trim().toLowerCase();
        if (!pairs.has(key)) pairs.set(key, key);
      }
    }
  }
  const results = [];
  for (const missileKey of pairs.keys()) {
    const records = {};
    const documents = {};
    for (const source of SOURCE_KEYS) {
      const document = sourceDocument(sources, source, 'missiles.txt');
      documents[source] = document;
      records[source] = document.records?.find((record) => String(record.key ?? '').trim().toLowerCase() === missileKey) ?? null;
    }
    const headers = [];
    const seen = new Set();
    for (const source of SOURCE_KEYS) {
      for (const rawHeader of documents[source].table.headers) {
        const canonical = canonicalFieldHeader(rawHeader);
        if (!seen.has(canonical)) {
          seen.add(canonical);
          headers.push(canonical);
        }
      }
    }
    for (const header of headers) {
      const evidence = Object.fromEntries(SOURCE_KEYS.map((source) => [source, buildCellEvidence(source, documents[source], records[source], header)]));
      const comparison = compareEvidence(evidence);
      if (semanticToken(evidence.pd2) === semanticToken(evidence.bkvince)) continue;
      if (evidence.pd2.semanticBlank && evidence.bkvince.semanticBlank) continue;
      let groupId = null;
      if (/^(?:vel|maxvel|range|size|collidekill|pierce|nexthit|nextdelay)$/i.test(header)) groupId = 'PROJECTILE_PHYSICS';
      else if (/func|^s(?:hit|dmg|clt)par\d+$/i.test(header)) groupId = 'NATIVE_EXECUTION';
      if (!groupId) continue;
      results.push({
        id: `missiles.txt:${missileKey}:${header}`,
        table: 'missiles.txt',
        rowKey: missileKey,
        header,
        groupId,
        evidence,
        ...comparison,
      });
    }
  }
  return results;
}

function buildFireBoltImpact(sources, skillReport, columns) {
  const skill = (skillReport?.skills ?? []).find((entry) => entry.canonicalName === 'Fire Bolt');
  if (!skill) throw new Error('The governed skill report does not contain Fire Bolt');
  const changedFields = skill.components.flatMap((component) => component.fields.filter((field) => field.changed));
  const columnsByHeader = fieldByHeader(columns);
  const reductions = {
    semanticBlank: [],
    preserveD2rColumnAbsentFromPd2: [],
    vanillaHistoricalOnly: [],
    technicalOrDocumentary: [],
    bundled: [],
  };
  const projections = new Map();
  const addProjection = (groupId, item) => {
    if (!projections.has(groupId)) projections.set(groupId, []);
    projections.get(groupId).push(item);
  };

  for (const field of changedFields) {
    const header = canonicalFieldHeader(field.header);
    const dictionaryField = columnsByHeader.get(header) ?? null;
    const comparison = sourceEvidenceForSkill(sources, 'Fire Bolt', header, dictionaryField);
    const item = { id: field.id, table: field.table, header, ...comparison };
    if (SOURCE_KEYS.every((source) => comparison.evidence[source].semanticBlank)) {
      reductions.semanticBlank.push(item);
      continue;
    }
    if (!comparison.evidence.pd2.columnPresent && !comparison.evidence.bkvince.semanticBlank) {
      reductions.preserveD2rColumnAbsentFromPd2.push(item);
      continue;
    }
    if (semanticToken(comparison.evidence.pd2) === semanticToken(comparison.evidence.bkvince)
      && semanticToken(comparison.evidence.vanilla32) !== semanticToken(comparison.evidence.bkvince)) {
      reductions.vanillaHistoricalOnly.push(item);
      continue;
    }
    if (dictionaryField?.decisionScope === 'NO_SKILL_DECISION') {
      reductions.technicalOrDocumentary.push(item);
      continue;
    }
    const groupId = fireBoltDynamicBundle(header, comparison.evidence, sources);
    if (!groupId) {
      reductions.technicalOrDocumentary.push({ ...item, unresolvedRouting: true });
      continue;
    }
    reductions.bundled.push({ ...item, groupId });
    addProjection(groupId, `skills.txt:${header}`);
  }

  const linkedDifferences = linkedFireBoltMissileDifferences(sources);
  for (const difference of linkedDifferences) addProjection(difference.groupId, difference.id);
  const bundleProjection = [...projections.entries()].map(([groupId, fields]) => {
    const contract = DECISION_BUNDLES.find((bundle) => bundle.id === groupId);
    return {
      groupId,
      labelFr: contract?.labelFr ?? groupId,
      protected: Boolean(contract?.protected),
      fields: [...new Set(fields)].sort(),
    };
  }).sort((left, right) => left.groupId.localeCompare(right.groupId));

  const accountedCurrentFields = Object.values(reductions).reduce((total, entries) => total + entries.length, 0);
  const rawCurrentIds = new Set(changedFields.map((field) => field.id));
  const accountedIds = new Set(Object.values(reductions).flat().map((item) => item.id));
  return {
    skillStableId: skill.stableId,
    skillName: 'Fire Bolt',
    currentModifiedFields: changedFields.length,
    currentRequiredDecisions: changedFields.length + 1,
    currentDecisionModel: 'ONE_GLOBAL_PLUS_ONE_PER_CHANGED_RAW_FIELD',
    reductions: Object.fromEntries(Object.entries(reductions).map(([key, entries]) => [key, {
      count: entries.length,
      fields: entries.map((item) => `${item.table}:${item.header}`),
      reasons: [...new Set(entries.map((item) => item.semanticDifferenceReason).filter(Boolean))],
    }])),
    bundleProjection,
    linkedDifferences,
    finalPlayerDecisions: bundleProjection.length,
    targetRange: { minimum: 4, maximum: 8, met: bundleProjection.length >= 4 && bundleProjection.length <= 8 },
    accounting: {
      currentFields: changedFields.length,
      accountedCurrentFields,
      unaccountedCurrentFieldIds: [...rawCurrentIds].filter((id) => !accountedIds.has(id)),
      linkedEvidenceAdded: linkedDifferences.length,
      noRelevantDifferenceHidden: accountedCurrentFields === changedFields.length,
    },
  };
}

function buildWitnesses(sources, columns, fireBoltImpact) {
  const byHeader = fieldByHeader(columns);
  const witness = (header) => byHeader.get(header);
  const emaxlev = [1, 2, 3, 4, 5].map((number) => witness(`emaxlev${number}`));
  return {
    cooldowns: {
      titleFr: 'delay / localdelay / globaldelay / perdelay',
      fields: ['delay', 'localdelay', 'globaldelay', 'perdelay'].map((header) => witness(header)),
      automaticMappingAllowed: false,
      policy: 'NO_AUTOMATIC_DELAY_TRANSLATION',
      conclusionFr: 'Les modèles sont exposés ensemble, mais aucune traduction de delay vers localdelay/globaldelay n’est autorisée sans preuve native.',
      summary: 'Aucun mapping automatique : les modèles et leurs consumers doivent être prouvés séparément.',
    },
    costAdd: {
      titleFr: 'cost add — économie des objets',
      field: witness('cost add'),
      conclusionFr: 'Ce champ influence la valeur en or des objets accordant le skill; il ne représente ni le mana ni un coût d’apprentissage.',
      skillBalanceDecision: false,
      summary: 'Influence la valeur en or d’un objet; aucune décision de balance du skill.',
    },
    itemTriggerExecution: {
      titleFr: 'itemeffect / itemclteffect',
      fields: [witness('itemeffect'), witness('itemclteffect')],
      bundleId: 'ITEM_TRIGGER_EXECUTION',
      protected: true,
      conclusionFr: 'Les deux callbacks gouvernent l’exécution objet serveur/client et forment une seule décision protégée.',
      summary: 'Exécution serveur/client déclenchée par objet, regroupée et protégée.',
    },
    elementalMaximumCurve: {
      titleFr: 'emaxlev1 à emaxlev5',
      fields: emaxlev,
      bundleId: 'ELEMENTAL_DAMAGE_CURVE',
      decisions: 1,
      conclusionFr: 'Les cinq paliers emaxlev1..5 sont constitutifs d’une seule courbe.',
      summary: 'Cinq paliers physiques, une seule décision de courbe.',
    },
    auraevent4: {
      titleFr: 'auraevent4 — vide / vide / absent',
      skill: 'Fire Bolt',
      ...sourceEvidenceForSkill(sources, 'Fire Bolt', 'auraevent4', witness('auraevent4')),
    },
    checkfunc: {
      titleFr: 'checkfunc PD2 non vide',
      field: witness('checkfunc'),
      nonBlankPd2Cells: witness('checkfunc').usage.bySource.pd2.nonBlankCells,
      examples: witness('checkfunc').examples.pd2,
      gate: 'GLOBAL_NATIVE_PORTABILITY',
      d2rSupportClaimed: false,
      summary: 'Gate global de portabilité; aucun support D2R 3.2 n’est prétendu.',
    },
    fireBolt: {
      ...fireBoltImpact,
      titleFr: 'Fire Bolt — simulation de réduction',
      summary: `${fireBoltImpact.currentRequiredDecisions} décisions actuelles ramenées à ${fireBoltImpact.finalPlayerDecisions} bundles sans perte de preuve.`,
    },
  };
}

function selectedSourceManifest(sources, references) {
  const manifest = {
    skills: {},
    references: {},
  };
  for (const source of SOURCE_KEYS) {
    const entry = sources.sourceManifest?.[source]?.tables?.['skills.txt'];
    if (!entry) throw new Error(`Missing ${source} skills.txt source manifest`);
    manifest.skills[source] = clone(entry);
  }
  for (const [id, reference] of Object.entries(references ?? {})) {
    const { document, raw, ...serializable } = reference;
    manifest.references[id] = clone(serializable);
  }
  return manifest;
}

function selectedSourceHashes(sourceManifest) {
  return {
    skills: Object.fromEntries(SOURCE_KEYS.map((source) => [source, sourceManifest.skills[source].sha256])),
    references: Object.fromEntries(Object.entries(sourceManifest.references).map(([id, reference]) => [id, reference.sha256])),
  };
}

function coverageFor(columns, sources, policies, fireBoltImpact) {
  const pd2Only = columns.filter((column) => column.presence.pd2 && !column.presence.vanilla32 && !column.presence.bkvince);
  const sourceOnlyHeaders = Object.fromEntries(SOURCE_KEYS.map((source) => [source, columns
    .filter((column) => column.presence[source] && SOURCE_KEYS.filter((candidate) => candidate !== source).every((candidate) => !column.presence[candidate]))
    .map((column) => column.canonicalHeader)]));
  const sourceOnlyUsed = Object.fromEntries(SOURCE_KEYS.map((source) => [source, sourceOnlyHeaders[source]
    .filter((header) => columns.find((column) => column.canonicalHeader === header).usage.bySource[source].nonBlankCells > 0)]));
  const sourceOnlySemanticBlank = Object.fromEntries(SOURCE_KEYS.map((source) => [source, sourceOnlyHeaders[source]
    .filter((header) => columns.find((column) => column.canonicalHeader === header).usage.bySource[source].nonBlankCells === 0)]));
  const countBy = (selector) => columns.reduce((counts, column) => {
    for (const value of selector(column)) counts[value] = (counts[value] ?? 0) + 1;
    return counts;
  }, {});
  return {
    canonicalHeaders: columns.length,
    sourceHeaders: Object.fromEntries(SOURCE_KEYS.map((source) => [source, sourceDocument(sources, source).table.headers.length])),
    sourceRows: Object.fromEntries(SOURCE_KEYS.map((source) => [source, sourceDocument(sources, source).table.rows.length])),
    pd2OnlyHeaders: pd2Only.map((column) => column.canonicalHeader),
    pd2OnlyUsed: pd2Only.filter((column) => column.usage.bySource.pd2.nonBlankCells > 0).map((column) => column.canonicalHeader),
    pd2OnlySemanticBlank: pd2Only.filter((column) => column.usage.bySource.pd2.nonBlankCells === 0).map((column) => column.canonicalHeader),
    sourceOnlyHeaders,
    sourceOnlyUsed,
    sourceOnlySemanticBlank,
    playerFacingFields: columns.filter((column) => !column.technicalOnly).length,
    technicalFields: columns.filter((column) => column.technicalOnly).length,
    documentedFields: columns.filter((column) => column.documentation.status === 'DOCUMENTED').length,
    headerOnlyDocumentaryFields: columns.filter((column) => column.documentation.status === 'HEADER_ONLY').length,
    classificationCounts: countBy((column) => column.classifications),
    primaryClassificationCounts: countBy((column) => [column.primaryClassification]),
    familyCounts: countBy((column) => [column.family]),
    decisionScopeCounts: countBy((column) => [column.decisionScope]),
    mechanicalContracts: MECHANICAL_CONTRACTS.length,
    decisionBundles: DECISION_BUNDLES.length,
    policies: policies.length,
    pendingPolicies: policies.filter((policy) => policy.decision === 'PENDING').length,
    fireBoltCurrentFieldsAccounted: fireBoltImpact.accounting.noRelevantDifferenceHidden,
  };
}

function provenEquivalences(columns) {
  return columns
    .filter((column) => column.potentialEquivalent.relation === 'SAME_CANONICAL_HEADER')
    .map((column) => ({
      sourceHeader: column.canonicalHeader,
      targetHeaders: column.potentialEquivalent.fields,
      relation: 'SAME_CANONICAL_HEADER',
      proofScope: 'SCHEMA_CONCEPT_ONLY',
      proofStatus: column.potentialEquivalent.proofStatus,
      valueSemanticsProven: column.potentialEquivalent.valueSemanticsProven,
      ...(column.potentialEquivalent.valueSemanticsProven ? {} : { notProvenReference: 'CROSS_ENGINE_CALLBACK_NUMBERS' }),
    }));
}

function unresolvedNativeQuestions(columns) {
  return [
    {
      id: 'PD2_DELAY_CONSUMER_AND_UNITS',
      titleFr: 'Consumer et unités du délai PD2',
      fields: ['delay', 'localdelay', 'globaldelay', 'perdelay'],
      questionFr: 'Quel consumer PD2 interprète delay, avec quelles unités et quelles interactions, et quelle intention peut être reproduite sans nouveau header D2R ?',
      question: 'Quel consumer PD2 interprète delay, avec quelles unités et quelles interactions, et quelle intention peut être reproduite sans nouveau header D2R ?',
      proofStatus: 'NATIVE_UNPROVEN',
    },
    {
      id: 'PD2_CHECKFUNC_D2R_SUPPORT',
      titleFr: 'Support D2R de checkfunc',
      fields: ['checkfunc'],
      questionFr: 'D2R 3.2 compile-t-il ou consomme-t-il un équivalent de checkfunc absent de son schéma suivi ?',
      question: 'D2R 3.2 compile-t-il ou consomme-t-il un équivalent de checkfunc absent de son schéma suivi ?',
      proofStatus: 'NATIVE_UNPROVEN',
    },
    {
      id: 'CROSS_ENGINE_CALLBACK_NUMBERS',
      titleFr: 'Numéros de callbacks entre moteurs',
      fields: ['srvstfunc', 'srvdofunc', 'srvstopfunc', 'cltstfunc', 'cltdofunc', 'cltstopfunc', 'itemeffect', 'itemclteffect'],
      questionFr: 'Pour chaque numéro divergent, quel comportement D2R 3.2 est prouvé au lieu d’être déduit du numéro PD2 ?',
      question: 'Pour chaque numéro divergent, quel comportement D2R 3.2 est prouvé au lieu d’être déduit du numéro PD2 ?',
      proofStatus: 'NATIVE_UNPROVEN',
      promotedFindings: ['srvdofunc 20 — Static Field', 'srvdofunc 30 — zone curses/states'],
    },
    {
      id: 'PD2_ONLY_USED_HEADERS',
      titleFr: 'Headers PD2-only utilisés',
      fields: columns.filter((column) => column.primaryClassification === 'NATIVE_EXTENSION_REQUIRED').map((column) => column.canonicalHeader),
      questionFr: 'Ces intentions PD2 peuvent-elles être traduites vers des mécanismes D2R existants avant toute extension native ?',
      question: 'Ces intentions PD2 peuvent-elles être traduites vers des mécanismes D2R existants avant toute extension native ?',
      proofStatus: 'NATIVE_UNPROVEN',
    },
    {
      id: 'MISSILE_NATIVE_CALLBACKS',
      titleFr: 'Callbacks natifs des missiles',
      fields: ['missiles.txt:*func', 'missiles.txt:*par'],
      questionFr: 'Les fonctions et paramètres de missile PD2 sélectionnés ont-ils un équivalent D2R 3.2 prouvé côté serveur et client ?',
      question: 'Les fonctions et paramètres de missile PD2 sélectionnés ont-ils un équivalent D2R 3.2 prouvé côté serveur et client ?',
      proofStatus: 'NATIVE_UNPROVEN',
    },
  ];
}

export function buildSchemaOrientationData(sources, skillReport, options = {}) {
  if (!sources || !skillReport) throw new Error('buildSchemaOrientationData requires governed sources and the skill report');
  const references = options.references ?? {};
  const schemaDocument = references.skillsSchema?.document ?? options.schemaDocument ?? null;
  if (!schemaDocument) throw new Error('schemas/skills.json document is required for reproducible field documentation');
  const columns = buildColumns(sources, skillReport, schemaDocument);
  const policies = policyDefinitions();
  const fireBoltImpact = buildFireBoltImpact(sources, skillReport, columns);
  const witnesses = buildWitnesses(sources, columns, fireBoltImpact);
  const sourceManifest = selectedSourceManifest(sources, references);
  const sourceHashes = selectedSourceHashes(sourceManifest);
  const coverage = coverageFor(columns, sources, policies, fireBoltImpact);
  const payload = {
    schemaVersion: ORIENTATION_SCHEMA_VERSION,
    orientationId: ORIENTATION_ID,
    productName: ORIENTATION_PRODUCT_NAME,
    state: 'PHASE_0_POLICY_REVIEW_ONLY_GAMEPLAY_APPLICATION_FORBIDDEN',
    frozenContractHash: FROZEN_ORIENTATION_CONTRACT_HASH,
    sourceManifest,
    sourceHashes,
    semanticBlankDefinition: 'value === null || value === undefined || String(value).trim() === ""',
    enums: {
      sources: SOURCE_KEYS,
      rawValueStates: RAW_VALUE_STATES,
      schemaClassifications: SCHEMA_CLASSIFICATIONS,
      decisionScopes: DECISION_SCOPES,
      policyDecisions: POLICY_DECISIONS,
    },
    coverage,
    columns,
    mechanicalContracts: clone(MECHANICAL_CONTRACTS),
    bundles: clone(DECISION_BUNDLES),
    policies,
    witnesses,
    fireBoltImpact,
    equivalences: {
      proven: provenEquivalences(columns),
      notProven: [
        { sourceHeader: 'delay', targetHeaders: ['localdelay', 'globaldelay', 'perdelay'], reason: 'Native model and units are not proven equivalent.' },
        { sourceHeader: 'checkfunc', targetHeaders: [], reason: 'No target header or D2R 3.2 consumer is proven.' },
        { sourceHeader: 'numeric callbacks', targetHeaders: ['same-named callback fields'], reason: 'Equal numeric selectors do not prove equal behavior across engines.' },
      ],
    },
    unresolvedNativeQuestions: unresolvedNativeQuestions(columns),
    nonMutationRules: clone(NON_MUTATION_RULES),
    interfaces: clone(ORIENTATION_INTERFACES),
  };
  const orientationHash = sha256Canonical(payload);
  return {
    ...payload,
    orientationHash,
    workbenchBinding: options.workbenchBinding ? clone(options.workbenchBinding) : null,
  };
}

export function buildFieldDictionary(orientation) {
  return {
    schemaVersion: ORIENTATION_SCHEMA_VERSION,
    orientationId: orientation.orientationId,
    orientationHash: orientation.orientationHash,
    documentationSource: orientation.sourceManifest.references.skillsSchema ?? null,
    fields: orientation.columns.map((column) => ({
      rawHeader: column.canonicalHeader,
      rawHeaders: column.rawHeaders,
      playerLabelFr: column.playerLabelFr,
      shortHelpFr: column.shortHelpFr,
      family: column.family,
      technicalOnly: column.technicalOnly,
      protected: column.protected,
      decisionScope: column.decisionScope,
      groupId: column.groupId,
      documentationSource: column.documentation.documentationSource,
      proofStatus: column.proofStatus,
    })),
  };
}

export function buildPolicySchema(orientation) {
  const properties = {};
  for (const policy of orientation.policies) {
    properties[policy.id] = {
      type: 'object',
      additionalProperties: false,
      required: [...POLICY_DECISION_ENTRY_CONTRACT.required],
      properties: {
        fingerprint: { const: policy.fingerprint },
        decision: { enum: [...POLICY_DECISIONS] },
        justification: { type: 'string' },
        customPolicy: { type: 'string' },
      },
      allOf: [
        {
          if: { properties: { decision: { const: 'MODIFY' } }, required: ['decision'] },
          then: { required: ['customPolicy'], properties: { justification: { minLength: 1 }, customPolicy: { minLength: 1 } } },
        },
        {
          if: { properties: { decision: { const: 'APPROVE' } }, required: ['decision'] },
          then: { properties: { justification: { minLength: 1 } } },
        },
      ],
    };
  }
  return {
    $schema: 'https://json-schema.org/draft/2020-12/schema',
    $id: 'pd2-skills-schema-policy.schema.json',
    title: 'PD2 Skills Phase 0 schema policy decisions',
    type: 'object',
    additionalProperties: false,
    required: ['schemaVersion', 'kind', 'orientationId', 'orientationHash', 'frozenContractHash', 'sourceHashes', 'exportedAt', 'decisions'],
    properties: {
      schemaVersion: { const: POLICY_SCHEMA_VERSION },
      kind: { const: POLICY_KIND },
      orientationId: { const: orientation.orientationId },
      orientationHash: { const: orientation.orientationHash },
      frozenContractHash: { const: orientation.frozenContractHash },
      sourceHashes: { const: orientation.sourceHashes },
      exportedAt: { type: 'string', format: 'date-time' },
      decisions: {
        type: 'object',
        additionalProperties: false,
        required: orientation.policies.map((policy) => policy.id),
        properties,
      },
    },
  };
}

export function buildPolicyEnvelope(orientation, overrides = {}) {
  const decisions = {};
  for (const policy of orientation.policies) {
    const override = overrides[policy.id] ?? {};
    decisions[policy.id] = {
      fingerprint: policy.fingerprint,
      decision: override.decision ?? 'PENDING',
      justification: override.justification ?? '',
      ...(override.customPolicy === undefined ? {} : { customPolicy: override.customPolicy }),
    };
  }
  return {
    schemaVersion: POLICY_SCHEMA_VERSION,
    kind: POLICY_KIND,
    orientationId: orientation.orientationId,
    orientationHash: orientation.orientationHash,
    frozenContractHash: orientation.frozenContractHash,
    sourceHashes: clone(orientation.sourceHashes),
    exportedAt: '1970-01-01T00:00:00.000Z',
    decisions,
  };
}

export function serializeJson(value) {
  return `${JSON.stringify(value, null, 2)}\n`;
}

function markdownCell(value) {
  return String(value ?? '—').replaceAll('|', '\\|').replaceAll('\n', ' ');
}

export function serializeOrientationMarkdown(orientation) {
  const lines = [];
  lines.push(`# ${orientation.productName}`, '');
  lines.push('Phase 0 analytique. Le schéma cible reste D2R 3.2 / BKVince; aucune table gameplay, ligne, colonne, ordinal ou valeur de dégâts n’est modifiée.', '');
  lines.push(`- Orientation hash : \`${orientation.orientationHash}\``);
  lines.push(`- Contrat gelé : \`${orientation.frozenContractHash}\``);
  lines.push(`- Headers : Vanilla ${orientation.coverage.sourceHeaders.vanilla32}, BKVince ${orientation.coverage.sourceHeaders.bkvince}, PD2 ${orientation.coverage.sourceHeaders.pd2}, union canonique ${orientation.coverage.canonicalHeaders}.`);
  lines.push(`- PD2-only utilisés : ${orientation.coverage.pd2OnlyUsed.map((value) => `\`${value}\``).join(', ')}.`);
  lines.push(`- PD2-only entièrement vides : ${orientation.coverage.pd2OnlySemanticBlank.map((value) => `\`${value}\``).join(', ')}.`, '');

  lines.push('## Contrats mécaniques', '');
  for (const contract of orientation.mechanicalContracts) {
    lines.push(`### ${contract.titleFr}`, '', `${contract.consumerFr}`, '', `Politique : \`${contract.translationPolicy}\` — preuve : \`${contract.proofStatus}\`.`, '');
  }

  lines.push('## Bundles atomiques', '', '| Bundle | Libellé | Protégé | Tables liées |', '|---|---|:---:|---|');
  for (const bundle of orientation.bundles) {
    lines.push(`| \`${bundle.id}\` | ${markdownCell(bundle.labelFr)} | ${bundle.protected ? 'oui' : 'non'} | ${markdownCell(bundle.linkedTables.join(', '))} |`);
  }
  lines.push('');

  lines.push('## Politiques globales', '');
  for (const policy of orientation.policies) {
    lines.push(`- \`${policy.id}\` — **PENDING** — ${policy.statementFr}`);
  }
  lines.push('');

  const fireBolt = orientation.fireBoltImpact;
  lines.push('## Témoin Fire Bolt', '');
  lines.push(`Le modèle actuel expose ${fireBolt.currentModifiedFields} champs modifiés et ${fireBolt.currentRequiredDecisions} décisions. Phase 0 les ramène à ${fireBolt.finalPlayerDecisions} décisions de comportement, sans perte de preuve.`);
  lines.push('');
  lines.push('| Réduction | Champs | Justification |', '|---|---:|---|');
  for (const [id, reduction] of Object.entries(fireBolt.reductions)) {
    lines.push(`| ${id} | ${reduction.count} | ${markdownCell(reduction.reasons.join(', ') || 'Projection gouvernée')} |`);
  }
  lines.push('');
  lines.push('Bundles finaux :', '');
  for (const projection of fireBolt.bundleProjection) {
    lines.push(`- \`${projection.groupId}\` : ${projection.fields.map((field) => `\`${field}\``).join(', ')}`);
  }
  lines.push('');

  lines.push('## Matrice exhaustive des headers Skills.txt', '');
  lines.push('| Header canonique | Vanilla | BKV | PD2 | Non vides V/B/P | Joueur V/B/P | Technique V/B/P | Classification | Portée | Politique |');
  lines.push('|---|:---:|:---:|:---:|---:|---:|---:|---|---|---|');
  for (const column of orientation.columns) {
    const counts = SOURCE_KEYS.map((source) => column.usage.bySource[source].nonBlankCells).join('/');
    const players = SOURCE_KEYS.map((source) => column.usage.bySource[source].playerSkills).join('/');
    const technical = SOURCE_KEYS.map((source) => column.usage.bySource[source].technicalOrMonsterRows).join('/');
    lines.push(`| \`${markdownCell(column.canonicalHeader)}\` | ${column.presence.vanilla32 ? '✓' : '—'} | ${column.presence.bkvince ? '✓' : '—'} | ${column.presence.pd2 ? '✓' : '—'} | ${counts} | ${players} | ${technical} | \`${column.primaryClassification}\` | \`${column.decisionScope}\` | \`${column.defaultPolicy}\` |`);
  }
  lines.push('');

  lines.push('## Questions natives non résolues', '');
  for (const question of orientation.unresolvedNativeQuestions) lines.push(`- \`${question.id}\` — ${question.questionFr}`);
  lines.push('', '## Sources hashées', '');
  for (const [source, hash] of Object.entries(orientation.sourceHashes.skills)) lines.push(`- Skills ${SOURCE_LABELS[source]} : \`${hash}\``);
  for (const [id, hash] of Object.entries(orientation.sourceHashes.references)) lines.push(`- ${id} : \`${hash}\``);
  return `${lines.join('\n')}\n`;
}

export const serializeOrientationJson = serializeJson;
export const serializeFieldDictionary = serializeJson;
export const serializePolicySchema = serializeJson;
export const serializePolicyEnvelope = serializeJson;
