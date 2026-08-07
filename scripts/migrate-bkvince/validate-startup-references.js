'use strict';

// Controle cible des references qui faisaient echouer les assertions de
// chargement de BKVince sous D2RLoader 3.2. Ce script est strictement en
// lecture seule et verifie aussi le round-trip byte-exact des tables.

const fs = require('fs');
const path = require('path');
const { parseTable, serializeTable } = require('../build-data/tsv');

const ROOT = path.resolve(__dirname, '..', '..');
const EXCEL = path.join(
  ROOT,
  'data-BKVince',
  'BKVince.mpq',
  'data',
  'global',
  'excel',
);

function assert(condition, message) {
  if (!condition) throw new Error(message);
}

function load(name) {
  const filePath = path.join(EXCEL, name);
  const raw = fs.readFileSync(filePath, 'latin1');
  const table = parseTable(filePath);
  assert(raw === serializeTable(table), `Round-trip non byte-exact: ${filePath}`);
  assert(table.eol === '\r\n', `Fin de ligne non CRLF: ${filePath}`);
  return table;
}

function headerIndexes(table) {
  return new Map(table.headers.map((header, index) => [header, index]));
}

function uniqueRow(table, key, tableName) {
  const rows = table.rows.filter((row) => row[0] === key);
  assert(rows.length === 1, `${tableName}: ${key} attendu une fois, trouve ${rows.length}`);
  return rows[0];
}

function value(row, indexes, header, tableName) {
  const index = indexes.get(header);
  assert(index !== undefined, `${tableName}: colonne ${header} absente`);
  return row[index] ?? '';
}

function main() {
  const hireling = load('hireling.txt');
  const skills = load('skills.txt');
  const skilldesc = load('skilldesc.txt');
  const states = load('states.txt');
  const missiles = load('missiles.txt');
  const monstats = load('monstats.txt');
  const treasureClasses = load('treasureclassex.txt');
  const hirelingIndexes = headerIndexes(hireling);
  const skillsIndexes = headerIndexes(skills);
  const skilldescIndexes = headerIndexes(skilldesc);
  const monstatsIndexes = headerIndexes(monstats);
  const treasureIndexes = headerIndexes(treasureClasses);

  const hirelingSkillNames = new Set();
  for (const row of hireling.rows) {
    for (let slot = 1; slot <= 6; slot += 1) {
      const skillName = value(row, hirelingIndexes, `Skill${slot}`, 'hireling.txt');
      if (skillName) hirelingSkillNames.add(skillName);
    }
  }
  for (const skillName of hirelingSkillNames) {
    const skill = uniqueRow(skills, skillName, 'skills.txt');
    const description = value(skill, skillsIndexes, 'skilldesc', 'skills.txt');
    assert(description, `skills.txt: ${skillName} utilise par hireling.txt sans skilldesc`);
    uniqueRow(skilldesc, description, 'skilldesc.txt');
  }

  const auraAiSkills = skills.rows.filter((row) => (
    value(row, skillsIndexes, 'aitype', 'skills.txt') === '1'
  ));
  for (const skill of auraAiSkills) {
    const skillName = value(skill, skillsIndexes, 'skill', 'skills.txt');
    const auraState = value(skill, skillsIndexes, 'aurastate', 'skills.txt');
    assert(auraState, `skills.txt: ${skillName} utilise aitype=1 sans aurastate`);
    uniqueRow(states, auraState, 'states.txt');
  }

  for (const ravenName of ['BKV Fire Raven', 'BKV Cold Raven']) {
    const raven = uniqueRow(skills, ravenName, 'skills.txt');
    assert(value(raven, skillsIndexes, 'aitype', 'skills.txt') === '1',
      `skills.txt: ${ravenName}.aitype doit rester un buff IA`);
    assert(value(raven, skillsIndexes, 'aurastate', 'skills.txt') === 'bkvrogueraven',
      `skills.txt: ${ravenName}.aurastate invalide`);
    assert(value(raven, skillsIndexes, 'srvstfunc', 'skills.txt') === '28',
      `skills.txt: ${ravenName}.srvstfunc invalide`);
    assert(value(raven, skillsIndexes, 'srvdofunc', 'skills.txt') === '44',
      `skills.txt: ${ravenName}.srvdofunc invalide`);
    assert(value(raven, skillsIndexes, 'srvmissilea', 'skills.txt')
      === 'blade shield attachment',
    `skills.txt: ${ravenName}.srvmissilea invalide`);
    assert(value(raven, skillsIndexes, 'sumskill1', 'skills.txt')
      === 'BKV Rogue Raven Aura',
    `skills.txt: ${ravenName}.sumskill1 invalide`);
    assert(value(raven, skillsIndexes, 'sumsk1calc', 'skills.txt') === '1',
      `skills.txt: ${ravenName}.sumsk1calc invalide`);
    assert(value(raven, skillsIndexes, 'pettype', 'skills.txt') === 'bkvrogueraven',
      `skills.txt: ${ravenName}.pettype invalide`);
    assert(value(raven, skillsIndexes, 'petmax', 'skills.txt') === '1',
      `skills.txt: ${ravenName}.petmax invalide`);
    assert(value(raven, skillsIndexes, 'Param5', 'skills.txt') === '',
      `skills.txt: ${ravenName}.Param5 doit rester vide sur le chemin Blade Sentinel`);
  }
  uniqueRow(states, 'bkvrogueraven', 'states.txt');
  uniqueRow(missiles, 'blade shield attachment', 'missiles.txt');
  const ravenAura = uniqueRow(skills, 'BKV Rogue Raven Aura', 'skills.txt');
  assert(value(ravenAura, skillsIndexes, 'srvdofunc', 'skills.txt') === '65',
    'skills.txt: BKV Rogue Raven Aura.srvdofunc invalide');
  assert(value(ravenAura, skillsIndexes, 'aurastate', 'skills.txt') === 'bkvrogueraven',
    'skills.txt: BKV Rogue Raven Aura.aurastate invalide');
  assert(value(ravenAura, skillsIndexes, 'auratargetstate', 'skills.txt') === 'bkvrogueraven',
    'skills.txt: BKV Rogue Raven Aura.auratargetstate invalide');

  for (const [monsterName, skillName] of [
    ['bkvfireraven', 'BKV Fire Raven'],
    ['bkvcoldraven', 'BKV Cold Raven'],
  ]) {
    const monster = uniqueRow(monstats, monsterName, 'monstats.txt');
    assert(value(monster, monstatsIndexes, 'AI', 'monstats.txt') === 'NecroPet',
      `monstats.txt: ${monsterName}.AI invalide`);
    assert(value(monster, monstatsIndexes, 'MonStatsEx', 'monstats.txt') === 'druidhawk',
      `monstats.txt: ${monsterName}.MonStatsEx doit reutiliser le rendu vanilla druidhawk`);
    assert(value(monster, monstatsIndexes, 'Skill1', 'monstats.txt') === '',
      `monstats.txt: ${monsterName}.Skill1 doit rester vide avec NecroPet`);
    assert(value(monster, monstatsIndexes, 'aip1', 'monstats.txt') === '90',
      `monstats.txt: ${monsterName}.aip1 invalide`);
  }

  const eruption = uniqueRow(skilldesc, 'eruption', 'skilldesc.txt');
  const eruptionFormula = value(
    eruption,
    skilldescIndexes,
    'dsc2calca3',
    'skilldesc.txt',
  );
  const missileMatch = /^miss\('([^']+)'\.rang\)$/.exec(eruptionFormula);
  assert(missileMatch, `skilldesc.txt: formule Eruption invalide: ${eruptionFormula}`);
  uniqueRow(missiles, missileMatch[1], 'missiles.txt');

  const essence = uniqueRow(
    treasureClasses,
    'Andariel Essence (H)',
    'treasureclassex.txt',
  );
  assert(value(essence, treasureIndexes, 'Picks', 'treasureclassex.txt') === '1',
    'treasureclassex.txt: Picks invalide pour Andariel Essence (H)');
  assert(value(essence, treasureIndexes, 'Item1', 'treasureclassex.txt') === 'tes',
    'treasureclassex.txt: Item1 invalide pour Andariel Essence (H)');
  assert(value(essence, treasureIndexes, 'Prob1', 'treasureclassex.txt') === '1',
    'treasureclassex.txt: Prob1 invalide pour Andariel Essence (H)');

  const andarielHell = uniqueRow(treasureClasses, 'Andariel (H)', 'treasureclassex.txt');
  assert(value(andarielHell, treasureIndexes, 'Item1', 'treasureclassex.txt')
    === 'Andariel Essence (H)',
  'treasureclassex.txt: Andariel (H) ne reference pas Andariel Essence (H)');
  assert(value(andarielHell, treasureIndexes, 'Prob1', 'treasureclassex.txt') === '1',
    'treasureclassex.txt: probabilite de la classe essence invalide pour Andariel (H)');

  const riftNightmare = uniqueRow(
    treasureClasses,
    'Rift Crafts (N) Premium',
    'treasureclassex.txt',
  );
  const riftBase = uniqueRow(
    treasureClasses,
    'Rift Crafts Premium',
    'treasureclassex.txt',
  );
  assert(treasureClasses.rows.indexOf(riftNightmare) < treasureClasses.rows.indexOf(riftBase),
    'treasureclassex.txt: Rift Crafts (N) Premium doit preceder son appelant');
  assert(value(riftBase, treasureIndexes, 'Item1', 'treasureclassex.txt')
    === 'Rift Crafts (N) Premium',
  'treasureclassex.txt: la progression Normal -> Nightmare des crafts Rift est invalide');

  console.log('VALID : references BKVince de demarrage resolues');
  console.log(`  Hireling skills -> ${hirelingSkillNames.size} skilldesc resolus`);
  console.log(`  Aura AI skills -> ${auraAiSkills.length} aurastate resolus`);
  console.log('  BKV Fire/Cold Raven -> cycle TDE + rendu vanilla druidhawk resolus');
  console.log(`  Eruption -> ${missileMatch[1]}`);
  console.log('  Andariel (H) -> Andariel Essence (H) -> tes');
  console.log('  Rift Crafts (N) Premium precede Rift Crafts Premium');
}

main();
