#!/usr/bin/env node

import { createInterface } from 'node:readline/promises';

import {
  BatchConversionError,
  chooseUnusedOutputDirectory,
  convertBatch,
  parseDirection,
} from './batch-converter.mjs';
import { DEFAULT_D2R_V105_CONSTANTS } from './default-schema.mjs';
import {
  loadConstantsFromMod,
  loadConstantsFromSchemaFile,
} from './mod-schema.mjs';

export async function runCli(argv, io = console) {
  const options = parseArguments(argv);
  if (options.help) {
    io.log(USAGE);
    return 0;
  }
  const direction = parseDirection(options.to);
  const schema = await loadSelectedSchema(options);
  const outputDirectory = options.output || await chooseUnusedOutputDirectory(
    options.inputs[0],
    direction.targetWidth,
  );
  const result = await convertBatch({
    inputs: options.inputs,
    outputDirectory,
    constants: schema.constants,
    ...direction,
  });

  io.log(`Converted ${result.files.length} save file${result.files.length === 1 ? '' : 's'}.`);
  io.log(`Schema: ${schema.name}`);
  io.log(`Output: ${result.outputDirectory}`);
  result.files.forEach((file) => {
    io.log(`  ${file.file}  ${file.inputBytes} -> ${file.outputBytes} bytes  [${file.kind}]`);
  });
  io.log('Original files were not modified.');
  return 0;
}

export async function promptForArguments(input = process.stdin, output = process.stdout) {
  const prompt = createInterface({ input, output });
  try {
    output.write(`${USAGE.split('\n').slice(0, 4).join('\n')}\n\n`);
    output.write('1. D2R 9-bit -> ISC12 12-bit\n');
    output.write('2. ISC12 12-bit -> D2R 9-bit\n');
    const direction = (await prompt.question('Choose 1 or 2: ')).trim();
    if (direction !== '1' && direction !== '2') throw new Error('Direction must be 1 or 2.');
    const source = unquote(await prompt.question('Save file or folder (you can drag it here): '));
    if (!source) throw new Error('A save file or folder is required.');
    const schema = unquote(await prompt.question(
      'Custom mod schema pack (press Enter for built-in D2R v105): ',
    ));
    return [
      '--to', direction === '1' ? 'isc12' : 'd2r9',
      ...(schema ? ['--schema', schema] : []),
      source,
    ];
  } finally {
    prompt.close();
  }
}

export function parseArguments(argv) {
  const options = { inputs: [] };
  for (let index = 0; index < argv.length; index += 1) {
    const argument = argv[index];
    if (argument === '--help' || argument === '-h') {
      options.help = true;
    } else if (argument === '--to') {
      options.to = readOptionValue(argv, ++index, '--to');
    } else if (argument === '--schema') {
      options.schema = readOptionValue(argv, ++index, '--schema');
    } else if (argument === '--mod') {
      options.mod = readOptionValue(argv, ++index, '--mod');
    } else if (argument === '--output' || argument === '-o') {
      options.output = readOptionValue(argv, ++index, argument);
    } else if (argument.startsWith('-')) {
      throw new Error(`Unknown option: ${argument}`);
    } else {
      options.inputs.push(argument);
    }
  }
  if (options.help) return Object.freeze(options);
  if (!options.to) throw new Error('Missing required option: --to isc12|d2r9.');
  if (options.schema && options.mod) throw new Error('Use either --schema or --mod, not both.');
  if (options.inputs.length === 0) throw new Error('At least one .d2s/.d2i file or save directory is required.');
  return Object.freeze(options);
}

async function loadSelectedSchema(options) {
  if (options.schema) return loadConstantsFromSchemaFile(options.schema);
  if (options.mod) {
    const result = await loadConstantsFromMod(options.mod);
    return Object.freeze({
      constants: result.constants,
      name: `Unpacked mod data: ${result.dataRoot}`,
    });
  }
  return Object.freeze({
    constants: DEFAULT_D2R_V105_CONSTANTS,
    name: 'Built-in Diablo II: Resurrected v105',
  });
}

function readOptionValue(argv, index, option) {
  const value = argv[index];
  if (!value || value.startsWith('-')) throw new Error(`Missing value for ${option}.`);
  return value;
}

function unquote(value) {
  const trimmed = String(value || '').trim();
  if (trimmed.length >= 2 && (
    (trimmed.startsWith('"') && trimmed.endsWith('"'))
    || (trimmed.startsWith("'") && trimmed.endsWith("'"))
  )) {
    return trimmed.slice(1, -1);
  }
  return trimmed;
}

export const USAGE = `ISC12 Save Converter

Converts copies of D2R v105 .d2s and .d2i files between D2R 9-bit and
ISC12 12-bit ItemStatCost streams. Original files are never overwritten.

Usage:
  isc12-save-converter --to isc12 [options] <file-or-directory> [...]
  isc12-save-converter --to d2r9 [options] <file-or-directory> [...]

Options:
  --to isc12|d2r9         Required explicit conversion direction.
  --schema <file>         Versioned JSON schema pack supplied by the mod author.
  --mod <directory>       Complete unpacked D2R mod data directory.
  --output, -o <dir>      New output directory. It must not already exist.
  --help, -h              Show this help.

Without --schema or --mod, the built-in D2R v105 schema is used. Custom mods
should distribute a matching schema pack so their custom item bases and stat
payload widths can be decoded safely.`;

export async function main(argv = process.argv.slice(2), io = console) {
  try {
    const argumentsToRun = argv.length > 0
      ? argv
      : await promptForArguments();
    return await runCli(argumentsToRun, io);
  } catch (error) {
    if (error instanceof BatchConversionError) {
      io.error(error.message);
      error.failures.forEach((failure) => {
        io.error(`  ${failure.file}: ${failure.message}`);
        failure.blockers?.forEach((blocker) => {
          io.error(`    stat ${blocker.id}: ${blocker.path}`);
        });
      });
    } else {
      io.error(error.message);
    }
    return 1;
  }
}
