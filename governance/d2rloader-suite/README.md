# RuffnecKk D2RLoader Suite Governance

This public directory in the
[Diablo workspace](https://github.com/BRODIABLO/Diablo) stores the versioned
release-governance inputs for the public
[RuffnecKk D2RLoader Suite](https://github.com/RuffDood/RuffnecKk-D2RLoader-Suite).

It intentionally contains no plugin binaries, release archives, credentials,
or runtime logs.

## Layout

- `schemas/next-release.schema.json` defines the release-plan contract.
- `releases/<version>/next-release.json` records scope, decisions, and gates.
- `releases/<version>/release-allowlist.json` pins the exact package inputs and
  SHA-256 values for that release.
- `releases/<version>/promotion-ledger.json` preserves incubation provenance
  when canonical source authority moves to the public Suite repository.

The public Suite scripts must receive these paths explicitly. Packaging is
expected to fail closed when any required governance input is missing.

These governance documents are public in `BRODIABLO/Diablo`, but they must
never be copied into the Suite product repository or its release assets. Only
the final asset checksum catalog belongs alongside the GitHub release assets.
