$ErrorActionPreference = 'Stop'

if (@(Get-Process -Name D2R,D2RLoader -ErrorAction SilentlyContinue).Count -ne 0) {
    throw 'Stop D2R and D2RLoader before preparing the BKVince test configuration.'
}

$harnessRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$testingRoot = Split-Path -Parent $harnessRoot
$bkvinceRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $testingRoot))
if ((Split-Path -Leaf $bkvinceRoot) -ne 'BKVince') {
    throw "The harness is not deployed inside BKVince: $bkvinceRoot"
}

Copy-Item -LiteralPath (Join-Path $harnessRoot 'config\lab.toml') `
    -Destination (Join-Path $bkvinceRoot 'd2rloader\config\ruffneckk-revive-overhaul.toml') `
    -Force

& (Join-Path $PSScriptRoot 'Get-BKVinceReviveLabStatus.ps1')
