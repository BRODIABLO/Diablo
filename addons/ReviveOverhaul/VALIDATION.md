# Revive Overhaul 2.1.1 validation

Validation date: 2026-08-31
Runtime matrix: Diablo II: Resurrected build 93847
Native-equivalence coverage: build 92777 while all used surfaces remain byte-identical
Plugin SHA-256: `22137F62C0293B3349F27DC480DADC06D365C880FDD1836E0BA031CCE862AEDC`
Release configuration SHA-256: `1A330B977F5EE2E0972E0C7D8E5462DE5620B9D7D845661CD07659EFF7FCE6E5`
Release ZIP SHA-256: `NOT BUILT — gameplay qualification is pending`

## Automated and static gates

- Two independent Release x64 builds: PASS, zero compiler warnings and
  byte-identical 190,976-byte DLLs.
- Policy test suite: PASS, 1/1.
- PluginSDK resource manifest: PASS, API 3.
- PluginSDK pin: `4933e2c42cb2592958cd0df3b6dc5003102252d1`.
- Build-name/version allowlist: REMOVED. Runtime identifiers are diagnostic
  only; all used native entries, callsites, return sites and ABI witnesses are
  fingerprinted fail-closed.
- PE export contract: PASS (`D2RLoaderGetPluginInfo`,
  `D2RLoaderLoadPlugin`, `D2RLoaderUnloadPlugin`).
- Governed RVA JSON parse: PASS.
- Governed common native corpus and index verification: PASS.
- Pinned eezstreet PluginPack
  `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`: clean, verified and no Revive
  surface found.
- Hook ownership audit: PASS. Cast Triggers uniquely owns `0x48FE20`; Revive
  Overhaul 2.1.1 contains no RVA, expected-byte witness, function pointer or
  call for `GetTargetUnit`. Aura capture uses the exact target delivered by
  the owned Revive validator hook inside a thread-local SrvDo58 scope.
- `Skills.txt` Revive row in both repository BKVince and the installed
  laboratory: PASS (`21 / 58 / 24 / blank / 3`, `TargetCorpse 1`,
  `PetType revive`).

## Prepared BKVince 93847 testing harness

The isolated profile is:

```text
C:\Games\Diablo II Resurrected\mods\BKVince\testing\revive-overhaul
```

Its static guard currently fingerprints:

- D2R 3.3.93847 image:
  `E1F5436E3D9687F644EF16938B1B183D1FDEF434F18CF66D852CF68F48CC8936`;
- D2RLoader 1.2.0-beta:
  `651FA9EB33083088349224B1624819F63ED79596F808950CF6468B5D82F7132E`;
- BKVince plugin inventory:
  `3D7C250FF7CA801E3CDAD29C2B4448C004DC7412457CA4859FDE80A34969F3F7`;
- laboratory patch inventory:
  `264958C3C5D6CDC6E17460EB3D0B5182E9A1F4B6408922588DC7546CB07E9CBA`;
- global plugin inventory:
  `27EB30B070709463D6A3826AA6EEBDD7E9FC15571B394F978D666E8E950F3019`;
- global patch inventory:
  `81D1479662117E800E17F2DCDFCE2596B80477D950F5C964EA223641207A2940`.

The harness uses the existing BKVince mod and save path. It has diagnostics
enabled, a protected launcher, an evidence collector and a two-phase test
matrix using D2RLoader's built-in Monster Spawner. Its active test
configuration SHA-256 is
`AEEAC0E547862FAE838A43C52C4E368E5C002F04E33F8A04A4E2AD70E420E6BF`.
The final static preflight reports `readyForAuthorizedTest: true` with every
guard passing and zero D2R/D2RLoader process active.

## Runtime matrix

No 2.1.1 runtime result is claimed. The 2.1.0 full-stack evidence is retained
as the reproduced `GetTargetUnit 0x48FE20` collision that this version removes.
No BKVince cold start or gameplay session was launched during this correction.

| Installation | Result | Evidence |
| --- | --- | --- |
| 93847 BKVince mod-local plus complete global stack | NOT RUN | Awaiting Vincent's explicit `GO`. |
| 93847 global-only plugin scope | NOT RUN | Must use the complete active stack. |
| 92777 native-equivalent coverage | NOT RUN | Inherited only after the 93847 matrix passes and every used surface remains governed byte-identical. |

## Functional gameplay matrix

The first authorized session must validate normal, Champion, Unique,
Aura Enchanted Unique, SuperUnique, close-range owner scatter, long-range
catch-up, zone transitions and diagnostic counters. Act bosses are deferred
until that main matrix passes and require a separate decision.

## Scope boundaries

- No `SrvDoFunc 49` patch.
- No TXT or TSV edit or port.
- No ROADMAP edit.
