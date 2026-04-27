#Requires -Version 5.1
param(
    [switch]$IncludeExamples
)

$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot

function Write-Step([string]$msg) {
    Write-Host "  >> $msg" -ForegroundColor Cyan
}

function Write-Ok([string]$msg) {
    Write-Host "  [ok] $msg" -ForegroundColor Green
}

function Write-Fail([string]$msg) {
    Write-Host "  [fail] $msg" -ForegroundColor Red
}

Write-Host ""
Write-Host "=====================================" -ForegroundColor White
Write-Host "  Fluxa Installer" -ForegroundColor White
Write-Host "=====================================" -ForegroundColor White
Write-Host ""

# --- Paths ---
$installRoot = Join-Path $env:LOCALAPPDATA "Programs\Fluxa"
$binDir      = Join-Path $installRoot "bin"
$libDir      = Join-Path $installRoot "lib"
$examplesDir = Join-Path $installRoot "examples"
$srcDir      = Join-Path $installRoot "src"
$destExe     = Join-Path $binDir "fluxa.exe"

# --- Step 1: Build fluxa.exe from source ---
Write-Step "Building fluxa.exe from source..."

$sourcePath = Join-Path $projectRoot "fluxa_launcher.c"
if (-not (Test-Path $sourcePath)) {
    Write-Fail "fluxa_launcher.c not found in: $projectRoot"
    exit 1
}

$gccCheck = Get-Command gcc -ErrorAction SilentlyContinue
if (-not $gccCheck) {
    Write-Fail "gcc not found. Install MinGW-w64 and add it to PATH."
    exit 1
}

$builtExe = Join-Path $projectRoot "fluxa.exe"
$tempExe  = Join-Path $projectRoot "fluxa.build.exe"

gcc $sourcePath -O2 -lm -o $tempExe 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Fail "Build failed."
    exit 1
}

Write-Ok "Build successful."

# --- Step 2: Create directories ---
Write-Step "Creating install directories..."

New-Item -ItemType Directory -Force -Path $binDir      | Out-Null
New-Item -ItemType Directory -Force -Path $libDir      | Out-Null
New-Item -ItemType Directory -Force -Path $srcDir      | Out-Null

Write-Ok "Directories created: $installRoot"

# --- Step 3: Copy executable ---
Write-Step "Copying fluxa.exe to $binDir..."

Copy-Item -Force -Path $tempExe -Destination $destExe
Remove-Item -Force -Path $tempExe -ErrorAction SilentlyContinue

Write-Ok "fluxa.exe installed."

# --- Step 4: Copy source ---
Copy-Item -Force -Path $sourcePath -Destination (Join-Path $srcDir "fluxa_launcher.c")

# --- Step 5: Copy .flx libraries ---
Write-Step "Copying standard library (.flx files)..."

$flxFiles = Get-ChildItem -Path $projectRoot -Filter "*.flx" -File -ErrorAction SilentlyContinue
$count = 0
foreach ($f in $flxFiles) {
    Copy-Item -Force -Path $f.FullName -Destination (Join-Path $libDir $f.Name)
    $count++
}

Write-Ok "$count library file(s) copied."

# --- Step 6: Copy package registry ---
$registrySrc = Join-Path $projectRoot "fluxa-packages.txt"
if (-not (Test-Path $registrySrc)) {
    $registrySrc = Join-Path $projectRoot "text.txt"
}
if (Test-Path $registrySrc) {
    Copy-Item -Force -Path $registrySrc -Destination (Join-Path $installRoot "fluxa-packages.txt")
    Write-Ok "Package registry copied."
}

# --- Step 7: Copy examples (optional) ---
$examplesSrc = Join-Path $projectRoot "examples"
if ($IncludeExamples -and (Test-Path $examplesSrc)) {
    Write-Step "Copying examples..."
    if (Test-Path $examplesDir) {
        Remove-Item -Recurse -Force $examplesDir
    }
    Copy-Item -Recurse -Force -Path $examplesSrc -Destination $examplesDir
    Write-Ok "Examples copied."
}

# --- Step 8: Add to PATH ---
Write-Step "Adding Fluxa to user PATH..."

function Normalize-Part([string]$p) {
    return $p.Trim().TrimEnd("\")
}

$userPath = [Environment]::GetEnvironmentVariable("Path", "User")
$parts = @()
if (-not [string]::IsNullOrWhiteSpace($userPath)) {
    $parts = $userPath.Split(";") | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
}

$binNorm = Normalize-Part $binDir
$alreadyInPath = $parts | Where-Object {
    [System.StringComparer]::OrdinalIgnoreCase.Equals((Normalize-Part $_), $binNorm)
}

if (-not $alreadyInPath) {
    $newPath = ($parts + $binDir) -join ";"
    [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
    Write-Ok "Added to PATH."
} else {
    Write-Ok "Already in PATH."
}

$env:Path = "$binDir;$env:Path"

# --- Step 9: Set environment variables ---
Write-Step "Setting FLUXA_HOME and FLUXA_PATH..."

[Environment]::SetEnvironmentVariable("FLUXA_HOME", $installRoot, "User")
[Environment]::SetEnvironmentVariable("FLUXA_PATH", $libDir, "User")
$env:FLUXA_HOME = $installRoot
$env:FLUXA_PATH = $libDir

Write-Ok "Environment variables set."

# --- Done ---
Write-Host ""
Write-Host "=====================================" -ForegroundColor White
Write-Host "  Installation complete!" -ForegroundColor Green
Write-Host "=====================================" -ForegroundColor White
Write-Host ""
Write-Host "  Home : $installRoot"
Write-Host "  Bin  : $binDir"
Write-Host "  Lib  : $libDir"
Write-Host ""
Write-Host "  Open a new terminal and run:" -ForegroundColor Yellow
Write-Host "    fluxa version" -ForegroundColor Yellow
Write-Host ""
