import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';
import { fileURLToPath } from 'node:url';

import {
  DEFAULT_STAT_COUNT,
  DEFAULT_FIXTURE_ITEM_ID,
  EXTENDED_ITEM_CEILING_BYTES,
  NETWORK_ITEM_BUFFER_BYTES,
  TEST_INVENTORY_HEIGHT,
  TEST_INVENTORY_WIDTH,
  buildBkvinceConstants,
  buildCeilingCharacterSave,
  buildCeilingFixtureReport,
  buildDirectCharacterSave,
  buildFixtureReport,
  directlyEncodableStats,
} from './extended-item-stats-fixture.mjs';

const REPOSITORY_ROOT = path.resolve(
  path.dirname(fileURLToPath(import.meta.url)),
  '..',
  '..',
);

test('round-trips an anonymous 120-stat BKVince jewel deterministically', async () => {
  const first = await buildFixtureReport();
  const second = await buildFixtureReport();

  assert.equal(first.report.containsExternalSaveData, false);
  assert.equal(first.report.statCount, DEFAULT_STAT_COUNT);
  assert.equal(first.report.itemBytes, 271);
  assert.equal(first.report.container, 'personal-stash');
  assert.equal(first.report.sha256, 'C807D3208C3AE06CBC5CE112F2759704C1A3EC3ECB4385C4917F5D4FB9D4223E');
  assert.deepEqual(first.bytes, second.bytes);
});

test('builds the largest directly encodable BKVince fixture in one jump', async () => {
  const { report } = await buildFixtureReport(233);

  assert.equal(report.statCount, 233);
  assert.equal(report.itemBytes, 576);
  assert.equal(report.container, 'personal-stash');
  assert.equal(report.lastStatId, 388);
});

test('crosses both governed network encodings before 120 logical stats', async () => {
  const { report } = await buildFixtureReport();

  assert.deepEqual(report.firstNetworkItemBufferOverflow, { statCount: 108, bytes: 245 });
  assert.ok(report.itemBytes > NETWORK_ITEM_BUFFER_BYTES);
  assert.equal(report.exceedsNetworkItemBuffer, true);
  assert.equal(report.exceedsOneByteLength, true);
  assert.equal(report.fitsSaveItemBuffer, true);
});

test('derives every fixture stat from the current governed BKVince schema', () => {
  const constants = buildBkvinceConstants();
  const available = directlyEncodableStats(constants);

  assert.equal(constants.magical_properties.length, 389);
  assert.equal(available.length, 233);
  assert.ok(available.length >= DEFAULT_STAT_COUNT);
});

test('round-trips one schema-valid item at the configured 4096-byte ceiling', async () => {
  const { report } = await buildCeilingFixtureReport();

  assert.equal(report.targetBytes, EXTENDED_ITEM_CEILING_BYTES);
  assert.equal(report.itemBytes, EXTENDED_ITEM_CEILING_BYTES);
  assert.equal(report.exactTarget, true);
  assert.equal(report.container, 'personal-stash');
  assert.equal(report.fitsConfiguredCeiling, true);
  assert.ok(report.layeredStatCount > 900);
  assert.equal(report.itemId, DEFAULT_FIXTURE_ITEM_ID);
});

test('creates a distinct ceiling item ID for a fresh stash fixture', async () => {
  const freshItemId = DEFAULT_FIXTURE_ITEM_ID + 1;
  const { report } = await buildCeilingFixtureReport(
    EXTENDED_ITEM_CEILING_BYTES,
    freshItemId,
  );

  assert.equal(report.itemId, freshItemId);
  assert.equal(report.itemBytes, EXTENDED_ITEM_CEILING_BYTES);
  assert.equal(report.container, 'personal-stash');
});

test('embeds the exact 4096-byte item directly in a clean personal stash', async () => {
  const basePath = path.join(
    REPOSITORY_ROOT,
    'analysis-cache',
    'runtime-validation',
    'extended-item-stats',
    '2026-07-23T124634',
    'QtyTester.original.d2s',
  );
  const { report } = await buildCeilingCharacterSave(fs.readFileSync(basePath));

  assert.equal(report.character, 'QtyTester');
  assert.equal(report.itemBytes, EXTENDED_ITEM_CEILING_BYTES);
  assert.equal(report.statCount, 1019);
  assert.equal(report.container, 'personal-stash');
  assert.deepEqual(report.position, { x: 0, y: 0 });
  assert.equal(report.itemCount, 1);
});

test('can block the whole test inventory while keeping the ceiling item in the stash', async () => {
  const basePath = path.join(
    REPOSITORY_ROOT,
    'analysis-cache',
    'runtime-validation',
    'extended-item-stats',
    '2026-07-23T124634',
    'QtyTester.original.d2s',
  );
  const { report } = await buildCeilingCharacterSave(
    fs.readFileSync(basePath),
    EXTENDED_ITEM_CEILING_BYTES,
    DEFAULT_FIXTURE_ITEM_ID + 1,
    true,
  );

  assert.equal(report.itemBytes, EXTENDED_ITEM_CEILING_BYTES);
  assert.equal(report.container, 'personal-stash');
  assert.deepEqual(report.position, { x: 0, y: 0 });
  assert.equal(
    report.inventoryBlockerCount,
    TEST_INVENTORY_WIDTH * TEST_INVENTORY_HEIGHT,
  );
  assert.equal(report.itemCount, 1 + TEST_INVENTORY_WIDTH * TEST_INVENTORY_HEIGHT);
});

test('embeds a fresh 233-stat tooltip fixture in the stash with a blocked inventory', async () => {
  const basePath = path.join(
    REPOSITORY_ROOT,
    'analysis-cache',
    'runtime-validation',
    'extended-item-stats',
    '2026-07-23T124634',
    'QtyTester.original.d2s',
  );
  const freshItemId = DEFAULT_FIXTURE_ITEM_ID + 2;
  const { report } = await buildDirectCharacterSave(
    fs.readFileSync(basePath),
    233,
    freshItemId,
    true,
  );

  assert.equal(report.itemId, freshItemId);
  assert.equal(report.itemBytes, 576);
  assert.equal(report.statCount, 233);
  assert.equal(report.container, 'personal-stash');
  assert.deepEqual(report.position, { x: 0, y: 0 });
  assert.equal(
    report.inventoryBlockerCount,
    TEST_INVENTORY_WIDTH * TEST_INVENTORY_HEIGHT,
  );
});
