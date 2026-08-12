import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

import {
  BEHAVIOR_GROUPS,
  CLASS_NAMES,
  CLASS_ORDER,
  COMPONENT_DECISIONS,
  FROZEN_CONTRACT_HASH,
  GLOBAL_DECISIONS,
  IMPLEMENTATION_STATUSES,
  MAPPING_TYPES,
  ORACLE_SCHEMA_VERSION,
  PLAYER_CLASS_CODES,
  PORTABILITY_CATEGORIES,
  PROOF_STATUSES,
  PROTECTED_FIELD_RULES,
  REVIEW_ID,
  NEW_SKILL_LINE_DECISIONS,
  SOURCE_ORDER,
  normalizeSkillName,
  physicalNodeId,
  sha256Canonical,
  stableSkillId,
} from './pd2-skills-review-contracts.mjs';
import {
  damageAtLevel,
  elementalLengthAtLevel,
  evaluateSimpleFormula,
  manaCostAtLevel,
} from '../audit-pd2-bkvince/skill-report.mjs';

const require = createRequire(import.meta.url);
const { ENCODING, parseTable, serializeTable } = require('../build-data/tsv.js');

export const repoRoot = fileURLToPath(new URL('../../', import.meta.url));
export const LEVELS = Object.freeze([1, 5, 10, 20, 30, 40]);

export const DEFAULT_SOURCE_ROOTS = Object.freeze({
  vanilla32: path.join(repoRoot, 'data-vanilla3.2', 'data', 'data', 'global', 'excel'),
  bkvince: path.join(repoRoot, 'data-BKVince', 'BKVince.mpq', 'data', 'global', 'excel'),
  pd2: process.env.PD2_SP_ROOT || path.resolve(
    repoRoot,
    '..',
    'PD2 Single PLayer',
    'PD2-Single-Player-Plus-mod-main',
    'data',
    'global',
    'excel',
  ),
  analyticalAudit: path.join(repoRoot, 'Mission', 'pd2-skills-vs-bkvince-full-audit.md'),
  vanillaSkillcalc: path.join(repoRoot, 'data-vanilla3.2', 'data', 'data', 'global', 'excel', 'skillcalc.txt'),
  bkvinceSkillsLocalization: path.join(repoRoot, 'data-BKVince', 'BKVince.mpq', 'data', 'local', 'lng', 'strings', 'skills.json'),
  pd2Patchstring: path.resolve(
    repoRoot, '..', 'PD2 Single PLayer', 'PD2-Single-Player-Plus-mod-main',
    'data', 'local', 'LNG', 'ENG', 'patchstring.tbl',
  ),
});

export const SOURCE_TABLE_NAMES = Object.freeze([
  'skills.txt',
  'skilldesc.txt',
  'missiles.txt',
  'states.txt',
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

const TABLE_KEYS = Object.freeze({
  'skills.txt': ['skill'],
  'skilldesc.txt': ['skilldesc'],
  'missiles.txt': ['missile'],
  'states.txt': ['state'],
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

const SOURCE_LABELS = Object.freeze({
  vanilla32: 'Vanilla D2R 3.2',
  bkvince: 'BKVince HEAD',
  pd2: 'Project Diablo 2 / Single Player+',
});

const SOURCE_PROVENANCE = Object.freeze({
  vanilla32: Object.freeze({ kind: 'governed-extraction', version: 'D2R 3.2' }),
  bkvince: Object.freeze({ kind: 'repository-head', authority: 'current working files' }),
  pd2: Object.freeze({
    kind: 'pinned-local-snapshot',
    commit: '3debc6781f33c3c1474a995b80369a4e618cd386',
    tree: '6f51e17e5f65abdd50b2fd33190c571fef296ccf',
  }),
});

const CLASS_TREE_LABELS = Object.freeze({
  ama: Object.freeze({ 1: 'Amazon — Bow and Crossbow', 2: 'Amazon — Passive and Magic', 3: 'Amazon — Javelin and Spear' }),
  sor: Object.freeze({ 1: 'Sorceress — Fire', 2: 'Sorceress — Lightning', 3: 'Sorceress — Cold' }),
  nec: Object.freeze({ 1: 'Necromancer — Curses', 2: 'Necromancer — Poison and Bone', 3: 'Necromancer — Summoning' }),
  pal: Object.freeze({ 1: 'Paladin — Combat', 2: 'Paladin — Offensive Auras', 3: 'Paladin — Defensive Auras' }),
  bar: Object.freeze({ 1: 'Barbarian — Combat', 2: 'Barbarian — Masteries', 3: 'Barbarian — Warcries' }),
  dru: Object.freeze({ 1: 'Druid — Summoning', 2: 'Druid — Shape Shifting', 3: 'Druid — Elemental' }),
  ass: Object.freeze({ 1: 'Assassin — Traps', 2: 'Assassin — Shadow Disciplines', 3: 'Assassin — Martial Arts' }),
  war: Object.freeze({ 1: 'Warlock — Demon', 2: 'Warlock — Eldritch', 3: 'Warlock — Chaos' }),
});

const GOVERNED_RELATIONS = Object.freeze([
  Object.freeze({ kind: 'RENAMED_ALIAS', pd2: 'AmpDmg', bkvince: 'Amplify Damage', semantic: true }),
  Object.freeze({ kind: 'RENAMED_ALIAS', pd2: 'LowRes', bkvince: 'Lower Resist', semantic: true }),
  Object.freeze({ kind: 'SAME_SKILL_MOVED_ORDINAL', pd2: 'Cold Enchant', bkvince: 'Cold Enchant', semantic: true }),
  Object.freeze({ kind: 'SAME_SKILL_MOVED_ORDINAL', pd2: 'Lightning Sentry', bkvince: 'Lightning Sentry', semantic: true }),
  Object.freeze({ kind: 'SLOT_REPLACEMENT', pd2: 'Slow Movement', bkvince: 'Slow Missiles', semantic: false }),
  Object.freeze({ kind: 'SLOT_REPLACEMENT', pd2: 'Javelin and Spear Mastery', bkvince: 'Impale', semantic: false }),
  Object.freeze({ kind: 'SLOT_REPLACEMENT', pd2: 'Desecrate', bkvince: 'Poison Explosion', semantic: false }),
  Object.freeze({ kind: 'SLOT_REPLACEMENT', pd2: 'Raise Skeleton Archer', bkvince: 'Summon Resist', semantic: false }),
  Object.freeze({ kind: 'SLOT_REPLACEMENT', pd2: 'Holy Sword', bkvince: 'Conversion', semantic: false }),
  Object.freeze({ kind: 'SLOT_REPLACEMENT', pd2: 'Sword Mastery', bkvince: 'Blade Mastery', semantic: false }),
  Object.freeze({ kind: 'SLOT_REPLACEMENT', pd2: 'One Hand Mastery', bkvince: 'Axe Mastery', semantic: false }),
  Object.freeze({ kind: 'SLOT_REPLACEMENT', pd2: 'Two Hand Mastery', bkvince: 'Pole Arm Mastery', semantic: false }),
  Object.freeze({ kind: 'SLOT_REPLACEMENT', pd2: 'Combat Reflexes', bkvince: 'Increased Endurance', semantic: false }),
]);

const CANONICAL_DISPLAY_ALIASES = Object.freeze({ CurMas: 'Curse Mastery' });

const AUDIT_PLAYER_OVERRIDES = Object.freeze(new Map([
  ['Shattering Arrow', 'The pinned exhaustive audit identifies this bow-tree row as a real player skill although its skilldesc coordinates are 0/0/0.'],
  ['Blade Dance', 'The pinned exhaustive audit identifies this Assassin row as a real player skill although its skilldesc placement is incomplete.'],
]));

const TECHNICAL_NAME_PATTERN = /(?:\bproc\b|temp\d*|map|merc|boss|monster|^mon|helper|unused|selfaura|ashens|ureh|cowboss|iceboss)/i;

const FORMULA_FIELD_PATTERN = /calc|delay$/i;
const SKILL_LINK_FIELDS = /^(?:reqskill[1-3]|sumskill[1-5])$/i;
const MISSILE_LINK_FIELDS = /missile/i;
const STATE_LINK_FIELDS = /(?:^state[1-3]$|state$)/i;
const ITEMSTAT_LINK_FIELDS = /stat(?:[1-9]|1[0-4])$/i;
const NATIVE_FUNCTION_PATTERN = /^(?:srv|clt).*(?:func|function)\d*$|hitfunc\d*$/i;
const CLIENT_SERVER_MISSILE_PATTERN = /^(?:srv|clt)missile[a-d]?$/i;
const GOVERNED_CALC1_COUNT_DESCRIPTION = /(?:^|\b)(?:#|number|num|max)\s*(?:of\s*)?(?:missiles?|bolts?|arrows?|targets?|hits?|chains?|summons?|units?)\b/i;
const REQUIRED_PRESENTATION_HEADER = /^(?:skill|charclass|skilldesc|reqlevel|reqskill[1-3]|maxlvl|mana|lvlmana|minmana|manashift|startmana|delay|localdelay|globaldelay|perdelay|interrupt|repeat|srcdam|hitshift|mindam|maxdam|minlevdam[1-5]|maxlevdam[1-5]|emin|emax|eminlev[1-5]|emaxlev[1-5]|etype|elen|elevlen[1-3]|tohit|levtohit|tohitcalc|dmgsympercalc|edmgsympercalc|elensympercalc|auralencalc|aurarangecalc|aurastate|auratargetstate|aurastat[1-6]|aurastatcalc[1-6]|srvmissile[a-c]?|cltmissile[a-d]?|calc1|petmax|summon|pettype|requirespettype|sumskill[1-5]|sumsk[1-5]calc|passivestat(?:[1-9]|1[0-4])|passivecalc(?:[1-9]|1[0-4])|srv.*func|clt.*func)$/i;

const LINKED_FACT_PATTERNS = Object.freeze({
  'missiles.txt': /^(?:vel|maxvel|range|size|collidekill|pierce|nexthit|submissile|hitfunc|explosion)/i,
  'skilldesc.txt': /^(?:skillpage|skillrow|skillcolumn|listrow|iconcel|str name|str short|str long|str alt)$/i,
  'states.txt': /^(?:id|state|group|remstat|nosend|transform|hide)/i,
  'itemstatcost.txt': /^(?:id|stat|save bits|savebits|save add|saveadd|send bits|sendbits|signed|csvsigned)/i,
  'properties.txt': /^(?:code|func|stat|set)/i,
  'pettype.txt': /^(?:pet type|pettype|group|basemax|name|icon)/i,
  'monstats.txt': /^(?:id|hcidx|baseid|monstatsex|ai|skill|sk|pet|velocity|aip)/i,
  'skills.txt': /^(?:skill|charclass|skilldesc|maxlvl|reqskill|srv.*func|clt.*func)/i,
});

const COMPONENT_HEADER_PATTERNS = Object.freeze({
  identity_availability: /^(?:id|skill|charclass|skilldesc|reqlevel|reqskill[1-3]|maxlvl|leftskill|rightskill|intown|ingame|restrict|itype|etype|requires)/i,
  cost_timing: /^(?:mana|lvlmana|minmana|manashift|startmana|delay|localdelay|globaldelay|perdelay|interrupt|repeat|usemana)/i,
  damage_model: /^(?:srcdam|hitshift|mindam|maxdam|minlevdam|maxlevdam|emin|emax|eminlev|emaxlev|etype$|elen|elevlen|damagerate|tohit|levtohit|tohitcalc|dmgsym|edmgsym|elensym)/i,
  area_targeting: /(?:range|radius|target|search|lineofsight|aura.*filter|select|warp)/i,
  projectiles_collisions: /(?:missile|collide|pierce|nexthit|hitfunc|vel|range|size|submissile|explode)/i,
  animation_sequence: /^(?:anim|seq|useattackrate|attackrank)/i,
  buffs_debuffs_auras_passives: /^(?:aura|passive|state|periodic|event)/i,
  synergies: /(?:sympercalc|reqskill)/i,
  summons: /^(?:summon|pet|sumskill|sumsk|requirespet)/i,
  engine_functions: /^(?:srv|clt).*(?:func|function)\d*$|hitfunc\d*$/i,
  interface_localization: /^(?:skilldesc|scroll|icon|str )/i,
});

const FIELD_LABELS = Object.freeze({
  mana: 'Mana', lvlmana: 'Mana per level', startmana: 'Start mana',
  localdelay: 'Local delay', globaldelay: 'Global delay', delay: 'PD2 delay',
  auralencalc: 'Duration', aurarangecalc: 'Radius', srcdam: 'Weapon damage',
  srvmissile: 'Server missile', cltmissile: 'Client missile', maxlvl: 'Maximum level',
  reqlevel: 'Required level', charclass: 'Class', skilldesc: 'Skill description',
  summon: 'Summoned monster', pettype: 'Pet type',
});

const LINKED_FACT_LABELS = Object.freeze({
  vel: 'Missile speed',
  maxvel: 'Maximum missile speed',
  range: 'Missile range',
  size: 'Missile hitbox size',
  collidekill: 'Destroyed on collision',
  pierce: 'Missile pierce',
  nexthit: 'Next Hit Delay',
  explosionmissile: 'Final explosion missile',
  hitfunc: 'Missile hit function',
});

export function sha256Buffer(value) {
  return crypto.createHash('sha256').update(value).digest('hex').toUpperCase();
}

export function stableHash(value) {
  return sha256Canonical(value);
}

function governedRelative(filePath) {
  const relative = path.relative(repoRoot, filePath).replaceAll('\\', '/');
  return relative.startsWith('../PD2 Single PLayer/')
    ? relative
    : relative.replace(/^\.\//, '');
}

function canonicalHeader(header) {
  const result = String(header ?? '').trim().toLowerCase();
  if (result === 'id' || result === '*id') return 'id';
  return result;
}

function normalizedLookup(value) {
  return String(value ?? '').trim().toLowerCase();
}

function resolveTablePath(root, requestedName) {
  const actual = fs.readdirSync(root).filter((entry) => entry.toLowerCase() === requestedName.toLowerCase());
  assert.equal(actual.length, 1, `${root}: expected exactly one ${requestedName}`);
  return path.join(root, actual[0]);
}

function loadTable(root, requestedName, source, inheritance = null) {
  const filePath = resolveTablePath(root, requestedName);
  const raw = fs.readFileSync(filePath);
  const transport = fs.readFileSync(filePath, ENCODING);
  const table = parseTable(filePath);
  assert.equal(serializeTable(table), transport, `${filePath}: non byte-exact TSV round-trip`);

  const indexes = new Map();
  table.headers.forEach((header, index) => {
    const key = canonicalHeader(header);
    if (indexes.has(key)) throw new Error(`${filePath}: duplicate canonical header ${key}`);
    indexes.set(key, index);
  });
  const keyHeader = (TABLE_KEYS[requestedName.toLowerCase()] ?? [])
    .find((candidate) => indexes.has(candidate)) ?? canonicalHeader(table.headers[0]);
  const records = table.rows.map((row, ordinal) => ({
    source,
    table: requestedName.toLowerCase(),
    ordinal,
    line: ordinal + 2,
    row,
    key: row[indexes.get(keyHeader)] ?? '',
    get(header) {
      const index = indexes.get(canonicalHeader(header));
      return index === undefined ? undefined : row[index];
    },
    has(header) {
      return indexes.has(canonicalHeader(header));
    },
  }));
  const byKey = new Map();
  for (const record of records) {
    const key = normalizedLookup(record.key);
    if (!byKey.has(key)) byKey.set(key, []);
    byKey.get(key).push(record);
  }
  const document = {
    source,
    requestedName: requestedName.toLowerCase(),
    actualName: path.basename(filePath),
    filePath,
    relativePath: governedRelative(filePath),
    sha256: sha256Buffer(raw),
    table,
    indexes,
    keyHeader,
    records,
    byKey,
    inheritance,
  };
  for (const record of records) record.document = document;
  return document;
}

function exactRecord(document, key) {
  const matches = document?.byKey.get(normalizedLookup(key)) ?? [];
  return matches.length === 1 ? matches[0] : null;
}

function tableManifest(document, extra = {}) {
  return {
    path: document.relativePath,
    sha256: document.sha256,
    rows: document.table.rows.length,
    columns: document.table.headers.length,
    eol: document.table.eol === '\r\n' ? 'CRLF' : 'LF',
    hasFinalEol: document.table.hasFinalEol,
    roundTripByteExact: true,
    ...extra,
  };
}

export function loadWorkbenchSources(roots = DEFAULT_SOURCE_ROOTS) {
  const sources = { roots, documents: {}, sourceManifest: {}, sourceHashes: {} };
  for (const source of SOURCE_ORDER) {
    sources.documents[source] = {};
    sources.sourceManifest[source] = {
      label: SOURCE_LABELS[source],
      provenance: SOURCE_PROVENANCE[source],
      tables: {},
    };
    sources.sourceHashes[source] = {};
    for (const tableName of SOURCE_TABLE_NAMES) {
      let root = roots[source];
      let inheritance = null;
      if (source === 'bkvince' && tableName === 'pettype.txt') {
        const direct = fs.existsSync(root)
          ? fs.readdirSync(root).some((entry) => entry.toLowerCase() === tableName)
          : false;
        if (!direct) {
          root = roots.vanilla32;
          inheritance = {
            mode: 'INHERITED_VANILLA32',
            reason: 'BKVince intentionally omits pettype.txt and inherits the governed Vanilla D2R 3.2 table.',
          };
        }
      }
      const document = loadTable(root, tableName, source, inheritance);
      sources.documents[source][tableName] = document;
      sources.sourceManifest[source].tables[tableName] = tableManifest(document, inheritance ? {
        logicalSource: 'bkvince',
        physicalSource: 'vanilla32',
        inheritance,
      } : {});
      sources.sourceHashes[source][tableName] = document.sha256;
    }
  }

  const auditRaw = fs.readFileSync(roots.analyticalAudit);
  sources.sourceManifest.analyticalAudit = {
    path: governedRelative(roots.analyticalAudit),
    sha256: sha256Buffer(auditRaw),
    purpose: 'Historical analytical baseline (449 BKVince rows / 108 collisions); never parsed as the row or value database.',
    historicalBaseline: { bkvinceRows: 449, collisions: 108 },
  };
  sources.sourceHashes.analyticalAudit = sources.sourceManifest.analyticalAudit.sha256;
  const skillcalc = loadTable(path.dirname(roots.vanillaSkillcalc), path.basename(roots.vanillaSkillcalc), 'vanilla32');
  sources.documents.vanilla32['skillcalc.txt'] = skillcalc;
  sources.sourceManifest.formulaReference = {
    path: skillcalc.relativePath,
    sha256: skillcalc.sha256,
    rows: skillcalc.table.rows.length,
    columns: skillcalc.table.headers.length,
    eol: skillcalc.table.eol === '\r\n' ? 'CRLF' : 'LF',
    purpose: 'Vanilla D2R 3.2 BBE skill formula reference.',
  };
  sources.sourceHashes.formulaReference = skillcalc.sha256;

  const bkvLocalizationRaw = fs.readFileSync(roots.bkvinceSkillsLocalization);
  const bkvLocalizationValue = JSON.parse(bkvLocalizationRaw.toString('utf8').replace(/^\uFEFF/, ''));
  const localizationByKey = new Map(bkvLocalizationValue
    .filter((entry) => entry && typeof entry.Key === 'string')
    .map((entry) => [entry.Key, entry]));
  const pd2PatchRaw = fs.readFileSync(roots.pd2Patchstring);
  sources.localization = { bkvinceByKey: localizationByKey };
  sources.sourceManifest.localization = {
    vanilla32: {
      expectedMissing: true,
      unavailable: true,
      provenance: 'The governed Vanilla 3.2 extraction does not version localization assets.',
    },
    bkvince: {
      path: governedRelative(roots.bkvinceSkillsLocalization),
      sha256: sha256Buffer(bkvLocalizationRaw),
      format: 'D2R JSON strings',
      entries: bkvLocalizationValue.length,
    },
    pd2: {
      path: governedRelative(roots.pd2Patchstring),
      sha256: sha256Buffer(pd2PatchRaw),
      format: 'binary TBL',
      parseStatus: 'UNSUPPORTED_BINARY_FORMAT_HASHED_ONLY',
      bytes: pd2PatchRaw.length,
    },
  };
  sources.sourceHashes.localization = {
    bkvince: sources.sourceManifest.localization.bkvince.sha256,
    pd2: sources.sourceManifest.localization.pd2.sha256,
  };
  return sources;
}

function rawObject(document, record) {
  if (!record) return null;
  return Object.fromEntries(document.table.headers.map((header, index) => [header, record.row[index] ?? '']));
}

function coordinates(document, skilldescKey) {
  const record = exactRecord(document, skilldescKey);
  if (!record) return null;
  const value = (name) => {
    const raw = record.get(name);
    if (raw === undefined || raw === '') return null;
    const parsed = Number(raw);
    return Number.isFinite(parsed) ? parsed : null;
  };
  return {
    source: document.source,
    rowOrdinal: record.ordinal,
    skilldescKey: record.key,
    page: value('skillpage'),
    row: value('skillrow'),
    column: value('skillcolumn'),
    listRow: value('listrow'),
    iconCel: value('iconcel'),
  };
}

function linkedRowsFor(record, documents) {
  const links = [];
  if (!record) return links;
  const add = (table, field, key) => {
    if (!key) return;
    const target = exactRecord(documents[table], key);
    links.push({
      source: record.source,
      table,
      field,
      key,
      found: Boolean(target),
      ordinal: target?.ordinal ?? null,
      fingerprint: target ? stableHash(target.row) : null,
    });
  };
  add('skilldesc.txt', 'skilldesc', record.get('skilldesc'));
  for (const header of record.document?.table?.headers ?? []) void header;
  for (const field of recordSourceHeaders(record)) {
    const value = String(record.get(field) ?? '').trim();
    if (!value) continue;
    if (MISSILE_LINK_FIELDS.test(field)) add('missiles.txt', field, value);
    else if (STATE_LINK_FIELDS.test(field)) add('states.txt', field, value);
    else if (SKILL_LINK_FIELDS.test(field)) add('skills.txt', field, value);
    else if (/^(?:summon)$/i.test(field)) add('monstats.txt', field, value);
    else if (/^(?:pettype|requirespettype)$/i.test(field)) add('pettype.txt', field, value);
    else if (ITEMSTAT_LINK_FIELDS.test(field)) add('itemstatcost.txt', field, value);
  }
  return links;
}

function recordSourceHeaders(record) {
  return record?.owner?.table.headers ?? [];
}

function formulaFinding(source, table, record, header, raw, evaluation) {
  return {
    source,
    table,
    row: record.ordinal,
    key: record.key,
    header,
    raw,
    status: evaluation.status,
    proofStatus: evaluation.status,
    value: evaluation.ok ? evaluation.value : null,
    reason: evaluation.ok ? null : evaluation.reason,
  };
}

function scanFormulaRows(source, skillsDocument, skilldescDocument, skillRecord) {
  const findings = [];
  const scan = (document, record) => {
    if (!record) return;
    for (const header of document.table.headers) {
      const canonical = canonicalHeader(header);
      if (canonical.startsWith('*') || !FORMULA_FIELD_PATTERN.test(canonical)) continue;
      const raw = String(record.get(canonical) ?? '').trim();
      if (!raw || /^-?\d+(?:\.\d+)?$/.test(raw)) continue;
      const evaluation = evaluateSimpleFormula(raw, skillRecord, 20, {
        statValue: 0,
        referencedSkillLevel: 0,
        applyDerivedSynergy: false,
      });
      findings.push(formulaFinding(source, document.requestedName, record, header, raw, evaluation));
    }
  };
  scan(skillsDocument, skillRecord);
  scan(skilldescDocument, exactRecord(skilldescDocument, skillRecord.get('skilldesc')));
  return findings;
}

function buildNodes(sources) {
  const nodes = [];
  const bySourceOrdinal = new Map();
  for (const source of SOURCE_ORDER) {
    const documents = sources.documents[source];
    const skillDocument = documents['skills.txt'];
    for (const record of skillDocument.records) {
      record.owner = skillDocument;
      const name = String(record.get('skill') ?? '').trim();
      const node = {
        id: physicalNodeId(source, record.ordinal),
        source,
        ordinal: record.ordinal,
        line: record.line,
        declaredId: String(record.get('id') ?? ''),
        name,
        normalizedName: normalizeSkillName(name),
        classCode: String(record.get('charclass') ?? '').trim().toLowerCase() || null,
        skilldescKey: String(record.get('skilldesc') ?? '').trim() || null,
        rowFingerprint: stableHash({ source, ordinal: record.ordinal, row: record.row }),
        raw: rawObject(skillDocument, record),
        tree: coordinates(documents['skilldesc.txt'], record.get('skilldesc')),
        linkedRows: linkedRowsFor(record, documents),
        formulaFindings: scanFormulaRows(source, skillDocument, documents['skilldesc.txt'], record),
      };
      nodes.push(node);
      bySourceOrdinal.set(`${source}:${record.ordinal}`, node);
    }
  }
  return { nodes, bySourceOrdinal };
}

function uniqueNamedNodes(nodes, source) {
  return nodes.filter((node) => node.source === source && node.name);
}

function relationMaps(nodes) {
  const bySourceName = {};
  for (const source of SOURCE_ORDER) {
    bySourceName[source] = new Map();
    for (const node of uniqueNamedNodes(nodes, source)) {
      const key = normalizedLookup(node.name);
      if (bySourceName[source].has(key)) throw new Error(`${source}: ambiguous skill name ${node.name}`);
      bySourceName[source].set(key, node);
    }
  }
  return bySourceName;
}

function canonicalSourceNode(group) {
  return group.bkvince ?? group.vanilla32 ?? group.pd2;
}

function mappingPriority(types) {
  const priority = [
    'PD2_ONLY_PLAYER_SKILL', 'BKV_ONLY_PLAYER_SKILL', 'RENAMED_ALIAS',
    'SLOT_REPLACEMENT', 'SAME_SKILL_MOVED_ORDINAL', 'SAME_SKILL_SAME_ORDINAL',
    'SAME_ORDINAL_DIFFERENT_SKILL', 'TECHNICAL_OR_CLASSLESS', 'IDENTICAL',
  ];
  return priority.find((type) => types.includes(type)) ?? types[0];
}

function semanticGroups(nodes) {
  const byName = relationMaps(nodes);
  const groups = [];
  const consumed = new Set();
  const addGroup = (members, relation = null) => {
    const group = { vanilla32: null, bkvince: null, pd2: null, relation };
    for (const node of members.filter(Boolean)) {
      if (consumed.has(node.id)) throw new Error(`Ambiguous governed mapping for ${node.id}`);
      consumed.add(node.id);
      group[node.source] = node;
    }
    groups.push(group);
    return group;
  };

  // Governed semantic aliases are reserved first. Slot replacements deliberately
  // remain separate semantic entities; their relation is attached later.
  for (const relation of GOVERNED_RELATIONS.filter((item) => item.semantic)) {
    const pd2 = byName.pd2.get(normalizedLookup(relation.pd2));
    const bkvince = byName.bkvince.get(normalizedLookup(relation.bkvince));
    const vanilla32 = byName.vanilla32.get(normalizedLookup(relation.bkvince));
    assert(pd2 && bkvince, `Missing governed relation ${relation.pd2}/${relation.bkvince}`);
    addGroup([vanilla32, bkvince, pd2], relation);
  }

  // Exact normalized names are the only automatic semantic edge. Runtime slots
  // are never used here; collisions are built in an independent graph below.
  for (const source of SOURCE_ORDER) {
    for (const node of uniqueNamedNodes(nodes, source)) {
      if (consumed.has(node.id)) continue;
      const key = normalizedLookup(node.name);
      const candidates = SOURCE_ORDER.map((candidate) => byName[candidate].get(key))
        .filter((candidate) => candidate && !consumed.has(candidate.id));
      if (!candidates.length) continue;
      const anchor = candidates[0];
      const compatible = candidates.filter((candidate) => {
        if (candidate === anchor) return true;
        const classCompatible = (candidate.classCode ?? null) === (anchor.classCode ?? null);
        const sameOrdinal = candidate.ordinal === anchor.ordinal;
        const descCompatible = normalizedLookup(candidate.skilldescKey) === normalizedLookup(anchor.skilldescKey);
        const treeCompatible = !candidate.classCode || !anchor.classCode
          ? descCompatible
          : candidate.tree && anchor.tree
            && candidate.tree.page === anchor.tree.page
            && candidate.tree.row === anchor.tree.row
            && candidate.tree.column === anchor.tree.column;
        return classCompatible && (sameOrdinal || (descCompatible && treeCompatible));
      });
      addGroup(compatible);
    }
  }
  assert.equal(consumed.size, nodes.filter((node) => node.name).length, 'Every named physical node must belong to exactly one semantic group');
  return { groups, byName };
}

function collisionGraph(nodes) {
  const byOrdinal = Object.fromEntries(SOURCE_ORDER.map((source) => [source, new Map(
    uniqueNamedNodes(nodes, source).map((node) => [node.ordinal, node]),
  )]));
  const collisions = [];
  const idsByNode = new Map();
  const addNodeCollision = (nodeId, collisionId) => {
    if (!idsByNode.has(nodeId)) idsByNode.set(nodeId, []);
    idsByNode.get(nodeId).push(collisionId);
  };
  const maximum = Math.min(byOrdinal.pd2.size ? Math.max(...byOrdinal.pd2.keys()) : -1,
    byOrdinal.bkvince.size ? Math.max(...byOrdinal.bkvince.keys()) : -1);
  for (let ordinal = 0; ordinal <= maximum; ordinal += 1) {
    const pd2 = byOrdinal.pd2.get(ordinal);
    const bkvince = byOrdinal.bkvince.get(ordinal);
    if (!pd2 || !bkvince || normalizedLookup(pd2.name) === normalizedLookup(bkvince.name)) continue;
    const id = `collision:pd2-bkvince:${ordinal}`;
    const semanticRelation = GOVERNED_RELATIONS.find((relation) => (
      relation.semantic
      && normalizedLookup(relation.pd2) === normalizedLookup(pd2.name)
      && normalizedLookup(relation.bkvince) === normalizedLookup(bkvince.name)
    ));
    const collision = {
      id,
      ordinal,
      kind: 'SAME_ORDINAL_DIFFERENT_SKILL',
      nodeIds: { pd2: pd2.id, bkvince: bkvince.id },
      names: { pd2: pd2.name, bkvince: bkvince.name },
      playerRelevant: Boolean(pd2.classCode || bkvince.classCode),
      resolution: semanticRelation
        ? 'RESOLVED_GOVERNED_SEMANTIC_IDENTITY'
        : 'UNRESOLVED_NO_AUTOMATIC_MERGE',
      ...(semanticRelation ? { semanticRelation: {
        kind: semanticRelation.kind,
        pd2: semanticRelation.pd2,
        bkvince: semanticRelation.bkvince,
      } } : {}),
    };
    collisions.push(collision);
    addNodeCollision(pd2.id, id);
    addNodeCollision(bkvince.id, id);
  }
  return { collisions, idsByNode };
}

function commonHeaders(documents) {
  const headerSets = documents.filter(Boolean).map((document) => new Set(
    document.table.headers.map(canonicalHeader).filter((header) => !header.startsWith('*') && header !== 'eol'),
  ));
  const union = new Set(headerSets.flatMap((set) => [...set]));
  return [...union].sort((left, right) => left.localeCompare(right, 'en'));
}

function componentForHeader(header) {
  for (const group of BEHAVIOR_GROUPS) {
    const pattern = COMPONENT_HEADER_PATTERNS[group.id];
    if (pattern?.test(header)) return group.id;
  }
  return 'consumers';
}

function proofForField(header, nodes) {
  if (NATIVE_FUNCTION_PATTERN.test(header)) return 'NATIVE_UNPROVEN';
  const findings = nodes.flatMap((node) => node?.formulaFindings ?? [])
    .filter((finding) => canonicalHeader(finding.header) === header);
  if (findings.some((finding) => finding.status === 'MALFORMED_SOURCE')) return 'MALFORMED_SOURCE';
  if (findings.some((finding) => finding.status === 'UNSUPPORTED_IDENTIFIER')) return 'UNSUPPORTED_IDENTIFIER';
  if (findings.some((finding) => finding.status === 'SYMBOLIC')) return 'SYMBOLIC';
  if (findings.length && findings.every((finding) => finding.status === 'EXACT_FORMULA')) return 'EXACT_FORMULA';
  return 'EXACT_TABLE';
}

function protectionForField(header, group, proofStatus, blockingCollisionIds, changed) {
  const reasons = [];
  if (header === 'id') reasons.push('runtime_ordinal');
  if (header === 'maxlvl') reasons.push('maxlvl');
  if (header === 'charclass') reasons.push('charclass');
  if (group.bkvince?.classCode === 'war') reasons.push('warlock');
  if (NATIVE_FUNCTION_PATTERN.test(header)) reasons.push('native_functions');
  if (changed && CLIENT_SERVER_MISSILE_PATTERN.test(header)) reasons.push('client_server_missile_behavior');
  if (['delay', 'localdelay', 'globaldelay'].includes(header)) reasons.push('delay_translation');
  if (proofStatus === 'MALFORMED_SOURCE') reasons.push('malformed_formula');
  if (blockingCollisionIds.length) reasons.push('ordinal_collision');
  return [...new Set(reasons)];
}

function sourceRecord(group, source, sources) {
  const node = group[source];
  return node ? sources.documents[source]['skills.txt'].records[node.ordinal] : null;
}

function linkedFacts(table, key, sources) {
  const pattern = LINKED_FACT_PATTERNS[table];
  if (!pattern) return [];
  const records = Object.fromEntries(SOURCE_ORDER.map((source) => [source, exactRecord(sources.documents[source][table], key)]));
  const headers = [...new Set(SOURCE_ORDER.flatMap((source) => (
    sources.documents[source][table].table.headers.map(canonicalHeader)
  )))].filter((header) => pattern.test(header)).sort((left, right) => left.localeCompare(right, 'en'));
  return headers.map((header) => {
    const values = Object.fromEntries(SOURCE_ORDER.map((source) => [source, records[source]?.get(header) ?? null]));
    return {
      header,
      label: LINKED_FACT_LABELS[header] ?? header,
      values,
      changed: new Set(Object.values(values).map((value) => JSON.stringify(value))).size > 1,
      proofStatus: NATIVE_FUNCTION_PATTERN.test(header) ? 'NATIVE_UNPROVEN' : 'EXACT_TABLE',
    };
  }).filter((fact) => fact.changed || Object.values(fact.values).some((value) => value !== null && value !== ''));
}

function exactFieldLocator(source, node, header) {
  return node ? {
    source,
    table: 'skills.txt',
    row: node.ordinal,
    key: node.name,
    header,
    nodeId: node.id,
  } : null;
}

function buildComponents(group, blockingCollisionIds, sources) {
  const presentDocuments = SOURCE_ORDER.filter((source) => group[source])
    .map((source) => sources.documents[source]['skills.txt']);
  const headers = commonHeaders(presentDocuments);
  const buckets = new Map(BEHAVIOR_GROUPS.map((item) => [item.id, []]));
  const nodeList = SOURCE_ORDER.map((source) => group[source]).filter(Boolean);
  for (const header of headers) {
    const values = Object.fromEntries(SOURCE_ORDER.map((source) => {
      const record = sourceRecord(group, source, sources);
      return [source, record ? (record.get(header) ?? null) : null];
    }));
    const presentSources = SOURCE_ORDER.filter((source) => group[source]);
    const presentValues = presentSources.map((source) => JSON.stringify(values[source]));
    const valueSet = new Set(presentValues);
    const atLeastOneNonEmptyValue = presentSources.some((source) => values[source] !== null && values[source] !== '');
    const changed = presentSources.length === 1 ? atLeastOneNonEmptyValue : valueSet.size > 1;
    // Nodes own complete raw rows exactly once. Components carry only actual
    // three-way differences; repeating hundreds of identical cells per
    // semantic entity would make the standalone oracle/HTML impractical.
    const present = SOURCE_ORDER.some((source) => values[source] !== null && values[source] !== '');
    if (!changed && !(present && REQUIRED_PRESENTATION_HEADER.test(header))) continue;
    const componentId = componentForHeader(header);
    const proofStatus = proofForField(header, nodeList);
    const protectionReasons = protectionForField(header, group, proofStatus, blockingCollisionIds, changed);
    const field = {
      id: `skills.txt:${header}`,
      table: 'skills.txt',
      header,
      label: FIELD_LABELS[header] ?? header,
      values,
      changed,
      protected: protectionReasons.length > 0,
      protectionReasons,
      proofStatus,
      ...(FORMULA_FIELD_PATTERN.test(header) ? { formula: {
        raw: values,
        findings: nodeList.flatMap((node) => node.formulaFindings)
          .filter((finding) => canonicalHeader(finding.header) === header),
      } } : {}),
      dependencyIds: [],
    };
    buckets.get(componentId).push(field);
  }
  return BEHAVIOR_GROUPS.map((definition) => {
    const fields = buckets.get(definition.id);
    const changed = fields.some((field) => field.changed);
    const proofStatus = fields.some((field) => field.proofStatus === 'MALFORMED_SOURCE')
      ? 'MALFORMED_SOURCE'
      : fields.some((field) => field.proofStatus === 'NATIVE_UNPROVEN')
        ? 'NATIVE_UNPROVEN'
        : fields.some((field) => field.proofStatus === 'UNSUPPORTED_IDENTIFIER')
          ? 'UNSUPPORTED_IDENTIFIER'
          : fields.some((field) => field.proofStatus === 'SYMBOLIC')
            ? 'SYMBOLIC'
            : fields.some((field) => field.proofStatus === 'EXACT_FORMULA')
              ? 'EXACT_FORMULA'
              : 'EXACT_TABLE';
    const portability = fields.length === 0 ? ['NOT_APPLICABLE'] : proofStatus === 'NATIVE_UNPROVEN'
      ? ['NATIVE_UNPROVEN']
      : fields.some((field) => field.dependencyIds.length) ? ['DATA_WITH_LINKED_TABLES'] : ['DATA_ONLY_PROVEN'];
    return {
      id: definition.id,
      label: definition.label,
      fingerprint: stableHash(fields.map((field) => ({ id: field.id, values: field.values, proofStatus: field.proofStatus }))),
      proofStatus,
      portability,
      changed,
      fields,
    };
  });
}

function directDependencies(group, sources) {
  const dependencies = [];
  const seen = new Set();
  const factsSeen = new Set();
  for (const source of SOURCE_ORDER) {
    const node = group[source];
    if (!node) continue;
    for (const link of node.linkedRows) {
      const id = `dependency:${source}:${link.table}:${normalizeSkillName(link.key)}:${canonicalHeader(link.field)}`;
      if (seen.has(id)) continue;
      seen.add(id);
      dependencies.push({
        id,
        source,
        table: link.table,
        field: link.field,
        key: link.key,
        ordinal: link.ordinal,
        fingerprint: link.fingerprint,
        required: source === 'pd2',
        found: link.found,
        closed: link.found,
        status: link.found ? 'RESOLVED_IN_SOURCE' : 'MISSING_IN_SOURCE',
        targetAvailability: Object.fromEntries(SOURCE_ORDER.map((candidate) => [
          candidate,
          Boolean(exactRecord(sources.documents[candidate][link.table], link.key)),
        ])),
        facts: (() => {
          const factsKey = `${link.table}:${normalizedLookup(link.key)}`;
          if (factsSeen.has(factsKey)) return [];
          factsSeen.add(factsKey);
          return linkedFacts(link.table, link.key, sources);
        })(),
      });
    }
    for (const finding of node.formulaFindings) {
      for (const match of finding.raw.matchAll(/\bskill\s*\(\s*'([^']+)'\s*\.\s*([A-Za-z0-9_]+)\s*\)/gi)) {
        const key = match[1];
        const id = `dependency:${source}:skills.txt:${normalizeSkillName(key)}:formula`;
        if (seen.has(id)) continue;
        seen.add(id);
        const target = exactRecord(sources.documents[source]['skills.txt'], key);
        dependencies.push({
          id,
          source,
          table: 'skills.txt',
          field: finding.header,
          key,
          property: match[2],
          ordinal: target?.ordinal ?? null,
          fingerprint: target ? stableHash(target.row) : null,
          required: source === 'pd2',
          found: Boolean(target),
          closed: Boolean(target),
          status: target ? 'RESOLVED_IN_SOURCE' : 'MISSING_IN_SOURCE',
          targetAvailability: Object.fromEntries(SOURCE_ORDER.map((candidate) => [
            candidate,
            Boolean(exactRecord(sources.documents[candidate]['skills.txt'], key)),
          ])),
          facts: (() => {
            const factsKey = `skills.txt:${normalizedLookup(key)}`;
            if (factsSeen.has(factsKey)) return [];
            factsSeen.add(factsKey);
            return linkedFacts('skills.txt', key, sources);
          })(),
        });
      }
    }
  }
  return dependencies.sort((left, right) => left.id.localeCompare(right.id, 'en'));
}

function consumerReferences(group, sources) {
  const names = new Set(SOURCE_ORDER.map((source) => group[source]?.name).filter(Boolean).map(normalizedLookup));
  const consumers = [];
  const referenceTables = ['skills.txt', 'charstats.txt', 'hireling.txt', 'uniqueitems.txt', 'setitems.txt', 'sets.txt', 'runes.txt', 'monstats.txt', 'properties.txt'];
  for (const source of SOURCE_ORDER) {
    for (const tableName of referenceTables) {
      const document = sources.documents[source][tableName];
      for (const record of document.records) {
        for (const header of document.table.headers) {
          const value = String(record.get(header) ?? '').trim();
          if (!value) continue;
          const textMatch = names.has(normalizedLookup(value));
          if (!textMatch) continue;
          if (tableName === 'skills.txt' && group[source]?.ordinal === record.ordinal && canonicalHeader(header) === 'skill') continue;
          consumers.push({
            id: `consumer:${source}:${tableName}:${record.ordinal}:${canonicalHeader(header)}`,
            source,
            table: tableName,
            row: record.ordinal,
            key: record.key,
            header,
            value,
            encoding: 'TEXT_NAME',
            remapRequired: false,
          });
        }
      }
    }
  }
  return consumers.sort((left, right) => left.id.localeCompare(right.id, 'en'));
}

function buildConsumerIndex(sources) {
  const index = new Map();
  const referenceTables = ['skills.txt', 'charstats.txt', 'hireling.txt', 'uniqueitems.txt', 'setitems.txt', 'sets.txt', 'runes.txt', 'monstats.txt', 'properties.txt'];
  for (const source of SOURCE_ORDER) {
    for (const tableName of referenceTables) {
      const document = sources.documents[source][tableName];
      for (const record of document.records) {
        for (const header of document.table.headers) {
          const value = String(record.get(header) ?? '').trim();
          if (!value) continue;
          const normalized = normalizedLookup(value);
          if (!index.has(normalized)) index.set(normalized, []);
          index.get(normalized).push({ source, table: tableName, row: record.ordinal, key: record.key, header, value });
        }
      }
    }
  }
  return index;
}

function indexedConsumerReferences(group, consumerIndex) {
  const names = [...new Set(SOURCE_ORDER.map((source) => group[source]?.name).filter(Boolean).map(normalizedLookup))];
  const selfNodeIds = new Set(SOURCE_ORDER.map((source) => group[source]?.id).filter(Boolean));
  const result = [];
  const seen = new Set();
  for (const name of names) {
    for (const reference of consumerIndex.get(name) ?? []) {
      if (reference.table === 'skills.txt') {
        const nodeId = physicalNodeId(reference.source, reference.row);
        if (selfNodeIds.has(nodeId) && canonicalHeader(reference.header) === 'skill') continue;
      }
      const id = `consumer:${reference.source}:${reference.table}:${reference.row}:${canonicalHeader(reference.header)}`;
      if (seen.has(id)) continue;
      seen.add(id);
      result.push({ id, ...reference, encoding: 'TEXT_NAME', remapRequired: false });
    }
  }
  return result.sort((left, right) => left.id.localeCompare(right.id, 'en'));
}

function damageWithSynergy(record, level, elemental, scenario) {
  const base = damageAtLevel(record, level, elemental);
  if (!base || !scenario.applySynergies) return { damage: base, synergy: null };
  const field = elemental ? 'edmgsympercalc' : 'dmgsympercalc';
  const raw = record.get(field);
  if (raw === undefined || String(raw).trim() === '') return { damage: base, synergy: null };
  const evaluated = evaluateSimpleFormula(raw, record, level, {
    statValue: 0,
    referencedSkillLevel: 0,
    referencedSkillLevels: scenario.referencedSkillLevels ?? {},
    applyDerivedSynergy: false,
  });
  if (!evaluated.ok) {
    return {
      damage: { ...base, min: null, max: null },
      synergy: { value: null, raw, proofStatus: evaluated.status, reason: evaluated.reason },
    };
  }
  const apply = (rawValue) => {
    if (rawValue === null) return null;
    const fixed = rawValue * (2 ** base.hitShift);
    return (fixed + Math.trunc((fixed * evaluated.value) / 100)) / 256;
  };
  return {
    damage: {
      ...base,
      min: apply(base.minRaw),
      max: apply(base.maxRaw),
    },
    synergy: {
      value: evaluated.value,
      raw,
      proofStatus: 'EXACT_FORMULA',
    },
  };
}

function curvePoint(record, level, scenario = {}) {
  if (!record) return null;
  const physical = damageWithSynergy(record, level, false, scenario);
  const elemental = damageWithSynergy(record, level, true, scenario);
  const damage = physical.damage;
  const elementalDamage = elemental.damage;
  const mana = manaCostAtLevel(record, level);
  const length = elementalLengthAtLevel(record, level);
  const evaluate = (field) => {
    const raw = record.get(field);
    if (raw === undefined || String(raw).trim() === '') return null;
    const result = evaluateSimpleFormula(raw, record, level, {
      statValue: 0,
      referencedSkillLevel: 0,
      applyDerivedSynergy: false,
    });
    return result.ok
      ? { value: result.value, proofStatus: 'EXACT_FORMULA', raw }
      : { value: null, proofStatus: result.status, raw, reason: result.reason };
  };
  const calc1Description = String(record.get('*calc1 desc') ?? '').trim();
  const calc1Count = GOVERNED_CALC1_COUNT_DESCRIPTION.test(calc1Description)
    ? { ...evaluate('calc1'), metadata: { field: '*calc1 desc', value: calc1Description } }
    : null;
  const min = damage?.min ?? elementalDamage?.min ?? null;
  const max = damage?.max ?? elementalDamage?.max ?? null;
  const elementalType = normalizedLookup(record.get('etype'));
  const poison = ['pois', 'poison'].includes(elementalType) && elementalDamage
    ? {
      encoded: {
        min: elementalDamage.minRaw,
        max: elementalDamage.maxRaw,
        proofStatus: 'EXACT_DERIVED',
      },
      duration: length === null ? null : {
        frames: length,
        seconds: length / 25,
        proofStatus: 'EXACT_DERIVED',
      },
      damagePerSecond: {
        min: null,
        max: null,
        proofStatus: 'SYMBOLIC',
        reason: 'The governed analytical engine preserves poison encoding but does not prove its runtime conversion to damage per second.',
      },
      totalDamage: {
        min: null,
        max: null,
        proofStatus: 'SYMBOLIC',
        reason: 'The governed analytical engine preserves poison encoding and duration separately; it does not prove runtime total-damage conversion.',
      },
    }
    : null;
  return {
    level,
    hardPoints: Math.min(level, Number(record.get('maxlvl')) || level),
    damage: {
      physical: damage,
      elemental: elementalDamage ? { ...elementalDamage, type: record.get('etype') || null } : null,
      synergies: {
        physical: physical.synergy,
        elemental: elemental.synergy,
      },
      min,
      max,
      average: min !== null && max !== null ? (min + max) / 2 : null,
      proofStatus: damage || elementalDamage ? 'EXACT_DERIVED' : 'NOT_APPLICABLE',
    },
    mana: mana === null ? null : { value: mana, proofStatus: 'EXACT_DERIVED' },
    duration: evaluate('auralencalc'),
    radius: evaluate('aurarangecalc'),
    projectilesOrTargets: calc1Count,
    localDelay: evaluate('localdelay'),
    globalDelay: evaluate('globaldelay'),
    pd2Delay: evaluate('delay'),
    periodicDelay: evaluate('perdelay'),
    elementalLength: length === null ? null : { value: length, frames: length, seconds: length / 25, proofStatus: 'EXACT_DERIVED' },
    poison,
    weaponModel: {
      srcdam: record.get('srcdam') || null,
      enhancedDamageFormula: record.get('dmgsympercalc') || null,
      attackRatingFormula: record.get('tohitcalc') || null,
    },
  };
}

function synergyInputs(group) {
  const result = [];
  for (const source of SOURCE_ORDER) {
    const node = group[source];
    if (!node) continue;
    for (const finding of node.formulaFindings) {
      for (const match of finding.raw.matchAll(/\bskill\s*\(\s*'([^']+)'\s*\.\s*blvl\s*\)/gi)) {
        const id = `${source}:${normalizeSkillName(match[1])}`;
        if (!result.some((item) => item.id === id)) result.push({ id, source, skill: match[1], defaultHardPoints: 0, maximum: 20 });
      }
    }
  }
  return result.sort((left, right) => left.id.localeCompare(right.id, 'en'));
}

function buildCurves(group, sources) {
  const standard = Object.fromEntries(SOURCE_ORDER.map((source) => {
    const record = sourceRecord(group, source, sources);
    return [source, LEVELS.map((level) => curvePoint(record, level))];
  }));
  const inputs = synergyInputs(group);
  const metricFor = (points, id, label, pick, proofStatus = 'EXACT_DERIVED') => ({
    id,
    label,
    values: Object.fromEntries(SOURCE_ORDER.map((source) => [
      source,
      points[source].map((point) => point === null ? null : (pick(point) ?? null)),
    ])),
    proofStatus,
  });
  const metric = (id, label, pick, proofStatus = 'EXACT_DERIVED') => metricFor(standard, id, label, pick, proofStatus);
  const standardSeries = [
    metric('damage_min', 'Minimum damage', (point) => point.damage.min),
    metric('damage_max', 'Maximum damage', (point) => point.damage.max),
    metric('damage_average', 'Average damage', (point) => point.damage.average),
    metric('mana', 'Mana', (point) => point.mana?.value ?? null),
    metric('radius', 'Radius', (point) => point.radius?.value ?? null, 'EXACT_FORMULA'),
    metric('duration', 'Duration', (point) => point.duration?.value ?? null, 'EXACT_FORMULA'),
    metric('projectiles_targets', 'Projectiles or targets', (point) => point.projectilesOrTargets?.value ?? null, 'EXACT_FORMULA'),
    metric('local_delay', 'D2R local delay', (point) => point.localDelay?.value ?? null, 'EXACT_FORMULA'),
    metric('global_delay', 'D2R global delay', (point) => point.globalDelay?.value ?? null, 'EXACT_FORMULA'),
    metric('pd2_delay', 'PD2 delay', (point) => point.pd2Delay?.value ?? null, 'EXACT_FORMULA'),
    metric('periodic_delay', 'Periodic cadence', (point) => point.periodicDelay?.value ?? null, 'EXACT_FORMULA'),
    metric('elemental_length', 'Elemental length', (point) => point.elementalLength?.frames ?? null),
    metric('poison_encoded_min', 'Poison encoded minimum', (point) => point.poison?.encoded.min ?? null),
    metric('poison_encoded_max', 'Poison encoded maximum', (point) => point.poison?.encoded.max ?? null),
    metric('poison_duration_frames', 'Poison duration (frames)', (point) => point.poison?.duration?.frames ?? null),
    metric('poison_duration_seconds', 'Poison duration (seconds)', (point) => point.poison?.duration?.seconds ?? null),
    metric('poison_dps_min', 'Poison damage per second minimum', (point) => point.poison?.damagePerSecond.min ?? null, 'SYMBOLIC'),
    metric('poison_dps_max', 'Poison damage per second maximum', (point) => point.poison?.damagePerSecond.max ?? null, 'SYMBOLIC'),
    metric('poison_total_min', 'Poison total damage minimum', (point) => point.poison?.totalDamage.min ?? null, 'SYMBOLIC'),
    metric('poison_total_max', 'Poison total damage maximum', (point) => point.poison?.totalDamage.max ?? null, 'SYMBOLIC'),
  ].filter((item) => SOURCE_ORDER.some((source) => item.values[source].some((value) => value !== null && value !== undefined)));
  const standardScenario = {
    id: 'standard',
    label: 'Sans synergie',
    levels: LEVELS,
    assumptions: { otherSkills: 0, stats: 0, masteries: 0 },
    series: standardSeries,
    symbolic: [
      ...SOURCE_ORDER.flatMap((source) => standard[source].flatMap((point) => point
      ? ['radius', 'duration', 'projectilesOrTargets', 'localDelay', 'globalDelay', 'pd2Delay', 'periodicDelay']
        .filter((key) => point[key]?.value === null && point[key]?.raw)
        .map((key) => ({ source, level: point.level, metric: key, ...point[key] }))
      : [])),
      ...SOURCE_ORDER.flatMap((source) => (group[source]?.formulaFindings ?? [])
        .filter((finding) => ['SYMBOLIC', 'UNSUPPORTED_IDENTIFIER', 'MALFORMED_SOURCE', 'NATIVE_UNPROVEN'].includes(finding.status))),
      ...SOURCE_ORDER.flatMap((source) => standard[source].flatMap((point) => point?.poison ? [
        { source, level: point.level, metric: 'poisonDamagePerSecond', ...point.poison.damagePerSecond },
        { source, level: point.level, metric: 'poisonTotalDamage', ...point.poison.totalDamage },
      ] : [])),
    ],
  };
  const buildSynergyScenario = (id, label, hardPoints) => {
    const hardPointsBySkill = Object.fromEntries(SOURCE_ORDER.map((source) => [
      source,
      Object.fromEntries(inputs.filter((input) => input.source === source).map((input) => [normalizeSkillName(input.skill), hardPoints])),
    ]));
    const points = Object.fromEntries(SOURCE_ORDER.map((source) => {
      const record = sourceRecord(group, source, sources);
      return [source, LEVELS.map((level) => curvePoint(record, level, {
        applySynergies: true,
        referencedSkillLevels: hardPointsBySkill[source],
      }))];
    }));
    const candidates = [
      metricFor(points, 'damage_min', 'Minimum damage', (point) => point.damage.min),
      metricFor(points, 'damage_max', 'Maximum damage', (point) => point.damage.max),
      metricFor(points, 'damage_average', 'Average damage', (point) => point.damage.average),
      metricFor(points, 'mana', 'Mana', (point) => point.mana?.value),
    ];
    const series = candidates.filter((item) => SOURCE_ORDER.some((source) => item.values[source].some((value) => value !== null)));
    const symbolic = SOURCE_ORDER.flatMap((source) => points[source].flatMap((point) => (
      point ? Object.entries(point.damage.synergies)
        .filter(([, synergy]) => synergy?.value === null)
        .map(([kind, synergy]) => ({ source, level: point.level, metric: `${kind}DamageSynergy`, ...synergy })) : []
    )));
    return {
      id,
      label,
      labelFr: label,
      levels: LEVELS,
      synergyInputs: inputs.map((input) => ({ ...input, hardPoints })),
      hardPointsBySkill,
      proofStatus: inputs.length === 0 ? 'NOT_APPLICABLE' : symbolic.length ? 'SYMBOLIC' : 'EXACT_FORMULA',
      series,
      symbolic,
    };
  };
  const synergies20Scenario = buildSynergyScenario('synergies20', 'Synergies à 20 hard points', 20);
  const customScenario = {
    ...buildSynergyScenario('custom', 'Synergies personnalisées', 0),
    calculationModel: {
      id: 'HARD_POINT_MAP_V1',
      evaluator: 'GOVERNED_SKILLCALC_FORMULA',
      unresolvedValuesRemainSymbolic: true,
    },
    formulas: Object.fromEntries(SOURCE_ORDER.map((source) => [
      source,
      group[source]?.formulaFindings.filter((finding) => /skill\s*\(/i.test(finding.raw)) ?? [],
    ])),
  };
  return {
    levels: LEVELS,
    standard: standardScenario,
    synergies20: synergies20Scenario,
    custom: customScenario,
    scenarios: [standardScenario, synergies20Scenario, customScenario],
  };
}

function treeForGroup(group) {
  const coordinatesBySource = Object.fromEntries(SOURCE_ORDER.map((source) => [source, group[source]?.tree ?? null]));
  const preferred = group.bkvince?.tree ?? group.pd2?.tree ?? group.vanilla32?.tree ?? null;
  const classCode = group.bkvince?.classCode ?? group.pd2?.classCode ?? group.vanilla32?.classCode ?? null;
  const page = preferred?.page ?? null;
  return {
    page,
    row: preferred?.row ?? null,
    column: preferred?.column ?? null,
    listRow: preferred?.listRow ?? null,
    label: classCode && page ? (CLASS_TREE_LABELS[classCode]?.[page] ?? `${CLASS_NAMES[classCode] ?? classCode} — Tree ${page}`) : 'Technical / classless',
    coordinatesBySource,
  };
}

function sameRawAcrossPresent(group) {
  const present = SOURCE_ORDER.map((source) => group[source]).filter(Boolean);
  if (present.length < 2) return false;
  const headerSets = present.map((node) => new Set(Object.keys(node.raw).map(canonicalHeader)
    .filter((header) => !header.startsWith('*') && header !== 'id' && header !== 'eol')));
  const headers = [...headerSets[0]].filter((header) => headerSets.every((set) => set.has(header)));
  return headers.every((header) => {
    const values = present.map((node) => {
      const key = Object.keys(node.raw).find((candidate) => canonicalHeader(candidate) === header);
      return key === undefined ? undefined : node.raw[key];
    });
    return values.every((value) => value === values[0]);
  });
}

function playerClassification(group) {
  const candidates = SOURCE_ORDER.map((source) => group[source]).filter(Boolean);
  const placed = candidates.filter((node) => (
    PLAYER_CLASS_CODES.includes(node.classCode)
      && node.tree
      && Number.isInteger(node.tree.page) && node.tree.page >= 1 && node.tree.page <= 3
      && Number.isInteger(node.tree.row) && node.tree.row >= 1 && node.tree.row <= 6
      && Number.isInteger(node.tree.column) && node.tree.column >= 1 && node.tree.column <= 3
      && !TECHNICAL_NAME_PATTERN.test(node.name)
  ));
  const override = group.pd2 && AUDIT_PLAYER_OVERRIDES.get(group.pd2.name);
  const pd2Only = Boolean(group.pd2 && !group.bkvince && !group.vanilla32);
  const isSlotReplacement = Boolean(group.pd2 && GOVERNED_RELATIONS.some((relation) => (
    relation.kind === 'SLOT_REPLACEMENT' && relation.pd2 === group.pd2.name
  )));
  const movedSemantic = Boolean(group.pd2 && ['Cold Enchant', 'Lightning Sentry'].includes(group.pd2.name));
  const newCandidate = pd2Only && !isSlotReplacement && !movedSemantic
    && (Boolean(override) || placed.some((node) => node.source === 'pd2'));
  if (newCandidate && group.relation) return {
    playerSkill: false,
    newPd2PlayerSkill: false,
    category: 'SEMANTIC_RELATION_NOT_NEW',
    reasons: ['A governed alias/semantic relation cannot be classified as a new PD2 player skill.'],
    evidence: { relation: group.relation },
  };
  if (newCandidate) return {
    playerSkill: true,
    newPd2PlayerSkill: true,
    category: override ? 'PLAYER_CANDIDATE_AUDIT_OVERRIDE' : 'PLAYER_CANDIDATE_TREE_PLACEMENT',
    reasons: [override || 'PD2-only semantic identity has a valid class skilldesc placement and no technical-name exclusion.'],
    evidence: { pd2NodeId: group.pd2.id, tree: group.pd2.tree, override: Boolean(override) },
  };
  if (placed.length) return {
    playerSkill: true,
    newPd2PlayerSkill: false,
    category: 'PLAYER_SKILL_TREE_PLACEMENT',
    reasons: ['At least one governed semantic source has a valid class tree placement and no technical-name exclusion.'],
    evidence: { nodeIds: placed.map((node) => node.id), trees: placed.map((node) => node.tree) },
  };
  if (group.bkvince?.classCode === 'war' && group.bkvince.tree
    && group.bkvince.tree.page >= 1 && group.bkvince.tree.page <= 3) return {
    playerSkill: true,
    newPd2PlayerSkill: false,
    category: 'BKVINCE_WARLOCK_TREE_SKILL',
    reasons: ['The BKVince Warlock row has a governed Warlock skill tree placement.'],
    evidence: { nodeId: group.bkvince.id, tree: group.bkvince.tree },
  };
  return {
    playerSkill: false,
    newPd2PlayerSkill: false,
    category: 'TECHNICAL_OR_CLASSLESS',
    reasons: [pd2Only && group.pd2?.classCode
      ? 'charclass alone is insufficient; no valid governed player placement or explicit audit override was proven.'
      : 'No governed player-tree placement was proven.'],
    evidence: { nodeIds: candidates.map((node) => node.id) },
  };
}

function semanticIdentityEvidence(group) {
  if (group.relation?.semantic) return {
    status: 'PROVEN',
    signals: [group.relation.kind === 'RENAMED_ALIAS' ? 'GOVERNED_ALIAS' : 'GOVERNED_MOVED_IDENTITY', 'AUDIT_EVIDENCE'],
    reason: `${group.relation.pd2} and ${group.relation.bkvince} are a frozen governed semantic relation.`,
  };
  const present = SOURCE_ORDER.map((source) => group[source]).filter(Boolean);
  if (present.length === 1) return {
    status: 'PROVEN',
    signals: ['SINGLE_SOURCE_PHYSICAL_IDENTITY'],
    reason: 'A single-source physical row defines one semantic entity without a cross-source merge.',
  };
  const classes = new Set(present.map((node) => node.classCode ?? null));
  const descs = new Set(present.map((node) => normalizedLookup(node.skilldescKey)));
  const sameOrdinal = new Set(present.map((node) => node.ordinal)).size === 1;
  const compatibleTree = present.every((node) => !node.classCode || !node.tree)
    || present.filter((node) => node.classCode && node.tree).every((node, _, array) => (
      node.tree.page === array[0].tree.page && node.tree.row === array[0].tree.row && node.tree.column === array[0].tree.column
    ));
  const status = classes.size === 1 && (sameOrdinal || (descs.size === 1 && compatibleTree)) ? 'PROVEN' : 'UNRESOLVED';
  return {
    status,
    signals: [classes.size === 1 ? 'COMPATIBLE_CLASS' : 'CLASS_MISMATCH', sameOrdinal ? 'SAME_RUNTIME_ORDINAL' : 'MOVED_RUNTIME_ORDINAL', descs.size === 1 ? 'COMPATIBLE_SKILLDESC' : 'SKILLDESC_MISMATCH', compatibleTree ? 'COMPATIBLE_TREE_OR_CLASSLESS' : 'TREE_MISMATCH'],
    reason: status === 'PROVEN'
      ? 'Same normalized name is corroborated by compatible class/scope, skilldesc and tree evidence.'
      : 'Name equality alone is insufficient; the cross-source semantic identity remains unresolved.',
  };
}

function playerSummary(components, group) {
  const changed = components.flatMap((component) => component.fields)
    .filter((field) => field.changed);
  if (!changed.length) return 'Les trois sources décrivent le même modèle de données pour cette skill.';
  const phrases = [];
  const by = (header) => changed.find((field) => field.header === header);
  if (by('mana')) phrases.push('coût de mana différent');
  if (by('aurarangecalc')) phrases.push('rayon différent');
  if (by('auralencalc')) phrases.push('durée différente');
  if (changed.some((field) => /missile/i.test(field.header))) phrases.push('projectiles ou trajectoires différents');
  if (changed.some((field) => /(?:mindam|maxdam|emin|emax|srcdam)/i.test(field.header))) phrases.push('modèle de dégâts différent');
  if (changed.some((field) => NATIVE_FUNCTION_PATTERN.test(field.header))) phrases.push('fonction native différente et comportement non prouvé');
  if (group.pd2 && !group.bkvince) phrases.unshift('nouvelle skill joueur PD2 absente de BKVince');
  if (phrases.length) return `${phrases.join('; ')}.`;
  const componentPhrases = components.filter((component) => component.changed && component.fields.length)
    .map((component) => component.label.toLowerCase());
  if (canonicalSourceNode(group)?.classCode && componentPhrases.length) {
    return `Changements de ${componentPhrases.slice(0, 4).join(', ')}${componentPhrases.length > 4 ? ' et autres comportements' : ''}.`;
  }
  return `${changed.length} différences techniques regroupées par comportement.`;
}

function buildPortability(group, components, dependencies, collisionIds, newPlayer) {
  const categories = new Set();
  const reasons = [];
  const tables = new Set(['skills.txt']);
  for (const dependency of dependencies) tables.add(dependency.table);
  if (newPlayer) {
    categories.add('APPEND_ONLY_REQUIRED');
    categories.add('BLOCKED_DEPENDENCY');
    categories.add('NATIVE_UNPROVEN');
    reasons.push('The PD2 player skill is absent from BKVince and may only be proposed after the last real BKVince ordinal.');
    reasons.push('Ordinal-encoded consumer coverage and binary PD2 localization text remain unproven; preview must block the proposal until both closures are governed.');
  }
  if (collisionIds.length) {
    categories.add('SAVE_OR_ID_RISK');
    reasons.push('The PD2 runtime ordinal is occupied by a different BKVince skill; no slot merge is safe.');
  }
  if (dependencies.some((item) => item.required && !item.closed)) {
    categories.add('BLOCKED_DEPENDENCY');
    reasons.push('At least one required PD2 dependency is unresolved in its source graph.');
  }
  if (dependencies.some((item) => item.required && !item.targetAvailability.bkvince)) {
    categories.add('DATA_WITH_LINKED_TABLES');
    reasons.push('One or more PD2 dependencies are absent from BKVince and require linked-table planning.');
  }
  const fields = components.flatMap((component) => component.fields);
  const nativeChanged = fields.some((field) => field.changed && NATIVE_FUNCTION_PATTERN.test(field.header));
  if (nativeChanged) {
    categories.add('NATIVE_FUNCTION_MISMATCH');
    categories.add('NATIVE_UNPROVEN');
    categories.add('NETWORK_OR_CLIENT_SERVER_RISK');
    reasons.push('Numeric callbacks are source-specific; changed server/client functions need D2R 3.2 native proof.');
  }
  if (!categories.size) categories.add(fields.some((field) => field.changed) ? 'DATA_ONLY_PROVEN' : 'NOT_APPLICABLE');
  return {
    categories: [...categories],
    classification: [...categories],
    reasons,
    tables: [...tables].sort(),
    missingDependencies: dependencies.filter((item) => item.required && !item.closed).map((item) => item.id),
    collisions: collisionIds,
    divergentFunctions: fields.filter((field) => field.changed && NATIVE_FUNCTION_PATTERN.test(field.header)).map((field) => field.header),
    saveRisk: collisionIds.length ? 'UNRESOLVED_ORDINAL_COLLISION' : 'NO_TABLE_ONLY_SAVE_RISK_DEMONSTRATED',
    networkRisk: nativeChanged ? 'CLIENT_SERVER_BEHAVIOR_UNPROVEN' : 'NO_DIVERGENT_NATIVE_FUNCTION_SELECTED',
    effort: newPlayer ? 'HIGH' : nativeChanged ? 'MEDIUM_OR_HIGH' : fields.some((field) => field.changed) ? 'LOW_OR_MEDIUM' : 'NOT_APPLICABLE',
    proofRequired: nativeChanged ? ['D2R_3_2_NATIVE_FUNCTION_AUDIT'] : [],
  };
}

function proposedBkvinceRow(group, proposedOrdinal, sources) {
  const pd2Document = sources.documents.pd2['skills.txt'];
  const bkvDocument = sources.documents.bkvince['skills.txt'];
  const record = sourceRecord(group, 'pd2', sources);
  const mapped = {};
  const provenance = {};
  for (const targetHeader of bkvDocument.table.headers) {
    const canonical = canonicalHeader(targetHeader);
    let value = '';
    const sourceHeader = pd2Document.table.headers.find((header) => canonicalHeader(header) === canonical);
    if (sourceHeader !== undefined) {
      value = record.get(sourceHeader) ?? '';
      provenance[targetHeader] = { mode: 'EXACT_CANONICAL_HEADER', sourceHeader };
    } else {
      provenance[targetHeader] = { mode: 'UNAVAILABLE_IN_PD2_BLANK' };
    }
    mapped[targetHeader] = value;
  }
  const documentaryId = bkvDocument.table.headers.find((header) => canonicalHeader(header) === 'id');
  if (documentaryId) {
    mapped[documentaryId] = String(proposedOrdinal);
    provenance[documentaryId] = {
      mode: 'APPEND_PREVIEW_DOCUMENTARY_VALUE',
      statement: 'The value mirrors the proposed append ordinal for documentation only; it was not used to allocate the ordinal.',
    };
  }
  return {
    sourceNodeId: group.pd2.id,
    targetTable: 'skills.txt',
    targetOrdinal: proposedOrdinal,
    targetHeaders: bkvDocument.table.headers,
    values: mapped,
    mappingProvenance: provenance,
  };
}

function localizationPlan(group, sources) {
  const pd2Skilldesc = exactRecord(sources.documents.pd2['skilldesc.txt'], group.pd2?.skilldescKey);
  const fields = ['str name', 'str short', 'str long', 'str alt'];
  const localizations = fields.map((header) => {
    const key = String(pd2Skilldesc?.get(header) ?? '').trim();
    const target = key ? sources.localization.bkvinceByKey.get(key) : null;
    return {
      id: `localization:${normalizeSkillName(group.pd2?.name)}:${normalizeSkillName(header)}`,
      key: key || null,
      field: header,
      required: header !== 'str alt',
      sourceText: null,
      status: !key ? 'MISSING_KEY_IN_PD2_SKILLDESC'
        : target ? 'TARGET_KEY_EXISTS_SOURCE_TEXT_UNPROVEN' : 'TARGET_KEY_MISSING_SOURCE_TEXT_UNPROVEN',
      provenance: {
        source: 'pd2:patchstring.tbl',
        sourceFormat: 'UNSUPPORTED_BINARY_FORMAT_HASHED_ONLY',
        target: 'bkvince:skills.json',
      },
      targetExists: Boolean(target),
    };
  });
  return {
    localizations,
    stringsRequired: localizations,
    localizationClosure: {
      required: localizations.filter((item) => item.required).map((item) => item.id),
      closed: localizations.filter((item) => item.required && item.sourceText && item.targetExists).map((item) => item.id),
      missing: localizations.filter((item) => item.required && (!item.sourceText || !item.targetExists)).map((item) => item.id),
      complete: localizations.every((item) => !item.required || (item.sourceText && item.targetExists)),
    },
  };
}

function newSkillPlan(group, proposedOrdinal, dependencies, collisionIds, collisions, sources) {
  if (!group.pd2 || group.bkvince || !Number.isInteger(proposedOrdinal)) return undefined;
  const linked = (table) => dependencies.filter((item) => item.source === 'pd2' && item.table === table);
  const localization = localizationPlan(group, sources);
  const sourceDependencyComplete = dependencies.every((item) => item.source !== 'pd2' || !item.required || item.closed);
  const targetDependencyComplete = dependencies.every((item) => item.source !== 'pd2' || !item.required || item.targetAvailability.bkvince);
  return {
    state: 'PREVIEW_ONLY_NOT_APPROVED',
    sourceOrdinal: group.pd2.ordinal,
    proposedTargetOrdinal: proposedOrdinal,
    appendOnly: true,
    insertionBeforeExistingRowsForbidden: true,
    currentOccupantAtPd2Ordinal: (() => {
      const collision = collisions.find((item) => item.ordinal === group.pd2.ordinal);
      return collision ? {
        source: 'bkvince',
        ordinal: collision.ordinal,
        nodeId: collision.nodeIds.bkvince,
        stableId: null,
        name: collision.names.bkvince,
      } : null;
    })(),
    directDependencies: dependencies.filter((item) => item.source === 'pd2').map((item) => item.id),
    missilesRequired: linked('missiles.txt').map((item) => item.key),
    statesRequired: linked('states.txt').map((item) => item.key),
    skilldescRequired: linked('skilldesc.txt').map((item) => item.key),
    itemStatCostRequired: linked('itemstatcost.txt').map((item) => item.key),
    pettypesRequired: linked('pettype.txt').map((item) => item.key),
    monstatsRequired: linked('monstats.txt').map((item) => item.key),
    dependencyClosure: {
      required: dependencies.filter((item) => item.source === 'pd2' && item.required).map((item) => item.id),
      closed: dependencies.filter((item) => item.source === 'pd2' && item.required && item.closed).map((item) => item.id),
      missing: dependencies.filter((item) => item.source === 'pd2' && item.required && !item.closed).map((item) => item.id),
      directCompleteInSource: sourceDependencyComplete,
      transitiveStatus: 'UNPROVEN',
      completeInSource: false,
      completeForBkvince: false,
      blockingGates: [
        'TRANSITIVE_DEPENDENCY_CLOSURE_UNPROVEN',
        ...(targetDependencyComplete ? [] : ['LINKED_TABLE_TARGETS']),
        ...(localization.localizationClosure.complete ? [] : ['LOCALIZATION']),
        'ORDINAL_CONSUMERS_NATIVE_UNPROVEN',
      ],
    },
    collisionIds,
    remappingPlan: 'Append the complete governed dependency closure after the current BKVince tail and remap every name/ordinal consumer in preview; never insert or move an existing row.',
    testsRequired: ['dependency-closure', 'ordinal-collision', 'native-functions', 'client-server', 'localization', 'consumer-remap'],
    proposedRow: proposedBkvinceRow(group, proposedOrdinal, sources),
    ...localization,
    consumerClosure: {
      status: 'NATIVE_UNPROVEN',
      complete: false,
      reasons: ['No generic numeric Param-to-skill ordinal heuristic is permitted; ordinal-encoded consumers require a governed table/function-specific proof.'],
      remapRequired: true,
    },
  };
}

function documentationFor(group) {
  const pd2 = group.pd2;
  return pd2 ? [{
    section: 'Skill Changes',
    revision: 23785,
    season: 'S9–S11 overview (page warns it may be outdated)',
    url: 'https://wiki.projectdiablo2.com/w/index.php?title=Skill_Changes&oldid=23785',
    summary: 'Pinned conceptual PD2 skill reference; table values remain authoritative.',
    status: 'TABLE_ONLY',
    portabilityEvidence: false,
  }] : [];
}

function buildSkills(groups, collisionData, sources) {
  const nextAppendOrdinal = sources.documents.bkvince['skills.txt'].table.rows.length;
  const collisionById = new Map(collisionData.collisions.map((item) => [item.id, item]));
  const nodeToStable = new Map();
  const skills = [];
  const consumerIndex = buildConsumerIndex(sources);
  const rowFingerprintByNode = new Map(groups.flatMap((group) => SOURCE_ORDER
    .map((source) => group[source]).filter(Boolean).map((node) => [node.id, node.rowFingerprint])));
  const appendGroups = groups.filter((group) => playerClassification(group).newPd2PlayerSkill).sort((left, right) => {
    const classDelta = CLASS_ORDER.indexOf(left.pd2.classCode) - CLASS_ORDER.indexOf(right.pd2.classCode);
    if (classDelta) return classDelta;
    const lt = [left.pd2.tree?.page ?? 999, left.pd2.tree?.row ?? 999, left.pd2.tree?.column ?? 999, left.pd2.ordinal];
    const rt = [right.pd2.tree?.page ?? 999, right.pd2.tree?.row ?? 999, right.pd2.tree?.column ?? 999, right.pd2.ordinal];
    for (let index = 0; index < lt.length; index += 1) if (lt[index] !== rt[index]) return lt[index] - rt[index];
    return 0;
  });
  const appendOrdinalByNode = new Map(appendGroups.map((group, index) => [group.pd2.id, nextAppendOrdinal + index]));
  for (const group of groups) {
    const canonical = canonicalSourceNode(group);
    const classCode = canonical.classCode ?? group.pd2?.classCode ?? group.bkvince?.classCode ?? null;
    const scope = classCode || 'classless';
    const rawCanonicalName = group.bkvince?.name || group.vanilla32?.name || group.pd2?.name;
    const canonicalName = CANONICAL_DISPLAY_ALIASES[rawCanonicalName] ?? rawCanonicalName;
    let stableId = stableSkillId(scope, canonicalName);
    if (skills.some((skill) => skill.stableId === stableId)) {
      const discriminatorNode = canonicalSourceNode(group);
      stableId = stableSkillId(scope, canonicalName, `${discriminatorNode.source}-${discriminatorNode.ordinal}`);
    }
    const nodeIds = Object.fromEntries(SOURCE_ORDER.map((source) => [source, group[source]?.id ?? null]));
    const collisionIds = [...new Set(SOURCE_ORDER.flatMap((source) => collisionData.idsByNode.get(group[source]?.id) ?? []))];
    const collisions = collisionIds.map((id) => collisionById.get(id)).filter(Boolean);
    const blockingCollisionIds = collisions
      .filter((collision) => collision.resolution === 'UNRESOLVED_NO_AUTOMATIC_MERGE')
      .map((collision) => collision.id);
    const classification = playerClassification(group);
    const newPlayer = classification.newPd2PlayerSkill;
    const playerSkill = classification.playerSkill;
    const bkvOnlyPlayer = Boolean(group.bkvince && !group.pd2 && playerSkill);
    const mappingTypes = [];
    if (group.relation?.kind === 'RENAMED_ALIAS') mappingTypes.push('RENAMED_ALIAS');
    const identityEvidence = semanticIdentityEvidence(group);
    if (group.pd2 && group.bkvince && identityEvidence.status === 'PROVEN') {
      mappingTypes.push(group.pd2.ordinal === group.bkvince.ordinal ? 'SAME_SKILL_SAME_ORDINAL' : 'SAME_SKILL_MOVED_ORDINAL');
    }
    if (newPlayer) mappingTypes.push('PD2_ONLY_PLAYER_SKILL');
    if (bkvOnlyPlayer) mappingTypes.push('BKV_ONLY_PLAYER_SKILL');
    if (blockingCollisionIds.length) mappingTypes.push('SAME_ORDINAL_DIFFERENT_SKILL');
    if (!playerSkill || (group.pd2 && !group.bkvince && !newPlayer)) mappingTypes.push('TECHNICAL_OR_CLASSLESS');
    const identical = sameRawAcrossPresent(group);
    if (identical) mappingTypes.push('IDENTICAL');
    const components = buildComponents(group, blockingCollisionIds, sources);
    const dependencies = directDependencies(group, sources);
    for (const component of components) {
      for (const field of component.fields) {
        field.dependencyIds = dependencies
          .filter((dependency) => canonicalHeader(dependency.field) === field.header)
          .map((dependency) => dependency.id);
      }
      if (component.fields.some((field) => field.dependencyIds.length)) {
        component.portability = [...new Set(component.portability
          .filter((category) => category !== 'DATA_ONLY_PROVEN').concat('DATA_WITH_LINKED_TABLES'))];
      }
      component.fingerprint = stableHash(component.fields.map((field) => ({
        id: field.id, values: field.values, proofStatus: field.proofStatus, dependencyIds: field.dependencyIds,
      })));
    }
    const consumers = indexedConsumerReferences(group, consumerIndex);
    const portability = buildPortability(group, components, dependencies, blockingCollisionIds, newPlayer);
    const tree = treeForGroup(group);
    const evidenceStatuses = [...new Set([
      ...SOURCE_ORDER.flatMap((source) => group[source]?.formulaFindings.map((finding) => finding.status) ?? []),
      ...components.map((component) => component.proofStatus),
    ])];
    const skill = {
      stableId,
      canonicalName,
      aliases: [...new Set(SOURCE_ORDER.map((source) => group[source]?.name).filter(Boolean).filter((name) => name !== canonicalName))],
      names: Object.fromEntries(SOURCE_ORDER.map((source) => [source, group[source]?.name ?? null])),
      classCode,
      scope,
      classification,
      semanticIdentityEvidence: identityEvidence,
      playerSkill,
      newPd2PlayerSkill: newPlayer,
      bkvinceOnlyPlayerSkill: bkvOnlyPlayer,
      nodeIds,
      ordinals: Object.fromEntries(SOURCE_ORDER.map((source) => [source, group[source]?.ordinal ?? null])),
      tree,
      mappingTypes: [...new Set(mappingTypes)],
      primaryMappingType: mappingPriority(mappingTypes),
      identical,
      readOnly: identical,
      status: identical ? 'IDENTICAL_AUTO_RESOLVED_READ_ONLY' : newPlayer ? 'NEW_PD2_PLAYER_CANDIDATE_UNAPPROVED' : 'REVIEW_REQUIRED',
      collisionIds,
      blockingCollisionIds,
      summary: playerSummary(components, group),
      evidence: {
        overall: evidenceStatuses.includes('MALFORMED_SOURCE') ? 'MALFORMED_SOURCE'
          : evidenceStatuses.includes('NATIVE_UNPROVEN') ? 'NATIVE_UNPROVEN'
            : evidenceStatuses.includes('UNSUPPORTED_IDENTIFIER') ? 'UNSUPPORTED_IDENTIFIER'
              : evidenceStatuses.includes('SYMBOLIC') ? 'SYMBOLIC'
                : evidenceStatuses.includes('EXACT_FORMULA') ? 'EXACT_FORMULA' : 'EXACT_TABLE',
        statuses: evidenceStatuses,
        findings: SOURCE_ORDER.flatMap((source) => group[source]?.formulaFindings ?? []),
      },
      portability,
      components,
      curves: buildCurves(group, sources),
      dependencies,
      dependencyClosure: {
        required: dependencies.filter((item) => item.required).map((item) => item.id),
        closed: dependencies.filter((item) => item.required && item.closed).map((item) => item.id),
        missing: dependencies.filter((item) => item.required && !item.closed).map((item) => item.id),
        directComplete: dependencies.every((item) => !item.required || item.closed),
        transitiveStatus: dependencies.length ? 'UNPROVEN' : 'NOT_APPLICABLE',
        complete: dependencies.length === 0,
        blockingGates: dependencies.length ? ['TRANSITIVE_DEPENDENCY_CLOSURE_UNPROVEN'] : [],
      },
      consumers,
      documentation: documentationFor(group),
      newSkillPlan: newSkillPlan(group, appendOrdinalByNode.get(group.pd2?.id), dependencies, collisionIds, collisions, sources),
    };
    skills.push(skill);
    for (const nodeId of Object.values(nodeIds).filter(Boolean)) nodeToStable.set(nodeId, stableId);
  }
  for (const relation of GOVERNED_RELATIONS.filter((item) => !item.semantic)) {
    const pd2 = groups.flatMap((group) => [group.pd2]).find((node) => node?.name === relation.pd2);
    const bkvince = groups.flatMap((group) => [group.bkvince]).find((node) => node?.name === relation.bkvince);
    if (!pd2 || !bkvince) throw new Error(`Missing slot replacement ${relation.pd2}/${relation.bkvince}`);
    const vanillaOccupant = groups.flatMap((group) => [group.vanilla32])
      .find((node) => node?.ordinal === pd2.ordinal);
    for (const stableId of [
      nodeToStable.get(pd2.id),
      nodeToStable.get(bkvince.id),
      nodeToStable.get(vanillaOccupant?.id),
    ].filter(Boolean)) {
      const skill = skills.find((item) => item.stableId === stableId);
      if (!skill.mappingTypes.includes('SLOT_REPLACEMENT')) skill.mappingTypes.push('SLOT_REPLACEMENT');
      skill.primaryMappingType = mappingPriority(skill.mappingTypes);
    }
  }
  for (const skill of skills) {
    const occupant = skill.newSkillPlan?.currentOccupantAtPd2Ordinal;
    if (occupant) occupant.stableId = nodeToStable.get(occupant.nodeId) ?? null;
  }
  for (const skill of skills) {
    skill.fingerprint = stableHash({
      stableId: skill.stableId,
      nodeIds: skill.nodeIds,
      ordinals: skill.ordinals,
      rows: Object.fromEntries(SOURCE_ORDER.map((source) => [
        source,
        rowFingerprintByNode.get(skill.nodeIds[source]) ?? null,
      ])),
      mappingTypes: skill.mappingTypes,
      semanticIdentityEvidence: skill.semanticIdentityEvidence,
      classification: skill.classification,
      collisionIds: skill.collisionIds,
      dependencies: skill.dependencies,
      dependencyClosure: skill.dependencyClosure,
      newSkillPlan: skill.newSkillPlan,
      protections: skill.components.flatMap((component) => component.fields)
        .filter((field) => field.protected).map((field) => ({ id: field.id, reasons: field.protectionReasons })),
    });
  }
  for (const collision of collisionData.collisions) {
    collision.skillIds = {
      pd2: nodeToStable.get(collision.nodeIds.pd2),
      bkvince: nodeToStable.get(collision.nodeIds.bkvince),
    };
    collision.fingerprint = stableHash(collision);
  }
  return skills.sort((left, right) => left.stableId.localeCompare(right.stableId, 'en'));
}

function navigationOrder(left, right) {
  const tuple = (skill) => [
    skill.tree.page ?? 999,
    skill.tree.row ?? 999,
    skill.tree.column ?? 999,
    skill.tree.listRow ?? 999,
    skill.stableId,
  ];
  const a = tuple(left);
  const b = tuple(right);
  for (let index = 0; index < a.length; index += 1) {
    const result = typeof a[index] === 'number' ? a[index] - b[index] : String(a[index]).localeCompare(String(b[index]), 'en');
    if (result) return result;
  }
  return 0;
}

function buildNavigation(skills) {
  const navigation = [];
  for (const classCode of CLASS_ORDER) {
    const classSkills = skills.filter((skill) => skill.playerSkill && skill.classCode === classCode).sort(navigationOrder);
    if (!classSkills.length) continue;
    const trees = [];
    for (const page of [1, 2, 3]) {
      const items = classSkills.filter((skill) => skill.tree.page === page).sort(navigationOrder);
      if (!items.length) continue;
      trees.push({
        id: `${classCode}:page:${page}`,
        page,
        label: CLASS_TREE_LABELS[classCode]?.[page] ?? `${CLASS_NAMES[classCode]} — Tree ${page}`,
        skillIds: items.map((skill) => skill.stableId),
        coordinates: items.map((skill) => ({ stableId: skill.stableId, page: skill.tree.page, row: skill.tree.row, column: skill.tree.column })),
      });
    }
    navigation.push({ id: classCode, label: CLASS_NAMES[classCode], kind: 'class', trees, skillIds: classSkills.map((skill) => skill.stableId) });
  }
  const special = [
    ['pd2_new', 'Nouveaux skills PD2', (skill) => skill.newPd2PlayerSkill],
    ['bkv_only', 'Skills propres à BKVince', (skill) => skill.bkvinceOnlyPlayerSkill],
    ['collisions', 'Collisions et remplacements', (skill) => skill.collisionIds.length || skill.mappingTypes.includes('SLOT_REPLACEMENT')],
    ['technical', 'Skills techniques / classless', (skill) => skill.mappingTypes.includes('TECHNICAL_OR_CLASSLESS')],
  ];
  for (const [id, label, predicate] of special) {
    navigation.push({ id, label, kind: 'special', trees: [], skillIds: skills.filter(predicate).sort(navigationOrder).map((skill) => skill.stableId) });
  }
  return navigation;
}

function coverageFor(nodes, skills, collisions, sources) {
  const byClass = Object.fromEntries(CLASS_ORDER.map((classCode) => [classCode, {
    total: skills.filter((skill) => skill.playerSkill && skill.classCode === classCode).length,
    newPd2PlayerSkills: skills.filter((skill) => skill.classCode === classCode && skill.newPd2PlayerSkill).length,
    bkvinceOnlyPlayerSkills: skills.filter((skill) => skill.classCode === classCode && skill.bkvinceOnlyPlayerSkill).length,
    collisions: skills.filter((skill) => skill.classCode === classCode && skill.collisionIds.length).length,
  }]));
  return {
    physicalRows: Object.fromEntries(SOURCE_ORDER.map((source) => [source, nodes.filter((node) => node.source === source).length])),
    namedRows: Object.fromEntries(SOURCE_ORDER.map((source) => [source, nodes.filter((node) => node.source === source && node.name).length])),
    semanticSkills: skills.length,
    playerSkills: Object.fromEntries(SOURCE_ORDER.map((source) => [source, skills.filter((skill) => (
      skill.playerSkill && skill.nodeIds[source]
    )).length])),
    charclassRows: Object.fromEntries(SOURCE_ORDER.map((source) => [source, nodes.filter((node) => (
      node.source === source && node.classCode
    )).length])),
    byClass,
    newPd2PlayerSkills: skills.filter((skill) => skill.newPd2PlayerSkill).length,
    bkvinceOnlyPlayerSkills: skills.filter((skill) => skill.bkvinceOnlyPlayerSkill).length,
    mappingAmbiguities: 0,
    collisions: collisions.length,
    nextAppendOrdinal: sources.documents.bkvince['skills.txt'].table.rows.length,
    historicalBaseline: {
      bkvinceRows: 449,
      collisions: 108,
      analyticalAuditHash: sources.sourceHashes.analyticalAudit,
    },
    currentBaseline: {
      bkvinceRows: sources.documents.bkvince['skills.txt'].table.rows.length,
      collisions: collisions.length,
    },
    baselineChangeExplanation: {
      rowDelta: sources.documents.bkvince['skills.txt'].table.rows.length - 449,
      collisionDelta: collisions.length - 108,
      addedTechnicalRows: [
        { ordinal: 449, pd2: 'Iceboss Blizzard', bkvince: 'BKV BloodRaven Immo' },
        { ordinal: 450, pd2: 'Lightning Strike Cowboss', bkvince: 'BKV Baal Lowres' },
      ],
      statement: 'HEAD adds two BKVince classless/monster rows at ordinals 449 and 450, creating two additional technical collisions; the historical 449/108 baseline is preserved as provenance only.',
    },
    consumerCounts: {
      exactTextual: skills.reduce((total, skill) => total + skill.consumers.length, 0),
      ordinalCoverage: {
        status: 'NATIVE_UNPROVEN',
        reason: 'Generic numeric Param matching is forbidden; only table/function-specific ordinal evidence may close this gate.',
      },
    },
    documentationCounts: {
      DOCUMENTED: skills.filter((skill) => skill.documentation.some((item) => item.status === 'DOCUMENTED')).length,
      TABLE_ONLY: skills.filter((skill) => skill.documentation.some((item) => item.status === 'TABLE_ONLY')).length,
      UNMAPPED: skills.filter((skill) => !skill.documentation.length || skill.documentation.some((item) => item.status === 'UNMAPPED')).length,
    },
    allRowsRepresentedOnce: SOURCE_ORDER.every((source) => (
      nodes.filter((node) => node.source === source).length === sources.documents[source]['skills.txt'].table.rows.length
    )),
  };
}

export function buildOracleData(sources) {
  const builtNodes = buildNodes(sources);
  const semantic = semanticGroups(builtNodes.nodes);
  const collisionData = collisionGraph(builtNodes.nodes);
  const skills = buildSkills(semantic.groups, collisionData, sources);
  const navigation = buildNavigation(skills);
  const coverage = coverageFor(builtNodes.nodes, skills, collisionData.collisions, sources);
  const policyHashes = {
    contract: FROZEN_CONTRACT_HASH,
    governedRelations: stableHash(GOVERNED_RELATIONS),
    playerCandidateClassification: stableHash({
      policy: 'DATA_DRIVEN_VALID_PLAYER_TREE_PLACEMENT_WITH_EXPLICIT_AUDIT_OVERRIDES',
      playerClassCodes: PLAYER_CLASS_CODES,
      technicalNamePattern: TECHNICAL_NAME_PATTERN.source,
      auditOverrides: [...AUDIT_PLAYER_OVERRIDES.entries()],
      governedRelations: GOVERNED_RELATIONS,
    }),
    canonicalDisplayAliases: stableHash(CANONICAL_DISPLAY_ALIASES),
    treeLabels: stableHash(CLASS_TREE_LABELS),
    componentRouting: stableHash(Object.fromEntries(Object.entries(COMPONENT_HEADER_PATTERNS).map(([key, value]) => [key, value.source]))),
    wikiPin: stableHash({
      title: 'Skill Changes',
      revision: 23785,
      date: '2026-06-17',
      url: 'https://wiki.projectdiablo2.com/w/index.php?title=Skill_Changes&oldid=23785',
      statusPolicy: 'TABLE_ONLY_UNLESS_EXACT_CLAIM_MAP',
    }),
  };
  sources.sourceManifest.documentation = {
    title: 'Skill Changes',
    revision: 23785,
    date: '2026-06-17',
    url: 'https://wiki.projectdiablo2.com/w/index.php?title=Skill_Changes&oldid=23785',
    contentFetched: false,
    statusPolicy: 'TABLE_ONLY_UNLESS_EXACT_CLAIM_MAP',
    policyHash: policyHashes.wikiPin,
  };
  const comparisonPayload = {
    schemaVersion: ORACLE_SCHEMA_VERSION,
    reviewId: REVIEW_ID,
    frozenContractHash: FROZEN_CONTRACT_HASH,
    sourceHashes: sources.sourceHashes,
    policyHashes,
    levels: LEVELS,
    nodes: builtNodes.nodes,
    skills,
    collisions: collisionData.collisions,
    coverage,
  };
  return {
    schemaVersion: ORACLE_SCHEMA_VERSION,
    reviewId: REVIEW_ID,
    productName: 'PD2 Skills Merge Workbench',
    state: 'REVIEW_ONLY_GAMEPLAY_APPLICATION_FORBIDDEN',
    frozenContractHash: FROZEN_CONTRACT_HASH,
    comparisonHash: stableHash(comparisonPayload),
    sourceManifest: sources.sourceManifest,
    sourceHashes: sources.sourceHashes,
    policyHashes,
    levels: LEVELS,
    enums: {
      mappingTypes: MAPPING_TYPES,
      proofStatuses: PROOF_STATUSES,
      portabilityCategories: PORTABILITY_CATEGORIES,
      classes: CLASS_ORDER,
      protectedFieldRules: PROTECTED_FIELD_RULES,
      globalDecisions: GLOBAL_DECISIONS,
      componentDecisions: COMPONENT_DECISIONS,
      newSkillLineDecisions: NEW_SKILL_LINE_DECISIONS,
      implementationStatuses: IMPLEMENTATION_STATUSES,
      behaviorGroups: BEHAVIOR_GROUPS,
    },
    coverage,
    navigation,
    nodes: builtNodes.nodes,
    skills,
    collisions: collisionData.collisions,
    documentation: {
      source: {
        title: 'Skill Changes', revision: 23785, date: '2026-06-17',
        url: 'https://wiki.projectdiablo2.com/w/index.php?title=Skill_Changes&oldid=23785',
      },
      rule: 'Wiki claims are documentary only; TABLE_ONLY and native proof status are derived independently from governed tables.',
    },
  };
}

export function generateOracleData(roots = DEFAULT_SOURCE_ROOTS) {
  return buildOracleData(loadWorkbenchSources(roots));
}

// Friendly aliases for callers that use the generator terminology from the
// frozen interface. None of these functions writes an artifact.
export const loadSources = loadWorkbenchSources;
export const buildOracleModel = buildOracleData;
export const generateOracleModel = generateOracleData;
