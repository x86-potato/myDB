CC = g++

STD = -std=c++17
WARN = -Wall -Wextra

# Always on
BASE = $(STD) $(WARN)

# ---- Build modes ----

# Default DEV build (no optimization, debug symbols)
DEV_FLAGS = -g -O0

# Debug with sanitizers (heavy, slow)
DBG_FLAGS = -g -O1 \
	-fsanitize=address,undefined \
	-fno-omit-frame-pointer \
	-D_GLIBCXX_DEBUG

# Production / benchmark build
PROD_FLAGS = -O3 -DNDEBUG

# ---- Mode selection ----
# Priority: DEBUG > PROD > DEV

ifeq ($(DEBUG),1)
	CFLAGS = $(BASE) $(DBG_FLAGS)
else ifeq ($(PROD),1)
	CFLAGS = $(BASE) $(PROD_FLAGS)
else
	CFLAGS = $(BASE) $(DEV_FLAGS)
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
