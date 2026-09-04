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

$backupRoot = Join-Path $harnessRoot 'backup\pre-2.2.0-0.6.0'
$revivePlugin = Join-Path $bkvinceRoot 'd2rloader\plugins\d2rl-ruffneckk-revive-overhaul.dll'
$reviveConfig = Join-Path $bkvinceRoot 'd2rloader\config\ruffneckk-revive-overhaul.toml'
$scriptedAiPlugin = Join-Path $bkvinceRoot 'd2rloader\plugins\d2rl-ruffneckk-scripted-ai.dll'
$scriptedAiConfig = Join-Path $bkvinceRoot 'd2rloader\config\ruffneckk-scripted-ai.toml'
$scriptedAiPolicy = Join-Path $bkvinceRoot 'd2rloader\scripts\ruffneckk-scripted-ai\revive-companion.lua'
$requiredBackups = @(
    (Join-Path $backupRoot 'd2rl-ruffneckk-revive-overhaul.dll'),
    (Join-Path $backupRoot 'ruffneckk-revive-overhaul.toml'),
    (Join-Path $backupRoot 'd2rl-ruffneckk-scripted-ai.dll'),
    (Join-Path $backupRoot 'ruffneckk-scripted-ai.toml')
)
$missingBackups = @($requiredBackups | Where-Object {
    -not (Test-Path -LiteralPath $_ -PathType Leaf)
})
$backupReady = $missingBackups.Count -eq 0

if ($backupReady) {
    Copy-Item -LiteralPath (Join-Path $backupRoot 'd2rl-ruffneckk-revive-overhaul.dll') `
        -Destination $revivePlugin -Force
    Copy-Item -LiteralPath (Join-Path $backupRoot 'ruffneckk-revive-overhaul.toml') `
        -Destination $reviveConfig -Force
    Copy-Item -LiteralPath (Join-Path $backupRoot 'd2rl-ruffneckk-scripted-ai.dll') `
        -Destination $scriptedAiPlugin -Force
    Copy-Item -LiteralPath (Join-Path $backupRoot 'ruffneckk-scripted-ai.toml') `
        -Destination $scriptedAiConfig -Force

    $policyBackup = Join-Path $backupRoot 'revive-companion.lua'
    if (Test-Path -LiteralPath $policyBackup -PathType Leaf) {
        Copy-Item -LiteralPath $policyBackup -Destination $scriptedAiPolicy -Force
    }
    elseif (Test-Path -LiteralPath $scriptedAiPolicy -PathType Leaf) {
        Remove-Item -LiteralPath $scriptedAiPolicy -Force
    }
}
else {
    Copy-Item -LiteralPath (Join-Path $harnessRoot 'config\production.toml') `
        -Destination $reviveConfig -Force
    Copy-Item -LiteralPath (Join-Path $harnessRoot 'config\scripted-ai-production.toml') `
        -Destination $scriptedAiConfig -Force
}

[pscustomobject]@{
    restored = $true
    reviveVersion = (Get-Item -LiteralPath $revivePlugin).VersionInfo.FileVersion
    reviveSha256 = (Get-FileHash -LiteralPath $revivePlugin -Algorithm SHA256).Hash
    scriptedAiVersion = (Get-Item -LiteralPath $scriptedAiPlugin).VersionInfo.FileVersion
    scriptedAiSha256 = (Get-FileHash -LiteralPath $scriptedAiPlugin -Algorithm SHA256).Hash
    backupUsed = $backupReady
} | ConvertTo-Json -Depth 4
