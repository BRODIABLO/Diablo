import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

import { parseOrdinalSpec } from './pd2-affixes-merge.mjs';

const require = createRequire(import.meta.url);
const { ENCODING, parseTable, serializeTable } = require('../build-data/tsv.js');
const repoRoot = fileURLToPath(new URL('../../', import.meta.url));
const catalogPath = path.join(repoRoot, 'Mission', 'pd2-affixes-merge.catalog.json');
const outputJson = path.join(repoRoot, 'Mission', 'pd2-affixes-review.json');
const outputHtml = path.join(repoRoot, 'Mission', 'pd2-affixes-review.html');
const targetRoot = path.join(repoRoot, 'data-BKVince', 'BKVince.mpq', 'data', 'global', 'excel');
const vanillaRoot = path.join(repoRoot, 'data-vanilla3.2', 'data', 'data', 'global', 'excel');

const TABLES = {
  'magicprefix.txt': { label: 'Préfixe', mapped: 670, targetRow: (row) => row },
  'magicsuffix.txt': { label: 'Suffixe', mapped: 748, targetRow: (row) => (row <= 662 ? row : row + 7) },
  'automagic.txt': { label: 'AutoMagic', mapped: 36, targetRow: (row) => row },
};

function assert(condition, message) {
  if (!condition) throw new Error(message);
}

function sha256(value) {
  return crypto.createHash('sha256').update(value).digest('hex').toUpperCase();
}

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8').replace(/^\uFEFF/, ''));
}

function findFile(root, wanted) {
  const matches = fs.readdirSync(root).filter((name) => name.toLowerCase() === wanted.toLowerCase());
  assert(matches.length === 1, `${root}: expected exactly one ${wanted}`);
  return path.join(root, matches[0]);
}

function loadTable(root, name) {
  const filePath = findFile(root, name);
  const raw = fs.readFileSync(filePath, ENCODING);
  const table = parseTable(filePath);
  assert(serializeTable(table) === raw, `${name}: non byte-exact TSV round-trip`);
  const indexes = new Map(table.headers.map((header, index) => [header.toLowerCase(), index]));
  return { filePath, raw, table, indexes, sha256: sha256(Buffer.from(raw, ENCODING)) };
}

function value(loaded, row, header) {
  const index = loaded.indexes.get(header.toLowerCase());
  return index === undefined ? '' : (loaded.table.rows[row]?.[index] ?? '');
}

function rowName(loaded, row) {
  return value(loaded, row, 'Name');
}

function isRealRow(loaded, row) {
  const name = rowName(loaded, row);
  return name !== '' && name !== 'Expansion';
}

function differences(left, leftRow, right, rightRow) {
  const rightHeaders = new Set(right.table.headers.map((header) => header.toLowerCase()));
  return left.table.headers
    .filter((header) => rightHeaders.has(header.toLowerCase()))
    .filter((header) => {
      const leftValue = value(left, leftRow, header);
      const rightValue = value(right, rightRow, header);
      if (header.toLowerCase() === 'multiply') {
        return (leftValue || '0') !== (rightValue || '0');
      }
      return leftValue !== rightValue;
    })
    .map((header) => ({
      column: header,
      left: value(left, leftRow, header),
      right: value(right, rightRow, header),
    }));
}

function compactDiffs(diffs, leftLabel, rightLabel) {
  return diffs.map(({ column, left, right }) => ({ column, [leftLabel]: left, [rightLabel]: right }));
}

function effect(loaded, row) {
  const parts = [];
  for (let slot = 1; slot <= 3; slot += 1) {
    const code = value(loaded, row, `mod${slot}code`);
    if (!code) continue;
    const parameter = value(loaded, row, `mod${slot}param`);
    const minimum = value(loaded, row, `mod${slot}min`);
    const maximum = value(loaded, row, `mod${slot}max`);
    const range = minimum || maximum ? `${minimum || '?'}–${maximum || minimum || '?'}` : '';
    parts.push([code, parameter ? `(${parameter})` : '', range].filter(Boolean).join(' '));
  }
  return parts.join(' · ') || 'Aucun mod direct';
}

function itemTypes(loaded, row) {
  const allowed = [];
  const excluded = [];
  for (let slot = 1; slot <= 7; slot += 1) {
    const code = value(loaded, row, `itype${slot}`);
    if (code) allowed.push(code);
  }
  for (let slot = 1; slot <= 5; slot += 1) {
    const code = value(loaded, row, `etype${slot}`);
    if (code) excluded.push(code);
  }
  return { allowed, excluded };
}

function blockedRows(expected) {
  const result = new Map();
  for (const [reason, spec] of Object.entries(expected.blocked ?? {})) {
    for (const row of parseOrdinalSpec(spec)) result.set(row, reason);
  }
  return result;
}

function sourceRootFromArgs(args) {
  const option = args.find((arg) => arg.startsWith('--source-root='));
  if (option) return path.resolve(option.slice('--source-root='.length));
  const official = path.join(repoRoot, 'analysis-cache', 'pd2-affixes-merge', 'official-s13');
  if (fs.existsSync(official)) return official;
  return path.resolve(repoRoot, '..', 'PD2 Single PLayer', 'PD2-Single-Player-Plus-mod-main', 'data', 'global', 'excel');
}

function buildReport(sourceRoot, catalog) {
  const entries = [];
  const counts = {};
  for (const [tableName, config] of Object.entries(TABLES)) {
    const source = loadTable(sourceRoot, tableName);
    const target = loadTable(targetRoot, tableName);
    const vanilla = loadTable(vanillaRoot, tableName);
    assert(target.sha256 === catalog.targetBaseline[tableName].sha256, `${tableName}: BKVince is not at review baseline`);
    assert(vanilla.sha256 === catalog.vanillaBaseline[tableName], `${tableName}: vanilla baseline drift`);
    const selectedRetunes = new Set(parseOrdinalSpec(catalog.expected.retunes[tableName].sourceRows));
    const selectedAppends = new Set(parseOrdinalSpec(catalog.expected.appends[tableName].sourceRows));
    const blocked = blockedRows(catalog.expected.appends[tableName]);
    const mappedTargets = new Set();

    for (let sourceRow = 0; sourceRow < source.table.rows.length; sourceRow += 1) {
      if (!isRealRow(source, sourceRow)) continue;
      const mapped = sourceRow < config.mapped;
      const targetRow = mapped ? config.targetRow(sourceRow) : null;
      if (mapped) mappedTargets.add(targetRow);
      const pd2VsVanilla = mapped
        ? differences(source, sourceRow, vanilla, sourceRow)
        : [];
      const bkvVsVanilla = mapped
        ? differences(target, targetRow, vanilla, sourceRow)
        : [];
      const pd2Deleted = mapped
        && value(vanilla, sourceRow, 'spawnable') === '1'
        && value(source, sourceRow, 'spawnable') !== '1';
      const selectedRetune = selectedRetunes.has(sourceRow);
      const selectedAppend = selectedAppends.has(sourceRow);
      const blockedReason = blocked.get(sourceRow) ?? null;
      let status;
      let statusLabel;
      let rationale;
      if (mapped && pd2Deleted) {
        status = 'pd2_deleted';
        statusLabel = 'Supprimé/désactivé par PD2';
        rationale = 'Vanilla le faisait apparaître; PD2 ne le fait plus apparaître.';
      } else if (mapped && selectedRetune) {
        status = tableName === 'automagic.txt' ? 'retune_deferred' : 'retune_candidate';
        statusLabel = tableName === 'automagic.txt' ? 'Retune PD2 différée' : 'Retune PD2 candidate';
        rationale = 'Différence techniquement portable, mais jamais approuvée produit.';
      } else if (mapped && pd2VsVanilla.length > 0) {
        status = 'pd2_modified';
        statusLabel = 'Modifié par PD2';
        rationale = 'PD2 change cette ligne vanilla; comparer avant toute adoption.';
      } else if (mapped) {
        status = 'shared';
        statusLabel = 'Commun';
        rationale = 'La ligne PD2 reste identique à vanilla sur les colonnes communes.';
      } else if (selectedAppend) {
        status = tableName === 'automagic.txt' ? 'pd2_new_deferred' : 'pd2_new_portable';
        statusLabel = tableName === 'automagic.txt' ? 'Nouveau PD2 différé' : 'Nouveau PD2 techniquement portable';
        rationale = tableName === 'automagic.txt'
          ? 'AutoMagic reste un lot séparé; aucune importation automatique.'
          : 'Les dépendances techniques connues ferment, mais Vincent doit choisir.';
      } else {
        status = 'pd2_new_review';
        statusLabel = blockedReason ? 'Nouveau PD2 bloqué' : 'Nouveau PD2 non retenu automatiquement';
        rationale = blockedReason
          ? `Blocage connu : ${blockedReason}.`
          : 'Hors allowlist conservatrice : dépendances, système map, compatibilité ou portée à examiner.';
      }
      const types = itemTypes(source, sourceRow);
      entries.push({
        id: `${tableName}:${sourceRow}`,
        table: tableName,
        tableLabel: config.label,
        source: 'PD2',
        sourceRow,
        targetRow,
        name: rowName(source, sourceRow),
        status,
        statusLabel,
        rationale,
        defaultDecision: ['shared'].includes(status) ? 'keep_bkvince' : 'undecided',
        level: value(source, sourceRow, 'level'),
        levelRequirement: value(source, sourceRow, 'levelreq'),
        frequency: value(source, sourceRow, 'frequency'),
        spawnable: value(source, sourceRow, 'spawnable'),
        rare: value(source, sourceRow, 'rare'),
        group: value(source, sourceRow, 'group'),
        effect: effect(source, sourceRow),
        allowedTypes: types.allowed,
        excludedTypes: types.excluded,
        pd2VsVanilla: compactDiffs(pd2VsVanilla, 'pd2', 'vanilla'),
        bkvVsVanilla: compactDiffs(bkvVsVanilla, 'bkvince', 'vanilla'),
      });
    }

    for (let targetRow = 0; targetRow < target.table.rows.length; targetRow += 1) {
      if (mappedTargets.has(targetRow) || !isRealRow(target, targetRow)) continue;
      const types = itemTypes(target, targetRow);
      entries.push({
        id: `${tableName}:bkv:${targetRow}`,
        table: tableName,
        tableLabel: config.label,
        source: 'BKVince',
        sourceRow: null,
        targetRow,
        name: rowName(target, targetRow),
        status: 'bkv_only',
        statusLabel: 'Propre à BKVince',
        rationale: 'Aucune ligne PD2 mappée ne possède cet ordinal BKVince.',
        defaultDecision: 'keep_bkvince',
        level: value(target, targetRow, 'level'),
        levelRequirement: value(target, targetRow, 'levelreq'),
        frequency: value(target, targetRow, 'frequency'),
        spawnable: value(target, targetRow, 'spawnable'),
        rare: value(target, targetRow, 'rare'),
        group: value(target, targetRow, 'group'),
        effect: effect(target, targetRow),
        allowedTypes: types.allowed,
        excludedTypes: types.excluded,
        pd2VsVanilla: [],
        bkvVsVanilla: [],
      });
    }
  }
  entries.sort((a, b) => a.table.localeCompare(b.table)
    || (a.sourceRow ?? Number.MAX_SAFE_INTEGER) - (b.sourceRow ?? Number.MAX_SAFE_INTEGER)
    || (a.targetRow ?? 0) - (b.targetRow ?? 0));
  for (const entry of entries) counts[entry.status] = (counts[entry.status] ?? 0) + 1;
  return {
    schemaVersion: 1,
    reviewId: 'pd2-affixes-review',
    state: 'review_only_no_import_approved',
    sourceAuthority: catalog.source.authority,
    sourceRoot,
    targetBaselineCommit: '756df5f53109729f16643b36aa459fead4cdbf94',
    decisionOptions: [
      { id: 'undecided', label: 'À décider' },
      { id: 'import_pd2', label: 'Importer PD2' },
      { id: 'keep_bkvince', label: 'Garder BKVince' },
      { id: 'exclude', label: 'Exclure' },
      { id: 'discuss', label: 'À discuter' },
    ],
    counts,
    entries,
  };
}

function escapeHtml(value) {
  return String(value).replace(/[&<>"']/g, (character) => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  })[character]);
}

function buildHtml(report) {
  const embedded = JSON.stringify(report).replace(/</g, '\\u003c');
  return `<!doctype html>
<html lang="fr"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Revue des affixes PD2 pour BKVince</title>
<style>
:root{color-scheme:dark;--bg:#101318;--panel:#191f27;--line:#303946;--text:#edf2f7;--muted:#a9b4c2;--accent:#f0a54a}*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:15px/1.45 system-ui,sans-serif}header{position:sticky;top:0;z-index:2;background:rgba(16,19,24,.97);padding:18px 24px;border-bottom:1px solid var(--line)}h1{margin:0 0 6px;font-size:24px}p{margin:4px 0;color:var(--muted)}.controls{display:flex;flex-wrap:wrap;gap:10px;margin-top:14px}.controls input,.controls select,.controls button,td select{background:#222a34;color:var(--text);border:1px solid #455264;border-radius:7px;padding:8px}.controls input{min-width:260px}.controls button{cursor:pointer;border-color:#8c6537}.stats{padding:14px 24px;display:flex;flex-wrap:wrap;gap:8px}.badge{background:var(--panel);border:1px solid var(--line);border-radius:999px;padding:5px 10px}main{padding:0 24px 32px;overflow:auto}table{width:100%;border-collapse:collapse;background:var(--panel)}th,td{padding:9px 10px;border-bottom:1px solid var(--line);vertical-align:top;text-align:left}th{position:sticky;top:153px;background:#222a34;z-index:1}tr:hover{background:#202833}.name{font-weight:700}.muted{color:var(--muted);font-size:13px}.status{white-space:nowrap}.diff{max-width:340px}.hidden{display:none}.warning{color:#ffd28c}.ok{color:#9dd6a5}@media(max-width:900px){th:nth-child(5),td:nth-child(5),th:nth-child(7),td:nth-child(7){display:none}header{position:static}th{top:0}}
</style></head><body>
<header><h1>Revue des affixes PD2 pour BKVince</h1><p><strong>Aucun import n'est approuvé.</strong> Cette page compare vanilla, PD2 S13 et la baseline BKVince restaurée.</p>
<div class="controls"><input id="search" placeholder="Rechercher un nom, effet ou type d'objet"><select id="status"><option value="">Tous les statuts</option></select><select id="decision"><option value="">Toutes les décisions</option></select><button id="export">Exporter mes décisions</button><button id="reset">Réinitialiser</button></div></header>
<div class="stats" id="stats"></div><main><table><thead><tr><th>Décision</th><th>Affixe</th><th>Statut</th><th>Effet PD2/BKV</th><th>Objets</th><th>Niveaux</th><th>Comparaison</th></tr></thead><tbody id="rows"></tbody></table></main>
<script>const report=${embedded};const key='pd2-affixes-review-decisions-v1';let decisions=JSON.parse(localStorage.getItem(key)||'{}');
const labels=Object.fromEntries(report.decisionOptions.map(x=>[x.id,x.label]));const status=document.querySelector('#status'),decision=document.querySelector('#decision'),search=document.querySelector('#search'),rows=document.querySelector('#rows');
Object.entries(report.counts).sort((a,b)=>a[0].localeCompare(b[0])).forEach(([id,count])=>{const e=report.entries.find(x=>x.status===id);status.add(new Option(e.statusLabel+' ('+count+')',id))});report.decisionOptions.forEach(x=>decision.add(new Option(x.label,x.id)));
document.querySelector('#stats').innerHTML='<span class="badge">'+report.entries.length+' lignes comparées</span>'+Object.entries(report.counts).map(([id,n])=>'<span class="badge">'+report.entries.find(x=>x.status===id).statusLabel+': '+n+'</span>').join('');
const esc=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
function current(e){return decisions[e.id]||e.defaultDecision}function render(){const q=search.value.trim().toLowerCase();const visible=report.entries.filter(e=>(!status.value||e.status===status.value)&&(!decision.value||current(e)===decision.value)&&(!q||[e.name,e.effect,e.statusLabel,...e.allowedTypes,...e.excludedTypes].join(' ').toLowerCase().includes(q)));rows.innerHTML=visible.map(e=>{const opts=report.decisionOptions.map(o=>'<option value="'+o.id+'" '+(current(e)===o.id?'selected':'')+'>'+esc(o.label)+'</option>').join('');const changes=e.pd2VsVanilla.slice(0,6).map(d=>esc(d.column)+': '+esc(d.vanilla||'∅')+' → '+esc(d.pd2||'∅')).join('<br>');return '<tr><td><select data-id="'+esc(e.id)+'">'+opts+'</select></td><td><div class="name">'+esc(e.name)+'</div><div class="muted">'+esc(e.tableLabel)+' · PD2 '+esc(e.sourceRow??'—')+' · BKV '+esc(e.targetRow??'—')+'</div></td><td class="status"><div>'+esc(e.statusLabel)+'</div><div class="muted">'+esc(e.rationale)+'</div></td><td>'+esc(e.effect)+'</td><td>'+esc(e.allowedTypes.join(', ')||'—')+(e.excludedTypes.length?'<div class="muted">exclut '+esc(e.excludedTypes.join(', '))+'</div>':'')+'</td><td>alvl '+esc(e.level||'—')+'<div class="muted">req '+esc(e.levelRequirement||'—')+' · freq '+esc(e.frequency||'—')+'</div></td><td class="diff">'+(changes||'<span class="muted">aucun changement vanilla détecté</span>')+'</td></tr>'}).join('');rows.querySelectorAll('select').forEach(s=>s.onchange=()=>{decisions[s.dataset.id]=s.value;localStorage.setItem(key,JSON.stringify(decisions));render()})}
[search,status,decision].forEach(x=>x.oninput=render);document.querySelector('#reset').onclick=()=>{if(confirm('Réinitialiser toutes les décisions locales ?')){decisions={};localStorage.removeItem(key);render()}};document.querySelector('#export').onclick=()=>{const payload={schemaVersion:1,reviewId:report.reviewId,decisions:Object.fromEntries(report.entries.map(e=>[e.id,current(e)]).filter(([,v])=>v!=='undecided'))};const blob=new Blob([JSON.stringify(payload,null,2)+'\\n'],{type:'application/json'});const a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download='pd2-affixes-decisions.json';a.click();URL.revokeObjectURL(a.href)};render();</script></body></html>\n`;
}

function run(args = process.argv.slice(2)) {
  const check = args.includes('--check');
  const sourceRoot = sourceRootFromArgs(args);
  const catalog = readJson(catalogPath);
  if (check && !fs.existsSync(sourceRoot)) {
    const expected = catalog.review;
    assert(expected, 'Catalog has no pinned review artifact expectations');
    const json = fs.readFileSync(outputJson);
    const html = fs.readFileSync(outputHtml);
    const stored = JSON.parse(json.toString('utf8'));
    assert(stored.entries.length === expected.entries, 'Stored review entry count drift');
    assert(JSON.stringify(stored.counts) === JSON.stringify(expected.counts), 'Stored review counts drift');
    assert(sha256(json) === expected.jsonSha256, 'Stored review JSON hash drift');
    assert(sha256(html) === expected.htmlSha256, 'Stored review HTML hash drift');
    console.log(JSON.stringify({ mode: 'check-pinned', entries: stored.entries.length, counts: stored.counts }, null, 2));
    return;
  }
  const report = buildReport(sourceRoot, catalog);
  const json = `${JSON.stringify(report, null, 2)}\n`;
  const html = buildHtml(report);
  if (check) {
    assert(fs.existsSync(outputJson) && fs.readFileSync(outputJson, 'utf8') === json, 'Review JSON is stale');
    assert(fs.existsSync(outputHtml) && fs.readFileSync(outputHtml, 'utf8') === html, 'Review HTML is stale');
  } else {
    fs.writeFileSync(outputJson, json, 'utf8');
    fs.writeFileSync(outputHtml, html, 'utf8');
  }
  console.log(JSON.stringify({
    mode: check ? 'check' : 'write',
    entries: report.entries.length,
    counts: report.counts,
    jsonSha256: sha256(Buffer.from(json)),
    htmlSha256: sha256(Buffer.from(html)),
  }, null, 2));
}

try {
  run();
} catch (error) {
  console.error(`INVALID PD2 Affixes Review: ${error.message}`);
  process.exitCode = 1;
}
