[CmdletBinding(PositionalBinding = $false)]
param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Arguments
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$previousVersion = $env:D2R_RUNTIME_VERSION
$previousCommand = $env:D2R_RE_COMMAND
$env:D2R_RUNTIME_VERSION = '3.3.93847'
$env:D2R_RE_COMMAND = 're:d2r33'

try {
    & (Join-Path $PSScriptRoot 'd2r32.ps1') @Arguments
    exit $LASTEXITCODE
} finally {
    $env:D2R_RUNTIME_VERSION = $previousVersion
    $env:D2R_RE_COMMAND = $previousCommand
}
