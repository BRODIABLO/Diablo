import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

const require = createRequire(import.meta.url);
const { ENCODING, parseTable, serializeTable, writeTable } = require('../build-data/tsv.js');

const SCRIPT_DIR = path.dirname(fileURLToPath(import.meta.url));
export const REPOSITORY_ROOT = path.resolve(SCRIPT_DIR, '..', '..');
export const WORKBENCH_ROOT = path.join(
  REPOSITORY_ROOT,
  'reverse-engineering',
  'd2r-3.2.92777',
);
export const DEFAULT_OUTPUT_ROOT = path.join(WORKBENCH_ROOT, 'player-sequences');
export const DEFAULT_RUNTIME_LAYOUT_PATH = path.join(
  DEFAULT_OUTPUT_ROOT,
  'd2r-3.3.93847-player-sequence-runtime.json',
);
export const DEFAULT_RUNTIME_CAPTURE_PATH = path.join(
  REPOSITORY_ROOT,
  'analysis-cache',
  'runtime',
  'player-sequences',
  'd2r-3.3.93847-runtime-snapshot.json',
);
export const D2MOO_ROOT = path.join(REPOSITORY_ROOT, 'analysis-cache', 'references', 'D2MOO');
export const D2MOO_SEQUENCE_SOURCE = path.join(
  D2MOO_ROOT,
  'source',
  'D2Common',
  'src',
  'DataTbls',
  'SequenceTbls.cpp',
);
export const D2MOO_SEQUENCE_HEADER = path.join(
  D2MOO_ROOT,
  'source',
  'D2Common',
  'include',
  'DataTbls',
  'SequenceTbls.h',
);
export const PLRMODE_PATH = path.join(
  REPOSITORY_ROOT,
  'data-vanilla3.3',
  'data',
  'data',
  'global',
  'excel',
  'plrmode.txt',
);
export const SKILLS_PATH = path.join(
  REPOSITORY_ROOT,
  'data-vanilla3.3',
  'data',
  'data',
  'global',
  'excel',
  'skills.txt',
);

const WEAPON_CLASSES = Object.freeze([
  'HTH',
  '1HT',
  '2HT',
  '1HS',
  '2HS',
  'BOW',
  'XBW',
  'STF',
  '1JS',
  '1JT',
  '1SS',
  '1ST',
  'HT1',
  'HT2',
]);

const EXPECTED_WEAPON_CLASS_IDS = Object.freeze([
  0,
  3,
  6,
  2,
  5,
  1,
  7,
  4,
  8,
  9,
  10,
  11,
  12,
  13,
]);

const PLAYER_MODE_KEYS = Object.freeze([
  'DEATH',
  'NEUTRAL',
  'WALK',
  'RUN',
  'GETHIT',
  'TOWNNEUTRAL',
  'TOWNWALK',
  'ATTACK1',
  'ATTACK2',
  'BLOCK',
  'CAST',
  'THROW',
  'KICK',
  'SPECIAL1',
  'SPECIAL2',
  'SPECIAL3',
  'SPECIAL4',
  'DEAD',
  'SEQUENCE',
  'KNOCKBACK',
]);

const EVENT_NAMES = Object.freeze([
  'NONE',
  'MELEE_ATTACK',
  'MISSILE_ATTACK',
  'PLAY_SOUND',
  'TRIGGER_SKILL',
]);

const EXPECTED_MODE_CODES = Object.freeze([
  'DT',
  'NU',
  'WL',
  'RN',
  'GH',
  'TN',
  'TW',
  'A1',
  'A2',
  'BL',
  'SC',
  'TH',
  'KK',
  'S1',
  'S2',
  'S3',
  'S4',
  'DD',
  'SQ',
  'KB',
]);

const NATIVE_LAYOUT = Object.freeze({
  sequenceNumberResolverRva: 0x33dbc0,
  playerSequenceResolverRva: 0x3cb890,
  playerSequenceTableRva: 0x2386650,
  weaponClassMapRva: 0x2386730,
  staticWeaponClassMapRva: 0x19eaf70,
  slotCount: 26,
  weaponClassCount: WEAPON_CLASSES.length,
  descriptorSize: 24,
  recordSize: 6,
});

const EXPECTED_BUILD = Object.freeze({
  version: '3.3.93847',
  buildKey: '623f7a1f73eabb08ccb2b2046e3f9164',
  buildInfoSha256: '2EBCAD0521DBF038D5A7FE5395E96B4BEF6D4F0774F7B1F840E03C3DE9CB067A',
  retailExeSha256: 'E1F5436E3D9687F644EF16938B1B183D1FDEF434F18CF66D852CF68F48CC8936',
});

function sha256(bytes) {
  return crypto.createHash('sha256').update(bytes).digest('hex').toUpperCase();
}

function hex(value) {
  return `0x${value.toString(16).toUpperCase()}`;
}

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

export function readGovernedTable(filePath) {
  const raw = fs.readFileSync(filePath, ENCODING);
  const table = parseTable(filePath);
  assert.equal(serializeTable(table), raw, `TSV round-trip drift: ${filePath}`);
  assert.equal(table.eol, '\r\n', `Expected CRLF TSV: ${filePath}`);
  return { raw, table };
}

function rowsAsObjects(table) {
  return table.rows.map((row) => Object.fromEntries(
    table.headers.map((header, index) => [header, row[index] ?? '']),
  ));
}

export function parsePlayerModes(table) {
  const rows = rowsAsObjects(table);
  assert.equal(rows.length, PLAYER_MODE_KEYS.length, 'Unexpected plrmode.txt row count');
  rows.forEach((row, index) => {
    assert.equal(row.Code, EXPECTED_MODE_CODES[index], `Unexpected player mode at ${index}`);
  });
  return rows.map((row, id) => ({
    id,
    key: PLAYER_MODE_KEYS[id],
    name: row.Name,
    token: row.Token,
    code: row.Code,
  }));
}

export function parseD2MooArrays(source, playerModes) {
  const modeByKey = new Map(playerModes.map((mode) => [mode.key, mode]));
  const arrays = [];
  const declaration = /D2AnimSeqTxt\s+(gPlayerSequence\w+)\[(\d+)\]\s*=\s*\{([\s\S]*?)\n\};/g;
  for (const match of source.matchAll(declaration)) {
    const [, name, countText, body] = match;
    const records = [];
    const rowPattern = /\{\s*0,\s*(PLRMODE_[A-Z0-9_]+|0),\s*(\d+),\s*(\d+),\s*ANIMSEQ_EVENT_([A-Z_]+)\s*\},/g;
    for (const row of body.matchAll(rowPattern)) {
      const [, modeExpression, frameText, directionText, eventKey] = row;
      const modeKey = modeExpression === '0' ? PLAYER_MODE_KEYS[0] : modeExpression.slice(8);
      const mode = modeByKey.get(modeKey);
      assert(mode, `Unknown D2MOO player mode ${modeExpression}`);
      const event = EVENT_NAMES.indexOf(eventKey);
      assert.notEqual(event, -1, `Unknown D2MOO sequence event ${eventKey}`);
      records.push({
        modeId: mode.id,
        frame: Number(frameText),
        direction: Number(directionText),
        eventId: event,
      });
    }
    const declaredCount = Number(countText);
    assert.equal(records.length, declaredCount, `Record count mismatch for ${name}`);
    arrays.push({ name, records });
  }
  assert.equal(arrays.length, 34, 'Expected 34 legacy player sequence arrays');
  return arrays;
}

export function parseD2MooGroups(source, arrays) {
  const knownArrays = new Set(arrays.map((array) => array.name));
  const groups = [];
  const declaration = /D2PlayerWeaponSequencesStrc\s+(gPlayerWeaponsSequence\w+)\s*=\s*\{([\s\S]*?)\n\};/g;
  for (const match of source.matchAll(declaration)) {
    const [, name, body] = match;
    const slots = [];
    const slotPattern = /^\s*(gPlayerSequence\w+|0),\s*(\d+),\s*(\d+),\s*$/gm;
    for (const slot of body.matchAll(slotPattern)) {
      const [, arrayName, sequenceFramesText, animationFramesText] = slot;
      if (arrayName !== '0') {
        assert(knownArrays.has(arrayName), `Unknown array ${arrayName} in ${name}`);
      }
      slots.push({
        arrayName: arrayName === '0' ? null : arrayName,
        sequenceFrames: Number(sequenceFramesText),
        animationFrames: Number(animationFramesText),
      });
    }
    assert(slots.length <= WEAPON_CLASSES.length, `Too many weapon slots in ${name}`);
    while (slots.length < WEAPON_CLASSES.length) {
      slots.push({ arrayName: null, sequenceFrames: 0, animationFrames: 0 });
    }
    groups.push({ name, slots });
  }
  assert.equal(groups.length, 23, 'Expected 23 legacy player sequence groups');
  return groups;
}

export function parseD2MooSequenceTable(source, groups) {
  const knownGroups = new Set(groups.map((group) => group.name));
  const declaration = /D2PlayerWeaponSequencesStrc\*\s+gPlayerWeaponsSequenceTable\[24\]\s*=\s*\{([\s\S]*?)\n\};/;
  const match = source.match(declaration);
  assert(match, 'Missing legacy player sequence pointer table');
  const entries = [];
  const entryPattern = /^\s*(NULL|&gPlayerWeaponsSequence\w+),\s*$/gm;
  for (const entry of match[1].matchAll(entryPattern)) {
    const name = entry[1] === 'NULL' ? null : entry[1].slice(1);
    if (name) assert(knownGroups.has(name), `Unknown group ${name}`);
    entries.push(name);
  }
  assert.equal(entries.length, 24, 'Expected 24 legacy pointer-table entries');
  assert.equal(entries[0], null, 'Legacy sequence index zero must be null');
  return entries;
}

export function encodeSequenceRecords(records) {
  const bytes = Buffer.alloc(records.length * 6);
  records.forEach((record, index) => {
    const offset = index * 6;
    bytes.writeUInt16LE(0, offset);
    bytes.writeUInt8(record.modeId, offset + 2);
    bytes.writeUInt8(record.frame, offset + 3);
    bytes.writeUInt8(record.direction, offset + 4);
    bytes.writeUInt8(record.eventId, offset + 5);
  });
  return bytes;
}

export function decodeSequenceRecords(bytes, count) {
  assert(bytes.length >= count * 6, 'Sequence record buffer is truncated');
  return Array.from({ length: count }, (_, index) => {
    const offset = index * 6;
    return {
      sequence: bytes.readUInt16LE(offset),
      modeId: bytes.readUInt8(offset + 2),
      frame: bytes.readUInt8(offset + 3),
      direction: bytes.readUInt8(offset + 4),
      eventId: bytes.readUInt8(offset + 5),
    };
  });
}

export function findAll(buffer, pattern) {
  const matches = [];
  let offset = 0;
  while (offset <= buffer.length - pattern.length) {
    const match = buffer.indexOf(pattern, offset);
    if (match === -1) break;
    matches.push(match);
    offset = match + 1;
  }
  return matches;
}

export function parsePe(buffer) {
  const peOffset = buffer.readUInt32LE(0x3c);
  assert.equal(buffer.toString('ascii', peOffset, peOffset + 4), 'PE\0\0', 'Invalid PE image');
  const sectionCount = buffer.readUInt16LE(peOffset + 6);
  const optionalHeaderSize = buffer.readUInt16LE(peOffset + 20);
  const optionalHeader = peOffset + 24;
  const imageBase = buffer.readBigUInt64LE(optionalHeader + 24);
  const sectionTable = optionalHeader + optionalHeaderSize;
  const sections = Array.from({ length: sectionCount }, (_, index) => {
    const offset = sectionTable + index * 40;
    return {
      name: buffer.toString('ascii', offset, offset + 8).replace(/\0+$/, ''),
      virtualSize: buffer.readUInt32LE(offset + 8),
      rva: buffer.readUInt32LE(offset + 12),
      rawSize: buffer.readUInt32LE(offset + 16),
      rawOffset: buffer.readUInt32LE(offset + 20),
    };
  });
  return { imageBase, sections };
}

export function fileOffsetToRva(pe, fileOffset) {
  const section = pe.sections.find((candidate) => (
    fileOffset >= candidate.rawOffset
    && fileOffset < candidate.rawOffset + candidate.rawSize
  ));
  assert(section, `Unmapped PE file offset ${hex(fileOffset)}`);
  return { section: section.name, rva: section.rva + fileOffset - section.rawOffset };
}

export function rvaToFileOffset(pe, rva) {
  const section = pe.sections.find((candidate) => (
    rva >= candidate.rva
    && rva < candidate.rva + Math.max(candidate.rawSize, candidate.virtualSize)
  ));
  assert(section, `Unmapped PE RVA ${hex(rva)}`);
  const relative = rva - section.rva;
  assert(relative < section.rawSize, `RVA has no raw bytes: ${hex(rva)}`);
  return section.rawOffset + relative;
}

function absolutePointer(imageBase, rva) {
  const buffer = Buffer.alloc(8);
  buffer.writeBigUInt64LE(imageBase + BigInt(rva));
  return buffer;
}

function parseHexInteger(value, label) {
  assert.match(value, /^0x[0-9a-f]+$/i, `Invalid hexadecimal ${label}: ${value}`);
  const parsed = Number(BigInt(value));
  assert(Number.isSafeInteger(parsed), `Unsafe hexadecimal ${label}: ${value}`);
  return parsed;
}

function verifyNativeEvidence(image, pe, weaponClassMap) {
  const witnesses = [
    {
      name: 'SKILLS_GetSeqNumFromSkill entry',
      rva: NATIVE_LAYOUT.sequenceNumberResolverRva,
      bytes: '48895C241048896C24184889742420574883EC20488BDA488BF94885C9',
    },
    {
      name: 'player seqnum field read',
      rva: 0x33dc42,
      bytes: '0FB64133EB62',
    },
    {
      name: 'DATATBLS_GetSeqRecordFromUnit entry',
      rva: NATIVE_LAYOUT.playerSequenceResolverRva,
      bytes: '4055564883EC38488BF1E8A101F8FF',
    },
    {
      name: 'runtime player sequence pointer-table access',
      rva: 0x3cb8f1,
      bytes: '4C8D3D0847C3FF488BCE4D8BB4EF50663802',
    },
    {
      name: '24-byte weapon descriptor stride',
      rva: 0x3cb987,
      bytes: '488D0C40498D3CCE',
    },
  ];
  for (const witness of witnesses) {
    const expected = Buffer.from(witness.bytes, 'hex');
    const offset = rvaToFileOffset(pe, witness.rva);
    assert(
      image.subarray(offset, offset + expected.length).equals(expected),
      `Native witness drift: ${witness.name} at ${hex(witness.rva)}`,
    );
  }

  const staticMapBytes = Buffer.alloc(weaponClassMap.length * 8);
  weaponClassMap.forEach((entry, index) => {
    staticMapBytes.writeInt32LE(entry.weaponClassIndex, index * 8);
    staticMapBytes.writeInt32LE(entry.weaponClassId, index * 8 + 4);
  });
  const mapMatches = findAll(image, staticMapBytes).map((fileOffset) => ({
    fileOffset,
    ...fileOffsetToRva(pe, fileOffset),
  }));
  assert.deepEqual(
    mapMatches.map((match) => match.rva),
    [NATIVE_LAYOUT.staticWeaponClassMapRva],
    'The static weapon-class routing witness is no longer unique',
  );
  return { witnesses, staticWeaponClassMap: mapMatches[0] };
}

function normalizeRuntimeSnapshot(snapshot) {
  assert.equal(snapshot.schemaVersion, 1, 'Unsupported player-sequence runtime snapshot');
  assert.equal(snapshot.target.version, EXPECTED_BUILD.version, 'Unexpected runtime build version');
  assert.equal(snapshot.target.buildKey, EXPECTED_BUILD.buildKey, 'Unexpected runtime Build Key');
  assert.equal(
    snapshot.target.buildInfoSha256,
    EXPECTED_BUILD.buildInfoSha256,
    'Unexpected installed .build.info hash',
  );
  assert.equal(
    snapshot.target.retailExeSha256,
    EXPECTED_BUILD.retailExeSha256,
    'Unexpected installed D2R.exe hash',
  );
  const layout = snapshot.nativeLayout;
  assert.equal(
    parseHexInteger(layout.playerSequenceTableRva, 'player sequence table RVA'),
    NATIVE_LAYOUT.playerSequenceTableRva,
  );
  assert.equal(
    parseHexInteger(layout.weaponClassMapRva, 'weapon class map RVA'),
    NATIVE_LAYOUT.weaponClassMapRva,
  );
  assert.equal(layout.slotCount, NATIVE_LAYOUT.slotCount, 'Unexpected player sequence slot count');
  assert.equal(
    layout.weaponClassCount,
    NATIVE_LAYOUT.weaponClassCount,
    'Unexpected player sequence weapon-class count',
  );
  assert.equal(layout.descriptorSize, NATIVE_LAYOUT.descriptorSize, 'Unexpected descriptor size');
  assert.equal(layout.recordSize, NATIVE_LAYOUT.recordSize, 'Unexpected sequence record size');
  assert.equal(layout.weaponClassMap.length, WEAPON_CLASSES.length, 'Incomplete weapon class map');
  const weaponClassMap = layout.weaponClassMap.map((entry, index) => {
    assert.equal(entry.weaponClassIndex, index, `Unexpected weapon-class index at ${index}`);
    assert.equal(
      entry.weaponClassId,
      EXPECTED_WEAPON_CLASS_IDS[index],
      `Unexpected weapon-class id at ${index}`,
    );
    return {
      weaponClassIndex: index,
      weaponClass: WEAPON_CLASSES[index],
      weaponClassId: entry.weaponClassId,
    };
  });

  const records = snapshot.records.map((record) => {
    const rva = parseHexInteger(record.recordsRva, 'runtime records RVA');
    const bytes = Buffer.from(record.bytes, 'hex');
    assert.equal(bytes.length, record.byteLength, `Runtime record byte length drift at ${hex(rva)}`);
    assert.equal(
      bytes.length,
      record.recordCount * NATIVE_LAYOUT.recordSize,
      `Runtime record count drift at ${hex(rva)}`,
    );
    assert.equal(sha256(bytes), record.sha256, `Runtime record hash drift at ${hex(rva)}`);
    const decoded = decodeSequenceRecords(bytes, record.recordCount);
    decoded.forEach((entry, index) => {
      assert.equal(entry.sequence, 0, `Nonzero player sequence field at ${hex(rva + index * 6)}`);
      assert(entry.modeId < PLAYER_MODE_KEYS.length, `Invalid player mode at ${hex(rva + index * 6)}`);
      assert(entry.eventId < EVENT_NAMES.length, `Invalid sequence event at ${hex(rva + index * 6)}`);
    });
    return {
      recordsRva: hex(rva),
      rva,
      recordCount: record.recordCount,
      byteLength: bytes.length,
      sha256: record.sha256,
      bytes,
      records: decoded,
    };
  }).sort((a, b) => a.rva - b.rva);
  const recordsByRva = new Map(records.map((record) => [record.rva, record]));
  assert.equal(recordsByRva.size, records.length, 'Duplicate runtime record RVA');

  assert.equal(snapshot.groups.length, NATIVE_LAYOUT.slotCount - 1, 'Incomplete runtime groups');
  const groups = snapshot.groups.map((group, groupIndex) => {
    const sequenceId = groupIndex + 1;
    assert.equal(group.sequenceId, sequenceId, `Unexpected runtime group order at ${sequenceId}`);
    const rva = parseHexInteger(group.groupRva, 'runtime group RVA');
    assert.equal(group.slots.length, WEAPON_CLASSES.length, `Incomplete runtime group ${sequenceId}`);
    const slots = group.slots.map((slot, weaponIndex) => {
      assert.equal(slot.weaponClassIndex, weaponIndex, `Unexpected weapon slot in group ${sequenceId}`);
      const descriptorRva = parseHexInteger(slot.descriptorRva, 'runtime descriptor RVA');
      assert.equal(
        descriptorRva,
        rva + weaponIndex * NATIVE_LAYOUT.descriptorSize,
        `Runtime descriptor topology drift in group ${sequenceId}`,
      );
      const recordsRva = slot.recordsRva === null
        ? null
        : parseHexInteger(slot.recordsRva, 'runtime slot records RVA');
      const record = recordsRva === null ? null : recordsByRva.get(recordsRva);
      if (recordsRva === null) {
        assert.equal(slot.sequenceFrameCount, 0, `Null runtime slot has sequence frames`);
        assert.equal(slot.animationFrameCount, 0, `Null runtime slot has animation frames`);
        assert.equal(slot.recordsSha256, null, `Null runtime slot has a record hash`);
      } else {
        assert(record, `Runtime slot points outside captured records: ${hex(recordsRva)}`);
        assert.equal(
          record.recordCount,
          slot.sequenceFrameCount,
          `Runtime sequence-frame count drift at ${hex(descriptorRva)}`,
        );
        assert.equal(record.sha256, slot.recordsSha256, `Runtime record hash drift at ${hex(descriptorRva)}`);
      }
      return {
        weaponClassIndex: weaponIndex,
        descriptorRva,
        recordsRva,
        sequenceFrameCount: slot.sequenceFrameCount,
        animationFrameCount: slot.animationFrameCount,
        extra: parseHexInteger(slot.extra, 'descriptor extra field'),
        record,
      };
    });
    return {
      sequenceId,
      groupRva: rva,
      descriptorsSha256: group.descriptorsSha256,
      slots,
    };
  });

  const normalized = {
    schemaVersion: 1,
    target: {
      product: 'Diablo II: Resurrected',
      version: snapshot.target.version,
      buildKey: snapshot.target.buildKey,
      buildInfoSha256: snapshot.target.buildInfoSha256,
      retailExeSha256: snapshot.target.retailExeSha256,
    },
    nativeLayout: {
      playerSequenceTableRva: hex(NATIVE_LAYOUT.playerSequenceTableRva),
      weaponClassMapRva: hex(NATIVE_LAYOUT.weaponClassMapRva),
      slotCount: NATIVE_LAYOUT.slotCount,
      weaponClassCount: NATIVE_LAYOUT.weaponClassCount,
      descriptorSize: NATIVE_LAYOUT.descriptorSize,
      recordSize: NATIVE_LAYOUT.recordSize,
      weaponClassMap: weaponClassMap.map((entry) => ({
        weaponClassIndex: entry.weaponClassIndex,
        weaponClassId: entry.weaponClassId,
      })),
    },
    nativeEvidence: {
      sequenceNumberResolverRva: hex(NATIVE_LAYOUT.sequenceNumberResolverRva),
      playerSequenceResolverRva: hex(NATIVE_LAYOUT.playerSequenceResolverRva),
      staticWeaponClassMapRva: hex(NATIVE_LAYOUT.staticWeaponClassMapRva),
    },
    groups: groups.map((group) => ({
      sequenceId: group.sequenceId,
      groupRva: hex(group.groupRva),
      descriptorsSha256: group.descriptorsSha256,
      slots: group.slots.map((slot) => ({
        weaponClassIndex: slot.weaponClassIndex,
        descriptorRva: hex(slot.descriptorRva),
        recordsRva: slot.recordsRva === null ? null : hex(slot.recordsRva),
        sequenceFrameCount: slot.sequenceFrameCount,
        animationFrameCount: slot.animationFrameCount,
        extra: hex(slot.extra),
        recordsSha256: slot.record?.sha256 ?? null,
      })),
    })),
    records: records.map((record) => ({
      recordsRva: hex(record.rva),
      recordCount: record.recordCount,
      byteLength: record.byteLength,
      sha256: record.sha256,
      bytes: record.bytes.toString('hex').toUpperCase(),
    })),
  };
  return { normalized, groups, records, recordsByRva, weaponClassMap };
}

function validateStaticRuntimeGroup(image, pe, fileOffset, group) {
  const groupSize = WEAPON_CLASSES.length * NATIVE_LAYOUT.descriptorSize;
  if (fileOffset < 0 || fileOffset + groupSize > image.length) return false;
  for (let weaponIndex = 0; weaponIndex < WEAPON_CLASSES.length; weaponIndex += 1) {
    const slot = group.slots[weaponIndex];
    const descriptorOffset = fileOffset + weaponIndex * NATIVE_LAYOUT.descriptorSize;
    const pointer = image.readBigUInt64LE(descriptorOffset);
    const sequenceFrames = image.readUInt32LE(descriptorOffset + 8);
    const animationFrames = image.readUInt32LE(descriptorOffset + 12);
    const extra = image.readBigUInt64LE(descriptorOffset + 16);
    if (
      sequenceFrames !== slot.sequenceFrameCount
      || animationFrames !== slot.animationFrameCount
      || extra !== BigInt(slot.extra)
    ) return false;
    if (slot.recordsRva === null) {
      if (pointer !== 0n) return false;
      continue;
    }
    if (pointer < pe.imageBase) return false;
    const staticRecordRva = Number(pointer - pe.imageBase);
    let staticRecordOffset;
    try {
      staticRecordOffset = rvaToFileOffset(pe, staticRecordRva);
    } catch {
      return false;
    }
    if (!image.subarray(
      staticRecordOffset,
      staticRecordOffset + slot.record.bytes.length,
    ).equals(slot.record.bytes)) return false;
  }
  return true;
}

function locateStaticRuntimeSeeds(image, pe, runtime) {
  const recordSeedRvas = new Map(runtime.records.map((record) => [record.rva, new Set()]));
  const groupSeeds = runtime.groups.map((group) => {
    const firstSlotIndex = group.slots.findIndex((slot) => slot.record !== null);
    assert.notEqual(firstSlotIndex, -1, `Empty runtime player sequence group ${group.sequenceId}`);
    const firstSlot = group.slots[firstSlotIndex];
    const possibleRecordMatches = findAll(image, firstSlot.record.bytes).map((fileOffset) => ({
      fileOffset,
      ...fileOffsetToRva(pe, fileOffset),
    }));
    const possibleGroupOffsets = new Set();
    for (const recordMatch of possibleRecordMatches) {
      const prefix = Buffer.alloc(NATIVE_LAYOUT.descriptorSize);
      absolutePointer(pe.imageBase, recordMatch.rva).copy(prefix, 0);
      prefix.writeUInt32LE(firstSlot.sequenceFrameCount, 8);
      prefix.writeUInt32LE(firstSlot.animationFrameCount, 12);
      prefix.writeBigUInt64LE(BigInt(firstSlot.extra), 16);
      for (const descriptorOffset of findAll(image, prefix)) {
        possibleGroupOffsets.add(
          descriptorOffset - firstSlotIndex * NATIVE_LAYOUT.descriptorSize,
        );
      }
    }
    const matches = [...possibleGroupOffsets]
      .filter((fileOffset) => validateStaticRuntimeGroup(image, pe, fileOffset, group))
      .map((fileOffset) => ({ fileOffset, ...fileOffsetToRva(pe, fileOffset) }))
      .sort((a, b) => a.rva - b.rva);
    assert(matches.length >= 1, `No static seed group for player sequence ${group.sequenceId}`);

    for (const match of matches) {
      for (let weaponIndex = 0; weaponIndex < WEAPON_CLASSES.length; weaponIndex += 1) {
        const slot = group.slots[weaponIndex];
        if (slot.recordsRva === null) continue;
        const descriptorOffset = match.fileOffset + weaponIndex * NATIVE_LAYOUT.descriptorSize;
        const staticRecordRva = Number(image.readBigUInt64LE(descriptorOffset) - pe.imageBase);
        recordSeedRvas.get(slot.recordsRva).add(staticRecordRva);
      }
    }
    return { sequenceId: group.sequenceId, matches };
  });
  for (const record of runtime.records) {
    assert(
      recordSeedRvas.get(record.rva).size >= 1,
      `No static record seed for runtime array ${hex(record.rva)}`,
    );
  }
  return {
    groupSeeds,
    groupSeedsBySequence: new Map(groupSeeds.map((entry) => [entry.sequenceId, entry.matches])),
    recordSeedRvas,
  };
}

function validateLegacyRuntimeParity(runtime, legacyArrays, legacyGroups, legacySequenceTable) {
  const arraysByName = new Map(legacyArrays.map((array) => [array.name, array]));
  const groupsByName = new Map(legacyGroups.map((group) => [group.name, group]));
  const namesByRuntimeRva = new Map();
  const referencedArrays = new Set();
  for (let sequenceId = 1; sequenceId < legacySequenceTable.length; sequenceId += 1) {
    const expectedGroupName = legacySequenceTable[sequenceId];
    const expectedGroup = groupsByName.get(expectedGroupName);
    const runtimeGroup = runtime.groups[sequenceId - 1];
    assert(expectedGroup, `Missing D2MOO group for legacy sequence ${sequenceId}`);
    for (let weaponIndex = 0; weaponIndex < WEAPON_CLASSES.length; weaponIndex += 1) {
      const expectedSlot = expectedGroup.slots[weaponIndex];
      const runtimeSlot = runtimeGroup.slots[weaponIndex];
      if (!expectedSlot.arrayName) {
        assert.equal(runtimeSlot.recordsRva, null, `Legacy null slot drift in sequence ${sequenceId}`);
        continue;
      }
      assert(runtimeSlot.record, `Legacy populated slot vanished in sequence ${sequenceId}`);
      assert.equal(
        runtimeSlot.sequenceFrameCount,
        expectedSlot.sequenceFrames,
        `Legacy sequence-frame drift in sequence ${sequenceId}`,
      );
      assert.equal(
        runtimeSlot.animationFrameCount,
        expectedSlot.animationFrames,
        `Legacy animation-frame drift in sequence ${sequenceId}`,
      );
      const legacyArray = arraysByName.get(expectedSlot.arrayName);
      assert(
        runtimeSlot.record.bytes.equals(encodeSequenceRecords(legacyArray.records)),
        `Legacy record-byte drift for ${expectedSlot.arrayName}`,
      );
      referencedArrays.add(expectedSlot.arrayName);
      if (!namesByRuntimeRva.has(runtimeSlot.recordsRva)) {
        namesByRuntimeRva.set(runtimeSlot.recordsRva, expectedSlot.arrayName);
      }
    }
  }
  assert.equal(referencedArrays.size, legacyArrays.length, 'Not every legacy semantic array is routed');
  return namesByRuntimeRva;
}

function validateReferencePin() {
  const manifest = readJson(path.join(REPOSITORY_ROOT, 'reverse-engineering', 'references.json'));
  const reference = manifest.references.find((candidate) => candidate.id === 'd2moo');
  assert(reference, 'Missing governed D2MOO reference');
  const status = spawnSync('git', ['-C', D2MOO_ROOT, 'rev-parse', 'HEAD'], {
    encoding: 'utf8',
    windowsHide: true,
  });
  assert.equal(status.status, 0, `Unable to inspect D2MOO pin: ${status.stderr}`);
  const actualCommit = status.stdout.trim();
  assert.equal(actualCommit, reference.commit, 'D2MOO checkout does not match the governed pin');
  return { ...reference, actualCommit };
}

function currentPlayerSkillMap(table) {
  const rows = rowsAsObjects(table)
    .filter((row) => row.charclass && Number(row.seqnum) > 0)
    .map((row) => ({
      skill: row.skill,
      charclass: row.charclass,
      anim: row.anim,
      seqnum: Number(row.seqnum),
      seqinput: row.seqinput,
    }));
  const bySequence = new Map();
  for (const row of rows) {
    if (!bySequence.has(row.seqnum)) bySequence.set(row.seqnum, []);
    bySequence.get(row.seqnum).push(row);
  }
  return { rows, bySequence, maxSequenceId: Math.max(...rows.map((row) => row.seqnum)) };
}

function stableArrayName(sequenceId, sequenceName, weaponClass, recordRva, aliasesByRva) {
  const existing = aliasesByRva.get(recordRva);
  if (existing) return existing;
  const normalized = sequenceName.replace(/[^A-Za-z0-9]+/g, '');
  const name = `D2R_PlayerSequence${normalized}_${weaponClass}`;
  aliasesByRva.set(recordRva, name);
  return name;
}

function makeTable(headers, rows) {
  return {
    headers,
    rows: rows.map((row) => headers.map((header) => String(row[header] ?? ''))),
    eol: '\r\n',
    hasFinalEol: true,
  };
}

function verifyWrittenTable(filePath) {
  const raw = fs.readFileSync(filePath, ENCODING);
  const table = parseTable(filePath);
  assert.equal(table.eol, '\r\n', `Generated TSV is not CRLF: ${filePath}`);
  assert.equal(serializeTable(table), raw, `Generated TSV round-trip drift: ${filePath}`);
  return { sha256: sha256(Buffer.from(raw, ENCODING)), bytes: Buffer.byteLength(raw, ENCODING) };
}

export function extractPlayerSequences({
  write = false,
  outputRoot = DEFAULT_OUTPUT_ROOT,
  runtimeSnapshotPath,
} = {}) {
  const workbench = readJson(path.join(WORKBENCH_ROOT, 'workbench.json'));
  const imagePath = path.join(WORKBENCH_ROOT, workbench.analysisImage.relativePath);
  const image = fs.readFileSync(imagePath);
  assert.equal(image.length, workbench.analysisImage.size, 'Governed analysis image size mismatch');
  assert.equal(sha256(image), workbench.analysisImage.sha256, 'Governed analysis image hash mismatch');
  const pe = parsePe(image);
  assert.equal(hex(pe.imageBase), workbench.target.imageBase, 'PE image base mismatch');

  const reference = validateReferencePin();
  const source = fs.readFileSync(D2MOO_SEQUENCE_SOURCE, 'utf8');
  const header = fs.readFileSync(D2MOO_SEQUENCE_HEADER, 'utf8');
  for (const event of EVENT_NAMES) {
    assert(header.includes(`ANIMSEQ_EVENT_${event}`), `Missing D2MOO event ${event}`);
  }
  const playerModeSource = readGovernedTable(PLRMODE_PATH);
  const skillsSource = readGovernedTable(SKILLS_PATH);
  const playerModes = parsePlayerModes(playerModeSource.table);
  const legacyArrays = parseD2MooArrays(source, playerModes);
  const legacyGroups = parseD2MooGroups(source, legacyArrays);
  const legacySequenceTable = parseD2MooSequenceTable(source, legacyGroups);
  const selectedRuntimeSource = runtimeSnapshotPath
    ?? (fs.existsSync(DEFAULT_RUNTIME_LAYOUT_PATH)
      ? DEFAULT_RUNTIME_LAYOUT_PATH
      : DEFAULT_RUNTIME_CAPTURE_PATH);
  assert(
    fs.existsSync(selectedRuntimeSource),
    `Missing runtime capture/layout: ${selectedRuntimeSource}. Run Capture-PlayerSequences.ps1 first.`,
  );
  const runtime = normalizeRuntimeSnapshot(readJson(selectedRuntimeSource));
  const nativeEvidence = verifyNativeEvidence(image, pe, runtime.weaponClassMap);
  const staticSeeds = locateStaticRuntimeSeeds(image, pe, runtime);
  const legacyNamesByRuntimeRva = validateLegacyRuntimeParity(
    runtime,
    legacyArrays,
    legacyGroups,
    legacySequenceTable,
  );
  const skillMap = currentPlayerSkillMap(skillsSource.table);
  assert.equal(
    skillMap.maxSequenceId,
    NATIVE_LAYOUT.slotCount - 1,
    'Current Skills.txt and the runtime player-sequence slot count disagree',
  );

  const sequenceNames = new Map();
  legacySequenceTable.forEach((groupName, sequenceId) => {
    if (groupName) sequenceNames.set(sequenceId, groupName.replace('gPlayerWeaponsSequence', ''));
  });
  for (const [sequenceId, rows] of skillMap.bySequence) {
    if (!sequenceNames.has(sequenceId)) {
      sequenceNames.set(sequenceId, rows[0].skill.replace(/\s+/g, ''));
    }
  }

  const aliasNamesByRva = new Map(
    legacyNamesByRuntimeRva,
  );
  const nativeArraysByRva = new Map();
  const mappingRows = [];
  for (let sequenceId = 1; sequenceId <= skillMap.maxSequenceId; sequenceId += 1) {
    const group = runtime.groups[sequenceId - 1];
    const staticGroupSeeds = staticSeeds.groupSeedsBySequence.get(sequenceId);
    const sequenceName = sequenceNames.get(sequenceId) ?? `Sequence${sequenceId}`;
    const skillRows = skillMap.bySequence.get(sequenceId) ?? [];
    const skillNames = skillRows.map((row) => `${row.charclass}:${row.skill}`).join('|');
    for (let weaponIndex = 0; weaponIndex < WEAPON_CLASSES.length; weaponIndex += 1) {
      const descriptor = group.slots[weaponIndex];
      let arrayName = '';
      if (descriptor.recordsRva !== null) {
        arrayName = legacyNamesByRuntimeRva.get(descriptor.recordsRva) ?? stableArrayName(
          sequenceId,
          sequenceName,
          WEAPON_CLASSES[weaponIndex],
          descriptor.recordsRva,
          aliasNamesByRva,
        );
        if (!nativeArraysByRva.has(descriptor.recordsRva)) {
          nativeArraysByRva.set(descriptor.recordsRva, {
            name: arrayName,
            ...descriptor.record,
          });
        }
      }
      const staticRecordSeedRvas = descriptor.recordsRva === null
        ? []
        : [...staticSeeds.recordSeedRvas.get(descriptor.recordsRva)].sort((a, b) => a - b);
      mappingRows.push({
        sequence_id: sequenceId,
        sequence_name: sequenceName,
        skill_rows: skillNames,
        weapon_class_index: weaponIndex,
        weapon_class: WEAPON_CLASSES[weaponIndex],
        weapon_class_id: runtime.weaponClassMap[weaponIndex].weaponClassId,
        available: descriptor.recordsRva === null ? 0 : 1,
        array_name: arrayName,
        runtime_group_rva: hex(group.groupRva),
        runtime_descriptor_rva: hex(descriptor.descriptorRva),
        runtime_records_rva: descriptor.recordsRva === null ? '' : hex(descriptor.recordsRva),
        static_group_seed_rvas: staticGroupSeeds.map((match) => hex(match.rva)).join('|'),
        static_records_seed_rvas: staticRecordSeedRvas.map(hex).join('|'),
        sequence_frame_count: descriptor.sequenceFrameCount,
        animation_frame_count: descriptor.animationFrameCount,
        descriptor_extra: hex(descriptor.extra),
      });
    }
  }
  assert.equal(
    nativeArraysByRva.size,
    runtime.records.length,
    'Captured runtime record arrays are not all routed by the player sequence table',
  );

  const recordRows = [];
  for (const array of [...nativeArraysByRva.values()].sort((a, b) => a.rva - b.rva)) {
    array.records.forEach((record, index) => {
      const mode = playerModes[record.modeId];
      assert(mode, `Unknown native player mode ${record.modeId} at ${hex(array.rva)}`);
      const eventName = EVENT_NAMES[record.eventId];
      assert(eventName, `Unknown native sequence event ${record.eventId} at ${hex(array.rva)}`);
      const recordBytes = array.bytes.subarray(index * 6, (index + 1) * 6);
      recordRows.push({
        array_name: array.name,
        runtime_records_rva: hex(array.rva),
        static_records_seed_rvas: [...staticSeeds.recordSeedRvas.get(array.rva)]
          .sort((a, b) => a - b)
          .map(hex)
          .join('|'),
        record_index: index,
        runtime_record_rva: hex(array.rva + index * 6),
        mode_id: record.modeId,
        mode_code: mode.code,
        mode_name: mode.name,
        frame: record.frame,
        direction: record.direction,
        event_id: record.eventId,
        event_name: eventName,
        native_bytes: recordBytes.toString('hex').toUpperCase(),
      });
    });
  }

  const recordHeaders = [
    'array_name',
    'runtime_records_rva',
    'static_records_seed_rvas',
    'record_index',
    'runtime_record_rva',
    'mode_id',
    'mode_code',
    'mode_name',
    'frame',
    'direction',
    'event_id',
    'event_name',
    'native_bytes',
  ];
  const mappingHeaders = [
    'sequence_id',
    'sequence_name',
    'skill_rows',
    'weapon_class_index',
    'weapon_class',
    'weapon_class_id',
    'available',
    'array_name',
    'runtime_group_rva',
    'runtime_descriptor_rva',
    'runtime_records_rva',
    'static_group_seed_rvas',
    'static_records_seed_rvas',
    'sequence_frame_count',
    'animation_frame_count',
    'descriptor_extra',
  ];
  const recordsTable = makeTable(recordHeaders, recordRows);
  const mappingTable = makeTable(mappingHeaders, mappingRows);

  const runtimeLayoutText = `${JSON.stringify(runtime.normalized, null, 2)}\n`;
  const runtimeLayoutProof = {
    sha256: sha256(Buffer.from(runtimeLayoutText, 'utf8')),
    bytes: Buffer.byteLength(runtimeLayoutText, 'utf8'),
  };
  const descriptorExtras = [...new Set(mappingRows.map((row) => row.descriptor_extra))].sort();
  assert.deepEqual(descriptorExtras, ['0x100'], 'Unexpected player sequence descriptor extra field');
  const ambiguousStaticGroupSeeds = staticSeeds.groupSeeds
    .filter((entry) => entry.matches.length > 1)
    .map((entry) => ({
      sequenceId: entry.sequenceId,
      rvas: entry.matches.map((match) => hex(match.rva)),
    }));

  const files = {
    runtime: path.join(outputRoot, 'd2r-3.3.93847-player-sequence-runtime.json'),
    records: path.join(outputRoot, 'd2r-3.3.93847-player-sequence-records.tsv'),
    mapping: path.join(outputRoot, 'd2r-3.3.93847-player-sequence-map.tsv'),
    manifest: path.join(outputRoot, 'd2r-3.3.93847-player-sequences.manifest.json'),
  };
  let outputProof = null;
  if (write) {
    fs.mkdirSync(outputRoot, { recursive: true });
    fs.writeFileSync(files.runtime, runtimeLayoutText, 'utf8');
    writeTable(files.records, recordsTable);
    writeTable(files.mapping, mappingTable);
    outputProof = {
      runtime: {
        sha256: sha256(fs.readFileSync(files.runtime)),
        bytes: fs.statSync(files.runtime).size,
      },
      records: verifyWrittenTable(files.records),
      mapping: verifyWrittenTable(files.mapping),
    };
  } else {
    outputProof = {
      runtime: runtimeLayoutProof,
      records: {
        sha256: sha256(Buffer.from(serializeTable(recordsTable), ENCODING)),
        bytes: Buffer.byteLength(serializeTable(recordsTable), ENCODING),
      },
      mapping: {
        sha256: sha256(Buffer.from(serializeTable(mappingTable), ENCODING)),
        bytes: Buffer.byteLength(serializeTable(mappingTable), ENCODING),
      },
    };
  }

  const manifest = {
    schemaVersion: 1,
    target: {
      product: 'Diablo II: Resurrected',
      version: '3.3',
      build: 93847,
      nativeCorpusProvenance: 'D2R 3.2.92777',
      analysisImageSha256: workbench.analysisImage.sha256,
      imageBase: hex(pe.imageBase),
    },
    reference: {
      name: reference.name,
      commit: reference.actualCommit,
      role: reference.role,
      source: 'source/D2Common/src/DataTbls/SequenceTbls.cpp',
      policy: 'Semantic names and legacy topology only; all addresses and bytes are proved independently in D2R.',
    },
    sources: {
      plrmode: {
        path: path.relative(REPOSITORY_ROOT, PLRMODE_PATH).replaceAll('\\', '/'),
        sha256: sha256(Buffer.from(playerModeSource.raw, ENCODING)),
        roundTripByteExact: true,
        eol: 'CRLF',
      },
      skills: {
        path: path.relative(REPOSITORY_ROOT, SKILLS_PATH).replaceAll('\\', '/'),
        sha256: sha256(Buffer.from(skillsSource.raw, ENCODING)),
        roundTripByteExact: true,
        eol: 'CRLF',
      },
    },
    nativeLayout: {
      sequenceNumberResolverRva: hex(NATIVE_LAYOUT.sequenceNumberResolverRva),
      playerSequenceResolverRva: hex(NATIVE_LAYOUT.playerSequenceResolverRva),
      playerSequenceTableRva: hex(NATIVE_LAYOUT.playerSequenceTableRva),
      weaponClassMapRva: hex(NATIVE_LAYOUT.weaponClassMapRva),
      staticWeaponClassMapRva: hex(nativeEvidence.staticWeaponClassMap.rva),
      recordSize: NATIVE_LAYOUT.recordSize,
      descriptorSize: NATIVE_LAYOUT.descriptorSize,
      weaponClassCount: WEAPON_CLASSES.length,
      playerSequenceSlotCount: NATIVE_LAYOUT.slotCount,
      activePlayerSequenceCount: skillMap.maxSequenceId,
      descriptorExtraValues: descriptorExtras,
      ambiguousStaticGroupSeeds,
    },
    counts: {
      legacySemanticArrays: legacyArrays.length,
      legacySemanticGroups: legacyGroups.length,
      nativeUniqueArrays: nativeArraysByRva.size,
      nativeUniqueArrayContents: new Set(runtime.records.map((record) => record.sha256)).size,
      nativeRecordRows: recordRows.length,
      mappingRows: mappingRows.length,
      availableMappings: mappingRows.filter((row) => row.available).length,
      unavailableMappings: mappingRows.filter((row) => !row.available).length,
      currentPlayerSkillRows: skillMap.rows.length,
    },
    proofs: {
      governedImageHashVerified: true,
      governedD2MooPinVerified: true,
      installedBuildIdentityVerified: true,
      runtimeLayoutValidated: true,
      selectorAndRoutingWitnessesExact: true,
      everyRuntimeGroupHasStaticSeeds: true,
      everyRuntimeRecordHasStaticSeeds: true,
      legacySemanticRoutingAndBytesExact: true,
      generatedTablesRoundTripByteExact: true,
    },
    outputs: {
      runtime: {
        path: path.relative(REPOSITORY_ROOT, files.runtime).replaceAll('\\', '/'),
        ...outputProof.runtime,
      },
      records: {
        path: path.relative(REPOSITORY_ROOT, files.records).replaceAll('\\', '/'),
        ...outputProof.records,
      },
      mapping: {
        path: path.relative(REPOSITORY_ROOT, files.mapping).replaceAll('\\', '/'),
        ...outputProof.mapping,
      },
    },
    unresolved: [
      'The semantic meaning of the 64-bit descriptor field after the two frame counts remains unproved.',
      'The complete client/server consumer graph beyond the authoritative resolver remains to be inventoried.',
      'Variable-length replacement safety and ownership are not proved by this documentary extraction.',
      'No runtime loading or gameplay modification is implemented in this phase.',
    ],
  };

  if (write) {
    fs.writeFileSync(files.manifest, `${JSON.stringify(manifest, null, 2)}\n`, 'utf8');
  }
  return { manifest, runtimeLayout: runtime.normalized, recordsTable, mappingTable, files };
}

function main() {
  const args = new Set(process.argv.slice(2));
  const write = args.has('--write');
  const result = extractPlayerSequences({ write });
  process.stdout.write(`${JSON.stringify(result.manifest, null, 2)}\n`);
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  main();
}
