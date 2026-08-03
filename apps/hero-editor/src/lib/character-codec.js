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
  };
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
  validateEditable(editable);
  if (snapshotsEqual(document.initial, editable)) {
    return {
      bytes: cloneBytes(document.sourceBytes),
      reparsed: document.model,
      byteExact: true,
    };
  }

  const nextModel = structuredClone(document.model);
  applyEditable(nextModel, editable);
  const bytes = cloneBytes(await write(nextModel, constants, CODEC_OPTIONS));
  validateSaveEnvelope(bytes);
  const reparsed = await read(bytes, constants, CODEC_OPTIONS);
  validateRoundTrip(reparsed, editable, bytes.length);
  return { bytes, reparsed, byteExact: false };
}

export function validateEditable(editable) {
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
}

function validateRoundTrip(model, editable, byteLength) {
  if (model.header.filesize !== byteLength) {
    throw new Error(`Export size mismatch: header=${model.header.filesize}, bytes=${byteLength}.`);
  }
  const actual = editableSnapshot(model);
  if (!snapshotsEqual(actual, editable)) {
    throw new Error('The exported D2S did not preserve all edited General/Stats values.');
  }
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
    items: [],
    corpse_items: [],
    merc_items: [],
    golem_item: null,
    demon: null,
    is_dead: 0,
  };
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
  ].map((key) => [key, { graphic: 0, tint: 0 }]));
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
