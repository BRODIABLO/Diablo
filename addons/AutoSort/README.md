# AutoSort 0.1.1

Sorts only the visible player inventory or the currently selected stash tab
into configurable compact groups. It is a separate RuffnecKk D2RLoader Suite
plugin and does not replace Bulk Currency Deposit.

Version 0.1.1 moves the prototype to the D2RLoader 1.2 and PluginSDK API v4
baseline. This is an ABI and maintenance update only: the native planner,
packet, and authoritative server transaction remain the sole sorting path.
AutoSort does not use the SDK v4 item transaction services because their
current operation and container limits do not cover the complete AutoSort
contract.

## Install

Copy the DLL and TOML to either the global or mod-local D2RLoader folders. Do
not install the same plugin in both scopes.

```text
d2rloader/plugins/d2rl-ruffneckk-autosort.dll
d2rloader/config/ruffneckk-autosort.toml
```

## Use

Use D2R's existing controller AutoSort action, or bind `AutoSort` under
`RuffnecKk Suite` in D2RLoader Controls. Its default binding is `Shift+H`,
separate from Bulk Currency Deposit's `Shift+D` binding. A saved user binding
continues to override the default.

Version 0.1 keeps the optional Inventory button behind a separate UI gate;
`button.enabled` must remain `false`. Controller and Controls inputs already
run the same planner.

The current development candidate ships with `diagnostics.dry_run = false`.
Pressing AutoSort logs one compact plan summary and applies a complete plan
only when its strict hierarchical checks pass. Set both `dry_run = true` and
`log_items = true` to inspect every classification while intentionally
refusing every transaction.

AutoSort touches only the current grid. Currency items left in that grid can
be arranged with custom item-code or item-type rules, but are never transferred
to another stash tab. Bulk Currency Deposit remains responsible for Advanced
Stash routing.

## Configuration

`[fixed_regions.inventory] right_columns` reserves whole columns on the right
edge of the player inventory. Items already there stay at their exact
coordinates and empty cells there are never sorting destinations. The packaged
BKVince configuration uses `right_columns = 1` for its frozen potion column.
This setting does not affect stash tabs; use `0` for mods with a fully movable
inventory.

The TOML supports these built-in categories:

- `armor`, `weapons`, `jewelry`, `charms`
- `potions`, `keys`, `scrolls`, `books`
- `runes`, `gems`, `jewels`, `quest_items`
- `misc`, `unknown`

Each category accepts one of nine anchors, including `middle`, or `ignore`.
Armor, weapons, jewelry, charms, and potions also expose configurable subgroup
orders and optional subgroup anchors. Built-in subgroups include armor slots,
concrete weapon families, rings versus amulets, charm sizes, and potion
families. An omitted subgroup anchor inherits its category anchor.

Placement uses one explicit precedence chain: the first matching
`[[custom_groups]]` rule for an item code/type/quality overrides its built-in
subgroup anchor; a subgroup anchor overrides its category anchor; and an
omitted category setting keeps the packaged default. Detection remains
automatic for built-in Diablo families, so modders only need selectors for
mod-specific items or intentional overrides.

Dedicated `[[exclusions]]` may combine item codes, item type codes, and
qualities. A matching item stays at its exact coordinates and becomes a fixed
packing obstacle. Exclusions run before `[[custom_groups]]`, and custom groups
run before built-in categories. The legacy `[[custom_rules]]` spelling remains
accepted when `custom_groups` is absent. Within each rule, selectors in one
list use OR while different selector lists use AND. The packaged configuration
contains an active `Horadric Cube` custom group because the Cube is technically
a quest item but normally remains in the player inventory. Change that rule's
anchor without moving every other quest item.

Inside each category, identical item codes stay together in natural code
order. A candidate is eligible only when every category, subgroup, and item-code
block is connected and sibling block envelopes do not overlap. With
`optimize_free_space = true`, configured destinations and category order stay
authoritative; among equally faithful eligible candidates, the one exposing
the largest contiguous empty rectangle wins. Potions
additionally use semantic families and tiers: `hp1` through
`hp5`, then `mp1` through `mp5`, then `rvs` before `rvl`. Rune codes retain
their natural order, for example `r09`, `r10`. D2 exposes short codes in two
forms: runtime base-item codes are NUL-padded, while compiled ItemTypes codes
are space-padded. AutoSort normalizes both forms before matching, so selectors
such as `box`, `rvs`, and `r01` match either native representation exactly.

The complete layout is calculated before native execution. AutoSort first
enforces category, subgroup, and item-code cohesion, then honors configured
destinations and category order. It maximizes the largest useful empty
rectangle among equally anchored layouts when optimization is enabled. Stable
item identities provide deterministic tie-breakers. If strict
item-code order cannot fit a very full grid, AutoSort may retry within each
category using larger dimensions first, but the same hierarchy checks still
apply. There is no global dimension-first fallback. An invalid snapshot or any
layout that would fragment or interleave hierarchy blocks is refused before an
item moves.

Useful examples:

```toml
[anchors]
armor = "top_left"
weapons = "middle_left"
jewelry = "bottom_right"

# Subgroup overrides intentionally split a parent category. Leave them absent
# when every subgroup should remain in one visually continuous category block.
[subgroup_anchors.armor]
boots = "bottom_left"

[subgroup_anchors.jewelry]
rings = "bottom_right"
amulets = "bottom_left"

[[exclusions]]
name = "All books"
item_type_codes = ["book"]

[[custom_groups]]
name = "Horadric Cube"
anchor = "middle_left"
item_codes = ["box"]

[[custom_groups]]
name = "Mod Tokens"
anchor = "bottom_middle"
item_codes = ["tok1", "tok2"]
```

## Compatibility

The DLL does not use a D2R build-name or version allowlist. It logs the
observed identity and accepts the runtime only when every native function,
callsite, packet path, layout witness, and hook owner it relies on matches the
governed fingerprint. A mismatch leaves D2R's vanilla controller AutoSort
untouched.

Runtime qualification targets the official D2R `3.3.93847` baseline in global
and mod-local scope with the full plugin stack. D2R `3.2.92777` is covered only
through the governed native equivalence when every surface used by AutoSort
remains byte-exact; it does not require a duplicate gameplay matrix.

## Rollback

Remove the DLL and TOML. AutoSort does not alter save formats or data tables;
without its planner hook, D2R uses its original AutoSort implementation.

## Credits

- Author: `RuffnecKk`.
- D2MOO is credited as a semantic reference for Diablo II inventory concepts.
