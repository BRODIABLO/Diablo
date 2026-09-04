# Automap Serialization Fix — validation ledger

Candidate: `0.1.0`

| Gate | Status |
|---|---|
| governed native workbench | PASS |
| PluginPack ownership audit | PASS — no writer found |
| config decision | PASS — no real setting, no config file |
| policy tests | PASS — CTest 1/1 |
| Release x64 `/W4 /WX` | PASS |
| reproducible build | PASS — two clean builds, identical SHA-256 |
| three D2RLoader exports and manifest | PASS |
| MapSense regression tests | PASS — CTest 1/1 and two reproducible final builds |
| Battle.net mod-local cold start, full stack | PASS — 39 plugins, 17 patches, five eezstreet DLLs |
| Battle.net global cold start, full stack | PASS — 39 plugins, 17 patches, five eezstreet DLLs |
| both MapSense load orders | PASS — exact vanilla/fixed states, no fresh errors |
| D2RLoader 1.2.1 preview 10 mod-local cold start | PASS — full stack, MapSense 1.0.0, startup complete |
| D2RLoader 1.2.1 preview 10 global cold start | PASS — full stack, MapSense 1.0.0, startup complete |
| gameplay payload above 32,767 bytes | PASS — 6,000 tag-zero cells, 36,000-byte single-tree payload |
| automap persistence after transition | PASS — layer 0 → 1 → 0, 6,000/6,000 restored as tag 1 |
| Steam 3.3.93787 | NOT RUN — not claimed |
| cross-build multiplayer | NOT RUN — separate gate |
| release-candidate ZIP | PASS — DLL-only `0.1.0-rc.1` |

Runtime success must name the exact DLL hash and keep every installed plugin,
all five eezstreet DLLs and all PluginPack features active.

Static candidate identity:

- file: `d2rl-ruffneckk-automap-serialization-fix.dll`;
- size: `25,600` bytes;
- SHA-256: `C7193FADB024236136E241B79164EFF4CF86C0ED5C0E7CA77454D1BD7CD8CE17`;
- PE and PluginInfo version: `0.1.0`;
- exports: `D2RLoaderGetPluginInfo`, `D2RLoaderLoadPlugin`,
  `D2RLoaderUnloadPlugin`.

Runtime evidence captured on September 3, 2026:

- Battle.net D2R `3.3.93847`, Build Key
  `623f7a1f73eabb08ccb2b2046e3f9164`;
- `.build.info` SHA-256
  `2EBCAD0521DBF038D5A7FE5395E96B4BEF6D4F0774F7B1F840E03C3DE9CB067A`;
- `D2R.exe` SHA-256
  `E1F5436E3D9687F644EF16938B1B183D1FDEF434F18CF66D852CF68F48CC8936`;
- D2RLoader `1.2.0-beta` SHA-256
  `651FA9EB33083088349224B1624819F63ED79596F808950CF6468B5D82F7132E`;
- MapSense `1.0.0` SHA-256
  `2B71748E53084FDE72E36731293251C9776072F690AA7224F4E579AE7CC624A1`;
- final mod-local fix-first cold start: PASS;
- final mod-local MapSense-first cold start: PASS;
- final global fix with mod-local MapSense cold start: PASS;
- all three final starts reached `D2R startup complete` with `39` plugins,
  `17` memory patches, the five eezstreet DLLs and zero loader, fix or serializer
  witness errors;
- the original runtime was restored afterward: MapSense `0.13.41` SHA-256
  `25B18515A47B121C4F5905E8D16A8FA8560370519028A6BA31C11E35A8D9E24A`,
  `38` plugin DLLs, no fix installed and no D2R process active.

The first pre-final cold start exposed a noisy MapSense fallback diagnostic:
the first exact alternative logged an error before the second succeeded. That
run is retained as superseded evidence. MapSense now compares both complete
states silently and emits one error only if neither matches; its final build,
package and tested runtime artifact are byte-identical.

D2RLoader 1.2.1 compatibility was qualified on September 3, 2026 with the
user-provided `1.2.1-beta+preview.10` package, SHA-256
`D61230B250A0A3D94DD80EB2511822642F1ACA0353E981FCCB6214FA6243FEB3`:

- `D2RLoader.exe`: `F566D30ED5D41C7079D21E82BA9A2129EC8B0DA5472B50996D0D185D2D7BB4AF`;
- `D2RCore.dll`: `667241D494F6A73E940E9EE89544872482B6083A08060BB539CFCDE5FADE7125`;
- `d2rloader.mpq`: `2130F4FF3FED8E78C92D6E547BAB4F445A02B24E3F0A074679E0EDCE6E9E6008`.

One mod-local and one global cold start loaded Automap Serialization Fix
`0.1.0` and MapSense `1.0.0`, reached `D2R startup complete`, and reported
`39 plugins loaded`, `1 global duplicate skipped`, `17 memory patches`, all
five eezstreet DLLs and no fresh loader, fix or serializer-witness error. The
same tested plugin hash was used in both scopes. Evidence is retained under
`analysis-cache/runtime-validation/automap-serialization-fix-d2rloader-1.2.1-preview10-20260903/`.

The runtime was restored afterward to its exact pre-test plugin state:
MapSense `0.13.41` at SHA-256
`25B18515A47B121C4F5905E8D16A8FA8560370519028A6BA31C11E35A8D9E24A`,
14 mod-local and 25 global DLLs, no Automap Serialization Fix installation and
no D2R or D2RLoader process. The already-installed D2RLoader 1.2.1 preview 10
baseline and its customized TOML were preserved byte-exact.

The oversized gameplay path was qualified on the same Battle.net D2R
`3.3.93847` and D2RLoader `1.2.1-beta+preview.10` baseline. A diagnostic-only,
gitignored harness first verified the exact installed release-fix fingerprint,
then inserted `6,000` ordinary tag-zero cells into the current layer-zero floor
tree. The tree grew from `2,654` to `8,654` nodes and the resulting single-tree
payload was `36,000` bytes. The initial Rogue Encampment → Blood Moor control
correctly retained all cells as tag zero because both areas use automap layer
zero. Entering the Den of Evil changed to layer one; returning to Blood Moor
recreated the layer-zero owner and the harness found all `6,000/6,000` witness
keys with tags `0:0, 1:6000, other:0`. No crash occurred.

The captured `Helena.ma0` grew from `50,310` to `86,348` bytes and has SHA-256
`D0D2A0184E08CC2DC51468859AE98A1415ABC860AF3C26333DF169DD68EC4EB2`.
The tested fix remained the exact `25,600`-byte artifact identified above. The
full active stack reported `40 plugins loaded`, `1 global duplicate skipped`,
`17 memory patches`, all five eezstreet DLLs and `D2R startup complete`.
Evidence is retained under
`analysis-cache/runtime-validation/automap-serialization-fix-gameplay-20260903-220747/`.
After capture, all six `Helena` files and the original 39-DLL inventory were
restored byte-exact; both test DLLs were removed and no D2R process remained.

`AutomapSerializationFix-0.1.0-rc.1.zip` contains only
`d2rl-ruffneckk-automap-serialization-fix.dll` at the archive root. The ZIP is
`11,465` bytes with SHA-256
`5D76BC7B9FC66D61CB5D843D187E076C2F480C40FC279352C5F9793235778131`.
The existing README remains beside the ZIP and is intentionally excluded from
it. This release candidate is not yet a public-release claim: Steam and
cross-build multiplayer remain open exactly as listed above.
