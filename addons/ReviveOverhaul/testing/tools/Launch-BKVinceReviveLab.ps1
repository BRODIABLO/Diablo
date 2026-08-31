param(
    [switch]$ConfirmGO
)

$ErrorActionPreference = 'Stop'

if (-not $ConfirmGO) {
    throw 'Launch blocked. Run only after Vincent gives GO, then pass -ConfirmGO.'
}

$status = & (Join-Path $PSScriptRoot 'Get-BKVinceReviveLabStatus.ps1') -AsObject
if (-not $status.readyForAuthorizedTest) {
    $status | ConvertTo-Json -Depth 8
    throw 'Static BKVince laboratory preflight failed. The game was not launched.'
}

$harnessRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$testingRoot = Split-Path -Parent $harnessRoot
$bkvinceRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $testingRoot))
$gameRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent (Split-Path -Parent $bkvinceRoot)))
$loaderPath = Join-Path $gameRoot 'D2RLoader.exe'

Start-Process -FilePath $loaderPath -WorkingDirectory $gameRoot -ArgumentList @(
    '-mod',
    'BKVince',
    '-txt',
    '-debug_mode'
)
