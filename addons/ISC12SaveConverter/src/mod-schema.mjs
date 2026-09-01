import { readdir, readFile, stat } from 'node:fs/promises';
import path from 'node:path';

import { readConstantData } from '@d2runewizard/d2s';

export const SAVE_SCHEMA_FORMAT = 'ruffneckk-isc12-save-schema';
export const SAVE_SCHEMA_VERSION = 1;
export const MAX_SCHEMA_BYTES = 64 * 1024 * 1024;

const EXCEL_FILES = Object.freeze([
  'Armor.txt',
  'CharStats.txt',
  'Gems.txt',
  'ItemStatCost.txt',
  'ItemTypes.txt',
  'MagicPrefix.txt',
  'MagicSuffix.txt',
  'Misc.txt',
  'PlayerClass.txt',
  'Properties.txt',
  'RarePrefix.txt',
  'RareSuffix.txt',
  'Runes.txt',
  'SetItems.txt',
  'SkillDesc.txt',
  'skills.txt',
  'UniqueItems.txt',
  'Weapons.txt',
]);

const STRING_FILES = Object.freeze([
  'item-gems.json',
  'item-modifiers.json',
  'item-nameaffixes.json',
  'item-names.json',
  'item-runes.json',
  'skills.json',
]);

export async function loadConstantsFromMod(modPath) {
  const dataRoot = await resolveModDataRoot(modPath);
  const excelRoot = path.join(dataRoot, 'global', 'excel');
  const stringsRoot = path.join(dataRoot, 'local', 'lng', 'strings');
  const buffers = {};

  await loadRequiredFiles(buffers, excelRoot, EXCEL_FILES, 'Excel table');
  await loadRequiredFiles(buffers, stringsRoot, STRING_FILES, 'string table');
  let constants;
  try {
    constants = readConstantData(buffers);
  } catch (error) {
    throw new Error(`Could not build the save schema from ${dataRoot}: ${error.message}`, {
      cause: error,
    });
  }
  validateConstants(constants);
  return Object.freeze({ constants, dataRoot });
}

export async function loadConstantsFromSchemaFile(schemaPath) {
  const absolutePath = path.resolve(schemaPath);
  const metadata = await stat(absolutePath);
  if (!metadata.isFile()) throw new Error(`Save schema is not a file: ${absolutePath}`);
  if (metadata.size > MAX_SCHEMA_BYTES) {
    throw new Error(`Save schema exceeds the ${MAX_SCHEMA_BYTES / 1024 / 1024} MiB safety limit.`);
  }
  let parsed;
  try {
    parsed = JSON.parse(await readFile(absolutePath, 'utf8'));
  } catch (error) {
    throw new Error(`Could not read save schema ${absolutePath}: ${error.message}`, { cause: error });
  }
  const constants = parsed?.format === SAVE_SCHEMA_FORMAT ? parsed.constants : parsed;
  if (parsed?.format === SAVE_SCHEMA_FORMAT && parsed.version !== SAVE_SCHEMA_VERSION) {
    throw new Error(`Unsupported save schema version: ${parsed.version}.`);
  }
  validateConstants(constants);
  return Object.freeze({
    constants,
    name: parsed?.format === SAVE_SCHEMA_FORMAT ? parsed.name || path.basename(absolutePath) : path.basename(absolutePath),
    schemaPath: absolutePath,
  });
}

export function createSaveSchema(constants, name = 'Custom D2R v105 mod') {
  validateConstants(constants);
  return Object.freeze({
    format: SAVE_SCHEMA_FORMAT,
    version: SAVE_SCHEMA_VERSION,
    name,
    constants,
  });
}

export async function resolveModDataRoot(modPath) {
  if (typeof modPath !== 'string' || modPath.trim() === '') {
    throw new TypeError('A mod or data directory is required.');
  }
  const root = path.resolve(modPath);
  if (!await isDirectory(root)) throw new Error(`Mod directory does not exist: ${root}`);

  const candidates = [root, path.join(root, 'data')];
  for (const entry of await readdir(root, { withFileTypes: true })) {
    if (entry.isDirectory() && entry.name.toLowerCase().endsWith('.mpq')) {
      candidates.push(path.join(root, entry.name, 'data'));
    }
  }
  for (const candidate of candidates) {
    if (await isFile(path.join(candidate, 'global', 'excel', 'ItemStatCost.txt'))) {
      return candidate;
    }
  }
  throw new Error(
    `No unpacked mod data was found under ${root}. Expected data/global/excel/ItemStatCost.txt.`,
  );
}

async function loadRequiredFiles(target, directory, requiredNames, kind) {
  if (!await isDirectory(directory)) throw new Error(`Missing ${kind} directory: ${directory}`);
  const entries = await readdir(directory, { withFileTypes: true });
  const names = new Map(entries
    .filter((entry) => entry.isFile())
    .map((entry) => [entry.name.toLowerCase(), entry.name]));
  const missing = [];
  for (const requiredName of requiredNames) {
    const actualName = names.get(requiredName.toLowerCase());
    if (!actualName) {
      missing.push(requiredName);
      continue;
    }
    target[requiredName] = await readFile(path.join(directory, actualName), 'utf8');
  }
  if (missing.length > 0) {
    throw new Error(`Missing required ${kind}${missing.length === 1 ? '' : 's'}: ${missing.join(', ')}`);
  }
}

async function isDirectory(filePath) {
  try {
    return (await stat(filePath)).isDirectory();
  } catch {
    return false;
  }
}

async function isFile(filePath) {
  try {
    return (await stat(filePath)).isFile();
  } catch {
    return false;
  }
}

function validateConstants(constants) {
  if (!Array.isArray(constants?.magical_properties) || constants.magical_properties.length === 0) {
    throw new Error('The mod schema does not contain any ItemStatCost definitions.');
  }
  if (!constants?.armor_items || !constants?.weapon_items || !constants?.other_items) {
    throw new Error('The mod schema does not contain the required item base definitions.');
  }
}

export const MOD_SCHEMA_FILES = Object.freeze({
  excel: EXCEL_FILES,
  strings: STRING_FILES,
});
