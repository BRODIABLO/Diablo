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
const BKV_EXCEL = path.join(
  ROOT,
  'data-BKVince',
  'BKVince.mpq',
  'data',
  'global',
  'excel',
);
const PD2_EXCEL = path.resolve(
  ROOT,
  '..',
  'PD2 Single PLayer',
  'PD2-Single-Player-Plus-mod-main',
  'data',
  'global',
  'excel',
);

const FILES = {
  skills: path.join(BKV_EXCEL, 'skills.txt'),
  missiles: path.join(BKV_EXCEL, 'missiles.txt'),
  states: path.join(BKV_EXCEL, 'states.txt'),
  overlay: path.join(BKV_EXCEL, 'overlay.txt'),
  skilldesc: path.join(BKV_EXCEL, 'skilldesc.txt'),
  pd2Missiles: path.join(PD2_EXCEL, 'missiles.txt'),
  pd2States: path.join(PD2_EXCEL, 'states.txt'),
  pd2Overlay: path.join(PD2_EXCEL, 'overlay.txt'),
};

const MODES = ['--apply', '--check', '--revert'].filter((mode) => process.argv.includes(mode));
assert(MODES.length === 1, 'Use exactly one of --apply, --check, or --revert.');
const MODE = MODES[0];

const SKILL_BEFORE = {
  srvdofunc: '19',
  srvmissilea: 'infernoflame1',
  srvmissileb: 'infernoflame1',
  srvmissilec: 'infernoflame1',
  auratargetstate: '',
  auralencalc: '',
  aurastat1: '',
  aurastatcalc1: '',
  cltdofunc: '24',
  cltmissilea: 'infernoflame1',
  cltmissileb: 'infernoflame2',
  calc1: 'ln12/2',
  '*calc1 desc': 'Range (Duration of Missile) (Min=1)',
  calc3: '',
  '*calc3 desc': '',
  Param1: '35',
  '*Param1 Description': 'Range baseline (Duration of missile)',
  Param2: '3',
  '*Param2 Description': 'Range per level (Duration of missile)',
  Param8: '16',
  '*Param8 Description': 'Damage synergy',
  HitShift: '3',
  EMin: '36',
  EMinLev1: '24',
  EMinLev2: '30',
  EMinLev3: '34',
  EMinLev4: '38',
  EMinLev5: '42',
  EMax: '72',
  EMaxLev1: '25',
  EMaxLev2: '31',
  EMaxLev3: '35',
  EMaxLev4: '39',
  EMaxLev5: '43',
  EDmgSymPerCalc: "(skill('Warmth'.blvl))*par8",
};

const SHRED_CALC = '"-min(lvl,100)"';
const SKILL_AFTER = {
  srvdofunc: '182',
  srvmissilea: 'infernodebuff',
  srvmissileb: '',
  srvmissilec: '',
  auratargetstate: 'inferno_debuff',
  auralencalc: '10',
  aurastat1: 'fireresist',
  aurastatcalc1: SHRED_CALC,
  cltdofunc: '111',
  cltmissilea: 'infernodebuff',
  cltmissileb: 'infernodebuff2',
  calc1: '"min((ln12/2),32)"',
  '*calc1 desc': 'PD2 missile range cap',
  calc3: '"min((2+(lvl/4)),5)"',
  '*calc3 desc': 'PD2 missile count',
  Param1: '22',
  '*Param1 Description': 'PD2 range baseline (doubled)',
  Param2: '2',
  '*Param2 Description': 'PD2 range per level (doubled)',
  Param8: '20',
  '*Param8 Description': 'PD2 damage synergy per hard point',
  HitShift: '5',
  EMin: '2',
  EMinLev1: '2',
  EMinLev2: '3',
  EMinLev3: '13',
  EMinLev4: '23',
  EMinLev5: '23',
  EMax: '3',
  EMaxLev1: '3',
  EMaxLev2: '5',
  EMaxLev3: '15',
  EMaxLev4: '25',
  EMaxLev5: '25',
  EDmgSymPerCalc: "(skill('Fire Wall'.blvl)+skill('Blaze'.blvl))*par8",
};

const SKILLDESC_BEFORE = {
  descline4: '',
  desctexta4: '',
  desccalca4: '',
  desctextb4: '',
  desccalcb4: '',
};
const SKILLDESC_AFTER = {
  descline4: '3',
  desctexta4: 'StrEnemyFireRes',
  desccalca4: SHRED_CALC,
  desctextb4: 'StrSkill23',
  desccalcb4: '',
};

const APPENDED_ROWS = {
  missiles: [
    { name: 'infernodebuff', ordinal: 756 },
    { name: 'infernodebuff2', ordinal: 757 },
    { name: 'infernotrail', ordinal: 758 },
  ],
  states: [{ name: 'inferno_debuff', ordinal: 245 }],
  overlay: [{ name: 'inferno_debuff', ordinal: 342 }],
};

function assert(condition, message) {
  if (!condition) throw new Error(message);
}

function cloneTable(table) {
  return {
    headers: [...table.headers],
    rows: table.rows.map((row) => [...row]),
    eol: table.eol,
    hasFinalEol: table.hasFinalEol,
  };
}

function load(filePath, label, expectedEol) {
  assert(fs.existsSync(filePath), `${label}: missing ${filePath}`);
  const raw = fs.readFileSync(filePath, ENCODING);
  const table = parseTable(filePath);
  assert(serializeTable(table) === raw, `${label}: initial round-trip is not byte-exact`);
  assert(table.eol === expectedEol, `${label}: unexpected EOL ${JSON.stringify(table.eol)}`);
  return { raw, table: cloneTable(table) };
}

function normalizedHeaders(table) {
  const indexes = new Map();
  table.headers.forEach((header, index) => {
    const key = header.toLowerCase();
    assert(!indexes.has(key), `Duplicate header ${header}`);
    indexes.set(key, index);
  });
  return indexes;
}

function indexOf(table, header) {
  const index = normalizedHeaders(table).get(header.toLowerCase());
  assert(index !== undefined, `Missing column ${header}`);
  return index;
}

function hasColumn(table, header) {
  return normalizedHeaders(table).has(header.toLowerCase());
}

function cell(table, row, header) {
  return row[indexOf(table, header)] ?? '';
}

function setCell(table, row, header, value) {
  row[indexOf(table, header)] = String(value);
}

function unique(table, header, value, label) {
  const rows = table.rows.filter((row) => cell(table, row, header) === value);
  assert(rows.length === 1, `${label}: expected ${header}=${value} once, found ${rows.length}`);
  return rows[0];
}

function setOwnedCells(table, keyHeader, keyValue, before, after, label, revert = false) {
  const row = unique(table, keyHeader, keyValue, label);
  const source = revert ? after : before;
  const target = revert ? before : after;
  let changed = false;
  for (const [header, targetValue] of Object.entries(target)) {
    const current = cell(table, row, header);
    const sourceValue = String(source[header]);
    assert(
      current === sourceValue || current === String(targetValue),
      `${label}: refusing divergent ${keyValue}.${header}=${JSON.stringify(current)}`,
    );
    if (current !== String(targetValue)) {
      setCell(table, row, header, targetValue);
      changed = true;
    }
  }
  return changed;
}

function mappedSourceRow(sourceTable, sourceRow, targetTable, identityHeader, identityValue, ordinal) {
  const sourceHeaders = normalizedHeaders(sourceTable);
  const row = new Array(targetTable.headers.length).fill('');
  targetTable.headers.forEach((header, targetIndex) => {
    const normalized = header.toLowerCase();
    if (normalized === '*id' || normalized === '*eol') return;
    const sourceIndex = sourceHeaders.get(normalized);
    if (sourceIndex !== undefined) row[targetIndex] = sourceRow[sourceIndex] ?? '';
  });
  setCell(targetTable, row, identityHeader, identityValue);
  setCell(targetTable, row, '*ID', ordinal);
  if (hasColumn(targetTable, '*eol')) setCell(targetTable, row, '*eol', '0');
  return row;
}

function rowsEqual(left, right) {
  return left.length === right.length && left.every((value, index) => value === right[index]);
}

function appendMappedRow(
  targetTable,
  sourceTable,
  keyHeader,
  name,
  ordinal,
  label,
) {
  const matches = targetTable.rows.filter((row) => cell(targetTable, row, keyHeader) === name);
  assert(matches.length <= 1, `${label}: duplicate ${name}`);
  const sourceRow = unique(sourceTable, keyHeader, name, `PD2 ${label}`);
  const expected = mappedSourceRow(sourceTable, sourceRow, targetTable, keyHeader, name, ordinal);
  if (matches.length === 1) {
    assert(targetTable.rows.indexOf(matches[0]) === ordinal, `${label}: ${name} moved from ${ordinal}`);
    assert(rowsEqual(matches[0], expected), `${label}: ${name} differs from the governed prototype`);
    return false;
  }
  assert(targetTable.rows.length === ordinal, `${label}: expected append ordinal ${ordinal}, got ${targetTable.rows.length}`);
  targetTable.rows.push(expected);
  return true;
}

function removeMappedRow(
  targetTable,
  sourceTable,
  keyHeader,
  name,
  ordinal,
  label,
) {
  const row = unique(targetTable, keyHeader, name, label);
  const sourceRow = unique(sourceTable, keyHeader, name, `PD2 ${label}`);
  const expected = mappedSourceRow(sourceTable, sourceRow, targetTable, keyHeader, name, ordinal);
  assert(targetTable.rows.indexOf(row) === ordinal, `${label}: ${name} moved from ${ordinal}`);
  assert(ordinal === targetTable.rows.length - 1, `${label}: ${name} is not the final row`);
  assert(rowsEqual(row, expected), `${label}: refusing to remove divergent ${name}`);
  targetTable.rows.pop();
}

function applyRows(tables, sources) {
  for (const descriptor of APPENDED_ROWS.missiles) {
    appendMappedRow(
      tables.missiles,
      sources.missiles,
      'Missile',
      descriptor.name,
      descriptor.ordinal,
      'missiles',
    );
  }
  appendMappedRow(tables.states, sources.states, 'state', 'inferno_debuff', 245, 'states');
  appendMappedRow(tables.overlay, sources.overlay, 'overlay', 'inferno_debuff', 342, 'overlay');
}

function revertRows(tables, sources) {
  removeMappedRow(tables.overlay, sources.overlay, 'overlay', 'inferno_debuff', 342, 'overlay');
  removeMappedRow(tables.states, sources.states, 'state', 'inferno_debuff', 245, 'states');
  for (const descriptor of [...APPENDED_ROWS.missiles].reverse()) {
    removeMappedRow(
      tables.missiles,
      sources.missiles,
      'Missile',
      descriptor.name,
      descriptor.ordinal,
      'missiles',
    );
  }
}

const DAMAGE_LEVEL_CAPS = [7, 8, 6, 6, Number.POSITIVE_INFINITY];

function damageAtLevel(table, row, level, prefix) {
  const base = Number(cell(table, row, prefix));
  const increments = [1, 2, 3, 4, 5].map((index) => Number(cell(table, row, `${prefix}Lev${index}`)));
  let encoded = base;
  let remaining = level - 1;
  for (let index = 0; remaining > 0; index += 1) {
    const count = Math.min(remaining, DAMAGE_LEVEL_CAPS[index]);
    encoded += count * increments[index];
    remaining -= count;
  }
  return encoded * (2 ** (Number(cell(table, row, 'HitShift')) - 8));
}

function validate(tables) {
  assert(tables.skills.rows.length === 449, 'skills: player/runtime ordinals must remain unchanged');
  const inferno = unique(tables.skills, 'skill', 'Inferno', 'skills');
  assert(tables.skills.rows.indexOf(inferno) === 41, 'Inferno runtime ordinal must remain 41');
  for (const [header, expected] of Object.entries(SKILL_AFTER)) {
    assert(cell(tables.skills, inferno, header) === expected, `Inferno.${header} mismatch`);
  }
  const retained = {
    KeepCursorStateOnKill: '1',
    ContinueCastUnselected: '1',
    ClearSelectedOnHold: '1',
    seqinput: '10',
    rightskill: '1',
    reqlevel: '6',
    startmana: '6',
    manashift: '2',
    mana: '24',
    lvlmana: '1',
    'cost mult': '384',
    'cost add': '3000',
    auralencalc: '10',
    aurarangecalc: 'ln12/4-2',
    aurastat1: 'fireresist',
    aurastatcalc1: SHRED_CALC,
  };
  for (const [header, expected] of Object.entries(retained)) {
    assert(cell(tables.skills, inferno, header) === expected, `Retained Inferno.${header} mismatch`);
  }

  const levels = [1, 10, 20, 40];
  const expectedRange = [3, 8, 13, 23];
  const expectedShred = [-1, -10, -20, -40];
  const expectedDamage = [
    [0.25, 0.375],
    [2.75, 4.25],
    [11.5, 15.5],
    [66.5, 75.5],
  ];
  levels.forEach((level, index) => {
    const linearRange = 22 + (level - 1) * 2;
    const range = Math.trunc(linearRange / 4) - 2;
    const shred = -Math.min(level, 100);
    const damage = [
      damageAtLevel(tables.skills, inferno, level, 'EMin'),
      damageAtLevel(tables.skills, inferno, level, 'EMax'),
    ];
    assert(range === expectedRange[index], `Inferno range L${level}=${range}`);
    assert(shred === expectedShred[index], `Inferno shred L${level}=${shred}`);
    assert(
      damage.every((value, damageIndex) => Math.abs(value - expectedDamage[index][damageIndex]) < 1e-9),
      `Inferno damage L${level}=${damage.join('-')}`,
    );
  });

  const description = unique(tables.skilldesc, 'skilldesc', 'inferno', 'skilldesc');
  for (const [header, expected] of Object.entries(SKILLDESC_AFTER)) {
    assert(cell(tables.skilldesc, description, header) === expected, `inferno tooltip ${header} mismatch`);
  }

  for (const descriptor of APPENDED_ROWS.missiles) {
    const row = unique(tables.missiles, 'Missile', descriptor.name, 'missiles');
    assert(tables.missiles.rows.indexOf(row) === descriptor.ordinal, `${descriptor.name} ordinal mismatch`);
  }
  assert(tables.missiles.rows.length === 759, 'missiles row count mismatch');
  assert(tables.missiles.rows.indexOf(unique(tables.missiles, 'Missile', 'infernoflame1', 'missiles')) === 60, 'infernoflame1 moved');
  assert(tables.missiles.rows.indexOf(unique(tables.missiles, 'Missile', 'infernoflame2', 'missiles')) === 61, 'infernoflame2 moved');

  const state = unique(tables.states, 'state', 'inferno_debuff', 'states');
  assert(tables.states.rows.indexOf(state) === 245, 'inferno_debuff state ordinal mismatch');
  assert(cell(tables.states, state, 'rfred') === '1', 'inferno_debuff must mark fire resistance red');
  assert(cell(tables.states, state, 'overlay1') === 'inferno_debuff', 'inferno_debuff overlay mismatch');
  assert(tables.states.rows.length === 246, 'states row count mismatch');

  const overlay = unique(tables.overlay, 'overlay', 'inferno_debuff', 'overlay');
  assert(tables.overlay.rows.indexOf(overlay) === 342, 'inferno_debuff overlay ordinal mismatch');
  assert(cell(tables.overlay, overlay, 'Filename') === 'Extra\\AuraResistFire', 'inferno_debuff overlay asset mismatch');
  assert(tables.overlay.rows.length === 343, 'overlay row count mismatch');
}

function writeChanged(loaded, tables) {
  const changed = [];
  for (const name of ['skills', 'missiles', 'states', 'overlay', 'skilldesc']) {
    const serialized = serializeTable(tables[name]);
    if (serialized !== loaded[name].raw) {
      changed.push(name);
      writeTable(FILES[name], tables[name]);
    }
  }
  for (const name of changed) {
    const raw = fs.readFileSync(FILES[name], ENCODING);
    const parsed = parseTable(FILES[name]);
    assert(serializeTable(parsed) === raw, `${name}: final round-trip is not byte-exact`);
    assert(parsed.eol === '\r\n', `${name}: final EOL is not CRLF`);
  }
  return changed;
}

function main() {
  const loaded = {
    skills: load(FILES.skills, 'skills', '\r\n'),
    missiles: load(FILES.missiles, 'missiles', '\r\n'),
    states: load(FILES.states, 'states', '\r\n'),
    overlay: load(FILES.overlay, 'overlay', '\r\n'),
    skilldesc: load(FILES.skilldesc, 'skilldesc', '\r\n'),
  };
  const sources = {
    missiles: load(FILES.pd2Missiles, 'PD2 missiles', '\n').table,
    states: load(FILES.pd2States, 'PD2 states', '\n').table,
    overlay: load(FILES.pd2Overlay, 'PD2 overlay', '\n').table,
  };
  const tables = Object.fromEntries(
    Object.entries(loaded).map(([name, value]) => [name, value.table]),
  );

  if (MODE === '--revert') {
    setOwnedCells(tables.skills, 'skill', 'Inferno', SKILL_BEFORE, SKILL_AFTER, 'skills', true);
    setOwnedCells(
      tables.skilldesc,
      'skilldesc',
      'inferno',
      SKILLDESC_BEFORE,
      SKILLDESC_AFTER,
      'skilldesc',
      true,
    );
    revertRows(tables, sources);
    const changed = writeChanged(loaded, tables);
    console.log(`mode=revert changed=${changed.join(',') || 'none'}`);
    console.log('VALID reverted PD2 Inferno shredder prototype');
    return;
  }

  setOwnedCells(tables.skills, 'skill', 'Inferno', SKILL_BEFORE, SKILL_AFTER, 'skills');
  setOwnedCells(
    tables.skilldesc,
    'skilldesc',
    'inferno',
    SKILLDESC_BEFORE,
    SKILLDESC_AFTER,
    'skilldesc',
  );
  applyRows(tables, sources);
  validate(tables);

  const changed = ['skills', 'missiles', 'states', 'overlay', 'skilldesc']
    .filter((name) => serializeTable(tables[name]) !== loaded[name].raw);
  if (MODE === '--check') {
    assert(changed.length === 0, `Migration required for: ${changed.join(', ')}`);
  } else {
    writeChanged(loaded, tables);
  }

  console.log(`mode=${MODE.slice(2)} changed=${changed.join(',') || 'none'}`);
  console.log('range=L1:3,L10:8,L20:13,L40:23');
  console.log('shred=L1:-1,L10:-10,L20:-20,L40:-40 duration=10');
  console.log('controls=KeepCursorStateOnKill,ContinueCastUnselected,ClearSelectedOnHold');
  console.log('VALID PD2 Inferno shredder prototype');
}

try {
  main();
} catch (error) {
  console.error(`INVALID PD2 Inferno shredder: ${error.message}`);
  process.exitCode = 1;
}
