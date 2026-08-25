'use strict';

const fs = require('fs');
const path = require('path');

const workspaceRoot = path.resolve(__dirname, '..', '..', '..');
const { parseTable, writeTable } = require(
  path.join(workspaceRoot, 'scripts', 'build-data', 'tsv.js'));

const sourceRoot = path.join(
  workspaceRoot,
  'data-vanilla3.3',
  'data',
  'data',
  'global',
  'excel');
const outputRoot = path.resolve(
  process.argv[2]
    || path.join(workspaceRoot, 'analysis-cache', 'cast-triggers-fixture'));
const modRoot = path.join(outputRoot, 'CastTriggersTest');
const mpqRoot = path.join(modRoot, 'CastTriggersTest.mpq');
const excelRoot = path.join(mpqRoot, 'data', 'global', 'excel');
const stringsRoot = path.join(mpqRoot, 'data', 'local', 'lng', 'strings');
const pluginConfigRoot = path.join(modRoot, 'd2rloader', 'config');

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

fs.mkdirSync(excelRoot, { recursive: true });
fs.mkdirSync(stringsRoot, { recursive: true });
fs.mkdirSync(pluginConfigRoot, { recursive: true });

for (const name of ['itemstatcost.txt', 'properties.txt', 'cubemain.txt']) {
  fs.copyFileSync(path.join(sourceRoot, name), path.join(excelRoot, name));
}

const itemStatPath = path.join(excelRoot, 'itemstatcost.txt');
const itemStats = parseTable(itemStatPath);
const itemStatId = nextId(itemStats, '*ID');
addItemStat(
  itemStats,
  itemStatId,
  'item_skilloncast',
  'RuffnecKkCastSkill');
addItemStat(
  itemStats,
  itemStatId + 1,
  'item_skilloncastsamelevel',
  'RuffnecKkCastSkillSameLevel');
assertAddedRow(itemStats, '*ID', itemStatId, 'item_skilloncast');
assertAddedRow(
  itemStats,
  '*ID',
  itemStatId + 1,
  'item_skilloncastsamelevel');
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
assertAddedRow(properties, '*Id', propertyId, 'cast-skill');
assertAddedRow(
  properties,
  '*Id',
  propertyId + 1,
  'cast-skill-same-level');
writeTable(propertiesPath, properties);

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
  'input 1': 'tsc',
  output: '"rin,mag"',
  lvl: '1',
  'mod 1': 'cast-skill-same-level',
  'mod 1 param': '48',
  'mod 1 min': '100',
  'mod 1 max': '64',
});
writeTable(cubePath, cube);

const localized = [
  {
    id: 199990,
    Key: 'RuffnecKkCastSkill',
    enUS: '%0%% Chance to cast level %1 %2 when casting a skill',
  },
  {
    id: 199991,
    Key: 'RuffnecKkCastSkillSameLevel',
    enUS: '%0%% Chance to cast %2 at the triggering skill level when casting a skill',
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
fs.writeFileSync(
  path.join(stringsRoot, 'ruffneckk-cast-triggers.json'),
  `${JSON.stringify(localized, null, 2)}\n`,
  'utf8');

fs.writeFileSync(
  path.join(mpqRoot, 'modinfo.json'),
  `${JSON.stringify({
    name: 'CastTriggersTest',
    savepath: 'CastTriggersTest/',
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
    '[diagnostics]',
    'enabled = true',
    '',
  ].join('\n'),
  'utf8');

for (const filePath of [itemStatPath, propertiesPath, cubePath]) {
  assertCrLf(filePath);
}

console.log(JSON.stringify({
  modRoot,
  itemStatIds: [itemStatId, itemStatId + 1],
  propertyIds: [propertyId, propertyId + 1],
  recipes: 2,
}, null, 2));
