# B15Implementation Makefile
# Linux Build-Automatisierung für B15F-Projekt

# Compiler und Flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Iinclude
LDFLAGS = 

# Verzeichnisse
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# Quelldateien
SOURCES = $(SRC_DIR)/checksum.cpp \
          $(SRC_DIR)/stats.cpp \
          $(SRC_DIR)/error_injector.cpp \
          $(SRC_DIR)/b15board.cpp \
          $(SRC_DIR)/main.cpp

# Objektdateien
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Ausgabedatei
TARGET = $(BUILD_DIR)/b15comm

# Farben für Terminal-Ausgaben
GREEN = \033[0;32m
YELLOW = \033[0;33m
CYAN = \033[0;36m
NC = \033[0m # No Color

# Standard-Target
.PHONY: all
all: $(TARGET)

# Erstelle Build-Verzeichnis
$(BUILD_DIR):
	@echo "$(YELLOW)Erstelle Build-Verzeichnis...$(NC)"
	@mkdir -p $(BUILD_DIR)

# Kompiliere Objektdateien
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "$(CYAN)Kompiliere $<...$(NC)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Linke Executable
$(TARGET): $(OBJECTS)
	@echo "$(CYAN)Linke $(TARGET)...$(NC)"
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "$(GREEN)Build erfolgreich!$(NC)"
	@echo "$(GREEN)Output: $(TARGET)$(NC)"

# Clean
.PHONY: clean
clean:
	@echo "$(CYAN)Räume Build-Verzeichnis auf...$(NC)"
	@rm -rf $(BUILD_DIR)
	@echo "$(GREEN)Build-Verzeichnis gelöscht.$(NC)"

# Rebuild
.PHONY: rebuild
rebuild: clean all

# Help
.PHONY: help
help:
	@echo ""
	@echo "$(CYAN)B15Implementation Makefile$(NC)"
	@echo "$(CYAN)===========================$(NC)"
	@echo ""
	@echo "$(YELLOW)Verfügbare Targets:$(NC)"
	@echo "  $(GREEN)make$(NC) oder $(GREEN)make all$(NC)           - Kompiliert das Projekt"
	@echo "  $(GREEN)make clean$(NC)                   - Löscht Build-Artefakte"
	@echo "  $(GREEN)make rebuild$(NC)                 - Clean + Build"
	@echo "  $(GREEN)make run-send-a$(NC)              - Startet Board A im Sender-Modus"
	@echo "  $(GREEN)make run-send-b$(NC)              - Startet Board B im Sender-Modus"
	@echo "  $(GREEN)make run-receive-a$(NC)           - Startet Board A im Receiver-Modus"
	@echo "  $(GREEN)make run-receive-b$(NC)           - Startet Board B im Receiver-Modus"
	@echo "  $(GREEN)make run-fullduplex-a$(NC)        - Startet Board A im Full-Duplex-Modus"
	@echo "  $(GREEN)make run-fullduplex-b$(NC)        - Startet Board B im Full-Duplex-Modus"
	@echo "  $(GREEN)make help$(NC)                    - Zeigt diese Hilfe"
	@echo ""
	@echo "$(YELLOW)Direkter Aufruf:$(NC)"
	@echo "  $(GREEN)$(TARGET) A fullduplex$(NC)      - Board A Full-Duplex"
	@echo "  $(GREEN)$(TARGET) B fullduplex$(NC)      - Board B Full-Duplex"
	@echo "  $(GREEN)$(TARGET) A send$(NC)            - Board A Sender"
	@echo "  $(GREEN)$(TARGET) B receive$(NC)         - Board B Receiver"
	@echo ""

# Run-Targets (Board A)
.PHONY: run-send-a
run-send-a: $(TARGET)
	@echo "$(CYAN)Starte Board A im Sender-Modus...$(NC)"
	@$(TARGET) A send

.PHONY: run-receive-a
run-receive-a: $(TARGET)
	@echo "$(CYAN)Starte Board A im Receiver-Modus...$(NC)"
	@$(TARGET) A receive

.PHONY: run-fullduplex-a
run-fullduplex-a: $(TARGET)
	@echo "$(CYAN)Starte Board A im Full-Duplex-Modus...$(NC)"
	@$(TARGET) A fullduplex

# Run-Targets (Board B)
.PHONY: run-send-b
run-send-b: $(TARGET)
	@echo "$(CYAN)Starte Board B im Sender-Modus...$(NC)"
	@$(TARGET) B send

.PHONY: run-receive-b
run-receive-b: $(TARGET)
	@echo "$(CYAN)Starte Board B im Receiver-Modus...$(NC)"
	@$(TARGET) B receive

.PHONY: run-fullduplex-b
run-fullduplex-b: $(TARGET)
	@echo "$(CYAN)Starte Board B im Full-Duplex-Modus...$(NC)"
	@$(TARGET) B fullduplex
