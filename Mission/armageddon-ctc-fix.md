# RuffnecKk Armageddon CtC Fix

## Mission

Deliver a public, autonomous D2RLoader plugin that lets Armageddon and Hurricane
start correctly from native chance-to-cast item effects.

Vincent approved implementation on **25 August 2026** and decided on
**27 August 2026** that this plugin remains separate from Cast Triggers. It is
an independent member of the RuffnecKk D2RLoader Suite and is not part of
BKVince.

No D2R or D2RLoader test may be opened without Vincent's explicit approval.

## Product contract

- DLL: `d2rl-ruffneckk-armageddon-ctc-fix.dll`
- Config: `ruffneckk-armageddon-ctc-fix.toml`
- Version: `0.1.1`
- Author: `RuffnecKk`
- Runtime tested: D2R `3.3.93847`, mod-local and offline
- Native-equivalent build: D2R `3.2.92777`, covered by the governed
  byte-identical surfaces without a duplicate runtime matrix
- Runtime policy: no build-name/version allowlist; the complete native
  fingerprint accepts or refuses loading before the first hook
- Scope contract: the same DLL supports global or mod-local installation and
  refuses duplicate installation
- Data dependency: none; no replacement `skills.txt`
- Saves: no proprietary payload and no persistent synthetic skill node
- Rollback: remove the DLL and TOML

## Reproduced failure

The original native item-effect path exposed two gates:

1. With Armageddon's blank `ItemEffect`, the helper asserted on
   `ptSkill->nItemEffect != 0` in `SkillItemEffect.cpp:136`.
2. Bypassing that field gate alone still did not start Armageddon because the
   shared callback requires a used skill node that item-triggered units may not
   own.

Chilling Armor triggered from the same disposable Fallen event, proving that
the CtC event and target were valid.

## Selected architecture

The plugin installs three strict-signature hooks:

1. The item-effect helper recognizes only enabled skill IDs 249 (Armageddon)
   and 250 (Hurricane). During the synchronous call, it temporarily supplies
   the missing item-effect flag and restores it on exit.
2. The shared start callback stays native unless the exact game, unit and skill
   match that scoped call. When no real matching skill exists, a stack-local
   surrogate is supplied only for the original callback.
3. A successful synthetic Armageddon start retains only its 32-bit periodic
   seed. Its active callback receives a temporary stack-local skill node, and
   the seed is erased when the native state expires. Hurricane needs no
   persistent seed bridge.

Native state, duration, events, targeting, missiles and damage calculations
remain authoritative. No synthetic skill node survives a hooked call or is
serialized.

## Native proof

The governed common corpus identifies the required D2R 3.2.92777 and
3.3.93847 surfaces with high confidence:

- `0x589930`: item-effect skill helper;
- `0x575DE0`: shared Armageddon/Hurricane start callback;
- `0x574E90`: Armageddon active callback;
- `0x575600`: Hurricane active callback;
- `0x097790`: context-aware SkillsTxt lookup;
- `0x33DD40`: highest-level skill lookup;
- `0x34B6E0`: unit skill-list lookup;
- `0x3351B0`: state predicate used for expired-seed cleanup.

The ABI and semantics are corroborated by D2MOO at pinned commit
`19019806df7f3e877fa105b05395d1e3597e2316`. D2MOO is a semantic reference
only; its 32-bit addresses and layouts are not transplanted.

## Coexistence and failure policy

Cast Triggers owns `0x43ACB0`, `0x5896E0` and `0x589820`; this plugin touches
none of those entries. A native-fingerprint mismatch, invalid present config or
duplicate installation refuses operation with a diagnostic instead of
guessing.

## Validation completed

- Strict TOML, policy, layout and fingerprint tests pass.
- Release x64 builds reproducibly against PluginSDK commit
  `4933e2c42cb2592958cd0df3b6dc5003102252d1`.
- The exact 0.1.1 DLL tested and packaged has SHA-256
  `FDA4D8905D60A5FDCDE12B734D4D272EE2FBDA7BF58E24C516A2A88CD5295F77`.
- D2R 3.3.93847 mod-local cold start completed with the installed stack.
- Armageddon CtC produced visible meteors and damage with blank `ItemEffect`
  and no assertion.
- A ten-second fixture proved native duration and caster tracking.
- Natural expiration produced `expired seed cleanups=1` and
  `retained seeds=0`.
- Hurricane CtC was visible on `QtyTester` with no assertion and no retained
  seed.
- Temporary `cubemain.txt` and `skills.txt` fixtures were restored exactly.

## Explicit release scope

The public release documents, without hiding them, the cases not run: global
runtime installation, duplicate-scope refusal, native-cast regression,
retrigger behavior, multiplayer authority and a complete all-features Suite
matrix. The five eezstreet plugins were loaded during the installed-stack test,
but this is not claimed as proof that every feature was active.

## Packaging

`ArmageddonCtCFix-0.1.1.zip` contains only the DLL and TOML. The public README
and validation record stay beside the ZIP for Vincent's review. The package
SHA-256 is
`3B4F00FEBFB116E55E8BB3DF4A3CC0679AF1ED1A238FF535565FDFC0EE8B486F`.

## Status — 27 August 2026

Implementation, governed native proof, approved 3.3 functional tests,
retained-seed cleanup, Hurricane validation and the public package are complete.
Version 0.1.1 is finalized as a standalone release candidate; no merge into
Cast Triggers is planned.
