'use strict';

// Selectively ports The Sounds of Variation 0.1g to the current BKVince 3.2
// tables. The old source tables are inputs only; they are never copied whole.

const fs = require('fs');
const path = require('path');
const {
  parseTable,
  serializeTable,
  writeTable,
  ENCODING,
} = require('../build-data/tsv');

const ROOT = path.resolve(__dirname, '..', '..');
const TARGET_EXCEL = path.join(ROOT, 'data-BKVince', 'BKVince.mpq', 'data', 'global', 'excel');
const VANILLA_EXCEL = path.join(ROOT, 'data-vanilla3.2', 'data', 'data', 'global', 'excel');
const NEW_MON_SOUND_IDS = Object.freeze([
  'sk_mage_fire',
  'sk_mage_cold',
  'sk_mage_ltng',
  'sk_mage_pois',
]);

function fail(message) {
  throw new Error(message);
}

function assert(condition, message) {
  if (!condition) fail(message);
}

function parseArgs(argv) {
  const result = { check: false };
  for (let index = 0; index < argv.length; index += 1) {
    const token = argv[index];
    if (token === '--check') {
      result.check = true;
      continue;
    }
    if (token === '--main-root' || token === '--fix-root') {
      const value = argv[index + 1];
      assert(value && !value.startsWith('--'), `${token} requires a directory`);
      result[token.slice(2).replace('-', '')] = path.resolve(value);
      index += 1;
      continue;
    }
    fail(`Unknown argument: ${token}`);
  }
  assert(result.mainroot, 'Missing --main-root <extracted mpq data directory>');
  assert(result.fixroot, 'Missing --fix-root <extracted fix mpq data directory>');
  return result;
}

function loadTable(filePath, label) {
  assert(fs.existsSync(filePath), `${label} missing: ${filePath}`);
  const raw = fs.readFileSync(filePath, ENCODING);
  const table = parseTable(filePath);
  assert(raw === serializeTable(table), `${label}: round-trip is not byte-exact`);
  assert(table.eol === '\r\n', `${label}: CRLF is required`);
  assert(
    table.rows.every((row) => row.length === table.headers.length),
    `${label}: invalid row width`,
  );
  return { raw, table };
}

function cloneTable(table) {
  return {
    headers: table.headers.slice(),
    rows: table.rows.map((row) => row.slice()),
    eol: table.eol,
    hasFinalEol: table.hasFinalEol,
  };
}

function headerIndexes(table, label) {
  const indexes = Object.fromEntries(table.headers.map((header, index) => [header, index]));
  assert(Object.keys(indexes).length === table.headers.length, `${label}: duplicate header`);
  return indexes;
}

function rowsByKey(table, keyHeader, label, allowEmpty = false) {
  const indexes = headerIndexes(table, label);
  assert(keyHeader in indexes, `${label}: missing key header ${keyHeader}`);
  const result = new Map();
  for (const row of table.rows) {
    const key = row[indexes[keyHeader]];
    if (!key && allowEmpty) continue;
    assert(key, `${label}: empty ${keyHeader}`);
    assert(!result.has(key), `${label}: duplicate ${keyHeader} ${key}`);
    result.set(key, row);
  }
  return result;
}

function mapRow(sourceTable, targetTable, sourceRow) {
  const sourceIndexes = headerIndexes(sourceTable, 'source table');
  return targetTable.headers.map((header) => (
    header in sourceIndexes ? sourceRow[sourceIndexes[header]] ?? '' : ''
  ));
}

function walkFiles(root) {
  const files = [];
  for (const entry of fs.readdirSync(root, { withFileTypes: true })) {
    const fullPath = path.join(root, entry.name);
    if (entry.isDirectory()) files.push(...walkFiles(fullPath));
    else files.push(fullPath);
  }
  return files;
}

function normalizeSoundFilename(value) {
  return String(value || '').replaceAll('\\', '/').toLowerCase();
}

function loadAudioManifest(mainRoot) {
  const audioRoot = path.join(mainRoot, 'hd', 'global', 'sfx');
  assert(fs.existsSync(audioRoot), `audio root missing: ${audioRoot}`);
  const files = walkFiles(audioRoot).filter((filePath) => path.extname(filePath).toLowerCase() === '.flac');
  assert(files.length === 1228, `expected 1228 FLAC files, found ${files.length}`);
  const relativeNames = new Set();
  for (const filePath of files) {
    const signature = fs.readFileSync(filePath).subarray(0, 4).toString('ascii');
    assert(signature === 'fLaC', `invalid FLAC signature: ${filePath}`);
    const relative = normalizeSoundFilename(path.relative(audioRoot, filePath));
    assert(!relativeNames.has(relative), `duplicate audio path: ${relative}`);
    relativeNames.add(relative);
  }
  return relativeNames;
}

function writeOrCheck(filePath, table, originalRaw, check, label) {
  const serialized = serializeTable(table);
  const changed = serialized !== originalRaw;
  if (check) {
    assert(!changed, `${label}: generated result differs from the current BKVince table`);
  } else if (changed) {
    fs.mkdirSync(path.dirname(filePath), { recursive: true });
    writeTable(filePath, table);
  }
  return changed;
}

function mergeSounds(fixRoot, audioFiles, check) {
  const sourcePath = path.join(fixRoot, 'global', 'excel', 'sounds.txt');
  const targetPath = path.join(TARGET_EXCEL, 'sounds.txt');
  const vanillaPath = path.join(VANILLA_EXCEL, 'sounds.txt');
  const source = loadTable(sourcePath, 'Sounds of Variation FIX sounds.txt').table;
  const targetLoaded = loadTable(targetPath, 'BKVince sounds.txt');
  const vanilla = loadTable(vanillaPath, 'vanilla 3.2 sounds.txt').table;
  const target = cloneTable(targetLoaded.table);
  const sourceIndexes = headerIndexes(source, 'source sounds.txt');
  const targetIndexes = headerIndexes(target, 'BKVince sounds.txt');
  const vanillaRows = rowsByKey(vanilla, 'Sound', 'vanilla 3.2 sounds.txt', true);
  const targetRows = rowsByKey(target, 'Sound', 'BKVince sounds.txt', true);
  const soundIndex = sourceIndexes.Sound;
  const sourceFilenameIndex = sourceIndexes.FileName;
  const sourceGroupSizeIndex = sourceIndexes['Group Size'];
  const targetFilenameIndex = targetIndexes.FileName;
  const targetGroupSizeIndex = targetIndexes['Group Size'];
  const targetCommentIndex = targetIndexes['*Index'];
  assert(soundIndex !== undefined, 'source sounds.txt: missing Sound');
  assert(sourceFilenameIndex !== undefined, 'source sounds.txt: missing FileName');
  assert(sourceGroupSizeIndex !== undefined, 'source sounds.txt: missing Group Size');
  assert(targetFilenameIndex !== undefined, 'BKVince sounds.txt: missing FileName');
  assert(targetGroupSizeIndex !== undefined, 'BKVince sounds.txt: missing Group Size');
  assert(targetCommentIndex !== undefined, 'BKVince sounds.txt: missing *Index');

  const groups = [];
  const coveredAudio = new Set();
  for (let rowIndex = 0; rowIndex < source.rows.length; rowIndex += 1) {
    const groupSize = Number.parseInt(source.rows[rowIndex][sourceGroupSizeIndex] || '0', 10);
    if (!Number.isInteger(groupSize) || groupSize <= 1) continue;
    const rows = source.rows.slice(rowIndex, rowIndex + groupSize);
    assert(rows.length === groupSize, `truncated sound group at source row ${rowIndex + 2}`);
    const mapped = rows.filter((row) => audioFiles.has(normalizeSoundFilename(row[sourceFilenameIndex])));
    if (mapped.length === 0) continue;
    for (const row of mapped) coveredAudio.add(normalizeSoundFilename(row[sourceFilenameIndex]));
    groups.push({
      baseKey: rows[0][soundIndex],
      groupSize,
      rows,
    });
  }
  assert(groups.length === 38, `expected 38 affected sound groups, found ${groups.length}`);
  assert(coveredAudio.size === 1228, `expected 1228 referenced FLAC files, found ${coveredAudio.size}`);
  for (const audioFile of audioFiles) {
    assert(coveredAudio.has(audioFile), `unreferenced FLAC file: ${audioFile}`);
  }

  const affectedKeys = new Set();
  for (const group of groups) {
    assert(targetRows.has(group.baseKey), `BKVince sounds.txt: missing group base ${group.baseKey}`);
    for (const row of group.rows) {
      const key = row[soundIndex];
      assert(!affectedKeys.has(key), `sound key belongs to multiple affected groups: ${key}`);
      affectedKeys.add(key);
    }
  }
  const vanillaMissing = [...affectedKeys].filter((key) => !vanillaRows.has(key));
  assert(vanillaMissing.length === 1222, `expected 1222 non-vanilla sound keys, found ${vanillaMissing.length}`);

  let nextCommentIndex = target.rows.reduce((maximum, row) => {
    const value = Number.parseInt(row[targetCommentIndex], 10);
    return Number.isInteger(value) ? Math.max(maximum, value) : maximum;
  }, -1);
  let additions = 0;
  let existingFilenameChanges = 0;
  let groupSizeChanges = 0;
  const blocks = new Map();
  for (const group of groups) {
    const block = group.rows.map((sourceRow, memberIndex) => {
      const key = sourceRow[soundIndex];
      const currentRow = targetRows.get(key);
      const output = currentRow ? currentRow.slice() : mapRow(source, target, sourceRow);
      if (!currentRow) {
        additions += 1;
        nextCommentIndex += 1;
        output[targetCommentIndex] = String(nextCommentIndex);
      }
      if (memberIndex === 0 && output[targetGroupSizeIndex] !== String(group.groupSize)) {
        output[targetGroupSizeIndex] = String(group.groupSize);
        groupSizeChanges += 1;
      }
      const sourceFilename = sourceRow[sourceFilenameIndex] || '';
      if (audioFiles.has(normalizeSoundFilename(sourceFilename))
        && output[targetFilenameIndex] !== sourceFilename) {
        if (currentRow) existingFilenameChanges += 1;
        output[targetFilenameIndex] = sourceFilename;
      }
      return output;
    });
    blocks.set(group.baseKey, block);
  }

  const rebuiltRows = [];
  const insertedBases = new Set();
  for (const row of target.rows) {
    const key = row[targetIndexes.Sound];
    if (blocks.has(key)) {
      rebuiltRows.push(...blocks.get(key));
      insertedBases.add(key);
      continue;
    }
    if (affectedKeys.has(key)) continue;
    rebuiltRows.push(row);
  }
  assert(insertedBases.size === groups.length, 'not every affected sound group was inserted');
  target.rows = rebuiltRows;

  const finalRows = rowsByKey(target, 'Sound', 'merged BKVince sounds.txt', true);
  for (const key of affectedKeys) assert(finalRows.has(key), `merged sounds.txt: missing ${key}`);
  for (const group of groups) {
    const basePosition = target.rows.findIndex((row) => row[targetIndexes.Sound] === group.baseKey);
    const actualKeys = target.rows
      .slice(basePosition, basePosition + group.groupSize)
      .map((row) => row[targetIndexes.Sound]);
    const expectedKeys = group.rows.map((row) => row[soundIndex]);
    assert(JSON.stringify(actualKeys) === JSON.stringify(expectedKeys), `sound group order mismatch: ${group.baseKey}`);
  }
  const changed = writeOrCheck(targetPath, target, targetLoaded.raw, check, 'BKVince sounds.txt');
  return {
    changed,
    additions,
    affectedGroups: groups.length,
    affectedKeys: affectedKeys.size,
    audioReferences: coveredAudio.size,
    existingFilenameChanges,
    groupSizeChanges,
    rows: target.rows.length,
  };
}

function mergeMonSounds(mainRoot, check) {
  const sourcePath = path.join(mainRoot, 'global', 'excel', 'monsounds.txt');
  const vanillaPath = path.join(VANILLA_EXCEL, 'monsounds.txt');
  const targetPath = path.join(TARGET_EXCEL, 'monsounds.txt');
  const source = loadTable(sourcePath, 'Sounds of Variation monsounds.txt').table;
  const vanillaLoaded = loadTable(vanillaPath, 'vanilla 3.2 monsounds.txt');
  const targetLoaded = fs.existsSync(targetPath)
    ? loadTable(targetPath, 'BKVince monsounds.txt')
    : { raw: null, table: cloneTable(vanillaLoaded.table) };
  const target = cloneTable(targetLoaded.table);
  const sourceIndexes = headerIndexes(source, 'source monsounds.txt');
  const vanillaIndexes = headerIndexes(vanillaLoaded.table, 'vanilla monsounds.txt');
  const targetIndexes = headerIndexes(target, 'BKVince monsounds.txt');
  const sourceRows = rowsByKey(source, 'Id', 'source monsounds.txt', true);
  const vanillaRows = rowsByKey(vanillaLoaded.table, 'Id', 'vanilla monsounds.txt', true);
  const targetRows = rowsByKey(target, 'Id', 'BKVince monsounds.txt', true);
  const sourceNecromage = sourceRows.get('necromage');
  const vanillaNecromage = vanillaRows.get('necromage');
  const targetNecromage = targetRows.get('necromage');
  assert(sourceNecromage && vanillaNecromage && targetNecromage, 'monsounds.txt: necromage row missing');

  const changedHeaders = source.headers.filter((header) => (
    header !== 'Id'
    && header in vanillaIndexes
    && header in targetIndexes
    && (sourceNecromage[sourceIndexes[header]] || '') !== (vanillaNecromage[vanillaIndexes[header]] || '')
  ));
  assert(changedHeaders.length === 9, `expected 9 necromage field deltas, found ${changedHeaders.length}`);
  let cellChanges = 0;
  for (const header of changedHeaders) {
    const value = sourceNecromage[sourceIndexes[header]] || '';
    if (targetNecromage[targetIndexes[header]] !== value) {
      targetNecromage[targetIndexes[header]] = value;
      cellChanges += 1;
    }
  }

  let additions = 0;
  for (const id of NEW_MON_SOUND_IDS) {
    const sourceRow = sourceRows.get(id);
    assert(sourceRow, `source monsounds.txt: missing ${id}`);
    if (targetRows.has(id)) continue;
    target.rows.push(mapRow(source, target, sourceRow));
    additions += 1;
  }
  const finalRows = rowsByKey(target, 'Id', 'merged BKVince monsounds.txt', true);
  for (const id of NEW_MON_SOUND_IDS) assert(finalRows.has(id), `merged monsounds.txt: missing ${id}`);
  assert(target.rows.length === vanillaLoaded.table.rows.length + 4, `unexpected monsounds.txt row count: ${target.rows.length}`);
  const changed = writeOrCheck(targetPath, target, targetLoaded.raw, check, 'BKVince monsounds.txt');
  return {
    changed,
    additions,
    cellChanges,
    necromageFields: changedHeaders,
    rows: target.rows.length,
  };
}

function mergeMonStats(fixRoot, check) {
  const sourcePath = path.join(fixRoot, 'global', 'excel', 'monstats.txt');
  const targetPath = path.join(TARGET_EXCEL, 'monstats.txt');
  const source = loadTable(sourcePath, 'Sounds of Variation FIX monstats.txt').table;
  const targetLoaded = loadTable(targetPath, 'BKVince monstats.txt');
  const target = cloneTable(targetLoaded.table);
  const sourceIndexes = headerIndexes(source, 'source monstats.txt');
  const targetIndexes = headerIndexes(target, 'BKVince monstats.txt');
  const targetRows = rowsByKey(target, 'Id', 'BKVince monstats.txt');
  const validMonSounds = new Set(NEW_MON_SOUND_IDS);
  const affectedSourceRows = source.rows.filter((row) => (
    validMonSounds.has(row[sourceIndexes.MonSound])
    || validMonSounds.has(row[sourceIndexes.UMonSound])
  ));
  assert(affectedSourceRows.length === 27, `expected 27 skeleton-mage monstats rows, found ${affectedSourceRows.length}`);

  let cellChanges = 0;
  const ids = [];
  for (const sourceRow of affectedSourceRows) {
    const id = sourceRow[sourceIndexes.Id];
    const targetRow = targetRows.get(id);
    assert(targetRow, `BKVince monstats.txt: missing ${id}`);
    ids.push(id);
    for (const header of ['MonSound', 'UMonSound']) {
      const value = sourceRow[sourceIndexes[header]] || '';
      assert(validMonSounds.has(value), `source monstats.txt: unexpected ${header} for ${id}: ${value}`);
      if (targetRow[targetIndexes[header]] !== value) {
        targetRow[targetIndexes[header]] = value;
        cellChanges += 1;
      }
    }
  }
  assert(new Set(ids).size === 27, 'source monstats.txt: duplicate affected Id');
  const changed = writeOrCheck(targetPath, target, targetLoaded.raw, check, 'BKVince monstats.txt');
  return {
    changed,
    affectedRows: ids.length,
    cellChanges,
    ids,
    rows: target.rows.length,
  };
}

function main() {
  const options = parseArgs(process.argv.slice(2));
  const audioFiles = loadAudioManifest(options.mainroot);
  const results = {
    mode: options.check ? 'check' : 'write',
    audioFiles: audioFiles.size,
    sounds: mergeSounds(options.fixroot, audioFiles, options.check),
    monsounds: mergeMonSounds(options.mainroot, options.check),
    monstats: mergeMonStats(options.fixroot, options.check),
  };
  console.log(JSON.stringify(results, null, 2));
}

try {
  main();
} catch (error) {
  console.error(`[sounds-of-variation] ${error.message}`);
  process.exitCode = 1;
}
