# Repeatable Services 0.1.9 prototype

Repeats Akara respec, Charsi imbue, Larzuk socketing, and Anya personalization after the native quest reward has been consumed. Each service can be disabled, free, or paid with a level-scaled gold price.

Repeat entries keep their normal selectable menu label and display the configured price. When carried gold plus personal-stash gold is below that price, the item-service window still opens but refuses to capture the item in its slot. The server repeats the affordability check before virtualizing the consumed quest reward. A late payment anomaly is logged and allowed to complete without charge so it can never delete the submitted item or strand the service UI.

This is an incubation build for a future merge into `plugin-quests.dll`. It targets D2R 3.2 build 92777 and works from either the global or mod-local D2RLoader plugin directory.

Place `RepeatableServices.dll` in a D2RLoader `plugins` directory and keep the
JSON in the matching standard configuration directory:

```text
<D2R>/d2rloader/plugins/RepeatableServices.dll
<D2R>/d2rloader/config/RepeatableServices.json
```

For a mod-local installation, use
`<D2R>/mods/<mod>/d2rloader/plugins/RepeatableServices.dll` and
`<D2R>/mods/<mod>/d2rloader/config/RepeatableServices.json` instead.

The first native quest reward remains free and preserves its normal quest bookkeeping. A repeat is offered only after `RewardGranted` is set and `RewardPending` is clear. Paid prices use `max(minimumGold, playerLevel * goldPerLevel)` and are recalculated by the authoritative server before any item, stat, or skill mutation.

For Charsi, Larzuk, and Anya, the plugin re-registers the native NPC action only for an already consumed reward. It does not rewrite quest flags or expose a service for an incomplete quest.

The repeat path identifies item services by their native selector and verifies the authoritative current-difficulty quest flags. It deliberately does not assume that the service record's one-based quest byte is identical across NPCs or data revisions.

Repeat labels are decorated at the final native menu-entry insertion call. The label uses the same client-side repeat decision that re-registers the action, while the server independently recalculates the authoritative price before charging.

Do not enable `infinite-quest-rewards.json` while testing this prototype. That patch intentionally prevents the native rewards from reaching the consumed state required to enter the paid repeat path.
