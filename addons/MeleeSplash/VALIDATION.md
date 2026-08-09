# MeleeSplash 0.1.0 validation record

This record separates reproducible static evidence, current-stack runtime
qualification, prepared gameplay tests, and repository-wide closure gates.

## Current status

| Gate | Status | Evidence or remaining action |
|---|---|---|
| Source review | **PASS** | The final combat-semantics review issued its explicit GO on the frozen source snapshot. |
| Release build | **PASS** | Two clean x64 Release builds produced the same SHA-256 and staged package artifact. |
| Automated configuration policy | **PASS** | CTest passed 1/1 on both frozen-source builds. |
| PE/ABI inspection | **PASS** | The final DLL is x64, carries API resource v2, and exposes only the three D2RLoader exports. |
| Static hook ownership | **PASS FOR CURRENT WORKSPACE** | No exact overlap was found against the governed PluginPack manifest, FloatingDamage, standalone source-visible addons, or active BKVince patch sites. Runtime ownership remains fail-closed. |
| BKVince table/config checks | **PASS** | `npm run test:bkvince-melee-splash-stats` passes with the default-off host profile and reserved IDs. |
| Current-stack runtime coexistence | **PASS** | Global-enabled, mod-local shadow-enabled, and final mod-local default-off cold starts completed with no failed or rejected plugin. |
| Formal every-feature PluginPack matrix | **BOUNDED BY PRE-EXISTING CONFLICTS** | Existing non-Melee collisions prevent mutually conflicting PluginPack features from all being enabled at once; this does not invalidate MeleeSplash on the qualified current stack. |
| Solo gameplay smoke A-H | **PARTIAL PASS AFTER FIX** | The first attempt exposed a missing governed Fill caller. After the correction, A passed with one successful normal Attack and a three-target burst; E, F, H and G-active also passed in diagnostics. B, C, D and the default-off visual rollback half of G remain not run. |
| Public ZIP | **PASS** | The final archive contains exactly the validated default-off DLL and JSON at its root. |

## Build and automated checks

The governed build command is:

```text
npm run native:build -- -Project MeleeSplash
```

CMake pins D2RLoader PluginSDK commit
`efcfaaa52eeec9e379b3fc2aad1013bb3dddc970` and nlohmann/json `v3.11.3`,
targets x64 Release, enables `/W4 /WX /permissive- /utf-8`, and runs the
`melee-splash-config-policy` CTest against both shipped generic JSON profiles.
Its post-build step stages only the DLL into `package/`; it does not deploy to
a game installation or create the public ZIP.

The latest preserved final-build test log is:

```text
analysis-cache/native-build/MeleeSplash/Release/Testing/Temporary/LastTest.log
2026-08-09 09:53:35 -04:00
1/1 melee-splash-config-policy: PASS
```

Two consecutive builds of the corrected source both passed CTest and produced:

```text
DBA0C40C191B2568A6B39D21324A45F770C1CBF8AD747B099AA3BCBEDEF8856C
199168 bytes
```

One configure attempt made while another agent was building the same derived
FetchContent directory failed before compilation with Ninja `failed
recompaction`. No source or runtime file changed in that attempt. The build was
not counted; the two subsequent isolated clean builds are the reproducibility
evidence above.

## Artifact ledger

These hashes describe the frozen build and packaging snapshot.

| Artifact | Modified (-04:00) | SHA-256 | Status |
|---|---|---|---|
| `src/plugin.cpp` | 2026-08-09 13:09:42 | `01C77FFDF6CA478EFA711CBC9BD24985600D5D8855B85C2F5694E625FF767B7A` | Corrected source with the governed direct-melee Fill caller and bounded rejection diagnostics |
| `package/MeleeSplash.dll` | 2026-08-09 13:09:53 | `DBA0C40C191B2568A6B39D21324A45F770C1CBF8AD747B099AA3BCBEDEF8856C` | Corrected build; byte-equal to build output and BKVince plugin copy |
| BKVince plugin copy | 2026-08-09 13:09:53 | `DBA0C40C191B2568A6B39D21324A45F770C1CBF8AD747B099AA3BCBEDEF8856C` | Byte-equal to corrected build and package |
| `package/MeleeSplash.json` | 2026-08-09 05:37:00 | `6AA40B37051189ADE2CA5D60FE133765EE1D426E0B6DA5E2059B619E77030C20` | Generic, default-off |
| Enabled generic example | 2026-08-09 05:37:00 | `CAE51A8A98F1BBA1278263DB7D3137D4DBFFC7DD8073670C65427F090A06B1EA` | Test/example only; excluded from ZIP |
| BKVince configuration | 2026-08-09 05:26:09 | `32747A94E8744C39813AEBC8EA32BD284AA12F10A5E898DA7272D3721D06E5E6` | Separate, default-off host profile; excluded from ZIP |
| `MeleeSplash-0.1.0.zip` | 2026-08-09 16:05:28 | `F137F1B708A4C51C8A88EA68B49BFB85619F380693E63899B508EB1C342E35A9` | Strict two-file public archive, 89072 bytes |

## PE and public ABI inspection

The frozen DLL inspection established:

- AMD64/x64 PE;
- D2RLoader plugin API resource value `2`;
- exactly `D2RLoaderGetPluginInfo`, `D2RLoaderLoadPlugin`, and
  `D2RLoaderUnloadPlugin` as exports;
- version `0.1.0`, company/author `RuffnecKk`, internal name
  `MeleeSplash`, and original filename `MeleeSplash.dll`;
- `NativeHooks` without `ModScopedOnly`, supporting global or mod-local scope;
- imports restricted to Windows/UCRT/MSVC runtime libraries, with no import,
  modification, link, or redistribution of an eezstreet DLL;
- no BKVince-specific name or path embedded in the generic DLL.

The exact imports are `KERNEL32.dll`, `MSVCP140.dll`, `VCRUNTIME140.dll`,
`VCRUNTIME140_1.dll`, and the required UCRT API-set libraries for runtime,
stdio, filesystem, heap, conversion, locale, math, and strings. The PE has a
reproducible-build debug record and no embedded PDB path. A binary scan for the
BKVince name, paths, legacy skill/missile identifiers, and custom stat names
returned zero hits.

Final inspection tools were Visual Studio Build Tools `17.14.37502.11`, MSVC
`19.44.35228`, COFF/PE Dumper `14.44.35228.0`, CMake `3.31.6-msvc6`, and Ninja
`1.12.1`.

## Static ownership and native surfaces

`NATIVE-HOOKS.md` records all owned RVAs, literal expected bytes, caller gates,
callback ABI, helper chaining policy, and the cold-rollback limitation for D2R
3.2.92777. `Mission/mechanics-native-proof-92777.md` and the governed
`known-rvas.json` retain the static caller/callee and damage-layout evidence.
`npm run re:d2r32 -- status` passed on 2026-08-09 with the canonical and
analysis images, persistent index, and Ghidra project all verified.

The current workspace scan found no exact write-range collision with:

- the governed 139-site PluginPack manifest;
- the 57 governed active BKVince patch sites;
- FloatingDamage's observer surface;
- current source-visible standalone addons.

This is a compatibility precondition, not proof for black-box binaries or
future versions. Every owned byte range is validated before mutation, and a
mismatch fails closed. Current-stack runtime coexistence is qualified below.

## BKVince data and integration checks

The targeted non-runtime command is:

```text
npm run test:bkvince-melee-splash-stats
```

It currently passes for stat IDs 391/392, property IDs 310/311, localization
IDs 65028/65029, TSV byte policy, and the separate default-off BKVince
configuration. The legacy EventFunc20 suppression is host-configured and
reversible; no historical skill, missile, state, or item row is deleted.
The governed BKVince DLL copy is byte-equal to the final package and qualified
mod-local runtime DLL.

## Current-stack runtime qualification

Runtime evidence is preserved outside the release payload under:

```text
analysis-cache/meleesplash-runtime-validation/20260809-095701652/
```

The original cold-start matrix used the frozen `D9A49607...21B0` artifact. The
corrected gameplay build is `DBA0C40C...8856C`. The final BKVince
configuration is default-off with SHA-256 `32747A94...6E5E6`. Three D2R
3.2.92777 cold starts were recorded:

1. **global enabled:** all twelve entry hooks and two call relays installed;
   18/18 BKVince patches applied, 17/17 plugins active, 0 disabled/rejected/
   failed, and frontend startup reached 24/24;
2. **mod-local enabled with global shadow:** the mod-local copy won as intended;
   18/18 patches applied, 17 plugins active, the one shadowed duplicate was the
   expected disabled entry, 0 rejected/failed, and startup reached 24/24;
3. **final mod-local default-off:** 18/18 patches applied, 17/17 plugins active,
   0 disabled/rejected/failed, startup reached 24/24, and the fresh plugin log
   states `loaded disabled; no hooks installed`.

The global test copy was removed, the mod-local DLL remains the final hash, the
mod-local JSON was restored to the governed default-off hash, and no D2R process
was left running.

This proves loading, ownership, global/mod-local precedence, rollback, and
coexistence with the currently installable stack. It does not prove the A-H
gameplay behavior.

The first A-H solo attempt ran on 2026-08-09 with QtyTester, normal Attack,
100% Deadly Strike/Open Wounds/Crushing Blow and the exact configured legacy
gate token `stat 384 / layer 430`. Four attacks emitted:

```text
MeleeSplash suppressed configured legacy Event20 token=0x018001AE.
```

No attack emitted any `MeleeSplash capture`, `MeleeSplash target`, or
`MeleeSplash burst` record. Narrow rejection telemetry then proved the root
cause: normal Attack returns from `FillDamageValues` at `0x4300BB`, while the
plugin admitted only the distinct queued continuation `0x44B6A0`. Static 92777
analysis closed the unique 29-byte caller context at `0x43009E` and its flow
through Prepare/Allocate/Consume. The corrected plugin admits exactly those two
governed melee continuations and still rejects the six other Fill callers.

Preserved read-only evidence:

```text
analysis-cache/meleesplash-runtime-validation/20260809-smoke-ah/melee-splash-smoke-active.log
SHA-256 17714A62867748BAB67253384E670B7FA39B351D76CE409F671837A830AC5628

analysis-cache/meleesplash-runtime-validation/20260809-capture-fix/melee-splash.corrected-success-attempt.log
SHA-256 8C03D564519462CDCA333F650C7293CBA152383CB20BC24016C57CB4EA339C01

analysis-cache/meleesplash-runtime-validation/20260809-capture-fix/d2rloader.corrected-run.log
SHA-256 B9997B2F7AEEB62CE876D5D9ABC5BBA81745D95A03180DA949FB99F29BD38676

analysis-cache/meleesplash-runtime-validation/20260809-capture-fix/melee-splash.corrected-final-default-off.log
SHA-256 E32849BA8BB48A7D682174EF7D9F75CEAAF012B0C016D548BC1DE5A98D27A634

analysis-cache/meleesplash-runtime-validation/20260809-capture-fix/d2rloader.corrected-final-default-off.log
SHA-256 1D66C46272263EE1AB1D66D4CC68CFB19EE7D11C8539B8DD8BBF551D804C967A
```

The corrected successful hit captured normal Attack with base/final radius 5
and splash percent 100, then applied one burst to three unique secondary
monsters. Their independent outcomes were weapon-mastery Critical or Deadly,
with exactly one successful Crushing Blow and one successful Open Wounds call
per target. EventFuncs 19 and 20 were filtered at recursion depth 1; the burst
ended at recursion depth 0. The primary GUID appeared only as the primary and
was rejected by the area callback, so it was not hit twice. This passes A, E,
F, H and the active suppression half of G. B, C, D and G default-off visual
rollback remain explicitly not run.

The runtime config was restored byte-exact to default-off SHA-256
`32747A94E8744C39813AEBC8EA32BD284AA12F10A5E898DA7272D3721D06E5E6`.
QtyTester and all its sidecars were restored; `QtyTester.d2s` is again SHA-256
`323EE322E0F22FE9B239359A0B4FF265F5CD4AA671E1DAEE518472166F1DA5F1`.
No D2R process remains active. The corrected mod-local DLL is installed, while
the runtime JSON is restored to default-off and QtyTester is restored byte-exact.
The final corrected cold start logged `loaded disabled; no hooks installed`,
18/18 patches, 17/17 plugins, 24/24 startup, and zero disabled/rejected/failed.

A formal matrix with every PluginPack feature simultaneously enabled is not
possible in the current workspace because of pre-existing conflicts at
`0x589736`, `0x314110`, and a `plugin-misc` rel32 owner. Those sites are not
MeleeSplash-owned and the limitation predates this plugin. Compatibility is
therefore claimed only for the current qualified stack, not for a hypothetical
mutually conflicting all-feature state or unknown future binaries.

## Strict public ZIP gate

The governed archive path is:

```text
addons/MeleeSplash/MeleeSplash-0.1.0.zip
```

`scripts/verify/zip-policy.json` permits exactly two root entries, with no
directory prefix:

```text
MeleeSplash.dll
MeleeSplash.json
```

The archive was created only from the frozen `package/` allowlist. Its audited
entries are:

| Root entry | Uncompressed bytes | SHA-256 |
|---|---:|---|
| `MeleeSplash.dll` | 199168 | `DBA0C40C191B2568A6B39D21324A45F770C1CBF8AD747B099AA3BCBEDEF8856C` |
| `MeleeSplash.json` | 601 | `6AA40B37051189ADE2CA5D60FE133765EE1D426E0B6DA5E2059B619E77030C20` |

Both uncompressed streams are byte-equal to their staged source. The JSON was
parsed from inside the ZIP and remains `enabled=false`. There are no directory
entries or extra payloads. README, sources, examples, symbols, logs, BKVince
files, and reverse-engineering evidence remain outside the public ZIP.

Archive size is 89072 bytes and SHA-256 is:

```text
F137F1B708A4C51C8A88EA68B49BFB85619F380693E63899B508EB1C342E35A9
```

The repository-wide ZIP verifier reports no MeleeSplash policy failure. It
still fails on nine unrelated undeclared historical archives:

- `AdvancedItemTooltips-3.2.3.zip` and `BurnFireResistance.zip`;
- RemoteStash `0.2.29`, `0.2.30`, `0.2.31`, `0.3.0`, `0.3.1`, and `1.1.7`;
- `Transmogrify-1.3.0-weighted-test.zip`.

Those pre-existing repository failures do not affect this archive's successful
exact two-entry audit.

## Governed deliverable inventory

The MeleeSplash lot currently owns the following release-facing delta. Refresh
this inventory before the final checkpoint if another file is added or removed.

- public plugin source and tests under `addons/MeleeSplash/src/`;
- public configuration under `addons/MeleeSplash/package/` and the enabled
  generic example under `addons/MeleeSplash/examples/`;
- `README.md`, `OPTIONS.md`, `CHANGELOG.md`, `NATIVE-HOOKS.md`,
  `SMOKE-TEST.md`, `BKVINCE-INTEGRATION.md`, and this validation record;
- the separate BKVince configuration and staged plugin copy under
  `data-BKVince/d2rloader/`;
- BKVince `itemstatcost.txt`, `properties.txt`, and
  `item-modifiers.json` append-only reservations;
- `scripts/migrate-bkvince/apply-melee-splash-stats.js` and the MeleeSplash
  entry in `scripts/verify/zip-policy.json`;
- `Mission/melee-splash-3.2.md`, the targeted Mechanics evidence updates,
  `reverse-engineering/d2r-3.2.92777/known-rvas.json`, and `ROADMAP.html`.

The generated public ZIP is part of the governed deliverable. Build directories
and runtime logs stay under `analysis-cache/` and are not release payloads.

## Repository closure

No MeleeSplash build, PE, current-stack runtime, default-off rollback, public
package, whitespace, or cartography blocker remains. The targeted data checks,
cartography schema validation, CURRENT/ROADMAP alignment, JSON parsing and
`git diff --check` pass. The repository-wide verifier remains red only for 13
unassigned files from unrelated concurrent work; none belongs to MeleeSplash.
Commit and push remain separate actions requiring Vincent's explicit request.
