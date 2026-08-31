# Revive Overhaul 2.1.1 — BKVince / D2R 3.3.93847

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
