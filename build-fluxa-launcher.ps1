param()

$source = Join-Path $PSScriptRoot "fluxa_launcher.c"
$output = Join-Path $PSScriptRoot "fluxa.exe"
$tempOutput = Join-Path $PSScriptRoot "fluxa.new.exe"

if (Test-Path $tempOutput) {
    Remove-Item -Force $tempOutput -ErrorAction SilentlyContinue
}

gcc $source -O2 -o $tempOutput

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to build fluxa.exe"
    exit $LASTEXITCODE
}

try {
    if (Test-Path $output) {
        Remove-Item -Force -ErrorAction Stop $output
    }
    Move-Item -Force -ErrorAction Stop -Path $tempOutput -Destination $output
    Write-Host "Built $output"
}
catch {
    Write-Warning "Built $tempOutput but could not replace $output (it may be running). Close Fluxa/VS Code that is using it and then rename manually."
    Write-Host "Built $tempOutput"
}
