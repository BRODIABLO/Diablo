import assert from 'node:assert/strict';
import { mkdtemp, mkdir, writeFile } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import path from 'node:path';
import test from 'node:test';

import {
  findCompanionSharedStashes,
  parseArguments,
  runCli,
  runtimeInstructionsForTargetWidth,
} from './cli.mjs';

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

test('finds adjacent shared stashes for a selected character save', async () => {
  const root = await mkdtemp(path.join(tmpdir(), 'isc12-cli-stashes-'));
  await writeFile(path.join(root, 'Hero.d2s'), new Uint8Array([1]));
  await writeFile(path.join(root, 'ModernSharedStashSoftCoreV2.d2i'), new Uint8Array([2]));
  await writeFile(path.join(root, 'SharedStashSoftCoreV2.d2i'), new Uint8Array([3]));
  await writeFile(path.join(root, 'Hero.d2rl'), new Uint8Array([4]));
  assert.deepEqual(
    await findCompanionSharedStashes(path.join(root, 'Hero.d2s')),
    [
      path.join(root, 'ModernSharedStashSoftCoreV2.d2i'),
      path.join(root, 'SharedStashSoftCoreV2.d2i'),
    ],
  );
});

test('does not search for companion stashes when a directory or d2i is selected', async () => {
  const root = await mkdtemp(path.join(tmpdir(), 'isc12-cli-no-stashes-'));
  await mkdir(path.join(root, 'saves'));
  const stash = path.join(root, 'SharedStashSoftCoreV2.d2i');
  await writeFile(stash, new Uint8Array([1]));
  assert.deepEqual(await findCompanionSharedStashes(root), []);
  assert.deepEqual(await findCompanionSharedStashes(stash), []);
});

test('help returns a structured successful result', async () => {
  const lines = [];
  assert.deepEqual(await runCli(['--help'], { log: (line) => lines.push(line) }), {
    exitCode: 0,
  });
  assert.match(lines.join('\n'), /Clean, unmodded D2R v105/);
});

test('explains which item-stat codec must load converted saves', () => {
  const isc12 = runtimeInstructionsForTargetWidth(12).join('\n');
  assert.match(isc12, /ISC12 enabled/);
  assert.match(isc12, /do not load both codecs together/);

  const d2r9 = runtimeInstructionsForTargetWidth(9).join('\n');
  assert.match(d2r9, /restoring the mod's D2R 9-bit ItemStatCost codec/);
  assert.match(d2r9, /disabling ISC12/);
});
