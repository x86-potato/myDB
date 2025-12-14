CC = g++
CFLAGS = -g -std=c++17

SRC_DIR = src
OBJ_DEST_DIR = obj
BIN_DIR = bin

# recursively find all .cpp files
SRC := $(shell find $(SRC_DIR) -type f -name '*.cpp')

# turn src/.../file.cpp into obj/.../file.o
OBJ := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DEST_DIR)/%.o,$(SRC))

EXEC = $(BIN_DIR)/a.out

all: $(EXEC)

$(EXEC): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# compile rule, create subdirectories as needed
$(OBJ_DEST_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(OBJ_DEST_DIR) $(EXEC)

run: $(EXEC)
	./$(EXEC)

.PHONY: all clean run test
test: clean $(EXEC)
	./$(EXEC)
