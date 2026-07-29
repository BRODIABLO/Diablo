# D2R Sprite ED2R

D2R Sprite ED2R is a third-party portable GUI editor for Diablo II: Resurrected `.sprite` assets. Version 1.5 adds an English interface and supports batch PNG/GIF/video conversion, frame editing, background removal, folder-set merging, and automatic `lowend.sprite` generation.

## Installed version

- Version: 1.5
- Author: TEAM ED2R (Inven author account)
- Release post: <https://www.inven.co.kr/board/diablo2/5842/7942>
- Official download: <https://drive.google.com/file/d/1dtZuuM8UgGuRHKP5iCHvPikeTni83Tz1/view>
- ZIP SHA-256: `B8591E7EE7B5F5C9F552AC66105186DB65F99A2AB09A0B8FC6D5DD0E4E7F0EAC`
- EXE SHA-256: `4C011584C265F52EBF83A5B524E3C719F7646426FAF30332156EF70334C66F24`
- License terms stated by the author: free use and redistribution; commercial use is prohibited.

The local executable is stored at `bin/D2R_Sprite_ED2R_1.5.exe`. The `bin/` directory is intentionally ignored by Git because this is a 122 MB third-party binary. Reinstall it from the official source and verify the EXE hash above when cloning the workspace on another machine.

## Language

Open Settings, select `English`, save the setting, and restart the application. Version 1.5 requires a restart before the updated interface language appears.

## Recommended BKVince workflow

1. Back up the target assets before editing them.
2. Use `data-BKVince/BKVince.mpq/data/hd/global/ui/` as the working tree for BKVince assets.
3. Treat `data-BK/`, `data-BT/`, `data-VNP/`, and `data-vanilla3.2/` as read-only references.
4. Review every generated `.sprite` and `.lowend.sprite` pair before runtime synchronization.
5. Keep application backups under this tool's ignored `backups/` directory or outside the repository.

## Security note

The executable is unsigned and has no Windows version metadata. Microsoft Defender reported no threats during a custom scan on 2026-07-29. VirusTotal had no verdict for this new hash at installation time. Provenance, hash verification, and the local scan reduce risk but do not provide the assurance of a signed or reproducibly built release.
