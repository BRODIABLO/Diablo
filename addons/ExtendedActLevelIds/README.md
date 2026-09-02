# Extended Act Level IDs 0.1.0

Extended Act Level IDs allows new level rows to belong to any act through the
`Act` value in `levels.txt`.

D2R normally resolves an act from the contiguous ranges compiled from
`actinfo.txt`. As a result, a new Level ID appended after the Act V range is
treated as Act V even when its `levels.txt` row declares another act. This
plugin replaces that one decision with the authoritative compiled
`Levels.Act` value while preserving the original resolver as a fail-safe.

## Installation

Install the DLL and configuration in one scope only.

Global installation:

```text
<D2R>/d2rloader/plugins/d2rl-ruffneckk-extended-act-level-ids.dll
<D2R>/d2rloader/config/ruffneckk-extended-act-level-ids.json
```

Mod-local installation:

```text
<D2R>/mods/<mod>/d2rloader/plugins/d2rl-ruffneckk-extended-act-level-ids.dll
<D2R>/mods/<mod>/d2rloader/config/ruffneckk-extended-act-level-ids.json
```

The plugin supports both scopes but refuses a duplicate global and mod-local
installation in the same process. When a mod is active, its configuration has
priority over the global configuration.

## Configuration

```json
{
  "enabled": true
}
```

The file is optional and the built-in default is enabled. A present file must
be valid JSON, contain only `enabled`, and use a Boolean value. Invalid or
unknown settings cause the plugin to refuse loading before it installs a hook.
Restart D2RLoader after changing the DLL or configuration.

## Runtime contract

The plugin owns one central native hook. After each `DataTablesLoaded` event it
uses PluginSDK API v4 to copy the Classic, LoD, and RotW `Levels` tables into
plugin-owned immutable caches. It validates all of the following before using
them:

- the complete 48-byte native resolver fingerprint;
- the PluginSDK service versions;
- compiled `Levels` row size `0x18C`;
- the `Id` field through a service lookup round-trip for every row;
- the `Act` field at `+0x0D`, including the five vanilla act boundaries;
- every act value is between `0` and `4`.

An unsupported data context, table revision, missing Level ID, invalid act,
incomplete cache, signature mismatch, or hook ownership conflict never guesses
an answer. The original D2R resolver remains authoritative in those cases.
Build names are logged for diagnostics only and are not an allowlist.

The console command `extended-act-level-ids` reports cache state, table
revision, row counts, resolutions, fallbacks, configuration path, and the
diagnostic build name. `extended-act-level-ids resolve <level-id>
[data-context]` calls the hooked central resolver and reports the zero-based
Act index; the optional data context defaults to RotW (`3`).

## Compatibility

Version 0.1.0 is built against D2RLoader 1.2.0-beta and PluginSDK API v4 commit
`4933e2c42cb2592958cd0df3b6dc5003102252d1`. Runtime qualification targets the
official D2R `3.3.93847` build. D2R `3.2.92777` is covered only by governed
byte-exact equivalence of every native surface used by the plugin.

The DLL is an autonomous member of the RuffnecKk D2RLoader Suite. It does not
modify, link, merge, or redistribute any eezstreet plugin.

## Credits

D2MOO documented the historical fixed-threshold behavior and explicitly noted
that the act should be looked up from `Levels.txt`. D2MOO is used as a semantic
reference only; no legacy 32-bit address, structure, or ABI is reused.

Implementation and D2R 3.3 integration: `RuffnecKk`.
