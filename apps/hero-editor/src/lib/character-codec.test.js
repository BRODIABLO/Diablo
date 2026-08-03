import assert from 'node:assert/strict';
import test from 'node:test';
import { write } from '@d2runewizard/d2s';

import {
  BK_STARTER_CHARM_LAYOUT,
  CODEC_OPTIONS,
  containerForPlacement,
  createBlankCharacter,
  describeItem,
  editableSnapshot,
  exportCharacter,
  moveItemPlacement,
  openCharacter,
  supportedClasses,
  validateSaveEnvelope,
} from './character-codec.js';
import constants from '../data/bkvince-constants.generated.js';

const REAL_BKVINCE_ITEM_FIXTURE = Buffer.from(`
VapVqmkAAAAtBAAAIa+fqwAAAAAAAAAABBAeAQAAAAD6eY9p////////AAD//wAA//8AAP//AAD//wAA//8AAP//AAD//wAA//8AAP//AAD//wAA//8AAP//AAD//wAA//8AAP//AAAAAAAAAAAAAAAAAAAAAAAA//////8E/0////////////////////////////////+AAAD4PIQzAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABCYXJiYXJpYW4AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFdvbyEGAAAAKgEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABXUwEAAABQAAIBAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAIBAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAIBAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAF3NAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABnZgA8CKCAAAoGZEBAvQJiBgD2wQGAfYAAgA0kAGADCgC4wAIAX8Bg3MDwyjr6P2lmAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAASk0JABAgogAVAADPTwAQIKIAFQQAz08AECCiABUIAM9PABAgogAVDADPTwAQIKIABeTEkAgQIKIABaTkRyIAECCCAA0RAD8ecoU3XgMCDg7+AxAgggBNFUChCI3Sv80BAQQYGPgPEACAAAXoRdhPYMUqnHZf6Av+BAHQBKAJoA2gneCWgtsKbi24VQnyH0pNAABqZmtmAAEAbGYAAA==
`.replace(/\s/g, ''), 'base64');

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
    assert.equal(document.model.items.length, 8);
    assert.deepEqual(
      document.model.items.map((item) => ({
        type: item.type,
        uniqueId: item.unique_id,
        x: item.position_x,
        y: item.position_y,
      })),
      BK_STARTER_CHARM_LAYOUT.map(({ type, uniqueId, x, y }) => ({ type, uniqueId, x, y })),
    );
    assert.ok(document.model.items.every((item) => (
      item.identified === 1
      && item.starter_item === 1
      && item.new === 0
      && item.quality === 7
      && item.type_id === 3
      && item.max_durability === 0
      && item.location_id === 0
      && item.alt_position_id === 1
      && item.timestamp === 1
      && item._unknown_data.realm_data.length === 4
    )));
    assert.ok(document.model.items.filter((item) => item.type === 'mfd').every((item) => (
      item._unknown_data.chest_stackable === 1
      && item.amount_in_shared_stash === 0
    )));
    assert.deepEqual(
      document.model.items.find((item) => item.type === 'mfc').magic_attributes,
      [
        { id: 39, values: [-30], name: 'fireresist' },
        { id: 41, values: [-30], name: 'lightresist' },
        { id: 43, values: [-30], name: 'coldresist' },
        { id: 45, values: [-30], name: 'poisonresist' },
        { id: 80, values: [-199], name: 'item_magicbonus' },
        { id: 240, values: [4], name: 'item_find_magic_perlevel' },
      ],
    );
    assert.deepEqual(
      document.model.items.find((item) => item.type === 'mff').magic_attributes,
      [{ id: 80, values: [35], name: 'item_magicbonus' }],
    );
    assert.equal(document.model.header.filesize, document.sourceBytes.length);
    assert.ok(Object.values(document.model.header.menu_appearance).every(
      ({ graphic, tint }) => graphic === 0xff && tint === 0xff,
    ));
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

test('moves an existing item between BKVince containers without rebuilding its record', async () => {
  const document = await characterWithSimpleItems([
    simpleItem('r01', 0, 0),
    simpleItem('r02', 1, 0),
  ]);
  const editable = editableSnapshot(document.model);
  const moved = moveItemPlacement(editable.itemPlacements, document.model.items, 0, 'cube', 5, 5);
  const result = await exportCharacter(document, { ...editable, itemPlacements: moved });

  assert.equal(result.byteExact, false);
  assert.equal(result.reparsed.items[0].type, 'r01');
  assert.equal(result.reparsed.items[0].location_id, 0);
  assert.equal(result.reparsed.items[0].alt_position_id, 4);
  assert.equal(result.reparsed.items[0].position_x, 5);
  assert.equal(result.reparsed.items[0].position_y, 5);
  assert.equal(containerForPlacement(moved[0]), 'cube');
  validateSaveEnvelope(result.bytes);
});

test('rejects item overlap and BKVince container overflow', async () => {
  const document = await characterWithSimpleItems([
    simpleItem('r01', 0, 0),
    simpleItem('r02', 1, 0),
  ]);
  const editable = editableSnapshot(document.model);
  assert.throws(
    () => moveItemPlacement(editable.itemPlacements, document.model.items, 0, 'inventory', 1, 0),
    /overlaps/i,
  );

  const tallItem = simpleItem('tbk', 0, 0);
  const tallPlacement = [{
    index: 0,
    type: 'tbk',
    locationId: 0,
    equippedId: 0,
    x: 0,
    y: 0,
    altPositionId: 1,
  }];
  assert.deepEqual(
    { width: describeItem(tallItem, 0).width, height: describeItem(tallItem, 0).height },
    { width: 1, height: 2 },
  );
  assert.throws(
    () => moveItemPlacement(tallPlacement, [tallItem], 0, 'cube', 0, 5),
    /does not fit/i,
  );
});

test('preserves every non-placement field in a real BKVince multi-item save', async () => {
  const document = await openCharacter(REAL_BKVINCE_ITEM_FIXTURE, 'DummyTester-Annihilus.d2s');
  const editable = editableSnapshot(document.model);
  assert.equal(document.model.items.length, 9);
  assert.equal(editable.itemPlacements.filter((entry) => containerForPlacement(entry) === 'belt').length, 4);
  assert.equal(editable.itemPlacements.filter((entry) => containerForPlacement(entry) === 'equipment').length, 2);
  assert.equal(editable.itemPlacements.filter((entry) => containerForPlacement(entry) === 'inventory').length, 3);
  assert.deepEqual(
    editable.itemPlacements.find((entry) => entry.type === 'cm1'),
    { index: 8, type: 'cm1', locationId: 0, equippedId: 0, x: 10, y: 7, altPositionId: 1 },
  );

  const noOp = await exportCharacter(document, editable);
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, new Uint8Array(REAL_BKVINCE_ITEM_FIXTURE));

  const targetIndex = document.model.items.findIndex((item) => item.type === 'tsc');
  const moved = moveItemPlacement(editable.itemPlacements, document.model.items, targetIndex, 'cube', 0, 0);
  const exported = await exportCharacter(document, { ...editable, itemPlacements: moved });
  assert.equal(exported.reparsed.items[targetIndex].alt_position_id, 4);
  assert.equal(exported.reparsed.items[targetIndex].position_x, 0);
  assert.equal(exported.reparsed.items[targetIndex].position_y, 0);
  assert.equal(exported.reparsed.items.find((item) => item.type === 'cm1').magic_attributes.length, 10);
  document.model.items.forEach((item, index) => {
    assert.deepEqual(itemPayload(item), itemPayload(exported.reparsed.items[index]));
  });
});

async function characterWithSimpleItems(items) {
  const blank = await createBlankCharacter({ name: 'ItemTest', className: 'Warlock' });
  const model = structuredClone(blank.model);
  model.items = items;
  const bytes = new Uint8Array(await write(model, constants, CODEC_OPTIONS));
  return openCharacter(bytes, 'ItemTest.d2s');
}

function simpleItem(type, x, y) {
  return {
    identified: 1,
    socketed: 0,
    new: 0,
    is_ear: 0,
    starter_item: 0,
    simple_item: 1,
    ethereal: 0,
    personalized: 0,
    given_runeword: 0,
    version: '101',
    location_id: 0,
    equipped_id: 0,
    position_x: x,
    position_y: y,
    alt_position_id: 1,
    type,
    nr_of_items_in_sockets: 0,
    _unknown_data: {},
    socketed_items: [],
  };
}

function itemPayload(item) {
  const payload = structuredClone(item);
  delete payload.location_id;
  delete payload.equipped_id;
  delete payload.position_x;
  delete payload.position_y;
  delete payload.alt_position_id;
  return payload;
}
