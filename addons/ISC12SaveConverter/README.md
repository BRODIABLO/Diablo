# ISC12 Save Converter

Converts standard (v105) D2R 9-bit `.d2s` files and compatible `.d2i` shared
stashes to and from ISC12 12-bit format. Supports clean vanilla saves and
modded saves using matching mod data. Original files are never overwritten.

Author: RuffnecKk

## Current status

The offline codec, interactive console and standalone Windows executable are
implemented. The 43-test suite proves byte-exact 9-to-12-to-9 round trips for a
real standalone v105 item, complete characters and shared stashes using vanilla,
BKVince and Yupgoolg data. Runtime qualification on D2R 3.3.93847 also loaded and
saved converted BKVince and Yupgoolg characters under ISC12. This is a qualified
release candidate, not yet a public release and never a replacement for backups.

The tool:

- converts copies instead of overwriting original saves;
- supports explicit 9-to-12 and 12-to-9 directions;
- refuses a D2R 9-bit conversion when any real stat ID is 511 or greater;
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

A custom mod that changes item bases, classes or serialized stat widths must be
selected through its installed mod folder or MPQ archive. The same effective
data must describe the source save and the target ISC12 environment. The tool
starts from its bundled vanilla tables and overlays only the files supplied by
the mod, matching D2R's normal inheritance behavior. It supports loose unpacked
data, folder-form `.mpq` installations and binary MPQ archives. Binary archives
are opened read-only and only the known save-schema tables are decompressed in
memory; the complete archive is never extracted. The converter-specific
`--schema` option remains available only as an advanced command-line integration
point; it is not presented in the public interactive menu.

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
ISC12SaveConverter.exe --to d2r9 --mod <installed-mod-or-mpq> <save-folder>
```

## Run from source

On Windows, double-click `Launch-ISC12-Save-Converter.cmd`, or run:

```text
npm run convert --prefix addons/ISC12SaveConverter -- --to isc12 <save-folder>
npm run convert --prefix addons/ISC12SaveConverter -- --to d2r9 --schema <schema.json> <save-folder>
```

Use `--help` for all options. The remaining public-release work is packaging,
human review of the release README and publication beside ISC12.

## Third-party component

Binary MPQ reading uses `stormlib-js` 0.1.1 by tmo-gg under the MIT license. It
is bundled into the standalone executable and is used read-only.

## Development test

```text
npm test --prefix addons/ISC12SaveConverter
```
