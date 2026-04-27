param(
    [ValidateSet("User", "Process")]
    [string]$Scope = "User",
    [switch]$IncludeExamples
)

$projectRoot = $PSScriptRoot
$exePath = Join-Path $projectRoot "fluxa.exe"
$exeNewPath = Join-Path $projectRoot "fluxa.new.exe"

if (-not (Test-Path $exePath)) {
    & (Join-Path $projectRoot "build-fluxa-launcher.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

$exeToInstall = $exePath
if (Test-Path $exeNewPath) {
    $candidates = @(
        Get-Item -ErrorAction SilentlyContinue $exePath
        Get-Item -ErrorAction SilentlyContinue $exeNewPath
    ) | Where-Object { $_ -ne $null }

    if ($candidates.Count -gt 0) {
        $exeToInstall = ($candidates | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
    }
}

$installRoot = Join-Path $env:LOCALAPPDATA "Programs\Fluxa"
$binDir = Join-Path $installRoot "bin"
$libDir = Join-Path $installRoot "lib"
$examplesDir = Join-Path $installRoot "examples"

New-Item -ItemType Directory -Force -Path $binDir | Out-Null
New-Item -ItemType Directory -Force -Path $libDir | Out-Null

# Copy the CLI executable
$destExe = Join-Path $binDir "fluxa.exe"
Copy-Item -Force -Path $exeToInstall -Destination $destExe

# Copy root-level libraries (.flx) into lib/
$rootLibs = Get-ChildItem -Path $projectRoot -Filter "*.flx" -File -ErrorAction SilentlyContinue
foreach ($f in $rootLibs) {
    Copy-Item -Force -Path $f.FullName -Destination (Join-Path $libDir $f.Name)
}

# Copy package registry (optional)
$registrySrc = Join-Path $projectRoot "fluxa-packages.txt"
if (-not (Test-Path $registrySrc)) {
    $registrySrc = Join-Path $projectRoot "text.txt"
}
if (Test-Path $registrySrc) {
    Copy-Item -Force -Path $registrySrc -Destination (Join-Path $installRoot "fluxa-packages.txt")
}

if ($IncludeExamples) {
    if (Test-Path (Join-Path $projectRoot "examples")) {
        if (Test-Path $examplesDir) {
            Remove-Item -Recurse -Force $examplesDir
        }
        Copy-Item -Recurse -Force -Path (Join-Path $projectRoot "examples") -Destination $examplesDir
    }
}

function Normalize-PathPart([string]$p) {
    if ($null -eq $p) { $p = "" }
    return $p.Trim().TrimEnd("\\")
}

if ($Scope -eq "User") {
    # Add bin/ to user PATH (so `fluxa` works everywhere)
    $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
    $pathParts = @()
    if (-not [string]::IsNullOrWhiteSpace($userPath)) {
        $pathParts = $userPath.Split(";") | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    }

    $binNorm = Normalize-PathPart $binDir
    $alreadyInPath = $false
    foreach ($part in $pathParts) {
        if ([System.StringComparer]::OrdinalIgnoreCase.Equals((Normalize-PathPart $part), $binNorm)) {
            $alreadyInPath = $true
            break
        }
    }

    if (-not $alreadyInPath) {
        $newPath = if ($pathParts.Count -gt 0) { ($pathParts + $binDir) -join ";" } else { $binDir }
        [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
    }
}

# Helpful env vars for module discovery
if ($Scope -eq "User") {
    [Environment]::SetEnvironmentVariable("FLUXA_HOME", $installRoot, "User")
    [Environment]::SetEnvironmentVariable("FLUXA_PATH", $libDir, "User")
} else {
    $env:FLUXA_HOME = $installRoot
    $env:FLUXA_PATH = $libDir
}

# Update current shell session PATH too (always)
$env:Path = "$binDir;$env:Path"

Write-Host "Fluxa installed."
Write-Host "Home: $installRoot"
Write-Host "Bin:  $binDir"
Write-Host "Lib:  $libDir"
Write-Host ""
Write-Host "Open a new terminal and run: fluxa version"
