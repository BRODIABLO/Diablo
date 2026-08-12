import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import zlib from 'node:zlib';
import { fileURLToPath } from 'node:url';

import {
  FROZEN_CONTRACT_HASH,
  ORACLE_SCHEMA_VERSION,
  REVIEW_ID,
  canonicalize,
} from './pd2-skills-review-contracts.mjs';
import {
  DEFAULT_SOURCE_ROOTS,
  buildOracleData,
  loadWorkbenchSources,
} from './pd2-skills-review-data.mjs';
import { buildSkillReviewHtml } from './pd2-skills-review-ui.mjs';
import { buildBrowserRuntimeSource } from './pd2-skills-review-runtime.mjs';
import {
  ORIENTATION_ARTIFACT_PATHS,
  buildSchemaOrientationArtifacts,
  buildWorkbenchOrientationBinding,
} from './pd2-skills-schema-orientation.mjs';

export const repoRoot = fileURLToPath(new URL('../../', import.meta.url));
export const OUTPUT_PATHS = Object.freeze({
  report: path.join(repoRoot, 'Mission', 'pd2-skills-review.json'),
  html: path.join(repoRoot, 'Mission', 'pd2-skills-review.html'),
  documentation: path.join(repoRoot, 'Mission', 'pd2-skills-documentation-map.json'),
  ...ORIENTATION_ARTIFACT_PATHS,
});

function sha256(value) {
  return crypto.createHash('sha256').update(value).digest('hex').toUpperCase();
}

export function compressOracleForHtml(report) {
  const compact = Buffer.from(JSON.stringify(report), 'utf8');
  const compressed = zlib.gzipSync(compact, { level: 9, mtime: 0 });
  // RFC 1952 byte 9 is only an OS hint and is not covered by the CRC. zlib
  // emits a platform-specific value, so normalize it for Windows/Linux CI.
  compressed[9] = 0xFF;
  return compressed;
}

function assertOracle(report) {
  if (report?.schemaVersion !== ORACLE_SCHEMA_VERSION) {
    throw new Error(`oracle schemaVersion must be ${ORACLE_SCHEMA_VERSION}`);
  }
  if (report.reviewId !== REVIEW_ID) throw new Error(`oracle reviewId must be ${REVIEW_ID}`);
  if (report.frozenContractHash !== FROZEN_CONTRACT_HASH) {
    throw new Error(`oracle contract hash ${report.frozenContractHash} does not match ${FROZEN_CONTRACT_HASH}`);
  }
  if (!/^[A-F0-9]{64}$/.test(report.comparisonHash ?? '')) {
    throw new Error('oracle comparisonHash must be an uppercase SHA-256');
  }
  if (!Array.isArray(report.nodes) || !Array.isArray(report.skills) || !Array.isArray(report.collisions)) {
    throw new Error('oracle must expose nodes, skills, and collisions arrays');
  }
  if (!report.schemaOrientation || report.schemaOrientation.orientationHash !== report.policyHashes?.schemaOrientation) {
    throw new Error('oracle must embed the governed Phase 0 schema orientation');
  }
  if (report.schemaOrientation.workbenchBinding?.comparisonHash !== report.comparisonHash) {
    throw new Error('schema orientation workbench binding must match the final comparisonHash');
  }
  const stableIds = new Set();
  for (const skill of report.skills) {
    if (!skill.stableId || stableIds.has(skill.stableId)) {
      throw new Error(`duplicate or missing stable skill id ${skill.stableId ?? '<missing>'}`);
    }
    stableIds.add(skill.stableId);
  }
  for (const view of report.navigation ?? []) {
    for (const stableId of view.skillIds ?? []) {
      if (!stableIds.has(stableId)) throw new Error(`navigation references unknown skill ${stableId}`);
    }
    for (const tree of view.trees ?? []) {
      for (const stableId of tree.skillIds ?? []) {
        if (!stableIds.has(stableId)) throw new Error(`tree navigation references unknown skill ${stableId}`);
      }
    }
  }
}

export function buildIntegratedWorkbenchReport(roots = DEFAULT_SOURCE_ROOTS) {
  const sources = loadWorkbenchSources(roots);
  const baseReport = buildOracleData(sources);
  const phase0 = buildSchemaOrientationArtifacts(roots, { sources, skillReport: baseReport });
  const orientation = phase0.orientation;
  const {
    sourceManifest,
    sourceHashes,
    policyHashes,
    comparisonHash,
  } = buildWorkbenchOrientationBinding(baseReport, orientation);
  const report = {
    ...baseReport,
    comparisonHash,
    sourceManifest,
    sourceHashes,
    policyHashes,
    schemaOrientation: {
      ...orientation,
      workbenchBinding: { reviewId: baseReport.reviewId, comparisonHash },
    },
  };
  return { report, orientationArtifacts: phase0 };
}

export function buildDocumentationMap(report) {
  return {
    schemaVersion: 1,
    reviewId: report.reviewId,
    comparisonHash: report.comparisonHash,
    frozenContractHash: report.frozenContractHash,
    sources: report.documentation?.sources
      ?? (report.documentation?.source ? [report.documentation.source] : []),
    policy: report.documentation?.policy ?? {
      documented: 'A pinned wiki statement is mapped to an exact skill/facet but is not native proof.',
      tableOnly: 'The governed tables show the fact without a matching pinned wiki assertion.',
      unmapped: 'The pinned wiki is retained as context without an exact table-to-claim mapping.',
    },
    entries: Object.fromEntries(report.skills.map((skill) => [skill.stableId, {
      fingerprint: skill.fingerprint,
      canonicalName: skill.canonicalName,
      references: skill.documentation ?? [],
    }])),
  };
}

export function generateSkillReviewArtifacts(options = {}) {
  const integrated = buildIntegratedWorkbenchReport(options.roots ?? DEFAULT_SOURCE_ROOTS);
  const { report, orientationArtifacts } = integrated;
  assertOracle(report);
  const documentation = buildDocumentationMap(report);
  const runtimeSource = buildBrowserRuntimeSource();
  const compressedOracle = compressOracleForHtml(report);
  const html = buildSkillReviewHtml(report, runtimeSource, {
    compressedOracleBase64: compressedOracle.toString('base64'),
  });
  const raw = Object.freeze({
    // Keep the governed oracle complete while avoiding more than 50 MiB of
    // deterministic presentation whitespace in the checked-in artifact.
    report: `${JSON.stringify(report)}\n`,
    documentation: `${JSON.stringify(documentation, null, 2)}\n`,
    html,
    ...orientationArtifacts.artifacts,
  });
  const hashes = Object.freeze(Object.fromEntries(
    Object.entries(raw).map(([key, value]) => [key, sha256(value)]),
  ));
  return { report, documentation, runtimeSource, orientationArtifacts, raw, hashes };
}

function compareFile(filePath, expected) {
  if (!fs.existsSync(filePath)) return { current: null, expected: sha256(expected), matches: false };
  const current = fs.readFileSync(filePath);
  return {
    current: sha256(current),
    expected: sha256(expected),
    matches: current.equals(Buffer.from(expected, 'utf8')),
  };
}

export function checkSkillReviewArtifacts(generated = generateSkillReviewArtifacts()) {
  const checks = Object.fromEntries(Object.entries(generated.raw).map(([id, value]) => [
    id,
    compareFile(OUTPUT_PATHS[id], value),
  ]));
  const stale = Object.entries(checks).filter(([, check]) => !check.matches);
  if (stale.length) {
    throw new Error(`stale PD2 skill review artifact(s): ${stale.map(([name]) => name).join(', ')}`);
  }
  return checks;
}

export function writeSkillReviewArtifacts(generated = generateSkillReviewArtifacts()) {
  for (const [id, value] of Object.entries(generated.raw)) {
    const filePath = OUTPUT_PATHS[id];
    if (!filePath) throw new Error(`No governed output path for ${id}`);
    fs.writeFileSync(filePath, value, 'utf8');
  }
  return generated.hashes;
}

function main(args = process.argv.slice(2)) {
  const unknown = args.filter((argument) => argument !== '--check');
  if (unknown.length) throw new Error(`unknown argument(s): ${unknown.join(', ')}`);
  const generated = generateSkillReviewArtifacts();
  if (args.includes('--check')) {
    checkSkillReviewArtifacts(generated);
    console.log(`VALID PD2 Skills Merge Workbench ${generated.report.comparisonHash}`);
  } else {
    writeSkillReviewArtifacts(generated);
    console.log(`GENERATED PD2 Skills Merge Workbench ${generated.report.comparisonHash}`);
  }
  console.log(`contract=${FROZEN_CONTRACT_HASH}`);
  console.log(`coverage=${canonicalize(generated.report.coverage)}`);
  for (const [name, hash] of Object.entries(generated.hashes)) console.log(`${name}Sha256=${hash}`);
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try {
    main();
  } catch (error) {
    console.error(`INVALID PD2 Skills Merge Workbench: ${error.message}`);
    process.exitCode = 1;
  }
}
