# Burn Damage Fix native hook contract

Targets: D2R `3.2.92777` and `3.3.93847`, using the verified common native
corpus whose historical source image is 3.2.92777.

| RVA | Ownership | Expected witness | Contract |
|---|---|---|---|
| `0x44CB32` | Exclusive, 6-byte rel32 mid-hook | `81 C3 3C 01 00 00 41 0F 48 DE` | Replaces the bogus `add ebx,316`, consumes the already advanced RNG value in `R8D`, adds a native max-exclusive `burningmin..burningmax` roll with Fire Mastery, recreates the sign flag and resumes at `0x44CB38`. |
| `0x451380` | Exclusive inline entry hook | `45 85 C9 0F 8E E6 01 00 00 48 89 6C 24 20 56 41 56 41 57 48 83 EC 40` | Resolves positive Burn through Fire Resistance once, then calls the original Burn application trampoline. |
| `0x4523E0` | Borrowed live function; never patched or signature-owned | Executable address only | Calls the resolver currently installed at this address with `dontAbsorb=1`. This is the deliberate composition seam with Monster Display and Resistance Floor. |
| `0x3351B0` | Borrowed diagnostic predicate; never patched | `48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 8B DA 48 8B F1 E8 07 68 01 00 85 C0 74 0E 83 E8 01` | With diagnostics enabled, observes state `burning` (115) after the original positive Burn application and increments active/missing counters. It never toggles the state. |

The production relay uses a private executable page allocated within rel32
reach. Its absolute jump targets the plugin's MASM relay; the loader-managed
patch at `0x44CB32` remains the only D2R executable write for this feature.

The native application path calls `STATES_ToggleState 0x3354C0` with state 115
after validating positive damage and duration. The governed 3.2, 3.3 and
BKVince tables all map state 115 to overlay `burning` (224), whose asset is
`Expansion\\On_Fire`. Burn Damage Fix leaves that native state/overlay path
unchanged and uses `0x3351B0` only as a post-application witness.

## Coexistence

- Monster Display has a binary reference to `0x4523E0` but none to `0x44CB32`
  or `0x451380`. Burn Damage Fix therefore validates the resolver only as a
  live executable address and observes Monster Display in either load order.
- Bind And Summon has no binary reference or exact signature match for any of
  the three Burn sites.
- Melee Splash owns the outer damage-fill entry `0x44C030`, not the internal
  production seam `0x44CB32`.
- Resistance Floor owns the two internal clamp operands `0x4524C4` and
  `0x4524E7`; calls made by Burn Damage Fix naturally traverse those relays.
- The five pinned eezstreet PluginPack DLLs own neither Burn seam. Their normal
  Fire-cap operands inside the resolver remain intact and visible to the live
  call.

Every strict witness is checked before hook installation. A build mismatch,
signature mismatch or existing owner causes a fail-closed load refusal.
