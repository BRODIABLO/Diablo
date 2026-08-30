#!/usr/bin/env node

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const scriptDirectory = path.dirname(fileURLToPath(import.meta.url));
const repositoryRoot = path.resolve(scriptDirectory, '..', '..');
const defaultLedgerPath = path.join(
  repositoryRoot,
  'reverse-engineering',
  'd2r-3.2.92777',
  'isc12',
  'native-sites.json',
);

const allowedStatuses = new Set(['ready', 'identified', 'blocked', 'excluded']);
const requiredGroups = [
  'G0-loader-descfunc',
  'G1-generic-item-codec',
  'G2-aux-player-codec',
  'G3-player-save-codec',
  'G4-player-preview-codec',
  'G5-packet-3e',
  'G6-packet-a8',
  'G7-packet-aa',
  'G8-packet-ac',
  'G9-full-item-packets',
  'G10-format-gate',
  'X1-quantity-not-stat-id',
  'X2-state-id-not-stat-id',
  'X3-legacy-save-version',
];

function loadLedger(ledgerPath, errors) {
  let source;
  try {
    source = fs.readFileSync(ledgerPath, 'utf8');
  } catch (error) {
    errors.push(`cannot read ledger: ${error.message}`);
    return null;
  }

  try {
    return JSON.parse(source);
  } catch (error) {
    errors.push(`invalid JSON: ${error.message}`);
    return null;
  }
}

function validateFormatContract(contract, errors) {
  if (!contract || typeof contract !== 'object' || Array.isArray(contract)) {
    errors.push('formatContract must be an object');
    return;
  }

  const expected = {
    currentBits: 9,
    targetBits: 12,
    currentSentinel: 511,
    targetSentinel: 4095,
    currentUsableIds: '0..510',
    targetUsableIds: '0..4094',
  };

  for (const [field, value] of Object.entries(expected)) {
    if (contract[field] !== value) {
      errors.push(`formatContract.${field} must equal ${JSON.stringify(value)}`);
    }
  }

  if (Number.isInteger(contract.currentBits)
      && contract.currentSentinel !== (2 ** contract.currentBits) - 1) {
    errors.push('formatContract.currentSentinel does not match currentBits');
  }
  if (Number.isInteger(contract.targetBits)
      && contract.targetSentinel !== (2 ** contract.targetBits) - 1) {
    errors.push('formatContract.targetSentinel does not match targetBits');
  }
}

function isConcretePattern(pattern) {
  if (typeof pattern !== 'string' || pattern.trim() === '') return false;
  const tokens = pattern.trim().split(/\s+/u);
  return tokens.length > 0 && tokens.every((token) => /^(?:[0-9A-Fa-f]{2}|\?\?)$/u.test(token));
}

function validateSites(sites, errors) {
  if (!Array.isArray(sites) || sites.length === 0) {
    errors.push('sites must be a non-empty array');
    return { groupCounts: new Map(), siteCount: 0 };
  }

  const seenIds = new Set();
  const groupCounts = new Map();

  sites.forEach((site, index) => {
    const location = `sites[${index}]`;
    if (!site || typeof site !== 'object' || Array.isArray(site)) {
      errors.push(`${location} must be an object`);
      return;
    }

    if (typeof site.id !== 'string' || site.id.trim() === '') {
      errors.push(`${location}.id must be a non-empty string`);
    } else if (seenIds.has(site.id)) {
      errors.push(`duplicate site id: ${site.id}`);
    } else {
      seenIds.add(site.id);
    }

    if (typeof site.atomicGroup !== 'string' || site.atomicGroup.trim() === '') {
      errors.push(`${location}.atomicGroup must be a non-empty string`);
    } else {
      groupCounts.set(site.atomicGroup, (groupCounts.get(site.atomicGroup) ?? 0) + 1);
    }

    if (!allowedStatuses.has(site.status)) {
      errors.push(`${location}.status must be one of: ${[...allowedStatuses].join(', ')}`);
    }

    if (site.status === 'ready') {
      const signature = site.signature;
      if (!signature || typeof signature !== 'object' || Array.isArray(signature)) {
        errors.push(`${location} (${site.id ?? 'missing id'}) is ready without a signature object`);
        return;
      }
      if (signature.unique !== true) {
        errors.push(`${location} (${site.id ?? 'missing id'}) is ready but signature.unique is not true`);
      }
      if (!['exact', 'masked'].includes(signature.kind)) {
        errors.push(`${location} (${site.id ?? 'missing id'}) is ready with a non-applicable signature kind`);
      }
      if (!isConcretePattern(signature.pattern)) {
        errors.push(`${location} (${site.id ?? 'missing id'}) is ready without a concrete byte pattern`);
      }
    }
  });

  for (const group of requiredGroups) {
    if (!groupCounts.has(group)) errors.push(`missing required atomic group: ${group}`);
  }

  return { groupCounts, siteCount: sites.length };
}

function main() {
  const ledgerPath = process.argv[2]
    ? path.resolve(process.cwd(), process.argv[2])
    : defaultLedgerPath;
  const errors = [];
  const ledger = loadLedger(ledgerPath, errors);
  let summary = { groupCounts: new Map(), siteCount: 0 };

  if (ledger) {
    validateFormatContract(ledger.formatContract, errors);
    summary = validateSites(ledger.sites, errors);
  }

  if (errors.length > 0) {
    console.error(`INVALID ISC12 native-site ledger: ${path.relative(repositoryRoot, ledgerPath)}`);
    for (const error of errors) console.error(`- ${error}`);
    process.exitCode = 1;
    return;
  }

  console.log(
    `VALID ISC12 native-site ledger: ${summary.siteCount} sites, `
      + `${requiredGroups.length} required groups, contract 9->12, sentinels 511/4095.`,
  );
}

main();
