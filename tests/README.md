# pinspect Test Suite

Unit tests for the Linux Process Inspector tool.

## Running Tests

### Quick Run
```bash
make test
```

### Using the Test Runner Script
```bash
./tests/run_tests.sh
```

## Test Coverage

### test_util.c
Tests for utility functions in `src/util.c`:

- **parse_pid()** - 7 tests
  - Valid PID parsing
  - Current PID parsing
  - Invalid input (non-numeric, negative, zero)
  - NULL and empty string handling

- **build_proc_path()** - 3 tests
  - Path construction with file
  - Path construction without file
  - Small buffer handling

- **build_task_path()** - 4 tests
  - Path construction with file (comm)
  - Path construction with status file
  - Small buffer handling
  - TID equal to PID case

- **pid_exists()** - 3 tests
  - PID 1 (init/systemd)
  - Current process
  - Non-existent PID

- **State Conversions** - 8 tests
  - state_to_string() for all states
  - char_to_state() for valid and invalid chars

**Total: 25 tests**

### test_proc_status.c
Tests for /proc/<PID>/status parsing in `src/proc_status.c`:

- **read_proc_status()** - 9 tests
  - Current process reading
  - Name, state, UIDs, GIDs, thread count validation
  - PID 1 reading
  - Non-existent PID error handling
  - errno verification

**Total: 9 tests**

### test_proc_task.c
Tests for thread enumeration in `src/proc_task.c`:

- **enumerate_threads()** - 7 tests
  - Current process enumeration
  - Main thread TID equals PID
  - Thread has valid name
  - Thread has valid state
  - PID 1 (with permission handling)
  - Non-existent PID error handling
  - Error state cleanup (count set to 0)

- **thread_info_free()** - 1 test
  - NULL pointer safety

**Total: 8 tests**

### test_proc_fd.c
Tests for file descriptor enumeration in `src/proc_fd.c`:

- **enumerate_fds()** - 6 tests
  - Current process enumeration
  - Standard FDs present (stdin/stdout/stderr)
  - FD entries have valid targets
  - PID 1 (with permission handling)
  - Non-existent PID error handling
  - Error state cleanup (count set to 0)

- **fd_entries_free()** - 1 test
  - NULL pointer safety

- **parse_socket_inode()** - 5 tests
  - Valid socket format parsing
  - Large inode numbers
  - Regular file rejection
  - Pipe format rejection
  - NULL input handling

**Total: 12 tests**

### test_net.c
Tests for network connection parsing in `src/net.c`:

- **tcp_state_to_string()** - 3 tests
  - ESTABLISHED state
  - LISTEN state
  - Unknown/invalid state handling

- **format_ip_port()** - 3 tests
  - Localhost IP formatting (127.0.0.1:8080)
  - Any address formatting (0.0.0.0:22)
  - NULL buffer safety

- **find_process_sockets()** - 2 tests
  - Current process socket enumeration
  - Non-existent PID error handling

- **socket_list_free()** - 1 test
  - NULL pointer safety

**Total: 9 tests**

## Test Output

Tests use color-coded output:
- 🟢 **[PASS]** - Test passed
- 🔴 **[FAIL]** - Test failed (with details)

Example output:
```
=== Running Utility Function Tests ===

Testing parse_pid with valid PID... [PASS]
Testing parse_pid with NULL... [PASS]
...

=== Test Results ===
Passed: 63
Failed: 0
Total:  63
```

## Adding New Tests

1. Create a new test file in `tests/` directory:
   ```bash
   tests/test_<module>.c
   ```

2. Include the module's header and test helpers:
   ```c
   #include "../include/<module>.h"
   #include <stdio.h>
   #include <assert.h>
   ```

3. Use the test macros:
   ```c
   TEST("description of test");
   ASSERT_EQ(actual, expected);
   ASSERT_TRUE(condition);
   ASSERT_FALSE(condition);
   ASSERT_STR_EQ(actual, expected);
   ```

4. Run `make test` - the Makefile automatically discovers new tests.

## Memory Safety

All tests run with Address Sanitizer and Undefined Behavior Sanitizer enabled:
```bash
CFLAGS += -fsanitize=address,undefined
```

This catches:
- Memory leaks
- Buffer overflows
- Use-after-free
- Undefined behavior

## Notes

- Tests use real `/proc` filesystem, so some may require appropriate permissions
- Error messages from functions (like "Expected a PID argument") are expected during negative tests
- All tests should pass on a standard Linux system
