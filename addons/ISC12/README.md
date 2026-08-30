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

The live process remains entirely 9-bit because the prepared generic-item and
player-save codec transaction is unpublished; packet codecs are not implanted.
Consequently, this stage must not be used to create, load, save or transmit
ISC12 data. The shipped TOML and embedded fallback both keep the experiment
disabled.

The current off-runtime source also prepares the G10-B persistence boundary:
exact D2S/D2I objects can be unwrapped only after the 96-byte envelope, schema
hash and complete inner payload pass validation, while writes use an ISC12-owned
sibling-temp/full-write/flush/atomic-replace transaction. Persistent RX relays,
native cleanup continuations and rundown state are prepared but deliberately
unpublished. `InstalledHookCount` remains zero and `codecReady` is never set, so
neither save seam can execute until the 12-bit item/player codecs are complete.

G1–G4 are now prepared off-runtime as one canonical four-group transaction,
ordered G2, G4, G1, G3 so overflow-status publication remains final. Twenty
exact mutable windows yield 49 governed byte slots: G1's nine
one-byte width/sentinel/seed changes across four unique interior windows, four
reader/copy CALL rel32 redirects, the remaining width 9 / terminator `0x1FF`
changes, a loader-bound rel32 to a copied RX finalize leaf, and final
overflow-status publication. Every mutation fingerprint is preflighted with
51 unchanged runtime owner, return, buffer, capacity, layout, cleanup and
overrun witnesses before any group can write. The G1 subsequent-reader window
also requires its interior CALL to resolve exactly to the governed native
`BITSTREAM_ReadBitsThunk`; arbitrary retargeting fails before any write.

The clean-sheet schema rejects `CsvBits > 32` and `CsvParamBits > 16`. The G2
and G3 exhaustive callers are prepared to enter process-lifetime RX relays,
then no-throw FRAME wrappers that accept only v105, validate marker, IDs,
widths, truncation, the 512-entry cap and terminator, and invoke the untouched
native readers under the immutable schema snapshot. Rejection returns the
native malformed status `0x12`; a successful native cursor that differs from
the preflight used-end fast-fails. G4 leaves its legacy branch A in 9-bit form,
changes only the v105 branch B, and prepares the exact copy CALL so the original
SaveObject copy runs once before whole-D2S validation. Invalid magic, version,
size, checksum, data context, marker, ID, payload bound or sentinel returns zero
through the native buffer-free/false exit.

The G3 finalize leaf preserves the native used-end result in RAX, returns the
sticky overrun flag in EDX, and is prepared without using D2R padding or
unwind-owned bytes. Its CALL is flushed before `mov eax, edx` is published as
the final site. A move-only opaque RAII lease now gates every full-set
fingerprint, write and flush, including loss-of-authority handling before and
after the first attempted mutation. The pinned D2RLoader SDK provides no
production issuer for that lease, so publication still requires a documented
loader-owned quiescent boundary, G9 network ownership/budgets, G10 activation
and immediate fail-fast handling of any uncertain write or flush. The process-lifetime relay
page is prepared, but no codec commit caller exists: `InstalledHookCount` and
`PublishedCodecMutationCount` are zero, `codecReady` stays false, and D2R has
not been run.

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
