import crypto from 'node:crypto';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

const require = createRequire(import.meta.url);
const { ENCODING, parseTable, serializeTable } = require('../build-data/tsv.js');

const repoRoot = fileURLToPath(new URL('../../', import.meta.url));
const catalogPath = path.join(repoRoot, 'Mission', 'pd2-affixes-merge.catalog.json');
const reportPath = path.join(repoRoot, 'Mission', 'pd2-affixes-merge.distribution.json');
const excelPrefix = 'data-BKVince/BKVince.mpq/data/global/excel';
const affixTables = Object.freeze(['magicprefix.txt', 'magicsuffix.txt']);
const baseTables = Object.freeze(['armor.txt', 'weapons.txt', 'misc.txt']);
const qualities = Object.freeze(['magic', 'rare']);

function assert(condition, message) {
  if (!condition) throw new Error(message);
}

function sha256(value) {
  return crypto.createHash('sha256').update(value).digest('hex').toUpperCase();
}

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8').replace(/^\uFEFF/, ''));
}

function indexes(table) {
  const result = new Map();
  table.headers.forEach((header, index) => {
    const key = header.toLowerCase();
    assert(!result.has(key), `Duplicate header ${header}`);
    result.set(key, index);
  });
  return result;
}

function value(row, columns, header) {
  const index = columns.get(header.toLowerCase());
  return index === undefined ? '' : (row[index] ?? '');
}

function numberValue(row, columns, header) {
  const parsed = Number(value(row, columns, header));
  return Number.isFinite(parsed) ? parsed : 0;
}

function readGitTable(commit, name, tempRoot) {
  const relativePath = `${excelPrefix}/${name}`;
  const bytes = execFileSync('git', ['show', `${commit}:${relativePath}`], {
    cwd: repoRoot,
    encoding: 'buffer',
    maxBuffer: 32 * 1024 * 1024,
  });
  const filePath = path.join(tempRoot, `${commit.slice(0, 8)}-${name}`);
  fs.writeFileSync(filePath, bytes);
  const table = parseTable(filePath);
  assert(
    serializeTable(table) === bytes.toString(ENCODING),
    `${commit}:${name}: governed TSV round-trip drift`,
  );
  return { table, columns: indexes(table), sha256: sha256(bytes) };
}

function itemTypeIndex(loaded) {
  const result = new Map();
  for (const row of loaded.table.rows) {
    const code = value(row, loaded.columns, 'code').toLowerCase();
    if (!code) continue;
    assert(!result.has(code), `Duplicate ItemType code ${code}`);
    result.set(code, {
      code,
      label: value(row, loaded.columns, 'itemtype') || code,
      parents: [
        value(row, loaded.columns, 'equiv1').toLowerCase(),
        value(row, loaded.columns, 'equiv2').toLowerCase(),
      ].filter(Boolean),
      classCode: value(row, loaded.columns, 'class').toLowerCase(),
    });
  }
  return result;
}

function ancestryFor(code, itemTypes, visiting = new Set()) {
  if (visiting.has(code)) throw new Error(`ItemType ancestry cycle at ${code}`);
  const entry = itemTypes.get(code);
  if (!entry) return new Set([code]);
  const next = new Set(visiting).add(code);
  const ancestry = new Set([code]);
  for (const parent of entry.parents) {
    for (const ancestor of ancestryFor(parent, itemTypes, next)) ancestry.add(ancestor);
  }
  return ancestry;
}

function concreteFamilies(loadedTables, itemTypes) {
  const result = new Map();
  for (const loaded of loadedTables) {
    for (const row of loaded.table.rows) {
      if (value(row, loaded.columns, 'spawnable') !== '1') continue;
      const code = value(row, loaded.columns, 'type').toLowerCase();
      if (!code || !itemTypes.has(code)) continue;
      const entry = itemTypes.get(code);
      result.set(code, {
        code,
        label: entry.label,
        ancestry: ancestryFor(code, itemTypes),
        classCode: entry.classCode,
      });
    }
  }
  return [...result.values()].sort((left, right) => left.code.localeCompare(right.code));
}

function compiledRows(loaded) {
  const rows = [];
  let id = 1;
  for (let physicalRow = 0; physicalRow < loaded.table.rows.length; physicalRow += 1) {
    const row = loaded.table.rows[physicalRow];
    if (value(row, loaded.columns, 'name').toLowerCase() === 'expansion') continue;
    rows.push({ id, physicalRow, row });
    id += 1;
  }
  return rows;
}

function typeList(row, columns, prefix, slots) {
  return Array.from({ length: slots }, (_, index) => (
    value(row, columns, `${prefix}${index + 1}`).toLowerCase()
  )).filter(Boolean);
}

function mods(row, columns) {
  return Array.from({ length: 3 }, (_, index) => {
    const slot = index + 1;
    const code = value(row, columns, `mod${slot}code`);
    if (!code) return null;
    return {
      code,
      parameter: value(row, columns, `mod${slot}param`),
      minimum: value(row, columns, `mod${slot}min`),
      maximum: value(row, columns, `mod${slot}max`),
    };
  }).filter(Boolean);
}

function eligibleAffixes(loaded, family, level, quality) {
  const result = [];
  for (const { id, physicalRow, row } of compiledRows(loaded)) {
    const name = value(row, loaded.columns, 'name');
    if (!name || value(row, loaded.columns, 'spawnable') !== '1') continue;
    if (quality === 'rare' && value(row, loaded.columns, 'rare') !== '1') continue;
    const minimumLevel = numberValue(row, loaded.columns, 'level');
    const maximumLevel = numberValue(row, loaded.columns, 'maxlevel');
    if (minimumLevel > level || (maximumLevel > 0 && level > maximumLevel)) continue;
    const frequency = numberValue(row, loaded.columns, 'frequency');
    if (frequency <= 0) continue;
    const allowed = typeList(row, loaded.columns, 'itype', 7);
    const excluded = typeList(row, loaded.columns, 'etype', 5);
    if (allowed.length > 0 && !allowed.some((code) => family.ancestry.has(code))) continue;
    if (excluded.some((code) => family.ancestry.has(code))) continue;
    const classSpecific = value(row, loaded.columns, 'classspecific').toLowerCase();
    if (classSpecific && classSpecific !== family.classCode) continue;
    result.push({
      id,
      physicalRow,
      name,
      group: value(row, loaded.columns, 'group') || '0',
      frequency,
      minimumLevel,
      maximumLevel,
      allowed,
      excluded,
      mods: mods(row, loaded.columns),
    });
  }
  return result;
}

function affixRecord(loaded, compiled) {
  const { id, physicalRow, row } = compiled;
  return {
    id,
    physicalRow,
    name: value(row, loaded.columns, 'name'),
    spawnable: value(row, loaded.columns, 'spawnable'),
    rare: value(row, loaded.columns, 'rare'),
    level: value(row, loaded.columns, 'level'),
    maxlevel: value(row, loaded.columns, 'maxlevel'),
    levelreq: value(row, loaded.columns, 'levelreq'),
    classspecific: value(row, loaded.columns, 'classspecific'),
    group: value(row, loaded.columns, 'group'),
    frequency: value(row, loaded.columns, 'frequency'),
    allowed: typeList(row, loaded.columns, 'itype', 7),
    excluded: typeList(row, loaded.columns, 'etype', 5),
    mods: mods(row, loaded.columns),
  };
}

function compareAffixTables(beforeTables, afterTables) {
  const result = [];
  for (const table of affixTables) {
    const before = new Map(compiledRows(beforeTables[table]).map((compiled) => {
      const record = affixRecord(beforeTables[table], compiled);
      return [record.id, record];
    }));
    const after = new Map(compiledRows(afterTables[table]).map((compiled) => {
      const record = affixRecord(afterTables[table], compiled);
      return [record.id, record];
    }));
    for (const id of new Set([...before.keys(), ...after.keys()])) {
      const left = before.get(id) ?? null;
      const right = after.get(id) ?? null;
      if (JSON.stringify(left) === JSON.stringify(right)) continue;
      result.push({
        affixKind: table === 'magicprefix.txt' ? 'prefix' : 'suffix',
        id,
        name: right?.name ?? left?.name,
        disposition: !left ? 'added' : (!right ? 'removed' : 'retuned'),
        before: left,
        after: right,
      });
    }
  }
  return result;
}

function affixIdentity(affix) {
  return `${affix.id}:${affix.name}`;
}

function poolIndex(pool) {
  const totalFrequency = pool.reduce((total, affix) => total + affix.frequency, 0);
  return {
    rows: pool.length,
    totalFrequency,
    groups: new Set(pool.map((affix) => affix.group)).size,
    affixes: new Map(pool.map((affix) => [affixIdentity(affix), {
      ...affix,
      probability: totalFrequency === 0 ? 0 : affix.frequency / totalFrequency,
    }])),
  };
}

function causalAffix(left, right) {
  if (!left || !right) return true;
  return left.frequency !== right.frequency
    || left.group !== right.group
    || JSON.stringify(left.mods) !== JSON.stringify(right.mods);
}

function changedGroupSummaries(before, after) {
  const summarize = (pool) => {
    const groups = new Map();
    for (const affix of pool.affixes.values()) {
      groups.set(affix.group, (groups.get(affix.group) ?? 0) + affix.frequency);
    }
    return groups;
  };
  const left = summarize(before);
  const right = summarize(after);
  const result = [];
  for (const group of new Set([...left.keys(), ...right.keys()])) {
    const beforeFrequency = left.get(group) ?? 0;
    const afterFrequency = right.get(group) ?? 0;
    if (beforeFrequency === afterFrequency) continue;
    result.push({
      group,
      beforeFrequency,
      afterFrequency,
      beforeProbability: before.totalFrequency === 0 ? 0 : beforeFrequency / before.totalFrequency,
      afterProbability: after.totalFrequency === 0 ? 0 : afterFrequency / after.totalFrequency,
    });
  }
  return result.sort((a, b) => Number(a.group) - Number(b.group) || a.group.localeCompare(b.group));
}

function totalVariationDistance(before, after) {
  if (before.totalFrequency === 0 || after.totalFrequency === 0) {
    return before.totalFrequency === after.totalFrequency ? 0 : 1;
  }
  const identities = new Set([...before.affixes.keys(), ...after.affixes.keys()]);
  let absoluteDifference = 0;
  for (const identity of identities) {
    absoluteDifference += Math.abs(
      (after.affixes.get(identity)?.probability ?? 0)
      - (before.affixes.get(identity)?.probability ?? 0),
    );
  }
  return absoluteDifference / 2;
}

function xorshift32(seedText) {
  let state = crypto.createHash('sha256').update(seedText).digest().readUInt32LE(0) || 0x9E3779B9;
  return () => {
    state ^= state << 13;
    state ^= state >>> 17;
    state ^= state << 5;
    return (state >>> 0) / 0x100000000;
  };
}

function sampleProbability(probability, draws, seedText) {
  if (draws === 0) return { draws: 0, expected: probability, observed: null, absoluteError: null };
  const random = xorshift32(seedText);
  let hits = 0;
  for (let draw = 0; draw < draws; draw += 1) {
    if (random() < probability) hits += 1;
  }
  const observed = hits / draws;
  return { draws, expected: probability, observed, absoluteError: Math.abs(observed - probability) };
}

function comparePools(beforePool, afterPool, simulationKey, config) {
  const before = poolIndex(beforePool);
  const after = poolIndex(afterPool);
  const identities = new Set([...before.affixes.keys(), ...after.affixes.keys()]);
  const causalIdentities = [...identities].filter((identity) => (
    causalAffix(before.affixes.get(identity), after.affixes.get(identity))
  ));
  const changed = causalIdentities.length > 0
    || before.rows !== after.rows
    || before.totalFrequency !== after.totalFrequency;
  if (!changed) return null;

  let addedRows = 0;
  let removedRows = 0;
  let changedRows = 0;
  let rangeChanges = 0;
  causalIdentities.forEach((identity) => {
    const left = before.affixes.get(identity);
    const right = after.affixes.get(identity);
    if (!left) addedRows += 1;
    else if (!right) removedRows += 1;
    else changedRows += 1;
    if (left && right && JSON.stringify(left.mods) !== JSON.stringify(right.mods)) rangeChanges += 1;
  });

  const affectedProbability = (pool) => causalIdentities.reduce(
    (total, identity) => total + (pool.affixes.get(identity)?.probability ?? 0),
    0,
  );
  const beforeAffected = affectedProbability(before);
  const afterAffected = affectedProbability(after);
  const draws = config.drawsPerChangedPool;
  const groupDeltas = changedGroupSummaries(before, after);
  return {
    before: { rows: before.rows, totalFrequency: before.totalFrequency, groups: before.groups },
    after: { rows: after.rows, totalFrequency: after.totalFrequency, groups: after.groups },
    delta: {
      rows: after.rows - before.rows,
      totalFrequency: after.totalFrequency - before.totalFrequency,
      groups: after.groups - before.groups,
      addedRows,
      removedRows,
      changedRows,
      rangeChanges,
      totalVariationDistance: totalVariationDistance(before, after),
    },
    affectedProbability: { before: beforeAffected, after: afterAffected },
    groupChanges: {
      count: groupDeltas.length,
      groups: groupDeltas.map(({ group }) => group),
    },
    sampling: {
      before: sampleProbability(beforeAffected, draws, `${config.seed}|${simulationKey}|before`),
      after: sampleProbability(afterAffected, draws, `${config.seed}|${simulationKey}|after`),
    },
  };
}

function loadCommit(commit, tempRoot) {
  const itemTypes = readGitTable(commit, 'itemtypes.txt', tempRoot);
  const typeIndex = itemTypeIndex(itemTypes);
  const bases = baseTables.map((name) => readGitTable(commit, name, tempRoot));
  return {
    itemTypes: typeIndex,
    families: concreteFamilies(bases, typeIndex),
    affixes: Object.fromEntries(affixTables.map((name) => [name, readGitTable(commit, name, tempRoot)])),
    hashes: {
      itemTypes: itemTypes.sha256,
      bases: Object.fromEntries(baseTables.map((name, index) => [name, bases[index].sha256])),
    },
  };
}

function aggregateReport(families, totalFamilyCount, config) {
  const changes = families.flatMap((family) => family.changes.map((change) => ({
    familyCode: family.code,
    familyLabel: family.label,
    ...change,
  })));
  const byCell = new Map();
  let maximumSamplingError = 0;
  let totalSampleDraws = 0;
  for (const change of changes) {
    const key = `${change.level}|${change.quality}|${change.affixKind}`;
    const aggregate = byCell.get(key) ?? {
      level: change.level,
      quality: change.quality,
      affixKind: change.affixKind,
      touchedFamilies: 0,
      addedEligibleOccurrences: 0,
      retunedEligibleOccurrences: 0,
      rangeChangedOccurrences: 0,
      maximumTotalVariationDistance: 0,
      maximumImpactFamily: null,
    };
    aggregate.touchedFamilies += 1;
    aggregate.addedEligibleOccurrences += change.delta.addedRows;
    aggregate.retunedEligibleOccurrences += change.delta.changedRows;
    aggregate.rangeChangedOccurrences += change.delta.rangeChanges;
    if (change.delta.totalVariationDistance > aggregate.maximumTotalVariationDistance) {
      aggregate.maximumTotalVariationDistance = change.delta.totalVariationDistance;
      aggregate.maximumImpactFamily = change.familyCode;
    }
    byCell.set(key, aggregate);
    for (const sample of [change.sampling.before, change.sampling.after]) {
      totalSampleDraws += sample.draws;
      maximumSamplingError = Math.max(maximumSamplingError, sample.absoluteError ?? 0);
    }
  }
  const largestImpacts = [...changes]
    .sort((left, right) => (
      right.delta.totalVariationDistance - left.delta.totalVariationDistance
      || left.familyCode.localeCompare(right.familyCode)
      || left.level - right.level
    ))
    .slice(0, 12)
    .map((change) => ({
      familyCode: change.familyCode,
      familyLabel: change.familyLabel,
      level: change.level,
      quality: change.quality,
      affixKind: change.affixKind,
      totalVariationDistance: change.delta.totalVariationDistance,
      addedRows: change.delta.addedRows,
      rangeChanges: change.delta.rangeChanges,
    }));
  return {
    concreteFamilyCount: totalFamilyCount,
    touchedFamilyCount: families.length,
    untouchedFamilyCount: totalFamilyCount - families.length,
    changedPoolCount: changes.length,
    drawsPerChangedPoolAndSide: config.drawsPerChangedPool,
    totalSampleDraws,
    maximumSamplingError,
    byLevelQualityAndKind: [...byCell.values()].sort((left, right) => (
      left.level - right.level
      || left.quality.localeCompare(right.quality)
      || left.affixKind.localeCompare(right.affixKind)
    )),
    largestImpacts,
  };
}

export function buildDistributionReport(catalog = readJson(catalogPath)) {
  const config = catalog.simulation;
  assert(config, 'Missing simulation policy in pd2-affixes-merge.catalog.json');
  assert(Array.isArray(config.levels) && config.levels.join(',') === '1,45,65,85,99', 'Simulation levels drift');
  assert(Number.isInteger(config.drawsPerChangedPool) && config.drawsPerChangedPool > 0, 'Invalid simulation draw count');
  const tempRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'pd2-affix-distribution-'));
  try {
    const before = loadCommit(config.beforeCommit, tempRoot);
    const after = loadCommit(config.afterCommit, tempRoot);
    const beforeCodes = before.families.map(({ code }) => code);
    const afterCodes = after.families.map(({ code }) => code);
    assert(JSON.stringify(beforeCodes) === JSON.stringify(afterCodes), 'Concrete item family set drifted across the merge');

    const families = [];
    for (const family of after.families) {
      const changes = [];
      for (const level of config.levels) {
        for (const quality of qualities) {
          for (const table of affixTables) {
            const affixKind = table === 'magicprefix.txt' ? 'prefix' : 'suffix';
            const simulationKey = `${family.code}|${level}|${quality}|${affixKind}`;
            const comparison = comparePools(
              eligibleAffixes(before.affixes[table], family, level, quality),
              eligibleAffixes(after.affixes[table], family, level, quality),
              simulationKey,
              config,
            );
            if (comparison) changes.push({ level, quality, affixKind, ...comparison });
          }
        }
      }
      if (changes.length > 0) families.push({ code: family.code, label: family.label, changes });
    }

    const summary = aggregateReport(families, after.families.length, config);
    const affixChanges = compareAffixTables(before.affixes, after.affixes);
    const publishedFamilies = families.map((family) => ({
      ...family,
      changes: family.changes.map(({ sampling, ...change }) => change),
    }));
    return {
      schemaVersion: 1,
      reportId: 'pd2-affixes-merge-distribution',
      model: {
        scope: 'One weighted prefix or suffix slot; affix-count progression remains owned by ProgressiveAffixesPlugin.',
        levelAxis: 'The governed MagicPrefix/MagicSuffix level and maxlevel fields are evaluated against item level.',
        quality: {
          magic: 'All spawnable eligible rows.',
          rare: 'Spawnable eligible rows with rare=1.',
        },
        typeResolution: 'Concrete spawnable Armor/Weapons/Misc type plus transitive ItemTypes Equiv ancestry.',
        exclusions: 'etype ancestry and classspecific restrictions are applied before weighting.',
        probability: 'frequency divided by total eligible frequency.',
        groups: 'Reported as an exclusion surface; multi-slot ordering is outside this one-slot model.',
        sampling: 'Deterministic xorshift32 checks the exact affected-affix probability mass.',
      },
      source: {
        beforeCommit: config.beforeCommit,
        afterCommit: config.afterCommit,
        beforeHashes: {
          ...before.hashes,
          affixes: Object.fromEntries(affixTables.map((name) => [name, before.affixes[name].sha256])),
        },
        afterHashes: {
          ...after.hashes,
          affixes: Object.fromEntries(affixTables.map((name) => [name, after.affixes[name].sha256])),
        },
      },
      levels: config.levels,
      seed: config.seed,
      summary,
      affixChanges,
      families: publishedFamilies,
    };
  } finally {
    fs.rmSync(tempRoot, { recursive: true, force: true });
  }
}

function canonicalReport(report) {
  return `${JSON.stringify(report, null, 2)}\n`;
}

function commitAvailable(commit) {
  try {
    execFileSync('git', ['cat-file', '-e', `${commit}^{commit}`], {
      cwd: repoRoot,
      stdio: 'ignore',
    });
    return true;
  } catch {
    return false;
  }
}

export function canReproduceDistribution(catalog = readJson(catalogPath)) {
  return commitAvailable(catalog.simulation.beforeCommit)
    && commitAvailable(catalog.simulation.afterCommit);
}

export function run(argv = process.argv.slice(2)) {
  const write = argv.includes('--write');
  const check = argv.includes('--check') || !write;
  assert(!(write && check), 'Choose exactly one of --write or --check');
  const catalog = readJson(catalogPath);
  const reproducibleHere = canReproduceDistribution(catalog);
  assert(!write || reproducibleHere, 'Pinned before/after commits are unavailable; a full Git history is required to rewrite the report');
  assert(fs.existsSync(reportPath) || write, 'Distribution report is missing; run with --write from a full Git history');
  const generatedReport = reproducibleHere ? buildDistributionReport(catalog) : null;
  const raw = generatedReport
    ? canonicalReport(generatedReport)
    : fs.readFileSync(reportPath, 'utf8');
  const report = generatedReport ?? JSON.parse(raw);
  if (write) {
    fs.writeFileSync(reportPath, raw, 'utf8');
  } else {
    if (reproducibleHere) {
      assert(fs.readFileSync(reportPath, 'utf8') === raw, 'Distribution report is stale');
    }
    assert(catalog.simulation.reportSha256, 'Distribution report hash is not pinned');
    assert(sha256(Buffer.from(raw, 'utf8')) === catalog.simulation.reportSha256, 'Distribution report hash drift');
    for (const [key, expected] of Object.entries(catalog.simulation.expectedSummary)) {
      assert(report.summary[key] === expected, `Simulation summary drift for ${key}`);
    }
    assert(
      report.summary.maximumSamplingError <= catalog.simulation.maximumSamplingError,
      `Sampling error ${report.summary.maximumSamplingError} exceeds the governed tolerance`,
    );
  }
  console.log(JSON.stringify({
    mode: write ? 'write' : 'check',
    reproduction: reproducibleHere ? 'complete' : 'skipped_missing_git_history',
    report: path.relative(repoRoot, reportPath),
    reportSha256: sha256(Buffer.from(raw, 'utf8')),
    summary: report.summary,
  }, null, 2));
}

const invokedPath = process.argv[1] ? path.resolve(process.argv[1]) : '';
if (invokedPath === fileURLToPath(import.meta.url)) {
  try {
    run();
  } catch (error) {
    console.error(`INVALID PD2 Affix Distribution: ${error.message}`);
    process.exitCode = 1;
  }
}
