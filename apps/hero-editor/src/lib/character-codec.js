import { read, write } from '@d2runewizard/d2s';

import constants from '../data/bkvince-constants.generated.js';

export const SAVE_VERSION = 0x69;
export const CODEC_OPTIONS = Object.freeze({
  disableItemEnhancements: true,
  sortProperties: false,
});

const SKILL_OFFSETS = Object.freeze({
  Amazon: 6,
  Sorceress: 36,
  Necromancer: 66,
  Paladin: 96,
  Barbarian: 126,
  Druid: 221,
  Assassin: 251,
  Warlock: 373,
});

const ATTRIBUTE_FIELDS = Object.freeze([
  ['strength', 'Strength', 0],
  ['energy', 'Energy', 1],
  ['dexterity', 'Dexterity', 2],
  ['vitality', 'Vitality', 3],
  ['unused_stats', 'Unused stat points', 4],
  ['unused_skill_points', 'Unused skill points', 5],
  ['current_hp', 'Current life', 6],
  ['max_hp', 'Maximum life', 7],
  ['current_mana', 'Current mana', 8],
  ['max_mana', 'Maximum mana', 9],
  ['current_stamina', 'Current stamina', 10],
  ['max_stamina', 'Maximum stamina', 11],
  ['level', 'Level', 12],
  ['experience', 'Experience', 13],
  ['gold', 'Carried gold', 14],
  ['stashed_gold', 'Stashed gold', 15],
]);

export const BK_STARTER_CHARM_LAYOUT = Object.freeze([
  Object.freeze({ type: 'mff', uniqueId: 439, x: 10, y: 0 }),
  Object.freeze({ type: 'mfc', uniqueId: 438, x: 10, y: 1 }),
  ...Array.from({ length: 6 }, (_, index) => Object.freeze({
    type: 'mfd',
    uniqueId: 440,
    x: 10,
    y: index + 2,
  })),
]);

const BK_STARTER_CHARM_ATTRIBUTES = Object.freeze({
  mfd: Object.freeze([]),
  mfc: Object.freeze([
    Object.freeze({ id: 39, values: Object.freeze([-30]), name: 'fireresist' }),
    Object.freeze({ id: 41, values: Object.freeze([-30]), name: 'lightresist' }),
    Object.freeze({ id: 43, values: Object.freeze([-30]), name: 'coldresist' }),
    Object.freeze({ id: 45, values: Object.freeze([-30]), name: 'poisonresist' }),
    Object.freeze({ id: 80, values: Object.freeze([-199]), name: 'item_magicbonus' }),
    Object.freeze({ id: 240, values: Object.freeze([4]), name: 'item_find_magic_perlevel' }),
  ]),
  mff: Object.freeze([
    Object.freeze({ id: 80, values: Object.freeze([35]), name: 'item_magicbonus' }),
  ]),
});

export const itemContainers = Object.freeze({
  inventory: Object.freeze({
    id: 'inventory',
    label: 'Inventory',
    width: 11,
    height: 8,
    locationId: 0,
    altPositionId: 1,
  }),
  cube: Object.freeze({
    id: 'cube',
    label: 'Horadric Cube',
    width: 6,
    height: 6,
    locationId: 0,
    altPositionId: 4,
  }),
  stash: Object.freeze({
    id: 'stash',
    label: 'Personal stash',
    width: 16,
    height: 13,
    locationId: 0,
    altPositionId: 5,
  }),
});

export const attributeFields = Object.freeze(ATTRIBUTE_FIELDS.map(([key, label, statId]) => {
  const bits = constants.magical_properties[statId]?.cB;
  if (!Number.isInteger(bits)) {
    throw new Error(`Missing save bits for ${key}.`);
  }
  const encodedMaximum = 2 ** bits - 1;
  return Object.freeze({
    key,
    label,
    statId,
    maximum: statId >= 6 && statId <= 11
      ? Math.floor(encodedMaximum / 256)
      : encodedMaximum,
  });
}));

export function supportedClasses() {
  return constants.classes
    .filter((entry) => entry && Number.isInteger(SKILL_OFFSETS[entry.n]))
    .map((entry) => ({ code: entry.c, name: entry.n }));
}

export function editableSnapshot(model) {
  return {
    name: model.header.name,
    className: model.header.class,
    mapId: model.header.map_id >>> 0,
    expansion: Boolean(model.header.status.expansion),
    hardcore: Boolean(model.header.status.hardcore),
    died: Boolean(model.header.status.died),
    ladder: Boolean(model.header.status.ladder),
    attributes: Object.fromEntries(
      attributeFields.map(({ key }) => [key, Number(model.attributes[key] ?? 0)]),
    ),
    itemPlacements: model.items.map((item, index) => itemPlacementSnapshot(item, index)),
  };
}

export function describeItem(item, index) {
  const details = itemDefinition(item?.type);
  const rawName = details?.n || item?.type_name || item?.categories?.[0] || item?.type || 'Unknown item';
  const name = cleanItemName(rawName);
  return {
    index,
    type: item?.type || '????',
    name,
    width: positiveDimension(details?.iw),
    height: positiveDimension(details?.ih),
    icon: details?.i || null,
    categories: Array.isArray(item?.categories) ? [...item.categories] : [],
  };
}

export function containerForPlacement(placement) {
  if (placement.locationId === 1) return 'equipment';
  if (placement.locationId === 2) return 'belt';
  if (placement.locationId !== 0) return 'other';
  return Object.values(itemContainers).find(
    ({ altPositionId }) => altPositionId === placement.altPositionId,
  )?.id || 'other';
}

export function moveItemPlacement(placements, items, itemIndex, containerId, x, y) {
  if (!Array.isArray(placements) || !Array.isArray(items) || placements.length !== items.length) {
    throw new Error('Item placements no longer match the parsed D2S item list.');
  }
  const container = itemContainers[containerId];
  if (!container) throw new Error(`Unsupported target container: ${containerId}.`);
  validateInteger('Item index', itemIndex, 0, placements.length - 1);
  validateInteger('Item column', x, 0, container.width - 1);
  validateInteger('Item row', y, 0, container.height - 1);

  const descriptor = describeItem(items[itemIndex], itemIndex);
  validateItemBounds(descriptor, container, x, y);
  const candidate = {
    ...placements[itemIndex],
    locationId: container.locationId,
    equippedId: 0,
    x,
    y,
    altPositionId: container.altPositionId,
  };
  validateCollision(candidate, descriptor, placements, items, itemIndex, container);

  return placements.map((placement, index) => (index === itemIndex ? candidate : placement));
}

export function snapshotsEqual(left, right) {
  return JSON.stringify(left) === JSON.stringify(right);
}

export async function openCharacter(input, fileName = 'character.d2s') {
  const sourceBytes = cloneBytes(input);
  validateSaveEnvelope(sourceBytes);
  const model = await read(sourceBytes, constants, CODEC_OPTIONS);
  validateSupportedModel(model);
  const initial = editableSnapshot(model);
  return {
    fileName,
    sourceBytes,
    model,
    initial,
    origin: 'opened',
  };
}

export async function createBlankCharacter({
  name,
  className,
  hardcore = false,
  ladder = false,
}) {
  validateName(name);
  const classData = constants.classes.find((entry) => entry?.n === className);
  if (!classData || !Number.isInteger(SKILL_OFFSETS[className])) {
    throw new Error(`Class ${className} cannot be serialized by the current BKVince codec.`);
  }

  const model = blankModel({ name, classData, hardcore, ladder });
  const bytes = cloneBytes(await write(model, constants, CODEC_OPTIONS));
  validateSaveEnvelope(bytes);
  const reparsed = await read(bytes, constants, CODEC_OPTIONS);
  validateSupportedModel(reparsed);
  return {
    fileName: `${name}.d2s`,
    sourceBytes: bytes,
    model: reparsed,
    initial: editableSnapshot(reparsed),
    origin: 'created',
  };
}

export async function exportCharacter(document, editable) {
  if (snapshotsEqual(document.initial, editable)) {
    return {
      bytes: cloneBytes(document.sourceBytes),
      reparsed: document.model,
      byteExact: true,
    };
  }
  validateEditable(editable, document.model.items);

  const nextModel = structuredClone(document.model);
  applyEditable(nextModel, editable);
  const bytes = cloneBytes(await write(nextModel, constants, CODEC_OPTIONS));
  validateSaveEnvelope(bytes);
  const reparsed = await read(bytes, constants, CODEC_OPTIONS);
  validateRoundTrip(reparsed, editable, bytes.length, nextModel);
  return { bytes, reparsed, byteExact: false };
}

export function validateEditable(editable, items = []) {
  validateName(editable.name);
  if (!supportedClasses().some(({ name }) => name === editable.className)) {
    throw new Error(`Unsupported BKVince class: ${editable.className}.`);
  }
  validateInteger('Map seed', editable.mapId, 0, 0xffffffff);
  for (const field of attributeFields) {
    validateInteger(field.label, editable.attributes[field.key], 0, field.maximum);
  }
  if (editable.attributes.level < 1 || editable.attributes.level > 99) {
    throw new RangeError('Level must be between 1 and 99.');
  }
  validateItemPlacements(editable.itemPlacements, items);
}

export function suggestedFileName(editable) {
  return `${editable.name.replace(/[^A-Za-z_-]/g, '') || 'character'}.d2s`;
}

export function validateSaveEnvelope(bytesInput) {
  const bytes = cloneBytes(bytesInput);
  if (bytes.length < 16) {
    throw new Error('D2S file is too short.');
  }
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const declaredSize = view.getUint32(8, true);
  if (declaredSize !== bytes.length) {
    throw new Error(`D2S size mismatch: header=${declaredSize}, bytes=${bytes.length}.`);
  }
  const storedChecksum = view.getUint32(12, true);
  let checksum = 0;
  for (let index = 0; index < bytes.length; index += 1) {
    let byte = index >= 12 && index <= 15 ? 0 : bytes[index];
    if ((checksum & 0x80000000) !== 0) byte += 1;
    checksum = (byte + checksum * 2) >>> 0;
  }
  if (storedChecksum !== checksum) {
    throw new Error(
      `D2S checksum mismatch: header=${storedChecksum.toString(16)}, calculated=${checksum.toString(16)}.`,
    );
  }
  return { declaredSize, checksum };
}

function validateName(name) {
  if (!/^[A-Za-z][A-Za-z_-]{1,14}$/.test(name)) {
    throw new Error('Character name must contain 2–15 letters, hyphens, or underscores and start with a letter.');
  }
}

function validateInteger(label, value, minimum, maximum) {
  if (!Number.isInteger(value) || value < minimum || value > maximum) {
    throw new RangeError(`${label} must be an integer between ${minimum} and ${maximum}.`);
  }
}

function validateSupportedModel(model) {
  if (model.header.version !== SAVE_VERSION) {
    throw new Error(`Unsupported D2S version ${model.header.version}; this slice accepts BKVince v105 saves.`);
  }
  if (!supportedClasses().some(({ name }) => name === model.header.class)) {
    throw new Error(`Unsupported BKVince class in save: ${model.header.class}.`);
  }
}

function applyEditable(model, editable) {
  model.header.name = editable.name;
  model.header.map_id = editable.mapId >>> 0;
  model.header.status = {
    ...model.header.status,
    expansion: editable.expansion,
    hardcore: editable.hardcore,
    died: editable.died,
    ladder: editable.ladder,
  };
  model.attributes = {
    ...model.attributes,
    ...editable.attributes,
  };
  model.header.level = editable.attributes.level;
  if (!Array.isArray(model.skills) || model.skills.length !== 30) {
    model.skills = blankSkills(model.header.class);
  }
  editable.itemPlacements.forEach((placement, index) => {
    const item = model.items[index];
    if (!item || item.type !== placement.type) {
      throw new Error(`Item ${index} no longer matches its parsed D2S record.`);
    }
    item.location_id = placement.locationId;
    item.equipped_id = placement.equippedId;
    item.position_x = placement.x;
    item.position_y = placement.y;
    item.alt_position_id = placement.altPositionId;
  });
}

function validateRoundTrip(model, editable, byteLength, writtenModel) {
  if (model.header.filesize !== byteLength) {
    throw new Error(`Export size mismatch: header=${model.header.filesize}, bytes=${byteLength}.`);
  }
  const actual = editableSnapshot(model);
  if (!snapshotsEqual(actual, editable)) {
    throw new Error('The exported D2S did not preserve all edited values and item placements.');
  }
  if (model.items.length !== writtenModel.items.length) {
    throw new Error('The exported D2S changed the number of root item records.');
  }
  model.items.forEach((item, index) => {
    if (JSON.stringify(itemPayloadSnapshot(item)) !== JSON.stringify(itemPayloadSnapshot(writtenModel.items[index]))) {
      throw new Error(`The exported D2S changed item ${index} outside its placement fields.`);
    }
  });
}

function itemPlacementSnapshot(item, index) {
  return {
    index,
    type: item.type,
    locationId: Number(item.location_id),
    equippedId: Number(item.equipped_id),
    x: Number(item.position_x),
    y: Number(item.position_y),
    altPositionId: Number(item.alt_position_id),
  };
}

function itemPayloadSnapshot(item) {
  const payload = structuredClone(item);
  delete payload.location_id;
  delete payload.equipped_id;
  delete payload.position_x;
  delete payload.position_y;
  delete payload.alt_position_id;
  return payload;
}

function validateItemPlacements(placements, items) {
  if (!Array.isArray(placements) || placements.length !== items.length) {
    throw new Error('Item placements no longer match the parsed D2S item list.');
  }
  placements.forEach((placement, index) => {
    const item = items[index];
    if (!item || placement.index !== index || placement.type !== item.type) {
      throw new Error(`Item ${index} no longer matches its parsed D2S record.`);
    }
    validateInteger(`Item ${index} location`, placement.locationId, 0, 7);
    validateInteger(`Item ${index} equipped slot`, placement.equippedId, 0, 15);
    validateInteger(`Item ${index} column`, placement.x, 0, 15);
    validateInteger(`Item ${index} row`, placement.y, 0, 15);
    validateInteger(`Item ${index} stored page`, placement.altPositionId, 0, 7);

    const containerId = containerForPlacement(placement);
    const container = itemContainers[containerId];
    if (!container) return;
    const descriptor = describeItem(item, index);
    validateItemBounds(descriptor, container, placement.x, placement.y);
    validateCollision(placement, descriptor, placements, items, index, container);
  });
}

function validateItemBounds(descriptor, container, x, y) {
  if (x + descriptor.width > container.width || y + descriptor.height > container.height) {
    throw new RangeError(
      `${descriptor.name} (${descriptor.width}×${descriptor.height}) does not fit at `
      + `${container.label} ${x + 1},${y + 1}.`,
    );
  }
}

function validateCollision(candidate, descriptor, placements, items, ignoredIndex, container) {
  for (let index = 0; index < placements.length; index += 1) {
    if (index === ignoredIndex) continue;
    const other = placements[index];
    if (containerForPlacement(other) !== container.id) continue;
    const otherDescriptor = describeItem(items[index], index);
    if (rectanglesOverlap(
      candidate.x,
      candidate.y,
      descriptor.width,
      descriptor.height,
      other.x,
      other.y,
      otherDescriptor.width,
      otherDescriptor.height,
    )) {
      throw new Error(`${descriptor.name} overlaps ${otherDescriptor.name} in ${container.label}.`);
    }
  }
}

function rectanglesOverlap(ax, ay, aw, ah, bx, by, bw, bh) {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

function itemDefinition(type) {
  return constants.armor_items[type]
    || constants.weapon_items[type]
    || constants.other_items[type]
    || null;
}

function cleanItemName(value) {
  const text = String(value);
  const visible = text.includes('}') ? text.slice(text.lastIndexOf('}') + 1) : text;
  return visible.replace(/ÿc./g, '').trim() || 'Unknown item';
}

function positiveDimension(value) {
  return Number.isInteger(value) && value > 0 ? value : 1;
}

function blankModel({ name, classData, hardcore, ladder }) {
  const now = Math.floor(Date.now() / 1000);
  const baseLife = Number(classData.a.vit) + Number(classData.a.hpadd);
  const skills = blankSkills(classData.n);
  return {
    header: {
      identifier: 'aa55aa55',
      version: SAVE_VERSION,
      filesize: 0,
      checksum: '00000000',
      name,
      status: {
        expansion: true,
        died: false,
        hardcore,
        ladder,
      },
      progression: 0,
      active_arms: 0,
      class: classData.n,
      level: 1,
      created: now,
      last_played: now,
      assigned_skills: [],
      left_skill: '',
      right_skill: '',
      left_swap_skill: '',
      right_swap_skill: '',
      menu_appearance: emptyMenuAppearance(),
      difficulty: { Normal: 0, Nightmare: 0, Hell: 0 },
      map_id: cryptoSafeUint32(),
      dead_merc: 0,
      merc_id: '0',
      merc_name_id: 0,
      merc_type: 0,
      merc_experience: 0,
      quests_normal: emptyQuestLog(),
      quests_nm: emptyQuestLog(),
      quests_hell: emptyQuestLog(),
      waypoints: {
        normal: emptyWaypoints(),
        nm: emptyWaypoints(),
        hell: emptyWaypoints(),
      },
      npcs: {
        normal: emptyNpcs(),
        nm: emptyNpcs(),
        hell: emptyNpcs(),
      },
    },
    attributes: {
      strength: Number(classData.a.str),
      energy: Number(classData.a.int),
      dexterity: Number(classData.a.dex),
      vitality: Number(classData.a.vit),
      unused_stats: 0,
      unused_skill_points: 0,
      current_hp: baseLife,
      max_hp: baseLife,
      current_mana: Number(classData.a.int),
      max_mana: Number(classData.a.int),
      current_stamina: Number(classData.a.stam),
      max_stamina: Number(classData.a.stam),
      level: 1,
      experience: 0,
      gold: 0,
      stashed_gold: 0,
    },
    item_bonuses: [],
    skills,
    items: bkStarterCharms(),
    corpse_items: [],
    merc_items: [],
    golem_item: null,
    demon: null,
    is_dead: 0,
  };
}

function bkStarterCharms() {
  return BK_STARTER_CHARM_LAYOUT.map(({ type, uniqueId, x, y }) => ({
    _unknown_data: {
      realm_data: Array.from({ length: 4 }, cryptoSafeUint32),
      chest_stackable: type === 'mfd' ? 1 : 0,
    },
    identified: 1,
    socketed: 0,
    new: 0,
    is_ear: 0,
    starter_item: 1,
    simple_item: 0,
    ethereal: 0,
    personalized: 0,
    given_runeword: 0,
    version: '101',
    location_id: itemContainers.inventory.locationId,
    equipped_id: 0,
    position_x: x,
    position_y: y,
    alt_position_id: itemContainers.inventory.altPositionId,
    type,
    type_id: 3,
    max_durability: 0,
    current_durability: 0,
    nr_of_items_in_sockets: 0,
    socketed_items: [],
    id: cryptoSafeUint32(),
    level: 1,
    quality: 7,
    multiple_pictures: 0,
    class_specific: 0,
    unique_id: uniqueId,
    timestamp: 1,
    amount_in_shared_stash: type === 'mfd' ? 0 : undefined,
    magic_attributes: BK_STARTER_CHARM_ATTRIBUTES[type].map((attribute) => ({
      ...attribute,
      values: [...attribute.values],
    })),
  }));
}

function blankSkills(className) {
  const offset = SKILL_OFFSETS[className];
  return Array.from({ length: 30 }, (_, index) => {
    const id = offset + index;
    const definition = constants.skills[id];
    if (!definition?.s) {
      throw new Error(`Missing BKVince skill ${id} for ${className}.`);
    }
    return { id, name: definition.s, points: 0 };
  });
}

function emptyQuest() {
  return {
    is_completed: false,
    is_requirement_completed: false,
    is_received: false,
    unk3: false,
    unk4: false,
    unk5: false,
    unk6: false,
    consumed_scroll: false,
    unk8: false,
    unk9: false,
    unk10: false,
    unk11: false,
    closed: false,
    done_recently: false,
    unk14: false,
    unk15: false,
  };
}

function emptyQuestLog() {
  const quest = () => emptyQuest();
  return {
    act_i: {
      introduced: false,
      den_of_evil: quest(),
      sisters_burial_grounds: quest(),
      tools_of_the_trade: quest(),
      the_search_for_cain: quest(),
      the_forgotten_tower: quest(),
      sisters_to_the_slaughter: quest(),
      completed: false,
    },
    act_ii: {
      introduced: false,
      radaments_lair: quest(),
      the_horadric_staff: quest(),
      tainted_sun: quest(),
      arcane_sanctuary: quest(),
      the_summoner: quest(),
      the_seven_tombs: quest(),
      completed: false,
    },
    act_iii: {
      introduced: false,
      lam_esens_tome: quest(),
      khalims_will: quest(),
      blade_of_the_old_religion: quest(),
      the_golden_bird: quest(),
      the_blackened_temple: quest(),
      the_guardian: quest(),
      completed: false,
    },
    act_iv: {
      introduced: false,
      the_fallen_angel: quest(),
      terrors_end: quest(),
      hellforge: quest(),
      completed: false,
    },
    act_v: {
      introduced: false,
      siege_on_harrogath: quest(),
      rescue_on_mount_arreat: quest(),
      prison_of_ice: quest(),
      betrayal_of_harrogath: quest(),
      rite_of_passage: quest(),
      eve_of_destruction: quest(),
      completed: false,
    },
  };
}

function emptyWaypoints() {
  return {
    act_i: fromKeys([
      'rogue_encampement', 'cold_plains', 'stony_field', 'dark_woods', 'black_marsh',
      'outer_cloister', 'jail_lvl_1', 'inner_cloister', 'catacombs_lvl_2',
    ]),
    act_ii: fromKeys([
      'lut_gholein', 'sewers_lvl_2', 'dry_hills', 'halls_of_the_dead_lvl_2',
      'far_oasis', 'lost_city', 'palace_cellar_lvl_1', 'arcane_sanctuary', 'canyon_of_the_magi',
    ]),
    act_iii: fromKeys([
      'kurast_docks', 'spider_forest', 'great_marsh', 'flayer_jungle', 'lower_kurast',
      'kurast_bazaar', 'upper_kurast', 'travincal', 'durance_of_hate_lvl_2',
    ]),
    act_iv: fromKeys(['the_pandemonium_fortress', 'city_of_the_damned', 'river_of_flame']),
    act_v: fromKeys([
      'harrogath', 'frigid_highlands', 'arreat_plateau', 'crystalline_passage',
      'glacial_trail', 'halls_of_pain', 'frozen_tundra', 'the_ancients_way',
      'worldstone_keep_lvl_2',
    ]),
  };
}

function emptyNpcs() {
  return Object.fromEntries([
    'warriv_act_ii', 'charsi', 'warriv_act_i', 'kashya', 'akara', 'gheed', 'greiz',
    'jerhyn', 'meshif_act_ii', 'geglash', 'lysnader', 'fara', 'drogan', 'alkor',
    'hratli', 'ashera', 'cain_act_iii', 'elzix', 'malah', 'anya', 'natalya',
    'meshif_act_iii', 'ormus', 'cain_act_v', 'qualkehk', 'nihlathak',
  ].map((key) => [key, { intro: false, congrats: false }]));
}

function emptyMenuAppearance() {
  return Object.fromEntries([
    'head', 'torso', 'legs', 'right_arm', 'left_arm', 'right_hand', 'left_hand', 'shield',
    'special1', 'special2', 'special3', 'special4', 'special5', 'special6', 'special7', 'special8',
  ].map((key) => [key, { graphic: 0xff, tint: 0xff }]));
}

function fromKeys(keys) {
  return Object.fromEntries(keys.map((key) => [key, false]));
}

function cryptoSafeUint32() {
  const words = new Uint32Array(1);
  if (globalThis.crypto?.getRandomValues) {
    globalThis.crypto.getRandomValues(words);
    return words[0];
  }
  return Math.floor(Math.random() * 0x100000000);
}

function cloneBytes(input) {
  if (input instanceof Uint8Array) {
    return new Uint8Array(input);
  }
  if (input instanceof ArrayBuffer) {
    return new Uint8Array(input.slice(0));
  }
  if (ArrayBuffer.isView(input)) {
    return new Uint8Array(input.buffer.slice(input.byteOffset, input.byteOffset + input.byteLength));
  }
  throw new TypeError('Expected D2S bytes as an ArrayBuffer or typed array.');
}
