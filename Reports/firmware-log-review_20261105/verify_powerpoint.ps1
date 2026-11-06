$ErrorActionPreference = 'Stop'
$reviewRoot = $PSScriptRoot
$reviewDeck = Get-ChildItem -LiteralPath (Join-Path $reviewRoot 'exports') -Filter '*.pptx' | Sort-Object LastWriteTime -Descending | Select-Object -First 1
$reviewRender = Join-Path $reviewRoot 'powerpoint-preview'
New-Item -ItemType Directory -Force -Path $reviewRender | Out-Null
$priorPowerPoint = @(Get-Process POWERPNT -ErrorAction SilentlyContinue).Count
$reviewApp = New-Object -ComObject PowerPoint.Application
$reviewPresentation = $null
try {
    $reviewPresentation = $reviewApp.Presentations.Open($reviewDeck.FullName, -1, 0, 0)
    $reviewPresentation.Export($reviewRender, 'PNG', 1280, 720)
    $reviewPresentation.SaveAs((Join-Path $reviewRoot 'exports/固件日志记录与上传.pdf'), 32)
    Write-Output ('PowerPoint opened and rendered {0} slides.' -f $reviewPresentation.Slides.Count)
}
finally {
    if ($null -ne $reviewPresentation) { $reviewPresentation.Close() }
    if ($priorPowerPoint -eq 0) { $reviewApp.Quit() }
}
