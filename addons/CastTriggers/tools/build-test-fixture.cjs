'use strict';

const fs = require('fs');
const path = require('path');

const workspaceRoot = path.resolve(__dirname, '..', '..', '..');
const { parseTable, serializeTable, writeTable, ENCODING } = require(
  path.join(workspaceRoot, 'scripts', 'build-data', 'tsv.js'));

const buildName = process.argv[3] || '93847';
const modName = process.argv[4] || 'CastTriggersTest';
const sourceDirectory = {
  92777: 'data-vanilla3.2',
  93787: 'data-vanilla3.3',
  93847: 'data-vanilla3.3',
}[buildName];
if (!sourceDirectory) {
  throw new Error('Build must be 92777, 93787 or 93847');
}
if (!/^[A-Za-z][A-Za-z0-9_-]{0,63}$/.test(modName)) {
  throw new Error('Mod name must use only letters, digits, underscores or hyphens');
}
const sourceRoot = process.argv[5]
  ? path.resolve(process.argv[5])
  : path.join(
    workspaceRoot,
    sourceDirectory,
    'data',
    'data',
    'global',
    'excel');
const itemModifiersSourcePath = process.argv[6]
  ? path.resolve(process.argv[6])
  : path.join(
    workspaceRoot,
    'data-BKVince',
    'BKVince.mpq',
    'data',
    'local',
    'lng',
    'strings',
    'item-modifiers.json');
const outputRoot = path.resolve(
  process.argv[2]
    || path.join(
      workspaceRoot,
      'analysis-cache',
      `cast-triggers-fixture-${buildName}`));
const modRoot = path.join(outputRoot, modName);
const mpqRoot = path.join(modRoot, `${modName}.mpq`);
const excelRoot = path.join(mpqRoot, 'data', 'global', 'excel');
const stringsRoot = path.join(mpqRoot, 'data', 'local', 'lng', 'strings');
const pluginConfigRoot = path.join(modRoot, 'd2rloader', 'config');
const pluginBinaryRoot = path.join(modRoot, 'd2rloader', 'plugins');
const packageRoot = path.join(workspaceRoot, 'addons', 'CastTriggers', 'package');

function rowObject(table, row) {
  return Object.fromEntries(table.headers.map((header, index) => [
    header,
    row[index] || '',
  ]));
}

function objectRow(table, object) {
  return table.headers.map((header) => object[header] || '');
}

function nextId(table, header) {
  const index = table.headers.indexOf(header);
  if (index < 0) throw new Error(`Missing required header: ${header}`);
  return Math.max(...table.rows.map((row) => Number(row[index]) || 0)) + 1;
}

function cloneNamedRow(table, name) {
  const row = table.rows.find((candidate) => candidate[0] === name);
  if (!row) throw new Error(`Missing required source row: ${name}`);
  return rowObject(table, row);
}

function addItemStat(table, id, stat, stringKey) {
  const row = cloneNamedRow(table, 'item_skillonattack');
  Object.assign(row, {
    Stat: stat,
    '*ID': String(id),
    itemevent1: 'doactive',
    itemeventfunc1: '20',
    itemevent2: '',
    itemeventfunc2: '',
    descstrpos: stringKey,
    descstrneg: stringKey,
  });
  table.rows.push(objectRow(table, row));
}

function addProperty(table, id, code, stat, tooltip) {
  const row = cloneNamedRow(table, 'att-skill');
  Object.assign(row, {
    code,
    '*Id': String(id),
    stat1: stat,
    '*Tooltip': tooltip,
    '*Notes': 'Cast Triggers intermod test property',
  });
  table.rows.push(objectRow(table, row));
}

function addScalarProperty(table, id, code, stat, tooltip) {
  const row = cloneNamedRow(table, 'crush');
  Object.assign(row, {
    code,
    '*Id': String(id),
    stat1: stat,
    '*Tooltip': tooltip,
    '*Notes': 'Cast Triggers laboratory-only scalar property',
  });
  table.rows.push(objectRow(table, row));
}

function addCubeRecipe(table, values) {
  const row = Object.fromEntries(table.headers.map((header) => [header, '']));
  Object.assign(row, values, { '*eol': '0' });
  table.rows.push(objectRow(table, row));
}

function assertCrLf(filePath) {
  const bytes = fs.readFileSync(filePath);
  const text = bytes.toString('latin1');
  if (!text.includes('\r\n') || /(^|[^\r])\n/.test(text)) {
    throw new Error(`Fixture table is not CRLF-only: ${filePath}`);
  }
}

function assertByteExactRoundTrip(filePath) {
  const raw = fs.readFileSync(filePath, ENCODING);
  const table = parseTable(filePath);
  if (serializeTable(table) !== raw) {
    throw new Error(`TSV round-trip is not byte-exact: ${filePath}`);
  }
  if (table.eol !== '\r\n') {
    throw new Error(`TSV is not CRLF: ${filePath}`);
  }
}

function assertAddedRow(table, idHeader, id, name) {
  if (table.rows.some((row) => row.length !== table.headers.length)) {
    throw new Error(`Fixture row width changed in table containing ${name}`);
  }
  const idIndex = table.headers.indexOf(idHeader);
  const matches = table.rows.filter((row) => (
    row[0] === name && row[idIndex] === String(id)
  ));
  if (matches.length !== 1) {
    throw new Error(`Expected one ${name} row at ID ${id}`);
  }
  const idOwners = table.rows.filter((row) => row[idIndex] === String(id));
  if (idOwners.length !== 1) {
    throw new Error(`Fixture ID ${id} collides with another row`);
  }
}

function mergeLocalizationEntries(target, expectedEntries) {
  for (const expected of expectedEntries) {
    const keyMatches = target.filter((candidate) => candidate.Key === expected.Key);
    const idMatches = target.filter((candidate) => candidate.id === expected.id);

    if (keyMatches.length > 1) {
      throw new Error(`Localization key is duplicated: ${expected.Key}`);
    }
    if (idMatches.length > 1) {
      throw new Error(`Localization ID is duplicated: ${expected.id}`);
    }
    if (keyMatches.length === 1 || idMatches.length === 1) {
      if (keyMatches.length !== 1 || idMatches.length !== 1
          || keyMatches[0] !== idMatches[0]) {
        throw new Error(
          `Localization key/ID collision: ${expected.Key} / ${expected.id}`);
      }
      for (const [field, value] of Object.entries(expected)) {
        if (keyMatches[0][field] !== value) {
          throw new Error(
            `Localization entry differs for ${expected.Key}: ${field}`);
        }
      }
      continue;
    }

    target.push(expected);
  }

  for (const expected of expectedEntries) {
    const matches = target.filter((candidate) => (
      candidate.Key === expected.Key && candidate.id === expected.id));
    if (matches.length !== 1) {
      throw new Error(
        `Expected one complete localization entry: ${expected.Key}`);
    }
  }
}

fs.mkdirSync(excelRoot, { recursive: true });
fs.mkdirSync(stringsRoot, { recursive: true });
fs.mkdirSync(pluginConfigRoot, { recursive: true });
fs.mkdirSync(pluginBinaryRoot, { recursive: true });

fs.copyFileSync(
  path.join(packageRoot, 'd2rl-ruffneckk-cast-triggers.dll'),
  path.join(pluginBinaryRoot, 'd2rl-ruffneckk-cast-triggers.dll'));

for (const name of [
  'itemstatcost.txt',
  'properties.txt',
  'cubemain.txt',
  'charstats.txt',
]) {
  const sourcePath = path.join(sourceRoot, name);
  assertByteExactRoundTrip(sourcePath);
  fs.copyFileSync(sourcePath, path.join(excelRoot, name));
}

const itemStatPath = path.join(excelRoot, 'itemstatcost.txt');
const itemStats = parseTable(itemStatPath);
const itemStatId = nextId(itemStats, '*ID');
addItemStat(
  itemStats,
  itemStatId,
  'item_skilloncast',
  'RuffnecKkCastOnCast');
addItemStat(
  itemStats,
  itemStatId + 1,
  'item_skilloncastsamelevel',
  'RuffnecKkCastOnCastSameLevel');
addItemStat(
  itemStats,
  itemStatId + 2,
  'item_skilloncritical',
  'RuffnecKkCastOnCritical');
addItemStat(
  itemStats,
  itemStatId + 3,
  'item_skilloncrushingblow',
  'RuffnecKkCastOnCrushingBlow');
addItemStat(
  itemStats,
  itemStatId + 4,
  'item_skillonopenwounds',
  'RuffnecKkCastOnOpenWounds');
addItemStat(
  itemStats,
  itemStatId + 5,
  'item_skillonattackattempt',
  'RuffnecKkCastOnAttackAttempt');
assertAddedRow(itemStats, '*ID', itemStatId, 'item_skilloncast');
assertAddedRow(
  itemStats,
  '*ID',
  itemStatId + 1,
  'item_skilloncastsamelevel');
assertAddedRow(
  itemStats,
  '*ID',
  itemStatId + 2,
  'item_skilloncritical');
assertAddedRow(
  itemStats,
  '*ID',
  itemStatId + 3,
  'item_skilloncrushingblow');
assertAddedRow(
  itemStats,
  '*ID',
  itemStatId + 4,
  'item_skillonopenwounds');
assertAddedRow(
  itemStats,
  '*ID',
  itemStatId + 5,
  'item_skillonattackattempt');
writeTable(itemStatPath, itemStats);

const propertiesPath = path.join(excelRoot, 'properties.txt');
const properties = parseTable(propertiesPath);
const propertyId = nextId(properties, '*Id');
addProperty(
  properties,
  propertyId,
  'cast-skill',
  'item_skilloncast',
  '#% Chance to cast level # [Skill] when casting a skill');
addProperty(
  properties,
  propertyId + 1,
  'cast-skill-same-level',
  'item_skilloncastsamelevel',
  '#% Chance to cast [Skill] at the triggering skill level');
addProperty(
  properties,
  propertyId + 2,
  'cast-skill-on-crit',
  'item_skilloncritical',
  '#% Chance to cast level # [Skill] on Critical Strike');
addProperty(
  properties,
  propertyId + 3,
  'cast-skill-on-cb',
  'item_skilloncrushingblow',
  '#% Chance to cast level # [Skill] on Crushing Blow');
addProperty(
  properties,
  propertyId + 4,
  'cast-skill-on-ow',
  'item_skillonopenwounds',
  '#% Chance to cast level # [Skill] on Open Wounds');
addScalarProperty(
  properties,
  propertyId + 5,
  'test-critical',
  'passive_critical_strike',
  '#% laboratory Critical Strike chance');
addProperty(
  properties,
  propertyId + 6,
  'cast-skill-on-attack',
  'item_skillonattackattempt',
  '#% Chance to cast level # [Skill] on Attack Attempt');
assertAddedRow(properties, '*Id', propertyId, 'cast-skill');
assertAddedRow(
  properties,
  '*Id',
  propertyId + 1,
  'cast-skill-same-level');
assertAddedRow(properties, '*Id', propertyId + 2, 'cast-skill-on-crit');
assertAddedRow(properties, '*Id', propertyId + 3, 'cast-skill-on-cb');
assertAddedRow(properties, '*Id', propertyId + 4, 'cast-skill-on-ow');
assertAddedRow(properties, '*Id', propertyId + 5, 'test-critical');
assertAddedRow(properties, '*Id', propertyId + 6, 'cast-skill-on-attack');
writeTable(propertiesPath, properties);

const charStatsPath = path.join(excelRoot, 'charstats.txt');
const charStats = parseTable(charStatsPath);
const sorceressRows = charStats.rows.filter((row) => row[0] === 'Sorceress');
if (sorceressRows.length !== 1) {
  throw new Error('Expected exactly one Sorceress row in CharStats.txt');
}
const sorceress = rowObject(charStats, sorceressRows[0]);
Object.assign(sorceress, {
  item3: 'vps',
  item3loc: '',
  item3count: '2',
  item3quality: '2',
  item4: 'isc',
  item4loc: '',
  item4count: '3',
  item4quality: '2',
  item5: 'box',
  item5loc: '',
  item5count: '1',
  item5quality: '2',
  item6: 'yps',
  item6loc: '',
  item6count: '1',
  item6quality: '2',
  item7: 'hp1',
  item7loc: '',
  item7count: '1',
  item7quality: '2',
  item8: 'mp1',
  item8loc: '',
  item8count: '1',
  item8quality: '2',
  item9: 'hp2',
  item9loc: '',
  item9count: '1',
  item9quality: '2',
  item10: 'mp2',
  item10loc: '',
  item10count: '1',
  item10quality: '2',
});
charStats.rows[charStats.rows.indexOf(sorceressRows[0])] = objectRow(
  charStats,
  sorceress);
if (charStats.rows.some((row) => row.length !== charStats.headers.length)) {
  throw new Error('Fixture row width changed in CharStats.txt');
}
writeTable(charStatsPath, charStats);

const cubePath = path.join(excelRoot, 'cubemain.txt');
const cube = parseTable(cubePath);
addCubeRecipe(cube, {
  description: 'Cast Triggers fixed-level test ring',
  enabled: '1',
  version: '100',
  numinputs: '1',
  'input 1': 'isc',
  output: '"rin,mag"',
  lvl: '1',
  'mod 1': 'cast-skill',
  'mod 1 param': '47',
  'mod 1 min': '100',
  'mod 1 max': '12',
});
addCubeRecipe(cube, {
  description: 'Cast Triggers same-level test ring',
  enabled: '1',
  version: '100',
  numinputs: '1',
  'input 1': 'vps',
  output: '"rin,mag"',
  lvl: '1',
  'mod 1': 'cast-skill-same-level',
  'mod 1 param': '48',
  'mod 1 min': '100',
  'mod 1 max': '63',
});
addCubeRecipe(cube, {
  description: 'Cast Triggers channel and proc-chain test ring',
  enabled: '1',
  version: '100',
  numinputs: '1',
  'input 1': 'yps',
  output: '"rin,mag"',
  lvl: '1',
  'mod 1': 'cast-skill',
  'mod 1 param': '47',
  'mod 1 min': '100',
  'mod 1 max': '12',
  'mod 2': 'oskill',
  'mod 2 param': '41',
  'mod 2 min': '10',
  'mod 2 max': '10',
  'mod 3': 'oskill',
  'mod 3 param': '53',
  'mod 3 min': '10',
  'mod 3 max': '10',
});
addCubeRecipe(cube, {
  description: 'Cast Triggers Cast on Attack Attempt test ring',
  enabled: '1',
  version: '100',
  numinputs: '1',
  'input 1': 'hp1',
  output: '"rin,mag"',
  lvl: '1',
  'mod 1': 'cast-skill-on-attack',
  'mod 1 param': '47',
  'mod 1 min': '100',
  'mod 1 max': '12',
});
addCubeRecipe(cube, {
  description: 'Cast Triggers Critical Strike test ring',
  enabled: '1',
  version: '100',
  numinputs: '1',
  'input 1': 'mp1',
  output: '"rin,mag"',
  lvl: '1',
  'mod 1': 'test-critical',
  'mod 1 min': '100',
  'mod 1 max': '100',
  'mod 2': 'cast-skill-on-crit',
  'mod 2 param': '47',
  'mod 2 min': '100',
  'mod 2 max': '12',
});
addCubeRecipe(cube, {
  description: 'Cast Triggers Deadly Strike exclusion test ring',
  enabled: '1',
  version: '100',
  numinputs: '1',
  'input 1': 'hp2',
  output: '"rin,mag"',
  lvl: '1',
  'mod 1': 'deadly',
  'mod 1 min': '100',
  'mod 1 max': '100',
  'mod 2': 'cast-skill-on-crit',
  'mod 2 param': '47',
  'mod 2 min': '100',
  'mod 2 max': '12',
});
addCubeRecipe(cube, {
  description: 'Cast Triggers Crushing Blow test ring',
  enabled: '1',
  version: '100',
  numinputs: '1',
  'input 1': 'mp2',
  output: '"rin,mag"',
  lvl: '1',
  'mod 1': 'crush',
  'mod 1 min': '100',
  'mod 1 max': '100',
  'mod 2': 'cast-skill-on-cb',
  'mod 2 param': '48',
  'mod 2 min': '100',
  'mod 2 max': '12',
});
addCubeRecipe(cube, {
  description: 'Cast Triggers Open Wounds test ring',
  enabled: '1',
  version: '100',
  numinputs: '1',
  'input 1': 'hp3',
  output: '"rin,mag"',
  lvl: '1',
  'mod 1': 'openwounds',
  'mod 1 min': '100',
  'mod 1 max': '100',
  'mod 2': 'cast-skill-on-ow',
  'mod 2 param': '44',
  'mod 2 min': '100',
  'mod 2 max': '12',
});
addCubeRecipe(cube, {
  description: 'Cast Triggers combat filtering and proc-chain test ring',
  enabled: '1',
  version: '100',
  numinputs: '1',
  'input 1': 'mp3',
  output: '"rin,mag"',
  lvl: '1',
  'mod 1': 'test-critical',
  'mod 1 min': '100',
  'mod 1 max': '100',
  'mod 2': 'cast-skill-on-crit',
  'mod 2 param': '47',
  'mod 2 min': '100',
  'mod 2 max': '12',
  'mod 3': 'cast-skill-on-cb',
  'mod 3 param': '48',
  'mod 3 min': '100',
  'mod 3 max': '12',
});
writeTable(cubePath, cube);

const localized = [
  {
    id: 199990,
    Key: 'RuffnecKkCastOnCast',
    enUS: '%d%% Chance to cast level %d %s when casting a skill',
  },
  {
    id: 199991,
    Key: 'RuffnecKkCastOnCastSameLevel',
    enUS: '%d%% Chance to cast %.*s at the source skill level when casting a skill',
  },
  {
    id: 199992,
    Key: 'RuffnecKkCastOnCritical',
    enUS: '%d%% Chance to cast level %d %s on Critical Strike',
  },
  {
    id: 199993,
    Key: 'RuffnecKkCastOnCrushingBlow',
    enUS: '%d%% Chance to cast level %d %s on Crushing Blow',
  },
  {
    id: 199994,
    Key: 'RuffnecKkCastOnOpenWounds',
    enUS: '%d%% Chance to cast level %d %s on Open Wounds',
  },
  {
    id: 199995,
    Key: 'RuffnecKkCastOnAttackAttempt',
    enUS: '%d%% Chance to cast level %d %s on Attack Attempt',
  },
].map((entry) => ({
  ...entry,
  deDE: entry.enUS,
  esES: entry.enUS,
  esMX: entry.enUS,
  frFR: entry.enUS,
  itIT: entry.enUS,
  jaJP: entry.enUS,
  koKR: entry.enUS,
  plPL: entry.enUS,
  ptBR: entry.enUS,
  ruRU: entry.enUS,
  zhCN: entry.enUS,
  zhTW: entry.enUS,
}));
const itemModifiersRaw = fs.readFileSync(itemModifiersSourcePath, 'utf8');
const itemModifiersHasBom = itemModifiersRaw.charCodeAt(0) === 0xFEFF;
const itemModifiers = JSON.parse(
  itemModifiersHasBom ? itemModifiersRaw.slice(1) : itemModifiersRaw);
if (!Array.isArray(itemModifiers)) {
  throw new Error('item-modifiers.json must contain an array');
}
mergeLocalizationEntries(itemModifiers, localized);
fs.writeFileSync(
  path.join(stringsRoot, 'item-modifiers.json'),
  `${itemModifiersHasBom ? '\uFEFF' : ''}${JSON.stringify(itemModifiers, null, 2)}\n`,
  'utf8');

fs.writeFileSync(
  path.join(mpqRoot, 'modinfo.json'),
  `${JSON.stringify({
    name: modName,
    savepath: `${modName}/`,
  }, null, 2)}\n`,
  'utf8');

fs.writeFileSync(
  path.join(pluginConfigRoot, 'ruffneckk-cast-triggers.toml'),
  [
    'enabled = true',
    '',
    '[on_cast]',
    'include_skill_ids = []',
    'exclude_skill_ids = []',
    '',
    '[while_channeling]',
    'enabled = true',
    'interval_frames = 50',
    'include_skill_ids = []',
    'exclude_skill_ids = []',
    '',
    '[combat_triggers]',
    `attack_attempt_stat_id = ${itemStatId + 5}`,
    `critical_strike_stat_id = ${itemStatId + 2}`,
    `crushing_blow_stat_id = ${itemStatId + 3}`,
    `open_wounds_stat_id = ${itemStatId + 4}`,
    '',
    '[diagnostics]',
    'enabled = true',
    '',
  ].join('\n'),
  'utf8');

for (const filePath of [
  itemStatPath,
  propertiesPath,
  cubePath,
  charStatsPath,
]) {
  assertCrLf(filePath);
  assertByteExactRoundTrip(filePath);
}

console.log(JSON.stringify({
  buildName,
  modName,
  sourceRoot,
  itemModifiersSourcePath,
  modRoot,
  itemStatIds: [
    itemStatId,
    itemStatId + 1,
    itemStatId + 2,
    itemStatId + 3,
    itemStatId + 4,
    itemStatId + 5,
  ],
  propertyIds: [
    propertyId,
    propertyId + 1,
    propertyId + 2,
    propertyId + 3,
    propertyId + 4,
    propertyId + 5,
    propertyId + 6,
  ],
  recipes: 9,
  starterItems: ['vps', 'isc', 'box', 'yps', 'hp1', 'mp1', 'hp2', 'mp2'],
}, null, 2));
