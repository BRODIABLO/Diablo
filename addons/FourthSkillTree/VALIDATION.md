# Fourth Skill Tree Framework 0.1.0 validation

Validation date: 2026-08-25
Targets: Diablo II: Resurrected 3.2.92777 and 3.3.93847
Status: engineering milestone; not publicly releasable

## Automated gates

- Release x64 build: PASS; two clean `/Brepro` builds produced the same DLL.
- DLL SHA-256:
  `7B366BE4E2465F492D86B4B9F7D6015CE31470E2B7E2DBEA2D0940AF1FFFA397`.
- Policy and source-contract tests: PASS (`1/1` CTest).
- Strict TOML version, types and unknown-key rejection: PASS.
- Build policy: PASS; accepts exactly `92777` and `93847`, rejects empty,
  adjacent and unknown build names.
- Unmodified BKVince tables: PASS (`456` skill rows, `266` skilldesc rows,
  zero page-four rows and eight classes).
- Synthetic contract: PASS (`457` skill rows, `267` skilldesc rows, one
  Amazon page-four row and eight classes).
- Synthetic `skills.txt` SHA-256:
  `AD0EC1512EF242577F2365B5307B098AC12C196948D0AEB2026C79A455984BF5`.
- Synthetic `skilldesc.txt` SHA-256:
  `0B76AB6F34A2A25171607C9EE4001A551EEE907567BE7EC2D57F580C7B938FF9`.
- PE audit: PASS; x64 PE32+ DLL, exactly three D2RLoader exports, High Entropy
  VA, Dynamic Base and NX Compatible.
- PluginSDK commit: `4933e2c42cb2592958cd0df3b6dc5003102252d1`.
- Native hooks: none by design in 0.1.0.

## Runtime provenance

The installed D2R 3.3 runtime was identified before any deployment:

- version: `3.3.93847`;
- Build Key: `623f7a1f73eabb08ccb2b2046e3f9164`;
- `.build.info` SHA-256:
  `2EBCAD0521DBF038D5A7FE5395E96B4BEF6D4F0774F7B1F840E03C3DE9CB067A`;
- `D2R.exe` SHA-256:
  `E1F5436E3D9687F644EF16938B1B183D1FDEF434F18CF66D852CF68F48CC8936`.

## Runtime and functional matrix

| Case | Status | Required evidence |
|---|---|---|
| D2R 3.2.92777 | policy PASS; runtime not run | Separate signatures, cold starts, scopes, coexistence and gameplay remain required. |
| D2R 3.3.93847 | policy, full-stack cold start and rank-zero persistence PASS | Invested allocation, dynamic respec and network remain open. |
| Disabled default | automated PASS; runtime not run | Parser remains inactive and no hook exists. |
| Unmodified mod without page 4 | contract PASS; runtime not run | BKVince tables validate with zero page-four rows. |
| Invalid table contract | automated PASS | Plugin refuses before any native mutation. |
| Synthetic 31st class skill | compilation and two save/reload cycles PASS; direct invested-save probe blocked at player materialization | Build the invested witness from a current-runtime-native character instead of grafting a 31st rank into an older save. |
| Five UI states | blocked | Native UI inventory and fixture gate are not closed. |
| Full Suite plus five eezstreet DLLs | cold start PASS | `33` plugins, `18` patches, `190` compiled tables and startup complete. |
| Global and mod-local scope | mod-local PASS; global not run | Priority and duplicate neutralization remain open. |
| Solo, host and client | isolated solo rank-zero save/reload PASS; network not run | Matching and mismatched data observations remain open. |

## Runtime 31-skill persistence proof

The disposable fixture cloned Amazon `Pierce` into skill ID `456`, assigned it
to `SkillPage = 4`, and compiled with `31` Amazon skills while every other class
remained at `30`. D2RLoader reached startup complete with the full active stack.

The isolated `ama.d2s` started at `1,283` bytes with `NumSkills = 30`. After
entering the game, Save and Exit, reloading the same character, entering again
and saving again, it measured `1,284` bytes with `NumSkills = 31`. The `if`
section contains exactly `31` zero-valued rank bytes and is followed immediately
by the native `JM` item marker. Final save SHA-256:
`158C39A272B74FD99E1D41DD75F28C1E1CB490E0930D9153519058EC86263C25`.

Evidence is retained under
`game-tests/artifacts/20260824-230340831-fourth-skill-tree-save-reload/`.
The runner result is PASS, both in-game captures are present, the original
BKVince saves remained byte-identical and the synthetic source remained
byte-identical.

## Native allocation and respec proofs

A controlled D2R 3.3.93847 runtime read proved that slot `0x3B` in the server
packet callback table at `D2R+0x1D2A790` points to
`D2GAME_PACKETCALLBACK_Rcv0x3B_AllocateSkillPoints 0x4B3EE0`. The callback
accepts a five-byte packet, validates the skill id against the complete compiled
SkillsTxt table, resolves its current rank and `MaxLvl`, and applies the rank
through the native server mutation path. It never reads `SkillPage`, `SkillRow`
or `SkillColumn`.

The authoritative respec opcode `0x39` calls
`D2GAME_PLAYER_ResetStatsAndSkills 0x580F20`. Its skill phase,
`D2GAME_PLAYER_ResetSkills 0x4360F0`, iterates the player's complete compiled
class-skill list, removes every invested base rank and refunds the total to stat
id `5`. These proofs establish that neither server allocation nor the native
respec traversal has a hard-coded page-three or 30-skill filter. A dynamic
invest/respec witness is still required.

The common UI dispatcher `UI_DispatchMessage 0x843D90` is already governed by
the `plugin-skills` broker; RemoteStash uses a narrow call-site redirect so that
ownership remains unique. FourthSkillTree must integrate through that existing
ownership contract and must not install a second entry hook.

## Invested allocation attempt

The native-allocation fixture cloned Barbarian `Bash` as skill id `456` into a
free native page-one cell, producing `457` SkillsTxt rows and `267` SkillDesc
rows. This deliberately bypassed the not-yet-implemented page-four UI so the
allocation path could be tested independently.

A gameplay-backed level-99 `QtyTester` save previously exercised by the current
runtime was expanded from 30 to 31 rank bytes. The source SHA-256 was
`A28F46040B65100B1110967365DC617F3FC8EBEFEBD75C50BDAB890939B3C030`;
the expanded 3,518-byte copy had `NumSkills = 31`, a zero-valued 31st rank,
the following native `JM` marker, a corrected D2S checksum and SHA-256
`5EEF21219696170E5BB265AB87296BDA0B538F605BF2CC4FDA82BDC947362C78`.

D2R accepted that copy in the offline character menu and displayed the expected
level-99 Barbarian. Selecting a difficulty then closed D2R while materializing
the player, before the skill panel or allocation action became available. The
save remained byte-identical at the expanded hash, so no rank was invested and
no persistence success is inferred. This result rejects direct rank-array
grafting as the validation route; it does not invalidate the already successful
runtime-native 31-rank serialization proof.

## Runtime restoration

- Installed `skills.txt` restored to
  `99AF76F35A995731FE7011EC4FA0EC52666EF1CCFD568CFAB7E4C573BAA2593A`.
- Installed `skilldesc.txt` restored to
  `7770B4A5BEA2B0613D19EC7364C409AEF9B8206A89169CF633843AED2E06769B`.
- Installed `modinfo.json` restored to
  `6EAD15F68F349B933A6087343AC591A30B65B77ABA3B92C3B183AA3C42509B7D`.
- The deployed FourthSkillTree DLL/config were removed and the disposable
  profile was archived under `analysis-cache/fourth-skill-tree-fixture/`; the
  final rank-zero proof remains in the governed artifact directory.
- No D2R or D2RLoader process remained active after validation.

This milestone proves a safe data contract, rank-zero 31-skill serialization,
page-independent authoritative allocation and full-list native respec traversal.
It does not yet claim a fourth-page UI, a successful invested runtime witness,
dynamic respec, networking or public gameplay support.
