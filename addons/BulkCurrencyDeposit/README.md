# Bulk Currency Deposit 1.1.0

Transfers every supported stackable currency item from the player inventory
to its assigned Advanced Stash slot. This includes runes, gems and custom
currencies registered by the active mod.

## Install

Copy the DLL and TOML to either the global or mod-local D2RLoader folders. Do
not install the same plugin in both scopes.

```text
d2rloader/plugins/d2rl-ruffneckk-bulk-currency-deposit.dll
d2rloader/config/ruffneckk-bulk-currency-deposit.toml
```

## Compatibility

The DLL does not use a D2R build-name or version allowlist. It logs the
observed runtime identity, then requires its strict native fingerprint and
Diagnostics ownership checks before installing any listener or resource. A
mismatched build is refused safely; an unnamed build may load only when every
required native witness matches. Official qualification is still performed
separately on D2R 3.2.92777 and 3.3.93847.

## Use

Open the stash and press `Shift+D`. Change the binding under
`RuffnecKk Suite > Bulk Currency Deposit` in D2RLoader Controls.

The optional Inventory button runs the same action. Enable or move the
plugin-owned Inventory injection in the TOML:

```toml
[deposit]
inventory_button_enabled = true

[button]
x = 3
y = 813
```

The default position is below Dimentio's standard Charm Inventory button.

## Add the button to a mod layout

No companion MPQ archive is required. Keep the automatic Inventory injection
disabled when the active mod provides its own button:

```toml
[deposit]
inventory_button_enabled = false
```

The plugin still registers its private click command and button sprites. Add
the following editable child to the active stash panel's `children` array and
adjust only the outer `rect` coordinates for that layout:

```json
{
  "type": "ImageWidget",
  "name": "bulk-currency-deposit/stash-button",
  "fields": {
    "rect": { "x": 3, "y": 813, "width": 54, "height": 141 },
    "filename": "d2rloader/bulk-currency-deposit/button-mold"
  },
  "children": [
    {
      "type": "ButtonWidget",
      "name": "bulk-currency-deposit/deposit-button",
      "fields": {
        "rect": { "x": 1, "y": 39 },
        "filename": "d2rloader/bulk-currency-deposit/deposit-button",
        "hoveredFrame": 3,
        "onClickMessage": "PanelManager:OpenPanel:RuffnecKkBulkCurrencyDeposit",
        "pressLabelOffset": { "x": 0, "y": 2 },
        "tooltipString": "@RuffnecKkBulkCurrencyDepositTooltip"
      }
    }
  ]
}
```

For a standard loose-file mod this means editing an ordinary JSON file below
`mods/<mod>/<mod>.mpq/data/global/ui/layouts/`; it does not mean opening or
repacking a binary MPQ archive. The exact stash layout filename and coordinates
remain owned by the active mod.

## Match Dimentio's Charm Inventory button

Dimentio permitted reuse of the optional button sprites. A local Charm
Inventory customization can therefore use the same validated mold by replacing
only these two members in its companion MPQ:

```text
data/hd/global/ui/d2rloader/charm-inv/button_mold.sprite
  <- assets/button-mold.sprite
data/hd/global/ui/d2rloader/charm-inv/button_mold.lowend.sprite
  <- assets/button-mold.lowend.sprite
```

The normal and low-end source hashes are respectively
`1D538B74295588757E5DA0C1417F29A147CB7F44B80A041504806D35DBA339DD`
and
`39DFF56F0BF7CE2F51E9C277C1836F1406491718074AF917DDFED79E268029A3`.
The BKVince runtime test on August 28, 2026 changed exactly these two of the
18 MPQ members, kept Dimentio's Charm Inventory 0.19.0 DLL byte-identical and
was visually accepted as a clean UI match. This repository does not distribute
the modified third-party MPQ.

## Language

The tooltip follows the active D2R client language automatically. The DLL
registers `RuffnecKkBulkCurrencyDepositTooltip` for all 13 D2R locales through
its virtual string table; no language setting or companion MPQ is required.
The active mod can replace the complete plugin string resource at
`data/local/lng/strings/d2rloader/bulk-currency-deposit/strings.json` when it
needs different wording.

The native Advanced Stash registry decides which items can move and where they
belong. Unsupported and ordinary non-stackable items remain in the inventory.
The optional `include_item_codes` and `exclude_item_codes` lists can narrow
the eligible currencies; they cannot force an unsupported item into the stash.

## Credits

- Author: `RuffnecKk`.
- D2MOO is credited as a semantic reference for Diablo II inventory concepts.
- The optional button sprites are reused with Dimentio's permission.
