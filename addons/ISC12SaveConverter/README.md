# ISC12 Save Converter

Converts Diablo II: Resurrected ItemStatCost save streams between the D2R
9-bit format and the ISC12 12-bit format.

Author: RuffnecKk

## Current status

The offline codec, interactive console and standalone Windows executable are
implemented. The 34-test suite proves byte-exact 9-to-12-to-9 round trips for a
real standalone v105 item, a complete v105 character and the governed BKVince
Shared Stash. A disposable D2R 3.3.93847 runtime qualification also passed two
load/save/reload cycles in both directions. This is a qualified release
candidate, not yet a public release and never a replacement for backups.

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

## Mod schemas

The converter is not tied to BKVince. It includes the standard D2R v105 schema
and accepts a versioned JSON schema pack supplied by any mod author. This data
is needed to locate variable-length stat payloads safely; it is not a code
adaptation to ISC12.

A custom mod that changes item bases, classes or serialized stat widths must
ship its matching schema pack. The same schema must describe the source save
and the target ISC12 environment. The tool fails closed when a selected
unpacked mod inherits required tables that are not present on disk; use its
complete schema pack instead of guessing missing definitions.

The converter does not copy an old `.d2rl` sidecar. Launch the converted save
under its intended D2RLoader mod and plugin environment so D2RLoader can create
the correct current sidecar.

## Run the standalone executable

Double-click `ISC12SaveConverter.exe` and follow the prompts, or use its
command-line interface:

```text
ISC12SaveConverter.exe --to isc12 <save-folder>
ISC12SaveConverter.exe --to d2r9 --schema <schema.json> <save-folder>
```

## Run from source

On Windows, double-click `Launch-ISC12-Save-Converter.cmd`, or run:

```text
npm run convert --prefix addons/ISC12SaveConverter -- --to isc12 <save-folder>
npm run convert --prefix addons/ISC12SaveConverter -- --to d2r9 --schema <schema.json> <save-folder>
```

Use `--help` for all options. The remaining public-release work is packaging,
human review of the release README and publication beside ISC12.

## Development test

```text
npm test --prefix addons/ISC12SaveConverter
```
