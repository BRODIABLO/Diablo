#!/usr/bin/env node

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import Ajv from 'ajv';

const scriptDirectory = path.dirname(fileURLToPath(import.meta.url));
export const repositoryRoot = path.resolve(scriptDirectory, '..', '..');
export const defaultAtlasDirectory = path.join(
  repositoryRoot,
  'reverse-engineering',
  'd2r-3.2.92777',
  'datatables-atlas',
);
export const defaultCatalogPath = path.join(defaultAtlasDirectory, 'catalog.json');
export const defaultSchemaPath = path.join(defaultAtlasDirectory, 'atlas.schema.json');

const slotSize = 8;
const concreteStatuses = new Set(['proven', 'candidate']);

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

function parseHex(value) {
  if (typeof value !== 'string' || !/^0x[0-9A-F]+$/u.test(value)) return null;
  const parsed = Number.parseInt(value.slice(2), 16);
  return Number.isSafeInteger(parsed) ? parsed : null;
}

function resolveRepositoryPath(relativePath) {
  const resolved = path.resolve(repositoryRoot, relativePath);
  const rootWithSeparator = `${repositoryRoot}${path.sep}`;
  if (resolved !== repositoryRoot && !resolved.startsWith(rootWithSeparator)) {
    throw new Error('path escapes the repository root');
  }
  return resolved;
}

function decodePointerToken(token) {
  return token.replaceAll('~1', '/').replaceAll('~0', '~');
}

function resolveJsonPointer(document, pointer) {
  if (pointer === '') return document;
  if (typeof pointer !== 'string' || !pointer.startsWith('/')) return undefined;
  let current = document;
  for (const rawToken of pointer.slice(1).split('/')) {
    const token = decodePointerToken(rawToken);
    if (current === null || typeof current !== 'object' || !(token in current)) return undefined;
    current = current[token];
  }
  return current;
}

function isSubset(expected, actual) {
  if (Array.isArray(expected)) {
    return Array.isArray(actual)
      && expected.length === actual.length
      && expected.every((value, index) => isSubset(value, actual[index]));
  }
  if (expected !== null && typeof expected === 'object') {
    return actual !== null
      && typeof actual === 'object'
      && !Array.isArray(actual)
      && Object.entries(expected).every(([key, value]) => (
        Object.hasOwn(actual, key) && isSubset(value, actual[key])
      ));
  }
  return Object.is(expected, actual);
}

function makeIssue(code, location, message) {
  return { code, location, message };
}

function claimEquals(expected, actual) {
  return isSubset(expected, actual) && isSubset(actual, expected);
}

function compareIssues(left, right) {
  return left.location.localeCompare(right.location)
    || left.code.localeCompare(right.code)
    || left.message.localeCompare(right.message);
}

function validateEvidenceSource(evidence, cache) {
  const issues = [];
  const location = `evidence.${evidence.id ?? '<missing>'}`;
  let sourcePath;
  try {
    sourcePath = resolveRepositoryPath(evidence.path);
  } catch (error) {
    return [makeIssue('EVIDENCE_PATH', `${location}.path`, error.message)];
  }
  if (!fs.existsSync(sourcePath)) {
    return [makeIssue('EVIDENCE_MISSING', `${location}.path`, `source does not exist: ${evidence.path}`)];
  }

  const supportSet = new Set(evidence.supports ?? []);
  const claimKeys = Object.keys(evidence.claims ?? {});
  for (const support of supportSet) {
    if (!Object.hasOwn(evidence.claims ?? {}, support)) {
      issues.push(makeIssue('EVIDENCE_CLAIM', `${location}.claims`, `missing claim for ${support}`));
    }
  }
  for (const claimKey of claimKeys) {
    if (!supportSet.has(claimKey)) {
      issues.push(makeIssue('EVIDENCE_CLAIM', `${location}.claims`, `claim is not declared in supports: ${claimKey}`));
    }
  }

  function cachedJson() {
    if (!cache.has(sourcePath)) cache.set(sourcePath, readJson(sourcePath));
    return cache.get(sourcePath);
  }

  try {
    if (evidence.kind === 'known-rva') {
      if (!evidence.name || !evidence.rva) {
        issues.push(makeIssue('EVIDENCE_SHAPE', location, 'known-rva requires name and rva'));
      } else {
        const entries = cachedJson().entries;
        const matches = Array.isArray(entries)
          ? entries.filter((entry) => entry.name === evidence.name && entry.rva === evidence.rva)
          : [];
        if (matches.length === 0) {
          issues.push(makeIssue(
            'EVIDENCE_MISMATCH',
            location,
            `known RVA ${evidence.name} ${evidence.rva} was not found`,
          ));
        } else if (!matches.some((entry) => entry.confidence === 'high')) {
          issues.push(makeIssue('EVIDENCE_CONFIDENCE', location, 'known RVA is not high confidence'));
        }
      }
    } else if (evidence.kind === 'native-site') {
      if (!evidence.siteId || !evidence.rva) {
        issues.push(makeIssue('EVIDENCE_SHAPE', location, 'native-site requires siteId and rva'));
      } else {
        const document = cachedJson();
        const sites = document.sites ?? document.entries;
        const match = Array.isArray(sites)
          ? sites.find((site) => site.id === evidence.siteId && site.rva === evidence.rva)
          : undefined;
        if (!match) {
          issues.push(makeIssue(
            'EVIDENCE_MISMATCH',
            location,
            `native site ${evidence.siteId} ${evidence.rva} was not found`,
          ));
        } else if (match.signature?.unique !== true) {
          issues.push(makeIssue('EVIDENCE_UNIQUE', location, 'native site lacks a unique signature'));
        }
      }
    } else if (evidence.kind === 'json-pointer') {
      if (!evidence.pointer || !Object.hasOwn(evidence, 'expected')) {
        issues.push(makeIssue('EVIDENCE_SHAPE', location, 'json-pointer requires pointer and expected'));
      } else {
        const actual = resolveJsonPointer(cachedJson(), evidence.pointer);
        if (actual === undefined) {
          issues.push(makeIssue('EVIDENCE_POINTER', location, `JSON pointer not found: ${evidence.pointer}`));
        } else if (!isSubset(evidence.expected, actual)) {
          issues.push(makeIssue('EVIDENCE_MISMATCH', location, 'JSON pointer value does not contain expected facts'));
        }
      }
    } else if (evidence.kind === 'text-witness') {
      if (!evidence.needle) {
        issues.push(makeIssue('EVIDENCE_SHAPE', location, 'text-witness requires needle'));
      } else {
        const source = fs.readFileSync(sourcePath, 'utf8');
        if (!source.includes(evidence.needle)) {
          issues.push(makeIssue('EVIDENCE_MISMATCH', location, 'text witness was not found verbatim'));
        }
      }
    }
  } catch (error) {
    issues.push(makeIssue('EVIDENCE_READ', location, error.message));
  }
  return issues;
}

function validateFact({ fact, location, supportToken, evidenceById, issues }) {
  if (!fact || typeof fact !== 'object') return;
  const evidenceIds = Array.isArray(fact.evidenceIds) ? fact.evidenceIds : [];
  if (fact.status === 'unknown') {
    if (Object.hasOwn(fact, 'value')) {
      issues.push(makeIssue('UNKNOWN_CONCRETE', `${location}.value`, 'unknown facts cannot carry a value'));
    }
    if (evidenceIds.length > 0) {
      issues.push(makeIssue('UNKNOWN_EVIDENCE', `${location}.evidenceIds`, 'unknown facts cannot claim evidence'));
    }
    return;
  }
  if (!concreteStatuses.has(fact.status)) return;
  if (!Object.hasOwn(fact, 'value')) {
    issues.push(makeIssue('FACT_VALUE', `${location}.value`, `${fact.status} fact requires a value`));
  }
  if (evidenceIds.length === 0) {
    issues.push(makeIssue('FACT_EVIDENCE', `${location}.evidenceIds`, `${fact.status} fact requires evidence`));
  }
  for (const evidenceId of evidenceIds) {
    const evidence = evidenceById.get(evidenceId);
    if (!evidence) {
      issues.push(makeIssue('EVIDENCE_REFERENCE', `${location}.evidenceIds`, `unknown evidence id: ${evidenceId}`));
    } else if (!evidence.supports.includes(supportToken)) {
      issues.push(makeIssue(
        'EVIDENCE_SUPPORT',
        `${location}.evidenceIds`,
        `${evidenceId} does not support ${supportToken}`,
      ));
    } else if (!claimEquals(fact.value, evidence.claims[supportToken])) {
      issues.push(makeIssue(
        'EVIDENCE_CLAIM',
        `${location}.evidenceIds`,
        `${evidenceId} claims ${JSON.stringify(evidence.claims[supportToken])}, not ${JSON.stringify(fact.value)}`,
      ));
    }
  }
}

function validatePostProcessing(table, evidenceById, issues) {
  const fact = table.postProcessing;
  const location = `tables.${table.id}.postProcessing`;
  const evidenceIds = Array.isArray(fact?.evidenceIds) ? fact.evidenceIds : [];
  if (fact?.status === 'unknown') {
    if (evidenceIds.length > 0) {
      issues.push(makeIssue('UNKNOWN_EVIDENCE', `${location}.evidenceIds`, 'unknown post-processing cannot claim evidence'));
    }
    return;
  }
  if (!concreteStatuses.has(fact?.status)) return;
  if (evidenceIds.length === 0) {
    issues.push(makeIssue('FACT_EVIDENCE', `${location}.evidenceIds`, `${fact.status} post-processing requires evidence`));
  }
  const supportToken = `${table.id}:post-processing`;
  for (const evidenceId of evidenceIds) {
    const evidence = evidenceById.get(evidenceId);
    if (!evidence) {
      issues.push(makeIssue('EVIDENCE_REFERENCE', `${location}.evidenceIds`, `unknown evidence id: ${evidenceId}`));
    } else if (!evidence.supports.includes(supportToken)) {
      issues.push(makeIssue('EVIDENCE_SUPPORT', `${location}.evidenceIds`, `${evidenceId} does not support ${supportToken}`));
    } else if (evidence.claims[supportToken] !== true) {
      issues.push(makeIssue('EVIDENCE_CLAIM', `${location}.evidenceIds`, `${evidenceId} does not claim proven post-processing`));
    }
  }
}

function validateField(table, field, evidenceById, issues) {
  const location = `tables.${table.id}.recordFields.${field.id ?? '<missing>'}`;
  if (field.status === 'unknown') {
    issues.push(makeIssue('UNKNOWN_FIELD', location, 'unknown fields must be omitted, not assigned concrete geometry'));
    return;
  }
  const evidenceIds = Array.isArray(field.evidenceIds) ? field.evidenceIds : [];
  if (evidenceIds.length === 0) {
    issues.push(makeIssue('FACT_EVIDENCE', `${location}.evidenceIds`, `${field.status} field requires evidence`));
  }
  const supportToken = `${table.id}:field:${field.id}`;
  for (const evidenceId of evidenceIds) {
    const evidence = evidenceById.get(evidenceId);
    if (!evidence) {
      issues.push(makeIssue('EVIDENCE_REFERENCE', `${location}.evidenceIds`, `unknown evidence id: ${evidenceId}`));
    } else if (!evidence.supports.includes(supportToken)) {
      issues.push(makeIssue('EVIDENCE_SUPPORT', `${location}.evidenceIds`, `${evidenceId} does not support ${supportToken}`));
    } else {
      const expectedClaim = { offset: field.offset, size: field.size, type: field.type };
      if (!claimEquals(expectedClaim, evidence.claims[supportToken])) {
        issues.push(makeIssue(
          'EVIDENCE_CLAIM',
          `${location}.evidenceIds`,
          `${evidenceId} claims ${JSON.stringify(evidence.claims[supportToken])}, not ${JSON.stringify(expectedClaim)}`,
        ));
      }
    }
  }
}

function findOverlaps(ranges) {
  const sorted = [...ranges].sort((left, right) => left.start - right.start || left.end - right.end);
  const overlaps = [];
  for (let leftIndex = 0; leftIndex < sorted.length; leftIndex += 1) {
    const left = sorted[leftIndex];
    for (let rightIndex = leftIndex + 1; rightIndex < sorted.length; rightIndex += 1) {
      const right = sorted[rightIndex];
      if (right.start >= left.end) break;
      overlaps.push([left, right]);
    }
  }
  return overlaps;
}

function validateCorpus(catalog, evidenceById, issues) {
  const workbenchPath = path.join(catalog.corpus.workbenchPath ?? '', 'workbench.json');
  let workbench;
  try {
    workbench = readJson(resolveRepositoryPath(workbenchPath));
  } catch (error) {
    issues.push(makeIssue('CORPUS_MANIFEST', 'corpus.workbenchPath', error.message));
    return;
  }
  const expectedIdentity = [
    ['provenanceBuild', workbench.target?.build],
    ['imageBase', workbench.target?.imageBase],
    ['canonicalImageSha256', workbench.canonicalImage?.sha256],
    ['analysisImageSha256', workbench.analysisImage?.sha256],
  ];
  for (const [field, expected] of expectedIdentity) {
    if (catalog.corpus[field] !== expected) {
      issues.push(makeIssue('CORPUS_IDENTITY', `corpus.${field}`, `must match workbench manifest value ${expected}`));
    }
  }

  const seenBuilds = new Set();
  for (const coverage of catalog.corpus.coveredBuilds ?? []) {
    const location = `corpus.coveredBuilds.${coverage.build}`;
    if (seenBuilds.has(coverage.build)) {
      issues.push(makeIssue('DUPLICATE_BUILD', location, `duplicate covered build: ${coverage.build}`));
    }
    seenBuilds.add(coverage.build);
    const supportToken = `coverage:${coverage.build}`;
    for (const evidenceId of coverage.evidenceIds ?? []) {
      const evidence = evidenceById.get(evidenceId);
      if (!evidence) {
        issues.push(makeIssue('EVIDENCE_REFERENCE', `${location}.evidenceIds`, `unknown evidence id: ${evidenceId}`));
      } else if (!evidence.supports.includes(supportToken)) {
        issues.push(makeIssue('BUILD_GENERALIZATION', location, `${evidenceId} does not justify ${supportToken}`));
      } else if (evidence.claims[supportToken] !== true) {
        issues.push(makeIssue('BUILD_GENERALIZATION', location, `${evidenceId} does not claim byte-exact coverage`));
      }
    }
  }
  if (!seenBuilds.has(catalog.corpus.provenanceBuild)) {
    issues.push(makeIssue('CORPUS_COVERAGE', 'corpus.provenanceBuild', 'provenance build is not covered'));
  }
  if (!seenBuilds.has(catalog.targetRuntime.build)) {
    issues.push(makeIssue('BUILD_GENERALIZATION', 'targetRuntime.build', 'target runtime lacks byte-exact coverage evidence'));
  }
}

function validateDataTables(catalog, evidenceById, issues) {
  const expectedClaims = {
    'datatables:accessor': catalog.dataTables.accessorRva,
    'datatables:contexts': catalog.dataTables.contextCount,
    'datatables:entry-stride': catalog.dataTables.globalEntryStride,
  };
  const supportTokens = Object.keys(expectedClaims);
  for (const evidenceId of catalog.dataTables.evidenceIds ?? []) {
    const evidence = evidenceById.get(evidenceId);
    if (!evidence) {
      issues.push(makeIssue('EVIDENCE_REFERENCE', 'dataTables.evidenceIds', `unknown evidence id: ${evidenceId}`));
      continue;
    }
    for (const token of supportTokens) {
      if (!evidence.supports.includes(token)) {
        issues.push(makeIssue('EVIDENCE_SUPPORT', 'dataTables.evidenceIds', `${evidenceId} does not support ${token}`));
      } else if (!claimEquals(expectedClaims[token], evidence.claims[token])) {
        issues.push(makeIssue(
          'EVIDENCE_CLAIM',
          'dataTables.evidenceIds',
          `${evidenceId} claims ${JSON.stringify(evidence.claims[token])}, not ${JSON.stringify(expectedClaims[token])}`,
        ));
      }
    }
  }
  if ((catalog.dataTables.evidenceIds ?? []).length === 0) {
    issues.push(makeIssue('FACT_EVIDENCE', 'dataTables.evidenceIds', 'DataTables container contract requires evidence'));
  }
}

function validateTables(catalog, evidenceById, issues) {
  const tableById = new Map();
  for (const table of catalog.tables ?? []) {
    if (tableById.has(table.id)) {
      issues.push(makeIssue('DUPLICATE_TABLE', `tables.${table.id}`, `duplicate table id: ${table.id}`));
    }
    tableById.set(table.id, table);
  }
  const expected = new Set(catalog.scope.expectedTableIds ?? []);
  for (const tableId of expected) {
    if (!tableById.has(tableId)) issues.push(makeIssue('MISSING_TABLE', 'tables', `missing expected table: ${tableId}`));
  }
  for (const tableId of tableById.keys()) {
    if (!expected.has(tableId)) issues.push(makeIssue('UNSCOPED_TABLE', `tables.${tableId}`, 'table is not declared in scope.expectedTableIds'));
  }

  const slots = [];
  for (const table of catalog.tables ?? []) {
    for (const [factName, supportName] of [
      ['records', 'records'],
      ['count', 'count'],
      ['recordSize', 'record-size'],
      ['linker', 'linker'],
    ]) {
      if (!table[factName]) continue;
      validateFact({
        fact: table[factName],
        location: `tables.${table.id}.${factName}`,
        supportToken: `${table.id}:${supportName}`,
        evidenceById,
        issues,
      });
    }

    for (const slotName of ['records', 'count', 'linker']) {
      const fact = table[slotName];
      if (!fact || !concreteStatuses.has(fact.status)) continue;
      const start = parseHex(fact.value);
      if (start !== null) slots.push({ start, end: start + slotSize, label: `${table.id}.${slotName}` });
    }

    const recordSize = concreteStatuses.has(table.recordSize?.status)
      ? parseHex(table.recordSize.value)
      : null;
    const fields = [];
    const fieldIds = new Set();
    for (const field of table.recordFields ?? []) {
      if (fieldIds.has(field.id)) {
        issues.push(makeIssue('DUPLICATE_FIELD', `tables.${table.id}.recordFields.${field.id}`, `duplicate field id: ${field.id}`));
      }
      fieldIds.add(field.id);
      validateField(table, field, evidenceById, issues);
      const start = parseHex(field.offset);
      if (start === null) continue;
      const end = start + field.size;
      fields.push({ start, end, label: `${table.id}.${field.id}` });
      if (recordSize !== null && end > recordSize) {
        issues.push(makeIssue(
          'FIELD_OUTSIDE_RECORD',
          `tables.${table.id}.recordFields.${field.id}`,
          `field ends at 0x${end.toString(16).toUpperCase()} beyond record size ${table.recordSize.value}`,
        ));
      }
    }
    for (const [left, right] of findOverlaps(fields)) {
      issues.push(makeIssue('FIELD_OVERLAP', `tables.${table.id}.recordFields`, `${left.label} overlaps ${right.label}`));
    }
    validatePostProcessing(table, evidenceById, issues);
  }
  for (const [left, right] of findOverlaps(slots)) {
    issues.push(makeIssue('SLOT_OVERLAP', 'tables', `${left.label} overlaps ${right.label}`));
  }
}

export function validateCatalog(catalog, options = {}) {
  const schemaPath = options.schemaPath ?? defaultSchemaPath;
  const issues = [];
  let schema;
  try {
    schema = readJson(schemaPath);
  } catch (error) {
    return { valid: false, issues: [makeIssue('SCHEMA_READ', 'schema', error.message)], summary: null };
  }

  const ajv = new Ajv({ allErrors: true, strict: true });
  const validateSchema = ajv.compile(schema);
  if (!validateSchema(catalog)) {
    for (const error of validateSchema.errors ?? []) {
      issues.push(makeIssue(
        'SCHEMA',
        error.instancePath || '/',
        error.message ?? 'schema validation failed',
      ));
    }
  }
  if (issues.length > 0) {
    issues.sort(compareIssues);
    return { valid: false, issues, summary: null };
  }

  const evidenceById = new Map();
  for (const evidence of catalog.evidence) {
    if (evidenceById.has(evidence.id)) {
      issues.push(makeIssue('DUPLICATE_EVIDENCE', `evidence.${evidence.id}`, `duplicate evidence id: ${evidence.id}`));
    }
    evidenceById.set(evidence.id, evidence);
  }

  const cache = new Map();
  for (const evidence of catalog.evidence) {
    issues.push(...validateEvidenceSource(evidence, cache));
  }
  validateCorpus(catalog, evidenceById, issues);
  validateDataTables(catalog, evidenceById, issues);
  validateTables(catalog, evidenceById, issues);

  issues.sort(compareIssues);
  const provenTriplets = catalog.tables.filter((table) => (
    table.records.status === 'proven'
    && table.count.status === 'proven'
    && table.recordSize.status === 'proven'
  )).length;
  const summary = {
    tableCount: catalog.tables.length,
    provenTriplets,
    evidenceCount: catalog.evidence.length,
    coveredBuilds: catalog.corpus.coveredBuilds.map((entry) => entry.build).sort((a, b) => a - b),
  };
  return { valid: issues.length === 0, issues, summary };
}

export function loadAndValidateCatalog(catalogPath = defaultCatalogPath, options = {}) {
  let catalog;
  try {
    catalog = readJson(catalogPath);
  } catch (error) {
    return {
      valid: false,
      issues: [makeIssue('CATALOG_READ', 'catalog', error.message)],
      summary: null,
    };
  }
  return validateCatalog(catalog, options);
}

function main() {
  const args = process.argv.slice(2);
  const jsonOutput = args.includes('--json');
  const positional = args.filter((arg) => arg !== '--json');
  const catalogPath = positional[0] ? path.resolve(process.cwd(), positional[0]) : defaultCatalogPath;
  const result = loadAndValidateCatalog(catalogPath);
  if (jsonOutput) {
    console.log(JSON.stringify(result, null, 2));
  } else if (result.valid) {
    const relative = path.relative(repositoryRoot, catalogPath);
    console.log(
      `VALID D2R 3.3 DataTables atlas: ${relative}; `
        + `${result.summary.provenTriplets}/${result.summary.tableCount} triplets proven; `
        + `${result.summary.evidenceCount} evidence records; `
        + `builds ${result.summary.coveredBuilds.join(', ')}.`,
    );
  } else {
    console.error(`INVALID D2R 3.3 DataTables atlas: ${path.relative(repositoryRoot, catalogPath)}`);
    for (const issue of result.issues) {
      console.error(`- [${issue.code}] ${issue.location}: ${issue.message}`);
    }
    process.exitCode = 1;
  }
}

if (import.meta.url === pathToFileURL(process.argv[1] ?? '').href) main();
