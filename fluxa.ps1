param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$FluxaArgs
)

$exe = Join-Path $PSScriptRoot "fluxa.exe"

if (-not (Test-Path $exe)) {
    & (Join-Path $PSScriptRoot "build-fluxa-launcher.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

& $exe @FluxaArgs
exit $LASTEXITCODE
