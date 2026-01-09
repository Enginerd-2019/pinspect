/*
 * test_proc_status.c - Unit tests for /proc/<PID>/status parsing
 *
 * Tests read_proc_status() with real processes
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../include/proc_status.h"
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

#define ASSERT_TRUE(condition, msg) \
    do { \
        if (condition) { \
            printf("%s\n", TEST_PASS); \
            tests_passed++; \
        } else { \
            printf("%s (%s)\n", TEST_FAIL, msg); \
            tests_failed++; \
        } \
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

/* Test reading current process */
void test_read_current_process(void)
{
    TEST("read_proc_status on current process");
    proc_info_t info;
    pid_t current_pid = getpid();

    int ret = read_proc_status(current_pid, &info);
    ASSERT_TRUE(ret == 0 && info.pid == current_pid, "failed to read or PID mismatch");
}

/* Test that name is populated */
void test_name_populated(void)
{
    TEST("process name is populated");
    proc_info_t info;
    int ret = read_proc_status(getpid(), &info);
    ASSERT_TRUE(ret == 0 && strlen(info.name) > 0, "name is empty");
}

/* Test that state is valid */
void test_state_valid(void)
{
    TEST("process state is valid");
    proc_info_t info;
    int ret = read_proc_status(getpid(), &info);
    ASSERT_TRUE(ret == 0 && info.state != PROC_STATE_UNKNOWN,
                "state is UNKNOWN");
}

/* Test that UIDs are set */
void test_uids_set(void)
{
    TEST("UIDs are set");
    proc_info_t info;
    int ret = read_proc_status(getpid(), &info);
    ASSERT_TRUE(ret == 0 && info.uid_real > 0 && info.uid_effective > 0,
                "UIDs not set");
}

/* Test that GIDs are set */
void test_gids_set(void)
{
    TEST("GIDs are set");
    proc_info_t info;
    int ret = read_proc_status(getpid(), &info);
    ASSERT_TRUE(ret == 0 && info.gid_real > 0 && info.gid_effective > 0,
                "GIDs not set");
}

/* Test that thread count is positive */
void test_thread_count(void)
{
    TEST("thread count is positive");
    proc_info_t info;
    int ret = read_proc_status(getpid(), &info);
    ASSERT_TRUE(ret == 0 && info.thread_count > 0,
                "thread count is not positive");
}

/* Test init/systemd process (PID 1) */
void test_read_init_process(void)
{
    TEST("read_proc_status on PID 1");
    proc_info_t info;
    int ret = read_proc_status(1, &info);
    ASSERT_TRUE(ret == 0 && info.pid == 1, "failed to read PID 1");
}

/* Test non-existent process */
void test_read_nonexistent_process(void)
{
    TEST("read_proc_status on non-existent PID");
    proc_info_t info;
    int ret = read_proc_status(999999, &info);
    ASSERT_EQ(ret, -1);
}

/* Test that errno is set correctly for non-existent */
void test_errno_nonexistent(void)
{
    TEST("errno is ENOENT for non-existent PID");
    proc_info_t info;
    errno = 0;
    read_proc_status(999999, &info);
    ASSERT_EQ(errno, ENOENT);
}

int main(void)
{
    printf("\n=== Running proc_status Tests ===\n\n");

    test_read_current_process();
    test_name_populated();
    test_state_valid();
    test_uids_set();
    test_gids_set();
    test_thread_count();
    test_read_init_process();
    test_read_nonexistent_process();
    test_errno_nonexistent();

    printf("\n=== Test Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);

    return tests_failed == 0 ? 0 : 1;
}
