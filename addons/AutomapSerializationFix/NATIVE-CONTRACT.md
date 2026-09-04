# Automap Serialization Fix — native contract

## Ownership

`Automap Serialization Fix` is the sole writer of
`D2R+0xD7E3F..D2R+0xD7E4B`. It replaces the vanilla signed 16-bit byte-count
epilogue with one static, checked 32-bit sequence. MapSense is a read-only
consumer of this witness and accepts only the complete vanilla or complete
corrected state.

No PluginPack or other RuffnecKk Suite component owns this range as of the
3 September 2026 audit.

## Required fingerprint

Before the patch, the DLL validates:

| Surface | RVA | Purpose |
|---|---:|---|
| serializer entry | `0xD7CE0` | function identity and frame |
| emission loop | `0xD7D30` | tag-byte filter, three `uint16` fields, two-byte stride |
| byte-count epilogue | `0xD7E3F` | exact 13-byte owned range |
| continuation | `0xD7E4C` | stack-cookie and return path |
| four callsites | `0xD62CB`, `0xD6345`, `0xD63B5`, `0xD6425` | four automap cell trees |
| owner commit | `0xD648E` | four dword lengths and sidecar writer handoff |

Every comparison is exact. No build name, channel, version or whole-PE hash is
used to select or accept an implementation.

## Owned bytes

Vanilla:

```text
0F B7 4E 08 66 03 C9 0F BF C9 41 89 0F
```

Corrected:

```text
33 C9 8B 56 08 03 D2 0F 43 CA 41 89 0F
```

The corrected code initializes the published dword to zero, reads the word
count as `uint32`, doubles it, conditionally selects the result only when the
addition did not carry, and writes the resulting dword. The return value in
`RAX`, nonvolatile registers, stack and continuation address are unchanged.

## Failure policy

- Any incomplete fingerprint refuses loading before the patch.
- An existing writer or changed owned range makes `PatchBytes` fail.
- Duplicate global and mod-local copies are refused by a process singleton.
- An unrepresentable 32-bit byte count publishes zero rather than a wrapped or
  sign-extended length.
