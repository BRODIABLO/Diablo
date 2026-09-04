Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-TestSha256 {
    param([Parameter(Mandatory)][string]$LiteralPath)
    $stream = [IO.File]::OpenRead($LiteralPath)
    try {
        $algorithm = [Security.Cryptography.SHA256]::Create()
        try { return ([BitConverter]::ToString($algorithm.ComputeHash($stream))).Replace('-', '') }
        finally { $algorithm.Dispose() }
    } finally { $stream.Dispose() }
}

$scriptPath = Join-Path $PSScriptRoot 'Sync-SuiteRelease.ps1'
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('diablo-suite-sync-' + [guid]::NewGuid().ToString('N'))
$repo = Join-Path $testRoot 'repo'
$suite = Join-Path $testRoot 'suite'
$artifacts = Join-Path $testRoot 'artifacts'
$game = Join-Path $testRoot 'game'
$pluginSource = Join-Path $artifacts 'plugins\demo.dll'
$configSource = Join-Path $suite 'plugins\demo\config\demo.toml'
$patchSource = Join-Path $suite 'patches\demo.json'
$allowlistPath = Join-Path $testRoot 'release-allowlist.json'
$runtimeRoot = Join-Path $game 'd2rloader'
$pluginTarget = Join-Path $runtimeRoot 'plugins\demo.dll'
$configTarget = Join-Path $runtimeRoot 'config\demo.toml'
$patchTarget = Join-Path $runtimeRoot 'patches\demo.json'

try {
    foreach ($directory in @(
        $repo,
        [IO.Path]::GetDirectoryName($pluginSource),
        [IO.Path]::GetDirectoryName($configSource),
        [IO.Path]::GetDirectoryName($patchSource),
        [IO.Path]::GetDirectoryName($pluginTarget),
        [IO.Path]::GetDirectoryName($configTarget)
    )) { New-Item -ItemType Directory -Path $directory -Force | Out-Null }
    [IO.File]::WriteAllText((Join-Path $game 'D2R.exe'), 'fixture')
    [IO.File]::WriteAllText($pluginSource, 'release-plugin')
    [IO.File]::WriteAllText($configSource, 'setting = "release"')
    [IO.File]::WriteAllText($patchSource, '{"patch":"release"}')
    [IO.File]::WriteAllText($pluginTarget, 'previous-plugin')
    [IO.File]::WriteAllText($configTarget, 'setting = "player"')
    $allowlist = [ordered]@{
        schemaVersion = 1
        suite = [ordered]@{ id = 'ruffneckk-d2rloader-suite'; version = '9.9.9' }
        entries = @(
            [ordered]@{ kind = 'plugin-dll'; componentId = 'ruffneckk-demo'; version = '1.0.0'; source = 'plugins/demo.dll'; destination = 'plugins/demo.dll'; sha256 = Get-TestSha256 $pluginSource },
            [ordered]@{ kind = 'plugin-config-toml'; componentId = 'ruffneckk-demo'; source = 'plugins/demo/config/demo.toml'; destination = 'config/demo.toml'; sha256 = Get-TestSha256 $configSource },
            [ordered]@{ kind = 'memory-patch-json'; source = 'patches/demo.json'; destination = 'patches/demo.json'; sha256 = Get-TestSha256 $patchSource }
        )
    }
    [IO.File]::WriteAllText($allowlistPath, (($allowlist | ConvertTo-Json -Depth 10) + [Environment]::NewLine))

    $common = @{
        AllowlistPath = $allowlistPath
        ArtifactRoot = $artifacts
        SuiteRoot = $suite
        GameRoot = $game
        Profile = 'fixture'
        Scope = 'Global'
        RepositoryRoot = $repo
    }
    $plan = & $scriptPath -Mode Plan @common
    if ($plan.entries.Count -ne 3) { throw 'Plan did not select the three deployable entries.' }
    if ([IO.File]::ReadAllText($pluginTarget) -ne 'previous-plugin') { throw 'Plan modified the runtime.' }

    $refused = $false
    try { & $scriptPath -Mode Apply @common 2>$null | Out-Null }
    catch { $refused = $_.Exception.Message -match 'ConfirmRuntimeControl' }
    if (-not $refused) { throw 'Apply did not require explicit runtime authorization.' }

    $receipt = & $scriptPath -Mode Apply @common -ConfirmRuntimeControl
    if ((Get-TestSha256 $pluginTarget) -ne (Get-TestSha256 $pluginSource)) { throw 'Plugin hash differs after Apply.' }
    if ((Get-TestSha256 $patchTarget) -ne (Get-TestSha256 $patchSource)) { throw 'Patch hash differs after Apply.' }
    if ([IO.File]::ReadAllText($configTarget) -ne 'setting = "player"') { throw 'Apply overwrote the player configuration.' }
    if (-not (Test-Path -LiteralPath $receipt.receiptPath -PathType Leaf)) { throw 'Apply receipt was not persisted.' }

    $verification = & $scriptPath -Mode Verify @common
    if (-not $verification.valid) { throw "Verify failed: $($verification.errors -join '; ')" }
    if ($verification.warnings.Count -ne 1) { throw 'Verify did not report the preserved configuration.' }

    & $scriptPath -Mode Rollback -GameRoot $game -Profile fixture -Scope Global -RepositoryRoot $repo -ReceiptPath $receipt.receiptPath -ConfirmRuntimeControl | Out-Null
    if ([IO.File]::ReadAllText($pluginTarget) -ne 'previous-plugin') { throw 'Rollback did not restore the previous plugin.' }
    if (Test-Path -LiteralPath $patchTarget) { throw 'Rollback did not remove the newly created patch.' }
    if ([IO.File]::ReadAllText($configTarget) -ne 'setting = "player"') { throw 'Rollback altered the player configuration.' }

    $catalogReceipt = & $scriptPath -Mode Apply @common -ConfirmRuntimeControl
    $componentOnly = $common.Clone()
    $componentOnly.ComponentId = @('ruffneckk-demo')
    [IO.File]::WriteAllText($patchTarget, '{"patch":"locally-modified"}')
    $localRemovalRefused = $false
    try { & $scriptPath -Mode Apply @componentOnly -ReplaceManagedSet -ConfirmRuntimeControl 2>$null | Out-Null }
    catch { $localRemovalRefused = $_.Exception.Message -match 'locally modified managed file' }
    if (-not $localRemovalRefused) { throw 'ReplaceManagedSet removed a locally modified managed file.' }
    Copy-Item -LiteralPath $patchSource -Destination $patchTarget -Force
    $subsetReceipt = & $scriptPath -Mode Apply @componentOnly -ReplaceManagedSet -ConfirmRuntimeControl
    if (Test-Path -LiteralPath $patchTarget) { throw 'ReplaceManagedSet did not remove the obsolete managed patch.' }
    & $scriptPath -Mode Rollback -GameRoot $game -Profile fixture -Scope Global -RepositoryRoot $repo -ReceiptPath $subsetReceipt.receiptPath -ConfirmRuntimeControl | Out-Null
    if ((Get-TestSha256 $patchTarget) -ne (Get-TestSha256 $patchSource)) { throw 'Subset rollback did not restore the managed patch.' }
    & $scriptPath -Mode Rollback -GameRoot $game -Profile fixture -Scope Global -RepositoryRoot $repo -ReceiptPath $catalogReceipt.receiptPath -ConfirmRuntimeControl | Out-Null
    if ([IO.File]::ReadAllText($pluginTarget) -ne 'previous-plugin') { throw 'Catalog rollback did not restore the original plugin.' }
    if (Test-Path -LiteralPath $patchTarget) { throw 'Catalog rollback did not remove its patch.' }

    $collisionRefused = $false
    $modLocal = $common.Clone()
    $modLocal.Scope = 'ModLocal'
    try {
        & $scriptPath -Mode Apply @modLocal -ConfirmRuntimeControl 2>$null | Out-Null
    } catch { $collisionRefused = $_.Exception.Message -match 'Cross-scope duplicates' }
    if (-not $collisionRefused) { throw 'Apply did not refuse the global/mod-local duplicate.' }

    Write-Output 'VALID : Suite release plan/apply/verify/rollback/config-preservation/managed-set/cross-scope gates'
} finally {
    $resolvedTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    $resolvedTest = [IO.Path]::GetFullPath($testRoot)
    if (-not $resolvedTest.StartsWith($resolvedTemp, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove a test directory outside TEMP: $resolvedTest"
    }
    if (Test-Path -LiteralPath $resolvedTest) { Remove-Item -LiteralPath $resolvedTest -Recurse -Force }
}
