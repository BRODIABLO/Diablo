[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-VsWherePath {
    $candidates = @(
        (Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'),
        (Join-Path $env:ProgramFiles 'Microsoft Visual Studio\Installer\vswhere.exe')
    )
    $path = $candidates |
        Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
        Select-Object -First 1
    if ([string]::IsNullOrWhiteSpace($path)) {
        throw 'vswhere.exe was not found. Install Visual Studio 2022 Build Tools with the MSVC x64 component.'
    }
    return $path
}

function Get-VsToolPath {
    param(
        [Parameter(Mandatory)]
        [string]$VsWhere,
        [Parameter(Mandatory)]
        [string]$Pattern
    )
    $path = & $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find $Pattern |
        Select-Object -First 1
    if ([string]::IsNullOrWhiteSpace($path) -or -not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Visual Studio tool not found through vswhere: $Pattern"
    }
    return [IO.Path]::GetFullPath($path)
}

function Import-MsvcEnvironment {
    param(
        [Parameter(Mandatory)]
        [string]$VcVarsPath
    )
    $commandLine = "`"$VcVarsPath`" >nul && set"
    $environmentLines = & $env:ComSpec /d /s /c $commandLine
    if ($LASTEXITCODE -ne 0) {
        throw "MSVC environment initialization failed with exit code $LASTEXITCODE."
    }
    foreach ($line in $environmentLines) {
        if ($line -match '^([^=]+)=(.*)$') {
            [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process')
        }
    }
}

$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$generatedRoot = Join-Path $repositoryRoot 'reverse-engineering\d2r-3.2.92777\datatables-atlas\generated'
$witness = Join-Path $generatedRoot 'd2r33_datatables_atlas_witness.cpp'
$buildRoot = Join-Path $repositoryRoot 'analysis-cache\d2r33-datatables-atlas-a2\compile'
$objectFile = Join-Path $buildRoot 'd2r33_datatables_atlas_witness.obj'
$generator = Join-Path $PSScriptRoot 'd2r33-datatables-generate.mjs'

& node $generator --check
if ($LASTEXITCODE -ne 0) {
    throw "Generated atlas validation failed with exit code $LASTEXITCODE."
}

$vswhere = Get-VsWherePath
$vcVars = Get-VsToolPath -VsWhere $vswhere -Pattern 'VC\Auxiliary\Build\vcvars64.bat'
$vsVersion = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion |
    Select-Object -First 1
Import-MsvcEnvironment -VcVarsPath $vcVars

$compiler = Get-Command 'cl.exe' -ErrorAction SilentlyContinue
if ($null -eq $compiler) {
    throw 'cl.exe is still unavailable after importing vcvars64.bat.'
}

New-Item -ItemType Directory -Path $buildRoot -Force | Out-Null
$arguments = @(
    '/nologo',
    '/std:c++20',
    '/W4',
    '/WX',
    '/EHsc',
    '/c',
    $witness,
    "/Fo$objectFile"
)
& $compiler.Source @arguments
if ($LASTEXITCODE -ne 0) {
    throw "DataTables atlas witness compilation failed with exit code $LASTEXITCODE."
}

$objectHash = (Get-FileHash -LiteralPath $objectFile -Algorithm SHA256).Hash
Write-Output "visualStudio=$vsVersion"
Write-Output "compiler=$($compiler.Source)"
Write-Output "object=$objectFile"
Write-Output "objectSha256=$objectHash"
Write-Output 'PASS D2R 3.3 DataTables atlas generated static_assert witness.'
