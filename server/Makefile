CC = g++

# language standard (always on)
STD = -std=c++17


BASE_FLAGS = $(STD) -Wall -Wextra

RELEASE_FLAGS = -g -O0

DEBUG_FLAGS = -g -O1 \
	-fsanitize=address,undefined \
	-fno-omit-frame-pointer \
	-D_GLIBCXX_DEBUG

# select flags based on DEBUG
ifeq ($(DEBUG),1)
	CFLAGS = $(BASE_FLAGS) $(DEBUG_FLAGS)
else
	CFLAGS = $(BASE_FLAGS) $(RELEASE_FLAGS)
endif

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

test: clean $(EXEC)
	./$(EXEC)

.PHONY: all clean run test
