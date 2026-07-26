# Vendor Stock Refresh

`VendorStockRefresh.dll` exposes D2R's native gamble refresh button in normal
vendor panels and asks the authoritative game server to rebuild the current
vendor stock through the existing vanilla refresh path.

## Compatibility

- Diablo II: Resurrected 3.2, build 92777 only.
- D2RLoader global or mod-local plugin scope.
- Standalone incubation DLL; future owner: `plugin-items.dll`.
- No dependency on, modification of, or redistribution of eezstreet DLLs.

The plugin validates every hooked function and patched UI site before changing
memory. Any build or signature mismatch refuses the plugin instead of guessing.

## Native flow

1. The existing `button_refresh` widget becomes visible and focusable in normal
   vendor mode while remaining hidden in repair mode.
2. `VendorPanelMessage:RefreshAll` keeps action 2 for gamble and sends action 1
   for a normal vendor in the native 9-byte opcode `0x38` packet.
3. The host checks that the player was already in the same normal vendor mode,
   resolves that vendor's live `VendorChainEntry`, and arms its vanilla
   refresh-pending byte.
4. The original store dispatcher clears, synchronizes, and rebuilds the stock,
   including permanent entries and any `plugin-items` vendor-overhaul policy.

Initial vendor openings, gamble, repair, player trade, rewards, missing vendor
entries, empty vendor inventories, and mismatched vendor sessions stay vanilla.

## Configuration

Create `VendorStockRefresh.json` in the active mod directory or in the global
D2R directory. The active mod file wins.

```jsonc
{
    "enabled": true
}
```

Unknown keys and non-boolean values are rejected. No TOML configuration is
supported.

Use the D2RLoader console command `vendor-stock-refresh` to display the loaded
configuration path and the client/server request counters.

## Build and test

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Credits

Authored by RuffnecKk. The D2RLoader Plugin SDK is maintained by the D2RLoader
project. The pinned eezstreet PluginPack was audited only for coexistence and
future `plugin-items.dll` ownership.
