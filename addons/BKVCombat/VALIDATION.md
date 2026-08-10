# BKVCombat validation ledger

Status values are `PASS`, `FAIL`, `BLOCKED`, and `NOT RUN`. Static proof never
stands in for runtime gameplay evidence.

## Build and static gates

| Gate | Status | Evidence |
|---|---|---|
| Configuration parser and policy tests | PASS | Release MSVC `/W4 /WX`; CTest covers schema 2, CBE configuration/math, MajorBoss validation, class priority, Herald/Ascendant Elite, fractions, and atomic activation planning. |
| D2R 3.2.92777 workbench | PASS | Canonical and analysis images verified by the governed persistent workbench. |
| Native contract | PASS | See `NATIVE-CONTRACT.md`; each Release 1 seam has a proved RVA, context, ABI, and owner disposition. |
| Critical/Deadly implementation | PASS | Fresh Release DLL built with `/W4 /WX`; runtime branch/RNG witnesses remain `NOT RUN`. |
| Crushing Blow implementation | PASS | Fresh Release DLL and checked-rational CBE/player-count tests pass; gameplay class/ranged/resistance witnesses remain `NOT RUN`. |
| Life/Mana steal baseline | PASS | Native call, consumer, Life Tap, and active difficulty divisors are validated; no leech hook is installed. Runtime credit/Drain witnesses remain `NOT RUN`. |
| Open Wounds three-stack implementation | PASS | Internal call owner, stat-list construction, aggregate tick, callback teardown, pet/merc quartering, and cap refresh are statically closed and compile cleanly. Runtime lifecycle witnesses remain `NOT RUN`. |
| Fresh native artifact | PASS | `BKVCombat.dll` SHA-256 `3EFCEB7374E26207FE603FF5AC43DAFBC8246E85C37426B62D0AEF1F38663D50`; CTest 1/1. |
| Public package | PASS | `BKVCombat-0.1.0.zip` SHA-256 `A6E89B7B4B8723704A44F95386AB841A6ABD4AD9C2C27003EE61A4B90331BE24`; root entries are exactly the DLL and default-off JSON. |
| PE/API metadata | PASS | x64 PE32+, D2RLoader API v2 manifest, version 0.1.0, RuffnecKk metadata, four exports including the optional combat API, and Windows/MSVC/UCRT imports only. |

## Coexistence gates

| Gate | Status | Required evidence |
|---|---|---|
| Static hook-owner matrix | PASS | No exact overlap with MeleeSplash entries, the five PluginPack manifests, FloatingDamage, BurnFireResistance, or active BKVince patches. |
| Current installed stack, BKVCombat before MeleeSplash | PASS | Six policies active; 19/19 plugins, 15/15 patchsets, zero disabled/rejected/failed, and frontend 24/24. |
| Current installed stack, MeleeSplash before BKVCombat | PASS TECHNICAL | Exact module names and six policies active; 19/19, 15/15 and 24/24. The lazy provider negotiation remains `NOT RUN` until the first hit. |
| Global scope | PASS TECHNICAL | BKVCombat loaded globally after mod-local MeleeSplash and reached 24/24. A later pre-existing render assertion prevents a broader stability claim. |
| Mod-local scope | PASS | Final governed DLL/config loaded from BKVince with source/runtime hashes equal. |
| Cold rollback | PASS | Final default-off cold start logged no BKVCombat hooks and reached 24/24; zero D2R process remains. |
| Every-feature PluginPack matrix | FAIL / BLOCKED OUTSIDE BKVCOMBAT | All five owner DLLs load, but the active baseline leaves multiple PluginPack features disabled. Historical `dxgi/plugin-items` and `PopcornUber` render failures also remain reproducible. No component was disabled for this test. |

## Gameplay matrix

The minimum Release 1 solo matrix is:

1. Critical success, Critical failure then Deadly success, and both failure;
2. 75/76/100 chance bounds and Critical 2.0x versus Deadly 1.5x;
3. Ordinary, Herald/Ascendant Elite, champion/unique/superunique/boss Elite,
   PrimeEvil, and MajorBoss CB targets;
4. melee/ranged fractions, p1 and scaled player-count states, current HP, 0/50/
   100 physical resistance, no flat-DR application, and CBE 0/50/100 from
   global and active-weapon sources without changing proc chance;
5. Life Tap plus ordinary life steal without double credit;
6. life and mana steal across Normal/Nightmare/Hell Drain divisors;
7. Open Wounds 1/2/3/4 applications, five-second expiry, three-stack aggregate,
   cap refresh of all three, physical resistance, mercenary/pet quartering,
   death/despawn, end-game, and GUID reuse;
8. MeleeSplash synthetic targets using the same Critical/Deadly provider and
   exactly one native CB/OW roll per target;
9. no regression to the primary target, RNG order, FloatingDamage observation,
   PluginPack player-count cap, or BKVince kill attribution.

Multi-attacker Open Wounds, multiplayer, PvP, release ZIP, and universal
compatibility remain outside the current proof until their corresponding rows
move from `NOT RUN` to `PASS`.

## Runtime evidence

Evidence is preserved under
`analysis-cache/runtime-validation/bkvcombat-20260810/`. The final runtime is
mod-local and default-off, with no global alias and no running D2R process.
Source/runtime hashes are byte-identical:

- BKVCombat DLL: `3EFCEB7374E26207FE603FF5AC43DAFBC8246E85C37426B62D0AEF1F38663D50`;
- MeleeSplash bridge DLL: `77D3A7DC6C77B319A8E80C49F76DA84707F93DDFEBBC9E18F5E5D83CCE25F2DF`;
- BKVince config: `82C4C703B59CC8928A7650A316EF4CC0E9DFE4689A44C6F6CF2D81801A0128BC`;
- ItemStatCost: `27EE4B1BE6BABA4FB0C0E4972B59337F5D8B649C8CC6B09939E6E3D7C8CD7C22`;
- Properties: `EDB9DA9B9E9E04D1A44152F69BBBF4DF05CF2AEFAD4444409575EFDD86A598E1`;
- item-modifiers: `FFBB86151F6BD447AB94CCB6C3B8FAD6B28204AAE554685D0BD38417D7709787`.

The first enabled-all start hit a `dxgi/plugin-items` access violation whose
signature matches reports predating BKVCombat. The exact reverse-order start
reached 24/24, then hit the existing `PopcornUber` render assertion. These
incidents are retained as visible full-stack blockers and are not attributed
to combat hooks without evidence.
