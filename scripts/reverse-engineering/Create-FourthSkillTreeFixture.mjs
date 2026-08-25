import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const {
  parseTable,
  serializeTable,
  writeTable,
  ENCODING,
} = require('../build-data/tsv.js');

function fail(message) {
  process.stderr.write(`FourthSkillTree fixture: ${message}\n`);
  process.exit(1);
}

function parseArguments(values) {
  const parsed = {};
  for (let index = 0; index < values.length; index += 2) {
    const key = values[index];
    const value = values[index + 1];
    if (!key?.startsWith('--') || !value) fail('expected --source-excel and --output-root');
    parsed[key.slice(2)] = value;
  }
  if (!parsed['source-excel'] || !parsed['output-root']) {
    fail('expected --source-excel and --output-root');
  }
  parsed['fixture-mode'] ??= 'page4-persistence';
  parsed['probe-class'] ??= 'ama';
  if (!['page4-persistence', 'native-allocation'].includes(parsed['fixture-mode'])) {
    fail('fixture-mode must be page4-persistence or native-allocation');
  }
  if (!['ama', 'bar'].includes(parsed['probe-class'])) {
    fail('probe-class must be ama or bar');
  }
  if (parsed['fixture-mode'] === 'page4-persistence'
      && parsed['probe-class'] !== 'ama') {
    fail('page4-persistence currently supports probe-class ama only');
  }
  return parsed;
}

function findUniqueRow(table, header, value, tableName) {
  const column = table.headers.indexOf(header);
  if (column < 0) fail(`${tableName} is missing ${header}`);
  const matches = table.rows.filter((row) => row[column] === value);
  if (matches.length !== 1) {
    fail(`${tableName} expected one ${header}=${value}, found ${matches.length}`);
  }
  return [...matches[0]];
}

function setCell(table, row, header, value, tableName) {
  const column = table.headers.indexOf(header);
  if (column < 0) fail(`${tableName} is missing ${header}`);
  row[column] = value;
}

function verifySource(filePath, table) {
  const raw = fs.readFileSync(filePath, ENCODING);
  assert.equal(serializeTable(table), raw, `round-trip mismatch: ${filePath}`);
  assert.equal(table.eol, '\r\n', `expected CRLF: ${filePath}`);
}

function verifyOutput(filePath) {
  const raw = fs.readFileSync(filePath, ENCODING);
  const parsed = parseTable(filePath);
  assert.equal(serializeTable(parsed), raw, `output round-trip mismatch: ${filePath}`);
  assert.equal(parsed.eol, '\r\n', `output lost CRLF: ${filePath}`);
  assert.equal(parsed.hasFinalEol, true, `output lost final CRLF: ${filePath}`);
}

const argumentsByName = parseArguments(process.argv.slice(2));
const fixtureMode = argumentsByName['fixture-mode'];
const probeClass = argumentsByName['probe-class'];
const repositoryRoot = path.resolve(import.meta.dirname, '..', '..');
const sourceExcel = path.resolve(argumentsByName['source-excel']);
const outputRoot = path.resolve(argumentsByName['output-root']);
const allowedOutputRoot = path.join(
  repositoryRoot,
  'analysis-cache',
  'fourth-skill-tree-fixture');
const relativeOutput = path.relative(allowedOutputRoot, outputRoot);
if (relativeOutput.startsWith('..') || path.isAbsolute(relativeOutput)) {
  fail(`output must stay under ${allowedOutputRoot}`);
}
if (path.resolve(sourceExcel) === outputRoot) fail('source and output must differ');

const sourceSkills = path.join(sourceExcel, 'skills.txt');
const sourceSkillDesc = path.join(sourceExcel, 'skilldesc.txt');
for (const filePath of [sourceSkills, sourceSkillDesc]) {
  if (!fs.statSync(filePath, { throwIfNoEntry: false })?.isFile()) {
    fail(`missing source table: ${filePath}`);
  }
}

const skills = parseTable(sourceSkills);
const skillDesc = parseTable(sourceSkillDesc);
verifySource(sourceSkills, skills);
verifySource(sourceSkillDesc, skillDesc);

const skillProbeName = 'Fourth Skill Tree Probe';
const skillDescProbeName = 'fourth skill tree probe';
const skillColumn = skills.headers.indexOf('skill');
const descriptionKeyColumn = skillDesc.headers.indexOf('skilldesc');
if (skills.rows.some((row) => row[skillColumn] === skillProbeName)
    || skillDesc.rows.some((row) => row[descriptionKeyColumn] === skillDescProbeName)) {
  fail('source already contains the synthetic probe keys');
}

const nativeAllocationMode = fixtureMode === 'native-allocation';
const fixture = nativeAllocationMode && probeClass === 'bar'
  ? {
      sourceSkill: 'Bash',
      sourceSkillDesc: 'bash',
      page: '1',
      row: '1',
      column: '1',
    }
  : nativeAllocationMode
    ? {
        sourceSkill: 'Inner Sight',
        sourceSkillDesc: 'inner sight',
        page: '2',
        row: '2',
        column: '1',
      }
    : {
        sourceSkill: 'Pierce',
        sourceSkillDesc: 'pierce',
        page: '4',
        row: '1',
        column: '1',
      };
const sourceSkill = fixture.sourceSkill;
const skillProbe = findUniqueRow(skills, 'skill', sourceSkill, 'skills.txt');
setCell(skills, skillProbe, 'skill', skillProbeName, 'skills.txt');
setCell(skills, skillProbe, 'skilldesc', skillDescProbeName, 'skills.txt');
setCell(skills, skillProbe, 'reqskill1', nativeAllocationMode ? '' : 'Pierce', 'skills.txt');
setCell(skills, skillProbe, 'reqskill2', '', 'skills.txt');
setCell(skills, skillProbe, 'reqskill3', '', 'skills.txt');
skills.rows.push(skillProbe);

const descriptionProbe = findUniqueRow(
  skillDesc,
  'skilldesc',
  fixture.sourceSkillDesc,
  'skilldesc.txt');
setCell(skillDesc, descriptionProbe, 'skilldesc', skillDescProbeName, 'skilldesc.txt');
setCell(skillDesc, descriptionProbe, 'SkillPage', fixture.page, 'skilldesc.txt');
setCell(skillDesc, descriptionProbe, 'SkillRow', fixture.row, 'skilldesc.txt');
setCell(skillDesc, descriptionProbe, 'SkillColumn', fixture.column, 'skilldesc.txt');
skillDesc.rows.push(descriptionProbe);

fs.mkdirSync(outputRoot, { recursive: true });
const outputSkills = path.join(outputRoot, 'skills.txt');
const outputSkillDesc = path.join(outputRoot, 'skilldesc.txt');
writeTable(outputSkills, skills);
writeTable(outputSkillDesc, skillDesc);
verifyOutput(outputSkills);
verifyOutput(outputSkillDesc);

process.stdout.write([
  'FourthSkillTree fixture=PASS',
  `source=${sourceExcel}`,
  `output=${outputRoot}`,
  `skills=${skills.rows.length}`,
  `skilldesc=${skillDesc.rows.length}`,
  `probe-skill-id=${skills.rows.length - 1}`,
  `probe-class=${probeClass}`,
  `fixture-mode=${fixtureMode}`,
  `probe-page=${fixture.page}`,
  `probe-source=${sourceSkill}`,
].join(' ') + '\n');
