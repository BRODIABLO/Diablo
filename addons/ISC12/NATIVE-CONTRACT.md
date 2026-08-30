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

## Prepared persistence boundary

G10-B P3b prepares, but does not publish, exact reader `0x9FC654` and writer
`0x9F95A2` mid-hooks. Both target objects are classified by their exact terminal
name. A target reader must report the current native success value and an
announced length equal to the actual DWORD byte count before ISC12 snapshots
the buffer. It then validates and unwraps the complete versioned envelope,
schema fingerprint and D2S/D2I inner payload before replacing native storage.
Every target rejection clears the native buffer and state before reusing the
governed close/unlock/status-6 continuation; uncertain cleanup fast-fails.

The target writer converts the already-canonical native UTF-8 path exactly,
builds the envelope, and reaches the final destination only through the
ISC12-owned sibling-temp, full-write, flush, atomic replace and rollback
transaction. Its native handle slot is initialized to `INVALID_HANDLE_VALUE`
before ISC12 selects the existing committed or rejected continuation. A failed
rollback that cannot prove preservation of the destination is terminal.

The persistence relay is process-lifetime RX with separate RW state and bounded
rundown, but remains inactive: `InstalledHookCount == 0`, `codecReady == false`
and neither save seam is patched. The PE unwind records describe only each MASM
stub's local `0x20`/`0x30` allocation. Because entry is a tail-jump from the
middle of a native frame and copied relays are not registered/chained, ISC12
does not claim generic recoverable unwind across these boundaries. Callbacks are
`noexcept`, contain the expected access/in-page/guard failures and fast-fail on
every unexpected exception. Any future recoverable cross-boundary unwind would
require a separately governed registered/chained unwind design.

## Prepared player/save/preview codec groups

G2 auxiliary player stats, G3 regular modern player stats and G4 frontend
preview are represented as three unpublished atomic groups. Fourteen exact
unique mutation-site patterns plus 39 unchanged owner, buffer, cardinality,
field-layout and overrun-path witnesses prove four auxiliary
reader/writer/terminator sites, six regular sites and the exhaustive four
preview ID reads. The two other
preview bit-reader calls are driven by the compiled record `SaveBits` value and
deliberately remain untouched.

The plan governs 28 byte slots: ID width immediates `9→12`, sentinel immediates
`0x1FF→0xFFF`, four dynamic rel32 bytes for the G3 finalize relay and two final
status bytes. The rel32 target is opaque outside the loader authority and is
bound to the copied RX leaf prepared from the governed MASM template. Dynamic
bytes already equal to their expected value are confirmed as no-op slots rather
than passed to the writer, while the complete instruction range is still
flushed. The plan validates every mutation and witness signature before the
first write and refuses publication without an external quiescence proof. The
caller's boolean is an assertion of quiescence, not an implementation of thread
suspension or transactionality.
Because the patch API cannot prove that a false result left a byte unchanged,
any attempted failure is a partial/uncertain commit requiring a cold restart;
there is no speculative hot rollback. `PublishedCodecMutationCount == 0`, and
no current loader path invokes the prepared commit function.

G2 owns a fixed `0x4000` buffer and an unchanged 512-entry cap. ISC12 rejects
the native schema before publication when `CsvBits > 32` or
`CsvParamBits > 16`, matching the native value/parameter representations. The
worst complete G2 section is therefore 3,844 bytes including its marker, well
inside the buffer. G3 instead receives only the space remaining after a
variable prefix. Its two total callers (`0x4000` stack and `0x8000` allocated)
are exhaustively fingerprinted. The prepared position-independent leaf
reproduces the native used-end result in RAX, loads the sticky
`bitstream+0x20` overrun flag into EDX, and returns without touching RSP or a
nonvolatile register. The redirected CALL is written and flushed first; the
non-overlapping `mov eax, edx` status site is written and flushed strictly last,
so the existing sole caller's nonzero failure edge observes late overflow.
The abandoned bytes at `0x5353F0` are not a code cave: they belong to the next
function's PDATA/unwind range and are never mutated.

Future publication must reserve the copied relay for process lifetime before
the first codec byte, keep all G2–G4 writers quiescent until both final flushes
complete, and fast-fail immediately on an uncertain write or flush before
quiescence is released. These are still activation gates, not claims of current
runtime behavior.

A no-allocation pure preflight now parses one supplied G2/G3 section in native
LSB-first order: marker `0x6667`, 12-bit IDs, schema-driven param/value widths,
512-entry cap and terminator `0xFFF`. It leaves output unchanged on invalid ID,
unsafe schema, truncation or missing terminator. It deliberately permits bytes
after the terminator because these sections are embedded; the future native
caller must therefore supply an exact governed structural window at reader
entries `0x530A00`/`0x533760`. No production caller or reader hook exists yet.
G4's fixed `0x4000` input and unobserved reader-overrun flag similarly require
strict whole-inner bounds through every value/param field and terminator, or a
governed native error exit.
