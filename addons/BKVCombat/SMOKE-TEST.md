# BKVCombat 0.1 solo smoke test

Run only on D2R `3.2.92777` in offline/local play. Keep the complete installed
plugin stack and every usable PluginPack feature active. Preserve the original
configuration, save family, and logs before starting; every configuration
change requires a cold restart.

## Technical baseline

1. Start once with the shipped default-off configuration and require the log
   `loaded disabled; no hooks installed`.
2. Enable all six policies in the mod-local configuration, keep
   `diagnosticLogging=true`, and cold-start again.
3. Require zero failed/rejected plugin, every BKVCombat signature accepted,
   the activation summary, and the complete normal frontend startup.

## Gameplay witnesses

Use one controlled player attacker and high-HP monster targets. Record target
type/GUID, current/max HP, physical resistance, melee/ranged source, player
count scaling, and the server combat log around each isolated hit.

1. **Critical/Deadly:** witness Critical success, Critical failure followed by
   Deadly success, and both failure. Check the 75% caps and physical multipliers
   2.0x versus 1.5x without changing the native lazy RNG order.
2. **Crushing Blow classes:** hit Ordinary, Elite (including the current
   Herald/Ascendant marker), PrimeEvil, and configured MajorBoss targets. Check
   melee fractions `1/6, 1/8, 1/16, 1/20` and ranged fractions
   `1/9, 1/12, 1/24, 1/30` from current HP.
3. **Player count and resistance:** repeat a fixed CB target at p1 and a scaled
   state, then at physical resistance 0/50/100. Confirm that flat physical DR
   does not enter the CB amount.
4. **Crushing Blow Efficiency:** witness CBE 0, +50, and +100 from an
   attacker-global item and from the active weapon. Check that both sources are
   added exactly once, inactive/offhand sources are excluded, and proc chance
   is unchanged.
5. **Open Wounds:** apply one, two, three, then a fourth successful proc. Check
   the three additive rates, 125-frame duration, collective refresh at cap,
   final state removal, physical-resistance ordering, and quarter damage to a
   mercenary and an owned pet. Multi-attacker OW is outside this version.
6. **Life/Mana Steal:** check the native Normal/Nightmare/Hell divisors
   `1/1, 1/2, 1/3`, target Drain, and caps. Apply Life Tap separately and verify
   that it credits once and is not doubled by ordinary life steal.
7. **MeleeSplash coexistence:** keep MeleeSplash active. Require its synthetic
   targets to report the BKVCombat Critical/Deadly resolver and exactly one
   native CB/OW roll per secondary target.

## Rollback

Exit the game, restore the byte-exact default-off configuration, and cold-start
once more. Open Wounds callbacks make hot unload unsupported: never remove the
DLL while a game process or managed stat list can still exist.

Gameplay, multiplayer, PvP, and universal compatibility remain unproved until
their evidence is added to `VALIDATION.md`.
