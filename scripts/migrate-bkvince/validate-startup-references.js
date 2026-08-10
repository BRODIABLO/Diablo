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

function assertTreasureClassOrdering(table, indexes, itemCodes, itemTypeTokens) {
  const tableName = 'treasureclassex.txt';
  const positions = new Map();

  for (let rowIndex = 0; rowIndex < table.rows.length; rowIndex += 1) {
    const name = value(table.rows[rowIndex], indexes, 'Treasure Class', tableName);
    if (!name) continue;
    assert(!positions.has(name), `${tableName}: Treasure Class dupliquee: ${name}`);
    positions.set(name, rowIndex);
  }

  for (let rowIndex = 0; rowIndex < table.rows.length; rowIndex += 1) {
    const row = table.rows[rowIndex];
    const parent = value(row, indexes, 'Treasure Class', tableName);
    for (let slot = 1; slot <= 10; slot += 1) {
      const item = value(row, indexes, `Item${slot}`, tableName);
      if (!item) continue;
      const childIndex = positions.get(item);
      if (childIndex !== undefined) {
        assert(
          childIndex < rowIndex,
          `${tableName}: ${parent} reference ${item} avant sa declaration`,
        );
        continue;
      }

      const itemCode = item.split(',', 1)[0];
      const generatedType = /^(.*?)(\d+)$/.exec(itemCode);
      assert(
        itemCodes.has(itemCode)
          || (generatedType && itemTypeTokens.has(generatedType[1])),
        `${tableName}: ${parent} contient une reference inconnue: ${item}`,
      );
    }
  }
}

function main() {
  const hireling = load('hireling.txt');
  const skills = load('skills.txt');
  const skilldesc = load('skilldesc.txt');
  const states = load('states.txt');
  const missiles = load('missiles.txt');
  const treasureClasses = load('treasureclassex.txt');
  const armor = load('armor.txt');
  const itemTypes = load('itemtypes.txt');
  const misc = load('misc.txt');
  const setItems = load('setitems.txt');
  const uniqueItems = load('uniqueitems.txt');
  const weapons = load('weapons.txt');
  const hirelingIndexes = headerIndexes(hireling);
  const skillsIndexes = headerIndexes(skills);
  const skilldescIndexes = headerIndexes(skilldesc);
  const treasureIndexes = headerIndexes(treasureClasses);
  const itemCodes = new Set();
  const itemTypeTokens = new Set();
  for (const [table, tableName] of [
    [armor, 'armor.txt'],
    [misc, 'misc.txt'],
    [weapons, 'weapons.txt'],
  ]) {
    const indexes = headerIndexes(table);
    for (const row of table.rows) {
      const code = value(row, indexes, 'code', tableName);
      if (code) itemCodes.add(code);
    }
  }
  for (const [table, tableName] of [
    [setItems, 'setitems.txt'],
    [uniqueItems, 'uniqueitems.txt'],
  ]) {
    const indexes = headerIndexes(table);
    for (const row of table.rows) {
      const index = value(row, indexes, 'index', tableName);
      if (index) itemCodes.add(index);
    }
  }

  const itemTypeIndexes = headerIndexes(itemTypes);
  for (const row of itemTypes.rows) {
    for (const header of ['Code', 'Equiv1', 'Equiv2']) {
      const token = value(row, itemTypeIndexes, header, 'itemtypes.txt');
      if (token) itemTypeTokens.add(token);
    }
  }

  assertTreasureClassOrdering(
    treasureClasses,
    treasureIndexes,
    itemCodes,
    itemTypeTokens,
  );

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
  assert(value(essence, treasureIndexes, 'Picks', 'treasureclassex.txt') === '6',
    'treasureclassex.txt: Picks invalide pour Andariel Essence (H)');
  assert(value(essence, treasureIndexes, 'NoDrop', 'treasureclassex.txt') === '982',
    'treasureclassex.txt: NoDrop invalide pour Andariel Essence (H)');
  assert(value(essence, treasureIndexes, 'Item1', 'treasureclassex.txt') === 'tes',
    'treasureclassex.txt: Item1 invalide pour Andariel Essence (H)');
  assert(value(essence, treasureIndexes, 'Prob1', 'treasureclassex.txt') === '15',
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
  console.log(`  Eruption -> ${missileMatch[1]}`);
  console.log(`  Treasure Classes -> toutes resolues dans l'ordre (${itemCodes.size} items resolvables)`);
  console.log('  Andariel (H) -> Andariel Essence (H) -> tes (6 picks, NoDrop 982, poids 15)');
  console.log('  Rift Crafts (N) Premium precede Rift Crafts Premium');
}

main();
