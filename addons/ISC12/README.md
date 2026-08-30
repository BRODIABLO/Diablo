# ISC12 0.2.0

> Experimental loader stage: disabled by default. Do not distribute, deploy,
> or use with real saves.

ISC12 is a clean-sheet D2RLoader format for overhaul mods that need more than
511 `ItemStatCost` rows. It reserves serialized IDs `0..4094` for stats and
`0xFFF` as the list terminator.

Version 0.2.0 implements the first guarded native stage only. When explicitly
enabled, it replaces the fixed 512-entry `DescFunc` sorting tail with bounded
4,095-entry storage, then raises the loader row cap from `0x1FF` to `0xFFF`.
The tail is attempted first. Relay/state lifetime becomes process-bound before
that patch API call because a false result does not prove that the non-aligned
target bytes remained untouched. An uncertain tail result logs the observed
eight-byte seam and terminates fail-closed so a cold restart is unavoidable.
After a confirmed tail commit, the relay publishes a conservative `cap may be
extended` guard before the second write for the same reason. A failed cap write
stays resident in a `partial-commit-cold-restart-required` state. The persistent
RX relay and separate RW state page also prevent shutdown from leaving an
executing thread inside an unloaded DLL.

The generic item, player-save and packet codecs are still 9-bit. Consequently,
this stage must not be used to create, load, save or transmit ISC12 data. The
shipped TOML and embedded fallback both keep the experiment disabled.

The current off-runtime source also prepares the G10-B persistence boundary:
exact D2S/D2I objects can be unwrapped only after the 96-byte envelope, schema
hash and complete inner payload pass validation, while writes use an ISC12-owned
sibling-temp/full-write/flush/atomic-replace transaction. Persistent RX relays,
native cleanup continuations and rundown state are prepared but deliberately
unpublished. `InstalledHookCount` remains zero and `codecReady` is never set, so
neither save seam can execute until the 12-bit item/player codecs are complete.

G2–G4 are now prepared off-runtime as three atomic player/save/preview groups.
Four auxiliary sites, six regular player-stat sites and four exhaustive
preview ID readers yield 28 governed byte slots: the width 9 / terminator
`0x1FF` changes, a loader-bound rel32 to a copied RX finalize leaf, and final
overflow-status publication. Every exact owner/site fingerprint is
preflighted with 39 unchanged owner, buffer, capacity, layout and overrun
witnesses before a group can write. The clean-sheet schema rejects
`CsvBits > 32` and `CsvParamBits > 16`, and a pure G2/G3 parser now refuses bad
markers, IDs, widths, truncation, more than 512 entries and missing terminators.
It is not connected to native reader entries yet. The G3 finalize leaf preserves
the native used-end result in RAX, returns the sticky overrun flag in EDX, and
is prepared without using D2R padding or unwind-owned bytes. Its CALL is flushed
before `mov eax, edx` is published as the final site. Publication still requires
a process-lifetime relay reservation, a separately proven quiescent boundary,
G4 whole-inner terminator bounds and immediate fail-fast handling of any
uncertain write or flush. These mutations are still source-only:
`PublishedCodecMutationCount` is zero.

## Planned release installation contract

After a future release is fully qualified, install the DLL and TOML in exactly
one D2RLoader scope.

Global:

```text
<D2R>/d2rloader/plugins/d2rl-ruffneckk-isc12.dll
<D2R>/d2rloader/config/ruffneckk-isc12.toml
```

Mod-local:

```text
<D2R>/mods/<mod>/d2rloader/plugins/d2rl-ruffneckk-isc12.dll
<D2R>/mods/<mod>/d2rloader/config/ruffneckk-isc12.toml
```

ISC12 saves will require a versioned marker and an `ItemStatCost` schema
fingerprint. Vanilla saves and mismatched schemas will be rejected before any
payload decode. Migration is intentionally delegated to a separate external
tool.

The 0.2.0 binary is an internal incubation artifact, not an installable
release. Runtime qualification will use disposable profiles and saves only
after the clean-sheet and network gates are implemented.

## Credits

- Author: `RuffnecKk`.
- D2MOO is credited for semantic knowledge of the historical ItemStatCost
  compiler and stat-list format. All addresses, signatures and x64 ABI are
  independently proven against the governed current D2R corpus.
- D2RLoader and PluginSDK provide the autonomous plugin runtime.
