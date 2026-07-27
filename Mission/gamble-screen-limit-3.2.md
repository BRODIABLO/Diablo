# Increased gamble screen item limit — D2RLoader 3.2

## Scope

Increase the number of item-generation attempts used to populate the gambling
screen in `D2R.exe 3.2.92777`, without changing item eligibility, quality rolls,
prices or vendor inventories outside gamble mode.

## Proven native behavior

The persistent workbench was verified before analysis. The canonical decrypted
image and analysis index both match their governed SHA-256 values.

`D2GAME_STORES_FillGamble` begins at RVA `0x541880`. Its successful-generation
counter is incremented at `0x541A4A`, and the loop bound is the unique sequence:

```text
0x541A7C  83 FD 0E                cmp ebp, 0x0E
0x541A7F  0F 8C DB FE FF FF       jl  0x541960
```

The exact nine-byte pattern occurs once in the 92777 `.text` section. It proves
that the vanilla bound is 14 attempts, not approximately 30–32. Item creation or
placement can still fail early, so the number of visible entries may be lower.

D2MOO provides semantic corroboration only:
`D2MOO@19019806df7f3e877fa105b05395d1e3597e2316:source/D2Game/src/UNIT/SUnitNpc.cpp:2423-2575`
implements the legacy `D2GAME_STORES_FillGamble` loop with `nCounter < 14`.
No legacy address, structure or ABI is transferred to D2R 3.2.

## Implementation

`GambleScreenLimit 1.2.0` is a hybrid D2RLoader plugin attributed to
`RuffnecKk`. It can be installed globally or under a mod and does not declare
`ModScopedOnly`.

The plugin validates build 92777 and the complete original nine-byte signature,
then replaces only the immediate bound byte at RVA `0x541A7E`. During its
standalone incubation, configuration is read from `GambleScreenLimit.json`,
first from the active mod and then from the game directory:

```jsonc
{
    "enabled": true
}
```

The only public option is the boolean `enabled`. When enabled, the plugin applies
the fixed limit 32; when disabled, it leaves the vanilla limit 14 untouched.
Unknown keys, including the former `itemLimit`, invalidate the configuration so
no value above 32 can be requested through JSON. The plugin changes no TXT table
and installs no inline hook.

Vincent confirmed `items` as the future PluginPack owner on 2026-07-22. The
standalone DLL does not modify, link or redistribute an eezstreet binary. Its
flat JSON object is ready to move under `items.gambleScreenLimit` in the single
`D2RPlugins.json` when the feature is merged into `plugin-items.dll`. The
official PluginPack already owns `D2GAME_STORES_FillGamble` at RVA `0x541880`
but writes a distinct byte range, so the current incubation is composable.

## Artifacts and validation

- source: `data-BKVince/d2rloader/plugins/GambleScreenLimit-src/`;
- Release DLL: `data-BKVince/d2rloader/plugins/GambleScreenLimit.dll`;
- configuration: `data-BKVince/BKVince.mpq/GambleScreenLimit.json`;
- public archive: `addons/GambleScreenLimit/GambleScreenLimit.zip`, containing
  only the DLL and JSON, without README, TOML or sources;
- Release DLL SHA-256: `D62CA2B907C37424A008D0FD586B8FDD78D041A56CFF8C101E083A2FB7256ABF`;
- JSON SHA-256: `75771FC2FA6E20A4832B9237857465E9BC81146AE360CDE1DC56FCC046C781A6`;
- ZIP SHA-256: `EDF43315B908A5DDECFCA4A2708C869665347EFDB7974EE7AD02A05686CB9574`;
- policy tests: disabled resolves to 14 and enabled resolves to fixed 32;
- workbench byte search: one match at `0x541A7C`.

Cold-start validation passed on 2026-07-22: D2RLoader accepted the v2 manifest,
loaded the mod-scoped plugin, and logged that the bound changed from 14 to 32.
The route to Gheed and the gamble UI was exercised separately; the post-patch
visible-count and purchase matrix remains open.

Vincent confirmed on 2026-07-22 that 32 is stable in his initial in-game test.
On 2026-07-26, 32 became the fixed product maximum because it corresponds to the
gambling merchant grid capacity; higher values are no longer exposed.

Vincent confirmed the final 1.2.0 build in game on 2026-07-26. The fixed limit
32 behaves correctly and the standalone plugin is ready for a future integration
under `plugin-items.dll`. Purchasing extremes, repeated refreshes, alternate
vendors, controller, resolutions and host/joiner remain useful regression cases
for that integration, but no longer block the standalone release.

Version 1.2.0 passed three fresh configuration states in cold starts: `true`
applied fixed 32, `false` preserved vanilla 14, and the legacy `itemLimit` key
was rejected. With all five eezstreet PluginPack DLLs present,
`GambleScreenLimit.dll` and `plugin-items.dll` loaded together with
`rejected=0` and `failed=0`; the final runtime state uses the mod-local JSON with
`enabled: true`.
