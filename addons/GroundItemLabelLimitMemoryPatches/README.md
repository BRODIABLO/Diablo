# Ground Item Label Limit memory patches

These fixed memory-patch presets raise the vanilla limit of 32 simultaneous
ground item labels on D2R 3.2.92777.

## Select one mode

| Mode | Action |
|---|---|
| Vanilla 32 | Install neither preset. |
| 64 labels | Copy only `presets/64/increase-floor-item-label-cap-to-64.json` to the active `d2rloader/patches/` directory. |
| 128 labels | Copy only `presets/128/increase-floor-item-label-cap-to-128.json` to the active `d2rloader/patches/` directory. |

Never load both presets at the same time. They intentionally patch the same
seven synchronized constants and are mutually exclusive.

Do not load either preset together with `GroundItemLabelLimit.dll`; both own the
same seven patch sites. The stock eezstreet PluginPack does not provide this
feature. Its confirmed future owner is `plugin-items.dll`, but no PluginPack
merge is currently active.

Both presets validate all seven original signatures before writing. The 128
preset uses wider instruction encodings where the original signed 8-bit
immediate cannot represent 128 safely.

These patches are build-specific. Do not use them with a D2R build other than
3.2.92777 without revalidating every signature.
