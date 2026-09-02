# ISC12 Save Converter

Converts standard (v105) D2R 9-bit `.d2s` files and compatible `.d2i` shared
stashes to and from ISC12 12-bit format. Supports clean vanilla saves and
modded saves using matching mod data. Original files are never overwritten.

Author: RuffnecKk

## Current status

The offline codec, interactive console and standalone Windows executable are
implemented. The 60-test suite proves byte-exact 9-to-12-to-9 round trips for a
real standalone v105 item, complete characters and shared stashes. It also
proves source-to-target schema migration between vanilla and BKVince data,
including changed stat IDs, `SaveBits`, `SaveParamBits`, `SaveAdd`, signed player
attributes, auto-affixes and raw table references. Earlier runtime qualification
on D2R 3.3.93847 loaded and saved same-schema BKVince and Yupgoolg conversions
under ISC12. The new cross-schema path still requires its final runtime gate and
is never a replacement for backups.

The tool:

- converts copies instead of overwriting original saves;
- supports explicit 9-to-12 and 12-to-9 directions;
- maps serialized stats by exact `Stat` name instead of assuming stable row IDs;
- converts values and params through the source and target save layouts;
- accepts a source stat above ID 510 during downgrade when the same named target
  stat has a valid D2R 9-bit ID and its value fits the target layout;
- reports every blocking save location without deleting items;
- covers player, item, socket, set, runeword, corpse, mercenary, Iron Golem and
  shared-stash stat streams.

It scans all inputs before creating the output directory. A corrupt or blocked
file therefore prevents the entire batch from writing partial converted saves.
Converted files keep their original names inside a new `ISC12 Converted` or
`D2R 9-bit Converted` directory. Existing output directories are never reused.

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
uses the source data to decode what the save means, then uses the target data to
encode the same character for the destination game environment. Source and
target may use the same data or two different table sets.

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
loaded with ISC12 enabled. If the mod already includes a D2R 9-bit ItemStatCost
extension such as `ExtendedItemStats.dll`, replace that plugin with ISC12; the
two codecs own the same serialization path and must not be loaded together.

After a downgrade to D2R 9-bit, restore the mod's original 9-bit ItemStatCost
plugin, if it had one, and disable ISC12 before loading the save. The executable
prints the corresponding instruction after every successful conversion.

The converter does not copy an old `.d2rl` sidecar. Launch the converted save
under its intended D2RLoader mod and plugin environment so D2RLoader can create
the correct current sidecar.

## Run the standalone executable

Double-click `ISC12SaveConverter.exe` and follow the prompts, or use its
command-line interface:

```text
ISC12SaveConverter.exe --to isc12 <save-folder>
ISC12SaveConverter.exe --to isc12 --source-mod <old-mod> --target-mod <isc12-mod> <save-folder>
ISC12SaveConverter.exe --to d2r9 --source-mod <isc12-mod> --target-mod <old-mod> <save-folder>
```

## Run from source

On Windows, double-click `Launch-ISC12-Save-Converter.cmd`, or run:

```text
npm run convert --prefix addons/ISC12SaveConverter -- --to isc12 <save-folder>
npm run convert --prefix addons/ISC12SaveConverter -- --to d2r9 --source-mod <isc12-mod> --target-vanilla <save-folder>
```

Use `--help` for all options. The remaining public-release work is packaging,
human review of the release README and publication beside ISC12.

## Third-party components and references

Save parsing and writing build on `@d2runewizard/d2s` 2.0.132 by prowner under
the ISC license. Binary MPQ reading uses `stormlib-js` 0.1.1 by tmo-gg under the
MIT license. Both are bundled into the standalone executable; MPQ access is
read-only.

D2MOO was used as a semantic reference for Diablo II save-value encoding,
including `SaveAdd`. No D2MOO binary code, address or 32-bit ABI is reused.

## Development test

```text
npm test --prefix addons/ISC12SaveConverter
```
