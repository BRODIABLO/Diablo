import {
  read,
  readItem,
  write,
  writeItem,
} from '@d2runewizard/d2s';

import {
  ISC12_STAT_ID_BITS,
  LEGACY_STAT_ID_BITS,
  statTerminator,
} from './stat-stream.mjs';

export const D2R_SAVE_VERSION = 0x69;

export class SaveConversionBlockedError extends Error {
  constructor(blockers) {
    const first = blockers[0];
    super(
      blockers.length === 1
        ? `${first.path}: stat ID ${first.id} cannot be represented in the D2R 9-bit format.`
        : `${blockers.length} stat entries cannot be represented in the D2R 9-bit format.`,
    );
    this.name = 'SaveConversionBlockedError';
    this.blockers = Object.freeze(blockers.map((blocker) => Object.freeze({ ...blocker })));
  }
}

export async function transcodeItemRecord({
  input,
  constants,
  sourceWidth,
  targetWidth,
  version = D2R_SAVE_VERSION,
  scope = 'Item',
}) {
  if (!(input instanceof Uint8Array)) {
    throw new TypeError('input must be a Uint8Array.');
  }
  validateConstants(constants);
  statTerminator(sourceWidth);
  statTerminator(targetWidth);

  const sourceConfig = codecConfig(sourceWidth);
  const targetConfig = codecConfig(targetWidth);
  const item = await readItem(input, version, constants, sourceConfig);
  const blockers = targetWidth === LEGACY_STAT_ID_BITS
    ? collectItemDowngradeBlockers(item, scope)
    : [];
  if (blockers.length > 0) {
    throw new SaveConversionBlockedError(blockers);
  }

  const bytes = new Uint8Array(await writeItem(
    structuredClone(item),
    version,
    constants,
    targetConfig,
  ));
  const reparsed = await readItem(bytes, version, constants, targetConfig);
  return Object.freeze({ bytes, item, reparsed });
}

export async function transcodeCharacterSave({
  input,
  constants,
  sourceWidth,
  targetWidth,
  scope = 'Character',
}) {
  if (!(input instanceof Uint8Array)) {
    throw new TypeError('input must be a Uint8Array.');
  }
  validateConstants(constants);
  validateD2sEnvelope(input);
  const sourceConfig = codecConfig(sourceWidth);
  const targetConfig = codecConfig(targetWidth);
  const model = await read(input, constants, sourceConfig);
  const blockers = targetWidth === LEGACY_STAT_ID_BITS
    ? collectCharacterDowngradeBlockers(model, scope)
    : [];
  if (blockers.length > 0) {
    throw new SaveConversionBlockedError(blockers);
  }

  const bytes = new Uint8Array(await write(
    structuredClone(model),
    constants,
    targetConfig,
  ));
  validateD2sEnvelope(bytes);
  const reparsed = await read(bytes, constants, targetConfig);
  return Object.freeze({ bytes, model, reparsed });
}

export function collectItemDowngradeBlockers(item, scope = 'Item') {
  const blockers = [];
  collectPropertyList(blockers, item?.magic_attributes, `${scope} > Magic`);
  if (Array.isArray(item?.set_attributes)) {
    item.set_attributes.forEach((properties, index) => {
      collectPropertyList(blockers, properties, `${scope} > Set list ${index + 1}`);
    });
  }
  collectPropertyList(blockers, item?.runeword_attributes, `${scope} > Runeword`);
  if (Array.isArray(item?.socketed_items)) {
    item.socketed_items.forEach((socketedItem, index) => {
      blockers.push(...collectItemDowngradeBlockers(
        socketedItem,
        `${scope} > Socket ${index + 1}`,
      ));
    });
  }
  return blockers;
}

export function collectCharacterDowngradeBlockers(model, scope = 'Character') {
  const blockers = [];
  if (Array.isArray(model?._raw_attributes)) {
    model._raw_attributes.forEach((entry, index) => {
      if (Number.isInteger(entry?.id) && entry.id >= statTerminator(LEGACY_STAT_ID_BITS)) {
        blockers.push(Object.freeze({
          id: entry.id,
          path: `${scope} > Player stats`,
          propertyIndex: index,
        }));
      }
    });
  }
  collectItemArray(blockers, model?.items, `${scope} > Items`);
  collectItemArray(blockers, model?.corpse_items, `${scope} > Corpse`);
  collectItemArray(blockers, model?.merc_items, `${scope} > Mercenary`);
  if (model?.golem_item) {
    blockers.push(...collectItemDowngradeBlockers(model.golem_item, `${scope} > Iron Golem`));
  }
  return blockers;
}

export function validateD2sEnvelope(input) {
  if (!(input instanceof Uint8Array) || input.length < 16) {
    throw new Error('D2S file is too short.');
  }
  const view = new DataView(input.buffer, input.byteOffset, input.byteLength);
  const declaredSize = view.getUint32(8, true);
  if (declaredSize !== input.length) {
    throw new Error(`D2S size mismatch: header=${declaredSize}, bytes=${input.length}.`);
  }
  const storedChecksum = view.getUint32(12, true);
  let checksum = 0;
  for (let index = 0; index < input.length; index += 1) {
    let byte = index >= 12 && index <= 15 ? 0 : input[index];
    if ((checksum & 0x80000000) !== 0) byte += 1;
    checksum = (byte + checksum * 2) >>> 0;
  }
  if (storedChecksum !== checksum) {
    throw new Error(
      `D2S checksum mismatch: header=${storedChecksum.toString(16)}, calculated=${checksum.toString(16)}.`,
    );
  }
  return Object.freeze({ declaredSize, checksum });
}

export function codecConfig(statIdBits) {
  statTerminator(statIdBits);
  return Object.freeze({
    disableItemEnhancements: true,
    preserveRawAttributes: true,
    preserveRawHeader: true,
    sortProperties: false,
    statIdBits,
  });
}

function collectPropertyList(blockers, properties, path) {
  if (!Array.isArray(properties)) return;
  properties.forEach((property, index) => {
    const id = property?.id;
    if (Number.isInteger(id) && id >= statTerminator(LEGACY_STAT_ID_BITS)) {
      blockers.push(Object.freeze({ id, path, propertyIndex: index }));
    }
  });
}

function collectItemArray(blockers, items, path) {
  if (!Array.isArray(items)) return;
  items.forEach((item, index) => {
    blockers.push(...collectItemDowngradeBlockers(item, `${path} > Item ${index + 1}`));
  });
}

function validateConstants(constants) {
  if (!constants || !Array.isArray(constants.magical_properties)) {
    throw new TypeError('constants.magical_properties must be an array.');
  }
}

export const SAVE_WIDTHS = Object.freeze({
  legacy: LEGACY_STAT_ID_BITS,
  isc12: ISC12_STAT_ID_BITS,
});
