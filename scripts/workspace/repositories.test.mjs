import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { resolveRepository, resolveWorkspaceLocation } from './repositories.mjs';

const root = path.resolve('C:/fixture/Diablo');

test('resolves sibling repositories relative to the workspace', () => {
  const repository = resolveRepository({
    id: 'suite', role: 'public-product-source', path: '../Suite', required: true,
  }, root, {});
  assert.equal(repository.path, path.resolve(root, '../Suite'));
  assert.equal(repository.pathSource, 'config');
});

test('honors an explicit environment override', () => {
  const repository = resolveRepository({
    id: 'suite', role: 'public-product-source', path: '../Suite', environmentVariable: 'SUITE_ROOT', required: true,
  }, root, { SUITE_ROOT: 'D:/products/Suite' });
  assert.equal(repository.path, path.resolve('D:/products/Suite'));
  assert.equal(repository.pathSource, 'environment');
});

test('refuses authoritative repositories under analysis-cache', () => {
  assert.throws(() => resolveRepository({
    id: 'suite', role: 'public-product-source', path: 'analysis-cache/suite', required: true,
  }, root, {}), /cannot live under analysis-cache/);
});

test('resolves Suite governance as a normal directory inside Diablo', () => {
  const governance = resolveWorkspaceLocation({
    id: 'suite-governance', role: 'public-release-governance', path: 'governance/d2rloader-suite', required: true,
  }, root);
  assert.equal(governance.path, path.resolve(root, 'governance/d2rloader-suite'));
  assert.equal(governance.pathSource, 'config');
});

test('refuses a governed workspace directory outside Diablo or in analysis-cache', () => {
  assert.throws(() => resolveWorkspaceLocation({
    id: 'suite-governance', path: '../Governance',
  }, root), /must stay inside the Diablo repository/);
  assert.throws(() => resolveWorkspaceLocation({
    id: 'suite-governance', path: 'analysis-cache/governance',
  }, root), /cannot live under analysis-cache/);
});
