<#
.SYNOPSIS
    Installs the Sentry Native SDK for Windows
.DESCRIPTION
    This script downloads and installs the Sentry Native SDK for Windows,
    configuring it for use with Mudlet's crash reporting system.
.NOTES
    Copyright (C) 2024 by ARYPROGRAMMER
    Copyright (C) 2024 by Stephen Lyons - slysven@virginmedia.com
#>

$ErrorActionPreference = "Stop"

# Configuration
$SENTRY_VERSION = "0.6.7"
$SENTRY_DIR = "C:\sentry-sdk"
$TEMP_DIR = "C:\temp\sentry-build"
$CMAKE_DIR = "C:\Program Files\CMake\bin"
$NINJA_VERSION = "1.11.1"

# Ensure admin privileges
if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error "This script requires administrative privileges. Please run as Administrator."
    exit 1
}

# Create directories
New-Item -ItemType Directory -Force -Path $SENTRY_DIR | Out-Null
New-Item -ItemType Directory -Force -Path $TEMP_DIR | Out-Null

# Download and install build tools if needed
if (-NOT (Test-Path "$CMAKE_DIR\cmake.exe")) {
    Write-Host "Installing CMake..."
    $cmake_installer = "$TEMP_DIR\cmake-installer.msi"
    Invoke-WebRequest -Uri "https://github.com/Kitware/CMake/releases/download/v3.25.1/cmake-3.25.1-windows-x86_64.msi" -OutFile $cmake_installer
    Start-Process -FilePath "msiexec.exe" -ArgumentList "/i", $cmake_installer, "/quiet" -Wait
}

# Download and extract Ninja
if (-NOT (Test-Path "$TEMP_DIR\ninja.exe")) {
    Write-Host "Downloading Ninja..."
    $ninja_zip = "$TEMP_DIR\ninja.zip"
    Invoke-WebRequest -Uri "https://github.com/ninja-build/ninja/releases/download/v$NINJA_VERSION/ninja-win.zip" -OutFile $ninja_zip
    Expand-Archive -Path $ninja_zip -DestinationPath $TEMP_DIR -Force
}

# Download Sentry SDK
Write-Host "Downloading Sentry Native SDK..."
$sentry_zip = "$TEMP_DIR\sentry-native.zip"
Invoke-WebRequest -Uri "https://github.com/getsentry/sentry-native/releases/download/$SENTRY_VERSION/sentry-native-windows-x64.zip" -OutFile $sentry_zip

# Extract SDK
Write-Host "Extracting Sentry Native SDK..."
Expand-Archive -Path $sentry_zip -DestinationPath "$TEMP_DIR\sentry-src" -Force

# Build Sentry
Write-Host "Building Sentry..."
Push-Location "$TEMP_DIR\sentry-src"

# Configure with CMake
& "$CMAKE_DIR\cmake.exe" `
    -G "Ninja" `
    -DCMAKE_BUILD_TYPE=Release `
    -DSENTRY_BUILD_SHARED_LIBS=OFF `
    -DSENTRY_BUILD_TESTS=OFF `
    -DSENTRY_BUILD_EXAMPLES=OFF `
    -B build

# Build
& "$CMAKE_DIR\cmake.exe" --build build --config Release

# Install
Copy-Item "build\lib\*" -Destination "$SENTRY_DIR\lib" -Force
Copy-Item "build\include\*" -Destination "$SENTRY_DIR\include" -Force
Copy-Item "build\bin\*" -Destination "$SENTRY_DIR\bin" -Force

Pop-Location

# Set environment variables
[Environment]::SetEnvironmentVariable("SENTRY_ROOT", $SENTRY_DIR, "Machine")
[Environment]::SetEnvironmentVariable("PATH", "$SENTRY_DIR\bin;$env:PATH", "Machine")

# Cleanup
Remove-Item -Recurse -Force $TEMP_DIR

Write-Host "Sentry Native SDK installation complete"
Write-Host "SDK Location: $SENTRY_DIR"
Write-Host "Version: $SENTRY_VERSION"