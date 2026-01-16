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
    Write-Host "  .\build.ps1 build          - Kompiliert das Projekt" -ForegroundColor Green
    Write-Host "  .\build.ps1 clean          - Loescht Build-Artefakte" -ForegroundColor Green
    Write-Host "  .\build.ps1 rebuild        - Clean + Build" -ForegroundColor Green
    Write-Host "  .\build.ps1 run-send       - Startet im Sender-Modus" -ForegroundColor Green
    Write-Host "  .\build.ps1 run-receive    - Startet im Receiver-Modus" -ForegroundColor Green
    Write-Host "  .\build.ps1 run-fullduplex - Startet im Full-Duplex-Modus" -ForegroundColor Green
    Write-Host "  .\build.ps1 help           - Zeigt diese Hilfe" -ForegroundColor Green
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
    if (-not (Test-Path $OUTPUT)) {
        Write-Host "Programm nicht gefunden. Fuehre Build aus..." -ForegroundColor Yellow
        Build-Project
    }
    
    Write-Host "Starte Sender-Modus..." -ForegroundColor Cyan
    & $OUTPUT send
}

function Run-Receiver {
    if (-not (Test-Path $OUTPUT)) {
        Write-Host "Programm nicht gefunden. Fuehre Build aus..." -ForegroundColor Yellow
        Build-Project
    }
    
    Write-Host "Starte Receiver-Modus..." -ForegroundColor Cyan
    & $OUTPUT receive
}

function Run-FullDuplex {
    if (-not (Test-Path $OUTPUT)) {
        Write-Host "Programm nicht gefunden. Fuehre Build aus..." -ForegroundColor Yellow
        Build-Project
    }
    
    Write-Host "Starte Full-Duplex-Modus..." -ForegroundColor Cyan
    & $OUTPUT fullduplex
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
    "run-send" {
        Run-Sender
    }
    "run-receive" {
        Run-Receiver
    }
    "run-fullduplex" {
        Run-FullDuplex
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
