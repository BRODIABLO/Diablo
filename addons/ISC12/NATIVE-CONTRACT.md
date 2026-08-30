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

G1 contributes nine one-byte semantic mutations across four unique interior
windows, including preservation of the first-reader `previousStatId = -1`
invariant. It is integrated into the canonical G1–G4 planner without claiming
the entry hooks owned by `plugin-items`, but remains unpublished pending the
format/network gates and a real publication-quiescence authority. Its
subsequent-reader interior CALL must retain the exact governed target
`BITSTREAM_ReadBitsThunk 0xA1B6C0`; an arbitrary retarget is not compatible.

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

## Canonical prepared G1–G4 codec transaction

The canonical transaction contains four unpublished groups, 20 exact mutable
windows, 49 governed byte slots and 51 runtime witnesses. G1 contributes four
windows and nine one-byte slots; G2–G4 retain their 16-window, 40-slot and
51-witness subplan. Publication order is G2, G4, G1, G3 so G3 overflow-status
publication remains strictly final. G1 decoder/serializer entry fingerprints
remain static identity and ownership evidence rather than runtime witnesses,
because compatible `plugin-items` entry hooks may legitimately own them.

G2 auxiliary player stats, G3 regular modern player stats and G4 frontend
preview are represented as three unpublished atomic groups. Sixteen exact
unique mutation-site patterns plus 51 unchanged owner, native-return, buffer,
cardinality, field-layout, cleanup and overrun-path witnesses prove five G2
sites, eight G3 sites and three v105 G4 sites. G4 branch A remains the unchanged
legacy 9-bit witness; only branch B is eligible for v105. The two other preview
bit-reader calls are driven by compiled-record value widths and remain
untouched.

The G2–G4 subplan governs 40 byte slots: four rel32 CALL groups for G2, both G3 callers
and G4 copy preflight; ID width immediates `9→12`; sentinel immediates
`0x1FF→0xFFF`; four dynamic rel32 bytes for the G3 finalize relay; and two final
status bytes. Every target is opaque outside the loader authority and is bound
to an entry in the copied process-lifetime RX template. Dynamic bytes already
equal to their expected value are confirmed as no-op slots rather than passed
to the writer, while each complete instruction range is still flushed. A target
that resolves back to the original native CALL target is rejected. The plan
validates every mutation and witness signature before the first write and
refuses publication without a live opaque, move-only RAII
`CodecPublicationQuiescenceLease`. The lease is revalidated before every
fingerprint and write and before and after every instruction-cache flush. The
pinned D2RLoader SDK exposes neither a quiescence transaction nor a production
issuer, so production code cannot construct or forge this authority.
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
reproduces the native used-end result in RAX, loads the sticky overrun DWORD at
`[bitstream+0x20]` into EDX, and returns without touching RSP or a
nonvolatile register. The redirected CALL is written and flushed first; the
non-overlapping `mov eax, edx` status site is written and flushed strictly last,
so the existing sole caller's nonzero failure edge observes late overflow.
The abandoned bytes at `0x5353F0` are not a code cave: they belong to the next
function's PDATA/unwind range and are never mutated.

Future publication must reserve every copied relay for process lifetime before
the first codec byte and hold a loader-owned quiescence lease across every
G1–G4 fingerprint, write and cache flush. Lease loss or any uncertain
write/flush after the first attempted mutation requires an immediate cold
restart; hot rollback remains forbidden. These are activation gates, not
claims of current runtime behavior.

A no-allocation preflight parses each supplied G2/G3 section in native
LSB-first order: marker `0x6667`, 12-bit IDs, schema-driven param/value widths,
512-entry cap and terminator `0xFFF`. It leaves output unchanged on invalid ID,
unsafe schema, truncation or missing terminator. The exhaustive CALLs at
`0x531A6D`, `0x52EC4A` and `0x530A34` are prepared to target copied RX entries.
Those entries increment rundown, require both `operational` and `codecReady`,
and tail-jump to MASM FRAME wrappers in the DLL. Each wrapper forwards the exact
six-argument ABI to a `noexcept` helper, then exits through a copied RX leaf that
decrements rundown only after all DLL code has finished.

The helpers accept exact inner version 105 only. They copy at most 3,844 bytes
from `[*cursor,end)`, acquire the shared immutable schema snapshot, preflight,
invoke the untouched native owner `0x530A00` or `0x533760`, and require a zero
native status to leave `*cursor` at the predicted byte used-end. Expected
rejection returns the exact native malformed status `0x12`; a native fault or a
successful-cursor mismatch fast-fails. The legacy G2-to-G3 path therefore
rejects before native recursion and never reacquires the snapshot lock.

G4 retargets only preview CALL `0x61CF90`; shared copy owner `0xA1E110` remains
intact for its other callers. The no-throw wrapper calls that owner exactly
once, after which the SaveObject is unlocked and EAX is the completed copy
length. It then validates only `[temp,temp+N)`: D2S magic, exact v105, declared
size, checksum, context byte at `+0xF8` below four, wrapper-required and
validated marker `0x6667` at `+0x341`, each 12-bit ID below the immutable schema
row count, every
CsvParamBits/CsvBits payload, the 512-entry cap and `0xFFF`. Invalid data returns
zero through the existing `cmp eax,8 / jb` edge to the exact buffer-free/false
exit at `0x61D87D`. Native G4 never checks `0x6667` itself and may dereference a
null ItemStatCost lookup, so neither marker nor ID validation is optional.

All four reader/copy relays, the common RX rundown return and their RW handler
slots are prepared, but `codecReady` and persistence `operational` are never set
and no loader path invokes `CommitPreparedCodecPatchSet`. Preparation is not
publication.
