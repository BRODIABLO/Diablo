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
