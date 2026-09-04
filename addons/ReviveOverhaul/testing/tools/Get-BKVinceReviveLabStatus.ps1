param(
    [switch]$AsObject
)

$ErrorActionPreference = 'Stop'

$harnessRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$testingRoot = Split-Path -Parent $harnessRoot
$bkvinceRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $testingRoot))
$gameRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent (Split-Path -Parent $bkvinceRoot)))
$pluginPath = Join-Path $bkvinceRoot 'd2rloader\plugins\d2rl-ruffneckk-revive-overhaul.dll'
$configPath = Join-Path $bkvinceRoot 'd2rloader\config\ruffneckk-revive-overhaul.toml'
$scriptedAiPluginPath = Join-Path $bkvinceRoot 'd2rloader\plugins\d2rl-ruffneckk-scripted-ai.dll'
$scriptedAiConfigPath = Join-Path $bkvinceRoot 'd2rloader\config\ruffneckk-scripted-ai.toml'
$scriptedAiTreePath = Join-Path $bkvinceRoot 'd2rloader\scripts\ruffneckk-scripted-ai\revive-companion.lua'
$modInfoPath = Join-Path $bkvinceRoot 'BKVince.mpq\modinfo.json'
$skillsPath = Join-Path $bkvinceRoot 'BKVince.mpq\data\global\excel\skills.txt'
$d2rPath = Join-Path $gameRoot 'D2R.exe'
$loaderPath = Join-Path $gameRoot 'D2RLoader.exe'
$globalLoaderConfigPath = Join-Path $gameRoot 'd2rloader\config\d2rloader.toml'
$modLoaderConfigPath = Join-Path $bkvinceRoot 'd2rloader\config\d2rloader.toml'
$separateModPath = Join-Path $gameRoot 'mods\ReviveOverhaulLab93847'
$separateSavePath = Join-Path $env:USERPROFILE 'Saved Games\Diablo II Resurrected\mods\ReviveOverhaulLab93847'

function Get-Sha256([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

function Get-InventoryDigest([string]$Path) {
    [string[]]$names = Get-ChildItem -LiteralPath $Path -File -ErrorAction Stop |
        ForEach-Object Name
    [Array]::Sort($names, [System.StringComparer]::Ordinal)
    $lines = foreach ($name in $names) {
        $item = Get-Item -LiteralPath (Join-Path $Path $name)
        '{0}|{1}|{2}' -f $item.Name, $item.Length, (Get-Sha256 $item.FullName)
    }
    $text = [string]::Join("`n", $lines)
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($text)
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        return -join ($sha.ComputeHash($bytes) | ForEach-Object { $_.ToString('X2') })
    }
    finally {
        $sha.Dispose()
    }
}

if ((Split-Path -Leaf $bkvinceRoot) -ne 'BKVince') {
    throw "The harness is not deployed inside the BKVince profile: $bkvinceRoot"
}

$latin1 = [System.Text.Encoding]::GetEncoding(28591)
$lines = [System.IO.File]::ReadAllLines($skillsPath, $latin1)
$headers = $lines[0].Split("`t")
$revive = $lines | Select-Object -Skip 1 | Where-Object { $_.Split("`t")[0] -eq 'Revive' } | Select-Object -First 1
if (-not $revive) {
    throw 'Revive row is missing from the active BKVince Skills.txt.'
}
$cells = $revive.Split("`t")
function Get-Cell([string]$Name) {
    $index = -1
    for ($candidate = 0; $candidate -lt $headers.Count; $candidate++) {
        if ($headers[$candidate].Equals($Name, [System.StringComparison]::OrdinalIgnoreCase)) {
            $index = $candidate
            break
        }
    }
    if ($index -lt 0) {
        throw "Missing Skills.txt column: $Name"
    }
    return $cells[$index]
}

$contract = [ordered]@{
    SrvStFunc = Get-Cell 'srvstfunc'
    SrvDoFunc = Get-Cell 'srvdofunc'
    CltStFunc = Get-Cell 'cltstfunc'
    CltDoFunc = Get-Cell 'cltdofunc'
    SelectProc = Get-Cell 'selectproc'
    TargetCorpse = Get-Cell 'targetcorpse'
    PetType = Get-Cell 'pettype'
}
$contractMatches =
    $contract.SrvStFunc -eq '21' -and
    $contract.SrvDoFunc -eq '58' -and
    $contract.CltStFunc -eq '24' -and
    $contract.CltDoFunc -eq '' -and
    $contract.SelectProc -eq '3' -and
    $contract.TargetCorpse -eq '1' -and
    $contract.PetType -eq 'revive'

$modInfo = Get-Content -LiteralPath $modInfoPath -Raw | ConvertFrom-Json
$runtimeProcesses = @(Get-Process -Name D2R,D2RLoader -ErrorAction SilentlyContinue)
$pluginHash = Get-Sha256 $pluginPath
$configHash = Get-Sha256 $configPath
$scriptedAiPluginHash = Get-Sha256 $scriptedAiPluginPath
$scriptedAiConfigHash = Get-Sha256 $scriptedAiConfigPath
$scriptedAiTreeHash = Get-Sha256 $scriptedAiTreePath
$d2rHash = Get-Sha256 $d2rPath
$loaderHash = Get-Sha256 $loaderPath
$modPluginDigest = Get-InventoryDigest (Join-Path $bkvinceRoot 'd2rloader\plugins')
$modPatchDigest = Get-InventoryDigest (Join-Path $bkvinceRoot 'd2rloader\patches')
$globalPluginDigest = Get-InventoryDigest (Join-Path $gameRoot 'd2rloader\plugins')
$globalPatchDigest = Get-InventoryDigest (Join-Path $gameRoot 'd2rloader\patches')
$globalLoaderConfig = Get-Content -LiteralPath $globalLoaderConfigPath -Raw
$modLoaderConfig = Get-Content -LiteralPath $modLoaderConfigPath -Raw
$eezstreetPlugins = @(
    'plugin-items.dll',
    'plugin-levels.dll',
    'plugin-misc.dll',
    'plugin-quests.dll',
    'plugin-skills.dll'
)
$eezstreetBaselinePresent = $true
foreach ($plugin in $eezstreetPlugins) {
    if (-not (Test-Path -LiteralPath (Join-Path $gameRoot ('d2rloader\plugins\' + $plugin)) -PathType Leaf)) {
        $eezstreetBaselinePresent = $false
    }
}

$checks = [ordered]@{
    runtimeStopped = $runtimeProcesses.Count -eq 0
    bkvinceIdentity = $modInfo.name -eq 'BKVince' -and $modInfo.savepath -eq 'BKVince/'
    noSeparateReviveMod = -not (Test-Path -LiteralPath $separateModPath)
    noSeparateReviveSavePath = -not (Test-Path -LiteralPath $separateSavePath)
    skillsContract = $contractMatches
    pluginVersion = (Get-Item -LiteralPath $pluginPath).VersionInfo.FileVersion -eq '2.3.0'
    pluginHash = $pluginHash -eq 'B4C6D3BEDBB798D2553935F25D528A80F03CBAD8FE87255BDA058E9E4A6931CC'
    labConfigHash = $configHash -eq 'CBC057F3E663CA51CE8DB04817A505D67517340673E127164B93B2E136D16401'
    scriptedAiVersion = (Get-Item -LiteralPath $scriptedAiPluginPath).VersionInfo.FileVersion -eq '0.7.0'
    scriptedAiHash = $scriptedAiPluginHash -eq 'F633EFACEABFB0DB1BAA96CF31D71A37F7EFA343B8566155F259F2E0239E7F39'
    scriptedAiLabConfigHash = $scriptedAiConfigHash -eq 'B2949EB953768EE39B67829D1DDDCD17CDF577B5B2A1258EA8B2F87229A671B9'
    scriptedAiTreeHash = $scriptedAiTreeHash -eq '72A39C2262B97A7F1BE3B6089663AA6A5A54147867B5D125F37931497020325C'
    d2r93847Image = $d2rHash -eq 'E1F5436E3D9687F644EF16938B1B183D1FDEF434F18CF66D852CF68F48CC8936'
    loader120 = (Get-Item -LiteralPath $loaderPath).VersionInfo.FileVersion -eq '1.2.0-beta'
    loaderHash = $loaderHash -eq '651FA9EB33083088349224B1624819F63ED79596F808950CF6468B5D82F7132E'
    bkvincePlugins = $modPluginDigest -eq 'E49445D88A46ECC0E4038CCF014ED7BBDDD84E9FA490D8169EC41D07790106E4'
    bkvincePatches = $modPatchDigest -eq 'F283705AA1D3D9AFFD3F36810EFCDF0175F2EFA5E46643C3B38B60DB5F624E4F'
    globalPlugins = $globalPluginDigest -eq '2D3F9AC98FEBE92D03BA75CD953B761473763ECB01029A01B4DCBA5AB639E4DE'
    globalPatches = $globalPatchDigest -eq '81D1479662117E800E17F2DCDFCE2596B80477D950F5C964EA223641207A2940'
    globalExtensionsEnabled = $globalLoaderConfig -match '(?m)^[ \t]*allow_global_extensions[ \t]*=[ \t]*true[ \t]*$'
    modExtensionsEnabled = $globalLoaderConfig -match '(?m)^[ \t]*allow_mod_extensions[ \t]*=[ \t]*true[ \t]*$'
    developerConsoleEnabled = $globalLoaderConfig -match '(?m)^[ \t]*enable_console[ \t]*=[ \t]*true[ \t]*$'
    bkvinceDoesNotSuppressGlobal = $modLoaderConfig -notmatch '(?m)^[ \t]*suppress_global_extensions[ \t]*=[ \t]*true[ \t]*$'
    debugToolsAvailable = -not (Test-Path -LiteralPath (Join-Path $bkvinceRoot 'BKVince.mpq\data\global\.DISABLE_DEBUG'))
    eezstreetBaselinePresent = $eezstreetBaselinePresent
}

$result = [pscustomobject]@{
    state = 'PREPARED_NOT_RUN'
    profile = 'BKVince'
    readyForAuthorizedTest = -not ($checks.Values -contains $false)
    runtimeProcesses = $runtimeProcesses.Count
    checks = $checks
    skillsContract = $contract
    artifacts = [ordered]@{
        pluginVersion = (Get-Item -LiteralPath $pluginPath).VersionInfo.FileVersion
        pluginSha256 = $pluginHash
        activeConfigSha256 = $configHash
        productionConfigSha256 = Get-Sha256 (Join-Path $harnessRoot 'config\production.toml')
        scriptedAiVersion = (Get-Item -LiteralPath $scriptedAiPluginPath).VersionInfo.FileVersion
        scriptedAiSha256 = $scriptedAiPluginHash
        scriptedAiConfigSha256 = $scriptedAiConfigHash
        scriptedAiTreeSha256 = $scriptedAiTreeHash
        d2rSha256 = $d2rHash
        loaderVersion = (Get-Item -LiteralPath $loaderPath).VersionInfo.FileVersion
        loaderSha256 = $loaderHash
        bkvincePluginInventory = $modPluginDigest
        bkvincePatchInventory = $modPatchDigest
        globalPluginInventory = $globalPluginDigest
        globalPatchInventory = $globalPatchDigest
    }
}

if ($AsObject) {
    return $result
}

$result | ConvertTo-Json -Depth 8
