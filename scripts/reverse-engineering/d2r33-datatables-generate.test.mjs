import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';
import assert from 'node:assert/strict';
import {
  buildGeneratedOutputs,
  defaultGeneratedDirectory,
} from './d2r33-datatables-generate.mjs';
import { defaultCatalogPath } from './d2r33-datatables-atlas.mjs';

const catalogBytes = fs.readFileSync(defaultCatalogPath);
const catalog = JSON.parse(catalogBytes.toString('utf8'));

function sha256(value) {
  return crypto.createHash('sha256').update(value).digest('hex').toUpperCase();
}

test('two generations from the same catalog are byte-identical', () => {
  const first = buildGeneratedOutputs(catalog, catalogBytes);
  const second = buildGeneratedOutputs(catalog, catalogBytes);
  assert.deepEqual([...first.entries()], [...second.entries()]);
});

test('checked-in generated outputs are current', () => {
  const expected = buildGeneratedOutputs(catalog, catalogBytes);
  for (const [name, content] of expected) {
    assert.equal(fs.readFileSync(path.join(defaultGeneratedDirectory, name), 'utf8'), content, name);
  }
});

test('C++ output asserts every admitted record field and DataTables slot', () => {
  const header = buildGeneratedOutputs(catalog, catalogBytes).get('d2r33_datatables_atlas.hpp');
  const expectedFieldOffsets = catalog.tables.reduce((total, table) => total + table.recordFields.length, 0);
  const expectedSlots = catalog.tables.reduce((total, table) => (
    total + ['records', 'count', 'linker'].filter((kind) => table[kind] && table[kind].status !== 'unknown').length
  ), 0);
  assert.equal((header.match(/static_assert\(offsetof\(/gu) ?? []).length, expectedFieldOffsets + expectedSlots);
  assert.equal((header.match(/static_assert\(sizeof\([^)]*RecordView\)/gu) ?? []).length, catalog.tables.length);
  assert.match(header, /static_assert\(sizeof\(std::uintptr_t\) == 8/u);
});

test('candidate fields remain explicitly labelled in both formats', () => {
  const outputs = buildGeneratedOutputs(catalog, catalogBytes);
  for (const name of ['d2r33_datatables_atlas.hpp', 'd2r33_datatables_ghidra.h']) {
    const content = outputs.get(name);
    assert.match(content, /int32_t parm0; \/\/ \+0x134 candidate; evidence: objects-parm0\./u);
    assert.match(content, /uint8_t levelMin; \/\/ \+0x18 candidate; evidence: shrines-level-min\./u);
  }
});

test('unknown regions are emitted only as byte padding', () => {
  const outputs = buildGeneratedOutputs(catalog, catalogBytes);
  const cpp = outputs.get('d2r33_datatables_atlas.hpp');
  const ghidra = outputs.get('d2r33_datatables_ghidra.h');
  assert.match(cpp, /std::uint8_t unknown0000\[/u);
  assert.match(ghidra, /uint8_t unknown0000\[/u);
  assert.doesNotMatch(cpp, /Unknown[A-Za-z]+;/u);
});

test('Ghidra output exposes all record sizes and known DataTables offsets', () => {
  const header = buildGeneratedOutputs(catalog, catalogBytes).get('d2r33_datatables_ghidra.h');
  for (const table of catalog.tables) {
    assert.match(header, new RegExp(`#define D2R33_${table.id.replaceAll('-', '_').toUpperCase()}_RECORD_SIZE ${table.recordSize.value}U`, 'u'));
  }
  assert.match(header, /typedef struct D2R33_DataTablesKnownPrefixView/u);
  assert.match(header, /#define D2R33_DATATABLES_SHRINES_COUNT_OFFSET 0x19B8U/u);
});

test('manifest hashes and byte counts cover every generated code output', () => {
  const outputs = buildGeneratedOutputs(catalog, catalogBytes);
  const manifest = JSON.parse(outputs.get('manifest.json'));
  assert.equal(manifest.catalogSha256, sha256(catalogBytes));
  assert.equal(manifest.outputs.length, 3);
  for (const output of manifest.outputs) {
    const content = outputs.get(output.name);
    assert.equal(output.bytes, Buffer.byteLength(content, 'utf8'));
    assert.equal(output.sha256, sha256(content));
  }
});

test('catalog bytes cannot be detached from the validated catalog object', () => {
  const mutated = structuredClone(catalog);
  mutated.targetRuntime.version = 'detached';
  assert.throws(
    () => buildGeneratedOutputs(mutated, catalogBytes),
    /catalog is invalid|catalogBytes do not represent/u,
  );
});
