import { useEffect, useId, useMemo, useReducer, useRef, useState } from 'react';

import {
  addChronicleEntrySnapshot,
  addCatalogItemBatchSnapshot,
  addItemBatchSnapshot,
  addItemGroupSnapshot,
  addImportedItemToEquipmentSlotSnapshot,
  addImportedItemsSnapshot,
  addItemToEquipmentSlotSnapshot,
  applyRunewordSnapshot,
  activePlacedItems,
  attributeFields,
  availableBeltItemBases,
  availableItemBases,
  availableItemGroups,
  availableNamedItems,
  availableRunewordItems,
  availableEquipmentItemBases,
  beltCapacityForPlacements,
  carriedGoldGameMaximum,
  characterItemIds,
  chronicleCatalog,
  compileItemTierPatch,
  compileMagicAffixPatch,
  compileManualPropertyPatch,
  compileNamedQualityPatch,
  compileRareAffixPatch,
  compileSetBonusPatch,
  containerForPlacement,
  createBlankCharacter,
  createMercenarySnapshot,
  demonDefinitions,
  describeItem,
  describeChronicleEntry,
  duplicateItemSnapshot,
  editStackableSharedStashCounterSnapshot,
  emptyPersonalStashSnapshot,
  difficultyDefinitions,
  editItemSnapshot,
  editableSharedStashInventorySnapshot,
  equipmentSlotDefinitions,
  editableItems,
  editableSnapshot,
  exportCharacter,
  exportSharedStashInventory,
  exportItemBundle,
  exportItemRecord,
  extractSocketFillerSnapshot,
  importItemFiles,
  hydrateSharedStashInventory,
  goldLimits,
  itemEditorOptions,
  itemBonusDashboard,
  itemBonusSummary,
  itemContainers,
  itemRecords,
  insertImportedSocketFillersSnapshot,
  insertSocketFillerSnapshot,
  moveItemPlacement,
  moveItemToEquipmentSlot,
  mercenaryDefinitions,
  openCharacter,
  openSharedStash,
  PERFECT_ITEM_LEVEL,
  previewManualPropertyPatch,
  questActs,
  setAllQuestsSnapshot,
  setAllWaypointsSnapshot,
  unlockHellSnapshot,
  setQuestCompletionSnapshot,
  setQuestConsumedScrollSnapshot,
  setSkillPointsSnapshot,
  setWaypointSnapshot,
  removeSetBonusPatch,
  removeSocketFillerSnapshot,
  removeItemSnapshot,
  removeChronicleEntrySnapshot,
  removeMercenarySnapshot,
  clearRunewordSnapshot,
  skillEditorDefinition,
  snapshotsEqual,
  suggestedFileName,
  supportedClasses,
  transferItemSnapshot,
  updateChronicleEntrySnapshot,
  waypointActs,
} from './lib/character-codec.js';
import { createHistory, historyReducer } from './lib/history.js';
import {
  equipmentPanelVisual,
  equipmentSlotPlaceholderVisuals,
  itemVisuals,
  mercenaryEquipmentPanelVisual,
} from './data/item-visuals.generated.js';
import {
  classPortraitVisuals,
  skillTreeVisuals,
  skillVisuals,
} from './data/skill-visuals.generated.js';

const navigation = [
  { id: 'general', label: 'General', enabled: true },
  { id: 'quests', label: 'Quests', enabled: true },
  { id: 'waypoints', label: 'Waypoints', enabled: true },
  { id: 'item-bonuses', label: 'Item Bonuses', enabled: true },
  { id: 'skills', label: 'Skills', enabled: true },
  { id: 'chronicle', label: 'Chronicle', badge: 'New', enabled: true },
  { id: 'mercenary', label: 'Mercenary', enabled: true },
  { id: 'demon', label: 'Demon', enabled: true },
];

const pageContent = {
  general: {
    title: 'General',
    description: 'Edit identity, save seed, and character state flags.',
  },
  quests: {
    title: 'Quests',
    description: 'Edit the real quest flags stored for Normal, Nightmare, and Hell.',
  },
  waypoints: {
    title: 'Waypoints',
    description: 'Toggle every native waypoint bit independently across all three difficulties.',
  },
  'item-bonuses': {
    title: 'Item Bonuses',
    description: 'Inspect the live combined bonuses from equipped gear, runewords, and socket fillers.',
  },
  skills: {
    title: 'Skills',
    description: 'Allocate the 30 class skills using BKVince tree positions and prerequisites.',
  },
  chronicle: {
    title: 'Shared Stash & Chronicle',
    description: 'Edit native Shared Stash pages and Chronicle discoveries with page-scoped, fail-closed exports.',
  },
  mercenary: {
    title: 'Mercenary',
    description: 'Edit the native hireling header and inspect every mercenary equipment record.',
  },
  demon: {
    title: 'Demon',
    description: 'Edit an existing BKVince Bound Demon while preserving every opaque native byte.',
  },
  stats: {
    title: 'Stats',
    description: 'Edit stored attributes within the limits declared by BKVince ItemStatCost.',
  },
  equipment: {
    title: 'Equipment',
    description: 'Inspect the real body slots and belt positions stored in the D2S.',
  },
  inventory: {
    title: 'Inventory',
    description: 'Select an existing item, then place it in a free Inventory, Cube, or personal-stash cell.',
  },
};

const equipmentSlots = equipmentSlotDefinitions.map(({ id, label }) => [id, label]);
const mercenaryEquipmentSlots = equipmentSlots.filter(([id]) => [1, 3, 4, 5].includes(id));

const attributeFieldsByKey = Object.freeze(
  Object.fromEntries(attributeFields.map((field) => [field.key, field])),
);

const dialogFocusableSelector = [
  'a[href]',
  'button:not([disabled])',
  'input:not([disabled])',
  'select:not([disabled])',
  'textarea:not([disabled])',
  '[tabindex]:not([tabindex="-1"])',
].join(',');

function visibleDialogControls(dialog) {
  return [...dialog.querySelectorAll(dialogFocusableSelector)]
    .filter((element) => element.getClientRects().length > 0 && element.getAttribute('aria-hidden') !== 'true');
}

function resetWorkspaceScroll() {
  globalThis.requestAnimationFrame(() => {
    globalThis.scrollTo({ top: 0, left: 0, behavior: 'auto' });
  });
}

function AccessibleModal({ className = '', labelledBy, onClose, children }) {
  const dialogRef = useRef(null);
  const closeRef = useRef(onClose);

  useEffect(() => {
    closeRef.current = onClose;
  });

  useEffect(() => {
    const dialog = dialogRef.current;
    if (!dialog) return undefined;
    const previouslyFocused = globalThis.document.activeElement instanceof HTMLElement
      ? globalThis.document.activeElement
      : null;
    const previousBodyOverflow = globalThis.document.body.style.overflow;
    const backdrop = dialog.parentElement;
    const shell = dialog.closest('.app-shell');
    const inertSiblings = shell
      ? [...shell.children]
        .filter((element) => element !== backdrop)
        .map((element) => ({ element, inert: element.inert }))
      : [];

    globalThis.document.body.style.overflow = 'hidden';
    inertSiblings.forEach(({ element }) => { element.inert = true; });

    const focusFrame = globalThis.requestAnimationFrame(() => {
      const initial = dialog.querySelector('[autofocus]')
        || dialog.querySelector('[data-dialog-initial-focus]')
        || visibleDialogControls(dialog)[0]
        || dialog;
      initial.focus({ preventScroll: true });
    });

    function handleKeyDown(event) {
      if (event.key === 'Escape') {
        if (event.target instanceof Element && event.target.closest('[data-escape-preserve-dialog]')) return;
        event.preventDefault();
        event.stopPropagation();
        closeRef.current();
        return;
      }
      if (event.key !== 'Tab') return;
      const controls = visibleDialogControls(dialog);
      if (controls.length === 0) {
        event.preventDefault();
        dialog.focus({ preventScroll: true });
        return;
      }
      const first = controls[0];
      const last = controls.at(-1);
      const active = globalThis.document.activeElement;
      if (event.shiftKey && (active === first || !dialog.contains(active))) {
        event.preventDefault();
        last.focus({ preventScroll: true });
      } else if (!event.shiftKey && active === last) {
        event.preventDefault();
        first.focus({ preventScroll: true });
      }
    }

    dialog.addEventListener('keydown', handleKeyDown);
    return () => {
      globalThis.cancelAnimationFrame(focusFrame);
      dialog.removeEventListener('keydown', handleKeyDown);
      globalThis.document.body.style.overflow = previousBodyOverflow;
      inertSiblings.forEach(({ element, inert }) => { element.inert = inert; });
      if (previouslyFocused?.isConnected) {
        globalThis.requestAnimationFrame(() => previouslyFocused.focus({ preventScroll: true }));
      }
    };
  }, []);

  return (
    <div
      className="modal-backdrop"
      role="presentation"
      onPointerDown={(event) => {
        if (event.target === event.currentTarget) closeRef.current();
      }}
    >
      <section
        ref={dialogRef}
        className={`modal ${className}`.trim()}
        role="dialog"
        aria-modal="true"
        aria-labelledby={labelledBy}
        tabIndex={-1}
      >
        {children}
      </section>
    </div>
  );
}

function ClassPortrait({ className }) {
  const normalizedClass = String(className || '').toLocaleLowerCase('en-US');
  const visual = classPortraitVisuals[normalizedClass];
  const initials = className ? className.slice(0, 2).toUpperCase() : '—';
  return (
    <div className="portrait" aria-hidden="true">
      <span className="portrait-initials">{initials}</span>
      {visual && (
        <img
          src={visual}
          alt=""
          draggable="false"
          onError={(event) => { event.currentTarget.hidden = true; }}
        />
      )}
    </div>
  );
}

function emptyVirtualStashSnapshot() {
  return { addedItems: [], itemEdits: [], itemPlacements: [] };
}

const itemDragMime = 'application/x-bkvince-item';
const draggableItemScopes = new Set(['player', 'shared-stash', 'virtual-stash', 'trash']);

function itemDropScopesCompatible(sourceScope, targetScope, targetKind) {
  if (targetKind === 'equipment') {
    return targetScope === 'player' && draggableItemScopes.has(sourceScope);
  }
  return draggableItemScopes.has(sourceScope) && draggableItemScopes.has(targetScope);
}

function useItemPointerDrag({ payload, onActivity, onDrop }) {
  const holdTimer = useRef(null);
  const active = useRef(false);
  const origin = useRef(null);
  const target = useRef(null);
  const suppressClick = useRef(false);

  function clearTarget() {
    target.current?.classList.remove('drag-over');
    target.current = null;
  }

  function resetPointerDrag() {
    if (holdTimer.current !== null) globalThis.clearTimeout(holdTimer.current);
    holdTimer.current = null;
    clearTarget();
    origin.current?.classList.remove('touch-dragging');
    origin.current = null;
    active.current = false;
    onActivity?.(null);
  }

  useEffect(() => () => {
    if (holdTimer.current !== null) globalThis.clearTimeout(holdTimer.current);
    holdTimer.current = null;
    target.current?.classList.remove('drag-over');
    target.current = null;
    origin.current?.classList.remove('touch-dragging');
    origin.current = null;
    active.current = false;
  }, []);

  function finishPointerDrag(event, shouldDrop) {
    if (holdTimer.current !== null) globalThis.clearTimeout(holdTimer.current);
    holdTimer.current = null;
    if (!active.current) return;
    event.preventDefault();
    event.stopPropagation();
    const destination = target.current;
    if (shouldDrop && destination) onDrop?.(payload, { ...destination.dataset });
    suppressClick.current = true;
    globalThis.setTimeout(() => { suppressClick.current = false; }, 0);
    resetPointerDrag();
  }

  return {
    enabled: Boolean(onDrop),
    onClickCapture(event) {
      if (!suppressClick.current) return;
      event.preventDefault();
      event.stopPropagation();
    },
    onPointerDown(event) {
      if (!onDrop || !['touch', 'pen'].includes(event.pointerType)) return;
      const source = event.currentTarget;
      const pointerId = event.pointerId;
      holdTimer.current = globalThis.setTimeout(() => {
        active.current = true;
        origin.current = source;
        source.classList.add('touch-dragging');
        source.setPointerCapture?.(pointerId);
        onActivity?.(payload);
      }, 220);
    },
    onPointerMove(event) {
      if (!active.current) return;
      event.preventDefault();
      const candidate = globalThis.document
        .elementFromPoint(event.clientX, event.clientY)
        ?.closest?.('[data-item-drop-target="true"]');
      const compatible = candidate && itemDropScopesCompatible(
        payload.scope,
        candidate.dataset.itemDropScope,
        candidate.dataset.itemDropKind,
      ) ? candidate : null;
      if (compatible === target.current) return;
      clearTarget();
      target.current = compatible;
      target.current?.classList.add('drag-over');
    },
    onPointerUp(event) {
      finishPointerDrag(event, true);
    },
    onPointerCancel(event) {
      finishPointerDrag(event, false);
      if (!active.current && holdTimer.current !== null) resetPointerDrag();
    },
  };
}

export default function App() {
  const classes = useMemo(() => supportedClasses(), []);
  const addableItems = useMemo(() => availableItemBases(), []);
  const itemGroups = useMemo(() => availableItemGroups(), []);
  const namedItems = useMemo(() => availableNamedItems(), []);
  const runewordItems = useMemo(() => availableRunewordItems(), []);
  const fileInput = useRef(null);
  const sharedStashInput = useRef(null);
  const [document, setDocument] = useState(null);
  const [history, dispatch] = useReducer(historyReducer, createHistory(null));
  const [sharedStashDocument, setSharedStashDocument] = useState(null);
  const [stashHistory, dispatchStash] = useReducer(historyReducer, createHistory(null));
  const [stashItemsHistory, dispatchStashItems] = useReducer(historyReducer, createHistory(null));
  const [virtualStashHistory, dispatchVirtualStash] = useReducer(
    historyReducer,
    createHistory(emptyVirtualStashSnapshot()),
  );
  const [trashHistory, dispatchTrash] = useReducer(
    historyReducer,
    createHistory(emptyVirtualStashSnapshot()),
  );
  const [compoundTransfers, setCompoundTransfers] = useState([]);
  const [sharedStashPageIndex, setSharedStashPageIndex] = useState(0);
  const [topStashMode, setTopStashMode] = useState('stash');
  const [activeTab, setActiveTab] = useState('general');
  const [selectedItemIndex, setSelectedItemIndex] = useState(null);
  const [selectedItemScope, setSelectedItemScope] = useState('player');
  const [draggedItem, setDraggedItem] = useState(null);
  const [itemEditorOpen, setItemEditorOpen] = useState(false);
  const [itemChooserTarget, setItemChooserTarget] = useState(null);
  const [createOpen, setCreateOpen] = useState(false);
  const [busy, setBusy] = useState(false);
  const [notice, setNotice] = useState({ tone: 'neutral', text: 'No save is loaded.' });

  const editable = history.present;
  const characterDirty = Boolean(document && editable && !snapshotsEqual(document.initial, editable));
  const chronicleEditable = stashHistory.present;
  const sharedStashItemsEditable = stashItemsHistory.present;
  const virtualStashEditable = virtualStashHistory.present;
  const trashEditable = trashHistory.present;
  const chronicleDirty = Boolean(
    sharedStashDocument
    && chronicleEditable
    && !snapshotsEqual(sharedStashDocument.initial, chronicleEditable),
  );
  const sharedStashItemsDirty = Boolean(
    sharedStashDocument
    && sharedStashItemsEditable
    && !snapshotsEqual(sharedStashDocument.inventoryInitial, sharedStashItemsEditable),
  );
  const virtualStashDirty = virtualStashEditable.itemPlacements.some(({ removed }) => !removed);
  const trashDirty = trashEditable.itemPlacements.some(({ removed }) => !removed);
  const isDirty = characterDirty || chronicleDirty || sharedStashItemsDirty || virtualStashDirty || trashDirty;
  const activeHistory = activeTab === 'chronicle'
    ? stashHistory
    : (topStashMode === 'virtual'
      ? virtualStashHistory
      : (topStashMode === 'trash' ? trashHistory : history));
  const activeHistoryKind = activeTab === 'chronicle'
    ? 'chronicle'
    : (topStashMode === 'virtual'
      ? 'virtual'
      : (topStashMode === 'trash' ? 'trash' : 'character'));
  const displayedItems = useMemo(
    () => (document && editable
      ? editableItems(document.model.items, editable.itemEdits, editable.addedItems)
      : []),
    [document, editable],
  );
  const sourceItems = useMemo(
    () => (document && editable ? itemRecords(document.model.items, editable.addedItems) : []),
    [document, editable],
  );
  const displayedMercItems = useMemo(
    () => (document && editable
      ? editableItems(document.model.merc_items || [], editable.mercItemEdits, editable.mercAddedItems)
      : []),
    [document, editable],
  );
  const selectedSharedStashPage = sharedStashDocument?.pages?.[sharedStashPageIndex] || null;
  const selectedSharedStashPageEditable = sharedStashItemsEditable?.pages?.[sharedStashPageIndex] || null;
  const displayedSharedStashItems = useMemo(
    () => (selectedSharedStashPage && selectedSharedStashPageEditable
      ? editableItems(
        selectedSharedStashPage.items,
        selectedSharedStashPageEditable.itemEdits,
        selectedSharedStashPageEditable.addedItems,
      )
      : []),
    [selectedSharedStashPage, selectedSharedStashPageEditable],
  );
  const sourceMercItems = useMemo(
    () => (document && editable
      ? itemRecords(document.model.merc_items || [], editable.mercAddedItems)
      : []),
    [document, editable],
  );
  const displayedVirtualStashItems = useMemo(
    () => editableItems([], virtualStashEditable.itemEdits, virtualStashEditable.addedItems),
    [virtualStashEditable],
  );
  const sourceVirtualStashItems = useMemo(
    () => itemRecords([], virtualStashEditable.addedItems),
    [virtualStashEditable],
  );
  const exportableVirtualStashItems = useMemo(
    () => activePlacedItems(
      [],
      virtualStashEditable.itemEdits,
      virtualStashEditable.addedItems,
      virtualStashEditable.itemPlacements,
    ),
    [virtualStashEditable],
  );
  const displayedTrashItems = useMemo(
    () => editableItems([], trashEditable.itemEdits, trashEditable.addedItems),
    [trashEditable],
  );
  const sourceTrashItems = useMemo(
    () => itemRecords([], trashEditable.addedItems),
    [trashEditable],
  );
  const exportableTrashItems = useMemo(
    () => activePlacedItems(
      [],
      trashEditable.itemEdits,
      trashEditable.addedItems,
      trashEditable.itemPlacements,
    ),
    [trashEditable],
  );
  const playerBonuses = useMemo(
    () => (document && editable
      ? itemBonusSummary(
        document.model.items,
        editable.itemEdits,
        editable.addedItems,
        editable.itemPlacements,
        editable.attributes.level,
      )
      : []),
    [document, editable],
  );
  const playerBonusDashboard = useMemo(
    () => itemBonusDashboard(playerBonuses),
    [playerBonuses],
  );
  const selectedDisplayedItems = selectedItemScope === 'mercenary'
    ? displayedMercItems
    : (selectedItemScope === 'shared-stash'
      ? displayedSharedStashItems
      : (selectedItemScope === 'virtual-stash'
        ? displayedVirtualStashItems
        : (selectedItemScope === 'trash' ? displayedTrashItems : displayedItems)));
  const selectedSourceItems = selectedItemScope === 'mercenary'
    ? sourceMercItems
    : (selectedItemScope === 'shared-stash'
      ? itemRecords(selectedSharedStashPage?.items || [], selectedSharedStashPageEditable?.addedItems || [])
      : (selectedItemScope === 'virtual-stash'
        ? sourceVirtualStashItems
        : (selectedItemScope === 'trash' ? sourceTrashItems : sourceItems)));
  const selectedItemEdits = selectedItemScope === 'mercenary'
    ? editable?.mercItemEdits
    : (selectedItemScope === 'shared-stash'
      ? selectedSharedStashPageEditable?.itemEdits
      : (selectedItemScope === 'virtual-stash'
        ? virtualStashEditable.itemEdits
        : (selectedItemScope === 'trash' ? trashEditable.itemEdits : editable?.itemEdits)));

  useEffect(() => {
    if (selectedItemIndex === null || selectedDisplayedItems[selectedItemIndex]) return;
    setSelectedItemIndex(null);
    setItemEditorOpen(false);
  }, [selectedDisplayedItems, selectedItemIndex]);

  function modelItemsForScope(scope = selectedItemScope) {
    if (scope === 'shared-stash') return selectedSharedStashPage?.items || [];
    if (scope === 'virtual-stash' || scope === 'trash') return [];
    if (!document) return [];
    return scope === 'mercenary' ? (document.model.merc_items || []) : document.model.items;
  }

  function editableForScope(scope = selectedItemScope) {
    if (scope === 'shared-stash') return selectedSharedStashPageEditable;
    if (scope === 'virtual-stash') return virtualStashEditable;
    if (scope === 'trash') return trashEditable;
    if (!editable || scope === 'player') return editable;
    return {
      ...editable,
      addedItems: editable.mercAddedItems,
      itemEdits: editable.mercItemEdits,
      itemPlacements: editable.mercItemPlacements,
    };
  }

  function mergeScopeEditable(next, scope = selectedItemScope) {
    if (scope === 'virtual-stash') return next;
    if (scope === 'trash') return next;
    if (scope === 'shared-stash') {
      return {
        ...sharedStashItemsEditable,
        pages: sharedStashItemsEditable.pages.map((page, index) => (
          index === sharedStashPageIndex ? next : page
        )),
      };
    }
    if (!editable || scope === 'player') return next;
    return {
      ...editable,
      mercAddedItems: next.addedItems,
      mercItemEdits: next.itemEdits,
      mercItemPlacements: next.itemPlacements,
    };
  }

  function commitScopedEditable(next, scope = selectedItemScope) {
    if (scope === 'virtual-stash') {
      dispatchVirtualStash({ type: 'edit', update: () => next });
    } else if (scope === 'trash') {
      dispatchTrash({ type: 'edit', update: () => next });
    } else if (scope === 'shared-stash') {
      dispatchStashItems({ type: 'edit', update: () => next });
    } else {
      dispatch({ type: 'edit', update: () => next });
    }
  }

  function allSharedStashItems() {
    if (!sharedStashDocument || !sharedStashItemsEditable) return [];
    return sharedStashDocument.pages.flatMap((page, index) => editableItems(
      page.items,
      sharedStashItemsEditable.pages[index].itemEdits,
      sharedStashItemsEditable.pages[index].addedItems,
    ));
  }

  function reservedItemIds() {
    return characterItemIds({
      items: [
        ...displayedItems,
        ...allSharedStashItems(),
        ...displayedVirtualStashItems,
        ...displayedTrashItems,
      ],
      merc_items: displayedMercItems,
    });
  }

  function historyKindForScope(scope) {
    if (scope === 'shared-stash') return 'shared-stash';
    if (scope === 'virtual-stash') return 'virtual';
    if (scope === 'trash') return 'trash';
    return 'character';
  }

  function historyForKind(kind) {
    if (kind === 'shared-stash') return stashItemsHistory;
    if (kind === 'virtual') return virtualStashHistory;
    if (kind === 'trash') return trashHistory;
    if (kind === 'chronicle') return stashHistory;
    return history;
  }

  function dispatchForKind(kind, action) {
    if (kind === 'shared-stash') dispatchStashItems(action);
    else if (kind === 'virtual') dispatchVirtualStash(action);
    else if (kind === 'trash') dispatchTrash(action);
    else if (kind === 'chronicle') dispatchStash(action);
    else dispatch(action);
  }

  function historyLabelForKind(kind) {
    if (kind === 'shared-stash') return 'Shared Stash';
    if (kind === 'virtual') return 'Virtual Stash';
    if (kind === 'trash') return 'Trash';
    if (kind === 'chronicle') return 'Chronicle';
    return 'character';
  }

  function matchingCompoundTransfer(kind, direction) {
    const candidates = direction === 'undo'
      ? [...compoundTransfers].reverse()
      : compoundTransfers;
    return candidates.find((transaction) => {
      if (![transaction.sourceKind, transaction.targetKind].includes(kind)) return false;
      const sourceHistory = historyForKind(transaction.sourceKind);
      const targetHistory = historyForKind(transaction.targetKind);
      if (direction === 'undo') {
        return transaction.status === 'applied'
          && snapshotsEqual(sourceHistory.present, transaction.sourceAfter)
          && snapshotsEqual(targetHistory.present, transaction.targetAfter)
          && snapshotsEqual(sourceHistory.past.at(-1), transaction.sourceBefore)
          && snapshotsEqual(targetHistory.past.at(-1), transaction.targetBefore);
      }
      return transaction.status === 'undone'
        && snapshotsEqual(sourceHistory.present, transaction.sourceBefore)
        && snapshotsEqual(targetHistory.present, transaction.targetBefore)
        && snapshotsEqual(sourceHistory.future[0], transaction.sourceAfter)
        && snapshotsEqual(targetHistory.future[0], transaction.targetAfter);
    }) || null;
  }

  function performHistoryAction(kind, direction) {
    const transaction = matchingCompoundTransfer(kind, direction);
    if (!transaction) {
      const currentHistory = historyForKind(kind);
      const canApply = direction === 'undo'
        ? currentHistory.past.length > 0
        : currentHistory.future.length > 0;
      if (!canApply) return;
      dispatchForKind(kind, { type: direction });
      setNotice({
        tone: 'success',
        text: `${direction === 'undo' ? 'Undid' : 'Redid'} the latest ${historyLabelForKind(kind)} change.`,
      });
      return;
    }
    dispatchForKind(transaction.sourceKind, { type: direction });
    dispatchForKind(transaction.targetKind, { type: direction });
    setCompoundTransfers((current) => current.map((candidate) => (
      candidate.id === transaction.id
        ? { ...candidate, status: direction === 'undo' ? 'undone' : 'applied' }
        : candidate
    )));
    setNotice({
      tone: 'success',
      text: `${direction === 'undo' ? 'Undid' : 'Redid'} the complete ${transaction.label} transfer in both workspaces.`,
    });
  }

  useEffect(() => {
    function handleWorkspaceShortcut(event) {
      if (event.defaultPrevented || busy || createOpen || itemEditorOpen || itemChooserTarget) return;
      const target = event.target;
      const editingText = target instanceof HTMLElement
        && (target.matches('input, textarea, select') || target.isContentEditable);
      if (editingText) return;

      if (event.key === 'Escape' && selectedItemIndex !== null) {
        event.preventDefault();
        setSelectedItemIndex(null);
        setNotice({ tone: 'neutral', text: 'Item movement canceled. No save data changed.' });
        return;
      }

      const modifier = event.ctrlKey || event.metaKey;
      const normalizedKey = event.key.toLowerCase();
      const direction = modifier && normalizedKey === 'z'
        ? (event.shiftKey ? 'redo' : 'undo')
        : (modifier && normalizedKey === 'y' ? 'redo' : null);
      if (!direction) return;
      const canApply = direction === 'undo'
        ? activeHistory.past.length > 0
        : activeHistory.future.length > 0;
      if (!canApply) return;
      event.preventDefault();
      performHistoryAction(activeHistoryKind, direction);
    }

    globalThis.document.addEventListener('keydown', handleWorkspaceShortcut);
    return () => globalThis.document.removeEventListener('keydown', handleWorkspaceShortcut);
  }, [
    activeHistory.future.length,
    activeHistory.past.length,
    activeHistoryKind,
    busy,
    createOpen,
    itemChooserTarget,
    itemEditorOpen,
    selectedItemIndex,
  ]);

  function transferTargetsForScope(scope) {
    const targets = [
      { scope: 'player', label: 'Personal Stash' },
      { scope: 'shared-stash', label: `Shared Stash · Page ${sharedStashPageIndex + 1}` },
      { scope: 'virtual-stash', label: 'Virtual Stash' },
      { scope: 'trash', label: 'Trash' },
    ];
    return targets
      .filter((target) => target.scope !== scope)
      .filter((target) => !(scope === 'mercenary' && target.scope === 'player'))
      .map((target) => {
        if (target.scope !== 'shared-stash') return { ...target, disabled: false, reason: '' };
        if (!sharedStashDocument || !selectedSharedStashPageEditable) {
          return { ...target, disabled: true, reason: 'Load a Shared Stash first.' };
        }
        if (selectedSharedStashPage?.isStackable) {
          return { ...target, disabled: true, reason: 'Choose an ordinary Shared Stash page.' };
        }
        return { ...target, disabled: false, reason: '' };
      });
  }

  function transferItemAt(sourceScope, sourceIndex, targetScope, {
    containerId = 'stash',
    x = 0,
    y = 0,
    slotId = null,
    autoPlace = false,
  } = {}) {
    if (!document || !editable || sourceScope === targetScope) return false;
    try {
      if (targetScope === 'shared-stash' && (!selectedSharedStashPageEditable || selectedSharedStashPage?.isStackable)) {
        throw new Error('Load and select an ordinary Shared Stash page before transferring this item.');
      }
      const sourceKind = historyKindForScope(sourceScope);
      const targetKind = historyKindForScope(targetScope);
      if (sourceKind === targetKind) {
        throw new Error('This transfer path shares one history and is not exposed until its combined placement ABI is defined.');
      }
      const sourceScoped = editableForScope(sourceScope);
      const targetScoped = editableForScope(targetScope);
      if (!sourceScoped || !targetScoped) throw new Error('The transfer source or target is not loaded.');
      const sourceDisplayItems = editableItems(
        modelItemsForScope(sourceScope),
        sourceScoped.itemEdits,
        sourceScoped.addedItems,
      );
      const descriptor = describeItem(sourceDisplayItems[sourceIndex], sourceIndex);
      const result = transferItemSnapshot(
        sourceScoped,
        modelItemsForScope(sourceScope),
        targetScoped,
        modelItemsForScope(targetScope),
        {
          sourceIndex,
          containerId,
          x,
          y,
          slotId,
          autoPlace,
          reservedItemIds: reservedItemIds(),
        },
      );
      const sourceBefore = historyForKind(sourceKind).present;
      const targetBefore = historyForKind(targetKind).present;
      const sourceAfter = mergeScopeEditable(result.source, sourceScope);
      const targetAfter = mergeScopeEditable(result.target, targetScope);
      dispatchForKind(sourceKind, { type: 'edit', update: () => sourceAfter });
      dispatchForKind(targetKind, { type: 'edit', update: () => targetAfter });
      const targetLabel = transferTargetsForScope(sourceScope)
        .find((target) => target.scope === targetScope)?.label || targetScope;
      const targetSlot = slotId === null || slotId === undefined
        ? null
        : equipmentSlotDefinitions.find(({ id }) => id === slotId);
      const destinationLabel = targetSlot
        ? `Player equipment · ${targetSlot.label}`
        : targetLabel;
      const sourceLabel = sourceScope === 'player'
        ? 'Player storage'
        : (sourceScope === 'shared-stash'
          ? `Shared Stash · Page ${sharedStashPageIndex + 1}`
          : (sourceScope === 'virtual-stash' ? 'Virtual Stash' : (sourceScope === 'trash' ? 'Trash' : 'Mercenary')));
      const transaction = {
        id: crypto.randomUUID(),
        status: 'applied',
        label: `${sourceLabel} → ${destinationLabel}`,
        sourceKind,
        targetKind,
        sourceBefore,
        targetBefore,
        sourceAfter,
        targetAfter,
      };
      setCompoundTransfers((current) => [...current, transaction]);
      setItemEditorOpen(false);
      setSelectedItemIndex(null);
      setDraggedItem(null);
      setSelectedItemScope(targetScope);
      if (targetScope === 'virtual-stash') setTopStashMode('virtual');
      else if (targetScope === 'trash') setTopStashMode('trash');
      else if (targetScope === 'player') setTopStashMode('stash');
      else if (targetScope === 'shared-stash') setActiveTab('chronicle');
      setNotice({
        tone: 'success',
        text: targetSlot
          ? `${descriptor.name} equipped atomically from ${sourceLabel} in ${targetSlot.label}. Undo reverses both workspaces together.`
          : `${descriptor.name} moved atomically from ${sourceLabel} to ${targetLabel} ${containerId === 'belt' ? `slot ${x + 1}` : `${itemContainers[containerId]?.label || containerId} ${x + 1},${y + 1}`}. Undo reverses both workspaces together.`,
      });
      return true;
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
      return false;
    }
  }

  function transferSelectedItem(targetScope) {
    if (!document || !editable || selectedItemIndex === null) return;
    transferItemAt(selectedItemScope, selectedItemIndex, targetScope, {
      autoPlace: targetScope === 'trash',
    });
  }

  async function loadFile(file) {
    if (!file) return;
    setBusy(true);
    try {
      const nextDocument = await openCharacter(await file.arrayBuffer(), file.name);
      setDocument(nextDocument);
      dispatch({ type: 'replace', value: editableSnapshot(nextDocument.model) });
      setCompoundTransfers([]);
      setActiveTab('general');
      setSelectedItemIndex(null);
      setSelectedItemScope('player');
      setItemEditorOpen(false);
      setItemChooserTarget(null);
      resetWorkspaceScroll();
      setNotice({
        tone: 'success',
        text: `${nextDocument.model.header.name} opened locally. Source bytes remain untouched.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    } finally {
      setBusy(false);
      if (fileInput.current) fileInput.current.value = '';
    }
  }

  async function loadSharedStash(file) {
    if (!file) return;
    setBusy(true);
    try {
      const nextDocument = await hydrateSharedStashInventory(
        openSharedStash(await file.arrayBuffer(), file.name),
      );
      setSharedStashDocument(nextDocument);
      dispatchStash({ type: 'replace', value: nextDocument.initial });
      dispatchStashItems({
        type: 'replace',
        value: editableSharedStashInventorySnapshot(nextDocument),
      });
      setCompoundTransfers([]);
      setSharedStashPageIndex(0);
      setActiveTab('chronicle');
      setNotice({
        tone: 'success',
        text: `${file.name} opened locally: ${nextDocument.pageCount.toLocaleString()} stash pages, ${nextDocument.pages.reduce((total, page) => total + page.items.length, 0).toLocaleString()} items, and ${chronicleDiscoveryCount(nextDocument.initial).toLocaleString()} Chronicle discoveries.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    } finally {
      setBusy(false);
      if (sharedStashInput.current) sharedStashInput.current.value = '';
    }
  }

  async function createHero(className) {
    if (characterDirty && !globalThis.confirm(
      'Create a new character and discard the current unsaved hero changes?',
    )) return;
    const blankHero = {
      name: className,
      className,
      hardcore: false,
      ladder: false,
    };
    setBusy(true);
    try {
      const nextDocument = await createBlankCharacter(blankHero);
      setDocument(nextDocument);
      dispatch({ type: 'replace', value: editableSnapshot(nextDocument.model) });
      setCompoundTransfers([]);
      setActiveTab('general');
      setSelectedItemIndex(null);
      setSelectedItemScope('player');
      setItemEditorOpen(false);
      setItemChooserTarget(null);
      setCreateOpen(false);
      resetWorkspaceScroll();
      setNotice({
        tone: 'success',
        text: `${className} created as a blank character. No preset or automatic build was applied.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    } finally {
      setBusy(false);
    }
  }

  async function downloadCopy() {
    if (!document || !editable) return;
    setBusy(true);
    try {
      const result = await exportCharacter(document, editable);
      downloadLocalBytes(result.bytes, suggestedFileName(editable));
      setNotice({
        tone: 'success',
        text: result.byteExact
          ? 'Downloaded the original bytes byte-for-byte; no rewrite was needed.'
          : 'Downloaded a new D2S after checksum, size, and reparse validation.',
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    } finally {
      setBusy(false);
    }
  }

  async function downloadSharedStash() {
    if (!sharedStashDocument || !chronicleEditable || !sharedStashItemsEditable) return;
    setBusy(true);
    try {
      const result = await exportSharedStashInventory(
        sharedStashDocument,
        chronicleEditable,
        sharedStashItemsEditable,
      );
      downloadLocalBytes(result.bytes, sharedStashDocument.fileName);
      setNotice({
        tone: 'success',
        text: result.byteExact
          ? 'Downloaded the original Shared Stash byte-for-byte; Chronicle was unchanged.'
          : `Downloaded ${sharedStashDocument.fileName}; edited pages and Chronicle reparsed while every untouched page stayed byte-exact.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    } finally {
      setBusy(false);
    }
  }

  async function downloadSelectedItem() {
    if (selectedItemIndex === null || !selectedDisplayedItems[selectedItemIndex]) return;
    setBusy(true);
    try {
      const item = selectedDisplayedItems[selectedItemIndex];
      const descriptor = describeItem(item, selectedItemIndex);
      const result = await exportItemRecord(item);
      downloadLocalBytes(result.bytes, `${safeDownloadName(descriptor.name)}.d2i`);
      setNotice({
        tone: 'success',
        text: `${descriptor.name} downloaded as one reparsed D2S v105 item record.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    } finally {
      setBusy(false);
    }
  }

  async function downloadItemBundle() {
    if (!editable || displayedItems.length === 0) return;
    setBusy(true);
    try {
      const bytes = await exportItemBundle(displayedItems, `${editable.name} items`);
      downloadLocalBytes(bytes, `${safeDownloadName(editable.name)}-items.bkitems.json`, 'application/json');
      setNotice({
        tone: 'success',
        text: `${displayedItems.length} item records downloaded in a named, versioned BKVince bundle.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    } finally {
      setBusy(false);
    }
  }

  async function downloadVirtualStashBundle() {
    if (exportableVirtualStashItems.length === 0) return;
    setBusy(true);
    try {
      const bytes = await exportItemBundle(exportableVirtualStashItems, 'BKVince Virtual Stash');
      downloadLocalBytes(bytes, 'BKVince-Virtual-Stash.bkitems.json', 'application/json');
      setNotice({
        tone: 'success',
        text: `${exportableVirtualStashItems.length} Virtual Stash item record${exportableVirtualStashItems.length === 1 ? '' : 's'} exported in a fingerprinted BKVince bundle. Physical saves remain unchanged.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    } finally {
      setBusy(false);
    }
  }

  async function downloadTrashBundle() {
    if (exportableTrashItems.length === 0) return;
    setBusy(true);
    try {
      const bytes = await exportItemBundle(exportableTrashItems, 'BKVince Trash recovery');
      downloadLocalBytes(bytes, 'BKVince-Trash-Recovery.bkitems.json', 'application/json');
      setNotice({
        tone: 'success',
        text: `${exportableTrashItems.length} Trash item record${exportableTrashItems.length === 1 ? '' : 's'} exported as a recovery bundle.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    } finally {
      setBusy(false);
    }
  }

  function edit(update) {
    dispatch({ type: 'edit', update });
  }

  function editHeroGold(key, value) {
    const field = attributeFieldsByKey[key];
    if (!field || !['gold', 'stashed_gold'].includes(key)) return;
    const nextValue = clampInteger(value, 0, field.maximum);
    edit((current) => ({
      ...current,
      attributes: { ...current.attributes, [key]: nextValue },
    }));
  }

  function editChronicle(update) {
    dispatchStash({ type: 'edit', update });
  }

  function addChronicleEntry(request) {
    if (!chronicleEditable) return;
    try {
      editChronicle((current) => addChronicleEntrySnapshot(current, request));
      setNotice({
        tone: 'success',
        text: `${describeChronicleEntry(request.category, request).name} added to Chronicle. Shared Stash pages remain untouched.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    }
  }

  function updateChronicleEntry(request) {
    if (!chronicleEditable) return;
    try {
      editChronicle((current) => updateChronicleEntrySnapshot(current, request));
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    }
  }

  function removeChronicleEntry(request) {
    if (!chronicleEditable) return;
    try {
      const record = chronicleEditable[request.category][request.index];
      editChronicle((current) => removeChronicleEntrySnapshot(current, request));
      setNotice({
        tone: 'success',
        text: `${describeChronicleEntry(request.category, record).name} removed from Chronicle. Undo can restore it.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    }
  }

  function activateItem(index, scope = 'player') {
    setSelectedItemScope(scope);
    setSelectedItemIndex(index);
    setItemEditorOpen(true);
  }

  function setDragActivity(payload) {
    setDraggedItem(payload);
    if (!payload) return;
    setItemEditorOpen(false);
  }

  function beginItemDrag(scope, index, event) {
    const payload = { scope, index };
    event.dataTransfer.effectAllowed = 'move';
    event.dataTransfer.setData(itemDragMime, JSON.stringify(payload));
    event.dataTransfer.setData('text/plain', `${scope}:${index}`);
    event.currentTarget.classList.add('native-dragging');
    setDragActivity(payload);
  }

  function endItemDrag(event) {
    event?.currentTarget?.classList.remove('native-dragging');
    setDraggedItem(null);
    globalThis.document.querySelectorAll('.drag-over').forEach((element) => element.classList.remove('drag-over'));
  }

  function dropPointerItem(payload, destination) {
    const itemIndex = Number(payload.index);
    const targetScope = destination.itemDropScope;
    if (!Number.isInteger(itemIndex) || !itemDropScopesCompatible(
      payload.scope,
      targetScope,
      destination.itemDropKind,
    )) return;
    if (targetScope === 'trash' && destination.itemDropKind === 'trash') {
      if (payload.scope !== 'trash') {
        transferItemAt(payload.scope, itemIndex, 'trash', { autoPlace: true });
      }
      return;
    }
    if (payload.scope !== targetScope) {
      transferItemAt(payload.scope, itemIndex, targetScope, {
        containerId: destination.itemDropContainer,
        x: Number(destination.itemDropX),
        y: Number(destination.itemDropY),
        slotId: destination.itemDropKind === 'equipment'
          ? Number(destination.itemDropSlot)
          : null,
      });
      return;
    }
    if (payload.scope === 'player') {
      if (destination.itemDropKind === 'equipment') {
        equipPlayerItem(itemIndex, Number(destination.itemDropSlot));
      } else {
        movePlayerItem(
          itemIndex,
          destination.itemDropContainer,
          Number(destination.itemDropX),
          Number(destination.itemDropY),
        );
      }
    } else if (payload.scope === 'virtual-stash') {
      moveVirtualStashItemAt(itemIndex, Number(destination.itemDropX), Number(destination.itemDropY));
    } else if (payload.scope === 'trash') {
      moveTrashItemAt(itemIndex, Number(destination.itemDropX), Number(destination.itemDropY));
    } else if (payload.scope === 'shared-stash') {
      moveSharedStashItemAt(itemIndex, Number(destination.itemDropX), Number(destination.itemDropY));
    }
  }

  function editSelectedItem(patch) {
    if (!document || !editable || selectedItemIndex === null) return;
    try {
      const scope = selectedItemScope;
      const scoped = editableForScope(scope);
      const nextScoped = editItemSnapshot(
        scoped,
        modelItemsForScope(),
        selectedItemIndex,
        patch,
      );
      const next = mergeScopeEditable(nextScoped, scope);
      commitScopedEditable(next, scope);
      const nextDisplayedItems = editableItems(
        modelItemsForScope(scope),
        nextScoped.itemEdits,
        nextScoped.addedItems,
      );
      const item = describeItem(nextDisplayedItems[selectedItemIndex], selectedItemIndex);
      setNotice({
        tone: 'success',
        text: `${item.name} item fields updated. Export will write, reparse, and compare the complete item payload.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    }
  }

  function removeSelectedItem() {
    if (!document || !editable || selectedItemIndex === null) return;
    try {
      const scope = selectedItemScope;
      const scoped = editableForScope(scope);
      const descriptor = describeItem(selectedDisplayedItems[selectedItemIndex], selectedItemIndex);
      const nextScoped = removeItemSnapshot(
        scoped,
        modelItemsForScope(scope),
        selectedItemIndex,
      );
      const next = mergeScopeEditable(nextScoped, scope);
      commitScopedEditable(next, scope);
      setItemEditorOpen(false);
      setSelectedItemIndex(null);
      setNotice({
        tone: 'success',
        text: `${descriptor.name} marked for deletion from ${scope === 'mercenary'
          ? 'the mercenary jf block'
          : (scope === 'shared-stash'
            ? `Shared Stash page ${sharedStashPageIndex + 1}`
            : (scope === 'virtual-stash'
              ? 'the temporary Virtual Stash workspace'
              : (scope === 'trash' ? 'Trash' : 'the player item block')))}. Undo can restore it.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    }
  }

  function insertSelectedSocketFiller(type) {
    if (!document || !editable || selectedItemIndex === null) return;
    try {
      const scope = selectedItemScope;
      const scoped = editableForScope(scope);
      const nextScoped = insertSocketFillerSnapshot(scoped, modelItemsForScope(scope), {
        parentIndex: selectedItemIndex,
        type,
        itemLevel: editable.attributes.level,
        reservedItemIds: reservedItemIds(),
      });
      const next = mergeScopeEditable(nextScoped, scope);
      commitScopedEditable(next, scope);
      const filler = nextScoped.itemEdits[selectedItemIndex].socketedItems.at(-1);
      const descriptor = describeItem(filler, filler.position_x);
      setNotice({
        tone: 'success',
        text: `${descriptor.name} inserted into socket ${filler.position_x + 1}. The parent and child payloads will both be reparsed on export.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    }
  }

  async function importSelectedSocketFillers(files) {
    if (!document || !editable || selectedItemIndex === null) return;
    setBusy(true);
    try {
      const importedItems = await importItemFiles(files);
      const scope = selectedItemScope;
      const scoped = editableForScope(scope);
      const nextScoped = insertImportedSocketFillersSnapshot(scoped, modelItemsForScope(scope), {
        parentIndex: selectedItemIndex,
        importedItems,
        reservedItemIds: reservedItemIds(),
      });
      const next = mergeScopeEditable(nextScoped, scope);
      commitScopedEditable(next, scope);
      const firstSocket = nextScoped.itemEdits[selectedItemIndex].socketedItems.length - importedItems.length + 1;
      setNotice({
        tone: 'success',
        text: `${importedItems.length} portable filler${importedItems.length === 1 ? '' : 's'} imported atomically from socket ${firstSocket}; complex quality and properties were preserved with new D2S IDs.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    } finally {
      setBusy(false);
    }
  }

  function extractSelectedSocketFiller(socketIndex) {
    if (!document || !editable || selectedItemIndex === null) return;
    try {
      if (selectedItemScope === 'mercenary') {
        throw new Error('Extract mercenary fillers after moving the item to the player; mercenaries have no inventory container.');
      }
      const scope = selectedItemScope;
      const scoped = editableForScope(scope);
      const nextScoped = extractSocketFillerSnapshot(scoped, modelItemsForScope(scope), {
        parentIndex: selectedItemIndex,
        socketIndex,
        containerId: ['shared-stash', 'virtual-stash', 'trash'].includes(scope) ? 'stash' : 'inventory',
        x: 0,
        y: 0,
      });
      const next = mergeScopeEditable(nextScoped, scope);
      commitScopedEditable(next, scope);
      const extracted = nextScoped.addedItems.at(-1);
      const descriptor = describeItem(extracted, nextScoped.itemEdits.length - 1);
      setNotice({
        tone: 'success',
        text: `${descriptor.name} extracted into the first free ${scope === 'shared-stash'
          ? 'Shared Stash'
          : (scope === 'virtual-stash'
            ? 'Virtual Stash'
            : (scope === 'trash' ? 'Trash' : 'Inventory'))} cell. Remaining fillers were compacted in canonical socket order.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    }
  }

  function deleteSelectedSocketFiller(socketIndex) {
    if (!document || !editable || selectedItemIndex === null) return;
    try {
      const scope = selectedItemScope;
      const scoped = editableForScope(scope);
      const filler = scoped.itemEdits[selectedItemIndex].socketedItems[socketIndex];
      const descriptor = describeItem(filler, socketIndex);
      const nextScoped = removeSocketFillerSnapshot(scoped, modelItemsForScope(scope), {
        parentIndex: selectedItemIndex,
        socketIndex,
      });
      const next = mergeScopeEditable(nextScoped, scope);
      commitScopedEditable(next, scope);
      setNotice({
        tone: 'success',
        text: `${descriptor.name} removed from socket ${socketIndex + 1}. Remaining fillers were compacted; Undo restores the deleted filler.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    }
  }

  function applySelectedRuneword(runewordId, rollMode) {
    if (!document || !editable || selectedItemIndex === null) return;
    try {
      const scope = selectedItemScope;
      const scoped = editableForScope(scope);
      const nextScoped = applyRunewordSnapshot(scoped, modelItemsForScope(scope), {
        parentIndex: selectedItemIndex,
        runewordId,
        rollMode,
        reservedItemIds: reservedItemIds(),
      });
      const next = mergeScopeEditable(nextScoped, scope);
      commitScopedEditable(next, scope);
      const runeword = itemEditorOptions(
        modelItemsForScope(scope)[selectedItemIndex],
        nextScoped.itemEdits[selectedItemIndex],
      ).runewords.find(({ id }) => id === runewordId);
      setNotice({
        tone: 'success',
        text: `${runeword?.name || 'Runeword'} built with its exact BKVince rune order and ${rollMode} governed rolls.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    }
  }

  function clearSelectedRuneword() {
    if (!document || !editable || selectedItemIndex === null) return;
    try {
      const scope = selectedItemScope;
      const scoped = editableForScope(scope);
      const nextScoped = clearRunewordSnapshot(scoped, modelItemsForScope(scope), selectedItemIndex);
      const next = mergeScopeEditable(nextScoped, scope);
      commitScopedEditable(next, scope);
      setNotice({
        tone: 'success',
        text: 'Runeword identity and generated attributes cleared. Socket fillers were preserved for extraction or rebuilding.',
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    }
  }

  function addSelectedManualProperty(request) {
    if (!document || !editable || selectedItemIndex === null) return;
    try {
      const patch = compileManualPropertyPatch(
        selectedSourceItems[selectedItemIndex],
        selectedItemEdits[selectedItemIndex],
        request,
      );
      const scope = selectedItemScope;
      const scoped = editableForScope(scope);
      const nextScoped = editItemSnapshot(scoped, modelItemsForScope(scope), selectedItemIndex, patch);
      const next = mergeScopeEditable(nextScoped, scope);
      commitScopedEditable(next, scope);
      setNotice({
        tone: 'success',
        text: `${request.propertyCode} compiled and appended as governed D2S item attributes.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    }
  }

  function duplicateSelectedItem(count) {
    if (!document || !editable || selectedItemIndex === null) return;
    try {
      if (selectedItemScope === 'mercenary') {
        throw new Error('Move a mercenary item to a player grid before duplicating it.');
      }
      if (selectedItemScope === 'shared-stash' && selectedSharedStashPage?.isStackable) {
        throw new Error('Stackable Shared Stash records cannot be duplicated; only their proven native counters are editable.');
      }
      const scope = selectedItemScope;
      const scoped = editableForScope(scope);
      const nextScoped = duplicateItemSnapshot(scoped, modelItemsForScope(scope), {
        itemIndex: selectedItemIndex,
        count,
        reservedItemIds: reservedItemIds(),
      });
      const next = mergeScopeEditable(nextScoped, scope);
      commitScopedEditable(next, scope);
      const descriptor = describeItem(selectedDisplayedItems[selectedItemIndex], selectedItemIndex);
      setNotice({
        tone: 'success',
        text: `${count} duplicate${count === 1 ? '' : 's'} of ${descriptor.name} created atomically with new D2S IDs. Undo removes the complete duplicate operation.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    }
  }

  function movePlayerItem(itemIndex, containerId, x, y) {
    if (!document || !editable) return;
    try {
      const nextPlacements = moveItemPlacement(
        editable.itemPlacements,
        document.model.items,
        itemIndex,
        containerId,
        x,
        y,
        editable.itemEdits,
        editable.addedItems,
      );
      edit((current) => ({ ...current, itemPlacements: nextPlacements }));
      const item = describeItem(displayedItems[itemIndex], itemIndex);
      const container = itemContainers[containerId];
      const destination = containerId === 'belt'
        ? `${container.label} slot ${x + 1}`
        : `${container.label} ${x + 1},${y + 1}`;
      setNotice({
        tone: 'success',
        text: `${item.name} moved to ${destination}. Export will verify the placement by reparsing.`,
      });
      setSelectedItemIndex(null);
      setDraggedItem(null);
      return true;
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
      return false;
    }
  }

  function moveSelectedItem(containerId, x, y) {
    if (!document || !editable) return;
    if (selectedItemScope !== 'player' || selectedItemIndex === null) {
      setSelectedItemScope('player');
      setSelectedItemIndex(null);
      setItemChooserTarget({ scope: 'player', containerId, x, y });
      return;
    }
    movePlayerItem(selectedItemIndex, containerId, x, y);
  }

  function equipPlayerItem(itemIndex, slotId) {
    if (!document || !editable) return false;
    try {
      const nextPlacements = moveItemToEquipmentSlot(
        editable.itemPlacements,
        document.model.items,
        itemIndex,
        slotId,
        editable.itemEdits,
        editable.addedItems,
      );
      edit((current) => ({ ...current, itemPlacements: nextPlacements }));
      const item = describeItem(displayedItems[itemIndex], itemIndex);
      const slot = equipmentSlotDefinitions.find(({ id }) => id === slotId);
      setNotice({
        tone: 'success',
        text: `${item.name} equipped in ${slot.label}. Export will reparse the native player item block.`,
      });
      setSelectedItemIndex(null);
      setDraggedItem(null);
      return true;
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
      return false;
    }
  }

  function activatePlayerEquipmentSlot(slotId) {
    if (!document || !editable) return;
    if (selectedItemScope !== 'player' || selectedItemIndex === null) {
      setSelectedItemScope('player');
      setSelectedItemIndex(null);
      setItemChooserTarget({ scope: 'player', equipmentSlot: slotId });
      return;
    }
    equipPlayerItem(selectedItemIndex, slotId);
  }

  function selectSharedStashPage(index) {
    setSharedStashPageIndex(index);
    if (selectedItemScope === 'shared-stash') {
      setSelectedItemIndex(null);
      setItemEditorOpen(false);
      setItemChooserTarget(null);
    }
  }

  function selectTopStashMode(mode) {
    setTopStashMode(mode);
    setSelectedItemIndex(null);
    setSelectedItemScope(mode === 'virtual' ? 'virtual-stash' : (mode === 'trash' ? 'trash' : 'player'));
    setItemEditorOpen(false);
    setItemChooserTarget(null);
  }

  function activateVirtualStashItem(index) {
    setSelectedItemScope('virtual-stash');
    setSelectedItemIndex(index);
    setItemEditorOpen(true);
  }

  function moveVirtualStashItemAt(itemIndex, x, y) {
    try {
      const nextPlacements = moveItemPlacement(
        virtualStashEditable.itemPlacements,
        [],
        itemIndex,
        'stash',
        x,
        y,
        virtualStashEditable.itemEdits,
        virtualStashEditable.addedItems,
      );
      dispatchVirtualStash({
        type: 'edit',
        update: (current) => ({ ...current, itemPlacements: nextPlacements }),
      });
      const item = describeItem(displayedVirtualStashItems[itemIndex], itemIndex);
      setNotice({
        tone: 'success',
        text: `${item.name} moved inside the temporary Virtual Stash. Export a bundle or move it into a physical save before leaving.`,
      });
      setSelectedItemIndex(null);
      setDraggedItem(null);
      return true;
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
      return false;
    }
  }

  function moveVirtualStashItem(x, y) {
    if (selectedItemScope !== 'virtual-stash' || selectedItemIndex === null) {
      setSelectedItemScope('virtual-stash');
      setSelectedItemIndex(null);
      setItemChooserTarget({
        scope: 'virtual-stash',
        containerId: 'stash',
        x,
        y,
      });
      return;
    }
    moveVirtualStashItemAt(selectedItemIndex, x, y);
  }

  function activateTrashItem(index) {
    setSelectedItemScope('trash');
    setSelectedItemIndex(index);
    setItemEditorOpen(true);
  }

  function moveTrashItemAt(itemIndex, x, y) {
    try {
      const nextPlacements = moveItemPlacement(
        trashEditable.itemPlacements,
        [],
        itemIndex,
        'stash',
        x,
        y,
        trashEditable.itemEdits,
        trashEditable.addedItems,
      );
      dispatchTrash({
        type: 'edit',
        update: (current) => ({ ...current, itemPlacements: nextPlacements }),
      });
      const item = describeItem(displayedTrashItems[itemIndex], itemIndex);
      setNotice({
        tone: 'success',
        text: `${item.name} rearranged inside Trash. Restore it to a workspace or Undo before emptying Trash.`,
      });
      setSelectedItemIndex(null);
      setDraggedItem(null);
      return true;
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
      return false;
    }
  }

  function moveTrashItem(x, y) {
    if (selectedItemScope !== 'trash' || selectedItemIndex === null) return;
    moveTrashItemAt(selectedItemIndex, x, y);
  }

  function emptyTrash() {
    if (!trashDirty) return;
    const count = trashEditable.itemPlacements.filter(({ removed }) => !removed).length;
    dispatchTrash({
      type: 'edit',
      update: (current) => ({
        ...current,
        itemPlacements: current.itemPlacements.map((placement) => ({ ...placement, removed: true })),
      }),
    });
    setSelectedItemIndex(null);
    setItemEditorOpen(false);
    setNotice({
      tone: 'success',
      text: `${count} Trash item${count === 1 ? '' : 's'} removed from the workspace. Undo restores the entire empty operation.`,
    });
  }

  function emptyPersonalStash() {
    if (!document || !editable) return;
    const count = editable.itemPlacements.filter((placement) => (
      containerForPlacement(placement) === 'stash'
    )).length;
    if (count === 0) return;
    try {
      const next = emptyPersonalStashSnapshot(editable, document.model.items);
      commitScopedEditable(next, 'player');
      setSelectedItemIndex(null);
      setItemEditorOpen(false);
      setNotice({
        tone: 'success',
        text: `${count} personal stash item${count === 1 ? '' : 's'} marked for deletion in one operation. Undo restores the complete stash.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    }
  }

  function activateSharedStashItem(index) {
    setSelectedItemScope('shared-stash');
    setSelectedItemIndex(index);
    setItemEditorOpen(true);
  }

  function moveSharedStashItemAt(itemIndex, x, y) {
    if (!selectedSharedStashPage || !selectedSharedStashPageEditable) return;
    if (selectedSharedStashPage.isStackable) {
      setNotice({
        tone: 'error',
        text: 'Stackable Shared Stash records cannot move or import; only their proven native counters are editable.',
      });
      return;
    }
    try {
      const nextPlacements = moveItemPlacement(
        selectedSharedStashPageEditable.itemPlacements,
        selectedSharedStashPage.items,
        itemIndex,
        'stash',
        x,
        y,
        selectedSharedStashPageEditable.itemEdits,
        selectedSharedStashPageEditable.addedItems,
      );
      const nextPage = { ...selectedSharedStashPageEditable, itemPlacements: nextPlacements };
      const next = mergeScopeEditable(nextPage, 'shared-stash');
      commitScopedEditable(next, 'shared-stash');
      const item = describeItem(displayedSharedStashItems[itemIndex], itemIndex);
      setNotice({
        tone: 'success',
        text: `${item.name} moved on Shared Stash page ${sharedStashPageIndex + 1}. Export will reparse only the changed sector.`,
      });
      setSelectedItemIndex(null);
      setDraggedItem(null);
      return true;
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
      return false;
    }
  }

  function moveSharedStashItem(x, y) {
    if (!selectedSharedStashPage || !selectedSharedStashPageEditable) return;
    if (selectedSharedStashPage.isStackable) {
      setNotice({
        tone: 'error',
        text: 'Stackable Shared Stash records cannot move or import; only their proven native counters are editable.',
      });
      return;
    }
    if (selectedItemScope !== 'shared-stash' || selectedItemIndex === null) {
      setSelectedItemScope('shared-stash');
      setSelectedItemIndex(null);
      setItemChooserTarget({
        scope: 'shared-stash',
        pageIndex: sharedStashPageIndex,
        containerId: 'stash',
        x,
        y,
      });
      return;
    }
    moveSharedStashItemAt(selectedItemIndex, x, y);
  }

  function updateSharedStashCounter(itemIndex, amount) {
    if (!selectedSharedStashPage || !selectedSharedStashPageEditable) return false;
    try {
      const nextPage = editStackableSharedStashCounterSnapshot(
        selectedSharedStashPageEditable,
        selectedSharedStashPage.items,
        itemIndex,
        amount,
      );
      const next = mergeScopeEditable(nextPage, 'shared-stash');
      commitScopedEditable(next, 'shared-stash');
      const item = describeItem(displayedSharedStashItems[itemIndex], itemIndex);
      setNotice({
        tone: 'success',
        text: `${item.name} stackable count set to ${amount}. Only its proven 8-bit counter will be rewritten.`,
      });
      return true;
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
      return false;
    }
  }

  function createMercenary() {
    if (!editable) return;
    try {
      const next = createMercenarySnapshot(editable);
      dispatch({ type: 'edit', update: () => next });
      const definition = mercenaryDefinitions.find(({ id }) => id === next.mercenary.type);
      setNotice({
        tone: 'success',
        text: `${definition?.hireling || 'Mercenary'} created from governed BKVince Hireling defaults. Export will reparse its native header before download.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    }
  }

  function removeMercenary() {
    if (!editable) return;
    try {
      const itemCount = editable.mercItemPlacements.filter((placement) => !placement.removed).length;
      const next = removeMercenarySnapshot(editable);
      dispatch({ type: 'edit', update: () => next });
      if (selectedItemScope === 'mercenary') {
        setItemEditorOpen(false);
        setSelectedItemIndex(null);
        setSelectedItemScope('player');
      }
      setItemChooserTarget(null);
      setNotice({
        tone: 'success',
        text: `Mercenary header${itemCount ? ` and ${itemCount} jf item record${itemCount === 1 ? '' : 's'}` : ''} marked for removal. Undo can restore everything.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    }
  }

  function chooseMercenaryItem(slotId) {
    if (!editable) return;
    if (!editable.mercenary.present) {
      try {
        const next = createMercenarySnapshot(editable);
        dispatch({ type: 'edit', update: () => next });
        const definition = mercenaryDefinitions.find(({ id }) => id === next.mercenary.type);
        setNotice({
          tone: 'success',
          text: `${definition?.hireling || 'Mercenary'} created from governed BKVince defaults. Choose its first item.`,
        });
      } catch (error) {
        setNotice({ tone: 'error', text: error.message });
        return;
      }
    }
    setItemChooserTarget({ scope: 'mercenary', equipmentSlot: slotId });
  }

  function moveMercenaryItem(itemIndex, slotId) {
    if (!document || !editable) return;
    try {
      const scoped = editableForScope('mercenary');
      const nextPlacements = moveItemToEquipmentSlot(
        scoped.itemPlacements,
        modelItemsForScope('mercenary'),
        itemIndex,
        slotId,
        scoped.itemEdits,
        scoped.addedItems,
      );
      const next = mergeScopeEditable({ ...scoped, itemPlacements: nextPlacements }, 'mercenary');
      dispatch({ type: 'edit', update: () => next });
      const item = describeItem(displayedMercItems[itemIndex], itemIndex);
      const slot = equipmentSlotDefinitions.find(({ id }) => id === slotId);
      setNotice({
        tone: 'success',
        text: `${item.name} moved to mercenary ${slot.label}. Export will reparse the jf block.`,
      });
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    }
  }

  function addChosenItems(request) {
    if (!document || !editable || !itemChooserTarget) return;
    const target = itemChooserTarget;
    const isEquipment = Number.isInteger(target.equipmentSlot)
      && (target.scope === 'player' || target.scope === 'mercenary');
    const scope = target.scope === 'mercenary'
      ? 'mercenary'
      : (target.scope === 'shared-stash'
        ? 'shared-stash'
        : (target.scope === 'virtual-stash' ? 'virtual-stash' : 'player'));
    const scoped = editableForScope(scope);
    const scopeItems = modelItemsForScope(scope);
    const firstIndex = scoped.itemPlacements.length;
    const allocatedItemIds = reservedItemIds();
    const nextScoped = isEquipment
      ? addItemToEquipmentSlotSnapshot(scoped, scopeItems, {
        ...request,
        slotId: target.equipmentSlot,
        reservedItemIds: allocatedItemIds,
      })
      : addItemBatchSnapshot(scoped, scopeItems, {
        ...request,
        ...target,
        reservedItemIds: allocatedItemIds,
      });
    const next = mergeScopeEditable(nextScoped, scope);
    commitScopedEditable(next, scope);
    setSelectedItemIndex(null);
    setItemChooserTarget(null);
    const added = editableItems(scopeItems, nextScoped.itemEdits, nextScoped.addedItems);
    const descriptor = describeItem(added.at(-1), firstIndex);
    const destination = isEquipment
      ? `${scope === 'mercenary' ? 'mercenary ' : ''}${equipmentSlotDefinitions.find(({ id }) => id === target.equipmentSlot)?.label}`
      : (scope === 'shared-stash'
        ? `Shared Stash page ${sharedStashPageIndex + 1}`
        : (scope === 'virtual-stash' ? 'the temporary Virtual Stash' : itemContainers[target.containerId].label));
    setNotice({
      tone: 'success',
      text: `${request.count}× ${descriptor.name} added atomically to ${destination}. Export will write and reparse every new record.`,
    });
  }

  function addChosenGroup(request) {
    if (!document || !editable || !itemChooserTarget) return;
    const target = itemChooserTarget;
    if (Number.isInteger(target.equipmentSlot) || target.containerId === 'belt') {
      setNotice({ tone: 'error', text: 'Quick groups cannot be placed into one equipment or belt slot.' });
      return;
    }
    const scope = target.scope === 'shared-stash'
      ? 'shared-stash'
      : (target.scope === 'virtual-stash' ? 'virtual-stash' : 'player');
    const scoped = editableForScope(scope);
    const scopeItems = modelItemsForScope(scope);
    const beforeCount = scoped.itemPlacements.length;
    const nextScoped = addItemGroupSnapshot(scoped, scopeItems, {
      ...request,
      ...target,
      reservedItemIds: reservedItemIds(),
    });
    const next = mergeScopeEditable(nextScoped, scope);
    commitScopedEditable(next, scope);
    setSelectedItemIndex(null);
    setItemChooserTarget(null);
    const addedCount = nextScoped.itemPlacements.length - beforeCount;
    const destination = scope === 'shared-stash'
      ? `Shared Stash page ${sharedStashPageIndex + 1}`
      : (scope === 'virtual-stash' ? 'the temporary Virtual Stash' : itemContainers[target.containerId].label);
    setNotice({
      tone: 'success',
      text: `${request.label}: ${addedCount} item${addedCount === 1 ? '' : 's'} added atomically to ${destination}. Undo removes the complete group.`,
    });
  }

  function addChosenCatalogBatch(request) {
    if (!document || !editable || !itemChooserTarget) return;
    const target = itemChooserTarget;
    if (Number.isInteger(target.equipmentSlot) || target.containerId === 'belt') {
      setNotice({ tone: 'error', text: 'A custom batch cannot be placed into one equipment or belt slot.' });
      return;
    }
    const scope = target.scope === 'shared-stash'
      ? 'shared-stash'
      : (target.scope === 'virtual-stash' ? 'virtual-stash' : 'player');
    const scoped = editableForScope(scope);
    const scopeItems = modelItemsForScope(scope);
    const beforeCount = scoped.itemPlacements.length;
    const nextScoped = addCatalogItemBatchSnapshot(scoped, scopeItems, {
      ...request,
      ...target,
      reservedItemIds: reservedItemIds(),
    });
    const next = mergeScopeEditable(nextScoped, scope);
    commitScopedEditable(next, scope);
    setSelectedItemIndex(null);
    setItemChooserTarget(null);
    const addedCount = nextScoped.itemPlacements.length - beforeCount;
    const destination = scope === 'shared-stash'
      ? `Shared Stash page ${sharedStashPageIndex + 1}`
      : (scope === 'virtual-stash' ? 'the temporary Virtual Stash' : itemContainers[target.containerId].label);
    setNotice({
      tone: 'success',
      text: `${addedCount} custom catalog item${addedCount === 1 ? '' : 's'} added atomically to ${destination}. Undo removes the complete batch.`,
    });
  }

  async function importChosenItems(files) {
    if (!document || !editable || !itemChooserTarget) return;
    const target = itemChooserTarget;
    setBusy(true);
    try {
      const importedItems = await importItemFiles(files);
      const isEquipment = Number.isInteger(target.equipmentSlot)
        && (target.scope === 'player' || target.scope === 'mercenary');
      const scope = target.scope === 'mercenary'
        ? 'mercenary'
        : (target.scope === 'shared-stash'
          ? 'shared-stash'
          : (target.scope === 'virtual-stash' ? 'virtual-stash' : 'player'));
      const scoped = editableForScope(scope);
      const scopeItems = modelItemsForScope(scope);
      const allocatedItemIds = reservedItemIds();
      const nextScoped = isEquipment
        ? addImportedItemToEquipmentSlotSnapshot(scoped, scopeItems, {
          importedItems,
          slotId: target.equipmentSlot,
          reservedItemIds: allocatedItemIds,
        })
        : addImportedItemsSnapshot(scoped, scopeItems, {
          importedItems,
          reservedItemIds: allocatedItemIds,
          ...target,
        });
      const next = mergeScopeEditable(nextScoped, scope);
      commitScopedEditable(next, scope);
      setSelectedItemIndex(null);
      setItemChooserTarget(null);
      const destination = isEquipment
        ? `${scope === 'mercenary' ? 'mercenary ' : ''}${equipmentSlotDefinitions.find(({ id }) => id === target.equipmentSlot)?.label}`
        : (scope === 'shared-stash'
          ? `Shared Stash page ${sharedStashPageIndex + 1}`
          : (scope === 'virtual-stash' ? 'the temporary Virtual Stash' : itemContainers[target.containerId].label));
      setNotice({
        tone: 'success',
        text: `${importedItems.length} portable item${importedItems.length === 1 ? '' : 's'} imported atomically into ${destination}; new D2S IDs were allocated.`,
      });
      return { count: importedItems.length, destination };
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
      throw error;
    } finally {
      setBusy(false);
    }
  }

  return (
    <div className="app-shell">
      <header className={`topbar ${document && editable ? 'loaded' : ''}`}>
        {document && editable ? (
          <div className="rune-hero-summary">
            <ClassPortrait className={editable.className} />
            <div>
              <strong>{editable.name}</strong>
              <span>{editable.className} · level {editable.attributes.level}</span>
              <small>BKVince · D2S v105</small>
            </div>
          </div>
        ) : (
          <div className="brand">
            <div className="brand-mark" aria-hidden="true">BK</div>
            <div><strong>BKVince</strong><span>Hero Editor</span></div>
          </div>
        )}
        <div className="top-actions">
          <input
            ref={fileInput}
            className="visually-hidden"
            type="file"
            accept=".d2s,application/octet-stream"
            onChange={(event) => loadFile(event.target.files?.[0])}
          />
          <input
            ref={sharedStashInput}
            className="visually-hidden"
            type="file"
            accept=".d2i,application/octet-stream"
            onChange={(event) => loadSharedStash(event.target.files?.[0])}
          />
          <button
            className="button ghost load-create-button"
            type="button"
            aria-label="Load / create"
            onClick={() => setCreateOpen(true)}
            disabled={busy}
          >
            <span className="load-create-icon" aria-hidden="true"><PlusIcon /></span>
            <span>Load / create</span>
          </button>
          <div className="history-actions" aria-label="Edit history">
            <button
              className="icon-button"
              type="button"
              aria-label="Undo"
              aria-keyshortcuts="Control+Z Meta+Z"
              title="Undo (Ctrl+Z)"
              disabled={busy || activeHistory.past.length === 0}
              onClick={() => performHistoryAction(activeHistoryKind, 'undo')}
            >
              <UndoIcon />
            </button>
            <button
              className="icon-button"
              type="button"
              aria-label="Redo"
              aria-keyshortcuts="Control+Shift+Z Meta+Shift+Z Control+Y Meta+Y"
              title="Redo (Ctrl+Shift+Z or Ctrl+Y)"
              disabled={busy || activeHistory.future.length === 0}
              onClick={() => performHistoryAction(activeHistoryKind, 'redo')}
            >
              <RedoIcon />
            </button>
          </div>
        </div>
      </header>

      <div className="rune-page">
        <main className="rune-main">
          <div
            className={`notice ${notice.tone} ${document && editable && notice.tone !== 'error' ? 'workspace-toast' : ''}`}
            role="status"
          >
            <span className="notice-dot" />
            {notice.text}
          </div>

          {!document || !editable ? (
            <EmptyState
              busy={busy}
              onOpen={() => fileInput.current?.click()}
              onCreate={() => setCreateOpen(true)}
            />
          ) : (
            <>
              <RuneItemOverview
                characterLevel={editable.attributes.level}
                carriedGold={editable.attributes.gold}
                stashedGold={editable.attributes.stashed_gold}
                onGoldChange={editHeroGold}
                items={displayedItems}
                placements={editable.itemPlacements}
                selectedItemIndex={selectedItemScope === 'player' ? selectedItemIndex : null}
                selectionItems={selectedDisplayedItems}
                selectionItemIndex={itemEditorOpen ? null : selectedItemIndex}
                draggedItem={draggedItem}
                onSelectItem={activateItem}
                onMoveItem={moveSelectedItem}
                onEquipmentSlot={activatePlayerEquipmentSlot}
                onStartItemDrag={beginItemDrag}
                onEndItemDrag={endItemDrag}
                onDragActivity={setDragActivity}
                onPointerDrop={dropPointerItem}
                onEditItem={() => setItemEditorOpen(true)}
                onClearSelection={() => setSelectedItemIndex(null)}
                onDownloadBundle={topStashMode === 'virtual'
                  ? downloadVirtualStashBundle
                  : (topStashMode === 'trash' ? downloadTrashBundle : downloadItemBundle)}
                mercenaryItems={displayedMercItems}
                mercenaryPlacements={editable.mercItemPlacements}
                mercenarySelectedItemIndex={selectedItemScope === 'mercenary' ? selectedItemIndex : null}
                onSelectMercenaryItem={(index) => activateItem(index, 'mercenary')}
                onAddMercenaryItem={chooseMercenaryItem}
                onMoveMercenaryItem={moveMercenaryItem}
                onLoadSharedStash={() => sharedStashInput.current?.click()}
                onEmptyPersonalStash={emptyPersonalStash}
                stashMode={topStashMode}
                onStashModeChange={selectTopStashMode}
                virtualItems={displayedVirtualStashItems}
                virtualPlacements={virtualStashEditable.itemPlacements}
                virtualSelectedItemIndex={selectedItemScope === 'virtual-stash' ? selectedItemIndex : null}
                virtualDirty={virtualStashDirty}
                virtualHistory={virtualStashHistory}
                onSelectVirtualItem={activateVirtualStashItem}
                onMoveVirtualItem={moveVirtualStashItem}
                onUndoVirtual={() => performHistoryAction('virtual', 'undo')}
                onRedoVirtual={() => performHistoryAction('virtual', 'redo')}
                trashItems={displayedTrashItems}
                trashPlacements={trashEditable.itemPlacements}
                trashSelectedItemIndex={selectedItemScope === 'trash' ? selectedItemIndex : null}
                trashDirty={trashDirty}
                trashHistory={trashHistory}
                onSelectTrashItem={activateTrashItem}
                onMoveTrashItem={moveTrashItem}
                onEmptyTrash={emptyTrash}
                onUndoTrash={() => performHistoryAction('trash', 'undo')}
                onRedoTrash={() => performHistoryAction('trash', 'redo')}
              />
              <section className="rune-stats-section">
                <div className="rune-section-title">
                  <h1>Stats</h1>
                  <span className={`dirty-badge ${isDirty ? 'changed' : ''}`}>
                    {isDirty ? 'Unsaved changes' : 'Source preserved'}
                  </span>
                </div>
                <div className="rune-editor-frame" data-section={activeTab}>
                  <nav className="rune-editor-nav" aria-label="Character editor sections">
                    {navigation.map((entry) => (
                      <button
                        key={entry.id}
                        type="button"
                        className={activeTab === entry.id ? 'active' : ''}
                        disabled={!entry.enabled}
                        onClick={() => setActiveTab(entry.id)}
                      >
                        <NavigationIcon type={entry.id} />
                        <span>{entry.label}</span>
                        {entry.badge && <small>{entry.badge}</small>}
                      </button>
                    ))}
                  </nav>
                  <div className="rune-editor-content">
                    {!['general', 'quests', 'waypoints', 'item-bonuses', 'skills', 'chronicle', 'mercenary'].includes(activeTab) && (
                      <div className="rune-content-heading">
                        <h2>{pageContent[activeTab]?.title || 'General'}</h2>
                        <p>{pageContent[activeTab]?.description}</p>
                      </div>
                    )}
                    {activeTab === 'general' && (
                      <div className="rune-general-stack">
                        <GeneralEditor editable={editable} edit={edit} />
                      </div>
                    )}
                    {activeTab === 'quests' && <QuestsEditor editable={editable} edit={edit} />}
                    {activeTab === 'waypoints' && <WaypointsEditor editable={editable} edit={edit} />}
                    {activeTab === 'item-bonuses' && (
                      <ItemBonusesEditor dashboard={playerBonusDashboard} />
                    )}
                    {activeTab === 'skills' && <SkillsEditor editable={editable} edit={edit} />}
                    {activeTab === 'chronicle' && (
                      <>
                        {sharedStashDocument && <h2 className="chronicle-page-title">Chronicle</h2>}
                        <div className="shared-stash-editor-stack">
                          <SharedStashInventoryEditor
                            characterLevel={editable.attributes.level}
                            document={sharedStashDocument}
                            editable={sharedStashItemsEditable}
                            dirty={sharedStashItemsDirty}
                            history={stashItemsHistory}
                            pageIndex={sharedStashPageIndex}
                            items={displayedSharedStashItems}
                            selectedItemIndex={selectedItemScope === 'shared-stash' ? selectedItemIndex : null}
                            draggedItem={draggedItem}
                            busy={busy}
                            onLoad={() => sharedStashInput.current?.click()}
                            onDownload={downloadSharedStash}
                            onPageChange={selectSharedStashPage}
                            onSelectItem={activateSharedStashItem}
                            onMoveItem={moveSharedStashItem}
                            onStartItemDrag={beginItemDrag}
                            onEndItemDrag={endItemDrag}
                            onDragActivity={setDragActivity}
                            onPointerDrop={dropPointerItem}
                            onUpdateStackableCounter={updateSharedStashCounter}
                            onUndo={() => performHistoryAction('shared-stash', 'undo')}
                            onRedo={() => performHistoryAction('shared-stash', 'redo')}
                          />
                          <ChronicleEditor
                            document={sharedStashDocument}
                            editable={chronicleEditable}
                            dirty={chronicleDirty}
                            busy={busy}
                            onLoad={() => sharedStashInput.current?.click()}
                            onDownload={downloadSharedStash}
                            onAdd={addChronicleEntry}
                            onUpdate={updateChronicleEntry}
                            onRemove={removeChronicleEntry}
                            onEdit={editChronicle}
                          />
                        </div>
                      </>
                    )}
                    {activeTab === 'mercenary' && (
                      <MercenaryEditor
                        editable={editable}
                        onEdit={edit}
                        onCreate={createMercenary}
                        onRemove={removeMercenary}
                      />
                    )}
                    {activeTab === 'demon' && (
                      <DemonEditor editable={editable} onEdit={edit} />
                    )}
                  </div>
                </div>
              </section>
              <div className="rune-save-note" role="note">
                BKVince heroes stay on D2S v105 so custom classes, quests, skills, mercenaries, and items remain intact.
              </div>
              <div className="rune-save-footer">
                <select aria-label="Save file version" defaultValue="BKVince v105">
                  <option value="BKVince v105">BKVince v105</option>
                </select>
                <button className="button rune-save-button" type="button" onClick={downloadCopy} disabled={busy}>
                  <DownloadIcon /> Save to file
                </button>
              </div>
            </>
          )}
        </main>
      </div>

      {createOpen && (
        <AccessibleModal
          className="load-create-modal"
          labelledBy="create-title"
          onClose={() => setCreateOpen(false)}
        >
            <div className="modal-heading">
              <h2 id="create-title">Load or Create a Character</h2>
              <button className="icon-button" type="button" aria-label="Close" onClick={() => setCreateOpen(false)}>×</button>
            </div>
            <button
              className="load-character-card"
              type="button"
              data-dialog-initial-focus
              onClick={() => {
                setCreateOpen(false);
                fileInput.current?.click();
              }}
              disabled={busy}
            >
              <FolderOpenIcon />
              <span>Load Character from Saved Games Folder</span>
            </button>
            <section className="create-character-section" aria-labelledby="create-character-heading">
              <h3 id="create-character-heading">Create a new Character</h3>
              <div className="create-class-grid">
                {classes.map(({ name }) => (
                  <button
                    className="create-class-card"
                    type="button"
                    key={name}
                    onClick={() => createHero(name)}
                    disabled={busy}
                  >
                    <img
                      src={classPortraitVisuals[name.toLocaleLowerCase('en-US')]}
                      alt=""
                      draggable="false"
                    />
                    <span>{name}</span>
                  </button>
                ))}
              </div>
            </section>
        </AccessibleModal>
      )}
      {itemEditorOpen && document && editable && selectedItemIndex !== null && (
        <ItemEditorModal
          characterLevel={editable.attributes.level}
          item={selectedSourceItems[selectedItemIndex]}
          edit={selectedItemEdits[selectedItemIndex]}
          onChange={editSelectedItem}
          onInsertSocket={insertSelectedSocketFiller}
          onImportSocket={importSelectedSocketFillers}
          onExtractSocket={selectedItemScope === 'mercenary' ? null : extractSelectedSocketFiller}
          onDeleteSocket={deleteSelectedSocketFiller}
          onApplyRuneword={applySelectedRuneword}
          onClearRuneword={clearSelectedRuneword}
          onAddManualProperty={addSelectedManualProperty}
          onDuplicate={selectedItemScope === 'mercenary' ? null : duplicateSelectedItem}
          onDownload={downloadSelectedItem}
          transferTargets={transferTargetsForScope(selectedItemScope)
            .filter((target) => target.scope !== 'trash')}
          onTransfer={transferSelectedItem}
          onDelete={selectedItemScope === 'trash'
            ? removeSelectedItem
            : () => transferSelectedItem('trash')}
          deleteLabel={selectedItemScope === 'trash' ? 'Delete permanently' : 'Move to Trash'}
          busy={busy}
          onClose={() => {
            setItemEditorOpen(false);
            setSelectedItemIndex(null);
          }}
        />
      )}
      {itemChooserTarget && editable && (
        <AddItemModal
          key={Number.isInteger(itemChooserTarget.equipmentSlot)
            ? `${itemChooserTarget.scope || 'player'}-${itemChooserTarget.equipmentSlot}`
            : `${itemChooserTarget.scope || 'player'}-${itemChooserTarget.pageIndex ?? 'hero'}-${itemChooserTarget.containerId}-${itemChooserTarget.x}-${itemChooserTarget.y}`}
          bases={Number.isInteger(itemChooserTarget.equipmentSlot)
            ? availableEquipmentItemBases(itemChooserTarget.equipmentSlot)
            : (itemChooserTarget.containerId === 'belt' ? availableBeltItemBases() : addableItems)}
          namedItems={namedItems}
          runewordItems={runewordItems}
          groups={Number.isInteger(itemChooserTarget.equipmentSlot) || itemChooserTarget.containerId === 'belt' ? [] : itemGroups}
          target={itemChooserTarget}
          defaultItemLevel={PERFECT_ITEM_LEVEL}
          onAdd={addChosenItems}
          onAddGroup={addChosenGroup}
          onAddCatalogBatch={addChosenCatalogBatch}
          onImport={importChosenItems}
          onClose={() => setItemChooserTarget(null)}
        />
      )}
    </div>
  );
}

function GoldControl({ label, value, gameMaximum, encodedMaximum, onChange }) {
  const overGameMaximum = value > gameMaximum;
  return (
    <div className={`rune-gold-control ${overGameMaximum ? 'over-game-maximum' : ''}`}>
      <GoldIcon />
      <label>
        <span className="visually-hidden">{label}</span>
        <input
          type="number"
          inputMode="numeric"
          aria-label={label}
          value={value}
          min={0}
          max={encodedMaximum}
          title={`${label}: ${formatNumber(value)}`}
          onChange={(event) => onChange(clampInteger(event.target.value, 0, encodedMaximum))}
        />
      </label>
      <button
        type="button"
        aria-label={`Set ${label.toLowerCase()} to game maximum ${gameMaximum}`}
        title={`Game maximum: ${formatNumber(gameMaximum)}`}
        onClick={() => onChange(gameMaximum)}
      >
        Max
      </button>
    </div>
  );
}

function RuneItemOverview({
  characterLevel,
  carriedGold,
  stashedGold,
  onGoldChange,
  items,
  placements,
  selectedItemIndex,
  selectionItems,
  selectionItemIndex,
  draggedItem,
  onSelectItem,
  onMoveItem,
  onEquipmentSlot,
  onStartItemDrag,
  onEndItemDrag,
  onDragActivity,
  onPointerDrop,
  onEditItem,
  onClearSelection,
  onDownloadBundle,
  mercenaryItems,
  mercenaryPlacements,
  mercenarySelectedItemIndex,
  onSelectMercenaryItem,
  onAddMercenaryItem,
  onMoveMercenaryItem,
  onLoadSharedStash,
  onEmptyPersonalStash,
  stashMode,
  onStashModeChange,
  virtualItems,
  virtualPlacements,
  virtualSelectedItemIndex,
  virtualDirty,
  virtualHistory,
  onSelectVirtualItem,
  onMoveVirtualItem,
  onUndoVirtual,
  onRedoVirtual,
  trashItems,
  trashPlacements,
  trashSelectedItemIndex,
  trashDirty,
  trashHistory,
  onSelectTrashItem,
  onMoveTrashItem,
  onEmptyTrash,
  onUndoTrash,
  onRedoTrash,
}) {
  const [equipmentMode, setEquipmentMode] = useState('player');
  const equipped = placements.filter((placement) => containerForPlacement(placement) === 'equipment');
  const mercenaryEquipped = mercenaryPlacements.filter(
    (placement) => containerForPlacement(placement) === 'equipment',
  );
  const belt = placements.filter((placement) => containerForPlacement(placement) === 'belt');
  const beltCapacity = beltCapacityForPlacements(placements, items);
  const carriedGoldMaximum = carriedGoldGameMaximum(characterLevel);
  const virtualActiveCount = virtualPlacements.filter(({ removed }) => !removed).length;
  const trashActiveCount = trashPlacements.filter(({ removed }) => !removed).length;
  const personalStashActiveCount = placements.filter((placement) => (
    containerForPlacement(placement) === 'stash'
  )).length;
  const globalTrashDropReady = Boolean(
    draggedItem
    && draggedItem.scope !== 'trash'
    && itemDropScopesCompatible(draggedItem.scope, 'trash', 'trash'),
  );
  return (
    <section className="rune-item-overview">
      {selectionItemIndex !== null && (
        <ItemSelectionBar
          characterLevel={characterLevel}
          items={selectionItems}
          selectedItemIndex={selectionItemIndex}
          emptyText="Select an item to edit it or place it in a free Inventory, Stash, or Cube cell."
          action={(
            <div className="selection-actions">
              <button className="button ghost compact" type="button" onClick={onEditItem}>Edit item</button>
              <button className="button ghost compact" type="button" aria-keyshortcuts="Escape" title="Cancel item movement (Esc)" onClick={onClearSelection}>Cancel selection</button>
            </div>
          )}
        />
      )}
      <div className="rune-storage-layout">
        <div className="rune-storage-column equipment-column">
          <h2>Equipment</h2>
          <div className="rune-storage-toolbar">
            <div className="rune-subtabs" role="tablist" aria-label="Equipment owner">
              <button
                className={equipmentMode === 'player' ? 'active' : ''}
                type="button"
                role="tab"
                aria-selected={equipmentMode === 'player'}
                onClick={() => setEquipmentMode('player')}
              >
                Player
              </button>
              <button
                className={equipmentMode === 'mercenary' ? 'active' : ''}
                type="button"
                role="tab"
                aria-selected={equipmentMode === 'mercenary'}
                onClick={() => setEquipmentMode('mercenary')}
              >
                Mercenary
              </button>
            </div>
            <GoldControl
              label="Carried gold"
              value={carriedGold}
              gameMaximum={carriedGoldMaximum}
              encodedMaximum={attributeFieldsByKey.gold.maximum}
              onChange={(value) => onGoldChange('gold', value)}
            />
          </div>
          <section
            className={`rune-equipment-board ${equipmentMode === 'mercenary' ? 'mercenary-board' : 'player-board'}`}
            aria-label="Equipment slots"
            style={{
              '--equipment-panel-visual': `url("${equipmentMode === 'mercenary'
                ? mercenaryEquipmentPanelVisual
                : equipmentPanelVisual}")`,
            }}
          >
            <div className="equipment-layout">
              {(equipmentMode === 'mercenary' ? mercenaryEquipmentSlots : equipmentSlots).map(([slot, label]) => {
                const activeItems = equipmentMode === 'mercenary' ? mercenaryItems : items;
                const activeEquipped = equipmentMode === 'mercenary' ? mercenaryEquipped : equipped;
                const placement = activeEquipped.find((candidate) => candidate.equippedId === slot);
                if (equipmentMode === 'mercenary') {
                  return (
                    <div
                      className={`equipment-slot slot-${slot} ${placement ? 'occupied' : 'addable'}`}
                      key={slot}
                      style={!placement ? {
                        '--equipment-slot-placeholder': `url("${equipmentSlotPlaceholderVisuals[slot]}")`,
                      } : undefined}
                      onDragOver={(event) => {
                        if (event.dataTransfer.types.includes('application/x-bkvince-merc-item')) {
                          event.preventDefault();
                          event.dataTransfer.dropEffect = 'move';
                        }
                      }}
                      onDrop={(event) => {
                        const rawIndex = event.dataTransfer.getData('application/x-bkvince-merc-item');
                        if (!rawIndex) return;
                        event.preventDefault();
                        onMoveMercenaryItem(Number(rawIndex), slot);
                      }}
                    >
                      <span>{label}</span>
                      {placement ? (
                        <ItemToken
                          characterLevel={characterLevel}
                          item={activeItems[placement.index]}
                          placement={placement}
                          selected={mercenarySelectedItemIndex === placement.index}
                          onSelect={() => onSelectMercenaryItem(placement.index)}
                          draggable
                          onDragStart={(event) => {
                            event.dataTransfer.effectAllowed = 'move';
                            event.dataTransfer.setData('application/x-bkvince-merc-item', String(placement.index));
                          }}
                        />
                      ) : (
                        <button
                          className="equipment-add-action"
                          type="button"
                          aria-label={`Add an item to mercenary ${label}`}
                          title={`Click to add compatible mercenary ${label} gear`}
                          onClick={() => onAddMercenaryItem(slot)}
                        >
                          <PlusIcon />
                          <strong>Click to add</strong>
                        </button>
                      )}
                    </div>
                  );
                }
                const acceptsDraggedItem = !placement && Boolean(draggedItem) && itemDropScopesCompatible(
                  draggedItem.scope,
                  'player',
                  'equipment',
                );
                return (
                  <div
                    className={`equipment-slot slot-${slot} ${placement ? 'occupied' : 'addable'} ${acceptsDraggedItem ? 'drop-ready' : ''}`}
                    key={slot}
                    style={!placement ? {
                      '--equipment-slot-placeholder': `url("${equipmentSlotPlaceholderVisuals[slot]}")`,
                    } : undefined}
                    data-item-drop-target={!placement ? 'true' : undefined}
                    data-item-drop-scope={!placement ? 'player' : undefined}
                    data-item-drop-kind={!placement ? 'equipment' : undefined}
                    data-item-drop-slot={!placement ? slot : undefined}
                    onDragEnter={(event) => {
                      if (!acceptsDraggedItem) return;
                      event.preventDefault();
                      event.currentTarget.classList.add('drag-over');
                    }}
                    onDragOver={(event) => {
                      if (!acceptsDraggedItem) return;
                      event.preventDefault();
                      event.dataTransfer.dropEffect = 'move';
                    }}
                    onDragLeave={(event) => event.currentTarget.classList.remove('drag-over')}
                    onDrop={(event) => {
                      event.currentTarget.classList.remove('drag-over');
                      if (!acceptsDraggedItem) return;
                      event.preventDefault();
                      onPointerDrop?.(draggedItem, {
                        itemDropScope: 'player',
                        itemDropKind: 'equipment',
                        itemDropSlot: String(slot),
                      });
                    }}
                  >
                    <span>{label}</span>
                    {placement ? (
                      <ItemToken
                        characterLevel={characterLevel}
                        item={items[placement.index]}
                        placement={placement}
                        selected={selectedItemIndex === placement.index}
                        onSelect={() => onSelectItem(placement.index)}
                        draggable
                        dragPayload={{ scope: 'player', index: placement.index }}
                        onDragStart={(event) => onStartItemDrag('player', placement.index, event)}
                        onDragEnd={onEndItemDrag}
                        onDragActivity={onDragActivity}
                        onPointerDrop={onPointerDrop}
                      />
                    ) : (
                      <button
                        className="equipment-add-action"
                        type="button"
                        aria-label={selectedItemIndex === null ? `Add an item to ${label}` : `Equip selected item in ${label}`}
                        title={selectedItemIndex === null ? `Click to add compatible ${label} gear` : `Equip selected item in ${label}`}
                        onClick={() => onEquipmentSlot(slot)}
                      >
                        <PlusIcon />
                        <strong>{selectedItemIndex === null ? 'Click to add' : 'Place here'}</strong>
                      </button>
                    )}
                  </div>
                );
              })}
            </div>
          </section>
          {equipmentMode === 'player' && (
            <ContainerGrid
              characterLevel={characterLevel}
              container={itemContainers.inventory}
              items={items}
              placements={placements}
              selectedItemIndex={selectedItemIndex}
              onSelectItem={onSelectItem}
              onMoveItem={onMoveItem}
              scope="player"
              draggedItem={draggedItem}
              onStartItemDrag={onStartItemDrag}
              onEndItemDrag={onEndItemDrag}
              onDragActivity={onDragActivity}
              onPointerDrop={onPointerDrop}
            />
          )}
        </div>
        <div className="rune-storage-column stash-column">
          <h2>Stash</h2>
          <div className="rune-storage-toolbar">
            <div className="rune-subtabs" aria-label="Stash workspaces">
              <button className={stashMode === 'stash' ? 'active' : ''} type="button" onClick={() => onStashModeChange('stash')}>Stash</button>
              <button className={stashMode === 'virtual' ? 'active' : ''} type="button" onClick={() => onStashModeChange('virtual')}>Virtual Stash</button>
              <button className={stashMode === 'trash' ? 'active' : ''} type="button" onClick={() => onStashModeChange('trash')}>Trash</button>
            </div>
            <GoldControl
              label="Stashed gold"
              value={stashedGold}
              gameMaximum={goldLimits.stashedMaximum}
              encodedMaximum={attributeFieldsByKey.stashed_gold.maximum}
              onChange={(value) => onGoldChange('stashed_gold', value)}
            />
          </div>
          {stashMode === 'virtual' ? (
            <>
              <div className="virtual-stash-note">
                <div><strong>Temporary item workspace</strong><span>Not written into the D2S or Shared Stash.</span></div>
                <small>{virtualDirty ? `${virtualActiveCount} item${virtualActiveCount === 1 ? '' : 's'} waiting` : 'Empty workspace'}</small>
              </div>
              <ContainerGrid
                characterLevel={characterLevel}
                container={{ ...itemContainers.stash, label: 'Virtual Stash' }}
                items={virtualItems}
                placements={virtualPlacements}
                selectedItemIndex={virtualSelectedItemIndex}
                onSelectItem={onSelectVirtualItem}
                onMoveItem={(_containerId, x, y) => onMoveVirtualItem(x, y)}
                scope="virtual-stash"
                draggedItem={draggedItem}
                onStartItemDrag={onStartItemDrag}
                onEndItemDrag={onEndItemDrag}
                onDragActivity={onDragActivity}
                onPointerDrop={onPointerDrop}
              />
              <div className="virtual-stash-actions">
                <button className="button ghost compact" type="button" onClick={onDownloadBundle} disabled={virtualActiveCount === 0}>
                  Export virtual bundle
                </button>
                <div className="history-actions" aria-label="Virtual Stash history">
                  <button className="icon-button" type="button" title="Undo Virtual Stash edit" aria-label="Undo Virtual Stash edit" disabled={virtualHistory.past.length === 0} onClick={onUndoVirtual}><UndoIcon /></button>
                  <button className="icon-button" type="button" title="Redo Virtual Stash edit" aria-label="Redo Virtual Stash edit" disabled={virtualHistory.future.length === 0} onClick={onRedoVirtual}><RedoIcon /></button>
                </div>
              </div>
            </>
          ) : (stashMode === 'trash' ? (
            <>
              <div className="virtual-stash-note trash-note">
                <div><strong>Recoverable Trash</strong><span>Items remain session-local until restored, exported, or emptied.</span></div>
                <small>{trashDirty ? `${trashActiveCount} item${trashActiveCount === 1 ? '' : 's'} recoverable` : 'Trash is empty'}</small>
              </div>
              <ContainerGrid
                characterLevel={characterLevel}
                container={{ ...itemContainers.stash, label: 'Trash' }}
                items={trashItems}
                placements={trashPlacements}
                selectedItemIndex={trashSelectedItemIndex}
                onSelectItem={onSelectTrashItem}
                onMoveItem={(_containerId, x, y) => onMoveTrashItem(x, y)}
                scope="trash"
                draggedItem={draggedItem}
                onStartItemDrag={onStartItemDrag}
                onEndItemDrag={onEndItemDrag}
                onDragActivity={onDragActivity}
                onPointerDrop={onPointerDrop}
              />
              <div className="virtual-stash-actions trash-actions">
                <button className="button ghost compact" type="button" onClick={onDownloadBundle} disabled={trashActiveCount === 0}>
                  Export recovery bundle
                </button>
                <button className="button danger compact" type="button" onClick={onEmptyTrash} disabled={trashActiveCount === 0}>
                  Empty Trash
                </button>
                <div className="history-actions" aria-label="Trash history">
                  <button className="icon-button" type="button" title="Undo Trash edit" aria-label="Undo Trash edit" disabled={trashHistory.past.length === 0} onClick={onUndoTrash}><UndoIcon /></button>
                  <button className="icon-button" type="button" title="Redo Trash edit" aria-label="Redo Trash edit" disabled={trashHistory.future.length === 0} onClick={onRedoTrash}><RedoIcon /></button>
                </div>
              </div>
            </>
          ) : (
            <>
              <ContainerGrid
                characterLevel={characterLevel}
                container={itemContainers.stash}
                items={items}
                placements={placements}
                selectedItemIndex={selectedItemIndex}
                onSelectItem={onSelectItem}
                onMoveItem={onMoveItem}
                scope="player"
                draggedItem={draggedItem}
                onStartItemDrag={onStartItemDrag}
                onEndItemDrag={onEndItemDrag}
                onDragActivity={onDragActivity}
                onPointerDrop={onPointerDrop}
              />
              <div className="rune-stash-actions">
                <button
                  className="button ghost compact"
                  type="button"
                  onClick={onEmptyPersonalStash}
                  disabled={personalStashActiveCount === 0}
                >
                  <TrashIcon /> Empty personal stash
                </button>
                <button className="button ghost compact" type="button" onClick={onLoadSharedStash}>
                  Load Shared Stash
                </button>
              </div>
            </>
          ))}
        </div>
        <div className="rune-storage-column compact-storage-column">
          <h2>Cube</h2>
          <ContainerGrid
            characterLevel={characterLevel}
            container={itemContainers.cube}
            items={items}
            placements={placements}
            selectedItemIndex={selectedItemIndex}
            onSelectItem={onSelectItem}
            onMoveItem={onMoveItem}
            scope="player"
            draggedItem={draggedItem}
            onStartItemDrag={onStartItemDrag}
            onEndItemDrag={onEndItemDrag}
            onDragActivity={onDragActivity}
            onPointerDrop={onPointerDrop}
          />
          <h2 className="belt-title">Belt</h2>
          <DisplayGrid
            characterLevel={characterLevel}
            columns={4}
            rows={4}
            capacity={beltCapacity}
            items={items}
            placements={belt}
            selectedItemIndex={selectedItemIndex}
            onSelectItem={onSelectItem}
            onMoveItem={onMoveItem}
            scope="player"
            draggedItem={draggedItem}
            onStartItemDrag={onStartItemDrag}
            onEndItemDrag={onEndItemDrag}
            onDragActivity={onDragActivity}
            onPointerDrop={onPointerDrop}
            emptyLabel={`Belt grid · ${beltCapacity} of 16 slots usable`}
          />
          <button
            className={`global-trash-target ${stashMode === 'trash' ? 'active' : ''} ${globalTrashDropReady ? 'drop-ready' : ''}`}
            type="button"
            aria-label={trashActiveCount > 0
              ? `Open Trash workspace with ${trashActiveCount} recoverable item${trashActiveCount === 1 ? '' : 's'}`
              : 'Open empty Trash workspace'}
            title={globalTrashDropReady ? 'Drop item into recoverable Trash' : 'Open recoverable Trash'}
            data-item-drop-target={globalTrashDropReady ? 'true' : undefined}
            data-item-drop-scope={globalTrashDropReady ? 'trash' : undefined}
            data-item-drop-kind={globalTrashDropReady ? 'trash' : undefined}
            onClick={() => onStashModeChange('trash')}
            onDragEnter={(event) => {
              if (!globalTrashDropReady) return;
              event.preventDefault();
              event.currentTarget.classList.add('drag-over');
            }}
            onDragOver={(event) => {
              if (!globalTrashDropReady) return;
              event.preventDefault();
              event.dataTransfer.dropEffect = 'move';
            }}
            onDragLeave={(event) => event.currentTarget.classList.remove('drag-over')}
            onDrop={(event) => {
              event.currentTarget.classList.remove('drag-over');
              if (!globalTrashDropReady) return;
              event.preventDefault();
              onPointerDrop?.(draggedItem, {
                itemDropScope: 'trash',
                itemDropKind: 'trash',
              });
            }}
          >
            <TrashIcon />
            {trashActiveCount > 0 && <span>{trashActiveCount}</span>}
          </button>
        </div>
      </div>
    </section>
  );
}

const chronicleSections = [
  { id: 'setItems', label: 'Set Items' },
  { id: 'uniqueItems', label: 'Unique Items' },
  { id: 'runewords', label: 'Runewords' },
];

function SharedStashInventoryEditor({
  characterLevel,
  document,
  editable,
  dirty,
  history,
  pageIndex,
  items,
  selectedItemIndex,
  draggedItem,
  busy,
  onLoad,
  onDownload,
  onPageChange,
  onSelectItem,
  onMoveItem,
  onStartItemDrag,
  onEndItemDrag,
  onDragActivity,
  onPointerDrop,
  onUpdateStackableCounter,
  onUndo,
  onRedo,
}) {
  if (!document || !editable) return null;
  const page = document.pages[pageIndex];
  const pageEditable = editable.pages[pageIndex];
  if (!page || !pageEditable) return null;
  const activePlacements = pageEditable.itemPlacements.filter(({ removed }) => !removed);
  const filledPages = editable.pages
    .map((candidate, index) => ({
      index,
      count: candidate.itemPlacements.filter(({ removed }) => !removed).length,
    }))
    .filter(({ count }) => count > 0);
  const totalItems = editable.pages.reduce(
    (total, candidate) => total + candidate.itemPlacements.filter(({ removed }) => !removed).length,
    0,
  );
  const stackableItems = page.isStackable
    ? items.map((item, index) => ({ item, descriptor: describeItem(item, index, characterLevel) }))
    : [];

  const selectPage = (index) => onPageChange(Math.max(0, Math.min(document.pageCount - 1, index)));

  return (
    <section className="shared-stash-inventory">
      <div className="shared-stash-heading">
        <div>
          <p className="eyebrow">Native D2I storage</p>
          <h3>Shared Stash</h3>
          <span>
            {document.pageCount.toLocaleString('en-US')} pages · {totalItems.toLocaleString('en-US')} items · untouched sectors remain byte-exact
          </span>
        </div>
        <div className="shared-stash-heading-actions">
          <span className={`dirty-badge ${dirty ? 'changed' : ''}`}>{dirty ? 'Pages modified' : 'Source preserved'}</span>
          <button className="button ghost compact" type="button" onClick={onLoad} disabled={busy}>Replace file</button>
          <button className="button primary compact" type="button" onClick={onDownload} disabled={busy}>
            <DownloadIcon /> Download Shared Stash
          </button>
        </div>
      </div>

      <div className="shared-stash-toolbar">
        <div className="rune-subtabs shared-stash-tabs" aria-label="Shared Stash storage modes">
          <span className="active">Shared</span>
          <span>Virtual Stash</span>
          <span className={page.isStackable ? 'active stackable' : ''}>Stackable</span>
        </div>
        <div className="shared-stash-pager" aria-label="Shared Stash page navigation">
          <button className="icon-button" type="button" aria-label="Previous page" disabled={pageIndex === 0} onClick={() => selectPage(pageIndex - 1)}>‹</button>
          <label>
            <span>Page</span>
            <input
              type="number"
              min="1"
              max={document.pageCount}
              value={pageIndex + 1}
              onChange={(event) => selectPage(Number(event.target.value || 1) - 1)}
            />
            <small>of {document.pageCount.toLocaleString('en-US')}</small>
          </label>
          <button className="icon-button" type="button" aria-label="Next page" disabled={pageIndex >= document.pageCount - 1} onClick={() => selectPage(pageIndex + 1)}>›</button>
          {filledPages.length > 0 && (
            <button
              className="button ghost compact"
              type="button"
              onClick={() => {
                const nextFilled = filledPages.find(({ index }) => index > pageIndex) || filledPages[0];
                selectPage(nextFilled.index);
              }}
            >
              Next filled
            </button>
          )}
        </div>
        <div className="shared-stash-history" aria-label="Shared Stash item history">
          <button className="icon-button" type="button" title="Undo Shared Stash item edit" aria-label="Undo Shared Stash item edit" disabled={busy || history.past.length === 0} onClick={onUndo}><UndoIcon /></button>
          <button className="icon-button" type="button" title="Redo Shared Stash item edit" aria-label="Redo Shared Stash item edit" disabled={busy || history.future.length === 0} onClick={onRedo}><RedoIcon /></button>
        </div>
      </div>

      <div className="shared-stash-page-meta">
        <div><span>Page {pageIndex + 1}</span><strong>{page.isStackable ? 'Stackable storage' : 'Shared storage'}</strong></div>
        <div><span>Items</span><strong>{activePlacements.length.toLocaleString('en-US')}</strong></div>
        <div><span>Gold</span><strong>{page.gold.toLocaleString('en-US')}</strong></div>
        <small>{page.isStackable ? 'Native counters editable · records and coordinates locked' : 'Click an empty cell to add · click an item to edit'}</small>
      </div>

      {page.isStackable ? (
        <div className="stackable-stash-panel">
          <div className="rune-info-note">
            The BKVince 8-bit native counter is proven and editable from 0 to 255. Record identity, properties, imports, deletions, and overlapping coordinates stay locked.
          </div>
          <div className="stackable-stash-list">
            {stackableItems.map(({ descriptor }, index) => (
              <div className="stackable-stash-item" key={`${descriptor.type}-${index}`}>
                <span className={`item-quality quality-${descriptor.quality?.toLowerCase() || 'normal'}`}>◆</span>
                <div><strong>{descriptor.name}</strong><small>{descriptor.type} · native record {index + 1}</small></div>
                <StackableCounterInput
                  amount={pageEditable.itemEdits[index].amountInSharedStash}
                  busy={busy}
                  itemName={descriptor.name}
                  onCommit={(amount) => onUpdateStackableCounter(index, amount)}
                />
              </div>
            ))}
            {stackableItems.length === 0 && <p className="chronicle-no-records">This stackable page contains no item records.</p>}
          </div>
        </div>
      ) : (
        <div className="shared-stash-grid">
          <ContainerGrid
            characterLevel={characterLevel}
            container={{ ...itemContainers.stash, label: `Shared Stash · Page ${pageIndex + 1}` }}
            items={items}
            placements={pageEditable.itemPlacements}
            selectedItemIndex={selectedItemIndex}
            onSelectItem={onSelectItem}
            onMoveItem={(_containerId, x, y) => onMoveItem(x, y)}
            scope="shared-stash"
            draggedItem={draggedItem}
            onStartItemDrag={onStartItemDrag}
            onEndItemDrag={onEndItemDrag}
            onDragActivity={onDragActivity}
            onPointerDrop={onPointerDrop}
          />
        </div>
      )}
    </section>
  );
}

function StackableCounterInput({ amount, busy, itemName, onCommit }) {
  const [draft, setDraft] = useState(String(amount));

  useEffect(() => setDraft(String(amount)), [amount]);

  const commit = (nextDraft = draft) => {
    const next = Number(nextDraft);
    if (!Number.isInteger(next) || next < 0 || next > 255 || !onCommit(next)) {
      setDraft(String(amount));
      return;
    }
    setDraft(String(next));
  };

  const nudge = (offset) => {
    const next = Math.max(0, Math.min(255, Number(amount) + offset));
    setDraft(String(next));
    commit(String(next));
  };

  return (
    <div className="stackable-counter" role="group" aria-label={`${itemName} native counter`}>
      <button type="button" disabled={busy || amount <= 0} aria-label={`Decrease ${itemName} counter`} onClick={() => nudge(-1)}>−</button>
      <label>
        <span>Count</span>
        <input
          type="number"
          min="0"
          max="255"
          step="1"
          value={draft}
          disabled={busy}
          aria-label={`${itemName} shared count`}
          onChange={(event) => setDraft(event.target.value)}
          onBlur={() => commit()}
          onKeyDown={(event) => {
            if (event.key === 'Enter') event.currentTarget.blur();
            if (event.key === 'Escape') {
              setDraft(String(amount));
              event.currentTarget.blur();
            }
          }}
        />
      </label>
      <button type="button" disabled={busy || amount >= 255} aria-label={`Increase ${itemName} counter`} onClick={() => nudge(1)}>+</button>
    </div>
  );
}

function ChronicleEditor({
  document,
  editable,
  dirty,
  busy,
  onLoad,
  onDownload,
  onAdd,
  onUpdate,
  onRemove,
  onEdit,
}) {
  const [category, setCategory] = useState('uniqueItems');
  const [search, setSearch] = useState('');
  const [catalogSearch, setCatalogSearch] = useState('');
  const [selectedItemId, setSelectedItemId] = useState(
    String(chronicleCatalog.uniqueItems[0]?.itemId ?? ''),
  );
  const [monster, setMonster] = useState('0');
  const [foundAt, setFoundAt] = useState(() => toDatetimeLocal(Math.floor(Date.now() / 60000) * 60));

  if (!document || !editable) {
    return (
      <section className="chronicle-empty">
        <h2>Chronicle</h2>
        <button className="button ghost compact" type="button" onClick={onLoad} disabled={busy}>
          Load Shared Stash
        </button>
      </section>
    );
  }

  const catalog = chronicleCatalog[category].filter((item) => item.spawnable !== false);
  const catalogNeedle = catalogSearch.trim().toLocaleLowerCase('en-US');
  const filteredCatalog = catalog.filter((item) => (
    !catalogNeedle
    || item.name.toLocaleLowerCase('en-US').includes(catalogNeedle)
    || String(item.itemId).includes(catalogNeedle)
    || String(item.baseCode || '').toLocaleLowerCase('en-US').includes(catalogNeedle)
  ));
  const effectiveSelectedItemId = filteredCatalog.some(({ itemId }) => String(itemId) === selectedItemId)
    ? selectedItemId
    : String(filteredCatalog[0]?.itemId ?? '');
  const recordNeedle = search.trim().toLocaleLowerCase('en-US');
  const rows = editable[category]
    .map((record, index) => ({ record, index, descriptor: describeChronicleEntry(category, record) }))
    .filter(({ record, descriptor }) => (
      !recordNeedle
      || descriptor.name.toLocaleLowerCase('en-US').includes(recordNeedle)
      || String(record.itemId).includes(recordNeedle)
      || String(record.monster).includes(recordNeedle)
    ));
  const totalDiscoveries = chronicleDiscoveryCount(editable);
  const monsterNumber = Number(monster);
  const discoveryDraftValid = foundAt !== ''
    && Number.isInteger(monsterNumber)
    && monsterNumber >= 0
    && monsterNumber <= 65535;
  const missingCount = catalog.filter(
    (item) => !editable[category].some((record) => record.itemId === item.itemId),
  ).length;

  function switchCategory(nextCategory) {
    setCategory(nextCategory);
    setCatalogSearch('');
    setSearch('');
    setSelectedItemId(String(chronicleCatalog[nextCategory][0]?.itemId ?? ''));
  }

  function submitEntry(event) {
    event.preventDefault();
    if (effectiveSelectedItemId === '') return;
    onAdd({
      category,
      itemId: Number(effectiveSelectedItemId),
      monster: monsterNumber,
      foundAt: fromDatetimeLocal(foundAt),
    });
  }

  function completeCategory() {
    const timestamp = fromDatetimeLocal(foundAt);
    onEdit((current) => catalog.reduce((next, item) => (
      next[category].some((record) => record.itemId === item.itemId)
        ? next
        : addChronicleEntrySnapshot(next, {
          category,
          itemId: item.itemId,
          monster: monsterNumber,
          foundAt: timestamp,
        })
    ), current));
  }

  return (
    <div className="chronicle-editor">
      <section className="chronicle-summary">
        <div>
          <span>Shared Stash</span>
          <strong>{document.fileName}</strong>
          <small>{document.pageCount.toLocaleString()} storage pages preserved byte-for-byte</small>
        </div>
        <div className="chronicle-summary-metric">
          <span>Discoveries</span>
          <strong>{totalDiscoveries.toLocaleString()}</strong>
          <small>{dirty ? 'Chronicle modified' : 'Source preserved'}</small>
        </div>
        <div className="chronicle-summary-actions">
          <button className="button ghost compact" type="button" onClick={onLoad} disabled={busy}>Replace file</button>
          <button className="button primary compact" type="button" onClick={onDownload} disabled={busy}>
            <DownloadIcon /> Download Shared Stash
          </button>
        </div>
      </section>

      <div className="chronicle-tabs" role="tablist" aria-label="Chronicle item categories">
        {chronicleSections.map((section) => (
          <button
            key={section.id}
            type="button"
            role="tab"
            aria-selected={category === section.id}
            className={category === section.id ? 'active' : ''}
            onClick={() => switchCategory(section.id)}
          >
            {section.label}
            <span>{editable[section.id].length.toLocaleString()}</span>
          </button>
        ))}
      </div>

      <section className="chronicle-add-panel">
        <div className="chronicle-panel-heading">
          <div>
            <h3>Add a discovery</h3>
            <p>Choose directly from the governed BKVince catalog; existing unknown IDs remain preserved.</p>
          </div>
          <button
            className="button ghost compact"
            type="button"
            onClick={completeCategory}
            disabled={busy || missingCount === 0 || !discoveryDraftValid}
          >
            Complete category ({missingCount.toLocaleString()})
          </button>
        </div>
        <form className="chronicle-add-form" onSubmit={submitEntry}>
          <label className="field chronicle-catalog-search">
            <span>Find an item</span>
            <input
              value={catalogSearch}
              placeholder="Name, ID, or base code"
              onChange={(event) => setCatalogSearch(event.target.value)}
            />
          </label>
          <label className="field chronicle-item-select">
            <span>BKVince item</span>
            <select
              value={effectiveSelectedItemId}
              onChange={(event) => setSelectedItemId(event.target.value)}
              disabled={filteredCatalog.length === 0}
            >
              {filteredCatalog.map((item) => (
                <option key={item.itemId} value={item.itemId}>
                  {item.name} · #{item.itemId}
                </option>
              ))}
            </select>
          </label>
          <label className="field chronicle-monster-field">
            <span>Monster ID</span>
            <input
              type="number"
              min="0"
              max="65535"
              required
              value={monster}
              onChange={(event) => setMonster(event.target.value)}
            />
          </label>
          <label className="field chronicle-date-field">
            <span>Found at</span>
            <input type="datetime-local" value={foundAt} required onChange={(event) => setFoundAt(event.target.value)} />
          </label>
          <button className="button primary chronicle-add-button" type="submit" disabled={busy || !effectiveSelectedItemId || !discoveryDraftValid}>
            <PlusIcon /> Add discovery
          </button>
        </form>
      </section>

      <section className="chronicle-records">
        <div className="chronicle-panel-heading">
          <div>
            <h3>{chronicleSections.find((section) => section.id === category)?.label}</h3>
            <p>{editable[category].length.toLocaleString()} recorded · {missingCount.toLocaleString()} catalog entries missing</p>
          </div>
          <label className="chronicle-record-search">
            <span className="visually-hidden">Search discoveries</span>
            <input
              value={search}
              placeholder="Search discoveries"
              onChange={(event) => setSearch(event.target.value)}
            />
          </label>
        </div>
        <div className="chronicle-record-table" role="table" aria-label="Chronicle discoveries">
          <div className="chronicle-record-head" role="row">
            <span>Discovery</span><span>Monster ID</span><span>Found at</span><span>Action</span>
          </div>
          {rows.length === 0 ? (
            <p className="chronicle-no-records">No discoveries match this view.</p>
          ) : rows.map(({ record, index, descriptor }) => (
            <div className="chronicle-record-row" role="row" key={`${category}-${record.itemId}`}>
              <div className="chronicle-record-name">
                <strong className={descriptor.known ? '' : 'unknown'}>{descriptor.name}</strong>
                <span>Record #{record.itemId}{descriptor.baseCode ? ` · ${descriptor.baseCode}` : ''}</span>
              </div>
              <input
                aria-label={`${descriptor.name} monster ID`}
                type="number"
                min="0"
                max="65535"
                defaultValue={record.monster}
                key={`monster-${record.itemId}-${record.monster}`}
                onBlur={(event) => onUpdate({
                  category,
                  index,
                  monster: Number(event.target.value),
                })}
              />
              <input
                aria-label={`${descriptor.name} discovery time`}
                type="datetime-local"
                defaultValue={toDatetimeLocal(record.foundAt)}
                key={`date-${record.itemId}-${record.foundAt}`}
                onBlur={(event) => onUpdate({
                  category,
                  index,
                  foundAt: fromDatetimeLocal(event.target.value),
                })}
              />
              <button
                className="chronicle-remove"
                type="button"
                aria-label={`Remove ${descriptor.name}`}
                title="Remove discovery"
                onClick={() => onRemove({ category, index })}
              >
                ×
              </button>
            </div>
          ))}
        </div>
      </section>
    </div>
  );
}

function CharacterCard({ document, editable, dirty }) {
  return (
    <div className="character-card">
      <ClassPortrait className={editable?.className} />
      <div className="character-meta">
        <span>{document ? editable.className : 'No hero'}</span>
        <strong>{document ? editable.name : 'Open or create'}</strong>
        <small>{document ? `Level ${editable.attributes.level}` : 'D2S v105 · BKVince'}</small>
      </div>
      {document && <span className={`status-orb ${dirty ? 'dirty' : ''}`} title={dirty ? 'Modified' : 'Unchanged'} />}
    </div>
  );
}

function EmptyState({ busy, onOpen, onCreate }) {
  return (
    <section className="empty-state">
      <div className="empty-emblem" aria-hidden="true">BK</div>
      <p className="eyebrow">BKVince-compatible workspace</p>
      <h1>Bring a hero back to life.</h1>
      <p>
        Open an existing D2S or start from a blank class save. Everything is parsed, edited, and exported locally.
      </p>
      <div className="empty-actions">
        <button className="button primary" type="button" onClick={onOpen} disabled={busy}>
          <UploadIcon /> Open D2S
        </button>
        <button className="button ghost" type="button" onClick={onCreate} disabled={busy}>
          <PlusIcon /> Create blank hero
        </button>
      </div>
      <div className="empty-facts">
        <span><CheckIcon /> Checksum checked</span>
        <span><CheckIcon /> Byte-exact no-op</span>
        <span><CheckIcon /> No uploads</span>
      </div>
    </section>
  );
}

function GeneralEditor({ editable, edit }) {
  const attributeColumns = [
    [
      ['strength', 'Strength'],
      ['dexterity', 'Dexterity'],
      ['energy', 'Energy'],
      ['vitality', 'Vitality'],
    ],
    [
      ['experience', 'Experience'],
      ['unused_stats', 'Unused Stats'],
      ['unused_skill_points', 'Unused Skill Points'],
    ],
    [
      ['current_hp', 'Current Hp'],
      ['max_hp', 'Max Hp'],
      ['current_mana', 'Current Mana'],
      ['max_mana', 'Max Mana'],
      ['max_stamina', 'Max Stamina'],
    ],
  ];

  function updateAttribute(key, value) {
    edit((current) => ({
      ...current,
      attributes: { ...current.attributes, [key]: value },
    }));
  }

  return (
    <section className="rune-general-card" aria-label="General character data">
      <label className="field rune-general-name">
        <span>Name (max 15 char)</span>
        <input
          value={editable.name}
          maxLength={15}
          onChange={(event) => edit((current) => ({ ...current, name: event.target.value }))}
        />
      </label>

      <div className="rune-general-identity-row">
        <NumberField
          label="Level"
          value={editable.attributes.level}
          min={1}
          max={99}
          onChange={(value) => updateAttribute('level', value)}
        />
        <NumberField
          label="Map Seed"
          value={editable.mapId}
          min={0}
          max={0xffffffff}
          onChange={(value) => edit((current) => ({ ...current, mapId: value }))}
        />
      </div>

      <div className="rune-general-stat-columns">
        {attributeColumns.map((column, columnIndex) => (
          <div className="rune-general-stat-column" key={`general-column-${columnIndex}`}>
            {column.map(([key, label]) => (
              <NumberField
                key={key}
                label={label}
                value={editable.attributes[key]}
                min={0}
                max={attributeFieldsByKey[key].maximum}
                onChange={(value) => updateAttribute(key, value)}
              />
            ))}
          </div>
        ))}
      </div>

      <div className="rune-general-toggles">
        {[
          ['expansion', 'Expansion'],
          ['hardcore', 'Hardcore'],
          ['died', 'Died'],
          ['ladder', 'Ladder'],
        ].map(([key, label]) => (
          <Toggle
            key={key}
            label={label}
            checked={editable[key]}
            onChange={(checked) => edit((current) => ({ ...current, [key]: checked }))}
          />
        ))}
      </div>

      <div className="rune-info-note rune-general-note">
        <span className="rune-general-info-icon" aria-hidden="true">!</span>
        <span>
          Character flags and values are written to their native D2S fields, then checksum and edited values are verified
          by reparsing before download.
        </span>
      </div>
    </section>
  );
}

const itemBonusGroups = [
  {
    id: 'attributes',
    title: 'Attributes',
    rows: [
      ['strength', 'Strength', 'number'],
      ['dexterity', 'Dexterity', 'number'],
      ['energy', 'Energy', 'number'],
      ['vitality', 'Vitality', 'number'],
      ['life', 'Life', 'number', true],
      ['mana', 'Mana', 'number'],
    ],
  },
  {
    id: 'resistances',
    title: 'Resistances',
    rows: [
      ['fire', 'Fire Res', 'percent'],
      ['lightning', 'Lightning Res', 'percent'],
      ['cold', 'Cold Res', 'percent'],
      ['poison', 'Poison Res', 'percent'],
      ['magic', 'Magic Res', 'percent', true],
      ['physical', 'Physical Res', 'percent'],
    ],
  },
  {
    id: 'breakpoints',
    title: 'Breakpoints',
    rows: [
      ['fasterCastRate', 'FCR', 'percent'],
      ['fasterHitRecovery', 'FHR', 'percent'],
      ['fasterBlockRate', 'FBR', 'percent'],
      ['increasedAttackSpeed', 'IAS', 'percent'],
    ],
  },
  {
    id: 'misc',
    title: 'Misc',
    rows: [
      ['allSkills', 'All Skills', 'signed'],
      ['magicFind', 'Magic Find', 'percent'],
      ['goldFind', 'Gold Find', 'percent'],
    ],
  },
];

function formatItemBonusMetric(value, format) {
  if (format === 'percent') return `${value}%`;
  if (format === 'signed') return `${value >= 0 ? '+' : ''}${value}`;
  return String(value);
}

function ItemBonusesEditor({ dashboard }) {
  return (
    <div className="item-bonuses-editor">
      {itemBonusGroups.map((group) => (
        <section className="item-bonus-column" key={group.id}>
          <h3>{group.title}</h3>
          <span className="item-bonus-spacer small" aria-hidden="true" />
          {group.rows.map(([key, label, format, separated]) => (
            <div className={`item-bonus-metric ${separated ? 'separated' : ''}`} key={key}>
              <span>{label}:</span>
              <output>{formatItemBonusMetric(dashboard[group.id][key], format)}</output>
            </div>
          ))}
        </section>
      ))}
    </div>
  );
}

function DemonEditor({ editable, onEdit }) {
  const [search, setSearch] = useState('');
  const demon = editable.demon;
  if (!demon.present) {
    return (
      <div className="demon-empty-state">
        <div className="demon-seal" aria-hidden="true">D</div>
        <h3>You don&apos;t have a bound demon.</h3>
        <p>
          BKVince has not written a native <code>lf</code> Demon payload in this save.
          The editor will not fabricate its undocumented opaque bytes.
        </p>
        <div className="rune-info-note">
          Bind a demon in BKVince, save the hero, then reopen that D2S here to edit it.
        </div>
      </div>
    );
  }

  const catalog = demon.isSuperUnique
    ? demonDefinitions.superUniques
    : demonDefinitions.monsters;
  const catalogKind = demon.isSuperUnique ? 'Super Unique' : 'Monster';
  const current = catalog.find(({ index }) => index === demon.index) || null;
  const needle = search.trim().toLocaleLowerCase('en-US');
  const matches = needle
    ? catalog.filter((entry) => (
      entry.name.toLocaleLowerCase('en-US').includes(needle)
      || entry.id.toLocaleLowerCase('en-US').includes(needle)
      || String(entry.index).includes(needle)
    )).slice(0, 160)
    : catalog;
  const visible = current && !matches.some(({ index }) => index === current.index)
    ? [current, ...matches]
    : matches;
  const modifiers = demonDefinitions.modifiers;

  const updateDemon = (patch) => onEdit((snapshot) => ({
    ...snapshot,
    demon: { ...snapshot.demon, ...patch },
  }));
  const updateModifier = (index, value) => onEdit((snapshot) => {
    const mods = [...snapshot.demon.mods];
    mods[index] = value;
    return { ...snapshot, demon: { ...snapshot.demon, mods } };
  });

  return (
    <div className="demon-editor">
      <section className="demon-identity panel">
        <div className="demon-heading">
          <div>
            <p className="eyebrow">Bound Demon · Work in Progress</p>
            <h3>{current?.name || `Unknown ${catalogKind} index ${demon.index}`}</h3>
            <span>{current ? `${current.id} · native index ${current.index}` : 'Unknown native identity preserved'}</span>
          </div>
          <div className="demon-badges">
            <small>{catalogKind}</small>
            {demon.isDesecrated && <small className="terrorized">Terrorized</small>}
          </div>
        </div>

        <div className="demon-toggle-row">
          <Toggle
            label="Terrorized"
            checked={demon.isDesecrated}
            onChange={(isDesecrated) => updateDemon({ isDesecrated })}
          />
          <Toggle
            label="Super Unique"
            checked={demon.isSuperUnique}
            onChange={(isSuperUnique) => updateDemon({ isSuperUnique })}
          />
        </div>

        <div className="demon-monster-picker">
          <label className="field demon-search-field">
            <span>Search the BKVince {catalogKind.toLocaleLowerCase('en-US')} catalog</span>
            <input
              type="search"
              value={search}
              placeholder="Name, internal ID, or native index"
              onChange={(event) => setSearch(event.target.value)}
            />
            <small>{matches.length.toLocaleString('en-US')} of {catalog.length.toLocaleString('en-US')} entries</small>
          </label>
          <label className="field demon-select-field">
            <span>{catalogKind}</span>
            <select
              value={demon.index}
              onChange={(event) => updateDemon({ index: Number(event.target.value) })}
            >
              {!current && <option value={demon.index}>Unknown native index {demon.index}</option>}
              {visible.map((entry) => (
                <option value={entry.index} key={`${entry.id}-${entry.index}`}>
                  {entry.name} · {entry.id} · #{entry.index}
                </option>
              ))}
            </select>
            <small>Index is sourced from the governed BKVince table row order.</small>
          </label>
        </div>
      </section>

      <section className="demon-modifier-panel panel">
        <div className="demon-section-heading">
          <div><h3>Mods</h3><p>RuneWizard exposes the first six native modifier slots.</p></div>
          <small>6 editable · 3 preserved</small>
        </div>
        <div className="demon-modifier-grid">
          {demon.mods.slice(0, 6).map((modifierId, index) => {
            const known = modifiers.some(({ id }) => id === modifierId);
            return (
              <label className="field" key={index}>
                <span>Modifier {index + 1}</span>
                <select
                  value={modifierId}
                  onChange={(event) => updateModifier(index, Number(event.target.value))}
                >
                  {!known && <option value={modifierId}>Unknown modifier #{modifierId}</option>}
                  {modifiers.map((modifier) => (
                    <option value={modifier.id} key={modifier.id}>
                      {modifier.label} · #{modifier.id}
                    </option>
                  ))}
                </select>
              </label>
            );
          })}
        </div>
      </section>

      <div className="rune-info-note demon-preservation-note">
        Difficulty, area, level, trailing stats, seven opaque byte blocks, and native modifier slots 7–9 are preserved and verified after reparse.
      </div>
    </div>
  );
}

function MercenaryEditor({
  editable,
  onEdit,
  onCreate,
  onRemove,
}) {
  const mercenary = editable.mercenary;
  const definition = mercenaryDefinitions.find(({ id }) => id === mercenary.type) || null;
  return (
    <div className="mercenary-editor">
      <div className="mercenary-heading">
        <h2>Mercenary <span>(Experimental)</span></h2>
      </div>
      <div className="mercenary-primary-controls">
        <Toggle
          label="died"
          checked={mercenary.present && mercenary.dead}
          onChange={(dead) => onEdit((current) => {
            const next = current.mercenary.present ? current : createMercenarySnapshot(current);
            return { ...next, mercenary: { ...next.mercenary, dead } };
          })}
        />
        {!mercenary.present && (
          <button className="button ghost compact" type="button" onClick={onCreate}>
            Create mercenary
          </button>
        )}
      </div>
      {mercenary.present && (
        <details className="mercenary-native-details">
          <summary>BKVince native hireling record</summary>
          <div className="mercenary-native-panel">
            <div className="mercenary-fields">
              <label className="field item-quality-field">
                <span>Hireling</span>
                <select
                  value={mercenary.type}
                  onChange={(event) => onEdit((current) => ({
                    ...current,
                    mercenary: { ...current.mercenary, type: Number(event.target.value) },
                  }))}
                >
                  {mercenaryDefinitions.map((entry) => (
                    <option value={entry.id} key={entry.id}>
                      Act {entry.act} · {entry.hireling} · {entry.subtype}
                    </option>
                  ))}
                </select>
              </label>
              <NumberField
                label="Name ID"
                value={mercenary.nameId}
                min={0}
                max={0xffff}
                onChange={(nameId) => onEdit((current) => ({
                  ...current,
                  mercenary: { ...current.mercenary, nameId },
                }))}
              />
              <NumberField
                label="Experience"
                value={mercenary.experience}
                min={0}
                max={0xffffffff}
                onChange={(experience) => onEdit((current) => ({
                  ...current,
                  mercenary: { ...current.mercenary, experience },
                }))}
              />
              <label className="field mercenary-id-field">
                <span>Native ID</span>
                <input value={mercenary.id.toUpperCase()} readOnly />
              </label>
            </div>
            {definition && (
              <div className="mercenary-definition">
                <span>Act {definition.act}</span>
                <span>Difficulty {definition.difficulty}</span>
                <span>Start XP {definition.startingExperience.toLocaleString('en-US')}</span>
                <span>Hire levels {definition.minimumLevel}–{definition.maximumLevel}</span>
                <span>{definition.skills.length} governed skills</span>
                <div>{definition.skills.map((skill) => <small key={skill}>{skill}</small>)}</div>
              </div>
            )}
            <button className="button danger compact mercenary-remove-action" type="button" onClick={onRemove}>
              Remove mercenary
            </button>
          </div>
        </details>
      )}
    </div>
  );
}

function QuestsEditor({ editable, edit }) {
  return (
    <div className="rune-data-editor quest-editor">
      <div className="rune-data-actions quest-unlock-action">
        <button type="button" onClick={() => edit((current) => unlockHellSnapshot(current))}>
          Unlock Hell
        </button>
      </div>
      <div className="rune-info-note quest-editor-note">
        <span className="rune-general-info-icon" aria-hidden="true">!</span>
        <span>
          Resetting a quest reward without resetting the quest is not possible currently because the save file only shows us
          if a quest is completed or not. Claiming a reward is required to complete the quest, this means that when the quest
          shows as completed, the reward is also claimed.
        </span>
      </div>
      <div className="rune-data-actions quest-bulk-actions">
        <button type="button" onClick={() => edit((current) => setAllQuestsSnapshot(current, true))}>
          Complete All
        </button>
        <button type="button" onClick={() => edit((current) => setAllQuestsSnapshot(current, false))}>
          Reset All
        </button>
      </div>
      {difficultyDefinitions.map((difficulty) => (
        <section className="rune-difficulty-block" key={difficulty.id}>
          <h3>{difficulty.label}</h3>
          <div className="quest-act-grid">
            {questActs.map((act) => (
              <article className="rune-act-card" key={act.id}>
                <h4>{act.label}</h4>
                <div className="compact-toggle-list">
                  {act.quests.map((quest) => (
                    <div className="quest-toggle-wrap" key={quest.id}>
                      <CompactToggle
                        leading={(
                          <QuestVisual
                            code={quest.iconCode}
                            completed={editable.quests[difficulty.id][act.id][quest.id].is_completed}
                            label={quest.label}
                          />
                        )}
                        label={quest.label}
                        checked={editable.quests[difficulty.id][act.id][quest.id].is_completed}
                        onChange={(checked) => edit((current) => (
                          setQuestCompletionSnapshot(current, difficulty.id, act.id, quest.id, checked)
                        ))}
                      />
                      {quest.consumedScroll && (
                        editable.quests[difficulty.id][act.id][quest.id].is_completed
                        || editable.quests[difficulty.id][act.id][quest.id].consumed_scroll
                      ) && (
                        <CompactToggle
                          nested
                          label="consumed_scroll"
                          checked={editable.quests[difficulty.id][act.id][quest.id].consumed_scroll}
                          onChange={(checked) => edit((current) => (
                            setQuestConsumedScrollSnapshot(current, difficulty.id, checked)
                          ))}
                        />
                      )}
                    </div>
                  ))}
                </div>
              </article>
            ))}
          </div>
        </section>
      ))}
    </div>
  );
}

function WaypointsEditor({ editable, edit }) {
  return (
    <div className="rune-data-editor waypoint-editor">
      <div className="rune-data-actions waypoint-bulk-actions">
        <button type="button" onClick={() => edit((current) => setAllWaypointsSnapshot(current, true))}>
          Unlock All
        </button>
        <button type="button" onClick={() => edit((current) => setAllWaypointsSnapshot(current, false))}>
          Reset All
        </button>
      </div>
      {difficultyDefinitions.map((difficulty) => (
        <section className="rune-difficulty-block" key={difficulty.id}>
          <h3>{difficulty.label}</h3>
          <div className="waypoint-act-grid">
            {waypointActs.map((act) => (
              <article className="rune-act-card" key={act.id}>
                <h4>{act.label}</h4>
                <div className="compact-toggle-list">
                  {act.waypoints.map((waypoint) => (
                    <CompactToggle
                      key={waypoint.id}
                      label={waypoint.label}
                      checked={editable.waypoints[difficulty.id][act.id][waypoint.id]}
                      onChange={(checked) => edit((current) => (
                        setWaypointSnapshot(current, difficulty.id, act.id, waypoint.id, checked)
                      ))}
                    />
                  ))}
                </div>
              </article>
            ))}
          </div>
        </section>
      ))}
    </div>
  );
}

function SkillsEditor({ editable, edit }) {
  const definition = useMemo(() => skillEditorDefinition(editable.className), [editable.className]);
  const displayTabs = [...definition.tabs].reverse();
  const [activePage, setActivePage] = useState(displayTabs[0]?.id ?? 1);
  const [ignoreRules, setIgnoreRules] = useState(false);
  const ranks = new Map(editable.skills.map((skill) => [skill.id, skill.points]));
  const pageSkills = definition.skills.filter((skill) => skill.page === activePage);
  const unusedPoints = editable.attributes.unused_skill_points;
  const activeTabIndex = Math.max(0, displayTabs.findIndex((tab) => tab.id === activePage));

  function prerequisitesMet(skill) {
    return skill.prerequisites.every((id) => Number(ranks.get(id) || 0) > 0);
  }

  function changeSkill(skill, delta) {
    edit((current) => {
      const existing = current.skills.find(({ id }) => id === skill.id)?.points ?? 0;
      const maximum = ignoreRules ? 255 : skill.maxLevel;
      const nextPoints = Math.max(0, Math.min(maximum, existing + delta));
      const appliedDelta = nextPoints - existing;
      if (appliedDelta === 0) return current;
      if (!ignoreRules && appliedDelta > 0) {
        if (current.attributes.level < skill.requiredLevel) return current;
        const currentRanks = new Map(current.skills.map((entry) => [entry.id, entry.points]));
        if (!skill.prerequisites.every((id) => Number(currentRanks.get(id) || 0) > 0)) return current;
      }
      return setSkillPointsSnapshot(current, skill.id, nextPoints);
    });
  }

  return (
    <div className="skills-editor">
      <div className="skills-toolbar">
        <CompactToggle label="Ignore Game Rules" checked={ignoreRules} onChange={setIgnoreRules} />
        <div className="skills-toolbar-summary">
          <strong>{unusedPoints} skill choice remaining</strong>
        </div>
      </div>
      <div className="skill-tree-stage">
        <img
          className="skill-tree-background"
          src={skillTreeVisuals[definition.classCode][activeTabIndex]}
          alt={`${displayTabs[activeTabIndex].label} background`}
          draggable="false"
        />
        <div className="skill-tree-tabs" role="tablist" aria-label={`${editable.className} skill trees`}>
          {displayTabs.map((tab) => (
            <button
              key={tab.id}
              type="button"
              role="tab"
              aria-selected={activePage === tab.id}
              className={activePage === tab.id ? 'active' : ''}
              onClick={() => setActivePage(tab.id)}
            >
              {tab.label}
            </button>
          ))}
        </div>
        <div className="skill-tree-grid">
          {pageSkills.map((skill) => {
          const points = Number(ranks.get(skill.id) || 0);
          const maximum = ignoreRules ? 255 : skill.maxLevel;
          const locked = !ignoreRules && !prerequisitesMet(skill);
          const canIncrease = points < maximum && (
            ignoreRules || (editable.attributes.level >= skill.requiredLevel && prerequisitesMet(skill))
          );
          const canDecrease = points > 0;
          return (
            <article
              className={`skill-node ${points > 0 ? 'invested' : ''} ${locked ? 'locked' : ''}`}
              key={skill.id}
              style={{ gridArea: `${skill.row} / ${skill.column}` }}
            >
              <button
                className="skill-node-control"
                type="button"
                aria-label={`${skill.name}, level ${points}`}
                disabled={locked}
                onClick={(event) => {
                  if (event.shiftKey ? canDecrease : canIncrease) {
                    changeSkill(skill, event.shiftKey ? -1 : 1);
                  }
                }}
                onContextMenu={(event) => {
                  event.preventDefault();
                  if (canDecrease) changeSkill(skill, -1);
                }}
              >
                <SkillVisual
                  classCode={definition.classCode}
                  iconCell={skill.iconCell}
                />
                <output aria-label={`${skill.name} level`}>{points > 0 ? points : ''}</output>
              </button>
            </article>
          );
          })}
        </div>
      </div>
      <p className="skills-footnote visually-hidden">
        Normal mode follows required levels and prerequisites. Ignore Game Rules writes raw levels from 0 to 255.
      </p>
    </div>
  );
}
function SkillVisual({ classCode, iconCell }) {
  const source = skillVisuals[classCode][iconCell / 2];
  return (
    <span className="skill-glyph" aria-hidden="true">
      <img
        src={source}
        alt=""
        loading="lazy"
        decoding="async"
        draggable="false"
      />
    </span>
  );
}

function QuestVisual({ code, completed, label }) {
  return (
    <span className={`quest-visual ${completed ? 'completed' : ''}`}>
      <img src={itemVisuals[code]} alt={label.toLowerCase().replaceAll("'", '')} draggable="false" />
    </span>
  );
}

function CompactToggle({ label, checked, onChange, nested = false, leading = null }) {
  return (
    <label className={`compact-toggle ${nested ? 'nested' : ''}`}>
      <input type="checkbox" checked={checked} onChange={(event) => onChange(event.target.checked)} />
      {leading}
      <span className="compact-toggle-track" aria-hidden="true"><span /></span>
      <strong>{label}</strong>
    </label>
  );
}

function InventoryEditor({
  items,
  placements,
  selectedItemIndex,
  onSelectItem,
  onMoveItem,
  onEditItem,
}) {
  return (
    <div className="item-workspace">
      <ItemSelectionBar
        items={items}
        selectedItemIndex={selectedItemIndex}
        emptyText="Select an item in any grid, then choose a free destination cell."
        action={selectedItemIndex !== null ? (
          <button className="button ghost compact" type="button" onClick={onEditItem}>
            Edit item
          </button>
        ) : null}
      />
      {Object.values(itemContainers).filter(({ id }) => id !== 'belt').map((container) => (
        <ContainerGrid
          key={container.id}
          container={container}
          items={items}
          placements={placements}
          selectedItemIndex={selectedItemIndex}
          onSelectItem={onSelectItem}
          onMoveItem={onMoveItem}
        />
      ))}
    </div>
  );
}

function EquipmentEditor({
  items,
  placements,
  selectedItemIndex,
  onSelectItem,
  onOpenInventory,
  onEditItem,
}) {
  const equipped = placements.filter((placement) => containerForPlacement(placement) === 'equipment');
  const belt = placements.filter((placement) => containerForPlacement(placement) === 'belt');
  const beltCapacity = beltCapacityForPlacements(placements, items);
  const knownSlots = new Set(equipmentSlots.map(([slot]) => slot));
  const otherEquipped = equipped.filter((placement) => !knownSlots.has(placement.equippedId));

  return (
    <div className="item-workspace">
      <ItemSelectionBar
        items={items}
        selectedItemIndex={selectedItemIndex}
        emptyText="Select equipped or belted gear to prepare a safe move into a stored container."
        action={selectedItemIndex !== null ? (
          <div className="selection-actions">
            <button className="button ghost compact" type="button" onClick={onEditItem}>Edit item</button>
            <button className="button ghost compact" type="button" onClick={onOpenInventory}>Choose destination</button>
          </div>
        ) : null}
      />
      <section className="panel">
        <PanelHeading
          title="Equipment slots"
          description="Body-location IDs are rendered exactly as stored. Equipping into a slot stays read-only until item-type compatibility is proven."
        />
        <div className="equipment-layout">
          {equipmentSlots.map(([slot, label]) => {
            const placement = equipped.find((candidate) => candidate.equippedId === slot);
            return (
              <div className={`equipment-slot slot-${slot}`} key={slot}>
                <span>{label}</span>
                {placement ? (
                  <ItemToken
                    item={items[placement.index]}
                    placement={placement}
                    selected={selectedItemIndex === placement.index}
                    onSelect={() => onSelectItem(placement.index)}
                  />
                ) : <small>Empty</small>}
              </div>
            );
          })}
        </div>
        {otherEquipped.length > 0 && (
          <div className="unmapped-items">
            <strong>Other equipped records</strong>
            {otherEquipped.map((placement) => (
              <ItemToken
                key={placement.index}
                item={items[placement.index]}
                placement={placement}
                selected={selectedItemIndex === placement.index}
                onSelect={() => onSelectItem(placement.index)}
              />
            ))}
          </div>
        )}
      </section>
      <section className="panel">
        <PanelHeading
          title="Belt"
          description={`D2S stores a flattened 0–15 index; the equipped BKVince belt currently exposes ${beltCapacity} native slots.`}
        />
        <DisplayGrid
          columns={4}
          rows={4}
          capacity={beltCapacity}
          items={items}
          placements={belt}
          selectedItemIndex={selectedItemIndex}
          onSelectItem={onSelectItem}
          emptyLabel={`Belt grid · ${beltCapacity} of 16 slots usable`}
        />
      </section>
    </div>
  );
}

function ContainerGrid({
  characterLevel = 1,
  container,
  items,
  placements,
  selectedItemIndex,
  onSelectItem,
  onMoveItem,
  scope = 'player',
  draggedItem = null,
  onStartItemDrag = null,
  onEndItemDrag = null,
  onDragActivity = null,
  onPointerDrop = null,
}) {
  const visible = placements.filter((placement) => containerForPlacement(placement) === container.id);
  const occupiedCells = new Set();
  visible.forEach((placement) => {
    const descriptor = describeItem(items[placement.index], placement.index, characterLevel);
    for (let offsetY = 0; offsetY < descriptor.height; offsetY += 1) {
      for (let offsetX = 0; offsetX < descriptor.width; offsetX += 1) {
        occupiedCells.add(`${placement.x + offsetX}-${placement.y + offsetY}`);
      }
    }
  });
  const cells = Array.from({ length: container.width * container.height }, (_, index) => ({
    x: index % container.width,
    y: Math.floor(index / container.width),
  }));
  return (
    <section className="panel container-panel">
      <PanelHeading
        title={container.label}
        description={`${container.width}×${container.height} cells from BKVince inventory.txt · ${visible.length} stored item${visible.length === 1 ? '' : 's'}${container.id === 'inventory' ? ' · right column: frozen BKVince charms' : ''}`}
      />
      <div className="item-grid-scroll">
        <div
          className={`item-grid item-grid-${container.id}`}
          style={{ '--grid-columns': container.width, '--grid-rows': container.height }}
          aria-label={`${container.label} grid`}
        >
          {cells.map(({ x, y }) => {
            const key = `${x}-${y}`;
            const style = { gridColumn: x + 1, gridRow: y + 1 };
            const frozenCharmCell = container.id === 'inventory' && x === container.width - 1;
            if (occupiedCells.has(key)) {
              return (
                <span
                  className={`grid-cell occupied-cell ${frozenCharmCell ? 'frozen-charm-cell' : ''}`}
                  key={key}
                  style={style}
                  aria-hidden="true"
                />
              );
            }
            const acceptsDraggedItem = Boolean(
              onPointerDrop
              && draggedItem
              && itemDropScopesCompatible(draggedItem.scope, scope, 'grid'),
            );
            return (
              <button
                className={`grid-cell ${frozenCharmCell ? 'frozen-charm-cell' : ''} ${acceptsDraggedItem ? 'drop-ready' : ''}`}
                type="button"
                key={key}
                style={style}
                data-item-drop-target={onPointerDrop ? 'true' : undefined}
                data-item-drop-scope={onPointerDrop ? scope : undefined}
                data-item-drop-kind={onPointerDrop ? 'grid' : undefined}
                data-item-drop-container={onPointerDrop ? container.id : undefined}
                data-item-drop-x={onPointerDrop ? x : undefined}
                data-item-drop-y={onPointerDrop ? y : undefined}
                data-tooltip-x={x === 0 ? 'start' : x === container.width - 1 ? 'end' : 'center'}
                data-tooltip-y={y === 0 ? 'start' : y === container.height - 1 ? 'end' : 'center'}
                aria-label={selectedItemIndex === null
                  ? `Add an item at ${container.label} column ${x + 1}, row ${y + 1}${frozenCharmCell ? ' in the frozen BKVince charm zone' : ''}`
                  : `Place selected item at ${container.label} column ${x + 1}, row ${y + 1}${frozenCharmCell ? ' in the frozen BKVince charm zone' : ''}`}
                data-action-label={selectedItemIndex === null ? 'Click to add' : 'Place here'}
                title={frozenCharmCell
                  ? 'Frozen BKVince charm zone'
                  : selectedItemIndex === null ? 'Click to add' : 'Place selected item here'}
                onClick={() => onMoveItem(container.id, x, y)}
                onDragEnter={(event) => {
                  if (!acceptsDraggedItem) return;
                  event.preventDefault();
                  event.currentTarget.classList.add('drag-over');
                }}
                onDragOver={(event) => {
                  if (!acceptsDraggedItem) return;
                  event.preventDefault();
                  event.dataTransfer.dropEffect = 'move';
                }}
                onDragLeave={(event) => event.currentTarget.classList.remove('drag-over')}
                onDrop={(event) => {
                  event.currentTarget.classList.remove('drag-over');
                  if (!acceptsDraggedItem) return;
                  event.preventDefault();
                  onPointerDrop(draggedItem, { ...event.currentTarget.dataset });
                }}
              />
            );
          })}
          {visible.map((placement) => (
            <GridItem
              characterLevel={characterLevel}
              key={placement.index}
              item={items[placement.index]}
              placement={placement}
              columns={container.width}
              rows={container.height}
              selected={selectedItemIndex === placement.index}
              onSelect={() => onSelectItem(placement.index)}
              draggable={Boolean(onPointerDrop && onStartItemDrag)}
              dragPayload={{ scope, index: placement.index }}
              onDragStart={onStartItemDrag
                ? (event) => onStartItemDrag(scope, placement.index, event)
                : null}
              onDragEnd={onEndItemDrag}
              onDragActivity={onDragActivity}
              onPointerDrop={onPointerDrop}
            />
          ))}
        </div>
      </div>
    </section>
  );
}

function DisplayGrid({
  characterLevel = 1,
  columns,
  rows,
  capacity = columns * rows,
  items,
  placements,
  selectedItemIndex,
  onSelectItem,
  onMoveItem = null,
  scope = 'player',
  draggedItem = null,
  onStartItemDrag = null,
  onEndItemDrag = null,
  onDragActivity = null,
  onPointerDrop = null,
  emptyLabel,
}) {
  const occupiedSlots = new Set(placements.map((placement) => placement.x));
  const cells = Array.from({ length: columns * rows }, (_, visualIndex) => {
    const x = visualIndex % columns;
    const y = Math.floor(visualIndex / columns);
    const nativeSlot = ((rows - 1 - y) * columns) + x;
    return { x, y, nativeSlot };
  });
  return (
    <div className="display-grid-wrap">
      <div
        className="item-grid display-only belt-grid"
        style={{ '--grid-columns': columns, '--grid-rows': rows }}
        aria-label={emptyLabel}
      >
        {cells.map(({ x, y, nativeSlot }) => {
          const style = { gridColumn: x + 1, gridRow: y + 1 };
          if (occupiedSlots.has(nativeSlot)) {
            return <span className="grid-cell occupied-cell" key={nativeSlot} style={style} aria-hidden="true" />;
          }
          if (!onMoveItem || nativeSlot >= capacity) {
            return (
              <span
                className={`grid-cell ${nativeSlot >= capacity ? 'locked-cell' : ''}`}
                key={nativeSlot}
                style={style}
                aria-hidden="true"
              />
            );
          }
          const acceptsDraggedItem = Boolean(
            onPointerDrop
            && draggedItem
            && itemDropScopesCompatible(draggedItem.scope, scope, 'grid'),
          );
          return (
            <button
              className={`grid-cell ${acceptsDraggedItem ? 'drop-ready' : ''}`}
              type="button"
              key={nativeSlot}
              style={style}
              data-item-drop-target={onPointerDrop ? 'true' : undefined}
              data-item-drop-scope={onPointerDrop ? scope : undefined}
              data-item-drop-kind={onPointerDrop ? 'grid' : undefined}
              data-item-drop-container={onPointerDrop ? 'belt' : undefined}
              data-item-drop-x={onPointerDrop ? nativeSlot : undefined}
              data-item-drop-y={onPointerDrop ? 0 : undefined}
              data-tooltip-x={x === 0 ? 'start' : x === columns - 1 ? 'end' : 'center'}
              data-tooltip-y={y === 0 ? 'start' : y === rows - 1 ? 'end' : 'center'}
              aria-label={selectedItemIndex === null
                ? `Add an item to Belt slot ${nativeSlot + 1}`
                : `Place selected item in Belt slot ${nativeSlot + 1}`}
              data-action-label={selectedItemIndex === null ? 'Click to add' : 'Place here'}
              title={selectedItemIndex === null ? 'Click to add a belt-compatible item' : 'Place selected item here'}
              onClick={() => onMoveItem('belt', nativeSlot, 0)}
              onDragEnter={(event) => {
                if (!acceptsDraggedItem) return;
                event.preventDefault();
                event.currentTarget.classList.add('drag-over');
              }}
              onDragOver={(event) => {
                if (!acceptsDraggedItem) return;
                event.preventDefault();
                event.dataTransfer.dropEffect = 'move';
              }}
              onDragLeave={(event) => event.currentTarget.classList.remove('drag-over')}
              onDrop={(event) => {
                event.currentTarget.classList.remove('drag-over');
                if (!acceptsDraggedItem) return;
                event.preventDefault();
                onPointerDrop(draggedItem, { ...event.currentTarget.dataset });
              }}
            />
          );
        })}
        {placements.map((placement) => {
          const coordinate = {
            x: placement.x % columns,
            y: rows - 1 - Math.floor(placement.x / columns),
          };
          return (
            <GridItem
              characterLevel={characterLevel}
              key={placement.index}
              item={items[placement.index]}
              placement={{ ...placement, ...coordinate }}
              columns={columns}
              rows={rows}
              selected={selectedItemIndex === placement.index}
              onSelect={() => onSelectItem(placement.index)}
              forceUnitSize
              draggable={Boolean(onPointerDrop && onStartItemDrag)}
              dragPayload={{ scope, index: placement.index }}
              onDragStart={onStartItemDrag
                ? (event) => onStartItemDrag(scope, placement.index, event)
                : null}
              onDragEnd={onEndItemDrag}
              onDragActivity={onDragActivity}
              onPointerDrop={onPointerDrop}
            />
          );
        })}
      </div>
    </div>
  );
}

function GridItem({
  characterLevel = 1,
  item,
  placement,
  columns,
  rows,
  selected,
  onSelect,
  forceUnitSize = false,
  draggable = false,
  dragPayload = null,
  onDragStart = null,
  onDragEnd = null,
  onDragActivity = null,
  onPointerDrop = null,
}) {
  const descriptor = describeItem(item, placement.index, characterLevel);
  const tooltipId = useId();
  const width = forceUnitSize ? 1 : descriptor.width;
  const height = forceUnitSize ? 1 : descriptor.height;
  const hasSprite = Boolean(itemVisuals[String(descriptor.type).toLowerCase()]);
  const pointerDrag = useItemPointerDrag({
    payload: dragPayload,
    onActivity: onDragActivity,
    onDrop: draggable ? onPointerDrop : null,
  });
  const tooltipVertical = placement.y + height > rows / 2 ? 'tooltip-above' : 'tooltip-below';
  let tooltipHorizontal = 'tooltip-align-center';
  if (placement.x + width <= columns / 3) tooltipHorizontal = 'tooltip-align-left';
  if (placement.x >= (columns * 2) / 3) tooltipHorizontal = 'tooltip-align-right';
  return (
    <button
      className={`grid-item ${hasSprite ? 'has-sprite' : ''} ${selected ? 'selected' : ''} ${draggable ? 'draggable' : ''} ${tooltipVertical} ${tooltipHorizontal}`}
      type="button"
      aria-label={`Edit ${descriptor.name}`}
      aria-describedby={tooltipId}
      style={{
        gridColumn: `${placement.x + 1} / span ${width}`,
        gridRow: `${placement.y + 1} / span ${height}`,
      }}
      draggable={draggable}
      onDragStart={onDragStart || undefined}
      onDragEnd={onDragEnd || undefined}
      onClickCapture={pointerDrag.onClickCapture}
      onPointerDown={pointerDrag.onPointerDown}
      onPointerMove={pointerDrag.onPointerMove}
      onPointerUp={pointerDrag.onPointerUp}
      onPointerCancel={pointerDrag.onPointerCancel}
      onClick={onSelect}
    >
      <ItemVisual descriptor={descriptor} />
      <ItemTooltip
        id={tooltipId}
        descriptor={descriptor}
        actionHints={draggable ? ['click to edit', 'drag to move'] : undefined}
      />
    </button>
  );
}

function ItemVisual({ descriptor, large = false }) {
  const code = String(descriptor.type || '????').toLowerCase();
  const source = itemVisuals[String(descriptor.visualKey || code).toLowerCase()] || itemVisuals[code] || null;
  return (
    <span className={`item-visual ${source ? 'sprite' : 'fallback'} ${large ? 'large' : ''}`} aria-hidden="true">
      {source
        ? <img src={source} alt="" draggable="false" />
        : <span>{code.toUpperCase()}</span>}
    </span>
  );
}

function ItemTooltip({ id, descriptor, actionHints = ['click to edit', 'choose a free cell to move'] }) {
  return (
    <span className="item-tooltip" id={id} role="tooltip">
      <strong className={`quality-${(descriptor.quality || 'simple').toLowerCase()}`}>{descriptor.name}</strong>
      {descriptor.name !== descriptor.baseName && <span>{descriptor.baseName}</span>}
      <ItemPropertyLines descriptor={descriptor} />
      {actionHints && <em>{actionHints.map((hint) => `[${hint}]`).join('/')}</em>}
    </span>
  );
}

function ItemPropertyLines({ descriptor }) {
  return (
    <>
      {descriptor.damageRanges.map(({ label, minimum, maximum }) => (
        <span key={label}>{label}: {minimum} to {maximum}</span>
      ))}
      {descriptor.defense !== null && <span>Defense: {descriptor.defense}</span>}
      {descriptor.durability && (
        <span>Durability: {descriptor.durability.current} of {descriptor.durability.maximum}</span>
      )}
      {descriptor.requiredStrength !== null && (
        <span className="tooltip-requirement">Required Strength: {descriptor.requiredStrength}</span>
      )}
      {descriptor.requiredDexterity !== null && (
        <span className="tooltip-requirement">Required Dexterity: {descriptor.requiredDexterity}</span>
      )}
      {descriptor.requiredLevel !== null && (
        <span className="tooltip-requirement">Required Level: {descriptor.requiredLevel}</span>
      )}
      {descriptor.itemLevel !== null && <span className="tooltip-item-level">Item Level: {descriptor.itemLevel}</span>}
      {descriptor.quantity !== null && <span>Quantity: {descriptor.quantity}</span>}
      {descriptor.ethereal && <span className="tooltip-ethereal">Ethereal</span>}
      {descriptor.personalized && <span>Personalized by {descriptor.personalizedName}</span>}
      {descriptor.magicAttributes.map((attribute, index) => (
        <span className="tooltip-magic" key={`${attribute}-${index}`}>{attribute}</span>
      ))}
      {descriptor.setBonusAttributes?.map((attributes, listIndex) => (
        <span className="tooltip-set-bonus" key={`set-bonus-${listIndex}`}>
          {attributes.map((attribute, attributeIndex) => (
            <span key={`${attribute}-${attributeIndex}`}>{attribute}</span>
          ))}
        </span>
      ))}
      {descriptor.sockets && (
        <span className="tooltip-magic">Socketed ({descriptor.sockets.filled}/{descriptor.sockets.total})</span>
      )}
      {descriptor.socketedItems.length > 0 && (
        <span className="socketed-item-chips" aria-label="Socketed items">
          {descriptor.socketedItems.map((socketedItem) => {
            const source = itemVisuals[String(socketedItem.type).toLowerCase()];
            return (
              <span className="socketed-item-chip" key={`${socketedItem.type}-${socketedItem.index}`} title={socketedItem.name}>
                {source
                  ? <img src={source} alt={socketedItem.name} draggable="false" />
                  : socketedItem.type.toUpperCase()}
              </span>
            );
          })}
        </span>
      )}
    </>
  );
}

function ItemToken({
  characterLevel = 1,
  item,
  placement,
  selected,
  onSelect,
  draggable = false,
  dragPayload = null,
  onDragStart = null,
  onDragEnd = null,
  onDragActivity = null,
  onPointerDrop = null,
}) {
  const descriptor = describeItem(item, placement.index, characterLevel);
  const tooltipId = useId();
  const hasSprite = Boolean(itemVisuals[String(descriptor.type).toLowerCase()]);
  const pointerDrag = useItemPointerDrag({
    payload: dragPayload,
    onActivity: onDragActivity,
    onDrop: draggable ? onPointerDrop : null,
  });
  return (
    <button
      className={`item-token ${hasSprite ? 'has-sprite' : ''} ${selected ? 'selected' : ''} ${draggable ? 'draggable' : ''}`}
      type="button"
      aria-label={`Edit ${descriptor.name}`}
      aria-describedby={tooltipId}
      draggable={draggable}
      onDragStart={onDragStart || undefined}
      onDragEnd={onDragEnd || undefined}
      onClickCapture={pointerDrag.onClickCapture}
      onPointerDown={pointerDrag.onPointerDown}
      onPointerMove={pointerDrag.onPointerMove}
      onPointerUp={pointerDrag.onPointerUp}
      onPointerCancel={pointerDrag.onPointerCancel}
      onClick={onSelect}
    >
      <ItemVisual descriptor={descriptor} />
      <ItemTooltip
        id={tooltipId}
        descriptor={descriptor}
        actionHints={draggable
          ? ['click to edit', 'drag to move']
          : null}
      />
    </button>
  );
}

function ItemSelectionBar({ characterLevel = 1, items, selectedItemIndex, emptyText, action = null }) {
  const item = selectedItemIndex === null ? null : describeItem(items[selectedItemIndex], selectedItemIndex, characterLevel);
  return (
    <div className={`selection-bar ${item ? 'active' : ''}`}>
      <div>
        <p className="eyebrow">Selected item</p>
        {item ? (
          <strong>{item.name} <span>{item.type.toUpperCase()} · {item.width}×{item.height}</span></strong>
        ) : <span>{emptyText}</span>}
      </div>
      {action}
    </div>
  );
}

function catalogBaseAbbreviation(name) {
  const words = String(name || '').trim().split(/\s+/).filter(Boolean);
  return words.length > 1 ? words.map((word) => word[0]?.toUpperCase()).join('') : words[0] || '';
}

function catalogRunewordLabel(name, baseName) {
  const words = String(name || '').trim().split(/\s+/).filter(Boolean);
  const runewordAbbreviation = words.length > 1
    ? ` (${words.map((word) => word[0]?.toUpperCase()).join('')})`
    : '';
  return `${name}${runewordAbbreviation} (${catalogBaseAbbreviation(baseName)})`;
}

function AddItemModal({
  bases,
  namedItems,
  runewordItems,
  groups,
  target,
  defaultItemLevel,
  onAdd,
  onAddGroup,
  onAddCatalogBatch,
  onImport,
  onClose,
}) {
  const [search, setSearch] = useState('');
  const [catalogFilter, setCatalogFilter] = useState('All');
  const [catalogOpen, setCatalogOpen] = useState(false);
  const [catalogHighlightIndex, setCatalogHighlightIndex] = useState(0);
  const [selectedKey, setSelectedKey] = useState('');
  const [runewordBaseCode, setRunewordBaseCode] = useState('');
  const rollMode = 'maximum';
  const [count, setCount] = useState(1);
  const [itemLevel, setItemLevel] = useState(Math.max(1, Math.min(99, defaultItemLevel)));
  const [quantity, setQuantity] = useState(1);
  const [queuedGroup, setQueuedGroup] = useState(null);
  const [catalogQueue, setCatalogQueue] = useState([]);
  const [importFiles, setImportFiles] = useState([]);
  const [importBusy, setImportBusy] = useState(false);
  const [error, setError] = useState('');
  const allowNamedBaseFallback = !Number.isInteger(target.equipmentSlot) && target.containerId !== 'belt';
  const catalogEntries = useMemo(() => {
    const baseByCode = new Map(bases.map((base) => [base.code, base]));
    const baseEntries = bases.map((base) => {
      const descriptor = describeItem({ type: base.code }, 0);
      return {
        key: `base:${base.code}`,
        kind: 'base',
        name: descriptor.name,
        base,
        baseCode: base.code,
        baseName: descriptor.baseName,
        source: base.source,
        categories: base.categories,
        path: `Bases › ${base.source} › ${descriptor.baseName}`,
      };
    });
    const governedEntries = namedItems.flatMap((entry) => {
      const base = baseByCode.get(entry.baseCode) || (allowNamedBaseFallback ? {
        code: entry.baseCode,
        name: entry.baseName,
        source: entry.source,
        width: entry.width,
        height: entry.height,
        categories: entry.categories,
        compactSave: false,
        stackable: false,
        maxStack: 0,
      } : null);
      if (!base) return [];
      const kindLabel = entry.kind === 'set' ? 'Sets' : 'Uniques';
      const family = entry.kind === 'set' && entry.setName ? entry.setName : entry.source;
      return [{
        ...entry,
        key: `${entry.kind}:${entry.id}`,
        base,
        path: `${kindLabel} › ${family} › ${entry.baseName} › ${entry.name}`,
      }];
    });
    const governedRunewords = runewordItems.flatMap((entry) => {
      const compatibleBases = entry.compatibleBases.flatMap((candidate) => {
        const base = baseByCode.get(candidate.code);
        return base ? [base] : [];
      });
      if (compatibleBases.length === 0) return [];
      const base = compatibleBases[0];
      return [{
        ...entry,
        key: `runeword:${entry.id}`,
        base,
        baseCode: base.code,
        baseName: base.name,
        source: base.source,
        categories: base.categories,
        compatibleBases,
        displayName: catalogRunewordLabel(entry.name, base.name),
        path: `Runewords › ${base.source === 'Weapons' ? 'Weapon' : base.source} › ${base.name} › ${entry.name}`,
      }];
    });
    return [...baseEntries, ...governedEntries, ...governedRunewords];
  }, [allowNamedBaseFallback, bases, namedItems, runewordItems]);
  const catalogCounts = useMemo(() => Object.fromEntries(
    ['base', 'set', 'unique', 'runeword'].map((kind) => [
      kind,
      catalogEntries.filter((entry) => entry.kind === kind).length,
    ]),
  ), [catalogEntries]);
  const query = search.trim().toLowerCase();
  const matches = catalogEntries
    .filter((entry) => {
      if (catalogFilter === 'Bases' && entry.kind !== 'base') return false;
      if (catalogFilter === 'Sets' && entry.kind !== 'set') return false;
      if (catalogFilter === 'Uniques' && entry.kind !== 'unique') return false;
      if (catalogFilter === 'Runewords' && entry.kind !== 'runeword') return false;
      if (['Armor', 'Weapons', 'Misc'].includes(catalogFilter) && entry.source !== catalogFilter) return false;
      if (!query) return true;
      return [
        entry.name,
        entry.displayName,
        entry.baseCode,
        entry.baseName,
        entry.source,
        entry.path,
        ...(entry.categories || []),
        ...(entry.runes || []),
        ...(entry.searchTerms || []),
      ].some((value) => String(value || '').toLowerCase().includes(query));
    })
    .sort((left, right) => {
      if (!query) return left.name.localeCompare(right.name, 'en-US');
      const rank = (entry) => {
        const name = String(entry.name || '').toLowerCase();
        if (name === query) return 0;
        if (name.startsWith(query)) return 1;
        if (String(entry.displayName || '').toLowerCase().startsWith(query)) return 2;
        if (name.includes(query)) return 3;
        return 4;
      };
      return rank(left) - rank(right)
        || left.name.localeCompare(right.name, 'en-US')
        || left.kind.localeCompare(right.kind, 'en-US');
    });
  const visible = (catalogOpen || query) ? matches.slice(0, 120) : [];
  const activeCatalogIndex = visible.length > 0
    ? Math.min(catalogHighlightIndex, visible.length - 1)
    : -1;
  const selectedEntry = catalogEntries.find(({ key }) => key === selectedKey) || null;
  const selected = selectedEntry?.kind === 'runeword'
    ? (selectedEntry.compatibleBases.find(({ code }) => code === runewordBaseCode) || selectedEntry.base)
    : (selectedEntry?.base || null);
  const selectedDescriptor = selectedEntry && selected
    ? describeItem({
      type: selected.code,
      level: itemLevel,
      quantity: selected.stackable ? quantity : undefined,
      simple_item: selectedEntry.kind === 'base' && selected.compactSave ? 1 : 0,
      quality: selectedEntry.kind === 'set' ? 5 : (selectedEntry.kind === 'unique' ? 7 : 2),
      set_id: selectedEntry.kind === 'set' ? selectedEntry.id : undefined,
      set_name: selectedEntry.kind === 'set' ? selectedEntry.name : undefined,
      unique_id: selectedEntry.kind === 'unique' ? selectedEntry.id : undefined,
      unique_name: selectedEntry.kind === 'unique' ? selectedEntry.name : undefined,
      given_runeword: selectedEntry.kind === 'runeword' ? 1 : 0,
      runeword_id: selectedEntry.kind === 'runeword' ? selectedEntry.id : undefined,
      runeword_name: selectedEntry.kind === 'runeword' ? selectedEntry.name : undefined,
    }, 0)
    : null;
  const equipmentSlot = Number.isInteger(target.equipmentSlot)
    ? equipmentSlotDefinitions.find(({ id }) => id === target.equipmentSlot)
    : null;
  const container = equipmentSlot ? null : itemContainers[target.containerId];
  const beltTarget = target.containerId === 'belt';
  const containerLabel = target.scope === 'shared-stash'
    ? `Shared Stash · Page ${(target.pageIndex ?? 0) + 1}`
    : (target.scope === 'virtual-stash' ? 'Virtual Stash · temporary workspace' : container?.label);
  const equipmentOwner = target.scope === 'mercenary' ? 'Mercenary' : 'Player';
  const destinationLabel = equipmentSlot
    ? `${equipmentOwner} · ${equipmentSlot.label}`
    : (beltTarget ? `${containerLabel} · slot ${target.x + 1}` : `${containerLabel} · cell ${target.x + 1},${target.y + 1}`);
  const queuedCount = queuedGroup?.entries.reduce((total, entry) => total + entry.count, 0) ?? 0;
  const catalogQueueTotal = catalogQueue.reduce((total, entry) => total + entry.count, 0);
  const submitCount = catalogQueueTotal || queuedCount || (equipmentSlot ? 1 : count);

  function chooseGroup(group) {
    setQueuedGroup({
      id: group.id,
      label: group.label,
      entries: group.entries.map((entry) => ({ ...entry, count: 1 })),
    });
    setSelectedKey('');
    setCatalogQueue([]);
    setRunewordBaseCode('');
    setSearch('');
    setCatalogOpen(false);
    setError('');
  }

  function chooseCatalogEntry(entry) {
    if (!entry) return;
    setSelectedKey(entry.key);
    setRunewordBaseCode(entry.kind === 'runeword' ? entry.base.code : '');
    setQueuedGroup(null);
    setQuantity(1);
    setError('');
  }

  function addSelectedToCatalogQueue() {
    if (!selectedEntry || !selected || equipmentSlot || beltTarget) return;
    if (catalogQueueTotal + count > 20) {
      setError(`A custom batch accepts at most 20 items. ${20 - catalogQueueTotal} slot${20 - catalogQueueTotal === 1 ? '' : 's'} remain.`);
      return;
    }
    const queueKey = [
      selectedEntry.key,
      selected.code,
      itemLevel,
      selected.stackable ? quantity : '-',
      ['set', 'unique', 'runeword'].includes(selectedEntry.kind) ? rollMode : '-',
    ].join(':');
    const request = {
      type: selected.code,
      itemLevel,
      quantity: selected.stackable ? quantity : null,
      quality: selectedEntry.kind === 'set' ? 5 : (selectedEntry.kind === 'unique' ? 7 : (selectedEntry.kind === 'runeword' ? 2 : null)),
      setId: selectedEntry.kind === 'set' ? selectedEntry.id : null,
      uniqueId: selectedEntry.kind === 'unique' ? selectedEntry.id : null,
      runewordId: selectedEntry.kind === 'runeword' ? selectedEntry.id : null,
      rollMode,
    };
    setCatalogQueue((current) => {
      const existing = current.find((entry) => entry.queueKey === queueKey);
      if (existing) {
        return current.map((entry) => entry.queueKey === queueKey
          ? { ...entry, count: entry.count + count }
          : entry);
      }
      return [...current, {
        queueKey,
        count,
        request,
        descriptor: selectedDescriptor,
        path: selectedEntry.path,
        kind: selectedEntry.kind,
      }];
    });
    setQueuedGroup(null);
    setCount(1);
    setError('');
  }

  function changeCatalogQueueCount(queueKey, delta) {
    setCatalogQueue((current) => {
      const currentTotal = current.reduce((total, entry) => total + entry.count, 0);
      return current.map((entry) => {
        if (entry.queueKey !== queueKey) return entry;
        const maximum = Math.max(1, 20 - (currentTotal - entry.count));
        return { ...entry, count: clampInteger(entry.count + delta, 1, maximum) };
      });
    });
    setError('');
  }

  function removeCatalogQueueEntry(queueKey) {
    setCatalogQueue((current) => current.filter((entry) => entry.queueKey !== queueKey));
    setError('');
  }

  function handleCatalogKeyDown(event) {
    if (visible.length === 0) return;
    if (event.key === 'ArrowDown') {
      event.preventDefault();
      setCatalogHighlightIndex((current) => (Math.min(current, visible.length - 1) + 1) % visible.length);
    } else if (event.key === 'ArrowUp') {
      event.preventDefault();
      setCatalogHighlightIndex((current) => (Math.min(current, visible.length - 1) - 1 + visible.length) % visible.length);
    } else if (event.key === 'Enter') {
      event.preventDefault();
      chooseCatalogEntry(visible[activeCatalogIndex]);
    }
  }

  function changeQueuedCount(id, delta) {
    setQueuedGroup((current) => {
      if (!current) return current;
      const currentTotal = current.entries.reduce((total, entry) => total + entry.count, 0);
      return {
        ...current,
        entries: current.entries.map((entry) => {
          if (entry.id !== id) return entry;
          const maximum = Math.max(1, 20 - (currentTotal - entry.count));
          return { ...entry, count: clampInteger(entry.count + delta, 1, maximum) };
        }),
      };
    });
  }

  function removeQueuedEntry(id) {
    setQueuedGroup((current) => {
      if (!current) return current;
      const entries = current.entries.filter((entry) => entry.id !== id);
      return entries.length > 0 ? { ...current, entries } : null;
    });
  }

  function stageImportFiles(fileList) {
    const files = Array.from(fileList || []);
    setError('');
    if (files.length > 20) {
      setImportFiles([]);
      setError('Choose at most 20 portable item files for one atomic import.');
      return;
    }
    setImportFiles(files);
  }

  function removeImportFile(fileIndex) {
    setImportFiles((current) => current.filter((_, index) => index !== fileIndex));
    setError('');
  }

  async function submitImport() {
    if (importFiles.length === 0 || importBusy) return;
    setError('');
    setImportBusy(true);
    try {
      await onImport(importFiles);
    } catch (caught) {
      setError(caught.message);
    } finally {
      setImportBusy(false);
    }
  }

  function submit(event) {
    event.preventDefault();
    setError('');
    if (catalogQueue.length > 0) {
      try {
        onAddCatalogBatch({
          selections: catalogQueue.map(({ count: entryCount, request }) => ({
            ...request,
            count: entryCount,
          })),
        });
      } catch (caught) {
        setError(caught.message);
      }
      return;
    }
    if (queuedGroup) {
      try {
        onAddGroup({
          groupId: queuedGroup.id,
          label: queuedGroup.label,
          selections: queuedGroup.entries.map(({ id, count: entryCount }) => ({ id, count: entryCount })),
          itemLevel,
        });
      } catch (caught) {
        setError(caught.message);
      }
      return;
    }
    if (!selectedEntry || !selected) {
      setError('Choose an item before adding it.');
      return;
    }
    try {
      onAdd({
        type: selected.code,
        count: equipmentSlot ? 1 : count,
        itemLevel,
        quantity: selected.stackable ? quantity : null,
        quality: selectedEntry.kind === 'set' ? 5 : (selectedEntry.kind === 'unique' ? 7 : (selectedEntry.kind === 'runeword' ? 2 : null)),
        setId: selectedEntry.kind === 'set' ? selectedEntry.id : null,
        uniqueId: selectedEntry.kind === 'unique' ? selectedEntry.id : null,
        runewordId: selectedEntry.kind === 'runeword' ? selectedEntry.id : null,
        rollMode,
      });
    } catch (caught) {
      setError(caught.message);
    }
  }

  return (
    <AccessibleModal className="add-item-modal" labelledBy="add-item-title" onClose={onClose}>
        <div className="modal-heading">
          <div>
            <p className="eyebrow">{destinationLabel}</p>
            <h2 id="add-item-title">Choose items to add</h2>
          </div>
          <button className="icon-button" type="button" aria-label="Close" onClick={onClose}>×</button>
        </div>
        <p className="modal-copy">
          {equipmentSlot
            ? `Only BKVince bases, Sets, Uniques, and Runewords whose governed BodyLoc accepts ${equipmentSlot.label} are shown. One native ${target.scope === 'mercenary' ? 'jf' : 'player'} record will be equipped directly in that slot.`
            : (beltTarget
              ? 'Only 1×1 items marked by the governed BKVince Misc.txt belt column are shown. Native belt capacity is enforced before any record is written.'
              : 'Search every governed BKVince base, Set, Unique, and Runeword. Named items are built with their native properties, while each Runeword is paired with a compatible socket base and its exact rune sequence.')}
        </p>
        <form onSubmit={submit}>
          <label className="item-search">
            <span className="visually-hidden">Search for an item</span>
            <SearchIcon />
            <input
              data-dialog-initial-focus
              type="search"
              placeholder="Search bases, Sets, Uniques, or Runewords"
              role="combobox"
              aria-controls="bkvince-item-catalog"
              aria-expanded={catalogOpen || Boolean(query)}
              aria-activedescendant={activeCatalogIndex >= 0 ? `bkvince-item-option-${visible[activeCatalogIndex].key.replace(/[^a-z0-9_-]/gi, '-')}` : undefined}
              value={search}
              onKeyDown={handleCatalogKeyDown}
              onChange={(event) => {
                setSearch(event.target.value);
                setCatalogOpen(Boolean(event.target.value.trim()));
                setCatalogHighlightIndex(0);
                setQueuedGroup(null);
              }}
            />
          </label>
          {!equipmentSlot && !beltTarget && (
            <div className="item-quick-groups" aria-label="BKVince quick item groups">
              {groups.map((group) => (
                <button
                  className={queuedGroup?.id === group.id ? 'active' : ''}
                  type="button"
                  key={group.id}
                  onClick={() => chooseGroup(group)}
                >
                  + {group.label}
                </button>
              ))}
            </div>
          )}
          {!queuedGroup && (
            <div className="item-source-tabs" aria-label="Item catalog filters">
              {['All', 'Bases', 'Sets', 'Uniques', 'Runewords', 'Armor', 'Weapons', 'Misc'].map((entry) => (
                <button
                  className={catalogFilter === entry ? 'active' : ''}
                  type="button"
                  key={entry}
                  onClick={() => {
                    setCatalogFilter(entry);
                    setCatalogOpen(true);
                    setCatalogHighlightIndex(0);
                    setQueuedGroup(null);
                  }}
                >
                  + {entry === 'All' ? 'All items' : entry}
                </button>
              ))}
            </div>
          )}
          {catalogQueue.length > 0 && (
            <div className="item-group-queue catalog-custom-queue" aria-label="Custom catalog batch">
              <div className="catalog-custom-queue-heading">
                <div>
                  <strong>Custom batch</strong>
                  <span>Mix governed Bases, Sets, Uniques, and Runewords before one atomic placement.</span>
                </div>
                <span>{catalogQueueTotal}/20 items</span>
              </div>
              {catalogQueue.map((entry) => (
                <div className="item-group-row" key={entry.queueKey}>
                  <span className="picker-item-visual group"><ItemVisual descriptor={entry.descriptor} /></span>
                  <div className="item-group-copy">
                    <strong>{entry.descriptor.name}</strong>
                    <span>{entry.path} · level {entry.request.itemLevel}</span>
                  </div>
                  <div className="item-group-quantity" aria-label={`${entry.descriptor.name} quantity`}>
                    <button type="button" disabled={entry.count <= 1} onClick={() => changeCatalogQueueCount(entry.queueKey, -1)}>−</button>
                    <span>{entry.count}</span>
                    <button type="button" disabled={catalogQueueTotal >= 20} onClick={() => changeCatalogQueueCount(entry.queueKey, 1)}>+</button>
                    <button
                      className="item-group-remove"
                      type="button"
                      aria-label={`Remove ${entry.descriptor.name}`}
                      onClick={() => removeCatalogQueueEntry(entry.queueKey)}
                    >
                      <TrashIcon />
                    </button>
                  </div>
                </div>
              ))}
            </div>
          )}
          {queuedGroup ? (
            <div className="item-group-queue" aria-label={`${queuedGroup.label} selection`}>
              {queuedGroup.entries.map((entry) => {
                const descriptor = describeItem({
                  type: entry.type,
                  quality: entry.quality,
                  set_id: entry.setId,
                  set_name: entry.quality === 5 ? entry.name : undefined,
                  simple_item: entry.quality ? 0 : 1,
                }, 0);
                return (
                  <div className="item-group-row" key={entry.id}>
                    <span className="picker-item-visual group"><ItemVisual descriptor={descriptor} /></span>
                    <div className="item-group-copy">
                      <strong>{entry.name}</strong>
                      <span>{entry.quality === 5 ? `Sets › Warlord's Glory › ${entry.baseName}` : `${queuedGroup.label} › ${entry.baseName}`}</span>
                    </div>
                    <div className="item-group-quantity" aria-label={`${entry.name} quantity`}>
                      <button type="button" disabled={entry.count <= 1} onClick={() => changeQueuedCount(entry.id, -1)}>−</button>
                      <span>{entry.count}</span>
                      <button type="button" disabled={queuedCount >= 20} onClick={() => changeQueuedCount(entry.id, 1)}>+</button>
                      <button
                        className="item-group-remove"
                        type="button"
                        aria-label={`Remove ${entry.name}`}
                        onClick={() => removeQueuedEntry(entry.id)}
                      >
                        <TrashIcon />
                      </button>
                    </div>
                  </div>
                );
              })}
            </div>
          ) : (catalogOpen || query) ? (
            <>
              <div className="item-picker-results" id="bkvince-item-catalog" role="listbox" aria-label="BKVince item catalog">
                {visible.map((entry, index) => {
                  const descriptor = describeItem({
                    type: entry.base.code,
                    simple_item: entry.kind === 'base' && entry.base.compactSave ? 1 : 0,
                    quality: entry.kind === 'set' ? 5 : (entry.kind === 'unique' ? 7 : 2),
                    set_name: entry.kind === 'set' ? entry.name : undefined,
                    unique_name: entry.kind === 'unique' ? entry.name : undefined,
                    given_runeword: entry.kind === 'runeword' ? 1 : 0,
                    runeword_name: entry.kind === 'runeword' ? entry.name : undefined,
                  }, 0);
                  const selectedOption = selectedKey === entry.key;
                  const highlightedOption = index === activeCatalogIndex;
                  const optionId = `bkvince-item-option-${entry.key.replace(/[^a-z0-9_-]/gi, '-')}`;
                  const matchingProperties = query
                    ? (entry.searchTerms || [])
                      .filter((term) => String(term).toLowerCase().includes(query))
                      .slice(0, 2)
                    : [];
                  return (
                    <button
                      className={`catalog-kind-${entry.kind} ${selectedOption ? 'selected' : ''} ${highlightedOption ? 'highlighted' : ''}`.trim()}
                      type="button"
                      role="option"
                      id={optionId}
                      aria-selected={selectedOption}
                      key={entry.key}
                      onMouseEnter={() => setCatalogHighlightIndex(index)}
                      onFocus={() => setCatalogHighlightIndex(index)}
                      onClick={() => chooseCatalogEntry(entry)}
                    >
                      <span className="picker-item-visual"><ItemVisual descriptor={descriptor} /></span>
                      <span className="picker-item-copy">
                        <strong>{entry.displayName || descriptor.name}</strong>
                        <small>{entry.path}</small>
                        {matchingProperties.length > 0 && (
                          <span className="picker-match-reason">
                            Matches: {matchingProperties.join(' · ')}
                          </span>
                        )}
                      </span>
                      <span className="picker-code">{entry.kind === 'base' ? entry.base.code.toUpperCase() : entry.kind}</span>
                    </button>
                  );
                })}
                {visible.length === 0 && <p className="picker-empty">No BKVince base, Set, Unique, or Runeword matches this search.</p>}
              </div>
              <p className="picker-result-count">
                Showing {visible.length} of {matches.length} matches · {catalogCounts.base} bases · {catalogCounts.set} Sets · {catalogCounts.unique} Uniques · {catalogCounts.runeword} Runewords valid for this destination.
              </p>
            </>
          ) : (
            <div className="item-picker-idle" aria-label="Item catalog ready">
              Search by name, code, rune, or item property, or open a governed BKVince catalog category.
            </div>
          )}
          {selectedDescriptor && (
            <div className={`item-picker-selection catalog-kind-${selectedEntry.kind}`} aria-live="polite">
              <span className="picker-selection-visual"><ItemVisual descriptor={selectedDescriptor} /></span>
              <div>
                <span className="eyebrow">Selected item</span>
                <strong>{selectedDescriptor.name}</strong>
                <small>{selectedEntry.path} · {selected.code.toUpperCase()} · {selected.width}×{selected.height}</small>
              </div>
              <span className="picker-selection-badge">{selectedEntry.kind === 'base' ? 'Base' : selectedEntry.kind}</span>
            </div>
          )}
          {selected && (
            <div className="picker-count-row">
              {!equipmentSlot && <NumberField label="Copies" value={count} min={1} max={20} onChange={setCount} />}
              <NumberField label="Item level" value={itemLevel} min={1} max={99} onChange={setItemLevel} />
              {selectedEntry?.kind === 'runeword' && (
                <label className="field picker-base-field">
                  <span>Compatible base</span>
                  <select
                    value={selected.code}
                    onChange={(event) => {
                      setRunewordBaseCode(event.target.value);
                      setError('');
                    }}
                  >
                    {selectedEntry.compatibleBases.map((base) => (
                      <option value={base.code} key={base.code}>{base.name} ({base.code.toUpperCase()})</option>
                    ))}
                  </select>
                  <small>{selectedEntry.runes.map((rune) => rune.toUpperCase()).join(' + ')}</small>
                </label>
              )}
              {selectedEntry && ['set', 'unique', 'runeword'].includes(selectedEntry.kind) && (
                <div className="field picker-roll-field">
                  <span>Property rolls</span>
                  <output className="picker-perfect-roll">
                    <strong>Perfect</strong>
                    <small>Maximum native save-compatible values</small>
                  </output>
                </div>
              )}
              {selected?.stackable && (
                <NumberField
                  label="Stack quantity"
                  value={quantity}
                  min={1}
                  max={Math.min(selected.maxStack > 0 ? selected.maxStack : 511, 511)}
                  onChange={setQuantity}
                />
              )}
            </div>
          )}
          {selected && !equipmentSlot && !beltTarget && (
            <div className="catalog-batch-actions">
              <button
                className="button ghost compact"
                type="button"
                disabled={catalogQueueTotal >= 20}
                onClick={addSelectedToCatalogQueue}
              >
                + Add {count} {count === 1 ? 'copy' : 'copies'} to batch
              </button>
              <span>{catalogQueueTotal > 0
                ? `${catalogQueueTotal} item${catalogQueueTotal === 1 ? '' : 's'} currently staged; the footer adds only this batch.`
                : 'Stage this selection to mix it with other catalog items.'}</span>
            </div>
          )}
          <div className="modal-actions">
            <button className="button primary" type="submit" disabled={!selected && !queuedGroup && catalogQueue.length === 0}>
              Add {submitCount} item{submitCount !== 1 ? 's' : ''}
            </button>
          </div>
          <div className="item-import-panel">
            <div>
              <strong>Import portable items</strong>
              <span>{equipmentSlot
                ? `Choose exactly one canonical v105 .d2i record. Its body location is validated before the ${target.scope === 'mercenary' ? 'jf' : 'player item'} block changes.`
                : 'Stage one or more canonical v105 .d2i records, or a fingerprinted BKVince .bkitems.json bundle.'}</span>
            </div>
            <label className={`button ghost compact ${importBusy ? 'disabled' : ''}`}>
              <UploadIcon /> Choose files
              <input
                className="visually-hidden"
                type="file"
                multiple={!equipmentSlot}
                accept=".d2i,.json,application/octet-stream,application/json"
                disabled={importBusy}
                onChange={(event) => {
                  if (event.target.files?.length) stageImportFiles(event.target.files);
                  event.target.value = '';
                }}
              />
            </label>
          </div>
          {importFiles.length > 0 && (
            <div className="portable-import-queue" aria-label="Portable item files ready to import">
              <div className="portable-import-heading">
                <strong>{importFiles.length} file{importFiles.length === 1 ? '' : 's'} ready</strong>
                <span>Nothing changes until the complete selection passes validation.</span>
              </div>
              <div className="portable-import-files">
                {importFiles.map((file, fileIndex) => {
                  const isBundle = file.name.toLocaleLowerCase('en-US').endsWith('.bkitems.json');
                  return (
                    <div className="portable-import-file" key={`${file.name}-${file.lastModified}-${fileIndex}`}>
                      <UploadIcon />
                      <span>
                        <strong>{file.name}</strong>
                        <small>{isBundle ? 'Fingerprint-checked BKVince bundle' : 'Canonical raw v105 item'} · {formatFileSize(file.size)}</small>
                      </span>
                      <button
                        className="attribute-delete"
                        type="button"
                        aria-label={`Remove ${file.name} from import`}
                        disabled={importBusy}
                        onClick={() => removeImportFile(fileIndex)}
                      >
                        <TrashIcon />
                      </button>
                    </div>
                  );
                })}
              </div>
              {importFiles.some((file) => !file.name.toLocaleLowerCase('en-US').endsWith('.bkitems.json')) && (
                <p className="portable-import-warning">
                  Raw .d2i records have no item-table fingerprint. Prefer a .bkitems.json bundle when moving complex BKVince items between editor versions.
                </p>
              )}
              <button
                className="button primary portable-import-submit"
                type="button"
                disabled={importBusy}
                onClick={submitImport}
              >
                <UploadIcon /> {importBusy ? 'Validating selection…' : `Import ${importFiles.length} selected file${importFiles.length === 1 ? '' : 's'}`}
              </button>
            </div>
          )}
          {error && <div className="picker-error" role="alert">{error}</div>}
        </form>
    </AccessibleModal>
  );
}

function ItemEditorModal({
  characterLevel = 1,
  item,
  edit,
  onChange,
  onInsertSocket,
  onImportSocket,
  onExtractSocket,
  onDeleteSocket,
  onApplyRuneword,
  onClearRuneword,
  onAddManualProperty,
  onDuplicate,
  onDownload,
  transferTargets,
  onTransfer,
  onDelete,
  deleteLabel,
  busy,
  onClose,
}) {
  const options = itemEditorOptions(item, edit);
  const attributesEditable = options.attributesEditable;
  const descriptor = describeItem({
    ...item,
    type: edit.type,
    level: edit.itemLevel,
    defense_rating: edit.defense,
    max_durability: edit.maximumDurability,
    current_durability: edit.currentDurability,
    quality: edit.quality ?? item.quality,
    multiple_pictures: edit.pictureId == null ? 0 : 1,
    picture_id: edit.pictureId == null ? undefined : edit.pictureId,
    identified: Number(edit.identified),
    ethereal: Number(edit.ethereal),
    personalized: Number(edit.personalized),
    personalized_name: edit.personalizedName,
    magic_prefix: edit.magicPrefix,
    magic_suffix: edit.magicSuffix,
    low_quality_id: edit.lowQualityId,
    rare_name_id: edit.rareNamePrefixId,
    rare_name: options.rareNamePrefixes.find(({ id }) => id === edit.rareNamePrefixId)?.name,
    rare_name_id2: edit.rareNameSuffixId,
    rare_name2: options.rareNameSuffixes.find(({ id }) => id === edit.rareNameSuffixId)?.name,
    magical_name_ids: edit.rareAffixIds,
    set_id: edit.setId,
    set_name: options.setItems.find(({ id }) => id === edit.setId)?.name,
    unique_id: edit.uniqueId,
    unique_name: options.uniqueItems.find(({ id }) => id === edit.uniqueId)?.name,
    magic_attributes: edit.magicAttributes,
    set_attributes: edit.setAttributes,
    set_list_count: edit.setAttributes.length,
    _unknown_data: { ...item._unknown_data, plist_flag: edit.setBonusMask },
    given_runeword: edit.runewordId === null ? 0 : 1,
    runeword_id: edit.runewordId,
    runeword_name: options.runewords.find(({ id }) => id === edit.runewordId)?.name,
    runeword_attributes: edit.runewordAttributes,
    socketed: Number(edit.socketed),
    total_nr_of_sockets: edit.totalSockets,
    socketed_items: edit.socketedItems,
  }, edit.index, characterLevel);
  const [selectedAttributeId, setSelectedAttributeId] = useState(
    String(options.magicAttributes[0]?.id ?? ''),
  );
  const [selectedSocketType, setSelectedSocketType] = useState(
    options.socketFillers[0]?.code ?? '',
  );
  const [requestedRunewordId, setRequestedRunewordId] = useState(String(
    edit.runewordId
      ?? options.runewords.find(({ compiler }) => compiler.supported)?.id
      ?? options.runewords[0]?.id
      ?? '',
  ));
  const selectedRunewordId = options.runewords.some(({ id }) => id === Number(requestedRunewordId))
    ? Number(requestedRunewordId)
    : (edit.runewordId
      ?? options.runewords.find(({ compiler }) => compiler.supported)?.id
      ?? options.runewords[0]?.id
      ?? null);
  const selectedRuneword = options.runewords.find(({ id }) => id === selectedRunewordId);
  const [propertySearch, setPropertySearch] = useState('');
  const [selectedPropertyCode, setSelectedPropertyCode] = useState(
    options.manualProperties[0]?.code ?? '',
  );
  const [propertyInputs, setPropertyInputs] = useState({
    parameter: '',
    minimum: '',
    maximum: '',
  });
  const [duplicateCount, setDuplicateCount] = useState(1);
  const [editingAttributeIndex, setEditingAttributeIndex] = useState(null);
  const [propertyBuilderOpen, setPropertyBuilderOpen] = useState(false);
  const [propertyHighlightIndex, setPropertyHighlightIndex] = useState(0);
  const propertySearchRef = useRef(null);
  const propertyToggleRef = useRef(null);
  const filteredManualProperties = options.manualProperties.filter((property) => {
    const query = propertySearch.trim().toLocaleLowerCase('en-US');
    return !query || `${property.label} ${property.code} ${property.notes || ''}`
      .toLocaleLowerCase('en-US')
      .includes(query);
  });
  const visibleManualProperties = filteredManualProperties.slice(0, 80);
  const effectivePropertyCode = filteredManualProperties.some(({ code }) => code === selectedPropertyCode)
    ? selectedPropertyCode
    : (filteredManualProperties[0]?.code ?? options.manualProperties[0]?.code ?? '');
  const selectedManualProperty = options.manualProperties.find(({ code }) => code === effectivePropertyCode);
  const manualPropertyComplete = Boolean(selectedManualProperty)
    && selectedManualProperty.fields.every((field) => (
      !field.required || String(propertyInputs[field.targets[0]] ?? '').trim() !== ''
    ));
  const manualPropertyEvaluation = useMemo(() => {
    if (!selectedManualProperty || !manualPropertyComplete) {
      return { valid: false, descriptions: [], error: null };
    }
    try {
      const preview = previewManualPropertyPatch(item, edit, {
        propertyCode: effectivePropertyCode,
        parameter: propertyInputs.parameter,
        minimum: propertyInputs.minimum,
        maximum: propertyInputs.maximum,
        rollMode: 'maximum',
      }, characterLevel);
      return { valid: true, descriptions: preview.descriptions, error: null };
    } catch (error) {
      return { valid: false, descriptions: [], error: error.message };
    }
  }, [
    characterLevel,
    edit,
    effectivePropertyCode,
    item,
    manualPropertyComplete,
    propertyInputs,
    selectedManualProperty,
  ]);
  const attributeDisplayGroups = [];
  const groupedAttributeIndices = new Set();
  [...options.manualProperties]
    .filter(({ attributeCount }) => attributeCount > 1)
    .sort((left, right) => right.attributeCount - left.attributeCount)
    .forEach((property) => {
      const attributeIndices = property.attributeIds.map((attributeId) => (
        edit.magicAttributes.findIndex((attribute, attributeIndex) => (
          attribute.id === attributeId && !groupedAttributeIndices.has(attributeIndex)
        ))
      ));
      if (attributeIndices.some((attributeIndex) => attributeIndex < 0)) return;
      const attributes = attributeIndices.map((attributeIndex) => edit.magicAttributes[attributeIndex]);
      const summaries = describeItem({
        type: edit.type,
        level: edit.itemLevel,
        magic_attributes: attributes,
      }, attributeIndices[0], characterLevel).magicAttributes;
      if (summaries.length !== 1) return;
      const definitions = attributes.map((attribute) => (
        options.magicAttributes.find(({ id }) => id === attribute.id)
      ));
      if (definitions.some((definition) => !definition)) return;
      const valueCount = attributes[0]?.values.length ?? 0;
      if (!attributes.every((attribute) => attribute.values.length === valueCount)
        || !definitions.every((definition) => definition.values.length === valueCount)) return;
      attributeIndices.forEach((attributeIndex) => groupedAttributeIndices.add(attributeIndex));
      attributeDisplayGroups.push({
        summary: summaries[0],
        attributes,
        attributeIndices,
        definitions,
        labels: [property.label],
      });
    });
  edit.magicAttributes.forEach((attribute, attributeIndex) => {
    if (groupedAttributeIndices.has(attributeIndex)) return;
    const definition = options.magicAttributes.find(({ id }) => id === attribute.id);
    const label = definition?.label || attribute.name || `Stat ${attribute.id}`;
    const summary = describeItem({
      type: edit.type,
      level: edit.itemLevel,
      magic_attributes: [attribute],
    }, attributeIndex, characterLevel).magicAttributes[0] || label;
    const compatibleGroup = definition
      ? attributeDisplayGroups.find((group) => (
        group.summary === summary
        && group.definitions.every(Boolean)
        && group.attributes[0].values.length === attribute.values.length
      ))
      : null;
    if (compatibleGroup) {
      compatibleGroup.attributes.push(attribute);
      compatibleGroup.attributeIndices.push(attributeIndex);
      compatibleGroup.definitions.push(definition);
      compatibleGroup.labels.push(label);
      return;
    }
    attributeDisplayGroups.push({
      summary,
      attributes: [attribute],
      attributeIndices: [attributeIndex],
      definitions: [definition],
      labels: [label],
    });
  });
  attributeDisplayGroups.sort((left, right) => left.attributeIndices[0] - right.attributeIndices[0]);
  const editorAttributeSummaries = attributeDisplayGroups.map(({ summary }) => summary);
  const remainingPreviewAttributes = [...descriptor.magicAttributes];
  editorAttributeSummaries.forEach((summary) => {
    const summaryIndex = remainingPreviewAttributes.indexOf(summary);
    if (summaryIndex >= 0) remainingPreviewAttributes.splice(summaryIndex, 1);
  });
  const previewDescriptor = {
    ...descriptor,
    magicAttributes: [...editorAttributeSummaries, ...remainingPreviewAttributes],
  };

  useEffect(() => {
    if (!propertyBuilderOpen) return;
    setPropertyHighlightIndex(0);
    propertySearchRef.current?.focus();
    propertySearchRef.current?.select();
  }, [propertyBuilderOpen]);

  useEffect(() => {
    if (!selectedManualProperty) return;
    const inputs = { parameter: '', minimum: '', maximum: '' };
    selectedManualProperty.fields.forEach((field) => {
      field.targets.forEach((target) => {
        inputs[target] = field.defaultValue;
      });
    });
    setPropertyInputs(inputs);
  }, [selectedManualProperty?.code]);

  function closePropertyBuilder({ restoreFocus = true } = {}) {
    setPropertyBuilderOpen(false);
    if (restoreFocus) requestAnimationFrame(() => propertyToggleRef.current?.focus());
  }

  function selectManualProperty(property) {
    setSelectedPropertyCode(property.code);
  }

  function handlePropertySearchKeyDown(event) {
    if (event.key === 'Escape') {
      event.preventDefault();
      event.stopPropagation();
      closePropertyBuilder();
      return;
    }
    if (event.key === 'ArrowDown' || event.key === 'ArrowUp') {
      event.preventDefault();
      const direction = event.key === 'ArrowDown' ? 1 : -1;
      setPropertyHighlightIndex((current) => {
        if (visibleManualProperties.length === 0) return 0;
        return (current + direction + visibleManualProperties.length) % visibleManualProperties.length;
      });
      return;
    }
    if (event.key === 'Enter' && visibleManualProperties[propertyHighlightIndex]) {
      event.preventDefault();
      selectManualProperty(visibleManualProperties[propertyHighlightIndex]);
    }
  }

  function setManualPropertyField(field, value) {
    setPropertyInputs((current) => {
      const next = { ...current };
      field.targets.forEach((target) => {
        next[target] = value;
      });
      return next;
    });
  }

  function manualSelectGroups(control, maximum = Number.MAX_SAFE_INTEGER) {
    const entries = (options.manualSelectOptions?.[control] || [])
      .filter(({ value }) => value <= maximum);
    const groups = new Map();
    entries.forEach((entry) => {
      const group = entry.group || 'Other';
      if (!groups.has(group)) groups.set(group, []);
      groups.get(group).push(entry);
    });
    return [...groups].map(([label, groupEntries]) => ({ label, entries: groupEntries }));
  }

  function addManualProperty() {
    onAddManualProperty({
      propertyCode: effectivePropertyCode,
      parameter: propertyInputs.parameter,
      minimum: propertyInputs.minimum,
      maximum: propertyInputs.maximum,
      rollMode: 'maximum',
    });
    setEditingAttributeIndex(edit.magicAttributes.length);
    closePropertyBuilder({ restoreFocus: false });
  }

  return (
    <AccessibleModal className="item-editor-modal" labelledBy="item-editor-title" onClose={onClose}>
        <div className="modal-heading">
          <div>
            <p className="eyebrow">D2S item record #{edit.index + 1}</p>
            <h2 id="item-editor-title">Edit {descriptor.name}</h2>
          </div>
          <button className="icon-button" type="button" aria-label="Close" onClick={onClose}>×</button>
        </div>
        <p className="modal-copy">
          Only record-compatible values are offered. Every exported item is reparsed and compared before download.
        </p>
        <div className="item-editor-layout">
          <div className="item-editor-controls">
        <div className="item-editor-utility">
          <div className="item-duplicate-control">
            <input
              aria-label="Number of duplicates"
              type="number"
              min="1"
              max="20"
              value={duplicateCount}
              disabled={busy || !onDuplicate}
              onChange={(event) => setDuplicateCount(clampInteger(event.target.value, 1, 20))}
            />
            <button
              className="button ghost compact"
              type="button"
              disabled={busy || !onDuplicate}
              title={onDuplicate ? 'Create exact copies with new D2S item IDs' : 'Move this item to a player grid before duplicating it'}
              onClick={() => onDuplicate?.(duplicateCount)}
            >
              Make {duplicateCount} duplicate{duplicateCount === 1 ? '' : 's'}
            </button>
          </div>
          <div className="item-editor-state-toggles">
            <Toggle
              label="Ethereal"
              checked={edit.ethereal}
              disabled={!options.etherealEnabled}
              onChange={(ethereal) => onChange({ ethereal })}
            />
            <Toggle
              label="Identified"
              checked={edit.identified}
              disabled={!options.identifiedEnabled}
              onChange={(identified) => onChange({ identified })}
            />
            <Toggle
              label="Personalized"
              checked={edit.personalized}
              disabled={!options.personalizedEnabled}
              onChange={(personalized) => onChange({
                personalized,
                personalizedName: personalized ? (edit.personalizedName || 'BKVince') : '',
              })}
            />
          </div>
        </div>
        {edit.personalized && (
          <label className="field personalized-name-field">
            <span>Personalized name</span>
            <input
              maxLength="15"
              value={edit.personalizedName}
              onChange={(event) => onChange({ personalizedName: event.target.value })}
            />
            <small>1–15 printable ASCII characters stored in the native v105 payload.</small>
          </label>
        )}
        <div className="item-tier-actions" aria-label="Item base tier">
          <button
            className="button ghost compact"
            type="button"
            disabled={busy || !options.downgradeBase}
            title={options.downgradeReason || `Change base to ${options.downgradeBase?.name}`}
            onClick={() => onChange(compileItemTierPatch(item, edit, 'down'))}
          >
            Downgrade
          </button>
          <button
            className="button ghost compact"
            type="button"
            disabled={busy || !options.upgradeBase}
            title={options.upgradeReason || `Change base to ${options.upgradeBase?.name}`}
            onClick={() => onChange(compileItemTierPatch(item, edit, 'up'))}
          >
            Upgrade
          </button>
          <span>
            {options.tierBases.length > 1
              ? `${['Normal', 'Exceptional', 'Elite'][options.tierIndex]} base`
              : 'No governed tier chain'}
          </span>
        </div>
        <div className="item-editor-fields">
          <label className="field item-base-field">
            <span>Quality</span>
            <select
              value={edit.quality ?? ''}
              disabled={options.qualities.length <= 1}
              onChange={(event) => onChange({ quality: Number(event.target.value) })}
            >
              {options.qualities.length === 0
                ? <option value="">Not stored by simple items</option>
                : options.qualities.map((quality) => <option key={quality.id} value={quality.id}>{quality.name}</option>)}
            </select>
            <small>
              {options.qualities.length <= 1
                ? 'Other qualities need a different payload layout and remain locked.'
                : 'Low, Normal, Superior, Magic, Set, Rare, Unique, and Crafted are offered only when this exact BKVince base has a governed payload.'}
            </small>
          </label>
          <label className="field">
            <span>Base item</span>
            <select
              value={edit.type}
              disabled={options.bases.length <= 1}
              onChange={(event) => onChange({ type: event.target.value })}
            >
              {options.bases.map((base) => (
                <option key={base.code} value={base.code}>
                  {describeItem({ type: base.code }, edit.index).name} · {base.code.toUpperCase()}
                </option>
              ))}
            </select>
            <small>Same binary family, dimensions, stack mode, or governed BKVince tier chain.</small>
          </label>
          {options.pictureVariants.length > 0 && (
            <fieldset className="item-picture-field full">
              <legend>Visual variant</legend>
              <div className="item-picture-options" role="radiogroup" aria-label="Native item visual variant">
                <button
                  className={edit.pictureId == null ? 'active' : ''}
                  type="button"
                  role="radio"
                  aria-checked={edit.pictureId == null}
                  onClick={() => onChange({ pictureId: null })}
                >
                  <span className="item-picture-thumbnail">
                    <img src={itemVisuals[edit.type]} alt="" draggable="false" />
                  </span>
                  <span>Default</span>
                </button>
                {options.pictureVariants.map((variant) => (
                  <button
                    className={edit.pictureId === variant.id ? 'active' : ''}
                    type="button"
                    role="radio"
                    aria-checked={edit.pictureId === variant.id}
                    key={variant.id}
                    onClick={() => onChange({ pictureId: variant.id })}
                  >
                    <span className="item-picture-thumbnail">
                      <img src={itemVisuals[variant.visualKey] || itemVisuals[edit.type]} alt="" draggable="false" />
                    </span>
                    <span>Variant {variant.id + 1}</span>
                  </button>
                ))}
              </div>
              <small>Writes the native 3-bit D2S picture ID used by rings, amulets, charms, and jewels.</small>
            </fieldset>
          )}
          {options.lowQualityEnabled && (
            <label className="field full">
              <span>Low quality variant</span>
              <select
                value={edit.lowQualityId}
                onChange={(event) => onChange({ lowQualityId: Number(event.target.value) })}
              >
                {options.lowQualityNames.map((entry) => (
                  <option key={entry.id} value={entry.id}>{entry.name}</option>
                ))}
              </select>
              <small>Native 3-bit variant from the governed D2R 3.3 LowQualityItems table.</small>
            </label>
          )}
          {(options.defenseEnabled || options.itemLevelEnabled) && (
            <div className="item-core-values full">
              {options.defenseEnabled && (
                <label className="field item-defense-field">
                  <span>Defense</span>
                  <input
                    type="number"
                    min="0"
                    max="2037"
                    value={edit.defense}
                    onChange={(event) => onChange({ defense: clampInteger(event.target.value, 0, 2037) })}
                  />
                  <small>
                    Base range {options.defenseMinimum ?? 0}–{options.defenseMaximum ?? 2037}; custom encoded values remain available.
                  </small>
                </label>
              )}
              {options.itemLevelEnabled && (
                <label className="field item-level-field">
                  <span>Item Level</span>
                  <input
                    type="number"
                    min="0"
                    max="127"
                    value={edit.itemLevel}
                    onChange={(event) => onChange({ itemLevel: clampInteger(event.target.value, 0, 127) })}
                  />
                  <small>Native 7-bit item level used by affix eligibility; BKVince normally uses 1–99.</small>
                </label>
              )}
            </div>
          )}
          {options.durabilityEnabled && (
            <details className="item-editor-advanced item-durability-fields full">
              <summary>Durability</summary>
              <div className="durability-fields">
              <label className="field">
                <span>Current Durability</span>
                <input
                  type="number"
                  min="0"
                  max={edit.maximumDurability}
                  value={edit.currentDurability ?? 0}
                  onChange={(event) => onChange({
                    currentDurability: clampInteger(event.target.value, 0, edit.maximumDurability),
                  })}
                />
              </label>
              <label className="field">
                <span>Max Durability</span>
                <input
                  type="number"
                  min="0"
                  max="255"
                  value={edit.maximumDurability}
                  onChange={(event) => {
                    const maximumDurability = clampInteger(event.target.value, 0, 255);
                    onChange({
                      maximumDurability,
                      currentDurability: Math.min(edit.currentDurability, maximumDurability),
                    });
                  }}
                />
              </label>
                <small>Governed base durability: {options.baseDurability ?? 0}.</small>
              </div>
            </details>
          )}
          {options.magicEnabled && (
            <>
              <div className="magic-affix-grid">
                <label className="field">
                <span>Magic prefix</span>
                <select
                  value={edit.magicPrefix}
                  onChange={(event) => onChange({ magicPrefix: Number(event.target.value) })}
                >
                  <option value="0">No prefix</option>
                  {options.prefixes.map((affix) => (
                    <option key={affix.id} value={affix.id}>{affix.name} · #{affix.id}</option>
                  ))}
                </select>
                <small>Spawnable for this BKVince item type and item level.</small>
                </label>
                <label className="field">
                <span>Magic suffix</span>
                <select
                  value={edit.magicSuffix}
                  onChange={(event) => onChange({ magicSuffix: Number(event.target.value) })}
                >
                  <option value="0">No suffix</option>
                  {options.suffixes.map((affix) => (
                    <option key={affix.id} value={affix.id}>{affix.name} · #{affix.id}</option>
                  ))}
                </select>
                <small>ABI IDs follow the d2s parser, including its Expansion separator.</small>
                </label>
              </div>
              <div className={`affix-compiler ${options.affixCompiler.supported ? '' : 'locked'}`}>
                <div>
                  <strong>Affix property compiler</strong>
                  <span>
                    {options.affixCompiler.supported
                      ? `${options.affixCompiler.modCount} governed mod declaration(s) compile into ${options.affixCompiler.attributeCount} D2S attribute group(s)${options.affixCompiler.socketCount ? ` and ${options.affixCompiler.socketCount} socket(s)` : ''}. Applying a roll replaces the complete Magic Attributes list and applies its governed structure; Undo restores it.`
                      : options.affixCompiler.reason}
                  </span>
                </div>
                <div className="affix-compiler-actions">
                  <button
                    className="button ghost compact"
                    type="button"
                    disabled={!options.affixCompiler.supported}
                    onClick={() => onChange(compileMagicAffixPatch(item, edit, 'minimum'))}
                  >
                    Apply minimum rolls
                  </button>
                  <button
                    className="button ghost compact"
                    type="button"
                    disabled={!options.affixCompiler.supported}
                    onClick={() => onChange(compileMagicAffixPatch(item, edit, 'maximum'))}
                  >
                    Apply maximum rolls
                  </button>
                </div>
              </div>
            </>
          )}
          {options.rareQualityEnabled && (
            <>
              <div className="magic-affix-grid">
                <label className="field">
                  <span>Rare name prefix</span>
                  <select
                    value={edit.rareNamePrefixId}
                    onChange={(event) => onChange({ rareNamePrefixId: Number(event.target.value) })}
                  >
                    {options.rareNamePrefixes.map((entry) => (
                      <option key={entry.id} value={entry.id}>{entry.name} &middot; #{entry.id}</option>
                    ))}
                  </select>
                  <small>First word of the native Rare / Crafted name.</small>
                </label>
                <label className="field">
                  <span>Rare name suffix</span>
                  <select
                    value={edit.rareNameSuffixId}
                    onChange={(event) => onChange({ rareNameSuffixId: Number(event.target.value) })}
                  >
                    {options.rareNameSuffixes.map((entry) => (
                      <option key={entry.id} value={entry.id}>{entry.name} &middot; #{entry.id}</option>
                    ))}
                  </select>
                  <small>Second word, filtered by this exact BKVince item family.</small>
                </label>
              </div>
              <div className="rare-affix-slots">
                {Array.from({ length: 3 }, (_, pairIndex) => (
                  <div className="magic-affix-grid" key={`rare-affix-pair-${pairIndex + 1}`}>
                    {[
                      { label: `Rare prefix ${pairIndex + 1}`, slot: pairIndex * 2, entries: options.rareAffixPrefixes },
                      { label: `Rare suffix ${pairIndex + 1}`, slot: pairIndex * 2 + 1, entries: options.rareAffixSuffixes },
                    ].map(({ label, slot, entries }) => (
                      <label className="field" key={label}>
                        <span>{label}</span>
                        <select
                          value={edit.rareAffixIds[slot] ?? 0}
                          onChange={(event) => {
                            const rareAffixIds = [...edit.rareAffixIds];
                            rareAffixIds[slot] = Number(event.target.value) || null;
                            onChange({ rareAffixIds });
                          }}
                        >
                          <option value="0">No {slot % 2 === 0 ? 'prefix' : 'suffix'}</option>
                          {entries.map((affix) => (
                            <option key={affix.id} value={affix.id}>{affix.name} &middot; #{affix.id}</option>
                          ))}
                        </select>
                        <small>Rare-enabled, spawnable, level-compatible governed affix.</small>
                      </label>
                    ))}
                  </div>
                ))}
              </div>
              <div className={`affix-compiler ${options.rareAffixCompiler.supported ? '' : 'locked'}`}>
                <div>
                  <strong>{edit.quality === 6 ? 'Rare' : 'Crafted'} affix compiler</strong>
                  <span>
                    {options.rareAffixCompiler.supported
                      ? `${options.rareAffixCompiler.modCount} governed mod declaration(s) compile into ${options.rareAffixCompiler.attributeCount} D2S attribute group(s)${options.rareAffixCompiler.socketCount ? ` and ${options.rareAffixCompiler.socketCount} socket(s)` : ''}. Applying a roll replaces Item Attributes from all six native affix slots.`
                      : options.rareAffixCompiler.reason}
                  </span>
                </div>
                <div className="affix-compiler-actions">
                  <button
                    className="button ghost compact"
                    type="button"
                    disabled={!options.rareAffixCompiler.supported}
                    onClick={() => onChange(compileRareAffixPatch(item, edit, 'minimum'))}
                  >
                    Apply minimum rolls
                  </button>
                  <button
                    className="button ghost compact"
                    type="button"
                    disabled={!options.rareAffixCompiler.supported}
                    onClick={() => onChange(compileRareAffixPatch(item, edit, 'maximum'))}
                  >
                    Apply maximum rolls
                  </button>
                </div>
              </div>
            </>
          )}
          {options.namedQualityEnabled && (
            <>
              <label className="field full item-named-field">
                <span>{edit.quality === 5 ? 'Set item' : 'Unique item'}</span>
                <select
                  value={edit.quality === 5 ? edit.setId : edit.uniqueId}
                  onChange={(event) => onChange(edit.quality === 5
                    ? { setId: Number(event.target.value) }
                    : { uniqueId: Number(event.target.value) })}
                >
                  {(edit.quality === 5 ? options.setItems : options.uniqueItems).map((entry) => (
                    <option key={entry.id} value={entry.id}>{entry.name} · #{entry.id}</option>
                  ))}
                </select>
                <small>Named records remain attached to their governed Normal / Exceptional / Elite base family.</small>
              </label>
              {edit.quality === 7 && options.namedQualityVariants && (
                <label className="field full">
                  <span>{options.namedQualityVariants.label}</span>
                  <select
                    value={options.namedQualityVariants.selectedId}
                    onChange={(event) => onChange(compileNamedQualityPatch(
                      item,
                      edit,
                      'maximum',
                      event.target.value,
                    ))}
                  >
                    {options.namedQualityVariants.entries.map((variant) => (
                      <option key={variant.id} value={variant.id}>{variant.label}</option>
                    ))}
                  </select>
                  <small>
                    {options.namedQualityVariants.entries.find(({ id }) => (
                      id === options.namedQualityVariants.selectedId
                    ))?.detail}. Changing the exclusive variant applies its Perfect roll and replaces only the governed Unique payload.
                  </small>
                </label>
              )}
              <details className={`affix-compiler item-editor-advanced item-named-rolls ${options.namedQualityCompiler.supported ? '' : 'locked'}`}>
                <summary>Roll options</summary>
                <div className="item-editor-advanced-content">
                  <span>
                    {options.namedQualityCompiler.supported
                      ? `${options.namedQualityCompiler.modCount} governed property declaration(s) compile into ${options.namedQualityCompiler.attributeCount} D2S attribute group(s)${options.namedQualityCompiler.socketCount ? ` and ${options.namedQualityCompiler.socketCount} socket(s)` : ''}. Applying a roll replaces the complete Item Attributes list.`
                      : options.namedQualityCompiler.reason}
                  </span>
                  <div className="affix-compiler-actions">
                    <button
                      className="button ghost compact"
                      type="button"
                      disabled={!options.namedQualityCompiler.supported}
                      onClick={() => onChange(compileNamedQualityPatch(item, edit, 'minimum'))}
                    >
                      Minimum
                    </button>
                    <button
                      className="button ghost compact"
                      type="button"
                      disabled={!options.namedQualityCompiler.supported}
                      onClick={() => onChange(compileNamedQualityPatch(item, edit, 'maximum'))}
                    >
                      Perfect
                    </button>
                  </div>
                </div>
              </details>
            </>
          )}
          {edit.quality === 5 && (
            <section className="set-bonus-editor">
              <div className="magic-attribute-heading">
                <div>
                  <strong>Item Set Bonuses</strong>
                  <span>
                    Each active row is stored behind its exact D2S plist bit. Non-contiguous masks are preserved instead of being flattened.
                  </span>
                </div>
                <span className="set-mask">Mask {edit.setBonusMask.toString(2).padStart(5, '0')}</span>
              </div>
              <div className="set-bonus-list">
                {options.setBonusLists.filter(({ active }) => active).map((list) => {
                  const bonusLines = list.attributes.length > 0
                    ? describeItem({
                      type: edit.type,
                      level: edit.itemLevel,
                      magic_attributes: list.attributes,
                    }, list.bit, characterLevel).magicAttributes
                    : [];
                  return (
                  <div className={`set-bonus-row ${list.active ? 'active' : ''}`} key={list.bit}>
                    <div>
                      <strong>Bonus list {list.bit + 1}</strong>
                      {bonusLines.length > 0 ? (
                        <span className="set-bonus-preview-lines">
                          {bonusLines.map((line, index) => <span key={`${line}-${index}`}>{line}</span>)}
                        </span>
                      ) : (
                        <span>
                          {list.propertyCodes.length > 0
                            ? list.propertyCodes.join(' · ')
                            : `${list.attributes.length} preserved attribute group(s)`}
                        </span>
                      )}
                    </div>
                    <div className="set-bonus-actions">
                      <button
                        className="button ghost compact"
                        type="button"
                        disabled={!list.supported}
                        title={list.reason || 'Compile minimum governed values'}
                        onClick={() => onChange(compileSetBonusPatch(item, edit, list.bit, 'minimum'))}
                      >
                        Min
                      </button>
                      <button
                        className="button ghost compact"
                        type="button"
                        disabled={!list.supported}
                        title={list.reason || 'Compile maximum governed values'}
                        onClick={() => onChange(compileSetBonusPatch(item, edit, list.bit, 'maximum'))}
                      >
                        Perfect
                      </button>
                      <button
                        className="attribute-delete"
                        type="button"
                        disabled={!list.active}
                        aria-label={`Remove Set bonus list ${list.bit + 1}`}
                        onClick={() => onChange(removeSetBonusPatch(item, edit, list.bit))}
                      >
                        <TrashIcon />
                      </button>
                    </div>
                  </div>
                  );
                })}
                <select
                  className="set-bonus-add"
                  aria-label="Add set attributes"
                  value=""
                  disabled={options.setBonusLists.every(({ supported, active }) => !supported || active)}
                  onChange={(event) => {
                    const bit = Number(event.target.value);
                    if (Number.isInteger(bit)) onChange(compileSetBonusPatch(item, edit, bit, 'maximum'));
                  }}
                >
                  <option value="">Add set attributes…</option>
                  {options.setBonusLists.filter(({ supported, active }) => supported && !active).map((list) => (
                    <option key={list.bit} value={list.bit}>Bonus list {list.bit + 1}</option>
                  ))}
                </select>
              </div>
            </section>
          )}
          {edit.socketed && [2, 3].includes(Number(edit.quality)) && options.runewords.length > 0 && (
            <section className={`runeword-editor ${selectedRuneword?.compiler.supported ? '' : 'locked'}`}>
              <div className="magic-attribute-heading">
                <div>
                  <strong>Runeword</strong>
                  <span>
                    Recipes, allowed bases, rune order, IDs, and property rolls come directly from BKVince runes.txt.
                  </span>
                </div>
                {edit.runewordId !== null && <span className="set-mask">Active #{edit.runewordId}</span>}
              </div>
              <label className="field full">
                <span>Recipe</span>
                <select
                  value={selectedRunewordId ?? ''}
                  onChange={(event) => setRequestedRunewordId(event.target.value)}
                >
                  {options.runewords.map((runeword) => (
                    <option key={runeword.id} value={runeword.id}>
                      {runeword.name} · {runeword.runes.map((rune) => rune.toUpperCase()).join(' + ')}
                      {runeword.compiler.supported ? '' : ' · locked'}
                    </option>
                  ))}
                </select>
              </label>
              {selectedRuneword && (
                <>
                  <div className="runeword-recipe" aria-label={`${selectedRuneword.name} rune order`}>
                    {selectedRuneword.runes.map((rune, index) => (
                      <span className="rune-chip" key={`${rune}-${index}`}>
                        <b>{index + 1}</b>{describeItem({ type: rune }, index).name}
                      </span>
                    ))}
                  </div>
                  <small>
                    {selectedRuneword.mods.map(({ code }) => code).join(' · ')}
                  </small>
                  <div className="runeword-actions">
                    <button
                      className="button ghost compact"
                      type="button"
                      disabled={busy || !selectedRuneword.compiler.supported}
                      title={selectedRuneword.compiler.reason || 'Build with minimum governed rolls'}
                      onClick={() => onApplyRuneword(selectedRuneword.id, 'minimum')}
                    >
                      Build minimum
                    </button>
                    <button
                      className="button ghost compact"
                      type="button"
                      disabled={busy || !selectedRuneword.compiler.supported}
                      title={selectedRuneword.compiler.reason || 'Build with maximum governed rolls'}
                      onClick={() => onApplyRuneword(selectedRuneword.id, 'maximum')}
                    >
                      Build maximum
                    </button>
                    <button
                      className="button danger compact"
                      type="button"
                      disabled={busy || edit.runewordId === null}
                      onClick={onClearRuneword}
                    >
                      Break runeword
                    </button>
                  </div>
                  <small className={selectedRuneword.compiler.supported ? '' : 'socket-warning'}>
                    {selectedRuneword.compiler.supported
                      ? `${selectedRuneword.compiler.attributeCount} generated D2S attribute group(s). Building fills empty sockets atomically; occupied non-recipe sockets are never overwritten.`
                      : selectedRuneword.compiler.reason}
                  </small>
                </>
              )}
            </section>
          )}
          <div className="toggle-grid modal-toggles item-socket-toggle">
            <Toggle
              label="Socketed"
              checked={edit.socketed}
              disabled={!options.socketedEnabled || options.filledSockets > 0}
              onChange={(socketed) => onChange({
                socketed,
                totalSockets: socketed ? Math.max(1, edit.totalSockets || 1) : 0,
              })}
            />
          </div>
          <div className="item-lock-notes item-socket-notes">
            {!options.identifiedEnabled && <small>{options.identifiedReason}</small>}
            {!options.etherealEnabled && <small>{options.etherealReason}</small>}
            {!options.personalizedEnabled && <small>{options.personalizedReason}</small>}
            {!options.socketedEnabled && <small>{options.socketedReason}</small>}
            {options.filledSockets > 0 && <small>Extract socketed items before removing the socket structure.</small>}
          </div>
          {edit.socketed && options.socketedEnabled && (
            <label className="field full item-socket-count">
              <span>Sockets</span>
              <select
                value={edit.totalSockets}
                onChange={(event) => onChange({ totalSockets: Number(event.target.value) })}
              >
                {Array.from(
                  { length: options.socketMaximum - Math.max(1, options.filledSockets) + 1 },
                  (_, offset) => Math.max(1, options.filledSockets) + offset,
                ).map((count) => <option key={count} value={count}>{count}</option>)}
              </select>
              <small>
                This BKVince base accepts up to {options.socketMaximum} socket(s); {options.filledSockets} are occupied.
              </small>
            </label>
          )}
          {edit.socketed && options.socketedEnabled && (
            <section className="socket-content-editor item-socket-content">
              <div className="magic-attribute-heading">
                <div>
                  <strong>Socketed Items</strong>
                  <span>Fillers are stored recursively after the parent in exact socket order.</span>
                </div>
                <span className="socket-capacity">{options.filledSockets}/{edit.totalSockets}</span>
              </div>
              <div className="socket-filler-list">
                {edit.socketedItems.map((socketedItem, socketIndex) => {
                  const socketedDescriptor = describeItem(socketedItem, socketIndex);
                  const socketedVisual = itemVisuals[String(socketedItem.type).toLowerCase()];
                  return (
                    <div className="socket-filler-row" key={`${socketedItem.id || socketedItem.type}-${socketIndex}`}>
                      <span className="socket-gem" aria-hidden="true">
                        {socketedVisual
                          ? <img src={socketedVisual} alt="" draggable="false" />
                          : '◆'}
                      </span>
                      <div>
                        <strong>{socketedDescriptor.name}</strong>
                        <span>{socketedItem.type.toUpperCase()} · socket {socketIndex + 1}</span>
                      </div>
                      <div className="socket-filler-actions">
                        <button
                          className="button ghost compact"
                          type="button"
                          disabled={busy || !onExtractSocket}
                          title={onExtractSocket
                            ? 'Extract into Inventory, break any active runeword, and compact the remaining socket order'
                            : 'Mercenaries have no inventory container; move the parent item to the player before extracting.'}
                          onClick={() => onExtractSocket?.(socketIndex)}
                        >
                          Extract
                        </button>
                        <button
                          className="attribute-delete socket-filler-delete"
                          type="button"
                          disabled={busy || !onDeleteSocket}
                          aria-label={`Delete ${socketedDescriptor.name} from socket ${socketIndex + 1}`}
                          title="Delete this filler; Undo restores it"
                          onClick={() => onDeleteSocket?.(socketIndex)}
                        >
                          <TrashIcon />
                        </button>
                      </div>
                    </div>
                  );
                })}
                {edit.socketedItems.length === 0 && <p>No socket fillers stored.</p>}
              </div>
              <div className="socket-filler-add">
                <select
                  aria-label="Socket filler to insert"
                  value={selectedSocketType}
                  disabled={busy || options.filledSockets >= edit.totalSockets}
                  onChange={(event) => setSelectedSocketType(event.target.value)}
                >
                  {options.socketFillers.map((filler) => (
                    <option key={filler.code} value={filler.code}>
                      {describeItem({ type: filler.code }, 0).name} · {filler.code.toUpperCase()}
                    </option>
                  ))}
                </select>
                <button
                  className="button ghost compact"
                  type="button"
                  disabled={busy || !selectedSocketType || options.filledSockets >= edit.totalSockets}
                  onClick={() => onInsertSocket(selectedSocketType)}
                >
                  Insert filler
                </button>
                <label
                  className="button ghost compact socket-import-control"
                  aria-disabled={busy || options.filledSockets >= edit.totalSockets}
                >
                  <UploadIcon /> Import filler files
                  <input
                    className="visually-hidden"
                    type="file"
                    multiple
                    disabled={busy || options.filledSockets >= edit.totalSockets}
                    accept=".d2i,.json,application/octet-stream,application/json"
                    onChange={(event) => {
                      if (event.target.files?.length) onImportSocket(event.target.files);
                      event.target.value = '';
                    }}
                  />
                </label>
              </div>
              <small>
                Canonical .d2i records and BKVince bundles may contain Magic, Set, Unique, rune, gem, or jewel fillers. The whole selection is rejected if one record is invalid or capacity is exceeded.
              </small>
              {edit.runewordId !== null ? (
                <small className="socket-warning">Extracting any filler automatically breaks the active runeword before the remaining sockets are compacted.</small>
              ) : null}
            </section>
          )}
          {options.quantityEnabled && (
            <label className="field full">
              <span>Quantity</span>
              <input
                type="number"
                min="0"
                max={options.quantityMaximum}
                value={edit.quantity ?? ''}
                placeholder="No quantity stored"
                onChange={(event) => {
                  if (event.target.value !== '') {
                    onChange({ quantity: clampInteger(event.target.value, 0, options.quantityMaximum) });
                  }
                }}
              />
              <small>The v105 quantity field accepts 0–{options.quantityMaximum}.</small>
            </label>
          )}
          <section className={`magic-attribute-editor ${attributesEditable ? '' : 'locked'}`}>
            <div className="magic-attribute-heading">
              <div>
                <strong>Magic Attributes</strong>
                <span>
                  {options.magicEnabled
                    ? 'Affix changes keep the current list until you explicitly apply minimum or maximum governed rolls. Numeric and governed elemental/skill/charge encodings are rebuilt atomically.'
                    : options.rareQualityEnabled
                      ? 'Rare and Crafted items expose both native name words and all three prefix / suffix pairs. Apply governed rolls, then fine-tune the encoded attributes if needed.'
                    : options.namedQualityEnabled
                      ? 'Set and Unique properties come directly from the selected BKVince table row; apply a governed roll, then fine-tune safe numeric values if needed.'
                      : attributesEditable
                        ? 'This complex D2S record can store custom attributes at its current quality. Safe numeric values and governed BKVince properties are written directly to its native attribute payload.'
                        : options.attributesReason}
                </span>
              </div>
            </div>
            <div className="magic-attribute-list">
              {attributeDisplayGroups.map((group) => {
                const [attribute] = group.attributes;
                const [attributeIndex] = group.attributeIndices;
                const [definition] = group.definitions;
                const [label] = group.labels;
                const editing = editingAttributeIndex === attributeIndex;
                const grouped = group.attributeIndices.length > 1;
                const editableGroup = group.definitions.every(Boolean);
                const statSummary = group.attributes.map(({ id }) => `#${id}`).join(', ');
                return (
                  <div
                    className={`magic-attribute-row ${editing ? 'editing' : ''}`}
                    key={`magic-${group.attributeIndices.join('-')}-${group.attributes.map(({ id }) => id).join('-')}`}
                  >
                    <div className="magic-attribute-summary">
                      <strong>{group.summary}</strong>
                      <span>
                        {grouped
                          ? `${group.attributeIndices.length} native attribute groups · Stats ${statSummary}`
                          : `${label} · Stat #${attribute.id}${definition ? '' : ' · complex encoding preserved'}`}
                      </span>
                    </div>
                    <div className="magic-attribute-actions">
                      <button
                        className="attribute-edit"
                        type="button"
                        disabled={!attributesEditable || !editableGroup}
                        aria-label={`${editing ? 'Close editor for' : 'Edit'} ${group.summary}`}
                        aria-expanded={editing}
                        onClick={() => setEditingAttributeIndex(editing ? null : attributeIndex)}
                      >
                        <EditIcon />
                      </button>
                      <button
                        className="attribute-delete"
                        type="button"
                        disabled={!attributesEditable}
                        aria-label={`Remove ${group.summary}`}
                        onClick={() => {
                          setEditingAttributeIndex(null);
                          onChange({
                            magicAttributes: edit.magicAttributes.filter((_, index) => (
                              !group.attributeIndices.includes(index)
                            )),
                          });
                        }}
                      >
                        <TrashIcon />
                      </button>
                    </div>
                    {editing && (
                      <div className="magic-attribute-values">
                        {attribute.values.map((value, valueIndex) => {
                          const valueDefinitions = group.definitions.map((entry) => entry?.values[valueIndex]);
                          const valueDefinition = valueDefinitions[0];
                          const minimum = Math.max(...valueDefinitions.map((entry) => entry.minimum));
                          const maximum = Math.min(...valueDefinitions.map((entry) => entry.maximum));
                          const combinedSkillTab = !grouped && attribute.id === 188 && valueIndex === 0;
                          if (!grouped && attribute.id === 188 && valueIndex === 1) return null;
                          const control = combinedSkillTab ? 'skillTab' : valueDefinition?.control;
                          const displayedValue = combinedSkillTab
                            ? (attribute.values[1] * 3) + attribute.values[0]
                            : value;
                          const selectGroups = control && control !== 'number'
                            ? manualSelectGroups(control, maximum)
                            : [];
                          const selectHasCurrentValue = selectGroups.some(({ entries }) => (
                            entries.some(({ value: optionValue }) => optionValue === displayedValue)
                          ));
                          const updateValue = (nextValue) => {
                            if (!valueDefinition) return;
                            const magicAttributes = structuredClone(edit.magicAttributes);
                            group.attributeIndices.forEach((groupAttributeIndex, groupIndex) => {
                              if (combinedSkillTab) {
                                magicAttributes[groupAttributeIndex].values[0] = nextValue % 3;
                                magicAttributes[groupAttributeIndex].values[1] = Math.floor(nextValue / 3);
                                return;
                              }
                              const groupValueDefinition = valueDefinitions[groupIndex];
                              magicAttributes[groupAttributeIndex].values[valueIndex] = clampInteger(
                                nextValue,
                                groupValueDefinition.minimum,
                                groupValueDefinition.maximum,
                              );
                            });
                            onChange({ magicAttributes });
                          };
                          return (
                            <label key={valueIndex}>
                              <span>{combinedSkillTab ? 'Skill tab' : (grouped ? 'Shared value' : (valueDefinition?.name || `Value ${valueIndex + 1}`))}</span>
                              {control && control !== 'number' ? (
                                <select
                                  disabled={!attributesEditable || !valueDefinition}
                                  value={displayedValue}
                                  onChange={(event) => updateValue(Number(event.target.value))}
                                >
                                  {!selectHasCurrentValue && (
                                    <option value={displayedValue}>Stored value #{displayedValue}</option>
                                  )}
                                  {selectGroups.map(({ label: groupLabel, entries }) => (
                                    <optgroup key={groupLabel} label={groupLabel}>
                                      {entries.map((entry) => (
                                        <option key={entry.value} value={entry.value}>{entry.label}</option>
                                      ))}
                                    </optgroup>
                                  ))}
                                </select>
                              ) : (
                                <BufferedNumberStepper
                                  label={valueDefinition?.name || `Value ${valueIndex + 1}`}
                                  disabled={!attributesEditable || !valueDefinition}
                                  min={minimum}
                                  max={maximum}
                                  value={value}
                                  onChange={updateValue}
                                />
                              )}
                            </label>
                          );
                        })}
                      </div>
                    )}
                  </div>
                );
              })}
              {edit.magicAttributes.length === 0 && <p>No item attributes stored.</p>}
            </div>
            <button
              className="property-builder-toggle"
              type="button"
              ref={propertyToggleRef}
              disabled={!attributesEditable}
              aria-expanded={propertyBuilderOpen}
              aria-controls="manual-property-picker"
              onClick={() => (propertyBuilderOpen ? closePropertyBuilder() : setPropertyBuilderOpen(true))}
            >
              <PlusIcon /> Add magic attributes… <span aria-hidden="true">{propertyBuilderOpen ? '▲' : '▼'}</span>
            </button>
            {propertyBuilderOpen && (
              <div className="manual-property-builder" id="manual-property-picker" data-escape-preserve-dialog>
              <div className="magic-attribute-heading">
                <div>
                  <strong>Choose an attribute</strong>
                  <span>Human-readable BKVince properties compile to their exact native D2S attributes.</span>
                </div>
                <span className="property-result-count">{filteredManualProperties.length}</span>
              </div>
              <div className="property-search-control">
                <SearchIcon />
                <input
                  ref={propertySearchRef}
                  type="search"
                  role="combobox"
                  aria-label="Search item attributes"
                  aria-controls="manual-property-results"
                  aria-expanded="true"
                  aria-activedescendant={visibleManualProperties[propertyHighlightIndex]
                    ? `manual-property-${visibleManualProperties[propertyHighlightIndex].code}`
                    : undefined}
                  value={propertySearch}
                  disabled={!attributesEditable}
                  placeholder="Search for an attribute"
                  onKeyDown={handlePropertySearchKeyDown}
                  onChange={(event) => {
                    setPropertySearch(event.target.value);
                    setPropertyHighlightIndex(0);
                  }}
                />
              </div>
              <div
                className="manual-property-results"
                id="manual-property-results"
                role="listbox"
                aria-label="Item attributes"
              >
                {visibleManualProperties.map((property, index) => (
                  <button
                    className={`manual-property-option ${property.code === effectivePropertyCode ? 'selected' : ''} ${index === propertyHighlightIndex ? 'highlighted' : ''}`}
                    id={`manual-property-${property.code}`}
                    key={property.code}
                    type="button"
                    role="option"
                    aria-selected={property.code === effectivePropertyCode}
                    onMouseEnter={() => setPropertyHighlightIndex(index)}
                    onClick={() => selectManualProperty(property)}
                  >
                    <span>
                      <strong>{property.label}</strong>
                      <small>{property.code}</small>
                    </span>
                    <em>{property.attributeCount} D2S {property.attributeCount === 1 ? 'attribute' : 'attributes'}</em>
                  </button>
                ))}
                {filteredManualProperties.length === 0 && (
                  <p className="manual-property-empty">No governed property matches “{propertySearch}”.</p>
                )}
              </div>
              {filteredManualProperties.length > visibleManualProperties.length && (
                <small>Showing the first {visibleManualProperties.length} matches. Refine the search to reach the others.</small>
              )}
              {selectedManualProperty && (
                <div className="manual-property-selection">
                  <div>
                    <strong>{selectedManualProperty.label}</strong>
                    <span>{selectedManualProperty.attributeCount} native D2S attribute group{selectedManualProperty.attributeCount === 1 ? '' : 's'} · {selectedManualProperty.code}</span>
                  </div>
              <div className="manual-property-values">
                {selectedManualProperty.fields.map((field) => {
                  const value = propertyInputs[field.targets[0]] ?? '';
                  const groups = field.control === 'number'
                    ? []
                    : manualSelectGroups(field.control, field.maximum);
                  return (
                    <label key={field.id} className={field.control === 'skill' ? 'full' : ''}>
                      <span>{field.label}</span>
                      {field.control === 'number' ? (
                        <NumberStepper
                          label={field.label}
                          value={value}
                          disabled={!attributesEditable}
                          min={field.minimum}
                          max={field.maximum}
                          onChange={(nextValue) => setManualPropertyField(field, nextValue)}
                        />
                      ) : (
                        <select
                          value={value}
                          disabled={!attributesEditable}
                          onChange={(event) => setManualPropertyField(field, event.target.value)}
                        >
                          <option value="">Choose {field.label.toLocaleLowerCase('en-US')}...</option>
                          {groups.map(({ label: groupLabel, entries }) => (
                            <optgroup key={groupLabel} label={groupLabel}>
                              {entries.map((entry) => (
                                <option key={entry.value} value={entry.value}>{entry.label}</option>
                              ))}
                            </optgroup>
                          ))}
                        </select>
                      )}
                    </label>
                  );
                })}
                {selectedManualProperty.fields.length === 0 && (
                  <p className="manual-property-no-values">This attribute has no variable value.</p>
                )}
              </div>
              <small>
                {selectedManualProperty?.notes
                  || selectedManualProperty?.label
                  || 'Choose a governed BKVince property.'}
              </small>
              <div
                className={`manual-property-preview ${manualPropertyEvaluation.error ? 'error' : ''}`}
                role={manualPropertyEvaluation.error ? 'alert' : 'status'}
              >
                <span>In-game preview</span>
                {manualPropertyEvaluation.valid ? (
                  manualPropertyEvaluation.descriptions.map((description) => (
                    <strong key={description}>{description}</strong>
                  ))
                ) : (
                  <strong>
                    {manualPropertyEvaluation.error
                      || `Complete the ${selectedManualProperty.fields.length || 'required'} field${selectedManualProperty.fields.length === 1 ? '' : 's'} to preview the exact result.`}
                  </strong>
                )}
              </div>
              <div className="manual-property-actions">
                <button
                  className="button primary compact"
                  type="button"
                  disabled={!attributesEditable || !manualPropertyEvaluation.valid}
                  onClick={addManualProperty}
                >
                  Add attribute
                </button>
              </div>
                </div>
              )}
              <details className="raw-attribute-details">
                <summary>Advanced: add a raw numeric stat</summary>
                <div className="magic-attribute-add">
                  <label>
                    <span>Native stat</span>
                    <select
                      value={selectedAttributeId}
                      disabled={!attributesEditable}
                      onChange={(event) => setSelectedAttributeId(event.target.value)}
                    >
                      {options.magicAttributes.map((attribute) => (
                        <option key={attribute.id} value={attribute.id}>
                          {attribute.label} · #{attribute.id}
                        </option>
                      ))}
                    </select>
                  </label>
                  <button
                    className="button ghost compact"
                    type="button"
                    disabled={!attributesEditable || selectedAttributeId === ''}
                    onClick={() => {
                      const definition = options.magicAttributes.find(({ id }) => id === Number(selectedAttributeId));
                      if (!definition) return;
                      setEditingAttributeIndex(edit.magicAttributes.length);
                      closePropertyBuilder({ restoreFocus: false });
                      onChange({
                        magicAttributes: [
                          ...edit.magicAttributes,
                          {
                            id: definition.id,
                            name: definition.name,
                            values: definition.values.map(({ defaultValue }) => defaultValue),
                          },
                        ],
                      });
                    }}
                  >
                    Add raw stat
                  </button>
                </div>
              </details>
              </div>
            )}
          </section>
        </div>
        <details className="item-transfer-panel">
          <summary>Move item</summary>
          <div className="item-transfer-actions">
            {transferTargets.map((target) => (
              <button
                className="button ghost compact"
                type="button"
                key={target.scope}
                disabled={busy || target.disabled}
                title={target.reason || `Move to ${target.label}`}
                onClick={() => onTransfer(target.scope)}
              >
                {target.scope === 'trash' ? 'Move to Trash' : `Move to ${target.label}`}
              </button>
            ))}
          </div>
        </details>
          </div>
          <aside className="item-editor-preview" aria-label="Live item preview">
            <div className={`item-preview-glyph quality-${(descriptor.quality || 'simple').toLowerCase()}`} aria-hidden="true">
              <ItemVisual descriptor={descriptor} large />
            </div>
            {descriptor.tint && (
              <div className="item-transform-label">
                <span
                  className="item-transform-swatch"
                  style={{ backgroundColor: descriptor.tint.color }}
                  aria-hidden="true"
                />
                <span>{descriptor.tint.label} tint</span>
              </div>
            )}
            <strong className={`quality-${(descriptor.quality || 'simple').toLowerCase()}`}>{descriptor.name}</strong>
            {descriptor.name !== descriptor.baseName && <span>{descriptor.baseName}</span>}
            <ItemPropertyLines descriptor={previewDescriptor} />
          </aside>
        </div>
        <div className="modal-actions">
          <button
            className="button danger"
            type="button"
            title={deleteLabel === 'Move to Trash' ? 'Move this item into recoverable Trash' : deleteLabel}
            onClick={onDelete}
          >
            {deleteLabel === 'Move to Trash' ? 'Delete' : deleteLabel}
          </button>
          <button className="button ghost" type="button" onClick={onDownload}>
            <DownloadIcon /> Download
          </button>
          <button className="button primary" type="button" onClick={onClose}>Save Changes</button>
        </div>
    </AccessibleModal>
  );
}

function NumberField({ label, value, min, max, onChange }) {
  return (
    <label className="field numeric-field">
      <span>{label}</span>
      <input
        type="number"
        inputMode="numeric"
        value={value}
        min={min}
        max={max}
        onChange={(event) => onChange(clampInteger(event.target.value, min, max))}
      />
      <small>{formatNumber(min)}–{formatNumber(max)}</small>
    </label>
  );
}

function BufferedNumberStepper({ label, value, min, max, disabled = false, onChange }) {
  const hasMinimum = min !== undefined && min !== null && Number.isFinite(Number(min));
  const hasMaximum = max !== undefined && max !== null && Number.isFinite(Number(max));
  const storedValue = String(value ?? '');
  const [draftValue, setDraftValue] = useState(storedValue);

  useEffect(() => {
    setDraftValue(storedValue);
  }, [storedValue]);

  function commitDraftValue() {
    const candidate = draftValue.trim();
    if (candidate === '' || candidate === '-' || candidate === '+' || !Number.isFinite(Number(candidate))) {
      setDraftValue(storedValue);
      return;
    }

    const nextValue = clampInteger(
      candidate,
      hasMinimum ? Number(min) : Number.MIN_SAFE_INTEGER,
      hasMaximum ? Number(max) : Number.MAX_SAFE_INTEGER,
    );
    setDraftValue(String(nextValue));
    if (nextValue !== Number(value)) onChange(nextValue);
  }

  return (
    <input
      className="rune-number-input"
      type="number"
      inputMode="numeric"
      autoComplete="off"
      aria-label={label}
      step={1}
      value={draftValue}
      min={hasMinimum ? min : undefined}
      max={hasMaximum ? max : undefined}
      disabled={disabled}
      onChange={(event) => setDraftValue(event.target.value)}
      onBlur={commitDraftValue}
      onKeyDown={(event) => {
        if (event.key === 'Enter') {
          event.preventDefault();
          event.currentTarget.blur();
        } else if (event.key === 'Escape') {
          event.preventDefault();
          setDraftValue(storedValue);
        }
      }}
    />
  );
}

function NumberStepper({ label, value, min, max, disabled = false, onChange }) {
  const hasMinimum = min !== undefined && min !== null && Number.isFinite(Number(min));
  const hasMaximum = max !== undefined && max !== null && Number.isFinite(Number(max));

  return (
    <input
      className="rune-number-input"
      type="number"
      inputMode="numeric"
      autoComplete="off"
      aria-label={label}
      step={1}
      value={value}
      min={hasMinimum ? min : undefined}
      max={hasMaximum ? max : undefined}
      disabled={disabled}
      onChange={(event) => onChange(event.target.value)}
    />
  );
}

function Toggle({ label, checked, onChange, disabled = false }) {
  return (
    <label className={`toggle ${disabled ? 'disabled' : ''}`}>
      <input
        type="checkbox"
        checked={checked}
        disabled={disabled}
        onChange={(event) => onChange(event.target.checked)}
      />
      <span className="toggle-track"><span /></span>
      <strong>{label}</strong>
    </label>
  );
}

function PanelHeading({ title, description }) {
  return (
    <div className="panel-heading">
      <div>
        <h2>{title}</h2>
        <p>{description}</p>
      </div>
    </div>
  );
}

function SummaryRow({ label, value }) {
  return <div><dt>{label}</dt><dd>{value}</dd></div>;
}

function clampInteger(raw, minimum, maximum) {
  const parsed = Number.parseInt(raw, 10);
  if (!Number.isFinite(parsed)) return minimum;
  return Math.min(maximum, Math.max(minimum, parsed));
}

function formatNumber(value) {
  return new Intl.NumberFormat('en-US').format(value);
}

function formatBytes(value) {
  return value < 1024 ? `${value} B` : `${(value / 1024).toFixed(1)} KB`;
}

function formatFileSize(value) {
  if (value < 1024) return `${value} B`;
  if (value < 1024 * 1024) return `${(value / 1024).toFixed(1)} KB`;
  return `${(value / 1024 / 1024).toFixed(1)} MB`;
}

function chronicleDiscoveryCount(editable) {
  if (!editable) return 0;
  return ['setItems', 'uniqueItems', 'runewords'].reduce(
    (total, category) => total + (editable[category]?.length || 0),
    0,
  );
}

function toDatetimeLocal(seconds) {
  const date = new Date(Number(seconds) * 1000);
  if (Number.isNaN(date.getTime())) return '';
  const local = new Date(date.getTime() - date.getTimezoneOffset() * 60000);
  return local.toISOString().slice(0, 16);
}

function fromDatetimeLocal(value) {
  const milliseconds = new Date(value).getTime();
  if (!value || Number.isNaN(milliseconds)) throw new Error('Chronicle discovery date is invalid.');
  return Math.floor(milliseconds / 60000) * 60;
}

function safeDownloadName(value) {
  return String(value).replace(/[^A-Za-z0-9_-]+/g, '-').replace(/^-+|-+$/g, '') || 'BKVince-item';
}

function downloadLocalBytes(bytes, fileName, type = 'application/octet-stream') {
  const blob = new Blob([bytes], { type });
  const url = URL.createObjectURL(blob);
  const anchor = window.document.createElement('a');
  anchor.href = url;
  anchor.download = fileName;
  anchor.click();
  window.setTimeout(() => URL.revokeObjectURL(url), 1000);
}

function UploadIcon() { return <IconPath path="M12 16V4m0 0L7 9m5-5 5 5M5 15v4h14v-4" />; }
function FolderOpenIcon() { return <IconPath path="M3 18V6a2 2 0 0 1 2-2h4l2 3h8a2 2 0 0 1 2 2v2M3 18l2-6h16l-2 6H3z" />; }
function DownloadIcon() { return <IconPath path="M12 4v12m0 0 5-5m-5 5-5-5M5 20h14" />; }
function PlusIcon() { return <IconPath path="M12 5v14M5 12h14" />; }
function SearchIcon() { return <IconPath path="M10.5 18a7.5 7.5 0 1 1 0-15 7.5 7.5 0 0 1 0 15zm5.5-2 5 5" />; }
function EditIcon() { return <IconPath path="M4 20l4-1L19 8l-3-3L5 16l-1 4zm10-13l3 3" />; }
function TrashIcon() { return <IconPath path="M5 7h14M9 7V4h6v3m-8 0 1 13h8l1-13M10 10v7m4-7v7" />; }
function UndoIcon() { return <IconPath path="M9 7H5v-4M5 7c2-3 5-4 8-3 4 1 6 5 5 9-1 4-5 6-9 5" />; }
function RedoIcon() { return <IconPath path="M15 7h4v-4m0 4c-2-3-5-4-8-3-4 1-6 5-5 9 1 4 5 6 9 5" />; }
function GoldIcon() { return <IconPath path="M5 7c0-2 3-3 7-3s7 1 7 3-3 3-7 3-7-1-7-3zm0 0v4c0 2 3 3 7 3s7-1 7-3V7m-14 4v4c0 2 3 3 7 3s7-1 7-3v-4" />; }
function ShieldIcon() { return <IconPath path="M12 3l7 3v5c0 5-3 8-7 10-4-2-7-5-7-10V6l7-3zm-3 9l2 2 4-5" />; }
function CheckIcon() { return <IconPath path="M5 12l4 4L19 6" />; }
const navigationIconPaths = Object.freeze({
  general: 'M7 4h10v16H7zM9 7h6M9 11h6M9 15h4',
  quests: 'M6 4h9l3 3v13H6zM15 4v4h4M9 11h6M9 15h5',
  waypoints: 'M12 3l3 6 6 3-6 3-3 6-3-6-6-3 6-3zM12 9v6M9 12h6',
  'item-bonuses': 'M5 19L16 8m-3-3 6 6M4 20l4-1-3-3-1 4zM14 4l6 6',
  skills: 'M5 6h4v4H5zM15 6h4v4h-4zM10 15h4v4h-4zM9 8h6M7 10v3h5v2M17 10v3h-5',
  chronicle: 'M5 4h6c2 0 3 1 3 3v13c0-2-1-3-3-3H5zM19 4h-5v16c0-2 1-3 3-3h2z',
  mercenary: 'M12 3l7 4v5c0 4-3 7-7 9-4-2-7-5-7-9V7zM8 11h8M10 8h4',
  demon: 'M8 5L5 3l1 5c-1 2-1 5 0 8l3 4h6l3-4c1-3 1-6 0-8l1-5-3 2M9 12h.01M15 12h.01M10 16h4',
});

function NavigationIcon({ type }) {
  return (
    <span className="rune-nav-icon" aria-hidden="true">
      <IconPath path={navigationIconPaths[type] || navigationIconPaths.general} />
    </span>
  );
}

function IconPath({ path }) {
  return (
    <svg viewBox="0 0 24 24" aria-hidden="true" focusable="false">
      <path d={path} fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  );
}
