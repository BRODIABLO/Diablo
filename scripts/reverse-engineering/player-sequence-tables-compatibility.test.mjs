import assert from 'node:assert/strict';
import test from 'node:test';

import {
  auditPlayerSequenceCompatibility,
} from './player-sequence-tables-compatibility.mjs';

test('proves unique ownership against the complete governed compatibility catalogs', () => {
  const report = auditPlayerSequenceCompatibility();
  assert.equal(report.candidate.bytes, 200);
  assert.equal(report.baseline.suiteWrites, 191);
  assert.equal(report.baseline.pluginPackWrites, 139);
  assert.equal(report.baseline.totalComparedWrites, 330);
  assert.equal(report.result.collisions, 0);
  assert.equal(report.result.uniqueOwner, true);
  assert.equal(report.result.hybridScope, true);
  assert.equal(report.result.eezstreetDllsModified, false);
  assert.equal(report.result.runtimeQualificationStillRequired, true);
});
