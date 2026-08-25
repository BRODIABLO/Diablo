import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

import {
  REPOSITORY_ROOT,
  extractPlayerSequences,
} from './player-sequences.mjs';

const require = createRequire(import.meta.url);
const { ENCODING, parseTable, serializeTable, writeTable } = require('../build-data/tsv.js');

const SCRIPT_PATH = fileURLToPath(import.meta.url);
export const DEFAULT_TABLE_ROOT = path.join(
  REPOSITORY_ROOT,
  'addons',
  'PlayerSequenceTables',
  'data',
  'global',
  'excel',
);

function sha256(bytes) {
  return crypto.createHash('sha256').update(bytes).digest('hex').toUpperCase();
}

function rowsAsObjects(table) {
  return table.rows.map((row) => Object.fromEntries(
    table.headers.map((header, index) => [header, row[index]]),
  ));
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
  const parsed = parseTable(filePath);
  assert.equal(parsed.eol, '\r\n', `Generated table is not CRLF: ${filePath}`);
  assert.equal(serializeTable(parsed), raw, `Generated table round-trip drift: ${filePath}`);
  assert(!raw.startsWith('\u00EF\u00BB\u00BF'), `Generated table has a UTF-8 BOM: ${filePath}`);
  return {
    bytes: Buffer.byteLength(raw, ENCODING),
    sha256: sha256(Buffer.from(raw, ENCODING)),
  };
}

export function buildPlayerSequenceTables() {
  const extraction = extractPlayerSequences();
  const mappings = rowsAsObjects(extraction.mappingTable);
  const records = rowsAsObjects(extraction.recordsTable);
  assert.equal(mappings.length, 350, 'The governed route matrix must contain 350 rows');

  const recordsByArray = new Map();
  for (const record of records) {
    let rows = recordsByArray.get(record.array_name);
    if (!rows) {
      rows = [];
      recordsByArray.set(record.array_name, rows);
    }
    assert.equal(Number(record.record_index), rows.length, `Non-contiguous ${record.array_name}`);
    rows.push(record);
  }
  assert.equal(recordsByArray.size, 47, 'The governed runtime must expose 47 record arrays');

  const contentByArray = new Map();
  for (const [arrayName, arrayRows] of recordsByArray) {
    const bytes = Buffer.from(arrayRows.map((row) => row.native_bytes).join(''), 'hex');
    contentByArray.set(arrayName, sha256(bytes));
  }

  const canonicalByHash = new Map();
  for (const mapping of mappings) {
    if (mapping.available !== '1') continue;
    const contentHash = contentByArray.get(mapping.array_name);
    assert(contentHash, `Missing records for ${mapping.array_name}`);
    if (!canonicalByHash.has(contentHash)) {
      canonicalByHash.set(contentHash, mapping.array_name);
    }
  }
  assert.equal(canonicalByHash.size, 44, 'The governed runtime must expose 44 unique contents');

  const canonicalByArray = new Map();
  for (const [arrayName, contentHash] of contentByArray) {
    canonicalByArray.set(arrayName, canonicalByHash.get(contentHash));
  }

  const routeRows = mappings.map((mapping, index) => {
    const sequence = Math.floor(index / 14) + 1;
    assert.equal(Number(mapping.sequence_id), sequence, `Unexpected sequence order at route ${index}`);
    return {
      seqnum: mapping.sequence_id,
      '*sequence': mapping.sequence_name,
      weaponclass: mapping.weapon_class,
      recordset: mapping.available === '1' ? canonicalByArray.get(mapping.array_name) : '',
      '*eol': 0,
    };
  });

  const recordRows = [];
  for (const canonicalName of canonicalByHash.values()) {
    const sourceRows = recordsByArray.get(canonicalName);
    assert(sourceRows?.length, `Missing canonical record set ${canonicalName}`);
    for (const record of sourceRows) {
      recordRows.push({
        recordset: canonicalName,
        mode: record.mode_code,
        frame: record.frame,
        dir: record.direction,
        event: record.event_id,
        '*eol': 0,
      });
    }
  }

  assert.equal(routeRows.filter((row) => row.recordset).length, 235);
  assert.equal(routeRows.filter((row) => !row.recordset).length, 115);
  assert.equal(recordRows.length, 757, 'Content de-duplication must retain 757 records');

  const routeTable = makeTable(
    ['seqnum', '*sequence', 'weaponclass', 'recordset', '*eol'],
    routeRows,
  );
  const recordTable = makeTable(
    ['recordset', 'mode', 'frame', 'dir', 'event', '*eol'],
    recordRows,
  );
  return {
    routeTable,
    recordTable,
    stats: {
      routes: routeRows.length,
      availableRoutes: routeRows.filter((row) => row.recordset).length,
      nullRoutes: routeRows.filter((row) => !row.recordset).length,
      sourceArrays: recordsByArray.size,
      uniqueRecordSets: canonicalByHash.size,
      records: recordRows.length,
    },
  };
}

export function generatePlayerSequenceTables({ outputRoot = DEFAULT_TABLE_ROOT } = {}) {
  const result = buildPlayerSequenceTables();
  fs.mkdirSync(outputRoot, { recursive: true });
  const files = {
    routes: path.join(outputRoot, 'playerseqmap.txt'),
    records: path.join(outputRoot, 'playerseq.txt'),
  };
  writeTable(files.routes, result.routeTable);
  writeTable(files.records, result.recordTable);
  return {
    ...result,
    files,
    outputs: {
      routes: verifyWrittenTable(files.routes),
      records: verifyWrittenTable(files.records),
    },
  };
}

function main() {
  const args = new Set(process.argv.slice(2));
  const result = args.has('--write')
    ? generatePlayerSequenceTables()
    : buildPlayerSequenceTables();
  process.stdout.write(`${JSON.stringify({ stats: result.stats, outputs: result.outputs }, null, 2)}\n`);
}

if (process.argv[1] && path.resolve(process.argv[1]) === SCRIPT_PATH) {
  main();
}
