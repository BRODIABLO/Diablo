[CmdletBinding()]
param(
    [Parameter()]
    [ValidatePattern('^[a-z0-9][a-z0-9-]{0,63}$')]
    [string]$Scenario = 'smoke-launch-save-exit',

    [Parameter()]
    [string]$ConfigPath = '',

    [Parameter()]
    [string]$ScenarioPath = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:ScriptPath = $MyInvocation.MyCommand.Path
$script:RunnerDirectory = Split-Path -Parent $script:ScriptPath
$script:RepositoryRoot = [IO.Path]::GetFullPath((Join-Path $script:RunnerDirectory '..\..'))
$script:PowerShellLogPath = $null
$script:RunNotes = New-Object 'System.Collections.Generic.List[string]'
$script:InvariantCulture = [Globalization.CultureInfo]::InvariantCulture
$script:TrustedGameDirectory = ''
$script:TestSaveMarkerFileName = '.game-test-owned.json'
$script:TestSaveMarkerId = 'RuffnecKk.GameTestRunner.test-save'
$script:TestSaveMarkerSchemaVersion = 1

function Resolve-AbsolutePath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$BasePath
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        throw 'A required path is empty.'
    }
    if ([IO.Path]::IsPathRooted($Path)) {
        return [IO.Path]::GetFullPath($Path)
    }
    return [IO.Path]::GetFullPath((Join-Path $BasePath $Path))
}

function Get-OptionalProperty {
    param(
        [Parameter(Mandatory = $true)]$Object,
        [Parameter(Mandatory = $true)][string]$Name,
        $DefaultValue = $null
    )

    if ($null -eq $Object) {
        return $DefaultValue
    }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $DefaultValue
    }
    return $property.Value
}

function Get-RequiredProperty {
    param(
        [Parameter(Mandatory = $true)]$Object,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Context
    )

    $value = Get-OptionalProperty -Object $Object -Name $Name
    if ($null -eq $value -or ($value -is [string] -and [string]::IsNullOrWhiteSpace($value))) {
        throw "Missing required property '$Name' in $Context."
    }
    return $value
}

function Write-RunLog {
    param(
        [Parameter(Mandatory = $true)][string]$Message,
        [ValidateSet('INFO', 'WARN', 'ERROR')][string]$Level = 'INFO'
    )

    $line = '{0} [{1}] {2}' -f ([DateTime]::UtcNow.ToString('o')), $Level, $Message
    Write-Host $line
    if ($null -ne $script:PowerShellLogPath) {
        [IO.File]::AppendAllText($script:PowerShellLogPath, $line + [Environment]::NewLine, (New-Object Text.UTF8Encoding($false)))
    }
}

function Write-JsonFile {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string]$Path
    )

    $json = $Value | ConvertTo-Json -Depth 64
    [IO.File]::WriteAllText($Path, $json + [Environment]::NewLine, (New-Object Text.UTF8Encoding($false)))
}

function Get-NormalizedDirectoryPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return [IO.Path]::GetFullPath($Path).TrimEnd([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar)
}

function Test-PathInsideDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Candidate,
        [Parameter(Mandatory = $true)][string]$Directory,
        [switch]$AllowEqual
    )

    $candidateFull = Get-NormalizedDirectoryPath $Candidate
    $directoryFull = Get-NormalizedDirectoryPath $Directory
    if ($AllowEqual -and $candidateFull.Equals($directoryFull, [StringComparison]::OrdinalIgnoreCase)) {
        return $true
    }
    $prefix = $directoryFull + [IO.Path]::DirectorySeparatorChar
    return $candidateFull.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)
}

function Assert-SafeLeafPath {
    param(
        [Parameter(Mandatory = $true)][string]$Candidate,
        [Parameter(Mandatory = $true)][string]$Parent,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $candidateFull = Get-NormalizedDirectoryPath $Candidate
    $parentFull = Get-NormalizedDirectoryPath $Parent
    if (-not (Test-PathInsideDirectory -Candidate $candidateFull -Directory $parentFull)) {
        throw "$Label is not strictly inside its expected parent directory."
    }
    if ($candidateFull.Equals($parentFull, [StringComparison]::OrdinalIgnoreCase)) {
        throw "$Label resolves to a broad parent directory."
    }
}

function Assert-NoReparsePoint {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $current = [IO.Path]::GetFullPath($Path)
    if (-not (Test-Path -LiteralPath $current)) {
        $current = Split-Path -Parent $current
    }
    while (-not [string]::IsNullOrWhiteSpace($current)) {
        $item = Get-Item -LiteralPath $current -Force -ErrorAction Stop
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "$Label crosses a reparse point or junction: $current"
        }
        $parent = Split-Path -Parent $current
        if ([string]::IsNullOrWhiteSpace($parent) -or $parent -eq $current) {
            break
        }
        $current = $parent
    }
}

function Replace-FileAtomically {
    param(
        [Parameter(Mandatory = $true)][string]$Destination,
        [Parameter(Mandatory = $true)][byte[]]$Bytes
    )

    $directory = Split-Path -Parent $Destination
    $temporary = Join-Path $directory ('.gametest-' + [Guid]::NewGuid().ToString('N') + '.tmp')
    $replacementBackup = Join-Path $directory ('.gametest-backup-' + [Guid]::NewGuid().ToString('N') + '.tmp')
    try {
        [IO.File]::WriteAllBytes($temporary, $Bytes)
        if (Test-Path -LiteralPath $Destination -PathType Leaf) {
            [IO.File]::Replace($temporary, $Destination, $replacementBackup, $true)
        }
        else {
            [IO.File]::Move($temporary, $Destination)
        }
    }
    finally {
        if (Test-Path -LiteralPath $temporary -PathType Leaf) {
            Remove-Item -LiteralPath $temporary -Force
        }
        if (Test-Path -LiteralPath $replacementBackup -PathType Leaf) {
            Remove-Item -LiteralPath $replacementBackup -Force
        }
    }
}

function Assert-TestSaveOwnershipMarker {
    param(
        [Parameter(Mandatory = $true)][string]$Directory,
        [Parameter(Mandatory = $true)][string]$TestSavePathName
    )

    $markerPath = Join-Path $Directory $script:TestSaveMarkerFileName
    if (-not (Test-Path -LiteralPath $markerPath -PathType Leaf)) {
        throw "The isolated test save directory does not contain a normal $($script:TestSaveMarkerFileName) ownership marker."
    }
    $markerItem = Get-Item -LiteralPath $markerPath -Force -ErrorAction Stop
    if ($markerItem.PSIsContainer -or ($markerItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "The isolated test save ownership marker is not a normal non-reparse file: $markerPath"
    }

    $markerObject = Get-Content -Raw -LiteralPath $markerPath -Encoding UTF8 | ConvertFrom-Json
    $markerProperties = @($markerObject.PSObject.Properties)
    $markerPropertyNames = @($markerProperties | ForEach-Object { $_.Name })
    $expectedPropertyNames = @('id', 'schemaVersion', 'savepath')
    if ($markerProperties.Count -ne $expectedPropertyNames.Count) {
        throw 'The isolated test save ownership marker has unexpected properties.'
    }
    foreach ($expectedPropertyName in $expectedPropertyNames) {
        if (-not ($markerPropertyNames -ccontains $expectedPropertyName)) {
            throw "The isolated test save ownership marker is missing exact property '$expectedPropertyName'."
        }
    }

    if ([string]$markerObject.id -cne $script:TestSaveMarkerId) {
        throw 'The isolated test save ownership marker identifier is invalid.'
    }
    $markerVersion = $markerObject.schemaVersion
    $integerTypes = @([byte], [sbyte], [int16], [uint16], [int32], [uint32], [int64], [uint64])
    if ($null -eq $markerVersion -or $integerTypes -notcontains $markerVersion.GetType() -or [string]$markerVersion -cne [string]$script:TestSaveMarkerSchemaVersion) {
        throw 'The isolated test save ownership marker schemaVersion is invalid.'
    }
    if ([string]$markerObject.savepath -cne ($TestSavePathName + '/')) {
        throw 'The isolated test save ownership marker savepath does not exactly match the configured test savepath.'
    }
}

function Assert-TestSaveDirectoryOwnershipState {
    param(
        [Parameter(Mandatory = $true)][string]$Directory,
        [Parameter(Mandatory = $true)][string]$TestSavePathName
    )

    if (-not (Test-Path -LiteralPath $Directory)) {
        return
    }
    if (-not (Test-Path -LiteralPath $Directory -PathType Container)) {
        throw "The isolated test save path exists but is not a directory: $Directory"
    }
    Assert-NoReparsePoint -Path $Directory -Label 'testSaveDirectory'

    $markerPath = Join-Path $Directory $script:TestSaveMarkerFileName
    if (Test-Path -LiteralPath $markerPath) {
        Assert-TestSaveOwnershipMarker -Directory $Directory -TestSavePathName $TestSavePathName
        return
    }

    $entries = @(Get-ChildItem -LiteralPath $Directory -Force -ErrorAction Stop)
    if ($entries.Count -gt 0) {
        throw "Refusing existing non-empty unowned test save directory: $Directory"
    }
}

function Ensure-TestSaveOwnershipMarker {
    param(
        [Parameter(Mandatory = $true)][string]$Directory,
        [Parameter(Mandatory = $true)][string]$TestSavePathName
    )

    [IO.Directory]::CreateDirectory($Directory) | Out-Null
    Assert-NoReparsePoint -Path $Directory -Label 'testSaveDirectory'
    $markerPath = Join-Path $Directory $script:TestSaveMarkerFileName
    if (Test-Path -LiteralPath $markerPath) {
        Assert-TestSaveOwnershipMarker -Directory $Directory -TestSavePathName $TestSavePathName
        return
    }

    $entries = @(Get-ChildItem -LiteralPath $Directory -Force -ErrorAction Stop)
    if ($entries.Count -gt 0) {
        throw "Refusing existing non-empty unowned test save directory: $Directory"
    }

    $markerValue = [ordered]@{
        id = $script:TestSaveMarkerId
        schemaVersion = $script:TestSaveMarkerSchemaVersion
        savepath = $TestSavePathName + '/'
    }
    $markerJson = ($markerValue | ConvertTo-Json -Depth 4) + [Environment]::NewLine
    $markerBytes = (New-Object Text.UTF8Encoding($false)).GetBytes($markerJson)
    Replace-FileAtomically -Destination $markerPath -Bytes $markerBytes
    Assert-TestSaveOwnershipMarker -Directory $Directory -TestSavePathName $TestSavePathName
}

function Get-DirectorySnapshot {
    param([Parameter(Mandatory = $true)][string]$Directory)

    $snapshot = [ordered]@{}
    if (-not (Test-Path -LiteralPath $Directory -PathType Container)) {
        return $snapshot
    }
    $root = Get-NormalizedDirectoryPath $Directory
    $files = Get-ChildItem -LiteralPath $root -File -Recurse -Force | Sort-Object FullName
    foreach ($file in $files) {
        $relative = $file.FullName.Substring($root.Length).TrimStart('\', '/')
        $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        $snapshot[$relative] = [ordered]@{
            length = [Int64]$file.Length
            sha256 = $hash
        }
    }
    return $snapshot
}

function Compare-Snapshot {
    param(
        [Parameter(Mandatory = $true)]$Before,
        [Parameter(Mandatory = $true)]$After
    )

    $beforeJson = $Before | ConvertTo-Json -Depth 8 -Compress
    $afterJson = $After | ConvertTo-Json -Depth 8 -Compress
    return $beforeJson -ceq $afterJson
}

function Get-Sha256Bytes {
    param([Parameter(Mandatory = $true)][byte[]]$Bytes)

    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString($sha.ComputeHash($Bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally {
        $sha.Dispose()
    }
}

function Get-AllowedProcessNames {
    param([Parameter(Mandatory = $true)]$Config)

    $names = @()
    foreach ($rawName in @(Get-RequiredProperty -Object $Config -Name 'processNames' -Context 'config')) {
        $name = ([string]$rawName).Trim()
        if ($name.EndsWith('.exe', [StringComparison]::OrdinalIgnoreCase)) {
            $name = [IO.Path]::GetFileNameWithoutExtension($name)
        }
        if ($name -notmatch '^[A-Za-z0-9_.-]+$') {
            throw "Unsafe process name in config: $rawName"
        }
        $names += $name
    }
    if ($names.Count -eq 0) {
        throw 'config.processNames must contain at least one executable name.'
    }
    $unique = @($names | Select-Object -Unique)
    foreach ($name in $unique) {
        if ($name -notin @('D2R', 'D2RLoader')) {
            throw "Process allowlist is fixed to D2R and D2RLoader; refused '$name'."
        }
    }
    return $unique
}

function Get-GameProcesses {
    param([Parameter(Mandatory = $true)][string[]]$Names)

    $result = @()
    foreach ($name in $Names) {
        $result += @(Get-Process -Name $name -ErrorAction SilentlyContinue)
    }
    $unique = @($result | Sort-Object Id -Unique)
    $verified = New-Object 'System.Collections.Generic.List[System.Diagnostics.Process]'
    if (-not [string]::IsNullOrWhiteSpace($script:TrustedGameDirectory)) {
        foreach ($process in $unique) {
            try {
                $process.Refresh()
                if ($process.HasExited) {
                    continue
                }
                $processPath = [string]$process.Path
            }
            catch {
                if ($null -eq (Get-Process -Id $process.Id -ErrorAction SilentlyContinue)) {
                    continue
                }
                $processPath = ''
            }
            if ([string]::IsNullOrWhiteSpace($processPath)) {
                $cimProcess = Get-CimInstance Win32_Process -Filter "ProcessId=$($process.Id)" -ErrorAction SilentlyContinue
                if ($null -eq $cimProcess) {
                    continue
                }
                $processPath = [string]$cimProcess.ExecutablePath
            }
            if ([string]::IsNullOrWhiteSpace($processPath) -or -not (Test-PathInsideDirectory -Candidate $processPath -Directory $script:TrustedGameDirectory)) {
                throw "Refusing process '$($process.ProcessName)' PID $($process.Id) outside the configured D2R working directory."
            }
            $verified.Add($process)
        }
        return @($verified)
    }
    return $unique
}

function Stop-GameProcesses {
    param(
        [Parameter(Mandatory = $true)][string[]]$Names,
        [int]$GracefulTimeoutMs = 10000
    )

    $processes = @(Get-GameProcesses -Names $Names)
    if ($processes.Count -eq 0) {
        return
    }

    Write-RunLog "Closing D2R process(es): $((@($processes.Id) -join ', '))."
    foreach ($process in $processes) {
        try {
            if ($process.MainWindowHandle -ne 0) {
                [void]$process.CloseMainWindow()
            }
        }
        catch {
            Write-RunLog "Graceful close request failed for PID $($process.Id): $($_.Exception.Message)" 'WARN'
        }
    }

    $deadline = [DateTime]::UtcNow.AddMilliseconds($GracefulTimeoutMs)
    do {
        Start-Sleep -Milliseconds 200
        $remaining = @(Get-GameProcesses -Names $Names)
    } while ($remaining.Count -gt 0 -and [DateTime]::UtcNow -lt $deadline)

    foreach ($process in @(Get-GameProcesses -Names $Names)) {
        Write-RunLog "Forcing isolated test process PID $($process.Id) to stop after the graceful timeout." 'WARN'
        Stop-Process -Id $process.Id -Force -ErrorAction Stop
    }

    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    do {
        Start-Sleep -Milliseconds 200
        $remaining = @(Get-GameProcesses -Names $Names)
    } while ($remaining.Count -gt 0 -and [DateTime]::UtcNow -lt $deadline)
    if ($remaining.Count -gt 0) {
        throw "D2R process(es) could not be stopped: $((@($remaining.Id) -join ', '))."
    }
}

function Wait-ForGameWindow {
    param(
        [Parameter(Mandatory = $true)][string[]]$Names,
        [Parameter(Mandatory = $true)][string]$WindowTitle,
        [Parameter(Mandatory = $true)][int]$TimeoutMs
    )

    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    do {
        $matches = @(
            Get-GameProcesses -Names $Names |
                Where-Object { $_.MainWindowHandle -ne 0 -and $_.MainWindowTitle -ceq $WindowTitle }
        )
        if ($matches.Count -eq 1) {
            return $matches[0]
        }
        if ($matches.Count -gt 1) {
            throw "More than one allowlisted process owns a '$WindowTitle' window."
        }
        Start-Sleep -Milliseconds 250
    } while ([DateTime]::UtcNow -lt $deadline)

    throw "Timed out waiting for the unique D2R window '$WindowTitle'."
}

function Convert-ToIniNumber {
    param([Parameter(Mandatory = $true)]$Value)

    return ([double]$Value).ToString('0.########', $script:InvariantCulture)
}

function Assert-NormalizedPoint {
    param(
        [Parameter(Mandatory = $true)]$Point,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $x = [double](Get-RequiredProperty -Object $Point -Name 'x' -Context $Label)
    $y = [double](Get-RequiredProperty -Object $Point -Name 'y' -Context $Label)
    if ($x -le 0 -or $x -ge 1 -or $y -le 0 -or $y -ge 1) {
        throw "$Label must be strictly inside normalized client coordinates."
    }
    return [ordered]@{ x = $x; y = $y }
}

function Get-NormalizedRegion {
    param(
        [Parameter(Mandatory = $true)]$Region,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $x = [double](Get-RequiredProperty -Object $Region -Name 'x' -Context $Label)
    $y = [double](Get-RequiredProperty -Object $Region -Name 'y' -Context $Label)
    $width = [double](Get-RequiredProperty -Object $Region -Name 'width' -Context $Label)
    $height = [double](Get-RequiredProperty -Object $Region -Name 'height' -Context $Label)
    if ($x -lt 0 -or $y -lt 0 -or $width -le 0 -or $height -le 0 -or ($x + $width) -gt 1 -or ($y + $height) -gt 1) {
        throw "$Label must be a normalized rectangle inside the D2R client."
    }
    return [ordered]@{ x1 = $x; y1 = $y; x2 = ($x + $width); y2 = ($y + $height) }
}

function Resolve-TemplatePath {
    param(
        [Parameter(Mandatory = $true)][AllowEmptyString()][string]$TemplatePath,
        [Parameter(Mandatory = $true)][string]$ConfigDirectory
    )

    if ([string]::IsNullOrWhiteSpace($TemplatePath)) {
        return ''
    }
    return Resolve-AbsolutePath -Path $TemplatePath -BasePath $ConfigDirectory
}

function Add-IniLine {
    param(
        [Parameter(Mandatory = $true)][Text.StringBuilder]$Builder,
        [Parameter(Mandatory = $true)][string]$Key,
        $Value = ''
    )

    $text = if ($null -eq $Value) { '' } else { [string]$Value }
    if ($text -match '[\r\n]') {
        throw "INI value '$Key' contains a line break."
    }
    [void]$Builder.AppendLine("$Key=$text")
}

function New-AhkPlan {
    param(
        [Parameter(Mandatory = $true)]$Config,
        [Parameter(Mandatory = $true)]$Profile,
        [Parameter(Mandatory = $true)]$ScenarioDefinition,
        [Parameter(Mandatory = $true)][string]$ConfigDirectory,
        [Parameter(Mandatory = $true)][string]$RunDirectory,
        [Parameter(Mandatory = $true)][string]$EventLogPath,
        [Parameter(Mandatory = $true)][string]$PlanPath
    )

    $expectedSize = Get-RequiredProperty -Object $Profile -Name 'expectedClientSize' -Context 'input profile'
    $keys = Get-RequiredProperty -Object $Profile -Name 'keys' -Context 'input profile'
    $coordinates = Get-RequiredProperty -Object $Profile -Name 'coordinates' -Context 'input profile'
    $characterSelection = Get-RequiredProperty -Object $Profile -Name 'characterSelection' -Context 'input profile'
    $stateProbes = Get-RequiredProperty -Object $Profile -Name 'stateProbes' -Context 'input profile'
    $timeouts = Get-RequiredProperty -Object $Profile -Name 'timeoutsMs' -Context 'input profile'
    $delays = Get-RequiredProperty -Object $Profile -Name 'delaysMs' -Context 'input profile'

    $play = Assert-NormalizedPoint -Point (Get-RequiredProperty -Object $coordinates -Name 'play' -Context 'coordinates') -Label 'coordinates.play'
    $saveAndExit = Assert-NormalizedPoint -Point (Get-RequiredProperty -Object $coordinates -Name 'saveAndExit' -Context 'coordinates') -Label 'coordinates.saveAndExit'
    $difficultyObject = Get-OptionalProperty -Object $coordinates -Name 'difficulty'
    $difficultyEnabled = $null -ne $difficultyObject
    if ($difficultyEnabled) {
        $difficulty = Assert-NormalizedPoint -Point $difficultyObject -Label 'coordinates.difficulty'
    }

    $characterTemplatePath = Resolve-TemplatePath -TemplatePath ([string](Get-OptionalProperty -Object $characterSelection -Name 'templatePath' -DefaultValue '')) -ConfigDirectory $ConfigDirectory
    $characterRegion = Get-NormalizedRegion -Region (Get-RequiredProperty -Object $characterSelection -Name 'searchRegion' -Context 'characterSelection') -Label 'characterSelection.searchRegion'
    $clickOffsetObject = Get-OptionalProperty -Object $characterSelection -Name 'clickOffset'
    $clickOffsetX = if ($null -eq $clickOffsetObject) { 0.0 } else { [double](Get-OptionalProperty -Object $clickOffsetObject -Name 'x' -DefaultValue 0.0) }
    $clickOffsetY = if ($null -eq $clickOffsetObject) { 0.0 } else { [double](Get-OptionalProperty -Object $clickOffsetObject -Name 'y' -DefaultValue 0.0) }
    if ([Math]::Abs($clickOffsetX) -gt 1 -or [Math]::Abs($clickOffsetY) -gt 1) {
        throw 'characterSelection.clickOffset must use normalized client units between -1 and 1.'
    }
    $fallbackObject = Get-OptionalProperty -Object $characterSelection -Name 'fallbackPosition'
    $fallback = $null
    if ($null -ne $fallbackObject) {
        $fallback = Assert-NormalizedPoint -Point $fallbackObject -Label 'characterSelection.fallbackPosition'
    }
    if ([string]::IsNullOrWhiteSpace($characterTemplatePath) -and $null -eq $fallback) {
        throw 'characterSelection requires a local templatePath or a calibrated fallbackPosition.'
    }

    $probeMap = [ordered]@{
        CharacterSelect = 'CHARACTER_SELECT'
        InGame = 'IN_GAME'
        EscMenu = 'ESC_MENU'
        Inventory = 'INVENTORY_OPEN'
    }
    $resolvedProbes = [ordered]@{}
    foreach ($entry in $probeMap.GetEnumerator()) {
        $probe = Get-RequiredProperty -Object $stateProbes -Name $entry.Value -Context 'stateProbes'
        $region = Get-NormalizedRegion -Region (Get-RequiredProperty -Object $probe -Name 'searchRegion' -Context "stateProbes.$($entry.Value)") -Label "stateProbes.$($entry.Value).searchRegion"
        $resolvedProbes[$entry.Key] = [ordered]@{
            path = Resolve-TemplatePath -TemplatePath ([string](Get-RequiredProperty -Object $probe -Name 'templatePath' -Context "stateProbes.$($entry.Value)")) -ConfigDirectory $ConfigDirectory
            region = $region
            variation = [int](Get-OptionalProperty -Object $probe -Name 'variation' -DefaultValue 20)
        }
    }

    $allowedActions = @(
        'launch_or_reuse_game', 'focus_game', 'select_character', 'enter_game',
        'wait_for_in_game', 'open_inventory', 'capture', 'close_inventory',
        'save_and_exit', 'wait_for_character_select'
    )
    $ahkSteps = @()
    foreach ($step in @($ScenarioDefinition.steps)) {
        $action = ([string](Get-RequiredProperty -Object $step -Name 'action' -Context 'scenario step')).ToLowerInvariant()
        if ($action -notin $allowedActions) {
            throw "Unsupported scenario action in v1: $action"
        }
        if ($action -ne 'launch_or_reuse_game') {
            $ahkSteps += $step
        }
    }
    if ($ahkSteps.Count -eq 0) {
        throw 'The scenario contains no AutoHotkey steps.'
    }

    $builder = New-Object Text.StringBuilder
    [void]$builder.AppendLine('[Runner]')
    Add-IniLine $builder 'ScenarioId' ([string]$ScenarioDefinition.id)
    Add-IniLine $builder 'RunDirectory' $RunDirectory
    Add-IniLine $builder 'EventLogPath' $EventLogPath
    Add-IniLine $builder 'ProcessNames' ((Get-AllowedProcessNames $Config) -join '|')
    Add-IniLine $builder 'WindowTitle' ([string](Get-RequiredProperty -Object $Config -Name 'windowTitle' -Context 'config'))
    Add-IniLine $builder 'WorkingDirectory' ([string](Get-RequiredProperty -Object $Config -Name 'workingDirectory' -Context 'config'))
    Add-IniLine $builder 'StepCount' $ahkSteps.Count
    [void]$builder.AppendLine()
    [void]$builder.AppendLine('[Profile]')
    Add-IniLine $builder 'ExpectedWidth' ([int](Get-RequiredProperty -Object $expectedSize -Name 'width' -Context 'expectedClientSize'))
    Add-IniLine $builder 'ExpectedHeight' ([int](Get-RequiredProperty -Object $expectedSize -Name 'height' -Context 'expectedClientSize'))
    Add-IniLine $builder 'GeometryTolerancePixels' ([int](Get-OptionalProperty -Object $Profile -Name 'geometryTolerancePixels' -DefaultValue 8))
    Add-IniLine $builder 'InventoryKey' ([string](Get-RequiredProperty -Object $keys -Name 'inventory' -Context 'keys'))
    Add-IniLine $builder 'EscapeKey' ([string](Get-RequiredProperty -Object $keys -Name 'escape' -Context 'keys'))
    Add-IniLine $builder 'PlayX' (Convert-ToIniNumber $play.x)
    Add-IniLine $builder 'PlayY' (Convert-ToIniNumber $play.y)
    Add-IniLine $builder 'DifficultyEnabled' ($(if ($difficultyEnabled) { '1' } else { '0' }))
    if ($difficultyEnabled) {
        Add-IniLine $builder 'DifficultyX' (Convert-ToIniNumber $difficulty.x)
        Add-IniLine $builder 'DifficultyY' (Convert-ToIniNumber $difficulty.y)
    }
    Add-IniLine $builder 'SaveAndExitX' (Convert-ToIniNumber $saveAndExit.x)
    Add-IniLine $builder 'SaveAndExitY' (Convert-ToIniNumber $saveAndExit.y)
    Add-IniLine $builder 'CharacterTemplatePath' $characterTemplatePath
    Add-IniLine $builder 'CharacterTemplateVariation' ([int](Get-OptionalProperty -Object $characterSelection -Name 'variation' -DefaultValue 20))
    Add-IniLine $builder 'CharacterSearchX1' (Convert-ToIniNumber $characterRegion.x1)
    Add-IniLine $builder 'CharacterSearchY1' (Convert-ToIniNumber $characterRegion.y1)
    Add-IniLine $builder 'CharacterSearchX2' (Convert-ToIniNumber $characterRegion.x2)
    Add-IniLine $builder 'CharacterSearchY2' (Convert-ToIniNumber $characterRegion.y2)
    Add-IniLine $builder 'CharacterClickOffsetX' (Convert-ToIniNumber $clickOffsetX)
    Add-IniLine $builder 'CharacterClickOffsetY' (Convert-ToIniNumber $clickOffsetY)
    if ($null -ne $fallback) {
        Add-IniLine $builder 'CharacterFallbackX' (Convert-ToIniNumber $fallback.x)
        Add-IniLine $builder 'CharacterFallbackY' (Convert-ToIniNumber $fallback.y)
    }
    foreach ($probeName in $resolvedProbes.Keys) {
        $probe = $resolvedProbes[$probeName]
        Add-IniLine $builder ($probeName + 'TemplatePath') $probe.path
        Add-IniLine $builder ($probeName + 'SearchX1') (Convert-ToIniNumber $probe.region.x1)
        Add-IniLine $builder ($probeName + 'SearchY1') (Convert-ToIniNumber $probe.region.y1)
        Add-IniLine $builder ($probeName + 'SearchX2') (Convert-ToIniNumber $probe.region.x2)
        Add-IniLine $builder ($probeName + 'SearchY2') (Convert-ToIniNumber $probe.region.y2)
        Add-IniLine $builder ($probeName + 'Variation') $probe.variation
    }
    Add-IniLine $builder 'WindowTimeoutMs' ([int](Get-RequiredProperty -Object $timeouts -Name 'windowReady' -Context 'timeoutsMs'))
    Add-IniLine $builder 'CharacterSelectTimeoutMs' ([int](Get-RequiredProperty -Object $timeouts -Name 'characterSelect' -Context 'timeoutsMs'))
    Add-IniLine $builder 'InGameTimeoutMs' ([int](Get-RequiredProperty -Object $timeouts -Name 'inGame' -Context 'timeoutsMs'))
    Add-IniLine $builder 'InventoryTimeoutMs' ([int](Get-RequiredProperty -Object $timeouts -Name 'inventory' -Context 'timeoutsMs'))
    Add-IniLine $builder 'SaveExitTimeoutMs' ([int](Get-RequiredProperty -Object $timeouts -Name 'transition' -Context 'timeoutsMs'))
    Add-IniLine $builder 'PollIntervalMs' ([int](Get-RequiredProperty -Object $delays -Name 'pollInterval' -Context 'delaysMs'))
    Add-IniLine $builder 'FocusSettleMs' ([int](Get-OptionalProperty -Object $delays -Name 'afterAction' -DefaultValue 100))
    Add-IniLine $builder 'AfterCharacterClickMs' ([int](Get-RequiredProperty -Object $delays -Name 'afterCharacterSelection' -Context 'delaysMs'))
    Add-IniLine $builder 'AfterPlayClickMs' ([int](Get-OptionalProperty -Object $delays -Name 'afterPlay' -DefaultValue 1000))
    Add-IniLine $builder 'AfterDifficultyClickMs' ([int](Get-OptionalProperty -Object $delays -Name 'afterDifficulty' -DefaultValue 1000))
    Add-IniLine $builder 'AfterInventoryKeyMs' ([int](Get-RequiredProperty -Object $delays -Name 'afterMenuOpen' -Context 'delaysMs'))
    Add-IniLine $builder 'AfterEscapeMs' ([int](Get-RequiredProperty -Object $delays -Name 'afterMenuOpen' -Context 'delaysMs'))
    Add-IniLine $builder 'AfterSaveExitClickMs' ([int](Get-OptionalProperty -Object $delays -Name 'afterAction' -DefaultValue 100))
    Add-IniLine $builder 'InputDurationMs' ([int](Get-RequiredProperty -Object $delays -Name 'input' -Context 'delaysMs'))
    Add-IniLine $builder 'MouseMoveSpeed' 0
    [void]$builder.AppendLine()

    for ($index = 0; $index -lt $ahkSteps.Count; $index++) {
        $step = $ahkSteps[$index]
        [void]$builder.AppendLine(('[Step.{0:D3}]' -f ($index + 1)))
        Add-IniLine $builder 'Action' ([string]$step.action)
        Add-IniLine $builder 'Name' ([string](Get-OptionalProperty -Object $step -Name 'name' -DefaultValue ''))
        [void]$builder.AppendLine()
    }

    [IO.File]::WriteAllText($PlanPath, $builder.ToString(), (New-Object Text.UnicodeEncoding($false, $true)))
    return $ahkSteps
}

function Quote-NativeArgument {
    param([Parameter(Mandatory = $true)][string]$Value)

    if ($Value -notmatch '[\s"]') {
        return $Value
    }
    return '"' + ($Value -replace '(\\*)"', '$1$1\"' -replace '(\\+)$', '$1$1') + '"'
}

function Read-JsonLines {
    param([Parameter(Mandatory = $true)][string]$Path)

    $events = @()
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $events
    }
    foreach ($line in Get-Content -LiteralPath $Path -Encoding UTF8) {
        if ([string]::IsNullOrWhiteSpace($line)) {
            continue
        }
        try {
            $events += ($line | ConvertFrom-Json)
        }
        catch {
            throw "Invalid JSONL event emitted by AutoHotkey: $line"
        }
    }
    return $events
}

function Test-FixtureFileName {
    param(
        [Parameter(Mandatory = $true)][string]$FileName,
        [Parameter(Mandatory = $true)][string]$CharacterName
    )

    if ([IO.Path]::GetFileName($FileName) -cne $FileName) {
        return $false
    }
    $escaped = [Regex]::Escape($CharacterName)
    return $FileName -cmatch "^$escaped\.(?:d2s|key|ctl|map|ma[0-9])$"
}

$startedAt = [DateTime]::UtcNow
$phase = 'initialization'
$runDirectory = $null
$resultPath = $null
$config = $null
$processNames = @()
$modInfoBytes = $null
$modInfoPath = $null
$modInfoOriginalHash = $null
$modInfoPatched = $false
$modInfoPatchAttempted = $false
$originalSaveDirectory = $null
$originalSnapshotBefore = $null
$fixtureSnapshotBefore = $null
$fixtureSourceDirectory = $null
$fixtureSourceSha256 = $null
$primaryFixtureSourcePath = $null
$testSaveDirectory = $null
$testSavePathName = $null
$testSaveOwnershipEstablished = $false
$fixtureDefinition = $null
$fixtureFiles = @()
$preserveWorkingCopy = $true
$runnerExitCode = $null
$events = @()
$integrityFailure = $false
$processAliveAtVerdict = $false
$scenarioDefinition = $null
$ahkProcess = $null
$orchestrationMutex = $null
$orchestrationMutexAcquired = $false
$result = [ordered]@{
    scenarioId = $Scenario
    status = 'fail'
    startedAt = $startedAt.ToString('o')
    completedAt = ''
    stepsCompleted = @()
    failedStep = $null
    processAlive = $false
    screenshots = @()
    logs = @()
    recoveryAttempted = $false
    notes = @()
    lastRecognizedState = 'NOT_RUNNING'
    lastActions = @()
    timings = @()
    recoveryResult = $null
}

try {
    $phase = 'acquire_runner_mutex'
    $orchestrationMutex = New-Object Threading.Mutex($false, 'Local\RuffnecKk.BKVince.GameTestRunner.PowerShell')
    try {
        $orchestrationMutexAcquired = $orchestrationMutex.WaitOne(0, $false)
    }
    catch [Threading.AbandonedMutexException] {
        $orchestrationMutexAcquired = $true
        $script:RunNotes.Add('Recovered an abandoned GameTestRunner orchestration mutex.')
    }
    if (-not $orchestrationMutexAcquired) {
        throw 'Another GameTestRunner orchestration already owns the D2R test session.'
    }

    $phase = 'load_config'
    if ([string]::IsNullOrWhiteSpace($ConfigPath)) {
        $ConfigPath = Join-Path $script:RunnerDirectory 'config.local.json'
    }
    $ConfigPath = Resolve-AbsolutePath -Path $ConfigPath -BasePath $script:RepositoryRoot
    if (-not (Test-Path -LiteralPath $ConfigPath -PathType Leaf)) {
        throw "Local config does not exist: $ConfigPath. Copy config.example.json to config.local.json and calibrate it."
    }
    $configDirectory = Split-Path -Parent $ConfigPath
    $config = Get-Content -Raw -LiteralPath $ConfigPath -Encoding UTF8 | ConvertFrom-Json
    if ([int](Get-RequiredProperty -Object $config -Name 'schemaVersion' -Context 'config') -ne 1) {
        throw 'Only config schemaVersion 1 is supported.'
    }
    $configuredLauncherArguments = @((Get-OptionalProperty -Object $config -Name 'launcherArguments' -DefaultValue @()))
    if (@($configuredLauncherArguments | Where-Object { [string]$_ -ceq '-offline' }).Count -ne 1) {
        throw 'The v1 runner requires exactly one literal -offline launcher argument.'
    }
    $modArgumentIndexes = @(
        for ($argumentIndex = 0; $argumentIndex -lt $configuredLauncherArguments.Count; $argumentIndex++) {
            if (([string]$configuredLauncherArguments[$argumentIndex]).Equals('-mod', [StringComparison]::OrdinalIgnoreCase)) {
                $argumentIndex
            }
        }
    )
    if ($modArgumentIndexes.Count -ne 1) {
        throw 'The v1 runner requires exactly one -mod <name> launcher argument pair.'
    }
    $modArgumentIndex = [int]$modArgumentIndexes[0]
    if ([string]$configuredLauncherArguments[$modArgumentIndex] -cne '-mod' -or $modArgumentIndex + 1 -ge $configuredLauncherArguments.Count) {
        throw 'The mod launcher argument must be the exact two-token pair -mod <name>.'
    }
    $configuredModName = [string]$configuredLauncherArguments[$modArgumentIndex + 1]
    if ($configuredModName -notmatch '^[A-Za-z0-9][A-Za-z0-9_.-]{0,63}$') {
        throw "The configured -mod name is unsafe: $configuredModName"
    }

    $artifactRoot = Resolve-AbsolutePath -Path ([string](Get-RequiredProperty -Object $config -Name 'artifactDirectory' -Context 'config')) -BasePath $script:RepositoryRoot
    $expectedArtifactRoot = [IO.Path]::GetFullPath((Join-Path $script:RepositoryRoot 'game-tests\artifacts'))
    if (-not (Get-NormalizedDirectoryPath $artifactRoot).Equals((Get-NormalizedDirectoryPath $expectedArtifactRoot), [StringComparison]::OrdinalIgnoreCase)) {
        throw 'config.artifactDirectory must resolve exactly to game-tests/artifacts.'
    }
    Assert-NoReparsePoint -Path $artifactRoot -Label 'artifactDirectory'
    [IO.Directory]::CreateDirectory($artifactRoot) | Out-Null
    Assert-NoReparsePoint -Path $artifactRoot -Label 'artifactDirectory'
    $runName = '{0}-{1}' -f ([DateTime]::Now.ToString('yyyyMMdd-HHmmssfff')), $Scenario
    $runDirectory = Join-Path $artifactRoot $runName
    [IO.Directory]::CreateDirectory($runDirectory) | Out-Null
    $script:PowerShellLogPath = Join-Path $runDirectory 'powershell.log'
    [IO.File]::WriteAllText($script:PowerShellLogPath, '', (New-Object Text.UTF8Encoding($false)))
    $resultPath = Join-Path $runDirectory 'result.json'
    Write-RunLog "GameTestRunner starting scenario '$Scenario'."

    $phase = 'load_scenario'
    if ([string]::IsNullOrWhiteSpace($ScenarioPath)) {
        $ScenarioPath = Join-Path $script:RepositoryRoot ("game-tests\scenarios\$Scenario.json")
    }
    $ScenarioPath = Resolve-AbsolutePath -Path $ScenarioPath -BasePath $script:RepositoryRoot
    if (-not (Test-Path -LiteralPath $ScenarioPath -PathType Leaf)) {
        throw "Scenario does not exist: $ScenarioPath"
    }
    $scenarioDefinition = Get-Content -Raw -LiteralPath $ScenarioPath -Encoding UTF8 | ConvertFrom-Json
    if ([string]$scenarioDefinition.id -cne $Scenario) {
        throw "Scenario id '$($scenarioDefinition.id)' does not match requested id '$Scenario'."
    }
    if (@($scenarioDefinition.steps).Count -eq 0 -or [string]$scenarioDefinition.steps[0].action -cne 'launch_or_reuse_game') {
        throw 'A v1 scenario must begin with launch_or_reuse_game.'
    }

    $phase = 'validate_profile'
    $profileName = [string](Get-RequiredProperty -Object $config -Name 'inputProfile' -Context 'config')
    $profiles = Get-RequiredProperty -Object $config -Name 'inputProfiles' -Context 'config'
    $profileProperty = $profiles.PSObject.Properties[$profileName]
    if ($null -eq $profileProperty) {
        throw "Configured input profile '$profileName' does not exist."
    }
    $profile = $profileProperty.Value
    $expectedSize = Get-RequiredProperty -Object $profile -Name 'expectedClientSize' -Context 'input profile'
    $expectedResolution = '{0}x{1}' -f $expectedSize.width, $expectedSize.height
    if ([string](Get-RequiredProperty -Object $config -Name 'resolution' -Context 'config') -cne $expectedResolution) {
        throw "config.resolution must match expected client size $expectedResolution."
    }
    if ([string](Get-RequiredProperty -Object $config -Name 'windowMode' -Context 'config') -cne 'windowed') {
        throw 'The v1 runner accepts only the calibrated windowed profile.'
    }
    $timeouts = Get-RequiredProperty -Object $profile -Name 'timeoutsMs' -Context 'input profile'
    $runnerStartupTimeout = [int](Get-RequiredProperty -Object $timeouts -Name 'runnerStartup' -Context 'timeoutsMs')
    $runnerTimeout = [int](Get-RequiredProperty -Object $timeouts -Name 'runner' -Context 'timeoutsMs')
    if ($runnerStartupTimeout -le 0 -or $runnerTimeout -le 0) {
        throw 'timeoutsMs.runnerStartup and timeoutsMs.runner must both be positive integers.'
    }
    if ($runnerStartupTimeout -ge $runnerTimeout) {
        throw 'timeoutsMs.runnerStartup must be strictly less than timeoutsMs.runner.'
    }

    $processNames = Get-AllowedProcessNames $config
    $windowTitle = [string](Get-RequiredProperty -Object $config -Name 'windowTitle' -Context 'config')
    if ($windowTitle -cne 'Diablo II: Resurrected') {
        throw 'config.windowTitle is fixed to Diablo II: Resurrected.'
    }
    $workingDirectory = Resolve-AbsolutePath -Path ([string](Get-RequiredProperty -Object $config -Name 'workingDirectory' -Context 'config')) -BasePath $configDirectory
    if (-not (Test-Path -LiteralPath $workingDirectory -PathType Container)) {
        throw "Configured D2R working directory does not exist: $workingDirectory"
    }
    $modsDirectory = Join-Path $workingDirectory 'mods'
    $configuredModDirectory = Join-Path $modsDirectory $configuredModName
    if (-not (Test-Path -LiteralPath $configuredModDirectory -PathType Container)) {
        throw "The configured -mod runtime directory does not exist: $configuredModDirectory"
    }
    Assert-NoReparsePoint -Path $configuredModDirectory -Label 'configured mod runtime directory'
    $script:TrustedGameDirectory = $workingDirectory
    $config.workingDirectory = $workingDirectory
    $autoHotkeyExecutable = Resolve-AbsolutePath -Path ([string](Get-RequiredProperty -Object $config -Name 'autoHotkeyExecutable' -Context 'config')) -BasePath $configDirectory
    $ahkScriptPath = Join-Path $script:RunnerDirectory 'GameTestRunner.ahk'
    if (-not (Test-Path -LiteralPath $autoHotkeyExecutable -PathType Leaf)) {
        throw "AutoHotkey v2 executable does not exist: $autoHotkeyExecutable"
    }
    if (-not (Test-Path -LiteralPath $ahkScriptPath -PathType Leaf)) {
        throw "AutoHotkey runner does not exist: $ahkScriptPath"
    }

    $phase = 'validate_fixture'
    $fixtureId = [string](Get-RequiredProperty -Object $scenarioDefinition -Name 'fixture' -Context 'scenario')
    if ($fixtureId -notmatch '^[a-z0-9][a-z0-9-]{0,63}$') {
        throw "Unsafe fixture id: $fixtureId"
    }
    $fixturePath = Join-Path $script:RepositoryRoot ("game-tests\fixtures\$fixtureId.json")
    if (-not (Test-Path -LiteralPath $fixturePath -PathType Leaf)) {
        throw "Fixture manifest does not exist: $fixturePath"
    }
    $fixtureDefinition = Get-Content -Raw -LiteralPath $fixturePath -Encoding UTF8 | ConvertFrom-Json
    if ([string]$fixtureDefinition.id -cne $fixtureId) {
        throw 'Fixture manifest id does not match its scenario reference.'
    }
    $testCharacter = [string](Get-RequiredProperty -Object $config -Name 'testCharacter' -Context 'config')
    $fixtureCharacter = [string](Get-OptionalProperty -Object $fixtureDefinition -Name 'characterName' -DefaultValue $testCharacter)
    if ($fixtureCharacter -cne $testCharacter) {
        throw "Fixture character '$fixtureCharacter' does not match config.testCharacter '$testCharacter'."
    }
    $fixtureFiles = @(Get-OptionalProperty -Object $fixtureDefinition -Name 'files' -DefaultValue @("$testCharacter.d2s"))
    if ($fixtureFiles.Count -eq 0) {
        throw 'Fixture manifest files cannot be empty.'
    }
    foreach ($fileName in $fixtureFiles) {
        if (-not (Test-FixtureFileName -FileName ([string]$fileName) -CharacterName $testCharacter)) {
            throw "Fixture file is not an allowlisted exact character file: $fileName"
        }
    }
    $primaryFixtureFileName = "$testCharacter.d2s"
    if (@($fixtureFiles | Where-Object { [string]$_ -ceq $primaryFixtureFileName }).Count -ne 1) {
        throw "Fixture manifest must contain exactly one primary character file named $primaryFixtureFileName."
    }
    $fixtureSourceSha256 = ([string](Get-RequiredProperty -Object $fixtureDefinition -Name 'sourceSha256' -Context 'fixture manifest')).Trim().ToLowerInvariant()
    if ($fixtureSourceSha256 -notmatch '^[a-f0-9]{64}$') {
        throw 'Fixture manifest sourceSha256 must be one SHA-256 digest.'
    }
    $preserveWorkingCopy = [bool](Get-OptionalProperty -Object $fixtureDefinition -Name 'preserveWorkingCopy' -DefaultValue $true)

    $fixtureSourceDirectory = Resolve-AbsolutePath -Path ([string](Get-RequiredProperty -Object $config -Name 'fixtureSourceDirectory' -Context 'config')) -BasePath $configDirectory
    $testSaveDirectory = Resolve-AbsolutePath -Path ([string](Get-RequiredProperty -Object $config -Name 'testSaveDirectory' -Context 'config')) -BasePath $configDirectory
    $modInfoPath = Resolve-AbsolutePath -Path ([string](Get-RequiredProperty -Object $config -Name 'runtimeModInfoPath' -Context 'config')) -BasePath $configDirectory
    $testSavePathName = ([string](Get-RequiredProperty -Object $config -Name 'testSavePathName' -Context 'config')).Trim().Trim('/', '\')
    if ($testSavePathName -notmatch '^[A-Za-z0-9_.-]+$') {
        throw 'testSavePathName must be one safe directory name.'
    }
    if (-not $testSavePathName.EndsWith('GameTest', [StringComparison]::Ordinal)) {
        throw 'testSavePathName must end exactly with GameTest.'
    }
    if ([IO.Path]::GetFileName((Get-NormalizedDirectoryPath $testSaveDirectory)) -cne $testSavePathName) {
        throw 'testSaveDirectory leaf must exactly match testSavePathName.'
    }
    if (-not (Test-Path -LiteralPath $fixtureSourceDirectory -PathType Container)) {
        throw "Fixture source directory does not exist: $fixtureSourceDirectory"
    }
    if (-not (Test-Path -LiteralPath $modInfoPath -PathType Leaf)) {
        throw "Runtime modinfo does not exist: $modInfoPath"
    }
    if (-not (Test-PathInsideDirectory -Candidate $modInfoPath -Directory $configuredModDirectory)) {
        throw 'runtimeModInfoPath must be strictly inside workingDirectory\mods\<configured-mod-name>.'
    }
    if (Test-PathInsideDirectory -Candidate $modInfoPath -Directory $script:RepositoryRoot -AllowEqual) {
        throw 'runtimeModInfoPath must target the deployed runtime, not the repository.'
    }
    Assert-NoReparsePoint -Path $modInfoPath -Label 'runtimeModInfoPath'

    $modInfoBytes = [IO.File]::ReadAllBytes($modInfoPath)
    $modInfoOriginalHash = Get-Sha256Bytes $modInfoBytes
    $modInfoText = (New-Object Text.UTF8Encoding($false, $true)).GetString($modInfoBytes).TrimStart([char]0xFEFF)
    $modInfoObject = $modInfoText | ConvertFrom-Json
    $runtimeModName = [string](Get-RequiredProperty -Object $modInfoObject -Name 'name' -Context 'runtime modinfo')
    if ($runtimeModName -cne $configuredModName) {
        throw "Runtime modinfo name '$runtimeModName' does not exactly match launcher mod '$configuredModName'."
    }
    $originalSavePathName = ([string](Get-RequiredProperty -Object $modInfoObject -Name 'savepath' -Context 'runtime modinfo')).Trim().Trim('/', '\')
    if ($originalSavePathName -notmatch '^[A-Za-z0-9_.-]+$') {
        throw 'The existing runtime savepath is not one safe directory name.'
    }
    if ($originalSavePathName.Equals($testSavePathName, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Runtime modinfo already points at the isolated test savepath; refusing an ambiguous stale state.'
    }
    $saveParent = Split-Path -Parent $testSaveDirectory
    if ([IO.Path]::GetFileName((Get-NormalizedDirectoryPath $saveParent)) -ine 'mods') {
        throw 'testSaveDirectory must be directly inside the D2R Saved Games mods directory.'
    }
    $savedGamesRoot = Split-Path -Parent $saveParent
    if ([IO.Path]::GetFileName((Get-NormalizedDirectoryPath $savedGamesRoot)) -ine 'Diablo II Resurrected') {
        throw 'testSaveDirectory is not under the expected Diablo II Resurrected Saved Games root.'
    }
    $originalSaveDirectory = Join-Path $saveParent $originalSavePathName
    if ((Get-NormalizedDirectoryPath $originalSaveDirectory).Equals((Get-NormalizedDirectoryPath $testSaveDirectory), [StringComparison]::OrdinalIgnoreCase)) {
        throw 'The test save directory aliases the original save directory.'
    }
    if (Test-PathInsideDirectory -Candidate $fixtureSourceDirectory -Directory $originalSaveDirectory -AllowEqual) {
        throw 'Fixture source is inside the original live save directory.'
    }
    if (Test-PathInsideDirectory -Candidate $fixtureSourceDirectory -Directory $testSaveDirectory -AllowEqual) {
        throw 'Fixture source is inside the isolated working save directory.'
    }
    Assert-SafeLeafPath -Candidate $testSaveDirectory -Parent $saveParent -Label 'testSaveDirectory'
    if (-not (Test-Path -LiteralPath $originalSaveDirectory -PathType Container)) {
        throw "Original BKVince save directory does not exist: $originalSaveDirectory"
    }
    Assert-NoReparsePoint -Path $savedGamesRoot -Label 'Saved Games root'
    Assert-NoReparsePoint -Path $originalSaveDirectory -Label 'originalSaveDirectory'
    Assert-NoReparsePoint -Path $fixtureSourceDirectory -Label 'fixtureSourceDirectory'
    Assert-NoReparsePoint -Path $testSaveDirectory -Label 'testSaveDirectory'
    Assert-TestSaveDirectoryOwnershipState -Directory $testSaveDirectory -TestSavePathName $testSavePathName

    foreach ($fileName in $fixtureFiles) {
        $sourcePath = Join-Path $fixtureSourceDirectory ([string]$fileName)
        if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
            throw "Fixture source file does not exist: $sourcePath"
        }
        $sourceItem = Get-Item -LiteralPath $sourcePath -Force -ErrorAction Stop
        if (($sourceItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Fixture source file must not be a reparse point: $sourcePath"
        }
    }
    $primaryFixtureSourcePath = Join-Path $fixtureSourceDirectory $primaryFixtureFileName
    $fixturePrimaryHashBefore = (Get-FileHash -LiteralPath $primaryFixtureSourcePath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($fixturePrimaryHashBefore -cne $fixtureSourceSha256) {
        throw "Fixture source SHA-256 does not match its manifest for $primaryFixtureFileName."
    }

    $phase = 'snapshot_originals'
    $existingProcesses = @(Get-GameProcesses -Names $processNames)
    if ($existingProcesses.Count -gt 0) {
        Write-RunLog 'An existing D2R session was detected but cannot be reused because it owns the live BKVince savepath; it will be closed before establishing the integrity baseline.'
    }
    Stop-GameProcesses -Names $processNames
    $originalSnapshotBefore = Get-DirectorySnapshot $originalSaveDirectory
    $fixtureSnapshotBefore = Get-DirectorySnapshot $fixtureSourceDirectory
    Write-JsonFile -Value $originalSnapshotBefore -Path (Join-Path $runDirectory 'original-saves-before.json')
    Write-JsonFile -Value $fixtureSnapshotBefore -Path (Join-Path $runDirectory 'fixture-source-before.json')
    Write-RunLog "Captured SHA-256 snapshots for original saves and fixture source."

    $phase = 'prepare_isolation'
    Ensure-TestSaveOwnershipMarker -Directory $testSaveDirectory -TestSavePathName $testSavePathName
    $testSaveOwnershipEstablished = $true
    $workingBackupDirectory = Join-Path $runDirectory 'working-copy-before'
    $workingFiles = @(
        Get-ChildItem -LiteralPath $testSaveDirectory -File -ErrorAction SilentlyContinue |
            Where-Object { Test-FixtureFileName -FileName $_.Name -CharacterName $testCharacter }
    )
    if ($workingFiles.Count -gt 0 -and $preserveWorkingCopy) {
        [IO.Directory]::CreateDirectory($workingBackupDirectory) | Out-Null
    }
    foreach ($workingFile in $workingFiles) {
        if (($workingFile.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Refused reparse-point working file: $($workingFile.FullName)"
        }
        if ($preserveWorkingCopy) {
            Copy-Item -LiteralPath $workingFile.FullName -Destination (Join-Path $workingBackupDirectory $workingFile.Name) -Force
        }
        if (-not (Test-PathInsideDirectory -Candidate $workingFile.FullName -Directory $testSaveDirectory)) {
            throw "Refused to replace a working file outside testSaveDirectory: $($workingFile.FullName)"
        }
        Remove-Item -LiteralPath $workingFile.FullName -Force
    }
    foreach ($fileName in $fixtureFiles) {
        $sourcePath = Join-Path $fixtureSourceDirectory ([string]$fileName)
        $destinationPath = Join-Path $testSaveDirectory ([string]$fileName)
        $sourceHash = (Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash.ToLowerInvariant()
        if ([string]$fileName -ceq $primaryFixtureFileName -and $sourceHash -cne $fixtureSourceSha256) {
            throw "Fixture source SHA-256 changed before copy for $fileName."
        }
        Copy-Item -LiteralPath $sourcePath -Destination $destinationPath -Force
        $destinationHash = (Get-FileHash -LiteralPath $destinationPath -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($sourceHash -cne $destinationHash) {
            throw "Fixture copy hash mismatch for $fileName."
        }
    }

    $modInfoBackupPath = Join-Path $runDirectory 'modinfo.original.json'
    [IO.File]::WriteAllBytes($modInfoBackupPath, $modInfoBytes)
    $modInfoObject.savepath = $testSavePathName + '/'
    $patchedText = ($modInfoObject | ConvertTo-Json -Depth 64) + [Environment]::NewLine
    $patchedBytes = (New-Object Text.UTF8Encoding($false)).GetBytes($patchedText)
    $modInfoPatchAttempted = $true
    Replace-FileAtomically -Destination $modInfoPath -Bytes $patchedBytes
    $modInfoPatched = $true
    $patchedVerify = Get-Content -Raw -LiteralPath $modInfoPath -Encoding UTF8 | ConvertFrom-Json
    if (([string]$patchedVerify.savepath).Trim().Trim('/', '\') -cne $testSavePathName) {
        throw 'Runtime modinfo savepath redirection could not be verified.'
    }
    Write-RunLog "Runtime savepath redirected to isolated '$testSavePathName/' after all D2R processes stopped."

    $phase = 'build_plan'
    $eventLogPath = Join-Path $runDirectory 'ahk-events.jsonl'
    $planPath = Join-Path $runDirectory 'runner-plan.ini'
    $ahkSteps = New-AhkPlan -Config $config -Profile $profile -ScenarioDefinition $scenarioDefinition -ConfigDirectory $configDirectory -RunDirectory $runDirectory -EventLogPath $eventLogPath -PlanPath $planPath

    $phase = 'launch_or_reuse_game'
    $launcherRaw = [string](Get-OptionalProperty -Object $config -Name 'launcherExecutable' -DefaultValue '')
    $gameRaw = [string](Get-OptionalProperty -Object $config -Name 'gameExecutable' -DefaultValue '')
    $executableRaw = if (-not [string]::IsNullOrWhiteSpace($launcherRaw)) { $launcherRaw } else { $gameRaw }
    $executable = Resolve-AbsolutePath -Path $executableRaw -BasePath $configDirectory
    $workingDirectory = Resolve-AbsolutePath -Path ([string](Get-RequiredProperty -Object $config -Name 'workingDirectory' -Context 'config')) -BasePath $configDirectory
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
        throw "Configured D2R executable does not exist: $executable"
    }
    if ([IO.Path]::GetFileName($executable) -notin @('D2R.exe', 'D2RLoader.exe')) {
        throw 'Configured launcher must be exactly D2R.exe or D2RLoader.exe.'
    }
    if (-not (Test-PathInsideDirectory -Candidate $executable -Directory $workingDirectory)) {
        throw 'Configured launcher must be inside the trusted D2R working directory.'
    }
    if (-not (Test-Path -LiteralPath $workingDirectory -PathType Container)) {
        throw "Configured D2R working directory does not exist: $workingDirectory"
    }
    $script:TrustedGameDirectory = $workingDirectory
    $launcherArguments = @($configuredLauncherArguments)
    $argumentLine = (@($launcherArguments | ForEach-Object { Quote-NativeArgument ([string]$_) }) -join ' ')
    Write-RunLog "Launching isolated D2R session with the configured local launcher."
    if ([string]::IsNullOrWhiteSpace($argumentLine)) {
        $launchedProcess = Start-Process -FilePath $executable -WorkingDirectory $workingDirectory -PassThru
    }
    else {
        $launchedProcess = Start-Process -FilePath $executable -ArgumentList $argumentLine -WorkingDirectory $workingDirectory -PassThru
    }
    $windowTimeout = [int](Get-RequiredProperty -Object $timeouts -Name 'windowReady' -Context 'timeoutsMs')
    $gameWindowProcess = Wait-ForGameWindow -Names $processNames -WindowTitle $windowTitle -TimeoutMs $windowTimeout
    Write-RunLog "Unique D2R window is available on PID $($gameWindowProcess.Id)."
    $result.stepsCompleted = @('launch_or_reuse_game')

    $phase = 'run_autohotkey'
    $ahkStdoutPath = Join-Path $runDirectory 'autohotkey.stdout.log'
    $ahkStderrPath = Join-Path $runDirectory 'autohotkey.stderr.log'
    $ahkArgumentLine = (Quote-NativeArgument $ahkScriptPath) + ' ' + (Quote-NativeArgument $planPath)
    $ahkRunnerStopwatch = [Diagnostics.Stopwatch]::StartNew()
    $ahkProcess = Start-Process -FilePath $autoHotkeyExecutable -ArgumentList $ahkArgumentLine -RedirectStandardOutput $ahkStdoutPath -RedirectStandardError $ahkStderrPath -WindowStyle Hidden -PassThru

    $eventStreamReady = Test-Path -LiteralPath $eventLogPath -PathType Leaf
    while (-not $eventStreamReady -and -not $ahkProcess.HasExited -and $ahkRunnerStopwatch.ElapsedMilliseconds -lt $runnerStartupTimeout) {
        $startupRemaining = [int]($runnerStartupTimeout - $ahkRunnerStopwatch.ElapsedMilliseconds)
        if ($startupRemaining -lt 1) {
            $startupRemaining = 1
        }
        $startupWaitSlice = [Math]::Min(100, $startupRemaining)
        [void]$ahkProcess.WaitForExit($startupWaitSlice)
        $eventStreamReady = Test-Path -LiteralPath $eventLogPath -PathType Leaf
    }
    if (-not $eventStreamReady) {
        $eventStreamReady = Test-Path -LiteralPath $eventLogPath -PathType Leaf
    }
    if (-not $eventStreamReady -and -not $ahkProcess.HasExited) {
        Stop-Process -Id $ahkProcess.Id -Force -ErrorAction SilentlyContinue
        [void]$ahkProcess.WaitForExit(5000)
        throw "AutoHotkey did not initialize event stream within $runnerStartupTimeout ms."
    }

    $runnerRemaining = [int64]$runnerTimeout - $ahkRunnerStopwatch.ElapsedMilliseconds
    if (-not $ahkProcess.HasExited -and ($runnerRemaining -le 0 -or -not $ahkProcess.WaitForExit([int]$runnerRemaining))) {
        Stop-Process -Id $ahkProcess.Id -Force -ErrorAction SilentlyContinue
        [void]$ahkProcess.WaitForExit(5000)
        throw "AutoHotkey exceeded the configured global timeout of $runnerTimeout ms."
    }
    $ahkProcess.WaitForExit()
    $runnerExitCode = [int]$ahkProcess.ExitCode
    Write-RunLog "AutoHotkey runner exited with code $runnerExitCode."

    $phase = 'aggregate_result'
    $events = @(Read-JsonLines $eventLogPath)
    $completedActions = @(
        $events |
            Where-Object { $_.status -ceq 'completed' -and $_.action -notin @('capture_started', 'failure_capture', 'recovery_save_and_exit') } |
            ForEach-Object { [string]$_.action }
    )
    $result.stepsCompleted = @('launch_or_reuse_game') + $completedActions
    $failedEvent = $events | Where-Object { $_.status -in @('failed', 'inconclusive') -and $_.action -ne 'failure_capture' } | Select-Object -First 1
    if ($null -ne $failedEvent) {
        $result.failedStep = [ordered]@{
            index = ([int]$failedEvent.stepIndex + 1)
            action = [string]$failedEvent.action
            status = [string]$failedEvent.status
            message = [string]$failedEvent.message
        }
    }
    $result.screenshots = @(
        $events |
            Where-Object { $null -ne $_.artifactPath -and -not [string]::IsNullOrWhiteSpace([string]$_.artifactPath) } |
            ForEach-Object { [string]$_.artifactPath } |
            Select-Object -Unique
    )
    $result.recoveryAttempted = @($events | Where-Object { $_.action -ceq 'recovery_save_and_exit' }).Count -gt 0
    $recoveryEvent = $events | Where-Object { $_.action -ceq 'recovery_save_and_exit' } | Select-Object -Last 1
    if ($null -ne $recoveryEvent) {
        $result.recoveryResult = [ordered]@{ status = [string]$recoveryEvent.status; message = [string]$recoveryEvent.message }
    }
    $lastEvent = $events | Select-Object -Last 1
    if ($null -ne $lastEvent) {
        $result.lastRecognizedState = [string]$lastEvent.state
    }
    $result.lastActions = @($events | Select-Object -Last 10 | ForEach-Object { [ordered]@{ action = [string]$_.action; status = [string]$_.status; message = [string]$_.message } })
    $result.timings = @($events | ForEach-Object { [ordered]@{ stepIndex = [int]$_.stepIndex; action = [string]$_.action; durationMs = [int]$_.durationMs } })

    $expectedActions = @($scenarioDefinition.steps | ForEach-Object { [string]$_.action })
    $allStepsConfirmed = $result.stepsCompleted.Count -eq $expectedActions.Count
    if ($allStepsConfirmed) {
        for ($index = 0; $index -lt $expectedActions.Count; $index++) {
            if ([string]$result.stepsCompleted[$index] -cne [string]$expectedActions[$index]) {
                $allStepsConfirmed = $false
                break
            }
        }
    }
    $captureConfirmed = @(
        $events | Where-Object {
            $_.action -ceq 'capture' -and $_.status -ceq 'completed' -and
            $null -ne $_.artifactPath -and (Test-Path -LiteralPath ([string]$_.artifactPath) -PathType Leaf) -and
            (Get-Item -LiteralPath ([string]$_.artifactPath)).Length -gt 0
        }
    ).Count -gt 0
    if ($runnerExitCode -eq 0 -and $allStepsConfirmed -and $captureConfirmed -and $result.lastRecognizedState -ceq 'CHARACTER_SELECT') {
        $result.status = 'pass'
    }
    elseif ($runnerExitCode -eq 2 -or ($null -ne $failedEvent -and $failedEvent.status -ceq 'inconclusive')) {
        $result.status = 'inconclusive'
    }
    else {
        $result.status = 'fail'
    }
    if ($result.status -cne 'pass' -and $null -eq $result.failedStep) {
        $result.failedStep = [ordered]@{
            index = $null
            action = 'run_autohotkey'
            status = $result.status
            message = "AutoHotkey exit code $runnerExitCode did not provide a complete confirmed event sequence."
        }
    }
    $processAliveAtVerdict = @(Get-GameProcesses -Names $processNames).Count -gt 0
    $result.processAlive = $processAliveAtVerdict
}
catch {
    $message = "Phase '$phase' failed: $($_.Exception.Message)"
    Write-RunLog $message 'ERROR'
    $result.status = 'fail'
    if ($null -eq $result.failedStep) {
        $result.failedStep = [ordered]@{ index = $null; action = $phase; status = 'failed'; message = $_.Exception.Message }
    }
    $script:RunNotes.Add($message)
}
finally {
    $cleanupErrors = New-Object 'System.Collections.Generic.List[string]'
    if ($null -ne $ahkProcess) {
        try {
            if (-not $ahkProcess.HasExited) {
                Write-RunLog "Stopping surviving AutoHotkey runner PID $($ahkProcess.Id) before D2R cleanup." 'WARN'
                Stop-Process -Id $ahkProcess.Id -Force -ErrorAction Stop
                [void]$ahkProcess.WaitForExit(5000)
            }
        }
        catch {
            $cleanupErrors.Add("Could not stop the AutoHotkey child: $($_.Exception.Message)")
        }
    }
    if ($processNames.Count -gt 0 -and $modInfoPatchAttempted) {
        try {
            Stop-GameProcesses -Names $processNames
            $script:RunNotes.Add('The isolated D2R process was closed before restoring runtime modinfo; v1 does not retain a fixture-backed session between runs.')
        }
        catch {
            $cleanupErrors.Add("Could not stop D2R during cleanup: $($_.Exception.Message)")
        }
    }

    if ($modInfoPatchAttempted -and $null -ne $modInfoBytes -and -not [string]::IsNullOrWhiteSpace($modInfoPath)) {
        try {
            Replace-FileAtomically -Destination $modInfoPath -Bytes $modInfoBytes
            $restoredBytes = [IO.File]::ReadAllBytes($modInfoPath)
            $restoredHash = Get-Sha256Bytes $restoredBytes
            if ($restoredHash -cne $modInfoOriginalHash -or $restoredBytes.Length -ne $modInfoBytes.Length) {
                throw 'Runtime modinfo byte-exact restoration check failed.'
            }
            $script:RunNotes.Add("Runtime modinfo restored byte-exact (SHA-256 $restoredHash).")
        }
        catch {
            $cleanupErrors.Add("Runtime modinfo restoration failed: $($_.Exception.Message)")
        }
    }

    if ($null -ne $originalSnapshotBefore -and -not [string]::IsNullOrWhiteSpace($originalSaveDirectory)) {
        try {
            $originalSnapshotAfter = Get-DirectorySnapshot $originalSaveDirectory
            if ($null -ne $runDirectory) {
                Write-JsonFile -Value $originalSnapshotAfter -Path (Join-Path $runDirectory 'original-saves-after.json')
            }
            if (-not (Compare-Snapshot -Before $originalSnapshotBefore -After $originalSnapshotAfter)) {
                throw 'Original BKVince save snapshot changed during the isolated test.'
            }
            $script:RunNotes.Add('Original BKVince saves remained SHA-256 byte-identical during the run.')
        }
        catch {
            $cleanupErrors.Add("Original save integrity check failed: $($_.Exception.Message)")
        }
    }

    if ($null -ne $fixtureSnapshotBefore -and -not [string]::IsNullOrWhiteSpace($fixtureSourceDirectory)) {
        try {
            if ([string]::IsNullOrWhiteSpace($fixtureSourceSha256) -or [string]::IsNullOrWhiteSpace($primaryFixtureSourcePath)) {
                throw 'Fixture source SHA-256 verification state is incomplete.'
            }
            $fixturePrimaryHashAfter = (Get-FileHash -LiteralPath $primaryFixtureSourcePath -Algorithm SHA256).Hash.ToLowerInvariant()
            if ($fixturePrimaryHashAfter -cne $fixtureSourceSha256) {
                throw 'Fixture primary source SHA-256 changed or no longer matches its manifest after the run.'
            }
            $fixtureSnapshotAfter = Get-DirectorySnapshot $fixtureSourceDirectory
            if ($null -ne $runDirectory) {
                Write-JsonFile -Value $fixtureSnapshotAfter -Path (Join-Path $runDirectory 'fixture-source-after.json')
            }
            if (-not (Compare-Snapshot -Before $fixtureSnapshotBefore -After $fixtureSnapshotAfter)) {
                throw 'Fixture source snapshot changed during the run.'
            }
            $script:RunNotes.Add('Fixture source remained SHA-256 byte-identical during the run.')
        }
        catch {
            $cleanupErrors.Add("Fixture source integrity check failed: $($_.Exception.Message)")
        }
    }

    $testSaveOwnershipValidForCleanup = $false
    if ($testSaveOwnershipEstablished -and -not [string]::IsNullOrWhiteSpace($testSaveDirectory) -and -not [string]::IsNullOrWhiteSpace($testSavePathName)) {
        try {
            Assert-TestSaveOwnershipMarker -Directory $testSaveDirectory -TestSavePathName $testSavePathName
            $testSaveOwnershipValidForCleanup = $true
            $script:RunNotes.Add('Isolated test save ownership marker remained exact and non-reparse during the run.')
        }
        catch {
            $cleanupErrors.Add("Test save ownership verification failed: $($_.Exception.Message)")
        }
    }

    if (-not $preserveWorkingCopy -and $testSaveOwnershipValidForCleanup -and -not [string]::IsNullOrWhiteSpace($testSaveDirectory) -and $null -ne $fixtureDefinition) {
        try {
            $testCharacterForCleanup = [string](Get-OptionalProperty -Object $fixtureDefinition -Name 'characterName' -DefaultValue '')
            if (-not [string]::IsNullOrWhiteSpace($testCharacterForCleanup)) {
                foreach ($file in @(Get-ChildItem -LiteralPath $testSaveDirectory -File -ErrorAction SilentlyContinue | Where-Object { Test-FixtureFileName -FileName $_.Name -CharacterName $testCharacterForCleanup })) {
                    if (-not (Test-PathInsideDirectory -Candidate $file.FullName -Directory $testSaveDirectory)) {
                        throw "Refused cleanup outside testSaveDirectory: $($file.FullName)"
                    }
                    Remove-Item -LiteralPath $file.FullName -Force
                }
            }
        }
        catch {
            $cleanupErrors.Add("Working-copy cleanup failed: $($_.Exception.Message)")
        }
    }

    try {
        $result.processAlive = $processNames.Count -gt 0 -and @(Get-GameProcesses -Names $processNames).Count -gt 0
    }
    catch {
        $result.processAlive = $false
        $cleanupErrors.Add("Could not verify final D2R process state: $($_.Exception.Message)")
    }

    if ($cleanupErrors.Count -gt 0) {
        $integrityFailure = $true
        $result.status = 'fail'
        foreach ($cleanupError in $cleanupErrors) {
            Write-RunLog $cleanupError 'ERROR'
            $script:RunNotes.Add($cleanupError)
        }
        if ($null -eq $result.failedStep) {
            $result.failedStep = [ordered]@{ index = $null; action = 'cleanup_integrity'; status = 'failed'; message = ($cleanupErrors -join ' ') }
        }
    }

    if ($null -ne $runDirectory) {
        $logCandidates = @(
            $script:PowerShellLogPath,
            (Join-Path $runDirectory 'autohotkey.stdout.log'),
            (Join-Path $runDirectory 'autohotkey.stderr.log'),
            (Join-Path $runDirectory 'ahk-events.jsonl'),
            (Join-Path $runDirectory 'runner-plan.ini')
        )
        $result.logs = @($logCandidates | Where-Object { $null -ne $_ -and (Test-Path -LiteralPath $_ -PathType Leaf) })
    }
    $result.notes = @($script:RunNotes)
    $result.completedAt = [DateTime]::UtcNow.ToString('o')
    if ($null -ne $resultPath) {
        try {
            Write-JsonFile -Value $result -Path $resultPath
            Write-Host "RESULT_PATH=$resultPath"
            Write-Host "RESULT_STATUS=$($result.status)"
        }
        catch {
            Write-Error "Could not write result.json: $($_.Exception.Message)"
            exit 1
        }
    }

    if ($orchestrationMutexAcquired -and $null -ne $orchestrationMutex) {
        try {
            $orchestrationMutex.ReleaseMutex()
            $orchestrationMutexAcquired = $false
        }
        catch {
            Write-RunLog "Could not release GameTestRunner orchestration mutex: $($_.Exception.Message)" 'ERROR'
        }
    }
    if ($null -ne $orchestrationMutex) {
        $orchestrationMutex.Dispose()
        $orchestrationMutex = $null
    }
}

switch ($result.status) {
    'pass' { exit 0 }
    'inconclusive' { exit 2 }
    default { exit 1 }
}
