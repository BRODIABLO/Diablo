Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$scriptPath = Join-Path $PSScriptRoot 'Test-SuiteGovernance.ps1'
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('diablo-suite-governance-' + [guid]::NewGuid().ToString('N'))
$workspace = Join-Path $testRoot 'workspace'
$suite = Join-Path $testRoot 'suite'
$registryPath = Join-Path $testRoot 'next-release.json'

function Write-Registry {
    param([Parameter(Mandatory)][string]$SourceRef, [Parameter(Mandatory)][string]$Asset, [string]$Status = 'package-ready', [bool]$ReleaseReady = $true)
    $registry = [ordered]@{
        schemaVersion = 1
        suite = [ordered]@{ id = 'ruffneckk-d2rloader-suite'; status = $Status; releaseReady = $ReleaseReady }
        components = @([ordered]@{
            id = 'ruffneckk-demo'
            kind = 'plugin'
            disposition = 'include'
            targetVersion = '1.2.3'
            asset = $Asset
            sourceRef = $SourceRef
            gates = [ordered]@{ packaging = 'passed' }
        })
    }
    [IO.File]::WriteAllText($registryPath, (($registry | ConvertTo-Json -Depth 10) + [Environment]::NewLine))
}

try {
    New-Item -ItemType Directory -Path (Join-Path $workspace 'addons\Demo') -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $suite 'plugins\demo') -Force | Out-Null
    [IO.File]::WriteAllText((Join-Path $workspace 'addons\Demo\README.md'), 'incubation')
    [IO.File]::WriteAllText((Join-Path $suite 'plugins\demo\README.md'), 'public')

    Write-Registry -SourceRef 'workspace:addons/Demo' -Asset 'RuffnecKk-demo-v1.2.3.zip'
    $workspaceRefused = $false
    try { & $scriptPath -RegistryPath $registryPath -WorkspaceRoot $workspace -SuiteRoot $suite 2>$null | Out-Null }
    catch { $workspaceRefused = $_.Exception.Message -match 'authoritative source is not suite' }
    if (-not $workspaceRefused) { throw 'Package-ready registry accepted a workspace source.' }

    Write-Registry -SourceRef 'suite:plugins/demo' -Asset 'RuffnecKk-demo-v1.2.2.zip'
    $versionRefused = $false
    try { & $scriptPath -RegistryPath $registryPath -WorkspaceRoot $workspace -SuiteRoot $suite 2>$null | Out-Null }
    catch { $versionRefused = $_.Exception.Message -match 'does not end with the declared version' }
    if (-not $versionRefused) { throw 'Registry accepted an asset/version mismatch.' }

    Write-Registry -SourceRef 'suite:plugins/demo' -Asset 'RuffnecKk-demo-v1.2.3.zip'
    $valid = & $scriptPath -RegistryPath $registryPath -WorkspaceRoot $workspace -SuiteRoot $suite -RequirePackageReady
    if (-not $valid.valid) { throw 'Valid promoted registry was rejected.' }

    Write-Registry -SourceRef 'suite:plugins/demo' -Asset 'RuffnecKk-demo-v1.2.3.zip' -Status published -ReleaseReady $true
    $published = & $scriptPath -RegistryPath $registryPath -WorkspaceRoot $workspace -SuiteRoot $suite -RequirePackageReady
    if (-not $published.valid) { throw 'Valid published registry was rejected.' }

    Write-Registry -SourceRef 'workspace:addons/Demo' -Asset 'RuffnecKk-demo-v1.2.3.zip' -Status planning -ReleaseReady $false
    $planning = & $scriptPath -RegistryPath $registryPath -WorkspaceRoot $workspace -SuiteRoot $suite
    if (-not $planning.valid) { throw 'Planning registry incorrectly required promotion.' }

    Write-Output 'VALID : Suite governance promotion/asset-version/package-ready gates'
} finally {
    $resolvedTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    $resolvedTest = [IO.Path]::GetFullPath($testRoot)
    if (-not $resolvedTest.StartsWith($resolvedTemp, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove a test directory outside TEMP: $resolvedTest"
    }
    if (Test-Path -LiteralPath $resolvedTest) { Remove-Item -LiteralPath $resolvedTest -Recurse -Force }
}
