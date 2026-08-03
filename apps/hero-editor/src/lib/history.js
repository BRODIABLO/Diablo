export function createHistory(present) {
  return { past: [], present, future: [] };
}

export function historyReducer(history, action) {
  switch (action.type) {
    case 'replace':
      return createHistory(action.value);
    case 'edit': {
      const next = action.update(history.present);
      if (JSON.stringify(next) === JSON.stringify(history.present)) {
        return history;
      }
      return {
        past: [...history.past, history.present],
        present: next,
        future: [],
      };
    }
    case 'undo':
      if (history.past.length === 0) return history;
      return {
        past: history.past.slice(0, -1),
        present: history.past.at(-1),
        future: [history.present, ...history.future],
      };
    case 'redo':
      if (history.future.length === 0) return history;
      return {
        past: [...history.past, history.present],
        present: history.future[0],
        future: history.future.slice(1),
      };
    default:
      return history;
  }
}
