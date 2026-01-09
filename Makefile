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
TEST_DIR = tests

# Target
TARGET = pinspect

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Test files
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_BINS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(BUILD_DIR)/%)

# Library objects (everything except main.o)
LIB_OBJS = $(filter-out $(BUILD_DIR)/main.o, $(OBJS))

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
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_BINS)

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

# Build test binaries
$(BUILD_DIR)/test_%: $(TEST_DIR)/test_%.c $(LIB_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $< $(LIB_OBJS) $(LDFLAGS)

# Build all tests
tests: $(TEST_BINS)

# Run tests
test: tests
	@echo "Running tests..."
	@for test in $(TEST_BINS); do \
		echo ""; \
		$$test || exit 1; \
	done
	@echo ""
	@echo "All tests passed!"

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

.PHONY: all clean install uninstall debug release test tests valgrind format-check format
