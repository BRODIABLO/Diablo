import zlib from 'node:zlib';

export const SCHEMA_ORIENTATION_STYLE = `
.schema-orientation{display:grid;gap:22px}.schema-hero{padding:18px;border:1px solid var(--line,#344a61);border-radius:12px;background:linear-gradient(145deg,var(--panel-2,#172536),var(--panel,#111c28))}.schema-hero h2{margin:2px 0 6px;font-size:26px}.schema-hero p{max-width:1050px}.schema-kicker{margin:0;color:var(--gold,#f0bd6b);font-size:11px;letter-spacing:.12em;text-transform:uppercase}.schema-gate{display:flex;align-items:center;gap:12px;margin-top:12px;padding:10px;border-left:3px solid var(--gold,#f0bd6b);background:#302615}.schema-gate.closed{border-color:var(--green,#6ed49a);background:#173024}.schema-gate progress{width:180px}.schema-summary-grid{display:grid;grid-template-columns:repeat(3,minmax(150px,1fr));gap:9px}.schema-summary-card{padding:12px;border:1px solid var(--line,#344a61);border-radius:9px;background:var(--panel,#111c28)}.schema-summary-card strong{display:block;font-size:24px}.schema-summary-card span,.schema-muted{color:var(--muted,#9fb0c1)}.schema-section{scroll-margin-top:275px}.schema-section-heading{display:flex;justify-content:space-between;gap:12px;align-items:end;margin-bottom:10px}.schema-section-heading h3{margin:0;font-size:20px}.schema-section-heading p{margin:2px 0;color:var(--muted,#9fb0c1)}.schema-controls{display:flex;gap:8px;align-items:end;flex-wrap:wrap;padding:10px;border:1px solid var(--line,#344a61);border-radius:9px;background:var(--panel,#111c28);margin-bottom:10px}.schema-controls .schema-search{flex:1;min-width:240px}.schema-controls input,.schema-controls select{width:100%}.schema-matrix-wrap{overflow:auto;border:1px solid var(--line,#344a61);border-radius:10px}.schema-matrix{min-width:1020px}.schema-matrix th{position:sticky;top:0;z-index:1}.schema-matrix .schema-source-cell{text-align:center}.schema-present{color:var(--green,#6ed49a);font-weight:700}.schema-absent{color:var(--muted,#9fb0c1)}.schema-column-button{width:100%;text-align:left;border:0;background:none;padding:0}.schema-column-button strong,.schema-column-button small{display:block}.schema-column-button small{color:var(--muted,#9fb0c1)}.schema-detail-row td{padding:0}.schema-column-detail{display:grid;gap:11px;padding:13px;background:#0b1520}.schema-detail-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:8px}.schema-evidence-card{padding:9px;border:1px solid var(--line,#344a61);border-radius:7px;background:var(--panel,#111c28)}.schema-evidence-card h5{margin:0 0 6px}.schema-evidence-card dl{margin:0}.schema-evidence-card dt{color:var(--muted,#9fb0c1);font-size:11px}.schema-evidence-card dd{margin:0 0 5px;overflow-wrap:anywhere}.schema-raw-state{display:inline-block;padding:2px 7px;border:1px solid var(--line,#344a61);border-radius:99px;font:11px ui-monospace,monospace}.schema-raw-state.zero{border-color:#745d91;color:#d8c4ff}.schema-raw-state.value{border-color:#477a58;color:#8be5a8}.schema-raw-state.absent_column,.schema-raw-state.null_value,.schema-raw-state.empty_string{color:var(--muted,#9fb0c1)}.schema-legend{display:flex;gap:6px;flex-wrap:wrap;margin:7px 0}.schema-tags{display:flex;gap:5px;flex-wrap:wrap}.schema-tag{display:inline-block;padding:2px 7px;border:1px solid var(--line,#344a61);border-radius:99px;font-size:11px}.schema-tag.protected,.schema-tag.native{border-color:#8a524c;color:#ffafa7}.schema-tag.policy{border-color:#85693e;color:#ffd28c}.schema-source-only,.schema-contract-grid,.schema-policy-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:9px}.schema-source-only article,.schema-contract,.schema-policy{padding:12px;border:1px solid var(--line,#344a61);border-radius:9px;background:var(--panel,#111c28)}.schema-contract h4,.schema-policy h4{margin:0}.schema-contract summary{cursor:pointer}.schema-contract ul{padding-left:18px}.schema-contract .hypothesis{color:var(--gold,#f0bd6b)}.schema-dictionary{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:8px}.schema-dictionary article{padding:10px;border:1px solid var(--line,#344a61);border-radius:8px;background:var(--panel,#111c28)}.schema-dictionary h4{margin:0}.schema-dictionary code{color:var(--muted,#9fb0c1)}.schema-policy{display:grid;gap:8px}.schema-policy.required{border-left:3px solid var(--gold,#f0bd6b)}.schema-policy.complete{border-left-color:var(--green,#6ed49a)}.schema-policy label span{display:block;color:var(--muted,#9fb0c1);font-size:12px}.schema-policy textarea{min-height:62px}.schema-policy .schema-policy-custom{border-left:2px solid var(--gold,#f0bd6b);padding-left:8px}.schema-policy-toolbar{display:flex;gap:7px;flex-wrap:wrap;margin-bottom:10px}.schema-impact-flow{display:grid;grid-template-columns:repeat(4,minmax(120px,1fr));gap:9px}.schema-impact-step{padding:11px;border:1px solid var(--line,#344a61);border-radius:9px;background:var(--panel,#111c28)}.schema-impact-step strong{display:block;font-size:22px}.schema-impact-reductions{display:grid;gap:6px;margin-top:10px}.schema-impact-reductions li{padding:8px;border-left:3px solid var(--blue,#72c8ff);background:var(--panel,#111c28)}.schema-questions{display:grid;gap:7px;padding-left:0;list-style:none}.schema-questions li{padding:10px;border-left:3px solid var(--red,#ff8d88);background:var(--panel,#111c28)}.schema-empty{padding:14px;border:1px dashed var(--line,#344a61);border-radius:8px;color:var(--muted,#9fb0c1)}.schema-toolbar-note{font:11px ui-monospace,monospace;color:var(--muted,#9fb0c1)}@media(max-width:1000px){.schema-summary-grid,.schema-detail-grid,.schema-dictionary{grid-template-columns:repeat(2,minmax(0,1fr))}.schema-source-only,.schema-contract-grid,.schema-policy-grid{grid-template-columns:1fr}.schema-impact-flow{grid-template-columns:repeat(2,1fr)}}@media(max-width:650px){.schema-summary-grid,.schema-detail-grid,.schema-dictionary,.schema-impact-flow{grid-template-columns:1fr}.schema-section-heading{display:block}}
`;

function protectInlineSource(source) {
  return String(source)
    .replace(/<\/script/gi, '<\\/script')
    .replace(/<script/gi, '<\\x73cript');
}

function schemaOrientationBrowserApplication() {
  'use strict';

  const SOURCE_KEYS = ['vanilla32', 'bkvince', 'pd2'];
  const SOURCE_LABELS = {
    vanilla32: 'Vanilla D2R 3.2',
    bkvince: 'BKVince HEAD',
    pd2: 'PD2 / SP+ épinglé',
  };
  const POLICY_DECISIONS = ['PENDING', 'APPROVE', 'MODIFY', 'REJECT', 'DISCUSS'];

  function escapeHtml(value) {
    return String(value ?? '').replace(/[&<>"']/g, (character) => ({
      '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
    })[character]);
  }

  function escapeAttribute(value) {
    return escapeHtml(value).replace(/`/g, '&#96;');
  }

  function asArray(value) {
    if (Array.isArray(value)) return value;
    if (value === null || value === undefined || value === '') return [];
    return [value];
  }

  function objectEntries(value) {
    return value && typeof value === 'object' && !Array.isArray(value) ? Object.entries(value) : [];
  }

  function firstDefined(...values) {
    return values.find((value) => value !== undefined && value !== null);
  }

  function numberFrom(value, keys, fallback = 0) {
    for (const key of keys) {
      const candidate = key.split('.').reduce((current, part) => current?.[part], value);
      if (Number.isFinite(Number(candidate))) return Number(candidate);
    }
    return fallback;
  }

  function friendly(value) {
    return String(value ?? '—').replaceAll('_', ' ').toLowerCase().replace(/^./, (letter) => letter.toUpperCase());
  }

  function display(value) {
    if (value === undefined) return 'absent';
    if (value === null) return 'null';
    if (value === '') return 'vide';
    if (typeof value === 'object') return JSON.stringify(value);
    return String(value);
  }

  function present(column, source) {
    const value = column?.presence?.[source];
    if (typeof value === 'boolean') return value;
    if (value && typeof value === 'object') return value.present !== false;
    const raw = column?.rawHeaders?.[source];
    return raw !== undefined && raw !== null && asArray(raw).length > 0;
  }

  function usageFor(column, source) {
    const sourceUsage = column?.usage?.bySource?.[source] || column?.usage?.[source] || {};
    return {
      nonEmpty: numberFrom(sourceUsage, ['nonBlankCells', 'nonEmptyCells', 'totalNonEmpty', 'usedRows'], 0),
      players: numberFrom(sourceUsage, ['playerSkills', 'playerRows', 'playerSkillCount'], 0),
      technical: numberFrom(sourceUsage, ['technicalOrMonsterRows', 'technicalOrMonsterLines', 'technicalMonsterRows', 'technicalRows', 'monsterOrTechnical'], 0),
    };
  }

  function totalUsage(column) {
    const summed = SOURCE_KEYS.reduce((totals, source) => {
      const usage = usageFor(column, source);
      totals.nonEmpty += usage.nonEmpty;
      totals.players += usage.players;
      totals.technical += usage.technical;
      return totals;
    }, { nonEmpty: 0, players: 0, technical: 0 });
    return {
      nonEmpty: numberFrom(column?.usage || {}, ['totalNonBlankCells', 'totalNonEmptyCells', 'nonEmptyCells'], summed.nonEmpty),
      players: numberFrom(column?.usage || {}, ['playerSkills', 'playerSkillCount'], summed.players),
      technical: numberFrom(column?.usage || {}, ['technicalOrMonsterRows', 'technicalOrMonsterLines', 'technicalMonsterRows'], summed.technical),
    };
  }

  function classificationsOf(column) {
    const values = [...asArray(column?.classifications), ...asArray(column?.primaryClassification)].filter(Boolean);
    return [...new Set(values)];
  }

  function rawHeaderFor(column, source) {
    const value = column?.rawHeaders?.[source];
    if (!present(column, source)) return 'ABSENT_COLUMN';
    return asArray(value).map(display).join(', ') || column.canonicalHeader || column.id || '—';
  }

  function examplesFor(column, source) {
    const examples = asArray(column?.examples?.[source] ?? column?.usage?.[source]?.examples);
    return examples.slice(0, 5);
  }

  function normalizeEvidence(example, source, column) {
    if (example && typeof example === 'object') {
      const rawValue = firstDefined(example.rawValue, example.value, example.cellValue);
      const rawState = example.rawState || (present(column, source) ? (rawValue === null ? 'NULL_VALUE' : rawValue === '' ? 'EMPTY_STRING' : String(rawValue).trim() === '0' ? 'ZERO' : 'VALUE') : 'ABSENT_COLUMN');
      return {
        label: example.skill || example.name || example.rowName || example.row || example.ordinal || 'exemple',
        rawValue,
        rawState,
        semanticBlank: example.semanticBlank === true || ['ABSENT_COLUMN', 'NULL_VALUE', 'EMPTY_STRING'].includes(rawState),
      };
    }
    const rawState = !present(column, source) ? 'ABSENT_COLUMN' : example === null ? 'NULL_VALUE' : example === '' ? 'EMPTY_STRING' : String(example).trim() === '0' ? 'ZERO' : 'VALUE';
    return { label: 'exemple', rawValue: example, rawState, semanticBlank: ['ABSENT_COLUMN', 'NULL_VALUE', 'EMPTY_STRING'].includes(rawState) };
  }

  function policyDecisionComplete(policy, value) {
    const decision = value?.decision || 'PENDING';
    const justified = Boolean(String(value?.justification || '').trim());
    if (decision === 'APPROVE') return justified;
    if (decision === 'MODIFY') return justified && Boolean(String(value?.customPolicy || '').trim());
    return false;
  }

  function createController(orientation, options = {}) {
    if (!orientation || typeof orientation !== 'object' || Array.isArray(orientation)) throw new TypeError('orientation must be an oracle object');
    const columns = asArray(orientation.columns);
    const policies = asArray(orientation.policies);
    const contracts = asArray(orientation.mechanicalContracts);
    const runtime = options.policyRuntime || globalThis.schemaPolicyRuntime || null;
    const storage = options.storage || globalThis.localStorage;
    const storageKeyFunction = runtime?.storageKey || runtime?.policyStorageKey;
    const storageKey = typeof storageKeyFunction === 'function'
      ? storageKeyFunction.call(runtime, orientation)
      : orientation.policyStorageKey || null;
    const state = {
      query: '', source: '', usage: '', classification: '', decisionScope: '', proof: '',
      expandedColumns: new Set(), expandedContracts: new Set(), saveTimer: null,
    };
    let envelope = loadEnvelope();

    function emptyEnvelope() {
      const createEnvelope = runtime?.createEmptyEnvelope || runtime?.createPolicyEnvelope;
      if (typeof createEnvelope === 'function') return createEnvelope.call(runtime, orientation);
      return {
        schemaVersion: orientation.policySchemaVersion,
        kind: orientation.policyKind,
        orientationId: orientation.orientationId,
        orientationHash: orientation.orientationHash,
        frozenContractHash: orientation.frozenContractHash,
        sourceHashes: orientation.sourceHashes || {},
        exportedAt: null,
        decisions: {},
      };
    }

    function validateEnvelope(candidate) {
      const validate = runtime?.validateImport || runtime?.validatePolicyEnvelope;
      if (typeof validate === 'function') {
        const result = validate.call(runtime, orientation, candidate);
        if (result?.valid === false) throw new Error(asArray(result.errors).join('; ') || 'Fichier de politiques invalide');
        return result?.envelope || result?.value || result || candidate;
      }
      if (!candidate || typeof candidate !== 'object' || Array.isArray(candidate)) throw new Error('Fichier de politiques invalide');
      if (candidate.orientationId !== orientation.orientationId) throw new Error('orientationId incompatible');
      if (candidate.orientationHash !== orientation.orientationHash) throw new Error('orientationHash périmé');
      if (candidate.frozenContractHash !== orientation.frozenContractHash) throw new Error('contrat gelé incompatible');
      if (!candidate.decisions || typeof candidate.decisions !== 'object' || Array.isArray(candidate.decisions)) throw new Error('decisions manquantes');
      for (const policy of policies) {
        const decision = candidate.decisions[policy.id];
        if (decision && decision.fingerprint !== policy.fingerprint) throw new Error('fingerprint de politique périmé : ' + policy.id);
      }
      return candidate;
    }

    function loadEnvelope() {
      const empty = emptyEnvelope();
      const candidates = [];
      if (options.initialEnvelope) {
        try {
          candidates.push({ source: 'initial', envelope: validateEnvelope(options.initialEnvelope) });
        } catch (error) {
          options.notify?.('Politiques du Workbench ignorées : ' + error.message, 'warning');
        }
      }
      if (storageKey) {
        try {
          const raw = storage?.getItem?.(storageKey);
          if (raw) candidates.push({ source: 'local', envelope: validateEnvelope(JSON.parse(raw)) });
        } catch (error) {
          options.notify?.('Politiques locales ignorées : ' + error.message, 'warning');
        }
      }
      if (!candidates.length) return empty;
      if (candidates.length === 1) return candidates[0].envelope;
      const initial = candidates.find((candidate) => candidate.source === 'initial');
      const local = candidates.find((candidate) => candidate.source === 'local');
      if (!initial || !local) return candidates[0].envelope;
      const initialClosed = closedPolicyCount(initial.envelope);
      const localClosed = closedPolicyCount(local.envelope);
      if (initialClosed === 0 && localClosed > 0 && allPoliciesPending(initial.envelope)) return local.envelope;
      const initialTime = exportedAtTime(initial.envelope);
      const localTime = exportedAtTime(local.envelope);
      if (localTime > initialTime) return local.envelope;
      if (localTime === initialTime && localClosed > initialClosed) return local.envelope;
      return initial.envelope;
    }

    function closedPolicyCount(candidate) {
      return policies.filter((policy) => policy.requiredForSkillCompletion !== false
        && policyDecisionComplete(policy, candidate?.decisions?.[policy.id])).length;
    }

    function allPoliciesPending(candidate) {
      return policies.every((policy) => (candidate?.decisions?.[policy.id]?.decision || 'PENDING') === 'PENDING');
    }

    function exportedAtTime(candidate) {
      const timestamp = Date.parse(candidate?.exportedAt || '');
      return Number.isFinite(timestamp) ? timestamp : 0;
    }

    function decisionFor(policy) {
      return envelope.decisions?.[policy.id] || { fingerprint: policy.fingerprint, decision: 'PENDING', justification: '', customPolicy: '' };
    }

    function updateDecision(policyId, patch) {
      const policy = policies.find((item) => item.id === policyId);
      if (!policy) return;
      if (typeof runtime?.updateDecision === 'function') {
        envelope = runtime.updateDecision(orientation, envelope, policyId, patch) || envelope;
      } else {
        envelope.decisions ||= {};
        envelope.decisions[policyId] = { ...decisionFor(policy), ...patch, fingerprint: policy.fingerprint };
      }
      scheduleSave();
      options.onPolicyChange?.(envelope, gateState());
    }

    function gateState() {
      const required = policies.filter((policy) => policy.requiredForSkillCompletion !== false);
      const complete = required.filter((policy) => policyDecisionComplete(policy, decisionFor(policy))).length;
      return { required: required.length, complete, remaining: required.length - complete, closed: required.length > 0 && complete === required.length };
    }

    function scheduleSave() {
      globalThis.clearTimeout?.(state.saveTimer);
      state.saveTimer = globalThis.setTimeout?.(() => {
        try {
          if (storageKey) {
            storage?.setItem?.(storageKey, JSON.stringify(envelope));
            options.notify?.('Politiques Phase 0 sauvegardées localement.', 'ok');
          }
        } catch (error) {
          options.notify?.('Autosave Phase 0 indisponible : ' + error.message, 'error');
        }
      }, 180);
    }

    function columnSearchText(column) {
      return [
        column.id, column.canonicalHeader, ...SOURCE_KEYS.flatMap((source) => asArray(column.rawHeaders?.[source])),
        column.playerLabelFr, column.shortHelpFr, column.family, column.consumer,
        ...classificationsOf(column), column.decisionScope, column.defaultPolicy, column.proofStatus,
        column.documentation?.description, column.documentation?.descriptionEn, column.documentation?.eezstreet,
      ].filter(Boolean).join(' ').toLocaleLowerCase('fr');
    }

    function visibleColumns() {
      return columns.filter((column) => {
        const usage = totalUsage(column);
        const presenceCount = SOURCE_KEYS.filter((source) => present(column, source)).length;
        if (state.query && !columnSearchText(column).includes(state.query.toLocaleLowerCase('fr'))) return false;
        if (state.source === 'source-only' && presenceCount !== 1) return false;
        if (SOURCE_KEYS.includes(state.source) && !present(column, state.source)) return false;
        if (state.usage === 'used' && usage.nonEmpty === 0) return false;
        if (state.usage === 'unused' && usage.nonEmpty !== 0) return false;
        if (state.usage === 'player' && usage.players === 0) return false;
        if (state.usage === 'technical' && usage.technical === 0) return false;
        if (state.classification && !classificationsOf(column).includes(state.classification)) return false;
        if (state.decisionScope && column.decisionScope !== state.decisionScope) return false;
        if (state.proof && column.proofStatus !== state.proof) return false;
        return true;
      });
    }

    function optionList(values, current, allLabel = 'Tous') {
      return '<option value="">' + escapeHtml(allLabel) + '</option>' + values.map((value) => '<option value="' + escapeAttribute(value) + '"' + (current === value ? ' selected' : '') + '>' + escapeHtml(friendly(value)) + '</option>').join('');
    }

    function sourceStats(source) {
      const presentColumns = columns.filter((column) => present(column, source));
      return {
        total: presentColumns.length,
        used: presentColumns.filter((column) => usageFor(column, source).nonEmpty > 0).length,
        sourceOnly: presentColumns.filter((column) => SOURCE_KEYS.filter((item) => present(column, item)).length === 1).length,
      };
    }

    function renderOverview() {
      return '<section class="schema-section" id="schema-overview"><div class="schema-section-heading"><div><p class="schema-kicker">Contrat cible D2R 3.2</p><h3>Vue d’ensemble des schémas</h3><p>Les colonnes D2R/BKVince sont préservées; PD2 reste une source sémantique tant que sa consommation D2R n’est pas prouvée.</p></div></div>'
        + '<div class="schema-summary-grid">' + SOURCE_KEYS.map((source) => {
          const stats = sourceStats(source);
          return '<article class="schema-summary-card"><span>' + escapeHtml(SOURCE_LABELS[source]) + '</span><strong>' + stats.total + '</strong><small>' + stats.used + ' utilisées · ' + stats.sourceOnly + ' propres à cette source</small></article>';
        }).join('') + '</div></section>';
    }

    function sourceOnlyColumns() {
      return columns.filter((column) => SOURCE_KEYS.filter((source) => present(column, source)).length === 1);
    }

    function renderSourceOnly() {
      const grouped = SOURCE_KEYS.map((source) => ({ source, columns: sourceOnlyColumns().filter((column) => present(column, source)) }));
      return '<section class="schema-section" id="schema-source-only"><div class="schema-section-heading"><div><h3>Colonnes propres à chaque source</h3><p>Une présence de header n’est jamais une preuve de support par un autre moteur.</p></div></div><div class="schema-source-only">'
        + grouped.map(({ source, columns: items }) => '<article><h4>' + escapeHtml(SOURCE_LABELS[source]) + ' · ' + items.length + '</h4>' + (items.length ? '<div class="schema-tags">' + items.map((column) => '<span class="schema-tag ' + (classificationsOf(column).some((value) => value.includes('NATIVE')) ? 'native' : '') + '">' + escapeHtml(column.canonicalHeader || column.id) + '</span>').join('') + '</div>' : '<p class="schema-muted">Aucune colonne propre.</p>') + '</article>').join('')
        + '</div></section>';
    }

    function renderControls(filteredCount) {
      const classifications = [...new Set(columns.flatMap(classificationsOf))].sort();
      const scopes = [...new Set(columns.map((column) => column.decisionScope).filter(Boolean))].sort();
      const proofs = [...new Set(columns.map((column) => column.proofStatus).filter(Boolean))].sort();
      return '<div class="schema-controls" aria-label="Recherche et filtres des colonnes">'
        + '<label class="schema-search"><span>Rechercher une colonne ou un concept</span><input type="search" data-schema-search value="' + escapeAttribute(state.query) + '" placeholder="mana, cooldown, itemeffect…"></label>'
        + '<label><span>Source</span><select data-schema-filter="source">' + optionList([...SOURCE_KEYS, 'source-only'], state.source) + '</select></label>'
        + '<label><span>Utilisation</span><select data-schema-filter="usage">' + optionList(['used', 'unused', 'player', 'technical'], state.usage) + '</select></label>'
        + '<label><span>Classification</span><select data-schema-filter="classification">' + optionList(classifications, state.classification) + '</select></label>'
        + '<label><span>Portée</span><select data-schema-filter="decisionScope">' + optionList(scopes, state.decisionScope) + '</select></label>'
        + '<label><span>Preuve</span><select data-schema-filter="proof">' + optionList(proofs, state.proof) + '</select></label>'
        + '<button type="button" data-schema-action="clear-filters">Effacer</button><span class="schema-toolbar-note">' + filteredCount + ' / ' + columns.length + ' headers</span></div>';
    }

    function renderRawLegend() {
      return '<div class="schema-legend" aria-label="États physiques des cellules">' + ['ABSENT_COLUMN', 'NULL_VALUE', 'EMPTY_STRING', 'ZERO', 'VALUE'].map((rawState) => '<span class="schema-raw-state ' + rawState.toLowerCase() + '">' + rawState + '</span>').join('') + '</div><p class="schema-muted"><code>semanticBlank</code> regroupe absent, null et vide pour la décision gameplay. <strong>ZERO reste une valeur.</strong> La preuve physique demeure distincte.</p>';
    }

    function renderEvidenceCard(column, source) {
      const usage = usageFor(column, source);
      const examples = examplesFor(column, source).map((example) => normalizeEvidence(example, source, column));
      if (!examples.length && !present(column, source)) examples.push(normalizeEvidence(undefined, source, column));
      return '<article class="schema-evidence-card"><h5>' + escapeHtml(SOURCE_LABELS[source]) + '</h5><dl><dt>Header brut</dt><dd><code>' + escapeHtml(rawHeaderFor(column, source)) + '</code></dd><dt>Utilisation</dt><dd>' + usage.nonEmpty + ' cellules non vides · ' + usage.players + ' skills joueurs · ' + usage.technical + ' lignes techniques/monstres</dd></dl>'
        + (examples.length ? '<ul>' + examples.map((example) => '<li><strong>' + escapeHtml(example.label) + '</strong> · <span class="schema-raw-state ' + String(example.rawState).toLowerCase() + '">' + escapeHtml(example.rawState) + '</span> · <code>' + escapeHtml(display(example.rawValue)) + '</code>' + (example.semanticBlank ? ' · semanticBlank' : '') + '</li>').join('') + '</ul>' : '<p class="schema-muted">Aucun exemple de cellule requis.</p>') + '</article>';
    }

    function renderColumnDetail(column) {
      const documentation = column.documentation || {};
      const equivalent = column.potentialEquivalent || column.equivalentPotential || 'Aucun équivalent prouvé';
      return '<div class="schema-column-detail">' + renderRawLegend()
        + '<div class="schema-detail-grid">' + SOURCE_KEYS.map((source) => renderEvidenceCard(column, source)).join('') + '</div>'
        + '<div class="schema-detail-grid"><div><strong>Description eezstreet</strong><p>' + escapeHtml(documentation.description || documentation.descriptionEn || documentation.eezstreet || documentation.text || 'Non documentée dans l’oracle.') + '</p></div><div><strong>Consumer connu</strong><p>' + escapeHtml(column.consumer || column.knownConsumer || 'UNKNOWN_NATIVE_CONSUMER') + '</p></div><div><strong>Équivalent potentiel</strong><p>' + escapeHtml(typeof equivalent === 'string' ? equivalent : JSON.stringify(equivalent)) + '</p></div></div>'
        + '<p><strong>Politique par défaut :</strong> ' + escapeHtml(column.defaultPolicy || 'Aucune') + ' · <strong>Raison décisionnelle :</strong> ' + escapeHtml(column.semanticDifferenceReason || 'Voir classification et contrat mécanique.') + '</p></div>';
    }

    function renderMatrix() {
      const filtered = visibleColumns();
      return '<section class="schema-section" id="schema-used-columns"><div class="schema-section-heading"><div><h3>Colonnes réellement utilisées</h3><p>Matrice canonique, descriptions joueur et preuves brutes accessibles sans perdre les distinctions absent / vide / zéro.</p></div></div>' + renderControls(filtered.length)
        + (filtered.length ? '<div class="schema-matrix-wrap"><table class="schema-matrix"><thead><tr><th>Champ gouverné</th><th>Vanilla</th><th>BKVince</th><th>PD2</th><th>Non vides</th><th>Joueur</th><th>Technique</th><th>Classification</th><th>Décision</th></tr></thead><tbody>' + filtered.map((column) => {
          const id = column.id || column.canonicalHeader;
          const usage = totalUsage(column);
          const expanded = state.expandedColumns.has(id);
          return '<tr><td><button type="button" class="schema-column-button" data-schema-action="toggle-column" data-schema-column-id="' + escapeAttribute(id) + '" aria-expanded="' + expanded + '"><strong>' + escapeHtml(column.playerLabelFr || column.canonicalHeader || id) + '</strong><small><code>' + escapeHtml(column.canonicalHeader || id) + '</code> · ' + escapeHtml(column.shortHelpFr || '') + '</small></button></td>'
            + SOURCE_KEYS.map((source) => '<td class="schema-source-cell ' + (present(column, source) ? 'schema-present' : 'schema-absent') + '">' + (present(column, source) ? '✓' : '—') + '<small>' + escapeHtml(rawHeaderFor(column, source)) + '</small></td>').join('')
            + '<td>' + usage.nonEmpty + '</td><td>' + usage.players + '</td><td>' + usage.technical + '</td><td>' + classificationsOf(column).map((value) => '<span class="schema-tag">' + escapeHtml(value) + '</span>').join('') + '</td><td><span class="schema-tag policy">' + escapeHtml(column.decisionScope || 'NO_SKILL_DECISION') + '</span><small>' + escapeHtml(column.groupId || '') + '</small></td></tr>'
            + (expanded ? '<tr class="schema-detail-row"><td colspan="9">' + renderColumnDetail(column) + '</td></tr>' : '');
        }).join('') + '</tbody></table></div>' : '<p class="schema-empty">Aucune colonne ne correspond aux filtres.</p>') + '</section>';
    }

    function renderContracts() {
      return '<section class="schema-section" id="schema-mechanical-contracts"><div class="schema-section-heading"><div><h3>Contrats mécaniques</h3><p>Relations prouvées, hypothèses et politique de traduction restent séparées.</p></div></div><div class="schema-contract-grid">' + contracts.map((contract) => '<details class="schema-contract"' + (state.expandedContracts.has(contract.id) ? ' open' : '') + '><summary><strong>' + escapeHtml(contract.titleFr || contract.id) + '</strong> · <span class="schema-tag">' + escapeHtml(contract.proofStatus || 'UNKNOWN') + '</span></summary><p>' + escapeHtml(contract.consumerFr || '') + '</p><div class="schema-tags">' + asArray(contract.fields).map((field) => '<code class="schema-tag">' + escapeHtml(field) + '</code>').join('') + '</div><h5>Relations prouvées</h5><ul>' + asArray(contract.provenRelations).map((item) => '<li>' + escapeHtml(item) + '</li>').join('') + '</ul><h5>Relations non prouvées</h5><ul class="hypothesis">' + asArray(contract.hypotheses).map((item) => '<li>' + escapeHtml(item) + '</li>').join('') + '</ul><p><strong>Politique proposée :</strong> <code>' + escapeHtml(contract.translationPolicy || '—') + '</code></p></details>').join('') + '</div></section>';
    }

    function renderDictionary() {
      const playerFields = columns.filter((column) => column.playerLabelFr || column.shortHelpFr);
      return '<section class="schema-section" id="schema-field-dictionary"><div class="schema-section-heading"><div><h3>Dictionnaire joueur</h3><p>Libellés courts gouvernés; le header brut reste le sous-titre technique.</p></div></div><div class="schema-dictionary">' + playerFields.map((column) => '<article><h4>' + escapeHtml(column.playerLabelFr || column.canonicalHeader) + '</h4><code>' + escapeHtml(column.canonicalHeader || column.id) + '</code><p>' + escapeHtml(column.shortHelpFr || 'Aide courte non disponible.') + '</p><div class="schema-tags"><span class="schema-tag">' + escapeHtml(column.family || 'technical') + '</span>' + (column.technicalOnly ? '<span class="schema-tag protected">Technique seulement</span>' : '') + (column.protected ? '<span class="schema-tag protected">Protégé</span>' : '') + '<span class="schema-tag policy">' + escapeHtml(column.groupId || column.decisionScope || '—') + '</span></div></article>').join('') + '</div></section>';
    }

    function policyOptions(selected) {
      return POLICY_DECISIONS.map((value) => '<option value="' + value + '"' + (value === selected ? ' selected' : '') + '>' + friendly(value) + '</option>').join('');
    }

    function renderPolicies() {
      const gate = gateState();
      return '<section class="schema-section" id="schema-global-policies"><div class="schema-section-heading"><div><h3>Décisions globales</h3><p>Les orientations proposées ne sont jamais préapprouvées. Elles doivent être confirmées ou modifiées explicitement par Vincent.</p></div></div><div class="schema-policy-toolbar"><button type="button" data-schema-action="export-policies">Exporter les politiques</button><button type="button" data-schema-action="import-policies">Importer les politiques</button><input type="file" data-schema-policy-file accept="application/json" hidden><button type="button" class="danger" data-schema-action="reset-policies">Réinitialiser les politiques</button><span class="schema-toolbar-note">' + (storageKey ? 'localStorage <code>' + escapeHtml(storageKey) + '</code>' : 'Adaptateur de gouvernance absent : état temporaire en mémoire') + '</span></div><div class="schema-policy-grid">' + policies.map((policy) => {
        const value = decisionFor(policy);
        const complete = policyDecisionComplete(policy, value);
        return '<article class="schema-policy ' + (policy.requiredForSkillCompletion !== false ? 'required ' : '') + (complete ? 'complete' : '') + '" data-policy-id="' + escapeAttribute(policy.id) + '"><h4>' + escapeHtml(policy.titleFr || policy.id) + '</h4><code>' + escapeHtml(policy.id) + '</code><small class="schema-muted">fingerprint ' + escapeHtml(policy.fingerprint || 'absent') + '</small><p>' + escapeHtml(policy.statementFr || '') + '</p><p class="schema-muted">Orientation proposée : <strong>' + escapeHtml(policy.proposedDecision || 'APPROVE') + '</strong> · état réel : <strong>' + escapeHtml(value.decision || 'PENDING') + '</strong></p><label><span>Décision de Vincent</span><select data-schema-policy-decision data-policy-id="' + escapeAttribute(policy.id) + '">' + policyOptions(value.decision || 'PENDING') + '</select></label><label><span>Justification' + (value.decision === 'APPROVE' || value.decision === 'MODIFY' ? ' obligatoire' : '') + '</span><textarea data-schema-policy-property="justification" data-policy-id="' + escapeAttribute(policy.id) + '">' + escapeHtml(value.justification || '') + '</textarea></label>' + (value.decision === 'MODIFY' ? '<label class="schema-policy-custom"><span>Politique modifiée explicite</span><textarea data-schema-policy-property="customPolicy" data-policy-id="' + escapeAttribute(policy.id) + '">' + escapeHtml(value.customPolicy || '') + '</textarea></label>' : '') + '</article>';
      }).join('') + '</div><p class="schema-gate ' + (gate.closed ? 'closed' : '') + '"><strong>Gate Phase 0 :</strong> ' + gate.complete + ' / ' + gate.required + ' politiques requises fermées. ' + (gate.closed ? 'Le gate global est fermé; cela n’autorise toujours aucune implantation.' : 'Les décisions de skills restent incomplètes tant que ce gate n’est pas fermé.') + '</p></section>';
    }

    function impactValue(impact, keys, fallback = '—') {
      const value = keys.map((key) => key.split('.').reduce((current, part) => current?.[part], impact)).find((candidate) => candidate !== undefined && candidate !== null);
      return value ?? fallback;
    }

    function renderImpact() {
      const impact = orientation.fireBoltImpact || {};
      const steps = [
        ['Champs bruts modifiés', impactValue(impact, ['currentModifiedFields', 'rawChangedFields', 'before.modifiedFields'])],
        ['Décisions actuelles', impactValue(impact, ['currentRequiredDecisions', 'before.requiredDecisions'])],
        ['Bundles atomiques', impactValue(impact, ['bundleCount', 'bundleProjection.length', 'bundles.length', 'after.bundleCount'])],
        ['Décisions joueur finales', impactValue(impact, ['finalPlayerDecisions', 'finalRequiredDecisions', 'after.requiredDecisions'])],
      ];
      const reductionEntries = objectEntries(impact.reductions);
      const reductions = reductionEntries.length
        ? reductionEntries.map(([id, value]) => ({ id, ...value }))
        : asArray(impact.reductionReasons || impact.eliminations);
      const blankFields = asArray(impact.semanticBlankFields || impact.eliminatedSemanticBlankFields || impact.reductions?.semanticBlank?.fields);
      const technicalFields = asArray(impact.technicalOrDocumentaryFields || impact.eliminatedTechnicalFields || impact.reductions?.technicalOrDocumentary?.fields);
      const bundles = asArray(impact.bundles || impact.bundleProjection);
      return '<section class="schema-section" id="schema-fire-bolt-impact"><div class="schema-section-heading"><div><h3>Impact sur le nombre de décisions — Fire Bolt</h3><p>La réduction vient de blancs sémantiques, de champs techniques auto-résolus et de bundles; aucune différence pertinente n’est cachée.</p></div></div><div class="schema-impact-flow">' + steps.map(([label, value]) => '<article class="schema-impact-step"><strong>' + escapeHtml(value) + '</strong><span>' + escapeHtml(label) + '</span></article>').join('') + '</div>'
        + '<div class="schema-detail-grid"><div><h4>SemanticBlank sans décision</h4>' + renderCompactList(blankFields) + '</div><div><h4>Technique / documentaire</h4>' + renderCompactList(technicalFields) + '</div><div><h4>Projection en bundles</h4>' + renderCompactList(bundles) + '</div></div>'
        + (reductions.length ? '<ul class="schema-impact-reductions">' + reductions.map((item) => '<li><strong>' + escapeHtml(typeof item === 'string' ? 'Réduction' : friendly(item.id || item.category || 'Réduction')) + '</strong> · ' + escapeHtml(typeof item === 'string' ? item : (item.count ?? asArray(item.fields).length) + ' champ(s) · ' + asArray(item.reasons || item.reason || item.justification).join('; ')) + '</li>').join('') + '</ul>' : '') + '</section>';
    }

    function renderCompactList(values) {
      return values.length ? '<div class="schema-tags">' + values.map((value) => '<span class="schema-tag">' + escapeHtml(typeof value === 'string' ? value : value.labelFr || value.groupId || value.id || value.header || value.bundleId || JSON.stringify(value)) + '</span>').join('') + '</div>' : '<p class="schema-muted">Aucun élément.</p>';
    }

    function renderQuestions() {
      const questions = asArray(orientation.unresolvedNativeQuestions);
      return '<section class="schema-section" id="schema-native-questions"><div class="schema-section-heading"><div><h3>Questions natives non résolues</h3><p>Un header ou un numéro de callback PD2 ne prouve jamais sa consommation par D2R 3.2.</p></div></div>' + (questions.length ? '<ul class="schema-questions">' + questions.map((question) => '<li><strong>' + escapeHtml(question.titleFr || question.id || 'Question native') + '</strong><p>' + escapeHtml(typeof question === 'string' ? question : question.question || question.summary || question.reason || JSON.stringify(question)) + '</p><span class="schema-tag native">' + escapeHtml(question.proofStatus || 'NATIVE_UNPROVEN') + '</span></li>').join('') + '</ul>' : '<p class="schema-empty">Aucune question native dans cet oracle.</p>') + '</section>';
    }

    function renderWitnesses() {
      const witnesses = orientation.witnesses;
      if (!witnesses || (Array.isArray(witnesses) && !witnesses.length) || (!Array.isArray(witnesses) && !objectEntries(witnesses).length)) return '';
      const entries = Array.isArray(witnesses) ? witnesses.map((item, index) => [item.id || String(index), item]) : objectEntries(witnesses);
      return '<section class="schema-section" id="schema-witnesses"><div class="schema-section-heading"><div><h3>Témoins gouvernés</h3><p>delay, cost add, item-trigger, paliers élémentaires, auraevent4, checkfunc et Fire Bolt.</p></div></div><div class="schema-contract-grid">' + entries.map(([id, witness]) => '<article class="schema-contract"><h4>' + escapeHtml(witness.titleFr || witness.label || id) + '</h4><p>' + escapeHtml(witness.summary || witness.semanticDifferenceReason || witness.reason || '') + '</p><pre>' + escapeHtml(JSON.stringify(witness, null, 2)) + '</pre></article>').join('') + '</div></section>';
    }

    function render() {
      const gate = gateState();
      return '<div class="schema-orientation" data-orientation-hash="' + escapeAttribute(orientation.orientationHash || '') + '"><section class="schema-hero"><p class="schema-kicker">Phase 0 · avant toute revue skill par skill</p><h2>' + escapeHtml(orientation.productName || 'PD2 Skills Schema and Engine Orientation') + '</h2><p>Contrat global entre Vanilla D2R 3.2, BKVince HEAD et le snapshot PD2/SP+ épinglé. Cette vue est analytique et ne peut modifier aucune table gameplay.</p><div class="schema-gate ' + (gate.closed ? 'closed' : '') + '"><strong>' + gate.complete + ' / ' + gate.required + '</strong><span>politiques requises fermées</span><progress max="' + Math.max(gate.required, 1) + '" value="' + gate.complete + '"></progress></div></section>'
        + renderOverview() + renderSourceOnly() + renderMatrix() + renderContracts() + renderDictionary() + renderPolicies() + renderImpact() + renderWitnesses() + renderQuestions() + '</div>';
    }

    function handleClick(event) {
      const button = event.target?.closest?.('[data-schema-action]');
      if (!button) return false;
      const action = button.dataset.schemaAction;
      if (action === 'toggle-column') {
        const id = button.dataset.schemaColumnId;
        if (state.expandedColumns.has(id)) state.expandedColumns.delete(id); else state.expandedColumns.add(id);
        options.requestRender?.();
      } else if (action === 'clear-filters') {
        state.query = ''; state.source = ''; state.usage = ''; state.classification = ''; state.decisionScope = ''; state.proof = '';
        options.requestRender?.();
      } else if (action === 'export-policies') {
        const exportPolicy = runtime?.exportEnvelope || runtime?.exportPolicyEnvelope;
        const exported = typeof exportPolicy === 'function' ? exportPolicy.call(runtime, orientation, envelope) : { ...envelope, exportedAt: new Date().toISOString() };
        (options.download || download)('pd2-skills-schema-policy.json', JSON.stringify(exported, null, 2) + '\n', 'application/json');
      } else if (action === 'import-policies') {
        options.root?.querySelector?.('[data-schema-policy-file]')?.click?.();
      } else if (action === 'reset-policies') {
        if ((options.confirm || globalThis.confirm)?.('Réinitialiser toutes les décisions globales Phase 0 ?')) {
          envelope = emptyEnvelope();
          if (storageKey) storage?.removeItem?.(storageKey);
          options.onPolicyChange?.(envelope, gateState());
          options.requestRender?.();
        }
      }
      return true;
    }

    function handleChange(event) {
      const target = event.target;
      if (target?.matches?.('[data-schema-filter]')) {
        state[target.dataset.schemaFilter] = target.value;
        options.requestRender?.();
        return true;
      }
      if (target?.matches?.('[data-schema-policy-decision]')) {
        updateDecision(target.dataset.policyId, { decision: target.value || 'PENDING' });
        options.requestRender?.();
        return true;
      }
      if (target?.matches?.('[data-schema-policy-file]')) {
        importPolicyFile(target.files?.[0]);
        target.value = '';
        return true;
      }
      return false;
    }

    function handleInput(event) {
      const target = event.target;
      if (target?.matches?.('[data-schema-search]')) {
        state.query = target.value;
        globalThis.clearTimeout?.(handleInput.searchTimer);
        handleInput.searchTimer = globalThis.setTimeout?.(() => options.requestRender?.(), 120);
        return true;
      }
      if (target?.matches?.('[data-schema-policy-property]')) {
        updateDecision(target.dataset.policyId, { [target.dataset.schemaPolicyProperty]: target.value });
        return true;
      }
      return false;
    }

    async function importPolicyFile(file) {
      if (!file) return;
      try {
        envelope = validateEnvelope(JSON.parse(await file.text()));
        if (storageKey) storage?.setItem?.(storageKey, JSON.stringify(envelope));
        options.notify?.('Politiques Phase 0 importées et validées.', 'ok');
        options.onPolicyChange?.(envelope, gateState());
        options.requestRender?.();
      } catch (error) {
        options.notify?.('Import Phase 0 refusé : ' + error.message, 'error');
      }
    }

    function setEnvelope(candidate, setOptions = {}) {
      envelope = validateEnvelope(candidate);
      if (setOptions.persist !== false && storageKey) storage?.setItem?.(storageKey, JSON.stringify(envelope));
      options.onPolicyChange?.(envelope, gateState());
      if (setOptions.render !== false) options.requestRender?.();
      return envelope;
    }

    function mount(root) {
      options.root = root;
      const rerender = () => { root.innerHTML = render(); };
      if (!options.requestRender) options.requestRender = rerender;
      root.addEventListener('click', handleClick);
      root.addEventListener('change', handleChange);
      root.addEventListener('input', handleInput);
      rerender();
      return controller;
    }

    function download(filename, content, type) {
      const url = URL.createObjectURL(new Blob([content], { type: type + ';charset=utf-8' }));
      const anchor = document.createElement('a');
      anchor.href = url;
      anchor.download = filename;
      anchor.click();
      globalThis.setTimeout?.(() => URL.revokeObjectURL(url), 0);
    }

    const controller = { render, mount, handleClick, handleChange, handleInput, gateState, getEnvelope: () => envelope, setEnvelope, storageKey, state };
    options.onPolicyChange?.(envelope, gateState());
    return controller;
  }

  globalThis.schemaOrientationUI = Object.freeze({ createController });
}

function schemaOrientationBootstrap() {
  'use strict';

  async function inflateOracle(base64) {
    if (typeof DecompressionStream !== 'function') throw new Error('Ce navigateur ne fournit pas DecompressionStream("gzip"). Utilisez une version récente de Chromium, Edge ou Firefox.');
    const binary = atob(base64);
    const compressed = new Uint8Array(binary.length);
    for (let index = 0; index < binary.length; index += 1) compressed[index] = binary.charCodeAt(index);
    const stream = new Blob([compressed]).stream().pipeThrough(new DecompressionStream('gzip'));
    return JSON.parse(await new Response(stream).text());
  }

  function fatal(error) {
    const root = document.querySelector('#schema-orientation') || document.body;
    root.innerHTML = '<main class="schema-empty"><h1>PD2 Skills Schema and Engine Orientation</h1><p>Impossible de charger l’oracle autonome.</p><pre></pre></main>';
    const details = root.querySelector?.('pre');
    if (details) details.textContent = error instanceof Error ? error.message : String(error);
  }

  globalThis.__PD2_SCHEMA_ORIENTATION_READY__ = (async () => {
    try {
      const orientation = await inflateOracle(globalThis.__PD2_SCHEMA_ORIENTATION_GZIP_BASE64__);
      globalThis.__PD2_SCHEMA_ORIENTATION_REPORT__ = orientation;
      __PD2_SCHEMA_POLICY_RUNTIME__
      __PD2_SCHEMA_ORIENTATION_APPLICATION__
      const root = document.querySelector('#schema-orientation');
      const controller = globalThis.schemaOrientationUI.createController(orientation, {
        policyRuntime: globalThis.schemaPolicyRuntime,
        storage: globalThis.localStorage,
        root,
      });
      controller.mount(root);
      globalThis.__PD2_SCHEMA_ORIENTATION_CONTROLLER__ = controller;
      return orientation;
    } catch (error) {
      fatal(error);
      throw error;
    } finally {
      delete globalThis.__PD2_SCHEMA_ORIENTATION_GZIP_BASE64__;
    }
  })();
}

export function buildSchemaOrientationApplicationSource() {
  return '(' + schemaOrientationBrowserApplication.toString() + ')();';
}

export function buildSchemaOrientationHtml(orientation, options = {}) {
  if (!orientation || typeof orientation !== 'object' || Array.isArray(orientation)) throw new TypeError('orientation must be an oracle object');
  if (!Array.isArray(orientation.columns)) throw new TypeError('orientation.columns must be an array');
  if (!orientation.orientationId) throw new TypeError('orientation.orientationId is required');
  if (!orientation.orientationHash) throw new TypeError('orientation.orientationHash is required');
  const compressedOracleBase64 = options.compressedOracleBase64 || zlib.gzipSync(Buffer.from(JSON.stringify(orientation)), { level: 9, mtime: 0 }).toString('base64');
  if (typeof compressedOracleBase64 !== 'string' || !compressedOracleBase64.length) throw new TypeError('compressed oracle payload must be non-empty');
  const policyRuntimeSource = typeof options.policyRuntimeSource === 'string' ? protectInlineSource(options.policyRuntimeSource) : '';
  const applicationSource = protectInlineSource(buildSchemaOrientationApplicationSource());
  const bootstrapSource = protectInlineSource('(' + schemaOrientationBootstrap.toString() + ')();')
    .replace('__PD2_SCHEMA_POLICY_RUNTIME__', () => policyRuntimeSource)
    .replace('__PD2_SCHEMA_ORIENTATION_APPLICATION__', () => applicationSource);
  const binding = options.workbenchBinding && typeof options.workbenchBinding === 'object'
    ? '<meta name="pd2-workbench-binding" content="' + escapeHtmlAttribute(JSON.stringify(options.workbenchBinding)) + '">'
    : '';
  return '<!doctype html>\n<html lang="fr"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="referrer" content="no-referrer">' + binding
    + '<title>PD2 Skills Schema and Engine Orientation</title><style>:root{color-scheme:dark;--bg:#091018;--panel:#111c28;--panel-2:#172536;--line:#344a61;--text:#eef5fb;--muted:#9fb0c1;--gold:#f0bd6b;--blue:#72c8ff;--green:#6ed49a;--red:#ff8d88}*{box-sizing:border-box}body{margin:0;background:radial-gradient(circle at 75% -20%,#17304c 0,transparent 42%),var(--bg);color:var(--text);font:14px/1.5 Inter,ui-sans-serif,system-ui,-apple-system,"Segoe UI",sans-serif}button,input,select,textarea{font:inherit;border:1px solid var(--line);border-radius:7px;background:var(--panel-2);color:var(--text);padding:8px 10px}button{cursor:pointer}button:hover,button:focus-visible{border-color:var(--gold);outline:none}textarea{width:100%;resize:vertical}code,pre{font:12px/1.45 ui-monospace,SFMono-Regular,Consolas,monospace}pre{white-space:pre-wrap;overflow:auto;max-height:420px;background:#09131e;padding:10px;border-radius:7px}.orientation-shell{max-width:1700px;margin:auto;padding:20px}.orientation-notice{padding:9px 12px;border-left:3px solid var(--gold);background:#302615;margin-bottom:14px}.danger{border-color:#74454a;color:#ffc0bd}table{width:100%;border-collapse:collapse}th,td{border:1px solid var(--line);padding:7px;text-align:left;vertical-align:top}th{background:#172536}small{display:block}' + SCHEMA_ORIENTATION_STYLE + '</style></head><body><main class="orientation-shell"><p class="orientation-notice"><strong>Phase analytique uniquement.</strong> Aucune table gameplay, sauvegarde, ordinal ou profil runtime ne peut être modifié depuis ce document.</p><div id="schema-orientation"></div></main><noscript><main class="orientation-shell"><h1>PD2 Skills Schema and Engine Orientation</h1><p>JavaScript local est requis pour lire l’oracle compressé et utiliser les filtres.</p></main></noscript><script>globalThis.__PD2_SCHEMA_ORIENTATION_GZIP_BASE64__="' + compressedOracleBase64 + '";\n' + bootstrapSource + '\n</script></body></html>\n';
}

function escapeHtmlAttribute(value) {
  return String(value).replace(/[&<>"]/g, (character) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' })[character]);
}
