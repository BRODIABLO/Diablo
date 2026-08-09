'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const {
  ENCODING,
  parseTable,
  serializeTable,
  writeTable,
} = require('../build-data/tsv');

const ROOT = path.resolve(__dirname, '..', '..');
const EXCEL = path.join(
  ROOT,
  'data-BKVince',
  'BKVince.mpq',
  'data',
  'global',
  'excel',
);
const ITEM_MODIFIERS = path.join(
  ROOT,
  'data-BKVince',
  'BKVince.mpq',
  'data',
  'local',
  'lng',
  'strings',
  'item-modifiers.json',
);
const BKVINCE_CONFIG = path.join(
  ROOT,
  'data-BKVince',
  'd2rloader',
  'config',
  'MeleeSplash.json',
);

const modeFlags = ['--apply', '--check'].filter((flag) => process.argv.includes(flag));
assert.strictEqual(modeFlags.length, 1, 'Use exactly one of --apply or --check.');
const MODE = modeFlags[0].slice(2);

const TABLES = Object.freeze({
  itemStatCost: path.join(EXCEL, 'itemstatcost.txt'),
  properties: path.join(EXCEL, 'properties.txt'),
});

const ITEM_STAT_ROWS = Object.freeze([
  Object.freeze({
    index: 391,
    key: 'inc_splash_radius',
    values: Object.freeze({
      Stat: 'inc_splash_radius',
      '*ID': '391',
      Signed: '1',
      'Send Bits': '16',
      '1.09-Save Bits': '10',
      '1.09-Save Add': '0',
      'Save Bits': '10',
      'Save Add': '0',
      descpriority: '159',
      descfunc: '19',
      descstrpos: 'ModIncSplashRadius',
      descstrneg: 'ModIncSplashRadius',
      advdisplay: '2',
      '*eol': '0',
    }),
  }),
  Object.freeze({
    index: 392,
    key: 'item_melee_splash_damage_percent',
    values: Object.freeze({
      Stat: 'item_melee_splash_damage_percent',
      '*ID': '392',
      Signed: '1',
      'Send Bits': '16',
      '1.09-Save Bits': '10',
      '1.09-Save Add': '0',
      'Save Bits': '10',
      'Save Add': '0',
      descpriority: '160',
      descfunc: '19',
      descstrpos: 'ModMeleeSplashDamagePercent',
      descstrneg: 'ModMeleeSplashDamagePercent',
      advdisplay: '2',
      '*eol': '0',
    }),
  }),
]);

const PROPERTY_ROWS = Object.freeze([
  Object.freeze({
    index: 310,
    key: 'splash-radius%',
    values: Object.freeze({
      code: 'splash-radius%',
      '*Id': '310',
      '*Enabled': '1',
      func1: '1',
      stat1: 'inc_splash_radius',
      '*Tooltip': '+#% Increased Melee Splash Radius',
      '*Min': 'Min %',
      '*Max': 'Max %',
      '*eol': '0',
    }),
  }),
  Object.freeze({
    index: 311,
    key: 'splash-dmg%',
    values: Object.freeze({
      code: 'splash-dmg%',
      '*Id': '311',
      '*Enabled': '1',
      func1: '1',
      stat1: 'item_melee_splash_damage_percent',
      '*Tooltip': '+#% Melee Splash Damage',
      '*Min': 'Min %',
      '*Max': 'Max %',
      '*eol': '0',
    }),
  }),
]);

const STRING_ROWS = Object.freeze([
  Object.freeze({
    id: 65028,
    Key: 'ModIncSplashRadius',
    enUS: '%+d%% Increased Melee Splash Radius',
  }),
  Object.freeze({
    id: 65029,
    Key: 'ModMeleeSplashDamagePercent',
    enUS: '%+d%% Melee Splash Damage',
  }),
]);

const EXPECTED_CONFIG = Object.freeze({
  enabled: false,
  activationMode: 'allEligibleMelee',
  allowNormalAttack: true,
  includedSkillIds: [],
  excludedSkillIds: [],
  requireGateStat: true,
  gateStatId: 384,
  increasedRadiusStatId: 391,
  radiusPercentPerTile: 20,
  splashDamagePercentStatId: 392,
  baseSplashDamagePercent: 100,
  baseRadiusNormalWeapon: 4,
  baseRadiusExceptionalEliteWeapon: 5,
  maximumRadiusTiles: 0,
  diagnosticLogging: false,
  skillOverrides: {},
  legacyEvent20Suppression: {
    enabled: true,
    statId: 384,
    layer: 430,
    playerAttackersOnly: true,
  },
});

function headerIndexes(table) {
  return new Map(table.headers.map((header, index) => [header, index]));
}

function getCell(table, row, header) {
  const index = headerIndexes(table).get(header);
  assert.notStrictEqual(index, undefined, `Missing column ${header}`);
  return row[index] ?? '';
}

function makeRow(table, values) {
  const indexes = headerIndexes(table);
  const row = new Array(table.headers.length).fill('');
  for (const [header, value] of Object.entries(values)) {
    const index = indexes.get(header);
    assert.notStrictEqual(index, undefined, `Missing column ${header}`);
    row[index] = String(value);
  }
  return row;
}

function rowsEqual(left, right) {
  return left.length === right.length
    && left.every((value, index) => value === right[index]);
}

function loadTable(filePath, label) {
  const raw = fs.readFileSync(filePath, ENCODING);
  const table = parseTable(filePath);
  assert.strictEqual(serializeTable(table), raw, `${label}: initial round-trip is not byte-exact`);
  assert.strictEqual(table.eol, '\r\n', `${label}: CRLF is required`);
  assert.strictEqual(table.hasFinalEol, true, `${label}: final EOL is required`);
  for (const row of table.rows) {
    assert.strictEqual(row.length, table.headers.length, `${label}: row width drift`);
  }
  return { filePath, label, raw, table };
}

function ensureAppendOnlyRow(document, definition, keyHeader, idHeader) {
  const { table, label } = document;
  const expected = makeRow(table, definition.values);
  const matches = table.rows.filter((row) => getCell(table, row, keyHeader) === definition.key);
  assert.ok(matches.length <= 1, `${label}: duplicate ${keyHeader}=${definition.key}`);

  const expectedId = String(definition.values[idHeader]);
  const idOwners = table.rows.filter((row) => getCell(table, row, idHeader) === expectedId);

  if (matches.length === 0) {
    assert.strictEqual(idOwners.length, 0, `${label}: ${idHeader}=${expectedId} is already owned`);
    assert.strictEqual(
      table.rows.length,
      definition.index,
      `${label}: ${definition.key} must be appended at runtime index ${definition.index}`,
    );
    assert.strictEqual(MODE, 'apply', `${label}: missing ${definition.key}`);
    table.rows.push(expected);
    return true;
  }

  const row = matches[0];
  assert.strictEqual(table.rows.indexOf(row), definition.index, `${label}: ${definition.key} moved`);
  assert.strictEqual(idOwners.length, 1, `${label}: ${idHeader}=${expectedId} is not unique for this addition`);
  assert.strictEqual(idOwners[0], row, `${label}: ${idHeader}=${expectedId} has another owner`);
  assert.ok(rowsEqual(row, expected), `${label}: ${definition.key} differs from the governed row`);
  return false;
}

function validateCrossReferences(itemStatCost, properties) {
  for (const definition of PROPERTY_ROWS) {
    const property = properties.table.rows[definition.index];
    const statName = getCell(properties.table, property, 'stat1');
    const statRows = itemStatCost.table.rows.filter((row) => getCell(itemStatCost.table, row, 'Stat') === statName);
    assert.strictEqual(statRows.length, 1, `properties: stat1=${statName} must resolve exactly once`);
  }
}

function validateStrings() {
  const bytes = fs.readFileSync(ITEM_MODIFIERS);
  assert.strictEqual(bytes.subarray(0, 3).toString('hex'), 'efbbbf', 'item-modifiers: UTF-8 BOM must be preserved');
  const text = bytes.toString('utf8');
  assert.strictEqual((text.match(/\r\n/g) || []).length, 0, 'item-modifiers: expected LF, not CRLF');
  assert.ok(text.endsWith('\n'), 'item-modifiers: final LF is required');
  const entries = JSON.parse(text.replace(/^\uFEFF/, ''));

  for (const expected of STRING_ROWS) {
    const idMatches = entries.filter((entry) => entry.id === expected.id);
    const keyMatches = entries.filter((entry) => entry.Key === expected.Key);
    assert.strictEqual(idMatches.length, 1, `item-modifiers: id ${expected.id} must occur once`);
    assert.strictEqual(keyMatches.length, 1, `item-modifiers: key ${expected.Key} must occur once`);
    assert.strictEqual(idMatches[0], keyMatches[0], `item-modifiers: id/key owner mismatch for ${expected.Key}`);
    assert.strictEqual(idMatches[0].enUS, expected.enUS, `item-modifiers: enUS drift for ${expected.Key}`);
  }
}

function validateConfig() {
  const actual = JSON.parse(fs.readFileSync(BKVINCE_CONFIG, 'utf8').replace(/^\uFEFF/, ''));
  assert.deepStrictEqual(actual, EXPECTED_CONFIG, 'BKVince MeleeSplash.json differs from the governed default-off profile');
}

function writeIfChanged(document) {
  const output = serializeTable(document.table);
  if (output === document.raw) return false;
  writeTable(document.filePath, document.table);
  const written = fs.readFileSync(document.filePath, ENCODING);
  const reread = parseTable(document.filePath);
  assert.strictEqual(serializeTable(reread), written, `${document.label}: written round-trip is not byte-exact`);
  assert.strictEqual(reread.eol, '\r\n', `${document.label}: written EOL drift`);
  assert.strictEqual(reread.hasFinalEol, true, `${document.label}: written final EOL drift`);
  return true;
}

function main() {
  const itemStatCost = loadTable(TABLES.itemStatCost, 'itemstatcost');
  const properties = loadTable(TABLES.properties, 'properties');
  const changed = [];

  for (const definition of ITEM_STAT_ROWS) {
    if (ensureAppendOnlyRow(itemStatCost, definition, 'Stat', '*ID')) changed.push(definition.key);
  }
  for (const definition of PROPERTY_ROWS) {
    if (ensureAppendOnlyRow(properties, definition, 'code', '*Id')) changed.push(definition.key);
  }
  validateCrossReferences(itemStatCost, properties);

  if (MODE === 'check') {
    assert.strictEqual(serializeTable(itemStatCost.table), itemStatCost.raw, 'itemstatcost: check would mutate the file');
    assert.strictEqual(serializeTable(properties.table), properties.raw, 'properties: check would mutate the file');
  } else {
    writeIfChanged(itemStatCost);
    writeIfChanged(properties);
  }

  validateStrings();
  validateConfig();

  console.log(JSON.stringify({
    mode: MODE,
    changed,
    itemStatCostRows: itemStatCost.table.rows.length,
    propertyRows: properties.table.rows.length,
    statIds: ITEM_STAT_ROWS.map(({ index, key }) => ({ id: index, stat: key })),
    propertyIds: PROPERTY_ROWS.map(({ index, key }) => ({ id: index, code: key })),
    stringIds: STRING_ROWS.map(({ id, Key }) => ({ id, key: Key })),
    configEnabled: EXPECTED_CONFIG.enabled,
  }, null, 2));
}

main();
