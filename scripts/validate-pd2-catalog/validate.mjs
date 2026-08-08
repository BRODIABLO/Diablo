import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import Ajv2020 from 'ajv/dist/2020.js';
import addFormats from 'ajv-formats';
import { buildAudit, defaultRoots } from '../audit-pd2-bkvince/audit.mjs';

const repoRoot = fileURLToPath(new URL('../../', import.meta.url));
const defaultCatalogPath = path.join(repoRoot, 'Mission', 'pd2-inspiration-bkvince.catalog.json');
const defaultSchemaPath = path.join(repoRoot, 'Mission', 'pd2-inspiration-bkvince.schema.json');

function loadJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

export function validateCatalog(catalog, schema, audit = null, options = {}) {
  const { requireTargetBaseline = false } = options;
  const errors = [];
  const ajv = new Ajv2020({ allErrors: true, strict: false });
  addFormats(ajv);
  const validateSchema = ajv.compile(schema);
  if (!validateSchema(catalog)) {
    for (const issue of validateSchema.errors || []) {
      errors.push(`schema ${issue.instancePath || '/'} ${issue.message}`);
    }
    return errors;
  }

  const sourceTables = new Set(catalog.coverage.pd2Tables);
  const targetOnlyTables = new Set(catalog.coverage.bkvinceOnly);
  const entries = new Map();
  for (const entry of catalog.entries) {
    if (entries.has(entry.id)) errors.push(`duplicate entry id: ${entry.id}`);
    entries.set(entry.id, entry);
    for (const table of entry.tables) {
      if (!sourceTables.has(table) && !targetOnlyTables.has(table)) {
        errors.push(`${entry.id}: unknown table evidence ${table}`);
      }
    }
    if (entry.disposition === 'needs_re' && !['memory_patch', 'plugin', 'hybrid', 'unknown'].includes(entry.route)) {
      errors.push(`${entry.id}: needs_re requires a native, hybrid, or unknown route`);
    }
    if (entry.route === 'data_only' && entry.disposition === 'needs_re') {
      errors.push(`${entry.id}: data-only entries cannot require reverse engineering`);
    }
  }

  const chapterIds = new Set();
  for (const chapter of catalog.chapters) {
    if (chapterIds.has(chapter.id)) errors.push(`duplicate chapter id: ${chapter.id}`);
    chapterIds.add(chapter.id);
  }

  for (const id of catalog.nativeBacklog) {
    const entry = entries.get(id);
    if (!entry) {
      errors.push(`native backlog references unknown entry: ${id}`);
      continue;
    }
    if (!['memory_patch', 'plugin', 'hybrid', 'unknown'].includes(entry.route)) {
      errors.push(`${id}: native backlog entry has non-native route ${entry.route}`);
    }
  }

  if (audit) {
    const expectedFiles = catalog.coverage.pd2Tables.join('\n');
    const actualFiles = audit.matrix.map((row) => row.table).join('\n');
    if (expectedFiles !== actualFiles) errors.push('PD2 table manifest differs from the audited source');
    if (audit.summary.pd2ManifestSha256 !== catalog.source.tableManifestSha256) {
      errors.push(`PD2 source manifest hash differs: ${audit.summary.pd2ManifestSha256}`);
    }
    if (audit.summary.pd2Tables !== catalog.source.tableCount) errors.push('PD2 table count differs from catalog source metadata');
    if (requireTargetBaseline) {
      if (audit.summary.bkvinceTables !== catalog.coverage.bkvinceTableCount) errors.push('BKVince table count differs from catalog coverage');
      if (audit.summary.commonPd2Bkvince !== catalog.coverage.commonTables) errors.push('common-table count differs from catalog coverage');
      if (audit.summary.exactNormalizedHeaders !== catalog.coverage.exactNormalizedHeaders) errors.push('exact-header count differs from catalog coverage');
      if (audit.summary.schemaDifferences !== catalog.coverage.schemaDifferences) errors.push('schema-difference count differs from catalog coverage');
      if (audit.summary.pd2Only.length !== catalog.coverage.pd2OnlyCount) errors.push('PD2-only table count differs from catalog coverage');
      if (audit.summary.bkvinceOnly.join('\n') !== catalog.coverage.bkvinceOnly.join('\n')) errors.push('BKVince-only table list differs from catalog coverage');
    }
    if (!audit.summary.pd2AllRoundTripByteExact) errors.push('PD2 source tables do not round-trip byte-exactly');
    if (!audit.summary.bkvinceAllRoundTripByteExact) errors.push('BKVince tables do not round-trip byte-exactly');
    if (audit.summary.pd2EolKinds.join(',') !== 'LF') errors.push(`PD2 EOL differs from catalog: ${audit.summary.pd2EolKinds.join(',')}`);
    if (audit.summary.bkvinceEolKinds.join(',') !== 'CRLF') errors.push(`BKVince EOL differs from governed CRLF: ${audit.summary.bkvinceEolKinds.join(',')}`);
  }
  return errors;
}

export function main(args = process.argv.slice(2)) {
  const requireSource = args.includes('--require-source');
  const requireTargetBaseline = args.includes('--require-target-baseline');
  let catalog;
  let schema;
  let audit = null;
  try {
    catalog = loadJson(defaultCatalogPath);
    schema = loadJson(defaultSchemaPath);
    if (requireSource || fs.existsSync(defaultRoots.pd2)) audit = buildAudit();
  } catch (error) {
    console.error(`INVALID : PD2 catalog could not be read or audited (${error.message})`);
    return 2;
  }
  const errors = validateCatalog(catalog, schema, audit, { requireTargetBaseline });
  if (errors.length === 0) {
    console.log(`VALID : PD2 inspiration catalog is governed and internally consistent${audit ? ' with the pinned 93-table PD2 source' : ''}`);
    return 0;
  }
  console.error(`INVALID : PD2 inspiration catalog (${errors.length} error(s))`);
  for (const error of errors) console.error(`  ${error}`);
  return 1;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  process.exitCode = main();
}
