$ErrorActionPreference = 'Stop'

if (@(Get-Process -Name D2R,D2RLoader -ErrorAction SilentlyContinue).Count -ne 0) {
    throw 'Stop D2R and D2RLoader before restoring the production configuration.'
}

$harnessRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$testingRoot = Split-Path -Parent $harnessRoot
$bkvinceRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $testingRoot))
if ((Split-Path -Leaf $bkvinceRoot) -ne 'BKVince') {
    throw "The harness is not deployed inside BKVince: $bkvinceRoot"
}

$source = Join-Path $harnessRoot 'config\production.toml'
$destination = Join-Path $bkvinceRoot 'd2rloader\config\ruffneckk-revive-overhaul.toml'
Copy-Item -LiteralPath $source -Destination $destination -Force

[pscustomobject]@{
    restored = $true
    destination = $destination
    sha256 = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
} | ConvertTo-Json -Depth 4
