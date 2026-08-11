import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

import {
  auditAffixProjection,
  buildAffixDependencyAuditContext,
  parseOrdinalSpec,
  resolvePd2BaseSourceRoot,
  resolvePd2AffixSourceRoot,
} from './pd2-affixes-merge.mjs';
import {
  FIELD_DECISIONS,
  NEW_AFFIX_CATEGORIES,
  NEW_AFFIX_LINE_DECISIONS,
  REVIEW_REQUIRED_CATEGORIES,
  applyLineFieldAction,
  effectiveFieldChoice,
  entryReviewState,
  fieldChoiceComplete,
  isNewAffix,
} from './pd2-affixes-decision-rules.mjs';
import { buildHighestHtml, buildReviewHtml } from './pd2-affixes-review-ui.mjs';

const require = createRequire(import.meta.url);
const { ENCODING, parseTable, serializeTable } = require('../build-data/tsv.js');
export const repoRoot = fileURLToPath(new URL('../../', import.meta.url));
const catalogPath = path.join(repoRoot, 'Mission', 'pd2-affixes-merge.catalog.json');
const outputJson = path.join(repoRoot, 'Mission', 'pd2-affixes-review.json');
const outputHtml = path.join(repoRoot, 'Mission', 'pd2-affixes-review.html');
const highestJson = path.join(repoRoot, 'Mission', 'pd2-affixes-highest-level.json');
const highestHtml = path.join(repoRoot, 'Mission', 'pd2-affixes-highest-level.html');
const documentationMapPath = path.join(repoRoot, 'Mission', 'pd2-affixes-documentation-map.json');
const targetRoot = path.join(repoRoot, 'data-BKVince', 'BKVince.mpq', 'data', 'global', 'excel');
const vanillaRoot = path.join(repoRoot, 'data-vanilla3.2', 'data', 'data', 'global', 'excel');

export const TABLES = {
  'magicprefix.txt': { label: 'Préfixe', mapped: 670, targetRow: (row) => row },
  'magicsuffix.txt': { label: 'Suffixe', mapped: 748, targetRow: (row) => (row <= 662 ? row : row + 7) },
  'automagic.txt': { label: 'AutoMagic', mapped: 36, targetRow: (row) => row },
};
const WIKI_URL = 'https://wiki.projectdiablo2.com/wiki/Item_Affixes';

function assert(condition, message) { if (!condition) throw new Error(message); }
export function sha256(value) { return crypto.createHash('sha256').update(value).digest('hex').toUpperCase(); }
function readJson(filePath) { return JSON.parse(fs.readFileSync(filePath, 'utf8').replace(/^\uFEFF/, '')); }
function findFile(root, wanted) {
  const matches = fs.readdirSync(root).filter((name) => name.toLowerCase() === wanted.toLowerCase());
  assert(matches.length === 1, `${root}: expected exactly one ${wanted}`);
  return path.join(root, matches[0]);
}
function loadTable(root, name) {
  const filePath = findFile(root, name);
  const raw = fs.readFileSync(filePath, ENCODING);
  const table = parseTable(filePath);
  assert(serializeTable(table) === raw, `${name}: non byte-exact TSV round-trip`);
  const indexes = new Map(table.headers.map((header, index) => [header.toLowerCase(), index]));
  return { filePath, raw, table, indexes, sha256: sha256(Buffer.from(raw, ENCODING)) };
}
function value(loaded, row, header) {
  if (row === null || row === undefined) return null;
  const index = loaded.indexes.get(header.toLowerCase());
  return index === undefined ? null : (loaded.table.rows[row]?.[index] ?? '');
}
function rowName(loaded, row) { return value(loaded, row, 'Name') ?? ''; }
function isRealRow(loaded, row) { const name = rowName(loaded, row); return name !== '' && name !== 'Expansion'; }
function normalize(header, cell) { return header.toLowerCase() === 'multiply' ? (cell || '0') : cell; }
function rowObject(loaded, row, headers) {
  if (row === null || row === undefined) return null;
  return Object.fromEntries(headers.map((header) => [header, value(loaded, row, header) ?? '']));
}
function pairDiff(headers, left, right, leftKey, rightKey) {
  if (!left && !right) return [];
  if (!left || !right) {
    return [{ field: '$rowPresence', [leftKey]: left ? 'PRESENT' : 'MISSING', [rightKey]: right ? 'PRESENT' : 'MISSING' }];
  }
  return headers.filter((header) => normalize(header, left[header]) !== normalize(header, right[header]))
    .map((field) => ({ field, [leftKey]: left[field], [rightKey]: right[field] }));
}
function threeWayDiff(headers, vanilla, bkvince, pd2) {
  return headers.filter((header) => {
    const values = [vanilla?.[header] ?? null, bkvince?.[header] ?? null, pd2?.[header] ?? null];
    const normalized = values.map((cell) => normalize(header, cell));
    return !(normalized[0] === normalized[1] && normalized[1] === normalized[2]);
  }).map((field) => {
    const protectedField = field.toLowerCase() === 'maxlevel' && Boolean(bkvince?.[field]);
    return {
      field,
      vanilla: vanilla?.[field] ?? null,
      bkvince: bkvince?.[field] ?? null,
      pd2: pd2?.[field] ?? null,
      protected: protectedField,
      defaultDecision: protectedField || normalize(field, bkvince?.[field] ?? null) === normalize(field, pd2?.[field] ?? null) ? 'KEEP_BKVINCE' : null,
    };
  });
}
function effect(row) {
  if (!row) return '—';
  const parts = [];
  for (let slot = 1; slot <= 3; slot += 1) {
    const code = row[`mod${slot}code`];
    if (!code) continue;
    const param = row[`mod${slot}param`];
    const min = row[`mod${slot}min`];
    const max = row[`mod${slot}max`];
    parts.push(`${code}${param ? `(${param})` : ''}${min || max ? ` ${min || '?'}–${max || min || '?'}` : ''}`);
  }
  return parts.join(' · ') || 'Aucun mod direct';
}
function types(row) {
  if (!row) return { allowed: [], excluded: [] };
  const allowed = [], excluded = [];
  for (let i = 1; i <= 7; i += 1) if (row[`itype${i}`]) allowed.push(row[`itype${i}`]);
  for (let i = 1; i <= 5; i += 1) if (row[`etype${i}`]) excluded.push(row[`etype${i}`]);
  return { allowed, excluded };
}
function blockedRows(expected) {
  const result = new Map();
  for (const [reason, spec] of Object.entries(expected.blocked ?? {})) for (const row of parseOrdinalSpec(spec)) result.set(row, reason);
  return result;
}
function statusFor({ mapped, pd2Deleted, vanilla, bkvince, pd2 }) {
  if (!mapped) return 'PD2_NEW';
  if (pd2Deleted) return 'PD2_DELETED';
  const equal = (a, b) => a && b && Object.keys(a).every((header) => normalize(header, a[header]) === normalize(header, b[header]));
  if (equal(vanilla, bkvince) && equal(bkvince, pd2)) return 'ALL_THREE_IDENTICAL';
  if (equal(vanilla, pd2)) return 'PD2_EQUALS_VANILLA_BKV_DIFFERS';
  if (equal(vanilla, bkvince)) return 'BKV_EQUALS_VANILLA_PD2_DIFFERS';
  if (equal(bkvince, pd2)) return 'BKV_EQUALS_PD2';
  return 'ALL_THREE_DIFFER';
}
const STATUS_LABELS = {
  ALL_THREE_IDENTICAL: 'Vanilla, BKVince et PD2 identiques',
  PD2_EQUALS_VANILLA_BKV_DIFFERS: 'PD2 = vanilla; BKVince diffère',
  BKV_EQUALS_VANILLA_PD2_DIFFERS: 'BKVince = vanilla; PD2 diffère',
  BKV_EQUALS_PD2: 'BKVince = PD2; vanilla diffère',
  ALL_THREE_DIFFER: 'Les trois versions diffèrent',
  PD2_DELETED: 'Supprimé ou désactivé par PD2',
  PD2_NEW: 'Nouveau dans PD2',
  BKV_ONLY: 'Propre à BKVince',
};
function familyFor(table, row, name) {
  if (!row) return { id: `${table}:bkv-only:${name}`, label: name || 'Sans nom' };
  const group = row.group || 'ungrouped';
  const codes = [row.mod1code, row.mod2code, row.mod3code].filter(Boolean).join('+') || 'no-direct-mod';
  const type = [row.itype1, row.itype2].filter(Boolean).join('/') || 'all-items';
  return { id: `${table}:${group}:${codes}:${type}`, label: `Groupe ${group} · ${codes} · ${type}` };
}
function documentationFor(documentationMap, identity, category) {
  const mapped = documentationMap.entries.find((entry) => entry.table === identity.table
    && entry.sourceRow === identity.sourceRow && entry.fingerprint === identity.fingerprint);
  if (mapped) {
    const fragment = mapped.reference.url?.includes('#') ? `#${mapped.reference.url.split('#')[1]}` : '';
    const rules = (mapped.ruleIds ?? []).map((ruleId) => documentationMap.rules?.find((rule) => rule.id === ruleId)).filter(Boolean);
    return {
      coverage: 'DOCUMENTED',
      ...mapped.reference,
      url: `${documentationMap.source.revisionUrl}${fragment}`,
      revisionId: documentationMap.source.revisionId,
      claimIds: mapped.claimIds ?? [],
      ruleIds: mapped.ruleIds ?? [],
      documentedFields: [...new Set(rules.flatMap((rule) => rule.documentedFields ?? []))],
      documentedFacets: [...new Set(rules.flatMap((rule) => rule.facets ?? []))],
      mapping: {
        table: mapped.table,
        sourceRow: mapped.sourceRow,
        fingerprint: mapped.fingerprint,
        name: mapped.name,
        properties: mapped.properties,
        itemTypes: mapped.itemTypes,
      },
    };
  }
  if (['PD2_DELETED', 'PD2_MODIFIED', 'PD2_NEW_PORTABLE', 'PD2_NEW_REVIEW'].includes(category)) {
    return { coverage: 'TABLE_ONLY', url: null, section: null, category: 'Table difference', season: 'unknown', summary: 'Différence prouvée par les tables officielles S13 sans correspondance documentaire occurrence-exacte.' };
  }
  return { coverage: 'UNMAPPED', url: null, section: null, category: 'No documented PD2 change', season: 'unknown', summary: 'Aucun changement PD2 documenté n’est revendiqué pour cette occurrence.' };
}

function assertDependencyPins(dependencyContext, catalog) {
  const dependencyFiles = ['properties.txt', 'itemtypes.txt', 'itemstatcost.txt', 'skills.txt'];
  for (const name of dependencyFiles) {
    assert(
      dependencyContext.dependencyHashes.source[name] === catalog.source.tables[name].officialSha256,
      `${name}: PD2 dependency is not the governed official S13 source`,
    );
    assert(
      dependencyContext.dependencyHashes.bkvince[name] === catalog.targetDependencies[name],
      `${name}: BKVince dependency drift`,
    );
  }
  const localization = dependencyContext.dependencyHashes.bkvince.localization;
  assert(
    localization.modern.baseSha256 === catalog.policy.localization.modernBaselineSha256,
    'modern affix localization baseline drift',
  );
  assert(
    localization.legacy.baseSha256 === catalog.policy.localization.legacyBaselineSha256,
    'legacy affix localization baseline drift',
  );
  assert(
    localization.modern.manifestSha256 === catalog.policy.localization.modernNamespaceManifestSha256,
    'modern localization namespace drift',
  );
  assert(
    localization.legacy.manifestSha256 === catalog.policy.localization.legacyNamespaceManifestSha256,
    'legacy localization namespace drift',
  );
}

export function buildReport(sourceRoot, catalog) {
  const entries = [], sourceHashes = {};
  const documentationMap = readJson(documentationMapPath);
  const dependencyContext = buildAffixDependencyAuditContext(sourceRoot, targetRoot);
  assertDependencyPins(dependencyContext, catalog);
  for (const [tableName, config] of Object.entries(TABLES)) {
    const source = loadTable(sourceRoot, tableName), target = loadTable(targetRoot, tableName), vanillaTable = loadTable(vanillaRoot, tableName);
    assert(target.sha256 === catalog.targetBaseline[tableName].sha256, `${tableName}: BKVince is not at review baseline`);
    assert(vanillaTable.sha256 === catalog.vanillaBaseline[tableName], `${tableName}: vanilla baseline drift`);
    assert(source.sha256 === catalog.source.tables[tableName].mirrorSha256 || source.sha256 === catalog.source.tables[tableName].officialSha256, `${tableName}: PD2 source drift`);
    sourceHashes[tableName] = catalog.source.tables[tableName].officialSha256;
    const headers = target.table.headers.filter((header) => source.indexes.has(header.toLowerCase()) && vanillaTable.indexes.has(header.toLowerCase()));
    const portable = new Set(parseOrdinalSpec(catalog.expected.appends[tableName].sourceRows));
    const blocked = blockedRows(catalog.expected.appends[tableName]);
    const mappedTargets = new Set();
    for (let sourceRow = 0; sourceRow < source.table.rows.length; sourceRow += 1) {
      if (!isRealRow(source, sourceRow)) continue;
      const mapped = sourceRow < config.mapped, targetRow = mapped ? config.targetRow(sourceRow) : null;
      if (mapped) mappedTargets.add(targetRow);
      const vanilla = mapped ? rowObject(vanillaTable, sourceRow, headers) : null;
      const bkvince = mapped ? rowObject(target, targetRow, headers) : null;
      const pd2 = rowObject(source, sourceRow, headers);
      const pd2Deleted = mapped && vanilla.spawnable === '1' && pd2.spawnable !== '1';
      const status = statusFor({ mapped, pd2Deleted, vanilla, bkvince, pd2 });
      const deferred = tableName === 'automagic.txt';
      const category = deferred ? 'AUTOMAGIC_DEFERRED' : mapped
        ? (pd2Deleted ? 'PD2_DELETED' : status === 'ALL_THREE_IDENTICAL' || status === 'PD2_EQUALS_VANILLA_BKV_DIFFERS' ? 'UNCHANGED_BY_PD2' : 'PD2_MODIFIED')
        : (portable.has(sourceRow) ? 'PD2_NEW_PORTABLE' : 'PD2_NEW_REVIEW');
      const changes = threeWayDiff(headers, vanilla, bkvince, pd2).map((diff) => ({
        ...diff,
        defaultDecision: category === 'UNCHANGED_BY_PD2' ? 'KEEP_BKVINCE' : diff.defaultDecision,
      }));
      const id = `${tableName}:${sourceRow}`;
      const name = rowName(source, sourceRow);
      const sourceTypes = types(pd2);
      const fingerprint = sha256(JSON.stringify({ tableName, sourceRow, targetRow, vanilla, bkvince, pd2 }));
      const technicalAudit = !mapped ? auditAffixProjection(dependencyContext, {
        tableName, sourceRow, targetRow, projected: pd2, sourceOriginal: pd2, kind: 'append', catalog,
      }) : null;
      const auditReasons = technicalAudit?.conflicts.map((conflict) => ({
        category: conflict.kind,
        code: conflict.code,
        reason: conflict.reason,
      })) ?? [];
      const catalogReason = blocked.get(sourceRow);
      if (catalogReason && !auditReasons.some((reason) => reason.category === catalogReason)) {
        auditReasons.unshift({ category: catalogReason, code: null, reason: `Governed selection exclusion: ${catalogReason}.` });
      }
      entries.push({
        id, fingerprint,
        table: tableName, tableLabel: config.label, sourceRow, targetRow, name, status, statusLabel: STATUS_LABELS[status], category,
        portable: portable.has(sourceRow), blockedReason: auditReasons[0] ?? null, blockedReasons: auditReasons, deferred,
        family: familyFor(tableName, pd2, name), documentation: documentationFor(documentationMap, { table: tableName, sourceRow, fingerprint }, category),
        rows: { vanilla, bkvince, pd2 }, effects: { vanilla: effect(vanilla), bkvince: effect(bkvince), pd2: effect(pd2) },
        itemTypes: sourceTypes,
        technicalAudit,
        comparisons: {
          pd2VsVanilla: pairDiff(headers, pd2, vanilla, 'pd2', 'vanilla'),
          bkvVsVanilla: pairDiff(headers, bkvince, vanilla, 'bkvince', 'vanilla'),
          bkvVsPd2: pairDiff(headers, bkvince, pd2, 'bkvince', 'pd2'),
        },
        fieldDifferences: changes,
      });
    }
    for (let targetRow = 0; targetRow < target.table.rows.length; targetRow += 1) {
      if (mappedTargets.has(targetRow) || !isRealRow(target, targetRow)) continue;
      const headers = target.table.headers;
      const bkvince = rowObject(target, targetRow, headers), name = rowName(target, targetRow), id = `${tableName}:bkv:${targetRow}`;
      const fingerprint = sha256(JSON.stringify({ tableName, targetRow, bkvince }));
      const differences = threeWayDiff(headers, null, bkvince, null).map((diff) => ({ ...diff, defaultDecision: 'KEEP_BKVINCE' }));
      entries.push({
        id, fingerprint, table: tableName, tableLabel: config.label,
        sourceRow: null, targetRow, name, status: 'BKV_ONLY', statusLabel: STATUS_LABELS.BKV_ONLY, category: tableName === 'automagic.txt' ? 'AUTOMAGIC_DEFERRED' : 'BKV_ONLY', portable: false,
        blockedReason: null, blockedReasons: [], deferred: tableName === 'automagic.txt', family: familyFor(tableName, bkvince, name), documentation: documentationFor(documentationMap, { table: tableName, sourceRow: null, fingerprint }, 'BKV_ONLY'),
        rows: { vanilla: null, bkvince, pd2: null }, effects: { vanilla: '—', bkvince: effect(bkvince), pd2: '—' }, itemTypes: types(bkvince),
        comparisons: {
          pd2VsVanilla: pairDiff(headers, null, null, 'pd2', 'vanilla'),
          bkvVsVanilla: pairDiff(headers, bkvince, null, 'bkvince', 'vanilla'),
          bkvVsPd2: pairDiff(headers, bkvince, null, 'bkvince', 'pd2'),
        },
        fieldDifferences: differences,
      });
    }
  }
  entries.sort((a, b) => a.table.localeCompare(b.table) || a.family.id.localeCompare(b.family.id) || (a.sourceRow ?? 99999) - (b.sourceRow ?? 99999));
  assert(Array.isArray(documentationMap.claims), 'Documentation claims must be an array');
  assert(Array.isArray(documentationMap.rules), 'Documentation rules must be an array');
  const documentationClaimIds = new Set();
  for (const claim of documentationMap.claims) {
    assert(typeof claim.id === 'string' && claim.id.length > 0, 'Documentation claim ID is missing');
    assert(!documentationClaimIds.has(claim.id), `Duplicate documentation claim ${claim.id}`);
    assert(claim.sourceRevisionId === documentationMap.source.revisionId, `Documentation revision drift for claim ${claim.id}`);
    documentationClaimIds.add(claim.id);
  }
  const documentationRuleIds = new Set();
  const documentationRulesById = new Map();
  for (const rule of documentationMap.rules) {
    assert(typeof rule.id === 'string' && rule.id.length > 0, 'Documentation rule ID is missing');
    assert(!documentationRuleIds.has(rule.id), `Duplicate documentation rule ${rule.id}`);
    documentationRuleIds.add(rule.id);
    documentationRulesById.set(rule.id, rule);
    assert(documentationClaimIds.has(rule.claimId), `Unknown claim ${rule.claimId} for documentation rule ${rule.id}`);
    assert(Number.isSafeInteger(rule.expectedOccurrenceCount), `Expected occurrence count missing for documentation rule ${rule.id}`);
    assert(rule.expectedOccurrenceCount === rule.materializedOccurrenceCount, `Materialized occurrence count drift for documentation rule ${rule.id}`);
    assert(/^[A-Fa-f0-9]{64}$/.test(rule.occurrenceSetSha256 ?? ''), `Occurrence-set hash missing for documentation rule ${rule.id}`);
    assert(rule.predicate?.notNameOnly === true, `Name-only matching is not forbidden for documentation rule ${rule.id}`);
    assert(Array.isArray(rule.documentedFields), `Documented fields missing for documentation rule ${rule.id}`);
    assert(Array.isArray(rule.facets), `Documented facets missing for documentation rule ${rule.id}`);
  }
  const documentationKeys = new Set();
  for (const mapped of documentationMap.entries) {
    const key = `${mapped.table}:${mapped.sourceRow}`;
    assert(!documentationKeys.has(key), `Duplicate documentation mapping ${key}`);
    documentationKeys.add(key);
    const entry = entries.find((candidate) => candidate.table === mapped.table && candidate.sourceRow === mapped.sourceRow);
    assert(entry, `Documentation occurrence missing: ${key}`);
    assert(entry.fingerprint === mapped.fingerprint, `Documentation fingerprint drift: ${key}`);
    assert(entry.name === mapped.name, `Documentation name drift: ${key}`);
    const properties = [1, 2, 3].map((slot) => entry.rows.pd2?.[`mod${slot}code`]).filter(Boolean);
    assert(JSON.stringify(properties) === JSON.stringify(mapped.properties), `Documentation properties drift: ${key}`);
    assert(JSON.stringify(entry.itemTypes) === JSON.stringify(mapped.itemTypes), `Documentation ItemTypes drift: ${key}`);
    assert(Array.isArray(mapped.claimIds) && mapped.claimIds.length > 0, `Documentation claims missing: ${key}`);
    assert(Array.isArray(mapped.ruleIds), `Documentation rules missing: ${key}`);
    for (const claimId of mapped.claimIds) assert(documentationClaimIds.has(claimId), `Unknown documentation claim ${claimId}: ${key}`);
    for (const ruleId of mapped.ruleIds) {
      const rule = documentationRulesById.get(ruleId);
      assert(rule, `Unknown documentation rule ${ruleId}: ${key}`);
      assert(mapped.claimIds.includes(rule.claimId), `Rule claim ${rule.claimId} is not referenced: ${key}`);
    }
    assert(mapped.claimIds.includes(mapped.reference.claimId), `Primary claim is not referenced: ${key}`);
    if (mapped.reference.section === 'Affix Changes Compilation') {
      assert(mapped.ruleIds.length > 0, `Compilation mapping has no governed rule: ${key}`);
    }
  }
  for (const rule of documentationMap.rules) {
    const actual = documentationMap.entries.filter((entry) => entry.ruleIds.includes(rule.id)).length;
    assert(actual === rule.materializedOccurrenceCount, `Documentation rule cardinality drift ${rule.id}: ${actual}/${rule.materializedOccurrenceCount}`);
  }
  assert(
    documentationMap.entries.filter((entry) => entry.reference.section === 'Removed Affixes').length === 125,
    'Removed Affixes mapping must contain all 125 proven occurrences',
  );
  const counts = {};
  for (const entry of entries) counts[entry.category] = (counts[entry.category] ?? 0) + 1;
  const core = {
    schemaVersion: 3, reviewId: 'pd2-affixes-review-v3', state: 'review_only_no_import_approved',
    sourceAuthority: catalog.source.authority, sourceHashes, dependencyHashes: dependencyContext.dependencyHashes,
    targetBaselineCommit: '756df5f53109729f16643b36aa459fead4cdbf94',
    targetBaselineHashes: Object.fromEntries(Object.entries(catalog.targetBaseline).map(([key, item]) => [key, item.sha256])),
    protectedFields: ['maxlevel'], fieldDecisionOptions: FIELD_DECISIONS, newAffixLineDecisionOptions: NEW_AFFIX_LINE_DECISIONS,
    reviewRequiredCategories: REVIEW_REQUIRED_CATEGORIES,
    documentationMap: {
      schemaVersion: documentationMap.schemaVersion,
      mapId: documentationMap.mapId,
      sha256: sha256(fs.readFileSync(documentationMapPath)),
      source: documentationMap.source,
      claims: documentationMap.claims ?? [],
      rules: documentationMap.rules ?? [],
    },
    counts,
    entries,
  };
  return { ...core, comparisonHash: sha256(JSON.stringify(core)) };
}

function baseCatalog(typeRoot, baseRoot, label) {
  const itemTypesTable = loadTable(typeRoot, 'itemtypes.txt');
  const typeParents = new Map(itemTypesTable.table.rows.map((_, row) => [value(itemTypesTable, row, 'Code'), [value(itemTypesTable, row, 'Equiv1'), value(itemTypesTable, row, 'Equiv2')].filter(Boolean)]));
  const ancestors = (code, seen = new Set()) => {
    if (!code || seen.has(code)) return seen;
    seen.add(code);
    for (const parent of typeParents.get(code) ?? []) ancestors(parent, seen);
    return seen;
  };
  const bases = [];
  for (const file of ['armor.txt', 'weapons.txt', 'misc.txt']) {
    const table = loadTable(baseRoot, file);
    for (let row = 0; row < table.table.rows.length; row += 1) {
      const code = value(table, row, 'code');
      if (!code || value(table, row, 'spawnable') !== '1') continue;
      const typeCodes = [value(table, row, 'type'), value(table, row, 'type2')].filter(Boolean);
      const lineage = new Set(typeCodes.flatMap((type) => [...ancestors(type, new Set())]));
      bases.push({ file, row, code, qlvl: Number(value(table, row, 'level') || 0), magicLvl: Number(value(table, row, 'magic lvl') || 0), lineage: [...lineage] });
    }
  }
  return { label, typeRoot, baseRoot, bases };
}

export function buildHighestLevelReport(report, sourceRoot) {
  const catalogs = {
    vanilla: baseCatalog(vanillaRoot, vanillaRoot, 'Vanilla D2R 3.2'),
    bkvince: baseCatalog(targetRoot, targetRoot, 'BKVince'),
    pd2: baseCatalog(sourceRoot, resolvePd2BaseSourceRoot(sourceRoot), 'PD2 S13'),
  };
  const alvlFor = (ilvl, qlvl, magicLvl) => {
    const i = Math.max(ilvl, qlvl);
    if (magicLvl > 0) return Math.min(99, i + magicLvl);
    return i < 99 - Math.floor(qlvl / 2) ? i - Math.floor(qlvl / 2) : 2 * i - 99;
  };
  const accessFor = (row, catalog) => {
    const ownTypes = types(row);
    const allowed = new Set(ownTypes.allowed), excluded = new Set(ownTypes.excluded);
    const eligible = catalog.bases.filter((base) => (!allowed.size || base.lineage.some((type) => allowed.has(type))) && !base.lineage.some((type) => excluded.has(type)));
    const wanted = Number(row.level || 0);
    const maximumLevel = Number(row.maxlevel || 0) || null;
    const reachable = eligible.map((base) => {
      let minimumIlvl = null;
      for (let ilvl = 1; ilvl <= 99; ilvl += 1) {
        const affixLevel = alvlFor(ilvl, base.qlvl, base.magicLvl);
        if (affixLevel >= wanted && (maximumLevel === null || affixLevel <= maximumLevel)) { minimumIlvl = ilvl; break; }
      }
      return { file: base.file, code: base.code, qlvl: base.qlvl, magicLvl: base.magicLvl, minimumIlvl, alvlAt99: alvlFor(99, base.qlvl, base.magicLvl) };
    }).sort((a, b) => (a.minimumIlvl ?? 999) - (b.minimumIlvl ?? 999) || a.code.localeCompare(b.code));
    const possible = reachable.filter((base) => base.minimumIlvl !== null);
    return {
      itemTypes: ownTypes, eligibleBaseCount: eligible.length, theoreticallyReachableBaseCount: possible.length,
      minimumRequiredIlvl: possible.length ? Math.min(...possible.map((base) => base.minimumIlvl)) : null,
      representativeBases: reachable.slice(0, 12),
      paths: {
        drops: possible.length ? 'POTENTIALLY_ACCESSIBLE_WHEN_DROP_ILVL_REACHES_BASE_THRESHOLD' : 'NOT_REACHABLE_BY_ALVL_FORMULA',
        crafts: possible.length ? 'POTENTIALLY_ACCESSIBLE_IF_CRAFT_OUTPUT_ILVL_REACHES_THRESHOLD' : 'NOT_REACHABLE_BY_ALVL_FORMULA',
        rerolls: possible.length ? 'RECIPE_DEPENDENT_OUTPUT_ILVL' : 'NOT_REACHABLE_BY_ALVL_FORMULA',
        gambling: possible.length ? 'CHARACTER_LEVEL_AND_BASE_OFFER_DEPENDENT' : 'NOT_REACHABLE_BY_ALVL_FORMULA',
      },
    };
  };
  const entries = report.entries.filter((entry) => Object.values(entry.rows).some((row) => Number(row?.level ?? 0) > 71));
  const analyzeVersion = (entry, version) => {
    const row = entry.rows[version];
    if (!row) return { state: 'MISSING', relevant: false, row: null, accessibility: null };
    const level = Number(row.level || 0), spawnable = row.spawnable === '1', ownTypes = types(row);
    if (!spawnable) return { state: 'NON_SPAWNABLE_INTERNAL', relevant: false, level, spawnable, itemTypes: ownTypes, row, accessibility: null };
    if (level <= 71) return { state: 'BELOW_HIGHEST_LEVEL_SCOPE', relevant: false, level, spawnable, itemTypes: ownTypes, row, accessibility: null };
    const accessibility = accessFor(row, catalogs[version]);
    if (level > 99) return { state: 'UNREACHABLE_LEVEL_SENTINEL_OR_INTERNAL_TIER', relevant: false, level, spawnable, itemTypes: ownTypes, row, accessibility };
    return { state: 'ACTIVE_HIGH_LEVEL', relevant: true, level, spawnable, itemTypes: ownTypes, row, accessibility };
  };
  const analyzedEntries = entries.map((entry) => {
    const versions = Object.fromEntries(['vanilla', 'bkvince', 'pd2'].map((version) => [version, analyzeVersion(entry, version)]));
    const activeProblem = versions.bkvince.relevant && versions.bkvince.accessibility.theoreticallyReachableBaseCount === 0;
    return {
      id: entry.id, fingerprint: entry.fingerprint, table: entry.table, sourceRow: entry.sourceRow, targetRow: entry.targetRow,
      name: entry.name, status: entry.status, category: entry.category, documentation: entry.documentation, versions,
      conclusion: activeProblem ? 'PARTIALLY_RELEVANT_NEEDS_PROOF' : Object.values(versions).some((version) => version.relevant) ? 'PARTIALLY_RELEVANT' : 'INFORMATIONAL_ONLY',
    };
  });
  const vita110 = analyzedEntries.find((entry) => entry.table === 'magicsuffix.txt' && entry.sourceRow === 340);
  return {
    schemaVersion: 3, reportId: 'pd2-affixes-highest-level-v3', comparisonHash: report.comparisonHash,
    scope: 'Affixes with alvl > 71; AutoMagic is reported but remains deferred.',
    formula: {
      normal: 'Let i=max(ilvl,qlvl). If magic_lvl>0: alvl=min(99,i+magic_lvl). Otherwise if i<99-floor(qlvl/2): alvl=i-floor(qlvl/2), else alvl=2*i-99.',
      drops: 'Drop accessibility depends on the dropped base qlvl, monster/item ilvl and eligible item types.',
      crafts: 'Craft output ilvl and the base qlvl determine alvl; ilvl 71 guarantees four craft affixes but does not itself guarantee every alvl>71 affix.',
      rerolls: 'The reroll recipe determines output ilvl; qlvl and magic_lvl still participate in alvl.',
      gambling: 'Gambled ilvl varies around character level; base upgrades and qlvl affect the resulting alvl.',
    },
    conclusion: analyzedEntries.length ? 'PARTIALLY_RELEVANT' : 'INFORMATIONAL_ONLY',
    baseCatalogs: Object.fromEntries(Object.entries(catalogs).map(([version, catalog]) => [version, { label: catalog.label, baseCount: catalog.bases.length }])),
    specialFindings: vita110 ? [{
      id: vita110.id, table: vita110.table, sourceRow: vita110.sourceRow, name: vita110.name,
      spawnable: Object.fromEntries(Object.entries(vita110.versions).map(([version, analysis]) => [version, analysis.spawnable ?? null])),
      status: vita110.status, category: vita110.category,
      itemTypes: Object.fromEntries(Object.entries(vita110.versions).map(([version, analysis]) => [version, analysis.itemTypes ?? null])),
      probableReason: 'The shared alvl 110 large-charm row is likely an unreachable internal/sentinel tier; alvl is capped at 99. This is not evidence of an active BKVince regression.',
      documentation: vita110.documentation,
      conclusion: 'INFORMATIONAL_ONLY_PENDING_HISTORICAL_CONFIRMATION',
    }] : [],
    limitations: [
      'This report proves table eligibility and three-way differences.',
      'It does not claim complete drop/craft/reroll/gambling reachability without a governed acquisition-path simulation for every eligible base.',
      'No gameplay change is authorized by this classification.',
    ],
    entries: analyzedEntries,
  };
}

const esc = (value) => String(value ?? '').replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' })[c]);
function buildHtmlLegacy(report) {
  const embedded = JSON.stringify(report).replace(/</g, '\\u003c');
  return `<!doctype html><html lang="fr"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Comparateur PD2 / BKVince / Vanilla</title>
<style>:root{color-scheme:dark;--bg:#0e1218;--panel:#18202a;--line:#344150;--text:#eef3f8;--muted:#aab5c3;--accent:#edae55;--protect:#633}*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:14px/1.45 system-ui}header{position:sticky;top:0;z-index:5;background:#0e1218f5;padding:16px 22px;border-bottom:1px solid var(--line)}h1{margin:0;font-size:23px}p{margin:4px 0;color:var(--muted)}button,input,select,textarea{background:#222c38;color:var(--text);border:1px solid #526174;border-radius:6px;padding:7px}button{cursor:pointer}.controls,.tabs,.progress{display:flex;gap:8px;flex-wrap:wrap;margin-top:10px}.tabs button.active{border-color:var(--accent);color:var(--accent)}main{padding:16px 22px}.family{margin:0 0 18px;border:1px solid var(--line);border-radius:9px;overflow:hidden}.family>h2{font-size:15px;margin:0;padding:10px 12px;background:#222c38}.entry{padding:12px;border-top:1px solid var(--line)}.entry h3{margin:0 0 6px}.chips{display:flex;gap:6px;flex-wrap:wrap}.chip{border:1px solid var(--line);border-radius:99px;padding:2px 7px;color:var(--muted)}.effects{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin:9px 0}.effects div{background:var(--panel);padding:8px;border-radius:6px}.actions{display:flex;gap:6px;flex-wrap:wrap;margin:8px 0}details{margin-top:7px}table{width:100%;border-collapse:collapse;margin-top:7px}th,td{padding:6px;border:1px solid var(--line);text-align:left;vertical-align:top}.protected{background:#3b2527}.protected strong{color:#ffb9ae}.notes{width:100%;min-height:46px;margin-top:7px}.custom-note{width:180px}.hidden{display:none}.doc{color:#9bd1ff}@media(max-width:800px){.effects{grid-template-columns:1fr}header{position:static}}</style></head><body>
<header><h1>Comparateur d’affixes à trois voies</h1><p><strong>Aucun import n’est approuvé.</strong> Vanilla D2R 3.2, BKVince restauré et PD2 S13 sont affichés côte à côte.</p>
<div class="tabs" id="tabs"></div><div class="controls"><input id="search" placeholder="Nom, effet, famille, type"><select id="table"><option value="">Toutes les tables</option><option>magicprefix.txt</option><option>magicsuffix.txt</option><option>automagic.txt</option></select><label><input type="checkbox" id="incomplete"> décisions incomplètes seulement</label><button id="import">Importer décisions</button><input type="file" id="file" accept="application/json" hidden><button id="export">Exporter décisions</button><button id="reset">Réinitialiser</button><a href="pd2-affixes-highest-level.html">Highest-Level Affixes</a></div><div class="progress" id="progress"></div></header><main id="content"></main>
<script>const report=${embedded};const storage='pd2-affixes-review-decisions-v2';let state=JSON.parse(localStorage.getItem(storage)||'{"entries":{}}');const cats=[['PD2_DELETED','1. Supprimés par PD2'],['PD2_MODIFIED','2. Existants modifiés'],['PD2_NEW_PORTABLE','3. Nouveaux portables'],['PD2_NEW_REVIEW','4. Nouveaux bloqués/à examiner'],['BKV_ONLY','5. Propres à BKVince'],['AUTOMAGIC_DEFERRED','6. AutoMagic différé']];let active='PD2_DELETED';
const E=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));const save=()=>localStorage.setItem(storage,JSON.stringify(state));function item(e){return state.entries[e.id]||(state.entries[e.id]={fingerprint:e.fingerprint,fields:{},notes:''})}function decision(e,d){return item(e).fields[d.field]?.decision||d.defaultDecision||''}function incomplete(e){return e.fieldDifferences.some(d=>!decision(e,d)||decision(e,d)==='DISCUSS'||(decision(e,d)==='CUSTOM'&&!item(e).fields[d.field]?.customValue))}
function apply(e,mode){const x=item(e);for(const d of e.fieldDifferences){if(x.fields[d.field])continue;if(mode==='KEEP'||mode==='DISCUSS')x.fields[d.field]={decision:mode==='KEEP'?'KEEP_BKVINCE':'DISCUSS'};else if(mode==='ADOPT_SAFE')x.fields[d.field]={decision:d.protected?'KEEP_BKVINCE':'ADOPT_PD2'};else if(mode==='ADOPT_ALL'&&!d.protected)x.fields[d.field]={decision:'ADOPT_PD2'}}save();render()}
function render(){document.querySelector('#tabs').innerHTML=cats.map(([id,l])=>'<button data-cat="'+id+'" class="'+(active===id?'active':'')+'">'+l+' ('+(report.counts[id]||0)+')</button>').join('');document.querySelectorAll('[data-cat]').forEach(b=>b.onclick=()=>{active=b.dataset.cat;render()});const q=document.querySelector('#search').value.toLowerCase(),t=document.querySelector('#table').value,inc=document.querySelector('#incomplete').checked;const visible=report.entries.filter(e=>e.category===active&&(!t||e.table===t)&&(!inc||incomplete(e))&&(!q||[e.name,e.family.label,e.effects.pd2,e.effects.bkvince,...e.itemTypes.allowed].join(' ').toLowerCase().includes(q)));const groups=Map.groupBy(visible,e=>e.family.id);document.querySelector('#content').innerHTML=[...groups.values()].map(g=>'<section class="family"><h2>'+E(g[0].family.label)+'</h2>'+g.map(row).join('')+'</section>').join('')||'<p>Aucune occurrence.</p>';bind();const all=report.entries.filter(e=>e.category!=='AUTOMAGIC_DEFERRED'),done=all.filter(e=>!incomplete(e)).length;document.querySelector('#progress').innerHTML='<span>'+done+' / '+all.length+' occurrences complètes ('+Math.round(done*100/Math.max(1,all.length))+'%)</span><span>AutoMagic masqué par défaut et différé</span>'}
function row(e){const x=item(e);const diffs=e.fieldDifferences.map(d=>{const f=x.fields[d.field]||{},v=decision(e,d);return '<tr class="'+(d.protected?'protected':'')+'"><td>'+E(d.field)+(d.protected?' <strong>PROTÉGÉ</strong>':'')+'</td><td>'+E(d.vanilla===''?'vide':d.vanilla)+'</td><td>'+E(d.bkvince===''?'vide':d.bkvince)+'</td><td>'+E(d.pd2===''?'vide':d.pd2)+'</td><td><select data-field="'+E(d.field)+'" data-id="'+E(e.id)+'"><option value="">À décider</option>'+report.fieldDecisionOptions.map(o=>'<option '+(v===o?'selected':'')+'>'+o+'</option>').join('')+'</select> '+(v==='CUSTOM'?'<input class="custom-note" data-custom="'+E(d.field)+'" data-id="'+E(e.id)+'" value="'+E(f.customValue||'')+'" placeholder="valeur personnalisée"><input class="custom-note" data-note="'+E(d.field)+'" data-id="'+E(e.id)+'" value="'+E(f.notes||'')+'" placeholder="note obligatoire">':'')+'</td></tr>'}).join('');const doc=e.documentation;return '<article class="entry"><h3>'+E(e.name)+' <span class="chip">'+E(e.statusLabel)+'</span></h3><div class="chips"><span class="chip">'+E(e.table)+' · PD2 '+E(e.sourceRow??'—')+' · BKV '+E(e.targetRow??'—')+'</span><span class="chip">'+E(doc.coverage)+'</span></div><div class="effects"><div><b>Vanilla</b><br>'+E(e.effects.vanilla)+'</div><div><b>BKVince</b><br>'+E(e.effects.bkvince)+'</div><div><b>PD2 S13</b><br>'+E(e.effects.pd2)+'</div></div><div class="actions"><button data-action="KEEP" data-id="'+E(e.id)+'">Garder toute la ligne BKVince</button><button data-action="ADOPT_ALL" data-id="'+E(e.id)+'">Adopter tous les champs PD2 admissibles</button><button data-action="ADOPT_SAFE" data-id="'+E(e.id)+'">Adopter PD2 sauf champs protégés</button><button data-action="DISCUSS" data-id="'+E(e.id)+'">Tout envoyer à discussion</button></div><details open><summary>'+e.fieldDifferences.length+' champs différents — tout afficher</summary><table><thead><tr><th>Champ</th><th>Vanilla</th><th>BKVince</th><th>PD2</th><th>Décision</th></tr></thead><tbody>'+diffs+'</tbody></table></details><details><summary>Comparaisons bilatérales et lignes complètes</summary><pre>'+E(JSON.stringify({comparisons:e.comparisons,rows:e.rows},null,2))+'</pre></details><p class="doc">'+E(doc.coverage)+' · '+E(doc.section||'tables officielles S13')+' · '+E(doc.summary)+'</p><textarea class="notes" data-row-note="'+E(e.id)+'" placeholder="Notes de ligne">'+E(x.notes||'')+'</textarea></article>'}
function bind(){document.querySelectorAll('[data-action]').forEach(b=>b.onclick=()=>apply(report.entries.find(e=>e.id===b.dataset.id),b.dataset.action));document.querySelectorAll('[data-field]').forEach(s=>s.onchange=()=>{const e=report.entries.find(e=>e.id===s.dataset.id),d=e.fieldDifferences.find(d=>d.field===s.dataset.field),x=item(e);x.fields[d.field]={...(x.fields[d.field]||{}),decision:s.value};if(d.protected&&s.value==='ADOPT_PD2')x.fields[d.field].protectedOverride=true;save();render()});document.querySelectorAll('[data-custom]').forEach(i=>i.onchange=()=>{item(report.entries.find(e=>e.id===i.dataset.id)).fields[i.dataset.custom].customValue=i.value;save()});document.querySelectorAll('[data-note]').forEach(i=>i.onchange=()=>{item(report.entries.find(e=>e.id===i.dataset.id)).fields[i.dataset.note].notes=i.value;save()});document.querySelectorAll('[data-row-note]').forEach(i=>i.onchange=()=>{item(report.entries.find(e=>e.id===i.dataset.rowNote)).notes=i.value;save()})}
['search','table','incomplete'].forEach(id=>document.querySelector('#'+id).oninput=render);document.querySelector('#reset').onclick=()=>{if(confirm('Réinitialiser les décisions locales ?')){state={entries:{}};save();render()}};document.querySelector('#import').onclick=()=>document.querySelector('#file').click();document.querySelector('#file').onchange=async ev=>{try{const p=JSON.parse(await ev.target.files[0].text());if(p.schemaVersion!==2||p.reviewId!==report.reviewId||p.comparisonHash!==report.comparisonHash||p.targetBaselineCommit!==report.targetBaselineCommit||JSON.stringify(p.sourceHashes)!==JSON.stringify(report.sourceHashes))throw Error('hash, source PD2 ou baseline BKVince incompatible');for(const [id,x] of Object.entries(p.entries||{})){const e=report.entries.find(e=>e.id===id);if(!e||e.fingerprint!==x.fingerprint)throw Error('occurrence ou fingerprint incompatible: '+id)}state={entries:p.entries||{}};save();render()}catch(err){alert('Import refusé: '+err.message)}};document.querySelector('#export').onclick=()=>{const payload={schemaVersion:2,reviewId:report.reviewId,comparisonHash:report.comparisonHash,sourceHashes:report.sourceHashes,targetBaselineCommit:report.targetBaselineCommit,exportedAt:new Date().toISOString(),entries:Object.fromEntries(report.entries.map(e=>[e.id,{fingerprint:e.fingerprint,...item(e)}]))};const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([JSON.stringify(payload,null,2)+'\\n'],{type:'application/json'}));a.download='pd2-affixes-decisions-v2.json';a.click();URL.revokeObjectURL(a.href)};render();</script></body></html>\n`;
}
function highestHtmlPageLegacy(report) {
  return `<!doctype html><html lang="fr"><head><meta charset="utf-8"><title>Highest-Level Affixes</title><style>body{font:15px system-ui;max-width:1400px;margin:auto;padding:24px;background:#101318;color:#eee}table{border-collapse:collapse;width:100%}th,td{border:1px solid #455;padding:6px;text-align:left}code{color:#fc9}</style></head><body><h1>Highest-Level Affixes — analyse séparée</h1><p><b>Conclusion : ${esc(report.conclusion)}</b></p><p>${esc(report.formula.normal)}</p><ul>${Object.entries(report.formula).slice(1).map(([k,v])=>`<li><b>${esc(k)}</b> : ${esc(v)}</li>`).join('')}</ul><p>Cette vue calcule le seuil ilvl théorique pour chaque base BKVince éligible. Elle ne prétend pas qu’un monstre, une recette ou un marchand précis produit effectivement ce seuil avant la simulation gouvernée des chemins d’acquisition.</p><table><thead><tr><th>Affixe</th><th>Table</th><th>alvl</th><th>Statut</th><th>Conclusion</th><th>Bases accessibles</th><th>ilvl minimum</th><th>Drops / crafts / rerolls / gamble</th><th>MaxLevel V/B/P</th></tr></thead><tbody>${report.entries.map(e=>`<tr><td>${esc(e.name)}</td><td>${esc(e.table)}</td><td>${esc(e.alvl)}</td><td>${esc(e.status)}</td><td>${esc(e.conclusion)}</td><td>${esc(e.accessibility.theoreticallyReachableBaseCount)} / ${esc(e.accessibility.eligibleBaseCount)}</td><td>${esc(e.accessibility.minimumRequiredIlvl)}</td><td>${esc(Object.values(e.accessibility.paths).join(' · '))}</td><td>${esc(e.maxlevel.vanilla)} / ${esc(e.maxlevel.bkvince)} / ${esc(e.maxlevel.pd2)}</td></tr>`).join('')}</tbody></table></body></html>\n`;
}

export function buildHtml(report) { return buildReviewHtml(report); }
function highestHtmlPage(report) { return buildHighestHtml(report); }

export function run(args = process.argv.slice(2)) {
  const check = args.includes('--check'), sourceRoot = resolvePd2AffixSourceRoot(args), catalog = readJson(catalogPath);
  const report = buildReport(sourceRoot, catalog), highest = buildHighestLevelReport(report, sourceRoot);
  const outputs = new Map([[outputJson, `${JSON.stringify(report, null, 2)}\n`], [outputHtml, buildHtml(report)], [highestJson, `${JSON.stringify(highest, null, 2)}\n`], [highestHtml, highestHtmlPage(highest)]]);
  if (check) {
    for (const [file, raw] of outputs) assert(fs.existsSync(file) && fs.readFileSync(file, 'utf8') === raw, `${path.basename(file)} is stale`);
    assert(catalog.review.schemaVersion === 3 && catalog.review.comparisonHash === report.comparisonHash, 'catalog review identity is stale');
    assert(catalog.review.jsonSha256 === sha256(Buffer.from(outputs.get(outputJson))), 'catalog review JSON hash is stale');
    assert(catalog.review.htmlSha256 === sha256(Buffer.from(outputs.get(outputHtml))), 'catalog review HTML hash is stale');
    assert(catalog.review.highestLevel.jsonSha256 === sha256(Buffer.from(outputs.get(highestJson))), 'catalog highest-level JSON hash is stale');
    assert(catalog.review.highestLevel.htmlSha256 === sha256(Buffer.from(outputs.get(highestHtml))), 'catalog highest-level HTML hash is stale');
    const governedDocumentation = readJson(documentationMapPath);
    const documentationPin = catalog.review.documentationMap;
    const coverageCounts = Object.fromEntries(['DOCUMENTED', 'TABLE_ONLY', 'UNMAPPED'].map((coverage) => [
      coverage,
      report.entries.filter((entry) => entry.documentation.coverage === coverage).length,
    ]));
    const claimsById = new Map(governedDocumentation.claims.map((claim) => [claim.id, claim.section]));
    const sectionOccurrenceCoverage = Object.fromEntries(governedDocumentation.source.sections.map((section) => [
      section,
      governedDocumentation.entries.filter((entry) => entry.claimIds.some((claimId) => claimsById.get(claimId) === section)).length,
    ]));
    assert(documentationPin.schemaVersion === governedDocumentation.schemaVersion, 'catalog documentation schema is stale');
    assert(documentationPin.mapId === governedDocumentation.mapId, 'catalog documentation map identity is stale');
    assert(documentationPin.entries === governedDocumentation.entries.length, 'catalog documentation entry count is stale');
    assert(documentationPin.claims === governedDocumentation.claims.length, 'catalog documentation claim count is stale');
    assert(documentationPin.rules === governedDocumentation.rules.length, 'catalog documentation rule count is stale');
    assert(documentationPin.sha256 === sha256(fs.readFileSync(documentationMapPath)), 'catalog documentation map hash is stale');
    assert(JSON.stringify(documentationPin.coverageCounts) === JSON.stringify(coverageCounts), 'catalog documentation coverage counts are stale');
    assert(JSON.stringify(documentationPin.sectionOccurrenceCoverage) === JSON.stringify(sectionOccurrenceCoverage), 'catalog documentation section coverage is stale');
  }
  else for (const [file, raw] of outputs) fs.writeFileSync(file, raw, 'utf8');
  console.log(JSON.stringify({ mode: check ? 'check' : 'write', entries: report.entries.length, counts: report.counts, comparisonHash: report.comparisonHash, highestLevelEntries: highest.entries.length }, null, 2));
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try { run(); } catch (error) { console.error(`INVALID PD2 Affixes Review: ${error.message}`); process.exitCode = 1; }
}
