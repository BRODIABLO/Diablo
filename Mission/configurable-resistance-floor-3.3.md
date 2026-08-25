# Configurable Resistance Floor — D2R 3.2.92777 and 3.3.93847

## Decision and product contract

Vincent authorized implementation on 24 August 2026 with `GO` and explicitly
excluded any ROADMAP edit. The selected mechanism is a permanent autonomous DLL
authored exactly by `RuffnecKk` and distributed as an independently versioned
member of the RuffnecKk D2RLoader Suite.

- Public name: **Resistance Floor**.
- Plugin id: `resistance-floor`.
- DLL: `ResistanceFloor.dll`.
- Configuration: `ruffneckk-resistance-floor.toml`.
- Description: `Lets configured units fall below the vanilla resistance floor.`
- Scope: hybrid global or mod-local installation, without `ModScopedOnly`.
- Baseline: D2RLoader PluginSDK v3 commit
  `4933e2c42cb2592958cd0df3b6dc5003102252d1`.
- Configuration precedence: active-mod config, current plugin scope, then global
  config. Present invalid TOML is refused explicitly.

The first candidate targets all six resistance stats used by the normal native
damage resolver: Physical 36, Magic 37, Fire 39, Lightning 41, Cold 43 and
Poison 45. The player-facing TOML separates `players`, `companions` and
`monsters`, each with its own enable switch and `minimum_resistance`. Shipped
defaults enable players and companions at `-1000`; monsters remain disabled at
the vanilla `-100` minimum until explicitly enabled.

BKVCombat's separate Crushing Blow and Open Wounds resistance policies remain
unchanged by Vincent's accepted scope. No TSV, save data or eezstreet binary is
modified.

## Governed native evidence

The product targets are D2R 3.2.92777 and 3.3.93847 with one DLL. Their
governed native evidence reuses the verified common corpus under
`reverse-engineering/d2r-3.2.92777/`; the historical path records provenance
for the shared native image.

`SUNITDMG_ApplyResistancesAndAbsorb` at RVA `0x4523E0` receives the resistance
context in RCX, the current 0x40-byte damage record in RDX and `dontAbsorb` in
R8D. Its caller at `0x44EC5A` passes a stack context and iterates exactly twelve
records. Inside the resolver:

- `[RSI+0x10]` is the defender `Unit*`, corroborated by the native
  `STATES_CheckState` call at `0x452508`;
- `[R14+0x08]` is the current resistance-stat id;
- `0x4524C4` begins the first vanilla lower clamp with
  `B9 9C FF FF FF 3B D9 0F 4F CB EB 51`;
- `0x4524E7` begins the capped-path lower clamp with
  `B9 9C FF FF FF 3B D9 0F 4F CB 8B D8`;
- the shared ten-byte prefix occurs exactly twice, while each extended witness
  above is unique in the governed image;
- the physical upper cap at `0x4524D6` and the elemental/magic upper cap at
  `0x4524DE` are distinct sites owned by eezstreet `plugin-items` and are not
  modified by this plugin.

The first prototype of Burn Fire Resistance that owned the resolver entry at
`0x4523E0` was rejected after an A/B test proved it prevented the installed
`monsterdisplay.dll` from loading. Resistance Floor therefore never hooks that
entry. It owns only the two five-byte `MOV ECX,-100` instructions through near
relays and returns to `0x4524C9` / `0x4524EC` respectively. Burn Fire
Resistance calls the live resolver and consequently composes with this floor.

`D2GAME_GetMinionOwner` at `0x4A53C0` has governed ABI
`Unit* (Unit* monster)`. It validates UnitMonster, reads the owner identity and
resolves the live owner through `SUNIT_GetServerUnit`. Resistance Floor follows
that owner chain with a strict bound and cycle detection; reaching UnitPlayer
classifies the original monster as player-owned. Null owner classifies it as a
non-player-owned monster. Unknown unit types and invalid/cyclic chains fall
back to `-100`.

The Character Screen has a separate shared lower clamp for its four native
elemental/poison resistance values. The unique witness
`8D 4F 4B 83 F9 5F 7C 05 B9 5F 00 00 00 B8 9C FF FF FF` begins at
`0x14E728C`; the `-100` operand begins at `0x14E729A`. The plugin writes only
that four-byte operand when native display synchronization is enabled.

Physical and Magic have no native numeric Character Screen slots. They remain
covered by the gameplay floor, but Resistance Floor does not add a custom
readout for them or depend on another plugin for display. Vincent explicitly
removed that integration from the product contract on 24 August 2026.

## Ownership and coexistence audit

- No local Suite source, BKVince runtime configuration or governed mission
  references `0x4524C4`, `0x4524E7`, `0x14E7299` or `0x14E729A` as an owned
  patch/hook site before this implementation.
- The pinned eezstreet reference is
  `D2RL-Plugins@dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`, clean and MIT.
  It exposes only `items.physResistCap` and `items.elementalResistCap`; searching
  `resistance floor` returns no implementation.
- Monster Display retains ownership of its resolver-entry interception.
- Plugin-items retains ownership of the two upper-cap operands.
- Floating Damage remains an observer of resolved damage and requires no
  integration change.

## Implementation contract

1. Parse and validate the dedicated TOML before any patch is installed.
2. Refuse builds other than governed 92777/93847 identities.
3. Validate strict full witnesses for both floor sites, the Character Screen
   clamp and every native function called directly.
4. Allocate one executable relay page within rel32 reach of D2R.exe. Each relay
   jumps absolutely into a MASM mid-hook that preserves RAX, RDX, R8-R11 and
   XMM0-XMM5, passes defender/stat id to a `noexcept` C++ policy helper, restores
   the volatile state, leaves the selected floor in ECX and resumes immediately
   before the original compare.
5. Apply a configured floor only to stats 36, 37, 39, 41, 43 and 45. Every
   other record receives vanilla `-100`.
6. Keep file I/O, parsing and logging out of the per-damage path. Optional usage
   counters use bounded atomics only.
7. Update the four native Character Screen values with the player floor. Do not
   add custom Physical/Magic display code or a cross-plugin display dependency.
8. Keep hot reload and hot unload unsupported for the first release candidate.
   Supported rollback is a cold restart after removing the DLL/TOML.

## Validation gates

### Static and build

- strict signatures unique and exact;
- TOML accepts `-100..-1000` and rejects `-99`, `-1001`, wrong types, unknown
  keys, missing required sections and duplicate semantic definitions;
- policy tests cover the six admitted stat ids, all unit categories, disabled
  targets, invalid/cyclic ownership and vanilla fallback;
- relay-range and displacement checks fail closed;
- x64 Release builds with `/W4 /WX /permissive- /EHsc /utf-8`;
- three D2RLoader exports, manifest v2/API v3 resource, file metadata and exact
  author are verified;
- public ZIP contains exactly the DLL and dedicated TOML; README remains beside
  the ZIP and outside it.

### Runtime and gameplay

- full active Suite plus all five eezstreet DLLs, with no disabled plugin or
  PluginPack feature;
- global and mod-local scope, plus coexistence with Monster Display and
  plugin-items;
- player, hireling, summon, pet, revive, converted unit, ordinary monster,
  champion, unique, superunique and boss;
- Physical, Magic, Fire, Lightning, Cold and Poison at `-99`, `-100`, `-101`,
  `-250`, `-1000` and below-floor input;
- Burn, difficulty penalties, pierce, immunities and upper caps remain composed;
- Character Screen four native values; Physical/Magic remain gameplay-only;
- solo, host, joiner and PvP with identical configs; the host remains gameplay
  authoritative and clients require matching TOML for matching local display;
- dense-pack performance with the full active plugin stack;
- checked 11x worst-case multiplier and final int32/fixed-point overflow edges.

## Rollback

Remove `ResistanceFloor.dll` and
`ruffneckk-resistance-floor.toml`, then cold-start D2R. D2RLoader restores its
tracked five-byte relays and display operand; no save migration or data rollback
is required.

## Implementation and validation status — 2026-08-24

- The standalone 0.3.0 candidate is implemented under
  `addons/ResistanceFloor/` with its dedicated English TOML, source, policy
  tests, MASM mid-hooks, documentation and public candidate archive.
- Its configuration version 3 TOML uses player-facing sections and labels: `players`,
  `companions`, `monsters`, `minimum_resistance`, `character_screen` and
  `troubleshooting`.
- Release x64 compilation and reproducibility evidence are recorded in
  `addons/ResistanceFloor/VALIDATION.md`.
- The DLL is x64, ASLR/high-entropy-ASLR/NX compatible and exports exactly the
  three D2RLoader entry points. The ZIP contains only the DLL and TOML; the
  README stays beside it.
- The installed D2R 3.3.93847 version, Build Key, `.build.info` hash and
  `D2R.exe` hash match the governed baseline exactly. Source and BKVince
  mod-local deployment hashes match.
- An external tester loaded the final 0.3.0 candidate and confirmed its
  functional gameplay and native display behavior. The earlier 0.1.0 candidate
  proved mod/global duplicate arbitration and global-only scope. A fresh
  governed 0.3.0 cold start with the complete active stack remains part of the
  release-specific qualification matrix.
- The diagnostic candidate first exposed a D2RLoader contract mismatch:
  `PatchJmpRel32` requires its tracked safety-check size to equal the five-byte
  jump size. The final implementation keeps the independent twelve-byte
  preflight witness and supplies the exact five-byte `MOV` witness to the
  tracked write. Both relays and the Character Screen operand are confirmed
  operational in fresh plugin logs.
- External gameplay confirms that player and configured-monster resistances
  below `-100` affect actual damage and that the native display follows the new
  floor. Vincent accepted 0.3.0 for the next Suite release on 25 August 2026.
  Companion-specific, overflow and separate multiplayer gates remain open; see
  `addons/ResistanceFloor/VALIDATION.md` for the explicit matrix.
- ROADMAP was intentionally not edited, following Vincent's instruction.
