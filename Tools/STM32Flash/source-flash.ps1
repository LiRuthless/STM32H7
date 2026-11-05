param(
    [string]$Firmware = "$PSScriptRoot\firmware\firmware.hex",
    [string]$Target = 'stm32h7x.cfg'
)
$ErrorActionPreference = 'Stop'
$openocd = Join-Path $PSScriptRoot 'openocd\bin\openocd.exe'
$scripts = Join-Path $PSScriptRoot 'openocd\share\openocd\scripts'
if (!(Test-Path -LiteralPath $Firmware -PathType Leaf)) {
    throw "HEX file missing: $Firmware. Copy your firmware to firmware\firmware.hex first."
}
if ([IO.Path]::GetExtension($Firmware) -ne '.hex') { throw 'Only .hex firmware is supported by this entry point.' }
$firmwarePath = (Resolve-Path -LiteralPath $Firmware).Path.Replace('\','/')
if ($firmwarePath -match '[{}\r\n]') { throw 'Firmware path must not contain braces or newlines.' }
& $openocd -s $scripts -f 'interface/stlink.cfg' -f "target/$Target" -c "program {$firmwarePath} verify reset exit"
if ($LASTEXITCODE -ne 0) { throw "Flash failed. OpenOCD exit code: $LASTEXITCODE" }
Write-Host 'Flash verified. Target reset and running.'
