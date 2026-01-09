/*
 * test_util.c - Unit tests for utility functions
 *
 * Tests parse_pid(), build_proc_path(), state conversions
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "../include/util.h"
#include "../include/pinspect.h"

#define TEST_PASS "\033[32m[PASS]\033[0m"
#define TEST_FAIL "\033[31m[FAIL]\033[0m"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    do { \
        printf("Testing %s... ", name); \
        fflush(stdout); \
    } while(0)

#define ASSERT_EQ(actual, expected) \
    do { \
        if ((actual) == (expected)) { \
            printf("%s\n", TEST_PASS); \
            tests_passed++; \
        } else { \
            printf("%s (expected %d, got %d)\n", TEST_FAIL, (int)(expected), (int)(actual)); \
            tests_failed++; \
        } \
    } while(0)

#define ASSERT_STR_EQ(actual, expected) \
    do { \
        if (strcmp((actual), (expected)) == 0) { \
            printf("%s\n", TEST_PASS); \
            tests_passed++; \
        } else { \
            printf("%s (expected \"%s\", got \"%s\")\n", TEST_FAIL, expected, actual); \
            tests_failed++; \
        } \
    } while(0)

#define ASSERT_TRUE(condition) \
    do { \
        if (condition) { \
            printf("%s\n", TEST_PASS); \
            tests_passed++; \
        } else { \
            printf("%s (condition was false)\n", TEST_FAIL); \
            tests_failed++; \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (!(condition)) { \
            printf("%s\n", TEST_PASS); \
            tests_passed++; \
        } else { \
            printf("%s (condition was true)\n", TEST_FAIL); \
            tests_failed++; \
        } \
    } while(0)

/* Test parse_pid() */
void test_parse_pid_valid(void)
{
    TEST("parse_pid with valid PID");
    ASSERT_EQ(parse_pid("1234"), 1234);
}

void test_parse_pid_current_shell(void)
{
    TEST("parse_pid with current PID");
    pid_t current = getpid();
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", current);
    ASSERT_EQ(parse_pid(buf), current);
}

void test_parse_pid_invalid(void)
{
    TEST("parse_pid with non-numeric input");
    ASSERT_EQ(parse_pid("abc"), -1);
}

void test_parse_pid_negative(void)
{
    TEST("parse_pid with negative number");
    ASSERT_EQ(parse_pid("-123"), -1);
}

void test_parse_pid_zero(void)
{
    TEST("parse_pid with zero");
    ASSERT_EQ(parse_pid("0"), -1);
}

void test_parse_pid_null(void)
{
    TEST("parse_pid with NULL");
    ASSERT_EQ(parse_pid(NULL), -1);
}

void test_parse_pid_empty(void)
{
    TEST("parse_pid with empty string");
    ASSERT_EQ(parse_pid(""), -1);
}

/* Test build_proc_path() */
void test_build_proc_path_with_file(void)
{
    TEST("build_proc_path with file");
    char buf[256];
    int ret = build_proc_path(1234, "status", buf, sizeof(buf));
    ASSERT_TRUE(ret == 0 && strcmp(buf, "/proc/1234/status") == 0);
}

void test_build_proc_path_without_file(void)
{
    TEST("build_proc_path without file");
    char buf[256];
    int ret = build_proc_path(1234, NULL, buf, sizeof(buf));
    ASSERT_TRUE(ret == 0 && strcmp(buf, "/proc/1234") == 0);
}

void test_build_proc_path_small_buffer(void)
{
    TEST("build_proc_path with small buffer");
    char buf[10];
    ASSERT_EQ(build_proc_path(1234, "status", buf, sizeof(buf)), -1);
}

/* Test pid_exists() */
void test_pid_exists_init(void)
{
    TEST("pid_exists with PID 1 (init/systemd)");
    ASSERT_TRUE(pid_exists(1));
}

void test_pid_exists_current(void)
{
    TEST("pid_exists with current process");
    ASSERT_TRUE(pid_exists(getpid()));
}

void test_pid_exists_invalid(void)
{
    TEST("pid_exists with non-existent PID");
    ASSERT_FALSE(pid_exists(999999));
}

/* Test state_to_string() */
void test_state_to_string_running(void)
{
    TEST("state_to_string with RUNNING");
    ASSERT_STR_EQ(state_to_string(PROC_STATE_RUNNING), "Running");
}

void test_state_to_string_sleeping(void)
{
    TEST("state_to_string with SLEEPING");
    ASSERT_STR_EQ(state_to_string(PROC_STATE_SLEEPING), "Sleeping");
}

void test_state_to_string_zombie(void)
{
    TEST("state_to_string with ZOMBIE");
    ASSERT_STR_EQ(state_to_string(PROC_STATE_ZOMBIE), "Zombie");
}

void test_state_to_string_unknown(void)
{
    TEST("state_to_string with UNKNOWN");
    ASSERT_STR_EQ(state_to_string(PROC_STATE_UNKNOWN), "Unknown");
}

/* Test char_to_state() */
void test_char_to_state_R(void)
{
    TEST("char_to_state with 'R'");
    ASSERT_EQ(char_to_state('R'), PROC_STATE_RUNNING);
}

void test_char_to_state_S(void)
{
    TEST("char_to_state with 'S'");
    ASSERT_EQ(char_to_state('S'), PROC_STATE_SLEEPING);
}

void test_char_to_state_Z(void)
{
    TEST("char_to_state with 'Z'");
    ASSERT_EQ(char_to_state('Z'), PROC_STATE_ZOMBIE);
}

void test_char_to_state_invalid(void)
{
    TEST("char_to_state with invalid char");
    ASSERT_EQ(char_to_state('X'), PROC_STATE_UNKNOWN);
}

int main(void)
{
    printf("\n=== Running Utility Function Tests ===\n\n");

    /* parse_pid tests */
    test_parse_pid_valid();
    test_parse_pid_current_shell();
    test_parse_pid_invalid();
    test_parse_pid_negative();
    test_parse_pid_zero();
    test_parse_pid_null();
    test_parse_pid_empty();

    /* build_proc_path tests */
    test_build_proc_path_with_file();
    test_build_proc_path_without_file();
    test_build_proc_path_small_buffer();

    /* pid_exists tests */
    test_pid_exists_init();
    test_pid_exists_current();
    test_pid_exists_invalid();

    /* state conversion tests */
    test_state_to_string_running();
    test_state_to_string_sleeping();
    test_state_to_string_zombie();
    test_state_to_string_unknown();
    test_char_to_state_R();
    test_char_to_state_S();
    test_char_to_state_Z();
    test_char_to_state_invalid();

    printf("\n=== Test Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);

    return tests_failed == 0 ? 0 : 1;
}
