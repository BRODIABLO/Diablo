const MAGIC = Buffer.from('EIT1', 'ascii');

export const EXTENDED_ITEM_TRANSPORT_VERSION = 1;
export const EXTENDED_ITEM_FRAME_HEADER_BYTES = 32;
// The largest observed item-action packet has a 0xFC-byte total budget and a
// 13-byte 0x9D header. Keep an EIT frame within the conservative 0xEF remainder.
export const DEFAULT_EXTENDED_ITEM_FRAME_BYTES = 0xef;
export const DEFAULT_MAX_EXTENDED_ITEM_BYTES = 0x1000;
export const DEFAULT_MAX_IN_FLIGHT_TRANSFERS = 32;
export const DEFAULT_REASSEMBLY_TIMEOUT_MS = 5000;

const FLAG_FIRST = 0x01;
const FLAG_LAST = 0x02;

const CRC32_TABLE = new Uint32Array(256);
for (let index = 0; index < CRC32_TABLE.length; index += 1) {
  let value = index;
  for (let bit = 0; bit < 8; bit += 1) {
    value = (value & 1) === 1
      ? (value >>> 1) ^ 0xedb88320
      : value >>> 1;
  }
  CRC32_TABLE[index] = value >>> 0;
}

export function crc32(bytes) {
  const buffer = Buffer.from(bytes);
  let value = 0xffffffff;
  for (const byte of buffer) {
    value = CRC32_TABLE[(value ^ byte) & 0xff] ^ (value >>> 8);
  }
  return (value ^ 0xffffffff) >>> 0;
}

function requireInteger(name, value, minimum, maximum) {
  if (!Number.isInteger(value) || value < minimum || value > maximum) {
    throw new RangeError(`${name} must be an integer between ${minimum} and ${maximum}.`);
  }
}

function normalizeOptions(options = {}) {
  const frameBytes = options.frameBytes ?? DEFAULT_EXTENDED_ITEM_FRAME_BYTES;
  const maxItemBytes = options.maxItemBytes ?? DEFAULT_MAX_EXTENDED_ITEM_BYTES;
  const transferId = options.transferId ?? 1;

  requireInteger('frameBytes', frameBytes, EXTENDED_ITEM_FRAME_HEADER_BYTES + 1, 0xffff);
  requireInteger('maxItemBytes', maxItemBytes, 1, 0xffffffff);
  requireInteger('transferId', transferId, 0, 0xffffffff);
  return { frameBytes, maxItemBytes, transferId };
}

function expectedFlags(chunkIndex, chunkCount) {
  return (chunkIndex === 0 ? FLAG_FIRST : 0)
    | (chunkIndex === chunkCount - 1 ? FLAG_LAST : 0);
}

export function fragmentExtendedItem(itemBytes, options = {}) {
  const item = Buffer.from(itemBytes);
  const { frameBytes, maxItemBytes, transferId } = normalizeOptions(options);
  if (item.length === 0) throw new RangeError('itemBytes must not be empty.');
  if (item.length > maxItemBytes) {
    throw new RangeError(`Extended item is ${item.length} bytes; configured maximum is ${maxItemBytes}.`);
  }

  const framePayloadBytes = frameBytes - EXTENDED_ITEM_FRAME_HEADER_BYTES;
  const chunkCount = Math.ceil(item.length / framePayloadBytes);
  requireInteger('chunkCount', chunkCount, 1, 0xffff);
  const itemChecksum = crc32(item);

  return Array.from({ length: chunkCount }, (_, chunkIndex) => {
    const chunkOffset = chunkIndex * framePayloadBytes;
    const chunk = item.subarray(chunkOffset, chunkOffset + framePayloadBytes);
    const frame = Buffer.alloc(EXTENDED_ITEM_FRAME_HEADER_BYTES + chunk.length);

    MAGIC.copy(frame, 0);
    frame.writeUInt8(EXTENDED_ITEM_TRANSPORT_VERSION, 4);
    frame.writeUInt8(expectedFlags(chunkIndex, chunkCount), 5);
    frame.writeUInt16LE(EXTENDED_ITEM_FRAME_HEADER_BYTES, 6);
    frame.writeUInt32LE(transferId, 8);
    frame.writeUInt32LE(item.length, 12);
    frame.writeUInt32LE(itemChecksum, 16);
    frame.writeUInt32LE(chunkOffset, 20);
    frame.writeUInt16LE(chunkIndex, 24);
    frame.writeUInt16LE(chunkCount, 26);
    frame.writeUInt16LE(chunk.length, 28);
    frame.writeUInt16LE(0, 30);
    chunk.copy(frame, EXTENDED_ITEM_FRAME_HEADER_BYTES);
    return frame;
  });
}

export function parseExtendedItemFrame(frameBytes, options = {}) {
  const frame = Buffer.from(frameBytes);
  const { frameBytes: maximumFrameBytes, maxItemBytes } = normalizeOptions(options);
  if (frame.length < EXTENDED_ITEM_FRAME_HEADER_BYTES || frame.length > maximumFrameBytes) {
    throw new RangeError(`Extended-item frame length ${frame.length} is invalid.`);
  }
  if (!frame.subarray(0, MAGIC.length).equals(MAGIC)) {
    throw new Error('Extended-item frame magic is invalid.');
  }
  if (frame.readUInt8(4) !== EXTENDED_ITEM_TRANSPORT_VERSION) {
    throw new Error(`Unsupported extended-item transport version ${frame.readUInt8(4)}.`);
  }
  if (frame.readUInt16LE(6) !== EXTENDED_ITEM_FRAME_HEADER_BYTES) {
    throw new Error('Extended-item frame header length is invalid.');
  }
  if (frame.readUInt16LE(30) !== 0) {
    throw new Error('Extended-item frame reserved bits are not zero.');
  }

  const transferId = frame.readUInt32LE(8);
  const totalItemBytes = frame.readUInt32LE(12);
  const itemChecksum = frame.readUInt32LE(16);
  const chunkOffset = frame.readUInt32LE(20);
  const chunkIndex = frame.readUInt16LE(24);
  const chunkCount = frame.readUInt16LE(26);
  const chunkBytes = frame.readUInt16LE(28);
  const flags = frame.readUInt8(5);

  if (totalItemBytes < 1 || totalItemBytes > maxItemBytes) {
    throw new RangeError(`Declared extended-item length ${totalItemBytes} is invalid.`);
  }
  if (chunkCount < 1 || chunkIndex >= chunkCount) {
    throw new Error('Extended-item chunk index/count is invalid.');
  }
  if (flags !== expectedFlags(chunkIndex, chunkCount)) {
    throw new Error('Extended-item chunk flags are invalid.');
  }
  if (chunkBytes < 1 || frame.length !== EXTENDED_ITEM_FRAME_HEADER_BYTES + chunkBytes) {
    throw new Error('Extended-item chunk length is invalid.');
  }
  if (chunkOffset >= totalItemBytes || chunkOffset + chunkBytes > totalItemBytes) {
    throw new Error('Extended-item chunk range is invalid.');
  }

  return {
    transferId,
    totalItemBytes,
    itemChecksum,
    chunkOffset,
    chunkIndex,
    chunkCount,
    payload: frame.subarray(EXTENDED_ITEM_FRAME_HEADER_BYTES),
  };
}

export function reassembleExtendedItem(frames, options = {}) {
  if (!Array.isArray(frames) || frames.length === 0) {
    throw new TypeError('frames must be a non-empty array.');
  }

  const parsed = frames.map((frame) => parseExtendedItemFrame(frame, options));
  const reference = parsed[0];
  if (frames.length !== reference.chunkCount) {
    throw new Error(`Expected ${reference.chunkCount} chunks, received ${frames.length}.`);
  }

  for (const chunk of parsed) {
    if (chunk.transferId !== reference.transferId
      || chunk.totalItemBytes !== reference.totalItemBytes
      || chunk.itemChecksum !== reference.itemChecksum
      || chunk.chunkCount !== reference.chunkCount) {
      throw new Error('Extended-item chunks do not belong to the same transfer.');
    }
  }

  parsed.sort((left, right) => left.chunkIndex - right.chunkIndex);
  const item = Buffer.alloc(reference.totalItemBytes);
  let expectedOffset = 0;
  parsed.forEach((chunk, chunkIndex) => {
    if (chunk.chunkIndex !== chunkIndex || chunk.chunkOffset !== expectedOffset) {
      throw new Error('Extended-item chunks are duplicated, missing, or non-contiguous.');
    }
    chunk.payload.copy(item, chunk.chunkOffset);
    expectedOffset += chunk.payload.length;
  });

  if (expectedOffset !== item.length) {
    throw new Error('Extended-item chunks do not cover the declared item length.');
  }
  if (crc32(item) !== reference.itemChecksum) {
    throw new Error('Extended-item checksum mismatch.');
  }
  return item;
}

function identityKey(name, value) {
  if (Buffer.isBuffer(value) || value instanceof Uint8Array) {
    return Buffer.from(value).toString('hex');
  }
  if (typeof value !== 'string' || value.length === 0) {
    throw new TypeError(`${name} must be a non-empty string or byte array.`);
  }
  return value;
}

export class ExtendedItemReassembler {
  constructor(options = {}) {
    const normalized = normalizeOptions(options);
    const maxInFlightTransfers = options.maxInFlightTransfers
      ?? DEFAULT_MAX_IN_FLIGHT_TRANSFERS;
    const timeoutMs = options.timeoutMs ?? DEFAULT_REASSEMBLY_TIMEOUT_MS;
    requireInteger('maxInFlightTransfers', maxInFlightTransfers, 1, 0xffff);
    requireInteger('timeoutMs', timeoutMs, 1, 0x7fffffff);

    this.options = normalized;
    this.maxInFlightTransfers = maxInFlightTransfers;
    this.timeoutMs = timeoutMs;
    this.transfers = new Map();
  }

  get inFlightTransfers() {
    return this.transfers.size;
  }

  expire(nowMs = Date.now()) {
    requireInteger('nowMs', nowMs, 0, Number.MAX_SAFE_INTEGER);
    let expired = 0;
    for (const [key, transfer] of this.transfers) {
      if (nowMs - transfer.createdAtMs >= this.timeoutMs) {
        this.transfers.delete(key);
        expired += 1;
      }
    }
    return expired;
  }

  accept(frame, context = {}) {
    const nowMs = context.nowMs ?? Date.now();
    requireInteger('nowMs', nowMs, 0, Number.MAX_SAFE_INTEGER);
    const channelKey = identityKey('channelKey', context.channelKey ?? 'default');
    const envelopeKey = identityKey('envelopeKey', context.envelopeKey ?? 'item-action');
    const parsed = parseExtendedItemFrame(frame, this.options);
    this.expire(nowMs);

    const key = `${channelKey}\0${parsed.transferId}`;
    let transfer = this.transfers.get(key);
    if (!transfer) {
      if (this.transfers.size >= this.maxInFlightTransfers) {
        throw new Error('Extended-item in-flight transfer limit reached.');
      }
      transfer = {
        createdAtMs: nowMs,
        envelopeKey,
        totalItemBytes: parsed.totalItemBytes,
        itemChecksum: parsed.itemChecksum,
        chunkCount: parsed.chunkCount,
        frames: new Map(),
      };
      this.transfers.set(key, transfer);
    }

    const inconsistent = transfer.envelopeKey !== envelopeKey
      || transfer.totalItemBytes !== parsed.totalItemBytes
      || transfer.itemChecksum !== parsed.itemChecksum
      || transfer.chunkCount !== parsed.chunkCount;
    if (inconsistent || transfer.frames.has(parsed.chunkIndex)) {
      this.transfers.delete(key);
      throw new Error(inconsistent
        ? 'Extended-item transfer metadata changed between chunks.'
        : 'Extended-item transfer contains a duplicate chunk.');
    }

    transfer.frames.set(parsed.chunkIndex, Buffer.from(frame));
    if (transfer.frames.size !== transfer.chunkCount) {
      return {
        status: 'pending',
        transferId: parsed.transferId,
        receivedChunks: transfer.frames.size,
        chunkCount: transfer.chunkCount,
      };
    }

    try {
      const itemBytes = reassembleExtendedItem([...transfer.frames.values()], this.options);
      this.transfers.delete(key);
      return { status: 'complete', transferId: parsed.transferId, itemBytes };
    } catch (error) {
      this.transfers.delete(key);
      throw error;
    }
  }
}
