import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { resolveRepository } from './repositories.mjs';

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
