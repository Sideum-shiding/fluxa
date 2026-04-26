param()

$source = Join-Path $PSScriptRoot "fluxa_launcher.c"
$output = Join-Path $PSScriptRoot "fluxa.exe"

if (Test-Path $output) {
    Remove-Item $output -Force
}

gcc $source -O2 -o $output

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to build fluxa.exe"
    exit $LASTEXITCODE
}

Write-Host "Built $output"
