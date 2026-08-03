import { useMemo, useReducer, useRef, useState } from 'react';

import {
  attributeFields,
  createBlankCharacter,
  editableSnapshot,
  exportCharacter,
  openCharacter,
  snapshotsEqual,
  suggestedFileName,
  supportedClasses,
} from './lib/character-codec.js';
import { createHistory, historyReducer } from './lib/history.js';

const navigation = [
  { id: 'general', label: 'General', enabled: true },
  { id: 'stats', label: 'Stats', enabled: true },
  { id: 'equipment', label: 'Equipment' },
  { id: 'inventory', label: 'Inventory' },
  { id: 'skills', label: 'Skills' },
  { id: 'quests', label: 'Quests' },
  { id: 'waypoints', label: 'Waypoints' },
  { id: 'mercenary', label: 'Mercenary' },
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
                  <h1>{activeTab === 'general' ? 'General' : 'Stats'}</h1>
                  <p>
                    {activeTab === 'general'
                      ? 'Edit identity, save seed, and character state flags.'
                      : 'Edit stored attributes within the limits declared by BKVince ItemStatCost.'}
                  </p>
                </div>
                <span className={`dirty-badge ${isDirty ? 'changed' : ''}`}>
                  {isDirty ? 'Unsaved changes' : 'Source preserved'}
                </span>
              </div>

              {activeTab === 'general' ? (
                <GeneralEditor editable={editable} edit={edit} />
              ) : (
                <StatsEditor editable={editable} edit={edit} />
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
            <p className="eyebrow">Slice 1</p>
            <h2>Safe foundation</h2>
            <ul>
              <li className="done">Open or create</li>
              <li className="done">General & stats</li>
              <li className="done">Undo & redo</li>
              <li className="done">Validated download</li>
              <li>Runtime proof pending</li>
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
