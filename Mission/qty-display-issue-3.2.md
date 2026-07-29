# Socketed quantity tooltip fix — D2RLoader 3.2

## Scope and ownership

Restore the quantity line when a stackable item is also socketed, covering
arrows, bolts, javelins and other throwing weapons configured with both
behaviors in BKVince. The feature changes tooltip presentation only; stack
storage, consumption, sockets, save data and item generation remain untouched.

Vincent confirmed `items` as the future PluginPack owner on 2026-07-23. The
standalone artifact is `QtyDisplayIssue.dll`; its future shared configuration
key is `items.qtyDisplayIssue`.

## Governed native evidence

The persistent D2R 3.2 workbench passed `status` and `self-test` for build
92777. The canonical decrypted image SHA-256 is
`CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715`;
the deterministic analysis image SHA-256 is
`673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB`.

- `ITEMS_GetStatsDescription`, RVA `0x2DC4B0`, has four direct callers. Its
  32-byte entry signature is unique in `.text` and is required before hooking.
- `STATLIST_UnitGetStatValue`, RVA `0x2F5C60`, reads current stat 70
  (`STAT_QUANTITY`) from the concrete item.
- `UNITS_GetItemData`, RVA `0x34A500`, returns the pointer stored at unit
  offset `+0x10`; item flags are read at item-data offset `+0x18`.
- `ITEMS_GetTotalMaxStack`, RVA `0x3719E0`, has 39 direct callers. Its verified
  body reads `items.txt` `maxstack` at record offset `+0xF0`, adds stat 254 and
  caps the result at 511.
- The socketed item flag is `0x00000800`. The 92777 tooltip builder itself
  checks this flag, while D2MOO corroborates its legacy semantic name as
  `IFLAG_SOCKETED` without contributing any address or ABI.

BKVince deliberately combines `stackable=1`, positive stack limits,
`hasinv=1` and non-zero `gemsockets` for the affected bases. This combination
is valid data but exposes the vanilla tooltip omission once sockets are added.

## PluginPack incubation audit

The governed reference is eezstreet `D2RL-Plugins` commit
`dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` (PluginPack 2.0.1, MIT).
`plugin-items.dll` reads the top-level `items` object from `D2RPlugins.json`.
Its current `ItemPluginOptions`, callbacks, patches and inline hooks do not
touch `ITEMS_GetStatsDescription` or any tooltip builder. It already uses
`STAT_QUANTITY` and a maximum-stack helper for vendor inventory, but that is a
separate code path.

The standalone plugin is therefore the sole owner of RVA `0x2DC4B0` during
incubation. Transmogrify remains the sole owner of the final tooltip hook at
`0x2BD480`; Advanced Item Tooltips installs no hook. A future merge moves the
feature, its option field and its hook into `plugin-items.dll`, after which the
standalone DLL must be removed to preserve unique hook ownership.

## Implementation and gates

`QtyDisplayIssue 1.0.0` is attributed exactly to `RuffnecKk`, supports global
and mod-local plugin folders, does not declare `ModScopedOnly`, and accepts only
build 92777. `QtyDisplayIssue.json` is resolved from the active mod first and
the game directory second. A missing file uses `enabled: true`; malformed
configuration is rejected.

After vanilla builds the stat description, the hook appends
`Quantity: current of maximum` only when the item is socketed and both native
quantity values are positive. The bounded append preserves the original buffer
on duplicates, invalid inputs or insufficient capacity.

Required validation:

- Release x64 build and unit tests;
- manifest/export inspection, author and description checks;
- missing, mod-local, global fallback and invalid JSON cases;
- arrows, bolts and javelins both unsocketed and socketed;
- inventory, stash, cube, merchant and ground tooltips;
- mouse and controller;
- coexistence with all five eezstreet DLLs, Transmogrify and Advanced Item
  Tooltips, with zero rejected or failed plugins;
- runtime validation in both mod-local and global plugin scopes;
- public ZIP containing only `QtyDisplayIssue.dll` and
  `QtyDisplayIssue.json`.

## Build, package and runtime validation — 2026-07-23

The Release x64 build and bounded tooltip unit tests pass under MSVC
19.44.35228. The PE is x64, embeds the D2RLoader v2 manifest, exports
`D2RLoaderGetPluginInfo`, `D2RLoaderLoadPlugin` and
`D2RLoaderUnloadPlugin`, and reports the expected RuffnecKk version metadata.
Its imported DLLs are Windows/MSVC runtime components only; it neither links
nor redistributes an eezstreet DLL.

Artifacts:

- source: `data-BKVince/d2rloader/plugins/QtyDisplayIssue-src/`;
- Release DLL: `data-BKVince/d2rloader/plugins/QtyDisplayIssue.dll`;
- configuration: `data-BKVince/BKVince.mpq/QtyDisplayIssue.json`;
- public archive: `addons/QtyDisplayIssue/QtyDisplayIssue.zip`;
- DLL SHA-256:
  `CDA241E093236212FC92627A2AB6CCCE9E90A799D78EB2B80C7D38D7B9E99CC4`;
- JSON SHA-256:
  `3D31ADF38FFDAE660197B692145A19558EDA74F465674A8F551661BDB2ED4327`;
- ZIP SHA-256:
  `F8193BA438314E71210B584929FFC5419CD0361735657730BC0231BE436008B6`.

The ZIP contains exactly `QtyDisplayIssue.dll` and `QtyDisplayIssue.json` at
its root.

| Domain | Case | Status | Current-run evidence |
|---|---|---|---|
| Deployment | Source/runtime hashes | passed | Mod-local DLL and JSON equal the governed hashes above. |
| Loading | Missing configuration | passed | Mod-local DLL loaded with its default and installed `0x2DC4B0`; startup reached 24/24. |
| Loading | Valid mod-local JSON | passed | Hook installed; `active=20`, `rejected=0`, `failed=0`; startup reached 24/24. |
| Loading | Global JSON fallback | passed | A global `enabled=false` suppressed the mod plugin hook while the plugin loaded successfully. |
| Loading | Global DLL scope | passed | DLL loaded as `[global]`, installed the same strict hook and reached 24/24. |
| Loading | Invalid JSON | passed | Wrong `enabled` type returned `D2RLoaderLoadPlugin=false`, installed no hook and produced the expected `failed=1`. |
| Coexistence | PluginPack and tooltip pipeline | passed | All five eezstreet DLLs, Transmogrify and Advanced Item Tooltips loaded with the enabled plugin; final summary `active=20`, `rejected=0`, `failed=0`. |
| Gameplay | Socketed projectile quantity visible | not run | Computer-use stopped when user input was detected in the Diablo window. |
| Gameplay | Inventory/stash/cube/vendor/ground | not run | Requires visual interaction with prepared affected items. |
| Input | Mouse/controller | not run | Requires visual gameplay validation. |

The final runtime state is restored to the mod-local DLL and valid mod-local
JSON only; the temporary global DLL and JSON were removed. The last automated
cold start reached frontend step 24/24 with the governed hook installed. A
later visual attempt launched Diablo, but automation stopped immediately when
user input was detected, as required; no visual result is inferred from that
session.

## Promotion dans le PluginPack — 28 juillet 2026

Le contrat `QtyDisplayIssue 1.1.0` est maintenant compilé directement dans
`plugin-items.dll` sous le bloc strict `items.qtyDisplayIssue`. Le template
joueur livre `enabled=false`; l'absence de clé suit le même défaut, de sorte que
les octets vanilla `75 0F` restent inchangés à `0x2BE118`. Une activation
vérifie l'unique signature de 33 octets à `0x2BE103`, puis remplace seulement le
branchement par `90 90` afin de laisser D2R afficher sa ligne native.

Le manifeste commun atteint 95 sites sans chevauchement. Les cinq DLL Release
compilent, 13/13 CTest passent et `plugin-items.dll` mesure `626688` octets avec
le SHA-256
`44D9992D67D1F1E02A5E2050DB7C461FD1D6E8DD8DD00204859EDBC075EEC006`.
Les cold starts actif et vanilla terminent à `24/24` avec
`scanned=26 active=25 disabled=1 rejected=0 failed=0`; une lecture mémoire du
processus prouve respectivement `90 90` et `75 0F`. Aucun crash frais n'est
apparu, le runtime est restauré byte-exact et aucun processus ne demeure.

Le checkpoint `d6562d4` (`Integrate Qty Display Fix prototype`) est poussé sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le DLL et le JSON
autonomes sont retirés du profil gouverné BKVince pour préserver un propriétaire
unique; les sources et l'archive autonome conservent la preuve gameplay déjà
obtenue. Les deux collisions de commandes console observées concernent de vieux
témoins Gamble et Enhanced Damage, pas le site Qty; le nettoyage final du lot
devra retirer tous les témoins remplacés avant le cold start global. Le visuel
intégré inventory/stash/Cube/vendor/ground et la manette restent `not run`.

Les preuves techniques sont conservées sous
`analysis-cache/pluginpack-foundation-runtime-validation/20260729-qty-display-issue/report.json`.
