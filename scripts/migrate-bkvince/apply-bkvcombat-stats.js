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
  'BKVCombat.json',
);
const modeFlags = ['--apply', '--check'].filter((flag) => process.argv.includes(flag));
assert.strictEqual(modeFlags.length, 1, 'Use exactly one of --apply or --check.');
const MODE = modeFlags[0].slice(2);

const DEFINITIONS = Object.freeze({
  itemStatCost: Object.freeze({
    filePath: path.join(EXCEL, 'itemstatcost.txt'),
    keyHeader: 'Stat',
    idHeader: '*ID',
    index: 393,
    key: 'item_crushingblow_efficiency',
    values: Object.freeze({
      Stat: 'item_crushingblow_efficiency',
      '*ID': '393',
      Signed: '1',
      'Send Bits': '16',
      '1.09-Save Bits': '10',
      '1.09-Save Add': '0',
      'Save Bits': '10',
      'Save Add': '0',
      damagerelated: '1',
      descpriority: '161',
      descfunc: '19',
      descstrpos: 'ModCrushingBlowEfficiency',
      descstrneg: 'ModCrushingBlowEfficiency',
      advdisplay: '2',
      '*eol': '0',
    }),
  }),
  properties: Object.freeze({
    filePath: path.join(EXCEL, 'properties.txt'),
    keyHeader: 'code',
    idHeader: '*Id',
    index: 312,
    key: 'crush-efficiency',
    values: Object.freeze({
      code: 'crush-efficiency',
      '*Id': '312',
      '*Enabled': '1',
      func1: '1',
      stat1: 'item_crushingblow_efficiency',
      '*Tooltip': '+#% Crushing Blow Efficiency',
      '*Min': 'Min %',
      '*Max': 'Max %',
      '*eol': '0',
    }),
  }),
});

const STRING_DEFINITION = Object.freeze({
  id: 65030,
  Key: 'ModCrushingBlowEfficiency',
  enUS: '%+d%% Crushing Blow Efficiency',
});

function indexes(table) {
  return new Map(table.headers.map((header, index) => [header, index]));
}

function cell(table, row, header) {
  const index = indexes(table).get(header);
  assert.notStrictEqual(index, undefined, `Missing column ${header}`);
  return row[index] ?? '';
}

function makeRow(table, values) {
  const row = new Array(table.headers.length).fill('');
  const byHeader = indexes(table);
  for (const [header, value] of Object.entries(values)) {
    const index = byHeader.get(header);
    assert.notStrictEqual(index, undefined, `Missing column ${header}`);
    row[index] = String(value);
  }
  return row;
}

function rowsEqual(left, right) {
  return left.length === right.length
    && left.every((value, index) => value === right[index]);
}

function load(definition, label) {
  const raw = fs.readFileSync(definition.filePath, ENCODING);
  const table = parseTable(definition.filePath);
  assert.strictEqual(serializeTable(table), raw, `${label}: initial round-trip drift`);
  assert.strictEqual(table.eol, '\r\n', `${label}: CRLF required`);
  assert.strictEqual(table.hasFinalEol, true, `${label}: final EOL required`);
  table.rows.forEach((row) => {
    assert.strictEqual(row.length, table.headers.length, `${label}: row width drift`);
  });
  return { definition, label, raw, table };
}

function ensure(document) {
  const { definition, label, table } = document;
  const expected = makeRow(table, definition.values);
  const keyMatches = table.rows.filter(
    (row) => cell(table, row, definition.keyHeader) === definition.key,
  );
  const expectedId = String(definition.values[definition.idHeader]);
  const idMatches = table.rows.filter(
    (row) => cell(table, row, definition.idHeader) === expectedId,
  );
  assert.ok(keyMatches.length <= 1, `${label}: duplicate key ${definition.key}`);
  assert.ok(idMatches.length <= 1, `${label}: duplicate ID ${expectedId}`);

  if (keyMatches.length === 0) {
    assert.strictEqual(idMatches.length, 0, `${label}: ID ${expectedId} already owned`);
    assert.strictEqual(
      table.rows.length,
      definition.index,
      `${label}: ${definition.key} must append at index ${definition.index}`,
    );
    assert.strictEqual(MODE, 'apply', `${label}: missing ${definition.key}`);
    table.rows.push(expected);
    return true;
  }

  const row = keyMatches[0];
  assert.strictEqual(table.rows.indexOf(row), definition.index, `${label}: row moved`);
  assert.strictEqual(idMatches.length, 1, `${label}: expected one ID owner`);
  assert.strictEqual(idMatches[0], row, `${label}: key/ID owner mismatch`);
  assert.ok(rowsEqual(row, expected), `${label}: governed row drift`);
  return false;
}

function writeIfChanged(document) {
  const output = serializeTable(document.table);
  if (output === document.raw) return false;
  writeTable(document.definition.filePath, document.table);
  const written = fs.readFileSync(document.definition.filePath, ENCODING);
  const reread = parseTable(document.definition.filePath);
  assert.strictEqual(serializeTable(reread), written, `${document.label}: written drift`);
  assert.strictEqual(reread.eol, '\r\n', `${document.label}: written EOL drift`);
  assert.strictEqual(reread.hasFinalEol, true, `${document.label}: written final EOL drift`);
  return true;
}

function validateStrings() {
  const bytes = fs.readFileSync(ITEM_MODIFIERS);
  assert.strictEqual(
    bytes.subarray(0, 3).toString('hex'),
    'efbbbf',
    'item-modifiers: UTF-8 BOM must be preserved',
  );
  const text = bytes.toString('utf8');
  assert.strictEqual(
    (text.match(/\r\n/g) || []).length,
    0,
    'item-modifiers: LF is required',
  );
  assert.ok(text.endsWith('\n'), 'item-modifiers: final LF is required');
  const entries = JSON.parse(text.replace(/^\uFEFF/, ''));
  const idMatches = entries.filter(({ id }) => id === STRING_DEFINITION.id);
  const keyMatches = entries.filter(({ Key }) => Key === STRING_DEFINITION.Key);
  assert.strictEqual(idMatches.length, 1, 'item-modifiers: string ID 65030 must occur once');
  assert.strictEqual(keyMatches.length, 1, 'item-modifiers: CBE key must occur once');
  assert.strictEqual(idMatches[0], keyMatches[0], 'item-modifiers: CBE id/key owner mismatch');
  assert.strictEqual(idMatches[0].enUS, STRING_DEFINITION.enUS, 'item-modifiers: CBE enUS drift');
}

function validateConfig() {
  const config = JSON.parse(
    fs.readFileSync(BKVINCE_CONFIG, 'utf8').replace(/^\uFEFF/, ''),
  );
  assert.strictEqual(config.schemaVersion, 2, 'BKVCombat: schemaVersion must be 2');
  assert.strictEqual(config.enabled, false, 'BKVCombat: BKVince profile must ship default-off');
  assert.strictEqual(
    config.stats?.crushingBlowEfficiencyStatId,
    393,
    'BKVCombat: BKVince CBE stat ID must be 393',
  );
}

function main() {
  const documents = Object.entries(DEFINITIONS).map(([label, definition]) => (
    load(definition, label)
  ));
  const changed = documents.filter(ensure).map(({ label }) => label);
  const statDocument = documents.find(({ label }) => label === 'itemStatCost');
  const propertyDocument = documents.find(({ label }) => label === 'properties');
  const propertyRow = propertyDocument.table.rows[DEFINITIONS.properties.index];
  const statName = cell(propertyDocument.table, propertyRow, 'stat1');
  assert.strictEqual(
    statDocument.table.rows.filter((row) => cell(statDocument.table, row, 'Stat') === statName).length,
    1,
    'properties.stat1 must resolve exactly once',
  );

  if (MODE === 'check') {
    documents.forEach((document) => {
      assert.strictEqual(
        serializeTable(document.table),
        document.raw,
        `${document.label}: check would mutate the file`,
      );
    });
  } else {
    documents.forEach(writeIfChanged);
  }

  validateStrings();
  validateConfig();

  console.log(JSON.stringify({
    mode: MODE,
    changed,
    statId: 393,
    propertyId: 312,
    stringId: STRING_DEFINITION.id,
    bkvinceConfigDefaultOff: true,
  }, null, 2));
}

main();
