# Compiler and base flags
CC = clang
BASE_CFLAGS= -std=c99 -Wall -Wno-typedef-redefinition -Iinclude `pkg-config --cflags sdl3` \
			 -DVK_USE_PLATFORM_MACOS_MVK
BASE_LDFLAGS= `pkg-config --libs sdl3`
# BASE_CFLAGS= -std=c99 -Wall -Wno-typedef-redefinition -Iinclude `pkg-config --cflags vulkan sdl3`
# BASE_LDFLAGS= `pkg-config --libs vulkan sdl3`

CFLAGS= $(BASE_CFLAGS) -g -DDEBUG
LDFLAGS= $(BASE_LDFLAGS)

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Target executable
TARGET = $(BIN_DIR)/vulkan

# Find all source files recursively in the src directory
SRCS = $(shell find $(SRC_DIR) -name '*.c')

# Generate corresponding object files in the obj directory
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Default target (debug)
all: $(TARGET)

# Release build (explicit target)
release: CFLAGS= $(BASE_CFLAGS) -O3 -DNDEBUG
release: LDFLAGS= $(BASE_LDFLAGS)
release: clean $(TARGET)

# Link the object files to create the executable
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ -o $@

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)  # Create the necessary subdirectories in obj/
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all release clean
