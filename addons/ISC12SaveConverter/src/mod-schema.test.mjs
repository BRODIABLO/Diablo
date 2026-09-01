import assert from 'node:assert/strict';
import { existsSync } from 'node:fs';
import { mkdtemp, writeFile } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import test from 'node:test';

import bkvinceConstants from '../../../apps/hero-editor/src/data/bkvince-constants.generated.js';

import {
  createSaveSchema,
  loadConstantsFromMod,
  loadConstantsFromSchemaFile,
  resolveModDataRoot,
  resolveModDataSource,
} from './mod-schema.mjs';

const BKVINCE_ROOT = fileURLToPath(new URL('../../../data-BKVince', import.meta.url));
const INSTALLED_PACKED_MOD = 'C:/Games/Diablo II Resurrected/mods/yupgoolg132';

test('resolves a governed unpacked mod root without a BKVince code path', async () => {
  const dataRoot = await resolveModDataRoot(BKVINCE_ROOT);
  assert.match(dataRoot.replaceAll('\\', '/'), /data-BKVince\/BKVince\.mpq\/data$/);
});

test('loads a partial mod as an overlay on the bundled vanilla tables', async () => {
  const loaded = await loadConstantsFromMod(BKVINCE_ROOT);
  assert.ok(loaded.constants.magical_properties.length > 300);
  assert.ok(Object.keys(loaded.constants.weapon_items).length > 100);
  assert.ok(loaded.overlaidExcelFiles.includes('ItemStatCost.txt'));
  assert.ok(!loaded.overlaidExcelFiles.includes('PlayerClass.txt'));
  assert.ok(loaded.overlaidStringFiles.includes('item-names.json'));
});

test('reads known schema tables directly from an installed binary MPQ', {
  skip: !existsSync(INSTALLED_PACKED_MOD),
}, async () => {
  const source = await resolveModDataSource(INSTALLED_PACKED_MOD);
  assert.match(source.archivePath.replaceAll('\\', '/'), /yupgoolg132\/yupgoolg132\.mpq$/i);
  const loaded = await loadConstantsFromMod(INSTALLED_PACKED_MOD);
  assert.ok(loaded.overlaidExcelFiles.includes('ItemStatCost.txt'));
  assert.ok(loaded.overlaidStringFiles.includes('item-names.json'));
  assert.equal(loaded.constants.magical_properties.length, 511);
});

test('loads a generic versioned schema pack supplied by a mod author', async () => {
  const root = await mkdtemp(path.join(tmpdir(), 'isc12-schema-'));
  const schemaPath = path.join(root, 'custom-mod.isc12-schema.json');
  await writeFile(schemaPath, JSON.stringify(createSaveSchema(bkvinceConstants, 'Custom Mod')));
  const loaded = await loadConstantsFromSchemaFile(schemaPath);
  assert.equal(loaded.name, 'Custom Mod');
  assert.ok(loaded.constants.magical_properties.length > 300);
  assert.ok(Object.keys(loaded.constants.weapon_items).length > 100);
});

test('fails closed when no unpacked mod schema exists', async () => {
  await assert.rejects(
    () => resolveModDataRoot(fileURLToPath(new URL('.', import.meta.url))),
    /No unpacked mod data/,
  );
});
