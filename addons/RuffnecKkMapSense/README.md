# RuffnecKk MapSense

RuffnecKk MapSense is an experimental D2RLoader client plugin for Diablo II:
Resurrected targeting builds 3.2.92777 and 3.3.93847. Version 0.12.4 is the
current source candidate and combines the native
map-reveal foundation, a compact in-game settings panel, simultaneous
hostile-monster markers, configurable immunity indicators, and Direct
navigation with static normal-quest routes and exact quest-object endpoints.
Labels, GPS, and projectile collectors remain planned.

## Live monster markers

While D2R's native automap is visible, MapSense uses the local-player automap
pass as a safe rendezvous for a two-tier monster pipeline. At most once every
100 ms (10 Hz), bounded discovery scans D2R's complete client monster hash
table and copies only the qualifying unit IDs, ranks, and immunity masks into
reusable storage. The table contract is proven from the current native
`CLIENT_GetUnitByIdAndType` and hash-chain witnesses: 128 buckets for monster
type 1 and `Unit+0x158` as the next link. Between discovery passes, a bounded
refresh resolves those copied IDs through `CLIENT_GetUnitByIdAndType` on every
local-player automap pulse, reads each current `DynamicPath`, and republishes
its current automap position without an artificial 16 ms ceiling. The hot path
is hard-capped by the complete 32,768-entry client-table traversal limit. Each
producer pulse replaces one complete latest-position snapshot instead of
appending duplicate IDs while `Present` is late. No `Unit`,
`DynamicPath`, automap-context, seed,
or pixel pointer survives either pass, and a missing ID is discarded.

Every living, Evil-aligned hostile present in D2R's complete client monster
table is keyed by native unit ID and considered simultaneously. Version 0.12.4
has no configurable scan radius and no spatial rejection: MapSense publishes
every qualifying client-known monster that D2R's native projection accepts
inside the active automap clip. Closing the automap lets observations expire
after 250 ms, so no MapSense marker is intended to remain on normal gameplay.

MapSense cannot manufacture a monster the server has not replicated to the
client and does not bypass D2R's active automap viewport. `mapsense status`
reports the current tracked set, discovery and per-pulse refresh counts with
average/maximum microsecond timings, resolved/missing ID lookups, and accepted
or clip-rejected observations in the 0–80, 81–140, 141–220, and beyond-220
world-subtile diagnostic bands. Those bands measure the real client-table and
projection result; they do not filter it.

Monster admission fails closed before the renderer-side unit-ID cache. A
candidate must be a living `UnitMonster`, must not carry the native mercenary or
asynchronous-actor flags, and must resolve to a killable MonStats record that is
not marked NPC, interactive, or in-town. The dedicated runtime alignment getter
must then return `Evil (0)`. Missing flags, metadata, or alignment state reject
the candidate instead of treating a default zero as hostile. On August 24,
2026, Vincent confirmed in the normal BKVince profile that NPC and ambient-actor
markers are gone while real hostile monsters remain visible. Mercenaries,
summons, converted monsters, and broader gameplay matrices remain open.

Normal, Minion, Champion, Unique, and Super Unique / Boss are always visible;
their tables configure appearance rather than visibility. Shape, color, alpha,
and size are configurable independently for every category. `x` and
`player_cross` also expose an independent per-category thickness. `dot` exposes
size only: its fixed one-pixel dark outline is part of the renderer and its
configured size remains the colored disk's diameter; the outline is added
outside it. Schema-4 defaults
use `player_cross`: a new vector silhouette modeled after D2R's native player
marker, with a customizable color, not the native marker texture. `x` preserves
the earlier hollow angular cross, and `dot` provides a compact circular marker.
Default colors remain white Normal, yellow Minion, blue Champion, orange Unique,
and red Super Unique / Boss.

The first in-game run of 0.7.0 exposed a rank-priority defect: Champions were
classified as Unique. Version 0.7.1 routes production and tests through the same
helper and applies `SuperUnique > Champion > Unique > Minion > Normal`.
Vincent visually confirmed that Champions are blue while Uniques are orange;
version 0.9.3 preserves that corrected classification.

The native hook always calls D2R first and exactly once. It performs no room
enumeration, retains no game pointer, and publishes only copied values from its
bounded client-unit scan through a double-buffered observation journal. The journal preallocates
65,536 observations per side outside the hook, so ordinary unit visits allocate
nothing. It grows in reusable chunks only if that high-water mark is exceeded,
and never applies a top-N policy or evicts one qualifying monster in favor of
another. The renderer's unit-ID cache is dynamic. Native automap clipping is
reproduced before publication. D2R native UI dimensions are also captured; if
their scale or aspect ratio does not agree with the current ImGui frame, markers
are suppressed instead of being drawn at uncertain positions.

This is the first multi-monster visual candidate, not the final monster radar.
Exact visual alignment, category classification, client-replication reach, zoom,
corner-map modes, and ultrawide behavior still require in-game approval.

## Immunity indicators

When immunity display is enabled, MapSense reads the live monster resistance
stats only after the unit has passed the hostile, projection, and native
automap clipping gates. A resistance of 100 or greater sets one copied bit for
Physical, Fire, Cold, Lightning, Poison, or Magic; no game pointer reaches the
renderer cache. Disabling immunity display also disables these additional stat
reads.

Two live-configurable styles are available:

- `colored_i` draws one colored `i` above the marker for each immunity, using
  one row for up to three immunities and two rows for four through six;
- `split_halo` surrounds the marker with one complete ring or equal colored
  segments for two through six simultaneous immunities.

Physical defaults to beige; Fire red, Cold blue, Lightning yellow, Poison green,
and Magic purple remain independently configurable. Indicator size, halo
thickness, all six colors, and the active style persist in the normal MapSense
TOML. Runtime visual approval of both styles remains open.

## Direct navigation candidate

MapSense 0.10.0 introduced straight-line navigation to D2R's native automap,
0.11.0 extended its progression policy across all five acts, and 0.12.0 adds
the exact correct-tomb adapter for the Canyon of the Magi. Version 0.12.4 adds
the static normal-quest route policy. It does not
redraw the map and does not claim that a line is walkable: Direct mode is a
direction indicator and may cross walls. The runtime candidate exposes three
real destination families:

- a blue line to the exact generated waypoint `PresetUnit` position in the
  current level, when one exists;
- a green line to the explicit main-progression exit throughout Acts I-V;
  outdoor hubs ignore optional entrances, and shared campaign quest routes
  stay green; after Duriel's quest is rewarded, the correct Tal Rasha tomb
  remains a green farming destination;
- red lines to whitelisted normal-quest side routes and proven quest-object
  presets, independent of quest-log state; the Canyon keeps its exact
  red-before-reward and green-after-reward exception;
- purple lines to directly connected levels selected by the player.

Activation, color, and common line thickness for all four families are edited
live in the normal MapSense menu. Only the purple target list is edited
manually in TOML. Every
entry contains exactly one `level_id` or canonical English `level_name`.
Ambiguous names such as `Tal Rasha's Tomb`, `Sewers Level 1`, and `Tristram`
fail closed; `level_id` remains available for those cases. The shipped examples
cover Pit Level 1, Mausoleum, Ancient Tunnels, and Icy Cellar.

The resolver runs only on D2RLoader's gameplay/UI lifecycle callback. D2R's
native `DRLGROOM_FindWaypointRoomAndCoordinates` resolver selects the waypoint
room and preset. MapSense validates that preset's object class and native
waypoint subclass, then publishes the raw generated world position
`roomTile * 5 + presetRelative` without replacing it with a nearby runtime
Unit. This matches the waypoint POI contract used by PrimeMH and d2mapapi. For
every copied type-5 source preset, MapSense materializes the source room, maps
its `LvlWarp+0x2C` identifier to the matching `PresetUnit`, and reads the true
destination `DrlgRoom` directly from `RoomTile+0`. The line endpoint remains
the exact source preset point. This deliberately avoids
`DRLGWARP_ResolveRoomTileLink`: that native helper additionally requires the
destination's reciprocal RoomTile and can return null while the source-side
exit is already complete. The direct layout is covered by an independent
fail-closed witness at `0x3DA9FB`.

Already materialized outdoor exits are discovered from D2R's native near-room
vector at `DrlgRoom+0x10`; its 64-bit element count is read from
`DrlgRoom+0x18`.
Version 0.9.4 incorrectly treated the flags at `DrlgRoom+0x50` as a 16-bit
count, which could hide the Cold Plains to Stony Field room link entirely.
Since version 0.9.9 MapSense also reads D2R's dynamic eight-slot Vis/Warp topology
through the governed accessors at `0x360880` and `0x3DAAD0`. For a direct
outdoor `Warp=-1` pair, it requires reciprocal Vis slots, the matching source
and target room flag bits, and exact cardinal room adjacency. Navigation now
uses the same captured client DRLG as Reveal, initializes the requested target
level through the already governed `DRLG_GetLevel` / `DRLG_InitLevel` path,
and materializes each needed client room through the fingerprinted
`DRLGROOM_CreateActiveRoom` helper. Version 0.12.2 collects both collision sides
for each exact native `RoomsNear` pair and retains that pair's source-room
identity, target-room identity, and fixed seam coordinate while merging
fragments. A type-5 `RoomTile` remains higher-priority evidence. For an outdoor
fallback, exactly one disconnected shared opening must remain; its midpoint is
published on the source cell beside the shared seam (`fixed` for Left/Top and
`fixed - 1` for Right/Bottom). Parallel seams and different room pairs cannot
be cross-matched. Multiple disconnected
openings are ambiguous and fail closed instead of selecting the widest seam.
No rectangle-center, internal room seam, one-sided opening, or loaded-room
fallback is published.

Transitions represented by portal objects do not use room centers or guessed
coordinates. MapSense traverses each materialized room's bounded native Unit
chain and accepts only an object whose class is mapped for that exact source
level: Palace Cellar 3 to Arcane Sanctuary (298), Arcane Sanctuary to Canyon of
the Magi (60), the true Tal Rasha tomb to Duriel's Lair (100), Durance of Hate
3 to Pandemonium Fortress (342), Chaos Sanctuary to Harrogath (566), and Throne
of Destruction to the Worldstone Chamber (563). In Arcane Sanctuary, the active
class-60 portal wins when present; before it spawns, the exact generated
Summoner monster preset (250) wins over the Arcane Tome object preset (357).
This mirrors PrimeMH's Boss/Quest POIs and D2MOO's portal-creation sequence
without importing any 32-bit address or ABI. The active Unit's native
client/dimetric coordinates are copied unchanged, matching D2R's own automap
projection. Dynamic-only levels are polled at most once per second while the
automap is rendering, stop polling as soon as their green destination exists,
and never retry forever when a quest has not spawned the portal yet.

Version 0.12.4 passively scans each room's generated presets before requesting
native room activation. Ordinary rooms without a type-5 exit are no longer
materialized for RoomTile discovery; dynamic portal levels retain their
all-room scan, and type-5 exits plus exact outdoor collision pairs retain the
activation needed to complete their native data. A bounded per-refresh cache
also prevents the same unresolved room from being requested repeatedly by
collision and RoomTile paths.

In the Canyon of the Magi, MapSense reads the generated staff tomb from the
active DRLG and accepts only a Tal Rasha tomb level from 66 through 72. It
initializes only that selected target, then reuses the exact RoomTile endpoint
for both states: the quest-target line is red until Act II Quest 6 reports
`RewardGranted`, and the main-progression line is green afterward for Duriel
farming. If the current-difficulty quest record, generated tomb, or exact exit
is unavailable, the resolver remains retryable and publishes no approximation.

Version 0.12.4 gives the existing `navigation.quests` option a static,
quest-log-independent policy. Green remains the main campaign route. Red marks
only side destinations associated with ordinary act quests: Den of Evil,
Burial Grounds, Forgotten Tower and its cellar chain; Lut Gholein Sewers,
Halls of the Dead and Maggot Lair; Spider Cavern, Flayer Dungeon, Kurast
Sewers and Ruined Temple; Frozen River and the Nihlathak dungeon chain. Upper
Kurast and Kurast Bazaar both recognize their generated Kurast Sewers exits.
Pit, Crypt, Mausoleum, Ancient Tunnels, Arachnid Lair, Swampy Pit, optional
Kurast temples, icy farming caves, red portals, Cow and Pandemonium areas are
deliberately absent. Shared campaign routes such as Andariel, Claw Viper
Temple, Durance, Chaos Sanctuary, Ancients and Baal remain green.

The same passive preset scan publishes proven quest objects in their own
levels, including the Cairn Stone, Tree of Inifuss, Cain's Gibbet, Horadric
Malus, Cube and Staff chests, Tainted Sun altar, Horadric Staff orifice,
Khalim organ chests, Lam Esen's Tome, Compelling Orb, Hellforge, frozen Anya
and the Ancients' altar. Repeated captive-cage presets are tagged as one
dynamic group: all immutable endpoints are discovered once, and every automap pass
renders only the cage nearest to the current player without another DRLG scan.
Diablo's five seals are intentionally excluded.

No `PresetUnit` pointer survives a native resolver call: source identifiers and
coordinates are copied before D2R may initialize a room. Version 0.10.0 first
initializes the explicit progression target and configured custom targets that
are direct `Vis` neighbours, then
materializes every current-level source room before reading its RoomTile chain.
The native town
predicate suppresses every navigation destination, performs no waypoint/exit
resolution, and explicitly clears the previous level's lines while the player
is in town. Null native results produce no guessed destination. Malformed
pointers, cycles, oversized chains, signature mismatches, unknown names, and
inconsistent room relationships fail closed.

During D2R's local-player automap pass, MapSense observes the proven current
`levelId`. A mismatch clears every destination from the previous level before
another line can be projected, rejects a late publication for that stale level,
and asks the UI thread to resolve the new level. If an exact static
main-progression exit is not ready, up to eight 250 ms refreshes retry it
without publishing a guessed line. A dynamic portal's absence is not an error.
No retry or DRLG traversal runs in Present.

Only copied coordinates and destination kinds cross into the automap observer.
Exit destinations remain world subtiles until the native automap boundary,
where they are converted to D2R client/dimetric coordinates with
`clientX = 16 * (subtileX - subtileY)` and
`clientY = 8 * (subtileX + subtileY)`. The local player already provides
authoritative native client coordinates; the waypoint remains the exact
generated preset world position selected by D2R's native waypoint resolver.
Projection occurs synchronously during D2R's existing local-player automap
pass; Present consumes short-lived immutable line snapshots and reads no DRLG
pointer. Static quest routing and the Canyon adapter use the existing Quest
Targets control in the panel. GPS routing is a later
collision/room-graph lot and never silently replaces Direct mode.

## In-frame settings panel

MapSense renders Dear ImGui inside D2R's existing DirectX 12 frame. It creates
no second game-sized window, external overlay process, OpenGL/WGL context, or
independent render thread.

The movable launcher appears only after D2R reports `LocalPlayerReady` and is
hidden again on `GameLeft`. It expands into a deliberately small accordion
panel. The current candidate contains:

- **Additions Opacity**, which controls the current monster markers and will
  also apply to later MapSense additions; it never controls D2R's native
  automap opacity;
- **Reveal Level**;
- **Reveal Act**;
- **Toggle Reveal All**;
- **Reveal All Off**;
- independent shape, color, alpha, and marker size for Normal, Minion,
  Champion, Unique, and Super Unique / Boss; each `X` or `Player Cross` category
  also exposes its own **Thickness**, while `Dot` deliberately does not;
- immunity display mode, indicator size or halo thickness, and six element
  colors.
- Direct-navigation thickness plus independent activation and colors for the
  waypoint, main progression, and custom-level lines; the custom section also
  shows the number of manually configured target levels.

Each color opens in a compact picker; permanent technical RGBA fields are not
shown in the panel. Hovering the color preview displays a custom technical
tooltip above the pointer. Theme selection and localization are deferred.

Closing or collapsing the panel returns to the launcher. Appearance, position,
and opacity changes are saved to the active MapSense configuration
scope. Mouse input owned by the panel is isolated from gameplay, while keyboard
and mouse input outside
its exact bounds remain D2R-owned. Dear ImGui does not create, show, hide, or
reshape a Windows cursor; D2R remains the sole cursor owner. This interaction
still requires gameplay qualification before a public release.

`Tab` is reserved exclusively for D2R's native automap. The Win32 subclass
rejects Tab key-down, key-up, system-key, and character messages before the
ImGui backend sees them; the backend and action callback repeat the same guard.
Even if a player binds `Toggle MapSense Settings` to Tab in Controls, MapSense
ignores the action for every modifier combination, never queues a menu change,
and leaves the key to D2R. Other configured settings keys and the
`mapsense menu` console command are unchanged.

### Fail-closed DirectX 12 ownership

MapSense 0.12.0 no longer hooks `ExecuteCommandLists` process-wide or assumes
that the first observed Direct queue owns D2R's swap chain. It intercepts the
standard DXGI `CreateSwapChain*` entry points and records the exact Direct
command queue supplied for each returned swap chain. `Present` is always passed
through, but MapSense submits no GPU work unless that exact association and the
swap-chain device identity both match.

The renderer becomes permanently fail-closed for the process after a queue
identity change, fence failure, allocator/list failure, queue-signal failure,
or DXGI device-removal result. The original `Present` result is never hidden or
replaced. A registered overlay client may keep CPU-side ImGui frames alive, but
frames with no command lists or vertices never reset or submit a D3D12 command
list. This leaves the shared MapSense/Floating Damage host GPU-dormant between
visible draws.

Version 0.12.1 also removes the fence wait from the latency-critical `Present`
path. If the allocator for the current back buffer is still in flight, MapSense
skips only that overlay frame and immediately lets D2R continue presenting; it
retries on a later back-buffer pass after the fence advances. Fence waits remain
limited to renderer lifecycle operations such as resize and shutdown. This is
a source/build property of the current candidate; its gameplay FPS and marker
fluidity gates remain to be measured.

## Configuration migration

MapSense 0.12.2 writes configuration schema 9. Existing schemas 1 through 8 are
accepted only when they do not contain a removed key. Schemas 1 through 3
migrate with `x` for every category, preserving
the earlier hollow angular-cross appearance instead of silently changing marker
shapes. Legacy category visibility switches do not silently disable categories:
schema 4 and later always display all five categories.

Schema 9 removes the global `monsters.marker_thickness` prototype setting
without a compatibility shim. Each category stores its own `thickness` when its
shape is `x` or `player_cross`; a `dot` table accepts and writes only its shape,
color, and size. A prototype TOML that still contains the removed global key is
rejected until that line is removed or the file is regenerated.

Version 0.12.2 retires `monsters.detection_radius` because it was only a
post-enumeration display filter, not a scan distance. Existing schema 1 through
8 files may still contain the key; it is accepted and ignored for compatibility,
then omitted the next time MapSense saves the file. The runtime now considers
the complete client monster table and retains only D2R's native projection and
active automap-clip gates.

Schema 5 adds immunity style, indicator size, and halo thickness. Earlier
schemas retain their immunity enabled state. Only the exact former default
Physical grey `#C7C7C7FF` migrates to the new beige `#D8C39AFF`; a customized
Physical color is preserved.

Schema 6 adds Direct-navigation line settings and the strict custom-level
target list. Every ordinary navigation setting is controlled from the in-game
panel; the target list is the sole intentional manual TOML exception.

Schema 7 historically changed monster-radius semantics from client/dimetric
units to world subtiles; that setting is now retired. Schema 8 activates the
red quest family and
exposes its switch and color in the in-game menu. Because schema 7 kept the
reserved quest switch hidden and disabled, its exact old false value migrates
once to enabled; schema 8 then preserves the player's explicit choice. Saving
any accepted legacy configuration writes schema 9.

## Renderer ownership and optional coexistence

MapSense and Floating Damage remain independent plugins:

- MapSense alone owns its in-frame renderer;
- Floating Damage alone keeps its autonomous renderer;
- when both are installed, MapSense has deterministic renderer priority and
  Floating Damage registers as an optional client of the same ImGui frame.

The shared API is versioned and rejects incompatible ImGui builds. MapSense is
not linked to Floating Damage and does not require it to load. Floating Damage
is expected to recover its autonomous renderer if a MapSense host goes away.

## Reveal controls

The plugin registers four independent actions in the D2R Controls menu under
`RuffnecKk Suite`. They intentionally have no default binding:

- `Reveal Current Level` remembers the stable current LevelId and calls the
  native automap callback for every room in that level. Its stable logical ID
  remains `reveal-zone` for existing bindings.
- `Reveal Current Act` remembers the act derived from the authoritative
  LevelId ranges, reveals the active level natively, then applies the same
  current-level operation as each later level in that act loads.
- `Toggle Reveal All Acts` reveals the active level and arms that current-level
  operation for every later loaded level in the current difficulty. Press it
  again to disarm.
- `Toggle MapSense Settings` expands or collapses the settings panel.

D2R exposes generated client geometry incrementally. Version 0.12.2 therefore
never mass-initializes unloaded acts and never dispatches D2RCore's `revealmap`
console worker: each active client level is revealed directly through the
already fingerprinted room callback when it loads. The plugin keeps remembered
LevelIds, ActIds, and Reveal All intent for the lifetime of the MapSense
process. Save & Exit followed by a new game at the same difficulty resets only
per-session acceptance/deduplication and reapplies matching remembered intent
as the new geometry appears, with at most eight retries spaced 250 ms apart.
An actual Normal/Nightmare/Hell change clears all remembered Reveal intent;
an unknown difficulty fails closed. `off` only disarms progressive Reveal All
and does not erase automap exploration already held by D2R. This memory is not
written to TOML or a character save and does not survive a process restart.

The equivalent console commands are:

```text
mapsense status
mapsense level
mapsense zone
mapsense act
mapsense all
mapsense off
mapsense menu
```

`mapsense zone` is retained as a compatibility alias for `mapsense level`.
MapSense 0.12.2 contains no `ExecuteConsoleCommand` or `revealmap` path. This
removes one former asynchronous source of room initialization. Necessary native
room activation can still reach D2R's guarded negative-index
`sFillLocation()` diagnostic. Version 0.12.3 owns a fail-closed, D2RLoader-
tracked suppression of that diagnostic CALL inside the DLL, so no separate mod
patch is required and the underlying guarded skip behavior remains unchanged.

The second `mapsense status` line describes the monster pipeline explicitly:
tracked IDs; discovery scans and average/maximum time; position refreshes and
average/maximum time; resolved/missing ID lookups; accepted distance bands; and
native-clip rejects by distance band. These counters measure the complete
client-table result but do not manufacture client Units or bypass D2R's automap
viewport.

D2RLoader InputService v1 supports no modifier or one of Ctrl, Alt, or Shift.
Windows-key and multiple-modifier combinations require another proven input
contract.

## Installation

Install `RuffnecKkMapSense.dll` either globally under
`<D2R>/d2rloader/plugins/` or for one mod under
`<D2R>/mods/<mod>/d2rloader/plugins/`. Both scopes are supported by the same
hybrid DLL. D2RLoader creates `ruffneckk-mapsense.toml` in the matching config
scope from the default embedded in the plugin.

No Diablo II: Lord of Destruction installation, MPQ, map server, companion EXE,
or external overlay is required. Reveal Level, Reveal Act, and Reveal All use
D2R's current native client DRLG and automap callback; Act and All differ only
in the process-lifetime intent that is reapplied as matching levels load. The
plugin owns two independent signature-checked hooks:
the existing level-initialization hook captures the current client DRLG, while
the automap-unit hook publishes copied monster observations and projects Direct
navigation during the local-player pass. The separate navigation resolver adds
no hook and validates every native helper signature before reading the active
level. If its proofs fail, navigation is disabled while Reveal, markers, and the
panel remain available according to their own independent gates.

The plugin defines no custom save format and does not alter character-save
data. Automap exploration remains owned by D2R.

## Compatibility status

MapSense uses D2RLoader/D2RCore 1.1.0-beta and PluginSDK API v3 at commit
`4933e2c42cb2592958cd0df3b6dc5003102252d1`. Build names and version numbers are
diagnostic only. Before every hook or native access, the plugin validates the
complete fail-closed fingerprint for the exact RVAs, signatures, layout/ABI
witnesses, and ranges it uses. An unnamed build may load only when that entire
fingerprint matches; a named build is still refused when any witness differs.
Full runtime qualification is performed once on the current official runtime.
Another build is covered without a duplicate gameplay matrix only when the
governed corpus proves every native surface used by MapSense byte-identical,
with the same RVAs, signatures, layout, and ABI. Any difference reopens a
separate runtime qualification.

Version 0.12.2 is currently an offline source candidate. It removes the false
distance control and all spatial monster filtering, refreshes every copied
monster ID on each automap pulse within the 32,768-entry hard cap, replaces
append-only observations with one latest-position producer snapshot, retains
the exact paired `RoomsNear` seam for outdoor navigation, and removes every
D2RCore `revealmap` dispatch. Reveal retries are keyed to the exact LevelId so a
new level cannot inherit an exhausted retry budget. A strict build and CTest
passed before the final test-only and retry/publication hardening edits; at
Vincent's request no further test, runtime deployment, or D2R launch was made
that night. Final `/W4 /WX`, CTest, hash, cold start, performance, fluidity,
Tamoe-to-Monastery, and no-`sFillLocation` gates therefore remain **NOT RUN** for
the exact final 0.12.2 source state.

The 0.12.1 Release x64 source candidate passes the strict `/W4 /WX` compiler
gate and CTest reports 1/1 passing. The DLL is 1,320,960 bytes, exposes the
expected four exports, carries PE version 0.12.1, and has SHA-256
`DAB61AADE352C87B9CA4F57DF1184EAA663617103AC7B1B9B80182066F8C39B4`.
Static tests cover the bounded 10 Hz discovery/about-60 Hz position pipeline,
distance-band diagnostics, unique whole-level outdoor-boundary opening with
ambiguity rejection, and same-difficulty process persistence for Reveal Level,
Reveal Act, and Reveal All with reset on every actual difficulty change. The
Canyon policy remains unchanged: the exact generated tomb is red while the
quest is active and green after reward. The same DLL hash is deployed mod-locally
in the normal BKVince profile with the active TOML preserved byte-identically.
Its fresh official D2R 3.3.93847 cold start on August 28, 2026 loads MapSense
0.12.1 in the complete installed stack, reports 36 loaded plugins, the single
pre-existing Revive Overhaul failure, all 18 memory patches, and reaches 24/24.
The log installs the early DXGI ownership hooks, records the exact command queue
at swap-chain creation, initializes the shared ImGui host, and Floating Damage
renders its first shared frame. No new `nvlddmkm`, `LiveKernelEvent 0x141`, or
matching Windows Error Reporting event appears after this launch. Deployment and
cold start are therefore technically PASS. FPS/fluidity, fixed-scene radius,
Tamoe-to-Monastery, Reveal-across-games, difficulty-change, and Canyon regression
remain gameplay gates; none is inferred from the technical PASS.

The 0.12.0 Release x64 candidate passes the strict `/W4 /WX` compiler gate and
CTest reports 1/1 passing. The DLL is 777,216 bytes, exposes the
expected four exports, carries PE version 0.12.0, and has SHA-256
`7E4F56DC7A47BB6B2EC73232D0A71248F10DB81BCD753F37FFE8BD38A5AB04D9`.
Its renderer binds each swap chain only to the exact Direct queue supplied at
DXGI creation, refuses unbound presentation paths, and skips empty ImGui GPU
submissions. The same hash is deployed mod-locally in BKVince. The fresh D2R
3.3.93847 cold start loads 36 plugins and all 18 memory patches, reaches 24/24,
installs the early DXGI ownership hooks before graphics initialization, records
the exact queue during swap-chain creation, initializes the shared ImGui host,
and renders Floating Damage's first frame through it. Windows reports no new
`nvlddmkm` or `LiveKernelEvent 0x141` event at this technical gate.

The same 0.12.0 artifact replaces the former automap-render subset with a
bounded traversal of all 128 client monster buckets every 50 ms at most. Its
policy tests cover the exact 79/80/81 and 219/220/221 world-subtile boundaries,
and diagnostics separate client presence, radius, projection, clip, and
publication. In the Canyon of the Magi, the resolver reads the generated staff
tomb, initializes only that level, and publishes the same exact RoomTile as a
red quest target before `RewardGranted` or a green farming target afterward.
Schema-7 quest settings migrate once to enabled in memory; schema 8 and later preserve
the player's explicit choice. These source, fingerprint, migration, and policy
gates pass. The fixed-scene 30/80/140/220 comparison and the red/green Canyon
visual witnesses remain gameplay tests rather than inferred successes.

The 0.11.2 Release x64 candidate passes the strict `/W4 /WX` compiler gate and
CTest reports 1/1 passing. The DLL is 766,464 bytes, exposes the expected four
exports, carries PE version 0.11.2, and has SHA-256
`8B7CF8140C895BDBA3017741E64CDD6932C861378A0B894FB39384C86DEA2A4E`.
Schema 7 stores a 30-to-220 circular radius in true world subtiles and migrates
schema-3-through-6 client-coordinate values with documented `/16` rounding.
The native filter validates and calls `UNITS_GetDynamicPath` at `0x34AE80`,
`PATH_GetX` at `0x341A20`, and `PATH_GetY` at `0x341A30`; their strict governed
fingerprints and ABI are covered by the D2R 3.2/3.3 common corpus. Outdoor
regressions require the exact shared opening from both rooms and publish its
destination-side midpoint. Arcane Sanctuary now resolves active portal object
60 first, generated Summoner preset 250 second, and Arcane Tome preset 357
last. The governed reverse-engineering self-test, Release build, policy tests,
PE metadata, and export audit pass. The same DLL is deployed byte-identically
in the normal mod-local BKVince profile. Its fresh official D2R 3.3.93847 cold
start on August 28, 2026 reaches 24/24 with 36 loaded plugins, all five
eezstreet plugins, and 18 memory patches; the only plugin failure is the
pre-existing Revive Overhaul incident. The active schema-6 TOML remains
byte-identical, so its legacy radius of 2500 is migrated in memory to about 156
true world subtiles without a silent rewrite. The targeted radius,
Tamoe-to-Monastery, and Arcane gameplay witness is in progress and has no
premature verdict.

The 0.11.1 Release x64 candidate passes the strict `/W4 /WX` compiler gate and
CTest reports 1/1 passing. The DLL is 763,904 bytes, exposes the expected four
exports, carries PE version 0.11.1, and has SHA-256
`0DE926350D90CCE7668E28652A59BE2505B897FB5C6B9A4DDED24ABEEBCCA429`.
Its exhaustive policy matrix covers 93 source levels and 94 green routes:
82 static RoomTile/outdoor transitions plus 12 exact dynamic-portal routes.
The dynamic rules are keyed by both current level and object class, retain the
active Unit's exact client coordinates, exclude dynamic targets from DRLG
initialization and missing-static-exit retries, and are refreshed only while
the native automap is rendering. Version 0.11.1 also makes projection logging
strictly opt-in and replaces the former 16-slot modulo cache with exact
per-level destination keys. Its regression test uses the real Frigid Highlands
waypoint/progression IDs that previously collided. The byte-identical DLL is
deployed mod-locally on D2R 3.3.93847 and passes the complete 24/24 cold start;
Frigid Highlands and Halls of Pain both retain normal performance with the
automap open in Vincent's August 28, 2026 gameplay test. The targeted
performance regression is `PASS`.

The 0.10.0 Release x64 DLL passes the `/W4 /WX` compiler gate and CTest reports
1/1 passing. The DLL is 760,320 bytes, exposes the expected four exports,
carries PE version 0.10.0, and has SHA-256
`CE06F62D82A72FDE8BFC22001D85828C3BE85E5A90F22B69DD44CBB6B619D405`.
This candidate initializes the explicit progression and directly visible
configured custom levels, materializes every current-level source room, and resolves exact type-5
exit presets through the direct `RoomTile+0` destination pointer without the
reciprocal-link gate. Its offline regressions cover the complete Act I
progression policy, including both Underground Passage transitions, Barracks,
all three Jail transitions, Cathedral, and all Catacombs transitions. They also
cover purple Pit Level 1 and Underground Passage Level 2 branches. The same DLL
is deployed byte-identically in the normal mod-local BKVince profile. Its fresh
D2R 3.3.93847 cold start on August 27, 2026 accepts the complete native
fingerprint, loads all 37 installed plugins and 18 memory patches, reaches
startup 24/24, installs the in-frame D3D12 hooks, captures D2R's command queue,
and initializes the ImGui host. This is a technical cold-start `PASS`. Vincent's
first live witness confirms the purple Tamoe Highland to Pit Level 1 line, whose
endpoint is backed by the exact type-5 `room-tile` preset. The green Tamoe
Highland to Monastery Gate line remains a precision failure: the current
fallback selects the midpoint of a 40-subtile `outdoor-collision` opening rather
than the visible normal entrance. Vincent then confirms the Barracks, Jail, and
Catacombs progression lines in gameplay. Fresh diagnostics publish the expected
28→29, 29→30, 30→31, 31→32, 33→34, 34→35, and 35→36 destinations.
Underground Passage and immunity witnesses remain open until explicit visual
verdicts are recorded.

The 0.9.9 Release x64 DLL passes the `/W4 /WX` compiler gate. No CTest was run.
The DLL is 759,296 bytes, carries PE version 0.9.9, and has SHA-256
`11C7D4B6276903FE4A85E1F387E8EF318B57C02C559BB139B96C515781527C03`.
It was deployed byte-identically in the normal mod-local BKVince profile while
the active TOML remained unchanged at that gate. Its fresh D2R 3.3.93847 cold
start on August 26, 2026 accepted the complete native
fingerprint, loaded MapSense 0.9.9, installed its standalone in-frame D3D12 hooks,
captured D2R's command queue, initialized the ImGui host, and reached startup
24/24 with the complete installed stack: 36 plugins loaded, the same two
pre-existing plugin failures, and all 18 active memory patches. This is a
technical cold-start `PASS`; exact waypoint and progression endpoints remain a
live Act I gameplay witness.

The 0.9.8 Release x64 build passes the `/W4 /WX` compiler gate and CTest reports
1/1 passing. The resulting DLL is 759,296 bytes, exposes the expected four
exports, carries PE version 0.9.8, and has SHA-256
`BB8626AB01B5B176168B5C44B401E58669DC3A4392E352F1C229F326FE9F9DF6`.
It was later deployed and rejected by its gameplay witness: blue remained
misaligned and green was absent in Cold Plains.

The 0.9.7 Release x64 build passes the `/W4 /WX` compiler gate and CTest reports
1/1 passing. The resulting DLL is 755,200 bytes, exposes the expected four
exports, carries PE version 0.9.7, and has SHA-256
`00C7F74B0299D0DA447AC54F5FE5E8323AE8DE6BF230B1587B4628871F20EBF3`.
It is deployed byte-identically in the normal mod-local BKVince profile while
the live TOML remains unchanged at
`5E0239440DB13D76B8A4B318E1BE221DEAA3C81040E3FE0729E5F381D191B613`.

The fresh D2R 3.3.93847 cold start on August 26, 2026 accepts MapSense's complete
native fingerprint, loads MapSense 0.9.7, installs its standalone in-frame
D3D12 hooks, captures D2R's command queue, initializes the ImGui host, and
reaches D2RLoader startup 24/24 with all 18 active memory patches. The complete
installed stack remained present. D2RLoader reports 36 plugins loaded and the
same unrelated Fourth Skill Tree failure already present before this candidate;
MapSense itself is neither rejected nor failed. This is a technical cold-start
`PASS`, not an endpoint-accuracy result. The subsequent Act I witness confirmed
that both lines render, but rejected their endpoints: green still used an
approximate room border and blue still used the waypoint preset rather than the
active object position.
A separate D2R 3.2.92777 runtime is not available locally, so that build's
runtime matrix also remains `NOT RUN`.

The first 0.9.0 Cold Plains witness exposed a resolver defect before any line
could be published: the client-slot context was incorrectly passed to the
Objects table helper, whose null DataTables result faulted while probing the
waypoint and discarded the Stony Field exit already collected. Version 0.9.1
uses the local player's proven unit data context, preflights its DataTables
slot, confines waypoint metadata faults, and retries an empty destination set.
Version 0.9.2 additionally binds each published destination batch to its proven
active level and validates the complete
`UNITS_GetRoom` / `PATH_GetRoom` / `ActiveRoom+0x18` /
`DRLGROOM_GetLevelId` witness chain before enabling navigation. Version 0.9.3
removes every room-center navigation guess, maps a type-5 preset's source-side
identifier through its exact `RoomTile` to the adjacent level, scans all object
presets for the exact waypoint, and clears all destinations in town through the
native town predicate. Its gameplay witness rejected that manual resolver: the
blue line targeted empty space and the expected green Cold Plains to Stony
Field line was absent. Version 0.9.4 replaces both manual probes with D2R's
native waypoint and reciprocal-warp resolvers, but its gameplay witness is also
`FAIL`: the blue line still targeted empty space, the green Cold Plains to
Stony Field line was absent, and Tab still reached the MapSense menu. Version
0.9.5 clears stale destinations immediately on level changes and in town, reads
the proven native near-room vector instead of the unrelated flags field, uses
native room and level accessors throughout, and isolates Tab at the Win32,
ImGui-backend, and action layers. Its gameplay qualification remained `NOT RUN`
and it is superseded by 0.9.7. Version 0.9.7 corrects the projection contract by
converting destination subtiles to client coordinates before the native
automap projector and adds the read-only reciprocal Vis/Warp topology resolver
for direct outdoor transitions. Its gameplay witness proves those mechanisms
produce lines, but also proves that their two structural anchors are not exact.
Version 0.9.8 replaces the blue preset anchor with the matching active Unit's
native client coordinates and replaces the green rectangle anchor with the
widest exact walkable collision opening. It publishes no approximation while
either native object or collision data is unavailable. The former 0.9.6 runtime
DLL is preserved as a local rollback copy under
`analysis-cache/runtime-sync-backups/`; the live profile still contains the
byte-identical 0.9.8 candidate documented above. Its gameplay witness rejected
the blue endpoint and showed that the green line disappeared because most
client collision rooms had not been materialized.

Version 0.9.9 follows the reference pipeline directly: the blue destination is
the raw generated waypoint preset position, while green outdoor destinations
come from exact walkable edge runs after resolving levels and rooms in Reveal's
client DRLG. PrimeMH commit `92b6a97d8e56346f8b63a88bb647c1af044d2c8b`
is used as a behavioral witness only; no PrimeMH source is copied into this
plugin. Version 0.9.9 has been compiled but has not been deployed or launched.

The 0.10.0 Direct-navigation candidate requires Vincent's visual witness. Green
must land on the normal entrance for Stony Field, Underground Passage 1,
Monastery Gate, Barracks, Jail 1/2/3, Cathedral, and Catacombs 1/2/3/4. Purple
must land on the exact Pit Level 1 and Underground Passage Level 2 presets when
configured. Blue must remain present and coincide with the waypoint itself.
Optional branches must never replace the explicit green progression target.
Act II through V progression and all quest targets remain intentionally absent
from this candidate.

Vincent previously confirmed NPC and ambient-actor rejection plus hostile
retention in gameplay. Checks for mercenaries, summons, converted monsters,
`colored_i`, `split_halo`, dense-pack performance, and dynamic resistance
changes remain open. Version 0.7.0 is retained only as the defective historical
candidate that revealed the rank-precedence bug. No ZIP or public release has
been produced.

The current-level traversal is a maintained RuffnecKk port of the earlier
RevealMap prototype. D2MOO is credited as a semantic Diablo II reference; no
legacy address, 32-bit ABI, source file, or binary is copied into this plugin.
D2RMH is credited as the semantic reference for intersecting native collision
openings; MapSense uses independently proven D2R 3.2/3.3 layouts and contains
no copied D2RMH code.
