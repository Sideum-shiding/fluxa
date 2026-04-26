param()

$projectRoot = $PSScriptRoot.TrimEnd("\")
$userPath = [Environment]::GetEnvironmentVariable("Path", "User")

if ([string]::IsNullOrWhiteSpace($userPath)) {
    Write-Host "Fluxa is not installed in user PATH."
    exit 0
}

$pathParts = $userPath.Split(";") | Where-Object {
    -not [string]::IsNullOrWhiteSpace($_) -and
    -not [System.StringComparer]::OrdinalIgnoreCase.Equals($_.TrimEnd("\"), $projectRoot)
}

[Environment]::SetEnvironmentVariable("Path", ($pathParts -join ";"), "User")
Write-Host "Fluxa removed from user PATH."
