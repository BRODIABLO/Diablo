import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';

const EXPECTED_SOURCE_SHA256S = new Set([
  '87C529363662D5F663047A29C3F92EA13BA7D8F6052AB46CBE0193385159F515',
  'A28F46040B65100B1110967365DC617F3FC8EBEFEBD75C50BDAB890939B3C030',
]);
const LEGACY_SKILL_COUNT = 30;
const SKILL_COUNT_OFFSET = 0x1a;

function fail(message) {
  throw new Error(`FourthSkillTree save expansion: ${message}`);
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

function fixHeader(bytes) {
  bytes.writeUInt32LE(bytes.length, 0x08);
  bytes.writeUInt32LE(0, 0x0c);
  let checksum = 0;
  for (const originalByte of bytes) {
    let byte = originalByte;
    if ((checksum & 0x80000000) !== 0) byte += 1;
    checksum = (byte + checksum * 2) >>> 0;
  }
  bytes.writeUInt32LE(checksum, 0x0c);
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
assert.ok(
  EXPECTED_SOURCE_SHA256S.has(sha256(sourceBytes)),
  'unexpected QtyTester source save');

const candidates = [];
for (let offset = 0; offset <= sourceBytes.length - 34; offset += 1) {
  if (sourceBytes[offset] !== 0x69 || sourceBytes[offset + 1] !== 0x66) continue;
  const nextSection = offset + 2 + LEGACY_SKILL_COUNT;
  if (sourceBytes[nextSection] === 0x4a && sourceBytes[nextSection + 1] === 0x4d) {
    candidates.push(offset);
  }
}
assert.deepEqual(candidates.length, 1, 'expected one 30-skill block followed by JM');

const skillHeaderOffset = candidates[0];
const insertionOffset = skillHeaderOffset + 2 + LEGACY_SKILL_COUNT;
const outputBytes = Buffer.concat([
  sourceBytes.subarray(0, insertionOffset),
  Buffer.from([0]),
  sourceBytes.subarray(insertionOffset),
]);
assert.equal(
  outputBytes[SKILL_COUNT_OFFSET],
  LEGACY_SKILL_COUNT,
  'source NumSkills is not 30');
outputBytes[SKILL_COUNT_OFFSET] = LEGACY_SKILL_COUNT + 1;
fixHeader(outputBytes);

assert.equal(outputBytes.length, sourceBytes.length + 1, 'save did not grow by one byte');
assert.equal(outputBytes[insertionOffset], 0, '31st skill rank is not zero');
assert.equal(outputBytes[insertionOffset + 1], 0x4a, 'JM marker shifted incorrectly');
assert.equal(outputBytes[insertionOffset + 2], 0x4d, 'JM marker shifted incorrectly');
assert.equal(outputBytes.readUInt32LE(0x08), outputBytes.length, 'header size mismatch');
assert.equal(outputBytes[SKILL_COUNT_OFFSET], 31, 'header NumSkills mismatch');

fs.mkdirSync(path.dirname(outputPath), { recursive: true });
fs.writeFileSync(outputPath, outputBytes);

process.stdout.write(`${JSON.stringify({
  status: 'PASS',
  source: sourcePath,
  sourceSha256: sha256(sourceBytes),
  output: outputPath,
  outputBytes: outputBytes.length,
  outputSha256: sha256(outputBytes),
  skillHeaderOffset,
  legacySkillCount: LEGACY_SKILL_COUNT,
  expandedSkillCount: LEGACY_SKILL_COUNT + 1,
  numSkillsOffset: SKILL_COUNT_OFFSET,
  insertedRank: outputBytes[insertionOffset],
  nextSection: outputBytes.subarray(insertionOffset + 1, insertionOffset + 3).toString('ascii'),
}, null, 2)}\n`);
