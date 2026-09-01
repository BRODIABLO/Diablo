import assert from 'node:assert/strict';
import test from 'node:test';

import attributesModule from '@d2runewizard/d2s/lib/d2/attributes.js';
import bitreaderModule from '@d2runewizard/d2s/lib/binary/bitreader.js';
import bkvinceConstants from '../../../apps/hero-editor/src/data/bkvince-constants.generated.js';
import { createBlankCharacter } from '../../../apps/hero-editor/src/lib/character-codec.js';

import {
  SaveConversionBlockedError,
  collectCharacterDowngradeBlockers,
  collectItemDowngradeBlockers,
  transcodeCharacterSave,
  transcodeItemRecord,
  validateD2sEnvelope,
} from './runewizard-bridge.mjs';
import {
  ISC12_STAT_ID_BITS,
  LEGACY_STAT_ID_BITS,
} from './stat-stream.mjs';

const NATIVE_MAGIC_D2I_FIXTURE = Buffer.from(
  'EADAAAXIRCgIeH4HIGNCNO34t+CfqfvPNNiIbPvgoWQrx8FBjlVWsaQopv8A',
  'base64',
);

const { readAttributes, writeAttributes } = attributesModule;
const { BitReader } = bitreaderModule;

test('keeps the default RuneWizard item codec byte-exact in legacy mode', async () => {
  const { readItem, writeItem } = await import('@d2runewizard/d2s');
  const item = await readItem(
    NATIVE_MAGIC_D2I_FIXTURE,
    0x69,
    bkvinceConstants,
    { disableItemEnhancements: true, sortProperties: false },
  );
  const rewritten = new Uint8Array(await writeItem(
    item,
    0x69,
    bkvinceConstants,
    { disableItemEnhancements: true, sortProperties: false },
  ));
  assert.deepEqual(rewritten, new Uint8Array(NATIVE_MAGIC_D2I_FIXTURE));
});

test('transcodes a native v105 item 9 to 12 to 9 byte-exact', async () => {
  const upgraded = await transcodeItemRecord({
    input: new Uint8Array(NATIVE_MAGIC_D2I_FIXTURE),
    constants: bkvinceConstants,
    sourceWidth: LEGACY_STAT_ID_BITS,
    targetWidth: ISC12_STAT_ID_BITS,
    scope: 'Victorious Bow',
  });
  assert.deepEqual(upgraded.reparsed.magic_attributes.map(({ id }) => id), [57, 138]);
  assert.ok(upgraded.bytes.length > NATIVE_MAGIC_D2I_FIXTURE.length);

  const downgraded = await transcodeItemRecord({
    input: upgraded.bytes,
    constants: bkvinceConstants,
    sourceWidth: ISC12_STAT_ID_BITS,
    targetWidth: LEGACY_STAT_ID_BITS,
    scope: 'Victorious Bow',
  });
  assert.deepEqual(downgraded.bytes, new Uint8Array(NATIVE_MAGIC_D2I_FIXTURE));
});

test('refuses downgrade and reports a high stat inside a socket path', () => {
  const item = {
    magic_attributes: [],
    set_attributes: [],
    runeword_attributes: [],
    socketed_items: [{
      magic_attributes: [{ id: 2013, values: [7] }],
      socketed_items: [],
    }],
  };
  assert.deepEqual(collectItemDowngradeBlockers(item, 'Mercenary > Weapon'), [{
    id: 2013,
    path: 'Mercenary > Weapon > Socket 1 > Magic',
    propertyIndex: 0,
  }]);
});

test('throws one public blocker error instead of deleting a high stat', async () => {
  const constants = structuredClone(bkvinceConstants);
  constants.magical_properties[2013] = {
    s: 'isc12_test_2013',
    sB: 6,
    sA: 0,
    sP: 0,
    np: 1,
  };

  const source = await transcodeItemRecord({
    input: new Uint8Array(NATIVE_MAGIC_D2I_FIXTURE),
    constants,
    sourceWidth: LEGACY_STAT_ID_BITS,
    targetWidth: ISC12_STAT_ID_BITS,
  });
  source.reparsed.magic_attributes.push({
    id: 2013,
    values: [7],
    name: 'isc12_test_2013',
  });

  const { writeItem } = await import('@d2runewizard/d2s');
  const highBytes = new Uint8Array(await writeItem(
    source.reparsed,
    0x69,
    constants,
    {
      disableItemEnhancements: true,
      preserveRawAttributes: true,
      sortProperties: false,
      statIdBits: ISC12_STAT_ID_BITS,
    },
  ));

  await assert.rejects(
    () => transcodeItemRecord({
      input: highBytes,
      constants,
      sourceWidth: ISC12_STAT_ID_BITS,
      targetWidth: LEGACY_STAT_ID_BITS,
      scope: 'Shared Stash > Page 3 > Item 4',
    }),
    (error) => {
      assert.ok(error instanceof SaveConversionBlockedError);
      assert.deepEqual(error.blockers, [{
        id: 2013,
        path: 'Shared Stash > Page 3 > Item 4 > Magic',
        propertyIndex: 2,
      }]);
      return true;
    },
  );
});

test('preserves raw player stat entries and refuses a high-ID player downgrade', async () => {
  const constants = { magical_properties: [] };
  constants.magical_properties[6] = { s: 'hitpoints', cB: 21 };
  constants.magical_properties[700] = { s: 'isc12_player_test', cB: 7 };
  const original = {
    attributes: {},
    _attributes_header_present: true,
    _raw_attributes: [
      { id: 6, value: 12345 },
      { id: 700, value: 77 },
    ],
  };
  const encoded = await writeAttributes(original, constants, {
    preserveRawAttributes: true,
    statIdBits: ISC12_STAT_ID_BITS,
  });
  const reparsed = { header: { level: 99 } };
  await readAttributes(
    reparsed,
    new BitReader(encoded),
    constants,
    { preserveRawAttributes: true, statIdBits: ISC12_STAT_ID_BITS },
  );
  assert.deepEqual(reparsed._raw_attributes, original._raw_attributes);
  assert.equal(reparsed.attributes.current_hp, 12345 >>> 8);

  await assert.rejects(
    () => writeAttributes(reparsed, constants, {
      preserveRawAttributes: true,
      statIdBits: LEGACY_STAT_ID_BITS,
    }),
    /Attribute id 700 cannot be represented with 9 bits/,
  );
});

test('transcodes a complete v105 character 9 to 12 to 9 byte-exact', async () => {
  const document = await createBlankCharacter({ name: 'ISCCodec', className: 'Amazon' });
  const upgraded = await transcodeCharacterSave({
    input: document.sourceBytes,
    constants: bkvinceConstants,
    sourceWidth: LEGACY_STAT_ID_BITS,
    targetWidth: ISC12_STAT_ID_BITS,
    scope: 'ISCCodec.d2s',
  });
  assert.ok(upgraded.bytes.length > document.sourceBytes.length);
  assert.equal(upgraded.reparsed.skills.length, 30);
  assert.equal(upgraded.reparsed.items.length, 10);
  assert.equal(upgraded.reparsed._raw_attributes.length, 11);
  validateD2sEnvelope(upgraded.bytes);

  const downgraded = await transcodeCharacterSave({
    input: upgraded.bytes,
    constants: bkvinceConstants,
    sourceWidth: ISC12_STAT_ID_BITS,
    targetWidth: LEGACY_STAT_ID_BITS,
    scope: 'ISCCodec.d2s',
  });
  assert.deepEqual(downgraded.bytes, document.sourceBytes);
});

test('preserves v105 raw header bytes and a mod-extended skill payload byte-exact', async () => {
  const document = await createBlankCharacter({ name: 'ISCRaw', className: 'Amazon' });
  const source = addV105OpaqueFields(document.sourceBytes, [0x00, 0x12, 0x34]);
  const upgraded = await transcodeCharacterSave({
    input: source,
    constants: bkvinceConstants,
    sourceWidth: LEGACY_STAT_ID_BITS,
    targetWidth: ISC12_STAT_ID_BITS,
    scope: 'ISCRaw.d2s',
  });
  assert.deepEqual(upgraded.model._raw_skill_tail, [0x00, 0x12, 0x34]);
  assert.equal(upgraded.model.items.length, 10);

  const downgraded = await transcodeCharacterSave({
    input: upgraded.bytes,
    constants: bkvinceConstants,
    sourceWidth: ISC12_STAT_ID_BITS,
    targetWidth: LEGACY_STAT_ID_BITS,
    scope: 'ISCRaw.d2s',
  });
  assert.deepEqual(downgraded.bytes, source);
});

test('collects player, corpse, mercenary, golem and socket blockers without mutation', () => {
  const high = (id) => ({ magic_attributes: [{ id }], socketed_items: [] });
  const model = {
    _raw_attributes: [{ id: 700, value: 1 }],
    items: [{ magic_attributes: [], socketed_items: [high(800)] }],
    corpse_items: [high(900)],
    merc_items: [high(1000)],
    golem_item: high(2013),
  };
  const snapshot = structuredClone(model);
  assert.deepEqual(collectCharacterDowngradeBlockers(model, 'Hero.d2s'), [
    { id: 700, path: 'Hero.d2s > Player stats', propertyIndex: 0 },
    { id: 800, path: 'Hero.d2s > Items > Item 1 > Socket 1 > Magic', propertyIndex: 0 },
    { id: 900, path: 'Hero.d2s > Corpse > Item 1 > Magic', propertyIndex: 0 },
    { id: 1000, path: 'Hero.d2s > Mercenary > Item 1 > Magic', propertyIndex: 0 },
    { id: 2013, path: 'Hero.d2s > Iron Golem > Magic', propertyIndex: 0 },
  ]);
  assert.deepEqual(model, snapshot);
});

test('rejects a corrupt D2S before conversion', async () => {
  const document = await createBlankCharacter({ name: 'ISCBad', className: 'Amazon' });
  const corrupt = new Uint8Array(document.sourceBytes);
  corrupt[corrupt.length - 1] ^= 0x01;
  await assert.rejects(
    () => transcodeCharacterSave({
      input: corrupt,
      constants: bkvinceConstants,
      sourceWidth: LEGACY_STAT_ID_BITS,
      targetWidth: ISC12_STAT_ID_BITS,
    }),
    /D2S checksum mismatch/,
  );
});

function addV105OpaqueFields(sourceBytes, skillTail) {
  const source = new Uint8Array(sourceBytes);
  const skillsHeader = findAscii(source, 'if', 800);
  assert.notEqual(skillsHeader, -1);
  const itemListOffset = skillsHeader + 2 + 30;
  assert.equal(new TextDecoder().decode(source.slice(itemListOffset, itemListOffset + 2)), 'JM');
  const expanded = new Uint8Array(source.length + skillTail.length);
  expanded.set(source.slice(0, itemListOffset), 0);
  expanded.set(skillTail, itemListOffset);
  expanded.set(source.slice(itemListOffset), itemListOffset + skillTail.length);
  expanded[0x1a] = 0x1f;
  expanded[0x103] = 0x04;
  expanded[0x10f] = 0x04;
  fixD2sEnvelope(expanded);
  return expanded;
}

function findAscii(bytes, value, start = 0) {
  const needle = new TextEncoder().encode(value);
  for (let offset = start; offset <= bytes.length - needle.length; offset += 1) {
    if (needle.every((byte, index) => bytes[offset + index] === byte)) return offset;
  }
  return -1;
}

function fixD2sEnvelope(bytes) {
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  view.setUint32(8, bytes.length, true);
  view.setUint32(12, 0, true);
  let checksum = 0;
  for (const byte of bytes) {
    const carry = (checksum & 0x80000000) !== 0 ? 1 : 0;
    checksum = (byte + carry + checksum * 2) >>> 0;
  }
  view.setUint32(12, checksum, true);
}
