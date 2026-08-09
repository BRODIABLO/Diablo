# Changelog

## 0.1.0 — 2026-08-09

- Replaced the quarantined proof-of-concept with a generic, default-off,
  standalone D2RLoader plugin.
- Added strict JSON configuration, activation policies, skill overrides,
  optional host stat IDs, diagnostics, and cold rollback behavior.
- Captured one pre-critical offensive packet and rolled Critical/Deadly,
  Crushing Blow, and Open Wounds independently per secondary target.
- Added per-target defenses, half life/mana leech, explicit exclusions,
  GUID deduplication/re-resolution, and a thread-local recursion guard.
- Added a separate reversible BKVince profile and append-only stat reservations.
- Added fail-closed build/signature/ownership validation and automated policy
  tests for D2R 3.2.92777.
- Fixed normal Attack capture by governing the direct `FillDamageValues`
  continuation used by the live 92777 melee handler, while retaining the queued
  melee continuation and rejecting all other Fill callers.
