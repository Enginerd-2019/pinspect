/*
 * test_net.c - Unit tests for network connection parsing
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/net.h"

/*Test macros*/
#define TEST_PASS "\33[32m[PASS]\33[0m"
#define TEST_FAIL "\33[31m[PASS]\33[0m"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name)\
    do{\
        printf("Testing %s... ", name);\
        fflush(stdout);\
    }while(0)

#define ASSERT_TRUE(condition) \
    do{\
        if(condition){\
            printf("%s\n", TEST_PASS); \
            tests_passed++; \
        }else{ \
            printf("%s (condition was false)\n", TEST_FAIL); \
            tests_failed++; \
        }\
    }while(0)

#define ASSERT_STREQ(actual, expected) \
    do { \
        if (strcmp(actual, expected) == 0) { \
            printf("%s\n", TEST_PASS); \
            tests_passed++; \
        } else { \
            printf("%s (expected \"%s\", got \"%s\")\n", \
                   TEST_FAIL, expected, actual); \
            tests_failed++; \
        } \
    } while(0)

/* Test tcp_state_to_string */
void test_tcp_state_established(void)
{
    TEST("tcp_state_to_string ESTABLISHED");
    ASSERT_STREQ(tcp_state_to_string(TCP_ESTABLISHED), "ESTABLISHED");
}

void test_tcp_state_listen(void)
{
    TEST("tcp_state_to_string LISTEN");
    ASSERT_STREQ(tcp_state_to_string(TCP_LISTEN), "LISTEN");
}

void test_tcp_state_unknown(void)
{
    TEST("tcp_state_to_string unknown value");
    ASSERT_STREQ(tcp_state_to_string((tcp_state_t)99), "UNKNOWN");
}

/* Test format_ip_port */
void test_format_ip_port_localhost(void)
{
    TEST("format_ip_port localhost:8080");
    char buf[32];
    uint32_t ip = htonl(0x7F000001);  /* 127.0.0.1 */
    format_ip_port(ip, 8080, buf, sizeof(buf));
    ASSERT_STREQ(buf, "127.0.0.1:8080");
}

void test_format_ip_port_any(void)
{
    TEST("format_ip_port 0.0.0.0:22");
    char buf[32];
    format_ip_port(0, 22, buf, sizeof(buf));
    ASSERT_STREQ(buf, "0.0.0.0:22");
}

void test_format_ip_port_null_buffer(void)
{
    TEST("format_ip_port with NULL buffer");
    format_ip_port(0, 22, NULL, 0);  /* Should not crash */
    ASSERT_TRUE(1);
}

/* Test find_process_sockets */
void test_find_process_sockets_current(void)
{
    TEST("find_process_sockets current process");
    socket_info_t *sockets = NULL;
    int count = 0;
    int ret = find_process_sockets(getpid(), &sockets, &count);
    /* Current process may or may not have sockets */
    bool pass = (ret == 0);  /* Should succeed */
    if (sockets != NULL) {
        socket_list_free(sockets);
    }
    ASSERT_TRUE(pass);
}

void test_find_process_sockets_nonexistent(void)
{
    TEST("find_process_sockets nonexistent PID");
    socket_info_t *sockets = NULL;
    int count = 0;
    int ret = find_process_sockets(999999, &sockets, &count);
    ASSERT_TRUE(ret == -1);
}

void test_socket_list_free_null(void)
{
    TEST("socket_list_free with NULL");
    socket_list_free(NULL);  /* Should not crash */
    ASSERT_TRUE(1);
}

int main(void)
{
    printf("\n=== Running Network Connection Tests ===\n\n");

    /* tcp_state_to_string tests */
    test_tcp_state_established();
    test_tcp_state_listen();
    test_tcp_state_unknown();

    /* format_ip_port tests */
    test_format_ip_port_localhost();
    test_format_ip_port_any();
    test_format_ip_port_null_buffer();

    /* find_process_sockets tests */
    test_find_process_sockets_current();
    test_find_process_sockets_nonexistent();
    test_socket_list_free_null();

    printf("\n=== Test Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);

    return tests_failed == 0 ? 0 : 1;
}