# Minimal offline smoke test

Run only on D2R 3.2.92777 in offline/local single-player. Keep the complete
installed plugin and PluginPack feature stack active. Enable diagnostic logging
for the shortest necessary session, then disable it again.

1. Record hashes for the DLL, JSON, active mod tables, PluginPack manifest, all
   loaded plugins, and active patches. Confirm `MeleeSplash` reports active and
   zero signature/config failures.
2. **A — baseline:** use an allowed normal attack or whitelisted melee skill
   with no radius/damage bonus. Confirm one primary hit and nearby secondary
   hits at the configured base radius.
3. **B — exclusion:** add that skill to `excludedSkillIds`, cold restart, and
   confirm the primary remains normal with no secondary splash.
4. **C — radius:** provide `+20%`, then `+40%` through the configured radius
   stat. Confirm exactly +1, then +2 tiles in diagnostics and boundary targets.
5. **D — damage:** provide `+50%` through the configured damage stat. Confirm
   the shared pre-defense packet scale is 150%, while each target still resolves
   its own defenses.
6. **E — chance effects:** equip the 100% Deadly Strike/Open Wounds/Crushing
   Blow witness. Confirm each secondary logs its own Critical/Deadly sequence,
   CB roll, and OW roll; do not infer shared outcomes from matching results.
7. **F — primary:** confirm the primary GUID appears once and its HP is changed
   only by the original hit.
8. **G — legacy:** with the host's exact reversible suppression token enabled,
   confirm no old splash missile/Next Hit Delay path is created. Disable/remove
   the plugin and cold restart to confirm the historical path returns.
9. **H — recursion:** cluster several monsters and confirm synthetic hits never
   become new splash centers; recursion counter/rejection log must remain clean.
10. Save and exit, cold restart once more, verify the complete stack and fresh
    logs, then restore the default-off configuration and any test items/tables.

Any crash, byte mismatch, duplicate primary, nested splash, unexpected CTC,
missing full-stack component, or ambiguous target identity fails the smoke test.
Multiplayer and PvP are intentionally not part of this protocol.
