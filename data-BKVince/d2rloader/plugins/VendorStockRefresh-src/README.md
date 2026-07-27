# Vendor Stock Refresh

`VendorStockRefresh.dll` exposes D2R's native gamble refresh button in normal
vendor panels and asks the authoritative game server to rebuild the current
vendor stock through the existing vanilla refresh path.

## Compatibility

- Diablo II: Resurrected 3.2, build 92777 only.
- D2RLoader global or mod-local plugin scope.
- Standalone incubation DLL; future owner: `plugin-items.dll`.
- No dependency on, modification of, or redistribution of eezstreet DLLs.

The plugin validates every hooked function and native UI helper before changing
memory. Any build or signature mismatch refuses the plugin instead of guessing.

## Native flow

1. The plugin reuses the original `button_refresh` widget and never ships or
   overwrites a vendor layout. In a normal vendor panel it reads the live gold
   block rectangle, centers the native button below it, and enables it. In
   gamble it restores the exact position supplied by the active layout and
   leaves vanilla visibility untouched.
2. `StashWidget` is the preferred live anchor. If a custom layout omits it, the
   plugin uses the union of `gold_icon` and `gold_amount`. If neither anchor nor
   a usable button rectangle exists, the normal refresh stays hidden and the
   plugin logs the incompatibility instead of placing a control over unrelated
   UI.
3. `VendorPanelMessage:RefreshAll` keeps action 2 for gamble and sends the
   private `VSRF` action marker for a normal vendor in the native 9-byte opcode
   `0x38` packet.
4. The authoritative host recognizes that marker, converts it to vanilla action
   1, then lets the original callback validate the NPC, act, and distance.
5. During that validated call only, the plugin resolves the live
   `VendorChainEntry` and arms its vanilla refresh-pending byte.
6. The original store dispatcher clears, synchronizes, and rebuilds the stock,
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
configuration path plus placement, request, armed, and rejection counters.

The plugin must be active on the authoritative host. A client-only installation
cannot refresh a remote host's vendor inventory.

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
