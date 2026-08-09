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
  skills: path.join(EXCEL, 'skills.txt'),
  missiles: path.join(EXCEL, 'missiles.txt'),
  states: path.join(EXCEL, 'states.txt'),
  monStats: path.join(EXCEL, 'monstats.txt'),
  uniqueItems: path.join(EXCEL, 'uniqueitems.txt'),
  treasureClassEx: path.join(EXCEL, 'treasureclassex.txt'),
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
  enabled: true,
  activationMode: 'allEligibleMelee',
  allowNormalAttack: true,
  includedSkillIds: [],
  excludedSkillIds: [],
  requireGateStat: false,
  gateStatId: -1,
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
    enabled: false,
    statId: -1,
    layer: -1,
    playerAttackersOnly: true,
  },
});

const LEGACY_SPLASH = Object.freeze({
  itemStat: 'item_splashonhit',
  itemStatId: '384',
  property: 'splash',
  propertyId: '302',
  playerSkill: 'Splash',
  playerSkillId: '430',
  summonSkill: 'Summon Splash',
  summonSkillId: '432',
  missile: 'proc_splashdamage',
  missileId: '743',
  state: 'splashdamage',
  stateId: '242',
  unique: "Titan's Echo",
});

function headerIndexes(table) {
  return new Map(table.headers.map((header, index) => [header, index]));
}

function getCell(table, row, header) {
  const index = headerIndexes(table).get(header);
  assert.notStrictEqual(index, undefined, `Missing column ${header}`);
  return row[index] ?? '';
}

function setCell(table, row, header, value) {
  const index = headerIndexes(table).get(header);
  assert.notStrictEqual(index, undefined, `Missing column ${header}`);
  const next = String(value);
  if ((row[index] ?? '') === next) return false;
  row[index] = next;
  return true;
}

function findUniqueRow(document, header, value) {
  const matches = document.table.rows.filter((row) => getCell(document.table, row, header) === value);
  assert.strictEqual(matches.length, 1, `${document.label}: expected one ${header}=${value}`);
  return matches[0];
}

function clearRowExcept(document, row, keepHeaders) {
  const keep = new Set(keepHeaders);
  let changed = false;
  for (const header of document.table.headers) {
    if (!keep.has(header)) changed = setCell(document.table, row, header, '') || changed;
  }
  if (keep.has('*eol')) changed = setCell(document.table, row, '*eol', '0') || changed;
  return changed;
}

function compactPairedSlots(document, row, skillHeaders, calcHeaders, removedSkill) {
  const pairs = skillHeaders.map((header, index) => ({
    skill: getCell(document.table, row, header),
    calc: getCell(document.table, row, calcHeaders[index]),
  }));
  const removed = pairs.filter(({ skill }) => skill === removedSkill).length;
  if (removed === 0) return { changed: false, removed: 0 };
  const kept = pairs.filter(({ skill }) => skill && skill !== removedSkill);
  let changed = false;
  for (let index = 0; index < skillHeaders.length; index += 1) {
    changed = setCell(document.table, row, skillHeaders[index], kept[index]?.skill ?? '') || changed;
    changed = setCell(document.table, row, calcHeaders[index], kept[index]?.calc ?? '') || changed;
  }
  return { changed, removed };
}

function retireLegacySplash(documents) {
  const changed = [];

  const itemStat = findUniqueRow(documents.itemStatCost, 'Stat', LEGACY_SPLASH.itemStat);
  assert.strictEqual(getCell(documents.itemStatCost.table, itemStat, '*ID'), LEGACY_SPLASH.itemStatId);
  let itemStatChanged = false;
  for (const header of [
    'fCallback', 'damagerelated', 'itemevent1', 'itemeventfunc1',
    'descpriority', 'descfunc', 'descval', 'descstrpos', 'descstrneg', 'advdisplay',
  ]) {
    itemStatChanged = setCell(documents.itemStatCost.table, itemStat, header, '') || itemStatChanged;
  }
  if (itemStatChanged) changed.push('retire-item_splashonhit-event');

  const property = findUniqueRow(documents.properties, 'code', LEGACY_SPLASH.property);
  assert.strictEqual(getCell(documents.properties.table, property, '*Id'), LEGACY_SPLASH.propertyId);
  let propertyChanged = clearRowExcept(
    documents.properties,
    property,
    ['code', '*Id', '*Enabled', '*eol'],
  );
  propertyChanged = setCell(documents.properties.table, property, '*Enabled', '0') || propertyChanged;
  if (propertyChanged) changed.push('retire-splash-property');

  for (const [skill, id] of [
    [LEGACY_SPLASH.playerSkill, LEGACY_SPLASH.playerSkillId],
    [LEGACY_SPLASH.summonSkill, LEGACY_SPLASH.summonSkillId],
  ]) {
    const row = findUniqueRow(documents.skills, 'skill', skill);
    assert.strictEqual(getCell(documents.skills.table, row, '*Id'), id);
    if (clearRowExcept(documents.skills, row, ['skill', '*Id', '*eol'])) {
      changed.push(`retire-skill-${id}`);
    }
  }

  const missile = findUniqueRow(documents.missiles, 'Missile', LEGACY_SPLASH.missile);
  assert.strictEqual(getCell(documents.missiles.table, missile, '*ID'), LEGACY_SPLASH.missileId);
  if (clearRowExcept(documents.missiles, missile, ['Missile', '*ID', '*eol'])) {
    changed.push('retire-proc_splashdamage-missile');
  }

  const state = findUniqueRow(documents.states, 'state', LEGACY_SPLASH.state);
  assert.strictEqual(getCell(documents.states.table, state, '*ID'), LEGACY_SPLASH.stateId);
  if (clearRowExcept(documents.states, state, ['state', '*ID', '*eol'])) {
    changed.push('retire-splashdamage-state');
  }

  const summonHeaders = Array.from({ length: 5 }, (_, index) => `sumskill${index + 1}`);
  const summonCalcHeaders = Array.from({ length: 5 }, (_, index) => `sumsk${index + 1}calc`);
  let skillReferencesRemoved = 0;
  for (const row of documents.skills.table.rows) {
    const result = compactPairedSlots(
      documents.skills,
      row,
      summonHeaders,
      summonCalcHeaders,
      LEGACY_SPLASH.summonSkill,
    );
    skillReferencesRemoved += result.removed;
  }
  if (skillReferencesRemoved) changed.push(`remove-${skillReferencesRemoved}-summon-skill-references`);

  const monsterSkillHeaders = Array.from({ length: 8 }, (_, index) => `Skill${index + 1}`);
  const monsterModeHeaders = Array.from({ length: 8 }, (_, index) => `Sk${index + 1}mode`);
  const monsterLevelHeaders = Array.from({ length: 8 }, (_, index) => `Sk${index + 1}lvl`);
  let monsterReferencesRemoved = 0;
  for (const row of documents.monStats.table.rows) {
    const skills = monsterSkillHeaders.map((header, index) => ({
      skill: getCell(documents.monStats.table, row, header),
      mode: getCell(documents.monStats.table, row, monsterModeHeaders[index]),
      level: getCell(documents.monStats.table, row, monsterLevelHeaders[index]),
    }));
    const removed = skills.filter(({ skill }) => skill === LEGACY_SPLASH.summonSkill).length;
    monsterReferencesRemoved += removed;
    if (removed === 0) continue;
    const kept = skills.filter(({ skill }) => skill && skill !== LEGACY_SPLASH.summonSkill);
    for (let index = 0; index < monsterSkillHeaders.length; index += 1) {
      setCell(documents.monStats.table, row, monsterSkillHeaders[index], kept[index]?.skill ?? '');
      setCell(documents.monStats.table, row, monsterModeHeaders[index], kept[index]?.mode ?? '');
      setCell(documents.monStats.table, row, monsterLevelHeaders[index], kept[index]?.level ?? '');
    }
  }
  if (monsterReferencesRemoved) changed.push(`remove-${monsterReferencesRemoved}-monster-skill-references`);

  const unique = findUniqueRow(documents.uniqueItems, 'index', LEGACY_SPLASH.unique);
  let uniqueChanged = setCell(documents.uniqueItems.table, unique, 'spawnable', '0');
  for (const header of ['prop1', 'par1', 'min1', 'max1']) {
    uniqueChanged = setCell(documents.uniqueItems.table, unique, header, '') || uniqueChanged;
  }
  if (uniqueChanged) changed.push('retire-titans-echo');

  const itemHeaders = Array.from({ length: 10 }, (_, index) => `Item${index + 1}`);
  const probabilityHeaders = Array.from({ length: 10 }, (_, index) => `Prob${index + 1}`);
  let treasureReferencesRemoved = 0;
  for (const row of documents.treasureClassEx.table.rows) {
    const result = compactPairedSlots(
      documents.treasureClassEx,
      row,
      itemHeaders,
      probabilityHeaders,
      LEGACY_SPLASH.unique,
    );
    treasureReferencesRemoved += result.removed;
  }
  if (treasureReferencesRemoved) changed.push(`remove-${treasureReferencesRemoved}-treasure-reference`);

  assert.ok(
    !documents.skills.table.rows.some((row) => summonHeaders.some(
      (header) => getCell(documents.skills.table, row, header) === LEGACY_SPLASH.summonSkill,
    )),
    'skills: legacy Summon Splash reference remains',
  );
  assert.ok(
    !documents.monStats.table.rows.some((row) => monsterSkillHeaders.some(
      (header) => getCell(documents.monStats.table, row, header) === LEGACY_SPLASH.summonSkill,
    )),
    'monstats: legacy Summon Splash reference remains',
  );
  assert.ok(
    !documents.treasureClassEx.table.rows.some((row) => itemHeaders.some(
      (header) => getCell(documents.treasureClassEx.table, row, header) === LEGACY_SPLASH.unique,
    )),
    "treasureclassex: Titan's Echo reference remains",
  );

  return changed;
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
  assert.deepStrictEqual(actual, EXPECTED_CONFIG, 'BKVince MeleeSplash.json differs from the governed active profile');
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
  const documents = Object.fromEntries(Object.entries(TABLES).map(([key, filePath]) => (
    [key, loadTable(filePath, key)]
  )));
  const { itemStatCost, properties } = documents;
  const changed = [];

  for (const definition of ITEM_STAT_ROWS) {
    if (ensureAppendOnlyRow(itemStatCost, definition, 'Stat', '*ID')) changed.push(definition.key);
  }
  for (const definition of PROPERTY_ROWS) {
    if (ensureAppendOnlyRow(properties, definition, 'code', '*Id')) changed.push(definition.key);
  }
  validateCrossReferences(itemStatCost, properties);
  changed.push(...retireLegacySplash(documents));

  if (MODE === 'check') {
    for (const document of Object.values(documents)) {
      assert.strictEqual(
        serializeTable(document.table),
        document.raw,
        `${document.label}: check would mutate the file`,
      );
    }
  } else {
    for (const document of Object.values(documents)) writeIfChanged(document);
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
    legacySplashRetired: true,
  }, null, 2));
}

main();
