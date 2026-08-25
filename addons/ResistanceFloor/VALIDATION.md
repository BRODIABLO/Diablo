# Resistance Floor 0.2.0 validation

Validation date: 2026-08-24
Target: Diablo II: Resurrected 3.3.93847
Plugin SHA-256: `32504A5E5CE8921EC24CCFC361DE4BD531F4A17F8B16F566805C3CC8B5C77DDC`
Configuration SHA-256: `2D1DD966AB3F5CA54F3D09F8AB6DB0A38624802F04890EA408D011AD3D89C2BC`
Candidate ZIP SHA-256: `4821D0AAFD3C1FBFC2702A5CA15E9857A9219BE06D3749C753EBE0AA8B55BA88`

## Automated gates

- Clean Release x64 build: PASS, zero compiler warnings.
- Policy and source-contract test suite: PASS, 1/1.
- Byte reproducibility: PASS; two builds, including one clean build, produced
  the same plugin hash.
- Player-facing configuration version 2 names, strict TOML types, required
  settings, unknown-key rejection and bounds `[-1000, -100]`: PASS.
- PE contract: PASS; x64, ASLR, high-entropy ASLR, NX and exactly the three
  D2RLoader exports.
- Governed RVA JSON parse and workbench status: PASS.
- PluginSDK commit: `4933e2c42cb2592958cd0df3b6dc5003102252d1`.
- ImGui/OverlayHost ABI commit: `f401021d5a5d56fe2304056c391e78f81c8d4b8f`.
- Candidate archive contents: PASS; DLL and TOML only. README remains beside
  the archive for human review.

## Runtime provenance

The installed runtime matches the governed baseline exactly:

- version: `3.3.93847`;
- Build Key: `623f7a1f73eabb08ccb2b2046e3f9164`;
- `.build.info` SHA-256:
  `2EBCAD0521DBF038D5A7FE5395E96B4BEF6D4F0774F7B1F840E03C3DE9CB067A`;
- `D2R.exe` SHA-256:
  `E1F5436E3D9687F644EF16938B1B183D1FDEF434F18CF66D852CF68F48CC8936`.

The source and final mod-local runtime copies of both candidate files are
byte-identical.

## Cold-start matrix

No installed plugin or PluginPack feature was disabled. The installed
inventory includes all five eezstreet plugins and the active RuffnecKk Suite.

| Installation | Status | Evidence |
|---|---|---|
| Mod-local BKVince 0.2.0 | PASS | Fresh run: `config_version = 2` accepted from the dedicated mod-local TOML; players and companions `-1000`, monsters disabled/`-1000`; both gameplay relays and the native Character Screen operand installed; 31 plugins, 18 memory patches, startup 24/24 complete. |
| Global plus mod-local duplicate | prior PASS / not rerun | Candidate 0.1.0 proved that the mod-local instance wins and the global duplicate is skipped. Scope-selection code did not change in 0.2.0; this row was not rerun for the terminology-only configuration migration. |
| Global only | prior PASS / not rerun | Candidate 0.1.0 proved global-only loading and global TOML selection. Scope-selection code did not change in 0.2.0; this row was not rerun for the terminology-only configuration migration. |

The final runtime installation is mod-local under BKVince. Its DLL and dedicated
TOML are byte-identical to the 0.2.0 sources; the D2RLoader-generated companion
TOML was also refreshed so no old technical labels remain visible. Global test
copies are absent and D2R was stopped. No installed plugin or PluginPack feature
was disabled during the fresh 0.2.0 run.

An initial diagnostic run was correctly refused because D2RLoader requires a
tracked `jmp-rel32` safety-check size equal to the five-byte patch size. The
final implementation retains the independent full twelve-byte preflight
witness, then supplies the exact five-byte `MOV` witness to the tracked write.
The fresh mod-local row uses the final byte-identical 0.2.0 candidate; the two
scope-retention rows explicitly preserve their earlier 0.1.0 provenance.

## Functional gameplay matrix

| Case | Status | Expected proof |
|---|---|---|
| Player six-type floor at `-1000` | not run | Controlled resistance debuff and damage comparison. |
| Hireling, summon, pet and Revive ownership | not run | Owned unit receives configured floor; ordinary monster remains at `-100`. |
| Monster opt-in | not run | Cold restart with monsters enabled, then controlled damage comparison. |
| Native Fire/Lightning/Cold/Poison display | not run | Character Screen visibly passes below `-100`. |
| Physical/Magic MapSense extension | not run | Readout appears only with Character Screen open and matches active values. |
| Eleven-times damage edge and integer safety | not run | Controlled highest-damage cases without overflow or instability. |
| Solo, host and client authority | not run | Separate observations for each network role. |

Compilation and a mixed startup do not close these gameplay gates. No runtime
or functional success is claimed until uncontested fresh logs and in-game
observations exist.

## Scope boundaries

- No TXT or TSV edit.
- No upper resistance-cap mutation.
- No eezstreet binary modification or redistribution.
- No ROADMAP edit, per the implementation instruction.
