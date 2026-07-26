import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import {
  read,
  readConstantData,
  readItem,
  write,
  writeItem,
} from '@d2runewizard/d2s';
import tsv from '../build-data/tsv.js';

const { ENCODING, parseTable, serializeTable } = tsv;

const SCRIPT_DIR = path.dirname(fileURLToPath(import.meta.url));
const REPOSITORY_ROOT = path.resolve(SCRIPT_DIR, '..', '..');
const EXCEL_ROOT = path.join(
  REPOSITORY_ROOT,
  'data-BKVince',
  'BKVince.mpq',
  'data',
  'global',
  'excel',
);
const STRINGS_ROOT = path.join(
  REPOSITORY_ROOT,
  'data-BKVince',
  'BKVince.mpq',
  'data',
  'local',
  'lng',
  'strings',
);

const TABLE_NAMES = Object.freeze([
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
]);
const STRING_NAMES = Object.freeze([
  'item-gems',
  'item-modifiers',
  'item-nameaffixes',
  'item-names',
  'item-runes',
  'skills',
]);

export const DEFAULT_STAT_COUNT = 120;
export const PERSONAL_STASH_ALT_POSITION_ID = 5;
export const ITEM_FORMAT_VERSION = 105;
export const NETWORK_ITEM_BUFFER_BYTES = 0xf4;
export const NETWORK_PACKET_BYTES = 0xfc;
export const SAVE_ITEM_BUFFER_BYTES = 0x4000;
export const EXTENDED_ITEM_CEILING_BYTES = 4096;
export const DEFAULT_FIXTURE_ITEM_ID = 0x45585453;
export const TEST_INVENTORY_WIDTH = 11;
export const TEST_INVENTORY_HEIGHT = 8;

const LAYERED_TRIGGER_STAT_IDS = Object.freeze([
  195, // item_skillonattack
  196, // item_skillonkill
  197, // item_skillondeath
  198, // item_skillonhit
  199, // item_skillonlevelup
  201, // item_skillongethit
]);

const CODEC_OPTIONS = Object.freeze({
  disableItemEnhancements: true,
  sortProperties: false,
});

function sha256(bytes) {
  return crypto.createHash('sha256').update(bytes).digest('hex').toUpperCase();
}

function readGovernedTable(filePath) {
  const parsed = parseTable(filePath);
  const serialized = Buffer.from(serializeTable(parsed), ENCODING);
  const original = fs.readFileSync(filePath);
  if (!serialized.equals(original)) {
    throw new Error(`TSV round-trip is not byte-exact: ${filePath}`);
  }
  return original.toString('utf8');
}

export function buildBkvinceConstants() {
  const buffers = {};
  for (const name of TABLE_NAMES) {
    const filePath = path.join(EXCEL_ROOT, `${name.toLowerCase()}.txt`);
    buffers[`${name}.txt`] = readGovernedTable(filePath);
  }
  for (const name of STRING_NAMES) {
    const filePath = path.join(STRINGS_ROOT, `${name}.json`);
    buffers[`${name}.json`] = fs.readFileSync(filePath, 'utf8');
  }

  // PlayerClass is a D2S ABI lookup. Gameplay class values still come from
  // BKVince charstats.txt.
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
  buffers['RareSuffix.txt'] = 'name\r\n';
  buffers['RarePrefix.txt'] = 'name\r\n';
  return readConstantData(buffers);
}

export function directlyEncodableStats(constants) {
  const continuationIds = new Set();
  constants.magical_properties.forEach((definition, id) => {
    for (let offset = 1; offset < (definition?.np || 1); offset += 1) {
      continuationIds.add(id + offset);
    }
  });

  return constants.magical_properties
    .map((definition, id) => ({ definition, id }))
    .filter(({ definition, id }) => id >= 16
      && id < 0x1ff
      && definition?.s
      && definition.sB
      && !definition.sP
      && definition.e !== 3
      && (definition.np || 1) === 1
      && !continuationIds.has(id));
}

function fixtureAttributes(constants, statCount) {
  const available = directlyEncodableStats(constants);
  if (!Number.isInteger(statCount) || statCount < 1 || statCount > available.length) {
    throw new RangeError(`statCount must be between 1 and ${available.length}.`);
  }
  return available.slice(0, statCount).map(({ definition, id }) => ({
    id,
    name: definition.s,
    // A decoded value which produces the small, non-zero raw representation 1.
    values: [1 - (definition.sA || 0)],
  }));
}

function validLayeredSkillIds(constants) {
  return constants.skills
    .map((definition, id) => ({ definition, id }))
    .filter(({ definition, id }) => id < 0x400 && definition?.s)
    .map(({ id }) => id);
}

function layeredTriggerAttributes(constants, statCount) {
  const skillIds = validLayeredSkillIds(constants);
  const layersPerLevel = skillIds.length * LAYERED_TRIGGER_STAT_IDS.length;
  const capacity = layersPerLevel * 0x3f;
  if (!Number.isInteger(statCount) || statCount < 0 || statCount > capacity) {
    throw new RangeError(`layered statCount must be between 0 and ${capacity}.`);
  }

  return Array.from({ length: statCount }, (_, index) => {
    const statId = LAYERED_TRIGGER_STAT_IDS[index % LAYERED_TRIGGER_STAT_IDS.length];
    const layerIndex = Math.floor(index / LAYERED_TRIGGER_STAT_IDS.length);
    const skillId = skillIds[layerIndex % skillIds.length];
    const skillLevel = Math.floor(layerIndex / skillIds.length) + 1;
    const definition = constants.magical_properties[statId];
    return {
      id: statId,
      name: definition.s,
      // e=2 stores a unique 16-bit layer (skill level + skill ID), followed
      // by the visible chance. Each complete record is exactly 32 bits.
      values: [skillLevel, skillId, 1],
    };
  });
}

function fixtureModelFromAttributes(attributes, itemId = DEFAULT_FIXTURE_ITEM_ID) {
  return {
    identified: 1,
    socketed: 0,
    new: 0,
    is_ear: 0,
    starter_item: 0,
    simple_item: 0,
    ethereal: 0,
    personalized: 0,
    given_runeword: 0,
    version: '101',
    location_id: 0,
    equipped_id: 0,
    position_x: 0,
    position_y: 0,
    // Stored directly in the character's personal stash so runtime fixtures
    // remain visible without first opening the inventory.
    alt_position_id: PERSONAL_STASH_ALT_POSITION_ID,
    type: 'jew',
    nr_of_items_in_sockets: 0,
    id: itemId,
    level: 99,
    quality: 4,
    multiple_pictures: 0,
    class_specific: 0,
    magic_prefix: 0,
    magic_suffix: 0,
    timestamp: 0,
    magic_attributes: attributes,
  };
}

function inventoryBlockerPotion(positionX, positionY) {
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
    position_x: positionX,
    position_y: positionY,
    alt_position_id: 1,
    type: 'hp5',
    nr_of_items_in_sockets: 0,
  };
}

function inventoryBlockerPotions() {
  return Array.from(
    { length: TEST_INVENTORY_WIDTH * TEST_INVENTORY_HEIGHT },
    (_, index) => inventoryBlockerPotion(
      index % TEST_INVENTORY_WIDTH,
      Math.floor(index / TEST_INVENTORY_WIDTH),
    ),
  );
}

function fixtureModel(constants, statCount, itemId = DEFAULT_FIXTURE_ITEM_ID) {
  return fixtureModelFromAttributes(fixtureAttributes(constants, statCount), itemId);
}

async function encodeFixtureModel(constants, model) {
  const bytes = await writeItem(model, ITEM_FORMAT_VERSION, constants, CODEC_OPTIONS);
  const decoded = await readItem(bytes, ITEM_FORMAT_VERSION, constants, CODEC_OPTIONS);
  return { bytes: Buffer.from(bytes), decoded };
}

async function encodeFixture(constants, statCount, itemId = DEFAULT_FIXTURE_ITEM_ID) {
  return encodeFixtureModel(constants, fixtureModel(constants, statCount, itemId));
}

async function firstCountOver(constants, byteLimit) {
  const available = directlyEncodableStats(constants);
  for (let statCount = 1; statCount <= available.length; statCount += 1) {
    const { bytes } = await encodeFixture(constants, statCount);
    if (bytes.length > byteLimit) return { statCount, bytes: bytes.length };
  }
  return null;
}

export async function buildFixtureReport(
  statCount = DEFAULT_STAT_COUNT,
  itemId = DEFAULT_FIXTURE_ITEM_ID,
) {
  if (!Number.isInteger(itemId) || itemId < 0 || itemId > 0xffffffff) {
    throw new RangeError('itemId must be an unsigned 32-bit integer.');
  }
  const constants = buildBkvinceConstants();
  const { bytes, decoded } = await encodeFixture(constants, statCount, itemId);
  const expectedIds = fixtureAttributes(constants, statCount).map(({ id }) => id);
  const decodedIds = decoded.magic_attributes.map(({ id }) => id);
  if (JSON.stringify(decodedIds) !== JSON.stringify(expectedIds)) {
    throw new Error('Synthetic item failed its stat-ID round-trip.');
  }

  const itemStatCostPath = path.join(EXCEL_ROOT, 'itemstatcost.txt');
  return {
    bytes,
    report: {
      fixture: 'synthetic-bkvince-magic-jewel',
      containsExternalSaveData: false,
      d2sVersion: ITEM_FORMAT_VERSION,
      itemId,
      statCount,
      itemBytes: bytes.length,
      container: 'personal-stash',
      sha256: sha256(bytes),
      firstStatId: decodedIds[0],
      lastStatId: decodedIds.at(-1),
      exceedsNetworkItemBuffer: bytes.length > NETWORK_ITEM_BUFFER_BYTES,
      exceedsOneByteLength: bytes.length > 0xff,
      fitsSaveItemBuffer: bytes.length <= SAVE_ITEM_BUFFER_BYTES,
      limits: {
        networkItemBufferBytes: NETWORK_ITEM_BUFFER_BYTES,
        networkPacketBytes: NETWORK_PACKET_BYTES,
        saveItemBufferBytes: SAVE_ITEM_BUFFER_BYTES,
      },
      firstNetworkItemBufferOverflow: await firstCountOver(
        constants,
        NETWORK_ITEM_BUFFER_BYTES,
      ),
      schema: {
        source: path.relative(REPOSITORY_ROOT, itemStatCostPath).replaceAll('\\', '/'),
        itemStatCostEntries: constants.magical_properties.length,
        directlyEncodableStats: directlyEncodableStats(constants).length,
        sha256: sha256(fs.readFileSync(itemStatCostPath)),
      },
    },
  };
}

export async function buildCeilingFixtureReport(
  targetBytes = EXTENDED_ITEM_CEILING_BYTES,
  itemId = DEFAULT_FIXTURE_ITEM_ID,
) {
  if (!Number.isInteger(targetBytes) || targetBytes < 1 || targetBytes > SAVE_ITEM_BUFFER_BYTES) {
    throw new RangeError(`targetBytes must be between 1 and ${SAVE_ITEM_BUFFER_BYTES}.`);
  }
  if (!Number.isInteger(itemId) || itemId < 0 || itemId > 0xffffffff) {
    throw new RangeError('itemId must be an unsigned 32-bit integer.');
  }

  const constants = buildBkvinceConstants();
  const paddingPool = directlyEncodableStats(constants);
  let best = null;

  for (let paddingCount = 0; paddingCount <= paddingPool.length; paddingCount += 1) {
    const padding = fixtureAttributes(constants, Math.max(1, paddingCount));
    if (paddingCount === 0) padding.length = 0;
    const base = await encodeFixtureModel(
      constants,
      fixtureModelFromAttributes(padding, itemId),
    );
    const remainingBytes = targetBytes - base.bytes.length;
    if (remainingBytes < 0) continue;

    const layeredStatCount = Math.floor(remainingBytes / 4);
    const attributes = [
      ...padding,
      ...layeredTriggerAttributes(constants, layeredStatCount),
    ].sort((left, right) => left.id - right.id);
    const encoded = await encodeFixtureModel(
      constants,
      fixtureModelFromAttributes(attributes, itemId),
    );
    if (encoded.bytes.length > targetBytes) continue;
    if (!best || encoded.bytes.length > best.bytes.length) {
      best = { ...encoded, attributes, layeredStatCount, paddingCount };
    }
    if (encoded.bytes.length === targetBytes) break;
  }

  if (!best) throw new Error(`Could not build an item at or below ${targetBytes} bytes.`);
  const expected = best.attributes.map(({ id, values }) => ({ id, values }));
  const actual = best.decoded.magic_attributes.map(({ id, values }) => ({ id, values }));
  if (JSON.stringify(actual) !== JSON.stringify(expected)) {
    throw new Error('Ceiling item failed its stat/layer/value round-trip.');
  }

  return {
    bytes: best.bytes,
    report: {
      fixture: 'synthetic-bkvince-ceiling-magic-jewel',
      containsExternalSaveData: false,
      d2sVersion: ITEM_FORMAT_VERSION,
      itemId,
      targetBytes,
      itemBytes: best.bytes.length,
      exactTarget: best.bytes.length === targetBytes,
      statCount: best.attributes.length,
      layeredStatCount: best.layeredStatCount,
      paddingStatCount: best.paddingCount,
      distinctLayeredStatIds: LAYERED_TRIGGER_STAT_IDS.length,
      container: 'personal-stash',
      sha256: sha256(best.bytes),
      fitsConfiguredCeiling: best.bytes.length <= EXTENDED_ITEM_CEILING_BYTES,
      fitsSaveItemBuffer: best.bytes.length <= SAVE_ITEM_BUFFER_BYTES,
    },
  };
}

export async function buildCeilingCharacterSave(
  baseD2sBytes,
  targetBytes = EXTENDED_ITEM_CEILING_BYTES,
  itemId = DEFAULT_FIXTURE_ITEM_ID,
  fillInventory = false,
) {
  const constants = buildBkvinceConstants();
  const fixture = await buildCeilingFixtureReport(targetBytes, itemId);
  const character = await read(Buffer.from(baseD2sBytes), constants, CODEC_OPTIONS);
  const item = await readItem(
    fixture.bytes,
    ITEM_FORMAT_VERSION,
    constants,
    CODEC_OPTIONS,
  );

  const originalItems = character.items || [];
  const retainedItems = originalItems.filter((candidate) => candidate.id !== item.id
    && candidate.id !== DEFAULT_FIXTURE_ITEM_ID
    && !(candidate.type === 'jew' && candidate.magic_attributes?.length >= 900));
  const removedFixtureCount = originalItems.length - retainedItems.length;
  character.items = retainedItems;
  if (fillInventory) {
    character.items = character.items.filter((candidate) => candidate.location_id !== 0
      || candidate.alt_position_id !== 1);
    character.items.push(...inventoryBlockerPotions());
  }
  character.items.push(item);
  const bytes = Buffer.from(await write(character, constants, CODEC_OPTIONS));
  const decoded = await read(bytes, constants, CODEC_OPTIONS);
  const decodedItem = decoded.items.find((candidate) => candidate.id === item.id);
  if (!decodedItem) throw new Error('Ceiling item is missing from the encoded character.');
  const rewrittenItem = Buffer.from(await writeItem(
    decodedItem,
    ITEM_FORMAT_VERSION,
    constants,
    CODEC_OPTIONS,
  ));
  if (!rewrittenItem.equals(fixture.bytes)) {
    throw new Error('Ceiling item changed while embedded in the character save.');
  }
  const inventoryBlockerCount = decoded.items.filter((candidate) => candidate.type === 'hp5'
    && candidate.location_id === 0
    && candidate.alt_position_id === 1).length;

  return {
    bytes,
    report: {
      character: decoded.header?.name,
      characterBytes: bytes.length,
      characterSha256: sha256(bytes),
      itemBytes: rewrittenItem.length,
      itemId: decodedItem.id,
      itemSha256: sha256(rewrittenItem),
      statCount: decodedItem.magic_attributes.length,
      container: 'personal-stash',
      position: { x: decodedItem.position_x, y: decodedItem.position_y },
      itemCount: decoded.items.length,
      removedFixtureCount,
      inventoryBlockerCount,
    },
  };
}

export async function buildDirectCharacterSave(
  baseD2sBytes,
  statCount = 233,
  itemId = DEFAULT_FIXTURE_ITEM_ID,
  fillInventory = false,
) {
  const constants = buildBkvinceConstants();
  const fixture = await buildFixtureReport(statCount, itemId);
  const character = await read(Buffer.from(baseD2sBytes), constants, CODEC_OPTIONS);
  const item = await readItem(
    fixture.bytes,
    ITEM_FORMAT_VERSION,
    constants,
    CODEC_OPTIONS,
  );

  character.items = (character.items || []).filter((candidate) => candidate.id !== item.id
    && candidate.id !== DEFAULT_FIXTURE_ITEM_ID
    && !(candidate.type === 'jew' && candidate.magic_attributes?.length >= 900));
  if (fillInventory) {
    character.items = character.items.filter((candidate) => candidate.location_id !== 0
      || candidate.alt_position_id !== 1);
    character.items.push(...inventoryBlockerPotions());
  }
  character.items.push(item);

  const bytes = Buffer.from(await write(character, constants, CODEC_OPTIONS));
  const decoded = await read(bytes, constants, CODEC_OPTIONS);
  const decodedItem = decoded.items.find((candidate) => candidate.id === item.id);
  if (!decodedItem) throw new Error('Direct-stat item is missing from the encoded character.');
  const rewrittenItem = Buffer.from(await writeItem(
    decodedItem,
    ITEM_FORMAT_VERSION,
    constants,
    CODEC_OPTIONS,
  ));
  if (!rewrittenItem.equals(fixture.bytes)) {
    throw new Error('Direct-stat item changed while embedded in the character save.');
  }

  const inventoryBlockerCount = decoded.items.filter((candidate) => candidate.type === 'hp5'
    && candidate.location_id === 0
    && candidate.alt_position_id === 1).length;
  return {
    bytes,
    report: {
      character: decoded.header?.name,
      characterBytes: bytes.length,
      characterSha256: sha256(bytes),
      itemBytes: rewrittenItem.length,
      itemId: decodedItem.id,
      itemSha256: sha256(rewrittenItem),
      statCount: decodedItem.magic_attributes.length,
      container: 'personal-stash',
      position: { x: decodedItem.position_x, y: decodedItem.position_y },
      itemCount: decoded.items.length,
      inventoryBlockerCount,
    },
  };
}

function argumentValue(args, flag) {
  const index = args.indexOf(flag);
  return index >= 0 ? args[index + 1] : undefined;
}

async function main(args) {
  const requestedTarget = argumentValue(args, '--target-bytes');
  if (requestedTarget !== undefined) {
    const targetBytes = Number.parseInt(requestedTarget, 10);
    const requestedItemId = argumentValue(args, '--item-id');
    const itemId = requestedItemId === undefined
      ? DEFAULT_FIXTURE_ITEM_ID
      : Number(requestedItemId);
    const fillInventory = args.includes('--fill-inventory');
    const outputPath = argumentValue(args, '--output');
    const baseD2sPath = argumentValue(args, '--base-d2s');
    const result = baseD2sPath
      ? await buildCeilingCharacterSave(
        fs.readFileSync(path.resolve(baseD2sPath)),
        targetBytes,
        itemId,
        fillInventory,
      )
      : await buildCeilingFixtureReport(targetBytes, itemId);
    const { bytes, report } = result;
    if (outputPath) {
      const resolved = path.resolve(outputPath);
      fs.mkdirSync(path.dirname(resolved), { recursive: true });
      fs.writeFileSync(resolved, bytes);
      report.output = resolved;
    }
    process.stdout.write(`${JSON.stringify(report, null, 2)}\n`);
    return;
  }

  const requestedCount = argumentValue(args, '--stats');
  const statCount = requestedCount === undefined
    ? DEFAULT_STAT_COUNT
    : Number.parseInt(requestedCount, 10);
  const outputPath = argumentValue(args, '--output');
  const requestedItemId = argumentValue(args, '--item-id');
  const itemId = requestedItemId === undefined
    ? DEFAULT_FIXTURE_ITEM_ID
    : Number(requestedItemId);
  const baseD2sPath = argumentValue(args, '--base-d2s');
  const result = baseD2sPath
    ? await buildDirectCharacterSave(
      fs.readFileSync(path.resolve(baseD2sPath)),
      statCount,
      itemId,
      args.includes('--fill-inventory'),
    )
    : await buildFixtureReport(statCount, itemId);
  const { bytes, report } = result;

  if (outputPath) {
    const resolved = path.resolve(outputPath);
    fs.mkdirSync(path.dirname(resolved), { recursive: true });
    fs.writeFileSync(resolved, bytes);
    report.output = resolved;
  }
  process.stdout.write(`${JSON.stringify(report, null, 2)}\n`);
}

const invokedPath = process.argv[1] ? path.resolve(process.argv[1]) : '';
if (invokedPath === fileURLToPath(import.meta.url)) {
  main(process.argv.slice(2)).catch((error) => {
    process.stderr.write(`${error.stack || error.message || String(error)}\n`);
    process.exitCode = 1;
  });
}
