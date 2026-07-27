'use strict';

const fs = require('fs');
const path = require('path');
const {
  parseTable,
  serializeTable,
  writeTable,
  ENCODING,
} = require('../build-data/tsv');

const ROOT = path.resolve(__dirname, '..', '..');
const FILES = {
  misc: path.join(
    ROOT,
    'data-BKVince',
    'BKVince.mpq',
    'data',
    'global',
    'excel',
    'misc.txt',
  ),
  itemNames: path.join(
    ROOT,
    'data-BKVince',
    'BKVince.mpq',
    'data',
    'local',
    'lng',
    'strings',
    'item-names.json',
  ),
};

const CHECK_ONLY = process.argv.includes('--check');
const TEST_CODE = 'rds';
const TEST_NAME = 'Clue Scroll Test';
const TEST_STRING_ID = 74077;
const READABLE_PSPELL = '-2';
const LOCALES = [
  'enUS', 'zhTW', 'deDE', 'esES', 'frFR', 'itIT', 'koKR',
  'plPL', 'esMX', 'jaJP', 'ptBR', 'ruRU', 'zhCN',
];

function fail(message) {
  throw new Error(message);
}

function loadMisc() {
  const raw = fs.readFileSync(FILES.misc, ENCODING);
  const table = parseTable(FILES.misc);
  if (serializeTable(table) !== raw) fail('misc.txt failed its pre-write byte-exact round trip');
  if (table.eol !== '\r\n') fail('misc.txt must retain CRLF line endings');
  return table;
}

function headerIndexes(table) {
  return Object.fromEntries(table.headers.map((header, index) => [header, index]));
}

function setCell(row, indexes, header, value) {
  if (indexes[header] === undefined) fail(`misc.txt is missing the ${header} header`);
  row[indexes[header]] = value;
}

function validateTestRow(row, indexes) {
  const expected = {
    name: TEST_NAME,
    code: TEST_CODE,
    namestr: TEST_CODE,
    normcode: TEST_CODE,
    ubercode: TEST_CODE,
    ultracode: TEST_CODE,
    useable: '1',
    pSpell: READABLE_PSPELL,
    spawnable: '0',
    rarity: '0',
    PermStoreItem: '1',
    multibuy: '0',
  };
  for (const [header, value] of Object.entries(expected)) {
    if (row[indexes[header]] !== value) {
      fail(`misc.txt ${TEST_CODE}.${header} must equal ${JSON.stringify(value)}`);
    }
  }
}

function updateMisc() {
  const table = loadMisc();
  const indexes = headerIndexes(table);
  const templateRows = table.rows.filter((row) => row[indexes.code] === 'tsc');
  if (templateRows.length !== 1) fail(`misc.txt tsc template count is ${templateRows.length}`);

  const matches = table.rows.filter((row) => row[indexes.code] === TEST_CODE);
  if (matches.length > 1) fail(`misc.txt ${TEST_CODE} is duplicated`);
  if (matches.length === 0) {
    if (CHECK_ONLY) fail(`misc.txt is missing the ${TEST_CODE} test witness`);
    const row = [...templateRows[0]];
    const values = {
      name: TEST_NAME,
      code: TEST_CODE,
      namestr: TEST_CODE,
      normcode: TEST_CODE,
      ubercode: TEST_CODE,
      ultracode: TEST_CODE,
      pSpell: READABLE_PSPELL,
      spawnable: '0',
      rarity: '0',
      multibuy: '0',
    };
    for (const [header, value] of Object.entries(values)) {
      setCell(row, indexes, header, value);
    }
    table.rows.push(row);
    writeTable(FILES.misc, table);
  } else {
    validateTestRow(matches[0], indexes);
  }

  const written = fs.readFileSync(FILES.misc, ENCODING);
  const reparsed = parseTable(FILES.misc);
  if (serializeTable(reparsed) !== written) fail('misc.txt failed its post-write byte-exact round trip');
  if (reparsed.eol !== '\r\n') fail('misc.txt lost CRLF line endings');
  const reparsedIndexes = headerIndexes(reparsed);
  const finalRows = reparsed.rows.filter((row) => row[reparsedIndexes.code] === TEST_CODE);
  if (finalRows.length !== 1) fail(`misc.txt ${TEST_CODE} final count is ${finalRows.length}`);
  validateTestRow(finalRows[0], reparsedIndexes);
}

function loadJsonDocument(filePath) {
  const raw = fs.readFileSync(filePath, 'utf8');
  const hasBom = raw.charCodeAt(0) === 0xFEFF;
  const body = hasBom ? raw.slice(1) : raw;
  const eol = body.includes('\r\n') ? '\r\n' : '\n';
  return {
    raw,
    hasBom,
    eol,
    hasFinalEol: body.endsWith(eol),
    data: JSON.parse(body),
  };
}

function serializeJsonDocument(document) {
  let body = JSON.stringify(document.data, null, 2).replace(/\n/g, document.eol);
  if (document.hasFinalEol) body += document.eol;
  return (document.hasBom ? '\uFEFF' : '') + body;
}

function validateStringEntry(entry) {
  if (entry.id !== TEST_STRING_ID || entry.Key !== TEST_CODE) {
    fail(`item-names.json ${TEST_CODE} has an unexpected id or key`);
  }
  for (const locale of LOCALES) {
    if (entry[locale] !== TEST_NAME) {
      fail(`item-names.json ${TEST_CODE}.${locale} must equal ${JSON.stringify(TEST_NAME)}`);
    }
  }
}

function updateItemNames() {
  const document = loadJsonDocument(FILES.itemNames);
  if (!Array.isArray(document.data)) fail('item-names.json must contain an array');
  const ids = document.data.map((entry) => entry.id);

  const matches = document.data.filter((entry) => entry.Key === TEST_CODE);
  if (matches.length > 1) fail(`item-names.json ${TEST_CODE} is duplicated`);
  if (matches.length === 0) {
    if (CHECK_ONLY) fail(`item-names.json is missing the ${TEST_CODE} test witness`);
    if (ids.includes(TEST_STRING_ID)) fail(`item-names.json id ${TEST_STRING_ID} is already used`);
    const entry = { id: TEST_STRING_ID, Key: TEST_CODE };
    for (const locale of LOCALES) entry[locale] = TEST_NAME;
    document.data.push(entry);
    fs.writeFileSync(FILES.itemNames, serializeJsonDocument(document), 'utf8');
  } else {
    validateStringEntry(matches[0]);
  }

  const written = loadJsonDocument(FILES.itemNames);
  const finalMatches = written.data.filter((entry) => entry.Key === TEST_CODE);
  if (finalMatches.length !== 1) fail(`item-names.json ${TEST_CODE} final count is ${finalMatches.length}`);
  validateStringEntry(finalMatches[0]);
  if (serializeJsonDocument(written) !== written.raw) {
    fail('item-names.json failed its exact formatting round trip');
  }
}

try {
  updateMisc();
  updateItemNames();
  console.log(`Readable Items test witness: VALID (${CHECK_ONLY ? 'check' : 'apply'})`);
} catch (error) {
  console.error(`Readable Items test witness: INVALID (${error.message})`);
  process.exitCode = 1;
}
