import { useMemo, useReducer, useRef, useState } from 'react';

import {
  attributeFields,
  containerForPlacement,
  createBlankCharacter,
  describeItem,
  editableSnapshot,
  exportCharacter,
  itemContainers,
  moveItemPlacement,
  openCharacter,
  snapshotsEqual,
  suggestedFileName,
  supportedClasses,
} from './lib/character-codec.js';
import { createHistory, historyReducer } from './lib/history.js';

const navigation = [
  { id: 'general', label: 'General', enabled: true },
  { id: 'stats', label: 'Stats', enabled: true },
  { id: 'equipment', label: 'Equipment', enabled: true },
  { id: 'inventory', label: 'Inventory', enabled: true },
  { id: 'skills', label: 'Skills' },
  { id: 'quests', label: 'Quests' },
  { id: 'waypoints', label: 'Waypoints' },
  { id: 'mercenary', label: 'Mercenary' },
];

const pageContent = {
  general: {
    title: 'General',
    description: 'Edit identity, save seed, and character state flags.',
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

const equipmentSlots = [
  [1, 'Head'],
  [2, 'Neck'],
  [3, 'Torso'],
  [4, 'Right hand'],
  [5, 'Left hand'],
  [6, 'Right ring'],
  [7, 'Left ring'],
  [8, 'Belt'],
  [9, 'Feet'],
  [10, 'Gloves'],
  [11, 'Right swap'],
  [12, 'Left swap'],
];

const statGroups = [
  {
    title: 'Core attributes',
    description: 'Base values and points waiting to be assigned.',
    keys: ['strength', 'energy', 'dexterity', 'vitality', 'unused_stats', 'unused_skill_points'],
  },
  {
    title: 'Resources',
    description: 'Current and maximum life, mana, and stamina.',
    keys: ['current_hp', 'max_hp', 'current_mana', 'max_mana', 'current_stamina', 'max_stamina'],
  },
  {
    title: 'Progression & gold',
    description: 'Level, experience, and carried or stashed gold.',
    keys: ['level', 'experience', 'gold', 'stashed_gold'],
  },
];

export default function App() {
  const classes = useMemo(() => supportedClasses(), []);
  const fileInput = useRef(null);
  const [document, setDocument] = useState(null);
  const [history, dispatch] = useReducer(historyReducer, createHistory(null));
  const [activeTab, setActiveTab] = useState('general');
  const [selectedItemIndex, setSelectedItemIndex] = useState(null);
  const [createOpen, setCreateOpen] = useState(false);
  const [draft, setDraft] = useState({
    name: 'NewHero',
    className: classes[0]?.name ?? 'Amazon',
    hardcore: false,
    ladder: false,
  });
  const [busy, setBusy] = useState(false);
  const [notice, setNotice] = useState({ tone: 'neutral', text: 'No save is loaded.' });

  const editable = history.present;
  const isDirty = Boolean(document && editable && !snapshotsEqual(document.initial, editable));

  async function loadFile(file) {
    if (!file) return;
    setBusy(true);
    try {
      const nextDocument = await openCharacter(await file.arrayBuffer(), file.name);
      setDocument(nextDocument);
      dispatch({ type: 'replace', value: editableSnapshot(nextDocument.model) });
      setActiveTab('general');
      setSelectedItemIndex(null);
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

  async function createHero(event) {
    event.preventDefault();
    setBusy(true);
    try {
      const nextDocument = await createBlankCharacter(draft);
      setDocument(nextDocument);
      dispatch({ type: 'replace', value: editableSnapshot(nextDocument.model) });
      setActiveTab('general');
      setSelectedItemIndex(null);
      setCreateOpen(false);
      setNotice({
        tone: 'success',
        text: `${draft.name} created as a blank ${draft.className}. No preset or automatic build was applied.`,
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
      const blob = new Blob([result.bytes], { type: 'application/octet-stream' });
      const url = URL.createObjectURL(blob);
      const anchor = window.document.createElement('a');
      anchor.href = url;
      anchor.download = suggestedFileName(editable);
      anchor.click();
      window.setTimeout(() => URL.revokeObjectURL(url), 1000);
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

  function edit(update) {
    dispatch({ type: 'edit', update });
  }

  function selectItem(index) {
    setSelectedItemIndex((current) => (current === index ? null : index));
  }

  function moveSelectedItem(containerId, x, y) {
    if (!document || !editable) return;
    if (selectedItemIndex === null) {
      setNotice({ tone: 'neutral', text: 'Select an existing item before choosing its destination cell.' });
      return;
    }
    try {
      const nextPlacements = moveItemPlacement(
        editable.itemPlacements,
        document.model.items,
        selectedItemIndex,
        containerId,
        x,
        y,
      );
      edit((current) => ({ ...current, itemPlacements: nextPlacements }));
      const item = describeItem(document.model.items[selectedItemIndex], selectedItemIndex);
      const container = itemContainers[containerId];
      setNotice({
        tone: 'success',
        text: `${item.name} moved to ${container.label} ${x + 1},${y + 1}. Export will verify the placement by reparsing.`,
      });
      setSelectedItemIndex(null);
    } catch (error) {
      setNotice({ tone: 'error', text: error.message });
    }
  }

  return (
    <div className="app-shell">
      <header className="topbar">
        <div className="brand">
          <div className="brand-mark" aria-hidden="true">BK</div>
          <div>
            <strong>BKVince</strong>
            <span>Hero Editor</span>
          </div>
        </div>
        <div className="top-actions">
          <input
            ref={fileInput}
            className="visually-hidden"
            type="file"
            accept=".d2s,application/octet-stream"
            onChange={(event) => loadFile(event.target.files?.[0])}
          />
          <button className="button ghost" type="button" onClick={() => fileInput.current?.click()} disabled={busy}>
            <UploadIcon /> Open D2S
          </button>
          <button className="button ghost" type="button" onClick={() => setCreateOpen(true)} disabled={busy}>
            <PlusIcon /> Create blank hero
          </button>
          <div className="history-actions" aria-label="Edit history">
            <button
              className="icon-button"
              type="button"
              aria-label="Undo"
              title="Undo"
              disabled={busy || history.past.length === 0}
              onClick={() => dispatch({ type: 'undo' })}
            >
              <UndoIcon />
            </button>
            <button
              className="icon-button"
              type="button"
              aria-label="Redo"
              title="Redo"
              disabled={busy || history.future.length === 0}
              onClick={() => dispatch({ type: 'redo' })}
            >
              <RedoIcon />
            </button>
          </div>
          <button className="button primary" type="button" onClick={downloadCopy} disabled={busy || !document}>
            <DownloadIcon /> Download copy
          </button>
        </div>
      </header>

      <div className="workspace">
        <aside className="sidebar">
          <CharacterCard document={document} editable={editable} dirty={isDirty} />
          <nav className="section-nav" aria-label="Character editor sections">
            <p className="eyebrow">Character</p>
            {navigation.map((entry) => (
              <button
                key={entry.id}
                type="button"
                className={`nav-item ${activeTab === entry.id ? 'active' : ''}`}
                disabled={!entry.enabled || !document}
                onClick={() => setActiveTab(entry.id)}
              >
                <span>{entry.label}</span>
                {!entry.enabled && <small>Later</small>}
              </button>
            ))}
          </nav>
          <div className="privacy-note">
            <ShieldIcon />
            <div>
              <strong>Local by design</strong>
              <span>Your save never leaves this browser.</span>
            </div>
          </div>
        </aside>

        <main className="main-panel">
          <div className={`notice ${notice.tone}`} role="status">
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
              <div className="page-heading">
                <div>
                  <p className="eyebrow">{editable.className} · D2R save v105</p>
                  <h1>{pageContent[activeTab].title}</h1>
                  <p>{pageContent[activeTab].description}</p>
                </div>
                <span className={`dirty-badge ${isDirty ? 'changed' : ''}`}>
                  {isDirty ? 'Unsaved changes' : 'Source preserved'}
                </span>
              </div>

              {activeTab === 'general' && <GeneralEditor editable={editable} edit={edit} />}
              {activeTab === 'stats' && <StatsEditor editable={editable} edit={edit} />}
              {activeTab === 'equipment' && (
                <EquipmentEditor
                  items={document.model.items}
                  placements={editable.itemPlacements}
                  selectedItemIndex={selectedItemIndex}
                  onSelectItem={selectItem}
                  onOpenInventory={() => setActiveTab('inventory')}
                />
              )}
              {activeTab === 'inventory' && (
                <InventoryEditor
                  items={document.model.items}
                  placements={editable.itemPlacements}
                  selectedItemIndex={selectedItemIndex}
                  onSelectItem={selectItem}
                  onMoveItem={moveSelectedItem}
                />
              )}
            </>
          )}
        </main>

        <aside className="inspector">
          <p className="eyebrow">Save summary</p>
          <dl className="summary-list">
            <SummaryRow label="Status" value={document ? (isDirty ? 'Modified' : 'Original') : 'Empty'} />
            <SummaryRow label="Class data" value={`${classes.length} encodable`} />
            <SummaryRow label="Format" value={document ? `v${document.model.header.version}` : '—'} />
            <SummaryRow label="Source" value={document ? formatBytes(document.sourceBytes.length) : '—'} />
            <SummaryRow label="Items" value={document ? String(document.model.items.length) : '—'} />
            <SummaryRow label="Skills" value={document ? String(document.model.skills.length) : '—'} />
          </dl>
          <div className="gate-card">
            <p className="eyebrow">Slice 2</p>
            <h2>Container editing</h2>
            <ul>
              <li className="done">Runtime-proven base</li>
              <li className="done">Real BKVince grids</li>
              <li className="done">Bounds & overlap gates</li>
              <li>Item runtime proof next</li>
            </ul>
          </div>
        </aside>
      </div>

      {createOpen && (
        <div className="modal-backdrop" role="presentation" onMouseDown={() => setCreateOpen(false)}>
          <section
            className="modal"
            role="dialog"
            aria-modal="true"
            aria-labelledby="create-title"
            onMouseDown={(event) => event.stopPropagation()}
          >
            <div className="modal-heading">
              <div>
                <p className="eyebrow">New save</p>
                <h2 id="create-title">Create a blank hero</h2>
              </div>
              <button className="icon-button" type="button" aria-label="Close" onClick={() => setCreateOpen(false)}>×</button>
            </div>
            <p className="modal-copy">
              Choose only the identity. Equipment, stats, and skills are never filled from a preset build.
            </p>
            <form onSubmit={createHero}>
              <label className="field full">
                <span>Character name</span>
                <input
                  autoFocus
                  value={draft.name}
                  maxLength={15}
                  onChange={(event) => setDraft({ ...draft, name: event.target.value })}
                />
                <small>2–15 letters, hyphens, or underscores.</small>
              </label>
              <label className="field full">
                <span>Class</span>
                <select
                  value={draft.className}
                  onChange={(event) => setDraft({ ...draft, className: event.target.value })}
                >
                  {classes.map(({ name }) => <option key={name}>{name}</option>)}
                </select>
              </label>
              <div className="toggle-grid modal-toggles">
                <Toggle
                  label="Hardcore"
                  checked={draft.hardcore}
                  onChange={(checked) => setDraft({ ...draft, hardcore: checked })}
                />
                <Toggle
                  label="Ladder"
                  checked={draft.ladder}
                  onChange={(checked) => setDraft({ ...draft, ladder: checked })}
                />
              </div>
              <div className="modal-actions">
                <button className="button ghost" type="button" onClick={() => setCreateOpen(false)}>Cancel</button>
                <button className="button primary" type="submit" disabled={busy}>Create blank hero</button>
              </div>
            </form>
          </section>
        </div>
      )}
    </div>
  );
}

function CharacterCard({ document, editable, dirty }) {
  return (
    <div className="character-card">
      <div className="portrait" aria-hidden="true">
        {editable?.className?.slice(0, 2).toUpperCase() ?? '—'}
      </div>
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
  return (
    <div className="editor-grid">
      <section className="panel span-7">
        <PanelHeading title="Identity" description="Stored character identity and world seed." />
        <div className="fields-grid">
          <label className="field">
            <span>Character name</span>
            <input
              value={editable.name}
              maxLength={15}
              onChange={(event) => edit((current) => ({ ...current, name: event.target.value }))}
            />
          </label>
          <label className="field">
            <span>Class</span>
            <input value={editable.className} disabled />
            <small>Class conversion is intentionally locked in this slice.</small>
          </label>
          <NumberField
            label="Map seed"
            value={editable.mapId}
            min={0}
            max={0xffffffff}
            onChange={(value) => edit((current) => ({ ...current, mapId: value }))}
          />
          <NumberField
            label="Level"
            value={editable.attributes.level}
            min={1}
            max={99}
            onChange={(value) => edit((current) => ({
              ...current,
              attributes: { ...current.attributes, level: value },
            }))}
          />
        </div>
      </section>
      <section className="panel span-5">
        <PanelHeading title="Character state" description="Flags stored in the save header." />
        <div className="toggle-grid">
          {[
            ['expansion', 'Expansion'],
            ['hardcore', 'Hardcore'],
            ['ladder', 'Ladder'],
            ['died', 'Dead'],
          ].map(([key, label]) => (
            <Toggle
              key={key}
              label={label}
              checked={editable[key]}
              onChange={(checked) => edit((current) => ({ ...current, [key]: checked }))}
            />
          ))}
        </div>
      </section>
      <section className="panel span-12 preservation-panel">
        <div className="preservation-icon"><ShieldIcon /></div>
        <div>
          <h2>Source-preserving export</h2>
          <p>
            An unchanged save downloads from its original bytes. A modified save is written to a new buffer, then its size,
            checksum, and edited values are verified by reparsing before download.
          </p>
        </div>
      </section>
    </div>
  );
}

function StatsEditor({ editable, edit }) {
  const fieldsByKey = Object.fromEntries(attributeFields.map((field) => [field.key, field]));
  return (
    <div className="stats-stack">
      {statGroups.map((group) => (
        <section className="panel" key={group.title}>
          <PanelHeading title={group.title} description={group.description} />
          <div className="stats-grid">
            {group.keys.map((key) => {
              const field = fieldsByKey[key];
              return (
                <NumberField
                  key={key}
                  label={field.label}
                  value={editable.attributes[key]}
                  min={key === 'level' ? 1 : 0}
                  max={key === 'level' ? 99 : field.maximum}
                  onChange={(value) => edit((current) => ({
                    ...current,
                    attributes: { ...current.attributes, [key]: value },
                  }))}
                />
              );
            })}
          </div>
        </section>
      ))}
    </div>
  );
}

function InventoryEditor({
  items,
  placements,
  selectedItemIndex,
  onSelectItem,
  onMoveItem,
}) {
  return (
    <div className="item-workspace">
      <ItemSelectionBar
        items={items}
        selectedItemIndex={selectedItemIndex}
        emptyText="Select an item in any grid, then choose a free destination cell."
      />
      {Object.values(itemContainers).map((container) => (
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
}) {
  const equipped = placements.filter((placement) => containerForPlacement(placement) === 'equipment');
  const belt = placements.filter((placement) => containerForPlacement(placement) === 'belt');
  const knownSlots = new Set(equipmentSlots.map(([slot]) => slot));
  const otherEquipped = equipped.filter((placement) => !knownSlots.has(placement.equippedId));

  return (
    <div className="item-workspace">
      <ItemSelectionBar
        items={items}
        selectedItemIndex={selectedItemIndex}
        emptyText="Select equipped or belted gear to prepare a safe move into a stored container."
        action={selectedItemIndex !== null ? (
          <button className="button ghost compact" type="button" onClick={onOpenInventory}>
            Choose destination
          </button>
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
          description="D2S stores belt slots as one flattened 0–15 index. The 4×4 view is accurate; placement remains read-only until the equipped belt capacity is derived."
        />
        <DisplayGrid
          columns={4}
          rows={4}
          items={items}
          placements={belt}
          selectedItemIndex={selectedItemIndex}
          onSelectItem={onSelectItem}
          coordinateFor={(placement) => ({
            x: placement.x % 4,
            y: Math.floor(placement.x / 4),
          })}
          emptyLabel="Empty belt"
        />
      </section>
    </div>
  );
}

function ContainerGrid({
  container,
  items,
  placements,
  selectedItemIndex,
  onSelectItem,
  onMoveItem,
}) {
  const visible = placements.filter((placement) => containerForPlacement(placement) === container.id);
  const cells = Array.from({ length: container.width * container.height }, (_, index) => ({
    x: index % container.width,
    y: Math.floor(index / container.width),
  }));
  return (
    <section className="panel container-panel">
      <PanelHeading
        title={container.label}
        description={`${container.width}×${container.height} cells from BKVince inventory.txt · ${visible.length} stored item${visible.length === 1 ? '' : 's'}`}
      />
      <div className="item-grid-scroll">
        <div
          className="item-grid"
          style={{ '--grid-columns': container.width, '--grid-rows': container.height }}
          aria-label={`${container.label} grid`}
        >
          {cells.map(({ x, y }) => (
            <button
              className="grid-cell"
              type="button"
              key={`${x}-${y}`}
              style={{ gridColumn: x + 1, gridRow: y + 1 }}
              aria-label={`Place selected item at ${container.label} column ${x + 1}, row ${y + 1}`}
              onClick={() => onMoveItem(container.id, x, y)}
            />
          ))}
          {visible.map((placement) => (
            <GridItem
              key={placement.index}
              item={items[placement.index]}
              placement={placement}
              selected={selectedItemIndex === placement.index}
              onSelect={() => onSelectItem(placement.index)}
            />
          ))}
        </div>
      </div>
    </section>
  );
}

function DisplayGrid({
  columns,
  rows,
  items,
  placements,
  selectedItemIndex,
  onSelectItem,
  coordinateFor,
  emptyLabel,
}) {
  const cells = Array.from({ length: columns * rows });
  return (
    <div className="display-grid-wrap">
      <div
        className="item-grid display-only"
        style={{ '--grid-columns': columns, '--grid-rows': rows }}
        aria-label={emptyLabel}
      >
        {cells.map((_, index) => (
          <span
            className="grid-cell"
            key={index}
            style={{ gridColumn: (index % columns) + 1, gridRow: Math.floor(index / columns) + 1 }}
          />
        ))}
        {placements.map((placement) => {
          const coordinate = coordinateFor(placement);
          return (
            <GridItem
              key={placement.index}
              item={items[placement.index]}
              placement={{ ...placement, ...coordinate }}
              selected={selectedItemIndex === placement.index}
              onSelect={() => onSelectItem(placement.index)}
              forceUnitSize
            />
          );
        })}
      </div>
    </div>
  );
}

function GridItem({ item, placement, selected, onSelect, forceUnitSize = false }) {
  const descriptor = describeItem(item, placement.index);
  const width = forceUnitSize ? 1 : descriptor.width;
  const height = forceUnitSize ? 1 : descriptor.height;
  return (
    <button
      className={`grid-item ${selected ? 'selected' : ''}`}
      type="button"
      style={{
        gridColumn: `${placement.x + 1} / span ${width}`,
        gridRow: `${placement.y + 1} / span ${height}`,
      }}
      title={`${descriptor.name} · ${descriptor.type} · ${descriptor.width}×${descriptor.height}`}
      onClick={onSelect}
    >
      <strong>{descriptor.type.toUpperCase()}</strong>
      <span>{descriptor.name}</span>
    </button>
  );
}

function ItemToken({ item, placement, selected, onSelect }) {
  const descriptor = describeItem(item, placement.index);
  return (
    <button className={`item-token ${selected ? 'selected' : ''}`} type="button" onClick={onSelect}>
      <strong>{descriptor.type.toUpperCase()}</strong>
      <span>{descriptor.name}</span>
    </button>
  );
}

function ItemSelectionBar({ items, selectedItemIndex, emptyText, action = null }) {
  const item = selectedItemIndex === null ? null : describeItem(items[selectedItemIndex], selectedItemIndex);
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

function Toggle({ label, checked, onChange }) {
  return (
    <label className="toggle">
      <input type="checkbox" checked={checked} onChange={(event) => onChange(event.target.checked)} />
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

function UploadIcon() { return <IconPath path="M12 16V4m0 0L7 9m5-5 5 5M5 15v4h14v-4" />; }
function DownloadIcon() { return <IconPath path="M12 4v12m0 0 5-5m-5 5-5-5M5 20h14" />; }
function PlusIcon() { return <IconPath path="M12 5v14M5 12h14" />; }
function UndoIcon() { return <IconPath path="M9 7H5v-4M5 7c2-3 5-4 8-3 4 1 6 5 5 9-1 4-5 6-9 5" />; }
function RedoIcon() { return <IconPath path="M15 7h4v-4m0 4c-2-3-5-4-8-3-4 1-6 5-5 9 1 4 5 6 9 5" />; }
function ShieldIcon() { return <IconPath path="M12 3l7 3v5c0 5-3 8-7 10-4-2-7-5-7-10V6l7-3zm-3 9l2 2 4-5" />; }
function CheckIcon() { return <IconPath path="M5 12l4 4L19 6" />; }

function IconPath({ path }) {
  return (
    <svg viewBox="0 0 24 24" aria-hidden="true" focusable="false">
      <path d={path} fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  );
}
