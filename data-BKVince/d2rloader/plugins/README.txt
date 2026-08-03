# D2RLoader mod plugins

Put plugin DLLs for this mod in this folder.

D2RLoader will load `.dll` files from here while the mod is active. These plugins are only used by this mod, unless they are also installed globally.

## Building a plugin

Build plugins against the external D2RLoader Plugin SDK:

https://github.com/D2RLoader/PluginSDK

```cpp
#include <D2RLPlugin/api.h>
```

CMake plugins should link the SDK target:

```cmake
target_link_libraries(MyPlugin PRIVATE D2RLPlugin::D2RLPlugin)
```

A plugin must:

* include the D2RLoader plugin manifest resource
* export `D2RLoaderGetPluginInfo`
* export `D2RLoaderLoadPlugin`

`D2RLoaderUnloadPlugin` is optional.

## Plugin info

`D2RLoaderGetPluginInfo` returns basic information about the plugin, such as:

* plugin id
* name
* version
* author
* optional description

The description is shown in the in-game Extensions tab when provided.

Plugin ids may only use lowercase letters, numbers, `.`, `_`, and `-`.

These ids are reserved and cannot be used:

```text
d2rloader
d2rloader.*
```

## Mod plugins vs global plugins

Plugins in this folder load before global plugins from:

```text
<game>/d2rloader/plugins
```

If a mod plugin and a global plugin use the same id, the global plugin will not load while this mod is active.

Use `D2RL::PluginFlags::ModScopedOnly` for plugins that should only ever load from a mod folder.

## Plugin context

`D2RLoaderLoadPlugin` receives a `D2RL::PluginContext`.

The context gives the plugin access to D2RLoader features such as:

* logging
* plugin config files
* console commands
* memory patch checks
* runtime memory patches

For plugins in this folder, D2RLoader uses mod-local paths:

```text
mods/<mod>/d2rloader/config/<plugin-id>.toml
mods/<mod>/d2rloader/logs/<plugin-id>.log
```

The context pointer stays valid until the plugin is unloaded during shutdown.

## Console commands

Plugins can register console commands from `D2RLoaderLoadPlugin`.

Example:

```cpp
static auto HelloCommand(
	D2R::Game::Client* client,
	const D2RL::ConsoleCommandContext* command,
	void*
) noexcept -> D2RL::ConsoleCommandResult {
	(void)client;

	command->plugin->WriteConsoleMessage("Hello from my plugin.");
	return D2RL::ConsoleCommandResult::Handled;
}

context->RegisterConsoleCommand(
	"hello-plugin",
	HelloCommand,
	"Print a plugin greeting."
);
```

The `client` pointer is available when there is a local player in an active game. It can be `nullptr` outside a game.

D2RLoader shows the plugin id as the command source automatically.

## Native hooks

Plugins can install raw inline hooks only when they declare:

```cpp
D2RL::PluginFlags::NativeHooks
```

Example:

```cpp
using PotionFn = void(__fastcall*)(void* player, void* item);

static PotionFn OriginalPotion = nullptr;

static void __fastcall PotionHook(void* player, void* item) {
	OriginalPotion(player, item);
}

context->InstallInlineHook(
	potionRva,
	expectedBytes,
	sizeof(expectedBytes),
	PotionHook,
	&OriginalPotion
);
```

Only use native hooks when needed. The hook function and original function pointer must exactly match the target function ABI.

D2RLoader checks the expected bytes before installing the hook and logs native hook installs to `d2rloader.log`.

Prefer D2RLoader-provided typed hooks when they are available.

## Unloading

Keep `D2RLoaderUnloadPlugin` quick and `noexcept`.

Use it to release resources owned by the plugin. D2RLoader calls unload callbacks during shutdown in reverse load order, but plugin DLLs stay loaded for the lifetime of the process.

## Installed D2RL-Plugins pack

The BKVince profile includes D2RL-Plugins 2.0.1, built in Release x64 from the
official repository at commit `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`:

https://github.com/eezstreet/D2RL-Plugins

The installed runtime DLLs are:

* `plugin-items.dll`
* `plugin-levels.dll`
* `plugin-misc.dll`
* `plugin-quests.dll`
* `plugin-skills.dll`

Starting with upstream commit `2111a354`, `plugin-shared` is a static library.
Its code is embedded in the five plugin DLLs, so there is no separate
`plugin-shared.dll` to install with D2RLoader 1.0.x.

The pack reads its feature configuration from
`BKVince.mpq/D2RPlugins.json`.

`plugin-misc.dll` is rebuilt from the same pinned commit with
`plugin-misc-native-hooks.patch`. The upstream plugin installs an inline hook
when either monster player-count cap is enabled, but declares no native-hook
capability; D2RLoader 1.0.1-beta correctly rejects it. The local one-line patch
declares `PluginFlags::NativeHooks` so the signature-checked hook can load.

`plugin-items.dll` is rebuilt from the pinned commit with
`plugin-items-repair-costs-cap.patch`. The internal Repair Costs Cap 1.4.0
feature is credited to RuffnecKk and reads `items.repairCostsCap` from the same
`D2RPlugins.json`. It caps individual repairs and the final Repair All total,
and can apply permanent durability wear per physically repaired item. Both the
price policy and durability wear are disabled in the shipped configuration, so
vanilla repair behavior remains the default.

Ground Item Label Limit 1.2.0 is an internal RuffnecKk feature of
`plugin-items.dll` under `items.groundItemLabels`. The shipped block is disabled,
so vanilla 32 remains effective; enabling it selects exactly 64 or 128. The
standalone JSON and disabled DLL were removed after the integrated cold starts.

Enhanced Damage Min/Max Fix 1.2.0 is also available internally in
`plugin-items.dll` under `items.enhancedDamageMinMaxFix`. The BKVince block is
enabled and the standalone DLL is no longer installed.

Bulk Skill Point Allocation 1.2.4 is integrated in `plugin-skills.dll` under
`skills.bulkSkillPointAllocation`. The shipped block is disabled and installs
no Bulk hooks, preserving vanilla clicks. When enabled, Ctrl uses the configured
native batch and Shift uses native assign-all. Its UI dispatcher broker composes
with RemoteStash 0.1.6 so exactly one module owns `0x843D90`; when Shift
confirmation is disabled, Bulk does not inspect or hook that dispatcher at all.

Transmute Hotkey 0.2.0 is integrated in `plugin-misc.dll` under
`misc.transmuteHotkey`. The shipped block is disabled and preserves vanilla
input. When enabled, it accepts a configurable keyboard chord or mouse button,
then dispatches the visible Cube Transmute action on the UI thread. It calls
through the current `0x843D90` dispatcher without owning that hook, so it
composes with RemoteStash or the optional Bulk confirmation broker.

Vendor Stock Refresh 0.1.5 is integrated in `plugin-items.dll` under
`items.vendorStockRefresh`. The shipped block is disabled and preserves vanilla
vendor stock. When enabled, it dynamically positions the native refresh button
under the loaded layout's gold display and keeps item generation authoritative
on the server. Its UI and packet hooks do not overlap the vendor-overhaul hooks
owned by `plugin-items.dll`.

Prevent Merc Death in Town 0.1.0 is integrated in `plugin-misc.dll` under
`misc.preventMercDeathInTown`. The shipped block is disabled and preserves
vanilla regeneration. When enabled, it prevents only a lethal lingering-damage
tick on a living mercenary in town, without healing the mercenary or protecting
it after leaving town. Its base-stat call composes with the Item Durability hook.

Item Durability 1.2.0 is integrated in `plugin-items.dll` under
`items.itemDurability`. The shipped block is disabled and its resistance,
ethereal maximum, forced maximum, ranged durability, and diagnostics values all
preserve vanilla behavior. The normal feature composes with Repair Costs Cap and
with standalone Transmogrify. The optional bow/crossbow extension shares
`0x314110` with standalone Transmogrify, so only one may own that site until an
external broker is implemented.

The former BKVince DLL and dedicated configuration files for all merged
features have been removed. Their `*-src` directories remain only as development
witnesses and must not be deployed alongside the integrated PluginPack options.

## BKVince standalone hybrid and mod-local plugins

* `PotionAutoPickup.dll` routes configured potion families through the vanilla
  server pickup path.
* `AdvancedItemTooltips.dll` 1.0.1 is hybrid (global or mod-local). It shows the
  native maximum socket count in vanilla gray and, for identified items only,
  appends variable affix and base-defense ranges in green. It reads the active
  mod's TXT tables instead of duplicating gameplay values. The command
  `advanced-item-tooltips` reports runtime counters.
* `FloatingDamage.dll` renders D2RLAN-style combat numbers and rolling DPS from
  the build-verified post-resistance damage observer. Its defaults live in
  `d2rloader/config/floating-damage.toml`; the command `floating-damage` controls
  status, activation, preview, reload, and reset.
