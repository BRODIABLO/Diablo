[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][ValidateNotNullOrEmpty()][string]$RegistryPath,
    [string]$WorkspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path,
    [string]$SuiteRoot,
    [switch]$RequirePackageReady
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Read-JsonDocument {
    param([Parameter(Mandatory)][string]$LiteralPath, [Parameter(Mandatory)][string]$Label)
    if (-not (Test-Path -LiteralPath $LiteralPath -PathType Leaf)) { throw "$Label not found: $LiteralPath" }
    try { return [IO.File]::ReadAllText([IO.Path]::GetFullPath($LiteralPath), [Text.Encoding]::UTF8) | ConvertFrom-Json }
    catch { throw "Invalid $Label JSON '$LiteralPath': $($_.Exception.Message)" }
}

function Resolve-WithinRoot {
    param([Parameter(Mandatory)][string]$Root, [Parameter(Mandatory)][string]$RelativePath, [Parameter(Mandatory)][string]$Label)
    if ([IO.Path]::IsPathRooted($RelativePath)) { throw "$Label must be relative: $RelativePath" }
    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar)
    $candidate = [IO.Path]::GetFullPath((Join-Path $rootPath $RelativePath))
    if ($candidate -ne $rootPath -and -not $candidate.StartsWith($rootPath + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw "$Label escapes its repository: $RelativePath"
    }
    return $candidate
}

function Resolve-PublicSuiteRoot {
    if (-not [string]::IsNullOrWhiteSpace($SuiteRoot)) { return [IO.Path]::GetFullPath($SuiteRoot) }
    if (-not [string]::IsNullOrWhiteSpace($env:RUFFNECKK_SUITE_ROOT)) { return [IO.Path]::GetFullPath($env:RUFFNECKK_SUITE_ROOT) }
    $configuration = Read-JsonDocument -LiteralPath (Join-Path $WorkspaceRoot 'workspace-repositories.json') -Label 'workspace repository configuration'
    $entry = @($configuration.repositories | Where-Object { [string]$_.id -eq 'suite' })
    if ($entry.Count -ne 1) { throw "workspace-repositories.json must define exactly one 'suite' repository." }
    return [IO.Path]::GetFullPath((Join-Path $WorkspaceRoot ([string]$entry[0].path)))
}

$registry = Read-JsonDocument -LiteralPath $RegistryPath -Label 'next-release registry'
$resolvedSuiteRoot = Resolve-PublicSuiteRoot
$errors = [Collections.Generic.List[string]]::new()
$status = [string]$registry.suite.status
$releaseReady = [bool]$registry.suite.releaseReady
$packageReady = $status -in 'package-ready', 'published' -or $releaseReady

if ($RequirePackageReady -and ($status -notin 'package-ready', 'published' -or -not $releaseReady)) {
    $null = $errors.Add('RequirePackageReady needs suite.status=package-ready/published and suite.releaseReady=true.')
}
if (($status -in 'package-ready', 'published') -ne $releaseReady) {
    $null = $errors.Add('suite.status package-ready/published and suite.releaseReady=true must change together.')
}

foreach ($component in @($registry.components)) {
    $id = [string]$component.id
    $sourceRef = [string]$component.sourceRef
    $included = [string]$component.disposition -eq 'include'
    $packagingPassed = [string]$component.gates.packaging -eq 'passed'
    if ($sourceRef -notmatch '^(suite|workspace):.+') {
        $null = $errors.Add("$id has an unsupported sourceRef '$sourceRef'.")
        continue
    }
    if ($packageReady -and $included -and $packagingPassed -and -not $sourceRef.StartsWith('suite:', [StringComparison]::Ordinal)) {
        $null = $errors.Add("$id is package-ready but its authoritative source is not suite: '$sourceRef'.")
    }
    if ($included -and $sourceRef.StartsWith('suite:', [StringComparison]::Ordinal)) {
        $relative = $sourceRef.Substring('suite:'.Length).Replace('/', [IO.Path]::DirectorySeparatorChar)
        try {
            $resolvedSource = Resolve-WithinRoot -Root $resolvedSuiteRoot -RelativePath $relative -Label "$id sourceRef"
            if (-not (Test-Path -LiteralPath $resolvedSource)) { $null = $errors.Add("$id suite sourceRef does not exist: $sourceRef") }
        } catch { $null = $errors.Add($_.Exception.Message) }
    } elseif ($included -and $sourceRef.StartsWith('workspace:', [StringComparison]::Ordinal)) {
        $relative = $sourceRef.Substring('workspace:'.Length).Replace('/', [IO.Path]::DirectorySeparatorChar)
        try {
            $resolvedSource = Resolve-WithinRoot -Root $WorkspaceRoot -RelativePath $relative -Label "$id sourceRef"
            if (-not (Test-Path -LiteralPath $resolvedSource)) { $null = $errors.Add("$id workspace sourceRef does not exist: $sourceRef") }
        } catch { $null = $errors.Add($_.Exception.Message) }
    }

    $targetVersion = if ($component.PSObject.Properties.Name -contains 'targetVersion') { [string]$component.targetVersion } else { '' }
    $asset = if ($component.PSObject.Properties.Name -contains 'asset') { [string]$component.asset } else { '' }
    if ($included -and [string]$component.kind -eq 'plugin' -and -not [string]::IsNullOrWhiteSpace($targetVersion) -and -not [string]::IsNullOrWhiteSpace($asset)) {
        $expectedSuffix = "-v$targetVersion.zip"
        if (-not $asset.EndsWith($expectedSuffix, [StringComparison]::Ordinal)) {
            $null = $errors.Add("$id asset '$asset' does not end with the declared version '$expectedSuffix'.")
        }
    }
}

if ($errors.Count -ne 0) {
    throw ("INVALID Suite governance:`n- " + ($errors -join "`n- "))
}

[pscustomobject]@{
    valid = $true
    registry = [IO.Path]::GetFullPath($RegistryPath)
    suiteRoot = $resolvedSuiteRoot
    status = $status
    releaseReady = $releaseReady
    components = @($registry.components).Count
}
