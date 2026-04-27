#Requires -Version 5.1
param()

$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot

function Write-Step([string]$msg) { Write-Host "  >> $msg" -ForegroundColor Cyan }
function Write-Ok([string]$msg)   { Write-Host "  [ok] $msg" -ForegroundColor Green }
function Write-Fail([string]$msg) { Write-Host "  [fail] $msg" -ForegroundColor Red; exit 1 }

Write-Host ""
Write-Host "=====================================" -ForegroundColor White
Write-Host "  Fluxa Installer Builder" -ForegroundColor White
Write-Host "=====================================" -ForegroundColor White
Write-Host ""

# --- Check gcc ---
if (-not (Get-Command gcc -ErrorAction SilentlyContinue)) {
    Write-Fail "gcc not found. Install MinGW-w64 and add it to PATH."
}

# --- Build fluxa.exe if not present ---
$fluxaExe = Join-Path $projectRoot "fluxa.exe"
$fluxaSrc = Join-Path $projectRoot "fluxa_launcher.c"

if (-not (Test-Path $fluxaSrc)) {
    Write-Fail "fluxa_launcher.c not found."
}

Write-Step "Building fluxa.exe..."
$tempExe = Join-Path $projectRoot "fluxa.tmp.exe"
gcc $fluxaSrc -O2 -lm -o $tempExe 2>&1
if ($LASTEXITCODE -ne 0) { Write-Fail "Failed to build fluxa.exe" }
Move-Item -Force $tempExe $fluxaExe
Write-Ok "fluxa.exe built."

# --- Collect .flx library files ---
$libFiles = Get-ChildItem -Path $projectRoot -Filter "*.flx" -File -ErrorAction SilentlyContinue

# --- Helper: convert file bytes to C array ---
function ConvertTo-CArray([string]$path) {
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $chunks = for ($i = 0; $i -lt $bytes.Length; $i += 16) {
        $line = $bytes[$i..([Math]::Min($i + 15, $bytes.Length - 1))] |
                ForEach-Object { "0x{0:x2}" -f $_ }
        "    " + ($line -join ", ")
    }
    return $chunks -join ",`n"
}

# --- Read version from fluxa_launcher.c ---
$version = "0.4.0"
$srcContent = Get-Content $fluxaSrc -Raw -ErrorAction SilentlyContinue
if ($srcContent -match '#define FLUXA_VERSION "([^"]+)"') {
    $version = $matches[1]
}

# --- Generate installer_data.h ---
Write-Step "Embedding files into installer_data.h..."

$header = New-Object System.Text.StringBuilder
[void]$header.AppendLine("#pragma once")
[void]$header.AppendLine("#include <stddef.h>")
[void]$header.AppendLine("")
[void]$header.AppendLine("#define FLUXA_INSTALLER_VERSION `"$version`"")
[void]$header.AppendLine("")

# Embed fluxa.exe
Write-Step "  Embedding fluxa.exe ($([System.IO.FileInfo]$fluxaExe | Select-Object -ExpandProperty Length) bytes)..."
$exeArray = ConvertTo-CArray $fluxaExe
$exeSize  = [System.IO.File]::ReadAllBytes($fluxaExe).Length
[void]$header.AppendLine("static const unsigned char fluxa_exe_data[] = {")
[void]$header.AppendLine($exeArray)
[void]$header.AppendLine("};")
[void]$header.AppendLine("static const int fluxa_exe_size = $exeSize;")
[void]$header.AppendLine("")

# Embed .flx files
$libCount = $libFiles.Count
[void]$header.AppendLine("static const int fluxa_lib_count = $libCount;")
[void]$header.AppendLine("")

$nameDecls  = New-Object System.Text.StringBuilder
$dataDecls  = New-Object System.Text.StringBuilder
$sizeDecls  = New-Object System.Text.StringBuilder
$nameArr    = New-Object System.Text.StringBuilder
$dataArr    = New-Object System.Text.StringBuilder
$sizeArr    = New-Object System.Text.StringBuilder

[void]$nameArr.AppendLine("static const char *fluxa_lib_names[] = {")
[void]$dataArr.AppendLine("static const unsigned char *fluxa_lib_data[] = {")
[void]$sizeArr.AppendLine("static const int fluxa_lib_sizes[] = {")

$idx = 0
foreach ($f in $libFiles) {
    Write-Step "  Embedding $($f.Name) ($($f.Length) bytes)..."
    $arr  = ConvertTo-CArray $f.FullName
    $size = $f.Length
    $var  = "lib_data_$idx"

    [void]$dataDecls.AppendLine("static const unsigned char ${var}[] = {")
    [void]$dataDecls.AppendLine($arr)
    [void]$dataDecls.AppendLine("};")
    [void]$dataDecls.AppendLine("")

    [void]$nameArr.AppendLine("    `"$($f.Name)`",")
    [void]$dataArr.AppendLine("    $var,")
    [void]$sizeArr.AppendLine("    $size,")

    $idx++
}

[void]$nameArr.AppendLine("};")
[void]$dataArr.AppendLine("};")
[void]$sizeArr.AppendLine("};")

[void]$header.Append($dataDecls.ToString())
[void]$header.AppendLine($nameArr.ToString())
[void]$header.AppendLine($dataArr.ToString())
[void]$header.AppendLine($sizeArr.ToString())

$headerPath = Join-Path $projectRoot "installer_data.h"
[System.IO.File]::WriteAllText($headerPath, $header.ToString(), [System.Text.Encoding]::UTF8)
Write-Ok "installer_data.h generated."

# --- Compile installer ---
Write-Step "Compiling fluxa_setup.exe..."

$installerSrc = Join-Path $projectRoot "installer.c"
$setupExe     = Join-Path $projectRoot "fluxa_setup.exe"

if (-not (Test-Path $installerSrc)) {
    Write-Fail "installer.c not found."
}

gcc $installerSrc -O2 -o $setupExe -lshlwapi -lshell32 -I $projectRoot 2>&1
if ($LASTEXITCODE -ne 0) { Write-Fail "Compilation failed." }

Write-Ok "fluxa_setup.exe created: $setupExe"

# --- Cleanup temp ---
Remove-Item -Force $headerPath -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "=====================================" -ForegroundColor Green
Write-Host "  Done! Share fluxa_setup.exe" -ForegroundColor Green
Write-Host "=====================================" -ForegroundColor Green
Write-Host ""
Write-Host "  File: $setupExe"
Write-Host ""
