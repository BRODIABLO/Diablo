import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

const require = createRequire(import.meta.url);
const { ENCODING, parseTable, serializeTable } = require('../build-data/tsv.js');

const repoRoot = fileURLToPath(new URL('../../', import.meta.url));

export const DEFAULT_LEVELS = Object.freeze([1, 5, 10, 20, 30, 40]);
export const CLASS_ORDER = Object.freeze(['ama', 'sor', 'nec', 'pal', 'bar', 'dru', 'ass', 'war']);
export const CLASS_NAMES = Object.freeze({
  ama: 'Amazon',
  sor: 'Sorceress',
  nec: 'Necromancer',
  pal: 'Paladin',
  bar: 'Barbarian',
  dru: 'Druid',
  ass: 'Assassin',
  war: 'Warlock',
});

const GOVERNED_SKILL_RELATIONS = Object.freeze([
  ['renamed-alias', 'AmpDmg', 'Amplify Damage', 'Même ordinal, même classe et même skilldesc.'],
  ['renamed-alias', 'LowRes', 'Lower Resist', 'Même ordinal, même classe et même skilldesc.'],
  ['slot-replacement', 'Slow Movement', 'Slow Missiles', 'Même slot Amazon; mécaniques distinctes.'],
  ['slot-replacement', 'Javelin and Spear Mastery', 'Impale', 'Même slot Amazon; passif contre attaque.'],
  ['slot-replacement', 'Desecrate', 'Poison Explosion', 'Même slot Necromancer; identités distinctes.'],
  ['slot-replacement', 'Raise Skeleton Archer', 'Summon Resist', 'Même slot Necromancer; invocation contre passif.'],
  ['slot-replacement', 'Holy Sword', 'Conversion', 'Même slot Paladin; identités distinctes.'],
  ['slot-replacement', 'Sword Mastery', 'Blade Mastery', 'Même slot Barbarian; périmètres d’armes distincts.'],
  ['slot-replacement', 'One Hand Mastery', 'Axe Mastery', 'Même slot Barbarian; périmètres d’armes distincts.'],
  ['slot-replacement', 'Two Hand Mastery', 'Pole Arm Mastery', 'Même slot Barbarian; périmètres d’armes distincts.'],
  ['slot-replacement', 'Combat Reflexes', 'Increased Endurance', 'Même slot Barbarian; passifs distincts.'],
]);

export const defaultSkillReportRoots = Object.freeze({
  pd2: process.env.PD2_SP_ROOT || path.resolve(
    repoRoot,
    '..',
    'PD2 Single PLayer',
    'PD2-Single-Player-Plus-mod-main',
    'data',
    'global',
    'excel',
  ),
  bkvince: path.join(repoRoot, 'data-BKVince', 'BKVince.mpq', 'data', 'global', 'excel'),
  vanilla32: path.join(repoRoot, 'data-vanilla3.2', 'data', 'data', 'global', 'excel'),
  output: path.join(repoRoot, 'Mission', 'pd2-skills-vs-bkvince-full-audit.md'),
});

const SOURCE_TABLES = Object.freeze([
  'skills.txt',
  'missiles.txt',
  'states.txt',
  'skilldesc.txt',
  'itemstatcost.txt',
  'properties.txt',
  'pettype.txt',
  'monstats.txt',
  'charstats.txt',
  'hireling.txt',
  'uniqueitems.txt',
  'setitems.txt',
  'sets.txt',
  'runes.txt',
]);

const REFERENCE_TABLES = new Set([
  'charstats.txt',
  'hireling.txt',
  'uniqueitems.txt',
  'setitems.txt',
  'sets.txt',
  'runes.txt',
  'monstats.txt',
]);

const KEY_CANDIDATES = Object.freeze({
  'skills.txt': ['skill'],
  'missiles.txt': ['missile'],
  'states.txt': ['state'],
  'skilldesc.txt': ['skilldesc'],
  'itemstatcost.txt': ['stat'],
  'properties.txt': ['code'],
  'pettype.txt': ['pet type', 'pettype'],
  'monstats.txt': ['id'],
  'charstats.txt': ['class'],
  'hireling.txt': ['hireling', 'id'],
  'uniqueitems.txt': ['index'],
  'setitems.txt': ['index'],
  'sets.txt': ['index'],
  'runes.txt': ['name'],
});

const SKILL_CLASS_SET = new Set(CLASS_ORDER);

const FUNCTION_FIELDS = new Set([
  'srvstfunc', 'srvdofunc', 'srvstopfunc',
  'cltstfunc', 'cltdofunc', 'cltstopfunc',
  'srvprgfunc1', 'srvprgfunc2', 'srvprgfunc3',
  'cltprgfunc1', 'cltprgfunc2', 'cltprgfunc3',
]);

const FORMULA_FIELD_PATTERN = /^(prgcalc[1-3]|auralencalc|aurarangecalc|aurastatcalc[1-6]|passivecalc(?:[1-9]|1[0-4])|petmax|sumsk[1-5]calc|cltcalc[1-3]|skpoints|localdelay|globaldelay|perdelay|delay|calc(?:[1-9]|10)|tohitcalc|dmgsympercalc|edmgsympercalc|elensympercalc)$/;

const CURATED_DECISIONS = Object.freeze({
  'inner sight': ['ADAPTER', "Étudier la réduction d'Attack Rating et le rayon progressif PD2."],
  'strafe': ['PROTOTYPER EN PACKAGE', 'Associer maximum 5 tirs, suppression du Next Hit Delay, missile et retuning DPS.'],
  'poison javelin': ['ADAPTER PARTIELLEMENT', 'Vitesse et hitbox d’abord; durée et dégâts doivent rester un package séparé.'],
  'plague javelin': ['ADAPTER PARTIELLEMENT', 'Vitesse et hitbox d’abord; recalcul complet avant toute durée fixe.'],
  'cloak of shadows': ['ADAPTER', 'Durée fixe prévisible à tester sans importer le rework Mind Blast.'],
  'dragon claw': ['ADAPTER', "L'interruptibilité est un changement isolable à faible risque de sauvegarde."],
  'dragon tail': ['ADAPTER PRUDEMMENT', 'Comparer précisément la pénalité de vitesse et le knockback.'],
  'shock web': ['PROTOTYPER', 'Suppression du Next Hit Delay seulement avec mesure DPS et densité.'],
  'corpse explosion': ['RETUNER, NE PAS COPIER', 'PD2 et BKVince utilisent deux philosophies de scaling radicalement différentes.'],
  'life tap': ['RETUNER', "La valeur constante BKVince et la courbe PD2 doivent être mesurées sur melee, mercenaires et summons."],
  'summon grizzly': ['ADAPTER', 'Ajouter seulement Summon Splash, déjà présent dans BKVince pour les autres animaux.'],
  'blessed hammer': ['ADAPTER PARTIELLEMENT', 'Tester vitesse et hitbox sans reprendre automatiquement la portée PD2.'],
  'sanctuary': ['PROTOTYPER', "Tous les ennemis sans importer d'abord le shred magique ou l'immunity break."],
  'enchant': ['PROTOTYPER', 'Tester uniquement le cast de groupe AoE en conservant durée et balance BKVince.'],
  'ice bolt': ['ADAPTER', 'La vitesse du projectile est isolable et réversible.'],
  'lightning': ['ADAPTER', "Tester l'animation de cast rapide et les breakpoints FCR."],
  'gust': ['DIFFÉRER', 'Nouvelle identité de mobilité, ID en collision et fonctions PD2 non portables.'],
  'blood warp': ['DIFFÉRER', 'Nouvelle mobilité Necromancer, ID en collision et logique de récupération native.'],
  'curse mastery': ['REJETER LE PORT LITTÉRAL', 'Collision Warlock et stat max_curses incompatible avec les saves BKVince.'],
  'dark pact': ['REJETER LE PORT LITTÉRAL', 'Collision Warlock et dépendance au modèle multi-curse PD2.'],
  'combustion': ['DIFFÉRER', 'Burst distinct mais nouveau skill, nouveaux missiles et ID en collision.'],
  'lesser hydra': ['REJETER POUR MAINTENANT', 'Redondant avec la Hydra BKVince déjà beaucoup plus généreuse.'],
  'joust': ['REJETER POUR MAINTENANT', 'Concurrence Charge et entre en collision avec Blood Oath.'],
});

function sha256(buffer) {
  return crypto.createHash('sha256').update(buffer).digest('hex').toUpperCase();
}

function canonicalHeader(header) {
  const normalized = String(header ?? '').trim().toLowerCase();
  if (normalized === 'id' || normalized === '*id') return 'id';
  return normalized;
}

function normalizedName(value) {
  return String(value ?? '').trim().toLowerCase();
}

function resolveTablePath(root, requestedName) {
  const direct = path.join(root, requestedName);
  if (fs.existsSync(direct)) return direct;
  const lower = requestedName.toLowerCase();
  const actual = fs.readdirSync(root).find((entry) => entry.toLowerCase() === lower);
  if (!actual) throw new Error(`Missing table ${requestedName} under ${root}`);
  return path.join(root, actual);
}

function findKeyHeader(tableName, headers) {
  const canonical = new Map(headers.map((header) => [canonicalHeader(header), header]));
  for (const candidate of KEY_CANDIDATES[tableName] ?? []) {
    const actual = canonical.get(candidate);
    if (actual) return actual;
  }
  return headers[0];
}

export function loadGovernedTable(root, requestedName) {
  const filePath = resolveTablePath(root, requestedName);
  const rawBuffer = fs.readFileSync(filePath);
  const rawText = fs.readFileSync(filePath, ENCODING);
  const table = parseTable(filePath);
  assert.equal(serializeTable(table), rawText, `Round-trip mismatch: ${filePath}`);

  const canonicalIndexes = new Map();
  table.headers.forEach((header, index) => {
    const canonical = canonicalHeader(header);
    if (canonicalIndexes.has(canonical)) {
      throw new Error(`Duplicate canonical header ${canonical} in ${filePath}`);
    }
    canonicalIndexes.set(canonical, index);
  });

  const keyHeader = findKeyHeader(requestedName.toLowerCase(), table.headers);
  const keyIndex = table.headers.indexOf(keyHeader);
  const records = table.rows.map((row, index) => ({
    document: null,
    row,
    index,
    line: index + 2,
    key: row[keyIndex] ?? '',
    get(field) {
      const position = canonicalIndexes.get(canonicalHeader(field));
      return position === undefined ? undefined : row[position];
    },
    has(field) {
      return canonicalIndexes.has(canonicalHeader(field));
    },
  }));

  const document = {
    requestedName: requestedName.toLowerCase(),
    actualName: path.basename(filePath),
    filePath,
    relativePath: path.relative(repoRoot, filePath).replaceAll('\\', '/'),
    rawBuffer,
    sha256: sha256(rawBuffer),
    table,
    canonicalIndexes,
    keyHeader,
    keyCanonical: canonicalHeader(keyHeader),
    records,
    byKey: new Map(),
  };
  for (const record of records) {
    record.document = document;
    const key = normalizedName(record.key);
    if (!document.byKey.has(key)) document.byKey.set(key, []);
    document.byKey.get(key).push(record);
  }
  return document;
}

export function loadSkillReportSources(roots = defaultSkillReportRoots) {
  const result = { pd2: {}, bkvince: {}, reference: {}, roots, metadata: {} };
  for (const source of ['pd2', 'bkvince']) {
    for (const tableName of SOURCE_TABLES) {
      result[source][tableName] = loadGovernedTable(roots[source], tableName);
    }
  }
  result.reference['skillcalc.txt'] = loadGovernedTable(roots.vanilla32, 'skillcalc.txt');
  const pd2MetadataPath = path.resolve(roots.pd2, '..', '..', '..', 'pd2-single-player-plus.json');
  if (fs.existsSync(pd2MetadataPath)) {
    const raw = fs.readFileSync(pd2MetadataPath);
    result.metadata.pd2 = {
      path: path.relative(repoRoot, pd2MetadataPath).replaceAll('\\', '/'),
      sha256: sha256(raw),
      value: JSON.parse(raw.toString('utf8')),
    };
  }
  return result;
}

function uniqueByName(records) {
  const result = new Map();
  for (const record of records) {
    const name = normalizedName(record.get('skill'));
    if (!result.has(name)) result.set(name, []);
    result.get(name).push(record);
  }
  return result;
}

function uniqueByRuntimeOrdinal(records) {
  const result = new Map();
  for (const record of records) {
    const ordinal = String(record.index);
    if (!result.has(ordinal)) result.set(ordinal, []);
    result.get(ordinal).push(record);
  }
  return result;
}

function declaredSkillId(record) {
  return record ? String(record.get('id') ?? '').trim() : '';
}

function runtimeSkillOrdinal(record) {
  return record ? record.index : null;
}

export function buildSkillMapping(pd2Document, bkvinceDocument) {
  const pd2Rows = pd2Document.records.filter((record) => String(record.get('skill') ?? '').trim());
  const bkvRows = bkvinceDocument.records.filter((record) => String(record.get('skill') ?? '').trim());
  const pd2Names = uniqueByName(pd2Rows);
  const bkvNames = uniqueByName(bkvRows);
  const usedBkv = new Set();
  const pairedByPd2Ordinal = new Map();
  const ambiguityByPd2Ordinal = new Map();

  // Axis 1: reserve every unique semantic/name match before considering ordinal collisions.
  for (const pd2 of pd2Rows) {
    const name = normalizedName(pd2.get('skill'));
    const pd2Matches = pd2Names.get(name) ?? [];
    const bkvMatches = bkvNames.get(name) ?? [];
    if (pd2Matches.length === 1 && bkvMatches.length === 1) {
      const [bkvince] = bkvMatches;
      pairedByPd2Ordinal.set(pd2.index, bkvince);
      usedBkv.add(bkvince.index);
    } else if (pd2Matches.length > 1 || bkvMatches.length > 1) {
      ambiguityByPd2Ordinal.set(
        pd2.index,
        `non-unique normalized name ${pd2.get('skill')} (PD2=${pd2Matches.length}, BKVince=${bkvMatches.length})`,
      );
    }
  }

  // Axis 2: pair still-unmatched rows only when the exact runtime ordinal remains available.
  const bkvByRuntimeOrdinal = new Map(bkvRows.map((record) => [record.index, record]));
  for (const pd2 of pd2Rows) {
    if (pairedByPd2Ordinal.has(pd2.index)) continue;
    const bkvince = bkvByRuntimeOrdinal.get(pd2.index);
    if (bkvince && !usedBkv.has(bkvince.index)) {
      pairedByPd2Ordinal.set(pd2.index, bkvince);
      usedBkv.add(bkvince.index);
    }
  }

  const entries = pd2Rows.map((pd2) => {
    const bkvince = pairedByPd2Ordinal.get(pd2.index) ?? null;
    let status = 'pd2-only';
    if (bkvince) {
      const sameName = normalizedName(pd2.get('skill')) === normalizedName(bkvince.get('skill'));
      status = sameName
        ? (pd2.index === bkvince.index ? 'same-name-same-id' : 'same-name-moved-id')
        : 'same-id-different-name';
    }
    return {
      pd2,
      bkvince,
      status,
      ambiguity: ambiguityByPd2Ordinal.get(pd2.index) ?? '',
    };
  });

  for (const bkvince of bkvRows) {
    if (!usedBkv.has(bkvince.index)) {
      entries.push({ pd2: null, bkvince, status: 'bkv-only', ambiguity: '' });
    }
  }

  const runtimeCollisions = pd2Rows
    .filter((pd2) => bkvByRuntimeOrdinal.has(pd2.index))
    .map((pd2) => ({ pd2, bkvince: bkvByRuntimeOrdinal.get(pd2.index) }))
    .filter(({ pd2, bkvince }) => normalizedName(pd2.get('skill')) !== normalizedName(bkvince.get('skill')));
  const declaredIdAnomalies = {
    pd2: pd2Rows.filter((record) => declaredSkillId(record) !== String(runtimeSkillOrdinal(record))),
    bkvince: bkvRows.filter((record) => declaredSkillId(record) !== String(runtimeSkillOrdinal(record))),
  };
  const semanticNamePairs = entries.filter(({ pd2, bkvince }) => (
    pd2 && bkvince && normalizedName(pd2.get('skill')) === normalizedName(bkvince.get('skill'))
  ));
  const pd2NameSet = new Set(pd2Rows.map((record) => normalizedName(record.get('skill'))));
  const bkvNameSet = new Set(bkvRows.map((record) => normalizedName(record.get('skill'))));

  return {
    entries,
    counts: Object.fromEntries(
      [...new Set(entries.map((entry) => entry.status))]
        .sort()
        .map((status) => [status, entries.filter((entry) => entry.status === status).length]),
    ),
    pd2Rows: pd2Rows.length,
    bkvRows: bkvRows.length,
    mappedPd2Rows: entries.filter((entry) => entry.pd2).length,
    mappedBkvRows: entries.filter((entry) => entry.bkvince).length,
    ambiguous: entries.filter((entry) => entry.ambiguity),
    runtimeCollisions,
    declaredIdAnomalies,
    semanticNamePairs,
    movedSemanticPairs: semanticNamePairs.filter(({ pd2, bkvince }) => pd2.index !== bkvince.index),
    semanticCounts: {
      sharedNames: semanticNamePairs.length,
      sameRuntimeOrdinal: semanticNamePairs.filter(({ pd2, bkvince }) => pd2.index === bkvince.index).length,
      movedRuntimeOrdinal: semanticNamePairs.filter(({ pd2, bkvince }) => pd2.index !== bkvince.index).length,
      pd2OnlyNames: [...pd2NameSet].filter((name) => !bkvNameSet.has(name)).length,
      bkvOnlyNames: [...bkvNameSet].filter((name) => !pd2NameSet.has(name)).length,
    },
  };
}

function numeric(value, fallback = null) {
  if (value === undefined || value === null || String(value).trim() === '') return fallback;
  const parsed = Number(String(value).trim());
  return Number.isFinite(parsed) ? parsed : fallback;
}

function trunc(value) {
  return Math.trunc(value);
}

export function linearValue(level, base, perLevel) {
  return numeric(base, 0) + (level - 1) * numeric(perLevel, 0);
}

export function diminishingValue(level, base, maximum) {
  const start = numeric(base, 0);
  const cap = numeric(maximum, 0);
  const value = trunc((110 * level * (cap - start)) / (100 * (level + 6))) + start;
  return Math.min(value, cap);
}

function tierCounts(level) {
  return [
    Math.max(0, Math.min(level, 8) - 1),
    Math.max(0, Math.min(level, 16) - 8),
    Math.max(0, Math.min(level, 22) - 16),
    Math.max(0, Math.min(level, 28) - 22),
    Math.max(0, level - 28),
  ];
}

export function tieredDamageValue(level, base, increments) {
  const counts = tierCounts(level);
  return numeric(base, 0) + counts.reduce(
    (total, count, index) => total + count * numeric(increments[index], 0),
    0,
  );
}

export function shiftedValue(rawValue, hitShift) {
  return rawValue * (2 ** numeric(hitShift, 0)) / 256;
}

export function manaCostAtLevel(record, level) {
  const mana = numeric(record.get('mana'));
  const levelMana = numeric(record.get('lvlmana'), 0);
  if (mana === null && levelMana === 0) return null;
  const shifted = (numeric(mana, 0) + (level - 1) * levelMana)
    * (2 ** numeric(record.get('manashift'), 0)) / 256;
  return Math.max(numeric(record.get('minmana'), 0), shifted);
}

export function elementalLengthAtLevel(record, level) {
  const base = numeric(record.get('elen'));
  if (base === null) return null;
  const first = Math.max(0, Math.min(level, 8) - 1);
  const second = Math.max(0, Math.min(level, 16) - 8);
  const third = Math.max(0, level - 16);
  return base
    + first * numeric(record.get('elevlen1'), 0)
    + second * numeric(record.get('elevlen2'), 0)
    + third * numeric(record.get('elevlen3'), 0);
}

export function damageAtLevel(record, level, elemental = false) {
  const prefix = elemental ? 'e' : '';
  const minBase = record.get(`${prefix}min${elemental ? '' : 'dam'}`);
  const maxBase = record.get(`${prefix}max${elemental ? '' : 'dam'}`);
  const minKey = elemental ? 'emin' : 'mindam';
  const maxKey = elemental ? 'emax' : 'maxdam';
  const minValue = record.get(minKey);
  const maxValue = record.get(maxKey);
  const hasMin = minValue !== undefined && String(minValue).trim() !== '';
  const hasMax = maxValue !== undefined && String(maxValue).trim() !== '';
  const hasAny = hasMin || hasMax;
  if (!hasAny) return null;
  const minRaw = hasMin
    ? tieredDamageValue(level, minValue, [1, 2, 3, 4, 5].map((index) => (
      record.get(elemental ? `eminlev${index}` : `minlevdam${index}`)
    )))
    : null;
  const maxRaw = hasMax
    ? tieredDamageValue(level, maxValue, [1, 2, 3, 4, 5].map((index) => (
      record.get(elemental ? `emaxlev${index}` : `maxlevdam${index}`)
    )))
    : null;
  const hitShift = numeric(record.get('hitshift'), 0);
  void minBase;
  void maxBase;
  return {
    min: minRaw === null ? null : shiftedValue(minRaw, hitShift),
    max: maxRaw === null ? null : shiftedValue(maxRaw, hitShift),
    minRaw,
    maxRaw,
    hitShift,
  };
}

class UnsupportedFormula extends Error {}

const MISSING_ROW = Symbol('missing-row');

function tokenizeFormula(source) {
  const tokens = [];
  let index = 0;
  while (index < source.length) {
    const rest = source.slice(index);
    const whitespace = rest.match(/^\s+/);
    if (whitespace) {
      index += whitespace[0].length;
      continue;
    }
    const number = rest.match(/^\d+/);
    if (number) {
      tokens.push({ type: 'number', value: Number(number[0]) });
      index += number[0].length;
      continue;
    }
    const identifier = rest.match(/^[A-Za-z_][A-Za-z0-9_]*/);
    if (identifier) {
      tokens.push({ type: 'identifier', value: identifier[0].toLowerCase() });
      index += identifier[0].length;
      continue;
    }
    const operator = rest.match(/^(\|\||&&|==|!=|<=|>=|[+\-*/%^<>()!,?:])/);
    if (operator) {
      tokens.push({ type: 'operator', value: operator[0] });
      index += operator[0].length;
      continue;
    }
    throw new UnsupportedFormula(`unsupported token at ${rest.slice(0, 20)}`);
  }
  tokens.push({ type: 'eof', value: '' });
  return tokens;
}

function baseSkillLevel(record, level) {
  const maximum = numeric(record.get('maxlvl'));
  return maximum && maximum > 0 ? Math.min(level, maximum) : level;
}

function parameterPair(code) {
  return ({
    12: [1, 2],
    34: [3, 4],
    56: [5, 6],
    78: [7, 8],
    91: [9, 10],
    21: [11, 12],
  })[code] ?? null;
}

function formulaPercent(record, field, level, options) {
  if (options.applyDerivedSynergy === false) return 0;
  const formula = record.get(field);
  if (formula === undefined || String(formula).trim() === '') return 0;
  const evaluated = evaluateSimpleFormula(formula, record, level, {
    ...options,
    applyDerivedSynergy: false,
  });
  if (!evaluated.ok) {
    throw new UnsupportedFormula(`unresolved ${field}: ${evaluated.status} ${evaluated.reason}`);
  }
  return evaluated.value;
}

function elementalFixedValue(record, level, side, options) {
  if (level <= 0) return 0;
  const damage = damageAtLevel(record, level, true);
  if (!damage) return 0;
  const raw = side === 'min' ? damage.minRaw : damage.maxRaw;
  if (raw === null) return 0;
  const baseFixed = raw * (2 ** damage.hitShift);
  const synergyPercent = formulaPercent(record, 'edmgsympercalc', level, options);
  return baseFixed + trunc((baseFixed * synergyPercent) / 100);
}

function elementalLengthValue(record, level, options) {
  if (level <= 0) return 0;
  const base = elementalLengthAtLevel(record, level) ?? 0;
  const synergyPercent = formulaPercent(record, 'elensympercalc', level, options);
  return base + trunc((base * synergyPercent) / 100);
}

function toHitValue(record, level, options) {
  if (level <= 0) return 0;
  const formula = record.get('tohitcalc');
  if (formula !== undefined && String(formula).trim() !== '') {
    if (options.resolvingToHit) throw new UnsupportedFormula('recursive tohitcalc');
    const evaluated = evaluateSimpleFormula(formula, record, level, {
      ...options,
      resolvingToHit: true,
    });
    if (!evaluated.ok) {
      throw new UnsupportedFormula(`unresolved tohitcalc: ${evaluated.status} ${evaluated.reason}`);
    }
    return evaluated.value;
  }
  return linearValue(level, record.get('tohit'), record.get('levtohit'));
}

function formulaFieldValue(record, field, level, options) {
  if (!record.has(field)) throw new UnsupportedFormula(`unsupported field ${field}`);
  const raw = String(record.get(field) ?? '').trim();
  if (!raw) return 0;
  const key = `${record.index}:${field}:${level}`;
  const resolvingFormulaFields = options.resolvingFormulaFields ?? new Set();
  if (resolvingFormulaFields.has(key)) {
    throw new UnsupportedFormula(`recursive formula field ${field}`);
  }
  const nextResolving = new Set(resolvingFormulaFields);
  nextResolving.add(key);
  const evaluated = evaluateSimpleFormula(raw, record, level, {
    ...options,
    resolvingFormulaFields: nextResolving,
  });
  if (!evaluated.ok) {
    throw new UnsupportedFormula(`unresolved ${field}: ${evaluated.status} ${evaluated.reason}`);
  }
  return evaluated.value;
}

function resolveFormulaIdentifier(identifier, record, level, options) {
  if (identifier === 'lvl') return level;
  if (identifier === 'blvl') return baseSkillLevel(record, level);
  if (identifier === 'true') return 1;
  if (identifier === 'false') return 0;
  const parameter = identifier.match(/^par(\d{1,2})$/);
  if (parameter) {
    if (Number(parameter[1]) < 1 || Number(parameter[1]) > 20) {
      throw new UnsupportedFormula(`unsupported identifier ${identifier}`);
    }
    const field = `param${parameter[1]}`;
    if (!record.has(field)) throw new UnsupportedFormula(`unsupported identifier ${identifier}`);
    return numeric(record.get(field), 0);
  }
  const extendedParameter = identifier.match(/^pa(1[0-9]|20)$/);
  if (extendedParameter) {
    const field = `param${extendedParameter[1]}`;
    if (!record.has(field)) throw new UnsupportedFormula(`unsupported identifier ${identifier}`);
    return numeric(record.get(field), 0);
  }
  const linear = identifier.match(/^ln(12|34|56|78|91|21)$/);
  if (linear) {
    const [first, second] = parameterPair(linear[1]);
    if (!record.has(`param${first}`) || !record.has(`param${second}`)) {
      throw new UnsupportedFormula(`unsupported identifier ${identifier}`);
    }
    return linearValue(level, record.get(`param${first}`), record.get(`param${second}`));
  }
  const diminishing = identifier.match(/^dm(12|34|56|78|91|21)$/);
  if (diminishing) {
    const [first, second] = parameterPair(diminishing[1]);
    if (!record.has(`param${first}`) || !record.has(`param${second}`)) {
      throw new UnsupportedFormula(`unsupported identifier ${identifier}`);
    }
    return diminishingValue(level, record.get(`param${first}`), record.get(`param${second}`));
  }
  if (identifier === 'edmn') return trunc(elementalFixedValue(record, level, 'min', options) / 256);
  if (identifier === 'edmx') return trunc(elementalFixedValue(record, level, 'max', options) / 256);
  if (identifier === 'edns') return elementalFixedValue(record, level, 'min', options);
  if (identifier === 'edxs') return elementalFixedValue(record, level, 'max', options);
  if (identifier === 'edln') return elementalLengthValue(record, level, options);
  // The report's standard scenario fixes elemental mastery bonuses at zero.
  // Therefore the mastery-aware BBE variants equal their non-mastery counterparts.
  if (identifier === 'enma') return trunc(elementalFixedValue(record, level, 'min', options) / 256);
  if (identifier === 'exma') return trunc(elementalFixedValue(record, level, 'max', options) / 256);
  if (identifier === 'enms') return elementalFixedValue(record, level, 'min', options);
  if (identifier === 'exms') return elementalFixedValue(record, level, 'max', options);
  if (identifier === 'edma') return elementalLengthValue(record, level, options);
  const calcField = identifier.match(/^clc([0-9])$/);
  if (calcField) {
    const number = calcField[1] === '0' ? 10 : Number(calcField[1]);
    return formulaFieldValue(record, `calc${number}`, level, options);
  }
  if (identifier === 'toht') return toHitValue(record, level, options);
  if (identifier === 'mana') return level > 0 ? trunc(manaCostAtLevel(record, level) ?? 0) : 0;
  if (identifier === 'usmc') return level > 0 ? trunc((manaCostAtLevel(record, level) ?? 0) * 256) : 0;
  throw new UnsupportedFormula(`unsupported identifier ${identifier}`);
}

function parseFormulaExpression(tokens, record, level, options) {
  let cursor = 0;
  const peek = () => tokens[cursor];
  const take = (value = null) => {
    const token = tokens[cursor];
    if (value !== null && token.value !== value) {
      throw new UnsupportedFormula(`expected ${value}, found ${token.value}`);
    }
    cursor += 1;
    return token;
  };

  const primary = () => {
    const token = peek();
    if (token.type === 'number') {
      take();
      return () => token.value;
    }
    if (token.type === 'identifier') {
      take();
      if (peek().value === '(') {
        if (!['min', 'max'].includes(token.value)) {
          throw new UnsupportedFormula(`unsupported function ${token.value}`);
        }
        take('(');
        const args = [conditional()];
        while (peek().value === ',') {
          take(',');
          args.push(conditional());
        }
        take(')');
        return () => (
          token.value === 'min'
            ? Math.min(...args.map((argument) => argument()))
            : Math.max(...args.map((argument) => argument()))
        );
      }
      return () => resolveFormulaIdentifier(token.value, record, level, options);
    }
    if (token.value === '(') {
      take('(');
      const value = conditional();
      take(')');
      return value;
    }
    throw new UnsupportedFormula(`unexpected token ${token.value}`);
  };

  const unary = () => {
    if (peek().value === '+') {
      take('+');
      return unary();
    }
    if (peek().value === '-') {
      take('-');
      const operand = unary();
      return () => -operand();
    }
    if (peek().value === '!') {
      take('!');
      const operand = unary();
      return () => (operand() ? 0 : 1);
    }
    return primary();
  };

  const power = () => {
    let value = unary();
    if (peek().value === '^') {
      take('^');
      const left = value;
      const right = power();
      value = () => trunc(left() ** right());
    }
    return value;
  };

  const multiplicative = () => {
    let value = power();
    while (['*', '/', '%'].includes(peek().value)) {
      const operator = take().value;
      const right = power();
      const left = value;
      if (operator === '*') value = () => left() * right();
      if (operator === '/') value = () => trunc(left() / right());
      if (operator === '%') value = () => left() % right();
    }
    return value;
  };

  const additive = () => {
    let value = multiplicative();
    while (['+', '-'].includes(peek().value)) {
      const operator = take().value;
      const right = multiplicative();
      const left = value;
      value = operator === '+' ? () => left() + right() : () => left() - right();
    }
    return value;
  };

  const relational = () => {
    let value = additive();
    while (['<', '<=', '>', '>='].includes(peek().value)) {
      const operator = take().value;
      const right = additive();
      const left = value;
      if (operator === '<') value = () => (left() < right() ? 1 : 0);
      if (operator === '<=') value = () => (left() <= right() ? 1 : 0);
      if (operator === '>') value = () => (left() > right() ? 1 : 0);
      if (operator === '>=') value = () => (left() >= right() ? 1 : 0);
    }
    return value;
  };

  const equality = () => {
    let value = relational();
    while (['==', '!='].includes(peek().value)) {
      const operator = take().value;
      const right = relational();
      const left = value;
      value = operator === '=='
        ? () => (left() === right() ? 1 : 0)
        : () => (left() !== right() ? 1 : 0);
    }
    return value;
  };

  const logicalAnd = () => {
    let value = equality();
    while (peek().value === '&&') {
      take('&&');
      const right = equality();
      const left = value;
      value = () => (left() ? (right() ? 1 : 0) : 0);
    }
    return value;
  };

  const logicalOr = () => {
    let value = logicalAnd();
    while (peek().value === '||') {
      take('||');
      const right = logicalAnd();
      const left = value;
      value = () => (left() ? 1 : (right() ? 1 : 0));
    }
    return value;
  };

  const conditional = () => {
    const condition = logicalOr();
    if (peek().value === '?') {
      take('?');
      const truthy = conditional();
      take(':');
      const falsy = conditional();
      return () => (condition() ? truthy() : falsy());
    }
    return condition;
  };

  const result = conditional();
  if (peek().type !== 'eof') throw new UnsupportedFormula(`trailing token ${peek().value}`);
  return result();
}

function referencedSkillProperty(record, referencedName, property, level, referencedSkillLevel, options) {
  const matches = record.document.byKey.get(normalizedName(referencedName)) ?? [];
  if (matches.length !== 1) return null;
  const [target] = matches;
  const self = target.index === record.index;
  const targetLevel = self ? level : referencedSkillLevel;
  const targetBaseLevel = self ? baseSkillLevel(record, level) : Math.min(referencedSkillLevel, numeric(target.get('maxlvl'), referencedSkillLevel));
  const normalizedProperty = property.toLowerCase();
  if (normalizedProperty === 'lvl') return targetLevel;
  if (normalizedProperty === 'blvl') return targetBaseLevel;
  const parameter = normalizedProperty.match(/^par(\d{1,2})$/);
  if (parameter) {
    const field = `param${parameter[1]}`;
    if (!target.has(field)) return null;
    return numeric(target.get(field), 0);
  }
  const extendedParameter = normalizedProperty.match(/^pa(1[0-9]|20)$/);
  if (extendedParameter) {
    const field = `param${extendedParameter[1]}`;
    if (!target.has(field)) return null;
    return numeric(target.get(field), 0);
  }
  const linear = normalizedProperty.match(/^ln(12|34|56|78|91|21)$/);
  if (linear) {
    if (targetLevel <= 0) return 0;
    const [first, second] = parameterPair(linear[1]);
    if (!target.has(`param${first}`) || !target.has(`param${second}`)) return null;
    return linearValue(targetLevel, target.get(`param${first}`), target.get(`param${second}`));
  }
  const diminishing = normalizedProperty.match(/^dm(12|34|56|78|91|21)$/);
  if (diminishing) {
    if (targetLevel <= 0) return 0;
    const [first, second] = parameterPair(diminishing[1]);
    if (!target.has(`param${first}`) || !target.has(`param${second}`)) return null;
    return diminishingValue(targetLevel, target.get(`param${first}`), target.get(`param${second}`));
  }
  if (normalizedProperty === 'edmn') return trunc(elementalFixedValue(target, targetLevel, 'min', options) / 256);
  if (normalizedProperty === 'edmx') return trunc(elementalFixedValue(target, targetLevel, 'max', options) / 256);
  if (normalizedProperty === 'edns') return elementalFixedValue(target, targetLevel, 'min', options);
  if (normalizedProperty === 'edxs') return elementalFixedValue(target, targetLevel, 'max', options);
  if (normalizedProperty === 'edln') return elementalLengthValue(target, targetLevel, options);
  const calcField = normalizedProperty.match(/^clc([0-9])$/);
  if (calcField) {
    const number = calcField[1] === '0' ? 10 : Number(calcField[1]);
    return formulaFieldValue(target, `calc${number}`, targetLevel, options);
  }
  return null;
}

export function evaluateSimpleFormula(source, record, level, options = {}) {
  const { statValue = 0, referencedSkillLevel = 0 } = options;
  let expression = String(source ?? '').trim();
  if (!expression) return { ok: false, status: 'SYMBOLIC', reason: 'empty formula' };
  const normalizedQuotes = expression.includes('"');
  if (normalizedQuotes) expression = expression.replaceAll('"', '').trim();

  let depth = 0;
  for (const character of expression) {
    if (character === '(') depth += 1;
    if (character === ')') depth -= 1;
    if (depth < 0) break;
  }
  if (depth !== 0) {
    return {
      ok: false,
      status: 'MALFORMED_SOURCE',
      normalizedQuotes,
      reason: `unbalanced parentheses (${depth})`,
    };
  }
  expression = expression.replace(
    /\bstat\s*\(\s*'[^']*'\s*\.\s*[A-Za-z0-9_]+\s*\)/gi,
    String(statValue),
  );
  expression = expression.replace(
    /\bskill\s*\(\s*'([^']*)'\s*\.\s*([A-Za-z0-9_]+)\s*\)/gi,
    (match, referencedName, property) => {
      const value = referencedSkillProperty(record, referencedName, property, level, referencedSkillLevel, options);
      return value === null ? match : String(value);
    },
  );
  try {
    const tokens = tokenizeFormula(expression);
    const value = parseFormulaExpression(tokens, record, level, options);
    if (!Number.isFinite(value)) throw new UnsupportedFormula('non-finite result');
    return {
      ok: true,
      status: 'EXACT_FORMULA',
      value,
      normalizedQuotes,
      int32Overflow: value < -2147483648 || value > 2147483647,
    };
  } catch (error) {
    if (error instanceof UnsupportedFormula) {
      const status = error.message.startsWith('unsupported identifier')
        ? 'UNSUPPORTED_IDENTIFIER'
        : 'SYMBOLIC';
      return { ok: false, status, normalizedQuotes, reason: error.message };
    }
    throw error;
  }
}

function markdownValue(value) {
  if (value === MISSING_ROW) return '`<ligne absente>`';
  if (value === undefined) return '`<colonne absente>`';
  if (value === null || String(value) === '') return '∅';
  const rendered = String(value)
    .replaceAll('|', '\\|')
    .replaceAll('\r', '\\r')
    .replaceAll('\n', '\\n');
  const backtickRuns = rendered.match(/`+/g) ?? [];
  if (!backtickRuns.length) return `\`${rendered}\``;
  const fence = '`'.repeat(Math.max(...backtickRuns.map((run) => run.length)) + 1);
  return `${fence} ${rendered} ${fence}`;
}

function formatNumber(value) {
  if (value === null || value === undefined || !Number.isFinite(value)) return '∅';
  const rounded = Math.round(value * 10000) / 10000;
  return Number.isInteger(rounded) ? String(rounded) : String(rounded);
}

function formatRange(damage) {
  if (!damage) return '∅';
  if (damage.min === null || damage.max === null) {
    return `Min=${damage.min === null ? '∅' : formatNumber(damage.min)}; Max=${damage.max === null ? '∅' : formatNumber(damage.max)}`;
  }
  const exact = `${formatNumber(damage.min)}–${formatNumber(damage.max)}`;
  const tooltipMin = trunc(damage.min);
  const tooltipMax = trunc(damage.max);
  if (tooltipMin === damage.min && tooltipMax === damage.max) return exact;
  return `${exact} (tooltip ${tooltipMin}–${tooltipMax})`;
}

function fieldGroup(header) {
  const field = canonicalHeader(header);
  if (['id', 'skill', 'charclass', 'skilldesc'].includes(field)) return 'Identité';
  if (FUNCTION_FIELDS.has(field) || /func\d*$/.test(field)) return 'Fonctions moteur';
  if (/missile/.test(field)) return 'Missiles';
  if (/^(aura|passive|state\d|periodic)/.test(field)) return 'Aura, passif et états';
  if (/^(summon|pet|sum)/.test(field)) return 'Invocation';
  if (/^(req|skpoints|maxlvl|restrict|itype|etype|requires)/.test(field)) return 'Prérequis et armes';
  if (/^(mana|lvlmana|minmana|manashift|delay|localdelay|globaldelay|perdelay|interrupt|repeat)/.test(field)) return 'Coût et timing';
  if (/^(calc|param|tohit|levtohit|srcdam|hitshift|min|emax|emin|maxdam|dmg|etype$|elen)/.test(field)) return 'Valeurs, dégâts et formules';
  if (/^(anim|seq|range|target|search|select|warp|lineofsight|alwayshit)/.test(field)) return 'Animation et ciblage';
  if (/^(leftskill|rightskill|scroll|ingame|intown)/.test(field)) return 'Interface et disponibilité';
  return 'Autres';
}

function isCommentHeader(header) {
  const canonical = canonicalHeader(header);
  return canonical.startsWith('*') || canonical === 'eol';
}

export function diffRecords(pd2, bkvince) {
  if (!pd2 && !bkvince) return [];
  if (!pd2 || !bkvince) {
    const present = pd2 || bkvince;
    const differences = [];
    for (const field of [...present.document.canonicalIndexes.keys()].sort((left, right) => left.localeCompare(right, 'en'))) {
      if (isCommentHeader(field) || field === '*eol') continue;
      const value = present.get(field);
      if (value === undefined || String(value).trim() === '') continue;
      differences.push({
        field,
        group: fieldGroup(field),
        pd2: pd2 ? value : MISSING_ROW,
        bkvince: bkvince ? value : MISSING_ROW,
      });
    }
    return differences;
  }
  const pd2Headers = pd2.document.canonicalIndexes;
  const bkvHeaders = bkvince.document.canonicalIndexes;
  const all = new Set([...pd2Headers.keys(), ...bkvHeaders.keys()]);
  const differences = [];
  for (const field of [...all].sort((left, right) => left.localeCompare(right, 'en'))) {
    if (isCommentHeader(field) || field === '*eol') continue;
    const pd2Value = pd2.get(field);
    const bkvValue = bkvince.get(field);
    if (pd2Value === undefined && (bkvValue === undefined || bkvValue === '')) continue;
    if (bkvValue === undefined && (pd2Value === undefined || pd2Value === '')) continue;
    if (String(pd2Value ?? '') === String(bkvValue ?? '')) continue;
    differences.push({
      field,
      group: fieldGroup(field),
      pd2: pd2Value,
      bkvince: bkvValue,
    });
  }
  return differences;
}

function recordDescription(record, field) {
  if (!record) return '';
  const match = canonicalHeader(field).match(/^param(\d{1,2})$/);
  if (!match) return '';
  for (const [header, index] of record.document.canonicalIndexes.entries()) {
    if (header.startsWith(`*param${match[1]} description`)) {
      return record.row[index] ?? '';
    }
  }
  return '';
}

function renderParameters(pd2, bkvince) {
  const rows = [];
  for (let index = 1; index <= 20; index += 1) {
    const field = `param${index}`;
    const pd2Value = pd2?.get(field);
    const bkvValue = bkvince?.get(field);
    if ([pd2Value, bkvValue].every((value) => value === undefined || value === '')) continue;
    rows.push([
      `Param${index}`,
      pd2Value,
      recordDescription(pd2, field),
      bkvValue,
      recordDescription(bkvince, field),
    ]);
  }
  if (!rows.length) return '';
  return [
    '**Paramètres déclarés**',
    '',
    '| Paramètre | PD2 | Description PD2 | BKVince | Description BKVince |',
    '|---|---:|---|---:|---|',
    ...rows.map(([field, pd2Value, pd2Description, bkvValue, bkvDescription]) => (
      `| ${field} | ${markdownValue(pd2Value)} | ${markdownValue(pd2Description)} | ${markdownValue(bkvValue)} | ${markdownValue(bkvDescription)} |`
    )),
    '',
  ].join('\n');
}

function renderCurveTable(pd2, bkvince, levels = DEFAULT_LEVELS) {
  const hasMana = levels.some((level) => manaCostAtLevel(pd2 ?? bkvince, level) !== null)
    || (pd2 && levels.some((level) => manaCostAtLevel(pd2, level) !== null))
    || (bkvince && levels.some((level) => manaCostAtLevel(bkvince, level) !== null));
  const hasPhysical = levels.some((level) => damageAtLevel(pd2 ?? bkvince, level, false))
    || (pd2 && levels.some((level) => damageAtLevel(pd2, level, false)))
    || (bkvince && levels.some((level) => damageAtLevel(bkvince, level, false)));
  const hasElemental = levels.some((level) => damageAtLevel(pd2 ?? bkvince, level, true))
    || (pd2 && levels.some((level) => damageAtLevel(pd2, level, true)))
    || (bkvince && levels.some((level) => damageAtLevel(bkvince, level, true)));
  const hasLength = (pd2 && elementalLengthAtLevel(pd2, 1) !== null)
    || (bkvince && elementalLengthAtLevel(bkvince, 1) !== null);
  if (!hasMana && !hasPhysical && !hasElemental && !hasLength) return '';

  const lines = [
    '**Courbes calculables sans équipement ni synergie (`EXACT_DERIVED`)**',
    '',
    '> Les colonnes `MinDam/MaxDam` et `EMin/EMax` utilisent les cinq paliers de `skills.txt` et `HitShift`. Certaines fonctions les réemploient pour une autre grandeur : le libellé reste donc celui de la colonne, pas une interprétation gameplay. Pour les poisons, la valeur est encodée avant interprétation durée/DPS. Une courbe calculable ne prouve pas que la fonction native consomme le champ.',
    '',
    '| Niveau | Mana PD2 | Mana BKV | MinDam/MaxDam PD2 | MinDam/MaxDam BKV | EMin/EMax PD2 | EMin/EMax BKV | ELen PD2 | ELen BKV |',
    '|---:|---:|---:|---:|---:|---:|---:|---:|---:|',
  ];
  for (const level of levels) {
    const pd2Physical = pd2 ? damageAtLevel(pd2, level, false) : null;
    const bkvPhysical = bkvince ? damageAtLevel(bkvince, level, false) : null;
    const pd2Elemental = pd2 ? damageAtLevel(pd2, level, true) : null;
    const bkvElemental = bkvince ? damageAtLevel(bkvince, level, true) : null;
    const pd2Type = pd2?.get('etype') || '';
    const bkvType = bkvince?.get('etype') || '';
    const pd2Length = pd2 ? elementalLengthAtLevel(pd2, level) : null;
    const bkvLength = bkvince ? elementalLengthAtLevel(bkvince, level) : null;
    lines.push(
      `| ${level} | ${formatNumber(pd2 ? manaCostAtLevel(pd2, level) : null)} | ${formatNumber(bkvince ? manaCostAtLevel(bkvince, level) : null)} | ${formatRange(pd2Physical)} | ${formatRange(bkvPhysical)} | ${pd2Elemental ? `${pd2Type || 'élément'} ${formatRange(pd2Elemental)}` : '∅'} | ${bkvElemental ? `${bkvType || 'élément'} ${formatRange(bkvElemental)}` : '∅'} | ${pd2Length === null ? '∅' : `${formatNumber(pd2Length)} f / ${formatNumber(pd2Length / 25)} s`} | ${bkvLength === null ? '∅' : `${formatNumber(bkvLength)} f / ${formatNumber(bkvLength / 25)} s`} |`,
    );
  }
  lines.push('');
  return lines.join('\n');
}

function renderFormulaSamples(pd2, bkvince, levels = DEFAULT_LEVELS) {
  const fields = new Set();
  for (const record of [pd2, bkvince]) {
    if (!record) continue;
    for (const field of record.document.canonicalIndexes.keys()) {
      if (FORMULA_FIELD_PATTERN.test(field)) {
        const value = record.get(field);
        if (value !== undefined && value !== '') fields.add(field);
      }
    }
  }
  if (!fields.size) return '';
  const lines = [
    '**Formules et échantillons contrôlés (`lvl = niveau effectif`, `blvl = min(lvl, maxlvl)`, `stat(...) = 0`)**',
    '',
    '| Champ | Formule PD2 | Valeurs PD2 L1/L10/L20/L40 | Formule BKV | Valeurs BKV L1/L10/L20/L40 |',
    '|---|---|---|---|---|',
  ];
  const sampleLevels = [1, 10, 20, 40];
  for (const field of [...fields].sort()) {
    const pFormula = pd2?.get(field);
    const bFormula = bkvince?.get(field);
    const samples = (record, formula) => {
      if (!record || formula === undefined || formula === '') return '∅';
      return sampleLevels.map((level) => {
        const result = evaluateSimpleFormula(formula, record, level);
        return result.ok
          ? `L${level}=${formatNumber(result.value)}${result.int32Overflow ? '[INT32_OVERFLOW]' : ''}`
          : `L${level}=?[${result.status || 'SYMBOLIC'}]`;
      }).join(', ');
    };
    lines.push(`| \`${field}\` | ${markdownValue(pFormula)} | ${samples(pd2, pFormula)} | ${markdownValue(bFormula)} | ${samples(bkvince, bFormula)} |`);
  }
  lines.push('', '> `?` signifie que la formule dépend encore d’un contexte non fixé (`skill(... .parN)`, unité, niveau d’unité, random ou callback natif). Les stats et les niveaux des autres skills valent zéro dans ce scénario naked/unsynergized; la formule brute reste la preuve autoritaire.', '');
  return lines.join('\n');
}

function renderDifferences(differences, title = 'Différences exactes de cellules') {
  if (!differences.length) return `**${title}** : aucune différence sur les colonnes comparables non documentaires.\n`;
  const lines = [
    `<details><summary>${title} — ${differences.length} cellule(s)</summary>`,
    '',
    '| Groupe | Colonne | PD2 | BKVince |',
    '|---|---|---|---|',
    ...differences.map((difference) => (
      `| ${difference.group} | \`${difference.field}\` | ${markdownValue(difference.pd2)} | ${markdownValue(difference.bkvince)} |`
    )),
    '',
    '</details>',
    '',
  ];
  return lines.join('\n');
}

function referencedValues(record, matcher) {
  if (!record) return new Map();
  const result = new Map();
  for (const [field, index] of record.document.canonicalIndexes.entries()) {
    if (isCommentHeader(field) || !matcher(field)) continue;
    const value = record.row[index];
    if (value !== undefined && value !== '') result.set(field, value);
  }
  return result;
}

function uniqueRecordByKey(document, key) {
  if (!document || key === undefined || key === '') return null;
  const matches = document.byKey.get(normalizedName(key)) ?? [];
  return matches.length === 1 ? matches[0] : null;
}

function renderLinkedRows(label, pd2Skill, bkvSkill, pd2Document, bkvDocument, matcher) {
  const pd2Refs = referencedValues(pd2Skill, matcher);
  const bkvRefs = referencedValues(bkvSkill, matcher);
  const fields = new Set([...pd2Refs.keys(), ...bkvRefs.keys()]);
  if (!fields.size) return '';
  const lines = [`**${label} référencés directement**`, ''];
  for (const field of [...fields].sort()) {
    const pd2Name = pd2Refs.get(field);
    const bkvName = bkvRefs.get(field);
    const pd2Row = uniqueRecordByKey(pd2Document, pd2Name);
    const bkvRow = uniqueRecordByKey(bkvDocument, bkvName);
    lines.push(`- \`${field}\` : PD2 ${markdownValue(pd2Name)}${pd2Row ? ` (ligne ${pd2Row.line})` : ''}; BKVince ${markdownValue(bkvName)}${bkvRow ? ` (ligne ${bkvRow.line})` : ''}.`);
    if (pd2Row || bkvRow) {
      lines.push('', renderDifferences(diffRecords(pd2Row, bkvRow), `Différences ${label.toLowerCase()} pour ${field}`).trim(), '');
    }
  }
  lines.push('');
  return lines.join('\n');
}

function buildReferenceIndex(documents) {
  const index = new Map();
  for (const document of Object.values(documents)) {
    if (!REFERENCE_TABLES.has(document.requestedName)) continue;
    for (const record of document.records) {
      for (const [header, column] of document.canonicalIndexes.entries()) {
        if (isCommentHeader(header)) continue;
        const value = String(record.row[column] ?? '').trim();
        if (!value) continue;
        const key = normalizedName(value);
        if (!index.has(key)) index.set(key, []);
        index.get(key).push({
          table: document.requestedName,
          line: record.line,
          row: record.key,
          field: header,
        });
      }
    }
  }
  return index;
}

function renderConsumers(skillName, pd2References, bkvReferences) {
  const pd2 = pd2References.get(normalizedName(skillName)) ?? [];
  const bkv = bkvReferences.get(normalizedName(skillName)) ?? [];
  if (!pd2.length && !bkv.length) return '';
  const format = (references) => references.map((reference) => (
    `\`${reference.table}:${reference.line} ${reference.row || '?'}[${reference.field}]\``
  )).join(', ') || 'aucune référence exacte';
  return [
    '**Références exactes par nom dans les tables consommatrices**',
    '',
    `- PD2 : ${format(pd2)}`,
    `- BKVince : ${format(bkv)}`,
    '',
    '> Cet index couvre les références textuelles exactes. Les consommateurs encodés uniquement par ID, propriété ou callback natif restent signalés par les diffs de tables et ne sont pas inférés.',
    '',
  ].join('\n');
}

function technicalRoute(entry, differences) {
  const skillName = normalizedName(entry.pd2?.get('skill') || entry.bkvince?.get('skill'));
  const curated = CURATED_DECISIONS[skillName];
  if (curated) return { disposition: curated[0], rationale: curated[1], curated: true };
  if (entry.status === 'same-id-different-name') {
    return { disposition: 'NE PAS COPIER', rationale: "Même ordinal runtime, identité différente : migration sémantique et sauvegardes à gouverner.", curated: false };
  }
  if (entry.status === 'same-name-moved-id') {
    return { disposition: 'REMAP OBLIGATOIRE', rationale: "Le même nom utilise un autre ordinal runtime; aucune copie de ligne ou de dépendance n'est sûre.", curated: false };
  }
  if (entry.status === 'pd2-only') {
    return { disposition: 'NOUVEAU SKILL — DIFFÉRER', rationale: 'Nouvel ID, UI, dépendances, sauvegardes et fonctions à prouver.', curated: false };
  }
  if (entry.status === 'bkv-only') {
    return { disposition: 'CONSERVER BKVINCE', rationale: "Aucun équivalent PD2 direct; préserver l'identité et les consommateurs existants.", curated: false };
  }
  if (!differences.length) {
    return { disposition: 'AUCUN EMPRUNT NÉCESSAIRE', rationale: 'Aucune différence de valeur sur les colonnes comparables.', curated: false };
  }
  if (differences.some((difference) => difference.group === 'Fonctions moteur')) {
    return { disposition: 'PREUVE NATIVE AVANT ADAPTATION', rationale: 'Au moins une fonction serveur/client diffère entre PD2 et D2R.', curated: false };
  }
  return { disposition: 'ARBITRAGE SOFTCODE', rationale: 'Valeurs ou formules différentes sur une identité commune; mesurer avant adaptation.', curated: false };
}

function statusLabel(status) {
  return ({
    'same-name-same-id': 'même nom, même ordinal runtime',
    'same-name-moved-id': 'même nom, ordinal runtime déplacé',
    'same-id-different-name': 'même ordinal runtime, skill différent',
    'pd2-only': 'PD2 uniquement',
    'bkv-only': 'BKVince uniquement',
  })[status] ?? status;
}

function renderSkillEntry(entry, sources, references, headingLevel = 3) {
  const pd2 = entry.pd2;
  const bkvince = entry.bkvince;
  const pd2Name = pd2?.get('skill');
  const bkvName = bkvince?.get('skill');
  const title = pd2Name && bkvName && pd2Name !== bkvName
    ? `${pd2Name} ↔ ${bkvName}`
    : (pd2Name || bkvName || 'Skill sans nom');
  const differences = diffRecords(pd2, bkvince);
  const route = technicalRoute(entry, differences);
  const lines = [
    `${'#'.repeat(headingLevel)} ${title}`,
    '',
    `- Correspondance : **${statusLabel(entry.status)}**.`,
    `- PD2 : ${pd2 ? `ordinal runtime \`${runtimeSkillOrdinal(pd2)}\`, Id déclaré \`${declaredSkillId(pd2)}\`, classe \`${pd2.get('charclass') || '—'}\`, \`${pd2.document.relativePath}:${pd2.line}\`` : 'absent'}.`,
    `- BKVince : ${bkvince ? `ordinal runtime \`${runtimeSkillOrdinal(bkvince)}\`, *Id documentaire \`${declaredSkillId(bkvince)}\`, classe \`${bkvince.get('charclass') || '—'}\`, \`${bkvince.document.relativePath}:${bkvince.line}\`` : 'absent'}.`,
    `- Disposition : **${route.disposition}** — ${route.rationale}`,
  ];
  if (entry.ambiguity) lines.push(`- Ambiguïté : **${entry.ambiguity}**.`);
  lines.push('');

  const curves = renderCurveTable(pd2, bkvince);
  if (curves) lines.push(curves);
  const formulas = renderFormulaSamples(pd2, bkvince);
  if (formulas) lines.push(formulas);
  const parameters = renderParameters(pd2, bkvince);
  if (parameters) lines.push(parameters);

  lines.push(renderDifferences(differences));
  lines.push(renderLinkedRows(
    'Missiles',
    pd2,
    bkvince,
    sources.pd2['missiles.txt'],
    sources.bkvince['missiles.txt'],
    (field) => field.includes('missile'),
  ));
  lines.push(renderLinkedRows(
    'États',
    pd2,
    bkvince,
    sources.pd2['states.txt'],
    sources.bkvince['states.txt'],
    (field) => ['aurastate', 'auratargetstate', 'passivestate', 'state1', 'state2', 'state3'].includes(field),
  ));
  lines.push(renderLinkedRows(
    'Statistiques ItemStatCost',
    pd2,
    bkvince,
    sources.pd2['itemstatcost.txt'],
    sources.bkvince['itemstatcost.txt'],
    (field) => /^(aurastat[1-6]|passivestat(?:[1-9]|1[0-4]))$/.test(field),
  ));
  lines.push(renderLinkedRows(
    'Descriptions de skill',
    pd2,
    bkvince,
    sources.pd2['skilldesc.txt'],
    sources.bkvince['skilldesc.txt'],
    (field) => field === 'skilldesc',
  ));
  lines.push(renderLinkedRows(
    'Types de familier',
    pd2,
    bkvince,
    sources.pd2['pettype.txt'],
    sources.bkvince['pettype.txt'],
    (field) => field === 'pettype',
  ));
  lines.push(renderLinkedRows(
    'Monstres invoqués',
    pd2,
    bkvince,
    sources.pd2['monstats.txt'],
    sources.bkvince['monstats.txt'],
    (field) => field === 'summon',
  ));
  lines.push(renderConsumers(pd2Name || bkvName, references.pd2, references.bkvince));
  return lines.filter((line) => line !== '').join('\n\n').replace(/\n{3,}/g, '\n\n').trim() + '\n';
}

function playerClassesForEntry(entry) {
  const pd2Class = entry.pd2?.get('charclass');
  const bkvClass = entry.bkvince?.get('charclass');
  return [...new Set([pd2Class, bkvClass].filter((classCode) => SKILL_CLASS_SET.has(classCode)))];
}

function hasClasslessSide(entry) {
  return [entry.pd2, entry.bkvince].some((record) => (
    record && !SKILL_CLASS_SET.has(record.get('charclass'))
  ));
}

function renderCoverageTable(mapping) {
  const playerPd2 = mapping.entries.filter((entry) => entry.pd2 && SKILL_CLASS_SET.has(entry.pd2.get('charclass')));
  const playerBkv = mapping.entries.filter((entry) => entry.bkvince && SKILL_CLASS_SET.has(entry.bkvince.get('charclass')));
  const lines = [
    '| Population | Nombre | Gate |',
    '|---|---:|---|',
    `| Toutes les lignes PD2 nommées | ${mapping.pd2Rows} | ${mapping.mappedPd2Rows === mapping.pd2Rows ? 'couvertes' : 'ÉCHEC'} |`,
    `| Toutes les lignes BKVince nommées | ${mapping.bkvRows} | ${mapping.mappedBkvRows === mapping.bkvRows ? 'couvertes' : 'ÉCHEC'} |`,
    `| Skills joueurs PD2 | ${playerPd2.length} | détaillés |`,
    `| Skills joueurs BKVince, Warlock inclus | ${playerBkv.length} | détaillés |`,
    `| Ambiguïtés de mapping | ${mapping.ambiguous.length} | ${mapping.ambiguous.length ? 'à examiner' : 'aucune'} |`,
    `| Collisions d'ordinal runtime PD2/BKVince | ${mapping.runtimeCollisions.length} | inventoriées |`,
    `| \`Id\` PD2 différent de l'ordinal | ${mapping.declaredIdAnomalies.pd2.length} | ${mapping.declaredIdAnomalies.pd2.length ? 'à examiner' : 'aucune'} |`,
    `| \`*Id\` BKVince différent de l'ordinal | ${mapping.declaredIdAnomalies.bkvince.length} | inventoriés comme commentaires |`,
  ];
  for (const [status, count] of Object.entries(mapping.counts)) {
    lines.push(`| Mapping \`${status}\` | ${count} | inventorié |`);
  }
  return lines.join('\n');
}

function actualHeader(document, canonical) {
  const index = document.canonicalIndexes.get(canonical);
  return index === undefined ? '' : document.table.headers[index];
}

function commonComparableFields(pd2Document, bkvDocument) {
  return [...pd2Document.canonicalIndexes.keys()]
    .filter((field) => bkvDocument.canonicalIndexes.has(field))
    .filter((field) => !actualHeader(pd2Document, field).trim().startsWith('*'))
    .filter((field) => !actualHeader(bkvDocument, field).trim().startsWith('*'));
}

function commonCellDiffCount(pd2, bkvince, fields) {
  return fields.reduce((count, field) => (
    String(pd2.get(field) ?? '') === String(bkvince.get(field) ?? '') ? count : count + 1
  ), 0);
}

function nonemptyCount(document, field) {
  if (!document.canonicalIndexes.has(field)) return null;
  return document.records.filter((record) => (
    String(record.get(field) ?? '').trim() !== '' && String(record.get('skill') ?? '').trim() !== ''
  )).length;
}

function renderDistribution(document, field) {
  if (!document.canonicalIndexes.has(field)) return '`<colonne absente>`';
  const named = document.records.filter((record) => String(record.get('skill') ?? '').trim());
  const counts = new Map();
  for (const record of named) {
    const value = String(record.get(field) ?? '').trim();
    counts.set(value, (counts.get(value) ?? 0) + 1);
  }
  const values = [...counts.entries()].sort(([left], [right]) => left.localeCompare(right, 'en', { numeric: true }));
  return values.map(([value, count]) => `${markdownValue(value)} × ${count}`).join('<br>');
}

function renderGlobalAudit(sources, mapping) {
  const pd2 = sources.pd2['skills.txt'];
  const bkv = sources.bkvince['skills.txt'];
  const pd2Fields = new Set(pd2.canonicalIndexes.keys());
  const bkvFields = new Set(bkv.canonicalIndexes.keys());
  const pd2LiteralFields = new Set(pd2.table.headers.map((header) => String(header).trim().toLowerCase()));
  const bkvLiteralFields = new Set(bkv.table.headers.map((header) => String(header).trim().toLowerCase()));
  const literalCommon = [...pd2LiteralFields].filter((field) => bkvLiteralFields.has(field));
  const literalPd2Only = [...pd2LiteralFields].filter((field) => !bkvLiteralFields.has(field));
  const literalBkvOnly = [...bkvLiteralFields].filter((field) => !pd2LiteralFields.has(field));
  const common = [...pd2Fields].filter((field) => bkvFields.has(field));
  const pd2Only = [...pd2Fields].filter((field) => !bkvFields.has(field)).sort();
  const bkvOnly = [...bkvFields].filter((field) => !pd2Fields.has(field)).sort();
  const comparable = commonComparableFields(pd2, bkv);
  const semanticDiffs = mapping.semanticNamePairs.map((pair) => ({
    ...pair,
    count: commonCellDiffCount(pair.pd2, pair.bkvince, comparable),
  }));
  const totalSemanticDiffs = semanticDiffs.reduce((total, pair) => total + pair.count, 0);
  const identical = semanticDiffs.filter((pair) => pair.count === 0).map((pair) => pair.pd2.get('skill'));

  const pd2Names = new Set(pd2.records.map((record) => normalizedName(record.get('skill'))));
  const bkvNames = new Set(bkv.records.map((record) => normalizedName(record.get('skill'))));
  const scopes = [...CLASS_ORDER, 'classless'];
  const scopeRows = scopes.map((scope) => {
    const pd2Rows = pd2.records.filter((record) => (
      scope === 'classless'
        ? !SKILL_CLASS_SET.has(record.get('charclass'))
        : record.get('charclass') === scope
    ));
    const bkvRows = bkv.records.filter((record) => (
      scope === 'classless'
        ? !SKILL_CLASS_SET.has(record.get('charclass'))
        : record.get('charclass') === scope
    ));
    return {
      scope,
      pd2Rows,
      bkvRows,
      pd2Shared: pd2Rows.filter((record) => bkvNames.has(normalizedName(record.get('skill')))).length,
      bkvShared: bkvRows.filter((record) => pd2Names.has(normalizedName(record.get('skill')))).length,
      commonDiffs: semanticDiffs
        .filter((pair) => (
          scope === 'classless'
            ? !SKILL_CLASS_SET.has(pair.pd2.get('charclass'))
            : pair.pd2.get('charclass') === scope
        ))
        .reduce((total, pair) => total + pair.count, 0),
    };
  });

  const profileFields = [
    'maxlvl', 'delay', 'localdelay', 'globaldelay', 'perdelay',
    'interrupt', 'repeat', 'usemanaondo', 'itemeffect',
    'leftskill', 'rightskill', 'intown', 'ingame',
    'manashift', 'hitshift', 'srcdam',
  ];

  return `## Audit global chiffré du système de skills

### Population, identité et volume de changements

| Portée | Lignes PD2 | Noms aussi dans BKV | Lignes BKV | Noms aussi dans PD2 | Diffs de cellules communes non documentaires |
|---|---:|---:|---:|---:|---:|
${scopeRows.map((row) => `| ${row.scope === 'classless' ? 'Classless/technique' : CLASS_NAMES[row.scope]} | ${row.pd2Rows.length} | ${row.pd2Shared} | ${row.bkvRows.length} | ${row.bkvShared} | ${row.commonDiffs} |`).join('\n')}

- Noms communs : **${mapping.semanticCounts.sharedNames}**; mêmes ordinals runtime : **${mapping.semanticCounts.sameRuntimeOrdinal}**; noms déplacés : **${mapping.semanticCounts.movedRuntimeOrdinal}**.
- Noms propres à PD2 : **${mapping.semanticCounts.pd2OnlyNames}**; noms propres à BKVince : **${mapping.semanticCounts.bkvOnlyNames}**.
- Sur les ${mapping.semanticCounts.sharedNames} paires nominales, **${totalSemanticDiffs}** cellules diffèrent dans les ${comparable.length} colonnes communes non documentaires.
- Paires sans aucune différence dans ces colonnes : ${identical.length ? identical.map((name) => `\`${name}\``).join(', ') : 'aucune'}.

### Schéma exact

- Headers littéralement communs (casse ignorée) : **${literalCommon.length}**; propres à PD2 : **${literalPd2Only.length}**; propres à BKVince : **${literalBkvOnly.length}**.
- Après normalisation gouvernée de \`Id\` PD2 ↔ \`*Id\` BKVince : **${common.length}** communs, **${pd2Only.length}** propres à PD2 et **${bkvOnly.length}** propres à BKVince.
- \`delay\` PD2 n'est pas assimilé automatiquement à \`localdelay\` ou \`globaldelay\` D2R; leurs consommations moteur doivent être prouvées séparément.

<details><summary>Colonnes propres à une source et nombre de cellules non vides</summary>

| Source | Colonne | Lignes nommées non vides |
|---|---|---:|
${pd2Only.map((field) => `| PD2 | \`${field}\` | ${nonemptyCount(pd2, field)} |`).join('\n')}
${bkvOnly.map((field) => `| BKVince | \`${field}\` | ${nonemptyCount(bkv, field)} |`).join('\n')}

</details>

### Distributions exactes des champs transversaux

Chaque distribution inclut les cellules vides (\`∅\`). \`ItemEffect\` est une fonction serveur déclenchée par un item, **pas** une preuve qu'un skill est « item-only ».

| Champ | Distribution PD2 | Distribution BKVince |
|---|---|---|
${profileFields.map((field) => `| \`${field}\` | ${renderDistribution(pd2, field)} | ${renderDistribution(bkv, field)} |`).join('\n')}

Les numéros de fonctions, les délais et les flags ci-dessus sont \`EXACT_TABLE\`. Leur équivalence native entre le moteur PD2 et D2R 3.2 n'est pas présumée.
`;
}

function skillByExactName(document, name) {
  return document.records.find((record) => record.get('skill') === name) ?? null;
}

function passiveStats(record) {
  if (!record) return [];
  return Array.from({ length: 14 }, (_unused, index) => record.get(`passivestat${index + 1}`))
    .filter((value) => value !== undefined && String(value).trim() !== '');
}

function durationSample(record, level) {
  if (!record) return 'absent';
  const formula = record.get('auralencalc');
  const evaluated = evaluateSimpleFormula(formula, record, level);
  if (!evaluated.ok) return `${markdownValue(formula)} → ?[${evaluated.status}]`;
  return `${formatNumber(evaluated.value)} frames / ${formatNumber(evaluated.value / 25)} s`;
}

function renderGlobalMechanicDecisions(sources) {
  const pd2Skills = sources.pd2['skills.txt'];
  const bkvSkills = sources.bkvince['skills.txt'];
  const pd2Properties = sources.pd2['properties.txt'];
  const bkvProperties = sources.bkvince['properties.txt'];
  const pd2ItemStatCost = sources.pd2['itemstatcost.txt'];
  const bkvItemStatCost = sources.bkvince['itemstatcost.txt'];
  const forceMove = skillByExactName(pd2Skills, 'Force Move');
  const bkvForceMove = skillByExactName(bkvSkills, 'Force Move');
  const uiPath = path.join(
    repoRoot,
    'data-BKVince',
    'BKVince.mpq',
    'data',
    'local',
    'lng',
    'strings',
    'ui.json',
  );
  const bkvUiForceMove = fs.existsSync(uiPath) && fs.readFileSync(uiPath, 'utf8').includes('CfgForceMove');
  const trapNames = [
    'Charged Bolt Sentry',
    'Wake of Fire Sentry',
    'Lightning Sentry',
    'Inferno Sentry',
    'Death Sentry',
  ];
  const buffNames = ['Quickness', 'Fade', 'Venom', 'Blade Shield'];
  const propertyNames = [
    'oskill', 'aura', 'skill', 'charged', 'hit-skill', 'gethit-skill', 'kill-skill',
    'rep-charge', 'equipped-skill', 'cast-skill', 'block-skill',
  ];
  const propertyPresence = (document, code) => (document.byKey.get(normalizedName(code)) ?? []).length;

  const lines = [
    '## Changements globaux PD2 : état exact et décision BKVince',
    '',
    '| Concept global | Preuve PD2/SP+ | Preuve BKVince | Décision |',
    '|---|---|---|---|',
    `| Move Only / Force Move | ${forceMove ? `ordinal ${forceMove.index}; fonctions srv ${markdownValue(forceMove.get('srvstfunc'))}/${markdownValue(forceMove.get('srvdofunc'))}, client ${markdownValue(forceMove.get('cltstfunc'))}/${markdownValue(forceMove.get('cltdofunc'))}` : 'ligne absente'} | ${bkvForceMove ? `ligne ordinal ${bkvForceMove.index}` : 'aucune ligne skill'}; clé UI \`CfgForceMove\` ${bkvUiForceMove ? 'présente' : 'absente'} dans \`${path.relative(repoRoot, uiPath).replaceAll('\\', '/')}\` | **Ne pas porter** : fonction déjà exposée par D2R/BKVince. |`,
    `| Suppression du cooldown global partagé | PD2 emploie le header \`delay\` sur ${nonemptyCount(pd2Skills, 'delay')} lignes nommées | BKVince expose \`localdelay\`/\`globaldelay\`; distributions exactes dans le chapitre global | **Ne pas traduire automatiquement** \`delay\`; auditer skill par skill. |`,
    '| Improved summon AI | Non démontrable par les cellules TXT seules | Aucune preuve globale dans les tables chargées | **Différer** : preuve runtime/native requise. |',
    '| Pénalité de réduction de résistance aux immunités | Règle moteur PD2, pas une cellule de `skills.txt` | Aucun équivalent prouvé par ce rapport TXT | **Différer** : aucun emprunt data-only démontré. |',
    '| Skills on items étendus | Quatre propriétés supplémentaires existent, voir matrice ci-dessous | Propriétés et ItemStatCost correspondants absents | **Rejeter le copier-coller** : chantier natif/save séparé. |',
    '',
    '### Transmission des masteries et pierces aux traps',
    '',
    '| Trap | Ordinal PD2 | Passive stats PD2 | Ordinal BKV | Passive stats BKVince | Décision |',
    '|---|---:|---|---:|---|---|',
  ];
  for (const name of trapNames) {
    const pd2 = skillByExactName(pd2Skills, name);
    const bkv = skillByExactName(bkvSkills, name);
    lines.push(`| ${name} | ${pd2?.index ?? '—'} | ${pd2 ? passiveStats(pd2).map((value) => `\`${value}\``).join(', ') || 'aucune' : 'absent'} | ${bkv?.index ?? '—'} | ${bkv ? passiveStats(bkv).map((value) => `\`${value}\``).join(', ') || 'aucune' : 'absent'} | **Déjà présent, et plus large dans BKVince**; ne rien dupliquer. |`);
  }
  lines.push(
    '',
    '### Durées des buffs Assassin',
    '',
    'Les durées sont des valeurs brutes de `auralencalc` converties à 25 frames/s. Elles ne préjugent pas des dispels, de la mort ou des transitions de zone.',
    '',
    '| Skill | PD2 L1 | PD2 L20 | BKVince L1 | BKVince L20 | Décision |',
    '|---|---:|---:|---:|---:|---|',
  );
  for (const name of buffNames) {
    const pd2 = skillByExactName(pd2Skills, name);
    const bkv = skillByExactName(bkvSkills, name);
    lines.push(`| ${name} | ${durationSample(pd2, 1)} | ${durationSample(pd2, 20)} | ${durationSample(bkv, 1)} | ${durationSample(bkv, 20)} | **Conserver BKVince**; sa durée configurée dépasse déjà largement cinq minutes. |`);
  }
  lines.push(
    '',
    '### Propriétés de skills sur objets',
    '',
    '| Code de propriété | Stat PD2 associée | Stat présente BKV | Lignes PD2 | Lignes BKVince | Portabilité |',
    '|---|---|---:|---:|---:|---|',
  );
  for (const code of propertyNames) {
    const pd2Count = propertyPresence(pd2Properties, code);
    const bkvCount = propertyPresence(bkvProperties, code);
    const pd2Property = uniqueRecordByKey(pd2Properties, code);
    const pd2Stat = pd2Property?.get('stat1') || '';
    const bkvStatCount = pd2Stat ? (bkvItemStatCost.byKey.get(normalizedName(pd2Stat)) ?? []).length : 0;
    const pd2StatCount = pd2Stat ? (pd2ItemStatCost.byKey.get(normalizedName(pd2Stat)) ?? []).length : 0;
    const portable = pd2Count && !bkvCount
      ? '`NATIVE_UNPROVEN` — propriété absente de BKVince'
      : 'mécanisme nominal présent des deux côtés; valeurs d’items à auditer séparément';
    lines.push(`| \`${code}\` | ${pd2Stat ? `${markdownValue(pd2Stat)} (${pd2StatCount})` : '—'} | ${pd2Stat ? bkvStatCount : '—'} | ${pd2Count} | ${bkvCount} | ${portable} |`);
  }
  lines.push('');
  return lines.join('\n');
}

function renderSemanticRelations(mapping) {
  return `## Graphe de correspondance sémantique

Le rapport conserve deux axes indépendants : **nom/identité sémantique** et **occupation de l'ordinal runtime**. Une paire primaire n'efface jamais une collision d'ordinal.

### Noms déplacés

| Skill | Ordinal PD2 | Ordinal BKVince | Classe PD2 | Classe BKVince |
|---|---:|---:|---|---|
${mapping.movedSemanticPairs.map(({ pd2, bkvince }) => `| ${pd2.get('skill')} | ${pd2.index} | ${bkvince.index} | ${pd2.get('charclass') || '—'} | ${bkvince.get('charclass') || '—'} |`).join('\n')}

### Aliases et remplacements gouvernés

| Relation | PD2 | BKVince | Interprétation |
|---|---|---|---|
${GOVERNED_SKILL_RELATIONS.map(([kind, pd2Name, bkvName, note]) => `| \`${kind}\` | ${pd2Name} | ${bkvName} | ${note} |`).join('\n')}

\`Cold Enchant\` doit conserver deux arêtes : correspondance lexicale PD2 ordinal 40 ↔ BKVince ordinal 408, et collision de slot PD2 \`Cold Enchant\` ↔ BKVince \`Frozen Armor\` à l'ordinal 40. De même, les sentries conservent simultanément leurs arêtes de nom et de slot.
`;
}

function scanFormulaIntegrity(document) {
  const skillNames = new Set(document.records.map((record) => normalizedName(record.get('skill'))));
  const result = {
    formulaCells: 0,
    skillCalls: 0,
    skillTargets: new Set(),
    malformed: [],
    unsupportedIdentifiers: [],
    symbolic: [],
    unresolvedFormulaSkills: [],
    directSkillEdges: 0,
    unresolvedDirectSkills: [],
  };

  for (const record of document.records) {
    for (const field of document.canonicalIndexes.keys()) {
      if (!FORMULA_FIELD_PATTERN.test(field)) continue;
      const raw = String(record.get(field) ?? '').trim();
      if (!raw) continue;
      result.formulaCells += 1;
      const evaluated = evaluateSimpleFormula(raw, record, 20);
      if (evaluated.status === 'MALFORMED_SOURCE') {
        result.malformed.push({ record, field, raw, reason: evaluated.reason });
      } else if (evaluated.status === 'UNSUPPORTED_IDENTIFIER') {
        result.unsupportedIdentifiers.push({ record, field, raw, reason: evaluated.reason });
      } else if (evaluated.status === 'SYMBOLIC') {
        result.symbolic.push({ record, field, raw, reason: evaluated.reason });
      }
      for (const match of raw.matchAll(/\bskill\s*\(\s*'([^']+)'\s*\.\s*([A-Za-z0-9_]+)\s*\)/gi)) {
        const target = match[1];
        result.skillCalls += 1;
        result.skillTargets.add(normalizedName(target));
        if (!skillNames.has(normalizedName(target))) {
          result.unresolvedFormulaSkills.push({ record, field, target, property: match[2] });
        }
      }
    }
    for (const field of ['reqskill1', 'reqskill2', 'reqskill3', 'sumskill1', 'sumskill2', 'sumskill3', 'sumskill4', 'sumskill5']) {
      const target = String(record.get(field) ?? '').trim();
      if (!target) continue;
      result.directSkillEdges += 1;
      if (!skillNames.has(normalizedName(target))) {
        result.unresolvedDirectSkills.push({ record, field, target });
      }
    }
  }
  return result;
}

function renderFormulaIntegrity(sources) {
  const scans = {
    pd2: scanFormulaIntegrity(sources.pd2['skills.txt']),
    bkvince: scanFormulaIntegrity(sources.bkvince['skills.txt']),
  };
  const lines = [
    '## Intégrité des formules et dépendances de skills',
    '',
    '| Source | Cellules de formule | Appels `skill()` | Cibles distinctes | Malformées | Identifiants non supportés | Symboliques/contextuelles | Arêtes req/sum | Arêtes non résolues |',
    '|---|---:|---:|---:|---:|---:|---:|---:|---:|',
    `| PD2 | ${scans.pd2.formulaCells} | ${scans.pd2.skillCalls} | ${scans.pd2.skillTargets.size} | ${scans.pd2.malformed.length} | ${scans.pd2.unsupportedIdentifiers.length} | ${scans.pd2.symbolic.length} | ${scans.pd2.directSkillEdges} | ${scans.pd2.unresolvedDirectSkills.length} |`,
    `| BKVince | ${scans.bkvince.formulaCells} | ${scans.bkvince.skillCalls} | ${scans.bkvince.skillTargets.size} | ${scans.bkvince.malformed.length} | ${scans.bkvince.unsupportedIdentifiers.length} | ${scans.bkvince.symbolic.length} | ${scans.bkvince.directSkillEdges} | ${scans.bkvince.unresolvedDirectSkills.length} |`,
    '',
    'Les formules malformées, identifiants non supportés, dépendances contextuelles et références absentes sont conservés comme preuve source; le générateur ne répare ni parenthèse ni alias.',
    '',
  ];

  for (const [sourceName, scan] of Object.entries(scans)) {
    const label = sourceName === 'pd2' ? 'PD2' : 'BKVince';
    const findings = [
      ...scan.malformed.map((finding) => ({ kind: 'MALFORMED_SOURCE', ...finding })),
      ...scan.unsupportedIdentifiers.map((finding) => ({ kind: 'UNSUPPORTED_IDENTIFIER', ...finding })),
      ...scan.symbolic.map((finding) => ({ kind: 'SYMBOLIC', ...finding })),
    ];
    lines.push(`<details><summary>${label} — ${findings.length} formule(s) non résolue(s) numériquement dans le scénario standard</summary>`, '');
    lines.push('| Statut | Skill | Ordinal | Champ | Formule brute | Diagnostic |', '|---|---|---:|---|---|---|');
    for (const finding of findings) {
      lines.push(`| \`${finding.kind}\` | ${finding.record.get('skill')} | ${finding.record.index} | \`${finding.field}\` | ${markdownValue(finding.raw)} | ${markdownValue(finding.reason)} |`);
    }
    if (!findings.length) lines.push('| — | — | — | — | — | aucune |');
    lines.push('', '</details>', '');

    const unresolved = [
      ...scan.unresolvedFormulaSkills.map((finding) => ({ kind: 'formula-skill', ...finding })),
      ...scan.unresolvedDirectSkills.map((finding) => ({ kind: 'req/sum-skill', ...finding })),
    ];
    lines.push(`<details><summary>${label} — ${unresolved.length} référence(s) de skill non résolue(s)</summary>`, '');
    lines.push('| Type | Skill source | Ordinal | Champ | Cible absente |', '|---|---|---:|---|---|');
    for (const finding of unresolved) {
      lines.push(`| \`${finding.kind}\` | ${finding.record.get('skill')} | ${finding.record.index} | \`${finding.field}\` | ${markdownValue(finding.target)} |`);
    }
    if (!unresolved.length) lines.push('| — | — | — | — | aucune |');
    lines.push('', '</details>', '');
  }
  return lines.join('\n');
}

function renderRuntimeIdentityAudit(mapping) {
  const lines = [
    '## Identifiants runtime et collisions',
    '',
    "Dans `skills.txt`, l'identifiant runtime est **l'ordinal de la ligne, en base zéro**. Le champ PD2 `Id` concorde avec cet ordinal. Le champ BKVince `*Id` est une colonne documentaire : il ne doit jamais servir à mapper ou à détecter une collision.",
    '',
    `<details><summary>${mapping.declaredIdAnomalies.bkvince.length} anomalie(s) documentaire(s) BKVince : ordinal runtime ≠ *Id</summary>`,
    '',
    '| Ordinal runtime BKV | Skill | `*Id` documentaire |',
    '|---:|---|---:|',
    ...mapping.declaredIdAnomalies.bkvince.map((record) => (
      `| ${runtimeSkillOrdinal(record)} | ${record.get('skill')} | ${markdownValue(declaredSkillId(record))} |`
    )),
    '',
    '</details>',
    '',
    `<details><summary>${mapping.runtimeCollisions.length} collision(s) sémantique(s) au même ordinal runtime</summary>`,
    '',
    '| Ordinal runtime | Skill PD2 | `Id` PD2 | Skill BKVince | `*Id` BKVince |',
    '|---:|---|---:|---|---:|',
    ...mapping.runtimeCollisions.map(({ pd2, bkvince }) => (
      `| ${runtimeSkillOrdinal(pd2)} | ${pd2.get('skill')} | ${markdownValue(declaredSkillId(pd2))} | ${bkvince.get('skill')} | ${markdownValue(declaredSkillId(bkvince))} |`
    )),
    '',
    '</details>',
    '',
  ];
  return lines.join('\n');
}

function renderExecutiveConclusions() {
  return `## Conclusions décisionnelles consolidées

1. **Aucun import de table ou d'ID PD2 n'est admissible.** Les schémas, fonctions, états, missiles et IDs divergent fortement.
2. **BKVince couvre déjà plusieurs objectifs globaux PD2** : cooldowns locaux, buffs quasi permanents, Fend/Zeal/Fury à trois frappes, Power Strike nova, Frozen Orb/FoH/Immolation sans cooldown, multi-summons et transmission élémentaire aux traps/Hydra.
3. **Les meilleurs candidats de faible portée** restent Inner Sight, les hitboxes/vitesses de certains projectiles, Summon Splash sur Grizzly, Cloak of Shadows et Dragon Claw.
4. **Strafe, Enchant AoE, Sanctuary universel, Corpse Explosion, Life Tap, BO/Oak et les résistances** exigent un prototype ou une rebalance complète, pas une cellule isolée.
5. **Les nouveaux skills PD2** — Blood Warp, Gust, Curse Mastery, Dark Pact, Holy Nova/Light, Joust, Combustion, Lesser Hydra, Chain Lightning Sentry — sont des chantiers append-only avec remapping et preuves natives.

Ces dispositions sont des décisions techniques issues des preuves TXT. La priorité gameplay reste une hypothèse tant qu'un problème BKVince n'a pas été mesuré.
`;
}

function renderSourceManifest(sources) {
  const rows = [];
  for (const sourceName of ['pd2', 'bkvince']) {
    for (const tableName of SOURCE_TABLES) {
      const document = sources[sourceName][tableName];
      rows.push(`| ${sourceName === 'pd2' ? 'PD2/SP+' : 'BKVince'} | \`${document.relativePath}\` | ${document.table.rows.length} | ${document.table.headers.length} | \`${document.sha256}\` |`);
    }
  }
  const skillcalc = sources.reference['skillcalc.txt'];
  rows.push(`| D2R 3.2 de référence | \`${skillcalc.relativePath}\` | ${skillcalc.table.rows.length} | ${skillcalc.table.headers.length} | \`${skillcalc.sha256}\` |`);
  return `### Manifeste des tables liées

<details><summary>${rows.length} fichiers lus avec round-trip byte-exact vérifié</summary>

| Source | Fichier | Lignes | Colonnes | SHA-256 |
|---|---|---:|---:|---|
${rows.join('\n')}

</details>
`;
}

function renderWorkedCorpseExplosion(mapping) {
  const entry = mapping.entries.find((candidate) => normalizedName(candidate.pd2?.get('skill')) === 'corpse explosion');
  if (!entry?.pd2 || !entry.bkvince) return '';
  const p = entry.pd2;
  const b = entry.bkvince;
  const pRadius1 = evaluateSimpleFormula(p.get('aurarangecalc'), p, 1);
  const pRadius20 = evaluateSimpleFormula(p.get('aurarangecalc'), p, 20);
  const bRadius1 = evaluateSimpleFormula(b.get('aurarangecalc'), b, 1);
  const bRadius20 = evaluateSimpleFormula(b.get('aurarangecalc'), b, 20);
  return `## Exemple de lecture exacte — Corpse Explosion

| Valeur | PD2/SP+ observable | BKVince |
|---|---:|---:|
| Part de vie de base du cadavre | ${p.get('param1')}–${p.get('param2')} % | ${b.get('param1')}–${b.get('param2')} % |
| Répartition élémentaire | ${p.get('param5')} % feu | ${b.get('param5')} % feu |
| Dégâts plats physiques L1 | ${formatRange(damageAtLevel(p, 1, false))} | ${formatRange(damageAtLevel(b, 1, false))} |
| Dégâts plats physiques L20 | ${formatRange(damageAtLevel(p, 20, false))} | ${formatRange(damageAtLevel(b, 20, false))} |
| Dégâts plats physiques L40 | ${formatRange(damageAtLevel(p, 40, false))} | ${formatRange(damageAtLevel(b, 40, false))} |
| Dégâts plats feu L1 | ${formatRange(damageAtLevel(p, 1, true))} | ${formatRange(damageAtLevel(b, 1, true))} |
| Dégâts plats feu L20 | ${formatRange(damageAtLevel(p, 20, true))} | ${formatRange(damageAtLevel(b, 20, true))} |
| Dégâts plats feu L40 | ${formatRange(damageAtLevel(p, 40, true))} | ${formatRange(damageAtLevel(b, 40, true))} |
| Rayon brut L1 | ${pRadius1.ok ? `${pRadius1.value} grid sub-tiles` : markdownValue(p.get('aurarangecalc'))} | ${bRadius1.ok ? `${bRadius1.value} grid sub-tiles` : markdownValue(b.get('aurarangecalc'))} |
| Rayon brut L20 | ${pRadius20.ok ? `${pRadius20.value} grid sub-tiles` : markdownValue(p.get('aurarangecalc'))} | ${bRadius20.ok ? `${bRadius20.value} grid sub-tiles` : markdownValue(b.get('aurarangecalc'))} |
| Mana L1 | ${formatNumber(manaCostAtLevel(p, 1))} | ${formatNumber(manaCostAtLevel(b, 1))} |
| Mana L20 | ${formatNumber(manaCostAtLevel(p, 20))} | ${formatNumber(manaCostAtLevel(b, 20))} |
| Synergie élémentaire | ${markdownValue(p.get('edmgsympercalc'))} | ${markdownValue(b.get('edmgsympercalc'))} |

Le rayon PD2 vaut bien **15** au niveau 1 dans le scénario \`stat(...) = 0\`, car la formule brute emploie \`Param3 + lvl × Param4\`. Une valeur de rayon brute n'est pas convertie génériquement en « cases » : sa consommation exacte dépend de la fonction native.

PD2 remplace donc l'essentiel du scaling par vie du cadavre par des dégâts plats qui montent avec le niveau et les hard points de synergie. BKVince conserve un scaling de 70–120 % de la vie de base, sans composante plate. Copier uniquement les pourcentages PD2 détruirait Corpse Explosion; copier uniquement les dégâts plats les superposerait au scaling BKVince et la suralimenterait. La présence des champs plats PD2 est \`EXACT_TABLE\`; leur consommation par \`srvdofunc 55\` dans D2R 3.2 reste \`NATIVE_UNPROVEN\`.
`;
}

export function buildSkillReport(sources) {
  const mapping = buildSkillMapping(sources.pd2['skills.txt'], sources.bkvince['skills.txt']);
  const references = {
    pd2: buildReferenceIndex(sources.pd2),
    bkvince: buildReferenceIndex(sources.bkvince),
  };
  const pd2Skills = sources.pd2['skills.txt'];
  const bkvSkills = sources.bkvince['skills.txt'];
  const pd2Metadata = sources.metadata.pd2;
  const sections = [
    '# Audit exhaustif de `Skills.txt` — Project Diablo 2 / Single Player+ versus BKVince',
    '',
    'Statut : rapport généré depuis les sources gouvernées; les adaptations gameplay BKVince sont consignées dans la mission.',
    'Date de l’audit : 8 août 2026.',
    'Audience : Vincent, ChatGPT 5.6 Pro et futurs agents du Workspace RuffnecKk.',
    '',
    '## Sources et reproductibilité',
    '',
    '- PD2/SP+ : [snapshot GitHub `3debc6781f33c3c1474a995b80369a4e618cd386`](https://github.com/Lukaszpg/PD2-Single-Player-Plus-mod/commit/3debc6781f33c3c1474a995b80369a4e618cd386); le SHA-256 local de `Skills.txt` correspond byte pour byte au raw de ce commit.',
    pd2Metadata
      ? `- Métadonnée embarquée \`${pd2Metadata.path}\` : version \`${pd2Metadata.value.version}\`, date \`${pd2Metadata.value.date}\`, SHA-256 \`${pd2Metadata.sha256}\`. Le libellé du commit GitHub indique « Version 13.0.2 » : le hash de table, et non le numéro marketing incohérent, fixe la baseline technique.`
      : '- Métadonnée de release PD2/SP+ : absente; la baseline est fixée uniquement par le commit et les hashes.',
    `- Wiki conceptuel : [Skill Changes, révision 23785 du 17 juin 2026](https://wiki.projectdiablo2.com/w/index.php?title=Skill_Changes&oldid=23785); cette page se déclare elle-même potentiellement obsolète pour son aperçu S9–S11.`,
    `- \`${pd2Skills.relativePath}\` : ${pd2Skills.table.rows.length} lignes × ${pd2Skills.table.headers.length} colonnes, ${pd2Skills.table.eol === '\n' ? 'LF' : 'CRLF'}, SHA-256 \`${pd2Skills.sha256}\`.`,
    `- \`${bkvSkills.relativePath}\` : ${bkvSkills.table.rows.length} lignes × ${bkvSkills.table.headers.length} colonnes, ${bkvSkills.table.eol === '\r\n' ? 'CRLF' : 'LF'}, SHA-256 \`${bkvSkills.sha256}\`.`,
    '- Identité runtime : ordinal de ligne en base zéro. Le `Id` PD2 est cohérent avec cet ordinal; le `*Id` BKVince est uniquement documentaire et comporte des doublons/anomalies inventoriés ci-dessous.',
    '- Les formules linéaires utilisent `a + (niveau - 1) × b`; les dégâts utilisent les paliers 2–8, 9–16, 17–22, 23–28 et 29+ documentés par d2rdoc.',
    `- Codes BBE D2R : \`${sources.reference['skillcalc.txt'].relativePath}\`, SHA-256 \`${sources.reference['skillcalc.txt'].sha256}\`; il gouverne notamment \`edmn/edmx/edln\`, \`enma/exma/enms/exms/edma\`, \`pa10…pa20\` et \`clc0…clc9\`.`,
    '- Scénario calculé standard : niveau effectif `L ∈ {1,5,10,20,30,40}`, hard points `B=min(L,maxlvl)`, autres skills/stats/masteries à zéro. Les dépendances restantes restent symboliques.',
    '- Périmètre exhaustif : chaque ligne et chaque cellule non documentaire de `Skills.txt` des deux sources. Les tables liées affichées sont les dépendances directes sélectionnées; elles ne constituent pas un graphe transitif complet du runtime.',
    '- Étiquettes de preuve : `EXACT_TABLE` pour les cellules brutes, `EXACT_FORMULA` pour une BBE résolue dans le scénario déclaré, `EXACT_DERIVED` pour les conversions documentées, `MALFORMED_SOURCE` ou `NATIVE_UNPROVEN` lorsqu’une conclusion numérique en jeu serait injustifiée.',
    '',
    renderSourceManifest(sources).trim(),
    '',
    '## Couverture',
    '',
    renderCoverageTable(mapping),
    '',
    renderGlobalAudit(sources, mapping).trim(),
    '',
    renderGlobalMechanicDecisions(sources).trim(),
    '',
    renderSemanticRelations(mapping).trim(),
    '',
    renderFormulaIntegrity(sources).trim(),
    '',
    renderRuntimeIdentityAudit(mapping).trim(),
    '',
    renderExecutiveConclusions().trim(),
    '',
    renderWorkedCorpseExplosion(mapping).trim(),
    '',
    '## Légende du mapping',
    '',
    '- **Même nom, même ordinal runtime** : comparaison sémantique directe possible, sans présumer la portabilité des fonctions.',
    '- **Même nom, ordinal déplacé** : comportement comparable, dépendances et sauvegardes à remapper.',
    '- **Même ordinal, skill différent** : collision sémantique; les lignes ne doivent jamais être fusionnées.',
    '- **PD2 uniquement** : nouveau skill ou ligne absente de BKVince.',
    '- **BKVince uniquement** : création BKVince, Warlock, commande ou skill de monstre sans équivalent PD2.',
    '',
    '> Les sections de classe sont des vues de portée. Une collision reliant deux classes différentes, ou une classe à une ligne classless, est répétée dans chaque vue concernée afin que les 30 skills Warlock et les deux côtés de chaque collision restent visibles. Le graphe primaire de couverture, lui, consomme chaque ligne une seule fois.',
    '',
  ];

  for (const classCode of CLASS_ORDER) {
    const classEntries = mapping.entries.filter((entry) => playerClassesForEntry(entry).includes(classCode));
    if (!classEntries.length) continue;
    sections.push(`## ${CLASS_NAMES[classCode]}`, '');
    sections.push('| Skill PD2 | Ord. PD2 | Skill BKVince | Ord. BKV | Mapping | Cellules différentes |');
    sections.push('|---|---:|---|---:|---|---:|');
    for (const entry of classEntries) {
      sections.push(`| ${entry.pd2?.get('skill') || '—'} | ${entry.pd2 ? runtimeSkillOrdinal(entry.pd2) : '—'} | ${entry.bkvince?.get('skill') || '—'} | ${entry.bkvince ? runtimeSkillOrdinal(entry.bkvince) : '—'} | ${statusLabel(entry.status)} | ${diffRecords(entry.pd2, entry.bkvince).length} |`);
    }
    sections.push('');
    for (const entry of classEntries) sections.push(renderSkillEntry(entry, sources, references, 3), '');
  }

  const classless = mapping.entries.filter((entry) => hasClasslessSide(entry));
  sections.push('## Appendice A — Skills généraux, item-only, monstres, commandes et lignes techniques', '');
  sections.push('Cet appendice montre toute entrée dont au moins un côté est classless, général, item-triggered, monstre, commande ou technique. Les collisions traversant une frontière de portée sont volontairement répétées ici et dans la classe concernée; le tableau de couverture primaire reste l’assertion d’unicité.', '');
  sections.push('| Skill PD2 | Ord. PD2 | Skill BKVince | Ord. BKV | Mapping | Cellules différentes |');
  sections.push('|---|---:|---|---:|---|---:|');
  for (const entry of classless) {
    sections.push(`| ${entry.pd2?.get('skill') || '—'} | ${entry.pd2 ? runtimeSkillOrdinal(entry.pd2) : '—'} | ${entry.bkvince?.get('skill') || '—'} | ${entry.bkvince ? runtimeSkillOrdinal(entry.bkvince) : '—'} | ${statusLabel(entry.status)} | ${diffRecords(entry.pd2, entry.bkvince).length} |`);
  }
  sections.push('');
  for (const entry of classless) sections.push(renderSkillEntry(entry, sources, references, 3), '');

  sections.push(
    '## Appendice B — Limites d’interprétation',
    '',
    '- Une différence de numéro `srvdofunc`, `cltdofunc` ou de fonction missile n’est jamais déclarée portable entre PD2 et D2R 3.2.',
    '- Une formule affichée dans une table ne prouve pas à elle seule que le callback PD2 et le callback D2R consomment les paramètres de la même manière.',
    '- Les valeurs calculées sont exactes pour les primitives documentées prises isolément. Elles excluent équipement, difficulté, résistance, PvP, bonus globaux et synergies non explicitement résolues.',
    '- Les références d’items listées par nom sont exhaustives pour les cellules textuelles exactes, mais pas pour les consommateurs encodés seulement par ID ou code natif.',
    '- Les missiles sont détaillés lorsqu’un champ de `Skills.txt` les nomme directement. Le rapport ne déroule pas récursivement leurs sous-missiles; il ne prétend donc pas couvrir transitivement chaque hitbox, vélocité, `NextHit`, overlay ou son déclenché en aval.',
    '- Les dépendances directes sélectionnées couvrent `missiles`, `states`, `skilldesc`, `itemstatcost`, `properties`, `pettype` et `monstats`. `itemtypes`, `monstats2`, overlays, sons, affixes, recettes cube et consommateurs natifs restent hors de cette expansion liée.',
    '- Aucune disposition gameplay n’est une priorité démontrée sans benchmark BKVince.',
    '',
    '## Validation attendue',
    '',
    '- le générateur doit reproduire ce fichier byte-identiquement;',
    '- toutes les lignes PD2 et BKVince doivent être consommées exactement une fois;',
    '- les hashes sources doivent rester inchangés;',
    '- le test Corpse Explosion doit fermer pourcentages, dégâts plats, rayon et mana;',
    '- aucune table sous `data-BKVince` ni dans la source PD2 read-only ne doit être écrite.',
    '',
  );

  return {
    report: `${sections.join('\n').replace(/\n{3,}/g, '\n\n').trim()}\n`,
    mapping,
    references,
  };
}

export function generateSkillReport(roots = defaultSkillReportRoots) {
  const sources = loadSkillReportSources(roots);
  const built = buildSkillReport(sources);
  return { ...built, sources };
}

function main() {
  const args = new Set(process.argv.slice(2));
  const generated = generateSkillReport();
  if (args.has('--stdout')) {
    process.stdout.write(generated.report);
    return;
  }
  if (args.has('--check')) {
    if (!fs.existsSync(defaultSkillReportRoots.output)) {
      throw new Error(`Missing generated report: ${defaultSkillReportRoots.output}`);
    }
    const existing = fs.readFileSync(defaultSkillReportRoots.output, 'utf8');
    assert.equal(existing, generated.report, 'Generated skill report is stale');
    console.log(`VALID: exhaustive PD2/BKVince skill report is current (${generated.mapping.pd2Rows} PD2 rows, ${generated.mapping.bkvRows} BKVince rows)`);
    return;
  }
  fs.writeFileSync(defaultSkillReportRoots.output, generated.report, 'utf8');
  console.log(`Generated ${path.relative(repoRoot, defaultSkillReportRoots.output)} (${Buffer.byteLength(generated.report, 'utf8')} bytes)`);
  console.log(`Coverage: PD2=${generated.mapping.pd2Rows} BKVince=${generated.mapping.bkvRows} ambiguities=${generated.mapping.ambiguous.length}`);
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try {
    main();
  } catch (error) {
    console.error(`PD2/BKVince skill report failed: ${error.stack || error.message}`);
    process.exitCode = 1;
  }
}
