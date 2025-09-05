# Compiler and flags
CC = g++
CFLAGS = -Wall -Wextra -std=c++17

# Source directory
SRC_DIR = src
OBJ_DEST_DIR = obj

# Project files
SRC = $(wildcard $(SRC_DIR)/*.cpp)
OBJ = $(SRC:.cpp=.o)
EXEC = a.out

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)

run: $(EXEC)
	./$(EXEC)

.PHONY: all clean run