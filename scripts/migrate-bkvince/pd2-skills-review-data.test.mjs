import assert from 'node:assert/strict';
import fs from 'node:fs';
import test from 'node:test';

import {
  DEFAULT_SOURCE_ROOTS,
  SOURCE_TABLE_NAMES,
  generateOracleData,
  loadWorkbenchSources,
  stableHash,
} from './pd2-skills-review-data.mjs';
import {
  BEHAVIOR_GROUPS,
  FROZEN_CONTRACT_HASH,
  SOURCE_ORDER,
} from './pd2-skills-review-contracts.mjs';

const sources = loadWorkbenchSources();
const oracle = generateOracleData();

function skill(name) {
  const matches = oracle.skills.filter((candidate) => candidate.canonicalName === name);
  assert.equal(matches.length, 1, `expected exactly one canonical skill ${name}`);
  return matches[0];
}

function field(candidate, header) {
  return candidate.components.flatMap((component) => component.fields)
    .find((item) => item.header.toLowerCase() === header.toLowerCase());
}

function hashLeaves(value, trail = 'sourceHashes') {
  if (typeof value === 'string') {
    assert.match(value, /^[A-F0-9]{64}$/, `${trail} must be a SHA-256 leaf`);
    return;
  }
  assert(value && typeof value === 'object' && !Array.isArray(value), `${trail} must be a nested object`);
  for (const [key, child] of Object.entries(value)) hashLeaves(child, `${trail}.${key}`);
}

test('all governed TSV sources round-trip byte-exact and hashes are stable leaves', () => {
  for (const source of SOURCE_ORDER) {
    for (const tableName of SOURCE_TABLE_NAMES) {
      const document = sources.documents[source][tableName];
      assert.equal(document.table.rows.length, oracle.sourceManifest[source].tables[tableName].rows);
      assert.equal(document.sha256, oracle.sourceHashes[source][tableName]);
      assert.equal(oracle.sourceManifest[source].tables[tableName].roundTripByteExact, true);
    }
  }
  hashLeaves(oracle.sourceHashes);
  assert.equal(oracle.sourceHashes.vanilla32['skills.txt'], 'EFAF7AC4BA0493109C698EF32ACF4A2B3A577E13500D0B50258C80B600986F51');
  assert.equal(oracle.sourceHashes.bkvince['skills.txt'], '08497CC0BD8B2B5CBD895F7477AD0CBF272571FB67B78061EDFABC31C48B8B77');
  assert.equal(oracle.sourceHashes.pd2['skills.txt'], 'AEEFC3F2C0C80811D62FC1A17C3B031DE2164E5606BF9779F34024B35BC87B8B');
  assert.equal(oracle.sourceHashes.formulaReference, '4028D9985368B89FE5D84A93F60177ECC34957599A170A4F8496DDD169636A4F');
  assert.equal(oracle.sourceHashes.localization.bkvince, '73E6C164F484B31372A5D91B4D0FFE65888C828CC7703E4D811378679FC48268');
  assert.equal(oracle.sourceHashes.localization.pd2, 'EC023659BFA1BA0E3FFADFBEECAD344D750E21E18DBDA1AA287D6D2835DC1107');
  assert.equal(oracle.sourceManifest.bkvince.tables['pettype.txt'].inheritance.mode, 'INHERITED_VANILLA32');
  assert.equal(oracle.sourceHashes.bkvince['pettype.txt'], oracle.sourceHashes.vanilla32['pettype.txt']);
});

test('the complete physical populations and current baseline are represented exactly once', () => {
  assert.deepEqual(oracle.coverage.physicalRows, { vanilla32: 429, bkvince: 451, pd2: 603 });
  assert.equal(oracle.nodes.length, 429 + 451 + 603);
  assert.equal(new Set(oracle.nodes.map((node) => node.id)).size, oracle.nodes.length);
  assert.equal(oracle.coverage.allRowsRepresentedOnce, true);
  assert.equal(oracle.coverage.mappingAmbiguities, 0);
  assert.equal(oracle.coverage.nextAppendOrdinal, 451);
  assert.equal(oracle.collisions.length, 110);
  assert.equal(oracle.coverage.historicalBaseline.bkvinceRows, 449);
  assert.equal(oracle.coverage.historicalBaseline.collisions, 108);
  assert.equal(oracle.coverage.baselineChangeExplanation.collisionDelta, 2);
  assert.deepEqual(oracle.coverage.baselineChangeExplanation.addedTechnicalRows.map((item) => item.ordinal), [449, 450]);
});

test('runtime ordinals, documentary ids and collision edges remain independent', () => {
  const concentration = oracle.nodes.find((node) => node.source === 'bkvince' && node.name === 'Concentration');
  assert.equal(concentration.ordinal, 108);
  assert.equal(concentration.declaredId, '113');
  const collision = oracle.collisions.find((item) => item.ordinal === 40);
  assert.deepEqual(collision.names, { pd2: 'Cold Enchant', bkvince: 'Frozen Armor' });
  const coldEnchant = skill('Cold Enchant');
  assert.deepEqual(coldEnchant.ordinals, { vanilla32: 408, bkvince: 408, pd2: 40 });
  assert.ok(coldEnchant.mappingTypes.includes('SAME_SKILL_MOVED_ORDINAL'));
  assert.ok(coldEnchant.mappingTypes.includes('SAME_ORDINAL_DIFFERENT_SKILL'));
  assert.equal(coldEnchant.semanticIdentityEvidence.status, 'PROVEN');
  assert.ok(coldEnchant.collisionIds.includes('collision:pd2-bkvince:40'));
});

test('semantic aliases, moved skills and slot replacements are governed, never name-only', () => {
  const amplify = skill('Amplify Damage');
  assert.equal(amplify.names.pd2, 'AmpDmg');
  assert.ok(amplify.mappingTypes.includes('RENAMED_ALIAS'));
  assert.ok(!amplify.mappingTypes.includes('SAME_ORDINAL_DIFFERENT_SKILL'));
  assert.equal(amplify.semanticIdentityEvidence.status, 'PROVEN');
  const amplifyCollision = oracle.collisions.find((item) => item.ordinal === 66);
  assert.equal(amplifyCollision.resolution, 'RESOLVED_GOVERNED_SEMANTIC_IDENTITY');
  assert.ok(amplify.components.flatMap((component) => component.fields)
    .some((item) => item.changed && !item.protected), 'governed alias must permit a selective hybrid decision');
  const lower = skill('Lower Resist');
  assert.equal(lower.names.pd2, 'LowRes');
  assert.ok(lower.mappingTypes.includes('RENAMED_ALIAS'));
  const lightningSentry = skill('Lightning Sentry');
  assert.deepEqual(lightningSentry.ordinals, { vanilla32: 271, bkvince: 271, pd2: 366 });
  assert.ok(lightningSentry.mappingTypes.includes('SAME_SKILL_MOVED_ORDINAL'));
  for (const candidate of oracle.skills.filter((item) => item.mappingTypes.some((type) => type.startsWith('SAME_SKILL')))) {
    assert.equal(candidate.semanticIdentityEvidence.status, 'PROVEN', candidate.stableId);
    assert.ok(candidate.semanticIdentityEvidence.signals.length >= 2, candidate.stableId);
  }
  for (const name of ['Slow Movement', 'Javelin and Spear Mastery', 'Desecrate', 'Raise Skeleton Archer', 'Holy Sword', 'Sword Mastery', 'One Hand Mastery', 'Two Hand Mastery', 'Combat Reflexes']) {
    assert.ok(skill(name).mappingTypes.includes('SLOT_REPLACEMENT'), name);
    assert.equal(skill(name).newPd2PlayerSkill, false, name);
  }
  assert.ok(skill('Increased Stamina').mappingTypes.includes('SLOT_REPLACEMENT'));
  assert.ok(oracle.skills.every((candidate) => candidate.mappingTypes.length > 0), 'every semantic entity needs a mapping category');
});

test('player classification excludes technical charclass rows and allocates all true new candidates append-only', () => {
  const candidates = oracle.skills.filter((candidate) => candidate.newPd2PlayerSkill);
  assert.equal(candidates.length, 15);
  assert.ok(candidates.some((candidate) => candidate.canonicalName === 'Chain Lightning Sentry'));
  assert.ok(candidates.some((candidate) => candidate.canonicalName === 'Shattering Arrow'
    && candidate.classification.category === 'PLAYER_CANDIDATE_AUDIT_OVERRIDE'));
  assert.ok(candidates.some((candidate) => candidate.canonicalName === 'Blade Dance'
    && candidate.classification.category === 'PLAYER_CANDIDATE_AUDIT_OVERRIDE'));
  assert.ok(!candidates.some((candidate) => /temp|proc|merc|boss|map/i.test(candidate.canonicalName)));
  assert.equal(skill('Dopplezon').newPd2PlayerSkill, false);
  const allocations = candidates.map((candidate) => candidate.newSkillPlan.proposedTargetOrdinal).sort((a, b) => a - b);
  assert.deepEqual(allocations, Array.from({ length: 15 }, (_, index) => 451 + index));
  for (const candidate of candidates) {
    const plan = candidate.newSkillPlan;
    assert.equal(plan.appendOnly, true);
    assert.equal(plan.proposedRow.targetOrdinal, plan.proposedTargetOrdinal);
    assert.equal(Object.keys(plan.proposedRow.values).length, 322);
    assert.equal(plan.proposedRow.sourceNodeId, candidate.nodeIds.pd2);
    assert.equal(plan.consumerClosure.complete, false);
    assert.equal(plan.localizationClosure.complete, false);
    assert.equal(plan.dependencyClosure.completeForBkvince, false);
    assert.ok(candidate.portability.categories.includes('APPEND_ONLY_REQUIRED'));
    assert.ok(candidate.portability.categories.includes('BLOCKED_DEPENDENCY'));
  }
  assert.deepEqual(skill('Combustion').newSkillPlan.currentOccupantAtPd2Ordinal, {
    source: 'bkvince',
    ordinal: 376,
    nodeId: 'bkvince:skills.txt:376',
    stableId: 'skill:war:summon-tainted',
    name: 'Summon Tainted',
  });
});

test('navigation uses real skilldesc page/row/column ordering and readable tree labels', () => {
  const sorceress = oracle.navigation.find((entry) => entry.id === 'sor');
  assert.deepEqual(sorceress.trees.map((tree) => tree.label), [
    'Sorceress — Fire', 'Sorceress — Lightning', 'Sorceress — Cold',
  ]);
  for (const tree of sorceress.trees) {
    const coordinates = tree.coordinates.map(({ page, row, column }) => [page, row, column]);
    const sorted = [...coordinates].sort((left, right) => (
      left[0] - right[0] || left[1] - right[1] || left[2] - right[2]
    ));
    assert.deepEqual(coordinates, sorted, tree.label);
  }
  const coldEnchant = skill('Cold Enchant');
  assert.deepEqual(
    [coldEnchant.tree.coordinatesBySource.pd2.page, coldEnchant.tree.coordinatesBySource.pd2.row, coldEnchant.tree.coordinatesBySource.pd2.column],
    [3, 1, 3],
  );
  assert.deepEqual(
    [coldEnchant.tree.coordinatesBySource.bkvince.page, coldEnchant.tree.coordinatesBySource.bkvince.row, coldEnchant.tree.coordinatesBySource.bkvince.column],
    [1, 4, 3],
  );
});

test('all skills have twelve behavior components and compact exact three-way fields', () => {
  for (const candidate of oracle.skills) {
    assert.deepEqual(candidate.components.map((component) => component.id), BEHAVIOR_GROUPS.map((component) => component.id));
    for (const component of candidate.components) {
      if (!component.fields.length) assert.deepEqual(component.portability, ['NOT_APPLICABLE']);
      for (const item of component.fields) {
        assert.deepEqual(Object.keys(item.values), SOURCE_ORDER);
        assert.equal(typeof item.changed, 'boolean');
        assert.ok(item.proofStatus);
        assert.ok(Array.isArray(item.dependencyIds));
        assert.equal('displayValues' in item, false, 'raw values must not be duplicated');
      }
    }
  }
  assert.equal(field(skill('Fire Wall'), 'delay').values.pd2, '38');
  assert.equal(field(skill('Fire Wall'), 'localdelay').values.bkvince, '15');
  assert.ok(field(skill('Fire Wall'), 'localdelay').protectionReasons.includes('delay_translation'));
  for (const candidate of oracle.skills) {
    for (const item of candidate.components.flatMap((component) => component.fields)) {
      if (item.changed && /^(?:srv|clt)missile[a-d]?$/i.test(item.header)) {
        assert.ok(item.protectionReasons.includes('client_server_missile_behavior'), `${candidate.stableId}:${item.header}`);
      }
    }
  }
});

test('formula evidence preserves malformed and symbolic witnesses without repair', () => {
  const fireBall = skill('Fire Ball').evidence.findings.find((finding) => (
    finding.source === 'bkvince' && finding.table === 'skilldesc.txt' && finding.header.toLowerCase() === 'desccalca3'
  ));
  assert.equal(fireBall.status, 'MALFORMED_SOURCE');
  assert.equal(fireBall.raw, "min(3,1+skill('Fire Ball'.blvl)/10");
  const fireWall = skill('Fire Wall').evidence.findings.find((finding) => (
    finding.source === 'vanilla32' && finding.header.toLowerCase() === 'edmgsympercalc'
  ));
  assert.equal(fireWall.status, 'MALFORMED_SOURCE');
  assert.equal(fireWall.raw, "(skill('Warmth'.blvl)*par8+skill('Inferno'.blvl)*par7");
  const raven = skill('Raven');
  const ravenFindings = raven.evidence.findings.filter((finding) => finding.header.toLowerCase() === 'calc2');
  assert.equal(ravenFindings.length, 3);
  assert.ok(ravenFindings.every((finding) => finding.status === 'UNSUPPORTED_IDENTIFIER' && finding.value === null));
  assert.ok(raven.curves.standard.symbolic.some((finding) => finding.header?.toLowerCase() === 'calc2'));
});

test('curves expose six levels and never fabricate symbolic synergy scenarios', () => {
  for (const candidate of [skill('Amplify Damage'), skill('Fire Ball'), skill('Frozen Orb'), skill('Hydra'), skill('Raven')]) {
    assert.deepEqual(candidate.curves.levels, [1, 5, 10, 20, 30, 40]);
    assert.deepEqual(candidate.curves.scenarios.map((scenario) => scenario.id), ['standard', 'synergies20', 'custom']);
    for (const series of candidate.curves.standard.series) {
      for (const source of SOURCE_ORDER) assert.equal(series.values[source].length, 6);
    }
    assert.deepEqual(candidate.curves.synergies20.series, []);
    assert.deepEqual(candidate.curves.custom.series, []);
  }
  assert.equal(skill('Fire Ball').curves.standard.series.find((series) => series.id === 'mana').values.bkvince[0], 5);
  const fireWallSeries = Object.fromEntries(skill('Fire Wall').curves.standard.series.map((series) => [series.id, series]));
  assert.equal('cooldown' in fireWallSeries, false, 'PD2 delay must never be aliased to a D2R cooldown');
  assert.equal(fireWallSeries.local_delay.values.bkvince[0], 15);
  assert.equal(fireWallSeries.local_delay.values.pd2[0], null);
  assert.equal(fireWallSeries.pd2_delay.values.pd2[0], 38);
  assert.equal(fireWallSeries.pd2_delay.values.bkvince[0], null);
  const guidedArrowProjectiles = skill('Guided Arrow').curves.standard.series
    .find((series) => series.id === 'projectiles_targets');
  assert.ok(Object.values(guidedArrowProjectiles.values).flat().every((value) => value === null),
    'a generic calc1 damage formula must not be presented as a projectile count');
  const chargedStrikeProjectiles = skill('Charged Strike').curves.standard.series
    .find((series) => series.id === 'projectiles_targets');
  assert.equal(chargedStrikeProjectiles.values.vanilla32[0], 3,
    'explicit # of bolts metadata permits governed count evaluation');
  const poison = skill('Poison Javelin').curves.standard;
  assert.equal(poison.series.find((series) => series.id === 'poison_encoded_min').values.vanilla32[0], 32);
  assert.equal(poison.series.find((series) => series.id === 'poison_duration_frames').values.pd2[0], 25);
  for (const id of ['poison_dps_min', 'poison_dps_max', 'poison_total_min', 'poison_total_max']) {
    const series = poison.series.find((candidate) => candidate.id === id);
    assert.equal(series.proofStatus, 'SYMBOLIC');
    assert.ok(Object.values(series.values).flat().every((value) => value === null), id);
  }
  assert.ok(poison.symbolic.some((finding) => finding.metric === 'poisonDamagePerSecond'
    && /does not prove/.test(finding.reason)));
});

test('linked dependency facts cover projectile and summon witnesses with explicit closure', () => {
  const frozenOrb = skill('Frozen Orb');
  const frozenMissileFacts = frozenOrb.dependencies.filter((dependency) => dependency.table === 'missiles.txt')
    .flatMap((dependency) => dependency.facts);
  for (const header of ['vel', 'maxvel', 'range', 'size', 'explosionmissile']) {
    assert.ok(frozenMissileFacts.some((fact) => fact.header === header), `Frozen Orb linked missile ${header}`);
  }
  assert.deepEqual(frozenMissileFacts.find((fact) => fact.header === 'vel').values,
    { vanilla32: '10', bkvince: '16', pd2: '10' });
  assert.deepEqual(frozenMissileFacts.find((fact) => fact.header === 'range').values,
    { vanilla32: '30', bkvince: '30', pd2: '45' });
  const hydra = skill('Hydra');
  assert.ok(hydra.dependencies.some((dependency) => dependency.table === 'pettype.txt' && dependency.key === 'hydra'));
  assert.ok(hydra.dependencies.some((dependency) => dependency.table === 'monstats.txt' && dependency.key === 'hydra1'));
  assert.ok(hydra.dependencies.some((dependency) => dependency.table === 'skills.txt' && dependency.key === 'Fire Ball'));
  assert.ok(field(hydra, 'sumskill3').dependencyIds.length > 0);
  assert.equal(hydra.dependencyClosure.transitiveStatus, 'UNPROVEN');
  assert.equal(hydra.dependencyClosure.complete, false);
  assert.ok(hydra.dependencyClosure.blockingGates.includes('TRANSITIVE_DEPENDENCY_CLOSURE_UNPROVEN'));
  const iceBarrage = skill('Ice Barrage');
  assert.ok(iceBarrage.dependencies.some((dependency) => dependency.table === 'skilldesc.txt' && dependency.key === 'ice barrage'));
  assert.ok(iceBarrage.dependencies.some((dependency) => dependency.table === 'missiles.txt'));
});

test('every numbered native callback divergence is unproven and protected', () => {
  const bladesOfIce = field(skill('Blades of Ice'), 'cltprgfunc2');
  assert.deepEqual(bladesOfIce.values, { vanilla32: '9', bkvince: '9', pd2: '81' });
  assert.equal(bladesOfIce.proofStatus, 'NATIVE_UNPROVEN');
  assert.equal(bladesOfIce.protected, true);
  assert.ok(bladesOfIce.protectionReasons.includes('native_functions'));
  assert.equal(skill('Blades of Ice').components.find((component) => (
    component.fields.some((item) => item.id === bladesOfIce.id)
  )).id, 'engine_functions');
  for (const candidate of oracle.skills) {
    for (const item of candidate.components.flatMap((component) => component.fields)) {
      if (item.changed && /^(?:srv|clt).*(?:func|function)\d*$|hitfunc\d*$/i.test(item.header)) {
        assert.equal(item.proofStatus, 'NATIVE_UNPROVEN', `${candidate.stableId}:${item.header}`);
        assert.ok(item.protectionReasons.includes('native_functions'), `${candidate.stableId}:${item.header}`);
      }
    }
  }
});

test('identical rows are read-only while all actionable player skills remain unapproved', () => {
  for (const name of ['Attack', 'Throw', 'Left Hand Throw']) {
    const candidate = skill(name);
    assert.equal(candidate.identical, true);
    assert.equal(candidate.readOnly, true);
    assert.equal(candidate.status, 'IDENTICAL_AUTO_RESOLVED_READ_ONLY');
  }
  for (const candidate of oracle.skills.filter((item) => item.playerSkill && !item.identical)) {
    assert.notEqual(candidate.status, 'IMPLEMENTATION_AUTHORIZED');
    assert.doesNotMatch(candidate.summary, /^\d+ différences techniques/);
  }
  const warlock = skill('Summon Tainted');
  assert.equal(warlock.bkvinceOnlyPlayerSkill, true);
  assert.equal(warlock.classification.category, 'BKVINCE_WARLOCK_TREE_SKILL');
  assert.ok(warlock.components.flatMap((component) => component.fields)
    .some((item) => item.protectionReasons.includes('warlock')));
});

test('wiki evidence remains table-only and policy changes affect comparison identity', () => {
  assert.equal(oracle.coverage.documentationCounts.DOCUMENTED, 0);
  assert.equal(oracle.documentation.source.revision, 23785);
  assert.ok(oracle.skills.flatMap((candidate) => candidate.documentation)
    .every((item) => item.status === 'TABLE_ONLY' && item.portabilityEvidence === false));
  assert.match(oracle.policyHashes.wikiPin, /^[A-F0-9]{64}$/);
  assert.equal(oracle.frozenContractHash, FROZEN_CONTRACT_HASH);
});

test('generation is deterministic, machine-independent and read-only', () => {
  const before = Object.fromEntries([
    DEFAULT_SOURCE_ROOTS.vanilla32,
    DEFAULT_SOURCE_ROOTS.bkvince,
    DEFAULT_SOURCE_ROOTS.pd2,
  ].map((root) => [root, fs.statSync(root).mtimeMs]));
  const again = generateOracleData();
  assert.equal(again.comparisonHash, oracle.comparisonHash);
  assert.equal(stableHash(again), stableHash(oracle));
  assert.ok(!JSON.stringify(oracle.sourceManifest).includes('C:\\Workspaces\\'));
  assert.deepEqual(Object.fromEntries(Object.keys(before).map((root) => [root, fs.statSync(root).mtimeMs])), before);
  assert.ok(Buffer.byteLength(JSON.stringify(oracle)) < 60 * 1024 * 1024, 'oracle must remain practical for one file:// HTML');
});
