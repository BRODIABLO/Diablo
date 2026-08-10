import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import test from 'node:test';
import { write } from '@d2runewizard/d2s';

import {
  addChronicleEntrySnapshot,
  addCatalogItemBatchSnapshot,
  addItemBatchSnapshot,
  addItemGroupSnapshot,
  addImportedItemToEquipmentSlotSnapshot,
  addImportedItemsSnapshot,
  addItemToEquipmentSlotSnapshot,
  applyRunewordSnapshot,
  activePlacedItems,
  availableBeltItemBases,
  availableItemBases,
  availableItemGroups,
  availableNamedItems,
  availableRunewordItems,
  availableEquipmentItemBases,
  beltCapacityForPlacements,
  carriedGoldGameMaximum,
  availableMagicAttributes,
  BK_STARTER_AUXILIARY_LAYOUT,
  BK_STARTER_CHARM_LAYOUT,
  characterItemIds,
  chronicleCatalog,
  CODEC_OPTIONS,
  compileItemTierPatch,
  compileMagicAffixAttributes,
  compileMagicAffixPatch,
  compileManualPropertyPatch,
  compileNamedQualityPatch,
  compileRareAffixPatch,
  compileRunewordPatch,
  compileSetBonusPatch,
  containerForPlacement,
  createBlankCharacter,
  createMercenarySnapshot,
  demonDefinitions,
  describeItem,
  describeChronicleEntry,
  duplicateItemSnapshot,
  editStackableSharedStashCounterSnapshot,
  emptyPersonalStashSnapshot,
  editItemSnapshot,
  editableItems,
  editableSharedStashInventorySnapshot,
  editableSnapshot,
  exportCharacter,
  exportSharedStash,
  exportSharedStashInventory,
  exportItemBundle,
  exportItemRecord,
  extractSocketFillerSnapshot,
  importItemFiles,
  hydrateSharedStashInventory,
  goldLimits,
  itemEditorOptions,
  itemBonusDashboard,
  itemBonusSummary,
  itemContainers,
  itemRecords,
  insertImportedSocketFillersSnapshot,
  insertSocketFillerSnapshot,
  moveItemPlacement,
  moveItemToEquipmentSlot,
  mercenaryDefinitions,
  openCharacter,
  openSharedStash,
  previewManualPropertyPatch,
  removeItemSnapshot,
  removeSocketFillerSnapshot,
  removeChronicleEntrySnapshot,
  removeMercenarySnapshot,
  resetSkillsSnapshot,
  setAllQuestsSnapshot,
  setAllWaypointsSnapshot,
  setQuestCompletionSnapshot,
  setQuestConsumedScrollSnapshot,
  setSkillPointsSnapshot,
  setWaypointSnapshot,
  skillEditorDefinition,
  supportedClasses,
  transferItemSnapshot,
  unlockHellSnapshot,
  updateChronicleEntrySnapshot,
  validateSaveEnvelope,
} from './character-codec.js';
import constants, {
  demonCatalog,
  itemCatalog,
  skillCatalog,
} from '../data/bkvince-constants.generated.js';
import {
  buildDemonCatalog,
  buildItemCatalog,
  buildMercenaryCatalog,
} from '../../scripts/generate-constants.mjs';

const REAL_BKVINCE_ITEM_FIXTURE = Buffer.from(`
VapVqmkAAAAtBAAAIa+fqwAAAAAAAAAABBAeAQAAAAD6eY9p////////AAD//wAA//8AAP//AAD//wAA//8AAP//AAD//wAA//8AAP//AAD//wAA//8AAP//AAD//wAA//8AAP//AAAAAAAAAAAAAAAAAAAAAAAA//////8E/0////////////////////////////////+AAAD4PIQzAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABCYXJiYXJpYW4AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFdvbyEGAAAAKgEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABXUwEAAABQAAIBAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAIBAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAIBAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAF3NAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABnZgA8CKCAAAoGZEBAvQJiBgD2wQGAfYAAgA0kAGADCgC4wAIAX8Bg3MDwyjr6P2lmAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAASk0JABAgogAVAADPTwAQIKIAFQQAz08AECCiABUIAM9PABAgogAVDADPTwAQIKIABeTEkAgQIKIABaTkRyIAECCCAA0RAD8ecoU3XgMCDg7+AxAgggBNFUChCI3Sv80BAQQYGPgPEACAAAXoRdhPYMUqnHZf6Av+BAHQBKAJoA2gneCWgtsKbi24VQnyH0pNAABqZmtmAAEAbGYAAA==
`.replace(/\s/g, ''), 'base64');

const D2R_RESAVED_ITEM_FIXTURE = Buffer.from(`
VapVqmkAAAC9BAAASy1CygAAAAAAAAAABBAeYwAAAADkk3Bq////////AAD//wAA//8AAP//AAD//wAA//8AAP//AAD//wAA//8A
AP//AAD//wAA//8AAP//AAD//wAA//8AAP//AAAAAAAAAAAAAAAAAAAAAAAA//////8E/0//////////////////////////////
//+AAAD4PIQzAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADAABoYXgg/wIAAAQAAABidWMg/wIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABI
RUl0ZW1Nb3ZlAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFdvbyEGAAAAKgEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABXUwEAAABQAAIBAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAIBAQAAAAAAAAAAAAAAAAAA
AAAAAAAAAAIBAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAF3NAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAABnZgA8CKCAAAoGZEBAvQJiBgD2wQGAfYAAABAkAGADCgCkwQIAX8Bg3MDwyjr6P2lmAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAASk0JABAAogAVAADPT0dx4SWZMpR+Kwf+OKKamC8AEACiABUEAM9PialXoGXykjxnX3jHZTYU2QEQAKIAFQgA
z08pBaF1mfqaPELH42fWKug9ARAAogAVDADPT0utVEC3+J9GYRdwleLBmnoAEACiAAUA0JDoxhCTSeIdkFghTAi2hllkNBAAogAF
pORHolTk3fskcU8loN82tqFCrW8AEACAAAXoRdhPYMUqnB5f6IvdIN+hQQVAv7WBTYDIYnJyANAEoAmgDaCd4JaC2wpuLbhVCfIn
+B8QAIIADREAPx5yhTdeA8LtDmQpSIemGk7l1QWByFg1Dg7+AxAAggBNFUChCI3Sv80BoREBwTZoNpHJKRFEGtA8qR4EGBj4D0pN
AABqZmtmAAEAbGYAAA==
`.replace(/\s/g, ''), 'base64');

const NATIVE_MAGIC_D2I_FIXTURE = Buffer.from(
  'EADAAAXIRCgIeH4HIGNCNO34t+CfqfvPNNiIbPvgoWQrx8FBjlVWsaQopv8A',
  'base64',
);

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
    assert.equal(document.model.items.length, 10);
    const starterCharms = document.model.items.filter(({ type }) => ['mff', 'mfc', 'mfd'].includes(type));
    assert.deepEqual(
      starterCharms.map((item) => ({
        type: item.type,
        uniqueId: item.unique_id,
        x: item.position_x,
        y: item.position_y,
      })),
      BK_STARTER_CHARM_LAYOUT.map(({ type, uniqueId, x, y }) => ({ type, uniqueId, x, y })),
    );
    assert.ok(starterCharms.every((item) => (
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
    assert.ok(starterCharms.filter((item) => item.type === 'mfd').every((item) => (
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
    const auxiliary = document.model.items.filter(({ type }) => ['box', 'tsc'].includes(type));
    assert.deepEqual(
      auxiliary.map(({ type, position_x: x, position_y: y }) => ({ type, x, y })),
      BK_STARTER_AUXILIARY_LAYOUT.map(({ type, x, y }) => ({ type, x, y })),
    );
    assert.ok(auxiliary.every((item) => (
      item.identified === 1
      && item.starter_item === 1
      && item.location_id === 0
      && item.alt_position_id === 1
      && item.timestamp === 1
      && item._unknown_data.realm_data.length === 4
    )));
    const starterCube = auxiliary.find(({ type }) => type === 'box');
    assert.equal(starterCube.simple_item, 0);
    assert.equal(starterCube.quality, 2);
    assert.equal(starterCube.quest_difficulty, 0);
    assert.deepEqual(starterCube.magic_attributes, []);
    const starterScroll = auxiliary.find(({ type }) => type === 'tsc');
    assert.equal(starterScroll.simple_item, 1);
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

test('opens the governed BKVince Chronicle and preserves an unchanged Shared Stash byte-for-byte', async () => {
  const fixture = new Uint8Array(await readFile(new URL(
    '../../../../data-BKVince/ModernSharedStashSoftCoreV2.d2i',
    import.meta.url,
  )));
  const document = openSharedStash(fixture, 'ModernSharedStashSoftCoreV2.d2i');
  assert.equal(document.pageCount, 6);
  assert.equal(document.chronicleOffset, 502);
  assert.equal(document.chronicleSize, 178);
  assert.deepEqual(
    Object.fromEntries(['setItems', 'uniqueItems', 'runewords'].map((category) => [
      category,
      document.initial[category].length,
    ])),
    { setItems: 0, uniqueItems: 3, runewords: 0 },
  );
  assert.deepEqual(
    document.initial.uniqueItems.map(({ itemId, monster, foundAt }) => ({ itemId, monster, foundAt })),
    [
      { itemId: 473, monster: 3319, foundAt: 29586827 * 60 },
      { itemId: 471, monster: 29000, foundAt: 29586851 * 60 },
      { itemId: 233, monster: 3139, foundAt: 29587616 * 60 },
    ],
  );
  assert.equal(describeChronicleEntry('uniqueItems', document.initial.uniqueItems[2]).name, 'Gravepalm');
  assert.equal(document.chronicle.trailingBytes.length, 64);

  const exported = exportSharedStash(document, document.initial);
  assert.equal(exported.byteExact, true);
  assert.deepEqual(exported.bytes, fixture);
});

test('scans 1,001 BKVince stash pages and edits Chronicle metadata without touching page bytes', async () => {
  const fixture = new Uint8Array(await readFile(new URL(
    '../../../../data-BKVince/ModernSharedStashSoftCoreV2.d2i',
    import.meta.url,
  )));
  const governed = openSharedStash(fixture);
  const fixtureView = new DataView(fixture.buffer, fixture.byteOffset, fixture.byteLength);
  const firstPageSize = fixtureView.getUint32(16, true);
  const firstPage = fixture.slice(0, firstPageSize);
  const chronicle = fixture.slice(governed.chronicleOffset);
  const extended = new Uint8Array(firstPage.length * 1001 + chronicle.length);
  for (let page = 0; page < 1001; page += 1) extended.set(firstPage, page * firstPage.length);
  extended.set(chronicle, firstPage.length * 1001);

  const document = openSharedStash(extended, 'BKVince-1001-pages.d2i');
  assert.equal(document.pageCount, 1001);
  const changed = updateChronicleEntrySnapshot(document.initial, {
    category: 'uniqueItems',
    index: 2,
    monster: 65535,
    foundAt: document.initial.uniqueItems[2].foundAt + 600,
  });
  const exported = exportSharedStash(document, changed);
  assert.equal(exported.byteExact, false);
  assert.deepEqual(
    exported.bytes.slice(0, document.chronicleOffset),
    extended.slice(0, document.chronicleOffset),
  );
  assert.deepEqual(exported.reparsed.chronicle.trailingBytes, document.chronicle.trailingBytes);
  const gravepalm = exported.reparsed.initial.uniqueItems.find(({ itemId }) => itemId === 233);
  assert.equal(gravepalm.monster, 65535);
  assert.equal(gravepalm.foundAt, changed.uniqueItems[2].foundAt);
});

test('adds and removes governed Chronicle discoveries while rebuilding changed-count trailing data', async () => {
  const fixture = new Uint8Array(await readFile(new URL(
    '../../../../data-BKVince/ModernSharedStashSoftCoreV2.d2i',
    import.meta.url,
  )));
  const document = openSharedStash(fixture);
  const setItem = chronicleCatalog.setItems[0];
  const changed = addChronicleEntrySnapshot(document.initial, {
    category: 'setItems',
    itemId: setItem.itemId,
    monster: 42,
    foundAt: 1775257200,
  });
  const exported = exportSharedStash(document, changed);
  assert.equal(exported.reparsed.initial.setItems.length, 1);
  assert.equal(describeChronicleEntry('setItems', exported.reparsed.initial.setItems[0]).name, setItem.name);
  assert.equal(exported.reparsed.chronicle.trailingBytes.length, 64);
  assert.ok(exported.reparsed.chronicle.trailingBytes.every((byte) => byte === 0));
  assert.deepEqual(
    exported.bytes.slice(0, document.chronicleOffset),
    fixture.slice(0, document.chronicleOffset),
  );

  const removed = removeChronicleEntrySnapshot(exported.reparsed.initial, {
    category: 'setItems',
    index: 0,
  });
  assert.equal(removed.setItems.length, 0);
  assert.throws(
    () => addChronicleEntrySnapshot(changed, {
      category: 'setItems',
      itemId: setItem.itemId,
      monster: 42,
      foundAt: 1775257200,
    }),
    /already recorded/i,
  );
});

test('rejects malformed or ambiguous Shared Stash envelopes before Chronicle parsing', async () => {
  const fixture = new Uint8Array(await readFile(new URL(
    '../../../../data-BKVince/ModernSharedStashSoftCoreV2.d2i',
    import.meta.url,
  )));
  const document = openSharedStash(fixture);
  const badSize = fixture.slice();
  new DataView(badSize.buffer).setUint32(16, 0xffffffff, true);
  assert.throws(() => openSharedStash(badSize), /invalid size/i);

  const badFormat = fixture.slice();
  new DataView(badFormat.buffer).setUint16(document.chronicleOffset + 68, 2, true);
  assert.throws(() => openSharedStash(badFormat), /Chronicle format version 2/i);

  assert.throws(
    () => openSharedStash(fixture.slice(0, document.chronicleOffset)),
    /does not contain a Chronicle/i,
  );
});

test('hydrates Shared Stash page items and preserves an unchanged inventory byte-for-byte', async () => {
  const fixture = await readFile(new URL(
    '../../../../data-BKVince/ModernSharedStashSoftCoreV2.d2i',
    import.meta.url,
  ));
  const scanned = openSharedStash(fixture, 'ModernSharedStashSoftCoreV2.d2i');
  assert.equal(scanned.pageSectors.length, 6);
  assert.deepEqual(
    scanned.pageSectors.map(({ index, itemCount, isStackable }) => ({ index, itemCount, isStackable })),
    [
      { index: 0, itemCount: 0, isStackable: false },
      { index: 1, itemCount: 0, isStackable: false },
      { index: 2, itemCount: 0, isStackable: false },
      { index: 3, itemCount: 0, isStackable: false },
      { index: 4, itemCount: 0, isStackable: false },
      { index: 5, itemCount: 9, isStackable: true },
    ],
  );
  const document = await hydrateSharedStashInventory(scanned);
  const editable = editableSharedStashInventorySnapshot(document);
  assert.equal(document.pages[5].items.length, 9);
  assert.equal(document.pages[5].items[0].type, 'gcg');
  const exported = await exportSharedStashInventory(document, document.initial, editable);
  assert.equal(exported.byteExact, true);
  assert.deepEqual(Buffer.from(exported.bytes), fixture);
});

test('adds and edits an item on one Shared Stash grid page without rewriting its neighbours or Chronicle', async () => {
  const fixture = await readFile(new URL(
    '../../../../data-BKVince/ModernSharedStashSoftCoreV2.d2i',
    import.meta.url,
  ));
  const document = await hydrateSharedStashInventory(openSharedStash(fixture));
  const initialInventory = editableSharedStashInventorySnapshot(document);
  const page = initialInventory.pages[0];
  const editedPage = addItemBatchSnapshot(page, document.pages[0].items, {
    type: 'hax',
    count: 1,
    itemLevel: 41,
    containerId: 'stash',
    x: 2,
    y: 3,
    reservedItemIds: characterItemIds({ items: document.pages.flatMap(({ items }) => items) }),
  });
  const editedInventory = {
    pages: initialInventory.pages.map((candidate, index) => (index === 0 ? editedPage : candidate)),
  };
  editedInventory.pages[0] = editItemSnapshot(
    editedInventory.pages[0],
    document.pages[0].items,
    0,
    { identified: false },
  );
  const exported = await exportSharedStashInventory(document, document.initial, editedInventory);
  assert.equal(exported.byteExact, false);
  assert.equal(exported.reparsed.pages[0].items.length, 1);
  assert.equal(exported.reparsed.pages[0].items[0].type, 'hax');
  assert.equal(exported.reparsed.pages[0].items[0].position_x, 2);
  assert.equal(exported.reparsed.pages[0].items[0].position_y, 3);
  assert.equal(exported.reparsed.pages[0].items[0].identified, 0);
  assert.deepEqual(exported.reparsed.initial, document.initial);

  for (let index = 1; index < document.pages.length; index += 1) {
    const before = document.pages[index];
    const after = exported.reparsed.pages[index];
    assert.deepEqual(
      Buffer.from(exported.bytes.subarray(after.offset, after.offset + after.size)),
      Buffer.from(document.sourceBytes.subarray(before.offset, before.offset + before.size)),
    );
  }
  const secondNoOp = await exportSharedStashInventory(
    exported.reparsed,
    exported.reparsed.initial,
    editableSharedStashInventorySnapshot(exported.reparsed),
  );
  assert.equal(secondNoOp.byteExact, true);
  assert.deepEqual(Buffer.from(secondNoOp.bytes), Buffer.from(exported.bytes));
});

test('exports Shared Stash page and Chronicle edits together without cross-sector loss', async () => {
  const fixture = await readFile(new URL(
    '../../../../data-BKVince/ModernSharedStashSoftCoreV2.d2i',
    import.meta.url,
  ));
  const document = await hydrateSharedStashInventory(openSharedStash(fixture));
  const inventory = editableSharedStashInventorySnapshot(document);
  inventory.pages[0] = addItemBatchSnapshot(inventory.pages[0], document.pages[0].items, {
    type: 'rin',
    count: 1,
    itemLevel: 67,
    containerId: 'stash',
    x: 4,
    y: 5,
    reservedItemIds: characterItemIds({ items: document.pages.flatMap(({ items }) => items) }),
  });
  const chronicleItem = chronicleCatalog.uniqueItems.find(
    ({ itemId }) => !document.initial.uniqueItems.some((record) => record.itemId === itemId),
  );
  assert.ok(chronicleItem);
  const chronicle = addChronicleEntrySnapshot(document.initial, {
    category: 'uniqueItems',
    itemId: chronicleItem.itemId,
    monster: 412,
    foundAt: 1_785_000_000,
  });

  const exported = await exportSharedStashInventory(document, chronicle, inventory);
  assert.equal(exported.reparsed.pages[0].items.length, 1);
  assert.equal(exported.reparsed.pages[0].items[0].type, 'rin');
  assert.ok(exported.reparsed.initial.uniqueItems.some((record) => (
    record.itemId === chronicleItem.itemId
    && record.monster === 412
    && record.foundAt === 1_785_000_000
  )));
  for (let index = 1; index < document.pages.length; index += 1) {
    const before = document.pages[index];
    const after = exported.reparsed.pages[index];
    assert.deepEqual(
      Buffer.from(exported.bytes.subarray(after.offset, after.offset + after.size)),
      Buffer.from(document.sourceBytes.subarray(before.offset, before.offset + before.size)),
    );
  }
});

test('edits only the proven 8-bit counters on the BKVince stackable Shared Stash page', async () => {
  const fixture = await readFile(new URL(
    '../../../../data-BKVince/ModernSharedStashSoftCoreV2.d2i',
    import.meta.url,
  ));
  const document = await hydrateSharedStashInventory(openSharedStash(fixture));
  const initial = editableSharedStashInventorySnapshot(document);
  const editedPage = editStackableSharedStashCounterSnapshot(
    initial.pages[5],
    document.pages[5].items,
    0,
    255,
  );
  assert.equal(initial.pages[5].itemEdits[0].amountInSharedStash, 0);
  assert.equal(editedPage.itemEdits[0].amountInSharedStash, 255);
  const editable = {
    pages: initial.pages.map((page, index) => (index === 5 ? editedPage : page)),
  };
  const exported = await exportSharedStashInventory(document, document.initial, editable);
  assert.equal(exported.byteExact, false);
  assert.equal(exported.reparsed.pages[5].items[0].amount_in_shared_stash, 255);
  assert.equal(exported.reparsed.pages[5].items[0]._unknown_data.chest_stackable, 1);
  assert.deepEqual(exported.reparsed.initial, document.initial);
  for (let index = 0; index < 5; index += 1) {
    const before = document.pages[index];
    const after = exported.reparsed.pages[index];
    assert.deepEqual(
      Buffer.from(exported.bytes.subarray(after.offset, after.offset + after.size)),
      Buffer.from(document.sourceBytes.subarray(before.offset, before.offset + before.size)),
    );
  }
  const secondNoOp = await exportSharedStashInventory(
    exported.reparsed,
    exported.reparsed.initial,
    editableSharedStashInventorySnapshot(exported.reparsed),
  );
  assert.equal(secondNoOp.byteExact, true);
  assert.deepEqual(Buffer.from(secondNoOp.bytes), Buffer.from(exported.bytes));

  for (const invalidAmount of [-1, 256, 1.5]) {
    assert.throws(
      () => editStackableSharedStashCounterSnapshot(
        initial.pages[5],
        document.pages[5].items,
        0,
        invalidAmount,
      ),
      /counter must be an integer between 0 and 255/i,
    );
  }

  const forbiddenPropertyEdit = structuredClone(initial);
  forbiddenPropertyEdit.pages[5].itemEdits[0].identified = false;
  await assert.rejects(
    () => exportSharedStashInventory(document, document.initial, forbiddenPropertyEdit),
    /only permits native counter edits/i,
  );
  const forbiddenPlacementEdit = structuredClone(initial);
  forbiddenPlacementEdit.pages[5].itemPlacements[0].x += 1;
  await assert.rejects(
    () => exportSharedStashInventory(document, document.initial, forbiddenPlacementEdit),
    /cannot change overlapping native coordinates/i,
  );
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
  editable.attributes.stashed_gold = 2345678;

  assert.equal(carriedGoldGameMaximum(1), 10_000);
  assert.equal(carriedGoldGameMaximum(42), 420_000);
  assert.equal(carriedGoldGameMaximum(99), goldLimits.carriedMaximum);
  assert.equal(goldLimits.stashedMaximum, 2_500_000);
  assert.throws(() => carriedGoldGameMaximum(0), /Character level/i);

  const exported = await exportCharacter(document, editable);
  assert.equal(exported.byteExact, false);
  assert.equal(exported.reparsed.header.name, 'After');
  assert.equal(exported.reparsed.header.level, 42);
  assert.equal(exported.reparsed.attributes.level, 42);
  assert.equal(exported.reparsed.attributes.strength, 321);
  assert.equal(exported.reparsed.attributes.gold, 123456);
  assert.equal(exported.reparsed.attributes.stashed_gold, 2345678);
  assert.equal(exported.reparsed.header.status.hardcore, true);
  validateSaveEnvelope(exported.bytes);
});

test('calculates live RuneWizard-style bonuses from the effective equipment placement', async () => {
  const document = await createBlankCharacter({ name: 'BonusLive', className: 'Warlock' });
  const editable = editableSnapshot(document.model);
  const charmIndex = document.model.items.findIndex((item) => item.type === 'mff');
  editable.itemPlacements[charmIndex] = {
    ...editable.itemPlacements[charmIndex],
    locationId: 1,
    equippedId: 1,
    x: 0,
    y: 0,
    altPositionId: 0,
  };

  assert.deepEqual(
    itemBonusSummary(
      document.model.items,
      editable.itemEdits,
      editable.addedItems,
      editable.itemPlacements,
      editable.attributes.level,
    ),
    [{
      id: 80,
      name: 'item_magicbonus',
      values: [35],
      description: '35% Better Chance of Getting Magic Items',
      visible: true,
    }],
  );
  editable.itemPlacements[charmIndex] = {
    ...editable.itemPlacements[charmIndex],
    locationId: 0,
    equippedId: 0,
    x: 10,
    y: 0,
    altPositionId: 1,
  };
  assert.deepEqual(
    itemBonusSummary(
      document.model.items,
      editable.itemEdits,
      editable.addedItems,
      editable.itemPlacements,
      editable.attributes.level,
    ),
    [],
  );
});

test('calculates every RuneWizard Item Bonuses column from real equipped BKVince items', async () => {
  const document = await createBlankCharacter({ name: 'BonusGrid', className: 'Amazon' });
  let editable = editableSnapshot(document.model);
  const loadout = [
    ['Harlequin Crest', 1],
    ["Mara's Kaleidoscope", 2],
    ['The Gnasher', 4],
    ['Arachnid Mesh', 8],
    ['Sandstorm Trek', 9],
    ['Chance Guards', 10],
  ];
  loadout.forEach(([name, slotId]) => {
    const entry = availableNamedItems().find((candidate) => candidate.name === name);
    assert.ok(entry, `${name} is present in the governed BKVince catalog`);
    editable = addItemToEquipmentSlotSnapshot(editable, document.model.items, {
      type: entry.baseCode,
      slotId,
      itemLevel: 99,
      quality: entry.quality,
      setId: entry.kind === 'set' ? entry.id : null,
      uniqueId: entry.kind === 'unique' ? entry.id : null,
    });
  });

  const bonuses = itemBonusSummary(
    document.model.items,
    editable.itemEdits,
    editable.addedItems,
    editable.itemPlacements,
    99,
  );
  assert.deepEqual(itemBonusDashboard(bonuses), {
    attributes: {
      strength: 40,
      dexterity: 25,
      energy: 25,
      vitality: 25,
      life: 148,
      mana: 148,
    },
    resistances: {
      fire: 30,
      lightning: 30,
      cold: 30,
      poison: 100,
      magic: 0,
      physical: 10,
    },
    breakpoints: {
      fasterCastRate: 20,
      fasterHitRecovery: 20,
      fasterBlockRate: 15,
      increasedAttackSpeed: 25,
    },
    misc: {
      allSkills: 6,
      magicFind: 90,
      goldFind: 200,
    },
  });
  const equippedItems = editableItems(document.model.items, editable.itemEdits, editable.addedItems);
  const harlequin = equippedItems.find(({ unique_name: uniqueName }) => uniqueName === 'Harlequin Crest');
  assert.ok(harlequin);
  assert.deepEqual(
    describeItem(harlequin, 0, 99).tint,
    { code: 'cgrn', label: 'Cyan Green', color: '#00ff7f' },
  );
  assert.ok(describeItem(harlequin, 0, 1).magicAttributes.includes(
    '+1 to Life (Based on Character Level)',
  ));
  assert.ok(describeItem(harlequin, 0, 99).magicAttributes.includes(
    '+148 to Life (Based on Character Level)',
  ));

  const exported = await exportCharacter(document, editable);
  const reopened = await openCharacter(exported.bytes, 'BonusGrid.d2s');
  const reopenedEditable = editableSnapshot(reopened.model);
  const reopenedBonuses = itemBonusSummary(
    reopened.model.items,
    reopenedEditable.itemEdits,
    reopenedEditable.addedItems,
    reopenedEditable.itemPlacements,
    99,
  );
  assert.deepEqual(itemBonusDashboard(reopenedBonuses), itemBonusDashboard(bonuses));
  const noOp = await exportCharacter(reopened, reopenedEditable);
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, exported.bytes);
});

test('shows the governed secondary line on every character-level item property', () => {
  const levelBasedStats = constants.magical_properties
    .map((definition, id) => ({ definition, id }))
    .filter(({ definition }) => definition?.d2 === '(Based on Character Level)');
  assert.equal(levelBasedStats.length, 37);

  levelBasedStats.forEach(({ definition, id }) => {
    const descriptor = describeItem({
      type: 'cap',
      level: 99,
      quality: 2,
      simple_item: 0,
      identified: 1,
      magic_attributes: [{ id, values: [8], name: definition.s }],
      runeword_attributes: [],
      socketed_items: [],
    }, id, 99);
    assert.equal(descriptor.magicAttributes.length, 1, definition.s);
    assert.ok(
      descriptor.magicAttributes[0].endsWith('(Based on Character Level)'),
      `${definition.s}: ${descriptor.magicAttributes[0]}`,
    );
  });
});

test('builds governed hireling definitions and round-trips native mercenary headers and jf items', async () => {
  const hirelingHeaders = [
    'Hireling', '*SubType', 'Id', 'Class', 'Act', 'Difficulty', 'Level',
    'Exp/Lvl', 'NameFirst', 'NameLast', 'equivalentcharclass',
    ...Array.from({ length: 6 }, (_, index) => `Skill${index + 1}`),
  ];
  const hirelingRows = [
    ['Rogue Scout', 'Fire - Normal', '0', '271', '1', '1', '3', '100', 'merc01', 'merc41', 'ama', 'Inner Sight', 'Fire Arrow'],
    ['Rogue Scout', 'Fire - Normal', '0', '271', '1', '1', '25', '110', 'merc01', 'merc41', 'ama', 'Inner Sight', 'Exploding Arrow'],
  ].map((row) => [...row, ...Array(Math.max(0, hirelingHeaders.length - row.length)).fill('')]);
  assert.deepEqual(buildMercenaryCatalog({ Hireling: table(hirelingHeaders, hirelingRows) }), [{
    id: 0,
    hireling: 'Rogue Scout',
    subtype: 'Fire - Normal',
    classId: 271,
    act: 1,
    difficulty: 1,
    minimumLevel: 3,
    maximumLevel: 25,
    experiencePerLevel: 100,
    startingExperience: 3600,
    nameFirst: 'merc01',
    nameLast: 'merc41',
    equivalentClass: 'ama',
    skills: ['Exploding Arrow', 'Fire Arrow', 'Inner Sight'],
    rows: [2, 3],
  }]);
  assert.equal(mercenaryDefinitions.length, 39);
  assert.ok(mercenaryDefinitions.some(({ id, skills }) => (
    id === 0
    && skills.includes('Exploding Arrow')
    && !skills.includes('BKV Fire Raven')
  )));

  const blank = await createBlankCharacter({ name: 'MercCodec', className: 'Warlock' });
  let playerEdit = addItemBatchSnapshot(editableSnapshot(blank.model), blank.model.items, {
    type: 'rin',
    containerId: 'inventory',
    x: 2,
    y: 0,
    count: 1,
    itemLevel: 41,
  });
  const withRing = await exportCharacter(blank, playerEdit);
  const mercModel = structuredClone(withRing.reparsed);
  const ring = mercModel.items.pop();
  Object.assign(ring, {
    location_id: 1,
    equipped_id: 6,
    position_x: 0,
    position_y: 0,
    alt_position_id: 0,
  });
  mercModel.header.dead_merc = 1;
  mercModel.header.merc_id = '1234abcd';
  mercModel.header.merc_name_id = 1;
  mercModel.header.merc_type = 0;
  mercModel.header.merc_experience = 114097216;
  mercModel.merc_items = [ring];
  const mercDocument = await openCharacter(
    new Uint8Array(await write(mercModel, constants, CODEC_OPTIONS)),
    'MercCodec.d2s',
  );
  let mercEdit = editableSnapshot(mercDocument.model);
  mercEdit = {
    ...mercEdit,
    mercenary: {
      ...mercEdit.mercenary,
      dead: false,
      nameId: 2,
      type: 1,
      experience: 114097217,
    },
  };
  const scoped = {
    ...mercEdit,
    addedItems: mercEdit.mercAddedItems,
    itemEdits: mercEdit.mercItemEdits,
    itemPlacements: mercEdit.mercItemPlacements,
  };
  const editedScope = editItemSnapshot(scoped, mercDocument.model.merc_items, 0, { identified: false });
  mercEdit = {
    ...mercEdit,
    mercAddedItems: editedScope.addedItems,
    mercItemEdits: editedScope.itemEdits,
    mercItemPlacements: editedScope.itemPlacements,
  };

  const exported = await exportCharacter(mercDocument, mercEdit);
  assert.equal(exported.byteExact, false);
  assert.deepEqual(
    pick(exported.reparsed.header, [
      'dead_merc', 'merc_id', 'merc_name_id', 'merc_type', 'merc_experience',
    ]),
    {
      dead_merc: 0,
      merc_id: '1234abcd',
      merc_name_id: 2,
      merc_type: 1,
      merc_experience: 114097217,
    },
  );
  assert.equal(exported.reparsed.merc_items.length, 1);
  assert.equal(exported.reparsed.merc_items[0].type, 'rin');
  assert.equal(exported.reparsed.merc_items[0].equipped_id, 6);
  assert.equal(exported.reparsed.merc_items[0].identified, 0);
  const reopened = await openCharacter(exported.bytes, 'MercCodecEdited.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, exported.bytes);
});

test('creates and removes an absent mercenary with governed BKVince defaults', async () => {
  const document = await createBlankCharacter({ name: 'MercCreate', className: 'Warlock' });
  const initial = editableSnapshot(document.model);
  assert.equal(initial.mercenary.present, false);

  const created = createMercenarySnapshot(initial);
  assert.deepEqual(
    pick(created.mercenary, ['present', 'dead', 'nameId', 'type', 'experience']),
    { present: true, dead: false, nameId: 0, type: 0, experience: 3600 },
  );
  assert.match(created.mercenary.id, /^[0-9a-f]{1,8}$/);
  assert.notEqual(created.mercenary.id, '0');
  assert.equal(createMercenarySnapshot(initial).mercenary.id, created.mercenary.id);
  assert.throws(() => createMercenarySnapshot(created), /already contains/i);
  assert.throws(() => createMercenarySnapshot(initial, { id: '0' }), /nonzero hexadecimal/i);

  const createdExport = await exportCharacter(document, created);
  assert.equal(createdExport.reparsed.header.merc_type, 0);
  assert.equal(createdExport.reparsed.header.merc_name_id, 0);
  assert.equal(createdExport.reparsed.header.merc_experience, 3600);
  assert.equal(createdExport.reparsed.header.merc_id, created.mercenary.id);
  assert.deepEqual(createdExport.reparsed.merc_items, []);

  const createdDocument = await openCharacter(createdExport.bytes, 'MercCreate.d2s');
  const removed = removeMercenarySnapshot(editableSnapshot(createdDocument.model));
  assert.deepEqual(removed.mercenary, {
    present: false,
    dead: false,
    id: '0',
    nameId: 0,
    type: 0,
    experience: 0,
  });
  const removedExport = await exportCharacter(createdDocument, removed);
  assert.equal(removedExport.reparsed.header.merc_id, '0');
  assert.deepEqual(removedExport.reparsed.merc_items, []);
  const reopened = await openCharacter(removedExport.bytes, 'MercCreateRemoved.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, removedExport.bytes);
});

test('adds, imports, and moves player equipment by governed BKVince body location', async () => {
  const document = await createBlankCharacter({ name: 'PlayerForge', className: 'Warlock' });
  let editable = editableSnapshot(document.model);
  const reservedItemIds = characterItemIds(document.model);

  editable = addItemBatchSnapshot(editable, document.model.items, {
    type: 'rin',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 41,
    reservedItemIds,
  });
  const ringIndex = editable.itemPlacements.length - 1;
  editable = {
    ...editable,
    itemPlacements: moveItemToEquipmentSlot(
      editable.itemPlacements,
      document.model.items,
      ringIndex,
      6,
      editable.itemEdits,
      editable.addedItems,
    ),
  };
  editable = addItemToEquipmentSlotSnapshot(editable, document.model.items, {
    type: 'cap',
    slotId: 1,
    itemLevel: 41,
    reservedItemIds,
  });

  const portableAmulet = addItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
    type: 'amu',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 41,
  }).addedItems[0];
  editable = addImportedItemToEquipmentSlotSnapshot(editable, document.model.items, {
    importedItems: [portableAmulet],
    slotId: 2,
    reservedItemIds,
  });

  assert.throws(
    () => addItemToEquipmentSlotSnapshot(editable, document.model.items, {
      type: 'rin', slotId: 3, itemLevel: 41,
    }),
    /cannot be equipped in Torso/i,
  );
  assert.throws(
    () => moveItemToEquipmentSlot(
      editable.itemPlacements,
      document.model.items,
      ringIndex,
      1,
      editable.itemEdits,
      editable.addedItems,
    ),
    /cannot be equipped in Head/i,
  );

  const exported = await exportCharacter(document, editable);
  assert.deepEqual(
    exported.reparsed.items
      .filter(({ location_id: locationId, equipped_id: equippedId }) => (
        locationId === 1 && equippedId >= 1 && equippedId <= 12
      ))
      .map(({ type, equipped_id: equippedId }) => ({ type, equippedId }))
      .sort((left, right) => left.equippedId - right.equippedId),
    [
      { type: 'cap', equippedId: 1 },
      { type: 'amu', equippedId: 2 },
      { type: 'rin', equippedId: 6 },
    ],
  );
  assert.equal(new Set(characterItemIds(exported.reparsed)).size, characterItemIds(exported.reparsed).length);

  const reopened = await openCharacter(exported.bytes, 'PlayerForge.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, exported.bytes);
});

test('adds, imports, and moves native belt items within governed BKVince capacity', async () => {
  const document = await createBlankCharacter({ name: 'BeltForge', className: 'Amazon' });
  let editable = editableSnapshot(document.model);
  const capacity = () => beltCapacityForPlacements(
    editable.itemPlacements,
    editableItems(document.model.items, editable.itemEdits, editable.addedItems),
  );

  assert.equal(capacity(), 4);
  editable = addItemBatchSnapshot(editable, document.model.items, {
    type: 'hp1',
    containerId: 'belt',
    x: 0,
    y: 0,
    count: 4,
    itemLevel: 1,
  });
  assert.deepEqual(
    editable.itemPlacements.filter((placement) => containerForPlacement(placement) === 'belt')
      .map(({ x }) => x),
    [0, 1, 2, 3],
  );
  assert.throws(
    () => addItemBatchSnapshot(editable, document.model.items, {
      type: 'mp1', containerId: 'belt', x: 4, y: 0, count: 1, itemLevel: 1,
    }),
    /exceeds the current 4-slot capacity/i,
  );
  assert.throws(
    () => moveItemPlacement(
      editable.itemPlacements,
      document.model.items,
      0,
      'belt',
      0,
      0,
      editable.itemEdits,
      editable.addedItems,
    ),
    /cannot be stored in a native BKVince belt slot/i,
  );

  editable = addItemToEquipmentSlotSnapshot(editable, document.model.items, {
    type: 'lbl',
    slotId: 8,
    itemLevel: 1,
  });
  assert.equal(capacity(), 8);

  const portablePotions = addItemBatchSnapshot(
    editableSnapshot(document.model),
    document.model.items,
    { type: 'mp1', containerId: 'inventory', x: 2, y: 0, count: 2, itemLevel: 1 },
  ).addedItems;
  editable = addImportedItemsSnapshot(editable, document.model.items, {
    importedItems: portablePotions,
    containerId: 'belt',
    x: 4,
    y: 0,
  });
  assert.deepEqual(
    editable.itemPlacements.filter((placement) => containerForPlacement(placement) === 'belt')
      .map(({ x }) => x),
    [0, 1, 2, 3, 4, 5],
  );

  const firstPotionIndex = document.model.items.length;
  editable = {
    ...editable,
    itemPlacements: moveItemPlacement(
      editable.itemPlacements,
      document.model.items,
      firstPotionIndex,
      'belt',
      6,
      0,
      editable.itemEdits,
      editable.addedItems,
    ),
  };
  editable = addItemBatchSnapshot(editable, document.model.items, {
    type: 'tsc', containerId: 'belt', x: 7, y: 0, count: 1, itemLevel: 1,
  });
  assert.throws(
    () => addItemBatchSnapshot(editable, document.model.items, {
      type: 'isc', containerId: 'belt', x: 8, y: 0, count: 1, itemLevel: 1,
    }),
    /exceeds the current 8-slot capacity/i,
  );

  const equippedBeltIndex = editable.itemPlacements.findIndex((placement) => (
    placement.locationId === 1 && placement.equippedId === 8
  ));
  assert.throws(
    () => removeItemSnapshot(editable, document.model.items, equippedBeltIndex),
    /exceeds the current 4-slot capacity/i,
  );

  const exported = await exportCharacter(document, editable);
  assert.deepEqual(
    exported.reparsed.items
      .filter(({ location_id: locationId }) => locationId === 2)
      .map(({ type, position_x: slot, position_y: row }) => ({ type, slot, row }))
      .sort((left, right) => left.slot - right.slot),
    [
      { type: 'hp1', slot: 1, row: 0 },
      { type: 'hp1', slot: 2, row: 0 },
      { type: 'hp1', slot: 3, row: 0 },
      { type: 'mp1', slot: 4, row: 0 },
      { type: 'mp1', slot: 5, row: 0 },
      { type: 'hp1', slot: 6, row: 0 },
      { type: 'tsc', slot: 7, row: 0 },
    ],
  );
  assert.equal(new Set(characterItemIds(exported.reparsed)).size, characterItemIds(exported.reparsed).length);

  const reopened = await openCharacter(exported.bytes, 'BeltForge.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, exported.bytes);
});

test('adds, imports, and moves equipment in the native mercenary jf block by governed body location', async () => {
  const document = await createBlankCharacter({ name: 'MercForge', className: 'Warlock' });
  let editable = editableSnapshot(document.model);
  editable = {
    ...editable,
    mercenary: {
      present: true,
      dead: false,
      id: '10203040',
      nameId: 1,
      type: 0,
      experience: 114097216,
    },
  };
  let scoped = {
    ...editable,
    addedItems: editable.mercAddedItems,
    itemEdits: editable.mercItemEdits,
    itemPlacements: editable.mercItemPlacements,
  };
  const reservedItemIds = characterItemIds(document.model);

  assert.ok(availableEquipmentItemBases(1).some(({ code }) => code === 'cap'));
  assert.ok(!availableEquipmentItemBases(1).some(({ code }) => code === 'rin'));
  assert.ok(availableEquipmentItemBases(6).some(({ code }) => code === 'rin'));
  scoped = addItemToEquipmentSlotSnapshot(scoped, document.model.merc_items, {
    type: 'cap',
    slotId: 1,
    itemLevel: 41,
    reservedItemIds,
  });
  scoped = addItemToEquipmentSlotSnapshot(scoped, document.model.merc_items, {
    type: 'rin',
    slotId: 6,
    itemLevel: 41,
    reservedItemIds,
  });
  assert.throws(
    () => addItemToEquipmentSlotSnapshot(scoped, document.model.merc_items, {
      type: 'cap', slotId: 1, itemLevel: 41,
    }),
    /already occupied/i,
  );
  assert.throws(
    () => addItemToEquipmentSlotSnapshot(scoped, document.model.merc_items, {
      type: 'rin', slotId: 3, itemLevel: 41,
    }),
    /cannot be equipped in Torso/i,
  );

  scoped = {
    ...scoped,
    itemPlacements: moveItemToEquipmentSlot(
      scoped.itemPlacements,
      document.model.merc_items,
      1,
      7,
      scoped.itemEdits,
      scoped.addedItems,
    ),
  };
  const playerSource = addItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
    type: 'amu',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 41,
  }).addedItems[0];
  scoped = addImportedItemToEquipmentSlotSnapshot(scoped, document.model.merc_items, {
    importedItems: [playerSource],
    slotId: 2,
    reservedItemIds,
  });
  assert.throws(
    () => addImportedItemToEquipmentSlotSnapshot(scoped, document.model.merc_items, {
      importedItems: [playerSource, playerSource], slotId: 3,
    }),
    /exactly one imported item/i,
  );

  editable = {
    ...editable,
    mercAddedItems: scoped.addedItems,
    mercItemEdits: scoped.itemEdits,
    mercItemPlacements: scoped.itemPlacements,
  };
  const exported = await exportCharacter(document, editable);
  assert.deepEqual(
    exported.reparsed.merc_items.map(({ type, location_id: locationId, equipped_id: equippedId }) => ({
      type, locationId, equippedId,
    })),
    [
      { type: 'cap', locationId: 1, equippedId: 1 },
      { type: 'rin', locationId: 1, equippedId: 7 },
      { type: 'amu', locationId: 1, equippedId: 2 },
    ],
  );
  assert.equal(new Set(characterItemIds(exported.reparsed)).size, characterItemIds(exported.reparsed).length);
  const reopened = await openCharacter(exported.bytes, 'MercForge.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, exported.bytes);

  let removal = editableSnapshot(reopened.model);
  let removalScope = {
    ...removal,
    addedItems: removal.mercAddedItems,
    itemEdits: removal.mercItemEdits,
    itemPlacements: removal.mercItemPlacements,
  };
  removalScope = removeItemSnapshot(removalScope, reopened.model.merc_items, 1);
  assert.equal(containerForPlacement(removalScope.itemPlacements[1]), 'removed');
  removal = {
    ...removal,
    mercAddedItems: removalScope.addedItems,
    mercItemEdits: removalScope.itemEdits,
    mercItemPlacements: removalScope.itemPlacements,
  };
  const removed = await exportCharacter(reopened, removal);
  assert.deepEqual(
    removed.reparsed.merc_items.map(({ type, equipped_id: equippedId }) => ({ type, equippedId })),
    [
      { type: 'cap', equippedId: 1 },
      { type: 'amu', equippedId: 2 },
    ],
  );
  const removedReopened = await openCharacter(removed.bytes, 'MercForgeRemoved.d2s');
  const removedNoOp = await exportCharacter(removedReopened, editableSnapshot(removedReopened.model));
  assert.equal(removedNoOp.byteExact, true);
  assert.deepEqual(removedNoOp.bytes, removed.bytes);

  const withoutMercenary = removeMercenarySnapshot(editableSnapshot(removedReopened.model));
  assert.ok(withoutMercenary.mercItemPlacements.every(({ removed: isRemoved }) => isRemoved));
  const withoutMercenaryExport = await exportCharacter(removedReopened, withoutMercenary);
  assert.equal(withoutMercenaryExport.reparsed.header.merc_id, '0');
  assert.equal(withoutMercenaryExport.reparsed.header.merc_experience, 0);
  assert.deepEqual(withoutMercenaryExport.reparsed.merc_items, []);
  const withoutMercenaryReopened = await openCharacter(
    withoutMercenaryExport.bytes,
    'MercForgeWithoutMercenary.d2s',
  );
  const withoutMercenaryNoOp = await exportCharacter(
    withoutMercenaryReopened,
    editableSnapshot(withoutMercenaryReopened.model),
  );
  assert.equal(withoutMercenaryNoOp.byteExact, true);
  assert.deepEqual(withoutMercenaryNoOp.bytes, withoutMercenaryExport.bytes);
});

test('edits governed quests, waypoints, and skill ranks and reparses their exact D2S fields', async () => {
  const document = await createBlankCharacter({ name: 'WorldState', className: 'Warlock' });
  let editable = editableSnapshot(document.model);
  editable.attributes.level = 30;
  editable.attributes.unused_skill_points = 12;
  editable = setQuestCompletionSnapshot(editable, 'normal', 'act_i', 'den_of_evil', true);
  editable = setQuestCompletionSnapshot(editable, 'normal', 'act_v', 'prison_of_ice', true);
  editable = setQuestConsumedScrollSnapshot(editable, 'normal', true);
  editable = setWaypointSnapshot(editable, 'normal', 'act_i', 'cold_plains', true);
  editable = setSkillPointsSnapshot(editable, 373, 7);

  const exported = await exportCharacter(document, editable);
  assert.equal(exported.byteExact, false);
  assert.equal(exported.reparsed.header.quests_normal.act_i.den_of_evil.is_completed, true);
  assert.equal(exported.reparsed.header.quests_normal.act_i.den_of_evil.is_requirement_completed, true);
  assert.equal(exported.reparsed.header.quests_normal.act_i.den_of_evil.is_received, true);
  assert.equal(exported.reparsed.header.quests_normal.act_i.den_of_evil.closed, true);
  assert.equal(exported.reparsed.header.quests_normal.act_v.introduced, true);
  assert.equal(exported.reparsed.header.quests_normal.act_v.prison_of_ice.is_completed, true);
  assert.equal(exported.reparsed.header.quests_normal.act_v.prison_of_ice.consumed_scroll, true);
  assert.equal(exported.reparsed.header.waypoints.normal.act_i.cold_plains, true);
  assert.equal(exported.reparsed.skills.find(({ id }) => id === 373).points, 7);
  assert.equal(exported.reparsed.skills.find(({ id }) => id === 374).points, 0);
  validateSaveEnvelope(exported.bytes);

  const reopened = await openCharacter(exported.bytes, 'WorldState.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, exported.bytes);
});

test('completes quest rewards with RuneWizard-compatible consumed scroll state', async () => {
  const document = await createBlankCharacter({ name: 'QuestRewards', className: 'Warlock' });
  const completed = setAllQuestsSnapshot(editableSnapshot(document.model), true);

  for (const difficulty of ['normal', 'nm', 'hell']) {
    assert.equal(completed.quests[difficulty].act_v.prison_of_ice.is_completed, true);
    assert.equal(completed.quests[difficulty].act_v.prison_of_ice.consumed_scroll, true);
  }
});

test('unlocks and resets every governed waypoint atomically', async () => {
  const document = await createBlankCharacter({ name: 'WaypointWorld', className: 'Warlock' });
  const editable = editableSnapshot(document.model);
  assert.equal(
    ['normal', 'nm', 'hell']
      .filter((difficulty) => editable.waypoints[difficulty].act_i.rogue_encampement)
      .length,
    3,
  );
  assert.equal(
    ['normal', 'nm', 'hell']
      .flatMap((difficulty) => Object.values(editable.waypoints[difficulty]))
      .flatMap((act) => Object.values(act))
      .filter(Boolean)
      .length,
    3,
  );
  const unlocked = setAllWaypointsSnapshot(editable, true);
  const reset = setAllWaypointsSnapshot(unlocked, false);

  for (const difficulty of ['normal', 'nm', 'hell']) {
    for (const act of Object.values(unlocked.waypoints[difficulty])) {
      assert.ok(Object.values(act).every(Boolean));
    }
    for (const act of Object.values(reset.waypoints[difficulty])) {
      assert.ok(Object.values(act).every((active) => !active));
    }
  }
});

test('resets the complete skill window atomically and refunds stored skill points', async () => {
  const document = await createBlankCharacter({ name: 'SkillReset', className: 'Warlock' });
  let editable = editableSnapshot(document.model);
  editable.attributes.unused_skill_points = 7;
  editable = setSkillPointsSnapshot(editable, 373, 3);
  editable = setSkillPointsSnapshot(editable, 374, 2);

  const reset = resetSkillsSnapshot(editable);
  assert.equal(reset.attributes.unused_skill_points, 12);
  assert.ok(reset.skills.every(({ points }) => points === 0));
  assert.strictEqual(resetSkillsSnapshot(reset), reset);

  const exported = await exportCharacter(document, reset);
  assert.equal(exported.reparsed.attributes.unused_skill_points, 12);
  assert.ok(exported.reparsed.skills.every(({ points }) => points === 0));
  validateSaveEnvelope(exported.bytes);

  const reopened = await openCharacter(exported.bytes, 'SkillReset.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, exported.bytes);

  const rawReset = resetSkillsSnapshot(editable, { refund: false });
  assert.equal(rawReset.attributes.unused_skill_points, 7);
  assert.ok(rawReset.skills.every(({ points }) => points === 0));
  assert.throws(
    () => resetSkillsSnapshot({
      ...editable,
      attributes: { ...editable.attributes, unused_skill_points: 255 },
    }),
    /overflow unused skill points/i,
  );
});

test('unlocks Hell by completing Normal and Nightmare without changing Hell quests', async () => {
  const document = await createBlankCharacter({ name: 'UnlockHell', className: 'Warlock' });
  const editable = unlockHellSnapshot(editableSnapshot(document.model));
  assert.equal(editable.quests.normal.act_v.eve_of_destruction.is_completed, true);
  assert.equal(editable.quests.nm.act_v.eve_of_destruction.is_completed, true);
  assert.equal(editable.quests.hell.act_i.den_of_evil.is_completed, false);

  const exported = await exportCharacter(document, editable);
  assert.equal(exported.reparsed.header.quests_normal.act_v.eve_of_destruction.is_completed, true);
  assert.equal(exported.reparsed.header.quests_nm.act_v.eve_of_destruction.is_completed, true);
  assert.equal(exported.reparsed.header.quests_hell.act_i.den_of_evil.is_completed, false);
  validateSaveEnvelope(exported.bytes);
});

test('builds every BKVince class skill tree from governed SkillDesc coordinates', () => {
  assert.equal(skillCatalog.length, 240);
  for (const { name: className } of supportedClasses()) {
    const definition = skillEditorDefinition(className);
    assert.equal(definition.tabs.length, 3, `${className} tab count`);
    assert.equal(definition.skills.length, 30, `${className} skill count`);
    for (const tab of definition.tabs) {
      const page = definition.skills.filter((skill) => skill.page === tab.id);
      assert.equal(page.length, 10, `${className} ${tab.label} skill count`);
      assert.equal(
        new Set(page.map((skill) => `${skill.row}/${skill.column}`)).size,
        10,
        `${className} ${tab.label} unique native cells`,
      );
    }
  }
  const warlock = skillEditorDefinition('Warlock');
  assert.deepEqual(warlock.tabs.map(({ label }) => label), ['Demon Skills', 'Eldritch Skills', 'Chaos Skills']);
  assert.equal(warlock.skills.length, 30);
  assert.deepEqual(
    pick(warlock.skills.find(({ id }) => id === 373), ['name', 'page', 'row', 'column', 'requiredLevel', 'maxLevel', 'prerequisites']),
    {
      name: 'Summon Goatman',
      page: 1,
      row: 1,
      column: 3,
      requiredLevel: 1,
      maxLevel: 20,
      prerequisites: [],
    },
  );
  assert.deepEqual(warlock.skills.find(({ id }) => id === 402).prerequisites, [397]);
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

test('preserves v105 realm data on simple items resaved by D2R', async () => {
  const document = await openCharacter(D2R_RESAVED_ITEM_FIXTURE, 'HEItemMove.d2s');
  assert.equal(document.model.items.length, 9);
  assert.ok(document.model.items.every((item) => (
    item.timestamp === 1
    && item._unknown_data.realm_data.length === 4
  )));

  const editable = editableSnapshot(document.model);
  editable.name = 'HERealmTest';
  const exported = await exportCharacter(document, editable);
  assert.equal(exported.byteExact, false);
  assert.equal(exported.reparsed.items.length, 9);
  assert.ok(exported.reparsed.items.every((item) => (
    item.timestamp === 1
    && item._unknown_data.realm_data.length === 4
  )));
  const scroll = exported.reparsed.items.find((item) => item.type === 'tsc');
  assert.equal(scroll.alt_position_id, 4);
  assert.equal(scroll.position_x, 0);
  assert.equal(scroll.position_y, 0);
  assert.equal(exported.reparsed.items.find((item) => item.type === 'cm1').magic_attributes.length, 10);
});

test('generates a deterministic fail-closed catalog from governed BKVince tables', () => {
  assert.equal(itemCatalog.bases.length, 800);
  assert.deepEqual(
    itemCatalog.bases.map(({ code }) => code),
    [...itemCatalog.bases.map(({ code }) => code)].sort((left, right) => left.localeCompare(right)),
  );
  assert.equal(new Set(itemCatalog.bases.map(({ code }) => code)).size, itemCatalog.bases.length);
  assert.deepEqual(
    pick(itemCatalog.bases.find(({ code }) => code === 'mff'), ['source', 'typeId', 'compactSave', 'itemType']),
    { source: 'Misc', typeId: 3, compactSave: false, itemType: 'chms' },
  );
  assert.deepEqual(
    pick(itemCatalog.properties.find(({ key }) => key === 'str'), ['supported']),
    { supported: true },
  );
  assert.deepEqual(
    itemCatalog.properties.find(({ key }) => key === 'dmg%').functions[0].outputs
      .map((output) => pick(output, ['statId', 'groupId', 'valueIndex', 'valueSource'])),
    [
      { statId: 17, groupId: 17, valueIndex: 0, valueSource: 'roll' },
      { statId: 18, groupId: 17, valueIndex: 1, valueSource: 'roll' },
    ],
  );
  assert.deepEqual(
    pick(itemCatalog.properties.find(({ key }) => key === 'oskill'), ['supported']),
    { supported: true },
  );
  assert.deepEqual(
    itemCatalog.properties.find(({ key }) => key === 'oskill').functions[0].outputs
      .map((output) => pick(output, ['statId', 'groupId', 'valueCount', 'encoding'])),
    [{ statId: 97, groupId: 97, valueCount: 2, encoding: 'stat-parameter' }],
  );
  assert.equal(itemCatalog.properties.find(({ key }) => key === 'fireskill').supported, true);
  assert.equal(itemCatalog.properties.find(({ key }) => key === 'charged').supported, true);
  assert.equal(itemCatalog.properties.find(({ key }) => key === 'sock').supported, true);
  assert.equal(itemCatalog.bases.find(({ code }) => code === 'gth').maxSockets, 4);
  assert.deepEqual(
    pick(itemCatalog.bases.find(({ code }) => code === 'hgl'), [
      'normalCode', 'exceptionalCode', 'eliteCode', 'defenseMinimum', 'defenseMaximum',
    ]),
    {
      normalCode: 'hgl',
      exceptionalCode: 'xhg',
      eliteCode: 'uhg',
      defenseMinimum: 12,
      defenseMaximum: 15,
    },
  );
  assert.deepEqual(itemCatalog.bases.find(({ code }) => code === 'cap').bodyLocations, ['head']);
  assert.deepEqual(itemCatalog.bases.find(({ code }) => code === 'rin').bodyLocations, ['rrin', 'lrin']);
  assert.equal(itemCatalog.bases.find(({ code }) => code === 'hp1').beltable, true);
  assert.equal(describeItem({ type: 'hp1' }, 0).name, 'Minor Healing Potion');
  assert.equal(itemCatalog.bases.find(({ code }) => code === 'tsc').beltable, true);
  assert.equal(itemCatalog.bases.find(({ code }) => code === 'cap').beltable, false);
  assert.equal(itemCatalog.bases.find(({ code }) => code === 'lbl').beltLayout, 1);
  assert.equal(itemCatalog.bases.find(({ code }) => code === 'hbl').beltLayout, 3);
  assert.equal(availableBeltItemBases().length, 19);
  assert.ok(availableBeltItemBases().every(({ beltable, width, height }) => (
    beltable && width === 1 && height === 1
  )));
  assert.equal(itemCatalog.uniqueItems.find(({ id }) => id === 381).mods.length, 4);
  assert.equal(itemCatalog.setItems.find(({ id }) => id === 115).mods.length, 7);
  assert.equal(itemCatalog.rareNamePrefixes.length, 46);
  assert.equal(itemCatalog.rareNameSuffixes.length, 155);
  assert.deepEqual(
    itemCatalog.lowQualityNames.map(({ id, name }) => ({ id, name })),
    [
      { id: 0, name: 'Crude' },
      { id: 1, name: 'Cracked' },
      { id: 2, name: 'Damaged' },
      { id: 3, name: 'Low Quality' },
    ],
  );
  assert.deepEqual(
    pick(itemCatalog.rareNamePrefixes[0], ['id', 'name']),
    { id: 156, name: 'Beast' },
  );
  assert.deepEqual(
    pick(itemCatalog.rareNameSuffixes[0], ['id', 'name']),
    { id: 1, name: 'Bite' },
  );
  assert.equal(itemCatalog.prefixes.filter(({ rare }) => rare).length, 663);
  assert.equal(itemCatalog.suffixes.filter(({ rare }) => rare).length, 704);
  assert.equal(itemCatalog.runewords.length, 112);
  assert.deepEqual(
    pick(itemCatalog.runewords.find(({ id }) => id === 155), ['name', 'runes', 'allowedTypes']),
    { name: 'Spirit', runes: ['r07', 'r10', 'r09', 'r11'], allowedTypes: ['shld', 'swor'] },
  );

  const fixture = catalogFixture();
  const built = buildItemCatalog(fixture.tables, fixture.constants);
  assert.deepEqual(built.bases.map(({ code }) => code), ['a01', 'm01', 'w01']);
  assert.deepEqual(
    built.bases.map(({ code, bodyLocations }) => ({ code, bodyLocations })),
    [
      { code: 'a01', bodyLocations: ['tors'] },
      { code: 'm01', bodyLocations: [] },
      { code: 'w01', bodyLocations: ['rarm', 'larm'] },
    ],
  );
  assert.throws(
    () => buildItemCatalog({ ...fixture.tables, Armor: table(['name', 'code'], [['Armor', 'a01']]) }, fixture.constants),
    /missing required headers/i,
  );
  assert.throws(
    () => buildItemCatalog({ ...fixture.tables, Misc: baseTable('a01', 'Misc') }, fixture.constants),
    /ambiguous BKVince item code a01/i,
  );
  const badUnique = structuredClone(fixture.tables);
  badUnique.UniqueItems.rows[0][badUnique.UniqueItems.headers.indexOf('code')] = 'nope';
  assert.throws(() => buildItemCatalog(badUnique, fixture.constants), /references unknown BKVince item code nope/i);
});

test('generates governed BKVince Bound Demon catalogs with native row indexes', () => {
  assert.equal(demonCatalog.monsters.length, 799);
  assert.equal(demonCatalog.superUniques.length, 70);
  assert.equal(demonCatalog.modifiers.length, 45);
  assert.deepEqual(
    pick(demonCatalog.monsters[0], ['index', 'row', 'id', 'name']),
    { index: 1, row: 2, id: 'skeleton1', name: 'Skeleton' },
  );
  assert.deepEqual(
    pick(demonCatalog.superUniques[0], ['index', 'row', 'id', 'name']),
    { index: 1, row: 2, id: 'Bishibosh', name: 'Bishibosh' },
  );
  assert.deepEqual(
    pick(demonCatalog.modifiers.find(({ id }) => id === 17), ['internalName', 'label', 'enabled']),
    { internalName: 'lightning', label: 'Lightning Enchanted', enabled: true },
  );
  assert.equal(demonDefinitions.monsters.length, demonCatalog.monsters.length);

  const fixture = demonCatalogFixture();
  const built = buildDemonCatalog(fixture.tables, fixture.buffers);
  assert.deepEqual(built.monsters.map(({ index, id, name }) => ({ index, id, name })), [
    { index: 1, id: 'testdemon', name: 'Localized Demon' },
  ]);
  assert.deepEqual(built.superUniques.map(({ index, id, name }) => ({ index, id, name })), [
    { index: 1, id: 'TestUnique', name: 'Localized Unique' },
  ]);
  assert.equal(built.modifiers[0].label, 'Extra Strong');
  assert.throws(
    () => buildDemonCatalog({ ...fixture.tables, MonStats: table([], []) }, fixture.buffers),
    /missing required headers/i,
  );
});

test('edits only the exposed Bound Demon fields and preserves its opaque native payload', async () => {
  const blank = await createBlankCharacter({ name: 'DemonCodec', className: 'Warlock' });
  const model = structuredClone(blank.model);
  model.demon = nativeDemonFixture();
  const bytes = Buffer.from(await write(model, constants, CODEC_OPTIONS));
  const document = await openCharacter(bytes, 'DemonCodec.d2s');
  const initial = editableSnapshot(document.model);
  assert.deepEqual(initial.demon, {
    present: true,
    isDesecrated: false,
    isSuperUnique: false,
    index: 1,
    mods: [5, 6, 9, 17, 18, 25, 0, 0, 0],
  });

  const noOp = await exportCharacter(document, initial);
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(Buffer.from(noOp.bytes), bytes);

  const edited = structuredClone(initial);
  edited.demon.isDesecrated = true;
  edited.demon.isSuperUnique = true;
  edited.demon.index = 2;
  edited.demon.mods[1] = 27;
  const exported = await exportCharacter(document, edited);
  assert.equal(exported.byteExact, false);
  assert.equal(exported.reparsed.demon.isDesecrated, 1);
  assert.equal(exported.reparsed.demon.isSuperUnique, 1);
  assert.equal(exported.reparsed.demon.index, 2);
  assert.equal(exported.reparsed.demon.mods[1], 27);
  assert.deepEqual(exported.reparsed.demon._unknown_data, document.model.demon._unknown_data);
  assert.deepEqual(exported.reparsed.demon.stats, document.model.demon.stats);
  assert.deepEqual(exported.reparsed.demon.mods.slice(6), [0, 0, 0]);
});

test('does not fabricate a Bound Demon block for a hero that has none', async () => {
  const document = await createBlankCharacter({ name: 'NoDemon', className: 'Warlock' });
  const editable = editableSnapshot(document.model);
  assert.deepEqual(editable.demon, { present: false });
  editable.demon = { present: false, index: 1 };
  await assert.rejects(() => exportCharacter(document, editable), /cannot fabricate Demon fields/i);
});

test('edits only codec-compatible item fields and reparses the result', async () => {
  const document = await openCharacter(REAL_BKVINCE_ITEM_FIXTURE, 'DummyTester-Annihilus.d2s');
  let editable = editableSnapshot(document.model);
  editable = editItemSnapshot(editable, document.model.items, 0, { type: 'hp2' });
  editable = editItemSnapshot(editable, document.model.items, 6, {
    quality: 3,
    identified: false,
    ethereal: true,
  });

  const exported = await exportCharacter(document, editable);
  assert.equal(exported.reparsed.items[0].type, 'hp2');
  assert.equal(exported.reparsed.items[0].simple_item, 1);
  assert.equal(exported.reparsed.items[6].type, 'hax');
  assert.equal(exported.reparsed.items[6].quality, 3);
  assert.equal(exported.reparsed.items[6].file_index, 0);
  assert.equal(exported.reparsed.items[6].identified, 0);
  assert.equal(exported.reparsed.items[6].ethereal, 1);
  assert.deepEqual(itemPayload(document.model.items[1]), itemPayload(exported.reparsed.items[1]));
  validateSaveEnvelope(exported.bytes);
});

test('refuses structurally incompatible item conversions and permits governed named downgrades', async () => {
  const document = await openCharacter(REAL_BKVINCE_ITEM_FIXTURE, 'DummyTester-Annihilus.d2s');
  const editable = editableSnapshot(document.model);
  assert.throws(
    () => editItemSnapshot(editable, document.model.items, 0, { type: 'tsc' }),
    /not structurally compatible/i,
  );
  const unidentified = editItemSnapshot(editable, document.model.items, 8, { identified: false });
  assert.equal(unidentified.itemEdits[8].identified, false);
  const downgraded = editItemSnapshot(editable, document.model.items, 8, { quality: 2 });
  assert.equal(downgraded.itemEdits[8].quality, 2);
  assert.equal(downgraded.itemEdits[8].uniqueId, null);
  assert.deepEqual(downgraded.itemEdits[8].magicAttributes, []);
  const created = await createBlankCharacter({ name: 'SafeUnique', className: 'Amazon' });
  const createdEditable = editableSnapshot(created.model);
  assert.equal(itemEditorOptions(created.model.items[0], createdEditable.itemEdits[0]).etherealEnabled, false);
  assert.throws(
    () => editItemSnapshot(createdEditable, created.model.items, 0, { type: 'mfc' }),
    /not structurally compatible/i,
  );
});

test('adds simple and Normal complex BKVince items and reparses every new record', async () => {
  const document = await createBlankCharacter({ name: 'ForgeItems', className: 'Amazon' });
  const starterItemCount = document.model.items.length;
  let editable = editableSnapshot(document.model);
  editable = addItemBatchSnapshot(editable, document.model.items, {
    type: 'hp1',
    containerId: 'cube',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 42,
  });
  editable = addItemBatchSnapshot(editable, document.model.items, {
    type: 'hax',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 2,
    itemLevel: 42,
  });

  assert.equal(editable.addedItems.length, 3);
  const exported = await exportCharacter(document, editable);
  assert.equal(exported.reparsed.items.length, starterItemCount + 3);
  const potion = exported.reparsed.items[starterItemCount];
  assert.equal(potion.type, 'hp1');
  assert.equal(potion.simple_item, 1);
  assert.equal(potion.alt_position_id, itemContainers.cube.altPositionId);
  assert.deepEqual([potion.position_x, potion.position_y], [0, 0]);
  assert.equal(potion.timestamp, 1);
  assert.equal(potion._unknown_data.realm_data.length, 4);

  const axes = exported.reparsed.items.slice(starterItemCount + 1);
  assert.deepEqual(axes.map(({ type }) => type), ['hax', 'hax']);
  assert.deepEqual(axes.map(({ position_x, position_y }) => [position_x, position_y]), [[0, 0], [1, 0]]);
  assert.ok(axes.every((item) => (
    item.simple_item === 0
    && item.quality === 2
    && item.level === 42
    && item.max_durability === 28
    && item.current_durability === 28
    && item.magic_attributes.length === 0
    && item.timestamp === 1
    && item._unknown_data.realm_data.length === 4
  )));
  assert.notEqual(axes[0].id, axes[1].id);
  validateSaveEnvelope(exported.bytes);
});

test('maintains an editor-only Virtual Stash and exports its live items as a portable bundle', async () => {
  const emptyVirtualStash = {
    addedItems: [],
    itemEdits: [],
    itemPlacements: [],
  };
  let virtualStash = addItemBatchSnapshot(emptyVirtualStash, [], {
    type: 'rin',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 2,
    itemLevel: 67,
  });

  assert.equal(virtualStash.addedItems.length, 2);
  assert.deepEqual(
    virtualStash.itemPlacements.map(({ x, y, removed }) => [x, y, Boolean(removed)]),
    [[0, 0, false], [1, 0, false]],
  );

  virtualStash = editItemSnapshot(virtualStash, [], 0, { identified: false });
  virtualStash = {
    ...virtualStash,
    itemPlacements: moveItemPlacement(
      virtualStash.itemPlacements,
      [],
      1,
      'stash',
      5,
      4,
      virtualStash.itemEdits,
      virtualStash.addedItems,
    ),
  };
  virtualStash = removeItemSnapshot(virtualStash, [], 0);

  const liveItems = activePlacedItems(
    [],
    virtualStash.itemEdits,
    virtualStash.addedItems,
    virtualStash.itemPlacements,
  );
  assert.equal(liveItems.length, 1);
  assert.deepEqual(
    [liveItems[0].position_x, liveItems[0].position_y, liveItems[0].alt_position_id],
    [5, 4, itemContainers.stash.altPositionId],
  );
  assert.deepEqual(
    virtualStash.itemPlacements.slice(1).map(({ x, y, removed }) => [x, y, Boolean(removed)]),
    [[5, 4, false]],
  );
  assert.equal(itemRecords([], virtualStash.addedItems).length, 2);

  const bundleBytes = await exportItemBundle(liveItems, 'Virtual Stash test');
  const bundle = JSON.parse(new TextDecoder().decode(bundleBytes));
  assert.equal(bundle.format, 'bkvince-item-bundle');
  assert.equal(bundle.name, 'Virtual Stash test');
  assert.equal(bundle.itemCount, 1);
  assert.equal(bundle.items.length, 1);
});

test('transfers one live item atomically across physical, virtual, and trash workspaces', async () => {
  const document = await createBlankCharacter({ name: 'TransferSafe', className: 'Warlock' });
  const starterItemCount = document.model.items.length;
  let player = addItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
    type: 'hax',
    containerId: 'stash',
    x: 0,
    y: 0,
    itemLevel: 42,
  });
  const sourceIndex = player.itemEdits.length - 1;
  const sourceId = player.addedItems.at(-1).id;
  player = editItemSnapshot(player, document.model.items, sourceIndex, { quality: 4 });
  player = editItemSnapshot(player, document.model.items, sourceIndex, {
    identified: false,
    magicPrefix: 13,
    magicSuffix: 106,
    magicAttributes: [
      { id: 0, values: [6], name: 'strength' },
      { id: 17, values: [20, 20], name: 'item_maxdamage_percent' },
    ],
  });
  let virtual = { addedItems: [], itemEdits: [], itemPlacements: [] };
  let trash = { addedItems: [], itemEdits: [], itemPlacements: [] };

  const movedToVirtual = transferItemSnapshot(player, document.model.items, virtual, [], {
    sourceIndex,
    reservedItemIds: characterItemIds(document.model),
  });
  player = movedToVirtual.source;
  virtual = movedToVirtual.target;
  assert.equal(player.itemPlacements[sourceIndex].removed, true);
  assert.equal(virtual.itemPlacements[0].removed, undefined);
  assert.equal(virtual.addedItems[0].type, 'hax');
  assert.notEqual(virtual.addedItems[0].id, sourceId);
  assert.equal(virtual.addedItems[0].quality, 4);
  assert.equal(virtual.addedItems[0].identified, 0);
  assert.equal(virtual.addedItems[0].magic_prefix, 13);
  assert.equal(virtual.addedItems[0].magic_suffix, 106);
  assert.deepEqual(virtual.addedItems[0].magic_attributes, [
    { id: 0, values: [6], name: 'strength' },
    { id: 17, values: [20, 20], name: 'item_maxdamage_percent' },
  ]);
  assert.deepEqual(
    [virtual.itemPlacements[0].x, virtual.itemPlacements[0].y],
    [0, 0],
  );

  const virtualBeforeFailure = structuredClone(virtual);
  const playerBeforeFailure = structuredClone(player);
  assert.throws(
    () => transferItemSnapshot(virtual, [], player, document.model.items, {
      sourceIndex: 0,
      containerId: 'stash',
      x: 15,
      y: 12,
      reservedItemIds: characterItemIds({
        items: itemRecords(document.model.items, player.addedItems),
        merc_items: [],
      }),
    }),
    /does not fit/i,
  );
  assert.deepEqual(virtual, virtualBeforeFailure);
  assert.deepEqual(player, playerBeforeFailure);

  const movedToTrash = transferItemSnapshot(virtual, [], trash, [], {
    sourceIndex: 0,
    reservedItemIds: characterItemIds({
      items: itemRecords(document.model.items, player.addedItems),
      merc_items: [],
    }),
  });
  virtual = movedToTrash.source;
  trash = movedToTrash.target;
  assert.equal(virtual.itemPlacements[0].removed, true);
  assert.equal(trash.addedItems.length, 1);

  const restored = transferItemSnapshot(trash, [], player, document.model.items, {
    sourceIndex: 0,
    containerId: 'cube',
    x: 2,
    y: 2,
    reservedItemIds: characterItemIds({
      items: [
        ...itemRecords(document.model.items, player.addedItems),
        ...itemRecords([], virtual.addedItems),
        ...itemRecords([], trash.addedItems),
      ],
      merc_items: [],
    }),
  });
  trash = restored.source;
  player = restored.target;
  assert.equal(trash.itemPlacements[0].removed, true);
  assert.equal(player.itemPlacements.filter(({ removed }) => !removed).length, starterItemCount + 1);
  assert.equal(player.addedItems.at(-1).type, 'hax');
  assert.deepEqual(
    pick(player.itemPlacements.at(-1), ['locationId', 'x', 'y']),
    { locationId: itemContainers.cube.locationId, x: 2, y: 2 },
  );

  const returnedToVirtual = transferItemSnapshot(player, document.model.items, virtual, [], {
    sourceIndex: player.itemPlacements.length - 1,
    reservedItemIds: characterItemIds({
      items: [
        ...itemRecords(document.model.items, player.addedItems),
        ...itemRecords([], virtual.addedItems),
        ...itemRecords([], trash.addedItems),
      ],
      merc_items: [],
    }),
  });
  player = returnedToVirtual.source;
  virtual = returnedToVirtual.target;
  const virtualLiveIndex = returnedToVirtual.targetIndex;
  const virtualBeforeEquipmentFailure = structuredClone(virtual);
  const playerBeforeEquipmentFailure = structuredClone(player);
  assert.throws(
    () => transferItemSnapshot(virtual, [], player, document.model.items, {
      sourceIndex: virtualLiveIndex,
      slotId: 1,
    }),
    /cannot be equipped in Head/i,
  );
  assert.deepEqual(virtual, virtualBeforeEquipmentFailure);
  assert.deepEqual(player, playerBeforeEquipmentFailure);

  const equipped = transferItemSnapshot(virtual, [], player, document.model.items, {
    sourceIndex: virtualLiveIndex,
    slotId: 4,
    reservedItemIds: characterItemIds({
      items: [
        ...itemRecords(document.model.items, player.addedItems),
        ...itemRecords([], virtual.addedItems),
        ...itemRecords([], trash.addedItems),
      ],
      merc_items: [],
    }),
  });
  virtual = equipped.source;
  player = equipped.target;
  assert.equal(virtual.itemPlacements[virtualLiveIndex].removed, true);
  assert.deepEqual(
    pick(player.itemPlacements.at(-1), ['locationId', 'equippedId', 'x', 'y']),
    { locationId: 1, equippedId: 4, x: 0, y: 0 },
  );

  const exported = await exportCharacter(document, player);
  assert.equal(exported.reparsed.items.length, starterItemCount + 1);
  assert.equal(exported.reparsed.items.at(-1).type, 'hax');
  assert.equal(exported.reparsed.items.at(-1).quality, 4);
  assert.equal(exported.reparsed.items.at(-1).identified, 0);
  assert.equal(exported.reparsed.items.at(-1).magic_prefix, 13);
  assert.equal(exported.reparsed.items.at(-1).magic_suffix, 106);
  assert.equal(exported.reparsed.items.at(-1).location_id, 1);
  assert.equal(exported.reparsed.items.at(-1).equipped_id, 4);
  assert.deepEqual(exported.reparsed.items.at(-1).magic_attributes, [
    { id: 0, values: [6], name: 'strength' },
    { id: 17, values: [20, 20], name: 'item_maxdamage_percent' },
  ]);
  validateSaveEnvelope(exported.bytes);
});

test('auto-places consecutive global Trash transfers without collision or source loss', async () => {
  const document = await createBlankCharacter({ name: 'TrashDrop', className: 'Barbarian' });
  let player = addItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
    type: 'hax',
    containerId: 'stash',
    x: 0,
    y: 0,
    itemLevel: 35,
  });
  const axeIndex = player.itemEdits.length - 1;
  player = addItemBatchSnapshot(player, document.model.items, {
    type: 'cap',
    containerId: 'stash',
    x: 2,
    y: 0,
    itemLevel: 35,
  });
  const capIndex = player.itemEdits.length - 1;
  let trash = { addedItems: [], itemEdits: [], itemPlacements: [] };

  const first = transferItemSnapshot(player, document.model.items, trash, [], {
    sourceIndex: axeIndex,
    autoPlace: true,
    reservedItemIds: characterItemIds(document.model),
  });
  player = first.source;
  trash = first.target;
  assert.deepEqual(
    trash.itemPlacements.map(({ type, x, y }) => [type, x, y]),
    [['hax', 0, 0]],
  );

  const second = transferItemSnapshot(player, document.model.items, trash, [], {
    sourceIndex: capIndex,
    autoPlace: true,
    reservedItemIds: characterItemIds({
      items: [
        ...itemRecords(document.model.items, player.addedItems),
        ...itemRecords([], trash.addedItems),
      ],
      merc_items: [],
    }),
  });
  player = second.source;
  trash = second.target;
  assert.equal(player.itemPlacements[axeIndex].removed, true);
  assert.equal(player.itemPlacements[capIndex].removed, true);
  assert.deepEqual(
    trash.itemPlacements.map(({ type, x, y, removed }) => [type, x, y, Boolean(removed)]),
    [['hax', 0, 0, false], ['cap', 1, 0, false]],
  );
  assert.deepEqual(trash.addedItems.map(({ type }) => type), ['hax', 'cap']);
  assert.notEqual(trash.addedItems[0].id, trash.addedItems[1].id);
});

test('adds complex stack quantities and refuses an oversized batch atomically', async () => {
  const document = await createBlankCharacter({ name: 'ForgeStack', className: 'Warlock' });
  const original = editableSnapshot(document.model);
  assert.throws(
    () => addItemBatchSnapshot(original, document.model.items, {
      type: 'cap',
      containerId: 'cube',
      x: 0,
      y: 0,
      count: 20,
      itemLevel: 1,
    }),
    /nothing was added/i,
  );
  assert.deepEqual(original.addedItems, []);
  assert.equal(original.itemPlacements.length, document.model.items.length);

  const added = addItemBatchSnapshot(original, document.model.items, {
    type: 'tbk',
    containerId: 'cube',
    x: 2,
    y: 2,
    count: 1,
    itemLevel: 9,
    quantity: 50,
  });
  const exported = await exportCharacter(document, added);
  const tome = exported.reparsed.items.at(-1);
  assert.equal(tome.type, 'tbk');
  assert.equal(tome.simple_item, 0);
  assert.equal(tome.quantity, 50);
  assert.deepEqual([tome.position_x, tome.position_y], [2, 2]);
});

test('empties only the personal stash in one source-preserving snapshot', async () => {
  const document = await createBlankCharacter({ name: 'EmptyStash', className: 'Amazon' });
  const starterInventoryCount = document.model.items.length;
  let editable = editableSnapshot(document.model);
  editable = addItemBatchSnapshot(editable, document.model.items, {
    type: 'hax',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 2,
    itemLevel: 41,
  });
  editable = addItemBatchSnapshot(editable, document.model.items, {
    type: 'cap',
    containerId: 'cube',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 17,
  });
  const beforeEmpty = structuredClone(editable);
  const emptied = emptyPersonalStashSnapshot(editable, document.model.items);
  assert.deepEqual(editable, beforeEmpty);
  assert.equal(emptied.itemPlacements.filter((placement) => containerForPlacement(placement) === 'stash').length, 0);
  assert.equal(emptied.itemPlacements.filter((placement) => containerForPlacement(placement) === 'cube').length, 1);
  assert.equal(emptied.itemPlacements.filter((placement) => containerForPlacement(placement) === 'inventory').length, starterInventoryCount);

  const exported = await exportCharacter(document, emptied);
  const reparsed = editableSnapshot(exported.reparsed);
  assert.equal(reparsed.itemPlacements.filter((placement) => containerForPlacement(placement) === 'stash').length, 0);
  assert.equal(reparsed.itemPlacements.filter((placement) => containerForPlacement(placement) === 'cube').length, 1);
  assert.equal(reparsed.itemPlacements.filter((placement) => containerForPlacement(placement) === 'inventory').length, starterInventoryCount);
  assert.equal(exported.reparsed.items.some(({ type }) => type === 'hax'), false);
  assert.equal(exported.reparsed.items.some(({ type }) => type === 'cap'), true);
  validateSaveEnvelope(exported.bytes);
});

test('exposes only spawnable four-character BKVince bases to click-to-add', () => {
  const bases = availableItemBases();
  assert.ok(bases.length > 700);
  assert.ok(bases.every(({ code, spawnable }) => spawnable && code.length <= 4));
  assert.ok(bases.some(({ code }) => code === 'hp1'));
  assert.ok(bases.some(({ code }) => code === 'hax'));
  assert.ok(bases.some(({ code }) => code === 'tbk'));
});

test('exposes and atomically adds governed Sets, Uniques, and Runewords from the unified catalog', async () => {
  const namedItems = availableNamedItems();
  const runewords = availableRunewordItems();
  assert.equal(namedItems.filter(({ kind }) => kind === 'set').length, 215);
  assert.equal(namedItems.filter(({ kind }) => kind === 'unique').length, 473);
  assert.equal(runewords.length, 112);

  const annihilus = namedItems.find(({ kind, id }) => kind === 'unique' && id === 381);
  const conquest = namedItems.find(({ kind, id }) => kind === 'set' && id === 127);
  const ravenFrost = namedItems.find(({ kind, name }) => kind === 'unique' && name === 'Raven Frost');
  const callToArms = runewords.find(({ id }) => id === 39);
  const plague = runewords.find(({ id }) => id === 131);
  const chaos = runewords.find(({ id }) => id === 42);
  const enigma = runewords.find(({ name }) => name === 'Enigma');
  assert.deepEqual(
    pick(annihilus, ['name', 'baseCode', 'baseName', 'attributeCount']),
    { name: 'Annihilus', baseCode: 'cm1', baseName: 'Small Charm', attributeCount: 10 },
  );
  assert.equal(conquest.name, "Warlord's Conquest");
  assert.ok(ravenFrost.searchTerms.some((term) => /cannot be frozen/i.test(term)));
  assert.ok(enigma.searchTerms.some((term) => /teleport/i.test(term)));
  assert.ok(annihilus.searchTerms.some((term) => /all skills/i.test(term)));
  assert.deepEqual(
    pick(callToArms, ['name', 'baseCode', 'baseName', 'runes']),
    { name: 'Call to Arms', baseCode: 'fla', baseName: 'Flail', runes: ['r11', 'r08', 'r23', 'r24', 'r27'] },
  );
  assert.deepEqual(
    pick(plague, ['name', 'baseCode', 'baseName', 'runes']),
    { name: 'Plague', baseCode: '7cr', baseName: 'Phase Blade', runes: ['r32', 'r13', 'r22'] },
  );
  assert.deepEqual(
    pick(chaos, ['name', 'baseCode', 'baseName', 'runes', 'attributeCount']),
    { name: 'Chaos', baseCode: 'ktr', baseName: 'Katar', runes: ['r19', 'r27', 'r22'], attributeCount: 7 },
  );
  assert.ok(chaos.searchTerms.some((term) => /whirlwind/i.test(term)));
  assert.ok(chaos.searchTerms.some((term) => /replenish durability|repairs 1 durability/i.test(term)));

  const document = await createBlankCharacter({ name: 'CatalogHero', className: 'Warlock' });
  let editable = editableSnapshot(document.model);
  editable = addItemBatchSnapshot(editable, document.model.items, {
    type: annihilus.baseCode,
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 99,
    quality: 7,
    uniqueId: annihilus.id,
    rollMode: 'maximum',
  });
  editable = addItemBatchSnapshot(editable, document.model.items, {
    type: conquest.baseCode,
    containerId: 'stash',
    x: 1,
    y: 0,
    count: 1,
    itemLevel: 99,
    quality: 5,
    setId: conquest.id,
    rollMode: 'minimum',
  });
  editable = addItemBatchSnapshot(editable, document.model.items, {
    type: callToArms.baseCode,
    containerId: 'stash',
    x: 4,
    y: 0,
    count: 1,
    itemLevel: 99,
    quality: 2,
    runewordId: callToArms.id,
    rollMode: 'maximum',
  });
  editable = addItemBatchSnapshot(editable, document.model.items, {
    type: plague.baseCode,
    containerId: 'stash',
    x: 7,
    y: 0,
    count: 1,
    itemLevel: 99,
    quality: 2,
    runewordId: plague.id,
    rollMode: 'maximum',
  });

  const exported = await exportCharacter(document, editable);
  const reparsedAnnihilus = exported.reparsed.items.find(({ unique_id: id }) => id === 381);
  const reparsedConquest = exported.reparsed.items.find(({ set_id: id }) => id === 127);
  const reparsedCallToArms = exported.reparsed.items.find(({ runeword_id: id }) => id === 39);
  const reparsedPlague = exported.reparsed.items.find(({ runeword_id: id }) => id === 131);
  assert.deepEqual(
    pick(reparsedAnnihilus, ['type', 'quality', 'unique_id']),
    { type: 'cm1', quality: 7, unique_id: 381 },
  );
  assert.equal(reparsedAnnihilus.magic_attributes.length, annihilus.attributeCount);
  assert.deepEqual(
    pick(reparsedConquest, ['type', 'quality', 'set_id']),
    { type: conquest.baseCode, quality: 5, set_id: 127 },
  );
  assert.deepEqual(
    reparsedCallToArms.socketed_items.map(({ type }) => type),
    callToArms.runes,
  );
  assert.deepEqual(
    reparsedPlague.socketed_items.map(({ type }) => type),
    plague.runes,
  );
  assert.equal(describeItem(reparsedCallToArms, 0).name, 'Call to Arms');
  assert.equal(describeItem(reparsedPlague, 0).baseName, 'Phase Blade');
  validateSaveEnvelope(exported.bytes);

  const reopened = await openCharacter(exported.bytes, 'CatalogHero.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, exported.bytes);

  const original = editableSnapshot(document.model);
  assert.throws(
    () => addItemBatchSnapshot(original, document.model.items, {
      type: 'rin',
      containerId: 'stash',
      x: 0,
      y: 0,
      count: 1,
      itemLevel: 99,
      quality: 7,
      uniqueId: annihilus.id,
    }),
    /requires base cm1/i,
  );
  assert.throws(
    () => addItemBatchSnapshot(original, document.model.items, {
      type: 'cap',
      containerId: 'stash',
      x: 0,
      y: 0,
      count: 1,
      itemLevel: 99,
      runewordId: callToArms.id,
    }),
    /not compatible|requires/i,
  );
  assert.deepEqual(original.addedItems, []);
});

test('creates catalog items with perfect governed rolls and maximum base defense by default', async () => {
  const namedItems = availableNamedItems();
  const runewords = availableRunewordItems();
  const annihilus = namedItems.find(({ kind, name }) => kind === 'unique' && name === 'Annihilus');
  const conquest = namedItems.find(({ kind, name }) => kind === 'set' && name === "Warlord's Conquest");
  const callToArms = runewords.find(({ name }) => name === 'Call to Arms');
  const capBase = itemCatalog.bases.find(({ code }) => code === 'cap');
  const conquestBase = itemCatalog.bases.find(({ code }) => code === conquest.baseCode);
  const document = await createBlankCharacter({ name: 'PerfectForge', className: 'Warlock' });
  let editable = editableSnapshot(document.model);
  const firstIndex = editable.itemEdits.length;

  editable = addItemBatchSnapshot(editable, document.model.items, {
    type: capBase.code,
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
  });
  editable = addItemBatchSnapshot(editable, document.model.items, {
    type: annihilus.baseCode,
    containerId: 'stash',
    x: 2,
    y: 0,
    count: 1,
    quality: 7,
    uniqueId: annihilus.id,
  });
  editable = addItemBatchSnapshot(editable, document.model.items, {
    type: conquest.baseCode,
    containerId: 'stash',
    x: 3,
    y: 0,
    count: 1,
    quality: 5,
    setId: conquest.id,
  });
  editable = addItemBatchSnapshot(editable, document.model.items, {
    type: callToArms.baseCode,
    containerId: 'stash',
    x: 5,
    y: 0,
    count: 1,
    quality: 2,
    runewordId: callToArms.id,
  });

  const records = itemRecords(document.model.items, editable.addedItems);
  const effective = editableItems(document.model.items, editable.itemEdits, editable.addedItems);
  const capIndex = firstIndex;
  const annihilusIndex = firstIndex + 1;
  const conquestIndex = firstIndex + 2;
  const callToArmsIndex = firstIndex + 3;
  assert.deepEqual(
    effective.slice(capIndex, callToArmsIndex + 1).map(({ level }) => level),
    [99, 99, 99, 99],
  );
  assert.equal(effective[capIndex].defense_rating, capBase.defenseMaximum);
  assert.equal(effective[conquestIndex].defense_rating, conquestBase.defenseMaximum);
  assert.deepEqual(
    editable.itemEdits[annihilusIndex].magicAttributes,
    compileNamedQualityPatch(records[annihilusIndex], editable.itemEdits[annihilusIndex], 'maximum').magicAttributes,
  );
  assert.deepEqual(
    editable.itemEdits[conquestIndex].magicAttributes,
    compileNamedQualityPatch(records[conquestIndex], editable.itemEdits[conquestIndex], 'maximum').magicAttributes,
  );
  assert.deepEqual(
    editable.itemEdits[callToArmsIndex].runewordAttributes,
    compileRunewordPatch(effective[callToArmsIndex], editable.itemEdits[callToArmsIndex], callToArms.id, 'maximum').runewordAttributes,
  );

  const exported = await exportCharacter(document, editable);
  const reparsedCap = exported.reparsed.items.find(({ type, quality }) => type === capBase.code && quality === 2);
  const reparsedConquest = exported.reparsed.items.find(({ set_id: id }) => id === conquest.id);
  assert.equal(reparsedCap.defense_rating, capBase.defenseMaximum);
  assert.equal(reparsedConquest.defense_rating, conquestBase.defenseMaximum);
  validateSaveEnvelope(exported.bytes);
});

test('adds every governed RuneWizard quick group atomically', async () => {
  const groups = availableItemGroups();
  assert.deepEqual(
    groups.map(({ id, label, entries }) => [id, label, entries.length]),
    [
      ['worldstone-shards', 'Worldstone Shards', 5],
      ['uber-ancients-materials', 'Uber Ancients Materials', 11],
      ['warlords-glory-set', "Warlord's Glory Set", 5],
      ['cube', 'Cube', 1],
      ['organ-set', 'Organ Set', 3],
      ['key-set', 'Key Set', 3],
    ],
  );

  for (const group of groups) {
    const document = await createBlankCharacter({ name: 'GroupHero', className: 'Warlock' });
    const original = editableSnapshot(document.model);
    const next = addItemGroupSnapshot(original, document.model.items, {
      groupId: group.id,
      selections: group.entries.map(({ id }) => ({ id, count: 1 })),
      containerId: 'stash',
      x: 0,
      y: 0,
      itemLevel: 99,
    });
    assert.equal(next.addedItems.length, group.entries.length);
    const exported = await exportCharacter(document, next);
    const added = exported.reparsed.items.slice(-group.entries.length);
    assert.deepEqual(added.map(({ type }) => type), group.entries.map(({ type }) => type));
    if (group.id === 'warlords-glory-set') {
      assert.deepEqual(added.map(({ quality }) => quality), [5, 5, 5, 5, 5]);
      assert.deepEqual(added.map(({ set_id }) => set_id), [127, 128, 129, 130, 131]);
      assert.ok(added.every(({ magic_attributes: attributes }) => attributes.length > 0));
      const conquest = describeItem(added[0], 0, 99);
      assert.equal(conquest.requiredStrength, 60);
      assert.equal(conquest.requiredLevel, 1);
      assert.ok(conquest.magicAttributes.includes('+15 to Strength'));
      assert.ok(conquest.magicAttributes.includes('+45 to Attack Rating'));
    }
    validateSaveEnvelope(exported.bytes);
  }
});

test('builds RuneWizard-style damage and requirement lines from governed BKVince constants', async () => {
  const document = await createBlankCharacter({ name: 'TooltipHero', className: 'Warlock' });
  const editable = addItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
    type: '7cr',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 85,
  });
  const itemIndex = editable.itemEdits.length - 1;
  const phaseBlade = editableItems(document.model.items, editable.itemEdits, editable.addedItems)[itemIndex];
  const originalPhaseBlade = structuredClone(phaseBlade);
  const descriptor = describeItem(phaseBlade, itemIndex, 99);
  assert.deepEqual(
    pick(descriptor, ['requiredStrength', 'requiredDexterity', 'requiredLevel', 'damageRanges']),
    {
      requiredStrength: 25,
      requiredDexterity: 136,
      requiredLevel: 54,
      damageRanges: [{ label: 'Damage', minimum: 47, maximum: 53 }],
    },
  );
  assert.deepEqual(phaseBlade, originalPhaseBlade);
});

test('duplicates an edited personalized item with new IDs and one undoable snapshot', async () => {
  const document = await createBlankCharacter({ name: 'DuplicateUI', className: 'Warlock' });
  let editable = addItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
    type: 'hax',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 88,
  });
  const itemIndex = editable.itemEdits.length - 1;
  editable = editItemSnapshot(editable, document.model.items, itemIndex, {
    quality: 4,
    ethereal: true,
    personalized: true,
    personalizedName: 'RuffnecKk',
  });
  editable = duplicateItemSnapshot(editable, document.model.items, {
    itemIndex,
    count: 2,
  });
  assert.equal(editable.addedItems.length, 3);
  assert.deepEqual(editable.itemPlacements.slice(-3).map(({ x, y }) => [x, y]), [[0, 0], [1, 0], [2, 0]]);

  const exported = await exportCharacter(document, editable);
  const axes = exported.reparsed.items.slice(-3);
  assert.equal(new Set(axes.map(({ id }) => id)).size, 3);
  assert.ok(axes.every((item) => item.type === 'hax' && item.quality === 4 && item.ethereal === 1));
  assert.ok(axes.every((item) => item.personalized === 1 && item.personalized_name === 'RuffnecKk'));
  validateSaveEnvelope(exported.bytes);
});

test('adds an arbitrary mixed catalog batch atomically with independent quantities and identities', async () => {
  const namedItems = availableNamedItems();
  const runewords = availableRunewordItems();
  const annihilus = namedItems.find(({ kind, name }) => kind === 'unique' && name === 'Annihilus');
  const conquest = namedItems.find(({ kind, name }) => kind === 'set' && name === "Warlord's Conquest");
  const callToArms = runewords.find(({ name }) => name === 'Call to Arms');
  const document = await createBlankCharacter({ name: 'MixedBatch', className: 'Warlock' });
  const original = editableSnapshot(document.model);
  const selections = [
    {
      type: annihilus.baseCode,
      count: 2,
      itemLevel: 99,
      quality: 7,
      uniqueId: annihilus.id,
      rollMode: 'maximum',
    },
    {
      type: conquest.baseCode,
      count: 1,
      itemLevel: 87,
      quality: 5,
      setId: conquest.id,
      rollMode: 'minimum',
    },
    {
      type: callToArms.baseCode,
      count: 1,
      itemLevel: 90,
      quality: 2,
      runewordId: callToArms.id,
      rollMode: 'maximum',
    },
  ];
  const editable = addCatalogItemBatchSnapshot(original, document.model.items, {
    selections,
    containerId: 'stash',
    x: 0,
    y: 0,
    reservedItemIds: characterItemIds(document.model),
  });
  const exported = await exportCharacter(document, editable);
  assert.equal(exported.reparsed.items.filter(({ unique_id: id }) => id === annihilus.id).length, 2);
  assert.equal(exported.reparsed.items.filter(({ set_id: id }) => id === conquest.id).length, 1);
  assert.equal(exported.reparsed.items.filter(({ runeword_id: id }) => id === callToArms.id).length, 1);
  assert.equal(exported.reparsed.items.filter(({
    location_id: locationId,
    alt_position_id: altPositionId,
  }) => (
    locationId === itemContainers.stash.locationId
    && altPositionId === itemContainers.stash.altPositionId
  )).length, 4);
  validateSaveEnvelope(exported.bytes);

  const impossible = Array.from({ length: 20 }, () => ({
    type: 'utp',
    count: 1,
    itemLevel: 99,
  }));
  assert.throws(
    () => addCatalogItemBatchSnapshot(original, document.model.items, {
      selections: impossible,
      containerId: 'cube',
      x: 0,
      y: 0,
    }),
    /room|fit|placement/i,
  );
  assert.deepEqual(original, editableSnapshot(document.model));
});

test('upgrades and downgrades a governed Set base while preserving its native item payload', async () => {
  const document = await createBlankCharacter({ name: 'TierForge', className: 'Warlock' });
  let editable = addItemGroupSnapshot(editableSnapshot(document.model), document.model.items, {
    groupId: 'warlords-glory-set',
    selections: availableItemGroups()
      .find(({ id }) => id === 'warlords-glory-set')
      .entries.map(({ id }) => ({ id, count: 1 })),
    containerId: 'stash',
    x: 0,
    y: 0,
    itemLevel: 99,
  });
  const records = itemRecords(document.model.items, editable.addedItems);
  const itemIndex = editable.itemEdits.findIndex(({ type, setId }) => type === 'hgl' && setId === 127);
  assert.ok(itemIndex >= 0);
  editable = editItemSnapshot(editable, document.model.items, itemIndex, {
    personalized: true,
    personalizedName: 'RuffnecKk',
  });
  const original = structuredClone(editable.itemEdits[itemIndex]);
  const normalOptions = itemEditorOptions(records[itemIndex], original);
  assert.deepEqual(normalOptions.tierBases.map(({ code }) => code), ['hgl', 'xhg', 'uhg']);
  assert.equal(normalOptions.downgradeBase, null);
  assert.equal(normalOptions.upgradeBase.code, 'xhg');

  editable = editItemSnapshot(
    editable,
    document.model.items,
    itemIndex,
    compileItemTierPatch(records[itemIndex], editable.itemEdits[itemIndex], 'up'),
  );
  assert.deepEqual(
    pick(editable.itemEdits[itemIndex], ['type', 'defense', 'maximumDurability', 'currentDurability']),
    { type: 'xhg', defense: 43, maximumDurability: 24, currentDurability: 24 },
  );
  editable = editItemSnapshot(
    editable,
    document.model.items,
    itemIndex,
    compileItemTierPatch(records[itemIndex], editable.itemEdits[itemIndex], 'up'),
  );
  const eliteEdit = editable.itemEdits[itemIndex];
  assert.equal(eliteEdit.type, 'uhg');
  assert.equal(eliteEdit.defense, 62);
  assert.equal(eliteEdit.quality, 5);
  assert.equal(eliteEdit.setId, 127);
  assert.equal(eliteEdit.personalizedName, 'RuffnecKk');
  assert.deepEqual(eliteEdit.magicAttributes, original.magicAttributes);
  assert.deepEqual(eliteEdit.setAttributes, original.setAttributes);
  assert.deepEqual(eliteEdit.socketedItems, original.socketedItems);

  const eliteExport = await exportCharacter(document, editable);
  const elite = eliteExport.reparsed.items[itemIndex];
  assert.deepEqual(
    pick(elite, ['type', 'defense_rating', 'max_durability', 'current_durability', 'level', 'quality', 'set_id']),
    {
      type: 'uhg',
      defense_rating: 62,
      max_durability: 24,
      current_durability: 24,
      level: 99,
      quality: 5,
      set_id: 127,
    },
  );
  assert.equal(elite.personalized_name, 'RuffnecKk');
  assert.deepEqual(elite.magic_attributes, original.magicAttributes);

  editable = editItemSnapshot(
    editable,
    document.model.items,
    itemIndex,
    compileItemTierPatch(records[itemIndex], editable.itemEdits[itemIndex], 'down'),
  );
  editable = editItemSnapshot(
    editable,
    document.model.items,
    itemIndex,
    compileItemTierPatch(records[itemIndex], editable.itemEdits[itemIndex], 'down'),
  );
  const normalExport = await exportCharacter(document, editable);
  const normal = normalExport.reparsed.items[itemIndex];
  assert.equal(normal.type, 'hgl');
  assert.equal(normal.defense_rating, 15);
  assert.equal(normal.set_id, 127);
  assert.equal(normal.personalized_name, 'RuffnecKk');
  assert.deepEqual(normal.magic_attributes, original.magicAttributes);
  validateSaveEnvelope(eliteExport.bytes);
  validateSaveEnvelope(normalExport.bytes);
});

test('rebuilds a governed Magic payload with compatible affixes and numeric attributes', async () => {
  assert.equal(itemCatalog.prefixes.find(({ sourceName }) => sourceName === 'Sturdy').id, 2);
  assert.equal(itemCatalog.prefixes.find(({ sourceName }) => sourceName === 'Stout').id, 119);
  assert.deepEqual(
    itemCatalog.suffixes.filter(({ sourceName }) => sourceName === 'of Health').map(({ id }) => id),
    [1, 115],
  );
  assert.ok(!itemCatalog.prefixes.some(({ sourceName }) => sourceName === 'Expansion'));
  assert.ok(!itemCatalog.suffixes.some(({ sourceName }) => sourceName === 'Expansion'));
  const strength = availableMagicAttributes().find(({ id }) => id === 0);
  const enhancedDamage = availableMagicAttributes().find(({ id }) => id === 17);
  const skillTab = availableMagicAttributes().find(({ id }) => id === 188);
  const skillWhenStruck = availableMagicAttributes().find(({ id }) => id === 201);
  assert.deepEqual(skillTab.values.map(({ name, control }) => [name, control]), [
    ['Tab within class', 'number'],
    ['Class', 'class'],
    ['Bonus', 'number'],
  ]);
  assert.deepEqual(skillWhenStruck.values.map(({ name, control }) => [name, control]), [
    ['Skill level', 'number'],
    ['Skill', 'skill'],
    ['Chance (%)', 'number'],
  ]);
  assert.deepEqual(
    pick(strength.values[0], ['minimum', 'maximum']),
    { minimum: -32, maximum: 223 },
  );

  const document = await createBlankCharacter({ name: 'MagicForge', className: 'Warlock' });
  let editable = addItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
    type: 'hax',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 99,
  });
  const itemIndex = editable.itemEdits.length - 1;
  editable = editItemSnapshot(editable, document.model.items, itemIndex, { quality: 4 });
  const item = editable.addedItems.at(-1);
  const magicOptions = itemEditorOptions(item, editable.itemEdits[itemIndex]);
  const prefix = magicOptions.prefixes.find(({ id }) => id === 13);
  const suffix = magicOptions.suffixes.find(({ id }) => id === 106);
  assert.ok(prefix && suffix);
  assert.ok(!magicOptions.prefixes.some(({ sourceName }) => sourceName === 'Sturdy'));
  editable = editItemSnapshot(editable, document.model.items, itemIndex, {
    magicPrefix: prefix.id,
    magicSuffix: suffix.id,
  });
  const compilerOptions = itemEditorOptions(item, editable.itemEdits[itemIndex]);
  assert.deepEqual(
    pick(compilerOptions.affixCompiler, ['supported', 'modCount', 'attributeCount']),
    { supported: true, modCount: 2, attributeCount: 2 },
  );
  assert.deepEqual(compileMagicAffixAttributes(item, editable.itemEdits[itemIndex], 'minimum'), [
    { id: 0, values: [4], name: 'strength' },
    { id: 17, values: [10, 10], name: enhancedDamage.name },
  ]);
  const maximumRolls = compileMagicAffixAttributes(item, editable.itemEdits[itemIndex], 'maximum');
  assert.deepEqual(maximumRolls, [
    { id: 0, values: [6], name: 'strength' },
    { id: 17, values: [20, 20], name: enhancedDamage.name },
  ]);
  editable = editItemSnapshot(editable, document.model.items, itemIndex, {
    magicAttributes: maximumRolls.toReversed(),
  });

  const warlockTabAndProc = {
    ...editable.itemEdits[itemIndex],
    magicPrefix: 711,
    magicSuffix: 436,
  };
  assert.deepEqual(compileMagicAffixAttributes(item, warlockTabAndProc, 'maximum'), [
    { id: 188, values: [0, 7, 3], name: 'item_addskill_tab' },
    { id: 201, values: [40, 48, 33], name: 'item_skillongethit' },
  ]);

  const warlockClassAndOskill = {
    ...editable.itemEdits[itemIndex],
    magicPrefix: 715,
    magicSuffix: 618,
  };
  assert.deepEqual(compileMagicAffixAttributes(item, warlockClassAndOskill, 'maximum'), [
    { id: 83, values: [7, 2], name: 'item_addclassskills' },
    { id: 97, values: [126, 1], name: 'item_nonclassskill' },
  ]);
  assert.deepEqual(
    compileMagicAffixPatch(item, { ...warlockClassAndOskill, magicPrefix: 420 }, 'maximum'),
    {
      magicAttributes: [{ id: 97, values: [126, 1], name: 'item_nonclassskill' }],
      socketed: true,
      totalSockets: 2,
    },
  );
  assert.deepEqual(
    compileMagicAffixAttributes(item, { ...warlockClassAndOskill, magicSuffix: 532 }, 'minimum'),
    [
      { id: 83, values: [7, 2], name: 'item_addclassskills' },
      { id: 204, values: [6, 54, 7, 52], name: 'item_charged_skill' },
    ],
  );
  assert.deepEqual(
    compileMagicAffixAttributes(item, { ...warlockClassAndOskill, magicSuffix: 532 }, 'maximum'),
    [
      { id: 83, values: [7, 2], name: 'item_addclassskills' },
      { id: 204, values: [6, 54, 52, 52], name: 'item_charged_skill' },
    ],
  );
  assert.deepEqual(
    compileMagicAffixAttributes(item, { ...warlockClassAndOskill, magicPrefix: 723 }, 'maximum'),
    [
      { id: 97, values: [126, 1], name: 'item_nonclassskill' },
      { id: 126, values: [2, 1], name: 'item_elemskill' },
      { id: 376, values: [0, 1], name: 'item_elemskill_lightning' },
    ],
  );

  const exported = await exportCharacter(document, editable);
  const magicAxe = exported.reparsed.items.at(-1);
  assert.equal(magicAxe.quality, 4);
  assert.equal(magicAxe.magic_prefix, prefix.id);
  assert.equal(magicAxe.magic_suffix, suffix.id);
  assert.deepEqual(magicAxe.magic_attributes, [
    { id: 0, values: [6], name: 'strength' },
    { id: 17, values: [20, 20], name: enhancedDamage.name },
  ]);
  assert.match(describeItem(magicAxe, itemIndex).name, new RegExp(prefix.name));
  assert.throws(
    () => editItemSnapshot(editable, document.model.items, itemIndex, {
      magicAttributes: [{ id: 0, values: [224], name: 'strength' }],
    }),
    /between -32 and 223/i,
  );
  const reverted = editItemSnapshot(editable, document.model.items, itemIndex, { quality: 2 });
  assert.equal(reverted.itemEdits[itemIndex].magicPrefix, null);
  assert.equal(reverted.itemEdits[itemIndex].magicSuffix, null);
  assert.deepEqual(reverted.itemEdits[itemIndex].magicAttributes, []);
});

test('exports and atomically imports one .d2i record or a named item bundle', async () => {
  const source = await createBlankCharacter({ name: 'ItemSource', className: 'Amazon' });
  let sourceEdit = addItemBatchSnapshot(editableSnapshot(source.model), source.model.items, {
    type: 'hax',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 42,
  });
  const sourceExport = await exportCharacter(source, sourceEdit);
  const sourceItem = sourceExport.reparsed.items.at(-1);
  const portable = await exportItemRecord(sourceItem);
  assert.ok(portable.bytes.length > 20);

  const imported = await importItemFiles([memoryFile('Hand-Axe.d2i', portable.bytes)]);
  assert.equal(imported.length, 1);
  assert.equal(imported[0].type, 'hax');
  const target = await createBlankCharacter({ name: 'ItemTarget', className: 'Warlock' });
  assert.deepEqual(
    characterItemIds({ items: [{ id: 1, socketed_items: [{ id: 2 }] }], merc_items: [{ id: 3 }] }),
    [1, 2, 3],
  );
  const targetEdit = addImportedItemsSnapshot(editableSnapshot(target.model), target.model.items, {
    importedItems: imported,
    containerId: 'cube',
    x: 0,
    y: 0,
  });
  const targetExport = await exportCharacter(target, targetEdit);
  const targetItem = targetExport.reparsed.items.at(-1);
  assert.equal(targetItem.type, 'hax');
  assert.notEqual(targetItem.id, sourceItem.id);
  assert.notDeepEqual(targetItem._unknown_data.realm_data, sourceItem._unknown_data.realm_data);
  assert.deepEqual([targetItem.alt_position_id, targetItem.position_x, targetItem.position_y], [4, 0, 0]);

  const trailing = new Uint8Array(portable.bytes.length + 3);
  trailing.set(portable.bytes);
  trailing.set([0xaa, 0xbb, 0xcc], portable.bytes.length);
  await assert.rejects(
    () => importItemFiles([memoryFile('Trailing.d2i', trailing)]),
    /Trailing\.d2i:.*(?:trailing bytes|non-canonical)/i,
  );
  await assert.rejects(
    () => importItemFiles([memoryFile('Wrong.txt', portable.bytes)]),
    /Wrong\.txt must use the \.d2i or \.bkitems\.json extension/i,
  );
  await assert.rejects(
    () => importItemFiles([memoryFile('Broken.bkitems.json', new TextEncoder().encode('{broken'))]),
    /Broken\.bkitems\.json is not valid JSON/i,
  );

  const bundleBytes = await exportItemBundle([sourceItem], 'Hand Axe test');
  assert.match(new TextDecoder().decode(bundleBytes), /"itemAbiSha256": "[0-9a-f]{64}"/);
  const bundleItems = await importItemFiles([memoryFile('Hand-Axe.bkitems.json', bundleBytes)]);
  assert.equal(bundleItems.length, 1);
  assert.equal(bundleItems[0].type, 'hax');
  const badCount = JSON.parse(new TextDecoder().decode(bundleBytes));
  badCount.itemCount = 2;
  await assert.rejects(
    () => importItemFiles([memoryFile(
      'Bad-Count.bkitems.json',
      new TextEncoder().encode(JSON.stringify(badCount)),
    )]),
    /itemCount does not match/i,
  );
  badCount.itemCount = 1;
  badCount.itemAbiSha256 = '0'.repeat(64);
  await assert.rejects(
    () => importItemFiles([memoryFile(
      'Bad-ABI.bkitems.json',
      new TextEncoder().encode(JSON.stringify(badCount)),
    )]),
    /different BKVince item-table ABI/i,
  );
  const malformedPayload = JSON.parse(new TextDecoder().decode(bundleBytes));
  malformedPayload.items[0] = '***';
  await assert.rejects(
    () => importItemFiles([memoryFile(
      'Malformed.bkitems.json',
      new TextEncoder().encode(JSON.stringify(malformedPayload)),
    )]),
    /Malformed\.bkitems\.json item 1:/i,
  );
  const oversizedBundle = JSON.parse(new TextDecoder().decode(bundleBytes));
  oversizedBundle.items = Array.from({ length: 21 }, () => oversizedBundle.items[0]);
  oversizedBundle.itemCount = oversizedBundle.items.length;
  await assert.rejects(
    () => importItemFiles([memoryFile(
      'Too-Many.bkitems.json',
      new TextEncoder().encode(JSON.stringify(oversizedBundle)),
    )]),
    /more than 20 item records; nothing was imported/i,
  );
});

test('writes and reparses the governed parameterized affix encodings', async () => {
  const document = await createBlankCharacter({ name: 'ParamForge', className: 'Warlock' });
  const barbarianHelm = itemCatalog.bases.find((base) => (
    base.spawnable && base.typeCodes.includes('phlm')
  ));
  assert.ok(barbarianHelm);
  let editable = editableSnapshot(document.model);

  const cases = [
    {
      type: 'ci0',
      x: 0,
      magicPrefix: 711,
      magicSuffix: 436,
      expected: [
        { id: 188, values: [0, 7, 3], name: 'item_addskill_tab' },
        { id: 201, values: [40, 48, 33], name: 'item_skillongethit' },
      ],
    },
    {
      type: 'dgr',
      x: 2,
      magicPrefix: 715,
      magicSuffix: 0,
      expected: [{ id: 83, values: [7, 2], name: 'item_addclassskills' }],
    },
    {
      type: barbarianHelm.code,
      x: 4,
      magicPrefix: 0,
      magicSuffix: 618,
      expected: [{ id: 97, values: [126, 1], name: 'item_nonclassskill' }],
    },
    {
      type: 'cm3',
      x: 6,
      magicPrefix: 723,
      magicSuffix: 0,
      expected: [
        { id: 126, values: [2, 1], name: 'item_elemskill' },
        { id: 376, values: [0, 1], name: 'item_elemskill_lightning' },
      ],
    },
    {
      type: 'sst',
      x: 7,
      magicPrefix: 0,
      magicSuffix: 532,
      expected: [{ id: 204, values: [6, 54, 52, 52], name: 'item_charged_skill' }],
    },
  ];

  for (const entry of cases) {
    editable = addItemBatchSnapshot(editable, document.model.items, {
      type: entry.type,
      containerId: 'stash',
      x: entry.x,
      y: 0,
      count: 1,
      itemLevel: 99,
    });
    const itemIndex = editable.itemEdits.length - 1;
    const item = editable.addedItems.at(-1);
    editable = editItemSnapshot(editable, document.model.items, itemIndex, { quality: 4 });
    editable = editItemSnapshot(editable, document.model.items, itemIndex, {
      magicPrefix: entry.magicPrefix,
      magicSuffix: entry.magicSuffix,
    });
    const attributes = compileMagicAffixAttributes(item, editable.itemEdits[itemIndex], 'maximum');
    assert.deepEqual(attributes, entry.expected);
    editable = editItemSnapshot(editable, document.model.items, itemIndex, { magicAttributes: attributes });
  }

  const exported = await exportCharacter(document, editable);
  assert.deepEqual(
    exported.reparsed.items.slice(-cases.length).map((item) => item.magic_attributes),
    cases.map(({ expected }) => expected),
  );
});

test('applies Magic socket affixes to the governed structural fields', async () => {
  const document = await createBlankCharacter({ name: 'SocketForge', className: 'Warlock' });
  let editable = editableSnapshot(document.model);
  const cases = [
    { type: 'hax', x: 0, prefix: 420, expectedSockets: 2 },
    { type: 'crn', x: 2, prefix: 421, expectedSockets: 3 },
    { type: 'gth', x: 4, prefix: 422, expectedSockets: 4 },
  ];

  for (const entry of cases) {
    editable = addItemBatchSnapshot(editable, document.model.items, {
      type: entry.type,
      containerId: 'stash',
      x: entry.x,
      y: 0,
      count: 1,
      itemLevel: 99,
    });
    const itemIndex = editable.itemEdits.length - 1;
    const item = editable.addedItems.at(-1);
    editable = editItemSnapshot(editable, document.model.items, itemIndex, { quality: 4 });
    editable = editItemSnapshot(editable, document.model.items, itemIndex, {
      magicPrefix: entry.prefix,
      magicSuffix: 0,
    });
    const affixPatch = compileMagicAffixPatch(item, editable.itemEdits[itemIndex], 'maximum');
    assert.deepEqual(affixPatch, {
      magicAttributes: [],
      socketed: true,
      totalSockets: entry.expectedSockets,
    });
    editable = editItemSnapshot(editable, document.model.items, itemIndex, affixPatch);
  }

  const exported = await exportCharacter(document, editable);
  assert.deepEqual(
    exported.reparsed.items.slice(-cases.length).map((item) => ({
      socketed: item.socketed,
      totalSockets: item.total_nr_of_sockets,
      filledSockets: item.nr_of_items_in_sockets,
      attributes: item.magic_attributes,
    })),
    cases.map(({ expectedSockets }) => ({
      socketed: 1,
      totalSockets: expectedSockets,
      filledSockets: 0,
      attributes: [],
    })),
  );
});

test('rebuilds governed Set and Unique items and preserves them through write, reparse, and no-op export', async () => {
  const document = await createBlankCharacter({ name: 'NamedForge', className: 'Warlock' });
  let editable = editableSnapshot(document.model);
  const cases = [
    {
      type: '7fb',
      x: 0,
      quality: 5,
      identity: { setId: 116 },
      expected: [
        { id: 0, values: [30], name: 'strength' },
        { id: 17, values: [250, 250], name: 'item_maxdamage_percent' },
        { id: 39, values: [20], name: 'fireresist' },
        { id: 41, values: [20], name: 'lightresist' },
        { id: 43, values: [20], name: 'coldresist' },
        { id: 45, values: [20], name: 'poisonresist' },
        { id: 60, values: [5], name: 'lifedrainmindam' },
        { id: 93, values: [20], name: 'item_fasterattackrate' },
        { id: 135, values: [15], name: 'item_openwounds' },
        { id: 198, values: [10, 92, 20], name: 'item_skillonhit' },
      ],
    },
    {
      type: 'cm1',
      x: 3,
      quality: 7,
      identity: { uniqueId: 381 },
      expected: [
        { id: 0, values: [20], name: 'strength' },
        { id: 1, values: [20], name: 'energy' },
        { id: 2, values: [20], name: 'dexterity' },
        { id: 3, values: [20], name: 'vitality' },
        { id: 39, values: [20], name: 'fireresist' },
        { id: 41, values: [20], name: 'lightresist' },
        { id: 43, values: [20], name: 'coldresist' },
        { id: 45, values: [20], name: 'poisonresist' },
        { id: 85, values: [15], name: 'item_addexperience' },
        { id: 127, values: [1], name: 'item_allskills' },
      ],
    },
  ];

  for (const entry of cases) {
    editable = addItemBatchSnapshot(editable, document.model.items, {
      type: entry.type,
      containerId: 'stash',
      x: entry.x,
      y: 0,
      count: 1,
      itemLevel: 99,
    });
    const itemIndex = editable.itemEdits.length - 1;
    const item = editable.addedItems.at(-1);
    editable = editItemSnapshot(editable, document.model.items, itemIndex, { quality: entry.quality });
    editable = editItemSnapshot(editable, document.model.items, itemIndex, entry.identity);
    const patch = compileNamedQualityPatch(item, editable.itemEdits[itemIndex], 'maximum');
    assert.deepEqual(patch.magicAttributes, entry.expected);
    editable = editItemSnapshot(editable, document.model.items, itemIndex, patch);
  }

  const exported = await exportCharacter(document, editable);
  const [setItem, uniqueItem] = exported.reparsed.items.slice(-cases.length);
  assert.deepEqual(
    pick(setItem, ['type', 'quality', 'set_id', 'magic_attributes']),
    { type: '7fb', quality: 5, set_id: 116, magic_attributes: cases[0].expected },
  );
  assert.equal(setItem._unknown_data.plist_flag, 0);
  assert.equal(setItem.set_attributes, undefined);
  assert.deepEqual(
    pick(uniqueItem, ['type', 'quality', 'unique_id', 'magic_attributes']),
    { type: 'cm1', quality: 7, unique_id: 381, magic_attributes: cases[1].expected },
  );

  const reopened = await openCharacter(exported.bytes, 'NamedForge.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, exported.bytes);
});

test('compiles permanent and non-contiguous Set bonus lists with their exact plist masks', async () => {
  const civerb = itemCatalog.setItems.find(({ id }) => id === 2);
  assert.equal(civerb.addFunction, 0);
  assert.equal(civerb.setBonusLists.length, 0);
  const civerbPatch = compileNamedQualityPatch(
    { type: 'gsc', level: 99, nr_of_items_in_sockets: 0 },
    { index: 0, type: 'gsc', quality: 5, setId: 2, uniqueId: null },
    'maximum',
  );
  assert.deepEqual(civerbPatch.magicAttributes, [
    { id: 22, values: [23], name: 'maxdamage' },
    { id: 50, values: [1, 20], name: 'lightmindam' },
    { id: 218, values: [8], name: 'item_maxdamage_perlevel' },
    { id: 224, values: [16], name: 'item_tohit_perlevel' },
  ]);

  const document = await createBlankCharacter({ name: 'SetLists', className: 'Warlock' });
  let editable = editableSnapshot(document.model);
  const cases = [
    {
      type: 'amu',
      x: 0,
      setId: 77,
      bits: [2],
      expectedMask: 4,
      expectedLists: [[{ id: 105, values: [10], name: 'item_fastercastrate' }]],
    },
    {
      type: 'upl',
      x: 2,
      setId: 136,
      bits: [0, 2, 4],
      expectedMask: 21,
      expectedLists: [
        [{ id: 45, values: [25], name: 'poisonresist' }],
        [{ id: 3, values: [15], name: 'vitality' }],
        [{ id: 36, values: [15], name: 'damageresist' }],
      ],
    },
  ];
  for (const entry of cases) {
    editable = addItemBatchSnapshot(editable, document.model.items, {
      type: entry.type,
      containerId: 'stash',
      x: entry.x,
      y: 0,
      count: 1,
      itemLevel: 99,
    });
    const itemIndex = editable.itemEdits.length - 1;
    const item = editable.addedItems.at(-1);
    editable = editItemSnapshot(editable, document.model.items, itemIndex, { quality: 5 });
    editable = editItemSnapshot(editable, document.model.items, itemIndex, { setId: entry.setId });
    editable = editItemSnapshot(
      editable,
      document.model.items,
      itemIndex,
      compileNamedQualityPatch(item, editable.itemEdits[itemIndex], 'maximum'),
    );
    for (const bit of entry.bits) {
      editable = editItemSnapshot(
        editable,
        document.model.items,
        itemIndex,
        compileSetBonusPatch(item, editable.itemEdits[itemIndex], bit, 'maximum'),
      );
    }
  }

  const exported = await exportCharacter(document, editable);
  const setItems = exported.reparsed.items.slice(-cases.length);
  cases.forEach((entry, index) => {
    assert.equal(setItems[index]._unknown_data.plist_flag, entry.expectedMask);
    assert.equal(setItems[index].set_list_count, entry.expectedLists.length);
    assert.deepEqual(setItems[index].set_attributes, entry.expectedLists);
  });
  assert.deepEqual(describeItem(setItems[1], 1).setBonusAttributes, [
    ['Poison Resist +25%'],
    ['+15 to Vitality'],
    ['Physical Damage Received Reduced by 15%'],
  ]);
  const reopened = await openCharacter(exported.bytes, 'SetLists.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, exported.bytes);
});

test('inserts, deletes, and extracts real socket fillers while preserving canonical payloads', async () => {
  const document = await createBlankCharacter({ name: 'SocketFill', className: 'Warlock' });
  let editable = addItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
    type: 'hax',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 55,
  });
  const parentIndex = editable.itemEdits.length - 1;
  editable = editItemSnapshot(editable, document.model.items, parentIndex, {
    socketed: true,
    totalSockets: 2,
  });
  editable = insertSocketFillerSnapshot(editable, document.model.items, {
    parentIndex,
    type: 'r01',
    itemLevel: 55,
  });
  editable = insertSocketFillerSnapshot(editable, document.model.items, {
    parentIndex,
    type: 'jew',
    itemLevel: 55,
  });
  const parentEdit = editable.itemEdits[parentIndex];
  assert.deepEqual(parentEdit.socketedItems.map((item) => ({
    type: item.type,
    location: item.location_id,
    x: item.position_x,
  })), [
    { type: 'r01', location: 6, x: 0 },
    { type: 'jew', location: 6, x: 1 },
  ]);
  const jewelId = parentEdit.socketedItems[1].id;
  assert.ok(Number.isInteger(jewelId));
  assert.throws(
    () => insertSocketFillerSnapshot(editable, document.model.items, {
      parentIndex,
      type: 'r02',
      itemLevel: 55,
    }),
    /no empty sockets/i,
  );
  const filledExport = await exportCharacter(document, editable);
  const filledParent = filledExport.reparsed.items[parentIndex];
  assert.equal(filledParent.nr_of_items_in_sockets, 2);
  assert.equal(filledParent.total_nr_of_sockets, 2);
  assert.deepEqual(filledParent.socketed_items.map((item) => [item.type, item.location_id, item.position_x]), [
    ['r01', 6, 0],
    ['jew', 6, 1],
  ]);
  assert.equal(filledParent.socketed_items[1].id, jewelId);
  const portable = await exportItemRecord(filledParent);
  assert.deepEqual(portable.reparsed.socketed_items.map(({ type }) => type), ['r01', 'jew']);

  const rootCountBeforeDelete = editable.addedItems.length;
  editable = removeSocketFillerSnapshot(editable, document.model.items, {
    parentIndex,
    socketIndex: 0,
  });
  assert.equal(editable.addedItems.length, rootCountBeforeDelete);
  assert.deepEqual(
    editable.itemEdits[parentIndex].socketedItems.map(({ type, position_x: positionX }) => [type, positionX]),
    [['jew', 0]],
  );
  const deletedExport = await exportCharacter(document, editable);
  assert.deepEqual(deletedExport.reparsed.items[parentIndex].socketed_items.map(({ type }) => type), ['jew']);

  editable = extractSocketFillerSnapshot(editable, document.model.items, {
    parentIndex,
    socketIndex: 0,
    containerId: 'inventory',
    x: 2,
    y: 0,
  });
  const extractedExport = await exportCharacter(document, editable);
  const extractedParent = extractedExport.reparsed.items[parentIndex];
  const extractedJewel = extractedExport.reparsed.items.at(-1);
  assert.equal(extractedParent.socketed, 1);
  assert.equal(extractedParent.total_nr_of_sockets, 2);
  assert.equal(extractedParent.nr_of_items_in_sockets, 0);
  assert.equal(extractedParent.socketed_items, undefined);
  assert.deepEqual(
    [extractedJewel.type, extractedJewel.id, extractedJewel.location_id, extractedJewel.position_x, extractedJewel.position_y],
    ['jew', jewelId, 0, 2, 0],
  );
  const reopened = await openCharacter(extractedExport.bytes, 'SocketFill.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, extractedExport.bytes);
});

test('imports complex filler bundles atomically and compacts any extracted socket', async () => {
  const source = await createBlankCharacter({ name: 'SocketSource', className: 'Warlock' });
  let sourceEdit = addItemBatchSnapshot(editableSnapshot(source.model), source.model.items, {
    type: 'jew',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 77,
  });
  const jewelIndex = sourceEdit.itemEdits.length - 1;
  sourceEdit = editItemSnapshot(sourceEdit, source.model.items, jewelIndex, { quality: 4 });
  sourceEdit = editItemSnapshot(sourceEdit, source.model.items, jewelIndex, {
    identified: true,
    magicAttributes: [{ id: 0, values: [7], name: 'strength' }],
  });
  sourceEdit = addItemBatchSnapshot(sourceEdit, source.model.items, {
    type: 'r02',
    containerId: 'stash',
    x: 1,
    y: 0,
    count: 1,
    itemLevel: 77,
  });
  const runeIndex = sourceEdit.itemEdits.length - 1;
  sourceEdit = addItemBatchSnapshot(sourceEdit, source.model.items, {
    type: 'hax',
    containerId: 'stash',
    x: 2,
    y: 0,
    count: 1,
    itemLevel: 77,
  });
  const invalidIndex = sourceEdit.itemEdits.length - 1;
  const sourceExport = await exportCharacter(source, sourceEdit);
  const sourceJewel = sourceExport.reparsed.items[jewelIndex];
  const sourceRune = sourceExport.reparsed.items[runeIndex];
  const invalidFiller = sourceExport.reparsed.items[invalidIndex];
  const bundle = await exportItemBundle([sourceJewel, sourceRune], 'Complex socket fillers');
  const imported = await importItemFiles([memoryFile('Complex-Sockets.bkitems.json', bundle)]);
  assert.deepEqual(imported.map(({ type }) => type), ['jew', 'r02']);

  const target = await createBlankCharacter({ name: 'SocketImport', className: 'Warlock' });
  let editable = addItemBatchSnapshot(editableSnapshot(target.model), target.model.items, {
    type: 'gth',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 77,
  });
  const parentIndex = editable.itemEdits.length - 1;
  editable = editItemSnapshot(editable, target.model.items, parentIndex, {
    socketed: true,
    totalSockets: 3,
  });
  editable = insertSocketFillerSnapshot(editable, target.model.items, {
    parentIndex,
    type: 'r01',
    itemLevel: 77,
  });

  const beforeRejectedImport = structuredClone(editable);
  assert.throws(
    () => insertImportedSocketFillersSnapshot(editable, target.model.items, {
      parentIndex,
      importedItems: [sourceJewel, invalidFiller],
    }),
    /not a BKVince socket filler/i,
  );
  assert.deepEqual(editable, beforeRejectedImport);

  editable = insertImportedSocketFillersSnapshot(editable, target.model.items, {
    parentIndex,
    importedItems: imported,
    reservedItemIds: characterItemIds(target.model),
  });
  const importedEdit = editable.itemEdits[parentIndex];
  assert.deepEqual(importedEdit.socketedItems.map((item) => [
    item.type,
    item.position_x,
    item.quality ?? null,
  ]), [
    ['r01', 0, null],
    ['jew', 1, 4],
    ['r02', 2, null],
  ]);
  assert.deepEqual(importedEdit.socketedItems[1].magic_attributes, [
    { id: 0, values: [7], name: 'strength' },
  ]);
  assert.notEqual(importedEdit.socketedItems[1].id, sourceJewel.id);
  assert.notDeepEqual(importedEdit.socketedItems[1]._unknown_data.realm_data, sourceJewel._unknown_data.realm_data);

  editable = extractSocketFillerSnapshot(editable, target.model.items, {
    parentIndex,
    socketIndex: 1,
    containerId: 'inventory',
    x: 2,
    y: 0,
  });
  const exported = await exportCharacter(target, editable);
  const parent = exported.reparsed.items[parentIndex];
  const extractedJewel = exported.reparsed.items.at(-1);
  assert.deepEqual(parent.socketed_items.map((item) => [item.type, item.position_x]), [
    ['r01', 0],
    ['r02', 1],
  ]);
  assert.equal(parent.nr_of_items_in_sockets, 2);
  assert.deepEqual([
    extractedJewel.type,
    extractedJewel.quality,
    extractedJewel.location_id,
    extractedJewel.position_x,
    extractedJewel.position_y,
  ], ['jew', 4, 0, 2, 0]);
  assert.deepEqual(extractedJewel.magic_attributes, [
    { id: 0, values: [7], name: 'strength' },
  ]);
  const reopened = await openCharacter(exported.bytes, 'SocketImport.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, exported.bytes);
});

test('builds a governed BKVince runeword recipe and reparses its separate D2S payload', async () => {
  const document = await createBlankCharacter({ name: 'RuneForge', className: 'Paladin' });
  let editable = addItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
    type: 'uit',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 85,
  });
  const parentIndex = editable.itemEdits.length - 1;
  editable = applyRunewordSnapshot(editable, document.model.items, {
    parentIndex,
    runewordId: 155,
    rollMode: 'maximum',
  });

  const spiritEdit = editable.itemEdits[parentIndex];
  assert.equal(spiritEdit.runewordId, 155);
  assert.equal(spiritEdit.totalSockets, 4);
  assert.deepEqual(spiritEdit.socketedItems.map(({ type }) => type), ['r07', 'r10', 'r09', 'r11']);
  assert.deepEqual(
    spiritEdit.runewordAttributes.map(({ id, values }) => [id, values]),
    [[3, [22]], [9, [112]], [32, [250]], [99, [25]], [105, [30]], [127, [1]], [147, [8]]],
  );

  const exported = await exportCharacter(document, editable);
  const spirit = exported.reparsed.items.find(({ runeword_id: id }) => id === 155);
  assert.ok(spirit);
  assert.equal(spirit.given_runeword, 1);
  assert.equal(spirit.runeword_name, 'Spirit');
  assert.deepEqual(
    pick(describeItem(spirit, parentIndex), ['name', 'baseName', 'quality']),
    { name: 'Spirit', baseName: 'Monarch', quality: 'Runeword' },
  );
  const spiritDescriptor = describeItem(spirit, parentIndex, 99);
  assert.ok(spiritDescriptor.magicAttributes.includes('+112 to Mana'));
  assert.deepEqual(spiritDescriptor.sockets, { filled: 4, total: 4 });
  assert.deepEqual(spiritDescriptor.socketedItems.map(({ type }) => type), ['r07', 'r10', 'r09', 'r11']);
  assert.deepEqual(spirit.socketed_items.map(({ type }) => type), ['r07', 'r10', 'r09', 'r11']);
  assert.deepEqual(
    spirit.runeword_attributes.map(({ id, values }) => [id, values]),
    spiritEdit.runewordAttributes.map(({ id, values }) => [id, values]),
  );
  const reopened = await openCharacter(exported.bytes, 'RuneForge.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, exported.bytes);

  const exile = itemCatalog.runewords.find(({ id }) => id === 63);
  const exileBase = itemCatalog.bases.find(({ code }) => code === 'pab');
  const exileItem = { type: exileBase.code, simple_item: 0, level: 85, nr_of_items_in_sockets: 0 };
  const exileEdit = { index: 0, type: exileBase.code, quality: 2 };
  assert.equal(exile.name, 'Exile');
  assert.deepEqual(
    compileRunewordPatch(exileItem, exileEdit, 63, 'minimum').runewordAttributes
      .find(({ id }) => id === 16).values,
    [220],
  );
  assert.deepEqual(
    compileRunewordPatch(exileItem, exileEdit, 63, 'maximum').runewordAttributes
      .find(({ id }) => id === 16).values,
    [260],
  );
});

test('serializes Chaos at its maximum native save-compatible durability roll', async () => {
  const runewords = availableRunewordItems();
  assert.equal(runewords.length, itemCatalog.runewords.length);
  runewords.forEach((runeword) => {
    const item = {
      type: runeword.baseCode,
      simple_item: 0,
      level: 99,
      nr_of_items_in_sockets: 0,
    };
    const edit = { index: 0, type: runeword.baseCode, quality: 2 };
    assert.doesNotThrow(() => compileRunewordPatch(item, edit, runeword.id, 'minimum'));
    assert.doesNotThrow(() => compileRunewordPatch(item, edit, runeword.id, 'maximum'));
  });

  const chaos = runewords.find(({ id }) => id === 42);
  const document = await createBlankCharacter({ name: 'ChaosForge', className: 'Assassin' });
  let editable = addItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
    type: chaos.baseCode,
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 99,
    quality: 2,
    runewordId: chaos.id,
    rollMode: 'maximum',
  });
  const itemIndex = editable.itemEdits.length - 1;
  const chaosEdit = editable.itemEdits[itemIndex];
  assert.equal(chaosEdit.runewordId, 42);
  assert.equal(chaosEdit.totalSockets, 3);
  assert.deepEqual(chaosEdit.socketedItems.map(({ type }) => type), ['r19', 'r27', 'r22']);
  assert.deepEqual(
    chaosEdit.runewordAttributes.map(({ id, values }) => [id, values]),
    [
      [17, [290, 290]],
      [52, [216, 471]],
      [93, [40]],
      [97, [151, 5]],
      [139, [15]],
      [201, [26, 64, 25]],
      [252, [63]],
    ],
  );
  assert.deepEqual(
    chaosEdit.runewordAttributes.filter(({ id }) => id === 252).map(({ id, values }) => ({ id, values })),
    [{ id: 252, values: [63] }],
  );

  const exported = await exportCharacter(document, editable);
  const reparsedChaos = exported.reparsed.items.find(({ runeword_id: id }) => id === 42);
  assert.ok(reparsedChaos);
  assert.deepEqual(reparsedChaos.socketed_items.map(({ type }) => type), ['r19', 'r27', 'r22']);
  assert.deepEqual(
    reparsedChaos.runeword_attributes.map(({ id, values }) => [id, values]),
    chaosEdit.runewordAttributes.map(({ id, values }) => [id, values]),
  );
  const descriptor = describeItem(reparsedChaos, itemIndex, 99);
  assert.equal(descriptor.name, 'Chaos');
  assert.equal(descriptor.baseName, 'Katar');
  assert.ok(descriptor.magicAttributes.includes('+5 to Whirlwind'));
  assert.ok(descriptor.magicAttributes.includes('Repairs 0.63 durability per second'));

  const reopened = await openCharacter(exported.bytes, 'ChaosForge.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, exported.bytes);
});

test('adds semantic manual Properties.txt declarations with grouped and parameterized encodings', async () => {
  const document = await createBlankCharacter({ name: 'ManualMods', className: 'Barbarian' });
  let editable = addItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
    type: 'hax',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 75,
  });
  const itemIndex = editable.itemEdits.length - 1;
  const item = editable.addedItems[0];
  editable = editItemSnapshot(editable, document.model.items, itemIndex, { quality: 4 });
  const manualProperties = itemEditorOptions(item, editable.itemEdits[itemIndex]).manualProperties;
  assert.deepEqual(
    ['str', 'res-all', 'dmg-elem'].map((code) => {
      const property = manualProperties.find((entry) => entry.code === code);
      return [property.code, property.label, property.attributeCount, property.attributeIds];
    }),
    [
      ['str', '+# to Strength', 1, [0]],
      ['res-all', 'All Resistances +#', 4, [39, 41, 43, 45]],
      ['dmg-elem', 'Adds #-# Fire/Lightning/Cold Damage', 3, [48, 50, 54]],
    ],
  );
  const allResistances = compileManualPropertyPatch(item, editable.itemEdits[itemIndex], {
    propertyCode: 'res-all',
    minimum: '25',
    maximum: '25',
  });
  assert.deepEqual(
    allResistances.magicAttributes.map(({ id, values }) => [id, values]),
    [[39, [25]], [41, [25]], [43, [25]], [45, [25]]],
  );
  assert.deepEqual(
    describeItem({ ...item, magic_attributes: allResistances.magicAttributes }, itemIndex).magicAttributes,
    ['All Resistances +25'],
  );

  const requests = [
    { propertyCode: 'str', minimum: '15', maximum: '15' },
    { propertyCode: 'red-dmg%', minimum: '52', maximum: '52' },
    { propertyCode: 'dmg-fire', minimum: '25', maximum: '50' },
    { propertyCode: 'oskill', parameter: 'Whirlwind', minimum: '3', maximum: '3' },
  ];
  for (const request of requests) {
    const edit = editable.itemEdits[itemIndex];
    editable = editItemSnapshot(
      editable,
      document.model.items,
      itemIndex,
      compileManualPropertyPatch(item, edit, { ...request, rollMode: 'maximum' }),
    );
  }

  assert.deepEqual(
    editable.itemEdits[itemIndex].magicAttributes.map(({ id, values }) => [id, values]),
    [[0, [15]], [36, [52]], [48, [25, 50]], [97, [151, 3]]],
  );
  const randomClass = compileManualPropertyPatch(item, editable.itemEdits[itemIndex], {
    propertyCode: 'randclassskill3',
    parameter: '5',
    minimum: '7',
    maximum: '7',
  }).magicAttributes.find(({ id }) => id === 83);
  assert.deepEqual(randomClass.values, [7, 5]);
  assert.deepEqual(
    describeItem({ ...item, magic_attributes: [randomClass] }, itemIndex).magicAttributes,
    ['+5 to Warlock Skills'],
  );
  const exported = await exportCharacter(document, editable);
  const reparsed = exported.reparsed.items.find(({ type }) => type === 'hax');
  assert.deepEqual(
    reparsed.magic_attributes.map(({ id, values }) => [id, values]),
    [[0, [15]], [36, [52]], [48, [25, 50]], [97, [151, 3]]],
  );
  const reopened = await openCharacter(exported.bytes, 'ManualMods.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
});

test('drives every supported manual property through a semantic form and D2S round-trip', async () => {
  const document = await createBlankCharacter({ name: 'FormMatrix', className: 'Warlock' });
  let editable = addItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
    type: 'hax',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 99,
  });
  const itemIndex = editable.itemEdits.length - 1;
  const item = editable.addedItems[0];
  editable = editItemSnapshot(editable, document.model.items, itemIndex, { quality: 4 });
  const edit = editable.itemEdits[itemIndex];
  const options = itemEditorOptions(item, edit);
  assert.equal(options.manualProperties.length, 236);
  assert.ok(options.manualProperties.every(({ fields }) => Array.isArray(fields)));
  assert.ok(options.manualSelectOptions.skill.some(({ value, label }) => value === 151 && label === 'Whirlwind'));
  assert.ok(options.manualSelectOptions.skill.some(({ value, group }) => value === 373 && group === 'Warlock'));
  assert.ok(options.manualSelectOptions.skillTab.some(({ value, group }) => value === 23 && group === 'Warlock'));
  assert.ok(options.manualSelectOptions.monster.some(({ value, label }) => value === 1 && label === 'Returned'));
  for (const control of ['skill', 'skillTab', 'class', 'monster']) {
    const values = options.manualSelectOptions[control].map(({ value }) => value);
    assert.equal(new Set(values).size, values.length, `${control} options must use unique native IDs.`);
  }
  assert.equal(
    options.manualSelectOptions.monster.find(({ value }) => value === 742)?.label,
    'Goatman / Pit Lord',
  );
  assert.deepEqual(
    options.manualProperties.find(({ code }) => code === 'oskill').fields
      .map(({ label, control, targets }) => [label, control, targets]),
    [
      ['Skill', 'skill', ['parameter']],
      ['Bonus', 'number', ['minimum', 'maximum']],
    ],
  );

  for (const property of options.manualProperties) {
    assert.equal(new Set(property.fields.map(({ id }) => id)).size, property.fields.length);
    const request = {
      propertyCode: property.code,
      parameter: '',
      minimum: '',
      maximum: '',
      rollMode: 'maximum',
    };
    property.fields.forEach((field) => {
      assert.ok(['number', 'skill', 'skillTab', 'class', 'monster'].includes(field.control));
      assert.ok(field.targets.every((target) => ['parameter', 'minimum', 'maximum'].includes(target)));
      let value = field.defaultValue;
      if (field.control === 'skill') value = '151';
      if (field.control === 'skillTab') value = '23';
      if (field.control === 'class') value = '7';
      if (field.control === 'monster') value = '1';
      field.targets.forEach((target) => {
        request[target] = value;
      });
    });
    const patch = compileManualPropertyPatch(item, edit, request);
    const preview = previewManualPropertyPatch(item, edit, request, 99);
    assert.ok(preview.descriptions.length > 0, `${property.code} has no semantic preview.`);
    preview.descriptions.forEach((description) => {
      assert.doesNotMatch(description, /%\d|\[[^\]]*\]|#/, `${property.code} has unresolved display tokens.`);
    });
    assert.deepEqual(preview.patch, patch);
    const candidate = editItemSnapshot(editable, document.model.items, itemIndex, patch);
    const exported = await exportCharacter(document, candidate);
    const reparsed = exported.reparsed.items.find(({ type }) => type === 'hax');
    assert.ok(reparsed, `${property.code} did not survive the D2S round-trip.`);
  }
});

test('edits and adds governed attributes on complex items without forcing Magic quality', async () => {
  const document = await createBlankCharacter({ name: 'NormalMods', className: 'Amazon' });
  let editable = addItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
    type: 'hax',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 45,
  });
  const itemIndex = editable.itemEdits.length - 1;
  const item = editable.addedItems[0];
  assert.equal(editable.itemEdits[itemIndex].quality, 2);
  assert.equal(itemEditorOptions(item, editable.itemEdits[itemIndex]).attributesEditable, true);

  editable = editItemSnapshot(
    editable,
    document.model.items,
    itemIndex,
    compileManualPropertyPatch(item, editable.itemEdits[itemIndex], {
      propertyCode: 'str',
      minimum: '15',
      maximum: '15',
    }),
  );
  editable = editItemSnapshot(editable, document.model.items, itemIndex, {
    magicAttributes: [{ id: 0, values: [21], name: 'strength' }],
  });

  const exported = await exportCharacter(document, editable);
  const reparsed = exported.reparsed.items.find(({ type }) => type === 'hax');
  assert.equal(reparsed.quality, 2);
  assert.deepEqual(reparsed.magic_attributes, [{ id: 0, values: [21], name: 'strength' }]);

  const simpleItem = { type: 'hp1', simple_item: 1 };
  const simpleEdit = { index: 0, type: 'hp1', quality: null, magicAttributes: [] };
  assert.equal(itemEditorOptions(simpleItem, simpleEdit).attributesEditable, false);
  assert.throws(
    () => compileManualPropertyPatch(simpleItem, simpleEdit, {
      propertyCode: 'str',
      minimum: '1',
      maximum: '1',
    }),
    /complex D2S item record/i,
  );
});

test('selects native ring, amulet, charm, and jewel picture variants through D2S round-trip', async () => {
  const document = await createBlankCharacter({ name: 'PictureRing', className: 'Amazon' });
  let editable = addItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
    type: 'rin',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 1,
    itemLevel: 75,
  });
  const itemIndex = editable.itemEdits.length - 1;
  const item = editable.addedItems[0];
  const options = itemEditorOptions(item, editable.itemEdits[itemIndex]);
  assert.deepEqual(options.pictureVariants.map(({ id, picture }) => [id, picture]), [
    [0, 'invrin1'],
    [1, 'invrin2'],
    [2, 'invrin3'],
    [3, 'invrin4'],
    [4, 'invrin5'],
  ]);
  assert.throws(
    () => editItemSnapshot(editable, document.model.items, itemIndex, { pictureId: 5 }),
    /picture id/i,
  );

  editable = editItemSnapshot(editable, document.model.items, itemIndex, { pictureId: 4 });
  const materialized = editableItems(document.model.items, editable.itemEdits, editable.addedItems)[itemIndex];
  assert.equal(describeItem(materialized, itemIndex).visualKey, 'rin@4');

  const exported = await exportCharacter(document, editable);
  const ring = exported.reparsed.items.find(({ type }) => type === 'rin');
  assert.equal(ring.multiple_pictures, 1);
  assert.equal(ring.picture_id, 4);
  const reopened = await openCharacter(exported.bytes, 'PictureRing.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);

  const familyCounts = Object.fromEntries(['amu', 'cm1', 'cm2', 'cm3', 'jew']
    .map((code) => [code, itemCatalog.bases.find((base) => base.code === code).pictures.length]));
  assert.deepEqual(familyCounts, { amu: 3, cm1: 3, cm2: 3, cm3: 3, jew: 6 });
});

test('builds Rare and Crafted names, six affix slots, and governed properties through D2S round-trip', async () => {
  for (const quality of [6, 8]) {
    const document = await createBlankCharacter({
      name: quality === 6 ? 'RareAxe' : 'CraftAxe',
      className: 'Amazon',
    });
    let editable = addItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
      type: 'hax',
      containerId: 'stash',
      x: 0,
      y: 0,
      count: 1,
      itemLevel: 99,
    });
    const itemIndex = editable.itemEdits.length - 1;
    const item = editable.addedItems[0];
    editable = editItemSnapshot(editable, document.model.items, itemIndex, { quality });

    let edit = editable.itemEdits[itemIndex];
    const options = itemEditorOptions(item, edit);
    assert.ok(options.qualities.some(({ id }) => id === 6));
    assert.ok(options.qualities.some(({ id }) => id === 8));
    assert.equal(edit.rareNamePrefixId, 156);
    assert.equal(edit.rareNameSuffixId, 1);
    assert.ok(options.rareAffixPrefixes.some(({ id }) => id === 13));
    assert.ok(options.rareAffixSuffixes.some(({ id }) => id === 12));

    editable = editItemSnapshot(editable, document.model.items, itemIndex, {
      rareAffixIds: [13, 12, null, null, null, null],
    });
    edit = editable.itemEdits[itemIndex];
    editable = editItemSnapshot(
      editable,
      document.model.items,
      itemIndex,
      compileRareAffixPatch(item, edit, 'maximum'),
    );

    const exported = await exportCharacter(document, editable);
    const reparsed = exported.reparsed.items.find(({ type }) => type === 'hax');
    assert.equal(reparsed.quality, quality);
    assert.equal(reparsed.rare_name_id, 156);
    assert.equal(reparsed.rare_name_id2, 1);
    assert.deepEqual(reparsed.magical_name_ids, [13, 12, null, null, null, null]);
    assert.deepEqual(reparsed.magic_attributes.map(({ id }) => id), [17, 120]);
    assert.equal(describeItem(reparsed, itemIndex).name, 'Beast Bite');

    const reopened = await openCharacter(exported.bytes, `${quality === 6 ? 'Rare' : 'Crafted'}Axe.d2s`);
    const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
    assert.equal(noOp.byteExact, true);
  }
});

test('rebuilds every governed Low quality variant and preserves its native 3-bit ID', async () => {
  const document = await createBlankCharacter({ name: 'LowAxes', className: 'Amazon' });
  let editable = addItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
    type: 'hax',
    containerId: 'stash',
    x: 0,
    y: 0,
    count: 4,
    itemLevel: 35,
  });
  const firstIndex = document.model.items.length;
  for (let variant = 0; variant < 4; variant += 1) {
    const itemIndex = firstIndex + variant;
    editable = editItemSnapshot(editable, document.model.items, itemIndex, { quality: 1 });
    editable = editItemSnapshot(editable, document.model.items, itemIndex, { lowQualityId: variant });
  }

  const exported = await exportCharacter(document, editable);
  const axes = exported.reparsed.items.filter(({ type }) => type === 'hax');
  assert.deepEqual(axes.map(({ quality, low_quality_id }) => ({ quality, low_quality_id })), [
    { quality: 1, low_quality_id: 0 },
    { quality: 1, low_quality_id: 1 },
    { quality: 1, low_quality_id: 2 },
    { quality: 1, low_quality_id: 3 },
  ]);
  assert.deepEqual(
    axes.map((item, index) => describeItem(item, index).name),
    ['Crude Hand Axe', 'Cracked Hand Axe', 'Damaged Hand Axe', 'Low Quality Hand Axe'],
  );
  const reopened = await openCharacter(exported.bytes, 'LowAxes.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
});

test('compiles every governed Set item and the encodable BKVince Unique catalog fail-closed', () => {
  const catalogs = [
    { quality: 5, entries: itemCatalog.setItems, idKey: 'setId' },
    { quality: 7, entries: itemCatalog.uniqueItems, idKey: 'uniqueId' },
  ];
  const results = catalogs.map(({ quality, entries, idKey }) => {
    const governed = entries.filter(({ baseCode, disabled, spawnable }) => baseCode && !disabled && spawnable);
    const failures = [];
    governed.forEach((entry) => {
      const item = { type: entry.baseCode, level: 99, nr_of_items_in_sockets: 0 };
      const edit = {
        index: 0,
        type: entry.baseCode,
        quality,
        setId: null,
        uniqueId: null,
        [idKey]: entry.id,
      };
      try {
        compileNamedQualityPatch(item, edit, 'minimum');
        compileNamedQualityPatch(item, edit, 'maximum');
      } catch (error) {
        failures.push({ id: entry.id, reason: error.message });
      }
    });
    return { total: governed.length, compiled: governed.length - failures.length, failures };
  });

  assert.deepEqual(results[0], { total: 215, compiled: 215, failures: [] });
  assert.equal(results[1].total, 473);
  assert.equal(results[1].compiled, 473);
  assert.deepEqual(results[1].failures, []);
});

test('writes runtime-canonical uncommon named properties and partial elemental groups', async () => {
  const document = await createBlankCharacter({ name: 'NamedEdges', className: 'Warlock' });
  let editable = editableSnapshot(document.model);
  const cases = [
    { id: 60, type: 'hbw', x: 0 },
    { id: 224, type: 'xuc', x: 2 },
    { id: 298, type: '7pa', x: 4 },
    { id: 473, type: 'jew', x: 8 },
  ];

  for (const entry of cases) {
    editable = addItemBatchSnapshot(editable, document.model.items, {
      type: entry.type,
      containerId: 'stash',
      x: entry.x,
      y: 0,
      count: 1,
      itemLevel: 99,
    });
    const itemIndex = editable.itemEdits.length - 1;
    const item = editable.addedItems.at(-1);
    editable = editItemSnapshot(editable, document.model.items, itemIndex, { quality: 7 });
    editable = editItemSnapshot(editable, document.model.items, itemIndex, { uniqueId: entry.id });
    const patch = compileNamedQualityPatch(item, editable.itemEdits[itemIndex], 'maximum');
    editable = editItemSnapshot(editable, document.model.items, itemIndex, patch);
  }

  const exported = await exportCharacter(document, editable);
  const [witherstring, visceratuant, tombReaver, titansEcho] = exported.reparsed.items.slice(-cases.length);
  assert.deepEqual(witherstring.magic_attributes.find(({ id }) => id === 54).values, [50, 50, 0]);
  assert.deepEqual(visceratuant.magic_attributes.find(({ id }) => id === 59).values, [100]);
  assert.deepEqual(tombReaver.magic_attributes.find(({ id }) => id === 151).values, [122, 18]);
  assert.deepEqual(tombReaver.magic_attributes.find(({ id }) => id === 155).values, [1, 10]);
  assert.deepEqual(titansEcho.magic_attributes, [
    { id: 384, values: [430, 100], name: 'item_splashonhit' },
  ]);

  const reopened = await openCharacter(exported.bytes, 'NamedEdges.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, exported.bytes);

});

test('serializes every recovered Unique edge at its perfect representable roll', async () => {
  const document = await createBlankCharacter({ name: 'UniqueEdges', className: 'Warlock' });
  const recoveredIds = [53, 95, 167, 255, 256, 301, 340, 455, 471];
  const selections = recoveredIds.map((id) => {
    const entry = itemCatalog.uniqueItems.find((candidate) => candidate.id === id);
    return {
      type: entry.baseCode,
      count: 1,
      itemLevel: 99,
      quality: 7,
      uniqueId: id,
      rollMode: 'maximum',
    };
  });
  const editable = addCatalogItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
    selections,
    containerId: 'stash',
    x: 0,
    y: 0,
    reservedItemIds: characterItemIds(document.model),
  });
  const exported = await exportCharacter(document, editable);
  const byId = new Map(exported.reparsed.items
    .filter(({ unique_id: id }) => recoveredIds.includes(id))
    .map((item) => [item.unique_id, item]));
  const values = (uniqueId, statId) => byId.get(uniqueId).magic_attributes
    .filter(({ id }) => id === statId)
    .map((attribute) => attribute.values);

  assert.deepEqual(values(53, 7), [[-32]]);
  assert.deepEqual(values(95, 214), [[63]]);
  assert.deepEqual(values(167, 20), [[63]]);
  assert.deepEqual(values(255, 117), [[1]]);
  assert.deepEqual(values(256, 54), [[1, 500, 255]]);
  assert.deepEqual(values(301, 54), [[250, 500, 255]]);
  assert.deepEqual(values(340, 54), [[300, 420, 255]]);
  assert.deepEqual(values(455, 54), [[300, 300, 255]]);
  assert.equal(byId.get(471).socketed, 1);
  assert.equal(byId.get(471).total_nr_of_sockets, 1);

  const reopened = await openCharacter(exported.bytes, 'UniqueEdges.d2s');
  const noOp = await exportCharacter(reopened, editableSnapshot(reopened.model));
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, exported.bytes);
});

test('builds and re-identifies every exclusive Wraithstep and Opalvein variant', async () => {
  const document = await createBlankCharacter({ name: 'NamedVariants', className: 'Warlock' });
  const plans = [
    { uniqueId: 413, variantId: 'chaos', statId: 188, values: [2, 7, 1] },
    { uniqueId: 413, variantId: 'demon', statId: 188, values: [0, 7, 1] },
    { uniqueId: 413, variantId: 'eldritch', statId: 188, values: [1, 7, 1] },
    { uniqueId: 416, variantId: 'magic', statId: 357, values: [5] },
    { uniqueId: 416, variantId: 'enhanced-damage', statId: 17, values: [40, 40] },
    { uniqueId: 416, variantId: 'fire', statId: 329, values: [5] },
    { uniqueId: 416, variantId: 'cold', statId: 331, values: [5] },
    { uniqueId: 416, variantId: 'lightning', statId: 330, values: [5] },
    { uniqueId: 416, variantId: 'poison', statId: 332, values: [5] },
  ];
  const selections = plans.map(({ uniqueId }) => {
    const entry = itemCatalog.uniqueItems.find(({ id }) => id === uniqueId);
    return {
      type: entry.baseCode,
      count: 1,
      itemLevel: 99,
      quality: 7,
      uniqueId,
      rollMode: 'maximum',
    };
  });
  let editable = addCatalogItemBatchSnapshot(editableSnapshot(document.model), document.model.items, {
    selections,
    containerId: 'stash',
    x: 0,
    y: 0,
    reservedItemIds: characterItemIds(document.model),
  });
  const firstIndex = document.model.items.length;
  const initialRecords = itemRecords(document.model.items, editable.addedItems);
  assert.equal(
    itemEditorOptions(initialRecords[firstIndex], editable.itemEdits[firstIndex]).namedQualityVariants.selectedId,
    'chaos',
  );
  assert.equal(
    itemEditorOptions(initialRecords[firstIndex + 3], editable.itemEdits[firstIndex + 3])
      .namedQualityVariants.selectedId,
    'magic',
  );

  plans.forEach((plan, offset) => {
    const index = firstIndex + offset;
    const records = itemRecords(document.model.items, editable.addedItems);
    const patch = compileNamedQualityPatch(records[index], editable.itemEdits[index], 'maximum', plan.variantId);
    editable = editItemSnapshot(editable, document.model.items, index, patch);
  });

  const wraithstep = availableNamedItems().find(({ kind, id }) => kind === 'unique' && id === 413);
  const opalvein = availableNamedItems().find(({ kind, id }) => kind === 'unique' && id === 416);
  assert.ok(wraithstep.searchTerms.some((term) => /Demon Skills/i.test(term)));
  assert.ok(wraithstep.searchTerms.some((term) => /Eldritch Skills/i.test(term)));
  assert.ok(opalvein.searchTerms.some((term) => /Enhanced Damage/i.test(term)));
  assert.ok(opalvein.searchTerms.some((term) => /Poison Skill Damage/i.test(term)));

  const exported = await exportCharacter(document, editable);
  const variants = exported.reparsed.items.slice(-plans.length);
  plans.forEach((plan, index) => {
    assert.equal(variants[index].unique_id, plan.uniqueId);
    assert.deepEqual(
      variants[index].magic_attributes.find(({ id }) => id === plan.statId)?.values,
      plan.values,
    );
    const exclusiveIds = plan.uniqueId === 413
      ? [188]
      : [17, 329, 330, 331, 332, 357];
    assert.deepEqual(
      variants[index].magic_attributes.filter(({ id }) => exclusiveIds.includes(id)).map(({ id }) => id),
      [plan.statId],
    );
  });

  const reopened = await openCharacter(exported.bytes, 'NamedVariants.d2s');
  const reopenedEditable = editableSnapshot(reopened.model);
  plans.forEach((plan, offset) => {
    const index = reopened.model.items.length - plans.length + offset;
    const options = itemEditorOptions(reopened.model.items[index], reopenedEditable.itemEdits[index]);
    assert.equal(options.namedQualityVariants.selectedId, plan.variantId);
  });
  const noOp = await exportCharacter(reopened, reopenedEditable);
  assert.equal(noOp.byteExact, true);
  assert.deepEqual(noOp.bytes, exported.bytes);
});

test('keeps native ethereal Magic durability byte-exact when reading one .d2i', async () => {
  const [item] = await importItemFiles([memoryFile('Victorious-Bow.d2i', NATIVE_MAGIC_D2I_FIXTURE)]);
  assert.deepEqual(
    pick(item, [
      'type', 'quality', 'ethereal', 'identified', 'id', 'level', 'magic_prefix',
      'magic_suffix', 'max_durability', 'current_durability', 'timestamp',
    ]),
    {
      type: 'cbw',
      quality: 4,
      ethereal: 1,
      identified: 1,
      id: 537362040,
      level: 99,
      magic_prefix: 418,
      magic_suffix: 237,
      max_durability: 28,
      current_durability: 28,
      timestamp: 1,
    },
  );
  assert.deepEqual(item._unknown_data.realm_data, [2583563135, 2202861498, 263637133, 1924549150]);
  assert.deepEqual(item.magic_attributes, [
    { id: 57, values: [171, 171, 150], name: 'poisonmindam' },
    { id: 138, values: [5], name: 'item_manaafterkill' },
  ]);
  const target = await createBlankCharacter({ name: 'NativeAffix', className: 'Warlock' });
  const editable = addImportedItemsSnapshot(editableSnapshot(target.model), target.model.items, {
    importedItems: [item],
    containerId: 'stash',
    x: 0,
    y: 0,
  });
  const importedItem = editable.addedItems.at(-1);
  const importedEdit = editable.itemEdits.at(-1);
  assert.deepEqual(compileMagicAffixAttributes(importedItem, importedEdit, 'minimum'), [
    { id: 57, values: [171, 171, 150], name: 'poisonmindam' },
    { id: 138, values: [2], name: 'item_manaafterkill' },
  ]);
  assert.deepEqual(
    compileMagicAffixAttributes(importedItem, importedEdit, 'maximum'),
    importedItem.magic_attributes,
  );
  const exported = await exportItemRecord(item);
  assert.deepEqual(exported.bytes, new Uint8Array(NATIVE_MAGIC_D2I_FIXTURE));
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

function pick(value, keys) {
  return Object.fromEntries(keys.map((key) => [key, value[key]]));
}

function memoryFile(name, bytes) {
  const copy = new Uint8Array(bytes);
  return {
    name,
    async arrayBuffer() {
      return copy.buffer.slice(copy.byteOffset, copy.byteOffset + copy.byteLength);
    },
  };
}

function table(headers, rows = []) {
  return { headers, rows, eol: '\r\n', hasFinalEol: true };
}

function baseTable(code, source) {
  const headers = [
    'name', 'code', 'compactsave', 'spawnable', 'level', 'levelreq', 'durability',
    'invwidth', 'invheight', 'stackable', 'maxstack', 'type',
    'normcode', 'ubercode', 'ultracode', 'minac', 'maxac', 'belt',
  ];
  const itemType = { Armor: 'armo', Weapons: 'weap', Misc: 'misc' }[source];
  return table(headers, [[
    source, code, '0', '1', '1', '0', '10', '1', '1', '0', '0', itemType,
    code, code, code, source === 'Armor' ? '10' : '', source === 'Armor' ? '20' : '', '',
  ]]);
}

function affixTable(name, allowedType) {
  const headers = [
    'Name', 'spawnable', 'rare', 'level', 'levelreq', 'group', 'classspecific', 'class',
    'mod1code', 'mod1param', 'mod1min', 'mod1max',
    'mod2code', 'mod2param', 'mod2min', 'mod2max',
    'mod3code', 'mod3param', 'mod3min', 'mod3max',
    'itype1', 'itype2', 'itype3', 'itype4', 'itype5', 'itype6', 'itype7',
    'etype1', 'etype2', 'etype3', 'etype4', 'etype5',
  ];
  const values = {
    Name: name,
    spawnable: '1',
    rare: '1',
    level: '1',
    levelreq: '1',
    group: '1',
    mod1code: 'dmg',
    mod1min: '1',
    mod1max: '2',
    itype1: allowedType,
  };
  return table(headers, [headers.map((header) => values[header] ?? '')]);
}

function catalogFixture() {
  const constantsFixture = {
    armor_items: { a01: { n: 'Armor', i: 'armor', c: ['Any Armor'] } },
    weapon_items: { w01: { n: 'Weapon', i: 'weapon', c: ['Weapon'] } },
    other_items: { m01: { n: 'Misc', i: 'misc', c: ['Miscellaneous'] } },
    magic_prefixes: [null, { n: 'Strong' }],
    magic_suffixes: [null, { n: 'of Tests' }],
    rare_names: [null, { n: 'Bite' }, { n: 'Beast' }],
    magical_properties: [{ s: 'strength', sB: 8, sA: 32, np: 1 }],
    skills: [null, { s: 'Test Skill' }],
    runewords: Array.from({ length: 28 }, (_, id) => (id === 27 ? { n: 'Test Word' } : null)),
  };
  return {
    constants: constantsFixture,
    tables: {
      Armor: baseTable('a01', 'Armor'),
      Weapons: baseTable('w01', 'Weapons'),
      Misc: baseTable('m01', 'Misc'),
      UniqueItems: namedQualityTable('Unique Armor', 'code', 'a01', 12),
      SetItems: namedQualityTable('Set Armor', 'item', 'a01', 9),
      Runes: runewordTable(),
      skills: table(['skill', '*Id'], [['Test Skill Internal', '1']]),
      MagicPrefix: affixTable('Strong', 'armo'),
      MagicSuffix: affixTable('of Tests', 'armo'),
      RarePrefix: table(
        ['name', 'itype1', 'itype2', 'itype3', 'itype4', 'itype5', 'itype6', 'itype7', 'etype1', 'etype2', 'etype3', 'etype4'],
        [['Beast', 'armo', '', '', '', '', '', '', '', '', '', '']],
      ),
      RareSuffix: table(
        ['name', 'itype1', 'itype2', 'itype3', 'itype4', 'itype5', 'itype6', 'itype7', 'etype1', 'etype2', 'etype3', 'etype4'],
        [['bite', 'armo', '', '', '', '', '', '', '', '', '', '']],
      ),
      LowQualityItems: table(['Name'], [['Crude'], ['Cracked'], ['Damaged'], ['Low Quality']]),
      Properties: propertyTable('dmg', 1, 'strength'),
      ItemTypes: table(
        ['ItemType', 'Code', 'Equiv1', 'Equiv2', 'Body', 'BodyLoc1', 'BodyLoc2'],
        [
          ['Armor', 'armo', '', '', '1', 'tors', 'tors'],
          ['Weapon', 'weap', '', '', '1', 'rarm', 'larm'],
          ['Misc', 'misc', '', '', '0', '', ''],
        ],
      ),
    },
  };
}

function demonCatalogFixture() {
  const localization = JSON.stringify([
    { Key: 'TestDemonName', enUS: 'Localized Demon' },
    { Key: 'TestUniqueName', enUS: 'Localized Unique' },
  ]);
  return {
    buffers: { 'monsters.json': localization },
    tables: {
      MonStats: table(
        ['Id', '*hcIdx', 'NameStr', 'enabled', 'killable', 'npc', 'boss', 'primeevil', 'CannotDesecrate'],
        [['testdemon', '42', 'TestDemonName', '1', '1', '0', '0', '0', '0']],
      ),
      SuperUniques: table(
        ['Superunique', 'Name', 'Class', 'hcIdx'],
        [['TestUnique', 'TestUniqueName', 'testdemon', '9']],
      ),
      MonUMod: table(
        ['uniquemod', 'id', 'enabled', 'xfer', 'champion'],
        [['strong', '5', '1', '1', '0']],
      ),
    },
  };
}

function nativeDemonFixture() {
  const bytes = (length, seed) => Array.from({ length }, (_, index) => (seed + index) & 0xff);
  return {
    _unknown_data: {
      b1_4: bytes(3, 1),
      b9_15: bytes(6, 9),
      b17_28: bytes(11, 17),
      b31_32: bytes(2, 31),
      b35_57: bytes(22, 35),
      b59_61: bytes(3, 59),
      b63_86: bytes(23, 63),
    },
    isSuperUnique: 0,
    index: 1,
    difficulty: 0,
    levelId: 1,
    level: 12,
    isDesecrated: 0,
    difficulty2: 0,
    difficulty3: 0,
    mods: [5, 6, 9, 17, 18, 25, 0, 0, 0],
    stats: [0x15, 0x2a, 0x7f, 0x80, 0xff],
  };
}

function runewordTable() {
  const headers = [
    'Name', '*Rune Name', 'complete', 'disallowCraftingInLadder',
    'disallowCraftingInNonLadder', 'firstLadderSeason', 'lastLadderSeason',
    '*Patch Release',
    ...Array.from({ length: 6 }, (_, index) => `itype${index + 1}`),
    ...Array.from({ length: 3 }, (_, index) => `etype${index + 1}`),
    '*RunesUsed',
    ...Array.from({ length: 6 }, (_, index) => `Rune${index + 1}`),
    ...Array.from({ length: 7 }, (_, index) => index + 1)
      .flatMap((slot) => [`T1Code${slot}`, `T1Param${slot}`, `T1Min${slot}`, `T1Max${slot}`]),
  ];
  const values = {
    Name: 'Runeword1',
    '*Rune Name': 'Test Word',
    complete: '1',
    itype1: 'armo',
    Rune1: 'm01',
    T1Code1: 'dmg',
    T1Min1: '1',
    T1Max1: '2',
  };
  return table(headers, [headers.map((header) => values[header] ?? '')]);
}

function namedQualityTable(name, codeHeader, code, slots) {
  const setHeaders = codeHeader === 'item'
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
  const headers = [
    'index', '*ID', codeHeader, 'disabled', 'spawnable', 'lvl', 'lvl req',
    ...Array.from({ length: slots }, (_, index) => index + 1)
      .flatMap((slot) => [`prop${slot}`, `par${slot}`, `min${slot}`, `max${slot}`]),
    ...setHeaders,
  ];
  const values = {
    index: name,
    '*ID': '0',
    [codeHeader]: code,
    disabled: '0',
    spawnable: '1',
    lvl: '1',
    'lvl req': '1',
    set: 'Test Set',
    'add func': '2',
    prop1: 'dmg',
    min1: '1',
    max1: '2',
  };
  return table(headers, [headers.map((header) => values[header] ?? '')]);
}

function propertyTable(code, functionId, statName) {
  const headers = [
    'code', '*Id',
    ...Array.from({ length: 7 }, (_, index) => index + 1)
      .flatMap((slot) => [`func${slot}`, `stat${slot}`, `set${slot}`, `val${slot}`]),
    'uiRangeType', '*Tooltip', '*Parameter', '*Min', '*Max', '*Notes',
  ];
  const values = {
    code,
    '*Id': '0',
    func1: String(functionId),
    stat1: statName,
    '*Tooltip': 'Test property',
  };
  return table(headers, [headers.map((header) => values[header] ?? '')]);
}
