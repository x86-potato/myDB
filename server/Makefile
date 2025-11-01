# Compiler and flags
CC = g++
CFLAGS = -Wall -Wextra -std=c++17

# Directories
SRC_DIR = src
OBJ_DEST_DIR = obj
BIN_DIR = bin

# Project files
SRC = $(wildcard $(SRC_DIR)/*.cpp)
OBJ = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DEST_DIR)/%.o,$(SRC))
EXEC = $(BIN_DIR)/a.out

all: $(EXEC)

# Link
$(EXEC): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source into obj dir
$(OBJ_DEST_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DEST_DIR)
	$(CC) $(CFLAGS) -c $< -o $@



clean:
	rm -f $(OBJ) $(EXEC)

run: $(EXEC)
	./$(EXEC)

.PHONY: test
test: clean $(EXEC)
	./$(EXEC)


.PHONY: all clean run
