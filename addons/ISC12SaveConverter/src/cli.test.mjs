import assert from 'node:assert/strict';
import { mkdtemp, mkdir, readFile, writeFile } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import path from 'node:path';
import test from 'node:test';
import { fileURLToPath } from 'node:url';

import { read, write } from '@d2runewizard/d2s';
import bkvinceConstants from '../../../apps/hero-editor/src/data/bkvince-constants.generated.js';
import { createBlankCharacter } from '../../../apps/hero-editor/src/lib/character-codec.js';

import {
  findCompanionSharedStashes,
  interactiveIntroduction,
  parseArguments,
  runCli,
  runtimeInstructionsForTargetWidth,
  shouldOpenOutputDirectory,
} from './cli.mjs';
import { DEFAULT_D2R_V105_CONSTANTS } from './default-schema.mjs';
import { codecConfig } from './runewizard-bridge.mjs';
import { LEGACY_STAT_ID_BITS } from './stat-stream.mjs';

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

test('parses separate source and target mod data without calling either ISC12-formatted', () => {
  assert.deepEqual(parseArguments([
    '--to', 'isc12',
    '--source-mod', 'Source Mod',
    '--target-mod', 'Target Mod',
    'Hero.d2s',
  ]), {
    inputs: ['Hero.d2s'],
    to: 'isc12',
    sourceMod: 'Source Mod',
    targetMod: 'Target Mod',
  });
  assert.throws(
    () => parseArguments([
      '--to', 'isc12',
      '--target-mod', 'Target Mod',
      '--target-vanilla',
      'Hero.d2s',
    ]),
    /only one target game-data option/,
  );
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
  assert.match(lines.join('\n'), /--source-mod/);
  assert.match(lines.join('\n'), /--target-mod/);
  assert.match(lines.join('\n'), /BIN-only/);
});

test('runs the public CLI across separate vanilla and BKVince schemas byte-exact', async () => {
  const root = await mkdtemp(path.join(tmpdir(), 'isc12-cli-schema-migration-'));
  const document = await createBlankCharacter({ name: 'ISCCli', className: 'Amazon' });
  const sourcePath = path.join(root, 'ISCCli.d2s');
  const upgradedDirectory = path.join(root, 'outputs', 'isc12');
  const restoredDirectory = path.join(root, 'outputs', 'd2r9');
  const bkvinceMod = path.resolve(
    path.dirname(fileURLToPath(import.meta.url)),
    '../../../data-BKVince',
  );
  const model = await read(
    document.sourceBytes,
    bkvinceConstants,
    codecConfig(LEGACY_STAT_ID_BITS),
  );
  model.items = [];
  const vanillaBytes = new Uint8Array(await write(
    model,
    DEFAULT_D2R_V105_CONSTANTS,
    codecConfig(LEGACY_STAT_ID_BITS),
  ));
  await writeFile(sourcePath, vanillaBytes);
  const logs = [];

  await runCli([
    '--to', 'isc12',
    '--target-mod', bkvinceMod,
    '--output', upgradedDirectory,
    sourcePath,
  ], { log: (line) => logs.push(line) });
  const upgradedPath = path.join(upgradedDirectory, 'ISCCli.d2s');

  await runCli([
    '--to', 'd2r9',
    '--source-mod', bkvinceMod,
    '--target-vanilla',
    '--output', restoredDirectory,
    upgradedPath,
  ], { log: (line) => logs.push(line) });

  assert.deepEqual(new Uint8Array(await readFile(
    path.join(restoredDirectory, 'ISCCli.d2s'),
  )), vanillaBytes);
  assert.match(logs.join('\n'), /Source game data: Built-in clean vanilla/);
  assert.match(logs.join('\n'), /Target game data: Installed mod data/);
  assert.match(logs.join('\n'), /Target game data: Built-in clean vanilla/);
});

test('interactive introduction keeps the complete wrapped description', () => {
  const introduction = interactiveIntroduction();
  assert.match(introduction, /Supports clean vanilla saves and\nmodded saves using matching mod data/);
  assert.match(introduction, /Original files are never overwritten/);
  assert.doesNotMatch(introduction, /Usage:/);
});

test('explains which item-stat codec must load converted saves', () => {
  const isc12 = runtimeInstructionsForTargetWidth(12).join('\n');
  assert.match(isc12, /ISC12 enabled/);
  assert.match(isc12, /ExtendedItemStats 0\.3\.14 may stay installed/);
  assert.match(isc12, /sole owner of all six full-item transport hooks/);
  assert.match(isc12, /unsupported providers fail closed/);

  const d2r9 = runtimeInstructionsForTargetWidth(9).join('\n');
  assert.match(d2r9, /restoring the mod's D2R 9-bit ItemStatCost codec/);
  assert.match(d2r9, /disabling ISC12/);
});

test('accepts both letter O and zero for opening the output folder', () => {
  assert.equal(shouldOpenOutputDirectory('O'), true);
  assert.equal(shouldOpenOutputDirectory('o'), true);
  assert.equal(shouldOpenOutputDirectory('0'), true);
  assert.equal(shouldOpenOutputDirectory(''), false);
  assert.equal(shouldOpenOutputDirectory('open'), false);
});
