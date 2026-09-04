import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import Ajv from 'ajv/dist/2020.js';
import addFormats from 'ajv-formats';

const SCRIPT_PATH = fileURLToPath(import.meta.url);
const REPO_ROOT = path.resolve(path.dirname(SCRIPT_PATH), '..', '..');
const DEFAULT_REGISTRY_PATH = path.join(
  REPO_ROOT,
  'reverse-engineering',
  'd2rloader-baselines.json',
);
const DEFAULT_SCHEMA_PATH = path.join(
  REPO_ROOT,
  'reverse-engineering',
  'd2rloader-baselines.schema.json',
);
const REQUIRED_ARTIFACTS = ['D2RLoader.exe', 'D2RCore.dll', 'd2rloader.mpq'];
const REQUIRED_PROMOTION_GATES = [
  'sourceVerified',
  'artifactIntegrity',
  'sdkAudit',
  'contractAudit',
  'staticCompatibility',
  'runtimeQualification',
  'fullStackCoexistence',
];
const GATE_STATUSES = new Set([
  'not-run',
  'passed',
  'failed',
  'blocked',
  'not-required',
]);

function clone(value) {
  return structuredClone(value);
}

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

function today() {
  return new Date().toISOString().slice(0, 10);
}

function normalizePath(filePath) {
  return filePath.split(path.sep).join('/');
}

function formatAjvErrors(errors = []) {
  return errors
    .map((error) => `${error.instancePath || '/'} ${error.message}`)
    .join('; ');
}

export function validateRegistry(registry, schema = readJson(DEFAULT_SCHEMA_PATH)) {
  const ajv = new Ajv({ allErrors: true, strict: true });
  addFormats(ajv);
  const validate = ajv.compile(schema);
  if (!validate(registry)) {
    throw new Error(`D2RLoader baseline schema violation: ${formatAjvErrors(validate.errors)}`);
  }

  const errors = [];
  const ids = new Set();
  for (const baseline of registry.baselines) {
    if (ids.has(baseline.id)) errors.push(`duplicate baseline id ${baseline.id}`);
    ids.add(baseline.id);

    const artifactNames = baseline.artifacts.map((artifact) => artifact.name);
    if (new Set(artifactNames).size !== artifactNames.length) {
      errors.push(`${baseline.id} has duplicate artifact names`);
    }

    const apiVersions = baseline.sdk.pins.map((pin) => pin.apiVersion);
    if (new Set(apiVersions).size !== apiVersions.length) {
      errors.push(`${baseline.id} has duplicate PluginSDK API pins`);
    }

    for (const [gateName, gate] of Object.entries(baseline.gates)) {
      if (gate.status === 'passed' && gate.evidence.length === 0) {
        errors.push(`${baseline.id} gate ${gateName} passed without evidence`);
      }
    }

    if (baseline.stage === 'promoted') {
      const missingArtifacts = REQUIRED_ARTIFACTS.filter(
        (name) => !artifactNames.includes(name),
      );
      if (missingArtifacts.length > 0 || artifactNames.length !== REQUIRED_ARTIFACTS.length) {
        errors.push(`${baseline.id} promoted without the exact three loader artifacts`);
      }
      if (baseline.sdk.pins.length === 0) {
        errors.push(`${baseline.id} promoted without a governed PluginSDK pin`);
      }
      if (!baseline.contracts.reviewed) {
        errors.push(`${baseline.id} promoted without a reviewed loader contract`);
      }
      if (baseline.promotedAt === null) {
        errors.push(`${baseline.id} promoted without promotedAt`);
      }
      for (const gateName of REQUIRED_PROMOTION_GATES) {
        if (baseline.gates[gateName].status !== 'passed') {
          errors.push(`${baseline.id} promoted while gate ${gateName} is not passed`);
        }
      }
    }
  }

  const promoted = registry.baselines.filter((baseline) => baseline.stage === 'promoted');
  if (promoted.length !== 1) {
    errors.push(`expected exactly one promoted baseline, found ${promoted.length}`);
  } else if (promoted[0].id !== registry.promotedBaselineId) {
    errors.push(
      `promotedBaselineId ${registry.promotedBaselineId} does not match ${promoted[0].id}`,
    );
  }
  if (!ids.has(registry.promotedBaselineId)) {
    errors.push(`promotedBaselineId ${registry.promotedBaselineId} does not exist`);
  }

  if (errors.length > 0) {
    throw new Error(`D2RLoader baseline semantic violation: ${errors.join('; ')}`);
  }
  return registry;
}

export function loadRegistry(
  registryPath = DEFAULT_REGISTRY_PATH,
  schemaPath = DEFAULT_SCHEMA_PATH,
) {
  return validateRegistry(readJson(registryPath), readJson(schemaPath));
}

function saveRegistry(registry, registryPath, schemaPath = DEFAULT_SCHEMA_PATH) {
  registry.generatedAt = new Date().toISOString();
  validateRegistry(registry, readJson(schemaPath));
  fs.writeFileSync(registryPath, `${JSON.stringify(registry, null, 2)}\n`, 'utf8');
}

function slug(value) {
  return value
    .toLowerCase()
    .replace(/[^a-z0-9.-]+/g, '-')
    .replace(/^-+|-+$/g, '');
}

function emptyGate() {
  return { status: 'not-run', evidence: [] };
}

export function createAnnouncement(registry, {
  version,
  channel = 'public',
  sourceUrl,
  observedAt = today(),
  id = `d2rloader-${slug(version)}-${slug(channel)}`,
}) {
  if (!version || !sourceUrl) {
    throw new Error('announce requires a version and an official source URL');
  }
  const next = clone(registry);
  if (next.baselines.some((baseline) => baseline.id === id)) {
    throw new Error(`baseline ${id} already exists`);
  }
  next.baselines.push({
    id,
    version,
    channel,
    stage: 'announced',
    observedAt,
    promotedAt: null,
    supersededAt: null,
    source: {
      kind: 'user-announcement',
      url: sourceUrl,
      verifiedAt: null,
      evidence: ['Vincent announced the release; official provenance remains to be verified.'],
    },
    artifacts: [],
    sdk: {
      selectionPolicy: 'minimum-required-capability',
      pins: [],
    },
    contracts: {
      reviewed: false,
      pluginManifestVersion: null,
      requiredExports: [],
      runtimeSelectionPolicy: 'native-fingerprint-only',
    },
    impactReview: [],
    gates: {
      sourceVerified: emptyGate(),
      artifactIntegrity: emptyGate(),
      sdkAudit: emptyGate(),
      contractAudit: emptyGate(),
      staticCompatibility: emptyGate(),
      runtimeQualification: emptyGate(),
      fullStackCoexistence: emptyGate(),
      multiplayer: emptyGate(),
    },
    notes: [
      'The previous promoted baseline remains authoritative until every promotion gate passes.',
      'Loader version and channel are diagnostic metadata and never authorize or reject plugin loading.',
    ],
  });
  return next;
}

async function sha256(filePath) {
  const hash = crypto.createHash('sha256');
  const stream = fs.createReadStream(filePath);
  for await (const chunk of stream) hash.update(chunk);
  return hash.digest('hex').toUpperCase();
}

export async function captureArtifacts(registry, baselineId, artifactDirectory) {
  const next = clone(registry);
  const baseline = next.baselines.find((entry) => entry.id === baselineId);
  if (!baseline) throw new Error(`unknown baseline ${baselineId}`);

  const artifacts = [];
  for (const name of REQUIRED_ARTIFACTS) {
    const filePath = path.resolve(artifactDirectory, name);
    const stat = fs.statSync(filePath);
    if (!stat.isFile()) throw new Error(`${filePath} is not a file`);
    artifacts.push({
      name,
      size: stat.size,
      sha256: await sha256(filePath),
      fileVersion: null,
      evidencePath: normalizePath(path.relative(REPO_ROOT, filePath)),
    });
  }
  baseline.artifacts = artifacts;
  baseline.notes.push(
    'Artifact hashes were captured locally; source verification remains an independent gate.',
  );
  return next;
}

function recomputeStage(baseline) {
  if (baseline.stage === 'promoted' || baseline.stage === 'superseded') return;
  const gates = baseline.gates;
  if (
    gates.runtimeQualification.status === 'passed'
    && gates.fullStackCoexistence.status === 'passed'
  ) {
    baseline.stage = 'runtime-qualified';
  } else if (
    gates.sourceVerified.status === 'passed'
    && gates.artifactIntegrity.status === 'passed'
    && gates.sdkAudit.status === 'passed'
    && gates.contractAudit.status === 'passed'
    && gates.staticCompatibility.status === 'passed'
  ) {
    baseline.stage = 'audited';
  } else if (gates.sourceVerified.status === 'passed') {
    baseline.stage = 'source-verified';
  } else {
    baseline.stage = 'announced';
  }
}

export function setGate(registry, baselineId, gateName, status, evidence) {
  if (!GATE_STATUSES.has(status)) throw new Error(`invalid gate status ${status}`);
  const next = clone(registry);
  const baseline = next.baselines.find((entry) => entry.id === baselineId);
  if (!baseline) throw new Error(`unknown baseline ${baselineId}`);
  if (!(gateName in baseline.gates)) throw new Error(`unknown gate ${gateName}`);
  if (status === 'passed' && !evidence) {
    throw new Error(`gate ${gateName} cannot pass without evidence`);
  }
  baseline.gates[gateName] = {
    status,
    evidence: evidence ? [evidence] : [],
  };
  recomputeStage(baseline);
  return next;
}

export function promoteBaseline(registry, baselineId, promotedAt = today()) {
  const next = clone(registry);
  const candidate = next.baselines.find((entry) => entry.id === baselineId);
  if (!candidate) throw new Error(`unknown baseline ${baselineId}`);

  const missingGates = REQUIRED_PROMOTION_GATES.filter(
    (gateName) => candidate.gates[gateName].status !== 'passed',
  );
  if (missingGates.length > 0) {
    throw new Error(`cannot promote ${baselineId}; open gates: ${missingGates.join(', ')}`);
  }
  const artifactNames = new Set(candidate.artifacts.map((artifact) => artifact.name));
  const missingArtifacts = REQUIRED_ARTIFACTS.filter((name) => !artifactNames.has(name));
  if (missingArtifacts.length > 0 || candidate.artifacts.length !== REQUIRED_ARTIFACTS.length) {
    throw new Error(`cannot promote ${baselineId}; incomplete loader artifact set`);
  }
  if (candidate.sdk.pins.length === 0) {
    throw new Error(`cannot promote ${baselineId}; no governed PluginSDK pin`);
  }
  if (!candidate.contracts.reviewed) {
    throw new Error(`cannot promote ${baselineId}; loader contract is not reviewed`);
  }

  for (const baseline of next.baselines) {
    if (baseline.stage === 'promoted' && baseline.id !== baselineId) {
      baseline.stage = 'superseded';
      baseline.supersededAt = promotedAt;
    }
  }
  candidate.stage = 'promoted';
  candidate.promotedAt = promotedAt;
  candidate.supersededAt = null;
  next.promotedBaselineId = candidate.id;
  return next;
}

function parseArguments(args) {
  const positional = [];
  const options = {};
  for (let index = 0; index < args.length; index += 1) {
    const value = args[index];
    if (!value.startsWith('--')) {
      positional.push(value);
      continue;
    }
    const key = value.slice(2);
    const next = args[index + 1];
    if (!next || next.startsWith('--')) throw new Error(`missing value for --${key}`);
    options[key] = next;
    index += 1;
  }
  return { positional, options };
}

function printStatus(registry, json = false) {
  const promoted = registry.baselines.find(
    (baseline) => baseline.id === registry.promotedBaselineId,
  );
  const candidates = registry.baselines.filter(
    (baseline) => baseline.stage !== 'promoted' && baseline.stage !== 'superseded',
  );
  if (json) {
    console.log(JSON.stringify({ promoted, candidates }, null, 2));
    return;
  }
  console.log('D2RLoader baseline registry VALID');
  console.log(
    `promoted=${promoted.id} version=${promoted.version} channel=${promoted.channel}`,
  );
  console.log(
    `sdk=${promoted.sdk.pins.map((pin) => `api-v${pin.apiVersion}@${pin.commit.slice(0, 12)}`).join(',')}`,
  );
  console.log(`candidates=${candidates.length}`);
  for (const candidate of candidates) {
    const open = REQUIRED_PROMOTION_GATES.filter(
      (gateName) => candidate.gates[gateName].status !== 'passed',
    );
    console.log(
      `candidate=${candidate.id} stage=${candidate.stage} openGates=${open.join(',') || 'none'}`,
    );
  }
}

function usage() {
  console.log(`Usage:
  npm run baseline:d2rloader -- status [--json true]
  npm run baseline:d2rloader -- validate
  npm run baseline:d2rloader -- announce <version> --source-url <url> [--channel public] [--observed-at YYYY-MM-DD] [--id id]
  npm run baseline:d2rloader -- capture <id> --artifact-dir <path>
  npm run baseline:d2rloader -- gate <id> <gate> <status> [--evidence text]
  npm run baseline:d2rloader -- promote <id> [--promoted-at YYYY-MM-DD]

All mutating commands accept --registry <path>. Promotion never treats a version
number or distribution channel as runtime compatibility evidence.`);
}

async function main(args = process.argv.slice(2)) {
  const { positional, options } = parseArguments(args);
  const command = positional[0] || 'status';
  if (command === 'help') {
    usage();
    return;
  }
  const registryPath = path.resolve(options.registry || DEFAULT_REGISTRY_PATH);
  const registry = loadRegistry(registryPath);

  if (command === 'status') {
    printStatus(registry, options.json === 'true');
    return;
  }
  if (command === 'validate') {
    console.log(
      `VALID ${normalizePath(path.relative(REPO_ROOT, registryPath))} baselines=${registry.baselines.length}`,
    );
    return;
  }

  let next;
  if (command === 'announce') {
    next = createAnnouncement(registry, {
      version: positional[1],
      channel: options.channel || 'public',
      sourceUrl: options['source-url'],
      observedAt: options['observed-at'] || today(),
      id: options.id,
    });
  } else if (command === 'capture') {
    if (!positional[1] || !options['artifact-dir']) {
      throw new Error('capture requires a baseline id and --artifact-dir');
    }
    next = await captureArtifacts(registry, positional[1], options['artifact-dir']);
  } else if (command === 'gate') {
    const [, baselineId, gateName, status] = positional;
    if (!baselineId || !gateName || !status) {
      throw new Error('gate requires a baseline id, gate name and status');
    }
    next = setGate(registry, baselineId, gateName, status, options.evidence);
  } else if (command === 'promote') {
    if (!positional[1]) throw new Error('promote requires a baseline id');
    next = promoteBaseline(registry, positional[1], options['promoted-at'] || today());
  } else {
    throw new Error(`unknown command ${command}`);
  }

  saveRegistry(next, registryPath);
  console.log(`UPDATED ${normalizePath(path.relative(REPO_ROOT, registryPath))}`);
  printStatus(next);
}

if (process.argv[1] && path.resolve(process.argv[1]) === SCRIPT_PATH) {
  main().catch((error) => {
    console.error(`ERROR ${error.message}`);
    process.exitCode = 1;
  });
}
