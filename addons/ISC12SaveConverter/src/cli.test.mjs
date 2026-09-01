import assert from 'node:assert/strict';
import test from 'node:test';

import { parseArguments } from './cli.mjs';

test('parses the minimal public CLI contract', () => {
  assert.deepEqual(parseArguments([
    '--to', 'isc12',
    '--schema', 'mod-schema.json',
    '--output', 'converted',
    'Saved Games',
  ]), {
    inputs: ['Saved Games'],
    to: 'isc12',
    schema: 'mod-schema.json',
    output: 'converted',
  });
});

test('rejects ambiguous schema sources and implicit directions', () => {
  assert.throws(
    () => parseArguments(['--to', 'isc12', '--schema', 'a', '--mod', 'b', 'save.d2s']),
    /either --schema or --mod/,
  );
  assert.throws(() => parseArguments(['save.d2s']), /Missing required option/);
});
