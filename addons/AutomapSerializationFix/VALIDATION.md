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
| gameplay payload above 32,767 bytes | NOT RUN |
| automap persistence after transition | NOT RUN |
| Steam 3.3.93787 | NOT RUN — not claimed |
| cross-build multiplayer | NOT RUN — separate gate |
| public ZIP | NOT CREATED |

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

The cold-start evidence does not promote this candidate to a public release.
Gameplay above 32,767 serialized bytes, transition persistence, Steam and
cross-build multiplayer remain open exactly as listed above.
