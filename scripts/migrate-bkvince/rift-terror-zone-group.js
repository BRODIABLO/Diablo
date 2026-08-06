'use strict';

// Adds the missing BKVince level group used by Rift levels 138-146 and
// validates the Rift/Cow terror-zone naming chains without rewriting any
// existing governed table.

const fs = require('fs');
const path = require('path');
const {
  parseTable,
  serializeTable,
  writeTable,
  ENCODING,
} = require('../build-data/tsv');

const ROOT = path.resolve(__dirname, '..', '..');
const TARGET_EXCEL = path.join(
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
const STRINGS_ROOT = path.join(
  ROOT,
  'data-BKVince',
  'BKVince.mpq',
  'data',
  'local',
  'lng',
);
const DESECRATED_PATH = path.join(
  ROOT,
  'data-BKVince',
  'BKVince.mpq',
  'data',
  'hd',
  'global',
  'excel',
  'desecratedzones.json',
);

const FILES = {
  vanillaGroups: path.join(VANILLA_EXCEL, 'levelgroups.txt'),
  targetGroups: path.join(TARGET_EXCEL, 'levelgroups.txt'),
  targetLevels: path.join(TARGET_EXCEL, 'levels.txt'),
  levelStrings: path.join(STRINGS_ROOT, 'strings', 'levels.json'),
};

const RIFT_GROUP = {
  LevelGroupId: 'Act 5 - Rift',
  ParentLevelGroupId: '',
  CompleteThreshold: '100',
  Effect: '',
  '*Name': 'Rifts',
  NameString: 'RiftGroup',
};
const RIFT_LEVEL_IDS = Array.from({ length: 9 }, (_, index) => 138 + index);
const REGULAR_RIFT_ZONE_ID = 'Act5-Rifts';
const MANUAL_RIFT_ZONE_ID = 'Act5-Rifts-Manual';
const RIFT_STRING_ID = 73040;
const COW_GROUP_STRING_ID = 27338;
const COW_GROUP_STRING_KEY = 'MooMooFarmGroup';
const COW_LEVEL_STRING_KEY = 'Moo Moo Farm';

function assert(condition, message) {
  if (!condition) throw new Error(message);
}

function loadTable(filePath, label) {
  const raw = fs.readFileSync(filePath, ENCODING);
  const table = parseTable(filePath);
  assert(serializeTable(table) === raw, `${label}: round-trip non byte-exact`);
  assert(table.eol === '\r\n', `${label}: fins de ligne autres que CRLF`);
  return { raw, table };
}

function cloneTable(table) {
  return {
    headers: [...table.headers],
    rows: table.rows.map((row) => [...row]),
    eol: table.eol,
    hasFinalEol: table.hasFinalEol,
  };
}

function indexes(headers) {
  return new Map(headers.map((header, index) => [header, index]));
}

function rowObject(table, row) {
  return Object.fromEntries(table.headers.map((header, index) => [header, row[index] ?? '']));
}

function buildExpectedGroups(vanillaGroups) {
  const expectedHeaders = [
    'LevelGroupId',
    'ParentLevelGroupId',
    'CompleteThreshold',
    'Effect',
    '*Name',
    'NameString',
  ];
  assert(
    JSON.stringify(vanillaGroups.headers) === JSON.stringify(expectedHeaders),
    `levelgroups vanilla 3.2: headers inattendus: ${vanillaGroups.headers.join(', ')}`,
  );
  const headerIndexes = indexes(vanillaGroups.headers);
  const existing = vanillaGroups.rows.filter(
    (row) => (row[headerIndexes.get('LevelGroupId')] ?? '') === RIFT_GROUP.LevelGroupId,
  );
  assert(existing.length === 0, 'levelgroups vanilla 3.2 contient deja Act 5 - Rift');

  const expected = cloneTable(vanillaGroups);
  expected.rows.push(expected.headers.map((header) => RIFT_GROUP[header] ?? ''));
  return expected;
}

function parseJson(filePath, label) {
  const raw = fs.readFileSync(filePath, 'utf8').replace(/^\uFEFF/, '');
  try {
    return JSON.parse(raw);
  } catch (error) {
    throw new Error(`${label}: JSON invalide: ${error.message}`);
  }
}

function validateLocalization() {
  const levels = parseJson(FILES.levelStrings, 'levels.json');
  const keyMatches = levels.filter((entry) => entry.Key === RIFT_GROUP.NameString);
  assert(keyMatches.length === 1, `levels.json: RiftGroup attendu une fois, trouve ${keyMatches.length}`);
  assert(keyMatches[0].id === RIFT_STRING_ID, `levels.json: RiftGroup doit utiliser l'ID ${RIFT_STRING_ID}`);
  assert(keyMatches[0].enUS === 'The Rifts', 'levels.json: texte enUS RiftGroup inattendu');

  const idMatches = [];
  for (const directoryName of ['strings', 'strings-legacy']) {
    const directory = path.join(STRINGS_ROOT, directoryName);
    for (const fileName of fs.readdirSync(directory).filter((name) => name.endsWith('.json'))) {
      const entries = parseJson(path.join(directory, fileName), `${directoryName}/${fileName}`);
      for (const entry of entries) {
        if (entry.id === RIFT_STRING_ID) {
          idMatches.push(`${directoryName}/${fileName}:${entry.Key}`);
        }
      }
    }
  }
  assert(
    idMatches.length === 1 && idMatches[0] === 'strings/levels.json:RiftGroup',
    `ID de localisation ${RIFT_STRING_ID} non unique: ${idMatches.join(', ')}`,
  );

  const cowGroupMatches = levels.filter((entry) => entry.Key === COW_GROUP_STRING_KEY);
  const cowLevelMatches = levels.filter((entry) => entry.Key === COW_LEVEL_STRING_KEY);
  assert(cowGroupMatches.length === 1, `levels.json: ${COW_GROUP_STRING_KEY} attendu une fois`);
  assert(cowLevelMatches.length === 1, `levels.json: ${COW_LEVEL_STRING_KEY} attendu une fois`);
  const cowGroup = cowGroupMatches[0];
  const cowLevel = cowLevelMatches[0];
  assert(cowGroup.id === COW_GROUP_STRING_ID, `levels.json: ${COW_GROUP_STRING_KEY} doit utiliser l'ID ${COW_GROUP_STRING_ID}`);
  for (const locale of Object.keys(cowLevel).filter((key) => !['id', 'Key'].includes(key))) {
    assert(
      cowGroup[locale] === cowLevel[locale],
      `levels.json: ${COW_GROUP_STRING_KEY}.${locale} doit reprendre ${COW_LEVEL_STRING_KEY}.${locale}`,
    );
  }
  assert(
    !Object.values(cowGroup).some((value) => typeof value === 'string' && value.includes('ÿc')),
    `levels.json: ${COW_GROUP_STRING_KEY} contient encore un code de couleur de la chaine factice`,
  );

  return {
    cowLocalizationId: COW_GROUP_STRING_ID,
    cowName: cowGroup.enUS,
  };
}

function validateReferences(groupsTable) {
  const levelsLoaded = loadTable(FILES.targetLevels, 'levels.txt BKVince');
  const levelIndexes = indexes(levelsLoaded.table.headers);
  const levelsById = new Map(
    levelsLoaded.table.rows.map((row) => [Number(row[levelIndexes.get('Id')]), rowObject(levelsLoaded.table, row)]),
  );
  const groupIndexes = indexes(groupsTable.headers);
  const groupIds = new Set(
    groupsTable.rows.map((row) => row[groupIndexes.get('LevelGroupId')] ?? '').filter(Boolean),
  );

  for (const levelId of RIFT_LEVEL_IDS) {
    const level = levelsById.get(levelId);
    assert(level, `levels.txt: Rift level ID ${levelId} absent`);
    assert(
      level.LevelGroup === RIFT_GROUP.LevelGroupId,
      `levels.txt: ID ${levelId}.LevelGroup=${JSON.stringify(level.LevelGroup)}`,
    );
  }

  const desecrated = parseJson(DESECRATED_PATH, 'desecratedzones.json');
  const config = desecrated.desecrated_zones?.[0];
  assert(config, 'desecratedzones.json: configuration principale absente');
  const regularZones = config.zones ?? [];
  const manualZones = (config.manual_zones ?? []).flatMap((group) => group.zones ?? []);
  const allZones = [...regularZones, ...manualZones];
  const references = allZones.flatMap((zone) => (zone.levels ?? []).map((level) => ({ zone: zone.id, ...level })));

  const zoneIdCounts = new Map();
  for (const zone of allZones) {
    zoneIdCounts.set(zone.id, (zoneIdCounts.get(zone.id) ?? 0) + 1);
  }
  const duplicateZoneIds = [...zoneIdCounts].filter(([, count]) => count > 1).map(([id]) => id);
  assert(
    duplicateZoneIds.length === 0,
    `desecratedzones.json: identifiants de zone dupliques: ${duplicateZoneIds.join(', ')}`,
  );

  const regularRiftZones = regularZones.filter((zone) => zone.id === REGULAR_RIFT_ZONE_ID);
  const manualRiftZones = manualZones.filter((zone) => zone.id === MANUAL_RIFT_ZONE_ID);
  assert(
    regularRiftZones.length === 1,
    `desecratedzones.json: ${REGULAR_RIFT_ZONE_ID} regulier attendu une fois, trouve ${regularRiftZones.length}`,
  );
  assert(
    manualRiftZones.length === 1,
    `desecratedzones.json: ${MANUAL_RIFT_ZONE_ID} manuel attendu une fois, trouve ${manualRiftZones.length}`,
  );
  const riftZones = [...regularRiftZones, ...manualRiftZones];
  for (const zone of riftZones) {
    const ids = zone.levels.map((level) => Number(level.level_id));
    assert(
      JSON.stringify(ids) === JSON.stringify(RIFT_LEVEL_IDS),
      `desecratedzones.json: ${zone.name} couvre ${ids.join(', ')}`,
    );
  }

  for (const reference of references) {
    const levelId = Number(reference.level_id);
    assert(
      Number.isInteger(levelId) && levelId > 0,
      `desecratedzones.json: level_id invalide ${reference.level_id}`,
    );
    const level = levelsById.get(levelId);
    assert(level, `desecratedzones.json: level_id ${reference.level_id} absent de levels.txt`);
    if (level.LevelGroup) {
      assert(
        groupIds.has(level.LevelGroup),
        `desecratedzones.json: level_id ${reference.level_id} utilise le groupe absent ${level.LevelGroup}`,
      );
    }
    if (reference.waypoint_level_id !== undefined) {
      const waypointLevelId = Number(reference.waypoint_level_id);
      assert(
        Number.isInteger(waypointLevelId) && waypointLevelId > 0 && levelsById.has(waypointLevelId),
        `desecratedzones.json: waypoint_level_id ${reference.waypoint_level_id} absent de levels.txt`,
      );
    }
  }

  const harrogath = levelsById.get(109);
  const riftOne = levelsById.get(138);
  assert(
    harrogath?.Vis0 === '138' && harrogath?.Warp0 === '83',
    'levels.txt: Harrogath doit relier Rift 1 par Vis0=138 et Warp0=83',
  );
  assert(
    riftOne?.Vis0 === '109' && riftOne?.Warp0 === '81',
    'levels.txt: Rift 1 doit relier Harrogath par Vis0=109 et Warp0=81',
  );

  return {
    referenceCount: references.length,
    uniqueZoneIdCount: zoneIdCounts.size,
    riftZoneCount: riftZones.length,
    riftZoneIds: riftZones.map((zone) => zone.id),
    harrogathRiftLink: { levelId: 109, Vis0: 138, Warp0: 83 },
  };
}

function main() {
  const checkOnly = process.argv.includes('--check');
  const vanillaLoaded = loadTable(FILES.vanillaGroups, 'levelgroups vanilla 3.2');
  const expected = buildExpectedGroups(vanillaLoaded.table);
  const expectedRaw = serializeTable(expected);

  let changed = true;
  if (fs.existsSync(FILES.targetGroups)) {
    const targetLoaded = loadTable(FILES.targetGroups, 'levelgroups BKVince');
    changed = targetLoaded.raw !== expectedRaw;
  }

  if (checkOnly) {
    assert(fs.existsSync(FILES.targetGroups), 'levelgroups BKVince absent; lancer le generateur');
    assert(!changed, 'levelgroups BKVince est perime ou contient des changements inattendus');
  } else if (changed) {
    writeTable(FILES.targetGroups, expected);
  }

  const targetLoaded = loadTable(FILES.targetGroups, 'levelgroups BKVince final');
  assert(targetLoaded.raw === expectedRaw, 'levelgroups BKVince final differe du resultat gouverne');
  const localizationSummary = validateLocalization();
  const referenceSummary = validateReferences(targetLoaded.table);

  console.log(JSON.stringify({
    mode: checkOnly ? 'check' : 'generate',
    changed: checkOnly ? false : changed,
    vanillaRows: vanillaLoaded.table.rows.length,
    finalRows: targetLoaded.table.rows.length,
    addedGroup: RIFT_GROUP,
    riftLevelIds: RIFT_LEVEL_IDS,
    localizationId: RIFT_STRING_ID,
    ...localizationSummary,
    ...referenceSummary,
  }, null, 2));
}

main();
