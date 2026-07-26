import assert from 'node:assert/strict';
import test from 'node:test';

import { buildFixtureReport } from './extended-item-stats-fixture.mjs';
import {
  DEFAULT_EXTENDED_ITEM_FRAME_BYTES,
  EXTENDED_ITEM_FRAME_HEADER_BYTES,
  ExtendedItemReassembler,
  crc32,
  fragmentExtendedItem,
  parseExtendedItemFrame,
  reassembleExtendedItem,
} from './extended-item-transport.mjs';

test('fragments and reassembles the anonymous 120-stat item byte-exact', async () => {
  const fixture = await buildFixtureReport();
  const frames = fragmentExtendedItem(fixture.bytes, { transferId: 0x45585431 });

  assert.equal(fixture.bytes.length, 271);
  assert.equal(frames.length, 2);
  assert.deepEqual(frames.map((frame) => frame.length), [239, 96]);
  assert.ok(frames.every((frame) => frame.length <= DEFAULT_EXTENDED_ITEM_FRAME_BYTES));
  assert.deepEqual(reassembleExtendedItem(frames.toReversed()), fixture.bytes);
});

test('fragments the maximum 233-stat BKVince item in one schema-sized jump', async () => {
  const fixture = await buildFixtureReport(233);
  const frames = fragmentExtendedItem(fixture.bytes, { transferId: 0x45585432 });

  assert.equal(fixture.bytes.length, 576);
  assert.equal(frames.length, 3);
  assert.ok(frames.every((frame) => frame.length <= DEFAULT_EXTENDED_ITEM_FRAME_BYTES));
  assert.deepEqual(reassembleExtendedItem(frames.toReversed()), fixture.bytes);
});

test('supports the configured 4096-byte ceiling without a mod-specific catalog', () => {
  const item = Buffer.alloc(4096);
  item.forEach((_, index) => {
    item[index] = (index * 73 + 19) & 0xff;
  });

  const frames = fragmentExtendedItem(item, { transferId: 7 });
  assert.equal(frames.length, 20);
  assert.ok(frames.every((frame) => frame.length <= DEFAULT_EXTENDED_ITEM_FRAME_BYTES));
  assert.deepEqual(reassembleExtendedItem(frames.toReversed()), item);
});

test('uses the standard CRC-32 check value', () => {
  assert.equal(crc32(Buffer.from('123456789', 'ascii')), 0xcbf43926);
});

test('rejects missing, mixed, oversized, and corrupted chunks', () => {
  const item = Buffer.alloc(600, 0x5a);
  const frames = fragmentExtendedItem(item, { transferId: 11 });
  const otherTransfer = fragmentExtendedItem(item, { transferId: 12 });

  assert.throws(() => reassembleExtendedItem(frames.slice(1)), /Expected .* chunks/);
  assert.throws(
    () => reassembleExtendedItem([frames[0], otherTransfer[1], frames[2]]),
    /same transfer/,
  );
  assert.throws(() => fragmentExtendedItem(Buffer.alloc(4097)), /configured maximum/);

  const corrupted = frames.map((frame) => Buffer.from(frame));
  corrupted.at(-1)[EXTENDED_ITEM_FRAME_HEADER_BYTES] ^= 0xff;
  assert.throws(() => reassembleExtendedItem(corrupted), /checksum mismatch/);
});

test('rejects malformed frame metadata before allocating the item', () => {
  const [frame] = fragmentExtendedItem(Buffer.alloc(32, 0x22));
  const malformed = Buffer.from(frame);
  malformed.writeUInt32LE(0xffffffff, 12);

  assert.throws(() => parseExtendedItemFrame(malformed), /Declared extended-item length/);
});

test('reassembles out-of-order fragments incrementally and releases the transfer', () => {
  const item = Buffer.alloc(700, 0x6d);
  const frames = fragmentExtendedItem(item, { transferId: 23 });
  const receiver = new ExtendedItemReassembler();
  const order = [2, 0, 3, 1];

  for (const frameIndex of order.slice(0, -1)) {
    const result = receiver.accept(frames[frameIndex], {
      channelKey: 'server-a',
      envelopeKey: '9d-action-4-item-42',
      nowMs: 1000 + frameIndex,
    });
    assert.equal(result.status, 'pending');
  }
  const complete = receiver.accept(frames[order.at(-1)], {
    channelKey: 'server-a',
    envelopeKey: '9d-action-4-item-42',
    nowMs: 1004,
  });

  assert.equal(complete.status, 'complete');
  assert.deepEqual(complete.itemBytes, item);
  assert.equal(receiver.inFlightTransfers, 0);
});

test('bounds concurrent transfers and expires incomplete state', () => {
  const receiver = new ExtendedItemReassembler({
    maxInFlightTransfers: 1,
    timeoutMs: 50,
  });
  const first = fragmentExtendedItem(Buffer.alloc(300, 1), { transferId: 1 });
  const second = fragmentExtendedItem(Buffer.alloc(300, 2), { transferId: 2 });

  receiver.accept(first[0], { channelKey: 'server', envelopeKey: 'first', nowMs: 100 });
  assert.throws(
    () => receiver.accept(second[0], {
      channelKey: 'server', envelopeKey: 'second', nowMs: 120,
    }),
    /in-flight transfer limit/,
  );
  assert.equal(receiver.expire(150), 1);
  assert.equal(receiver.inFlightTransfers, 0);
  assert.equal(receiver.accept(second[0], {
    channelKey: 'server', envelopeKey: 'second', nowMs: 151,
  }).status, 'pending');
});

test('drops a transfer when its item-action envelope changes or a chunk repeats', () => {
  const item = Buffer.alloc(300, 3);
  const frames = fragmentExtendedItem(item, { transferId: 31 });
  const receiver = new ExtendedItemReassembler();

  receiver.accept(frames[0], { channelKey: 'server', envelopeKey: '9c-action-1' });
  assert.throws(
    () => receiver.accept(frames[1], {
      channelKey: 'server', envelopeKey: '9c-action-2',
    }),
    /metadata changed/,
  );
  assert.equal(receiver.inFlightTransfers, 0);

  receiver.accept(frames[0], { channelKey: 'server', envelopeKey: '9c-action-1' });
  assert.throws(
    () => receiver.accept(frames[0], {
      channelKey: 'server', envelopeKey: '9c-action-1',
    }),
    /duplicate chunk/,
  );
  assert.equal(receiver.inFlightTransfers, 0);
});
