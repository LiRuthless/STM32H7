param(
    [string]$Firmware = "$PSScriptRoot\..\..\SmartCar\MDK-ARM\SmartCar\SmartCar.hex",
    [switch]$CheckOnly
)
$ErrorActionPreference = 'Stop'
try {
    $openocd = Join-Path $PSScriptRoot 'openocd\bin\openocd.exe'
    $scripts = Join-Path $PSScriptRoot 'openocd\share\openocd\scripts'
    foreach ($required in @($openocd,
            (Join-Path $scripts 'interface\stlink.cfg'),
            (Join-Path $scripts 'target\stm32h7x.cfg'))) {
        if (!(Test-Path -LiteralPath $required -PathType Leaf)) {
            throw "Tool file missing: $required. Restore openocd from the supplied ZIP; see README.md."
        }
    }
    if (!(Test-Path -LiteralPath $Firmware -PathType Leaf)) {
        throw "HEX file missing: $Firmware. Build SmartCar in Keil first."
    }
    if ([IO.Path]::GetExtension($Firmware) -ine '.hex') {
        throw 'Only .hex firmware is supported.'
    }
    $firmwarePath = (Resolve-Path -LiteralPath $Firmware).ProviderPath.Replace('\', '/')
    if ($firmwarePath -match '[{}\r\n]') {
        throw 'Firmware path must not contain braces or newlines.'
    }
    $program = "program {$firmwarePath} verify reset exit"
    Write-Host "Firmware: $firmwarePath"
    Write-Host "OpenOCD: $openocd"
    if ($CheckOnly) {
        Write-Host "Check only (no device access): $program"
        exit 0
    }
    & $openocd -s $scripts -f 'interface/stlink.cfg' -f 'target/stm32h7x.cfg' -c $program
    if ($LASTEXITCODE -ne 0) {
        throw "Flash failed. OpenOCD exit code: $LASTEXITCODE"
    }
    Write-Host 'Flash verified. Target reset and running.'
    exit 0
}
catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
}
