# ISC12 G10 clean-sheet format audit

Date: 2026-08-30
Status: **BLOCKED — no marker implementation is authorized**

## Decision

ISC12 needs a real outer file envelope, not a repurposed D2S version, padding
field, D2I reserved byte range or sidecar. The envelope must be validated in its
entirety before the first 12-bit stat or item payload is decoded. The write path
must produce the same contract atomically.

The D2S read side has a proven pre-payload seam. The final D2S writer and the
whole-file shared-stash read/write seams are not yet identified. G10 therefore
remains blocked.

## Governed D2S evidence

- `0x41DF40` is the outer file loader. Its long entry signature is unique. It
  receives the owning context in RCX, the file/view object in RDX and an output
  pointer in R9, then obtains the raw buffer through `0x484FE0(file, 0)` and its
  size through `0x485070(file, 0)`. Its sole indexed caller is `0x483B92`.
- `0x41DFEC` requires the normal `0xAA55AA55` D2S magic. `0x41DFF3` sends
  versions at most 91 to the legacy decoder and every larger value to the modern
  path. A private version value is therefore not a fail-closed namespace.
- `0x52E910` is the modern payload decoder and has one direct caller,
  `0x41E03B`. The exact long entry signature is unique.
- `0x52E9C6` selects the pre-104 or 104+ header parser. `0x52EA3E` later checks
  the `Woo!` header witness.
- The exact call setup at `0x52EC28` reaches the modern player-stat decoder
  `0x533760` at `0x52EC4A`. An outer rejection at `0x41DF40` can consequently
  occur before the first proven ISC-sensitive stat decode.

The standard D2S header still carries its normal magic, version, declared size
and checksum. That checksum protects file integrity; it does not identify an
ISC12 namespace or bind the payload to an `ItemStatCost` schema.

## Governed shared-stash evidence

The governed fixture `data-BKVince/ModernSharedStashSoftCoreV2.d2i` is 680 bytes
and contains seven concatenated sectors. Each observed sector starts with a
64-byte header:

- `+0x00`: `0xAA55AA55`;
- `+0x04`: format `2`;
- `+0x08`: item format `105`;
- `+0x0C`: gold;
- `+0x10`: 32-bit sector size;
- `+0x14`: sector type;
- `+0x15..+0x3F`: zero in this fixture.

The observed sector sizes are `68, 68, 68, 68, 68, 162, 178`. The local Hero
Editor codec first walks every 64-byte header and declared size, then hydrates
items in a second phase. That is useful format evidence, but it does not prove
that D2R itself has an equivalent whole-file preflight seam.

Native candidates around `0x61CFA7` visibly compare the normal sector magic,
and item-decoder xrefs occur later in the same broad cluster. Their file-level
ABI, complete sector loop, mutation order and writer symmetry are not proven.
No current native site is therefore eligible to host an ISC12 shared-stash
envelope.

The zero bytes at `+0x15..+0x3F` are only an observation. Nothing proves that
vanilla rejects, preserves or semantically ignores them. They are not a safe
marker.

## Rejected marker designs

- **D2S version only:** values above 91 already enter the modern decoder and do
  not bind the payload to ISC12.
- **D2S padding or D2I reserved bytes:** absence of ISC12 may still let vanilla
  enter its 9-bit decoders.
- **One marker per D2I sector without a whole-file preflight:** an earlier tab
  could be decoded or mutated before a later mismatch is discovered.
- **Sidecar file:** copying, renaming or losing one file can pair a payload with
  the wrong schema; the update is not intrinsically atomic.

## Candidate envelope contract

The final values are intentionally not frozen yet. The candidate outer header
contains at least:

1. an ISC12 magic/namespace of at least 64 bits;
2. `envelopeVersion` and `headerSize`;
3. `storeKind` (`D2S` or whole-file `D2I`);
4. `codecBits = 12` and `sentinel = 0xFFF`;
5. a SHA-256 schema fingerprint;
6. payload length and a cryptographic payload hash;
7. any flags required to reconstruct and validate the inner store.

The inner payload remains a known D2S or D2I structure encoded with the ISC12
codec. A different outer magic makes unmodified vanilla reject the file before
interpreting the payload.

The schema fingerprint must be calculated from a canonical ordered codec
descriptor, not raw TXT bytes. That descriptor must cover every `ItemStatCost`
field that changes bit encoding or interpretation, including ID/order,
save/CSV widths and params, signedness, add/shift, encode and op semantics. The
exact field set is still an audit gate.

## Next gate — G10-A file-envelope seams

Prove four byte-exact surfaces and their complete ABI/lifetime contracts:

1. D2S outer load and unwrap before any player or item mutation;
2. D2S final write, size/checksum construction and atomic disk commit;
3. D2I whole-file load and validation of every sector before the first item
   decode;
4. D2I whole-file write and atomic disk commit.

For each surface, prove unique signatures, ownership, cleanup on every failure
edge and absence of partially constructed player/stash state. Only then freeze
the header and golden vectors for valid, vanilla, wrong-schema, truncated,
mixed-sector and bad-payload-hash cases.
