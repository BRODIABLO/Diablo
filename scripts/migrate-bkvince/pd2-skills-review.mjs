import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import {
  FROZEN_CONTRACT_HASH,
  ORACLE_SCHEMA_VERSION,
  REVIEW_ID,
  canonicalize,
} from './pd2-skills-review-contracts.mjs';
import { generateOracleData } from './pd2-skills-review-data.mjs';
import { buildSkillReviewHtml } from './pd2-skills-review-ui.mjs';
import { buildBrowserRuntimeSource } from './pd2-skills-review-runtime.mjs';

export const repoRoot = fileURLToPath(new URL('../../', import.meta.url));
export const OUTPUT_PATHS = Object.freeze({
  report: path.join(repoRoot, 'Mission', 'pd2-skills-review.json'),
  html: path.join(repoRoot, 'Mission', 'pd2-skills-review.html'),
  documentation: path.join(repoRoot, 'Mission', 'pd2-skills-documentation-map.json'),
});

function sha256(value) {
  return crypto.createHash('sha256').update(value).digest('hex').toUpperCase();
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
  const report = generateOracleData(options.roots);
  assertOracle(report);
  const documentation = buildDocumentationMap(report);
  const runtimeSource = buildBrowserRuntimeSource();
  const html = buildSkillReviewHtml(report, runtimeSource);
  const raw = Object.freeze({
    report: `${JSON.stringify(report, null, 2)}\n`,
    documentation: `${JSON.stringify(documentation, null, 2)}\n`,
    html,
  });
  const hashes = Object.freeze(Object.fromEntries(
    Object.entries(raw).map(([key, value]) => [key, sha256(value)]),
  ));
  return { report, documentation, runtimeSource, raw, hashes };
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
  const checks = {
    report: compareFile(OUTPUT_PATHS.report, generated.raw.report),
    html: compareFile(OUTPUT_PATHS.html, generated.raw.html),
    documentation: compareFile(OUTPUT_PATHS.documentation, generated.raw.documentation),
  };
  const stale = Object.entries(checks).filter(([, check]) => !check.matches);
  if (stale.length) {
    throw new Error(`stale PD2 skill review artifact(s): ${stale.map(([name]) => name).join(', ')}`);
  }
  return checks;
}

export function writeSkillReviewArtifacts(generated = generateSkillReviewArtifacts()) {
  fs.writeFileSync(OUTPUT_PATHS.report, generated.raw.report, 'utf8');
  fs.writeFileSync(OUTPUT_PATHS.documentation, generated.raw.documentation, 'utf8');
  fs.writeFileSync(OUTPUT_PATHS.html, generated.raw.html, 'utf8');
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
