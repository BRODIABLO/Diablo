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
const VANILLA_EXCEL = path.join(
  ROOT,
  'data-vanilla3.2',
  'data',
  'data',
  'global',
  'excel',
);

const FILES = {
  hireling: path.join(EXCEL, 'hireling.txt'),
  itemStatCost: path.join(EXCEL, 'itemstatcost.txt'),
  monProp: path.join(EXCEL, 'monprop.txt'),
  monStats: path.join(EXCEL, 'monstats.txt'),
  monStats2: path.join(EXCEL, 'monstats2.txt'),
  petType: path.join(EXCEL, 'pettype.txt'),
  properties: path.join(EXCEL, 'properties.txt'),
  skills: path.join(EXCEL, 'skills.txt'),
  states: path.join(EXCEL, 'states.txt'),
  vanillaPetType: path.join(VANILLA_EXCEL, 'pettype.txt'),
};

const NAMES = {
  mastery: 'BKV Bow Mastery',
  masterySkillDesc: 'bkv bow mastery',
  masteryState: 'bkvbowmastery',
  fireRavenSkill: 'BKV Fire Raven',
  fireRavenSkillDesc: 'bkv fire raven',
  coldRavenSkill: 'BKV Cold Raven',
  coldRavenSkillDesc: 'bkv cold raven',
  ravenAuraSkill: 'BKV Rogue Raven Aura',
  ravenState: 'bkvrogueraven',
  fireRavenMonster: 'bkvfireraven',
  coldRavenMonster: 'bkvcoldraven',
  ravenMonStatsEx: 'bkvrogueraven',
  ravenPetType: 'bkvrogueraven',
  meleeProcProperty: 'bkv-gethitmelee-skill',
  meleeProcStat: 'item_bkv_skillongethitmelee',
};

const APPLY = process.argv.includes('--apply');
const CHECK = process.argv.includes('--check');

assert(APPLY !== CHECK, 'Use exactly one of --apply or --check.');

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

function load(filePath, label, allowMissing = false) {
  if (allowMissing && !fs.existsSync(filePath)) return null;
  const raw = fs.readFileSync(filePath, ENCODING);
  const table = parseTable(filePath);
  assert(serializeTable(table) === raw, `${label}: initial round-trip is not byte-exact`);
  assert(table.eol === '\r\n', `${label}: expected CRLF`);
  return { raw, table: cloneTable(table) };
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
  const indexes = headerMap(table);
  const row = new Array(table.headers.length).fill('');
  for (const [header, value] of Object.entries(values)) {
    const index = indexes.get(header);
    assert(index !== undefined, `Missing column ${header}`);
    row[index] = String(value);
  }
  return row;
}

function unique(table, header, value, label) {
  const rows = table.rows.filter((row) => cell(table, row, header) === value);
  assert(rows.length === 1, `${label}: expected ${header}=${value} once, found ${rows.length}`);
  return rows[0];
}

function appendOrVerify(table, keyHeader, keyValue, values, label) {
  const matches = table.rows.filter((row) => cell(table, row, keyHeader) === keyValue);
  assert(matches.length <= 1, `${label}: duplicate ${keyHeader}=${keyValue}`);
  if (matches.length === 0) {
    table.rows.push(rowFrom(table, values));
    return true;
  }
  for (const [header, expected] of Object.entries(values)) {
    assert(
      cell(table, matches[0], header) === String(expected),
      `${label}: ${keyValue}.${header} differs`,
    );
  }
  return false;
}

function removeOwnedRow(table, keyHeader, keyValue, values, label) {
  const matches = table.rows.filter((row) => cell(table, row, keyHeader) === keyValue);
  assert(matches.length <= 1, `${label}: duplicate ${keyHeader}=${keyValue}`);
  if (matches.length === 0) return false;
  for (const [header, expected] of Object.entries(values)) {
    assert(
      cell(table, matches[0], header) === String(expected),
      `${label}: refusing to remove divergent ${keyValue}.${header}`,
    );
  }
  table.rows.splice(table.rows.indexOf(matches[0]), 1);
  return true;
}

function migrateOwnedCell(
  table,
  keyHeader,
  keyValue,
  header,
  previousValues,
  expectedValue,
  label,
) {
  const matches = table.rows.filter((row) => cell(table, row, keyHeader) === keyValue);
  assert(matches.length <= 1, `${label}: duplicate ${keyHeader}=${keyValue}`);
  if (matches.length === 0) return false;
  const currentValue = cell(table, matches[0], header);
  const acceptedPreviousValues = Array.isArray(previousValues) ? previousValues : [previousValues];
  assert(
    currentValue === expectedValue || acceptedPreviousValues.includes(currentValue),
    `${label}: ${keyValue}.${header} has unexpected value ${JSON.stringify(currentValue)}`,
  );
  if (currentValue === expectedValue) return false;
  setCell(table, matches[0], header, expectedValue);
  return true;
}

function cloneRowWith(table, sourceHeader, sourceValue, values, label) {
  const source = unique(table, sourceHeader, sourceValue, label);
  const row = [...source];
  for (const [header, value] of Object.entries(values)) setCell(table, row, header, value);
  return row;
}

function appendCloneOrVerify(table, keyHeader, keyValue, sourceHeader, sourceValue, values, label) {
  const expected = cloneRowWith(table, sourceHeader, sourceValue, values, label);
  const expectedValues = Object.fromEntries(table.headers.map((header, index) => [header, expected[index] ?? '']));
  return appendOrVerify(table, keyHeader, keyValue, expectedValues, label);
}

function assertNumericOwned(table, idHeader, idValue, keyHeader, keyValue, label) {
  const owners = table.rows
    .filter((row) => cell(table, row, idHeader) === String(idValue))
    .map((row) => cell(table, row, keyHeader));
  assert(
    owners.length === 1 && owners[0] === keyValue,
    `${label}: ${idHeader}=${idValue} owners=${owners.join(',') || 'none'}`,
  );
}

function preparePetType(targetLoad, vanillaLoad) {
  if (targetLoad) return targetLoad;
  return { raw: null, table: cloneTable(vanillaLoad.table) };
}

function applyItemEventStat(table) {
  return appendCloneOrVerify(
    table,
    'Stat',
    NAMES.meleeProcStat,
    'Stat',
    'item_skillongethit',
    {
      Stat: NAMES.meleeProcStat,
      '*ID': '390',
      itemevent1: 'damagedinmelee',
      itemeventfunc1: '21',
      itemevent2: '',
      itemeventfunc2: '',
      descpriority: '158',
      descfunc: '15',
      descstrpos: 'ItemExpansiveChanc2',
      descstrneg: 'ItemExpansiveChanc2',
      '*eol': '0',
    },
    'itemstatcost',
  );
}

function applyProperty(table) {
  return appendCloneOrVerify(
    table,
    'code',
    NAMES.meleeProcProperty,
    'code',
    'gethit-skill',
    {
      code: NAMES.meleeProcProperty,
      '*Id': '309',
      stat1: NAMES.meleeProcStat,
      '*Tooltip': '#% Chance to cast level # [Skill] when struck in melee',
      '*eol': '0',
    },
    'properties',
  );
}

function applyMonProp(table) {
  return appendOrVerify(table, 'Id', 'roguehire', {
    Id: 'roguehire',
    prop1: NAMES.meleeProcProperty,
    par1: '77',
    min1: '25',
    max1: '5',
    'prop1 (N)': NAMES.meleeProcProperty,
    'par1 (N)': '77',
    'min1 (N)': '33',
    'max1 (N)': '10',
    'prop1 (H)': NAMES.meleeProcProperty,
    'par1 (H)': '77',
    'min1 (H)': '50',
    'max1 (H)': '15',
    '*eol': '0',
  }, 'monprop');
}

function applyRogueMonStats(table) {
  const row = unique(table, 'Id', 'roguehire', 'monstats');
  const before = cell(table, row, 'MonProp');
  assert(before === '' || before === 'roguehire', `monstats: roguehire.MonProp=${before}`);
  setCell(table, row, 'MonProp', 'roguehire');
  return before !== 'roguehire';
}

function ravenMonsterValues(name, hcIdx, damageSkill) {
  return {
    Id: name,
    '*hcIdx': hcIdx,
    BaseId: 'druidhawk',
    TransLvl: '0',
    NameStr: 'Druid Hawk',
    // Reuse the vanilla Raven/Hawk render identity. A custom MonStatsEx key
    // without matching HD assets creates a live server unit that is invisible
    // on the D2R client.
    MonStatsEx: 'druidhawk',
    MonProp: 'druidhawk',
    AI: 'NecroPet',
    Code: 'hk',
    enabled: '1',
    MinGrp: '1',
    MaxGrp: '1',
    Velocity: '11',
    Run: '22',
    Rarity: '0',
    MonSound: 'raven',
    UMonSound: 'raven',
    aidel: '1',
    'aidel(N)': '1',
    'aidel(H)': '1',
    aip1: '90',
    'aip1(N)': '90',
    'aip1(H)': '90',
    aip2: '',
    'aip2(N)': '',
    'aip2(H)': '',
    aip3: '',
    'aip3(N)': '',
    'aip3(H)': '',
    aip4: '',
    'aip4(N)': '',
    'aip4(H)': '',
    aip5: '',
    'aip5(N)': '',
    'aip5(H)': '',
    Align: '1',
    isSpawn: '1',
    inTown: '1',
    flying: '1',
    opendoors: '1',
    killable: '1',
    switchai: '1',
    neverCount: '1',
    CannotHerald: '1',
    Skill1: '',
    Sk1mode: '',
    Sk1lvl: '',
    Drain: '100',
    'Drain(N)': '100',
    'Drain(H)': '100',
    coldeffect: '-50',
    'coldeffect(N)': '-50',
    'coldeffect(H)': '-50',
    DamageRegen: '2',
    SkillDamage: damageSkill,
    noRatio: '1',
    Crit: '5',
    minHP: '26',
    maxHP: '26',
    AC: '25',
    'MinHP(N)': '26',
    'MaxHP(N)': '26',
    'AC(N)': '25',
    'MinHP(H)': '26',
    'MaxHP(H)': '26',
    'AC(H)': '25',
    '*eol': '0',
  };
}

function migrateRavenMonster(table, name, damageSkill) {
  const previousValues = {
    MonStatsEx: NAMES.ravenMonStatsEx,
    AI: 'Raven',
    aidel: '15',
    'aidel(N)': '8',
    'aidel(H)': '0',
    aip1: '10',
    'aip1(N)': '10',
    'aip1(H)': '10',
    aip2: '6',
    'aip2(N)': '6',
    'aip2(H)': '6',
    aip3: '2',
    'aip3(N)': '2',
    'aip3(H)': '2',
    aip4: '95',
    'aip4(N)': '95',
    'aip4(H)': '95',
    aip5: '35',
    'aip5(N)': '35',
    'aip5(H)': '35',
    Skill1: damageSkill,
    Sk1mode: 'NU',
    Sk1lvl: '0',
    Drain: '',
    'Drain(N)': '',
    'Drain(H)': '',
  };
  const expected = ravenMonsterValues(name, '', damageSkill);
  let changed = false;
  for (const [header, previousValue] of Object.entries(previousValues)) {
    changed = migrateOwnedCell(
      table,
      'Id',
      name,
      header,
      previousValue,
      expected[header],
      'monstats',
    ) || changed;
  }
  return changed;
}

function applyRavenMonsters(table) {
  let changed = false;
  changed = migrateRavenMonster(
    table,
    NAMES.fireRavenMonster,
    NAMES.fireRavenSkill,
  ) || changed;
  changed = migrateRavenMonster(
    table,
    NAMES.coldRavenMonster,
    NAMES.coldRavenSkill,
  ) || changed;
  changed = appendOrVerify(
    table,
    'Id',
    NAMES.fireRavenMonster,
    ravenMonsterValues(NAMES.fireRavenMonster, '787', NAMES.fireRavenSkill),
    'monstats',
  ) || changed;
  changed = appendOrVerify(
    table,
    'Id',
    NAMES.coldRavenMonster,
    ravenMonsterValues(NAMES.coldRavenMonster, '788', NAMES.coldRavenSkill),
    'monstats',
  ) || changed;
  return changed;
}

function applyRavenMonStatsEx(table) {
  const expected = cloneRowWith(table, 'Id', 'druidhawk', {
    Id: NAMES.ravenMonStatsEx,
    '*hcIdx': '755',
    Light: '7',
    'light-r': '255',
    'light-g': '255',
    'light-b': '255',
    '*eol': '0',
  }, 'monstats2');
  const expectedValues = Object.fromEntries(
    table.headers.map((header, index) => [header, expected[index] ?? '']),
  );
  return removeOwnedRow(
    table,
    'Id',
    NAMES.ravenMonStatsEx,
    expectedValues,
    'monstats2',
  );
}

function applyPetType(table) {
  return appendOrVerify(table, 'pet type', NAMES.ravenPetType, {
    'pet type': NAMES.ravenPetType,
    basemax: '1',
    warp: '1',
    automap: '1',
  }, 'pettype');
}

function applyStates(table) {
  let changed = false;
  changed = appendOrVerify(table, 'state', NAMES.masteryState, {
    state: NAMES.masteryState,
    '*ID': '243',
    '*eol': '0',
  }, 'states') || changed;
  changed = appendOrVerify(table, 'state', NAMES.ravenState, {
    state: NAMES.ravenState,
    '*ID': '244',
    '*eol': '0',
  }, 'states') || changed;
  return changed;
}

function baseSkillValues(name) {
  return {
    skill: name,
    '*Id': '9999',
    enhanceable: '1',
    attackrank: '0',
    range: 'none',
    UseAttackRate: '1',
    reqlevel: '1',
    minmana: '0',
    manashift: '8',
    mana: '0',
    lvlmana: '0',
    interrupt: '1',
    InGame: '1',
    HitShift: '8',
    'cost add': '0',
    '*eol': '0',
  };
}

function masterySkillValues() {
  return {
    ...baseSkillValues(NAMES.mastery),
    skilldesc: NAMES.masterySkillDesc,
    passivestate: NAMES.masteryState,
    passiveitype: 'bow',
    passivestat1: 'passive_mastery_melee_th',
    passivecalc1: 'ln12',
    passivestat2: 'passive_mastery_melee_dmg',
    passivecalc2: 'ln34',
    passivestat3: 'passive_mastery_melee_crit',
    passivecalc3: 'dm56',
    aura: '1',
    passive: '1',
    Param1: '28',
    '*Param1 Description': 'Attack % base',
    Param2: '8',
    '*Param2 Description': 'Attack % per level',
    Param3: '28',
    '*Param3 Description': 'Damage % base',
    Param4: '5',
    '*Param4 Description': 'Damage % per level',
    Param5: '0',
    '*Param5 Description': 'Critical strike floor',
    Param6: '35',
    '*Param6 Description': 'Critical strike ceiling',
    EType: 'stat',
  };
}

function ravenSkillValues(name, monster, element) {
  const values = {
    ...baseSkillValues(name),
    skilldesc: element === 'fire' ? NAMES.fireRavenSkillDesc : NAMES.coldRavenSkillDesc,
    srvstfunc: '28',
    srvdofunc: '44',
    srvmissilea: 'blade shield attachment',
    aurastate: NAMES.ravenState,
    auralencalc: '125',
    summon: monster,
    pettype: NAMES.ravenPetType,
    petmax: '1',
    summode: 'S1',
    sumskill1: NAMES.ravenAuraSkill,
    sumsk1calc: '1',
    stsound: 'druid_summon',
    anim: 'SC',
    seqtrans: 'SC',
    monanim: 'xx',
    restrict: '1',
    InTown: '1',
    calc1: '',
    '*calc1 desc': '',
    calc2: '',
    '*calc2 desc': '',
    ToHitCalc: '512*ulvl',
    MinDam: '2',
    MinLevDam1: '1',
    MinLevDam2: '2',
    MinLevDam3: '4',
    MinLevDam4: '7',
    MinLevDam5: '12',
    MaxDam: '4',
    MaxLevDam1: '2',
    MaxLevDam2: '3',
    MaxLevDam3: '5',
    MaxLevDam4: '8',
    MaxLevDam5: '14',
    Param5: '',
    '*Param5 Description': '',
    Param6: '',
    '*Param6 Description': '',
    EType: element,
    aitype: '1',
  };
  if (element === 'fire') {
    Object.assign(values, {
      passivestat1: 'firemindam', passivecalc1: 'edmn',
      passivestat2: 'firemaxdam', passivecalc2: 'edmx',
      EMin: '1', EMinLev1: '2', EMinLev2: '3', EMinLev3: '6', EMinLev4: '12', EMinLev5: '24',
      EMax: '4', EMaxLev1: '2', EMaxLev2: '3', EMaxLev3: '7', EMaxLev4: '14', EMaxLev5: '27',
    });
  } else {
    Object.assign(values, {
      passivestat1: 'coldmindam', passivecalc1: 'edmn',
      passivestat2: 'coldmaxdam', passivecalc2: 'edmx',
      passivestat3: 'coldlength', passivecalc3: 'edln',
      EMin: '3', EMinLev1: '2', EMinLev2: '3', EMinLev3: '5', EMinLev4: '9', EMinLev5: '15',
      EMax: '4', EMaxLev1: '2', EMaxLev2: '3', EMaxLev3: '6', EMaxLev4: '10', EMaxLev5: '17',
      ELen: '100', ELevLen1: '30', ELevLen2: '30', ELevLen3: '30',
    });
  }
  return values;
}

function ravenAuraSkillValues() {
  return {
    ...baseSkillValues(NAMES.ravenAuraSkill),
    srvdofunc: '65',
    aurafilter: '65539',
    aurastate: NAMES.ravenState,
    auratargetstate: NAMES.ravenState,
    aurarangecalc: '1024',
    immediate: '1',
    monanim: 'NU',
    aura: '1',
    perdelay: '50',
  };
}

function migrateRavenSkill(table, skillName, monster, element) {
  const expected = ravenSkillValues(skillName, monster, element);
  const previousValues = {
    srvstfunc: '',
    srvdofunc: ['119', '114'],
    srvmissilea: '',
    aurastate: '',
    auralencalc: '',
    passivestat1: '',
    passivecalc1: '',
    passivestat2: '',
    passivecalc2: '',
    passivestat3: '',
    passivecalc3: '',
    sumskill1: ['', 'Summon Splash'],
    sumsk1calc: '',
    calc1: '0',
    '*calc1 desc': 'HP %',
    calc2: 'ulvl',
    '*calc2 desc': 'Summon pet level',
    Param5: '999999',
    '*Param5 Description': 'Maximum attacks before expiration',
    Param6: '0',
    '*Param6 Description': 'Additional attacks per skill level',
    aitype: ['', '1'],
  };
  let changed = false;
  for (const [header, previousValue] of Object.entries(previousValues)) {
    changed = migrateOwnedCell(
      table,
      'skill',
      skillName,
      header,
      previousValue,
      expected[header] ?? '',
      'skills',
    ) || changed;
  }
  return changed;
}

function applySkills(table) {
  let changed = false;
  changed = migrateRavenSkill(
    table,
    NAMES.fireRavenSkill,
    NAMES.fireRavenMonster,
    'fire',
  ) || changed;
  changed = migrateRavenSkill(
    table,
    NAMES.coldRavenSkill,
    NAMES.coldRavenMonster,
    'cold',
  ) || changed;
  changed = appendOrVerify(table, 'skill', NAMES.mastery, masterySkillValues(), 'skills') || changed;
  changed = appendOrVerify(
    table,
    'skill',
    NAMES.ravenAuraSkill,
    ravenAuraSkillValues(),
    'skills',
  ) || changed;
  changed = appendOrVerify(
    table,
    'skill',
    NAMES.fireRavenSkill,
    ravenSkillValues(NAMES.fireRavenSkill, NAMES.fireRavenMonster, 'fire'),
    'skills',
  ) || changed;
  changed = appendOrVerify(
    table,
    'skill',
    NAMES.coldRavenSkill,
    ravenSkillValues(NAMES.coldRavenSkill, NAMES.coldRavenMonster, 'cold'),
    'skills',
  ) || changed;
  return changed;
}

function assignSkill(table, row, slot, values) {
  for (const [field, value] of Object.entries(values)) setCell(table, row, `${field}${slot}`, value);
}

function applyHirelings(table) {
  const rows = table.rows.filter((row) => (
    cell(table, row, 'Hireling') === 'Rogue Scout'
    && cell(table, row, 'Version') === '100'
  ));
  assert(rows.length === 12, `hireling: expected 12 expansion Rogue rows, found ${rows.length}`);

  const tiers = {
    3: {
      defaultChance: 75,
      innerLevel: 1,
      fireBaseLevel: 1,
      coldBaseLevel: 1,
      fireMidLevel: 0,
      coldMidLevel: 0,
      midLevelGrowth: 0,
      fireTopLevel: 0,
      coldTopLevel: 0,
      topLevelGrowth: 0,
      ravenLevel: 1,
      masteryLevel: 1,
    },
    36: {
      defaultChance: 50,
      innerLevel: 12,
      fireBaseLevel: 12,
      coldBaseLevel: 12,
      fireMidLevel: 7,
      coldMidLevel: 7,
      midLevelGrowth: 5,
      fireTopLevel: 4,
      coldTopLevel: 3,
      topLevelGrowth: 3,
      ravenLevel: 7,
      masteryLevel: 12,
    },
    67: {
      defaultChance: 20,
      innerLevel: 22,
      fireBaseLevel: 22,
      coldBaseLevel: 15,
      fireMidLevel: 12,
      coldMidLevel: 9,
      midLevelGrowth: 5,
      fireTopLevel: 7,
      coldTopLevel: 7,
      topLevelGrowth: 3,
      ravenLevel: 12,
      masteryLevel: 22,
    },
  };

  for (const row of rows) {
    const level = Number(cell(table, row, 'Level'));
    const tier = tiers[level];
    assert(tier, `hireling: unsupported Rogue breakpoint ${level}`);
    const subtype = cell(table, row, '*SubType');
    const fire = subtype.startsWith('Fire -');
    const cold = subtype.startsWith('Ice -');
    assert(fire || cold, `hireling: unexpected Rogue subtype ${subtype}`);
    const baseLevel = fire ? tier.fireBaseLevel : tier.coldBaseLevel;
    const midLevel = fire ? tier.fireMidLevel : tier.coldMidLevel;
    const topLevel = fire ? tier.fireTopLevel : tier.coldTopLevel;

    setCell(table, row, 'DefaultChance', tier.defaultChance);
    assignSkill(table, row, 1, {
      Skill: 'Inner Sight', Mode: '4', Chance: '30', ChancePerLvl: '0',
      Level: tier.innerLevel, LvlPerLvl: '10',
    });
    assignSkill(table, row, 2, {
      Skill: fire ? 'Fire Arrow' : 'Cold Arrow', Mode: '4', Chance: '50', ChancePerLvl: '0',
      Level: baseLevel, LvlPerLvl: '10',
    });
    assignSkill(table, row, 3, {
      Skill: fire ? 'Exploding Arrow' : 'Ice Arrow', Mode: '4',
      Chance: level === 3 ? '0' : (fire ? '75' : '50'), ChancePerLvl: '0',
      Level: midLevel, LvlPerLvl: tier.midLevelGrowth,
    });
    assignSkill(table, row, 4, {
      Skill: fire ? 'Immolation Arrow' : 'Freezing Arrow', Mode: '4',
      Chance: level === 3 ? '0' : (fire ? '50' : '75'), ChancePerLvl: '0',
      Level: topLevel, LvlPerLvl: tier.topLevelGrowth,
    });
    assignSkill(table, row, 5, {
      Skill: fire ? NAMES.fireRavenSkill : NAMES.coldRavenSkill, Mode: '4',
      Chance: '25', ChancePerLvl: '0', Level: tier.ravenLevel, LvlPerLvl: '5',
    });
    assignSkill(table, row, 6, {
      Skill: NAMES.mastery, Mode: '1', Chance: '1', ChancePerLvl: '0',
      Level: tier.masteryLevel, LvlPerLvl: '10',
    });
  }
  return true;
}

const DAMAGE_LEVEL_CAPS = [7, 8, 6, 6, Number.POSITIVE_INFINITY];

function hirelingSkillLevel(table, row, slot, currentLevel) {
  const rowLevel = Number(cell(table, row, 'Level'));
  const baseLevel = Number(cell(table, row, `Level${slot}`));
  const growth = Number(cell(table, row, `LvlPerLvl${slot}`));
  assert(currentLevel >= rowLevel, `hireling: level ${currentLevel} precedes row ${rowLevel}`);
  return baseLevel + Math.floor((growth * (currentLevel - rowLevel)) / 32);
}

function elementalBaseDamage(skills, skillName, skillLevel) {
  assert(skillLevel >= 1, `${skillName}: damage requested below skill level 1`);
  const row = unique(skills, 'skill', skillName, 'skills');
  const hitShift = Number(cell(skills, row, 'HitShift'));
  assert(Number.isInteger(hitShift), `${skillName}: invalid HitShift`);
  const calculate = (prefix) => {
    const increments = [
      Number(cell(skills, row, prefix)),
      ...[1, 2, 3, 4, 5].map((index) => Number(cell(skills, row, `${prefix}Lev${index}`))),
    ];
    let value = increments[0];
    let remaining = skillLevel - 1;
    for (let index = 0; remaining > 0; index += 1) {
      const count = Math.min(remaining, DAMAGE_LEVEL_CAPS[index]);
      value += count * increments[index + 1];
      remaining -= count;
    }
    return value;
  };
  return { minimum: calculate('EMin'), maximum: calculate('EMax'), hitShift };
}

function elementalDamage(skills, skillName, skillLevel, synergyPercent) {
  const { minimum, maximum, hitShift } = elementalBaseDamage(skills, skillName, skillLevel);
  const multiplier = (1 + synergyPercent / 100) * (2 ** (hitShift - 8));
  return [Math.round(minimum * multiplier), Math.round(maximum * multiplier)];
}

function rogueRowAtLevel(hireling, subtype, currentLevel) {
  const matches = hireling.rows
    .filter((row) => (
      cell(hireling, row, 'Hireling') === 'Rogue Scout'
      && cell(hireling, row, 'Version') === '100'
      && cell(hireling, row, '*SubType') === subtype
      && Number(cell(hireling, row, 'Level')) <= currentLevel
    ))
    .sort((left, right) => Number(cell(hireling, right, 'Level')) - Number(cell(hireling, left, 'Level')));
  assert(matches.length > 0, `hireling: no ${subtype} row at level ${currentLevel}`);
  return matches[0];
}

function damageSnapshot(tables, subtype, currentLevel) {
  const row = rogueRowAtLevel(tables.hireling, subtype, currentLevel);
  const levels = new Map();
  for (let slot = 2; slot <= 4; slot += 1) {
    levels.set(cell(tables.hireling, row, `Skill${slot}`), hirelingSkillLevel(
      tables.hireling,
      row,
      slot,
      currentLevel,
    ));
  }
  const damage = {};
  for (const [skillName, skillLevel] of levels) {
    if (skillLevel === 0) continue;
    let synergyPercent = 0;
    if (skillName === 'Fire Arrow') synergyPercent = (levels.get('Exploding Arrow') || 0) * 12;
    if (skillName === 'Exploding Arrow') {
      synergyPercent = ((levels.get('Fire Arrow') || 0) + (levels.get('Immolation Arrow') || 0)) * 5;
    }
    if (skillName === 'Immolation Arrow') {
      synergyPercent = ((levels.get('Fire Arrow') || 0) + (levels.get('Exploding Arrow') || 0)) * 5;
    }
    if (skillName === 'Cold Arrow') synergyPercent = (levels.get('Ice Arrow') || 0) * 12;
    if (skillName === 'Ice Arrow') synergyPercent = (levels.get('Cold Arrow') || 0) * 8;
    if (skillName === 'Freezing Arrow') {
      synergyPercent = ((levels.get('Cold Arrow') || 0) + (levels.get('Ice Arrow') || 0)) * 3;
    }
    damage[skillName] = elementalDamage(tables.skills, skillName, skillLevel, synergyPercent);
  }
  return {
    currentLevel,
    rowLevel: Number(cell(tables.hireling, row, 'Level')),
    levels: Object.fromEntries(levels),
    damage,
  };
}

function assertSnapshot(actual, expected, label) {
  assert(
    JSON.stringify(actual.damage) === JSON.stringify(expected),
    `${label}: expected ${JSON.stringify(expected)}, got ${JSON.stringify(actual.damage)}`,
  );
}

function validateDamageBudget(tables) {
  const snapshots = [
    damageSnapshot(tables, 'Fire - Normal', 3),
    damageSnapshot(tables, 'Ice - Normal', 3),
    damageSnapshot(tables, 'Fire - Normal', 36),
    damageSnapshot(tables, 'Ice - Normal', 36),
    damageSnapshot(tables, 'Fire - Normal', 67),
    damageSnapshot(tables, 'Ice - Normal', 67),
    damageSnapshot(tables, 'Fire - Normal', 90),
    damageSnapshot(tables, 'Ice - Normal', 90),
  ];
  const expected = [
    { 'Fire Arrow': [1, 4] },
    { 'Cold Arrow': [3, 4] },
    { 'Fire Arrow': [50, 55], 'Exploding Arrow': [74, 99], 'Immolation Arrow': [23, 27] },
    { 'Cold Arrow': [50, 52], 'Ice Arrow': [82, 90], 'Freezing Arrow': [86, 118] },
    { 'Fire Arrow': [183, 205], 'Exploding Arrow': [233, 279], 'Immolation Arrow': [57, 62] },
    { 'Cold Arrow': [72, 74], 'Ice Arrow': [132, 143], 'Freezing Arrow': [163, 198] },
    { 'Fire Arrow': [479, 546], 'Exploding Arrow': [380, 444], 'Immolation Arrow': [93, 99] },
    { 'Cold Arrow': [149, 159], 'Ice Arrow': [265, 287], 'Freezing Arrow': [242, 283] },
  ];
  snapshots.forEach((snapshot, index) => assertSnapshot(snapshot, expected[index], `damage snapshot ${index + 1}`));

  for (const row of tables.hireling.rows.filter((candidate) => (
    cell(tables.hireling, candidate, 'Hireling') === 'Rogue Scout'
    && cell(tables.hireling, candidate, 'Version') === '100'
  ))) {
    const rowLevel = Number(cell(tables.hireling, row, 'Level'));
    if (rowLevel === 3) {
      for (const slot of [3, 4]) {
        assert(cell(tables.hireling, row, `Chance${slot}`) === '0', `hireling: level 3 slot ${slot} must be locked`);
        assert(cell(tables.hireling, row, `Level${slot}`) === '0', `hireling: level 3 slot ${slot} must be level 0`);
        assert(cell(tables.hireling, row, `LvlPerLvl${slot}`) === '0', `hireling: level 3 slot ${slot} must not grow`);
      }
    }
    if (rowLevel === 36) {
      const fire = cell(tables.hireling, row, '*SubType').startsWith('Fire -');
      assert(cell(tables.hireling, row, 'Chance4') === (fire ? '50' : '75'), 'hireling: level 36 top skill chance mismatch');
      assert(cell(tables.hireling, row, 'Level4') === (fire ? '4' : '3'), 'hireling: level 36 top skill level mismatch');
      assert(cell(tables.hireling, row, 'LvlPerLvl4') === '3', 'hireling: level 36 top skill growth mismatch');
    }
  }
  return snapshots;
}

function validateReferences(tables) {
  const skillNames = new Set(tables.skills.rows.map((row) => cell(tables.skills, row, 'skill')).filter(Boolean));
  const monsterNames = new Set(tables.monStats.rows.map((row) => cell(tables.monStats, row, 'Id')).filter(Boolean));
  const monStatsExNames = new Set(tables.monStats2.rows.map((row) => cell(tables.monStats2, row, 'Id')).filter(Boolean));
  const petTypes = new Set(tables.petType.rows.map((row) => cell(tables.petType, row, 'pet type')).filter(Boolean));
  const properties = new Set(tables.properties.rows.map((row) => cell(tables.properties, row, 'code')).filter(Boolean));
  const stats = new Set(tables.itemStatCost.rows.map((row) => cell(tables.itemStatCost, row, 'Stat')).filter(Boolean));

  for (const name of [
    NAMES.mastery,
    NAMES.fireRavenSkill,
    NAMES.coldRavenSkill,
  ]) {
    assert(skillNames.has(name), `Missing skill ${name}`);
  }
  for (const name of [NAMES.fireRavenMonster, NAMES.coldRavenMonster]) {
    assert(monsterNames.has(name), `Missing monster ${name}`);
  }
  assert(!monStatsExNames.has(NAMES.ravenMonStatsEx), 'Obsolete custom Raven MonStats2 row still present');
  assert(petTypes.has(NAMES.ravenPetType), 'Missing Raven pet type');
  assert(properties.has(NAMES.meleeProcProperty), 'Missing melee proc property');
  assert(stats.has(NAMES.meleeProcStat), 'Missing melee proc stat');

  const terror = unique(tables.skills, 'skill', 'Terror', 'skills');
  assert(cell(tables.skills, terror, '*Id') === '77', 'Terror skill ID must remain 77');
  assertNumericOwned(tables.itemStatCost, '*ID', '390', 'Stat', NAMES.meleeProcStat, 'itemstatcost');
  assertNumericOwned(tables.properties, '*Id', '309', 'code', NAMES.meleeProcProperty, 'properties');
  assertNumericOwned(tables.states, '*ID', '243', 'state', NAMES.masteryState, 'states');
  assertNumericOwned(tables.states, '*ID', '244', 'state', NAMES.ravenState, 'states');
  assertNumericOwned(tables.monStats, '*hcIdx', '787', 'Id', NAMES.fireRavenMonster, 'monstats');
  assertNumericOwned(tables.monStats, '*hcIdx', '788', 'Id', NAMES.coldRavenMonster, 'monstats');
}

function masterySnapshot(skillLevel) {
  return {
    skillLevel,
    attackRatingPercent: 28 + 8 * (skillLevel - 1),
    physicalDamagePercent: 28 + 5 * (skillLevel - 1),
  };
}

function main() {
  const loaded = {
    hireling: load(FILES.hireling, 'hireling'),
    itemStatCost: load(FILES.itemStatCost, 'itemstatcost'),
    monProp: load(FILES.monProp, 'monprop'),
    monStats: load(FILES.monStats, 'monstats'),
    monStats2: load(FILES.monStats2, 'monstats2'),
    petType: load(FILES.petType, 'pettype', true),
    properties: load(FILES.properties, 'properties'),
    skills: load(FILES.skills, 'skills'),
    states: load(FILES.states, 'states'),
    vanillaPetType: load(FILES.vanillaPetType, 'vanilla pettype'),
  };
  loaded.petType = preparePetType(loaded.petType, loaded.vanillaPetType);

  const tables = Object.fromEntries(
    Object.entries(loaded)
      .filter(([name]) => name !== 'vanillaPetType')
      .map(([name, value]) => [name, value.table]),
  );

  applyItemEventStat(tables.itemStatCost);
  applyProperty(tables.properties);
  applyMonProp(tables.monProp);
  applyRogueMonStats(tables.monStats);
  applyRavenMonsters(tables.monStats);
  applyRavenMonStatsEx(tables.monStats2);
  applyPetType(tables.petType);
  applyStates(tables.states);
  applySkills(tables.skills);
  applyHirelings(tables.hireling);
  validateReferences(tables);
  const damageSnapshots = validateDamageBudget(tables);

  const outputs = {
    hireling: FILES.hireling,
    itemStatCost: FILES.itemStatCost,
    monProp: FILES.monProp,
    monStats: FILES.monStats,
    monStats2: FILES.monStats2,
    petType: FILES.petType,
    properties: FILES.properties,
    skills: FILES.skills,
    states: FILES.states,
  };

  const changed = [];
  for (const [name, filePath] of Object.entries(outputs)) {
    const serialized = serializeTable(tables[name]);
    const raw = loaded[name].raw;
    if (raw !== serialized) changed.push(name);
    if (APPLY && raw !== serialized) writeTable(filePath, tables[name]);
  }

  if (CHECK) assert(changed.length === 0, `Migration required for: ${changed.join(', ')}`);

  if (APPLY) {
    for (const [name, filePath] of Object.entries(outputs)) {
      const raw = fs.readFileSync(filePath, ENCODING);
      const parsed = parseTable(filePath);
      assert(raw === serializeTable(parsed), `${name}: final round-trip is not byte-exact`);
      assert(parsed.eol === '\r\n', `${name}: final EOL is not CRLF`);
    }
  }

  const snapshots = [1, 12, 22, 50].map(masterySnapshot);
  console.log(`mode=${APPLY ? 'apply' : 'check'} changed=${changed.join(',') || 'none'}`);
  console.log(`mastery=${JSON.stringify(snapshots)}`);
  console.log(`damage=${JSON.stringify(damageSnapshots)}`);
  console.log('VALID Act I Rogue Scout data migration');
}

main();
