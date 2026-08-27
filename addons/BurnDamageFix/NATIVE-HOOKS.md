# Burn Damage Fix native hook contract

Runtime qualification: D2R `3.3.93847`. D2R `3.2.92777` is covered by the
verified byte-identical native surfaces in the common governed corpus, whose
historical source image is 3.2.92777. These names describe evidence coverage;
they never form a build or version allowlist.

| RVA | Ownership | Expected witness | Contract |
|---|---|---|---|
| `0x44CB32` | Exclusive, 6-byte rel32 mid-hook | `81 C3 3C 01 00 00 41 0F 48 DE` | Replaces the bogus `add ebx,316`, consumes the already advanced RNG value in `R8D`, adds a native max-exclusive `burningmin..burningmax` roll with Fire Mastery, recreates the sign flag and resumes at `0x44CB38`. |
| `0x451380` | Exclusive inline entry hook | `45 85 C9 0F 8E E6 01 00 00 48 89 6C 24 20 56 41 56 41 57 48 83 EC 40` | Resolves positive Burn through Fire Resistance once, then calls the original Burn application trampoline. |
| `0x4523E0` | Borrowed live resolver; never patched | Vanilla 32-byte entry, or one DiagnosticsService-tracked inline owner `monsterdisplay`; strict internal witnesses listed below | Calls the live resolver with third argument `0`. The synthetic Fire record uses resistance `39`, max resistance `40`, pierce `333`, immunity pierce `189`, absorb sentinels `-1/-1`, reduction index `2`, and a zero MDR slot. |
| `0x300830` | Borrowed difficulty-record resolver; never patched | Unique 32-byte entry beginning `40 53 56 57 ...` | Builds the native difficulty context from the fingerprinted `Game+0x106` data-set byte and `Game+0x104` difficulty byte. |
| `0x300A90` | Borrowed data-table context resolver; never patched | Unique 28-byte entry beginning `48 83 EC 28 0F B6 C1 ...` | Resolves one of the four compiled data-table contexts selected by the defender. |
| `0x34A0E0` | Borrowed unit data-context resolver; never patched | Strict 47-byte body beginning `48 83 EC 28 48 85 C9 ...` | Reads the defender's data context before the process-local state-row update. No unit pointer is retained. |
| `0x307EB3` | Read-only StatesTxt field-layout witness | Unique 33-byte descriptor sequence | Proves that the first compiled state overlay is the 16-bit word at record offset `+0x02`. |
| `0x3083D7` | Read-only StatesTxt stride witness | Unique 22-byte instruction-aligned prefix containing record size `0x44`; stops before the compiler `CALL` that D2RLoader may redirect | Proves the compiled state-record stride used to locate state 115 without claiming ownership of D2RLoader's compiler integration. |
| `0x30843C` | Read-only StatesTxt vector witness | Unique 26-byte sequence containing `DataTables+0x290` | Proves the compiled state-vector base field. |
| `0x3354E0` | Read-only state count/context witness | Unique 32-byte sequence calling both context resolvers and reading `DataTables+0x298` | Proves the active state count and the same context selection used by `STATES_ToggleState`. |
| `0x394640` | Read-only empty-overlay sentinel witness | Unique 47-byte `0x44`-stride initializer containing `C7 00 00 00 FF FF` | Proves that the empty first-overlay value is `0xFFFF`; BurnDamageFix never guesses a zero sentinel. |
| `0x2F5020` | Borrowed unit-stat getter; never patched | Unique 32-byte entry beginning `48 89 5C 24 10 ...` | Reads generic Burn stats and observes `unit_dooverlay` stat 178 before a direct-overlay write. |
| `0x34B9D0` | Borrowed unit-type getter; never patched | Unique 28-byte entry beginning `48 83 EC 28 ...` | Guards player/monster dispatchers and classifies units in the synthetic damage context. |
| `0x3AF240` | Borrowed hireling-type resolver; never patched | Unique 24-byte entry beginning `40 56 48 83 EC 20 ...` | Keeps nonzero hireling types out of the native monster-attacker/defender flags. |
| `0x48FF00` | Borrowed unit-to-game resolver; never patched | Unique 32-byte entry beginning `40 53 48 83 EC 20 ...` | Obtains the authoritative game pointer from attacker, or defender when attacker is absent. |
| `0x42CE30` | Exclusive player-event dispatcher hook when overlay replay is enabled | Unique 32-byte entry beginning `48 89 5C 24 08 ...` | Before event type 3 reaches the original six-argument dispatcher, verifies unit type 0 and conditionally replays `fire_hit` on a live unit whose state 115 remains active. All six arguments are forwarded unchanged exactly once. |
| `0x447420` | Exclusive monster-event dispatcher hook when overlay replay is enabled | Unique 39-byte entry beginning `48 89 5C 24 10 ...` | Same contract with an explicit unit-type-1 guard. This seam is upstream of the `plugin-misc` hook at `0x448C00`. |
| `0x3351B0` | Borrowed state predicate; never patched | `48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 8B DA 48 8B F1 E8 07 68 01 00 85 C0 74 0E 83 E8 01` | Checks state `burning` 115 for diagnostics and for immediate/periodic overlay replay. It never toggles the state. |
| `0x349020` | Borrowed overlay setter; never patched | Unique 24-byte entry beginning `48 89 5C 24 18 ...` | Applies `fire_hit` overlay 81 on layer 0. Its stat-list flag `0x80` is not a stat ID and is not used as an activity predicate. |
| `0x34916C` | Read-only overlay-stat witness | Unique 30-byte witness containing `41 B8 B2 00 00 00` | Proves that `UNITS_SetOverlay` writes the overlay ID to `unit_dooverlay` stat `178` (`0xB2`). The stat stores the last direct write rather than reliable animation lifetime. |
| `0x34C2C0` | Borrowed death predicate; never patched | Unique 32-byte entry beginning `48 83 EC 28 48 85 C9 ...` | Prevents a replay on a dead unit. |
| `0x42E615` | Read-only layout witness | Unique 17-byte witness beginning `44 8B 89 70 01 00 00 ...` | Proves that the current server frame is `Game+0x170`. |
| `0x44DF40` | Read-only layout witness | Unique 38-byte witness beginning `48 89 4C 24 30 0F B6 91 ...` | Proves difficulty at `Game+0x104`, data-set context at `Game+0x106`, and their call order into the difficulty-record resolver. |

The resolver entry is accepted only when it is exact and unowned, or when
DiagnosticsService reports exactly one tracked inline owner with plugin id
`monsterdisplay`. Without DiagnosticsService, the exact vanilla entry is
mandatory. In every case the strict internal witnesses at `0x4523F2`,
`0x452412`, `0x4524A1`, `0x44F8CF`, `0x44F6F5`, `0x450685`, `0x45076D`,
`0x45251F`, `0x452614` and `0x452658` must match.

The production relay uses a private executable page allocated within rel32
reach. Its absolute jump targets the plugin's MASM relay; the loader-managed
patch at `0x44CB32` remains the only D2R executable write for this feature.

The native application path calls `STATES_ToggleState 0x3354C0` with state 115
after validating positive damage and duration. The governed 3.2, 3.3 and
BKVince tables all map state 115 to overlay `burning` 224, whose asset is
`Expansion\\On_Fire`. With `overlay.suppress_native_burning=true`, 2.2 resolves
the active compiled row immediately before the original Burn application and
performs only an aligned atomic compare/exchange from `224` to `0xFFFF`. An
already-empty row is accepted, while any other custom overlay id is preserved
and reported. No `states.txt` is shipped or rewritten. On plugin unload, the
same cell is restored to `224` only when this DLL owns that exact record and it
still contains `0xFFFF`; a third-party replacement is never overwritten.

The independent `fire_hit` replay remains unchanged. It runs immediately after
the original Burn application confirms state 115, then before event type 3
reaches either dispatcher whenever `gameFrame % repeat_frames == 0`, the state
is still active and the unit is alive. The last emitted animation may finish
naturally after the state disappears. The default `repeat_frames=10` is a
0.4-second candidate cadence for the table's eight-frame,
16-frames-per-second `fire_hit`; client stacking/restart behavior and density
remain runtime gates.

## Coexistence

- Monster Display may own `0x4523E0`; the explicit tracked-owner contract keeps
  its live hook in the chain while rejecting unknown owners.
- Bind And Summon has no executable-code ownership or exact signature match for
  either dispatcher. Its apparent address coincidences were `.pdata` entries.
- The 2.2 change adds no executable hook. Its process-local StatesTxt write is
  restricted to state 115 overlay1 when the value is exactly vanilla `224`;
  custom mappings are preserved, and neither Monster Display nor Bind And
  Summon owns this data cell in the audited builds.
- Melee Splash owns the outer damage-fill entry `0x44C030`, not the internal
  production seam `0x44CB32`.
- Resistance Floor owns the two internal clamp operands `0x4524C4` and
  `0x4524E7`; calls made by Burn Damage Fix naturally traverse those relays.
- The five pinned eezstreet PluginPack DLLs own neither Burn seam. Their normal
  Fire-cap operands inside the resolver remain intact and visible to the live
  call.
- Prevent Merc Death in Town owns `0x448C00`; the two selected dispatchers are
  upstream and do not overlap it.
- BKVCombat borrows `UNITS_SetOverlay` for Crushing Blow. Burn Damage Fix skips
  no native callback and shares the direct-overlay channel's last-write-wins
  behavior. A foreign `unit_dooverlay` value is counted and then replaced by
  `fire_hit`; Crushing Blow mechanics are untouched, but either one-shot visual
  may be shortened. Curse, aura and Crushing-Blow visual coexistence still
  require gameplay qualification.

Every enabled surface is fingerprinted before hook installation. A signature,
layout, ABI or ownership mismatch causes a fail-closed load refusal. The
observed build name never decides whether the DLL loads.
