/*
 * test_proc_task.c - Unit tests for thread enumeration
 *
 * Tests enumerate_threads() and thread_info_free()
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include "../include/proc_task.h"
#include "../include/util.h"

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

/* Test enumerate_threads with current process (single-threaded) */
void test_enumerate_threads_current(void)
{
    TEST("enumerate_threads with current process");
    thread_info_t *threads = NULL;
    int count = 0;
    int ret = enumerate_threads(getpid(), &threads, &count);
    bool pass = (ret == 0 && threads != NULL && count >= 1);
    if (pass) {
        thread_info_free(threads);
    }
    ASSERT_TRUE(pass);
}

/* Test that main thread TID equals PID */
void test_enumerate_threads_main_tid(void)
{
    TEST("enumerate_threads main thread TID equals PID");
    thread_info_t *threads = NULL;
    int count = 0;
    pid_t pid = getpid();
    int ret = enumerate_threads(pid, &threads, &count);
    bool found_main = false;
    if (ret == 0 && threads != NULL) {
        for (int i = 0; i < count; i++) {
            if (threads[i].tid == pid) {
                found_main = true;
                break;
            }
        }
        thread_info_free(threads);
    }
    ASSERT_TRUE(found_main);
}

/* Test thread has valid name */
void test_enumerate_threads_has_name(void)
{
    TEST("enumerate_threads thread has name");
    thread_info_t *threads = NULL;
    int count = 0;
    int ret = enumerate_threads(getpid(), &threads, &count);
    bool has_name = false;
    if (ret == 0 && threads != NULL && count > 0) {
        has_name = (strlen(threads[0].name) > 0);
        thread_info_free(threads);
    }
    ASSERT_TRUE(has_name);
}

/* Test thread has valid state */
void test_enumerate_threads_has_state(void)
{
    TEST("enumerate_threads thread has valid state");
    thread_info_t *threads = NULL;
    int count = 0;
    int ret = enumerate_threads(getpid(), &threads, &count);
    bool valid_state = false;
    if (ret == 0 && threads != NULL && count > 0) {
        /* State should be one of the known states */
        valid_state = (threads[0].state >= PROC_STATE_RUNNING &&
                       threads[0].state <= PROC_STATE_UNKNOWN);
        thread_info_free(threads);
    }
    ASSERT_TRUE(valid_state);
}

/* Test enumerate_threads with PID 1 (init/systemd) */
void test_enumerate_threads_pid1(void)
{
    TEST("enumerate_threads with PID 1");
    thread_info_t *threads = NULL;
    int count = 0;
    int ret = enumerate_threads(1, &threads, &count);
    /* May succeed or fail with EACCES depending on permissions */
    bool pass = (ret == 0 && count >= 1) || (ret == -1 && errno == EACCES);
    if (ret == 0) {
        thread_info_free(threads);
    }
    ASSERT_TRUE(pass);
}

/* Test enumerate_threads with non-existent PID */
void test_enumerate_threads_nonexistent(void)
{
    TEST("enumerate_threads with non-existent PID");
    thread_info_t *threads = NULL;
    int count = 0;
    int ret = enumerate_threads(999999, &threads, &count);
    ASSERT_TRUE(ret == -1 && errno == ENOENT);
}

/* Test enumerate_threads sets count to 0 on error */
void test_enumerate_threads_error_count(void)
{
    TEST("enumerate_threads sets count to 0 on error");
    thread_info_t *threads = (thread_info_t *)0xDEADBEEF; /* Garbage pointer */
    int count = 42;
    enumerate_threads(999999, &threads, &count);
    ASSERT_TRUE(threads == NULL && count == 0);
}

/* Test thread_info_free with NULL is safe */
void test_thread_info_free_null(void)
{
    TEST("thread_info_free with NULL");
    thread_info_free(NULL); /* Should not crash */
    ASSERT_TRUE(1); /* If we got here, it didn't crash */
}

int main(void)
{
    printf("\n=== Running Thread Enumeration Tests ===\n\n");

    test_enumerate_threads_current();
    test_enumerate_threads_main_tid();
    test_enumerate_threads_has_name();
    test_enumerate_threads_has_state();
    test_enumerate_threads_pid1();
    test_enumerate_threads_nonexistent();
    test_enumerate_threads_error_count();
    test_thread_info_free_null();

    printf("\n=== Test Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);

    return tests_failed == 0 ? 0 : 1;
}
