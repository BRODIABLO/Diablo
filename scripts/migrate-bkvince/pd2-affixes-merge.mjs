import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

const require = createRequire(import.meta.url);
const {
  ENCODING,
  parseTable,
  serializeTable,
  writeTable,
} = require('../build-data/tsv.js');

const repoRoot = fileURLToPath(new URL('../../', import.meta.url));
const catalogPath = path.join(repoRoot, 'Mission', 'pd2-affixes-merge.catalog.json');
const targetExcelRoot = path.join(
  repoRoot,
  'data-BKVince',
  'BKVince.mpq',
  'data',
  'global',
  'excel',
);
const vanillaExcelRoot = path.join(
  repoRoot,
  'data-vanilla3.2',
  'data',
  'data',
  'global',
  'excel',
);
const modernStringsRoot = path.join(
  repoRoot,
  'data-BKVince',
  'BKVince.mpq',
  'data',
  'local',
  'lng',
  'strings',
);
const legacyStringsRoot = path.join(
  repoRoot,
  'data-BKVince',
  'BKVince.mpq',
  'data',
  'local',
  'lng',
  'strings-legacy',
);
const defaultReportPath = path.join(
  repoRoot,
  'analysis-cache',
  'pd2-affixes-merge',
  'merge-report.json',
);

const TABLE_CONFIG = Object.freeze({
  'magicprefix.txt': Object.freeze({
    mappedRows: 670,
    appendStart: 670,
    targetRow(sourceRow) { return sourceRow; },
    applyRetunes: true,
    applyAppends: true,
  }),
  'magicsuffix.txt': Object.freeze({
    mappedRows: 748,
    appendStart: 748,
    targetRow(sourceRow) { return sourceRow <= 662 ? sourceRow : sourceRow + 7; },
    applyRetunes: true,
    applyAppends: true,
  }),
  'automagic.txt': Object.freeze({
    mappedRows: 36,
    appendStart: 36,
    targetRow(sourceRow) { return sourceRow; },
    applyRetunes: false,
    applyAppends: false,
  }),
});

const STRUCTURAL_HEADERS = Object.freeze([
  'Name',
  'version',
  'spawnable',
  'rare',
  'classspecific',
  'class',
  'classlevelreq',
  'group',
  'transformcolor',
  'itype1',
  'itype2',
  'itype3',
  'itype4',
  'itype5',
  'itype6',
  'itype7',
  'etype1',
  'etype2',
  'etype3',
  'etype4',
  'etype5',
]);

const MOD_IDENTITY_HEADERS = Object.freeze([
  'mod1code',
  'mod1param',
  'mod2code',
  'mod2param',
  'mod3code',
  'mod3param',
]);

const EXISTING_IDENTITY_HEADERS = Object.freeze([
  ...STRUCTURAL_HEADERS,
  ...MOD_IDENTITY_HEADERS,
]);

const MOD_CODE_HEADERS = Object.freeze(['mod1code', 'mod2code', 'mod3code']);
const ITEM_TYPE_HEADERS = Object.freeze([
  'itype1',
  'itype2',
  'itype3',
  'itype4',
  'itype5',
  'itype6',
  'itype7',
  'etype1',
  'etype2',
  'etype3',
  'etype4',
  'etype5',
]);

const MODERN_LOCALES = Object.freeze([
  'enUS',
  'zhTW',
  'deDE',
  'esES',
  'frFR',
  'itIT',
  'koKR',
  'plPL',
  'esMX',
  'jaJP',
  'ptBR',
  'ruRU',
  'zhCN',
]);

const LEGACY_LOCALES = Object.freeze([
  'enUS',
  'zhTW',
  'deDE',
  'esES',
  'frFR',
  'itIT',
  'koKR',
  'plPL',
]);

function assert(condition, message) {
  if (!condition) throw new Error(message);
}

function sha256(value) {
  return crypto.createHash('sha256').update(value).digest('hex').toUpperCase();
}

function stableSha(value) {
  return sha256(Buffer.from(JSON.stringify(value), 'utf8'));
}

function readJson(filePath) {
  const raw = fs.readFileSync(filePath, 'utf8').replace(/^\uFEFF/, '');
  return JSON.parse(raw);
}

function findFile(root, normalizedName) {
  assert(fs.existsSync(root), `Source directory missing: ${root}`);
  const matches = fs.readdirSync(root)
    .filter((name) => name.toLowerCase() === normalizedName.toLowerCase());
  assert(matches.length === 1, `${root}: expected one ${normalizedName}, found ${matches.length}`);
  return path.join(root, matches[0]);
}

export function loadTable(root, normalizedName) {
  const filePath = findFile(root, normalizedName);
  const rawBuffer = fs.readFileSync(filePath);
  const rawText = fs.readFileSync(filePath, ENCODING);
  const table = parseTable(filePath);
  assert(rawText === serializeTable(table), `Non byte-exact TSV round-trip: ${filePath}`);
  for (const [rowIndex, row] of table.rows.entries()) {
    assert(
      row.length === table.headers.length,
      `${normalizedName}: row ${rowIndex} has ${row.length}/${table.headers.length} cells`,
    );
  }
  return {
    filePath,
    rawBuffer,
    sha256: sha256(rawBuffer),
    table,
  };
}

function cloneTable(table) {
  return {
    headers: [...table.headers],
    rows: table.rows.map((row) => [...row]),
    eol: table.eol,
    hasFinalEol: table.hasFinalEol,
  };
}

export function headerIndexes(table) {
  const result = new Map();
  for (const [index, header] of table.headers.entries()) {
    const normalized = header.toLowerCase();
    assert(!result.has(normalized), `Duplicate case-insensitive header: ${header}`);
    result.set(normalized, index);
  }
  return result;
}

function cell(table, indexes, rowIndex, header) {
  const column = indexes.get(header.toLowerCase());
  assert(column !== undefined, `Missing header ${header}`);
  return table.rows[rowIndex]?.[column] ?? '';
}

function rowCell(row, indexes, header) {
  const column = indexes.get(header.toLowerCase());
  assert(column !== undefined, `Missing header ${header}`);
  return row[column] ?? '';
}

function uniqueValues(values) {
  return [...new Set(values)];
}

export function parseOrdinalSpec(spec) {
  assert(typeof spec === 'string' && spec.length > 0, 'Ordinal specification is empty');
  const result = [];
  for (const token of spec.split(',')) {
    const match = /^(\d+)(?:-(\d+))?$/.exec(token);
    assert(match, `Invalid ordinal token: ${token}`);
    const start = Number(match[1]);
    const end = match[2] === undefined ? start : Number(match[2]);
    assert(end >= start, `Descending ordinal range: ${token}`);
    for (let value = start; value <= end; value += 1) result.push(value);
  }
  assert(new Set(result).size === result.length, `Duplicate ordinal in ${spec}`);
  return result;
}

function sameArrays(left, right) {
  return left.length === right.length && left.every((value, index) => value === right[index]);
}

function assertExactOrdinals(actual, spec, label) {
  const expected = parseOrdinalSpec(spec);
  assert(
    sameArrays(actual, expected),
    `${label}: expected ordinals ${spec}, got ${actual.join(',')}`,
  );
}

function sourceRootFromArgs(args) {
  const option = args.find((arg) => arg.startsWith('--source-root='));
  if (option) return path.resolve(option.slice('--source-root='.length));
  if (process.env.PD2_AFFIX_SOURCE_ROOT) return path.resolve(process.env.PD2_AFFIX_SOURCE_ROOT);
  if (process.env.PD2_SP_ROOT) return path.resolve(process.env.PD2_SP_ROOT);

  const officialExtraction = path.join(
    repoRoot,
    'analysis-cache',
    'pd2-affixes-merge',
    'official-s13',
  );
  if (fs.existsSync(officialExtraction)) return officialExtraction;
  return path.resolve(
    repoRoot,
    '..',
    'PD2 Single PLayer',
    'PD2-Single-Player-Plus-mod-main',
    'data',
    'global',
    'excel',
  );
}

function targetExcelRootFromArgs(args) {
  const option = args.find((arg) => arg.startsWith('--target-excel-root='));
  return option
    ? path.resolve(option.slice('--target-excel-root='.length))
    : targetExcelRoot;
}

function verifySourceTable(name, loaded, expected) {
  const acceptedHashes = [expected.officialSha256, expected.mirrorSha256].filter(Boolean);
  assert(
    acceptedHashes.includes(loaded.sha256),
    `${name}: unrecognized PD2 source hash ${loaded.sha256}`,
  );
  if (expected.rows !== undefined) {
    assert(loaded.table.rows.length === expected.rows, `${name}: source row count drift`);
  }
  if (expected.columns !== undefined) {
    assert(loaded.table.headers.length === expected.columns, `${name}: source column count drift`);
  }
}

export function canonicalPropertySignature(row, indexes) {
  const signature = [];
  for (let slot = 1; slot <= 7; slot += 1) {
    signature.push([
      rowCell(row, indexes, `func${slot}`),
      rowCell(row, indexes, `stat${slot}`),
      rowCell(row, indexes, `set${slot}`),
      rowCell(row, indexes, `val${slot}`),
    ]);
  }
  return signature;
}

function keyedRows(table, keyHeader, signatureBuilder) {
  const indexes = headerIndexes(table);
  const keyColumn = indexes.get(keyHeader.toLowerCase());
  assert(keyColumn !== undefined, `Missing key header ${keyHeader}`);
  const result = new Map();
  for (const [rowIndex, row] of table.rows.entries()) {
    const rawKey = row[keyColumn] ?? '';
    if (!rawKey) continue;
    const key = rawKey.toLowerCase();
    assert(!result.has(key), `Duplicate ${keyHeader} ${rawKey} at row ${rowIndex}`);
    result.set(key, {
      rawKey,
      row,
      rowIndex,
      signature: signatureBuilder ? signatureBuilder(row, indexes) : null,
    });
  }
  return result;
}

export function propertyCompatibility(sourceTable, targetTable) {
  const source = keyedRows(sourceTable, 'code', canonicalPropertySignature);
  const target = keyedRows(targetTable, 'code', canonicalPropertySignature);
  const compatible = new Set();
  const incompatible = new Set();
  const missing = new Set();
  for (const [code, record] of source) {
    const targetRecord = target.get(code);
    if (!targetRecord) {
      missing.add(code);
    } else if (JSON.stringify(record.signature) === JSON.stringify(targetRecord.signature)) {
      compatible.add(code);
    } else {
      incompatible.add(code);
    }
  }
  return { source, target, compatible, incompatible, missing };
}

export function itemStatIndex(table) {
  return keyedRows(table, 'Stat', null);
}

function optionalInteger(value, label) {
  if (value === '') return null;
  assert(/^-?\d+$/.test(value), `${label}: expected an integer, got ${value}`);
  const parsed = Number(value);
  assert(Number.isSafeInteger(parsed), `${label}: unsafe integer ${value}`);
  return parsed;
}

export function storedValueRange(statRecord, statIndexes) {
  const bits = optionalInteger(
    rowCell(statRecord.row, statIndexes, 'Save Bits'),
    `${statRecord.rawKey} Save Bits`,
  );
  if (!bits) return null;
  assert(bits > 0 && bits <= 32, `${statRecord.rawKey}: unsupported Save Bits ${bits}`);
  const add = optionalInteger(
    rowCell(statRecord.row, statIndexes, 'Save Add'),
    `${statRecord.rawKey} Save Add`,
  ) ?? 0;
  return {
    minimum: add === 0 ? 0 : -add,
    maximum: Number((2n ** BigInt(bits)) - 1n) - add,
    bits,
    add,
  };
}

function propertyFunctionStats(func, declaredStat) {
  if (func === '5') return ['mindamage'];
  if (func === '6') return ['maxdamage'];
  if (func === '7') return ['item_maxdamage_percent', 'item_mindamage_percent'];
  if (func === '20') return ['item_indesctructible'];
  return declaredStat ? [declaredStat] : [];
}

export function propertyFunctionValues(func, minimum, maximum, parameter, fixedValue) {
  if (['4', '16'].includes(func)) return [maximum, fixedValue].filter((value) => value !== null);
  if (func === '15') return [minimum, fixedValue].filter((value) => value !== null);
  if (func === '17') {
    return [parameter ?? minimum, parameter === null ? maximum : null, fixedValue]
      .filter((value) => value !== null);
  }
  if (['1', '2', '3', '5', '6', '7', '8', '10', '21'].includes(func)) {
    return [minimum, maximum, fixedValue].filter((value) => value !== null);
  }
  return [minimum, maximum, fixedValue].filter((value) => value !== null);
}

export function rowItemStatAudit(row, rowIndexes, properties, itemStats, label) {
  const reasons = [];
  for (let slot = 1; slot <= 3; slot += 1) {
    const rawProperty = rowCell(row, rowIndexes, `mod${slot}code`);
    if (!rawProperty) continue;
    const property = properties.source.get(rawProperty.toLowerCase());
    assert(property, `${label}: unknown source Property ${rawProperty}`);
    const minimum = optionalInteger(rowCell(row, rowIndexes, `mod${slot}min`), `${label} mod${slot}min`);
    const maximum = optionalInteger(rowCell(row, rowIndexes, `mod${slot}max`), `${label} mod${slot}max`);
    const parameter = optionalInteger(rowCell(row, rowIndexes, `mod${slot}param`), `${label} mod${slot}param`);

    for (const [func, declaredStat, , rawFixedValue] of property.signature) {
      if (!func) continue;
      const fixedValue = optionalInteger(rawFixedValue, `${label} ${rawProperty} val`);
      const values = propertyFunctionValues(func, minimum, maximum, parameter, fixedValue);
      const stats = propertyFunctionStats(func, declaredStat);
      for (const rawStat of stats) {
        const statName = rawStat.toLowerCase();
        const sourceStat = itemStats.source.get(statName);
        const targetStat = itemStats.target.get(statName);
        if (!sourceStat || !targetStat) {
          reasons.push(`${rawProperty}:${rawStat}:missing-itemstatcost`);
          continue;
        }
        const range = storedValueRange(targetStat, itemStats.targetIndexes);
        if (range) {
          for (const value of values) {
            if (value < range.minimum || value > range.maximum) {
              reasons.push(
                `${rawProperty}:${rawStat}:${value} outside ${range.minimum}..${range.maximum}`,
              );
            }
          }
        }

        const sendBits = optionalInteger(
          rowCell(targetStat.row, itemStats.targetIndexes, 'Send Bits'),
          `${rawStat} Send Bits`,
        );
        if (sendBits) {
          assert(sendBits > 0 && sendBits <= 32, `${rawStat}: unsupported Send Bits ${sendBits}`);
          const signed = rowCell(targetStat.row, itemStats.targetIndexes, 'Signed') === '1' && sendBits < 32;
          const sendMinimum = signed ? -Number(2n ** BigInt(sendBits - 1)) : 0;
          const sendMaximum = signed ? Number((2n ** BigInt(sendBits - 1)) - 1n) : Number((2n ** BigInt(sendBits)) - 1n);
          for (const value of values) {
            if (value < sendMinimum || value > sendMaximum) reasons.push(`${rawProperty}:${rawStat}:${value} outside send ${sendMinimum}..${sendMaximum}`);
          }
        }

        const parameterBits = optionalInteger(
          rowCell(targetStat.row, itemStats.targetIndexes, 'Save Param Bits'),
          `${rawStat} Save Param Bits`,
        );
        if (parameterBits) {
          const parameterCandidates = [parameter, fixedValue].filter((value) => value !== null);
          const maximumParameter = Number((2n ** BigInt(parameterBits)) - 1n);
          for (const value of parameterCandidates) {
            if (value < 0 || value > maximumParameter) {
              reasons.push(`${rawProperty}:${rawStat}:param ${value} outside 0..${maximumParameter}`);
            }
          }
        }
        const sendParameterBits = optionalInteger(
          rowCell(targetStat.row, itemStats.targetIndexes, 'Send Param Bits'),
          `${rawStat} Send Param Bits`,
        );
        if (sendParameterBits) {
          assert(sendParameterBits > 0 && sendParameterBits <= 32, `${rawStat}: unsupported Send Param Bits ${sendParameterBits}`);
          const parameterCandidates = [parameter, fixedValue].filter((value) => value !== null);
          const minimumParameter = sendParameterBits < 32 ? -Number(2n ** BigInt(sendParameterBits - 1)) : -2147483648;
          const maximumParameter = sendParameterBits < 32 ? Number((2n ** BigInt(sendParameterBits - 1)) - 1n) : 2147483647;
          for (const value of parameterCandidates) {
            if (value < minimumParameter || value > maximumParameter) reasons.push(`${rawProperty}:${rawStat}:param ${value} outside send ${minimumParameter}..${maximumParameter}`);
          }
        }
      }
    }
  }
  return { ok: reasons.length === 0, reasons: uniqueValues(reasons) };
}

export function itemTypeIndex(table) {
  const indexes = headerIndexes(table);
  const codeColumn = indexes.get('code');
  assert(codeColumn !== undefined, 'ItemTypes is missing Code');
  const result = new Map();
  for (const [rowIndex, row] of table.rows.entries()) {
    const rawCode = row[codeColumn] ?? '';
    if (!rawCode) continue;
    const code = rawCode.toLowerCase();
    assert(!result.has(code), `Duplicate ItemType code ${rawCode} at row ${rowIndex}`);
    result.set(code, {
      code,
      equiv1: rowCell(row, indexes, 'Equiv1').toLowerCase(),
      equiv2: rowCell(row, indexes, 'Equiv2').toLowerCase(),
    });
  }
  return result;
}

export function itemTypeReaches(itemTypes, startCode, wantedCode) {
  const wanted = wantedCode.toLowerCase();
  const pending = [startCode.toLowerCase()];
  const visited = new Set();
  while (pending.length > 0) {
    const code = pending.pop();
    if (!code || visited.has(code)) continue;
    if (code === wanted) return true;
    visited.add(code);
    const record = itemTypes.get(code);
    if (!record) continue;
    pending.push(record.equiv1, record.equiv2);
  }
  return false;
}

function usedValues(row, indexes, headers) {
  return headers.map((header) => rowCell(row, indexes, header)).filter(Boolean);
}

function rowProperties(row, indexes) {
  return usedValues(row, indexes, MOD_CODE_HEADERS).map((code) => code.toLowerCase());
}

function rowIdentity(row, indexes) {
  return JSON.stringify(EXISTING_IDENTITY_HEADERS.map((header) => rowCell(row, indexes, header)));
}

export function isMapRow(row, indexes, sourceItemTypes) {
  if (rowProperties(row, indexes).some((code) => /^map-/.test(code))) return true;
  return usedValues(row, indexes, ITEM_TYPE_HEADERS)
    .some((code) => itemTypeReaches(sourceItemTypes, code, 'map'));
}

function projectSourceRow(sourceRow, sourceIndexes, targetTable) {
  const projected = targetTable.headers.map((header) => {
    const sourceColumn = sourceIndexes.get(header.toLowerCase());
    assert(sourceColumn !== undefined, `PD2 source is missing target header ${header}`);
    return sourceRow[sourceColumn] ?? '';
  });
  const targetIndexes = headerIndexes(targetTable);
  projected[targetIndexes.get('multiply')] = '0';
  return projected;
}

function existingIdentitySha(table, physicalRowCount) {
  const indexes = headerIndexes(table);
  const identity = table.rows.slice(0, physicalRowCount).map((row, rowIndex) => [
    rowIndex,
    ...EXISTING_IDENTITY_HEADERS.map((header) => rowCell(row, indexes, header)),
  ]);
  return stableSha(identity);
}

function expansionRow(table) {
  const indexes = headerIndexes(table);
  const nameColumn = indexes.get('name');
  const matches = table.rows
    .map((row, index) => ({ index, row }))
    .filter(({ row }) => (row[nameColumn] ?? '') === 'Expansion');
  assert(matches.length <= 1, 'Multiple Expansion sentinels');
  const caseInsensitive = table.rows
    .filter((row) => String(row[nameColumn] ?? '').toLowerCase() === 'expansion');
  assert(caseInsensitive.length === matches.length, 'Expansion sentinel casing drift');
  return matches[0] ?? null;
}

function compiledRowCount(table) {
  return table.rows.filter((row) => row[0] !== 'Expansion').length;
}

function assertTargetTableShape(name, loaded, expected, finalMode = false) {
  assert(loaded.table.eol === '\r\n', `${name}: target must remain CRLF`);
  assert(loaded.table.hasFinalEol, `${name}: target must retain its final EOL`);
  const expansion = expansionRow(loaded.table);
  if (expected.expansionPhysicalRow === null) {
    assert(expansion === null, `${name}: unexpected Expansion sentinel`);
  } else {
    assert(expansion?.index === expected.expansionPhysicalRow, `${name}: Expansion moved`);
  }
  if (!finalMode) {
    assert(loaded.table.rows.length === expected.physicalRows, `${name}: baseline physical row drift`);
    assert(compiledRowCount(loaded.table) === expected.compiledRows, `${name}: baseline ID count drift`);
  }
}

function mappedIdentityCheck(name, config, source, vanilla, target) {
  const sourceIndexes = headerIndexes(source.table);
  const vanillaIndexes = headerIndexes(vanilla.table);
  const targetIndexes = headerIndexes(target.table);
  for (let sourceRow = 0; sourceRow < config.mappedRows; sourceRow += 1) {
    const targetRow = config.targetRow(sourceRow);
    const sourceName = cell(source.table, sourceIndexes, sourceRow, 'Name');
    const vanillaName = cell(vanilla.table, vanillaIndexes, sourceRow, 'Name');
    const targetName = cell(target.table, targetIndexes, targetRow, 'Name');
    assert(
      sourceName === vanillaName && vanillaName === targetName,
      `${name}: identity map drift at source ${sourceRow}/target ${targetRow}`,
    );
  }
}

function allHeadersEqual(sourceRow, sourceIndexes, vanillaRow, vanillaIndexes, targetRow, targetIndexes, headers) {
  return headers.every((header) => {
    const sourceValue = rowCell(sourceRow, sourceIndexes, header);
    const vanillaValue = rowCell(vanillaRow, vanillaIndexes, header);
    const targetValue = rowCell(targetRow, targetIndexes, header);
    return sourceValue === vanillaValue && vanillaValue === targetValue;
  });
}

function computeRetunes(name, config, source, vanilla, target, properties, itemStats, catalog) {
  const sourceIndexes = headerIndexes(source.table);
  const vanillaIndexes = headerIndexes(vanilla.table);
  const targetIndexes = headerIndexes(target.table);
  const retuneColumns = catalog.policy.retuneColumns;
  const cells = [];

  for (let sourceRowIndex = 0; sourceRowIndex < config.mappedRows; sourceRowIndex += 1) {
    const targetRowIndex = config.targetRow(sourceRowIndex);
    const sourceRow = source.table.rows[sourceRowIndex];
    const vanillaRow = vanilla.table.rows[sourceRowIndex];
    const targetRow = target.table.rows[targetRowIndex];
    assert(sourceRow && vanillaRow && targetRow, `${name}: mapped row is missing at ${sourceRowIndex}`);

    if (!allHeadersEqual(
      sourceRow,
      sourceIndexes,
      vanillaRow,
      vanillaIndexes,
      targetRow,
      targetIndexes,
      STRUCTURAL_HEADERS,
    )) continue;
    if (rowCell(sourceRow, sourceIndexes, 'spawnable') !== '1') continue;
    if (!allHeadersEqual(
      sourceRow,
      sourceIndexes,
      vanillaRow,
      vanillaIndexes,
      targetRow,
      targetIndexes,
      MOD_IDENTITY_HEADERS,
    )) continue;

    const propertyCodes = rowProperties(sourceRow, sourceIndexes);
    if (!propertyCodes.every((code) => properties.compatible.has(code))) continue;
    if (!rowItemStatAudit(
      sourceRow,
      sourceIndexes,
      properties,
      itemStats,
      `${name} source row ${sourceRowIndex}`,
    ).ok) continue;

    for (const column of retuneColumns) {
      const sourceValue = rowCell(sourceRow, sourceIndexes, column);
      const vanillaValue = rowCell(vanillaRow, vanillaIndexes, column);
      const targetValue = rowCell(targetRow, targetIndexes, column);
      if (sourceValue === vanillaValue || targetValue !== vanillaValue) continue;
      cells.push({
        table: name,
        sourceRow: sourceRowIndex,
        targetRow: targetRowIndex,
        name: rowCell(sourceRow, sourceIndexes, 'Name'),
        column,
        before: targetValue,
        after: sourceValue,
      });
    }
  }

  const expected = catalog.expected.retunes[name];
  const rows = uniqueValues(cells.map((change) => change.sourceRow));
  assertExactOrdinals(rows, expected.sourceRows, `${name} retunes`);
  assert(rows.length === expected.rows, `${name}: retune row count drift`);
  assert(cells.length === expected.cells, `${name}: retune cell count drift`);
  if (expected.byColumn) {
    const byColumn = Object.fromEntries(retuneColumns.map((column) => [
      column,
      cells.filter((change) => change.column === column).length,
    ]).filter(([, count]) => count > 0));
    const columnsMatch = Object.keys(byColumn).length === Object.keys(expected.byColumn).length
      && Object.entries(expected.byColumn).every(([column, count]) => byColumn[column] === count);
    assert(
      columnsMatch,
      `${name}: retune columns drift ${JSON.stringify(byColumn)} != ${JSON.stringify(expected.byColumn)}`,
    );
  }
  return cells;
}

function rowIsExactDuplicate(projected, targetTable) {
  return targetTable.rows.some((row) => sameArrays(row, projected));
}

function computeAppends(
  name,
  config,
  source,
  vanilla,
  target,
  properties,
  sourceItemTypes,
  targetItemTypes,
  itemStats,
  catalog,
) {
  const sourceIndexes = headerIndexes(source.table);
  const vanillaIndexes = headerIndexes(vanilla.table);
  const vanillaIdentities = new Set(vanilla.table.rows.map((row) => rowIdentity(row, vanillaIndexes)));
  const skillParamProperties = new Set(catalog.policy.skillParamProperties.map((code) => code.toLowerCase()));
  const selected = [];
  const blocked = {
    unnamed: [],
    nonSpawnable: [],
    map: [],
    property: [],
    itemType: [],
    skillParam: [],
    duplicate: [],
    relocatedVanilla: [],
    itemStat: [],
  };

  for (let sourceRow = config.appendStart; sourceRow < source.table.rows.length; sourceRow += 1) {
    const row = source.table.rows[sourceRow];
    const nameValue = rowCell(row, sourceIndexes, 'Name');
    if (!nameValue) {
      blocked.unnamed.push(sourceRow);
      continue;
    }
    if (rowCell(row, sourceIndexes, 'spawnable') !== '1') {
      blocked.nonSpawnable.push(sourceRow);
      continue;
    }
    if (vanillaIdentities.has(rowIdentity(row, sourceIndexes))) {
      blocked.relocatedVanilla.push(sourceRow);
      continue;
    }
    if (isMapRow(row, sourceIndexes, sourceItemTypes)) {
      blocked.map.push(sourceRow);
      continue;
    }

    const propertyCodes = rowProperties(row, sourceIndexes);
    if (!propertyCodes.every((code) => properties.compatible.has(code))) {
      blocked.property.push(sourceRow);
      continue;
    }

    const typeCodes = usedValues(row, sourceIndexes, ITEM_TYPE_HEADERS).map((code) => code.toLowerCase());
    if (!typeCodes.every((code) => targetItemTypes.has(code))) {
      blocked.itemType.push(sourceRow);
      continue;
    }
    const statAudit = rowItemStatAudit(
      row,
      sourceIndexes,
      properties,
      itemStats,
      `${name} source row ${sourceRow}`,
    );
    if (!statAudit.ok) {
      blocked.itemStat.push({ sourceRow, reasons: statAudit.reasons });
      continue;
    }
    if (propertyCodes.some((code) => skillParamProperties.has(code))) {
      blocked.skillParam.push(sourceRow);
      continue;
    }

    const projected = projectSourceRow(row, sourceIndexes, target.table);
    if (rowIsExactDuplicate(projected, target.table)) {
      blocked.duplicate.push(sourceRow);
      continue;
    }
    selected.push({
      table: name,
      sourceRow,
      name: nameValue,
      sourceGroup: rowCell(row, sourceIndexes, 'group'),
      properties: propertyCodes,
      itemTypes: typeCodes,
      projected,
      sourceFingerprint: stableSha(row),
      projectedFingerprint: stableSha(projected),
    });
  }

  const expected = catalog.expected.appends[name];
  assertExactOrdinals(selected.map((entry) => entry.sourceRow), expected.sourceRows, `${name} appends`);
  assert(selected.length === expected.rows, `${name}: append row count drift`);
  assert(
    new Set(selected.map((entry) => entry.projectedFingerprint)).size === selected.length,
    `${name}: duplicate projected append rows`,
  );
  const projectionSha256 = stableSha(selected.map((entry) => [entry.sourceRow, entry.projected]));
  if (expected.projectionSha256) {
    assert(projectionSha256 === expected.projectionSha256, `${name}: append projection drift`);
  }
  for (const [disposition, spec] of Object.entries(expected.blocked ?? {})) {
    const actual = disposition === 'itemStat'
      ? blocked.itemStat.map((entry) => entry.sourceRow)
      : blocked[disposition];
    assert(Array.isArray(actual), `${name}: unknown blocked disposition ${disposition}`);
    assertExactOrdinals(actual, spec, `${name} blocked ${disposition}`);
  }
  return { selected, blocked };
}

function auditGroups(name, config, source, target, selected, catalog) {
  const sourceIndexes = headerIndexes(source.table);
  const targetIndexes = headerIndexes(target.table);
  const anchored = new Set();
  for (let sourceRow = 0; sourceRow < config.mappedRows; sourceRow += 1) {
    const targetRow = config.targetRow(sourceRow);
    const sourceGroup = cell(source.table, sourceIndexes, sourceRow, 'group');
    const targetGroup = cell(target.table, targetIndexes, targetRow, 'group');
    if (sourceGroup && sourceGroup === targetGroup) anchored.add(sourceGroup);
  }
  const usedTargetGroups = new Set(
    target.table.rows.map((row) => rowCell(row, targetIndexes, 'group')).filter(Boolean),
  );
  const explicitSafe = new Set(catalog.policy.safeUnmappedGroupCollisions[name]);
  const groups = uniqueValues(selected.map((entry) => entry.sourceGroup).filter(Boolean)).map((group) => {
    let disposition = 'free';
    if (usedTargetGroups.has(group)) {
      if (anchored.has(group)) disposition = 'anchored';
      else if (explicitSafe.has(group)) disposition = 'explicit-safe';
      else disposition = 'unsafe';
    }
    return { group, disposition };
  });
  const unsafe = groups.filter((entry) => entry.disposition === 'unsafe');
  assert(unsafe.length === 0, `${name}: unsafe group collision(s): ${unsafe.map((entry) => entry.group).join(',')}`);
  return groups;
}

export function buildAffixDependencyAuditContext(sourceRoot, targetRoot = targetExcelRoot) {
  const sourceProperties = loadTable(sourceRoot, 'properties.txt');
  const targetProperties = loadTable(targetRoot, 'properties.txt');
  const properties = propertyCompatibility(sourceProperties.table, targetProperties.table);
  const sourceItemStatsLoaded = loadTable(sourceRoot, 'itemstatcost.txt');
  const targetItemStatsLoaded = loadTable(targetRoot, 'itemstatcost.txt');
  const sourceItemTypesLoaded = loadTable(sourceRoot, 'itemtypes.txt');
  const targetItemTypesLoaded = loadTable(targetRoot, 'itemtypes.txt');
  const sourceSkills = loadTable(sourceRoot, 'skills.txt');
  const targetSkills = loadTable(targetRoot, 'skills.txt');
  const localizationSnapshots = {
    modern: loadLocalizationSnapshot(modernStringsRoot),
    legacy: loadLocalizationSnapshot(legacyStringsRoot),
  };
  const tables = {};
  for (const name of Object.keys(TABLE_CONFIG)) {
    tables[name] = {
      source: loadTable(sourceRoot, name),
      target: loadTable(targetRoot, name),
      vanilla: loadTable(vanillaExcelRoot, name),
    };
  }
  return {
    sourceRoot,
    targetRoot,
    dependencyHashes: {
      source: {
        'properties.txt': sourceProperties.sha256,
        'itemtypes.txt': sourceItemTypesLoaded.sha256,
        'itemstatcost.txt': sourceItemStatsLoaded.sha256,
        'skills.txt': sourceSkills.sha256,
      },
      bkvince: {
        'properties.txt': targetProperties.sha256,
        'itemtypes.txt': targetItemTypesLoaded.sha256,
        'itemstatcost.txt': targetItemStatsLoaded.sha256,
        'skills.txt': targetSkills.sha256,
        localization: {
          modern: {
            baseSha256: localizationSnapshots.modern.base.sha256,
            manifestSha256: localizationSnapshots.modern.manifestSha256,
          },
          legacy: {
            baseSha256: localizationSnapshots.legacy.base.sha256,
            manifestSha256: localizationSnapshots.legacy.manifestSha256,
          },
        },
      },
    },
    properties,
    sourceItemTypes: itemTypeIndex(sourceItemTypesLoaded.table),
    targetItemTypes: itemTypeIndex(targetItemTypesLoaded.table),
    itemStats: {
      source: itemStatIndex(sourceItemStatsLoaded.table),
      target: itemStatIndex(targetItemStatsLoaded.table),
      sourceIndexes: headerIndexes(sourceItemStatsLoaded.table),
      targetIndexes: headerIndexes(targetItemStatsLoaded.table),
    },
    sourceSkills,
    targetSkills,
    tables,
    localizationSnapshots,
    modernLocalization: localizationSnapshots.modern.base,
    legacyLocalization: localizationSnapshots.legacy.base,
  };
}

function objectAsTargetRow(object, targetTable) {
  return targetTable.headers.map((header) => object?.[header] ?? '');
}

function skillNameAt(loaded, id) {
  if (!Number.isSafeInteger(id) || id < 0) return null;
  const indexes = headerIndexes(loaded.table);
  const idColumn = indexes.get('*id') ?? indexes.get('id');
  const nameColumn = indexes.get('skill');
  if (idColumn === undefined || nameColumn === undefined) return null;
  const matches = loaded.table.rows.filter((row) => Number(row[idColumn]) === id);
  if (matches.length !== 1) return null;
  return matches[0][nameColumn] ?? null;
}

export function auditAffixProjection(context, {
  tableName,
  sourceRow,
  targetRow,
  projected,
  sourceOriginal,
  kind,
  catalog,
  provenanceByField = {},
}) {
  const tableContext = context.tables[tableName];
  assert(tableContext, `Unknown affix table ${tableName}`);
  const targetIndexes = headerIndexes(tableContext.target.table);
  const projectedRow = objectAsTargetRow(projected, tableContext.target.table);
  if (kind === 'append') projectedRow[targetIndexes.get('multiply')] = '0';
  const provenance = new Map(Object.entries(provenanceByField).map(([field, origin]) => [field.toLowerCase(), origin]));
  const adoptsPd2 = (field) => kind === 'append' || provenance.get(field.toLowerCase()) === 'PD2';
  const dependencies = [];
  const conflicts = [];
  const propertyCodes = rowProperties(projectedRow, targetIndexes);
  const supportedFunctions = new Set([
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '10', '11', '12', '13', '14', '15', '16', '17', '18', '19', '20', '21', '22', '23', '24', '25', '36',
  ]);
  for (let slot = 1; slot <= 3; slot += 1) {
    const code = rowCell(projectedRow, targetIndexes, `mod${slot}code`).toLowerCase();
    if (!code) continue;
    const sourceRecord = context.properties.source.get(code);
    const targetRecord = context.properties.target.get(code);
    const sourceSemantics = adoptsPd2(`mod${slot}code`);
    let status = 'compatible-existing';
    let reason = 'Property exists in BKVince with target runtime semantics.';
    if (!targetRecord) {
      status = 'absent';
      reason = 'Property is absent from BKVince.';
    } else if (sourceSemantics && !sourceRecord) {
      status = 'absent-source-definition';
      reason = 'The adopted PD2 Property has no unique source definition.';
    } else if (sourceSemantics && JSON.stringify(sourceRecord.signature) !== JSON.stringify(targetRecord.signature)) {
      status = 'incompatible';
      reason = 'PD2 and BKVince Property function/stat signatures differ.';
    } else if (sourceSemantics) {
      status = 'compatible';
      reason = 'PD2 and BKVince Property signatures are identical.';
    }
    dependencies.push({ kind: 'Property', code, slot, provenance: sourceSemantics ? 'PD2' : 'BKVINCE_OR_CUSTOM', status, reason, signature: targetRecord?.signature ?? null });
    if (['absent', 'absent-source-definition', 'incompatible'].includes(status)) conflicts.push({ kind: 'Property', code, reason });
    const property = targetRecord;
    if (property) {
      for (const [func, stat] of property.signature) {
        if (!func) continue;
        const functionStatus = supportedFunctions.has(func) ? 'supported' : 'unsupported';
        dependencies.push({ kind: 'PropertyFunction', code: func, property: code, slot, status: functionStatus });
        if (functionStatus === 'unsupported') conflicts.push({ kind: 'PropertyFunction', code: func, reason: `Property ${code} uses an unsupported function.` });
        if (!stat) continue;
        const statCode = stat.toLowerCase();
        const statRecord = context.itemStats.target.get(statCode);
        const statStatus = statRecord ? 'compatible-existing' : 'absent';
        dependencies.push({ kind: 'ItemStatCost', code: statCode, status: statStatus, property: code, func });
        if (!statRecord) conflicts.push({ kind: 'ItemStatCost', code: statCode, reason: `Required by ${code} func ${func}` });
      }
    }
  }
  for (const header of ITEM_TYPE_HEADERS) {
    const code = rowCell(projectedRow, targetIndexes, header).toLowerCase();
    if (!code) continue;
    const sourceType = context.sourceItemTypes.get(code);
    const targetType = context.targetItemTypes.get(code);
    const sourceSemantics = adoptsPd2(header);
    let status = 'compatible-existing';
    let reason = 'ItemType exists in BKVince.';
    if (!targetType) { status = 'absent'; reason = 'ItemType is absent from BKVince.'; }
    else if (sourceSemantics && !sourceType) { status = 'absent-source-definition'; reason = 'The adopted PD2 ItemType has no unique source definition.'; }
    else if (sourceSemantics && (sourceType.equiv1 !== targetType.equiv1 || sourceType.equiv2 !== targetType.equiv2)) {
      status = 'incompatible'; reason = 'PD2 and BKVince ItemType ancestry differs.';
    } else if (sourceSemantics) { status = 'compatible'; reason = 'PD2 and BKVince ItemType ancestry matches.'; }
    dependencies.push({ kind: 'ItemType', code, field: header, provenance: sourceSemantics ? 'PD2' : 'BKVINCE_OR_CUSTOM', status, reason });
    if (['absent', 'absent-source-definition', 'incompatible'].includes(status)) conflicts.push({ kind: 'ItemType', code, reason });
  }
  if (isMapRow(projectedRow, targetIndexes, context.targetItemTypes)) {
    conflicts.push({ kind: 'system', code: 'map', reason: 'Map affixes require a separate governed system.' });
  }
  const targetPropertyContext = { source: context.properties.target };
  const targetStatContext = {
    source: context.itemStats.target,
    target: context.itemStats.target,
    sourceIndexes: context.itemStats.targetIndexes,
    targetIndexes: context.itemStats.targetIndexes,
  };
  const serialization = propertyCodes.every((code) => context.properties.target.has(code))
    ? rowItemStatAudit(
      projectedRow,
      targetIndexes,
      targetPropertyContext,
      targetStatContext,
      `${tableName} ${kind} source ${sourceRow ?? '-'} target ${targetRow ?? '-'}`,
    )
    : { ok: false, reasons: ['serialization audit blocked by absent target Property'] };
  for (const reason of serialization.reasons) conflicts.push({ kind: 'serialization', code: 'ItemStatCost', reason });

  const skillProperties = new Set(catalog.policy.skillParamProperties.map((code) => code.toLowerCase()));
  for (let slot = 1; slot <= 3; slot += 1) {
    const code = rowCell(projectedRow, targetIndexes, `mod${slot}code`).toLowerCase();
    const property = context.properties.target.get(code);
    const functions = new Set(property?.signature.map(([func]) => func).filter(Boolean) ?? []);
    const rawParameter = rowCell(projectedRow, targetIndexes, `mod${slot}param`);
    const parameter = optionalInteger(rawParameter, `${tableName} mod${slot}param`);
    if (functions.has('10')) {
      const valid = parameter !== null && parameter >= 0 && parameter <= 20;
      dependencies.push({ kind: 'SkillTabParameter', code: rawParameter, status: valid ? 'compatible' : 'out-of-range', property: code });
      if (!valid) conflicts.push({ kind: 'SkillTabParameter', code: rawParameter, reason: 'Skill-tab parameter must be a D2R class tab ID in 0..20.' });
    }
    if (functions.has('18')) {
      const valid = parameter !== null && parameter >= 0 && parameter <= 3;
      dependencies.push({ kind: 'TimeParameter', code: rawParameter, status: valid ? 'compatible' : 'out-of-range', property: code });
      if (!valid) conflicts.push({ kind: 'TimeParameter', code: rawParameter, reason: 'Time-of-day parameter must be in 0..3.' });
    }
    if (!skillProperties.has(code) && !['9', '11', '19'].some((func) => functions.has(func))) continue;
    const sourceSemantics = kind === 'append' || adoptsPd2(`mod${slot}code`) || adoptsPd2(`mod${slot}param`);
    const sourceSkill = skillNameAt(context.sourceSkills, parameter);
    const targetSkill = skillNameAt(context.targetSkills, parameter);
    const status = !targetSkill ? 'absent' : !sourceSemantics ? 'compatible-existing' : sourceSkill === targetSkill ? 'compatible' : 'incompatible';
    dependencies.push({ kind: 'SkillParameter', code: rawParameter, provenance: sourceSemantics ? 'PD2' : 'BKVINCE_OR_CUSTOM', status, property: code, sourceSkill, targetSkill });
    if (!['compatible', 'compatible-existing'].includes(status)) conflicts.push({ kind: 'SkillParameter', code: rawParameter, reason: `Numeric skill parameter is not name-stable (${sourceSkill ?? 'missing'} / ${targetSkill ?? 'missing'}).` });
  }

  const group = rowCell(projectedRow, targetIndexes, 'group');
  let groupAudit = { group, status: 'none' };
  if (group) {
    const targetGroups = new Set(tableContext.target.table.rows.map((row) => rowCell(row, targetIndexes, 'group')).filter(Boolean));
    if (kind === 'existing' && targetRow !== null && group === cell(tableContext.target.table, targetIndexes, targetRow, 'group')) {
      groupAudit = { group, status: 'existing-compatible' };
    } else if (!targetGroups.has(group)) groupAudit = { group, status: 'free' };
    else {
      const config = TABLE_CONFIG[tableName];
      const sourceIndexes = headerIndexes(tableContext.source.table);
      const anchored = new Set();
      for (let row = 0; row < config.mappedRows; row += 1) {
        if (cell(tableContext.source.table, sourceIndexes, row, 'group') === group
          && cell(tableContext.target.table, targetIndexes, config.targetRow(row), 'group') === group) anchored.add(group);
      }
      const safe = new Set(catalog.policy.safeUnmappedGroupCollisions[tableName] ?? []);
      groupAudit = { group, status: anchored.has(group) ? 'anchored' : safe.has(group) ? 'explicit-safe' : 'incompatible-collision' };
      if (groupAudit.status === 'incompatible-collision') conflicts.push({ kind: 'Group', code: group, reason: 'Unanchored target group collision.' });
    }
  }

  const name = rowCell(projectedRow, targetIndexes, 'Name');
  const localization = {
    key: name,
    modern: context.modernLocalization.byKey.has(name) ? 'existing-compatible' : 'missing-addition-required',
    legacy: context.legacyLocalization.byKey.has(name) ? 'existing-compatible' : 'missing-addition-required',
  };
  if (kind === 'append' && tableContext.target.table.rows.some((row) => sameArrays(row, projectedRow))) {
    conflicts.push({ kind: 'duplicate', code: name, reason: 'Projected row is an exact duplicate of a BKVince row.' });
  }
  if (kind === 'append') {
    const vanillaIndexes = headerIndexes(tableContext.vanilla.table);
    const projectedIdentity = rowIdentity(projectedRow, targetIndexes);
    if (tableContext.vanilla.table.rows.some((row) => rowIdentity(row, vanillaIndexes) === projectedIdentity)) {
      conflicts.push({ kind: 'relocatedVanilla', code: name, reason: 'Source identity already exists in vanilla at another ordinal.' });
    }
  }
  return {
    status: conflicts.length ? 'blocked' : 'compatible',
    kind,
    dependencies,
    serialization,
    group: groupAudit,
    localization,
    conflicts,
  };
}

function loadLocalizationRecord(filePath, file = path.basename(filePath)) {
  const rawBuffer = fs.readFileSync(filePath);
  const raw = rawBuffer.toString('utf8');
  const entries = JSON.parse(raw.replace(/^\uFEFF/, ''));
  assert(Array.isArray(entries), `${filePath}: localization root must be an array`);
  return { file, filePath, raw, entries, sha256: sha256(rawBuffer) };
}

function localizationBaseFromRecord(record) {
  const byKey = new Map();
  for (const entry of record.entries) {
    assert(typeof entry.Key === 'string', `${record.filePath}: localization entry without Key`);
    if (!byKey.has(entry.Key)) byKey.set(entry.Key, entry);
  }
  return {
    filePath: record.filePath,
    raw: record.raw,
    entries: record.entries,
    byKey,
    sha256: record.sha256,
  };
}

function loadLocalizationBase(root) {
  return localizationBaseFromRecord(
    loadLocalizationRecord(path.join(root, 'item-nameaffixes.json'), 'item-nameaffixes.json'),
  );
}

function loadLocalizationRecords(root) {
  return fs.readdirSync(root)
    .filter((entry) => entry.toLowerCase().endsWith('.json'))
    .sort()
    .map((file) => loadLocalizationRecord(path.join(root, file), file));
}

function localizationIdsFromRecords(records) {
  const ids = new Map();
  for (const record of records) {
    for (const entry of record.entries) {
      if (!Number.isSafeInteger(entry.id)) continue;
      if (!ids.has(entry.id)) ids.set(entry.id, []);
      ids.get(entry.id).push({ file: record.file, key: entry.Key });
    }
  }
  return ids;
}

function loadLocalizationSnapshot(root) {
  const records = loadLocalizationRecords(root);
  const baseRecords = records.filter((record) => record.file.toLowerCase() === 'item-nameaffixes.json');
  assert(baseRecords.length === 1, `${root}: expected exactly one item-nameaffixes.json`);
  // Governance hash: uppercase SHA-256 of the UTF-8 JSON.stringify output for this
  // ordinal filename-sorted [{ file, sha256(raw file bytes) }] manifest.
  const manifest = records.map((record) => ({ file: record.file, sha256: record.sha256 }));
  return {
    root,
    base: localizationBaseFromRecord(baseRecords[0]),
    ids: localizationIdsFromRecords(records),
    manifest,
    manifestSha256: stableSha(manifest),
  };
}

function allLocalizationIds(root) {
  return localizationIdsFromRecords(loadLocalizationRecords(root));
}

function localizedEntry(id, key, locales) {
  return Object.fromEntries([
    ['id', id],
    ['Key', key],
    ...locales.map((locale) => [locale, key]),
  ]);
}

function appendJsonEntries(base, entries) {
  assert(base.startsWith('\uFEFF['), 'Localization baseline must retain its UTF-8 BOM');
  assert(base.endsWith('\n]'), 'Localization baseline must end at the JSON array delimiter');
  assert(!base.endsWith('\n'), 'Localization baseline must not gain a final EOL');
  if (entries.length === 0) return base;
  const serializedArray = JSON.stringify(entries, null, 2);
  const body = serializedArray.slice(2, -2);
  return `${base.slice(0, -2)},\n${body}\n]`;
}

function buildLocalization(selectedByTable, catalog) {
  const orderedKeys = uniqueValues([
    ...selectedByTable['magicprefix.txt'].map((entry) => entry.name),
    ...selectedByTable['magicsuffix.txt'].map((entry) => entry.name),
  ]);
  const modernBase = loadLocalizationBase(modernStringsRoot);
  const legacyBase = loadLocalizationBase(legacyStringsRoot);
  const localizationPolicy = catalog.policy.localization;
  assert(
    modernBase.sha256 === localizationPolicy.modernBaselineSha256,
    `Modern localization baseline hash drift ${modernBase.sha256}`,
  );
  assert(
    legacyBase.sha256 === localizationPolicy.legacyBaselineSha256,
    `Legacy localization baseline hash drift ${legacyBase.sha256}`,
  );
  assert(
    modernBase.entries.length === localizationPolicy.modernBaselineRows,
    'Modern localization baseline row count drift',
  );
  assert(
    legacyBase.entries.length === localizationPolicy.legacyBaselineRows,
    'Legacy localization baseline row count drift',
  );
  const missingModern = orderedKeys.filter((key) => !modernBase.byKey.has(key));
  const missingLegacy = orderedKeys.filter((key) => !legacyBase.byKey.has(key));
  const expected = catalog.expected.localization;
  assert(
    orderedKeys.length === expected.uniqueSelectedKeys,
    `Selected localization key count drift ${orderedKeys.length}/${expected.uniqueSelectedKeys}`,
  );
  assert(
    missingModern.length === expected.newModernKeys,
    `Modern localization gap count drift ${missingModern.length}/${expected.newModernKeys}`,
  );
  assert(
    missingLegacy.length === expected.newLegacyKeys,
    `Legacy localization gap count drift ${missingLegacy.length}/${expected.newLegacyKeys}`,
  );

  const legacyOnlyGaps = missingLegacy.filter((key) => modernBase.byKey.has(key));
  assert(
    JSON.stringify(legacyOnlyGaps) === JSON.stringify(expected.legacyOnlyGaps),
    `Legacy-only localization gaps drift: ${legacyOnlyGaps.join(',')}`,
  );

  const firstNewId = catalog.policy.localization.firstNewId;
  const assigned = new Map(missingModern.map((key, index) => [key, firstNewId + index]));
  const modernIds = allLocalizationIds(modernStringsRoot);
  const legacyIds = allLocalizationIds(legacyStringsRoot);
  for (const [key, id] of assigned) {
    assert(!modernIds.has(id), `Modern localization ID ${id} for ${key} is already used`);
    assert(!legacyIds.has(id), `Legacy localization ID ${id} for ${key} is already used`);
  }

  const modernEntries = missingModern.map((key) => localizedEntry(assigned.get(key), key, MODERN_LOCALES));
  const legacyEntries = missingLegacy.map((key) => {
    const modern = modernBase.byKey.get(key);
    const id = assigned.get(key) ?? modern.id;
    assert(Number.isSafeInteger(id), `No stable localization ID for ${key}`);
    const conflicts = legacyIds.get(id) ?? [];
    assert(conflicts.length === 0, `Legacy localization ID ${id} for ${key} is already used`);
    return localizedEntry(id, key, LEGACY_LOCALES);
  });

  const modernPath = path.join(modernStringsRoot, catalog.policy.localization.modernFile);
  const legacyPath = path.join(legacyStringsRoot, catalog.policy.localization.legacyFile);
  assert(modernPath === modernBase.filePath, 'Modern localization target must remain item-nameaffixes.json');
  assert(legacyPath === legacyBase.filePath, 'Legacy localization target must remain item-nameaffixes.json');
  const modernSerialized = appendJsonEntries(modernBase.raw, modernEntries);
  const legacySerialized = appendJsonEntries(legacyBase.raw, legacyEntries);
  assert(
    JSON.parse(modernSerialized.replace(/^\uFEFF/, '')).length
      === modernBase.entries.length + modernEntries.length,
    'Modern localization serialization is invalid',
  );
  assert(
    JSON.parse(legacySerialized.replace(/^\uFEFF/, '')).length
      === legacyBase.entries.length + legacyEntries.length,
    'Legacy localization serialization is invalid',
  );
  return {
    orderedKeys,
    missingModern,
    missingLegacy,
    legacyOnlyGaps,
    modern: {
      filePath: modernPath,
      entries: modernEntries,
      baselineRows: modernBase.entries.length,
      serialized: modernSerialized,
    },
    legacy: {
      filePath: legacyPath,
      entries: legacyEntries,
      baselineRows: legacyBase.entries.length,
      serialized: legacySerialized,
    },
  };
}

function localizationSnapshotsForPlan(context) {
  if (context === undefined || context === null) {
    return {
      modern: loadLocalizationSnapshot(modernStringsRoot),
      legacy: loadLocalizationSnapshot(legacyStringsRoot),
    };
  }
  const snapshots = context.localizationSnapshots ?? context;
  assert(
    snapshots?.modern?.base && snapshots?.modern?.ids instanceof Map
      && snapshots?.legacy?.base && snapshots?.legacy?.ids instanceof Map,
    'Localization planning context is missing the modern or legacy governed snapshot',
  );
  return snapshots;
}

export function planAffixLocalization(keys, catalog, context = null) {
  const orderedKeys = uniqueValues(keys.filter(Boolean));
  const snapshots = localizationSnapshotsForPlan(context);
  const modernBase = snapshots.modern.base;
  const legacyBase = snapshots.legacy.base;
  const modernIds = snapshots.modern.ids;
  const legacyIds = snapshots.legacy.ids;
  const reserved = new Map();
  const conflicts = [];
  let nextId = catalog.policy.localization.firstNewId;
  const idConflicts = (index, id, key) => (index.get(id) ?? []).filter((entry) => entry.key !== key);
  const allocate = (key) => {
    const modern = modernBase.byKey.get(key);
    const legacy = legacyBase.byKey.get(key);
    if (modern && legacy && modern.id !== legacy.id) {
      conflicts.push({ kind: 'Localization', code: key, reason: `Modern and legacy IDs differ (${modern.id}/${legacy.id}).` });
      return null;
    }
    let id = modern?.id ?? legacy?.id ?? null;
    if (id === null) {
      while (modernIds.has(nextId) || legacyIds.has(nextId) || reserved.has(nextId)) nextId += 1;
      id = nextId;
      nextId += 1;
    }
    if (!Number.isSafeInteger(id)) {
      conflicts.push({ kind: 'Localization', code: key, reason: 'No stable numeric localization ID is available.' });
      return null;
    }
    const collisions = [...idConflicts(modernIds, id, key), ...idConflicts(legacyIds, id, key)];
    if (collisions.length || (reserved.has(id) && reserved.get(id) !== key)) {
      conflicts.push({ kind: 'Localization', code: key, reason: `Localization ID ${id} is already used by another key.` });
      return null;
    }
    reserved.set(id, key);
    return id;
  };
  const entries = orderedKeys.map((key) => {
    const modern = modernBase.byKey.get(key);
    const legacy = legacyBase.byKey.get(key);
    let id;
    if (modern && legacy) {
      id = modern.id;
      if (modern.id !== legacy.id) conflicts.push({ kind: 'Localization', code: key, reason: `Modern and legacy IDs differ (${modern.id}/${legacy.id}).` });
    } else id = allocate(key);
    return {
      key,
      id,
      modern: modern ? { status: 'existing-compatible', id: modern.id } : { status: 'addition-planned', entry: id === null ? null : localizedEntry(id, key, MODERN_LOCALES) },
      legacy: legacy ? { status: 'existing-compatible', id: legacy.id } : { status: 'addition-planned', entry: id === null ? null : localizedEntry(id, key, LEGACY_LOCALES) },
    };
  });
  return {
    status: conflicts.length ? 'blocked' : 'planned',
    format: { encoding: 'UTF-8 BOM', lineEndings: 'LF', finalEol: false },
    dependencyHashes: {
      modern: {
        baseSha256: snapshots.modern.base.sha256,
        manifestSha256: snapshots.modern.manifestSha256,
      },
      legacy: {
        baseSha256: snapshots.legacy.base.sha256,
        manifestSha256: snapshots.legacy.manifestSha256,
      },
    },
    entries,
    conflicts,
  };
}

function assertBaselineIdentity(catalog, targets) {
  for (const [name, loaded] of Object.entries(targets)) {
    const expected = catalog.targetBaseline[name];
    assert(loaded.sha256 === expected.sha256, `${name}: BKVince baseline hash drift ${loaded.sha256}`);
    assertTargetTableShape(name, loaded, expected);
    const identity = existingIdentitySha(loaded.table, expected.physicalRows);
    if (expected.identitySha256) {
      assert(identity === expected.identitySha256, `${name}: existing ID identity drift`);
    }
  }
}

function applyChanges(prepared, catalog) {
  for (const [name, result] of Object.entries(prepared)) {
    if (!TABLE_CONFIG[name].applyRetunes && !TABLE_CONFIG[name].applyAppends) continue;
    const indexes = headerIndexes(result.outputTable);
    if (TABLE_CONFIG[name].applyRetunes) {
      for (const change of result.retunes) {
        result.outputTable.rows[change.targetRow][indexes.get(change.column.toLowerCase())] = change.after;
      }
    }
    if (TABLE_CONFIG[name].applyAppends) {
      for (const append of result.appends.selected) result.outputTable.rows.push([...append.projected]);
    }
    assert(
      existingIdentitySha(result.outputTable, catalog.targetBaseline[name].physicalRows)
        === existingIdentitySha(result.target.table, catalog.targetBaseline[name].physicalRows),
      `${name}: an existing compiled identity changed`,
    );
    assert(result.outputTable.eol === '\r\n', `${name}: output EOL drift`);
    assert(result.outputTable.hasFinalEol, `${name}: output final EOL drift`);
    assert(
      fs.readFileSync(result.target.filePath, ENCODING) === serializeTable(result.target.table),
      `${name}: target changed during preparation`,
    );
  }
}

function writeOutputs(prepared, localization) {
  const operations = [];
  for (const [name, result] of Object.entries(prepared)) {
    if (!TABLE_CONFIG[name].applyRetunes && !TABLE_CONFIG[name].applyAppends) continue;
    operations.push({
      filePath: result.target.filePath,
      write() {
        writeTable(result.target.filePath, result.outputTable);
      },
      verify() {
        const written = fs.readFileSync(result.target.filePath, ENCODING);
        assert(
          written === serializeTable(parseTable(result.target.filePath)),
          `${name}: post-write TSV round-trip failed`,
        );
        assert(
          written === serializeTable(result.outputTable),
          `${name}: written TSV differs from the prepared output`,
        );
      },
    });
  }
  for (const output of [localization.modern, localization.legacy]) {
    const temporary = `${output.filePath}.tmp`;
    operations.push({
      filePath: output.filePath,
      cleanupPaths: [temporary],
      write() {
        fs.writeFileSync(temporary, output.serialized, 'utf8');
        fs.renameSync(temporary, output.filePath);
      },
      verify() {
        const written = fs.readFileSync(output.filePath, 'utf8');
        assert(written === output.serialized, `${output.filePath}: written localization drift`);
        JSON.parse(written.replace(/^\uFEFF/, ''));
      },
    });
  }
  assert(operations.length === 4, `Expected four transactional outputs, found ${operations.length}`);
  transactionalWriteFiles(operations);
}

function summarizeColumns(changes) {
  const result = {};
  for (const change of changes) result[change.column] = (result[change.column] ?? 0) + 1;
  return result;
}

function buildSelectionFingerprint(prepared) {
  return stableSha(Object.fromEntries(Object.entries(prepared).map(([name, result]) => [name, {
    retunes: result.retunes,
    appends: result.appends.selected.map(({ projected, ...entry }) => ({
      ...entry,
      projected,
    })),
  }])));
}

function outputSummary(prepared, localization, catalog) {
  const tables = {};
  for (const [name, result] of Object.entries(prepared)) {
    const serialized = serializeTable(result.outputTable);
    tables[name] = {
      sha256: sha256(Buffer.from(serialized, ENCODING)),
      physicalRows: result.outputTable.rows.length,
      compiledRows: compiledRowCount(result.outputTable),
      identitySha256: existingIdentitySha(
        result.outputTable,
        catalog.targetBaseline[name].physicalRows,
      ),
      retuneRows: uniqueValues(result.retunes.map((change) => change.targetRow)).length,
      retuneCells: result.retunes.length,
      retunesByColumn: summarizeColumns(result.retunes),
      appendedRows: result.appends.selected.length,
      appendedProjectionSha256: stableSha(result.appends.selected.map((entry) => [
        entry.sourceRow,
        entry.projected,
      ])),
      groups: result.groups,
    };
  }
  return {
    tables,
    localization: {
      uniqueSelectedKeys: localization.orderedKeys.length,
      modernEntries: localization.modern.entries.length,
      legacyEntries: localization.legacy.entries.length,
      modernSha256: sha256(Buffer.from(localization.modern.serialized, 'utf8')),
      legacySha256: sha256(Buffer.from(localization.legacy.serialized, 'utf8')),
      legacyOnlyGaps: localization.legacyOnlyGaps,
    },
  };
}

function writeReport(reportPath, report) {
  fs.mkdirSync(path.dirname(reportPath), { recursive: true });
  fs.writeFileSync(reportPath, `${JSON.stringify(report, null, 2)}\n`, 'utf8');
}

function assertSha256Pin(value, label) {
  assert(
    typeof value === 'string' && /^[0-9A-F]{64}$/.test(value),
    `${label}: expected a pinned uppercase SHA-256, got ${String(value)}`,
  );
}

function canonicalComparable(value) {
  if (Array.isArray(value)) return value.map(canonicalComparable);
  if (value && typeof value === 'object') {
    return Object.fromEntries(
      Object.keys(value).sort().map((key) => [key, canonicalComparable(value[key])]),
    );
  }
  return value;
}

function assertSameRecord(actual, expected, label) {
  assert(
    JSON.stringify(canonicalComparable(actual)) === JSON.stringify(canonicalComparable(expected)),
    `${label}: ${JSON.stringify(actual)} != ${JSON.stringify(expected)}`,
  );
}

export function assertPinnedOutput(catalog, selectionSha256, summary) {
  const expected = catalog.expected;
  assertSha256Pin(expected.selectionSha256, 'Selection fingerprint');
  assert(
    selectionSha256 === expected.selectionSha256,
    `Selection fingerprint drift ${selectionSha256}/${expected.selectionSha256}`,
  );

  for (const [name, tableSummary] of Object.entries(summary.tables)) {
    const tableExpected = expected.final[name];
    assertSha256Pin(tableExpected, `${name} final hash`);
    assert(
      tableSummary.sha256 === tableExpected,
      `${name}: predicted final hash drift ${tableSummary.sha256}/${tableExpected}`,
    );

    const baseline = catalog.targetBaseline[name];
    const retunes = expected.retunes[name];
    const appends = expected.appends[name];
    assert(baseline && retunes && appends, `${name}: incomplete pinned expectations`);
    assert(
      tableSummary.identitySha256 === baseline.identitySha256,
      `${name}: predicted existing-ID identity drift`,
    );
    assert(tableSummary.retuneRows === retunes.rows, `${name}: predicted retune row count drift`);
    assert(tableSummary.retuneCells === retunes.cells, `${name}: predicted retune cell count drift`);
    if (retunes.byColumn) {
      assertSameRecord(tableSummary.retunesByColumn, retunes.byColumn, `${name} retunes by column`);
    }
    assert(tableSummary.appendedRows === appends.rows, `${name}: predicted append row count drift`);

    const appliedRows = TABLE_CONFIG[name].applyAppends ? appends.rows : 0;
    assert(
      tableSummary.physicalRows === baseline.physicalRows + appliedRows,
      `${name}: predicted physical row count drift`,
    );
    assert(
      tableSummary.compiledRows === baseline.compiledRows + appliedRows,
      `${name}: predicted compiled row count drift`,
    );

    if (TABLE_CONFIG[name].applyAppends) {
      assertSha256Pin(appends.projectionSha256, `${name} append projection`);
      assert(
        tableSummary.appendedProjectionSha256 === appends.projectionSha256,
        `${name}: append projection drift ${tableSummary.appendedProjectionSha256}/${appends.projectionSha256}`,
      );
      assert(
        Number.isSafeInteger(appends.firstCompiledId)
          && appends.firstCompiledId === baseline.nextCompiledId,
        `${name}: first appended compiled ID is not pinned to ${baseline.nextCompiledId}`,
      );
    }
  }

  assertSha256Pin(expected.final.modernLocalization, 'Modern localization final hash');
  assertSha256Pin(expected.final.legacyLocalization, 'Legacy localization final hash');
  assert(
    summary.localization.modernSha256 === expected.final.modernLocalization,
    'Modern localization predicted hash drift',
  );
  assert(
    summary.localization.legacySha256 === expected.final.legacyLocalization,
    'Legacy localization predicted hash drift',
  );
  assert(
    summary.localization.uniqueSelectedKeys === expected.localization.uniqueSelectedKeys,
    'Localization selected-key count drift',
  );
  assert(
    summary.localization.modernEntries === expected.localization.newModernKeys,
    'Modern localization append count drift',
  );
  assert(
    summary.localization.legacyEntries === expected.localization.newLegacyKeys,
    'Legacy localization append count drift',
  );
  assertSameRecord(
    summary.localization.legacyOnlyGaps,
    expected.localization.legacyOnlyGaps,
    'Legacy-only localization gaps',
  );
}

export function transactionalWriteFiles(operations) {
  assert(Array.isArray(operations) && operations.length > 0, 'Transactional write set is empty');
  const uniquePaths = new Set(operations.map((operation) => operation.filePath));
  assert(uniquePaths.size === operations.length, 'Transactional write paths must be unique');
  for (const operation of operations) {
    assert(typeof operation.filePath === 'string', 'Transactional write is missing a file path');
    assert(typeof operation.write === 'function', `${operation.filePath}: missing write callback`);
  }

  const originals = operations.map((operation) => ({
    filePath: operation.filePath,
    bytes: fs.readFileSync(operation.filePath),
  }));

  try {
    for (const operation of operations) {
      operation.write();
      if (operation.verify) operation.verify();
    }
  } catch (error) {
    const restoreErrors = [];
    for (const original of originals) {
      try {
        fs.writeFileSync(original.filePath, original.bytes);
        const restored = fs.readFileSync(original.filePath);
        assert(
          restored.equals(original.bytes),
          `${original.filePath}: restored bytes do not match the original`,
        );
      } catch (restoreError) {
        restoreErrors.push(new Error(`${original.filePath}: ${restoreError.message}`));
      }
    }
    for (const operation of operations) {
      for (const cleanupPath of operation.cleanupPaths ?? []) {
        try {
          if (fs.existsSync(cleanupPath)) fs.unlinkSync(cleanupPath);
        } catch (cleanupError) {
          restoreErrors.push(new Error(`${cleanupPath}: cleanup failed: ${cleanupError.message}`));
        }
      }
    }
    if (restoreErrors.length > 0) {
      throw new AggregateError(
        [error, ...restoreErrors],
        `Transactional write failed and restoration was incomplete: ${error.message}`,
      );
    }
    throw new Error(
      `Transactional write failed; all original bytes were restored: ${error.message}`,
      { cause: error },
    );
  }
}

function loadInputs(sourceRoot, catalog, resolvedTargetExcelRoot = targetExcelRoot) {
  const source = {};
  const vanilla = {};
  const target = {};
  for (const name of Object.keys(TABLE_CONFIG)) {
    source[name] = loadTable(sourceRoot, name);
    verifySourceTable(name, source[name], catalog.source.tables[name]);
    vanilla[name] = loadTable(vanillaExcelRoot, name);
    assert(
      vanilla[name].sha256 === catalog.vanillaBaseline[name],
      `${name}: vanilla baseline hash drift`,
    );
    target[name] = loadTable(resolvedTargetExcelRoot, name);
  }

  const sourceProperties = loadTable(sourceRoot, 'properties.txt');
  const sourceItemTypes = loadTable(sourceRoot, 'itemtypes.txt');
  const sourceItemStats = loadTable(sourceRoot, 'itemstatcost.txt');
  verifySourceTable('properties.txt', sourceProperties, catalog.source.tables['properties.txt']);
  verifySourceTable('itemtypes.txt', sourceItemTypes, catalog.source.tables['itemtypes.txt']);
  verifySourceTable('itemstatcost.txt', sourceItemStats, catalog.source.tables['itemstatcost.txt']);
  const targetProperties = loadTable(resolvedTargetExcelRoot, 'properties.txt');
  const targetItemTypes = loadTable(resolvedTargetExcelRoot, 'itemtypes.txt');
  const targetItemStats = loadTable(resolvedTargetExcelRoot, 'itemstatcost.txt');
  for (const [name, loaded] of Object.entries({
    'properties.txt': targetProperties,
    'itemtypes.txt': targetItemTypes,
    'itemstatcost.txt': targetItemStats,
  })) {
    assert(loaded.sha256 === catalog.targetDependencies[name], `${name}: target dependency hash drift`);
  }
  return {
    source,
    vanilla,
    target,
    sourceProperties,
    sourceItemTypes,
    sourceItemStats,
    targetProperties,
    targetItemTypes,
    targetItemStats,
  };
}

function prepareMerge(inputs, catalog) {
  assertBaselineIdentity(catalog, inputs.target);
  const properties = propertyCompatibility(
    inputs.sourceProperties.table,
    inputs.targetProperties.table,
  );
  assert(properties.compatible.size === 273, 'Compatible Property count drift');
  assert(properties.incompatible.size === 3, 'Incompatible Property count drift');
  assert(properties.missing.size === 174, 'Missing Property count drift');
  const sourceItemTypes = itemTypeIndex(inputs.sourceItemTypes.table);
  const targetItemTypes = itemTypeIndex(inputs.targetItemTypes.table);
  const itemStats = {
    source: itemStatIndex(inputs.sourceItemStats.table),
    target: itemStatIndex(inputs.targetItemStats.table),
    sourceIndexes: headerIndexes(inputs.sourceItemStats.table),
    targetIndexes: headerIndexes(inputs.targetItemStats.table),
  };
  const prepared = {};

  for (const [name, config] of Object.entries(TABLE_CONFIG)) {
    const source = inputs.source[name];
    const vanilla = inputs.vanilla[name];
    const target = inputs.target[name];
    mappedIdentityCheck(name, config, source, vanilla, target);
    const retunes = computeRetunes(
      name,
      config,
      source,
      vanilla,
      target,
      properties,
      itemStats,
      catalog,
    );
    const appends = computeAppends(
      name,
      config,
      source,
      vanilla,
      target,
      properties,
      sourceItemTypes,
      targetItemTypes,
      itemStats,
      catalog,
    );
    const groups = config.applyAppends
      ? auditGroups(name, config, source, target, appends.selected, catalog)
      : [];
    prepared[name] = {
      source,
      vanilla,
      target,
      outputTable: cloneTable(target.table),
      retunes,
      appends,
      groups,
    };
  }
  const localization = buildLocalization(Object.fromEntries(
    Object.entries(prepared).map(([name, result]) => [name, result.appends.selected]),
  ), catalog);
  applyChanges(prepared, catalog);
  return { prepared, localization, properties };
}

function checkFinal(catalog) {
  let totalCompiledAffixes = 0;
  for (const [name, expectedHash] of Object.entries(catalog.expected.final)) {
    if (!name.endsWith('.txt')) continue;
    assert(expectedHash, `${name}: final hash is not pinned in the catalog`);
    const loaded = loadTable(targetExcelRoot, name);
    assert(loaded.sha256 === expectedHash, `${name}: final hash drift ${loaded.sha256}`);
    const baseline = catalog.targetBaseline[name];
    assertTargetTableShape(name, loaded, baseline, true);
    const expectedAdded = TABLE_CONFIG[name].applyAppends ? catalog.expected.appends[name].rows : 0;
    assert(
      loaded.table.rows.length === baseline.physicalRows + expectedAdded,
      `${name}: final physical row count drift`,
    );
    assert(
      compiledRowCount(loaded.table) === baseline.compiledRows + expectedAdded,
      `${name}: final compiled ID count drift`,
    );
    const compiled = compiledRowCount(loaded.table);
    assert(compiled <= 2047, `${name}: compiled IDs exceed the 11-bit D2S field`);
    totalCompiledAffixes += compiled;
    assert(loaded.table.headers.length === 39, `${name}: target header count must remain 39`);
    assert(
      existingIdentitySha(loaded.table, baseline.physicalRows) === baseline.identitySha256,
      `${name}: existing compiled identities changed`,
    );
  }
  assert(totalCompiledAffixes <= 65535, 'Unified tooltip affix IDs exceed uint16');

  const localizationChecks = [
    [
      path.join(modernStringsRoot, catalog.policy.localization.modernFile),
      catalog.expected.final.modernLocalization,
      catalog.expected.localization.newModernKeys,
      catalog.policy.localization.modernBaselineRows,
      MODERN_LOCALES,
    ],
    [
      path.join(legacyStringsRoot, catalog.policy.localization.legacyFile),
      catalog.expected.final.legacyLocalization,
      catalog.expected.localization.newLegacyKeys,
      catalog.policy.localization.legacyBaselineRows,
      LEGACY_LOCALES,
    ],
  ];
  for (const [filePath, expectedHash, newRows, baselineRows, locales] of localizationChecks) {
    assert(expectedHash, `${filePath}: final hash is not pinned in the catalog`);
    const raw = fs.readFileSync(filePath);
    assert(sha256(raw) === expectedHash, `${filePath}: localization hash drift`);
    assert(raw.subarray(0, 3).toString('hex') === 'efbbbf', `${filePath}: UTF-8 BOM drift`);
    assert(!raw.includes(Buffer.from('\r\n')), `${filePath}: localization must remain LF`);
    assert(raw.at(-1) === 0x5d, `${filePath}: localization must keep no final EOL`);
    const entries = readJson(filePath);
    assert(entries.length === baselineRows + newRows, `${filePath}: localization row count drift`);
    const baseline = entries.slice(0, baselineRows);
    const appended = entries.slice(baselineRows);
    const baselineKeys = new Set(baseline.map((entry) => entry.Key));
    assert(new Set(appended.map((entry) => entry.Key)).size === appended.length, `${filePath}: duplicate new key`);
    assert(
      appended.every((entry) => !baselineKeys.has(entry.Key)),
      `${filePath}: a new key overrides a baseline key`,
    );
    assert(
      appended.every((entry) => locales.every((locale) => typeof entry[locale] === 'string' && entry[locale].length > 0)),
      `${filePath}: incomplete appended locale fields`,
    );
    assert(new Set(entries.map((entry) => entry.id)).size === entries.length, `${filePath}: duplicate ID`);
  }
  console.log('VALID PD2 Affixes Merge: final hashes, IDs, CRLF and localization are stable.');
}

function parseReportPath(args) {
  const option = args.find((arg) => arg.startsWith('--report='));
  return option ? path.resolve(option.slice('--report='.length)) : defaultReportPath;
}

export function run(argv = process.argv.slice(2)) {
  const catalog = readJson(catalogPath);
  const apply = argv.includes('--apply');
  const audit = argv.includes('--audit');
  const check = argv.includes('--check') || (!apply && !audit);
  assert([apply, audit, check].filter(Boolean).length === 1, 'Choose exactly one of --apply, --audit or --check');
  assert(
    !apply || catalog.status === 'implementation_approved',
    'Affix import is not approved; complete and govern the selection review first',
  );
  if (check) {
    checkFinal(catalog);
    return;
  }

  const sourceRoot = sourceRootFromArgs(argv);
  const resolvedTargetExcelRoot = targetExcelRootFromArgs(argv);
  assert(
    !apply || resolvedTargetExcelRoot === targetExcelRoot,
    '--target-excel-root is audit-only and cannot redirect writes',
  );
  const inputs = loadInputs(sourceRoot, catalog, resolvedTargetExcelRoot);
  const { prepared, localization, properties } = prepareMerge(inputs, catalog);
  const selectionSha256 = buildSelectionFingerprint(prepared);
  const summary = outputSummary(prepared, localization, catalog);
  if (apply || catalog.expected.selectionSha256) {
    assertPinnedOutput(catalog, selectionSha256, summary);
  }
  const report = {
    schemaVersion: 1,
    generatedAt: new Date().toISOString(),
    mode: apply ? 'apply' : 'audit',
    sourceRoot,
    targetExcelRoot: resolvedTargetExcelRoot,
    sourceHashes: Object.fromEntries(Object.entries(inputs.source).map(([name, value]) => [name, value.sha256])),
    propertyAudit: {
      compatible: properties.compatible.size,
      incompatible: properties.incompatible.size,
      missing: properties.missing.size,
    },
    selectionSha256,
    summary,
    details: Object.fromEntries(Object.entries(prepared).map(([name, result]) => [name, {
      retunes: result.retunes,
      appends: result.appends,
      groups: result.groups,
    }])),
    localization: {
      orderedKeys: localization.orderedKeys,
      missingModern: localization.missingModern,
      missingLegacy: localization.missingLegacy,
      legacyOnlyGaps: localization.legacyOnlyGaps,
    },
  };
  writeReport(parseReportPath(argv), report);
  if (apply) writeOutputs(prepared, localization);

  console.log(JSON.stringify({
    mode: report.mode,
    sourceRoot,
    targetExcelRoot: resolvedTargetExcelRoot,
    selectionSha256,
    summary,
    report: parseReportPath(argv),
  }, null, 2));
}

const invokedPath = process.argv[1] ? path.resolve(process.argv[1]) : '';
if (invokedPath === fileURLToPath(import.meta.url)) {
  try {
    run();
  } catch (error) {
    console.error(`INVALID PD2 Affixes Merge: ${error.message}`);
    process.exitCode = 1;
  }
}
