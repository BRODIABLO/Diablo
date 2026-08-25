import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import { REPOSITORY_ROOT } from './player-sequences.mjs';

const SCRIPT_PATH = fileURLToPath(import.meta.url);
const SUITE_ROOT = path.join(REPOSITORY_ROOT, 'analysis-cache', 'ruffneckk-d2rloader-suite');
const SUITE_WRITES = path.join(SUITE_ROOT, 'manifests', 'native-writes-3.2.92777.json');
const RELEASE_ALLOWLIST = path.join(SUITE_ROOT, 'manifests', 'release-allowlist.json');
const PLUGINPACK_WRITES = path.join(
  REPOSITORY_ROOT,
  'analysis-cache',
  'pluginpack-foundation',
  'hook-manifest.json',
);
const RUNTIME_LAYOUT = path.join(
  REPOSITORY_ROOT,
  'reverse-engineering',
  'd2r-3.2.92777',
  'player-sequences',
  'd2r-3.3.93847-player-sequence-runtime.json',
);
const PLUGIN_SOURCE = path.join(
  REPOSITORY_ROOT,
  'addons',
  'PlayerSequenceTables',
  'src',
  'plugin.cpp',
);

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

function parseRva(value) {
  assert.match(value, /^0x[0-9A-F]+$/i);
  return Number.parseInt(value.slice(2), 16);
}

function collectWrites(value, location = '$', output = []) {
  if (Array.isArray(value)) {
    value.forEach((entry, index) => collectWrites(entry, `${location}[${index}]`, output));
    return output;
  }
  if (!value || typeof value !== 'object') return output;
  const size = Number.isInteger(value.size)
    ? value.size
    : Number.isInteger(value.length) ? value.length : null;
  if (typeof value.rva === 'string' && size !== null) {
    assert(size > 0, `Invalid write size at ${location}`);
    output.push({
      location,
      rva: parseRva(value.rva),
      size,
      owner: value.owner ?? value.id ?? value.componentId ?? 'manifest-entry',
      kind: value.kind ?? 'fixed-write',
    });
  }
  for (const [key, entry] of Object.entries(value)) {
    collectWrites(entry, `${location}.${key}`, output);
  }
  return output;
}

function overlaps(left, right) {
  return left.rva < right.rva + right.size
    && right.rva < left.rva + left.size;
}

export function auditPlayerSequenceCompatibility() {
  for (const required of [
    SUITE_WRITES,
    RELEASE_ALLOWLIST,
    PLUGINPACK_WRITES,
    RUNTIME_LAYOUT,
    PLUGIN_SOURCE,
  ]) {
    assert(fs.existsSync(required), `Missing governed compatibility input: ${required}`);
  }

  const suiteManifest = readJson(SUITE_WRITES);
  const releaseAllowlist = readJson(RELEASE_ALLOWLIST);
  const pluginPackManifest = readJson(PLUGINPACK_WRITES);
  const runtime = readJson(RUNTIME_LAYOUT);
  const source = fs.readFileSync(PLUGIN_SOURCE, 'utf8');
  assert.equal(suiteManifest.target.pluginSdkCommit,
    '4933e2c42cb2592958cd0df3b6dc5003102252d1');
  assert(releaseAllowlist.suite.validatedGameBuilds.includes(93847));
  assert.equal(releaseAllowlist.policy.expectedCounts.pluginDlls, 17);
  assert.equal(pluginPackManifest.targetBuild, '3.2.92777');
  assert.equal(runtime.target.version, '3.3.93847');

  const candidate = {
    rva: 0x2386658,
    size: 25 * 8,
  };
  const suiteWrites = collectWrites(suiteManifest);
  const pluginPackWrites = collectWrites(pluginPackManifest);
  assert.equal(suiteWrites.length, 191, 'Suite native-write manifest count drift');
  assert.equal(pluginPackWrites.length, 139, 'PluginPack native-write manifest count drift');
  const collisions = [
    ...suiteWrites.map((write) => ({ catalog: 'Suite', ...write })),
    ...pluginPackWrites.map((write) => ({ catalog: 'eezstreet PluginPack', ...write })),
  ].filter((write) => overlaps(candidate, write));
  assert.deepEqual(collisions, [], 'Player sequence pointer range has a governed owner collision');

  assert.equal(runtime.groups.length, 25);
  const groupRvas = runtime.groups.map((group) => parseRva(group.groupRva));
  for (const rva of groupRvas) {
    assert(source.includes(`0x${rva.toString(16).toUpperCase()}`)
      || source.includes(`0x${rva.toString(16)}`));
  }
  assert.equal((source.match(/Context->PatchBytes\(/g) ?? []).length, 1);
  assert(!source.includes('InstallInlineHook'));
  assert(!source.includes('ModScopedOnly'));
  assert(source.includes('PluginFlags::Shared | D2RL::PluginFlags::NativeHooks'));

  return {
    schemaVersion: 1,
    target: 'D2R 3.3.93847',
    candidate: {
      id: 'ruffneckk-player-sequence-tables',
      rva: '0x2386658',
      endExclusive: '0x2386720',
      bytes: candidate.size,
      mechanism: 'single SDK PatchBytes transaction; no code hook',
    },
    baseline: {
      suiteVersion: releaseAllowlist.suite.version,
      releasedSuitePlugins: releaseAllowlist.policy.expectedCounts.pluginDlls,
      suiteWrites: suiteWrites.length,
      pluginPackWrites: pluginPackWrites.length,
      totalComparedWrites: suiteWrites.length + pluginPackWrites.length,
      pluginSdkCommit: suiteManifest.target.pluginSdkCommit,
    },
    result: {
      collisions: collisions.length,
      uniqueOwner: true,
      hybridScope: true,
      eezstreetDllsModified: false,
      compatibleByStaticOwnership: true,
      runtimeQualificationStillRequired: true,
    },
  };
}

function main() {
  process.stdout.write(`${JSON.stringify(auditPlayerSequenceCompatibility(), null, 2)}\n`);
}

if (process.argv[1] && path.resolve(process.argv[1]) === SCRIPT_PATH) main();
