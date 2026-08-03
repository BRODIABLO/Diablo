import assert from 'node:assert/strict';
import test from 'node:test';

import {
  createBlankCharacter,
  editableSnapshot,
  exportCharacter,
  openCharacter,
  supportedClasses,
  validateSaveEnvelope,
} from './character-codec.js';

test('creates, reparses, and rewrites every codec-supported BKVince class', async () => {
  const classes = supportedClasses();
  assert.deepEqual(
    classes.map(({ name }) => name),
    ['Amazon', 'Sorceress', 'Necromancer', 'Paladin', 'Barbarian', 'Druid', 'Assassin', 'Warlock'],
  );

  for (const { name } of classes) {
    const document = await createBlankCharacter({ name: `Test-${name.slice(0, 8)}`, className: name });
    assert.equal(document.model.header.class, name);
    assert.equal(document.model.skills.length, 30);
    assert.equal(document.model.items.length, 0);
    assert.equal(document.model.header.filesize, document.sourceBytes.length);
    validateSaveEnvelope(document.sourceBytes);
  }
});

test('returns the original bytes for an unchanged document', async () => {
  const created = await createBlankCharacter({ name: 'ByteExact', className: 'Warlock' });
  const reopened = await openCharacter(created.sourceBytes, 'ByteExact.d2s');
  const exported = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(exported.byteExact, true);
  assert.deepEqual(exported.bytes, created.sourceBytes);
});

test('edits General and Stats values and validates the serialized result', async () => {
  const document = await createBlankCharacter({ name: 'Before', className: 'Amazon' });
  const editable = editableSnapshot(document.model);
  editable.name = 'After';
  editable.mapId = 0x12345678;
  editable.hardcore = true;
  editable.attributes.level = 42;
  editable.attributes.strength = 321;
  editable.attributes.unused_stats = 17;
  editable.attributes.gold = 123456;

  const exported = await exportCharacter(document, editable);
  assert.equal(exported.byteExact, false);
  assert.equal(exported.reparsed.header.name, 'After');
  assert.equal(exported.reparsed.header.level, 42);
  assert.equal(exported.reparsed.attributes.level, 42);
  assert.equal(exported.reparsed.attributes.strength, 321);
  assert.equal(exported.reparsed.attributes.gold, 123456);
  assert.equal(exported.reparsed.header.status.hardcore, true);
  validateSaveEnvelope(exported.bytes);
});

test('rejects corrupted checksums before parsing', async () => {
  const document = await createBlankCharacter({ name: 'Checksum', className: 'Druid' });
  const corrupted = new Uint8Array(document.sourceBytes);
  corrupted[100] ^= 0xff;
  assert.throws(() => validateSaveEnvelope(corrupted), /checksum mismatch/i);
  await assert.rejects(() => openCharacter(corrupted), /checksum mismatch/i);
});

test('fills the required skill window when a level-one model has no skill block', async () => {
  const document = await createBlankCharacter({ name: 'FirstSave', className: 'Sorceress' });
  document.model.skills = [];
  const editable = editableSnapshot(document.model);
  editable.attributes.energy += 1;
  const exported = await exportCharacter(document, editable);
  assert.equal(exported.reparsed.skills.length, 30);
  assert.equal(exported.reparsed.attributes.energy, editable.attributes.energy);
});
