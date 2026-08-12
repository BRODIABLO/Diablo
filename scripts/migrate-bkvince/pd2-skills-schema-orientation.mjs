import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import zlib from 'node:zlib';
import { fileURLToPath } from 'node:url';

import {
  DEFAULT_SOURCE_ROOTS,
  buildOracleData,
  loadWorkbenchSources,
  repoRoot,
} from './pd2-skills-review-data.mjs';
import { sha256Canonical } from './pd2-skills-review-contracts.mjs';
import {
  buildFieldDictionary,
  buildPolicyEnvelope,
  buildPolicySchema,
  buildSchemaOrientationData,
  serializeJson,
  serializeOrientationMarkdown,
} from './pd2-skills-schema-orientation-data.mjs';
import { buildSchemaOrientationHtml } from './pd2-skills-schema-orientation-ui.mjs';
import { buildBrowserPolicyRuntimeSource } from './pd2-skills-schema-policy-runtime.mjs';

export const ORIENTATION_ARTIFACT_PATHS = Object.freeze({
  orientationJson: path.join(repoRoot, 'Mission', 'pd2-skills-schema-orientation.json'),
  orientationMarkdown: path.join(repoRoot, 'Mission', 'pd2-skills-schema-orientation.md'),
  orientationHtml: path.join(repoRoot, 'Mission', 'pd2-skills-schema-orientation.html'),
  fieldDictionary: path.join(repoRoot, 'Mission', 'pd2-skills-field-dictionary.json'),
  policySchema: path.join(repoRoot, 'Mission', 'pd2-skills-schema-policy.schema.json'),
  policyExample: path.join(repoRoot, 'Mission', 'pd2-skills-schema-policy.example.json'),
  policyCurrent: path.join(repoRoot, 'Mission', 'pd2-skills-schema-policy.json'),
});

const REFERENCE_PATHS = Object.freeze({
  skillsSchema: path.join(repoRoot, 'schemas', 'skills.json'),
  analyticalAudit: path.join(repoRoot, 'Mission', 'pd2-skills-vs-bkvince-full-audit.md'),
  nativeFindings: path.join(repoRoot, 'reverse-engineering', 'd2r-3.2.92777', 'findings.md'),
  knownRvas: path.join(repoRoot, 'reverse-engineering', 'd2r-3.2.92777', 'known-rvas.json'),
});

function sha256(buffer) {
  return crypto.createHash('sha256').update(buffer).digest('hex').toUpperCase();
}

export function compressSchemaOrientationForHtml(orientation) {
  const compact = Buffer.from(JSON.stringify(orientation), 'utf8');
  const compressed = zlib.gzipSync(compact, { level: 9, mtime: 0 });
  // RFC 1952 byte 9 is only an OS hint. Normalize it so the standalone
  // artifact is byte-identical on Windows and Linux CI.
  compressed[9] = 0xFF;
  return compressed;
}

function governedPath(filePath) {
  return path.relative(repoRoot, filePath).replaceAll('\\', '/');
}

function readJson(filePath) {
  const raw = fs.readFileSync(filePath);
  return JSON.parse(raw.toString('utf8').replace(/^\uFEFF/, ''));
}

export function loadOrientationReferences(referencePaths = REFERENCE_PATHS) {
  const references = {};
  for (const [id, filePath] of Object.entries(referencePaths)) {
    const raw = fs.readFileSync(filePath);
    references[id] = {
      path: governedPath(filePath),
      sha256: sha256(raw),
      bytes: raw.length,
      role: id === 'skillsSchema'
        ? 'Tracked reproducible snapshot of eezstreet/d2rdoc Skills.txt documentation.'
        : id === 'analyticalAudit'
          ? 'Historical analytical baseline; never parsed as the skills value database.'
          : 'Governed D2R 3.2 native evidence; only explicitly promoted findings close a proof gate.',
      ...(id === 'skillsSchema' ? { document: readJson(filePath) } : {}),
    };
  }
  return references;
}

function assertCanonicalPd2SkillsPath(sources) {
  const manifest = sources.sourceManifest?.pd2?.tables?.['skills.txt'];
  if (!manifest) throw new Error('PD2 Skills.txt manifest is missing');
  if (path.basename(manifest.path) !== 'Skills.txt') {
    throw new Error(`PD2 canonical source must resolve to case-sensitive Skills.txt, got ${manifest.path}`);
  }
}

export function buildWorkbenchOrientationBinding(baseReport, orientation) {
  if (!baseReport?.comparisonHash || !orientation?.orientationHash) {
    throw new Error('A base workbench report and governed orientation are required');
  }
  const sourceManifest = {
    ...baseReport.sourceManifest,
    schemaOrientationReferences: orientation.sourceManifest.references,
  };
  const sourceHashes = {
    ...baseReport.sourceHashes,
    schemaOrientationReferences: orientation.sourceHashes.references,
  };
  const policyHashes = {
    ...baseReport.policyHashes,
    schemaOrientationContract: orientation.frozenContractHash,
    schemaOrientation: orientation.orientationHash,
  };
  const comparisonHash = sha256Canonical({
    composition: 'PD2_SKILLS_WORKBENCH_PLUS_SCHEMA_ORIENTATION_V1',
    baseComparisonHash: baseReport.comparisonHash,
    sourceHashes,
    policyHashes,
  });
  return { sourceManifest, sourceHashes, policyHashes, comparisonHash };
}

export function buildSchemaOrientationArtifacts(
  roots = DEFAULT_SOURCE_ROOTS,
  options = {},
) {
  const sources = options.sources ?? loadWorkbenchSources(roots);
  assertCanonicalPd2SkillsPath(sources);
  const skillReport = options.skillReport ?? buildOracleData(sources);
  const references = options.references ?? loadOrientationReferences();
  let orientation;
  if (skillReport.schemaOrientation) {
    for (const [id, reference] of Object.entries(references)) {
      const attachedHash = skillReport.schemaOrientation.sourceHashes?.references?.[id];
      if (attachedHash !== reference.sha256) {
        throw new Error(`Attached Phase 0 orientation has stale ${id} hash`);
      }
    }
    orientation = {
      ...skillReport.schemaOrientation,
      workbenchBinding: {
        reviewId: skillReport.reviewId,
        comparisonHash: skillReport.comparisonHash,
      },
    };
  } else {
    orientation = buildSchemaOrientationData(sources, skillReport, {
      references,
      workbenchBinding: null,
    });
    const binding = buildWorkbenchOrientationBinding(skillReport, orientation);
    orientation = {
      ...orientation,
      workbenchBinding: {
        reviewId: skillReport.reviewId,
        comparisonHash: binding.comparisonHash,
      },
    };
  }
  const dictionary = buildFieldDictionary(orientation);
  const policySchema = buildPolicySchema(orientation);
  const policyExample = buildPolicyEnvelope(orientation);
  const policyCurrent = buildPolicyEnvelope(orientation);
  const policyRuntimeSource = buildBrowserPolicyRuntimeSource();
  const orientationHtml = buildSchemaOrientationHtml(orientation, {
    policyRuntimeSource,
    workbenchBinding: orientation.workbenchBinding,
    compressedOracleBase64: compressSchemaOrientationForHtml(orientation).toString('base64'),
  });
  return {
    sources,
    skillReport,
    orientation,
    dictionary,
    policySchema,
    policyExample,
    policyCurrent,
    artifacts: {
      orientationJson: serializeJson(orientation),
      orientationMarkdown: serializeOrientationMarkdown(orientation),
      orientationHtml,
      fieldDictionary: serializeJson(dictionary),
      policySchema: serializeJson(policySchema),
      policyExample: serializeJson(policyExample),
      policyCurrent: serializeJson(policyCurrent),
    },
  };
}

function verifyArtifactTargets() {
  const missionRoot = path.resolve(repoRoot, 'Mission');
  for (const filePath of Object.values(ORIENTATION_ARTIFACT_PATHS)) {
    const resolved = path.resolve(filePath);
    if (path.dirname(resolved) !== missionRoot) {
      throw new Error(`Refusing non-documentary output outside Mission/: ${resolved}`);
    }
  }
}

export function writeSchemaOrientationArtifacts(generated) {
  verifyArtifactTargets();
  for (const [id, filePath] of Object.entries(ORIENTATION_ARTIFACT_PATHS)) {
    fs.writeFileSync(filePath, generated.artifacts[id], 'utf8');
  }
}

export function checkSchemaOrientationArtifacts(generated) {
  verifyArtifactTargets();
  const mismatches = [];
  for (const [id, filePath] of Object.entries(ORIENTATION_ARTIFACT_PATHS)) {
    if (!fs.existsSync(filePath)) {
      mismatches.push(`${governedPath(filePath)} is missing`);
      continue;
    }
    const actual = fs.readFileSync(filePath, 'utf8');
    if (actual !== generated.artifacts[id]) mismatches.push(`${governedPath(filePath)} is stale`);
  }
  if (mismatches.length) throw new Error(`Phase 0 generated artifacts differ:\n- ${mismatches.join('\n- ')}`);
  return true;
}

export function generateSchemaOrientation(
  roots = DEFAULT_SOURCE_ROOTS,
  options = {},
) {
  const generated = buildSchemaOrientationArtifacts(roots, options);
  if (options.check) checkSchemaOrientationArtifacts(generated);
  else writeSchemaOrientationArtifacts(generated);
  return generated;
}

const isMain = process.argv[1]
  && path.resolve(process.argv[1]) === path.resolve(fileURLToPath(import.meta.url));

if (isMain) {
  const check = process.argv.includes('--check');
  const generated = generateSchemaOrientation(DEFAULT_SOURCE_ROOTS, { check });
  process.stdout.write(`${check ? 'VALID' : 'GENERATED'} ${generated.orientation.orientationHash}\n`);
  process.stdout.write(`columns=${generated.orientation.coverage.canonicalHeaders} fireBoltDecisions=${generated.orientation.fireBoltImpact.finalPlayerDecisions}\n`);
}
