# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -I./headers `pkg-config --cflags openssl`
LDFLAGS = -lz `pkg-config --libs openssl`

# Directories and source files
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = .

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Executable target
TARGET = $(BIN_DIR)/mygit

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up compiled files
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Run the program (example usage)
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
