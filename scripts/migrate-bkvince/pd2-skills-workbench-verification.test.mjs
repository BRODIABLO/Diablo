import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';
import { spawnSync } from 'node:child_process';
import test from 'node:test';
import vm from 'node:vm';
import { fileURLToPath } from 'node:url';

import {
  BEHAVIOR_GROUPS,
  CLASS_NAMES,
  FROZEN_CONTRACT_HASH,
  MAPPING_TYPES,
  NON_MUTATION_RULES,
  PLAYER_CLASS_CODES,
  PORTABILITY_CATEGORIES,
  PROOF_STATUSES,
  physicalNodeId,
  sha256Canonical,
} from './pd2-skills-review-contracts.mjs';
import {
  DEFAULT_SOURCE_ROOTS,
  buildOracleData,
  loadWorkbenchSources,
} from './pd2-skills-review-data.mjs';
import {
  applyBulk,
  createEmptyEnvelope,
  createEntry,
  entryState,
  storageKey,
  validateImport,
} from './pd2-skills-review-runtime.mjs';
import { generateSkillReviewArtifacts } from './pd2-skills-review.mjs';

const require = createRequire(import.meta.url);
const { ENCODING, parseTable, serializeTable } = require('../build-data/tsv.js');
const repoRoot = fileURLToPath(new URL('../../', import.meta.url));
const previewPath = path.join(repoRoot, 'scripts', 'migrate-bkvince', 'pd2-skills-decisions-preview.mjs');
const missionPath = path.join(repoRoot, 'Mission', 'pd2-skills-merge.md');

const EXPECTED_SKILLS_SOURCES = Object.freeze({
  pd2: Object.freeze({
    path: path.join(DEFAULT_SOURCE_ROOTS.pd2, 'Skills.txt'),
    rows: 603,
    columns: 256,
    eol: '\n',
    sha256: 'AEEFC3F2C0C80811D62FC1A17C3B031DE2164E5606BF9779F34024B35BC87B8B',
  }),
  bkvince: Object.freeze({
    path: path.join(repoRoot, 'data-BKVince', 'BKVince.mpq', 'data', 'global', 'excel', 'skills.txt'),
    rows: 451,
    columns: 322,
    eol: '\r\n',
    sha256: '08497CC0BD8B2B5CBD895F7477AD0CBF272571FB67B78061EDFABC31C48B8B77',
  }),
  vanilla32: Object.freeze({
    path: path.join(repoRoot, 'data-vanilla3.2', 'data', 'data', 'global', 'excel', 'skills.txt'),
    rows: 429,
    columns: 322,
    eol: '\r\n',
    sha256: 'EFAF7AC4BA0493109C698EF32ACF4A2B3A577E13500D0B50258C80B600986F51',
  }),
});

function sha256(value) {
  return crypto.createHash('sha256').update(value).digest('hex').toUpperCase();
}

const baselineBytes = Object.fromEntries(
  Object.entries(EXPECTED_SKILLS_SOURCES).map(([source, expected]) => [source, fs.readFileSync(expected.path)]),
);
const artifacts = generateSkillReviewArtifacts();
const { report } = artifacts;
const skillsById = new Map(report.skills.map((skill) => [skill.stableId, skill]));
const nodesById = new Map(report.nodes.map((node) => [node.id, node]));

function approvedSchemaPolicy() {
  const policy = structuredClone(createEmptyEnvelope(report).schemaPolicy);
  for (const entry of Object.values(policy.decisions)) {
    entry.decision = 'APPROVE';
    entry.justification = 'Verification fixture explicitly approves the governed Phase 0 policy.';
  }
  policy.exportedAt = '2026-08-12T00:00:00.000Z';
  return policy;
}

function normalized(value) {
  return String(value ?? '').trim().toLocaleLowerCase('en');
}

function unique(values) {
  return [...new Set(values)];
}

function hashLeaves(value, result = []) {
  if (typeof value === 'string' && /^[A-F0-9]{64}$/i.test(value)) result.push(value.toUpperCase());
  else if (Array.isArray(value)) value.forEach((item) => hashLeaves(item, result));
  else if (value && typeof value === 'object') Object.values(value).forEach((item) => hashLeaves(item, result));
  return result;
}

function nodeIdsOf(skill) {
  return Object.values(skill.nodeIds ?? {}).flat().filter((value) => typeof value === 'string');
}

function nodeOf(skill, source) {
  const value = skill.nodeIds?.[source];
  const id = Array.isArray(value) ? value[0] : value;
  return id ? nodesById.get(id) : null;
}

function allFields(skill) {
  return (skill.components ?? []).flatMap((component) => (
    (component.fields ?? []).map((field) => ({ component, field }))
  ));
}

function findSkill(name) {
  const wanted = normalized(name);
  const matches = report.skills.filter((skill) => unique([
    skill.canonicalName,
    ...(skill.aliases ?? []),
    ...Object.values(skill.names ?? {}),
  ]).some((value) => normalized(value) === wanted));
  assert.equal(matches.length, 1, `${name} must resolve to one canonical skill, got ${matches.length}`);
  return matches[0];
}

function skillText(skill) {
  return JSON.stringify(skill);
}

function fieldsMatching(skill, pattern) {
  return allFields(skill).filter(({ field }) => pattern.test([
    field.id, field.table, field.header, field.label,
  ].join(' ')));
}

function proofStatusesOf(skill) {
  const result = [
    skill.evidence?.overall,
    skill.evidence?.status,
    skill.evidence?.proofStatus,
    skill.proofStatus,
  ];
  for (const { component, field } of allFields(skill)) {
    result.push(component.proofStatus, field.proofStatus, field.formula?.status);
  }
  for (const nodeId of nodeIdsOf(skill)) {
    for (const finding of nodesById.get(nodeId)?.formulaFindings ?? []) result.push(finding.status, finding.proofStatus);
  }
  return unique(result.filter(Boolean));
}

function portabilityOf(value) {
  const portability = value?.portability ?? value;
  if (Array.isArray(portability)) return unique(portability.map((item) => (
    typeof item === 'string' ? item : item?.category ?? item?.classification
  )).filter(Boolean));
  if (typeof portability === 'string') return [portability];
  if (!portability || typeof portability !== 'object') return [];
  return unique([
    ...(portability.categories ?? []),
    ...(portability.classifications ?? []),
    portability.category,
    portability.classification,
  ].flat().filter(Boolean));
}

function scenariosOf(skill) {
  const curves = skill.curves ?? {};
  if (Array.isArray(curves.scenarios)) return curves.scenarios;
  if (curves.scenarios && typeof curves.scenarios === 'object') {
    return Object.entries(curves.scenarios).map(([id, value]) => ({ id, ...value }));
  }
  if (curves.standard) return [{ id: 'standard', ...curves.standard }];
  return [];
}

function seriesOf(scenario) {
  if (Array.isArray(scenario.series)) return scenario.series;
  const values = scenario.metrics ?? scenario.values ?? {};
  return Object.entries(values).map(([id, value]) => ({ id, ...(value ?? {}) }));
}

function comparisonValue(field, source) {
  return (field.values ?? field.displayValues ?? {})[source];
}

function coordinate(skill) {
  const tree = skill.tree ?? {};
  const number = (value) => Number.isFinite(Number(value)) ? Number(value) : Number.MAX_SAFE_INTEGER;
  return [number(tree.page), number(tree.row), number(tree.column)];
}

function compareCoordinates(left, right) {
  for (let index = 0; index < Math.max(left.length, right.length); index += 1) {
    if (left[index] !== right[index]) return left[index] - right[index];
  }
  return 0;
}

test('skills TSV authorities round-trip byte-exact with frozen row counts and hashes', () => {
  for (const [source, expected] of Object.entries(EXPECTED_SKILLS_SOURCES)) {
    const rawText = fs.readFileSync(expected.path, ENCODING);
    const rawBytes = fs.readFileSync(expected.path);
    const parsed = parseTable(expected.path);
    assert.equal(serializeTable(parsed), rawText, `${source} round-trip`);
    assert.equal(parsed.rows.length, expected.rows, `${source} rows`);
    assert.equal(parsed.headers.length, expected.columns, `${source} columns`);
    assert.equal(parsed.eol, expected.eol, `${source} EOL`);
    assert.equal(sha256(rawBytes), expected.sha256, `${source} SHA-256`);
  }
  const governedHashes = new Set(hashLeaves(report.sourceHashes));
  for (const [label, hash] of Object.entries({
    'Vanilla skillcalc.txt': '4028D9985368B89FE5D84A93F60177ECC34957599A170A4F8496DDD169636A4F',
    'BKVince skills.json': '73E6C164F484B31372A5D91B4D0FFE65888C828CC7703E4D811378679FC48268',
    'PD2 patchstring.tbl': 'EC023659BFA1BA0E3FFADFBEECAD344D750E21E18DBDA1AA287D6D2835DC1107',
  })) assert(governedHashes.has(hash), `${label} is missing from governed sourceHashes`);
  assert.match(JSON.stringify(report.sourceManifest?.bkvince?.tables?.['pettype.txt']), /INHERITED_VANILLA32/);
});

test('PD2_SP_ROOT and canonical Skills.txt casing are honored', () => {
  if (process.env.PD2_SP_ROOT) {
    assert.equal(path.resolve(DEFAULT_SOURCE_ROOTS.pd2), path.resolve(process.env.PD2_SP_ROOT));
  }
  assert.equal(EXPECTED_SKILLS_SOURCES.pd2.path, path.join(DEFAULT_SOURCE_ROOTS.pd2, 'Skills.txt'));
  assert(fs.readdirSync(DEFAULT_SOURCE_ROOTS.pd2).includes('Skills.txt'), 'PD2 source must expose canonical Skills.txt');
  if (process.platform !== 'win32') {
    assert.equal(fs.existsSync(path.join(DEFAULT_SOURCE_ROOTS.pd2, 'skills.txt')), false, 'lowercase compatibility path must not be required');
  }
});

test('oracle covers every physical ordinal exactly once and never uses documentary Id as identity', () => {
  assert.equal(report.frozenContractHash, FROZEN_CONTRACT_HASH);
  assert.deepEqual(report.levels, [1, 5, 10, 20, 30, 40]);
  const legacySkills = new Map(buildOracleData(loadWorkbenchSources(DEFAULT_SOURCE_ROOTS)).skills
    .map((skill) => [skill.stableId, skill]));
  const owners = new Map();
  for (const skill of report.skills) {
    assert.match(skill.stableId, /^skill:[a-z0-9-]+:[a-z0-9-]+(?::[a-z0-9-]+)?$/);
    assert.match(skill.fingerprint, /^[A-F0-9]{64}$/);
    const expectedFingerprint = sha256Canonical({
      previousFingerprint: legacySkills.get(skill.stableId).fingerprint,
      decisionBundles: skill.decisionBundles,
      policyApplication: skill.policyApplication,
      curves: skill.curves,
    });
    assert.equal(skill.fingerprint, expectedFingerprint, `${skill.stableId}: fingerprint does not cover its final mapping/data/protections`);
    for (const nodeId of nodeIdsOf(skill)) {
      assert(nodesById.has(nodeId), `${skill.stableId} references missing ${nodeId}`);
      assert(!owners.has(nodeId), `${nodeId} belongs to both ${owners.get(nodeId)} and ${skill.stableId}`);
      owners.set(nodeId, skill.stableId);
    }
  }
  assert.equal(owners.size, report.nodes.length, 'every physical node has one semantic owner');

  for (const [source, expected] of Object.entries(EXPECTED_SKILLS_SOURCES)) {
    const nodes = report.nodes.filter((node) => node.source === source);
    assert.equal(nodes.length, expected.rows, `${source} physical coverage`);
    assert.deepEqual(nodes.map((node) => node.ordinal).sort((a, b) => a - b), Array.from({ length: expected.rows }, (_, index) => index));
    for (const node of nodes) {
      assert.equal(node.id, physicalNodeId(source, node.ordinal));
      assert.equal(node.ordinal, node.line - 2);
      assert.match(node.rowFingerprint, /^[A-F0-9]{64}$/);
    }
  }
  assert.equal(report.nodes.length, 603 + 451 + 429);
  const documentaryMismatch = report.nodes.find((node) => node.source === 'bkvince' && node.name === 'Concentration');
  assert(documentaryMismatch);
  assert.notEqual(String(documentaryMismatch.declaredId), String(documentaryMismatch.ordinal));
  assert.equal(documentaryMismatch.id, `bkvince:skills.txt:${documentaryMismatch.ordinal}`);
});

test('semantic mapping is unambiguous while semantic identity and runtime collisions stay independent', () => {
  assert.equal(new Set(report.skills.map((skill) => skill.stableId)).size, report.skills.length);
  assert.equal(new Set(report.nodes.map((node) => node.id)).size, report.nodes.length);
  assert.equal(report.coverage?.mappingAmbiguities ?? report.coverage?.ambiguousMappings ?? report.coverage?.ambiguous ?? 0, 0);
  for (const skill of report.skills) {
    assert(skill.mappingTypes.length > 0, `${skill.stableId} has no mapping`);
    for (const mapping of skill.mappingTypes) assert(MAPPING_TYPES.includes(mapping), `${skill.stableId}: ${mapping}`);
    if (skill.mappingTypes.some((mapping) => /^(?:SAME_SKILL_|RENAMED_ALIAS|IDENTICAL)/.test(mapping))) {
      const evidence = skill.semanticIdentityEvidence ?? skill.mappingEvidence;
      assert(evidence && typeof evidence === 'object', `${skill.stableId}: semantic edge lacks evidence`);
      assert.equal(evidence.status, 'PROVEN', `${skill.stableId}: semantic edge is not proven`);
      const signals = evidence.signals ?? evidence.reasons ?? evidence.axes ?? [];
      assert(Array.isArray(signals) && signals.length > 0, `${skill.stableId}: semantic proof signals missing`);
      const independent = signals.some((item) => !/^(?:SAME_)?(?:NORMALIZED_)?NAME$/i.test(String(item.type ?? item.kind ?? item)));
      assert(independent, `${skill.stableId}: identical name is the only semantic evidence`);
    }
    if (skill.playerSkill && !skill.identical) {
      const summary = typeof skill.summary === 'string' ? skill.summary : skill.summary?.player ?? skill.summary?.text ?? '';
      assert(summary.trim(), `${skill.stableId}: player summary missing`);
      assert.doesNotMatch(summary, /^\d+\s+diff[eé]rences?\s+techniques?/i, `${skill.stableId}: summary is only a raw cell count`);
    }
  }

  assert.equal(report.collisions.length, 110, 'HEAD must explain the two collisions added after the 108-collision audit');
  assert.equal(new Set(report.collisions.map((collision) => collision.id)).size, report.collisions.length);
  for (const collision of report.collisions) {
    assert.match(collision.fingerprint, /^[A-F0-9]{64}$/);
    assert.equal(collision.kind, 'SAME_ORDINAL_DIFFERENT_SKILL');
    assert.notEqual(normalized(collision.names?.pd2), normalized(collision.names?.bkvince));
    for (const stableId of Object.values(collision.skillIds ?? {}).filter(Boolean)) assert(skillsById.has(stableId), `${collision.id}: unknown ${stableId}`);
  }
  const mission = fs.readFileSync(missionPath, 'utf8');
  assert.match(mission, /449 lignes(?: BKVince)? et 108 collisions/);
  assert.match(mission, /451 lignes et \*\*110 collisions\*\*/);

  const coldEnchant = findSkill('Cold Enchant');
  assert.equal(coldEnchant.ordinals.pd2, 40);
  assert.equal(coldEnchant.ordinals.bkvince, 408);
  assert(coldEnchant.mappingTypes.includes('SAME_SKILL_MOVED_ORDINAL'));
  assert((coldEnchant.collisionIds ?? []).length > 0);
  const collisions = (coldEnchant.collisionIds ?? []).map((id) => report.collisions.find((item) => item.id === id));
  assert(collisions.some((collision) => /Frozen Armor/i.test(JSON.stringify(collision))), 'PD2 ordinal 40 must collide with BKVince Frozen Armor');
});

test('navigation covers all player skills and preserves governed class/tree ordering', () => {
  const requiredViews = ['ama', 'sor', 'nec', 'pal', 'bar', 'dru', 'ass', 'pd2_new', 'bkv_only', 'collisions', 'technical'];
  const views = new Map(report.navigation.map((view) => [view.id, view]));
  for (const id of requiredViews) assert(views.has(id), `missing navigation view ${id}`);
  const navigated = new Set(report.navigation.flatMap((view) => [
    ...(view.skillIds ?? []),
    ...(view.trees ?? []).flatMap((tree) => tree.skillIds ?? []),
  ]));
  for (const skill of report.skills.filter((item) => item.playerSkill)) {
    assert(navigated.has(skill.stableId), `${skill.stableId} player skill is not navigable`);
  }
  for (const code of PLAYER_CLASS_CODES) {
    const view = views.get(code);
    assert.equal(view.label, CLASS_NAMES[code]);
    assert.equal((view.trees ?? []).length, 3, `${code} must expose its three real trees`);
    for (const tree of view.trees) {
      const positions = (tree.skillIds ?? []).map((id) => coordinate(skillsById.get(id)));
      for (let index = 1; index < positions.length; index += 1) {
        assert(compareCoordinates(positions[index - 1], positions[index]) <= 0, `${code}/${tree.id} is not ordered by skilldesc coordinates`);
      }
    }
  }
  const forbiddenTechnical = /\b(?:amatemp\d*|bartemp\d*|drutemp\d*|asatemp\d*|proc|monster|merc(?:enary)?|helper|map|test)\b/i;
  for (const code of PLAYER_CLASS_CODES) {
    const contaminated = (views.get(code).skillIds ?? []).map((id) => skillsById.get(id)).filter((skill) => forbiddenTechnical.test(skill?.canonicalName ?? ''));
    assert.deepEqual(contaminated.map((skill) => skill.canonicalName), [], `${code} player navigation contains technical lines`);
  }
});

test('new PD2 candidates exclude technical lines and receive unique append-only plans with closure gates', () => {
  const candidates = report.skills.filter((skill) => skill.newPd2PlayerSkill);
  assert.equal(candidates.length, 15, 'governed player/tree classification must retain all real PD2-only candidates');
  const proposed = [];
  for (const skill of candidates) {
    assert(PLAYER_CLASS_CODES.includes(skill.classCode), `${skill.stableId}: invalid player class`);
    assert.equal(Boolean(nodeOf(skill, 'pd2')), true);
    assert.equal(Boolean(nodeOf(skill, 'bkvince')), false);
    assert(skill.mappingTypes.includes('PD2_ONLY_PLAYER_SKILL'));
    assert.doesNotMatch(skill.canonicalName, /\b(proc|monster|merc(?:enary)?|helper|map|test|temp(?:orary)?)\b/i);
    const plan = skill.newSkillPlan;
    assert(plan, `${skill.stableId}: missing newSkillPlan`);
    const target = plan.proposedTargetOrdinal ?? plan.appendOnlyOrdinal;
    assert(Number.isInteger(target) && target >= 451, `${skill.stableId}: invalid append-only target ${target}`);
    proposed.push(target);
    for (const concept of ['skilldesc', 'native', 'test', 'remap']) {
      assert(skillText(skill).toLocaleLowerCase('en').includes(concept), `${skill.stableId}: plan omits ${concept}`);
    }
    assert(/strings?|locali[sz]ation/i.test(skillText(skill)), `${skill.stableId}: plan omits strings/localization`);
    assert(plan.proposedRow && typeof plan.proposedRow === 'object', `${skill.stableId}: append-only preview lacks a projected BKVince-schema row`);
    const projectedValues = plan.proposedRow.values ?? plan.proposedRow;
    assert.equal(Object.keys(projectedValues).length, 322, `${skill.stableId}: projected append row must match the BKVince 322-column schema`);
    if (plan.proposedRow.values) {
      assert.equal(plan.proposedRow.targetOrdinal, target);
      assert.deepEqual(Object.keys(plan.proposedRow.mappingProvenance ?? {}).sort(), Object.keys(projectedValues).sort(), `${skill.stableId}: every projected cell needs provenance`);
      const documentaryId = Object.entries(plan.proposedRow.mappingProvenance ?? {}).find(([header]) => /^\*?id$/i.test(header));
      assert(documentaryId, `${skill.stableId}: projected documentary Id provenance missing`);
      assert.equal(documentaryId[1].mode, 'APPEND_PREVIEW_DOCUMENTARY_VALUE', `${skill.stableId}: documentary Id must never allocate the append ordinal`);
    }
    const consumerClosure = plan.consumerClosure ?? skill.consumerClosure;
    assert(consumerClosure && typeof consumerClosure === 'object', `${skill.stableId}: consumer coverage must be explicit`);
    const consumerStatus = String(consumerClosure.status ?? consumerClosure.proofStatus ?? '');
    assert.match(consumerStatus, /VERIFIED|EXACT|UNMAPPED|NATIVE_UNPROVEN|INCOMPLETE/);
    if (/UNMAPPED|NATIVE_UNPROVEN|INCOMPLETE/.test(consumerStatus)) {
      assert(portabilityOf(skill).some((category) => ['BLOCKED_DEPENDENCY', 'NATIVE_UNPROVEN'].includes(category)), `${skill.stableId}: unproven consumer closure must block portability`);
      assert.notEqual(plan.dependencyClosure?.completeForBkvince, true, `${skill.stableId}: consumer uncertainty cannot claim complete BKVince closure`);
    }
    const open = (skill.dependencies ?? []).filter((dependency) => dependency.closed === false);
    if (open.length) assert(portabilityOf(skill).includes('BLOCKED_DEPENDENCY'), `${skill.stableId}: open dependency without BLOCKED_DEPENDENCY`);
    assert.equal(plan.dependencyClosure?.completeForBkvince, false, `${skill.stableId}: direct source references must not claim transitive BKVince closure`);
    assert.equal(plan.localizationClosure?.complete, false, `${skill.stableId}: binary localization without governed text must stay open`);
    assert.equal(plan.consumerClosure?.complete, false, `${skill.stableId}: unproven ordinal consumers must stay open`);
    for (const gate of ['LINKED_TABLE_TARGETS', 'LOCALIZATION', 'ORDINAL_CONSUMERS_NATIVE_UNPROVEN']) {
      assert(plan.dependencyClosure?.blockingGates?.includes(gate), `${skill.stableId}: missing blocking gate ${gate}`);
    }
  }
  assert.equal(new Set(proposed).size, proposed.length, 'append-only preview ordinals collide');
  assert.deepEqual([...proposed].sort((a, b) => a - b), Array.from({ length: proposed.length }, (_, index) => 451 + index));

  const technicalIds = new Set(report.navigation.find((view) => view.id === 'technical').skillIds ?? []);
  for (const skill of candidates) assert(!technicalIds.has(skill.stableId), `${skill.stableId} is both new player and technical`);
  const dopplezon = findSkill('Dopplezon');
  assert.equal(dopplezon.newPd2PlayerSkill, false);
  assert.equal(dopplezon.playerSkill, true, 'Dopplezon is the governed internal name of the existing Amazon Decoy player skill');
  assert(!technicalIds.has(dopplezon.stableId), 'the existing Amazon Decoy skill must not be quarantined as technical');
  const chainSentry = findSkill('Chain Lightning Sentry');
  const lightningSentry = findSkill('Lightning Sentry');
  const curseMastery = findSkill('Curse Mastery');
  assert.equal(curseMastery.names.pd2, 'CurMas', 'raw PD2 name remains documented behind the governed player alias');
  assert(curseMastery.newPd2PlayerSkill);
  assert(chainSentry.newPd2PlayerSkill, 'Chain Lightning Sentry must remain a distinct player candidate');
  assert.equal(chainSentry.ordinals.pd2, 271);
  assert.equal(lightningSentry.ordinals.pd2, 366);
  assert.equal(lightningSentry.ordinals.bkvince, 271);
  assert(lightningSentry.mappingTypes.includes('SAME_SKILL_MOVED_ORDINAL'));
});

test('mandatory witnesses retain exact mapping, malformed, symbolic and linked-table evidence', () => {
  const amplify = findSkill('Amplify Damage');
  assert.match(skillText(amplify), /Cur(?:se )?Mas(?:tery)?/i);
  for (const concept of [/mana/i, /radius|rayon|aurarange/i, /duration|dur[eé]e|auralen/i, /damage|d[eé]g[aâ]ts|damageresist/i]) {
    assert(concept.test(skillText(amplify)), `Amplify Damage omits ${concept}`);
  }

  const fireBall = findSkill('Fire Ball');
  assert(proofStatusesOf(fireBall).includes('MALFORMED_SOURCE'));
  const fireBallMalformed = (fireBall.evidence?.findings ?? []).find((finding) => finding.raw === "min(3,1+skill('Fire Ball'.blvl)/10");
  assert(fireBallMalformed, 'exact malformed multishot formula is missing');
  assert.equal(fireBallMalformed.status, 'MALFORMED_SOURCE');
  assert(!(fireBall.evidence?.findings ?? []).some((finding) => finding.raw === "min(3,1+skill('Fire Ball'.blvl)/10)"), 'malformed source must not be repaired');

  const fireWall = findSkill('Fire Wall');
  assert(proofStatusesOf(fireWall).includes('MALFORMED_SOURCE'));
  const fireWallMalformed = (fireWall.evidence?.findings ?? []).find((finding) => finding.raw === "(skill('Warmth'.blvl)*par8+skill('Inferno'.blvl)*par7");
  assert(fireWallMalformed, 'exact Vanilla Fire Wall malformed formula is missing');
  assert.equal(fireWallMalformed.status, 'MALFORMED_SOURCE');
  const pd2Delay = fieldsMatching(fireWall, /(^|\W)delay(\W|$)/i).some(({ field }) => String(comparisonValue(field, 'pd2')) === '38');
  const bkvLocal = fieldsMatching(fireWall, /localdelay/i).some(({ field }) => String(comparisonValue(field, 'bkvince')) === '15');
  assert(pd2Delay, 'Fire Wall PD2 delay=38 must remain a distinct source field');
  assert(bkvLocal, 'Fire Wall BKVince localdelay=15 must remain a distinct source field');

  const combustion = findSkill('Combustion');
  assert(combustion.newPd2PlayerSkill);
  assert.equal(combustion.newSkillPlan?.currentOccupantAtPd2Ordinal?.name, 'Summon Tainted');
  assert((combustion.newSkillPlan?.proposedTargetOrdinal ?? combustion.newSkillPlan?.appendOnlyOrdinal) >= 451);

  const barrage = findSkill('Ice Barrage');
  assert.match(skillText(barrage), /MonHolyShock/i);
  assert.match(skillText(barrage), /missiles?\.txt/i);
  assert.match(skillText(barrage), /skilldesc\.txt/i);

  const raven = findSkill('Raven');
  assert(proofStatusesOf(raven).some((status) => ['SYMBOLIC', 'UNSUPPORTED_IDENTIFIER'].includes(status)));
  assert.match(skillText(raven), /ulvl \+ par1 \+ lvl/);

  const hydra = findSkill('Hydra');
  for (const dependency of [/pettype/i, /monstats/i, /summon/i, /master(?:y|ies)/i]) {
    assert(dependency.test(skillText(hydra)), `Hydra omits ${dependency}`);
  }

  const frozenOrb = findSkill('Frozen Orb');
  for (const behavior of [/missile/i, /speed|velocity|vitesse/i, /range|port[eé]e/i, /explosion/i]) {
    assert(behavior.test(skillText(frozenOrb)), `Frozen Orb omits ${behavior}`);
  }

  assert(report.skills.some((skill) => skill.identical && skill.readOnly), 'missing identical auto-resolved skill');
  assert(report.skills.some((skill) => skill.classCode === 'war' && skill.bkvinceOnlyPlayerSkill), 'missing BKVince-only Warlock player skill');
});

test('curves use only governed levels and never manufacture malformed, symbolic or native values', () => {
  const scenarioSkills = report.skills.filter((skill) => scenariosOf(skill).length);
  assert(scenarioSkills.length > 0, 'no curve scenarios generated');
  for (const skill of scenarioSkills) {
    for (const scenario of scenariosOf(skill)) {
      assert.deepEqual(scenario.levels ?? report.levels, [1, 5, 10, 20, 30, 40], `${skill.stableId}/${scenario.id}`);
      for (const series of seriesOf(scenario)) {
        const values = series.values ?? {};
        const arrays = Array.isArray(values) ? [values] : Object.values(values).filter(Array.isArray);
        for (const points of arrays) assert.equal(points.length, 6, `${skill.stableId}/${scenario.id}/${series.id}`);
      }
    }
  }
  const fireBall = findSkill('Fire Ball');
  assert.deepEqual(scenariosOf(fireBall).map((scenario) => scenario.id), ['standard', 'synergies20', 'custom']);
  const standardMetrics = seriesOf(scenariosOf(fireBall)[0]);
  assert(standardMetrics.length > 0, 'standard scenario must expose UI-consumable metric series');
  for (const metric of ['damageMin', 'damageMax', 'damageAverage', 'mana']) {
    const canonical = (value) => normalized(value).replace(/[^a-z0-9]/g, '');
    assert(standardMetrics.some((series) => canonical(series.id) === canonical(metric)), `standard curves omit ${metric}`);
  }
  for (const skill of scenarioSkills) for (const scenario of scenariosOf(skill)) for (const series of seriesOf(scenario)) {
    const values = Object.values(series.values ?? {}).filter(Array.isArray).flat();
    assert(values.some((value) => value !== null && value !== undefined), `${skill.stableId}/${scenario.id}/${series.id} is entirely inapplicable`);
  }
  const raven = findSkill('Raven');
  assert(scenariosOf(raven).some((scenario) => (scenario.symbolic ?? []).some((item) => /ulvl/i.test(JSON.stringify(item)))), 'Raven ulvl must remain symbolic');
  for (const skill of [fireBall, findSkill('Fire Wall')]) {
    const malformedText = JSON.stringify(scenariosOf(skill).flatMap((scenario) => scenario.symbolic ?? []));
    assert.match(malformedText, /MALFORMED_SOURCE|malform/i);
  }
});

test('three-way fields, proof and portability enums are structurally complete', () => {
  for (const key of ['mappingTypes', 'proofStatuses', 'portabilityCategories', 'globalDecisions', 'componentDecisions', 'newSkillLineDecisions', 'implementationStatuses']) {
    assert(Array.isArray(report.enums?.[key]) && report.enums[key].length > 0, `oracle enums.${key} missing`);
  }
  const statuses = new Set();
  for (const skill of report.skills) {
    for (const status of proofStatusesOf(skill)) {
      assert(PROOF_STATUSES.includes(status), `${skill.stableId}: ${status}`);
      statuses.add(status);
    }
    for (const category of portabilityOf(skill)) assert(PORTABILITY_CATEGORIES.includes(category), `${skill.stableId}: ${category}`);
    assert.equal(skill.components.length, BEHAVIOR_GROUPS.length, `${skill.stableId}: must expose all 12 behavior groups`);
    assert.deepEqual(skill.components.map((component) => component.id), BEHAVIOR_GROUPS.map((group) => group.id));
    for (const component of skill.components) {
      const categories = portabilityOf(component);
      assert(categories.length > 0, `${skill.stableId}/${component.id}: component portability missing`);
      for (const category of categories) assert(PORTABILITY_CATEGORIES.includes(category), `${skill.stableId}/${component.id}: ${category}`);
      if (component.changed === false && (component.fields ?? []).length === 0) {
        assert(categories.includes('NOT_APPLICABLE'), `${skill.stableId}/${component.id}: empty component must be NOT_APPLICABLE`);
      }
    }
    for (const { component, field } of allFields(skill)) {
      const values = field.values ?? field.displayValues;
      assert(values && typeof values === 'object', `${skill.stableId}/${field.id}: values missing`);
      for (const source of ['vanilla32', 'bkvince', 'pd2']) assert(Object.hasOwn(values, source), `${skill.stableId}/${field.id}: missing ${source}`);
      if (/(?:srv|clt).*(?:func|function)|hitfunc/i.test(`${field.header} ${field.id}`) && field.changed) {
        assert.equal(component.id, 'engine_functions', `${skill.stableId}/${field.id}: native callback is assigned to ${component.id}`);
        assert(field.protected, `${skill.stableId}/${field.id}: divergent native field is not protected`);
        assert(['NATIVE_UNPROVEN', 'EXACT_TABLE'].includes(field.proofStatus));
        const portability = unique([...portabilityOf(field), ...portabilityOf(component), ...portabilityOf(skill)]);
        assert(portability.some((category) => ['NATIVE_UNPROVEN', 'NATIVE_FUNCTION_MISMATCH'].includes(category)), `${skill.stableId}/${field.id}: native divergence lacks portability gate`);
      }
    }
  }
  for (const required of ['EXACT_TABLE', 'EXACT_FORMULA', 'SYMBOLIC', 'MALFORMED_SOURCE', 'NATIVE_UNPROVEN']) {
    assert(statuses.has(required), `oracle never emits ${required}`);
  }
  for (const skill of report.skills) for (const document of skill.documentation ?? []) {
    assert(['DOCUMENTED', 'TABLE_ONLY', 'UNMAPPED'].includes(document.status), `${skill.stableId}: invalid documentation status`);
    assert.equal(document.revision, 23785);
    assert(document.section && document.summary);
    assert.equal(document.portabilityEvidence, false, 'wiki must never become portability proof');
    if (document.status === 'DOCUMENTED') {
      assert(document.exactMapping && typeof document.exactMapping === 'object', `${skill.stableId}: DOCUMENTED requires an explicit claim/table mapping`);
    }
  }
});

test('hybrid decisions work by behavior bundle without authorizing implementation', () => {
  const skill = findSkill('Amplify Damage');
  const entry = createEntry(skill);
  entry.globalDecision = 'ADAPT_PD2_SELECTIVELY';
  entry.notes.finalJustification = 'Conserver la puissance BKVince et sélectionner seulement les paramètres PD2 prouvés.';
  entry.notes.testPlan = 'Comparer rayon, durée, mana et résistance aux niveaux gouvernés.';
  for (const bundle of skill.decisionBundles.filter((item) => item.scope === 'PLAYER')) {
    entry.bundleDecisions[bundle.id] = { decision: 'KEEP_BKVINCE' };
  }
  const adoptable = skill.decisionBundles.find((bundle) => bundle.scope === 'PLAYER' && !bundle.protected);
  assert(adoptable, 'Amplify Damage needs one non-protected behavior bundle for a hybrid decision');
  entry.bundleDecisions[adoptable.id] = { decision: 'ADOPT_PD2' };
  const state = entryState(report, skill, entry, approvedSchemaPolicy());
  assert(state.complete, state.reasons.join('\n'));
  assert.equal(entry.implementationStatus, 'NOT_REVIEWED');

  const bulk = applyBulk(report, [skill.stableId], { [skill.stableId]: entry }, 'DISCUSS', { replace: false });
  assert.equal(bulk[skill.stableId].bundleDecisions[adoptable.id].decision, 'ADOPT_PD2', 'bulk fill must preserve an existing bundle decision');
  assert.equal(bulk[skill.stableId].implementationStatus, 'NOT_REVIEWED');
  assert.throws(() => applyBulk(report, [skill.stableId], bulk, 'KEEP_BKVINCE', { replace: true }), /confirmed:true/);
});

test('completion requires governed notes without coupling design to implementation authorization', () => {
  const skill = findSkill('Amplify Damage');
  const entry = createEntry(skill);
  entry.globalDecision = 'KEEP_BKVINCE';
  for (const bundle of skill.decisionBundles.filter((item) => item.scope === 'PLAYER')) {
    entry.bundleDecisions[bundle.id] = { decision: 'KEEP_BKVINCE' };
  }
  const schemaPolicy = approvedSchemaPolicy();
  let state = entryState(report, skill, entry, schemaPolicy);
  assert.equal(state.complete, false);
  assert(state.reasons.some((reason) => /finalJustification/i.test(reason)));
  entry.notes.finalJustification = 'La baseline BKVince reste le modèle retenu.';
  state = entryState(report, skill, entry, schemaPolicy);
  assert.equal(state.complete, true, state.reasons.join('\n'));
  assert.equal(entry.implementationStatus, 'NOT_REVIEWED');

  entry.globalDecision = 'ADAPT_PD2_SELECTIVELY';
  state = entryState(report, skill, entry, schemaPolicy);
  assert.equal(state.complete, false);
  assert(state.reasons.some((reason) => /testPlan/i.test(reason)));
  entry.notes.testPlan = 'Tester la composante sélectionnée contre les six niveaux gouvernés.';
  assert.equal(entryState(report, skill, entry, schemaPolicy).complete, true);

  const discuss = createEntry(skill);
  discuss.globalDecision = 'DISCUSS';
  assert.equal(entryState(report, skill, discuss, schemaPolicy).complete, false);
  discuss.notes.general = 'Question ouverte sur la fonction native.';
  const discussState = entryState(report, skill, discuss, schemaPolicy);
  assert.equal(discussState.complete, false, 'DISCUSS remains deliberately unresolved even with its mandatory explanatory note');
  assert(!discussState.reasons.some((reason) => /note|general|finalJustification/i.test(reason)), 'the required DISCUSS note is now satisfied');
  assert.equal(discuss.implementationStatus, 'NOT_REVIEWED');
});

test('decision persistence is comparison-bound and rejects stale hashes and fingerprints', () => {
  const envelope = createEmptyEnvelope(report);
  assert.equal(storageKey(report), `pd2-skills-review-decisions-v3:${report.comparisonHash}`);
  assert(Object.values(envelope.entries).every((entry) => entry.implementationStatus === 'NOT_REVIEWED'));
  const staleHash = structuredClone(envelope);
  staleHash.comparisonHash = '0'.repeat(64);
  assert.equal(validateImport(report, staleHash).valid, false);
  assert(validateImport(report, staleHash).errors.some((error) => /stale comparison hash/i.test(error)));

  const [stableId] = Object.keys(envelope.entries);
  const staleFingerprint = structuredClone(envelope);
  staleFingerprint.entries[stableId].fingerprint = 'F'.repeat(64);
  assert.equal(validateImport(report, staleFingerprint).valid, false);
  assert(validateImport(report, staleFingerprint).errors.some((error) => /stale fingerprint/i.test(error)));
});

test('generator, comparison hash, fingerprints and standalone HTML are deterministic', () => {
  const secondArtifacts = generateSkillReviewArtifacts();
  const secondReport = secondArtifacts.report;
  assert.equal(JSON.stringify(secondReport), JSON.stringify(report));
  assert.deepEqual(secondArtifacts.hashes, artifacts.hashes);
  assert.equal(secondArtifacts.raw.html, artifacts.raw.html);
  assert.match(report.comparisonHash, /^[A-F0-9]{64}$/);
  assert.match(report.policyHashes?.wikiPin ?? '', /^[A-F0-9]{64}$/, 'pinned wiki revision/policy must participate in comparison identity');
  assert.equal(sha256Canonical(report.skills.map(({ stableId, fingerprint }) => ({ stableId, fingerprint }))), sha256Canonical(secondReport.skills.map(({ stableId, fingerprint }) => ({ stableId, fingerprint }))));

  const html = artifacts.raw.html;
  assert.match(html, /^<!doctype html>/i);
  assert.match(html, /file:\/\//);
  assert.doesNotMatch(html, /<script[^>]+src=|<link[^>]+rel=["']?stylesheet|\bfetch\s*\(|XMLHttpRequest|type=["']module["']/i);
  assert.doesNotMatch(html, /cdn\./i);
  const embedded = html.match(/<script>([\s\S]*)<\/script>/)?.[1];
  assert(embedded, 'standalone HTML must embed one classic script');
  assert.doesNotThrow(() => new vm.Script(embedded, { filename: 'pd2-skills-review.html' }));
  for (const text of [
    'Recherche globale', 'Décisions incomplètes seulement', 'Skill suivant à décider',
    'Vue joueur', 'Vue technique', 'Copier le briefing du skill', 'Exporter ce skill en Markdown',
    'Exporter le dossier de révision de cette classe', 'Baseline courante (HEAD)',
    'Audit historique du 8 août',
    '<svg viewBox=',
  ]) assert(html.includes(text), `HTML omits ${text}`);
  const fireBall = report.skills.find((skill) => skill.canonicalName === 'Fire Ball');
  assert(fireBall, 'Fire Ball witness must exist');
  assert.equal(fireBall.evidence?.overall, 'MALFORMED_SOURCE');
  assert(fireBall.evidence?.statuses?.includes('MALFORMED_SOURCE'));
  const uiSource = fs.readFileSync(path.join(repoRoot, 'scripts', 'migrate-bkvince', 'pd2-skills-review-ui.mjs'), 'utf8');
  assert.match(uiSource, /asArray\(skill\.evidence\?\.overall\)/);
  assert.match(uiSource, /asArray\(skill\.evidence\?\.statuses\)/);
});

test('preview CLI is strictly read-only, rejects --apply, and cannot write to gameplay roots', async () => {
  const source = fs.readFileSync(previewPath, 'utf8');
  assert.match(source, /PREVIEW_ONLY|preview-only|read-only/i);
  assert.match(source, /--apply/);
  assert.doesNotMatch(source, /function\s+(?:apply|deploy|install)\w*\s*\(/i);

  const result = spawnSync(process.execPath, [previewPath, '--apply'], {
    cwd: repoRoot,
    encoding: 'utf8',
    timeout: 30_000,
  });
  assert.notEqual(result.status, 0);
  assert.match(`${result.stdout}\n${result.stderr}`, /forbidden|interdit|refus/i);

  const preview = await import('./pd2-skills-decisions-preview.mjs');
  const compile = preview.compileDecisionPreview ?? preview.buildDecisionPreview ?? preview.compilePreview;
  assert.equal(typeof compile, 'function', 'preview compiler must export its pure compiler');
  assert.equal(typeof preview.assertSafeOutputPath, 'function');
  for (const root of NON_MUTATION_RULES.forbiddenWriteRoots) {
    assert.throws(() => preview.assertSafeOutputPath(path.resolve(repoRoot, root, 'forbidden-preview.json')), /must stay under/i, root);
  }
  const empty = createEmptyEnvelope(report);
  const projection = compile(report, empty);
  assert.equal(projection.ready ?? projection.applicable, false);
  assert.equal(projection.proposedManifest, null);
  const diagnostics = [
    ...Object.values(projection.diagnostics ?? {}).flat(),
    ...(projection.incomplete ?? []),
    ...(projection.conflicts ?? []),
  ];
  assert(diagnostics.length > 0, 'incomplete preview must emit atomic diagnostics');
  assert.equal(projection.implementationAuthorized, false);
  for (const collection of ['exactChangesByTable', 'keptCells', 'adoptedCells', 'customCells', 'appendOnlyRows']) {
    const value = projection[collection];
    assert.equal(Array.isArray(value) ? value.length : Object.keys(value ?? {}).length, 0, `${collection} must stay atomic/empty while blocked`);
  }

  const candidate = report.skills.find((skill) => (
    skill.newPd2PlayerSkill
    && skill.newSkillPlan?.proposedTargetOrdinal === report.coverage.nextAppendOrdinal
  ));
  assert(candidate, 'an append candidate at the first computed ordinal is required');
  const entry = createEntry(candidate);
  entry.globalDecision = 'IMPORT_NEW_PD2_SKILL';
  entry.newSkillLineDecision = 'IMPORT_APPEND_ONLY';
  entry.notes.finalJustification = 'Verification-only complete import selection.';
  entry.notes.testPlan = 'Verify append projection and every unresolved transitive gate.';
  for (const bundle of candidate.decisionBundles.filter((item) => item.scope === 'PLAYER')) {
    entry.bundleDecisions[bundle.id] = { decision: 'KEEP_BKVINCE' };
  }
  const importEnvelope = createEmptyEnvelope(report);
  importEnvelope.schemaPolicy = approvedSchemaPolicy();
  importEnvelope.exportScope = 'COMPLETE_ONLY';
  importEnvelope.entries = { [candidate.stableId]: entry };
  assert.equal(entryState(report, candidate, entry, importEnvelope.schemaPolicy).complete, true, 'verification import fixture must be decision-complete');
  const gatedImport = compile(report, importEnvelope);
  assert.equal(gatedImport.ready, false);
  assert.equal(gatedImport.proposedManifest, null);
  const conflictCodes = new Set((gatedImport.conflicts ?? []).map((conflict) => conflict.code));
  for (const code of ['DEPENDENCY_CLOSURE_INCOMPLETE', 'LOCALIZATION_CLOSURE_INCOMPLETE', 'CONSUMER_CLOSURE_INCOMPLETE']) {
    assert(conflictCodes.has(code), `${candidate.stableId}: preview does not enforce ${code}`);
  }
  assert(!(gatedImport.conflicts ?? []).some((conflict) => (
    conflict.code === 'APPEND_FIELD_LOCATOR_MISSING' && /^skills\.txt:\*?id$/i.test(conflict.fieldId)
  )), 'documentary *Id mapping must resolve by its governed append-preview mode without becoming ordinal allocation');
  for (const collection of ['exactChangesByTable', 'keptCells', 'adoptedCells', 'customCells', 'appendOnlyRows']) {
    const value = gatedImport[collection];
    assert.equal(Array.isArray(value) ? value.length : Object.keys(value ?? {}).length, 0, `${collection} must remain empty while transitive closure is blocked`);
  }
});

test('generation and preview verification do not change any gameplay source byte', () => {
  for (const [source, expected] of Object.entries(EXPECTED_SKILLS_SOURCES)) {
    assert.deepEqual(fs.readFileSync(expected.path), baselineBytes[source], `${source} gameplay source changed during verification`);
  }
});
