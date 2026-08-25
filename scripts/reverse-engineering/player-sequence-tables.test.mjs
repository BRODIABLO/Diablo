import assert from 'node:assert/strict';
import fs from 'node:fs';
import test from 'node:test';

import {
  DEFAULT_TABLE_ROOT,
  buildPlayerSequenceTables,
  generatePlayerSequenceTables,
} from './player-sequence-tables.mjs';

function objects(table) {
  return table.rows.map((row) => Object.fromEntries(
    table.headers.map((header, index) => [header, row[index]]),
  ));
}

test('normalizes the governed runtime into the public two-table contract', () => {
  const result = buildPlayerSequenceTables();
  assert.deepEqual(result.stats, {
    routes: 350,
    availableRoutes: 235,
    nullRoutes: 115,
    sourceArrays: 47,
    recordSets: 47,
    uniqueContents: 44,
    records: 808,
  });
  assert.deepEqual(result.routeTable.headers, [
    'seqnum', '*sequence', 'weaponclass', 'recordset', '*eol',
  ]);
  assert.deepEqual(result.recordTable.headers, [
    'recordset', 'mode', 'frame', 'dir', 'event', '*eol',
  ]);

  const routes = objects(result.routeTable);
  for (let seqnum = 1; seqnum <= 25; seqnum += 1) {
    const group = routes.filter((route) => Number(route.seqnum) === seqnum);
    assert.equal(group.length, 14, `Sequence ${seqnum} does not have 14 weapon routes`);
    assert.equal(new Set(group.map((route) => route.weaponclass)).size, 14);
  }
  assert(routes.filter((route) => route.seqnum === '24').every((route) => route['*sequence'] === 'Cleave'));
  assert(routes.filter((route) => route.seqnum === '25').every((route) => route['*sequence'] === 'MirroredBlades'));

  const recordRows = objects(result.recordTable);
  const recordSets = new Set(recordRows.map((row) => row.recordset));
  assert.equal(recordSets.size, 47);
  for (const route of routes) {
    assert(!route.recordset || recordSets.has(route.recordset));
  }

  const route = (seqnum, weaponclass) => routes.find(
    (row) => row.seqnum === String(seqnum) && row.weaponclass === weaponclass,
  );
  assert.equal(route(23, 'HTH').recordset, 'gPlayerSequenceBladeFury');
  assert.equal(route(24, 'HTH').recordset, 'D2R_PlayerSequenceCleave_HTH');
  assert.equal(route(24, 'BOW').recordset, 'D2R_PlayerSequenceCleave_HTH');
  assert.equal(route(24, '1HT').recordset, 'D2R_PlayerSequenceCleave_1HT');
  assert.equal(route(24, '1HS').recordset, 'D2R_PlayerSequenceCleave_1HS');
  assert.equal(route(25, '1HT').recordset, 'D2R_PlayerSequenceMirroredBlades_1HT');
  assert.equal(route(25, '1HS').recordset, 'D2R_PlayerSequenceMirroredBlades_1HS');

  const content = (recordset) => recordRows
    .filter((row) => row.recordset === recordset)
    .map(({ recordset: ignored, ...row }) => row);
  assert.deepEqual(
    content('gPlayerSequenceBladeFury'),
    content('D2R_PlayerSequenceCleave_HTH'),
  );
  assert.deepEqual(
    content('D2R_PlayerSequenceCleave_1HT'),
    content('D2R_PlayerSequenceCleave_1HS'),
  );
  assert.deepEqual(
    content('D2R_PlayerSequenceMirroredBlades_1HT'),
    content('D2R_PlayerSequenceMirroredBlades_1HS'),
  );
});

test('writes CRLF, BOM-free, byte-exact public tables', () => {
  const result = generatePlayerSequenceTables();
  assert.equal(result.outputs.routes.bytes, fs.statSync(result.files.routes).size);
  assert.equal(result.outputs.records.bytes, fs.statSync(result.files.records).size);
  for (const fileName of ['playerseqmap.txt', 'playerseq.txt']) {
    const bytes = fs.readFileSync(`${DEFAULT_TABLE_ROOT}/${fileName}`);
    assert(!bytes.subarray(0, 3).equals(Buffer.from([0xEF, 0xBB, 0xBF])));
    const text = bytes.toString('latin1');
    assert(!/(?<!\r)\n/.test(text));
    assert(text.endsWith('\r\n'));
  }
});
