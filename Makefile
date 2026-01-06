# pinspect - Linux Process Inspector
# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11 -g
CFLAGS += -fsanitize=address,undefined
LDFLAGS = -fsanitize=address,undefined

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

# Target
TARGET = pinspect

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Default target
all: $(BUILD_DIR) $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c -o $@ $<

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Install to /usr/local/bin (requires sudo)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Run with sanitizers (default build already includes them)
debug: all

# Release build (no sanitizers, optimized)
release: CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11 -O2
release: LDFLAGS =
release: clean all

# Run tests
test: all
	@echo "Running tests..."
	@# TODO: Add test runner

# Check for memory leaks with valgrind
valgrind: release
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) $$$$

# Format check (requires clang-format)
format-check:
	@find $(SRC_DIR) $(INC_DIR) -name '*.c' -o -name '*.h' | \
		xargs clang-format --dry-run --Werror

# Format code
format:
	@find $(SRC_DIR) $(INC_DIR) -name '*.c' -o -name '*.h' | \
		xargs clang-format -i

.PHONY: all clean install uninstall debug release test valgrind format-check format
