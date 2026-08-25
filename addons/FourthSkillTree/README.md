# Fourth Skill Tree Framework 0.1.0

Validates active-mod data for a moddable fourth skill tree.

This is the first engineering milestone, not the finished fourth-page UI. It
deliberately installs no D2R hook. Its purpose is to freeze and test the data
boundary before any additional invested skill can reach a real save file.

## Ownership boundary

The framework owns validation, future page lifecycle, widgets, navigation and
safe integration with allocation, respec, persistence and networking. The mod
author owns every skill, prerequisite, formula, icon, text, position and balance
choice. RuffnecKk does not build skill trees on request.

Contract version 1 uses ordinary mod tables:

- `skills.txt` owns the skill and its `charclass`, `skilldesc` and prerequisites;
- `skilldesc.txt` selects the fourth page with `SkillPage = 4`;
- `SkillRow = 1..6` and `SkillColumn = 1..3` select the initial native grid.

The validator rejects duplicate cells, unknown or cross-class prerequisites,
fourth-page cycles, invalid grid positions, more than 18 page-four skills per
class and more than 255 serialized class skills. It does not copy skill content
into TOML.

## Install this milestone

Copy the DLL and TOML to either the global or mod-local D2RLoader folders. Do
not install the same plugin in both scopes.

```text
d2rloader/plugins/FourthSkillTree.dll
d2rloader/config/fourth-skill-tree.toml
```

The same layout is valid below `mods/<mod>/d2rloader/` for a mod-local install.
Set `enabled = true` only in a disposable fixture mod that intentionally contains
`SkillPage = 4` rows. Use the `fourth-skill-tree` D2RLoader console command to
show the validated source counts.

## Safety and compatibility

- Supported builds: Diablo II: Resurrected 3.2.92777 and 3.3.93847.
- Loader: governed D2RLoader/PluginSDK v3 baseline.
- Scope: global or mod-local; the plugin is not mod-scoped-only.
- Native ownership in 0.1.0: none; no hook or patch is installed.
- `UI_DispatchMessage 0x843D90` remains owned by the established
  RemoteStash/`plugin-skills` broker and is outside this plugin's design.
- The milestone is not eligible for a public release archive.

## Qualified milestone

Release build, exact dual-build policy tests, unmodified BKVince tables and the
synthetic 31-skill contract pass. A full-stack D2R 3.3.93847 run compiled the
31st Amazon skill and preserved a 31-byte rank section through two isolated
Save and Exit/reload cycles. Runtime validation on D2R 3.2.92777 has not run;
allocation, respec, the five-state UI, global scope and networking also remain
outside this milestone; see `VALIDATION.md`.

## Credits

- Author: `RuffnecKk`.
- Talonrage is credited as a historical behavioral reference only. No binary or
  asset from the historical D2Mod plugin is redistributed.
