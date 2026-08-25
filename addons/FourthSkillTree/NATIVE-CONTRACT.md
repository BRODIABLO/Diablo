# Fourth Skill Tree Framework native contract

Target runtimes: D2R 3.2.92777 and D2R 3.3.93847. The governed analysis corpus
retains its historical `d2r-3.2.92777` path because the useful native image is
shared by both supported builds.

## Milestone 0.1.0

The DLL is a fail-closed data-contract validator. It installs no hook, calls no
unproven native ABI and writes no save data. The only runtime mutation is the
optional registration of the `fourth-skill-tree` D2RLoader console command.

## Proven paths reserved for later milestones

- `DATATBLS_GetClassSkillCount 0x33CB30` returns the compiled class-skill count.
- `DATATBLS_GetClassSkillIdByIndex 0x33DDE0` resolves a compiled list entry.
- The native D2S header and skill-section writer use those dynamic values; the
  reader consumes the saved byte count and advances by `2 + count`.
- Allocation at `0x14C69D6` sends the native skill-ID command path.
- Page state currently clamps to `0..3` at `0x14C3B10`; navigation repeats that
  bound at `0x14C6BBC..0x14C6BFF`.

No later milestone may install a UI patch until every signature, ABI,
continuation and owner is promoted in the governed RVA registry and a 31-skill
fixture has passed compilation, save/reload and respec checks.

## Hook ownership exclusion

FourthSkillTree does not hook `UI_DispatchMessage 0x843D90`. That site already
has a governed broker shared by RemoteStash and the Bulk Skill Point Allocation
feature inside `plugin-skills.dll`. Fourth-page allocation remains skill-ID
based and does not require a second dispatcher owner.
