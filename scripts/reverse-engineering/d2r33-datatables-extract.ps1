[CmdletBinding(PositionalBinding = $false)]
param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Arguments
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$pythonCandidates = @(
    (Join-Path $repositoryRoot 'reverse-engineering\d2r-3.2.92777\analysis-cache\python\Scripts\python.exe'),
    (Join-Path $env:LOCALAPPDATA 'Programs\Python\Python312\python.exe'),
    (Join-Path $env:LOCALAPPDATA 'Programs\Python\Python313\python.exe'),
    (Join-Path $env:LOCALAPPDATA 'Programs\Python\Python311\python.exe')
)
$python = $pythonCandidates |
    Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
    Select-Object -First 1
if ([string]::IsNullOrWhiteSpace($python)) {
    throw 'The governed D2R reverse-engineering Python environment was not found.'
}

$scriptPath = Join-Path $PSScriptRoot 'd2r33-datatables-extract.py'
$env:PYTHONUTF8 = '1'
& $python $scriptPath @Arguments
exit $LASTEXITCODE
