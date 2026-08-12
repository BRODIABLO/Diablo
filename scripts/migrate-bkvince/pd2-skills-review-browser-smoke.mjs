#!/usr/bin/env node

/**
 * Real-browser operational smoke and performance gate for the self-contained
 * PD2 Skills Merge Workbench. This script deliberately opens the generated
 * artifact through file://; serving it over HTTP would not validate the product
 * contract or its localStorage behaviour.
 */

import assert from 'node:assert/strict';
import { readFile, stat } from 'node:fs/promises';
import { execFile } from 'node:child_process';
import { fileURLToPath, pathToFileURL } from 'node:url';
import path from 'node:path';
import { promisify } from 'node:util';
import { chromium } from 'playwright';
const execFileAsync = promisify(execFile);
const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const HTML_PATH = path.resolve(process.env.PD2_SKILLS_WORKBENCH_HTML || path.join(ROOT, 'Mission', 'pd2-skills-review.html'));
const JSON_PATH = path.resolve(process.env.PD2_SKILLS_WORKBENCH_JSON || path.join(ROOT, 'Mission', 'pd2-skills-review.json'));
const HTML_URL = pathToFileURL(HTML_PATH).href;

// Wide ceilings catch order-of-magnitude regressions while remaining stable on
// slower GitHub-hosted runners. The measured values remain visible in the log.
export const PERFORMANCE_THRESHOLDS = Object.freeze({
  coldLoadMs: 20_000,
  timeToInteractiveMs: 20_000,
  peakBrowserRssBytes: 1_500 * 1024 * 1024,
  peakJsHeapBytes: 512 * 1024 * 1024,
  globalSearchMs: 3_000,
  filterMs: 3_000,
  classChangeMs: 3_000,
  decisionsExportMs: 10_000,
});

const errors = [];
const checkpoints = [];
const startWall = Date.now();

function elapsedMs(start) {
  return Number((performance.now() - start).toFixed(2));
}

function mib(bytes) {
  return Number((bytes / 1024 / 1024).toFixed(2));
}

async function browserRssBytes(browserSession) {
  const { processInfo = [] } = await browserSession.send('SystemInfo.getProcessInfo');
  const pids = [...new Set(processInfo.map((item) => Number(item.id)).filter(Number.isInteger))];
  if (!pids.length) return null;
  if (process.platform === 'win32') {
    const command = `$ids=@(${pids.join(',')}); $sum=(Get-Process -Id $ids -ErrorAction SilentlyContinue | Measure-Object WorkingSet64 -Sum).Sum; if ($null -eq $sum) { 0 } else { [Int64]$sum }`;
    const { stdout } = await execFileAsync('powershell.exe', ['-NoProfile', '-NonInteractive', '-Command', command], { windowsHide: true });
    return Number.parseInt(stdout.trim(), 10) || null;
  }
  if (process.platform === 'linux') {
    let totalKb = 0;
    for (const pid of pids) {
      try {
        const status = await readFile(`/proc/${pid}/status`, 'utf8');
        totalKb += Number.parseInt(/^VmRSS:\s+(\d+)\s+kB$/mu.exec(status)?.[1] || '0', 10);
      } catch {
        // Chromium utility processes can exit between enumeration and sampling.
      }
    }
    return totalKb ? totalKb * 1024 : null;
  }
  return null;
}

async function main() {
  const [htmlStat, jsonStat] = await Promise.all([stat(HTML_PATH), stat(JSON_PATH)]);
  const browser = await chromium.launch({
    headless: true,
    args: ['--allow-file-access-from-files', '--enable-precise-memory-info'],
  });
  const context = await browser.newContext({ acceptDownloads: true, viewport: { width: 1600, height: 1000 } });
  const page = await context.newPage();
  const pageSession = await context.newCDPSession(page);
  const browserSession = await browser.newBrowserCDPSession();
  await pageSession.send('Performance.enable');

  const memory = { peakBrowserRssBytes: 0, peakJsHeapBytes: 0, observations: [], diagnosticWarnings: [] };
  async function sampleMemory(label) {
    const { metrics = [] } = await pageSession.send('Performance.getMetrics');
    const jsHeap = metrics.find((metric) => metric.name === 'JSHeapUsedSize')?.value || 0;
    memory.peakJsHeapBytes = Math.max(memory.peakJsHeapBytes, jsHeap);
    let rss = null;
    try {
      rss = await browserRssBytes(browserSession);
      if (rss) memory.peakBrowserRssBytes = Math.max(memory.peakBrowserRssBytes, rss);
    } catch (error) {
      memory.diagnosticWarnings.push(`${label}: ${error.message}`);
    }
    memory.observations.push({ label, browserRssBytes: rss, jsHeapBytes: jsHeap });
  }

  page.on('pageerror', (error) => errors.push(`pageerror: ${error.stack || error.message}`));
  page.on('console', (message) => {
    if (message.type() === 'error') errors.push(`console.error: ${message.text()}`);
  });

  try {
    const navigationStart = performance.now();
    checkpoints.push('browser launched; file navigation starting');
    const response = await page.goto(HTML_URL, { waitUntil: 'load', timeout: PERFORMANCE_THRESHOLDS.coldLoadMs });
    // Chromium represents the local document as a synthetic 200 response. The
    // URL, rather than the response object's presence, is the protocol gate.
    assert.equal(response?.status(), 200, 'Chromium must complete the local document load');
    assert.equal(new URL(response.url()).protocol, 'file:', 'The Workbench must be loaded directly through file://');
    checkpoints.push('file navigation loaded');
    const bootstrap = await page.evaluate(async () => {
      if (globalThis.__PD2_SKILLS_WORKBENCH_READY__) await globalThis.__PD2_SKILLS_WORKBENCH_READY__;
      if (globalThis.__PD2_SKILLS_WORKBENCH_ERROR__) throw globalThis.__PD2_SKILLS_WORKBENCH_ERROR__;
      return { compressedBootstrap: Boolean(globalThis.__PD2_SKILLS_WORKBENCH_READY__), skillCount: globalThis.__PD2_SKILLS_REPORT__?.skills?.length || null };
    });
    if (bootstrap.compressedBootstrap) assert.ok(bootstrap.skillCount > 0, 'The compressed oracle must be decoded before interaction');
    checkpoints.push('oracle bootstrap completed');
    const coldLoadMs = elapsedMs(navigationStart);
    await page.locator('#global-search').waitFor({ state: 'visible', timeout: 10_000 });
    checkpoints.push('search control visible');
    await page.evaluate(() => new Promise((resolve) => requestAnimationFrame(() => requestAnimationFrame(resolve))));
    checkpoints.push('two animation frames completed');
    const timeToInteractiveMs = elapsedMs(navigationStart);
    assert.equal(await page.locator('h1').textContent(), 'PD2 Skills Merge Workbench');
    assert.equal(await page.locator('#global-search').isEnabled(), true);
    assert.match(await page.locator('.non-mutation').innerText(), /file:\/\//u);
    checkpoints.push('initial UI assertions passed');
    await sampleMemory('cold-ready');
    checkpoints.push('initial memory sampled');

    const classStart = performance.now();
    await page.locator('[data-view="sor"]').click();
    await page.locator('[data-view="sor"].active').waitFor();
    await page.locator('#dashboard-title').filter({ hasText: 'Sorceress' }).waitFor();
    const classChangeMs = elapsedMs(classStart);
    checkpoints.push('Sorceress opened');

    const necromancerStart = performance.now();
    await page.locator('[data-view="nec"]').click();
    await page.locator('[data-view="nec"].active').waitFor();
    await page.locator('#dashboard-title').filter({ hasText: 'Necromancer' }).waitFor();
    const necromancerClassChangeMs = elapsedMs(necromancerStart);
    checkpoints.push('Necromancer opened');

    const filterStart = performance.now();
    await page.locator('[data-filter="significant"]').check();
    await page.locator('.result-summary').filter({ hasText: 'différences significatives' }).waitFor();
    const filterMs = elapsedMs(filterStart);
    await page.locator('[data-action="clear-filters"]').click();

    const searchStart = performance.now();
    await page.locator('#global-search').fill('Amplify Damage');
    const amplifyCard = page.locator('article[data-skill-id="skill:nec:amplify-damage"]');
    await amplifyCard.waitFor({ state: 'visible', timeout: PERFORMANCE_THRESHOLDS.globalSearchMs });
    const globalSearchMs = elapsedMs(searchStart);
    assert.match(await amplifyCard.innerText(), /Amplify Damage/u);
    await amplifyCard.locator('[data-action="toggle-skill"]').click();
    await amplifyCard.locator('.skill-card-body').waitFor();

    await amplifyCard.locator('[data-global-decision]').selectOption('ADAPT_PD2_SELECTIVELY');
    await amplifyCard.locator('[data-component-decision][data-component-id="cost_timing"]').selectOption('KEEP_BKVINCE');
    await amplifyCard.locator('[data-component-decision][data-component-id="area_targeting"]').selectOption('ADOPT_PD2');
    await amplifyCard.locator('[data-component-decision][data-component-id="buffs_debuffs_auras_passives"]').selectOption('CUSTOM');
    const customValue = 'Hybrid smoke value: max(8, 4 + lvl / 2)';
    const customJustification = 'Smoke test: preserve BKVince power while reviewing PD2 curse behaviour.';
    await amplifyCard.locator('[data-choice-level="component"][data-component-id="buffs_debuffs_auras_passives"][data-choice-property="customValue"]').fill(customValue);
    await amplifyCard.locator('[data-choice-level="component"][data-component-id="buffs_debuffs_auras_passives"][data-choice-property="justification"]').fill(customJustification);
    await page.waitForFunction(({ skillId, expectedValue, expectedJustification }) => {
      const key = Object.keys(localStorage).find((item) => item.startsWith('pd2-skills-review-decisions-v1:'));
      if (!key) return false;
      const entry = JSON.parse(localStorage.getItem(key))?.entries?.[skillId];
      const custom = entry?.componentDecisions?.buffs_debuffs_auras_passives;
      return entry?.globalDecision === 'ADAPT_PD2_SELECTIVELY'
        && entry?.componentDecisions?.cost_timing?.decision === 'KEEP_BKVINCE'
        && entry?.componentDecisions?.area_targeting?.decision === 'ADOPT_PD2'
        && custom?.decision === 'CUSTOM'
        && custom?.customValue === expectedValue
        && custom?.justification === expectedJustification;
    }, { skillId: 'skill:nec:amplify-damage', expectedValue: customValue, expectedJustification: customJustification });
    checkpoints.push('Amplify Damage hybrid and CUSTOM decisions saved');
    await sampleMemory('decisions-saved');

    await page.reload({ waitUntil: 'load', timeout: PERFORMANCE_THRESHOLDS.coldLoadMs });
    await page.evaluate(async () => {
      if (globalThis.__PD2_SKILLS_WORKBENCH_READY__) await globalThis.__PD2_SKILLS_WORKBENCH_READY__;
      if (globalThis.__PD2_SKILLS_WORKBENCH_ERROR__) throw globalThis.__PD2_SKILLS_WORKBENCH_ERROR__;
    });
    await page.locator('#global-search').waitFor({ state: 'visible' });
    await page.locator('#global-search').fill('Amplify Damage');
    await amplifyCard.waitFor({ state: 'visible' });
    await amplifyCard.locator('[data-action="toggle-skill"]').click();
    await amplifyCard.locator('.skill-card-body').waitFor();
    assert.equal(await amplifyCard.locator('[data-global-decision]').inputValue(), 'ADAPT_PD2_SELECTIVELY');
    assert.equal(await amplifyCard.locator('[data-component-decision][data-component-id="cost_timing"]').inputValue(), 'KEEP_BKVINCE');
    assert.equal(await amplifyCard.locator('[data-component-decision][data-component-id="area_targeting"]').inputValue(), 'ADOPT_PD2');
    assert.equal(await amplifyCard.locator('[data-choice-property="customValue"][data-component-id="buffs_debuffs_auras_passives"]').inputValue(), customValue);
    assert.equal(await amplifyCard.locator('[data-choice-property="justification"][data-component-id="buffs_debuffs_auras_passives"]').inputValue(), customJustification);
    checkpoints.push('localStorage restored after reload');

    const decisionsExportStart = performance.now();
    const decisionsDownloadPromise = page.waitForEvent('download');
    await page.locator('[data-action="export-all"]').click();
    const decisionsDownload = await decisionsDownloadPromise;
    const decisionsExportMs = elapsedMs(decisionsExportStart);
    assert.equal(decisionsDownload.suggestedFilename(), 'pd2-skills-decisions-all.json');
    const decisionsPath = await decisionsDownload.path();
    const exportedDecisions = JSON.parse(await readFile(decisionsPath, 'utf8'));
    assert.equal(exportedDecisions.entries['skill:nec:amplify-damage'].globalDecision, 'ADAPT_PD2_SELECTIVELY');
    checkpoints.push('decisions exported');

    page.once('dialog', (dialog) => dialog.accept());
    await page.locator('[data-action="reset-global"]').click();
    const chooserPromise = page.waitForEvent('filechooser');
    await page.locator('[data-action="import"]').click();
    const chooser = await chooserPromise;
    await chooser.setFiles(decisionsPath);
    // importFile renders immediately after its transient success notice, so the
    // governed envelope in localStorage is the durable success witness.
    await page.waitForFunction((skillId) => {
      const key = Object.keys(localStorage).find((item) => item.startsWith('pd2-skills-review-decisions-v1:'));
      return Boolean(key && JSON.parse(localStorage.getItem(key))?.entries?.[skillId]?.globalDecision === 'ADAPT_PD2_SELECTIVELY');
    }, 'skill:nec:amplify-damage');
    checkpoints.push('decisions reimported');

    await page.locator('[data-view="nec"]').click();
    const classDownloadPromise = page.waitForEvent('download');
    await page.locator('[data-action="export-class"][data-view-id="nec"]').click();
    const classDownload = await classDownloadPromise;
    assert.equal(classDownload.suggestedFilename(), 'necromancer-pd2-skills-review.md');
    const classMarkdown = await readFile(await classDownload.path(), 'utf8');
    assert.match(classMarkdown, /^# Dossier de révision — Necromancer/mu);
    assert.match(classMarkdown, /# Amplify Damage — PD2 Skills Merge Workbench/u);
    checkpoints.push('Necromancer Markdown dossier exported');

    await page.locator('[data-action="next-incomplete"]').click();
    await page.locator('[data-action="toggle-skill"][aria-expanded="true"]').first().waitFor();
    assert.ok(await page.locator('.skill-card-body').count() >= 1);
    checkpoints.push('next incomplete skill opened');

    await page.locator('[data-view="pd2_new"]').click();
    const combustionCard = page.locator('article[data-skill-id="skill:sor:combustion"]');
    await combustionCard.waitFor();
    assert.match(await combustionCard.innerText(), /Nouveau skill PD2/iu);
    await combustionCard.locator('[data-action="toggle-skill"]').click();
    assert.match(await combustionCard.locator('.new-skill-plan').innerText(), /Gate de ligne append-only/u);
    assert.match(await combustionCard.locator('.new-skill-plan').innerText(), /Cible append-only proposée/u);
    checkpoints.push('new PD2 skill append-only gate verified');

    await page.locator('[data-view="war"]').click();
    const warlockCollisionCard = page.locator('article[data-skill-id="skill:war:summon-tainted"]');
    await warlockCollisionCard.waitFor();
    assert.match(await warlockCollisionCard.innerText(), /1 collision\(s\)/u);
    await warlockCollisionCard.locator('[data-action="toggle-skill"]').click();
    const collisionText = await warlockCollisionCard.locator('.collisions').innerText();
    assert.match(collisionText, /Aucune fusion automatique/u);
    assert.match(collisionText, /Combustion/u);
    checkpoints.push('Warlock ordinal collision verified');

    await page.locator('#global-search').fill('Fire Ball');
    const fireBallCard = page.locator('article[data-skill-id="skill:sor:fire-ball"]');
    await fireBallCard.waitFor({ state: 'visible', timeout: PERFORMANCE_THRESHOLDS.globalSearchMs });
    assert.match(await fireBallCard.innerText(), /MALFORMED_SOURCE/u);
    checkpoints.push('Fire Ball MALFORMED_SOURCE displayed');

    await sampleMemory('final-smoke-state');

    const result = {
      status: 'PASS',
      artifact: {
        htmlPath: HTML_PATH,
        htmlUrl: HTML_URL,
        htmlBytes: htmlStat.size,
        jsonPath: JSON_PATH,
        jsonBytes: jsonStat.size,
      },
      environment: {
        node: process.version,
        platform: `${process.platform}/${process.arch}`,
        chromium: browser.version(),
        playwright: JSON.parse(await readFile(path.join(ROOT, 'node_modules', 'playwright', 'package.json'), 'utf8')).version,
        protocol: new URL(page.url()).protocol,
      },
      performance: {
        coldLoadMs,
        timeToInteractiveMs,
        globalSearchMs,
        filterMs,
        classChangeMs,
        necromancerClassChangeMs,
        decisionsExportMs,
        peakBrowserRssBytes: memory.peakBrowserRssBytes || null,
        peakBrowserRssMiB: memory.peakBrowserRssBytes ? mib(memory.peakBrowserRssBytes) : null,
        peakJsHeapBytes: memory.peakJsHeapBytes,
        peakJsHeapMiB: mib(memory.peakJsHeapBytes),
      },
      thresholds: PERFORMANCE_THRESHOLDS,
      checkpoints,
      javascriptErrors: errors,
      memoryDiagnostics: {
        observations: memory.observations,
        warnings: memory.diagnosticWarnings,
      },
      durationMs: Date.now() - startWall,
    };

    for (const [metric, ceiling] of Object.entries(PERFORMANCE_THRESHOLDS)) {
      if (result.performance[metric] == null) continue;
      assert.ok(result.performance[metric] <= ceiling, `${metric} ${result.performance[metric]} exceeded ceiling ${ceiling}`);
    }
    assert.equal(errors.length, 0, `JavaScript errors were captured:\n${errors.join('\n')}`);
    assert.equal(result.environment.protocol, 'file:', 'The final page must remain usable through file://');
    console.log(JSON.stringify(result, null, 2));
  } finally {
    await context.close();
    await browser.close();
  }
}

main().catch((error) => {
  console.error(JSON.stringify({
    status: 'FAIL',
    error: error.stack || error.message,
    checkpoints,
    javascriptErrors: errors,
    durationMs: Date.now() - startWall,
  }, null, 2));
  process.exitCode = 1;
});
