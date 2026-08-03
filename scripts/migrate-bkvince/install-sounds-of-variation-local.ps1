[CmdletBinding()]
param(
    [ValidateSet('Validate', 'Apply')]
    [string]$Mode = 'Validate',

    [string]$MainArchive = (Join-Path $env:USERPROFILE 'Downloads\The Sounds of Variation (Sounds)-287-v0-1g-1673569945.rar'),

    [string]$FixArchive = (Join-Path $env:USERPROFILE 'Downloads\The Sounds of Variation - Sounds (FIX for no drops bug)-287-v0-1g-fix-1675802605.rar'),

    [string]$D2RRoot = 'C:\Games\Diablo II Resurrected',

    [string]$SevenZip = 'C:\Program Files\7-Zip\7z.exe'
)

$ErrorActionPreference = 'Stop'
$MainArchiveSha256 = '3787E81A38BBC1959A2816EAA3E83B7B79F69E1931DD41095358631338E7699E'
$FixArchiveSha256 = '537E238BE4289FA5DBBD36E7D32947A1743652C605DCFFEC5296C2E9C18E0E28'
$ExpectedAudioCount = 1228
$WorkspaceRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$MigrationScript = Join-Path $PSScriptRoot 'integrate-sounds-of-variation.js'
$RuntimeSfxRoot = Join-Path $D2RRoot 'mods\BKVince\BKVince.mpq\data\hd\global\sfx'
$ReportBase = Join-Path $WorkspaceRoot 'analysis-cache\runtime-sync\sounds-of-variation'
$TempBase = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$TempRoot = Join-Path $TempBase ("ruffneckk-sounds-of-variation-{0}" -f [guid]::NewGuid().ToString('N'))

function Assert-Condition {
    param(
        [bool]$Condition,
        [string]$Message
    )
    if (-not $Condition) {
        throw $Message
    }
}

function Get-ExactSha256 {
    param([string]$LiteralPath)
    return (Get-FileHash -LiteralPath $LiteralPath -Algorithm SHA256).Hash.ToUpperInvariant()
}

function Test-Archive {
    param([string]$LiteralPath)
    $output = & $SevenZip t $LiteralPath -y 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "7-Zip archive test failed for $LiteralPath`n$($output -join [Environment]::NewLine)"
    }
}

function Expand-ArchiveWithSevenZip {
    param(
        [string]$LiteralPath,
        [string]$Destination
    )
    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    $output = & $SevenZip x $LiteralPath "-o$Destination" -y 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "7-Zip extraction failed for $LiteralPath`n$($output -join [Environment]::NewLine)"
    }
}

function Find-ModDataRoot {
    param(
        [string]$ExtractionRoot,
        [ValidateSet('Main', 'Fix')]
        [string]$ArchiveKind
    )
    $candidates = @(Get-ChildItem -LiteralPath $ExtractionRoot -Directory -Recurse | Where-Object {
        $_.Name -ieq 'data' -and
        (Test-Path -LiteralPath (Join-Path $_.FullName 'global\excel\sounds.txt')) -and
        (Test-Path -LiteralPath (Join-Path $_.FullName 'global\excel\monstats.txt'))
    })
    if ($ArchiveKind -eq 'Main') {
        $candidates = @($candidates | Where-Object {
            (Test-Path -LiteralPath (Join-Path $_.FullName 'global\excel\monsounds.txt')) -and
            (Test-Path -LiteralPath (Join-Path $_.FullName 'hd\global\sfx'))
        })
    }
    Assert-Condition ($candidates.Count -eq 1) "Expected one $ArchiveKind mod data root, found $($candidates.Count)."
    return $candidates[0].FullName
}

function Test-FlacSignature {
    param([string]$LiteralPath)
    $stream = [System.IO.File]::OpenRead($LiteralPath)
    try {
        $signature = New-Object byte[] 4
        $read = $stream.Read($signature, 0, 4)
        return $read -eq 4 -and [System.Text.Encoding]::ASCII.GetString($signature) -ceq 'fLaC'
    }
    finally {
        $stream.Dispose()
    }
}

function Get-SafeDestination {
    param(
        [string]$Root,
        [string]$RelativePath
    )
    $rootFull = [System.IO.Path]::GetFullPath($Root).TrimEnd('\') + '\'
    $destination = [System.IO.Path]::GetFullPath((Join-Path $Root $RelativePath))
    Assert-Condition $destination.StartsWith($rootFull, [System.StringComparison]::OrdinalIgnoreCase) "Destination escapes runtime root: $destination"
    return $destination
}

function Get-SafeRelativePath {
    param(
        [string]$Root,
        [string]$Child
    )
    $rootFull = [System.IO.Path]::GetFullPath($Root).TrimEnd('\') + '\'
    $childFull = [System.IO.Path]::GetFullPath($Child)
    Assert-Condition $childFull.StartsWith($rootFull, [System.StringComparison]::OrdinalIgnoreCase) "Path is not below source root: $childFull"
    return $childFull.Substring($rootFull.Length)
}

Assert-Condition (Test-Path -LiteralPath $MainArchive -PathType Leaf) "Main archive missing: $MainArchive"
Assert-Condition (Test-Path -LiteralPath $FixArchive -PathType Leaf) "Fix archive missing: $FixArchive"
Assert-Condition (Test-Path -LiteralPath $SevenZip -PathType Leaf) "7-Zip missing: $SevenZip"
Assert-Condition (Test-Path -LiteralPath $MigrationScript -PathType Leaf) "Migration script missing: $MigrationScript"
Assert-Condition ($null -ne (Get-Command node -ErrorAction SilentlyContinue)) 'Node.js is required.'

$mainHash = Get-ExactSha256 $MainArchive
$fixHash = Get-ExactSha256 $FixArchive
Assert-Condition ($mainHash -ceq $MainArchiveSha256) "Unexpected main archive SHA-256: $mainHash"
Assert-Condition ($fixHash -ceq $FixArchiveSha256) "Unexpected fix archive SHA-256: $fixHash"

Test-Archive $MainArchive
Test-Archive $FixArchive
New-Item -ItemType Directory -Path $TempRoot -Force | Out-Null

try {
    $mainExtraction = Join-Path $TempRoot 'main'
    $fixExtraction = Join-Path $TempRoot 'fix'
    Expand-ArchiveWithSevenZip $MainArchive $mainExtraction
    Expand-ArchiveWithSevenZip $FixArchive $fixExtraction
    $mainDataRoot = Find-ModDataRoot $mainExtraction 'Main'
    $fixDataRoot = Find-ModDataRoot $fixExtraction 'Fix'
    $sourceSfxRoot = Join-Path $mainDataRoot 'hd\global\sfx'
    $audioFiles = @(Get-ChildItem -LiteralPath $sourceSfxRoot -File -Recurse -Filter '*.flac' | Sort-Object FullName)
    Assert-Condition ($audioFiles.Count -eq $ExpectedAudioCount) "Expected $ExpectedAudioCount FLAC files, found $($audioFiles.Count)."
    foreach ($audioFile in $audioFiles) {
        Assert-Condition (Test-FlacSignature $audioFile.FullName) "Invalid FLAC signature: $($audioFile.FullName)"
    }

    $migrationArguments = @(
        $MigrationScript,
        '--main-root', $mainDataRoot,
        '--fix-root', $fixDataRoot
    )
    if ($Mode -eq 'Validate') {
        $migrationArguments += '--check'
    }
    & node @migrationArguments
    Assert-Condition ($LASTEXITCODE -eq 0) 'The selective TSV migration failed.'

    $pending = [System.Collections.Generic.List[object]]::new()
    foreach ($audioFile in $audioFiles) {
        $relativePath = Get-SafeRelativePath $sourceSfxRoot $audioFile.FullName
        $destination = Get-SafeDestination $RuntimeSfxRoot $relativePath
        $sourceHash = Get-ExactSha256 $audioFile.FullName
        $destinationHash = if (Test-Path -LiteralPath $destination -PathType Leaf) {
            Get-ExactSha256 $destination
        } else {
            $null
        }
        if ($destinationHash -cne $sourceHash) {
            $pending.Add([pscustomobject]@{
                RelativePath = $relativePath.Replace('\', '/')
                SourcePath = $audioFile.FullName
                DestinationPath = $destination
                SourceSha256 = $sourceHash
                PreviousSha256 = $destinationHash
            })
        }
    }

    $timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $reportRoot = Join-Path $ReportBase $timestamp
    $backupRoot = Join-Path $reportRoot 'backup'
    $stoppedProcesses = @()
    $backedUp = 0
    $copied = 0

    if ($Mode -eq 'Apply' -and $pending.Count -gt 0) {
        $running = @(Get-Process -Name 'D2R', 'D2RLauncher' -ErrorAction SilentlyContinue)
        foreach ($process in $running) {
            $stoppedProcesses += [pscustomobject]@{ Name = $process.ProcessName; Id = $process.Id }
            Stop-Process -Id $process.Id -Force
        }
        if ($running.Count -gt 0) {
            Wait-Process -Id $running.Id -Timeout 15 -ErrorAction SilentlyContinue
        }
        New-Item -ItemType Directory -Path $reportRoot -Force | Out-Null
        foreach ($item in $pending) {
            $relativeWindows = $item.RelativePath.Replace('/', '\')
            if ($null -ne $item.PreviousSha256) {
                $backupPath = Get-SafeDestination $backupRoot $relativeWindows
                New-Item -ItemType Directory -Path (Split-Path -Parent $backupPath) -Force | Out-Null
                Copy-Item -LiteralPath $item.DestinationPath -Destination $backupPath -Force
                $backedUp += 1
            }
            New-Item -ItemType Directory -Path (Split-Path -Parent $item.DestinationPath) -Force | Out-Null
            Copy-Item -LiteralPath $item.SourcePath -Destination $item.DestinationPath -Force
            $installedHash = Get-ExactSha256 $item.DestinationPath
            Assert-Condition ($installedHash -ceq $item.SourceSha256) "Runtime hash mismatch: $($item.DestinationPath)"
            $copied += 1
        }
    }

    $installed = 0
    $mismatched = [System.Collections.Generic.List[string]]::new()
    foreach ($audioFile in $audioFiles) {
        $relativePath = Get-SafeRelativePath $sourceSfxRoot $audioFile.FullName
        $destination = Get-SafeDestination $RuntimeSfxRoot $relativePath
        if (-not (Test-Path -LiteralPath $destination -PathType Leaf)) {
            $mismatched.Add($relativePath.Replace('\', '/'))
            continue
        }
        if ((Get-ExactSha256 $destination) -cne (Get-ExactSha256 $audioFile.FullName)) {
            $mismatched.Add($relativePath.Replace('\', '/'))
            continue
        }
        $installed += 1
    }
    if ($Mode -eq 'Apply') {
        Assert-Condition ($mismatched.Count -eq 0) "Runtime audio verification failed for $($mismatched.Count) files."
    }

    $report = [ordered]@{
        SchemaVersion = 1
        GeneratedAt = (Get-Date).ToString('o')
        Mode = $Mode
        MainArchive = [ordered]@{ Path = $MainArchive; Sha256 = $mainHash }
        FixArchive = [ordered]@{ Path = $FixArchive; Sha256 = $fixHash }
        RuntimeSfxRoot = $RuntimeSfxRoot
        Audio = [ordered]@{
            Expected = $ExpectedAudioCount
            Found = $audioFiles.Count
            PendingBeforeApply = $pending.Count
            Copied = $copied
            BackedUp = $backedUp
            InstalledAndHashExact = $installed
            MissingOrMismatched = $mismatched.Count
        }
        StoppedProcesses = $stoppedProcesses
        PendingFiles = $pending
        MismatchedFiles = $mismatched
    }
    if ($Mode -eq 'Apply') {
        New-Item -ItemType Directory -Path $reportRoot -Force | Out-Null
        $reportPath = Join-Path $reportRoot 'report.json'
        $json = $report | ConvertTo-Json -Depth 8
        [System.IO.File]::WriteAllText($reportPath, $json + [Environment]::NewLine, [System.Text.UTF8Encoding]::new($false))
        Write-Host "Report: $reportPath"
    }
    [ordered]@{
        Mode = $Mode
        MainArchiveSha256 = $mainHash
        FixArchiveSha256 = $fixHash
        ExpectedAudio = $ExpectedAudioCount
        FoundAudio = $audioFiles.Count
        PendingBeforeApply = $pending.Count
        Copied = $copied
        BackedUp = $backedUp
        InstalledAndHashExact = $installed
        MissingOrMismatched = $mismatched.Count
        StoppedProcesses = $stoppedProcesses.Count
    } | ConvertTo-Json
}
finally {
    $resolvedTempRoot = [System.IO.Path]::GetFullPath($TempRoot)
    $requiredPrefix = $TempBase.TrimEnd('\') + '\ruffneckk-sounds-of-variation-'
    if ((Test-Path -LiteralPath $resolvedTempRoot) -and
        $resolvedTempRoot.StartsWith($requiredPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        Remove-Item -LiteralPath $resolvedTempRoot -Recurse -Force
    }
}
