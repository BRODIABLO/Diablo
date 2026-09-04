import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';
import { buildState, writeStateAtomic } from './checkpoint.mjs';

function fixture() {
  return {
    inspector: {
      branch: 'main', upstream: 'origin/main', ahead: 0, behind: 0,
      staged: { count: 0 }, unstaged: { count: 2 }, untracked: { count: 1 }, mixed: { count: 0 }, conflicts: { count: 0 },
      cachedWhitespaceClean: true,
      workstreams: { validationErrors: [], unassigned: [], overlaps: [], shared: [{}], streams: [{ id: 'alpha', name: 'Alpha', status: 'active', count: 3 }] },
    },
    registry: { workstreams: [{ id: 'alpha', status: 'active', mission: 'Mission/alpha.md', nextGate: 'Test it.' }] },
    currentMission: { name: 'Alpha', path: 'Mission/alpha.md', nextGate: 'Validate Alpha.' },
    head: { sha: 'abc', subject: 'Previous checkpoint', committedAt: '2026-07-23T00:00:00Z' },
    generatedAt: '2026-07-23T12:00:00Z',
    repositories: [
      { id: 'workspace', role: 'incubation-and-runtime-laboratory', path: '/workspace', pathSource: 'config', required: true, exists: true, gitRepository: true, branch: 'main', upstream: 'origin/main', head: 'abc', origin: 'origin', workstreamId: null, changed: 3, staged: 0, unstaged: 2, untracked: 1 },
      { id: 'suite', role: 'public-product-source', path: '/suite', pathSource: 'config', required: true, exists: true, gitRepository: true, branch: 'main', upstream: 'origin/main', head: 'def', origin: 'origin', workstreamId: 'alpha', changed: 4, staged: 1, unstaged: 2, untracked: 1 },
    ],
  };
}

test('selects the current mission and reports a safe checkpoint', () => {
  const state = buildState(fixture());
  assert.equal(state.currentMission.workstreamId, 'alpha');
  assert.equal(state.currentMission.changedFiles, 7);
  assert.equal(state.currentMission.repositoryChanges[0].id, 'suite');
  assert.equal(state.checkpoint.ready, true);
  assert.equal(state.checkpoint.recommendedWorkstream, 'alpha');
});

test('blocks when a required sibling repository is unavailable', () => {
  const input = fixture();
  input.repositories[1].exists = false;
  input.repositories[1].gitRepository = false;
  const state = buildState(input);
  assert.equal(state.checkpoint.ready, false);
  assert.match(state.checkpoint.blockers.join('\n'), /required repository 'suite'/);
});

test('turns ownership and Git hazards into blockers', () => {
  const input = fixture();
  input.inspector.mixed.count = 1;
  input.inspector.workstreams.unassigned = ['loose.txt'];
  const state = buildState(input);
  assert.equal(state.checkpoint.ready, false);
  assert.match(state.checkpoint.blockers.join('\n'), /mixed/);
  assert.match(state.checkpoint.blockers.join('\n'), /unassigned/);
});

test('persists valid JSON atomically', (t) => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'diablo-checkpoint-'));
  t.after(() => fs.rmSync(root, { recursive: true, force: true }));
  const target = path.join(root, 'nested', 'state.json');
  const state = buildState(fixture());
  writeStateAtomic(target, state);
  assert.deepEqual(JSON.parse(fs.readFileSync(target, 'utf8')), state);
  assert.deepEqual(fs.readdirSync(path.dirname(target)), ['state.json']);
});
