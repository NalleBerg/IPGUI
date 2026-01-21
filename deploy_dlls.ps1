# Deploy required DLLs to the executable directory
# This script copies necessary MinGW and Qt DLLs to run IPGui.exe

$ErrorActionPreference = "Stop"

# Paths
$mingwBin = "C:\mingw64\bin"
$qtBin = "C:\Qt\6.9.1\mingw_64\bin"
$qtPlugins = "C:\Qt\6.9.1\mingw_64\plugins"
$qtTranslations = "C:\Qt\6.9.1\mingw_64\translations"

# Determine target directory (use build directory by default)
$targetDir = ".\build"
if (-not (Test-Path $targetDir)) {
    $targetDir = ".\IPGui"
}

Write-Host "Deploying DLLs to: $targetDir" -ForegroundColor Cyan

# MinGW Runtime DLLs
$mingwDlls = @(
    "libgcc_s_seh-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll"
)

Write-Host "`nCopying MinGW runtime DLLs..." -ForegroundColor Yellow
foreach ($dll in $mingwDlls) {
    $source = Join-Path $mingwBin $dll
    if (Test-Path $source) {
        Copy-Item $source $targetDir -Force
        Write-Host "  ✓ Copied $dll" -ForegroundColor Green
    } else {
        Write-Host "  ✗ Warning: $dll not found at $source" -ForegroundColor Red
    }
}

# Qt Runtime DLLs (basic ones - windeployqt will handle the rest)
Write-Host "`nRunning Qt deployment tool..." -ForegroundColor Yellow
$windeployqt = Join-Path $qtBin "windeployqt6.exe"
$exePath = Join-Path $targetDir "IPGui.exe"

if (Test-Path $windeployqt) {
    if (Test-Path $exePath) {
        & $windeployqt $exePath --verbose 1
        Write-Host "  ✓ Qt deployment completed" -ForegroundColor Green
    } else {
        Write-Host "  ✗ IPGui.exe not found at $exePath" -ForegroundColor Red
        Write-Host "    Please build the project first!" -ForegroundColor Yellow
    }
} else {
    Write-Host "  ✗ windeployqt not found. Copying Qt DLLs manually..." -ForegroundColor Red
    
    # Manual Qt DLL copying as fallback
    $qtDlls = @(
        "Qt6Core.dll",
        "Qt6Gui.dll",
        "Qt6Widgets.dll",
        "Qt6Network.dll",
        "Qt6Concurrent.dll"
    )
    
    foreach ($dll in $qtDlls) {
        $source = Join-Path $qtBin $dll
        if (Test-Path $source) {
            Copy-Item $source $targetDir -Force
            Write-Host "  ✓ Copied $dll" -ForegroundColor Green
        }
    }
}

Write-Host "`n✓ Deployment complete!" -ForegroundColor Green
Write-Host "You can now run IPGui.exe from $targetDir" -ForegroundColor Cyan
