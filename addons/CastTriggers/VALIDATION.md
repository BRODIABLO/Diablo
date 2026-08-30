# Cast Triggers 0.1.0 validation

Status as of **30 August 2026**: **0.1.0 release package prepared locally; not
yet published**.
The channeling, combat-trigger and persistent authoritative input-routing
implementation is built and documented. Vincent authorized deployment to the
full BKVince/QtyTester stack. The fresh full-stack cold start passed; gameplay
for targeting, channeling, Attack Attempt, Crushing Blow and Open Wounds passed.
The Critical marker-loss cause is proven, fixed and passed in focused gameplay.
The Deadly Strike negative gate exposed stale Critical provenance on reused
`D2Damage` storage; the lifecycle reset is implemented and passed its focused
gameplay retest. Combat-family filtering and proc-chain exclusion also passed
their focused gameplay gate. The split runtime counter export is now fully
visible and passed with zero Critical-marker overflow.

## Current candidate

| Gate | Status | Evidence |
|---|---|---|
| Version and author | passed | `0.1.0`, author `RuffnecKk` |
| Scope | passed statically | Global or mod-local; no `ModScopedOnly` |
| Version allowlist | passed | Build name/version are diagnostic only; no allowlist exists |
| TOML parser | passed | Strict top-level, channel interval and distinct combat-stat ID validation |
| Public TOML collision safety | passed | Combat IDs default to `0/0/0/0`; a consuming mod must opt in with its own IDs |
| Tooltip localization | passed statically, at cold start and through focused item inspection | The fixture idempotently merges all six canonical entries into the loaded `item-modifiers.json`, rejects key/ID/content collisions and verifies one complete entry per trigger; the README gives modders the exact recognized path, six JSON entries, collision rules and same-level formatter requirements |
| Channel cadence | passed in gameplay | Inferno dispatches immediately and then every 50 authoritative server frames while held |
| Cast-on-cast attack exclusion | passed statically; historical gameplay pass | Weapon animations are not eligible source casts |
| Custom Cast on Attack Attempt | passed in gameplay | Accepted attack-family inputs dispatch before hit resolution; direct targets, misses and Shift-ground attempts work without a skill-ID allowlist |
| Critical provenance | passed in gameplay, including Deadly exclusion | Weapon-mastery/passive Critical RNG is predicted without mutating the seed and confirmed by the native result. A new logical hit retires any marker on reused `D2Damage` storage before prediction, while copies made during that hit still preserve provenance. A 100% Deadly Strike ring produced no Critical Fire Ball after a positive Critical proc in the same session |
| Diagnostic performance | synchronous cause proven and removed; buffered export runtime-confirmed | The earlier synchronous trace produced 122 log lines in 11 seconds around 9 Critical Fire Ball procs and caused visible frame drops. The current candidate replaces hot-path writes with a bounded 64-entry in-memory trace; the runtime command reported `retained=64/64`, `total=702` and `combat-hook logging=deferred`. The public TOML still defaults diagnostics to false |
| Crushing Blow observation | passed in gameplay | EventFunc16 original return is authoritative |
| Open Wounds observation | passed in gameplay | EventFunc15 original return is authoritative |
| Combat stat filtering | passed by policy tests and gameplay | With the Mana Potion ring and no Crushing Blow source, Critical launched Fire Ball while the unmatched Crushing Blow Nova remained inactive |
| Proc-chain guard | passed statically and in gameplay | With the Mana Potion and Antidote rings equipped together, the Critical Fire Ball did not feed the cast-on-cast family or launch a second Fire Ball |
| Target routing | passed in gameplay | War Cry and Taunt preserve direct-unit and Shift-click ground targets across direction changes; Inferno channel ticks retain current input |
| Console diagnostics | passed at runtime | The three status lines remain fully visible. The final capture reported Critical `created=1`, `propagated=3`, `consumed=1`, `removed=1`, `event flags without marker=0` and `critical marker overflows=0` |
| Debug build/test | passed | CTest 1/1 |
| Release build/test | passed | CTest 1/1 |
| Reproducible DLL | passed | Two independent Debug/Release build trees passed CTest 1/1 and produced byte-identical Release DLLs, SHA-256 `495C8AFE...3C5ED` |
| Full BKVince 3.3 lab | final packaged DLL cold start passed | The packaged/runtime DLL hash matched the independent rebuild, its fingerprint passed, all 24 startup stages completed, 36 plugins loaded and all five eezstreet plugins remained active. Its config-only embedded comment delta does not alter gameplay hooks. The known Revive Overhaul failure remains unrelated |
| Public ZIP | prepared locally; human README insertion and publication pending | `CastTriggers-0.1.0.zip` contains exactly the DLL and TOML at its root. Both extracted entries hash-match the validated package. README is beside the ZIP for Vincent's required review |

## Native fingerprint

All 27 exact witnesses are validated before the first hook is installed. A
mismatch refuses loading cleanly. The governed common corpus proves the same
RVA, bytes and ABI for D2R 3.2.92777 and 3.3.93847; only 3.3.93847 receives the
current runtime matrix unless a surface or environment differs.

| Surface | RVA | Use |
|---|---:|---|
| Central server skill handler | `0x43ACB0` | Hook manual cast completion |
| Skill-handler context witness | `0x43ACEC` | Prove `Game+0x106` access |
| Server-frame witness | `0x42E615` | Prove `Game+0x170` frame |
| Unit-stat event wrapper | `0x44D570` | Hook damage events; dispatch synthetic `doactive` |
| Player position-input executor | `0x4FDB40` | Persist exact ground/Shift input before player-mode finalization |
| Player unit-input executor | `0x4F8DE0` | Persist exact target type/GUID before player-mode finalization |
| Active-skill layout witness | `0x33DBA0` | Prove `D2Skill+0x00 -> SkillsTxt` and the compiled skill ID association |
| Server unit resolver | `0x48FE80` | Resolve persisted type/GUID to a fresh native unit at handler consumption |
| Target resolver | `0x48FE20` | Observe native unit targets |
| Unit type helper | `0x34B9D0` | Restrict source actors to players |
| Dynamic path helper | `0x34AE80` | Filter ground-target observations |
| First-point X/Y | `0x341CC0`, `0x341CD0` | Observe native ground target |
| SkillsTxt lookup | `0x097790` | Classify cast/repeat animation |
| SkillsTxt stride witness | `0x09780B` | Prove compiled stride `0x2EC` |
| Item-skill casters | `0x5896E0`, `0x589820` | Same-level substitution, target routing and chain guard |
| Damage builder | `0x44C030` | Capture strict Critical provenance |
| Damage copy/move/destructor | `0x4494B0`, `0x449760`, `0x4496E0` | Propagate, transfer and retire Critical markers |
| Open Wounds callback | `0x584170` | Observe successful native Open Wounds |
| Crushing Blow callback | `0x583150` | Observe successful native Crushing Blow |
| EventFunc20 | `0x583B30` | Filter synthetic stat families only |
| Active weapon resolver | `0x4242B0` | Mirror mastery-Critical prerequisites |
| Mastery Critical helper | `0x33D4F0` | Read native Critical chance |
| Unit stat getter | `0x2F5020` | Read passive Critical stat 337 |
| Unit seed accessor | `0x34A1E0` | Predict the native roll without advancing it |

The five pinned eezstreet plugins do not own EventFunc15, EventFunc16,
EventFunc20, the damage builder/copy/move/destructor or the Critical helper surfaces.
Their item-skill patches remain inside the caster bodies, after Cast Triggers'
entry hooks. The current candidate passed a fresh full-stack cold start; another
is required only if a remaining gate changes the DLL before release. The public
plugin has no BKVince, BKVCombat or Melee Splash dependency.

## BKVince laboratory

Gameplay qualification is performed in the full active BKVince stack with
QtyTester, not in an isolated mod. The deterministic fixture provides nine
recipes:

1. fixed cast-on-cast Fire Ball;
2. same-level Nova from a Stamina Potion;
3. Inferno/Chain Lightning channel and cast-chain gate;
4. custom Cast on Attack Attempt;
5. positive Critical Strike;
6. Deadly Strike exclusion;
7. Crushing Blow;
8. Open Wounds;
9. combat-family filtering plus proc-chain exclusion.

No custom recipe consumes a Town Portal Scroll, and no separate 25% gameplay
case exists. All gameplay gates use 100% for deterministic observation. The
generated ItemStatCost, Properties, CubeMain and CharStats tables passed
byte-exact parser round-trip, CRLF and row-width checks before deployment. The
source tables and recipes are already present in BKVince. The current DLL
deployment is authorized and its installed hash must match the reproducible
artifact before launch.

The full BKVince laboratory completed a fresh 3.3.93847 cold start. Fingerprint
acceptance, hook installation, TXT compilation and startup passed. Vincent also
passed War Cry/Taunt direct and Shift-click routing plus Inferno immediate and
50-frame channel cadence. Diagnostics show Inferno position dispatches at frames
3695, 3745, 3795, 3845 and 3895. The focused Critical retest proved each native
outcome traversed marker creation, deep copy, move into the combat record, final
deep copy, event consumption and `critical-strike` dispatch. The Deadly Strike
negative gate then proved that D2R can reuse the original damage-builder address:
four created markers yielded 48 Critical dispatches from a recurring address.
The new lifecycle reset removes that stale marker before each new player damage
build. Vincent then confirmed in the same runtime that the 100% Deadly Strike
ring no longer launches Fire Ball. Combat-family filtering and proc-chain
exclusion subsequently passed. The final status-only candidate completed a full
cold start and displayed every counter without clipping; the capture ended with
`event flags without marker=0` and `critical marker overflows=0`. No gameplay
gate remains open for this DLL candidate.

## Artifacts

| File | Bytes | SHA-256 |
|---|---:|---|
| `d2rl-ruffneckk-cast-triggers.dll` | 227328 | `495C8AFED5F2A613F080A9B8ECF5009819FF556CC4090D7832114C532203C5ED` |
| `ruffneckk-cast-triggers.toml` | 1462 | `18AE9459DA72730CB43B1A4154351D6296D2D118EC770217B169EBFF12888531` |
| `CastTriggers-0.1.0.zip` | 102560 | `C91D58DB4A1BB55D9FDCFFE23827ED0EFCE4C691107AA4B6901B631F4EE34140` |
| adjacent `README.md` | 19571 | `03F05C897301DD075B9DA00D182DA70616B2624CBE812807AEC575D35744FE4D` |
| previous `CastTriggers-0.1.0-rc.zip` (superseded) | 96317 | `95B012496F74C8EE7C516AFC1E19B1A5C6A9F01A2360BFA99F8A7500D32AF00C` |

The superseded ZIP contains exactly:

```text
d2rl-ruffneckk-cast-triggers.dll
ruffneckk-cast-triggers.toml
```

README, intermod guide, lab guide and validation stay beside the archive for
human review.

## Runtime gate before publication

1. Explicitly authorize deployment and launch in BKVince with QtyTester.
2. Confirm all 27 fingerprints, 16 hooks, TXT compilation and startup from
   fresh logs.
3. Execute the deterministic matrix in `LAB-GUIDE.md`.
4. Inspect `cast-triggers` counters for channel cadence, all combat outcomes,
   family filtering, chain suppression and zero Critical-marker overflow.
5. Run the public full-stack coexistence cold start with all five eezstreet
   plugins active.
6. Run the relevant multiplayer host/client coverage because the hooks are
   server-authoritative.
7. Review the README beside the release-candidate ZIP before publication.

## Rollback

Remove the DLL and TOML or restore the previous candidate. Items retain their
native encoded stats but custom `doactive` triggers become inert. The plugin
adds no proprietary character or stash payload and requires no save migration.
