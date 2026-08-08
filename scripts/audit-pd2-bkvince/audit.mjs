import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

const require = createRequire(import.meta.url);
const { ENCODING, parseTable, serializeTable } = require('../build-data/tsv.js');

const repoRoot = fileURLToPath(new URL('../../', import.meta.url));
export const defaultRoots = {
  pd2: process.env.PD2_SP_ROOT || path.resolve(repoRoot, '..', 'PD2 Single PLayer', 'PD2-Single-Player-Plus-mod-main', 'data', 'global', 'excel'),
  bkvince: path.join(repoRoot, 'data-BKVince', 'BKVince.mpq', 'data', 'global', 'excel'),
  bk: path.join(repoRoot, 'data-BK', 'global', 'excel'),
  vanilla32: path.join(repoRoot, 'data-vanilla3.2', 'data', 'data', 'global', 'excel'),
};

function sha256(buffer) {
  return crypto.createHash('sha256').update(buffer).digest('hex').toUpperCase();
}

function listTables(root) {
  if (!fs.existsSync(root)) return new Map();
  return new Map(
    fs.readdirSync(root)
      .filter((name) => name.toLowerCase().endsWith('.txt'))
      .sort((left, right) => left.toLowerCase().localeCompare(right.toLowerCase(), 'en'))
      .map((name) => [name.toLowerCase(), name]),
  );
}

function auditRoot(root) {
  const files = listTables(root);
  const tables = new Map();
  const manifestLines = [];
  for (const [normalizedName, actualName] of files) {
    const filePath = path.join(root, actualName);
    const rawBuffer = fs.readFileSync(filePath);
    const rawText = fs.readFileSync(filePath, ENCODING);
    const table = parseTable(filePath);
    const digest = sha256(rawBuffer);
    const record = {
      name: normalizedName,
      actualName,
      bytes: rawBuffer.length,
      rows: table.rows.length,
      columns: table.headers.length,
      headers: table.headers,
      eol: table.eol === '\r\n' ? 'CRLF' : table.eol === '\n' ? 'LF' : JSON.stringify(table.eol),
      hasFinalEol: table.hasFinalEol,
      roundTripByteExact: serializeTable(table) === rawText,
      sha256: digest,
    };
    tables.set(normalizedName, record);
    manifestLines.push(`${normalizedName}\t${digest}`);
  }
  return {
    root,
    files: [...files.keys()],
    manifestSha256: sha256(Buffer.from(manifestLines.join('\n'), 'utf8')),
    allRoundTripByteExact: [...tables.values()].every((table) => table.roundTripByteExact),
    eolKinds: [...new Set([...tables.values()].map((table) => table.eol))].sort(),
    tables,
  };
}

function compactTable(table) {
  if (!table) return null;
  return {
    rows: table.rows,
    columns: table.columns,
    bytes: table.bytes,
    eol: table.eol,
    roundTripByteExact: table.roundTripByteExact,
  };
}

function compareTables(pd2, bkvince, bk, vanilla32) {
  const matrix = [];
  for (const name of pd2.files) {
    const source = pd2.tables.get(name);
    const target = bkvince.tables.get(name);
    const sourceHeaders = new Set(source.headers.map((header) => header.toLowerCase()));
    const targetHeaders = new Set((target?.headers || []).map((header) => header.toLowerCase()));
    const commonHeaders = [...sourceHeaders].filter((header) => targetHeaders.has(header));
    const normalizedSourceHeaders = source.headers.map((header) => header.toLowerCase());
    const normalizedTargetHeaders = (target?.headers || []).map((header) => header.toLowerCase());
    matrix.push({
      table: name,
      pd2: compactTable(source),
      bkvince: compactTable(target),
      bk: compactTable(bk.tables.get(name)),
      vanilla32: compactTable(vanilla32.tables.get(name)),
      sharedWithBkvince: Boolean(target),
      exactNormalizedHeader: Boolean(target) && JSON.stringify(normalizedSourceHeaders) === JSON.stringify(normalizedTargetHeaders),
      commonHeaders: commonHeaders.length,
      pd2OnlyHeaders: [...sourceHeaders].filter((header) => !targetHeaders.has(header)).length,
      bkvinceOnlyHeaders: [...targetHeaders].filter((header) => !sourceHeaders.has(header)).length,
    });
  }
  return matrix;
}

export function buildAudit(roots = defaultRoots) {
  if (!fs.existsSync(roots.pd2)) {
    throw new Error(`PD2 Single Player+ table root not found: ${roots.pd2}`);
  }
  const pd2 = auditRoot(roots.pd2);
  const bkvince = auditRoot(roots.bkvince);
  const bk = auditRoot(roots.bk);
  const vanilla32 = auditRoot(roots.vanilla32);
  const matrix = compareTables(pd2, bkvince, bk, vanilla32);
  const pd2Names = new Set(pd2.files);
  const bkvinceNames = new Set(bkvince.files);
  return {
    generatedAt: new Date().toISOString(),
    roots,
    summary: {
      pd2Tables: pd2.files.length,
      bkvinceTables: bkvince.files.length,
      bkTables: bk.files.length,
      vanilla32Tables: vanilla32.files.length,
      commonPd2Bkvince: pd2.files.filter((name) => bkvinceNames.has(name)).length,
      exactNormalizedHeaders: matrix.filter((row) => row.exactNormalizedHeader).length,
      schemaDifferences: matrix.filter((row) => row.sharedWithBkvince && !row.exactNormalizedHeader).length,
      pd2Only: pd2.files.filter((name) => !bkvinceNames.has(name)),
      bkvinceOnly: bkvince.files.filter((name) => !pd2Names.has(name)),
      pd2ManifestSha256: pd2.manifestSha256,
      pd2AllRoundTripByteExact: pd2.allRoundTripByteExact,
      pd2EolKinds: pd2.eolKinds,
      bkvinceAllRoundTripByteExact: bkvince.allRoundTripByteExact,
      bkvinceEolKinds: bkvince.eolKinds,
    },
    matrix,
  };
}

function printSummary(audit) {
  const { summary } = audit;
  console.log(`PD2=${summary.pd2Tables} BKVince=${summary.bkvinceTables} BK=${summary.bkTables} vanilla3.2=${summary.vanilla32Tables}`);
  console.log(`shared=${summary.commonPd2Bkvince} exactHeaders=${summary.exactNormalizedHeaders} schemaDifferences=${summary.schemaDifferences}`);
  console.log(`PD2 manifest=${summary.pd2ManifestSha256} roundTrip=${summary.pd2AllRoundTripByteExact} eol=${summary.pd2EolKinds.join(',')}`);
  console.log(`BKVince roundTrip=${summary.bkvinceAllRoundTripByteExact} eol=${summary.bkvinceEolKinds.join(',')}`);
  console.log('table\tPD2 rows/cols\tBKVince rows/cols\tcommon headers\tPD2-only headers\tBKVince-only headers');
  for (const row of audit.matrix) {
    const source = `${row.pd2.rows}/${row.pd2.columns}`;
    const target = row.bkvince ? `${row.bkvince.rows}/${row.bkvince.columns}` : '-';
    console.log(`${row.table}\t${source}\t${target}\t${row.commonHeaders}\t${row.pd2OnlyHeaders}\t${row.bkvinceOnlyHeaders}`);
  }
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try {
    const audit = buildAudit();
    if (process.argv.includes('--json')) console.log(JSON.stringify(audit, null, 2));
    else printSummary(audit);
  } catch (error) {
    console.error(`PD2/BKVince audit failed: ${error.message}`);
    process.exitCode = 1;
  }
}
