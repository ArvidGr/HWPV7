# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall
TARGET = simulator.exe

# Source files
SOURCES = main.cpp checksum.cpp stats.cpp error_injector.cpp patch_cable.cpp b15simulator.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Header files
HEADERS = protocol.h checksum.h stats.h error_injector.h patch_cable.h b15simulator.h

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Compile source files to object files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	del /Q *.o $(TARGET) patchcable.bin 2>nul || echo Cleaned

# Clean and rebuild
rebuild: clean all

# Run sender mode
run-sender:
	$(TARGET) A send

# Run receiver mode with 20% error rate
run-receiver:
	$(TARGET) B receive 20

.PHONY: all clean rebuild run-sender run-receiver
