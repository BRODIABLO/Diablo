import assert from 'node:assert/strict';
import { existsSync, readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import test from 'node:test';

import { itemCatalog, skillCatalog } from '../data/bkvince-constants.generated.js';
import {
  equipmentPanelVisual,
  equipmentSlotPlaceholderVisuals,
  itemVisuals,
} from '../data/item-visuals.generated.js';
import {
  classPortraitVisuals,
  skillTreeVisuals,
  skillVisuals,
} from '../data/skill-visuals.generated.js';
import { availableItemGroups, questActs } from './character-codec.js';

function publicAssetPath(url) {
  return fileURLToPath(new URL(`../../public${url}`, import.meta.url));
}

test('generates the governed BKVince paper-doll and available item sprites', () => {
  assert.equal(equipmentPanelVisual, '/ui/equipment-panel.png');
  assert.ok(existsSync(publicAssetPath(equipmentPanelVisual)));
  assert.ok(Object.keys(itemVisuals).length >= 814);
  assert.ok(new Set(Object.values(itemVisuals)).size >= 388);
  assert.deepEqual(
    itemCatalog.bases.filter(({ code }) => !itemVisuals[code]).map(({ code }) => code),
    [],
  );

  for (const code of ['mff', 'mfc', 'mfd', 'r33', 'hax', 'cap', 'hgl', 'mfg', 'rds']) {
    assert.ok(itemVisuals[code], `Missing generated visual for ${code}.`);
    assert.ok(existsSync(publicAssetPath(itemVisuals[code])), `Missing PNG for ${code}.`);
  }

  assert.deepEqual(Object.keys(equipmentSlotPlaceholderVisuals), [
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '10', '11', '12',
  ]);
  for (const [slot, visual] of Object.entries(equipmentSlotPlaceholderVisuals)) {
    assert.ok(existsSync(publicAssetPath(visual)), `Missing equipment placeholder PNG for slot ${slot}.`);
  }
});

test('generates every governed native picture variant as a real PNG', () => {
  const variantBases = itemCatalog.bases.filter(({ pictures }) => pictures.length > 0);
  assert.deepEqual(variantBases.map(({ code }) => code), ['amu', 'cjw', 'cm1', 'cm2', 'cm3', 'cs2', 'jew', 'rin', 'vip']);
  assert.equal(variantBases.reduce((count, base) => count + base.pictures.length, 0), 35);
  for (const base of variantBases) {
    base.pictures.forEach((picture, pictureId) => {
      const key = `${base.code}@${pictureId}`;
      const source = itemVisuals[key];
      assert.ok(source, `Missing native visual ${key} (${picture}).`);
      const path = publicAssetPath(source);
      assert.ok(existsSync(path), `Missing PNG for ${key}.`);
      const png = readFileSync(path);
      assert.deepEqual([...png.subarray(0, 8)], [137, 80, 78, 71, 13, 10, 26, 10]);
      assert.ok(png.readUInt32BE(16) > 0, `${key} width`);
      assert.ok(png.readUInt32BE(20) > 0, `${key} height`);
    });
  }
});

test('gives every RuneWizard-style quick-add entry a governed item sprite', () => {
  const entries = availableItemGroups().flatMap((group) => group.entries);
  assert.ok(entries.length > 0);
  for (const entry of entries) {
    assert.ok(itemVisuals[entry.type], `Missing quick-add visual for ${entry.name} (${entry.type}).`);
    assert.ok(existsSync(publicAssetPath(itemVisuals[entry.type])), `Missing quick-add PNG for ${entry.type}.`);
  }
});

test('generates every governed skill as an individual native visual', () => {
  assert.deepEqual(Object.keys(skillVisuals).sort(), ['ama', 'ass', 'bar', 'dru', 'nec', 'pal', 'sor', 'war']);
  assert.equal(skillCatalog.length, 240);
  for (const [classCode, sources] of Object.entries(skillVisuals)) {
    assert.equal(sources.length, 30, `${classCode} individual skill visual count`);
    assert.equal(new Set(sources).size, 30, `${classCode} unique skill visual count`);
    for (const source of sources) {
      const path = publicAssetPath(source);
      assert.ok(existsSync(path), `Missing individual skill visual ${source}.`);
      const png = readFileSync(path);
      assert.deepEqual([...png.subarray(0, 8)], [137, 80, 78, 71, 13, 10, 26, 10]);
      assert.equal(png.readUInt32BE(16), 130, `${source} width`);
      assert.equal(png.readUInt32BE(20), 130, `${source} height`);
    }
    const cells = skillCatalog
      .filter((skill) => skill.classCode === classCode)
      .map((skill) => skill.iconCell)
      .sort((left, right) => left - right);
    assert.deepEqual(cells, Array.from({ length: 30 }, (_, index) => index * 2));
    assert.deepEqual(
      cells.map((cell) => sources[cell / 2]),
      sources,
      `${classCode} iconCell mapping`,
    );
  }
});

test('generates the three native skill tree backgrounds for every BKVince class', () => {
  assert.deepEqual(Object.keys(skillTreeVisuals).sort(), ['ama', 'ass', 'bar', 'dru', 'nec', 'pal', 'sor', 'war']);
  for (const [classCode, sources] of Object.entries(skillTreeVisuals)) {
    assert.equal(sources.length, 3, `${classCode} background count`);
    for (const source of sources) {
      const path = publicAssetPath(source);
      assert.ok(existsSync(path), `Missing native skill tree ${source}.`);
      const png = readFileSync(path);
      assert.deepEqual([...png.subarray(0, 8)], [137, 80, 78, 71, 13, 10, 26, 10]);
      assert.equal(png.readUInt32BE(16), 895, `${source} width`);
      assert.equal(png.readUInt32BE(20), 1169, `${source} height`);
    }
  }
});

test('generates a square portrait for every BKVince class', () => {
  assert.deepEqual(
    Object.keys(classPortraitVisuals).sort(),
    ['amazon', 'assassin', 'barbarian', 'druid', 'necromancer', 'paladin', 'sorceress', 'warlock'],
  );
  for (const [className, source] of Object.entries(classPortraitVisuals)) {
    const path = publicAssetPath(source);
    assert.ok(existsSync(path), `Missing portrait for ${className}.`);
    const png = readFileSync(path);
    assert.deepEqual([...png.subarray(0, 8)], [137, 80, 78, 71, 13, 10, 26, 10]);
    assert.equal(png.readUInt32BE(16), 120, `${className} portrait width`);
    assert.equal(png.readUInt32BE(20), 120, `${className} portrait height`);
  }
});

test('gives every quest a versioned BKVince item motif', () => {
  const quests = questActs.flatMap((act) => act.quests);
  assert.equal(quests.length, 27);
  for (const quest of quests) {
    assert.ok(quest.iconCode, `Missing motif code for ${quest.id}.`);
    assert.ok(itemVisuals[quest.iconCode], `Missing visual for ${quest.id} (${quest.iconCode}).`);
    assert.ok(existsSync(publicAssetPath(itemVisuals[quest.iconCode])));
  }
});
