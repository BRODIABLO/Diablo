import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';
import { fileURLToPath } from 'node:url';

const repoRoot = fileURLToPath(new URL('../../', import.meta.url));
const reportPath = path.join(repoRoot, 'Mission', 'pd2-affixes-review.json');
const htmlPath = path.join(repoRoot, 'Mission', 'pd2-affixes-review.html');
const catalog = JSON.parse(fs.readFileSync(path.join(repoRoot, 'Mission', 'pd2-affixes-merge.catalog.json'), 'utf8'));
const report = JSON.parse(fs.readFileSync(reportPath, 'utf8'));

function sha256(filePath) {
  return crypto.createHash('sha256').update(fs.readFileSync(filePath)).digest('hex').toUpperCase();
}

test('review artifacts are pinned and contain every governed comparison class', () => {
  assert.equal(report.state, 'review_only_no_import_approved');
  assert.equal(report.entries.length, 2117);
  assert.deepEqual(report.counts, catalog.review.counts);
  assert.equal(sha256(reportPath), catalog.review.jsonSha256);
  assert.equal(sha256(htmlPath), catalog.review.htmlSha256);
  for (const status of [
    'shared',
    'pd2_modified',
    'pd2_deleted',
    'pd2_new_portable',
    'pd2_new_review',
    'bkv_only',
  ]) {
    assert.ok(report.entries.some((entry) => entry.status === status), `missing ${status}`);
  }
});

test('technically portable rows remain unapproved and PD2 deletions are explicit', () => {
  const portable = report.entries.filter((entry) => entry.status === 'pd2_new_portable');
  assert.equal(portable.length, 200);
  assert.ok(portable.every((entry) => entry.defaultDecision === 'undecided'));
  const retunes = report.entries.filter((entry) => entry.status === 'retune_candidate');
  assert.equal(retunes.length, 71);
  assert.ok(retunes.every((entry) => entry.defaultDecision === 'undecided'));
  const deleted = report.entries.filter((entry) => entry.status === 'pd2_deleted');
  assert.equal(deleted.length, 134);
  assert.ok(deleted.every((entry) => entry.rationale.includes('PD2 ne le fait plus apparaître')));
  assert.equal(report.entries.filter((entry) => entry.defaultDecision === 'import_pd2').length, 0);
});

test('review page offers filters, persistent choices and a decision export', () => {
  const html = fs.readFileSync(htmlPath, 'utf8');
  assert.match(html, /Aucun import n'est approuvé/);
  assert.match(html, /localStorage/);
  assert.match(html, /Exporter mes décisions/);
  assert.match(html, /pd2-affixes-decisions\.json/);
});

test('direct merge apply is blocked until the governed review is approved', () => {
  const result = spawnSync(process.execPath, [
    path.join(repoRoot, 'scripts', 'migrate-bkvince', 'pd2-affixes-merge.mjs'),
    '--apply',
  ], { cwd: repoRoot, encoding: 'utf8' });
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /Affix import is not approved/);
});
