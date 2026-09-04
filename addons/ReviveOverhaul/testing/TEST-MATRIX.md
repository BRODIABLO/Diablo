# Revive Overhaul 2.3.0 + Scripted AI 0.7.0 — BKVince / D2R 3.3.93847

Initial state: `NOT_RUN`.

## Phase A — main qualification

| ID | Case | Monster Spawner preparation | Expected result | Result |
| --- | --- | --- | --- | --- |
| A0 | Full-stack cold start | Disable nothing | The plugin accepts its complete fingerprint and reports no hook collision | NOT RUN |
| A1 | Normal monster | Spawn, kill and Revive one normal monster | Corpse selection, consumption and Revive match the baseline | NOT RUN |
| A2 | Champion | Spawn a Champion pack and kill the Champion | The Champion corpse highlights and revives; no wrong corpse is consumed | NOT RUN |
| A3 | Unique without aura | Spawn a Unique with Multishot, Cold Enchanted or Lightning Enchanted | The corpse revives and its visible native modifiers remain | NOT RUN |
| A4 | Aura Enchanted Unique | Spawn an Aura Enchanted Unique and record its aura before death | The exact same aura is active after Revive, without rerolling | NOT RUN |
| A5 | SuperUnique | Spawn a non-scripted SuperUnique with a valid corpse | It revives only when all retained native protections allow it | NOT RUN |
| A6 | Owner scatter | Stand close and move around the Revive | The pet no longer walks away because of owner scatter | NOT RUN |
| A7 | Catch-up | Move beyond 12 units | Native catch-up activates, then settles near follow distance 8 | NOT RUN |
| A8 | Stability | Change area, use a portal and fight several groups | No crash, stuck pet, invalid corpse or corrupted ownership | NOT RUN |
| A9 | Diagnostics | Run `revive-overhaul` before and after | Counters match the performed cases and no fingerprint failed | NOT RUN |
| A10 | Scripted domain disabled | Keep the complete stack and disable only `[domains.revive]` through its supported configuration | Revive Overhaul loads and native Revive behavior remains unchanged | NOT RUN |
| A11 | Peaceful follow baseline | Enable Scripted AI and `[domains.revive]`, keep five Revives without a target, then move through doors and narrow paths | Revive Overhaul/native AI owns the tick; pets close distance and native catch-up recovers obstacles without a Scripted target action | NOT RUN |
| A12 | Melee tactical profile | Revive a melee monster, expose one enemy at a time, then move while the fight continues | It closes to melee range, keeps the same live target inside the owner combat radius and does not break off merely to hug the owner | NOT RUN |
| A13 | Ranged physical tactical profile | Revive an archer or other missile attacker, then let a melee enemy approach it | It attacks at range, retreats after close attacks, reacquires its locked live target and does not wander while the target remains valid | NOT RUN |
| A14 | Caster tactical profile | Revive a monster with two or more valid cast-mode MonStats skills and fight several durable enemies | It rotates valid native spell slots, kites at close range and stays within the owner-centered combat radius | NOT RUN |
| A15 | Hard leash and invalid target | Pull the owner beyond the tactical radius, kill the locked target, then repeat with a disappearing target across an area edge | Tactical handling delegates immediately; native follow resumes and no stale target is attacked | NOT RUN |
| A16 | ABI load orders and lifecycle | Exercise both relevant plugin load orders and a leave/rejoin cycle with the complete stack | Discovery is safe, the active session and tactical blackboard are renewed, and absence/incompatibility delegates to native AI | NOT RUN |
| A17 | Town/combat transitions | With ProcDump hang capture armed, cross town-to-field and field-to-town boundaries repeatedly with five Revives before, during and after combat | No hang or crash, no stale-target action, every transition remains responsive and no dump is emitted | NOT RUN |
| A18 | Tactical diagnostics | Run `revive-overhaul` before and after A12-A17 and retain the relevant Scripted AI log lines | Callback route applications, tactical calls and handled counts match observed actions; failures stay zero | NOT RUN |

For A4, retain at least one capture before death and one after Revive. Record
the monster name, rank, modifiers and observed aura.

## Phase B — Act bosses after Phase A passes

Do not start this phase during the first session. It needs a separate decision
after A0–A9 are conclusive.

| ID | Case | Conservative expectation | Result |
| --- | --- | --- | --- |
| B1 | Andariel | Clean native acceptance or rejection; never a crash | DEFERRED |
| B2 | Duriel | Clean native acceptance or rejection; never a crash | DEFERRED |
| B3 | Mephisto | Clean native acceptance or rejection; never a crash | DEFERRED |
| B4 | Diablo | Clean native acceptance or rejection; never a crash | DEFERRED |
| B5 | Baal | Clean native acceptance or rejection; never a crash | DEFERRED |

Native rejection of an Act boss is not automatically a plugin defect. Boss,
prime-evil, scripted-unit, corpse, mode, state and `SwitchAI` protections stay
enabled.
