# RuffnecKk Cast Triggers — D2R 3.2.92777 and 3.3.93847

## Mission

Create a public, intermod D2RLoader plugin that lets mod authors attach native
chance-to-cast effects to items and trigger them when a player casts another
skill. The plugin is an autonomous member of the RuffnecKk D2RLoader Suite and
is not part of BKVince.

Vincent approved implementation on **24 August 2026**. The first bounded
release supports:

- a fixed target-skill level through `cast-skill`;
- the effective source-skill level through `cast-skill-same-level`;
- successful manual player casts using the player cast animation;
- non-repeating casts once and repeating/channelled casts on a governed server-
  frame cadence;
- native item chance rolls and native item-skill targeting.

On **25 August 2026**, tester evidence added two release-candidate corrections:
the target router must cover unit, position and self/subject proc skills even
when the source cast has no creature target, and the source-level marker must
remain visible to native `descfunc=15`. Vincent explicitly required a generic
concept-level fix rather than a Teleport/Frost Nova special case.

On **25 August 2026**, Vincent required version 0.1.0 to cover D2R 3.2.92777
and 3.3.93847. On **27 August 2026**, he removed the redundant second runtime
matrix: the official current runtime is tested once on 3.3.93847, while
3.2.92777 is covered by the governed byte-exact native equivalence of every
surface used. A separate runtime qualification returns if any surface or
environment differs.

Later that day, Vincent removed runtime version allowlists from the Suite
contract. Build names and versions are diagnostic only; native compatibility
is decided by a complete fail-closed fingerprint before any hook is installed.

On **27 August 2026**, gameplay showed that the path's retained first point can
diverge from the player's visible facing after a Chain Lightning cast. Vincent
required every directional CtC skill, such as Fire Ball, to consume the source
cast's native target without a skill-ID list. First-point, pre-handler direction
and post-handler direction candidates all failed rapid-turn gameplay. The
approved replacement observes the descriptor the successful native source
handler actually consumes: the exact unit returned by its target resolver, the
exact paired first-point coordinates it reads for a ground cast, or no target
when it consumes neither surface.

On **28 August 2026**, broader BKVince gameplay disproved handler-only
observation as a generic contract. War Cry never consumed a target helper and
Taunt intermittently reached the handler without one, so their triggered Fire
Ball could inherit an unrelated direction even though the player input carried
an exact destination. The first replacement also failed because it retained
the descriptor only for the input executor's synchronous call. Runtime proved
that player-mode finalization returns before the central do-handler consumes
the cast. The approved correction keeps the latest authoritative position or
unit identity in a bounded per-player record, associates it with the native
active-skill ID and consumes it only in the matching successful handler. A
channel retains and refreshes the record across later ticks. Handler
observation remains a fallback. The design stays skill-ID-free and does not
read client cursor state.

On **27 August 2026**, Vincent approved channelled sources for version 0.1.0.
The existing `cast-skill` and `cast-skill-same-level` item properties remain the
only data contract: a successful repeating/channelled source dispatches once
immediately, then no more often than the TOML `while_channeling.interval_frames`
cadence. The approved default is 50 authoritative server frames, exactly two
seconds at D2R's 25 frames per second. This is not a wall-clock timer and does
not add a second item property family.

On **27 August 2026**, Vincent also approved the complete combat-trigger gate
for version 0.1.0: successful native Critical Strike, Crushing Blow and Open
Wounds outcomes may dispatch their own configured EventFunc20 item stats.
Critical Strike is strict: weapon-mastery and passive Critical count, while
Deadly Strike does not. Triggered item skills are recursion-guarded so they
cannot start a second Cast Triggers chain.

On **29 August 2026**, Vincent required a distinct plugin-owned Cast on Attack
Attempt family instead of relying on native `att-skill`. It dispatches after a
common authoritative player-skill input executor accepts an attack and before
hit resolution. Direct-unit inputs, Shift-ground inputs and misses are therefore
eligible. The source classifier is generic: native player modes `A1`, `A2`,
throw, kick and `S1` through `S4`, plus `SQ` records transitioning into one of
those families. No source skill ID is embedded. Runtime diagnostics proved
Berserk inputs reached the hook but exposed an incorrect initial enum mapping:
the candidate admitted town modes 5/6, omitted `A1=7`, `A2=8` and kick 12, and
admitted dead mode 17. The corrected policy uses the complete governed player-
attack family and explicitly rejects every non-attack mode. Native `att-skill`
and `hit-skill` remain unmodified and distinct.

The combat item-stat IDs are consumer-owned and declared in TOML. A synthetic
`doactive` dispatch exposes exactly one configured combat family at a time;
ordinary cast-on-cast dispatch suppresses those reserved IDs. This preserves
native chance rolls and item-skill encoding without embedding item mappings or
requiring BKVince, BKVCombat or Melee Splash.

On **30 August 2026**, the full-stack runtime A/B proved that synchronous
diagnostic logging was unsuitable for combat hooks: `diagnostics=true`
produced 122 Cast Triggers log lines in 11 seconds around nine Critical Fire
Ball procs and caused visible frame drops, while `diagnostics=false` removed
the drops with the DLL and plugin stack unchanged. Vincent authorized
`GO CastTriggers diagnostics`. Diagnostic events must now be retained only in
a fixed-capacity in-memory trace and exposed on explicit console request; no
combat hook may call the D2RLoader logger. Existing aggregate counters remain
authoritative, and overflow overwrites the oldest trace sample without
allocation or gameplay mutation.

## Product contract

- DLL: `d2rl-ruffneckk-cast-triggers.dll`
- Config: `ruffneckk-cast-triggers.toml`
- Author: `RuffnecKk`
- Runtime qualification: D2R `3.3.93847`; D2R `3.2.92777` covered by governed
  byte-exact native equivalence
- Runtime gate: no build-name/version allowlist; complete native fingerprint
  must match before any hook
- Scope: global or mod-local, never both at once
- Consumer data: the active mod owns its ItemStatCost, Properties, item rows,
  localization keys and permanent numeric IDs
- Plugin data: no item mapping and no BKVince table dependency
- Combat config: consumer-owned ItemStatCost IDs for Critical Strike, Crushing
  Blow and Open Wounds; `0` disables an unused family
- Saves: native item-stat encoding only; the plugin adds no proprietary save
  payload
- Multiplayer: the authoritative server skill handler dispatches the proc

## Native contract

The implementation targets D2R `3.2.92777` and `3.3.93847` with one DLL and
uses their governed common native corpus.

- `0x43ACB0`: central server skill handler. A successful manual execution uses
  `a5=1`, `a6=0`, `a7=0`.
- `0x44D570`: safe wrapper around the unit-stat event dispatcher. Event `4`
  is `doactive` (unit used a skill).
- `0x583B30`: native EventFunc20 chance-to-cast callback. Cast Triggers filters
  only its own thread-local synthetic `doactive` dispatch so ordinary casts do
  not consume combat-only stats and each combat outcome sees only its matching
  stat ID. Calls outside that scope remain native and unchanged.
- `0x584170` and `0x583150`: native Open Wounds and Crushing Blow callbacks.
  Cast Triggers calls each original callback first and dispatches only when its
  authoritative return reports that the native outcome was applied.
- `0x44C030`: native damage builder. Cast Triggers predicts the exact native
  weapon-mastery/passive Critical RNG path without advancing the seed, calls
  the original builder, and marks the packet only when the native shared
  Critical/Deadly result bit confirms success. The prediction deliberately
  stops before Deadly Strike.
- `0x4494B0` and `0x4496E0`: native D2Damage copy constructor and destructor.
  They propagate and retire the Critical provenance marker across the native
  damage pipeline without changing the 0x180-byte packet or save data.
- `0x44D570`: in addition to synthetic event `4`, the hook observes the
  authoritative offensive damage events `5` and `6`. A marked successful
  packet dispatches Critical exactly once after the native damage event has
  passed target validation.
- `0x5896E0` and `0x589820`: native target and position item-skill casters.
  Cast Triggers owns these two entries and changes its reserved positive level
  marker only while its own `doactive` dispatch is active.
- `0x4FDB40` and `0x4F8DE0`: common authoritative server executors for
  player-skill position and unit input. Cast Triggers owns these entries and
  stores their exact position or unit type/GUID in a bounded `(game, player,
  skill)` record until the matching successful handler consumes it. Unit
  identity is resolved again at consumption through fingerprinted
  `0x48FE80`; that resolver is called, never hooked.
- `0x33DBA0`: exact active-skill layout witness. Its entry proves the
  `D2Skill+0x00 -> SkillsTxt` pointer and the record word at `+0x00` used to
  associate an input with its native skill ID. Cast Triggers reads those two
  fields only after validating the witness.
- `0x48FE20`, `0x341CC0` and `0x341CD0`: native target-unit and lower path
  first-point X/Y helpers. Cast Triggers owns their entries and observes them
  only in thread-local scope while the eligible source handler runs. This
  descriptor is a fallback for channel ticks that execute outside the original
  player-input scope.
  `0x34AE80` supplies the exact source-player DynamicPath used to filter the
  lower X/Y calls; it is fingerprinted and called, never hooked.
  The last native unit resolution is retained; a null resolution followed by
  a complete X/Y pair becomes a position; no observed consumption remains
  self/none. Compiled Skills.txt target flags do not decide this descriptor.
- The synthetic ordinary EventFunc20 self target is replaced by exactly one
  native call when the source consumed a target: unit-target flag 1 for the
  observed unit or position flag 1 for the observed coordinates. A self/none
  descriptor preserves the original target call. Native ItemTgtDo flag 0
  remains unchanged. The plugin neither reconstructs nor writes direction,
  and it does not chain result-driven fallback targets.
- `0x097790`: context-aware SkillsTxt lookup. The compiled record stride is
  `0x2EC`; flags are at `+0x24`, player animation at `+0x30` and sequence
  transition at `+0x32`.
- `0x43ACEC`: unique witness for `game+0x106` and the context-aware SkillsTxt
  call used by the plugin.
- `0x09780B`: unique witness for the compiled SkillsTxt stride `0x2EC`.
- `0x42E615`: read-only layout witness proving that the current authoritative
  server frame is the dword at `Game+0x170`. Cast Triggers reads that frame only
  after a successful repeating/channelled server skill handler call.

The source filter accepts player cast mode `SC` and non-repeating sequence mode
`SQ` transitioning to `SC`. A repeating sequence may transition to `SC` or
remain in `SQ`, which covers Inferno generically without a skill-ID exception.
Non-repeating records use the existing one-dispatch route. Records carrying the
native `repeat` flag use a per-game, per-player and per-source-skill frame
throttle. Weapon attack animations remain excluded from
cast-on-cast. Non-player units, triggered item casts and nested dispatches
remain rejected. A separate thread-local proc-execution guard also suppresses
combat outcomes caused while a triggered item skill is executing.

## Encoding contract

Mod authors add the cast-on-cast ItemStatCost rows using `doactive` and
EventFunc20, plus Properties rows using native property function 11. The fixed property uses
a normal `max` level from 1 through 62. The same-level property reserves
positive `max=63`; Cast Triggers replaces that marker with the successful
source skill's effective level at the final native item-skill caster. The
earlier `max=64` release-candidate contract encoded internal level zero and
could suppress the complete native `descfunc=15` tooltip line, so it is
obsolete and test items created with it must be regenerated.

Attack Attempt, Critical Strike, Crushing Blow and Open Wounds each use another consumer-owned
ItemStatCost row with `doactive` and EventFunc20 plus a fixed-level Properties
row using function 11. Their four numeric stat IDs must match
`[combat_triggers]` in the TOML and must be distinct. Combat same-level markers
are outside version 0.1.0. Every consuming mod permanently freezes its chosen
IDs; the DLL reads them from configuration and never embeds item IDs.

The plugin-owned property is conventionally named `cast-skill-on-attack` and
maps to the consumer-owned `item_skillonattackattempt` stat. It is separate from
vanilla `att-skill` and `hit-skill`, which the plugin forwards unchanged.

## Validation gates

1. Policy and TOML tests pass.
2. Release x64 DLL builds against the governed PluginSDK baseline.
3. No runtime version allowlist exists; all 26 native fingerprint checks,
   15 hook signatures and call ownership are proven in the governed common
   corpus.
4. A disposable public intermod fixture proves fixed level, same source level,
   100% chance, cast-on-cast attack exclusion, custom Cast on Attack Attempt
   semantics, two-second channel cadence, channel stop, current channel target,
   strict Critical (not Deadly), Crushing Blow, Open Wounds, configuration
   filtering and no proc chains. Chance arithmetic below 100% remains covered
   by native encoding and policy tests rather than a duplicate gameplay case.
5. On the official current runtime, a full-stack cold start retains every active compatible
   RuffnecKk Suite component and all five eezstreet plugins/features.
6. On the official current runtime, global and mod-local installation scopes are qualified
   without a duplicate installation.
7. The public ZIP contains only the DLL and TOML; README files remain beside
   the ZIP for Vincent's review.

## Rollback

Remove the DLL and TOML. Items keep their native encoded stats, but the custom
`doactive` event has no plugin-generated dispatch and therefore produces no
cast-on-cast effect. No character or shared-stash migration is required.

## Implementation status — 26 August 2026

The autonomous DLL, strict TOML parser, intermod guide, native fixture builder
and Release tests are implemented. Version 0.1.0 no longer gates on build
names. It logs the observed identity and requires eleven native fingerprint
checks before its three hooks. Separate fixtures generated from the vanilla
3.2 and 3.3 sources each preserve CRLF/row widths and compile 188 tables in the
matching runtime without touching BKVince.

Two independent Release builds produced the same 178,176-byte DLL, SHA-256
`6F676E7A00A909F9861B4CB10931AE4DE09DE0EB7C94E2B45580F58791028708`.
Fresh global and mod-local cold starts accepted the fingerprint on both
official builds. The 93847 installed BKVince stack loads 37 plugins and 18
patches cleanly with Cast Triggers in either scope. The 92777 qualification
stack loads 31 plugins, including Cast Triggers and all five eezstreet plugins,
in either scope, but its pre-existing RogueScoutMovement plugin still reports
its own load failure; that profile therefore cannot provide a clean
complete-Suite claim yet.

The rebuilt `CastTriggers-0.1.0-rc.zip` contains only the byte-identical DLL and
the TOML and has SHA-256
`2C424C356936C71488BD6448B0945895119A0A37F179433BF11E2228512DB60B`.
Steam 3.3.93787 remains provisional until the tester returns fresh logs showing
the observed identity, `native fingerprint accepted`, scope and completed
startup.

Publication also remains blocked by gameplay gates, and the 93847 fixture still
reports D2Prism `Present failed` after startup completes. The generic resolver
now passes policy inspection and focused 93847 gameplay for targetless,
real-target and directional projectile source casts, but the remaining skill
families and matrices are still open. Marker `63` remains runtime-valid for
source-level substitution, but its tooltip line is not yet reliable. Under the
current Suite rule, 3.2.92777 inherits native coverage only where the governed
corpus proves every used surface byte-exact with the same RVA and ABI; no
duplicate gameplay matrix is required solely because its build number differs.

Fresh BKVince 93847 gameplay on **26 August 2026** first proved fixed level-12
Fire Ball, then exposed two superseded routing defects: the original build
required a creature target, and the partial resolver still centered Nova on
that monster. The full result-based resolver was built twice with byte-identical
DLL SHA-256
`980053FC3615F84B04DBD0FFE1A691568AE4A0B6303B92CA3B3D3659EA4F54FE`,
copied to the package and BKVince mod-local runtime, and accepted its native
fingerprint on a fresh 93847 start. Battle Orders (no target) and Taunt (real
monster target) both triggered Fire Ball through position and Nova through the
player/self fallback. Every Nova attempt logged `position-result=0`,
`subject-result=1` and `unit-target-result=-1`, matching the visual result around
the Barbarian. The partial `CD8CFE4D...C9A5` and original
`6F676E7A...028708` DLLs remain in local evidence archives for rollback. The
earlier screenshot also disproved the visible-tooltip assumption: the stat can
execute while its source-level description remains hidden, so tooltip diagnosis
stays open separately.

Fresh BKVince 93847 gameplay on **27 August 2026** exposed and corrected a
third routing defect: `SUNIT_GetTargetUnit` can retain a monster even when the
source skill did not semantically consume a unit target. The resolver now
admits that pointer only when the compiled source Skills.txt flags describe
unit selection or search semantics; it does not hardcode skill IDs. Eight
Battle Orders casts logged `unit-target-semantics=0`, `captured-target=0` and
never attempted Attract's unit fallback. Six directly targeted Taunts logged
`unit-target-semantics=1`, `captured-target=1`; Attract reached the unit route
on every attempt and returned native success whenever its target state allowed
the cast. Policy tests pass and the deployed/package DLL is 178,688 bytes,
SHA-256 `BFCEE9AB5B0480C7D720DAA9026B95C0E7383CF4D46874F72F6EF50AB23D4F66`.
Independent reproducibility and lab redeployment remain open.

Fresh BKVince 93847 gameplay later on **27 August 2026** initially appeared to
validate the replacement directional-projectile route. The Release build, package and
BKVince mod-local runtime used DLL SHA-256
`907FD0B34386C6468FF6FA632C2F3B791B728721C3D4468BA545CCA8934E6DD7`.
Its eleven-witness fingerprint passed before hooks were installed, and the
complete BKVince cold start reached 37 plugins and 18 patches. Eleven level-14
Chain Lightning casts on the Barbarian, aimed across multiple directions, each
triggered level-12 Fire Ball through the position route. Vincent confirmed that
every proc appeared to follow the direction the character faced. The logs
showed varied native first-point coordinates with identical pre- and
post-handler aim on every observed cast.

A later cold start with the exact same DLL disproved that positive conclusion:
Fire Ball became directionally unstable in general, not only after another
proc. The position caster still received the retained point and returned native
success. The first-point candidate is therefore not a reliable facing contract,
and the directional gate is open again. In the same batch, normal attacks and
Bash produced no proc, sustained Inferno produced no proc, and seven Chain
Lightning casts each produced exactly one Nova and one Fire Ball attempt with
no triggered-skill recursion. Attack exclusion, Inferno exclusion, sequence
single-dispatch and proc-chain exclusion are validated on 3.3.93847.

The first setter candidate captured the pre-handler raw D2R direction and
restored it with governed `PATH_SetDirection 0x342860`. The setter operated as
designed, but kiting exposed the input as stale: 16 differently aimed Chain
Lightning casts all logged pre-handler direction `73`, and Fire Ball could
diverge after a rapid turn. The post-handler replacement also logged `73`
across changed aim points and failed gameplay. Both setter candidates are
rejected and `PATH_GetDirection`/`PATH_SetDirection` are no longer in Cast
Triggers' active fingerprint.

Vincent then approved native-target routing. The 14-witness replacement swaps
the two direction surfaces for governed `PATH_GetX/Y`, captures target and aim
before the source handler, verifies target-position equality, and dispatches
one native unit or position route. No Chain Lightning, Fire Ball, Nova or other
skill ID is embedded. Debug and Release CTest pass, and two independent Release
builds produced byte-identical DLL SHA-256
`2B3ED79C80C63D166DC4EAD031F78BF9AA58016A88B78522A4A07AD9B4C63CD0`;
the same DLL is synchronized to the package, governed build cache and BKVince
mod-local runtime. A fresh 3.3.93847 cold start accepted all fourteen witnesses,
loaded 37 plugins and 18 patches, compiled 190 tables and completed startup.
Native-target gameplay remains the next gate.

The first native-target gameplay batch improved Fire Ball direction but still
produced occasional divergence. Fresh diagnostics proved why: every Chain
Lightning source reported `unit-target-semantics=0`, so Skills.txt flags cannot
identify its real creature target, and exact target-position equality was
never satisfied. Vincent approved replacing that heuristic. The current
candidate hooks the already governed target and first-point helpers and records
only calls made while the successful source handler is executing. Debug and
Release CTest pass; three Release outputs are byte-identical at 181,760 bytes,
SHA-256 `0F3A68AD594DD9B8B0FCDEDC237D9FE6E6D7D08006293AF1F7FAF3AC2568FA07`.
Its BKVince gameplay proved the unit-target route improved creature clicks, but
Shift-ground remained fundamentally broken: fresh logs counted 29 unit
descriptors, 34 self/none descriptors and zero positions. The source handler
called the lower `PATH_GetFirstPointX/Y` accessors directly and bypassed the
hooked `UNITS_` wrappers.

The replacement candidate owns those lower accessors at `0x341CC0/0x341CD0`
and filters them against the exact player DynamicPath captured through
`0x34AE80`. It retains the native unit and self routes and does not reconstruct
facing or embed skill IDs. Debug and Release CTest pass; three independent
Release outputs are byte-identical at 182,272 bytes, SHA-256
`AD4FC8BDD3C6C54897A860B812FEF253525916D00B79291EAFF610B7EE2D31F5`.
The same DLL is synchronized to the package and BKVince mod-local runtime. A
fresh single-instance 3.3.93847 cold start accepted all twelve witnesses,
compiled 190 tables and completed startup with 36 plugins and 18 patches;
Revive Overhaul alone reported its independent load failure. Fresh Barbarian
gameplay then recorded 11 native position descriptors, 5 native unit
descriptors and zero self/none fallbacks while each Fire Ball used the matching
single route. Vincent visually confirmed that Shift-ground, direct monster
clicks and rapid direction changes now work perfectly. The generic directional
target-routing gate is closed for the tested Chain Lightning source.

Vincent requested a personal test laboratory so gameplay validation no longer
depends on external testers. Two isolated labs are prepared locally:
`CastTriggersLab92777` and `CastTriggersLab93847`. They use distinct mod names
and save paths. The current 93847 public laboratory contains the reproducible
combat/channel candidate DLL SHA-256
`1E8BFE40D0D398D7D4D6A68C910D119F2501F590EFEF2FD578E6EAE4531723B5`,
diagnostics, nine deterministic cube recipes and a modified Sorceress starter
row that supplies a Horadric Cube and primary inputs. `LAB-GUIDE.md` covers
fixed/source-level cast-on-cast, two-second channeling, native Cast on Attack,
strict Critical versus Deadly, Crushing Blow, Open Wounds, combat-family
filtering and proc-chain exclusion. No custom recipe consumes a Town Portal
Scroll and there is no duplicate 25% gameplay case.

The current 93847 generated tables passed static byte-exact TSV, CRLF,
row-width, ID, recipe, localization, modinfo and hash checks and were copied to
the installed isolated profile. At Vincent's explicit request, the launcher,
D2RLoader and D2R were not started. Cold start, table compilation, character
creation and the complete combat/channel gameplay matrix remain `not run`
until separate authorization. The governed 3.2 build inherits native coverage
without a duplicate runtime matrix while all used surfaces remain byte-exact.

Fresh BKVince 93847 gameplay on **28 August 2026** rejected the first
input-executor candidate, SHA-256
`1FD2520B3A497CEFE19FAD56825C279618277481CE5EA0477FB0ED9D4B0D036D`.
Across 64 eligible casts, diagnostics recorded zero
`descriptor-source=input`: all 64 handlers executed after the temporary input
scope ended. War Cry produced 17 `target=none` descriptors. Taunt produced 22
unit descriptors on direct monster clicks and six `target=none` descriptors
for the failing Shift cases. The same fixture's Inferno produced zero eligible
channel diagnostics and no Fire Ball. Static table evidence explained the
second failure independently: Inferno is `anim=SQ`, `seqtrans=SQ`, `repeat=1`,
while the rejected classifier admitted only `SC` or `SQ -> SC`.

Vincent authorized the combined correction later that day. The source now
keeps the authoritative input beyond player-mode finalization in a bounded
per-player record, resolves stored unit type/GUID afresh at consumption,
promotes a matching channel descriptor across ticks and expires an unconsumed
ordinary input after 250 server frames. The generic channel classifier accepts
repeating `SQ -> SQ` without naming Inferno. A strict 24-byte witness at
`0x33DBA0` guards the active-skill layout used for the skill association. Debug
and Release policy tests pass. Three independent Release builds are byte-exact
at 219,648 bytes, SHA-256
`AAFA26D9910D524DA387D1E0FC02A8BBB7A6840AB536078D14F93F6B1BCD6819`.
The same DLL is deployed to the package and BKVince mod-local runtime. A fresh
full-stack 3.3.93847 cold start accepted the 27-witness fingerprint, loaded Cast
Triggers 0.1.0 and completed all 24 startup stages with all five eezstreet
plugins active. The consolidated gameplay matrix is still pending.

Vincent then passed the authoritative targeting and channeling gameplay gate on
QtyTester. War Cry and Taunt preserved direct-unit and Shift-click ground
targets across direction changes. Inferno dispatched Fire Ball immediately and
then every 50 server frames (two seconds) while held. Fresh diagnostics confirm
ordinary position/unit descriptors were consumed from persisted input and show
Inferno position dispatches at frames 3695, 3745, 3795, 3845 and 3895. Combat
trigger families and proc-chain exclusion remain the next gameplay gate.

On **29 August 2026**, the combat-ring tooltip gate exposed a laboratory data
deployment defect rather than a DLL defect. The loaded
`item-modifiers.json` contained only cast-on-cast, same-level and Attack Attempt;
Critical Strike, Crushing Blow and Open Wounds had been written to an arbitrary
JSON filename that D2R does not mount as a string table, producing `An Evil
Force`. The fixture builder now performs an idempotent merge of all six
canonical entries into the recognized `item-modifiers.json`, accepts exact
pre-existing entries, rejects key/ID/content collisions and proves one complete
entry per trigger. The corrected BKVince table contains six unique keys and six
unique IDs; a fresh full-stack cold start loaded Cast Triggers 0.1.0, all five
eezstreet plugins, 36 plugins total and all 24 startup stages. Visual tooltip
confirmation and the remaining combat/proc-chain gameplay gate are pending.

## Release 0.1.0 prepared — 30 August 2026

Vincent completed the consolidated BKVince/QtyTester gameplay matrix on the
official D2R 3.3.93847 runtime. Fixed-level and source-level cast-on-cast,
direct-unit and Shift-ground routing, rapid direction changes, Inferno's
immediate plus 50-frame cadence, channel stop, sequence single-dispatch, generic
Cast on Attack Attempt, Critical Strike, Crushing Blow and Open Wounds all
passed. Deadly Strike initially exposed a stale Critical marker on reused
`D2Damage` storage. The final lifecycle resets prior provenance before each new
player damage build while preserving every copy made during that logical hit;
the focused same-session Critical-to-Deadly retest produced no false Fire Ball.

Combat-family filtering then passed with Critical Fire Ball active and the
unmatched Crushing Blow Nova inactive. Equipping the Mana Potion and Antidote
rings together produced no second cast-on-cast Fire Ball, closing the proc-chain
gate. The split `cast-triggers` status output confirmed `combat stats
filtered=5`, a complete Critical lifecycle (`created=1`, `propagated=3`,
`consumed=1`, `removed=1`), `event flags without marker=0` and `critical marker
overflows=0`.

Synchronous hot-hook diagnostics were replaced by a bounded 64-entry in-memory
trace emitted only on explicit console request. The runtime reported deferred
combat logging, and the public TOML defaults diagnostics to disabled. The README
is standalone and modder-facing: it documents all six ItemStatCost rows, all six
Properties rows, the recognized `item-modifiers.json` path, complete tooltip
keys, collision handling, `%d%% ... %.*s`, the `max=63` source-level marker and
configuration examples. D2MOO is credited explicitly.

Two independent MSVC Debug/Release build trees passed CTest `1/1` and produced
the same 227,328-byte Release DLL:

- DLL SHA-256:
  `495C8AFED5F2A613F080A9B8ECF5009819FF556CC4090D7832114C532203C5ED`;
- public TOML SHA-256:
  `18AE9459DA72730CB43B1A4154351D6296D2D118EC770217B169EBFF12888531`;
- adjacent README SHA-256:
  `03F05C897301DD075B9DA00D182DA70616B2624CBE812807AEC575D35744FE4D`.

The exact DLL was synchronized to the package and BKVince mod-local runtime. A
fresh full-stack cold start accepted the complete 27-witness fingerprint,
loaded 36 plugins and 18 memory patches, kept all five eezstreet plugins active
and completed all 24 startup stages. Revive Overhaul alone retained its known,
independent load failure.

The local agent-generated `CastTriggers-0.1.0.zip` is 102,560 bytes, SHA-256
`C91D58DB4A1BB55D9FDCFFE23827ED0EFCE4C691107AA4B6901B631F4EE34140`.
Its inspected root allowlist contains exactly:

```text
d2rl-ruffneckk-cast-triggers.dll
ruffneckk-cast-triggers.toml
```

Both extracted entries hash-match the tested package. `README.md` is beside the
ZIP for Vincent's required human review and manual insertion into the final
public archive. Commit, push, GitHub Release publication and any Steam 93787
compatibility claim remain unexecuted; Steam requires its own complete evidence
before being named as qualified.
