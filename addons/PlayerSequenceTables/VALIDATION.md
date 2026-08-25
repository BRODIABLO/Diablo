# Player Sequence Tables validation

Status as of **25 August 2026**: **corrected test candidate; native record-set
ownership, loader coexistence, all four input policies and a fresh mod-local
24/24 cold start are proven on D2R 3.3.93847. Gameplay, reload, multiplayer and
the required D2R 3.2.92777 matrix remain untested, so the candidate is not
publishable**.

## Static and build gates

| Gate | Status | Required proof |
|---|---|---|
| Governed D2R target | passed | Workbench and analysis image verified for D2R 3.3.93847 |
| Native layout | passed | 25 groups, 14 weapon classes, 24-byte descriptors and 6-byte records |
| Baseline extraction | passed | 350 routes, 47 source arrays and 808 source records |
| Public normalization | passed | 47 independently editable native record sets, 44 current unique contents and 808 records |
| TXT integrity | passed | CRLF, no BOM and byte-exact governed round trip |
| Hook/write collision audit | passed | No Suite, patch or eezstreet owner overlaps `0x2386658..0x238671F` |
| Strict parser tests | passed | CTest `player-sequence-tables-policy`, valid baseline plus malformed config/table cases |
| Release x64 build | passed | MSVC 19.44 Release against PluginSDK `4933e2c42cb2592958cd0df3b6dc5003102252d1` |
| Reproducible DLL | passed | Two clean builds: `66D5C5EF9BA530740082A0C1C6BAFCABC02116E7C65D7A7C1F424AA20E4B2F2B` |
| D2R 3.2.92777 dual-build gate | not run | Version 0.1.0 currently accepts only build 93847; this test package remains 3.3.93847-only |

## Runtime matrix

| Gate | Status | Evidence |
|---|---|---|
| Tables absent | passed at loader gate | Plugin loads and logs that vanilla remains unchanged; no pointer transaction is installed |
| Valid baseline tables | passed at loader gate | Fresh corrected run logs 235 routes, 47 recordsets, 808 records and combined hash `8E93E155E600FCF9302A120FD3D3D62B5FD209E475A48BD67E80BB62BAB7E696` |
| One table missing | passed | Load refuses the pair before arena construction or pointer write with an actionable path error |
| Invalid present table | passed | Load refuses `playerseq.txt` line 2 before arena construction or pointer write with `unknown player mode` |
| Global installation | passed before ownership correction | Candidate reports `[global]`; the corrected tables have not been rerun in this scope |
| Mod-local installation | passed | Fresh corrected candidate reports `[mod]`; all 34 current plugins and 18 patches remain active, including the five eezstreet plugins, with no global duplicate |
| Full D2R startup | passed | Corrected tables compile with 190 total TXT tables and the fresh mod-local run reports `D2R startup complete` at 24/24 |
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

The corrected 25 August run produced no new crash report. The final restored
runtime is mod-local, with no global duplicate and no D2R process left running.
Source and runtime hashes match:

- DLL: `66D5C5EF9BA530740082A0C1C6BAFCABC02116E7C65D7A7C1F424AA20E4B2F2B`;
- `playerseqmap.txt`: `FA3AFD197906399911AA6D6BDFDF8FEBD4E630648B5533018ED2C8B5E5F4A46D`;
- `playerseq.txt`: `2A49C6B8E3BAE28DB1E8FB965B7A3E00565C18080D30FBED625AECFAFBA7A252`.

## Release and rollback gates

The public plugin archive policy permits only the DLL and TOML. The explicitly
authorized Discord test package `PlayerSequenceTables-test.zip` additionally
contains the README and both essential TXT tables. That test exception does not
settle the final public distribution channel; validation documents remain
outside the generated ZIP for human review.

The corrected five-entry test archive has SHA-256
`5E39FE93715E92207636106BF42CE4EBCE782058E3E713610A58959817330969`; every
payload was rehashed from the archive and matches its governed source.

Rollback is file-only: remove the DLL, TOML and both TXT tables, then restart.
No save migration is required.
