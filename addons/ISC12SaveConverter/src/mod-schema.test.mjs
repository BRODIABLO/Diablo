import assert from 'node:assert/strict';
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
} from './mod-schema.mjs';

const BKVINCE_ROOT = fileURLToPath(new URL('../../../data-BKVince', import.meta.url));

test('resolves a governed unpacked mod root without a BKVince code path', async () => {
  const dataRoot = await resolveModDataRoot(BKVINCE_ROOT);
  assert.match(dataRoot.replaceAll('\\', '/'), /data-BKVince\/BKVince\.mpq\/data$/);
});

test('fails closed when an unpacked mod inherits required tables not present on disk', async () => {
  await assert.rejects(
    () => loadConstantsFromMod(BKVINCE_ROOT),
    /Missing required Excel tables: PlayerClass\.txt, RarePrefix\.txt, RareSuffix\.txt/,
  );
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
