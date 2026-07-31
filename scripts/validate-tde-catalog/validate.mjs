import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import Ajv2020 from 'ajv/dist/2020.js';
import addFormats from 'ajv-formats';

const defaultRepoRoot = fileURLToPath(new URL('../../', import.meta.url));
const defaultCatalogPath = path.join(defaultRepoRoot, 'Mission', 'tde-inspiration-bkvince.catalog.json');
const defaultSchemaPath = path.join(defaultRepoRoot, 'Mission', 'tde-inspiration-bkvince.schema.json');
const effortRank = new Map([['low', 0], ['medium', 1], ['high', 2], ['unknown', 3]]);

function loadJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

function expectedPriority(scores) {
  return scores.playerValue * 6
    + scores.bkvinceFit * 5
    + scores.technicalConfidence * 4
    + scores.isolation * 3
    + scores.maintenance * 2;
}

function compareEntries(left, right) {
  return right.scores.priority - left.scores.priority
    || right.scores.playerValue - left.scores.playerValue
    || right.scores.bkvinceFit - left.scores.bkvinceFit
    || effortRank.get(left.effort) - effortRank.get(right.effort)
    || left.id.localeCompare(right.id, 'en');
}

export function validateCatalog(catalog, schema) {
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

  const ids = new Set();
  const entries = new Map();
  const tableFiles = new Set(catalog.coverage.tables.files);
  for (const entry of catalog.entries) {
    if (ids.has(entry.id)) errors.push(`duplicate entry id: ${entry.id}`);
    ids.add(entry.id);
    entries.set(entry.id, entry);

    const priority = expectedPriority(entry.scores);
    if (entry.scores.priority !== priority) {
      errors.push(`${entry.id}: priority ${entry.scores.priority} does not match formula result ${priority}`);
    }
    for (const ref of entry.sourceRefs) {
      const paragraph = Number(ref.slice('readme:p'.length));
      if (paragraph > catalog.source.paragraphCount) {
        errors.push(`${entry.id}: source reference exceeds paragraph count: ${ref}`);
      }
    }
    for (const table of entry.tableEvidence) {
      if (!tableFiles.has(table)) errors.push(`${entry.id}: unknown table evidence: ${table}`);
    }
    if (entry.disposition === 'shortlisted' && entry.requiresNewAssets) {
      errors.push(`${entry.id}: shortlisted entries cannot require new assets`);
    }
    if (entry.lane === 'native' && entry.route === 'data_only') {
      errors.push(`${entry.id}: native entries cannot use a data-only route`);
    }
    if (entry.lane === 'native' && !entry.futureDestinationGateRequired) {
      errors.push(`${entry.id}: native entries must retain the future plugin destination gate`);
    }
  }

  let expectedStart = 1;
  for (const chunk of catalog.coverage.readmeChunks) {
    if (chunk.start !== expectedStart) {
      errors.push(`README coverage gap or overlap before paragraph ${chunk.start}; expected ${expectedStart}`);
    }
    if (chunk.end < chunk.start) errors.push(`README chunk ends before it starts: ${chunk.start}-${chunk.end}`);
    expectedStart = chunk.end + 1;
  }
  if (expectedStart !== catalog.source.paragraphCount + 1) {
    errors.push(`README coverage ends at ${expectedStart - 1}; expected ${catalog.source.paragraphCount}`);
  }
  if (catalog.coverage.tables.files.length !== catalog.source.tableCount) {
    errors.push(`table manifest has ${catalog.coverage.tables.files.length} files; expected ${catalog.source.tableCount}`);
  }
  if (!catalog.coverage.tables.allCrlf || !catalog.coverage.tables.roundTripByteExact) {
    errors.push('table evidence must remain CRLF and byte-exact under the governed round trip');
  }

  const shortlistMembers = new Set();
  for (const [laneName, shortlist] of Object.entries(catalog.shortlists)) {
    const expectedLane = laneName === 'dataFirst' ? 'data' : 'native';
    const selected = [];
    for (const id of shortlist) {
      if (shortlistMembers.has(id)) errors.push(`entry appears in both shortlists: ${id}`);
      shortlistMembers.add(id);
      const entry = entries.get(id);
      if (!entry) {
        errors.push(`${laneName}: unknown shortlisted id ${id}`);
        continue;
      }
      selected.push(entry);
      if (entry.lane !== expectedLane) errors.push(`${id}: expected ${expectedLane} lane for ${laneName}`);
      if (entry.disposition !== 'shortlisted') errors.push(`${id}: shortlist member must have shortlisted disposition`);
    }
    const sorted = [...selected].sort(compareEntries).map((entry) => entry.id);
    if (sorted.join('\n') !== shortlist.join('\n')) {
      errors.push(`${laneName}: shortlist order does not follow the governed score and tie-break rules`);
    }
  }
  for (const entry of catalog.entries) {
    if (entry.disposition === 'shortlisted' && !shortlistMembers.has(entry.id)) {
      errors.push(`${entry.id}: shortlisted disposition is missing from both Top 10 lists`);
    }
  }

  return errors;
}

export function validateCatalogFiles(catalogPath = defaultCatalogPath, schemaPath = defaultSchemaPath) {
  return validateCatalog(loadJson(catalogPath), loadJson(schemaPath));
}

export function main(catalogPath = defaultCatalogPath, schemaPath = defaultSchemaPath) {
  let errors;
  try {
    errors = validateCatalogFiles(catalogPath, schemaPath);
  } catch (error) {
    console.error(`INVALID : TDE catalog could not be read (${error.message})`);
    return 2;
  }
  if (errors.length === 0) {
    console.log('VALID : TDE inspiration catalog is governed, exhaustive, and internally consistent');
    return 0;
  }
  console.error(`INVALID : TDE inspiration catalog (${errors.length} error(s))`);
  for (const error of errors) console.error(`  ${error}`);
  return 1;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  process.exitCode = main(
    process.argv[2] ? path.resolve(process.argv[2]) : defaultCatalogPath,
    process.argv[3] ? path.resolve(process.argv[3]) : defaultSchemaPath,
  );
}
