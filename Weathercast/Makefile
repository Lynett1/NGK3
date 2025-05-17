# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -I/path/to/vcpkg/installed/x64-linux/include

# Libraries and includes
LIBS = -lrestinio -ljson_dto -lpthread
INCLUDES = 

# Source files and target
SRC = Routing.cpp
TARGET = Routing

# Build target
all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LIBS)

# Clean build files
clean:
	rm -f $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)