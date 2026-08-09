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
  missiles: path.join(EXCEL, 'missiles.txt'),
  skilldesc: path.join(EXCEL, 'skilldesc.txt'),
};

const MODES = ['--apply', '--check', '--revert'].filter((mode) => process.argv.includes(mode));
assert(MODES.length === 1, 'Use exactly one of --apply, --check, or --revert.');
const MODE = MODES[0];

const SKILL_BEFORE = {
  localdelay: '45',
  manashift: '8',
  mana: '23',
  Param1: '7',
  Param8: '5',
  EMin: '45',
  EMinLev1: '15',
  EMinLev2: '30',
  EMinLev3: '45',
  EMinLev4: '55',
  EMinLev5: '65',
  EMax: '75',
  EMaxLev1: '16',
  EMaxLev2: '31',
  EMaxLev3: '46',
  EMaxLev4: '56',
  EMaxLev5: '66',
  EDmgSymPerCalc:
    "(skill('Ice Bolt'.blvl)+skill('Ice Blast'.blvl)+skill('Glacial Spike'.blvl))*par8",
};

const SKILL_AFTER = {
  localdelay: '23',
  manashift: '7',
  mana: '26',
  Param1: '8',
  Param8: '12',
  EMin: '17',
  EMinLev1: '5',
  EMinLev2: '6',
  EMinLev3: '8',
  EMinLev4: '10',
  EMinLev5: '14',
  EMax: '24',
  EMaxLev1: '7',
  EMaxLev2: '8',
  EMaxLev3: '11',
  EMaxLev4: '14',
  EMaxLev5: '19',
  EDmgSymPerCalc: "(skill('Ice Bolt'.blvl)+skill('Ice Blast'.blvl))*par8",
};

const SKILLDESC_BEFORE = {
  dsc2calca2: '45',
  dsc3line4: '76',
  dsc3texta4: 'Colddplev',
  dsc3textb4: 'skillname55',
  dsc3calca4: 'par8',
};

const SKILLDESC_AFTER = {
  dsc2calca2: '23',
  dsc3line4: '',
  dsc3texta4: '',
  dsc3textb4: '',
  dsc3calca4: '',
};

const CENTER_BEFORE = { Range: '100' };
const CENTER_AFTER = { Range: '50' };
const SHARD_BEFORE = { Size: '2' };
const SHARD_AFTER = { Size: '3' };
const SHARDS = ['blizzard1', 'blizzard2', 'blizzard3', 'blizzard4'];

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

function load(filePath, label) {
  assert(fs.existsSync(filePath), `${label}: missing ${filePath}`);
  const raw = fs.readFileSync(filePath, ENCODING);
  const table = parseTable(filePath);
  assert(serializeTable(table) === raw, `${label}: initial round-trip is not byte-exact`);
  assert(table.eol === '\r\n', `${label}: expected CRLF, got ${JSON.stringify(table.eol)}`);
  return { raw, table: cloneTable(table) };
}

function headerIndexes(table) {
  const indexes = new Map();
  table.headers.forEach((header, index) => {
    const key = header.toLowerCase();
    assert(!indexes.has(key), `Duplicate header ${header}`);
    indexes.set(key, index);
  });
  return indexes;
}

function indexOf(table, header) {
  const index = headerIndexes(table).get(header.toLowerCase());
  assert(index !== undefined, `Missing column ${header}`);
  return index;
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

const DAMAGE_LEVEL_CAPS = [7, 8, 6, 6, Number.POSITIVE_INFINITY];

function damageAtLevel(table, row, level, prefix) {
  const base = Number(cell(table, row, prefix));
  const increments = [1, 2, 3, 4, 5].map((index) =>
    Number(cell(table, row, `${prefix}Lev${index}`)),
  );
  let encoded = base;
  let remaining = level - 1;
  for (let index = 0; remaining > 0; index += 1) {
    const count = Math.min(remaining, DAMAGE_LEVEL_CAPS[index]);
    encoded += count * increments[index];
    remaining -= count;
  }
  return encoded * (2 ** (Number(cell(table, row, 'HitShift')) - 8));
}

function manaAtLevel(table, row, level) {
  const encoded = Number(cell(table, row, 'mana'))
    + (level - 1) * Number(cell(table, row, 'lvlmana'));
  return encoded * (2 ** (Number(cell(table, row, 'manashift')) - 8));
}

function validateApplied(tables) {
  assert(tables.skills.rows.length === 449, 'skills: runtime ordinals must remain unchanged');
  assert(tables.missiles.rows.length === 759, 'missiles: row count must remain unchanged');
  assert(tables.skilldesc.rows.length === 269, 'skilldesc: row count must remain unchanged');

  const blizzard = unique(tables.skills, 'skill', 'Blizzard', 'skills');
  assert(tables.skills.rows.indexOf(blizzard) === 59, 'Blizzard runtime ordinal must remain 59');
  for (const [header, expected] of Object.entries(SKILL_AFTER)) {
    assert(cell(tables.skills, blizzard, header) === expected, `Blizzard.${header} mismatch`);
  }

  const retained = {
    srvdofunc: '28',
    srvmissilea: 'blizzardcenter',
    srvmissileb: 'blizzardcenter',
    srvmissilec: 'blizzardcenter',
    cltdofunc: '28',
    cltmissilea: 'blizzardcenter',
    calc1: 'par1',
    calc2: 'par3',
    Param3: '2',
    lvlmana: '1',
    HitShift: '8',
    ELen: '100',
  };
  for (const [header, expected] of Object.entries(retained)) {
    assert(cell(tables.skills, blizzard, header) === expected, `Retained Blizzard.${header} mismatch`);
  }

  const levels = [1, 10, 20, 40];
  const expectedDamage = [
    [17, 24],
    [64, 89],
    [132, 181],
    [376, 515],
  ];
  const expectedMana = [13, 17.5, 22.5, 32.5];
  levels.forEach((level, index) => {
    const damage = [
      damageAtLevel(tables.skills, blizzard, level, 'EMin'),
      damageAtLevel(tables.skills, blizzard, level, 'EMax'),
    ];
    assert(
      damage.every((value, damageIndex) => value === expectedDamage[index][damageIndex]),
      `Blizzard damage L${level}=${damage.join('-')}`,
    );
    assert(manaAtLevel(tables.skills, blizzard, level) === expectedMana[index],
      `Blizzard mana L${level}=${manaAtLevel(tables.skills, blizzard, level)}`);
  });

  const center = unique(tables.missiles, 'Missile', 'blizzardcenter', 'missiles');
  assert(cell(tables.missiles, center, 'Range') === '50', 'blizzardcenter.Range mismatch');
  assert(cell(tables.missiles, center, 'pSrvDoFunc') === '10', 'blizzardcenter server behavior changed');
  assert(cell(tables.missiles, center, 'pCltDoFunc') === '13', 'blizzardcenter client behavior changed');

  for (const name of SHARDS) {
    const shard = unique(tables.missiles, 'Missile', name, 'missiles');
    assert(cell(tables.missiles, shard, 'Size') === '3', `${name}.Size mismatch`);
    assert(cell(tables.missiles, shard, 'pSrvHitFunc') === '', `${name}: PD2 callback must not be copied`);
    assert(cell(tables.missiles, shard, 'sHitPar1') === '', `${name}: PD2 callback parameter must not be copied`);
    assert(cell(tables.missiles, shard, 'CollideType') === '3', `${name}.CollideType changed`);
    assert(cell(tables.missiles, shard, 'Skill') === 'Blizzard', `${name}.Skill changed`);
  }

  const description = unique(tables.skilldesc, 'skilldesc', 'blizzard', 'skilldesc');
  for (const [header, expected] of Object.entries(SKILLDESC_AFTER)) {
    assert(cell(tables.skilldesc, description, header) === expected, `blizzard tooltip ${header} mismatch`);
  }
  assert(cell(tables.skilldesc, description, 'dsc3calca1') === '2', 'tooltip synergy count changed');
  assert(cell(tables.skilldesc, description, 'dsc3textb2') === 'skillname39', 'Ice Bolt synergy missing');
  assert(cell(tables.skilldesc, description, 'dsc3textb3') === 'skillname45', 'Ice Blast synergy missing');
}

function validateReverted(tables) {
  const blizzard = unique(tables.skills, 'skill', 'Blizzard', 'skills');
  for (const [header, expected] of Object.entries(SKILL_BEFORE)) {
    assert(cell(tables.skills, blizzard, header) === expected, `Reverted Blizzard.${header} mismatch`);
  }
  const center = unique(tables.missiles, 'Missile', 'blizzardcenter', 'missiles');
  assert(cell(tables.missiles, center, 'Range') === '100', 'reverted blizzardcenter.Range mismatch');
  for (const name of SHARDS) {
    const shard = unique(tables.missiles, 'Missile', name, 'missiles');
    assert(cell(tables.missiles, shard, 'Size') === '2', `reverted ${name}.Size mismatch`);
  }
  const description = unique(tables.skilldesc, 'skilldesc', 'blizzard', 'skilldesc');
  for (const [header, expected] of Object.entries(SKILLDESC_BEFORE)) {
    assert(cell(tables.skilldesc, description, header) === expected, `reverted tooltip ${header} mismatch`);
  }
}

function writeChanged(loaded, tables) {
  const changed = [];
  for (const name of ['skills', 'missiles', 'skilldesc']) {
    if (serializeTable(tables[name]) === loaded[name].raw) continue;
    writeTable(FILES[name], tables[name]);
    changed.push(name);
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
  const loaded = Object.fromEntries(
    Object.entries(FILES).map(([name, filePath]) => [name, load(filePath, name)]),
  );
  const tables = Object.fromEntries(
    Object.entries(loaded).map(([name, value]) => [name, value.table]),
  );

  const revert = MODE === '--revert';
  setOwnedCells(tables.skills, 'skill', 'Blizzard', SKILL_BEFORE, SKILL_AFTER, 'skills', revert);
  setOwnedCells(
    tables.missiles,
    'Missile',
    'blizzardcenter',
    CENTER_BEFORE,
    CENTER_AFTER,
    'missiles',
    revert,
  );
  for (const name of SHARDS) {
    setOwnedCells(
      tables.missiles,
      'Missile',
      name,
      SHARD_BEFORE,
      SHARD_AFTER,
      'missiles',
      revert,
    );
  }
  setOwnedCells(
    tables.skilldesc,
    'skilldesc',
    'blizzard',
    SKILLDESC_BEFORE,
    SKILLDESC_AFTER,
    'skilldesc',
    revert,
  );

  if (revert) validateReverted(tables);
  else validateApplied(tables);

  const changed = Object.keys(FILES).filter(
    (name) => serializeTable(tables[name]) !== loaded[name].raw,
  );
  if (MODE === '--check') {
    assert(changed.length === 0, `Migration required for: ${changed.join(', ')}`);
  } else {
    writeChanged(loaded, tables);
  }

  console.log(`mode=${MODE.slice(2)} changed=${changed.join(',') || 'none'}`);
  if (revert) {
    console.log('behavior=radius:7,spawn-interval:2,duration:100,cooldown:45,hitbox:2');
    console.log('VALID reverted PD2 Blizzard prototype for D2R 3.2');
  } else {
    console.log('behavior=radius:8,spawn-interval:2,duration:50,cooldown:23,hitbox:3');
    console.log('damage=L1:17-24,L10:64-89,L20:132-181,L40:376-515');
    console.log('mana=L1:13,L10:17.5,L20:22.5,L40:32.5 synergies=Ice Bolt+Ice Blast@12%');
    console.log('VALID PD2 Blizzard prototype for D2R 3.2');
  }
}

try {
  main();
} catch (error) {
  console.error(`INVALID PD2 Blizzard prototype: ${error.message}`);
  process.exitCode = 1;
}
