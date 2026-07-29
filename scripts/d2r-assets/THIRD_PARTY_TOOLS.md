# D2R model toolchain — pinned third-party tools

This manifest makes the local D2R model workflow reproducible without vendoring
third-party source trees or unsigned binaries in the RuffnecKk repository.
Install the tools only under `analysis-cache/tools/`; that directory is ignored
by Git and excluded from the governed cadastre.

## Capability boundary

The two pinned tools cover different parts of the workflow:

| Tool | Proven capability | Missing capability |
|---|---|---|
| CarbonEngineJS Blender GR2 importer | Reads modern Granny meshes, UVs, normals, skeletons, weights, morphs, and animations | Does not export edited Blender geometry to GR2 |
| D2R GPS Fix1 | Converts exported GR2 files to D2R `.model`, including batch conversion | Does not convert DAE or FBX to GR2 |

Neither tool closes a generic `DAE/FBX -> GR2 -> D2R` round trip. The governed
first target remains an in-place morph: retain the original D2R container,
topology, vertex order, UVs, weights, skeleton, materials, and metadata, then
patch only verified vertex attributes and recalculate the Granny CRC.

## CarbonEngineJS Blender GR2 importer

- Upstream: <https://github.com/carbonenginejs/tool-blender.git>
- Pinned commit: `cd00830df8d892249bf10fd066d28613ea65d396`
- Commit date: `2026-07-17T15:32:44+12:00`
- GR2 importer version documented at that commit: `0.1.2`
- Supported Blender versions documented upstream: Blender 4.0 and newer
- Effective license at that commit: EUPL-1.2
- Local destination: `analysis-cache/tools/carbon-tools-blender-cd00830/`

The repository must retain its `LICENSE`, `NOTICE`,
`THIRD-PARTY-NOTICES.md`, and `EVE-CREATOR-LICENSE.md` files. Do not copy
Carbon source into a RuffnecKk release archive.

Install the pinned source from the repository root:

```powershell
$d2rAssetTools = Join-Path (Get-Location) 'analysis-cache\tools'
$carbonTarget = Join-Path $d2rAssetTools 'carbon-tools-blender-cd00830'
git clone https://github.com/carbonenginejs/tool-blender.git $carbonTarget
git -C $carbonTarget checkout --detach cd00830df8d892249bf10fd066d28613ea65d396
git -C $carbonTarget fsck --full --no-dangling
git -C $carbonTarget status --short
```

Expected result: repository integrity succeeds and `git status --short` is
empty at the pinned commit.

### D2R smoke witness

The pinned reader successfully inspected the decompressed Sorceress head sample
at `analysis-cache/sorceress-hair-test/head_lod0.vanilla.decompressed.gr2`:

- Granny version: `7`
- sections: `8`
- meshes: `8`
- materials: `40`
- textures: `16`
- embedded source path:
  `X:/assets/character/player/sorceress/rigging/approved/maya/player_sorceress_rig_approved.ma`

This proves read/inspection compatibility with the current D2R character
sample. It does not prove edited-geometry export or runtime acceptance.

## D2R GPS Fix1

- Author publication:
  <https://www.inven.co.kr/board/diablo2/5842/7833?iskin=diablo2>
- Original download:
  <https://drive.google.com/file/d/1kkQpzGGPaFIDRxnakww_GMo0VxcJK8pF/view?usp=sharing>
- Publication date: `2026-07-05`
- Retrieved and inspected: `2026-07-29`
- Local destination: `analysis-cache/tools/d2r-gps-fix1/`
- Original archive name used locally: `D2R_GPS_Fix1-original.zip`
- Archive SHA-256:
  `08B3F861426152D20F669D097B2624CF0122A015B262AD4CD1A4806D010379E3`
- Executable SHA-256:
  `B8AAFCCD32417DB78C149F42EA17131DA05C0051EA9A04725EF516FC1AB94C17`

The downloaded ZIP contains exactly one file, `D2R_GPS_Fix1.exe`. Static
inspection established:

- PE architecture: x64
- PE section count: `7`
- linker timestamp: `2026-07-21T09:29:52Z`
- packaging: PyInstaller
- embedded payload strings include `d2rpp`
- Authenticode status: `NotSigned`
- Windows file-version metadata: absent
- separate license file in the archive: absent

The publication states that the author's own material may be modified and
redistributed, but the original archive and this provenance record must remain
intact. Do not place the executable in a public RuffnecKk archive.

Download and verify without executing it:

```powershell
$d2rAssetTools = Join-Path (Get-Location) 'analysis-cache\tools'
$gpsRoot = Join-Path $d2rAssetTools 'd2r-gps-fix1'
New-Item -ItemType Directory -Path $gpsRoot -Force | Out-Null
$gpsArchive = Join-Path $gpsRoot 'D2R_GPS_Fix1-original.zip'
curl.exe --fail --location `
  'https://drive.usercontent.google.com/download?id=1kkQpzGGPaFIDRxnakww_GMo0VxcJK8pF&export=download&confirm=t' `
  --output $gpsArchive
Get-FileHash -Algorithm SHA256 -LiteralPath $gpsArchive
```

Stop if the archive hash differs from the pinned value. An upstream replacement
must be reviewed and recorded as a new version; never silently update this pin.
Because the executable is unsigned, run it only in an isolated disposable test
environment after static extraction and inspection of its PyInstaller payload.

## Governed first experiment

The next real model witness is one player-head LOD0 morph with these invariants:

1. produce a no-op baseline before any edit;
2. preserve vertex and index counts, vertex order, topology, UVs, bone indices,
   bone weights, skeleton, materials, and Granny extended data;
3. keep a stable original vertex identifier through Blender;
4. patch positions in the original decompressed container and recalculate CRC;
5. recompute and patch normals/tangents only when the position-only witness
   proves that shading requires it;
6. compare MoPaH/d2rpp and D2R GPS output from the same GR2 input;
7. prove parsing, bounded binary differences, hashes, rollback, and runtime
   behavior before propagating the edit to other LODs.

An AI-generated head may be used as a sculpting target only. It is not a direct
replacement asset until a separate full-topology, rigging, metadata, and runtime
pipeline is proven.
