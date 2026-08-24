import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import test from 'node:test';

import { extractPlayerSequences } from './player-sequences.mjs';

function sha256(bytes) {
  return crypto.createHash('sha256').update(bytes).digest('hex').toUpperCase();
}

function tableRows(table) {
  return table.rows.map((row) => Object.fromEntries(
    table.headers.map((header, index) => [header, row[index]]),
  ));
}

test('player sequence baseline reproduces the governed D2R 3.3 layout', () => {
  const result = extractPlayerSequences();
  assert.equal(result.manifest.nativeLayout.playerSequenceSlotCount, 26);
  assert.equal(result.manifest.counts.nativeUniqueArrays, 47);
  assert.equal(result.manifest.counts.nativeRecordRows, 808);
  assert.equal(result.manifest.counts.mappingRows, 350);
  assert.deepEqual(result.manifest.nativeLayout.ambiguousStaticGroupSeeds, [{
    sequenceId: 6,
    rvas: ['0x1992660', '0x1992DF0'],
  }]);

  const mappings = tableRows(result.mappingTable);
  const cleave = mappings.filter((row) => row.sequence_id === '24');
  const mirroredBlades = mappings.filter((row) => row.sequence_id === '25');
  assert.equal(cleave.length, 14);
  assert.equal(mirroredBlades.length, 14);
  assert(cleave.every((row) => row.sequence_name === 'Cleave'));
  assert(mirroredBlades.every((row) => row.sequence_name === 'MirroredBlades'));
  assert(cleave.every((row) => row.runtime_group_rva === '0x2385EE0'));
  assert(mirroredBlades.every((row) => row.runtime_group_rva === '0x2386350'));

  for (const [kind, output] of Object.entries(result.manifest.outputs)) {
    const filePath = result.files[kind];
    const bytes = fs.readFileSync(filePath);
    assert.equal(bytes.length, output.bytes, `${kind} byte length drift`);
    assert.equal(sha256(bytes), output.sha256, `${kind} hash drift`);
    if (kind !== 'runtime') {
      const text = bytes.toString('utf8');
      assert(!text.startsWith('\uFEFF'), `${kind} unexpectedly has a BOM`);
      assert(!/(?<!\r)\n/.test(text), `${kind} unexpectedly has a lone LF`);
    }
  }

  assert.deepEqual(
    JSON.parse(fs.readFileSync(result.files.runtime, 'utf8')),
    result.runtimeLayout,
  );
  assert.deepEqual(
    JSON.parse(fs.readFileSync(result.files.manifest, 'utf8')),
    result.manifest,
  );
});
