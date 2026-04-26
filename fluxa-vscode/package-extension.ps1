param()

Push-Location $PSScriptRoot

if (-not (Test-Path "node_modules")) {
    npm install
    if ($LASTEXITCODE -ne 0) {
        Pop-Location
        exit $LASTEXITCODE
    }
}

npx vsce package
$code = $LASTEXITCODE
Pop-Location
exit $code
