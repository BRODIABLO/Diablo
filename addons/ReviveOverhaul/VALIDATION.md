# Revive Overhaul validation

## REVIVE-TACTICAL-DIRECTOR-2.3.0 candidate

Validation date: 2026-09-03
Runtime target: Diablo II: Resurrected build 93847
Candidate DLL SHA-256: `B4C6D3BEDBB798D2553935F25D528A80F03CBAD8FE87255BDA058E9E4A6931CC`

- [x] Keep Revive Overhaul as the unique `AiFunction03` hook owner and expose
  one optional private Scripted AI ABI V3 with explicit size, version, magic,
  capability and result checks.
- [x] Copy the combat snapshot across the ABI. No game pointer, unit pointer,
  GUID or persistent native identity is exposed to Lua.
- [x] Classify Revives from compiled MonStats data as melee, ranged physical
  or caster without modifying `Skills.txt`, `MonStats.txt` or any other table.
- [x] Keep the target blackboard native, session-scoped and bounded; re-resolve
  a locked target by type and GUID, require the same address, reject death and
  owner-radius escape, and clear state on peaceful ticks and lifecycle changes.
- [x] Let one Lua tree select at most one validated native target action per
  handled tick: melee pressure, ranged attack/retreat, or caster spell rotation
  and kiting. Delegate every refusal, fallback and incompatibility to the
  original Revive AI exactly once.
- [x] Preserve Revive Overhaul's peaceful follow, obstacle recovery, catch-up
  and transition behavior outside the tactical combat path.
- [x] Expand Scripted AI's fail-closed native fingerprint for unit identity,
  death, server-unit resolution, distance and the compiled MonStats flags
  witness used by automatic profile selection.
- [x] Add static behavior tests for all three profiles, target-action fallback,
  ABI compatibility and automatic loadout classification.
- [x] Make the Release test harness execute every assertion and fix the latent
  TOML coercion it exposed: boolean settings now reject integer values such as
  `1` instead of silently accepting them.
- [x] Prove two independent Release x64 builds with MSVC `19.44.35228`, Windows
  SDK `10.0.26100`, `/W4 /WX` and `/Brepro`; both meaningful CTest suites pass
  `1/1`, and both `200192`-byte DLLs are byte-identical with the recorded hash.
- [x] Inspect PE32+ AMD64, exactly the three required D2RLoader exports and
  embedded RuffnecKk version `2.3.0` metadata.
- [ ] Deploy only after a separate runtime-test GO and qualify the complete
  active BKVince stack using A0-A18 in `testing/TEST-MATRIX.md`.

No deployment, live runtime result, ZIP, release, commit or push is claimed by
this candidate. It contains no `SrvDoFunc 49`, TXT/TSV or ROADMAP change.

## Historical 2.2.1 validation

Validation date: 2026-09-02
Runtime matrix: Diablo II: Resurrected build 93847
Native-equivalence coverage: build 92777 while all used surfaces remain byte-identical
Candidate plugin SHA-256: `CEBFA6EAA850038866873A019DA383FDFB84354DB8644297460F259139F9CA8F`
Release configuration SHA-256: `EF4E946EB907A99F25655DDFCF3D2464A3AB5CBB0AE1E4E370C617574CF7D9E7`
Release ZIP SHA-256: `NOT BUILT — gameplay qualification is pending`

## Automated and static gates

- Two independent Release x64 builds: PASS, zero compiler warnings and
  byte-identical 198,144-byte DLLs.
- Policy test suite: PASS, 1/1.
- PluginSDK resource manifest: PASS, API 3.
- PluginSDK pin: `4933e2c42cb2592958cd0df3b6dc5003102252d1`.
- Build-name/version allowlist: REMOVED. Runtime identifiers are diagnostic
  only; all used native entries, callsites, return sites and ABI witnesses are
  fingerprinted fail-closed.
- PE export contract: PASS (`D2RLoaderGetPluginInfo`,
  `D2RLoaderLoadPlugin`, `D2RLoaderUnloadPlugin`).
- Optional Scripted AI ABI consumer: PASS. ABI V2 checks size, version, magic
  and `CapabilityRequestNativeFollow`; no DLL is loaded by the consumer. Each
  provider call holds a temporary module reference. `RequestNativeFollow` and
  every absence, incompatibility, refusal, error or unknown result execute the
  original Revive callback exactly once.
- Peace/combat split: PASS. The governed tick layout uses target at `+0x10` and
  `inCombat` at `+0x24`; `8 / 4 / 80` transforms are armed only when there is
  no target and no combat. Targeted and combat ticks run native AI untouched.
- Governed RVA JSON parse: PASS.
- Governed common native corpus and index verification: PASS.
- Pinned eezstreet PluginPack
  `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`: clean, verified and no Revive
  surface found.
- Hook ownership audit: PASS. Cast Triggers uniquely owns `0x48FE20`; Revive
  Overhaul 2.1.2 contains no RVA, expected-byte witness, function pointer or
  call for `GetTargetUnit`. Aura capture uses the exact target delivered by
  the owned Revive validator hook inside a thread-local SrvDo58 scope.
- Source `Skills.txt` Revive row: PASS (`21 / 58 / 24 / blank / 3`,
  `TargetCorpse 1`, `PetType revive`). Version 2.2.1 verifies that tuple in all
  three compiled banks and applies `blank / 58 / 39 / 36 / 2` in memory; no
  TXT file is changed.
- New native witnesses: PASS. The canonical corpus contains exactly one match
  for the 20-byte AI-special-state dispatch read at `0x4A2BC8` and exactly one
  match for the 36-byte `D2GAME_GetMinionOwner` entry at `0x4A53C0`.

## Prepared BKVince 93847 testing harness

The isolated profile is:

```text
C:\Games\Diablo II Resurrected\mods\BKVince\testing\revive-overhaul
```

After candidate deployment, its static guard expects:

- D2R 3.3.93847 image:
  `E1F5436E3D9687F644EF16938B1B183D1FDEF434F18CF66D852CF68F48CC8936`;
- D2RLoader 1.2.0-beta:
  `651FA9EB33083088349224B1624819F63ED79596F808950CF6468B5D82F7132E`;
- BKVince plugin inventory:
  `E49445D88A46ECC0E4038CCF014ED7BBDDD84E9FA490D8169EC41D07790106E4`;
- laboratory patch inventory:
  `264958C3C5D6CDC6E17460EB3D0B5182E9A1F4B6408922588DC7546CB07E9CBA`;
- global plugin inventory:
  `27EB30B070709463D6A3826AA6EEBDD7E9FC15571B394F978D666E8E950F3019`;
- global patch inventory:
  `81D1479662117E800E17F2DCDFCE2596B80477D950F5C964EA223641207A2940`.

The harness uses the existing BKVince mod and save path. It has diagnostics
enabled, a protected launcher, an evidence collector and a two-phase test
matrix using D2RLoader's built-in Monster Spawner. Its candidate test
configuration SHA-256 is
`CBC057F3E663CA51CE8DB04817A505D67517340673E127164B93B2E136D16401`.
No `readyForAuthorizedTest` result is claimed until a separate deployment GO
copies both candidate DLLs and configurations into BKVince.

## Runtime matrix

The 2.1.1 full-stack cold start passed, but live gameplay failed high-rank
admission: Champion, Unique and SuperUnique corpses could be identified under
the cursor while right-click submitted no Revive action. Version 2.1.2 replaces
the shared AI eligibility entry hook with the owned client Revive callsite
redirect. Its live counters proved the gate accepted Champion and Unique while
the server remained untouched. Version 2.1.3 then proved that the complete
selector returned success while `server admissions` remained zero. Version
2.1.4 replaced the disproven predicate-only assumption with the automatic
compiled callback bridge. Version 2.2.0 then proved high-rank revival and
native aura preservation, but its direct Scripted AI owner chase reproduced
two application hangs during town/combat transitions and made Revives too
owner-leashed during combat. Version 2.2.1 removes that direct movement path:
Scripted AI is policy-only, Revive Overhaul calls native `AiFunction03` once,
and peaceful-only tuning preserves native obstacle recovery and teleport. No
2.2.1 runtime result is claimed before redeployment and retest.

| Installation | Result | Evidence |
| --- | --- | --- |
| 93847 BKVince mod-local plus complete global stack | COLD START PASS | 2026-09-02: 38 plugins, 17 patches, startup 24/24, Revive Overhaul 2.1.4 active, automatic callback route accepted after revision 1, fingerprint `efc49b51…a44533`. Gameplay remains pending. |
| 93847 global-only plugin scope | NOT RUN | Must use the complete active stack. |
| 92777 native-equivalent coverage | PENDING | Inherited only after the 93847 gameplay matrix passes and every used surface remains governed byte-identical. |

The next authorized matrix must also prove Scripted AI absent, disabled and
enabled; both relevant plugin load orders; peaceful far-owner handling;
untouched combat behavior; obstacle catch-up; lifecycle reload; and repeated
town/combat transitions with ProcDump hang capture armed. The current GO did
not authorize deployment or a live launch.

## Functional gameplay matrix

The first authorized session must validate normal, Champion, Unique,
Aura Enchanted Unique, SuperUnique, close-range owner scatter, long-range
catch-up, zone transitions and diagnostic counters. Act bosses are deferred
until that main matrix passes and require a separate decision.

## Scope boundaries

- No `SrvDoFunc 49` patch.
- No TXT or TSV edit or port.
- No ROADMAP edit.
