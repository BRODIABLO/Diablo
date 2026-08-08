'use strict';

const path = require('path');
const {
  parseTable,
  serializeTable,
  writeTable,
  ENCODING,
} = require('../build-data/tsv');
const fs = require('fs');

const ROOT = path.resolve(__dirname, '..', '..');
const EXCEL = path.join(
  ROOT,
  'data-BKVince',
  'BKVince.mpq',
  'data',
  'global',
  'excel',
);

const FILES = {
  skills: path.join(EXCEL, 'skills.txt'),
  states: path.join(EXCEL, 'states.txt'),
  overlay: path.join(EXCEL, 'overlay.txt'),
};

const MODE = process.argv.includes('--apply')
  ? 'apply'
  : process.argv.includes('--check')
    ? 'check'
    : process.argv.includes('--rollback')
      ? 'rollback'
      : '';

assert(MODE, 'Use exactly one of --apply, --check, or --rollback.');
assert(
  ['--apply', '--check', '--rollback'].filter((flag) => process.argv.includes(flag)).length === 1,
  'Use exactly one mode.',
);

function assert(condition, message) {
  if (!condition) throw new Error(message);
}

function load(filePath, label) {
  const raw = fs.readFileSync(filePath, ENCODING);
  const table = parseTable(filePath);
  assert(serializeTable(table) === raw, `${label}: initial round-trip is not byte-exact`);
  assert(table.eol === '\r\n', `${label}: expected CRLF`);
  return { raw, table };
}

function headerMap(table) {
  return new Map(table.headers.map((header, index) => [header, index]));
}

function cell(table, row, header) {
  const index = headerMap(table).get(header);
  assert(index !== undefined, `Missing column ${header}`);
  return row[index] ?? '';
}

function setCell(table, row, header, value) {
  const index = headerMap(table).get(header);
  assert(index !== undefined, `Missing column ${header}`);
  row[index] = String(value);
}

function rowFrom(table, values) {
  const row = new Array(table.headers.length).fill('');
  for (const [header, value] of Object.entries(values)) setCell(table, row, header, value);
  return row;
}

function unique(table, header, value, label) {
  const rows = table.rows.filter((row) => cell(table, row, header) === value);
  assert(rows.length === 1, `${label}: expected ${header}=${value} once, found ${rows.length}`);
  return rows[0];
}

function setOwnedCell(table, row, header, baseline, expected, label) {
  const current = cell(table, row, header);
  assert(
    current === baseline || current === expected,
    `${label}.${header}: unexpected value ${JSON.stringify(current)}`,
  );
  setCell(table, row, header, MODE === 'rollback' ? baseline : expected);
}

function appendOwnedRow(table, keyHeader, keyValue, values, label) {
  const matches = table.rows.filter((row) => cell(table, row, keyHeader) === keyValue);
  assert(matches.length <= 1, `${label}: duplicate ${keyHeader}=${keyValue}`);
  if (MODE === 'rollback') {
    if (matches.length === 0) return;
    for (const [header, expected] of Object.entries(values)) {
      assert(
        cell(table, matches[0], header) === String(expected),
        `${label}: refusing to remove divergent ${keyValue}.${header}`,
      );
    }
    table.rows.splice(table.rows.indexOf(matches[0]), 1);
    return;
  }
  if (matches.length === 0) {
    table.rows.push(rowFrom(table, values));
    return;
  }
  for (const [header, expected] of Object.entries(values)) {
    assert(
      cell(table, matches[0], header) === String(expected),
      `${label}: ${keyValue}.${header} differs`,
    );
  }
}

function applySkills(table) {
  const row = unique(table, 'skill', 'Static Field', 'skills');
  const changes = {
    srvdofunc: ['20', '20'],
    aurafilter: ['34691', '34691'],
    auratargetstate: ['', 'staticfield_debuff'],
    auralencalc: ['', "125 + (5 * skill('Lightning Mastery'.blvl))"],
    aurarangecalc: ['ln12', '"min(ln12 / 2, 14)"'],
    aurastat1: ['', 'lightresist'],
    aurastatcalc1: ['', '"-min(lvl, 100)"'],
    Param1: ['5', '8'],
    Param2: ['1', '1'],
    Param3: ['0', '0'],
    Param4: ['25', '25'],
  };
  for (const [header, [baseline, expected]] of Object.entries(changes)) {
    setOwnedCell(table, row, header, baseline, expected, 'Static Field');
  }
}

function applyStates(table) {
  const idOwners = table.rows.filter((row) => cell(table, row, '*ID') === '246');
  assert(
    idOwners.length === 0 || (idOwners.length === 1 && cell(table, idOwners[0], 'state') === 'staticfield_debuff'),
    `states: *ID=246 is owned by ${idOwners.map((row) => cell(table, row, 'state')).join(',')}`,
  );
  appendOwnedRow(table, 'state', 'staticfield_debuff', {
    state: 'staticfield_debuff',
    '*ID': '246',
    overlay1: 'staticfield_debuff',
    '*eol': '0',
  }, 'states');
}

function applyOverlay(table) {
  const idOwners = table.rows.filter((row) => cell(table, row, '*ID') === '343');
  assert(
    idOwners.length === 0 || (idOwners.length === 1 && cell(table, idOwners[0], 'overlay') === 'staticfield_debuff'),
    `overlay: *ID=343 is owned by ${idOwners.map((row) => cell(table, row, 'overlay')).join(',')}`,
  );
  appendOwnedRow(table, 'overlay', 'staticfield_debuff', {
    overlay: 'staticfield_debuff',
    '*ID': '343',
    Filename: 'ShockHitSm',
    version: '100',
    '*Frames': '20',
    Character: 'all',
    PreDraw: '0',
    '1ofN': '1',
    Xoffset: '0',
    Yoffset: '-36',
    Height1: '14',
    Height2: '0',
    Height3: '-14',
    Height4: '-60',
    AnimRate: '16',
    LoopWaitTime: '0',
    Trans: '3',
    InitRadius: '0',
    Radius: '0',
    Red: '20',
    Green: '20',
    Blue: '20',
    NumDirections: '1',
    LocalBlood: '0',
  }, 'overlay');
}

function validateProgression() {
  const radius = (level) => Math.min(Math.trunc((8 + level - 1) / 2), 14);
  const resistance = (level) => -Math.min(level, 100);
  assert(radius(1) === 4, 'radius level 1 must be 4');
  assert(radius(10) === 8, 'radius level 10 must be 8');
  assert(radius(20) === 13, 'radius level 20 must be 13');
  assert(radius(40) === 14, 'radius level 40 must be 14');
  assert(resistance(1) === -1, 'resistance level 1 must be -1');
  assert(resistance(10) === -10, 'resistance level 10 must be -10');
  assert(resistance(20) === -20, 'resistance level 20 must be -20');
  assert(resistance(40) === -40, 'resistance level 40 must be -40');
  assert(125 + (5 * 20) === 225, 'duration with Lightning Mastery level 20 must be 225');
}

const loads = Object.fromEntries(
  Object.entries(FILES).map(([label, filePath]) => [label, load(filePath, label)]),
);

applySkills(loads.skills.table);
applyStates(loads.states.table);
applyOverlay(loads.overlay.table);
validateProgression();

if (MODE === 'apply' || MODE === 'rollback') {
  for (const [label, filePath] of Object.entries(FILES)) {
    writeTable(filePath, loads[label].table);
    const reread = parseTable(filePath);
    assert(serializeTable(reread) === fs.readFileSync(filePath, ENCODING), `${label}: written round-trip failed`);
  }
}

if (MODE === 'check') {
  for (const [label, loadResult] of Object.entries(loads)) {
    assert(
      serializeTable(loadResult.table) === loadResult.raw,
      `${label}: checked file does not contain the expected Static Field rework`,
    );
  }
}

console.log(`Static Field rework ${MODE}: OK`);
