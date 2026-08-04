'use strict';

// Integre la competence Mercenary Command 3.2 dans BKVince sans reprendre les
// modifications d'AI/hireling du profil D2RMM. Les Rogues Acte 1 conservent
// explicitement Chance1=30, la valeur TCP qui evite le spam d'Inner Sight.

const crypto = require('crypto');
const fs = require('fs');
const path = require('path');
const {
  parseTable,
  serializeTable,
  writeTable,
  ENCODING,
} = require('../build-data/tsv');

const ROOT = path.resolve(__dirname, '..', '..');
const CHECK_ONLY = process.argv.includes('--check');
const SOURCE_ROOT = path.join(ROOT, 'data-TCP');
const TARGET_ROOT = path.join(ROOT, 'data-BKVince', 'BKVince.mpq', 'data');
const SOURCE_EXCEL = path.join(SOURCE_ROOT, 'global', 'excel');
const TARGET_EXCEL = path.join(TARGET_ROOT, 'global', 'excel');

const COMMAND_SPECS = Object.freeze([
  { className: 'Amazon', skill: 'CommandAma', overlay: 'dopllezon_appear', startSound: 'amazon_chat_b_1', castSound: 'amazon_valkyrie_cast' },
  { className: 'Sorceress', skill: 'CommandSor', overlay: 'light_cast_2', startSound: 'sorceress_chat_b_1', castSound: 'sorceress_energyshield' },
  { className: 'Necromancer', skill: 'CommandNec', overlay: 'commandnec', startSound: 'necromancer_chat_b_1', castSound: 'necromancer_bone_cast' },
  { className: 'Paladin', skill: 'CommandPal', overlay: 'commandpal', startSound: 'paladin_chat_b_1', castSound: 'shrine_exchange' },
  { className: 'Barbarian', skill: 'CommandBar', overlay: 'commandbar', startSound: 'barbarian_chat_b_1', castSound: 'barbarian_circle_1' },
  { className: 'Druid', skill: 'CommandDru', overlay: 'commanddru', startSound: 'druid_chat_b_1', castSound: 'druid_windcast', restrict: '1' },
  { className: 'Assassin', skill: 'CommandAss', overlay: 'commandass', startSound: 'assassin_chat_b_1', castSound: 'assassin_fade' },
  { className: 'Warlock', skill: 'CommandWar', overlay: 'warlock_shadow_cast_1', startSound: 'warlock_chat_b_1', castSound: 'Warlock_Skill_Psychic_Ward_Cast_03' },
]);

const COMMAND_SKILLS = Object.freeze(COMMAND_SPECS.map((entry) => entry.skill));
const COMMAND_OVERLAYS = Object.freeze(['commandbar', 'commandpal', 'commanddru', 'commandnec', 'commandass']);
const COMMAND_STRING_IDS = Object.freeze({ command: 28106, commanddesc: 28107 });
const COMMAND_STRING_KEYS = Object.freeze(Object.keys(COMMAND_STRING_IDS));

const ASSETS = Object.freeze([
  'hd/global/ui/spells/submenu/skillicon.lowend.sprite',
  'hd/global/ui/spells/submenu/skillicon.sprite',
  'hd/overlays/assassin/shadowwarriorappear.json',
  'hd/overlays/assassin/shadowwarriordeath.json',
  'hd/overlays/common/commandass.json',
  'hd/overlays/common/commandbar.json',
  'hd/overlays/common/commanddru.json',
  'hd/overlays/common/commandnec.json',
  'hd/overlays/common/commandpal.json',
  'hd/overlays/paladin/charge.json',
]);

const PRESERVED_ASSET_OVERRIDES = Object.freeze([
  'hd/overlays/common/commandbar.json',
]);

const FILES = Object.freeze({
  source: {
    overlay: path.join(SOURCE_EXCEL, 'overlay.txt'),
    skillDesc: path.join(SOURCE_EXCEL, 'skilldesc.txt'),
    strings: path.join(SOURCE_ROOT, 'local', 'lng', 'strings', 'skills.json'),
  },
  target: {
    charStats: path.join(TARGET_EXCEL, 'charstats.txt'),
    hireling: path.join(TARGET_EXCEL, 'hireling.txt'),
    overlay: path.join(TARGET_EXCEL, 'overlay.txt'),
    skillDesc: path.join(TARGET_EXCEL, 'skilldesc.txt'),
    skills: path.join(TARGET_EXCEL, 'skills.txt'),
    strings: path.join(TARGET_ROOT, 'local', 'lng', 'strings', 'skills.json'),
    stringsDirectory: path.join(TARGET_ROOT, 'local', 'lng', 'strings'),
  },
});

function fail(message) {
  throw new Error(message);
}

function assert(condition, message) {
  if (!condition) fail(message);
}

function indexes(headers) {
  return Object.fromEntries(headers.map((header, index) => [header, index]));
}

function sameRow(left, right) {
  return left.length === right.length && left.every((value, index) => value === right[index]);
}

function loadTable(filePath, label) {
  assert(fs.existsSync(filePath), `${label} absent: ${filePath}`);
  const raw = fs.readFileSync(filePath, ENCODING);
  const table = parseTable(filePath);
  assert(raw === serializeTable(table), `${label}: round-trip non byte-exact`);
  assert(table.eol === '\r\n', `${label}: CRLF requis`);
  for (const row of table.rows) {
    assert(row.length === table.headers.length, `${label}: largeur de ligne invalide`);
  }
  return { filePath, raw, table };
}

function rowFromObject(table, values, label) {
  const headerIndexes = indexes(table.headers);
  for (const header of Object.keys(values)) {
    assert(header in headerIndexes, `${label}: colonne ${header} absente`);
  }
  return table.headers.map((header) => values[header] ?? '');
}

function mapSourceRow(source, target, sourceRow) {
  const sourceIndexes = indexes(source.table.headers);
  return target.table.headers.map((header) => (
    header in sourceIndexes ? sourceRow[sourceIndexes[header]] ?? '' : ''
  ));
}

function appendOrValidateRows(document, keyHeader, expectedRows, expectedKeys, label, changed) {
  const keyIndex = indexes(document.table.headers)[keyHeader];
  assert(keyIndex !== undefined, `${label}: colonne cle ${keyHeader} absente`);
  const existing = document.table.rows.filter((row) => expectedKeys.includes(row[keyIndex]));
  if (existing.length === 0) {
    assert(!CHECK_ONLY, `${label}: portage absent`);
    if (!CHECK_ONLY) {
      document.table.rows.push(...expectedRows.map((row) => row.slice()));
      changed.push(label);
    }
    return;
  }
  assert(existing.length === expectedRows.length, `${label}: portage partiel (${existing.length}/${expectedRows.length})`);
  const byKey = new Map(existing.map((row) => [row[keyIndex], row]));
  expectedRows.forEach((expected, index) => {
    const key = expectedKeys[index];
    assert(sameRow(byKey.get(key), expected), `${label}: ligne ${key} differente`);
  });
}

function prepareCharStats(document, changed) {
  const headerIndexes = indexes(document.table.headers);
  assert(headerIndexes.class !== undefined, 'charstats: colonne class absente');
  assert(headerIndexes['Skill 9'] !== undefined, 'charstats: colonne Skill 9 absente');
  assert(headerIndexes['Skill 10'] !== undefined, 'charstats: colonne Skill 10 absente');
  const byClass = new Map(document.table.rows.map((row) => [row[headerIndexes.class], row]));
  for (const spec of COMMAND_SPECS) {
    const row = byClass.get(spec.className);
    assert(row, `charstats: classe ${spec.className} absente`);
    const skill9 = row[headerIndexes['Skill 9']] ?? '';
    const skill10 = row[headerIndexes['Skill 10']] ?? '';

    // SuperTK reprend le slot 10 historique de TCP et deplace Command au slot 9.
    // Le generateur Command doit reconnaitre les deux etats pour rester composable.
    if (skill10 === 'SuperTK') {
      const displaced = spec.className === 'Barbarian' ? 'Unsummon' : '';
      assert(
        skill9 === spec.skill || skill9 === displaced,
        `charstats: Skill 9 de ${spec.className} occupe par ${skill9}`,
      );
      assert(!CHECK_ONLY || skill9 === spec.skill, `charstats: Command absent pour ${spec.className}`);
      if (skill9 !== spec.skill && !CHECK_ONLY) {
        row[headerIndexes['Skill 9']] = spec.skill;
        if (!changed.includes('charstats.txt')) changed.push('charstats.txt');
      }
      continue;
    }

    assert(skill10 === '' || skill10 === spec.skill, `charstats: Skill 10 de ${spec.className} occupe par ${skill10}`);
    assert(!CHECK_ONLY || skill10 === spec.skill, `charstats: Command absent pour ${spec.className}`);
    if (skill10 === '' && !CHECK_ONLY) {
      row[headerIndexes['Skill 10']] = spec.skill;
      if (!changed.includes('charstats.txt')) changed.push('charstats.txt');
    }
  }
}

function validateHirelings(document) {
  const headerIndexes = indexes(document.table.headers);
  for (const header of [
    'Hireling', 'Version', 'Difficulty', 'Level',
    'Skill1', 'Mode1', 'Chance1',
    'Skill2', 'Mode2', 'Chance2',
    'Skill3', 'Mode3', 'Chance3',
    'Skill4', 'Mode4', 'Chance4',
  ]) {
    assert(headerIndexes[header] !== undefined, `hireling: colonne ${header} absente`);
  }

  const cell = (row, header) => row[headerIndexes[header]];
  const assertProfiles = (rows, keyForRow, expected, label) => {
    const actual = rows.map(keyForRow).sort();
    const wanted = [...expected].sort();
    assert(
      actual.length === wanted.length && actual.every((key, index) => key === wanted[index]),
      `hireling: profils ${label} inattendus; attendu [${wanted.join(', ')}], recu [${actual.join(', ')}]`,
    );
  };

  const rogues = document.table.rows.filter((row) => row[headerIndexes.Hireling] === 'Rogue Scout');
  assert(rogues.length === 24, `hireling: 24 lignes Rogue Scout attendues, recu ${rogues.length}`);
  for (const row of rogues) {
    assert(row[headerIndexes.Skill1] === 'Inner Sight', 'hireling: Skill1 Rogue doit rester Inner Sight');
    assert(row[headerIndexes.Mode1] === '4', 'hireling: Mode1 Rogue doit rester 4');
    assert(row[headerIndexes.Chance1] === '30', 'hireling: Chance1 Rogue doit rester 30 (TCP, sans spam)');
  }

  const desert = document.table.rows.filter((row) => row[headerIndexes.Hireling] === 'Desert Mercenary');
  assert(desert.length === 36, `hireling: 36 lignes Desert Mercenary attendues, recu ${desert.length}`);

  const legacyDesert = desert.filter((row) => cell(row, 'Version') === '0');
  const expansionDesert = desert.filter((row) => cell(row, 'Version') === '100');
  assert(
    legacyDesert.length + expansionDesert.length === desert.length,
    'hireling: seules les versions Desert Mercenary 0 et 100 sont supportees',
  );
  assertProfiles(
    legacyDesert,
    (row) => `${cell(row, 'Difficulty')}:${cell(row, 'Level')}:${cell(row, 'Skill2')}`,
    [
      '1:9:Prayer', '1:31:Prayer', '1:55:Prayer',
      '1:9:Defiance', '1:31:Defiance', '1:55:Defiance',
      '1:9:Blessed Aim', '1:31:Blessed Aim', '1:55:Blessed Aim',
      '2:31:Thorns', '2:55:Thorns',
      '2:31:Holy Freeze', '2:55:Holy Freeze',
      '2:31:Might', '2:55:Might',
      '3:55:Prayer', '3:55:Defiance', '3:55:Blessed Aim',
    ],
    'Desert legacy',
  );
  for (const row of legacyDesert) {
    assert(cell(row, 'Skill1') === 'Jab', 'hireling: Skill1 Desert legacy doit rester Jab');
    assert(cell(row, 'Mode1') === '14', 'hireling: Mode1 Desert legacy doit rester 14');
    assert(cell(row, 'Mode2') === '4', 'hireling: aura Skill2 Desert legacy doit rester en Mode2=4');
    assert(cell(row, 'Chance2') === '9999', 'hireling: aura Skill2 Desert legacy doit rester en Chance2=9999');
  }

  const expansionCombat = expansionDesert.filter((row) => cell(row, 'Skill1') === 'Jab');
  const expansionAura = expansionDesert.filter((row) => cell(row, 'Skill1') !== 'Jab');
  assertProfiles(
    expansionCombat,
    (row) => `${cell(row, 'Difficulty')}:${cell(row, 'Level')}`,
    ['1:9', '1:43', '1:75', '2:43', '2:75', '3:75'],
    'Desert expansion Combat',
  );
  for (const row of expansionCombat) {
    assert(cell(row, 'Mode1') === '14' && cell(row, 'Chance1') === '60', 'hireling: Jab Combat doit rester Mode1=14 Chance1=60');
    assert(cell(row, 'Skill2') === 'Fend' && cell(row, 'Mode2') === '4' && cell(row, 'Chance2') === '30', 'hireling: Fend Combat doit rester Skill2 Mode2=4 Chance2=30');
    assert(cell(row, 'Skill3') === 'BKV Desert Smite' && cell(row, 'Mode3') === '4' && cell(row, 'Chance3') === '10', 'hireling: BKV Desert Smite doit rester Skill3 Mode3=4 Chance3=10');
    assert(cell(row, 'Skill4') === 'BKV Desert Mastery' && cell(row, 'Mode4') === '1' && cell(row, 'Chance4') === '1', 'hireling: BKV Desert Mastery doit rester Skill4 Mode4=1 Chance4=1');
  }

  assertProfiles(
    expansionAura,
    (row) => `${cell(row, 'Difficulty')}:${cell(row, 'Level')}:${cell(row, 'Skill1')}`,
    [
      '1:9:Defiance', '1:43:Defiance', '1:75:Defiance',
      '1:9:Blessed Aim', '1:43:Blessed Aim', '1:75:Blessed Aim',
      '2:43:Prayer', '2:75:Prayer',
      '2:43:Thorns', '2:75:Thorns',
      '3:75:Holy Freeze', '3:75:Might',
    ],
    'Desert expansion Aura',
  );
  for (const row of expansionAura) {
    assert(cell(row, 'Mode1') === '4' && cell(row, 'Chance1') === '9999', 'hireling: aura Desert expansion doit rester Skill1 Mode1=4 Chance1=9999');
    assert(cell(row, 'Skill2') === 'Jab' && cell(row, 'Mode2') === '14' && cell(row, 'Chance2') === '30', 'hireling: Jab Desert expansion doit rester Skill2 Mode2=14 Chance2=30');
    assert(cell(row, 'Skill3') === 'Iron Skin' && cell(row, 'Mode3') === '1' && cell(row, 'Chance3') === '1', 'hireling: Iron Skin Desert expansion doit rester Skill3 Mode3=1 Chance3=1');
    assert(cell(row, 'Skill4') === '' && cell(row, 'Mode4') === '' && cell(row, 'Chance4') === '', 'hireling: Skill4 Desert expansion Aura doit rester vide');
  }
}

function commandSkillRows(table) {
  return COMMAND_SPECS.map((spec) => rowFromObject(table, {
    skill: spec.skill,
    '*Id': '9999',
    skilldesc: 'command',
    srvdofunc: '155',
    auralencalc: '0',
    aurarangecalc: '38',
    pettype: 'hireable',
    requirespettype: '1',
    sumoverlay: spec.overlay,
    stsound: spec.startSound,
    dosound: spec.castSound,
    attackrank: '9',
    range: 'none',
    anim: 'SC',
    seqtrans: 'SC',
    monanim: 'NU',
    reqlevel: '1',
    restrict: spec.restrict ?? '',
    localdelay: '25',
    leftskill: '0',
    rightskill: '1',
    minmana: '0',
    manashift: '8',
    mana: '0',
    lvlmana: '0',
    interrupt: '1',
    calc1: '0',
    calc2: '1',
    InGame: '1',
    HitShift: '8',
    '*eol': '0',
  }, spec.skill));
}

function sourceRows(source, target, keyHeader, keys, label) {
  const sourceKeyIndex = indexes(source.table.headers)[keyHeader];
  assert(sourceKeyIndex !== undefined, `${label}: colonne ${keyHeader} absente`);
  const byKey = new Map(source.table.rows.map((row) => [row[sourceKeyIndex], row]));
  return keys.map((key) => {
    const row = byKey.get(key);
    assert(row, `${label}: ligne ${key} absente`);
    return mapSourceRow(source, target, row);
  });
}

function stripBom(raw) {
  return raw.startsWith('\uFEFF') ? raw.slice(1) : raw;
}

function readJsonArray(filePath, label) {
  assert(fs.existsSync(filePath), `${label} absent: ${filePath}`);
  const raw = fs.readFileSync(filePath, 'utf8');
  const bom = raw.startsWith('\uFEFF') ? '\uFEFF' : '';
  const body = stripBom(raw);
  const data = JSON.parse(body);
  assert(Array.isArray(data), `${label}: tableau JSON attendu`);
  const eol = body.includes('\r\n') ? '\r\n' : '\n';
  return { filePath, raw, bom, body, data, eol };
}

function cloneJson(value) {
  return JSON.parse(JSON.stringify(value));
}

function appendJsonEntries(document, entries, changed) {
  const closing = document.body.lastIndexOf(']');
  assert(closing >= 0, 'skills.json: fermeture absente');
  const before = document.body.slice(0, closing).replace(/\s*$/, '');
  const after = document.body.slice(closing + 1);
  const separator = before.endsWith('[') ? '' : ',';
  const rendered = entries
    .map((entry) => JSON.stringify(entry, null, 2).split('\n').map((line) => `  ${line}`).join(document.eol))
    .join(`,${document.eol}`);
  document.body = `${before}${separator}${document.eol}${rendered}${document.eol}]${after}`;
  document.raw = `${document.bom}${document.body}`;
  document.data = JSON.parse(document.body);
  changed.push('local/lng/strings/skills.json');
}

function usedStringIds(directory) {
  const used = new Map();
  for (const name of fs.readdirSync(directory).filter((entry) => entry.endsWith('.json'))) {
    const document = readJsonArray(path.join(directory, name), `strings/${name}`);
    for (const entry of document.data) {
      if (!Number.isFinite(Number(entry.id))) continue;
      const id = Number(entry.id);
      if (!used.has(id)) used.set(id, []);
      used.get(id).push(`${name}:${entry.Key}`);
    }
  }
  return used;
}

function prepareStrings(source, target, changed) {
  const sourceByKey = new Map(source.data.map((entry) => [entry.Key, entry]));
  const targetByKey = new Map(target.data.map((entry) => [entry.Key, entry]));
  const expected = COMMAND_STRING_KEYS.map((key) => {
    const entry = sourceByKey.get(key);
    assert(entry, `skills.json TCP: string ${key} absente`);
    const clone = cloneJson(entry);
    clone.id = COMMAND_STRING_IDS[key];
    return clone;
  });
  const present = COMMAND_STRING_KEYS.filter((key) => targetByKey.has(key));
  assert(present.length === 0 || present.length === expected.length, `skills.json: portage partiel (${present.length}/${expected.length})`);
  if (present.length === 0) {
    assert(!CHECK_ONLY, 'skills.json: strings Command absentes');
    const used = usedStringIds(FILES.target.stringsDirectory);
    for (const entry of expected) {
      const collision = used.get(Number(entry.id));
      assert(!collision, `skills.json: collision ID ${entry.id} avec ${(collision ?? []).join(', ')}`);
    }
    if (!CHECK_ONLY) appendJsonEntries(target, expected, changed);
    return;
  }
  for (const entry of expected) {
    assert(JSON.stringify(targetByKey.get(entry.Key)) === JSON.stringify(entry), `skills.json: string ${entry.Key} differente`);
  }
}

function sha256(filePath) {
  let content = fs.readFileSync(filePath);
  if (path.extname(filePath).toLowerCase() === '.json') {
    content = Buffer.from(content.toString('utf8').replace(/\r\n/g, '\n'), 'utf8');
  }
  return crypto.createHash('sha256').update(content).digest('hex').toUpperCase();
}

function prepareAssets(changed, preserved) {
  for (const relative of ASSETS) {
    const source = path.join(SOURCE_ROOT, ...relative.split('/'));
    const target = path.join(TARGET_ROOT, ...relative.split('/'));
    assert(fs.existsSync(source), `asset TCP absent: ${relative}`);
    if (!fs.existsSync(target)) {
      assert(!CHECK_ONLY, `asset BKVince absent: ${relative}`);
      if (!CHECK_ONLY) {
        fs.mkdirSync(path.dirname(target), { recursive: true });
        fs.copyFileSync(source, target);
        changed.push(relative);
      }
      continue;
    }
    if (sha256(source) !== sha256(target)) {
      assert(PRESERVED_ASSET_OVERRIDES.includes(relative), `asset BKVince different: ${relative}`);
      preserved.push(relative);
    }
  }
}

function writeTableIfChanged(document) {
  const output = serializeTable(document.table);
  if (output === document.raw) return false;
  writeTable(document.filePath, document.table);
  return true;
}

function writeJsonIfChanged(document) {
  if (document.raw === fs.readFileSync(document.filePath, 'utf8')) return false;
  const temporary = `${document.filePath}.tmp`;
  fs.writeFileSync(temporary, document.raw, 'utf8');
  fs.renameSync(temporary, document.filePath);
  return true;
}

function main() {
  const source = {
    overlay: loadTable(FILES.source.overlay, 'overlay TCP'),
    skillDesc: loadTable(FILES.source.skillDesc, 'skilldesc TCP'),
    strings: readJsonArray(FILES.source.strings, 'skills.json TCP'),
  };
  const target = {
    charStats: loadTable(FILES.target.charStats, 'charstats BKVince'),
    hireling: loadTable(FILES.target.hireling, 'hireling BKVince'),
    overlay: loadTable(FILES.target.overlay, 'overlay BKVince'),
    skillDesc: loadTable(FILES.target.skillDesc, 'skilldesc BKVince'),
    skills: loadTable(FILES.target.skills, 'skills BKVince'),
    strings: readJsonArray(FILES.target.strings, 'skills.json BKVince'),
  };
  const changed = [];
  const preservedAssets = [];

  prepareCharStats(target.charStats, changed);
  validateHirelings(target.hireling);

  appendOrValidateRows(
    target.skills,
    'skill',
    commandSkillRows(target.skills.table),
    COMMAND_SKILLS,
    'skills.txt',
    changed,
  );
  appendOrValidateRows(
    target.overlay,
    'overlay',
    sourceRows(source.overlay, target.overlay, 'overlay', COMMAND_OVERLAYS, 'overlay TCP'),
    COMMAND_OVERLAYS,
    'overlay.txt',
    changed,
  );
  appendOrValidateRows(
    target.skillDesc,
    'skilldesc',
    sourceRows(source.skillDesc, target.skillDesc, 'skilldesc', ['command'], 'skilldesc TCP'),
    ['command'],
    'skilldesc.txt',
    changed,
  );
  prepareStrings(source.strings, target.strings, changed);
  prepareAssets(changed, preservedAssets);

  if (CHECK_ONLY) {
    assert(changed.length === 0, `portage incomplet: ${changed.join(', ')}`);
  } else {
    writeTableIfChanged(target.charStats);
    writeTableIfChanged(target.overlay);
    writeTableIfChanged(target.skillDesc);
    writeTableIfChanged(target.skills);
    writeJsonIfChanged(target.strings);
  }

  const finalTables = {
    charStats: loadTable(FILES.target.charStats, 'charstats final'),
    hireling: loadTable(FILES.target.hireling, 'hireling final'),
    overlay: loadTable(FILES.target.overlay, 'overlay final'),
    skillDesc: loadTable(FILES.target.skillDesc, 'skilldesc final'),
    skills: loadTable(FILES.target.skills, 'skills final'),
  };
  if (!CHECK_ONLY) {
    // Le premier passage a prepare les documents en memoire; le second valide
    // les bytes reellement ecrits sans appliquer une nouvelle mutation.
    prepareCharStats(finalTables.charStats, []);
    validateHirelings(finalTables.hireling);
  }

  console.log(JSON.stringify({
    mode: CHECK_ONLY ? 'check' : 'write',
    changed,
    commandSkills: COMMAND_SKILLS.length,
    classes: COMMAND_SPECS.length,
    rogueAct1Chance1: 30,
    desertMercenaryProfiles: {
      legacyAura: 18,
      expansionAura: 12,
      expansionCombat: 6,
    },
    assets: ASSETS.length,
    preservedAssets,
    strings: COMMAND_STRING_KEYS.length,
  }, null, 2));
}

main();
