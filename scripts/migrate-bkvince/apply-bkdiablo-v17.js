'use strict';

const assert = require('assert');
const crypto = require('crypto');
const fs = require('fs');
const path = require('path');
const {
  ENCODING,
  parseTable,
  serializeTable,
} = require('../build-data/tsv');
const manifest = require('./bkdiablo-v17-selected-delta.json');

const ROOT = path.resolve(__dirname, '..', '..');
const DATA_ROOT = path.join(ROOT, 'data-BKVince', 'BKVince.mpq', 'data');
const modeFlags = ['--apply', '--check'].filter((flag) => process.argv.includes(flag));
assert.strictEqual(modeFlags.length, 1, 'Use exactly one of --apply or --check.');
const MODE = modeFlags[0].slice(2);

function absolute(relativePath) {
  return path.join(DATA_ROOT, ...relativePath.split('/'));
}

function sha256(buffer) {
  return crypto.createHash('sha256').update(buffer).digest('hex');
}

function headerIndex(table, header, occurrence = 1) {
  let seen = 0;
  for (let index = 0; index < table.headers.length; index += 1) {
    if (table.headers[index] !== header) continue;
    seen += 1;
    if (seen === occurrence) return index;
  }
  assert.fail(`Missing header ${header}#${occurrence}`);
}

function loadTsv(definition) {
  const filePath = absolute(definition.path);
  const before = fs.readFileSync(filePath);
  const raw = before.toString(ENCODING);
  const table = parseTable(filePath);
  assert.strictEqual(serializeTable(table), raw, `${definition.path}: initial round-trip drift`);
  assert.strictEqual(table.eol, '\r\n', `${definition.path}: CRLF required`);
  assert.strictEqual(table.hasFinalEol, true, `${definition.path}: final EOL required`);
  table.rows.forEach((row, index) => {
    assert.strictEqual(row.length, table.headers.length, `${definition.path}: row ${index} width drift`);
  });

  let changedCells = 0;
  for (const change of definition.changes) {
    const rowIndex = change.targetOrdinal - 2;
    const row = table.rows[rowIndex];
    assert.ok(row, `${definition.path}: missing governed row at ordinal ${change.targetOrdinal}`);
    const keyIndex = headerIndex(table, definition.keyHeader);
    assert.ok(
      [change.rowKey, change.desiredRowKey].includes(row[keyIndex]),
      `${definition.path}: governed row ${change.targetOrdinal} is ${JSON.stringify(row[keyIndex])}`,
    );
    const columnIndex = headerIndex(table, change.header, change.headerOccurrence);
    const current = row[columnIndex] ?? '';
    assert.ok(
      current === change.base || current === change.desired,
      `${definition.path}:${change.targetOrdinal}:${change.header} has third value ${JSON.stringify(current)}`,
    );
    if (current !== change.desired) {
      row[columnIndex] = change.desired;
      changedCells += 1;
    }
  }

  const output = Buffer.from(serializeTable(table), ENCODING);
  return { definition, filePath, before, output, table, changedCells };
}

function loadJsonDocument(relativePath, requireRoundTrip = true) {
  const filePath = absolute(relativePath);
  const before = fs.readFileSync(filePath);
  const utf8 = before.toString('utf8');
  const bom = utf8.startsWith('\uFEFF');
  const text = bom ? utf8.slice(1) : utf8;
  const eol = text.includes('\r\n') ? '\r\n' : '\n';
  const finalEol = text.endsWith(eol);
  const value = JSON.parse(text);
  const serialized = `${bom ? '\uFEFF' : ''}${JSON.stringify(value, null, 2).replace(/\n/g, eol)}${finalEol ? eol : ''}`;
  if (requireRoundTrip) {
    assert.deepStrictEqual(Buffer.from(serialized, 'utf8'), before, `${relativePath}: initial JSON round-trip drift`);
  }
  return { relativePath, filePath, before, bom, eol, finalEol, value };
}

function serializeJsonDocument(document) {
  const text = JSON.stringify(document.value, null, 2).replace(/\n/g, document.eol);
  return Buffer.from(`${document.bom ? '\uFEFF' : ''}${text}${document.finalEol ? document.eol : ''}`, 'utf8');
}

function uniqueRecord(records, predicate, label) {
  const matches = records.filter(predicate);
  assert.strictEqual(matches.length, 1, `${label}: expected exactly one record, got ${matches.length}`);
  return matches[0];
}

function applyLocalization() {
  const holy = manifest.localization.holyBolt;
  const skills = loadJsonDocument(holy.path);
  const holyRecord = uniqueRecord(
    skills.value,
    ({ id }) => id === holy.id,
    `${holy.path}: id ${holy.id}`,
  );
  assert.strictEqual(holyRecord.Key, holy.Key, `${holy.path}: Holy Bolt key drift`);
  let changedCells = 0;
  for (const [field, change] of Object.entries(holy.values)) {
    assert.ok(
      holyRecord[field] === change.base || holyRecord[field] === change.desired,
      `${holy.path}:${holy.id}.${field} has third value ${JSON.stringify(holyRecord[field])}`,
    );
    if (holyRecord[field] !== change.desired) {
      holyRecord[field] = change.desired;
      changedCells += 1;
    }
  }

  const uiDefinition = manifest.localization.ui;
  const ui = loadJsonDocument(uiDefinition.path);
  const byId = new Map();
  const byKey = new Map();
  for (const record of ui.value) {
    assert.ok(!byId.has(record.id), `${uiDefinition.path}: duplicate id ${record.id}`);
    byId.set(record.id, record);
    if (!byKey.has(record.Key)) byKey.set(record.Key, []);
    byKey.get(record.Key).push(record);
  }
  for (const owner of uiDefinition.protectedOwners) {
    if (owner.optional && !byId.has(owner.id) && !byKey.has(owner.Key)) continue;
    assert.strictEqual(byId.get(owner.id)?.Key, owner.Key, `${uiDefinition.path}: protected id ${owner.id} drift`);
    assert.deepStrictEqual(
      byKey.get(owner.Key)?.map(({ id }) => id),
      [owner.id],
      `${uiDefinition.path}: protected key ${owner.Key} drift`,
    );
  }
  for (const change of uiDefinition.changes) {
    const record = byId.get(change.id);
    assert.ok(record, `${uiDefinition.path}: missing id ${change.id}`);
    assert.ok(
      record[change.field] === change.base || record[change.field] === change.desired,
      `${uiDefinition.path}:${change.id}.${change.field} has third value ${JSON.stringify(record[change.field])}`,
    );
    if (record[change.field] !== change.desired) {
      record[change.field] = change.desired;
      changedCells += 1;
    }
  }
  let addedRecords = 0;
  for (const addition of uiDefinition.additions) {
    const idOwner = byId.get(addition.id);
    const keyOwners = byKey.get(addition.Key) ?? [];
    if (idOwner || keyOwners.length) {
      assert.deepStrictEqual(idOwner, addition, `${uiDefinition.path}: id ${addition.id} conflict`);
      assert.deepStrictEqual(keyOwners, [idOwner], `${uiDefinition.path}: key ${addition.Key} conflict`);
      continue;
    }
    ui.value.push(addition);
    byId.set(addition.id, addition);
    byKey.set(addition.Key, [addition]);
    addedRecords += 1;
  }

  return {
    documents: [
      { ...skills, output: serializeJsonDocument(skills) },
      { ...ui, output: serializeJsonDocument(ui) },
    ],
    changedCells,
    addedRecords,
  };
}

function walk(value, visit) {
  if (!value || typeof value !== 'object') return;
  visit(value);
  if (Array.isArray(value)) value.forEach((child) => walk(child, visit));
  else Object.values(value).forEach((child) => walk(child, visit));
}

function widgetsByName(value, name) {
  const matches = [];
  walk(value, (candidate) => {
    if (!Array.isArray(candidate) && candidate.name === name) matches.push(candidate);
  });
  return matches;
}

function findObjectRange(text, name) {
  const token = `\"name\": ${JSON.stringify(name)}`;
  const tokenIndex = text.indexOf(token);
  assert.notStrictEqual(tokenIndex, -1, `Layout: missing ${name}`);
  assert.strictEqual(text.indexOf(token, tokenIndex + token.length), -1, `Layout: duplicate ${name}`);
  const start = text.lastIndexOf('{', tokenIndex);
  assert.notStrictEqual(start, -1, `Layout: missing object start for ${name}`);
  let depth = 0;
  let inString = false;
  let escaped = false;
  let end = -1;
  for (let index = start; index < text.length; index += 1) {
    const character = text[index];
    if (inString) {
      if (escaped) escaped = false;
      else if (character === '\\') escaped = true;
      else if (character === '"') inString = false;
      continue;
    }
    if (character === '"') inString = true;
    else if (character === '{') depth += 1;
    else if (character === '}') {
      depth -= 1;
      if (depth === 0) {
        end = index + 1;
        break;
      }
    }
  }
  assert.notStrictEqual(end, -1, `Layout: unterminated object ${name}`);
  const lineStart = text.lastIndexOf('\n', start - 1) + 1;
  const indent = text.slice(lineStart, start);
  assert.ok(/^\s*$/.test(indent), `Layout: invalid indentation for ${name}`);
  return { start, end, lineStart, indent };
}

function removeWidget(text, name, eol) {
  const range = findObjectRange(text, name);
  let end = range.end;
  assert.strictEqual(text[end], ',', `Layout: ${name} must have a following comma`);
  end += 1;
  if (text.startsWith(eol, end)) end += eol.length;
  else assert.fail(`Layout: ${name} must end with ${JSON.stringify(eol)}`);
  return text.slice(0, range.lineStart) + text.slice(end);
}

function insertWidgetBefore(text, anchor, widget, eol) {
  const range = findObjectRange(text, anchor);
  const formatted = JSON.stringify(widget, null, 2)
    .split('\n')
    .map((line) => `${range.indent}${line}`)
    .join(eol);
  return `${text.slice(0, range.lineStart)}${formatted},${eol}${text.slice(range.lineStart)}`;
}

function sameJson(left, right) {
  return JSON.stringify(left) === JSON.stringify(right);
}

function tableChildren(layout, tableName) {
  const table = uniqueRecord(
    widgetsByName(layout, tableName),
    () => true,
    `layout table ${tableName}`,
  );
  assert.ok(Array.isArray(table.children), `layout table ${tableName}: children missing`);
  return table.children;
}

function applyLayouts() {
  const definition = manifest.layouts;
  const mod = loadJsonDocument(definition.modSpecificPath, false);
  const unique = loadJsonDocument(definition.uniqueSetPath, false);
  let modText = mod.before.toString('utf8');
  let uniqueText = unique.before.toString('utf8');
  if (mod.bom) modText = modText.slice(1);
  if (unique.bom) uniqueText = uniqueText.slice(1);
  const modBlank = widgetsByName(mod.value, 'blank_charm_recipe');
  const uniqueBlank = widgetsByName(unique.value, 'blank_charm_recipe');
  assert.ok(
    (modBlank.length === 0 && uniqueBlank.length === 1)
      || (modBlank.length === 1 && uniqueBlank.length === 0),
    'Layouts: blank charm must exist once in either source or destination',
  );
  let movedBlank = false;
  if (uniqueBlank.length === 1) {
    assert.ok(sameJson(uniqueBlank[0], definition.blankCharmWidget), 'Layouts: blank charm widget drift');
    uniqueText = removeWidget(uniqueText, 'blank_charm_recipe', unique.eol);
    modText = insertWidgetBefore(
      modText,
      definition.blankCharmAnchor,
      definition.blankCharmWidget,
      mod.eol,
    );
    movedBlank = true;
  } else {
    assert.ok(sameJson(modBlank[0], definition.blankCharmWidget), 'Layouts: moved blank charm widget drift');
  }

  const currentMod = JSON.parse(modText);
  const flaskMatches = widgetsByName(currentMod, 'flask_recipe');
  assert.ok(flaskMatches.length <= 1, 'Layouts: duplicate flask recipe');
  let addedFlask = false;
  if (flaskMatches.length === 0) {
    modText = insertWidgetBefore(modText, definition.flaskAnchor, definition.flaskWidget, mod.eol);
    addedFlask = true;
  } else {
    assert.ok(sameJson(flaskMatches[0], definition.flaskWidget), 'Layouts: flask widget drift');
  }

  mod.value = JSON.parse(modText);
  unique.value = JSON.parse(uniqueText);
  assert.strictEqual(widgetsByName(mod.value, 'blank_charm_recipe').length, 1, 'Layouts: blank charm destination count');
  assert.strictEqual(widgetsByName(unique.value, 'blank_charm_recipe').length, 0, 'Layouts: blank charm source count');
  assert.strictEqual(widgetsByName(mod.value, 'flask_recipe').length, 1, 'Layouts: flask destination count');
  const children = tableChildren(mod.value, 'CubeRecipesModSpecificTable');
  const names = children.map(({ name }) => name);
  assert.ok(names.indexOf('blank_charm_recipe') < names.indexOf(definition.blankCharmAnchor), 'Layouts: blank charm order drift');
  assert.ok(names.indexOf('flask_recipe') < names.indexOf(definition.flaskAnchor), 'Layouts: flask order drift');

  const modOutput = Buffer.from(`${mod.bom ? '\uFEFF' : ''}${modText}`, 'utf8');
  const uniqueOutput = Buffer.from(`${unique.bom ? '\uFEFF' : ''}${uniqueText}`, 'utf8');
  return {
    documents: [
      { ...mod, output: modOutput },
      { ...unique, output: uniqueOutput },
    ],
    movedBlank,
    addedFlask,
  };
}

function tsvRow(table, keyHeader, key) {
  const index = headerIndex(table, keyHeader);
  return uniqueRecord(table.rows, (row) => row[index] === key, `${keyHeader}=${key}`);
}

function assertCell(table, row, header, expected, label) {
  assert.strictEqual(row[headerIndex(table, header)] ?? '', expected, label);
}

function validateExclusions(tsvDocuments, skillsJson) {
  const skills = tsvDocuments.find(({ definition }) => definition.path === 'global/excel/skills.txt').table;
  for (const summon of ['Summon Grizzly', 'Shadow Warrior', 'Shadow Master']) {
    const row = tsvRow(skills, 'skill', summon);
    assertCell(skills, row, 'sumskill1', '', `${summon}: BK summon splash assignment must stay excluded`);
    assertCell(skills, row, 'sumsk1calc', '', `${summon}: BK summon splash calc must stay excluded`);
  }
  const splash = tsvRow(skills, 'skill', 'Splash');
  assertCell(skills, splash, 'useServerMissilesOnRemoteClients', '', 'Splash: BK remote-client flag must stay excluded');
  const summonSplash = tsvRow(skills, 'skill', 'Summon Splash');
  for (const header of ['srvmissilea', 'cltmissilea', 'passiveitype', 'maxlvl', 'SrcDam']) {
    assertCell(skills, summonSplash, header, '', `Summon Splash.${header}: softcoded model must stay tombstoned`);
  }

  const missiles = parseTable(absolute('global/excel/missiles.txt'));
  const proc = tsvRow(missiles, 'Missile', 'proc_splashdamage');
  const permitted = new Set(['Missile', '*ID', '*eol']);
  missiles.headers.forEach((header, index) => {
    if (!permitted.has(header)) assert.strictEqual(proc[index] ?? '', '', `proc_splashdamage.${header}: tombstone drift`);
  });

  const itemTypes = parseTable(absolute('global/excel/itemtypes.txt'));
  const codeIndex = headerIndex(itemTypes, 'Code');
  assert.strictEqual(
    itemTypes.rows.filter((row) => /^[MNO]\d{3}$/.test(row[codeIndex] ?? '')).length,
    0,
    'BK v17 synthetic summon item types must stay excluded',
  );
  const splashString = uniqueRecord(skillsJson.value, ({ id }) => id === 28301, 'skills.json id 28301');
  assert.strictEqual(splashString.Key, 'splash3', 'BK SumSplash localization key must stay excluded');

  const cube = tsvDocuments.find(({ definition }) => definition.path === 'global/excel/cubemain.txt').table;
  const descriptionIndex = headerIndex(cube, 'description');
  assert.strictEqual(
    cube.rows.filter((row) => row[descriptionIndex] === 'Readable Items Test - Town Portal Scroll').length,
    1,
    'BKVince Readable Items Test recipe must be preserved',
  );
}

function atomicWrite(filePath, buffer) {
  const temporary = `${filePath}.bk17-${process.pid}.tmp`;
  fs.writeFileSync(temporary, buffer);
  fs.renameSync(temporary, filePath);
}

function backupChanged(documents) {
  const stamp = new Date().toISOString().replace(/[-:TZ.]/g, '');
  const root = path.join(ROOT, 'analysis-cache', 'bkdiablo-v17-apply-backups', stamp);
  for (const document of documents) {
    if (document.before.equals(document.output)) continue;
    const relative = path.relative(ROOT, document.filePath);
    const destination = path.join(root, relative);
    fs.mkdirSync(path.dirname(destination), { recursive: true });
    fs.writeFileSync(destination, document.before);
  }
  return root;
}

function main() {
  assert.strictEqual(manifest.schemaVersion, 1, 'Unsupported selected-delta manifest');
  const tsvDocuments = manifest.tsv.map(loadTsv);
  const localization = applyLocalization();
  const layouts = applyLayouts();
  validateExclusions(tsvDocuments, localization.documents[0]);
  const documents = [...tsvDocuments, ...localization.documents, ...layouts.documents];
  const changedDocuments = documents.filter(({ before, output }) => !before.equals(output));
  if (MODE === 'check') {
    assert.strictEqual(
      changedDocuments.length,
      0,
      `BKDiablo v17 selected delta is not applied (${changedDocuments.map(({ filePath }) => path.relative(ROOT, filePath)).join(', ')})`,
    );
  } else if (changedDocuments.length) {
    const backupRoot = backupChanged(documents);
    for (const document of changedDocuments) {
      atomicWrite(document.filePath, document.output);
      assert.strictEqual(sha256(fs.readFileSync(document.filePath)), sha256(document.output), `${document.filePath}: write verification failed`);
    }
    console.log(`Backup: ${path.relative(ROOT, backupRoot)}`);
  }

  console.log(JSON.stringify({
    mode: MODE,
    sourceBase: manifest.source.baseCommit,
    sourceV17: manifest.source.v17Commit,
    changedFiles: changedDocuments.map(({ filePath }) => path.relative(ROOT, filePath).replace(/\\/g, '/')),
    tsvCells: manifest.tsv.reduce((sum, table) => sum + table.changes.length, 0),
    localizationCells: Object.keys(manifest.localization.holyBolt.values).length + manifest.localization.ui.changes.length,
    localizationRecords: manifest.localization.ui.additions.length,
    layoutOperations: 3,
    excludedSummonSplash: true,
    preservedNativeMeleeSplash: true,
  }, null, 2));
}

main();
