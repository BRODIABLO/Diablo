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

- `ITEMS_BuildQuantityDescription`, RVA `0x2C1E50`, is the native localized
  quantity formatter. It reads stat 70 (`STAT_QUANTITY`), calls
  `ITEMS_GetTotalMaxStack`, formats the `ItemStats1i` string and appends the
  native line separator. It has four direct tooltip callers.
- The primary tooltip path calls that formatter at RVA `0x2BE124`. Immediately
  before it, RVA `0x2BE111` tests `IFLAG_SOCKETED` (`0x00000800`) and the
  two-byte `jne` at RVA `0x2BE118` jumps exactly past the call. Replacing
  `75 0F` with `90 90` restores the vanilla call; the formatter remains empty
  for items whose quantity and maximum stack are both non-positive.
- The strict 33-byte signature beginning at RVA `0x2BE103` is unique in
  `.text`. It covers the socketed-flag test, its conditional jump and the
  native formatter call.
- `ITEMS_GetTotalMaxStack`, RVA `0x3719E0`, has 39 direct callers. Its verified
  body reads `items.txt` `maxstack` at record offset `+0xF0`, adds stat 254 and
  caps the result at 511.
- D2MOO commit `19019806df7f3e877fa105b05395d1e3597e2316`
  corroborates only the legacy semantic name and value `IFLAG_SOCKETED =
  0x00000800`; no D2MOO address, structure or ABI is reused.

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

The standalone plugin is therefore the sole owner of the two-byte patch at RVA
`0x2BE118` during incubation. Transmogrify remains the sole owner of the final
tooltip hook at `0x2BD480`; Advanced Item Tooltips installs no hook. The
abandoned `ITEMS_GetStatsDescription` prototype at `0x2DC4B0` is no longer
owned by QtyDisplayIssue. A future merge moves the feature, its option field
and this patch into `plugin-items.dll`, after which the standalone DLL must be
removed to preserve unique ownership.

## Implementation and gates

`QtyDisplayIssue 1.1.0` is attributed exactly to `RuffnecKk`, supports global
and mod-local plugin folders, does not declare `ModScopedOnly`, and accepts only
build 92777. `QtyDisplayIssue.json` is resolved from the active mod first and
the game directory second. A missing file uses `enabled: true`; malformed
configuration is rejected.

Version 1.0.0 appended a custom line from `ITEMS_GetStatsDescription`. Vincent's
retail screenshot on 2026-07-23 proved this was the wrong presentation layer:
the reverse-assembled buffer placed the blue quantity text on the same rendered
line as `Javelin Class`. Version 1.1.0 removes that hook and custom text
entirely. Its sole code change converts the native socketed-item skip at
`0x2BE118` from `jne +0x0F` to two NOPs, so D2R itself controls placement,
centering, color, localization and formatting.

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

## Build, package and runtime validation — 2026-07-23 to 2026-07-26

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
  `0AA995A50A0E31129D094DE1E4F062A2D679367879CE0451534E9BAF5862B8FC`;
- JSON SHA-256:
  `3D31ADF38FFDAE660197B692145A19558EDA74F465674A8F551661BDB2ED4327`;
- ZIP SHA-256:
  `380FC845B819E194D807CB73854E179E6EF4CE6DC6914516D89628C16E4B508A`.

The ZIP contains exactly `QtyDisplayIssue.dll` and `QtyDisplayIssue.json` at
its root.

| Domain | Case | Status | Current-run evidence |
|---|---|---|---|
| Deployment | Source/runtime hashes | passed | The governed DLL, the tested global DLL and the tested mod-local DLL were byte-identical at `0AA995A5…62B8FC`; the JSON hash is `3D31ADF3…D4327`. |
| Build | Release x64 and bounded patch test | passed | Version 1.1.0 compiled under MSVC 19.44.35228; the test proves the unique 33-byte signature and the exact `75 0F` to `90 90` replacement. |
| Loading | Missing configuration | not run | Version 1.0.0 passed this policy case; it was not rerun after the native-branch repair. |
| Loading | Valid mod-local JSON | passed | Version 1.1.0 loaded as `[mod]`; the BKVince cold start reached 24/24 with `rejected=0` and `failed=0`. |
| Loading | Global JSON fallback and DLL scope | passed | With no QtyDisplayIssue files in BKVince, version 1.1.0 loaded only as `[global]`; `active=20`, `disabled=0`, `rejected=0`, `failed=0`, startup 24/24. |
| Loading | Invalid JSON | not run | Version 1.0.0 rejected the wrong `enabled` type as expected; the case was not rerun on 1.1.0. |
| Coexistence | PluginPack and tooltip pipeline | passed | The BKVince plugin set loaded with QtyDisplayIssue 1.1.0 globally, 20 active plugins and no rejection or loading failure. |
| Gameplay | Socketed projectile quantity visible | passed | Vincent reported the 1.1.0 in-game retest successful on 2026-07-26: the affected socketed stackable uses D2R's native quantity line and placement. |
| Gameplay | Inventory/stash/cube/vendor/ground | not run | Requires visual interaction with prepared affected items. |
| Input | Mouse/controller | not run | Requires visual gameplay validation. |

The first retail visual attempt disproved the version 1.0.0 placement despite
its successful static and cold-start gates. Version 1.1.0 replaces that custom
tooltip append with the native-branch repair and has now passed the central
in-game case. Vincent deferred the PluginPack merge on 2026-07-26, then
explicitly added Qty Display Fix to the accepted PluginPack lot on 2026-07-28.
The validated standalone DLL and JSON remain the distributable witness until
the selected merge step moves the patch and `items.qtyDisplayIssue` into
`plugin-items.dll`, reruns the integration matrix, then removes the standalone
artifacts to preserve one owner for RVA `0x2BE118`.

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

## Gameplay intégré — 30 juillet 2026

Vincent confirme que l'affichage de quantité fonctionne sur le témoin préparé
dans le PluginPack intégré. Le chemin nominal observé est `passed`; les variantes
inventory/stash/Cube/vendor/ground et la manette qui n'ont pas été observées
explicitement restent `not run`.
