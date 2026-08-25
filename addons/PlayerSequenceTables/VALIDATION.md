# Player Sequence Tables validation

Status as of **24 August 2026**: **release candidate; loader coexistence, both
installation scopes, all four input policies and complete 24/24 cold starts are
proven. Gameplay, reload and multiplayer remain untested, so the candidate is
not publishable**.

## Static and build gates

| Gate | Status | Required proof |
|---|---|---|
| Governed D2R target | passed | Workbench and analysis image verified for D2R 3.3.93847 |
| Native layout | passed | 25 groups, 14 weapon classes, 24-byte descriptors and 6-byte records |
| Baseline extraction | passed | 350 routes, 47 source arrays and 808 source records |
| Public normalization | passed | 44 unique record sets and 757 de-duplicated records |
| TXT integrity | passed | CRLF, no BOM and byte-exact governed round trip |
| Hook/write collision audit | passed | No Suite, patch or eezstreet owner overlaps `0x2386658..0x238671F` |
| Strict parser tests | passed | CTest `player-sequence-tables-policy`, valid baseline plus malformed config/table cases |
| Release x64 build | passed | MSVC 19.44 Release against PluginSDK `4933e2c42cb2592958cd0df3b6dc5003102252d1` |
| Reproducible DLL | passed | Two clean builds: `66D5C5EF9BA530740082A0C1C6BAFCABC02116E7C65D7A7C1F424AA20E4B2F2B` |

## Runtime matrix

| Gate | Status | Evidence |
|---|---|---|
| Tables absent | passed at loader gate | Plugin loads and logs that vanilla remains unchanged; no pointer transaction is installed |
| Valid baseline tables | passed at loader gate | Plugin logs 235 routes, 44 recordsets, 757 records and combined hash `F1C043E1D66E48C86BB4ED4E0A4FF7E8B57F4B68ACA5854340C20D60B1EA4EAA` |
| One table missing | passed | Load refuses the pair before arena construction or pointer write with an actionable path error |
| Invalid present table | passed | Load refuses `playerseq.txt` line 2 before arena construction or pointer write with `unknown player mode` |
| Global installation | passed | Candidate reports `[global]`; all 32 plugins and 18 patches remain active, including the five eezstreet plugins, and startup reaches 24/24 |
| Mod-local installation | passed | Candidate reports `[mod]`; the same 32/18 stack reaches 24/24 and no global duplicate is installed |
| Full D2R startup | passed | Valid tables compile with 190 total TXT tables; global and restored mod-local runs both report `D2R startup complete` |
| Sequence 1 legacy skill | not run | Requires a full startup and representative native-event gameplay witness |
| Sequence 24 Cleave | not run | Requires a full startup and applicable weapon-class gameplay witness |
| Sequence 25 Mirrored Blades | not run | Requires a full startup and applicable weapon-class gameplay witness |
| Null route | not run | Requires a full startup and unavailable-route gameplay witness |
| Edited record | not run | Requires one reversible frame/event edit followed by restart and gameplay |
| Multiplayer | not run | Host and client must use the identical logged table hash |
| Unload/reload safety | not run | Menu/game transitions and D2RLoader unload/reload still require observation |

Two earlier attempts reproduced the known intermittent `0xC0000005` at
`dxgi.dll + 0x38B1C1`, most recently in
`C:\Games\Diablo II Resurrected\d2rloader\crashes\d2r-crash-report (2026_08_25 00_37_56 UTC).log`.
The failure is outside the plugin-owned pointer range and occurred after the
complete plugin/patch load. It is retained as environmental history rather than
hidden. Subsequent unchanged full-stack starts passed at 20:50 mod-local, 20:51
global and 20:52 restored mod-local; no fresh crash report was produced.

The final restored runtime is mod-local, with no global duplicate and no D2R
process left running. Source and runtime hashes match:

- DLL: `66D5C5EF9BA530740082A0C1C6BAFCABC02116E7C65D7A7C1F424AA20E4B2F2B`;
- `playerseqmap.txt`: `3D00E1BB391E2A19878B164ECAE45CDC2C35239ADC081816C93D3CCFCA3E47F9`;
- `playerseq.txt`: `BDD70BC115EC8A3E9B207DDFDD1C999B23D41A6561E60DE21FEC0C7B8D245589`.

## Release and rollback gates

The plugin archive policy permits only the DLL and TOML. The two public TXT
tables are essential mod data, so their distribution channel must be settled
before any release archive is created. README and validation documents remain
outside any generated ZIP for human review.

Rollback is file-only: remove the DLL, TOML and both TXT tables, then restart.
No save migration is required.
