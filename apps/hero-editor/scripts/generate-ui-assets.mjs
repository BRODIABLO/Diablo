import {
  mkdir,
  readFile,
  readdir,
  writeFile,
} from 'node:fs/promises';
import { existsSync } from 'node:fs';
import { dirname, extname, relative, resolve, sep } from 'node:path';
import { fileURLToPath } from 'node:url';
import { deflateSync } from 'node:zlib';

import { itemCatalog } from '../src/data/bkvince-constants.generated.js';

const SCRIPT_DIR = dirname(fileURLToPath(import.meta.url));
const WORKSPACE_ROOT = resolve(SCRIPT_DIR, '../../..');
const APP_ROOT = resolve(SCRIPT_DIR, '..');
const BKVINCE_HD = resolve(WORKSPACE_ROOT, 'data-BKVince/BKVince.mpq/data/hd');
const ITEM_REGISTRY = resolve(BKVINCE_HD, 'items/items.json');
const ITEM_SPRITE_ROOT = resolve(BKVINCE_HD, 'global/ui/items');
const VANILLA_ITEM_SPRITE_ROOT = resolve(
  WORKSPACE_ROOT,
  'analysis-cache/hero-editor-vanilla-ui/hd/global/ui/items',
);
const VANILLA_LEGACY_ITEM_ROOT = resolve(
  WORKSPACE_ROOT,
  'analysis-cache/hero-editor-vanilla-ui/global/items',
);
const VANILLA_LEGACY_PALETTE = resolve(
  WORKSPACE_ROOT,
  'analysis-cache/hero-editor-vanilla-ui/global/palette/units/pal.dat',
);
const VANILLA_SKILL_SPRITE_ROOT = resolve(
  WORKSPACE_ROOT,
  'analysis-cache/hero-editor-vanilla-ui/hd/global/ui/spells',
);
const VANILLA_SKILL_TREE_SPRITE_ROOT = resolve(
  VANILLA_SKILL_SPRITE_ROOT,
  'skill_trees',
);
const VANILLA_PORTRAIT_SPRITE_ROOT = resolve(
  WORKSPACE_ROOT,
  'analysis-cache/hero-editor-vanilla-ui/hd/global/ui/hireables',
);
const EQUIPMENT_SOURCE = resolve(BKVINCE_HD, 'global/ui/panel/inventory/classic_background.sprite');
const MERCENARY_EQUIPMENT_SOURCE = resolve(BKVINCE_HD, 'global/ui/panel/hireling/hirelingpanel.sprite');
const WARLOCK_SKILL_SOURCE = resolve(BKVINCE_HD, 'global/ui/spells/submenu/skillicon.sprite');
const PUBLIC_UI_ROOT = resolve(APP_ROOT, 'public/ui');
const ITEM_OUTPUT_ROOT = resolve(PUBLIC_UI_ROOT, 'items');
const SKILL_OUTPUT_ROOT = resolve(PUBLIC_UI_ROOT, 'skills');
const SKILL_TREE_OUTPUT_ROOT = resolve(PUBLIC_UI_ROOT, 'skill-trees');
const PORTRAIT_OUTPUT_ROOT = resolve(PUBLIC_UI_ROOT, 'portraits');
const GENERATED_MANIFEST = resolve(APP_ROOT, 'src/data/item-visuals.generated.js');
const GENERATED_SKILL_MANIFEST = resolve(APP_ROOT, 'src/data/skill-visuals.generated.js');
const SPRITE_HEADER_SIZE = 40;
const SKILL_FRAME_COUNT = 60;
const SKILL_TREE_FRAME_COUNT = 3;
const SKILL_TREE_OUTPUT_SIZE = Object.freeze({ width: 895, height: 1169 });
const SKILL_SOURCES = Object.freeze({
  ama: 'amazon/amskillicon.sprite',
  sor: 'sorceress/soskillicon.sprite',
  nec: 'necromancer/neskillicon.sprite',
  pal: 'paladin/paskillicon.sprite',
  bar: 'barbarian/baskillicon.sprite',
  dru: 'druid/drskillicon.sprite',
  ass: 'assassin/asskillicon.sprite',
  war: null,
});
const SKILL_TREE_SOURCES = Object.freeze({
  ama: 'amskilltree.sprite',
  sor: 'soskilltree.sprite',
  nec: 'neskilltree.sprite',
  pal: 'paskilltree.sprite',
  bar: 'baskilltree.sprite',
  dru: 'drskilltree.sprite',
  ass: 'asskilltree.sprite',
  war: 'waskilltree.sprite',
});
const CLASS_PORTRAIT_SOURCES = Object.freeze({
  amazon: 'amazonicon.sprite',
  sorceress: 'sorceressicon.sprite',
  necromancer: 'necromancericon.sprite',
  paladin: 'paladinicon.sprite',
  barbarian: 'barbarianicon.sprite',
  druid: 'druidicon.sprite',
  assassin: 'assassinicon.sprite',
  warlock: null,
});
const WARLOCK_PORTRAIT_SKILL_FRAME = 20;
const PORTRAIT_SIZE = 120;
const ITEM_VISUAL_ALIASES = Object.freeze({
  mfg: 'cm1', // Splash Charm uses the governed invchm art.
  rds: 'tsc', // Clue Scroll Test uses the governed invbsc art.
});
const EQUIPMENT_SLOT_PLACEHOLDER_CODES = Object.freeze({
  1: 'cap',
  2: 'amu',
  3: 'qui',
  4: 'ssd',
  5: 'ssd',
  6: 'rin',
  7: 'rin',
  8: 'lbl',
  9: 'lbt',
  10: 'lgl',
  11: 'ssd',
  12: 'ssd',
});

const CRC_TABLE = new Uint32Array(256);
for (let index = 0; index < CRC_TABLE.length; index += 1) {
  let value = index;
  for (let bit = 0; bit < 8; bit += 1) {
    value = (value & 1) ? (0xedb88320 ^ (value >>> 1)) : (value >>> 1);
  }
  CRC_TABLE[index] = value >>> 0;
}

function crc32(bytes) {
  let crc = 0xffffffff;
  for (const byte of bytes) crc = CRC_TABLE[(crc ^ byte) & 0xff] ^ (crc >>> 8);
  return (crc ^ 0xffffffff) >>> 0;
}

function pngChunk(type, payload) {
  const typeBytes = Buffer.from(type, 'ascii');
  const output = Buffer.allocUnsafe(12 + payload.length);
  output.writeUInt32BE(payload.length, 0);
  typeBytes.copy(output, 4);
  payload.copy(output, 8);
  output.writeUInt32BE(crc32(Buffer.concat([typeBytes, payload])), 8 + payload.length);
  return output;
}

function encodePng(width, height, rgba) {
  const stride = width * 4;
  const scanlines = Buffer.allocUnsafe((stride + 1) * height);
  for (let y = 0; y < height; y += 1) {
    const destination = y * (stride + 1);
    scanlines[destination] = 0;
    rgba.copy(scanlines, destination + 1, y * stride, (y + 1) * stride);
  }
  const header = Buffer.alloc(13);
  header.writeUInt32BE(width, 0);
  header.writeUInt32BE(height, 4);
  header[8] = 8;
  header[9] = 6;
  return Buffer.concat([
    Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]),
    pngChunk('IHDR', header),
    pngChunk('IDAT', deflateSync(scanlines, { level: 9 })),
    pngChunk('IEND', Buffer.alloc(0)),
  ]);
}

function decodeRgbaSprite(source, sourcePath) {
  const magic = source.subarray(0, 4).toString('ascii');
  const version = source.readUInt16LE(4);
  const width = source.readInt32LE(8);
  const height = source.readInt32LE(12);
  if (!/^S[pP][aA]1$/.test(magic)) throw new Error(`${sourcePath}: unsupported sprite magic ${magic}.`);
  if (version !== 31) throw new Error(`${sourcePath}: expected RGBA sprite v31, got v${version}.`);
  const payloadSize = width * height * 4;
  if (width <= 0 || height <= 0 || source.length < SPRITE_HEADER_SIZE + payloadSize) {
    throw new Error(`${sourcePath}: truncated or invalid ${width}×${height} RGBA payload.`);
  }
  return {
    width,
    height,
    rgba: Buffer.from(source.subarray(SPRITE_HEADER_SIZE, SPRITE_HEADER_SIZE + payloadSize)),
  };
}

function decodeIndexedDc6(source, palette, sourcePath) {
  if (source.length < 60 || source.readUInt32LE(0) !== 6) {
    throw new Error(`${sourcePath}: unsupported or truncated DC6 file.`);
  }
  if (palette.length !== 256 * 3) {
    throw new Error(`${VANILLA_LEGACY_PALETTE}: expected a 768-byte units palette.`);
  }
  const directions = source.readUInt32LE(16);
  const framesPerDirection = source.readUInt32LE(20);
  if (directions !== 1 || framesPerDirection !== 1) {
    throw new Error(`${sourcePath}: expected one DC6 direction and one frame.`);
  }
  const frameOffset = source.readUInt32LE(24);
  if (frameOffset + 32 > source.length) throw new Error(`${sourcePath}: truncated DC6 frame header.`);
  const flip = source.readUInt32LE(frameOffset);
  const width = source.readUInt32LE(frameOffset + 4);
  const height = source.readUInt32LE(frameOffset + 8);
  const encodedLength = source.readUInt32LE(frameOffset + 28);
  const dataStart = frameOffset + 32;
  const dataEnd = dataStart + encodedLength;
  if (width <= 0 || height <= 0 || dataEnd > source.length) {
    throw new Error(`${sourcePath}: invalid ${width}×${height} DC6 payload.`);
  }

  const rgba = Buffer.alloc(width * height * 4);
  let cursor = dataStart;
  let x = 0;
  let y = flip ? 0 : height - 1;
  let completedRows = 0;
  while (cursor < dataEnd && completedRows < height) {
    const control = source[cursor];
    cursor += 1;
    if (control === 0x80) {
      x = 0;
      y += flip ? 1 : -1;
      completedRows += 1;
      continue;
    }
    if ((control & 0x80) !== 0) {
      x += control & 0x7f;
      if (x > width) throw new Error(`${sourcePath}: transparent DC6 run exceeds its frame.`);
      continue;
    }
    const runLength = control;
    if (cursor + runLength > dataEnd || x + runLength > width || y < 0 || y >= height) {
      throw new Error(`${sourcePath}: literal DC6 run exceeds its frame.`);
    }
    for (let pixel = 0; pixel < runLength; pixel += 1) {
      const paletteIndex = source[cursor + pixel];
      const sourceOffset = paletteIndex * 3;
      const outputOffset = ((y * width) + x + pixel) * 4;
      rgba[outputOffset] = palette[sourceOffset];
      rgba[outputOffset + 1] = palette[sourceOffset + 1];
      rgba[outputOffset + 2] = palette[sourceOffset + 2];
      rgba[outputOffset + 3] = 0xff;
    }
    cursor += runLength;
    x += runLength;
  }
  if (completedRows !== height) {
    throw new Error(`${sourcePath}: decoded ${completedRows} of ${height} DC6 rows.`);
  }
  return { width, height, rgba };
}

function cropRgba(image, crop) {
  const { x, y, width, height } = crop;
  if (x < 0 || y < 0 || x + width > image.width || y + height > image.height) {
    throw new Error(`Crop ${x},${y} ${width}×${height} exceeds ${image.width}×${image.height}.`);
  }
  const output = Buffer.allocUnsafe(width * height * 4);
  for (let row = 0; row < height; row += 1) {
    const sourceStart = ((y + row) * image.width + x) * 4;
    image.rgba.copy(output, row * width * 4, sourceStart, sourceStart + width * 4);
  }
  return { width, height, rgba: output };
}

function scaleRgbaNearest(image, width, height) {
  const output = Buffer.allocUnsafe(width * height * 4);
  for (let y = 0; y < height; y += 1) {
    const sourceY = Math.min(image.height - 1, Math.floor((y * image.height) / height));
    for (let x = 0; x < width; x += 1) {
      const sourceX = Math.min(image.width - 1, Math.floor((x * image.width) / width));
      const sourceOffset = (sourceY * image.width + sourceX) * 4;
      image.rgba.copy(output, (y * width + x) * 4, sourceOffset, sourceOffset + 4);
    }
  }
  return { width, height, rgba: output };
}

async function walk(directory) {
  const entries = await readdir(directory, { withFileTypes: true });
  const files = [];
  for (const entry of entries) {
    const entryPath = resolve(directory, entry.name);
    if (entry.isDirectory()) files.push(...await walk(entryPath));
    else files.push(entryPath);
  }
  return files;
}

function normalizedAssetPath(root, filePath) {
  return normalizedAssetKey(relative(root, filePath).split(sep).join('/').replace(/\.sprite$/i, ''));
}

function normalizedAssetKey(value) {
  return String(value || '')
    .replaceAll('\\', '/')
    .split('/')
    .filter((segment) => segment && segment !== '.' && segment !== '..')
    .join('/')
    .toLocaleLowerCase('en-US');
}

function safeOutputName(assetPath) {
  return `${assetPath.toLowerCase().replace(/[^a-z0-9_-]+/g, '__')}.png`;
}

function assertGeneratedPath(path) {
  const relativePath = relative(APP_ROOT, path);
  if (relativePath.startsWith('..') || relativePath === '') {
    throw new Error(`Refusing to mutate a generated path outside ${APP_ROOT}: ${path}`);
  }
}

async function writeSpritePng(sourcePath, outputPath) {
  const decoded = decodeRgbaSprite(await readFile(sourcePath), sourcePath);
  await mkdir(dirname(outputPath), { recursive: true });
  await writeFile(outputPath, encodePng(decoded.width, decoded.height, decoded.rgba));
  return decoded;
}

async function generateEquipmentPanel() {
  const decoded = decodeRgbaSprite(await readFile(EQUIPMENT_SOURCE), EQUIPMENT_SOURCE);
  const equipment = cropRgba(decoded, { x: 78, y: 138, width: 1112, height: 714 });
  const outputPath = resolve(PUBLIC_UI_ROOT, 'equipment-panel.png');
  await mkdir(dirname(outputPath), { recursive: true });
  await writeFile(outputPath, encodePng(equipment.width, equipment.height, equipment.rgba));
  return { width: equipment.width, height: equipment.height };
}

async function generateMercenaryEquipmentPanel() {
  const decoded = decodeRgbaSprite(
    await readFile(MERCENARY_EQUIPMENT_SOURCE),
    MERCENARY_EQUIPMENT_SOURCE,
  );
  const equipment = cropRgba(decoded, { x: 25, y: 95, width: 1112, height: 547 });
  const outputPath = resolve(PUBLIC_UI_ROOT, 'mercenary-equipment-panel.png');
  await mkdir(dirname(outputPath), { recursive: true });
  await writeFile(outputPath, encodePng(equipment.width, equipment.height, equipment.rgba));
  return { width: equipment.width, height: equipment.height };
}

async function generateItemVisuals() {
  const registry = JSON.parse(await readFile(ITEM_REGISTRY, 'utf8'));
  const codeToAsset = new Map(registry.flatMap((entry) => Object.entries(entry))
    .map(([code, value]) => [code, normalizedAssetKey(value.asset)]));
  const spriteFiles = (await walk(ITEM_SPRITE_ROOT))
    .filter((path) => extname(path).toLowerCase() === '.sprite' && !path.toLowerCase().endsWith('.lowend.sprite'));
  const vanillaSpriteFiles = existsSync(VANILLA_ITEM_SPRITE_ROOT)
    ? (await walk(VANILLA_ITEM_SPRITE_ROOT))
      .filter((path) => extname(path).toLowerCase() === '.sprite' && !path.toLowerCase().endsWith('.lowend.sprite'))
    : [];
  const candidateMap = (files, root) => {
    const candidates = new Map();
    for (const spritePath of files) {
      const normalized = normalizedAssetPath(root, spritePath);
      const withoutKind = normalized.split('/').slice(1).join('/');
      for (const key of [normalized, withoutKind]) {
        if (!candidates.has(key)) candidates.set(key, []);
        candidates.get(key).push(spritePath);
      }
    }
    return candidates;
  };
  const overlayCandidates = candidateMap(spriteFiles, ITEM_SPRITE_ROOT);
  const vanillaCandidates = candidateMap(vanillaSpriteFiles, VANILLA_ITEM_SPRITE_ROOT);

  assertGeneratedPath(ITEM_OUTPUT_ROOT);
  await mkdir(ITEM_OUTPUT_ROOT, { recursive: true });

  const manifest = {};
  const renderedAssets = new Map();
  for (const [code, asset] of [...codeToAsset.entries()].sort(([left], [right]) => left.localeCompare(right))) {
    const overlay = overlayCandidates.get(asset) || [];
    const vanilla = vanillaCandidates.get(asset) || [];
    const candidates = overlay.length === 1 ? overlay : vanilla;
    const outputName = renderedAssets.get(asset) || safeOutputName(asset);
    const outputPath = resolve(ITEM_OUTPUT_ROOT, outputName);
    if (candidates.length === 1) {
      if (!renderedAssets.has(asset)) await writeSpritePng(candidates[0], outputPath);
    } else if (!existsSync(outputPath)) {
      continue;
    }
    renderedAssets.set(asset, outputName);
    manifest[code] = `/ui/items/${outputName}`;
  }
  for (const [code, sourceCode] of Object.entries(ITEM_VISUAL_ALIASES)) {
    if (!manifest[sourceCode]) throw new Error(`Item visual alias ${code} references missing source ${sourceCode}.`);
    manifest[code] = manifest[sourceCode];
  }
  const palette = existsSync(VANILLA_LEGACY_PALETTE)
    ? await readFile(VANILLA_LEGACY_PALETTE)
    : null;
  const renderedLegacy = new Map();
  let legacyVariants = 0;
  for (const base of itemCatalog.bases) {
    for (const [pictureId, picture] of base.pictures.entries()) {
      const outputName = `legacy__${safeOutputName(picture)}`;
      const outputPath = resolve(ITEM_OUTPUT_ROOT, outputName);
      const sourcePath = resolve(VANILLA_LEGACY_ITEM_ROOT, `${picture}.dc6`);
      if (!renderedLegacy.has(picture)) {
        if (palette && existsSync(sourcePath)) {
          const decoded = decodeIndexedDc6(await readFile(sourcePath), palette, sourcePath);
          await writeFile(outputPath, encodePng(decoded.width, decoded.height, decoded.rgba));
        } else if (!existsSync(outputPath)) {
          throw new Error(`Missing governed legacy item visual ${picture}; run npm run extract:vanilla-ui -w apps/hero-editor.`);
        }
        renderedLegacy.set(picture, outputName);
      }
      manifest[`${base.code}@${pictureId}`] = `/ui/items/${outputName}`;
      legacyVariants += 1;
    }
  }
  return {
    manifest,
    uniqueAssets: new Set(Object.values(manifest)).size,
    overlaySprites: spriteFiles.length,
    vanillaSprites: vanillaSpriteFiles.length,
    legacyAssets: renderedLegacy.size,
    legacyVariants,
  };
}

async function generateSkillVisuals() {
  assertGeneratedPath(SKILL_OUTPUT_ROOT);
  await mkdir(SKILL_OUTPUT_ROOT, { recursive: true });
  const manifest = {};
  let refreshed = 0;
  let preserved = 0;
  for (const [classCode, relativeSource] of Object.entries(SKILL_SOURCES)) {
    const sourcePath = classCode === 'war'
      ? WARLOCK_SKILL_SOURCE
      : resolve(VANILLA_SKILL_SPRITE_ROOT, relativeSource);
    const outputSources = Array.from({ length: SKILL_FRAME_COUNT / 2 }, (_, index) => {
      const frame = index * 2;
      const outputName = `${classCode}-${String(frame).padStart(2, '0')}.png`;
      return {
        frame,
        outputPath: resolve(SKILL_OUTPUT_ROOT, outputName),
        publicPath: `/ui/skills/${outputName}`,
      };
    });
    if (existsSync(sourcePath)) {
      const decoded = decodeRgbaSprite(await readFile(sourcePath), sourcePath);
      const frameWidth = decoded.width / SKILL_FRAME_COUNT;
      if (!Number.isInteger(frameWidth) || frameWidth <= 0) {
        throw new Error(`${sourcePath}: expected ${SKILL_FRAME_COUNT} horizontal skill frames.`);
      }
      if (frameWidth !== decoded.height + 2) {
        throw new Error(`${sourcePath}: expected two transparent columns around each ${decoded.height}px skill frame.`);
      }
      for (const { frame, outputPath } of outputSources) {
        const squareFrame = cropRgba(decoded, {
          x: (frame * frameWidth) + 1,
          y: 0,
          width: decoded.height,
          height: decoded.height,
        });
        await writeFile(
          outputPath,
          encodePng(squareFrame.width, squareFrame.height, squareFrame.rgba),
        );
        refreshed += 1;
      }
    } else if (outputSources.every(({ outputPath }) => existsSync(outputPath))) {
      preserved += outputSources.length;
    } else {
      throw new Error(`Missing governed individual skill visuals for class ${classCode}: ${sourcePath}.`);
    }
    manifest[classCode] = outputSources.map(({ publicPath }) => publicPath);
  }
  return { manifest, refreshed, preserved };
}

async function generateSkillTreeVisuals() {
  assertGeneratedPath(SKILL_TREE_OUTPUT_ROOT);
  await mkdir(SKILL_TREE_OUTPUT_ROOT, { recursive: true });
  const manifest = {};
  for (const [classCode, sourceName] of Object.entries(SKILL_TREE_SOURCES)) {
    const sourcePath = resolve(VANILLA_SKILL_TREE_SPRITE_ROOT, sourceName);
    if (!existsSync(sourcePath)) {
      throw new Error(`Missing governed skill tree for class ${classCode}: ${sourcePath}.`);
    }
    const atlas = decodeRgbaSprite(await readFile(sourcePath), sourcePath);
    const frameWidth = atlas.width / SKILL_TREE_FRAME_COUNT;
    if (!Number.isInteger(frameWidth) || frameWidth <= 0) {
      throw new Error(`${sourcePath}: expected ${SKILL_TREE_FRAME_COUNT} horizontal skill tree frames.`);
    }
    if (frameWidth !== SKILL_TREE_OUTPUT_SIZE.width + 2 || atlas.height !== SKILL_TREE_OUTPUT_SIZE.height) {
      throw new Error(
        `${sourcePath}: expected ${SKILL_TREE_OUTPUT_SIZE.width}×${SKILL_TREE_OUTPUT_SIZE.height} `
        + 'skill tree frames with one transparent column on each side.',
      );
    }
    manifest[classCode] = [];
    for (let frame = 0; frame < SKILL_TREE_FRAME_COUNT; frame += 1) {
      const cropped = cropRgba(atlas, {
        x: (frame * frameWidth) + 1,
        y: 0,
        width: SKILL_TREE_OUTPUT_SIZE.width,
        height: atlas.height,
      });
      const outputName = `${classCode}-${frame}.png`;
      await writeFile(
        resolve(SKILL_TREE_OUTPUT_ROOT, outputName),
        encodePng(cropped.width, cropped.height, cropped.rgba),
      );
      manifest[classCode].push(`/ui/skill-trees/${outputName}`);
    }
  }
  return { manifest, ...SKILL_TREE_OUTPUT_SIZE };
}

async function generateClassPortraits() {
  assertGeneratedPath(PORTRAIT_OUTPUT_ROOT);
  await mkdir(PORTRAIT_OUTPUT_ROOT, { recursive: true });
  const manifest = {};
  const warlockAtlas = decodeRgbaSprite(await readFile(WARLOCK_SKILL_SOURCE), WARLOCK_SKILL_SOURCE);
  const warlockFrameWidth = warlockAtlas.width / SKILL_FRAME_COUNT;
  if (!Number.isInteger(warlockFrameWidth) || warlockFrameWidth <= 0) {
    throw new Error(`${WARLOCK_SKILL_SOURCE}: expected ${SKILL_FRAME_COUNT} horizontal skill frames.`);
  }

  for (const [className, relativeSource] of Object.entries(CLASS_PORTRAIT_SOURCES)) {
    let portrait;
    if (className === 'warlock') {
      const frame = cropRgba(warlockAtlas, {
        x: WARLOCK_PORTRAIT_SKILL_FRAME * warlockFrameWidth,
        y: 0,
        width: warlockFrameWidth,
        height: warlockAtlas.height,
      });
      portrait = scaleRgbaNearest(frame, PORTRAIT_SIZE, PORTRAIT_SIZE);
    } else {
      const sourcePath = resolve(VANILLA_PORTRAIT_SPRITE_ROOT, relativeSource);
      if (!existsSync(sourcePath)) {
        throw new Error(`Missing governed class portrait for ${className}: ${sourcePath}.`);
      }
      portrait = decodeRgbaSprite(await readFile(sourcePath), sourcePath);
      if (portrait.width !== PORTRAIT_SIZE || portrait.height !== PORTRAIT_SIZE) {
        throw new Error(`${sourcePath}: expected ${PORTRAIT_SIZE}x${PORTRAIT_SIZE} class portrait.`);
      }
    }
    const outputPath = resolve(PORTRAIT_OUTPUT_ROOT, `${className}.png`);
    await writeFile(outputPath, encodePng(portrait.width, portrait.height, portrait.rgba));
    manifest[className] = `/ui/portraits/${className}.png`;
  }
  return { manifest, size: PORTRAIT_SIZE };
}

async function main() {
  assertGeneratedPath(PUBLIC_UI_ROOT);
  assertGeneratedPath(GENERATED_MANIFEST);
  assertGeneratedPath(GENERATED_SKILL_MANIFEST);
  await mkdir(PUBLIC_UI_ROOT, { recursive: true });
  const equipment = await generateEquipmentPanel();
  const mercenaryEquipment = await generateMercenaryEquipmentPanel();
  const items = await generateItemVisuals();
  const skills = await generateSkillVisuals();
  const skillTrees = await generateSkillTreeVisuals();
  const portraits = await generateClassPortraits();
  const equipmentSlotPlaceholderVisuals = Object.fromEntries(
    Object.entries(EQUIPMENT_SLOT_PLACEHOLDER_CODES).map(([slot, code]) => {
      const visual = items.manifest[code];
      if (!visual) throw new Error(`Missing equipment placeholder visual for slot ${slot}: ${code}.`);
      return [slot, visual];
    }),
  );
  const source = `// Generated by scripts/generate-ui-assets.mjs from governed BKVince and locally extracted D2R SpA1 v31 sprites.\n`
    + `export const equipmentPanelVisual = '/ui/equipment-panel.png';\n\n`
    + `export const mercenaryEquipmentPanelVisual = '/ui/mercenary-equipment-panel.png';\n\n`
    + `export const equipmentSlotPlaceholderVisuals = Object.freeze(${JSON.stringify(equipmentSlotPlaceholderVisuals, null, 2)});\n\n`
    + `export const itemVisuals = Object.freeze(${JSON.stringify(items.manifest, null, 2)});\n`;
  await writeFile(GENERATED_MANIFEST, source, 'utf8');
  const skillSource = `// Generated by scripts/generate-ui-assets.mjs from governed BKVince and locally extracted D2R skill atlases.\n`
    + `export const skillVisuals = Object.freeze(${JSON.stringify(skills.manifest, null, 2)});\n\n`
    + `export const skillTreeVisuals = Object.freeze(${JSON.stringify(skillTrees.manifest, null, 2)});\n\n`
    + `export const classPortraitVisuals = Object.freeze(${JSON.stringify(portraits.manifest, null, 2)});\n`;
  await writeFile(GENERATED_SKILL_MANIFEST, skillSource, 'utf8');
  console.log(`Generated ${Object.keys(portraits.manifest).length} class portraits (${portraits.size}x${portraits.size}).`);
  console.log(`Generated ${Object.keys(skillTrees.manifest).length * SKILL_TREE_FRAME_COUNT} native skill tree backgrounds (${skillTrees.width}x${skillTrees.height}).`);
  console.log(`Generated player equipment panel ${equipment.width}×${equipment.height}, mercenary equipment panel ${mercenaryEquipment.width}×${mercenaryEquipment.height}, ${items.uniqueAssets} item visuals (${Object.keys(items.manifest).length} keys; ${items.overlaySprites} BKVince, ${items.vanillaSprites} cached vanilla sprites, and ${items.legacyAssets} legacy picture assets for ${items.legacyVariants} variants), and ${Object.values(skills.manifest).flat().length} individual skill visuals (${skills.refreshed} refreshed, ${skills.preserved} preserved).`);
}

await main();
