[CmdletBinding()]
param(
    [string]$WorkRoot,
    [switch]$KeepWork
)
$ErrorActionPreference = 'Stop'
if (-not $env:IDF_PYTHON_ENV_PATH) {
    Write-Error 'Activate the specified ESP-IDF v6.0.0 environment first; no tools will be installed.'
    exit 1
}
$bridgePython = Join-Path $env:IDF_PYTHON_ENV_PATH 'Scripts/python.exe'
if (-not (Test-Path -LiteralPath $bridgePython -PathType Leaf)) {
    Write-Error 'Selected IDF Python environment is unavailable.'
    exit 1
}
$bridgeArguments = @((Join-Path $PSScriptRoot 'build_support.py'), 'build')
if ($WorkRoot) { $bridgeArguments += @('--work-root', $WorkRoot) }
if ($KeepWork) { $bridgeArguments += '--keep-work' }
& $bridgePython @bridgeArguments
exit $LASTEXITCODE
