param(
    [string]$GameDataPath = 'C:\Games\Diablo II Resurrected\data',
    [string]$CascLibPath = 'C:\Games\Diablo II Resurrected\D2RMM Custom 1.9.1\tools\CascLib.dll',
    [string]$CachePath = ''
)

$ErrorActionPreference = 'Stop'

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$workspaceRoot = [System.IO.Path]::GetFullPath((Join-Path $scriptRoot '..\..\..'))
$registryPath = Join-Path $workspaceRoot 'data-BKVince\BKVince.mpq\data\hd\items\items.json'
if (-not $CachePath) {
    $CachePath = Join-Path $workspaceRoot 'analysis-cache\hero-editor-vanilla-ui'
}

$resolvedData = [System.IO.Path]::GetFullPath($GameDataPath)
$resolvedLibrary = [System.IO.Path]::GetFullPath($CascLibPath)
$resolvedCache = [System.IO.Path]::GetFullPath($CachePath)
if (-not [System.IO.Directory]::Exists($resolvedData)) {
    throw "D2R CASC data directory was not found: $resolvedData"
}
if (-not [System.IO.File]::Exists($resolvedLibrary)) {
    throw "CascLib was not found: $resolvedLibrary"
}
if (-not [System.IO.File]::Exists($registryPath)) {
    throw "BKVince item registry was not found: $registryPath"
}

$escapedLibrary = $resolvedLibrary.Replace('\', '\\')
$source = @"
using System;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;

public static class HeroEditorCascExtractor
{
    [DllImport("$escapedLibrary", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool CascOpenStorage(string path, uint flags, out IntPtr storage);

    [DllImport("$escapedLibrary", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool CascOpenFile(IntPtr storage, string fileName, uint locale, uint flags, out IntPtr file);

    [DllImport("$escapedLibrary", CallingConvention = CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool CascGetFileSize64(IntPtr file, out ulong size);

    [DllImport("$escapedLibrary", CallingConvention = CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool CascReadFile(IntPtr file, byte[] buffer, uint bytesToRead, out uint bytesRead);

    [DllImport("$escapedLibrary", CallingConvention = CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool CascCloseFile(IntPtr file);

    [DllImport("$escapedLibrary", CallingConvention = CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool CascCloseStorage(IntPtr storage);

    [DllImport("$escapedLibrary", CallingConvention = CallingConvention.Cdecl)]
    private static extern uint GetCascError();

    public static IntPtr Open(string gameDataPath)
    {
        foreach (string candidate in new[] { gameDataPath + ":osi", gameDataPath + ":", gameDataPath })
        {
            IntPtr storage;
            if (CascOpenStorage(candidate, 0, out storage)) return storage;
        }
        uint error = GetCascError();
        throw new Win32Exception((int)error, "CascOpenStorage failed (CASC error " + error + ")");
    }

    public static bool TryExtract(IntPtr storage, string filePath, string outputPath)
    {
        string cascPath = "data:data\\" + filePath.TrimStart('\\', '/').Replace('/', '\\');
        IntPtr file;
        if (!CascOpenFile(storage, cascPath, 0, 0, out file)) return false;
        try
        {
            ulong size;
            if (!CascGetFileSize64(file, out size) || size == 0 || size > Int32.MaxValue) return false;
            byte[] buffer = new byte[(int)size];
            uint bytesRead;
            if (!CascReadFile(file, buffer, (uint)buffer.Length, out bytesRead) || bytesRead != buffer.Length)
                throw new EndOfStreamException("Incomplete CASC read for " + filePath + ".");
            Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(outputPath)));
            File.WriteAllBytes(outputPath, buffer);
            return true;
        }
        finally
        {
            CascCloseFile(file);
        }
    }

    public static void Close(IntPtr storage)
    {
        if (storage != IntPtr.Zero) CascCloseStorage(storage);
    }
}
"@

Add-Type -TypeDefinition $source -Language CSharp

$registry = Get-Content -Raw -LiteralPath $registryPath | ConvertFrom-Json
$assets = $registry |
    ForEach-Object { $_.PSObject.Properties } |
    ForEach-Object { $_.Value.asset } |
    Where-Object { $_ } |
    Sort-Object -Unique

$skillAtlases = @(
    'hd/global/ui/spells/amazon/amskillicon.sprite',
    'hd/global/ui/spells/sorceress/soskillicon.sprite',
    'hd/global/ui/spells/necromancer/neskillicon.sprite',
    'hd/global/ui/spells/paladin/paskillicon.sprite',
    'hd/global/ui/spells/barbarian/baskillicon.sprite',
    'hd/global/ui/spells/druid/drskillicon.sprite',
    'hd/global/ui/spells/assassin/asskillicon.sprite'
)

$skillTrees = @(
    'hd/global/ui/spells/skill_trees/amskilltree.sprite',
    'hd/global/ui/spells/skill_trees/soskilltree.sprite',
    'hd/global/ui/spells/skill_trees/neskilltree.sprite',
    'hd/global/ui/spells/skill_trees/paskilltree.sprite',
    'hd/global/ui/spells/skill_trees/baskilltree.sprite',
    'hd/global/ui/spells/skill_trees/drskilltree.sprite',
    'hd/global/ui/spells/skill_trees/asskilltree.sprite',
    'hd/global/ui/spells/skill_trees/waskilltree.sprite'
)

$classPortraits = @(
    'hd/global/ui/hireables/amazonicon.sprite',
    'hd/global/ui/hireables/sorceressicon.sprite',
    'hd/global/ui/hireables/necromancericon.sprite',
    'hd/global/ui/hireables/paladinicon.sprite',
    'hd/global/ui/hireables/barbarianicon.sprite',
    'hd/global/ui/hireables/druidicon.sprite',
    'hd/global/ui/hireables/assassinicon.sprite'
)

# D2S picture_id selects these classic inventory frames for the four native
# multi-picture families. They remain Blizzard assets extracted from Vincent's
# installed game and are converted to PNG by the build; no RuneWizard asset is
# copied.
$legacyInventoryIcons = @(
    'invrin1', 'invrin2', 'invrin3', 'invrin4', 'invrin5',
    'invamu1', 'invamu2', 'invamu3',
    'invjw1', 'invjw2', 'invjw3', 'invjw4', 'invjw5', 'invjw6',
    'invch1', 'invch2', 'invch3', 'invch4', 'invch5', 'invch6', 'invch7', 'invch8', 'invch9'
)
$legacyPalette = 'global/palette/units/pal.dat'

$storage = [HeroEditorCascExtractor]::Open($resolvedData)
$extracted = 0
$reused = 0
$missing = [System.Collections.Generic.List[string]]::new()
$skillExtracted = 0
$skillReused = 0
$missingSkills = [System.Collections.Generic.List[string]]::new()
$skillTreeExtracted = 0
$skillTreeReused = 0
$missingSkillTrees = [System.Collections.Generic.List[string]]::new()
$portraitExtracted = 0
$portraitReused = 0
$missingPortraits = [System.Collections.Generic.List[string]]::new()
$legacyExtracted = 0
$legacyReused = 0
$missingLegacyIcons = [System.Collections.Generic.List[string]]::new()
$legacyPaletteExtracted = $false
$legacyPaletteReused = $false
try {
    foreach ($asset in $assets) {
        $found = $false
        foreach ($kind in @('armor', 'weapon', 'misc')) {
            $relativePath = "hd/global/ui/items/$kind/$asset.sprite"
            $outputPath = Join-Path $resolvedCache ($relativePath.Replace('/', '\'))
            if ((Test-Path -LiteralPath $outputPath) -and (Get-Item -LiteralPath $outputPath).Length -gt 0) {
                $reused++
                $found = $true
                break
            }
            if ([HeroEditorCascExtractor]::TryExtract($storage, $relativePath, $outputPath)) {
                $extracted++
                $found = $true
                break
            }
        }
        if (-not $found) { $missing.Add($asset) }
    }
    foreach ($relativePath in $skillAtlases) {
        $outputPath = Join-Path $resolvedCache ($relativePath.Replace('/', '\'))
        if ((Test-Path -LiteralPath $outputPath) -and (Get-Item -LiteralPath $outputPath).Length -gt 0) {
            $skillReused++
            continue
        }
        if ([HeroEditorCascExtractor]::TryExtract($storage, $relativePath, $outputPath)) {
            $skillExtracted++
        }
        else {
            $missingSkills.Add($relativePath)
        }
    }
    foreach ($relativePath in $skillTrees) {
        $outputPath = Join-Path $resolvedCache ($relativePath.Replace('/', '\'))
        if ((Test-Path -LiteralPath $outputPath) -and (Get-Item -LiteralPath $outputPath).Length -gt 0) {
            $skillTreeReused++
            continue
        }
        if ([HeroEditorCascExtractor]::TryExtract($storage, $relativePath, $outputPath)) {
            $skillTreeExtracted++
        }
        else {
            $missingSkillTrees.Add($relativePath)
        }
    }
    foreach ($relativePath in $classPortraits) {
        $outputPath = Join-Path $resolvedCache ($relativePath.Replace('/', '\'))
        if ((Test-Path -LiteralPath $outputPath) -and (Get-Item -LiteralPath $outputPath).Length -gt 0) {
            $portraitReused++
            continue
        }
        if ([HeroEditorCascExtractor]::TryExtract($storage, $relativePath, $outputPath)) {
            $portraitExtracted++
        }
        else {
            $missingPortraits.Add($relativePath)
        }
    }
    foreach ($icon in $legacyInventoryIcons) {
        $relativePath = "global/items/$icon.dc6"
        $outputPath = Join-Path $resolvedCache ($relativePath.Replace('/', '\'))
        if ((Test-Path -LiteralPath $outputPath) -and (Get-Item -LiteralPath $outputPath).Length -gt 0) {
            $legacyReused++
            continue
        }
        if ([HeroEditorCascExtractor]::TryExtract($storage, $relativePath, $outputPath)) {
            $legacyExtracted++
        }
        else {
            $missingLegacyIcons.Add($relativePath)
        }
    }
    $legacyPaletteOutput = Join-Path $resolvedCache ($legacyPalette.Replace('/', '\'))
    if ((Test-Path -LiteralPath $legacyPaletteOutput) -and (Get-Item -LiteralPath $legacyPaletteOutput).Length -eq 768) {
        $legacyPaletteReused = $true
    }
    elseif ([HeroEditorCascExtractor]::TryExtract($storage, $legacyPalette, $legacyPaletteOutput)) {
        $legacyPaletteExtracted = $true
    }
}
finally {
    [HeroEditorCascExtractor]::Close($storage)
}

$report = [ordered]@{
    generatedAt = (Get-Date).ToUniversalTime().ToString('o')
    gameDataPath = $resolvedData
    assets = $assets.Count
    extracted = $extracted
    reused = $reused
    missing = @($missing)
    skillAtlases = $skillAtlases.Count
    skillExtracted = $skillExtracted
    skillReused = $skillReused
    missingSkills = @($missingSkills)
    skillTrees = $skillTrees.Count
    skillTreeExtracted = $skillTreeExtracted
    skillTreeReused = $skillTreeReused
    missingSkillTrees = @($missingSkillTrees)
    classPortraits = $classPortraits.Count
    portraitExtracted = $portraitExtracted
    portraitReused = $portraitReused
    missingPortraits = @($missingPortraits)
    legacyInventoryIcons = $legacyInventoryIcons.Count
    legacyExtracted = $legacyExtracted
    legacyReused = $legacyReused
    missingLegacyIcons = @($missingLegacyIcons)
    legacyPalette = $legacyPalette
    legacyPaletteExtracted = $legacyPaletteExtracted
    legacyPaletteReused = $legacyPaletteReused
}
$reportPath = Join-Path $resolvedCache 'extraction-report.json'
New-Item -ItemType Directory -Force -Path $resolvedCache | Out-Null
$reportJson = $report | ConvertTo-Json -Depth 4
[System.IO.File]::WriteAllText($reportPath, $reportJson, [System.Text.UTF8Encoding]::new($false))
Write-Output "Vanilla UI sprites: $extracted extracted, $reused reused, $($missing.Count) missing of $($assets.Count)."
Write-Output "Vanilla skill atlases: $skillExtracted extracted, $skillReused reused, $($missingSkills.Count) missing of $($skillAtlases.Count)."
Write-Output "Vanilla skill trees: $skillTreeExtracted extracted, $skillTreeReused reused, $($missingSkillTrees.Count) missing of $($skillTrees.Count)."
Write-Output "Vanilla class portraits: $portraitExtracted extracted, $portraitReused reused, $($missingPortraits.Count) missing of $($classPortraits.Count)."
Write-Output "Vanilla legacy item visuals: $legacyExtracted extracted, $legacyReused reused, $($missingLegacyIcons.Count) missing of $($legacyInventoryIcons.Count); palette ready: $($legacyPaletteExtracted -or $legacyPaletteReused)."
Write-Output "Cache: $resolvedCache"
