/*
 * test_proc_fd.c - Unit tests for file descriptor enumeration
 *
 * Tests enumerate_fds(), fd_entries_free(), and parse_socket_inode()
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "../include/proc_fd.h"

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

/* Test enumerate_fds with current process */
void test_enumerate_fds_current(void)
{
    TEST("enumerate_fds with current process");
    fd_entry_t *entries = NULL;
    int count = 0;
    int ret = enumerate_fds(getpid(), &entries, &count);
    bool pass = (ret == 0 && entries != NULL && count >= 3); /* At least stdin, stdout, stderr */
    if (pass) {
        fd_entries_free(entries);
    }
    ASSERT_TRUE(pass);
}

/* Test that stdin/stdout/stderr are present */
void test_enumerate_fds_standard_fds(void)
{
    TEST("enumerate_fds includes stdin/stdout/stderr");
    fd_entry_t *entries = NULL;
    int count = 0;
    int ret = enumerate_fds(getpid(), &entries, &count);
    bool found_stdin = false, found_stdout = false, found_stderr = false;
    if (ret == 0 && entries != NULL) {
        for (int i = 0; i < count; i++) {
            if (entries[i].fd == 0) found_stdin = true;
            if (entries[i].fd == 1) found_stdout = true;
            if (entries[i].fd == 2) found_stderr = true;
        }
        fd_entries_free(entries);
    }
    ASSERT_TRUE(found_stdin && found_stdout && found_stderr);
}

/* Test FD entries have valid targets */
void test_enumerate_fds_has_targets(void)
{
    TEST("enumerate_fds entries have targets");
    fd_entry_t *entries = NULL;
    int count = 0;
    int ret = enumerate_fds(getpid(), &entries, &count);
    bool has_targets = true;
    if (ret == 0 && entries != NULL && count > 0) {
        for (int i = 0; i < count; i++) {
            if (strlen(entries[i].target) == 0) {
                has_targets = false;
                break;
            }
        }
        fd_entries_free(entries);
    } else {
        has_targets = false;
    }
    ASSERT_TRUE(has_targets);
}

/* Test enumerate_fds with PID 1 */
void test_enumerate_fds_pid1(void)
{
    TEST("enumerate_fds with PID 1");
    fd_entry_t *entries = NULL;
    int count = 0;
    int ret = enumerate_fds(1, &entries, &count);
    /* May succeed or fail with EACCES depending on permissions */
    bool pass = (ret == 0) || (ret == -1 && errno == EACCES);
    if (ret == 0) {
        fd_entries_free(entries);
    }
    ASSERT_TRUE(pass);
}

/* Test enumerate_fds with non-existent PID */
void test_enumerate_fds_nonexistent(void)
{
    TEST("enumerate_fds with non-existent PID");
    fd_entry_t *entries = NULL;
    int count = 0;
    int ret = enumerate_fds(999999, &entries, &count);
    ASSERT_TRUE(ret == -1 && errno == ENOENT);
}

/* Test enumerate_fds sets count to 0 on error */
void test_enumerate_fds_error_count(void)
{
    TEST("enumerate_fds sets count to 0 on error");
    fd_entry_t *entries = (fd_entry_t *)0xDEADBEEF;
    int count = 42;
    enumerate_fds(999999, &entries, &count);
    ASSERT_TRUE(entries == NULL && count == 0);
}

/* Test fd_entries_free with NULL is safe */
void test_fd_entries_free_null(void)
{
    TEST("fd_entries_free with NULL");
    fd_entries_free(NULL); /* Should not crash */
    ASSERT_TRUE(1);
}

/* Test parse_socket_inode with valid socket format */
void test_parse_socket_inode_valid(void)
{
    TEST("parse_socket_inode with valid socket");
    unsigned long inode = 0;
    bool result = parse_socket_inode("socket:[12345]", &inode);
    ASSERT_TRUE(result && inode == 12345);
}

/* Test parse_socket_inode with large inode */
void test_parse_socket_inode_large(void)
{
    TEST("parse_socket_inode with large inode");
    unsigned long inode = 0;
    bool result = parse_socket_inode("socket:[4294967295]", &inode);
    ASSERT_TRUE(result && inode == 4294967295UL);
}

/* Test parse_socket_inode with non-socket */
void test_parse_socket_inode_file(void)
{
    TEST("parse_socket_inode with regular file");
    unsigned long inode = 0;
    bool result = parse_socket_inode("/dev/pts/1", &inode);
    ASSERT_FALSE(result);
}

/* Test parse_socket_inode with pipe */
void test_parse_socket_inode_pipe(void)
{
    TEST("parse_socket_inode with pipe");
    unsigned long inode = 0;
    bool result = parse_socket_inode("pipe:[67890]", &inode);
    ASSERT_FALSE(result);
}

/* Test parse_socket_inode with NULL */
void test_parse_socket_inode_null(void)
{
    TEST("parse_socket_inode with NULL");
    unsigned long inode = 0;
    bool result = parse_socket_inode(NULL, &inode);
    ASSERT_FALSE(result);
}

int main(void)
{
    printf("\n=== Running File Descriptor Tests ===\n\n");

    /* enumerate_fds tests */
    test_enumerate_fds_current();
    test_enumerate_fds_standard_fds();
    test_enumerate_fds_has_targets();
    test_enumerate_fds_pid1();
    test_enumerate_fds_nonexistent();
    test_enumerate_fds_error_count();
    test_fd_entries_free_null();

    /* parse_socket_inode tests */
    test_parse_socket_inode_valid();
    test_parse_socket_inode_large();
    test_parse_socket_inode_file();
    test_parse_socket_inode_pipe();
    test_parse_socket_inode_null();

    printf("\n=== Test Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);

    return tests_failed == 0 ? 0 : 1;
}
