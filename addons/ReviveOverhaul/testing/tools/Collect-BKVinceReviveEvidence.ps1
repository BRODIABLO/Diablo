param(
    [string]$Label = 'manual'
)

$ErrorActionPreference = 'Stop'

if (@(Get-Process -Name D2R,D2RLoader -ErrorAction SilentlyContinue).Count -ne 0) {
    throw 'Close the authorized test session before collecting evidence.'
}

$safeLabel = $Label -replace '[^A-Za-z0-9_-]', '_'
$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$harnessRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$testingRoot = Split-Path -Parent $harnessRoot
$bkvinceRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $testingRoot))
$gameRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent (Split-Path -Parent $bkvinceRoot)))
$destination = Join-Path $harnessRoot ("evidence\{0}-{1}" -f $timestamp, $safeLabel)
New-Item -ItemType Directory -Path $destination -Force | Out-Null
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

$status = & (Join-Path $PSScriptRoot 'Get-BKVinceReviveLabStatus.ps1') -AsObject
[System.IO.File]::WriteAllText(
    (Join-Path $destination 'static-status.json'),
    ($status | ConvertTo-Json -Depth 8),
    $utf8NoBom)

$sources = @(
    (Join-Path $bkvinceRoot 'd2rloader\logs\d2rloader.log'),
    (Join-Path $bkvinceRoot 'd2rloader\logs\revive-overhaul.log'),
    (Join-Path $bkvinceRoot 'd2rloader\logs\ruffneckk-scripted-ai.log'),
    (Join-Path $gameRoot 'd2rloader\logs\d2rloader.log'),
    (Join-Path $gameRoot 'd2rloader\logs\revive-overhaul.log'),
    (Join-Path $gameRoot 'd2rloader\logs\ruffneckk-scripted-ai.log')
)
foreach ($source in $sources) {
    if (Test-Path -LiteralPath $source -PathType Leaf) {
        $scope = if ($source.StartsWith((Join-Path $bkvinceRoot 'd2rloader'), [System.StringComparison]::OrdinalIgnoreCase)) { 'mod' } else { 'global' }
        Copy-Item -LiteralPath $source -Destination (Join-Path $destination ("{0}-{1}" -f $scope, (Split-Path -Leaf $source)))
    }
}

$hashes = Get-ChildItem -LiteralPath $destination -File |
    Sort-Object Name |
    ForEach-Object {
        [pscustomobject]@{
            file = $_.Name
            bytes = $_.Length
            sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        }
    } | ConvertTo-Json -Depth 4
[System.IO.File]::WriteAllText(
    (Join-Path $destination 'hashes.json'),
    $hashes,
    $utf8NoBom)

Write-Output $destination
