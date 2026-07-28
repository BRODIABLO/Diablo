# EtherealItemRules

`EtherealItemRules` is a hybrid D2RLoader plugin for D2R 3.2.92777. It can be
installed globally in `d2rloader/plugins` or locally in a mod plugin folder.
Its future eezstreet PluginPack owner is `plugin-items.dll`.

The component unifies two RuffnecKk witnesses without loading either one:

- `NoEtherealItemTypes.dll`, whose scoped hook filters configured item-type
  families from the two ethereal generation eligibility calls;
- `ethereal-item-rules.json`, whose four strict writes control generation
  chance, set-item eligibility, and indestructible-item eligibility.

The standalone JSON contract uses the same future PluginPack section names:

```json
{
  "etherealExclusions": {
    "enabled": false,
    "itemTypes": []
  },
  "etherealItemRules": {
    "enabled": false,
    "chancePercent": 5,
    "allowSetItems": false,
    "allowIndestructibleItems": false
  }
}
```

Configuration is read first from the active mod directory and then from the
game directory. Unknown keys, invalid types, item-type codes longer than four
characters, and chance values outside 0 through 100 are rejected. If no JSON
exists, every feature defaults to disabled and the vanilla executable remains
unchanged.

Native ownership for build 92777 is strict:

- inline hook: `ITEMS_CheckItemTypeId` at RVA `0x00373890`, scoped by return
  RVAs `0x004432DA` and `0x004432E9`;
- ethereal chance byte: RVA `0x004434DF`;
- set-quality branch: RVA `0x00443315`;
- durability eligibility call: RVA `0x004432F4`;
- indestructible-aware helper cave: RVA `0x0046D840`.

All enabled sites are preflighted before installation. Existing copies of the
two source witnesses must be disabled during validation to prevent duplicate
ownership. Exclusions remain absolute, including parent item types and forced
ethereal creation paths.

The source pins the MIT-licensed D2RLoader PluginSDK at commit
`efcfaaa52eeec9e379b3fc2aad1013bb3dddc970` and nlohmann/json at v3.11.3.
No eezstreet DLL is linked, modified, or redistributed. A public archive must
contain exactly `EtherealItemRules.dll` and `EtherealItemRules.json`.
