# B15Implementation Build Script
# PowerShell Build-Automatisierung für B15F-Projekt

param(
    [string]$Command = "build"
)

$ErrorActionPreference = "Stop"

$PROJECT_NAME = "b15comm"
$SRC_DIR = "src"
$INCLUDE_DIR = "include"
$BUILD_DIR = "build"

$SRC_FILES = @(
    "$SRC_DIR/checksum.cpp",
    "$SRC_DIR/stats.cpp",
    "$SRC_DIR/error_injector.cpp",
    "$SRC_DIR/b15board.cpp",
    "$SRC_DIR/main.cpp"
)

$COMPILER = "g++"
$CFLAGS = "-std=c++11 -Wall -I$INCLUDE_DIR"
$OUTPUT = "$BUILD_DIR/$PROJECT_NAME.exe"

function Show-Help {
    Write-Host ""
    Write-Host "B15Implementation Build Script" -ForegroundColor Cyan
    Write-Host "==============================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Verfuegbare Befehle:" -ForegroundColor Yellow
    Write-Host "  .\build.ps1 build            - Kompiliert das Projekt" -ForegroundColor Green
    Write-Host "  .\build.ps1 clean            - Loescht Build-Artefakte" -ForegroundColor Green
    Write-Host "  .\build.ps1 rebuild          - Clean + Build" -ForegroundColor Green
    Write-Host "  .\build.ps1 run-send-a       - Startet Board A im Sender-Modus" -ForegroundColor Green
    Write-Host "  .\build.ps1 run-send-b       - Startet Board B im Sender-Modus" -ForegroundColor Green
    Write-Host "  .\build.ps1 run-receive-a    - Startet Board A im Receiver-Modus" -ForegroundColor Green
    Write-Host "  .\build.ps1 run-receive-b    - Startet Board B im Receiver-Modus" -ForegroundColor Green
    Write-Host "  .\build.ps1 run-fullduplex-a - Startet Board A im Full-Duplex-Modus" -ForegroundColor Green
    Write-Host "  .\build.ps1 run-fullduplex-b - Startet Board B im Full-Duplex-Modus" -ForegroundColor Green
    Write-Host "  .\build.ps1 help             - Zeigt diese Hilfe" -ForegroundColor Green
    Write-Host ""
}

function Build-Project {
    Write-Host "Kompiliere Projekt..." -ForegroundColor Cyan
    
    if (-not (Test-Path $BUILD_DIR)) {
        New-Item -ItemType Directory -Path $BUILD_DIR | Out-Null
        Write-Host "Build-Verzeichnis erstellt: $BUILD_DIR" -ForegroundColor Yellow
    }
    
    $srcString = $SRC_FILES -join " "
    $compileCmd = "$COMPILER $CFLAGS $srcString -o $OUTPUT"
    
    Write-Host "Ausfuehre: $compileCmd" -ForegroundColor Gray
    
    try {
        Invoke-Expression $compileCmd
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Build erfolgreich!" -ForegroundColor Green
            Write-Host "Output: $OUTPUT" -ForegroundColor Green
        } else {
            Write-Host "Build fehlgeschlagen mit Exit-Code: $LASTEXITCODE" -ForegroundColor Red
            exit $LASTEXITCODE
        }
    } catch {
        Write-Host "Fehler beim Kompilieren: $_" -ForegroundColor Red
        exit 1
    }
}

function Clean-Project {
    Write-Host "Raeume Build-Verzeichnis auf..." -ForegroundColor Cyan
    
    if (Test-Path $BUILD_DIR) {
        Remove-Item -Recurse -Force $BUILD_DIR
        Write-Host "Build-Verzeichnis geloescht." -ForegroundColor Green
    } else {
        Write-Host "Nichts zu loeschen." -ForegroundColor Yellow
    }
}

function Run-Sender {
    param([string]$Board)
    if (-not (Test-Path $OUTPUT)) {
        Write-Host "Programm nicht gefunden. Fuehre Build aus..." -ForegroundColor Yellow
        Build-Project
    }
    
    Write-Host "Starte Board $Board im Sender-Modus..." -ForegroundColor Cyan
    & $OUTPUT $Board send
}

function Run-Receiver {
    param([string]$Board)
    if (-not (Test-Path $OUTPUT)) {
        Write-Host "Programm nicht gefunden. Fuehre Build aus..." -ForegroundColor Yellow
        Build-Project
    }
    
    Write-Host "Starte Board $Board im Receiver-Modus..." -ForegroundColor Cyan
    & $OUTPUT $Board receive
}

function Run-FullDuplex {
    param([string]$Board)
    if (-not (Test-Path $OUTPUT)) {
        Write-Host "Programm nicht gefunden. Fuehre Build aus..." -ForegroundColor Yellow
        Build-Project
    }
    
    Write-Host "Starte Board $Board im Full-Duplex-Modus..." -ForegroundColor Cyan
    & $OUTPUT $Board fullduplex
}

# Hauptlogik
switch ($Command.ToLower()) {
    "build" {
        Build-Project
    }
    "clean" {
        Clean-Project
    }
    "rebuild" {
        Clean-Project
        Build-Project
    }
    "run-send-a" {
        Run-Sender -Board "A"
    }
    "run-send-b" {
        Run-Sender -Board "B"
    }
    "run-receive-a" {
        Run-Receiver -Board "A"
    }
    "run-receive-b" {
        Run-Receiver -Board "B"
    }
    "run-fullduplex-a" {
        Run-FullDuplex -Board "A"
    }
    "run-fullduplex-b" {
        Run-FullDuplex -Board "B"
    }
    "help" {
        Show-Help
    }
    default {
        Write-Host "Unbekannter Befehl: $Command" -ForegroundColor Red
        Show-Help
        exit 1
    }
}
