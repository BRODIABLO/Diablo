# D2R Save Converter

Converts standard (v105) D2R `.d2s` files and compatible `.d2i` shared stashes
across D2R 9-bit and ISC12 12-bit formats. Source-to-target mod-schema migration
is independent from the selected stat-ID widths. Original files are never
overwritten.

Author: RuffnecKk

This executable is the standalone save-conversion tool selected for the next
RuffnecKk D2RLoader Suite 1.3.0 release. Its final release packaging remains
pending.

## Current status

The offline codec, interactive console and standalone Windows executable are
implemented. The 64-test suite proves byte-exact 9-to-12-to-9 round trips for a
real standalone v105 item, complete characters and shared stashes. It also
proves all four width combinations (`9→12`, `12→9`, `9→9` and `12→12`) plus
source-to-target schema migration between vanilla and BKVince data, including
changed stat IDs, `SaveBits`, `SaveParamBits`, `SaveAdd`, signed player
attributes, auto-affixes and raw table references. Runtime qualification on D2R
3.3.93847 loaded, saved and reloaded the cross-schema BKVince-to-Yupgoolg
character and shared-stash witnesses under ISC12. The nine-item shared stash
remained byte-identical after a full process restart and a separate global-scope
run. The converter is never a replacement for backups.

The rebuilt standalone executable also migrates the same compatible BKVince
character and nine-item shared stash to Yupgoolg in both 9-to-9 and 12-to-12
modes, then restores each source byte-exact. Changing width and schema in one
pass produces the exact same target bytes as either two-step order. The final
ISC12 character and shared-stash outputs are byte-identical to the witnesses
already qualified in the game runtime.

The tool:

- converts copies instead of overwriting original saves;
- supports explicit 9-to-12, 12-to-9, 9-to-9 and 12-to-12 conversions;
- maps serialized stats by exact `Stat` name instead of assuming stable row IDs;
- converts values and params through the source and target save layouts;
- accepts a source stat above ID 510 during downgrade when the same named target
  stat has a valid D2R 9-bit ID and its value fits the target layout;
- reports every blocking save location without deleting items;
- covers player, item, socket, set, runeword, corpse, mercenary, Iron Golem and
  shared-stash stat streams.

It scans all inputs before creating the output directory. A corrupt or blocked
file therefore prevents the entire batch from writing partial converted saves.
Converted files keep their original names inside a new `ISC12 Converted`,
`D2R 9-bit Converted`, `ISC12 Mod Migrated` or `D2R 9-bit Mod Migrated`
directory. Existing output directories are never reused.

When a character `.d2s` is selected interactively, the converter finds every
adjacent shared-stash `.d2i`, lists the exact paths and offers to include them in
the same atomic batch. A `.d2i` can also be selected directly, while selecting a
save directory converts all `.d2s` and `.d2i` files beneath it.

Interactive completion is explicit. The executable prints `SUCCESS` or
`FAILED`, keeps errors and the exact output path visible, writes nothing after a
failed preflight, and waits for Enter before closing. After success it also
offers to open the output directory in Windows Explorer.
The open-folder prompt accepts both `O` (letter) and `0` (zero).

## Mod schemas

The converter is not tied to BKVince. Its interactive workflow distinguishes a
clean, unmodded D2R v105 save from a save produced by an installed mod. This
data is needed to locate variable-length stat payloads safely; it is not a code
adaptation to ISC12.

A custom mod that changes item bases, classes or serialized stat layouts must be
selected through its installed mod folder or MPQ archive. The converter first
uses the source stat-ID width and source data to decode what the save means,
then uses the target width and target data to encode the same character for the
destination game environment. Width and schema are independent: one operation
can change either one or both. Source and target may use the same data or two
different table sets.

The tool starts from its bundled vanilla tables and overlays only the TXT files
supplied by each mod, matching D2R's normal inheritance behavior. It supports
loose unpacked data, folder-form `.mpq` installations and binary MPQ archives.
Binary archives are opened read-only and only the known save-schema TXT tables
are decompressed in memory; the complete archive is never extracted. A mod that
exposes only compiled BIN tables is unsupported and fails before output is
created. The converter-specific JSON schema options remain advanced command-line
integration points; they are not presented in the public interactive menu.

Migration is fail-closed. Exact stat names are required, decoded values must fit
the target widths, compound stat layouts must remain semantically compatible,
and referenced bases, affixes, set items, unique items and runewords must exist
unambiguously in the target data. A mismatch is reported with its save location;
the converter never guesses, deletes the item or writes a partial batch.

Matching mod data covers table-driven save layouts. A mod DLL that changes item
serialization outside the TXT contract cannot be inferred from its MPQ; such a
save fails closed without creating output.

## Loading converted saves

Converting the save does not modify the installed mod. A 12-bit save must be
loaded with ISC12 enabled. `ExtendedItemStats.dll` version 0.3.14 may remain
installed: ISC12 accepts it only after verifying that exact installed version
and its exclusive ownership of all six full-item transport hooks, then
delegates that transport to it. Any other D2R 9-bit ItemStatCost codec or
ExtendedItemStats version remains unsupported and must be removed before
loading; ISC12 refuses an unknown, partial or mixed provider instead of
guessing.

After a downgrade to D2R 9-bit, restore the mod's original 9-bit ItemStatCost
plugin, if it had one, and disable ISC12 before loading the save. The executable
prints the corresponding instruction after every successful conversion.

The converter does not copy an old `.d2rl` sidecar. Launch the converted save
under its intended D2RLoader mod and plugin environment so D2RLoader can create
the correct current sidecar.

## Run the standalone executable

Double-click `D2RSaveConverter.exe` and follow the prompts, or use its
command-line interface:

```text
D2RSaveConverter.exe --to isc12 <save-folder>
D2RSaveConverter.exe --to isc12 --source-mod <old-mod> --target-mod <isc12-mod> <save-folder>
D2RSaveConverter.exe --to d2r9 --source-mod <isc12-mod> --target-mod <old-mod> <save-folder>
D2RSaveConverter.exe --from d2r9 --to d2r9 --source-mod <old-mod> --target-mod <new-mod> <save-folder>
D2RSaveConverter.exe --from isc12 --to isc12 --source-mod <old-mod> --target-mod <new-mod> <save-folder>
```

## Run from source

On Windows, double-click `Launch-D2R-Save-Converter.cmd`, or run:

```text
npm run convert --prefix addons/D2RSaveConverter -- --to isc12 <save-folder>
npm run convert --prefix addons/D2RSaveConverter -- --to d2r9 --source-mod <isc12-mod> --target-vanilla <save-folder>
npm run convert --prefix addons/D2RSaveConverter -- --from d2r9 --to d2r9 --source-mod <old-mod> --target-mod <new-mod> <save-folder>
```

Use `--help` for all options. The remaining public-release work is human review
of this README, replacement packaging and publication beside ISC12.

## Standalone candidate status

The previous test ZIP is superseded by the four-mode build and must not be
published. This README stays beside the local artifacts for human review before
a replacement public package is assembled.

- executable: 94,679,552 bytes, SHA-256
  `5BC08306B1A165641383D991AC430CF135126E5FB034667B34DE2A00FB95FD48`;
- reproducibility: two independent standalone builds are byte-identical;
- replacement ZIP: not packaged in this change.

## Third-party components and references

Save parsing and writing build on `@d2runewizard/d2s` 2.0.132 by prowner under
the ISC license. Binary MPQ reading uses `stormlib-js` 0.1.1 by tmo-gg under the
MIT license. Both are bundled into the standalone executable; MPQ access is
read-only.

D2MOO was used as a semantic reference for Diablo II save-value encoding,
including `SaveAdd`. No D2MOO binary code, address or 32-bit ABI is reused.

## Development test

```text
npm test --prefix addons/D2RSaveConverter
```
