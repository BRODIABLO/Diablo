import assert from 'node:assert/strict';
import test from 'node:test';

import { createHistory, historyReducer } from './history.js';

test('supports edit, undo, redo, and future invalidation', () => {
  let history = createHistory({ level: 1 });
  history = historyReducer(history, { type: 'edit', update: () => ({ level: 2 }) });
  history = historyReducer(history, { type: 'edit', update: () => ({ level: 3 }) });
  history = historyReducer(history, { type: 'undo' });
  assert.equal(history.present.level, 2);
  history = historyReducer(history, { type: 'redo' });
  assert.equal(history.present.level, 3);
  history = historyReducer(history, { type: 'undo' });
  history = historyReducer(history, { type: 'edit', update: () => ({ level: 9 }) });
  assert.equal(history.present.level, 9);
  assert.equal(history.future.length, 0);
});
