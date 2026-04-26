param()

$projectRoot = $PSScriptRoot
$exePath = Join-Path $projectRoot "fluxa.exe"

if (-not (Test-Path $exePath)) {
    & (Join-Path $projectRoot "build-fluxa-launcher.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

$userPath = [Environment]::GetEnvironmentVariable("Path", "User")
$pathParts = @()

if (-not [string]::IsNullOrWhiteSpace($userPath)) {
    $pathParts = $userPath.Split(";") | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
}

$alreadyInstalled = $false
foreach ($part in $pathParts) {
    if ([System.StringComparer]::OrdinalIgnoreCase.Equals($part.TrimEnd("\"), $projectRoot.TrimEnd("\"))) {
        $alreadyInstalled = $true
        break
    }
}

if (-not $alreadyInstalled) {
    $newPath = if ($pathParts.Count -gt 0) {
        ($pathParts + $projectRoot) -join ";"
    }
    else {
        $projectRoot
    }

    [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
}

$env:Path = "$projectRoot;$env:Path"

Write-Host "Fluxa command installed."
Write-Host "Project root added to user PATH: $projectRoot"
Write-Host "Open a new terminal and run: fluxa version"
