# Explicitly choose existing tools for this project; dot-source from a clean terminal.
[CmdletBinding()]
param(
    [string]$ToolsRoot = 'C:\Espressif\tools',
    [string]$PythonEnv = 'C:\Espressif\tools\python\v6.0\venv'
)
$ErrorActionPreference = 'Stop'
$bridgeIdf = 'D:\esp\v6.0\esp-idf'
if ($env:IDF_PATH -and [IO.Path]::GetFullPath($env:IDF_PATH).TrimEnd('\','/') -ine $bridgeIdf) {
    throw 'Another IDF_PATH is selected. Open a clean terminal; no SDK will be switched automatically.'
}
if ($env:IDF_PYTHON_ENV_PATH -and [IO.Path]::GetFullPath($env:IDF_PYTHON_ENV_PATH).TrimEnd('\','/') -ine [IO.Path]::GetFullPath($PythonEnv).TrimEnd('\','/')) {
    throw 'Another IDF Python environment is selected. Open a clean terminal.'
}
$bridgeMetadata = Get-Content -LiteralPath (Join-Path $bridgeIdf 'tools/tools.json') -Raw | ConvertFrom-Json
$bridgePaths = @((Join-Path $PythonEnv 'Scripts'))
foreach ($bridgeName in @('xtensa-esp-elf','cmake','ninja')) {
    $bridgeTool = $bridgeMetadata.tools | Where-Object { $_.name -eq $bridgeName }
    $bridgeVersion = @($bridgeTool.versions | Where-Object { $_.status -eq 'recommended' })
    if ($bridgeVersion.Count -ne 1) { throw 'SDK recommended tool version is ambiguous.' }
    $bridgeBase = Join-Path (Join-Path $ToolsRoot $bridgeName) $bridgeVersion[0].name
    foreach ($bridgeRelative in $bridgeTool.export_paths) {
        $bridgeToolPath = $bridgeBase
        foreach ($bridgeSegment in $bridgeRelative) {
            if ($bridgeSegment) { $bridgeToolPath = Join-Path $bridgeToolPath $bridgeSegment }
        }
        if (-not (Test-Path -LiteralPath $bridgeToolPath -PathType Container)) { throw "Required installed tool directory is missing: $bridgeName" }
        $bridgePaths += $bridgeToolPath
    }
}
$bridgeSelectedPython = Join-Path $PythonEnv 'Scripts/python.exe'
if (-not (Test-Path -LiteralPath $bridgeSelectedPython -PathType Leaf)) { throw 'Existing IDF Python interpreter is missing.' }
$bridgeNew = @{
    PATH = (($bridgePaths -join ';') + ';' + $env:PATH)
    IDF_PATH = $bridgeIdf
    IDF_TOOLS_PATH = $ToolsRoot
    IDF_PYTHON_ENV_PATH = $PythonEnv
    IDF_COMPONENT_MANAGER = '0'
    IDF_CCACHE_ENABLE = '0'
}
$bridgePrevious = @{}
try {
    foreach ($bridgeKey in $bridgeNew.Keys) {
        $bridgePrevious[$bridgeKey] = [Environment]::GetEnvironmentVariable($bridgeKey, 'Process')
        [Environment]::SetEnvironmentVariable($bridgeKey, $bridgeNew[$bridgeKey], 'Process')
    }
    & $bridgeSelectedPython (Join-Path $PSScriptRoot 'build_support.py') environment
    if ($LASTEXITCODE -ne 0) { throw 'Selected tools failed the v6.0.0 environment check.' }
} catch {
    foreach ($bridgeKey in $bridgePrevious.Keys) {
        if ($null -eq $bridgePrevious[$bridgeKey]) {
            Remove-Item -LiteralPath ('Env:' + $bridgeKey) -ErrorAction SilentlyContinue
        } else {
            Set-Item -LiteralPath ('Env:' + $bridgeKey) -Value $bridgePrevious[$bridgeKey]
        }
    }
    throw
}
Write-Host 'ESP32Bridge v6.0.0 tools selected and checked; no installation or flashing performed.'
