/**
 * Build the standalone PD2 Skills Merge Workbench document.
 *
 * browserRuntimeSource must install `globalThis.decisionRuntime` with this API:
 * - createEmptyEnvelope(report)
 * - createEntry(skill)
 * - entryState(report, skill, entry)
 * - applyBulk(report, skillIds, entries, action, { replace, confirmed })
 * - validateImport(report, payload)
 * - migrateEnvelope(report, payload)
 * - exportEnvelope(report, entries, { scope })
 * Optional helpers used when present: storageKey(report), progress(report, entries),
 * resolveFieldChoice(skill, entry, component, field), validateChoice(...), constants.
 *
 * The UI deliberately delegates decision semantics, completion, migration and bulk
 * protection rules to that runtime. It only owns rendering and browser I/O.
 */

function serializeForInlineScript(value) {
  return JSON.stringify(value)
    .replace(/&/g, '\\u0026')
    .replace(/</g, '\\u003c')
    .replace(/>/g, '\\u003e')
    .replace(/\u2028/g, '\\u2028')
    .replace(/\u2029/g, '\\u2029');
}

function protectInlineSource(source) {
  return String(source)
    .replace(/<\/script/gi, '<\\/script')
    .replace(/<script/gi, '<\\x73cript');
}

function skillReviewBrowserApplication() {
  'use strict';

  const runtime = globalThis.decisionRuntime;
  const requiredRuntimeMethods = [
    'createEmptyEnvelope',
    'createEntry',
    'entryState',
    'applyBulk',
    'validateImport',
    'migrateEnvelope',
    'exportEnvelope',
  ];
  const missingRuntimeMethods = requiredRuntimeMethods.filter((name) => typeof runtime?.[name] !== 'function');
  if (missingRuntimeMethods.length) {
    document.body.innerHTML = '<main class="fatal"><h1>PD2 Skills Merge Workbench</h1><p>Le moteur de décisions embarqué est incomplet.</p><pre>'
      + escapeHtml(missingRuntimeMethods.join(', ')) + '</pre></main>';
    return;
  }

  const SOURCE_KEYS = ['vanilla32', 'bkvince', 'pd2'];
  const SOURCE_LABELS = {
    vanilla32: 'Vanilla D2R 3.2',
    bkvince: 'BKVince actuel',
    pd2: 'Project Diablo 2',
  };
  const FALLBACK_GLOBAL_DECISIONS = [
    'KEEP_BKVINCE', 'ADAPT_PD2_SELECTIVELY', 'ADOPT_PD2_MODEL',
    'IMPORT_NEW_PD2_SKILL', 'REJECT_PD2', 'DEFER_NATIVE_PROOF', 'DISCUSS',
  ];
  const FALLBACK_COMPONENT_DECISIONS = ['KEEP_BKVINCE', 'ADOPT_PD2', 'CUSTOM', 'DISCUSS', 'NOT_APPLICABLE'];
  const FALLBACK_LINE_DECISIONS = ['IMPORT_APPEND_ONLY', 'IMPORT_CUSTOMIZED', 'REJECT_PD2_SKILL', 'DEFER_NATIVE_PROOF', 'DISCUSS'];
  const FALLBACK_IMPLEMENTATION_STATUSES = [
    'NOT_REVIEWED', 'DECISION_INCOMPLETE', 'DECISION_COMPLETE', 'SELECTED_FOR_PROTOTYPE',
    'IMPLEMENTATION_NOT_AUTHORIZED', 'IMPLEMENTATION_AUTHORIZED', 'IMPLEMENTED', 'TESTED', 'REJECTED',
  ];
  const enums = report.enums || {};
  const GLOBAL_DECISIONS = enumValues('globalDecisions', 'GLOBAL_DECISIONS', FALLBACK_GLOBAL_DECISIONS);
  const COMPONENT_DECISIONS = enumValues('componentDecisions', 'COMPONENT_DECISIONS', FALLBACK_COMPONENT_DECISIONS);
  const LINE_DECISIONS = enumValues('newSkillLineDecisions', 'NEW_SKILL_LINE_DECISIONS', FALLBACK_LINE_DECISIONS);
  const IMPLEMENTATION_STATUSES = enumValues('implementationStatuses', 'IMPLEMENTATION_STATUSES', FALLBACK_IMPLEMENTATION_STATUSES);
  const MAPPING_TYPES = enumValues('mappingTypes', 'MAPPING_TYPES', unique(report.skills.flatMap((skill) => skill.mappingTypes || [])));
  const PROOF_STATUSES = enumValues('proofStatuses', 'PROOF_STATUSES', unique(report.skills.flatMap(proofStatusesOf)));
  const PORTABILITY_CATEGORIES = enumValues('portabilityCategories', 'PORTABILITY_CATEGORIES', unique(report.skills.flatMap(portabilityCategoriesOf)));
  const storageKey = typeof runtime.storageKey === 'function'
    ? runtime.storageKey(report)
    : 'pd2-skills-review-decisions-v1:' + report.comparisonHash;
  const skillById = new Map(report.skills.map((skill) => [skill.stableId, skill]));
  const nodeById = new Map((report.nodes || []).map((node) => [node.id, node]));
  const collisionById = new Map((report.collisions || []).map((collision) => [collision.id, collision]));
  const normalizedNavigation = normalizeNavigation(report.navigation || [], report.skills);
  const navById = new Map(normalizedNavigation.map((view) => [view.id, view]));
  const ui = {
    activeViewId: normalizedNavigation[0]?.id || 'all',
    mode: 'player',
    expanded: new Set(),
    filters: {
      query: '', incomplete: false, significant: false, portability: '', mapping: '', proof: '',
      decision: '', collision: '', nativeRisk: '', pd2New: '',
    },
    scenario: new Map(),
    synergyInputs: new Map(),
    saveTimer: null,
    storageAvailable: true,
  };
  let envelope = loadEnvelope();

  const root = document.querySelector('#workbench');
  root.addEventListener('click', onClick);
  root.addEventListener('change', onChange);
  root.addEventListener('input', onInput);
  render();

  function enumValues(camelName, upperName, fallback) {
    const candidate = enums[camelName] || enums[upperName] || runtime.constants?.[camelName] || runtime.constants?.[upperName];
    return Array.isArray(candidate) && candidate.length ? candidate : fallback;
  }

  function escapeHtml(value) {
    return String(value ?? '').replace(/[&<>"']/g, (character) => ({
      '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
    })[character]);
  }

  function escapeAttribute(value) {
    return escapeHtml(value).replace(/`/g, '&#96;');
  }

  function unique(values) {
    return [...new Set((values || []).filter((value) => value !== null && value !== undefined && value !== ''))];
  }

  function asArray(value) {
    if (Array.isArray(value)) return value;
    if (value === null || value === undefined || value === '') return [];
    return [value];
  }

  function objectValues(value) {
    return value && typeof value === 'object' && !Array.isArray(value) ? Object.values(value) : [];
  }

  function display(value) {
    if (value === null || value === undefined) return 'absent';
    if (value === '') return 'vide';
    if (typeof value === 'object') return JSON.stringify(value);
    return String(value);
  }

  function friendly(value) {
    return String(value || '—').replaceAll('_', ' ').toLowerCase().replace(/^./, (letter) => letter.toUpperCase());
  }

  function normalizeNavigation(navigation, skills) {
    const result = [];
    for (const [index, sourceView] of navigation.entries()) {
      const view = sourceView || {};
      const id = String(view.id || view.classCode || view.scope || 'view-' + index);
      const trees = asArray(view.trees || view.skillTrees).map((tree, treeIndex) => ({
        id: String(tree.id || tree.key || id + '-tree-' + treeIndex),
        label: tree.label || tree.name || tree.treeName || 'Arbre ' + (treeIndex + 1),
        skillIds: asArray(tree.skillIds || tree.skills).map((item) => typeof item === 'string' ? item : item?.stableId).filter(Boolean),
      }));
      let skillIds = asArray(view.skillIds || view.skills).map((item) => typeof item === 'string' ? item : item?.stableId).filter(Boolean);
      if (!skillIds.length) skillIds = trees.flatMap((tree) => tree.skillIds);
      result.push({
        id,
        label: view.label || view.name || view.className || friendly(id),
        classCode: view.classCode || view.scope || id,
        trees,
        skillIds: unique(skillIds),
      });
    }
    if (!result.length) {
      const groups = new Map();
      for (const skill of skills) {
        const key = skill.classCode || skill.scope || 'technical';
        if (!groups.has(key)) groups.set(key, []);
        groups.get(key).push(skill.stableId);
      }
      for (const [id, skillIds] of groups) result.push({ id, label: friendly(id), classCode: id, trees: [], skillIds });
    }
    const represented = new Set(result.flatMap((view) => view.skillIds));
    const missing = skills.filter((skill) => !represented.has(skill.stableId)).map((skill) => skill.stableId);
    if (missing.length) result.push({ id: 'unclassified', label: 'Non classés', classCode: 'technical', trees: [], skillIds: missing });
    return result;
  }

  function proofStatusesOf(skill) {
    const statuses = [];
    statuses.push(...asArray(skill.evidence?.status || skill.evidence?.proofStatus || skill.proofStatus));
    for (const component of skill.components || []) {
      statuses.push(...asArray(component.proofStatus));
      for (const field of component.fields || []) statuses.push(...asArray(field.proofStatus));
    }
    return unique(statuses);
  }

  function portabilityCategoriesOf(value) {
    const portability = value?.portability ?? value;
    if (Array.isArray(portability)) return unique(portability.map((item) => typeof item === 'string' ? item : item?.category || item?.classification).filter(Boolean));
    if (typeof portability === 'string') return [portability];
    if (!portability || typeof portability !== 'object') return [];
    return unique([
      ...asArray(portability.categories),
      ...asArray(portability.classifications),
      ...asArray(portability.category),
      ...asArray(portability.classification),
    ].map((item) => typeof item === 'string' ? item : item?.category || item?.classification).filter(Boolean));
  }

  function isNativeRisk(skill) {
    const categories = portabilityCategoriesOf(skill);
    const proof = proofStatusesOf(skill);
    return categories.some((item) => ['NATIVE_FUNCTION_MISMATCH', 'NATIVE_UNPROVEN', 'NETWORK_OR_CLIENT_SERVER_RISK'].includes(item))
      || proof.includes('NATIVE_UNPROVEN');
  }

  function entryFor(skill, create) {
    const existing = envelope.entries?.[skill.stableId];
    if (existing || !create) return existing || {};
    const created = runtime.createEntry(skill);
    envelope.entries ||= {};
    envelope.entries[skill.stableId] = created;
    return created;
  }

  function reviewState(skill) {
    try {
      return runtime.entryState(report, skill, entryFor(skill, false)) || {};
    } catch (error) {
      return { required: !skill.readOnly, complete: false, reasons: [error.message] };
    }
  }

  function isComplete(skill) {
    if (skill.readOnly || skill.identical) return true;
    return reviewState(skill).complete === true;
  }

  function loadEnvelope() {
    const empty = runtime.createEmptyEnvelope(report);
    try {
      const raw = localStorage.getItem(storageKey);
      if (!raw) return empty;
      const candidate = JSON.parse(raw);
      const validated = validatedEnvelope(candidate);
      notifyLater('Décisions locales restaurées et validées.', 'ok');
      return validated;
    } catch (error) {
      ui.storageAvailable = storageUsable();
      notifyLater('État local ignoré : ' + error.message, 'warning');
      return empty;
    }
  }

  function validatedEnvelope(candidate) {
    const result = runtime.validateImport(report, candidate);
    if (result?.valid === false) throw new Error(asArray(result.errors).join('; ') || 'Enveloppe de décisions invalide');
    return result?.envelope || result?.value || result || candidate;
  }

  function storageUsable() {
    try {
      const key = storageKey + ':probe';
      localStorage.setItem(key, '1');
      localStorage.removeItem(key);
      return true;
    } catch {
      return false;
    }
  }

  function scheduleSave() {
    window.clearTimeout(ui.saveTimer);
    setSaveStatus('Modifications en attente…', 'pending');
    ui.saveTimer = window.setTimeout(() => {
      try {
        localStorage.setItem(storageKey, JSON.stringify(envelope));
        ui.storageAvailable = true;
        setSaveStatus('Sauvegardé localement', 'saved');
      } catch (error) {
        ui.storageAvailable = false;
        setSaveStatus('Autosave indisponible : exportez vos décisions', 'error');
        showNotice(error.message, 'error');
      }
    }, 240);
  }

  function setSaveStatus(text, status) {
    const target = document.querySelector('#save-status');
    if (target) {
      target.textContent = text;
      target.dataset.status = status;
    }
  }

  function notifyLater(text, type) {
    window.setTimeout(() => showNotice(text, type), 0);
  }

  function showNotice(text, type) {
    const target = document.querySelector('#notice');
    if (!target) return;
    target.textContent = text;
    target.className = 'notice ' + (type || 'info');
    target.hidden = false;
    window.clearTimeout(showNotice.timer);
    showNotice.timer = window.setTimeout(() => { target.hidden = true; }, 7000);
  }

  function activeView() {
    return navById.get(ui.activeViewId) || normalizedNavigation[0] || { id: 'all', label: 'Tous', trees: [], skillIds: report.skills.map((skill) => skill.stableId) };
  }

  function skillSearchText(skill) {
    return [
      skill.canonicalName, ...objectValues(skill.names), ...(skill.aliases || []), skill.classCode, skill.scope,
      skill.tree?.label, skill.tree?.name, skill.summary?.player, skill.summary?.text, skill.summary,
      ...(skill.mappingTypes || []), ...proofStatusesOf(skill), ...portabilityCategoriesOf(skill),
      ...(skill.components || []).flatMap((component) => [component.label, ...(component.fields || []).flatMap((field) => [field.label, field.header])]),
    ].filter((value) => typeof value !== 'object').join(' ').toLocaleLowerCase('fr');
  }

  function significant(skill) {
    return !skill.identical && (skill.status !== 'IDENTICAL' || (skill.components || []).some((component) => component.changed));
  }

  function matchesFilters(skill) {
    const filter = ui.filters;
    if (filter.query && !skillSearchText(skill).includes(filter.query.toLocaleLowerCase('fr'))) return false;
    if (filter.incomplete && isComplete(skill)) return false;
    if (filter.significant && !significant(skill)) return false;
    if (filter.portability && !portabilityCategoriesOf(skill).includes(filter.portability)) return false;
    if (filter.mapping && !(skill.mappingTypes || []).includes(filter.mapping)) return false;
    if (filter.proof && !proofStatusesOf(skill).includes(filter.proof)) return false;
    if (filter.decision && entryFor(skill, false).globalDecision !== filter.decision) return false;
    if (filter.collision === 'yes' && !(skill.collisionIds || []).length) return false;
    if (filter.collision === 'no' && (skill.collisionIds || []).length) return false;
    if (filter.nativeRisk === 'yes' && !isNativeRisk(skill)) return false;
    if (filter.nativeRisk === 'no' && isNativeRisk(skill)) return false;
    if (filter.pd2New === 'yes' && !skill.newPd2PlayerSkill) return false;
    if (filter.pd2New === 'no' && skill.newPd2PlayerSkill) return false;
    return true;
  }

  function visibleSkills() {
    const queryIsGlobal = Boolean(ui.filters.query);
    const ids = queryIsGlobal ? report.skills.map((skill) => skill.stableId) : activeView().skillIds;
    return unique(ids).map((id) => skillById.get(id)).filter(Boolean).filter(matchesFilters);
  }

  function progressFor(skills) {
    const required = skills.filter((skill) => !skill.readOnly && !skill.identical && reviewState(skill).required !== false);
    const complete = required.filter(isComplete).length;
    return { required: required.length, complete, remaining: required.length - complete, percent: required.length ? Math.round(complete * 100 / required.length) : 100 };
  }

  function dashboardStats(skills) {
    const progress = progressFor(skills);
    return {
      total: skills.length,
      identical: skills.filter((skill) => skill.identical).length,
      modified: skills.filter(significant).length,
      pd2Only: skills.filter((skill) => skill.newPd2PlayerSkill || (skill.mappingTypes || []).includes('PD2_ONLY_PLAYER_SKILL')).length,
      bkvOnly: skills.filter((skill) => skill.bkvinceOnlyPlayerSkill || (skill.mappingTypes || []).includes('BKV_ONLY_PLAYER_SKILL')).length,
      collisions: skills.filter((skill) => (skill.collisionIds || []).length).length,
      native: skills.filter(isNativeRisk).length,
      complete: progress.complete,
      remaining: progress.remaining,
      newCandidates: skills.filter((skill) => skill.newPd2PlayerSkill).length,
      prototypes: skills.filter((skill) => entryFor(skill, false).implementationStatus === 'SELECTED_FOR_PROTOTYPE').length,
      percent: progress.percent,
    };
  }

  function render() {
    const currentView = activeView();
    const allStats = dashboardStats(report.skills);
    const viewSkills = currentView.skillIds.map((id) => skillById.get(id)).filter(Boolean);
    const viewStats = dashboardStats(viewSkills);
    const skills = visibleSkills();
    root.innerHTML = renderHeader(allStats)
      + '<div id="notice" class="notice" hidden role="status" aria-live="polite"></div>'
      + '<div class="layout">'
      + renderNavigation()
      + '<main id="main-content" tabindex="-1">'
      + renderDashboard(currentView, viewStats)
      + renderActiveFilters(skills.length)
      + renderSkillGroups(skills, currentView)
      + '</main></div>';
  }

  function renderHeader(stats) {
    return '<header class="topbar">'
      + '<div class="title-row"><div><h1>PD2 Skills Merge Workbench</h1><p>Comparateur PD2 / BKVince / Vanilla D2R 3.2</p></div>'
      + '<div class="global-progress"><strong>' + stats.complete + ' / ' + (stats.complete + stats.remaining) + '</strong> décisions complètes'
      + '<progress max="100" value="' + stats.percent + '">' + stats.percent + '%</progress></div></div>'
      + '<p class="non-mutation"><strong>Prévisualisation seulement.</strong> Fonctionne directement sous <code>file://</code>. Aucune table gameplay, sauvegarde ou installation runtime ne peut être modifiée depuis ce document.</p>'
      + '<div class="primary-controls">'
      + '<label class="search"><span>Recherche globale</span><input id="global-search" type="search" value="' + escapeAttribute(ui.filters.query) + '" placeholder="Nom, alias, effet, formule…"></label>'
      + '<button type="button" data-action="next-incomplete">Skill suivant à décider</button>'
      + '<button type="button" data-action="expand-all">Développer tout</button>'
      + '<button type="button" data-action="collapse-all">Réduire tout</button>'
      + '<div class="segmented" role="group" aria-label="Mode de comparaison"><button type="button" data-mode="player" class="' + (ui.mode === 'player' ? 'active' : '') + '">Vue joueur</button><button type="button" data-mode="technical" class="' + (ui.mode === 'technical' ? 'active' : '') + '">Vue technique</button></div>'
      + '<span id="save-status" data-status="' + (ui.storageAvailable ? 'saved' : 'error') + '" role="status">' + (ui.storageAvailable ? 'Autosave actif' : 'Autosave indisponible') + '</span>'
      + '</div>' + renderFilters() + renderPersistenceToolbar() + '</header>';
  }

  function filterSelect(id, label, values, current, anyLabel) {
    return '<label><span>' + escapeHtml(label) + '</span><select data-filter="' + id + '"><option value="">' + escapeHtml(anyLabel || 'Tous') + '</option>'
      + values.map((value) => '<option value="' + escapeAttribute(value) + '"' + (current === value ? ' selected' : '') + '>' + escapeHtml(friendly(value)) + '</option>').join('')
      + '</select></label>';
  }

  function yesNoFilter(id, label, current) {
    return '<label><span>' + escapeHtml(label) + '</span><select data-filter="' + id + '"><option value="">Tous</option><option value="yes"' + (current === 'yes' ? ' selected' : '') + '>Avec</option><option value="no"' + (current === 'no' ? ' selected' : '') + '>Sans</option></select></label>';
  }

  function renderFilters() {
    return '<details class="filter-panel" open><summary>Filtres combinables</summary><div class="filters">'
      + '<label class="check"><input type="checkbox" data-filter="incomplete"' + (ui.filters.incomplete ? ' checked' : '') + '> Décisions incomplètes seulement</label>'
      + '<label class="check"><input type="checkbox" data-filter="significant"' + (ui.filters.significant ? ' checked' : '') + '> Différences significatives seulement</label>'
      + filterSelect('portability', 'Portabilité', PORTABILITY_CATEGORIES, ui.filters.portability)
      + filterSelect('mapping', 'Type de mapping', MAPPING_TYPES, ui.filters.mapping)
      + filterSelect('proof', 'Statut de preuve', PROOF_STATUSES, ui.filters.proof)
      + filterSelect('decision', 'Décision finale', GLOBAL_DECISIONS, ui.filters.decision)
      + yesNoFilter('collision', 'Collision', ui.filters.collision)
      + yesNoFilter('nativeRisk', 'Risque natif', ui.filters.nativeRisk)
      + yesNoFilter('pd2New', 'Nouveau skill PD2', ui.filters.pd2New)
      + '<button type="button" data-action="clear-filters">Effacer les filtres</button></div></details>';
  }

  function renderPersistenceToolbar() {
    return '<div class="persistence-toolbar" aria-label="Persistance et exports">'
      + '<button type="button" data-action="export-all">Exporter toutes les décisions</button>'
      + '<button type="button" data-action="export-complete">Exporter les décisions complètes seulement</button>'
      + '<button type="button" data-action="import">Importer JSON</button><input id="decision-file" type="file" accept="application/json" hidden>'
      + '<button type="button" class="danger" data-action="reset-global">Réinitialisation globale</button>'
      + '<span class="hash" title="Clé localStorage">comparisonHash ' + escapeHtml(String(report.comparisonHash).slice(0, 12)) + '…</span></div>';
  }

  function renderNavigation() {
    return '<aside class="sidebar"><nav aria-label="Classes et vues">'
      + normalizedNavigation.map((view) => {
        const skills = view.skillIds.map((id) => skillById.get(id)).filter(Boolean);
        const progress = progressFor(skills);
        return '<button type="button" data-view="' + escapeAttribute(view.id) + '" class="nav-view ' + (view.id === ui.activeViewId ? 'active' : '') + '"><span>' + escapeHtml(view.label) + '</span><small>' + progress.complete + '/' + progress.required + '</small></button>';
      }).join('')
      + '</nav><div id="tree-nav">' + renderTreeNavigation(activeView()) + '</div></aside>';
  }

  function renderTreeNavigation(view) {
    if (!view.trees.length) return '';
    return '<h2>Arbres</h2><nav aria-label="Arbres de skills">' + view.trees.map((tree) => '<button type="button" data-scroll-tree="' + escapeAttribute(tree.id) + '">' + escapeHtml(tree.label) + '</button>').join('') + '</nav>';
  }

  function metric(label, value, tone) {
    return '<div class="metric ' + (tone || '') + '"><strong>' + escapeHtml(value) + '</strong><span>' + escapeHtml(label) + '</span></div>';
  }

  function renderDashboard(view, stats) {
    const scopeLabel = view.label || 'Tous les skills';
    return '<section class="dashboard" aria-labelledby="dashboard-title"><div class="section-title"><div><p class="eyebrow">Tableau de bord</p><h2 id="dashboard-title">' + escapeHtml(scopeLabel) + '</h2></div>'
      + '<div class="dashboard-actions"><button type="button" data-action="export-class" data-view-id="' + escapeAttribute(view.id) + '">Exporter le dossier de révision de cette classe</button><button type="button" class="danger" data-action="reset-class" data-view-id="' + escapeAttribute(view.id) + '">Réinitialiser cette classe</button></div></div>'
      + '<div class="metrics">'
      + metric('Skills', stats.total) + metric('Identiques', stats.identical, 'ok') + metric('Modifiés', stats.modified)
      + metric('Propres à PD2', stats.pd2Only) + metric('Propres à BKVince', stats.bkvOnly)
      + metric('Collisions', stats.collisions, stats.collisions ? 'warning' : '') + metric('Preuves natives requises', stats.native, stats.native ? 'danger' : '')
      + metric('Décisions terminées', stats.complete, 'ok') + metric('Décisions restantes', stats.remaining, stats.remaining ? 'warning' : 'ok')
      + metric('Nouveaux candidats', stats.newCandidates) + metric('Sélectionnés pour prototype', stats.prototypes)
      + '</div><div class="scope-bulk"><strong>Actions en lot — classe</strong>' + bulkControls(view.skillIds, 'class') + '</div></section>';
  }

  function renderActiveFilters(count) {
    const active = [];
    if (ui.filters.query) active.push('recherche « ' + ui.filters.query + ' »');
    if (ui.filters.incomplete) active.push('incomplètes');
    if (ui.filters.significant) active.push('différences significatives');
    for (const key of ['portability', 'mapping', 'proof', 'decision', 'collision', 'nativeRisk', 'pd2New']) if (ui.filters[key]) active.push(key + ': ' + ui.filters[key]);
    return '<div class="result-summary"><strong>' + count + '</strong> skills affichés' + (active.length ? ' · ' + escapeHtml(active.join(' · ')) : '') + '</div>';
  }

  function treeForSkill(skill, view) {
    const declared = skill.tree || {};
    const matching = view.trees.find((tree) => tree.skillIds.includes(skill.stableId));
    return {
      id: matching?.id || declared.id || declared.key || 'other',
      label: matching?.label || declared.label || declared.name || declared.treeName || 'Autres skills',
      page: declared.page,
      row: declared.row,
      column: declared.column,
    };
  }

  function renderSkillGroups(skills, view) {
    if (!skills.length) return '<p class="empty-state">Aucun skill ne correspond à cette combinaison de filtres.</p>';
    const groups = new Map();
    for (const skill of skills) {
      const tree = treeForSkill(skill, view);
      if (!groups.has(tree.id)) groups.set(tree.id, { tree, skills: [] });
      groups.get(tree.id).skills.push(skill);
    }
    return '<div class="skill-groups">' + [...groups.values()].map(({ tree, skills: treeSkills }) => '<section class="tree" id="tree-' + escapeAttribute(tree.id) + '"><div class="tree-heading"><div><p class="eyebrow">Arbre de skills</p><h2>' + escapeHtml(tree.label) + '</h2></div><div class="tree-bulk"><span>Actions en lot — arbre</span>' + bulkControls(treeSkills.map((skill) => skill.stableId), 'tree') + '</div></div>' + treeSkills.map(renderSkillCard).join('') + '</section>').join('') + '</div>';
  }

  function bulkControls(skillIds, scope) {
    const encodedIds = escapeAttribute(skillIds.join('|'));
    return '<div class="bulk-controls" data-bulk-scope="' + scope + '"><select data-bulk-mode><option value="fill">Seulement les décisions indécises</option><option value="replace">Remplacer toutes les décisions (confirmation)</option></select>'
      + '<button type="button" data-bulk="KEEP_BKVINCE" data-skill-ids="' + encodedIds + '">Garder BKVince</button>'
      + '<button type="button" data-bulk="ADOPT_PD2" data-skill-ids="' + encodedIds + '">Adopter PD2 non protégé</button>'
      + '<button type="button" data-bulk="DISCUSS" data-skill-ids="' + encodedIds + '">Tout discuter</button>'
      + '<button type="button" data-bulk="CLEAR_UNRESOLVED" data-skill-ids="' + encodedIds + '">Vider uniquement les décisions indécises</button></div>';
  }

  function renderSkillCard(skill) {
    const current = entryFor(skill, false);
    const state = reviewState(skill);
    const expanded = ui.expanded.has(skill.stableId);
    const tree = treeForSkill(skill, activeView());
    const names = SOURCE_KEYS.map((source) => skill.names?.[source] || nodeFor(skill, source)?.name || '—');
    const ordinals = SOURCE_KEYS.map((source) => skill.ordinals?.[source] ?? nodeFor(skill, source)?.ordinal ?? '—');
    const progressLabel = skill.readOnly || skill.identical ? 'Auto-résolu · lecture seule' : state.complete ? 'Décision complète' : 'Décision incomplète';
    return '<article class="skill-card ' + (state.complete ? 'complete' : 'incomplete') + (skill.readOnly ? ' read-only' : '') + '" id="skill-' + escapeAttribute(skill.stableId) + '" data-skill-id="' + escapeAttribute(skill.stableId) + '">'
      + '<div class="skill-card-heading"><button type="button" class="expander" data-action="toggle-skill" data-skill-id="' + escapeAttribute(skill.stableId) + '" aria-expanded="' + expanded + '" aria-controls="body-' + escapeAttribute(skill.stableId) + '"><span class="chevron" aria-hidden="true">' + (expanded ? '▾' : '▸') + '</span><span><strong>' + escapeHtml(skill.canonicalName) + '</strong><small>' + escapeHtml(skill.classCode || skill.scope || 'classless') + ' · ' + escapeHtml(tree.label) + ' · position ' + escapeHtml(positionLabel(tree)) + '</small></span></button>'
      + '<div class="decision-progress ' + (state.complete ? 'done' : '') + '">' + escapeHtml(progressLabel) + '</div></div>'
      + '<div class="identity-strip"><div><span>Vanilla</span><strong>' + escapeHtml(names[0]) + '</strong><small>ordinal ' + escapeHtml(ordinals[0]) + '</small></div><div><span>BKVince</span><strong>' + escapeHtml(names[1]) + '</strong><small>ordinal ' + escapeHtml(ordinals[1]) + '</small></div><div><span>PD2</span><strong>' + escapeHtml(names[2]) + '</strong><small>ordinal ' + escapeHtml(ordinals[2]) + '</small></div></div>'
      + '<div class="chips">' + (skill.mappingTypes || []).map((value) => chip(value, 'mapping')).join('') + chip(skill.status || (skill.identical ? 'IDENTICAL' : 'MODIFIED'), 'status') + portabilityCategoriesOf(skill).map((value) => chip(value, 'portability')).join('') + proofStatusesOf(skill).map((value) => chip(value, 'proof')).join('') + ((skill.collisionIds || []).length ? chip((skill.collisionIds || []).length + ' collision(s)', 'collision') : '') + (skill.newPd2PlayerSkill ? chip('Nouveau skill PD2', 'new') : '') + '</div>'
      + '<p class="player-summary">' + escapeHtml(playerSummary(skill)) + '</p>'
      + '<div class="card-decision">' + renderGlobalDecision(skill, current, state) + '</div>'
      + (expanded ? '<div class="skill-card-body" id="body-' + escapeAttribute(skill.stableId) + '">' + renderExpandedSkill(skill, current, state) + '</div>' : '')
      + '</article>';
  }

  function positionLabel(tree) {
    const values = [tree.page !== undefined ? 'page ' + tree.page : '', tree.row !== undefined ? 'rangée ' + tree.row : '', tree.column !== undefined ? 'colonne ' + tree.column : ''].filter(Boolean);
    return values.join(', ') || 'non documentée';
  }

  function nodeFor(skill, source) {
    const nodeId = skill.nodeIds?.[source];
    return nodeId ? nodeById.get(nodeId) : null;
  }

  function chip(value, kind) {
    return '<span class="chip ' + escapeAttribute(kind || '') + '">' + escapeHtml(friendly(value)) + '</span>';
  }

  function playerSummary(skill) {
    if (typeof skill.summary === 'string') return skill.summary;
    return skill.summary?.player || skill.summary?.playerFacing || skill.summary?.text || skill.summary?.short || (skill.identical ? 'Les trois versions sont identiques sur les composantes gouvernées.' : 'Résumé joueur indisponible; consulter les composantes et les cellules sources.');
  }

  function optionList(values, selected, blankLabel) {
    return '<option value="">' + escapeHtml(blankLabel || 'À décider') + '</option>' + values.map((value) => '<option value="' + escapeAttribute(value) + '"' + (selected === value ? ' selected' : '') + '>' + escapeHtml(friendly(value)) + '</option>').join('');
  }

  function renderGlobalDecision(skill, current, state) {
    if (skill.readOnly || skill.identical) return '<div class="read-only-banner"><strong>Lecture seule.</strong> Ce skill identique est auto-résolu et aucune décision n’est requise.</div>';
    const reasons = asArray(state.reasons || state.errors);
    return '<label><span>Décision globale de Vincent</span><select data-global-decision data-skill-id="' + escapeAttribute(skill.stableId) + '">' + optionList(GLOBAL_DECISIONS, current.globalDecision) + '</select></label>'
      + (reasons.length ? '<details class="validation-errors"><summary>' + reasons.length + ' point(s) incomplet(s)</summary><ul>' + reasons.map((reason) => '<li>' + escapeHtml(typeof reason === 'string' ? reason : reason.message || JSON.stringify(reason)) + '</li>').join('') + '</ul></details>' : '');
  }

  function renderExpandedSkill(skill, current, state) {
    return '<div class="card-actions"><button type="button" data-action="copy-briefing" data-skill-id="' + escapeAttribute(skill.stableId) + '">Copier le briefing du skill</button><button type="button" data-action="export-skill-md" data-skill-id="' + escapeAttribute(skill.stableId) + '">Exporter ce skill en Markdown</button><button type="button" data-action="export-skill-json" data-skill-id="' + escapeAttribute(skill.stableId) + '">Exporter ce skill en JSON</button><button type="button" class="danger" data-action="reset-skill" data-skill-id="' + escapeAttribute(skill.stableId) + '">Réinitialiser ce skill</button></div>'
      + '<section class="comparison-section"><h3>Comparaison à trois voies</h3>' + renderTriWay(skill) + '</section>'
      + renderCurves(skill)
      + (skill.newPd2PlayerSkill ? renderNewSkillPlan(skill, current) : '')
      + '<section><div class="section-title"><h3>Composantes de gameplay</h3><span>Décisions par composante et par champ</span></div>' + (skill.components || []).map((component) => renderComponent(skill, component, current)).join('') + '</section>'
      + renderDependencies(skill)
      + renderPortability(skill)
      + renderCollisions(skill)
      + renderDocumentation(skill)
      + renderTechnicalRaw(skill)
      + (skill.readOnly ? '' : '<section class="scope-bulk"><strong>Actions en lot — skill</strong>' + bulkControls([skill.stableId], 'skill') + '</section>' + renderNotes(skill, current, state));
  }

  function renderTriWay(skill) {
    const headlineRows = playerMetrics(skill);
    if (!headlineRows.length) {
      return '<div class="tri-way">' + SOURCE_KEYS.map((source) => '<div><h4>' + SOURCE_LABELS[source] + '</h4><p>' + escapeHtml(skill.summary?.[source] || nodeFor(skill, source)?.name || 'Source absente') + '</p></div>').join('') + '</div>';
    }
    return '<div class="table-wrap"><table class="comparison-table"><caption>Valeurs joueur comparées</caption><thead><tr><th>Comportement</th>' + SOURCE_KEYS.map((source) => '<th>' + SOURCE_LABELS[source] + '</th>').join('') + '</tr></thead><tbody>'
      + headlineRows.map((row) => '<tr><th>' + escapeHtml(row.label) + '</th>' + SOURCE_KEYS.map((source) => '<td>' + escapeHtml(display(row.values?.[source])) + '</td>').join('') + '</tr>').join('')
      + '</tbody></table></div>';
  }

  function playerMetrics(skill) {
    const direct = skill.playerComparison || skill.playerMetrics || skill.summary?.metrics;
    if (Array.isArray(direct)) return direct;
    if (direct && typeof direct === 'object') return Object.entries(direct).map(([id, value]) => ({ id, label: value.label || friendly(id), values: value.values || value }));
    const rows = [];
    for (const component of skill.components || []) for (const field of component.fields || []) {
      if (!field.changed || field.technicalOnly) continue;
      rows.push({ label: field.label || field.header || field.id, values: field.displayValues || field.values || {} });
      if (rows.length >= 12) return rows;
    }
    return rows;
  }

  function scenariosOf(skill) {
    const curves = skill.curves || {};
    if (Array.isArray(curves.scenarios)) return curves.scenarios;
    if (curves.scenarios && typeof curves.scenarios === 'object') return Object.entries(curves.scenarios).map(([id, value]) => ({ id, label: value.label || friendly(id), ...value }));
    if (curves.standard) return [{ id: 'standard', label: 'Sans synergie', ...curves.standard }];
    if (Array.isArray(curves)) return [{ id: 'standard', label: 'Sans synergie', series: curves }];
    return [];
  }

  function seriesOfScenario(scenario) {
    if (Array.isArray(scenario.series)) return scenario.series;
    const metrics = scenario.metrics || scenario.values || {};
    return Object.entries(metrics).map(([id, value]) => ({ id, label: value.label || friendly(id), values: value.values || value }));
  }

  function renderCurves(skill) {
    const scenarios = scenariosOf(skill);
    if (!scenarios.length) return '<section><h3>Courbes et simulation</h3><p class="empty-inline">Aucune courbe numérique gouvernée. Les dépendances symboliques, malformées ou natives ne sont pas extrapolées.</p></section>';
    const selectedId = ui.scenario.get(skill.stableId) || scenarios[0].id || 'standard';
    const scenario = scenarios.find((item) => item.id === selectedId) || scenarios[0];
    const series = seriesOfScenario(scenario);
    const levels = scenario.levels || report.levels || [1, 5, 10, 20, 30, 40];
    return '<section class="curves"><div class="section-title"><div><h3>Courbes et simulation</h3><p>Niveau effectif L; hard points B=min(L,maxlvl). Une valeur non prouvée reste symbolique.</p></div><label><span>Scénario</span><select data-scenario data-skill-id="' + escapeAttribute(skill.stableId) + '">' + scenarios.map((item) => '<option value="' + escapeAttribute(item.id || 'standard') + '"' + (item === scenario ? ' selected' : '') + '>' + escapeHtml(item.label || friendly(item.id)) + '</option>').join('') + '</select></label></div>'
      + (scenario.description ? '<p>' + escapeHtml(scenario.description) + '</p>' : '')
      + renderSynergyInputs(skill, scenario)
      + '<div class="charts">' + series.map((item, index) => renderCurve(skill, item, levels, index)).join('') + '</div>'
      + (scenario.symbolic?.length ? '<div class="symbolic"><strong>Valeurs conservées symboliquement</strong><ul>' + scenario.symbolic.map((item) => '<li>' + escapeHtml(typeof item === 'string' ? item : item.label + ': ' + item.value) + '</li>').join('') + '</ul></div>' : '') + '</section>';
  }

  function renderSynergyInputs(skill, scenario) {
    const inputs = asArray(scenario.synergyInputs);
    if (!inputs.length) return '';
    return '<fieldset class="synergy-inputs"><legend>Hard points de synergies</legend><p>Ces entrées documentent le scénario; une formule symbolique ou non prouvée ne devient jamais numérique.</p><div class="filters">'
      + inputs.map((input) => {
        const id = input.id || input.skill || input.name;
        const key = skill.stableId + '::' + (scenario.id || 'custom') + '::' + id;
        const value = ui.synergyInputs.has(key) ? ui.synergyInputs.get(key) : (input.hardPoints ?? input.defaultHardPoints ?? 0);
        return '<label><span>' + escapeHtml(input.skill || input.name || id) + '</span><input type="number" min="0" max="' + escapeAttribute(input.maximum ?? 20) + '" value="' + escapeAttribute(value) + '" data-synergy-input data-skill-id="' + escapeAttribute(skill.stableId) + '" data-scenario-id="' + escapeAttribute(scenario.id || 'custom') + '" data-input-id="' + escapeAttribute(id) + '"></label>';
      }).join('') + '</div></fieldset>';
  }

  function sourceSeries(metric) {
    const values = metric.values || {};
    if (Array.isArray(values)) return [{ source: metric.source || 'comparison', values }];
    return SOURCE_KEYS.map((source) => ({ source, values: Array.isArray(values[source]) ? values[source] : [] })).filter((item) => item.values.length);
  }

  function renderCurve(skill, metric, levels, index) {
    const sourceSeriesValues = sourceSeries(metric);
    const numeric = sourceSeriesValues.flatMap((item) => item.values.filter((value) => Number.isFinite(Number(value))).map(Number));
    const chartId = 'chart-' + safeDomId(skill.stableId) + '-' + index;
    if (!numeric.length) return '<div class="chart-card"><h4>' + escapeHtml(metric.label || metric.id) + '</h4><p class="symbolic-value">' + escapeHtml(display(metric.formula || metric.values || 'SYMBOLIC')) + '</p></div>';
    const min = Math.min(...numeric);
    const max = Math.max(...numeric);
    const width = 440;
    const height = 180;
    const padding = 24;
    const color = { vanilla32: '#a7b0bd', bkvince: '#6fc7ff', pd2: '#ffbd66', comparison: '#b795ff' };
    const pointsFor = (values) => values.map((raw, position) => {
      const value = Number(raw);
      if (!Number.isFinite(value)) return null;
      const x = padding + position * ((width - padding * 2) / Math.max(1, levels.length - 1));
      const y = height - padding - ((value - min) / Math.max(1, max - min)) * (height - padding * 2);
      return x.toFixed(1) + ',' + y.toFixed(1);
    }).filter(Boolean).join(' ');
    return '<figure class="chart-card"><figcaption id="' + chartId + '-title">' + escapeHtml(metric.label || metric.id) + '</figcaption><svg viewBox="0 0 ' + width + ' ' + height + '" role="img" aria-labelledby="' + chartId + '-title ' + chartId + '-desc"><desc id="' + chartId + '-desc">Courbe locale de ' + escapeHtml(metric.label || metric.id) + ' aux niveaux ' + escapeHtml(levels.join(', ')) + '.</desc><path class="axis" d="M24 10 V156 H430" />'
      + sourceSeriesValues.map((item) => '<polyline fill="none" stroke="' + (color[item.source] || '#b795ff') + '" stroke-width="3" points="' + pointsFor(item.values) + '"><title>' + escapeHtml(SOURCE_LABELS[item.source] || item.source) + '</title></polyline>').join('')
      + '</svg><div class="chart-legend">' + sourceSeriesValues.map((item) => '<span style="--legend:' + (color[item.source] || '#b795ff') + '">' + escapeHtml(SOURCE_LABELS[item.source] || item.source) + '</span>').join('') + '</div>'
      + '<div class="table-wrap"><table class="curve-table"><caption>Données accessibles de ' + escapeHtml(metric.label || metric.id) + '</caption><thead><tr><th>Source</th>' + levels.map((level) => '<th>L' + escapeHtml(level) + '</th>').join('') + '</tr></thead><tbody>' + sourceSeriesValues.map((item) => '<tr><th>' + escapeHtml(SOURCE_LABELS[item.source] || item.source) + '</th>' + levels.map((level, position) => '<td>' + escapeHtml(display(item.values[position])) + '</td>').join('') + '</tr>').join('') + '</tbody></table></div></figure>';
  }

  function safeDomId(value) {
    return String(value).replace(/[^a-zA-Z0-9_-]/g, '-');
  }

  function renderNewSkillPlan(skill, current) {
    const plan = skill.newSkillPlan || {};
    const gateClosed = Boolean(current.newSkillLineDecision);
    const facts = [
      ['Classe prévue', plan.classCode || skill.classCode], ['Emplacement PD2', plan.treePosition || positionLabel(skill.tree || {})],
      ['Ordinal PD2', skill.ordinals?.pd2], ['Occupant BKVince du même ordinal', plan.currentOccupantAtPd2Ordinal || plan.bkvinceOrdinalOccupant || plan.currentOccupant],
      ['Cible append-only proposée', plan.proposedTargetOrdinal ?? plan.appendOnlyOrdinal], ['Plan de remapping', plan.remappingPlan],
      ['Tests nécessaires', plan.testsRequired || plan.tests || plan.testPlan],
    ];
    return '<section class="new-skill-plan"><div class="section-title"><div><p class="eyebrow">Nouveau skill joueur PD2</p><h3>Gate de ligne append-only</h3></div>' + chip(gateClosed ? 'Gate décidé' : 'Décision de ligne obligatoire', gateClosed ? 'ok' : 'warning') + '</div>'
      + '<label><span>Décision de ligne</span><select data-line-decision data-skill-id="' + escapeAttribute(skill.stableId) + '">' + optionList(LINE_DECISIONS, current.newSkillLineDecision, 'Décision obligatoire avant les champs') + '</select></label>'
      + '<p class="warning-box">Cette cible est une prévisualisation calculée après le dernier ordinal réel. Aucune insertion, ligne ou localisation n’est créée par le Workbench.</p>'
      + '<dl class="facts">' + facts.map(([label, value]) => '<div><dt>' + escapeHtml(label) + '</dt><dd>' + escapeHtml(display(value)) + '</dd></div>').join('') + '</dl>'
      + renderListGrid('Fermeture de dépendances', [
        ['Missiles requis', plan.missilesRequired || plan.missiles], ['States requis', plan.statesRequired || plan.states], ['skilldesc requis', plan.skilldescRequired || plan.skilldesc], ['Strings requises', plan.stringsRequired || plan.localizations || plan.strings],
        ['ItemStatCost / Properties', plan.itemStatCostRequired || plan.itemStatCost || plan.propertiesRequired || plan.properties], ['PetType requis', plan.pettypesRequired || plan.pettypeRequired || plan.pettype], ['Summons / monstats', plan.summonsRequired || plan.summons || plan.monstatsRequired || plan.monstats], ['Fonctions natives', plan.nativeFunctions || skill.portability?.divergentFunctions], ['Consommateurs', plan.consumers || skill.consumers],
      ]) + '</section>';
  }

  function renderListGrid(title, values) {
    return '<div class="list-grid"><h4>' + escapeHtml(title) + '</h4>' + values.map(([label, value]) => '<div><strong>' + escapeHtml(label) + '</strong><span>' + escapeHtml(asArray(value).map((item) => display(item)).join(', ') || 'aucun documenté') + '</span></div>').join('') + '</div>';
  }

  function componentChoice(current, component) {
    return current.componentDecisions?.[component.id] || {};
  }

  function fieldChoice(skill, current, component, field) {
    if (typeof runtime.resolveFieldChoice === 'function') {
      try { return runtime.resolveFieldChoice(skill, current, component, field) || {}; } catch { /* explicit choice below */ }
    }
    return current.fieldDecisions?.[field.id] || componentChoice(current, component) || {};
  }

  function renderComponent(skill, component, current) {
    const choice = componentChoice(current, component);
    const changedFields = (component.fields || []).filter((field) => field.changed).length;
    const lineGateOpen = !skill.newPd2PlayerSkill || Boolean(current.newSkillLineDecision);
    return '<details class="component" ' + (component.changed ? 'open' : '') + '><summary><span><strong>' + escapeHtml(component.label || friendly(component.id)) + '</strong><small>' + changedFields + ' champ(s) modifié(s)</small></span><span>' + chip(component.proofStatus || 'EXACT_TABLE', 'proof') + portabilityCategoriesOf(component).map((value) => chip(value, 'portability')).join('') + '</span></summary>'
      + '<div class="component-body">'
      + (skill.readOnly ? '' : '<label class="component-decision"><span>Décision de composante</span><select data-component-decision data-skill-id="' + escapeAttribute(skill.stableId) + '" data-component-id="' + escapeAttribute(component.id) + '"' + (!lineGateOpen ? ' disabled' : '') + '>' + optionList(COMPONENT_DECISIONS, choice.decision, lineGateOpen ? 'À décider' : 'Décision de ligne requise') + '</select></label>' + renderChoiceDetails('component', skill, component, null, choice))
      + '<div class="field-list">' + (component.fields || []).map((field) => renderField(skill, component, field, current, lineGateOpen)).join('') + '</div></div></details>';
  }

  function renderField(skill, component, field, current, lineGateOpen) {
    const explicit = current.fieldDecisions?.[field.id] || {};
    const effective = fieldChoice(skill, current, component, field);
    const decision = effective.decision || '';
    const values = field.displayValues || field.values || {};
    return '<div class="field-row ' + (field.protected ? 'protected' : '') + (field.changed ? ' changed' : '') + '"><div class="field-title"><div><strong>' + escapeHtml(field.label || field.header || field.id) + '</strong><small>' + escapeHtml(field.table || 'skills.txt') + ' · ' + escapeHtml(field.header || field.id) + '</small></div><div>' + (field.protected ? chip('Champ protégé', 'protected') : '') + chip(field.proofStatus || component.proofStatus || 'EXACT_TABLE', 'proof') + '</div></div>'
      + '<div class="field-values">' + SOURCE_KEYS.map((source) => '<div><span>' + SOURCE_LABELS[source] + '</span><code>' + escapeHtml(display(values[source])) + '</code></div>').join('') + '</div>'
      + (field.formula ? '<div class="formula"><strong>Formule source</strong><code>' + escapeHtml(display(field.formula.raw || field.formula.expression || field.formula)) + '</code><span>' + escapeHtml(field.formula.status || field.proofStatus || '') + '</span></div>' : '')
      + (skill.readOnly ? '' : '<div class="field-decision"><label><span>Décision de champ</span><select data-field-decision data-skill-id="' + escapeAttribute(skill.stableId) + '" data-component-id="' + escapeAttribute(component.id) + '" data-field-id="' + escapeAttribute(field.id) + '"' + (!lineGateOpen ? ' disabled' : '') + '>' + optionList(COMPONENT_DECISIONS, explicit.decision, decision && !explicit.decision ? 'Hérité : ' + friendly(decision) : lineGateOpen ? 'Hériter de la composante' : 'Décision de ligne requise') + '</select></label>' + renderChoiceDetails('field', skill, component, field, explicit) + '</div>')
      + (field.protected ? '<div class="protection-warning"><strong>Modification protégée</strong><ul>' + asArray(field.protectionReasons).map((reason) => '<li>' + escapeHtml(reason) + '</li>').join('') + '</ul></div>' : '') + '</div>';
  }

  function renderChoiceDetails(level, skill, component, field, choice) {
    const decision = choice?.decision;
    const identifiers = ' data-choice-level="' + level + '" data-skill-id="' + escapeAttribute(skill.stableId) + '" data-component-id="' + escapeAttribute(component.id) + '"' + (field ? ' data-field-id="' + escapeAttribute(field.id) + '"' : '');
    let result = '';
    if (decision === 'CUSTOM') {
      result += '<div class="custom-fields"><label><span>Valeur ou formule CUSTOM *</span><textarea data-choice-property="customValue"' + identifiers + ' placeholder="Valeur ou formule explicite">' + escapeHtml(choice.customValue || '') + '</textarea></label><label><span>Justification *</span><textarea data-choice-property="justification"' + identifiers + ' placeholder="Pourquoi cette valeur ?">' + escapeHtml(choice.justification || '') + '</textarea></label><label><span>Objectif de gameplay</span><textarea data-choice-property="gameplayObjective"' + identifiers + '>' + escapeHtml(choice.gameplayObjective || '') + '</textarea></label><label><span>Plan de test</span><textarea data-choice-property="testPlan"' + identifiers + '>' + escapeHtml(choice.testPlan || '') + '</textarea></label></div>';
    }
    const protectedTarget = field?.protected && ['ADOPT_PD2', 'CUSTOM'].includes(decision);
    if (protectedTarget) {
      const override = choice.protectedOverride || {};
      result += '<fieldset class="override-fields"><legend>Override protégé obligatoire</legend><label class="check"><input type="checkbox" data-override-approved' + identifiers + (override.approved ? ' checked' : '') + '> J’autorise explicitement cette exception</label><label><span>Justification *</span><textarea data-override-property="justification"' + identifiers + '>' + escapeHtml(override.justification || '') + '</textarea></label><label><span>Preuve reconnue *</span><select data-override-property="acknowledgedProofStatus"' + identifiers + '>' + optionList(PROOF_STATUSES, override.acknowledgedProofStatus) + '</select></label>'
        + ((field.proofStatus === 'NATIVE_UNPROVEN' || isNativeRisk(skill)) ? '<label class="check"><input type="checkbox" data-override-property="nativeRiskAccepted"' + identifiers + (override.nativeRiskAccepted ? ' checked' : '') + '> J’accepte explicitement le risque natif non prouvé</label>' : '')
        + (field.proofStatus === 'MALFORMED_SOURCE' ? '<label><span>Résolution gouvernée de la formule *</span><textarea data-override-property="malformedResolution"' + identifiers + '>' + escapeHtml(override.malformedResolution || '') + '</textarea></label>' : '') + '</fieldset>';
    }
    return result;
  }

  function renderDependencies(skill) {
    const dependencies = skill.dependencies || [];
    const consumers = skill.consumers || [];
    return '<section class="two-column"><div><h3>Dépendances et fermeture</h3>' + (dependencies.length ? '<ul class="structured-list">' + dependencies.map(renderDependencyItem).join('') + '</ul>' : '<p class="empty-inline">Aucune dépendance déclarée.</p>') + '</div><div><h3>Consommateurs</h3>' + (consumers.length ? '<ul class="structured-list">' + consumers.map((consumer) => '<li><details><summary><strong>' + escapeHtml(consumer.label || consumer.name || consumer.key || consumer.type || 'Consommateur') + '</strong> · ' + escapeHtml(consumer.table || consumer.reference || consumer.ordinal || '') + '</summary><pre>' + escapeHtml(JSON.stringify(consumer, null, 2)) + '</pre></details></li>').join('') + '</ul>' : '<p class="empty-inline">Aucun consommateur déclaré.</p>') + '</div></section>';
  }

  function renderDependencyItem(dependency) {
    const facts = asArray(dependency.facts);
    const title = dependency.label || dependency.name || dependency.key || dependency.id || dependency.table || 'Dépendance';
    const status = dependency.status || dependency.proofStatus || dependency.provenance || 'statut non documenté';
    const factTable = facts.length ? '<div class="table-wrap"><table><caption>Cellules liées gouvernées</caption><thead><tr><th>Champ</th>' + SOURCE_KEYS.map((source) => '<th>' + SOURCE_LABELS[source] + '</th>').join('') + '<th>Preuve</th></tr></thead><tbody>' + facts.map((fact) => '<tr><th>' + escapeHtml(fact.label || fact.header || fact.id) + '</th>' + SOURCE_KEYS.map((source) => '<td><code>' + escapeHtml(display(fact.values?.[source])) + '</code></td>').join('') + '<td>' + escapeHtml(fact.proofStatus || 'EXACT_TABLE') + '</td></tr>').join('') + '</tbody></table></div>' : '';
    return '<li><details><summary><strong>' + escapeHtml(title) + '</strong> · <span>' + escapeHtml(status) + '</span>' + (dependency.closed === false ? chip('Non fermée', 'danger') : '') + '</summary>' + factTable + '<pre>' + escapeHtml(JSON.stringify({ ...dependency, facts: undefined }, null, 2)) + '</pre></details></li>';
  }

  function renderPortability(skill) {
    const portability = skill.portability || {};
    const items = [
      ['Raisons', portability.reasons], ['Tables nécessaires', portability.tables || portability.requiredTables],
      ['Dépendances manquantes', portability.missingDependencies], ['Collisions', portability.collisions],
      ['Fonctions divergentes', portability.divergentFunctions || portability.nativeFunctions], ['Risque de sauvegarde', portability.saveRisk],
      ['Risque client/serveur', portability.networkRisk || portability.clientServerRisk], ['Effort estimé', portability.effort || portability.estimatedEffort],
      ['Preuve encore requise', portability.requiredProof || portability.missingProof],
    ];
    return '<section class="portability"><h3>Portabilité explicable</h3><div class="chips">' + portabilityCategoriesOf(skill).map((category) => chip(category, 'portability')).join('') + '</div><dl class="facts">' + items.map(([label, value]) => '<div><dt>' + escapeHtml(label) + '</dt><dd>' + escapeHtml(asArray(value).map((item) => display(item)).join(', ') || 'aucun documenté') + '</dd></div>').join('') + '</dl></section>';
  }

  function renderCollisions(skill) {
    const collisions = (skill.collisionIds || []).map((id) => collisionById.get(id) || { id });
    if (!collisions.length) return '';
    return '<section class="collisions"><h3>Collisions et remplacements — Aucune fusion automatique</h3><div class="collision-grid">' + collisions.map((collision) => '<article><strong>' + escapeHtml(collision.label || collision.type || collision.id) + '</strong><p>' + escapeHtml(collision.summary || collision.reason || 'Occupation concurrente d’un ordinal runtime.') + '</p><pre>' + escapeHtml(JSON.stringify(collision, null, 2)) + '</pre></article>').join('') + '</div></section>';
  }

  function renderDocumentation(skill) {
    const documents = skill.documentation || [];
    if (!documents.length) return '<section><h3>Références Wiki PD2 épinglées</h3><p class="empty-inline">UNMAPPED — aucune affirmation documentaire exacte reliée à ce skill.</p></section>';
    return '<section><h3>Références Wiki PD2 épinglées</h3><div class="documentation">' + documents.map((document) => '<article><div>' + chip(document.status || 'UNMAPPED', 'proof') + '<strong>' + escapeHtml(document.section || document.title || 'Section non mappée') + '</strong></div><p>' + escapeHtml(document.summary || 'Aucun résumé.') + '</p><small>Révision ' + escapeHtml(document.revision || report.documentation?.revision || 'inconnue') + ' · saison ' + escapeHtml(document.season || report.documentation?.season || 'inconnue') + '</small><p class="warning-box">' + escapeHtml(document.status === 'TABLE_ONLY' ? 'Fait prouvé par les tables gouvernées; aucune correspondance Wiki exacte n’est revendiquée.' : document.status === 'DOCUMENTED' ? 'Référence documentaire mappée; elle ne constitue pas une preuve native.' : 'Référence contextuelle non mappée aux cellules.') + '</p>' + (document.url ? '<a href="' + escapeAttribute(document.url) + '" target="_blank" rel="noopener noreferrer">Consulter la référence épinglée</a>' : '') + '</article>').join('') + '</div></section>';
  }

  function renderTechnicalRaw(skill) {
    if (ui.mode !== 'technical') return '';
    const rawNodes = Object.fromEntries(SOURCE_KEYS.map((source) => [source, nodeFor(skill, source)]));
    return '<section class="technical"><h3>Données techniques brutes</h3><p>Les cellules, formules, fonctions moteur et lignes liées restent exactement celles de l’oracle.</p><details><summary>Nœuds physiques et cellules brutes</summary><pre>' + escapeHtml(JSON.stringify(rawNodes, null, 2)) + '</pre></details><details><summary>Objet canonique du skill</summary><pre>' + escapeHtml(JSON.stringify(skill, null, 2)) + '</pre></details></section>';
  }

  function renderNotes(skill, current, state) {
    const notes = current.notes || {};
    return '<section class="review-notes"><h3>Décision, notes et statut d’implantation</h3><div class="notes-grid">'
      + noteField(skill, 'general', 'Notes générales', notes.general)
      + noteField(skill, 'designObjective', 'Objectif de design', notes.designObjective)
      + noteField(skill, 'bkvinceProblem', 'Problème BKVince à régler', notes.bkvinceProblem)
      + noteField(skill, 'finalJustification', 'Justification finale', notes.finalJustification)
      + noteField(skill, 'testPlan', 'Plan de test', notes.testPlan)
      + '</div><label><span>Statut d’implantation — séparé de la décision de design</span><select data-implementation-status data-skill-id="' + escapeAttribute(skill.stableId) + '">' + optionList(IMPLEMENTATION_STATUSES, current.implementationStatus, 'NOT_REVIEWED') + '</select></label><p class="warning-box">DECISION_COMPLETE ne devient jamais automatiquement IMPLEMENTATION_AUTHORIZED. Le Workbench ne lance aucune implantation.</p>'
      + (asArray(state.questions || state.unresolvedQuestions).length ? '<div class="questions"><strong>Questions non résolues</strong><ul>' + asArray(state.questions || state.unresolvedQuestions).map((question) => '<li>' + escapeHtml(question) + '</li>').join('') + '</ul></div>' : '') + '</section>';
  }

  function noteField(skill, property, label, value) {
    return '<label><span>' + escapeHtml(label) + '</span><textarea data-note-property="' + property + '" data-skill-id="' + escapeAttribute(skill.stableId) + '">' + escapeHtml(value || '') + '</textarea></label>';
  }

  function onClick(event) {
    const button = event.target.closest('button');
    if (!button) return;
    if (button.dataset.view) {
      ui.activeViewId = button.dataset.view;
      ui.filters.query = '';
      render();
      return;
    }
    if (button.dataset.mode) {
      ui.mode = button.dataset.mode;
      render();
      return;
    }
    if (button.dataset.scrollTree) {
      document.querySelector('#tree-' + cssEscape(button.dataset.scrollTree))?.scrollIntoView({ behavior: 'smooth', block: 'start' });
      return;
    }
    if (button.dataset.bulk) {
      applyBulkFromButton(button);
      return;
    }
    const action = button.dataset.action;
    if (!action) return;
    const skill = button.dataset.skillId ? skillById.get(button.dataset.skillId) : null;
    if (action === 'toggle-skill' && skill) {
      if (ui.expanded.has(skill.stableId)) ui.expanded.delete(skill.stableId); else ui.expanded.add(skill.stableId);
      render();
    } else if (action === 'expand-all') {
      for (const item of visibleSkills()) ui.expanded.add(item.stableId);
      render();
    } else if (action === 'collapse-all') {
      ui.expanded.clear();
      render();
    } else if (action === 'next-incomplete') {
      openNextIncomplete();
    } else if (action === 'clear-filters') {
      ui.filters = { query: '', incomplete: false, significant: false, portability: '', mapping: '', proof: '', decision: '', collision: '', nativeRisk: '', pd2New: '' };
      render();
    } else if (action === 'export-all') {
      exportDecisions('ALL');
    } else if (action === 'export-complete') {
      exportDecisions('COMPLETE_ONLY');
    } else if (action === 'import') {
      document.querySelector('#decision-file').click();
    } else if (action === 'reset-global') {
      resetGlobal();
    } else if (action === 'reset-class') {
      resetSkills(navById.get(button.dataset.viewId)?.skillIds || [], 'Réinitialiser toutes les décisions de cette classe ?');
    } else if (action === 'reset-skill' && skill) {
      resetSkills([skill.stableId], 'Réinitialiser toutes les décisions de « ' + skill.canonicalName + ' » ?');
    } else if (action === 'copy-briefing' && skill) {
      copyText(skillMarkdown(skill, true));
    } else if (action === 'export-skill-md' && skill) {
      download(slug(skill.canonicalName) + '-review.md', skillMarkdown(skill, false), 'text/markdown');
    } else if (action === 'export-skill-json' && skill) {
      download(slug(skill.canonicalName) + '-review.json', JSON.stringify(skillExport(skill), null, 2) + '\n', 'application/json');
    } else if (action === 'export-class') {
      exportClass(button.dataset.viewId);
    }
  }

  function onChange(event) {
    const target = event.target;
    if (target.matches('[data-filter]')) {
      const property = target.dataset.filter;
      ui.filters[property] = target.type === 'checkbox' ? target.checked : target.value;
      render();
      return;
    }
    if (target.id === 'decision-file') {
      importFile(target.files?.[0]);
      target.value = '';
      return;
    }
    if (target.matches('[data-scenario]')) {
      ui.scenario.set(target.dataset.skillId, target.value);
      render();
      return;
    }
    if (target.matches('[data-synergy-input]')) {
      const skillId = target.dataset.skillId;
      const scenarioId = target.dataset.scenarioId;
      const inputId = target.dataset.inputId;
      const key = skillId + '::' + scenarioId + '::' + inputId;
      ui.synergyInputs ||= new Map();
      ui.synergyInputs.set(key, Number(target.value));
      render();
      return;
    }
    const skill = skillById.get(target.dataset.skillId);
    if (!skill || skill.readOnly) return;
    if (target.matches('[data-global-decision]')) {
      entryFor(skill, true).globalDecision = target.value || undefined;
    } else if (target.matches('[data-line-decision]')) {
      entryFor(skill, true).newSkillLineDecision = target.value || undefined;
    } else if (target.matches('[data-component-decision]')) {
      const entry = entryFor(skill, true);
      entry.componentDecisions ||= {};
      setDecision(entry.componentDecisions, target.dataset.componentId, target.value);
    } else if (target.matches('[data-field-decision]')) {
      const entry = entryFor(skill, true);
      entry.fieldDecisions ||= {};
      setDecision(entry.fieldDecisions, target.dataset.fieldId, target.value);
    } else if (target.matches('[data-implementation-status]')) {
      entryFor(skill, true).implementationStatus = target.value || 'NOT_REVIEWED';
    } else if (target.matches('[data-override-approved], [data-override-property]')) {
      updateOverride(target, skill);
    } else {
      return;
    }
    scheduleSave();
    render();
  }

  function onInput(event) {
    const target = event.target;
    if (target.id === 'global-search') {
      ui.filters.query = target.value;
      window.clearTimeout(onInput.searchTimer);
      onInput.searchTimer = window.setTimeout(render, 160);
      return;
    }
    const skill = skillById.get(target.dataset.skillId);
    if (!skill || skill.readOnly) return;
    if (target.matches('[data-note-property]')) {
      const entry = entryFor(skill, true);
      entry.notes ||= {};
      entry.notes[target.dataset.noteProperty] = target.value;
    } else if (target.matches('[data-choice-property]')) {
      const choice = mutableChoice(target, skill);
      choice[target.dataset.choiceProperty] = target.value;
    } else if (target.matches('[data-override-property]')) {
      updateOverride(target, skill);
    } else {
      return;
    }
    scheduleSave();
  }

  function setDecision(container, id, decision) {
    if (!decision) {
      delete container[id];
      return;
    }
    const previous = container[id] || {};
    container[id] = { ...previous, decision };
    if (decision !== 'CUSTOM') {
      delete container[id].customValue;
      delete container[id].justification;
      delete container[id].gameplayObjective;
      delete container[id].testPlan;
    }
    if (!['CUSTOM', 'ADOPT_PD2'].includes(decision)) delete container[id].protectedOverride;
  }

  function mutableChoice(target, skill) {
    const entry = entryFor(skill, true);
    const isField = target.dataset.choiceLevel === 'field';
    const container = isField ? (entry.fieldDecisions ||= {}) : (entry.componentDecisions ||= {});
    const id = isField ? target.dataset.fieldId : target.dataset.componentId;
    return container[id] ||= {};
  }

  function updateOverride(target, skill) {
    const choice = mutableChoice(target, skill);
    choice.protectedOverride ||= {};
    if (target.matches('[data-override-approved]')) choice.protectedOverride.approved = target.checked;
    else {
      const property = target.dataset.overrideProperty;
      choice.protectedOverride[property] = target.type === 'checkbox' ? target.checked : target.value;
    }
  }

  function applyBulkFromButton(button) {
    const skillIds = button.dataset.skillIds.split('|').filter(Boolean);
    const mode = button.closest('.bulk-controls')?.querySelector('[data-bulk-mode]')?.value || 'fill';
    const replace = mode === 'replace';
    let confirmed = false;
    if (replace) {
      confirmed = window.confirm('Remplacer toutes les décisions de cette portée ? Les décisions CUSTOM et les notes ne seront jamais écrasées silencieusement.');
      if (!confirmed) return;
    }
    try {
      const result = runtime.applyBulk(report, skillIds, envelope.entries || {}, button.dataset.bulk, { replace, confirmed });
      envelope.entries = result?.entries || result;
      scheduleSave();
      showNotice('Action en lot appliquée à ' + skillIds.length + ' skill(s).', 'ok');
      render();
    } catch (error) {
      showNotice('Action refusée : ' + error.message, 'error');
    }
  }

  function openNextIncomplete() {
    const ordered = report.skills.filter((skill) => !skill.readOnly && !skill.identical && !isComplete(skill));
    const skill = ordered[0];
    if (!skill) {
      showNotice('Toutes les décisions requises sont complètes.', 'ok');
      return;
    }
    const view = normalizedNavigation.find((candidate) => candidate.skillIds.includes(skill.stableId));
    if (view) ui.activeViewId = view.id;
    ui.filters.query = '';
    ui.expanded.add(skill.stableId);
    render();
    window.setTimeout(() => document.querySelector('#skill-' + cssEscape(skill.stableId))?.scrollIntoView({ behavior: 'smooth', block: 'start' }), 0);
  }

  function resetGlobal() {
    if (!window.confirm('Réinitialiser toutes les décisions locales liées à ce comparisonHash ?')) return;
    envelope = runtime.createEmptyEnvelope(report);
    try { localStorage.removeItem(storageKey); } catch { /* export remains available */ }
    showNotice('Décisions globales réinitialisées.', 'ok');
    render();
  }

  function resetSkills(skillIds, prompt) {
    if (!window.confirm(prompt)) return;
    envelope.entries ||= {};
    for (const id of skillIds) delete envelope.entries[id];
    scheduleSave();
    render();
  }

  async function importFile(file) {
    if (!file) return;
    try {
      const payload = JSON.parse(await file.text());
      let imported;
      try {
        imported = validatedEnvelope(payload);
        showNotice('Import validé : hashes et fingerprints compatibles.', 'ok');
      } catch (validationError) {
        if (!window.confirm('Import direct refusé : ' + validationError.message + '\nTenter une migration contrôlée par stableId et fingerprint ?')) throw validationError;
        const migration = runtime.migrateEnvelope(report, payload);
        imported = migration.envelope || migration.value;
        const migrationReport = migration.report || migration;
        showNotice('Migration contrôlée : ' + (migrationReport.retained?.length || 0) + ' conservées, ' + (migrationReport.stale?.length || 0) + ' périmées, ' + (migrationReport.dropped?.length || 0) + ' rejetées.', 'warning');
        download('pd2-skills-decisions-migration-report.json', JSON.stringify(migrationReport, null, 2) + '\n', 'application/json');
      }
      if (!imported?.entries) throw new Error('La migration n’a produit aucune enveloppe de décisions.');
      envelope = imported;
      scheduleSave();
      render();
    } catch (error) {
      showNotice('Import refusé : ' + error.message, 'error');
    }
  }

  function exportDecisions(scope) {
    try {
      const payload = runtime.exportEnvelope(report, envelope.entries || {}, { scope });
      download(scope === 'COMPLETE_ONLY' ? 'pd2-skills-decisions-complete.json' : 'pd2-skills-decisions-all.json', JSON.stringify(payload, null, 2) + '\n', 'application/json');
    } catch (error) {
      showNotice('Export refusé : ' + error.message, 'error');
    }
  }

  function skillExport(skill) {
    return {
      schemaVersion: report.schemaVersion,
      reviewId: report.reviewId,
      comparisonHash: report.comparisonHash,
      sourceHashes: report.sourceHashes,
      skill,
      decision: entryFor(skill, false),
      reviewState: reviewState(skill),
      collisions: (skill.collisionIds || []).map((id) => collisionById.get(id) || { id }),
      exportedAt: new Date().toISOString(),
      disclaimer: 'Review and preview only; no gameplay implementation is authorized.',
    };
  }

  function skillMarkdown(skill, briefing) {
    const current = entryFor(skill, false);
    const state = reviewState(skill);
    const lines = [
      '# ' + skill.canonicalName + ' — PD2 Skills Merge Workbench', '',
      '> Dossier de revue seulement. Aucune implantation gameplay n’est autorisée.', '',
      '## Identité', '',
      '- Stable ID: `' + skill.stableId + '`',
      '- Classe / portée: ' + (skill.classCode || skill.scope || 'classless'),
      '- Arbre / position: ' + (skill.tree?.label || skill.tree?.name || 'non documenté') + ' · ' + positionLabel(skill.tree || {}),
      '- Ordinals Vanilla / BKVince / PD2: ' + SOURCE_KEYS.map((source) => display(skill.ordinals?.[source] ?? nodeFor(skill, source)?.ordinal)).join(' / '),
      '- Mapping: ' + (skill.mappingTypes || []).join(', '), '',
      '## Résumé des différences', '', playerSummary(skill), '',
      '## Portabilité et risques', '',
      '- Classifications: ' + portabilityCategoriesOf(skill).join(', '),
      '- Preuves: ' + proofStatusesOf(skill).join(', '),
      '- Raisons: ' + asArray(skill.portability?.reasons).map(display).join('; '),
      '- Dépendances manquantes: ' + asArray(skill.portability?.missingDependencies).map(display).join('; '),
      '- Preuves encore requises: ' + asArray(skill.portability?.requiredProof || skill.portability?.missingProof).map(display).join('; '), '',
      '## Décisions actuelles', '', '```json', JSON.stringify(current, null, 2), '```', '',
      '## Questions non résolues', '', ...asArray(state.questions || state.unresolvedQuestions || state.reasons).map((item) => '- ' + display(item)), '',
      '## Notes de Vincent', '', ...Object.entries(current.notes || {}).map(([key, value]) => '- ' + friendly(key) + ': ' + (value || '—')), '',
    ];
    if (!briefing) {
      lines.push('## Courbes', '', markdownCurves(skill), '', '## Formules et composantes', '', markdownComponents(skill), '', '## Dépendances', '', markdownObjects(skill.dependencies), '', '## Consommateurs', '', markdownObjects(skill.consumers), '', '## Collisions', '', markdownObjects((skill.collisionIds || []).map((id) => collisionById.get(id) || { id })), '');
    }
    return lines.join('\n');
  }

  function markdownCurves(skill) {
    const scenarios = scenariosOf(skill);
    if (!scenarios.length) return 'Aucune courbe numérique gouvernée.';
    return scenarios.map((scenario) => {
      const levels = scenario.levels || report.levels || [1, 5, 10, 20, 30, 40];
      return '### ' + (scenario.label || friendly(scenario.id)) + '\n\n' + seriesOfScenario(scenario).map((metric) => {
        const sourceRows = sourceSeries(metric);
        return '| ' + (metric.label || metric.id) + ' | ' + levels.map((level) => 'L' + level).join(' | ') + ' |\n|---|' + levels.map(() => '---:').join('|') + '|\n' + sourceRows.map((item) => '| ' + (SOURCE_LABELS[item.source] || item.source) + ' | ' + item.values.map(display).join(' | ') + ' |').join('\n');
      }).join('\n\n');
    }).join('\n\n');
  }

  function markdownComponents(skill) {
    return (skill.components || []).map((component) => '### ' + (component.label || component.id) + '\n\n' + (component.fields || []).map((field) => '- **' + (field.label || field.header || field.id) + '** (`' + (field.table || 'skills.txt') + '.' + (field.header || field.id) + '`): ' + SOURCE_KEYS.map((source) => (SOURCE_LABELS[source] + '=' + display((field.displayValues || field.values || {})[source]))).join(' · ') + (field.formula ? ' · formule `' + display(field.formula.raw || field.formula.expression || field.formula) + '`' : '') + (field.protected ? ' · **PROTÉGÉ**' : '')).join('\n')).join('\n\n');
  }

  function markdownObjects(items) {
    return asArray(items).length ? asArray(items).map((item) => '- ' + (typeof item === 'string' ? item : '`' + JSON.stringify(item) + '`')).join('\n') : 'Aucun élément documenté.';
  }

  function exportClass(viewId) {
    const view = navById.get(viewId);
    if (!view) return;
    const skills = view.skillIds.map((id) => skillById.get(id)).filter(Boolean);
    const stats = dashboardStats(skills);
    const markdown = '# Dossier de révision — ' + view.label + '\n\n> PD2 Skills Merge Workbench · comparisonHash `' + report.comparisonHash + '` · aucune autorisation gameplay.\n\n## Tableau de bord\n\n- Skills: ' + stats.total + '\n- Identiques: ' + stats.identical + '\n- Modifiés: ' + stats.modified + '\n- Nouveaux candidats PD2: ' + stats.newCandidates + '\n- Collisions: ' + stats.collisions + '\n- Preuves natives requises: ' + stats.native + '\n- Décisions complètes: ' + stats.complete + '\n- Décisions restantes: ' + stats.remaining + '\n\n' + skills.map((skill) => skillMarkdown(skill, false)).join('\n\n---\n\n');
    download(slug(view.label) + '-pd2-skills-review.md', markdown + '\n', 'text/markdown');
  }

  async function copyText(text) {
    try {
      if (navigator.clipboard?.writeText) await navigator.clipboard.writeText(text);
      else {
        const textarea = document.createElement('textarea');
        textarea.value = text;
        textarea.style.position = 'fixed';
        textarea.style.opacity = '0';
        document.body.append(textarea);
        textarea.select();
        document.execCommand('copy');
        textarea.remove();
      }
      showNotice('Briefing copié dans le presse-papiers.', 'ok');
    } catch (error) {
      showNotice('Copie impossible : ' + error.message, 'error');
    }
  }

  function download(filename, content, type) {
    const url = URL.createObjectURL(new Blob([content], { type: type + ';charset=utf-8' }));
    const anchor = document.createElement('a');
    anchor.href = url;
    anchor.download = filename;
    anchor.click();
    window.setTimeout(() => URL.revokeObjectURL(url), 0);
  }

  function slug(value) {
    return String(value || 'skill').normalize('NFKD').replace(/[\u0300-\u036f]/g, '').toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '');
  }

  function cssEscape(value) {
    if (globalThis.CSS?.escape) return CSS.escape(value);
    return String(value).replace(/[^a-zA-Z0-9_-]/g, (character) => '\\' + character.codePointAt(0).toString(16) + ' ');
  }
}

const STYLE = `
:root{color-scheme:dark;--bg:#091018;--panel:#111c28;--panel-2:#172536;--panel-3:#213247;--line:#344a61;--text:#eef5fb;--muted:#9fb0c1;--gold:#f0bd6b;--blue:#72c8ff;--green:#6ed49a;--red:#ff8d88;--violet:#b9a1ff;--shadow:0 12px 35px #0006}*{box-sizing:border-box}html{scroll-behavior:smooth}body{margin:0;background:radial-gradient(circle at 75% -20%,#17304c 0,transparent 42%),var(--bg);color:var(--text);font:14px/1.5 Inter,ui-sans-serif,system-ui,-apple-system,"Segoe UI",sans-serif}button,input,select,textarea{font:inherit}button,select,input,textarea{border:1px solid var(--line);border-radius:7px;background:var(--panel-2);color:var(--text);padding:8px 10px}button{cursor:pointer}button:hover,button:focus-visible{border-color:var(--gold);outline:none}button.active{background:#2d3e55;color:#fff;border-color:var(--gold)}button.danger{border-color:#74454a;color:#ffc0bd}label>span{display:block;color:var(--muted);font-size:12px;margin-bottom:3px}textarea{width:100%;min-height:68px;resize:vertical}code,pre{font:12px/1.45 ui-monospace,SFMono-Regular,Consolas,monospace}pre{white-space:pre-wrap;overflow-wrap:anywhere;background:#09131e;padding:12px;border-radius:8px;border:1px solid var(--line);max-height:520px;overflow:auto}.topbar{position:sticky;top:0;z-index:20;padding:13px 20px 11px;background:#091018f2;backdrop-filter:blur(15px);border-bottom:1px solid var(--line);box-shadow:0 4px 18px #0005}.title-row{display:flex;justify-content:space-between;gap:20px;align-items:center}.title-row h1{font-size:23px;margin:0}.title-row p{margin:1px 0;color:var(--muted)}.global-progress{display:grid;grid-template-columns:auto auto;gap:2px 10px;align-items:center}.global-progress progress{grid-column:1/-1;width:220px;height:7px}.non-mutation{margin:8px 0;padding:7px 10px;border-left:3px solid var(--gold);background:#302615;color:#ffdfa8}.primary-controls,.persistence-toolbar,.filters,.chips,.card-actions,.bulk-controls,.dashboard-actions{display:flex;gap:7px;align-items:end;flex-wrap:wrap}.primary-controls .search{flex:1;min-width:230px}.primary-controls .search input{width:100%}.segmented{display:flex}.segmented button{border-radius:0}.segmented button:first-child{border-radius:7px 0 0 7px}.segmented button:last-child{border-radius:0 7px 7px 0}.filter-panel{margin-top:8px}.filter-panel summary{cursor:pointer;color:var(--muted)}.filters{padding-top:8px}.filters label:not(.check){min-width:145px}.check{align-self:center}.persistence-toolbar{margin-top:8px}.hash{color:var(--muted);font:11px ui-monospace,monospace}#save-status{padding:5px 8px;border-radius:99px;color:var(--muted)}#save-status[data-status=saved]{color:var(--green)}#save-status[data-status=error]{color:var(--red)}.notice{position:fixed;right:18px;bottom:18px;z-index:50;max-width:520px;padding:12px 15px;background:var(--panel-3);border:1px solid var(--line);border-radius:8px;box-shadow:var(--shadow)}.notice.ok{border-color:var(--green)}.notice.error{border-color:var(--red)}.notice.warning{border-color:var(--gold)}.layout{display:grid;grid-template-columns:230px minmax(0,1fr);max-width:1800px;margin:auto}.sidebar{position:sticky;top:253px;align-self:start;max-height:calc(100vh - 270px);overflow:auto;padding:18px 12px;border-right:1px solid var(--line)}.sidebar nav{display:grid;gap:5px}.sidebar button{display:flex;justify-content:space-between;text-align:left}.sidebar small{color:var(--muted)}.sidebar h2{font-size:12px;text-transform:uppercase;letter-spacing:.1em;color:var(--muted);margin:18px 5px 7px}#main-content{min-width:0;padding:20px}.dashboard,.tree{margin-bottom:24px}.section-title,.tree-heading{display:flex;justify-content:space-between;gap:12px;align-items:flex-start}.eyebrow{text-transform:uppercase;letter-spacing:.12em;color:var(--gold);font-size:11px;margin:0}.section-title h2,.tree-heading h2{margin:1px 0}.metrics{display:grid;grid-template-columns:repeat(6,minmax(105px,1fr));gap:8px;margin:12px 0}.metric{background:linear-gradient(145deg,var(--panel-2),var(--panel));border:1px solid var(--line);border-radius:9px;padding:10px}.metric strong{font-size:22px;display:block}.metric span{color:var(--muted);font-size:12px}.metric.ok strong{color:var(--green)}.metric.warning strong{color:var(--gold)}.metric.danger strong{color:var(--red)}.scope-bulk,.tree-bulk{padding:9px;background:var(--panel);border:1px solid var(--line);border-radius:9px}.tree-bulk>span{display:block;color:var(--muted);font-size:11px}.result-summary{padding:8px 11px;background:#0d1722;border:1px solid var(--line);border-radius:7px;margin-bottom:14px;color:var(--muted)}.tree{scroll-margin-top:275px}.tree-heading{border-bottom:1px solid var(--line);padding-bottom:9px;margin-bottom:10px}.skill-card{scroll-margin-top:275px;border:1px solid var(--line);border-radius:11px;background:linear-gradient(150deg,#132131,#0f1925);margin:10px 0;overflow:hidden;box-shadow:0 6px 20px #0003}.skill-card.incomplete{border-left:4px solid var(--gold)}.skill-card.complete{border-left:4px solid var(--green)}.skill-card.read-only{border-left-color:#72849a}.skill-card-heading{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:12px 14px}.expander{display:flex;align-items:center;gap:8px;background:none;border:0;text-align:left;padding:0;flex:1}.expander strong{display:block;font-size:18px}.expander small{display:block;color:var(--muted)}.chevron{font-size:20px;color:var(--gold)}.decision-progress{font-size:12px;color:var(--gold)}.decision-progress.done{color:var(--green)}.identity-strip{display:grid;grid-template-columns:repeat(3,1fr);border-top:1px solid var(--line);border-bottom:1px solid var(--line)}.identity-strip>div{padding:9px 13px;border-right:1px solid var(--line)}.identity-strip>div:last-child{border:0}.identity-strip span,.identity-strip small{display:block;color:var(--muted);font-size:11px}.identity-strip strong{display:block}.skill-card>.chips,.player-summary,.card-decision{margin:10px 14px}.chip{display:inline-block;border:1px solid var(--line);border-radius:99px;padding:2px 7px;color:#c2cfdb;font-size:11px}.chip.portability{border-color:#5a5184;color:#cfbfff}.chip.proof{border-color:#3f6a67;color:#9fe2da}.chip.collision,.chip.warning,.chip.danger{border-color:#8a524c;color:#ffafa7}.chip.new{border-color:#85693e;color:#ffd28c}.chip.ok{border-color:#477a58;color:#8be5a8}.player-summary{font-size:15px}.card-decision{display:flex;align-items:start;gap:10px}.card-decision>label{min-width:290px}.read-only-banner,.warning-box,.validation-errors,.protection-warning,.symbolic{padding:9px 11px;border-radius:7px;background:#1d2834;border-left:3px solid #71859a}.warning-box,.protection-warning{background:#302619;border-left-color:var(--gold)}.validation-errors{background:#321f23;border-left-color:var(--red)}.validation-errors ul,.protection-warning ul{margin:5px 0}.skill-card-body{border-top:1px solid var(--line);padding:14px}.card-actions{margin-bottom:14px}.comparison-section,.curves,.new-skill-plan,.skill-card-body>section{margin:18px 0}.tri-way{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}.tri-way>div{padding:10px;border:1px solid var(--line);border-radius:8px;background:var(--panel)}.table-wrap{overflow:auto}table{width:100%;border-collapse:collapse}th,td{border:1px solid var(--line);padding:7px;text-align:left;vertical-align:top}th{background:#172536}caption{text-align:left;color:var(--muted);padding:4px}.charts{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.chart-card{margin:0;padding:10px;border:1px solid var(--line);border-radius:9px;background:var(--panel)}.chart-card figcaption,.chart-card h4{font-weight:700;margin:0 0 5px}.chart-card svg{width:100%;height:auto;background:#0b1520;border-radius:6px}.chart-card .axis{stroke:#52667b;fill:none}.chart-legend{display:flex;gap:12px;flex-wrap:wrap}.chart-legend span:before{content:"";display:inline-block;width:10px;height:3px;background:var(--legend);margin-right:4px;vertical-align:middle}.curve-table{font-size:11px}.symbolic-value{color:var(--gold);font-family:ui-monospace,monospace}.facts{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:8px}.facts div,.list-grid>div{background:var(--panel);padding:8px;border-radius:7px}.facts dt,.list-grid strong{color:var(--muted);font-size:11px}.facts dd{margin:2px 0}.list-grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:7px}.list-grid h4{grid-column:1/-1}.list-grid span{display:block}.component{border:1px solid var(--line);border-radius:9px;margin:8px 0;background:#0f1a26}.component>summary{cursor:pointer;display:flex;justify-content:space-between;gap:10px;padding:10px}.component>summary strong,.component>summary small{display:block}.component>summary small{color:var(--muted)}.component-body{padding:0 10px 10px}.component-decision{display:block;max-width:340px;margin:5px 0 9px}.field-list{display:grid;gap:7px}.field-row{padding:9px;border:1px solid #2b3f53;border-radius:8px;background:var(--panel)}.field-row.changed{border-left:3px solid var(--blue)}.field-row.protected{border-left-color:var(--red);background:#211b21}.field-title{display:flex;justify-content:space-between;gap:8px}.field-title small{display:block;color:var(--muted)}.field-values{display:grid;grid-template-columns:repeat(3,1fr);gap:5px;margin:7px 0}.field-values>div{padding:6px;background:#0b1520;border-radius:5px}.field-values span{display:block;color:var(--muted);font-size:10px}.field-values code{overflow-wrap:anywhere}.formula{display:flex;gap:8px;align-items:start;background:#0a141e;padding:7px;border-radius:6px}.formula code{flex:1;overflow-wrap:anywhere}.field-decision{display:grid;grid-template-columns:minmax(180px,300px) 1fr;gap:8px;margin-top:8px}.custom-fields,.override-fields{display:grid;grid-template-columns:repeat(2,1fr);gap:7px}.override-fields{border:1px solid var(--red);border-radius:7px;padding:8px}.override-fields legend{color:var(--red)}.two-column{display:grid;grid-template-columns:1fr 1fr;gap:14px}.structured-list{list-style:none;padding:0}.structured-list li{display:flex;justify-content:space-between;gap:8px;padding:7px;border-bottom:1px solid var(--line)}.structured-list span{color:var(--muted)}.collision-grid,.documentation{display:grid;grid-template-columns:repeat(2,1fr);gap:8px}.collision-grid article,.documentation article{padding:10px;border:1px solid var(--line);border-radius:8px;background:var(--panel)}.documentation a{display:block;color:var(--blue);margin-top:5px}.technical{border-top:1px dashed var(--line);padding-top:10px}.notes-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:8px}.notes-grid label:last-child{grid-column:1/-1}.empty-state,.empty-inline{color:var(--muted);padding:14px;border:1px dashed var(--line);border-radius:8px}.fatal{max-width:760px;margin:60px auto;padding:24px}.fatal pre{border-color:var(--red)}@media(max-width:1200px){.metrics{grid-template-columns:repeat(4,1fr)}.topbar{position:relative}.sidebar{top:12px}.skill-card,.tree{scroll-margin-top:12px}}@media(max-width:850px){.layout{display:block}.sidebar{position:relative;top:0;max-height:none;border-right:0;border-bottom:1px solid var(--line)}.sidebar nav{grid-template-columns:repeat(2,1fr)}.metrics{grid-template-columns:repeat(2,1fr)}.identity-strip,.tri-way,.field-values,.facts,.two-column,.charts,.collision-grid,.documentation,.notes-grid{grid-template-columns:1fr}.field-decision,.custom-fields,.list-grid{grid-template-columns:1fr}.title-row,.tree-heading,.section-title{display:block}.topbar{padding:12px}.skill-card-heading{align-items:flex-start}.decision-progress{max-width:130px}.list-grid h4{grid-column:auto}}@media print{.topbar,.sidebar,.bulk-controls,.card-actions,button,select,input[type=checkbox],#notice{display:none!important}.layout{display:block}.skill-card-body{display:block!important}.skill-card{break-inside:avoid}.technical pre{max-height:none}}
`;

export function buildSkillReviewHtml(report, browserRuntimeSource) {
  if (!report || typeof report !== 'object' || Array.isArray(report)) throw new TypeError('report must be an oracle object');
  if (!Array.isArray(report.skills)) throw new TypeError('report.skills must be an array');
  if (!report.comparisonHash) throw new TypeError('report.comparisonHash is required');
  if (typeof browserRuntimeSource !== 'string' || !browserRuntimeSource.trim()) throw new TypeError('browserRuntimeSource must be a non-empty string');
  const embeddedReport = serializeForInlineScript(report);
  const runtimeSource = protectInlineSource(browserRuntimeSource);
  const applicationSource = protectInlineSource('(' + skillReviewBrowserApplication.toString() + ')();');
  return '<!doctype html>\n'
    + '<html lang="fr"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">'
    + '<meta name="referrer" content="no-referrer"><title>PD2 Skills Merge Workbench — Vanilla / BKVince / PD2</title>'
    + '<style>' + STYLE + '</style></head><body><div id="workbench"></div>'
    + '<noscript><main class="fatal"><h1>PD2 Skills Merge Workbench</h1><p>JavaScript local est requis pour les filtres, décisions et exports.</p></main></noscript>'
    + '<script>const report=' + embeddedReport + ';\n'
    + '/* Browser decision runtime: canonical decision semantics supplied by the generator. */\n'
    + runtimeSource + '\n' + applicationSource + '\n</script></body></html>\n';
}
