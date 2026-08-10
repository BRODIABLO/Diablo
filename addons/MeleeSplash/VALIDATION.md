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
| BKVince table/config checks | **PASS** | `npm run test:bkvince-melee-splash-stats` passes with the active no-gate host profile, reserved IDs, and complete legacy retirement. |
| Current-stack runtime coexistence | **PASS** | The earlier global/shadow/default-off matrix and the final active no-gate BKVince retirement cold start completed with no failed or rejected plugin. |
| Formal every-feature PluginPack matrix | **BOUNDED BY PRE-EXISTING CONFLICTS** | Existing non-Melee collisions prevent mutually conflicting PluginPack features from all being enabled at once; this does not invalidate MeleeSplash on the qualified current stack. |
| Solo gameplay smoke A-H | **PASS WITH G SUPERSEDED** | A/E/F/H and legacy suppression passed after the Fill-caller fix. B passed by excluding skill 0; C passed with exact stats 391=20/40; D passed with exact stat 392=50. The old default-off path asserted, so Vincent retired it and superseded that half of G. |
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

Two consecutive builds of the BKVCombat-provider bridge source both passed
CTest and produced:

```text
77D3A7DC6C77B319A8E80C49F76DA84707F93DDFEBBC9E18F5E5D83CCE25F2DF
200192 bytes
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
| `src/plugin.cpp` | 2026-08-10 16:19:58 | `9C8968AEF2F8CBC209D90E3BCA6ED6FC53F2C82D3C14FA50C6DE1043E42465C6` | Governed direct-melee capture plus optional BKVCombat API v1 resolver |
| `package/MeleeSplash.dll` | 2026-08-10 17:10:53 | `77D3A7DC6C77B319A8E80C49F76DA84707F93DDFEBBC9E18F5E5D83CCE25F2DF` | Reproducible bridge build; byte-equal to build output and BKVince plugin copy |
| BKVince plugin copy | 2026-08-10 17:10:53 | `77D3A7DC6C77B319A8E80C49F76DA84707F93DDFEBBC9E18F5E5D83CCE25F2DF` | Byte-equal to corrected build and package |
| `package/MeleeSplash.json` | 2026-08-09 05:37:00 | `6AA40B37051189ADE2CA5D60FE133765EE1D426E0B6DA5E2059B619E77030C20` | Generic, default-off |
| Enabled generic example | 2026-08-09 05:37:00 | `CAE51A8A98F1BBA1278263DB7D3137D4DBFFC7DD8073670C65427F090A06B1EA` | Test/example only; excluded from ZIP |
| BKVince configuration | 2026-08-09 16:34:48 | `0A7B1878C6D20CE3362F9B95055B7DBF9E56EDEC81C24001E2B88A400017802D` | Separate, active no-gate host profile; excluded from ZIP |
| `MeleeSplash-0.1.0.zip` | 2026-08-10 17:15:31 | `3B159EC80791A82AF7ED0A9BD40CDFAEEAED5BC656128CB7C5BEF1DD1D46CE9C` | Strict two-file public archive, 89547 bytes |

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
IDs 65028/65029, TSV byte policy, and the separate active BKVince
configuration. It also verifies that stat 384/property 302, skills 430/432,
missile 743 and state 242 are inert ID-stable tombstones; every Summon Splash
reference is gone; Titan's Echo is non-spawnable and unreferenced. No row is
deleted or renumbered.
The governed BKVince DLL copy is byte-equal to the final package and qualified
mod-local runtime DLL.

## Current-stack runtime qualification

Runtime evidence is preserved outside the release payload under:

```text
analysis-cache/meleesplash-runtime-validation/20260809-095701652/
```

The original cold-start matrix used the frozen `D9A49607...21B0` artifact. The
corrected gameplay build was `DBA0C40C...8856C`; the optional BKVCombat bridge
build is `77D3A7DC...5F2DF`. The fresh BKVCombat matrix loaded both exact module
orders at 19/19 plugins, 15/15 patchsets and frontend 24/24. The bridge lookup
is lazy, so actual provider negotiation remains `NOT RUN` until a gameplay hit.
Its historical default-off
baseline used SHA-256 `32747A94...6E5E6`. Three D2R 3.2.92777 cold starts were
recorded for that packaging matrix:

1. **global enabled:** all twelve entry hooks and two call relays installed;
   18/18 BKVince patches applied, 17/17 plugins active, 0 disabled/rejected/
   failed, and frontend startup reached 24/24;
2. **mod-local enabled with global shadow:** the mod-local copy won as intended;
   18/18 patches applied, 17 plugins active, the one shadowed duplicate was the
   expected disabled entry, 0 rejected/failed, and startup reached 24/24;
3. **final mod-local default-off:** 18/18 patches applied, 17/17 plugins active,
   0 disabled/rejected/failed, startup reached 24/24, and the fresh plugin log
   states `loaded disabled; no hooks installed`.

The global test copy was removed and the mod-local DLL remains the final hash.
That default-off state was later superseded only for BKVince by the governed
active no-gate profile; the public package itself remains default-off.

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
F, H and the active suppression half of G.

The complementary smoke then passed B with `excludedSkillIds:[0]`, C with the
exact reserved radius stat at 20 and 40 (radius bonus 1/2, final radius 6/7),
and D with the exact reserved damage stat at 50 (`splashPercent=150`). Evidence
is preserved under
`analysis-cache/meleesplash-runtime-validation/20260809-smoke-bcdg/`.
The attempted default-off visual rollback reached the obsolete skill-item
effect path and asserted `ptSkill->nItemEffect != 0` before the old missile
could be qualified. Vincent explicitly superseded that rollback requirement:
the legacy graph is now retired in data, and disabling/removing the plugin
means no splash.

## BKVince legacy-retirement qualification

After Vincent superseded the legacy rollback, the seven governed Excel tables
and active no-gate JSON were synchronized byte-exact to the mod-local runtime.
The final configuration is 602 bytes, SHA-256
`0A7B1878C6D20CE3362F9B95055B7DBF9E56EDEC81C24001E2B88A400017802D`.

A diagnostic hit by the unmodified no-gate QtyTester path logged
`gateSeen=false`, `radiusFinal=5`, `splashPercent=100`, a valid capture and a
completed burst. It emitted no configured-legacy suppression record and no
EventFunc20 token. No secondary monster happened to be in radius for that hit;
the earlier A/E/F/H witnesses retain the multi-target application proof.

The final silent cold start on the retired data reached 18/18 patches,
17/17 plugins and 24/24 startup with zero disabled, rejected or failed entries.
The session was stopped and no D2R process remains. Preserved evidence:

```text
analysis-cache/meleesplash-runtime-validation/20260809-legacy-retirement/melee-splash-hit-no-gate.log
SHA-256 F36551F4960A17C88ED101EB4B99A4B8EEA5509AB398DF8A7F56F8CD29B98992

analysis-cache/meleesplash-runtime-validation/20260809-legacy-retirement/d2rloader-final-active-silent.log
SHA-256 D5F9C307A6DA46E912D68E7C1555867B992DFEABD40DF82180D3784BACDBDA20

analysis-cache/meleesplash-runtime-validation/20260809-legacy-retirement/melee-splash-final-active-silent.log
SHA-256 462101B04E98CEA44FF98DA1C1CEB752B846471C3E4A8831E1A834B97020FCFE
```

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

Archive size is 89547 bytes and SHA-256 is:

```text
3B159EC80791A82AF7ED0A9BD40CDFAEEAED5BC656128CB7C5BEF1DD1D46CE9C
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
- BKVince `itemstatcost.txt`, `properties.txt`, `skills.txt`, `missiles.txt`,
  `monstats.txt`, `uniqueitems.txt`, `treasureclassex.txt`, and
  `item-modifiers.json` reservations/legacy retirement;
- `scripts/migrate-bkvince/apply-melee-splash-stats.js` and the MeleeSplash
  entry in `scripts/verify/zip-policy.json`;
- `Mission/melee-splash-3.2.md`, the targeted Mechanics evidence updates,
  `reverse-engineering/d2r-3.2.92777/known-rvas.json`, and `ROADMAP.html`.

The generated public ZIP is part of the governed deliverable. Build directories
and runtime logs stay under `analysis-cache/` and are not release payloads.

## Repository closure

No MeleeSplash build, PE, current-stack runtime, active-host integration,
public package, whitespace, or cartography blocker remains. The old default-off
rollback is intentionally superseded, not claimed as successful. The targeted data checks,
cartography schema validation, CURRENT/ROADMAP alignment, JSON parsing and
`git diff --check` pass. The repository-wide verifier remains red only for 13
unassigned files from unrelated concurrent work; none belongs to MeleeSplash.
Commit and push remain separate actions requiring Vincent's explicit request.
