[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateRange(1, [int]::MaxValue)]
    [int]$ProcessId,

    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$playerSequenceTableRva = 0x2386650L
$weaponClassMapRva = 0x2386730L
$playerSequenceSlotCount = 26
$weaponClassCount = 14
$descriptorSize = 24
$recordSize = 6

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

public static class PlayerSequenceNativeMethods
{
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr OpenProcess(uint access, bool inheritHandle, int processId);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool ReadProcessMemory(
        IntPtr process,
        IntPtr address,
        byte[] buffer,
        IntPtr size,
        out IntPtr bytesRead);

    [DllImport("kernel32.dll")]
    public static extern bool CloseHandle(IntPtr handle);
}
'@

function Format-Hex([long]$Value) {
    return '0x{0:X}' -f $Value
}

function Get-Sha256([byte[]]$Bytes) {
    $hasher = [System.Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString($hasher.ComputeHash($Bytes))).Replace('-', '')
    }
    finally {
        $hasher.Dispose()
    }
}

function Read-NativeBytes([long]$Address, [int]$Size) {
    $buffer = [byte[]]::new($Size)
    $bytesRead = [IntPtr]::Zero
    $success = [PlayerSequenceNativeMethods]::ReadProcessMemory(
        $script:processHandle,
        [IntPtr]$Address,
        $buffer,
        [IntPtr]$Size,
        [ref]$bytesRead)
    if (-not $success -or $bytesRead.ToInt64() -ne $Size) {
        $errorCode = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
        throw "ReadProcessMemory failed at $(Format-Hex $Address): read $($bytesRead.ToInt64())/$Size bytes (Win32 $errorCode)."
    }
    return $buffer
}

$process = Get-Process -Id $ProcessId
$module = $process.MainModule
if (-not $module) {
    throw "Process $ProcessId has no readable main module."
}
if ($process.ProcessName -notin @('D2R', 'D2RLoader')) {
    throw "Process $ProcessId is $($process.ProcessName), not D2R or D2RLoader."
}

$moduleBase = $module.BaseAddress.ToInt64()
$gameRoot = Split-Path -Parent $module.FileName
$buildInfoPath = Join-Path $gameRoot '.build.info'
$retailExePath = Join-Path $gameRoot 'D2R.exe'
if (-not (Test-Path -LiteralPath $buildInfoPath -PathType Leaf)) {
    throw "Missing .build.info beside the runtime host: $buildInfoPath"
}
if (-not (Test-Path -LiteralPath $retailExePath -PathType Leaf)) {
    throw "Missing D2R.exe beside the runtime host: $retailExePath"
}

$buildInfoText = Get-Content -LiteralPath $buildInfoPath -Raw
$buildRows = $buildInfoText -split "`r?`n" | Where-Object { $_ }
if ($buildRows.Count -lt 2) {
    throw 'The installed .build.info has no active build row.'
}
$headers = $buildRows[0] -split '\|'
$values = $buildRows[1] -split '\|'
$build = @{}
for ($index = 0; $index -lt [Math]::Min($headers.Count, $values.Count); $index += 1) {
    $name = ($headers[$index] -split '!')[0]
    $build[$name] = $values[$index]
}

$script:processHandle = [PlayerSequenceNativeMethods]::OpenProcess(0x0410, $false, $ProcessId)
if ($script:processHandle -eq [IntPtr]::Zero) {
    $errorCode = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
    throw "OpenProcess failed for $ProcessId (Win32 $errorCode)."
}

try {
    $pointerTableBytes = Read-NativeBytes `
        ($moduleBase + $playerSequenceTableRva) `
        ($playerSequenceSlotCount * 8)
    $weaponClassMapBytes = Read-NativeBytes `
        ($moduleBase + $weaponClassMapRva) `
        ($weaponClassCount * 8)
    $weaponClassMap = [System.Collections.Generic.List[object]]::new()
    for ($weaponIndex = 0; $weaponIndex -lt $weaponClassCount; $weaponIndex += 1) {
        $entryOffset = $weaponIndex * 8
        $mappedIndex = [BitConverter]::ToInt32($weaponClassMapBytes, $entryOffset)
        $weaponClassId = [BitConverter]::ToInt32($weaponClassMapBytes, $entryOffset + 4)
        if ($mappedIndex -ne $weaponIndex) {
            throw "Weapon-class routing entry $weaponIndex maps to unexpected index $mappedIndex."
        }
        $weaponClassMap.Add([ordered]@{
            weaponClassIndex = $mappedIndex
            weaponClassId = $weaponClassId
        })
    }

    $groups = [System.Collections.Generic.List[object]]::new()
    $uniqueRecords = [ordered]@{}
    for ($sequenceId = 0; $sequenceId -lt $playerSequenceSlotCount; $sequenceId += 1) {
        $groupPointer = [BitConverter]::ToInt64($pointerTableBytes, $sequenceId * 8)
        if ($sequenceId -eq 0) {
            if ($groupPointer -ne 0) {
                throw 'Player sequence slot zero is not null.'
            }
            continue
        }
        if ($groupPointer -lt $moduleBase) {
            throw "Player sequence $sequenceId has an invalid group pointer."
        }

        $groupRva = $groupPointer - $moduleBase
        $groupBytes = Read-NativeBytes $groupPointer ($weaponClassCount * $descriptorSize)
        $slots = [System.Collections.Generic.List[object]]::new()
        for ($weaponIndex = 0; $weaponIndex -lt $weaponClassCount; $weaponIndex += 1) {
            $descriptorOffset = $weaponIndex * $descriptorSize
            $recordsPointer = [BitConverter]::ToInt64($groupBytes, $descriptorOffset)
            $sequenceFrames = [BitConverter]::ToUInt32($groupBytes, $descriptorOffset + 8)
            $animationFrames = [BitConverter]::ToUInt32($groupBytes, $descriptorOffset + 12)
            $extra = [BitConverter]::ToUInt64($groupBytes, $descriptorOffset + 16)
            if ($sequenceFrames -gt 256 -or $animationFrames -gt 256) {
                throw "Player sequence $sequenceId weapon slot $weaponIndex has an implausible frame count."
            }

            $recordsRva = $null
            $recordHash = $null
            if ($recordsPointer -eq 0) {
                if ($sequenceFrames -ne 0 -or $animationFrames -ne 0) {
                    throw "Player sequence $sequenceId weapon slot $weaponIndex has counts without records."
                }
            }
            else {
                if ($recordsPointer -lt $moduleBase -or $sequenceFrames -eq 0) {
                    throw "Player sequence $sequenceId weapon slot $weaponIndex has an invalid record pointer."
                }
                $recordsRvaValue = $recordsPointer - $moduleBase
                $recordsRva = Format-Hex $recordsRvaValue
                $recordBytes = Read-NativeBytes $recordsPointer ($sequenceFrames * $recordSize)
                for ($recordIndex = 0; $recordIndex -lt $sequenceFrames; $recordIndex += 1) {
                    $recordOffset = $recordIndex * $recordSize
                    if ([BitConverter]::ToUInt16($recordBytes, $recordOffset) -ne 0) {
                        throw "Player sequence record $(Format-Hex ($recordsRvaValue + $recordOffset)) has a nonzero sequence field."
                    }
                    if ($recordBytes[$recordOffset + 2] -gt 19) {
                        throw "Player sequence record $(Format-Hex ($recordsRvaValue + $recordOffset)) has an invalid player mode."
                    }
                    if ($recordBytes[$recordOffset + 5] -gt 4) {
                        throw "Player sequence record $(Format-Hex ($recordsRvaValue + $recordOffset)) has an invalid event."
                    }
                }
                $recordHash = Get-Sha256 $recordBytes
                $recordKey = $recordsRva.ToUpperInvariant()
                if ($uniqueRecords.Contains($recordKey)) {
                    $existing = $uniqueRecords[$recordKey]
                    if ($existing.sha256 -ne $recordHash -or $existing.byteLength -ne $recordBytes.Length) {
                        throw "Conflicting runtime record arrays at $recordsRva."
                    }
                }
                else {
                    $uniqueRecords[$recordKey] = [ordered]@{
                        recordsRva = $recordsRva
                        recordCount = $sequenceFrames
                        byteLength = $recordBytes.Length
                        sha256 = $recordHash
                        bytes = ([BitConverter]::ToString($recordBytes)).Replace('-', '')
                    }
                }
            }

            $slots.Add([ordered]@{
                weaponClassIndex = $weaponIndex
                descriptorRva = Format-Hex ($groupRva + $descriptorOffset)
                recordsRva = $recordsRva
                sequenceFrameCount = $sequenceFrames
                animationFrameCount = $animationFrames
                extra = Format-Hex ([long]$extra)
                recordsSha256 = $recordHash
            })
        }

        $groups.Add([ordered]@{
            sequenceId = $sequenceId
            groupRva = Format-Hex $groupRva
            descriptorsSha256 = Get-Sha256 $groupBytes
            slots = $slots
        })
    }

    if ($groups.Count -ne ($playerSequenceSlotCount - 1)) {
        throw "Expected 25 initialized player sequence groups, found $($groups.Count)."
    }

    $snapshot = [ordered]@{
        schemaVersion = 1
        capturedAt = (Get-Date).ToUniversalTime().ToString('o')
        process = [ordered]@{
            id = $ProcessId
            name = $process.ProcessName
            module = Split-Path -Leaf $module.FileName
            moduleBase = Format-Hex $moduleBase
        }
        target = [ordered]@{
            product = 'Diablo II: Resurrected'
            version = $build.Version
            buildKey = $build.'Build Key'
            buildInfoSha256 = (Get-FileHash -LiteralPath $buildInfoPath -Algorithm SHA256).Hash
            retailExeSha256 = (Get-FileHash -LiteralPath $retailExePath -Algorithm SHA256).Hash
        }
        nativeLayout = [ordered]@{
            playerSequenceTableRva = Format-Hex $playerSequenceTableRva
            weaponClassMapRva = Format-Hex $weaponClassMapRva
            slotCount = $playerSequenceSlotCount
            weaponClassCount = $weaponClassCount
            descriptorSize = $descriptorSize
            recordSize = $recordSize
            weaponClassMap = $weaponClassMap
        }
        groups = $groups
        records = @($uniqueRecords.Values)
    }

    $json = $snapshot | ConvertTo-Json -Depth 8
    if ($OutputPath) {
        $resolvedOutput = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputPath)
        $outputDirectory = Split-Path -Parent $resolvedOutput
        if ($outputDirectory) {
            [IO.Directory]::CreateDirectory($outputDirectory) | Out-Null
        }
        [IO.File]::WriteAllText($resolvedOutput, "$json`n", [Text.UTF8Encoding]::new($false))
    }
    else {
        $json
    }
}
finally {
    [void][PlayerSequenceNativeMethods]::CloseHandle($script:processHandle)
}
