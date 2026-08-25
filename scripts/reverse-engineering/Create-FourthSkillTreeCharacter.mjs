import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';

import { read, write } from '@d2runewizard/d2s';

import { buildBkvinceConstants } from './extended-item-stats-fixture.mjs';

const EXPECTED_SOURCE_SHA256 =
  '0872B19927BAA1ACFFF683B734267C5CF4D4EF5A751D52F9100A9F566D0D16FE';
const CODEC_OPTIONS = Object.freeze({
  disableItemEnhancements: true,
  sortProperties: false,
});

function fail(message) {
  throw new Error(`FourthSkillTree character fixture: ${message}`);
}

function sha256(bytes) {
  return crypto.createHash('sha256').update(bytes).digest('hex').toUpperCase();
}

function parseArguments(values) {
  const parsed = {};
  for (let index = 0; index < values.length; index += 2) {
    const key = values[index];
    const value = values[index + 1];
    if (!key?.startsWith('--') || !value) {
      fail('expected --source-d2s and --output-d2s');
    }
    parsed[key.slice(2)] = value;
  }
  if (!parsed['source-d2s'] || !parsed['output-d2s']) {
    fail('expected --source-d2s and --output-d2s');
  }
  return parsed;
}

const argumentsByName = parseArguments(process.argv.slice(2));
const repositoryRoot = path.resolve(import.meta.dirname, '..', '..');
const sourcePath = path.resolve(argumentsByName['source-d2s']);
const outputPath = path.resolve(argumentsByName['output-d2s']);
const allowedOutputRoot = path.join(
  repositoryRoot,
  'analysis-cache',
  'fourth-skill-tree-fixture');
const relativeOutput = path.relative(allowedOutputRoot, outputPath);
if (relativeOutput.startsWith('..') || path.isAbsolute(relativeOutput)) {
  fail(`output must stay under ${allowedOutputRoot}`);
}
if (sourcePath === outputPath) fail('source and output must differ');

const sourceBytes = fs.readFileSync(sourcePath);
assert.equal(
  sha256(sourceBytes),
  EXPECTED_SOURCE_SHA256,
  'unexpected Amazon source save');

const constants = buildBkvinceConstants();
const character = await read(sourceBytes, constants, CODEC_OPTIONS);
assert.equal(character.header?.name, 'ama', 'unexpected character name');
assert.equal(character.header?.class, 'Amazon', 'unexpected character class');
assert.equal(character.skills?.length, 30, 'source must contain 30 Amazon skills');
assert.ok(character.skills.every((skill) => skill.points === 0),
  'source must not contain an invested rank');

// A level-one character has no serialized `gf` attribute section in a fresh
// save. The level-two editor state is accepted by the frontend but normalized
// back to level one when D2R materializes the player unit. Use the already
// exercised BKVince level-99 threshold so the disposable fixture has a stable
// serialized attribute section while keeping exactly one point for this test.
character.header.level = 99;
character.attributes.level = 99;
character.attributes.experience = 3520485254;
character.attributes.unused_stats = 0;
character.attributes.unused_skill_points = 1;
character.header.quests_normal.act_i.den_of_evil = {
  is_completed: false,
  is_requirement_completed: true,
  is_received: true,
  closed: false,
  done_recently: false,
};

const outputBytes = Buffer.from(await write(character, constants, CODEC_OPTIONS));
const decoded = await read(outputBytes, constants, CODEC_OPTIONS);
const denOfEvil = decoded.header.quests_normal.act_i.den_of_evil;
assert.equal(decoded.header.level, 99,
  'fixture header level is not 99');
assert.equal(decoded.attributes.level, 99,
  'fixture attribute level is not 99');
assert.equal(decoded.attributes.experience, 3520485254,
  'fixture experience does not match BKVince level 99');
assert.equal(decoded.attributes.unused_skill_points, 1,
  'fixture lost its unspent skill point');
assert.equal(denOfEvil.is_completed, false,
  'fixture unexpectedly marks the Akara reward as granted');
assert.equal(denOfEvil.is_requirement_completed, true,
  'fixture lost Akara RewardPending');
assert.ok(decoded.skills.every((skill) => skill.points === 0),
  'fixture unexpectedly contains an invested rank');

fs.mkdirSync(path.dirname(outputPath), { recursive: true });
fs.writeFileSync(outputPath, outputBytes);

process.stdout.write(`${JSON.stringify({
  status: 'PASS',
  source: sourcePath,
  sourceSha256: sha256(sourceBytes),
  output: outputPath,
  outputBytes: outputBytes.length,
  outputSha256: sha256(outputBytes),
  character: decoded.header.name,
  class: decoded.header.class,
  sourceSkillCount: decoded.skills.length,
  investedRanks: decoded.skills.reduce((sum, skill) => sum + skill.points, 0),
  level: decoded.header.level,
  experience: decoded.attributes.experience,
  unusedStatPoints: decoded.attributes.unused_stats,
  unusedSkillPoints: decoded.attributes.unused_skill_points,
  akaraRewardPending: denOfEvil.is_requirement_completed,
  akaraRewardGranted: denOfEvil.is_completed,
}, null, 2)}\n`);
