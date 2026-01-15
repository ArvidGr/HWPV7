# Build script for B15F Simulator
param(
    [Parameter(Position=0)]
    [string]$Command = "build"
)

$CXX = "g++"
$CXXFLAGS = "-std=c++11 -pthread -Wall"
$TARGET = "simulator.exe"
$SOURCES = @("main.cpp", "checksum.cpp", "stats.cpp", "error_injector.cpp", "patch_cable.cpp", "b15simulator.cpp")

function Build {
    Write-Host "Building $TARGET..." -ForegroundColor Green
    $sourceFiles = $SOURCES -join " "
    $cmd = "$CXX $CXXFLAGS $sourceFiles -o $TARGET"
    Write-Host $cmd -ForegroundColor Yellow
    Invoke-Expression $cmd
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Build successful!" -ForegroundColor Green
    } else {
        Write-Host "Build failed!" -ForegroundColor Red
        exit 1
    }
}

function Clean {
    Write-Host "Cleaning build artifacts..." -ForegroundColor Green
    Remove-Item *.o -ErrorAction SilentlyContinue
    Remove-Item $TARGET -ErrorAction SilentlyContinue
    Remove-Item patchcable.bin -ErrorAction SilentlyContinue
    Write-Host "Cleaned!" -ForegroundColor Green
}

function Rebuild {
    Clean
    Build
}

function Run-Sender {
    if (!(Test-Path $TARGET)) {
        Write-Host "Executable not found. Building first..." -ForegroundColor Yellow
        Build
    }
    Write-Host "Starting Board A in sender mode..." -ForegroundColor Cyan
    & ".\$TARGET" A send
}

function Run-Receiver {
    if (!(Test-Path $TARGET)) {
        Write-Host "Executable not found. Building first..." -ForegroundColor Yellow
        Build
    }
    Write-Host "Starting Board B in receiver mode with 20% error rate..." -ForegroundColor Cyan
    & ".\$TARGET" B receive 20
}

function Run-Fullduplex-A {
    if (!(Test-Path $TARGET)) {
        Write-Host "Executable not found. Building first..." -ForegroundColor Yellow
        Build
    }
    Write-Host "Starting Board A in FULL-DUPLEX mode..." -ForegroundColor Magenta
    & ".\$TARGET" A fullduplex
}

function Run-Fullduplex-B {
    if (!(Test-Path $TARGET)) {
        Write-Host "Executable not found. Building first..." -ForegroundColor Yellow
        Build
    }
    Write-Host "Starting Board B in FULL-DUPLEX mode with 20% error rate..." -ForegroundColor Magenta
    & ".\$TARGET" B fullduplex 20
}

function Show-Help {
    Write-Host @"
B15F Simulator Build Script

Usage: .\build.ps1 [command]

Commands:
    build               Build the project (default)
    clean               Remove build artifacts
    rebuild             Clean and rebuild
    run-sender          Run Board A in sender mode (half-duplex)
    run-receiver        Run Board B in receiver mode (half-duplex, 20% error)
    run-fullduplex-a    Run Board A in full-duplex mode
    run-fullduplex-b    Run Board B in full-duplex mode (20% error)
    help                Show this help message

Examples:
    .\build.ps1
    .\build.ps1 build
    .\build.ps1 clean
    .\build.ps1 rebuild
    
    # Half-Duplex (in two terminals):
    .\build.ps1 run-sender
    .\build.ps1 run-receiver
    
    # Full-Duplex (in two terminals):
    .\build.ps1 run-fullduplex-a
    .\build.ps1 run-fullduplex-b
"@ -ForegroundColor Cyan
}

# Execute command
switch ($Command.ToLower()) {
    "build" { Build }
    "clean" { Clean }
    "rebuild" { Rebuild }
    "run-sender" { Run-Sender }
    "run-receiver" { Run-Receiver }
    "run-fullduplex-a" { Run-Fullduplex-A }
    "run-fullduplex-b" { Run-Fullduplex-B }
    "help" { Show-Help }
    default {
        Write-Host "Unknown command: $Command" -ForegroundColor Red
        Show-Help
        exit 1
    }
}
