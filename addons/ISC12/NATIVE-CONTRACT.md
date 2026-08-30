# ISC12 native contract

ISC12 never selects compatibility from a D2R build name or version. Those
values are diagnostic only. Before every native mutation, the DLL must validate the
complete governed fingerprint, native layout witnesses and exclusive ownership
of every surface.

Duplicate global/mod-local loads are excluded by a Windows mutex whose name is
suffixed with the current process ID. It permits separate host and joiner D2R
processes in the same Windows session while still allowing only one ISC12 owner
inside each process.

Version 0.2.0 fingerprints the complete loader/count/DescFunc seam, signed
priority comparator, native qsort, native vector layout, resizer, register
restores and epilogue contract. It installs a `PatchJmpRel32` over the exact
eight-byte seam at `0x31F0AB`, activates the guarded replacement, and only then
writes the aligned count immediate at `0x31ED38` from `0x1FF` to `0xFFF`.

The eight-byte seam is not naturally aligned, and the PluginSDK contract does
not prove that `PatchJmpRel32` suspends competing threads or publishes the write
atomically. Runtime activation therefore remains blocked until quiescent loader
startup or an equivalent transactional guarantee is proven.

Before calling `PatchJmpRel32`, ISC12 irrevocably reserves the RX relay and RW
state for the process lifetime. A false return is an uncertain mutation result,
not a pre-mutation failure: ISC12 reads and logs the eight observed seam bytes,
keeps both pages resident and immediately terminates fail-closed with a cold
restart requirement. It never unloads a possible jump target.

The replacement uses a fixed 4,095-entry scratch array, the native qsort and
signed comparator, and the native vector whose ABI is `{data*, size,
capacity|flag}`. It never interprets that vector as three pointers. Failures
before native-vector mutation may return to vanilla only when the proven table
count is at most 511; extended counts and every post-resize failure terminate
fail-closed.

The entry relay is copied to an RX process-lifetime page and references a
separate RW state page. Both success and vanilla exits return through this
persistent relay, which decrements the callback count only after all DLL code
has finished. After the tail is confirmed installed, the RW state
conservatively treats the count cap as potentially modified even if the cap API
reports failure. An inactive relay accepts only a proven vanilla row count;
otherwise it issues a non-resumable Windows fast-fail. Rundown has a five-second
bound and also fast-fails rather than permitting the DLL to unmap with an active
callback. This closes the DLL-unload race. Hot rollback remains unsupported.

The full-item decoder and serializer entries are intentionally absent from the
exclusive foundation fingerprint because `plugin-items` may already own those
entry hooks. ISC12 fingerprints and will later mutate only the proven internal
ID-width sites; both plugin load orders must remain valid.

Current and planned atomic groups are:

1. loader count plus bounded DescFunc replacement tail;
2. generic item reader plus writer plus sentinels;
3. each player/save reader-writer pair plus preview readers;
4. packet `0x3E` producer and consumer;
5. packet `0xA8` producer, consumer and terminator;
6. packet `0xAA` producer, consumer, count and estimator.

Quantity and outer State-ID fields remain 9-bit exclusions. Packet `0xAC`,
the shared-stash envelope and the native-cap fallback for item packets remain
blocked. The entry hooks already owned by `plugin-items` are never claimed by
ISC12.

The G1 item codec owns nine one-byte semantic mutations, including preservation
of the first-reader `previousStatId = -1` invariant. These internal sites do
not overlap the entry hooks owned by `plugin-items`, but G1 remains unpublished
until the clean-sheet marker and full-item packet budget gates are closed.
