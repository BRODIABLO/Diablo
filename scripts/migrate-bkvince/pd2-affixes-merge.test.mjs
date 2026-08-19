import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';

import {
  assertPinnedOutput,
  buildAffixDependencyAuditContext,
  canonicalPropertySignature,
  chargedSkillSerializationAudit,
  decodeChargedSkillFormula,
  encodedBitRange,
  headerIndexes,
  itemTypeReaches,
  loadTable,
  parseOrdinalSpec,
  propertyFunctionValues,
  REQUIRED_PD2_AFFIX_SOURCE_TABLES,
  resolvePd2AffixSourceRoot,
  rowItemStatAudit,
  storedValueRange,
  transactionalWriteFiles,
} from './pd2-affixes-merge.mjs';
import {
  buildDistributionReport,
  canReproduceDistribution,
} from './pd2-affixes-distribution.mjs';

const repoRoot = path.resolve(import.meta.dirname, '..', '..');
const targetRoot = path.join(repoRoot, 'data-BKVince', 'BKVince.mpq', 'data', 'global', 'excel');
let sharedAuditContext;

function auditContext() {
  sharedAuditContext ??= buildAffixDependencyAuditContext(resolvePd2AffixSourceRoot(), targetRoot);
  return sharedAuditContext;
}

function auditSyntheticProperty(code, parameter, minimum, maximum) {
  const context = auditContext();
  const table = {
    headers: ['mod1code', 'mod1param', 'mod1min', 'mod1max', 'mod2code', 'mod2param', 'mod2min', 'mod2max', 'mod3code', 'mod3param', 'mod3min', 'mod3max'],
    rows: [[code, String(parameter ?? ''), String(minimum ?? ''), String(maximum ?? ''), '', '', '', '', '', '', '', '']],
  };
  return rowItemStatAudit(
    table.rows[0],
    headerIndexes(table),
    { source: context.properties.target },
    {
      source: context.itemStats.target,
      target: context.itemStats.target,
      sourceIndexes: context.itemStats.targetIndexes,
      targetIndexes: context.itemStats.targetIndexes,
    },
    `synthetic ${code}`,
    { skills: context.targetSkills },
  );
}

test('PD2 affix source resolution honors explicit, environment, official and mirror precedence', () => {
  const temporaryRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'pd2-affix-source-resolution-'));
  const makeRoot = (name) => {
    const root = path.join(temporaryRoot, name);
    fs.mkdirSync(root, { recursive: true });
    for (const table of REQUIRED_PD2_AFFIX_SOURCE_TABLES) fs.writeFileSync(path.join(root, table), 'fixture');
    return root;
  };
  try {
    const explicit = makeRoot('explicit');
    const environment = makeRoot('environment');
    const repository = path.join(temporaryRoot, 'repository');
    const official = path.join(repository, 'analysis-cache', 'pd2-affixes-merge', 'official-s13');
    fs.mkdirSync(official, { recursive: true });
    for (const table of REQUIRED_PD2_AFFIX_SOURCE_TABLES) fs.writeFileSync(path.join(official, table), 'fixture');
    assert.equal(resolvePd2AffixSourceRoot([`--source-root=${explicit}`], {
      environment: { PD2_AFFIX_SOURCE_ROOT: environment }, repositoryRoot: repository,
    }), explicit);
    assert.equal(resolvePd2AffixSourceRoot([], {
      environment: { PD2_AFFIX_SOURCE_ROOT: environment }, repositoryRoot: repository,
    }), environment);
    assert.equal(resolvePd2AffixSourceRoot([], { environment: {}, repositoryRoot: repository }), official);
    fs.rmSync(official, { recursive: true, force: true });
    const historical = path.join(
      temporaryRoot,
      'PD2 Single PLayer',
      'PD2-Single-Player-Plus-mod-main',
      'data',
      'global',
      'excel',
    );
    fs.mkdirSync(historical, { recursive: true });
    for (const table of REQUIRED_PD2_AFFIX_SOURCE_TABLES) fs.writeFileSync(path.join(historical, table), 'fixture');
    assert.equal(resolvePd2AffixSourceRoot([], { environment: {}, repositoryRoot: repository }), historical);
    assert.throws(() => resolvePd2AffixSourceRoot([], {
      environment: {}, repositoryRoot: path.join(temporaryRoot, 'isolated', 'repository'),
    }), /PD2 affix source unavailable/);
  } finally {
    assert(temporaryRoot.startsWith(path.resolve(os.tmpdir())));
    fs.rmSync(temporaryRoot, { recursive: true, force: true });
  }
});

test('the governed public mirror normalizes only New Orbs and retains official dependency identities', () => {
  const mirrorRoot = process.env.PD2_AFFIX_SOURCE_ROOT
    ? path.resolve(process.env.PD2_AFFIX_SOURCE_ROOT)
    : path.resolve(
      repoRoot,
      '..',
      'PD2 Single PLayer',
      'PD2-Single-Player-Plus-mod-main',
      'data',
      'global',
      'excel',
    );
  assert(fs.existsSync(mirrorRoot), `governed public mirror is unavailable: ${mirrorRoot}`);
  const targetRoot = path.join(repoRoot, 'data-BKVince', 'BKVince.mpq', 'data', 'global', 'excel');
  const catalog = JSON.parse(fs.readFileSync(path.join(repoRoot, 'Mission', 'pd2-affixes-merge.catalog.json'), 'utf8'));
  const context = buildAffixDependencyAuditContext(mirrorRoot, targetRoot);
  assert.equal(context.sourceItemTypes.has('norb'), false);
  for (const table of ['properties.txt', 'itemtypes.txt', 'itemstatcost.txt', 'skills.txt']) {
    assert.equal(context.dependencyHashes.source[table], catalog.source.tables[table].officialSha256);
  }
});

test('ordinal ranges preserve the explicit deterministic order', () => {
  assert.deepEqual(parseOrdinalSpec('1-3,7,10-11'), [1, 2, 3, 7, 10, 11]);
  assert.throws(() => parseOrdinalSpec('3-1'), /Descending/);
  assert.throws(() => parseOrdinalSpec('1,1'), /Duplicate/);
});

test('headers are resolved case-insensitively without accepting ambiguity', () => {
  const indexes = headerIndexes({ headers: ['Name', 'mod1code'], rows: [] });
  assert.equal(indexes.get('name'), 0);
  assert.equal(indexes.get('mod1code'), 1);
  assert.throws(
    () => headerIndexes({ headers: ['Code', 'code'], rows: [] }),
    /Duplicate case-insensitive header/,
  );
});

test('Property compatibility fingerprint preserves tuple order and raw bytes', () => {
  const headers = [];
  for (let slot = 1; slot <= 7; slot += 1) {
    headers.push(`func${slot}`, `stat${slot}`, `set${slot}`, `val${slot}`);
  }
  const row = headers.map((header) => `${header}-value`);
  const signature = canonicalPropertySignature(row, headerIndexes({ headers, rows: [row] }));
  assert.equal(signature.length, 7);
  assert.deepEqual(signature[0], [
    'func1-value',
    'stat1-value',
    'set1-value',
    'val1-value',
  ]);
  assert.deepEqual(signature[6], [
    'func7-value',
    'stat7-value',
    'set7-value',
    'val7-value',
  ]);
});

test('map exclusion follows transitive ItemTypes equivalence ancestry', () => {
  const itemTypes = new Map([
    ['map', { code: 'map', equiv1: '', equiv2: '' }],
    ['t1m', { code: 't1m', equiv1: 'map', equiv2: '' }],
    ['custom-map', { code: 'custom-map', equiv1: 't1m', equiv2: '' }],
    ['ring', { code: 'ring', equiv1: 'jewelry', equiv2: '' }],
    ['jewelry', { code: 'jewelry', equiv1: '', equiv2: '' }],
  ]);
  assert.equal(itemTypeReaches(itemTypes, 'custom-map', 'map'), true);
  assert.equal(itemTypeReaches(itemTypes, 'ring', 'map'), false);
});

test('ItemStatCost bounds use target Save Bits/Add and property value sources', () => {
  const table = {
    headers: ['Stat', 'Save Bits', 'Save Add', 'Save Param Bits'],
    rows: [['poisonmaxdam', '10', '0', '']],
  };
  const range = storedValueRange(
    { rawKey: 'poisonmaxdam', row: table.rows[0] },
    headerIndexes(table),
  );
  assert.deepEqual(range, { minimum: 0, maximum: 1023, bits: 10, add: 0 });
  assert.deepEqual(propertyFunctionValues('15', 626, 800, 150, null), [626]);
  assert.deepEqual(propertyFunctionValues('16', 626, 800, 150, null), [800]);
  assert.deepEqual(propertyFunctionValues('17', 626, 800, 150, null), [150]);
});

test('func19 charged formulas decode vanilla negative min/max values instead of storing them raw', () => {
  const targetSuffixes = loadTable(targetRoot, 'magicsuffix.txt').table;
  const vanillaSuffixes = loadTable(
    path.join(repoRoot, 'data-vanilla3.2', 'data', 'data', 'global', 'excel'),
    'magicsuffix.txt',
  ).table;
  const indexes = headerIndexes(targetSuffixes);
  const vanillaIndexes = headerIndexes(vanillaSuffixes);
  const context = auditContext();
  const auditRow = (row, rowIndexes, label) => rowItemStatAudit(
    row,
    rowIndexes,
    { source: context.properties.target },
    {
      source: context.itemStats.target,
      target: context.itemStats.target,
      sourceIndexes: context.itemStats.targetIndexes,
      targetIndexes: context.itemStats.targetIndexes,
    },
    label,
    { skills: context.targetSkills },
  );

  assert.equal(auditRow(targetSuffixes.rows[532], indexes, 'vanilla Teleportation').ok, true);
  assert.equal(auditRow(vanillaSuffixes.rows[569], vanillaIndexes, 'vanilla Iron Maiden').ok, true);

  for (const [minimum, maximum] of [[-60, -10], [-20, -4]]) {
    const audit = chargedSkillSerializationAudit(minimum, maximum, {
      requiredLevel: 1,
      maximumLevel: 20,
    });
    assert.equal(audit.ok, true, `${minimum}/${maximum}: ${audit.reasons.join(', ')}`);
    assert(audit.outcomes.every(({ level, charges }) => level >= 1 && level <= 63 && charges >= 1 && charges <= 255));
  }
  assert.deepEqual(
    decodeChargedSkillFormula({ minimum: -20, maximum: -4, itemLevel: 99, requiredLevel: 12, maximumLevel: 20 }),
    { level: 4, charges: 30 },
  );
});

test('class skill parameters 0..6 use the unsigned three-bit selector domain', () => {
  assert.deepEqual(encodedBitRange(3, false), { minimum: 0, maximum: 7 });
  const classes = [
    ['ama', 0],
    ['sor', 1],
    ['nec', 2],
    ['pal', 3],
    ['bar', 4],
    ['dru', 5],
    ['ass', 6],
  ];
  for (const [property, classId] of classes) {
    const audit = auditSyntheticProperty(property, null, 1, 1);
    assert.equal(audit.ok, true, `${property}=${classId}: ${audit.reasons.join(', ')}`);
  }
  assert.equal(auditSyntheticProperty('bar', null, 1, 1).ok, true);
  assert.equal(auditSyntheticProperty('dru', null, 1, 1).ok, true);
  assert.equal(auditSyntheticProperty('ass', null, 1, 1).ok, true);
});

test('howl accepts its governed 0..128 scaled domain without changing Wailing', () => {
  assert.equal(auditSyntheticProperty('howl', null, 128, 128).ok, true);
  const invalid = auditSyntheticProperty('howl', null, 129, 129);
  assert.equal(invalid.ok, false);
  assert(invalid.reasons.some((reason) => /outside 0\.\.128/.test(reason)));
});

test('apply pins fail closed and cover predicted table, projection and localization hashes', () => {
  const hashes = Object.fromEntries(
    ['selection', 'table', 'identity', 'projection', 'modern', 'legacy']
      .map((name, index) => [name, String(index + 1).repeat(64)]),
  );
  const catalog = {
    targetBaseline: {
      'magicprefix.txt': {
        physicalRows: 10,
        compiledRows: 9,
        nextCompiledId: 10,
        identitySha256: hashes.identity,
      },
    },
    expected: {
      selectionSha256: hashes.selection,
      retunes: {
        'magicprefix.txt': { rows: 1, cells: 2, byColumn: { level: 2 } },
      },
      appends: {
        'magicprefix.txt': {
          rows: 2,
          firstCompiledId: 10,
          projectionSha256: hashes.projection,
        },
      },
      localization: {
        uniqueSelectedKeys: 3,
        newModernKeys: 1,
        newLegacyKeys: 2,
        legacyOnlyGaps: ['existing-modern-key'],
      },
      final: {
        'magicprefix.txt': hashes.table,
        modernLocalization: hashes.modern,
        legacyLocalization: hashes.legacy,
      },
    },
  };
  const summary = {
    tables: {
      'magicprefix.txt': {
        sha256: hashes.table,
        physicalRows: 12,
        compiledRows: 11,
        identitySha256: hashes.identity,
        retuneRows: 1,
        retuneCells: 2,
        retunesByColumn: { level: 2 },
        appendedRows: 2,
        appendedProjectionSha256: hashes.projection,
      },
    },
    localization: {
      uniqueSelectedKeys: 3,
      modernEntries: 1,
      legacyEntries: 2,
      modernSha256: hashes.modern,
      legacySha256: hashes.legacy,
      legacyOnlyGaps: ['existing-modern-key'],
    },
  };

  assert.doesNotThrow(() => assertPinnedOutput(catalog, hashes.selection, summary));
  catalog.expected.selectionSha256 = null;
  assert.throws(
    () => assertPinnedOutput(catalog, hashes.selection, summary),
    /expected a pinned uppercase SHA-256/,
  );
  catalog.expected.selectionSha256 = hashes.selection;
  catalog.expected.appends['magicprefix.txt'].projectionSha256 = null;
  assert.throws(
    () => assertPinnedOutput(catalog, hashes.selection, summary),
    /append projection: expected a pinned uppercase SHA-256/,
  );
});

test('transactional writes restore all original bytes after a later write fails', () => {
  const temporaryRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'pd2-affixes-transaction-'));
  const files = Array.from({ length: 4 }, (_, index) => path.join(temporaryRoot, `${index}.dat`));
  try {
    files.forEach((filePath, index) => fs.writeFileSync(filePath, `original-${index}`));
    assert.throws(
      () => transactionalWriteFiles(files.map((filePath, index) => ({
        filePath,
        write() {
          fs.writeFileSync(filePath, `changed-${index}`);
          if (index === 2) throw new Error('injected third-write failure');
        },
      }))),
      /all original bytes were restored: injected third-write failure/,
    );
    files.forEach((filePath, index) => {
      assert.equal(fs.readFileSync(filePath, 'utf8'), `original-${index}`);
    });
  } finally {
    assert(temporaryRoot.startsWith(path.resolve(os.tmpdir())));
    fs.rmSync(temporaryRoot, { recursive: true, force: true });
  }
});

test('governed Magic and Rare distributions reproduce the pinned AFM-01/02 report', () => {
  const catalogPath = path.join(repoRoot, 'Mission', 'pd2-affixes-merge.catalog.json');
  const reportPath = path.join(repoRoot, 'Mission', 'pd2-affixes-merge.distribution.json');
  const catalog = JSON.parse(fs.readFileSync(catalogPath, 'utf8'));
  const committedRaw = fs.readFileSync(reportPath, 'utf8');
  const report = canReproduceDistribution(catalog)
    ? buildDistributionReport(catalog)
    : JSON.parse(committedRaw);
  const raw = `${JSON.stringify(report, null, 2)}\n`;
  if (canReproduceDistribution(catalog)) assert.equal(committedRaw, raw);
  assert.equal(
    crypto.createHash('sha256').update(raw).digest('hex').toUpperCase(),
    catalog.simulation.reportSha256,
  );
  assert.deepEqual(report.levels, [1, 45, 65, 85, 99]);
  assert.equal(report.summary.concreteFamilyCount, 64);
  assert.equal(report.summary.touchedFamilyCount, 43);
  assert.equal(report.summary.untouchedFamilyCount, 21);
  assert.equal(report.summary.changedPoolCount, 698);
  assert.equal(report.summary.totalSampleDraws, 34_900_000);
  assert(report.summary.maximumSamplingError <= catalog.simulation.maximumSamplingError);
  assert.equal(report.affixChanges.filter(({ disposition }) => disposition === 'retuned').length, 71);
  assert.equal(report.affixChanges.filter(({ disposition }) => disposition === 'added').length, 200);
  assert.equal(report.affixChanges.filter(({ disposition }) => disposition === 'removed').length, 0);
});
