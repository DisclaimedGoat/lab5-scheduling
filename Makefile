# Compiler and flags
CXX = g++

# Source files
SRCS = main.cpp
TARGET = scheduler

$(TARGET): $(SRCS)
	$(CXX) -o $(TARGET) $(SRCS)

# Default target
all: $(TARGET)

# Run the program with default input and output files
run: $(TARGET)
	./$(TARGET) input.txt output.txt

# Run the program with bad input and output files. Supposed to be messy deadlines
bad: $(TARGET)
	./$(TARGET) input-bad.txt output-bad.txt

# Clean up build files
clean:
	rm -f $(TARGET) *.o