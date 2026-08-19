import fs from 'node:fs';
import path from 'node:path';
import { createHash } from 'node:crypto';
import { fileURLToPath } from 'node:url';

import { readConstantData } from '@d2runewizard/d2s';
import tsv from '../../../scripts/build-data/tsv.js';

const { ENCODING, parseTable, serializeTable } = tsv;
const scriptDirectory = path.dirname(fileURLToPath(import.meta.url));
const repositoryRoot = path.resolve(scriptDirectory, '..', '..', '..');
const excelRoot = path.join(
  repositoryRoot,
  'data-BKVince',
  'BKVince.mpq',
  'data',
  'global',
  'excel',
);
const vanillaExcelRoot = path.join(
  repositoryRoot,
  'data-vanilla3.3',
  'data',
  'data',
  'global',
  'excel',
);
const stringsRoot = path.join(
  repositoryRoot,
  'data-BKVince',
  'BKVince.mpq',
  'data',
  'local',
  'lng',
  'strings',
);
const outputPath = path.resolve(scriptDirectory, '..', 'src', 'data', 'bkvince-constants.generated.js');

const tableNames = Object.freeze([
  'CharStats',
  'SkillDesc',
  'skills',
  'MagicPrefix',
  'MagicSuffix',
  'Properties',
  'ItemStatCost',
  'Runes',
  'SetItems',
  'UniqueItems',
  'ItemTypes',
  'Armor',
  'Weapons',
  'Misc',
  'Gems',
  'Hireling',
  'MonStats',
  'SuperUniques',
  'MonUMod',
]);

const inheritedTableNames = Object.freeze([
  'LowQualityItems',
  'RarePrefix',
  'RareSuffix',
]);

const stringNames = Object.freeze([
  'item-gems',
  'item-modifiers',
  'item-nameaffixes',
  'item-names',
  'item-runes',
  'skills',
  'monsters',
]);

function readGovernedTable(filePath) {
  const parsed = parseTable(filePath);
  const serialized = Buffer.from(serializeTable(parsed), ENCODING);
  const original = fs.readFileSync(filePath);
  if (!serialized.equals(original)) {
    throw new Error(`TSV round-trip is not byte-exact: ${filePath}`);
  }
  return { parsed, text: original.toString('utf8') };
}

function buildConstants() {
  const buffers = {};
  const tables = {};
  for (const name of tableNames) {
    const filePath = path.join(excelRoot, `${name.toLowerCase()}.txt`);
    const governed = readGovernedTable(filePath);
    buffers[`${name}.txt`] = governed.text;
    tables[name] = governed.parsed;
  }
  for (const name of inheritedTableNames) {
    const filePath = path.join(vanillaExcelRoot, `${name.toLowerCase()}.txt`);
    const governed = readGovernedTable(filePath);
    buffers[`${name}.txt`] = governed.text;
    tables[name] = governed.parsed;
  }
  for (const name of stringNames) {
    const filePath = path.join(stringsRoot, `${name}.json`);
    buffers[`${name}.json`] = fs.readFileSync(filePath, 'utf8');
  }

  // This table is a D2S ABI lookup. Gameplay values remain sourced from
  // BKVince charstats.txt; blank rows preserve the numeric class identifiers.
  buffers['PlayerClass.txt'] = [
    'Code',
    'ama',
    'sor',
    'nec',
    'pal',
    'bar',
    '',
    'dru',
    'ass',
    'war',
    '',
  ].join('\r\n');
  const constants = readConstantData(buffers);
  constants.classes.filter(Boolean).forEach((entry) => {
    entry.as = normalizeSignedFormat(entry.as);
    entry.ts = (entry.ts || []).map((label) => normalizeSignedFormat(label));
  });
  const classes = constants.classes.filter(Boolean);
  const duplicateNames = classes.filter(
    (candidate, index) => classes.findIndex((entry) => entry.n === candidate.n) !== index,
  );
  if (classes.length === 0 || duplicateNames.length > 0) {
    throw new Error('BKVince class constants are empty or ambiguous.');
  }
  return { constants, tables, buffers };
}

function normalizeSignedFormat(value) {
  return typeof value === 'string' ? value.replaceAll('+%d', '%+d') : value;
}

export function buildItemCatalog(tables, constants) {
  const itemTypes = buildItemTypeRows(tables);
  const itemTypeMap = new Map(itemTypes.map((entry) => [entry.code, entry]));
  const baseSpecs = [
    ['Armor', 1, ['name', 'code', 'compactsave', 'spawnable', 'level', 'levelreq', 'durability', 'invwidth', 'invheight', 'stackable', 'maxstack', 'type', 'normcode', 'ubercode', 'ultracode', 'minac', 'maxac', 'belt']],
    ['Weapons', 3, ['name', 'code', 'compactsave', 'spawnable', 'level', 'levelreq', 'durability', 'invwidth', 'invheight', 'stackable', 'maxstack', 'type', 'normcode', 'ubercode', 'ultracode']],
    ['Misc', 4, ['name', 'code', 'compactsave', 'spawnable', 'level', 'levelreq', 'invwidth', 'invheight', 'stackable', 'maxstack', 'type', 'normcode', 'ubercode', 'ultracode', 'belt']],
  ];
  const bases = [];
  const baseCodes = new Map();

  for (const [source, typeId, headers] of baseSpecs) {
    const table = requireTable(tables, source, headers);
    const columns = headerMap(table);
    table.rows.forEach((row, rowIndex) => {
      const code = valueAt(row, columns, 'code').trim();
      if (!code) return;
      if (baseCodes.has(code)) {
        const previous = baseCodes.get(code);
        throw new Error(
          `Ambiguous BKVince item code ${code}: ${previous.source} row ${previous.row} and ${source} row ${rowIndex + 2}.`,
        );
      }
      const definition = itemDefinition(constants, code);
      if (!definition) {
        throw new Error(`BKVince item ${code} from ${source} row ${rowIndex + 2} is absent from codec constants.`);
      }
      const categories = Array.isArray(definition.c) ? [...definition.c] : [];
      const sourceName = valueAt(row, columns, 'name').trim();
      const itemType = valueAt(row, columns, 'type').trim();
      const itemTypeDefinition = itemTypeMap.get(itemType);
      if (!itemTypeDefinition) {
        throw new Error(`BKVince item ${code} references unknown ItemTypes code ${itemType}.`);
      }
      const pictures = Array.isArray(definition.ig)
        ? definition.ig.map((picture) => String(picture || '').trim()).filter(Boolean)
        : [];
      if (pictures.length > 8 || new Set(pictures).size !== pictures.length) {
        throw new Error(`BKVince item ${code} has an invalid native picture_id catalog.`);
      }
      const entry = {
        code,
        name: preferredCatalogItemName(definition.n, sourceName, code),
        sourceName,
        source,
        row: rowIndex + 2,
        typeId: categories.includes('Any Armor') ? 1 : (categories.includes('Weapon') ? 3 : typeId),
        itemType,
        typeCodes: itemTypeClosure(itemType, itemTypeMap),
        bodyLocations: [...itemTypeDefinition.bodyLocations],
        beltable: source === 'Misc' && flagAt(row, columns, 'belt'),
        beltLayout: source === 'Armor' ? integerAt(row, columns, 'belt') : null,
        compactSave: flagAt(row, columns, 'compactsave'),
        spawnable: flagAt(row, columns, 'spawnable'),
        stackable: flagAt(row, columns, 'stackable'),
        maxStack: integerAt(row, columns, 'maxstack'),
        maxSockets: integerAt(row, columns, 'gemsockets') ?? 0,
        width: positiveIntegerAt(row, columns, 'invwidth'),
        height: positiveIntegerAt(row, columns, 'invheight'),
        durability: source === 'Misc' ? null : integerAt(row, columns, 'durability'),
        defenseMinimum: source === 'Armor' ? integerAt(row, columns, 'minac') : null,
        defenseMaximum: source === 'Armor' ? integerAt(row, columns, 'maxac') : null,
        level: integerAt(row, columns, 'level'),
        levelRequirement: integerAt(row, columns, 'levelreq'),
        normalCode: valueAt(row, columns, 'normcode').trim() || null,
        exceptionalCode: valueAt(row, columns, 'ubercode').trim() || null,
        eliteCode: valueAt(row, columns, 'ultracode').trim() || null,
        icon: definition.i || null,
        pictures,
        categories,
      };
      bases.push(entry);
      baseCodes.set(code, entry);
    });
  }

  for (const base of bases) {
    if (base.beltable && (base.width !== 1 || base.height !== 1)) {
      throw new Error(`BKVince belt item ${base.code} must occupy exactly one native belt slot.`);
    }
    if (base.bodyLocations.includes('belt') && ![0, 1, 2, 3, 4, 5, 6].includes(base.beltLayout)) {
      throw new Error(`BKVince equipped belt ${base.code} references unsupported belt layout ${base.beltLayout}.`);
    }
    for (const [tier, code] of [
      ['normal', base.normalCode],
      ['exceptional', base.exceptionalCode],
      ['elite', base.eliteCode],
    ]) {
      if (!code) continue;
      const target = baseCodes.get(code);
      if (!target) {
        throw new Error(`BKVince item ${base.code} references unknown ${tier} tier code ${code}.`);
      }
      const sameStorageFamily = target.source === base.source
        && target.typeId === base.typeId
        && target.compactSave === base.compactSave
        && target.stackable === base.stackable
        && target.width === base.width
        && target.height === base.height;
      if (!sameStorageFamily) {
        throw new Error(
          `BKVince item ${base.code} ${tier} tier ${code} changes its D2S storage family.`,
        );
      }
    }
  }

  const uniqueItems = buildNamedQualityCatalog(
    tables,
    'UniqueItems',
    'code',
    12,
    baseCodes,
  );
  const setItems = buildNamedQualityCatalog(
    tables,
    'SetItems',
    'item',
    9,
    baseCodes,
  );
  const properties = buildPropertyRows(tables, constants);
  const runewords = buildRunewordRows(tables, constants);
  const itemSkills = buildItemSkillCatalog(tables, constants);
  const prefixes = attachAffixAbi(
    buildAffixRows(tables, 'MagicPrefix'),
    constants.magic_prefixes,
    'MagicPrefix',
  );
  const suffixes = attachAffixAbi(
    buildAffixRows(tables, 'MagicSuffix'),
    constants.magic_suffixes,
    'MagicSuffix',
  );
  const rareSuffixNames = buildRareNameRows(tables, constants, 'RareSuffix', 1);
  const rarePrefixNames = buildRareNameRows(
    tables,
    constants,
    'RarePrefix',
    rareSuffixNames.nextId,
  );
  const lowQualityNames = buildLowQualityRows(tables);

  return {
    bases: bases.sort((left, right) => left.code.localeCompare(right.code)),
    qualities: [
      { id: 1, name: 'Low' },
      { id: 2, name: 'Normal' },
      { id: 3, name: 'Superior' },
      { id: 4, name: 'Magic' },
      { id: 5, name: 'Set' },
      { id: 6, name: 'Rare' },
      { id: 7, name: 'Unique' },
      { id: 8, name: 'Crafted' },
    ],
    uniqueItems,
    setItems,
    prefixes,
    suffixes,
    rareNamePrefixes: rarePrefixNames.entries,
    rareNameSuffixes: rareSuffixNames.entries,
    lowQualityNames,
    properties,
    runewords,
    itemSkills,
    itemTypes,
  };
}

export function buildMercenaryCatalog(tables) {
  const skillHeaders = Array.from({ length: 6 }, (_, index) => `Skill${index + 1}`);
  const table = requireTable(tables, 'Hireling', [
    'Hireling', '*SubType', 'Id', 'Class', 'Act', 'Difficulty', 'Level',
    'Exp/Lvl', 'NameFirst', 'NameLast', 'equivalentcharclass', ...skillHeaders,
  ]);
  const columns = headerMap(table);
  const byId = new Map();

  table.rows.forEach((row, rowIndex) => {
    const id = integerAt(row, columns, 'Id');
    if (!Number.isInteger(id)) return;
    const level = integerAt(row, columns, 'Level');
    const experiencePerLevel = integerAt(row, columns, 'Exp/Lvl');
    const startingExperience = Number.isInteger(level) && Number.isInteger(experiencePerLevel)
      ? level * level * (level + 1) * experiencePerLevel
      : 0;
    const skills = skillHeaders
      .map((header) => valueAt(row, columns, header).trim())
      .filter(Boolean);
    const existing = byId.get(id);
    if (existing) {
      if (Number.isInteger(level)) {
        if (level < existing.minimumLevel
          || (level === existing.minimumLevel && startingExperience < existing.startingExperience)) {
          existing.experiencePerLevel = Number.isInteger(experiencePerLevel) ? experiencePerLevel : 0;
          existing.startingExperience = startingExperience;
        }
        existing.minimumLevel = Math.min(existing.minimumLevel, level);
        existing.maximumLevel = Math.max(existing.maximumLevel, level);
      }
      skills.forEach((skill) => {
        if (!existing.skills.includes(skill)) existing.skills.push(skill);
      });
      existing.rows.push(rowIndex + 2);
      return;
    }
    byId.set(id, {
      id,
      hireling: valueAt(row, columns, 'Hireling').trim() || `Mercenary ${id}`,
      subtype: valueAt(row, columns, '*SubType').trim() || null,
      classId: integerAt(row, columns, 'Class'),
      act: integerAt(row, columns, 'Act'),
      difficulty: integerAt(row, columns, 'Difficulty'),
      minimumLevel: Number.isInteger(level) ? level : 1,
      maximumLevel: Number.isInteger(level) ? level : 1,
      experiencePerLevel: Number.isInteger(experiencePerLevel) ? experiencePerLevel : 0,
      startingExperience,
      nameFirst: valueAt(row, columns, 'NameFirst').trim() || null,
      nameLast: valueAt(row, columns, 'NameLast').trim() || null,
      equivalentClass: valueAt(row, columns, 'equivalentcharclass').trim() || null,
      skills,
      rows: [rowIndex + 2],
    });
  });

  return [...byId.values()]
    .map((entry) => ({ ...entry, skills: [...entry.skills].sort() }))
    .sort((left, right) => left.id - right.id);
}

export function buildDemonCatalog(tables, buffers) {
  const monsterNames = localizedStrings(buffers, 'monsters');
  const monStats = requireTable(tables, 'MonStats', [
    'Id', '*hcIdx', 'NameStr', 'enabled', 'killable', 'npc', 'boss', 'primeevil',
    'CannotDesecrate',
  ]);
  const monStatsColumns = headerMap(monStats);
  const monsters = monStats.rows.flatMap((row, rowIndex) => {
    const id = valueAt(row, monStatsColumns, 'Id').trim();
    if (!id) return [];
    const nameKey = valueAt(row, monStatsColumns, 'NameStr').trim();
    return [{
      index: rowIndex + 1,
      row: rowIndex + 2,
      id,
      hcIndex: integerAt(row, monStatsColumns, '*hcIdx'),
      nameKey: nameKey || null,
      name: monsterNames.get(nameKey) || nameKey || id,
      enabled: flagAt(row, monStatsColumns, 'enabled'),
      killable: flagAt(row, monStatsColumns, 'killable'),
      npc: flagAt(row, monStatsColumns, 'npc'),
      boss: flagAt(row, monStatsColumns, 'boss'),
      primeEvil: flagAt(row, monStatsColumns, 'primeevil'),
      cannotDesecrate: flagAt(row, monStatsColumns, 'CannotDesecrate'),
    }];
  });

  const superUniquesTable = requireTable(tables, 'SuperUniques', [
    'Superunique', 'Name', 'Class', 'hcIdx',
  ]);
  const superUniqueColumns = headerMap(superUniquesTable);
  const superUniques = superUniquesTable.rows.flatMap((row, rowIndex) => {
    const id = valueAt(row, superUniqueColumns, 'Superunique').trim();
    if (!id) return [];
    const nameKey = valueAt(row, superUniqueColumns, 'Name').trim();
    return [{
      index: rowIndex + 1,
      row: rowIndex + 2,
      id,
      nameKey: nameKey || null,
      name: monsterNames.get(nameKey) || nameKey || id,
      classId: valueAt(row, superUniqueColumns, 'Class').trim() || null,
      hcIndex: integerAt(row, superUniqueColumns, 'hcIdx'),
    }];
  });

  const modifiersTable = requireTable(tables, 'MonUMod', [
    'uniquemod', 'id', 'enabled', 'xfer', 'champion',
  ]);
  const modifierColumns = headerMap(modifiersTable);
  const modifierIds = new Set();
  const modifiers = modifiersTable.rows.flatMap((row, rowIndex) => {
    const internalName = valueAt(row, modifierColumns, 'uniquemod').trim();
    const id = integerAt(row, modifierColumns, 'id');
    if (!internalName || !Number.isInteger(id)) return [];
    if (modifierIds.has(id)) throw new Error(`Ambiguous MonUMod id ${id}.`);
    modifierIds.add(id);
    return [{
      id,
      row: rowIndex + 2,
      internalName,
      label: demonModifierLabel(internalName),
      enabled: flagAt(row, modifierColumns, 'enabled'),
      transferable: flagAt(row, modifierColumns, 'xfer'),
      champion: flagAt(row, modifierColumns, 'champion'),
    }];
  }).sort((left, right) => left.id - right.id);

  if (monsters.length === 0 || superUniques.length === 0 || modifiers.length === 0) {
    throw new Error('The governed BKVince Demon catalogs are empty.');
  }
  return { monsters, superUniques, modifiers };
}

function localizedStrings(buffers, name) {
  const source = buffers?.[`${name}.json`];
  if (typeof source !== 'string') throw new Error(`Missing governed BKVince localization ${name}.json.`);
  const records = JSON.parse(source.replace(/^\uFEFF/, ''));
  if (!Array.isArray(records)) throw new Error(`${name}.json must contain a localization array.`);
  const result = new Map();
  records.forEach((record) => {
    const key = String(record?.Key || '').trim();
    if (!key || result.has(key)) return;
    result.set(key, String(record.enUS || key));
  });
  return result;
}

function demonModifierLabel(internalName) {
  const labels = {
    none: 'None',
    strong: 'Extra Strong',
    fast: 'Extra Fast',
    curse: 'Cursed',
    resist: 'Magic Resistant',
    fire: 'Fire Enchanted',
    champion: 'Champion',
    lightning: 'Lightning Enchanted',
    cold: 'Cold Enchanted',
    manahit: 'Mana Burn',
    teleport: 'Teleportation',
    spectralhit: 'Spectral Hit',
    stoneskin: 'Stone Skin',
    multishot: 'Multiple Shots',
    aura: 'Aura Enchanted',
    ghostly: 'Ghostly',
    fanatic: 'Fanatic',
    possessed: 'Possessed',
    berserk: 'Berserker',
  };
  return labels[internalName] || internalName
    .replace(/_/g, ' ')
    .replace(/\b\w/g, (character) => character.toUpperCase());
}

function buildRunewordRows(tables, constants) {
  const modHeaders = Array.from({ length: 7 }, (_, index) => index + 1)
    .flatMap((slot) => [`T1Code${slot}`, `T1Param${slot}`, `T1Min${slot}`, `T1Max${slot}`]);
  const table = requireTable(tables, 'Runes', [
    'Name', '*Rune Name', 'complete', 'disallowCraftingInLadder',
    'disallowCraftingInNonLadder', 'firstLadderSeason', 'lastLadderSeason',
    '*Patch Release',
    ...Array.from({ length: 6 }, (_, index) => `itype${index + 1}`),
    ...Array.from({ length: 3 }, (_, index) => `etype${index + 1}`),
    '*RunesUsed',
    ...Array.from({ length: 6 }, (_, index) => `Rune${index + 1}`),
    ...modHeaders,
  ]);
  const columns = headerMap(table);
  return table.rows.flatMap((row, rowIndex) => {
    const internalName = valueAt(row, columns, 'Name').trim();
    const complete = flagAt(row, columns, 'complete');
    const runes = Array.from({ length: 6 }, (_, index) => (
      valueAt(row, columns, `Rune${index + 1}`).trim()
    )).filter(Boolean);
    if (!internalName || !complete || runes.length === 0) return [];

    // D2S stores the runeword identity directly. The first Runes.txt row is
    // ABI id 27 (Runeword1), which is also how @d2runewizard/d2s indexes the
    // governed runeword string table.
    const id = rowIndex + 27;
    const name = constants.runewords[id]?.n;
    if (!name) {
      throw new Error(`Complete BKVince runeword ${internalName} row ${rowIndex + 2} has no codec ABI name at id ${id}.`);
    }
    const mods = Array.from({ length: 7 }, (_, index) => index + 1).flatMap((slot) => {
      const code = valueAt(row, columns, `T1Code${slot}`).trim();
      if (!code) return [];
      return [{
        code,
        parameter: valueAt(row, columns, `T1Param${slot}`).trim() || null,
        minimum: optionalIntegerAt(row, columns, `T1Min${slot}`, `Runes row ${rowIndex + 2}`),
        maximum: optionalIntegerAt(row, columns, `T1Max${slot}`, `Runes row ${rowIndex + 2}`),
      }];
    });
    return [{
      id,
      name,
      internalName,
      runeName: valueAt(row, columns, '*Rune Name').trim() || null,
      row: rowIndex + 2,
      runes,
      allowedTypes: Array.from({ length: 6 }, (_, index) => (
        valueAt(row, columns, `itype${index + 1}`).trim()
      )).filter(Boolean),
      excludedTypes: Array.from({ length: 3 }, (_, index) => (
        valueAt(row, columns, `etype${index + 1}`).trim()
      )).filter(Boolean),
      disallowCraftingInLadder: flagAt(row, columns, 'disallowCraftingInLadder'),
      disallowCraftingInNonLadder: flagAt(row, columns, 'disallowCraftingInNonLadder'),
      firstLadderSeason: integerAt(row, columns, 'firstLadderSeason'),
      lastLadderSeason: integerAt(row, columns, 'lastLadderSeason'),
      patchRelease: valueAt(row, columns, '*Patch Release').trim() || null,
      mods,
    }];
  }).sort((left, right) => left.name.localeCompare(right.name));
}

function buildItemSkillCatalog(tables, constants) {
  const table = requireTable(tables, 'skills', ['skill', '*Id']);
  const columns = headerMap(table);
  return table.rows.flatMap((row, rowIndex) => {
    const internalName = valueAt(row, columns, 'skill').trim();
    const rawId = valueAt(row, columns, '*Id').trim();
    if (!internalName || !rawId) return [];
    const id = strictInteger(rawId, `skills row ${rowIndex + 2} *Id`);
    return [{ id, internalName, name: constants.skills[id]?.s || null }];
  }).sort((left, right) => left.id - right.id);
}

export function buildSkillCatalog(tables, constants) {
  const skillsTable = requireTable(tables, 'skills', [
    'skill', '*Id', 'charclass', 'skilldesc', 'reqlevel', 'maxlvl',
    'reqskill1', 'reqskill2', 'reqskill3',
  ]);
  const descriptionsTable = requireTable(tables, 'SkillDesc', [
    'skilldesc', 'SkillPage', 'SkillRow', 'SkillColumn', 'ListRow', 'IconCel',
  ]);
  const skillColumns = headerMap(skillsTable);
  const descriptionColumns = headerMap(descriptionsTable);
  const descriptions = new Map();

  descriptionsTable.rows.forEach((row, rowIndex) => {
    const key = valueAt(row, descriptionColumns, 'skilldesc').trim();
    if (!key) return;
    if (descriptions.has(key)) {
      throw new Error(`Ambiguous SkillDesc key ${key} at row ${rowIndex + 2}.`);
    }
    descriptions.set(key, {
      page: integerAt(row, descriptionColumns, 'SkillPage'),
      row: integerAt(row, descriptionColumns, 'SkillRow'),
      column: integerAt(row, descriptionColumns, 'SkillColumn'),
      listRow: integerAt(row, descriptionColumns, 'ListRow'),
      iconCell: integerAt(row, descriptionColumns, 'IconCel'),
    });
  });

  const entries = skillsTable.rows.flatMap((row, rowIndex) => {
    const classCode = valueAt(row, skillColumns, 'charclass').trim();
    const internalName = valueAt(row, skillColumns, 'skill').trim();
    const descriptionKey = valueAt(row, skillColumns, 'skilldesc').trim();
    if (!classCode || !internalName || !descriptionKey) return [];
    const id = strictInteger(valueAt(row, skillColumns, '*Id'), `skills row ${rowIndex + 2} *Id`);
    const layout = descriptions.get(descriptionKey);
    if (!layout) throw new Error(`Skill ${internalName} references missing SkillDesc ${descriptionKey}.`);
    if (layout.page < 1 || layout.page > 3 || layout.row < 1 || layout.row > 6 || layout.column < 1 || layout.column > 3) {
      throw new Error(`Skill ${internalName} has an unsupported tree position ${layout.page}/${layout.row}/${layout.column}.`);
    }
    if (layout.iconCell < 0 || layout.iconCell > 58 || layout.iconCell % 2 !== 0) {
      throw new Error(`Skill ${internalName} has an unsupported icon cell ${layout.iconCell}.`);
    }
    const definition = constants.skills[id];
    if (!definition?.s) throw new Error(`Skill ${internalName} has no governed localized name at id ${id}.`);
    const classDefinition = constants.classes.find((entry) => entry?.c === classCode);
    if (!classDefinition) throw new Error(`Skill ${internalName} references unknown class ${classCode}.`);
    return [{
      id,
      internalName,
      name: definition.s,
      classCode,
      className: classDefinition.n,
      page: layout.page,
      row: layout.row,
      column: layout.column,
      listRow: layout.listRow,
      iconCell: layout.iconCell,
      requiredLevel: integerAt(row, skillColumns, 'reqlevel'),
      maxLevel: integerAt(row, skillColumns, 'maxlvl'),
      prerequisiteNames: ['reqskill1', 'reqskill2', 'reqskill3']
        .map((header) => valueAt(row, skillColumns, header).trim())
        .filter(Boolean),
    }];
  });
  const byInternalName = new Map(entries.map((entry) => [entry.internalName, entry]));
  return entries.map(({ prerequisiteNames, ...entry }) => ({
    ...entry,
    prerequisites: prerequisiteNames.map((name) => {
      const prerequisite = byInternalName.get(name);
      if (!prerequisite || prerequisite.classCode !== entry.classCode) {
        throw new Error(`Skill ${entry.internalName} references unknown class prerequisite ${name}.`);
      }
      return prerequisite.id;
    }),
  })).sort((left, right) => left.id - right.id);
}

function buildNamedQualityCatalog(tables, tableName, codeHeader, propertySlots, baseCodes) {
  const propertyHeaders = Array.from({ length: propertySlots }, (_, index) => index + 1)
    .flatMap((slot) => [`prop${slot}`, `par${slot}`, `min${slot}`, `max${slot}`]);
  const setBonusHeaders = tableName === 'SetItems'
    ? [
      'set', 'add func',
      ...Array.from({ length: 5 }, (_, index) => index + 1).flatMap((slot) => (
        ['a', 'b'].flatMap((side) => [
          `aprop${slot}${side}`,
          `apar${slot}${side}`,
          `amin${slot}${side}`,
          `amax${slot}${side}`,
        ])
      )),
    ]
    : [];
  const table = requireTable(tables, tableName, [
    'index', '*ID', codeHeader, 'disabled', 'spawnable', 'lvl', 'lvl req',
    ...propertyHeaders, ...setBonusHeaders,
  ]);
  const columns = headerMap(table);
  const identifiers = new Set();
  return table.rows.flatMap((row, rowIndex) => {
    const rawId = valueAt(row, columns, '*ID').trim();
    if (!rawId) return [];
    const id = strictInteger(rawId, `${tableName} row ${rowIndex + 2} *ID`);
    if (identifiers.has(id)) throw new Error(`Ambiguous ${tableName} *ID ${id}.`);
    identifiers.add(id);
    const baseCode = valueAt(row, columns, codeHeader).trim();
    if (baseCode && !baseCodes.has(baseCode)) {
      throw new Error(`${tableName} *ID ${id} references unknown BKVince item code ${baseCode}.`);
    }
    const readMod = (codeField, parameterField, minimumField, maximumField) => {
      const code = valueAt(row, columns, codeField).trim();
      if (!code) return [];
      return [{
        code,
        parameter: valueAt(row, columns, parameterField).trim() || null,
        minimum: optionalIntegerAt(row, columns, minimumField, `${tableName} row ${rowIndex + 2}`),
        maximum: optionalIntegerAt(row, columns, maximumField, `${tableName} row ${rowIndex + 2}`),
      }];
    };
    const mods = Array.from({ length: propertySlots }, (_, index) => index + 1).flatMap((slot) => (
      readMod(`prop${slot}`, `par${slot}`, `min${slot}`, `max${slot}`)
    ));
    const addFunction = tableName === 'SetItems'
      ? (optionalIntegerAt(row, columns, 'add func', `${tableName} row ${rowIndex + 2}`) ?? 0)
      : null;
    if (tableName === 'SetItems' && ![0, 1, 2].includes(addFunction)) {
      throw new Error(`${tableName} row ${rowIndex + 2} uses unsupported add func ${addFunction}.`);
    }
    const setBonusLists = tableName === 'SetItems'
      ? Array.from({ length: 5 }, (_, index) => index + 1).map((slot) => ({
        bit: slot - 1,
        mods: ['a', 'b'].flatMap((side) => readMod(
          `aprop${slot}${side}`,
          `apar${slot}${side}`,
          `amin${slot}${side}`,
          `amax${slot}${side}`,
        )),
      }))
      : [];
    if (addFunction === 0) {
      mods.push(...setBonusLists.flatMap(({ mods: bonusMods }) => bonusMods));
    }
    return [{
      id,
      name: valueAt(row, columns, 'index'),
      baseCode: baseCode || null,
      row: rowIndex + 2,
      disabled: flagAt(row, columns, 'disabled'),
      spawnable: flagAt(row, columns, 'spawnable'),
      level: integerAt(row, columns, 'lvl'),
      levelRequirement: integerAt(row, columns, 'lvl req'),
      ...(tableName === 'SetItems' ? {
        setName: valueAt(row, columns, 'set'),
        addFunction,
        setBonusLists: addFunction === 0 ? [] : setBonusLists,
      } : {}),
      mods,
    }];
  });
}

function buildAffixRows(tables, tableName) {
  const table = requireTable(tables, tableName, [
    'Name', 'spawnable', 'rare', 'level', 'levelreq', 'group', 'classspecific', 'class',
    'mod1code', 'mod1param', 'mod1min', 'mod1max',
    'mod2code', 'mod2param', 'mod2min', 'mod2max',
    'mod3code', 'mod3param', 'mod3min', 'mod3max',
    'itype1', 'itype2', 'itype3', 'itype4', 'itype5', 'itype6', 'itype7',
    'etype1', 'etype2', 'etype3', 'etype4', 'etype5',
  ]);
  const columns = headerMap(table);
  let abiId = 1;
  return table.rows.flatMap((row, rowIndex) => {
    const name = valueAt(row, columns, 'Name');
    if (name === 'Expansion') return [];
    const id = abiId;
    abiId += 1;
    if (!name) return [];
    const mods = [1, 2, 3].flatMap((slot) => {
      const code = valueAt(row, columns, `mod${slot}code`).trim();
      if (!code) return [];
      return [{
        code,
        parameter: valueAt(row, columns, `mod${slot}param`).trim() || null,
        minimum: optionalIntegerAt(row, columns, `mod${slot}min`, `${tableName} row ${rowIndex + 2}`),
        maximum: optionalIntegerAt(row, columns, `mod${slot}max`, `${tableName} row ${rowIndex + 2}`),
      }];
    });
    return [{
      id,
      name,
      row: rowIndex + 2,
      spawnable: flagAt(row, columns, 'spawnable'),
      rare: flagAt(row, columns, 'rare'),
      level: integerAt(row, columns, 'level'),
      levelRequirement: integerAt(row, columns, 'levelreq'),
      group: integerAt(row, columns, 'group'),
      classSpecific: flagAt(row, columns, 'classspecific'),
      classCode: valueAt(row, columns, 'class').trim() || null,
      allowedTypes: [1, 2, 3, 4, 5, 6, 7]
        .map((slot) => valueAt(row, columns, `itype${slot}`).trim())
        .filter(Boolean),
      excludedTypes: [1, 2, 3, 4, 5]
        .map((slot) => valueAt(row, columns, `etype${slot}`).trim())
        .filter(Boolean),
      mods,
    }];
  });
}

function buildRareNameRows(tables, constants, tableName, firstId) {
  const table = requireTable(tables, tableName, [
    'name',
    'itype1', 'itype2', 'itype3', 'itype4', 'itype5', 'itype6', 'itype7',
    'etype1', 'etype2', 'etype3', 'etype4',
  ]);
  const columns = headerMap(table);
  let id = firstId;
  const entries = table.rows.flatMap((row, rowIndex) => {
    const sourceName = valueAt(row, columns, 'name').trim();
    if (!sourceName) return [];
    const currentId = id;
    id += 1;
    const localizedName = constants.rare_names[currentId]?.n;
    return [{
      id: currentId,
      name: localizedName || sourceName,
      sourceName,
      row: rowIndex + 2,
      allowedTypes: [1, 2, 3, 4, 5, 6, 7]
        .map((slot) => valueAt(row, columns, `itype${slot}`).trim())
        .filter(Boolean),
      excludedTypes: [1, 2, 3, 4]
        .map((slot) => valueAt(row, columns, `etype${slot}`).trim())
        .filter(Boolean),
    }];
  });
  return { entries, nextId: id };
}

function buildLowQualityRows(tables) {
  const table = requireTable(tables, 'LowQualityItems', ['Name']);
  return table.rows.flatMap((row, rowIndex) => {
    const name = row[table.headers.indexOf('Name')]?.trim();
    return name ? [{ id: rowIndex, name, row: rowIndex + 2 }] : [];
  });
}

function buildPropertyRows(tables, constants) {
  const functionHeaders = Array.from({ length: 7 }, (_, index) => index + 1)
    .flatMap((slot) => [`func${slot}`, `stat${slot}`, `set${slot}`, `val${slot}`]);
  const table = requireTable(tables, 'Properties', [
    'code', '*Id', ...functionHeaders,
    'uiRangeType', '*Tooltip', '*Parameter', '*Min', '*Max', '*Notes',
  ]);
  const columns = headerMap(table);
  const keys = new Set();
  const statIds = new Map(constants.magical_properties.flatMap((entry, id) => (
    entry?.s ? [[entry.s, id]] : []
  )));
  const statGroups = buildMagicStatGroups(constants);
  return table.rows.flatMap((row, rowIndex) => {
    const key = valueAt(row, columns, 'code').trim();
    if (!key) return [];
    if (keys.has(key)) throw new Error(`Ambiguous Properties code ${key}.`);
    keys.add(key);
    const functions = Array.from({ length: 7 }, (_, index) => index + 1).flatMap((slot) => {
      const rawFunctionId = valueAt(row, columns, `func${slot}`).trim();
      if (!rawFunctionId) return [];
      const functionId = strictInteger(rawFunctionId, `Properties row ${rowIndex + 2} func${slot}`);
      return [compilePropertyFunction({
        functionId,
        statName: valueAt(row, columns, `stat${slot}`).trim() || null,
        set: flagAt(row, columns, `set${slot}`),
        value: optionalIntegerAt(row, columns, `val${slot}`, `Properties row ${rowIndex + 2}`),
        statIds,
        statGroups,
        constants,
      })];
    });
    const unsupported = functions.filter(({ supported }) => !supported);
    return [{
      key,
      row: rowIndex + 2,
      functions,
      supported: functions.length > 0 && unsupported.length === 0,
      unsupportedReason: functions.length === 0
        ? 'The property has no item-mod function.'
        : unsupported.map(({ reason }) => reason).join(' ') || null,
      uiRangeType: integerAt(row, columns, 'uiRangeType'),
      tooltip: valueAt(row, columns, '*Tooltip').trim() || null,
      parameterHint: valueAt(row, columns, '*Parameter').trim() || null,
      minimumHint: valueAt(row, columns, '*Min').trim() || null,
      maximumHint: valueAt(row, columns, '*Max').trim() || null,
      notes: valueAt(row, columns, '*Notes').trim() || null,
    }];
  });
}

function compilePropertyFunction({
  functionId,
  statName,
  set,
  value,
  statIds,
  statGroups,
  constants,
}) {
  const elementalSkillStats = [
    'item_elemskill_fire',
    'item_elemskill_lightning',
    'item_elemskill_magic',
    'item_elemskill_cold',
    'item_elemskill_poison',
  ];
  if (functionId === 3 && elementalSkillStats.includes(statName)) {
    return parameterizedPropertyFunction({
      functionId,
      statName,
      set,
      value,
      encoding: 'zero-parameter',
      expectedValueCount: 2,
      allowedStats: elementalSkillStats,
      statIds,
      constants,
    });
  }
  const regularSources = new Map([
    [1, 'roll'],
    // Func 2 receives the already-rolled property value. Native Exile saves
    // prove item_armor_percent may remain anywhere inside T1Min/T1Max.
    [2, 'roll'],
    [3, 'roll'],
    [4, 'maximum'],
    [8, 'roll'],
    [15, 'minimum'],
    [16, 'maximum'],
    [17, 'parameter-or-roll'],
  ]);
  if (regularSources.has(functionId)) {
    return propertyStatFunction({
      functionId,
      statName,
      valueSource: regularSources.get(functionId),
      set,
      value,
      statIds,
      statGroups,
      constants,
    });
  }
  if (functionId === 5) {
    return propertyStatFunction({
      functionId,
      statName: 'mindamage',
      valueSource: 'roll',
      set,
      value,
      statIds,
      statGroups,
      constants,
    });
  }
  if (functionId === 6) {
    return propertyStatFunction({
      functionId,
      statName: 'maxdamage',
      valueSource: 'roll',
      set,
      value,
      statIds,
      statGroups,
      constants,
    });
  }
  if (functionId === 7) {
    const maximumDamage = propertyStatOutput(
      'item_maxdamage_percent',
      'roll',
      set,
      statIds,
      statGroups,
      constants,
    );
    const minimumDamage = propertyStatOutput(
      'item_mindamage_percent',
      'roll',
      set,
      statIds,
      statGroups,
      constants,
    );
    const outputs = [maximumDamage, minimumDamage];
    const reason = outputs.find(({ reason: outputReason }) => outputReason)?.reason || null;
    return { functionId, statName: null, set, value, outputs, supported: !reason, reason };
  }
  if (functionId === 10) {
    return parameterizedPropertyFunction({
      functionId,
      statName,
      set,
      value,
      encoding: 'skill-tab',
      expectedValueCount: 3,
      allowedStats: ['item_addskill_tab'],
      statIds,
      constants,
    });
  }
  if (functionId === 11) {
    if (statName === 'item_splashonhit') {
      return parameterizedPropertyFunction({
        functionId,
        statName,
        set,
        value,
        encoding: 'skill-event-chance',
        expectedValueCount: 2,
        allowedStats: ['item_splashonhit'],
        statIds,
        constants,
      });
    }
    return parameterizedPropertyFunction({
      functionId,
      statName,
      set,
      value,
      encoding: 'skill-event',
      expectedValueCount: 3,
      allowedStats: [
        'item_skillonattack',
        'item_skillonkill',
        'item_skillonhit',
        'item_skillongethit',
        'item_skillondeath',
        'item_skillonlevelup',
      ],
      statIds,
      constants,
    });
  }
  if (functionId === 21) {
    return parameterizedPropertyFunction({
      functionId,
      statName,
      set,
      value,
      encoding: 'property-value-parameter',
      expectedValueCount: 2,
      allowedStats: ['item_addclassskills', 'item_elemskill'],
      statIds,
      constants,
    });
  }
  if (functionId === 22) {
    return parameterizedPropertyFunction({
      functionId,
      statName,
      set,
      value,
      encoding: 'stat-parameter',
      expectedValueCount: 2,
      allowedStats: ['item_nonclassskill', 'item_singleskill', 'item_aura'],
      statIds,
      constants,
    });
  }
  if (functionId === 12) {
    return parameterizedPropertyFunction({
      functionId,
      statName,
      set,
      value,
      encoding: 'random-skill',
      expectedValueCount: 2,
      allowedStats: ['item_singleskill'],
      statIds,
      constants,
    });
  }
  if (functionId === 20) {
    const output = propertyStatOutput(
      'item_indesctructible',
      'one',
      set,
      statIds,
      statGroups,
      constants,
    );
    return {
      functionId,
      statName: null,
      set,
      value,
      outputs: [output],
      supported: !output.reason,
      reason: output.reason || null,
    };
  }
  if (functionId === 14) {
    return {
      functionId,
      statName,
      set,
      value,
      outputs: [],
      structure: { encoding: 'sockets' },
      supported: true,
      reason: null,
    };
  }
  if (functionId === 23) {
    return {
      functionId,
      statName,
      set,
      value,
      outputs: [],
      structure: { encoding: 'ethereal' },
      supported: true,
      reason: null,
    };
  }
  if (functionId === 24) {
    return parameterizedPropertyFunction({
      functionId,
      statName,
      set,
      value,
      encoding: 'numeric-stat-parameter',
      expectedValueCount: 2,
      allowedStats: ['item_reanimate'],
      statIds,
      constants,
    });
  }
  if (functionId === 36) {
    return parameterizedPropertyFunction({
      functionId,
      statName,
      set,
      value,
      encoding: 'random-class-skill',
      expectedValueCount: 2,
      allowedStats: ['item_addclassskills'],
      statIds,
      constants,
    });
  }
  if (functionId === 19) {
    return parameterizedPropertyFunction({
      functionId,
      statName,
      set,
      value,
      encoding: 'charged-skill',
      expectedValueCount: 4,
      allowedStats: ['item_charged_skill'],
      statIds,
      constants,
    });
  }
  const names = {
    9: 'single-skill',
    13: 'durability structure',
    14: 'socket structure',
    18: 'time-based value',
    19: 'charged skill',
    23: 'ethereal structure',
    24: 'parameterized shared value',
    25: 'random property stat',
    36: 'swapped property parameter',
  };
  return {
    functionId,
    statName,
    set,
    value,
    outputs: [],
    supported: false,
    reason: `Property function ${functionId} (${names[functionId] || 'unknown encoding'}) remains locked.`,
  };
}

function parameterizedPropertyFunction({
  functionId,
  statName,
  set,
  value,
  encoding,
  expectedValueCount,
  allowedStats,
  statIds,
  constants,
}) {
  if (!allowedStats.includes(statName)) {
    return {
      functionId,
      statName,
      set,
      value,
      outputs: [],
      supported: false,
      reason: `Property function ${functionId} has no governed ${statName || '(blank)'} oracle.`,
    };
  }
  const statId = statIds.get(statName);
  if (!Number.isInteger(statId)) {
    return {
      functionId,
      statName,
      set,
      value,
      outputs: [],
      supported: false,
      reason: `Unknown ItemStatCost stat ${statName || '(blank)'}.`,
    };
  }
  const valueCount = magicStatValueCount(constants, statId);
  if (valueCount !== expectedValueCount) {
    return {
      functionId,
      statName,
      set,
      value,
      outputs: [],
      supported: false,
      reason: `Stat ${statName} uses ${valueCount} values; ${encoding} requires ${expectedValueCount}.`,
    };
  }
  return {
    functionId,
    statName,
    set,
    value,
    outputs: [{
      statId,
      statName,
      groupId: statId,
      valueCount,
      encoding,
      set,
    }],
    supported: true,
    reason: null,
  };
}

function magicStatValueCount(constants, id) {
  const definition = constants.magical_properties[id];
  const count = Number.isInteger(definition?.np) && definition.np > 0 ? definition.np : 1;
  let valueCount = 0;
  for (let offset = 0; offset < count; offset += 1) {
    const entry = constants.magical_properties[id + offset];
    if (!entry) throw new Error(`Magic stat ${id} references a missing grouped stat.`);
    if (entry.sP) {
      if (entry.dF === 14) valueCount += 1;
      if ([2, 3].includes(Number(entry.e))) valueCount += 1;
      valueCount += 1;
    }
    valueCount += Number(entry.e) === 3 ? 2 : 1;
  }
  return valueCount;
}

function propertyStatFunction({
  functionId,
  statName,
  valueSource,
  set,
  value,
  statIds,
  statGroups,
  constants,
}) {
  const output = propertyStatOutput(
    statName,
    valueSource,
    set,
    statIds,
    statGroups,
    constants,
  );
  return {
    functionId,
    statName,
    set,
    value,
    outputs: [output],
    supported: !output.reason,
    reason: output.reason || null,
  };
}

function propertyStatOutput(statName, valueSource, set, statIds, statGroups, constants) {
  const statId = statIds.get(statName);
  if (!Number.isInteger(statId)) {
    return { statName, valueSource, set, reason: `Unknown ItemStatCost stat ${statName || '(blank)'}.` };
  }
  const group = statGroups.get(statId);
  if (!group) {
    return { statId, statName, valueSource, set, reason: `Stat ${statName} has no codec group.` };
  }
  if (!group.safe) {
    return {
      statId,
      statName,
      valueSource,
      set,
      reason: `Stat ${statName} uses a parameterized or unsupported v105 encoding.`,
    };
  }
  return {
    statId,
    statName,
    groupId: group.id,
    valueIndex: statId - group.id,
    valueCount: group.count,
    valueSource,
    set,
  };
}

function buildMagicStatGroups(constants) {
  const groups = new Map();
  const covered = new Set();
  constants.magical_properties.forEach((definition, id) => {
    if (!definition || covered.has(id)) return;
    const count = Number.isInteger(definition.np) && definition.np > 0 ? definition.np : 1;
    const entries = Array.from({ length: count }, (_, offset) => constants.magical_properties[id + offset]);
    const safe = entries.every((entry) => (
      entry
      && Number.isInteger(entry.sB)
      && entry.sB > 0
      && entry.sB <= 32
      && !Number.isInteger(entry.sP)
      && ![2, 3].includes(Number(entry.e))
    ));
    entries.forEach((_, offset) => {
      covered.add(id + offset);
      groups.set(id + offset, { id, count, safe });
    });
  });
  return groups;
}

function buildItemTypeRows(tables) {
  const table = requireTable(
    tables,
    'ItemTypes',
    ['ItemType', 'Code', 'Equiv1', 'Equiv2', 'Body', 'BodyLoc1', 'BodyLoc2'],
  );
  const columns = headerMap(table);
  const codes = new Set();
  return table.rows.flatMap((row, rowIndex) => {
    const code = valueAt(row, columns, 'Code').trim();
    if (!code) return [];
    if (codes.has(code)) throw new Error(`Ambiguous ItemTypes Code ${code}.`);
    codes.add(code);
    return [{
      key: code,
      code,
      name: valueAt(row, columns, 'ItemType'),
      row: rowIndex + 2,
      equivalents: ['Equiv1', 'Equiv2']
        .map((header) => valueAt(row, columns, header).trim())
        .filter(Boolean),
      body: flagAt(row, columns, 'Body'),
      bodyLocations: ['BodyLoc1', 'BodyLoc2']
        .map((header) => valueAt(row, columns, header).trim())
        .filter((entry, index, entries) => entry && entries.indexOf(entry) === index),
    }];
  });
}

function attachAffixAbi(entries, codecEntries, tableName) {
  return entries.map((entry) => {
    const codecEntry = codecEntries[entry.id];
    if (!codecEntry) {
      throw new Error(
        `${tableName} row ${entry.row} has no codec entry at ABI ID ${entry.id}.`,
      );
    }
    return { ...entry, sourceName: entry.name, name: codecEntry.n || entry.name };
  });
}

function itemTypeClosure(code, itemTypeMap, visiting = new Set()) {
  const normalized = String(code).trim();
  if (!normalized) return [];
  if (visiting.has(normalized)) {
    throw new Error(`Circular BKVince ItemTypes equivalence at ${normalized}.`);
  }
  const definition = itemTypeMap.get(normalized);
  if (!definition) throw new Error(`Unknown BKVince ItemTypes code ${normalized}.`);
  const nextVisiting = new Set(visiting).add(normalized);
  const closure = new Set([normalized]);
  definition.equivalents.forEach((equivalent) => {
    itemTypeClosure(equivalent, itemTypeMap, nextVisiting).forEach((entry) => closure.add(entry));
  });
  return [...closure];
}

function preferredCatalogItemName(localizedName, sourceName, code) {
  const localized = String(localizedName || '').trim();
  const visible = localized
    .replace(/\u00ff[cC]./g, '')
    .replace(/[\uE000-\uF8FF]/g, '')
    .replace(/\s+/g, ' ')
    .trim();
  if (visible.length > 2 && !/^\d+$/.test(visible)) return localized;
  return String(sourceName || code).trim() || code;
}

function buildKeyedRows(tables, tableName, keyHeader, headers) {
  const table = requireTable(tables, tableName, headers);
  const columns = headerMap(table);
  const keys = new Set();
  return table.rows.flatMap((row, rowIndex) => {
    const key = valueAt(row, columns, keyHeader).trim();
    if (!key) return [];
    if (keys.has(key)) throw new Error(`Ambiguous ${tableName} ${keyHeader} ${key}.`);
    keys.add(key);
    return [{ key, row: rowIndex + 2 }];
  });
}

function requireTable(tables, tableName, headers) {
  const table = tables?.[tableName];
  if (!table || !Array.isArray(table.headers) || !Array.isArray(table.rows)) {
    throw new Error(`Missing governed BKVince table ${tableName}.txt.`);
  }
  const missing = headers.filter((header) => !table.headers.includes(header));
  if (missing.length > 0) {
    throw new Error(`${tableName}.txt is missing required headers: ${missing.join(', ')}.`);
  }
  return table;
}

function headerMap(table) {
  return Object.fromEntries(table.headers.map((header, index) => [header, index]));
}

function valueAt(row, columns, header) {
  const index = columns[header];
  return Number.isInteger(index) ? String(row[index] ?? '') : '';
}

function flagAt(row, columns, header) {
  return valueAt(row, columns, header).trim() === '1';
}

function integerAt(row, columns, header) {
  const value = valueAt(row, columns, header).trim();
  return value === '' ? null : strictInteger(value, header);
}

function optionalIntegerAt(row, columns, header, context) {
  const value = valueAt(row, columns, header).trim();
  return value === '' ? null : strictInteger(value, `${context} ${header}`);
}

function positiveIntegerAt(row, columns, header) {
  const value = integerAt(row, columns, header);
  return Number.isInteger(value) && value > 0 ? value : 1;
}

function strictInteger(value, label) {
  if (!/^-?\d+$/.test(String(value))) throw new Error(`${label} must be an integer; received ${value}.`);
  return Number.parseInt(value, 10);
}

function itemDefinition(constants, code) {
  return constants.armor_items[code]
    || constants.weapon_items[code]
    || constants.other_items[code]
    || null;
}

function main() {
  const built = buildConstants();
  const { constants } = built;
  const itemCatalog = buildItemCatalog(built.tables, constants);
  const skillCatalog = buildSkillCatalog(built.tables, constants);
  const mercenaryCatalog = buildMercenaryCatalog(built.tables);
  const demonCatalog = buildDemonCatalog(built.tables, built.buffers);
  const generatedSource = {
    tables: tableNames,
    inheritedTables: inheritedTableNames,
    strings: stringNames,
    classCount: constants.classes.filter(Boolean).length,
    itemCatalog: {
      bases: itemCatalog.bases.length,
      uniqueItems: itemCatalog.uniqueItems.length,
      setItems: itemCatalog.setItems.length,
      prefixes: itemCatalog.prefixes.length,
      suffixes: itemCatalog.suffixes.length,
      rareNamePrefixes: itemCatalog.rareNamePrefixes.length,
      rareNameSuffixes: itemCatalog.rareNameSuffixes.length,
      lowQualityNames: itemCatalog.lowQualityNames.length,
      properties: itemCatalog.properties.length,
      runewords: itemCatalog.runewords.length,
      itemSkills: itemCatalog.itemSkills.length,
      itemTypes: itemCatalog.itemTypes.length,
    },
    skillCatalog: skillCatalog.length,
    mercenaryCatalog: mercenaryCatalog.length,
    demonCatalog: {
      monsters: demonCatalog.monsters.length,
      superUniques: demonCatalog.superUniques.length,
      modifiers: demonCatalog.modifiers.length,
    },
    itemAbiSha256: hashGeneratedInputs(built.buffers),
  };
  const generated = [
    '// Generated from governed BKVince and inherited vanilla 3.3 tables by scripts/generate-constants.mjs.',
    '// Do not edit this file directly.',
    `export const generatedSource = ${JSON.stringify(generatedSource, null, 2)};`,
    `export const itemCatalog = ${JSON.stringify(itemCatalog)};`,
    `export const skillCatalog = ${JSON.stringify(skillCatalog)};`,
    `export const mercenaryCatalog = ${JSON.stringify(mercenaryCatalog)};`,
    `export const demonCatalog = ${JSON.stringify(demonCatalog)};`,
    `const constants = ${JSON.stringify(constants)};`,
    'export default constants;',
    '',
  ].join('\n');

  if (process.argv.includes('--check')) {
    if (!fs.existsSync(outputPath) || fs.readFileSync(outputPath, 'utf8') !== generated) {
      throw new Error('Generated BKVince constants are stale. Run npm run generate -w apps/hero-editor.');
    }
    console.log(`BKVince constants are current (${generatedSourceSummary(constants)}, ${itemCatalog.bases.length} item bases).`);
  } else {
    fs.writeFileSync(outputPath, generated, 'utf8');
    console.log(`Generated ${path.relative(repositoryRoot, outputPath)} (${generatedSourceSummary(constants)}, ${itemCatalog.bases.length} item bases).`);
  }
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  main();
}

function generatedSourceSummary(value) {
  return `${value.classes.filter(Boolean).length} classes, ${value.magical_properties.filter(Boolean).length} stats`;
}

function hashGeneratedInputs(buffers) {
  const hash = createHash('sha256');
  Object.entries(buffers).sort(([left], [right]) => left.localeCompare(right)).forEach(([name, value]) => {
    hash.update(name);
    hash.update('\0');
    hash.update(String(value));
    hash.update('\0');
  });
  return hash.digest('hex');
}
