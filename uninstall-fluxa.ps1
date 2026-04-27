param(
    [ValidateSet("User", "Process")]
    [string]$Scope = "User"
)

$installRoot = Join-Path $env:LOCALAPPDATA "Programs\Fluxa"
$binDir = Join-Path $installRoot "bin"
$userPath = if ($Scope -eq "User") {
    [Environment]::GetEnvironmentVariable("Path", "User")
} else {
    $env:Path
}

if ([string]::IsNullOrWhiteSpace($userPath)) {
    Write-Host "Fluxa is not installed in PATH."
} else {
    function Normalize-PathPart([string]$p) {
        if ($null -eq $p) { $p = "" }
        return $p.Trim().TrimEnd("\\")
    }

    $binNorm = Normalize-PathPart $binDir
    $pathParts = $userPath.Split(";") | Where-Object {
        -not [string]::IsNullOrWhiteSpace($_) -and
        -not [System.StringComparer]::OrdinalIgnoreCase.Equals((Normalize-PathPart $_), $binNorm)
    }

    if ($Scope -eq "User") {
        [Environment]::SetEnvironmentVariable("Path", ($pathParts -join ";"), "User")
        Write-Host "Fluxa removed from user PATH."
    } else {
        $env:Path = ($pathParts -join ";")
        Write-Host "Fluxa removed from process PATH."
    }
}

if ($Scope -eq "User") {
    [Environment]::SetEnvironmentVariable("FLUXA_HOME", $null, "User")
    [Environment]::SetEnvironmentVariable("FLUXA_PATH", $null, "User")
} else {
    Remove-Item Env:FLUXA_HOME -ErrorAction SilentlyContinue
    Remove-Item Env:FLUXA_PATH -ErrorAction SilentlyContinue
}

if (Test-Path $installRoot) {
    Remove-Item -Recurse -Force $installRoot
    Write-Host "Removed: $installRoot"
}
