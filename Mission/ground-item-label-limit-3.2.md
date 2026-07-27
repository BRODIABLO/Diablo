# Ground item label limit — D2R 3.2.92777

## Goal

Raise the vanilla limit of 32 simultaneous ground item labels without changing
item drops, pickup behavior or loot-filter rules.

## Proven rendering path

The persistent 92777 workbench establishes the following chain:

1. `ITEMS_GetDisplayName` at RVA `0x9A1B0` resolves the localized display name
   for an item unit.
2. `UI_BuildGroundItemTooltip` at RVA `0xCBEB0` has the independently known 2.4
   ABI `(unit, textBuffer, bufferSize, colorCode)` and calls the name resolver.
3. Its only direct caller is the ground-label record builder at RVA `0x1FA9F0`.
4. The remastered renderer calls that builder while filling `0x144`-byte label
   records in the collection managed around RVA `0x1516D60`.

At RVA `0x1516EBE`, the renderer compares the collection size with `0x20`. If
the collection is larger, it erases records beginning at
`base + 0x2880`, where `0x2880 = 32 * 0x144`. If it is smaller, RVA
`0x1516F41` appends `32 - size` default records. This proves a fixed logical
capacity of 32 backed by a dynamically allocated vector.

A second synchronized layout list is normalized to 32 nodes around RVA
`0x1519A14`. Its comparisons at `0x1519A4F`, `0x1519AAA` and `0x1519AF9`
must change with the label-record collection.

## Implementation

On 2026-07-27, Vincent confirmed `items` as the future PluginPack category,
`plugin-items.dll` as the owner and `items.groundItemLabels` as the future key.
The earlier experimental merge into `plugin-misc.dll` was therefore reverted.

During incubation, `GroundItemLabelLimit` remains an autonomous hybrid DLL by
`RuffnecKk`; it neither links nor redistributes an eezstreet DLL. Version 1.1.0
reads the commentable `GroundItemLabelLimit.json` from the active mod first and
the game directory second. Its contract is deliberately narrow:

- `enabled=false` keeps the vanilla limit of 32;
- `enabled=true, limit=64` selects the proven 64 encoding;
- `enabled=true, limit=128` selects the wider 128 encoding;
- every other key, type or limit is rejected explicitly.

The plugin validates all seven complete 92777 signatures before performing any
write. The 64 path changes the existing immediates. The 128 path re-encodes the
three register comparisons with 16-bit immediates, uses the exact `0xA200`
record endpoint and preserves the proven bounded-byte treatment of the layout
list. No TOML is used.

## Static validation

- Release build: MSVC 19.44, x64.
- Version 1.1.0: SHA-256
  `CEEAF9A56E2C43E39A4719B7F993EC5A3915D686AC2FA112D139A03B9739541F`.
- Policy tests pass 1/1 and cover built-in defaults, comments, disabled mode,
  exact limits 64/128, invalid limits/types/keys and `0x144` offset calculation.
- The DLL exports the three D2RLoader entry points, carries the exact author
  `RuffnecKk` and has no dependency on an eezstreet DLL.
- All seven original byte signatures come from the verified local analysis
  image whose canonical build is D2R 3.2.92777.

Vincent confirmed in game on 2026-07-26 that both the 64-label and 128-label
witnesses function. The exact rendering mode, input mode, pile density, frame
cost and long-session stability were not recorded separately, so those remain
open compatibility and product checks rather than inferred passes.

### Autonomous 1.1.0 validation matrix

| Domain | Case | Expected | Status |
|---|---|---|---|
| Build | Release x64, manifest and exports | DLL builds; three D2RLoader exports | passed |
| Config | Default, disabled, 64 and 128 | Accepted with exact effective limit | passed — policy test 1/1 |
| Config | Other limits, invalid types and unknown keys | Explicit refusal | passed — policy test 1/1 |
| Ownership | `plugin-misc` rollback | No label feature or config remains under `misc` | passed — source/runtime hashes and cold start |
| Loading | Mod-local autonomous DLL with 64 | Seven patches accepted | not run |
| Loading | Mod-local autonomous DLL with 128 | Seven wider patches accepted | not run |
| Loading | Global DLL and configuration fallback | Same plugin ID and behavior | not run |
| Gameplay | 64/128 rendering and input comparison | Stable labels with acceptable usability | not run for 1.1.0 |

## Fixed 64/128 memory-patch presets — 2026-07-26

Vincent requested a fixed 128-label witness to decide whether the larger cap is
functional and desirable. The supplied three-site JSON cannot be raised from
64 to 128 by replacing `0x40` with `0x80`: it omits the four coupled layout-list
sites, and the original `cmp r64, imm8` instructions sign-extend `0x80` to a
negative 64-bit value.

The prepared bundle is stored outside every active D2RLoader patch directory at
`addons/GroundItemLabelLimitMemoryPatches/`. D2RLoader's patch JSON format has
no numeric option field, so it contains two mutually exclusive patchsets:

- `presets/64/increase-floor-item-label-cap-to-64.json` patches all seven
  coupled sites. This supersedes the supplied three-site 64 witness without
  changing its requested limit.
- `presets/128/increase-floor-item-label-cap-to-128.json` uses `cmp ax, 0x80`
  for the register comparisons, the exact `0xA200` record-array endpoint
  (`128 * 0x144`), and a bounded byte comparison only for the final layout-list
  shrink loop. That byte comparison is safe for this fixed experiment because
  the list starts at the vanilla 32 entries and this patch can only grow it to
  128; it is not evidence for arbitrary limits above 128.

Vanilla 32 means loading neither preset. Exactly one of the two JSONs may be
loaded for 64 or 128, and any PluginPack or standalone plugin that owns the same
seven sites must be disabled first.

The standalone 1.1.0 DLL remains stored with the non-loadable
`.standalone-disabled` extension and its JSON has not been copied to the active
runtime. D2RLoader patch JSON has no variables, so the separate memory-patch
bundle still offers exact 64 and 128 presets rather than a numeric field.

### Fixed-preset validation matrix

| Domain | Case | Expected | Status |
|---|---|---|---|
| Static | 64 preset against the seven original 92777 signatures | All match the verified image | passed — 7/7 |
| Static | 128 preset against the seven original 92777 signatures | All match the verified image | passed — 7/7; prior full patch directory 66/66 |
| Loading | Temporary 128 JSON witness with PluginPack feature disabled | One owner; patch applied | passed technically, then removed from active runtime |
| Cold start | 128 witness on D2R build 92777 | Startup completes without rejection or failure | passed technically — 24/24 in 3.5 s; no gameplay claim |
| Gameplay | 64 fixed witness | Functions in game | passed — confirmed by Vincent; detailed conditions not recorded |
| Gameplay | 128 fixed witness | Functions in game | passed — confirmed by Vincent; detailed conditions not recorded |
| Rendering | Remastered and legacy | Stable layout with no crash or corrupted labels | not run |
| Input | Mouse and controller label reveal | Stable and usable | not run |
| Product | Readability and frame cost | Vincent decides whether 128 is desirable | not run |

The transient 2026-07-26 15:51 cold start loaded
`increase-floor-item-label-cap-to-128.json` with seven patches. D2RLoader
reported `scanned=21 applied=21 disabled=0 failed=0`, then
`scanned=22 active=22 disabled=0 rejected=0 failed=0`, and reached startup
step 24/24 in 3.5 seconds. No Ground Item Label Limit activation line came
from `plugin-misc`. This proves only that the patch loaded; no dense-pile,
rendering, input, stability or desirability case was run in game.

On 2026-07-27, the incorrect `plugin-misc` ownership was removed from both the
governed source and active BKVince profile. `D2RPlugins.json` no longer contains
`misc.groundItemLabels`; source/runtime SHA-256 are both
`3119AC5C934A08287068C832467BF7D5711B4B362AD623665CF605FF2C73A87A`.
The rebuilt `plugin-misc.dll` preserves only its required eezstreet
`NativeHooks` correction; source/runtime SHA-256 are both
`4831EDDE0FBBFD3F01EB6F5AAFF7B9EFA476B78AB78850E9830CFEC519B50194`.

The fresh 07:49 cold start loaded `plugin-misc` 2.0.1 with flags `0x2` and its
existing `0x542F40` hook, but emitted no Ground Item Label activation. D2RLoader
reported memory patches `20/20`, plugins `active=24`, `rejected=0`, `failed=0`
and startup `24/24` in 4.062 seconds. The test instance was then closed. With no
standalone DLL or memory preset loaded, active BKVince is currently vanilla 32.

Next gate: only when Vincent requests a new test window, deploy the autonomous
1.1.0 witness and validate both JSON choices 64 and 128, invalid-value refusal,
mod-local/global configuration priority and the rendering/input matrix. The
future merge belongs in `plugin-items.dll`, never `plugin-misc.dll`.
