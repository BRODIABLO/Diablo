[CmdletBinding()]
param(
    [ValidateSet('Plan', 'Apply', 'Verify', 'Rollback')][string]$Mode = 'Plan',
    [string]$AllowlistPath,
    [string]$ArtifactRoot,
    [string]$SuiteRoot,
    [Parameter(Mandatory = $true)][ValidateNotNullOrEmpty()][string]$GameRoot,
    [Parameter(Mandatory = $true)][ValidatePattern('^[A-Za-z0-9._-]+$')][string]$Profile,
    [ValidateSet('Global', 'ModLocal')][string]$Scope = 'ModLocal',
    [ValidatePattern('^[A-Za-z0-9._-]+$')][string]$ModName = 'BKVince',
    [string[]]$ComponentId = @(),
    [string]$RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path,
    [string]$ReceiptPath,
    [switch]$OverwriteConfiguration,
    [switch]$ReplaceManagedSet,
    [switch]$ConfirmRuntimeControl
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$runtimeKinds = @(
    'plugin-dll',
    'plugin-companion-exe',
    'loose-config-json',
    'plugin-config-toml',
    'memory-patch-json'
)
$configurationKinds = @('loose-config-json', 'plugin-config-toml')

function Get-Sha256 {
    param([Parameter(Mandatory)][string]$LiteralPath)
    $stream = [IO.File]::OpenRead($LiteralPath)
    try {
        $algorithm = [Security.Cryptography.SHA256]::Create()
        try {
            return ([BitConverter]::ToString($algorithm.ComputeHash($stream))).Replace('-', '')
        } finally {
            $algorithm.Dispose()
        }
    } finally {
        $stream.Dispose()
    }
}

function Resolve-WithinRoot {
    param(
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)][string]$RelativePath,
        [Parameter(Mandatory)][string]$Label
    )
    if ([IO.Path]::IsPathRooted($RelativePath)) { throw "$Label must be relative: $RelativePath" }
    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar)
    $candidate = [IO.Path]::GetFullPath((Join-Path $rootPath $RelativePath))
    if ($candidate -ne $rootPath -and -not $candidate.StartsWith($rootPath + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw "$Label escapes its root: $RelativePath"
    }
    return $candidate
}

function Read-JsonDocument {
    param([Parameter(Mandatory)][string]$LiteralPath, [Parameter(Mandatory)][string]$Label)
    if (-not (Test-Path -LiteralPath $LiteralPath -PathType Leaf)) { throw "$Label not found: $LiteralPath" }
    try { return [IO.File]::ReadAllText([IO.Path]::GetFullPath($LiteralPath), [Text.Encoding]::UTF8) | ConvertFrom-Json }
    catch { throw "Invalid $Label JSON '$LiteralPath': $($_.Exception.Message)" }
}

function Write-JsonDocument {
    param([Parameter(Mandatory)][string]$LiteralPath, [Parameter(Mandatory)]$Value)
    $parent = [IO.Path]::GetDirectoryName($LiteralPath)
    if (-not (Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
    $temporary = "$LiteralPath.$PID.tmp"
    [IO.File]::WriteAllText($temporary, (($Value | ConvertTo-Json -Depth 20) + [Environment]::NewLine), [Text.UTF8Encoding]::new($false))
    Move-Item -LiteralPath $temporary -Destination $LiteralPath -Force
}

function Resolve-SuiteRoot {
    if (-not [string]::IsNullOrWhiteSpace($SuiteRoot)) { return [IO.Path]::GetFullPath($SuiteRoot) }
    if (-not [string]::IsNullOrWhiteSpace($env:RUFFNECKK_SUITE_ROOT)) { return [IO.Path]::GetFullPath($env:RUFFNECKK_SUITE_ROOT) }
    $configPath = Join-Path $RepositoryRoot 'workspace-repositories.json'
    $config = Read-JsonDocument -LiteralPath $configPath -Label 'workspace repository configuration'
    $entry = @($config.repositories | Where-Object { [string]$_.id -eq 'suite' })
    if ($entry.Count -ne 1) { throw "workspace-repositories.json must define exactly one 'suite' repository." }
    return [IO.Path]::GetFullPath((Join-Path $RepositoryRoot ([string]$entry[0].path)))
}

function Get-RuntimeRoot {
    $resolvedGame = [IO.Path]::GetFullPath($GameRoot)
    if (-not (Test-Path -LiteralPath (Join-Path $resolvedGame 'D2R.exe') -PathType Leaf)) {
        throw "GameRoot does not contain D2R.exe: $resolvedGame"
    }
    if ($Scope -eq 'Global') { return Join-Path $resolvedGame 'd2rloader' }
    return Join-Path $resolvedGame ("mods\{0}\d2rloader" -f $ModName)
}

function Get-OppositeRuntimeRoot {
    $resolvedGame = [IO.Path]::GetFullPath($GameRoot)
    if ($Scope -eq 'Global') { return Join-Path $resolvedGame ("mods\{0}\d2rloader" -f $ModName) }
    return Join-Path $resolvedGame 'd2rloader'
}

function Get-OwnedRuntimeProcesses {
    $resolvedGame = [IO.Path]::GetFullPath($GameRoot).TrimEnd('\') + '\'
    $owned = [Collections.Generic.List[object]]::new()
    foreach ($process in @(Get-Process -Name 'D2R', 'D2RLauncher' -ErrorAction SilentlyContinue)) {
        $processPath = $null
        try { $processPath = $process.Path } catch { $processPath = $null }
        if ([string]::IsNullOrWhiteSpace($processPath)) {
            throw "Cannot establish ownership of running process $($process.ProcessName) ($($process.Id))."
        }
        if ([IO.Path]::GetFullPath($processPath).StartsWith($resolvedGame, [StringComparison]::OrdinalIgnoreCase)) {
            $null = $owned.Add($process)
        }
    }
    return @($owned)
}

function Stop-OwnedRuntimeProcesses {
    $owned = @(Get-OwnedRuntimeProcesses)
    foreach ($process in $owned) { Stop-Process -Id $process.Id -Force }
    if ($owned.Count -ne 0) {
        Start-Sleep -Milliseconds 250
        $remaining = @(Get-OwnedRuntimeProcesses)
        if ($remaining.Count -ne 0) { throw 'Owned D2R processes are still running after stop.' }
    }
    return @($owned | ForEach-Object { [pscustomobject]@{ id = $_.Id; name = $_.ProcessName } })
}

function Resolve-EntrySource {
    param([Parameter(Mandatory)]$Entry, [Parameter(Mandatory)][string]$ResolvedSuiteRoot)
    $source = ([string]$Entry.source).Replace('/', [IO.Path]::DirectorySeparatorChar)
    $expected = ([string]$Entry.sha256).ToUpperInvariant()
    if ($expected -notmatch '^[0-9A-F]{64}$') { throw "Invalid allowlist SHA-256 for '$($Entry.source)'." }
    $roots = [Collections.Generic.List[string]]::new()
    if (-not [string]::IsNullOrWhiteSpace($ArtifactRoot)) { $null = $roots.Add([IO.Path]::GetFullPath($ArtifactRoot)) }
    $null = $roots.Add($ResolvedSuiteRoot)
    $observed = [Collections.Generic.List[string]]::new()
    foreach ($root in @($roots | Select-Object -Unique)) {
        $candidate = Resolve-WithinRoot -Root $root -RelativePath $source -Label 'Allowlist source'
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            $actual = Get-Sha256 -LiteralPath $candidate
            $null = $observed.Add("$candidate=$actual")
            if ($actual -eq $expected) { return $candidate }
        }
    }
    if ($observed.Count -eq 0) { throw "Allowlist source not found in ArtifactRoot or SuiteRoot: $($Entry.source)" }
    throw "No source matches the allowlist SHA-256 for '$($Entry.source)': $($observed -join '; ')"
}

function New-DeploymentPlan {
    if ([string]::IsNullOrWhiteSpace($AllowlistPath)) { throw 'AllowlistPath is required for Plan, Apply and Verify.' }
    $resolvedAllowlist = [IO.Path]::GetFullPath($AllowlistPath)
    $allowlist = Read-JsonDocument -LiteralPath $resolvedAllowlist -Label 'release allowlist'
    $resolvedSuiteRoot = Resolve-SuiteRoot
    $runtimeRoot = Get-RuntimeRoot
    $oppositeRoot = Get-OppositeRuntimeRoot
    $entries = @($allowlist.entries | Where-Object { [string]$_.kind -in $runtimeKinds })
    if ($ComponentId.Count -ne 0) {
        $entries = @($entries | Where-Object {
            $_.PSObject.Properties.Name -contains 'componentId' -and [string]$_.componentId -in $ComponentId
        })
        $missingIds = @($ComponentId | Where-Object { $_ -notin @($entries | ForEach-Object { [string]$_.componentId }) })
        if ($missingIds.Count -ne 0) { throw "ComponentId not found among deployable entries: $($missingIds -join ', ')" }
    }
    if ($entries.Count -eq 0) { throw 'The allowlist selection contains no runtime-deployable entries.' }

    $destinations = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $planned = [Collections.Generic.List[object]]::new()
    $collisions = [Collections.Generic.List[object]]::new()
    foreach ($entry in $entries) {
        $destination = ([string]$entry.destination).Replace('/', [IO.Path]::DirectorySeparatorChar)
        if (-not $destinations.Add($destination)) { throw "Duplicate runtime destination in allowlist selection: $destination" }
        $sourcePath = Resolve-EntrySource -Entry $entry -ResolvedSuiteRoot $resolvedSuiteRoot
        $targetPath = Resolve-WithinRoot -Root $runtimeRoot -RelativePath $destination -Label 'Runtime destination'
        $oppositePath = Resolve-WithinRoot -Root $oppositeRoot -RelativePath $destination -Label 'Opposite-scope destination'
        $beforeSha = if (Test-Path -LiteralPath $targetPath -PathType Leaf) { Get-Sha256 -LiteralPath $targetPath } else { $null }
        $expectedSha = ([string]$entry.sha256).ToUpperInvariant()
        $isConfiguration = [string]$entry.kind -in $configurationKinds
        $action = if ($beforeSha -eq $expectedSha) {
            'unchanged'
        } elseif ($isConfiguration -and $null -ne $beforeSha -and -not $OverwriteConfiguration) {
            'preserve-configuration'
        } elseif ($null -eq $beforeSha) {
            'create'
        } else {
            'replace'
        }
        if (-not $isConfiguration -and (Test-Path -LiteralPath $oppositePath -PathType Leaf)) {
            $null = $collisions.Add([pscustomobject]@{
                destination = ([string]$entry.destination).Replace('\', '/')
                selectedScope = $Scope
                oppositePath = $oppositePath
                oppositeSha256 = Get-Sha256 -LiteralPath $oppositePath
            })
        }
        $null = $planned.Add([pscustomobject]@{
            kind = [string]$entry.kind
            componentId = if ($entry.PSObject.Properties.Name -contains 'componentId') { [string]$entry.componentId } else { $null }
            source = $sourcePath
            destination = ([string]$entry.destination).Replace('\', '/')
            target = $targetPath
            expectedSha256 = $expectedSha
            beforeSha256 = $beforeSha
            action = $action
        })
    }

    return [pscustomobject]@{
        schemaVersion = 1
        mode = $Mode
        generatedAt = [DateTime]::UtcNow.ToString('o')
        profile = $Profile
        scope = $Scope
        modName = $ModName
        suiteVersion = [string]$allowlist.suite.version
        allowlist = $resolvedAllowlist
        allowlistSha256 = Get-Sha256 -LiteralPath $resolvedAllowlist
        suiteRoot = $resolvedSuiteRoot
        artifactRoot = if ([string]::IsNullOrWhiteSpace($ArtifactRoot)) { $null } else { [IO.Path]::GetFullPath($ArtifactRoot) }
        runtimeRoot = [IO.Path]::GetFullPath($runtimeRoot)
        collisions = @($collisions)
        entries = @($planned)
    }
}

function Get-StatePath {
    $stateRoot = Join-Path ([IO.Path]::GetFullPath($RepositoryRoot)) 'analysis-cache\runtime-deployments\suite\state'
    return Join-Path $stateRoot ("{0}-{1}.json" -f $Profile, $Scope.ToLowerInvariant())
}

function Invoke-Apply {
    if (-not $ConfirmRuntimeControl) { throw 'Apply requires -ConfirmRuntimeControl after explicit runtime authorization.' }
    $plan = New-DeploymentPlan
    if ($plan.collisions.Count -ne 0) {
        throw "Cross-scope duplicates detected; remove or relocate them before Apply: $(@($plan.collisions.destination) -join ', ')"
    }
    $stopped = @(Stop-OwnedRuntimeProcesses)
    $runId = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
    $runRoot = Join-Path ([IO.Path]::GetFullPath($RepositoryRoot)) ("analysis-cache\runtime-deployments\suite\{0}\{1}" -f $Profile, $runId)
    $backupRoot = Join-Path $runRoot 'backup'
    New-Item -ItemType Directory -Path $backupRoot -Force | Out-Null
    $statePath = Get-StatePath
    $stateBackup = $null
    $previousState = $null
    if (Test-Path -LiteralPath $statePath -PathType Leaf) {
        $previousState = Read-JsonDocument -LiteralPath $statePath -Label 'managed deployment state'
        $stateBackup = Join-Path $runRoot 'state-before.json'
        Copy-Item -LiteralPath $statePath -Destination $stateBackup
    }

    $changes = [Collections.Generic.List[object]]::new()
    foreach ($entry in $plan.entries) {
        $backupPath = $null
        if ($entry.action -eq 'replace') {
            $backupPath = Resolve-WithinRoot -Root $backupRoot -RelativePath $entry.destination -Label 'Backup destination'
            New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($backupPath)) -Force | Out-Null
            Copy-Item -LiteralPath $entry.target -Destination $backupPath
        }
        if ($entry.action -in 'create', 'replace') {
            New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($entry.target)) -Force | Out-Null
            Copy-Item -LiteralPath $entry.source -Destination $entry.target -Force
            $afterSha = Get-Sha256 -LiteralPath $entry.target
            if ($afterSha -ne $entry.expectedSha256) { throw "Post-copy hash mismatch: $($entry.target)" }
        } else {
            $afterSha = $entry.beforeSha256
        }
        $null = $changes.Add([pscustomobject]@{
            action = $entry.action
            kind = $entry.kind
            componentId = $entry.componentId
            destination = $entry.destination
            target = $entry.target
            source = $entry.source
            expectedSha256 = $entry.expectedSha256
            beforeSha256 = $entry.beforeSha256
            afterSha256 = $afterSha
            backup = $backupPath
        })
    }

    if ($ReplaceManagedSet -and $null -ne $previousState) {
        $desired = @($plan.entries.destination)
        foreach ($old in @($previousState.entries | Where-Object { [string]$_.destination -notin $desired })) {
            $target = Resolve-WithinRoot -Root $plan.runtimeRoot -RelativePath ([string]$old.destination) -Label 'Managed removal target'
            if (-not (Test-Path -LiteralPath $target -PathType Leaf)) { continue }
            $beforeSha = Get-Sha256 -LiteralPath $target
            if ($beforeSha -ne [string]$old.sha256) {
                throw "Refusing to remove locally modified managed file: $target"
            }
            $backupPath = Resolve-WithinRoot -Root $backupRoot -RelativePath ([string]$old.destination) -Label 'Removal backup destination'
            New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($backupPath)) -Force | Out-Null
            Copy-Item -LiteralPath $target -Destination $backupPath
            Remove-Item -LiteralPath $target -Force
            $null = $changes.Add([pscustomobject]@{
                action = 'remove'
                kind = [string]$old.kind
                componentId = [string]$old.componentId
                destination = [string]$old.destination
                target = $target
                source = $null
                expectedSha256 = $null
                beforeSha256 = $beforeSha
                afterSha256 = $null
                backup = $backupPath
            })
        }
    }

    $managedEntries = @($changes | Where-Object {
        $_.action -ne 'preserve-configuration' -and $_.action -ne 'remove'
    } | ForEach-Object {
        [pscustomobject]@{ destination = $_.destination; kind = $_.kind; componentId = $_.componentId; sha256 = $_.afterSha256 }
    })
    $state = [pscustomobject]@{
        schemaVersion = 1
        updatedAt = [DateTime]::UtcNow.ToString('o')
        profile = $Profile
        scope = $Scope
        suiteVersion = $plan.suiteVersion
        allowlistSha256 = $plan.allowlistSha256
        runtimeRoot = $plan.runtimeRoot
        entries = $managedEntries
    }
    Write-JsonDocument -LiteralPath $statePath -Value $state
    $receiptFile = Join-Path $runRoot 'receipt.json'
    $receipt = [pscustomobject]@{
        schemaVersion = 1
        operation = 'apply'
        completedAt = [DateTime]::UtcNow.ToString('o')
        receiptPath = $receiptFile
        statePath = $statePath
        stateBackup = $stateBackup
        profile = $Profile
        scope = $Scope
        suiteVersion = $plan.suiteVersion
        allowlist = $plan.allowlist
        allowlistSha256 = $plan.allowlistSha256
        runtimeRoot = $plan.runtimeRoot
        stoppedProcesses = $stopped
        changes = @($changes)
    }
    Write-JsonDocument -LiteralPath $receiptFile -Value $receipt
    return $receipt
}

function Invoke-Verify {
    $plan = New-DeploymentPlan
    $errors = [Collections.Generic.List[string]]::new()
    $warnings = [Collections.Generic.List[string]]::new()
    foreach ($collision in $plan.collisions) { $null = $errors.Add("Cross-scope duplicate: $($collision.destination)") }
    foreach ($entry in $plan.entries) {
        if (-not (Test-Path -LiteralPath $entry.target -PathType Leaf)) {
            $null = $errors.Add("Missing runtime file: $($entry.destination)")
            continue
        }
        $actual = Get-Sha256 -LiteralPath $entry.target
        if ($actual -ne $entry.expectedSha256) {
            if ($entry.kind -in $configurationKinds -and -not $OverwriteConfiguration) {
                $null = $warnings.Add("Preserved configuration differs from release default: $($entry.destination)")
            } else {
                $null = $errors.Add("Runtime hash mismatch: $($entry.destination)")
            }
        }
    }
    return [pscustomobject]@{
        schemaVersion = 1
        mode = 'Verify'
        checkedAt = [DateTime]::UtcNow.ToString('o')
        profile = $Profile
        scope = $Scope
        suiteVersion = $plan.suiteVersion
        valid = $errors.Count -eq 0
        errors = @($errors)
        warnings = @($warnings)
        entries = @($plan.entries)
    }
}

function Invoke-Rollback {
    if (-not $ConfirmRuntimeControl) { throw 'Rollback requires -ConfirmRuntimeControl after explicit runtime authorization.' }
    if ([string]::IsNullOrWhiteSpace($ReceiptPath)) { throw 'Rollback requires ReceiptPath.' }
    $resolvedReceipt = [IO.Path]::GetFullPath($ReceiptPath)
    $receipt = Read-JsonDocument -LiteralPath $resolvedReceipt -Label 'deployment receipt'
    if ([string]$receipt.operation -ne 'apply') { throw 'Rollback accepts only an apply receipt.' }
    $null = Stop-OwnedRuntimeProcesses
    $restored = [Collections.Generic.List[object]]::new()
    $changes = @($receipt.changes)
    [array]::Reverse($changes)
    foreach ($change in $changes) {
        $action = [string]$change.action
        if ($action -in 'unchanged', 'preserve-configuration') { continue }
        $target = [IO.Path]::GetFullPath([string]$change.target)
        if ($action -in 'create', 'replace') {
            if (-not (Test-Path -LiteralPath $target -PathType Leaf)) {
                $receiptSummary = @($receipt.changes | ForEach-Object { "{0}:{1}" -f $_.action, $_.destination }) -join ', '
                throw "Rollback target is missing: $target (action=$action; receipt=$receiptSummary)"
            }
            $currentSha = Get-Sha256 -LiteralPath $target
            if ($currentSha -ne [string]$change.afterSha256) { throw "Rollback refuses a locally modified file: $target" }
        }
        if ($action -eq 'create') {
            Remove-Item -LiteralPath $target -Force
        } elseif ($action -in 'replace', 'remove') {
            $backup = [string]$change.backup
            if (-not (Test-Path -LiteralPath $backup -PathType Leaf)) { throw "Rollback backup is missing: $backup" }
            New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($target)) -Force | Out-Null
            Copy-Item -LiteralPath $backup -Destination $target -Force
            if ((Get-Sha256 -LiteralPath $target) -ne [string]$change.beforeSha256) { throw "Rollback hash mismatch: $target" }
        }
        $null = $restored.Add([pscustomobject]@{ action = $action; target = $target })
    }
    $statePath = [string]$receipt.statePath
    $stateBackup = [string]$receipt.stateBackup
    if (-not [string]::IsNullOrWhiteSpace($stateBackup) -and (Test-Path -LiteralPath $stateBackup -PathType Leaf)) {
        New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($statePath)) -Force | Out-Null
        Copy-Item -LiteralPath $stateBackup -Destination $statePath -Force
    } elseif (-not [string]::IsNullOrWhiteSpace($statePath) -and (Test-Path -LiteralPath $statePath -PathType Leaf)) {
        Remove-Item -LiteralPath $statePath -Force
    }
    $rollbackPath = Join-Path ([IO.Path]::GetDirectoryName($resolvedReceipt)) 'rollback-receipt.json'
    $rollback = [pscustomobject]@{
        schemaVersion = 1
        operation = 'rollback'
        completedAt = [DateTime]::UtcNow.ToString('o')
        sourceReceipt = $resolvedReceipt
        rollbackReceiptPath = $rollbackPath
        restored = @($restored)
    }
    Write-JsonDocument -LiteralPath $rollbackPath -Value $rollback
    return $rollback
}

switch ($Mode) {
    'Plan' { New-DeploymentPlan }
    'Apply' { Invoke-Apply }
    'Verify' { Invoke-Verify }
    'Rollback' { Invoke-Rollback }
}
