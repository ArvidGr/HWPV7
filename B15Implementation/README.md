# B15Implementation

Modulares B15F Full-Duplex Kommunikationssystem mit 2-Bit Parallel-Protokoll, CRC8-Checksum und ARQ.

## Projektstruktur

```
B15Implementation/
├── include/
│   ├── protocol.h          # Protokoll-Konstanten (ACK, NACK, EOT, Bit-Masken)
│   ├── checksum.h          # CRC8 Checksum-Berechnung
│   ├── stats.h             # Statistik-Tracking
│   ├── error_injector.h    # Fehler-Injektion (nur für Simulation)
│   └── b15board.h          # B15Board Hauptklasse
├── src/
│   ├── checksum.cpp        # CRC8 Implementation
│   ├── stats.cpp           # Statistik-Funktionen
│   ├── error_injector.cpp  # Fehler-Injektion Implementation
│   ├── b15board.cpp        # B15Board Implementation
│   └── main.cpp            # Hauptprogramm
├── build/                  # Build-Ausgabeverzeichnis (wird automatisch erstellt)
├── build.ps1               # PowerShell Build-Script
└── README.md               # Diese Datei
```

## Hardware-Anforderungen

- **B15F Board** mit AVR-Mikrocontroller
- **Crossover-Kabel** zwischen zwei Boards:
  - Bit 0 (Board A) ↔ Bit 7 (Board B)
  - Bit 1 (Board A) ↔ Bit 6 (Board B)
  - Bit 2 (Board A) ↔ Bit 5 (Board B)
  - Bit 3 (Board A) ↔ Bit 4 (Board B)
- **Pin-Konfiguration**: DDRA = 0x0F
  - Bits 0-3: Output (Senden)
  - Bits 4-7: Input (Empfangen)

## Protokoll-Details

### 2-Bit Parallel-Protokoll

- **DATA0** (Bit 0): Datenbit 0
- **DATA1** (Bit 1): Datenbit 1
- **CLOCK** (Bit 2): Taktsignal für Synchronisation
- **ACK** (Bit 3): Acknowledgment-Signal

### Fehlerbehandlung

- **CRC8-Checksum** mit Polynom 0x07
- **ARQ** (Automatic Repeat Request) mit bis zu 5 Wiederholungen
- **ACK/NACK/EOT** Kontrollbytes für Zustandsverwaltung

### Betriebsmodi

1. **Half-Duplex Sender**: Sendet Nachrichten zeilenweise
2. **Half-Duplex Receiver**: Empfängt Nachrichten bis EOT
3. **Full-Duplex**: Simultanes Senden und Empfangen mit Threads

## Kompilieren

### Voraussetzungen

- `g++` mit C++11-Support
- `b15f` Library installiert

### Build-Befehle

```powershell
# Projekt kompilieren
.\build.ps1 build

# Build-Verzeichnis aufräumen
.\build.ps1 clean

# Clean + Build
.\build.ps1 rebuild

# Hilfe anzeigen
.\build.ps1 help
```

## Verwendung

### Half-Duplex Kommunikation

**Board 1 (Sender):**

```powershell
.\build.ps1 run-send
```

oder

```powershell
.\build\b15comm.exe send
```

**Board 2 (Receiver):**

```powershell
.\build.ps1 run-receive
```

oder

```powershell
.\build\b15comm.exe receive
```

### Full-Duplex Kommunikation

**Beide Boards:**

```powershell
.\build.ps1 run-fullduplex
```

oder

```powershell
.\build\b15comm.exe fullduplex
```

### Verbose-Modus

Für detaillierte Debug-Ausgaben:

```powershell
.\build\b15comm.exe send 1
.\build\b15comm.exe receive 1
.\build\b15comm.exe fullduplex 1
```

## Datei-Ausgabe

Im Full-Duplex-Modus werden empfangene Nachrichten automatisch in Dateien geschrieben:

- `received_B15.txt` - Empfangene Nachrichten

## Technische Details

### Thread-Sicherheit

- **Mutex-Locks** schützen alle B15F-Registerzugriffe (PORTA/PINA)
- **Atomic Variables** für Statistik-Zähler
- Separate TX/RX-Threads im Full-Duplex-Modus

### Bit-Reversal

Die `read_input()` Funktion führt Bit-Reversal durch, um die physikalische Kabelkreuzung zu kompensieren:

```
PINA[7] -> Bit[0]  (DATA0)
PINA[6] -> Bit[1]  (DATA1)
PINA[5] -> Bit[2]  (CLOCK)
PINA[4] -> Bit[3]  (ACK)
```

### Timing

- **Clock-Synchronisation**: Polling mit 100µs Delays
- **Timeout**: 5000 Iterationen × 100µs = 500ms pro 2-Bit-Übertragung
- **Initialisierung**: 200ms + 100ms Delays für Stabilität

## Statistiken

Das Programm zeigt nach Beendigung folgende Statistiken:

- Bytes gesendet
- Bytes empfangen
- Anzahl Wiederholungen (Retransmissions)
- Checksum-Fehler
- Fehlerrate in Prozent

## Autor

Erstellt für HWP Semester 5 - B15F Kommunikationsprojekt

## Lizenz

Akademisches Projekt - Hochschule für Wirtschaft und Praxis
